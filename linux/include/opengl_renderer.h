
#ifndef OPENGLRENDERER_H
#define OPENGLRENDERER_H

#include <flutter_linux/flutter_linux.h>
#include <flutter_linux/fl_texture_registrar.h>
#include <memory>
#include <map>

class OpenglRenderer {
  public:
    uint32_t width = 0;
    uint32_t height = 0;
    int64_t textureId = 0;

    virtual ~OpenglRenderer();
    OpenglRenderer(OpenglRenderer&& other) noexcept; 
    OpenglRenderer& operator=(OpenglRenderer&& other) noexcept; 
    OpenglRenderer(FlTextureRegistrar*,GdkGLContext*,int,int);
    FlValue *createTexture();
    // [index] says which of the two pixel buffers holds the finished frame.
    void updateTexture(int index);
    void changeSize(int, int);
    void dispose(bool);

    template<class T, class U = T>
    T exchange(T& obj, U&& new_value){
      T old_value = std::move(obj);
      obj = std::forward<U>(new_value);
      return old_value;
    }

  private:
    GdkGLContext* context;
    FlTextureRegistrar *textureRegistrar;
    FlTexture *texture = nullptr;

    // Private method to swap members, helpful for both move constructor and assignment
    void swap(OpenglRenderer& other) noexcept;
};

typedef std::map<int64_t, std::unique_ptr<OpenglRenderer>> RendererMap;

class Map{
  public:
    RendererMap renderers;
};

#endif