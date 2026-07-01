# Codebase Refactoring Plan

This plan outlines the steps required to bring the `visual-music` codebase into compliance with the rules defined in `.agents/AGENTS.md`.

## Overview
The main goal is to eliminate the architectural nightmares currently present in the codebase, such as the "God Widget" in `main.dart`, the heavy reliance on Singletons for state management, and the violation of SOLID principles (especially SRP). We will introduce Riverpod for clean state management and reorganize the file structure.

## Task Breakdown

### 1. State Management & Dependencies Setup
- [x] Add `flutter_riverpod` to `pubspec.yaml`.
- [x] Run `flutter pub get`.
- [x] Wrap the `VisualMusicApp` in `main.dart` with a `ProviderScope`.

### 2. File Restructuring & Extracting the "God Widget"
- [x] Create `lib/features/visualizer/visualizer_screen.dart`.
- [x] Move the UI shell of `VisualizerScreen` from `main.dart` into `visualizer_screen.dart`.
- [x] Clean up `main.dart` so it only acts as the application entry point.

### 3. Implement Riverpod Controllers (Decoupling Logic)
- [x] **Visualizer Controller**: Create `lib/features/visualizer/visualizer_controller.dart`. Move the FFI initialization (`projectmInit`), `Ticker` management, and preset loading logic into a `StateNotifier` or `Notifier`.
- [x] **Audio Controller**: Create `lib/features/settings/audio_controller.dart` to manage the audio capture state and expose it via a Provider.
- [x] **Preset Controller**: Create `lib/features/presets/preset_controller.dart` to expose the current preset state and handle `Auto-DJ` timer logic.

### 4. Refactoring Services (SOLID & SRP)
- [x] **PresetService (`preset_service.dart`)**:
  - Split the SQLite database initialization/querying from the ZIP file extraction logic.
  - Create a new `lib/features/presets/preset_extractor.dart` specifically for handling `ZipDecoder` and Isolate logic.
- [x] **InternalAudioPlayer & SystemAudioCapture**:
  - Remove direct singleton usage (`.instance`). 
  - Wrap these services in Riverpod Providers so they can be injected into the UI.

### 5. Fix UI Components & Coding Standards
- [x] Update `lib/features/visualizer/widgets/music_player_bar.dart` to consume the Riverpod Audio Provider instead of directly invoking `InternalAudioPlayer.instance`.
- [x] Update `lib/features/visualizer/overlay_ui.dart` to read preset state via Riverpod instead of relying on passed-down props from a Stateful parent.
- [x] Replace all `debugPrint` calls in the audio files with `Logger()` instances.

### 6. Testing Strategy
- [x] **State Tests**: Write unit tests for the `VisualizerController` (mocking the FFI layer) to ensure state transitions from `initializing` -> `ready` -> `error` work as expected.
- [x] **PresetService Tests**: Write tests to ensure the database querying returns the expected results without actually running the heavy Zip extraction.
- [x] **Audio Controller Tests**: Test that pausing/resuming updates the controller state correctly.

## Next Steps
Once this plan is confirmed by the human, I will proceed with Step 1 (adding `flutter_riverpod` and starting the state management migration).
