#ifndef FLUTTER_PLUGIN_PROJECTM_TEXTURE_PLUGIN_H_
#define FLUTTER_PLUGIN_PROJECTM_TEXTURE_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>

#include <memory>
#include <mutex>

class ProjectmTexturePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  ProjectmTexturePlugin(flutter::TextureRegistrar* texture_registrar);
  virtual ~ProjectmTexturePlugin();

  // Disallow copy and assign.
  ProjectmTexturePlugin(const ProjectmTexturePlugin&) = delete;
  ProjectmTexturePlugin& operator=(const ProjectmTexturePlugin&) = delete;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
      
  flutter::TextureRegistrar* texture_registrar_;
  int64_t texture_id_ = -1;
  std::unique_ptr<flutter::TextureVariant> texture_variant_;
};

#endif  // FLUTTER_PLUGIN_PROJECTM_TEXTURE_PLUGIN_H_
