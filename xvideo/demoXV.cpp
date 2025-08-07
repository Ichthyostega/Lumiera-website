/*
  demoXV.cpp  -  output video from a GTK application, using the X-Video standard

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU GPL version 2+ See the LICENSE file for details.

* ************************************************************************/


#include "commons.hpp"
#include "gtk-xv-app.hpp"
#include "image-generator.hpp"

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>
#include <set>

// for low-level access -> X-Window
#include <gdk/gdkx.h>

// X11 and XVideo extension
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

using std::string;



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

/** display fourCC code in human readable form */
string
fourCCstring (int fcc)
{
  string id{"????"};
  for (uint c=0; c<4; ++c)
    id[c] = 0xFF & (fcc >> c*8);
  return id;
}


const std::set<int> SUPPORTED_FORMATS = {fourCC("I420")
                                        ,fourCC("YV12")
                                        ,fourCC("YUY2")
                                        ,fourCC("UYVY")
                                        ,fourCC("YVYU")
                                       };

/**
 * Output connection context used for opening X-Video display.
 */
struct XvCtx
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

    /** shared memory image descriptor for the video output */
    XvImage* xvImage{nullptr};

    /** descriptor of the shared memory segment used for data exchange */
    XShmSegmentInfo shmInfo;

    using ImgGen = ImageGenerator<VIDEO_WIDTH,VIDEO_HEIGHT>;
    ImgGen imgGen_;

    XvCtx(FrameRate fps)
      : imgGen_{fps}
      { }
  };


namespace { // implementation details : pixel format conversion

  using std::clamp;

  using ImgGen    = XvCtx::ImgGen;
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


  /**
   * These 4:2:2 _packed formats_ have in common that they process two consecutive pixels,
   * while outputting only the chroma information from the first pixel; they differ in
   * the actual arrangement of the four output elements into the packed data sequence.
   */
  void
  rgb_buffer_to_packed (int format, PackedRGB const& in, byte* out)
  {
    assert (format == fourCC ("YUY2") or
            format == fourCC ("UYVY") or
            format == fourCC ("YVYU"));
    uint cntPix = in.size();
    assert (cntPix % 2 == 0);

    for (uint i = 0; i < cntPix; i += 2)
      {// convert and interleave 2 pixels in one step
        uint op = i * 2;                           // Output packed in groups with 2 bytes
        Trip const& rgb0 = in[i];
        Trip const& rgb1 = in[i+1];
        Trip yuv0 = rgb_to_yuv (rgb0);
        Trip yuv1 = rgb_to_yuv (rgb1);

        auto& [y0,u0,v0] = yuv0;
        auto& [y1,_u,_v] = yuv1;                   // note: these formats discard half of the chroma information

        switch (format) {
          case fourCC ("YUY2"):
            out[op    ] = y0;
            out[op + 1] = u0;
            out[op + 2] = y1;
            out[op + 3] = v0;
            break;
          case fourCC ("UYVY"):
            out[op    ] = u0;
            out[op + 1] = y0;
            out[op + 2] = v0;
            out[op + 3] = y1;
            break;
          case fourCC ("YVYU"):
            out[op    ] = y0;
            out[op + 1] = v0;
            out[op + 2] = y1;
            out[op + 3] = u0;
            break;
          default:
            __FAIL ("Logic broken");
            break;
        }
      }
  }


  /**
   * These 4:2:0 _planar formats_ place all Luma information into one data block,
   * followed by two chroma data blocks with 1/4 size each.
   * Every 2 x 2 Block of pixels shares a single Chroma sample:
   * - 2 adjacent horizontal Y components on the same line share a UV Chroma
   *   => store the UV component for all even columns and skip odd columns
   * - 2 stacked adjacent vertical Y components, one above the other,
   *   on consecutive lines share a single Chroma sample
   *   => do not store any UV data on every second line of pixels
   */
  void
  rgb_buffer_to_planar (int format, PackedRGB const& in, byte* out, uint width, uint height)
    {
      assert (format == fourCC("I420") or
              format == fourCC("YV12"));
      uint cntPix = in.size ();
      assert (cntPix % 4 == 0);
      assert (in.size() == width*height);

      byte* outU;
      byte* outV;
      if (format == fourCC("I420"))
        {
          outU = out + cntPix;
          outV = outU + cntPix /4;
        }
      else
      if (format == fourCC("YV12"))
        {
          outV = out + cntPix;
          outU = outV + cntPix /4;
        }
      else
        __FAIL ("Developer wake up");

      for (uint yIdx = 0, uvIdx = 0; yIdx < cntPix; ++yIdx)
        {
          bool evenLine = yIdx % (2*width) < width;
          bool evenCol  = yIdx % 2 == 0;

          Trip const& rgb = in[yIdx];
          Trip yuv = rgb_to_yuv (rgb);
          auto& [y, u, v] = yuv;

          out[yIdx] = y;
          if (evenLine and evenCol)
            {// output chroma
              outU[uvIdx] = u;
              outV[uvIdx] = v;
              ++uvIdx;
            }
        }
    }
} // (End) implementation details



void
convert_RGB_intoBuffer (int format
                       ,char* targetBuff, int targetSiz
                       ,PackedRGB const& inputFrame
                       ,uint width, uint height
                       )
{
  static_assert (sizeof(byte) == sizeof(char));
  byte* outputData = reinterpret_cast<byte*> (targetBuff);

  if (format == fourCC("YUY2") or
      format == fourCC("UYVY") or
      format == fourCC("YVYU"))
    { // Handle popular packed formats:
      // These formats discard 1/3 of the information
      // input comes in RGB triplets, output discards 50% chroma
      assert (targetSiz > 0);
      assert (static_cast<uint>(targetSiz) == 2 * inputFrame.size());
      rgb_buffer_to_packed (format, inputFrame, outputData);
    }
  else
  if (format == fourCC("I420") or
      format == fourCC("YV12"))
    { // Handle common planar formats:
      // These formats discard half of the information
      // input comes in RGB triplets, output discards 75% chroma
      assert (targetSiz > 0);
      assert (static_cast<uint>(targetSiz) == inputFrame.size() *3/2);
      rgb_buffer_to_planar (format, inputFrame, outputData, width, height);
    }
  else
    __FAIL ("Logic broken: unsupported output target format");
}



XvCtx
openDisplay (Gtk::Window& appWindow, FrameRate fps)
{
  std::cout << "Open X-Video display slot..." << std::endl;

  Glib::RefPtr<Gdk::Window> gdkWindow = appWindow.get_window();

  XvCtx ctx{fps};
  ctx.window  = GDK_WINDOW_XID      (gdkWindow->gobj());
  ctx.display = GDK_WINDOW_XDISPLAY (gdkWindow->gobj());

  if (not XShmQueryExtension(ctx.display))
    __FAIL ("X11 shared memory extension not available for this display.");


  std::set<int> formats{}; // collection of usable formats supported by this setup

  uint count;
  XvAdaptorInfo* adaptorInfo;
  if (Success != XvQueryAdaptors (ctx.display, ctx.window, &count, &adaptorInfo))
    __FAIL ("unable to query XVideo adapters -- XV extension not available.");
  else
    {
      bool foundPort{false};
      for (uint n = 0; n < count and not foundPort; ++n )
        {
          if (not (adaptorInfo[n].type & XvImageMask))
            continue;                 // supports output of (frame)image data
          for (uint port = adaptorInfo[n].base_id;
                  port < adaptorInfo[n].base_id + adaptorInfo[n].num_ports;
                  port ++ )
            {
              if (Success == XvGrabPort (ctx.display, port, CurrentTime))
                {
                  auto isSupportedFormat = [](int formatCode){ return contains (SUPPORTED_FORMATS, formatCode); };

                  int num_formats;
                  XvImageFormatValues* list = XvListImageFormats (ctx.display, port, &num_formats);
                  for (int i = 0; i < num_formats; ++i)
                      if (isSupportedFormat (list[i].id))
                        formats.insert (list[i].id);

                  foundPort = not formats.empty();
                  if (foundPort)
                    {
                      ctx.port = port;
                      break;
                    }
                  else
                    XvUngrabPort (ctx.display, port, CurrentTime );
                }
            }//for all ports
        }// for all adaptors
      XvFreeAdaptorInfo (adaptorInfo);

      if (not foundPort)
        __FAIL ("unable to allocate XV port or unsupported pixel format.");

      // after having established a connection to the X-server,
      // allocate resources and setup buffers for the actual output
      ctx.gc = XCreateGC (ctx.display, ctx.window, 0, nullptr);

      // select one of the supported output data formats
      ctx.format = *formats.begin();
      std::cout << "Using Format: " << fourCCstring(ctx.format) << std::endl;

      ctx.xvImage = static_cast<XvImage*> (XvShmCreateImage (ctx.display
                                                            ,ctx.port
                                                            ,ctx.format
                                                            ,nullptr          // shared-mem buffer will be attached later
                                                            ,ctx.VIDEO_WIDTH
                                                            ,ctx.VIDEO_HEIGHT
                                                            ,&ctx.shmInfo
                                                            ));
      // allocate a shared-memory buffer
      // with a size as indicated in xvImage
      ctx.shmInfo.shmid = shmget (IPC_PRIVATE, ctx.xvImage->data_size, IPC_CREAT | 0777);
      if (ctx.shmInfo.shmid < 0)
        __FAIL ("unable to allocate a shared memory buffer for image data exchange");


      ctx.xvImage->data =
      ctx.shmInfo.shmaddr = static_cast<char*> (shmat (ctx.shmInfo.shmid, nullptr, 0));
      ctx.shmInfo.readOnly = false;

      if (not XShmAttach (ctx.display, &ctx.shmInfo))
        __FAIL ("failed to establish shared-memory setup for communication with XServer");

      XSync (ctx.display, false);
      shmctl(ctx.shmInfo.shmid, IPC_RMID, 0);
    }
   // hand-over the activated connection context
  //  to be managed by the GTK application...
  std::cout << "Started playback at "<<fps<<" frames/sec." << std::endl;
  return ctx;
}


void
displayFrame (XvCtx& ctx)
{
  assert (ctx.xvImage and ctx.xvImage->data);
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

  convert_RGB_intoBuffer (ctx.format
                         ,ctx.xvImage->data
                         ,ctx.xvImage->data_size
                         ,ctx.imgGen_.buildNext()
                         ,ctx.VIDEO_WIDTH
                         ,ctx.VIDEO_HEIGHT
                         );

  XvShmPutImage (ctx.display, ctx.port, ctx.window, ctx.gc
                ,ctx.xvImage
                ,0, 0, ctx.VIDEO_WIDTH, ctx.VIDEO_HEIGHT
                ,org_x, org_y, destW, destH
                ,false
                );
  XFlush (ctx.display);
}


void
cleanUp (XvCtx& ctx)
{
  std::cout << "STOP " << ctx.imgGen_.getFrameNr() << " frames displayed." << std::endl;

  XvStopVideo (ctx.display, ctx.port, ctx.window);
  XSync (ctx.display, false);
  if (ctx.shmInfo.shmaddr)
    {
      XShmDetach (ctx.display, &ctx.shmInfo);
      shmdt (ctx.shmInfo.shmaddr);
    }
  if (ctx.xvImage)
    XFree (ctx.xvImage);
  XFreeGC (ctx.display, ctx.gc);
  XvUngrabPort (ctx.display, ctx.port, CurrentTime);
}



int
main (int, const char*[])
{
    return GtkXvApp<XvCtx>{"demo.xv"}
            .onStart (openDisplay)
            .onFrame (displayFrame)
            .onClose (cleanUp)
            .run(30);
}
