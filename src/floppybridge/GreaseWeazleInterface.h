#ifndef GREASEWEAZLE_INTERFACE
#define GREASEWEAZLE_INTERFACE
/* GreaseWeazle C++ Interface for reading and writing Amiga Disks
*
* By Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
* 
* Based on the excellent code by Keir Fraser <keir.xen@gmail.com>
* https://github.com/keirf/Greaseweazle/
* 
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

#include <string>
#include <functional>
#include <vector>
#include "RotationExtractor.h"
#include "SerialIO.h"

// Paula on the Amiga used to find the SYNC then read 1900 WORDS. (12868 bytes)
// As the PC is doing the SYNC we need to read more than this to allow a further overlap
// This number must match what the sketch in the Arduino is set to. 
#define RAW_TRACKDATA_LENGTH    (0x1900*2+0x440)
// With the disk spinning at 300rpm, and data rate of 500kbps, for a full revolution we should receive 12500 bytes of data (12.5k)
// The above buffer assumes a full Paula data capture plsu the size of a sector.

namespace GreaseWeazle {

	// Array to hold data from a floppy disk read
	typedef unsigned char RawTrackData[RAW_TRACKDATA_LENGTH];

	// Represent which side of the disk we're looking at
	enum class DiskSurface {
		dsUpper,            // The upper side of the disk
		dsLower             // The lower side of the disk
	};

	// Speed at which the head will seek to a track
	enum class TrackSearchSpeed {
		tssSlow,
		tssNormal,
		tssFast,
		tssVeryFast
	};

	// Command set
	enum class Cmd {
		GetInfo = 0,
		Update = 1,
		Seek = 2,
		Head = 3,
		SetParams = 4,
		GetParams = 5,
		Motor = 6,
		ReadFlux = 7,
		WriteFlux = 8,
		GetFluxStatus = 9,
		GetIndexTimes = 10,
		SwitchFwMode = 11,
		Select = 12,
		Deselect = 13,
		SetBusType = 14,
		SetPin = 15,
		Reset = 16,
		EraseFlux = 17,
		SourceBytes = 18,
		SinkBytes = 19,
		GetPin = 20
	};

	// Command responses/acknowledgements
	enum class Ack {
		Okay = 0,
		BadCommand = 1,
		NoIndex = 2,
		NoTrk0 = 3,
		FluxOverflow = 4,
		FluxUnderflow = 5,
		Wrprot = 6,
		NoUnit = 7,
		NoBus = 8,
		BadUnit = 9,
		BadPin = 10,
		BadCylinder = 11
	};


	// Diagnostic responses from the interface
	enum class GWResponse {
		drOK,

		// Responses from openPort
		drPortInUse,
		drPortNotFound,
		drPortError,
		drAccessDenied,
		drComportConfigError,
		drErrorMalformedVersion,
		drOldFirmware,
		drInUpdateMode,

		// Responses from commands
		drReadResponseFailed,
		drSerialOverrun,
		drError,

		// Response from selectTrack
		drTrackRangeError,
		drSelectTrackError,
		drWriteProtected,

		// Returned if there is no disk in the drive
		drNoDiskInDrive,

		drRewindFailure,
	};


#pragma pack(1)
	// <4BI3B21x
	struct GWVersionInformation  {
		unsigned char major, minor, is_main_firmware, max_cmd;
		unsigned int sample_freq;   // sample_freq * 1e-6 = Mhz
		unsigned char hw_model, hw_submodel, usb_speed;

		unsigned char padding[21];
	};

	// <5H
	struct GWDriveDelays {
		unsigned short select_delay, step_delay;			// in uSec
		unsigned short seek_settle_delay, motor_delay, watchdog_delay;  // in mSec
	};

	// ## Cmd.SetBusType values
	enum class BusType { Invalid = 0, IBMPC = 1, Shugart = 2 };
		
#pragma pack()



	class GreaseWeazleInterface {
	private:
		// Windows handle to the serial port device
		SerialIO		m_comPort;
		BusType			m_currentBusType;
		unsigned char	m_currentDriveIndex;
		bool			m_diskInDrive;
		bool			m_motorIsEnabled;
		bool			m_shouldAbortReading = false;
		bool			m_pinDskChangeAvailable = false;
		bool			m_pinWrProtectAvailable = false;
		bool			m_isWriteProtected = false;

		// Version information read during openPort
		GWVersionInformation m_gwVersionInformation;

		// Delay settings
		GWDriveDelays m_gwDriveDelays;

		// Apply and change the timeouts on the com port
		void applyCommTimeouts(bool shortTimeouts);

		// send a command out to the GW and receive its response.  Returns FALSE on error
		bool sendCommand(Cmd command, const std::vector<unsigned char>& params, Ack& response, unsigned char extraResponseSize = 0);
		bool sendCommand(Cmd command, void* params, unsigned int paramsLength, Ack& response, unsigned char extraResponseSize = 0);
		bool sendCommand(Cmd command, unsigned char param, Ack& response, unsigned char extraResponseSize = 0);

		// Update the delay counts from m_gwDriveDelays
		bool updateDriveDelays();

		// Trigger selecting this drive
		bool selectDrive(bool select);

		// Polls for the state of pins on the board
		void checkPins();
	public:
		// Constructor for this class
		GreaseWeazleInterface();

		// Free me
		~GreaseWeazleInterface();

		const bool isOpen() const { return m_comPort.isPortOpen(); };

		// Returns the timr before the motor switches back off
		int getMotorTimeout() const { return m_gwDriveDelays.watchdog_delay; }

		inline bool supportsDiskChange() const { return m_pinDskChangeAvailable; };
		inline bool isWriteProtected() const { return m_isWriteProtected; };

		// Attempts to open the reader running on the COM port number provided.  
		GWResponse openPort(bool useDriveA);

		// Reads a complete rotation of the disk, and returns it using the callback function whcih can return FALSE to stop
		// An instance of RotationExtractor is required.  This is purely to save on re-allocations.  It is internally reset each time
		GWResponse readRotation(RotationExtractor& extractor, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns,
			std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation);

		// Turns on and off the reading interface.  dontWait disables the GW timeout waiting, so you must instead.  Returns drOK, drError, 
		GWResponse enableMotor(const bool enable, const bool dontWait = false);

		// Seek to track 0 - Can return drRewindFailure, drSelectTrackError, drOK
		GWResponse findTrack0();

		// Select the track, this makes the motor seek to this position. Can return drRewindFailure, drSelectTrackError, drOK, drTrackRangeError
		GWResponse selectTrack(const unsigned char trackIndex, const TrackSearchSpeed searchSpeed = TrackSearchSpeed::tssNormal, bool ignoreDiskInsertCheck = false);

		// Choose which surface of the disk to read from.  Can return drError or drOK
		GWResponse selectSurface(const DiskSurface side);

		// Write data to the disk.  Can return drReadResponseFailed, drWriteProtected, drSerialOverrun, drWriteProtected, drOK
		GWResponse writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp);

		// Attempt to abort reading
		void abortReadStreaming();

		// Check if  adisk is present in the drive
		GWResponse checkForDisk(bool force);

		// Closes the port down
		void closePort();
	};

};

#endif