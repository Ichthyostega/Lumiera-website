/*
  commons.hpp  -  common definitions and utils for the video output demo code

   Copyright (C)
     2025,            Benny Lyons <benny.lyons@gmx.net>
     2025,            Hermann Vosseler <Ichthyostega@web.de>


  This program is Open Source software and provided under the MIT License.
  See the LICENSE file for details.

* *****************************************************************/

#ifndef COMMONS_H
#define COMMONS_H


#include <cmath>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <algorithm>
#include <string>
#include <array>
#include <set>

using std::byte;
using std::string;
using uint = unsigned int;
using FrameRate = uint;


inline void
__FAIL (std::string msg)
{
  std::cerr << "FAIL: " << msg << std::endl;
  std::abort();
}

/** shortcut for set value containment test (C++17) */
template <typename T, class CMP, class ALO>
inline bool
contains (std::set<T,CMP,ALO> const& set, T const& val)
{
  return set.end() != set.find (val);
}




/* == simplistic 2D vector math == */

struct Vec2
  {
    int x{0};
    int y{0};

    Vec2
    operator-()  const
      {
        return {-x, -y};
      }

    friend Vec2
    operator+ (Vec2 const& u, Vec2 const& v)
      {
        return {u.x+v.x, u.y+v.y};
      }
    friend Vec2
    operator- (Vec2 const& u, Vec2 const& v)
      {
        return {u.x-v.x, u.y-v.y};
      }

    friend int
    dot (Vec2 const& u, Vec2 const& v)
      {
        return u.x*v.x + u.y*v.y;
      }

    Vec2&
    operator+= (Vec2 const& o)
      {
        x += o.x;
        y += o.y;
        return *this;
      }

    int
    norm()  const
      {
        return dot (*this,*this);
      }

    friend int
    dist (Vec2 const& u, Vec2 const& v)
      {
        float normDiff = (v - u).norm();
        return int(std::sqrt (normDiff));
      }
  };


/* == 8bit colour triplets == */

using Trip = std::array<byte,3>;

constexpr inline byte
bClamp (int val)
{
  return byte(std::clamp (val, 0,0xFF));
}

constexpr inline Trip
trip (int c1, int c2, int c3)
{
  return Trip{{bClamp(c1)
              ,bClamp(c2)
              ,bClamp(c3)
             }};
}

constexpr inline Trip
gray (int lum =0)
{
  return trip (lum,lum,lum);
}

template<uint idx>
constexpr inline int
c (Trip const& t)
{
  static_assert (idx < t.size());
  return int(t[idx]);
}

constexpr inline Trip
operator* (double fac, Trip const& t)
{
  return trip (fac * c<0>(t)
              ,fac * c<1>(t)
              ,fac * c<2>(t)
              );
}

constexpr inline Trip
operator+ (Trip const& t1, Trip const& t2)
{
  return trip (c<0>(t1) + c<0>(t2)
              ,c<1>(t1) + c<1>(t2)
              ,c<2>(t1) + c<2>(t2)
              );
}

constexpr inline Trip
operator- (Trip const& t1, Trip const& t2)
{
  return trip (c<0>(t1) - c<0>(t2)
              ,c<1>(t1) - c<1>(t2)
              ,c<2>(t1) - c<2>(t2)
              );
}

constexpr inline void
operator += (Trip& c, Trip const& o)
{
  c = c + o;
}


/** closest prime to (2^32 * goldenRatio) % 2^32 */
const uint32_t KNUTH_MAGIC{0x9e3779b1};

/**
 * A cheap source of random bits, based on repeated mixing.
 * @warning fast but not high quality...
 */
inline uint32_t
noise()
{
  static uint32_t state{0x55555555};
  return state ^= KNUTH_MAGIC
                + (state<<6)
                + (state>>2)
                ;
}

/**
 * Manipulate a colour value to add a decay step towards a target colour.
 * When applying this repeatedly to the same colour, it will approach
 * the target colour asymptotically, by an exponential function,
 * because ∂/∂x e^kx ≡ k · e^kx
 * @param feedback controls the strength of the effect
 * @note adding 4 bit of random dither to each channel to avoid banding
 */
inline void
decay (Trip& col, double feedback, Trip const& target)
{
  auto randBits = noise();
  auto dither = [&]{ return 0.07 * (-7.5 + (0xF & (randBits >>= 4))); };
  col = trip (c<0>(col) + feedback * (c<0>(target) - c<0>(col)) + dither()
             ,c<1>(col) + feedback * (c<1>(target) - c<1>(col)) + dither()
             ,c<2>(col) + feedback * (c<2>(target) - c<2>(col)) + dither()
             );
}


#endif /*COMMONS_H*/
