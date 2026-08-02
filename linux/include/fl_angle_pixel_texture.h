#ifndef FL_ANGLE_PIXEL_TEXTURE_H
#define FL_ANGLE_PIXEL_TEXTURE_H

#include <flutter_linux/flutter_linux.h>
#include <atomic>
#include <cstdint>

// A texture Flutter reads out of ordinary memory.
//
// The GL texture route (FlTextureGL) cannot work from a plugin on current
// Flutter: the engine creates its GL contexts with plain EGL and shares them
// with nothing, while a plugin can only get a GDK context, which on X11 is
// GLX. A texture name from one is meaningless in the other - Flutter ends up
// binding an unrelated object of its own that happens to carry the same
// number, which is why the whole UI showed up mirrored inside the background.
//
// Pixels have no such problem. The renderer reads its frame back into one of
// two buffers and says which one is ready; Flutter uploads it in whatever
// context it likes. Two buffers rather than one so the side writing the next
// frame never touches the one Flutter is reading.
G_DECLARE_FINAL_TYPE(
  FlAnglePixelTexture,
  fl_angle_pixel_texture,
  FL,
  ANGLE_PIXEL_TEXTURE,
  FlPixelBufferTexture
)

struct _FlAnglePixelTexture{
  FlPixelBufferTexture parent_instance;
  uint8_t *buffers[2];
  uint32_t width;
  uint32_t height;
  // Which buffer holds the finished frame. Written by the platform thread,
  // read by the raster thread.
  std::atomic<int> ready;
};

FlAnglePixelTexture *fl_angle_pixel_texture_new(uint32_t width, uint32_t height);

// Points the texture at freshly allocated buffers of a new size.
void fl_angle_pixel_texture_resize(
  FlAnglePixelTexture *self,
  uint32_t width,
  uint32_t height
);

// Marks buffer [index] as the one holding the finished frame.
void fl_angle_pixel_texture_set_ready(FlAnglePixelTexture *self, int index);

#endif // FL_ANGLE_PIXEL_TEXTURE_H
