import 'dart:ffi';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:logger/logger.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:path/path.dart' as p;
import 'dart:io';

enum AudioSource { system, localFile }
enum AudioPlaybackStatus { stopped, playing, paused }

class AudioState {
  final AudioSource activeSource;
  final AudioPlaybackStatus localPlaybackStatus;
  final String? currentTrackName;

  AudioState({
    this.activeSource = AudioSource.system,
    this.localPlaybackStatus = AudioPlaybackStatus.stopped,
    this.currentTrackName,
  });

  AudioState copyWith({
    AudioSource? activeSource,
    AudioPlaybackStatus? localPlaybackStatus,
    String? currentTrackName,
  }) {
    return AudioState(
      activeSource: activeSource ?? this.activeSource,
      localPlaybackStatus: localPlaybackStatus ?? this.localPlaybackStatus,
      currentTrackName: currentTrackName ?? this.currentTrackName,
    );
  }
}

class AudioController extends Notifier<AudioState> {
  final _logger = Logger();
  Pointer<Void>? _pmHandle;

  @override
  AudioState build() {
    return AudioState();
  }

  void init(Pointer<Void> pmHandle) {
    _pmHandle = pmHandle;
    // System capture starts by default
    _startSystemCapture();
  }

  void _startSystemCapture() {
    if (_pmHandle == null) return;
    projectmStartAudioCapture(_pmHandle!);
    state = state.copyWith(activeSource: AudioSource.system);
  }

  void _stopSystemCapture() {
    if (_pmHandle == null) return;
    projectmStopAudioCapture();
  }

  Future<void> playLocalFile(String path) async {
    if (_pmHandle == null) {
      _logger.e("pmHandle is null. Cannot play file.");
      return;
    }

    final file = File(path);
    if (!await file.exists()) {
      _logger.e("Audio file does not exist: $path");
      return;
    }

    // Ensure system capture is stopped before playing local file
    if (state.activeSource == AudioSource.system) {
      _stopSystemCapture();
    }

    final success = projectmPlayFile(_pmHandle!, path);
    if (success) {
      state = state.copyWith(
        activeSource: AudioSource.localFile,
        localPlaybackStatus: AudioPlaybackStatus.playing,
        currentTrackName: p.basenameWithoutExtension(path),
      );
    } else {
      _logger.e("Failed to play audio file: $path");
      // Fallback to system capture if playback failed and we were stopped
      if (state.localPlaybackStatus == AudioPlaybackStatus.stopped) {
         _startSystemCapture();
      }
    }
  }

  void pauseLocalFile() {
    if (state.activeSource == AudioSource.localFile && state.localPlaybackStatus == AudioPlaybackStatus.playing) {
      projectmPauseAudio();
      state = state.copyWith(localPlaybackStatus: AudioPlaybackStatus.paused);
    }
  }

  void resumeLocalFile() {
    if (state.activeSource == AudioSource.localFile && state.localPlaybackStatus == AudioPlaybackStatus.paused) {
      projectmResumeAudio();
      state = state.copyWith(localPlaybackStatus: AudioPlaybackStatus.playing);
    }
  }

  void stopLocalFile() {
    if (state.activeSource == AudioSource.localFile && state.localPlaybackStatus != AudioPlaybackStatus.stopped) {
      projectmStopAudio();
      state = state.copyWith(
        localPlaybackStatus: AudioPlaybackStatus.stopped,
        currentTrackName: null,
      );
      // Automatically fallback to system capture when local file stops
      _startSystemCapture();
    }
  }

  void disposeAll() {
    _stopSystemCapture();
    if (state.activeSource == AudioSource.localFile && state.localPlaybackStatus != AudioPlaybackStatus.stopped) {
      projectmStopAudio();
    }
  }
}

final audioControllerProvider = NotifierProvider<AudioController, AudioState>(() {
  return AudioController();
});
