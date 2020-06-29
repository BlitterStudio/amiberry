#include <stdbool.h>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include <guisan/sdl/sdltruetypefont.hpp>
#include "SelectorEntry.hpp"
#include "sysdeps.h"
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
	{"About", "", "", "Shutdown", "Paths"},
	{"Paths", "SystemROMs", "SystemROMs", "About", "Quickstart"},
	{"Quickstart", "qsNTSC", "cboAModel", "Paths", "Configurations"},
	{"Configurations", "ConfigList", "ConfigList", "Quickstart", "CPU and FPU"},
	{"CPU and FPU", "7 Mhz", "68000", "Configurations", "Chipset"},
	{"Chipset", "Fast copper", "OCS", "CPU and FPU", "ROM"},
	{"ROM", "MainROM", "cboMainROM", "Chipset", "RAM"},
	{"RAM", "Chipmem", "Chipmem", "ROM", "Floppy drives"},
	{"Floppy drives", "cmdSel0", "DF0:", "RAM", "Hard drives/CD"},
	{"Hard drives/CD", "cmdCreateHDF", "cmdAddDir", "Floppy drives", "Display"},
	{"Display", "sldWidth", "sldWidth", "Hard drives/CD", "Sound"},
	{"Sound", "sndDisable", "sndDisable", "Display", "Input"},
	{"Input", "cboPort0mode", "cboPort0", "Sound", "Custom controls"},
	{"Custom controls", "Right Trigger", "0: Mouse", "Input", "Miscellaneous"},
	{"Miscellaneous", "StatusLine", "StatusLine", "Custom controls", "Savestates"},
#ifdef ANDROID
{ "Savestates",       "State0",         "State0",         "Miscellaneous",  "OnScreen" },
{ "OnScreen",         "OnScrButton3",   "OnScrCtrl",      "Savestates",     "Shutdown" },
{ "Quit",             "Start",          "Help",           "OnScreen",       "Paths" },
{ "Help",             "Quit",           "Start",          "OnScreen",       "Paths" },
{ "Start",            "Help",           "Quit",           "OnScreen",       "Paths" },
#else
	{"Savestates", "State0", "State0", "Miscellaneous", "Shutdown"},
	{"Shutdown", "Start", "Quit", "Savestates", "Paths"},
	{"Quit", "Shutdown", "Restart", "Savestates", "Paths"},
	{"Restart", "Quit", "Help", "Savestates", "Paths"},
	{"Help", "Restart", "Reset", "Savestates", "Paths"},
	{"Reset", "Help", "Start", "Savestates", "Paths"},
	{"Start", "Reset", "Shutdown", "Savestates", "Paths"},
#endif

	// PanelPaths
	{"SystemROMs", "Paths", "Paths", "RescanROMs", "ConfigPath"},
	{"ConfigPath", "Paths", "Paths", "SystemROMs", "ControllersPath"},
	{"ControllersPath", "Paths", "Paths", "ConfigPath", "RetroArchFile"},
	{"RetroArchFile", "Paths", "Paths", "ControllersPath", "chkEnableLogging"},
	{"chkEnableLogging", "Paths", "Paths", "RetroArchFile", "cmdLogfilePath"},
	{"cmdLogfilePath", "Paths", "Paths", "chkEnableLogging", "RescanROMs"},
	{"RescanROMs", "Paths", "DownloadXML", "cmdLogfilePath", "SystemROMs"},
	{"DownloadXML", "RescanROMs", "Paths", "cmdLogfilePath", "SystemROMs"},

	//  active            move left         move right        move up           move down
	// PanelQuickstart
	{"cboAModel", "Quickstart", "qsNTSC", "cboWhdload", "cboAConfig"},
	{"qsNTSC", "cboAModel", "Quickstart", "qsMode", "cboAConfig"},
	{"cboAConfig", "Quickstart", "Quickstart", "cboAModel", "qscmdSel0"},
	{"qsDF0", "Quickstart", "qsWP0", "cboAConfig", "cboDisk0"},
	{"qsWP0", "qsDF0", "qscmdEject0", "cboAConfig", "cboDisk0"},
	//  { "qsInfo0",        "Quickstart",     "",     "",               "" },
	{"qscmdEject0", "qsWP0", "qscmdSel0", "cboAConfig", "cboDisk0"},
	{"qscmdSel0", "qscmdEject0", "Quickstart", "cboAConfig", "cboDisk0"},
	{"cboDisk0", "Quickstart", "Quickstart", "qscmdSel0", "qscmdSel1"},
	{"qsDF1", "Quickstart", "qsWP1", "cboDisk0", "cboDisk1"},
	{"qsWP1", "qsDF1", "qscmdEject1", "cboDisk0", "cboDisk1"},
	//  { "qsInfo1",        "Quickstart",     "",     "",               "" },
	{"qscmdEject1", "qsWP1", "qscmdSel1", "cboDisk0", "cboDisk1"},
	{"qscmdSel1", "qscmdEject1", "Quickstart", "cboDisk0", "cboDisk1"},
	{"cboDisk1", "Quickstart", "Quickstart", "qsDF1", "qsCDSelect"},
	{"qsCD drive", "Quickstart", "qscdEject", "cboDisk1", "cboCD"},
	{"qscdEject", "qsCD drive", "qsCDSelect", "cboDisk1", "cboCD"},
	{"qsCDSelect", "qscdEject", "Quickstart", "cboDisk1", "cboCD"},
	{"cboCD", "Quickstart", "Quickstart", "qsCDSelect", "qsMode"},
	{"qsMode", "Quickstart", "Quickstart", "cboCD", "cmdSetConfig"},
	{"cmdSetConfig", "Quickstart", "Quickstart", "qsMode", "cmdWhdloadEject"},
	{"cmdWhdloadEject", "Quickstart", "cmdWhdloadSelect", "cmdSetConfig", "cboWhdload"},
	{"cmdWhdloadSelect", "cmdWhdloadEject", "Quickstart", "cmdSetConfig", "cboWhdload"},
	{"cboWhdload", "Quickstart", "Quickstart", "cmdWhdloadEject", "cboAModel"},

	// PanelConfig
	{"ConfigList", "Configurations", "ConfigLoad", "", ""},
	{"ConfigLoad", "Configurations", "ConfigSave", "ConfigList", "ConfigList"},
	{"ConfigSave", "ConfigLoad", "CfgDelete", "ConfigList", "ConfigList"},
	{"CfgDelete", "ConfigSave", "Configurations", "ConfigList", "ConfigList"},

	//  active            move left         move right        move up           move down
	// PanelCPU
	{"68000", "CPU and FPU", "FPUnone", "JIT", "68010"},
	{"68010", "CPU and FPU", "68881", "68000", "68020"},
	{"68020", "CPU and FPU", "68882", "68010", "68030"},
	{"68030", "CPU and FPU", "CPU internal", "68020", "68040"},
	{"68040", "CPU and FPU", "FPUstrict", "68030", "CPU24Bit"},
	{"CPU24Bit", "CPU and FPU", "FPUJIT", "68040", "CPUComp"},
	{"CPUComp", "CPU and FPU", "FPUJIT", "CPU24Bit", "JIT"},
	{"JIT", "CPU and FPU", "FPUJIT", "CPUComp", "68000"},
	{"FPUnone", "68000", "7 Mhz", "FPUJIT", "68881"},
	{"68881", "68010", "14 Mhz", "FPUnone", "68882"},
	{"68882", "68020", "25 Mhz", "68881", "CPU internal"},
	{"CPU internal", "68030", "Fastest", "68882", "FPUstrict"},
	{"FPUstrict", "68040", "Fastest", "CPU internal", "FPUJIT"},
	{"FPUJIT", "CPU24Bit", "Fastest", "FPUstrict", "FPUnone"},
	{"7 Mhz", "FPUnone", "CPU and FPU", "sldCpuIdle", "14 Mhz"},
	{"14 Mhz", "FPUnone", "CPU and FPU", "7 Mhz", "25 Mhz"},
	{"25 Mhz", "FPUnone", "CPU and FPU", "14 Mhz", "Fastest"},
	{"Fastest", "FPUnone", "CPU and FPU", "25 Mhz", "sldCpuIdle"},
	{"sldCpuIdle", "", "", "Fastest", "7 Mhz"},

	// PanelChipset
	{"OCS", "Chipset", "ChipsetExtra", "CollFull", "ECS Agnus"},
	{"ECS Agnus", "Chipset", "Immediate", "OCS", "Full ECS"},
	{"Full ECS", "Chipset", "BlitWait", "ECS Agnus", "AGA"},
	{"AGA", "Chipset", "Fast copper", "Full ECS", "NTSC"},
	{"NTSC", "Chipset", "Fast copper", "AGA", "CollNone"},
	{"ChipsetExtra", "OCS", "BlitNormal", "", ""},
	{"BlitNormal", "ChipsetExtra", "Chipset", "Fast copper", "Immediate"},
	{"Immediate", "ECS Agnus", "Chipset", "BlitNormal", "BlitWait"},
	{"BlitWait", "Full ECS", "Chipset", "Immediate", "Fast copper"},
	{"Fast copper", "NTSC", "Chipset", "BlitWait", "BlitNormal"},
	{"CollNone", "Chipset", "Chipset", "NTSC", "Sprites only"},
	{"Sprites only", "Chipset", "Chipset", "CollNone", "CollPlay"},
	{"CollPlay", "Chipset", "Chipset", "Sprites only", "CollFull"},
	{"CollFull", "Chipset", "Chipset", "CollPlay", "OCS"},

	//  active            move left         move right        move up           move down
	// PanelROM
	{"cboMainROM", "ROM", "MainROM", "cboCartROM", "cboExtROM"},
	{"MainROM", "cboMainROM", "ROM", "CartROM", "ExtROM"},
	{"cboExtROM", "ROM", "ExtROM", "cboMainROM", "cboCartROM"},
	{"ExtROM", "cboExtROM", "ROM", "MainROM", "CartROM"},
	{"cboCartROM", "ROM", "CartROM", "cboExtROM", "cboUAEROM"},
	{"CartROM", "cboCartROM", "ROM", "ExtROM", "cboUAEROM"},
	{ "cboUAEROM", "ROM", "ROM", "cboCartROM", "cboMainROM"},

	//PanelRAM
	{"Chipmem", "", "", "RAM", "Slowmem"},
	{"Slowmem", "", "", "Chipmem", "Fastmem"},
	{"Fastmem", "", "", "Slowmem", "Z3mem"},
	{"Z3mem", "", "", "Fastmem", "Gfxmem"},
	{"Gfxmem", "", "", "Z3mem", "A3000Low"},
	{"A3000Low", "", "", "Gfxmem", "A3000High"},
	{"A3000High", "", "", "A3000Low", "Chipmem"},

	//PanelFloppy
	{"DF0:", "Floppy drives", "cboType0", "SaveForDisk", "cboDisk0"},
	{"cboType0", "DF0:", "cmdEject0", "SaveForDisk", "cboDisk0"},
	{"cmdEject0", "cboType0", "cmdSel0", "CreateHD", "cboDisk0"},
	{"cmdSel0", "cmdEject0", "Floppy drives", "CreateHD", "cboDisk0"},
	{"cboDisk0", "Floppy drives", "Floppy drives", "DF0:", "LoadDiskCfg"},
	{"LoadDiskCfg", "Floppy drives", "Floppy drives", "cboDisk0", "DF1:"},
	{"DF1:", "Floppy drives", "cboType1", "LoadDiskCfg", "cboDisk1"},
	{"cboType1", "DF1:", "cmdEject1", "LoadDiskCfg", "cboDisk1"},
	{"cmdEject1", "cboType1", "cmdSel1", "LoadDiskCfg", "cboDisk1"},
	{"cmdSel1", "cmdEject1", "Floppy drives", "LoadDiskCfg", "cboDisk1"},
	{"cboDisk1", "Floppy drives", "Floppy drives", "DF1:", "DF2:"},
	{"DF2:", "Floppy drives", "cboType2", "cboDisk1", "cboDisk2"},
	{"cboType2", "DF2:", "cmdEject2", "cboDisk1", "cboDisk2"},
	{"cmdEject2", "cboType2", "cmdSel2", "cboDisk1", "cboDisk2"},
	{"cmdSel2", "cmdEject2", "Floppy drives", "cboDisk1", "cboDisk2"},
	{"cboDisk2", "Floppy drives", "Floppy drives", "DF2:", "DF3:"},
	{"DF3:", "Floppy drives", "cboType3", "cboDisk2", "cboDisk3"},
	{"cboType3", "DF3:", "cmdEject3", "cboDisk2", "cboDisk3"},
	{"cmdEject3", "cboType3", "cmdSel3", "cboDisk2", "cboDisk3"},
	{"cmdSel3", "cmdEject3", "Floppy drives", "cboDisk2", "cboDisk3"},
	{"cboDisk3", "Floppy drives", "Floppy drives", "DF3:", "DriveSpeed"},
	{"DriveSpeed", "", "", "cboDisk3", "CreateDD"},
	{"SaveForDisk", "Floppy drives", "CreateDD", "DriveSpeed", "DF0:"},
	{"CreateDD", "SaveForDisk", "CreateHD", "DriveSpeed", "cboType0"},
	{"CreateHD", "CreateDD", "Floppy drives", "DriveSpeed", "cmdEject0"},

	//  active            move left           move right          move up           move down
	// PanelHD
	{"cmdProp0", "Hard drives/CD", "cmdDel0", "CDVol", "cmdProp1"},
	{"cmdDel0", "cmdProp0", "Hard drives/CD", "CDVol", "cmdDel1"},
	{"cmdProp1", "Hard drives/CD", "cmdDel1", "cmdProp0", "cmdProp2"},
	{"cmdDel1", "cmdProp1", "Hard drives/CD", "cmdDel0", "cmdDel2"},
	{"cmdProp2", "Hard drives/CD", "cmdDel2", "cmdProp1", "cmdProp3"},
	{"cmdDel2", "cmdProp2", "Hard drives/CD", "cmdDel1", "cmdDel3"},
	{"cmdProp3", "Hard drives/CD", "cmdDel3", "cmdProp2", "cmdProp4"},
	{"cmdDel3", "cmdProp3", "Hard drives/CD", "cmdDel2", "cmdDel4"},
	{"cmdProp4", "Hard drives/CD", "cmdDel4", "cmdProp3", "cmdAddDir"},
	{"cmdDel4", "cmdProp4", "Hard drives/CD", "cmdDel3", "cmdAddHDF"},
	{"cmdAddDir", "Hard drives/CD", "cmdAddHDF", "cmdProp4", "chkHDRO"},
	{"cmdAddHDF", "cmdAddDir", "cmdCreateHDF", "cmdDel4", "chkHDRO"},
	{"cmdCreateHDF", "cmdAddHDF", "Hard drives / CD", "cmdDel4", "chkSCSI" },
	{"chkHDRO", "Hard drives / CD", "chkSCSI", "cmdAddDir", "CD drive" },
	{"chkSCSI", "chkHDRO", "Hard drives / CD", "cmdCreateHDF", "cdEject" },
	{"CD drive", "Hard drives/CD", "cdEject", "chkHDRO", "cboCD"},
	{"cdEject", "CD drive", "CDSelect", "chkSCSI", "cboCD" },
	{"CDSelect", "cdEject", "Hard drives/CD", "cmdCreateHDF", "cboCD"},
	{"cboCD", "Hard drives/CD", "Hard drives/CD", "CD drive", "CDVol"},
	{"CDVol", "", "", "cboCD", "cmdProp0"},

	//  active            move left           move right          move up           move down
	// PanelDisplay
	{"sldWidth", "", "", "Frameskip", "sldHeight"},
	{"sldHeight", "", "", "sldWidth", "chkAutoHeight"},
	{"chkAutoHeight", "Display", "Horizontal", "sldHeight", "Auto"},
	{"Auto", "Display", "Single", "chkAutoHeight", "Nearest Neighbor (pixelated)"},
	{"Nearest Neighbor (pixelated)", "Display", "Double", "Auto", "Linear (smooth)"},
	{"Linear (smooth)", "Display", "Scanlines", "Nearest Neighbor (pixelated)", "CorrectAR"},
	{"Single", "Auto", "Auto", "Vertical", "Double"},
	{"Double", "Nearest Neighbor (pixelated)", "Nearest Neighbor (pixelated)", "Single", "Scanlines"},
	{"Scanlines", "Linear (smooth)", "Linear (smooth)", "Double", "cboScreenmode"},
	{"CorrectAR", "Display", "cboScreenmode", "Linear (smooth)", "Frameskip"},
	{"cboScreenmode", "CorrectAR", "CorrectAR", "Scanlines", "Frameskip"},
	{"Frameskip", "Display", "Display", "CorrectAR", "sldWidth"},
	{"Vertical", "", "", "Horizontal", "Single"},
	{"Horizontal", "chkAutoHeight", "", "", "Vertical"},

	//  active            move left           move right          move up           move down
	//PanelSound
	{"sndDisable", "Sound", "Mono", "sldPaulaVol", "sndDisEmu"},
	{"sndDisEmu", "Sound", "Stereo", "sndDisable", "sndEmulate"},
	{"sndEmulate", "Sound", "Stereo", "sndDisEmu", "sndEmuBest"},
	{"sndEmuBest", "Sound", "Stereo", "sndEmulate", "cboFrequency"},
	{"Mono", "sndDisable", "Sound", "sldStereoDelay", "Stereo"},
	{"Stereo", "sndDisEmu", "Sound", "Mono", "cboFrequency"},
	{"cboFrequency", "Sound", "Sound", "sndEmuBest", "cboInterpol"},
	{"cboInterpol", "Sound", "Sound", "cboFrequency", "cboFilter"},
	{"cboFilter", "Sound", "Sound", "cboInterpol", "sldSeparation"},
	{"sldSeparation", "", "", "cboFilter", "sldStereoDelay"},
	{"sldStereoDelay", "", "", "sldSeparation", "sldPaulaVol"},
	{"sldPaulaVol", "", "", "sldStereoDelay", "sndDisable"},

	//  active            move left           move right          move up           move down
	// PanelInput
	{"cboPort0", "Input", "cboPort0mode", "cboAutofire", "cboPort1"},
	{"cboPort0mode", "cboPort0", "Input", "cboTapDelay", "cboPort1mode"},
	{"cboPort1", "Input", "cboPort1mode", "cboPort0", "cboPort2"},
	{"cboPort1mode", "cboPort1", "Input", "cboPort0mode", "cboPort2"},
	{"cboPort2", "Input", "cboPort2mode", "cboPort1", "cboPort3"},
	{"cboPort3", "Input", "cboPort3mode", "cboPort2", "cboPort0mousemode"},
	{"cboPort0mousemode", "Input", "MouseSpeed", "cboPort3", "cboPort1mousemode"},
	{"cboPort1mousemode", "Input", "MouseSpeed", "cboPort0mousemode", "cboAutofire"},
	{"MouseSpeed", "", "", "cboPort3", "MouseHack"},
	{"MouseHack", "cboAutofire", "cboAutofire", "MouseSpeed", "cboPort0"},
	{"cboAutofire", "Input", "MouseHack", "cboPort1mousemode", "cboPort0"},

	// PanelCustom
	{"0: Mouse", "Custom controls", "1: Joystick", "chkAnalogRemap", "None"},
	{"1: Joystick", "0: Mouse", "2: Parallel 1", "chkAnalogRemap", "HotKey"},
	{"2: Parallel 1", "1: Joystick", "3: Parallel 2", "chkAnalogRemap", "Left Trigger"},
	{"3: Parallel 2", "2: Parallel 1", "Custom controls", "chkAnalogRemap", "Right Trigger"},

	{"None", "Custom controls", "HotKey", "0: Mouse", "cboCustomAction0"},
	{"HotKey", "None", "Left Trigger", "1: Joystick", "cboCustomAction0"},
	{"Left Trigger", "HotKey", "Right Trigger", "2: Parallel 1", "cboCustomAction0"},
	{"Right Trigger", "Left Trigger", "Custom controls", "3: Parallel 2", "cboCustomAction0"},

	{"cboCustomAction0", "Custom controls", "cboCustomAction7", "None", "cboCustomAction1"},
	{"cboCustomAction1", "Custom controls", "cboCustomAction8", "cboCustomAction0", "cboCustomAction2"},
	{"cboCustomAction2", "Custom controls", "cboCustomAction9", "cboCustomAction1", "cboCustomAction3"},
	{"cboCustomAction3", "Custom controls", "cboCustomAction10", "cboCustomAction2", "cboCustomAction4"},
	{"cboCustomAction4", "Custom controls", "cboCustomAction11", "cboCustomAction3", "cboCustomAction5"},
	{"cboCustomAction5", "Custom controls", "cboCustomAction12", "cboCustomAction4", "cboCustomAction6"},
	{"cboCustomAction6", "Custom controls", "cboCustomAction13", "cboCustomAction5", "cboCustomAction7"},

	{"cboCustomAction7", "cboCustomAction0", "Custom controls", "cboCustomAction6", "cboCustomAction8"},
	{"cboCustomAction8", "cboCustomAction1", "Custom controls", "cboCustomAction7", "cboCustomAction9"},
	{"cboCustomAction9", "cboCustomAction2", "Custom controls", "cboCustomAction8", "cboCustomAction10"},
	{"cboCustomAction10", "cboCustomAction3", "Custom controls", "cboCustomAction9", "cboCustomAction11"},
	{"cboCustomAction11", "cboCustomAction4", "Custom controls", "cboCustomAction10", "cboCustomAction12"},
	{"cboCustomAction12", "cboCustomAction5", "Custom controls", "cboCustomAction11", "cboCustomAction13"},
	{"cboCustomAction13", "cboCustomAction6", "Custom controls", "cboCustomAction12", "chkAnalogRemap"},

	{"chkAnalogRemap", "Custom controls", "Custom controls", "cboCustomAction13", "0: Mouse"},

	// PanelMisc
	//  active            move left           move right          move up           move down

	{"StatusLine", "Miscellaneous", "RetroArchQuit", "cboScrolllock", "ShowGUI"},
	{"ShowGUI", "Miscellaneous", "RetroArchMenu", "StatusLine", "chkMouseUntrap"},
	{"chkMouseUntrap", "Miscellaneous", "RetroArchReset", "ShowGUI", "BSDSocket"},
	{"RetroArchQuit", "StatusLine", "Miscellaneous", "KeyForQuit", "RetroArchMenu"},
	{"RetroArchMenu", "ShowGUI", "Miscellaneous", "RetroArchQuit", "RetroArchReset"},
	{"RetroArchReset", "chkMouseUntrap", "Miscellaneous", "RetroArchMenu", "BSDSocket"},

	{"BSDSocket", "Miscellaneous", "Miscellaneous", "chkMouseUntrap", "MasterWP"},
	{"MasterWP", "Miscellaneous", "Miscellaneous", "BSDSocket", "cboNumlock"},
	{"cboNumlock", "Miscellaneous", "cboScrolllock", "MasterWP", "OpenGUI"},
	{"cboScrolllock", "cboNumlock", "Miscellaneous", "MasterWP", "KeyForQuit"},
	{"OpenGUI", "Miscellaneous", "KeyForQuit", "cboNumlock", "KeyActionReplay"},
	{"KeyForQuit", "OpenGUI", "Miscellaneous", "cboScrolllock", "KeyFullScreen"},
	{"KeyActionReplay", "Miscellaneous", "KeyFullScreen", "OpenGUI", "StatusLine"},
	{"KeyFullScreen", "KeyActionReplay", "KeyActionReplay", "KeyForQuit", "RetroArchQuit"},

	// PanelSavestate
	{"State0", "Savestates", "Savestates", "LoadState", "State1"},
	{"State1", "Savestates", "Savestates", "State0", "State2"},
	{"State2", "Savestates", "Savestates", "State1", "State3"},
	{"State3", "Savestates", "Savestates", "State2", "LoadState"},
	{"LoadState", "Savestates", "SaveState", "State3", "State0"},
	{"SaveState", "LoadState", "Savestates", "State3", "State0"},

#ifdef ANDROID
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
{ "DisableMenuVKeyb","OnScreen",       "CustomPos",    "FloatJoy",       "Shutdown" },
#endif

	//  active            move left         move right        move up           move down
	// EditFilesysVirtual
	{"virtPath", "", "", "txtVirtBootPri", "virtCancel"},
	{"virtRW", "txtVirtDev", "txtVirtDev", "virtOK", "virtAutoboot"},
	{"virtAutoboot", "txtVirtVol", "txtVirtBootPri", "virtRW", "virtPath"},
	{"virtOK", "virtCancel", "virtCancel", "virtPath", "virtRW"},
	{"virtCancel", "virtOK", "virtOK", "virtPath", "virtRW"},

	// EditFilesysHardfile
	{"hdfRW", "txtHdfDev", "hdfAutoboot", "hdfOK", "hdfPath"},
	{"hdfAutoboot", "hdfRW", "txtHdfBootPri", "hdfOK", "hdfPath"},
	{"hdfPath", "txtHdfPath", "txtHdfPath", "txtHdfBootPri", "txtHdfReserved"},
	{"hdfController", "hdfUnit", "hdfUnit", "txtHdfSectors", "hdfOK"},
	{"hdfUnit", "hdfController", "hdfController", "hdfBlocksize", "hdfOK"},
	{"hdfOK", "hdfCancel", "hdfCancel", "hdfUnit", "txtHdfBootPri"},
	{"hdfCancel", "hdfOK", "hdfOK", "hdfUnit", "txtHdfBootPri"},

	// CreateFilesysHardfile
	{"createHdfAutoboot", "createHdfPath", "createHdfPath", "createHdfOK", "createHdfOK"},
	{"createHdfPath", "createHdfAutoboot", "createHdfOK", "createHdfAutoboot", "createHdfOK"},
	{"createHdfOK", "createHdfCancel", "createHdfCancel", "createHdfPath", "createHdfAutoboot"},
	{"createHdfCancel", "createHdfOK", "createHdfOK", "createHdfPath", "createHdfAutoboot"},

	{"END", "", "", "", ""}
};


bool HandleNavigation(int direction)
{
	gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
	gcn::Widget* focusTarget = nullptr;

	if (focusHdl != nullptr)
	{
		gcn::Widget* activeWidget = focusHdl->getFocused();

		if (activeWidget != nullptr 
			&& activeWidget->getId().length() > 0 
			&& activeWidget->getId().substr(0, 3) != "txt")
		{
			std::string activeName = activeWidget->getId();
			auto bFoundEnabled = false;
			auto tries = 10;

			while (!bFoundEnabled && tries > 0)
			{
				std::string searchFor;

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
								if (focusTarget->isEnabled() && focusTarget->isVisible())
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
				if (activeWidget->getId().substr(0, 3) == "cbo")
				{
					auto* dropdown = dynamic_cast<gcn::DropDown*>(activeWidget);
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

void PushFakeKey(const SDL_Keycode inKey)
{
	SDL_Event event;

	event.type = SDL_KEYDOWN; // and the key up
	event.key.keysym.sym = inKey;
	gui_input->pushInput(event); // Fire key down
	event.type = SDL_KEYUP; // and the key up
	gui_input->pushInput(event); // Fire key down
}
