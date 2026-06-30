#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <mutex>
#include <string>
#include <projectM-4/projectM.h>

struct ProjectmFfiState {
    projectm_handle instance{nullptr};
    uint32_t width{800};
    uint32_t height{600};
    std::string pending_preset_path;
    bool has_pending_preset{false};
    bool pending_smooth_transition{true};
    std::mutex mutex;
};

ProjectmFfiState* projectm_ffi_state_from_handle(void* handle);
#endif

#if _WIN32
#define FFI_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FFI_PLUGIN_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Add audio data thread-safely
FFI_PLUGIN_EXPORT void projectm_ffi_add_audio(void* handle, const float* data, int frameCount);

// Initialize wrapper state and return a handle.
// The underlying projectM instance is created lazily on the render thread,
// where Flutter's OpenGL context is current.
FFI_PLUGIN_EXPORT void* projectm_ffi_init();

// Destroy the projectM instance
FFI_PLUGIN_EXPORT void projectm_ffi_destroy(void* handle);

// Set the render window size
FFI_PLUGIN_EXPORT void projectm_ffi_set_window_size(void* handle, int width, int height);

// Render a single frame into the current OpenGL context
FFI_PLUGIN_EXPORT void projectm_ffi_render_frame(void* handle, uint32_t width, uint32_t height);

// Load a preset file (.milk)
FFI_PLUGIN_EXPORT void projectm_ffi_load_preset(void* handle, const char* path, bool smooth_transition);

// Start capturing audio from the default microphone and sending it to the projectM instance
FFI_PLUGIN_EXPORT bool projectm_ffi_start_audio_capture(void* handle);

// Stop audio capture
FFI_PLUGIN_EXPORT void projectm_ffi_stop_audio_capture();

FFI_PLUGIN_EXPORT bool projectm_ffi_play_file(void* handle, const char* filepath);
FFI_PLUGIN_EXPORT void projectm_ffi_pause_audio();
FFI_PLUGIN_EXPORT void projectm_ffi_resume_audio();
FFI_PLUGIN_EXPORT void projectm_ffi_stop_audio();

#ifdef __cplusplus
}
#endif
