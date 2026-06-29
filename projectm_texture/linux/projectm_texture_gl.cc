#include "include/projectm_texture/projectm_texture_plugin.h"
#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <mutex>
#include <iostream>

// ----- Flutter Texture GObject Boilerplate -----
#define PROJECTM_TEXTURE_GL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), projectm_texture_gl_get_type(), ProjectmTextureGL))

struct _ProjectmTextureGL {
  FlTextureGL parent_instance;
  GLuint front_texture;
  uint32_t width;
  uint32_t height;
};

G_DEFINE_TYPE(_ProjectmTextureGL, projectm_texture_gl, fl_texture_gl_get_type())

static void projectm_texture_gl_class_init(_ProjectmTextureGLClass* klass) {
  FlTextureGLClass* fl_texture_gl_class = FL_TEXTURE_GL_CLASS(klass);
  fl_texture_gl_class->populate = [](FlTextureGL* texture, uint32_t* target, uint32_t* format,
                                     uint32_t* width, uint32_t* height, GError** error) -> gboolean {
    _ProjectmTextureGL* self = PROJECTM_TEXTURE_GL(texture);
    *target = GL_TEXTURE_2D;
    *format = GL_RGBA8;
    *width = self->width;
    *height = self->height;
    return TRUE; // The texture ID is returned via a different method in newer Flutter headers, wait.
  };
}

// Actually, in Flutter Linux, populate returns TRUE, but HOW does it return the texture ID?
// Let's check flutter_linux headers...
