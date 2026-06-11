// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#include <stdint.h>     //needs to be here for "Windows compatible" defs
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <unistd.h>
#define MAX_PATH PATH_MAX
#define _access access
#define MAX_FILENAMELEN (MAX_PATH*2)

// 64 Bit LFS is available since 10.5 in fopen, so we nned to define
#define off64_t off_t
#define fopen64 fopen
#define fseeko64 fseeko
#define ftello64 ftello


#define __cdecl
#define DllImport

typedef void *LPGUID;

#define MAX_PATH                PATH_MAX

#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define _access access
inline int _mkdir(const char *f) {
	return mkdir(f, 0755);
}

// add missing STD features
#include "STDFix.h"

// Windows compatible definitions for other platforms
#include "GUIDDefinition.h"
#include "BMPDefinition.h"

// expose some math constants
#define _USE_MATH_DEFINES

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



// standard C/C++ includes & fix
#include <iostream>
#include <cinttypes>
#ifndef PRIX32
//some OS X SDKs do not define PRIX32, strange....
#define PRIX32        "X"
#endif
#include <inttypes.h>

// OSX driver
#undef USE_LIBUSB
#define USE_DARWINUSB

//#define DEBUG_REL

//no serial
//#define DARWIN_NO_SERIAL
//always use serial
//#define DARWIN_ALWAYS_SERIAL

// build Release binaries with Debug Output
#ifdef DEBUG_REL
#define DARWIN_DEBUG
#endif

//DEBUG is set by Xcode when Debugs builds are made
#if defined (DARWIN_DEBUG) || (DEBUG)
#warning Debugging active
#define DEBUG_PRINTF(x) do{printf("%s::%s: ", __FILE__, __func__);printf x;}while(0)
#else
#define DEBUG_PRINTF(x)
#endif

#ifdef DARWIN_NO_SERIAL
#warning Serial/Modem support disabled
#undef DARWIN_ALWAYS_SERIAL
#endif
#ifdef DARWIN_ALWAYS_SERIAL
#warning Will always use serial port for firmware upload
#undef DARWIN_NO_SERIAL
#endif

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>

