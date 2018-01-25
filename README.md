# Amiga emulator for the Raspberry Pi

Warning: this branch is still Work In Progress - If you're looking for the latest "stable" version, please use the master branch for now.

# History (newest first)
- Added Line Doubling mode for Interlace resolutions
- Added multi-threaded drawing routines to improve performance
- Improved emulation accuracy
- [SDL2] Added an option for choosing Scaling Method (for non-Picasso modes): Auto, Nearest Neighbor (pixelated) or Linear (smooth). Auto will automatically choose between the other two modes on the fly, depending on the Amiga resolution requested and if the native monitor resolution can display it as an exact multiple or not. This vastly improves the sharpness of the resulting image.
- [SDL2] Improved image centering (for non-Picasso modes)
- [SDL2] Ported to SDL2
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
      git clone https://github.com/midwan/amiberry -b dev amiberry-dev
      cd amiberry-dev
      
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
