Munt mt32emu
============

mt32emu is a part of the Munt project. It represents a _C/C++ library_
named **libmt32emu** which allows to emulate (approximately) [the Roland MT-32,
CM-32L and LAPC-I synthesiser modules](https://en.wikipedia.org/wiki/Roland_MT-32).

This library is intended for developers wishing to integrate an MT-32 emulator
into a driver or an application. "Official" driver for Windows and a cross-platform
UI-enabled application are available in the Munt project and use this library:
[mt32emu_win32drv](https://github.com/munt/munt/tree/master/mt32emu_win32drv)
and [mt32emu_qt](https://github.com/munt/munt/tree/master/mt32emu_qt) respectively.


Building
========

mt32emu requires CMake to build. More info can be found at [the CMake homepage](http://www.cmake.org/).
For a simple in-tree build in a POSIX environment, you can probably just run the following commands
from the library directory:

    cmake -DCMAKE_BUILD_TYPE:STRING=Release .
    make
    sudo make install

The library can be built either statically or dynamically linked. In order to facilitate
usage of the library with programs written in other languages, a C-compatible API is provided
as a wrapper for C++ classes. It forms a well-defined ABI as well as makes it easier to use
the library as a plugin loaded in run-time.

The build script recognises the following configuration options to control the build:

  * `libmt32emu_SHARED` - specifies whether to build a statically or dynamically linked library
  * `libmt32emu_C_INTERFACE` - specifies whether to include C-compatible API
  * `libmt32emu_CPP_INTERFACE` - specifies whether to expose C++ classes in the shared library
    (old-fashioned C++ API, compiler-specific ABI).

The options can be set in various ways:

  * specified directly as the command line arguments within the `cmake` command
  * by editing `CMakeCache.txt` file that CMake creates in the target directory
  * using *the CMake GUI*

By default, a shared library is created that exposes all the supported API.
However, the compiler optimisations are typically disabled. In order to get
a well-performing binary, be sure to set the value of the `CMAKE_BUILD_TYPE` variable
to Release or customise the compiler options otherwise.

Besides, an external sample rate conversion library may be used as an optional dependency
to facilitate converting the synthesiser output to any desired sample rate. By default,
an internal implementation provides this function. This can be overridden by disabling
the build option `libmt32emu_WITH_INTERNAL_RESAMPLER`. The following sample rate
conversion libraries are supported directly:

1) libsoxr - The SoX Resampler library - to perform fast and high quality sample rate conversion
   @ http://sourceforge.net/projects/soxr/

2) libsamplerate - Secret Rabbit Code - Sample Rate Converter that is widely available
   @ http://www.mega-nerd.com/SRC/


Hardware requirements
=====================

The emulation engine requires enough processing power from CPU to perform in real-time.
The exact minimum depends on many factors (e.g. CPU brand, amount of played MIDI messages,
type of audio card and so forth). Roughly, a 800 MHz Intel Pentium III CPU could suffice.
8MB of RAM is needed to run _mt32emu_qt_ with a single synth.


License
=======

Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher<br>
Copyright (C) 2011-2022 Dean Beeler, Jerome Fisher, Sergey V. Mikayev

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


Trademark disclaimer
====================

Roland is a trademark of Roland Corp. All other brand and product names are
trademarks or registered trademarks of their respective holder. Use of
trademarks is for informational purposes only and does not imply endorsement by
or affiliation with the holder.
