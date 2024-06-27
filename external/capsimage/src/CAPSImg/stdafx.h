// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// OS compatiblity
#ifdef _WIN32
#include "targetver.h"
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS         // avoid _s warnings.

// Windows specific
#ifdef _WIN32
#include <windows.h>
#endif

// detect memory leaks in debug builds
#define _CRTDBG_MAP_ALLOC

// standard C/C++ includes
#include <stdlib.h>
#ifdef _WIN32
#include <crtdbg.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <vector>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#endif
#include <dirent.h>

#include <stddef.h>			// offsetof
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>			// localtime

#ifndef _WIN32
#include <unistd.h>
#define MAX_PATH ( 260 )
#ifndef __cdecl
#if defined(__GNUC__) && !defined(__LP64__)
#define __cdecl __attribute__((cdecl))
#else
#define __cdecl
#endif
#endif

#define _lrotl(x,n) (((x) << (n)) | ((x) >> (sizeof(x)*8-(n))))
#define _lrotr(x,n) (((x) >> (n)) | ((x) << (sizeof(x)*8-(n))))
typedef const char *LPCSTR;
typedef const char *LPCTSTR;
#endif


#define INTEL
#define MAX_FILENAMELEN (MAX_PATH*2)

// external definitions
#include "CommonTypes.h"

// Core components
#include "BaseFile.h"
#include "DiskFile.h"
#include "MemoryFile.h"
#include "CRC.h"
#include "BitBuffer.h"

// IPF library public definitions
#include "CapsLibAll.h"

// CODECs
#include "DiskEncoding.h"
#include "CapsDefinitions.h"
#include "CTRawCodec.h"

// file support
#include "CapsFile.h"
#include "DiskImage.h"
#include "CapsLoader.h"
#include "CapsImageStd.h"
#include "CapsImage.h"
#include "StreamImage.h"
#include "StreamCueImage.h"
#include "DiskImageFactory.h"

// Device access
#include "C2Comm.h"

// system
#include "CapsCore.h"
#include "CapsFDCEmulator.h"
#include "CapsFormatMFM.h"


#ifndef _WIN32
#define _access access
#ifndef __MINGW32__
#define _mkdir(x) mkdir(x,0)
#else
#define _mkdir(x) mkdir(x)
#endif
#define d_namlen d_reclen
#define __assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#define min(x, y) (((x) < (y)) ? (x) : (y))

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
#endif
