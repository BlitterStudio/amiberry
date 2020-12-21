/*
* UAE - The Un*x Amiga Emulator
*
* UAE Native Interface (UNI)
*
* Copyright 2013-2014 Frode Solheim
*/

#ifndef UAE_UNI_COMMON_H
#define UAE_UNI_COMMON_H

#define UNI_VERSION 1

#ifndef UNI_MIN_VERSION
// MIN_UNI_VERSION decides which callback functions are declared available.
// The default, unless overridden is to require the current UNI version.
#define UNI_MIN_VERSION UNI_VERSION
#endif

#ifdef _WIN32
#ifdef UNI_IMPORT
#define UNIAPI __declspec(dllimport)
#else
#define UNIAPI __declspec(dllexport)
#endif
#define UNICALL __cdecl
#else
#define UNIAPI
#define UNICALL
#endif

#define UNI_ERROR_NOT_ENABLED            0x70000001
#define UNI_ERROR_INVALID_LIBRARY        0x70000002
#define UNI_ERROR_INVALID_FUNCTION       0x70000002
#define UNI_ERROR_FUNCTION_NOT_FOUND     0x70000003
#define UNI_ERROR_LIBRARY_NOT_FOUND      0x70000004
#define UNI_ERROR_INVALID_FLAGS          0x70000005
#define UNI_ERROR_COULD_NOT_OPEN_LIBRARY 0x70000006
#define UNI_ERROR_ILLEGAL_LIBRARY_NAME   0x70000007
#define UNI_ERROR_LIBRARY_TOO_OLD        0x70000008
#define UNI_ERROR_INIT_FAILED            0x70000009
#define UNI_ERROR_INTERFACE_TOO_OLD      0x7000000a

// these errors are only returned from the Amiga-side uni wrappers
#define UNI_ERROR_AMIGALIB_NOT_OPEN      0x71000000

#include <stdint.h>

typedef int8_t   uni_byte;
typedef uint8_t  uni_ubyte;
typedef int16_t  uni_word;
typedef uint16_t uni_uword;
typedef int32_t  uni_long;
typedef uint32_t uni_ulong;

// deprecated typedefs
typedef int8_t   uni_char;
typedef uint8_t  uni_uchar;
typedef int16_t  uni_short;
typedef uint16_t uni_ushort;

typedef int (UNICALL *uni_version_function)();
typedef void * (UNICALL *uni_resolve_function)(uni_ulong ptr);
typedef const char * (UNICALL *uni_uae_version_function)(void);

struct uni {
    uni_long d1;
    uni_long d2;
    uni_long d3;
    uni_long d4;
    uni_long d5;
    uni_long d6;
    uni_long d7;
    uni_long a1;
    uni_long a2;
    uni_long a3;
    uni_long a4;
    uni_long a5;
    uni_long a7;

#if UNI_VERSION >= 2
    // add new struct entries for version 2 here
#endif

#ifdef CPUEMU_0  // UAE
    uni_long result;
    uni_long error;
    uni_ulong function;
    uni_ulong library;
    void *native_function;
    void *uaevar_compat;
    int flags;
    uaecptr task;
#endif
};

#endif /* UAE_UNI_COMMON_H */
