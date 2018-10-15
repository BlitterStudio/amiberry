# Amiga emulator for the Raspberry Pi and other ARM SoC
Amiberry is an optimized Amiga emulator, for ARM-based boards (like the Raspberry Pi, ASUS Tinkerboard, Odroid XU4, etc). The core emulation comes from WinUAE, but stripped down to remove excess stuff in order to achieve good performance in underpowered boards. It includes ARM JIT support, to get high-performance results on CPU-intensive emulated environments. On top of that, we have some unique features developed only for Amiberry.

Amiberry requires the SDL framework (https://libsdl.org) for graphics display, input handling and audio out. Currently both SDL1.x and SDL2 are supported, but in the future we'll probably drop support for SDL1 as things move forward.

# Compiling SDL2
If you want to run the SDL2 version, you currently need to compile SDL2 from source on the Raspberry Pi, to get support for launching full screen applications from the console. The version bundled with Raspbian Stretch is unfortunately not compiled with support for the "rpi" driver, so it only works under X11. Furthermore, it is an older version and includes some known bugs, which were fixed in later updates of SDL2.

Follow these steps to download, compile and install SDL2 from source:

https://github.com/midwan/amiberry/wiki/Compile-SDL2-from-source
      
With SDL2 installed, you can proceed to install Amiberry as follows:

# Pre-requisites
Amiberry requires the following packages, besides SDL (v1 or v2) itself:

      sudo apt-get install libxml2-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev

# Getting Amiberry
You can clone this repo, like so:
      
      cd ~
      git clone https://github.com/midwan/amiberry
      cd amiberry
      
# Choosing a target
Amiberry's makefile includes several targets, to cover various platforms. For the Raspberry Pi platform specifically, we offer multiple combinations to get the best performance out of the platform, depending on the environment used:
- SDL1 with DispmanX back-end for graphics
- SDL2 with DispmanX back-end for graphics
- SDL2 with whatever back-end it was configured with (e.g. KMS, OpenGL, X11, etc.)

For the Raspberry Pi specifically, the best performance can be gained by using the DispmanX back-end, because none of the other options offer multi-threaded graphics updates, unfortunately. For other platforms (e.g. using the Mali GPU), multi-threaded updates can be supported using pure SDL2 (and DispmanX is an RPI-only feature anyway).

# Compiling a target
## Using SDL1 + DispmanX
For the Raspberry Pi 3(B+) (SDL1 + Dispmanx), type:

      make all

For the Raspberry Pi 2 (SDL1 + Dispmanx), type:

      make all PLATFORM=rpi2

For the Raspberry Pi 1/Zero (SDL1 + Dispmanx), type:

      make all PLATFORM=rpi1

## Using SDL2 + DispmanX

For the Raspberry Pi 3(B+) (SDL2)

      make all PLATFORM=rpi3-sdl2-dispmanx

For the Raspberry Pi 2 (SDL2):

      make all PLATFORM=rpi2-sdl2-dispmanx
      
For the Raspberry Pi 1/Zero (SDL2):

      make all PLATFORM=rpi1-sdl2-dispmanx

## Using SDL2 + whatever back-end it was configured with

For the Raspberry Pi 3(B+) (SDL2)

      make all PLATFORM=rpi3-sdl2

For the Raspberry Pi 2 (SDL2):

      make all PLATFORM=rpi2-sdl2
      
For the Raspberry Pi 1/Zero (SDL2):

      make all PLATFORM=rpi1-sdl2

For the Odroid XU4 (SDL2):

      make all PLATFORM=xu4
      
For the ASUS Tinker board (SDL2):

      make all PLATFORM=tinker
      
For the Odroid C1 (SDL2):

      make all PLATFORM=c1

For the Vero 4k (SDL2):

      make all PLATFORM=vero4k
     
For the OrangePi PC (SDL2):

      make all PLATFORM=orancepi-pc
      
You can check the Makefile for a full list of supported platforms!

For more documentation subjects, please check the [Wiki page](https://github.com/midwan/amiberry/wiki)
