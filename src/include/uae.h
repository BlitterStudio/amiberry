 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Prototypes for main.c
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef UAE_UAE_H
#define UAE_UAE_H

#include "uae/types.h"

extern void do_start_program (void);
extern void start_program (void);
extern void leave_program (void);
extern void real_main (int, TCHAR **);
extern void sleep_millis (int ms);

#define UAE_QUIT 1
#define UAE_RESET 2
#define UAE_RESET_KEYBOARD 3
#define UAE_RESET_HARD 4

extern void uae_reset (int, int);
extern void uae_quit (void);
extern void uae_restart (int, const TCHAR*);
extern void target_reset (void);
extern void target_addtorecent (const TCHAR*, int);
extern void target_run (void);
extern void target_quit (void);
extern void target_restart (void);
extern void target_startup_msg(TCHAR *title, TCHAR *msg);
extern void stripslashes (TCHAR *p);
extern void fixtrailing (TCHAR *p);
extern void getpathpart (TCHAR *outpath, int size, const TCHAR *inpath);
extern void getfilepart (TCHAR *out, int size, const TCHAR *path);
extern uae_u32 getlocaltime (void);

extern int quit_program;

extern TCHAR start_path_data[256];

/* This structure is used to define menus. The val field can hold key
 * shortcuts, or one of these special codes:
 *   -4: deleted entry, not displayed, not selectable, but does count in
 *       select value
 *   -3: end of table
 *   -2: line that is displayed, but not selectable
 *   -1: line that is selectable, but has no keyboard shortcut
 *    0: Menu title
 */
struct bstring {
    const TCHAR *data;
    int val;
};

extern void fetch_saveimagepath (TCHAR*, int, int);
extern void fetch_datapath (TCHAR *out, int size);
extern void fetch_rompath (TCHAR *out, int size);
#define uaerand() rand()

#endif /* UAE_UAE_H */
