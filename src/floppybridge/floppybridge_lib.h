/* floppybridge_lib
*
* Copyright (C) 2021-2023 Robert Smith (@RobSmithDev)
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

#pragma once
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "floppybridge_abstract.h"
#include "floppybridge_common.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <vector>
#include <string>


#define BRIDGE_STRING_MAX_LENGTH 255
typedef TCHAR TCharString[BRIDGE_STRING_MAX_LENGTH];

typedef void* BridgeDriverHandle;

// Class to access the 'floppybridge' via a DLL but using the same interface
class FloppyBridgeAPI : public FloppyDiskBridge {
public:

	// How to use disk density
	enum class BridgeDensityMode : unsigned char {
		bdmAuto = 0,						// Auto-detect the type of disk when its inserted
		bdmDDOnly = 1,						// Always detect all inserted disks as DD
		bdmHDOnly = 2,						// Always detect all inserted disks as HD
		bdmMax = 2
	};

	// Information about the bridge driver DLL that was loaded.
	struct BridgeInformation {
		// Information about the bridge
		TCharString about;
		// A url where you can get support/download updates from
		TCharString url;
		// The current version
		unsigned int majorVersion, minorVersion;
		// Is this version a beta build?
		bool isBeta;
		// Is there an update to this version available?
		bool isUpdateAvailable;
		// The version number of the update/latest version available
		unsigned int updateMajorVersion, updateMinorVersion;
	};

	// These bitmasks are used with DriverInformation::configOptions
	static const unsigned int ConfigOption_AutoCache			= 0x01;	// The driver can cache data from other cylinders while the disk isn't in use
	static const unsigned int ConfigOption_ComPort				= 0x02;	// The driver requires a COM port selection
	static const unsigned int ConfigOption_AutoDetectComport	= 0x04;	// The driver supports automatic com port detection and selection
	static const unsigned int ConfigOption_DriveABCable			= 0x08;	// The driver allows you to specify using cable select for Drive A or Drive B
	static const unsigned int ConfigOption_SmartSpeed			= 0x10;	// The driver supports dynamically switching between normal and Turbo hopefully without breaking copy protection
	static const unsigned int ConfigOption_SupportsShugartMode  = 0x20; // The driver supports Shugart modes as well as IBM PC modes

	// Information about a Bridge Driver (eg: DrawBridge, Greaseweazle etc)
	struct DriverInformation {
		// Used to identify the type of driver.  Use this to create the device or multiple instances of the device (possibly on different COM ports)
		unsigned int driverIndex;

		// Details about the driver
		TCharString name;
		TCharString url;
		TCharString manufacturer;
		TCharString driverAuthor;

		// A bitmask of which options in configuration the driver can support, aside from the standard ones. See the ConfigOption_ consts above
		unsigned int configOptions;
	};

	// Information about a floppy bridge profile
	struct FloppyBridgeProfileInformation {
		// Unique ID of this profile
		unsigned int profileID;

		// Driver Index, in case it's shown on the GUI
		unsigned int driverIndex;

		// Some basic information
		FloppyBridge::BridgeMode bridgeMode;
		FloppyBridge::BridgeDensityMode bridgeDensityMode;

		// Profile name
		TCharString name;

		// Pointer to the Configuration data for this profile. - Be careful. Assume this pointer is invalid after calling *any* of the *profile* functions apart from getAllProfiles
		const char* profileConfig;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Static functions for accessing parts of the API before creating an instance of this 
	// Route is: choose driver, create driver, configure it, then set the "bridge" variable in UAE to the created instance and it will do the rest

	// Returns TRUE if the floppy bridge library has been loaded and is ready to be queried.  All functions are safe to call regardless the return value
	static bool isAvailable();

	// Populates bridgeInformation with information about the Bridge DLL. This should be called and shown somewhere
	// As it contains update and support information too.  If this returns FALSE it will still contain basic information such as a URL to get the DLL from.
	static bool getBridgeDriverInformation(bool allowCheckForUpdates, BridgeInformation& bridgeInformation);

	// Creates a driver instance.  If it fails, it will return NULL.  It should only fail if the index is invalid.
	static FloppyBridgeAPI* createDriver(unsigned int driverIndex);

	// Create a driver instance from a config string previously saved.  This will auto-select the driverIndex.
	static FloppyBridgeAPI* createDriverFromString(const char* config);

	// Creates the driver instance from a profile ID.  You need to call importProfilesFromString() before using this function
	static FloppyBridgeAPI* createDriverFromProfileID(unsigned int profileID);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Direct management

	// Populates driverList with a list of available floppy bridge drivers that could be created
	static void getDriverList(std::vector<DriverInformation>& driverList);

	// Populates portList with a list of serial port devices that you can use with setComPort() below
	// NOTE: The TCHARs in the vector are only valid until this function is called again
	static void enumCOMPorts(std::vector<const TCHAR*>& portList);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Profile based management

	// Displays the config dialog (modal) for Floppy Bridge profiles.  
	// *If* you pass a profile ID, the dialog will jump to editing that profile, or return FALSE if it was not found.
	// Returns FALSE if cancel was pressed
#ifdef _WIN32
	static bool showProfileConfigDialog(HWND hwndParent, unsigned int* profileID = nullptr);
#endif

	// Retrieve a list of all of the profiles currently loaded that can be used.
	static bool getAllProfiles(std::vector<FloppyBridgeProfileInformation>& profileList);

	// Imports all profiles into memory from the supplied string.  This will erase any currently in memory
	static bool importProfilesFromString(const char* profilesString);

	// Exports all profiles and returns a pointer to the string.  This pointer is only valid while the driver is loaded and until this is called again
	static bool exportProfilesToString(char** profilesString);

	// Returns a pointer to a string containing the details for a profile
	static bool getProfileConfigAsString(unsigned int profileID, char** config);

	// Updates a profile from the supplied string
	static bool setProfileConfigFromString(unsigned int profileID, const char* config);

	// Updates a profile name the supplied string
	static bool setProfileName(unsigned int profileID, const char* name);

	// Creates a new profile and returns its unique ID
	static bool createNewProfile(unsigned int driverIndex, unsigned int* profileID);

	// Deletes a profile by ID.
	static bool deleteProfile(unsigned int profileID);

#ifdef _WIN32
	// By default FloppyBridge will inform DiskFlashback when it wants the drive. Use this to turn that feature off
	static void enableUsageNotifications(bool enable);
#endif

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Functions for configuring the driver once it has been created.	
	// Returns a pointer to a string containing the current config.  This can be used with setConfigFromString() or createDriverFromString()
	bool getConfigAsString(char** config) const;
	// Applies the config to the currently driver.  Returns TRUE if successful.
	bool setConfigFromString(const char* config) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This for setting and getting the driver index in use. Shouldnt normally be changed while in use
	bool getDriverIndex(int& driverIndex) const;
	// Set it
	bool setDriverIndex(const int driverIndex);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Return the current bridge mode selected
	bool getBridgeMode(FloppyBridge::BridgeMode* mode) const;
	// Set the currently active bridge mode.  This can be set while the bridge is in use
	bool setBridgeMode(const FloppyBridge::BridgeMode newMode) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Return the current bridge density mode selected
	bool getBridgeDensityMode(FloppyBridge::BridgeDensityMode* mode) const;
	// Set the currently active bridge density mode.  This can be set while the bridge is in use
	bool setBridgeDensityMode(const FloppyBridge::BridgeDensityMode newMode) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// These require ConfigOption_AutoCache bit set in DriverInformation::configOptions
	// Returns if auto-disk caching (while the drive is idle) mode is enabled
	bool getAutoCacheMode(bool* autoCacheMode) const;
	// Sets if auto-disk caching (while the drive is idle) mode is enabled.  This can be set while the bridge is in use
	bool setAutoCacheMode(const bool autoCacheMode) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// These require ConfigOption_ComPort bit set in DriverInformation::configOptions
	// Returns the currently selected COM port.  This port is only used if auto detect com port is false
	bool getComPort(TCharString* comPort) const;
	// Sets the com port to use.  This port is only used if auto detect com port is false.
	bool setComPort(const TCHAR* comPort) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// These require ConfigOption_AutoDetectComport bit set in DriverInformation::configOptions
	// Returns if com port auto-detect is enabled
	bool getComPortAutoDetect(bool* autoDetect) const;
	// Sets if auto-detect com port should be used
	bool setComPortAutoDetect(const bool autoDetect) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// DEPRECATED: These require ConfigOption_DriveABCable bit set in DriverInformation::configOptions
	// Returns if the driver should use a drive connected as Drive B (true) on the cable rather than Drive A (false)
	bool getDriveCableSelection(bool* connectToDriveB) const;
	// Sets if the driver should use a drive connected as Drive B (true) on the cable rather than Drive A (false)
	bool setDriveCableSelection(const bool connectToDriveB) const;

	// New versions! = connectToDrive 
	bool getDriveCableSelection(FloppyBridge::DriveSelection* connectToDrive) const;
	// Sets if the driver should use a drive connected as Drive B (true) on the cable rather than Drive A (false)
	bool setDriveCableSelection(const FloppyBridge::DriveSelection connectToDrive) const;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// These require ConfigOption_SmartSpeed bit set in DriverInformation::configOptions
	// Returns if the driver currently has Smart Speed enabled which can dynamically switch between normal and turbo disk speed without breaking copy protection
	bool getSmartSpeedEnabled(bool* enabled) const;
	//  Sets if the driver can dynamically switch between normal and turbo disk speed without breaking copy protectionThis can be set while the bridge is in use
	bool setSmartSpeedEnabled(const bool enabled) const;



	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Functions required by UAE - see floppybridge_abstract for more details
	FloppyBridgeAPI(unsigned int driverIndex, BridgeDriverHandle handle); // Don't call this. You should use the static createDriver member to create it.
	virtual ~FloppyBridgeAPI();
	virtual bool initialise() override;
	virtual void shutdown() override;
	virtual const BridgeDriver* getDriverInfo() override;	
	virtual unsigned char getBitSpeed() override;
	virtual DriveTypeID getDriveTypeID() override;
	virtual const char* getLastErrorMessage() override;
	virtual bool resetDrive(int trackNumber) override;
	virtual bool isAtCylinder0() override;
	virtual unsigned char getMaxCylinder() override;
	virtual void gotoCylinder(int cylinderNumber, bool side) override;
	virtual void handleNoClickStep(bool side) override;
	virtual unsigned char getCurrentCylinderNumber() override;
	virtual bool isMotorRunning() override;
	virtual void setMotorStatus(bool side, bool turnOn) override;
	virtual bool isReady() override;
	virtual bool isDiskInDrive() override;
	virtual bool hasDiskChanged() override;
	virtual bool getCurrentSide() override;
	virtual bool isMFMPositionAtIndex(int mfmPositionBits) override;
	virtual bool isMFMDataAvailable() override;
	virtual bool getMFMBit(const int mfmPositionBits) override;
	virtual int getMFMSpeed(const int mfmPositionBits) override;
	virtual void mfmSwitchBuffer(bool side) override;
	virtual void setSurface(bool side) override;
	virtual int maxMFMBitPosition() override;
	virtual void writeShortToBuffer(bool side, unsigned int track, unsigned short mfmData, int mfmPosition)  override;
	virtual int getMFMTrack(bool side, unsigned int track, bool resyncRotation, const int bufferSizeInBytes, void* output) override;
	virtual bool setDirectMode(bool directModeEnable) override;
	virtual bool writeMFMTrackToBuffer(bool side, unsigned int track, bool writeFromIndex, int sizeInBytes, void* mfmData) override;
	virtual bool isWriteProtected() override;
	virtual unsigned int commitWriteBuffer(bool side, unsigned int track)  override;
	virtual bool isWritePending() override;
	virtual bool isWriteComplete() override;
	virtual bool canTurboWrite() override;
	virtual bool isReadyToWrite() override;
	const unsigned int getDriverTypeIndex() const;
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Private stuff
	BridgeDriverHandle m_handle;				// Handle to loaded driver
	unsigned int m_driverIndex;					// Index of that driver
	TCharString m_error = { 0 };				// Last error
	TCharString m_warning = { 0 };				// Last warning
	BridgeDriver* m_driverInfo{};					// Pointer to the driver info if retrieved
#ifdef _UNICODE
	std::string m_lastErrorAnsi;				// Non-unicode version of the last error
#endif
	bool m_isOpen = false;						// If the driver is 'open' (ie connected to the drive) or not
};