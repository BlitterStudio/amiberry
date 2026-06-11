#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

// target Windows XP for 32 bit, the latest Windows version for 64 bit compilation
#ifndef _WIN64
#include <winsdkver.h>
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif

#include <SDKDDKVer.h>
