/* DrawBridge (Arduino Reader/Writer) Bridge for *UAE
*
* Copyright (C) 2021-2024 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This file is multi-licensed under the terms of the Mozilla Public
* License Version 2.0 as published by Mozilla Corporation and the
* GNU General Public License, version 2 or later, as published by the
* Free Software Foundation.
*
* MPL2: https://www.mozilla.org/en-US/MPL/2.0/
* GPL2: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/ 

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// This class provides an interface between WinUAE and the Arduino Floppy Reader/Writer aka DrawBridge //
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ROMTYPE_ARDUINOREADER_WRITER must be defined for this to work

#include "floppybridge_config.h"

#ifdef ROMTYPE_ARDUINOREADER_WRITER

#include "floppybridge_abstract.h"
#include "CommonBridgeTemplate.h"
#include "ArduinoFloppyBridge.h"
#include "ArduinoInterface.h"


static const FloppyDiskBridge::BridgeDriver DriverArduinoFloppy = {
	"DrawBridge aka Arduino Reader/Writer", "https://amiga.robsmithdev.co.uk", "RobSmithDev", "RobSmithDev", CONFIG_OPTIONS_COMPORT | CONFIG_OPTIONS_COMPORT_AUTODETECT | CONFIG_OPTIONS_SMARTSPEED | CONFIG_OPTIONS_AUTOCACHE
};

// Flags from WINUAE
ArduinoFloppyDiskBridge::ArduinoFloppyDiskBridge(FloppyBridge::BridgeMode bridgeMode, FloppyBridge::BridgeDensityMode bridgeDensity, bool enableAutoCache, bool useSmartSpeed, bool autoDetectComPort, char* comPort) :
	CommonBridgeTemplate(bridgeMode, bridgeDensity, enableAutoCache, useSmartSpeed), m_comPort(autoDetectComPort ? "" : comPort) {
}

// This is for the static version
ArduinoFloppyDiskBridge::ArduinoFloppyDiskBridge(FloppyBridge::BridgeMode bridgeMode, FloppyBridge::BridgeDensityMode bridgeDensity, int uaeSettings) :
	CommonBridgeTemplate(bridgeMode, bridgeDensity, false, false) {

	if (uaeSettings > 0) {
		char buffer[20];
#ifdef _WIN32
		sprintf_s(buffer, "COM%i", uaeSettings);
#else		
		snprintf(buffer, 20, "COM%i", uaeSettings);
#endif
		m_comPort = buffer;
	}
	else m_comPort = "";
}


ArduinoFloppyDiskBridge::~ArduinoFloppyDiskBridge() {
}

// If your device supports being able to abort a disk read, mid-read then implement this
void ArduinoFloppyDiskBridge::abortDiskReading() {
	m_io.abortReadStreaming();
};

// A manual way to detect a disk change event
bool ArduinoFloppyDiskBridge::attemptToDetectDiskChange() {
	return getDiskChangeStatus(true);
}

// If your device supports the DiskChange option then return TRUE here.  If not, then the code will simulate it
bool ArduinoFloppyDiskBridge::supportsDiskChange() {
	return m_io.getFirwareVersion().fullControlMod;
}

// Called when the class is about to shut down
void ArduinoFloppyDiskBridge::closeInterface() {
	// Turn everything off
	m_io.enableReading(false);
	m_io.closePort();
}

// Returns the COM port number to use
std::wstring ArduinoFloppyDiskBridge::getComPort() {
	// 0 means auto-detect.
	if (m_comPort.length()) {
		std::wstring widePort;
		quicka2w(m_comPort, widePort);
		return widePort;
	}

	// Returns a list of ports this could be available on
	std::vector<std::wstring> portsAvailable;
	ArduinoFloppyReader::ArduinoInterface::enumeratePorts(portsAvailable);

	for (const std::wstring& port : portsAvailable)
		if (ArduinoFloppyReader::ArduinoInterface::isPortCorrect(port)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return port;
		}
	
	return L"";
}

// Called to start the interface, you should update any error messages if it fails.  This needs to be ready to see to any cylinder and so should already know where cylinder 0 is
bool ArduinoFloppyDiskBridge::openInterface(std::string& errorMessage) {

	const std::wstring prt = getComPort();
	if (prt.empty()) {
		errorMessage = "The serial port could not be found or detected.  Please try re-connecting the interface.";
		return false;
	}

	ArduinoFloppyReader::DiagnosticResponse error = m_io.openPort(prt);
	if (error == ArduinoFloppyReader::DiagnosticResponse::drOK) {
		m_currentCylinder = 0;

		// See which version it is
		ArduinoFloppyReader::FirmwareVersion fv = m_io.getFirwareVersion();
		if ((fv.major <= 1) && (fv.minor < 8)) {
			// Must be at least V1.8
			char buf[20];
#ifdef _WIN32			
			sprintf_s(buf, "%i.%i.%i", fv.major, fv.minor, fv.buildNumber);
#else
			sprintf(buf, "%i.%i.%i", fv.major, fv.minor, fv.buildNumber);
#endif			
			errorMessage = "DrawBridge aka Arduino Floppy Reader/Writer Firmware is Out Of Date\n\nWinUAE requires V1.8 (and ideally with the modded circuit design).\n\n";
			errorMessage += "You are currently using V" + std::string(buf) + ".  Please update the firmware.";
		}
		else {
#ifdef _WIN32

			if (!fv.fullControlMod) {
				DWORD hasBeenSeen = 0;
				DWORD dataSize = sizeof(hasBeenSeen);
				
				HKEY key = 0;
				if (SUCCEEDED(RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\RobSmithDev\\ArduinoReaderWriter", 0, NULL, 0, KEY_READ | KEY_WRITE | KEY_SET_VALUE | KEY_CREATE_SUB_KEY, NULL, &key, NULL)))
					if (!RegQueryValueEx(key, L"WarningShown", NULL, NULL, (LPBYTE)&hasBeenSeen, &dataSize)) dataSize = 0;

				if (!hasBeenSeen) {
					hasBeenSeen = 1;
					if (key) RegSetValueEx(key, L"WarningShown", NULL, REG_DWORD, (LPBYTE)&hasBeenSeen, sizeof(hasBeenSeen));
					errorMessage = "DrawBridge aka Arduino Reader / Writer hasn't had the 'hardware mod' applied for optimal WinUAE Support.\nThis mod is highly recommended for best experience and will reduce disk access.";
				}
				if (key) RegCloseKey(key);
			}
#endif
			m_io.findTrack0();
			return true;
		}
	}
	else {
		errorMessage = m_io.getLastErrorStr();
	}

	return false;
}

// Called when a disk is inserted so that you can (re)populate the response to _getDriveTypeID()
void ArduinoFloppyDiskBridge::checkDiskType() {
	bool capacity;

	if (m_io.checkDiskCapacity(capacity) == ArduinoFloppyReader::DiagnosticResponse::drOK) {
		m_isHDDisk = capacity;
		m_io.setDiskCapacity(m_isHDDisk);
	}
	else {
		m_isHDDisk = false;
		m_io.setDiskCapacity(m_isHDDisk);
	}
}

// Called to force into DD or HD mode.  Overrides checkDiskType() until checkDiskType() is called again
void ArduinoFloppyDiskBridge::forceDiskDensity(bool forceHD) {
	m_isHDDisk = forceHD;
	m_io.setDiskCapacity(m_isHDDisk);
}

const FloppyDiskBridge::BridgeDriver* ArduinoFloppyDiskBridge::staticBridgeInformation() {
	return &DriverArduinoFloppy;
}

// Get the name of the drive
const FloppyDiskBridge::BridgeDriver* ArduinoFloppyDiskBridge::_getDriverInfo() {
	return staticBridgeInformation();
}

// Duplicate of the one below, but here for consistency - Returns the name of interface.  This pointer should remain valid after the class is destroyed
const FloppyDiskBridge::DriveTypeID ArduinoFloppyDiskBridge::_getDriveTypeID() {
	return m_isHDDisk ? DriveTypeID::dti35HD : DriveTypeID::dti35DD;
}

// Called to switch which head is being used right now.  Returns success or not
bool ArduinoFloppyDiskBridge::setActiveSurface(const DiskSurface activeSurface) {
	return m_io.selectSurface(activeSurface == DiskSurface::dsUpper ? ArduinoFloppyReader::DiskSurface::dsUpper : ArduinoFloppyReader::DiskSurface::dsLower) == ArduinoFloppyReader::DiagnosticResponse::drOK;
}

// Set the status of the motor on the drive. The motor should maintain this status until switched off or reset.  This should *NOT* wait for the motor to spin up
bool ArduinoFloppyDiskBridge::setMotorStatus(const bool switchedOn) {
	return m_io.enableReading(switchedOn, false, true) == ArduinoFloppyReader::DiagnosticResponse::drOK;
}

// Called to ask the drive what the current write protect status is - return true if its write protected
bool ArduinoFloppyDiskBridge::checkWriteProtectStatus(const bool forceCheck) {
	return m_io.checkIfDiskIsWriteProtected(forceCheck) == ArduinoFloppyReader::DiagnosticResponse::drWriteProtected;
}

// If the above is TRUE then this is called to get the status of the DiskChange line.  Basically, this is TRUE if there is a disk in the drive.
// If force is true you should re-check, if false, then you are allowed to return a cached value from the last disk operation (eg: seek)
bool ArduinoFloppyDiskBridge::getDiskChangeStatus(const bool forceCheck) {
	if ((forceCheck) && (m_io.getFirwareVersion().fullControlMod)) {
		if (m_currentCylinder == 0) {
			if (performNoClickSeek()) {
				return m_io.isDiskInDrive();
			}
		}
	}

	// Perform the check, if required
	return m_io.checkForDisk(forceCheck) != ArduinoFloppyReader::DiagnosticResponse::drNoDiskInDrive;
}

// If we're on track 0, this is the emulator trying to seek to track -1.  We catch this as a special case.  
// Should perform the same operations as setCurrentCylinder in terms of disk change etc but without changing the current cylinder
// Return FALSE if this is not supported by the bridge
bool ArduinoFloppyDiskBridge::performNoClickSeek() {
	// Claim we did it anyway
	if (!m_io.getFirwareVersion().fullControlMod) return true;

	if (m_io.performNoClickSeek() == ArduinoFloppyReader::DiagnosticResponse::drOK) {
		updateLastManualCheckTime();
		return true;
	}
	return false;
}

// Trigger a seek to the requested cylinder, this can block until complete
bool ArduinoFloppyDiskBridge::setCurrentCylinder(const unsigned int cylinder) {
	m_currentCylinder = cylinder;

	// No need if its busy
	bool ignoreDiskCheck = isMotorRunning() && !isReady();

	// If we don't support disk change then don't allow the hardware to check for a disk as it takes time, unless it's due anyway
	if (!m_io.getFirwareVersion().fullControlMod) ignoreDiskCheck |= !isReadyForManualDiskCheck();

	// Go! - and don't ask
	ArduinoFloppyReader::DiagnosticResponse dr = m_io.selectTrack(cylinder, ArduinoFloppyReader::TrackSearchSpeed::tssFast, ignoreDiskCheck);
	if (dr != ArduinoFloppyReader::DiagnosticResponse::drOK) dr = m_io.selectTrack(cylinder, ArduinoFloppyReader::TrackSearchSpeed::tssNormal, ignoreDiskCheck);
	if (dr != ArduinoFloppyReader::DiagnosticResponse::drOK) dr = m_io.selectTrack(cylinder, ArduinoFloppyReader::TrackSearchSpeed::tssNormal, ignoreDiskCheck);
	if (dr != ArduinoFloppyReader::DiagnosticResponse::drOK) dr = m_io.selectTrack(cylinder, ArduinoFloppyReader::TrackSearchSpeed::tssNormal, ignoreDiskCheck);
	if (dr == ArduinoFloppyReader::DiagnosticResponse::drOK) {
		if (!ignoreDiskCheck) updateLastManualCheckTime();
		setWriteProtectStatus(m_io.checkIfDiskIsWriteProtected(false) == ArduinoFloppyReader::DiagnosticResponse::drWriteProtected);

		return true;
	}

	return false;
}

// Called when data should be read from the drive.
//		pll:           supplied if you use it
//		maxBufferSize: Maximum number of RotationExtractor::MFMSample in the buffer.  If we're trying to detect a disk, this might be set VERY LOW
// 	    buffer:		   Where to save to.  When a buffer is saved, position 0 MUST be where the INDEX pulse is.  RevolutionExtractor will do this for you
//		indexMarker:   Used by rotationExtractor if you use it, to help be consistent where the INDEX position is read back at
//		onRotation: A function you should call for each complete revolution received.  If the function returns FALSE then you should abort reading, else keep sending revolutions
// Returns: ReadResponse, explains its self
CommonBridgeTemplate::ReadResponse ArduinoFloppyDiskBridge::readData(PLL::BridgePLL& pll, const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
	std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation) {
	
	ArduinoFloppyReader::DiagnosticResponse result = m_io.readRotation(*pll.rotationExtractor(), maxBufferSize, buffer, indexMarker,
		[&onRotation](RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits) -> bool {
			return onRotation(*mfmData, dataLengthInBits);
		}, true);
		
	switch (result) {
		case ArduinoFloppyReader::DiagnosticResponse::drOK: return ReadResponse::rrOK;
		case ArduinoFloppyReader::DiagnosticResponse::drNoDiskInDrive: return ReadResponse::rrNoDiskInDrive;
		default:  return ReadResponse::rrError;
	}
}

// Called when a cylinder revolution should be written to the disk.
// Parameters are:	rawMFMData						The raw data to be written.  This is an actual MFM stream, going from MSB to LSB for each byte
//					numBytes						Number of bits in the buffer to write
//					writeFromIndex					If an attempt should be made to write this from the INDEX pulse rather than just a random position
//					suggestUsingPrecompensation		A suggestion that you might want to use write pre-compensation, optional
// Returns TRUE if success, or false if it fails.  Largely doesn't matter as most stuff should verify with a read straight after
bool ArduinoFloppyDiskBridge::writeData(const unsigned char* rawMFMData, const unsigned int numBits, const bool writeFromIndex, const bool suggestUsingPrecompensation) {
	switch (m_io.writeCurrentTrackPrecomp(rawMFMData, (numBits + 7) / 8, writeFromIndex, suggestUsingPrecompensation)) {
	case ArduinoFloppyReader::DiagnosticResponse::drOK: return true;
	case ArduinoFloppyReader::DiagnosticResponse::drWriteProtected: setWriteProtectStatus(true); return false;
	default: return false;
	}
}

#endif
