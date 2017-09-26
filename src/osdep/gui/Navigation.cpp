#include <guichan.hpp>
#include <SDL/SDL_ttf.h>
#include <guichan/sdl.hpp>
#include "sdltruetypefont.hpp"
#include "SelectorEntry.hpp"
#include "UaeRadioButton.hpp"
#include "UaeDropDown.hpp"

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "gui_handling.h"

typedef struct
{
  std::string activeWidget;
  std::string leftWidget;
  std::string rightWidget;
  std::string upWidget;
  std::string downWidget;
} NavigationMap;


static NavigationMap navMap[] = 
{
//  active              move left         move right        move up             move down
// main_window
  { "Paths",            "SystemROMs",     "SystemROMs",     "Reset",            "Quickstart" },
  { "Quickstart",       "qsNTSC",         "AModel",         "Paths",            "Configurations" },
  { "Configurations",   "ConfigList",     "ConfigList",     "Quickstart",       "CPU and FPU" },
  { "CPU and FPU",      "7 Mhz",          "68000",          "Configurations",   "Chipset" },
  { "Chipset",          "Fast copper",    "OCS",            "CPU and FPU",      "ROM" },
  { "ROM",              "MainROM",        "cboMainROM",     "Chipset",          "RAM" },
  { "RAM",              "Chipmem",        "Chipmem",        "ROM",              "Floppy drives" },
  { "Floppy drives",    "cmdSel0",        "DF0:",           "RAM",              "Hard drives / CD" },
  { "Hard drives / CD", "cmdCreateHDF",   "cmdAddDir",      "Floppy drives",    "Display" },
  { "Display",          "sldWidth",       "sldWidth",       "Hard drives / CD", "Sound" },
  { "Sound",            "sndDisable",     "sndDisable",     "Display",          "Input" },
  { "Input",            "cboPort0",       "cboPort0",       "Sound",            "Miscellaneous" },
  { "Miscellaneous",    "StatusLine",     "StatusLine",     "Input",            "Savestates" },
  { "Savestates",       "State0",         "State0",         "Miscellaneous",    "Reset" },
  { "Reset",            "Start",          "Quit",           "Savestates",       "Paths" },
  { "Quit",             "Reset",          "Help",          "Savestates",       "Paths" },
  { "Help",             "Quit",           "Start",          "Savestates",       "Paths" },
  { "Start",            "Help",           "Reset",          "Savestates",       "Paths" },

// PanelPaths
  { "SystemROMs",     "Paths",          "Paths",          "RescanROMs",     "ConfigPath" },
  { "ConfigPath",     "Paths",          "Paths",          "SystemROMs",     "RescanROMs" },
  { "RescanROMs",     "Paths",          "Paths",          "ConfigPath",     "SystemROMs" },

//  active            move left         move right        move up           move down
// PanelQuickstart
  { "AModel",         "Quickstart",     "qsNTSC",         "qsMode",         "AConfig" },
  { "qsNTSC",         "AModel",         "Quickstart",     "qsMode",         "AConfig" },
  { "AConfig",        "Quickstart",     "Quickstart",     "AModel",         "qscmdSel0" },
  { "qsDF0",          "Quickstart",     "qsWP0",          "AConfig",        "qscboDisk0" },
  { "qsWP0",          "qsDF0",          "qscmdEject0",    "AConfig",        "qscboDisk0" },
//  { "qsInfo0",        "Quickstart",     "",     "",               "" },
  { "qscmdEject0",    "qsWP0",          "qscmdSel0",      "AConfig",        "qscboDisk0" },
  { "qscmdSel0",      "qscmdEject0",    "Quickstart",     "AConfig",        "qscboDisk0" },
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
  { "qsMode",         "Quickstart",     "Quickstart",     "qscboCD",        "qsNTSC" },

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
  { "CPU24Bit",       "CPU and FPU",    "SoftFloat",      "68040",          "CPUComp" },
  { "CPUComp",        "CPU and FPU",    "SoftFloat",      "CPU24Bit",       "JIT" },
  { "JIT",            "CPU and FPU",    "SoftFloat",      "CPUComp",        "68000" },
  { "FPUnone",        "68000",          "7 Mhz",          "SoftFloat",      "68881" },
  { "68881",          "68010",          "14 Mhz",         "FPUnone",        "68882" },
  { "68882",          "68020",          "25 Mhz",         "68881",          "CPU internal" },
  { "CPU internal",   "68030",          "Fastest",        "68882",          "FPUstrict" },
  { "FPUstrict",      "68040",          "Fastest",        "CPU internal",   "SoftFloat" },
  { "SoftFloat",      "CPU24Bit",       "Fastest",        "FPUstrict",      "FPUnone" },
  { "7 Mhz",          "FPUnone",        "CPU and FPU",    "Fastest",        "14 Mhz" },
  { "14 Mhz",         "68881",          "CPU and FPU",    "7 Mhz",          "25 Mhz" },
  { "25 Mhz",         "68882",          "CPU and FPU",    "14 Mhz",         "Fastest" },
  { "Fastest",        "CPU internal",   "CPU and FPU",    "25 Mhz",         "7 Mhz" },

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
#ifdef ACTION_REPLAY
  { "cboMainROM",     "ROM",            "MainROM",        "cboCartROM",     "cboExtROM" },
  { "MainROM",        "cboMainROM",     "ROM",            "CartROM",        "ExtROM" },
  { "cboExtROM",      "ROM",            "ExtROM",         "cboMainROM",     "cboCartROM" },
  { "ExtROM",         "cboExtROM",      "ROM",            "MainROM",        "CartROM" },
  { "cboCartROM",     "ROM",            "CartROM",        "cboExtROM",      "cboMainROM" },
  { "CartROM",        "cboCartROM",     "ROM",            "ExtROM",         "MainROM" },
#else
  { "cboMainROM",     "ROM",            "MainROM",        "cboExtROM",      "cboExtROM" },
  { "MainROM",        "cboMainROM",     "ROM",            "ExtROM",         "ExtROM" },
  { "cboExtROM",      "ROM",            "ExtROM",         "cboMainROM",     "cboMainROM" },
  { "ExtROM",         "cboExtROM",      "ROM",            "MainROM",        "MainROM" },
#endif

//PanelRAM
  { "Chipmem",        "",               "",               "RAM",            "Slowmem" },
  { "Slowmem",        "",               "",               "Chipmem",        "Fastmem" },
  { "Fastmem",        "",               "",               "Slowmem",        "Z3mem" },
  { "Z3mem",          "",               "",               "Fastmem",        "Gfxmem" },
  { "Gfxmem",         "",               "",               "Z3mem",          "A3000Low" },
  { "A3000Low",       "",               "",               "Gfxmem",         "A3000High" },
  { "A3000High",      "",               "",               "A3000Low",       "RAM" },

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
  { "cmdProp0",       "Hard drives / CD", "cmdDel0",          "CDVol",          "cmdProp1" },
  { "cmdDel0",        "cmdProp0",         "Hard drives / CD", "CDVol",          "cmdDel1" },
  { "cmdProp1",       "Hard drives / CD", "cmdDel1",          "cmdProp0",       "cmdProp2" },
  { "cmdDel1",        "cmdProp1",         "Hard drives / CD", "cmdDel0",        "cmdDel2" },
  { "cmdProp2",       "Hard drives / CD", "cmdDel2",          "cmdProp1",       "cmdProp3" },
  { "cmdDel2",        "cmdProp2",         "Hard drives / CD", "cmdDel1",        "cmdDel3" },
  { "cmdProp3",       "Hard drives / CD", "cmdDel3",          "cmdProp2",       "cmdProp4" },
  { "cmdDel3",        "cmdProp3",         "Hard drives / CD", "cmdDel2",        "cmdDel4" },
  { "cmdProp4",       "Hard drives / CD", "cmdDel4",          "cmdProp3",       "cmdAddDir" },
  { "cmdDel4",        "cmdProp4",         "Hard drives / CD", "cmdDel3",        "cmdAddHDF" },
  { "cmdAddDir",      "Hard drives / CD", "cmdAddHDF",        "cmdProp4",       "chkHDRO" },
  { "cmdAddHDF",      "cmdAddDir",        "cmdCreateHDF",     "cmdDel4",        "chkHDRO" },
  { "cmdCreateHDF",   "cmdAddHDF",        "Hard drives / CD", "cmdDel4",        "chkHDRO" },
  { "chkHDRO",        "Hard drives / CD", "Hard drives / CD", "cmdAddDir",      "CD drive" },
  { "CD drive",       "Hard drives / CD", "cdEject",          "chkHDRO",        "cboCD" },
  { "cdEject",        "CD drive",         "CDSelect",         "cmdCreateHDF",   "cboCD" },
  { "CDSelect",       "cdEject",          "Hard drives / CD", "cmdCreateHDF",   "cboCD" },
  { "cboCD",          "Hard drives / CD", "Hard drives / CD", "CD drive",       "CDVol" },
  { "CDVol",          "",                 "",                 "cboCD",          "cmdProp0" },
  
// PanelDisplay
#ifndef RASPBERRY
  { "sldWidth",       "",               "",               "Frameskip",      "sldHeight" },
  { "sldHeight",      "",               "",               "sldWidth",       "sldVertPos" },
  { "sldVertPos",     "",               "",               "sldHeight",      "Frameskip" },
  { "Frameskip",      "Display",        "Display",        "sldVertPos",     "sldWidth" },
#else
    { "sldWidth",       "",             "",               "Frameskip",      "sldHeight" },
    { "sldHeight",      "",             "",               "sldWidth",       "sldVertPos" },
    { "sldVertPos",     "",             "",               "sldHeight",      "FSRatio" },
    { "FSRatio",        "",             "",               "sldVertPos",     "4by3Ratio" },
    { "4by3Ratio",      "Display",      "Display",        "FSRatio",        "Frameskip" },
    { "Frameskip",      "Display",      "Display",        "4by3Ratio",      "sldWidth" },
#endif

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
  { "cboPort0",       "Input",          "Input",          "cboLeft",        "cboPort1" },
  { "cboPort1",       "Input",          "cboAutofire",    "cboPort0",       "MouseSpeed" },
  { "cboAutofire",    "cboPort1",       "Input",          "cboPort0",       "cboTapDelay" },
  { "MouseSpeed",     "",               "",               "cboPort1",       "MouseHack" },
  { "MouseHack",      "Input",          "cboTapDelay",    "MouseSpeed",     "CustomCtrl" },
  { "cboTapDelay",    "MouseHack",      "Input",          "cboAutofire",    "cboB" },
  { "CustomCtrl",     "Input",          "Input",          "MouseHack",      "cboA" },
  { "cboA",           "Input",          "cboB",           "CustomCtrl",     "cboX" },
  { "cboB",           "cboA",           "Input",          "cboTapDelay",    "cboY" },
  { "cboX",           "Input",          "cboY",           "cboA",           "cboL" },
  { "cboY",           "cboX",           "Input",          "cboB",           "cboR" },
  { "cboL",           "Input",          "cboR",           "cboX",           "cboUp" },
  { "cboR",           "cboL",           "Input",          "cboY",           "cboDown" },
  { "cboUp",          "Input",          "cboDown",        "cboL",           "cboLeft" },
  { "cboDown",        "cboUp",          "Input",          "cboR",           "cboRight" },
  { "cboLeft",        "Input",          "cboRight",       "cboUp",          "cboPort0" },
  { "cboRight",       "cboLeft",        "Input",          "cboDown",        "cboPort0" },

// PanelMisc
#ifndef RASPBERRY
  { "StatusLine",     "Miscellaneous",  "Miscellaneous",  "MasterWP",       "HideIdle" },
  { "HideIdle",       "Miscellaneous",  "Miscellaneous",  "StatusLine",     "ShowGUI" },
  { "ShowGUI",        "Miscellaneous",  "Miscellaneous",  "HideIdle",       "PandSpeed" },
  { "PandSpeed",      "",               "",               "ShowGUI",        "BSDSocket" },
  { "BSDSocket",      "Miscellaneous",  "Miscellaneous",  "PandSpeed",      "MasterWP" },
  { "MasterWP",       "Miscellaneous",  "Miscellaneous",  "BSDSocket",      "StatusLine" },
#else
  { "StatusLine",     "Miscellaneous",  "Miscellaneous",  "scrolllock",      "HideIdle" },
  { "HideIdle",       "Miscellaneous",  "Miscellaneous",  "StatusLine",     "ShowGUI" },
  { "ShowGUI",        "Miscellaneous",  "Miscellaneous",  "HideIdle",       "BSDSocket" },
  { "BSDSocket",      "Miscellaneous",  "Miscellaneous",  "ShowGUI",        "numlock" },
  { "MasterWP",       "Miscellaneous",  "Miscellaneous",  "BSDSocket",      "numlock" },
	{ "numlock",		    "Miscellaneous",	"scrolllock",	    "MasterWP",	      "StatusLine"},
	{ "scrolllock",		  "numlock",			  "Miscellaneous",  "MasterWP",	      "StatusLine"},
#endif
  
// PanelSavestate
  { "State0",         "Savestates",     "Savestates",     "LoadState",      "State1" },
  { "State1",         "Savestates",     "Savestates",     "State0",         "State2" },
  { "State2",         "Savestates",     "Savestates",     "State1",         "State3" },
  { "State3",         "Savestates",     "Savestates",     "State2",         "LoadState" },
  { "LoadState",      "Savestates",     "SaveState",      "State3",         "State0" },
  { "SaveState",      "LoadState",      "Savestates",     "State3",         "State0" },


//  active            move left         move right        move up           move down
// EditFilesysVirtual
  { "virtDev",        "virtRW",         "virtRW",         "virtOK",         "virtVol" },
  { "virtVol",        "virtBootpri",    "virtAutoboot",   "virtDev",        "virtPath" },
  { "virtPath",       "",               "",               "virtBootpri",    "virtCancel" },
  { "virtRW",         "virtDev",        "virtDev",        "virtOK",         "virtAutoboot" },
  { "virtAutoboot",   "virtVol",        "virtBootpri",    "virtRW",         "virtPath" },
  { "virtBootpri",    "virtAutoboot",   "virtVol",        "virtRW",         "virtPath" },
  { "virtOK",         "virtCancel",     "virtCancel",     "virtPath",       "virtRW" },
  { "virtCancel",     "virtOK",         "virtOK",         "virtPath",       "virtRW" },

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
  gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
  gcn::Widget* focusTarget = NULL;
    
  if(focusHdl != NULL)
  {
    gcn::Widget* activeWidget = focusHdl->getFocused();
    
    if(activeWidget != NULL && activeWidget->getId().length() > 0)
    {
      std::string activeName = activeWidget->getId();
      bool bFoundEnabled = false;
      int tries = 10;
      
      while(!bFoundEnabled && tries > 0)
      {
        std::string searchFor = "";
        
        for(int i=0; navMap[i].activeWidget != "END"; ++i)
        {
          if(navMap[i].activeWidget == activeName)
          {
            switch(direction)
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
            }
            if(searchFor.length() > 0)
            {
              focusTarget = gui_top->findWidgetById(searchFor);
              if(focusTarget != NULL)
              {
                if(focusTarget->isEnabled() && focusTarget->isVisible())
                  bFoundEnabled = true;
                else
                  activeName = searchFor;
              }
            }
            break;
          }
        }
        if(searchFor == "")
          bFoundEnabled = true; // No entry to navigate to -> exit loop
        --tries;
      }
      
      if(focusTarget != NULL)
      {
        if(!activeWidget->getId().substr(0, 3).compare("cbo") || !activeWidget->getId().substr(0, 5).compare("qscbo"))
        {
          gcn::UaeDropDown *dropdown = (gcn::UaeDropDown *) activeWidget;
          if(dropdown->isDroppedDown() && (direction == DIRECTION_UP || direction == DIRECTION_DOWN))
            focusTarget = NULL; // Up/down navigates in list if dropped down
        }
      }
    }
  }   

  if(focusTarget != NULL)
    focusTarget->requestFocus();
  return (focusTarget != NULL);
}
