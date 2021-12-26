 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Input recording and playback
  *
  * Copyright 2010 Toni Wilen
  */

#ifndef UAE_INPUTRECORD_H
#define UAE_INPUTRECORD_H

#include "uae/types.h"

extern int inputrecord_debug;

//#define INPREC_JOYPORT 1
//#define INPREC_JOYBUTTON 2
//#define INPREC_KEY 3
#define INPREC_DISKINSERT 4
#define INPREC_DISKREMOVE 5
//#define INPREC_VSYNC 6
//#define INPREC_CIAVSYNC 7
#define INPREC_EVENT 8
#define INPREC_CIADEBUG 0x61
#define INPREC_DEBUG 0x62
#define INPREC_DEBUG2 0x63
#define INPREC_STOP 0x7d
#define INPREC_END 0x7e
#define INPREC_QUIT 0x7f

#define INPREC_RECORD_START 1
#define INPREC_RECORD_NORMAL 2
#define INPREC_RECORD_RERECORD 3
#define INPREC_RECORD_PLAYING 4
#define INPREC_PLAY_NORMAL 1
#define INPREC_PLAY_RERECORD 2

extern int input_record, input_play;
extern void inprec_close (bool);
extern void inprec_save (const TCHAR*, const TCHAR*);
extern int inprec_open (const TCHAR*, const TCHAR*);
extern bool inprec_prepare_record (const TCHAR*);
extern void inprec_playtorecord (void);
extern void inprec_startup (void);

extern bool inprec_playevent (int *nr, int *state, int *max, int *autofire);
extern void inprec_playdiskchange (void);
extern void inprec_recordevent (int nr, int state, int max, int autofire);
extern void inprec_recorddiskchange (int nr, const TCHAR *fname, bool writeprotected);

extern void inprec_recorddebug (uae_u32);
extern void inprec_playdebug (uae_u32);
extern void inprec_recorddebug_cpu (int);
extern void inprec_playdebug_cpu (int);
extern void inprec_recorddebug_cia (uae_u32, uae_u32, uae_u32);
extern void inprec_playdebug_cia (uae_u32, uae_u32, uae_u32);

extern int inprec_getposition (void);
extern void inprec_setposition (int offset, int replaycounter);
extern bool inprec_realtime (void);
extern void inprec_getstatus (TCHAR*);

#endif /* UAE_INPUTRECORD_H */
