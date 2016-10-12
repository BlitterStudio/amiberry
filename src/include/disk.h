 /*
  * UAE - The Un*x Amiga Emulator
  *
  * disk support
  *
  * (c) 1995 Bernd Schmidt
  */

typedef enum { DRV_NONE = -1, DRV_35_DD = 0, DRV_35_HD, DRV_525_SD, DRV_35_DD_ESCOM } drive_type;

#define HISTORY_FLOPPY 0
#define HISTORY_CD 1

struct diskinfo
{
	uae_u8 bootblock[1024];
	bool bb_crc_valid;
	uae_u32 crc32;
	bool hd;
	bool unreadable;
	int bootblocktype;
	TCHAR diskname[110];
};

extern void DISK_init (void);
extern void DISK_free (void);
extern void DISK_select (uae_u8 data);
extern void DISK_select_set (uae_u8 data);
extern uae_u8 DISK_status (void);
extern void disk_eject (int num);
extern int disk_empty (int num);
extern void disk_insert (int num, const TCHAR *name);
extern void disk_insert (int num, const TCHAR *name, bool forcedwriteprotect);
extern void disk_insert_force (int num, const TCHAR *name, bool forcedwriteprotect);
extern void DISK_vsync (void);
extern int DISK_validate_filename (struct uae_prefs *p, const TCHAR *fname, int leave_open, bool *wrprot, uae_u32 *crc32, struct zfile **zf);
extern void DISK_handler (uae_u32);
extern void DISK_motordelay_func(uae_u32 v);
extern void DISK_update (int hpos);
extern void DISK_update_adkcon (int hpos, uae_u16 v);
extern void DISK_hsync (void);
extern void DISK_reset (void);
extern int disk_getwriteprotect (struct uae_prefs *p, const TCHAR *name);
extern int disk_setwriteprotect (struct uae_prefs *p, int num, const TCHAR *name, bool writeprotected);
extern bool disk_creatediskfile (const TCHAR *name, int type, drive_type adftype, const TCHAR *disk_name, bool ffs, bool bootable, struct zfile *copyfrom);
extern int DISK_history_add (const TCHAR *name, int idx, int type, int donotcheck);
extern TCHAR *DISK_history_get (int idx, int type);
int DISK_examine_image (struct uae_prefs *p, int num, struct diskinfo *di);
extern TCHAR *DISK_get_saveimagepath (const TCHAR *name);
extern void DISK_reinsert (int num);

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

#define DISK_DEBUG_DMA_READ 1
#define DISK_DEBUG_DMA_WRITE 2
#define DISK_DEBUG_PIO 4

#define MAX_PREVIOUS_FLOPPIES 99

