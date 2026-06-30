#include "projectm_texture_plugin.h"
#include <windows.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include "gl_d3d_interop.h"
#include <iostream>

namespace {

class ProjectmTexturePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  ProjectmTexturePlugin(flutter::TextureRegistrar* texture_registrar);
  virtual ~ProjectmTexturePlugin();

  // Disallow copy and assign.
  ProjectmTexturePlugin(const ProjectmTexturePlugin&) = delete;
  ProjectmTexturePlugin& operator=(const ProjectmTexturePlugin&) = delete;

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  flutter::TextureRegistrar* texture_registrar_;
  int64_t texture_id_ = -1;
  std::unique_ptr<flutter::TextureVariant> texture_variant_;
  std::unique_ptr<GLD3DInterop> interop_;
};

// C API Registration
void ProjectmTexturePluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  ProjectmTexturePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

void ProjectmTexturePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "projectm_texture",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<ProjectmTexturePlugin>(registrar->texture_registrar());

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

ProjectmTexturePlugin::ProjectmTexturePlugin(flutter::TextureRegistrar* texture_registrar)
    : texture_registrar_(texture_registrar), interop_(std::make_unique<GLD3DInterop>()) {}

ProjectmTexturePlugin::~ProjectmTexturePlugin() {
  if (texture_id_ != -1) {
    texture_registrar_->UnregisterTexture(texture_id_);
  }
}

void ProjectmTexturePlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  
  if (method_call.method_name() == "initialize") {
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    int width = 800;
    int height = 600;
    
    if (args) {
      auto width_it = args->find(flutter::EncodableValue("width"));
      if (width_it != args->end() && std::holds_alternative<int>(width_it->second)) {
        width = std::get<int>(width_it->second);
      }
      auto height_it = args->find(flutter::EncodableValue("height"));
      if (height_it != args->end() && std::holds_alternative<int>(height_it->second)) {
        height = std::get<int>(height_it->second);
      }
    }

    if (texture_id_ != -1) {
      texture_registrar_->UnregisterTexture(texture_id_);
      texture_id_ = -1;
    }

    if (!interop_->Initialize(width, height)) {
      result->Error("GL_D3D_INIT_FAILED", "Failed to initialize GL-D3D interop");
      return;
    }

    HANDLE shared_handle = interop_->GetDXGISharedHandle();
    
    texture_variant_ = std::make_unique<flutter::TextureVariant>(
        flutter::GpuSurfaceTexture(
            kFlutterDesktopGpuSurfaceTypeDxgiSharedHandle, 
            shared_handle));

    texture_id_ = texture_registrar_->RegisterTexture(texture_variant_.get());
    
    flutter::EncodableMap response;
    response[flutter::EncodableValue("textureId")] = flutter::EncodableValue(texture_id_);
    response[flutter::EncodableValue("fboId")] = flutter::EncodableValue((int)interop_->GetFBO());
    
    result->Success(flutter::EncodableValue(response));
    
  } else if (method_call.method_name() == "requestFrame") {
    if (texture_id_ != -1) {
      texture_registrar_->MarkTextureFrameAvailable(texture_id_);
    }
    result->Success(flutter::EncodableValue(true));
  } else if (method_call.method_name() == "lock") {
      interop_->LockForGL();
      result->Success(flutter::EncodableValue(true));
  } else if (method_call.method_name() == "unlock") {
      interop_->UnlockFromGL();
      result->Success(flutter::EncodableValue(true));
  } else {
    result->NotImplemented();
  }
}

}  // namespace

// C API Export
#include "include/projectm_texture/projectm_texture_plugin_c_api.h"
void ProjectmTexturePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  ProjectmTexturePluginRegisterWithRegistrar(registrar);
}
