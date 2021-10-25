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
  This library dynamically loads the DLL/SO rather than static linking to it.
*/
#include "ftdi.h"

#ifdef FTDI_D2XX_AVAILABLE

using namespace FTDI;

#ifndef _WIN32
#include <dlfcn.h>
#endif

// Based on the FTDI2XX.h file, but with dynamic loading

static int libraryLoadCounter = 0;

// Purge rx and tx buffers
#define FT_PURGE_RX         1
#define FT_PURGE_TX         2

#ifdef _WIN32
#define CALLING_CONVENTION WINAPI
#define GETFUNC GetProcAddress
#else
#define CALLING_CONVENTION
#define GETFUNC dlsym
#endif

typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Open)(int deviceNumber, FT_HANDLE *pHandle) ;
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_OpenEx)(LPVOID pArg1, DWORD Flags, FT_HANDLE *pHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ListDevices)( LPVOID pArg1, LPVOID pArg2, DWORD Flags);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Close)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetComPortNumber)(FT_HANDLE ftHandle, LPLONG lplComPortNumber);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Read)(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesReturned);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Write)(FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesWritten);	 
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_IoCtl)(FT_HANDLE ftHandle, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetBaudRate)(FT_HANDLE ftHandle, ULONG BaudRate);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetDivisor)(FT_HANDLE ftHandle, USHORT Divisor);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetDataCharacteristics)(FT_HANDLE ftHandle, UCHAR WordLength, UCHAR StopBits, UCHAR Parity);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetFlowControl)(FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ResetDevice)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetDtr)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ClrDtr)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetRts)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_ClrRts)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetModemStatus)(FT_HANDLE ftHandle,ULONG *pModemStatus);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetChars)(FT_HANDLE ftHandle,UCHAR EventChar,UCHAR EventCharEnabled,UCHAR ErrorChar,UCHAR ErrorCharEnabled);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_Purge)(FT_HANDLE ftHandle,ULONG Mask);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetTimeouts)(FT_HANDLE ftHandle,ULONG ReadTimeout,ULONG WriteTimeout);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetQueueStatus)(FT_HANDLE ftHandle,DWORD *dwRxBytes);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetEventNotification)(FT_HANDLE ftHandle,DWORD Mask,LPVOID Param);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetEventStatus)(FT_HANDLE ftHandle,DWORD *dwEventDWord);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_GetStatus)(FT_HANDLE ftHandle,DWORD *dwRxBytes,DWORD *dwTxBytes,DWORD *dwEventDWord);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetBreakOn)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetBreakOff)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_SetWaitMask)(FT_HANDLE ftHandle,DWORD Mask);
typedef FTDI::FT_STATUS (CALLING_CONVENTION *_FT_WaitOnMask)(FT_HANDLE ftHandle,DWORD *Mask);

typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_CreateDeviceInfoList)(LPDWORD lpdwNumDevs);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetDeviceInfoList)(FT_DEVICE_LIST_INFO_NODE* pDest, LPDWORD lpdwNumDevs);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetDeviceInfoDetail)(DWORD dwIndex, LPDWORD lpdwFlags, LPDWORD lpdwType, LPDWORD lpdwID, LPDWORD lpdwLocId, PCHAR pcSerialNumber, PCHAR pcDescription, FT_HANDLE* ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetDriverVersion)(FT_HANDLE ftHandle, LPDWORD lpdwDriverVersion);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetLibraryVersion)(LPDWORD lpdwDLLVersion);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_ResetPort)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_CyclePort)(FT_HANDLE ftHandle);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_GetComPortNumber)(FT_HANDLE ftHandle, LPLONG port);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_SetUSBParameters)(FT_HANDLE ftHandle, DWORD dwInTransferSize, DWORD dwOutTransferSize);
typedef FTDI::FT_STATUS (CALLING_CONVENTION* _FT_SetLatencyTimer)(FT_HANDLE ftHandle, UCHAR ucTimer);

_FT_Open	FT_Open = nullptr;
_FT_OpenEx	FT_OpenEx = nullptr;
_FT_ListDevices	FT_ListDevices = nullptr;
_FT_Close	FT_Close = nullptr;
_FT_Read	FT_Read = nullptr;
_FT_Write	FT_Write = nullptr;
_FT_IoCtl 	FT_IoCtl = nullptr;
_FT_SetBaudRate	FT_SetBaudRate = nullptr;
_FT_SetDivisor	FT_SetDivisor = nullptr;
_FT_SetDataCharacteristics	FT_SetDataCharacteristics = nullptr;
_FT_SetFlowControl	FT_SetFlowControl = nullptr;
_FT_ResetDevice	FT_ResetDevice = nullptr;
_FT_SetDtr	FT_SetDtr = nullptr;
_FT_ClrDtr	FT_ClrDtr = nullptr;
_FT_SetRts	FT_SetRts = nullptr;
_FT_ClrRts	FT_ClrRts = nullptr;
_FT_GetModemStatus	FT_GetModemStatus = nullptr;
_FT_SetChars	FT_SetChars = nullptr;
_FT_Purge	FT_Purge = nullptr;
_FT_SetTimeouts	FT_SetTimeouts = nullptr;
_FT_GetQueueStatus	FT_GetQueueStatus = nullptr;
_FT_SetEventNotification	FT_SetEventNotification = nullptr;
_FT_GetEventStatus	FT_GetEventStatus = nullptr;
_FT_GetStatus	FT_GetStatus = nullptr;
_FT_SetBreakOn	FT_SetBreakOn = nullptr;
_FT_SetBreakOff	FT_SetBreakOff = nullptr;
_FT_SetWaitMask	FT_SetWaitMask = nullptr;
_FT_WaitOnMask	FT_WaitOnMask = nullptr;

_FT_CreateDeviceInfoList FT_CreateDeviceInfoList = nullptr;
_FT_GetDeviceInfoList FT_GetDeviceInfoList = nullptr;
_FT_GetDeviceInfoDetail FT_GetDeviceInfoDetail = nullptr;
_FT_GetDriverVersion FT_GetDriverVersion = nullptr;
_FT_GetLibraryVersion FT_GetLibraryVersion = nullptr;
_FT_ResetPort FT_ResetPort = nullptr;
_FT_CyclePort FT_CyclePort = nullptr;
_FT_GetComPortNumber FT_GetComPortNumber = nullptr;
_FT_SetUSBParameters FT_SetUSBParameters = nullptr;
_FT_SetLatencyTimer FT_SetLatencyTimer = nullptr;

#ifdef _WIN32
HMODULE m_dll = 0;
#else
void* m_dll = nullptr;
#endif


void initFTDILibrary() {
	libraryLoadCounter++;
	if (libraryLoadCounter == 1) {
#ifdef WIN32
		m_dll = LoadLibrary(L"FTD2XX.DLL");
#else
		m_dll = dlopen("libftd2xx.so", RTLD_NOW);
#endif
		if (m_dll) {
			FT_Open = (_FT_Open)GETFUNC(m_dll, "FT_Open");
			FT_OpenEx = (_FT_OpenEx)GETFUNC(m_dll, "FT_OpenEx");
			FT_ListDevices = (_FT_ListDevices)GETFUNC(m_dll, "FT_ListDevices");
			FT_Close = (_FT_Close)GETFUNC(m_dll, "FT_Close");
			FT_Read = (_FT_Read)GETFUNC(m_dll, "FT_Read");
			FT_Write = (_FT_Write)GETFUNC(m_dll, "FT_Write");
			FT_IoCtl = (_FT_IoCtl)GETFUNC(m_dll, "FT_IoCtl ");
			FT_SetBaudRate = (_FT_SetBaudRate)GETFUNC(m_dll, "FT_SetBaudRate");
			FT_SetDivisor = (_FT_SetDivisor)GETFUNC(m_dll, "FT_SetDivisor");
			FT_SetDataCharacteristics = (_FT_SetDataCharacteristics)GETFUNC(m_dll, "FT_SetDataCharacteristics");
			FT_SetFlowControl = (_FT_SetFlowControl)GETFUNC(m_dll, "FT_SetFlowControl");
			FT_ResetDevice = (_FT_ResetDevice)GETFUNC(m_dll, "FT_ResetDevice");
			FT_SetDtr = (_FT_SetDtr)GETFUNC(m_dll, "FT_SetDtr");
			FT_ClrDtr = (_FT_ClrDtr)GETFUNC(m_dll, "FT_ClrDtr");
			FT_SetRts = (_FT_SetRts)GETFUNC(m_dll, "FT_SetRts");
			FT_ClrRts = (_FT_ClrRts)GETFUNC(m_dll, "FT_ClrRts");
			FT_GetModemStatus = (_FT_GetModemStatus)GETFUNC(m_dll, "FT_GetModemStatus");
			FT_SetChars = (_FT_SetChars)GETFUNC(m_dll, "FT_SetChars");
			FT_Purge = (_FT_Purge)GETFUNC(m_dll, "FT_Purge");
			FT_SetTimeouts = (_FT_SetTimeouts)GETFUNC(m_dll, "FT_SetTimeouts");
			FT_GetQueueStatus = (_FT_GetQueueStatus)GETFUNC(m_dll, "FT_GetQueueStatus");
			FT_SetEventNotification = (_FT_SetEventNotification)GETFUNC(m_dll, "FT_SetEventNotification");
			FT_GetEventStatus = (_FT_GetEventStatus)GETFUNC(m_dll, "FT_GetEventStatus");
			FT_GetStatus = (_FT_GetStatus)GETFUNC(m_dll, "FT_GetStatus");
			FT_SetBreakOn = (_FT_SetBreakOn)GETFUNC(m_dll, "FT_SetBreakOn");
			FT_SetBreakOff = (_FT_SetBreakOff)GETFUNC(m_dll, "FT_SetBreakOff");
			FT_SetWaitMask = (_FT_SetWaitMask)GETFUNC(m_dll, "FT_SetWaitMask");
			FT_WaitOnMask = (_FT_WaitOnMask)GETFUNC(m_dll, "FT_WaitOnMask");
			FT_CreateDeviceInfoList = (_FT_CreateDeviceInfoList)GETFUNC(m_dll, "FT_CreateDeviceInfoList");
			FT_GetDeviceInfoList = (_FT_GetDeviceInfoList)GETFUNC(m_dll, "FT_GetDeviceInfoList");
			FT_GetDeviceInfoDetail = (_FT_GetDeviceInfoDetail)GETFUNC(m_dll, "FT_GetDeviceInfoDetail");
			FT_GetDriverVersion = (_FT_GetDriverVersion)GETFUNC(m_dll, "FT_GetDriverVersion");
			FT_GetLibraryVersion = (_FT_GetLibraryVersion)GETFUNC(m_dll, "FT_GetLibraryVersion");
			FT_ResetPort = (_FT_ResetPort)GETFUNC(m_dll, "FT_ResetPort");
			FT_CyclePort = (_FT_CyclePort)GETFUNC(m_dll, "FT_CyclePort");
			FT_GetComPortNumber = (_FT_GetComPortNumber)GETFUNC(m_dll, "FT_GetComPortNumber");
			FT_SetUSBParameters = (_FT_SetUSBParameters)GETFUNC(m_dll, "FT_SetUSBParameters");
			FT_SetLatencyTimer = (_FT_SetLatencyTimer)GETFUNC(m_dll, "FT_SetLatencyTimer");
		}
	}
}


void freeFTDILibrary() {
	if (libraryLoadCounter) {
		libraryLoadCounter--;
		if (libraryLoadCounter == 0) {
#ifdef WIN32
			if (m_dll) FreeLibrary(m_dll);
			m_dll = 0;
#else
			if (m_dll) dlclose(m_dll);
			m_dll = nullptr;
#endif
			FT_Open = nullptr;
			FT_OpenEx = nullptr;
			FT_ListDevices = nullptr;
			FT_Close = nullptr;
			FT_Read = nullptr;
			FT_Write = nullptr;
			FT_IoCtl = nullptr;
			FT_SetBaudRate = nullptr;
			FT_SetDivisor = nullptr;
			FT_SetDataCharacteristics = nullptr;
			FT_SetFlowControl = nullptr;
			FT_ResetDevice = nullptr;
			FT_SetDtr = nullptr;
			FT_ClrDtr = nullptr;
			FT_SetRts = nullptr;
			FT_ClrRts = nullptr;
			FT_GetModemStatus = nullptr;
			FT_SetChars = nullptr;
			FT_Purge = nullptr;
			FT_SetTimeouts = nullptr;
			FT_GetQueueStatus = nullptr;
			FT_SetEventNotification = nullptr;
			FT_GetEventStatus = nullptr;
			FT_GetStatus = nullptr;
			FT_SetBreakOn = nullptr;
			FT_SetBreakOff = nullptr;
			FT_SetWaitMask = nullptr;
			FT_WaitOnMask = nullptr;
			FT_CreateDeviceInfoList = nullptr;
			FT_GetDeviceInfoList = nullptr;
			FT_GetDeviceInfoDetail = nullptr;
			FT_GetDriverVersion = nullptr;
			FT_GetLibraryVersion = nullptr;
			FT_ResetPort = nullptr;
			FT_CyclePort = nullptr;
			FT_GetComPortNumber = nullptr;
			FT_SetUSBParameters = nullptr;
			FT_SetLatencyTimer = nullptr;
		}
	}
}

FTDIInterface::FTDIInterface() {
	initFTDILibrary();
}

FTDIInterface::~FTDIInterface() {
	FT_Close(); 

	freeFTDILibrary();
}

FTDI::FT_STATUS FTDIInterface::FT_Open(int deviceNumber) {
	FT_Close(); 

	if (::FT_Open) {
		FTDI::FT_STATUS status = ::FT_Open(deviceNumber, &m_handle);
		if (status != FTDI::FT_STATUS::FT_OK) m_handle = 0;
		return status;
	}
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_OpenEx(LPVOID pArg1, DWORD Flags) { 
	if (::FT_OpenEx) {
		FTDI::FT_STATUS status = ::FT_OpenEx(pArg1, Flags, &m_handle);
		if (status != FTDI::FT_STATUS::FT_OK) m_handle = 0;
		return status;
	}
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ListDevices(LPVOID pArg1, LPVOID pArg2, DWORD Flags) { 
	if (::FT_ListDevices) return ::FT_ListDevices(pArg1, pArg2, Flags);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_Close() {
	if (::FT_Close) {
		FTDI::FT_STATUS ret;
		if (m_handle) ret = ::FT_Close(m_handle); else ret = FTDI::FT_STATUS::FT_OK;
		m_handle = 0;
		return ret;
	}
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_Read(LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesReturned) { 
	if ((::FT_Read) && (m_handle)) return ::FT_Read(m_handle, lpBuffer, nBufferSize, lpBytesReturned);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_Write(LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesWritten) { 
	if ((::FT_Write) && (m_handle)) return ::FT_Write(m_handle, lpBuffer, nBufferSize, lpBytesWritten);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_IoCtl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) { 
	if ((::FT_IoCtl) && (m_handle)) return ::FT_IoCtl(m_handle, dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf, nOutBufSize, lpBytesReturned, lpOverlapped);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetBaudRate(ULONG BaudRate) { 
	if ((::FT_SetBaudRate) && (m_handle)) return ::FT_SetBaudRate(m_handle, BaudRate);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetDivisor(USHORT Divisor) { 
	if ((::FT_SetDivisor) && (m_handle)) return ::FT_SetDivisor(m_handle, Divisor);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetDataCharacteristics(FT_BITS WordLength, FT_STOP_BITS StopBits, FT_PARITY Parity) {
	if ((::FT_SetDataCharacteristics) && (m_handle)) return ::FT_SetDataCharacteristics(m_handle, (UCHAR)WordLength, (UCHAR)StopBits, (UCHAR)Parity);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetFlowControl(USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar) { 
	if ((::FT_SetFlowControl) && (m_handle)) return ::FT_SetFlowControl(m_handle, FlowControl, XonChar, XoffChar);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ResetDevice() { 
	if ((::FT_ResetDevice) && (m_handle)) return ::FT_ResetDevice(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetDtr() { 
	if ((::FT_SetDtr) && (m_handle)) return ::FT_SetDtr(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ClrDtr() { 
	if ((::FT_ClrDtr) && (m_handle)) return ::FT_ClrDtr(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetRts() { 
	if ((::FT_SetRts) && (m_handle)) return ::FT_SetRts(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ClrRts() { 
	if ((::FT_ClrRts) && (m_handle)) return ::FT_ClrRts(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetModemStatus(ULONG* pModemStatus) { 
	if ((::FT_GetModemStatus) && (m_handle)) return ::FT_GetModemStatus(m_handle, pModemStatus);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetChars(UCHAR EventChar, UCHAR EventCharEnabled, UCHAR ErrorChar, UCHAR ErrorCharEnabled) { 
	if ((::FT_SetChars) && (m_handle)) return ::FT_SetChars(m_handle, EventChar, EventCharEnabled, ErrorChar, ErrorCharEnabled);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_Purge(bool purgeRX, bool purgeTX) { 
	if ((::FT_Purge) && (m_handle)) return ::FT_Purge(m_handle, (purgeRX ? FT_PURGE_RX : 0) | (purgeTX ? FT_PURGE_TX : 0));
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetTimeouts(ULONG ReadTimeout, ULONG WriteTimeout) { 
	if ((::FT_SetTimeouts) && (m_handle)) return ::FT_SetTimeouts(m_handle, ReadTimeout, WriteTimeout);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetQueueStatus(DWORD* dwRxBytes) {
	if ((::FT_GetQueueStatus) && (m_handle)) return ::FT_GetQueueStatus(m_handle, dwRxBytes);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetEventNotification(DWORD Mask, LPVOID Param) { 
	if ((::FT_SetEventNotification) && (m_handle)) return ::FT_SetEventNotification(m_handle, Mask, Param);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetEventStatus(DWORD* dwEventDWord) {
	if ((::FT_GetEventStatus) && (m_handle)) return ::FT_GetEventStatus(m_handle, dwEventDWord);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetStatus(DWORD* dwRxBytes, DWORD* dwTxBytes, DWORD* dwEventDWord) { 
	if ((::FT_GetStatus) && (m_handle)) return ::FT_GetStatus(m_handle, dwRxBytes, dwTxBytes, dwEventDWord);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetBreakOn() { 
	if ((::FT_SetBreakOn) && (m_handle)) return ::FT_SetBreakOn(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetBreakOff() { 
	if ((::FT_SetBreakOff) && (m_handle)) return ::FT_SetBreakOff(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetWaitMask(DWORD Mask) { 
	if ((::FT_SetWaitMask) && (m_handle)) return ::FT_SetWaitMask(m_handle, Mask);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_WaitOnMask(DWORD* Mask) { 
	if ((::FT_WaitOnMask) && (m_handle)) return ::FT_WaitOnMask(m_handle, Mask);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_CreateDeviceInfoList(LPDWORD lpdwNumDevs) {
	if (::FT_CreateDeviceInfoList) return ::FT_CreateDeviceInfoList(lpdwNumDevs);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE* pDest, LPDWORD lpdwNumDevs) {
	if (::FT_GetDeviceInfoList) return ::FT_GetDeviceInfoList(pDest, lpdwNumDevs);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetComPortNumber(FT_HANDLE handle, LPLONG port) {
	if ((::FT_GetComPortNumber) && (handle)) return ::FT_GetComPortNumber(handle, port);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
}

FTDI::FT_STATUS FTDIInterface::FT_GetDeviceInfoDetail(DWORD dwIndex, LPDWORD lpdwFlags, LPDWORD lpdwType, LPDWORD lpdwID, LPDWORD lpdwLocId, PCHAR pcSerialNumber, PCHAR pcDescription, FT_HANDLE* handle) {
	if (::FT_GetDeviceInfoDetail) return ::FT_GetDeviceInfoDetail(dwIndex, lpdwFlags, lpdwType, lpdwID, lpdwLocId, pcSerialNumber, pcDescription, handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetDriverVersion(LPDWORD lpdwDriverVersion) {
	if ((::FT_GetDriverVersion) && (m_handle)) return ::FT_GetDriverVersion(m_handle, lpdwDriverVersion);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_GetLibraryVersion(LPDWORD lpdwDLLVersion) {
	if ((::FT_GetLibraryVersion) && (m_handle)) return ::FT_GetLibraryVersion(lpdwDLLVersion);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_ResetPort() {
	if ((::FT_ResetPort) && (m_handle)) return ::FT_ResetPort(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_CyclePort() {
	if ((::FT_CyclePort) && (m_handle)) return ::FT_CyclePort(m_handle);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
};

FTDI::FT_STATUS FTDIInterface::FT_SetUSBParameters(DWORD dwInTransferSize, DWORD dwOutTransferSize) {
	if ((::FT_SetUSBParameters) && (m_handle)) return ::FT_SetUSBParameters(m_handle, dwInTransferSize, dwOutTransferSize);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
}

FTDI::FT_STATUS FTDIInterface::FT_SetLatencyTimer(UCHAR ucTimer) {
	if ((::FT_SetLatencyTimer) && (m_handle)) return ::FT_SetLatencyTimer(m_handle, ucTimer);
	return FTDI::FT_STATUS::FT_DRIVER_NOT_AVAILABLE;
}

#endif