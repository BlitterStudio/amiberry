#ifndef CAPSLIB_H
#define CAPSLIB_H

#undef LIB_USER
#ifdef CAPS_USER
#define LIB_USER
#endif
#include "ComLib.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

ExtSub SDWORD __cdecl CAPSInit();
ExtSub SDWORD __cdecl CAPSExit();
ExtSub SDWORD __cdecl CAPSAddImage();
ExtSub SDWORD __cdecl CAPSRemImage(SDWORD id);
ExtSub SDWORD __cdecl CAPSLockImage(SDWORD id, PCHAR name);
ExtSub SDWORD __cdecl CAPSLockImageMemory(SDWORD id, PUBYTE buffer, UDWORD length, UDWORD flag);
ExtSub SDWORD __cdecl CAPSUnlockImage(SDWORD id);
ExtSub SDWORD __cdecl CAPSLoadImage(SDWORD id, UDWORD flag);
ExtSub SDWORD __cdecl CAPSGetImageInfo(PCAPSIMAGEINFO pi, SDWORD id);
ExtSub SDWORD __cdecl CAPSLockTrack(PVOID ptrackinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD flag);
ExtSub SDWORD __cdecl CAPSUnlockTrack(SDWORD id, UDWORD cylinder, UDWORD head);
ExtSub SDWORD __cdecl CAPSUnlockAllTracks(SDWORD id);
ExtSub PCHAR  __cdecl CAPSGetPlatformName(UDWORD pid);
ExtSub SDWORD __cdecl CAPSGetVersionInfo(PVOID pversioninfo, UDWORD flag);
ExtSub UDWORD __cdecl CAPSFdcGetInfo(SDWORD iid, PCAPSFDC pc, SDWORD ext);
ExtSub SDWORD __cdecl CAPSFdcInit(PCAPSFDC pc);
ExtSub void   __cdecl CAPSFdcReset(PCAPSFDC pc);
ExtSub void   __cdecl CAPSFdcEmulate(PCAPSFDC pc, UDWORD cyclecnt);
ExtSub UDWORD __cdecl CAPSFdcRead(PCAPSFDC pc, UDWORD address);
ExtSub void   __cdecl CAPSFdcWrite(PCAPSFDC pc, UDWORD address, UDWORD data);
ExtSub SDWORD __cdecl CAPSFdcInvalidateTrack(PCAPSFDC pc, SDWORD drive);
ExtSub SDWORD __cdecl CAPSFormatDataToMFM(PVOID pformattrack, UDWORD flag);
ExtSub SDWORD __cdecl CAPSGetInfo(PVOID pinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD inftype, UDWORD infid);
ExtSub SDWORD __cdecl CAPSSetRevolution(SDWORD id, UDWORD value);
ExtSub SDWORD __cdecl CAPSGetImageType(PCHAR name);
ExtSub SDWORD __cdecl CAPSGetImageTypeMemory(PUBYTE buffer, UDWORD length);
ExtSub SDWORD __cdecl CAPSGetDebugRequest();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
