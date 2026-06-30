import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:visual_music/features/presets/preset_service.dart';
import 'package:visual_music/features/visualizer/overlay_ui.dart';

void main() {
  testWidgets('overlay shows current preset and controls', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: Stack(
            children: [
              OverlayUI(
                currentPreset: Preset(
                  id: 1,
                  name: 'Test Preset',
                  path: '/test/path',
                  isBanned: false,
                  isHearted: false,
                ),
                isAutoDjEnabled: true,
                onNextPreset: () {},
                onToggleAutoDj: () {},
                onToggleHeart: () {},
                onBanPreset: () {},
              ),
            ],
          ),
        ),
      ),
    );

    expect(find.text('Test Preset'), findsOneWidget);
    expect(find.byIcon(Icons.auto_awesome), findsOneWidget);
    expect(find.byIcon(Icons.skip_next), findsOneWidget);
  });
}
