import 'dart:async';
import 'package:logger/logger.dart';
import 'package:visual_music/core/audio/system_audio_capture.dart';
import 'dart:ffi';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:projectm_texture/projectm_texture.dart';
import 'package:visual_music/features/presets/preset_service.dart';
import 'package:visual_music/features/visualizer/overlay_ui.dart';
import 'package:visual_music/core/audio/internal_audio_player.dart';
import 'package:visual_music/features/visualizer/widgets/music_player_bar.dart';
import 'package:path_provider/path_provider.dart';
import 'package:path/path.dart' as p;

final _logger = Logger();

void main() {
  runApp(const VisualMusicApp());
}

class VisualMusicApp extends StatelessWidget {
  const VisualMusicApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Visual Music',
      theme: ThemeData.dark(),
      home: const VisualizerScreen(),
    );
  }
}

class VisualizerScreen extends StatefulWidget {
  const VisualizerScreen({super.key});

  @override
  State<VisualizerScreen> createState() => _VisualizerScreenState();
}

class _VisualizerScreenState extends State<VisualizerScreen>
    with SingleTickerProviderStateMixin {
  int? _textureId;
  Pointer<Void>? _pmHandle;
  Ticker? _ticker;
  Preset? _currentPreset;
  bool _isAutoDjEnabled = false;
  Timer? _autoDjTimer;
  String? _startupError;
  bool _isUiVisible = true;
  Timer? _uiHideTimer;

  @override
  void initState() {
    super.initState();
    _initProjectM();
    _wakeUi();
  }

  Future<void> _initProjectM() async {
    try {
      _logger.i("Dart: Initializing Preset Service...");
      await PresetService.instance.init();
      if (!mounted) return;

      _logger.i("Dart: Calling projectmInit()...");
      _pmHandle = projectmInit();
      _logger.i("Dart: projectmInit() returned: ${_pmHandle?.address}");
      if (_pmHandle == null || _pmHandle!.address == 0) {
        setState(() {
          _startupError = 'Could not initialize projectM.';
        });
        return;
      }

      _logger.i("Dart: Setting texture search path...");
      final appDir = await getApplicationDocumentsDirectory();
      final texturesDir = p.join(appDir.path, 'visual_music', 'textures');
      _logger.i("Dart: Textures dir: $texturesDir");
      projectmSetTextureSearchPath(_pmHandle!, texturesDir);
      _logger.i("Dart: Texture search path set!");

      _logger.i("Dart: Initializing InternalAudioPlayer...");
      InternalAudioPlayer.instance.init(_pmHandle!);

      _logger.i("Dart: Initializing Texture Plugin...");
      _textureId = await ProjectmTexture.initialize(
        800,
        600,
        projectmRenderFramePointer,
        _pmHandle!,
      );
      _logger.i("Dart: Texture Plugin initialized with ID: $_textureId");
      if (!mounted) return;

      _logger.i("Dart: Setting Window Size...");
      projectmSetWindowSize(_pmHandle!, 800, 600);

      _logger.i("Dart: Starting audio capture...");
      SystemAudioCapture.start(_pmHandle!);
      _logger.i("Dart: Audio capture started!");

      _logger.i("Dart: Loading initial preset...");
      await _loadNextPreset();
      if (!mounted) return;

      _logger.i("Dart: Updating state to clear spinner...");
      setState(() {});

      _logger.i("Dart: Starting render ticker...");
      bool isRequesting = false;
      Duration lastFrameTime = Duration.zero;
      _ticker = createTicker((elapsed) async {
        // Cap framerate to ~30 FPS (33ms) to drastically reduce CPU usage 
        // since WSL uses llvmpipe (software rendering on the CPU).
        if (elapsed - lastFrameTime < const Duration(milliseconds: 33)) return;
        lastFrameTime = elapsed;

        if (_textureId != null && !isRequesting) {
          isRequesting = true;
          await ProjectmTexture.requestFrame(_textureId!);
          isRequesting = false;
        }
      });
      _ticker?.start();
      _logger.i("Dart: Init sequence completely finished!");
    } catch (e, stack) {
      _logger.e("Dart: Error initializing ProjectM", error: e, stackTrace: stack);
      if (mounted) {
        setState(() {
          _startupError = 'Error: $e';
        });
      }
    }
  }

  @override
  void dispose() {
    _autoDjTimer?.cancel();
    _uiHideTimer?.cancel();
    _ticker?.dispose();
    SystemAudioCapture.stop();
    if (_pmHandle != null && _pmHandle!.address != 0) {
      projectmDestroy(_pmHandle!);
    }
    super.dispose();
  }

  void _wakeUi() {
    if (!mounted) return;
    setState(() {
      _isUiVisible = true;
    });
    _uiHideTimer?.cancel();
    _uiHideTimer = Timer(const Duration(seconds: 3), () {
      if (mounted) {
        setState(() {
          _isUiVisible = false;
        });
      }
    });
  }

  Future<void> _loadNextPreset() async {
    final nextPreset = await PresetService.instance.getRandomUnbannedPreset();
    if (!mounted) return;

    if (nextPreset != null && _pmHandle != null && _pmHandle!.address != 0) {
      projectmLoadPreset(_pmHandle!, nextPreset.path, true);
      setState(() {
        _currentPreset = nextPreset;
      });
    }
  }

  Future<void> _toggleHeart() async {
    if (_currentPreset != null) {
      final newHeartState = !_currentPreset!.isHearted;
      await PresetService.instance.toggleHeart(
        _currentPreset!.path,
        newHeartState,
      );
      setState(() {
        _currentPreset = _currentPreset!.copyWith(isHearted: newHeartState);
      });
    }
  }

  Future<void> _banPreset() async {
    if (_currentPreset != null) {
      await PresetService.instance.banPreset(_currentPreset!.path);
      _loadNextPreset();
    }
  }

  void _toggleAutoDj() {
    setState(() {
      _isAutoDjEnabled = !_isAutoDjEnabled;
      if (_isAutoDjEnabled) {
        _autoDjTimer = Timer.periodic(const Duration(seconds: 15), (timer) {
          _loadNextPreset();
        });
      } else {
        _autoDjTimer?.cancel();
        _autoDjTimer = null;
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.black,
      body: MouseRegion(
        onHover: (_) => _wakeUi(),
        child: GestureDetector(
          onTap: _wakeUi,
          onPanUpdate: (_) => _wakeUi(),
          behavior: HitTestBehavior.translucent,
          child: Stack(
            children: [
              Center(
                child: _startupError != null
                    ? Text(
                        _startupError!,
                        style: const TextStyle(color: Colors.white),
                      )
                    : _textureId == null
                    ? const CircularProgressIndicator()
                    : SizedBox(
                        width: double.infinity,
                        height: double.infinity,
                        child: Texture(textureId: _textureId!),
                      ),
              ),
              if (_textureId != null)
                Positioned(
                  bottom: 40,
                  left: 0,
                  right: 0,
                  child: AnimatedOpacity(
                    opacity: _isUiVisible ? 1.0 : 0.0,
                    duration: const Duration(milliseconds: 300),
                    child: IgnorePointer(
                      ignoring: !_isUiVisible,
                      child: OverlayUI(
                        currentPreset: _currentPreset,
                        isAutoDjEnabled: _isAutoDjEnabled,
                        onNextPreset: () {
                          _wakeUi();
                          _loadNextPreset();
                        },
                        onToggleAutoDj: () {
                          _wakeUi();
                          _toggleAutoDj();
                        },
                        onToggleHeart: () {
                          _wakeUi();
                          _toggleHeart();
                        },
                        onBanPreset: () {
                          _wakeUi();
                          _banPreset();
                        },
                      ),
                    ),
                  ),
                ),
              if (_textureId != null)
                Positioned(
                  top: 40,
                  left: 0,
                  right: 0,
                  child: AnimatedOpacity(
                    opacity: _isUiVisible ? 1.0 : 0.0,
                    duration: const Duration(milliseconds: 300),
                    child: IgnorePointer(
                      ignoring: !_isUiVisible,
                      child: const Center(
                        child: MusicPlayerBar(),
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}
