/*
  demoGLX.cpp  -  output video from a GTK application, using GLX - OpenGL binding

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/


#include "commons.hpp"
#include "gtk-glx-app.hpp"
#include "image-generator.hpp"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <set>

// for low-level access -> X-Window
#include <gdk/gdkx.h>

// X11 and GLX extension
#include <X11/Xlib.h>
#include <GL/glx.h>



/**
 * Connection and drawing context used for GLX based video display.
 */
struct GlxCtx
  {
    /** X11 connection. */
    Display* display{nullptr};
    Window window{0};
    int screen{0};

    GLXContext glx{nullptr};
    uint texID{0};
    float scaleX{1};
    float scaleY{1};

    // hard wired here (should be configurable in real-world usage)
    constexpr static uint VIDEO_WIDTH {320};
    constexpr static uint VIDEO_HEIGHT{240};

    using ImgGen = ImageGenerator<VIDEO_WIDTH,VIDEO_HEIGHT>;
    ImgGen imgGen_;

    GlxCtx(FrameRate fps)
      : imgGen_{fps}
      { }
  };




GlxCtx
openDisplay (Gtk::Window& appWindow, FrameRate fps)
{
  std::cout << "Open GLX display-connection..." << std::endl;

  GlxCtx ctx{fps};

  // use the X-Window as anchor to build an OpenGL context via GLX
  Glib::RefPtr<Gdk::Window> gdkWindow = appWindow.get_window();
  ctx.window  = GDK_WINDOW_XID      (gdkWindow->gobj());
  ctx.display = GDK_WINDOW_XDISPLAY (gdkWindow->gobj());
  ctx.screen  = DefaultScreen (ctx.display);


  int DESIRED_ATTRIBS[]
    {GLX_RGBA            // require true-colour, not palette-colour
    ,GLX_DOUBLEBUFFER    // want hardware backed double-buffering
    ,GLX_RED_SIZE,  4    // need at minimum support for 12 bit per pixel
    ,GLX_GREEN_SIZE,4
    ,GLX_BLUE_SIZE, 4
    ,0};

  XVisualInfo* visual = glXChooseVisual (ctx.display, ctx.screen, DESIRED_ATTRIBS);
  if (not visual)
    __FAIL ("unable to connect to OpenGL visual with desired attributes");

  ctx.glx = glXCreateContext (ctx.display, visual
                             ,nullptr          // do not share display list definitions
                             ,true             // prefer direct rendering if possible
                             );
  XFree (visual);
  if (not ctx.glx)
    __FAIL ("failed to create OpenGL context for this display with desired visuals");

  // create a binding for the current thread to use this context on this window
  if (not glXMakeCurrent (ctx.display, ctx.window, ctx.glx))
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
displayFrame (GlxCtx& ctx)
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

  // draw a quadrilateral which exactly fills the viewport
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
  glXSwapBuffers (ctx.display, ctx.window);
}


void
cleanUp (GlxCtx& ctx)
{
  std::cout << "STOP " << ctx.imgGen_.getFrameNr() << " frames displayed." << std::endl;

  // detach binding with OpenGL context
  glXMakeCurrent (ctx.display, None, nullptr);
  if (ctx.glx)
    glXDestroyContext (ctx.display, ctx.glx);
}



int
main (int, const char*[])
{
    return GtkGlxApp<GlxCtx>{"demo.glx"}
            .onStart (openDisplay)
            .onFrame (displayFrame)
            .onClose (cleanUp)
            .run(30);
}
