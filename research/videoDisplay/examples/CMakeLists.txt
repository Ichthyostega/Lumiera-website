#
# CMake-Build for a collection of demo applications for video output
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU GPL version 2+ See the LICENSE file for details.
#
cmake_minimum_required (VERSION 3.20)

# default to Debug build type, unless specified in cache or command line
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Debug CACHE STRING "Default to Debug build")
    set (CMAKE_VERBOSE_MAKEFILE TRUE CACHE STRING "Show full compile commands")
endif()
# optimise debugging symbols for GNU gdb
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb3" CACHE STRING "Special option for debug builds")


project(
    VideoOutputDemos
)

add_compile_options("-Wall")

add_subdirectory (skel)
add_subdirectory (xvideo)
add_subdirectory (sdl1)
add_subdirectory (glx)
add_subdirectory (egl)
