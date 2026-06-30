import 'dart:ui';
import 'package:flutter/material.dart';

class OverlayUI extends StatelessWidget {
  final String? currentPresetName;
  final bool isAutoDjEnabled;
  final VoidCallback onNextPreset;
  final VoidCallback onToggleAutoDj;

  const OverlayUI({
    super.key,
    required this.currentPresetName,
    required this.isAutoDjEnabled,
    required this.onNextPreset,
    required this.onToggleAutoDj,
  });

  @override
  Widget build(BuildContext context) {
    return Positioned(
      bottom: 40,
      left: 0,
      right: 0,
      child: Center(
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(
              0.7,
            ), // Solid transparent instead of blur
            borderRadius: BorderRadius.circular(30),
            border: Border.all(color: Colors.white.withOpacity(0.2)),
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
                    currentPresetName ?? 'Idle',
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
                  isAutoDjEnabled
                      ? Icons.auto_awesome
                      : Icons.auto_awesome_outlined,
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
      ),
    );
  }
}
