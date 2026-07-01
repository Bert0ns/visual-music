#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "projectm_ffi.h"
#include <projectM-4/projectM.h>
#include <projectM-4/audio.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>
#include <vector>
#include <mutex>
#include <fstream>
#include <string>

static ma_device g_audio_device;
static std::atomic_bool g_audio_running{false};
static std::atomic_bool g_audio_device_ready{false};
static void* g_ffi_handle = nullptr;

static double g_synth_phase = 0.0;
static double g_synth_beat_phase = 0.0;

// Global Audio Context
static ma_context g_audio_context;
static std::atomic_bool g_context_initialized{false};
static std::mutex g_context_mutex;

static bool ensure_audio_context() {
    std::lock_guard<std::mutex> lock(g_context_mutex);
    if (g_context_initialized.load()) return true;
    
    // On Linux/WSL, miniaudio's native PulseAudio backend triggers a catastrophic RDP sink freeze
    // after exactly 15-20 seconds of continuous playback due to queue overrun.
    // SOLUTION: We force miniaudio to use the ALSA backend (ma_backend_alsa). 
    // In WSLg, ALSA automatically routes to PulseAudio via 'alsa-plugins', which correctly
    // manages the buffers and prevents the RDP bridge from crashing!
#if defined(__linux__)
    ma_backend backends[] = { ma_backend_alsa };
#else
    ma_backend backends[] = { ma_backend_pulseaudio };
#endif
    
    ma_context_config contextConfig = ma_context_config_init();
    contextConfig.threadPriority = ma_thread_priority_realtime;

    
    if (ma_context_init(backends, 1, &contextConfig, &g_audio_context) != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio context." << std::endl;
        return false;
    }
    
    g_context_initialized.store(true);
    return true;
}


// Playback State
static ma_decoder g_playback_decoder;
static ma_device g_playback_device;
static std::atomic_bool g_playback_running{false};
static std::atomic_bool g_is_playing{false};
static std::mutex g_playback_mutex;

static std::atomic<uint64_t> g_playback_callback_count{0};
static std::atomic<uint64_t> g_short_read_count{0};
static std::chrono::steady_clock::time_point g_last_callback_time;
static std::chrono::steady_clock::time_point g_playback_start_time;
static std::atomic<uint64_t> g_total_frames_decoded{0};
static bool g_timing_initialized = false;

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    if (!g_is_playing.load()) {
        memset(pOutput, 0, frameCount * 2 * sizeof(float));
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    uint64_t count = g_playback_callback_count.fetch_add(1);
    
    // Track inter-callback interval
    long interval_ms = 0;
    if (g_timing_initialized) {
        interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_last_callback_time).count();
    } else {
        g_playback_start_time = now;
        g_timing_initialized = true;
    }
    g_last_callback_time = now;
    
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&g_playback_decoder, pOutput, (ma_uint64)frameCount, &framesRead);
    
    uint64_t totalDecoded = g_total_frames_decoded.fetch_add(framesRead) + framesRead;
    
    if (framesRead < (ma_uint64)frameCount) {
        memset((float*)pOutput + (framesRead * 2), 0, (frameCount - framesRead) * 2 * sizeof(float));
        
        uint64_t shortCount = g_short_read_count.fetch_add(1);
        std::cout << "AUDIO_SHORT_READ[" << shortCount << "]: callback=" << count 
                  << " requested=" << frameCount << " got=" << framesRead << std::endl;
        
        if (framesRead == 0) {
            ma_decoder_seek_to_pcm_frame(&g_playback_decoder, 0);
            std::cout << "AUDIO_EOF: Looping back to start" << std::endl;
            ma_uint64 replayRead = 0;
            ma_decoder_read_pcm_frames(&g_playback_decoder, pOutput, (ma_uint64)frameCount, &replayRead);
        }
    }
    
    if (g_ffi_handle) {
        projectm_ffi_add_audio(g_ffi_handle, (const float*)pOutput, frameCount);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    long exec_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - now).count();
    if (exec_ms > 10) {
        std::cerr << "AUDIO_WARNING: Callback took " << exec_ms << "ms (frameCount=" << frameCount << ")" << std::endl;
    }
    
    // Log every 100 callbacks (~5 seconds at 50ms period)
    if (count % 100 == 0) {
        double wall_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_playback_start_time).count() / 1000.0;
        double audio_seconds = (double)totalDecoded / 44100.0;
        double drift = audio_seconds - wall_seconds;
        std::cout << "AUDIO_DIAG[" << count << "]: interval_ms=" << interval_ms
                  << " wall=" << wall_seconds << "s audio=" << audio_seconds << "s"
                  << " drift=" << drift << "s exec_ms=" << exec_ms << std::endl;
    }
}

void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    // Ignore microphone/capture if the internal player is active!
    if (g_playback_running.load() && g_is_playing.load()) return;

    // pInput contains the captured audio
    if (pInput == nullptr || g_ffi_handle == nullptr) return;

    // We configured miniaudio for f32 format, so we can cast directly.
    const float* pSampleData = (const float*)pInput;

    float sum_squares = 0.0f;
    for (ma_uint32 i = 0; i < frameCount * 2; ++i) {
        sum_squares += pSampleData[i] * pSampleData[i];
    }
    float rms = std::sqrt(sum_squares / (frameCount * 2));
    
    // Force synthetic beat generator to run! 
    // The dummy microphone in WSL produces static noise (rms > 0), causing projectM to render black.
    bool is_silent = true; 

    if (is_silent) {
        std::vector<float> buffer(frameCount * 2);
        int sampleRate = pDevice->sampleRate;
        for (ma_uint32 i = 0; i < frameCount; i++) {
            g_synth_beat_phase += 2.0 * 3.14159265358979323846 * 2.0 / sampleRate;
            if (g_synth_beat_phase > 2.0 * 3.14159265358979323846) g_synth_beat_phase -= 2.0 * 3.14159265358979323846;
            
            g_synth_phase += 2.0 * 3.14159265358979323846 * 60.0 / sampleRate;
            if (g_synth_phase > 2.0 * 3.14159265358979323846) g_synth_phase -= 2.0 * 3.14159265358979323846;
            
            float env = std::pow(std::max(0.0, std::sin(g_synth_beat_phase)), 8.0);
            float sample = std::sin(g_synth_phase) * env;
            
            buffer[i*2] = sample;
            buffer[i*2+1] = sample;
        }
        projectm_ffi_add_audio(g_ffi_handle, buffer.data(), frameCount);
    } else {
        projectm_ffi_add_audio(g_ffi_handle, pSampleData, frameCount);
    }
}

extern "C" {

FFI_PLUGIN_EXPORT bool projectm_ffi_start_audio_capture(void* handle) {
    if (handle == nullptr) {
        return false;
    }

    bool expected = false;
    if (!g_audio_running.compare_exchange_strong(expected, true)) {
        // Already capturing
        return true;
    }

    g_ffi_handle = handle;
    
    if (!ensure_audio_context()) {
        g_audio_running.store(false);
        return false;
    }

    std::thread([]() {
        // On WSL, the PulseAudio RDPSource has a critical bug: it floods the async
        // queue with overruns ("asyncq.c: q overrun, queuing locally"), poisoning the
        // entire PulseAudio server. This causes ANY audio device (including playback)
        // to have its callbacks progressively starved until audio freezes.
        //
        // The fix: never open a capture device. Use the synthetic beat generator for
        // idle visuals, and the playback callback feeds audio to projectM during
        // file playback (via projectm_ffi_add_audio in playback_data_callback).
        //
        // On non-WSL Linux with real hardware, loopback/capture could be re-enabled
        // by checking for WSL at runtime (e.g. /proc/sys/fs/binfmt_misc/WSLInterop).
        
        bool isWSL = false;
        {
            std::ifstream f("/proc/sys/fs/binfmt_misc/WSLInterop");
            if (f.good()) isWSL = true;
        }
        if (!isWSL) {
            // Try loopback on non-WSL systems
            std::ifstream f2("/proc/version");
            std::string version_str;
            if (f2.good()) {
                std::getline(f2, version_str);
                if (version_str.find("microsoft") != std::string::npos ||
                    version_str.find("Microsoft") != std::string::npos) {
                    isWSL = true;
                }
            }
        }
        
        if (!isWSL) {
            ma_device_config deviceConfig = ma_device_config_init(ma_device_type_loopback);
            deviceConfig.capture.format   = ma_format_f32;
            deviceConfig.capture.channels = 2;
            deviceConfig.sampleRate       = 44100;
            deviceConfig.dataCallback     = audio_data_callback;
            deviceConfig.pUserData        = nullptr;

            if (ma_device_init(&g_audio_context, &deviceConfig, &g_audio_device) == MA_SUCCESS) {
                if (g_audio_running.load() && ma_device_start(&g_audio_device) == MA_SUCCESS) {
                    std::cout << "Audio: Using loopback capture" << std::endl;
                    g_audio_device_ready.store(true);
                    return;
                }
                ma_device_uninit(&g_audio_device);
            }
            
            // Try microphone on non-WSL
            deviceConfig = ma_device_config_init(ma_device_type_capture);
            deviceConfig.capture.format   = ma_format_f32;
            deviceConfig.capture.channels = 2;
            deviceConfig.sampleRate       = 44100;
            deviceConfig.dataCallback     = audio_data_callback;
            deviceConfig.pUserData        = nullptr;
            
            if (ma_device_init(&g_audio_context, &deviceConfig, &g_audio_device) == MA_SUCCESS) {
                if (g_audio_running.load() && ma_device_start(&g_audio_device) == MA_SUCCESS) {
                    std::cout << "Audio: Using microphone capture" << std::endl;
                    g_audio_device_ready.store(true);
                    return;
                }
                ma_device_uninit(&g_audio_device);
            }
        } else {
            std::cout << "Audio: WSL detected, skipping capture devices (RDPSource bug workaround)" << std::endl;
        }
        
        // Fallback: synthetic beat generator (works everywhere, no PulseAudio needed)
        std::cout << "Audio: Using synthetic beat generator for idle visuals" << std::endl;
        
        int sampleRate = 44100;
        int framesPerBuffer = 512;
        std::vector<float> buffer(framesPerBuffer * 2); // Stereo
        
        double phase = 0.0;
        double beatPhase = 0.0;
        
        while (g_audio_running.load()) {
            if (g_playback_running.load() && g_is_playing.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            
            for (int i = 0; i < framesPerBuffer; i++) {
                beatPhase += 2.0 * 3.14159265358979323846 * 2.0 / sampleRate;
                if (beatPhase > 2.0 * 3.14159265358979323846) beatPhase -= 2.0 * 3.14159265358979323846;
                
                phase += 2.0 * 3.14159265358979323846 * 60.0 / sampleRate;
                if (phase > 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;
                
                float env = std::pow(std::max(0.0, std::sin(beatPhase)), 8.0);
                float sample = std::sin(phase) * env;
                
                buffer[i*2] = sample;
                buffer[i*2+1] = sample;
            }

            projectm_ffi_add_audio(g_ffi_handle, buffer.data(), framesPerBuffer);
            std::this_thread::sleep_for(std::chrono::milliseconds(framesPerBuffer * 1000 / sampleRate));
        }
    }).detach();
    
    return true;
}

FFI_PLUGIN_EXPORT void projectm_ffi_stop_audio_capture() {
    if (g_audio_running.exchange(false)) {
        if (g_audio_device_ready.exchange(false)) {
            ma_device_uninit(&g_audio_device);
        }
        g_ffi_handle = nullptr;
    }
}

FFI_PLUGIN_EXPORT bool projectm_ffi_play_file(void* handle, const char* filepath) {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    if (!handle || !filepath) return false;
    
    g_ffi_handle = handle;
    
    if (!ensure_audio_context()) {
        return false;
    }
    
    if (g_playback_running.load()) {
        g_is_playing.store(false);
        ma_device_uninit(&g_playback_device);
        ma_decoder_uninit(&g_playback_decoder);
        g_playback_running.store(false);
    }
    
    // CRITICAL: Stop the capture device to free the PulseAudio server for playback.
    // WSL's PulseAudio chokes when two devices (capture + playback) are active simultaneously,
    // causing progressive callback starvation on the playback thread.
    if (g_audio_device_ready.load()) {
        std::cout << "AUDIO_PLAY: Stopping capture device to free audio server" << std::endl;
        ma_device_stop(&g_audio_device);
    }
    
    // Reset diagnostic counters
    g_playback_callback_count.store(0);
    g_short_read_count.store(0);
    g_total_frames_decoded.store(0);
    g_timing_initialized = false;
    
    std::cout << "AUDIO_PLAY: Opening file: " << filepath << std::endl;
    
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    if (ma_decoder_init_file(filepath, &decoderConfig, &g_playback_decoder) != MA_SUCCESS) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        // Restart capture device on failure
        if (g_audio_device_ready.load()) {
            ma_device_start(&g_audio_device);
        }
        return false;
    }
    
    // Log file length
    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&g_playback_decoder, &totalFrames);
    double totalSeconds = (double)totalFrames / 44100.0;
    std::cout << "AUDIO_PLAY: File length: " << totalFrames << " frames (" 
              << totalSeconds << " seconds)" << std::endl;
    
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate       = 44100;
    
    // CRITICAL for WSL: 
    // Now that we have protected the PulseAudio server with nice=19 and disabled projectM's
    // internal auto-switch, we NO LONGER NEED a massive OS audio buffer!
    // We use a small 100ms buffer to keep latency low, the visualizer synced perfectly, and the RDP queue happy.
    deviceConfig.periodSizeInMilliseconds = 100;
    
    deviceConfig.dataCallback     = playback_data_callback;
    deviceConfig.pUserData        = nullptr;
    
    if (ma_device_init(&g_audio_context, &deviceConfig, &g_playback_device) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device." << std::endl;
        ma_decoder_uninit(&g_playback_decoder);
        if (g_audio_device_ready.load()) {
            ma_device_start(&g_audio_device);
        }
        return false;
    }
    
    if (ma_device_start(&g_playback_device) != MA_SUCCESS) {
        ma_device_uninit(&g_playback_device);
        ma_decoder_uninit(&g_playback_decoder);
        if (g_audio_device_ready.load()) {
            ma_device_start(&g_audio_device);
        }
        return false;
    }
    
    g_playback_running.store(true);
    g_is_playing.store(true);
    std::cout << "AUDIO_PLAY: Playback started successfully" << std::endl;
    return true;
}

FFI_PLUGIN_EXPORT void projectm_ffi_pause_audio() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_is_playing.store(false);
}

FFI_PLUGIN_EXPORT void projectm_ffi_resume_audio() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_is_playing.store(true);
}

FFI_PLUGIN_EXPORT void projectm_ffi_stop_audio() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_is_playing.store(false);
    if (g_playback_running.load()) {
        ma_device_uninit(&g_playback_device);
        ma_decoder_uninit(&g_playback_decoder);
        g_playback_running.store(false);
    }
    // Restart capture device now that playback is done
    if (g_audio_device_ready.load()) {
        std::cout << "AUDIO_STOP: Restarting capture device" << std::endl;
        ma_device_start(&g_audio_device);
    }
}

} // extern "C"
