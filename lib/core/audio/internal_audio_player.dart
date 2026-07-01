import 'dart:ffi';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:logger/logger.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:path/path.dart' as p;

enum AudioPlayerState { stopped, playing, paused }

class AudioPlayerStateData {
  final AudioPlayerState state;
  final String? currentTrackName;

  AudioPlayerStateData({this.state = AudioPlayerState.stopped, this.currentTrackName});

  AudioPlayerStateData copyWith({AudioPlayerState? state, String? currentTrackName}) {
    return AudioPlayerStateData(
      state: state ?? this.state,
      currentTrackName: currentTrackName ?? this.currentTrackName,
    );
  }
}

class InternalAudioPlayerNotifier extends Notifier<AudioPlayerStateData> {
  final _logger = Logger();
  Pointer<Void>? _pmHandle;

  @override
  AudioPlayerStateData build() {
    return AudioPlayerStateData();
  }

  void init(Pointer<Void> pmHandle) {
    _pmHandle = pmHandle;
  }

  Future<void> playFile(String path) async {
    if (_pmHandle == null) {
      _logger.e("pmHandle is null. Cannot play file.");
      return;
    }
    
    // Call the FFI function
    final success = projectmPlayFile(_pmHandle!, path);
    if (success) {
      state = state.copyWith(
        currentTrackName: p.basenameWithoutExtension(path),
        state: AudioPlayerState.playing,
      );
    } else {
      _logger.e("Failed to play audio file: $path");
    }
  }

  void pause() {
    if (state.state == AudioPlayerState.playing) {
      projectmPauseAudio();
      state = state.copyWith(state: AudioPlayerState.paused);
    }
  }

  void resume() {
    if (state.state == AudioPlayerState.paused) {
      projectmResumeAudio();
      state = state.copyWith(state: AudioPlayerState.playing);
    }
  }

  void stop() {
    if (state.state != AudioPlayerState.stopped) {
      projectmStopAudio();
      state = AudioPlayerStateData(state: AudioPlayerState.stopped, currentTrackName: null);
    }
  }
}

final internalAudioPlayerProvider = NotifierProvider<InternalAudioPlayerNotifier, AudioPlayerStateData>(() {
  return InternalAudioPlayerNotifier();
});
