# Technical Architecture: ProjectM Cross-Platform Visualizer

## 1. Core Technology Stack

- **Frontend Framework:** Flutter (Dart) for a unified, smooth UI across all platforms.
- **Rendering Engine:** projectM (C++ / OpenGL), bound to Flutter via FFI (Foreign Function Interface) and rendered using Flutter's `Texture` widget.

## 2. C++ Dependency Management (ProjectM)

The **projectM** C++ source code (specifically targeting release `v4.1.6`) is managed via **CMake `FetchContent`** rather than Git Submodules or precompiled binaries.

- **Rationale:** This fully automates the build process. Flutter's native build system will automatically download the correct source code and compile it alongside the app for the target architecture (Linux, Android) without requiring manual cross-compilation.

## 3. Graphics Pipeline Architecture (Double-Buffering)

To satisfy the strict 60FPS performance requirement, the OpenGL rendering pipeline is strictly separated into two isolated Flutter plugins following the Single Responsibility Principle:

1. **`projectm_ffi`**: A pure C++ FFI wrapper that embeds the `projectM` engine. It handles audio data, parses presets, and issues OpenGL draw calls. It knows absolutely nothing about Flutter or GTK.
2. **`projectm_texture`**: A platform-specific MethodChannel plugin (e.g., using GTK/`FlTextureGL` on Linux). It creates an OpenGL context shared with Flutter, manages Frame Buffer Objects (FBOs), and exposes the resulting texture ID to Dart.

**Thread-Safe Rendering (Avoiding Raster Stalls):**
`projectM` draws into a "Back Buffer" FBO on a **Background Thread** (or via a Dart Isolate) using the shared OpenGL context. Once the frame is completely drawn and flushed to the GPU, the `projectm_texture` plugin safely swaps the Back and Front buffers and notifies Flutter's Texture Registry. This guarantees that Flutter's Raster Thread (which calls `FlTextureGL::populate`) never blocks waiting for `projectM` to finish rendering an expensive frame, ensuring perfectly smooth UI overlay animations.

## 4. Audio Capture Architecture

- **Android:** Utilizing the Android 10+ `AudioPlaybackCapture` API.
- **Linux:** Using PulseAudio / PipeWire monitor streams.
- **Implementation:** Handled via platform-specific C++ integrations (`miniaudio`) to avoid FFI overhead when passing real-time PCM audio chunks to projectM.

## 5. Development Environment Setup (WSL)

Since development occurs in a Windows Subsystem for Linux (WSL) environment while utilizing Android Studio on the Windows host, the following setup is required:

- **Code & Editor:** Project files must reside inside the WSL filesystem (e.g., `~/visual-music`). VS Code runs on Windows using the **"WSL" extension**.
- **Flutter SDK:** The Linux version of the Flutter SDK must be installed _inside_ WSL to ensure Linux-compatible compilation tools are available.
- **Android Compilation:** The Android Command Line Tools (Linux version) must be installed inside WSL.
- **ADB Bridge (The Emulator Connection):** The ADB server inside WSL must be configured to connect to the ADB host on Windows over TCP.
- **Desktop Targets:** Running `flutter run -d linux` inside WSL will compile the Linux native desktop app (which can be displayed via WSLg).
