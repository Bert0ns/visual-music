# High-Level Features & Vision: ProjectM Cross-Platform Visualizer

## Overview

A high-performance, cross-platform music visualization application built with Flutter and the **projectM** (MilkDrop) rendering engine. The app targets Linux, Android, and Android TV, delivering mind-bending visual responses to whatever music the user is listening to.

## Target Platforms

- **Linux** (Desktop)
- **Android** (Mobile)
- **Android TV** (Living Room)
- *(Windows, macOS/iOS deferred for future phases)*

## Core Features

### 1. Immersive UI/UX
- **Immersive Mode:** The interface completely hides itself by default to let the visuals take over the entire screen.
- **Interaction:**
  - **Desktop / Mobile:** Moving the mouse or tapping the screen reveals a sleek, semi-transparent, professional overlay containing controls. It auto-hides after a few seconds of inactivity.
  - **Android TV:** Standard D-Pad behavior. Pressing any button wakes the overlay; the D-Pad then navigates the on-screen buttons, and 'OK' triggers them.

### 2. Audio Sources
Capturing audio reliably is the most critical feature. The app will support:
- **System Audio Capture:** Visuals react to whatever audio is playing on the device (e.g., Spotify, YouTube).
- **Microphone Fallback:** Seamless fallback for Android if the foreground media app blocks system capture via DRM/privacy flags.
- **Local Files:** In-app selection of local audio files.

### 3. Preset Engine & Discovery
- **Rich Library:** The app bundles a curated, high-quality subset of **5,000 to 10,000 presets** (sourced from repositories like `presets-cream-of-the-crop` and `presets-milkdrop-original`). This ensures an endless variety of offline visuals straight out of the box.
- **"Auto-DJ" Playback:** The app automatically crossfades into a new random preset every 15-30 seconds.
- **Personalization:** Users can "Heart" or "Ban" presets on the fly from the UI overlay. This trains the app's randomizer to favor their aesthetic preferences over time and skip banned presets.
- **Future Expansion:** A future backend service may allow fetching and streaming new presets on demand.
