import 'package:flutter_test/flutter_test.dart';
import 'package:projectm_texture/projectm_texture.dart';
import 'package:projectm_texture/projectm_texture_platform_interface.dart';
import 'package:projectm_texture/projectm_texture_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockProjectmTexturePlatform
    with MockPlatformInterfaceMixin
    implements ProjectmTexturePlatform {
  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final ProjectmTexturePlatform initialPlatform = ProjectmTexturePlatform.instance;

  test('$MethodChannelProjectmTexture is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelProjectmTexture>());
  });

  test('getPlatformVersion', () async {
    ProjectmTexture projectmTexturePlugin = ProjectmTexture();
    MockProjectmTexturePlatform fakePlatform = MockProjectmTexturePlatform();
    ProjectmTexturePlatform.instance = fakePlatform;

    expect(await projectmTexturePlugin.getPlatformVersion(), '42');
  });
}
