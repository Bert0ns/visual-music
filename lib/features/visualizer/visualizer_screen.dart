import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:visual_music/core/audio/audio_controller.dart';
import 'package:visual_music/features/visualizer/overlay_ui.dart';
import 'package:visual_music/features/visualizer/visualizer_controller.dart';
import 'package:visual_music/features/visualizer/widgets/music_player_bar.dart';

class VisualizerScreen extends ConsumerStatefulWidget {
  const VisualizerScreen({super.key});

  @override
  ConsumerState<VisualizerScreen> createState() => _VisualizerScreenState();
}

class _VisualizerScreenState extends ConsumerState<VisualizerScreen>
    with SingleTickerProviderStateMixin {
  Ticker? _ticker;
  bool isRequesting = false;
  Duration lastFrameTime = Duration.zero;

  @override
  void initState() {
    super.initState();
    _startTicker();
  }

  void _startTicker() {
    _ticker = createTicker((elapsed) {
      final audioState = ref.read(audioControllerProvider);
      
      // If playing a local file or system audio is active, run at higher framerate
      final isActiveAudio = audioState.activeSource == AudioSource.system || 
                            audioState.localPlaybackStatus == AudioPlaybackStatus.playing;
                            
      final frameInterval = Duration(milliseconds: isActiveAudio ? 50 : 33);
      
      if (elapsed - lastFrameTime < frameInterval) return;
      lastFrameTime = elapsed;

      final controller = ref.read(visualizerControllerProvider.notifier);
      final state = ref.read(visualizerControllerProvider);
      
      if (state.textureId != null && !isRequesting) {
        isRequesting = true;
        controller.requestFrame();
        isRequesting = false;
      }
    });
    _ticker?.start();
  }

  @override
  void dispose() {
    _ticker?.dispose();
    ref.read(visualizerControllerProvider.notifier).disposeAll();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(visualizerControllerProvider);
    final controller = ref.read(visualizerControllerProvider.notifier);

    return Scaffold(
      backgroundColor: Colors.black,
      body: MouseRegion(
        onHover: (_) => controller.wakeUi(),
        child: GestureDetector(
          onTap: controller.wakeUi,
          onPanUpdate: (_) => controller.wakeUi(),
          behavior: HitTestBehavior.translucent,
          child: Stack(
            children: [
              Center(
                child: state.startupError != null
                    ? Text(
                        state.startupError!,
                        style: const TextStyle(color: Colors.white),
                      )
                    : state.textureId == null
                        ? const CircularProgressIndicator()
                        : SizedBox(
                            width: double.infinity,
                            height: double.infinity,
                            child: Texture(textureId: state.textureId!),
                          ),
              ),
              if (state.textureId != null)
                Positioned(
                  bottom: 40,
                  left: 0,
                  right: 0,
                  child: AnimatedOpacity(
                    opacity: state.isUiVisible ? 1.0 : 0.0,
                    duration: const Duration(milliseconds: 300),
                    child: IgnorePointer(
                      ignoring: !state.isUiVisible,
                      child: OverlayUI(
                        currentPreset: state.currentPreset,
                        isAutoDjEnabled: state.isAutoDjEnabled,
                        onNextPreset: () {
                          controller.wakeUi();
                          controller.loadNextPreset();
                        },
                        onToggleAutoDj: () {
                          controller.wakeUi();
                          controller.toggleAutoDj();
                        },
                        onToggleHeart: () {
                          controller.wakeUi();
                          controller.toggleHeart();
                        },
                        onBanPreset: () {
                          controller.wakeUi();
                          controller.banPreset();
                        },
                      ),
                    ),
                  ),
                ),
              if (state.textureId != null)
                Positioned(
                  top: 40,
                  left: 0,
                  right: 0,
                  child: AnimatedOpacity(
                    opacity: state.isUiVisible ? 1.0 : 0.0,
                    duration: const Duration(milliseconds: 300),
                    child: IgnorePointer(
                      ignoring: !state.isUiVisible,
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
