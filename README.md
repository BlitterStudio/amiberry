# Amiga emulator for Raspberry Pi

# History (newest first)
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

Binary package dependencies (install these if you only want to run the binary): 

      sudo apt-get install libsdl1.2debian libsdl-image1.2 libsdl-gfx1.2-5 libsdl-ttf2.0-0 libmpg123-0 libguichan-sdl-0.8.1-1 libxml2 pulseaudio

How to compile from source (on Raspbian Jessie):

   Install following packages:

      sudo apt-get install libsdl-dev libguichan-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libxml2-dev libflac-dev libmpg123-dev

   Then for Raspberry Pi 3:  

      make

   For Raspberry Pi 2:

      make PLATFORM=rpi2

   For Raspberry Pi 1/Zero:  

      make PLATFORM=rpi1
      
# uae4arm-rpi (Chips) history
Port of uae4arm on Raspberry Pi

v0.5:  
Merge of latest TomB version for Pandora.  
Picasso fully working.  
Keyboard management improved.  
Add deadzone for joystick.  

v0.4:  
Merge of latest TomB version for Pandora.  
Keep position between file selection   
Joystick management improved.  

v0.3:  
Rework of dispmanX management (huge picasso improvement).  
Add 4/3 shrink for 16/9 screen.  
Alt key can now be used to switch between mouse and 2nd joystick.  

v0.2:  
Merge latest TomB improvements from Pandora version (Zorro3 memory, picasso...).  

v0.1:  
Use dispmanX for fast scaling and double buffering.  
Enable hat usage on joystick.  
Add Sony 6axis joystick workaround.  
