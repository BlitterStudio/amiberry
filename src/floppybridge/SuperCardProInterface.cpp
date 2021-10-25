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

#include <sstream>
#include <vector>
#include <queue>
#include <cstring>
#include "SuperCardProInterface.h"
#include "RotationExtractor.h"
#include <thread>

#ifdef _WIN32
#include <winsock.h>
#pragma comment(lib,"Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

#define BITCELL_SIZE_IN_NS 2000L

#define BIT_READ_BITCELL_SIZE_8     (1 << 1)
#define BIT_READ_BITCELL_SIZE_16    (0 << 1)

#define BIT_READ_FROM_INDEX         (1 << 0)

using namespace SuperCardPro;


// Constructor for this class
SCPInterface::SCPInterface() {
	m_diskInDrive = false;
	m_isWriteProtected = true;
}

// Free me
SCPInterface::~SCPInterface() {
	closePort();

	if (m_dataBuffer) free(m_dataBuffer);
	m_dataBuffer = nullptr;
}

// Allocate the m_dataBuffer
bool SCPInterface::allocateBuffer(unsigned int size) {
	if (!m_dataBuffer) {
		m_dataBuffer = (uint16_t*)malloc(size);
		if (!m_dataBuffer) return false;
		m_dataBufferLength = size;
		return true;
	}

	if (m_dataBufferLength < size) {
		uint16_t* newBuffer = (uint16_t*)realloc(m_dataBuffer, size);
		if (!newBuffer) return false;
		m_dataBuffer = newBuffer;
		m_dataBufferLength = size;
		return true;
	}

	return true;
}


bool SCPInterface::sendCommand(const SCPCommand command, SCPResponse& response) {
	return sendCommand(command, NULL, 0, response);
}

bool SCPInterface::sendCommand(const SCPCommand command, const unsigned char parameter, SCPResponse& response) {
	return sendCommand(command, &parameter, 1, response);
}

bool SCPInterface::sendCommand(const SCPCommand command, const unsigned char* payload, const unsigned char payloadLength, SCPResponse& response, bool readResponse) {
	std::vector<unsigned char> packet;
	packet.push_back((unsigned char)command);
	packet.push_back(payloadLength);

	if ((payloadLength) && (payload)) {
		packet.resize(2 + payloadLength);
		unsigned char* target = ((unsigned char*)packet.data()) + 2;
#ifdef _WIN32
		memcpy_s(target, payloadLength, payload, payloadLength);
#else
		memcpy(target, payload, payloadLength);
#endif
	}
	unsigned char checksum = 0x4A;
	for (const unsigned char data : packet) checksum += data;
	packet.push_back(checksum);

	if (m_comPort.write(packet.data(), (unsigned int)packet.size()) != packet.size()) {
		response = SCPResponse::pr_writeError;
		return false;
	}

	if (readResponse) {
		SCPCommand cmdReply;
		if (m_comPort.read(&cmdReply, 1)!=1) return false;
		if (m_comPort.read(&response, 1)!=1) return false;
	}

	return true;
}


// Attempts to open the reader running on the COM port number provided.  Port MUST support 2M baud
SCPErr SCPInterface::openPort(std::string& comPort, bool useDriveA) {
	closePort();
	m_useDriveA = useDriveA;

	m_selectStatus = false;
	m_motorIsEnabled = false;

	std::vector<SerialIO::SerialPortInformation> ports;
	m_comPort.enumSerialPorts(ports);

	if (comPort.length()) {
		std::wstring wComport;
		quicka2w(comPort, wComport);
		switch (m_comPort.openPort(wComport)) {
		case SerialIO::Response::rInUse:return SCPErr::scpInUse;
		case SerialIO::Response::rNotFound:return SCPErr::scpNotFound;
		case SerialIO::Response::rOK: break;
		default: return SCPErr::scpUnknownError;
		}
	}
	else {
		// Find the port
		for (const SerialIO::SerialPortInformation& port : ports) {
			if ((port.portName.find(L"SCP-JIM") != std::wstring::npos) || (port.portName.find(L"Supercard Pro") != std::wstring::npos)) {
				switch (m_comPort.openPort(port.portName)) {
				case SerialIO::Response::rInUse:return SCPErr::scpInUse;
				case SerialIO::Response::rNotFound:return SCPErr::scpNotFound;
				case SerialIO::Response::rOK: break;
				default: return SCPErr::scpUnknownError;
				}
			}
			if (m_comPort.isPortOpen()) break;
		}
	}
	if (!m_comPort.isPortOpen()) SCPErr::scpNotFound;

	// Configure the port
	SerialIO::Configuration config;
	config.baudRate = 9600;
	config.ctsFlowControl = false;

	if (m_comPort.configurePort(config) != SerialIO::Response::rOK) return SCPErr::scpUnknownError;

	applyCommTimeouts(false);

	m_comPort.purgeBuffers();

	// Lookup what firmware this is running
	SCPResponse response;
	if (!sendCommand(SCPCommand::DoCMD_SCPInfo, response)) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	if (response != SCPResponse::pr_Ok) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	unsigned char data[2];
	if (m_comPort.read(data, 2) != 2) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	m_firmwareVersion.hardwareVersion = data[0] >> 4;
	m_firmwareVersion.hardwareRevision = data[0] & 0xFF;
	m_firmwareVersion.firmwareVersion = data[1] >> 4;
	m_firmwareVersion.firmwareRevision = data[1] & 0xFF;

	// Turn everything off
	sendCommand(SCPCommand::DoCMD_MTRAOFF, response);  
	if (response != SCPResponse::pr_Ok) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	sendCommand(SCPCommand::DoCMD_MTRBOFF, response);
	if (response != SCPResponse::pr_Ok) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	sendCommand(SCPCommand::DoCMD_DSELA, response);
	if (response != SCPResponse::pr_Ok) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	sendCommand(SCPCommand::DoCMD_DSELB, response);
	if (response != SCPResponse::pr_Ok) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}

	if (!findTrack0()) {
		m_comPort.closePort();
		return SCPErr::scpUnknownError;
	}
	m_currentTrack = 0;

	// Ok, success
	return SCPErr::scpOK;
}

// Trigger drive select
bool SCPInterface::selectDrive(bool select) {
	SCPResponse response;

	if (select == m_selectStatus) return true;

	if (select) {
		if (sendCommand(m_useDriveA ? SCPCommand::DoCMD_SELA : SCPCommand::DoCMD_SELB, response)) {
			m_selectStatus = select;
			return true;
		}
	}
	else {
		if (sendCommand(m_useDriveA ? SCPCommand::DoCMD_DSELA : SCPCommand::DoCMD_DSELB, response)) {
			m_selectStatus = select;
			return true;
		}
	}
	return false;
}

// Apply and change the timeouts on the com port
void SCPInterface::applyCommTimeouts(bool shortTimeouts) {
	m_comPort.setWriteTimeouts(2000, 200);

	if (shortTimeouts) {
		m_comPort.setReadTimeouts(15, 5);
	}
	else {
		m_comPort.setReadTimeouts(2000, 200);
	}		
}

// Closes the port down
void SCPInterface::closePort() {
	enableMotor(false);
	
	m_comPort.closePort();
}

// Turns on and off the reading interface
bool SCPInterface::enableMotor(const bool enable, const bool dontWait) {
	SCPResponse response;

	if (enable) {
		uint16_t payload[5];
		if (dontWait) {
			payload[0] = htons(1000);   // Drive select delay (microseconds)
			payload[1] = htons(5000);   // Step command delay (microseconds)
			payload[2] = htons(0);      // Motor on delay (milliseconds)
			payload[3] = htons(1);   // Track Seek Delay (Milliseconds)
			payload[4] = htons(0);   // Before turning off the motor automatically (milliseconds)
		}
		else {
			payload[0] = htons(1000);   // Drive select delay (microseconds)
			payload[1] = htons(5000);   // Step command delay (microseconds)
			payload[2] = htons(750);      // Motor on delay (milliseconds)
			payload[3] = htons(15);   // Track Seek Delay (Milliseconds)
			payload[4] = htons(20000);   // Before turning off the motor automatically (milliseconds)
		}

		if (!sendCommand(SCPCommand::DoCMD_SETPARAMS, (unsigned char*)payload, sizeof(payload), response)) return false;

		if (!sendCommand(m_useDriveA ? SCPCommand::DoCMD_MTRAON : SCPCommand::DoCMD_MTRBON, response)) return false;

		// make the light come on too
		selectDrive(true);

		if (!dontWait) std::this_thread::sleep_for(std::chrono::milliseconds(500));

		m_motorIsEnabled = enable;
		return true;
	}
	else {
		m_motorIsEnabled = false;
		return sendCommand(m_useDriveA ? SCPCommand::DoCMD_MTRAOFF : SCPCommand::DoCMD_MTRBOFF, response);
	}
}

// Do the 'no-click' seek
bool SCPInterface::performNoClickSeek() {
	if (m_currentTrack == 0) {
		SCPResponse response;

		selectDrive(true);
		bool ret = sendCommand(SCPCommand::DoCMD_STEPOUT, response);
		if (!m_motorIsEnabled) selectDrive(false);

		checkPins();

		return ret;
	}
	else {
		return false;
	}
}

// Select the track, this makes the motor seek to this position
bool SCPInterface::selectTrack(const unsigned char trackIndex, bool ignoreDiskInsertCheck) {
	if (m_currentTrack < 0) return false;
	if (trackIndex < 0) return false;
	if (trackIndex > 81) return false;

	selectDrive(true);
	SCPResponse response;
	bool ret = sendCommand(SCPCommand::DoCMD_STEPTO, trackIndex, response );
	if (!m_motorIsEnabled) selectDrive(false);

	if (ret && (response == SCPResponse::pr_Ok)) {
		m_currentTrack = trackIndex;

		// We have to check for a disk manually
		if (!ignoreDiskInsertCheck) {
			checkForDisk(true);
		}

		checkPins();

		return true;
	}
	else return false;
}

// Checks the pins from boards that support it
void SCPInterface::checkPins() {
	SCPResponse response;

	selectDrive(true);
	bool ret = sendCommand(SCPCommand::DoCMD_STATUS, response);	

	if (!ret) {
		if (!m_motorIsEnabled) selectDrive(false);
		return;
	}

	unsigned char bytes[2];
	if (m_comPort.read(bytes, 2) != 2) {
		if (!m_motorIsEnabled) selectDrive(false);
		return;
	}
	if (!m_motorIsEnabled) selectDrive(false);

	unsigned short status = (bytes[1] | (bytes[0] << 8));
	m_isWriteProtected = (status & (1 << 7)) == 0;
	m_diskInDrive = (status & (1 << 6)) !=0;
	m_isAtTrack0 = (status & (1 << 5)) == 0;
}

// Search for track 0
bool SCPInterface::findTrack0() {
	SCPResponse response;

	selectDrive(true);
	bool ret = sendCommand(SCPCommand::DoCMD_SEEK0, response);
	if (!m_motorIsEnabled) selectDrive(false);
	
	return ret;
}

// Choose which surface of the disk to read from
bool SCPInterface::selectSurface(const DiskSurface side) {
	SCPResponse response;

	selectDrive(true);
	bool ret = sendCommand(SCPCommand::DoCMD_SIDE, (side == DiskSurface::dsUpper) ? 1 : 0, response);
	if (!m_motorIsEnabled) selectDrive(false);

	return ret;
}

// Read RAW data from the current track and surface 
SCPErr SCPInterface::checkForDisk(bool force) {
	if (force) checkPins();

	return m_diskInDrive ? SCPErr::scpOK : SCPErr::scpNoDiskInDrive;
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

// Write data to the disk
SCPErr SCPInterface::writeCurrentTrackPrecomp(const unsigned char* mfmData, const unsigned short numBytes, const bool writeFromIndexPulse, bool usePrecomp) {
	std::vector<uint16_t> outputBuffer;

	// Original data was written from MSB downto LSB
	int pos = 0;
	int bit = 7;
	unsigned char sequence = 0xAA;  // start at 10101010
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

		if (m_isHDMode) timeInNS /= 2;

		// convert for SCP
		timeInNS /= 25;
		while (timeInNS > 65535) {
			outputBuffer.push_back(0);
			timeInNS -= 65536;
		}
		outputBuffer.push_back(htons(timeInNS));
	}	

	struct writeheader {
		uint32_t ramOffset;
		uint32_t transferLength;
	} ;

	writeheader header;
	header.ramOffset = 0;
	header.transferLength = htonl((u_long)outputBuffer.size() * 2UL);

	// Write the above to the RAM on the SCP
	SCPResponse response;
	if (!sendCommand(SCPCommand::DoCMD_LOADRAM_USB,(unsigned char*) &header, sizeof(header), response, false)) return SCPErr::scpUnknownError;

	// Now send the data
	if (m_comPort.write(outputBuffer.data(), (unsigned int)(outputBuffer.size() * 2)) != outputBuffer.size() * 2) 
		return SCPErr::scpUnknownError;

	// Now read the response
	SCPCommand cmdReply;
	if (!m_comPort.read(&cmdReply, 1)) return SCPErr::scpUnknownError;
	if (cmdReply != SCPCommand::DoCMD_LOADRAM_USB) return SCPErr::scpUnknownError;
	if (!m_comPort.read(&response, 1)) return SCPErr::scpUnknownError;
	if (response != SCPResponse::pr_Ok) return SCPErr::scpUnknownError;

	// Now ask the drive to write it
	selectDrive(true);

	unsigned char buffer[5];
	header.transferLength = htonl((u_long)outputBuffer.size());
	memcpy(buffer, &header.transferLength, 4);
	buffer[4] = BIT_READ_BITCELL_SIZE_16 | (writeFromIndexPulse ? BIT_READ_FROM_INDEX : 0);

	if (!sendCommand(SCPCommand::DoCMD_WRITEFLUX, buffer, 5, response, true)) return SCPErr::scpUnknownError;
	if (!m_motorIsEnabled) selectDrive(false);

	if (response == SCPResponse::pr_WPEnabled) {
		m_isWriteProtected = true;
		return SCPErr::scpWriteProtected;
	}
	m_isWriteProtected = false;

	return SCPErr::scpOK;
}

struct ReadResponseRotation {
	uint32_t indexTime, bitcells;
};

// Switch the density mode
bool SCPInterface::selectDiskDensity(bool hdMode) {
	m_isHDMode = hdMode;
	return true;
}

// Read ram from the SCP
bool SCPInterface::readSCPRam(const unsigned int offset, const unsigned int length) {
	if (!allocateBuffer(length)) return false;

	// Now ask for the data
	uint32_t params[2] = { htonl(offset), htonl(length) };
	SCPResponse response;

	bool ret = sendCommand(SCPCommand::DoCMD_SENDRAM_USB, (unsigned char*)params, sizeof(params), response, false);
	if (!ret)  return false;

	if (m_comPort.read(m_dataBuffer, length) != length) return false;

	// Read the command
	SCPCommand cmd;
	if (m_comPort.read((unsigned char*)&cmd, 1) != 1) return false;
	if (cmd != SCPCommand::DoCMD_SENDRAM_USB) return false;

	// Read the status
	if (m_comPort.read(&response, 1) != 1) return false;
	return (response == SCPResponse::pr_Ok);
}

// Test if its an HD disk
bool SCPInterface::checkDiskCapacity(bool& isHD) {
	SCPResponse response;

	bool alreadySpun = m_motorIsEnabled;
	if (!alreadySpun)
		if (!enableMotor(true, false)) return false;

	// Issue a read-request
	selectDrive(true);
	unsigned char payload[] = { 1, BIT_READ_BITCELL_SIZE_16 };

	bool ret = sendCommand(SCPCommand::DoCMD_READFLUX, payload, 2, response);
	
	if (!m_motorIsEnabled) selectDrive(false);

	if (!ret) {
		if ((response == SCPResponse::pr_NotReady) || (response == SCPResponse::pr_NoDisk)) m_diskInDrive = false;
		// Turn the motor back off if we started it
		if (!alreadySpun) enableMotor(false, false);
		return false;
	}

	// Fetch what happened
	ret = sendCommand(SCPCommand::DoCMD_GETFLUXINFO, response);
	if (!ret) {
		if ((response == SCPResponse::pr_NotReady) || (response == SCPResponse::pr_NoDisk)) m_diskInDrive = false;
		// Turn the motor back off if we started it
		if (!alreadySpun) enableMotor(false, false);
		return false;
	}

	// The function receives data for the potential full 5
	ReadResponseRotation rotations[5];
	if (m_comPort.read(rotations, sizeof(rotations)) != sizeof(rotations)) {
		// Turn the motor back off if we started it
		if (!alreadySpun) enableMotor(false, false);
		return false;
	}
	
	rotations[0].indexTime = htonl(rotations[0].indexTime);
	rotations[0].bitcells = htonl(rotations[0].bitcells);

	// Stop! probably no disk in the drive
	if (rotations[0].bitcells < 10) {
		if (!alreadySpun) enableMotor(false, false);
		m_diskInDrive = false;
		return false;
	}	

	unsigned int totalBitCells = rotations[0].bitcells;
	// We dont need that much data
	if (totalBitCells > 20000) totalBitCells = 20000;

	if (!readSCPRam(0, totalBitCells * 2)) {
		if (!alreadySpun) enableMotor(false, false);
		return false;
	}
	if (!alreadySpun) enableMotor(false, false);


	unsigned int nsCounting = 0;
	unsigned int hdBits = 0;
	unsigned int ddBits = 0;

	// Look at the data
	for (unsigned int bit = 0; bit < totalBitCells; bit++) {

		// Work out the actual time this tick took in nanoSeconds.
		unsigned int tickInNS = ((unsigned int)htons(m_dataBuffer[bit])) * 25;

		if (tickInNS == 0) {
			nsCounting += 0x10000;
			continue;
		}

		// Enough bit-cells?
		if (tickInNS + nsCounting > BITCELL_SIZE_IN_NS) {
			tickInNS += nsCounting;
			nsCounting = 0;
			if (tickInNS < 3000) hdBits++;
			if ((tickInNS > 4500) && (tickInNS < 8000)) ddBits++;
		}
	}

	// Probably no disk
	if (hdBits + ddBits < 100) {
		m_diskInDrive = false;
		return false;
	}
		
	isHD = (hdBits > ddBits);

	return true;
}


// Reads a complete rotation of the disk, and returns it using the callback function whcih can return FALSE to stop
// An instance of RotationExtractor is required.  This is purely to save on re-allocations.  It is internally reset each time
SCPErr SCPInterface::readRotation(RotationExtractor& extractor, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns,
	std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation) {
	SCPResponse response;

	m_shouldAbortReading = false;

	// Issue a read-request
	selectDrive(true);
	unsigned int numRotations = 4;// startBitPatterns.valid ? 2 : 1;  // TODO: remove BIT_READ_FROM_INDEX if valid
	unsigned char payload[] = { (unsigned char)numRotations, BIT_READ_FROM_INDEX | BIT_READ_BITCELL_SIZE_16};

	bool ret = sendCommand(SCPCommand::DoCMD_READFLUX, payload, 2, response);
	if (!m_motorIsEnabled) selectDrive(false);
	if (!ret) {
		if ((response == SCPResponse::pr_NotReady) || (response == SCPResponse::pr_NoDisk)) {
			m_diskInDrive = false;
			return SCPErr::scpNoDiskInDrive;
		}
		return SCPErr::scpUnknownError;
	}
	
	// Fetch what happened
	ret = sendCommand(SCPCommand::DoCMD_GETFLUXINFO, response);
	if (!ret) {
		if ((response == SCPResponse::pr_NotReady) || (response == SCPResponse::pr_NoDisk)) {
			m_diskInDrive = false;
			return SCPErr::scpNoDiskInDrive;
		}
		return SCPErr::scpUnknownError;
	}

	// The function receives data for the potential full 5
	ReadResponseRotation rotations[5];
	if (m_comPort.read(rotations, sizeof(rotations))!=sizeof(rotations)) return SCPErr::scpUnknownError;
	for (int a = 0; a < 5; a++) {
		rotations[a].indexTime = htonl(rotations[a].indexTime);
		rotations[a].bitcells = htonl(rotations[a].bitcells);
	}

	// Stop! probably no disk in the drive
	if (rotations[0].bitcells < 10) {
		m_diskInDrive = false;
		return SCPErr::scpNoDiskInDrive;
	}

	// Reset ready for extraction
	extractor.reset(false);

	// Remind it if the 'index' data we want to sync to
	extractor.setIndexSequence(startBitPatterns);
	
	unsigned int offset = 0;
	unsigned int nsCounting = 0;
	unsigned int numRot = numRotations;
	if (numRot < 2) numRot = 2;

	// Now read and process the data
	for (unsigned int currentRotation = 0; currentRotation < numRot; currentRotation++) {

		// Just allow the first rotation to be fed back in
		unsigned int actualRotation = currentRotation % numRotations;
		if (currentRotation < numRotations) {
			if (!readSCPRam(offset, rotations[currentRotation].bitcells * 2)) return SCPErr::scpUnknownError;
			offset += rotations[currentRotation].bitcells * 2;
		}

		bool isIndex = true;

		for (unsigned int bit = 0; bit < rotations[actualRotation].bitcells; bit++) {

			// Work out the actual time this tick took in nanoSeconds.
			unsigned int tickInNS = ((unsigned int)htons(m_dataBuffer[bit])) * 25;
			if (m_isHDMode) tickInNS *= 2;

			if (tickInNS == 0) {
				nsCounting += 0x10000;
				continue;
			}

			// Enough bit-cells?
			if (tickInNS + nsCounting > BITCELL_SIZE_IN_NS) {
				tickInNS += nsCounting;

				nsCounting = 0;

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
						extractor.submitSequence(sample, isIndex, false);
						isIndex = false;
						sequence -= 4;
					}

					// And follow it up with an 01
					RotationExtractor::MFMSequenceInfo sample;
					sample.mfm = RotationExtractor::MFMSequence::mfm01;
					sample.timeNS = BITCELL_SIZE_IN_NS * 2;
					extractor.submitSequence(sample, isIndex, false);
					isIndex = false;
				}
				else {
					RotationExtractor::MFMSequenceInfo sample;
					sample.mfm = (RotationExtractor::MFMSequence)sequence;
					sample.timeNS = tickInNS;

					extractor.submitSequence(sample, isIndex, false);
					isIndex = false;
				}
			}

			if (extractor.canExtract()) {
				unsigned int bits = 0;
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
		
		if (m_shouldAbortReading) break;
	}
	
	return SCPErr::scpOK;
}

// Attempt to abort reading
void SCPInterface::abortReadStreaming() {
	m_shouldAbortReading = true;
}

