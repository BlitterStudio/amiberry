/* CommonBridgeTemplate for *UAE
*
* Copyright (C) 2021-2022 Robert Smith (@RobSmithDev)
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
//


#include <thread>
#include <functional>
#include <queue>
#include <chrono>
#include <cstring>

#include "floppybridge_abstract.h"
#include "CommonBridgeTemplate.h"
#include "RotationExtractor.h"
#include "SerialIO.h"

#ifdef HIGH_RESOLUTION_MODE
HIGH_RESOLUTION_MODE is not required and just uses up more memory(revolution extractor)
#endif


#ifdef OUTPUT_TIME_IN_NS
OUTPUT_TIME_IN_NS should not be set(revolution extractor)
#endif


// Assume a disk rotates every at 300rpm.
// Assume the bitcel size is 2uSec
// This means 5 revolutions per second
// Each revolution is 200 milliseconds
// 200ms / 2uSec = 100000 bits
// 100000 bits is 12500 bytes
// Assume the drive spin speed tolerance is +/- 3%, so 
// Biggest this can be is: 12886 bytes (-3 %)
// Perfect this can be is: 12500 bytes 
// Smallest this can be is: 12135 bytes (+3 %)
#define THEORETICAL_MINIMUM_TRACK_LENGTH			12134

// Speed to report to WinUAE while in 'garbage' mode (1000 is normal, *should* be between 700 (fast) and 3000 (slow))
// This "cheat" makes things a little more reliable with some loaders.  Not sure why.
#define DRIVE_GARBAGE_SPEED							6000

// The 'bit' to return during this garbage time.  
#define DRIVE_GARBAGE_VALUE							1

// Auto-sense the disk, just like Amiga OS will, be do this if it hasn't 
#define DISKCHANGE_BEFORE_INSERTED_CHECK_INTERVAL	3000

// We need to poll the drive once theres a disk in there to know when it's been removed.  This is the poll interval
#define DISKCHANGE_ONCE_INSERTED_CHECK_INTERVAL		500

// Auto-sense the disk, just like Amiga OS will, be do this if it hasn't - this is for non dick change hardware
#define DISKCHANGE_BEFORE_INSERTED_CHECK_INTERVAL_NODSKCHG 3000

// We need to poll the drive once theres a disk in there to know when its been removed.   this is for disk change compatible hardware
#define DISKCHANGE_ONCE_INSERTED_CHECK_INTERVAL_NODSKCHG 3000

// The cylinder number that precomp should begin at
#define WRITE_PRECOMP_START 40

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#endif

// Flags from WINUAE
CommonBridgeTemplate::CommonBridgeTemplate(BridgeMode bridgeMode, BridgeDensityMode bridgeDensity, bool shouldAutoCache, bool useSmartSpeed) :
	m_currentWriteStartMfmPosition(0), m_delayStreaming(false), m_driveResetStatus(false), m_driveStreamingData(false), m_isCurrentlyHeadCheating(false), m_inHDMode(false),
	m_control(nullptr), m_currentTrack(0), m_actualCurrentCylinder(0), m_writeProtected(false), m_diskInDrive(false), m_firstTrackMode(false), m_autocacheModifiedCurrentCylinder(false),
	m_bridgeMode(bridgeMode), m_bridgeDensity(bridgeDensity), m_motorSpinningUp(false), m_motorIsReady(false), m_isMotorRunning(false), m_autoCacheMotorStatus(false), m_useSmartSpeed(useSmartSpeed),
	m_queueSemaphore(0), m_readBufferAvailable(false), m_floppySide(DiskSurface::dsLower), m_actualFloppySide(DiskSurface::dsLower), m_shouldAutoCache(shouldAutoCache), m_pll(true, true) {

	memset(&m_mfmRead, 0, sizeof(m_mfmRead));;
	m_pll.setRotationExtractor(&m_extractor);
}

// Change to a different bridge-mode (in real-time)
void CommonBridgeTemplate::changeBridgeMode(BridgeMode bridgeMode) {
	m_bridgeMode = bridgeMode;
}

// Change to a different bridge-density (in real-time)
void CommonBridgeTemplate::changeBridgeDensity(BridgeDensityMode bridgeDensity) {
	m_bridgeDensity = bridgeDensity;
}

// Quick confirmation from UAE that we're actually on the same side
void CommonBridgeTemplate::setSurface(bool side) {
	switchDiskSide(side);
}

// Free
CommonBridgeTemplate::~CommonBridgeTemplate() {
	terminate();
}

// If it returns TRUE, this is the next cylinder and side that should be cached in the background because there is NO data for it
bool CommonBridgeTemplate::getNextTrackToAutoCache(int& cylinder, DiskSurface& side) {
	if (!m_shouldAutoCache) return false;

	// Plan is cache track 0-10, 40-50, and then fill in.
	for (cylinder = 0; cylinder < 10; cylinder++) {
		for (int s = 0; s < 2; s++) {
			if (!m_mfmRead[cylinder][s].current.ready) {
				side = (DiskSurface)s;
				return true;
			}
		}
	}

	// Now the 40-50
	for (cylinder = 40; cylinder < 50; cylinder++) {
		for (int s = 0; s < 2; s++) {
			if (!m_mfmRead[cylinder][s].current.ready) {
				side = (DiskSurface)s;
				return true;
			}
		}
	}

	// Now fill in the rest
	for (cylinder = 10; cylinder < 40; cylinder++) {
		for (int s = 0; s < 2; s++) {
			if (!m_mfmRead[cylinder][s].current.ready) {
				side = (DiskSurface)s;
				return true;
			}
		}
	}

	for (cylinder = 50; cylinder < 80; cylinder++) {
		for (int s = 0; s < 2; s++) {
			if (!m_mfmRead[cylinder][s].current.ready) {
				side = (DiskSurface)s;
				return true;
			}
		}
	}

	return false;
}

// Handle disk side change
void CommonBridgeTemplate::switchDiskSide(const bool side) {
	// Just to be 100% sure this is working right
	DiskSurface incommingSide = side ? DiskSurface::dsUpper : DiskSurface::dsLower;

	if (incommingSide != m_floppySide) {
		resetWriteBuffer();
		m_floppySide = incommingSide;
		if (!m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
			std::lock_guard lock(m_readBufferAvailableLock);
			m_readBufferAvailable = false;
		}
		queueCommand(QueueCommand::qcSelectDiskSide, side, !m_isCurrentlyHeadCheating);
	}
}

// Add a command for the thread to process
void CommonBridgeTemplate::queueCommand(const QueueCommand command, const int optionI, const bool shouldAbortStreaming) {
	QueueInfo info{};
	info.command = command;
	info.option.i = optionI;

	pushOntoQueue(info, shouldAbortStreaming);
}

// Add a command for the thread to process
void CommonBridgeTemplate::queueCommand(const QueueCommand command, const bool optionB, const bool shouldAbortStreaming) {
	QueueInfo info{};
	info.command = command;
	info.option.b = optionB;

	pushOntoQueue(info, shouldAbortStreaming);
}

// Add to queue
void CommonBridgeTemplate::pushOntoQueue(const QueueInfo& info, const bool shouldAbortStreaming, bool insertAtStart) {
	{
		std::lock_guard lock(m_queueProtect);
		if (insertAtStart) m_queue.push_front(info); else m_queue.push_back(info);
	}
	{
		std::lock_guard lock(m_queueSemaphoreLock);
		m_queueSemaphore++;
		m_queueSemaphoreFlag.notify_one();
	}

	// A little sneaky trick.  If there's a command to move on, but we have like 90% of the data and we don't have a complete reading for that track yet, it makes sense to carry on and finish it.
	if ((m_driveStreamingData) && ((m_bridgeMode == BridgeMode::bmStalling) || m_extractor.isNearlyReady()) && (!m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].current.ready)) return;
	if (m_firstTrackMode) return;  // don't stop me now

	// Make it drop out of streaming if that's what its doing
	if (shouldAbortStreaming && (!m_writeCompletePending) && (!m_writePending)) abortDiskReading();
}

// Process the queue.  Return TRUE if the thread should quit
bool CommonBridgeTemplate::processQueue() {
	QueueInfo cmd{};

	{
		std::lock_guard lock(m_queueProtect);
		if (m_queue.empty()) return false;

		cmd = m_queue.front();
		m_queue.pop_front();
	}

	// Special exit condition
	if (cmd.command == QueueCommand::qcTerminate) {
		return true;
	}

	processCommand(cmd);

	return false;
}

// Returns TRUE if we should be making a manual check for the presence of a disk
bool CommonBridgeTemplate::isReadyForManualDiskCheck() {
	const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastDiskCheckTime).count();
	
	if (supportsDiskChange()) {
		return ((timePassed > DISKCHANGE_ONCE_INSERTED_CHECK_INTERVAL) && (m_diskInDrive)) ||
			   ((timePassed > DISKCHANGE_BEFORE_INSERTED_CHECK_INTERVAL) && (!m_diskInDrive));
	}
	else {
		// This version we cant detect disk change to easily.  So we do it less often as it impacts the disk more.
		return ((timePassed > DISKCHANGE_ONCE_INSERTED_CHECK_INTERVAL_NODSKCHG) && (m_diskInDrive)) ||
			   ((timePassed > DISKCHANGE_BEFORE_INSERTED_CHECK_INTERVAL_NODSKCHG) && (!m_diskInDrive));
	}
}

// The main thread
void CommonBridgeTemplate::mainThread() {
	m_lastDiskCheckTime = std::chrono::steady_clock::now();

	for (;;) {
		// Extract processing needed?
		poll();

		// Check if the motor should be disabled.
		checkMotorOff();

		bool lastDiskState = m_diskInDrive;
		bool diskInDrive = m_diskInDrive;

		bool queueReady = false;

		{
			bool autoCachingRequired = false;
			if (m_shouldAutoCache) {
				int nextCylinder;
				DiskSurface nextSurface;
				autoCachingRequired = getNextTrackToAutoCache(nextCylinder, nextSurface);
			}

			// Check if we should do this
			const auto queuePause = std::chrono::milliseconds((m_motorIsReady || autoCachingRequired) ? 1 : 50);

			{
				std::unique_lock lck(m_queueSemaphoreLock);
				queueReady = m_driveResetStatusFlag.wait_for(lck, queuePause, [this] { return m_queueSemaphore > 0; });
			}
		}
		if (queueReady) {
			{
				std::lock_guard lck(m_queueSemaphoreLock);
				m_queueSemaphore--;
			}

			if (processQueue())
				return;
		}
		else {
			// Trigger background reading if we're not busy
			if (m_motorIsReady) {
				const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_delayStreamingStart).count();
				if ((!m_delayStreaming) || ((m_delayStreaming) && (timePassed > 100)))
					handleBackgroundDiskRead();
			}
			else
				handleBackgroundCaching();

			// If the queue is empty, the motor isn't on, and we think there's a disk in the drive we periodically check as we don't have any other way to find out
			if ((isReadyForManualDiskCheck()) && (m_queue.empty())) {
				// Monitor for disk
				if (!supportsDiskChange()) {
					diskInDrive = attemptToDetectDiskChange();
				}
				else {
					// This may cause the drive to temporarily step
					diskInDrive = getDiskChangeStatus(true);
				}
				m_lastDiskCheckTime = std::chrono::steady_clock::now();

				m_writeProtected = checkWriteProtectStatus(false);
			}
		}

		if (m_motorSpinningUp) {
			const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_motorSpinningUpStart).count();
			if (timePassed >= getDriveSpinupTime()) {
				m_firstTrackMode = !(m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].current.ready || m_mfmRead[m_actualCurrentCylinder][1-(int)m_actualFloppySide].current.ready);
				m_motorSpinningUp = false;
				m_motorIsReady = true;
			}
		}


		if (supportsDiskChange()) 
			diskInDrive = getDiskChangeStatus(false);

		if (lastDiskState != diskInDrive) {
			// Erase track cache 
			if (!diskInDrive) {
				resetMFMCache();
				m_inHDMode = false;
			}
			else {
				m_writeProtected = checkWriteProtectStatus(true);
				// Request the type of disk before we let the OS know a disk has actually been inserted
				internalCheckDiskDensity(true);
			}

			m_diskInDrive = diskInDrive;
		}
	}
}

// Internally check the disk density
void CommonBridgeTemplate::internalCheckDiskDensity(bool newDiskInserted) {
	switch (m_bridgeDensity) {
	case BridgeDensityMode::bdmAuto:
		// Run the test
		if (m_diskInDrive || newDiskInserted) {
			// To do this properly we should be on track 0, lower side.
			setCurrentCylinder(0);
			setActiveSurface(DiskSurface::dsLower);

			checkDiskType();

			// Revert
			setActiveSurface(m_actualFloppySide);
			setCurrentCylinder(m_actualCurrentCylinder);
		}
		else {
			forceDiskDensity(false);
		}
		break;

	case BridgeDensityMode::bdmDDOnly:
		forceDiskDensity(false);
		break;

	case BridgeDensityMode::bdmHDOnly:
		forceDiskDensity(true);
		break;
	}
	m_inHDMode = _getDriveTypeID() == DriveTypeID::dti35HD;
}


// Reset the previously setup queue
void CommonBridgeTemplate::resetMFMCache() {
	std::lock_guard lock(m_switchBufferLock);

	for (int a = 0; a < MAX_CYLINDER_BRIDGE; a++)
		for (int c = 0; c < 2; c++) {
			m_mfmRead[a][c].startBitPatterns.valid = false;
			memset(&m_mfmRead[a][c].next, 0, sizeof(m_mfmRead[a][c].next));
			memset(&m_mfmRead[a][c].current, 0, sizeof(m_mfmRead[a][c].current));
			memset(&m_mfmRead[a][c].last, 0, sizeof(m_mfmRead[a][c].last));
		}
	resetWriteBuffer();
	m_extractor.newDisk(m_inHDMode);
	m_pll.reset();
	
	std::lock_guard lockbuff(m_readBufferAvailableLock);
	m_readBufferAvailable = false;

	m_writePending = false;
	m_writeComplete = false;
	m_lastWroteTo = -1;
}

// Scans the MFM data to see if this track should allow smart speed or not based on timing data
void CommonBridgeTemplate::checkSmartSpeed(const int cylinder, const DiskSurface side, MFMCache& track) {
	// By default, say "no"
	track.supportsSmartSpeed = false;

	// Typically copy protection is on the first 3 tracks anyway
	if (cylinder <= 3) return;    // this actually isn't needed for Lemmings etc, the code below catches it!

	// Step 1: Find the average speed
#ifdef HIGH_RESOLUTION_MODE
	uint64_t total = 0;
	for (unsigned int mfmPositionBits = 0; mfmPositionBits < track.amountReadInBits; mfmPositionBits++) {
		const int mfmPositionBit = 7 - (mfmPositionBits & 7);
		total += track.mfmBuffer[mfmPositionBits>>3].speed[mfmPositionBit];
	}
	total /= track.amountReadInBits;
#else
	uint64_t total = 0;
	unsigned int totalBytes = (track.amountReadInBits + 7) / 8;
	for (unsigned int byte = 0; byte < totalBytes; byte++)
		total += track.mfmBuffer[byte].speed;
	total /= totalBytes;
#endif
	// total=100 means exact proper normal speed

	// If a little too far out?
	if (total > 120) return;
	if (total < 80) return;

	// Step 2: Scan the data and see how much differs from this
	unsigned int differ = 0;
	const int threshold = 4;
#ifdef HIGH_RESOLUTION_MODE
	for (unsigned int mfmPositionBits = 0; mfmPositionBits < track.amountReadInBits; mfmPositionBits++) {
		const int mfmPositionBit = 7 - (mfmPositionBits & 7);
		if (abs((int)track.mfmBuffer[mfmPositionBits >> 3].speed[mfmPositionBit] - (int)total) > threshold) differ++;
	}
	total /= track.amountReadInBits;
#else
	for (unsigned int byte = 0; byte < totalBytes; byte++)
		if (abs((int)track.mfmBuffer[byte].speed - (int)total) > threshold) differ++;
#endif

	if (differ > 75) return;

	track.supportsSmartSpeed = true;
}

// Save a new disk side and switch it in if it can be
void CommonBridgeTemplate::saveNextBuffer(const int cylinder, const DiskSurface side) {

	// Save the new buffer
	{
		std::lock_guard lock(m_switchBufferLock);
		if (m_mfmRead[cylinder][(int)side].next.amountReadInBits) {
			m_mfmRead[cylinder][(int)side].next.ready = true;
		}
		else {
			// Shouldn't be able to ever get here
			m_mfmRead[cylinder][(int)side].next.ready = false;
		}
	}

	// Stop of the above blocked the buffer
	if (!m_mfmRead[cylinder][(int)side].next.ready) return;

	// Check if this track should allow smartSpeed
	if (m_useSmartSpeed) {
		checkSmartSpeed(cylinder, side, m_mfmRead[cylinder][(int)side].next);
	}

	// Go live now?
	if (!m_mfmRead[cylinder][(int)side].current.ready) {

		internalSwitchCylinder(cylinder, side);

		// This test *should* always be true
		if ((cylinder == m_currentTrack) && (side == m_floppySide)) {
			std::lock_guard lock(m_readBufferAvailableLock);
			m_readBufferAvailable = true;
			m_readBufferAvailableFlag.notify_one();
		}
	}
}

// Handle reading the disk data in the background while the queue is idle
void CommonBridgeTemplate::handleBackgroundDiskRead() {
	// Don't do anything until the motor is ready.  The flag that checks for disk change will also report spin ready status if the drive hasn't spun up properly yet
	if (!((m_motorIsReady) && (!m_motorSpinningUp))) {
		if (m_shouldAutoCache) handleBackgroundCaching();
		return;
	}

	// If we already have the next buffer full then stop
	if (m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next.ready) {
		if (m_shouldAutoCache) handleBackgroundCaching();
		return;
	}

	// Make sure the right surface is selected
	if (!setActiveSurface(m_actualFloppySide)) return;
	bool flipSide = false;

	// Restore the correct cylinder
	if (m_autocacheModifiedCurrentCylinder) {
		if (!setCurrentCylinder(m_actualCurrentCylinder)) return;
		m_autocacheModifiedCurrentCylinder = false;
	}

	// Switch this depending on whats going on
	m_extractor.setAlwaysUseIndex(m_firstTrackMode || (m_bridgeMode == BridgeMode::bmCompatible) || (m_bridgeMode == BridgeMode::bmStalling));
	{
		// Scope it
		MFMCache& trackData = m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next;
		trackData.amountReadInBits = 0;
		trackData.ready = false;

		m_driveStreamingData = true;
		bool revolutionExtracted = false;


		// Grab full revolutions if possible.
		ReadResponse r =  readData(m_pll, MFM_BUFFER_MAX_TRACK_LENGTH, trackData.mfmBuffer, m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].startBitPatterns,
			[this, &trackData, &flipSide, &revolutionExtracted](RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits) -> bool {
				trackData.amountReadInBits = dataLengthInBits;

				saveNextBuffer(m_actualCurrentCylinder, m_actualFloppySide);
				revolutionExtracted = true;

				// This is a little cache prediction, but could cause issues with things needing to re-read the same track over and over.  However eventually it will continue anyway
				if (m_bridgeMode != BridgeMode::bmStalling && m_bridgeMode != BridgeMode::bmCompatible && !m_inHDMode || m_firstTrackMode) {
					// So, as a little speed up.  Is the other side of this track in memory?
					if (!m_mfmRead[m_actualCurrentCylinder][1 - (int)m_actualFloppySide].current.ready) {
						// No.  Is anything else going on?
						if (m_queue.empty()) {
							// No. 
							flipSide = true;
							return false;
						}
					}
				}

				// If we have everything then stop
				if (!m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next.ready) return false;

				// Else read another revolution
				return m_queue.empty();

			});
		switch (r) {
			case ReadResponse::rrNoDiskInDrive:
				 m_diskInDrive = false;
				 m_delayStreaming = true;
				 m_delayStreamingStart = std::chrono::steady_clock::now();
				 resetMFMCache();
				 m_inHDMode = false;
				 break;

			case ReadResponse::rrOK:
				if (!m_diskInDrive) {
					m_diskInDrive = true;
					m_delayStreaming = false;
					m_lastDiskCheckTime = std::chrono::steady_clock::now();
					m_inHDMode = false;
				} else 
					if ((revolutionExtracted) && (!m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next.ready)) {
						
						// Try for a re-play
						MFMCache& trackData = m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next;
						trackData.amountReadInBits = 0;
						trackData.ready = false;

						m_pll.rePlayData(MFM_BUFFER_MAX_TRACK_LENGTH, trackData.mfmBuffer, m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].startBitPatterns,
							[this, &trackData, &flipSide, &revolutionExtracted](RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits) -> bool {
								trackData.amountReadInBits = dataLengthInBits;
								saveNextBuffer(m_actualCurrentCylinder, m_actualFloppySide);
								return false;
							});
					}

				break;
		}
	}

	// Did the reader want us to flip sides?
	if (flipSide && m_diskInDrive) {
		DiskSurface flipSurface = m_actualFloppySide == DiskSurface::dsLower ? DiskSurface::dsUpper : DiskSurface::dsLower;
		MFMCache& trackData = m_mfmRead[m_actualCurrentCylinder][(int)flipSurface].next;

		// Switch the read head
		if ((!trackData.ready) && (setActiveSurface(flipSurface))) {

			// If there still isn't any jobs to do, then start reading it.
			if (m_queue.empty()) {
				trackData.amountReadInBits = 0;

				// Let the world know what we're doing.
				m_isCurrentlyHeadCheating = true;

				unsigned int oldRevolutionTime = m_extractor.getRevolutionTime();
				if (m_firstTrackMode) m_extractor.newDisk(m_inHDMode);
				bool trackWasRead = false;

				// and go for it
				ReadResponse r = readData(m_pll, MFM_BUFFER_MAX_TRACK_LENGTH, trackData.mfmBuffer, m_mfmRead[m_actualCurrentCylinder][(int)flipSurface].startBitPatterns,
					[this, flipSurface, &trackData, &trackWasRead](RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits) -> bool {
						trackData.amountReadInBits = dataLengthInBits;
						saveNextBuffer(m_actualCurrentCylinder, flipSurface);
						trackWasRead = true;
						// We only want an initial one.
						return false;

					});
				switch (r) {
				case ReadResponse::rrNoDiskInDrive:
					m_diskInDrive = false;
					m_delayStreaming = true;
					m_delayStreamingStart = std::chrono::steady_clock::now();
					m_inHDMode = false;
					resetMFMCache();
					break;

				case ReadResponse::rrOK:
					if (!m_diskInDrive) {
						m_diskInDrive = true;
						m_delayStreaming = false;
						m_lastDiskCheckTime = std::chrono::steady_clock::now();
						m_inHDMode = false;
					}
					else {
						if (m_firstTrackMode) {
							if (trackWasRead) {
								// Average the speed of both revolutions
								unsigned int newSpeed = m_extractor.getRevolutionTime();
								m_extractor.setRevolutionTime((newSpeed + oldRevolutionTime) / 2);
							}
							else {
								// Apply the first one... This shouldn't happen
								m_extractor.setRevolutionTime(oldRevolutionTime);
							}
						}

						if (!m_mfmRead[m_actualCurrentCylinder][(int)flipSurface].next.ready) {
							// Try for a re-play
							trackData = m_mfmRead[m_actualCurrentCylinder][(int)flipSurface].next;
							trackData.amountReadInBits = 0;
							trackData.ready = false;

							m_pll.rePlayData(MFM_BUFFER_MAX_TRACK_LENGTH, trackData.mfmBuffer, m_mfmRead[m_actualCurrentCylinder][(int)flipSurface].startBitPatterns,
								[this, &trackData, &flipSurface](RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits) -> bool {
									trackData.amountReadInBits = dataLengthInBits;
									saveNextBuffer(m_actualCurrentCylinder, flipSurface);
									return false;
								});
						}
					}
					break;
				}				

				m_isCurrentlyHeadCheating = false;
			}
		}
	}

	// Reset this flag if we have the first cylinder recorded with at least 1 revolution
	if (m_firstTrackMode) m_firstTrackMode = !(m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].current.ready || m_mfmRead[m_actualCurrentCylinder][1 - (int)m_actualFloppySide].current.ready);

	// Stopped streaming
	m_driveStreamingData = false;

	// Just flag this so we don't get an immediate check
	m_lastDiskCheckTime = std::chrono::steady_clock::now();
}

// Return TRUE if we're at the INDEX marker - we fake the start of the buffer being where the index marker is.
bool CommonBridgeTemplate::isMFMPositionAtIndex(int mfmPositionBits) {
	if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
		return (mfmPositionBits == 0) || (mfmPositionBits == m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits);
	}
	return mfmPositionBits == 0;
}

// Return the maximum size of the internal track buffer in BITS
int CommonBridgeTemplate::maxMFMBitPosition() {
	if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready)
		if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits)
			return m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits;

	// If there is no buffer ready, it's difficult to tell WinUAE what it wants to know, as we don't either.  So we supply a absolute MINIMUM that *should* be available on a disk
	// As this is dynamically read each time it *shouldn't* be a problem and by the time it hopefully reaches it the buffer will have gone live
#ifdef _WIN32
	return max(THEORETICAL_MINIMUM_TRACK_LENGTH * 8, m_mfmRead[m_currentTrack][(int)m_floppySide].next.amountReadInBits);
#else
	return std::max(THEORETICAL_MINIMUM_TRACK_LENGTH * 8, m_mfmRead[m_currentTrack][(int)m_floppySide].next.amountReadInBits);
#endif
}

// This is called to switch to a different copy of the track so multiple revolutions can be read
void CommonBridgeTemplate::mfmSwitchBuffer(bool side) {
	switchDiskSide(side);
	internalSwitchCylinder(m_currentTrack, m_floppySide);
}

// This is called to switch to a different copy of the track so multiple revolutions can be read
void CommonBridgeTemplate::internalSwitchCylinder(const int cylinder, const DiskSurface side) {
	std::lock_guard lock(m_switchBufferLock);

	// Is there a new one?
	if (m_mfmRead[cylinder][(int)side].next.ready) {
		// New one is ready.  If the current one is valid swap it to old
		if (m_mfmRead[cylinder][(int)side].current.ready) {
			m_mfmRead[cylinder][(int)side].last = m_mfmRead[cylinder][(int)side].current;
		}

		// and replace it
		m_mfmRead[cylinder][(int)side].current = m_mfmRead[cylinder][(int)side].next;
		m_mfmRead[cylinder][(int)side].next.amountReadInBits = 0;
		m_mfmRead[cylinder][(int)side].next.ready = false;
	} else // No new one? Do we have an old one we're cycling around?
		if (m_mfmRead[cylinder][(int)side].last.ready) {
			std::swap(m_mfmRead[cylinder][(int)side].current,m_mfmRead[cylinder][(int)side].last);
		}

	if (m_writeCompletePending) {
		std::lock_guard write_lock(m_writeLockCompleteFlag);
		m_writeCompletePending = false;
		m_writeComplete = true;
		m_lastWroteTo = (cylinder * 2) + ((int)side);
	}
	else m_lastWroteTo = -1;
}

// Get the speed at this position.  1000=100%.  
int CommonBridgeTemplate::getMFMSpeed(const int mfmPositionBits) {
	if (mfmPositionBits < 0) return DRIVE_GARBAGE_SPEED;
	if (!isReady()) return DRIVE_GARBAGE_SPEED;

	if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
		if (m_lastWroteTo == ((m_currentTrack * 2) + (int)m_floppySide)) {
			// We just write to this, but to help with any re-tries, we'll slow this down
			if (m_mfmRead[m_currentTrack][(int)m_floppySide].next.ready) m_lastWroteTo = -1; else {
				// Force normal speed
				return 1000;
			}
		}

		// This shouldn't happen
		if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits < 1) return 1000;

		if (m_inHDMode) return 100; else
			if ((m_bridgeMode == BridgeMode::bmTurboAmigaDOS) || (((m_bridgeMode == BridgeMode::bmFast) || (m_bridgeMode == BridgeMode::bmCompatible)) && (m_mfmRead[m_currentTrack][(int)m_floppySide].current.supportsSmartSpeed))) return 100;

		const int modPositionBit = mfmPositionBits % m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits;

		// Get the 'bit' we're reading
		const int mfmPositionByte = modPositionBit >> 3;

#ifdef HIGH_RESOLUTION_MODE
		const int mfmPositionBit = 7 - (modPositionBit & 7);
		int speed = (10 * (int)(m_mfmRead[m_currentTrack][(int)m_floppySide].current.mfmBuffer[mfmPositionByte].speed[mfmPositionBit]));
#else
		int speed = (10 * (int)(m_mfmRead[m_currentTrack][(int)m_floppySide].current.mfmBuffer[mfmPositionByte].speed));
#endif
		if (speed < 700)  speed = 700;
		if (speed > 3000)  speed = 3000;
		
		return speed;
	}

	return DRIVE_GARBAGE_SPEED;
}

// Returns TRUE if data is ready and available
bool CommonBridgeTemplate::isMFMDataAvailable() {
	if (m_bridgeMode == BridgeMode::bmStalling) return true;
	return m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready;
}

// Read a single bit from the data stream.   this should call triggerReadWriteAtIndex where appropriate
bool CommonBridgeTemplate::getMFMBit(const int mfmPositionBits) {
	if (!isReady()) return DRIVE_GARBAGE_VALUE;
	if (mfmPositionBits < 0) return false; // weird, but this shouldn't happen

	// This shouldn't happen
	if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits < 1) return DRIVE_GARBAGE_VALUE;

	const int modPositionBit = mfmPositionBits % m_mfmRead[m_currentTrack][(int)m_floppySide].current.amountReadInBits;


	// Internally manage loops until the data is ready
	const int mfmPositionByte = modPositionBit >> 3;
	const int mfmPositionBit = 7 - (modPositionBit & 7);

	if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
		return (m_mfmRead[m_currentTrack][(int)m_floppySide].current.mfmBuffer[mfmPositionByte].mfmData & (1 << mfmPositionBit)) != 0;
	}

	// Given the reading is quick enough mostly this is ok.  It kinda simulates the drive settling time, just a little extended
	if (m_bridgeMode != BridgeMode::bmStalling) return DRIVE_GARBAGE_VALUE;

	// Try to wait for the data.  A full revolution should be about 200ms, but given its trying to INDEX align, worse case this could be approx 400ms.
	// Average should be 300ms based on starting to read when the head is at least half way from the index.
	const unsigned int DelayBetweenChecksMS = 5;
	const auto DelayBetweenChecks = std::chrono::milliseconds(DelayBetweenChecksMS);
	for (unsigned int a = 0; a < 450 / DelayBetweenChecksMS; a++) {
		// Causes a delay of 5ms or until a buffer is made live
		{
			std::unique_lock<std::mutex> lck(m_readBufferAvailableLock);
			m_readBufferAvailableFlag.wait_for(lck, DelayBetweenChecks, [this] { return m_readBufferAvailable; });
		}

		// No full buffer ready yet.
		if (m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
			// If a buffer is available, go for it!
			return (m_mfmRead[m_currentTrack][(int)m_floppySide].current.mfmBuffer[mfmPositionByte].mfmData & (1 << mfmPositionBit)) != 0;
		}
	}

	// If we get here, we give up and return nothing. So we behave like no data
	return DRIVE_GARBAGE_VALUE;
}

// Called if you do something which is classed as a manual check for disk presence etc
void CommonBridgeTemplate::updateLastManualCheckTime() {
	m_lastDiskCheckTime = std::chrono::steady_clock::now();
}

// Returns TRUE when the last command requested has completed
bool CommonBridgeTemplate::isReady() { 
	// All of these cause "not ready"
	return (m_motorIsReady) && (!m_motorSpinningUp) && (m_diskInDrive) && (!m_firstTrackMode);
};


// Handle caching track data in the background
void CommonBridgeTemplate::handleBackgroundCaching() {
	if (!m_shouldAutoCache) return;
	if (!m_queue.empty()) return;
	if (m_lastWroteTo >= 0) return;  // don't do this if a write is happening
	if (!m_diskInDrive) {
		// Turn off the motor if its not needed and we started it
		if (m_motorIsReady || m_motorSpinningUp) return;
		if (m_autoCacheMotorStatus) {
			m_autoCacheMotorStatus = false;
			setMotorStatus(false);
		}
		return;
	}

	const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_delayStreamingStart).count();
	if ((m_delayStreaming) && (timePassed < 100)) return;

	// Check if we should do this
	int nextCylinder;
	DiskSurface nextSurface;
	if (!getNextTrackToAutoCache(nextCylinder, nextSurface)) {
		// Nothing left to cache.
		
		if (m_motorIsReady || m_motorSpinningUp) return;
		// Turn off the motor if its not needed and we started it
		if (m_autoCacheMotorStatus) {
			m_autoCacheMotorStatus = false;
			setMotorStatus(false);
		}
		return;
	}

	// Does the OS think the motor is already running?
	if (!m_motorIsReady) {
		// It's on a spin-up.  Stop. It's the OS's turn now
		if (m_motorSpinningUp) return;

		// Have we internally turned the motor on?
		if (!m_autoCacheMotorStatus) {
			m_autoCacheMotorStatus = true;
			setMotorStatus(true);
			m_autoCacheMotorStart = std::chrono::steady_clock::now();
		}
	}

	// We can finally do something.
	if ((m_autoCacheMotorStatus) && (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_autoCacheMotorStart).count() > getDriveSpinupTime())) {
		// Lets read a single rotation
		if (!setActiveSurface(nextSurface)) return;
		if (!setCurrentCylinder(nextCylinder)) return;
		m_autocacheModifiedCurrentCylinder = true;
		m_extractor.setAlwaysUseIndex(true);

		// Scope it
		MFMCache& trackData = m_mfmRead[nextCylinder][(int)nextSurface].next;
		trackData.amountReadInBits = 0;
		trackData.ready = false;

		m_driveStreamingData = true;
		bool revolutionExtracted = false;

		// Grab full revolutions if possible.
		ReadResponse r = readData(m_pll, MFM_BUFFER_MAX_TRACK_LENGTH, trackData.mfmBuffer, m_mfmRead[nextCylinder][(int)nextSurface].startBitPatterns,
			[this, &revolutionExtracted , &trackData, nextCylinder, nextSurface](RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits) -> bool {
				trackData.amountReadInBits = dataLengthInBits;

				saveNextBuffer(nextCylinder, nextSurface);
				revolutionExtracted = true;

				// Stop
				return false;

			});

		// Re-play the data with jitter
		if (revolutionExtracted) {
			m_pll.rePlayData(MFM_BUFFER_MAX_TRACK_LENGTH, trackData.mfmBuffer, m_mfmRead[nextCylinder][(int)nextSurface].startBitPatterns,
				[this, &trackData, nextCylinder, nextSurface](RotationExtractor::MFMSample* mfmData, const unsigned int dataLengthInBits) -> bool {
					trackData.amountReadInBits = dataLengthInBits;

					saveNextBuffer(nextCylinder, nextSurface);

					// Stop
					return false;

				});
		}

		switch (r) {
		case ReadResponse::rrNoDiskInDrive:
			m_diskInDrive = false;
			m_delayStreaming = true;
			m_delayStreamingStart = std::chrono::steady_clock::now();
			resetMFMCache();
			m_inHDMode = false;
			break;

		case ReadResponse::rrOK:
			if (!m_diskInDrive) {
				m_diskInDrive = true;
				m_delayStreaming = false;
				m_lastDiskCheckTime = std::chrono::steady_clock::now();
				m_inHDMode = false;
			}
			break;
		}
		m_driveStreamingData = false;
		m_lastDiskCheckTime = std::chrono::steady_clock::now();
	}	
}

// Manage motor status
void CommonBridgeTemplate::internalSetMotorStatus(bool enabled) {
	if (m_shouldAutoCache) {
		// Do something different here
		if (enabled) {
			if (m_autoCacheMotorStatus) {
				// Do nothing, its coming online anyway
				return;
			}
			else {
				setMotorStatus(true);
				m_autoCacheMotorStatus = true;
				m_autoCacheMotorStart = std::chrono::steady_clock::now();
			}
		}
		else {
			// Switch off the motor eh?
			int c;
			DiskSurface s;
			// Is there still data to be read?
			if (getNextTrackToAutoCache(c, s)) {
				// Ignore it.
			}
			else {
				// Turn off the motor
				m_autoCacheMotorStatus = false;
				setMotorStatus(false);
			}
		}
	} 
	else {
		setMotorStatus(enabled);
		m_autoCacheMotorStatus = false;
	}
}

// Check if the motor should be turned off
void CommonBridgeTemplate::checkMotorOff() {
	if (!m_motorTurnOffEnabled) return;

	const auto timePassed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_motorTurnOffStart).count();
	// Time passed.  We can turn the drive back off
	if (timePassed >= getDriveSpinupTime()) {
		m_motorTurnOffEnabled = false;

		QueueInfo cmd{};
		cmd.command = QueueCommand::qcMotorOff;
		cmd.option.b = false;
		pushOntoQueue(cmd, true, true);
	}
}


// Handle processing the command
void CommonBridgeTemplate::processCommand(const QueueInfo& info) {
	// See what command is being ran
	switch (info.command) {
	case QueueCommand::qcResetDrive:
		// Delete all future writes
		{
			std::lock_guard lock(m_pendingWriteLock);
			m_pendingTrackWrites.clear();
		}
		m_writePending = false;
		m_writeComplete = false;
		m_autoCacheMotorStatus = false;
		setMotorStatus(false);
		internalSetMotorStatus(false);
		m_autoCacheMotorStatus = false;
		m_firstTrackMode = false;
		m_motorTurnOffEnabled = false;
		m_motorSpinningUp = false;
		m_writeCompletePending = false;
		m_motorIsReady = false;
		m_isMotorRunning = false;		
		resetMFMCache();
		{
			std::lock_guard lock(m_driveResetStatusFlagLock);
			m_driveResetStatus = true;
			m_driveResetStatusFlag.notify_one();
		}
		break;

	case QueueCommand::qcMotorOn:
		m_motorTurnOffEnabled = false;
		internalSetMotorStatus(true);
		m_firstTrackMode = false;
		m_motorSpinningUp = true;
		m_motorSpinningUpStart = std::chrono::steady_clock::now();
		break;

	case QueueCommand::qcNoClickSeek:
		if (m_actualCurrentCylinder == 0) {
			// If it fails, simulate it
			if (!performNoClickSeek()) {
				setCurrentCylinder(1);
				setCurrentCylinder(0);
			}
		}
		break;

	case QueueCommand::qcGotoToTrack:
		setCurrentCylinder(info.option.i);
		m_actualCurrentCylinder = info.option.i;
		m_autocacheModifiedCurrentCylinder = false;
		m_lastWroteTo = -1;
		break;

	case QueueCommand::qcMotorOffDelay:
		m_motorTurnOffEnabled = true;
		m_motorTurnOffStart = std::chrono::steady_clock::now();
		break;

	case QueueCommand::qcMotorOff:
		m_motorTurnOffEnabled = false;
		internalSetMotorStatus(false);
		m_motorSpinningUp = false;
		m_motorIsReady = false;
		m_lastWroteTo = -1;
		break;

	case QueueCommand::qcSelectDiskSide:
		m_actualFloppySide = info.option.b ? DiskSurface::dsUpper : DiskSurface::dsLower;
		setActiveSurface(m_actualFloppySide);
		m_lastWroteTo = -1;
		break;

	case QueueCommand::writeMFMData:
		// Grab the item
		TrackToWrite track;
		{
			std::lock_guard lock(m_pendingWriteLock);
			if (m_pendingTrackWrites.empty()) return;
			track = m_pendingTrackWrites.front();
			m_pendingTrackWrites.erase(m_pendingTrackWrites.begin());
		}
		// Is there data to write?
		if (track.floppyBufferSizeBits) {
			if (m_actualCurrentCylinder != track.trackNumber) {
				m_actualCurrentCylinder = track.trackNumber;
				setCurrentCylinder(track.trackNumber);
			}
			if (m_actualFloppySide != track.side) {
				m_actualFloppySide = track.side;
				setActiveSurface(track.side);
			}
			// This isn't great.  IF this fails it might be hard for WinUAE to know.
			writeData(track.mfmBuffer, track.floppyBufferSizeBits, track.writeFromIndex, m_actualCurrentCylinder >= WRITE_PRECOMP_START);				
			// Wipe the copies of the previous version of this
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].current.ready = false;
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].last.ready = false;
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next.ready = false;
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].current.supportsSmartSpeed = false;
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].last.supportsSmartSpeed = false;
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].next.supportsSmartSpeed = false;
			m_mfmRead[m_actualCurrentCylinder][(int)m_actualFloppySide].startBitPatterns.valid = false;
			m_delayStreaming = false;
			// Delay the completion until we've read a track back
			m_writeCompletePending = true;
			m_writePending = false;
		}

		break;

	default:
		break;
	}
}

// This is called prior to closing down, but should reverse initialise
void CommonBridgeTemplate::shutdown() {
	terminate();
	closeInterface();
}

// Called to reverse initialise.  This will automatically be called in the destructor too.
void CommonBridgeTemplate::terminate() {

	if (m_control) {
		queueCommand(QueueCommand::qcTerminate);

		if (m_control->joinable())
			m_control->join();

		delete m_control;
		m_control = nullptr;
	}

	m_lastError = "";
}

// Startup.  This will display an error message if there's an issue with the interface
bool CommonBridgeTemplate::initialise() {
	// Stop first
	if (m_control) shutdown();

	// Setup some initial values
	m_currentTrack = 0;
	m_isMotorRunning = false;
	m_motorIsReady = false;
	m_writeProtected = true;
	m_diskInDrive = false;
	m_inHDMode = false;
	m_autoCacheMotorStatus = false;
	m_actualFloppySide = DiskSurface::dsLower;
	m_floppySide = DiskSurface::dsLower;
	m_actualCurrentCylinder = m_currentTrack;

	// Clear down the queue
	{
		std::lock_guard lock(m_queueProtect);
		m_queue.clear();
	}
	{
		std::lock_guard lock(m_queueSemaphoreLock);
		m_queueSemaphore = 0;
	}
	
	m_lastError = "";

	// Setup
	if (openInterface(m_lastError)) {
		setMotorStatus(false);
		internalSetMotorStatus(false);
		m_autoCacheMotorStatus = false;
		setActiveSurface(m_actualFloppySide);

		// Get some real disk status values
		if (supportsDiskChange()) {
			m_diskInDrive = getDiskChangeStatus(true);
		}
		else {
			m_diskInDrive = attemptToDetectDiskChange();
		}
		m_writeProtected = checkWriteProtectStatus(true);
		// Request the type of disk before we let the OS know a disk has actually been inserted
		internalCheckDiskDensity(false); 

		// Startup the thread
		m_control = new std::thread([this]() {
#ifdef _WIN32
			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#else
			struct sched_param sch;
			int policy;
			pthread_getschedparam(pthread_self(), &policy, &sch);
			policy = SCHED_RR;
			sch.sched_priority = sched_get_priority_max(policy); // boost priority
			pthread_setschedparam(pthread_self(), policy, &sch);
#endif
			this->mainThread();
		});

		// Success
		return true;
	}

	return false;
}

// Call to get the last error message.  Length is the size of the buffer in TCHARs, the function returns the size actually needed
const char* CommonBridgeTemplate::getLastErrorMessage() {
	return m_lastError.c_str();
}

// Reset the drive.  This should reset it to the state it would be at power up
bool CommonBridgeTemplate::resetDrive(int trackNumber) {
	// Delete all future writes
	{
		std::lock_guard lock(m_pendingWriteLock);
		m_pendingTrackWrites.clear();
	} 

	// Reset flag
	{
		std::lock_guard lock(m_driveResetStatusFlagLock);
		m_driveResetStatus = false;
	}
	queueCommand(QueueCommand::qcResetDrive);

	// Wait for the reset to occur.  At this point we know the queue is also clear
	std::unique_lock lck(m_driveResetStatusFlagLock);
	m_driveResetStatusFlag.wait(lck, [this] { return m_driveResetStatus; });

	// Ready
	return true;
}

// Set the status of the motor. 
void CommonBridgeTemplate::setMotorStatus(bool side, bool turnOn) {
	switchDiskSide(side);

	if (m_isMotorRunning == turnOn) return;
	m_isMotorRunning = turnOn;

	m_motorIsReady = false;
	m_motorSpinningUp = false;

	if (turnOn) {
		{
			// Remove any turn off commands in the queue
			std::lock_guard lock(m_queueProtect);
			if (!m_queue.empty()) {
				switch (m_queue.back().command) {
				case QueueCommand::qcMotorOff:
				case QueueCommand::qcMotorOffDelay:
					m_queue.back().command = QueueCommand::qcNOP;
					break;
				}
			}
		}
	
		queueCommand(QueueCommand::qcMotorOn);
	}
	else {
		queueCommand(QueueCommand::qcMotorOffDelay);
	}

}

// Handle the drive stepping to track -1 - this is used to 'no-click' detect the disk
void CommonBridgeTemplate::handleNoClickStep(bool side) {
	switchDiskSide(side);

	queueCommand(QueueCommand::qcNoClickSeek);
}

// Seek to a specific track
void CommonBridgeTemplate::gotoCylinder(int trackNumber, bool side) {
	switchDiskSide(side);
	if (m_currentTrack == trackNumber) return;
	resetWriteBuffer();
	m_currentTrack = trackNumber;

	bool queueUpdated = false;
	

	// We want to see if there are other 'goto track' commands in the queue just before this one.  If there is, we can replace them
	{
		std::lock_guard lock(m_queueProtect);
		if (!m_queue.empty()) {
			if (m_queue.back().command == QueueCommand::qcGotoToTrack) {
				m_queue.back().option.i = trackNumber;
				queueUpdated = true;
				if (!m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
					std::lock_guard lock(m_readBufferAvailableLock);
					m_readBufferAvailable = false;
				}
			}
		}
	}

	// Nope? Well we'll just add it then
	if (!queueUpdated) {
		if (!m_mfmRead[m_currentTrack][(int)m_floppySide].current.ready) {
			std::lock_guard lock(m_readBufferAvailableLock);
			m_readBufferAvailable = false;
		}

		queueCommand(QueueCommand::qcGotoToTrack, trackNumber);
	}
}

// Reset and clear out any data we have received thus far
void CommonBridgeTemplate::resetWriteBuffer() {
	m_currentWriteTrack.writeFromIndex = false;
	m_currentWriteTrack.floppyBufferSizeBits = 0;
	m_currentWriteTrack.trackNumber = -1;
	m_currentWriteStartMfmPosition = 0;
}

// Submits a single WORD of data received during a DMA transfer to the disk buffer.  This needs to be saved.  It is usually flushed when commitWriteBuffer is called
// You should reset this buffer if side or track changes
void CommonBridgeTemplate::writeShortToBuffer(bool side, unsigned int track, unsigned short mfmData, int mfmPosition) {
	gotoCylinder(track, side);

	// Prevent background reading while we're busy
	m_delayStreaming = true;
	m_delayStreamingStart = std::chrono::steady_clock::now();
	abortDiskReading();	

	// Check there is enough space left.  Bytes to bits, then 16 bits for the next block of data
	if (m_currentWriteTrack.floppyBufferSizeBits < (MFM_BUFFER_MAX_TRACK_LENGTH * 8) - 16) {
		if (m_currentWriteTrack.floppyBufferSizeBits == 0) {
			m_writePending = false;
			m_writeComplete = false;
			m_writeCompletePending = false;

			m_currentWriteTrack.trackNumber = track;
			m_currentWriteTrack.side = side ? DiskSurface::dsUpper : DiskSurface::dsLower;
			m_currentWriteStartMfmPosition = mfmPosition;
		}
		m_currentWriteTrack.mfmBuffer[m_currentWriteTrack.floppyBufferSizeBits >> 3] = mfmData >> 8;
		m_currentWriteTrack.mfmBuffer[(m_currentWriteTrack.floppyBufferSizeBits >> 3) + 1] = mfmData & 0xFF;
		m_currentWriteTrack.floppyBufferSizeBits += 16;
	}	
}
 
// Return TRUE if there is data ready to be committed to disk
bool CommonBridgeTemplate::isReadyToWrite() {
	return (m_currentWriteTrack.floppyBufferSizeBits);
}


// Requests that any data received via writeShortToBuffer be saved to disk. The side and track should match against what you have been collected
// and the buffer should be reset upon completion.  You should return the new track length (maxMFMBitPosition) with optional padding if needed
unsigned int CommonBridgeTemplate::commitWriteBuffer(bool side, unsigned int track) {
	gotoCylinder(track, side);

	// Prevent background reading while we're busy
	m_delayStreaming = true;
	m_delayStreamingStart = std::chrono::steady_clock::now();
	abortDiskReading();

	// If there was data?
	if ((m_currentWriteTrack.floppyBufferSizeBits) && (m_currentWriteTrack.trackNumber == track) && (m_currentWriteTrack.side == (side ? DiskSurface::dsUpper : DiskSurface::dsLower))) {
		// Roughly accurate	
		m_currentWriteTrack.writeFromIndex = (m_currentWriteStartMfmPosition <= 30) || (m_currentWriteStartMfmPosition >= maxMFMBitPosition() - 30);

		// Add a small amount of data to the end
		if (m_currentWriteTrack.floppyBufferSizeBits < (MFM_BUFFER_MAX_TRACK_LENGTH * 8) - 16) {
			m_currentWriteTrack.mfmBuffer[m_currentWriteTrack.floppyBufferSizeBits >> 3] = 0x55;
			m_currentWriteTrack.mfmBuffer[(m_currentWriteTrack.floppyBufferSizeBits >> 3) + 1] = 0x55;
			m_currentWriteTrack.floppyBufferSizeBits += 8;
		}

		{
			std::lock_guard lock(m_pendingWriteLock);
			m_pendingTrackWrites.push_back(m_currentWriteTrack);
			m_writePending = true;
			queueCommand(QueueCommand::writeMFMData);
			{
				std::lock_guard lock2(m_switchBufferLock);

				// Prevent old data being read back by WinUAE
				MFMCaches* cache = &m_mfmRead[track][(int)m_floppySide];
				cache->current.ready = false;
				cache->last.ready = false;
				cache->next.ready = false;
				cache->startBitPatterns.valid = false;
			}
		}
	}

	resetWriteBuffer();

	return maxMFMBitPosition();
}


// Returns TRUE if commitWriteBuffer has been called but not written to disk yet
bool CommonBridgeTemplate::isWritePending() {
	return m_writePending;
}

// Returns TRUE if a write is no longer pending.  This should only return TRUE the first time, and then should reset
bool CommonBridgeTemplate::isWriteComplete() {
	std::lock_guard lock(m_writeLockCompleteFlag);

	bool ret = m_writeComplete;
	m_writeComplete = false;
	
	return ret;
}


