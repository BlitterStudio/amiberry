// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// default to little-endian platform, unless specified otherwise
#if !defined(__BYTE_ORDER__) || (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define LITTLE_ENDIAN
#endif

#ifdef _WIN32

// OS compatibility
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// detect memory leaks in debug builds
#define _CRTDBG_MAP_ALLOC

// don't define min-max macros
#define NOMINMAX

// Windows specific
#include <windows.h>
#include <crtdbg.h>
#endif

// standard C/C++ includes
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <unordered_map>
#include <string_view>
#include <string>
#include <string.h> //memcpy & co
#include <limits>
#include <vector>
#include <algorithm>
#include <memory>
#ifdef __APPLE__
#include <sys/syslimits.h>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
#define off64_t off_t
#define fopen64 fopen
#define fseeko64 fseeko
#define ftello64 ftello
#endif // __APPLE__

#ifdef ANDROID
#if __ANDROID_API__ < 24
#define fopen64 fopen
#define fseeko64 fseeko
#define ftello64 ftello
#endif
#endif

#ifndef _WIN32

#define __cdecl
#define DllImport
typedef uint16_t WORD;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;
extern "C" void GetLocalTime(LPSYSTEMTIME lpSystemTime);

#endif // !_WIN32



// add missing 64 bit stream functions to stdio
// MinGW already provides fopen64/fseeko64/ftello64 and off64_t natively
#if defined(_WIN32) && !defined(__MINGW32__)
#include <stream64.h>
#endif

// external definitions
#include "CommonTypes.h"

// IPF library public definitions
#include "CapsLibAll.h"

// Core components
#include "BaseFile.h"
#include "DiskFile.h"
#include "MemoryFile.h"
#include "CRC.h"
#include "BitBuffer.h"

// CODECs
#include "DiskEncoding.h"
#include "CapsDefinitions.h"
#include "CTRawCodec.h"

// file support
#include "DiskImage.h"
#include "CapsLoader.h"
#include "CapsImageStd.h"
#include "CapsImage.h"
#include "StreamImage.h"
#include "StreamCueImage.h"
#include "DiskImageFactory.h"

// Device access
#define C2_APP_ACCESS
#include "C2Comm.h"

// system
#include "CapsCore.h"
#include "CapsFDCEmulator.h"
#include "CapsFormatMFM.h"
