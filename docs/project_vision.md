# Project Vision: ProjectM Cross-Platform Visualizer

## Overview

A high-performance, cross-platform music visualization application built with Flutter and the **projectM** (MilkDrop) C++ rendering engine. The app targets Windows, Linux, Android, and Android TV, delivering mind-bending visual responses to whatever music the user is listening to.

## 1. Core Technology Stack

- **Frontend Framework:** Flutter (Dart) for a unified, smooth UI across all platforms.
- **Rendering Engine:** projectM (C++ / OpenGL), bound to Flutter via FFI (Foreign Function Interface) and rendered using Flutter's `Texture` widget.
- **Target Platforms:** Windows, Linux, Android, Android TV. (macOS/iOS deferred for future phases).

## 2. Audio Capture Strategy

Capturing audio reliably is the most critical feature. The app will support:

- **System Audio Capture:**
  - **Android:** Utilizing the Android 10+ `AudioPlaybackCapture` API.
  - **Windows:** Using WASAPI loopback.
  - **Linux:** Using PulseAudio / PipeWire monitor streams.
- **Fallback / Alternative Sources:**
  - **Microphone:** Seamless fallback for Android if the foreground media app (like Spotify) blocks capture via DRM/privacy flags.
  - **Local Files:** In-app selection of local audio files.

## 3. UI/UX Design (Immersive & Minimal)

- **Immersive Mode:** The interface completely hides itself by default to let the visuals take over the entire screen.
- **Interaction:**
  - **Desktop / Mobile:** Moving the mouse or tapping the screen reveals a sleek, semi-transparent, professional overlay containing controls. It auto-hides after a few seconds of inactivity.
  - **Android TV:** Standard D-Pad behavior. Pressing any button wakes the overlay; the D-Pad then navigates the on-screen buttons, and 'OK' triggers them.

## 4. Preset Engine & Discovery

- **Delivery:** The app will bundle a curated, high-quality subset of **5,000 to 10,000 presets** (sourced from repositories like `presets-cream-of-the-crop` and `presets-milkdrop-original`). The trade-off of a larger app size (~100MB+) is accepted to ensure an endless variety of offline visuals straight out of the box.
- **"Auto-DJ" Playback:** The app automatically crossfades into a new random preset every 15-30 seconds.
- **Personalization:** Users can "Heart" or "Ban" presets on the fly from the UI overlay. This trains the app's randomizer to favor their aesthetic preferences over time and skip banned presets.
- **Future Expansion (Backend Service):** Since a mega database of 100k+ presets exists, a future phase of the project may include an external backend database service. This would allow the app to fetch and stream new presets on demand without bloating the local installation.

## 5. Next Steps for Development

When you are ready to begin writing code, the immediate technical milestones will be:

1. Setting up the base Flutter project with native C++ FFI capabilities.
2. Compiling the `projectM` library for Windows, Linux, and Android.
3. Establishing the OpenGL texture bridge to stream projectM's rendered frames directly into a Flutter widget.
4. Implementing the platform-specific audio capture channels (Android AudioPlaybackCapture, WASAPI, etc.) and feeding the raw PCM audio data into projectM.

## 6. Development Environment Setup (WSL)

Since development will occur in a Windows Subsystem for Linux (WSL) environment while utilizing Android Studio on the Windows host, the following setup is required:

- **Code & Editor:** Project files must reside inside the WSL filesystem (e.g., `~/visual-music`). VS Code runs on Windows using the **"WSL" extension** to edit these files seamlessly.
- **Flutter SDK:** The Linux version of the Flutter SDK must be installed *inside* WSL to ensure Linux-compatible compilation tools are available.
- **Android Compilation:** The Android Command Line Tools (Linux version) must be installed inside WSL. Full Android Studio is not needed in WSL; only the CLI tools are required for `flutter build apk`.
- **ADB Bridge (The Emulator Connection):** Android Emulators will run on the Windows host via Android Studio. To allow WSL's Flutter installation to see the Windows emulator, the ADB server inside WSL must be configured to connect to the ADB host on Windows over TCP (e.g., setting the WSL ADB host to connect to the Windows IP/host).
- **Desktop Targets:** Running `flutter run -d linux` inside WSL will compile the Linux native desktop app (which can be displayed via WSLg). Compiling the Windows `.exe` native app will require checking out the repository on the Windows host and compiling it natively outside of WSL.
