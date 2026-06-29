#include <flutter_linux/flutter_linux.h>
#include <epoxy/gl.h>

struct _MyTexture {
  FlTextureGL parent_instance;
  GLuint texture_id;
};

G_DEFINE_TYPE(MyTexture, my_texture, fl_texture_gl_get_type())

static gboolean my_texture_populate(FlTextureGL* texture,
                                    uint32_t* target,
                                    uint32_t* format,
                                    uint32_t* width,
                                    uint32_t* height,
                                    GError** error) {
  MyTexture* self = (MyTexture*)texture;
  *target = GL_TEXTURE_2D;
  *format = GL_RGBA8;
  return TRUE;
}
