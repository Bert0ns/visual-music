import 'dart:io';
import 'dart:ffi';
import 'dart:typed_data';
import 'package:ffi/ffi.dart';
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

    if (Platform.isAndroid) {
      // Android uses MediaProjection / AudioPlaybackCapture
      final success = await _methodChannel.invokeMethod<bool>('startCapture');
      if (success == true) {
        _eventChannel.receiveBroadcastStream().listen((dynamic event) {
          if (event is Float32List) {
            // Pass the PCM data from Kotlin straight into projectM via FFI
            // We use a Pointer<Float> to efficiently pass data
            // (Note: in a real production app, allocating FFI memory on every frame is slow, 
            // we would pre-allocate a persistent buffer and use memcpy)
            // But for now, since projectM is initialized and handles audio internally, 
            // passing small arrays via FFI is acceptable.
            final Float32List pcm = event;
            final int frameCount = pcm.length ~/ 2; // Stereo
            
            // Allocate native memory for the frame
            final Pointer<Float> nativeBuffer = malloc.allocate<Float>(pcm.length * sizeOf<Float>());
            final Float32List nativeList = nativeBuffer.asTypedList(pcm.length);
            nativeList.setAll(0, pcm);

            // Send to C++
            projectmAddAudio(pmHandle, nativeBuffer, frameCount);

            // Free the memory immediately after the synchronous C++ call returns
            malloc.free(nativeBuffer);
          }
        });
      }
    } else {
      // Windows, Linux, macOS handle loopback natively in miniaudio
      projectmStartAudioCapture(pmHandle);
    }
  }

  static Future<void> stop() async {
    if (!_isCapturing) return;
    _isCapturing = false;

    if (Platform.isAndroid) {
      await _methodChannel.invokeMethod('stopCapture');
    } else {
      projectmStopAudioCapture();
    }
  }
}
