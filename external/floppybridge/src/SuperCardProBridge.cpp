/* WinUAE Supercard Pro Interface for *UAE
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

#include "floppybridge_config.h"

#ifdef ROMTYPE_SUPERCARDPRO_WRITER

#include "floppybridge_abstract.h"
#include "CommonBridgeTemplate.h"
#include "SuperCardProBridge.h"
#include "SuperCardProInterface.h"


using namespace SuperCardPro;


static const FloppyDiskBridge::BridgeDriver DriverSupercardProFloppy = {
	"SuperCard Pro", "https://www.cbmstuff.com/", "Jim Drew", "RobSmithDev", CONFIG_OPTIONS_COMPORT | CONFIG_OPTIONS_COMPORT_AUTODETECT | CONFIG_OPTIONS_DRIVE_AB | CONFIG_OPTIONS_SMARTSPEED | CONFIG_OPTIONS_AUTOCACHE
};


// Flags from WINUAE
SupercardProDiskBridge::SupercardProDiskBridge(FloppyBridge::BridgeMode bridgeMode, FloppyBridge::BridgeDensityMode bridgeDensity, bool enableAutoCache, bool useSmartSpeed, bool autoDetectComPort, char* comPort, bool driveOnB) :
	CommonBridgeTemplate(bridgeMode, bridgeDensity, enableAutoCache, useSmartSpeed), m_comPort(autoDetectComPort ? comPort : ""), m_useDriveA(!driveOnB) {
}

// This is for the static version
SupercardProDiskBridge::SupercardProDiskBridge(FloppyBridge::BridgeMode bridgeMode, FloppyBridge::BridgeDensityMode bridgeDensity, int uaeSettings) :
	CommonBridgeTemplate(bridgeMode, bridgeDensity, false, false), m_useDriveA((uaeSettings & 0x0F) == 0), m_comPort("") {
}


SupercardProDiskBridge::~SupercardProDiskBridge() {
}

// If your device supports being able to abort a disk read, mid-read then implement this
void SupercardProDiskBridge::abortDiskReading() {
	m_io.abortReadStreaming();
};

// A manual way to detect a disk change event
bool SupercardProDiskBridge::attemptToDetectDiskChange() {
	switch (m_io.checkForDisk(true)) {
	case SCPErr::scpOK: return true;
	case SCPErr::scpNoDiskInDrive: return false;
	default: return isDiskInDrive();
	}
}

// If your device supports the DiskChange option then return TRUE here.  If not, then the code will simulate it
bool SupercardProDiskBridge::supportsDiskChange() {
	return true;
}

// Called when the class is about to shut down
void SupercardProDiskBridge::closeInterface() {
	// Turn everything off
	m_io.enableMotor(false);
	m_io.closePort();
}

// Called to start the interface, you should update any error messages if it fails.  This needs to be ready to see to any cylinder and so should already know where cylinder 0 is
bool SupercardProDiskBridge::openInterface(std::string& errorMessage) {
	SCPErr error = m_io.openPort(m_useDriveA);

	if (error == SCPErr::scpOK) {
		m_io.findTrack0();
		m_currentCylinder = 0;
		return true;
	}
	else {
		switch (error) {
		case SCPErr::scpFirmwareTooOld: errorMessage = "SuperCard Pro firmware must be updated to V1.3"; break;
		case SCPErr::scpNotFound: errorMessage = "SuperCard Pro board was not detected."; break;
		case SCPErr::scpInUse: errorMessage = "SuperCard Pro board is already in use."; break;
		default: errorMessage = "An unknown error occurred connecting to your SuperCard Pro."; break;
		}
	}

	return false;
}


const FloppyDiskBridge::BridgeDriver* SupercardProDiskBridge::staticBridgeInformation() {
	return &DriverSupercardProFloppy;
}

// Get the name of the drive
const FloppyDiskBridge::BridgeDriver* SupercardProDiskBridge::_getDriverInfo() {
	return staticBridgeInformation();
}

// Duplicate of the one below, but here for consistency - Returns the name of interface.  This pointer should remain valid after the class is destroyed
const FloppyDiskBridge::DriveTypeID SupercardProDiskBridge::_getDriveTypeID() {
	return m_isHDDisk ? DriveTypeID::dti35HD : DriveTypeID::dti35DD;
}

// Called when a disk is inserted so that you can (re)populate the response to _getDriveTypeID()
void SupercardProDiskBridge::checkDiskType() {
	bool capacity;

	// Check capacity
	if (m_io.checkDiskCapacity(capacity)) {
		m_isHDDisk = capacity;
		m_io.selectDiskDensity(m_isHDDisk);
	}
	else {
		m_isHDDisk = false;
		m_io.selectDiskDensity(m_isHDDisk);
	}
}


// Called to force into DD or HD mode.  Overrides checkDiskType() until checkDiskType() is called again
void SupercardProDiskBridge::forceDiskDensity(bool forceHD) {
	m_isHDDisk = forceHD;
	m_io.selectDiskDensity(m_isHDDisk);
}


// Called to switch which head is being used right now.  Returns success or not
bool SupercardProDiskBridge::setActiveSurface(const DiskSurface activeSurface) {
	return m_io.selectSurface(activeSurface == DiskSurface::dsUpper ? SuperCardPro::DiskSurface::dsUpper : SuperCardPro::DiskSurface::dsLower);
}

// Set the status of the motor on the drive. The motor should maintain this status until switched off or reset.  This should *NOT* wait for the motor to spin up
bool SupercardProDiskBridge::setMotorStatus(const bool switchedOn) {
	m_motorIsEnabled = switchedOn;
	m_motorTurnOnTime = std::chrono::steady_clock::now();
	return m_io.enableMotor(switchedOn, true);
}

// Called to ask the drive what the current write protect status is - return true if its write protected
bool SupercardProDiskBridge::checkWriteProtectStatus(const bool forceCheck) {
	return m_io.isWriteProtected();
}

// If the above is TRUE then this is called to get the status of the DiskChange line.  Basically, this is TRUE if there is a disk in the drive.
// If force is true you should re-check, if false, then you are allowed to return a cached value from the last disk operation (eg: seek)
bool SupercardProDiskBridge::getDiskChangeStatus(const bool forceCheck) {
	// We actually trigger a SEEK operation to ensure this is right
	if (forceCheck) {
		if (m_io.checkForDisk(forceCheck) == SCPErr::scpNoDiskInDrive) {
			if (m_currentCylinder == 0) {
				m_io.performNoClickSeek();
			}
			else {
				m_io.selectTrack((m_currentCylinder > 40) ? m_currentCylinder - 1 : m_currentCylinder + 1, true);
				m_io.selectTrack(m_currentCylinder, true);
			}
		}
	}

	switch (m_io.checkForDisk(forceCheck)) {
	case SCPErr::scpOK: return true;
	case SCPErr::scpNoDiskInDrive: return false;
	default: return isDiskInDrive();
	}
}

// If we're on track 0, this is the emulator trying to seek to track -1.  We catch this as a special case.  
// Should perform the same operations as setCurrentCylinder in terms of disk change etc but without changing the current cylinder
// Return FALSE if this is not supported by the bridge
bool SupercardProDiskBridge::performNoClickSeek() {
	if (m_io.performNoClickSeek()) {
		updateLastManualCheckTime();
		return true;
	}
	return false;
}


// Trigger a seek to the requested cylinder, this can block until complete
bool SupercardProDiskBridge::setCurrentCylinder(const unsigned int cylinder) {
	m_currentCylinder = cylinder;

	// No need if its busy
	bool ignoreDiskCheck = (isMotorRunning()) && (!isReady());

	// Go!
	if (m_io.selectTrack(cylinder, ignoreDiskCheck)) {
		if (!ignoreDiskCheck) updateLastManualCheckTime();	
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
CommonBridgeTemplate::ReadResponse SupercardProDiskBridge::readData(PLL::BridgePLL& pll, const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
	std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation) {
	SCPErr result = m_io.readRotation(pll, maxBufferSize, buffer, indexMarker,
	                                  [&onRotation](RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits) -> bool {
		                                  return onRotation(*mfmData, dataLengthInBits);
	                                  });
	m_motorTurnOnTime = std::chrono::steady_clock::now();
	
	switch (result) {
	case SCPErr::scpOK: return ReadResponse::rrOK;
	case SCPErr::scpNoDiskInDrive: return ReadResponse::rrNoDiskInDrive;
	default:  return ReadResponse::rrError;
	}
	
}

// Called when a cylinder revolution should be written to the disk.
// Parameters are:	rawMFMData						The raw data to be written.  This is an actual MFM stream, going from MSB to LSB for each byte
//					numBytes						Number of bits in the buffer to write
//					writeFromIndex					If an attempt should be made to write this from the INDEX pulse rather than just a random position
//					suggestUsingPrecompensation		A suggestion that you might want to use write pre-compensation, optional
// Returns TRUE if success, or false if it fails.  Largely doesn't matter as most stuff should verify with a read straight after
bool SupercardProDiskBridge::writeData(const unsigned char* rawMFMData, const unsigned int numBits, const bool writeFromIndex, const bool suggestUsingPrecompensation) {
	SCPErr response = m_io.writeCurrentTrackPrecomp(rawMFMData, (numBits + 7) / 8, writeFromIndex, suggestUsingPrecompensation);

	m_motorTurnOnTime = std::chrono::steady_clock::now();

	switch (response) {
	case SCPErr::scpOK: return true;
	case SCPErr::scpWriteProtected: setWriteProtectStatus(true); return false;
	default:  return false;
	}
}

// This is called by the main thread in case you need to do anything specific at regular intervals
void SupercardProDiskBridge::poll() {
	// SCP has a watchdog timeout and after a while the motor will switch off after inactivity.  We can't allow this as doesn't work for how the Amiga might do things
	if (m_motorIsEnabled) {
		const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_motorTurnOnTime).count();
		if (timePassed > (m_io.getMotorIdleTimeoutTime() / 2)) {
			m_io.enableMotor(true, true); 
			m_motorTurnOnTime = std::chrono::steady_clock::now();
		}
	}
}


#endif
