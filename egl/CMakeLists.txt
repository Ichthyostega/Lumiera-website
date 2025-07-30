#
# Demo application  -  GTK application with OpenGL attached via EGL
#
# Copyright 2025, Hermann Vosseler <Ichthyostega@web.de>
#
# This program is Open Source software and provided under the MIT License.
# See the LICENSE file for details.
#
cmake_minimum_required(VERSION 3.20)

project(
    DemoEglVideo
    VERSION 1.0
    LANGUAGES CXX
)


# dependencies via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK  REQUIRED IMPORTED_TARGET gtkmm-3.0>=3.20)
pkg_check_modules(XLIB REQUIRED IMPORTED_TARGET x11>=1.7)
pkg_check_modules(EGL  REQUIRED IMPORTED_TARGET egl>=1.5)
pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew>=2.2)



# build complete source tree (detect changes on rebuild)
file(GLOB_RECURSE demo_egl_sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE demo_egl_headers CONFIGURE_DEPENDS "*.hpp")

add_executable(demo_egl ${demo_egl_sources} ${demo_egl_headers})
target_compile_features(demo_egl PUBLIC cxx_std_17)
target_include_directories(demo_egl PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(demo_egl PRIVATE PkgConfig::GTK)
target_link_libraries(demo_egl PRIVATE PkgConfig::XLIB)
target_link_libraries(demo_egl PRIVATE PkgConfig::EGL)
target_link_libraries(demo_egl PRIVATE PkgConfig::GLEW)

