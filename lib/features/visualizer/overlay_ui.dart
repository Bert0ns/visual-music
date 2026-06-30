import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:visual_music/features/presets/preset_service.dart';

class OverlayUI extends StatelessWidget {
  final Preset? currentPreset;
  final bool isAutoDjEnabled;
  final VoidCallback onNextPreset;
  final VoidCallback onToggleAutoDj;
  final VoidCallback onToggleHeart;
  final VoidCallback onBanPreset;

  const OverlayUI({
    super.key,
    required this.currentPreset,
    required this.isAutoDjEnabled,
    required this.onNextPreset,
    required this.onToggleAutoDj,
    required this.onToggleHeart,
    required this.onBanPreset,
  });

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
        decoration: BoxDecoration(
          color: Colors.black.withValues(
            alpha: 0.7,
          ), // Solid transparent instead of blur
          borderRadius: BorderRadius.circular(30),
          border: Border.all(color: Colors.white.withValues(alpha: 0.2)),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            IconButton(
              icon: const Icon(Icons.settings, color: Colors.white),
              onPressed: () {
                // TODO: Implement settings
              },
            ),
            const SizedBox(width: 16),
            Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  currentPreset?.name ?? 'Idle',
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(height: 4),
                const Text(
                  'ProjectM Visualizer',
                  style: TextStyle(color: Colors.white70, fontSize: 12),
                ),
              ],
            ),
            const SizedBox(width: 16),
            IconButton(
              icon: Icon(
                currentPreset?.isHearted == true ? Icons.favorite : Icons.favorite_border,
                color: currentPreset?.isHearted == true ? Colors.redAccent : Colors.white,
              ),
              onPressed: currentPreset != null ? onToggleHeart : null,
              tooltip: 'Heart Preset',
            ),
            IconButton(
              icon: const Icon(Icons.block, color: Colors.white),
              onPressed: currentPreset != null ? onBanPreset : null,
              tooltip: 'Ban Preset',
            ),
            IconButton(
              icon: Icon(
                isAutoDjEnabled ? Icons.auto_awesome : Icons.auto_awesome_outlined,
                color: isAutoDjEnabled ? Colors.tealAccent : Colors.white,
              ),
              onPressed: onToggleAutoDj,
              tooltip: 'Auto-DJ',
            ),
            IconButton(
              icon: const Icon(Icons.skip_next, color: Colors.white),
              onPressed: onNextPreset,
              tooltip: 'Next Preset',
            ),
          ],
        ),
      ),
    );
  }
}
