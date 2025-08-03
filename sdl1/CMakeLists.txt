#
# Demo application  -  GTK application with SDL-1 video output
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU GPL version 2+ See the LICENSE file for details.
#
cmake_minimum_required(VERSION 3.20)

project(
    DemoSdl1Video
    VERSION 1.0
    LANGUAGES CXX
)


# dependencies via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK  REQUIRED IMPORTED_TARGET gtkmm-3.0>=3.20)
pkg_check_modules(SDL1 REQUIRED IMPORTED_TARGET sdl>=1.2)
#pkg_check_modules(XLIB REQUIRED IMPORTED_TARGET x11>=1.7)
#pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext>=1.3)
#pkg_check_modules(XV   REQUIRED IMPORTED_TARGET xv>=1.0)


# build complete source tree (detect changes on rebuild)
file(GLOB_RECURSE demo_sdl1_sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE demo_sdl1_headers CONFIGURE_DEPENDS "*.hpp")

add_executable(demo_sdl1 ${demo_sdl1_sources} ${demo_sdl1_headers})
target_compile_features(demo_sdl1 PUBLIC cxx_std_17)
target_compile_options (demo_sdl1 PRIVATE -Wall)
target_include_directories(demo_sdl1 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(demo_sdl1 PRIVATE PkgConfig::GTK)
target_link_libraries(demo_sdl1 PRIVATE PkgConfig::SDL1)
#target_link_libraries(demo_sdl1 PRIVATE PkgConfig::XLIB)
#target_link_libraries(demo_sdl1 PRIVATE PkgConfig::XEXT)
#target_link_libraries(demo_sdl1 PRIVATE PkgConfig::XV)

