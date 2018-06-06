 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Interface to the Tcl/Tk GUI
  *
  * Copyright 1996 Bernd Schmidt
  */

#ifndef UAE_GUI_H
#define UAE_GUI_H

#include "uae/types.h"

extern int gui_init (void);
extern int gui_update (void);
extern void gui_exit (void);
#ifdef WIN32
extern void gui_led (int, int, int);
#else
STATIC_INLINE void gui_led (int led, int on, int brightness) { }
#endif
extern void gui_filename (int, const TCHAR *);
extern void gui_flicker_led (int, int, int);
extern void gui_display (int shortcut);

extern bool no_gui;

#define LED_CD_ACTIVE 1
#define LED_CD_ACTIVE2 2
#define LED_CD_AUDIO 4

#define LED_POWER 0
#define LED_DF0 1
#define LED_DF1 2
#define LED_DF2 3
#define LED_DF3 4
#define LED_HD 5
#define LED_CD 6
#define LED_FPS 7
#define LED_CPU 8
#define LED_SND 9
#define LED_MD 10
#define LED_NET 11
#define LED_MAX 12

struct gui_info
{
    bool drive_motor[4];    /* motor on off */
    uae_u8 drive_track[4];    /* rw-head track */
    bool drive_writing[4];  /* drive is writing */
    bool drive_disabled[4];	/* drive is disabled */
    bool powerled;          /* state of power led */
	uae_u8 powerled_brightness;	/* 0 to 255 */
    uae_s8 drive_side;				/* floppy side */
    uae_s8 hd;			          /* harddrive */
    uae_s8 cd;			          /* CD */
	uae_s8 md;					/* CD32 or CDTV internal storage */
	uae_s8 net;					/* network */
    int cpu_halted;
    int fps, idle;
	int fps_color;
    int sndbuf, sndbuf_status;
	bool sndbuf_avail;
    TCHAR df[4][256];		    /* inserted image */
    uae_u32 crc32[4];		    /* crc32 of image */
};
#define NUM_LEDS (LED_MAX)
#define VISIBLE_LEDS (LED_MAX - 1)

extern struct gui_info gui_data;

void notify_user (int msg);
void notify_user_parms (int msg, const TCHAR *parms, ...);
int translate_message (int msg, TCHAR *out);

typedef enum {
	NUMSG_NEEDEXT2, // 0
	NUMSG_NOROM,
	NUMSG_NOROMKEY,
	NUMSG_KSROMCRCERROR,
	NUMSG_KSROMREADERROR,
	NUMSG_NOEXTROM, // 5
	NUMSG_MODRIP_NOTFOUND,
	NUMSG_MODRIP_FINISHED,
	NUMSG_MODRIP_SAVE,
	NUMSG_KS68EC020,
	NUMSG_KS68020, // 10
	NUMSG_KS68030,
	NUMSG_ROMNEED,
	NUMSG_EXPROMNEED,
	NUMSG_NOZLIB,
	NUMSG_STATEHD, // 15
	NUMSG_NOCAPS,
	NUMSG_OLDCAPS,
	NUMSG_KICKREP,
	NUMSG_KICKREPNO,
	NUMSG_KS68030PLUS, // 20
	NUMSG_NO_PPC,
	NUMSG_UAEBOOTROM_PPC,
	NUMSG_NOMEMORY,
	NUMSG_LAST
} notify_user_msg;

#endif /* UAE_GUI_H */
