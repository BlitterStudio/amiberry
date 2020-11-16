/*
* UAE - The Un*x Amiga Emulator
*
* UAE Native Interface (UNI)
*
* Copyright 2013-2014 Frode Solheim
*/

#ifndef UAE_UAENATIVE_H
#define UAE_UAENATIVE_H

#ifdef WITH_UAENATIVE

#define UNI_IMPORT
#include "uni_common.h"

#define UNI_FLAG_ASYNCHRONOUS 1
#define UNI_FLAG_COMPAT 2
#define UNI_FLAG_NAMED_FUNCTION 4

uae_u32 uaenative_open_library(TrapContext *context, int flags);
uae_u32 uaenative_get_function(TrapContext *context, int flags);
uae_u32 uaenative_call_function(TrapContext *context, int flags);
uae_u32 uaenative_close_library(TrapContext *context, int flags);

void *uaenative_get_uaevar(void);

void uaenative_install ();
uaecptr uaenative_startup(TrapContext*, uaecptr resaddr);

/* This function must return a list of directories to look for native
 * libraries in. The returned list must be NULL-terminated, and must not
 * be de-allocated. */
const TCHAR **uaenative_get_library_dirs(void);

#endif /* WITH_UAENATIVE */

#endif /* UAE_UAENATIVE_H */
