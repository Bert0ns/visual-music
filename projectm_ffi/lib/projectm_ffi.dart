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
final _InitDart projectmInit =
    dylib.lookupFunction<_InitC, _InitDart>('projectm_ffi_init');

// projectm_ffi_destroy
typedef _DestroyC = Void Function(Pointer<Void> handle);
typedef _DestroyDart = void Function(Pointer<Void> handle);
final _DestroyDart projectmDestroy =
    dylib.lookupFunction<_DestroyC, _DestroyDart>('projectm_ffi_destroy');

// projectm_ffi_set_window_size
typedef _SetWindowSizeC = Void Function(Pointer<Void> handle, Int32 width, Int32 height);
typedef _SetWindowSizeDart = void Function(Pointer<Void> handle, int width, int height);
final _SetWindowSizeDart projectmSetWindowSize =
    dylib.lookupFunction<_SetWindowSizeC, _SetWindowSizeDart>('projectm_ffi_set_window_size');

// projectm_ffi_render_frame
typedef _RenderFrameC = Void Function(Pointer<Void> handle);
typedef _RenderFrameDart = void Function(Pointer<Void> handle);
final _RenderFrameDart projectmRenderFrame =
    dylib.lookupFunction<_RenderFrameC, _RenderFrameDart>('projectm_ffi_render_frame');

// projectm_ffi_load_preset
typedef _LoadPresetC = Void Function(Pointer<Void> handle, Pointer<Utf8> path, Bool smooth_transition);
typedef _LoadPresetDart = void Function(Pointer<Void> handle, Pointer<Utf8> path, bool smooth_transition);
final _LoadPresetDart projectmLoadPreset =
    dylib.lookupFunction<_LoadPresetC, _LoadPresetDart>('projectm_ffi_load_preset');

// --- Helper Functions ---
Pointer<NativeFunction<_RenderFrameC>> get projectmRenderFramePointer {
  return dylib.lookup<NativeFunction<_RenderFrameC>>('projectm_ffi_render_frame');
}
