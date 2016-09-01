# Changes in forked version
- New target platform: Pi 3
- Optimizations for Pi 3 added
- Pi 3 is now the default target if no Platform is specified
- Added fixes for keyboard LEDs
- Code formatting and cleanup
- Merged improvements from Android port

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

   For Raspberry Pi 1:  

      make PLATFORM=rpi1
   
   For Generic SDL:  

      make PLATFORM=generic-sdl

How to compile on Raspbian Whezzy:  

   You need to install same packages as Jessie (see above)
   and a more updated gcc (otherwise alter Makefile).
   In order to do this enter the following in command line:  

      sudo apt-get install g++-4.8


   Then for Raspberry Pi 2:  

      make CXX=g++-4.8

   Or for Raspberry Pi 1:  

      make PLATFORM=rpi1 CXX=g++-4.8

