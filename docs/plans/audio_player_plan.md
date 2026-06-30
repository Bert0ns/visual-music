# Internal Music Player Implementation Plan

## Objective
Build a minimal, high-performance internal music player that decodes local audio files (MP3/WAV/FLAC), plays them through the system speakers, and simultaneously feeds the exact PCM frames directly into the `projectM` visualizer. This entirely bypasses the host OS's loopback limitations (e.g., WSL missing PulseAudio/Jack bridges) and guarantees zero-latency audio-visual synchronization.

## Architecture

We will leverage the existing `miniaudio.h` library in our C++ codebase. Instead of a `capture` device, we will initialize a `playback` device. A `ma_decoder` will read the audio file, and inside the playback callback, we will route the decoded frames to both the hardware output and `projectm_ffi_add_audio`.

---

## Task Breakdown

### Phase 1: C++ Audio Playback Engine
- [ ] Create `projectm_ffi/src/audio_playback.cpp` (or append to `audio_capture.cpp`).
- [ ] Initialize an `ma_decoder` to decode files from a given filepath.
- [ ] Initialize an `ma_device` configured for `ma_device_type_playback`.
- [ ] Implement the `playback_data_callback` to read from the decoder, write to `pOutput` (speakers), and call `projectm_ffi_add_audio(..., pOutput, ...)` for the visualizer.
- [ ] Expose C FFI functions:
  - `bool projectm_ffi_play_file(void* handle, const char* filepath)`
  - `void projectm_ffi_pause_audio()`
  - `void projectm_ffi_resume_audio()`
  - `void projectm_ffi_stop_audio()`
  - *(Optional)* `float projectm_ffi_get_audio_progress()` to drive a progress bar.

### Phase 2: Dart FFI Bindings & State Management
- [ ] Add the new C functions to `projectm_ffi/lib/projectm_ffi.dart` using `dart:ffi`.
- [ ] Create `lib/core/audio/internal_audio_player.dart`.
- [ ] Implement a Singleton or Provider class (`InternalAudioPlayer`) that wraps the FFI calls and manages the player state (Playing, Paused, Stopped).
- [ ] Handle passing the file path across the FFI boundary using `Utf8` strings.

### Phase 3: Flutter UI & File Selection
- [ ] Add the `file_picker` dependency to `pubspec.yaml` to allow users to select local music files.
- [ ] Create a new UI component: `lib/features/visualizer/widgets/music_player_bar.dart`.
- [ ] Design a sleek, floating, glassmorphic bottom bar containing:
  - A button to "Load Track" (opens file picker).
  - Play/Pause toggle buttons.
  - Track name display.
- [ ] Integrate this widget into the main `VisualizerScreen` overlay.

### Phase 4: Unit Testing
- [ ] **FFI Wrapper Tests**: Write unit tests in `test/core/audio/internal_audio_player_test.dart` to mock the FFI layer and ensure the state machine transitions correctly (Stopped -> Playing -> Paused).
- [ ] **Widget Tests**: Write tests for `MusicPlayerBar` to verify that tapping "Play/Pause" triggers the correct state changes and updates the icon.

---

## Constraints & Considerations
- **Memory Management**: Ensure `const char*` filepaths sent from Dart to C++ are properly freed using `calloc`/`free` or Dart's `Arena`.
- **Concurrency**: The `miniaudio` decoder must be thread-safe if we attempt to seek or pause while the audio callback is actively reading. We will use atomics or mutexes in C++ to manage the play state safely.
- **SOLID Principles**: Keep the UI strictly separated from the FFI logic by using the `InternalAudioPlayer` controller as an intermediary.
