 /*
  * UAE - The Un*x Amiga Emulator
  *
  * disk support
  *
  * (c) 1995 Bernd Schmidt
  */

#ifndef UAE_DISK_H
#define UAE_DISK_H

#include "uae/types.h"

typedef enum {
	DRV_NONE = -1, DRV_35_DD = 0, DRV_35_HD, DRV_525_SD, DRV_35_DD_ESCOM, DRV_PC_525_ONLY_40, DRV_PC_35_ONLY_80, DRV_PC_525_40_80, DRV_525_DD, DRV_FB
} drive_type;

#define HISTORY_FLOPPY 0
#define HISTORY_CD 1
#define HISTORY_DIR 2
#define HISTORY_HDF 3
#define HISTORY_FS 4
#define HISTORY_TAPE 5
#define HISTORY_GENLOCK_IMAGE 6
#define HISTORY_GENLOCK_VIDEO 7
#define HISTORY_GEO 8
#define HISTORY_STATEFILE 9
#define HISTORY_CONFIGFILE 10
#define HISTORY_MAX 11

struct diskinfo
{
	uae_u8 bootblock[1024];
	bool bb_crc_valid;
	bool image_crc_value;
	uae_u32 imagecrc32;
	uae_u32 bootblockcrc32;
	bool hd;
	bool unreadable;
	int bootblocktype;
	TCHAR diskname[110];
	TCHAR bootblockinfo[256];
	TCHAR bootblockclass[256];
};

#define FLOPPY_RATE_500K 0
#define FLOPPY_RATE_300K 1
#define FLOPPY_RATE_250K 2
#define FLOPPY_RATE_1M 3

struct floppy_reserved
{
	int num;
	struct zfile *img;
	bool wrprot;
	int cyl;
	int cyls;
	int heads;
	int secs;
	int drive_cyls;
	bool disk_changed;
	int rate;
};
void disk_reserved_setinfo(int num, int cyl, int head, int motor);
bool disk_reserved_getinfo(int num, struct floppy_reserved *fr);
void disk_reserved_reset_disk_change(int num);

extern void DISK_init (void);
extern void DISK_free (void);
extern void DISK_select (uae_u8 data);
extern void DISK_select_set (uae_u8 data);
extern uae_u8 DISK_status_ciaa (void);
extern uae_u8 DISK_status_ciab (uae_u8);
extern void disk_eject (int num);
extern int disk_empty (int num);
extern void disk_insert (int num, const TCHAR *name);
extern void disk_insert (int num, const TCHAR *name, bool forcedwriteprotect);
extern void disk_insert_force (int num, const TCHAR *name, bool forcedwriteprotect);
extern void DISK_vsync (void);
extern int DISK_validate_filename (struct uae_prefs *p, const TCHAR *fname, int num, TCHAR *outfname, int leave_open, bool *wrprot, uae_u32 *crc32, struct zfile **zf);
extern void DISK_handler (uae_u32);
extern void DISK_update (int hpos);
extern void DISK_update_adkcon (int hpos, uae_u16 v);
extern void DISK_hsync (void);
extern void DISK_reset (void);
extern int disk_getwriteprotect (struct uae_prefs *p, const TCHAR *name, int num);
extern int disk_setwriteprotect (struct uae_prefs *p, int num, const TCHAR *name, bool writeprotected);
extern bool disk_creatediskfile (struct uae_prefs *p, const TCHAR *name, int type, drive_type adftype, int hd, const TCHAR *disk_name, bool ffs, bool bootable, struct zfile *copyfrom);
extern void dumpdisk (const TCHAR*);
extern int DISK_history_add (const TCHAR *name, int idx, int type, int donotcheck);
extern TCHAR *DISK_history_get (int idx, int type);
int DISK_examine_image (struct uae_prefs *p, int num, struct diskinfo *di, bool deepcheck, TCHAR *info);
extern TCHAR *DISK_get_saveimagepath(const TCHAR *name, int type);
extern void DISK_reinsert (int num);
extern int disk_prevnext (int drive, int dir);
extern int disk_prevnext_name (TCHAR *img, int dir);
extern void DISK_get_path_text(struct uae_prefs *p, int n, TCHAR *text);

extern bool gui_ask_disk(int drv, TCHAR*);

extern void DSKLEN (uae_u16 v, int hpos);
extern uae_u16 DSKBYTR (int hpos);
extern void DSKSYNC (int, uae_u16);
extern void DSKPTL (uae_u16);
extern void DSKPTH (uae_u16);
extern void DSKDAT (uae_u16);
extern uae_u16 DSKDATR (void);
extern uae_u16 disk_dmal (void);
extern uaecptr disk_getpt (void);
extern int disk_fifostatus (void);

extern int disk_debug_logging;
extern int disk_debug_mode;
extern int disk_debug_track;
#define DISK_DEBUG_DMA_READ 1
#define DISK_DEBUG_DMA_WRITE 2
#define DISK_DEBUG_PIO 4

#define MAX_PREVIOUS_IMAGES 50

#ifdef FLOPPYBRIDGE
bool floppybridge_has(void);
bool DISK_isfloppybridge(struct uae_prefs*, int);
void floppybridge_init(struct uae_prefs *p);
void floppybridge_modified(int);
void floppybridge_reload_profiles(void);
void floppybridge_set_config(const char*);
extern bool floppybridge_available;
#endif

#endif /* UAE_DISK_H */
