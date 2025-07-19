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

    int
    abs()  const
      {
        return dot (*this,*this);
      }

    friend int
    dist (Vec2 const& u, Vec2 const& v)
      {
        float absDiff = (v - u).abs();
        return int(std::sqrt (absDiff));
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
cval (Trip const& t)
{
  static_assert (idx < t.size());
  return int(t[idx]);
}

constexpr inline Trip
operator* (double fac, Trip const& t)
{
  return trip (fac * cval<0>(t)
              ,fac * cval<1>(t)
              ,fac * cval<2>(t)
              );
}

constexpr inline Trip
operator+ (Trip const& tu, Trip const& tv)
{
  return trip (cval<0>(tu) + cval<0>(tv)
              ,cval<1>(tu) + cval<1>(tv)
              ,cval<2>(tu) + cval<2>(tv)
              );
}

constexpr inline Trip
operator- (Trip const& tu, Trip const& tv)
  {
    return trip (cval<0>(tu) - cval<0>(tv)
                ,cval<1>(tu) - cval<1>(tv)
                ,cval<2>(tu) - cval<2>(tv)
                );
  }

constexpr inline int
distNorm (Trip const& tu, Trip const& tv)
  {
    return cval<0>(tu) * cval<0>(tv)
         + cval<1>(tu) * cval<1>(tv)
         + cval<2>(tu) * cval<2>(tv)
         ;
  }


#endif /*COMMONS_H*/
