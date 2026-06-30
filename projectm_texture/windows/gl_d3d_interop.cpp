#include "gl_d3d_interop.h"
#include <iostream>

// WGL NV DX interop function pointers
typedef HANDLE (WINAPI * PFNWGLDXOPENDEVICENVPROC) (void *dxDevice);
typedef BOOL (WINAPI * PFNWGLDXCLOSEDEVICENVPROC) (HANDLE hDevice);
typedef HANDLE (WINAPI * PFNWGLDXREGISTEROBJECTNVPROC) (HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
typedef BOOL (WINAPI * PFNWGLDXUNREGISTEROBJECTNVPROC) (HANDLE hDevice, HANDLE hObject);
typedef BOOL (WINAPI * PFNWGLDXOBJECTACCESSNVPROC) (HANDLE hObject, GLenum access);
typedef BOOL (WINAPI * PFNWGLDXLOCKOBJECTSNVPROC) (HANDLE hDevice, GLint count, HANDLE *hObjects);
typedef BOOL (WINAPI * PFNWGLDXUNLOCKOBJECTSNVPROC) (HANDLE hDevice, GLint count, HANDLE *hObjects);

static PFNWGLDXOPENDEVICENVPROC wglDXOpenDeviceNV = nullptr;
static PFNWGLDXCLOSEDEVICENVPROC wglDXCloseDeviceNV = nullptr;
static PFNWGLDXREGISTEROBJECTNVPROC wglDXRegisterObjectNV = nullptr;
static PFNWGLDXUNREGISTEROBJECTNVPROC wglDXUnregisterObjectNV = nullptr;
static PFNWGLDXLOCKOBJECTSNVPROC wglDXLockObjectsNV = nullptr;
static PFNWGLDXUNLOCKOBJECTSNVPROC wglDXUnlockObjectsNV = nullptr;

#define WGL_ACCESS_READ_WRITE_NV 0x0001
#define GL_TEXTURE_2D 0x0DE1

GLD3DInterop::GLD3DInterop() {}

GLD3DInterop::~GLD3DInterop() {
    Cleanup();
}

void GLD3DInterop::Cleanup() {
    std::lock_guard<std::mutex> lock(render_mutex_);
    
    if (gl_texture_handle_ && wglDXUnregisterObjectNV) {
        wglDXUnregisterObjectNV(gl_device_handle_, gl_texture_handle_);
        gl_texture_handle_ = nullptr;
    }
    if (fbo_id_ != 0) {
        // We'd call glDeleteFramebuffers but need extension loaded
        fbo_id_ = 0;
    }
    if (gl_texture_id_ != 0) {
        glDeleteTextures(1, &gl_texture_id_);
        gl_texture_id_ = 0;
    }
    if (gl_device_handle_ && wglDXCloseDeviceNV) {
        wglDXCloseDeviceNV(gl_device_handle_);
        gl_device_handle_ = nullptr;
    }
    if (shared_texture_) {
        shared_texture_->Release();
        shared_texture_ = nullptr;
    }
    if (d3d_context_) {
        d3d_context_->Release();
        d3d_context_ = nullptr;
    }
    if (d3d_device_) {
        d3d_device_->Release();
        d3d_device_ = nullptr;
    }
    if (hglrc_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(hglrc_);
        hglrc_ = nullptr;
    }
    if (hdc_ && dummy_hwnd_) {
        ReleaseDC(dummy_hwnd_, hdc_);
        hdc_ = nullptr;
    }
    if (dummy_hwnd_) {
        DestroyWindow(dummy_hwnd_);
        dummy_hwnd_ = nullptr;
    }
}

bool GLD3DInterop::Initialize(uint32_t width, uint32_t height) {
    Cleanup();
    width_ = width;
    height_ = height;

    if (!SetupWGL()) {
        std::cerr << "GLD3DInterop: SetupWGL failed" << std::endl;
        return false;
    }
    if (!SetupD3D11()) {
        std::cerr << "GLD3DInterop: SetupD3D11 failed" << std::endl;
        return false;
    }
    if (!LoadWGLInteropExtensions()) {
        std::cerr << "GLD3DInterop: LoadWGLInteropExtensions failed" << std::endl;
        return false;
    }
    
    gl_device_handle_ = wglDXOpenDeviceNV(d3d_device_);
    if (!gl_device_handle_) {
        std::cerr << "GLD3DInterop: wglDXOpenDeviceNV failed" << std::endl;
        return false;
    }

    return CreateSharedTexture(width, height);
}

bool GLD3DInterop::SetupWGL() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "ProjectM_DummyGLWindow";
    RegisterClassA(&wc);

    dummy_hwnd_ = CreateWindowA(wc.lpszClassName, "Dummy", 0, 0, 0, 1, 1, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!dummy_hwnd_) return false;

    hdc_ = GetDC(dummy_hwnd_);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;

    int pixelFormat = ChoosePixelFormat(hdc_, &pfd);
    SetPixelFormat(hdc_, pixelFormat, &pfd);

    hglrc_ = wglCreateContext(hdc_);
    if (!hglrc_) return false;
    wglMakeCurrent(hdc_, hglrc_);

    return true;
}

bool GLD3DInterop::SetupD3D11() {
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   featureLevels, 1, D3D11_SDK_VERSION, &d3d_device_, nullptr, &d3d_context_);
    return SUCCEEDED(hr);
}

bool GLD3DInterop::LoadWGLInteropExtensions() {
    wglDXOpenDeviceNV = (PFNWGLDXOPENDEVICENVPROC)wglGetProcAddress("wglDXOpenDeviceNV");
    wglDXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENVPROC)wglGetProcAddress("wglDXCloseDeviceNV");
    wglDXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNVPROC)wglGetProcAddress("wglDXRegisterObjectNV");
    wglDXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNVPROC)wglGetProcAddress("wglDXUnregisterObjectNV");
    wglDXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNVPROC)wglGetProcAddress("wglDXLockObjectsNV");
    wglDXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNVPROC)wglGetProcAddress("wglDXUnlockObjectsNV");

    return wglDXOpenDeviceNV && wglDXCloseDeviceNV && wglDXRegisterObjectNV && 
           wglDXUnregisterObjectNV && wglDXLockObjectsNV && wglDXUnlockObjectsNV;
}

bool GLD3DInterop::CreateSharedTexture(uint32_t width, uint32_t height) {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    HRESULT hr = d3d_device_->CreateTexture2D(&desc, nullptr, &shared_texture_);
    if (FAILED(hr)) return false;

    IDXGIResource* dxgiResource = nullptr;
    hr = shared_texture_->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);
    if (FAILED(hr)) return false;

    dxgiResource->GetSharedHandle(&dxgi_shared_handle_);
    dxgiResource->Release();

    // Create GL texture
    glGenTextures(1, &gl_texture_id_);
    
    // Register interop
    gl_texture_handle_ = wglDXRegisterObjectNV(gl_device_handle_, shared_texture_, gl_texture_id_, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
    if (!gl_texture_handle_) {
        std::cerr << "GLD3DInterop: wglDXRegisterObjectNV failed" << std::endl;
        return false;
    }
    
    // Note: FBO creation usually requires glew/gl3w for glGenFramebuffers. 
    // For simplicity, we are returning the texture_id, and the Dart caller will use projectm to render to it,
    // OR we load glGenFramebuffers dynamically here.
    
    // Let's manually load glGenFramebuffers and glFramebufferTexture2D
    typedef void (APIENTRY * PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
    typedef void (APIENTRY * PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
    typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
    
    if (glGenFramebuffers && glBindFramebuffer && glFramebufferTexture2D) {
        glGenFramebuffers(1, &fbo_id_);
        glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, fbo_id_);
        glFramebufferTexture2D(0x8D40 /* GL_FRAMEBUFFER */, 0x8CE0 /* GL_COLOR_ATTACHMENT0 */, GL_TEXTURE_2D, gl_texture_id_, 0);
        glBindFramebuffer(0x8D40 /* GL_FRAMEBUFFER */, 0);
    } else {
        std::cerr << "GLD3DInterop: Failed to load FBO functions" << std::endl;
        return false;
    }

    return true;
}

HANDLE GLD3DInterop::GetDXGISharedHandle() const {
    return dxgi_shared_handle_;
}

bool GLD3DInterop::LockForGL() {
    render_mutex_.lock();
    if (wglDXLockObjectsNV && gl_device_handle_ && gl_texture_handle_) {
        wglMakeCurrent(hdc_, hglrc_);
        return wglDXLockObjectsNV(gl_device_handle_, 1, &gl_texture_handle_);
    }
    render_mutex_.unlock();
    return false;
}

void GLD3DInterop::UnlockFromGL() {
    if (wglDXUnlockObjectsNV && gl_device_handle_ && gl_texture_handle_) {
        glFlush();
        wglDXUnlockObjectsNV(gl_device_handle_, 1, &gl_texture_handle_);
        wglMakeCurrent(nullptr, nullptr);
    }
    render_mutex_.unlock();
}
