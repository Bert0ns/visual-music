#include "projectm_ffi.h"
#include <projectM-4/projectM.h>
#include <stddef.h>

extern "C" {

FFI_PLUGIN_EXPORT void* projectm_ffi_init() {
    // projectm_create requires an active OpenGL context!
    // In our case, the Flutter runner will ensure a context is current 
    // before calling this via FFI or before rendering.
    projectm_handle pm = projectm_create();
    return static_cast<void*>(pm);
}

FFI_PLUGIN_EXPORT void projectm_ffi_destroy(void* handle) {
    if (handle) {
        projectm_destroy(static_cast<projectm_handle>(handle));
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_set_window_size(void* handle, int width, int height) {
    if (handle) {
        projectm_set_window_size(static_cast<projectm_handle>(handle), width, height);
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_render_frame(void* handle) {
    if (handle) {
        projectm_opengl_render_frame(static_cast<projectm_handle>(handle));
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_load_preset(void* handle, const char* path, bool smooth_transition) {
    if (handle && path) {
        projectm_load_preset_file(static_cast<projectm_handle>(handle), path, smooth_transition);
    }
}

} // extern "C"
