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
extern void CIA_hsync_prehandler (void);
extern void CIA_hsync_posthandler (bool, bool);
extern void CIA_handler (void);
extern void CIAA_tod_inc (int);
extern void CIAB_tod_handler (int);

extern void diskindex_handler (void);
extern void cia_parallelack (void);
extern void cia_diskindex (void);

extern void dumpcia (void);
extern void rethink_cias (void);
extern int resetwarning_do (int);
extern void cia_set_overlay (bool);
void cia_heartbeat (void);

extern int parallel_direct_write_data (uae_u8, uae_u8);
extern int parallel_direct_read_data (uae_u8*);
extern int parallel_direct_write_status (uae_u8, uae_u8);
extern int parallel_direct_read_status (uae_u8*);

extern void rtc_hardreset (void);

extern void keyboard_connected(bool);

#endif /* UAE_CIA_H */
