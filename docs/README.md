# Amiga emulator for ARM boards

<a rel="me" href="https://mastodon.social/@midwan">Follow me Mastodon!</a>

[![C/C++ CI](https://github.com/midwan/amiberry/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/midwan/amiberry/actions/workflows/c-cpp.yml)

[![Gitter](https://badges.gitter.im/amiberry/Amiberry.svg)](https://gitter.im/amiberry/Amiberry?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

[![Backers on Open Collective](https://opencollective.com/amiberry/backers/badge.svg)](#backers) [![Sponsors on Open Collective](https://opencollective.com/amiberry/sponsors/badge.svg)](#sponsors)

![Amiberry logo](https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1)

Amiberry is an optimized Amiga emulator, primarily targeted for ARM-based boards (like the Raspberry Pi) but also ported on x86 (macOS, Linux).

The core emulation comes from [WinUAE](https://www.winuae.net), and the main GUI is designed to look similar to that. However, not all WinUAE features are implemented, as Amiberry tries to achieve a balance between good performance on low-powered hardware and emulation accuracy.

It includes JIT support, to get high-performance results on CPU-intensive emulated environments, like desktop applications. On top of that, there are some unique features developed specifically for Amiberry, such as the WHDLoad booter, support for RetroArch controller mapping, and several more.

## Requirements

### Linux

Amiberry has been tested on the following distros:

- Debian/RPI-OS Buster and Bullseye ARM and x86 (32-bit and 64-bit)
- Ubuntu ARM and x86 (64-bit)
- Manjaro ARM (64-bit)
- DietPi ARM (32-bit)
- RetroPie ARM (32-bit)
and several other distros.

Some even include it in their app ecosystem (e.g. DietPi, RetroPie and others), so you can install and upgrade it directly from their menu system.

### macOS

Amiberry has experimental support for macOS, and has been tested on:

- Catalina (x86)
- Monterey (x86 and M1)

You will need to install the required libraries using Homebrew.
If you want to compile it from source, please refer to the [relevant wiki page.](https://github.com/BlitterStudio/amiberry/wiki/Compiling-for-OSX)

### Dependencies

Amiberry requires the [SDL2 framework](https://libsdl.org) for graphics display, input handling and audio output. Additionally, a few extra libraries are used for CD32 MPEG and mp3 decoding.

If you just want to just run the Amiberry binary, you can install the required libraries on Debian/Raspbian/Ubuntu derived distros like this:

      sudo apt install libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-image-2.0-0 flac mpg123 libmpeg2-4

If you want to compile Amiberry from source, you'll need the `-dev` version of the same packages instead. For example, on Debian-based distros:

      sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev

Or if you're using an Arch-based distro (e.g. Manjaro), the relevant package names are these (these include the `dev` versions by default):

      sudo pacman -S base-devel sdl2 sdl2_ttf sdl2_image flac mpg123 libmpeg2

## Getting Amiberry

### Distro package management

Several popular distros (like RetroPie, DietPi, Amibian, Pimiga and others) already include Amiberry either pre-installed, or through their package management systems. Please follow the methods provided in those distros for a smoother experience, and refer to their owners for support during this process.

### Standalone binaries

The latest `stable` releases come with binaries for several different platforms, that you can download from the [Releases](https://github.com/midwan/amiberry/releases) area. If your platform is not included, then you will have to compile it yourself (or perhaps donate a board to me so I can include it!)

### Compile from source

Alternatively, you can of course compile the latest version of Amiberry from source yourself. To do that, follow these steps:

### First, clone this repository locally

      cd ~
      git clone https://github.com/BlitterStudio/amiberry
      cd amiberry

### Then, choose a platform to compile for

Amiberry's Makefile includes several targets, to cover various platforms.
You can specify your platform using the following syntax:

      make PLATFORM=<platform>

Where `<platform>` is one of the supported platforms.

Please consult the [relevant Wiki page](https://github.com/midwan/amiberry/wiki/Available-Platforms) for the full list of available platforms.
Alternatively, you can also check the Makefile itself for a full list of supported platforms.

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
