#ifndef UAE_BLKDEV_H
#define UAE_BLKDEV_H

#include "uae/types.h"

#define DEVICE_SCSI_BUFSIZE (65536 - 1024)

#define SCSI_UNIT_DISABLED -1
#define SCSI_UNIT_DEFAULT 0
#define SCSI_UNIT_IMAGE 1
#define SCSI_UNIT_IOCTL 2

//#define device_debug write_log
#define device_debug

#define	INQ_DASD	0x00		/* Direct-access device (disk) */
#define	INQ_SEQD	0x01		/* Sequential-access device (tape) */
#define	INQ_PRTD	0x02 		/* Printer device */
#define	INQ_PROCD	0x03 		/* Processor device */
#define	INQ_OPTD	0x04		/* Write once device (optical disk) */
#define	INQ_WORM	0x04		/* Write once device (optical disk) */
#define	INQ_ROMD	0x05		/* CD-ROM device */
#define	INQ_SCAN	0x06		/* Scanner device */
#define	INQ_OMEM	0x07		/* Optical Memory device */
#define	INQ_JUKE	0x08		/* Medium Changer device (jukebox) */
#define	INQ_COMM	0x09		/* Communications device */
#define	INQ_IT8_1	0x0A		/* IT8 */
#define	INQ_IT8_2	0x0B		/* IT8 */
#define	INQ_STARR	0x0C		/* Storage array device */
#define	INQ_ENCL	0x0D		/* Enclosure services device */
#define	INQ_NODEV	0x1F		/* Unknown or no device */
#define	INQ_NOTPR	0x1F		/* Logical unit not present (SCSI-1) */

#define MAX_TOC_ENTRIES 103
struct cd_toc
{
	uae_u8 adr, control;
	uae_u8 tno;
	uae_u8 point;
	uae_u8 track;
	int address; // LSN
	int paddress; // LSN
	uae_u8 zero;
	uae_u8 crc[2];
};
struct cd_toc_head
{
	int first_track, first_track_offset;
	int last_track, last_track_offset;
	int firstaddress; // LSN
	int lastaddress; // LSN
	int tracks;
	int points;
	struct cd_toc toc[MAX_TOC_ENTRIES];
};

#define SUB_ENTRY_SIZE 12
#define SUB_CHANNEL_SIZE 96
#define SUBQ_SIZE (4 + 12)

#define AUDIO_STATUS_NOT_SUPPORTED  0x00
#define AUDIO_STATUS_IN_PROGRESS    0x11
#define AUDIO_STATUS_PAUSED         0x12
#define AUDIO_STATUS_PLAY_COMPLETE  0x13
#define AUDIO_STATUS_PLAY_ERROR     0x14
#define AUDIO_STATUS_NO_STATUS      0x15

struct device_info {
	bool open;
  int type;
  int media_inserted;
	int audio_playing;
  int removable;
  int write_protected;
  int cylinders;
  int trackspercylinder;
  int sectorspertrack;
  int bytespersector;
  int bus, target, lun;
  int unitnum;
  TCHAR label[MAX_DPATH];
	TCHAR mediapath[MAX_DPATH];
	TCHAR vendorid[10];
	TCHAR productid[18];
	TCHAR revision[6];
	const TCHAR *backend;
	struct cd_toc_head toc;
	TCHAR system_id[33];
	TCHAR volume_id[33];
};

struct amigascsi
{
  uae_u8 *data;
  uae_s32 len;
  uae_u8 cmd[16];
  uae_s32 cmd_len;
  uae_u8 flags;
  uae_u8 sensedata[256];
  uae_u16 sense_len;
  uae_u16 cmdactual;
  uae_u8 status;
  uae_u16 actual;
  uae_u16 sactual;
};

typedef int (*check_bus_func)(int flags);
typedef int (*open_bus_func)(int flags);
typedef void (*close_bus_func)(void);
typedef int (*open_device_func)(int, const TCHAR*, int);
typedef void (*close_device_func)(int);
typedef struct device_info* (*info_device_func)(int, struct device_info*, int, int);
typedef uae_u8* (*execscsicmd_out_func)(int, uae_u8*, int);
typedef uae_u8* (*execscsicmd_in_func)(int, uae_u8*, int, int*);
typedef int (*execscsicmd_direct_func)(int, struct amigascsi*);

typedef void (*play_subchannel_callback)(uae_u8*, int);
typedef int (*play_status_callback)(int, int);

typedef int (*pause_func)(int, int);
typedef int (*stop_func)(int);
typedef int (*play_func)(int, int, int, int, play_status_callback, play_subchannel_callback);
typedef uae_u32 (*volume_func)(int, uae_u16, uae_u16);
typedef int (*qcode_func)(int, uae_u8*, int, bool);
typedef int (*toc_func)(int, struct cd_toc_head*);
typedef int (*read_func)(int, uae_u8*, int, int);
typedef int (*rawread_func)(int, uae_u8*, int, int, int, uae_u32);
typedef int (*write_func)(int, uae_u8*, int, int);
typedef int (*isatapi_func)(int);
typedef int (*ismedia_func)(int, int);
typedef int (*scsiemu_func)(int, uae_u8*);

struct device_functions {
	const TCHAR *name;
	open_bus_func openbus;
	close_bus_func closebus;
	open_device_func opendev;
	close_device_func closedev;
	info_device_func info;
	execscsicmd_out_func exec_out;
	execscsicmd_in_func exec_in;
	execscsicmd_direct_func exec_direct;

	pause_func pause;
	stop_func stop;
	play_func play;
	volume_func volume;
	qcode_func qcode;
	toc_func toc;
	read_func read;
	rawread_func rawread;
	write_func write;

	isatapi_func isatapi;
	ismedia_func ismedia;

	scsiemu_func scsiemu;

};

extern int device_func_init(int flags);
extern void device_func_reset(void);
extern int sys_command_open (int unitnum);
extern void sys_command_close (int unitnum);
extern int sys_command_isopen (int unitnum);
extern struct device_info *sys_command_info (int unitnum, struct device_info *di, int);
extern int sys_command_cd_pause (int unitnum, int paused);
extern void sys_command_cd_stop (int unitnum);
extern int sys_command_cd_play (int unitnum, int startlsn, int endlsn, int);
extern int sys_command_cd_play (int unitnum, int startlsn, int endlsn, int scan, play_status_callback statusfunc, play_subchannel_callback subfunc);
extern uae_u32 sys_command_cd_volume (int unitnum, uae_u16 volume_left, uae_u16 volume_right);
extern int sys_command_cd_qcode (int unitnum, uae_u8*, int lsn, bool all);
extern int sys_command_cd_toc (int unitnum, struct cd_toc_head*);
extern int sys_command_cd_read (int unitnum, uae_u8 *data, int block, int size);
extern int sys_command_cd_rawread (int unitnum, uae_u8 *data, int sector, int size, int sectorsize);
int sys_command_cd_rawread (int unitnum, uae_u8 *data, int sector, int size, int sectorsize, uae_u8 sectortype, uae_u8 scsicmd9, uae_u8 subs);
extern int sys_command_read (int unitnum, uae_u8 *data, int block, int size);
extern int sys_command_write (int unitnum, uae_u8 *data, int block, int size);
extern int sys_command_scsi_direct_native (int unitnum, int type, struct amigascsi *as);
extern int sys_command_scsi_direct(TrapContext *ctx, int unitnum, int type, uaecptr request);
extern int sys_command_ismedia (int unitnum, int quick);
extern struct device_info *sys_command_info_session (int unitnum, struct device_info *di, int, int);
extern bool blkdev_get_info (struct uae_prefs *p, int unitnum, struct device_info *di);

extern void scsi_atapi_fixup_pre (uae_u8 *scsi_cmd, int *len, uae_u8 **data, int *datalen, int *parm);
extern void scsi_atapi_fixup_post (uae_u8 *scsi_cmd, int len, uae_u8 *olddata, uae_u8 *data, int *datalen, int parm);

extern void scsi_log_before (uae_u8 *cdb, int cdblen, uae_u8 *data, int datalen);
extern void scsi_log_after (uae_u8 *data, int datalen, uae_u8 *sense, int senselen);

extern int scsi_cd_emulate (int unitnum, uae_u8 *cmdbuf, int scsi_cmd_len,
	uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len, bool atapi);

extern void blkdev_vsync (void);
extern void restore_blkdev_start(void);

extern int msf2lsn (int msf);
extern int lsn2msf (int lsn);
extern uae_u8 frombcd (uae_u8 v);
extern uae_u8 tobcd (uae_u8 v);
extern int fromlongbcd (uae_u8 *p);
extern void tolongbcd (uae_u8 *p, int v);

extern void blkdev_default_prefs (struct uae_prefs *p);
extern void blkdev_fix_prefs (struct uae_prefs *p);
extern int isaudiotrack (struct cd_toc_head*, int block);
extern int isdatatrack (struct cd_toc_head*, int block);
void sub_to_interleaved (const uae_u8 *s, uae_u8 *d);
void sub_to_deinterleaved (const uae_u8 *s, uae_u8 *d);

enum cd_standard_unit { CD_STANDARD_UNIT_DEFAULT, CD_STANDARD_UNIT_AUDIO, CD_STANDARD_UNIT_CDTV, CD_STANDARD_UNIT_CD32 };

extern int get_standard_cd_unit (enum cd_standard_unit csu);
extern void close_standard_cd_unit (int);
extern void blkdev_cd_change (int unitnum, const TCHAR *name);

extern void blkdev_entergui (void);
extern void blkdev_exitgui (void);

extern struct device_functions devicefunc_cdimage;

#endif /* UAE_BLKDEV_H */
