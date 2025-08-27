Video Output Example Code
=========================
:revision: 1.0
:author: Benny Lyons & Hermann Voßeler
:date: 8/2025


This directory contains a collection of code examples to demonstrate how to
display generated video frames from a C++ application running under Linux.
The code is Free Software and can be used under the terms of the *GPL v2+*.
See the file LICENSE for details.


Building the code
-----------------
As convenience, a CMake build system is provided, structured hierarchically:
The 'examples/' directory defines a common build for all examples; yet each
individual subdirectory provides again a self-contained CMake build definition,
which is included from the top-level build. The reason for this setup is to allow
to build only a subset, because each of the examples deals with a specific technology
and thus requires a lot of special library dependencies.

For a simple build from the command line, use the following scheme:

--------------
cmake -S <path/to/source> -B <path/to/builddir>  [-D SOME_SETTING]
cmake --build <path/to/builddir> --target <ID> -- [options for make....]
--------------

A typical invocation for a complete build might be...

[source,sh]
--------------
cmake -S examples -B build
cmake --build build -- -j4

# clean tree with...
cmake --build build --target clean
--------------

TIP: The `-j4` option tells make to use 4 cores concurrently to speed up the build.
     Furthermore, you might want to configure further options with a graphical GUI,
     e.g: `cmake-gui build`; this way you can configure individual library dependencies
     to point to custom locations.

Use at least GCC-14
~~~~~~~~~~~~~~~~~~~
Some older C++ compilers seem to have problems with a `constexpr` function used in 'commons.hpp'.
Either comment out the line with the static assertion, or use at least GCC-14, which can be
configured for CMake with

-----
cmake -S examples -B build -DCMAKE_CXX_COMPILER=g++-14
-----


Dependencies
~~~~~~~~~~~~
The objective of this demo and tutorial is to explore several options to display video frames.
This implies to use various different libraries and frameworks -- each which its unique set
of dependencies; generally speaking, we need the _development setup_ of the libraries, since
we are compiling and linking against the dependencies; so either you have to install a lot of
transitive dependencies manually, or (preferrably) you might want to rely on the _package manager_
of Linux distribution. The following list shows how to install the dependencies on Debian / Ubuntu.

Basic build environment::
+
-----------
apt install build-essential cmake
-----------
+
Build a GTK-3 application with C++::
+
-----------
apt install libgtkmm-3.0-dev
-----------
+
Use the XVideo X11 extension::
+
-----------
apt install libx11-dev libxext-dev libxv-dev
-----------
+
SDL Demo(Legacy v1.2)::
+
-----------
apt install libsdl1.2-dev
-----------
+
GLX Demo::
+
-----------
apt install libglx-dev libx11-dev libgl-dev
-----------
+
EGL Demo::
+
-----------
apt install libx11-dev libglew-dev libegl-dev libgl-dev
-----------

