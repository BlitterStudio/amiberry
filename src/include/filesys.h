 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Unix file system handler for AmigaDOS
  *
  * Copyright 1997 Bernd Schmidt
  */

#ifndef FILESYS_H
#define FILESYS_H

struct hardfiledata {
  uae_u64 virtsize; // virtual size
  uae_u64 physsize; // physical size (dynamic disk)
  uae_u64 offset;
	struct uaedev_config_info ci;
  struct hardfilehandle *handle;
  int handle_valid;
  int dangerous;
  int flags;
  uae_u8 *cache;
  int cache_valid;
  uae_u64 cache_offset;
  TCHAR vendor_id[8 + 1];
  TCHAR product_id[16 + 1];
  TCHAR product_rev[4 + 1];
  TCHAR device_name[256];
  /* geometry from possible RDSK block */
  int rdbcylinders;
  int rdbsectors;
  int rdbheads;
  uae_u8 *virtual_rdb;
  uae_u64 virtual_size;
  int unitnum;
  int byteswap;
  int adide;
  int hfd_type;

  int drive_empty;
  TCHAR *emptyname;

	struct uaedev_config_info delayedci;
	int reinsertdelay;
	bool isreinsert;
};

struct hd_hardfiledata {
    struct hardfiledata hfd;
    uae_u64 size;
    int cyls;
    int heads;
    int secspertrack;
    int cyls_def;
    int secspertrack_def;
    int heads_def;
    int ansi_version;
};

#define HD_CONTROLLER_UAE 0
#define HD_CONTROLLER_IDE0 1
#define HD_CONTROLLER_IDE1 2
#define HD_CONTROLLER_IDE2 3
#define HD_CONTROLLER_IDE3 4
#define HD_CONTROLLER_SCSI0 5
#define HD_CONTROLLER_SCSI1 6
#define HD_CONTROLLER_SCSI2 7
#define HD_CONTROLLER_SCSI3 8
#define HD_CONTROLLER_SCSI4 9
#define HD_CONTROLLER_SCSI5 10
#define HD_CONTROLLER_SCSI6 11
#define HD_CONTROLLER_PCMCIA_SRAM 12
#define HD_CONTROLLER_PCMCIA_IDE 13

#define FILESYS_VIRTUAL 0
#define FILESYS_HARDFILE 1
#define FILESYS_HARDFILE_RDB 2
#define FILESYS_HARDDRIVE 3
#define FILESYS_CD 4
#define FILESYS_TAPE 5

#define MAX_FILESYSTEM_UNITS 30

struct uaedev_mount_info;
extern struct uaedev_mount_info options_mountinfo;

extern struct hardfiledata *get_hardfile_data (int nr);
#define FILESYS_MAX_BLOCKSIZE 2048
extern int hdf_open (struct hardfiledata *hfd);
extern int hdf_open (struct hardfiledata *hfd, const TCHAR *altname);
extern void hdf_close (struct hardfiledata *hfd);
extern int hdf_read_rdb (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
extern int hdf_read (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
extern int hdf_write (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
extern int get_native_path(uae_u32 lock, TCHAR *out);
extern void hardfile_do_disk_change (struct uaedev_config_data *uci, bool insert);
extern void hardfile_send_disk_change (struct hardfiledata *hfd, bool insert);
extern int hardfile_media_change (struct hardfiledata *hfd, struct uaedev_config_info *ci, bool inserted, bool timer);

void hdf_hd_close(struct hd_hardfiledata *hfd);
int hdf_hd_open(struct hd_hardfiledata *hfd);

extern int hdf_open_target (struct hardfiledata *hfd, const TCHAR *name);
extern void hdf_close_target (struct hardfiledata *hfd);
extern int hdf_read_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
extern int hdf_write_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len);
extern void getchsgeometry (uae_u64 size, int *pcyl, int *phead, int *psectorspertrack);
extern void getchsgeometry_hdf (struct hardfiledata *hfd, uae_u64 size, int *pcyl, int *phead, int *psectorspertrack);
extern void getchspgeometry (uae_u64 total, int *pcyl, int *phead, int *psectorspertrack, bool idegeometry);

#endif /* MEMORY_H */
