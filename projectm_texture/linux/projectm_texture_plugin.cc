#include "include/projectm_texture/projectm_texture_plugin.h"
#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <unordered_map>
#include <iostream>

#define PROJECTM_TEXTURE_GL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), projectm_texture_gl_get_type(), ProjectmTextureGL))

typedef void (*RenderCallback)(void*, uint32_t, uint32_t);

typedef struct _ProjectmTextureGL ProjectmTextureGL;
typedef struct {
  FlTextureGLClass parent_class;
} ProjectmTextureGLClass;

struct _ProjectmTextureGL {
  FlTextureGL parent_instance;
  GLuint fbo_id;
  GLuint texture_id;
  GLuint depth_rb;
  uint32_t width;
  uint32_t height;
  RenderCallback render_cb;
  void* render_ctx;
};

G_DEFINE_TYPE(ProjectmTextureGL, projectm_texture_gl, fl_texture_gl_get_type())

static void projectm_texture_gl_dispose(GObject* object) {
  ProjectmTextureGL* self = PROJECTM_TEXTURE_GL(object);
  if (self->texture_id != 0) {
    glDeleteTextures(1, &self->texture_id);
    self->texture_id = 0;
  }
  if (self->fbo_id != 0) {
    glDeleteFramebuffers(1, &self->fbo_id);
  }
  if (self->depth_rb != 0) {
    glDeleteRenderbuffers(1, &self->depth_rb);
  }
  self->fbo_id = 0;
  self->depth_rb = 0;
  G_OBJECT_CLASS(projectm_texture_gl_parent_class)->dispose(object);
}

static gboolean projectm_texture_gl_populate(FlTextureGL* texture, uint32_t* target,
                                             uint32_t* name, uint32_t* width,
                                             uint32_t* height, GError** error) {
  ProjectmTextureGL* self = PROJECTM_TEXTURE_GL(texture);

  if (self->texture_id == 0) {
    glGenTextures(1, &self->texture_id);
    glBindTexture(GL_TEXTURE_2D, self->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->width, self->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glGenFramebuffers(1, &self->fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->texture_id, 0);

    // CRITICAL: ProjectM requires a depth/stencil buffer for many presets.
    // Without it, glClear(GL_DEPTH_BUFFER_BIT) fails, causing the frame to abort silently!
    glGenRenderbuffers(1, &self->depth_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, self->depth_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, self->width, self->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, self->depth_rb);
  }

  // Save state
  GLint old_fbo, old_program, old_vao, old_vbo, old_tex, old_active_tex;
  GLint old_viewport[4], old_scissor[4];
  GLboolean old_blend, old_depth, old_cull, old_scissor_test;
  GLboolean old_color_mask[4], old_depth_mask;
  GLint old_unpack_align, old_pack_align;
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &old_unpack_align);
  glGetIntegerv(GL_PACK_ALIGNMENT, &old_pack_align);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &old_active_tex);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
  glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &old_vbo);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_tex);
  glGetIntegerv(GL_VIEWPORT, old_viewport);
  glGetIntegerv(GL_SCISSOR_BOX, old_scissor);
  glGetBooleanv(GL_COLOR_WRITEMASK, old_color_mask);
  glGetBooleanv(GL_DEPTH_WRITEMASK, &old_depth_mask);
  old_blend = glIsEnabled(GL_BLEND);
  old_depth = glIsEnabled(GL_DEPTH_TEST);
  old_cull = glIsEnabled(GL_CULL_FACE);
  old_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
  
  glBindFramebuffer(GL_FRAMEBUFFER, self->fbo_id);
  glBindFramebuffer(GL_FRAMEBUFFER, self->fbo_id);
  glViewport(0, 0, self->width, self->height);

  if (self->render_cb) {
    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo_id);
    glViewport(0, 0, self->width, self->height);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // Clear to RED
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT);

    // 2. Call ProjectM
    self->render_cb(self->render_ctx, self->width, self->height);
    
    // 3. Re-bind and force Alpha to 1.0 so it's opaque to Flutter
    glBindFramebuffer(GL_FRAMEBUFFER, self->fbo_id);
    
    // CRITICAL: ProjectM often leaves GL_SCISSOR_TEST enabled, which restricts 
    // where glClear can draw! If scissor is active, our alpha fix does NOTHING.
    glDisable(GL_SCISSOR_TEST);
    
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // RED background!
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    
    glFlush();
  } else {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  // Restore state
  glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
  glUseProgram(old_program);
  glBindVertexArray(old_vao);
  glBindBuffer(GL_ARRAY_BUFFER, old_vbo);
  glActiveTexture(old_active_tex);
  glBindTexture(GL_TEXTURE_2D, old_tex);
  glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);
  glScissor(old_scissor[0], old_scissor[1], old_scissor[2], old_scissor[3]);
  glColorMask(old_color_mask[0], old_color_mask[1], old_color_mask[2], old_color_mask[3]);
  glDepthMask(old_depth_mask);
  glPixelStorei(GL_UNPACK_ALIGNMENT, old_unpack_align);
  glPixelStorei(GL_PACK_ALIGNMENT, old_pack_align);
  
  if (old_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
  if (old_depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
  if (old_cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
  if (old_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

  *target = GL_TEXTURE_2D;
  *name = self->texture_id;
  *width = self->width;
  *height = self->height;

  return TRUE;
}

static void projectm_texture_gl_class_init(ProjectmTextureGLClass* klass) {
  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->dispose = projectm_texture_gl_dispose;
  FL_TEXTURE_GL_CLASS(klass)->populate = projectm_texture_gl_populate;
}

static void projectm_texture_gl_init(ProjectmTextureGL* self) {
  self->fbo_id = 0;
  self->texture_id = 0;
  self->width = 800;
  self->height = 600;
  self->render_cb = nullptr;
  self->render_ctx = nullptr;
}

static ProjectmTextureGL* projectm_texture_gl_new(uint32_t width, uint32_t height) {
  ProjectmTextureGL* self = PROJECTM_TEXTURE_GL(g_object_new(projectm_texture_gl_get_type(), nullptr));
  self->width = width;
  self->height = height;
  return self;
}

// ----- Plugin Boilerplate -----

#define PROJECTM_TEXTURE_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), projectm_texture_plugin_get_type(), \
                              ProjectmTexturePlugin))

struct _ProjectmTexturePlugin {
  GObject parent_instance;
  FlTextureRegistrar* texture_registrar;
};

G_DEFINE_TYPE(ProjectmTexturePlugin, projectm_texture_plugin, g_object_get_type())

static std::unordered_map<int64_t, ProjectmTextureGL*> global_textures;

static void projectm_texture_plugin_handle_method_call(ProjectmTexturePlugin* self, FlMethodCall* method_call) {
  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, "initialize") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    uint32_t width = fl_value_get_int(fl_value_lookup_string(args, "width"));
    uint32_t height = fl_value_get_int(fl_value_lookup_string(args, "height"));

    ProjectmTextureGL* texture = projectm_texture_gl_new(width, height);
    fl_texture_registrar_register_texture(self->texture_registrar, FL_TEXTURE(texture));
    
    // fl_texture_registrar_register_texture assigns an internal ID. We must get it using fl_texture_get_id.
    int64_t actual_id = fl_texture_get_id(FL_TEXTURE(texture));
    
    // Store the mapping so we can retrieve it in requestFrame or setCallback
    global_textures[actual_id] = texture;

    g_autoptr(FlValue) result = fl_value_new_int(actual_id);
    fl_method_call_respond(method_call, FL_METHOD_RESPONSE(fl_method_success_response_new(result)), nullptr);
  } else if (strcmp(method, "requestFrame") == 0) {
    FlValue* args = fl_method_call_get_args(method_call);
    int64_t texture_id = fl_value_get_int(fl_value_lookup_string(args, "textureId"));
    
    if (global_textures.count(texture_id)) {
      fl_texture_registrar_mark_texture_frame_available(self->texture_registrar, FL_TEXTURE(global_textures[texture_id]));
      fl_method_call_respond(method_call, FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr)), nullptr);
    } else {
      fl_method_call_respond(method_call, FL_METHOD_RESPONSE(fl_method_error_response_new("invalid_id", "Texture not found", nullptr)), nullptr);
    }
  } else {
    fl_method_call_respond(method_call, FL_METHOD_RESPONSE(fl_method_not_implemented_response_new()), nullptr);
  }
}

static void projectm_texture_plugin_class_init(ProjectmTexturePluginClass* klass) {}
static void projectm_texture_plugin_init(ProjectmTexturePlugin* self) {}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call, gpointer user_data) {
  ProjectmTexturePlugin* plugin = PROJECTM_TEXTURE_PLUGIN(user_data);
  projectm_texture_plugin_handle_method_call(plugin, method_call);
}

void projectm_texture_plugin_register_with_registrar(FlPluginRegistrar* registrar) {
  ProjectmTexturePlugin* plugin = PROJECTM_TEXTURE_PLUGIN(g_object_new(projectm_texture_plugin_get_type(), nullptr));
  plugin->texture_registrar = fl_plugin_registrar_get_texture_registrar(registrar);

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "projectm_texture", FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(channel, method_call_cb, g_object_ref(plugin), g_object_unref);
  g_object_unref(plugin);
}

// ----- FFI EXPORTS -----
extern "C" __attribute__((visibility("default"))) 
void projectm_texture_set_callback(int64_t texture_id, RenderCallback cb, void* ctx) {
  if (global_textures.count(texture_id)) {
    global_textures[texture_id]->render_cb = cb;
    global_textures[texture_id]->render_ctx = ctx;
  }
}
