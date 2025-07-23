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
 * Sequence of letters of a "fourCC" format ID,
 * packaged numerically into a single 32-bit int.
 * @param id the human-readable 4-character literal string of the fourCC
 * @return ASCII values of these characters packaged in little-endian order.
 */
constexpr int
fourCC (const char id[5])
{
  uint32_t code{0};
  for (uint c=0; c<4; ++c)
      code |= uint(id[c]) << c*8;
  return code;
}

const std::set<int> SUPPORTED_FORMATS = {fourCC("I420")    ///////TODO implement
                                        ,fourCC("YV12")    ///////TODO implement
                                        ,fourCC("YUY2")
                                        ,fourCC("UYVY")    ///////TODO implement
                                        };



/**
 * Output connection context used for opening X-Video display.
 */
struct GlxCtx
  {
    /** X11 connection. */
    Display* display{nullptr};
    Window window{0};
    uint port{0};
    GC gc{nullptr};

    int format{0};

    // hard wired here (should be configurable in real-world usage)
    constexpr static uint VIDEO_WIDTH {320};
    constexpr static uint VIDEO_HEIGHT{240};


    using ImgGen = ImageGenerator<VIDEO_WIDTH,VIDEO_HEIGHT>;
    ImgGen imgGen_;

    GlxCtx(FrameRate fps)
      : imgGen_{fps}
      { }
  };


namespace { // implementation details : pixel format conversion

  using std::clamp;

  using ImgGen    = GlxCtx::ImgGen;
  using PackedRGB = ImgGen::PackedRGB;


  /** slightly simplified conversion from RGB components to Y'CbCr with Rec.601 (MPEG style) */
  inline Trip
  rgb_to_yuv (Trip const& rgb)
  {
    auto r = int(rgb[0]);
    auto g = int(rgb[1]);
    auto b = int(rgb[2]);
    Trip yuv;
    auto& [y,u,v] = yuv;
    y = byte(clamp (  0 + (    299 * r +    587 * g +    114 * b) /    1000, 16,235));   // Luma clamped to MPEG scan range
    u = byte(clamp (128 + (-168736 * r - 331264 * g + 500000 * b) / 1000000, 0, 255));   // Chroma components mapped according to Rec.601
    v = byte(clamp (128 + ( 500000 * r - 418688 * g -  81312 * b) / 1000000, 0, 255));   // (but with integer arithmetics and truncating)
    return yuv;
  }


  void
  rgb_buffer_to_yuy2 (PackedRGB const& in, byte* out)
  {
    uint cntPix = in.size();
    assert (cntPix %2 == 0);
    for (uint i = 0; i < cntPix; i += 2)
      {// convert and interleave 2 pixels in one step
        uint op = i * 2;                           // Output packed in groups with 2 bytes
        Trip const& rgb0 = in[i];
        Trip const& rgb1 = in[i+1];
        Trip yuv0 = rgb_to_yuv (rgb0);
        Trip yuv1 = rgb_to_yuv (rgb1);

        auto& [y0,u0,v0] = yuv0;
        auto& [y1,_u,_v] = yuv1;                   // note: this format discards half of the chroma information

        out[op    ] = y0;
        out[op + 1] = u0;
        out[op + 2] = y1;
        out[op + 3] = v0;
  }   }


} // (End) implementation details


void
convert_RGB_intoBuffer (int format, char* targetBuff, int targetSiz
                       ,PackedRGB const& inputFrame)
{
  static_assert (sizeof(byte) == sizeof(char));
  byte* outputData = reinterpret_cast<byte*> (targetBuff);

  if (format == fourCC("YUY2"))
    {
      // this format discards 1/3 of the information
      // input comes in RGB triplets, output discards 50% chroma
      assert (targetSiz == 2 * inputFrame.size());
      rgb_buffer_to_yuy2 (inputFrame, outputData);
    }
  else
    __FAIL ("Logic broken: unsupported output target format");
}



GlxCtx
openDisplay (Gtk::Window& appWindow, FrameRate fps)
{
  std::cout << "Open GLX display-connection..." << std::endl;

  Glib::RefPtr<Gdk::Window> gdkWindow = appWindow.get_window();

  GlxCtx ctx{fps};
  ctx.window  = GDK_WINDOW_XID      (gdkWindow->gobj());
  ctx.display = GDK_WINDOW_XDISPLAY (gdkWindow->gobj());

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

  int org_x = 0
    , org_y = 0
    , destW = ctx.VIDEO_WIDTH
    , destH = ctx.VIDEO_HEIGHT;
  //  Note: this demo uses a fixed-size window and hard-coded video size;
  //        a real-world implementation would have to place the video frame
  //        dynamically into the available screen space, possibly scaling up/down

//  convert_RGB_intoBuffer (ctx.format
//                         ,ctx.xvImage->data
//                         ,ctx.xvImage->data_size
//                         ,ctx.imgGen_.buildNext()
//                         );

}


void
cleanUp (GlxCtx& ctx)
{
  std::cout << "STOP " << ctx.imgGen_.getFrameNr() << " frames displayed." << std::endl;

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
