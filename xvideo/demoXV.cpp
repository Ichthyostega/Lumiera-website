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

const std::set<int> SUPPORTED_FORMATS = {fourCC("I420")    ///////TODO implement
                                        ,fourCC("YV12")    ///////TODO implement
                                        ,fourCC("YUY2")
                                        ,fourCC("UYVY")    ///////TODO implement
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




void
convert_RGB_intoBuffer (int format, char* targetBuff, int targetSiz
                       ,XvCtx::ImgGen::Img const& inputFrame)
{
  if (format == fourCC("YUY2"))
    {
      
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
  return ctx;
}


void
displayFrame (XvCtx& ctx)
{
  assert (ctx.xvImage and ctx.xvImage->data);
  std::cout << "tick ... " << ctx.imgGen_.getFrameNr() << std::endl;
  
  int org_x = 0
    , org_y = 0
    , destW = ctx.VIDEO_WIDTH
    , destH = ctx.VIDEO_HEIGHT;
  /////////////////////////////////////////TODO actually compute output window position

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
            .run(4);
}
