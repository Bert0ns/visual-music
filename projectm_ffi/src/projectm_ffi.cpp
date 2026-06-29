#include "projectm_ffi.h"
#include <projectM-4/projectM.h>
#include <stddef.h>
#include <iostream>

extern "C" {

FFI_PLUGIN_EXPORT void* projectm_ffi_init() {
    std::cout << "C++: projectm_ffi_init called" << std::endl;
    projectm_handle pm = projectm_create();
    std::cout << "C++: projectm_create finished, returning " << pm << std::endl;
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

FFI_PLUGIN_EXPORT void projectm_ffi_render_frame(void* handle, uint32_t width, uint32_t height) {
    if (handle) {
        projectm_handle pm = static_cast<projectm_handle>(handle);
        projectm_set_window_size(pm, width, height);
        projectm_opengl_render_frame(pm);
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_load_preset(void* handle, const char* path, bool smooth_transition) {
    if (handle && path) {
        projectm_load_preset_file(static_cast<projectm_handle>(handle), path, smooth_transition);
    }
}

} // extern "C"
