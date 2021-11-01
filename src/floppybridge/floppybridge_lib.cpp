/* floppybridge_lib
*
* Copyright (C) 2021 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This class connects to the external FloppyBridge DLL library rather than
* having all the code compiled in. That library is maintained at
* https://amiga.robsmithdev.co.uk/winuae
*
* This is free and unencumbered released into the public domain
* But please don't remove the above information
*
* For more details visit <http://unlicense.org>.
*
*/

#include "floppybridge_lib.h"
#include <string>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <cstring>
#ifndef _WIN32
#include <dlfcn.h>
#endif


// Used by BRIDGE_About
struct BridgeAbout {
	const char* about;
	const char* url;
	unsigned int majorVersion, minorVersion;
	unsigned int isBeta;
	unsigned int isUpdateAvailable;
	unsigned int updateMajorVersion, updateMinorVersion;
};

// Information about a floppy bridge profile
struct FloppyBridgeProfileInformationDLL {
	// Unique ID of this profile
	unsigned int profileID;

	// Driver Index, incase it's shown on the GUI
	unsigned int driverIndex;

	// Some basic information
	FloppyBridgeAPI::BridgeMode bridgeMode;
	FloppyBridgeAPI::BridgeDensityMode bridgeDensityMode;

	// Profile name
	char* name;

	// Pointer to the Configuration data for this profile. - Be careful. Assume this pointer is invalid after calling *any* of the *profile* functions apart from getAllProfiles
	char* profileConfig;
};

#ifdef _WIN32
#include <Windows.h>
#ifdef WINUAE
HMODULE WIN32_LoadLibrary(const TCHAR*);
#endif
#define CALLING_CONVENSION _cdecl
#define GETFUNC GetProcAddress
#else
#define CALLING_CONVENSION
#define GETFUNC dlsym
#endif

#ifdef _WIN64
#define MODULENAME _T("FloppyBridge_x64.dll")
#else
#ifdef _WIN32
#define MODULENAME _T("FloppyBridge.dll")
#else
#define MODULENAME "FloppyBridge.so"
#endif
#endif

#ifdef _WIN32
HMODULE hBridgeDLLHandle = 0;
#else
void* hBridgeDLLHandle = nullptr;
#endif



// Bridge library function definitions
typedef void 			 (CALLING_CONVENSION* _BRIDGE_About)(bool allowCheckForUpdates, BridgeAbout** output);
typedef unsigned int 	 (CALLING_CONVENSION* _BRIDGE_NumDrivers)(void);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_GetDriverInfo)(unsigned int driverIndex, FloppyDiskBridge::BridgeDriver** driverInformation);
#ifdef _WIN32
typedef bool			 (CALLING_CONVENSION* _BRIDGE_ShowConfigDialog)(HWND hwndParent, unsigned int* profileID);
#endif
typedef bool			 (CALLING_CONVENSION* _BRIDGE_GetAllProfiles)(FloppyBridgeProfileInformationDLL** profiles, unsigned int* numProfiles);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_ImportProfilesFromString)(char* profilesConfigString);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_ExportProfilesToString)(char** profilesConfigString);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_GetProfileConfigFromString)(unsigned int profileID, char** configString);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_SetProfileConfigFromString)(unsigned int profileID, char* configString);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_SetProfileName)(unsigned int profileID, char* name);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_CreateNewProfile)(unsigned int driverIndex, unsigned int* profileID);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_DeleteProfile)(unsigned int profileID);
typedef bool		 	 (CALLING_CONVENSION* _BRIDGE_EnumComports)(char* output, unsigned int* bufferSize);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_CreateDriver)(unsigned int driverIndex, BridgeDriverHandle* bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_CreateDriverFromConfigString)(char* config, BridgeDriverHandle* bridgeDriverHandle);
typedef bool			 (CALLING_CONVENSION* _BRIDGE_CreateDriverFromProfileID)(unsigned int profileID, BridgeDriverHandle* bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_Close)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_Open)(BridgeDriverHandle bridgeDriverHandle, char** errorMessage);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_GetDriverIndex)(BridgeDriverHandle bridgeDriverHandle, unsigned int* driverIndex);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_FreeDriver)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_GetConfigString)(BridgeDriverHandle bridgeDriverHandle, char** config);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_SetConfigFromString)(BridgeDriverHandle bridgeDriverHandle, char* config);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetAutoCache)(BridgeDriverHandle bridgeDriverHandle, bool* isAutoCacheMode);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetAutoCache)(BridgeDriverHandle bridgeDriverHandle, bool isAutoCacheMode);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetMode)(BridgeDriverHandle bridgeDriverHandle, FloppyBridgeAPI::BridgeMode* bridgeMode);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetMode)(BridgeDriverHandle bridgeDriverHandle, FloppyBridgeAPI::BridgeMode bridgeMode);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetDensityMode)(BridgeDriverHandle bridgeDriverHandle, FloppyBridgeAPI::BridgeDensityMode* densityMode);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetDensityMode)(BridgeDriverHandle bridgeDriverHandle, FloppyBridgeAPI::BridgeDensityMode densityMode);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetCurrentComPort)(BridgeDriverHandle bridgeDriverHandle, char** comPort);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetCurrentComPort)(BridgeDriverHandle bridgeDriverHandle, char* comPort);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetAutoDetectComPort)(BridgeDriverHandle bridgeDriverHandle, bool* autoDetectComPort);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetAutoDetectComPort)(BridgeDriverHandle bridgeDriverHandle, bool autoDetectComPort);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetCable)(BridgeDriverHandle bridgeDriverHandle, bool* isOnB);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetCable)(BridgeDriverHandle bridgeDriverHandle, bool isOnB);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverGetSmartSpeedEnabled)(BridgeDriverHandle bridgeDriverHandle, bool* enabled);
typedef bool 			 (CALLING_CONVENSION* _BRIDGE_DriverSetSmartSpeedEnabled)(BridgeDriverHandle bridgeDriverHandle, bool enabled);
typedef unsigned char 	 (CALLING_CONVENSION* _DRIVER_getBitSpeed)(BridgeDriverHandle bridgeDriverHandle);
typedef FloppyDiskBridge::DriveTypeID (CALLING_CONVENSION* _DRIVER_getDriveTypeID)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_resetDrive)(BridgeDriverHandle bridgeDriverHandle, int trackNumber);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isAtCylinder0)(BridgeDriverHandle bridgeDriverHandle);
typedef unsigned char 	 (CALLING_CONVENSION* _DRIVER_getMaxCylinder)(BridgeDriverHandle bridgeDriverHandle);
typedef void 			 (CALLING_CONVENSION* _DRIVER_gotoCylinder)(BridgeDriverHandle bridgeDriverHandle, int cylinderNumber, bool side);
typedef void 			 (CALLING_CONVENSION* _DRIVER_handleNoClickStep)(BridgeDriverHandle bridgeDriverHandle, bool side);
typedef unsigned char 	 (CALLING_CONVENSION* _DRIVER_getCurrentCylinderNumber)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isMotorRunning)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_getCurrentSide)(BridgeDriverHandle bridgeDriverHandle);
typedef void 			 (CALLING_CONVENSION* _DRIVER_setMotorStatus)(BridgeDriverHandle bridgeDriverHandle, bool side, bool turnOn);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isReady)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isDiskInDrive)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_hasDiskChanged)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isMFMPositionAtIndex)(BridgeDriverHandle bridgeDriverHandle, int mfmPositionBits);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isMFMDataAvailable)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_getMFMBit)(BridgeDriverHandle bridgeDriverHandle, int mfmPositionBits);
typedef int 			 (CALLING_CONVENSION* _DRIVER_getMFMSpeed)(BridgeDriverHandle bridgeDriverHandle, int mfmPositionBits);
typedef void 			 (CALLING_CONVENSION* _DRIVER_mfmSwitchBuffer)(BridgeDriverHandle bridgeDriverHandle, bool side);
typedef void 			 (CALLING_CONVENSION* _DRIVER_setSurface)(BridgeDriverHandle bridgeDriverHandle, bool side);
typedef int 			 (CALLING_CONVENSION* _DRIVER_maxMFMBitPosition)(BridgeDriverHandle bridgeDriverHandle);
typedef void 			 (CALLING_CONVENSION* _DRIVER_writeShortToBuffer)(BridgeDriverHandle bridgeDriverHandle, bool side, unsigned int track, unsigned short mfmData, int mfmPosition);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isWriteProtected)(BridgeDriverHandle bridgeDriverHandle);
typedef unsigned int 	 (CALLING_CONVENSION* _DRIVER_commitWriteBuffer)(BridgeDriverHandle bridgeDriverHandle, bool side, unsigned int track);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isWritePending)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isWriteComplete)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_canTurboWrite)(BridgeDriverHandle bridgeDriverHandle);
typedef bool 			 (CALLING_CONVENSION* _DRIVER_isReadyToWrite)(BridgeDriverHandle bridgeDriverHandle);


// Library function pointers
_BRIDGE_About	BRIDGE_About = nullptr;
_BRIDGE_NumDrivers	BRIDGE_NumDrivers = nullptr;
_BRIDGE_EnumComports	BRIDGE_EnumComports = nullptr;
_BRIDGE_GetDriverInfo	BRIDGE_GetDriverInfo = nullptr;
_BRIDGE_CreateDriver	BRIDGE_CreateDriver = nullptr;
_BRIDGE_Close	BRIDGE_Close = nullptr;
_BRIDGE_Open	BRIDGE_Open = nullptr;
_BRIDGE_CreateDriverFromProfileID BRIDGE_CreateDriverFromProfileID = nullptr;
_BRIDGE_GetAllProfiles BRIDGE_GetAllProfiles = nullptr;
_BRIDGE_ImportProfilesFromString BRIDGE_ImportProfilesFromString = nullptr;
_BRIDGE_ExportProfilesToString BRIDGE_ExportProfilesToString = nullptr;
_BRIDGE_GetProfileConfigFromString BRIDGE_GetProfileConfigFromString = nullptr;
_BRIDGE_SetProfileConfigFromString BRIDGE_SetProfileConfigFromString = nullptr;
_BRIDGE_SetProfileName BRIDGE_SetProfileName = nullptr;
_BRIDGE_CreateNewProfile BRIDGE_CreateNewProfile = nullptr;
_BRIDGE_DeleteProfile BRIDGE_DeleteProfile = nullptr;
#ifdef _WIN32
_BRIDGE_ShowConfigDialog BRIDGE_ShowConfigDialog = nullptr;
#endif
_BRIDGE_GetDriverIndex BRIDGE_GetDriverIndex = nullptr;
_BRIDGE_FreeDriver	BRIDGE_FreeDriver = nullptr;
_BRIDGE_DriverGetMode	BRIDGE_DriverGetMode = nullptr;
_BRIDGE_DriverSetMode	BRIDGE_DriverSetMode = nullptr;
_BRIDGE_DriverGetDensityMode	BRIDGE_DriverGetDensityMode = nullptr;
_BRIDGE_DriverSetDensityMode	BRIDGE_DriverSetDensityMode = nullptr;
_BRIDGE_DriverGetCurrentComPort	BRIDGE_DriverGetCurrentComPort = nullptr;
_BRIDGE_DriverSetCurrentComPort	BRIDGE_DriverSetCurrentComPort = nullptr;
_BRIDGE_DriverGetAutoDetectComPort	BRIDGE_DriverGetAutoDetectComPort = nullptr;
_BRIDGE_DriverSetAutoDetectComPort	BRIDGE_DriverSetAutoDetectComPort = nullptr;
_BRIDGE_DriverGetCable	BRIDGE_DriverGetCable = nullptr;
_BRIDGE_DriverSetCable	BRIDGE_DriverSetCable = nullptr;
_BRIDGE_DriverGetAutoCache BRIDGE_DriverGetAutoCache = nullptr;
_BRIDGE_DriverSetAutoCache BRIDGE_DriverSetAutoCache = nullptr;
_BRIDGE_DriverGetSmartSpeedEnabled BRIDGE_DriverGetSmartSpeedEnabled = nullptr;
_BRIDGE_DriverSetSmartSpeedEnabled BRIDGE_DriverSetSmartSpeedEnabled = nullptr;
_BRIDGE_GetConfigString BRIDGE_GetConfigString = nullptr;
_BRIDGE_SetConfigFromString BRIDGE_SetConfigFromString = nullptr;
_BRIDGE_CreateDriverFromConfigString BRIDGE_CreateDriverFromConfigString = nullptr;
_DRIVER_getBitSpeed	DRIVER_getBitSpeed = nullptr;
_DRIVER_getDriveTypeID	DRIVER_getDriveTypeID = nullptr;
_DRIVER_resetDrive	DRIVER_resetDrive = nullptr;
_DRIVER_isAtCylinder0	DRIVER_isAtCylinder0 = nullptr;
_DRIVER_getCurrentSide DRIVER_getCurrentSide = nullptr;
_DRIVER_getMaxCylinder	DRIVER_getMaxCylinder = nullptr;
_DRIVER_gotoCylinder	DRIVER_gotoCylinder = nullptr;
_DRIVER_handleNoClickStep	DRIVER_handleNoClickStep = nullptr;
_DRIVER_getCurrentCylinderNumber	DRIVER_getCurrentCylinderNumber = nullptr;
_DRIVER_isMotorRunning	DRIVER_isMotorRunning = nullptr;
_DRIVER_setMotorStatus	DRIVER_setMotorStatus = nullptr;
_DRIVER_isReady	DRIVER_isReady = nullptr;
_DRIVER_isDiskInDrive	DRIVER_isDiskInDrive = nullptr;
_DRIVER_hasDiskChanged	DRIVER_hasDiskChanged = nullptr;
_DRIVER_isMFMPositionAtIndex	DRIVER_isMFMPositionAtIndex = nullptr;
_DRIVER_isMFMDataAvailable	DRIVER_isMFMDataAvailable = nullptr;
_DRIVER_getMFMBit	DRIVER_getMFMBit = nullptr;
_DRIVER_getMFMSpeed	DRIVER_getMFMSpeed = nullptr;
_DRIVER_mfmSwitchBuffer	DRIVER_mfmSwitchBuffer = nullptr;
_DRIVER_setSurface	DRIVER_setSurface = nullptr;
_DRIVER_maxMFMBitPosition	DRIVER_maxMFMBitPosition = nullptr;
_DRIVER_writeShortToBuffer	DRIVER_writeShortToBuffer = nullptr;
_DRIVER_isWriteProtected	DRIVER_isWriteProtected = nullptr;
_DRIVER_commitWriteBuffer	DRIVER_commitWriteBuffer = nullptr;
_DRIVER_isWritePending	DRIVER_isWritePending = nullptr;
_DRIVER_isWriteComplete	DRIVER_isWriteComplete = nullptr;
_DRIVER_canTurboWrite	DRIVER_canTurboWrite = nullptr;
_DRIVER_isReadyToWrite	DRIVER_isReadyToWrite = nullptr;

// Sets up the bridge.  We assume it will persist while the application is open.
void prepareBridge() {
	if (hBridgeDLLHandle) return;

#ifdef WIN32
#ifdef WINUAE
	hBridgeDLLHandle = WIN32_LoadLibrary(MODULENAME);
#else
#ifdef _UNICODE
	hBridgeDLLHandle = LoadLibraryW(MODULENAME);
#else
	hBridgeDLLHandle = LoadLibraryA(MODULENAME);
#endif
#endif
#else
	hBridgeDLLHandle = dlopen(MODULENAME, RTLD_NOW);
#endif

	// Did it open?
	if (!hBridgeDLLHandle) return;

	BRIDGE_About = (_BRIDGE_About)GETFUNC(hBridgeDLLHandle, "BRIDGE_About");
	BRIDGE_NumDrivers = (_BRIDGE_NumDrivers)GETFUNC(hBridgeDLLHandle, "BRIDGE_NumDrivers");
	BRIDGE_EnumComports = (_BRIDGE_EnumComports)GETFUNC(hBridgeDLLHandle, "BRIDGE_EnumComports");
	BRIDGE_GetDriverInfo = (_BRIDGE_GetDriverInfo)GETFUNC(hBridgeDLLHandle, "BRIDGE_GetDriverInfo");
	BRIDGE_CreateDriver = (_BRIDGE_CreateDriver)GETFUNC(hBridgeDLLHandle, "BRIDGE_CreateDriver");
	BRIDGE_GetDriverIndex = (_BRIDGE_GetDriverIndex)GETFUNC(hBridgeDLLHandle, "BRIDGE_GetDriverIndex");
#ifdef _WIN32
	BRIDGE_ShowConfigDialog = (_BRIDGE_ShowConfigDialog)GETFUNC(hBridgeDLLHandle, "BRIDGE_ShowConfigDialog");
#endif
	BRIDGE_Close = (_BRIDGE_Close)GETFUNC(hBridgeDLLHandle, "BRIDGE_Close");
	BRIDGE_Open = (_BRIDGE_Open)GETFUNC(hBridgeDLLHandle, "BRIDGE_Open");
	BRIDGE_CreateDriverFromProfileID = (_BRIDGE_CreateDriverFromProfileID)GETFUNC(hBridgeDLLHandle, "BRIDGE_CreateDriverFromProfileID");
	BRIDGE_GetAllProfiles = (_BRIDGE_GetAllProfiles)GETFUNC(hBridgeDLLHandle, "BRIDGE_GetAllProfiles");
	BRIDGE_ImportProfilesFromString = (_BRIDGE_ImportProfilesFromString)GETFUNC(hBridgeDLLHandle, "BRIDGE_ImportProfilesFromString");
	BRIDGE_ExportProfilesToString = (_BRIDGE_ExportProfilesToString)GETFUNC(hBridgeDLLHandle, "BRIDGE_ExportProfilesToString");
	BRIDGE_GetProfileConfigFromString = (_BRIDGE_GetProfileConfigFromString)GETFUNC(hBridgeDLLHandle, "BRIDGE_GetProfileConfigFromString");
	BRIDGE_SetProfileConfigFromString = (_BRIDGE_SetProfileConfigFromString)GETFUNC(hBridgeDLLHandle, "BRIDGE_SetProfileConfigFromString");
	BRIDGE_SetProfileName = (_BRIDGE_SetProfileName)GETFUNC(hBridgeDLLHandle, "BRIDGE_SetProfileName");	
	BRIDGE_CreateNewProfile = (_BRIDGE_CreateNewProfile)GETFUNC(hBridgeDLLHandle, "BRIDGE_CreateNewProfile");
	BRIDGE_DeleteProfile = (_BRIDGE_DeleteProfile)GETFUNC(hBridgeDLLHandle, "BRIDGE_DeleteProfile");
	BRIDGE_FreeDriver = (_BRIDGE_FreeDriver)GETFUNC(hBridgeDLLHandle, "BRIDGE_FreeDriver");
	BRIDGE_DriverGetAutoCache = (_BRIDGE_DriverGetAutoCache)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetAutoCache");
	BRIDGE_DriverSetAutoCache = (_BRIDGE_DriverSetAutoCache)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetAutoCache");
	BRIDGE_GetConfigString = (_BRIDGE_GetConfigString)GETFUNC(hBridgeDLLHandle, "BRIDGE_GetConfigString");
	BRIDGE_SetConfigFromString = (_BRIDGE_SetConfigFromString)GETFUNC(hBridgeDLLHandle, "BRIDGE_SetConfigFromString");
	BRIDGE_CreateDriverFromConfigString = (_BRIDGE_CreateDriverFromConfigString)GETFUNC(hBridgeDLLHandle, "BRIDGE_CreateDriverFromConfigString");
	BRIDGE_DriverGetMode = (_BRIDGE_DriverGetMode)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetMode");
	BRIDGE_DriverSetMode = (_BRIDGE_DriverSetMode)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetMode");
	BRIDGE_DriverGetDensityMode = (_BRIDGE_DriverGetDensityMode)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetDensityMode");
	BRIDGE_DriverSetDensityMode = (_BRIDGE_DriverSetDensityMode)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetDensityMode");
	BRIDGE_DriverGetCurrentComPort = (_BRIDGE_DriverGetCurrentComPort)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetCurrentComPort");
	BRIDGE_DriverSetCurrentComPort = (_BRIDGE_DriverSetCurrentComPort)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetCurrentComPort");
	BRIDGE_DriverGetAutoDetectComPort = (_BRIDGE_DriverGetAutoDetectComPort)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetAutoDetectComPort");
	BRIDGE_DriverSetAutoDetectComPort = (_BRIDGE_DriverSetAutoDetectComPort)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetAutoDetectComPort");
	BRIDGE_DriverGetSmartSpeedEnabled = (_BRIDGE_DriverGetSmartSpeedEnabled)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetSmartSpeedEnabled");
	BRIDGE_DriverSetSmartSpeedEnabled = (_BRIDGE_DriverSetSmartSpeedEnabled)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetSmartSpeedEnabled");
	BRIDGE_DriverGetCable = (_BRIDGE_DriverGetCable)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverGetCable");
	BRIDGE_DriverSetCable = (_BRIDGE_DriverSetCable)GETFUNC(hBridgeDLLHandle, "BRIDGE_DriverSetCable");
	DRIVER_getBitSpeed = (_DRIVER_getBitSpeed)GETFUNC(hBridgeDLLHandle, "DRIVER_getBitSpeed");
	DRIVER_getCurrentSide = (_DRIVER_getCurrentSide)GETFUNC(hBridgeDLLHandle, "DRIVER_getCurrentSide");
	DRIVER_getDriveTypeID = (_DRIVER_getDriveTypeID)GETFUNC(hBridgeDLLHandle, "DRIVER_getDriveTypeID");
	DRIVER_resetDrive = (_DRIVER_resetDrive)GETFUNC(hBridgeDLLHandle, "DRIVER_resetDrive");
	DRIVER_isAtCylinder0 = (_DRIVER_isAtCylinder0)GETFUNC(hBridgeDLLHandle, "DRIVER_isAtCylinder0");
	DRIVER_getMaxCylinder = (_DRIVER_getMaxCylinder)GETFUNC(hBridgeDLLHandle, "DRIVER_getMaxCylinder");
	DRIVER_gotoCylinder = (_DRIVER_gotoCylinder)GETFUNC(hBridgeDLLHandle, "DRIVER_gotoCylinder");
	DRIVER_handleNoClickStep = (_DRIVER_handleNoClickStep)GETFUNC(hBridgeDLLHandle, "DRIVER_handleNoClickStep");
	DRIVER_getCurrentCylinderNumber = (_DRIVER_getCurrentCylinderNumber)GETFUNC(hBridgeDLLHandle, "DRIVER_getCurrentCylinderNumber");
	DRIVER_isMotorRunning = (_DRIVER_isMotorRunning)GETFUNC(hBridgeDLLHandle, "DRIVER_isMotorRunning");
	DRIVER_setMotorStatus = (_DRIVER_setMotorStatus)GETFUNC(hBridgeDLLHandle, "DRIVER_setMotorStatus");
	DRIVER_isReady = (_DRIVER_isReady)GETFUNC(hBridgeDLLHandle, "DRIVER_isReady");
	DRIVER_isDiskInDrive = (_DRIVER_isDiskInDrive)GETFUNC(hBridgeDLLHandle, "DRIVER_isDiskInDrive");
	DRIVER_hasDiskChanged = (_DRIVER_hasDiskChanged)GETFUNC(hBridgeDLLHandle, "DRIVER_hasDiskChanged");
	DRIVER_isMFMPositionAtIndex = (_DRIVER_isMFMPositionAtIndex)GETFUNC(hBridgeDLLHandle, "DRIVER_isMFMPositionAtIndex");
	DRIVER_isMFMDataAvailable = (_DRIVER_isMFMDataAvailable)GETFUNC(hBridgeDLLHandle, "DRIVER_isMFMDataAvailable");
	DRIVER_getMFMBit = (_DRIVER_getMFMBit)GETFUNC(hBridgeDLLHandle, "DRIVER_getMFMBit");
	DRIVER_getMFMSpeed = (_DRIVER_getMFMSpeed)GETFUNC(hBridgeDLLHandle, "DRIVER_getMFMSpeed");
	DRIVER_mfmSwitchBuffer = (_DRIVER_mfmSwitchBuffer)GETFUNC(hBridgeDLLHandle, "DRIVER_mfmSwitchBuffer");
	DRIVER_setSurface = (_DRIVER_setSurface)GETFUNC(hBridgeDLLHandle, "DRIVER_setSurface");
	DRIVER_maxMFMBitPosition = (_DRIVER_maxMFMBitPosition)GETFUNC(hBridgeDLLHandle, "DRIVER_maxMFMBitPosition");
	DRIVER_writeShortToBuffer = (_DRIVER_writeShortToBuffer)GETFUNC(hBridgeDLLHandle, "DRIVER_writeShortToBuffer");
	DRIVER_isWriteProtected = (_DRIVER_isWriteProtected)GETFUNC(hBridgeDLLHandle, "DRIVER_isWriteProtected");
	DRIVER_commitWriteBuffer = (_DRIVER_commitWriteBuffer)GETFUNC(hBridgeDLLHandle, "DRIVER_commitWriteBuffer");
	DRIVER_isWritePending = (_DRIVER_isWritePending)GETFUNC(hBridgeDLLHandle, "DRIVER_isWritePending");
	DRIVER_isWriteComplete = (_DRIVER_isWriteComplete)GETFUNC(hBridgeDLLHandle, "DRIVER_isWriteComplete");
	DRIVER_canTurboWrite = (_DRIVER_canTurboWrite)GETFUNC(hBridgeDLLHandle, "DRIVER_canTurboWrite");
	DRIVER_isReadyToWrite = (_DRIVER_isReadyToWrite)GETFUNC(hBridgeDLLHandle, "DRIVER_isReadyToWrite");

	// Test a few
	if ((!BRIDGE_About) || (!BRIDGE_NumDrivers) || (!BRIDGE_DeleteProfile)) {
#ifdef WIN32
		if (hBridgeDLLHandle) FreeLibrary(hBridgeDLLHandle);
		hBridgeDLLHandle = 0;
#else
		if (hBridgeDLLHandle) dlclose(hBridgeDLLHandle);
		hBridgeDLLHandle = nullptr;
#endif
	}
}


// character conversions
using convert_t = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_t, wchar_t> strconverter;

void _quickw2a(const std::wstring& wstr, std::string& str) {
	str = strconverter.to_bytes(wstr);
}
void _quicka2w(const std::string& str, std::wstring& wstr) {
	wstr = strconverter.from_bytes(str);
}

// Copy or convert a char* to a TCHAR
void _char2TChar(const char* input, TCHAR* output, unsigned maxLength) {
#ifdef _UNICODE
	std::wstring outputw;
	_quicka2w(input, outputw);
#ifdef _WIN32
	wcscpy_s(output, maxLength, outputw.c_str());
#else
	wcscpy(output, outputw.c_str());
#endif
#else
#ifdef _WIN32
	strcpy_s(output, maxLength, input);
#else
	strcpy(output, input);
#endif
#endif
}

#ifdef _UNICODE
std::vector<std::wstring> memoryPortList;
#else
std::vector<std::string> memoryPortList;
#endif
std::vector<std::string> stringListsForProfiles;


/*********** STATIC FUNCTIONS ************************/

// Returns TRUE if the floppy bridge library has been loaded and is ready to be queried
const bool FloppyBridgeAPI::isAvailable() {
	prepareBridge();

	return hBridgeDLLHandle != 0;
}

// Populates bridgeInformation with information about the Bridge DLL. This should be called and shown somewhere
// As it contains update and support information too
bool FloppyBridgeAPI::getBridgeDriverInformation(bool allowCheckForUpdates, BridgeInformation& bridgeInformation) {
	if (!isAvailable()) {
		// Populate some basics
		memset(&bridgeInformation, 0, sizeof(bridgeInformation));
		_char2TChar("FloppyBridge Driver Not Installed.", bridgeInformation.about, BRIDGE_STRING_MAX_LENGTH - 1);
		_char2TChar("https://amiga.robsmithdev.co.uk/winuae", bridgeInformation.url, BRIDGE_STRING_MAX_LENGTH - 1);
		return false;
	}
	
	BridgeAbout* info = nullptr;
	BRIDGE_About(allowCheckForUpdates, &info);
	if (!info) return false;

	bridgeInformation.isBeta = info->isBeta != 0;
	bridgeInformation.isUpdateAvailable = info->isUpdateAvailable != 0;

	bridgeInformation.majorVersion = info->majorVersion;
	bridgeInformation.minorVersion = info->minorVersion;
	bridgeInformation.updateMajorVersion = info->updateMajorVersion;
	bridgeInformation.updateMinorVersion = info->updateMinorVersion;

	_char2TChar(info->about, bridgeInformation.about, BRIDGE_STRING_MAX_LENGTH - 1);
	_char2TChar(info->url, bridgeInformation.url, BRIDGE_STRING_MAX_LENGTH - 1);
	return true;
}

// Populates driverList with a list of available floppy bridge drivers that could be created
void FloppyBridgeAPI::getDriverList(std::vector<DriverInformation>& driverList) {
	driverList.clear();

	if (!isAvailable()) return;

	unsigned int total = BRIDGE_NumDrivers();
	if (total < 1) return;

	BridgeDriver* info = nullptr;
	for (unsigned int index = 0; index < total; index++) {
		if (BRIDGE_GetDriverInfo(index, &info)) {

			DriverInformation infoOut;

			_char2TChar(info->name, infoOut.name, BRIDGE_STRING_MAX_LENGTH - 1);
			_char2TChar(info->url, infoOut.url, BRIDGE_STRING_MAX_LENGTH - 1);
			_char2TChar(info->manufacturer, infoOut.manufacturer, BRIDGE_STRING_MAX_LENGTH - 1);
			_char2TChar(info->driverAuthor, infoOut.driverAuthor, BRIDGE_STRING_MAX_LENGTH - 1);
			infoOut.configOptions = info->configOptions;
			infoOut.driverIndex = index;

			driverList.push_back(infoOut);
		}
	}
}

// Creates a driver.  If it fails, it will return NULL.  It should only fail if the index is invalid.
FloppyBridgeAPI* FloppyBridgeAPI::createDriver(unsigned int driverIndex) {
	if (!isAvailable()) return nullptr;

	BridgeDriverHandle driverHandle = nullptr;

	if (!BRIDGE_CreateDriver(driverIndex, &driverHandle)) return nullptr;

	// Create the class and return it.
	return new FloppyBridgeAPI(driverIndex, driverHandle);
}

// Createw a driver from a config string previously saved.  This will auto-select the driverIndex.
FloppyBridgeAPI* FloppyBridgeAPI::createDriverFromString(const char* config) {
	if (!isAvailable()) return nullptr;

	BridgeDriverHandle driverHandle = nullptr;

	if (!BRIDGE_CreateDriverFromConfigString((char*)config, &driverHandle)) return nullptr;

	unsigned int driverIndex;
	if (!BRIDGE_GetDriverIndex(driverHandle, &driverIndex)) {
		BRIDGE_FreeDriver(driverHandle);
		return nullptr;
	}

	return new FloppyBridgeAPI(driverIndex, driverHandle);
}

// Populates portList with a list of serial port devices that you can use with setComPort() below
void FloppyBridgeAPI::enumCOMPorts(std::vector<const TCHAR*>& portList) {
	portList.clear();

	if (!isAvailable()) return;

	unsigned int sizeNeeded = 0;
	BRIDGE_EnumComports(NULL, &sizeNeeded);

	char* tmp = (char*)malloc(sizeNeeded);
	if (!tmp) return;

	if (BRIDGE_EnumComports(tmp, &sizeNeeded)) {
		char* str = tmp;
		TCharString opString;

		memoryPortList.clear();

		while (*str) {
			_char2TChar(str, opString, BRIDGE_STRING_MAX_LENGTH);
			memoryPortList.push_back(opString);
			str += strlen(str) + 1;  // skip pas the null terminator
		}

		for (auto& string : memoryPortList)
			portList.push_back(string.c_str());
	}
	free(tmp);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile based management


// Creates the driver instance from a profile ID.  You need to call importProfilesFromString() before using this function
FloppyBridgeAPI* FloppyBridgeAPI::createDriverFromProfileID(unsigned int profileID) {
	if (!isAvailable()) return nullptr;

	BridgeDriverHandle driverHandle = nullptr;

	if (!BRIDGE_CreateDriverFromProfileID(profileID, &driverHandle)) return nullptr;

	unsigned int driverIndex;
	if (!BRIDGE_GetDriverIndex(driverHandle, &driverIndex)) {
		BRIDGE_FreeDriver(driverHandle);
		return nullptr;
	}

	return new FloppyBridgeAPI(driverIndex, driverHandle);
}

// Retreive a list of all of the profiles currently loaded that can be used.
bool FloppyBridgeAPI::getAllProfiles(std::vector<FloppyBridgeProfileInformation>& profileList) {
	if (!isAvailable()) return false;

	profileList.clear();
	stringListsForProfiles.clear();

	FloppyBridgeProfileInformationDLL* profile = nullptr;
	unsigned int numProfiles = 0;

	if (!BRIDGE_GetAllProfiles(&profile, &numProfiles)) return false;
	
	while (numProfiles) {
		FloppyBridgeProfileInformation p;
		p.driverIndex = profile->driverIndex;
		p.profileID = profile->profileID;

		p.bridgeMode = profile->bridgeMode;
		p.bridgeDensityMode = profile->bridgeDensityMode;

		_char2TChar(profile->name, p.name, BRIDGE_STRING_MAX_LENGTH);
		stringListsForProfiles.push_back(profile->profileConfig);
		profileList.push_back(p);
		numProfiles--;
		profile++;
	}

	// Just populate the strings.. This was incase vector resizes etc changed memory locations
	for (size_t pos = 0; pos < profileList.size(); pos++)
		profileList[pos].profileConfig = stringListsForProfiles[pos].c_str();

	return true;
}

// Imports all profiles into memory from the supplied string.  This will erase any currently in memory
bool FloppyBridgeAPI::importProfilesFromString(const char* profilesString) {
	if (!isAvailable()) return false;

	return BRIDGE_ImportProfilesFromString((char*)profilesString);
}

// Exports all profiles and returns a pointer to the string.  This pointer is only valid while the driver is loaded and until this is called again
bool FloppyBridgeAPI::exportProfilesToString(char** profilesString) {
	if (!isAvailable()) return false;

	return BRIDGE_ExportProfilesToString(profilesString);
}

// Returns a pointer to a string containing the details for a profile
bool FloppyBridgeAPI::getProfileConfigAsString(unsigned int profileID, char** config) {
	if (!isAvailable()) return false;

	return BRIDGE_GetProfileConfigFromString(profileID, config);
}

// Creates a new profile and returns its unique ID
bool FloppyBridgeAPI::createNewProfile(unsigned int driverIndex, unsigned int* profileID) {
	if (!isAvailable()) return false;

	return BRIDGE_CreateNewProfile(driverIndex, profileID);
}


// Updates a profile from the supplied string
bool FloppyBridgeAPI::setProfileConfigFromString(unsigned int profileID, const char* config) {
	if (!isAvailable()) return false;

	return BRIDGE_SetProfileConfigFromString(profileID, (char*)config);
}

// Updates a profile from the supplied string
bool FloppyBridgeAPI::setProfileName(unsigned int profileID, const char* config) {
	if (!isAvailable()) return false;

	return BRIDGE_SetProfileName(profileID, (char*)config);
}

// Deletes a profile by ID.
bool FloppyBridgeAPI::deleteProfile(unsigned int profileID) {
	if (!isAvailable()) return false;

	return BRIDGE_DeleteProfile(profileID);
}

#ifdef _WIN32
// Displys the config dialog (modal) for Floppy Bridge profiles.  
// *If* you pass a profile ID, the dialog will jump to editing that profile, or return FALSE if it was not found.
// Returns FALSE if cancel was pressed
bool FloppyBridgeAPI::showProfileConfigDialog(HWND hwndParent, unsigned int* profileID) {
	if (!isAvailable()) return false;
	 
	return BRIDGE_ShowConfigDialog(hwndParent, profileID);
}
#endif

/*********** CLASS FUNCTIONS ************************/

// Dont call this. You should use the static createDriver member to create it.
FloppyBridgeAPI::FloppyBridgeAPI(unsigned int driverIndex, BridgeDriverHandle handle) : FloppyDiskBridge(), m_handle(handle), m_driverIndex(driverIndex) {
}

FloppyBridgeAPI::~FloppyBridgeAPI() {	
	BRIDGE_FreeDriver(m_handle);
}

/************** CONFIG RELATED FUNCTIONS *************************************/
// Returns a pointer to a string containing the current config.  This can be used with setConfigFromString() or createDriverFromString()
bool FloppyBridgeAPI::getConfigAsString(char** config) {
	return BRIDGE_GetConfigString(m_handle, config);
}
// Applies the config to the currently driver.  Returns TRUE if successful.
bool FloppyBridgeAPI::setConfigFromString(char* config) {
	return BRIDGE_SetConfigFromString(m_handle, config);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return the current bridge mode selected
bool FloppyBridgeAPI::getBridgeMode(FloppyBridgeAPI::BridgeMode* mode) {
	return BRIDGE_DriverGetMode(m_handle, mode);
}
// Set the currently active bridge mode.  This can be set while the bridge is in use
bool FloppyBridgeAPI::setBridgeMode(BridgeMode newMode) {
	return BRIDGE_DriverSetMode(m_handle, newMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return the current bridge density mode selected
bool FloppyBridgeAPI::getBridgeDensityMode(FloppyBridgeAPI::BridgeDensityMode* mode) {
	return BRIDGE_DriverGetDensityMode(m_handle, mode);
}
// Set the currently active bridge density mode.  This can be set while the bridge is in use
bool FloppyBridgeAPI::setBridgeDensityMode(BridgeDensityMode newMode) {
	return BRIDGE_DriverSetDensityMode(m_handle, newMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These require ConfigOption_AutoCache bit set in DriverInformation::configOptions
// Returns if auto-disk caching (while the drive is idle) mode is enabled
bool FloppyBridgeAPI::getAutoCacheMode(bool* autoCacheMode) {
	return BRIDGE_DriverGetAutoCache(m_handle, autoCacheMode);
}
// Sets if auto-disk caching (while the drive is idle) mode is enabled.  This can be set while the bridge is in use
bool FloppyBridgeAPI::setAutoCacheMode(bool autoCacheMode) {
	return BRIDGE_DriverSetAutoCache(m_handle, autoCacheMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These require ConfigOption_ComPort bit set in DriverInformation::configOptions
// Returns the currently selected COM port.  This port is only used if auto detect com port is false
bool FloppyBridgeAPI::getComPort(TCharString* comPort) {
	char* port = nullptr;
	if (!BRIDGE_DriverGetCurrentComPort(m_handle, &port)) return false;
	if (!port) return false;
	
	_char2TChar(port, *comPort, BRIDGE_STRING_MAX_LENGTH - 1);
	return true;
}
// Sets the com port to use.  This port is only used if auto detect com port is false.
bool FloppyBridgeAPI::setComPort(TCHAR* comPort) {
	if (!comPort) return false;
	
#ifdef _UNICODE
	std::string comPortA;
	_quickw2a(comPort, comPortA);
	return BRIDGE_DriverSetCurrentComPort(m_handle, (char*)comPortA.c_str());
#else
	return BRIDGE_DriverSetCurrentComPort(m_handle, comPort);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These require ConfigOption_AutoDetectComport bit set in DriverInformation::configOptions
// Returns if com port auto-detect is enabled
bool FloppyBridgeAPI::getComPortAutoDetect(bool* autoDetect) {
	return BRIDGE_DriverGetAutoDetectComPort(m_handle, autoDetect);
}
// Sets if auto-detect com port should be used
bool FloppyBridgeAPI::setComPortAutoDetect(bool autoDetect) {
	return BRIDGE_DriverSetAutoDetectComPort(m_handle, autoDetect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These require ConfigOption_DriveABCable bit set in DriverInformation::configOptions
// Returns if the driver should use a drive connected as Drive B (true) on the cable rather than Drive A (false)
bool FloppyBridgeAPI::getDriveCableSelection(bool* connectToDriveB) {
	return BRIDGE_DriverGetCable(m_handle, connectToDriveB);
}
// Sets if the driver should use a drive connected as Drive B (true) on the cable rather than Drive A (false)
bool FloppyBridgeAPI::setDriveCableSelection(bool connectToDriveB) {
	return BRIDGE_DriverSetCable(m_handle, connectToDriveB);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These require ConfigOption_SmartSpeed bit set in DriverInformation::configOptions
// Returns if the driver currently has Smart Speed enabled which can dynamically switch between normal and turbo disk speed without breaking copy protection
bool FloppyBridgeAPI::getSmartSpeedEnabled(bool* enabled) {
	return BRIDGE_DriverGetSmartSpeedEnabled(m_handle, enabled);
}
//  Sets if the driver can dynamically switch between normal and turbo disk speed without breaking copy protectionThis can be set while the bridge is in use
bool FloppyBridgeAPI::setSmartSpeedEnabled(bool enabled) {
	return BRIDGE_DriverSetSmartSpeedEnabled(m_handle, enabled);
}


/******************* BRIDGE Functions for UAE **********************************/

bool FloppyBridgeAPI::initialise() {
	if (m_isOpen) shutdown();

	memset(m_error, 0, sizeof(m_error));
	memset(m_warning, 0, sizeof(m_warning));

	char* msg;
	m_isOpen = BRIDGE_Open(m_handle, &msg);

	if (m_isOpen) {
		if (msg) _char2TChar(msg, m_warning, BRIDGE_STRING_MAX_LENGTH-1);
		return true;
	}
	else {
		if (msg) _char2TChar(msg, m_error, BRIDGE_STRING_MAX_LENGTH - 1);
		return false;
	}
}
void FloppyBridgeAPI::shutdown() {
	if (m_isOpen) {
		BRIDGE_Close(m_handle);
		m_isOpen = false;
	}
	FloppyDiskBridge::shutdown();
}


//virtual const BridgeDriver* getDriverInfo() override;
unsigned char FloppyBridgeAPI::getBitSpeed() {
	return DRIVER_getBitSpeed(m_handle);
}
FloppyDiskBridge::DriveTypeID FloppyBridgeAPI::getDriveTypeID() {
	return DRIVER_getDriveTypeID(m_handle);
}
const char* FloppyBridgeAPI::getLastErrorMessage() {
#ifdef _UNICODE
	_quickw2a(m_error, m_lastErrorAnsi);
	return m_lastErrorAnsi.c_str();
#else
	return m_error;
#endif
}
const FloppyDiskBridge::BridgeDriver* FloppyBridgeAPI::getDriverInfo() {
	if (BRIDGE_GetDriverInfo(m_driverIndex, &m_driverInfo)) return m_driverInfo;
	return nullptr;
}
bool FloppyBridgeAPI::resetDrive(int trackNumber) {
	return DRIVER_resetDrive(m_handle, trackNumber);
}
bool FloppyBridgeAPI::isAtCylinder0() {
	return DRIVER_isAtCylinder0(m_handle);
}
unsigned char FloppyBridgeAPI::getMaxCylinder() {
	return DRIVER_getMaxCylinder(m_handle);
}
void FloppyBridgeAPI::gotoCylinder(int cylinderNumber, bool side) {
	DRIVER_gotoCylinder(m_handle, cylinderNumber, side);
}
void FloppyBridgeAPI::handleNoClickStep(bool side) {
	DRIVER_handleNoClickStep(m_handle, side);
}
unsigned char FloppyBridgeAPI::getCurrentCylinderNumber() {
	return DRIVER_getCurrentCylinderNumber(m_handle);
}
bool FloppyBridgeAPI::isMotorRunning() {
	return DRIVER_isMotorRunning(m_handle);
}
void FloppyBridgeAPI::setMotorStatus(bool side, bool turnOn) {
	DRIVER_setMotorStatus(m_handle, side, turnOn);
}
bool FloppyBridgeAPI::getCurrentSide() {
	return DRIVER_getCurrentSide(m_handle);
}
bool FloppyBridgeAPI::isReady() {
	return DRIVER_isReady(m_handle);
}
bool FloppyBridgeAPI::isDiskInDrive() {
	return DRIVER_isDiskInDrive(m_handle);
}
bool FloppyBridgeAPI::hasDiskChanged() {
	return DRIVER_hasDiskChanged(m_handle);
}
bool FloppyBridgeAPI::isMFMPositionAtIndex(int mfmPositionBits) {
	return DRIVER_isMFMPositionAtIndex(m_handle, mfmPositionBits);
}
bool FloppyBridgeAPI::isMFMDataAvailable() {
	return DRIVER_isMFMDataAvailable(m_handle);
}
bool FloppyBridgeAPI::getMFMBit(const int mfmPositionBits) {
	return DRIVER_getMFMBit(m_handle, mfmPositionBits);
}
int FloppyBridgeAPI::getMFMSpeed(const int mfmPositionBits) {
	return DRIVER_getMFMSpeed(m_handle, mfmPositionBits);
}
void FloppyBridgeAPI::mfmSwitchBuffer(bool side) {
	DRIVER_mfmSwitchBuffer(m_handle, side);
}
void FloppyBridgeAPI::setSurface(bool side) {
	DRIVER_setSurface(m_handle, side);
}
int FloppyBridgeAPI::maxMFMBitPosition() {
	return DRIVER_maxMFMBitPosition(m_handle);
}
void FloppyBridgeAPI::writeShortToBuffer(bool side, unsigned int track, unsigned short mfmData, int mfmPosition) {
	DRIVER_writeShortToBuffer(m_handle, side, track, mfmData, mfmPosition);
}
bool FloppyBridgeAPI::isWriteProtected() {
	return DRIVER_isWriteProtected(m_handle);
}
unsigned int FloppyBridgeAPI::commitWriteBuffer(bool side, unsigned int track) {
	return DRIVER_commitWriteBuffer(m_handle, side, track);
}
bool FloppyBridgeAPI::isWritePending() {
	return DRIVER_isWritePending(m_handle);
}
bool FloppyBridgeAPI::isWriteComplete() {
	return DRIVER_isWriteComplete(m_handle);
}
bool FloppyBridgeAPI::canTurboWrite() {
	return DRIVER_canTurboWrite(m_handle);
}
bool FloppyBridgeAPI::isReadyToWrite() {
	return DRIVER_isReadyToWrite(m_handle);
}