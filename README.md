Amiga emulator for the Raspberry Pi
=================================
# History (newest first)
- Ported to SDL2
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

To get full screen SDL2 support from the console on the Raspberry Pi, you will have to compile SDL2 from source. First you will need to get all the necessary tools:

      sudo apt-get update && sudo apt-get upgrade
      sudo apt-get install build-essential libfreeimage-dev libopenal-dev libpango1.0-dev libsndfile-dev libudev-dev libasound2-dev libjpeg8-dev libtiff5-dev libwebp-dev automake

Then download the SDL2 source tarball (currently on v2.0.5):

      cd ~ 
      wget https://www.libsdl.org/release/SDL2-2.0.5.tar.gz 
      tar zxvf SDL2-2.0.5.tar.gz 
      cd SDL2-2.0.5 && mkdir build && cd build

Next, configure SDL2 to use only the OpenGL ES backend directly from the console:

      ../configure --disable-pulseaudio --disable-esd --disable-video-mir --disable-video-wayland --disable-video-x11 --disable-video-opengl

Finally, compile and install SDL2:

      make -j 4 && sudo make install

With SDL2 installed, you can proceed to install Amiberry as follows:

   Install the following packages:

      sudo apt-get install libsdl2-image-dev libsdl2-ttf-dev libxml2-dev libflac-dev libmpg123-dev

   Then for Raspberry Pi 3:  

      make

   For Raspberry Pi 2:

      make PLATFORM=rpi2

   For Raspberry Pi 1:  

      make PLATFORM=rpi1

