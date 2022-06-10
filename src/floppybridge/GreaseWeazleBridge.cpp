/* WinUAE Greaseweazle Interface for *UAE
*
* By Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/
 
#include "floppybridge_config.h"

#ifdef ROMTYPE_GREASEWEAZLEREADER_WRITER

#include "floppybridge_abstract.h"
#include "CommonBridgeTemplate.h"
#include "GreaseWeazleBridge.h"
#include "GreaseWeazleInterface.h"


using namespace GreaseWeazle;


static const FloppyDiskBridge::BridgeDriver DriverGreaseweazleFloppy = {
	"Greaseweazle", "https://github.com/keirf/Greaseweazle", "Keir Fraser", "RobSmithDev", CONFIG_OPTIONS_COMPORT | CONFIG_OPTIONS_COMPORT_AUTODETECT | CONFIG_OPTIONS_DRIVE_AB | CONFIG_OPTIONS_DRIVE_123 | CONFIG_OPTIONS_SMARTSPEED | CONFIG_OPTIONS_AUTOCACHE
};

// Flags from WINUAE
GreaseWeazleDiskBridge::GreaseWeazleDiskBridge(BridgeMode bridgeMode, BridgeDensityMode bridgeDensity, bool enableAutoCache, bool useSmartSpeed, bool autoDetectComPort, char* comPort, CommonBridgeTemplate::DriveSelection drive) :
	CommonBridgeTemplate(bridgeMode, bridgeDensity, enableAutoCache, useSmartSpeed), m_useDrive(drive), m_comPort(autoDetectComPort ? "" : comPort) {
}

// This is for the static version
GreaseWeazleDiskBridge::GreaseWeazleDiskBridge(BridgeMode bridgeMode, BridgeDensityMode bridgeDensity, int uaeSettings) :
	CommonBridgeTemplate(bridgeMode, bridgeDensity, false, false), m_useDrive((DriveSelection)((uaeSettings & 0x0F) == 0)), m_comPort("") {
}


GreaseWeazleDiskBridge::~GreaseWeazleDiskBridge() {
}

// If your device supports being able to abort a disk read, mid-read then implement this
void GreaseWeazleDiskBridge::abortDiskReading() {
	m_io.abortReadStreaming();
};

// A manual way to detect a disk change event
bool GreaseWeazleDiskBridge::attemptToDetectDiskChange() {
	switch (m_io.checkForDisk(true)) {
	case GWResponse::drOK: return true;
	case GWResponse::drNoDiskInDrive: return false;
	default: return isDiskInDrive();
	}
}

// If your device supports the DiskChange option then return TRUE here.  If not, then the code will simulate it
bool GreaseWeazleDiskBridge::supportsDiskChange() {
	return m_io.supportsDiskChange();
}

// Called when the class is about to shut down
void GreaseWeazleDiskBridge::closeInterface() {
	// Turn everything off
	m_io.enableMotor(false);
	m_io.closePort();
}

// Called to start the interface, you should update any error messages if it fails.  This needs to be ready to see to any cylinder and so should already know where cylinder 0 is
bool GreaseWeazleDiskBridge::openInterface(std::string& errorMessage) {

	GWResponse error = m_io.openPort(m_comPort, (GreaseWeazle::DriveSelection)m_useDrive);

	if (error == GWResponse::drOK) {
		if (m_io.findTrack0() == GWResponse::drRewindFailure) {
			errorMessage = "Failed to find track 0 (usually when Drive A and B are the wrong way around)";
			m_io.closePort();
			return false;
		}
		m_currentCylinder = 0;

		if (!m_io.supportsDiskChange()) {
#ifdef _WIN32			
			DWORD hasBeenSeen = 0;
			DWORD dataSize = sizeof(hasBeenSeen);

			HKEY key = 0;
			if (SUCCEEDED(RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\RobSmithDev\\GreaseWeazleSupport", 0, NULL, 0, KEY_READ | KEY_WRITE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY, NULL, &key, NULL)))
				if (!RegQueryValueEx(key, L"WarningShown", NULL, NULL, (LPBYTE)&hasBeenSeen, &dataSize)) dataSize = 0;

			if (!hasBeenSeen) {
				hasBeenSeen = 1;
				if (key) RegSetValueEx(key, L"GreaseWeazleSupport", NULL, REG_DWORD, (LPBYTE)&hasBeenSeen, sizeof(hasBeenSeen));
				errorMessage = "The Greaseweazle board you are using does not support access to the DISK CHANGE pin.\nThis is not optimal and will result in a higher amount of disk access.\nIf you are sure that it does, please update its firmware.";
			}
			if (key) RegCloseKey(key);
#endif			
		}

		return true;
	}
	else {
		switch (error) {
		case GWResponse::drPortNotFound: errorMessage = "Greaseweazle board was not detected."; break;
		case GWResponse::drPortInUse: errorMessage = "Greaseweazle board is already in use."; break;
		case GWResponse::drPortError: errorMessage = "Unknown error connecting to your Greaseweazle board."; break;
		case GWResponse::drComportConfigError: errorMessage = "Error configuring communication with your Greaseweazle board."; break;
		case GWResponse::drErrorMalformedVersion: errorMessage = "Error communicating with your Greaseweazle board. Please unplug it and re-connect it."; break;
		case GWResponse::drOldFirmware: errorMessage = "Your Greaseweazle firmware is too old. V0.27 or newer is required."; break;
		case GWResponse::drInUpdateMode: errorMessage = "Your Greaseweazle is currently in update mode.  Please restore it to normal mode."; break;
		case GWResponse::drError: errorMessage = "Unable to select the drive on your Greaseweazle."; break;
		default: errorMessage = "An unknown error occurred connecting to your Greaseweazle."; break;
		}
	}

	return false;
}

const FloppyDiskBridge::BridgeDriver* GreaseWeazleDiskBridge::staticBridgeInformation() {
	return &DriverGreaseweazleFloppy;
}

// Get the name of the drive
const FloppyDiskBridge::BridgeDriver* GreaseWeazleDiskBridge::_getDriverInfo() {
	return staticBridgeInformation();
}

// Duplicate of the one below, but here for consistency - Returns the name of interface.  This pointer should remain valid after the class is destroyed
const FloppyDiskBridge::DriveTypeID GreaseWeazleDiskBridge::_getDriveTypeID() {
	return m_isHDDisk ? DriveTypeID::dti35HD : DriveTypeID::dti35DD;
}

// Called when a disk is inserted so that you can (re)populate the response to _getDriveTypeID()
void GreaseWeazleDiskBridge::checkDiskType() {
	bool capacity;

	if (m_io.checkDiskCapacity(capacity) == GWResponse::drOK) {
		m_isHDDisk = capacity;
		m_io.setDiskCapacity(m_isHDDisk);
	}
	else {
		m_isHDDisk = false;
		m_io.setDiskCapacity(m_isHDDisk);
	}
}

// Called to force into DD or HD mode.  Overrides checkDiskType() until checkDiskType() is called again
void GreaseWeazleDiskBridge::forceDiskDensity(bool forceHD) {
	m_isHDDisk = forceHD;
	m_io.setDiskCapacity(m_isHDDisk);
}


// Called to switch which head is being used right now.  Returns success or not
bool GreaseWeazleDiskBridge::setActiveSurface(const DiskSurface activeSurface) {
	return m_io.selectSurface(activeSurface == DiskSurface::dsUpper ? GreaseWeazle::DiskSurface::dsUpper : GreaseWeazle::DiskSurface::dsLower) ==
		GWResponse::drOK;
}

// Set the status of the motor on the drive. The motor should maintain this status until switched off or reset.  This should *NOT* wait for the motor to spin up
bool GreaseWeazleDiskBridge::setMotorStatus(const bool switchedOn) {
	m_motorIsEnabled = switchedOn;
	m_motorTurnOnTime = std::chrono::steady_clock::now();
	return m_io.enableMotor(switchedOn, true) == GWResponse::drOK;
}

// Called to ask the drive what the current write protect status is - return true if its write protected
bool GreaseWeazleDiskBridge::checkWriteProtectStatus(const bool forceCheck) {
	return m_io.isWriteProtected();
}

// If the above is TRUE then this is called to get the status of the DiskChange line.  Basically, this is TRUE if there is a disk in the drive.
// If force is true you should re-check, if false, then you are allowed to return a cached value from the last disk operation (eg: seek)
bool GreaseWeazleDiskBridge::getDiskChangeStatus(const bool forceCheck) {
	// We actually trigger a SEEK operation to ensure this is right
	if (forceCheck) {
		if (m_io.checkForDisk(forceCheck) == GWResponse::drNoDiskInDrive) {
			if ((m_currentCylinder == 0) && (m_io.supportsDiskChange())) {
				m_io.performNoClickSeek();
			}
			else {
				m_io.selectTrack((m_currentCylinder > 40) ? m_currentCylinder - 1 : m_currentCylinder + 1, TrackSearchSpeed::tssNormal, true);
				m_io.selectTrack(m_currentCylinder, TrackSearchSpeed::tssNormal, true);
			}
		}
	}

	switch (m_io.checkForDisk(forceCheck)) {
	case GWResponse::drOK: return true;
	case GWResponse::drNoDiskInDrive: return false;
	default: return isDiskInDrive();
	}
}

// If we're on track 0, this is the emulator trying to seek to track -1.  We catch this as a special case.  
// Should perform the same operations as setCurrentCylinder in terms of disk change etc but without changing the current cylinder
// Return FALSE if this is not supported by the bridge
bool GreaseWeazleDiskBridge::performNoClickSeek() {
	// Claim we did it anyway
	if (!m_io.supportsDiskChange()) return true;

	if (m_io.performNoClickSeek() == GWResponse::drOK) {
		updateLastManualCheckTime();
		return true;
	}
	return false;
}


// Trigger a seek to the requested cylinder, this can block until complete
bool GreaseWeazleDiskBridge::setCurrentCylinder(const unsigned int cylinder) {
	m_currentCylinder = cylinder;

	// No need if its busy
	bool ignoreDiskCheck = (isMotorRunning()) && (!isReady());

	// If we don't support disk change then don't allow the hardware to check for a disk as it takes time, unless it's due anyway
	if (!supportsDiskChange()) ignoreDiskCheck |= !isReadyForManualDiskCheck();

	// Go!
	if (m_io.selectTrack(cylinder, TrackSearchSpeed::tssNormal, ignoreDiskCheck) == GWResponse::drOK) {
		if (!ignoreDiskCheck) updateLastManualCheckTime();	
		return true;
	}

	return false;
}

// Called when data should be read from the drive.
//		rotationExtractor: supplied if you use it
//		maxBufferSize: Maximum number of RotationExtractor::MFMSample in the buffer.  If we're trying to detect a disk, this might be set VERY LOW
// 	    buffer:		   Where to save to.  When a buffer is saved, position 0 MUST be where the INDEX pulse is.  RevolutionExtractor will do this for you
//		indexMarker:   Used by rotationExtractor if you use it, to help be consistent where the INDEX position is read back at
//		onRotation: A function you should call for each complete revolution received.  If the function returns FALSE then you should abort reading, else keep sending revolutions
// Returns: ReadResponse, explains its self
CommonBridgeTemplate::ReadResponse GreaseWeazleDiskBridge::readData(PLL::BridgePLL& pll, const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
	std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation) {
	GWResponse result = m_io.readRotation(pll, maxBufferSize, buffer, indexMarker,
	                                      [&onRotation](RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits) -> bool {
		                                      return onRotation(*mfmData, dataLengthInBits);
	                                      });
	m_motorTurnOnTime = std::chrono::steady_clock::now();

	switch (result) {
	case GWResponse::drOK: return ReadResponse::rrOK;
	case GWResponse::drNoDiskInDrive: return ReadResponse::rrNoDiskInDrive;
	default:  return ReadResponse::rrError;
	}
	
}

// Called when a cylinder revolution should be written to the disk.
// Parameters are:	rawMFMData						The raw data to be written.  This is an actual MFM stream, going from MSB to LSB for each byte
//					numBytes						Number of bits in the buffer to write
//					writeFromIndex					If an attempt should be made to write this from the INDEX pulse rather than just a random position
//					suggestUsingPrecompensation		A suggestion that you might want to use write pre-compensation, optional
// Returns TRUE if success, or false if it fails.  Largely doesn't matter as most stuff should verify with a read straight after
bool GreaseWeazleDiskBridge::writeData(const unsigned char* rawMFMData, const unsigned int numBits, const bool writeFromIndex, const bool suggestUsingPrecompensation) {
	GWResponse response = m_io.writeCurrentTrackPrecomp(rawMFMData, (numBits + 7) / 8, writeFromIndex, suggestUsingPrecompensation);
	m_motorTurnOnTime = std::chrono::steady_clock::now();

	switch (response) {
	case GWResponse::drOK: return true;
	case GWResponse::drWriteProtected: setWriteProtectStatus(true); return false;
	default:  return false;
	}
}

// This is called by the main thread incase you need to do anything specific at regular intervals
void GreaseWeazleDiskBridge::poll() {
	// GW has a watchdog timeout and after a while the motor will switch off after inactivity.  We can't allow this as doesn't work for how the Amiga might do things
	if (m_motorIsEnabled) {
		const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_motorTurnOnTime).count();
		if (timePassed > (unsigned int)(m_io.getMotorTimeout()/2)) {
			m_io.enableMotor(true, true);
			m_motorTurnOnTime = std::chrono::steady_clock::now();
		}
	}
}


#endif