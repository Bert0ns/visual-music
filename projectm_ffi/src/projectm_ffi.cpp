#include "projectm_ffi.h"
#include <projectM-4/projectM.h>
#include <stddef.h>
#include <stdint.h>
#include <fstream>
#include <iostream>

ProjectmFfiState* projectm_ffi_state_from_handle(void* handle) {
    return static_cast<ProjectmFfiState*>(handle);
}

projectm_handle projectm_ffi_native_handle(void* handle) {
    ProjectmFfiState* state = projectm_ffi_state_from_handle(handle);
    if (!state) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    return state->instance;
}

static bool file_exists(const char* path) {
    std::ifstream file(path);
    return file.good();
}

extern "C" {

FFI_PLUGIN_EXPORT void* projectm_ffi_init() {
    std::cout << "C++: projectm_ffi_init creating wrapper state" << std::endl;
    return static_cast<void*>(new ProjectmFfiState());
}

FFI_PLUGIN_EXPORT void projectm_ffi_destroy(void* handle) {
    ProjectmFfiState* state = projectm_ffi_state_from_handle(handle);
    if (!state) {
        return;
    }

    projectm_handle instance = nullptr;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        instance = state->instance;
        state->instance = nullptr;
    }

    if (instance) {
        projectm_destroy(instance);
    }

    delete state;
}

FFI_PLUGIN_EXPORT void projectm_ffi_set_window_size(void* handle, int width, int height) {
    ProjectmFfiState* state = projectm_ffi_state_from_handle(handle);
    if (!state || width <= 0 || height <= 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->width = static_cast<uint32_t>(width);
    state->height = static_cast<uint32_t>(height);
    if (state->instance) {
        projectm_set_window_size(state->instance, state->width, state->height);
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_render_frame(void* handle, uint32_t width, uint32_t height) {
    ProjectmFfiState* state = projectm_ffi_state_from_handle(handle);
    if (!state || width == 0 || height == 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);

    if (!state->instance) {
        std::cout << "C++: Creating projectM on render thread" << std::endl;
        state->instance = projectm_create();
        if (!state->instance) {
            std::cerr << "C++: projectm_create failed. OpenGL context is not ready or not supported." << std::endl;
            return;
        }
        std::cout << "C++: projectm_create finished, returning " << state->instance << std::endl;
    }

    state->width = width;
    state->height = height;

    try {
        projectm_set_window_size(state->instance, state->width, state->height);

        if (state->has_pending_preset) {
            std::cout << "C++: Loading pending preset: " << state->pending_preset_path << std::endl;
            projectm_load_preset_file(
                state->instance,
                state->pending_preset_path.c_str(),
                state->pending_smooth_transition
            );
            state->has_pending_preset = false;
            std::cout << "C++: Pending preset loaded!" << std::endl;
        }

        projectm_opengl_render_frame(state->instance);
    } catch (const std::exception& error) {
        state->has_pending_preset = false;
        std::cerr << "C++: projectM render/load failed: " << error.what() << std::endl;
    } catch (...) {
        state->has_pending_preset = false;
        std::cerr << "C++: projectM render/load failed with an unknown error" << std::endl;
    }
}

FFI_PLUGIN_EXPORT void projectm_ffi_load_preset(void* handle, const char* path, bool smooth_transition) {
    ProjectmFfiState* state = projectm_ffi_state_from_handle(handle);
    if (state && path) {
        if (!file_exists(path)) {
            std::cerr << "C++: Preset file does not exist: " << path << std::endl;
            return;
        }

        std::cout << "C++: Queuing preset: " << path << std::endl;
        std::lock_guard<std::mutex> lock(state->mutex);
        state->pending_preset_path = path;
        state->pending_smooth_transition = smooth_transition;
        state->has_pending_preset = true;
        std::cout << "C++: Preset queued successfully!" << std::endl;
    }
}

} // extern "C"
