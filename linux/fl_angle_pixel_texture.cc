#include "include/fl_angle_pixel_texture.h"

#include <cstdlib>
#include <cstring>

G_DEFINE_TYPE(
  FlAnglePixelTexture,
  fl_angle_pixel_texture,
  fl_pixel_buffer_texture_get_type()
)

static void free_buffers(FlAnglePixelTexture *self){
  for(int i = 0; i < 2; i++){
    free(self->buffers[i]);
    self->buffers[i] = nullptr;
  }
}

static void allocate_buffers(FlAnglePixelTexture *self, uint32_t width, uint32_t height){
  size_t size = (size_t)width * (size_t)height * 4;
  for(int i = 0; i < 2; i++){
    self->buffers[i] = (uint8_t*)calloc(1, size);
  }
  self->width = width;
  self->height = height;
  self->ready.store(0);
}

// Called on Flutter's raster thread.
static gboolean fl_angle_pixel_texture_copy_pixels(
  FlPixelBufferTexture *texture,
  const uint8_t **out_buffer,
  uint32_t *width,
  uint32_t *height,
  GError **error
){
  FlAnglePixelTexture *self = FL_ANGLE_PIXEL_TEXTURE(texture);
  uint8_t *buffer = self->buffers[self->ready.load()];
  if(buffer == nullptr){
    g_set_error(error, g_quark_from_static_string("flutter_angle"), 0,
                "No pixel buffer to show yet.");
    return FALSE;
  }
  *out_buffer = buffer;
  *width = self->width;
  *height = self->height;
  return TRUE;
}

FlAnglePixelTexture *fl_angle_pixel_texture_new(uint32_t width, uint32_t height){
  auto self = FL_ANGLE_PIXEL_TEXTURE(
    g_object_new(fl_angle_pixel_texture_get_type(), nullptr));
  allocate_buffers(self, width, height);
  return self;
}

void fl_angle_pixel_texture_resize(FlAnglePixelTexture *self, uint32_t width, uint32_t height){
  free_buffers(self);
  allocate_buffers(self, width, height);
}

void fl_angle_pixel_texture_set_ready(FlAnglePixelTexture *self, int index){
  self->ready.store(index == 0?0:1);
}

static void fl_angle_pixel_texture_dispose(GObject *object){
  free_buffers(FL_ANGLE_PIXEL_TEXTURE(object));
  G_OBJECT_CLASS(fl_angle_pixel_texture_parent_class)->dispose(object);
}

static void fl_angle_pixel_texture_class_init(FlAnglePixelTextureClass *klass){
  FL_PIXEL_BUFFER_TEXTURE_CLASS(klass)->copy_pixels = fl_angle_pixel_texture_copy_pixels;
  G_OBJECT_CLASS(klass)->dispose = fl_angle_pixel_texture_dispose;
}

static void fl_angle_pixel_texture_init(FlAnglePixelTexture *self){
  self->buffers[0] = nullptr;
  self->buffers[1] = nullptr;
  self->width = 0;
  self->height = 0;
  self->ready.store(0);
}
