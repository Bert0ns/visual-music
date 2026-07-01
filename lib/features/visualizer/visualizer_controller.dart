import 'dart:async';
import 'dart:ffi';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:logger/logger.dart';
import 'package:path_provider/path_provider.dart';
import 'package:path/path.dart' as p;
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:projectm_texture/projectm_texture.dart';
import 'package:visual_music/core/audio/audio_controller.dart';
import 'package:visual_music/features/presets/preset_service.dart';

class VisualizerState {
  final int? textureId;
  final String? startupError;
  final Pointer<Void>? pmHandle;
  final Preset? currentPreset;
  final bool isAutoDjEnabled;
  final bool isUiVisible;

  VisualizerState({
    this.textureId,
    this.startupError,
    this.pmHandle,
    this.currentPreset,
    this.isAutoDjEnabled = true,
    this.isUiVisible = true,
  });

  VisualizerState copyWith({
    int? textureId,
    String? startupError,
    Pointer<Void>? pmHandle,
    Preset? currentPreset,
    bool? isAutoDjEnabled,
    bool? isUiVisible,
  }) {
    return VisualizerState(
      textureId: textureId ?? this.textureId,
      startupError: startupError ?? this.startupError,
      pmHandle: pmHandle ?? this.pmHandle,
      currentPreset: currentPreset ?? this.currentPreset,
      isAutoDjEnabled: isAutoDjEnabled ?? this.isAutoDjEnabled,
      isUiVisible: isUiVisible ?? this.isUiVisible,
    );
  }
}

class VisualizerController extends Notifier<VisualizerState> {
  final _logger = Logger();
  Timer? _autoDjTimer;
  Timer? _uiHideTimer;

  @override
  VisualizerState build() {
    // Start initialization when the controller is created
    Future.microtask(() => _initProjectM());
    _startUiHideTimer();
    return VisualizerState();
  }

  Future<void> _initProjectM() async {
    try {
      _logger.i("Dart: Initializing Preset Service...");
      final presetService = ref.read(presetServiceProvider);
      await presetService.init();

      _logger.i("Dart: Calling projectmInit()...");
      final pmHandle = projectmInit();
      _logger.i("Dart: projectmInit() returned: ${pmHandle.address}");
      if (pmHandle.address == 0) {
        state = state.copyWith(startupError: 'Could not initialize projectM.');
        return;
      }

      _logger.i("Dart: Setting texture search path...");
      final appDir = await getApplicationDocumentsDirectory();
      final texturesDir = p.join(appDir.path, 'visual_music', 'textures');
      projectmSetTextureSearchPath(pmHandle, texturesDir);

      _logger.i("Dart: Initializing AudioController...");
      ref.read(audioControllerProvider.notifier).init(pmHandle);

      _logger.i("Dart: Initializing Texture Plugin...");
      final textureId = await ProjectmTexture.initialize(
        400,
        300,
        projectmRenderFramePointer,
        pmHandle,
      );

      _logger.i("Dart: Setting Window Size...");
      projectmSetWindowSize(pmHandle, 400, 300);

      state = state.copyWith(textureId: textureId, pmHandle: pmHandle);

      _logger.i("Dart: Loading initial preset...");
      await loadNextPreset();

      if (state.isAutoDjEnabled) {
        _startAutoDj();
      }
    } catch (e, stack) {
      _logger.e("Dart: Error in _initProjectM", error: e, stackTrace: stack);
      state = state.copyWith(startupError: 'Error: $e');
    }
  }

  void _startAutoDj() {
    _autoDjTimer?.cancel();
    _autoDjTimer = Timer.periodic(const Duration(seconds: 15), (timer) {
      loadNextPreset();
    });
  }

  Future<void> loadNextPreset() async {
    if (state.pmHandle == null || state.pmHandle!.address == 0) return;
    
    final presetService = ref.read(presetServiceProvider);
    final nextPreset = await presetService.getRandomUnbannedPreset();

    if (nextPreset != null) {
      projectmLoadPreset(state.pmHandle!, nextPreset.path, true);
      state = state.copyWith(currentPreset: nextPreset);
    }
  }

  Future<void> toggleHeart() async {
    if (state.currentPreset != null) {
      final newHeartState = !state.currentPreset!.isHearted;
      final presetService = ref.read(presetServiceProvider);
      await presetService.toggleHeart(state.currentPreset!.path, newHeartState);
      state = state.copyWith(
        currentPreset: state.currentPreset!.copyWith(isHearted: newHeartState),
      );
    }
  }

  Future<void> banPreset() async {
    if (state.currentPreset != null) {
      final presetService = ref.read(presetServiceProvider);
      await presetService.banPreset(state.currentPreset!.path);
      await loadNextPreset();
    }
  }

  void toggleAutoDj() {
    final newValue = !state.isAutoDjEnabled;
    state = state.copyWith(isAutoDjEnabled: newValue);
    if (newValue) {
      _startAutoDj();
    } else {
      _autoDjTimer?.cancel();
      _autoDjTimer = null;
    }
  }

  void wakeUi() {
    if (!state.isUiVisible) {
      state = state.copyWith(isUiVisible: true);
    }
    _startUiHideTimer();
  }

  void _startUiHideTimer() {
    _uiHideTimer?.cancel();
    _uiHideTimer = Timer(const Duration(seconds: 3), () {
      state = state.copyWith(isUiVisible: false);
    });
  }

  void requestFrame() {
    if (state.textureId != null) {
      ProjectmTexture.requestFrame(state.textureId!);
    }
  }

  void disposeAll() {
    _autoDjTimer?.cancel();
    _uiHideTimer?.cancel();
    ref.read(audioControllerProvider.notifier).disposeAll();
    if (state.pmHandle != null && state.pmHandle!.address != 0) {
      projectmDestroy(state.pmHandle!);
    }
  }
}

final visualizerControllerProvider = NotifierProvider<VisualizerController, VisualizerState>(() {
  return VisualizerController();
});
