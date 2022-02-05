/* FloppyBridge DLL for *UAE
*
* Copyright (C) 2021 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, see http://www.gnu.org/licenses/
*/

/*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/ 

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FLOPPYBRIDGE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FLOPPYBRIDGE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

#define CALLING_CONVENSION _cdecl
#define FLOPPYBRIDGE_API __declspec(dllexport) 

#else

#define CALLING_CONVENSION
#define FLOPPYBRIDGE_API 

#endif 

#include "CommonBridgeTemplate.h"

#define MAX_NUM_DRIVERS     2

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

	// Driver Index, in case it's shown on the GUI
	unsigned int driverIndex;

	// Some basic information
	CommonBridgeTemplate::BridgeMode bridgeMode;
	CommonBridgeTemplate::BridgeDensityMode bridgeDensityMode;

	// Profile name
	char* name;

	// Pointer to the Configuration data for this profile. - Be careful. Assume this pointer is invalid after calling *any* of the *profile* functions apart from getAllProfiles
	char* profileConfig;
};

// Config for a bridge setup
class BridgeConfig {
private:
	// Last serialisation of the below data, made when you call toString()
	char lastSerialise[255] = { 0 };
public:
	// Which type of interface to open
	unsigned int bridgeIndex = 0;

	// The settings that will be used to open the above bridge
	CommonBridgeTemplate::BridgeMode bridgeMode = CommonBridgeTemplate::BridgeMode::bmFast;
	CommonBridgeTemplate::BridgeDensityMode bridgeDensity = CommonBridgeTemplate::BridgeDensityMode::bdmAuto;

	// This isn't saved with fromString and toString
	char profileName[128] = { 0 };

	// Not all of these settings are used.
	char comPortToUse[128] = { 0 };
	bool autoDetectComPort = true;
	bool driveCableIsB = false;
	bool autoCache = false;
	bool smartSpeed = false;

	// Serialising into a nice string
	bool fromString(char* string);
	void toString(char** serialisedOptions);  //the pointer returned is actually lastSerialise
};


// This is private, from the outside of the DLL its just a pointer to this
struct BridgeOpened {
	/// Details about the driver
	FloppyDiskBridge::BridgeDriver* driverDetails = nullptr;

	// The bridge, when created.
	CommonBridgeTemplate* bridge = nullptr;

	// Last error/warning message received
	char lastMessage[255] = { 0 };

	// Currently active config
	BridgeConfig config;
};





extern "C" {
	FLOPPYBRIDGE_API void CALLING_CONVENSION BRIDGE_About(bool checkForUpdates, BridgeAbout** output);
	FLOPPYBRIDGE_API unsigned int CALLING_CONVENSION BRIDGE_NumDrivers(void);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_EnumComports(char* output, unsigned int* outputLength);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetConfigString(BridgeOpened* bridgeDriverHandle, char** serialisedOptions);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetConfigFromString(BridgeOpened* bridgeDriverHandle, char* serialisedOptions);
#ifdef _WIN32
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_ShowConfigDialog(HWND hwndParent, unsigned int* profileID);
#endif
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetAllProfiles(FloppyBridgeProfileInformationDLL** profiles, unsigned int* numProfiles);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_ImportProfilesFromString(char* profilesConfigString);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_ExportProfilesToString(char** profilesConfigString);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetProfileConfigFromString(unsigned int profileID, char** profilesConfigString);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetProfileConfigFromString(unsigned int profileID, char* profilesConfigString);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_SetProfileName(unsigned int profileID, char* profileName);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateNewProfile(unsigned int driverIndex, unsigned int* profileID);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DeleteProfile(unsigned int profileID);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetDriverInfo(unsigned int driverIndex, FloppyDiskBridge::BridgeDriver** driverInformation);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_Close(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateDriver(unsigned int driverIndex, BridgeOpened** bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateDriverFromConfigString(char* configString, BridgeOpened** bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_CreateDriverFromProfileID(unsigned int profileID, BridgeOpened** bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_Open(BridgeOpened* bridgeDriverHandle, char** errorMessage);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_GetDriverIndex(BridgeOpened* bridgeDriverHandle, unsigned int* driverIndex);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_FreeDriver(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetMode(BridgeOpened* bridgeDriverHandle, CommonBridgeTemplate::BridgeMode* bridgeMode);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetMode(BridgeOpened* bridgeDriverHandle, CommonBridgeTemplate::BridgeMode bridgeMode);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetDensityMode(BridgeOpened* bridgeDriverHandle, CommonBridgeTemplate::BridgeDensityMode* densityMode);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetDensityMode(BridgeOpened* bridgeDriverHandle, CommonBridgeTemplate::BridgeDensityMode densityMode);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetSmartSpeedEnabled(BridgeOpened* bridgeDriverHandle, bool* enabled);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetSmartSpeedEnabled(BridgeOpened* bridgeDriverHandle, bool enabled);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetAutoCache(BridgeOpened* bridgeDriverHandle, bool* isAutoCache);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetAutoCache(BridgeOpened* bridgeDriverHandle, bool isAutoCache);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetCurrentComPort(BridgeOpened* bridgeDriverHandle, char** comPort);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetCurrentComPort(BridgeOpened* bridgeDriverHandle, char* comPort);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetAutoDetectComPort(BridgeOpened* bridgeDriverHandle, bool* autoDetectComPort);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetAutoDetectComPort(BridgeOpened* bridgeDriverHandle, bool autoDetectComPort);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverGetCable(BridgeOpened* bridgeDriverHandle, bool* isOnB);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION BRIDGE_DriverSetCable(BridgeOpened* bridgeDriverHandle, bool isOnB);

	FLOPPYBRIDGE_API unsigned char CALLING_CONVENSION DRIVER_getBitSpeed(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API FloppyDiskBridge::DriveTypeID CALLING_CONVENSION DRIVER_getDriveTypeID(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_resetDrive(BridgeOpened* bridgeDriverHandle, int trackNumber);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isAtCylinder0(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API unsigned char CALLING_CONVENSION DRIVER_getMaxCylinder(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_gotoCylinder(BridgeOpened* bridgeDriverHandle, int cylinderNumber, bool side);
	FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_handleNoClickStep(BridgeOpened* bridgeDriverHandle, bool side);
	FLOPPYBRIDGE_API unsigned char CALLING_CONVENSION DRIVER_getCurrentCylinderNumber(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_getCurrentSide(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_setMotorStatus(BridgeOpened* bridgeDriverHandle, bool side, bool turnOn);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isMotorRunning(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isReady(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isDiskInDrive(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_hasDiskChanged(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isMFMPositionAtIndex(BridgeOpened* bridgeDriverHandle, int mfmPositionBits);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isMFMDataAvailable(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_getMFMBit(BridgeOpened* bridgeDriverHandle, int mfmPositionBits);
	FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_getMFMSpeed(BridgeOpened* bridgeDriverHandle, int mfmPositionBits);
	FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_mfmSwitchBuffer(BridgeOpened* bridgeDriverHandle, bool side);
	FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_setSurface(BridgeOpened* bridgeDriverHandle, bool side);
	FLOPPYBRIDGE_API int CALLING_CONVENSION DRIVER_maxMFMBitPosition(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API void CALLING_CONVENSION DRIVER_writeShortToBuffer(BridgeOpened* bridgeDriverHandle, bool side, unsigned int track, unsigned short mfmData, int mfmPosition);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isWriteProtected(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API unsigned int CALLING_CONVENSION DRIVER_commitWriteBuffer(BridgeOpened* bridgeDriverHandle, bool side, unsigned int track);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isWritePending(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isWriteComplete(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_canTurboWrite(BridgeOpened* bridgeDriverHandle);
	FLOPPYBRIDGE_API bool CALLING_CONVENSION DRIVER_isReadyToWrite(BridgeOpened* bridgeDriverHandle);
}
