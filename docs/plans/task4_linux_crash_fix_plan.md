# Task 4 Linux Crash Fix Plan

## Goal

Make `flutter run -d linux` stable through implementation-plan Task 4:

- Initialize projectM without crashing.
- Start audio capture or its synthetic fallback without crashing.
- Extract/index bundled presets.
- Load and render a real `.milk` preset.

## Current Symptom

The app starts, initializes projectM, creates a texture, starts audio capture, queues a preset, then loses the device connection while loading the pending preset during rendering.

## Investigation Tasks

- [x] Inspect the Dart/native initialization order around projectM, the texture plugin, and the render callback.
- [x] Reproduce or collect enough crash output to identify whether the failure is in preset loading, OpenGL context usage, or audio shutdown.
- [x] Check for null handles and plugin lifecycle gaps that can turn native failures into process crashes.

## Implementation Tasks

- [x] Keep projectM lifecycle calls on the side of the boundary that owns the active OpenGL context where required.
- [x] Add defensive checks around native handles, texture IDs, and asynchronous startup/disposal.
- [x] Keep the first-load path simple: real preset rendering should work before adding Task 5 overlay behavior.

## Test Sketch

- [ ] `flutter analyze`
- [x] `flutter test`
- [x] `flutter build linux`
- [x] Manual Linux run: app reaches a visible projectM preset without losing device connection.
