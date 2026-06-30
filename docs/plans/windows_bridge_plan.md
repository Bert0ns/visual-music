# Windows Texture Bridge Implementation Plan

## Objective

Enable hardware-accelerated 60fps rendering of the projectM OpenGL engine on Windows by building a custom texture bridge plugin (`projectm_texture`).

## Architectural Challenge

- **Flutter Windows** uses DirectX (or ANGLE which wraps DirectX) for hardware-accelerated texture sharing via `flutter::GpuSurfaceTexture` (specifically using `kFlutterGpuSurfaceTypeDxgiSharedHandle`).
- **projectM** is purely an **OpenGL** engine.
- We cannot use a simple `PixelBufferTexture` (using `glReadPixels` to copy pixels to the CPU) because the RAM copy overhead at 800x600 or 1080p will absolutely destroy the 60fps requirement and cause massive CPU spikes.

## Solution: Zero-Copy OpenGL-to-DirectX Interop

We must use **WGL_NV_DX_interop** (a widely supported extension on Windows for both NVIDIA/AMD/Intel) to share a texture directly on the GPU between DirectX 11 and OpenGL.

### The Architecture

1. **DirectX 11 Device:** Initialize a headless D3D11 device in the Flutter Windows plugin.
2. **DXGI Shared Texture:** Create a D3D11 Texture2D with the `D3D11_RESOURCE_MISC_SHARED` flag. This gives us a HANDLE that Flutter can read.
3. **OpenGL Context:** Create a WGL (Windows GL) context that runs on the same GPU.
4. **WGL/DX Interop:**
   - Use `wglDXOpenDeviceNV` to link the GL context to the D3D device.
   - Use `wglDXRegisterObjectNV` to map the D3D11 Texture into an OpenGL renderbuffer/texture.
5. **Rendering Loop:**
   - Lock the object (`wglDXLockObjectsNV`).
   - `projectM` renders its frame directly to this mapped OpenGL texture/FBO.
   - Unlock the object (`wglDXUnlockObjectsNV`).
6. **Flutter Registration:** Pass the underlying DXGI `HANDLE` to Flutter's `TextureRegistrar` using `flutter::GpuSurfaceTexture`. Flutter's compositor reads the texture at zero-copy 60fps.

## Step-by-Step Implementation

- [ ] **Step 1: Setup Windows Plugin Structure**
  - Create the `windows` folder inside `projectm_texture/`.
  - Add `CMakeLists.txt` configured to link `flutter_wrapper_plugin`, `d3d11`, and `opengl32`.
  - Add standard Flutter Windows plugin scaffolding (`projectm_texture_plugin.cpp` and `.h`).
  - Update `projectm_texture/pubspec.yaml` to declare the Windows platform plugin.

- [ ] **Step 2: Initialize D3D11 and WGL Context**
  - Implement a `WindowsTexture` C++ class.
  - Create the D3D11 device and device context.
  - Create a dummy HWND to create a WGL context.
  - Load the `wglDX*` extension function pointers (using an extension loader like GLEW or manually via `wglGetProcAddress`).

- [ ] **Step 3: Texture Lifecycle & Interop Mapping**
  - On `initialize()` (called via MethodChannel from Dart):
    - Create a D3D11 shared texture matching the requested width/height.
    - Map it to an OpenGL FBO using the NV interop extension.
    - Return the Flutter `textureId`.
  - Handle cleanup/resizing by unregistering the interop object and recreating the texture.

- [ ] **Step 4: The Render Callback**
  - Implement the `requestFrame` MethodChannel call.
  - Acquire the DX/GL lock.
  - Execute the FFI callback to trigger `projectm_ffi_render_frame(pmHandle, FBO_ID)`.
  - Release the lock.
  - Notify Flutter via `TextureRegistrar::MarkTextureFrameAvailable`.

## Fallback Consideration (Optional Phase 2)

If a user's GPU drivers somehow lack `WGL_NV_DX_interop`, the fallback is ANGLE (EGL surface to DXGI mapping) or a raw CPU memory `PixelBufferTexture`. For this MVP, WGL_NV_DX_interop is supported on 99% of modern Windows hardware and should be the primary path.
