#ifndef CAPSLIB_H
#define CAPSLIB_H

#undef LIB_USER
#ifdef CAPS_USER
#define LIB_USER
#endif
#include "ComLib.h"

ExtSub int32_t __cdecl CAPSInit();
ExtSub int32_t __cdecl CAPSExit();
ExtSub int32_t __cdecl CAPSAddImage();
ExtSub int32_t __cdecl CAPSRemImage(int32_t id);
ExtSub int32_t __cdecl CAPSLockImage(int32_t id, const char *name);
ExtSub int32_t __cdecl CAPSLockImageMemory(int32_t id, const uint8_t *buffer, uint32_t length, uint32_t flag);
ExtSub int32_t __cdecl CAPSUnlockImage(int32_t id);
ExtSub int32_t __cdecl CAPSLoadImage(int32_t id, uint32_t flag);
ExtSub int32_t __cdecl CAPSGetImageInfo(PCAPSIMAGEINFO pi, int32_t id);
ExtSub int32_t __cdecl CAPSLockTrack(void *ptrackinfo, int32_t id, uint32_t cylinder, uint32_t head, uint32_t flag);
ExtSub int32_t __cdecl CAPSUnlockTrack(int32_t id, uint32_t cylinder, uint32_t head);
ExtSub int32_t __cdecl CAPSUnlockAllTracks(int32_t id);
ExtSub const char * __cdecl CAPSGetPlatformName(uint32_t pid);
ExtSub int32_t __cdecl CAPSGetVersionInfo(void *pversioninfo, uint32_t flag);
ExtSub uint32_t __cdecl CAPSFdcGetInfo(int32_t iid, PCAPSFDC pc, int32_t ext);
ExtSub int32_t __cdecl CAPSFdcInit(PCAPSFDC pc);
ExtSub void __cdecl CAPSFdcReset(PCAPSFDC pc);
ExtSub int32_t __cdecl CAPSFdcResetState(PCAPSFDC pc, int32_t reset_state);
ExtSub void __cdecl CAPSFdcEmulate(PCAPSFDC pc, uint32_t cyclecnt);
ExtSub uint32_t __cdecl CAPSFdcRead(PCAPSFDC pc, uint32_t address);
ExtSub void __cdecl CAPSFdcWrite(PCAPSFDC pc, uint32_t address, uint32_t data);
ExtSub int32_t __cdecl CAPSFdcInvalidateTrack(PCAPSFDC pc, int32_t drive);
ExtSub int32_t __cdecl CAPSFormatDataToMFM(void *pformattrack, uint32_t flag);
ExtSub int32_t __cdecl CAPSGetInfo(void *pinfo, int32_t id, uint32_t cylinder, uint32_t head, uint32_t inftype, uint32_t infid);
ExtSub int32_t __cdecl CAPSSetRevolution(int32_t id, uint32_t value);
ExtSub int32_t __cdecl CAPSGetImageType(const char *name);
ExtSub int32_t __cdecl CAPSGetImageTypeMemory(const uint8_t *buffer, uint32_t length);
ExtSub int32_t __cdecl CAPSGetDebugRequest();

#endif
