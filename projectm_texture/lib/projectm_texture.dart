import 'package:flutter/services.dart';
import 'dart:ffi';
import 'dart:io';

typedef _SetCallbackC =
    Void Function(
      Int64 textureId,
      Pointer<NativeFunction<Void Function(Pointer<Void>, Uint32, Uint32)>> cb,
      Pointer<Void> ctx,
    );
typedef _SetCallbackDart =
    void Function(
      int textureId,
      Pointer<NativeFunction<Void Function(Pointer<Void>, Uint32, Uint32)>> cb,
      Pointer<Void> ctx,
    );

class ProjectmTexture {
  static const MethodChannel _channel = MethodChannel('projectm_texture');

  static final DynamicLibrary _dylib = DynamicLibrary.process();

  static final _SetCallbackDart? _setCallback = Platform.isLinux ? _dylib
      .lookupFunction<_SetCallbackC, _SetCallbackDart>(
        'projectm_texture_set_callback',
      ) : null;

  // Render FFI pointer for Windows to call directly
  static Pointer<NativeFunction<Void Function(Pointer<Void>, Uint32, Uint32)>>? _renderCallback;
  static Pointer<Void>? _renderContext;
  static int _fboId = 0;

  static Future<int> initialize(
    int width,
    int height,
    Pointer<NativeFunction<Void Function(Pointer<Void>, Uint32, Uint32)>>
    renderCallback,
    Pointer<Void> renderContext,
  ) async {
    _renderCallback = renderCallback;
    _renderContext = renderContext;

    final result = await _channel.invokeMethod('initialize', {
      'width': width,
      'height': height,
    });
    
    int textureId = 0;

    if (result is int) {
      textureId = result;
    } else if (result is Map) {
      textureId = result['textureId'] as int;
      _fboId = result['fboId'] as int;
    }

    if (Platform.isLinux && _setCallback != null) {
      // Register the FFI callback pointer with the C++ GTK plugin
      _setCallback!(textureId, renderCallback, renderContext);
    }

    return textureId;
  }

  /// Requests the platform side to mark the frame as available, causing the Texture widget to redraw.
  static Future<void> requestFrame(int textureId) async {
    if (Platform.isAndroid && _renderCallback != null && _renderContext != null) {
      // On Android, we just call the FFI render function directly.
      // C++ handles EGL context making, rendering, and eglSwapBuffers.
      // SurfaceTexture automatically notifies Flutter of the new frame.
      final renderFunc = _renderCallback!.asFunction<void Function(Pointer<Void>, int, int)>();
      renderFunc(_renderContext!, 0, 0);
      return; // No need to invoke MethodChannel
    }
    
    if (Platform.isWindows && _renderCallback != null && _renderContext != null) {
      // On Windows, we must manually lock the WGL-D3D interop texture, render it, and unlock it.
      await _channel.invokeMethod('lock');
      
      // Call the FFI render function synchronously
      final renderFunc = _renderCallback!.asFunction<void Function(Pointer<Void>, int, int)>();
      renderFunc(_renderContext!, _fboId, 0); // For now, passing 0 as width/height since FBO is fixed internally
      
      await _channel.invokeMethod('unlock');
    }
    await _channel.invokeMethod('requestFrame', {'textureId': textureId});
  }
}

