/*
  image-generator.cpp  -  generate animated test video frames

   Copyright (C)
     2025,            Hermann Vosseler <Ichthyostega@web.de>


  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/

#ifndef IMAGE_GENERATOR_H
#define IMAGE_GENERATOR_H

#include "commons.hpp"

#include <cstddef>
#include <array>


using uint = unsigned int;
using std::byte;

template<uint W, uint H>
class ImageGenerator
  {

  public:
    using Row = std::array<Trip, W>;
    using Img = std::array<Row,  H>;

    static_assert (sizeof(Img) == W * H * 3);

    using PackedRGB = std::array<Trip, W*H>;


    ImageGenerator(uint fps)
      : fps_{fps}
      , frameNr_{0}
      , img_{Row{Trip{}}}
      { };


    PackedRGB const&
    current()  const
      {
        return reinterpret_cast<PackedRGB const&> (img_);
      }

    /**
     * generate the next frame of the animation.
     * @return reference to the buffer with RGB888 data.
     */
    PackedRGB const&
    buildNext()
      {
        if (frameNr_ == 0)
          initGen();
        animatePos();
        drawBall();
        ++frameNr_;
        return current();
      }

    uint getFrameNr()  const { return frameNr_; }
    uint getFps()      const { return fps_;     }

  private:
    uint fps_;
    uint frameNr_;
    Img img_;

    void initGen();
    void animatePos();
    void drawBall();

    constexpr static auto BLACK = gray (0);
    constexpr static auto WHITE = gray (0xFF);
    constexpr static auto GRAY1 = 0.25 * WHITE;
    constexpr static auto GRAY2 = 0.50 * WHITE;
    constexpr static auto GRAY3 = 0.75 * WHITE;
    constexpr static auto LT_YELLOW = trip (0xFF, 0xFF, 0x40);

    constexpr static auto BALL = std::array{BLACK, GRAY3, WHITE, GRAY3, BLACK
                                           ,GRAY3, WHITE, WHITE, WHITE, GRAY3
                                           ,WHITE, WHITE, WHITE, WHITE, WHITE
                                           ,GRAY3, WHITE, WHITE, WHITE, GRAY3
                                           ,BLACK, GRAY3, WHITE, GRAY3, BLACK
                                           };
    constexpr static int BALL_SIZ = std::ceil (std::sqrt (BALL.size()));
  };


  template<uint W, uint H>
  void
  ImageGenerator<W,H>::initGen()
    {

    }

  template<uint W, uint H>
  void
  ImageGenerator<W,H>::animatePos()
    {

    }

  template<uint W, uint H>
  void
  ImageGenerator<W,H>::drawBall()
    {
      ///////////////TODO real animation
      img_[(frameNr_ / W) % H][frameNr_ % W] = LT_YELLOW;
    }



#endif /*IMAGE_GENERATOR_H*/

