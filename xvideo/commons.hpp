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


#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <set>

using std::byte;
using uint = unsigned int;
using FrameRate = uint;


void
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


#endif /*COMMONS_H*/

