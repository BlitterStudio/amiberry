/* ArduinoFloppyReader (and writer)
*
* Copyright (C) 2017-2022 Robert Smith (@RobSmithDev)
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

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS


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

// OS X, sigurbjornl, 20220208
#elif defined __MACH__

#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>
#include <term.h>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#ifndef TIOCINQ
#ifdef FIONREAD
#define TIOCINQ FIONREAD
#else
#define TIOCINQ 0x541B
#endif
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
#include <algorithm>

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
#ifdef FTDI_D2XX_AVAILABLE
	m_ftdi.FT_Close();
#endif
	closePort();
}

// Returns TRUE if the port is open
bool SerialIO::isPortOpen() const {
#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) return true;
#endif
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

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		m_ftdi.FT_Purge(true, true);
		return;
	}
#endif

#ifdef _WIN32
	PurgeComm(m_portHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
	tcflush(m_portHandle, TCIOFLUSH);
#endif
}

// Sets the status of the DTR line
void SerialIO::setRTS(bool enableRTS) {
	if (!isPortOpen()) return;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		if (enableRTS) m_ftdi.FT_SetRts(); else m_ftdi.FT_ClrRts();
		return;
	}
#endif

#ifdef _WIN32
	EscapeCommFunction(m_portHandle, enableRTS ? SETRTS : CLRRTS);
#else
	int pinToControl = TIOCM_RTS;
	ioctl(m_portHandle, enableRTS ? TIOCMBIS : TIOCMBIC, &pinToControl);
#endif
}

// Sets the status of the DTR line
void SerialIO::setDTR(bool enableDTR) {
	if (!isPortOpen()) return;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		if (enableDTR) m_ftdi.FT_SetDtr(); else m_ftdi.FT_ClrDtr();
		return;
	}
#endif

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

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		ULONG status;
		if (m_ftdi.FT_GetModemStatus(&status) != FTDI::FT_STATUS::FT_OK) return false;
		return (status & FT_MODEM_STATUS_CTS) != 0;
	}
#endif

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

#ifdef FTDI_D2XX_AVAILABLE
	// Add in the FTDI ports detected
	DWORD numDevs;
	FTDI::FTDIInterface ftdi;
	FTDI::FT_STATUS status = ftdi.FT_CreateDeviceInfoList(&numDevs);
	if ((status == FTDI::FT_STATUS::FT_OK) && (numDevs)) {
		FTDI::FT_DEVICE_LIST_INFO_NODE* devList = (FTDI::FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FTDI::FT_DEVICE_LIST_INFO_NODE) * numDevs);
		if (devList) {

			status = ftdi.FT_GetDeviceInfoList(devList, &numDevs);
			if (status == FTDI::FT_STATUS::FT_OK) {
				for (unsigned int index = 0; index < numDevs; index++) {
					SerialPortInformation info;
					info.instanceID = std::to_wstring(devList[index].LocId);
					info.pid = devList[index].ID & 0xFFFF;
					info.vid = devList[index].ID >> 16;
					quicka2w(devList[index].Description, info.productName);

					// Ensure no duplicate port numbers names
					int portIndex = 0;
					do {
						// Create name
						std::string tmp = FTDI_PORT_PREFIX;
						if (portIndex) tmp += std::to_string(portIndex) + "-";
						portIndex++;
						tmp += std::string(devList[index].SerialNumber);

						// Convert to wide
						quicka2w(tmp, info.portName);

						// Finish it
						info.portName += L" (" + info.productName + L")";
						info.ftdiIndex = index;

						// Test if one with this name already exists
					} while (std::find_if(serialPorts.begin(), serialPorts.end(), [&info](const SerialPortInformation& port)->bool {
						return (info.portName == port.portName);
					}) != serialPorts.end());

					// Save
					serialPorts.push_back(info);
				}
			}
			free(devList);
		}
	}
#endif


#ifdef _WIN32
	std::vector<GUID> toSearch;

	// Check normal COMPORTS guids
	toSearch.push_back(GUID_DEVINTERFACE_COMPORT);

	DWORD requiredSize = 8;
	std::vector< GUID> tmp(8);
	// Check 'PORTS' Guids
	if (SetupDiClassGuidsFromNameA("Ports", (LPGUID)tmp.data(), requiredSize, &requiredSize)) {
		if (requiredSize > 8) {
			tmp.resize(requiredSize);
			if (!SetupDiClassGuidsFromNameA("Ports", (LPGUID)tmp.data(), requiredSize, &requiredSize)) requiredSize = 0;
		}
		// Don't add duplicates
		for (size_t c = 0; c < requiredSize; c++)
			if (std::find(toSearch.begin(), toSearch.end(), tmp[c]) == toSearch.end())
				toSearch.push_back(tmp[c]);
	}
	requiredSize = 8;
	tmp.resize(8);
	// Check 'MODEM' Guids
	if (SetupDiClassGuidsFromNameA("Modem", (LPGUID)tmp.data(), requiredSize, &requiredSize)) {
		if (requiredSize > 8) {
			tmp.resize(requiredSize);
			if (!SetupDiClassGuidsFromNameA("Modem", (LPGUID)tmp.data(), requiredSize, &requiredSize)) requiredSize = 0;
		}
		// Don't add duplicates
		for (size_t c = 0; c < requiredSize; c++)
			if (std::find(toSearch.begin(), toSearch.end(), tmp[c]) == toSearch.end())
				toSearch.push_back(tmp[c]);
	}

	for (GUID g : toSearch) {
		// Query for hardware
		HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&g, nullptr, nullptr, DIGCF_PRESENT);
		if (hDevInfoSet == INVALID_HANDLE_VALUE) return;

		// Scan for items
		DWORD devIndex = 0;
		SP_DEVINFO_DATA devInfo;
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

		// Enum devices
		while (SetupDiEnumDeviceInfo(hDevInfoSet, devIndex, &devInfo)) {
			HKEY key = SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
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

						// Don't add any duplicates
						if (std::find_if(serialPorts.begin(), serialPorts.end(), [&port](SerialPortInformation search)->bool {
							return search.portName == port.portName;
						}) == serialPorts.end()) serialPorts.push_back(port);
					}
				}
				RegCloseKey(key);
			}

			devIndex++;
		}

		SetupDiDestroyDeviceInfoList(hDevInfoSet);
	}

#else

	DIR* dir = opendir("/sys/class/tty");
	if (!dir) return;

	dirent* entry;
	struct stat statbuf{};

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
			std::string subDir;

			for (int i = 0; i < 5; i++) {
				subDir += "../";
				std::string vidPath = dirName + "/";
				vidPath.append(subDir + "/idVendor");

				int file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				FILE* fle = fdopen(file, "r");
				int count = fscanf(fle, "%4x", &prt.vid);
				fclose(fle);
				if (count != 1) continue;

				vidPath = dirName + "/";
				vidPath.append(subDir + "/idProduct");
				file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				fle = fdopen(file, "r");
				count = fscanf(fle, "%4x", &prt.pid);
				fclose(fle);
				if (count != 1) continue;

				vidPath = dirName + "/";
				vidPath.append(subDir + "/product");
				file = open(vidPath.c_str(), O_RDONLY | O_CLOEXEC);
				if (file == -1) continue;
				fle = fdopen(file, "r");
				char* p;
				if ((p = fgets(target, sizeof(target), fle))) {
					if (p[strlen(p) - 1] == '\n') p[strlen(p) - 1] = '\0';
					quicka2w(target, prt.productName);
				}
				fclose(fle);

				vidPath = dirName + "/";
				vidPath.append(subDir + "/serial");
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

	std::sort(serialPorts.begin(), serialPorts.end(), [](const SerialPortInformation& a, const SerialPortInformation& b)->int {
		return a.portName < b.portName;
	});
}

// Attempt ot change the size of the buffers used by the OS
void SerialIO::setBufferSizes(const unsigned int rxSize, const unsigned int txSize) {
	if (!isPortOpen()) return;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		// Larger than this size actually causes slowdowns.  This doesn't work the same as below.  Below is a buffer in Windows.  This is on the USB device I think
		m_ftdi.FT_SetUSBParameters(rxSize < 256 ? rxSize : 256, txSize);
		return;
	}
#endif

#ifdef _WIN32
	SetupComm(m_portHandle, rxSize, txSize);
#endif
}

// Open a port by name
SerialIO::Response SerialIO::openPort(const std::wstring& portName) {
	closePort();

#ifdef FTDI_D2XX_AVAILABLE
	if (portName.length() > std::string(FTDI_PORT_PREFIX).length()) {
		std::wstring prefix;
		quicka2w(FTDI_PORT_PREFIX, prefix);
		// Is it FTDI?
		if (portName.substr(0, prefix.length()) == prefix) {
			std::vector<SerialPortInformation> serialPorts;
			enumSerialPorts(serialPorts);

			// See if it exists
			auto f = std::find_if(serialPorts.begin(), serialPorts.end(), [&portName](const SerialPortInformation& serialport)-> bool {
				return portName == serialport.portName;
			});

			// was it found?
			if (f != serialPorts.end()) {
				switch (m_ftdi.FT_Open(f->ftdiIndex)) {
					case FTDI::FT_STATUS::FT_OK: break;
					case FTDI::FT_STATUS::FT_DEVICE_NOT_OPENED: return Response::rInUse;
					case FTDI::FT_STATUS::FT_DEVICE_NOT_FOUND: return Response::rNotFound;
					default: return Response::rUnknownError;
				}
				updateTimeouts();

				return Response::rOK;
			} else return Response::rNotFound;
		}
	}
#endif

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
	m_portHandle = open(apath.c_str(), O_RDWR | O_NOCTTY);
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

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		m_ftdi.FT_Close();
		return;
	}
#endif

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

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		if (m_ftdi.FT_SetFlowControl(configuration.ctsFlowControl ? FT_FLOW_RTS_CTS : FT_FLOW_NONE, 0, 0) != FTDI::FT_STATUS::FT_OK) return SerialIO::Response::rUnknownError;
		if (m_ftdi.FT_SetDataCharacteristics(FTDI::FT_BITS::_8, FTDI::FT_STOP_BITS::_1, FTDI::FT_PARITY::NONE) != FTDI::FT_STATUS::FT_OK) return SerialIO::Response::rUnknownError;
		if (m_ftdi.FT_SetBaudRate(configuration.baudRate) != FTDI::FT_STATUS::FT_OK) return SerialIO::Response::rUnknownError;
		m_ftdi.FT_SetLatencyTimer(2);
		m_ftdi.FT_ClrDtr();
		m_ftdi.FT_ClrRts();
		return SerialIO::Response::rOK;
	}
#endif

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
	config.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	config.dcb.fDsrSensitivity = false;
	config.dcb.fNull = false;
	config.dcb.fTXContinueOnXoff = false;
	config.dcb.fRtsControl = RTS_CONTROL_ENABLE;
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
	term.c_cc[VTIME] = 1;

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
	}
#endif
	// Apply that nonsense
	tcflush (m_portHandle, TCIFLUSH);
	if (tcsetattr(m_portHandle, TCSANOW, &term) != 0) return Response::rUnknownError;
#ifdef ASYNC_LOW_LATENCY
	struct serial_struct serial;
	ioctl(m_portHandle, TIOCGSERIAL, &serial);
	serial.flags |= ASYNC_LOW_LATENCY;
	ioctl(m_portHandle, TIOCSSERIAL, &serial);
#endif

	setDTR(true);
	setRTS(true);


	return Response::rOK;
#endif

	return Response::rNotImplemented;
}

// Check if we wrre quick enough reading the data
bool SerialIO::checkForOverrun() {
	if (!isPortOpen()) return false;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		ULONG status;
		if (m_ftdi.FT_GetModemStatus(&status) != FTDI::FT_STATUS::FT_OK) return false;
		return (status & (FT_MODEM_STATUS_OE | FT_MODEM_STATUS_FE)) != 0;
	}
#endif

#ifdef _WIN32
	DWORD errors=0;
	COMSTAT comstatbuffer;

	if (!ClearCommError(m_portHandle, &errors, &comstatbuffer)) return 0;
	return (errors & (CE_OVERRUN | CE_FRAME | CE_RXOVER)) != 0;
#else
	return false;
#endif
}

// Returns the number of bytes waiting to be read
unsigned int SerialIO::getBytesWaiting() {
	if (!isPortOpen()) return 0;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		DWORD queueSize = 0;
		if (m_ftdi.FT_GetQueueStatus(&queueSize) != FTDI::FT_STATUS::FT_OK) return 0;
		return queueSize;
	}
#endif

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

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier * dataLength), m_writeTimeout + (m_writeTimeoutMultiplier * dataLength));

		DWORD written = 0;
		if (m_ftdi.FT_Write((LPVOID)data, dataLength, &written) != FTDI::FT_STATUS::FT_OK) written = 0;
		return written;
	}
#endif

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
			if (errno == EINTR || errno == EAGAIN) continue; else return 0;
		}
		else if (result == 0) break;

		result = ::write(m_portHandle, buffer, dataLength - written);

		if (result < 0) {
			if (errno == EINTR || errno == EAGAIN) continue; else return 0;
		}

		written += result;
		buffer += result;
	}

	return written;
#endif

	return 0;
}

// A very simple, uncluttered version of the below, mainly for linux
unsigned int SerialIO::justRead(void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) return 0;
	if (!isPortOpen()) return 0;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier * dataLength), m_writeTimeout + (m_writeTimeoutMultiplier * dataLength));

		DWORD dataRead = 0;
		if (m_ftdi.FT_Read((LPVOID)data, dataLength, &dataRead) != FTDI::FT_STATUS::FT_OK) dataRead = 0;
		return dataRead;
	}
#endif

#ifdef _WIN32
	DWORD read = 0;
	if (!ReadFile(m_portHandle, data, dataLength, &read, NULL)) read = 0;
	return read;
#else
	int result = ::read(m_portHandle, data, dataLength);

	if (result < 0) return 0;

	return (unsigned int)result;


#endif

	return 0;

}

// Attempts to read some data from the port.  Returns how much it actually read.
// Returns how much it actually read
unsigned int SerialIO::read(void* data, unsigned int dataLength) {
	if ((data == nullptr) || (dataLength == 0)) return 0;
	if (!isPortOpen()) return 0;

#ifdef FTDI_D2XX_AVAILABLE
	if (m_ftdi.isOpen()) {
		m_ftdi.FT_SetTimeouts(m_readTimeout + (m_readTimeoutMultiplier * dataLength), m_writeTimeout + (m_writeTimeoutMultiplier * dataLength));

		DWORD dataRead = 0;
		if (m_ftdi.FT_Read((LPVOID)data, dataLength, &dataRead) != FTDI::FT_STATUS::FT_OK) dataRead = 0;
		return dataRead;
	}
#endif

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
			if (errno == EINTR || errno == EAGAIN) continue; else return 0;
		}
		else if (result == 0) break;
		result = ::read(m_portHandle, buffer, dataLength - read);

		if (result < 0) {
			if (errno == EINTR || errno == EAGAIN) continue; else return 0;
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
