 /*
  * UAE - The Un*x Amiga Emulator
  *
  * a SCSI device
  *
  * (c) 1995 Bernd Schmidt (hardfile.c)
  * (c) 1999 Patrick Ohly
  * (c) 2001-2005 Toni Wilen
  */

uaecptr scsidev_startup (uaecptr resaddr);
void scsidev_install (void);
void scsidev_reset (void);
void scsidev_start_threads (void);
int scsi_do_disk_change (int unitnum, int insert, int *pollmode);
int scsi_do_disk_device_change (void);
uae_u32 scsi_get_cd_drive_mask (void);
uae_u32 scsi_get_cd_drive_media_mask (void);

extern int log_scsi;

#ifdef _WIN32
#define UAESCSI_CDEMU 0
#define UAESCSI_SPTI 1
#define UAESCSI_SPTISCAN 2
#define UAESCSI_LAST 2
#endif
