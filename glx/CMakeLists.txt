#
# Demo application  -  GTK application with GLX video output
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is Open Source software and provided under the MIT License.
# See the LICENSE file for details.
#
cmake_minimum_required(VERSION 3.20)

project(
    DemoGlxVideo
    VERSION 1.0
    LANGUAGES CXX
)


# dependencies via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK  REQUIRED IMPORTED_TARGET gtkmm-3.0>=3.20)
pkg_check_modules(XLIB REQUIRED IMPORTED_TARGET x11>=1.7)
pkg_check_modules(GLX  REQUIRED IMPORTED_TARGET glx>=1.4)
pkg_check_modules(OGL  REQUIRED IMPORTED_TARGET gl>=1.2)


# build complete source tree (detect changes on rebuild)
file(GLOB_RECURSE demo_glx_sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE demo_glx_headers CONFIGURE_DEPENDS "*.hpp")

add_executable(demo_glx ${demo_glx_sources} ${demo_glx_headers})
target_compile_features(demo_glx PUBLIC cxx_std_17)
target_compile_options (demo_glx PRIVATE -Wall)
target_include_directories(demo_glx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(demo_glx PRIVATE PkgConfig::GTK)
target_link_libraries(demo_glx PRIVATE PkgConfig::XLIB)
target_link_libraries(demo_glx PRIVATE PkgConfig::GLX)
target_link_libraries(demo_glx PRIVATE PkgConfig::OGL)

