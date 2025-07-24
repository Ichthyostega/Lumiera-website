/*
  demoSDL1.cpp  -  output video from a GTK application, using the SDL Abstraction (v1.2)

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/


#include "commons.hpp"
#include "gtk-sdl-app.hpp"
#include "image-generator.hpp"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <array>

#include <SDL/SDL.h>



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

constexpr auto SUPPORTED_FORMATS = std::array{fourCC("YUY2")
//                                           ,fourCC("UYVY")    ///////TODO implement
//                                           ,fourCC("YVYU")
//                                           ,fourCC("IYUV")    ///////TODO implement  (this is equivalent to I420)
//                                           ,fourCC("YV12")    ///////TODO implement
                                             };



/**
 * Output connection context used for video display via SDL.
 */
struct SdlCtx
  {
    SDL_Surface* surface_{nullptr};
    SDL_Overlay* overlay_{nullptr};
    SDL_Rect windowPos_{};
    SDL_Rect targetPos_{};


    // hard wired here (should be configurable in real-world usage)
    constexpr static uint VIDEO_WIDTH {320};
    constexpr static uint VIDEO_HEIGHT{240};

    using ImgGen = ImageGenerator<VIDEO_WIDTH,VIDEO_HEIGHT>;
    ImgGen imgGen_;

    SdlCtx(FrameRate fps)
      : imgGen_{fps}
      { }
  };


namespace { // implementation details : pixel format conversion

  using std::clamp;

  using ImgGen    = SdlCtx::ImgGen;
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
convert_RGB_intoBuffer (SDL_Overlay& target
                       ,PackedRGB const& inputFrame)
{
  static_assert (sizeof(byte) == sizeof(uint8_t));

  if (target.format == fourCC("YUY2"))
    {
      // this format discards 1/3 of the information
      // input comes in RGB triplets, output discards 50% chroma
      assert (target.planes == 1);
      assert (target.pitches[0] == 2 * SdlCtx::VIDEO_WIDTH);
      byte* outputData = reinterpret_cast<byte*> (target.pixels[0]);
      rgb_buffer_to_yuy2 (inputFrame, outputData);
    }
  else
    __FAIL ("Logic broken: unsupported output target format");
}


SDL_Rect
determinePosition (Gtk::Window const& appWindow)
{
  int x,y,w,h;
  appWindow.get_position (x, y);
  appWindow.get_size (w,h);
  SDL_Rect pos;
  pos.x = x; // note narrowing conversion int -> int16_t
  pos.y = y;
  pos.w = w;
  pos.h = h;
  return pos;
}


SdlCtx
openDisplay (Gtk::Window& appWindow, FrameRate fps)
{
  std::cout << "Open X-Video display slot..." << std::endl;

  SdlCtx ctx{fps};
  int success = SDL_Init(SDL_INIT_VIDEO);
  if (success < 0)
    __FAIL ("SDL framework not usable.");

  // Note: when calling GetVideoInfo _before_ SetVideoMode,
  //       SDL will fill in the _best available_ video format.
  SDL_VideoInfo const* videoInfo = SDL_GetVideoInfo();
  uint8_t bpp = videoInfo->vfmt->BitsPerPixel;
  if (bpp == 8)
    __FAIL ("this system only supports palette colours.");

  ctx.windowPos_ = determinePosition (appWindow);
  std::cout << ".... System supports "<<uint(bpp)<<" bit-per-pixel colour\n"
            << ".... Screen size: "
            << videoInfo->current_w << " x "
            << videoInfo->current_h << " px\n"
            << ".... Window: at ("
            << ctx.windowPos_.x <<","<< ctx.windowPos_.y << ") size "
            << ctx.windowPos_.w<<" x "<<ctx.windowPos_.h << " px"
            << std::endl;

  assert (bpp == 16 or bpp == 24 or bpp == 32);
  assert (ctx.windowPos_.w == ctx.VIDEO_WIDTH and // ◁────────────────────┨ specifically arranged for this demo
          ctx.windowPos_.h == ctx.VIDEO_HEIGHT);

  const auto DISPLAY_FLAGS = SDL_HWSURFACE        // framebuffer surface backed by video hardware memory
                           | SDL_DOUBLEBUF        // request hardware supported double-buffering
                           | SDL_NOFRAME;         // request surface without window decoration

  ctx.surface_ = SDL_SetVideoMode (ctx.VIDEO_WIDTH, ctx.VIDEO_HEIGHT, bpp, DISPLAY_FLAGS);
  if (not ctx.surface_)
    __FAIL ("access to hardware backed framebuffer not possible");

  for (int formatCode : SUPPORTED_FORMATS)
    {
      ctx.overlay_ = SDL_CreateYUVOverlay (ctx.VIDEO_WIDTH
                                          ,ctx.VIDEO_HEIGHT
                                          ,formatCode     // ◁────────────┨ supported formats are defined in sdl.h (but happen to be fourCC codes)
                                          ,ctx.surface_); // ◁────────────┨ this establishes a link to the underlying surface
      if (ctx.overlay_
          and ctx.overlay_->hw_overlay)
        break;
    }
  if (not ctx.overlay_)
    __FAIL ("unable to setup a YUV converter with a supported format");

   //////////////////////////////////////TODO find out how to position a transparent overly into the App window 
  ctx.targetPos_ = ctx.windowPos_;
  ctx.targetPos_.x = 0;
  ctx.targetPos_.y = 0;

   // hand-over the activated connection context
  //  to be managed by the GTK application...
  std::cout << "Started playback at "<<fps<<" frames/sec." << std::endl;
  return ctx;
}


void
displayFrame (SdlCtx& ctx)
{
  uint frameNr = ctx.imgGen_.getFrameNr();
  uint fps     = ctx.imgGen_.getFps();
  if (0 == frameNr % fps)
    std::cout << "tick ... " << ctx.imgGen_.getFrameNr() << std::endl;

  SDL_LockYUVOverlay (ctx.overlay_);
  convert_RGB_intoBuffer (*ctx.overlay_
                         ,ctx.imgGen_.buildNext()
                         );
  SDL_UnlockYUVOverlay (ctx.overlay_);

  //  Note: this demo uses a fixed-size window and hard-coded video size;
  //        a real-world implementation would have to place the video frame
  //        dynamically into the available screen space, possibly scaling up/down;
  //        In such a case, you'd pass the desired target position here, and SDL would scale
  SDL_DisplayYUVOverlay (ctx.overlay_, & ctx.targetPos_);
}


void
cleanUp (SdlCtx& ctx)
{
  std::cout << "STOP " << ctx.imgGen_.getFrameNr() << " frames displayed." << std::endl;

  if (ctx.overlay_)
    SDL_FreeYUVOverlay (ctx.overlay_);
  // note: ctx.surface_ is a system resource and will be managed/freed by SDL
  SDL_Quit();
}



int
main (int, const char*[])
{
    return GtkSdlApp<SdlCtx>{"demo.sdl1"}
            .onStart (openDisplay)
            .onFrame (displayFrame)
            .onClose (cleanUp)
            .run(30);
}
