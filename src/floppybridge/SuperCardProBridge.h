#ifndef SUPERCARDPRO_FLOPPY_BRIDGE
#define SUPERCARDPRO_FLOPPY_BRIDGE
/* WinUAE Supercard Pro Interface for *UAE
*
* By Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

//
// ROMTYPE_SUPERCARDPRO_WRITER must be defined for this to work

#ifdef ROMTYPE_SUPERCARDPRO_WRITER

#include "floppybridge_abstract.h"
#include "CommonBridgeTemplate.h"
#include "SuperCardProInterface.h"
#include "pll.h"


class SupercardProDiskBridge : public CommonBridgeTemplate {
private:
	// When the motor last switched on	
	std::chrono::time_point<std::chrono::steady_clock> m_motorTurnOnTime;
	bool m_motorIsEnabled = false;

	// Which com port should we use? or blank for automatic
	std::string m_comPort;
	
	// Which drive to use
	bool m_useDriveA = false;

	// Is this a HD disk?
	bool m_isHDDisk = false;

	// Hardware connection
	SuperCardPro::SCPInterface m_io;

	// Remember where we are
	int m_currentCylinder = 0;

protected:
	// Called when a disk is inserted so that you can (re)populate the response to _getDriveTypeID()
	virtual void checkDiskType() override;

	// Called to force into DD or HD mode.  Overrides checkDiskType() until checkDiskType() is called again
	virtual void forceDiskDensity(bool forceHD) override;

	// This is called by the main thread in case you need to do anything specific at regular intervals
	virtual void poll() override;

	// If your device supports being able to abort a disk read, mid-read then implement this
	virtual void abortDiskReading() override;

	// If your device supports the DiskChange option then return TRUE here.  If not, then the code will simulate it
	virtual bool supportsDiskChange() override;

	// If the above is TRUE then this is called to get the status of the DiskChange line.  Basically, this is TRUE if there is a disk in the drive.
	// If force is true you should re-check, if false, then you are allowed to return a cached value from the last disk operation (eg: seek)
	virtual bool getDiskChangeStatus(const bool forceCheck) override;

	// Called when the class is about to shut down
	virtual void closeInterface() override;

	// Called to start the interface, you should update any error messages if it fails.  This needs to be ready to see to any cylinder and so should already know where cylinder 0 is
	virtual bool openInterface(std::string& errorMessage) override;

	// Called to ask the drive what the current write protect status is - return true if its write protected.  If forceCheck is true you should actually check the drive, 
	// else use the last status checked which was probably from a SEEK setCurrentCylinder call.  If you can ONLY get this information here then you should always force check
	virtual bool checkWriteProtectStatus(const bool forceCheck) override;

	// Get the name of the drive
	virtual const BridgeDriver* _getDriverInfo() override;

	// Duplicate of the one below, but here for consistency - Returns the name of interface.  This pointer should remain valid after the class is destroyed
	virtual const DriveTypeID _getDriveTypeID() override;

	// Called to switch which head is being used right now.  Returns success or not
	virtual bool setActiveSurface(const DiskSurface activeSurface) override;

	// Set the status of the motor on the drive. The motor should maintain this status until switched off or reset.  This should *NOT* wait for the motor to spin up
	virtual bool setMotorStatus(const bool switchedOn) override;

	// Trigger a seek to the requested cylinder, this can block until complete
	virtual bool setCurrentCylinder(const unsigned int cylinder) override;

	// If we're on track 0, this is the emulator trying to seek to track -1.  We catch this as a special case.  
	// Should perform the same operations as setCurrentCylinder in terms of disk change etc but without changing the current cylinder
	// Return FALSE if this is not supported by the bridge
	virtual bool performNoClickSeek() override;

	// Called when data should be read from the drive.
	//		pll:		   supplied if you use it
	//		maxBufferSize: Maximum number of RotationExtractor::MFMSample in the buffer.  If we're trying to detect a disk, this might be set VERY LOW
	// 	    buffer:		   Where to save to.  When a buffer is saved, position 0 MUST be where the INDEX pulse is.  RevolutionExtractor will do this for you
	//		indexMarker:   Used by rotationExtractor if you use it, to help be consistent where the INDEX position is read back at
	//		onRotation: A function you should call for each complete revolution received.  If the function returns FALSE then you should abort reading, else keep sending revolutions
	// Returns: ReadResponse, explains its self
	virtual ReadResponse readData(PLL::BridgePLL& pll, const unsigned int maxBufferSize, RotationExtractor::MFMSample* buffer, RotationExtractor::IndexSequenceMarker& indexMarker,
		std::function<bool(RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits)> onRotation) override;

	// Called when a cylinder revolution should be written to the disk.
	// Parameters are:	rawMFMData						The raw data to be written.  This is an actual MFM stream, going from MSB to LSB for each byte
	//					numBytes						Number of bits in the buffer to write
	//					writeFromIndex					If an attempt should be made to write this from the INDEX pulse rather than just a random position
	//					suggestUsingPrecompensation		A suggestion that you might want to use write pre-compensation, optional
	// Returns TRUE if success, or false if it fails.  Largely doesnt matter as most stuff should verify with a read straight after
	virtual bool writeData(const unsigned char* rawMFMData, const unsigned int numBits, const bool writeFromIndex, const bool suggestUsingPrecompensation) override;

	// A manual way to check for disk change.  This is simulated by issuing a read message and seeing if there's any data.  Returns TRUE if data or an INDEX pulse was detected
	// It's virtual as the default method issues a read and looks for data.  If you have a better implementation then override this
	virtual bool attemptToDetectDiskChange() override;

public:
	SupercardProDiskBridge(BridgeMode bridgeMode, BridgeDensityMode bridgeDensity, bool enableAutoCache, bool useSmartSpeed, bool autoDetectComPort, char* comPort, bool driveOnB);

	// This is for the static version
	SupercardProDiskBridge(BridgeMode bridgeMode, BridgeDensityMode bridgeDensity, int uaeSettings);


	virtual ~SupercardProDiskBridge();

	static const BridgeDriver* staticBridgeInformation();
};



#endif
#endif