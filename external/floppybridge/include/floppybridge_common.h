/* floppybridge_common
*
* Copyright (C) 2021-2024 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* Shared class between the dll and class to connect to it.
*
* This is free and unencumbered released into the public domain
* But please don't remove the above information
*
* For more details visit <http://unlicense.org>.
*
*/
#pragma once

namespace FloppyBridge {

	// Type of bridge mode
	enum class BridgeMode : unsigned char {
		bmFast = 0,
		bmCompatible = 1,
		bmTurboAmigaDOS = 2,
		bmStalling = 3,
		bmMax = 3
	};

	// How to use disk density
	enum class BridgeDensityMode : unsigned char {
		bdmAuto = 0,
		bdmDDOnly = 1,
		bdmHDOnly = 2,
		bdmMax = 2
	};

	// Used to select the drive connected
	enum class DriveSelection : unsigned char {
		dsDriveA = 0,     // IBM PC
		dsDriveB = 1,
		dsDrive0 = 2,     // SHUGART
		dsDrive1 = 3,
		dsDrive2 = 4,
		dsDrive3 = 5
	};


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
		BridgeMode bridgeMode;
		BridgeDensityMode bridgeDensityMode;

		// Profile name
		char* name;

		// Pointer to the Configuration data for this profile. - Be careful. Assume this pointer is invalid after calling *any* of the *profile* functions apart from getAllProfiles
		char* profileConfig;
	};



};