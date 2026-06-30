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

static ma_device g_audio_device;
static std::atomic_bool g_audio_running{false};
static std::atomic_bool g_audio_device_ready{false};
static void* g_ffi_handle = nullptr;

void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    // pInput contains the captured audio
    projectm_handle pm_handle = projectm_ffi_native_handle(g_ffi_handle);
    if (pInput == nullptr || pm_handle == nullptr) return;

    // We configured miniaudio for f32 format, so we can cast directly.
    const float* pSampleData = (const float*)pInput;

    // Add audio to projectM. 
    // projectM expects samples to be within the range -1 to 1.
    // If it's stereo, we pass PROJECTM_STEREO. Our miniaudio is set to stereo below.
    projectm_pcm_add_float(pm_handle, pSampleData, frameCount, PROJECTM_STEREO);
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
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
        deviceConfig.capture.format   = ma_format_f32;
        deviceConfig.capture.channels = 2;
        deviceConfig.sampleRate       = 44100;
        deviceConfig.dataCallback     = audio_data_callback;
        deviceConfig.pUserData        = nullptr;

        if (ma_device_init(NULL, &deviceConfig, &g_audio_device) != MA_SUCCESS) {
            std::cerr << "Failed to initialize capture device. Falling back to synthetic beat generator." << std::endl;

            int sampleRate = 44100;
            int framesPerBuffer = 512;
            std::vector<float> buffer(framesPerBuffer * 2); // Stereo
            
            double phase = 0.0;
            double beatPhase = 0.0;
            
            while (g_audio_running.load()) {
                for (int i = 0; i < framesPerBuffer; i++) {
                    // 120 BPM = 2 beats per second = 2 Hz for the envelope
                    beatPhase += 2.0 * 3.14159265358979323846 * 2.0 / sampleRate;
                    if (beatPhase > 2.0 * 3.14159265358979323846) beatPhase -= 2.0 * 3.14159265358979323846;
                    
                    // Base frequency (60Hz bass)
                    phase += 2.0 * 3.14159265358979323846 * 60.0 / sampleRate;
                    if (phase > 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;
                    
                    // Envelope shape (sharp attack, exponential decay)
                    float env = std::pow(std::max(0.0, std::sin(beatPhase)), 8.0);
                    
                    float sample = std::sin(phase) * env;
                    
                    buffer[i*2] = sample;     // Left
                    buffer[i*2+1] = sample;   // Right
                }

                projectm_handle pm_handle = projectm_ffi_native_handle(g_ffi_handle);
                if (pm_handle) {
                    projectm_pcm_add_float(pm_handle, buffer.data(), framesPerBuffer, PROJECTM_STEREO);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(framesPerBuffer * 1000 / sampleRate));
            }
            return;
        }

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

} // extern "C"
