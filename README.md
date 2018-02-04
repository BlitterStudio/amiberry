# Amiga emulator for the Raspberry Pi and other ARM SoC

# Pre-requisites
Please check the relevant Wiki section: [First installation](https://github.com/midwan/amiberry/wiki/First-Installation)

# Compiling Amiberry
Clone this repo:
      
      cd ~
      git clone https://github.com/midwan/amiberry
      cd amiberry
      
The default platform is currently "rpi3", so for Raspberry Pi 3 (SDL1) you can just type:

      make -j2

For Raspberry Pi 2 (SDL1):

      make -j2 PLATFORM=rpi2

And for the SDL2 versions, you can use the following:

      make -j2 PLATFORM=rpi3-sdl2-dispmanx

Or for Raspberry Pi 2 (SDL2):

      make -j2 PLATFORM=rpi2-sdl2-dispmanx
      
You can check the Makefile for a full list of supported platforms!
