import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'projectm_texture_platform_interface.dart';

/// An implementation of [ProjectmTexturePlatform] that uses method channels.
class MethodChannelProjectmTexture extends ProjectmTexturePlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('projectm_texture');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>(
      'getPlatformVersion',
    );
    return version;
  }
}
