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

# Most people er expect are using GNU GDB, so lets optimise debugging symbols for gdb
if( CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET( CMAKE_CXX_FLAGS  "-O0 -ggdb3 -Wall")
endif()

project(
    VideoOutputDemos
)


add_subdirectory (skel)
add_subdirectory (xvideo)
add_subdirectory (sdl1)
add_subdirectory (glx)
add_subdirectory (egl)
