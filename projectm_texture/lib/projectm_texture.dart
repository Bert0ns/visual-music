import 'package:flutter/services.dart';
import 'dart:ffi';
import 'dart:io';

typedef _SetCallbackC = Void Function(Int64 textureId, Pointer<NativeFunction<Void Function(Pointer<Void>)>> cb, Pointer<Void> ctx);
typedef _SetCallbackDart = void Function(int textureId, Pointer<NativeFunction<Void Function(Pointer<Void>)>> cb, Pointer<Void> ctx);

class ProjectmTexture {
  static const MethodChannel _channel = MethodChannel('projectm_texture');

  static final DynamicLibrary _dylib = DynamicLibrary.process();

  static final _SetCallbackDart _setCallback = _dylib.lookupFunction<_SetCallbackC, _SetCallbackDart>('projectm_texture_set_callback');

  /// Initializes the OpenGL texture on the platform side and returns the texture ID.
  /// Also sets the C function pointer that GTK will call to render the frame.
  static Future<int> initialize(int width, int height, Pointer<NativeFunction<Void Function(Pointer<Void>)>> renderCallback, Pointer<Void> renderContext) async {
    final int textureId = await _channel.invokeMethod('initialize', {
      'width': width,
      'height': height,
    });
    
    // Register the FFI callback pointer with the C++ GTK plugin
    _setCallback(textureId, renderCallback, renderContext);
    
    return textureId;
  }

  /// Requests the platform side to mark the frame as available, causing the Texture widget to redraw.
  static Future<void> requestFrame(int textureId) async {
    await _channel.invokeMethod('requestFrame', {
      'textureId': textureId,
    });
  }
}
