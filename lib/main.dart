import 'dart:async';
import 'dart:ffi';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:projectm_texture/projectm_texture.dart';
import 'package:visual_music/features/presets/preset_service.dart';

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

class _VisualizerScreenState extends State<VisualizerScreen> with SingleTickerProviderStateMixin {
  int? _textureId;
  Pointer<Void>? _pmHandle;
  late Ticker _ticker;

  @override
  void initState() {
    super.initState();
    _initProjectM();
  }

  Future<void> _initProjectM() async {
    print("Dart: Initializing Preset Service...");
    await PresetService.instance.init();
    
    print("Dart: Calling projectmInit()...");
    _pmHandle = projectmInit();
    print("Dart: projectmInit() returned: ${_pmHandle?.address}");

    print("Dart: Initializing Texture Plugin...");
    _textureId = await ProjectmTexture.initialize(
      800,
      600,
      projectmRenderFramePointer,
      _pmHandle!,
    );
    print("Dart: Texture Plugin initialized with ID: $_textureId");
    
    projectmSetWindowSize(_pmHandle!, 800, 600);
    
    print("Dart: Starting audio capture...");
    projectmStartAudioCapture(_pmHandle!);
    print("Dart: Audio capture started!");

    // Load a random preset
    final randomPreset = await PresetService.instance.getRandomUnbannedPreset();
    if (randomPreset != null) {
      projectmLoadPreset(_pmHandle!, randomPreset.path, true);
    }

    setState(() {});

    // 4. Start render loop
    bool _isRequesting = false;
    _ticker = createTicker((elapsed) async {
      if (_textureId != null && !_isRequesting) {
        _isRequesting = true;
        await ProjectmTexture.requestFrame(_textureId!);
        _isRequesting = false;
      }
    });
    _ticker.start();
  }

  @override
  void dispose() {
    _ticker.dispose();
    projectmStopAudioCapture();
    if (_pmHandle != null) {
      projectmDestroy(_pmHandle!);
    }
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.black,
      body: Center(
        child: _textureId == null
            ? const CircularProgressIndicator()
            : SizedBox(
                width: 800,
                height: 600,
                child: Texture(textureId: _textureId!),
              ),
      ),
    );
  }
}
