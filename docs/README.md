# Amiga emulator for ARM boards

[![Build Status](https://dev.azure.com/BlitterStudio/Amiberry/_apis/build/status/azure-pipelines/amiberry?branchName=master)](https://dev.azure.com/BlitterStudio/Amiberry/_build/latest?definitionId=4&branchName=master)

[![Gitter](https://badges.gitter.im/amiberry/Amiberry.svg)](https://gitter.im/amiberry/Amiberry?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

[![Backers on Open Collective](https://opencollective.com/amiberry/backers/badge.svg)](#backers) [![Sponsors on Open Collective](https://opencollective.com/amiberry/sponsors/badge.svg)](#sponsors)

![](https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1)

Amiberry is an optimized Amiga emulator, for ARM-based boards (like the Raspberry Pi, ASUS Tinkerboard, Odroid XU4, etc). The core emulation comes from WinUAE, but stripped down somewhat in order to achieve good performance in underpowered boards. It includes JIT CPU and FPU support, to get high-performance results on CPU-intensive emulated environments. On top of that, we have some unique features developed specifically for Amiberry, such as the WHDLoad booter and support for RetroArch controller mapping.

Amiberry requires the [SDL2 framework](https://libsdl.org) for graphics display, input handling and audio output. On the 32-bit RPI platform specifically, we offer a special alternative version which uses Dispmanx directly for the emulation screen, for maximum performance.

## Requirements

Amiberry has been tested on Debian/Raspbian Buster, and requires the following packages to run:

      sudo apt-get install libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-image-2.0-0 flac mpg123 libmpeg2-4

If you want to compile Amiberry from source, you'll need the `-dev` version of the same packages instead:

      sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev

On the Raspberry Pi, if you're not running Raspbian you'll also need this:

      sudo apt-get install libraspberrypi-dev

Or if you're using an Arch-based distro (e.g. Manjaro), you can use these instead:

      sudo pacman -S base-devel sdl2 sdl2_ttf sdl2_image flac mpg123 libmpeg2

## Getting Amiberry

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

For the Raspberry Pi 4 64-bit

      make -j2 PLATFORM=pi64

For the Raspberry Pi 3(B+)

      make -j2 PLATFORM=rpi3-sdl2

For the Raspberry Pi 2

      make -j2 PLATFORM=rpi2-sdl2

For the Raspberry Pi 1/Zero

      make PLATFORM=rpi1-sdl2

For the Odroid XU4

      make -j6 PLATFORM=xu4

For the ASUS Tinker board (Supported distro: Armbian)

      make -j6 PLATFORM=RK3288

For the Odroid C1

      make -j4 PLATFORM=c1

For the Vero 4k

      make -j4 PLATFORM=vero4k

For the OrangePi PC

      make -j2 PLATFORM=orangepi-pc

For the Odroid N2/RockPro64

      make -j2 PLATFORM=n2

You can check the Makefile for a full list of supported platforms!

For more documentation subjects, please check the [Wiki page](https://github.com/midwan/amiberry/wiki)

## Contributors

This project exists thanks to all the people who contribute. [[Contribute]](../.github/CONTRIBUTING.md).
<a href="graphs/contributors"><img src="https://opencollective.com/amiberry/contributors.svg?width=890" /></a>

## Backers

Thank you to all our backers! 🙏 [[Become a backer](https://opencollective.com/amiberry#backer)]
<a href="https://opencollective.com/amiberry#backers" target="_blank"><img src="https://opencollective.com/amiberry/backers.svg?width=890"></a>

## Sponsors

Support this project by becoming a sponsor. Your logo will show up here with a link to your website. [[Become a sponsor](https://opencollective.com/amiberry#sponsor)]
<a href="https://opencollective.com/amiberry/sponsor/0/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/0/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/1/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/1/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/2/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/2/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/3/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/3/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/4/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/4/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/5/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/5/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/6/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/6/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/7/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/7/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/8/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/8/avatar.svg"></a>
<a href="https://opencollective.com/amiberry/sponsor/9/website" target="_blank"><img src="https://opencollective.com/amiberry/sponsor/9/avatar.svg"></a>

