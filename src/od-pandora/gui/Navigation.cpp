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
#include "target.h"
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
//  active            move left         move right        move up           move down
// main_window
  { "Paths",          "SystemROMs",     "SystemROMs",     "Reset",          "Configurations" },
  { "Configurations", "ConfigList",     "ConfigList",     "Paths",          "CPU" },
  { "CPU",            "7 Mhz",          "68000",          "Configurations", "Chipset" },
  { "Chipset",        "BlitNormal",     "OCS",            "CPU",            "ROM" },
  { "ROM",            "MainROM",        "cboMainROM",     "Chipset",        "RAM" },
  { "RAM",            "Chipmem",        "Chipmem",        "ROM",            "Floppy drives" },
  { "Floppy drives",  "cmdSel0",        "DF0:",           "RAM",            "Hard drives" },
  { "Hard drives",    "cmdDel0",        "cmdProp0",       "Floppy drives",  "Display" },
  { "Display",        "sldWidth",       "sldWidth",       "Hard drives",    "Sound" },
  { "Sound",          "sndDisable",     "sndDisable",     "Display",        "Input" },
  { "Input",          "cboCtrlConfig",  "cboCtrlConfig",  "Sound",          "Miscellaneous" },
  { "Miscellaneous",  "Status Line",    "Status Line",    "Input",          "Savestates" },
  { "Savestates",     "State0",         "State0",         "Miscellaneous",  "Reset" },
  { "Reset",          "Start",          "Quit",           "Savestates",     "Paths" },
  { "Quit",           "Reset",          "Restart",        "Savestates",     "Paths" },
  { "Restart",        "Quit",           "Start",          "Savestates",     "Paths" },
  { "Start",          "Restart",        "Reset",          "Savestates",     "Paths" },

// PanelPaths
  { "SystemROMs",     "Paths",          "Paths",          "RescanROMs",     "ConfigPath" },
  { "ConfigPath",     "Paths",          "Paths",          "SystemROMs",     "RescanROMs" },
  { "RescanROMs",     "Paths",          "Paths",          "ConfigPath",     "SystemROMs" },

// PanelConfig
  { "ConfigList",     "Configurations", "ConfigName",     "",               "" },
  { "ConfigName",     "Configurations", "Configurations", "ConfigList",     "ConfigDesc" },
  { "ConfigDesc",     "Configurations", "Configurations", "ConfigName",     "ConfigLoad" },
  { "ConfigLoad",     "Configurations", "ConfigSave",     "ConfigDesc",     "ConfigList" },
  { "ConfigSave",     "ConfigLoad",     "CfgDelete",      "ConfigDesc",     "ConfigList" },
  { "CfgDelete",      "ConfigSave",     "Configurations", "ConfigDesc",     "ConfigList" },

//  active            move left         move right        move up           move down
// PanelCPU
  { "68000",          "CPU",            "FPUnone",        "JIT",            "68010" },
  { "68010",          "CPU",            "68881",          "68000",          "68EC020" },
  { "68EC020",        "CPU",            "68882",          "68010",          "68020" },
  { "68020",          "CPU",            "CPU internal",   "68EC020",        "68040" },
  { "68040",          "CPU",            "CPU internal",   "68020",          "CPUComp" },
  { "CPUComp",        "CPU",            "CPU internal",   "68040",          "JIT" },
  { "JIT",            "CPU",            "CPU internal",   "CPUComp",        "68000" },
  { "FPUnone",        "68000",          "7 Mhz",          "CPU internal",   "68881" },
  { "68881",          "68010",          "14 Mhz",         "FPUnone",        "68882" },
  { "68882",          "68EC020",        "28 Mhz",         "68881",          "CPU internal" },
  { "CPU internal",   "68020",          "Fastest",        "68882",          "FPUnone" },
  { "7 Mhz",          "FPUnone",        "CPU",            "Fastest",        "14 Mhz" },
  { "14 Mhz",         "68881",          "CPU",            "7 Mhz",          "28 Mhz" },
  { "28 Mhz",         "68882",          "CPU",            "14 Mhz",         "Fastest" },
  { "Fastest",        "CPU internal",   "CPU",            "28 Mhz",         "7 Mhz" },

// PanelChipset
  { "OCS",            "Chipset",        "BlitNormal",     "CollFull",       "ECS" },
  { "ECS",            "Chipset",        "Immediate",      "OCS",            "AGA" },
  { "AGA",            "Chipset",        "Improved",       "ECS",            "NTSC" },
  { "NTSC",           "Chipset",        "Chipset",        "AGA",            "CollNone" },
  { "BlitNormal",     "OCS",            "Chipset",        "CollFull",       "Immediate" },
  { "Immediate",      "ECS",            "Chipset",        "BlitNormal",     "Improved" },
  { "Improved",       "AGA",            "Chipset",        "Immediate",      "CollNone" },
  { "CollNone",       "Chipset",        "Chipset",        "NTSC",           "Sprites only" },
  { "Sprites only",   "Chipset",        "Chipset",        "CollNone",       "CollPlay" },
  { "CollPlay",       "Chipset",        "Chipset",        "Sprites only",   "CollFull" },
  { "CollFull",       "Chipset",        "Chipset",        "CollPlay",       "OCS" },

//  active            move left         move right        move up           move down
// PanelROM
  { "cboMainROM",     "ROM",            "MainROM",        "cboExtROM",      "cboExtROM" },
  { "MainROM",        "cboMainROM",     "ROM",            "ExtROM",         "ExtROM" },
  { "cboExtROM",      "ROM",            "ExtROM",         "cboMainROM",     "cboMainROM" },
  { "ExtROM",         "cboExtROM",      "ROM",            "MainROM",        "MainROM" },

//PanelRAM
  { "Chipmem",        "",               "",               "RAM",            "Slowmem" },
  { "Slowmem",        "",               "",               "Chipmem",        "Fastmem" },
  { "Fastmem",        "",               "",               "Slowmem",        "Z3mem" },
  { "Z3mem",          "",               "",               "Fastmem",        "Gfxmem" },
  { "Gfxmem",         "",               "",               "Z3mem",          "RAM" },

//PanelFloppy
  { "DF0:",           "Floppy drives",  "cboType0",       "SaveForDisk",     "cboDisk0" },
  { "cboType0",       "DF0:",           "cmdEject0",      "SaveForDisk",     "cboDisk0" },
  { "cmdEject0",      "cboType0",       "cmdSel0",        "SaveForDisk",     "cboDisk0" },
  { "cmdSel0",        "cmdEject0",      "Floppy drives",  "SaveForDisk",     "cboDisk0" },
  { "cboDisk0",       "Floppy drives",  "Floppy drives",  "DF0:",           "DF1:" },
  { "DF1:",           "Floppy drives",  "cboType1",       "cboDisk0",       "cboDisk1" },
  { "cboType1",       "DF1:",           "cmdEject1",      "cboDisk0",       "cboDisk1" },
  { "cmdEject1",      "cboType1",       "cmdSel1",        "cboDisk0",       "cboDisk1" },
  { "cmdSel1",        "cmdEject1",      "Floppy drives",  "cboDisk0",       "cboDisk1" },
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
  { "DriveSpeed",     "",               "",               "cboDisk3",       "SaveForDisk" },
  { "SaveForDisk",    "Floppy drives",  "Floppy drives",  "DriveSpeed",     "DF0:" },

//  active            move left         move right        move up           move down
// PanelHD
  { "cmdProp0",       "Hard drives",    "cmdDel0",        "cmdAddDir",      "cmdProp1" },
  { "cmdDel0",        "cmdProp0",       "Hard drives",    "cmdAddHDF",      "cmdDel1" },
  { "cmdProp1",       "Hard drives",    "cmdDel1",        "cmdProp0",       "cmdProp2" },
  { "cmdDel1",        "cmdProp1",       "Hard drives",    "cmdDel0",        "cmdDel2" },
  { "cmdProp2",       "Hard drives",    "cmdDel2",        "cmdProp1",       "cmdProp3" },
  { "cmdDel2",        "cmdProp2",       "Hard drives",    "cmdDel1",        "cmdDel3" },
  { "cmdProp3",       "Hard drives",    "cmdDel3",        "cmdProp2",       "cmdProp4" },
  { "cmdDel3",        "cmdProp3",       "Hard drives",    "cmdDel2",        "cmdDel4" },
  { "cmdProp4",       "Hard drives",    "cmdDel4",        "cmdProp3",       "cmdAddDir" },
  { "cmdDel4",        "cmdProp4",       "Hard drives",    "cmdDel3",        "cmdAddHDF" },
  { "cmdAddDir",      "Hard drives",    "cmdAddHDF",      "cmdProp4",       "cmdProp0" },
  { "cmdAddHDF",      "cmdAddDir",      "Hard drives",    "cmdDel4",        "cmdDel0" },

// PanelDisplay
  { "sldWidth",       "",               "",               "Frameskip",      "sldHeight" },
  { "sldHeight",      "",               "",               "sldWidth",       "sldVertPos" },
  { "sldVertPos",     "",               "",               "sldHeight",      "Frameskip" },
  { "Frameskip",      "Display",        "Display",        "sldVertPos",     "sldWidth" },

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

// PanelInput
  { "cboCtrlConfig",  "Input",          "Input",          "cboLeft",        "cboJoystick" },
  { "cboJoystick",    "Input",          "cboAutofire",    "cboCtrlConfig",  "cboTapDelay" },
  { "cboAutofire",    "cboJoystick",    "Input",          "cboCtrlConfig",  "cboTapDelay" },
  { "cboTapDelay",    "Input",          "Input",          "cboJoystick",    "MouseSpeed" },
  { "MouseSpeed",     "",               "",               "cboTapDelay",    "StylusOffset" },
  { "StylusOffset",   "",               "",               "MouseSpeed",     "cboDPAD" },
  { "cboDPAD",        "Input",          "CustomCtrl",     "StylusOffset",   "cboA" },
  { "CustomCtrl",     "cboDPAD",        "Input",          "StylusOffset",   "cboB" },
  { "cboA",           "Input",          "cboB",           "cboDPAD",        "cboX" },
  { "cboB",           "cboA",           "Input",          "CustomCtrl",     "cboY" },
  { "cboX",           "Input",          "cboY",           "cboA",           "cboL" },
  { "cboY",           "cboX",           "Input",          "cboB",           "cboR" },
  { "cboL",           "Input",          "cboR",           "cboX",           "cboUp" },
  { "cboR",           "cboL",           "Input",          "cboY",           "cboDown" },
  { "cboUp",          "Input",          "cboDown",        "cboL",           "cboLeft" },
  { "cboDown",        "cboUp",          "Input",          "cboR",           "cboRight" },
  { "cboLeft",        "Input",          "cboRight",       "cboUp",          "cboCtrlConfig" },
  { "cboRight",       "cboLeft",        "Input",          "cboDown",        "cboCtrlConfig" },

// PanelMisc
  { "Status Line",    "Miscellaneous",  "Miscellaneous",  "PandSpeed",      "ShowGUI" },
  { "ShowGUI",        "Miscellaneous",  "Miscellaneous",  "Status Line",    "PandSpeed" },
  { "PandSpeed",      "",               "",               "ShowGUI",        "Status Line" },

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
  { "virtVol",        "virtBootpri",    "virtBootpri",    "virtDev",        "virtPath" },
  { "virtPath",       "",               "",               "virtBootpri",    "virtCancel" },
  { "virtRW",         "virtDev",        "virtDev",        "virtOK",         "virtBootpri" },
  { "virtBootpri",    "virtVol",        "virtVol",        "virtRW",         "virtPath" },
  { "virtOK",         "virtCancel",     "virtCancel",     "virtPath",       "virtRW" },
  { "virtCancel",     "virtOK",         "virtOK",         "virtPath",       "virtRW" },

// EditFilesysHardfile
  { "hdfDev",         "hdfBootPri",     "hdfRW",          "hdfOK",          "hdfPath" },
  { "hdfRW",          "hdfDev",         "hdfBootPri",     "hdfOK",          "hdfPath" },
  { "hdfBootPri",     "hdfRW",          "hdfDev",         "hdfCancel",      "hdfPath" },
  { "hdfSurface",     "hdfReserved",    "hdfReserved",    "hdfPath",        "hdfSectors" },
  { "hdfReserved",    "hdfSurface",     "hdfSurface",     "hdfPath",        "hdfBlocksize" },
  { "hdfSectors",     "hdfBlocksize",   "hdfBlocksize",   "hdfSurface",     "hdfOK" },
  { "hdfBlocksize",   "hdfSectors",     "hdfSectors",     "hdfReserved",    "hdfOK" },
  { "hdfPath",        "",               "",               "hdfBootPri",     "hdfReserved" },
  { "hdfOK",          "hdfCancel",      "hdfCancel",      "hdfBlocksize",   "hdfBootPri" },
  { "hdfCancel",      "hdfOK",          "hdfOK",          "hdfBlocksize",   "hdfBootPri" },

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
      
      while(!bFoundEnabled)
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
                if(focusTarget->isEnabled())
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
      }
      
      if(focusTarget != NULL)
      {
        if(!activeWidget->getId().substr(0, 3).compare("cbo"))
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
