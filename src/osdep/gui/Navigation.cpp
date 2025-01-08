#include <guisan.hpp>
#include <guisan/sdl.hpp>

#include "config.h"
#include "SelectorEntry.hpp"
#include "sysdeps.h"
#include "gui_handling.h"
#include "amiberry_input.h"

typedef struct
{
	std::string active_widget;
	std::string left_widget;
	std::string right_widget;
	std::string up_widget;
	std::string down_widget;
} NavigationMap;


static NavigationMap nav_map[] =
{
	//  active              move left         move right        move up             move down
	// main_window
	{"About", "", "", "Quit", "Paths"},
	{"Paths", "cmdSystemROMs", "cmdSystemROMs", "About", "Quickstart"},
	{"Quickstart", "qsNTSC", "cboAModel", "Paths", "Configurations"},
	{"Configurations", "ConfigList", "ConfigList", "Quickstart", "CPU and FPU"},
	{"CPU and FPU", "optCPUSpeedFastest", "optCPU68000", "Configurations", "Chipset"},
	{"Chipset", "chkFastCopper", "optOCS", "CPU and FPU", "ROM"},
	{"ROM", "MainROM", "cboMainROM", "Chipset", "RAM"},
	{"RAM", "sldChipmem", "sldChipmem", "ROM", "Floppy drives"},
	{"Floppy drives", "cmdSel0", "DF0:", "RAM", "Hard drives/CD"},
	{"Hard drives/CD", "cmdCreateHDF", "cmdAddDir", "Floppy drives", "Expansions"},
	{"Expansions", "chkBSDSocket", "chkBSDSocket", "Hard drives/CD", "RTG board"},
	{"RTG board", "cboBoard", "cboBoard", "Expansions", "Hardware info"},
	{"Hardware info", "", "", "RTG board", "Display"},
	{"Display", "cboScreenmode", "cboScreenmode", "Hardware info", "Sound"},
	{"Sound", "chkSystemDefault", "chkSystemDefault", "Display", "Input"},
	{"Input", "cboPort0mode", "cboPort0", "Sound", "IO Ports"},
	{"IO Ports", "cboSampler", "cboSampler", "Input", "Custom controls"},
	{"Custom controls", "Right Trigger", "0: Mouse", "IO Ports", "Disk swapper"},
	{"Disk swapper", "cmdDiskSwapperDrv0", "cmdDiskSwapperAdd0", "Custom controls", "Miscellaneous"},
	{"Miscellaneous", "chkMouseUntrap", "chkMouseUntrap", "Disk swapper", "Priority"},
	{"Priority", "cboInactiveRunAtPrio", "cboActiveRunAtPrio", "Miscellaneous", "Savestates" },
	{"Savestates", "State0", "State0", "Priority", "Virtual Keyboard"},
	{"Virtual Keyboard", "chkVkEnabled", "chkVkEnabled", "Savestates", "WHDLoad"},
	{"WHDLoad", "cmdWhdloadEject", "cmdWhdloadEject", "Virtual Keyboard", "Themes"},
	{"Themes", "cmdThemeSaveAs", "cboThemePreset", "WHDLoad", "Quit"},
	{"Shutdown", "Start", "Quit", "Themes", "About"},
	{"Quit", "Shutdown", "Restart", "Themes", "About"},
	{"Restart", "Quit", "Help", "Themes", "About"},
	{"Help", "Restart", "Reset", "Themes", "About"},
	{"Reset", "Help", "Start", "Themes", "About"},
	{"Start", "Reset", "Shutdown", "Themes", "About"},

	// PanelPaths
	{"scrlPaths", "cmdSystemROMs", "Paths", "", "" },
	{"cmdSystemROMs", "Paths", "scrlPaths", "cmdRescanROMs", "cmdConfigPath"},
	{"cmdConfigPath", "Paths", "scrlPaths", "cmdSystemROMs", "cmdNvramFiles"},
	{"cmdNvramFiles", "Paths", "scrlPaths", "cmdConfigPath", "cmdNvramFiles"},
	{"cmdPluginsFiles", "Paths", "scrlPaths", "cmdNvramFiles", "cmdScreenshotFiles"},
	{"cmdScreenshotFiles", "Paths", "scrlPaths", "cmdPluginsFiles", "cmdStateFiles"},
	{"cmdStateFiles", "Paths", "scrlPaths", "cmdScreenshotFiles", "cmdControllersPath"},
	{"cmdControllersPath", "Paths", "scrlPaths", "cmdStateFiles", "cmdRetroArchFile"},
	{"cmdRetroArchFile", "Paths", "scrlPaths", "cmdControllersPath", "cmdWHDBootPath"},
	{"cmdWHDBootPath", "Paths", "scrlPaths", "cmdRetroArchFile", "cmdWHDLoadArchPath"},
	{"cmdWHDLoadArchPath", "Paths", "scrlPaths", "cmdWHDBootPath", "cmdFloppyPath"},
	{"cmdFloppyPath", "Paths", "scrlPaths", "cmdWHDLoadArchPath", "cmdCDPath"},
	{"cmdCDPath", "Paths", "scrlPaths", "cmdFloppyPath", "cmdHardDrivesPath"},
	{"cmdHardDrivesPath", "Paths", "scrlPaths", "cmdCDPath", "chkEnableLogging"},
	{"chkEnableLogging", "Paths", "Paths", "cmdHardDrivesPath", "cmdLogfilePath"},
	{"cmdLogfilePath", "Paths", "Paths", "chkEnableLogging", "cmdRescanROMs"},
	{"cmdRescanROMs", "Paths", "cmdDownloadXML", "cmdLogfilePath", "cmdSystemROMs"},
	{"cmdDownloadXML", "cmdRescanROMs", "cmdDownloadCtrlDb", "cmdLogfilePath", "cmdSystemROMs"},
	{"cmdDownloadCtrlDb", "cmdDownloadXML", "Paths", "cmdLogfilePath", "cmdSystemROMs" },

	//  active            move left         move right        move up           move down
	// PanelQuickstart
	{"cboAModel", "Quickstart", "qsNTSC", "cboQsWhdload", "cboAConfig"},
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
	{"cmdSetConfig", "Quickstart", "Quickstart", "qsMode", "cmdQsWhdloadEject"},
	{"cmdQsWhdloadEject", "Quickstart", "cmdQsWhdloadSelect", "cmdSetConfig", "cboQsWhdload"},
	{"cmdQsWhdloadSelect", "cmdQsWhdloadEject", "Quickstart", "cmdSetConfig", "cboQsWhdload"},
	{"cboQsWhdload", "Quickstart", "Quickstart", "cmdQsWhdloadEject", "cboAModel"},

	// PanelConfig
	{"ConfigList", "Configurations", "ConfigLoad", "", ""},
	{"ConfigLoad", "Configurations", "ConfigSave", "ConfigList", "ConfigList"},
	{"ConfigSave", "ConfigLoad", "CfgDelete", "ConfigList", "ConfigList"},
	{"CfgDelete", "ConfigSave", "Configurations", "ConfigList", "ConfigList"},

	//  active            move left         move right        move up           move down
	// PanelCPU
	{"optCPU68000", "CPU and FPU", "optCPUSpeedFastest", "chkFPUStrict", "optCPU68010"},
	{"optCPU68010", "CPU and FPU", "optCPUSpeedFastest", "optCPU68000", "optCPU68020"},
	{"optCPU68020", "CPU and FPU", "optCPUSpeedFastest", "optCPU68010", "optCPU68030"},
	{"optCPU68030", "CPU and FPU", "optCPUSpeedFastest", "optCPU68020", "optCPU68040"},
	{"optCPU68040", "CPU and FPU", "optCPUSpeedFastest", "optCPU68030", "optCPU68060"},
	{"optCPU68060", "CPU and FPU", "optCPUSpeedFastest", "optCPU68040", "chk24Bit"},
	{"chk24Bit", "CPU and FPU", "cboCPUFrequency", "optCPU68040", "chkCPUCompatible"},
	{"chkCPUCompatible", "CPU and FPU", "cboCPUFrequency", "chk24Bit", "chkCpuDataCache"},
	{"chkCpuDataCache", "CPU and FPU", "cboCPUFrequency", "chkCPUCompatible", "chkJIT"},
	{"chkJIT", "CPU and FPU", "cboCPUFrequency", "chkCpuDataCache", "optMMUNone"},
	{"optMMUNone", "CPU and FPU", "sldJitCacheSize", "chkJIT", "optMMUEnabled"},
	{"optMMUEnabled", "CPU and FPU", "optMMUEC", "optMMUNone", "optFPUnone"},
	{"optMMUEC", "optMMUEnabled", "sldJitCacheSize", "optMMUNone", "optFPUnone"},
	{"optFPUnone", "CPU and FPU", "sldJitCacheSize", "optMMUEnabled", "optFPU68881"},
	{"optFPU68881", "CPU and FPU", "sldJitCacheSize", "optFPUnone", "optFPU68882"},
	{"optFPU68882", "CPU and FPU", "sldJitCacheSize", "optFPU68881", "optFPUinternal"},
	{"optFPUinternal", "CPU and FPU", "sldJitCacheSize", "optFPU68882", "chkFPUStrict"},
	{"chkFPUStrict", "CPU and FPU", "sldJitCacheSize", "optFPUinternal", "optCPU68000"},

	{"optCPUSpeedFastest", "optCPU68000", "CPU and FPU", "chkCatchExceptions", "optCPUSpeedReal"},
	{"optCPUSpeedReal", "optCPU68000", "CPU and FPU", "optCPUSpeedFastest", "sldCpuSpeed"},
	{"sldCpuSpeed", "", "", "optCPUSpeedReal", "sldCpuIdle"},
	{"sldCpuIdle", "", "", "sldCpuSpeed", "cboCPUFrequency"},
	{"cboCPUFrequency", "chk24Bit", "CPU and FPU", "sldCpuIdle", "chkPPCEnabled"},
	{"chkPPCEnabled", "optMMUNone", "CPU and FPU", "cboCPUFrequency", "sldPPCIdle" },
	{"sldPPCIdle", "", "", "chkPPCEnabled", "sldJitCacheSize" },
	{"sldJitCacheSize", "", "", "sldPPCIdle", "chkFPUJIT"},
	{"chkFPUJIT", "optFPUnone", "chkConstantJump", "sldJitCacheSize", "optDirect"},
	{"chkConstantJump", "chkFPUJIT", "chkHardFlush", "sldJitCacheSize", "optIndirect"},
	{"chkHardFlush", "chkConstantJump", "CPU and FPU", "sldJitCacheSize", "chkNoFlags"},
	{"optDirect", "optFPUnone", "optIndirect", "chkFPUJIT", "chkCatchExceptions"},
	{"optIndirect", "optDirect", "chkNoFlags", "chkConstantJump", "chkCatchExceptions"},
	{"chkNoFlags", "optIndirect", "CPU and FPU", "chkHardFlush", "chkCatchExceptions"},
	{"chkCatchExceptions", "optFPUnone", "CPU and FPU", "optDirect", "optCPUSpeedFastest"},

	// PanelChipset
	{ "optOCS", "Chipset", "optAGA", "optCollFull", "optECSAgnus" },
	{ "optECSAgnus", "Chipset", "optECSDenise", "optOCS", "optFullECS" },
	{ "optFullECS", "Chipset", "chkNTSC", "optECSAgnus", "chkCycleExact" },
	{ "chkCycleExact", "Chipset", "chkMultithreadedDrawing", "optFullECS", "chkMemoryCycleExact" },
	{ "chkMemoryCycleExact", "Chipset", "chkMultithreadedDrawing", "chkCycleExact", "cboChipset" },
	{ "optAGA", "optOCS", "chkSubpixelEmu", "optCollFull", "optECSDenise" },
	{ "optECSDenise", "optECSAgnus", "optBlitImmed", "optAGA", "chkNTSC" },
	{ "chkNTSC", "Chipset", "chkBlitWait", "optECSDenise", "chkCycleExact" },
	{ "cboChipset", "chkMemoryCycleExact", "cboSpecialMonitors", "", "" },
	{ "chkSubpixelEmu", "optAGA", "Chipset", "optCollFull", "chkBlitImmed" },
	{ "chkBlitImmed", "optAGA", "Chipset", "chkSubpixelEmu", "chkBlitWait" },
	{ "chkBlitWait", "chkNTSC", "Chipset", "chkBlitImmed", "chkMultithreadedDrawing" },
	{ "chkMultithreadedDrawing", "chkCycleExact", "Chipset", "chkBlitWait", "cboSpecialMonitors" },
	{ "cboSpecialMonitors", "cboChipset", "cboKeyboardOptions", "", "" },

	{ "cboKeyboardOptions", "cboChipset", "optCollNone", "", "" },
	{ "chkKeyboardNKRO", "Chipset", "Chipset", "cboKeyboardOptions", "optCollNone" },

	{ "optCollNone", "Chipset", "optCollPlayfield", "chkKeyboardNKRO", "optCollSprites" },
	{ "optCollSprites", "Chipset", "optCollFull", "optCollNone", "optOCS" },
	{ "optCollPlayfield", "optCollNone", "Chipset", "optCollSprites", "optCollFull" },
	{ "optCollFull", "optCollSprites", "Chipset", "optCollPlayfield", "chkSubpixelEmu" },

	//  active            move left         move right        move up           move down
	// PanelROM
	{ "cboMainROM", "ROM", "MainROM", "cboCartROM", "cboExtROM" },
	{ "MainROM", "cboMainROM", "ROM", "CartROM", "ExtROM" },
	{ "cboExtROM", "ROM", "ExtROM", "cboMainROM", "chkMapRom" },
	{ "ExtROM", "cboExtROM", "ROM", "MainROM", "chkShapeShifter" },
	{ "chkMapRom", "ROM", "chkShapeShifter", "cboExtROM", "cboCartROM" },
	{ "chkShapeShifter", "chkMapRom", "ROM", "cboExtROM", "cboCartROM" },
	{ "cboCartROM", "ROM", "CartROM", "chkMapRom", "cboUAEROM" },
	{ "CartROM", "cboCartROM", "ROM", "chkShapeShifter", "cboUAEROM" },
	{ "cboUAEROM", "ROM", "ROM", "cboCartROM", "chkShapeShifter" },

	// PanelRAM
	{ "sldChipmem", "", "", "RAM", "sldSlowmem" },
	{ "sldSlowmem", "", "", "sldChipmem", "sldFastmem" },
	{ "sldFastmem", "", "", "sldSlowmem", "sldZ3mem" },
	{ "sldZ3mem", "", "", "sldFastmem", "sldMbResLowmem" },
	{ "sldMbResLowmem", "", "", "sldZ3mem", "sldMbResHighmem" },
	{ "sldMbResHighmem", "", "", "sldMbResLowmem", "sldChipmem" },

	// PanelFloppy
	{ "DF0:", "Floppy drives", "cboType0", "cmdCreateDDDisk", "cboDisk0" },
	{ "cboType0", "DF0:", "chkWP0", "cmdCreateDDDisk", "cboDisk0" },
	{ "chkWP0", "cboType0", "cmdInfo0", "cmdCreateDDDisk", "cboDisk0" },
	{ "cmdInfo0", "chkWP0", "cmdEject0", "cmdCreateDDDisk", "cboDisk0" },
	{ "cmdEject0", "cmdInfo0", "cmdSel0", "cmdCreateHDDisk", "cboDisk0" },
	{ "cmdSel0", "cmdEject0", "Floppy drives", "cmdCreateHDDisk", "cboDisk0" },
	{ "cboDisk0", "Floppy drives", "Floppy drives", "DF0:", "DF1:" },
	{ "DF1:", "Floppy drives", "cboType1", "cboDisk0", "cboDisk1" },
	{ "cboType1", "DF1:", "chkWP1", "cboDisk0", "cboDisk1" },
	{ "chkWP1", "cboType1", "cmdInfo1", "cboDisk0", "cboDisk1" },
	{ "cmdInfo1", "chkWP1", "cmdEject1", "cboDisk0", "cboDisk1" },
	{ "cmdEject1", "cmdInfo1", "cmdSel1", "cboDisk0", "cboDisk1" },
	{ "cmdSel1", "cmdEject1", "Floppy drives", "cboDisk0", "cboDisk1" },
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
	
	{ "cboDBDriver", "Floppy drives", "Floppy drives", "sldDriveSpeed", "chkDBSmartSpeed" },
	{ "chkDBSmartSpeed", "Floppy drives", "Floppy drives", "cboDBDriver", "chkDBAutoCache" },
	{ "chkDBAutoCache", "Floppy drives", "Floppy drives", "chkDBSmartSpeed", "chkDBCableDriveB" },
	{ "chkDBCableDriveB", "Floppy drives", "Floppy drives", "chkDBAutoCache", "cmdCreateDDDisk"},

	{ "cmdCreateDDDisk", "Floppy drives", "cmdCreateHDDisk", "chkDBCableDriveB", "DF0:" },
	{ "cmdCreateHDDisk", "cmdCreateDDDisk", "Floppy drives", "chkDBCableDriveB", "DF0:" },

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
	{ "cmdAddDir", "Hard drives/CD", "cmdAddHDF", "cmdProp7", "cmdAddCDDrive" },
	{ "cmdAddHDF", "cmdAddDir", "cmdAddHardDrive", "cmdProp7", "cmdAddTapeDrive" },
	{ "cmdAddHardDrive", "cmdAddHDF", "cmdAddDir", "cmdProp7", "cmdCreateHDF" },
	{ "cmdAddCDDrive", "Hard drives/CD", "cmdAddTapeDrive", "cmdAddDir", "chkCD" },
	{ "cmdAddTapeDrive", "cmdAddCDDrive", "cmdCreateHDF", "cmdAddHDF", "chkCD" },
	{ "cmdCreateHDF", "cmdAddTapeDrive", "cmdAddCDDrive", "cmdAddHardDrive", "chkCD" },
	{ "chkCD", "Hard drives/CD", "cmdCDSelectFile", "cmdAddCDDrive", "cboCD" },
	{ "cmdCDSelectFile", "chkCD", "cmdCDEject", "cmdAddTapeDrive", "cboCD" },
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
	{ "chkRtgHardwareSprite", "RTG board", "", "chkRtgHardwareInterrupt", "chkRtgMultithreaded" },
	{ "chkRtgMultithreaded", "RTG board", "", "chkRtgHardwareSprite", "cboRtgRefreshRate" },
	{ "cboRtgRefreshRate", "RTG board", "cboRtgBufferMode", "chkRtgMultithreaded", "cboBoard" },
	{ "cboRtgBufferMode", "cboRtgRefreshRate", "cboRtgAspectRatio", "chkRtgHardwareSprite", "cboBoard" },
	{ "cboRtgAspectRatio", "cboRtgBufferMode", "RTG board", "chkRtgHardwareSprite", "cboBoard" },

	//  active            move left           move right          move up           move down
	// PanelDisplay
	{ "cboFullscreen", "Display", "chkHorizontal", "sldBrightness", "cboScreenmode" },
	{ "cboScreenmode", "Display", "chkVertical", "cboFullscreen", "chkManualCrop" },
	{ "chkManualCrop", "Display", "chkHorizontal", "cboScreenmode", "sldWidth" },
	{ "sldWidth", "", "", "chkManualCrop", "sldHeight" },
	{ "sldHeight", "", "", "sldWidth", "chkAutoCrop" },
	{ "chkAutoCrop", "Display", "chkBorderless", "sldHeight", "cboVSyncNative" },
	{ "chkBorderless", "chkAutoCrop", "optSingle", "sldHeight", "cboVSyncNative" },
	{ "cboVSyncNative", "Display", "optSingle", "chkAutoCrop", "cboVSyncRtg" },
	{ "cboVSyncRtg", "Display", "optSingle", "cboVSyncNative", "sldHOffset" },
	{ "sldHOffset", "", "", "cboVSyncRtg", "sldVOffset" },
	{ "sldVOffset", "", "", "sldHOffset", "cboScalingMethod" },
	{ "cboScalingMethod", "Display", "optScanlines", "sldVOffset", "cboResolution" },
	{ "cboResolution", "Display", "optDouble2", "cboScalingMethod", "cboResSwitch" },
	{ "cboResSwitch", "Display", "optDouble2", "cboResolution", "chkBlackerThanBlack" },
	{ "chkBlackerThanBlack", "Display", "chkFilterLowRes", "cboResSwitch", "chkAspect" },
	{ "chkFilterLowRes", "chkBlackerThanBlack", "optISingle", "cboResSwitch", "chkAspect" },
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
	{ "cboSoundcard", "Sound", "Sound", "sldFloppySoundDisk", "chkSystemDefault" },
	{"chkSystemDefault", "Sound", "Sound", "cboSoundcard", "sndDisable"},
	{ "sndDisable", "Sound", "sldPaulaVol", "chkSystemDefault", "sndDisEmu" },
	{ "sndDisEmu", "Sound", "sldPaulaVol", "sndDisable", "sndEmulate" },
	{ "sndEmulate", "Sound", "sldPaulaVol", "sndDisEmu", "chkAutoSwitching" },
	{ "chkAutoSwitching", "Sound", "sldPaulaVol", "sndEmulate", "cboChannelMode" },
	{ "cboChannelMode", "Sound", "cboSeparation", "chkAutoSwitching", "cboFrequency" },
	{ "cboFrequency", "Sound", "cboSwapChannels", "cboChannelMode", "chkFloppySound" },
	{ "cboSwapChannels", "cboFrequency", "cboStereoDelay", "cboChannelMode", "chkFloppySound" },
	{ "cboInterpol", "cboSeparation", "Sound", "sldMIDIVol", "cboFilter" },
	{ "cboFilter", "cboStereoDelay", "Sound", "cboInterpol", "sldSoundBufferSize" },
	{ "cboSeparation", "cboChannelMode", "cboInterpol", "chkAutoSwitching", "cboStereoDelay" },
	{ "cboStereoDelay", "cboSwapChannels", "cboFilter", "cboSeparation", "chkFloppySound" },
	{ "sldPaulaVol", "", "", "optSoundPush", "sldCDVol" },
	{ "sldCDVol", "", "", "sldPaulaVol", "sldAHIVol" },
	{ "sldAHIVol", "", "", "sldCDVol", "sldMIDIVol" },
	{ "sldMIDIVol", "", "", "sldAHIVol", "cboInterpol" },
	{ "chkFloppySound", "Sound", "sldSoundBufferSize", "cboFrequency", "sldFloppySoundEmpty" },
	{ "sldFloppySoundEmpty", "", "", "chkFloppySound", "sldFloppySoundDisk" },
	{ "sldFloppySoundDisk", "", "", "sldFloppySoundEmpty", "cboSoundcard" },
	{ "sldSoundBufferSize", "", "", "cboFilter", "optSoundPull" },
	{ "optSoundPull", "sldFloppySoundEmpty", "Sound", "sldSoundBufferSize", "optSoundPush" },
	{ "optSoundPush", "sldFloppySoundDisk", "Sound", "optSoundPull", "sldPaulaVol" },

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

	// active		move left		move right			move up			move down
	// PanelIO
	{ "cboSampler", "IO Ports", "IO Ports", "cboProtectionDongle", "cboSerialPort" },
	{ "cboSerialPort", "IO Ports", "IO Ports", "cboSampler", "chkSerialShared" },
	{ "chkSerialShared", "IO Ports", "chkRTSCTS", "cboSerialPort", "chkSerialStatus" },
	{ "chkRTSCTS", "chkSerialShared", "chkSerialDirect", "cboSerialPort", "chkSerialStatus" },
	{ "chkSerialDirect", "chkRTSCTS", "chkUaeSerial", "cboSerialPort", "chkSerialStatus" },
	{ "chkUaeSerial", "chkSerialDirect", "IO Ports", "cboSerialPort", "chkSerialStatus" },
	{ "chkSerialStatus", "IO Ports", "chkSerialStatusRi", "chkSerialShared", "cboMidiOut" },
	{ "chkSerialStatusRi", "chkSerialStatus", "IO Ports", "chkUaeSerial", "cboMidiIn" },
	{ "cboMidiOut", "IO Ports", "cboMidiIn", "chkSerialStatus", "chkMidiRoute" },
	{ "cboMidiIn", "cboMidiOut", "chkSerialStatusRi", "chkSerialStatusRi", "chkMidiRoute" },
	{ "chkMidiRoute", "IO Ports", "IO Ports", "cboMidiOut", "cboProtectionDongle" },
	{ "cboProtectionDongle", "IO Ports", "IO Ports", "chkMidiRoute", "cboSampler" },

	// PanelCustom
	{ "0: Mouse", "Custom controls", "1: Joystick", "cmdSaveMapping", "None" },
	{ "1: Joystick", "0: Mouse", "2: Parallel 1", "cmdSaveMapping", "HotKey" },
	{ "2: Parallel 1", "1: Joystick", "3: Parallel 2", "cmdSaveMapping", "HotKey" },
	{ "3: Parallel 2", "2: Parallel 1", "Custom controls", "cmdSaveMapping", "cmdSetHotkey" },

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
	// Left column bottom
	{ "cboCustomAxisAction0", "Custom controls", "cboCustomButtonAction14", "cboCustomButtonAction6", "cboCustomAxisAction1" },
	{ "cboCustomAxisAction1", "Custom controls", "cboCustomAxisAction4", "cboCustomAxisAction0", "cboCustomAxisAction2" },
	{ "cboCustomAxisAction2", "Custom controls", "cboCustomAxisAction5", "cboCustomAxisAction1", "cboCustomAxisAction3" },
	{ "cboCustomAxisAction3", "Custom controls", "", "cboCustomAxisAction2", "0: Mouse" },
		// Right column bottom
	{ "cboCustomAxisAction4", "cboCustomAxisAction1", "Custom controls", "cboCustomButtonAction14", "cboCustomAxisAction5" },
	{ "cboCustomAxisAction5", "cboCustomAxisAction2", "Custom controls", "cboCustomAxisAction4", "cmdSetHotkey" },
#endif
	{ "cmdSaveMapping", "Custom controls", "Custom controls", "cboCustomAxisAction3", "0: Mouse" },

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
	{ "chkMouseUntrap", "Miscellaneous", "scrlMisc", "chkRetroArchReset", "chkShowGUI" },
	{ "chkShowGUI", "Miscellaneous", "scrlMisc", "chkMouseUntrap", "chkMainAlwaysOnTop" },
	{ "chkMainAlwaysOnTop", "Miscellaneous", "scrlMisc", "chkShowGUI", "chkGuiAlwaysOnTop" },
	{ "chkGuiAlwaysOnTop","Miscellaneous", "scrlMisc", "chkMainAlwaysOnTop", "chkSyncClock" },
	{ "chkSyncClock", "Miscellaneous", "scrlMisc", "chkGuiAlwaysOnTop", "chkResetDelay" },
	{ "chkResetDelay", "Miscellaneous", "scrlMisc", "chkSyncClock", "chkFasterRTG" },
	{ "chkFasterRTG", "Miscellaneous", "scrlMisc", "chkResetDelay", "chkClipboardSharing" },
	{ "chkClipboardSharing", "Miscellaneous", "scrlMisc", "chkFasterRTG", "chkAllowNativeCode" },
	{ "chkAllowNativeCode", "Miscellaneous", "scrlMisc", "chkClipboardSharing", "chkStatusLineNative" },
	{ "chkStatusLineNative", "Miscellaneous", "scrlMisc", "chkAllowNativeCode", "chkStatusLineRtg" },
	{ "chkStatusLineRtg", "Miscellaneous", "scrlMisc", "chkStatusLineNative", "chkIllegalMem" },
	{ "chkIllegalMem", "Miscellaneous", "scrlMisc", "chkStatusLineRtg", "chkMinimizeInactive" },
	{ "chkMinimizeInactive", "Miscellaneous", "scrlMisc", "chkIllegalMem", "chkMasterWP" },
	{ "chkMasterWP", "Miscellaneous", "scrlMisc", "chkMinimizeInactive", "chkHDReadOnly" },
	{ "chkHDReadOnly", "Miscellaneous", "scrlMisc", "chkMasterWP", "chkHideAutoconfig" },
	{ "chkHideAutoconfig", "Miscellaneous", "scrlMisc", "chkHDReadOnly", "chkRCtrlIsRAmiga" },
	{ "chkRCtrlIsRAmiga", "Miscellaneous", "scrlMisc", "chkHideAutoconfig", "chkCaptureAlways" },
	{ "chkCaptureAlways", "Miscellaneous", "scrlMisc", "chkRCtrlIsRAmiga", "chkScsiDisable" },
	{ "chkScsiDisable", "Miscellaneous", "scrlMisc", "chkCaptureAlways", "chkAltTabRelease" },
	{ "chkAltTabRelease", "Miscellaneous", "scrlMisc", "chkScsiDisable", "chkWarpModeReset" },
	{ "chkWarpModeReset", "Miscellaneous", "scrlMisc", "chkAltTabRelease", "chkRetroArchQuit" },
	{ "chkRetroArchQuit", "Miscellaneous", "scrlMisc", "chkWarpModeReset", "chkRetroArchMenu" },
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
	{ "cboCapsLock", "chkMouseUntrap", "Miscellaneous", "cboScrolllock", "cmdKeyOpenGUI" },

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
	{ "State0", "Savestates", "Savestates", "cmdLoadState", "State1" },
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
	{ "State14", "Savestates", "Savestates", "State13", "cmdLoadStateSlot" },
	{ "cmdLoadStateSlot", "Savestates", "cmdSaveStateSlot", "State14", "cmdLoadState" },
	{ "cmdSaveStateSlot", "cmdLoadStateSlot", "cmdDeleteStateSlot", "State14", "cmdSaveState" },
	{ "cmdDeleteStateSlot", "cmdSaveStateSlot", "Savestates", "State14", "cmdSaveState" },
	{ "cmdLoadState", "Savestates", "cmdSaveState", "cmdLoadStateSlot", "State0" },
	{ "cmdSaveState", "cmdLoadState", "Savestates", "cmdSaveStateSlot", "State0" },

	// Virtual Keyboard
	{ "chkVkEnabled", "Virtual Keyboard", "Virtual Keyboard", "chkRetroArchVkbd", "chkVkHires"},
	{ "chkVkHires", "Virtual Keyboard", "Virtual Keyboard", "chkVkEnabled", "chkVkExit"},
	{ "chkVkExit", "Virtual Keyboard", "Virtual Keyboard", "chkVkHires", "sldVkTransparency"},
	{ "sldVkTransparency", "", "", "chkVkExit", "cboVkLanguage"},
	{ "cboVkLanguage", "Virtual Keyboard", "Virtual Keyboard", "sldVkTransparency", "cboVkStyle"},
	{ "cboVkStyle", "Virtual Keyboard", "Virtual Keyboard", "cboVkLanguage", "cmdVkSetHotkey"},
	{ "cmdVkSetHotkey",  "Virtual Keyboard", "cmdVkSetHotkeyClear", "cboVkStyle", "chkRetroArchVkbd"},
	{ "cmdVkSetHotkeyClear", "cmdVkSetHotkey", "Virtual Keyboard", "cboVkStyle", "chkRetroArchVkbd"},
	{ "chkRetroArchVkbd", "Virtual Keyboard", "Virtual Keyboard", "cmdVkSetHotkey", "chkVkEnabled"},

	// WHDLoad
	{ "cmdWhdloadEject", "WHDLoad", "cmdWhdloadSelect", "chkQuitOnExit", "cboWhdload" },
	{ "cmdWhdloadSelect", "cmdWhdloadEject", "WHDLoad", "chkQuitOnExit", "cboWhdload" },
	{ "cboWhdload", "WHDLoad", "WHDLoad", "cmdWhdloadSelect", "cboSlaves" },
	{ "cboSlaves", "WHDLoad", "WHDLoad", "cboWhdload", "cmdCustomFields" },
	{ "cmdCustomFields", "WHDLoad", "WHDLoad", "cboSlaves", "chkButtonWait" },
	{ "chkButtonWait", "WHDLoad", "WHDLoad", "cmdCustomFields", "chkShowSplash" },
	{ "chkShowSplash", "WHDLoad", "WHDLoad", "chkButtonWait", "chkWriteCache" },
	{ "chkWriteCache", "WHDLoad", "WHDLoad", "chkShowSplash", "chkQuitOnExit" },
	{ "chkQuitOnExit", "WHDLoad", "WHDLoad", "chkWriteCache", "cmdWhdloadEject" },

	// Themes
	{ "cboThemePreset", "Themes", "cmdThemeSave", "cmdThemeUse", "cmdThemeFont" },
	{ "cmdThemeSave", "cboThemePreset", "cmdThemeSaveAs", "cmdThemeUse", "cmdThemeFont" },
	{ "cmdThemeSaveAs", "cmdThemeSave", "Themes", "cmdThemeUse", "cmdThemeFont" },
	{ "cmdThemeFont", "Themes", "Themes", "cboThemePreset", "cmdThemeUse" },
	{ "cmdThemeUse", "Themes", "cmdThemeReset", "cmdThemeFont", "cboThemePreset" },
	{ "cmdThemeReset", "cmdThemeUse", "Themes", "cmdThemeFont", "cboThemePreset" },

	//  active            move left         move right        move up           move down
	// EditFilesysVirtual
	{ "txtVirtDevice", "", "", "txtVirtPath", "txtVirtVolume" },
	{ "txtVirtVolume", "", "", "txtVirtDevice", "txtVirtPath" },
	{ "txtVirtPath", "", "", "txtVirtVolume", "cmdVirtSelectDir" },
	{ "cmdVirtSelectDir", "cmdVirtSelectFile", "cmdVirtSelectFile", "chkVirtBootable", "cmdVirtCancel" },
	{ "cmdVirtSelectFile", "cmdVirtSelectDir", "cmdVirtSelectDir", "chkVirtBootable", "cmdVirtOK" },
	{ "chkVirtRW", "txtVirtDev", "txtVirtDev", "cmdVirtOK", "chkVirtBootable" },
	{ "chkVirtBootable", "txtVirtVol", "txtVirtBootPri", "chkVirtRW", "cmdVirtSelectDir" },
	{ "cmdVirtOK", "cmdVirtCancel", "cmdVirtCancel", "cmdVirtSelectDir", "chkVirtRW" },
	{ "cmdVirtCancel", "cmdVirtOK", "cmdVirtOK", "cmdVirtSelectFile", "chkVirtRW" },

	// EditFilesysHardfile
	{ "cmdHdfPath", "", "", "cmdHdfCancel", "cmdHdfGeometry" },
	{ "cmdHdfGeometry", "", "", "cmdHdfPath", "cmdHdfFilesys" },
	{ "cmdHdfFilesys", "", "", "cmdHdfGeometry", "chkHdfRW" },
	{ "txtHdfDev", "", "", "cmdHdfFilesys", "chkHdfRW" },
	{ "txtHdfBootPri", "", "", "cmdHdfFilesys", "hdfAutoboot" },
	{ "chkHdfRW", "hdfAutoboot", "hdfAutoboot", "cmdHdfFilesys", "chkHdfDoNotMount" },
	{ "hdfAutoboot", "chkHdfRW", "chkHdfRW", "cmdHdfFilesys", "chkHdfDoNotMount" },
	{ "chkHdfDoNotMount", "", "", "chkHdfRW", "chkHdfRDB" },
	{ "chkHdfRDB", "", "", "chkHdfDoNotMount", "cboHdfController" },
	{ "cboHdfController", "cboHdfUnit", "cboHdfUnit", "chkHdfRDB", "cmdHdfOK" },
	{ "cboHdfUnit", "cboHdfController", "cboHdfController", "cmdHdfPath", "cmdHdfOK" },
	{ "cboHdfControllerTpe", "cboHdfUnit", "cboHdfController", "cboHdfFeatureLevel", "cmdHdfOK" },
	{ "cboHdfFeatureLevel", "", "", "chkHdfRDB", "cboHdfUnit" },
	{ "cmdHdfOK", "cmdHdfCancel", "cmdHdfCancel", "cboHdfController", "cmdHdfPath" },
	{ "cmdHdfCancel", "cmdHdfOK", "cmdHdfOK", "cboHdfController", "cmdHdfPath" },

	// EditTapeDrive
	{ "cmdTapeDriveSelectDir", "cmdTapeDriveSelectFile", "cmdTapeDriveSelectFile", "cmdTapeDriveOK", "cboTapeDriveController" },
	{ "cmdTapeDriveSelectFile", "cmdTapeDriveSelectDir", "cmdTapeDriveSelectDir", "cmdTapeDriveCancel", "cboTapeDriveUnit" },
	{ "cboTapeDriveController", "cboTapeDriveUnit", "cboTapeDriveUnit", "cmdTapeDriveSelectDir", "cmdTapeDriveOK" },
	{ "cboTapeDriveUnit", "cboTapeDriveController", "cboTapeDriveController", "cmdTapeDriveSelectFile", "cmdTapeDriveOK" },
	{ "cmdTapeDriveOK", "cmdTapeDriveCancel", "cmdTapeDriveCancel", "cboTapeDriveController", "cmdTapeDriveSelectDir" },
	{ "cmdTapeDriveCancel", "cmdTapeDriveOK", "cmdTapeDriveOK", "cboTapeDriveUnit", "cmdTapeDriveSelectFile" },

	// EditFilesysHardDrive
	{ "txtHDDPath", "", "", "cmdHDDOk", "cboHDController" },
	{ "cboHDController", "cboHDFeatureLevel", "cboHDControllerUnit", "cmdHDDOk", "cmdHDDOk" },
	{ "cboHDControllerUnit", "cboHDController", "cboHDControllerType", "cmdHDDOk", "cmdHDDOk" },
	{ "cboHDControllerType", "cboHDControllerUnit", "cboHDFeatureLevel", "cmdHDDOk", "cmdHDDOk" },
	{ "cboHDFeatureLevel", "cboHDControllerType", "cboHDController", "cmdHDDOk", "cmdHDDOk" },
	{ "cmdHDDOk", "cmdHDDCancel", "cmdHDDCancel", "cboHDController", "cboHDController" },
	{ "cmdHDDCancel", "cmdHDDOk", "cmdHDDOk", "cboHDController", "cboHDController" },

	// CreateFilesysHardfile
	{ "txtCreateDevice", "", "", "cmdCreateHdfOK", "cmdCreateHdfOK" },
	{ "cmdCreateHdfPath", "txtCreatePath", "txtCreatePath", "cmdCreateHdfOK", "chkDynamic" },
	{ "chkDynamic", "", "", "cmdCreateHdfPath", "cmdCreateHdfOK" },
	{ "cmdCreateHdfOK", "cmdCreateHdfCancel", "cmdCreateHdfCancel", "chkDynamic", "cmdCreateHdfPath" },
	{ "cmdCreateHdfCancel", "cmdCreateHdfOK", "cmdCreateHdfOK", "chkDynamic", "cmdCreateHdfPath" },

	{ "END", "", "", "", "" }
};


bool handle_navigation(const int direction)
{
	const gcn::FocusHandler* focus_hdl = gui_top->_getFocusHandler();
	gcn::Widget* focus_target = nullptr;

	if (focus_hdl != nullptr)
	{
		gcn::Widget* active_widget = focus_hdl->getFocused();

		if (active_widget != nullptr)
		{
			std::string active_name = active_widget->getId();
			if (!active_name.empty())
			{
				auto bFoundEnabled = false;
				auto tries = 10;

				while (!bFoundEnabled && tries > 0)
				{
					std::string search_for;

					for (auto i = 0; nav_map[i].active_widget != "END"; ++i)
					{
						if (nav_map[i].active_widget == active_name)
						{
							if (active_name.substr(0, 3) != "txt")
							{
								switch (direction)
								{
								case DIRECTION_LEFT:
									search_for = nav_map[i].left_widget;
									break;
								case DIRECTION_RIGHT:
									search_for = nav_map[i].right_widget;
									break;
								case DIRECTION_UP:
									search_for = nav_map[i].up_widget;
									break;
								case DIRECTION_DOWN:
									search_for = nav_map[i].down_widget;
									break;
								default:
									break;
								}
							}
							if (!search_for.empty())
							{
								focus_target = gui_top->findWidgetById(search_for);
								if (focus_target != nullptr)
								{
									if (focus_target->isEnabled() && focus_target->isVisible())
										bFoundEnabled = true;
									else
										active_name = search_for;
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
					if (search_for.empty())
						bFoundEnabled = true; // No entry to navigate to -> exit loop
					--tries;
				}

				if (active_name.substr(0, 4) == "scrl")
				{
					// Scroll area detected
					auto* scrollarea = dynamic_cast<gcn::ScrollArea*>(active_widget);
					if (direction == DIRECTION_UP)
					{
						const auto scroll = scrollarea->getVerticalScrollAmount();
						scrollarea->setVerticalScrollAmount(scroll - 30);
					}
					else if (direction == DIRECTION_DOWN)
					{
						const auto scroll = scrollarea->getVerticalScrollAmount();
						scrollarea->setVerticalScrollAmount(scroll + 30);
					}
				}

				if (focus_target != nullptr && active_name.substr(0, 3) == "cbo")
				{
					auto* dropdown = dynamic_cast<gcn::DropDown*>(active_widget);
					if (dropdown && dropdown->isEnabled() && dropdown->isDroppedDown() && (direction == DIRECTION_UP || direction == DIRECTION_DOWN))
						focus_target = nullptr; // Up/down navigates in list if dropped down
				}
			}
		}
	}

	if (focus_target != nullptr)
		focus_target->requestFocus();
	return focus_target != nullptr;
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

bool handle_keydown(SDL_Event& event, bool& dialog_finished, bool& nav_left, bool& nav_right)
{
	const auto key = event.key.keysym.sym;

	auto push_fake_key_multiple_times = [&](const SDL_Keycode key_code, const int times = 1) {
		for (auto z = 0; z < times; ++z) {
			PushFakeKey(key_code);
		}
		};

	switch (key)
	{
	case VK_ESCAPE:
		dialog_finished = true;
		break;

	case VK_LEFT:
		push_fake_key_multiple_times(SDLK_LEFT);
		nav_left = true;
		break;

	case VK_RIGHT:
		push_fake_key_multiple_times(SDLK_RIGHT);
		nav_right = true;
		break;

	case VK_Red:
	case VK_Green:
		event.key.keysym.sym = SDLK_RETURN;
		gui_input->pushInput(event); // Fire key down
		event.type = SDL_KEYUP; // and the key up
		break;

	case SDLK_PAGEDOWN:
		push_fake_key_multiple_times(SDLK_DOWN, 10);
		break;

	case SDLK_PAGEUP:
		push_fake_key_multiple_times(SDLK_UP, 10);
		break;

	default:
		break;
	}
	return true;
}

bool handle_joybutton(didata* did, bool& dialog_finished, bool& nav_left, bool& nav_right)
{
	const int hat = SDL_JoystickGetHat(gui_joystick, 0);

	if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_A]) ||
		SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_B]))
	{
		PushFakeKey(SDLK_RETURN);
		return true;
	}
	if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_X]) ||
		SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_Y]) ||
		SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_START]))
	{
		dialog_finished = true;
		return true;
	}
	if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_LEFT]) || hat & SDL_HAT_LEFT)
	{
		nav_left = true;
		return true;
	}
	if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_RIGHT]) || hat & SDL_HAT_RIGHT)
	{
		nav_right = true;
		return true;
	}
	if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_UP]) || hat & SDL_HAT_UP)
	{
		PushFakeKey(SDLK_UP);
		return true;
	}
	if (SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_DPAD_DOWN]) || hat & SDL_HAT_DOWN)
	{
		PushFakeKey(SDLK_DOWN);
		return true;
	}
	if ((did->mapping.is_retroarch || !did->is_controller)
		&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
		|| SDL_GameControllerGetButton(did->controller,
			static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_LEFTSHOULDER])))
	{
		for (auto z = 0; z < 10; ++z)
		{
			PushFakeKey(SDLK_UP);
		}
	}
	if ((did->mapping.is_retroarch || !did->is_controller)
		&& SDL_JoystickGetButton(gui_joystick, did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
		|| SDL_GameControllerGetButton(did->controller,
			static_cast<SDL_GameControllerButton>(did->mapping.button[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])))
	{
		for (auto z = 0; z < 10; ++z)
		{
			PushFakeKey(SDLK_DOWN);
		}
	}

	return true;
}

bool handle_joyaxis(const SDL_Event& event, bool& nav_left, bool& nav_right)
{
	const auto value = event.jaxis.value;
	const auto axis = event.jaxis.axis;

	if (axis == SDL_CONTROLLER_AXIS_LEFTX)
	{
		if (value > joystick_dead_zone && last_x != 1)
		{
			last_x = 1;
			nav_right = true;
		}
		else if (value < -joystick_dead_zone && last_x != -1)
		{
			last_x = -1;
			nav_left = true;
		}
		else if (value > -joystick_dead_zone && value < joystick_dead_zone)
		{
			last_x = 0;
		}
	}
	else if (axis == SDL_CONTROLLER_AXIS_LEFTY)
	{
		if (value < -joystick_dead_zone && last_y != -1)
		{
			last_y = -1;
			PushFakeKey(SDLK_UP);
		}
		else if (value > joystick_dead_zone && last_y != 1)
		{
			last_y = 1;
			PushFakeKey(SDLK_DOWN);
		}
		else if (value > -joystick_dead_zone && value < joystick_dead_zone)
		{
			last_y = 0;
		}
	}
	return true;
}

bool handle_finger(const SDL_Event& event, SDL_Event& touch_event)
{
	// Copy the event to touch_event
	memcpy(&touch_event, &event, sizeof event);

	// Calculate the x and y coordinates
	const int x = gui_graphics->getTarget()->w * static_cast<int>(event.tfinger.x);
	const int y = gui_graphics->getTarget()->h * static_cast<int>(event.tfinger.y);

	switch (event.type)
	{
	case SDL_FINGERDOWN:
		touch_event.type = SDL_MOUSEBUTTONDOWN;
		touch_event.button.which = 0;
		touch_event.button.button = SDL_BUTTON_LEFT;
		touch_event.button.state = SDL_PRESSED;
		touch_event.button.x = x;
		touch_event.button.y = y;
		break;
	case SDL_FINGERUP:
		touch_event.type = SDL_MOUSEBUTTONUP;
		touch_event.button.which = 0;
		touch_event.button.button = SDL_BUTTON_LEFT;
		touch_event.button.state = SDL_RELEASED;
		touch_event.button.x = x;
		touch_event.button.y = y;
		break;
	case SDL_FINGERMOTION:
		touch_event.type = SDL_MOUSEMOTION;
		touch_event.motion.which = 0;
		touch_event.motion.state = 0;
		touch_event.motion.x = x;
		touch_event.motion.y = y;
		break;
	default:
		break;
	}
	return true;
}

bool handle_mousewheel(const SDL_Event& event)
{
	const auto key = event.wheel.y > 0 ? SDLK_UP : SDLK_DOWN;
	const auto times = std::abs(event.wheel.y);

	for (auto z = 0; z < times; ++z)
	{
		PushFakeKey(key);
	}

	return true;
}

