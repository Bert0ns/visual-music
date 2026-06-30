import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

const String _libName = 'projectm_ffi';

final DynamicLibrary dylib = () {
  if (Platform.isMacOS || Platform.isIOS) {
    return DynamicLibrary.open('$_libName.framework/$_libName');
  }
  if (Platform.isAndroid || Platform.isLinux) {
    return DynamicLibrary.open('lib$_libName.so');
  }
  if (Platform.isWindows) {
    return DynamicLibrary.open('$_libName.dll');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

// projectm_ffi_init
typedef _InitC = Pointer<Void> Function();
typedef _InitDart = Pointer<Void> Function();
final _InitDart projectmInit = dylib.lookupFunction<_InitC, _InitDart>(
  'projectm_ffi_init',
);

// projectm_ffi_destroy
typedef _DestroyC = Void Function(Pointer<Void> handle);
typedef _DestroyDart = void Function(Pointer<Void> handle);
final _DestroyDart projectmDestroy = dylib
    .lookupFunction<_DestroyC, _DestroyDart>('projectm_ffi_destroy');

// projectm_ffi_set_window_size
typedef _SetWindowSizeC =
    Void Function(Pointer<Void> handle, Int32 width, Int32 height);
typedef _SetWindowSizeDart =
    void Function(Pointer<Void> handle, int width, int height);
final _SetWindowSizeDart projectmSetWindowSize = dylib
    .lookupFunction<_SetWindowSizeC, _SetWindowSizeDart>(
      'projectm_ffi_set_window_size',
    );

// projectm_ffi_render_frame
typedef _RenderFrameC =
    Void Function(Pointer<Void> handle, Uint32 width, Uint32 height);
typedef _RenderFrameDart =
    void Function(Pointer<Void> handle, int width, int height);
final _RenderFrameDart projectmRenderFrame = dylib
    .lookupFunction<_RenderFrameC, _RenderFrameDart>(
      'projectm_ffi_render_frame',
    );

final Pointer<NativeFunction<_RenderFrameC>> projectmRenderFramePointer = dylib
    .lookup<NativeFunction<_RenderFrameC>>('projectm_ffi_render_frame');

// projectm_ffi_load_preset
typedef _LoadPresetC =
    Void Function(
      Pointer<Void> handle,
      Pointer<Utf8> path,
      Bool smooth_transition,
    );
typedef _LoadPresetDart =
    void Function(
      Pointer<Void> handle,
      Pointer<Utf8> path,
      bool smooth_transition,
    );
final _LoadPresetDart _projectmLoadPresetNative = dylib
    .lookupFunction<_LoadPresetC, _LoadPresetDart>('projectm_ffi_load_preset');

void projectmLoadPreset(
  Pointer<Void> handle,
  String path,
  bool smoothTransition,
) {
  final pathPtr = path.toNativeUtf8();
  _projectmLoadPresetNative(handle, pathPtr, smoothTransition);
  malloc.free(pathPtr);
}

// projectm_ffi_start_audio_capture
typedef _StartAudioCaptureC = Bool Function(Pointer<Void> handle);
typedef _StartAudioCaptureDart = bool Function(Pointer<Void> handle);
final _StartAudioCaptureDart projectmStartAudioCapture = dylib
    .lookupFunction<_StartAudioCaptureC, _StartAudioCaptureDart>(
      'projectm_ffi_start_audio_capture',
    );

// projectm_ffi_stop_audio_capture
typedef _StopAudioCaptureC = Void Function();
typedef _StopAudioCaptureDart = void Function();
final _StopAudioCaptureDart projectmStopAudioCapture = dylib
    .lookupFunction<_StopAudioCaptureC, _StopAudioCaptureDart>(
      'projectm_ffi_stop_audio_capture',
    );

// projectm_ffi_add_audio
typedef _AddAudioC = Void Function(Pointer<Void> handle, Pointer<Float> data, Int32 frameCount);
typedef _AddAudioDart = void Function(Pointer<Void> handle, Pointer<Float> data, int frameCount);
final _AddAudioDart projectmAddAudio = dylib
    .lookupFunction<_AddAudioC, _AddAudioDart>(
      'projectm_ffi_add_audio',
    );

// --- End ---
