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

extern int gui_init(void);
extern void gui_update(void);
extern void gui_exit(void);
extern void gui_led(int, int, int);
extern void gui_handle_events(void);
extern void gui_filename(int, const TCHAR*);
extern void gui_fps(int fps, int lines, bool lace, int idle, int color);
extern void gui_changesettings(void);
extern void gui_lock(void);
extern void gui_unlock(void);
extern void gui_flicker_led(int, int, int);
extern void gui_disk_image_change(int, const TCHAR*, bool writeprotected);
extern unsigned int gui_ledstate;
extern void gui_display(int shortcut);

//extern void gui_gameport_button_change (int port, int button, int onoff);
//extern void gui_gameport_axis_change (int port, int axis, int state, int max);

extern bool no_gui, quit_to_gui;

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
#define LED_LINES 8
#define LED_CPU 9
#define LED_SND 10
#define LED_MD 11
#define LED_NET 12
#define LED_CAPS 13
#ifdef AMIBERRY
#define LED_TEMP 14 // Temperature sensor LED
#define LED_MAX 15
#else
#define LED_MAX 14
#endif


struct gui_info_drive {
	bool drive_motor;		/* motor on off */
	uae_u8 drive_track;		/* rw-head track */
	bool drive_writing;		/* drive is writing */
	bool drive_disabled;	/* drive is disabled */
	TCHAR df[256];			/* inserted image */
	uae_u32 crc32;			/* crc32 of image */
	bool floppy_protected;	/* image is write protected */
	bool floppy_inserted;   /* disk inserted */
};

struct gui_info
{
	bool powerled;				/* state of power led */
	uae_u8 powerled_brightness;	/* 0 to 255 */
	bool capslock;				/* caps lock state if KB MCU mode */
	uae_s8 drive_side;			/* floppy side */
	uae_s8 hd;					/* harddrive */
	uae_s8 cd;					/* CD */
	uae_s8 md;					/* CD32 or CDTV internal storage */
	uae_s8 net;					/* network */
	int cpu_halted;
	int fps, lines, lace, idle;
	int fps_color;
	int sndbuf, sndbuf_status;
	bool sndbuf_avail;
	struct gui_info_drive drives[4];
#ifdef AMIBERRY
	int temperature;
#endif
};
#define NUM_LEDS (LED_MAX)
#define VISIBLE_LEDS (LED_MAX - 1)

extern struct gui_info gui_data;

/* Functions to be called when prefs are changed by non-gui code.  */
extern void gui_update_gfx(void);

void notify_user(int msg);
void notify_user_parms(int msg, const TCHAR* parms, ...);
int translate_message(int msg, TCHAR* out);

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
	NUMSG_INPUT_NONE,
	NUMSG_LAST
} notify_user_msg;

#ifdef USE_GPIOD
#include <gpiod.h>
extern struct gpiod_line* lineRed;    // Red LED
extern struct gpiod_line* lineGreen;  // Green LED
extern struct gpiod_line* lineYellow; // Yellow LED
#endif
#endif /* UAE_GUI_H */
