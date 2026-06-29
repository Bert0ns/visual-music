#pragma once
#include <stdint.h>
#include <stdbool.h>

#if _WIN32
#define FFI_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FFI_PLUGIN_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Initialize projectM and return a handle.
// Must be called on a thread with an active OpenGL context.
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

#ifdef __cplusplus
}
#endif
