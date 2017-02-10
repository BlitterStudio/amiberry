Amiga emulator for the Raspberry Pi
=================================
Warning: this branch is still Work In Progress - it requires a few extra steps to build and some things may not be finished yet! :)
If you're looking for the latest "stable" version, please use the master branch for now.
Once this branch is complete, it will be merged back to the master and replace it.

# History (newest first)
- Ported to SDL2
- Added new Picasso resolutions
- Added NetBeans project
- Added Visual Studio solution using VC++ for Linux
- Fixed bugs related to video and audio glitches
- Renamed folder structure according to the WinUAE standard
- The emulator now changes screen resolution on the host dynamically instead of always scaling to the native one (improves performance a lot)
- Added mapping option for keyboard key to Quit the emulator directly
- Added mapping option for game controller button to a) Enter GUI and b) Quit the emulator
- Added Shutdown button, to power off the (host) computer
- Added Visual Studio solution (requires VisualGDB), so we can compile and debug from Windows PC
- Fixed bugs and crashes in GUI keyboard navigation
- Loading the Configuration file now respects the input settings
- Removed Pandora specific keyboard shortcuts which caused crashes
- Pi Zero / Pi 1 version now has full Picasso96 support (up to 1080p 24bit)
- FullHD (1080p) resolution supported in Picasso96 mode on all Pi models
- Code formatting and cleanup
- Added support for custom functions assignable to keyboard LEDs (e.g. HD activity)
- Pi 3 is now the default target if no Platform is specified
- Optimizations for Pi 3 added
- New target platform: Pi 3

# Compiling from source
To get full screen SDL2 support from the console on the Raspberry Pi, you will have to compile SDL2 from source. That is because the OpenGL driver is on higher priority than the OpenGL ES2 one on the Pi, and the OpenGL driver requires X11. If you don't mind that and you want to run the emulator under X11 only, then you can skip this step and install SDL2 from the package manager instead. Otherwise, you can just copy paste the following commands in your terminal. 

First you will need to get all the necessary tools and requirements for SDL2:

      sudo apt-get update && sudo apt-get upgrade
      sudo apt-get install build-essential libfreeimage-dev libopenal-dev libpango1.0-dev libsndfile-dev libudev-dev libasound2-dev libtiff5-dev libwebp-dev automake libegl1-mesa-dev libgl1-mesa-dev libgles2-mesa-dev

Then download the SDL2 source tarball (currently on v2.0.5):

      cd ~ 
      wget https://www.libsdl.org/release/SDL2-2.0.5.tar.gz 
      tar zxvf SDL2-2.0.5.tar.gz 
      cd SDL2-2.0.5 && mkdir build && cd build

Next, configure SDL2 to disable OpenGL, in order to be able to open screens directly from the console using OpenGL ES2 instead:

      ../configure --disable-pulseaudio --disable-esd --disable-video-x11 --disable-video-opengl

Compile and install SDL2:

      make -j 4 && sudo make install

Next, we need SDL2_image (currently v2.0.1):

      cd ~ 
      wget https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.1.tar.gz
      tar zxvf SDL2_image-2.0.1.tar.gz 
      cd SDL2_image-2.0.1 && mkdir build && cd build 
      ../configure 
      make -j 4 
      sudo make install 

Next, SDL2_ttf (currently v2.0.14):

      cd ~
      wget https://libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.14.tar.gz
      tar zxvf SDL2_ttf-2.0.14.tar.gz
      cd SDL2_ttf-2.0.14 && mkdir build && cd build
      ../configure
      make -j 4
      sudo make install

Finally, SDL2_mixer (currently v2.0.1):

      cd ~
      wget https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-2.0.1.tar.gz
      tar zxvf SDL2_mixer-2.0.1.tar.gz
      cd SDL2_mixer-2.0.1 && mkdir build && cd build
      ../configure
      make -j 4
      sudo make install
      
With SDL2 installed, you can proceed to install Amiberry as follows:

Install the following packages:

      sudo apt-get install libxml2-dev libflac-dev libmpg123-dev

Clone this repo:
      
      cd ~
      git clone https://github.com/midwan/amiberry -b sdl2 amiberry-sdl2
      cd amiberry-sdl2
      
Then for Raspberry Pi 3:  

      make all

For Raspberry Pi 2:

      make all PLATFORM=rpi2

For Raspberry Pi 1:  

      make all PLATFORM=rpi1

