# Optimized Amiga emulator for multiple platforms

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/X8X4FHDY4)

<a rel="me" href="https://mastodon.social/@midwan">Follow me Mastodon!</a>

[![C/C++ CI](https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/BlitterStudio/amiberry/actions/workflows/c-cpp.yml)

[![Discord](https://img.shields.io/badge/My-Discord-%235865F2.svg)](https://discord.gg/wWndKTGpGV)

![Amiberry logo](https://i2.wp.com/blitterstudio.com/wp-content/uploads/2020/01/Logo-v3-1.png?resize=768%2C543&ssl=1)

Amiberry is an optimized Amiga emulator for ARM32, ARM64, x86_64 and RISC-V platforms. It runs on both macOS and Linux. Windows users can always use [WinUAE](https://www.winuae.net).

The core emulation comes from [WinUAE](https://www.winuae.net), and even the main GUI is designed to look similar to that.
It includes JIT support, to get high-performance results on CPU-intensive emulated environments, like desktop applications. On top of that, there are some unique features developed specifically for Amiberry, such as the WHDLoad booter, support for RetroArch controller mapping, and several more.

There are currently two Amiberry versions available: **Amiberry and Amiberry-Lite** - [see here](https://github.com/BlitterStudio/amiberry/wiki/First-Installation) to see which is the best pick for your needs.

## Platforms

### Linux

Amiberry has been tested on the following Linux distros:

- Debian/RPI-OS Bullseye, Bookworm ARM32, ARM64 and x86_64
- Ubuntu ARM64 and x86_64
- Manjaro ARM64 and x86_64
- DietPi
- RetroPie
- Batocera
- Knulli
- MuOS
- TheRA
it should also work on several others, as long as the requirements are met.

Some distros include it in their app ecosystem (e.g. DietPi, RetroPie and others), so you can install and upgrade it directly from their menu system.

### macOS

Amiberry also runs on macOS, and has been tested on:

- Catalina (x86_64)
- Monterey (x86_64 and Apple Silicon)
- Sonoma (x86_64 and Apple Silicon)
- Sequoia (x86_64 and Apple Silicon)
- Newer macOS versions _should_ work as well

## Dependencies

Please note that you will probably also need some Kickstart ROMs. Amiberry includes the free AROS ROM, so you can start it up and use AROS with it directly, but most games will require a Kickstart 1.3 (for A500 emulation) or Kickstart 3.x (for A1200 emulation). Amiga Forever is a good source for those.

### Linux

Amiberry requires the [SDL2 framework](https://libsdl.org) for graphics display, input handling and audio output. Additionally, a few extra libraries are used for CD32 MPEG and mp3 decoding.

If you just want to just run the Amiberry binary, you can install the required libraries on Debian/Raspbian/Ubuntu derived distros like this:

      sudo apt install cmake libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-image-2.0-0 flac mpg123 libmpeg2-4 libserialport0 libportmidi0

if you have an AUR helper like [yay](https://github.com/Jguer/yay), you can build and run [Amiberry](https://aur.archlinux.org/packages/amiberry) through:

      yay -S amiberry

### macOS

The macOS application bundle includes all the required libraries, so you don't need to install anything extra.

## Getting Amiberry

### DEB packages

Amiberry is available as a DEB package for ARM32, ARM64 and x86_64 Debian-based Linux platforms. You can download the latest version from the [Releases](https://github.com/BlitterStudio/amiberry/releases) area.
Then you can install it with:
    
        sudo dpkg -i amiberry_7.0.0_arm64.deb

### macOS

Amiberry is available as a DMG package for macOS. You can download the latest version from the [Releases](https://github.com/BlitterStudio/amiberry/releases) area.
After installing it in your Applications folder, you'll need to open a console and run `xattr -rd com.apple.quarantine Amiberry.app`, to whitelist it and allow it to run.

### Distro package management

Several popular distros (like RetroPie, DietPi, Pimiga and others) already include Amiberry either pre-installed, or through their package management systems. Please follow the methods provided in those distros for a smoother experience, and refer to their owners for support during this process.

### Flatpak

A flatpak version is available on [Flathub](https://flathub.org/apps/com.blitterstudio.amiberry)

## Compile from source

Alternatively, you can of course compile the latest version of Amiberry from source yourself. To do that, follow these steps:

### Linux

You'll need the `-dev` version of the required packages. For example, on Debian-based distros:

      sudo apt install cmake libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev libserialport-dev libportmidi-dev

Or if you're using an Arch-based distro (e.g. Manjaro), the relevant package names are these (these include the `dev` versions by default):

      sudo pacman -S base-devel cmake sdl2 sdl2_ttf sdl2_image flac mpg123 libmpeg2 libserialport portmidi

### macOS

Under macOS, you will need to install the required libraries using [Homebrew](https://docs.brew.sh/Installation):

        brew install cmake sdl2 mpg123 sdl2_ttf sdl2_image flac libmpeg2 libserialport portmidi dylibbundler wget

### Clone this repository locally

      git clone https://github.com/BlitterStudio/amiberry
      cd amiberry

### Build Amiberry

Amiberry requires CMake to build. The following commands will create a `build` directory, configure the build, and then compile the source code:

      cmake -B build && cmake --build build

You can also use Ninja, if you prefer:

      cmake -B build -G Ninja && cmake --build build

### Packaging

The following will create a DEB package for Debian-based distros, or a DMG package for macOS:

    cpack --config build/CPackConfig.cmake

For more documentation subjects, please check the [Wiki page](https://github.com/BlitterStudio/amiberry/wiki)
