Amiga emulator for the Raspberry Pi
=================================
Warning: this branch is still Work In Progress - it requires a few extra steps to build and some things may not be finished yet! :)
If you're looking for the latest "stable" version, please use the master branch for now.
Once this branch is complete, it will be merged back to the master and replace it.

# History (newest first)
- Added GPerfTools for profiling and optimized malloc functions (note: this adds 2 extra dependencies, check below)
- Added an option for choosing Scaling Method (for non-Picasso modes): Auto, Nearest Neighbor (pixelated) or Linear (smooth). Auto will automatically choose between the other two modes on the fly, depending on the Amiga resolution requested and if the native monitor resolution can display it as an exact multiple or not. This vastly improves the sharpness of the resulting image.
- Improved image centering (for non-Picasso modes)
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

# Compiling SDL2
Unfortunately we need to compile SDL2 from source on the Raspberry Pi, to get support for launching full screen applications from the console. Luckily, the RetroPie project has a handy script that takes care of downloading, compiling and installing SDL2 for us. This currently only handles SDL2, so we still need SDL2_ttf and SDL2_image (see below).

Run these to clone the RetroPie-Setup repo, then run the relevant script:

      git clone https://github.com/RetroPie/RetroPie-Setup
      cd RetroPie-Setup
      sudo ./retropie_packages.sh sdl2

Next, we need SDL2_image and SDL2_ttf:

      sudo apt-get install libsdl2-image-dev libsdl2-ttf-dev 

With SDL2 installed, you can proceed to install Amiberry as follows:

Install the following packages:

      sudo apt-get install libxml2-dev libflac-dev libmpg123-dev libpng-dev libmpeg2-4-dev

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

