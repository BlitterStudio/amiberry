/* Supercard Pro C++ Interface for *UAE
*
* By Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* Based on the documentation and hardware by Jim Drew at
* https://www.cbmstuff.com/
* 
* This requires firmware V1.3
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

#include <sstream>
#include <vector>
#include <queue>
#include <cstring>
#include <thread>
#include <deque>
#include "SuperCardProInterface.h"
#include "pll.h"
#include "RotationExtractor.h"
#ifdef _WIN32
#include <winsock.h>
#pragma comment(lib,"Ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

#define MOTOR_AUTOOFF_DELAY 10000

#define BITCELL_SIZE_IN_NS 2000L

#define BIT_READ_FROM_INDEX         (1 << 0)


#define STREAMFLAGS_RAWFLUX				(0 << 4)

#define STREAMFLAGS_RESOLUTION_25NS     (0 << 2)
#define STREAMFLAGS_RESOLUTION_50NS     (1 << 2)
#define STREAMFLAGS_RESOLUTION_100NS    (2 << 2)
#define STREAMFLAGS_RESOLUTION_200NS    (3 << 2)

#define FLAGS_BITSIZE_8BIT        (1 << 1)
#define FLAGS_BITSIZE_16BIT       (0 << 1)

#define STREAMFLAGS_IMMEDIATEREAD       (0 << 0)
#define STREAMFLAGS_STREAMFROMINDEX     (1 << 0)

using namespace SuperCardPro;


// Constructor for this class
SCPInterface::SCPInterface() {
	m_diskInDrive = false;
	m_isWriteProtected = true;
	m_abortSignalled = false;
	m_isStreaming = false;
	m_abortStreaming = true;
}

// Free me
SCPInterface::~SCPInterface() {
	abortReadStreaming();
	closePort();
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
SCPErr SCPInterface::openPort(bool useDriveA) {
	closePort();
	m_useDriveA = useDriveA;

	m_selectStatus = false;
	m_motorIsEnabled = false;

	std::vector<SerialIO::SerialPortInformation> ports;
	SerialIO::enumSerialPorts(ports);

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

	// Check we're on v1.3 of the firmware
	if ((m_firmwareVersion.firmwareVersion <= 1) && (m_firmwareVersion.firmwareRevision < 3)) {
		m_comPort.closePort();
		return SCPErr::scpFirmwareTooOld;
	}

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

// Returns the motor idle (auto switch off) time
unsigned int SCPInterface::getMotorIdleTimeoutTime() {
	return MOTOR_AUTOOFF_DELAY;
}


// Turns on and off the reading interface
bool SCPInterface::enableMotor(const bool enable, const bool dontWait) {
	SCPResponse response;
	
	if (enable) {
		uint16_t payload[5];
		if (dontWait) {
			payload[0] = htons(1000);   // Drive select delay (microseconds)
			payload[1] = htons(5000);   // Step command delay (microseconds)
			payload[2] = htons(1);      // Motor on delay (milliseconds)
			payload[3] = htons(1);      // Track Seek Delay (Milliseconds)
			payload[4] = htons(MOTOR_AUTOOFF_DELAY);   // Before turning off the motor automatically (milliseconds)
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

	uint16_t status = (bytes[1] | (bytes[0] << 8));
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

	// Re-encode the data into our format and apply precomp.  The +1 is to ensure there's some padding around the edge 
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
		int timeInNS = extraTimeFromPrevious + (count * 2000);     // 2=4000, 3=6000, 4=8000, (5=10000 although is isn't strictly allowed)

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
	buffer[4] = FLAGS_BITSIZE_16BIT | (writeFromIndexPulse ? BIT_READ_FROM_INDEX : 0);

	if (!sendCommand(SCPCommand::DoCMD_WRITEFLUX, buffer, 5, response, true)) return SCPErr::scpUnknownError;
	if (!m_motorIsEnabled) selectDrive(false);

	if (response == SCPResponse::pr_WPEnabled) {
		m_isWriteProtected = true;
		return SCPErr::scpWriteProtected;
	}
	m_isWriteProtected = false;

	return SCPErr::scpOK;
}

// Switch the density mode
bool SCPInterface::selectDiskDensity(bool hdMode) {
	m_isHDMode = hdMode;
	return true;
}


#ifndef _WIN32
#define USE_THREADDED_READER_SCP
#endif


// Test if its an HD disk
bool SCPInterface::checkDiskCapacity(bool& isHD) {
	SCPResponse response;

	bool alreadySpun = m_motorIsEnabled;
	if (!alreadySpun)
		if (!enableMotor(true, false)) return false;

	// Issue a read-request
	selectDrive(true);

	// Request real-time flux stream, 8-bit resolution, and 50ns timings.
	const unsigned char payload[1] = { STREAMFLAGS_RESOLUTION_50NS | FLAGS_BITSIZE_16BIT | STREAMFLAGS_IMMEDIATEREAD | STREAMFLAGS_RAWFLUX };
	bool ret = sendCommand(SCPCommand::DoCMD_STARTSTREAM, payload, 1, response);
	if (!ret) {
		if ((response == SCPResponse::pr_NotReady) || (response == SCPResponse::pr_NoDisk)) {
			m_diskInDrive = false;
			return false;
		}
		return false;
	}



#ifdef USE_THREADDED_READER_SCP
	std::mutex safetyLock;
	// I was using a deque, but its much faster to just keep switching between two vectors
	std::vector<unsigned char> readBuffer;
	readBuffer.reserve(4096);
	std::vector<unsigned char> tempReadBuffer;
	tempReadBuffer.reserve(4096);
#ifdef _WIN32
	m_comPort.setReadTimeouts(100, 0);  // match linux
#endif
	std::thread* backgroundReader = new std::thread([this, &readBuffer, &safetyLock]() {
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif
		unsigned char buffer[1024];  // the LINUX serial buffer is only 512 bytes anyway
		while (m_isStreaming) {
			uint32_t waiting = m_comPort.justRead(buffer, 1024);
			if (waiting>0) {
				safetyLock.lock();				
				readBuffer.insert(readBuffer.end(), buffer, buffer+waiting);
				safetyLock.unlock();
			} else
				std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
	});


#else
	applyCommTimeouts(true);
	unsigned char tempReadBuffer[2048] = { 0 };
#endif

	m_isStreaming = true;
	m_abortStreaming = false;
	m_abortSignalled = false;

	// Number of times we failed to read anything
	int readFail = 0;

	// Sliding window for abort
	unsigned char slidingWindow[4] = { 0,0,0,0 };
	bool timeout = false;
	bool dataState = false;
	bool isFirstByte = true;
	unsigned char mfmSequences = 0;
	int hitIndex = 0;
	uint32_t tickInNS = 0;

	unsigned int nsCounting = 0;
	unsigned int hdBits = 0;
	unsigned int ddBits = 0;

	for (;;) {

		// More efficient to read several bytes in one go		
#ifdef USE_THREADDED_READER_SCP
		tempReadBuffer.resize(0); // should be just as fast as clear, but just in case
		safetyLock.lock();
		std::swap(tempReadBuffer, readBuffer);
		safetyLock.unlock();

		unsigned int bytesRead = tempReadBuffer.size();

		if (bytesRead < 1) std::this_thread::sleep_for(std::chrono::milliseconds(20));
		for (const unsigned char byteRead : tempReadBuffer) {
#else
		unsigned int bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		unsigned int bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);
		for (size_t a = 0; a < bytesRead; a++) {
			const unsigned char byteRead = tempReadBuffer[a];
#endif
			if (m_abortSignalled) {
				for (int s = 0; s < 3; s++) slidingWindow[s] = slidingWindow[s + 1];
				slidingWindow[3] = byteRead;

				// Watch the sliding window for the pattern we need
				if ((slidingWindow[0] == 0xDE) && (slidingWindow[1] == 0xAD) && (slidingWindow[2] == (unsigned char)SCPCommand::DoCMD_STOPSTREAM)) {
					m_isStreaming = false;
					m_comPort.purgeBuffers();

					applyCommTimeouts(false);
					if (!alreadySpun) enableMotor(false, false);

					isHD = (hdBits > ddBits);
#ifdef USE_THREADDED_READER_SCP
					if (backgroundReader) {
						if (backgroundReader->joinable()) backgroundReader->join();
						delete backgroundReader;
					}
#endif
					if (timeout) return false;
					if (slidingWindow[3] == (unsigned char)SCPResponse::pr_Overrun) return false;
					return true;
				}
			}
			else {

				switch (byteRead) {
				case 0xFF:
					hitIndex = 1; break;
				case 0x00:
					if (hitIndex == 1) hitIndex = 2; else tickInNS += m_isHDMode ? (256 * 100) : (256 * 50);
				default:
					tickInNS += byteRead * 50;
					// Enough bit-cells?
					if (tickInNS > BITCELL_SIZE_IN_NS) {
						if (tickInNS < 3000) hdBits++;
						if ((tickInNS > 4500) && (tickInNS < 8000)) ddBits++;

						if ((hdBits + ddBits > 10000) && (!m_abortStreaming)) 
							abortReadStreaming();
						tickInNS = 0;
						hitIndex = 0;
					}
				}
			}
		}
		if (bytesRead < 1) {
			readFail++;
			if (readFail > 20) {
				if (!m_abortStreaming) {
					abortReadStreaming();
					readFail = 0;
					m_diskInDrive = false;
				}
				else {
					isHD = (hdBits > ddBits);
					applyCommTimeouts(false);
					if (!alreadySpun) enableMotor(false, false);
					m_isStreaming = false;
#ifdef USE_THREADDED_READER_SCP
					if (backgroundReader) {
						if (backgroundReader->joinable()) backgroundReader->join();
						delete backgroundReader;
					}
#endif
					return false;
				}
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		else {
			readFail = 0;
		}
	}
}

// Reads a complete rotation of the disk, and returns it using the callback function which can return FALSE to stop
// An instance of PLL is required which contains a rotation extractor.  This is purely to save on re-allocations.  It is internally reset each time
SCPErr SCPInterface::readRotation(PLL::BridgePLL& pll, const unsigned int maxOutputSize, RotationExtractor::MFMSample* firstOutputBuffer, RotationExtractor::IndexSequenceMarker& startBitPatterns,
	std::function<bool(RotationExtractor::MFMSample** mfmData, const unsigned int dataLengthInBits)> onRotation) {
	SCPResponse response;

	pll.prepareExtractor(m_isHDMode, startBitPatterns);

	// Issue a read-request
	selectDrive(true);

	// Request real-time flux stream, 8-bit resolution, and 50ns timings.
	const unsigned char payload[1] = { STREAMFLAGS_RESOLUTION_50NS | FLAGS_BITSIZE_8BIT | STREAMFLAGS_IMMEDIATEREAD | STREAMFLAGS_RAWFLUX };
	bool ret = sendCommand(SCPCommand::DoCMD_STARTSTREAM, payload, 1, response);
	if (!ret) {
		if (!m_motorIsEnabled) selectDrive(false);
		if ((response == SCPResponse::pr_NotReady) || (response == SCPResponse::pr_NoDisk)) {
			m_diskInDrive = false;
			return SCPErr::scpNoDiskInDrive;
		}
		return SCPErr::scpUnknownError;
	}
	

#ifdef USE_THREADDED_READER_SCP
	std::mutex safetyLock;
	// I was using a deque, but its much faster to just keep switching between two vectors
	std::vector<unsigned char> readBuffer;
	readBuffer.reserve(4096);
	std::vector<unsigned char> tempReadBuffer;
	tempReadBuffer.reserve(4096);
#ifdef _WIN32
	m_comPort.setReadTimeouts(100, 0);  // match linux
#endif
	std::thread* backgroundReader = new std::thread([this, &readBuffer, &safetyLock]() {
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else
		struct sched_param sch;
		int policy;
		if (pthread_getschedparam(pthread_self(), &policy, &sch) == 0) {
			policy = SCHED_FIFO;
			sch.sched_priority = sched_get_priority_max(policy); // boost priority
			pthread_setschedparam(pthread_self(), policy, &sch);
		}
#endif
		unsigned char buffer[1024];  // the LINUX serial buffer is only 512 bytes anyway
		while (m_isStreaming) {
			uint32_t waiting = m_comPort.justRead(buffer, 1024);
			if (waiting > 0) {
				safetyLock.lock();
				readBuffer.insert(readBuffer.end(), buffer, buffer + waiting);
				safetyLock.unlock();
}
			else
				std::this_thread::sleep_for(std::chrono::microseconds(200));
		}
	});


#else
	applyCommTimeouts(true);
	unsigned char tempReadBuffer[2048] = { 0 };
#endif

	m_isStreaming = true;
	m_abortStreaming = false;
	m_abortSignalled = false;
	
	// Number of times we failed to read anything
	int readFail = 0;

	pll.prepareExtractor(m_isHDMode, startBitPatterns);

	// Sliding window for abort
	unsigned char slidingWindow[4] = { 0,0,0,0 };
	bool timeout = false;
	bool dataState = false;
	bool isFirstByte = true;
	unsigned char mfmSequences = 0;
	int hitIndex = 0;
	uint32_t tickInNS = 0;

	for (;;) {

		// More efficient to read several bytes in one go	
#ifdef USE_THREADDED_READER_SCP
		tempReadBuffer.resize(0); // should be just as fast as clear, but just in case
		safetyLock.lock();
		std::swap(tempReadBuffer, readBuffer);
		safetyLock.unlock();

		unsigned int bytesRead = tempReadBuffer.size();

		if (bytesRead < 1) std::this_thread::sleep_for(std::chrono::milliseconds(20));
		for (const unsigned char byteRead : tempReadBuffer) {
#else
		unsigned int bytesAvailable = m_comPort.getBytesWaiting();
		if (bytesAvailable < 1) bytesAvailable = 1;
		if (bytesAvailable > sizeof(tempReadBuffer)) bytesAvailable = sizeof(tempReadBuffer);
		unsigned int bytesRead = m_comPort.read(tempReadBuffer, m_abortSignalled ? 1 : bytesAvailable);
		for (size_t a = 0; a < bytesRead; a++) {
			const unsigned char byteRead = tempReadBuffer[a];
#endif
			if (m_abortSignalled) {
				for (int s = 0; s < 3; s++) slidingWindow[s] = slidingWindow[s + 1];
				slidingWindow[3] = byteRead;

				// Watch the sliding window for the pattern we need
				if ((slidingWindow[0] == 0xDE) && (slidingWindow[1] == 0xAD) && (slidingWindow[2] == (unsigned char)SCPCommand::DoCMD_STOPSTREAM)) {
					m_isStreaming = false;
#ifdef USE_THREADDED_READER_SCP
					if (backgroundReader) {
						if (backgroundReader->joinable()) backgroundReader->join();
						delete backgroundReader;
					}
#endif
					m_comPort.purgeBuffers();
					applyCommTimeouts(false);
					if (!m_diskInDrive) return SCPErr::scpNoDiskInDrive;
					if (timeout) return SCPErr::scpUnknownError;
					if (slidingWindow[3] == (unsigned char)SCPResponse::pr_Overrun) return SCPErr::scpOverrun;
					return SCPErr::scpOK;
				}
			}
			else {
				switch (byteRead) {
				case 0xFF:
					hitIndex = 1; 
					break;
				case 0x00: 
					if (hitIndex == 1) hitIndex = 2; else {
						tickInNS += m_isHDMode ? (256 * 100) : (256 * 50);
						hitIndex = 0;
					}
					break;
				default:
					pll.submitFlux(tickInNS + (m_isHDMode ? (byteRead * 100UL) : (byteRead * 50UL)), hitIndex == 2);
					tickInNS = 0;
					hitIndex = 0;
					break;
				}

				// Is it ready to extract?
				if (pll.canExtract()) {
					unsigned int bits = 0;
					// Go!
					if (pll.extractRotation(firstOutputBuffer, bits, maxOutputSize, true)) {
						m_diskInDrive = true;

						if (!onRotation(&firstOutputBuffer, bits)) {
							// And if the callback says so we stop.
							abortReadStreaming();
						}
						// Always save this back
						pll.getIndexSequence(startBitPatterns);
					}
				}
				else {
					if (pll.totalTimeReceived() > (m_isHDMode ? 1200000000U : 600000000U)) {
						// No data, stop
						abortReadStreaming();
						timeout = true;
					}
				}
			}
		}
		if (bytesRead < 1) {
			readFail++;
			if (readFail > 30) {
				if (!m_abortStreaming) {
					abortReadStreaming();
					readFail = 0;
					m_diskInDrive = false;
				}
				else {
					m_isStreaming = false;
#ifdef USE_THREADDED_READER_SCP
					if (backgroundReader) {
						if (backgroundReader->joinable()) backgroundReader->join();
						delete backgroundReader;
					}
#endif
					applyCommTimeouts(false);
					return SCPErr::scpUnknownError;
				}
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		else {
			readFail = 0;
		}
	}
}


// Stops the read streaming immediately and any data in the buffer will be discarded.
bool SCPInterface::abortReadStreaming() {
	// Prevent two things doing this at once, and thus ending with two bytes being written
	std::lock_guard<std::mutex> lock(m_protectAbort);
	if (!m_isStreaming) return true;

	if (!m_abortStreaming) {
		SCPResponse response;
		m_abortSignalled = true;
		if (!sendCommand(SCPCommand::DoCMD_STOPSTREAM, NULL, 0, response, false))  return false;
	}
	m_abortStreaming = true;
	return true;
}

