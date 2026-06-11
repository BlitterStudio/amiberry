// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef POSIXFIX_H

#include <stdint.h>     //needs to be here for "Windows compatible" defs
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <iostream>

typedef uint32_t DWORD;   // DWORD = unsigned 32 bit value
typedef uint16_t WORD;    // WORD = unsigned 16 bit value
typedef uint8_t BYTE;     // BYTE = unsigned 8 bit value


#define __cdecl
#define DllImport
#undef USE_WINUSB

typedef void *LPGUID;

#define MAX_PATH   FILENAME_MAX

#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define _access access

inline int _mkdir(const char *f) {
	return mkdir(f, 0755);
}

// Define this to enable libusb driver
#define USE_LIBUSB
// Ensure the binary is compatible with older glibc versions, if linked against
// a newer glibc, under Linux only.
#ifdef HAVE_FEATURES_H
#include <features.h>
#ifdef __GLIBC__
#if (__WORDSIZE == 64)
#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 14))
__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");
#endif // GLIBC version check
#else // Not 64-bit
#endif // 32/64-bit check
#endif // __GLIBC__
#endif // HAVE_FEATURES_H
// Autoconf with firmware dir
//#include "config.h"

// add missing STD features
#include "STDFix.h"

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

// function is defined posixfix.cpp
void GetLocalTime(LPSYSTEMTIME lpSystemTime);


// Windows compatible definitions for other platforms
#include "GUIDDefinition.h"
#include "BMPDefinition.h"

// expose some math constants
#define _USE_MATH_DEFINES

// standard C/C++ includes & fix
#include <cinttypes>
#include <inttypes.h>

#endif
