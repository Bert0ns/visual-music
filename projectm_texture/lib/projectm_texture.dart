
import 'projectm_texture_platform_interface.dart';

class ProjectmTexture {
  Future<String?> getPlatformVersion() {
    return ProjectmTexturePlatform.instance.getPlatformVersion();
  }
}
