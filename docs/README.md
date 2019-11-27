<img src="resources/icon.png" align="right" />

# Amiga emulator for the Raspberry Pi and other ARM SoC

Amiberry is an optimized Amiga emulator, for ARM-based boards (like the Raspberry Pi, ASUS Tinkerboard, Odroid XU4, etc). The core emulation comes from WinUAE, but stripped down somewhat in order to achieve good performance in underpowered boards. It includes JIT CPU and FPU support, to get high-performance results on CPU-intensive emulated environments. On top of that, we have some unique features developed only for Amiberry, such as the WHDLoad booter and support for RetroArch controller mapping.

Amiberry requires the [SDL2 framework](https://libsdl.org) for graphics display, input handling and audio output. On the RPI platform specifically, we offer a special alternative version which uses Dispmanx directly for the emulation screen, for maximum performance.

# Donations

If you like this project and would like to contribute with a donation, you can use this PayPal pool: [Amiberry PayPal pool](https://paypal.me/pools/c/8apqkBQovm)

Donations would help with porting Amiberry to new devices, keeping the motivation going to spend those countless hours on the project, and perhaps getting us some chocolate.

# Requirements

Amiberry has been tested on Debian/Raspbian Buster, and requires the following packages to run:

      sudo apt-get install libsdl2 libsdl2-ttf libsdl2-image libxml2 libflac libmpg123 libpng libmpeg2-4

If you want to compile Amiberry from source, you'll need the `-dev` version of the same packages instead:

      sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libxml2-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev

On the Raspberry Pi, if you're not running Raspbian you'll also need this:

      sudo apt-get install libraspberrypi-dev

# Getting Amiberry

The latest stable releases come with binaries, that you can download from the [Releases](https://github.com/midwan/amiberry/releases) area.
Several popular distros (like RetroPie, DietPi, Amibian, and others) already include Amiberry either pre-installed, or through their package management systems.
Please follow the methods provided in those distros for a smoother experience.

Alternatively, you can compile the latest from source yourself. Clone this repo, like so:

      cd ~
      git clone https://github.com/midwan/amiberry
      cd amiberry

## Choosing a target

Amiberry's Makefile includes several targets, to cover various platforms. For the Raspberry Pi platform specifically, we offer a special Dispmanx version for maximum performance:

- SDL2 with DispmanX back-end for graphics (RPI platforms only)
- SDL2 with whatever back-end it was configured with (e.g. KMS, OpenGL, X11, etc.)

## Compiling a target

You must specify the Platform you want to build as a parameter to the `make` command. The process will abort if you use an incorrect platform or no platform at all.
Some example platforms are shown below, but feel free to explore the full list within the Makefile.

### Raspberry Pi with SDL2 + DispmanX

Use the following command line options for compiling the special RPI-specific versions with Dispmanx:

For the Raspberry Pi 4

      make -j2 PLATFORM=rpi4

For the Raspberry Pi 3(B+)

      make -j2 PLATFORM=rpi3

For the Raspberry Pi 2

      make -j2 PLATFORM=rpi2

For the Raspberry Pi 1/Zero

      make PLATFORM=rpi1

### SDL2 Platforms

Below is an example list of several available targets in the Makefile, and what command line option to use when compiling for them:

For the Raspberry Pi 4

      make -j2 PLATFORM=rpi4-sdl2

For the Raspberry Pi 3(B+)

      make -j2 PLATFORM=rpi3-sdl2

For the Raspberry Pi 2

      make -j2 PLATFORM=rpi2-sdl2

For the Raspberry Pi 1/Zero

      make PLATFORM=rpi1-sdl2

For the Odroid XU4

      make -j6 PLATFORM=xu4

For the ASUS Tinker board

      make -j6 PLATFORM=RK3328

For the Odroid C1

      make -j4 PLATFORM=c1

For the Vero 4k

      make -j4 PLATFORM=vero4k

For the OrangePi PC

      make -j2 PLATFORM=orangepi-pc

You can check the Makefile for a full list of supported platforms!

For more documentation subjects, please check the [Wiki page](https://github.com/midwan/amiberry/wiki)
