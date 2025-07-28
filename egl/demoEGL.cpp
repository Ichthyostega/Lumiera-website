/*
  demoEGL.cpp  -  output video from a GTK application, using EGL - OpenGL binding

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/


#include "commons.hpp"
#include "gtk-egl-app.hpp"
#include "image-generator.hpp"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <set>

// for low-level access -> X-Window
#include <gdk/gdkx.h>

// X11 and GLX extension
#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>  // defines plattform-specific config and extensions
#include <GL/glx.h> //////////////////////TODO



/**
 * Connection and drawing context used for EGL based video display.
 */
struct EglCtx
  {
    /** EGL / X11 connection. */
    EGLDisplay display{nullptr};
    EGLSurface surface{nullptr};

    EGLContext egl{nullptr};

    uint texID{0};
    float scaleX{1};
    float scaleY{1};

    // hard wired here (should be configurable in real-world usage)
    constexpr static uint VIDEO_WIDTH {320};
    constexpr static uint VIDEO_HEIGHT{240};

    using ImgGen = ImageGenerator<VIDEO_WIDTH,VIDEO_HEIGHT>;
    ImgGen imgGen_;

    EglCtx(FrameRate fps)
      : imgGen_{fps}
      { }
  };




EglCtx
openDisplay (Gtk::Window& appWindow, FrameRate fps)
{
  std::cout << "Open display-connection through EGL..." << std::endl;

  EglCtx ctx{fps};

  // use the X-Window as anchor to build an OpenGL context via EGL
  Glib::RefPtr<Gdk::Window> gdkWindow = appWindow.get_window();
  Window   xWindow  = GDK_WINDOW_XID      (gdkWindow->gobj());
  Display* xDisplay = GDK_WINDOW_XDISPLAY (gdkWindow->gobj());

  long xScreen;
  XWindowAttributes xWinAttrs;
  if (XGetWindowAttributes (xDisplay, xWindow, &xWinAttrs))
    xScreen = XScreenNumberOfScreen (xWinAttrs.screen);
  else
    __FAIL ("unable to retrieve screen number from the X11 window attributes.");

  auto SCREEN_SPEC
    = asArray (EGL_PLATFORM_X11_SCREEN_EXT, xScreen
              ,EGL_NONE);

  ctx.display = eglGetPlatformDisplay (EGL_PLATFORM_X11_EXT, xDisplay, SCREEN_SPEC.data());
  if (EGL_NO_DISPLAY == ctx.display
      or not eglInitialize(ctx.display, NULL,NULL))
    __FAIL ("could not establish EGL Display connection.");

  auto DESIRED_ATTRIBS
    = asArray (EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER
              ,EGL_RED_SIZE, 4
              ,EGL_GREEN_SIZE, 4
              ,EGL_BLUE_SIZE, 4
              ,EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT // config must support creating an OpenGL context
              ,EGL_SURFACE_TYPE, EGL_WINDOW_BIT    // want to create a window surface
              ,EGL_DEPTH_SIZE,   0                 // prefer config without depth buffer (occlusion testing not needed)
              ,EGL_STENCIL_SIZE, 0                 // prefer config without stencil buffer (no advanced visual effects)
              ,EGL_NONE);



  EGLint _cnt;
  EGLConfig config;
  if (not eglChooseConfig (ctx.display
                          ,DESIRED_ATTRIBS.data()
                          ,& config
                          ,1,&_cnt)
        or not config)
    __FAIL ("unable to select a EGL display config with the required attributes");


  ctx.surface = eglCreateWindowSurface (ctx.display,config,xWindow,nullptr);
  if (not ctx.surface)
    switch (eglGetError())
      {
        case EGL_BAD_CONFIG:
          __FAIL ("selected display configuration is not valid");
          break;
        case EGL_BAD_ALLOC:
          __FAIL ("unable to allocate resources or collision with existing resources");
          break;
        case EGL_BAD_MATCH:
          __FAIL ("mismatch between X11 window visuals and the capabilities of the selected config");
          break;
        default:
          __FAIL ("unable to create EGL window surface, for unknown reasons");
        break;
      }

  auto querySurf = [&](EGLint attr)
                {
                  EGLint val;
                  if (not eglQuerySurface (ctx.display,ctx.surface, attr, &val))
                    __FAIL ("Tilt");
                  return val;
                };
  auto val = querySurf (EGL_CONFIG_ID);
  std::cout << "Config("<<val<<"):"  <<std::endl;
  val = querySurf (EGL_GL_COLORSPACE);
  std::cout << "EGL_GL_COLORSPACE: "  << (val == EGL_GL_COLORSPACE_SRGB? "EGL_GL_COLORSPACE_SRGB":"EGL_GL_COLORSPACE_LINEAR")<<std::endl;
  val = querySurf (EGL_TEXTURE_FORMAT);
  std::cout << "EGL_TEXTURE_FORMAT: " << (val == EGL_NO_TEXTURE? "EGL_NO_TEXTURE":(val== EGL_TEXTURE_RGB?"EGL_TEXTURE_RGB":"EGL_TEXTURE_RGBA"))<<std::endl;
  val = querySurf (EGL_RENDER_BUFFER);
  std::cout << "EGL_RENDER_BUFFER: "  << (val == EGL_BACK_BUFFER? "client renders to back-buffer":"client renders directly to visible display")<<std::endl;
  val = querySurf (EGL_SWAP_BEHAVIOR);
  std::cout << "EGL_SWAP_BEHAVIOR: "  << (val == EGL_BUFFER_PRESERVED? "contents preserved on buffer-swap":"buffer or contents destroyed on buffer-swap")<<std::endl;
  val = querySurf (EGL_GL_COLORSPACE);
  std::cout << "EGL_GL_COLORSPACE: "  << (val == EGL_GL_COLORSPACE_SRGB? "EGL_GL_COLORSPACE_SRGB":"EGL_GL_COLORSPACE_LINEAR")<<std::endl;
  auto queryConf = [&](EGLint attr)
                {
                  EGLint val;
                  if (not eglGetConfigAttrib (ctx.display,config, attr, &val))
                    __FAIL ("Tilt");
                  return val;
                };
  val = queryConf (EGL_BUFFER_SIZE);
  std::cout << "Color depth total: " << val <<" bit"<<std::endl;
  std::cout << "Color depth ....l: " << queryConf (EGL_RED_SIZE) <<"R bit "
                                     << queryConf (EGL_GREEN_SIZE) <<"G bit "
                                     << queryConf (EGL_BLUE_SIZE) <<"B bit "
                                     << queryConf (EGL_ALPHA_SIZE) <<"α bit "<<std::endl;
  val = queryConf (EGL_DEPTH_SIZE);
  std::cout << "Depth buffer    l: " << val <<" bit"<<std::endl;
  
  
  if (not eglBindAPI( EGL_OPENGL_API))
    __FAIL ("unable to configure EGL for usage with the OpenGL API.");

  auto CTX_ATTRIBS
    = std::array{EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
                ,EGL_NONE};

  ctx.egl = eglCreateContext (ctx.display, config
                             ,EGL_NO_CONTEXT   // do not share definitions and data with another context
                             ,CTX_ATTRIBS.data());
  if (not ctx.egl)
    __FAIL ("failed to create OpenGL context for this display with desired visuals");

  // create a binding for the current thread to use this context on this window
  if (not eglMakeCurrent (ctx.display, ctx.surface,ctx.surface, ctx.egl))
    __FAIL ("failed to attach an OpenGL context to the application X-Window");

  glDisable (GL_DEPTH_TEST);                   // we do not need 3D layering / positioning
  glEnable (GL_TEXTURE_RECTANGLE_ARB);         // allow texture size to be *not* a power of two

  // setup a 2D texture, to be mapped into the viewport
  glGenTextures (1, &ctx.texID);                                       // allocate 1 new texture ID
  glBindTexture (GL_TEXTURE_RECTANGLE_ARB, ctx.texID);                 // use this textureID as "the" RECTANGLE_ARB texture
  glTexEnvi     (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);       // disable blending (≙transparency), "decal" means just to paint opaque
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    // configure image scaling filter
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // clamp, don't wrap at the texture edges
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // setup coordinate system
  glViewport (0, 0, ctx.VIDEO_WIDTH, ctx.VIDEO_HEIGHT);                // Origin in the middle of the window (note Y points upwards)
  glMatrixMode (GL_PROJECTION);                                        // the following matrix commands affect the image projection matrix
  glLoadIdentity();
  glOrtho (-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);                     // setup orthograpic (non-perspective) projection within standard coordinates


   // hand-over the activated connection context
  //  to be managed by the GTK application...
  std::cout << "Started playback at "<<fps<<" frames/sec." << std::endl;
  return ctx;
}


void
displayFrame (EglCtx& ctx)
{
  uint frameNr = ctx.imgGen_.getFrameNr();
  uint fps     = ctx.imgGen_.getFps();
  if (0 == frameNr % fps)
    std::cout << "tick ... " << ctx.imgGen_.getFrameNr() << std::endl;

  // compute a buffer with RGB data and bind it into the prepared texture...
  const void* buffer = ctx.imgGen_.buildNext().data();
  glTexImage2D (GL_TEXTURE_RECTANGLE_ARB                               // the target texture store to work on, here "the" RECTANGLE_ARB
               ,0                                                      // detail level (when using mipmap reduction, which we don't)
               ,GL_RGB                                                 // internal format or features to use for this texture
               ,ctx.VIDEO_WIDTH,ctx.VIDEO_HEIGHT, /*border*/ 0
               ,GL_RGB                                                 // data layout of the provided pixels
               ,GL_UNSIGNED_BYTE                                       // data format / size of the pixels
               ,buffer
               );

  //  Note: this demo uses a fixed-size window and hard-coded video size;
  //        a real-world implementation would have to place the video frame
  //        dynamically into the available screen space, possibly scaling up/down
  GLfloat w{ctx.VIDEO_WIDTH};
  GLfloat h{ctx.VIDEO_HEIGHT};
  GLfloat sX{ctx.scaleX};
  GLfloat sY{ctx.scaleY};

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // draw a quatrilateral which exactly fills the viewport
  // each vertex is also supplied with a texture mapping point
  glBegin(GL_QUADS);
  glTexCoord2f (0, h);
  glVertex2f (-sX,-sY);

  glTexCoord2f (w, h);
  glVertex2f ( sX,-sY);

  glTexCoord2f (w, 0);
  glVertex2f ( sX, sY);

  glTexCoord2f (0, 0);
  glVertex2f (-sX, sY);
  glEnd();

  // double-buffer flip, automatically invokes glFlush()
  eglSwapBuffers (ctx.display, ctx.surface);
}


void
cleanUp (EglCtx& ctx)
{
  std::cout << "STOP " << ctx.imgGen_.getFrameNr() << " frames displayed." << std::endl;

  // detach binding with OpenGL context
  eglMakeCurrent (ctx.display,  EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface (ctx.display, ctx.surface);
  eglDestroyContext (ctx.display, ctx.egl);
  eglTerminate(ctx.display); // detach EGL from display
}



int
main (int, const char*[])
{
    return GtkEglApp<EglCtx>{"demo.egl"}
            .onStart (openDisplay)
            .onFrame (displayFrame)
            .onClose (cleanUp)
            .run(30);
}
