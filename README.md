# Changes in forked version

- New target platform: Pi 3
- Optimizations for Pi 3 added
- Pi 3 is now the default target if no Platform is specified
- Added support for custom functions assignable to keyboard LEDs (e.g. HD activity)
- Code formatting and cleanup
- FullHD (1080p) resolution supported in Picasso96 mode on all Pi models
- Pi Zero / Pi 1 version now has full Picasso96 support (up to 1080p 24bit)
- Removed Pandora specific keyboard shortcuts which caused crashes
- Loading the Configuration file now respects the input settings
- Fixed bugs and crashes in GUI keyboard navigation
- Added Visual Studio solution (requires VisualGDB), so we can compile and debug from Windows PC
- Added Shutdown button, to power off the computer
- Added mapping option for game controller button to a) Enter GUI and b) Quit the emulator
- Added mapping option for keyboard key to Quit the emulator directly

# uae4arm-rpi
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


How to compile on Raspbian Jessie:

   Install following packages:

      sudo apt-get install libsdl-dev libguichan-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libxml2-dev libflac-dev libmpg123-dev

   Then for Raspberry Pi 3:  

      make

   For Raspberry Pi 2:

      make PLATFORM=rpi2

   For Raspberry Pi 1/Zero:  

      make PLATFORM=rpi1
