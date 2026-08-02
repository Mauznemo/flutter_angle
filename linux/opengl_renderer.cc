#include "include/opengl_renderer.h"
#include "include/fl_angle_pixel_texture.h"
#include <flutter_linux/fl_texture_registrar.h>
#include <iostream>

OpenglRenderer::OpenglRenderer(
  FlTextureRegistrar* textureRegistrar,
  GdkGLContext* context,
  int width,
  int height
){
  this->textureRegistrar = textureRegistrar;
  this->context = context;
  this->width = width;
  this->height = height;

  changeSize(width, height);
}

FlValue *OpenglRenderer::createTexture() {
  auto pixels = FL_ANGLE_PIXEL_TEXTURE(texture);
  g_autoptr(FlValue) value = fl_value_new_map ();
  fl_value_set_string_take(value, "textureId", fl_value_new_int(textureId));
  // The two buffers the renderer reads its frames back into. Handed over as
  // plain addresses: the Dart side writes straight into them, so a frame never
  // travels over the method channel.
  fl_value_set_string_take(value, "buffer0", fl_value_new_int((int64_t)pixels->buffers[0]));
  fl_value_set_string_take(value, "buffer1", fl_value_new_int((int64_t)pixels->buffers[1]));
  return fl_value_ref(value);
}

void OpenglRenderer::changeSize(int width, int height) {
  this->width = width;
  this->height = height;

  if(texture == nullptr){
    texture = FL_TEXTURE(fl_angle_pixel_texture_new(width, height));
    fl_texture_registrar_register_texture(textureRegistrar, texture);
    textureId = fl_texture_get_id(texture);
  }
  else{
    // Reuse the registered texture. Registering a replacement would leave the
    // Dart side holding the first id, which nothing draws into any more.
    fl_angle_pixel_texture_resize(FL_ANGLE_PIXEL_TEXTURE(texture), width, height);
  }

  std::cerr << "flutter_angle: pixel texture " << textureId << " at "
            << width << "x" << height << std::endl;
}

void OpenglRenderer::updateTexture(int index) {
  if(texture == nullptr) return;
  fl_angle_pixel_texture_set_ready(FL_ANGLE_PIXEL_TEXTURE(texture), index);
  fl_texture_registrar_mark_texture_frame_available(textureRegistrar, texture);
}

void OpenglRenderer::dispose(bool release_context) {
  if(release_context && texture != nullptr){
    fl_texture_registrar_unregister_texture(textureRegistrar, texture);
    texture = nullptr;
    textureId = 0;
    // The context belongs to the plugin and is shared by every renderer, so it
    // is not this one's to clear or unref.
  }
}

OpenglRenderer::~OpenglRenderer() {
  //dispose(true);
}

// Move constructor definition
OpenglRenderer::OpenglRenderer(OpenglRenderer&& other) noexcept:
  width(exchange(other.width, 0)),
  height(exchange(other.height, 0)),
  textureId(exchange(other.textureId, 0)),
  context(exchange(other.context, nullptr)),
  textureRegistrar(exchange(other.textureRegistrar, nullptr)),
  texture(exchange(other.texture, nullptr)
){
}


// Move assignment operator definition
OpenglRenderer& OpenglRenderer::operator=(OpenglRenderer&& other) noexcept {
  if (this != &other) { // Handle self-assignment
    // Release existing resources before stealing from other
    this->~OpenglRenderer(); // Call destructor to release resources

    // Transfer resources
    textureRegistrar = exchange(other.textureRegistrar, nullptr);
    context = exchange(other.context, nullptr);
    width = exchange(other.width, 0);
    height = exchange(other.height, 0);
    textureId = exchange(other.textureId, 0);
    texture = exchange(other.texture, nullptr);
  }
  return *this;
}

// Helper swap function (useful for copy-and-swap idiom for copy assignment, though not strictly needed here)
void OpenglRenderer::swap(OpenglRenderer& other) noexcept {
  using std::swap;
  swap(textureRegistrar, other.textureRegistrar);
  swap(context, other.context);
  swap(width, other.width);
  swap(height, other.height);
  swap(textureId, other.textureId);
  swap(texture, other.texture);
}
