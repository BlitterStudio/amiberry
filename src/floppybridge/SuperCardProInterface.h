#ifndef SUPERCARDPRO_INTERFACE
#define SUPERCARDPRO_INTERFACE
/* Supercard Pro C++ Interface for *UAE
*
* By Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* Based on the documentation and hardware by Jim Drew at
* https://www.cbmstuff.com/
*
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/


#include <string>
#include <functional>
#include <vector>
#include <mutex>
#include "RotationExtractor.h"
#include "SerialIO.h"
#include "pll.h"

namespace SuperCardPro {

	// Represent which side of the disk we're looking at
	enum class DiskSurface {
		dsUpper,            // The upper side of the disk
		dsLower             // The lower side of the disk
	};

	// Potential responses from the SCP
	enum class SCPResponse : unsigned char {
		pr_Unused = 0x00,			// not used(to disallow NULL responses)
		pr_BadCommand = 0x01,		// bad command
		pr_CommandErr = 0x02,		// command error(bad structure, etc.)
		pr_Checksum = 0x03,			// packet checksum failed
		pr_Timeout = 0x04,			// USB timeout
		pr_NoTrk0 = 0x05,			// track zero never found(possibly no drive online)
		pr_NoDriveSel = 0x06,		// no drive selected
		pr_NoMotorSel = 0x07,		// motor not enabled(required for read or write)
		pr_NotReady = 0x08,			// drive not ready(disk change is high)
		pr_NoIndex = 0x09,			// no index pulse detected
		pr_ZeroRevs = 0x0A,			// zero revolutions chosen
		pr_ReadTooLong = 0x0B,		// read data was more than RAM would hold
		pr_BadLength = 0x0C,		// invalid length value
		pr_Reserved1 = 0x0D,		// reserved for future use
		pr_BoundaryOdd = 0x0E,		// location boundary is odd
		pr_WPEnabled = 0x0F,		// disk is write protected
		pr_BadRAM = 0x10,			// RAM test failed
		pr_NoDisk = 0x11,			// no disk in drive
		pr_BadBaud = 0x12,			// bad baud rate selected
		pr_BadCmdOnPort = 0x13,		// command is not available for this type of port
		pr_NoStream = 0x14,			// stream attempted to be stop when it was not occurring!
		pr_Overrun = 0x15,			// buffer overrun occurred
		pr_Ok = 0x4F,				// packet good(letter 'O' for OK)


		pr_writeError = 0xFF		// Used internally
	};

	enum class SCPCommand : unsigned char {
		DoCMD_SELA = 0x80, // select drive A
		DoCMD_SELB = 0x81, // select drive B
		DoCMD_DSELA = 0x82, // deselect drive A
		DoCMD_DSELB = 0x83, // deselect drive B

		DoCMD_MTRAON = 0x84, // turn motor A on
		DoCMD_MTRBON = 0x85, // turn motor B on
		DoCMD_MTRAOFF = 0x86, // turn motor A off
		DoCMD_MTRBOFF = 0x87, // turn motor B off

		DoCMD_SEEK0 = 0x88, // seek track 0
		DoCMD_STEPTO = 0x89, // step to specified track
		DoCMD_STEPIN = 0x8A, // step towards inner(higher) track
		DoCMD_STEPOUT = 0x8B, // step towards outer(lower) track

		DoCMD_SELDENS = 0x8C, // select density

		DoCMD_SIDE = 0x8D, // select side
		DoCMD_STATUS = 0x8E, // get drive status
		DoCMD_GETPARAMS = 0x90, // get parameters
		DoCMD_SETPARAMS = 0x91, // set parameters
		DoCMD_RAMTEST = 0x92, // do RAM test
		DoCMD_SETPIN33 = 0x93, // set pin 33 of floppy connector
		DoCMD_READFLUX = 0xA0, // read flux level
		DoCMD_GETFLUXINFO = 0xA1, // get info for last flux read
		DoCMD_WRITEFLUX = 0xA2, // write flux level
		DoCMD_SENDRAM_USB = 0xA9, // send data from buffer to USB
		DoCMD_LOADRAM_USB = 0xAA, // get data from USB and store in buffer
		DoCMD_SENDRAM_232 = 0xAB, // send data from buffer to the serial port
		DoCMD_LOADRAM_232 = 0xAC, // get data from the serial port and store in buffer
		DoCMD_STARTSTREAM = 0xAE, // start streaming flux data from the drive (requires firmware 1.3)
		DoCMD_STOPSTREAM = 0xAF, // Stop an active stream
		DoCMD_SCPInfo = 0xD0, // get info about SCP hardware / firmware
		DoCMD_SETBAUD1 = 0xD1, // sets the baud rate of port labeled RS232, // 1
		DoCMD_SETBAUD2 = 0xD2 // sets the baud rate of port labeled RS232, // 2
	};

	enum class SCPErr {
		scpOK,
		scpNotFound,
		scpInUse,
		scpNoDiskInDrive,
		scpWriteProtected,
		scpFirmwareTooOld,
		scpOverrun,
		scpUnknownError
	};

	class SCPInterface {
	private:
		// Windows handle to the serial port device
		SerialIO		m_comPort;
		bool			m_diskInDrive;
		bool			m_motorIsEnabled = false;
		bool			m_isWriteProtected = false;
		bool			m_useDriveA = false;
		bool			m_isAtTrack0 = false;
		int				m_currentTrack = -1;
		bool			m_isHDMode = false;
		bool			m_selectStatus = false;
		std::mutex      m_protectAbort;
		bool			m_abortStreaming = false;
		bool			m_abortSignalled = true;
		bool			m_isStreaming = false;

		struct {
			unsigned char hardwareVersion =0, hardwareRevision =0;
			unsigned char firmwareVersion =0, firmwareRevision =0;
		} m_firmwareVersion;

		// Apply and change the timeouts on the com port
		void applyCommTimeouts(bool shortTimeouts);
		
		// Trigger selecting this drive
		bool selectDrive(bool select);

		// Polls for the state of pins on the board
		void checkPins();

		// Various variants
		bool sendCommand(const SCPCommand command, SCPResponse& response);
		bool sendCommand(const SCPCommand command, const unsigned char parameter, SCPResponse& response);
		bool sendCommand(const SCPCommand command, const unsigned char* payload, const unsigned char payloadLength, SCPResponse& response, bool readResponse = true);

		// Read ram from the SCP
		//bool readSCPRam(const unsigned int offset, const unsigned int length);
	public:
		// Constructor for this class
		SCPInterface();

		// Free me
		~SCPInterface();

		const bool isOpen() const { return m_comPort.isPortOpen(); };

		bool isWriteProtected() const { return m_isWriteProtected; };

		// Attempts to open the reader running on the COM port number provided.  
		SCPErr openPort(bool useDriveA);

		// Reads a complete rotation of the disk, and returns it using the callback function whcih can return FALSE to stop
		// An instance of PLL is required which contains a rotation extractor.  This is purely to save on re-allocations.  It is internally reset each time
		SCPErr readRotation(PLL::BridgePLL& pll, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns,
			std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation);

		// Turns on and off the reading interface.  dontWait disables the GW timeout waiting, so you must instead.  
		bool enableMotor(const bool enable, const bool dontWait = false);

		// Seek to track 0 - Can return drRewindFailure, drSelectTrackError, drOK
		bool findTrack0();

		// Test if its an HD disk
		bool checkDiskCapacity(bool& isHD);

		// Select the track, this makes the motor seek to this position. Can return drRewindFailure, drSelectTrackError, drOK, drTrackRangeError
		bool selectTrack(const unsigned char trackIndex, bool ignoreDiskInsertCheck = false);

		// Special command that asks to do a 'seek to track -1' which isnt allowed but can be used for disk detection
		bool performNoClickSeek();

		// Choose which surface of the disk to read from.  Can return drError or drOK
		bool selectSurface(const DiskSurface side);

		// Write data to the disk.  Can return drReadResponseFailed, drWriteProtected, drSerialOverrun, drWriteProtected, drOK
		SCPErr writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp);

		// Attempt to abort reading
		bool abortReadStreaming();

		// Check if  adisk is present in the drive
		SCPErr checkForDisk(bool force);

		// Closes the port down
		void closePort();

		// Switch the density mode
		bool selectDiskDensity(bool hdMode);

		// Returns the motor iddle (auto switch off) time
		unsigned int getMotorIdleTimeoutTime();
	};

};

#endif