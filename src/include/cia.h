 /*
  * UAE - The Un*x Amiga Emulator
  *
  * CIA chip support
  *
  * (c) 1995 Bernd Schmidt
  */

extern void CIA_reset (void);
extern void CIA_vsync_handler (void);
extern void CIA_hsync_handler (void);
extern void CIA_handler (void);

extern void diskindex_handler (void);
extern void cia_diskindex (void);

extern void rethink_cias (void);

extern void rtc_hardreset(void);
