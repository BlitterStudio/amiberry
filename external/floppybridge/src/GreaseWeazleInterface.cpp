/* Greaseweazle C++ Interface for reading and writing Amiga Disks
*
* Copyright (C) 2021-2024 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This file is multi-licensed under the terms of the Mozilla Public
* License Version 2.0 as published by Mozilla Corporation and the
* GNU General Public License, version 2 or later, as published by the
* Free Software Foundation.
*
* MPL2: https://www.mozilla.org/en-US/MPL/2.0/
* GPL2: https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html
*
* Based on the excellent code by Keir Fraser <keir.xen@gmail.com>
* https://github.com/keirf/Greaseweazle/
* 
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

#include <sstream>
#include <vector>
#include <queue>
#include <cstring>
#include "GreaseWeazleInterface.h"
#include "RotationExtractor.h"
#include "pll.h"

#define ONE_NANOSECOND 1000000000UL
#define BITCELL_SIZE_IN_NS 2000L

#define PIN_DISKCHG_IBMPC	34
#define PIN_DISKCHG_SHUGART 2     // Greaseweazle cannot read from this pin
#define PIN_WRPROT			28

using namespace GreaseWeazle;

enum class GetInfo { Firmware = 0, BandwidthStats = 1 };
// ## Cmd.{Get,Set}Params indexes
enum class Params { Delays = 0 };
// ## Flux read stream opcodes, preceded by 0xFF byte
enum class FluxOp { Index = 1, Space = 2, Astable = 3};

#pragma pack(1) 

/* CMD_READ_FLUX */
struct GWReadFlux {
	/* Maximum ticks to read for (or 0, for no limit). */
	uint32_t ticks;
	/* Maximum index pulses to read (or 0, for no limit). */
	uint16_t max_index;
	/* Linger time, in ticks, to continue reading after @max_index pulses. */
	uint32_t max_index_linger; /* default: 500 microseconds */
};

/* CMD_WRITE_FLUX */
struct GWWriteFlux {
	/* If non-zero, start the write at the index pulse. */
	unsigned char cue_at_index;
	/* If non-zero, terminate the write at the next index pulse. */
	unsigned char terminate_at_index;
};

#pragma pack()

struct Sequence {
	unsigned char sequence;
	uint16_t speed;     // speed compared to what it should be (for this sequence)
	bool atIndex;			// At index
};

// We're working in nanoseconds
struct PLLData {
	uint32_t freq = 0;			  // sample frequency in Hz
	uint32_t ticks = 0;
	bool indexHit = false;
};


// Constructor for this class
GreaseWeazleInterface::GreaseWeazleInterface() : m_currentBusType(BusType::IBMPC), m_currentDriveIndex(0), m_diskInDrive(false),
												 m_isWriteProtected(true)
{
	memset(&m_gwVersionInformation, 0, sizeof(m_gwVersionInformation));
	memset(&m_gwDriveDelays, 0, sizeof(m_gwDriveDelays));
}

// Free me
GreaseWeazleInterface::~GreaseWeazleInterface() {
	closePort();
}
			
// Search and find the Greaseweazle COM port
std::wstring findPortNumber() {
	std::vector<SerialIO::SerialPortInformation> portList;
	SerialIO::enumSerialPorts(portList);

	// Scan for items
	std::wstring bestPort;
	int maxScore = 0;

	for (const SerialIO::SerialPortInformation& port : portList) {
		int score = 0;

		// Check Greaseweazle official VID/PID 
		if ((port.vid == 0x1209) && (port.pid == 0x4d69)) score += 20;
		else // old shared Test PID. It's not guaranteed to be Greaseweazle.
			if ((port.vid == 0x1209) && (port.pid == 0x0001)) {
				score += 10;
			}
		if (port.productName == L"Greaseweazle") score += 10;
		if (port.instanceID.find(L"\\GW") != std::wstring::npos) score += 10;
#ifdef __APPLE__
		if (port.portName.find(L".usbmodem") != std::wstring::npos) score += 10;
#endif		

		// Keep this port if its achieved a place on our high score table
		if (score > maxScore) {
			maxScore = score;
			bestPort = port.portName;
		}
	}

	return bestPort;
}

// Attempts to open the reader running on the COM port provided (or blank for auto-detect)
GWResponse GreaseWeazleInterface::openPort(const std::string& comPort, DriveSelection drive) {
	closePort();

	m_motorIsEnabled = false;

	m_inHDMode = false;

	std::wstring wComPort;
	quicka2w(comPort, wComPort);

	// Find and detect the port
	std::wstring gwPortNumber = wComPort.length() ? wComPort : findPortNumber();

	// If no device detected?
	if (gwPortNumber.empty()) return GWResponse::drPortNotFound;

	switch (m_comPort.openPort(gwPortNumber)) {
	case SerialIO::Response::rInUse:return GWResponse::drPortInUse;
	case SerialIO::Response::rNotFound:return GWResponse::drPortNotFound;
	case SerialIO::Response::rOK: break;
	default: return GWResponse::drPortError;
	}

	// Configure the port
	SerialIO::Configuration config;
	config.baudRate = 9600;
	config.ctsFlowControl = false;

	if (m_comPort.configurePort(config) != SerialIO::Response::rOK) return GWResponse::drPortError;

	applyCommTimeouts(false);

	m_comPort.purgeBuffers();

	// Lets ask GW it's firmware version
	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::GetInfo, (unsigned char)GetInfo::Firmware, response)) {
		m_comPort.purgeBuffers();

		if (!sendCommand(Cmd::GetInfo, (unsigned char)GetInfo::Firmware, response)) {
			closePort();
			return GWResponse::drErrorMalformedVersion;
		}
	};
	unsigned long read = m_comPort.read(&m_gwVersionInformation, sizeof(m_gwVersionInformation));
	if (read != 32) {
		closePort();
		return GWResponse::drErrorMalformedVersion;
	}
	if ((m_gwVersionInformation.major == 0) && (m_gwVersionInformation.minor < 27)) {
		closePort();
		return GWResponse::drOldFirmware;
	}
	if (m_gwVersionInformation.is_main_firmware == 0) {
		closePort();
		return GWResponse::drInUpdateMode;
	}

	// Reset it
	if (!sendCommand(Cmd::Reset, nullptr, 0, response)) {
		closePort();
		return GWResponse::drErrorMalformedVersion;
	};

	// Now get the drive delays
	if (!sendCommand(Cmd::GetParams, (unsigned char)Params::Delays, response, sizeof(m_gwDriveDelays))) {
		closePort();
		return GWResponse::drErrorMalformedVersion;
	};

	read = m_comPort.read(&m_gwDriveDelays, sizeof(m_gwDriveDelays));
	if (read != sizeof(m_gwDriveDelays)) {
		closePort();
		return GWResponse::drErrorMalformedVersion;
	};

	// Select the drive we want to communicate with
	switch (drive) {
	case DriveSelection::dsA:
		m_currentBusType = BusType::IBMPC;
		m_currentDriveIndex = 0;
		break;
	case DriveSelection::dsB:
		m_currentBusType = BusType::IBMPC;
		m_currentDriveIndex = 1;
		break;
	case DriveSelection::ds0:
		m_currentBusType = BusType::Shugart;
		m_currentDriveIndex = 0;
		break;
	case DriveSelection::ds1:
		m_currentBusType = BusType::Shugart;
		m_currentDriveIndex = 1;
		break;
	case DriveSelection::ds2:
		m_currentBusType = BusType::Shugart;
		m_currentDriveIndex = 2;
		break;
	case DriveSelection::ds3:
		m_currentBusType = BusType::Shugart;
		m_currentDriveIndex = 3;
		break;
	}
	
	if (!sendCommand(Cmd::SetBusType, (unsigned char)m_currentBusType, response)) {
		closePort();
		return GWResponse::drError;
	}

	// Do an initial probe to see whats available
	checkPins();

	// Firmware doesnt support the pin command
	if (!m_pinWrProtectAvailable) {
		closePort();
		return GWResponse::drOldFirmware;
	}

	// Ok, success
	return GWResponse::drOK;
}

// Change the drive delays as required
bool GreaseWeazleInterface::updateDriveDelays() {
	unsigned char buffer[11];
	buffer[0] = (unsigned char)Params::Delays;
#ifdef _WIN32	
	memcpy_s(buffer + 1, 10, &m_gwDriveDelays, sizeof(m_gwDriveDelays));
#else
	memcpy(buffer + 1, &m_gwDriveDelays, sizeof(m_gwDriveDelays));
#endif	

	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::SetParams, buffer, 11, response)) return false;

	return true;
}

// Trigger drive select
bool GreaseWeazleInterface::selectDrive(bool select) {
	Ack response = Ack::Okay;

	if (select == m_selectStatus) return true;

	if (select) {
		if (sendCommand(Cmd::Select, m_currentDriveIndex, response)) {
			m_selectStatus = select;
			return true;
		}
	}
	else {
		if (sendCommand(Cmd::Deselect, nullptr, 0, response)) {
			m_selectStatus = select;
			return true;
		}
	}
	return false;
}

// Apply and change the timeouts on the com port
void GreaseWeazleInterface::applyCommTimeouts(bool shortTimeouts) {
	m_comPort.setWriteTimeouts(2000, 200);

	if (shortTimeouts) {
		m_comPort.setReadTimeouts(15, 5);
	}
	else {
		m_comPort.setReadTimeouts(2000, 200);
	}		
}

// Closes the port down
void GreaseWeazleInterface::closePort() {
	enableMotor(false);
	
	m_comPort.closePort();
}

// Turns on and off the reading interface
GWResponse GreaseWeazleInterface::enableMotor(const bool enable, const bool dontWait) {
	Ack response = Ack::Okay;

	std::vector<unsigned char> params;

	unsigned int delay = dontWait ? 10 : 750;
	if (delay != m_gwDriveDelays.motor_delay) {
		m_gwDriveDelays.motor_delay = delay;
		updateDriveDelays();
	}
	
	unsigned char buffer[2] = { m_currentDriveIndex, ((unsigned char)(enable ? 1 : 0)) };
	if (!sendCommand(Cmd::Motor, buffer, 2, response)) {
		return GWResponse::drError;
	}

	if (response == Ack::Okay) {
		m_motorIsEnabled = enable;
		if (m_motorIsEnabled) selectDrive(true);
	}

	return (response == Ack::Okay) ? GWResponse::drOK : GWResponse::drError;
}

GWResponse GreaseWeazleInterface::performNoClickSeek() {
	Ack response = Ack::Okay;

	selectDrive(true);
	sendCommand(Cmd::NoClickStep, nullptr, 0, response);
	if (!m_motorIsEnabled) selectDrive(false);

	if (response == Ack::BadCommand) return GWResponse::drError;
	if (response == Ack::Okay) {
		if (!checkPins()) return GWResponse::drError;

		return GWResponse::drOK;
	} 
	else return GWResponse::drOldFirmware;
}

// Select the track, this makes the motor seek to this position
GWResponse GreaseWeazleInterface::selectTrack(const unsigned char trackIndex, const TrackSearchSpeed searchSpeed, bool ignoreDiskInsertCheck) {
	if (trackIndex > 81) {
		return GWResponse::drTrackRangeError; // no chance, it can't be done.
	}

	uint16_t newSpeed = m_gwDriveDelays.step_delay;
	switch (searchSpeed) {
		case TrackSearchSpeed::tssSlow:		newSpeed = 5000;  break;
		case TrackSearchSpeed::tssNormal:	newSpeed = 3000;  break;
		case TrackSearchSpeed::tssFast:		newSpeed = 3000;  break;
		case TrackSearchSpeed::tssVeryFast: newSpeed = 3000;  break;
	}
	if (newSpeed != m_gwDriveDelays.step_delay) {
		m_gwDriveDelays.step_delay = newSpeed;
		updateDriveDelays();
	}	

	selectDrive(true);

	Ack response = Ack::Okay;
	sendCommand(Cmd::Seek, trackIndex, response);
	if (!m_motorIsEnabled) selectDrive(false);

	// We have to check for a disk manually
	if (!ignoreDiskInsertCheck) {
		checkForDisk(true);
	}

	switch (response) {
	case Ack::NoTrk0: return GWResponse::drRewindFailure;
	case Ack::Okay: checkPins();  return GWResponse::drOK;
	default: return GWResponse::drSelectTrackError;
	}
}

// Checks the pins from boards that support it
bool GreaseWeazleInterface::checkPins() {
	Ack response;
	selectDrive(true);

	if ((!sendCommand(Cmd::GetPin, PIN_WRPROT, response)) || (response != Ack::Okay)) {
		m_pinWrProtectAvailable = false;
		if (response == Ack::BadCommand) return false;
	}
	else {
		unsigned char value = 0;
		unsigned long read = m_comPort.read(&value, 1);
		if (read == 1) {
			m_pinWrProtectAvailable = true;
			m_isWriteProtected = value == 0;
		}
	}

	if (m_currentBusType == BusType::Shugart) {
		m_pinDskChangeAvailable = false; // not supported by Greaseweazle on Pin 2
	}
	else {
		if ((!sendCommand(Cmd::GetPin, PIN_DISKCHG_IBMPC, response)) || (response != Ack::Okay)) {
			m_pinDskChangeAvailable = false;
			if (response == Ack::BadCommand) return false;
		}
		else {
			unsigned char value = 0;
			unsigned long read = m_comPort.read(&value, 1);
			if (read == 1) {
				m_pinDskChangeAvailable = true;
				m_diskInDrive = value == 1;
			}
		}
	}

	if (!m_motorIsEnabled) selectDrive(false);
	return true;
}

// Search for track 0
GWResponse GreaseWeazleInterface::findTrack0() {
	return selectTrack(0, TrackSearchSpeed::tssFast, true);
}

// Choose which surface of the disk to read from
GWResponse GreaseWeazleInterface::selectSurface(const DiskSurface side) {
	Ack response = Ack::Okay;
	sendCommand(Cmd::Head, (side == DiskSurface::dsUpper ? 1 : 0), response);

	return (response == Ack::Okay) ? GWResponse::drOK : GWResponse::drError;
}

// send a command out to the GW and receive its response.  Returns FALSE on error
bool GreaseWeazleInterface::sendCommand(Cmd command, void* params, unsigned int paramsLength, Ack& response, unsigned char extraResponseSize) {
	std::vector<unsigned char> data;
	data.resize(paramsLength + 2);
	data[0] = (unsigned char)command;
	data[1] = 2 + paramsLength + (extraResponseSize>0 ? 1 : 0);
	if ((params) && (paramsLength)) {
#ifdef _WIN32		
		memcpy_s(((unsigned char*)data.data()) + 2, paramsLength, params, paramsLength);
#else
		memcpy(((unsigned char*)data.data()) + 2, params, paramsLength);
#endif		
	}
	if (extraResponseSize>0) data.push_back(extraResponseSize);

	// Write request
	unsigned long written = (unsigned long)m_comPort.write(data.data(), (unsigned int)data.size());
	if (written != data.size()) {
		response = Ack::BadCommand;
		return false;
	}
	
	// Read response
	unsigned char responseMsg[2];
	unsigned long read = m_comPort.read(&responseMsg, 2);
	if (read != 2) {
		if (read == 0) {
			// Try again
			read = m_comPort.read(&responseMsg, 2);
		}
		if (read != 2) {
			response = Ack::BadCommand;
			return false;
		}
	}

	response = (Ack)responseMsg[1];

	// Was it the response to the command we issued?
	if (responseMsg[0] != (unsigned char)command) {
		response = Ack::BadCommand;
		return false;
	}

	return true;
}
bool GreaseWeazleInterface::sendCommand(Cmd command, const std::vector<unsigned char>& params, Ack& response, unsigned char extraResponseSize) {
	return sendCommand(command, (void*)params.data(), (unsigned int)params.size(), response, extraResponseSize);
}
bool GreaseWeazleInterface::sendCommand(Cmd command, unsigned char param, Ack& response, unsigned char extraResponseSize) {
	return sendCommand(command, (void*)(&param), 1, response, extraResponseSize);
}

// Taken from optimised.c and modified
static int read_28bit(std::queue<unsigned char>& queue) {
	int x;
	x = queue.front() >> 1;  queue.pop();
	x |= (queue.front() & 0xfe) << 6; queue.pop();
	x |= (queue.front() & 0xfe) << 13; queue.pop();
	x |= (queue.front() & 0xfe) << 20; queue.pop();

	return x;
}

// Convert from nSec to TICKS
static unsigned int nSecToTicks(unsigned int nSec, unsigned int sampleFrequency) {
	return ((unsigned long long)nSec * (unsigned long long)sampleFrequency) / (unsigned long long)ONE_NANOSECOND;
}

// Convert from TICKS to nSec
static unsigned int ticksToNSec(unsigned int ticks, unsigned int sampleFrequency) {
	return ((unsigned long long)ticks * ONE_NANOSECOND) / (unsigned long long)sampleFrequency;
}

// Look at the stream and count up the types of data
void countSampleTypes(PLLData& pllData, std::queue<unsigned char>& queue, unsigned int& hdBits, unsigned int& ddBits) {
	while (!queue.empty()) {

		unsigned char i = queue.front();
		if (i == 255) {
			if (queue.size() < 6) return;
			queue.pop();

			// Get opcode
			switch ((FluxOp)queue.front()) {
			case FluxOp::Index:
				queue.pop();
				read_28bit(queue);
				break;

			case FluxOp::Space:
				queue.pop();
				pllData.ticks += read_28bit(queue);
				break;

			default:
				queue.pop();
				break;
			}
		}
		else {
			if (i < 250) {
				pllData.ticks += (int)i;
				queue.pop();
			}
			else {
				if (queue.size() < 2) return;
				queue.pop();
				pllData.ticks += (250 + (((unsigned int)i) - 250) * 255) + (queue.front() - 1);
				queue.pop();
			}

			// Work out the actual time this tick took in nanoSeconds.
			unsigned int tickInNS = ticksToNSec(pllData.ticks, pllData.freq);
			if (tickInNS > BITCELL_SIZE_IN_NS) {
				if (tickInNS < 3000) hdBits++; 
				if ((tickInNS > 4500) && (tickInNS < 8000)) ddBits++;
				pllData.ticks = 0;
			}
		}
	}
}

// Test the inserted disk and see if its HD or not
GWResponse GreaseWeazleInterface::checkDiskCapacity(bool& isHD) {
	GWReadFlux header{};
	header.ticks = 0;
	header.max_index = 1;
	header.max_index_linger = nSecToTicks(50, m_gwVersionInformation.sample_freq);

	bool alreadySpun = m_motorIsEnabled;
	if (!alreadySpun)
		if (enableMotor(true, false) != GWResponse::drOK) return GWResponse::drError;

	selectDrive(true);

	// Write request
	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::ReadFlux, (void*)&header, sizeof(header), response)) {
		selectDrive(false);
		return GWResponse::drError;
	}

	// Buffer to read into
	unsigned char tempReadBuffer[64] = { 0 };
	bool zeroDetected = false;
	unsigned int failCount = 0;

	// Read the flux data
	PLLData pllData;
	pllData.freq = m_gwVersionInformation.sample_freq;
	unsigned int hdBits = 0;
	unsigned int ddBits = 0;

	std::queue<unsigned char> queue;

	do {
		// More efficient to read several bytes in one go		
		unsigned long bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		unsigned long bytesRead = m_comPort.read(tempReadBuffer, m_shouldAbortReading ? 1 : bytesAvailable);
		if (bytesRead < 1) {
			failCount++;
			if (failCount > 10) break;
		}
		else failCount = 0;

		if (bytesRead) {

			for (unsigned int a = 0; a < bytesRead; a++) {
				queue.push(tempReadBuffer[a]);
				zeroDetected |= tempReadBuffer[a] == 0;
			}

			// Look at the stream and count up the types of data
			countSampleTypes(pllData, queue, hdBits, ddBits);
		}		
	} while (!zeroDetected);

	// Check for errors
	response = Ack::Okay;
	sendCommand(Cmd::GetFluxStatus, nullptr, 0, response);

	selectDrive(false);

	if (!alreadySpun) enableMotor(false, false);

	isHD = (hdBits > ddBits);
	return GWResponse::drOK;
}

// Read RAW data from the current track and surface 
GWResponse GreaseWeazleInterface::checkForDisk(bool force) {
	if (force)
		if (!checkPins()) return GWResponse::drError;

	if (m_pinDskChangeAvailable) {
		return m_diskInDrive ? GWResponse::drOK : GWResponse::drNoDiskInDrive;
	}

	if (force) {
		GWReadFlux header;
		header.ticks = 0;
		header.max_index = 2;
		header.max_index_linger = 0;

		bool alreadySpun = m_motorIsEnabled;
		if (!alreadySpun)
			if (enableMotor(true, false) != GWResponse::drOK) return GWResponse::drOK;

		selectDrive(true);

		// Write request
		Ack response = Ack::Okay;
		if (!sendCommand(Cmd::ReadFlux, (void*)&header, sizeof(header), response)) {
			selectDrive(false);
			if (response == Ack::BadCommand) return GWResponse::drError;
			return GWResponse::drOK;
		}

		for (;;) {
			// Read a single byte
			unsigned char byte;
			if (m_comPort.read(&byte, 1))
				if (byte == 0) break;
		};

		// Check for errors
		response = Ack::Okay;
		sendCommand(Cmd::GetFluxStatus, nullptr, 0, response);
		if (response == Ack::BadCommand) return GWResponse::drError;
		selectDrive(false);

		if (!alreadySpun) enableMotor(false, false);

		m_diskInDrive = response != Ack::NoIndex;
	}
	return m_diskInDrive ? GWResponse::drOK : GWResponse::drNoDiskInDrive;
}

// Read a bit from the MFM data provided
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

// Write 28bit value ot the output
void write28bit(int value, std::vector<unsigned char>& output) {
	output.push_back(1 | ((value << 1) & 255));
	output.push_back(1 | ((value >> 6) & 255));
	output.push_back(1 | ((value >> 13) & 255));
	output.push_back(1 | ((value >> 20) & 255));
}

// Write data to the disk
GWResponse GreaseWeazleInterface::writeCurrentTrackPrecomp(const unsigned char* mfmData, const uint16_t numBytes, const bool writeFromIndexPulse, bool usePrecomp) {
	std::vector<unsigned char> outputBuffer;

	// Original data was written from MSB down to LSB
	int pos = 0;
	int bit = 7;
	unsigned char sequence = 0xAA;  // start at 10101010

	const int nfa_thresh = (int)((float)150e-6 * (float)m_gwVersionInformation.sample_freq);
	const int nfa_period = (int)((float)1.25e-6 * (float)m_gwVersionInformation.sample_freq);
	const int precompTime = m_inHDMode ? 70 : 140;   // Amiga default precomp amount (140ns)
	int extraTimeFromPrevious = 0;

	// Re-encode the data into our format and apply precomp. 
	while (pos < numBytes+8) {
		int b, count = 0;

		// See how many zero bits there are before we hit a 1
		do {
			b = readBit(mfmData, numBytes, pos, bit);
			sequence = ((sequence << 1) & 0x7F) | b;
			count++;
		} while (((sequence & 0x08) == 0) && (pos < numBytes + 8));  // the +8 is for safety

		// Validate range
		if (count < 1) count = 1;  //
		if (count > 5) count = 5;  // max we support 01, 001, 0001, 00001
		
		// Calculate the time for this in nanoseconds
		int timeInNS = extraTimeFromPrevious + (count * (m_inHDMode ? 1000 : 2000));     // 2=4000, 3=6000, 4=8000, (5=10000 although is isn't strictly allowed)

		if (usePrecomp) {
			const unsigned char BitSeq5 = (sequence & 0x3E);  // extract these bits 00111110 - bit 3 is the ACTUAL bit
			// Updated from https://github.com/keirf/greaseweazle/blob/master/src/greaseweazle/track.py
			// The actual idea is that the magnetic fields will repel each other, so we push them closer hoping they will land in the correct spot!
			switch (BitSeq5) {
			case 0x28:     // xx10100x
				timeInNS -= precompTime;
				extraTimeFromPrevious = precompTime;
				break;
			case 0x0A:     // xx00101x
				timeInNS += precompTime;
				extraTimeFromPrevious = -precompTime;
				break;
			default: 
				{
					const unsigned char BitSeq3 = (sequence & 0x1C);  // extract these bits 00011100 - bit 3 is the ACTUAL bit
					switch (BitSeq3) {
					case 0x18:   // xxx110xx
						timeInNS -= precompTime;
						extraTimeFromPrevious = precompTime;
						break;
					case 0x0C:   // xxx011xx
						timeInNS += precompTime;
						extraTimeFromPrevious = precompTime;
						break;
					default:
						extraTimeFromPrevious = 0;
						break;
					}
					break;
				}
			}
		}

		const int ticks = nSecToTicks(timeInNS, m_gwVersionInformation.sample_freq);
		
		// Encode for GW
		if (ticks>0) {
			if (ticks < 250) {
				outputBuffer.push_back(ticks);
			}
			else {
				if (ticks > nfa_thresh) {
					outputBuffer.push_back(255);
					outputBuffer.push_back((unsigned char)FluxOp::Space);   // Space
					write28bit(ticks, outputBuffer);
					outputBuffer.push_back(255);
					outputBuffer.push_back((unsigned char)FluxOp::Astable);   // Space
					outputBuffer.push_back(nfa_period);
				}
				else {
					unsigned int high = (ticks - 250) / 255;
					if (high < 5) {
						outputBuffer.push_back(250 + high);
						outputBuffer.push_back(1 + (ticks - 250) % 255);
					}
					else {
						outputBuffer.push_back(255);
						outputBuffer.push_back((unsigned char)FluxOp::Space);   // Space
						write28bit(ticks - 249, outputBuffer);
						outputBuffer.push_back(249);
					}
				}
			}
		}
	}
	
	outputBuffer.push_back(0); // finish	

	selectDrive(true);

	// Header
	GWWriteFlux header;
	header.cue_at_index = writeFromIndexPulse ? 1 : 0;
	header.terminate_at_index = 0;

	// Write request
	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::WriteFlux, (void*)&header, sizeof(header), response)) {
		if (!m_motorIsEnabled) selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}

	switch (response) {
		case Ack::Wrprot:
			if (!m_motorIsEnabled) selectDrive(false);
			return GWResponse::drWriteProtected;

		case Ack::Okay: 
			break;

		default:
			if (!m_motorIsEnabled) selectDrive(false);
			return GWResponse::drReadResponseFailed;
	}

	// Write it
	unsigned long write = m_comPort.write(outputBuffer.data(), (unsigned int)outputBuffer.size());
	if (write != outputBuffer.size()) {
		if (!m_motorIsEnabled) selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}

	// Sync with GW
	unsigned char sync;
	unsigned long read = m_comPort.read(&sync, 1);
	if (read != 1) {
		selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}

	// Check the value of SYNC
	response = Ack::Okay;
	sendCommand(Cmd::GetFluxStatus, nullptr, 0, response);

	selectDrive(false);

	switch (response) {
	case Ack::FluxUnderflow: return GWResponse::drSerialOverrun;
	case Ack::Wrprot: m_isWriteProtected = true;  return GWResponse::drWriteProtected;
	case Ack::Okay: return GWResponse::drOK;
	default: return GWResponse::drReadResponseFailed;
	}
}

// Process queue and work out whats going on
inline void unpackStreamQueue(std::queue<unsigned char>& queue, PLLData& pllData, PLL::BridgePLL& pll, bool isHDMode) {
	// Run until theres not enough data left to process
	while (!queue.empty()) {

		unsigned char i = queue.front();
		if (i == 255) {
			if (queue.size() < 6) return;
			queue.pop();

			// Get opcode
			switch ((FluxOp)queue.front()) {
			case FluxOp::Index:
				queue.pop();
				read_28bit(queue);
				pllData.indexHit = true;
				break;

			case FluxOp::Space:
				queue.pop();
				pllData.ticks += read_28bit(queue);
				break;

			default:
				queue.pop();
				break;
			}
		}
		else {
			if (i < 250) {
				pllData.ticks += (int32_t)i;
				queue.pop();
			}
			else {
				if (queue.size() < 2) return;
				queue.pop();				
				pllData.ticks += (250 + (((int32_t)i) - 250) * 255) + ((int32_t)queue.front() - 1);
				queue.pop();
			}

			// Work out the actual time this tick took in nanoSeconds.
			uint32_t tickInNS = ticksToNSec(pllData.ticks, pllData.freq);

			// This is how the Amiga expects it
			if (isHDMode) tickInNS *= 2;

			pll.submitFlux(tickInNS, pllData.indexHit);
			pllData.indexHit = false;
			pllData.ticks = 0;
		}
	}

}

// Reads "enough" data to extract data from the disk. This doesnt care about creating a perfect revolution - pll should have the LinearExtractor configured
GWResponse GreaseWeazleInterface::readData(PLL::BridgePLL& pll) {
	GWReadFlux header;

	// 220ms is long enough for even the worst drives. If a revolution takes longer than this then theres a problem anyway
	header.ticks = nSecToTicks(220 * 1000 * 1000, m_gwVersionInformation.sample_freq);
	header.max_index = 0;
	header.max_index_linger = 0;

	m_shouldAbortReading = false;

	// Sample storage
	std::queue<unsigned char> queue;

	// Reset ready for extraction	
	pll.rotationExtractor()->reset(m_inHDMode);

	selectDrive(true);

	// Write request
	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::ReadFlux, (void*)&header, sizeof(header), response)) {
		if (!m_motorIsEnabled) selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}
	int32_t failCount = 0;

	// Buffer to read into
	unsigned char tempReadBuffer[64] = { 0 };
	bool zeroDetected = false;

	applyCommTimeouts(true);

	PLLData pllData;
	pllData.freq = m_gwVersionInformation.sample_freq;

	do {
		// More efficient to read several bytes in one go		
		uint32_t bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		uint32_t bytesRead = m_comPort.read(tempReadBuffer, m_shouldAbortReading ? 1 : bytesAvailable);
		if (bytesRead < 1) {
			failCount++;
			if (failCount > 10) break;
		}
		else failCount = 0;

		// If theres this many we can process as this is the maximum we need to process
		if ((!m_shouldAbortReading) && (bytesRead)) {

			for (uint32_t a = 0; a < bytesRead; a++) {
				queue.push(tempReadBuffer[a]);
				zeroDetected |= tempReadBuffer[a] == 0;
			}

			unpackStreamQueue(queue, pllData, pll, m_inHDMode);			
		}
		else {
			for (uint32_t a = 0; a < bytesRead; a++) zeroDetected |= tempReadBuffer[a] == 0;
		}
	} while (!zeroDetected);

	applyCommTimeouts(false);

	// Check for errors
	response = Ack::Okay;
	sendCommand(Cmd::GetFluxStatus, nullptr, 0, response);

	if (!m_motorIsEnabled) selectDrive(false);

	// Update this flag
	m_diskInDrive = response != Ack::NoIndex;

	switch (response) {
	case Ack::FluxOverflow: return GWResponse::drSerialOverrun;
	case Ack::NoIndex: return GWResponse::drNoDiskInDrive;
	case Ack::Okay: return GWResponse::drOK;
	default: return  GWResponse::drReadResponseFailed;
	}
}

// Reads a complete rotation of the disk, and returns it using the callback function which can return FALSE to stop
// An instance of RotationExtractor is required.  This is purely to save on re-allocations.  It is internally reset each time
// This is slower than the above because this one focuses on an accurate rotation image of the data rather than just a stream
GWResponse GreaseWeazleInterface::readRotation(PLL::BridgePLL& pll, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns,
	std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation) {	
	GWReadFlux header;

	const uint32_t extraTime = OVERLAP_SEQUENCE_MATCHES * (OVERLAP_EXTRA_BUFFER) * 8000;  // this is approx 49mS
	if ((pll.rotationExtractor()->isInIndexMode()) || (!pll.rotationExtractor()->hasLearntRotationSpeed())) {
		header.ticks = 0;
		header.max_index = 1;
		header.max_index_linger = nSecToTicks((210 * 1000 * 1000) + extraTime, m_gwVersionInformation.sample_freq);
	}
	else {
		header.ticks = nSecToTicks((210 * 1000 * 1000) + extraTime, m_gwVersionInformation.sample_freq);
		header.max_index = 0;
		header.max_index_linger = 0;
	}
	
	m_shouldAbortReading = false;

	// Sample storage
	std::queue<unsigned char> queue;

	// Reset ready for extraction
	pll.prepareExtractor(m_inHDMode, startBitPatterns);

	selectDrive(true);

	// Write request
	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::ReadFlux, (void*)&header, sizeof(header), response)) {
		if (!m_motorIsEnabled) selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}
	int32_t failCount = 0;

	// Buffer to read into
	unsigned char tempReadBuffer[64] = { 0 };
	bool zeroDetected = false;

	applyCommTimeouts(true);

	PLLData pllData;
	pllData.freq = m_gwVersionInformation.sample_freq;

	do {
		// More efficient to read several bytes in one go		
		uint32_t bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		uint32_t bytesRead = m_comPort.read(tempReadBuffer, m_shouldAbortReading ? 1 : bytesAvailable);
		if (bytesRead<1) {
			failCount++;
			if (failCount > 10) break;
		}
		else failCount = 0;

		// If theres this many we can process as this is the maximum we need to process
		if ((!m_shouldAbortReading) && (bytesRead)) {

			for (uint32_t a = 0; a < bytesRead; a++) {
				queue.push(tempReadBuffer[a]);
				zeroDetected |= tempReadBuffer[a] == 0;
			}

			unpackStreamQueue(queue, pllData, pll, m_inHDMode);

			// Is it ready to extract?
			if (pll.canExtract()) {
				uint32_t bits = 0;
				// Go!
				if (pll.extractRotation(firstOutputBuffer, bits, maxOutputSize)) {
					if (!onRotation(&firstOutputBuffer, bits)) {
						// And if the callback says so we stop. - unsupported at present
						abortReadStreaming();
					}
					// Always save this back
					pll.getIndexSequence(startBitPatterns);
				}
			}
		}
		else {
			for (uint32_t a = 0; a < bytesRead; a++) zeroDetected |= tempReadBuffer[a] == 0;
		}
	} while (!zeroDetected);

	applyCommTimeouts(false);

	// Check for errors
	response = Ack::Okay;
	sendCommand(Cmd::GetFluxStatus, nullptr, 0, response);

	if (!m_motorIsEnabled) selectDrive(false);

	// Update this flag
	m_diskInDrive = response != Ack::NoIndex;

	switch (response) {
	case Ack::FluxOverflow: return GWResponse::drSerialOverrun;
	case Ack::NoIndex: return GWResponse::drNoDiskInDrive;
	case Ack::Okay: return GWResponse::drOK;
	default: return  GWResponse::drReadResponseFailed;
	}
}

// Attempt to abort reading
void GreaseWeazleInterface::abortReadStreaming() {
	m_shouldAbortReading = true;
}

