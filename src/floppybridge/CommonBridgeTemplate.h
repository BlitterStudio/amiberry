#ifndef COMMON_BRIDGE_TEMPLATE
#define COMMON_BRIDGE_TEMPLATE
/* CommonBridgeTemplate for *UAE
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

//////////////////////////////////////////////////////////////////////////////////////////
// As both the implementations (as far as the "bridge" is concerned are almost identical
// I have merged the common parts into this class so only the differences are in the 
// respective ones.  This is a good template for future devices to base off of
//////////////////////////////////////////////////////////////////////////////////////////
// Everything in protected needs implementing, but thats a lot less logic than on your own
// All the thread sync and WinUAE i/o is done for you in this class

#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "floppybridge_abstract.h"
#include "RotationExtractor.h"

// Maximum data we can receive.  
#define MFM_BUFFER_MAX_TRACK_LENGTH			0x3800

// Max number of cylinders we can offer
#define MAX_CYLINDER_BRIDGE 82


// Some of the functions in FloppyDiskBridge are implemented, some not.
class CommonBridgeTemplate : public FloppyDiskBridge {
protected:
	// Type of queue message
	enum class QueueCommand { qcTerminate, qcMotorOn, qcMotorOff, writeMFMData, qcGotoToTrack, qcSelectDiskSide, qcResetDrive };

	// Represent which side of the disk we're looking at.  
	enum class DiskSurface {
		dsUpper = 1,            // The upper side of the disk
		dsLower = 0				// The lower side of the disk
	};

	// Possible responses from the read command
	enum class ReadResponse { rrOK, rrError, rrNoDiskInDrive };

	// So you can change the status of this as you detect it, for example, after a seek, or surface change, or after diskchange effect
	void setWriteProtectStatus(const bool isWriteProtected) { m_writeProtected = isWriteProtected; }

	// Returns TRUE if we should be making a manual check for the presence of a disk
	bool isReadyForManualDiskCheck();

	// Called if you do something which is classed as a manual check for disk presence etc
	void updateLastManualCheckTime();

private:
	// Data to hold in the queue
	struct QueueInfo {
		QueueCommand command;
		union {
			int i;
			bool b;
		} option;
	};

	// Track data to write
	struct TrackToWrite {
		// Which track to write it to
		unsigned char mfmBuffer[MFM_BUFFER_MAX_TRACK_LENGTH];

		// Which side to write it to
		DiskSurface side;

		// Which track to write to
		unsigned char trackNumber;

		// Size of the above buffer in bits
		unsigned int floppyBufferSizeBits;

		// Should we start writing from the INDEX pulse?
		bool writeFromIndex;
	};

	// Protect the above vector in the multithreadded environment
	std::mutex m_pendingWriteLock;

	// A list of writes that still need to happen to a disk
	std::vector<TrackToWrite> m_pendingTrackWrites;

	// The current track being written to (or more accurately read from WinUAE)
	TrackToWrite m_currentWriteTrack;

	// For extracting rotations from disks
	RotationExtractor m_extractor;

	// Current stored position for receiving data t be written from WinUAE
	int m_currentWriteStartMfmPosition;

	// If we should pause streaming data from the disk
	bool m_delayStreaming;

	// When the above delay started
	std::chrono::time_point<std::chrono::steady_clock> m_delayStreamingStart;

	// An Event to wait for the drive reset operation to happen (the queue will be clear afterwards)
	std::mutex m_driveResetStatusFlagLock;
	std::condition_variable m_driveResetStatusFlag;
	bool m_driveResetStatus;

	// An event to signal if the drive is currently being read
	bool m_driveStreamingData;

	// This flag gets set if we're being sneaky and reading the opposite disk side
	bool m_isCurrentlyHeadCheating;

	// Cache of drive read data
	struct MFMCache {
		// Buffer for current data.  This is a circular buffer
		RotationExtractor::MFMSample mfmBuffer[MFM_BUFFER_MAX_TRACK_LENGTH];

		// If this is a complete revolution or not
		bool ready;

		// Size fo the mfm data we actually have (in bits) so far
		int amountReadInBits;
	};

	// Current disk cache history
	struct MFMCaches {
		// Currently being read by WinUAE version of this track
		MFMCache current;
		// The last buffer we swapped out.  We keep several buffers on the go to combat 'weak transitions' and also help with disk errors
		MFMCache last;
		// The track we're about to read in
		MFMCache next;
		// For tracking what the index looks like
		RotationExtractor::IndexSequenceMarker startBitPatterns;
	};

	// Cache of entire disk (or what we have so far)
	MFMCaches m_mfmRead[MAX_CYLINDER_BRIDGE][2];

	// The main thread
	std::thread* m_control;

	// The track we tell WinUAE we're on
	int m_currentTrack;

	// The actual track we're on
	int m_actualCurrentCylinder;

	// For reporting to WinUAE if the disk is write protected
	bool m_writeProtected;

	// For reporting to WinUAE if theres a disk in the drive
	bool m_diskInDrive;

	// This is set to TRUE to inform the system we're going to read the first cylinder (both sides) before we officially tell UAE the drive is ready.
	bool m_firstTrackMode;

	// Flag to specify if the interface is allowed to stall WinUAE
	const bool m_canStall;

	// Flag to specify if the interface MUST use the INDEX pulses to locate entire disk revolutions
	const bool m_useIndex;

	// Set to true while the motor is spinning up, but not there yet
	bool m_motorSpinningUp;

	// Set to when the motor started spinning up
	std::chrono::time_point<std::chrono::steady_clock> m_motorSpinningUpStart;

	// Set to true once the motor is spun up
	bool m_motorIsReady;

	// Bool flag from what WinUAE has asked of us regarding motor status
	bool m_isMotorRunning;

	// When we last checked to see if a disk was still in the drive (you cant tell without moving the head)
	std::chrono::time_point<std::chrono::steady_clock> m_lastDiskCheckTime;

	// A queue to hold the commands to process on the thread
	std::queue<QueueInfo> m_queue;

	// A lock to prevent the queue being accessed in two places at once
	std::mutex m_queueProtect;

	// A lock to prevent buffer switch being accessed or changed in two places at once
	std::mutex m_switchBufferLock;

	// A semaphore to track the number messages waiting in the queue
	std::mutex m_queueSemaphoreLock;
	std::condition_variable m_queueSemaphoreFlag;
	unsigned int m_queueSemaphore;

	// An event that is set the moment data is available
	std::mutex m_readBufferAvailableLock;
	std::condition_variable m_readBufferAvailableFlag;
	bool m_readBufferAvailable;

	// Error messages
	std::string m_lastError;
	std::wstring m_lastErrorW;

	// The side WinUAE think's we're on
	DiskSurface m_floppySide;

	// The side we're actually on
	DiskSurface m_actualFloppySide;

	// The main thread
	void mainThread();

	// Add a command for the thread to process
	void queueCommand(const QueueCommand command, const bool optionB, const bool shouldAbortStreaming = true);
	void queueCommand(const QueueCommand command, const int optionI = 0, const bool shouldAbortStreaming = true);
	// Push a specific message onto the queue
	void pushOntoQueue(const QueueInfo& info, const bool shouldAbortStreaming = true);

	// Handle processing the command
	void processCommand(const QueueInfo& info);

	// Handle background reading
	void handleBackgroundDiskRead();

	// Reset the current MFM buffer used by readMFMBit()
	void resetMFMCache();

	// Called to reverse initialise.  This will automatically be called in the destructor too.
	void terminate();

	// Handle disk side change
	void switchDiskSide(const bool side);

	// Process the queue.  Return TRUE if the thread should quit
	bool processQueue();

	// This is called to switch to a different copy of the track so multiple revolutions can ve read
	void internalSwitchCylinder(const int cylinder, const DiskSurface side);

	// Save a new disk side and switch it in if it can be
	void saveNextBuffer(const int cylinder, const DiskSurface side);

	// Reset and clear out any data we have received thus far
	void resetWriteBuffer();

protected:

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Stuff that you need to implement in your derived class, a lot less than on the original bridge - These are all allowed to block as they're called from a thread
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Return the number of milliseconds required for the disk to spin up.  You *may* need to override this
	virtual const unsigned int getDriveSpinupTime() { return 600; };

	// If your device supports being able to abort a disk read, mid-read then implement this
	virtual void abortDiskReading() {};

	// This is called by the main thread incase you need to do anything specific at regulat intervals
	virtual void poll() {};

	// If your device supports the DiskChange option then return TRUE here.  If not, then the code will simulate it
	virtual bool supportsDiskChange()  = 0;

	// If the above is TRUE then this is called to get the status of the DiskChange line.  Basically, this is TRUE if there is a disk in the drive.
	// If force is true you should re-check, if false, then you are allowed to return a cached value from the last disk operation (eg: seek)
	virtual bool getDiskChangeStatus(const bool forceCheck)  = 0;

	// Called when the class is about to shut down
	virtual void closeInterface()  = 0;

	// Called to start the interface, you should update any error messages if it fails.  This needs to be ready to see to any cylinder and so should already know where cylinder 0 is
	virtual bool openInterface(std::string& errorMessage)  = 0;

	// Called to ask the drive what the current write protect status is - return true if its write protected.  If forceCheck is true you should actually check the drive, 
	// else use the last status checked which was probably from a SEEK setCurrentCylinder call.  If you can ONLY get this information here then you should always force check
	virtual bool checkWriteProtectStatus(const bool forceCheck)  = 0;

	// Duplicate of the one below, but here for consistancy - Returns the name of interface.  This pointer should remain valid after the class is destroyed
	virtual const TCHAR* _getDriveIDName()  = 0;

	// Duplicate of the one below, but here for consistancy - Returns the name of interface.  This pointer should remain valid after the class is destroyed
	virtual const DriveTypeID _getDriveTypeID()  = 0;

	// Called to switch which head is being used right now.  Returns success or not
	virtual bool setActiveSurface(const DiskSurface activeSurface)  = 0;

	// Set the status of the motor on the drive. The motor should maintain this status until switched off or reset.  This should *NOT* wait for the motor to spin up
	virtual bool setMotorStatus(const bool switchedOn)  = 0;

	// Trigger a seek to the requested cylinder, this can block until complete
	virtual bool setCurrentCylinder(const unsigned int cylinder)  = 0;

	// Called when data should be read from the drive.
	//		rotationExtractor: supplied if you use it
	//		maxBufferSize: Maximum number of RotationExtractor::MFMSample in the buffer.  If we're trying to detect a disk, this might be set VERY LOW
	// 	    buffer:		   Where to save to.  When a buffer is saved, position 0 MUST be where the INDEX pulse is.  RevolutionExtractor will do this for you
	//		indexMarker:   Used by rotationExtractor if you use it, to help be consistant where the INDEX position is read back at
	//		onRotation: A function you should call for each complete revolution received.  If the function returns FALSE then you should abort reading, else keep sending revolutions
	// Returns: ReadResponse, explains its self
	virtual ReadResponse readData(RotationExtractor& rotationExtractor, const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
		std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation)  = 0;

	// Called when a cylinder revolution should be written to the disk.
	// Parameters are:	rawMFMData						The raw data to be written.  This is an actual MFM stream, going from MSB to LSB for each byte
	//					numBytes						Number of bits in the buffer to write
	//					writeFromIndex					If an attempt should be made to write this from the INDEX pulse rather than just a random position
	//					suggestUsingPrecompensation		A suggestion that you might want to use write precompensation, optional
	// Returns TRUE if success, or false if it fails.  Largely doesnt matter as most stuff should verify with a read straight after
	virtual bool writeData(const unsigned char* rawMFMData, const unsigned int numBits, const bool writeFromIndex, const bool suggestUsingPrecompensation)  = 0;

	// A manual way to check for disk change.  This is simulated by issuing a read message and seeing if theres any data.  Returns TRUE if data or an INDEX pulse was detected
	// It's virtual as the default method issues a read and looks for data.  If you have a better implementation then override this
	virtual bool attemptToDetectDiskChange()  = 0;

public:
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// All the public functions are already implemented for you
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 
	// Flags from WINUAE
	CommonBridgeTemplate(const bool canStall, const bool useIndex);
	virtual ~CommonBridgeTemplate();

	// Call to start the system up
	virtual bool initialise() override final;

	// This is called prior to closing down, but shoudl reverse initialise
	virtual void shutdown() override final;

	// Returns the name of interface.  This pointer should remain valid after the class is destroyed
	virtual const TCHAR* getDriveIDName() override final { return _getDriveIDName(); };

	// Return the type of disk connected
	virtual DriveTypeID getDriveTypeID() override final { return _getDriveTypeID(); };

	// Call to get the last error message.  Length is the size of the buffer in TCHARs, the function returns the size actually needed
	virtual unsigned int getLastErrorMessage(TCHAR* errorMessage, unsigned int length) override final;

	// Return TRUE if the drive is currently on cylinder 0
	virtual bool isAtCylinder0() override final { return m_currentTrack == 0; }

	// Return the number of cylinders the drive supports.
	virtual unsigned char getMaxCylinder() override final { return MAX_CYLINDER_BRIDGE; };

	// Return true if the motor is spinning
	virtual bool isMotorRunning() override final { return m_isMotorRunning; };

	// Returns TRUE when the last command requested has completed
	virtual bool isReady() override;

	// Return TRUE if there is a disk in the drive, else return false.  Some drives dont detect this until the head moves once
	virtual bool isDiskInDrive() override final { return m_diskInDrive; };

	// Check if the disk has changed.  Basically returns FALSE if theres no disk in the drive
	virtual bool hasDiskChanged() override final { return !m_diskInDrive; };

	// Return the current track number we're on
	virtual unsigned char getCurrentCylinderNumber() override final { return m_currentTrack; };

	// Return TRUE if the currently insrted disk is write protected
	virtual bool isWriteProtected() override final { return m_writeProtected; };

	// Get the speed at this position.  1000=100%.  
	virtual int getMFMSpeed(const int mfmPositionBits) override final;

	// While not doing anything else, the library should be continuously streaming the current track if the motor is on.  mfmBufferPosition is in BITS 
	virtual bool getMFMBit(const int mfmPositionBits) override final;

	// Set the status of the motor. 
	virtual void setMotorStatus(bool side, bool turnOn) override final;

	// Return the maximum size of the internal track buffer in BITS
	virtual int maxMFMBitPosition() override final;

	// This is called to switch to a different copy of the track so multiple revolutions can ve read
	virtual void mfmSwitchBuffer(bool side) override final;

	// Quick confirmation from UAE that we're actually on the same side
	virtual void setSurface(bool side) override final;

	// Seek to a specific track
	virtual void gotoCylinder(int trackNumber, bool side) override final;

	// Submits a single WORD of data received during a DMA transfer to the disk buffer.  This needs to be saved.  It is usually flushed when commitWriteBuffer is called
	// You should reset this buffer if side or track changes.  mfmPosition is provided purely for any index sync you may wish to do
	virtual void writeShortToBuffer(bool side, unsigned int track, unsigned short mfmData, int mfmPosition) override final;

	// Requests that any data received via writeShortToBuffer be saved to disk. The side and track should match against what you have been collected
	// and the buffer should be reset upon completion.  You should return the new tracklength (maxMFMBitPosition) with optional padding if needed
	virtual unsigned int commitWriteBuffer(bool side, unsigned int track) override final;

	// Return TRUE if we're at the INDEX marker
	virtual bool isMFMPositionAtIndex(int mfmPositionBits) override final;

	// Reset the drive.  This should reset it to the state it would be at powerup
	virtual bool resetDrive(int trackNumber) override final;
};

#endif