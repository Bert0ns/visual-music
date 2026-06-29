import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'projectm_texture_method_channel.dart';

abstract class ProjectmTexturePlatform extends PlatformInterface {
  /// Constructs a ProjectmTexturePlatform.
  ProjectmTexturePlatform() : super(token: _token);

  static final Object _token = Object();

  static ProjectmTexturePlatform _instance = MethodChannelProjectmTexture();

  /// The default instance of [ProjectmTexturePlatform] to use.
  ///
  /// Defaults to [MethodChannelProjectmTexture].
  static ProjectmTexturePlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [ProjectmTexturePlatform] when
  /// they register themselves.
  static set instance(ProjectmTexturePlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<String?> getPlatformVersion() {
    throw UnimplementedError('platformVersion() has not been implemented.');
  }
}
