import 'dart:ffi';
import 'package:flutter/services.dart';
import 'package:projectm_ffi/projectm_ffi.dart';

class SystemAudioCapture {
  static const MethodChannel _methodChannel = MethodChannel('com.example.visual_music/audio_control');
  static const EventChannel _eventChannel = EventChannel('com.example.visual_music/audio_data');
  static bool _isCapturing = false;

  /// Starts system audio capture (loopback).
  /// On Windows/Linux, it delegates directly to miniaudio in C++ (via projectmStartAudioCapture).
  /// On Android, it invokes AudioPlaybackCapture via Kotlin MethodChannel and pipes the PCM data to C++.
  static Future<void> start(Pointer<Void> pmHandle) async {
    if (_isCapturing) return;
    _isCapturing = true;
    // Windows, Linux, macOS, and Android handle capture natively in miniaudio (which falls back to microphone on Android)
    projectmStartAudioCapture(pmHandle);
  }

  static Future<void> stop() async {
    if (!_isCapturing) return;
    _isCapturing = false;

    projectmStopAudioCapture();
  }
}
