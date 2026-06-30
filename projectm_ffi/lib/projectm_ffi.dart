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
typedef InitC = Pointer<Void> Function();
typedef InitDart = Pointer<Void> Function();
final InitDart projectmInit = dylib.lookupFunction<InitC, InitDart>(
  'projectm_ffi_init',
);

// projectm_ffi_destroy
typedef DestroyC = Void Function(Pointer<Void> handle);
typedef DestroyDart = void Function(Pointer<Void> handle);
final DestroyDart projectmDestroy = dylib
    .lookupFunction<DestroyC, DestroyDart>('projectm_ffi_destroy');

// projectm_ffi_set_window_size
typedef SetWindowSizeC =
    Void Function(Pointer<Void> handle, Int32 width, Int32 height);
typedef SetWindowSizeDart =
    void Function(Pointer<Void> handle, int width, int height);
final SetWindowSizeDart projectmSetWindowSize = dylib
    .lookupFunction<SetWindowSizeC, SetWindowSizeDart>(
      'projectm_ffi_set_window_size',
    );

// projectm_ffi_render_frame
typedef RenderFrameC =
    Void Function(Pointer<Void> handle, Uint32 width, Uint32 height);
typedef RenderFrameDart =
    void Function(Pointer<Void> handle, int width, int height);
final RenderFrameDart projectmRenderFrame = dylib
    .lookupFunction<RenderFrameC, RenderFrameDart>(
      'projectm_ffi_render_frame',
    );

final Pointer<NativeFunction<RenderFrameC>> projectmRenderFramePointer = dylib
    .lookup<NativeFunction<RenderFrameC>>('projectm_ffi_render_frame');

// projectm_ffi_load_preset
typedef LoadPresetC =
    Void Function(
      Pointer<Void> handle,
      Pointer<Utf8> path,
      Bool smoothTransition,
    );
typedef LoadPresetDart =
    void Function(
      Pointer<Void> handle,
      Pointer<Utf8> path,
      bool smoothTransition,
    );
final LoadPresetDart _projectmLoadPresetNative = dylib
    .lookupFunction<LoadPresetC, LoadPresetDart>('projectm_ffi_load_preset');

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
typedef StartAudioCaptureC = Bool Function(Pointer<Void> handle);
typedef StartAudioCaptureDart = bool Function(Pointer<Void> handle);
final StartAudioCaptureDart projectmStartAudioCapture = dylib
    .lookupFunction<StartAudioCaptureC, StartAudioCaptureDart>(
      'projectm_ffi_start_audio_capture',
    );

// projectm_ffi_stop_audio_capture
typedef StopAudioCaptureC = Void Function();
typedef StopAudioCaptureDart = void Function();
final StopAudioCaptureDart projectmStopAudioCapture = dylib
    .lookupFunction<StopAudioCaptureC, StopAudioCaptureDart>(
      'projectm_ffi_stop_audio_capture',
    );

// projectm_ffi_add_audio
typedef AddAudioC = Void Function(Pointer<Void> handle, Pointer<Float> data, Int32 frameCount);
typedef AddAudioDart = void Function(Pointer<Void> handle, Pointer<Float> data, int frameCount);
final AddAudioDart projectmAddAudio = dylib
    .lookupFunction<AddAudioC, AddAudioDart>(
      'projectm_ffi_add_audio',
    );

// --- End ---
