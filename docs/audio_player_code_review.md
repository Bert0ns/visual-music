## Review: Audio Player Implementation

### Context
I have reviewed the audio player implementation across `internal_audio_player.dart`, `system_audio_capture.dart`, and `music_player_bar.dart`. While the recent refactoring successfully introduced Riverpod, there are several critical logic bugs and architectural inconsistencies remaining in the audio pipeline.

### Correctness
- **Critical: Audio Source Conflict.** When `playFile()` is called in `InternalAudioPlayerNotifier`, it does not stop the `SystemAudioCapture`. Both system audio loopback and the local file decoder will attempt to push PCM audio data into projectM's buffers simultaneously. This will result in corrupted/glitchy visualizations or a potential crash in the C++ layer. You must stop system capture when playing a local file, and resume it when the local file is stopped.
- **Important:** In `MusicPlayerBar`, the `_showManualPathDialog` accepts raw string input and passes it to `playFile` without verifying if the file actually exists on the filesystem. This can cause silent failures.
- **Important:** If `projectmPlayFile` returns `false` (failure), the state is not updated, but the C++ engine might have already stopped the previous track. We need to ensure the state reflects a `stopped` state on failure if nothing is playing.

### Readability
- **Nit:** `AudioPlayerStateData` is a bit verbose. Consider renaming it to `AudioPlayerState` and changing the enum to `AudioPlaybackStatus` to avoid confusion.
- **FYI:** The fallback manual path dialog is a great usability feature for environments where the native file picker fails (like WSL/Linux setups).

### Architecture
- **Important: Static State Anti-pattern.** `SystemAudioCapture` is still using a static class with static `_isCapturing` state. Since we are moving to a Riverpod-based architecture, this should be converted into a `NotifierProvider` (e.g., `SystemAudioCaptureNotifier`).
- **Consider:** Instead of having two separate disparate systems (`InternalAudioPlayerNotifier` and `SystemAudioCapture`), consider creating a unified `AudioController` that manages the active source. This controller would guarantee that only one source is active at a time and orchestrate the transitions cleanly.

### Security
- **Consider:** The manual path input dialog passes a user-provided string directly to the FFI boundary (`projectmPlayFile`). Ensure that the underlying C++ code (likely `miniaudio`) gracefully handles arbitrarily long strings, invalid paths, and permissions errors without triggering a segmentation fault.

### Verdict
- [ ] **Request changes** — The concurrent audio source bug (Critical) and the static state in `SystemAudioCapture` (Important) must be addressed before this is considered robust.

---
**Recommendation for next steps:**
Create a unified `AudioController` (via Riverpod) that manages both System Capture and Local File playback, ensuring mutual exclusion.
