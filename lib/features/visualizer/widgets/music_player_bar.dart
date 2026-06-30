import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:file_picker/file_picker.dart';
import 'package:visual_music/core/audio/internal_audio_player.dart';

class MusicPlayerBar extends StatefulWidget {
  const MusicPlayerBar({super.key});

  @override
  State<MusicPlayerBar> createState() => _MusicPlayerBarState();
}

class _MusicPlayerBarState extends State<MusicPlayerBar> {
  final _player = InternalAudioPlayer.instance;

  @override
  void initState() {
    super.initState();
    _player.addListener(_onPlayerStateChanged);
  }

  @override
  void dispose() {
    _player.removeListener(_onPlayerStateChanged);
    super.dispose();
  }

  void _onPlayerStateChanged() {
    setState(() {});
  }

  Future<void> _pickAndPlayFile() async {
    try {
      FilePickerResult? result = await FilePicker.pickFiles(
        type: FileType.custom,
        allowedExtensions: ['mp3', 'wav', 'flac', 'ogg'],
      );

      if (result != null && result.files.single.path != null) {
        await _player.playFile(result.files.single.path!);
      }
    } catch (e) {
      debugPrint("Native file picker failed: $e");
      if (mounted) {
        _showManualPathDialog();
      }
    }
  }

  void _showManualPathDialog() {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          backgroundColor: Colors.grey[900],
          title: const Text("Enter MP3 File Path", style: TextStyle(color: Colors.white)),
          content: TextField(
            controller: controller,
            style: const TextStyle(color: Colors.white),
            decoration: const InputDecoration(
              hintText: "/home/berto/visual-music/test.mp3",
              hintStyle: TextStyle(color: Colors.white54),
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text("Cancel"),
            ),
            TextButton(
              onPressed: () {
                Navigator.pop(context);
                if (controller.text.isNotEmpty) {
                  _player.playFile(controller.text);
                }
              },
              child: const Text("Play"),
            ),
          ],
        );
      }
    );
  }

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: BorderRadius.circular(24),
      child: BackdropFilter(
        filter: ImageFilter.blur(sigmaX: 12, sigmaY: 12),
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
          decoration: BoxDecoration(
            color: Colors.black.withValues(alpha: 0.4),
            borderRadius: BorderRadius.circular(24),
            border: Border.all(color: Colors.white.withValues(alpha: 0.15)),
          ),
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              IconButton(
                icon: const Icon(Icons.library_music, color: Colors.white70),
                onPressed: _pickAndPlayFile,
                tooltip: "Load Track",
              ),
              const SizedBox(width: 8),
              if (_player.state == AudioPlayerState.playing)
                IconButton(
                  icon: const Icon(Icons.pause, color: Colors.white, size: 32),
                  onPressed: _player.pause,
                )
              else
                IconButton(
                  icon: const Icon(Icons.play_arrow, color: Colors.white, size: 32),
                  onPressed: _player.state == AudioPlayerState.paused
                      ? _player.resume
                      : _pickAndPlayFile,
                ),
              if (_player.state != AudioPlayerState.stopped) ...[
                IconButton(
                  icon: const Icon(Icons.stop, color: Colors.white70),
                  onPressed: _player.stop,
                ),
              ],
              const SizedBox(width: 16),
              Container(
                constraints: const BoxConstraints(maxWidth: 250),
                child: Text(
                  _player.currentTrackName ?? "No Track Loaded",
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 16,
                    fontWeight: FontWeight.w500,
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
