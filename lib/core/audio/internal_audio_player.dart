import 'dart:ffi';
import 'package:flutter/foundation.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:path/path.dart' as p;

enum AudioPlayerState { stopped, playing, paused }

class InternalAudioPlayer extends ChangeNotifier {
  static final InternalAudioPlayer instance = InternalAudioPlayer._();
  InternalAudioPlayer._();

  Pointer<Void>? _pmHandle;
  AudioPlayerState _state = AudioPlayerState.stopped;
  String? _currentTrackName;

  AudioPlayerState get state => _state;
  String? get currentTrackName => _currentTrackName;

  void init(Pointer<Void> pmHandle) {
    _pmHandle = pmHandle;
  }

  Future<void> playFile(String path) async {
    if (_pmHandle == null) return;
    
    // Call the FFI function
    final success = projectmPlayFile(_pmHandle!, path);
    if (success) {
      _currentTrackName = p.basenameWithoutExtension(path);
      _state = AudioPlayerState.playing;
      notifyListeners();
    } else {
      debugPrint("Failed to play audio file: $path");
    }
  }

  void pause() {
    if (_state == AudioPlayerState.playing) {
      projectmPauseAudio();
      _state = AudioPlayerState.paused;
      notifyListeners();
    }
  }

  void resume() {
    if (_state == AudioPlayerState.paused) {
      projectmResumeAudio();
      _state = AudioPlayerState.playing;
      notifyListeners();
    }
  }

  void stop() {
    if (_state != AudioPlayerState.stopped) {
      projectmStopAudio();
      _state = AudioPlayerState.stopped;
      _currentTrackName = null;
      notifyListeners();
    }
  }
}
