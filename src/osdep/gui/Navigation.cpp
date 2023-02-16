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
	{"About", "", "", "Quit", "Paths"},
	{"Paths", "cmdSystemROMs", "cmdSystemROMs", "About", "Quickstart"},
	{"Quickstart", "qsNTSC", "cboAModel", "Paths", "Configurations"},
	{"Configurations", "ConfigList", "ConfigList", "Quickstart", "CPU and FPU"},
	{"CPU and FPU", "7 Mhz", "68000", "Configurations", "Chipset"},
	{"Chipset", "chkFastCopper", "optOCS", "CPU and FPU", "ROM"},
	{"ROM", "MainROM", "cboMainROM", "Chipset", "RAM"},
	{"RAM", "sldChipmem", "sldChipmem", "ROM", "Floppy drives"},
	{"Floppy drives", "cmdSel0", "DF0:", "RAM", "Hard drives/CD"},
	{"Hard drives/CD", "cmdCreateHDF", "cmdAddDir", "Floppy drives", "Expansions"},
	{"Expansions", "chkBSDSocket", "chkBSDSocket", "Hard drives/CD", "RTG board"},
	{"RTG board", "cboBoard", "cboBoard", "Expansions", "Hardware info"},
	{"Hardware info", "", "", "RTG board", "Display"},
	{"Display", "cboScreenmode", "cboScreenmode", "Hardware info", "Sound"},
	{"Sound", "cboSoundcard", "cboSoundcard", "Display", "Input"},
	{"Input", "cboPort0mode", "cboPort0", "Sound", "IO Ports"},
	{"IO Ports", "txtSerialDevice", "txtSerialDevice", "Input", "Custom controls"},
	{"Custom controls", "Right Trigger", "0: Mouse", "IO Ports", "Disk swapper"},
	{"Disk swapper", "cmdDiskSwapperDrv0", "cmdDiskSwapperAdd0", "Custom controls", "Miscellaneous"},
	{"Miscellaneous", "chkMouseUntrap", "chkMouseUntrap", "Disk swapper", "Priority"},
#ifdef ANDROID
	{ "Savestates",       "State0",         "State0",         "Miscellaneous",  "OnScreen" },
	{ "OnScreen",         "OnScrButton3",   "OnScrCtrl",      "Savestates",     "Shutdown" },
	{ "Quit",             "Start",          "Help",           "OnScreen",       "Paths" },
	{ "Help",             "Quit",           "Start",          "OnScreen",       "Paths" },
	{ "Start",            "Help",           "Quit",           "OnScreen",       "Paths" },
#else
	{"Priority", "cboInactiveRunAtPrio", "cboActiveRunAtPrio", "Miscellaneous", "Savestates" },
	{"Savestates", "State0", "State0", "Priority", "Virtual Keyboard"},
	{"Virtual Keyboard", "chkVkbdHires", "chkVkbdHires", "Savestates", "Quit"},
	{"Shutdown", "Start", "Quit", "Virtual Keyboard", "Paths"},
	{"Quit", "Shutdown", "Restart", "Virtual Keyboard", "Paths"},
	{"Restart", "Quit", "Help", "Virtual Keyboard", "Paths"},
	{"Help", "Restart", "Reset", "Virtual Keyboard", "Paths"},
	{"Reset", "Help", "Start", "Virtual Keyboard", "Paths"},
	{"Start", "Reset", "Shutdown", "Virtual Keyboard", "Paths"},
#endif

	// PanelPaths
	{"cmdSystemROMs", "Paths", "Paths", "cmdRescanROMs", "cmdConfigPath"},
	{"cmdConfigPath", "Paths", "Paths", "cmdSystemROMs", "cmdNvramFiles"},
	{"cmdNvramFiles", "Paths", "Paths", "cmdConfigPath", "cmdScreenshotFiles"},
	{"cmdScreenshotFiles", "Paths", "Paths", "cmdNvramFiles", "cmdStateFiles"},
	{"cmdStateFiles", "Paths", "Paths", "cmdScreenshotFiles", "cmdControllersPath"},
	{"cmdControllersPath", "Paths", "Paths", "cmdStateFiles", "cmdRetroArchFile"},
	{"cmdRetroArchFile", "Paths", "Paths", "cmdControllersPath", "chkEnableLogging"},
	{"chkEnableLogging", "Paths", "Paths", "cmdRetroArchFile", "cmdLogfilePath"},
	{"cmdLogfilePath", "Paths", "Paths", "chkEnableLogging", "cmdRescanROMs"},
	{"cmdRescanROMs", "Paths", "cmdDownloadXML", "cmdLogfilePath", "cmdSystemROMs"},
	{"cmdDownloadXML", "cmdRescanROMs", "cmdDownloadCtrlDb", "cmdLogfilePath", "cmdSystemROMs"},
	{"cmdDownloadCtrlDb", "cmdDownloadXML", "Paths", "cmdLogfilePath", "cmdSystemROMs" },

	//  active            move left         move right        move up           move down
	// PanelQuickstart
	{"cboAModel", "Quickstart", "qsNTSC", "cboWhdload", "cboAConfig"},
	{"qsNTSC", "cboAModel", "Quickstart", "qsMode", "cboAConfig"},
	{"cboAConfig", "Quickstart", "Quickstart", "cboAModel", "qscmdSel0"},
	{"qsDF0", "Quickstart", "cboqsType0", "cboAConfig", "cboqsDisk0"},
	{"cboqsType0", "qsDF0", "qsWP0", "cboAConfig", "cboqsDisk0"},
	{"qsWP0", "cboqsType0", "qsInfo0", "cboAConfig", "cboqsDisk0"},
	{"qsInfo0", "qsWP0", "qscmdEject0", "cboAConfig", "cboqsDisk0" },
	{"qscmdEject0", "qsInfo0", "qscmdSel0", "cboAConfig", "cboqsDisk0"},
	{"qscmdSel0", "qscmdEject0", "Quickstart", "cboAConfig", "cboqsDisk0"},
	{"cboqsDisk0", "Quickstart", "Quickstart", "qscmdSel0", "qscmdSel1"},
	{"qsDF1", "Quickstart", "cboqsType1", "cboqsDisk0", "cboqsDisk1"},
	{"cboqsType1", "qsDF1", "qsWP1", "cboqsDisk0", "cboqsDisk1"},
	{"qsWP1", "cboqsType1", "qsInfo1", "cboqsDisk0", "cboqsDisk1"},
	{"qsInfo1", "qsWP1", "qscmdEject1", "cboqsDisk0", "cboqsDisk1" },
	{"qscmdEject1", "qsInfo1", "qscmdSel1", "cboqsDisk0", "cboqsDisk1"},
	{"qscmdSel1", "qscmdEject1", "Quickstart", "cboqsDisk0", "cboqsDisk1"},
	{"cboqsDisk1", "Quickstart", "Quickstart", "qsDF1", "qsCDSelect"},
	{"qsCD drive", "Quickstart", "qscdEject", "cboqsDisk1", "cboCD"},
	{"qscdEject", "qsCD drive", "qsCDSelect", "cboqsDisk1", "cboCD"},
	{"qsCDSelect", "qscdEject", "Quickstart", "cboqsDisk1", "cboCD"},
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
	{"Fastest", "FPUnone", "CPU and FPU", "25 Mhz", "chkCPUCycleExact"},
	{"chkCPUCycleExact", "FPUnone", "CPU and FPU", "Fastest", "sldCpuIdle"},
	{"sldCpuIdle", "", "", "chkCPUCycleExact", "7 Mhz"},

	// PanelChipset
	{ "optOCS", "Chipset", "optAGA", "optCollFull", "optECSAgnus" },
	{ "optECSAgnus", "Chipset", "optECSDenise", "optOCS", "optFullECS" },
	{ "optFullECS", "Chipset", "chkNTSC", "optECSAgnus", "chkCycleExact" },
	{ "chkCycleExact", "Chipset", "optBlitNormal", "optFullECS", "chkMemoryCycleExact" },
	{ "chkMemoryCycleExact", "Chipset", "chkFastCopper", "chkCycleExact", "chkMultithreadedDrawing" },
	{ "optAGA", "optOCS", "optBlitNormal", "optCollFull", "optECSDenise" },
	{ "optECSDenise", "optECSAgnus", "optBlitImmed", "optAGA", "chkNTSC" },
	{ "chkNTSC", "Chipset", "optBlitWait", "optECSDenise", "chkCycleExact" },
	{ "cboChipset", "Chipset", "chkFastCopper", "", "" },
	{ "chkMultithreadedDrawing", "Chipset", "cboChipset", "chkMemoryCycleExact", "optCollNone" },
	{ "optBlitNormal", "optAGA", "Chipset", "chkFastCopper", "optBlitImmed" },
	{ "optBlitImmed", "optECSDenise", "Chipset", "optBlitNormal", "optBlitWait" },
	{ "optBlitWait", "chkNTSC", "Chipset", "optBlitImmed", "chkFastCopper" },
	{ "chkFastCopper", "chkMemoryCycleExact", "Chipset", "optBlitWait", "optCollNone" },
	{ "optCollNone", "Chipset", "Chipset", "chkMultithreadedDrawing", "optCollSprites" },
	{ "optCollSprites", "Chipset", "Chipset", "optCollNone", "optCollPlayfield" },
	{ "optCollPlayfield", "Chipset", "Chipset", "optCollSprites", "optCollFull" },
	{ "optCollFull", "Chipset", "Chipset", "optCollPlayfield", "optOCS" },

	//  active            move left         move right        move up           move down
	// PanelROM
	{ "cboMainROM", "ROM", "MainROM", "cboCartROM", "cboExtROM" },
	{ "MainROM", "cboMainROM", "ROM", "CartROM", "ExtROM" },
	{ "cboExtROM", "ROM", "ExtROM", "cboMainROM", "cboCartROM" },
	{ "ExtROM", "cboExtROM", "ROM", "MainROM", "CartROM" },
	{ "cboCartROM", "ROM", "CartROM", "cboExtROM", "cboUAEROM" },
	{ "CartROM", "cboCartROM", "ROM", "ExtROM", "cboUAEROM" },
	{ "cboUAEROM", "ROM", "ROM", "cboCartROM", "chkShapeShifter" },
	{ "chkShapeShifter", "ROM", "ROM", "cboUAEROM", "cboMainROM" },

	// PanelRAM
	{ "sldChipmem", "", "", "RAM", "sldSlowmem" },
	{ "sldSlowmem", "", "", "sldChipmem", "sldFastmem" },
	{ "sldFastmem", "", "", "sldSlowmem", "sldZ3mem" },
	{ "sldZ3mem", "", "", "sldFastmem", "sldMbResLowmem" },
	{ "sldMbResLowmem", "", "", "sldZ3mem", "sldMbResHighmem" },
	{ "sldMbResHighmem", "", "", "sldMbResLowmem", "sldChipmem" },

	// PanelFloppy
	{ "DF0:", "Floppy drives", "cboType0", "cmdSaveForDisk", "cboDisk0" },
	{ "cboType0", "DF0:", "chkWP0", "cmdSaveForDisk", "cboDisk0" },
	{ "chkWP0", "cboType0", "cmdInfo0", "cmdSaveForDisk", "cboDisk0" },
	{ "cmdInfo0", "chkWP0", "cmdEject0", "cmdSaveForDisk", "cboDisk0" },
	{ "cmdEject0", "chkWP0", "cmdSel0", "cmdCreateHDDisk", "cboDisk0" },
	{ "cmdSel0", "cmdEject0", "Floppy drives", "cmdCreateHDDisk", "cboDisk0" },
	{ "cboDisk0", "Floppy drives", "Floppy drives", "DF0:", "chkLoadDiskCfg" },
	{ "chkLoadDiskCfg", "Floppy drives", "Floppy drives", "cboDisk0", "DF1:" },
	{ "DF1:", "Floppy drives", "cboType1", "chkLoadDiskCfg", "cboDisk1" },
	{ "cboType1", "DF1:", "chkWP1", "chkLoadDiskCfg", "cboDisk1" },
	{ "chkWP1", "cboType1", "cmdInfo1", "chkLoadDiskCfg", "cboDisk1" },
	{ "cmdInfo1", "chkWP1", "cmdEject1", "chkLoadDiskCfg", "cboDisk1" },
	{ "cmdEject1", "cmdInfo1", "cmdSel1", "chkLoadDiskCfg", "cboDisk1" },
	{ "cmdSel1", "cmdEject1", "Floppy drives", "chkLoadDiskCfg", "cboDisk1" },
	{ "cboDisk1", "Floppy drives", "Floppy drives", "DF1:", "DF2:" },
	{ "DF2:", "Floppy drives", "cboType2", "cboDisk1", "cboDisk2" },
	{ "cboType2", "DF2:", "chkWP2", "cboDisk1", "cboDisk2" },
	{ "chkWP2", "cboType2", "cmdInfo2", "cboDisk1", "cboDisk2" },
	{ "cmdInfo2", "chkWP2", "cmdEject2", "cboDisk1", "cboDisk2" },
	{ "cmdEject2", "cmdInfo2", "cmdSel2", "cboDisk1", "cboDisk2" },
	{ "cmdSel2", "cmdEject2", "Floppy drives", "cboDisk1", "cboDisk2" },
	{ "cboDisk2", "Floppy drives", "Floppy drives", "DF2:", "DF3:" },
	{ "DF3:", "Floppy drives", "cboType3", "cboDisk2", "cboDisk3" },
	{ "cboType3", "DF3:", "chkWP3", "cboDisk2", "cboDisk3" },
	{ "chkWP3", "cboType3", "cmdInfo3", "cboDisk2", "cboDisk3" },
	{ "cmdInfo3", "chkWP3", "cmdEject3", "cboDisk2", "cboDisk3" },
	{ "cmdEject3", "cmdInfo3", "cmdSel3", "cboDisk2", "cboDisk3" },
	{ "cmdSel3", "cmdEject3", "Floppy drives", "cboDisk2", "cboDisk3" },
	{ "cboDisk3", "Floppy drives", "Floppy drives", "DF3:", "sldDriveSpeed" },
	{ "sldDriveSpeed", "", "", "cboDisk3", "cboDBDriver" },
	
	{ "cboDBDriver", "Floppy drives", "", "sldDriveSpeed", "chkDBSmartSpeed" },
	{ "chkDBSmartSpeed", "Floppy drives", "", "cboDBDriver", "chkDBAutoCache" },
	{ "chkDBAutoCache", "Floppy drives", "", "chkDBSmartSpeed", "chkDBCableDriveB" },
	{ "chkDBCableDriveB", "Floppy drives", "", "chkDBAutoCache", "cmdSaveForDisk"},

	{ "cmdSaveForDisk", "Floppy drives", "cmdCreateDDDisk", "chkDBCableDriveB", "DF0:" },
	{ "cmdCreateDDDisk", "cmdSaveForDisk", "cmdCreateHDDisk", "chkDBCableDriveB", "cboType0" },
	{ "cmdCreateHDDisk", "cmdCreateDDDisk", "Floppy drives", "chkDBCableDriveB", "cmdEject0" },

	//  active            move left           move right          move up           move down
	// PanelHD
	{ "cmdProp0", "Hard drives/CD", "cmdDel0", "chkCDTurbo", "cmdProp1" },
	{ "cmdDel0", "cmdProp0", "Hard drives/CD", "chkCDTurbo", "cmdDel1" },
	{ "cmdProp1", "Hard drives/CD", "cmdDel1", "cmdProp0", "cmdProp2" },
	{ "cmdDel1", "cmdProp1", "Hard drives/CD", "cmdDel0", "cmdDel2" },
	{ "cmdProp2", "Hard drives/CD", "cmdDel2", "cmdProp1", "cmdProp3" },
	{ "cmdDel2", "cmdProp2", "Hard drives/CD", "cmdDel1", "cmdDel3" },
	{ "cmdProp3", "Hard drives/CD", "cmdDel3", "cmdProp2", "cmdProp4" },
	{ "cmdDel3", "cmdProp3", "Hard drives/CD", "cmdDel2", "cmdDel4" },
	{ "cmdProp4", "Hard drives/CD", "cmdDel4", "cmdProp3", "cmdProp5" },
	{ "cmdDel4", "cmdProp4", "Hard drives/CD", "cmdDel3", "cmdDel5" },
	{ "cmdProp5", "Hard drives/CD", "cmdDel5", "cmdProp4", "cmdProp6" },
	{ "cmdDel5", "cmdProp5", "Hard drives/CD", "cmdDel4", "cmdDel6" },
	{ "cmdProp6", "Hard drives/CD", "cmdDel6", "cmdProp5", "cmdProp7" },
	{ "cmdDel6", "cmdProp6", "Hard drives/CD", "cmdDel5", "cmdDel7" },
	{ "cmdProp7", "Hard drives/CD", "cmdDel7", "cmdProp6", "cmdAddDir" },
	{ "cmdDel7", "cmdProp7", "Hard drives/CD", "cmdDel6", "cmdAddDir" },
	{ "cmdAddDir", "Hard drives/CD", "cmdAddHDF", "cmdProp7", "cmdCreateHDF" },
	{ "cmdAddHDF", "cmdAddDir", "cmdAddHardDrive", "cmdProp7", "cmdCreateHDF" },
	{ "cmdAddHardDrive", "cmdAddHDF", "cmdAddDir", "cmdProp7", "cmdCreateHDF" },
	{ "cmdCreateHDF", "Hard drives/CD", "Hard drives/CD", "cmdAddDir", "chkCD" },
	{ "chkCD", "Hard drives/CD", "cmdCDSelectFile", "cmdCreateHDF", "cboCD" },
	{ "cmdCDSelectFile", "chkCD", "cmdCDEject", "cmdCreateHDF", "cboCD" },
	{ "cmdCDEject", "cmdCDSelectFile", "chkCD", "cmdCreateHDF", "cboCD" },
	{ "cboCD", "Hard drives/CD", "Hard drives/CD", "chkCD", "chkCDTurbo" },
	{ "chkCDTurbo", "Hard drives/CD", "Hard drives/CD", "cboCD", "cmdProp0" },

	// PanelExpansions
	{ "chkBSDSocket", "Expansions", "chkSana2", "", "chkSCSI" },
	{ "chkSCSI", "Expansions", "chkCD32Fmv", "chkBSDSocket", "" },
	{ "chkSana2", "chkBSDSocket", "Expansions", "", "chkCD32Fmv" },
	{ "chkCD32Fmv", "chkSCSI", "Expansions", "chkSana2", "" },

	// PanelRTG
	{ "cboBoard", "RTG board", "cboRtg16bitModes", "cboRtgRefreshRate", "sldGfxmem" },
	{ "cboRtg16bitModes", "cboBoard", "RTG board", "cboRtgAspectRatio", "cboRtg32bitModes" },
	{ "cboRtg32bitModes", "sldGfxmem", "RTG board", "cboRtg16bitModes", "chkRtgMatchDepth" },
	{ "sldGfxmem", "", "", "cboBoard", "chkRtgMatchDepth" },
	{ "chkRtgMatchDepth", "RTG board", "", "sldGfxmem", "chkRtgAutoscale" },
	{ "chkRtgAutoscale", "RTG board", "", "chkRtgMatchDepth", "chkRtgAllowScaling" },
	{ "chkRtgAllowScaling", "RTG board", "", "chkRtgAutoscale", "chkRtgAlwaysCenter" },
	{ "chkRtgAlwaysCenter", "RTG board", "", "chkRtgAllowScaling", "chkRtgHardwareInterrupt" },
	{ "chkRtgHardwareInterrupt", "RTG board", "", "chkRtgAlwaysCenter", "chkRtgHardwareSprite" },
	{ "chkRtgHardwareSprite", "RTG board", "", "chkRtgHardwareInterrupt", "cboRtgRefreshRate" },
	{ "cboRtgRefreshRate", "RTG board", "cboRtgBufferMode", "chkRtgHardwareSprite", "cboBoard" },
	{ "cboRtgBufferMode", "cboRtgRefreshRate", "cboRtgAspectRatio", "chkRtgHardwareSprite", "cboBoard" },
	{ "cboRtgAspectRatio", "cboRtgBufferMode", "RTG board", "chkRtgHardwareSprite", "cboBoard" },

	//  active            move left           move right          move up           move down
	// PanelDisplay
	{ "cboFullscreen", "Display", "chkHorizontal", "sldBrightness", "cboScreenmode" },
	{ "cboScreenmode", "Display", "chkVertical", "cboFullscreen", "sldWidth" },
	{ "sldWidth", "", "", "cboScreenmode", "sldHeight" },
	{ "sldHeight", "", "", "sldWidth", "chkAutoCrop" },
	{ "chkAutoCrop", "Display", "chkBorderless", "sldHeight", "sldHOffset" },
	{ "chkBorderless", "chkAutoCrop", "optSingle", "sldHeight", "sldHOffset" },
	{ "sldHOffset", "", "", "chkAutoCrop", "sldVOffset" },
	{ "sldVOffset", "", "", "sldHOffset", "cboScalingMethod" },
	{ "cboScalingMethod", "Display", "optScanlines", "sldVOffset", "cboResolution" },
	{ "cboResolution", "Display", "optDouble2", "cboScalingMethod", "chkBlackerThanBlack" },
	{ "chkBlackerThanBlack", "Display", "chkFilterLowRes", "cboResolution", "chkAspect" },
	{ "chkFilterLowRes", "chkBlackerThanBlack", "optISingle", "cboResolution", "chkAspect" },
	{ "chkAspect", "Display", "optISingle", "chkBlackerThanBlack", "chkFlickerFixer" },
	{ "chkFlickerFixer", "Display", "optIDouble", "chkAspect", "chkFrameskip" },
	{ "chkFrameskip", "Display", "optIDouble2", "chkFlickerFixer", "sldBrightness" },
	{ "sldRefresh", "", "", "chkFlickerFixer", "sldBrightness" },
	{ "sldBrightness", "", "", "chkFrameskip", "cboFullscreen" },

	{ "chkHorizontal", "cboScreenmode", "", "optIDouble3", "chkVertical" },
	{ "chkVertical", "cboScreenmode", "", "chkHorizontal", "optSingle" },
	{ "optSingle", "chkBorderless", "", "chkVertical", "optDouble" },
	{ "optDouble", "chkBorderless", "", "optSingle", "optScanlines" },
	{ "optScanlines", "cboScalingMethod", "", "optDouble", "optDouble2" },
	{ "optDouble2", "cboResolution", "", "optScanlines", "optDouble3" },
	{ "optDouble3", "chkFilterLowRes", "", "optDouble2", "optISingle" },
	{ "optISingle", "chkBlackerThanBlack", "", "optDouble3", "optIDouble" },
	{ "optIDouble", "chkFlickerFixer", "", "optISingle", "optIDouble2" },
	{ "optIDouble2", "chkFrameskip", "", "optIDouble", "optIDouble3" },
	{ "optIDouble3", "chkFrameskip", "", "optIDouble2", "chkHorizontal" },

	//  active            move left           move right          move up           move down
	// PanelSound
	{ "cboSoundcard", "Sound", "Sound", "sldFloppySoundDisk", "sndDisable" },
	{ "sndDisable", "Sound", "sldPaulaVol", "cboSoundcard", "sndDisEmu" },
	{ "sndDisEmu", "Sound", "sldPaulaVol", "sndDisable", "sndEmulate" },
	{ "sndEmulate", "Sound", "sldPaulaVol", "sndDisEmu", "sndEmuBest" },
	{ "sndEmuBest", "Sound", "sldPaulaVol", "sndEmulate", "cboChannelMode" },
	{ "cboChannelMode", "Sound", "cboSeparation", "sndEmuBest", "cboFrequency" },
	{ "cboFrequency", "Sound", "cboStereoDelay", "cboChannelMode", "chkFloppySound" },
	{ "cboInterpol", "cboSeparation", "Sound", "sldAHIVol", "cboFilter" },
	{ "cboFilter", "cboStereoDelay", "Sound", "cboInterpol", "sldSoundBufferSize" },
	{ "cboSeparation", "cboChannelMode", "cboInterpol", "sndEmuBest", "cboStereoDelay" },
	{ "cboStereoDelay", "cboFrequency", "cboFilter", "cboSeparation", "chkFloppySound" },
	{ "sldMasterVol", "", "", "optSoundPush", "sldPaulaVol" },
	{ "sldPaulaVol", "", "", "sldMasterVol", "sldCDVol" },
	{ "sldCDVol", "", "", "sldPaulaVol", "sldAHIVol" },
	{ "sldAHIVol", "", "", "sldCDVol", "cboInterpol" },
	{ "chkFloppySound", "Sound", "sldSoundBufferSize", "cboFrequency", "sldFloppySoundEmpty" },
	{ "sldFloppySoundEmpty", "", "", "chkFloppySound", "sldFloppySoundDisk" },
	{ "sldFloppySoundDisk", "", "", "sldFloppySoundEmpty", "cboSoundcard" },
	{ "sldSoundBufferSize", "", "", "cboFilter", "optSoundPull" },
	{ "optSoundPull", "sldFloppySoundEmpty", "Sound", "sldSoundBufferSize", "optSoundPush" },
	{ "optSoundPush", "sldFloppySoundDisk", "Sound", "optSoundPull", "sldMasterVol" },

	//  active            move left           move right          move up           move down
	// PanelInput
	{ "cboPort0", "Input", "Input", "optBoth", "cboPort0Autofire" },
	{ "cboPort0Autofire", "Input", "cboPort0mode", "cboPort0", "cboPort1" },
	{ "cboPort0mode", "cboPort0Autofire", "cmdRemap0", "cboPort0", "cboPort1" },
	{ "cmdRemap0", "cboPort0mode", "Input", "cboPort0", "cboPort1" },
	{ "cboPort1", "Input", "Input", "cboPort0Autofire", "cboPort1Autofire" },
	{ "cboPort1Autofire", "Input", "cboPort1mode", "cboPort1", "cmdSwapPorts" },
	{ "cboPort1mode", "cboPort1Autofire", "cmdRemap1", "cboPort1", "chkInputAutoswitch" },
	{ "cmdRemap1", "cboPort1mode", "Input", "cboPort1", "chkInputAutoswitch" },
	{ "cmdSwapPorts", "Input", "chkInputAutoswitch", "cboPort1Autofire", "cboPort2" },
	{ "chkInputAutoswitch", "Input", "", "cboPort1mode", "cboPort2" },
	{ "cboPort2", "Input", "Input", "cmdSwapPorts", "cboPort2Autofire" },
	{ "cboPort2Autofire", "Input", "Input", "cboPort2", "cboPort3" },
	{ "cboPort3", "Input", "Input", "cboPort2Autofire", "cboPort3Autofire" },
	{ "cboPort3Autofire", "Input", "Input", "cboPort3", "cboPort0mousemode" },
	{ "cboPort0mousemode", "Input", "sldDigitalJoyMouseSpeed", "cboPort3Autofire", "cboPort1mousemode" },
	{ "cboPort1mousemode", "Input", "sldAnalogJoyMouseSpeed", "cboPort0mousemode", "cboAutofireRate" },
	{ "cboAutofireRate", "Input", "sldMouseSpeed", "cboPort1mousemode", "chkMouseHack" },
	{ "sldDigitalJoyMouseSpeed", "", "", "cboPort3Autofire", "sldAnalogJoyMouseSpeed" },
	{ "sldAnalogJoyMouseSpeed", "", "", "sldDigitalJoyMouseSpeed", "sldMouseSpeed" },
	{ "sldMouseSpeed", "", "", "sldAnalogJoyMouseSpeed", "chkMagicMouseUntrap" },
	{ "chkMagicMouseUntrap", "chkMouseHack", "", "sldMouseSpeed", "optHost" },
	{ "chkMouseHack", "Input", "chkMagicMouseUntrap", "cboAutofireRate", "optBoth" },
	{ "optBoth", "Input", "optNative", "chkMouseHack", "cboPort0" },
	{ "optNative", "optBoth", "optHost", "chkMouseHack", "cboPort0" },
	{ "optHost", "optNative", "", "chkMouseHack", "cboPort0" },

	// active		move left				move right			move up			move down
	// PanelIO
	{ "txtSerialDevice", "", "", "cboProtectionDongle", "chkRTSCTS" },
	{ "chkRTSCTS", "IO Ports", "chkSerialDirect", "txtSerialDevice", "cboProtectionDongle" },
	{ "chkSerialDirect", "chkRTSCTS", "chkUaeSerial", "txtSerialDevice", "cboProtectionDongle" },
	{ "chkUaeSerial", "chkSerialDirect", "IO Ports", "txtSerialDevice", "cboProtectionDongle" },
	{ "cboProtectionDongle", "IO Ports", "IO Ports", "chkRTSCTS", "txtSerialDevice" },

	// PanelCustom
	{ "0: Mouse", "Custom controls", "1: Joystick", "", "None" },
	{ "1: Joystick", "0: Mouse", "2: Parallel 1", "", "HotKey" },
	{ "2: Parallel 1", "1: Joystick", "3: Parallel 2", "", "HotKey" },
	{ "3: Parallel 2", "2: Parallel 1", "Custom controls", "", "cmdSetHotkey" },

	{ "None", "Custom controls", "HotKey", "0: Mouse", "cboCustomButtonAction0" },
	{ "HotKey", "None", "cmdSetHotkey", "1: Joystick", "cboCustomButtonAction0" },

#if SDL_VERSION_ATLEAST( 2, 0, 14 )
	{ "cmdSetHotkey", "HotKey", "cmdSetHotkeyClear", "3: Parallel 2", "cboCustomButtonAction10" },
	{ "cmdSetHotkeyClear", "cmdSetHotkey", "Custom controls", "3: Parallel 2", "cboCustomButtonAction10" },

	// Left column
	{ "cboCustomButtonAction0", "Custom controls", "cboCustomButtonAction10", "None", "cboCustomButtonAction1" },
	{ "cboCustomButtonAction1", "Custom controls", "cboCustomButtonAction11", "cboCustomButtonAction0", "cboCustomButtonAction2" },
	{ "cboCustomButtonAction2", "Custom controls", "cboCustomButtonAction12", "cboCustomButtonAction1", "cboCustomButtonAction3" },
	{ "cboCustomButtonAction3", "Custom controls", "cboCustomButtonAction13", "cboCustomButtonAction2", "cboCustomButtonAction4" },
	{ "cboCustomButtonAction4", "Custom controls", "cboCustomButtonAction14", "cboCustomButtonAction3", "cboCustomButtonAction5" },
	{ "cboCustomButtonAction5", "Custom controls", "cboCustomButtonAction15", "cboCustomButtonAction4", "cboCustomButtonAction6" },
	{ "cboCustomButtonAction6", "Custom controls", "cboCustomButtonAction16", "cboCustomButtonAction5", "cboCustomButtonAction7" },
	{ "cboCustomButtonAction7", "Custom controls", "cboCustomButtonAction17", "cboCustomButtonAction6", "cboCustomButtonAction8" },
	{ "cboCustomButtonAction8", "Custom controls", "cboCustomButtonAction18", "cboCustomButtonAction7", "cboCustomButtonAction9" },
	{ "cboCustomButtonAction9", "Custom controls", "cboCustomButtonAction19", "cboCustomButtonAction8", "cboCustomAxisAction0" },
		// Right column
	{ "cboCustomButtonAction10", "cboCustomButtonAction0", "Custom controls", "None", "cboCustomButtonAction11" },
	{ "cboCustomButtonAction11", "cboCustomButtonAction1", "Custom controls", "cboCustomButtonAction10", "cboCustomButtonAction12" },
	{ "cboCustomButtonAction12", "cboCustomButtonAction2", "Custom controls", "cboCustomButtonAction11", "cboCustomButtonAction13" },
	{ "cboCustomButtonAction13", "cboCustomButtonAction3", "Custom controls", "cboCustomButtonAction12", "cboCustomButtonAction14" },
	{ "cboCustomButtonAction14", "cboCustomButtonAction4", "Custom controls", "cboCustomButtonAction13", "cboCustomButtonAction15" },
	{ "cboCustomButtonAction15", "cboCustomButtonAction5", "Custom controls", "cboCustomButtonAction14", "cboCustomButtonAction16" },
	{ "cboCustomButtonAction16", "cboCustomButtonAction6", "Custom controls", "cboCustomButtonAction15", "cboCustomButtonAction17" },
	{ "cboCustomButtonAction17", "cboCustomButtonAction7", "Custom controls", "cboCustomButtonAction16", "cboCustomButtonAction18" },
	{ "cboCustomButtonAction18", "cboCustomButtonAction8", "Custom controls", "cboCustomButtonAction17", "cboCustomButtonAction19" },
	{ "cboCustomButtonAction19", "cboCustomButtonAction9", "Custom controls", "cboCustomButtonAction18", "cboCustomButtonAction20" },
	{ "cboCustomButtonAction20", "cboCustomAxisAction0", "Custom controls", "cboCustomButtonAction19", "cboCustomAxisAction4" },
		// Left column bottom
	{ "cboCustomAxisAction0", "Custom controls", "cboCustomButtonAction20", "cboCustomButtonAction9", "cboCustomAxisAction1" },
	{ "cboCustomAxisAction1", "Custom controls", "cboCustomAxisAction4", "cboCustomAxisAction0", "cboCustomAxisAction2" },
	{ "cboCustomAxisAction2", "Custom controls", "cboCustomAxisAction5", "cboCustomAxisAction1", "cboCustomAxisAction3" },
	{ "cboCustomAxisAction3", "Custom controls", "", "cboCustomAxisAction2", "0: Mouse" },
		// Right column bottom
	{ "cboCustomAxisAction4", "cboCustomAxisAction1", "Custom controls", "cboCustomButtonAction20", "cboCustomAxisAction5" },
	{ "cboCustomAxisAction5", "cboCustomAxisAction2", "Custom controls", "cboCustomAxisAction4", "cmdSetHotkey" },
#else
	// Previous SDL2 versions only had a maximum of 15 buttons (0 - 14)
	{ "cmdSetHotkey", "HotKey", "cmdSetHotkeyClear", "3: Parallel 2", "cboCustomButtonAction7" },
	{ "cmdSetHotkeyClear", "cmdSetHotkey", "Custom controls", "3: Parallel 2", "cboCustomButtonAction7" },
		// Left column
	{ "cboCustomButtonAction0", "Custom controls", "cboCustomButtonAction7", "None", "cboCustomButtonAction1" },
	{ "cboCustomButtonAction1", "Custom controls", "cboCustomButtonAction8", "cboCustomButtonAction0", "cboCustomButtonAction2" },
	{ "cboCustomButtonAction2", "Custom controls", "cboCustomButtonAction9", "cboCustomButtonAction1", "cboCustomButtonAction3" },
	{ "cboCustomButtonAction3", "Custom controls", "cboCustomButtonAction10", "cboCustomButtonAction2", "cboCustomButtonAction4" },
	{ "cboCustomButtonAction4", "Custom controls", "cboCustomButtonAction11", "cboCustomButtonAction3", "cboCustomButtonAction5" },
	{ "cboCustomButtonAction5", "Custom controls", "cboCustomButtonAction12", "cboCustomButtonAction4", "cboCustomButtonAction6" },
	{ "cboCustomButtonAction6", "Custom controls", "cboCustomButtonAction13", "cboCustomButtonAction5", "cboCustomAxisAction0" },

		// Right column
	{ "cboCustomButtonAction7", "cboCustomButtonAction0", "Custom controls", "None", "cboCustomButtonAction8" },
	{ "cboCustomButtonAction8", "cboCustomButtonAction1", "Custom controls", "cboCustomButtonAction7", "cboCustomButtonAction9" },
	{ "cboCustomButtonAction9", "cboCustomButtonAction2", "Custom controls", "cboCustomButtonAction8", "cboCustomButtonAction10" },
	{ "cboCustomButtonAction10", "cboCustomButtonAction3", "Custom controls", "cboCustomButtonAction9", "cboCustomButtonAction11" },
	{ "cboCustomButtonAction11", "cboCustomButtonAction4", "Custom controls", "cboCustomButtonAction10", "cboCustomButtonAction12" },
	{ "cboCustomButtonAction12", "cboCustomButtonAction5", "Custom controls", "cboCustomButtonAction11", "cboCustomButtonAction13" },
	{ "cboCustomButtonAction13", "cboCustomButtonAction6", "Custom controls", "cboCustomButtonAction12", "cboCustomButtonAction14" },
	{ "cboCustomButtonAction14", "cboCustomAxisAction0", "Custom controls", "cboCustomButtonAction13", "cboCustomAxisAction1" },

	{ "cboCustomAxisAction0", "Custom controls", "cboCustomButtonAction14", "cboCustomButtonAction6", "cboCustomAxisAction1" },
	{ "cboCustomAxisAction1", "Custom controls", "cboCustomAxisAction4", "cboCustomAxisAction0", "cboCustomAxisAction2" },
	{ "cboCustomAxisAction2", "Custom controls", "cboCustomAxisAction5", "cboCustomAxisAction1", "cboCustomAxisAction3" },
	{ "cboCustomAxisAction3", "Custom controls", "", "cboCustomAxisAction2", "0: Mouse" },
		// Right column bottom
	{ "cboCustomAxisAction4", "cboCustomAxisAction1", "Custom controls", "cboCustomButtonAction14", "cboCustomAxisAction5" },
	{ "cboCustomAxisAction5", "cboCustomAxisAction2", "Custom controls", "cboCustomAxisAction4", "cmdSetHotkey" },

#endif

	// PanelDiskSwapper
	// active				move left			move right			move up			move down
	// row 0
	{ "cmdDiskSwapperAdd0", "Disk swapper", "cmdDiskSwapperDel0", "cmdDiskSwapperRemoveAll", "cmdDiskSwapperAdd1"},
	{ "cmdDiskSwapperDel0", "cmdDiskSwapperAdd0", "cmdDiskSwapperDrv0", "cmdDiskSwapperRemoveAll", "cmdDiskSwapperDel1" },
	{ "cmdDiskSwapperDrv0", "cmdDiskSwapperDel0", "Disk swapper", "cmdDiskSwapperRemoveAll", "cmdDiskSwapperDrv1" },
	// row 1
	{ "cmdDiskSwapperAdd1", "Disk swapper", "cmdDiskSwapperDel1", "cmdDiskSwapperAdd0", "cmdDiskSwapperAdd2" },
	{ "cmdDiskSwapperDel1", "cmdDiskSwapperAdd1", "cmdDiskSwapperDrv1", "cmdDiskSwapperDel0", "cmdDiskSwapperDel2" },
	{ "cmdDiskSwapperDrv1", "cmdDiskSwapperDel1", "Disk swapper", "cmdDiskSwapperDrv0", "cmdDiskSwapperDrv2" },
	// row 2
	{ "cmdDiskSwapperAdd2", "Disk swapper", "cmdDiskSwapperDel2", "cmdDiskSwapperAdd1", "cmdDiskSwapperAdd3" },
	{ "cmdDiskSwapperDel2", "cmdDiskSwapperAdd2", "cmdDiskSwapperDrv2", "cmdDiskSwapperDel1", "cmdDiskSwapperDel3" },
	{ "cmdDiskSwapperDrv2", "cmdDiskSwapperDel2", "Disk swapper", "cmdDiskSwapperDrv1", "cmdDiskSwapperDrv3" },
	// row 3
	{ "cmdDiskSwapperAdd3", "Disk swapper", "cmdDiskSwapperDel3", "cmdDiskSwapperAdd2", "cmdDiskSwapperAdd4" },
	{ "cmdDiskSwapperDel3", "cmdDiskSwapperAdd3", "cmdDiskSwapperDrv3", "cmdDiskSwapperDel2", "cmdDiskSwapperDel4" },
	{ "cmdDiskSwapperDrv3", "cmdDiskSwapperDel3", "Disk swapper", "cmdDiskSwapperDrv2", "cmdDiskSwapperDrv4" },
	// row 4
	{ "cmdDiskSwapperAdd4", "Disk swapper", "cmdDiskSwapperDel4", "cmdDiskSwapperAdd3", "cmdDiskSwapperAdd5" },
	{ "cmdDiskSwapperDel4", "cmdDiskSwapperAdd4", "cmdDiskSwapperDrv4", "cmdDiskSwapperDel3", "cmdDiskSwapperDel5" },
	{ "cmdDiskSwapperDrv4", "cmdDiskSwapperDel4", "Disk swapper", "cmdDiskSwapperDrv3", "cmdDiskSwapperDrv5" },
	// row 5
	{ "cmdDiskSwapperAdd5", "Disk swapper", "cmdDiskSwapperDel5", "cmdDiskSwapperAdd4", "cmdDiskSwapperAdd6" },
	{ "cmdDiskSwapperDel5", "cmdDiskSwapperAdd5", "cmdDiskSwapperDrv5", "cmdDiskSwapperDel4", "cmdDiskSwapperDel6" },
	{ "cmdDiskSwapperDrv5", "cmdDiskSwapperDel5", "Disk swapper", "cmdDiskSwapperDrv4", "cmdDiskSwapperDrv6" },
	// row 6
	{ "cmdDiskSwapperAdd6", "Disk swapper", "cmdDiskSwapperDel6", "cmdDiskSwapperAdd5", "cmdDiskSwapperAdd7" },
	{ "cmdDiskSwapperDel6", "cmdDiskSwapperAdd6", "cmdDiskSwapperDrv6", "cmdDiskSwapperDel5", "cmdDiskSwapperDel7" },
	{ "cmdDiskSwapperDrv6", "cmdDiskSwapperDel6", "Disk swapper", "cmdDiskSwapperDrv5", "cmdDiskSwapperDrv7" },
	// row 7
	{ "cmdDiskSwapperAdd7", "Disk swapper", "cmdDiskSwapperDel7", "cmdDiskSwapperAdd6", "cmdDiskSwapperAdd8" },
	{ "cmdDiskSwapperDel7", "cmdDiskSwapperAdd7", "cmdDiskSwapperDrv7", "cmdDiskSwapperDel6", "cmdDiskSwapperDel8" },
	{ "cmdDiskSwapperDrv7", "cmdDiskSwapperDel7", "Disk swapper", "cmdDiskSwapperDrv6", "cmdDiskSwapperDrv8" },
	// row 8
	{ "cmdDiskSwapperAdd8", "Disk swapper", "cmdDiskSwapperDel8", "cmdDiskSwapperAdd7", "cmdDiskSwapperAdd9" },
	{ "cmdDiskSwapperDel8", "cmdDiskSwapperAdd8", "cmdDiskSwapperDrv8", "cmdDiskSwapperDel7", "cmdDiskSwapperDel9" },
	{ "cmdDiskSwapperDrv8", "cmdDiskSwapperDel8", "Disk swapper", "cmdDiskSwapperDrv7", "cmdDiskSwapperDrv9" },
	// row 9
	{ "cmdDiskSwapperAdd9", "Disk swapper", "cmdDiskSwapperDel9", "cmdDiskSwapperAdd8", "cmdDiskSwapperRemoveAll" },
	{ "cmdDiskSwapperDel9", "cmdDiskSwapperAdd9", "cmdDiskSwapperDrv9", "cmdDiskSwapperDel8", "cmdDiskSwapperRemoveAll" },
	{ "cmdDiskSwapperDrv9", "cmdDiskSwapperDel9", "Disk swapper", "cmdDiskSwapperDrv8", "cmdDiskSwapperRemoveAll" },

	{ "cmdDiskSwapperRemoveAll", "Disk swapper", "Disk swapper", "cmdDiskSwapperAdd9", "cmdDiskSwapperAdd0" },

	// PanelMisc
	//  active            move left           move right          move up           move down
	{ "scrlMisc", "chkMouseUntrap", "cmdKeyOpenGUI", "", "" },
	{ "chkMouseUntrap", "Miscellaneous", "scrlMisc", "chkRTSCTS", "chkShowGUI" },
	{ "chkShowGUI", "Miscellaneous", "scrlMisc", "chkMouseUntrap", "chkSyncClock" },
	{ "chkSyncClock", "Miscellaneous", "scrlMisc", "chkBSDSocket", "chkResetDelay" },
	{ "chkResetDelay", "Miscellaneous", "scrlMisc", "chkSyncClock", "chkFasterRTG" },
	{ "chkFasterRTG", "Miscellaneous", "scrlMisc", "chkResetDelay", "chkClipboardSharing" },
	{ "chkClipboardSharing", "Miscellaneous", "scrlMisc", "chkFasterRTG", "chkAllowNativeCode" },
	{ "chkAllowNativeCode", "Miscellaneous", "scrlMisc", "chkClipboardSharing", "chkStatusLineNative" },
	{ "chkStatusLineNative", "Miscellaneous", "scrlMisc", "chkAllowNativeCode", "chkStatusLineRtg" },
	{ "chkStatusLineRtg", "Miscellaneous", "scrlMisc", "chkStatusLineNative", "chkIllegalMem" },
	{ "chkIllegalMem", "Miscellaneous", "scrlMisc", "chkStatusLineRtg", "chkMinimizeInactive" },
	{ "chkMinimizeInactive", "Miscellaneous", "scrlMisc", "chkIllegalMem", "chkMasterWP" },
	{ "chkMasterWP", "Miscellaneous", "scrlMisc", "chkMinimizeInactive", "chkHDRO" },
	{ "chkHDRO", "Miscellaneous", "scrlMisc", "chkMasterWP", "chkHideAutoconfig" },
	{ "chkHideAutoconfig", "Miscellaneous", "scrlMisc", "chkHDRO", "chkRCtrlIsRAmiga" },
	{ "chkRCtrlIsRAmiga", "Miscellaneous", "scrlMisc", "chkHideAutoconfig", "chkCaptureAlways" },
	{ "chkCaptureAlways", "Miscellaneous", "scrlMisc", "chkRCtrlIsRAmiga", "chkScsiDisable" },
	{ "chkScsiDisable", "Miscellaneous", "scrlMisc", "chkCaptureAlways", "chkAltTabRelease" },
	{ "chkAltTabRelease", "Miscellaneous", "scrlMisc", "chkCaptureAlways", "chkRetroArchQuit" },
	{ "chkRetroArchQuit", "Miscellaneous", "scrlMisc", "chkAltTabRelease", "chkRetroArchMenu" },
	{ "chkRetroArchMenu", "Miscellaneous", "scrlMisc", "chkRetroArchQuit", "chkRetroArchReset" },
	{ "chkRetroArchReset", "Miscellaneous", "scrlMisc", "chkRetroArchMenu", "cmdKeyOpenGUI" },

	{ "cmdKeyOpenGUI", "scrlMisc", "cmdKeyOpenGUIClear", "cboCapsLock", "cmdKeyForQuit" },
	{ "cmdKeyOpenGUIClear", "cmdKeyOpenGUI", "Miscellaneous", "cboCapsLock", "cmdKeyForQuitClear" },
	{ "cmdKeyForQuit", "scrlMisc", "cmdKeyForQuitClear", "cmdKeyOpenGUI", "cmdKeyActionReplay" },
	{ "cmdKeyForQuitClear", "cmdKeyForQuit", "Miscellaneous", "cmdKeyOpenGUIClear", "cmdKeyActionReplayClear" },
	{ "cmdKeyActionReplay", "scrlMisc", "cmdKeyActionReplayClear", "cmdKeyForQuit", "cmdKeyFullScreen" },
	{ "cmdKeyActionReplayClear", "cmdKeyActionReplay", "Miscellaneous", "cmdKeyForQuitClear", "cmdKeyFullScreenClear" },
	{ "cmdKeyFullScreen", "scrlMisc", "cmdKeyFullScreenClear", "cmdKeyActionReplay", "cmdKeyMinimize" },
	{ "cmdKeyFullScreenClear", "cmdKeyFullScreen", "Miscellaneous", "cmdKeyActionReplayClear", "cmdKeyMinimizeClear" },
	{ "cmdKeyMinimize", "scrlMisc", "cmdKeyMinimizeClear", "cmdKeyFullScreen", "cboNumlock" },
	{ "cmdKeyMinimizeClear", "cmdKeyMinimize", "Miscellaneous", "cmdKeyFullScreenClear", "cboNumlock" },
	{ "cboNumlock", "chkMouseUntrap", "Miscellaneous", "cmdKeyMinimize", "cboScrolllock" },
	{ "cboScrolllock", "chkMouseUntrap", "Miscellaneous", "cboNumlock", "cboCapsLock" },
	{ "cboCapsLock", "chkMouseUntrap", "Miscellaneous", "cboScrolllock", "cmdKeyOpenGUI"},

	// PanelPrio
	{ "cboActiveRunAtPrio", "Priority", "cboInactiveRunAtPrio", "chkActiveDisableSound", "chkActivePauseEmulation" },
	{ "chkActivePauseEmulation", "Priority", "chkInactivePauseEmulation", "cboActiveRunAtPrio", "chkActiveDisableSound" },
	{ "chkActiveDisableSound", "Priority", "chkInactiveDisableSound", "chkActivePauseEmulation", "cboActiveRunAtPrio" },
	{ "cboInactiveRunAtPrio", "cboActiveRunAtPrio", "cboMinimizedRunAtPrio", "chkInactiveDisableControllers", "chkInactivePauseEmulation" },
	{ "chkInactivePauseEmulation", "chkActivePauseEmulation", "chkMinimizedPauseEmulation", "cboInactiveRunAtPrio", "chkInactiveDisableSound" },
	{ "chkInactiveDisableSound", "chkActiveDisableSound", "chkMinimizedDisableSound", "chkInactivePauseEmulation", "chkInactiveDisableControllers" },
	{ "chkInactiveDisableControllers", "chkActiveDisableSound", "chkMinimizedDisableControllers", "chkInactiveDisableSound", "cboInactiveRunAtPrio" },
	{ "cboMinimizedRunAtPrio", "cboInactiveRunAtPrio", "Priority", "chkMinimizedDisableControllers", "chkMinimizedPauseEmulation" },
	{ "chkMinimizedPauseEmulation", "chkInactivePauseEmulation", "Priority", "cboMinimizedRunAtPrio", "chkMinimizedDisableSound" },
	{ "chkMinimizedDisableSound", "chkInactiveDisableSound", "Priority", "chkMinimizedPauseEmulation", "chkMinimizedDisableControllers" },
	{ "chkMinimizedDisableControllers", "chkInactiveDisableControllers", "Priority", "chkMinimizedDisableSound", "cboMinimizedRunAtPrio" },

	// PanelSavestate
	{ "State0", "Savestates", "Savestates", "LoadState", "State1" },
	{ "State1", "Savestates", "Savestates", "State0", "State2" },
	{ "State2", "Savestates", "Savestates", "State1", "State3" },
	{ "State3", "Savestates", "Savestates", "State2", "State4" },
	{ "State4", "Savestates", "Savestates", "State3", "State5" },
	{ "State5", "Savestates", "Savestates", "State4", "State6" },
	{ "State6", "Savestates", "Savestates", "State5", "State7" },
	{ "State7", "Savestates", "Savestates", "State6", "State8" },
	{ "State8", "Savestates", "Savestates", "State7", "State9" },
	{ "State9", "Savestates", "Savestates", "State8", "State10" },
	{ "State10", "Savestates", "Savestates", "State9", "State11" },
	{ "State11", "Savestates", "Savestates", "State10", "State12" },
	{ "State12", "Savestates", "Savestates", "State11", "State13" },
	{ "State13", "Savestates", "Savestates", "State12", "State14" },
	{ "State14", "Savestates", "Savestates", "State13", "LoadState" },
	{ "LoadState", "Savestates", "SaveState", "State14", "State0" },
	{ "SaveState", "LoadState", "Savestates", "State14", "State0" },

	// Virtual Keyboard
	{ "chkVkbdHires", "Virtual Keyboard", "Virtual Keyboard", "cboVkbdStyle", "chkVkbdExit"},
	{ "chkVkbdExit", "Virtual Keyboard", "Virtual Keyboard", "chkVkbdHires", "sldVkbdTransparency"},
	{ "sldVkbdTransparency", "", "", "chkVkbdExit", "cboVkbdLanguage"},
	{ "cboVkbdLanguage", "Virtual Keyboard", "Virtual Keyboard", "sldVkbdTransparency", "cboVkbdStyle"},
	{ "cboVkbdStyle", "Virtual Keyboard", "Virtual Keyboard", "cboVkbdLanguage", "chkVkbdHires"},

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
	{ "txtVirtDevice", "", "", "txtVirtPath", "txtVirtVolume" },
	{ "txtVirtVolume", "", "", "txtVirtDevice", "txtVirtPath" },
	{ "txtVirtPath", "", "", "txtVirtVolume", "cmdVirtPath" },
	{ "cmdVirtPath", "", "", "txtVirtBootPri", "cmdVirtCancel" },
	{ "chkVirtRW", "txtVirtDev", "txtVirtDev", "cmdVirtOK", "virtAutoboot" },
	{ "virtAutoboot", "txtVirtVol", "txtVirtBootPri", "chkVirtRW", "cmdVirtPath" },
	{ "cmdVirtOK", "cmdVirtCancel", "cmdVirtCancel", "cmdVirtPath", "chkVirtRW" },
	{ "cmdVirtCancel", "cmdVirtOK", "cmdVirtOK", "cmdVirtPath", "chkVirtRW" },

	// EditFilesysHardfile
	{ "txtHdfDev", "", "", "cmdHdfOK", "chkHdfRW" },
	{ "chkHdfRW", "txtHdfDev", "", "cmdHdfOK", "hdfAutoboot" },
	{ "hdfAutoboot", "txtHdfDev", "txtHdfBootPri", "chkHdfRW", "cmdHdfPath" },
	{ "txtHdfBootPri", "", "", "hdfAutoboot", "txtHdfPath" },
	{ "txtHdfPath", "", "", "hdfAutoboot", "cmdHdfPath" },
	{ "cmdHdfPath", "txtHdfPath", "txtHdfPath", "txtHdfBootPri", "hdfController" },
	{ "hdfController", "cboHdfUnit", "cboHdfUnit", "cmdHdfPath", "cmdHdfOK" },
	{ "cboHdfUnit", "hdfController", "hdfController", "cmdHdfPath", "cmdHdfOK" },
	{ "cmdHdfOK", "cmdHdfCancel", "cmdHdfCancel", "cboHdfUnit", "chkHdfRW" },
	{ "cmdHdfCancel", "cmdHdfOK", "cmdHdfOK", "cboHdfUnit", "chkHdfRW" },

	// EditFilesysHardDrive
	{ "txtHDDPath", "", "", "cmdHDDOk", "cmdHDDOk" },
	{ "cmdHDDOk", "cmdHDDCancel", "cmdHDDCancel", "txtHDDPath", "txtHDDPath" },
	{ "cmdHDDCancel", "cmdHDDOk", "cmdHDDOk", "txtHDDPath", "txtHDDPath" },

	// CreateFilesysHardfile
	{ "txtCreateDevice", "", "", "cmdCreateHdfOK", "chkCreateHdfAutoboot" },
	{ "chkCreateHdfAutoboot", "txtCreateDevice", "txtCreateBootPri", "cmdCreateHdfOK", "cmdCreateHdfPath" },
	{ "txtCreateBootPri", "", "", "chkCreateHdfAutoboot", "cmdCreateHdfPath" },
	{ "cmdCreateHdfPath", "txtCreatePath", "txtCreatePath", "txtCreateBootPri", "chkDynamic" },
	{ "chkDynamic", "", "", "cmdCreateHdfPath", "cmdCreateHdfOK" },
	{ "cmdCreateHdfOK", "cmdCreateHdfCancel", "cmdCreateHdfCancel", "chkDynamic", "txtCreateDevice" },
	{ "cmdCreateHdfCancel", "cmdCreateHdfOK", "cmdCreateHdfOK", "chkDynamic", "txtCreateDevice" },

	{ "END", "", "", "", "" }
};


bool HandleNavigation(int direction)
{
	gcn::FocusHandler* focusHdl = gui_top->_getFocusHandler();
	gcn::Widget* focusTarget = nullptr;

	if (focusHdl != nullptr)
	{
		gcn::Widget* activeWidget = focusHdl->getFocused();

		if (activeWidget != nullptr && activeWidget->getId().length() > 0)
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
							if (activeWidget->getId().substr(0, 3) != "txt")
								searchFor = navMap[i].leftWidget;
							break;
						case DIRECTION_RIGHT:
							if (activeWidget->getId().substr(0, 3) != "txt")
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

			if (activeWidget->getId().substr(0, 4) == "scrl")
			{
				// Scroll area detected
				auto* scrollarea = dynamic_cast<gcn::ScrollArea*>(activeWidget);
				if (direction == DIRECTION_UP)
				{
					auto scroll = scrollarea->getVerticalScrollAmount();
					scrollarea->setVerticalScrollAmount(scroll - 30);
				}
				else if (direction == DIRECTION_DOWN)
				{
					auto scroll = scrollarea->getVerticalScrollAmount();
					scrollarea->setVerticalScrollAmount(scroll + 30);
				}
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
	SDL_Event event{};

	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = inKey;
	gui_input->pushInput(event); // Fire key down
	event.type = SDL_KEYUP; 
	gui_input->pushInput(event); // and the key up
}
