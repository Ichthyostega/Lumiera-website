/*
  demoXV.cpp  -  output video from a GTK application, using the X-Video standard

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/


#include "commons.hpp"
#include "gtk-xv-app.hpp"
#include "image-generator.hpp"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <set>

// for low-level access -> X-Window
#include <gdk/gdkx.h>

// X11 and XVideo extension
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>



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

const std::set<int> SUPPORTED_FORMATS = {fourCC("I420")    ///////DOING
                                        ,fourCC("YV12")    ///////DOING
                                        ,fourCC("YUY2")
                                        ,fourCC("UYVY")    ///////DOING
                                       };

//// TODO:
//// Get rid of this. Defining the same things twice
//// is not a good idea, but I needed the checks in a
//// heavily used loop and calling functions might prove
//// expensive and I did not want to tax the loop unnecessarily 
enum packedFomrats { pYUY2
                    ,pUYVY
                    ,pYVYU
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

  void
  rgb_buffer_to_packed (int format, PackedRGB const &in, byte *out)
  {
    uint cntPix = in.size ();
    assert (cntPix % 2 == 0);

    packedFomrats pFormat;
    if (format == fourCC ("YUY2"))
      pFormat = pYUY2;
    else if (format == fourCC ("UYVY"))
      pFormat = pUYVY;
    else if (format == fourCC ("YVYU"))
      pFormat = pYVYU;
    else
      __FAIL ("Invald packed format; cannot convert");                 
                     
    
    for (uint i = 0; i < cntPix; i += 2)
      {// convert and interleave 2 pixels in one step
        uint op = i * 2;                           // Output packed in groups with 2 bytes
        Trip const& rgb0 = in[i];
        Trip const& rgb1 = in[i+1];
        Trip yuv0 = rgb_to_yuv (rgb0);
        Trip yuv1 = rgb_to_yuv (rgb1);

        auto& [y0,u0,v0] = yuv0;
        auto& [y1,_u,_v] = yuv1;                   // note: this format discards half of the chroma information

        
        // Both g++ Version 12 fails with SUPPORTED_FORMATS for pFormat
        switch (pFormat) {
        case  pYUY2:
          out[op    ] = y0;
          out[op + 1] = u0;
          out[op + 2] = y1;
          out[op + 3] = v0;  
          break;
        case pUYVY:
          out[op    ] = u0;
          out[op + 1] = y0;
          out[op + 2] = v0;
          out[op + 3] = y1;
          break;
        case pYVYU:
          out[op    ] = y0;
          out[op + 1] = v0;
          out[op + 2] = y1;
          out[op + 3] = u0;
          break;
        default:
          __FAIL ("Invald format for pFormat");
          break;
        }
  }
  }

  // I420
  // *out: Y1Y2Y3Y4....U1U2....V1V2...
  void
  rgb_buffer_to_i420 (PackedRGB const &in, byte *out)
  {
    uint cntP = in.size ();
    uint cntUV = (in.size () / 2) * (in.size () / 2);
    uint offSet = cntP + cntUV;
    assert (cntP % 2 == 0);

    // u, v components should not be over counted
    const uint freq = 2; // set of numbers, but only want every third position;
                         // but we're commencing at 0, so it's freq-1
    uint uvCnt = 0;      // index over uv components
    uint j;              // every second matched uv component counter;

    for (uint i = 0; i < cntP; i++)
      {
        Trip const &rgb = in[i];
        Trip yuv = rgb_to_yuv (rgb);

        // Memory: y*8 u*2 v*2 bits/pixel
        auto &[y, u, v] = yuv;
        out[i] = y;

        if (i % 2)
          { // odd
            // Nothing to fo here for the uv components
          }
        else
          { // even including i=0
            if (!(uvCnt % freq))
              {
                if (!(j % 2)) // only want every second element
                  {
                    out[i + offSet] = u;
                    out[i + offSet + offSet / 4] = v;
                    j++;
                  }
              }
            uvCnt++;
          }
      }
  }

  // YV12: 
  //      planar YUV 4:2:0. 12Bits/pixel
  //      YV indicates the the U and V colour planes are exchanged
  //      YYYYYYYY VV UU   --> YYYYYYYY UU VV
  void
  rgb_buffer_to_yv12 (PackedRGB const& in, byte* out)
  {
    uint cntPix = in.size();
    //uint cntPix = (in.VIDEO_WIDTH * in.VIDEO_WIDTH);
    //assert (cntPix %2 == 0);
    for (uint i = 0; i < cntPix; i += 2)
      {// convert and interleave 2 pixels in one step
        uint op = i * 2;                           // Output packed in groups with 2 bytes
        Trip const& rgb0 = in[i];
        Trip const& rgb1 = in[i+1];
        Trip yuv0 = rgb_to_yuv (rgb0);
        Trip yuv1 = rgb_to_yuv (rgb1);

        auto& [y0,u0,v0] = yuv0;
        auto& [y1,_u,_v] = yuv1;                   // note: this format discards half of the chroma information

        // FORMAT: over 2 pixells: Y1V1U1Y2
        out[op    ] = y0;
        out[op + 1] = v0; 
        out[op + 2] = u0; 
        out[op + 3] = y1;
  }   }


  // UXVY
  void
  rgb_buffer_to_uyvy (PackedRGB const& in, byte* out)
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

        out[op    ] = u0;
        out[op + 1] = y0;
        out[op + 2] = v0;
        out[op + 3] = y1;
   }  }

  
} // (End) implementation details


void
convert_RGB_intoBuffer (int format, char* targetBuff, int targetSiz
                       ,PackedRGB const& inputFrame)
{
  static_assert (sizeof(byte) == sizeof(char));
  byte* outputData = reinterpret_cast<byte*> (targetBuff);



  if (format == fourCC("YUY2") ||
      format == fourCC("UYVY") ||
      format == fourCC("YVYU"))
    {
      // Popular packed formats: similar conversion procedure
            
      // this format discards 1/3 of the information
      // input comes in RGB triplets, output discards 50% chroma
      assert (targetSiz == 2 * inputFrame.size());
      rgb_buffer_to_packed (format, inputFrame, outputData);
    }
  else if (format == fourCC("I420"))
    {
      // planar 4:2:0 YUV
      rgb_buffer_to_i420 (inputFrame, outputData);
    }
  else if (format == fourCC("YV12"))
    {
      // planar YUV 4:2:0. 12Bits/pixel
      // same as above but U and V exchanged
      // TODO: this should be pass?
      // assert (targetSiz == 2 * inputFrame.size());
      rgb_buffer_to_yv12 (inputFrame, outputData);
    }
  else if (format == fourCC("UYVY"))
    {
      // packed 4:2:2 YUV
      rgb_buffer_to_uyvy (inputFrame, outputData);
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
        __FAIL ("unable to allocate XV port with supported pixel format.");

      // after having established a connection to the X-server,
      // allocate resources and setup buffers for the actual output
      ctx.gc = XCreateGC (ctx.display, ctx.window, 0, nullptr);

      // select suitable graphic data format
      if (contains (formats, fourCC("YUY2")))
        ctx.format = fourCC("YUY2");
      else if (contains (formats, fourCC("I420")))
        ctx.format = fourCC("I420");
      else if (contains (formats, fourCC("YV12")))
        ctx.format = fourCC("YV12");
      else if (contains (formats, fourCC("UYVY")))
              ctx.format = fourCC("UYVY");
      else
       __FAIL ("current setup can not handle any of the pixel formats supported by this implementation.");

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
