UAE4ARM - Pandora edition

Version 1.0.0.0

Thanks to all for working on the different versions of UAE this emulator based on:
 - Toni Wilen
 - Chui for the UAE4all emulator
 - Notaz for the GP2X port
 - Pickle for the Wiz port
 - john4p for his work on the Pandora version
 - tuki_cat for improved keyboard gfx and betatesting
 - lubomyr for the Android version
 - all people from aranym project for the ARM JIT


Description

A fast and optimized Amiga Emulator

Features: AGA/OCS/ECS, 68000, 68020 and 68040 emulation, harddisk-support, WHDLoad-support, Chip/Slow/Fast-mem settings, 
savestates, vsync, most games (not AGA) run fullspeed at 600 MHz with no frameskip and lot of AGA games run fullspeed 
at 1000 MHz, smooth 50Hz scrolling, custom controls, custom screen modes, screen positioning, heavy/medium/light manual 
autofire, nub mouse input, support for stylus input

This version based on UAE4ALL for Pandora. Instead of implement the changes in UAE4ALL, I decided to start a new project.
UAE4ALL is in a stable state and there are fundamental changes like switching to a new 68000 core for the new version
which may cause new bugs, performance losses and breaks compatibility to few games.

The main differences to UAE4ALL:
 - Different 68000 core (newcpu instead of FAME/C) with support for 32 bit addressing mode
 - Support for 68030 and 68040 cpus
 - FPU (68881, 68882 and internal 68040)
 - JIT
 - Autodetect Amiga ROMs
 - Up to 5 HDDs
 - Lot of new audio options
 - Picasso96
 - GUI
 
What's missing compared to WinUAE:
 - Cycle exact emulation of cpu and blitter
 - SCSI support
 - bsdsockets
 - AHI
 - CD
 - Builtin debugger
 - RDB-harddisks
and some more...
 

Index

1. Setup

2. Controls
   2.1 General and quick keys (outside of GUI)
   2.2 Joystick mode
   2.3 Mouse mode
   2.4 Stylus mode
   2.5 Specific controls inside GUI

3. GUI
   3.1 Paths
   3.2 Configurations
   3.3 CPU and FPU
   3.4 Chipset
   3.5 ROM
   3.6 RAM
   3.7 Floppy drives
   3.8 Hard drives
   3.9 Display
   3.10 Sound
   3.11 Input
   3.12 Miscellaneous
   3.13 Savestates
   

------------------------------------

1. Setup

Place the UAE4ALL PND in the required directory (menu, desktop or apps). Running it for the first time will create the 
necessary folders in pandora/appdata on you SD card.

In order to use UAE4ALL you need an Amiga Bios image (kickstart rom). Place all your roms and the keyfiles (if required)
in the appdata folder of UAE4ARM and select this folder with the GUI (goto page Paths, choose folder in option "System ROMS"
and click on "Rescan ROMs").


------------------------------------

2. Controls

2.1 General & Quick Keys (outside of GUI)

SELECT: Open GUI
START: Toggle joystick/mouse/stylus mode
Pandora Key: pressing this at any time (inside or outside the GUI) and it will minimise UAE4AALL
Hold L+R + D-pad up/down: Move displayed gfx vertically
Hold L+R + 1/2/3/4/5/6/7/8: select preset screenmode
Hold L+R + 9/0: set number of displayed lines
Hold L+R + w: change Amiga displayed width
L+c: Toggle custom controls
L+d: Toggle status line
L+f: Toggle frameskip
L+l: Quick load
L+s: Quick save
R+D-pad: Arrow keys in all control modes


2.2 Joystick Mode

D-pad: Joystick movement
A/X: Joystick fire button (depends which button mode you are in)
X/A: Up on joystick (used as jump in platform games, etc. depends which button mode you are in)
B: mapped to the space bar
Y: Space bar
L: Alt
L+D-pad: Move mouse pointer
L+A: Left mouse click
L+B: Right mouse click
R+D-pad: Arrow keys
R+Y: Toggle auto fire on/off
R+A: Ctrl
R+X: Help
R+B: Num Enter


2.3 Mouse Mode

D-pad: Move mouse pointer
Left Nub: Move mouse pointer (not workng)
Right Nub: Flick left for "left mouse button", flick right for "right mouse button" (not working)
Y: Space bar
A: left mouse click
B: right mouse click
D-pad left or right: Left flipper (Pinball Dreams/Fantasies)
A/B: Right flipper (Pinball Dreams/Fantasies)
X: Pull spring (Pinball Dreams/Fantasies)


2.4 Stylus Mode

Tap screen: left click
L/R: Hold and tap with stylus performs right click
D-pad: Left operates left mouse button, right operates right mouse button. Hold down on D-pad to prevent mouse from clicking and up on D-pad
equals a right and left click together.
Recalibration: Hold Stylus against screen, hold left trigger and reposition mouse pointer using d-pad.


2.5 Specific controls inside GUI

Q: Quits the emulator.
R: Reset Amiga
SELECT: Continue emulation


------------------------------------

3. GUI

You can navigate through the controls with the D-pad and select an option by pressing X.

There are always three buttons visible:
 - Reset: Reset the Amiga
 - Quit: Quit emulator
 - Start/Resume: Start of emulation / resume emulation


3.1 Paths

Specify the location of your kickstart roms and the folder where the configurations files should be stored.
After changing the location of the kickstart roms, click on "Rescan ROMS" to refresh the list of the available ROMs.


3.2 Configurations

To load a configuration, select the entry in the list and then click on "Load". If you doubleclick on an entry in the list,
the emulation starts with this configuration.
If you want to create a new configuration, setup all options, enter a new name in "Name", provide a short description and
then click on "Save".
"Delete" will delete the selected configuration.


3.3 CPU and FPU

Select the required Amiga CPU (68000 - 68040). 

If you select 68020, you can choose between 24-bit addressing (68EC020) or 32-bit addressing (68020). 

The option "More compatible" is only available if 68000 is selected and emulates simple prefetch of the 68000. This may 
improve compatibility in few situations but it's not required for most games and demos.

JIT enables the Just-in-time compiler. This may breaks compatibility in some games. You will not notice a big performance
improvement as long as you didn't select "Fastest" in "CPU Speed".

What options are available for FPU depends on the selected CPU.

Note: In current version, you will not see a difference in the performance for 68020, 68030 and 68040. The cpu cycles for
the opcodes are based on 68020. The different cycles for 68030 and 68040 will come in a later version.


3.4 Chipset

If you want to emulate an Amiga 1200, select AGA. For most Amiga 500 games, select "Full ECS". Some older Amiga games 
requires "OCS" or "ECS Agnus". You have to play with these options if a game won't work as expected.
For some games you have to switch to "NTSC" (60 Hz instead of 50 Hz) for correct timing.

When you see some graphic issues in a game, try "Immediate" for blitter and/or disable "Fast copper".
"Fast copper" uses a prediction algorithm instead of checking the copper state on a more regular basis. This may cause 
issues but brings a big performance improvement. The option was removed in WinUAE in an early state, but for most games,
it works fine and the better performance is helpful for 600 MHz Pandoras.

For "Collision Level", select "Sprites and Sprites vs. Playfield" which is fine for nearly all games.


3.5 ROM

Select the required kickstart ROM for the Amiga you want to emulate.

"Extended ROM File" isn't really useful in the current version of UAE4ARM.


3.6 RAM

Select the amount of RAM for each type you want in your Amiga.
"Slow" is the simple memory extension of an Amiga 500.
"Z3 fast" and "RTG" are only available when a 32-bit CPU is selected.
"RTG" is the graphics memory used by Picasso96.


3.7 Floppy drives

You can enable/disable each drive by clicking the checkbox next to DFx or select the drive type in the dropdown control.
"3.5'' DD" is the right choose for nearly all ADF and ADZ files. 
The option "Write-protected" indicates if the emulator can write to the ADF.
The button "..." opens a dialog to select the required disk file.
With the big dropdown control, you can select one of the disks you recently used.

You can reduce the loading time for lot of games by increasing the floppy drive emulation speed. A few games will not load
with higher drive speed and you have to select 100%.

"Save config for disk" will create a new configuration file with the name of the disk in DF0. This configuration will be 
loaded each time you select the disk and have the option "Load config with same name as disk" enabled.


3.8 Hard drives

Use "Add Directory" to add a folder or "Add Hardfile" to add a HDF file as a hard disk.

To edit the settings of a HDD, click on "..." left to the entry in the list.

With "Create Hardfile", you can create a new formatted HDF file up to 2 GB. For large files, it will take some time to
create the new hard disk. You have to format the new HDD in Amiga via the Workbench.


3.9 Display

Select the required width and height of the Amiga screen.
If you select "NTSC", a value greater than 240 for "Height" makes no sense.
When the game, demo or workbench uses Hires mode and you selected a "Width" lower than 640, you will only see half of the
pixels.
With "Vert. offset" you can adjust the position of the first drawn line of the Amiga screen. You can also change this during
emulation with L+R + dpad up/down.
When you activate "Frameskip", only every seconds frame is drawn. This will improve performance and some more games are
playable.


3.10 Sound

You can turn on sound emulation with different levels of accuracy and choose between mono and stereo.

The different types of interpolation have different impact on the performance. Play with the settings to find the type you
like most. You may need headphones the really hear the differences between the interpolations.

With "Filter", you can select the type of the Amiga audio filter.

With "Stereo separation" and "Stereo delay" you can adjust how the left and right audio channels of the Amiga are mixed to
the left and right channels of the Pandora. A value of 70% for separation and no delay is a good start.


3.11 Input

With "Control config", you can choose one of four preset button configurations.
Select the port for which the joystick should be emulated with "Joystick".
"Tap Delay" specifies the time between taping the screen and an emulated mouse button click.
Set the emulated mouse speed to .25x, .5x, 1x 2x and 4x to slow down or speed up the mouse.
With "Stylus Offset", you can set the offset between the screen mouse pointer and the stylus by the following amount of 
pixels: 0px, 1px, 3px, 5px and 8px.

When enabling "Custom Control", you can define which Amiga key should be emulated by pressing one of the ABXY- or D-pad
buttons. Useful to setup controls for pinball games. During emulation, you can switch between regular behaviour of the
buttons and custom settings by pressing left shoulder button and 'c'.


3.12 Miscellaneous

"Status Line" shows/hides the status line indicator. During emulation, you can show/hide this by pressing left shoulder
button and 'd'. The first value in the status line shows the idle time of the Pandora CPU in %, the second value is the 
current frame rate. When you have a HDD in your Amiga emulation, the HD indicator shows read (blue) and write (red) access 
to the HDD. The next values are showing the track number for each disk drive and indicated disk access.

When you deactivate the option "Show GUI on startup" and use this configuration by specifying it with the command line
parameter "-config=<file>", the emulations starts directly without showing the GUI.

Set the speed for the Pandora CPU to overclock it for games which need more power. Be careful with this parameter.


3.13 Savestates

Savestates are stored with the name of the disk in drive DF0 attached with the selected number.

Note: Savestates will not work with HDDs.


------------------------------------

4. Command line options

-config=<file>
or
-f <file>
  Start emulator with specified configuration file instead of the default file.

-s <option>=<value>
  Set a specific option to the new value. Look at the configuration files to see the names of the available options and 
  how the value for an option is provided.
