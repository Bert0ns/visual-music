#ifndef GL_D3D_INTEROP_H_
#define GL_D3D_INTEROP_H_

#include <windows.h>
#include <d3d11.h>
#include <gl/GL.h>
#include <stdint.h>
#include <memory>
#include <mutex>

// WGL NV DX interop extension declarations
DECLARE_HANDLE(HANDLE);

class GLD3DInterop {
public:
    GLD3DInterop();
    ~GLD3DInterop();

    bool Initialize(uint32_t width, uint32_t height);
    void Cleanup();

    // Resize the texture
    bool Resize(uint32_t width, uint32_t height);

    // Get the underlying DXGI shared handle for Flutter TextureVariant
    HANDLE GetDXGISharedHandle() const;

    // Lock the texture for OpenGL rendering
    bool LockForGL();

    // Unlock the texture to allow D3D/Flutter to read it
    void UnlockFromGL();

    // Returns the OpenGL Framebuffer Object ID
    GLuint GetFBO() const { return fbo_id_; }

private:
    bool SetupD3D11();
    bool SetupWGL();
    bool LoadWGLInteropExtensions();
    bool CreateSharedTexture(uint32_t width, uint32_t height);

    HWND dummy_hwnd_ = nullptr;
    HDC hdc_ = nullptr;
    HGLRC hglrc_ = nullptr;

    ID3D11Device* d3d_device_ = nullptr;
    ID3D11DeviceContext* d3d_context_ = nullptr;
    HANDLE gl_device_handle_ = nullptr;

    ID3D11Texture2D* shared_texture_ = nullptr;
    HANDLE dxgi_shared_handle_ = nullptr;

    HANDLE gl_texture_handle_ = nullptr;
    GLuint gl_texture_id_ = 0;
    GLuint fbo_id_ = 0;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    
    std::mutex render_mutex_;
};

#endif // GL_D3D_INTEROP_H_
