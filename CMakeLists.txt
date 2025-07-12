#
# Seletton of a demo application  -  open a GTK window and say hellw
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is free software: 
# TODO License
#
cmake_minimum_required (VERSION 3.20)

# default to Debug build type, unless specified in cache or command line
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Debug CACHE STRING "Default to Debug build")
    set (CMAKE_VERBOSE_MAKEFILE TRUE CACHE STRING "Show full compile commands")
endif()


project(
    VideoOutputDemos
)


add_subdirectory (skel)
