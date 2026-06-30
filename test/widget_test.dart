import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
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
                currentPresetName: 'Test Preset',
                isAutoDjEnabled: true,
                onNextPreset: () {},
                onToggleAutoDj: () {},
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
