 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Drive Click emulation stuff
  *
  * Copyright 2004 James Bagg, Toni Wilen
  */

#ifndef UAE_DRIVECLICK_H
#define UAE_DRIVECLICK_H

#include "uae/types.h"

#define CLICK_TRACKS 84

struct drvsample {
    int len;
    int pos;
    uae_s16 *p;
    int indexes[CLICK_TRACKS];
    int lengths[CLICK_TRACKS];
};

#define DS_CLICK 0
#define DS_SPIN 1
#define DS_SPINND 2
#define DS_START 3
#define DS_SNATCH 4
#define DS_END 5

extern void driveclick_click (int drive, int startOffset);
extern void driveclick_motor (int drive, int running);
extern void driveclick_insert (int drive, int eject);
extern void driveclick_init (void);
extern void driveclick_free (void);
extern void driveclick_reset (void);
extern void driveclick_mix (uae_s16*, int, int);
extern int driveclick_loadresource (struct drvsample*, int);
extern void driveclick_check_prefs (void);
extern uae_s16 *decodewav (uae_u8 *s, int *len);

#define DS_BUILD_IN_SOUNDS 1
#define DS_NAME_CLICK _T("drive_click_")
#define DS_NAME_SPIN _T("drive_spin_")
#define DS_NAME_SPIN_ND _T("drive_spinnd_")
#define DS_NAME_START _T("drive_start_")
#define DS_NAME_SNATCH _T("drive_snatch_")

extern int driveclick_fdrawcmd_open (int);
extern void driveclick_fdrawcmd_close (int);
extern void driveclick_fdrawcmd_detect (void);
extern void driveclick_fdrawcmd_seek (int, int);
extern void driveclick_fdrawcmd_motor (int, int);
extern void driveclick_fdrawcmd_vsync (void);
extern int driveclick_pcdrivemask, driveclick_pcdrivenum;

#endif /* UAE_DRIVECLICK_H */
