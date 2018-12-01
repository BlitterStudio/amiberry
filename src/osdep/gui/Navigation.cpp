#ifdef USE_SDL1
#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#elif USE_SDL2
#include <guisan.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#endif
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

typedef struct
{
	string activeWidget;
	string leftWidget;
	string rightWidget;
	string upWidget;
	string downWidget;
} NavigationMap;


static NavigationMap navMap[] =
{
//  active              move left         move right        move up             move down
// main_window
{ "About",            "",               "",               "Reset",            "Paths" },
{ "Paths",            "SystemROMs",     "SystemROMs",     "About",            "Quickstart" },
{ "Quickstart",       "qsNTSC",         "qscboAModel",    "Paths",            "Configurations" },
{ "Configurations",   "ConfigList",     "ConfigList",     "Quickstart",       "CPU and FPU" },
{ "CPU and FPU",      "7 Mhz",          "68000",          "Configurations",   "Chipset" },
{ "Chipset",          "Fast copper",    "OCS",            "CPU and FPU",      "ROM" },
{ "ROM",              "MainROM",        "cboMainROM",     "Chipset",          "RAM" },
{ "RAM",              "Chipmem",        "Chipmem",        "ROM",              "Floppy drives" },
{ "Floppy drives",    "cmdSel0",        "DF0:",           "RAM",              "Hard drives/CD" },
{ "Hard drives/CD",   "cmdCreateHDF",   "cmdAddDir",      "Floppy drives",    "Display" },
{ "Display",          "sldWidth",       "sldWidth",       "Hard drives/CD",   "Sound" },
{ "Sound",            "sndDisable",     "sndDisable",     "Display",          "Input" },
{ "Input",            "cboPort0mode",   "cboPort0",       "Sound",            "Custom controls" },
{ "Custom controls",  "Right Trigger",  "0: Mouse",       "Input",            "Miscellaneous" },
{ "Miscellaneous",    "StatusLine",     "StatusLine",     "Custom controls",  "Savestates" },
#ifdef ANDROIDSDL
{ "Savestates",       "State0",         "State0",         "Miscellaneous",  "OnScreen" },
{ "OnScreen",         "OnScrButton3",   "OnScrCtrl",      "Savestates",     "Reset" },
{ "Reset",            "Start",          "Quit",           "OnScreen",       "Paths" },
{ "Quit",             "Reset",          "Help",           "OnScreen",       "Paths" },
{ "Help",             "Quit",           "Start",          "OnScreen",       "Paths" },
{ "Start",            "Help",           "Reset",          "OnScreen",       "Paths" },
#else
{ "Savestates",       "State0",         "State0",         "Miscellaneous",  "Reset" },
{ "Reset",            "Start",          "Quit",           "Savestates",     "Paths" },
{ "Quit",             "Reset",          "Shutdown",       "Savestates",     "Paths" },
{ "Shutdown",         "Quit",           "Help",           "Savestates",     "Paths" },
{ "Help",             "Shutdown",       "Start",          "Savestates",     "Paths" },
{ "Start",            "Help",           "Reset",          "Savestates",     "Paths" },
#endif

// PanelPaths
{ "SystemROMs",     "Paths",          "Paths",          "RescanROMs",     "ConfigPath" },
{ "ConfigPath",     "Paths",          "Paths",          "SystemROMs",     "ControllersPath" },
{ "ControllersPath","Paths",          "Paths",          "ConfigPath",     "RetroArchFile" },
{ "RetroArchFile",  "Paths",          "Paths",          "ControllersPath","RescanROMs" },
{ "RescanROMs",     "Paths",          "Paths",          "RetroArchFile",  "SystemROMs" },

//  active            move left         move right        move up           move down
// PanelQuickstart
{ "qscboAModel",    "Quickstart",     "qsNTSC",         "cmdSetConfig",   "qscboAConfig" },
{ "qsNTSC",         "qscboAModel",    "Quickstart",     "qsMode",         "qscboAConfig" },
{ "qscboAConfig",   "Quickstart",     "Quickstart",     "qscboAModel",    "qscmdSel0" },
{ "qsDF0",          "Quickstart",     "qsWP0",          "qscboAConfig",   "qscboDisk0" },
{ "qsWP0",          "qsDF0",          "qscmdEject0",    "qscboAConfig",   "qscboDisk0" },
//  { "qsInfo0",        "Quickstart",     "",     "",               "" },
{ "qscmdEject0",    "qsWP0",          "qscmdSel0",      "qscboAConfig",   "qscboDisk0" },
{ "qscmdSel0",      "qscmdEject0",    "Quickstart",     "qscboAConfig",   "qscboDisk0" },
{ "qscboDisk0",     "Quickstart",     "Quickstart",     "qscmdSel0",      "qscmdSel1" },
{ "qsDF1",          "Quickstart",     "qsWP1",          "qscboDisk0",     "qscboDisk1" },
{ "qsWP1",          "qsDF1",          "qscmdEject1",    "qscboDisk0",     "qscboDisk1" },
//  { "qsInfo1",        "Quickstart",     "",     "",               "" },
{ "qscmdEject1",    "qsWP1",          "qscmdSel1",      "qscboDisk0",     "qscboDisk1" },
{ "qscmdSel1",      "qscmdEject1",    "Quickstart",     "qscboDisk0",     "qscboDisk1" },
{ "qscboDisk1",     "Quickstart",     "Quickstart",     "qsDF1",          "qsCDSelect" },
{ "qsCD drive",     "Quickstart",     "qscdEject",      "qscboDisk1",     "qscboCD" },
{ "qscdEject",      "qsCD drive",     "qsCDSelect",     "qscboDisk1",     "qscboCD" },
{ "qsCDSelect",     "qscdEject",      "Quickstart",     "qscboDisk1",     "qscboCD" },
{ "qscboCD",        "Quickstart",     "Quickstart",     "qsCDSelect",     "qsMode" },
{ "qsMode",         "Quickstart",     "Quickstart",     "qscboCD",        "cmdSetConfig" },
{ "cmdSetConfig",   "Quickstart",     "Quickstart",     "qsMode",         "qscboAModel" },

// PanelConfig
{ "ConfigList",     "Configurations", "ConfigName",     "",               "" },
{ "ConfigName",     "Configurations", "Configurations", "ConfigList",     "ConfigDesc" },
{ "ConfigDesc",     "Configurations", "Configurations", "ConfigName",     "ConfigLoad" },
{ "ConfigLoad",     "Configurations", "ConfigSave",     "ConfigDesc",     "ConfigList" },
{ "ConfigSave",     "ConfigLoad",     "CfgDelete",      "ConfigDesc",     "ConfigList" },
{ "CfgDelete",      "ConfigSave",     "Configurations", "ConfigDesc",     "ConfigList" },

//  active            move left         move right        move up           move down
// PanelCPU
{ "68000",          "CPU and FPU",    "FPUnone",        "JIT",            "68010" },
{ "68010",          "CPU and FPU",    "68881",          "68000",          "68020" },
{ "68020",          "CPU and FPU",    "68882",          "68010",          "68030" },
{ "68030",          "CPU and FPU",    "CPU internal",   "68020",          "68040" },
{ "68040",          "CPU and FPU",    "FPUstrict",      "68030",          "CPU24Bit" },
{ "CPU24Bit",       "CPU and FPU",    "FPUJIT",         "68040",          "CPUComp" },
{ "CPUComp",        "CPU and FPU",    "FPUJIT",         "CPU24Bit",       "JIT" },
{ "JIT",            "CPU and FPU",    "FPUJIT",         "CPUComp",        "68000" },
{ "FPUnone",        "68000",          "7 Mhz",          "FPUJIT",         "68881" },
{ "68881",          "68010",          "14 Mhz",         "FPUnone",        "68882" },
{ "68882",          "68020",          "25 Mhz",         "68881",          "CPU internal" },
{ "CPU internal",   "68030",          "Fastest",        "68882",          "FPUstrict" },
{ "FPUstrict",      "68040",          "Fastest",        "CPU internal",   "FPUJIT" },
{ "FPUJIT",         "CPU24Bit",       "Fastest",        "FPUstrict",      "FPUnone" },
{ "7 Mhz",          "FPUnone",        "CPU and FPU",    "Turbo",          "14 Mhz" },
{ "14 Mhz",         "FPUnone",        "CPU and FPU",    "7 Mhz",          "25 Mhz" },
{ "25 Mhz",         "FPUnone",        "CPU and FPU",    "14 Mhz",         "Fastest" },
{ "Fastest",        "FPUnone",        "CPU and FPU",    "25 Mhz",         "Turbo" },
{ "Turbo",			"FPUnone",	      "CPU and FPU",    "Fastest",        "7 Mhz" },

// PanelChipset
{ "OCS",            "Chipset",        "ChipsetExtra",   "CollFull",       "ECS Agnus" },
{ "ECS Agnus",      "Chipset",        "Immediate",      "OCS",            "Full ECS" },
{ "Full ECS",       "Chipset",        "BlitWait",       "ECS Agnus",      "AGA" },
{ "AGA",            "Chipset",        "Fast copper",    "Full ECS",       "NTSC" },
{ "NTSC",           "Chipset",        "Fast copper",    "AGA",            "CollNone" },
{ "ChipsetExtra",   "OCS",            "BlitNormal",     "",               "" },
{ "BlitNormal",     "ChipsetExtra",   "Chipset",        "Fast copper",    "Immediate" },
{ "Immediate",      "ECS Agnus",      "Chipset",        "BlitNormal",     "BlitWait" },
{ "BlitWait",       "Full ECS",       "Chipset",        "Immediate",      "Fast copper" },
{ "Fast copper",    "NTSC",           "Chipset",        "BlitWait",       "BlitNormal" },
{ "CollNone",       "Chipset",        "Chipset",        "NTSC",           "Sprites only" },
{ "Sprites only",   "Chipset",        "Chipset",        "CollNone",       "CollPlay" },
{ "CollPlay",       "Chipset",        "Chipset",        "Sprites only",   "CollFull" },
{ "CollFull",       "Chipset",        "Chipset",        "CollPlay",       "OCS" },

//  active            move left         move right        move up           move down
// PanelROM
{ "cboMainROM",     "ROM",            "MainROM",        "cboCartROM",     "cboExtROM" },
{ "MainROM",        "cboMainROM",     "ROM",            "CartROM",        "ExtROM" },
{ "cboExtROM",      "ROM",            "ExtROM",         "cboMainROM",     "cboCartROM" },
{ "ExtROM",         "cboExtROM",      "ROM",            "MainROM",        "CartROM" },
{ "cboCartROM",     "ROM",            "CartROM",        "cboExtROM",      "cboMainROM" },
{ "CartROM",        "cboCartROM",     "ROM",            "ExtROM",         "MainROM" },

//PanelRAM
{ "Chipmem",        "",               "",               "RAM",            "Slowmem" },
{ "Slowmem",        "",               "",               "Chipmem",        "Fastmem" },
{ "Fastmem",        "",               "",               "Slowmem",        "Z3mem" },
{ "Z3mem",          "",               "",               "Fastmem",        "Gfxmem" },
{ "Gfxmem",         "",               "",               "Z3mem",          "A3000Low" },
{ "A3000Low",       "",               "",               "Gfxmem",         "A3000High" },
{ "A3000High",      "",               "",               "A3000Low",       "Chipmem" },

//PanelFloppy
{ "DF0:",           "Floppy drives",  "cboType0",       "SaveForDisk",    "cboDisk0" },
{ "cboType0",       "DF0:",           "cmdEject0",      "SaveForDisk",    "cboDisk0" },
{ "cmdEject0",      "cboType0",       "cmdSel0",        "CreateHD",       "cboDisk0" },
{ "cmdSel0",        "cmdEject0",      "Floppy drives",  "CreateHD",       "cboDisk0" },
{ "cboDisk0",       "Floppy drives",  "Floppy drives",  "DF0:",           "LoadDiskCfg" },
{ "LoadDiskCfg",    "Floppy drives",  "Floppy drives",  "cboDisk0",       "DF1:" },
{ "DF1:",           "Floppy drives",  "cboType1",       "LoadDiskCfg",    "cboDisk1" },
{ "cboType1",       "DF1:",           "cmdEject1",      "LoadDiskCfg",    "cboDisk1" },
{ "cmdEject1",      "cboType1",       "cmdSel1",        "LoadDiskCfg",    "cboDisk1" },
{ "cmdSel1",        "cmdEject1",      "Floppy drives",  "LoadDiskCfg",    "cboDisk1" },
{ "cboDisk1",       "Floppy drives",  "Floppy drives",  "DF1:",           "DF2:" },
{ "DF2:",           "Floppy drives",  "cboType2",       "cboDisk1",       "cboDisk2" },
{ "cboType2",       "DF2:",           "cmdEject2",      "cboDisk1",       "cboDisk2" },
{ "cmdEject2",      "cboType2",       "cmdSel2",        "cboDisk1",       "cboDisk2" },
{ "cmdSel2",        "cmdEject2",      "Floppy drives",  "cboDisk1",       "cboDisk2" },
{ "cboDisk2",       "Floppy drives",  "Floppy drives",  "DF2:",           "DF3:" },
{ "DF3:",           "Floppy drives",  "cboType3",       "cboDisk2",       "cboDisk3" },
{ "cboType3",       "DF3:",           "cmdEject3",      "cboDisk2",       "cboDisk3" },
{ "cmdEject3",      "cboType3",       "cmdSel3",        "cboDisk2",       "cboDisk3" },
{ "cmdSel3",        "cmdEject3",      "Floppy drives",  "cboDisk2",       "cboDisk3" },
{ "cboDisk3",       "Floppy drives",  "Floppy drives",  "DF3:",           "DriveSpeed" },
{ "DriveSpeed",     "",               "",               "cboDisk3",       "CreateDD" },
{ "SaveForDisk",    "Floppy drives",  "CreateDD",       "DriveSpeed",     "DF0:" },
{ "CreateDD",       "SaveForDisk",    "CreateHD",       "DriveSpeed",     "cboType0" },
{ "CreateHD",       "CreateDD",       "Floppy drives",  "DriveSpeed",     "cmdEject0" },

//  active            move left           move right          move up           move down
// PanelHD
{ "cmdProp0", "Hard drives/CD", "cmdDel0", "CDVol", "cmdProp1" },
{ "cmdDel0", "cmdProp0", "Hard drives/CD", "CDVol", "cmdDel1" },
{ "cmdProp1", "Hard drives/CD", "cmdDel1", "cmdProp0", "cmdProp2" },
{ "cmdDel1", "cmdProp1", "Hard drives/CD", "cmdDel0", "cmdDel2" },
{ "cmdProp2", "Hard drives/CD", "cmdDel2", "cmdProp1", "cmdProp3" },
{ "cmdDel2", "cmdProp2", "Hard drives/CD", "cmdDel1", "cmdDel3" },
{ "cmdProp3", "Hard drives/CD", "cmdDel3", "cmdProp2", "cmdProp4" },
{ "cmdDel3", "cmdProp3", "Hard drives/CD", "cmdDel2", "cmdDel4" },
{ "cmdProp4", "Hard drives/CD", "cmdDel4", "cmdProp3", "cmdAddDir" },
{ "cmdDel4", "cmdProp4", "Hard drives/CD", "cmdDel3", "cmdAddHDF" },
{ "cmdAddDir", "Hard drives/CD", "cmdAddHDF", "cmdProp4", "chkHDRO" },
{ "cmdAddHDF", "cmdAddDir", "cmdCreateHDF", "cmdDel4", "chkHDRO" },
{ "cmdCreateHDF", "cmdAddHDF", "Hard drives/CD", "cmdDel4", "chkHDRO" },
{ "chkHDRO",        "Hard drives/CD", "Hard drives/CD", "cmdAddDir",      "CD drive" },
{ "CD drive", "Hard drives/CD", "cdEject", "chkHDRO", "cboCD" },
{ "cdEject", "CD drive", "CDSelect", "cmdCreateHDF", "cboCD" },
{ "CDSelect", "cdEject", "Hard drives/CD", "cmdCreateHDF", "cboCD" },
{ "cboCD", "Hard drives/CD", "Hard drives/CD", "CD drive", "CDVol" },
{ "CDVol", "", "", "cboCD", "cmdProp0" },

//  active            move left           move right          move up           move down
// PanelDisplay
{ "sldWidth",       "",             "",               "Frameskip",      "sldHeight" },
{ "sldHeight",      "",             "",               "sldWidth",       "sldVertPos" },
#ifdef USE_SDL1
{ "sldVertPos",     "",             "",               "sldHeight",       "Single" },
{ "Single",        "Display",       "Display",        "sdlVertPos",      "Double" },
{ "Double",        "Display",       "Display",        "Single",          "Scanlines" },
{ "Scanlines",     "Display",       "Display",        "Double",          "Fullscreen" },
{ "CorrectAR",      "Display",      "Fullscreen",     "Scanlines",       "Frameskip" },
#elif USE_SDL2
{ "sldVertPos",     "",             "",               "sldHeight",       "Auto" },
{ "Auto",           "Display",      "Single",         "sldVertPos",      "Nearest Neighbor (pixelated)" },
{ "Nearest Neighbor (pixelated)","Display","Double",  "Auto",            "Linear (smooth)" },
{ "Linear (smooth)","Display",      "Scanlines", "Nearest Neighbor (pixelated)", "CorrectAR" },
{ "Single",         "Auto",         "Auto",           "sdlVertPos",      "Double" },
{ "Double", "Nearest Neighbor (pixelated)", "Nearest Neighbor (pixelated)", "Single", "Scanlines" },
{ "Scanlines", "Linear (smooth)",   "Linear (smooth)", "Double",         "Fullscreen" },
{ "CorrectAR",      "Display",      "Fullscreen",     "Linear (smooth)", "Frameskip" },
#endif
{ "Fullscreen",     "CorrectAR",    "CorrectAR",      "Scanlines", "Frameskip" },
{ "Frameskip",      "Display",      "Display",        "CorrectAR",  "sldWidth" },

//  active            move left           move right          move up           move down
//PanelSound
{ "sndDisable",     "Sound",          "Mono",           "sldStereoDelay", "sndDisEmu" },
{ "sndDisEmu",      "Sound",          "Stereo",         "sndDisable",     "sndEmulate" },
{ "sndEmulate",     "Sound",          "Stereo",         "sndDisEmu",      "sndEmuBest" },
{ "sndEmuBest",     "Sound",          "Stereo",         "sndEmulate",     "cboFrequency" },
{ "Mono",           "sndDisable",     "Sound",          "sldStereoDelay", "Stereo" },
{ "Stereo",         "sndDisEmu",      "Sound",          "Mono",           "cboFrequency" },
{ "cboFrequency",   "Sound",          "Sound",          "sndEmuBest",     "cboInterpol" },
{ "cboInterpol",    "Sound",          "Sound",          "cboFrequency",   "cboFilter" },
{ "cboFilter",      "Sound",          "Sound",          "cboInterpol",    "sldSeparation" },
{ "sldSeparation",  "",               "",               "cboFilter",      "sldStereoDelay" },
{ "sldStereoDelay", "",               "",               "sldSeparation",  "sndDisable" },

//  active            move left           move right          move up           move down
// PanelInput
{ "cboPort0",         "Input",          "cboPort0mode",       "cboAutofire",      "cboPort1" },
{ "cboPort0mode",     "cboPort0",       "Input",              "cboTapDelay",      "cboPort1mode" },
{ "cboPort1",         "Input",          "cboPort1mode",       "cboPort0",         "cboPort2" },
{ "cboPort1mode",     "cboPort1",       "Input",              "cboPort0mode",     "cboPort2" },
{ "cboPort2",         "Input",          "cboPort2mode",       "cboPort1",         "cboPort3" },
{ "cboPort3",         "Input",          "cboPort3mode",       "cboPort2",         "cboPort0mousemode" },
{ "cboPort0mousemode","Input",          "MouseSpeed",         "cboPort3",         "cboPort1mousemode" },
{ "cboPort1mousemode","Input",          "MouseSpeed",         "cboPort0mousemode","cboAutofire" },
{ "MouseSpeed",       "",               "",                   "cboPort3",         "MouseHack" },
{ "MouseHack",        "cboAutofire",    "cboAutofire",        "MouseSpeed",       "cboPort0" },
{ "cboAutofire",      "Input",          "MouseHack",          "cboPort1mousemode","cboPort0" },

// PanelCustom
{ "0: Mouse",         "Custom controls",  "1: Joystick",      "chkAnalogRemap",     "None" },
{ "1: Joystick",      "0: Mouse",         "2: Parallel 1",    "chkAnalogRemap",     "HotKey" },
{ "2: Parallel 1",    "1: Joystick",      "3: Parallel 2",    "chkAnalogRemap",     "Left Trigger" },
{ "3: Parallel 2",    "2: Parallel 1",    "Custom controls",  "chkAnalogRemap",     "Right Trigger" },

{ "None",             "Custom controls",  "HotKey",           "0: Mouse",          "cboCustomAction0" },
{ "HotKey",           "None",             "Left Trigger",     "1: Joystick",       "cboCustomAction0" },
{ "Left Trigger",     "HotKey",           "Right Trigger",    "2: Parallel 1",     "cboCustomAction0" },
{ "Right Trigger",    "Left Trigger",     "Custom controls",  "3: Parallel 2",     "cboCustomAction0" },

{ "cboCustomAction0", "Custom controls",  "cboCustomAction7" , "None",              "cboCustomAction1" },
{ "cboCustomAction1", "Custom controls",  "cboCustomAction8" , "cboCustomAction0",  "cboCustomAction2" },
{ "cboCustomAction2", "Custom controls",  "cboCustomAction9" , "cboCustomAction1",  "cboCustomAction3" },
{ "cboCustomAction3", "Custom controls",  "cboCustomAction10", "cboCustomAction2",  "cboCustomAction4" },
{ "cboCustomAction4", "Custom controls",  "cboCustomAction11", "cboCustomAction3",  "cboCustomAction5" },
{ "cboCustomAction5", "Custom controls",  "cboCustomAction12", "cboCustomAction4",  "cboCustomAction6" },
{ "cboCustomAction6", "Custom controls",  "cboCustomAction13", "cboCustomAction5",  "cboCustomAction7" },

{ "cboCustomAction7",  "cboCustomAction0",  "Custom controls",  "cboCustomAction6",  "cboCustomAction8" },
{ "cboCustomAction8",  "cboCustomAction1",  "Custom controls" , "cboCustomAction7",  "cboCustomAction9" },
{ "cboCustomAction9",  "cboCustomAction2",  "Custom controls" , "cboCustomAction8",  "cboCustomAction10" },
{ "cboCustomAction10", "cboCustomAction3",  "Custom controls" , "cboCustomAction9",  "cboCustomAction11" },
{ "cboCustomAction11", "cboCustomAction4",  "Custom controls", "cboCustomAction10",  "cboCustomAction12" },
{ "cboCustomAction12", "cboCustomAction5",  "Custom controls", "cboCustomAction11",  "cboCustomAction13" },
{ "cboCustomAction13", "cboCustomAction6",  "Custom controls", "cboCustomAction12",  "chkAnalogRemap" },

{ "chkAnalogRemap",     "Custom controls",   "Custom controls",    "cboCustomAction13",     "0: Mouse" },

// PanelMisc
//  active            move left           move right          move up           move down

{ "StatusLine",     "Miscellaneous",  "RetroArchQuit",  "cboScrolllock",     "HideIdle" },
{ "HideIdle",       "Miscellaneous",  "RetroArchMenu",  "StatusLine",     "ShowGUI" },
{ "ShowGUI",        "Miscellaneous",  "RetroArchReset",  "HideIdle",      "BSDSocket" },
{ "RetroArchQuit",  "StatusLine",     "Miscellaneous",  "KeyForQuit",     "RetroArchMenu" },
{ "RetroArchMenu",  "HideIdle",       "Miscellaneous",  "RetroArchQuit",  "RetroArchReset" },
{ "RetroArchReset", "ShowGUI",        "Miscellaneous",  "RetroArchMenu",  "BSDSocket" },

{ "BSDSocket",      "Miscellaneous",  "Miscellaneous",  "ShowGUI",        "MasterWP" },
{ "MasterWP",       "Miscellaneous",  "Miscellaneous",  "BSDSocket",      "cboNumlock" },
{ "cboNumlock",     "Miscellaneous",  "cboScrolllock",	"MasterWP",	      "OpenGUI" },
{ "cboScrolllock",  "cboNumlock",	  "Miscellaneous",  "MasterWP",	      "KeyForQuit" },
{ "OpenGUI",        "Miscellaneous",  "KeyForQuit",     "cboNumlock",	  "KeyActionReplay" },
{ "KeyForQuit",     "OpenGUI",	      "Miscellaneous",  "cboScrolllock",  "KeyFullScreen" },
{ "KeyActionReplay","Miscellaneous",  "KeyFullScreen",  "OpenGUI",        "StatusLine" },
{ "KeyFullScreen",  "KeyActionReplay","KeyActionReplay","KeyForQuit",     "RetroArchQuit" },

// PanelSavestate
{ "State0", "Savestates", "Savestates", "LoadState", "State1" },
{ "State1", "Savestates", "Savestates", "State0", "State2" },
{ "State2", "Savestates", "Savestates", "State1", "State3" },
{ "State3", "Savestates", "Savestates", "State2", "LoadState" },
{ "LoadState", "Savestates", "SaveState", "State3", "State0" },
{ "SaveState", "LoadState", "Savestates", "State3", "State0" },

#ifdef ANDROIDSDL
// PanelOnScreen
{ "OnScrCtrl",      "OnScreen",       "OnScrButton3", "DisableMenuVKeyb", "OnScrTextInput" },
{ "OnScrButton3",   "OnScrCtrl",      "OnScreen",     "CustomPos",     "OnScrButton4" },
{ "OnScrTextInput", "OnScreen",       "OnScrButton4", "OnScrCtrl",      "OnScrDpad" },
{ "OnScrButton4",   "OnScrTextInput", "OnScreen",     "OnScrButton3",   "OnScrButton5" },
{ "OnScrDpad",      "OnScreen",       "OnScrButton5", "OnScrTextInput", "OnScrButton1" },
{ "OnScrButton5",   "OnScrDpad",      "OnScreen",     "OnScrButton4",   "OnScrButton6" },
{ "OnScrButton1",   "OnScreen",       "OnScrButton6", "OnScrDpad",      "OnScrButton2" },
{ "OnScrButton6",   "OnScrButton1",   "OnScreen",     "OnScrButton5",   "CustomPos" },
{ "OnScrButton2",   "OnScreen",       "CustomPos",    "OnScrButton1",   "FloatJoy" },
{ "CustomPos",      "OnScrButton2",   "OnScreen",     "OnScrButton6",   "Reset" },
{ "FloatJoy",       "OnScreen",       "CustomPos",    "OnScrButton2",   "DisableMenuVKeyb" },
{ "DisableMenuVKeyb","OnScreen",       "CustomPos",    "FloatJoy",       "Reset" },
#endif

//  active            move left         move right        move up           move down
// EditFilesysVirtual
{ "virtDev", "virtRW", "virtRW", "virtOK", "virtVol" },
{ "virtVol", "virtBootpri", "virtAutoboot", "virtDev", "virtPath" },
{ "virtPath", "", "", "virtBootpri", "virtCancel" },
{ "virtRW", "virtDev", "virtDev", "virtOK", "virtAutoboot" },
{ "virtAutoboot", "virtVol", "virtBootpri", "virtRW", "virtPath" },
{ "virtBootpri", "virtAutoboot", "virtVol", "virtRW", "virtPath" },
{ "virtOK", "virtCancel", "virtCancel", "virtPath", "virtRW" },
{ "virtCancel", "virtOK", "virtOK", "virtPath", "virtRW" },

// EditFilesysHardfile
{ "hdfDev",         "hdfBootPri",     "hdfRW",          "hdfOK",          "hdfPath" },
{ "hdfRW",          "hdfDev",         "hdfAutoboot",    "hdfOK",          "hdfPath" },
{ "hdfAutoboot",    "hdfRW",          "hdfBootPri",     "hdfOK",          "hdfPath" },
{ "hdfBootPri",     "hdfAutoboot",    "hdfDev",         "hdfCancel",      "hdfPath" },
{ "hdfSurface",     "hdfReserved",    "hdfReserved",    "hdfPath",        "hdfSectors" },
{ "hdfReserved",    "hdfSurface",     "hdfSurface",     "hdfPath",        "hdfBlocksize" },
{ "hdfSectors",     "hdfBlocksize",   "hdfBlocksize",   "hdfSurface",     "hdfController" },
{ "hdfBlocksize",   "hdfSectors",     "hdfSectors",     "hdfReserved",    "hdfUnit" },
{ "hdfPath",        "",               "",               "hdfBootPri",     "hdfReserved" },
{ "hdfController",  "hdfUnit",        "hdfUnit",        "hdfSectors",     "hdfOK" },
{ "hdfUnit",        "hdfController",  "hdfController",  "hdfBlocksize",   "hdfOK" },
{ "hdfOK",          "hdfCancel",      "hdfCancel",      "hdfUnit",        "hdfBootPri" },
{ "hdfCancel",      "hdfOK",          "hdfOK",          "hdfUnit",        "hdfBootPri" },

// CreateFilesysHardfile
{ "createHdfDev",       "createHdfBootPri",   "createHdfAutoboot",  "createHdfOK",      "createHdfPath" },
{ "createHdfAutoboot",  "createHdfDev",       "createHdfBootPri",   "createHdfOK",      "createHdfPath" },
{ "createHdfBootPri",   "createHdfAutoboot",  "createHdfDev",       "createHdfOK",      "createHdfPath" },
{ "createHdfSize",      "",                   "",                   "createHdfPath",    "createHdfOK" },
{ "createHdfPath",      "",                   "",                   "createHdfBootPri", "createHdfSize" },
{ "createHdfOK",        "createHdfCancel",    "createHdfCancel",    "createHdfSize",    "createHdfBootPri" },
{ "createHdfCancel",    "createHdfOK",        "createHdfOK",        "createHdfSize",    "createHdfBootPri" },

{ "END", "", "", "", "" }
};


bool HandleNavigation(int direction)
{
	const auto focusHdl = gui_top->_getFocusHandler();
	gcn::Widget* focusTarget = nullptr;

	if (focusHdl != nullptr)
	{
		auto activeWidget = focusHdl->getFocused();

		if (activeWidget != nullptr && activeWidget->getId().length() > 0)
		{
			auto activeName = activeWidget->getId();
			auto bFoundEnabled = false;
			auto tries = 10;

			while (!bFoundEnabled && tries > 0)
			{
				string searchFor;

				for (auto i = 0; navMap[i].activeWidget != "END"; ++i)
				{
					if (navMap[i].activeWidget == activeName)
					{
						switch (direction)
						{
						case DIRECTION_LEFT:
							searchFor = navMap[i].leftWidget;
							break;
						case DIRECTION_RIGHT:
							searchFor = navMap[i].rightWidget;
							break;
						case DIRECTION_UP:
							searchFor = navMap[i].upWidget;
							break;
						case DIRECTION_DOWN:
							searchFor = navMap[i].downWidget;
							break;
						default: 
							break;
						}
						if (searchFor.length() > 0)
						{
							focusTarget = gui_top->findWidgetById(searchFor);
							if (focusTarget != nullptr)
							{
								if(focusTarget->isEnabled() && focusTarget->isVisible())
									bFoundEnabled = true;
								else
									activeName = searchFor;
							}
							else
							{
								bFoundEnabled = true;
								break;
							}
						}
						break;
					}
				}
				if (searchFor.empty())
					bFoundEnabled = true; // No entry to navigate to -> exit loop
				--tries;
			}

			if (focusTarget != nullptr)
			{
				if (activeWidget->getId().substr(0, 3) == "cbo" || activeWidget->getId().substr(0, 5) == "qscbo")
				{
					const auto dropdown = dynamic_cast<gcn::UaeDropDown *>(activeWidget);
					if (dropdown->isDroppedDown() && (direction == DIRECTION_UP || direction == DIRECTION_DOWN))
						focusTarget = nullptr; // Up/down navigates in list if dropped down
				}
			}
		}
	}

	if (focusTarget != nullptr)
		focusTarget->requestFocus();
	return focusTarget != nullptr;
}

#ifdef USE_SDL1
void PushFakeKey(const SDLKey inKey)
{
	SDL_Event nuevent;

	nuevent.type = SDL_KEYDOWN;  // and the key up
	nuevent.key.keysym.sym = inKey;
	gui_input->pushInput(nuevent); // Fire key down
	nuevent.type = SDL_KEYUP;  // and the key up
	gui_input->pushInput(nuevent); // Fire key down
}
#elif USE_SDL2
void PushFakeKey(const SDL_Keycode inKey)
{
	SDL_Event nuevent;

	nuevent.type = SDL_KEYDOWN;  // and the key up
	nuevent.key.keysym.sym = inKey;
	gui_input->pushInput(nuevent); // Fire key down
	nuevent.type = SDL_KEYUP;  // and the key up
	gui_input->pushInput(nuevent); // Fire key down
}
#endif