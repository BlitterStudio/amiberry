 /*
  * UAE - The Un*x Amiga Emulator
  *
  * disk support
  *
  * (c) 1995 Bernd Schmidt
  */

typedef enum { DRV_NONE = -1, DRV_35_DD = 0, DRV_35_HD, DRV_525_SD, DRV_35_DD_ESCOM } drive_type;

extern void DISK_init (void);
extern void DISK_free (void);
extern void DISK_select (uae_u8 data);
extern uae_u8 DISK_status (void);
extern void disk_eject (int num);
extern int disk_empty (int num);
extern void disk_insert (int num, const char *name);
extern void DISK_check_change (void);
extern struct zfile *DISK_validate_filename (const char *, int, int *, uae_u32 *);
extern void DISK_handler (uae_u32);
extern void DISK_update (int hpos);
extern void DISK_hsync (int hpos);
extern void DISK_reset (void);
extern int disk_getwriteprotect (const char *name);
extern int disk_setwriteprotect (int num, const char *name, int protect);
extern void disk_creatediskfile (char *name, int type, drive_type adftype, char *disk_name);
extern int DISK_history_add (const char *name, int idx);
extern char *DISK_history_get (int idx);
int DISK_examine_image (struct uae_prefs *p, int num, uae_u32 *crc32);
extern char *DISK_get_saveimagepath (const char *name);
extern void DISK_reinsert (int num);

extern void DSKLEN (uae_u16 v, int hpos);
extern uae_u16 DSKBYTR (int hpos);
#define DSKDAT(uae_u16)
extern void DSKSYNC (int, uae_u16);
extern void DSKPTL (uae_u16);
extern void DSKPTH (uae_u16);

extern int disk_debug_logging;
extern int disk_debug_mode;
extern int disk_debug_track;
#define DISK_DEBUG_DMA_READ 1
#define DISK_DEBUG_DMA_WRITE 2
#define DISK_DEBUG_PIO 4

#define MAX_PREVIOUS_FLOPPIES 99
