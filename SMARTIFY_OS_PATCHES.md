# flutter_angle, patched for SmartifyOS

A copy of [flutter_angle](https://github.com/Knightro63/flutter_angle) 0.3.9 with
its **Linux** backend rewritten to hand Flutter pixels instead of a GL texture.
Only `linux/` and a Linux-only branch in `lib/desktop/angle.dart` differ, so
macOS, Windows and Android run the stock code untouched.

`three_js` draws through this package. Without this, the 3D car background on
Linux shows the whole UI mirrored back at itself at shrinking scale, or
artifacts, or nothing – on x86 and ARM alike.

Pulled in by `dependency_overrides` in both `smartify_os_core/pubspec.yaml` and
`smartify_os_miata_app/pubspec_overrides.yaml`. Both are needed – an override
only applies to the package it is written in.

## Why the GL texture route cannot work

`FlTextureGL` hands Flutter the *name* of an OpenGL texture. Names only mean
anything inside the context that created them, or one sharing with it.

The [documentation](https://api.flutter.dev/linux-embedder/struct___fl_texture_g_l_class.html)
still tells plugins to create a `GdkGLContext` from the `FlView`'s window and
promises "the context will be shared with the one used by Flutter". That is no
longer true. In Flutter 3.41 the engine builds its own contexts in
`fl_opengl_manager.cc` with plain EGL:

```c
self->render_context = eglCreateContext(self->display, config, EGL_NO_CONTEXT, ...);
```

`EGL_NO_CONTEXT` – it shares with nothing. Meanwhile `gdk_window_create_gl_context`
on X11 returns a **GLX** context, a different API entirely. There is no public
way for a plugin to reach the engine's context; `fl_engine_get_opengl_manager`
lives in a private header.

Measured on the test laptop, which is what settled it:

```
plugin side:   texture 14 created,  eglContext=0                 <- GLX, no EGL context
Flutter side:  populate name=14,    eglContext=0x608c19c00400    <- engine's EGL context
               existsInFlutterContext=1
```

Both contexts happened to have a texture numbered 14, so Flutter dutifully drew
*its own* number 14 – one of its render targets – which is why the interface
appeared nested inside its own background, live and in sync. Filling the
plugin's texture with solid magenta and seeing no magenta on screen confirmed
Flutter never reads it.

## What it does instead

`FlPixelBufferTexture`. The frame crosses as ordinary memory, so no context has
to share with any other. This is the same fallback the engine itself uses for
its own frames on X11 – see the comment in `fl_view.cc`'s `setup_opengl`.

- `linux/fl_angle_pixel_texture.cc` – an `FlPixelBufferTexture` holding **two**
  buffers and an atomic index saying which one is complete, so the side writing
  the next frame never touches the one Flutter is reading.
- `linux/opengl_renderer.cc` – allocates those buffers, registers the texture
  once, and hands their addresses to Dart. It makes no GL calls at all now,
  which also removes the old bug of calling `glGenTextures` from the platform
  thread with no context current.
- `lib/desktop/angle.dart` – after rendering, binds the FBO explicitly and
  `glReadPixels` straight into the buffer Flutter is not reading, then names
  that buffer in the `textureFrameAvailable` call. Binding explicitly matters:
  the renderer does its own framebuffer work for shadow passes and does not
  leave the right one bound. Dart also allocates the colour renderbuffer now,
  since the plugin no longer owns one.

Dart still renders through the `GdkGLContext` the plugin creates. That part was
always fine – the scene was drawing correctly the whole time, into a texture
nobody could see.

## Cost

One `glReadPixels` per frame, 1920x1080x4 = 8.3 MB on the test laptop, plus
Flutter's upload. If a slower board struggles, render the background smaller:
`CarOrbitBackground` fixes its render size once at startup and Flutter scales
the result, so dropping it to 1280x720 is a one-line change with no visible
loss on a slowly orbiting background.

## Other fixes carried in the same patch

- `changeSize` reused the registered `FlTexture` instead of registering a second
  one and returning the first one's id, which permanently orphaned the texture
  on screen after any window resize.
- `dispose` no longer calls `gdk_gl_context_clear_current()` and `g_object_unref()`
  on the context the plugin owns and shares, which used to take it away from
  Flutter's compositor on the way out (`Failed to cleanup compositor shaders`).

## Getting rid of this fork

These are upstream problems, not SmartifyOS ones, and worth a pull request to
[Knightro63/flutter_angle](https://github.com/Knightro63/flutter_angle) some day.
Note upstream is a monorepo with the package under `flutter_angle/`, so a real
fork of it would need `path: flutter_angle` in the dependency; this repository
is a plain copy with the package at its root, which avoids that.

Once an upstream release carries these fixes, drop the two
`dependency_overrides` entries in SmartifyOS and this repository can go.
