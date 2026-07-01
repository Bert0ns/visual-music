# Codebase Analysis Report

## 1. Executive Summary
An in-depth review of the `visual-music` codebase was conducted against the rules defined in `.agents/AGENTS.md`. While the project successfully implements native FFI integrations and heavy isolate computations (following performance guidelines), the UI and state architecture suffers from severe anti-patterns. The codebase currently violates several SOLID principles, abuses `StatefulWidget`s for global state management, and relies on scattered singletons.

## 2. Architectural Nightmares & Mistakes

### 2.1. The "God Widget" (`lib/main.dart`)
**Issue:** `main.dart` contains a massive `StatefulWidget` (`VisualizerScreen`) that handles everything from FFI initialization, `Ticker` loop management, timer logic (`auto-dj`), preset loading, UI visibility toggling, and layout rendering. 
**Violation:** This heavily violates the **Single Responsibility Principle (SRP)**. The entry point file has become a dumping ground for the visualizer controller logic.
**Impact:** It is nearly impossible to test the render loop or preset loading in isolation.

### 2.2. State Management Anti-Patterns
**Issue:** The app entirely bypasses the mandated state management solutions (Riverpod / Provider). Instead, state is passed around through tight coupling, and `setState()` is called at the top of the tree in `_VisualizerScreenState`, causing massive rebuilds.
**Violation:** Violates the **State Management** rule: _"Avoid passing state deeply through the widget tree. Use a defined state management solution."_ and the **Stateless vs Stateful** rule: _"Favor StatelessWidget over StatefulWidget. Keep logic out of the UI tree."_

### 2.3. Singleton Overuse & Tight Coupling
**Issue:** `PresetService.instance` and `InternalAudioPlayer.instance` are heavily used directly inside UI widgets (e.g., `MusicPlayerBar`). 
**Violation:** Violates the **Dependency Inversion Principle (DIP)**. The UI components directly depend on concrete singleton instances instead of abstractions or injected dependencies, hindering testability.

### 2.4. Overloaded `PresetService`
**Issue:** `PresetService` handles SQLite DB initialization, random querying, AND filesystem zip extraction (`ZipDecoder`).
**Violation:** Violates **SRP**. The extraction and filesystem logic should be segregated into a dedicated repository or filesystem service.

## 3. Bugs & Coding Standard Violations

### 3.1. Improper Logging
**Issue:** In `lib/features/visualizer/widgets/music_player_bar.dart` and `lib/core/audio/internal_audio_player.dart`, errors are logged using `debugPrint("...");`.
**Violation:** Violates the **Error Handling & Logging** rule: _"Do not use print(). Use a dedicated logging package (like logger) to record errors..."_

## 4. Refactoring Opportunities

To bring the project in line with `AGENTS.md`, the following refactoring steps are highly recommended:

1. **Implement Riverpod/Provider:**
   - Introduce a state management library.
   - Create a `VisualizerController` (or Notifier) to handle the `Ticker`, FFI handle, and timers, extracting all logic from `VisualizerScreen`.
   
2. **Restructure `main.dart`:**
   - Move `VisualizerScreen` into `lib/features/visualizer/visualizer_screen.dart`.
   - Leave `main.dart` purely as the application entry point (`runApp`).

3. **Decouple UI from Singletons:**
   - Instead of `MusicPlayerBar` calling `InternalAudioPlayer.instance` directly, it should read the player state from the state management provider.
   
4. **Fix Logging:**
   - Replace all instances of `debugPrint` in `internal_audio_player.dart` and `music_player_bar.dart` with the centralized `Logger()`.

## 5. Good Practices Found

- **Isolates for Heavy Tasks:** `preset_service.dart` correctly uses `compute` to offload the heavy `zip` extraction and file writing to background isolates, perfectly respecting the **60+ FPS Requirement** and threading rules.
- **FFI Memory Management:** The `projectmDestroy(_pmHandle!);` is correctly called in the dispose phase, adhering to the strict memory management rules across the Dart/C++ boundary.
- **File Structure Base:** The separation of `core`, `features/presets`, and `features/visualizer` exists, but needs to be strictly enforced.
