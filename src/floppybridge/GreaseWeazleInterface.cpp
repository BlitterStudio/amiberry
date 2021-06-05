/* Greaseweazle C++ Interface for reading and writing Amiga Disks
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

#include <sstream>
#include <vector>
#include <queue>
#include <cstring>
#include "GreaseWeazleInterface.h"
#include "RotationExtractor.h"

#define ONE_NANOSECOND 1000000000UL
#define BITCELL_SIZE_IN_NS 2000L

#define PIN_DISKCHG			34
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
	unsigned long ticks;
	/* Maximum index pulses to read (or 0, for no limit). */
	unsigned short max_index;
	/* Linger time, in ticks, to continue reading after @max_index pulses. */
	unsigned long max_index_linger; /* default: 500 microseconds */
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
	unsigned short speed;     // speed compared to what it should be (for this sequence)
	bool atIndex;			// At index
};

// We're working in nanoseconds
struct PLLData {
	unsigned long freq = 0;			  // sample frequency in Hz
	unsigned long ticks = 0;
	bool indexHit = false;
};


// Constructor for this class
GreaseWeazleInterface::GreaseWeazleInterface() {
	m_diskInDrive = false;

	m_currentBusType = BusType::IBMPC;
	m_currentDriveIndex = 0;

	memset(&m_gwVersionInformation, 0, sizeof(m_gwVersionInformation));
	memset(&m_gwDriveDelays, 0, sizeof(m_gwDriveDelays));

	m_isWriteProtected = true;
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
	std::wstring bestPort = L"";
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

		// Keep this port if its achieved a place on our high score table
		if (score > maxScore) {
			maxScore = score;
			bestPort = port.portName;
		}
	}

	return bestPort;
}

// Attempts to open the reader running on the COM port number provided.  Port MUST support 2M baud
GWResponse GreaseWeazleInterface::openPort(bool useDriveA) {
	closePort();

	m_motorIsEnabled = false;

	// Find and detect the port
	std::wstring gwPortNumber = findPortNumber();

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
	m_currentBusType = BusType::IBMPC;
	m_currentDriveIndex = useDriveA ? 0 : 1;

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

	if (select) {
		return sendCommand(Cmd::Select, m_currentDriveIndex, response);
	}
	else {
		return sendCommand(Cmd::Deselect, nullptr, 0, response);
	}
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

	if (response == Ack::Okay) m_motorIsEnabled = enable;

	return (response == Ack::Okay) ? GWResponse::drOK : GWResponse::drError;
}

// Select the track, this makes the motor seek to this position
GWResponse GreaseWeazleInterface::selectTrack(const unsigned char trackIndex, const TrackSearchSpeed searchSpeed, bool ignoreDiskInsertCheck) {
	if (trackIndex > 81) {
		return GWResponse::drTrackRangeError; // no chance, it can't be done.
	}

	unsigned short newSpeed = m_gwDriveDelays.step_delay;
	switch (searchSpeed) {
		case TrackSearchSpeed::tssSlow:		newSpeed = 8000;  break;
		case TrackSearchSpeed::tssNormal:	newSpeed = 7000;  break;
		case TrackSearchSpeed::tssFast:		newSpeed = 6000;  break;
		case TrackSearchSpeed::tssVeryFast: newSpeed = 5000;  break;
	}
	if (newSpeed != m_gwDriveDelays.step_delay) {
		m_gwDriveDelays.step_delay = newSpeed;
		updateDriveDelays();
	}	

	selectDrive(true);

	Ack response = Ack::Okay;
	sendCommand(Cmd::Seek, trackIndex, response);
	selectDrive(false);

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
void GreaseWeazleInterface::checkPins() {
	Ack response;
	selectDrive(true);

	if ((!sendCommand(Cmd::GetPin, PIN_WRPROT, response)) || (response != Ack::Okay)) {
		m_pinWrProtectAvailable = false;
	}
	else {
		unsigned char value = 0;
		unsigned long read = m_comPort.read(&value, 1);
		if (read == 1) {
			m_pinWrProtectAvailable = true;
			m_isWriteProtected = value == 0;
		}
	}

	if ((!sendCommand(Cmd::GetPin, PIN_DISKCHG, response)) || (response != Ack::Okay)) {
		m_pinDskChangeAvailable = false;
	}
	else {
		unsigned char value = 0;
		unsigned long read = m_comPort.read(&value, 1);
		if (read == 1) {
			m_pinDskChangeAvailable = true;
			m_diskInDrive = value == 1;
		}
	}

	selectDrive(false);
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
		response = Ack::BadCommand;
		return false;
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

// Read RAW data from the current track and surface 
GWResponse GreaseWeazleInterface::checkForDisk(bool force) {
	if (force) checkPins();

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
GWResponse GreaseWeazleInterface::writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp) {
	std::vector<unsigned char> outputBuffer;

	// Original data was written from MSB downto LSB
	int pos = 0;
	int bit = 7;
	unsigned char sequence = 0xAA;  // start at 10101010

	const int nfa_thresh = (int)((float)150e-6 * (float)m_gwVersionInformation.sample_freq);
	const int precompTime = 140;   // Amiga default precomp amount (140ns)
	int extraTimeFromPrevious = 0;

	// Re-encode the data into our format and apply precomp.  The +1 is to ensure theres some padding around the edge 
	while (pos < numBytes + 1) {
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
		
		// Calculate the time for this in nanoseconds
		int timeInNS = extraTimeFromPrevious + (count * 2000);     // 2=4000, 3=6000, 4=8000, (5=10000 although is isnt strictly allowed)

		if (usePrecomp) {
			switch (sequence) {
			case 0x09:
			case 0x0A:
			case 0x4A: // early;
				timeInNS -= precompTime;
				extraTimeFromPrevious = precompTime;
				break;

			case 0x28:
			case 0x29:
			case 0x48: // late
				timeInNS += precompTime;
				extraTimeFromPrevious = -precompTime;
				break;

			default:
				extraTimeFromPrevious = 0;
				break;
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
					outputBuffer.push_back((unsigned char)FluxOp::Space);   // Space
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
		selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}

	if (response == Ack::Wrprot) {
		selectDrive(false);
		return GWResponse::drWriteProtected;
	}

	if (response != Ack::Okay) {
		selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}

	// Write it
	unsigned long write = m_comPort.write(outputBuffer.data(), (unsigned int)outputBuffer.size());
	if (write != outputBuffer.size()) {
		selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}

	// Sync with GW
	unsigned char sync;
	unsigned long read = m_comPort.write(&sync, 1);
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
inline void unpackStreamQueue(std::queue<unsigned char>& queue, PLLData& pllData, RotationExtractor& extractor) {
	// Run until theres not enough data left to process
	while (queue.size()) {

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

			// Enough bit-cells?
			if (tickInNS > BITCELL_SIZE_IN_NS) {
				
				const int onePointFiveBitCells = BITCELL_SIZE_IN_NS + (BITCELL_SIZE_IN_NS / 2);
				int sequence = tickInNS >= onePointFiveBitCells ? (tickInNS - onePointFiveBitCells) / BITCELL_SIZE_IN_NS : 0;
				// This shouldnt ever happen
				if (sequence < 0) sequence = 0;
				

				// Rare condition
				if (sequence >= 3) {
					// Account for the ending 01
					tickInNS -= BITCELL_SIZE_IN_NS * 2;
					sequence -= 2;

					// Based on the rules we can't output a sequence this big and times must be accurate so we output as many 0000's as possible
					while (sequence > 0) {
						RotationExtractor::MFMSequenceInfo sample;
						sample.mfm = RotationExtractor::MFMSequence::mfm0000;
						unsigned int thisTicks = tickInNS;
						if (thisTicks > BITCELL_SIZE_IN_NS * 4) thisTicks = BITCELL_SIZE_IN_NS * 4;
						sample.timeNS = thisTicks;
						tickInNS -= sample.timeNS;
						extractor.submitSequence(sample, pllData.indexHit);
						pllData.indexHit = false;
						sequence -= 4;
					}

					// And follow it up with an 01
					RotationExtractor::MFMSequenceInfo sample;
					sample.mfm = RotationExtractor::MFMSequence::mfm01;
					sample.timeNS = BITCELL_SIZE_IN_NS * 2;
					extractor.submitSequence(sample, pllData.indexHit);
					pllData.indexHit = false;
				}
				else {
					RotationExtractor::MFMSequenceInfo sample;
					sample.mfm = (RotationExtractor::MFMSequence)sequence;
					sample.timeNS = tickInNS;

					extractor.submitSequence(sample, pllData.indexHit);
					pllData.indexHit = false;
				}

				pllData.ticks = 0;
			}
		}
	}

}

// Reads a complete rotation of the disk, and returns it using the callback function whcih can return FALSE to stop
// An instance of RotationExtractor is required.  This is purely to save on re-allocations.  It is internally reset each time
GWResponse GreaseWeazleInterface::readRotation(RotationExtractor& extractor, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns,
	std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation) {	
	GWReadFlux header;

	const unsigned int extraTime = OVERLAP_SEQUENCE_MATCHES * (OVERLAP_EXTRA_BUFFER) * 8000;
	if ((extractor.isInIndexMode()) || (!extractor.hasLearntRotationSpeed())) {
		header.ticks = 0;
		header.max_index = 1;
		header.max_index_linger = nSecToTicks((212 * 1000 * 1000) + extraTime, m_gwVersionInformation.sample_freq);
	}
	else {
		header.ticks = nSecToTicks((212 * 1000 * 1000) + extraTime, m_gwVersionInformation.sample_freq);
		header.max_index = 0;
		header.max_index_linger = 0;
	}
	
	m_shouldAbortReading = false;

	// Sample storage
	std::queue<unsigned char> queue;

	// Reset ready for extraction
	extractor.reset();

	// Remind it if the 'index' data we want to sync to
	extractor.setIndexSequence(startBitPatterns);

	selectDrive(true);

	// Write request
	Ack response = Ack::Okay;
	if (!sendCommand(Cmd::ReadFlux, (void*)&header, sizeof(header), response)) {
		selectDrive(false);
		return GWResponse::drReadResponseFailed;
	}
	int failCount = 0;

	// Buffer to read into
	unsigned char tempReadBuffer[64] = { 0 };
	bool zeroDetected = false;

	applyCommTimeouts(true);

	PLLData pllData;
	pllData.freq = m_gwVersionInformation.sample_freq;

	do {
		// More efficient to read several bytes in one go		
		unsigned long bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		unsigned long bytesRead = m_comPort.read(tempReadBuffer, m_shouldAbortReading ? 1 : bytesAvailable);
		if (bytesRead<1) {
			failCount++;
			if (failCount > 10) break;
		}
		else failCount = 0;

		// If theres this many we can process as this is the maximum we need to process
		if ((!m_shouldAbortReading) && (bytesRead)) {

			for (unsigned int a = 0; a < bytesRead; a++) {
				queue.push(tempReadBuffer[a]);
				zeroDetected |= tempReadBuffer[a] == 0;
			}

			unpackStreamQueue(queue, pllData, extractor);

			// Is it ready to extract?
			if (extractor.canExtract()) {
				unsigned int bits = 0;
				// Go!
				if (extractor.extractRotation(firstOutputBuffer, bits, maxOutputSize)) {
					if (!onRotation(&firstOutputBuffer, bits)) {
						// And if the callback says so we stop. - unsupported at present
						abortReadStreaming();
					}
					// Always save this back
					extractor.getIndexSequence(startBitPatterns);
				}
			}
		}
		else {
			for (unsigned int a = 0; a < bytesRead; a++) zeroDetected |= tempReadBuffer[a] == 0;
		}
	} while (!zeroDetected);

	applyCommTimeouts(false);

	// Check for errors
	response = Ack::Okay;
	sendCommand(Cmd::GetFluxStatus, nullptr, 0, response);

	selectDrive(false);

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

