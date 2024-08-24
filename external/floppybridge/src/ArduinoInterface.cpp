/* ArduinoFloppyReaderWriter aka DrawBridge
*
* Copyright (C) 2017-2024 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 3 of the License, or (at your option) any later version.
*
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
#include <thread>
#include <chrono>
#include "RotationExtractor.h"
#include <mutex>
#include <math.h>
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

// New commands for more direct control of the drive.  Some of these are more efficient or don't turn the disk motor on for modded hardware
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
#define COMMAND_READTRACKSTREAM_FLUX          'L'   // Requires Firmware V1.9.22
#define COMMAND_READTRACKSTREAM_HALFPLL       'l'   // Requires Firmware V1.9.22
#define COMMAND_EEPROM_READ        'E'    // Read a value from the eeprom
#define COMMAND_EEPROM_WRITE       'e'    // Write a value to the eeprom
#define COMMAND_RESET              'R'    // Reset
#define COMMAND_WRITEFLUX          'Y'    // Requires Firmware V1.9.22
#define COMMAND_ERASEFLUX          'w'    // Requires Firmware V1.9.18

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
	case LastCommand::lcWriteFlux:		return "Write Flux";
	case LastCommand::lcEraseFlux:		return "Erase Flux";
	default:							return "Unknown";
	}
}

// Uses the above fields to constructor a suitable error message, hopefully useful in diagnosing the issue
const std::string ArduinoInterface::getLastErrorStr() const {
	std::stringstream tmp;
	switch (m_lastError) {
	case DiagnosticResponse::drOldFirmware: return "The Arduino/DrawBridge is running an older version of the firmware/sketch.  Please re-upload.";
	case DiagnosticResponse::drOK: return "Last command completed successfully.";
	case DiagnosticResponse::drPortInUse: return "The specified COM port is currently in use by another application.";
	case DiagnosticResponse::drPortNotFound: return "The specified COM port was not found.";
	case DiagnosticResponse::drAccessDenied: return "The operating system denied access to the specified COM port.";
	case DiagnosticResponse::drComportConfigError: return "We were unable to configure the COM port using the SetCommConfig() command.";
	case DiagnosticResponse::drBaudRateNotSupported: return "The COM port does not support the 2M baud rate required by this application.";
	case DiagnosticResponse::drErrorReadingVersion: return "An error occurred attempting to read the version of the sketch running on the Arduino.";
	case DiagnosticResponse::drErrorMalformedVersion: return "The Arduino returned an unexpected string when version was requested.  This could be a baud rate mismatch or incorrect loaded sketch.";
	case DiagnosticResponse::drCTSFailure: return "Diagnostics reports the CTS line is not connected correctly or is not behaving correctly.";
	case DiagnosticResponse::drTrackRangeError: return "An error occurred attempting to go to a track number that was out of allowed range.";
	case DiagnosticResponse::drWriteProtected: return "Unable to write to the disk.  The disk is write protected.";
	case DiagnosticResponse::drPortError: return "An unknown error occurred attempting to open access to the specified COM port.";
	case DiagnosticResponse::drDiagnosticNotAvailable: return "CTS diagnostic not available, command GetCommModemStatus failed to execute.";
	case DiagnosticResponse::drSelectTrackError: return "Arduino reported an error seeking to a specific track.";
	case DiagnosticResponse::drTrackWriteResponseError: return "Error receiving status from Arduino after a track write operation.";
	case DiagnosticResponse::drSendDataFailed: return "Error sending track data to be written to disk.  This could be a COM timeout.";
	case DiagnosticResponse::drRewindFailure: return "Arduino was unable to find track 0.  This could be a wiring fault or power supply failure.";
	case DiagnosticResponse::drNoDiskInDrive: return "No disk in drive";
	case DiagnosticResponse::drMediaTypeMismatch: if (m_lastCommand == LastCommand::lcWriteFlux)
		return "Write Flux only supports DD disks and data"; else return "An attempt to read or write had an incorrect amount of data given the DD/HD media used";
	case DiagnosticResponse::drWriteTimeout: return "The Arduino could not receive the data quick enough to write to disk. Try connecting via USB2 and not using a USB hub.\n\nIf this still does not work, turn off precomp if you are using it.";
	case DiagnosticResponse::drFramingError: return "The Arduino received bad data from the PC. This could indicate poor connectivity, bad baud rate matching or damaged cables.";
	case DiagnosticResponse::drSerialOverrun: return "The Arduino received data faster than it could handle. This could either be a fault with the CTS connection or the USB/serial interface is faulty";
	case DiagnosticResponse::drUSBSerialBad: return "The USB->Serial converter being used isn't suitable and doesn't run consistently fast enough.  Please ensure you use a genuine FTDI adapter.";
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
	if (m_tempBuffer) free(m_tempBuffer);
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
	if (m_version.major == 1 && m_version.minor < 8) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	// Send and update flag
	m_lastError = checkForDisk(true);
	if (m_lastError == DiagnosticResponse::drStatusError || m_lastError == DiagnosticResponse::drOK) {

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
	if (m_version.major == 1 && m_version.minor < 8) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	char response;
	m_lastError = runCommand(COMMAND_CHECKDISKEXISTS, '\0', &response);
	if (m_lastError == DiagnosticResponse::drStatusError || m_lastError == DiagnosticResponse::drOK) {
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

		if (response == '1' || response == '#') m_isWriteProtected = response == '1';
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return m_lastError;
}

// Check if an index pulse can be detected from the drive
DiagnosticResponse ArduinoInterface::testIndexPulse() {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opened.  We need to check what happens as the pin is toggled
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '3');

	return m_lastError;
}

// Guess if the drive is actually a PLUS Mode drive
DiagnosticResponse ArduinoInterface::guessPlusMode(bool& isProbablyPlus) {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opened.  We need to check what happens as the pin is toggled
	char response = '0';
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '6', &response);
	isProbablyPlus = response != '0';

	if (m_lastError == DiagnosticResponse::drError) m_lastError = DiagnosticResponse::drOK;

	return m_lastError;
}

// Check if data can be detected from the drive
DiagnosticResponse ArduinoInterface::testDataPulse() {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opened.  We need to check what happens as the pin is toggled
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '4');

	return m_lastError;
}

// Check if the USB to Serial device can keep up properly
DiagnosticResponse ArduinoInterface::testTransferSpeed() {
	m_lastCommand = LastCommand::lcRunDiagnostics;

	// Port opened.  We need to receive about 10 * 256 bytes of data and verify its all valid
	m_lastError = runCommand(COMMAND_DIAGNOSTICS, '5');
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	applyCommTimeouts(true);

	unsigned char buffer[256];
	for (int a = 0; a <= 10; a++) {
		unsigned long read;

		read = m_comPort.read(buffer, sizeof buffer);

		// With the timeouts we have this shouldn't happen
		if (read != sizeof buffer) {
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
		// Port opened.  We need to check what happens as the pin is toggled
		m_lastError = runCommand(COMMAND_DIAGNOSTICS, a & 1 ? '1' : '2');
		if (m_lastError != DiagnosticResponse::drOK) {
			m_lastCommand = LastCommand::lcRunDiagnostics;
			closePort();
			return m_lastError;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		bool ctsStatus = m_comPort.getCTSStatus();

		// This doesn't actually run a command, this switches the CTS line back to its default setting
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

	memset(buffer, 0, sizeof buffer);
	int counterNoData = 0;
	int counterData = 0;
	int bytesRead = 0;

	// Keep a rolling buffer looking for the 1Vxxx response
	std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
	for (;;) {
		const auto timePassed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count();

		// Timeout after 8 seconds
		if (timePassed >= 8)
			return DiagnosticResponse::drPortError;

		size = port.read(&buffer[4], 1);
		bytesRead += size;
		// Was something read?
		if (size) {

			if (buffer[0] == '1' && buffer[1] == 'V' && (buffer[2] >= '1' && buffer[2] <= '9') && (buffer[3] == ',' || buffer[3] == '.') && (buffer[4] >= '0' && buffer[4] <= '9')) {

				// Success
				port.purgeBuffers();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				port.purgeBuffers();
				versionString = &buffer[1];
				return DiagnosticResponse::drOK;
			}
			else {
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
			if (counterNoData % 7 == 6 && bytesRead == 0) {
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

	if (port.configurePort(config) != SerialIO::Response::rOK)
	{
		port.closePort();
		return DiagnosticResponse::drPortError;
	}

	port.setBufferSizes(16, 16);
	port.setReadTimeouts(10, 250);
	port.setWriteTimeouts(2000, 200);

	// Try to get the version
	DiagnosticResponse response = attemptToSync(versionString, port);
	if (response != DiagnosticResponse::drOK) {
		// It failed. Issue a reset if we're allowed and try again
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

	// it's possible there's still redundant data in the buffer
	char buffer[2];
	int counter = 0;
	while (m_comPort.getBytesWaiting()) {
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
	return ffcount < 20 && zerocount < 20;
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
	memset(buffer, 0, sizeof buffer);

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
		// 'x' means we didn't check it
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

	if (trackIndex > 83) {
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
			// 'x' means we didn't check it
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

	m_lastError = runCommand(side == DiskSurface::dsUpper ? COMMAND_HEAD0 : COMMAND_HEAD1);

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
			switch ((data[index] >> b) & 3) {
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

// Reads just enough data to fulfill most extractions needed, but doesnt care about rotation position or index - pll should have the LinearExtractor configured
DiagnosticResponse ArduinoInterface::readData(PLL::BridgePLL& pll) {
	pll.rotationExtractor()->reset(m_isHDMode);
	LinearExtractor* linearExtractor = dynamic_cast<LinearExtractor*>(pll.rotationExtractor());
	if (!linearExtractor) return DiagnosticResponse::drError;

	if (!m_tempBuffer) {
		m_tempBuffer = malloc(RAW_TRACKDATA_LENGTH_HD);
		if (!m_tempBuffer) return DiagnosticResponse::drError;
	}

	const uint32_t sizeRequired = m_isHDMode ? RAW_TRACKDATA_LENGTH_HD : RAW_TRACKDATA_LENGTH_DD;
	DiagnosticResponse response = readCurrentTrack(m_tempBuffer, sizeRequired, false);
	if (response != DiagnosticResponse::drOK) return response;
	linearExtractor->copyToBuffer(m_tempBuffer, sizeRequired);
	return response;
	
	m_lastCommand = LastCommand::lcReadTrackStream;

	if (m_version.major == 1 && m_version.minor < 8) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	const bool highPrecisionMode = !m_isHDMode && m_version.deviceFlags1 & FLAGS_HIGH_PRECISION_SUPPORT;
	char mode = highPrecisionMode ? COMMAND_READTRACKSTREAM_HIGHPRECISION : COMMAND_READTRACKSTREAM;

	if (mode == COMMAND_READTRACKSTREAM_HIGHPRECISION && m_version.deviceFlags1 & FLAGS_FLUX_READ) mode = COMMAND_READTRACKSTREAM_HALFPLL;
	m_lastError = runCommand(mode);
	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;
	m_isStreaming = true;

	applyCommTimeouts(true);
	unsigned char tempReadBuffer[4096] = { 0 };

	// Let the class know we're doing some streaming stuff
	m_abortStreaming = false;
	m_abortSignalled = false;

	// Number of times we failed to read anything
	int32_t readFail = 0;

	// Sliding window for abort
	char slidingWindow[5] = { 0,0,0,0,0 };
	bool timeout = false;
	bool dataState = false;
	bool isFirstByte = true;
	unsigned char mfmSequences = 0;

	MFMExtractionTarget* extractor = pll.rotationExtractor();

	for (;;) {

		// More efficient to read several bytes in one go		
		unsigned int bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof tempReadBuffer) bytesAvailable = sizeof tempReadBuffer;
		unsigned int bytesRead = m_comPort.read(tempReadBuffer, bytesAvailable);
		for (size_t a = 0; a < bytesRead; a++) {
			const unsigned char byteRead = tempReadBuffer[a];
			if (m_abortSignalled) {
				// Make space
				for (int s = 0; s < 4; s++) slidingWindow[s] = slidingWindow[s + 1];
				// Append the new byte
				slidingWindow[4] = byteRead;

				// Watch the sliding window for the pattern we need
				if (slidingWindow[0] == 'X' && slidingWindow[1] == 'Y' && slidingWindow[2] == 'Z' && slidingWindow[3] == SPECIAL_ABORT_CHAR && slidingWindow[4] == '1') {
					m_isStreaming = false;
					m_comPort.purgeBuffers();
					m_lastError = timeout ? DiagnosticResponse::drError : DiagnosticResponse::drOK;
					applyCommTimeouts(false);
					return m_lastError;
				}
			}
			else {
				unsigned char tmp;
				RotationExtractor::MFMSequenceInfo sequence{};
				if (m_isHDMode) {
					for (int i = 6; i >= 0; i -= 2) {
						tmp = byteRead >> i & 0x03;
						sequence.mfm = (RotationExtractor::MFMSequence)((tmp == 0x03 ? 0 : tmp) + 1);
						sequence.timeNS = 2000 + (unsigned int)sequence.mfm * 2000;
						extractor->submitSequence(sequence, tmp == 0x03);
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

								if (mode == COMMAND_READTRACKSTREAM_HALFPLL) {
									const int32_t sequences[4] = { (mfmSequences >> 6) & 0x03, (mfmSequences >> 4) & 0x03, (mfmSequences >> 2) & 0x03, mfmSequences & 0x03 };

									// First remove any flux time that us 000 as thats always fixed
									int32_t readSpeed = (unsigned int)(byteRead & 0x7F) * 2000 / 128;

									// Output the fluxes
									for (int a = 0; a < 4; a++) {
										if (sequences[a] == 0) {
											sequence.mfm = RotationExtractor::MFMSequence::mfm000;
											sequence.timeNS = 5000 + readSpeed;
										}
										else {
											sequence.mfm = (RotationExtractor::MFMSequence)sequences[a];
											sequence.timeNS = 1000 + readSpeed + (((int)sequence.mfm) * 2000);
										}
										sequence.pllTimeNS = sequence.timeNS;
										extractor->submitSequence(sequence, ((byteRead & 0x80) != 0) && (a == 0));
									}

								}
								else {
									int32_t readSpeed = (unsigned int)(byteRead & 0x7F) * 2000 / 128;
									tmp = mfmSequences >> 6 & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor->submitSequence(sequence, (byteRead & 0x80) != 0);

									tmp = mfmSequences >> 4 & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor->submitSequence(sequence, false);

									tmp = mfmSequences >> 2 & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor->submitSequence(sequence, false);

									tmp = mfmSequences & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor->submitSequence(sequence, false);
								}

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
						unsigned short readSpeed = (unsigned long long)((unsigned int)((byteRead & 0x07) * 16) * 2000) / 128;

						// Now packet up the data in the format the rotation extractor expects it to be in
						tmp = byteRead >> 5 & 0x03;
						sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
						sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + readSpeed;

						extractor->submitSequence(sequence, (byteRead & 0x80) != 0);

						tmp = byteRead >> 3 & 0x03;
						sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
						sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + readSpeed;

						extractor->submitSequence(sequence, false);
					}
				}

				// Is it ready to extract? - takes 15-16ms to abort so take that into account
				if ((extractor->canExtract() || extractor->totalTimeReceived()>(220000000 - 14000000))) {
					if (!m_abortSignalled) abortReadStreaming();
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
		}
		else {
			readFail = 0;
		}
	}
}

// Read RAW data from the current track and surface HD mode
DiagnosticResponse ArduinoInterface::readCurrentTrack(void* trackData, const int dataLength, const bool readFromIndexPulse) {
	m_lastCommand = LastCommand::lcReadTrack;

	// Length must be one of the two types
	if (dataLength == RAW_TRACKDATA_LENGTH_DD && m_isHDMode) {
		m_lastError = DiagnosticResponse::drMediaTypeMismatch;
		return m_lastError;
	}
	if (dataLength == RAW_TRACKDATA_LENGTH_HD && !m_isHDMode) {
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
			if (bytesAvailable > sizeof tempReadBuffer) bytesAvailable = sizeof tempReadBuffer;
			unsigned long bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);

			for (size_t a = 0; a < bytesRead; a++) {
				if (m_abortSignalled) {
					// Make space
					for (int s = 0; s < 4; s++) slidingWindow[s] = slidingWindow[s + 1];
					// Append the new byte
					slidingWindow[4] = tempReadBuffer[a];

					// Watch the sliding window for the pattern we need
					if (slidingWindow[0] == 'X' && slidingWindow[1] == 'Y' && slidingWindow[2] == 'Z' && slidingWindow[3] == SPECIAL_ABORT_CHAR && slidingWindow[4] == '1') {
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
						tmp2 = tempReadBuffer[a] >> i & 0x03;
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

		// Keep reading until he hit dataLength or a null byte is received
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


#ifndef _WIN32
#define USE_THREADDED_READER
#endif

// Reads a complete rotation of the disk, and returns it using the callback function which can return FALSE to stop
// An instance of RotationExtractor is required.  This is purely to save on re-allocations.  It is internally reset each time
DiagnosticResponse ArduinoInterface::readRotation(MFMExtractionTarget& extractor, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns, std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation, bool useHalfPLL) {
	m_lastCommand = LastCommand::lcReadTrackStream;

	if (m_version.major == 1 && m_version.minor < 8) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}

	// Who would do this, right?
	if (maxOutputSize < 1 || !firstOutputBuffer) {
		m_lastError = DiagnosticResponse::drError;
		return m_lastError;
	}

	const bool highPrecisionMode = !m_isHDMode && m_version.deviceFlags1 & FLAGS_HIGH_PRECISION_SUPPORT;
	char mode = highPrecisionMode ? COMMAND_READTRACKSTREAM_HIGHPRECISION : COMMAND_READTRACKSTREAM;

	if (mode == COMMAND_READTRACKSTREAM_HIGHPRECISION && m_version.deviceFlags1 & FLAGS_FLUX_READ && useHalfPLL) mode = COMMAND_READTRACKSTREAM_HALFPLL;

	
	m_lastError = runCommand(mode);

	if (m_lastError != DiagnosticResponse::drOK) return m_lastError;

	m_isStreaming = true;


#ifdef USE_THREADDED_READER
	std::mutex safetyLock;
	// I was using a deque, but its much faster to just keep switching between two vectors
	std::vector<unsigned char> readBuffer;
	readBuffer.reserve(4096);
	std::vector<unsigned char> tempReadBuffer;
	tempReadBuffer.reserve(4096);
#ifdef _WIN32
	m_comPort.setReadTimeouts(100, 0);  // match linux
#endif
	std::thread* backgroundReader = new std::thread([this, &readBuffer, &safetyLock]() {
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif
		unsigned char buffer[1024];  // the LINUX serial buffer is only 512 bytes anyway
		while (m_isStreaming) {
			uint32_t waiting = m_comPort.justRead(buffer, 1024);
			if (waiting>0) {
				safetyLock.lock();				
				readBuffer.insert(readBuffer.end(), buffer, buffer+waiting);
				safetyLock.unlock();
			} else
				std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
	});


#else
	applyCommTimeouts(true);
	unsigned char tempReadBuffer[2048] = { 0 };
#endif

	// Let the class know we're doing some streaming stuff
	m_abortStreaming = false;
	m_abortSignalled = false;

	// Number of times we failed to read anything
	int32_t readFail = 0;

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

	for (;;) {

		// More efficient to read several bytes in one go		
#ifdef USE_THREADDED_READER
		tempReadBuffer.resize(0); // should be just as fast as clear, but just in case
		safetyLock.lock();
		std::swap(tempReadBuffer, readBuffer);
		safetyLock.unlock();

		unsigned int bytesRead = tempReadBuffer.size();
		
		if (bytesRead < 1) std::this_thread::sleep_for(std::chrono::milliseconds(20)); 
		for (const unsigned char byteRead : tempReadBuffer) {
#else
		unsigned int bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof tempReadBuffer) bytesAvailable = sizeof tempReadBuffer;
		unsigned int bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);
		for (size_t a = 0; a < bytesRead; a++) {
			const unsigned char byteRead = tempReadBuffer[a];
#endif
			if (m_abortSignalled) {
				// Make space
				for (int s = 0; s < 4; s++) slidingWindow[s] = slidingWindow[s + 1];
				// Append the new byte
				slidingWindow[4] = byteRead;

				// Watch the sliding window for the pattern we need
				if (slidingWindow[0] == 'X' && slidingWindow[1] == 'Y' && slidingWindow[2] == 'Z' && slidingWindow[3] == SPECIAL_ABORT_CHAR && slidingWindow[4] == '1') {
					m_isStreaming = false;
#ifdef USE_THREADDED_READER
					if (backgroundReader) {
						if (backgroundReader->joinable()) backgroundReader->join();
						delete backgroundReader;
					}
#endif
					m_comPort.purgeBuffers();
					m_lastError = timeout ? DiagnosticResponse::drError : DiagnosticResponse::drOK;
					applyCommTimeouts(false);
					return m_lastError;
				}
			}
			else {
				unsigned char tmp;
				RotationExtractor::MFMSequenceInfo sequence{};
				if (m_isHDMode) {
					for (int i = 6; i >= 0; i -= 2) {
						tmp = byteRead >> i & 0x03;
						sequence.mfm = (RotationExtractor::MFMSequence)((tmp == 0x03 ? 0 : tmp)+1);
						sequence.timeNS = 2000 + (unsigned int)sequence.mfm * 2000;
						extractor.submitSequence(sequence, tmp == 0x03);
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

								if (mode == COMMAND_READTRACKSTREAM_HALFPLL) {
									const int32_t sequences[4] = { (mfmSequences >> 6) & 0x03, (mfmSequences >> 4) & 0x03, (mfmSequences >> 2) & 0x03, mfmSequences & 0x03 };

									// First remove any flux time that us 000 as thats always fixed
									int32_t readSpeed = (unsigned int)(byteRead & 0x7F) * 2000 / 128;

									// Output the fluxes
									for (int a = 0; a < 4; a++) {
										if (sequences[a] == 0) {
											sequence.mfm = RotationExtractor::MFMSequence::mfm000;
											sequence.timeNS = 5000 + readSpeed;
										}
										else {
											sequence.mfm = (RotationExtractor::MFMSequence)sequences[a];
											sequence.timeNS = 1000 + readSpeed + (((int)sequence.mfm) * 2000);
										}
										sequence.pllTimeNS = sequence.timeNS;
										extractor.submitSequence(sequence, ((byteRead & 0x80) != 0) && (a==0));
									}

								}
								else {
									int32_t readSpeed = (unsigned int)(byteRead & 0x7F) * 2000 / 128;
									tmp = mfmSequences >> 6 & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor.submitSequence(sequence, (byteRead & 0x80) != 0);

									tmp = mfmSequences >> 4 & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor.submitSequence(sequence, false);

									tmp = mfmSequences >> 2 & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor.submitSequence(sequence, false);

									tmp = mfmSequences & 0x03;
									sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
									sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + (sequence.mfm == RotationExtractor::MFMSequence::mfm000 ? -2000 : readSpeed);
									extractor.submitSequence(sequence, false);
								}

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
						unsigned short readSpeed = (unsigned long long)((unsigned int)((byteRead & 0x07) * 16) * 2000) / 128;

						// Now packet up the data in the format the rotation extractor expects it to be in
						tmp = byteRead >> 5 & 0x03;
						sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
						sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + readSpeed;

						extractor.submitSequence(sequence, (byteRead & 0x80) != 0);

						tmp = byteRead >> 3 & 0x03;
						sequence.mfm = tmp == 0 ? RotationExtractor::MFMSequence::mfm000 : (RotationExtractor::MFMSequence)(tmp);
						sequence.timeNS = 1000 + (unsigned int)sequence.mfm * 2000 + readSpeed;

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
					if (extractor.totalTimeReceived() > (m_isHDMode ? 1200000000U : 600000000U)) {
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
#ifdef USE_THREADDED_READER
				if (backgroundReader) {
					if (backgroundReader->joinable()) backgroundReader->join();
					delete backgroundReader;
				}
#endif
				applyCommTimeouts(false);
				return m_lastError;
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		else {
			readFail = 0;
		}
	}
}

// This is experiment and as such is not currently in use
#define MAX_FLUX_SIGNAL           31

// RAW Counter Values
#define MIN_FLUX_ALLOWED          48                      // in 62.5 time 
#define MAX_FLUX_ALLOWED          (MIN_FLUX_ALLOWED+61)   // in 62.5 time - comes out as '30'
#define MAX_FLUX_REPEAT           (MAX_FLUX_ALLOWED-7)    // in 62.5 time - comes out as '26'
#define FLUX_REPEAT_OFFSET        (MAX_FLUX_REPEAT-MIN_FLUX_ALLOWED)    // The amount MAX_FLUX_SIGNAL represents in clock ticks, which is 3625ns

// Reads a complete rotation of the disk, and returns it using the callback function which can return FALSE to stop
// An instance of PLL is required.  This is purely to save on re-allocations.  It is internally reset each time
DiagnosticResponse ArduinoInterface::readFlux(PLL::BridgePLL& pll, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns, std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation) {
	m_lastCommand = LastCommand::lcReadTrackStream;

	if (!(m_version.deviceFlags1 & FLAGS_FLUX_READ) || m_isHDMode) {
		// Fall back if not supported
		return readRotation(*pll.rotationExtractor(), maxOutputSize, firstOutputBuffer, startBitPatterns, onRotation, false);
	}

	// Who would do this, right?
	if (maxOutputSize < 1 || !firstOutputBuffer) {
		m_lastError = DiagnosticResponse::drError;
		return m_lastError;
	}

	m_lastError = runCommand(COMMAND_READTRACKSTREAM_FLUX);
	if (m_lastError != DiagnosticResponse::drOK)
		return m_lastError;
	
	// Let the class know we're doing some streaming stuff
	m_isStreaming = true;
	m_abortStreaming = false;
	m_abortSignalled = false;

	// Number of times we failed to read anything
	int readFail = 0;

	// Buffer to read into
	unsigned char tempReadBuffer[2048] = { 0 };

	// Sliding window for abort
	char slidingWindow[5] = { 0,0,0,0,0 };
	bool timeout = false;
	bool dataState = false;
	bool indexDetected = false;
	unsigned char byte1 = 0;
	applyCommTimeouts(true);

	uint32_t fluxSoFar = 0;

	pll.prepareExtractor(false, startBitPatterns);

	for (;;) {

		// More efficient to read several bytes in one go		
		unsigned long bytesAvailable, bytesRead = 0;		
		bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof tempReadBuffer) bytesAvailable = sizeof tempReadBuffer;
		bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);

		for (size_t a = 0; a < bytesRead; a++) {
			if (m_abortSignalled) {
				// Make space
				for (int s = 0; s < 4; s++) slidingWindow[s] = slidingWindow[s + 1];
				// Append the new byte
				slidingWindow[4] = tempReadBuffer[a];

				// Watch the sliding window for the pattern we need
				if (slidingWindow[0] == 'X' && slidingWindow[1] == 'Y' && slidingWindow[2] == 'Z' && slidingWindow[3] == SPECIAL_ABORT_CHAR && slidingWindow[4] == '1') {
					m_isStreaming = false;					
					m_comPort.purgeBuffers();
					m_lastError = timeout ? DiagnosticResponse::drError : DiagnosticResponse::drOK;
					applyCommTimeouts(false);
					return m_lastError;
				}
			}
			else {
				unsigned char byteRead = tempReadBuffer[a];

				if (dataState) {

					uint32_t flux[3];
					flux[0] = byte1 & 0x1F;
					flux[1] = (byte1 >> 5) | ((byteRead >> 2) & 0x18);
					flux[2] = byteRead & 0x1F;
					indexDetected |= (byteRead & 0x80) != 0;

					for (int a = 0; a < 3; a++) {
						switch (flux[a]) {
						case MAX_FLUX_SIGNAL:
							fluxSoFar += (uint32_t)(62.5f * FLUX_REPEAT_OFFSET);
							break;
						default:
							fluxSoFar += (uint32_t)((flux[a] * 2 + MIN_FLUX_ALLOWED) * 62.5f);
							pll.submitFlux(fluxSoFar, indexDetected);
							indexDetected = false;
							fluxSoFar = 0;
						}
					}

					dataState = false;
				}
				else {
					// in 'false' mode we get the flux data
					dataState = true;
					byte1 = byteRead;
				}


				// Is it ready to extract?
				if (pll.canExtract()) {
					unsigned int bits = 0;
					// Go!
					if (pll.extractRotation(firstOutputBuffer, bits, maxOutputSize)) {
						m_diskInDrive = true;

						if (!onRotation(&firstOutputBuffer, bits)) {
							// And if the callback says so we stop.
							abortReadStreaming();
						}
						// Always save this back
						pll.getIndexSequence(startBitPatterns);
					}
				}
				else {
					if (pll.totalTimeReceived() > (m_isHDMode ? 1200000000U : 600000000U)) {
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
		}
		else {
			readFail = 0;
		}
	}
}

// Stops the read streaming immediately and any data in the buffer will be discarded.
bool ArduinoInterface::abortReadStreaming() {
	if (m_version.major == 1 && m_version.minor < 8) {
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
		return bit & 1 ? 0 : 1;
	}

	int ret = buffer[pos] >> bit & 1;
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

	if (m_version.major == 1 && m_version.minor < 9) return DiagnosticResponse::drOldFirmware;

	// First step is we need to re-encode the supplied buffer into a packed format 
	// Each nybble looks like: wwxxyyzz, each group is a code which is the transition time

	// *4 is a worse case situation, ie: if everything was a 01.  The +128 is for any extra padding
	const unsigned int maxOutSize = numBytes * 4 + 16;
	unsigned char* outputBuffer = (unsigned char*)malloc(maxOutSize);

	if (!outputBuffer) {
		m_lastError = DiagnosticResponse::drSendParameterFailed;
		return m_lastError;
	}

	// Original data was written from MSB down to LSB
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
			} while ((sequence & 0x08) == 0 && pos < numBytes + 8);

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
		if (outputPos >= maxOutSize - 1) {
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

/* If we have a look at the previous and next three bits, assume the current is a '1', then these are the only valid combinations that could be allowed
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

// The precomp version of the above. Don't use the above function directly to write precomp mode, it wont work.  Data must be passed with an 0xAA each side at least
DiagnosticResponse ArduinoInterface::writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp) {
	m_lastCommand = LastCommand::lcWriteTrack;

	if (m_version.major == 1 && m_version.minor < 8) return DiagnosticResponse::drOldFirmware;

	if (m_isHDMode) return writeCurrentTrackHD(mfmData, numBytes, writeFromIndexPulse);

	// First step is we need to re-encode the supplied buffer into a packed format with precomp pre-calculated.
	// Each nybble looks like: xxyy
	// where xx is: 0=no precomp, 1=-ve, 2=+ve
	//		 yy is: 0: 4us,		1: 6us,		2: 8us,		3: 10us

	// *4 is a worse case situation, ie: if everything was a 01.  The +128 is for any extra padding
	const unsigned int maxOutSize = numBytes * 4 + 16;
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

	// Re-encode the data into our format and apply precomp.  The +8 is to ensure there's some padding around the edge which will come out as 010101 etc
	while (pos < numBytes) {
		*output = 0;

		for (int i = 0; i < 2; i++) {
			int b, count = 0;

			// See how many zero bits there are before we hit a 1
			do {
				b = readBit(mfmData, numBytes, pos, bit);
				sequence = ((sequence << 1) & 0x7F) | b;
				count++;
			} while ((sequence & 0x08) == 0 && pos < numBytes + 8);

			// Validate range
			if (count < 2) count = 2;  // <2 would be a 11 sequence, not allowed
			if (count > 5) count = 5;  // max we support 01, 001, 0001, 00001

			// Write to stream. Based on the rules above we apply some precomp
			int precomp = 0;
			if (usePrecomp) {
				const unsigned char BitSeq5 = (sequence & 0x3E);  // extract these bits 00111110 - bit 3 is the ACTUAL bit
				// The actual idea is that the magnetic fields will repel each other, so we push them closer hoping they will land in the correct spot!
				switch (BitSeq5) {
				case 0x28:     // xx10100x
					precomp = PRECOMP_ERLY;
					break;
				case 0x0A:     // xx00101x
					precomp = PRECOMP_LATE;
					break;
				default:
					precomp = PRECOMP_NONE;
					break;				
				}
			} else precomp = PRECOMP_NONE;

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
	if (m_version.major == 1 && m_version.minor < 8 && usePrecomp) {
		return DiagnosticResponse::drOldFirmware;
	}

	m_lastError = runCommand(m_isHDMode ? COMMAND_WRITETRACK : usePrecomp ? COMMAND_WRITETRACKPRECOMP : COMMAND_WRITETRACK);
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

	// HD doesn't need the length as the data is null terminated
	if (!m_isHDMode) {
		// Now we send the number of bytes we're planning on transmitting
		unsigned char b = numBytes >> 8;
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




// Removes all flux transitions from the current track
DiagnosticResponse ArduinoInterface::eraseFluxOnTrack() {
	m_lastCommand = LastCommand::lcEraseFlux;

	if (((m_version.major == 1) && (m_version.minor < 9)) || ((m_version.minor == 9) && (m_version.buildNumber < 18))) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}
	m_lastError = runCommand(COMMAND_ERASEFLUX);
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

#define FLUX_OFFSET				44     // Minimum timer value
#define FLUX_MULTIPLIER_TIME_DB	125    // Our flux writing resolution in nanoseconds
#define FLUX_MINIMUM_NS         (DWORD)(FLUX_OFFSET*62.5f)

#define FLUX_MINIMUM_DB		   (FLUX_MINIMUM_NS / FLUX_MULTIPLIER_TIME_DB)  // Minimum time in DB time

#define FLUX_NOFLUX_OFFSET      5    // Starting value when 'no flux' is written. 
#define FLUX_MINSAFE_OFFSET     5    // This is also the safe minimum per byte so we don't overrun the serial port

#define FLUX_MINIMUM_PER_8_NS   ((FLUX_MINIMUM_DB + FLUX_MINSAFE_OFFSET) * 8)		// Minimum time per 8 flux timings allowed in total in ns
#define FLUX_TIME_10000NS_DB    ((10000/FLUX_MULTIPLIER_TIME_DB)-FLUX_MINIMUM_DB)   // DB number for 10000ns (10us)

#define FLUX_REPEAT_BLANK_DB    29    // The highest single amount of delay before a flux. This DOES NOT include FLUX_MINIMUM_DB
#define FLUX_REPEAT_COUNTER     (FLUX_MINIMUM_DB+FLUX_MINSAFE_OFFSET)

#define FLUX_SPECIAL_CODE_BLANK 30    // This special code causes DB to skip 3125ns of time without a flux transition.
#define FLUX_SPECIAL_CODE_END   31    // This special code causes the writing to finish
#define FLUX_JITTER				2        // Amount of time taken off of the revolution time in case there's some jitter (x FLUX_MULTIPLIER_TIME)
#define BIT(x)					(1 << x)  // Quick mapping of a bit to bitmask for that bit
#define BITSET(byte, x)			(byte & BIT(x)) // Quick check of bit set
#define GET_BIT_IF_SET(byte,inputBit,outputBit) (BITSET(byte, inputBit)?BIT(outputBit):0)   // If inputBit in byte is set then returns outputBit as a mask

// Structure to store temporary groupings of flux before encoding
struct Times8 {
	union {
		uint8_t times[8];
		struct {
			uint8_t a, b, c, d, e, f, g, h;
		};
	};
	DWORD numUsed;
};

// Returns the TOTAL timing for a converted DB time value. If you multiply this by FLUX_MULTIPLIER_TIME_DB you get actual flux time in NS 
inline DWORD getDBTime(uint8_t dbTime) {
	if (dbTime <= FLUX_REPEAT_BLANK_DB) return dbTime + FLUX_MINIMUM_DB;
	return FLUX_NOFLUX_OFFSET + FLUX_MINIMUM_DB;
}

// Checks that the block will not get written so fast that we'll have an issue with the serial port.  This works in DB timescales
// Returns the number of extra timing values that were needed to be added to make this meet minimum requirements
DWORD validateBlock(Times8& block) {
	DWORD timingsAdjusted = 0;
	DWORD totalTime = 0;
	int numUnder = 0;
	for (size_t i = 0; i < block.numUsed; i++) {
		totalTime += getDBTime(block.times[i]);
		if (block.times[i] < FLUX_MINSAFE_OFFSET) numUnder++;
	}

	// Ok, an issue, so we'll have to fix some of the slower flux times, this isn't ideal
	if (totalTime < FLUX_MINIMUM_PER_8_NS) {
		DWORD fixAmountNeeded = (DWORD)ceil((float)(FLUX_MINIMUM_PER_8_NS - totalTime) / (float)numUnder);
		for (size_t i = 0; i < block.numUsed; i++)
			if (block.times[i] < FLUX_MINSAFE_OFFSET) {
				block.times[i] += (uint8_t)fixAmountNeeded;
				totalTime += fixAmountNeeded;
				timingsAdjusted++;
				// Only do what we have to
				if (totalTime >= FLUX_MINIMUM_PER_8_NS) break;
			}

		// Still too low?
		if (fixAmountNeeded) {
			for (size_t i = 0; i < block.numUsed; i++)
				if (block.times[i] < FLUX_REPEAT_BLANK_DB) {
					block.times[i] += (uint8_t)fixAmountNeeded;
					totalTime += fixAmountNeeded;
					timingsAdjusted++;
					// Only do what we have to
					if (totalTime >= FLUX_MINIMUM_PER_8_NS) break;
				}
		}
	}

	return timingsAdjusted;
}

// Append a fluxTime to a block. fluxTime is in DB time. timingsExtra is a running count of extra data that was needed to make blocks valid
void appendToBlock(DWORD fluxTime, DWORD& timingsExtra, Times8& currentBlock, std::vector<Times8>& output) {
	DWORD timingsAdjusted = 0;
	// Add extra blocks if this is longer than the 'don't write' repeat interval
	while (fluxTime > FLUX_REPEAT_BLANK_DB) {
		// Re-claim some time
		if (timingsExtra && fluxTime > FLUX_REPEAT_BLANK_DB + 1) {
			fluxTime--;
			timingsExtra--;
		}

		currentBlock.times[currentBlock.numUsed++] = FLUX_SPECIAL_CODE_BLANK;
		if (currentBlock.numUsed >= 8) {
			timingsAdjusted += validateBlock(currentBlock);
			output.push_back(currentBlock);
			currentBlock.numUsed = 0;
		}
		fluxTime -= FLUX_REPEAT_COUNTER;
	}

	// Re-claim some time
	if (timingsExtra && fluxTime > FLUX_MINSAFE_OFFSET) {
		fluxTime--;
		timingsExtra--;
	}

	// Don't forget the actual data
	currentBlock.times[currentBlock.numUsed++] = (uint8_t)fluxTime;
	if (currentBlock.numUsed >= 8) {
		timingsAdjusted += validateBlock(currentBlock);
		output.push_back(currentBlock);
		currentBlock.numUsed = 0;
	}
}

// Writes the flux timings (in nanoseconds) to the drive.  The Drive RPM is needed to compensate and correct the flux times.
DiagnosticResponse ArduinoInterface::writeFlux(const std::vector<DWORD>& fluxTimes, const DWORD offsetFromIndex, const float driveRPM, bool compensateFluxTimings, bool terminateAtIndex) {
	m_lastCommand = LastCommand::lcWriteFlux;

	if ((m_version.major == 1) && ((m_version.minor < 9) || ((m_version.minor == 9) && (m_version.buildNumber < 22)))) {
		m_lastError = DiagnosticResponse::drOldFirmware;
		return m_lastError;
	}
	if (m_isHDMode) {
		m_lastError = DiagnosticResponse::drMediaTypeMismatch;
		return m_lastError;
	}

	// Assume this is an unformatted track
	if (fluxTimes.empty()) {
		m_lastError = eraseFluxOnTrack();
		return m_lastError;
	}

	// Step 1: calculate the total flux length and convert into DB time values
	DWORD totalFluxTime = 0;
	bool existsOver10000 = false;
	bool existsUnderMinimum = false;
	std::vector<DWORD> dbTime;
	int hdStyleCount = 0;
	int ddStyleCount = 0;

	DWORD fluxTimeCounters[7] = { 0,0,0,0,0,0,0 };
	DWORD totalCounter = 0;

	for (DWORD t : fluxTimes) {
		if (t < 1000) continue;  // skip really fast ones
		if (t > 4500) ddStyleCount++;
		if (t < 3500) hdStyleCount++;
		DWORD tmp = (t + 1000) / 2000;
		if (t < FLUX_MINIMUM_NS)
			t = FLUX_MINIMUM_NS;
		t = (t - FLUX_MINIMUM_NS + FLUX_MULTIPLIER_TIME_DB / 2) / FLUX_MULTIPLIER_TIME_DB;
		dbTime.push_back(t);
		totalFluxTime += t + FLUX_MINIMUM_DB;
		if (t > FLUX_TIME_10000NS_DB) existsOver10000 = true;
		if (t < FLUX_MINSAFE_OFFSET) existsUnderMinimum = true;
		if (tmp < 7) {
			fluxTimeCounters[tmp]++;  // keep a counter of which types of flux are in the image
			totalCounter++;
		}
	}

	// We cant write this.
	if (hdStyleCount > ddStyleCount) {
		bool unformatted = true;

		// This picks up 'unformatted track' simulation output from HxC and replaces it with a proper unformatted track, if that's what it is.
		int percentageAllowed = 50;
		for (size_t i = 1; i < 6; i++) {
			int percentageOfFlux = fluxTimeCounters[i] * 100 / totalCounter;
			if ((int)abs(percentageOfFlux - percentageAllowed) <= (int)(percentageAllowed + (7 - i))) {
				// Within the allowed window of 'randomness' for unformatted
				percentageAllowed /= 2;
				continue;
			}
			else {
				// Out of range, probably not an unformatted track
				unformatted = false;
				break;
			}
		}

		// This is an unformatted track.  We can write that easily
		if (unformatted) {
			m_lastError = eraseFluxOnTrack();
			return m_lastError;
		}

		m_lastError = DiagnosticResponse::drMediaTypeMismatch;
		return m_lastError;
	}

	// Step 2: calculate the time taken for a full revolution in nanoseconds
	const uint64_t rpmNanoSeconds = (uint64_t)(60000000000.0f / driveRPM);
	// Convert it to DB time
	const uint64_t rpmInDBTime = rpmNanoSeconds / FLUX_MULTIPLIER_TIME_DB - FLUX_JITTER;

	// Step 3: Make the data fit the revolution
	if (compensateFluxTimings) {
		if (totalFluxTime < rpmInDBTime - 100) {
			// No, not really. First, lets increase all the really slow pulses under FLUX_MINSAFE_OFFSET
			while (totalFluxTime < rpmInDBTime - 100) {
				bool madeChanges = false;
				for (DWORD& t : dbTime) {
					if (t < FLUX_MINSAFE_OFFSET) {
						t++;
						totalFluxTime++;
						madeChanges = true;

						if (t < FLUX_MINSAFE_OFFSET) existsUnderMinimum = true;
						if (totalFluxTime >= rpmInDBTime - 100) break;
					}
				}
				if (!existsUnderMinimum) break;
				if (!madeChanges) break;
			}			
		}
		else {
			if (totalFluxTime > rpmInDBTime - 10) {
				// Do we have too much data?  First, lets shorten some of the really long flux transitions, ie: over 10000ns
				if (existsOver10000) {
					while (totalFluxTime >= rpmInDBTime) {
						existsOver10000 = false;
						for (DWORD& t : dbTime) {
							if (t > FLUX_TIME_10000NS_DB) {
								t--;
								totalFluxTime--;

								if (t > FLUX_TIME_10000NS_DB) existsOver10000 = true;
								if (totalFluxTime < rpmInDBTime - 10) break;
							}
						}
						if (!existsOver10000) break;
					}
				}

				// Are we still over?
				if (totalFluxTime >= rpmInDBTime) {
					// Drop every sample over FLUX_MINSAFE_OFFSET down by one
					for (DWORD& t : dbTime) {
						if (t > FLUX_MINSAFE_OFFSET) {
							t--;
							totalFluxTime--;
							if (totalFluxTime < rpmInDBTime) break;
						}
					}
				}
			}
		}
	}

	// We now should have data that approx matches a revolution of disk data. Lets convert it into DB packets.
	// Step 4: Group into blocks of 8 flux times that do not exceed the rules of that block
	std::vector<Times8> fluxTimesGrouped;
	Times8 block;
	block.numUsed = 0;
	DWORD timingsOver = 0;
	uint8_t firstFlux;

	// Extract FirstFlux.  This is a special one just to kick-start the process
	if (dbTime[0] > FLUX_REPEAT_BLANK_DB) {
		dbTime[0] -= FLUX_REPEAT_COUNTER;
		firstFlux = FLUX_SPECIAL_CODE_BLANK;
	}
	else {
		firstFlux = (uint8_t)dbTime[0];
		dbTime.erase(dbTime.begin());
	}

	for (const DWORD& t : dbTime) appendToBlock(t, timingsOver, block, fluxTimesGrouped);
	// Don't forget the final block
	if (block.numUsed) {
		// Add the special break codes
		for (int i = block.numUsed; i < 8; i++) block.times[i] = FLUX_SPECIAL_CODE_END;
		timingsOver += validateBlock(block);
		fluxTimesGrouped.push_back(block);
	}
	else {
		// Add a block with the BREAK code in it
		block.numUsed = 8;
		for (int i = 0; i < 8; i++) block.times[i] = FLUX_SPECIAL_CODE_END;
		fluxTimesGrouped.push_back(block);
	}

	// Hmm, this could be an issue
	if (timingsOver >= FLUX_JITTER && compensateFluxTimings) {
		// We'll look at the blocks of 8, and see if there's any we can shorten.  Its rare though
		for (Times8& block : fluxTimesGrouped) {
			DWORD total = 0;
			// See how long this block is
			for (size_t a = 0; a < block.numUsed; a++) total += getDBTime(block.times[a]);
			// Does it have some "space"
			if (total > FLUX_MINIMUM_PER_8_NS) {
				// Re-claim from this
				for (size_t a = 0; a < block.numUsed; a++) {
					if (block.times[a] && block.times[a] <= FLUX_REPEAT_BLANK_DB) {
						total--;
						block.times[a]--;
						timingsOver--;
						if (!total) break;
						if (!timingsOver) break;
					}
				}
			}
			// Stop if we succeeded
			if (!timingsOver) break;
		}
	}

	// Now convert the blocks of 8, into packed data of 5 bytes for DB according to the following schema:
	//    Bit :  7   6   5   4   3   2   1   0  
	// Byte 1 : D4  C4  B4  A4  A3  A2  A1  A0
	// Byte 2 : C3  C2  C1  C0  B3  B2  B1  B0
	// Byte 3 : E3  E2  E1  E0  D3  D2  D1  D0
	// Byte 4 : E4  H4  G4  F4  F3  F2  F1  F0
	// Byte 5 : H3  H2  H1  H0  G3  G2  G1  G0
	std::vector<uint8_t> flux;
	flux.push_back(firstFlux);   // special value to get it started

	for (const Times8& block : fluxTimesGrouped) {
		flux.push_back((block.a & 0x1F) | GET_BIT_IF_SET(block.b, 4, 5) | GET_BIT_IF_SET(block.c, 4, 6) | GET_BIT_IF_SET(block.d, 4, 7));
		flux.push_back((block.b & 0x0F) | ((block.c & 0x0F) << 4));
		flux.push_back((block.d & 0x0F) | ((block.e & 0x0F) << 4));
		flux.push_back((block.f & 0x1F) | GET_BIT_IF_SET(block.g, 4, 5) | GET_BIT_IF_SET(block.h, 4, 6) | GET_BIT_IF_SET(block.e, 4, 7));
		flux.push_back((block.g & 0x0F) | ((block.h & 0x0F) << 4));
	}

	m_lastError = runCommand(COMMAND_WRITEFLUX);
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

	// Send the offset from index in ARDUINO ticks
	uint32_t t = (uint32_t)ceil((float)offsetFromIndex / (driveRPM / 300.0f) / 62.5f);
	// Send the three bytes containing this.
	unsigned char amount[4];
	amount[0] = t & 0xFF;
	amount[1] = t >> 8 & 0xFF;
	amount[2] = t >> 16 & 0xFF;
	amount[3] = terminateAtIndex ? 1 : 0;   // flags
	if (!deviceWrite((const void*)amount, 4)) {
		m_lastError = DiagnosticResponse::drSendDataFailed;
		return m_lastError;
	}

	// Send the actual data
	if (!deviceWrite((const void*)flux.data(), (unsigned int)flux.size())) {
		m_lastError = DiagnosticResponse::drSendDataFailed;
		return m_lastError;
	}

	unsigned char response;
	if (!deviceRead(&response, 1, true)) {
		m_lastError = DiagnosticResponse::drTrackWriteResponseError;
		return m_lastError;
	}

	// If this is a '1' then the Arduino didn't miss a single bit!
	if ((response != '1') && (response != 'I')) {
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


// Returns a list of ports this could be available on
void ArduinoInterface::enumeratePorts(std::vector<std::wstring>& portList) {
	portList.clear();
	std::vector<SerialIO::SerialPortInformation> prts;

	SerialIO prt;
	prt.enumSerialPorts(prts);

	for (const SerialIO::SerialPortInformation& port : prts) {
		// Skip CH340 boards
		if ((port.vid == 0x1a86) && (port.pid == 0x7523)) continue;

		// Skip any Greaseweazle boards 
		if ((port.vid == 0x1209) && (port.pid == 0x4d69)) continue;
		if ((port.vid == 0x1209) && (port.pid == 0x0001)) continue;
		if (port.productName == L"Greaseweazle") continue;
		if (port.instanceID.find(L"\\GW") != std::wstring::npos) continue;

		// Skip Supercard pro
		if (port.portName.find(L"SCP-JIM") != std::wstring::npos) continue;
		if (port.portName.find(L"Supercard Pro") != std::wstring::npos) continue;

		portList.push_back(port.portName);
	}
}

// Reset reason information
DiagnosticResponse ArduinoInterface::getResetReason(bool& WD, bool& BOD, bool& ExtReset, bool& PowerOn) {
	if ((m_version.major > 1) || ((m_version.major == 1) && (m_version.minor > 9)) || ((m_version.major == 1) && (m_version.minor == 9) && (m_version.buildNumber >= 26))) {
		// TODO
		return DiagnosticResponse::drOldFirmware;
	}
	return DiagnosticResponse::drOldFirmware;
}
DiagnosticResponse ArduinoInterface::clearResetReason() {
	if ((m_version.major > 1) || ((m_version.major == 1) && (m_version.minor > 9)) || ((m_version.major == 1) && (m_version.minor == 9) && (m_version.buildNumber >= 26))) {
		// TODO
		return DiagnosticResponse::drOldFirmware;
	}
	return DiagnosticResponse::drOldFirmware;
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
		char* target2 = (char*)target + read;
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

	enabled = (ret[0] == 0x52) && (ret[1] == 0x6F) && (ret[2] == 0x62) && (ret[3] == 0x53);
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_IsDrawbridgePlusMode(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a + 4, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = (ret[0] == 0x2B) && (ret[1] == 0xB2);
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}


DiagnosticResponse ArduinoInterface::eeprom_IsDensityDetectDisabled(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a + 6, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = (ret[0] == 0x44) && (ret[1] == 0x53);
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_IsSlowSeekMode(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a + 8, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = (ret[0] == 0x53) && (ret[1] == 0x77);
	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}

DiagnosticResponse ArduinoInterface::eeprom_IsIndexAlignMode(bool& enabled) {
	unsigned char ret[2];

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromRead(a + 10, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	enabled = (ret[0] == 0x69) && (ret[1] == 0x61);
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


DiagnosticResponse ArduinoInterface::eeprom_SetIndexAlignMode(bool enabled) {
	unsigned char ret[2];

	if (enabled) {
		ret[0] = 0x69;
		ret[1] = 0x61;
	}
	else memset(ret, 0, 2);

	for (int a = 0; a < 2; a++) {
		DiagnosticResponse r = eepromWrite(a + 10, ret[a]);
		if (r != DiagnosticResponse::drOK) return r;
	}

	m_lastError = DiagnosticResponse::drOK;
	return m_lastError;
}


