/* ArduinoFloppyReaderWriter aka DrawBridge
*
* Copyright (C) 2017-2021 Robert Smith (@RobSmithDev)
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

////////////////////////////////////////////////////////////////////////////////////////
// Class to manage the communication between the computer and the Arduino    V2.5     //
////////////////////////////////////////////////////////////////////////////////////////
//
// Purpose:
// The class handles the command interface to the arduino.  It doesn't do any decoding
// Just open ports, switch motors on and off, seek to tracks etc.
//
//
//

#include "ArduinoInterface.h"
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include "RotationExtractor.h"
#include <mutex>
#ifndef _WIN32
#include <string.h>
#endif

using namespace ArduinoFloppyReader;

// Command that the ARDUINO Sketch understands
#define COMMAND_VERSION            '?'
#define COMMAND_REWIND             '.'
#define COMMAND_GOTOTRACK          '#'
#define COMMAND_HEAD0              '['
#define COMMAND_HEAD1              ']'
#define COMMAND_READTRACK          '<'
#define COMMAND_ENABLE             '+'
#define COMMAND_DISABLE            '-'
#define COMMAND_WRITETRACK         '>'
#define COMMAND_ENABLEWRITE        '~'
#define COMMAND_DIAGNOSTICS        '&'
#define COMMAND_ERASETRACK		   'X'
#define COMMAND_SWITCHTO_DD		   'D'   // Requires Firmware V1.6
#define COMMAND_SWITCHTO_HD		   'H'   // Requires Firmware V1.6

// New commands for more direct control of the drive.  Some of these are more efficient or dont turn the disk motor on for modded hardware
#define COMMAND_READTRACKSTREAM    '{'    // Requires Firmware V1.8
#define COMMAND_WRITETRACKPRECOMP  '}'    // Requires Firmware V1.8
#define COMMAND_CHECKDISKEXISTS    '^'    // Requires Firmware V1.8 (and modded hardware for fast version)
#define COMMAND_ISWRITEPROTECTED   '$'    // Requires Firmware V1.8
#define COMMAND_ENABLE_NOWAIT      '*'    // Requires Firmware V1.8
#define COMMAND_GOTOTRACK_REPORT   '='	  // Requires Firmware V1.8
#define COMMAND_DO_NOCLICK_SEEK    'O'    // Requires Firmware V1.8a

#define COMMAND_CHECK_DENSITY      'T'    // Requires Firmware V1.9
#define COMMAND_TEST_RPM           'P'    // Requires Firmware V1.9
#define COMMAND_CHECK_FEATURES     '@'    // Requires Firmware V1.9
#define COMMAND_READTRACKSTREAM_HIGHPRECISION 'F' // Requires Firmware V1.9
#define COMMAND_EEPROM_READ        'E'    // Read a value from the eeprom
#define COMMAND_EEPROM_WRITE       'e'    // Write a value to the eeprom
#define COMMAND_RESET              'R'    // Reset

#define SPECIAL_ABORT_CHAR		   'x'

// Convert the last executed command that had an error to a string
std::string lastCommandToName(LastCommand cmd) {
	switch (cmd) {
	case LastCommand::lcOpenPort:		return "OpenPort";
	case LastCommand::lcGetVersion:		return "GetVersion";
	case LastCommand::lcEnableWrite:	return "EnableWrite";
	case LastCommand::lcRewind:			return "Rewind";
	case LastCommand::lcDisableMotor:	return "DisableMotor";
	case LastCommand::lcEnableMotor:	return "EnableMotor";
	case LastCommand::lcGotoTrack:		return "GotoTrack";
	case LastCommand::lcSelectSurface:	return "SelectSurface";
	case LastCommand::lcReadTrack:		return "ReadTrack";
	case LastCommand::lcWriteTrack:		return "WriteTrack";
	case LastCommand::lcRunDiagnostics:	return "RunDiagnostics";
	case LastCommand::lcSwitchDiskMode: return "SetCapacity";
	case LastCommand::lcReadTrackStream: return "ReadTrackStream";
	case LastCommand::lcCheckDiskInDrive: return "CheckDiskInDrive";
	case LastCommand::lcCheckDiskWriteProtected: return "CheckDiskWriteProtected";
	case LastCommand::lcEraseTrack:		return "EraseTrack";
	case LastCommand::lcNoClickCheck:	return "NoClickCheck";
	case LastCommand::lcCheckDensity:	return "CheckDensity";
	case LastCommand::lcMeasureRPM:		return "MeasureRPM";
	case LastCommand::lcEEPROMRead:	    return "EEPROM Read";
	case LastCommand::lcEEPROMWrite:	return "EEPROM Write";
	default:							return "Unknown";
	}
}

// Uses the above fields to constructr a suitable error message, hopefully useful in diagnosing the issue
const std::string ArduinoInterface::getLastErrorStr() const {
	std::stringstream tmp;
	switch (m_lastError) {
	case DiagnosticResponse::drOldFirmware: return "The Arduino is running an older version of the firmware/sketch.  Please re-upload.";
	case DiagnosticResponse::drOK: return "Last command completed successfully.";
	case DiagnosticResponse::drPortInUse: return "The specified COM port is currently in use by another application.";
	case DiagnosticResponse::drPortNotFound: return "The specified COM port was not found.";
	case DiagnosticResponse::drAccessDenied: return "The operating system denied access to the specified COM port.";
	case DiagnosticResponse::drComportConfigError: return "We were unable to configure the COM port using the SetCommConfig() command.";
	case DiagnosticResponse::drBaudRateNotSupported: return "The COM port does not support the 2M baud rate required by this application.";
	case DiagnosticResponse::drErrorReadingVersion: return "An error occured attempting to read the version of the sketch running on the Arduino.";
	case DiagnosticResponse::drErrorMalformedVersion: return "The Arduino returned an unexpected string when version was requested.  This could be a baud rate mismatch or incorrect loaded sketch.";
	case DiagnosticResponse::drCTSFailure: return "Diagnostics reports the CTS line is not connected correctly or is not behaving correctly.";
	case DiagnosticResponse::drTrackRangeError: return "An error occured attempting to go to a track number that was out of allowed range.";
	case DiagnosticResponse::drWriteProtected: return "Unable to write to the disk.  The disk is write protected.";
	case DiagnosticResponse::drPortError: return "An unknown error occured attempting to open access to the specified COM port.";
	case DiagnosticResponse::drDiagnosticNotAvailable: return "CTS diagnostic not available, command GetCommModemStatus failed to execute.";
	case DiagnosticResponse::drSelectTrackError: return "Arduino reported an error seeking to a specific track.";
	case DiagnosticResponse::drTrackWriteResponseError: return "Error receiving status from Arduino after a track write operation.";
	case DiagnosticResponse::drSendDataFailed: return "Error sending track data to be written to disk.  This could be a COM timeout.";
	case DiagnosticResponse::drRewindFailure: return "Arduino was unable to find track 0.  This could be a wiring fault or power supply failure.";
	case DiagnosticResponse::drNoDiskInDrive: return "No disk in drive";
	case DiagnosticResponse::drMediaTypeMismatch: return "An attempt to read or write had an incorrect amount of data given the DD/HD media used";
	case DiagnosticResponse::drWriteTimeout: return "The Arduino could not receive the data quick enough to write to disk. Try connecting via USB2 and not using a USB hub.\n\nIf this still does not work, turn off precomp if you are using it.";
	case DiagnosticResponse::drFramingError: return "The Arduino received bad data from the PC. This could indicate poor connectivity, bad baud rate matching or damaged cables.";
	case DiagnosticResponse::drSerialOverrun: return "The Arduino received data faster than it could handle. This could either be a fault with the CTS connection or the USB/serial interface is faulty";
	case DiagnosticResponse::drUSBSerialBad: return "The USB->Serial converter being used isn't suitable and doesnt run consistantly fast enough.  Please ensure you use a genuine FTDI adapter.";
	case DiagnosticResponse::drError:	tmp << "Arduino responded with an error running the " << lastCommandToName(m_lastCommand) << " command.";
		return tmp.str();

	case DiagnosticResponse::drReadResponseFailed:
		switch (m_lastCommand) {
		case LastCommand::lcGotoTrack: return "Unable to read response from Arduino after requesting to go to a specific track";
		case LastCommand::lcReadTrack: return "Gave up trying to read a full track from the disk.";
		case LastCommand::lcWriteTrack: return "Unable to read response to requesting to write a track.";
		default: tmp << "Error reading response from the Arduino while running command " << lastCommandToName(m_lastCommand) << ".";
			return tmp.str();
		}

	case DiagnosticResponse::drSendFailed:
		if (m_lastCommand == LastCommand::lcGotoTrack)
			return "Unable to send the complete select track command to the Arduino.";
		else {
			tmp << "Error sending the command " << lastCommandToName(m_lastCommand) << " to the Arduino.";
			return tmp.str();
		}

	case DiagnosticResponse::drSendParameterFailed:	tmp << "Unable to send a parameter while executing the " << lastCommandToName(m_lastCommand) << " command.";
		return tmp.str();
	case DiagnosticResponse::drStatusError: tmp << "An unknown response was was received from the Arduino while executing the " << lastCommandToName(m_lastCommand) << " command.";
		return tmp.str();

	default: return "Unknown error.";
	}
}

// Constructor for this class
ArduinoInterface::ArduinoInterface() {
	m_abortStreaming = true;
	m_version = { 0,0,false };
	m_lastError = DiagnosticResponse::drOK;
	m_lastCommand = LastCommand::lcGetVersion;
	m_inWriteMode = false;
	m_isWriteProtected = false;
	m_diskInDrive = false;
	m_isHDMode = false;
	m_abortSignalled = false;
	m_isStreaming = false;
}

// Free me
ArduinoInterface::~ArduinoInterface() {
	abortReadStreaming();
	closePort();
}

// Checks if the disk is write protected.  If forceCheck=false then the last cached version is returned.  This is also updated by checkForDisk() as well as this function
DiagnosticResponse ArduinoInterface::checkIfDiskIsWriteProtected(bool forceCheck) {
	// Test manually
	m_lastCommand = LastCommand::lcCheckDiskWriteProtected;

	if (!forceCheck) {
		return m_isWriteProtected ? DiagnosticResponse::drWriteProtected : DiagnosticResponse::drOK;
	}

	// If no hardware support then return no change
	if ((m_version.major == 1) && (m_version.minor < 8)) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	// Send and update flag
	m_lastError = checkForDisk(true);
	if ((m_lastError == DiagnosticResponse::drStatusError) || (m_lastError == DiagnosticResponse::drOK)) {

		if (m_isWriteProtected)  m_lastError = DiagnosticResponse::drWriteProtected;
	}

	return m_lastError;
}

// This only works normally after the motor has been stepped in one direction or another.  This requires the 'advanced' configuration
DiagnosticResponse ArduinoInterface::checkForDisk(bool forceCheck) {
	// Test manually	
	m_lastCommand = LastCommand::lcCheckDiskInDrive;

	if (!forceCheck) {
		return m_diskInDrive ? DiagnosticResponse::drOK : DiagnosticResponse::drNoDiskInDrive;
	}


	// If no hardware support then return no change
	if ((m_version.major == 1) && (m_version.minor < 8)) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	char response;
	m_lastError = runCommand(COMMAND_CHECKDISKEXISTS, '\0', &response);
	if ((m_lastError == DiagnosticResponse::drStatusError) || (m_lastError == DiagnosticResponse::drOK)) {
		if (response == '#') {
			m_lastError = DiagnosticResponse::drNoDiskInDrive;
			m_diskInDrive = false;
		}
		else {
			if (response == '1') m_diskInDrive = true; else {
				m_lastError = DiagnosticResponse::drReadResponseFailed;
				return m_lastError;
			}
		}

		// Also read the write protect status
		if (!deviceRead(&response, 1, true)) {
			m_lastError = DiagnosticResponse::drReadResponseFailed;
			return m_lastError;
		}

		if ((response == '1') || (response == '#')) m_isWriteProtected = response == '1';
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return m_lastError;
}

// Check if an index pulse can be detected from the drive
DiagnosticResponse ArduinoInterface::testIndexPulse() {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opned.  We need to check what happens as the pin is toggled
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '3');

	return m_lastError;
}

// Guess if the drive is actually a PLUS Mode drive
DiagnosticResponse ArduinoInterface::guessPlusMode(bool &isProbablyPlus) {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opned.  We need to check what happens as the pin is toggled
	char response = '0';
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '6', &response);
	isProbablyPlus = (response != '0');

	if (m_lastError == DiagnosticResponse::drError) m_lastError = DiagnosticResponse::drOK;

	return m_lastError;
}

// Check if data can be detected from the drive
DiagnosticResponse ArduinoInterface::testDataPulse() {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opned.  We need to check what happens as the pin is toggled
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '4');

	return m_lastError;
}

// Check if the USB to Serial device can keep up properly
DiagnosticResponse ArduinoInterface::testTransferSpeed() {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opned.  We need to receive about 10 * 256 bytes of data and verify its all valid
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '5');
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	applyCommTimeouts(true);

	unsigned char buffer[256];
	for (int a = 0; a <= 10; a++) {
		unsigned long read;

		read = m_comPort.read(buffer, sizeof(buffer));

		// With the timeouts we have this shouldn't happen
		if (read != sizeof(buffer)) {
			m_lastError = DiagnosticResponse::drUSBSerialBad;
			applyCommTimeouts(false);
			return m_lastError;
		}

		for (size_t c = 0; c < read; c++) {
			if (buffer[c] != c) {
				m_lastError = DiagnosticResponse::drUSBSerialBad;
				applyCommTimeouts(false);
				return m_lastError;
			}
		}
	}

	applyCommTimeouts(false);
	return m_lastError;
}

// Check CTS status by asking the device to set it and then checking what happened
DiagnosticResponse ArduinoInterface::testCTS() {

	for (int a = 1; a <= 10; a++) {
		// Port opned.  We need to check what happens as the pin is toggled
		m_lastError = runCommand(COMMAND_DIAGNOSTICS, (a & 1) ? '1' : '2');
		if (m_lastError != DiagnosticResponse::drOK) {
			m_lastCommand = LastCommand::lcRunDiagnostics;
			closePort();
			return m_lastError;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		bool ctsStatus = m_comPort.getCTSStatus();

		// This doesnt actually run a command, this switches the CTS line back to its default setting
		m_lastError = runCommand(COMMAND_DIAGNOSTICS);

		if (ctsStatus ^ ((a & 1) != 0)) {
			// If we get here then the CTS value isn't what it should be
			closePort();
			m_lastError = DiagnosticResponse::drCTSFailure;
			return m_lastError;
		}
		// Pass.  Try the other state
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return DiagnosticResponse::drOK;
}

// Attempts to verify if the reader/writer is running on this port
bool ArduinoInterface::isPortCorrect(const std::wstring& portName) {
	// Attempts to verify if the reader/writer is running on this port
	SerialIO port;
	std::string version;
	DiagnosticResponse dr = internalOpenPort(portName, false, true, version, port);
	port.closePort();
	return dr == DiagnosticResponse::drOK;
}

// Attempt to sync and get version
DiagnosticResponse ArduinoInterface::attemptToSync(std::string& versionString, SerialIO& port) {
	char buffer[10];

	// Send 'Version' Request 
	buffer[0] = SPECIAL_ABORT_CHAR;
	buffer[1] = COMMAND_RESET;   // Reset
	buffer[2] = COMMAND_VERSION;

	unsigned long size = port.write(&buffer[0], 3);
	if (size != 3) {
		// Couldn't write to device
		port.closePort();
		return DiagnosticResponse::drPortError;
	}

	memset(buffer, 0, sizeof(buffer));
	int counterNoData = 0;
	int counterData = 0;
	int bytesRead = 0;

	// Keep a rolling buffer looking for the 1Vxxx response
	for (;;) {
		size = port.read(&buffer[4], 1);
		bytesRead += size;
		// Was something read?
		if (size) {
			
			if ((buffer[0] == '1') && (buffer[1] == 'V') && ((buffer[2] >= '1') && (buffer[2] <= '9')) && ((buffer[3] == ',') || (buffer[3] == '.')) && ((buffer[4] >= '0') && (buffer[4] <= '9'))) {

				// Success
				port.purgeBuffers();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				port.purgeBuffers();
				versionString = &buffer[1];
				return DiagnosticResponse::drOK;
			} else {
				if (bytesRead) bytesRead--;
			}

			// Move backwards
			for (int a = 0; a < 4; a++) buffer[a] = buffer[a + 1];

			if (counterData++ > 2048) {
				port.closePort();
				return DiagnosticResponse::drErrorMalformedVersion;
			}
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			if (counterNoData++ > 120) {
				port.closePort();
				return DiagnosticResponse::drErrorReadingVersion;
			}
			if (((counterNoData % 10) == 9) && (bytesRead == 0)) {
				// Give it a kick
				buffer[0] = COMMAND_VERSION;
				size = port.write(&buffer[0], 1);
				if (size != 1) {
					// Couldn't write to device
					port.closePort();
					return DiagnosticResponse::drPortError;
				}
			}
		}
	}
}

// Attempts to verify if the reader/writer is running on this port
DiagnosticResponse ArduinoInterface::internalOpenPort(const std::wstring& portName, bool enableCTSflowcontrol, bool triggerReset, std::string& versionString, SerialIO& port) {
	switch (port.openPort(portName)) {
	case SerialIO::Response::rInUse:return DiagnosticResponse::drPortInUse;
	case SerialIO::Response::rNotFound:return DiagnosticResponse::drPortNotFound;
	case SerialIO::Response::rOK: break;
	default: return DiagnosticResponse::drPortError;
	}

	// Configure the port
	SerialIO::Configuration config;
	config.baudRate = 2000000;
	config.ctsFlowControl = enableCTSflowcontrol;

	if (port.configurePort(config) != SerialIO::Response::rOK) return DiagnosticResponse::drPortError;

	port.setBufferSizes(16, 16);
	port.setReadTimeouts(10, 250);
	port.setWriteTimeouts(2000, 200);

	// Try to get the version
	DiagnosticResponse response = attemptToSync(versionString, port);
	if (response != DiagnosticResponse::drOK) {
		// It failed.  Issue a reset if we're allowed and try again
		if (triggerReset) {
			port.setDTR(false);
			port.setRTS(false);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			port.setDTR(true);
			port.setRTS(true);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			port.closePort();
			std::this_thread::sleep_for(std::chrono::milliseconds(150));

			// Now re-connect and try again
			if (port.openPort(portName) != SerialIO::Response::rOK) return DiagnosticResponse::drPortError;

			response = attemptToSync(versionString, port);
			if (response != DiagnosticResponse::drOK) {
				port.closePort();
				return response;
			}
		}
		else {
			port.closePort();
			return response;
		}
	} 

	return response;
}

// Attempts to open the reader running on the COM port number provided.  Port MUST support 2M baud
DiagnosticResponse ArduinoInterface::openPort(const std::wstring& portName, bool enableCTSflowcontrol) {
	m_lastCommand = LastCommand::lcOpenPort;
	closePort();

	// Quickly force streaming to be aborted
	m_abortStreaming = true;

	std::string versionString;
	m_lastError = internalOpenPort(portName, enableCTSflowcontrol, true, versionString, m_comPort);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	// it's possible theres still redundant data in the buffer
	char buffer[2];
	int counter = 0;
	for (;;) {
		unsigned long size = m_comPort.read(buffer, 1);
		if (size < 1)
			if (counter++ >= 5) break;
	}

	// Possibly a bit overkill
	m_comPort.setBufferSizes(RAW_TRACKDATA_LENGTH_DD * 2, RAW_TRACKDATA_LENGTH_DD);

	m_version.major = versionString[1] - '0';
	m_version.minor = versionString[3] - '0';
	m_version.fullControlMod = versionString[2] == ',';

	// Check features
	m_version.deviceFlags1 = 0;
	m_version.deviceFlags2 = 0;
	m_version.buildNumber = 0;

	if ((m_version.major > 1) || ((m_version.major == 1) && (m_version.minor >= 9))) {
		m_lastError = runCommand(COMMAND_CHECK_FEATURES);
		if (m_lastError != DiagnosticResponse::drOK) return m_lastError;
		if (!deviceRead(&m_version.deviceFlags1, 1)) {
			m_lastError = DiagnosticResponse::drErrorReadingVersion;
			return m_lastError;
		}
		if (!deviceRead(&m_version.deviceFlags2, 1)) {
			m_lastError = DiagnosticResponse::drErrorReadingVersion;
			return m_lastError;
		}
		if (!deviceRead(&m_version.buildNumber, 1)) {
			m_lastError = DiagnosticResponse::drErrorReadingVersion;
			return m_lastError;
		}
	}

	// Switch to normal timeouts
	applyCommTimeouts(false);

	// Ok, success
	return m_lastError;
}

// Apply and change the timeouts on the com port
void ArduinoInterface::applyCommTimeouts(bool shortTimeouts) {
	if (shortTimeouts) {
		m_comPort.setReadTimeouts(5, 12);
	}
	else {
		m_comPort.setReadTimeouts(2000, 200);
	}
	m_comPort.setWriteTimeouts(2000, 200);
}

// Closes the port down
void ArduinoInterface::closePort() {
	LastCommand old = m_lastCommand;
	if (m_comPort.isPortOpen()) {
		// Force the drive to power down
		enableReading(false);
		// And close the handle
		m_comPort.closePort();
	}
	m_inWriteMode = false;
	m_isWriteProtected = false;
	m_diskInDrive = false;
	m_lastCommand = old;
}

// Returns true if the track actually contains some data, else its considered blank or unformatted
bool ArduinoInterface::trackContainsData(const RawTrackDataDD& trackData) const {
	int zerocount = 0, ffcount = 0;
	unsigned char lastByte = trackData[0];
	for (unsigned int counter = 1; counter < RAW_TRACKDATA_LENGTH_DD; counter++) {
		if (trackData[counter] == lastByte) {
			switch (lastByte) {
			case 0xFF: ffcount++; zerocount = 0; break;
			case 0x00: ffcount = 0; zerocount++; break;
			default: zerocount = 0; ffcount = 0;
			}
		}
		else {
			lastByte = trackData[counter];
			zerocount = 0; ffcount = 0;
		}
	}

	// More than this in a row and its bad
	return ((ffcount < 20) && (zerocount < 20));
}

// Turns on and off the writing interface.  If irError is returned the disk is write protected
DiagnosticResponse ArduinoInterface::enableWriting(const bool enable, const bool reset) {

	if (enable) {
		m_lastCommand = LastCommand::lcEnableWrite;

		// Enable the device
		m_lastError = runCommand(COMMAND_ENABLEWRITE);
		if (m_lastError == DiagnosticResponse::drError) {
			m_lastError = DiagnosticResponse::drWriteProtected;
			return m_lastError;
		}
		if (m_lastError != DiagnosticResponse::drOK) return m_lastError;
		m_inWriteMode = true;

		// Reset?
		if (reset) {
			// And rewind to the first track
			m_lastError = findTrack0();
			if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

			// Lets know where we are
			return selectSurface(DiskSurface::dsUpper);
		}
		m_lastError = DiagnosticResponse::drOK;
		return m_lastError;
	}
	else {
		m_lastCommand = LastCommand::lcDisableMotor;

		// Disable the device
		m_lastError = runCommand(COMMAND_DISABLE);
		if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

		m_inWriteMode = false;

		return m_lastError;
	}
}

// Seek to track 0
DiagnosticResponse ArduinoInterface::findTrack0() {
#ifdef _DEBUG
	return  DiagnosticResponse::drOK;
#endif
	m_lastCommand = LastCommand::lcRewind;

	// And rewind to the first track
	char status = '0';
	m_lastError = runCommand(COMMAND_REWIND, '\000', &status);
	if (m_lastError != DiagnosticResponse::drOK) {
		if (status == '#') return DiagnosticResponse::drRewindFailure;
	}
	return m_lastError;
}

// Turns on and off the reading interface
DiagnosticResponse ArduinoInterface::enableReading(const bool enable, const bool reset, const bool dontWait) {
	m_inWriteMode = false;
	if (enable) {
		m_lastCommand = LastCommand::lcEnableMotor;

		// Enable the device
		m_lastError = runCommand(dontWait ? COMMAND_ENABLE_NOWAIT : COMMAND_ENABLE);
		if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

		// Reset?
		if (reset) {
			m_lastError = findTrack0();
			if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

			// Lets know where we are
			return selectSurface(DiskSurface::dsUpper);
		}
		m_lastError = DiagnosticResponse::drOK;
		m_inWriteMode = m_version.fullControlMod;
		return m_lastError;

	}
	else {
		m_lastCommand = LastCommand::lcDisableMotor;

		// Disable the device
		m_lastError = runCommand(COMMAND_DISABLE);

		return m_lastError;
	}
}

// Query the RPM of the drive
DiagnosticResponse ArduinoInterface::measureDriveRPM(float& rpm) {
	m_lastCommand = LastCommand::lcMeasureRPM;

	bool isV19 = (m_version.major > 1) || ((m_version.major == 1) && (m_version.minor >= 9));
	if (!isV19) return DiagnosticResponse::drOldFirmware;

	// Query the RPM
	m_lastError = runCommand(COMMAND_TEST_RPM);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	// Now we read bytes until we get an '\n' or a max of 10
	char buffer[11];
	int index = 0;
	int failCount = 0;
	memset(buffer, 0, sizeof(buffer));

	// Read RPM
	while (index < 10) {
		if (deviceRead(&buffer[index], 1)) {
			if (buffer[index] == '\n') {
				buffer[index] = '\0';
				break;
			}
			index++;
		}
		else
			if (failCount++ > 10) break;
	}

	// And output it as a float
	rpm = (float)atof(buffer);

	if (rpm < 10) m_lastError = DiagnosticResponse::drNoDiskInDrive;

	return m_lastError;
}

// Checks to see what the density of the current disk is most likely to be.  Ideally this should be done while at track 0, probably lower head
DiagnosticResponse ArduinoInterface::checkDiskCapacity(bool& isHD) {
	m_lastCommand = LastCommand::lcCheckDensity;
	//isHD = true;
	//return DiagnosticResponse::drOK;


	bool isV19 = (m_version.major > 1) || ((m_version.major == 1) && (m_version.minor >= 9));
	if (!isV19) return DiagnosticResponse::drOldFirmware;

	if ((m_version.deviceFlags1 & FLAGS_DENSITYDETECT_ENABLED) == 0) {
		isHD = false;
		return DiagnosticResponse::drOK;
	}

	// Query the density
	m_lastError = runCommand(COMMAND_CHECK_DENSITY);
	if (m_lastError != DiagnosticResponse::drOK) {
		return m_lastError;
	}

	// And read the type
	char status;
	if (!deviceRead(&status, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	// 'x' means no disk in drive
	switch (status) {
	case 'x':
		// m_diskInDrive = false;
		// We're not going to update the disk in drive flag, just use it as an OK error
		m_lastError = DiagnosticResponse::drNoDiskInDrive;
		break;

	case 'H':
		m_diskInDrive = true;
		isHD = true;
		m_lastError = DiagnosticResponse::drOK;
		break;

	case 'D':
		m_diskInDrive = true;
		isHD = false;
		m_lastError = DiagnosticResponse::drOK;
		break;
	}

	return m_lastError;
}

// Check and switch to HD disk
DiagnosticResponse ArduinoInterface::setDiskCapacity(bool switchToHD_Disk) {
	m_lastCommand = LastCommand::lcSwitchDiskMode;

	m_lastError = runCommand(switchToHD_Disk ? COMMAND_SWITCHTO_HD : COMMAND_SWITCHTO_DD);

	if (m_lastError == DiagnosticResponse::drOK) m_isHDMode = switchToHD_Disk;

	return m_lastError;
}

// If the drive is on track 0, this does a test seek to -1 if supported
DiagnosticResponse ArduinoInterface::performNoClickSeek() {
	// And send the command and track.  This is sent as ASCII text as a result of terminal testing.  Easier to see whats going on
	bool isV18 = (m_version.major > 1) || ((m_version.major == 1) && (m_version.minor >= 8));
	if (!isV18) return DiagnosticResponse::drOldFirmware;
	if (!m_version.fullControlMod) return DiagnosticResponse::drOldFirmware;

	m_lastCommand = LastCommand::lcNoClickCheck;

	m_lastError = runCommand(COMMAND_DO_NOCLICK_SEEK);
	if (m_lastError == DiagnosticResponse::drOK) {
		// Read extended data
		char status;
		if (!deviceRead(&status, 1, true)) {
			m_lastError = DiagnosticResponse::drReadResponseFailed;
			return m_lastError;
		}
		// 'x' means we didnt check it
		if (status != 'x') m_diskInDrive = status == '1';

		// Also read the write protect status
		if (!deviceRead(&status, 1, true)) {
			m_lastError = DiagnosticResponse::drReadResponseFailed;
			return m_lastError;
		}
		m_isWriteProtected = status == '1';
	}

	return m_lastError;
}

// Select the track, this makes the motor seek to this position
DiagnosticResponse ArduinoInterface::selectTrack(const unsigned char trackIndex, const TrackSearchSpeed searchSpeed, bool ignoreDiskInsertCheck) {
	m_lastCommand = LastCommand::lcGotoTrack;

	if (trackIndex > 81) {
		m_lastError = DiagnosticResponse::drTrackRangeError;
		return m_lastError; // no chance, it can't be done.
	}

	// And send the command and track.  This is sent as ASCII text as a result of terminal testing.  Easier to see whats going on
	bool isV18 = (m_version.major > 1) || ((m_version.major == 1) && (m_version.minor >= 8));
	char buf[8];
	if (isV18) {
		char flags = (int)searchSpeed;
		if (!ignoreDiskInsertCheck) flags |= 4;
#ifdef _WIN32		
		sprintf_s(buf, "%c%02i%c", COMMAND_GOTOTRACK_REPORT, trackIndex, flags);
#else
		sprintf(buf, "%c%02i%c", COMMAND_GOTOTRACK_REPORT, trackIndex, flags);
#endif	
	}
	else {
#ifdef _WIN32			
		sprintf_s(buf, "%c%02i", COMMAND_GOTOTRACK, trackIndex);
#else		
		sprintf(buf, "%c%02i", COMMAND_GOTOTRACK, trackIndex);
#endif	
	}

	// Send track number. 
	if (!deviceWrite(buf, (unsigned int)strlen(buf))) {
		m_lastError = DiagnosticResponse::drSendFailed;
		return m_lastError;
	}

	// Get result
	char result;

	if (!deviceRead(&result, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	switch (result) {
	case '2': m_lastError = DiagnosticResponse::drOK; break; // already at track.  No op needed.  V1.8 only
	case '1': m_lastError = DiagnosticResponse::drOK;		 
		if (isV18) {
			// Read extended data
			char status;
			if (!deviceRead(&status, 1, true)) {
				m_lastError = DiagnosticResponse::drReadResponseFailed;
				return m_lastError;
			}
			// 'x' means we didnt check it
			if (status != 'x') m_diskInDrive = status == '1';

			// Also read the write protect status
			if (!deviceRead(&status, 1, true)) {
				m_lastError = DiagnosticResponse::drReadResponseFailed;
				return m_lastError;
			}
			m_isWriteProtected = status == '1';
		}
		break;
	case '0':    m_lastError = DiagnosticResponse::drSelectTrackError;
		break;
	default:	 m_lastError = DiagnosticResponse::drStatusError;
		break;
	}

	return m_lastError;
}

// Erases the current track by writing 0xAA to it
DiagnosticResponse ArduinoInterface::eraseCurrentTrack() {
	m_lastCommand = LastCommand::lcEraseTrack;
	m_lastError = runCommand(COMMAND_ERASETRACK);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	char result;
	if (!deviceRead(&result, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	if (result == 'N') {
		m_lastError = DiagnosticResponse::drWriteProtected;
		return m_lastError;
	}

	// Check result
	if (!deviceRead(&result, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	if (result != '1') {
		m_lastError = DiagnosticResponse::drError;
		return m_lastError;
	}

	return m_lastError;
}

// Choose which surface of the disk to read from
DiagnosticResponse ArduinoInterface::selectSurface(const DiskSurface side) {
	m_lastCommand = LastCommand::lcSelectSurface;

	m_lastError = runCommand((side == DiskSurface::dsUpper) ? COMMAND_HEAD0 : COMMAND_HEAD1);

	return m_lastError;
}

void writeBit(unsigned char* output, int& pos, int& bit, int value, const int maxLength) {
	if (pos >= maxLength) return;

	output[pos] <<= 1;
	output[pos] |= value;
	bit++;
	if (bit >= 8) {
		pos++;
		bit = 0;
	}
}

void unpack(const unsigned char* data, unsigned char* output, const int maxLength) {
	int pos = 0;
	size_t index = 0;
	int p2 = 0;
	memset(output, 0, maxLength);
	while (pos < maxLength) {
		// Each byte contains four pairs of bits that identify an MFM sequence to be encoded

		for (int b = 6; b >= 0; b -= 2) {
			unsigned char value = (data[index] >> b) & 3;
			switch (value) {
			case 0:
				// This can't happen, its invalid data but we account for 4 '0' bits
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 0, maxLength);
				break;
			case 1: // This is an '01'
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 1, maxLength);
				break;
			case 2: // This is an '001'
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 1, maxLength);
				break;
			case 3: // this is an '0001'
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 0, maxLength);
				writeBit(output, pos, p2, 1, maxLength);
				break;
			}
		}
		index++;
		if (index >= (size_t)maxLength) return;
	}
	// There will be left-over data
}

// Read RAW data from the current track and surface 
DiagnosticResponse ArduinoInterface::readCurrentTrack(RawTrackDataDD& trackData, const bool readFromIndexPulse) {
	return readCurrentTrack(&trackData, RAW_TRACKDATA_LENGTH_DD, readFromIndexPulse);
}

// Read RAW data from the current track and surface HD mode
DiagnosticResponse ArduinoInterface::readCurrentTrack(void* trackData, const int dataLength, const bool readFromIndexPulse) {
	m_lastCommand = LastCommand::lcReadTrack;

	// Length must be one of the two types
	if ((dataLength == RAW_TRACKDATA_LENGTH_DD) && (m_isHDMode)) {
		m_lastError = DiagnosticResponse::drMediaTypeMismatch;
		return m_lastError;
	}
	if ((dataLength == RAW_TRACKDATA_LENGTH_HD) && (!m_isHDMode)) {
		m_lastError = DiagnosticResponse::drMediaTypeMismatch;
		return m_lastError;
	}

	RawTrackDataHD* tmp = (RawTrackDataHD*)malloc(sizeof(RawTrackDataHD));
	if (!tmp) {
		m_lastError = DiagnosticResponse::drError;
		return m_lastError;
	}

	if (m_isHDMode) {
		m_lastCommand = LastCommand::lcReadTrackStream;

		m_lastError = runCommand(COMMAND_READTRACKSTREAM);
		// Allow command retry
		if (m_lastError != DiagnosticResponse::drOK) {
			// Clear the buffer
			m_lastError = runCommand(COMMAND_READTRACKSTREAM);
			if (m_lastError != DiagnosticResponse::drOK) {
				free(tmp);
				return m_lastError;
			}
		}

		// Number of times we failed to read anything
		int readFail = 0;

		// Buffer to read into
		unsigned char tempReadBuffer[64] = { 0 };

		// Sliding window for abort
		char slidingWindow[5] = { 0,0,0,0,0 };
		bool timeout = false;

		unsigned int writePosition = 0;
		m_isStreaming = true;
		m_abortStreaming = false;
		m_abortSignalled = false;
		
		// We know what this is, but the A
		applyCommTimeouts(true);

		while (m_isStreaming) {

			// More efficient to read several bytes in one go		
			unsigned long bytesAvailable = m_comPort.getBytesWaiting();
			if (bytesAvailable < 1) bytesAvailable = 1;
			if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
			unsigned long bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);

			for (size_t a = 0; a < bytesRead; a++) {
				if (m_abortSignalled) {
					// Make space
					for (int s = 0; s < 4; s++) slidingWindow[s] = slidingWindow[s + 1];
					// Append the new byte
					slidingWindow[4] = tempReadBuffer[a];

					// Watch the sliding window for the pattern we need
					if ((slidingWindow[0] == 'X') && (slidingWindow[1] == 'Y') && (slidingWindow[2] == 'Z') && (slidingWindow[3] == SPECIAL_ABORT_CHAR) && (slidingWindow[4] == '1')) {
						m_isStreaming = false;
						m_comPort.purgeBuffers();
						m_lastError = timeout ? DiagnosticResponse::drNoDiskInDrive : DiagnosticResponse::drOK;
						applyCommTimeouts(false);
						bytesRead = 0;
					}
				}
				else {
					// HD Mode
					unsigned char tmp2;
					unsigned char outputByte = 0;

					for (int i = 6; i >= 0; i -= 2) {
						// Convert to other format
						tmp2 = (tempReadBuffer[a] >> i) & 0x03;
						if (tmp2 == 3) tmp2 = 0;

						outputByte <<= 2;
						outputByte |= tmp2 + 1;
					}

					(*tmp)[writePosition] = outputByte;
					writePosition++;
					if (writePosition >= (size_t)dataLength) {
						abortReadStreaming();
					}
				}
			}
			if (bytesRead < 1) {
				readFail++;
				if (readFail > 30) {
					m_abortStreaming = false; // force the 'abort' command to be sent
					abortReadStreaming();
					m_lastError = DiagnosticResponse::drReadResponseFailed;
					m_isStreaming = false;
					free(tmp);
					applyCommTimeouts(false);
					// Force a check for disk
					checkForDisk(true);
					return m_lastError;
				}
				else std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}
	else {

		m_lastError = runCommand(COMMAND_READTRACK);

		// Allow command retry
		if (m_lastError != DiagnosticResponse::drOK) {
			// Clear the buffer
			deviceRead(tmp, dataLength);
			m_lastError = runCommand(COMMAND_READTRACK);
			if (m_lastError != DiagnosticResponse::drOK) {
				free(tmp);
				return m_lastError;
			}
		}

		unsigned char signalPulse = readFromIndexPulse ? 1 : 0;
		if (!deviceWrite(&signalPulse, 1)) {
			m_lastError = DiagnosticResponse::drSendParameterFailed;
			free(tmp);
			return m_lastError;
		}

		// Keep reading until he hit dataLength or a null byte is recieved
		int bytePos = 0;
		int readFail = 0;
		for (;;) {
			unsigned char value;
			if (deviceRead(&value, 1, true)) {
				if (value == 0) break; else
					if (bytePos < dataLength) (*tmp)[bytePos++] = value;
			}
			else {
				readFail++;
				if (readFail > 4) {
					free(tmp);
					m_lastError = DiagnosticResponse::drReadResponseFailed;
					return m_lastError;
				}
			}
		}
	}

	unpack(*tmp, (unsigned char*)trackData, dataLength);
	free(tmp);

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}


// Reads a complete rotation of the disk, and returns it using the callback function whcih can return FALSE to stop
// An instance of RotationExtractor is required.  This is purely to save on re-allocations.  It is internally reset each time
DiagnosticResponse ArduinoInterface::readRotation(RotationExtractor& extractor, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns, std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation) {
	m_lastCommand = LastCommand::lcReadTrackStream;

	if ((m_version.major == 1) && (m_version.minor < 8)) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	// Who would do this, right?
	if ((maxOutputSize < 1) || (!firstOutputBuffer)) {
		m_lastError = DiagnosticResponse::drError;
		return m_lastError;
	}

	const bool highPrecisionMode = (!m_isHDMode) && (m_version.deviceFlags1 & FLAGS_HIGH_PRECISION_SUPPORT);
	m_lastError = runCommand(highPrecisionMode ? COMMAND_READTRACKSTREAM_HIGHPRECISION : COMMAND_READTRACKSTREAM);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	// Let the class know we're doing some streaming stuff
	m_isStreaming = true;
	m_abortStreaming = false;
	m_abortSignalled = false;

	// Number of times we failed to read anything
	int readFail = 0;

	// Buffer to read into
	unsigned char tempReadBuffer[1024] = { 0 };

	// Reset ready for extraction
	extractor.reset(m_isHDMode);

	// Remind it if the 'index' data we want to sync to
	extractor.setIndexSequence(startBitPatterns);

	// Sliding window for abort
	char slidingWindow[5] = { 0,0,0,0,0 };
	bool timeout = false;
	bool dataState = false;
	bool isFirstByte = true;
	unsigned char mfmSequences = 0;
	applyCommTimeouts(true);

	for (;;) {

		// More efficient to read several bytes in one go		
		unsigned long bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		unsigned long bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);

		for (size_t a = 0; a < bytesRead; a++) {
			if (m_abortSignalled) {
				// Make space
				for (int s = 0; s < 4; s++) slidingWindow[s] = slidingWindow[s + 1];
				// Append the new byte
				slidingWindow[4] = tempReadBuffer[a];

				// Watch the sliding window for the pattern we need
				if ((slidingWindow[0] == 'X') && (slidingWindow[1] == 'Y') && (slidingWindow[2] == 'Z') && (slidingWindow[3] == SPECIAL_ABORT_CHAR) && (slidingWindow[4] == '1')) {
					m_isStreaming = false;
					m_comPort.purgeBuffers();
					m_lastError = timeout ? DiagnosticResponse::drError : DiagnosticResponse::drOK;
					applyCommTimeouts(false);
					return m_lastError;
				}
			}
			else {
				unsigned char byteRead = tempReadBuffer[a];

				unsigned char tmp;
				RotationExtractor::MFMSequenceInfo sequence;
				if (m_isHDMode) {
					for (int i = 6; i >= 0; i -= 2) {
						tmp = (byteRead >> i) & 0x03;
						sequence.mfm = (RotationExtractor::MFMSequence)((tmp == 0x03) ? 0 : tmp);
						sequence.timeNS = 4000 + ((unsigned int)sequence.mfm * 2000);
						extractor.submitSequence(sequence, (tmp == 0x03));
					}
				}
				else {
					if (highPrecisionMode) {
						if (isFirstByte) {
							// Throw away the first byte
							isFirstByte = false;
							if (byteRead != 0xC3) {
								// This should never happen.
								abortReadStreaming();
							}
						}
						else {
							if (dataState) {
								unsigned short readSpeed = ((unsigned int)(byteRead & 0x7F) * 2000) / 128;

								tmp = (mfmSequences >> 6) & 0x03;
								sequence.mfm = (tmp == 0) ? RotationExtractor::MFMSequence::mfm0000 : (RotationExtractor::MFMSequence)(tmp - 1);
								sequence.timeNS = 3000 + ((unsigned int)sequence.mfm * 2000) + readSpeed;
								extractor.submitSequence(sequence, (byteRead & 0x80) != 0);

								tmp = (mfmSequences >> 4) & 0x03;
								sequence.mfm = (tmp == 0) ? RotationExtractor::MFMSequence::mfm0000 : (RotationExtractor::MFMSequence)(tmp - 1);
								sequence.timeNS = 3000 + ((unsigned int)sequence.mfm * 2000) + readSpeed;
								extractor.submitSequence(sequence, false);

								tmp = (mfmSequences >> 2) & 0x03;
								sequence.mfm = (tmp == 0) ? RotationExtractor::MFMSequence::mfm0000 : (RotationExtractor::MFMSequence)(tmp - 1);
								sequence.timeNS = 3000 + ((unsigned int)sequence.mfm * 2000) + readSpeed;
								extractor.submitSequence(sequence, false);

								tmp = (mfmSequences) & 0x03;
								sequence.mfm = (tmp == 0) ? RotationExtractor::MFMSequence::mfm0000 : (RotationExtractor::MFMSequence)(tmp - 1);
								sequence.timeNS = 3000 + ((unsigned int)sequence.mfm * 2000) + readSpeed;
								extractor.submitSequence(sequence, false);

								dataState = false;
							}
							else {
								// in 'false' mode we get the flux data
								dataState = true;
								mfmSequences = byteRead;
							}
						}
					}
					else {
						unsigned short readSpeed = (((unsigned long long)(((unsigned int)((byteRead & 0x07) * 16)) * 2000) / 128));

						// Now packet up the data in the format the rotation extractor expects it to be in
						tmp = (byteRead >> 5) & 0x03;
						sequence.mfm = (tmp == 0) ? RotationExtractor::MFMSequence::mfm0000 : (RotationExtractor::MFMSequence)(tmp - 1);
						sequence.timeNS = 3000 + ((unsigned int)sequence.mfm * 2000) + readSpeed;

						extractor.submitSequence(sequence, (byteRead & 0x80) != 0);

						tmp = (byteRead >> 3) & 0x03;
						sequence.mfm = (tmp == 0) ? RotationExtractor::MFMSequence::mfm0000 : (RotationExtractor::MFMSequence)(tmp - 1);
						sequence.timeNS = 3000 + ((unsigned int)sequence.mfm * 2000) + readSpeed;

						extractor.submitSequence(sequence, false);
					}
				}


				// Is it ready to extract?
				if (extractor.canExtract()) {
					unsigned int bits = 0;
					// Go!
					if (extractor.extractRotation(firstOutputBuffer, bits, maxOutputSize)) {
						m_diskInDrive = true;

						if (!onRotation(&firstOutputBuffer, bits)) {
							// And if the callback says so we stop.
							abortReadStreaming();
						}
						// Always save this back
						extractor.getIndexSequence(startBitPatterns);
					}
				}
				else {
					if (extractor.totalTimeReceived() > (m_isHDMode ? 1200000000U : 600000000U))  {
						// No data, stop
						abortReadStreaming();
						timeout = true;
					}
				}
			}
		}
		if (bytesRead < 1) {
			readFail++;
			if (readFail > 30) {
				m_abortStreaming = false; // force the 'abort' command to be sent
				abortReadStreaming();
				m_lastError = DiagnosticResponse::drReadResponseFailed;
				m_isStreaming = false;
				applyCommTimeouts(false);
				return m_lastError;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));			
			}
		} else {
			readFail = 0;
		}
	}
}

// Stops the read streamming immediately and any data in the buffer will be discarded.
bool ArduinoInterface::abortReadStreaming() {
	if ((m_version.major == 1) && (m_version.minor < 8)) {
		return false;
	}


	if (!m_isStreaming) return true;

	// Prevent two things doing this at once, and thus ending with two bytes being written
	std::lock_guard<std::mutex> lock(m_protectAbort);

	if (!m_abortStreaming) {
		// We know what this is, but the Arduino doesn't
		unsigned char command = SPECIAL_ABORT_CHAR;

		m_abortSignalled = true;

		// Send a byte.  This triggers the 'abort' on the Arduino
		if (!deviceWrite(&command, 1)) {
			return false;
		}

	}
	m_abortStreaming = true;
	return true;
}

// Read a bit from the data provided
inline int readBit(const unsigned char* buffer, const unsigned int maxLength, int& pos, int& bit) {
	if (pos >= (int)maxLength) {
		bit--;
		if (bit < 0) {
			bit = 7;
			pos++;
		}
		return (bit & 1) ? 0 : 1;
	}

	int ret = (buffer[pos] >> bit) & 1;
	bit--;
	if (bit < 0) {
		bit = 7;
		pos++;
	}

	return ret;
}


// HD - a bit like the precomp as its more accurate, but no precomp
DiagnosticResponse ArduinoInterface::writeCurrentTrackHD(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse) {
	m_lastCommand = LastCommand::lcWriteTrack;

	if ((m_version.major == 1) && (m_version.minor < 9)) return DiagnosticResponse::drOldFirmware;

	// First step is we need to re-encode the supplied buffer into a packed format 
	// Each nybble looks like: wwxxyyzz, each group is a code which is the transition time

	// *4 is a worse case situation, ie: if everything was a 01.  The +128 is for any extra padding
	const unsigned int maxOutSize = (numBytes * 4) + 16;
	unsigned char* outputBuffer = (unsigned char*)malloc(maxOutSize);

	if (!outputBuffer) {
		m_lastError = DiagnosticResponse::drSendParameterFailed;
		return m_lastError;
	}

	// Original data was written from MSB downto LSB
	int pos = 0;
	int bit = 7;
	unsigned int outputPos = 0;
	unsigned char sequence = 0xAA;  // start at 10101010
	unsigned char* output = outputBuffer;

	// Re-encode the data into our format.  
	while (pos < numBytes) {
		*output = 0;

		for (int i = 0; i < 4; i++) {
			int b, count = 0;

			// See how many zero bits there are before we hit a 1
			do {
				b = readBit(mfmData, numBytes, pos, bit);
				sequence = ((sequence << 1) & 0x7F) | b;
				count++;
			} while (((sequence & 0x08) == 0) && (pos < numBytes + 8));

			// Validate range
			if (count < 2) count = 2;  // <2 would be a 11 sequence, not allowed
			if (count > 4) count = 4;  // max we support 01, 001 and 0001
			switch (i) {
			case 1:
			case 3:
				*output |= (count - 1) << (i * 2);
				break;
			case 0:
				*output |= (count - 1) << (2 * 2);
				break;
			case 2:
				*output |= (count - 1) << (0 * 2);
				break;
			}

		}

		output++;
		outputPos++;
		if (outputPos >= maxOutSize-1) {
			// should never happen
			free(outputBuffer);
			m_lastError = DiagnosticResponse::drSendParameterFailed;
			return m_lastError;
		}	
	}

	// 0 means stop
	*output = 0;
	outputPos++;

	m_lastError = internalWriteTrack(outputBuffer, outputPos, writeFromIndexPulse, false);

	free(outputBuffer);

	return m_lastError;
}

#define PRECOMP_NONE 0x00
#define PRECOMP_ERLY 0x04   
#define PRECOMP_LATE 0x08   

/* If we have a look at the previous and next three bits, assume the current is a '1', then these are the only valid combinations that coudl be allowed
	I have chosen the PRECOMP rule based on trying to get the current '1' as far away as possible from the nearest '1'

	   LAST		 CURRENT      NEXT			PRECOMP			Hex Value
	0	0	0    	1	   0	0	0		Normal
	0	0	0    	1	   0	0	1       Early			0x09
	0	0	0    	1	   0	1	0       Early			0x0A
	0	1	0    	1	   0	0	0       Late			0x28
	0	1	0    	1	   0	0	1       Late			0x29
	0	1	0    	1	   0	1	0       Normal
	1	0	0    	1	   0	0	0       Late			0x48
	1	0	0    	1	   0	0	1       Normal
	1	0	0    	1	   0	1	0       Early			0x4A
*/

// The precomp version of the above. Dont use the above function directly to write precomp mode, it wont work.  Data must be passed with an 0xAA each side at least
DiagnosticResponse ArduinoInterface::writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp) {
	m_lastCommand = LastCommand::lcWriteTrack;

	if ((m_version.major == 1) && (m_version.minor < 8)) return DiagnosticResponse::drOldFirmware;

	if (m_isHDMode) return writeCurrentTrackHD(mfmData, numBytes, writeFromIndexPulse);

	// First step is we need to re-encode the supplied buffer into a packed format with precomp pre-calculated.
	// Each nybble looks like: xxyy
	// where xx is: 0=no precomp, 1=-ve, 2=+ve
	//		 yy is: 0: 4us,		1: 6us,		2: 8us,		3: 10us

	// *4 is a worse case situation, ie: if everything was a 01.  The +128 is for any extra padding
	const unsigned int maxOutSize = (numBytes * 4) + 16;
	unsigned char* outputBuffer = (unsigned char*)malloc(maxOutSize);

	if (!outputBuffer) {
		m_lastError = DiagnosticResponse::drSendParameterFailed;
		return m_lastError;
	}

	// Original data was written from MSB downto LSB
	int pos = 0;
	int bit = 7;
	unsigned int outputPos = 0;
	unsigned char sequence = 0xAA;  // start at 10101010
	unsigned char* output = outputBuffer;
	int lastCount = 2;

	// Re-encode the data into our format and apply precomp.  The +8 is to ensure theres some padding around the edge which will come out as 010101 etc
	while (pos < numBytes) {
		*output = 0;

		for (int i = 0; i < 2; i++) {
			int b, count = 0;

			// See how many zero bits there are before we hit a 1
			do {
				b = readBit(mfmData, numBytes, pos, bit);
				sequence = ((sequence << 1) & 0x7F) | b;
				count++;
			} while (((sequence & 0x08) == 0) && (pos < numBytes + 8));

			// Validate range
			if (count < 2) count = 2;  // <2 would be a 11 sequence, not allowed
			if (count > 5) count = 5;  // max we support 01, 001, 0001, 00001
			
			// Write to stream. Based on the rules above we apply some precomp
			int precomp = 0;
			if (usePrecomp) {
				switch (sequence) {
				case 0x09:
				case 0x0A:
				case 0x4A: // early;
					precomp = PRECOMP_ERLY;
					break;

				case 0x28:
				case 0x29:
				case 0x48: // late
					precomp = PRECOMP_LATE;
					break;

				default:
					precomp = PRECOMP_NONE;
					break;
				}
			}
			else precomp = PRECOMP_NONE;

			*output |= ((lastCount - 2) | precomp) << (i * 4);
			lastCount = count;
		}

		output++;
		outputPos++;
		if (outputPos >= maxOutSize) {
			// should never happen
			free(outputBuffer);
			m_lastError = DiagnosticResponse::drSendParameterFailed;
			return m_lastError;
		}
	}

	m_lastError = internalWriteTrack(outputBuffer, outputPos, writeFromIndexPulse, true);

	free(outputBuffer);

	return m_lastError;
}

// Writes RAW data onto the current track
DiagnosticResponse ArduinoInterface::writeCurrentTrack(const unsigned char* data, const unsigned short numBytes, const bool writeFromIndexPulse) {
	// recomp not supported for HD currently
	if (m_isHDMode) return writeCurrentTrackHD(data, numBytes, writeFromIndexPulse);

	return internalWriteTrack(data, numBytes, writeFromIndexPulse, false);
}

// Writes RAW data onto the current track
DiagnosticResponse ArduinoInterface::internalWriteTrack(const unsigned char* data, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp) {
	m_lastCommand = LastCommand::lcWriteTrack;

	// Fall back if older firmware
	if ((m_version.major == 1) && (m_version.minor < 8) && usePrecomp) {
		return DiagnosticResponse::drOldFirmware;
	}

	m_lastError = runCommand(m_isHDMode ? COMMAND_WRITETRACK : (usePrecomp ? COMMAND_WRITETRACKPRECOMP : COMMAND_WRITETRACK));
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	unsigned char chr;
	if (!deviceRead(&chr, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	// 'N' means NO Writing, aka write protected
	if (chr == 'N') {
		m_lastError = DiagnosticResponse::drWriteProtected;
		return m_lastError;
	}
	if (chr != 'Y') {
		m_lastError = DiagnosticResponse::drStatusError;
		return m_lastError;
	}

	// HD doesnt need the length as the data is null terminated
	if (!m_isHDMode) {
		// Now we send the number of bytes we're planning on transmitting
		unsigned char b = (numBytes >> 8);
		if (!deviceWrite(&b, 1)) {
			m_lastError = DiagnosticResponse::drSendParameterFailed;
			return m_lastError;
		}
		b = numBytes & 0xFF;
		if (!deviceWrite(&b, 1)) {
			m_lastError = DiagnosticResponse::drSendParameterFailed;
			return m_lastError;
		}
	}

	// Explain if we want index pulse sync writing (slower and not required by normal AmigaDOS disks)
	unsigned char b = writeFromIndexPulse ? 1 : 0;
	if (!deviceWrite(&b, 1)) {
		m_lastError = DiagnosticResponse::drSendParameterFailed;
		return m_lastError;
	}

	unsigned char response;
	if (!deviceRead(&response, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	if (response != '!') {
		m_lastError = DiagnosticResponse::drStatusError;

		return m_lastError;
	}

	if (!deviceWrite((const void*)data, numBytes)) {
		m_lastError = DiagnosticResponse::drSendDataFailed;
		return m_lastError;
	}

	if (!deviceRead(&response, 1, true)) {
		m_lastError = DiagnosticResponse::drTrackWriteResponseError;
		return m_lastError;
	}

	// If this is a '1' then the Arduino didn't miss a single bit!
	if (response != '1') {
		switch (response) {
		case 'X': m_lastError = DiagnosticResponse::drWriteTimeout;  break;
		case 'Y': m_lastError = DiagnosticResponse::drFramingError; break;
		case 'Z': m_lastError = DiagnosticResponse::drSerialOverrun; break;
		default:
			m_lastError = DiagnosticResponse::drStatusError;
			break;
		}
		return m_lastError;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

// Returns a list of ports this coudl be available on
void ArduinoInterface::enumeratePorts(std::vector<std::wstring>& portList) {
	portList.clear();
	std::vector<SerialIO::SerialPortInformation> prts;

	SerialIO prt;
	prt.enumSerialPorts(prts);

	for (const SerialIO::SerialPortInformation& port : prts)
		portList.push_back(port.portName);
}


// Run a command that returns 1 or 0 for its response
DiagnosticResponse ArduinoInterface::runCommand(const char command, const char parameter, char* actualResponse) {
	unsigned char response;

	// Pause for I/O
	std::this_thread::sleep_for(std::chrono::milliseconds(1));

	// Send the command
	if (!deviceWrite(&command, 1)) {
		m_lastError = DiagnosticResponse::drSendFailed;
		return m_lastError;
	}

	// Only send the parameter if its not NULL
	if (parameter != '\0')
		if (!deviceWrite(&parameter, 1)) {
			m_lastError = DiagnosticResponse::drSendParameterFailed;
			return m_lastError;
		}

	// And read the response
	if (!deviceRead(&response, 1, true)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	if (actualResponse) *actualResponse = response;

	// Evaluate the response
	switch (response) {
	case '1': m_lastError = DiagnosticResponse::drOK;
		break;
	case '0': m_lastError = DiagnosticResponse::drError;
		break;
	default:  m_lastError = DiagnosticResponse::drStatusError;
		break;
	}
	return m_lastError;
}

// Read a desired number of bytes into the target pointer
bool ArduinoInterface::deviceRead(void* target, const unsigned int numBytes, const bool failIfNotAllRead) {
	if (!m_comPort.isPortOpen()) return false;

	unsigned long read = m_comPort.read(target, numBytes);

	if (read < numBytes) {
		if (failIfNotAllRead) return false;

		// Clear the unread bytes
		char* target2 = ((char*)target) + read;
		memset(target2, 0, numBytes - read);
	}

	return true;
}

// Writes a desired number of bytes from the the pointer
bool ArduinoInterface::deviceWrite(const void* source, const unsigned int numBytes) {
	return m_comPort.write(source, numBytes) == numBytes;
}

// Read a byte from the user eeprom area
DiagnosticResponse ArduinoInterface::eepromRead(unsigned char position, unsigned char& value) {
	m_lastCommand = LastCommand::lcEEPROMRead;

	// Fall back if older firmware
	if ((m_version.major == 1) && (m_version.minor < 9)) {
		return DiagnosticResponse::drOldFirmware;
	}

	m_lastError = runCommand(COMMAND_EEPROM_READ);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	if (!deviceWrite(&position, 1)) {
		m_lastError = DiagnosticResponse::drWriteTimeout;
		return m_lastError;
	}

	if (!deviceRead(&value, 1)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

// Write a byte to the user eeprom area
DiagnosticResponse ArduinoInterface::eepromWrite(unsigned char position, unsigned char value) {
	m_lastCommand = LastCommand::lcEEPROMWrite;

	// Fall back if older firmware
	if ((m_version.major == 1) && (m_version.minor < 9)) {
		return DiagnosticResponse::drOldFirmware;
	}

	m_lastError = runCommand(COMMAND_EEPROM_WRITE);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	if (!deviceWrite(&position, 1)) {
		m_lastError = DiagnosticResponse::drWriteTimeout;
		return m_lastError;
	}

	if (!deviceWrite(&value, 1)) {
		m_lastError = DiagnosticResponse::drWriteTimeout;
		return m_lastError;
	}

	if (!deviceRead(&value, 1)) {
		m_lastError = DiagnosticResponse::drReadResponseFailed;
		return m_lastError;
	}

	if (value != '1') {
		m_lastError = DiagnosticResponse::drError;
		return m_lastError;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_IsAdvancedController(bool& enabled) {
	unsigned char ret[4];

	for (int a = 0; a < 4; a++) {
		DiagnosticResponse r = eepromRead(a, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = ((ret[0] == 0x52) && (ret[1] == 0x6F) && (ret[2] == 0x62) && (ret[3] == 0x53));
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_IsDrawbridgePlusMode(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a+4, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = (ret[0] == 0x2B) && (ret[1] == 0xB2);
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}


DiagnosticResponse ArduinoInterface::eeprom_IsDensityDetectDisabled(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a+6, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = ((ret[0] == 0x44) && (ret[1] == 0x53));
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_IsSlowSeekMode(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a+8, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = (ret[0] == 0x53) && (ret[1] == 0x77);
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_SetAdvancedController(bool enabled) {
	unsigned char ret[4];

	if (enabled) {
		ret[0] = 0x52;
		ret[1] = 0x6F;
		ret[2] = 0x62;
		ret[3] = 0x53;
	}
	else memset(ret, 0, 4);

	for (int a = 0; a < 4; a++) {
		DiagnosticResponse r = eepromWrite(a, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_SetDrawbridgePlusMode(bool enabled) {
	unsigned char ret[2];

	if (enabled) {
		ret[0] = 0x2B;
		ret[1] = 0xB2;
	}
	else memset(ret, 0, 2);

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromWrite(a + 4, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_SetDensityDetectDisabled(bool enabled) {
	unsigned char ret[2];

	if (enabled) {
		ret[0] = 0x44;
		ret[1] = 0x53;
	}
	else memset(ret, 0, 2);

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromWrite(a + 6, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_SetSlowSeekMode(bool enabled) {
	unsigned char ret[2];

	if (enabled) {
		ret[0] = 0x53;
		ret[1] = 0x77;
	}
	else memset(ret, 0, 2);

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromWrite(a + 8, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

