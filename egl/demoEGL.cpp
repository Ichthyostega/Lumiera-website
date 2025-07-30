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
#include <string>
#include <set>

// for low-level access -> X-Window
#include <gdk/gdkx.h>

// X11 and GLX extension
#include <X11/Xlib.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>  // defines plattform-specific config and extensions
#include <GL/glew.h>     // provides system-specific bindings for OpenGL extensions
#include <GL/gl.h>

using std::string;


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
    uint vboID{0};
    uint vaoID{0};

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

  long SCREEN_SPEC[]
    {EGL_PLATFORM_X11_SCREEN_EXT, xScreen
    ,EGL_NONE};

  ctx.display = eglGetPlatformDisplay (EGL_PLATFORM_X11_EXT, xDisplay, SCREEN_SPEC);
  if (EGL_NO_DISPLAY == ctx.display
      or not eglInitialize(ctx.display, NULL,NULL))
    __FAIL ("could not establish EGL Display connection.");

  EGLint DESIRED_ATTRIBS[]
    {EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER
    ,EGL_RED_SIZE, 4
    ,EGL_GREEN_SIZE, 4
    ,EGL_BLUE_SIZE, 4
    ,EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT // config must support creating an OpenGL context
    ,EGL_SURFACE_TYPE, EGL_WINDOW_BIT    // want to create a window surface
    ,EGL_DEPTH_SIZE,   0                 // prefer config without depth buffer (occlusion testing not needed)
    ,EGL_STENCIL_SIZE, 0                 // prefer config without stencil buffer (no advanced visual effects)
    ,EGL_NONE};

  EGLint _cnt;
  EGLConfig config;
  if (not eglChooseConfig (ctx.display
                          ,DESIRED_ATTRIBS
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

  if (not eglBindAPI( EGL_OPENGL_API))
    __FAIL ("unable to configure EGL for usage with the OpenGL API.");

  EGLint CTX_ATTRIBS[]                 // Note: require modern OpenGL
    {EGL_CONTEXT_OPENGL_PROFILE_MASK,  EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT
//    ,EGL_CONTEXT_MAJOR_VERSION, 3
//    ,EGL_CONTEXT_MINOR_VERSION, 3
    ,EGL_NONE};

  ctx.egl = eglCreateContext (ctx.display, config
                             ,EGL_NO_CONTEXT   // do not share definitions and data with another context
                             ,CTX_ATTRIBS);
  if (not ctx.egl)
    __FAIL ("failed to create OpenGL context for this display with desired visuals");

  // create a binding for the current thread to use this context on this window
  if (not eglMakeCurrent (ctx.display, ctx.surface,ctx.surface, ctx.egl))
    __FAIL ("failed to attach an OpenGL context to the application X-Window");


  // use lib-GLEW to manage OpenGL extensions (notably GL Shader Language)
  GLenum glewErr = glewInit();
////////////////////////////////////////////////////////////////////////////////////TODO for some reason I get glewErr == 4 ("unknown error"), but it works non the less
//  if (GLEW_OK != glewErr)
//    __FAIL ("could not bind OpenGL extensions through lib-GLEW:\n"
//           +string{reinterpret_cast<const char*> (glewGetErrorString(glewErr))});


  auto compileShader = [](GLenum kind, string code)
                        {
                          uint shaderID = glCreateShader (kind);
                          const GLchar* codeSegment = code.c_str();
                          glShaderSource (shaderID, 1, &codeSegment, NULL);
                          glCompileShader(shaderID);
                          // diagnostics...
                          int success;
                          glGetShaderiv (shaderID, GL_COMPILE_STATUS, &success);
                          if (not success)
                            {
                              int logSiz{0};
                              glGetShaderiv (shaderID, GL_INFO_LOG_LENGTH, &logSiz);
                              string report(logSiz,'.');
                              glGetShaderInfoLog (shaderID, logSiz, NULL, & report[0]);
                              std::cout << "\n■□■□■□■□■----Shader-Code-------------------------\n"
                                        << code
                                        << "\n■□■□■□■□■----Failure-Report----------------------\n"
                                        << report.c_str()
                                        << std::endl;
                              __FAIL ("shader compilation failure");
                            }
                          return shaderID;
                        };

  auto checkLinkError = [](uint programID)
                        {
                          int success;
                          glGetProgramiv (programID, GL_LINK_STATUS, &success);
                          if (not success)
                            {
                              int logSiz{0};
                              glGetProgramiv (programID, GL_INFO_LOG_LENGTH, &logSiz);
                              string report(logSiz,'.');
                              glGetProgramInfoLog (programID, logSiz, NULL, & report[0]);
                              std::cout << "\n■□■□■□■□■----Failure-Report----------------------\n"
                                        << report.c_str()
                                        << std::endl;
                              __FAIL ("unable to link shader code");
                            }
                        };


  uint vertexShader
    = compileShader (GL_VERTEX_SHADER,
      R"-(#version 330 core

          layout(location = 0) in vec2 position;
          layout(location = 1) in vec2 texCoord;

          out vec2 fragTexCoord;

          void main() {
              gl_Position  = vec4 (position, 0.0, 1.0);
              fragTexCoord = texCoord;
          }
        )-");

  uint fragmentShader
    = compileShader (GL_FRAGMENT_SHADER,
      R"-(#version 330 core

          in  vec2 fragTexCoord;
          out vec4 colour;

          uniform sampler2D textureSampler;

          void main() {
              colour = texture (textureSampler, fragTexCoord);
          }
        )-");

  uint gpuProgram = glCreateProgram();
  glAttachShader (gpuProgram, vertexShader);
  glAttachShader (gpuProgram, fragmentShader);
  glLinkProgram  (gpuProgram);
  checkLinkError (gpuProgram);

  glUseProgram  (gpuProgram);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);


  /* === setup a 2D texture, to be mapped into the viewport === */
  glActiveTexture(GL_TEXTURE0);                                        // activate texture-unit #0 on the GPU
  glGenTextures  (1, &ctx.texID);                                      // allocate 1 new texture ID
  glBindTexture  (GL_TEXTURE_2D, ctx.texID);                           // use this textureID as "the" TEXTURE_2D
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    // configure image scaling filter
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // clamp, don't wrap at the texture edges
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // configure the `textureSampler` parameter (»uniform« is a global parameter visible in all shaders)
  GLint samplerParam = glGetUniformLocation(gpuProgram, "textureSampler");
  glUniform1i(samplerParam, 0);                                        // configure the shader to use texture-unit #0 of the GPU


  /* === setup a fixed geometry to hold that texture === */
  glDisable (GL_DEPTH_TEST);                                           // we do not need 3D layering / positioning
  glViewport (0, 0, ctx.VIDEO_WIDTH, ctx.VIDEO_HEIGHT);                // Origin in the middle of the window (note Y points upwards)

  //  Note: this demo uses a fixed-size window and hard-coded video size;
  //        a real-world implementation would have to place the video frame
  //        dynamically into the available screen space, possibly scaling up/down
  GLfloat sX{ctx.scaleX};
  GLfloat sY{ctx.scaleY};                                              // Note: positions use »normalised device coordinates« (NDC)
  GLfloat w{1};                                                        //       TEXTURE_2D points cover the range (0,0) ... (1,1)
  GLfloat h{1};

  GLfloat vertexData[]                                                 // Geometry and mapping data to be sent to the GPU...
    //-point-+--texture                                                // We define the 4 edges of a rectangular »projection screen«
    {-sX,-sY ,  0,h                                                    // For each edge, we define the position in NDC [-1 ... +1]
    , sX,-sY ,  w,h                                                    // and we specify which point in the texture to map there
    , sX, sY ,  w,0                                                    // Note: the rows of the video image run top-down, and thus we
    ,-sX, sY ,  0,0                                                    // have to map the texture flipped, since OpenGL orients Y upwards.
    };

  glGenBuffers     (1, &ctx.vboID);                                    // allocate 1 new vertex-buffer-object
  glGenVertexArrays(1, &ctx.vaoID);                                    // allocate 1 new vertex-array-object (to attach all definitions)

  glBindVertexArray(ctx.vaoID);                                        // activate the VAO to pick up the following definitions...
  glBindBuffer (GL_ARRAY_BUFFER, ctx.vboID);
  // copy the vertexData into the vertex-buffer-object currently bound to GL_ARRAY_BUFFER
  glBufferData (GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

  auto SIZ = sizeof(float);                                            // parameters are transferred to the GPU as float data
  auto attrib = 2;                                                     // we use 2 input attributes for the shader pipeline
  auto coords = 2;                                                     // our data vectors have 2 coordinates (2D)
  auto stride = attrib * coords * SIZ;                                 // stride is the offset in bytes between full data points
  auto offset = [&](uint elm){ return (void*) (elm * SIZ); };          // offset of one attribute relative to the start of a data point (packaged into a void*)
  auto normalise = GL_FALSE;                                           // do not auto-normalise, positions are already [-1 ... +1]

  // define how two two attributes are packed into the data sequence we send to the GPU
  glVertexAttribPointer (0, coords, GL_FLOAT, normalise, stride, offset(0) );
  glVertexAttribPointer (1, coords, GL_FLOAT, normalise, stride, offset(2) );
  glEnableVertexAttribArray (0);                                       // activate attribute #1 ≙ input argument `position` in vertexShader
  glEnableVertexAttribArray (1);                                       // activate attribute #2 ≙ input argument `texCoord` in vertexShader


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
  glTexImage2D (GL_TEXTURE_2D                                          // the target texture store to work on, here "the" TEXTURE_2D
               ,0                                                      // detail level (when using mipmap reduction, which we don't)
               ,GL_RGB                                                 // internal format or features to use for this texture
               ,ctx.VIDEO_WIDTH,ctx.VIDEO_HEIGHT, /*border*/ 0
               ,GL_RGB                                                 // data layout of the provided pixels
               ,GL_UNSIGNED_BYTE                                       // data format / size of the pixels
               ,buffer
               );

  // Draw the quadrilateral
  glDrawArrays(GL_QUADS, 0, 4);

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
