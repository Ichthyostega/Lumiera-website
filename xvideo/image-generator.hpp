/*
  image-generator.cpp  -  generate animated test video frames

   Copyright (C)
     2025,            Hermann Vosseler <Ichthyostega@web.de>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU GPL version 2+ See the LICENSE file for details.

* ************************************************************************/

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
        attenuate();
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
    void attenuate();
    void drawBall();

    constexpr static auto BLACK = gray (0);
    constexpr static auto WHITE = gray (0xFF);
    constexpr static auto GRAY1 = 0.25 * WHITE;
    constexpr static auto GRAY2 = 0.50 * WHITE;
    constexpr static auto GRAY3 = 0.75 * WHITE;
    constexpr static auto LT_YELLOW = trip (0xFF, 0xFF, 0xE0);
    constexpr static auto DARK_BLUE = trip (0x10, 0   , 0x50);

    constexpr static auto BALL = std::array{BLACK, GRAY3, WHITE, GRAY3, BLACK
                                           ,GRAY3, WHITE, WHITE, WHITE, GRAY3
                                           ,WHITE, WHITE, WHITE, WHITE, WHITE
                                           ,GRAY3, WHITE, WHITE, WHITE, GRAY3
                                           ,BLACK, GRAY3, WHITE, GRAY3, BLACK
                                           };
    constexpr static int BALL_SIZ = std::ceil (std::sqrt (BALL.size()));

    Vec2 p1_, p2_, v1_, v2_;
    double a1_{0}, a2_{0};

    void maybeBounce (Vec2&, Vec2&);
  };


  template<uint W, uint H>
  void
  ImageGenerator<W,H>::initGen()
    {
      std::srand (std::time (nullptr));
      p1_ = {0,0};
      v1_ = { 1 + rand() % 5,  1 + rand() % 5};
      p2_ = {rand() % int(W), rand() % int(H)};
      v2_ = {-1,-1};

      // initially fill background with colour bar pattern
      byte ON {0xE0};
      byte OFF{0};

      // classic NTSC colour bars  --R---G---B--
      std::array<Trip, 7> bars = {{{ ON, ON, ON}
                                  ,{ ON, ON,OFF}
                                  ,{OFF, ON, ON}
                                  ,{OFF, ON,OFF}
                                  ,{ ON,OFF, ON}
                                  ,{ ON,OFF,OFF}
                                  ,{OFF,OFF, ON}
                                 }};
      // create a colour strip pattern in the first row...
      for (uint x = 0; x < W; ++x)
        {  //  quantise into 7 columns
          uint col = x  * 7/W;
          img_[0][x] = bars[col];
        }
       // fill remaining rows alternating
      for (uint y = 1; y < H; ++y)
        if ((y/25) % 2 == 0)
          img_[y] = img_[0];
    }

  template<uint W, uint H>
  void
  ImageGenerator<W,H>::animatePos()
    {
      p1_ += v1_;
      maybeBounce (p1_, v1_);
      p2_ += v2_;
      maybeBounce (p2_, v2_);

      auto fade = [this](double& val, double target, uint dur)
                        {
                          if (val == target)
                            return;
                          val = std::min (target, val + target/(dur*fps_));
                        };
      fade (a1_, 0.05, 9);
      fade (a2_, 0.1,  2);
    }

  template<uint W, uint H>
  void
  ImageGenerator<W,H>::attenuate()
    {
      double dim{H*W};
      for (int row=0; row < int(H); ++row)
        for (int col=0; col < int(W); ++col)
          {
            Vec2 point{col,row};
            auto vicinity = [&](Vec2& p, int s)
                              {
                                int range = (p - point).norm();
                                return 1.0 /(1 + s/dim * range);
                              };
            Trip& px{img_[row][col]};

            decay (px, a1_,      (1.0 * vicinity (p2_, 1)) * DARK_BLUE);
            decay (px, a2_, px + (0.5 * vicinity (p1_,13)) * LT_YELLOW);
          }
    }

  template<uint W, uint H>
  void
  ImageGenerator<W,H>::drawBall()
    {
      for (uint row=0; row < BALL_SIZ; ++row)
        for (uint col=0; col < BALL_SIZ; ++col)
          img_[p1_.y+row][p1_.x+col] += BALL[row*BALL_SIZ + col];
    }

  template<uint W, uint H>
  void
  ImageGenerator<W,H>::maybeBounce (Vec2& p, Vec2& v)
    {
      auto randomise = [](int& val)
                          {
                            int offset{rand() % 7 - 3};
                            val += offset;
                            if (abs(val > 3) and val*offset > 0)
                              val /= 2; // damp excess outward trend
                          };
      int limX = W-BALL_SIZ;
      int limY = H-BALL_SIZ;

      if (p.x <= 0)
        {
          p.x *= -1;
          v.x *= -1;
          randomise (v.y);
        }
      else
      if (p.x >= limX)
        {
          p.x  = limX - (p.x-limX);
          v.x *= -1;
          randomise (v.y);
        }
      if (p.y <= 0)
        {
          p.y *= -1;
          v.y *= -1;
          randomise (v.x);
        }
      else
      if (p.y >= limY)
        {
          p.y  = limY - (p.y-limY);
          v.y *= -1;
          randomise (v.x);
        }
    }


#endif /*IMAGE_GENERATOR_H*/
