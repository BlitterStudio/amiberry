 /*
  * UAE - The Un*x Amiga Emulator
  *
  * CIA chip support
  *
  * (c) 1995 Bernd Schmidt
  */

#ifndef UAE_CIA_H
#define UAE_CIA_H

#include "uae/types.h"

extern void CIA_reset (void);
extern void CIA_vsync_prehandler (void);
extern void CIA_hsync_posthandler (bool);
extern void CIA_handler (void);
extern void CIAA_tod_handler (uae_u32);
extern void CIAB_tod_inc_event (uae_u32);
extern void CIAA_tod_inc (int);
extern void CIAB_tod_handler (int);

extern void cia_diskindex (void);

extern void rethink_cias (void);
extern void cia_set_overlay (bool);

extern void rtc_hardreset(void);

#endif /* UAE_CIA_H */
