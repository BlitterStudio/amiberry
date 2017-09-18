 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Call (some) Amiga Exec functions outside the main UAE thread
  * and without stack magic.
  *
  * Copyright 1999 Patrick Ohly
  *
  * Uses the EXTER interrupt that is setup in filesys.c
  * and some of it needs thread support.
  */

#ifndef UAE_NATIVE2AMIGA_H
#define UAE_NATIVE2AMIGA_H

#include "uae/types.h"
#include "native2amiga_api.h"
#include "traps.h"

/*
 * The following functions are shortcuts for calling
 * the exec.library function with CallLib (), so they
 * are only available in a trap function. This trap
 * function has to be setup with deftrap2() and
 * TRAPFLAG_EXTRA_STACK and stack magic is required.
 */
uaecptr uae_AllocMem (TrapContext *context, uae_u32 size, uae_u32 flags, uaecptr sysbase);
void uae_FreeMem (TrapContext *context, uaecptr memory, uae_u32 size, uaecptr sysbase);


/*
 * to be called when setting up the hardware
 */
void native2amiga_install (void);

void native2amiga_reset (void);

/*
 * to be called when the Amiga boots, i.e. by filesys_diagentry()
 */
void native2amiga_startup (void);

/**** internal stuff ****/
#ifdef SUPPORT_THREADS
/* This pipe is filled by Signal() with pairs of
 * (uae_u32)0/(uaecptr)task/(uae_u32)signal_set,
 * by PutMsg() with (uae_u32)1/(uaecptr)port/(uaecptr)msg and by
 * ReplyMsg() with (uae_u32)2/(uaecptr)msg.
 * It's emptied via exter_int_helper by the EXTER interrupt. */
extern smp_comm_pipe native2amiga_pending;
#endif

STATIC_INLINE void do_uae_int_requested (void)
{
  atomic_or(&uae_int_requested, 1);
}

#endif /* UAE_NATIVE2AMIGA_H */
