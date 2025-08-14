#
# Skeleton of a demo application  -  open a GTK window and say hello
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU GPL version 2+ See the LICENSE file for details.
#
cmake_minimum_required(VERSION 3.20)

project(
    DemoSkeleton
    VERSION 1.0
    LANGUAGES CXX
)


# dependencies via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtkmm-3.0>=3.20)


# build complete source tree (detect changes on rebuild)
file(GLOB_RECURSE demo_sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE demo_headers CONFIGURE_DEPENDS "*.hpp")

add_executable(demo_gtk ${demo_sources} ${demo_headers})
target_compile_features(demo_gtk PUBLIC cxx_std_17)
target_include_directories(demo_gtk PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(demo_gtk PRIVATE PkgConfig::GTK)

