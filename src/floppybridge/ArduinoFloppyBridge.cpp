/* DrawBridge (Arduino Reader/Writer) Bridge for *UAE
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


static const TCHAR* DriverName = _T("Arduino Floppy Disk Reader/Writer, https://amiga.robsmithdev.co.uk");

// Flags from WINUAE
ArduinoFloppyDiskBridge::ArduinoFloppyDiskBridge(const int device_settings, const bool canStall, const bool useIndex) : CommonBridgeTemplate(canStall, useIndex), m_comPort(device_settings) {
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
	// Turn everythign off
	m_io.enableReading(false);
	m_io.closePort();
}

// Returns the COM port number to use
std::wstring ArduinoFloppyDiskBridge::getComPort() {
	// 0 means auto-detect.
	if (m_comPort > 0) {
		wchar_t buffer[20];
#ifdef _WIN32
		swprintf_s(buffer, L"COM%i", m_comPort);
#else		
		swprintf(buffer, 20, L"COM%i", m_comPort);
#endif
		return buffer;
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
		// See which version it is
		ArduinoFloppyReader::FirmwareVersion fv = m_io.getFirwareVersion();
		if ((fv.major <= 1) && (fv.minor < 8)) {
			// Must be at least V1.8
			char buf[20];
#ifdef _WIN32			
			sprintf_s(buf, "%i.%i", fv.major, fv.minor);
#else
			sprintf(buf, "%i.%i", fv.major, fv.minor);
#endif			
			errorMessage = "Arduino Floppy Reader/Writer Firmware is Out Of Date\n\nWinUAE requires V1.8 (and ideally with the modded circuit design).\n\n";
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
					MessageBox(GetDesktopWindow(), _T("The Arduino Reader/Writer hasn't had the 'hardware mod' applied for optimal WinUAE Support.\nThis mod is highly recommended for best experience and will reduce disk access."), _T("Arduino Reader/Writer"), MB_OK | MB_ICONINFORMATION);
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

// Duplicate of the one below, but here for consistancy - Returns the name of interface.  This pointer should remain valid after the class is destroyed
const TCHAR* ArduinoFloppyDiskBridge::_getDriveIDName() {
	return DriverName;
}

// Duplicate of the one below, but here for consistancy - Returns the name of interface.  This pointer should remain valid after the class is destroyed
const FloppyDiskBridge::DriveTypeID ArduinoFloppyDiskBridge::_getDriveTypeID() {
	return FloppyDiskBridge::DriveTypeID::dti35DD;
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
	// Perform the check, if required
	return m_io.checkForDisk(forceCheck) != ArduinoFloppyReader::DiagnosticResponse::drNoDiskInDrive;
}

// If we're on track 0, this is the emulator trying to seek to track -1.  We catch this as a special case.  
// Should perform the same operations as setCurrentCylinder in terms of diskchange etc but without changing the current cylinder
// Return FALSE if this is not supported by the bridge
bool ArduinoFloppyDiskBridge::performNoClickSeek() {
	// Claim we did it anyway
	if (!m_io.getFirwareVersion().fullControlMod) return true;

	if (m_io.performNoClickSeek() == ArduinoFloppyReader::DiagnosticResponse::drOK) {
		bool ignoreDiskCheck = (isMotorRunning()) && (!isReady());
		updateLastManualCheckTime();
		return true;
	}
	return false;
}

// Trigger a seek to the requested cylinder, this can block until complete
bool ArduinoFloppyDiskBridge::setCurrentCylinder(const unsigned int cylinder) {
	// No need if its busy
	bool ignoreDiskCheck = (isMotorRunning()) && (!isReady());

	// If we dont support diskchange then dont allow the hardware to check for a disk as it takes time, unless it's due anyway
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
//		rotationExtractor: supplied if you use it
//		maxBufferSize: Maximum number of RotationExtractor::MFMSample in the buffer.  If we're trying to detect a disk, this might be set VERY LOW
// 	    buffer:		   Where to save to.  When a buffer is saved, position 0 MUST be where the INDEX pulse is.  RevolutionExtractor will do this for you
//		indexMarker:   Used by rotationExtractor if you use it, to help be consistant where the INDEX position is read back at
//		onRotation: A function you should call for each complete revolution received.  If the function returns FALSE then you should abort reading, else keep sending revolutions
// Returns: ReadResponse, explains its self
CommonBridgeTemplate::ReadResponse ArduinoFloppyDiskBridge::readData(RotationExtractor& rotationExtractor, const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
	std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation) {
	
	ArduinoFloppyReader::DiagnosticResponse result = m_io.readRotation(rotationExtractor, maxBufferSize, buffer, indexMarker,
		[&onRotation](RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits) -> bool {
			return onRotation(*mfmData, dataLengthInBits);
		});

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
//					suggestUsingPrecompensation		A suggestion that you might want to use write precompensation, optional
// Returns TRUE if success, or false if it fails.  Largely doesnt matter as most stuff should verify with a read straight after
bool ArduinoFloppyDiskBridge::writeData(const unsigned char* rawMFMData, const unsigned int numBits, const bool writeFromIndex, const bool suggestUsingPrecompensation) {
	switch (m_io.writeCurrentTrackPrecomp(rawMFMData, (numBits + 7) / 8, writeFromIndex, suggestUsingPrecompensation)) {
	case ArduinoFloppyReader::DiagnosticResponse::drOK: return true;
	case ArduinoFloppyReader::DiagnosticResponse::drWriteProtected: setWriteProtectStatus(true); return false;
	default: return false;
	}
}

#endif
