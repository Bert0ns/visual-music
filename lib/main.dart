import 'dart:async';
import 'dart:ffi';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:projectm_ffi/projectm_ffi.dart';
import 'package:projectm_texture/projectm_texture.dart';

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
    // 1. Initialize projectM FFI (creates the instance)
    _pmHandle = projectmInit();

    // 2. Initialize Texture Plugin
    // Pass the render callback and the projectM instance handle
    _textureId = await ProjectmTexture.initialize(
      800,
      600,
      projectmRenderFramePointer,
      _pmHandle!,
    );
    
    projectmSetWindowSize(_pmHandle!, 800, 600);

    setState(() {});

    // 3. Start render loop
    _ticker = createTicker((elapsed) {
      if (_textureId != null) {
        ProjectmTexture.requestFrame(_textureId!);
      }
    });
    _ticker.start();
  }

  @override
  void dispose() {
    _ticker.dispose();
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
