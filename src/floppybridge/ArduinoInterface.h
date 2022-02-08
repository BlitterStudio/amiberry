#ifndef ARDUINO_FLOPPY_READER_WRITER_INTERFACE
#define ARDUINO_FLOPPY_READER_WRITER_INTERFACE
/* ArduinoFloppyReaderWriter aka DrawBridge
*
* Copyright (C) 2017-2022 Robert Smith (@RobSmithDev)
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
// Class to manage the communication between the computer and the Arduino    V2.7     //
////////////////////////////////////////////////////////////////////////////////////////
//
// Purpose:
// The class handles the command interface to the arduino.  It doesn't do any decoding
// Just open ports, switch motors on and off, seek to tracks etc.
//
//
//

#include <string>
#include <functional>
#include <vector>
#include <mutex>

#include "RotationExtractor.h"
#include "pll.h"
#include "SerialIO.h"

// Paula on the Amiga used to find the SYNC then read 1900 WORDS. (12868 bytes)
// As the PC is doing the SYNC we need to read more than this to allow a further overlap
// This number must match what the sketch in the Arduino is set to. 
#define RAW_TRACKDATA_LENGTH_DD (0x1900*2+0x440)
#define RAW_TRACKDATA_LENGTH_HD (2*RAW_TRACKDATA_LENGTH_DD)
// With the disk spinning at 300rpm, and data rate of 500kbps, for a full revolution we should receive 12500 bytes of data (12.5k)
// The above buffer assumes a full Paula data capture plus the size of a sector.


#define FLAGS_HIGH_PRECISION_SUPPORT   (1 << 0)
#define FLAGS_DISKCHANGE_SUPPORT	   (1 << 1)
#define FLAGS_DRAWBRIDGE_PLUSMODE	   (1 << 2)
#define FLAGS_DENSITYDETECT_ENABLED    (1 << 3)
#define FLAGS_SLOWSEEKING_MODE		   (1 << 4)
#define FLAGS_INDEX_ALIGN_MODE		   (1 << 5)
#define FLAGS_FLUX_READ				   (1 << 6)
#define FLAGS_FIRMWARE_BETA            (1 << 7)

namespace ArduinoFloppyReader {

	// Array to hold data from a floppy disk read
	typedef unsigned char RawTrackDataDD[RAW_TRACKDATA_LENGTH_DD];
	typedef unsigned char RawTrackDataHD[RAW_TRACKDATA_LENGTH_HD];

	// Sketch firmware version
	struct FirmwareVersion {
		unsigned char major, minor;
		bool fullControlMod;

		// Extra in V1.9
		unsigned char deviceFlags1;
		unsigned char deviceFlags2;
		unsigned char buildNumber;
	};

	// Represent which side of the disk we're looking at
	enum class DiskSurface {
		dsUpper,            // The upper side of the disk
		dsLower             // The lower side of the disk
	};

	// Speed at which the head will seek to a track
	enum class TrackSearchSpeed {
		tssSlow = 0,
		tssNormal = 1,
		tssFast = 2,
		tssVeryFast = 3
	};

	// Diagnostic responses from the interface
	enum class DiagnosticResponse {
		drOK,

		// Responses from openPort
		drPortInUse,
		drPortNotFound,
		drPortError,
		drAccessDenied,
		drComportConfigError,
		drBaudRateNotSupported,
		drErrorReadingVersion,
		drErrorMalformedVersion,
		drOldFirmware,

		// Responses from commands
		drSendFailed,
		drSendParameterFailed,
		drReadResponseFailed,
		drWriteTimeout,
		drSerialOverrun,
		drFramingError,
		drError,

		// Response from selectTrack
		drTrackRangeError,
		drSelectTrackError,
		drWriteProtected,
		drStatusError,
		drSendDataFailed,
		drTrackWriteResponseError,

		// Returned if there is no disk in the drive
		drNoDiskInDrive,

		drDiagnosticNotAvailable,
		drUSBSerialBad,
		drCTSFailure,
		drRewindFailure,

		drMediaTypeMismatch
	};

	enum class LastCommand {
		lcOpenPort,
		lcGetVersion,
		lcEnableWrite,
		lcRewind,
		lcDisableMotor,
		lcEnableMotor,
		lcGotoTrack,
		lcSelectSurface,
		lcReadTrack,
		lcWriteTrack,
		lcRunDiagnostics,
		lcSwitchDiskMode,
		lcReadTrackStream,
		lcCheckDiskInDrive,
		lcCheckDiskWriteProtected,
		lcEraseTrack,
		lcNoClickCheck,
		lcCheckDensity,
		lcMeasureRPM,
		lcEEPROMRead,
		lcEEPROMWrite,
		lcWriteFlux,
		lcEraseFlux
	};

	class ArduinoInterface {
	private:
		// Windows handle to the serial port device
		SerialIO		m_comPort;
		FirmwareVersion m_version;
		bool			m_inWriteMode;
		LastCommand		m_lastCommand;
		DiagnosticResponse m_lastError;
		bool			m_abortStreaming;
		bool			m_isWriteProtected;
		bool			m_diskInDrive;
		bool			m_abortSignalled;
		bool			m_isStreaming;
		bool			m_isHDMode;
		std::mutex		m_protectAbort;

		// Read a desired number of bytes into the target pointer
		bool deviceRead(void* target, const unsigned int numBytes, const bool failIfNotAllRead = false);
		// Writes a desired number of bytes from the the pointer
		bool deviceWrite(const void* source, const unsigned int numBytes);

		// Version of the above where the command has a parameter on the end (as long as its not char 0)
		DiagnosticResponse runCommand(const char command, const char parameter = '\0', char* actualResponse = nullptr);

		// Apply and change the timeouts on the com port
		void applyCommTimeouts(bool shortTimeouts);

		// Attempts to write a sector back to the disk.  This must be pre-formatted and MFM encoded correctly depending on usePrecomp
		DiagnosticResponse internalWriteTrack(const unsigned char* data, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp);

		// Attempts to verify if the reader/writer is running on this port
		static DiagnosticResponse internalOpenPort(const std::wstring& portName, bool enableCTSflowcontrol, bool triggerReset, std::string& versionString, SerialIO& port);

		// Attempt to sync and get version
		static DiagnosticResponse attemptToSync(std::string& versionString, SerialIO& port);

		// HD - a bit like the precomp as its more accurate, but no precomp
		DiagnosticResponse writeCurrentTrackHD(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse);

		// Read from the EEPROM
		DiagnosticResponse eepromRead(unsigned char position, unsigned char& value);

		// Write to the eeprom
		DiagnosticResponse eepromWrite(unsigned char position, unsigned char value);
	public:
		// Constructor for this class
		ArduinoInterface();

		// Free me
		~ArduinoInterface();

		const LastCommand getLastFailedCommand() const { return m_lastCommand; }
		const DiagnosticResponse getLastError() const { return m_lastError; }

		// Uses the above fields to constructor a suitable error message, hopefully useful in diagnosing the issue
		const std::string getLastErrorStr() const;

		const bool isOpen() const { return m_comPort.isPortOpen(); }
		const bool isInWriteMode() const { return m_inWriteMode; }

		// Returns a list of ports this could be available on
		static void enumeratePorts(std::vector<std::wstring>& portList);

		// Returns TRUE if there is a disk in the drive.   This is ONLY updated by checkForDisk or checkIfDiskIsWriteProtected
		bool isDiskInDrive() const { return m_diskInDrive; }

		// Get the current firmware version.  Only valid if openPort is successful
		const FirmwareVersion getFirwareVersion() const { return m_version; }

		// Turns on and off the reading interface.  For the new modded firmware this also allows writing as such the function below is no longer needed
		DiagnosticResponse enableReading(const bool enable, const bool reset = true, const bool dontWait = false);

		// Turns on and off the reading interface. If irError is returned the disk is write protected.  This is no longer needed if you are using the new modded firmware
		DiagnosticResponse  enableWriting(const bool enable, const bool reset = true);

		// Attempts to open the reader running on the COM port number provided.  Port MUST support 2M baud
		DiagnosticResponse openPort(const std::wstring& portName, bool enableCTSflowcontrol = true);

		// Attempts to verify if the reader/writer is running on this port
		static bool isPortCorrect(const std::wstring& portName);

		// Checks if the disk is write protected.  If forceCheck=false then the last cached version is returned.  This is also updated by checkForDisk() as well as this function
		DiagnosticResponse checkIfDiskIsWriteProtected(bool forceCheck);

		// Seek to track 0
		DiagnosticResponse findTrack0();

		// Check to see if a disk is inserted in the drive. If forceCheck then the last cached version is returned, which is also updated by diskStepOnce.
		DiagnosticResponse checkForDisk(bool forceCheck);

		// Reads a complete rotation of the disk, and returns it using the callback function which can return FALSE to stop
		// An instance of BridgePLL is required.  
		DiagnosticResponse readRotation(RotationExtractor& extractor, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns, std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation, bool useHalfPLL);
		// Same as the above, but this uses the newer much more accurate flux read
		DiagnosticResponse readFlux(PLL::BridgePLL& pll, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns, std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation);


		// Stops the read streaming immediately and any data in the buffer will be discarded. The above function will exit when the Arduino has also stopped streaming data
		bool abortReadStreaming();

		// Returns TRUE if the disk si currently streaming data
		bool isStreaming() { return m_isStreaming; }

		// Check if an index pulse can be detected from the drive
		DiagnosticResponse testIndexPulse();

		// Check if data can be detected from the drive
		DiagnosticResponse testDataPulse();

		// Query the RPM of the drive
		DiagnosticResponse measureDriveRPM(float& rpm);

		// Checks to see what the density of the current disk is most likely to be, Ideally this should be done while at track 0, probably lower head
		DiagnosticResponse checkDiskCapacity(bool& isHD);

		// Check and switch to HD disk
		DiagnosticResponse setDiskCapacity(bool switchToHD_Disk);

		// Guess if the drive is actually a PLUS Mode drive
		DiagnosticResponse guessPlusMode(bool& isProbablyPlus);

		// Select the track, this makes the motor seek to this position
		DiagnosticResponse  selectTrack(const unsigned char trackIndex, const TrackSearchSpeed searchSpeed = TrackSearchSpeed::tssNormal, bool ignoreDiskInsertCheck = false);

		// If the drive is on track 0, this does a test seek to -1 if supported
		DiagnosticResponse performNoClickSeek();

		// Choose which surface of the disk to read from
		DiagnosticResponse  selectSurface(const DiskSurface side);

		// Read RAW data from the current track and surface selected - this works properly with the HD and DD options
		// dataLength must be either RAW_TRACKDATA_LENGTH or RAW_TRACKDATA_LENGTH_HD.  If you mismatch the type then the function will return an error
		DiagnosticResponse  readCurrentTrack(void* trackData, const int dataLength, const bool readFromIndexPulse);

		// Read RAW data from the current track and surface selected in DD
		DiagnosticResponse  readCurrentTrack(RawTrackDataDD& trackData, const bool readFromIndexPulse);

		// Attempts to write a track back to the disk.  This must be pre-formatted and MFM encoded correctly
		DiagnosticResponse  writeCurrentTrack(const unsigned char* data, const unsigned short numBytes, const bool writeFromIndexPulse);
		// The precomp version of the above. 
		DiagnosticResponse  writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp);

		// Writes the flux timings (in nanoseconds) to the drive.  The Drive RPM is needed tp compensate and correct the flux times (if enabled).
		DiagnosticResponse writeFlux(const std::vector<DWORD>& fluxTimes, const DWORD offsetFromIndex, const float driveRPM, bool compensateFluxTimings, bool terminateAtIndex = false);

		// Removes all flux transitions from the current track
		DiagnosticResponse eraseFluxOnTrack();

		// Erases the current track by writing 0xAA to it
		DiagnosticResponse eraseCurrentTrack();

		// Check CTS status - you must open with CTS disabled for this to work
		DiagnosticResponse testCTS();

		// Check if the USB to Serial device can keep up properly
		DiagnosticResponse testTransferSpeed();

		// Returns true if the track actually contains some data, else its considered blank or unformatted
		bool trackContainsData(const RawTrackDataDD& trackData) const;

		// Closes the port down
		void closePort();

		// Check the EEPROM setting for advanced controller
		DiagnosticResponse eeprom_IsAdvancedController(bool& enabled);
		DiagnosticResponse eeprom_IsDrawbridgePlusMode(bool& enabled);
		DiagnosticResponse eeprom_IsDensityDetectDisabled(bool& enabled);
		DiagnosticResponse eeprom_IsSlowSeekMode(bool& enabled);
		DiagnosticResponse eeprom_IsIndexAlignMode(bool& enabled);

		// Check the EEPROM setting for advanced controller
		DiagnosticResponse eeprom_SetAdvancedController(bool enabled);
		DiagnosticResponse eeprom_SetDrawbridgePlusMode(bool enabled);
		DiagnosticResponse eeprom_SetDensityDetectDisabled(bool enabled);
		DiagnosticResponse eeprom_SetSlowSeekMode(bool enabled);
		DiagnosticResponse eeprom_SetIndexAlignMode(bool enabled);
	};
};


#endif