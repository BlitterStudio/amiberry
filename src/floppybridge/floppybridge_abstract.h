/* floppybridge_abstract
*
* Copyright (C) 2021-2022 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This library defines a standard interface for connecting physical disk drives to *UAE
*
* Derived classes must be implemented so they are unlikely to cause a delay in any function
* as doing so would cause audio and mouse cursors to stutter
* 
* This is free and unencumbered released into the public domain.
* See the file COPYING for more details, or visit <http://unlicense.org>.
*
*/

/*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

#pragma once

#include <functional>
#include <cstddef>

#ifdef _WIN32
#include <tchar.h>
#else
#ifndef TCHAR
#ifdef _UNICODE
#define TCHAR wchar_t
#define _T(x) L ##x
#else
#define TCHAR char
#define _T(x) x
#endif
#endif
#endif



class FloppyDiskBridge {
public:

	// Driver information
	struct BridgeDriver {
		// Details about the driver
		const char* name;
		const char* url;
		const char* manufacturer;
		const char* driverAuthor;

		// Which options in configuration it can support, aside from the standard ones
		const unsigned int configOptions;
	};

	// Definition of the type of drive
	enum class DriveTypeID : unsigned char { dti35DD = 0, dti35HD = 1, dti5255SD = 2 };

	FloppyDiskBridge() {}
	// This is just to force this being virtual
	virtual ~FloppyDiskBridge() {}

	// Call to start the system up.  Return false if it fails
	virtual bool initialise() = 0;

	// This is called prior to closing down, but should reverse initialise
	virtual void shutdown() {}

	// Returns the name of interface.  This pointer should remain valid *after* the class is destroyed so should be static
	virtual const BridgeDriver* getDriverInfo()  = 0;

	// Return the 'bit cell' time in uSec.  Standard DD Amiga disks this would be 2uS, HD disks would be 1us I guess, but mainly used for =4 for SD I think
	virtual unsigned char getBitSpeed() { return 2; }

	// Return the type of disk connected.  This is used to tell WinUAE if we're DD or HD.  This must return INSTANTLY
	virtual DriveTypeID getDriveTypeID()  = 0;

	// Call to get the last error message.  If the board initialised this may return a compatibility warning instead
	virtual const char* getLastErrorMessage() { return NULL; }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




	// Reset the drive.  This should reset it to the state it would be at powerup, ie: motor switched off etc.  The current cylinder number can be 'unknown' at this point
	virtual bool resetDrive(int trackNumber)  = 0;




	/////////////////////// Head movement Controls //////////////////////////////////////////
	// Return TRUE if the drive is currently on cylinder 0
	virtual bool isAtCylinder0()  = 0;

	// Return the number of cylinders the drive supports.  Eg: 80 or 82 (or 40)
	virtual unsigned char getMaxCylinder()  = 0;

	// Seek to a specific cylinder
	virtual void gotoCylinder(int cylinderNumber, bool side)  = 0;

	// Handle the drive stepping to track -1 - this is used to 'no-click' detect the disk
	virtual void handleNoClickStep(bool side) = 0;

	// Return the current cylinder number we're on
	virtual unsigned char getCurrentCylinderNumber()  = 0;



	/////////////////////// Drive Motor Controls /////////////////////////////////////////////
	// Return true if the motor is spinning, but not necessarily up to speed
	virtual bool isMotorRunning()  = 0;

	// Turn on and off the motor
	virtual void setMotorStatus(bool side, bool turnOn)  = 0;

	// Returns TRUE if the drive is ready (ie: the motor has spun up to speed to speed)
	virtual bool isReady()  = 0;

	// Returns the currently selected side
	virtual bool getCurrentSide() = 0;

	/////////////////////// Disk Detection ///////////////////////////////////////////////////
	// Return TRUE if there is a disk in the drive.  This is usually called after gotoCylinder
	virtual bool isDiskInDrive()  = 0;

	// Check if the disk has changed.  Basically returns FALSE if there's no disk in the drive
	virtual bool hasDiskChanged()  = 0;



	/////////////////////// Reading Data /////////////////////////////////////////////////////
	// Return TRUE if we're at the INDEX marker/sensor.  mfmPositionBits is in BITS
	virtual bool isMFMPositionAtIndex(int mfmPositionBits)  = 0;

	// Returns TRUE if data is ready and available
	virtual bool isMFMDataAvailable() = 0;

	// This returns a single MFM bit at the position provided
	virtual bool getMFMBit(const int mfmPositionBits)  = 0;

	// This asks the time taken to read the bit at mfmPositionBits.  1000=100%, <1000 data is read faster, >1000 data is read slower.
	// This number is used in a loop (scaled) starting with a number, and decrementing by this number.
	// Each loop a single bit is read.  So the smaller the number, the more loops that occur, and the more bits that are read
	virtual int getMFMSpeed(const int mfmPositionBits)  = 0;

	// This is called in both modes.  It is called when WinUAE detects a full revolution of data has been read.  This could allow you to switch to a different recording of the same cylinder if needed.
	virtual void mfmSwitchBuffer(bool side)  = 0;

	// Quick confirmation from UAE that we're actually on the same side
	virtual void setSurface(bool side)  = 0;

	// Return the maximum size of bits available in this revolution.  This is the maximum passed to getMFMBit
	virtual int maxMFMBitPosition()  = 0;

	/////////////////////// Writing Data /////////////////////////////////////////////////////

	// Submits a single WORD of data received during a DMA transfer to the disk buffer.  This needs to be saved.  It is usually flushed when commitWriteBuffer is called
	// You should reset this buffer if side or track changes, mfmPosition is provided purely for any index sync you may wish to do
	virtual void writeShortToBuffer(bool side, unsigned int track, unsigned short mfmData, int mfmPosition)  = 0;

	// Return TRUE if the currently inserted disk is write protected
	virtual bool isWriteProtected()  = 0;

	// Requests that any data received via writeShortToBuffer be saved to disk. The side and track should match against what you have been collected
	// and the buffer should be reset upon completion.  You should return the new track length (maxMFMBitPosition) with optional padding if needed
	virtual unsigned int commitWriteBuffer(bool side, unsigned int track)  = 0;

	// Returns TRUE if commitWriteBuffer has been called but not written to disk yet
	virtual bool isWritePending() = 0;

	// Returns TRUE if a write is no longer pending.  This should only return TRUE the first time, and then should reset
	virtual bool isWriteComplete() = 0;

	// Set to TRUE if turbo writing is allowed (this is a sneaky DMA bypass trick)
	virtual bool canTurboWrite() = 0;

	// Return TRUE if there is data ready to be committed to disk
	virtual bool isReadyToWrite() = 0;
};


