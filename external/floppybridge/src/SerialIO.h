#ifndef DISKREADERWRITER_SERIAL_IO
#define DISKREADERWRITER_SERIAL_IO
/* ArduinoFloppyReader (and writer)
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
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*/

////////////////////////////////////////////////////////////////////////////////////////
// Class to manage the communication between the computer and a serial port           //
////////////////////////////////////////////////////////////////////////////////////////
//
//

#include <string>
#include <vector>
#include <stdint.h>

#ifdef _WIN32
#ifdef USING_MFC
#include <afxwin.h>
#else
#include <Windows.h>
#endif
#else
#include <termios.h>
#endif

#include "ftdi.h"

#define FTDI_PORT_PREFIX "FTDI:"

extern void quickw2a(const std::wstring& wstr, std::string& str);
extern void quicka2w(const std::string& str, std::wstring& wstr);


class SerialIO {
private:
	unsigned int m_readTimeout = 0, m_readTimeoutMultiplier = 0;
	unsigned int m_writeTimeout = 0, m_writeTimeoutMultiplier = 0;
#ifdef FTDI_D2XX_AVAILABLE
	FTDI::FTDIInterface m_ftdi;
#endif

#ifdef _WIN32
	HANDLE m_portHandle = INVALID_HANDLE_VALUE;
#else
	int m_portHandle = -1;
#ifdef HAVE_STRUCT_TERMIOS2
	struct termios2 term;
#else
	struct termios term;
#endif
#endif

	// Update timeouts
	void updateTimeouts();

public:
	// Definition of a serial port
	struct SerialPortInformation {
		// The "file" name of the port
		std::wstring portName;
		// Port details
		unsigned int vid = 0, pid = 0;
		// Product name
		std::wstring productName;
		// Instance ID
		std::wstring instanceID;
#ifdef FTDI_D2XX_AVAILABLE
		// Set if this is an FTDI device index
		int ftdiIndex = -1;
#endif
	};

	// Configuration settings for the port
	struct Configuration {
		unsigned int baudRate	= 9600;		
		bool ctsFlowControl		= false;
	};

	enum class Response { rOK, rInUse, rNotFound, rUnknownError, rNotImplemented };

	// Constructor etc
	SerialIO();
	virtual ~SerialIO();

	// Sets the status of the DTR line
	void setDTR(bool enableDTR);

	// Sets the status of the RTS line
	void setRTS(bool enableRTS);

	// Returns the status of the CTS pin
	bool getCTSStatus();

	// Returns TRUE if the port is open
	bool isPortOpen() const;

	// Returns a list of serial ports discovered on the system
	static void enumSerialPorts(std::vector<SerialPortInformation>& serialPorts);

	// Purge any data left in the buffer
	void purgeBuffers();

	// Returns the number of bytes waiting to be read
	unsigned int getBytesWaiting();

	// Check if we were quick enough reading the data
	bool checkForOverrun();

	// Open a port by name
	Response openPort(const std::wstring& portName);

	// Shuts the port down
	void closePort();

	// Attempt ot change the size of the buffers used by the OS
	void setBufferSizes(const unsigned int rxSize, const unsigned int txSize);

	// Changes the configuration on the port
	Response configurePort(const Configuration& configuration);

	// Attempts to write some data to the port.  Returns how much it actually wrote.
	unsigned int write(const void* data, unsigned int dataLength);

	// Attempts to read some data from the port.  Returns how much it actually read.
	// Returns how mcuh it actually read
	unsigned int read(void* data, unsigned int dataLength);

	// A very simple, uncluttered version of the above, mainly for linux
	unsigned int justRead(void* data, unsigned int dataLength);

	// Sets the read timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
	void setReadTimeouts(unsigned int waitTimetimeout, unsigned int multiplier);

	// Sets the write timeouts. The actual timeout is calculated as waitTimetimeout + (multiplier * num bytes)
	void setWriteTimeouts(unsigned int waitTimetimeout, unsigned int multiplier);
};
 

#endif
