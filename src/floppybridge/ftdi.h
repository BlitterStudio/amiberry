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

/*
  This is a FTDI driver class, based on the FTDI2XX.h file from FTDI.
  This library dynamically loads the DLL rather than static linking to it.
*/

#ifndef FTDI_CLASS_H
#define FTDI_CLASS_H

#define FTDI_D2XX_AVAILABLE

#ifdef FTDI_D2XX_AVAILABLE

namespace FTDI {	


#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#include <stdint.h>
#ifndef DWORD
#define DWORD uint32_t
#endif
#ifndef LPDWORD
#define LPDWORD uint32_t*
#endif
#ifndef ULONG
#define ULONG uint32_t
#endif
#ifndef LONG
#define LONG int32_t
#endif
#ifndef LPLONG
#define LPLONG int32_t*
#endif
#ifndef UCHAR
#define UCHAR unsigned char
#endif
#ifndef USHORT
#define USHORT uint16_t
#endif
#ifndef LPVOID
#define LPVOID void*
#endif
#ifndef LPOVERLAPPED
#define LPOVERLAPPED void*
#endif
#ifndef PCHAR
#define PCHAR char*
#endif
#endif

	typedef ULONG FT_HANDLE;

	// Device status
	enum class FT_STATUS : DWORD {
		FT_OK = 0,
		FT_INVALID_HANDLE = 1,
		FT_DEVICE_NOT_FOUND = 2,
		FT_DEVICE_NOT_OPENED = 3,
		FT_IO_ERROR = 4,
		FT_INSUFFICIENT_RESOURCES = 5,
		FT_INVALID_PARAMETER = 6,
		FT_INVALID_BAUD_RATE = 7,
		FT_DEVICE_NOT_OPENED_FOR_ERASE = 8,
		FT_DEVICE_NOT_OPENED_FOR_WRITE = 9,
		FT_FAILED_TO_WRITE_DEVICE = 10,
		FT_EEPROM_READ_FAILED = 11,
		FT_EEPROM_WRITE_FAILED = 12,
		FT_EEPROM_ERASE_FAILED = 13,
		FT_EEPROM_NOT_PRESENT = 14,
		FT_EEPROM_NOT_PROGRAMMED = 15,
		FT_INVALID_ARGS = 16,
		FT_NOT_SUPPORTED = 17,
		FT_OTHER_ERROR = 18,
		FT_DRIVER_NOT_AVAILABLE 
	};

	#define FT_SUCCESS(status) ((status) == FT_OK)

	// FT_OpenEx Flags
	enum class FT_OPENEX : UCHAR {
		BY_SERIAL_NUMBER = 1,
		BY_DESCRIPTION = 2,
		BY_LOCATION = 4
	};

	// FT_ListDevices Flags (used in conjunction with FT_OpenEx Flags
	#define FT_LIST_NUMBER_ONLY			0x80000000
	#define FT_LIST_BY_INDEX			0x40000000
	#define FT_LIST_ALL					0x20000000

	#define FT_LIST_MASK (FT_LIST_NUMBER_ONLY|FT_LIST_BY_INDEX|FT_LIST_ALL)

	// Device type
	enum class FT_DEVICE : DWORD {		
		_232BM = 0,
		_232AM = 1,
		_100AX = 2,
		_UNKNOWN = 3,
		_2232C = 4,
		_232R = 5,
		_2232H = 6,
		_4232H = 7,
		_232H = 8,
		_X_SERIES = 9
	};

	// Common Baud Rates
	#define FT_BAUD_300			300
	#define FT_BAUD_600			600
	#define FT_BAUD_1200		1200
	#define FT_BAUD_2400		2400
	#define FT_BAUD_4800		4800
	#define FT_BAUD_9600		9600
	#define FT_BAUD_14400		14400
	#define FT_BAUD_19200		19200
	#define FT_BAUD_38400		38400
	#define FT_BAUD_57600		57600
	#define FT_BAUD_115200		115200
	#define FT_BAUD_230400		230400
	#define FT_BAUD_460800		460800
	#define FT_BAUD_921600		921600

	#define FT_MODEM_STATUS_BI		0x01			// Break Interrupt
	#define FT_MODEM_STATUS_OE		0x02			// Overrun Error
	#define FT_MODEM_STATUS_PE		0x04			// Parity Error
	#define FT_MODEM_STATUS_FE		0x08			// Framing Error
	#define FT_MODEM_STATUS_CTS		0x10			// Clear to Send
	#define FT_MODEM_STATUS_DSR		0x20			// Data Set Ready
	#define FT_MODEM_STATUS_RI		0x40			// Ring Indicator
	#define FT_MODEM_STATUS_DCD		0x80			// Data Carrier Detect

	// Word Lengths
	enum class FT_BITS : UCHAR {
		_8 = 8,
		_7 = 7,
		_6 = 6,
		_5 = 5
	};
	
	// Stop Bits
	enum class FT_STOP_BITS : UCHAR {
		_1	 = 0,
		_1_5 = 1,
		_2	 = 2 
	};

	// Parity
	enum class FT_PARITY : UCHAR {
		NONE  = 0,
		ODD   = 1,
		EVEN  = 2,
		MARK  = 3,
		SPACE = 4
	};

	// Flow Control
	#define FT_FLOW_NONE        0x0000
	#define FT_FLOW_RTS_CTS     0x0100
	#define FT_FLOW_DTR_DSR     0x0200
	#define FT_FLOW_XON_XOFF    0x0400
	
	// Events
	typedef void (*PFT_EVENT_HANDLER)(DWORD,DWORD);

	#define FT_EVENT_RXCHAR		    1
	#define FT_EVENT_MODEM_STATUS   2
	#define FT_EVENT_LINE_STATUS    4

	// FT_DEVICE_LIST_INFO_NODE
	typedef struct _ft_device_list_info_node {
		DWORD Flags;
		DWORD Type;
		DWORD ID;
		DWORD LocId;
		char SerialNumber[16];
		char Description[64];
		FT_HANDLE ftHandle;
	} FT_DEVICE_LIST_INFO_NODE;

	// Driver Type
	enum class FT_DTIVER_TYPE : DWORD {
		D2XX = 0,
		VCP = 1
	};

	// Timeouts
	#define FT_DEFAULT_RX_TIMEOUT   300
	#define FT_DEFAULT_TX_TIMEOUT   300


	class FTDIInterface {
	private:
		FT_HANDLE m_handle = 0;
	public:

		FTDIInterface();
		~FTDIInterface();

		// Return TRUE if the port is open
		bool isOpen() const { return m_handle != 0; };

		FT_STATUS FT_Open(int deviceNumber);
		FT_STATUS FT_OpenEx(LPVOID pArg1, DWORD Flags);

		FT_STATUS FT_ListDevices(LPVOID pArg1, LPVOID pArg2, DWORD Flags);

		FT_STATUS FT_Close();

		FT_STATUS FT_Read(LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesReturned);
		FT_STATUS FT_Write(LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesWritten);

		FT_STATUS FT_IoCtl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);

		FT_STATUS FT_SetBaudRate(ULONG BaudRate);
		FT_STATUS FT_SetDivisor(USHORT Divisor);
		FT_STATUS FT_SetTimeouts(ULONG ReadTimeout, ULONG WriteTimeout);

		FT_STATUS FT_SetDataCharacteristics(FT_BITS WordLength, FT_STOP_BITS StopBits, FT_PARITY Parity);
		FT_STATUS FT_SetFlowControl(USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);

		FT_STATUS FT_ResetDevice();
		FT_STATUS FT_GetQueueStatus(DWORD* dwRxBytes);

		FT_STATUS FT_SetDtr();
		FT_STATUS FT_ClrDtr();
		FT_STATUS FT_SetRts();
		FT_STATUS FT_ClrRts();

		FT_STATUS FT_GetModemStatus(ULONG* pModemStatus);

		FT_STATUS FT_SetChars(UCHAR EventChar, UCHAR EventCharEnabled, UCHAR ErrorChar, UCHAR ErrorCharEnabled);

		FT_STATUS FT_Purge(bool purgeRX, bool purgeTX);


		FT_STATUS FT_SetEventNotification(DWORD Mask,LPVOID Param);
		FT_STATUS FT_GetEventStatus(DWORD* dwEventDWord);
		FT_STATUS FT_GetStatus(DWORD* dwRxBytes, DWORD* dwTxBytes, DWORD* dwEventDWord);

		FT_STATUS FT_SetBreakOn();
		FT_STATUS FT_SetBreakOff();

		FT_STATUS FT_SetWaitMask(DWORD Mask);
		FT_STATUS FT_WaitOnMask(DWORD* Mask);

		FT_STATUS FT_CreateDeviceInfoList(LPDWORD lpdwNumDevs);
		FT_STATUS FT_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* pDest, LPDWORD lpdwNumDevs);
		FT_STATUS FT_GetDeviceInfoDetail(DWORD dwIndex, LPDWORD lpdwFlags, LPDWORD lpdwType, LPDWORD lpdwID, LPDWORD lpdwLocId, PCHAR pcSerialNumber, PCHAR pcDescription, FT_HANDLE* handle);
		FT_STATUS FT_GetDriverVersion(LPDWORD lpdwDriverVersion);
		FT_STATUS FT_GetLibraryVersion(LPDWORD lpdwDLLVersion);
		FT_STATUS FT_ResetPort();
		FT_STATUS FT_CyclePort();
		FT_STATUS FT_GetComPortNumber(FT_HANDLE handle, LPLONG port);
		FT_STATUS FT_SetUSBParameters(DWORD dwInTransferSize, DWORD dwOutTransferSize);
		FT_STATUS FT_SetLatencyTimer(UCHAR ucTimer);
	};
};

#endif
#endif