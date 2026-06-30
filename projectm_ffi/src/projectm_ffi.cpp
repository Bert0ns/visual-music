#include "projectm_ffi.h"
#include <projectM-4/projectM.h>
#include <stddef.h>
#include <stdint.h>
#include <fstream>
#include <iostream>

ProjectmFfiState* projectm_ffi_state_from_handle(void* handle) {
    return static_cast<ProjectmFfiState*>(handle);
}

#if defined(__ANDROID__)
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window_jni.h>
#include <jni.h>

static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLContext g_eglContext = EGL_NO_CONTEXT;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static ANativeWindow* g_nativeWindow = nullptr;
static std::mutex g_eglMutex;

extern "C" JNIEXPORT void JNICALL Java_com_example_projectm_1texture_ProjectmTexturePlugin_nativeSetSurface(JNIEnv* env, jobject thiz, jobject surface, jint width, jint height) {
    std::lock_guard<std::mutex> lock(g_eglMutex);
    
    if (g_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(g_eglDisplay, g_eglSurface);
        g_eglSurface = EGL_NO_SURFACE;
    }
    if (g_nativeWindow) {
        ANativeWindow_release(g_nativeWindow);
        g_nativeWindow = nullptr;
    }
    
    if (surface) {
        g_nativeWindow = ANativeWindow_fromSurface(env, surface);
        if (g_nativeWindow && g_eglDisplay != EGL_NO_DISPLAY) {
            // If display is already initialized, recreate the surface
            EGLConfig config;
            EGLint numConfigs;
            const EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                EGL_NONE
            };
            eglChooseConfig(g_eglDisplay, attribs, &config, 1, &numConfigs);
            g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, g_nativeWindow, nullptr);
        }
    }
}
#endif

static bool file_exists(const char* path) {
    std::ifstream file(path);
    return file.good();
}

extern "C" {

FFI_PLUGIN_EXPORT void projectm_ffi_add_audio(void* handle, const float* data, int frameCount) {
    ProjectmFfiState* state = projectm_ffi_state_from_handle(handle);
    if (!state || !data || frameCount <= 0) {
        return;
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    if (state->instance) {
        projectm_pcm_add_float(state->instance, data, frameCount, PROJECTM_STEREO);
    }
}

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

#if defined(__ANDROID__)
    g_eglMutex.lock();
    if (!g_nativeWindow) {
        g_eglMutex.unlock();
        return;
    }

    if (g_eglDisplay == EGL_NO_DISPLAY) {
        g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(g_eglDisplay, nullptr, nullptr);

        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_ALPHA_SIZE, 8,
            EGL_NONE
        };
        EGLConfig config;
        EGLint numConfigs;
        eglChooseConfig(g_eglDisplay, attribs, &config, 1, &numConfigs);

        const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
        g_eglContext = eglCreateContext(g_eglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
        g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, g_nativeWindow, nullptr);
    }
    
    if (g_eglSurface != EGL_NO_SURFACE) {
        eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext);
    }
    g_eglMutex.unlock();
#endif

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
        
#if defined(__ANDROID__)
        g_eglMutex.lock();
        if (g_eglDisplay != EGL_NO_DISPLAY && g_eglSurface != EGL_NO_SURFACE) {
            eglSwapBuffers(g_eglDisplay, g_eglSurface);
        }
        g_eglMutex.unlock();
#endif
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
