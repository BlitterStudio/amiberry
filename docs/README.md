# Amiga emulator for ARM boards

<a rel="me" href="https://mastodon.social/@midwan">Follow me Mastodon!</a>

[![C/C++ CI](https://github.com/midwan/amiberry/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/midwan/amiberry/actions/workflows/c-cpp.yml)

[![Gitter](https://badges.gitter.im/amiberry/Amiberry.svg)](https://gitter.im/amiberry/Amiberry?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

[![Backers on Open Collective](https://opencollective.com/amiberry/backers/badge.svg)](#backers) [![Sponsors on Open Collective](https://opencollective.com/amiberry/sponsors/badge.svg)](#sponsors)

![Amiberry logo](https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1)

Amiberry is an optimized Amiga emulator, for ARM32, ARM64, x86_64 (macOS and Linux) and RISC-V platforms.

The core emulation comes from [WinUAE](https://www.winuae.net), and the main GUI is designed to look similar to that. However, not all WinUAE features are implemented, as Amiberry tries to achieve a balance between good performance on low-powered hardware and emulation accuracy.

It includes JIT support, to get high-performance results on CPU-intensive emulated environments, like desktop applications. On top of that, there are some unique features developed specifically for Amiberry, such as the WHDLoad booter, support for RetroArch controller mapping, and several more.

There are currently two Amiberry versions available: v5.x and v6.x - see [here](https://github.com/BlitterStudio/amiberry/wiki/First-Installation) to see which is the best pick for your needs.

## Requirements

### Linux

Amiberry has been tested on the following Linux distros:

- Debian/RPI-OS Bullseye, Bookworm ARM32, ARM64 and x86_64
- Ubuntu ARM64 and x86_64
- Manjaro ARM and x86_64
- DietPi
- RetroPie
it should also work on several others, as long as the requirements are met.

Some distros include it in their app ecosystem (e.g. DietPi, RetroPie and others), so you can install and upgrade it directly from their menu system.

### macOS

Amiberry supports macOS, and has been tested on:

- Catalina (x86_64)
- Monterey (x86_64 and Apple Silicon)
- Newer macOS versions should work as well

Under macOS, you will need to install the required libraries using Homebrew.
If you want to compile it from source, please refer to the [relevant wiki page.](https://github.com/BlitterStudio/amiberry/wiki/Compiling-for-macOS)

### Dependencies

Amiberry requires the [SDL2 framework](https://libsdl.org) for graphics display, input handling and audio output. Additionally, a few extra libraries are used for CD32 MPEG and mp3 decoding.

If you just want to just run the Amiberry binary, you can install the required libraries on Debian/Raspbian/Ubuntu derived distros like this:

      sudo apt install cmake libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-image-2.0-0 flac mpg123 libmpeg2-4 libserialport0 libportmidi0

If you want to compile Amiberry from source, you'll need the `-dev` version of the same packages instead. For example, on Debian-based distros:

      sudo apt install cmake libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev libserialport-dev libportmidi-dev

Or if you're using an Arch-based distro (e.g. Manjaro), the relevant package names are these (these include the `dev` versions by default):

      sudo pacman -S base-devel cmake sdl2 sdl2_ttf sdl2_image flac mpg123 libmpeg2 libserialport portmidi

if you have an AUR helper like [yay](https://github.com/Jguer/yay), you can build and run [Amiberry](https://aur.archlinux.org/packages/amiberry) through:

      yay -S amiberry

Additionally, please note that you will probably also need some Kickstart ROMs. Amiberry includes the free AROS ROM, so you can start it up and use AROS with it directly, but most games will require a Kickstart 1.3 (for A500 emulation) or Kickstart 3.x (for A1200 emulation). Amiga Forever is a good source for those.

## Getting Amiberry

### Distro package management

Several popular distros (like RetroPie, DietPi, Pimiga and others) already include Amiberry either pre-installed, or through their package management systems. Please follow the methods provided in those distros for a smoother experience, and refer to their owners for support during this process.

### Flatpak

A flatpak version is available on [Flathub](https://flathub.org/apps/com.blitterstudio.amiberry)

### Standalone binaries

The latest `stable` releases come with binaries for several different platforms, that you can download from the [Releases](https://github.com/midwan/amiberry/releases) area. If your platform is not included, or if you want to test a newer version than the stable release, then you will have to compile it yourself. Read on to see how to do that.

### Compile from source

Alternatively, you can of course compile the latest version of Amiberry from source yourself. To do that, follow these steps:

### First, clone this repository locally

      cd ~
      git clone https://github.com/BlitterStudio/amiberry
      cd amiberry

### Then, choose a platform to compile for

Amiberry's Makefile includes several targets, to cover various platforms.
You will need to specify your platform using the following syntax:

      make PLATFORM=<platform>

Where `<platform>` is one of the supported platforms. 

If you have more than 1GB of RAM, you can also use multiple CPU cores to compile it faster, by adding `-j<cores>`, where `<cores>` is the number of CPU cores you want to use. For example, on a Raspberry Pi 4 (32-bit) with at least 2GB of RAM, you can use all four CPU cores with the following:

      make -j4 PLATFORM=rpi4

Please consult the [relevant Wiki page](https://github.com/BlitterStudio/amiberry/wiki/Available-Platforms) for the full list of available platforms, as there are many (and separate for 32-bit and 64-bit ones).
Alternatively, you can also check the Makefile itself for a full list of supported platforms.

For more documentation subjects, please check the [Wiki page](https://github.com/BlitterStudio/amiberry/wiki)
