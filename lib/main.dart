import 'dart:async';
import 'package:visual_music/core/audio/system_audio_capture.dart';
import 'dart:ffi';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:projectm_texture/projectm_texture.dart';
import 'package:visual_music/features/presets/preset_service.dart';
import 'package:visual_music/features/visualizer/overlay_ui.dart';

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
    print("Dart: Initializing Preset Service...");
    await PresetService.instance.init();
    if (!mounted) return;

    print("Dart: Calling projectmInit()...");
    _pmHandle = projectmInit();
    print("Dart: projectmInit() returned: ${_pmHandle?.address}");
    if (_pmHandle == null || _pmHandle!.address == 0) {
      setState(() {
        _startupError = 'Could not initialize projectM.';
      });
      return;
    }

    print("Dart: Initializing Texture Plugin...");
    _textureId = await ProjectmTexture.initialize(
      800,
      600,
      projectmRenderFramePointer,
      _pmHandle!,
    );
    print("Dart: Texture Plugin initialized with ID: $_textureId");
    if (!mounted) return;

    projectmSetWindowSize(_pmHandle!, 800, 600);

    print("Dart: Starting audio capture...");
    SystemAudioCapture.start(_pmHandle!);
    print("Dart: Audio capture started!");

    // Load initial preset
    await _loadNextPreset();
    if (!mounted) return;

    setState(() {});

    // 4. Start render loop
    bool isRequesting = false;
    _ticker = createTicker((elapsed) async {
      if (_textureId != null && !isRequesting) {
        isRequesting = true;
        await ProjectmTexture.requestFrame(_textureId!);
        isRequesting = false;
      }
    });
    _ticker?.start();
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
            ],
          ),
        ),
      ),
    );
  }
}
