#
# Demo application  -  GTK application with XVideo output
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is Open Source software and provided under the MIT License.
# See the LICENSE file for details.
#
cmake_minimum_required(VERSION 3.20)

project(
    DemoXVideo
    VERSION 1.0
    LANGUAGES CXX
)


# dependencies via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK  REQUIRED IMPORTED_TARGET gtkmm-3.0>=3.20)
pkg_check_modules(XLIB REQUIRED IMPORTED_TARGET x11>=1.7)
pkg_check_modules(XEXT REQUIRED IMPORTED_TARGET xext>=1.3)
pkg_check_modules(XV   REQUIRED IMPORTED_TARGET xv>=1.0)


# build complete source tree (detect changes on rebuild)
file(GLOB_RECURSE demo_xv_sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE demo_xv_headers CONFIGURE_DEPENDS "*.hpp")

add_executable(demo_xv ${demo_xv_sources} ${demo_xv_headers})
target_compile_features(demo_xv PUBLIC cxx_std_17)
target_compile_options (demo_xv PRIVATE -Wall)
target_include_directories(demo_xv PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(demo_xv PRIVATE PkgConfig::GTK)
target_link_libraries(demo_xv PRIVATE PkgConfig::XLIB)
target_link_libraries(demo_xv PRIVATE PkgConfig::XEXT)
target_link_libraries(demo_xv PRIVATE PkgConfig::XV)

