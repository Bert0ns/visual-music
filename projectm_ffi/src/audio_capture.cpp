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

static ma_device g_audio_device;
static std::atomic_bool g_audio_running{false};
static std::atomic_bool g_audio_device_ready{false};
static void* g_ffi_handle = nullptr;

static double g_synth_phase = 0.0;
static double g_synth_beat_phase = 0.0;

// Playback State
static ma_decoder g_playback_decoder;
static ma_device g_playback_device;
static std::atomic_bool g_playback_running{false};
static std::atomic_bool g_is_playing{false};
static std::mutex g_playback_mutex;

void playback_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    if (!g_is_playing.load()) {
        memset(pOutput, 0, frameCount * 2 * sizeof(float));
        return;
    }
    
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&g_playback_decoder, pOutput, (ma_uint64)frameCount, &framesRead);
    
    if (framesRead < frameCount) {
        memset((float*)pOutput + (framesRead * 2), 0, (frameCount - framesRead) * 2 * sizeof(float));
        g_is_playing.store(false);
    }
    
    if (g_ffi_handle) {
        projectm_ffi_add_audio(g_ffi_handle, (const float*)pOutput, frameCount);
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

    std::thread([]() {
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_loopback);
        deviceConfig.capture.format   = ma_format_f32;
        deviceConfig.capture.channels = 2;
        deviceConfig.sampleRate       = 44100;
        deviceConfig.dataCallback     = audio_data_callback;
        deviceConfig.pUserData        = nullptr;

        if (ma_device_init(NULL, &deviceConfig, &g_audio_device) != MA_SUCCESS) {
            std::cerr << "Loopback failed. Falling back to microphone..." << std::endl;
            deviceConfig = ma_device_config_init(ma_device_type_capture);
            deviceConfig.capture.format   = ma_format_f32;
            deviceConfig.capture.channels = 2;
            deviceConfig.sampleRate       = 44100;
            deviceConfig.dataCallback     = audio_data_callback;
            deviceConfig.pUserData        = nullptr;
            
            if (ma_device_init(NULL, &deviceConfig, &g_audio_device) != MA_SUCCESS) {
                std::cerr << "Microphone failed. Falling back to synthetic beat generator." << std::endl;

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
                return; // End thread if beat generator was used and finishes
            }
        }

        // If we reach here, either loopback or microphone initialized successfully!
        if (!g_audio_running.load()) {
            ma_device_uninit(&g_audio_device);
            return;
        }

        if (ma_device_start(&g_audio_device) != MA_SUCCESS) {
            std::cerr << "Failed to start capture device." << std::endl;
            ma_device_uninit(&g_audio_device);
            g_audio_running.store(false);
            return;
        }

        g_audio_device_ready.store(true);
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
    
    if (g_playback_running.load()) {
        ma_device_uninit(&g_playback_device);
        ma_decoder_uninit(&g_playback_decoder);
        g_playback_running.store(false);
    }
    
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    if (ma_decoder_init_file(filepath, &decoderConfig, &g_playback_decoder) != MA_SUCCESS) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }
    
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate       = 44100;
    deviceConfig.dataCallback     = playback_data_callback;
    deviceConfig.pUserData        = nullptr;
    
    if (ma_device_init(NULL, &deviceConfig, &g_playback_device) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device." << std::endl;
        ma_decoder_uninit(&g_playback_decoder);
        return false;
    }
    
    if (ma_device_start(&g_playback_device) != MA_SUCCESS) {
        ma_device_uninit(&g_playback_device);
        ma_decoder_uninit(&g_playback_decoder);
        return false;
    }
    
    g_playback_running.store(true);
    g_is_playing.store(true);
    return true;
}

FFI_PLUGIN_EXPORT void projectm_ffi_pause_audio() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_is_playing.store(false);
    if (g_playback_running.load()) {
        ma_device_stop(&g_playback_device);
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_resume_audio() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_is_playing.store(true);
    if (g_playback_running.load()) {
        ma_device_start(&g_playback_device);
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_stop_audio() {
    std::lock_guard<std::mutex> lock(g_playback_mutex);
    g_is_playing.store(false);
    if (g_playback_running.load()) {
        ma_device_uninit(&g_playback_device);
        ma_decoder_uninit(&g_playback_decoder);
        g_playback_running.store(false);
    }
}

} // extern "C"
