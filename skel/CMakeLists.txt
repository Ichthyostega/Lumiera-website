#
# Seletton of a demo application  -  open a GTK window and say hellw
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is free software: 
# TODO License
#
cmake_minimum_required(VERSION 3.20)

project(
    DemoSkeletton
    VERSION 1.0
    LANGUAGES CXX
)


# dependencies via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile>=1.0.28)


# build complete source tree (detect changes on rebuild)
file(GLOB_RECURSE demo_sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE demo_headers CONFIGURE_DEPENDS "*.hpp")

add_executable(demo_gtk ${demo_sources} ${demo_headers})
target_compile_features(demo_gtk PUBLIC cxx_std_17)
target_include_directories(demo_gtk PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(demo_gtk PRIVATE PkgConfig::SNDFILE)

