# Amiga emulator for the Raspberry Pi and other ARM SoC

Warning: this branch is where Development takes place. It may be unstable, crash, not work from time to time - If you're looking for the latest "stable" version, please use the master branch for now.

# Compiling SDL2
If you want to run the SDL2 version, you currently need to compile SDL2 from source on the Raspberry Pi, to get support for launching full screen applications from the console. The version bundled with Stretch is not compiled with support for the "rpi" driver, so it only works under X11.

Follow these steps to download, compile and install SDL2 from source:

https://github.com/midwan/amiberry/wiki/Compile-SDL2-from-source
      
With SDL2 installed, you can proceed to install Amiberry as follows:

# Pre-requisites
Install the following packages:

      sudo apt-get install libxml2-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev

# Compiling Amiberry
Clone this repo:
      
      cd ~
      git clone https://github.com/midwan/amiberry
      cd amiberry
      
The default platform is currently "rpi3", so for Raspberry Pi 3 (SDL1) you can just type:

      make all

For Raspberry Pi 2 (SDL1):

      make all PLATFORM=rpi2

For Raspberry Pi 1/Zero (SDL1):  

      make all PLATFORM=rpi1

And for the SDL2 versions, you can use the following:

      make all PLATFORM=rpi3-sdl2

Or for Raspberry Pi 2 (SDL2):

      make all PLATFORM=rpi2-sdl2
      
Or for Raspberry Pi 1/Zero (SDL2):

      make all PLATFORM=rpi1-sdl2

You can check the Makefile for a full list of supported platforms!
