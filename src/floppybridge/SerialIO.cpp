/* ArduinoFloppyReader (and writer)
*
* Copyright (C) 2017-2021 Robert Smith (@RobSmithDev)
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

////////////////////////////////////////////////////////////////////////////////////////
// Class to manage the communication between the computer and a serial port           //
////////////////////////////////////////////////////////////////////////////////////////
//
//
#include "SerialIO.h"

#ifdef _WIN32
#include <SetupAPI.h>
#include <Devpropdef.h>
#include "RotationExtractor.h"
#pragma comment(lib, "setupapi.lib")
static const DEVPROPKEY DEVPKEY_Device_BusReportedDeviceDesc2 = { {0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2}, 4 };
static const DEVPROPKEY DEVPKEY_Device_InstanceId2 = { {0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57}, 256 };   
#ifndef GUID_DEVINTERFACE_COMPORT
DEFINE_GUID(GUID_DEVINTERFACE_COMPORT,0x86e0d1e0, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
#endif

#else

#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>
#include <linux/serial.h>

#endif

#include <string>
#include <codecvt>
#include <locale>

using convert_t = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_t, wchar_t> strconverter;

void quickw2a(const std::wstring& wstr, std::string& str) {
	str = strconverter.to_bytes(wstr);
}
void quicka2w(const std::string& str, std::wstring& wstr) {
	wstr = strconverter.from_bytes(str);
}

// Constructor etc
SerialIO::SerialIO() {
}

SerialIO::~SerialIO() {
	closePort();
}

// Returns TRUE if the port is open
bool SerialIO::isPortOpen() const {
#ifdef _WIN32
	return m_portHandle != INVALID_HANDLE_VALUE;
#else
	return m_portHandle != -1;
#endif

	return false;
}

// Purge any data left in the buffer
void SerialIO::purgeBuffers() {
	if (!isPortOpen()) return;

#ifdef _WIN32
	PurgeComm(m_portHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
	tcflush(m_portHandle, TCIOFLUSH);
#endif
}

// Sets the status of the DTR line
void SerialIO::setDTR(bool enableDTR) {
	if (!isPortOpen()) return;

#ifdef _WIN32
	EscapeCommFunction(m_portHandle, enableDTR ? SETDTR : CLRDTR);
#else
	int pinToControl = TIOCM_DTR;
	ioctl(m_portHandle, enableDTR ? TIOCMBIS : TIOCMBIC, &pinToControl);
#endif
}

// Returns the status of the CTS pin
bool SerialIO::getCTSStatus() {
	if (!isPortOpen()) return false;

#ifdef _WIN32
	DWORD mask = 0;
	if (!GetCommModemStatus(m_portHandle, &mask)) return false;
	return (mask & MS_CTS_ON) != 0;
#else
	int status;
	ioctl(m_portHandle, TIOCMGET, &status);
	return (status & TIOCM_CTS) != 0;
#endif

	return false;
}

// Returns a list of serial ports discovered on the system
void SerialIO::enumSerialPorts(std::vector<SerialPortInformation>& serialPorts) {
	serialPorts.clear();

#ifdef _WIN32
	// Query for hardware
	HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hDevInfoSet == INVALID_HANDLE_VALUE) return;

	// Scan for items
	DWORD devIndex = 0;
	SP_DEVINFO_DATA devInfo;
	devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	// Enum devices
	while (SetupDiEnumDeviceInfo(hDevInfoSet, devIndex, &devInfo)) {
		HKEY key = SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
		if (key != INVALID_HANDLE_VALUE) {

			WCHAR name[128];
			DWORD nameLength = 128;

			// Get the COM Port Name
			if (RegQueryValueExW(key, L"PortName", NULL, NULL, (LPBYTE)name, &nameLength) == ERROR_SUCCESS) {
				SerialPortInformation port;
				port.portName = name;

				// Check it starts with COM
				if ((port.portName.length() >= 4) && (port.portName.substr(0, 3) == L"COM")) {
					// Get the hardware ID
					nameLength = 128;
					DWORD dwType;
					if (SetupDiGetDeviceRegistryPropertyW(hDevInfoSet, &devInfo, SPDRP_HARDWAREID, &dwType, (LPBYTE)name, 128, &nameLength)) {
						std::wstring deviceString = name;
						int a = (int)deviceString.find(L"VID_");
						if (a != std::wstring::npos) port.vid = wcstol(deviceString.substr(a + 4).c_str(), NULL, 16);
						a = (int)deviceString.find(L"PID_");
						if (a != std::wstring::npos) port.pid = wcstol(deviceString.substr(a + 4).c_str(), NULL, 16);
					}

					// Description
					DWORD type;
					if (SetupDiGetDeviceProperty(hDevInfoSet, &devInfo, &DEVPKEY_Device_BusReportedDeviceDesc2, &type, (PBYTE)name, 128, 0, 0)) port.productName = name;

					// Instance 
					if (SetupDiGetDeviceProperty(hDevInfoSet, &devInfo, &DEVPKEY_Device_InstanceId2, &type, (PBYTE)name, 128, 0, 0)) port.instanceID = name;

					serialPorts.push_back(port);
				}
			}
			RegCloseKey(key);
		}

		devIndex++;
	}

	SetupDiDestroyDeviceInfoList(hDevInfoSet);

#else

	DIR* dir = opendir("/sys/class/tty");
	if (!dir) return;

	struct dirent* entry;
	struct stat statbuf;

	while ((entry = readdir(dir))) {
		std::string deviceRoot = "/sys/class/tty/" + std::string(entry->d_name);
		if (lstat(deviceRoot.c_str(), &statbuf) == -1) continue;
		if (!S_ISLNK(statbuf.st_mode)) deviceRoot += "/device";

		char target[PATH_MAX];
		int len = readlink(deviceRoot.c_str(), target, sizeof(target));
		if ((len <= 0) || (len >= PATH_MAX-1)) continue;
		target[len] = '\0';

		if (strstr(target, "virtual")) continue;
		if (strstr(target, "bluetooth")) continue;

		if (strstr(target, "usb")) {
			std::string name = "/dev/" + std::string(entry->d_name);

			SerialPortInformation prt;
			quicka2w(name, prt.portName);
			
			std::string dirName = "/sys/class/tty/" + std::string(entry->d_name) + "/device/";
			std::string subDir = "";

			for (int i=0; i<5; i++) {
				subDir += "../";
				std::string vidPath = dirName + "/"+subDir+"/idVendor";

				int file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				FILE* fle = fdopen(file, "r");
				int count = fscanf(fle, "%4x", &prt.vid);
				fclose(fle);
				if (count !=1) continue;

				vidPath = dirName + "/"+subDir+"/idProduct";
				file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				fle = fdopen(file, "r");
				count = fscanf(fle, "%4x", &prt.pid);
				fclose(fle);
				if (count !=1) continue;

				vidPath = dirName + "/"+subDir+"/product";
				file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				fle = fdopen(file, "r");
				char* p;
				if ((p = fgets(target, sizeof(target), fle))) {
					if (p[strlen(p)-1] == '\n') p[strlen(p)-1] = '\0';
					quicka2w(target, prt.productName); 
				}
				fclose(fle);

				vidPath = dirName + "/"+subDir+"/serial";
				file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				fle = fdopen(file, "r");
				if ((p = fgets(target, sizeof(target), fle))) quicka2w(target, prt.instanceID); 
				fclose(fle);

				serialPorts.push_back(prt);
				break;
			}
		}
	}
	closedir(dir);
#endif
}

// Attempt ot change the size of the buffers used by the OS
void SerialIO::setBufferSizes(const unsigned int rxSize, const unsigned int txSize) {
	if (!isPortOpen()) return;

#ifdef _WIN32
	SetupComm(m_portHandle, rxSize, txSize);
#endif
}

// Open a port by name
SerialIO::Response SerialIO::openPort(const std::wstring& portName) {
	closePort();

#ifdef _WIN32
	std::wstring path = L"\\\\.\\" + portName;
	m_portHandle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
	if (m_portHandle == INVALID_HANDLE_VALUE) {
		switch (GetLastError()) {
		case ERROR_FILE_NOT_FOUND:  return Response::rNotFound;
		case ERROR_ACCESS_DENIED:   return Response::rInUse;
		default: return Response::rUnknownError;
		}
	}
	updateTimeouts();
	return Response::rOK;
#else
	std::string apath;
	quickw2a(portName, apath);
	m_portHandle = open(apath.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
	if (m_portHandle == -1) {
		switch (errno) {
		case ENOENT: return Response::rNotFound;
		case EBUSY: return Response::rInUse;
		default: return Response::rUnknownError;
		}
	}

#ifdef HAVE_FLOCK
	flock(m_portHandle, LOCK_EX | LOCK_NB);
#endif
#ifdef TIOCEXCL
	ioctl(m_portHandle, TIOCEXCL);
#endif

	updateTimeouts();
	return Response::rOK;
#endif

	return Response::rNotImplemented;
}

// Shuts the port down
void SerialIO::closePort() {
	if (!isPortOpen()) return;

#ifdef _WIN32
	CloseHandle(m_portHandle);
	m_portHandle = INVALID_HANDLE_VALUE;

#else
	close(m_portHandle);
	m_portHandle = -1;
#endif
}

// Changes the configuration on the port
SerialIO::Response SerialIO::configurePort(const Configuration& configuration) {
	if (!isPortOpen()) return Response::rUnknownError;

#ifdef _WIN32
	// Configure the port
	COMMCONFIG config;
	DWORD comConfigSize = sizeof(config);
	memset(&config, 0, sizeof(config));

	GetCommConfig(m_portHandle, &config, &comConfigSize);
	config.dwSize = sizeof(config);
	config.dcb.DCBlength = sizeof(config.dcb);
	config.dcb.BaudRate = configuration.baudRate;  
	config.dcb.ByteSize = 8;       
	config.dcb.fBinary = true;
	config.dcb.Parity = false;
	config.dcb.fOutxCtsFlow = configuration.ctsFlowControl;
	config.dcb.fOutxDsrFlow = false;
	config.dcb.fDtrControl = DTR_CONTROL_DISABLE; 
	config.dcb.fDsrSensitivity = false;
	config.dcb.fNull = false;
	config.dcb.fTXContinueOnXoff = false;
	config.dcb.fRtsControl = RTS_CONTROL_DISABLE; 
	config.dcb.fAbortOnError = false;
	config.dcb.StopBits = 0;
	config.dcb.fOutX = 0;
	config.dcb.fInX = 0;
	config.dcb.fErrorChar = 0;
	config.dcb.fAbortOnError = 0;
	config.dcb.fInX = 0;
	return SetCommConfig(m_portHandle, &config, sizeof(config)) ? Response::rOK : Response::rUnknownError;

#else	
	if (tcgetattr(m_portHandle, &term) < 0) return Response::rUnknownError;
#ifdef cfmakeraw
	cfmakeraw(&term);
#endif	

	term.c_iflag &= ~ (IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL);
	term.c_lflag &= ~ (ISIG | ICANON | ECHO | ECHOE | ECHONL | IEXTEN);
	term.c_oflag &= ~ (OPOST | ONLCR | OCRNL | ONOCR | ONLRET);
	term.c_cflag &= ~(HUPCL | CSIZE | PARENB | PARODD | CSTOPB);
	term.c_cflag |= CREAD | CLOCAL | CS8;
	term.c_iflag |= IGNPAR;
	
#ifdef XCASE 
	term.c_lflag &= ~XCASE;
#endif
#ifdef IUTF8 
	term.c_iflag &= ~IUTF8;
#endif
#ifdef CMSPAR
	term.c_cflag &= ~CMSPAR;
#endif
#ifdef IMAXBEL
	term.c_iflag &= ~IMAXBEL;
#endif
#ifdef IUCLC
	term.c_iflag &= ~IUCLC;
#endif
#ifdef OLCUC
	term.c_oflag &= ~OLCUC;
#endif
#ifdef NLDLY
	term.c_oflag &= ~NLDLY;
#endif
#ifdef CRDLY
	term.c_oflag &= ~CRDLY;
#endif
#ifdef TABDLY
	term.c_oflag &= ~TABDLY;
#endif
#ifdef BSDLY
	term.c_oflag &= ~BSDLY;
#endif
#ifdef VTDLY
	term.c_oflag &= ~VTDLY;
#endif
#ifdef FFDLY
	term.c_oflag &= ~FFDLY;
#endif
#ifdef OFILL
	term.c_oflag &= ~OFILL;
#endif

	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;

	int ctsRtsFlags = 0;
#ifdef CRTSCTS
	ctsRtsFlags = CRTSCTS;
#else
#ifdef CNEW_RTSCTS
	ctsRtsFlags  = CNEW_RTSCTS;
#else
	ctsRtsFlags = CCTS_OFLOW;
#endif
#endif

	if (configuration.ctsFlowControl) term.c_cflag |= ctsRtsFlags; else term.c_cflag &= ~ctsRtsFlags; 

	// Now try to set the baud rate
	int baud = configuration.baudRate;
#ifdef __APPLE__
	if (ioctl(m_portHandle, IOSSIOSPEED, &baud) == -1) return Response::rUnknownError;
#else
if (baud == 9600) {
	term.c_cflag &= ~CBAUD;
	term.c_cflag |= B9600;
} else {
#if defined(BOTHER) && defined(HAVE_STRUCT_TERMIOS2)
	term.c_cflag &= ~CBAUD;
	term.c_cflag |= BOTHER;
	term.c_ispeed = configuration.baudRate;
	term.c_ospeed = configuration.baudRate;
	if (ioctl(m_portHandle, TCSETS2, &term) < 0) return Response::rUnknownError;
#else
	term.c_cflag &= ~CBAUD;
	term.c_cflag |= CBAUDEX;
	if (cfsetspeed(&term, baud) < 0) return Response::rUnknownError; 
#endif
#endif
}
	// Apply that nonsense
	tcflush (m_portHandle, TCIFLUSH);
	if (tcsetattr(m_portHandle, TCSANOW, &term) != 0) return Response::rUnknownError;
#ifdef ASYNC_LOW_LATENCY	
	struct serial_struct serial;
	ioctl(m_portHandle, TIOCGSERIAL, &serial); 
	serial.flags |= ASYNC_LOW_LATENCY; 
	ioctl(m_portHandle, TIOCSSERIAL, &serial);	
#endif

	setDTR(false);


	return Response::rOK;
#endif

	return Response::rNotImplemented;
}

// Returns the number of bytes waiting to be read
unsigned int SerialIO::getBytesWaiting() {
	if (!isPortOpen()) return 0;
#ifdef _WIN32 
	DWORD errors;
	COMSTAT comstatbuffer;

	if (!ClearCommError(m_portHandle, &errors, &comstatbuffer)) return 0;
	return comstatbuffer.cbInQue;
#else
	int waiting;
	if (ioctl(m_portHandle, TIOCINQ, &waiting) < 0) return 0;
	return (unsigned int)waiting;
#endif
}

// Attempts to write some data to the port.  Returns how much it actually wrote.
// If writeAll is not TRUE then it will write what it can until it times out
unsigned int SerialIO::write(const void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) return 0;
	if (!isPortOpen()) return 0;

#ifdef _WIN32
	DWORD written = 0;
	if (!WriteFile(m_portHandle, data, dataLength, &written, NULL)) written = 0;
	return written;
#else
	unsigned int totalTime = m_writeTimeout + (m_writeTimeoutMultiplier * dataLength);

	struct timeval timeout;
	timeout.tv_sec = totalTime / 1000;
	timeout.tv_usec = (totalTime - (timeout.tv_sec * 1000)) * 1000;

	size_t written = 0;
	unsigned char* buffer = (unsigned char*)data;

	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(m_portHandle, &fds);

	// Write with a timeout
	while (written < dataLength) {
		if ((timeout.tv_sec < 1) && (timeout.tv_usec < 1)) break;
		
		int result = select(m_portHandle + 1, NULL, &fds, NULL, &timeout);		
		if (result < 0) {
			if (errno == EINTR) continue; else {
				return 0;
			}

		}
		else if (result == 0) break;
				
		result = ::write(m_portHandle, buffer, dataLength - written);

		if (result < 0) {
			if (errno == EAGAIN) continue; else {
				return 0;
			}
		}

		written += result;
		buffer += result;
	}
	
	return written;
#endif

	return 0;
}

// Attempts to read some data from the port.  Returns how much it actually read.
// If readAll is TRUE then it will wait until all data has been read or an error occurs
// Returns how mcuh it actually read
unsigned int SerialIO::read(void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) return 0;
	if (!isPortOpen()) return 0;

#ifdef _WIN32
	DWORD read = 0;
	if (!ReadFile(m_portHandle, data, dataLength, &read, NULL)) read = 0;
	return read;
#else
	unsigned int totalTime = m_readTimeout + (m_readTimeoutMultiplier * dataLength);

	struct timeval timeout;
	timeout.tv_sec = totalTime / 1000;
	timeout.tv_usec = (totalTime - (timeout.tv_sec * 1000)) * 1000;

	size_t read = 0;
	unsigned char* buffer = (unsigned char*)data;

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_portHandle, &fds);

	while (read < dataLength) {
		if ((timeout.tv_sec < 1) && (timeout.tv_usec < 1)) {
			break;
		}

		int result = select(m_portHandle + 1, &fds, NULL, NULL, &timeout);

		if (result < 0) {
			if (errno == EINTR) continue; else break;
		}
		else if (result == 0) break;
		result = ::read(m_portHandle, buffer, dataLength - read);

		if (result < 0) {
			if (errno == EAGAIN) continue; else break;
		}
		read += result;
		buffer += result;
	}

	return read;
	
	
#endif

	return 0;
}

// Update timeouts
void SerialIO::updateTimeouts() {
	if (!isPortOpen()) return;

#ifdef _WIN32
	COMMTIMEOUTS tm;
	tm.ReadTotalTimeoutConstant = m_readTimeout;
	tm.ReadTotalTimeoutMultiplier = m_readTimeoutMultiplier;
	tm.ReadIntervalTimeout = 0;
	tm.WriteTotalTimeoutConstant = m_writeTimeout;
	tm.WriteTotalTimeoutMultiplier = m_writeTimeoutMultiplier;
	SetCommTimeouts(m_portHandle, &tm);
#endif
}

// Sets the read timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
void SerialIO::setReadTimeouts(unsigned int waitTimetimeout, unsigned int multiplier) {
	m_readTimeout = waitTimetimeout;
	m_readTimeoutMultiplier = multiplier;

	updateTimeouts();
}

// Sets the write timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
void SerialIO::setWriteTimeouts(unsigned int waitTimetimeout, unsigned int multiplier) {
	m_writeTimeout = waitTimetimeout;
	m_writeTimeoutMultiplier = multiplier;

	updateTimeouts();
}