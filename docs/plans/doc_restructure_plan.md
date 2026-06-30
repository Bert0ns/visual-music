# Documentation Restructuring Plan

## Overview
Currently, `project_vision.md` contains a mix of high-level product vision and deep technical implementation details (like FFI bindings and OpenGL double-buffering). To improve maintainability and readability, we will separate the documentation into distinct high-level feature documentation and low-level technical architecture documentation.

## Proposed Structure

### 1. High-Level Features & Vision
**File:** `docs/features_and_vision.md` (will replace `project_vision.md`)
**Content:**
- **Product Overview:** What the app is and who it is for.
- **Target Platforms:** Linux, Android, Android TV.
- **Core Features:**
  - Immersive, auto-hiding UI.
  - Audio sources (System audio, Microphone fallback).
  - Preset Engine (Bundled presets, Auto-DJ crossfading).
  - Personalization (Heart/Ban system).
- *This file will contain NO implementation details (no mentions of FFI, CMake, Isolates, or OpenGL).*

### 2. Technical Architecture & Implementation
**File:** `docs/technical_architecture.md`
**Content:**
- **Core Technology Stack:** Flutter, Dart, projectM (C++).
- **C++ Dependency Management:** CMake `FetchContent` strategy.
- **Graphics Pipeline:** Double-buffering, `projectm_ffi`, `projectm_texture`, and background thread rendering.
- **Audio Capture Architecture:** `miniaudio` integration, PulseAudio/PipeWire, Android `AudioPlaybackCapture`.
- **Data & State:** SQLite database for presets, Isolate parsing, Riverpod state management.
- **Development Environment Setup:** WSL configuration and ADB bridging.

### Tasks
- [x] Create `docs/features_and_vision.md` by extracting high-level sections from `project_vision.md`.
- [x] Create `docs/technical_architecture.md` by extracting technical sections from `project_vision.md`.
- [x] Delete the old `docs/project_vision.md` to prevent duplication.
- [x] Update `README.md` to link to these two new foundational documents.

