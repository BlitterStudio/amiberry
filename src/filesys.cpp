/*
* UAE - The Un*x Amiga Emulator
*
* Unix file system handler for AmigaDOS
*
* Copyright 1996 Ed Hanway
* Copyright 1996, 1997 Bernd Schmidt
*
* Version 0.4: 970308
*
* Based on example code (c) 1988 The Software Distillery
* and published in Transactor for the Amiga, Volume 2, Issues 2-5.
* (May - August 1989)
*
* Known limitations:
* Does not support several (useless) 2.0+ packet types.
* May not return the correct error code in some cases.
* Does not check for sane values passed by AmigaDOS.  May crash the emulation
* if passed garbage values.
* Could do tighter checks on malloc return values.
* Will probably fail spectacularly in some cases if the filesystem is
* modified at the same time by another process while UAE is running.
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "threaddep/thread.h"
#include "options.h"
#include "traps.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "fsusage.h"
#include "native2amiga.h"
#include "scsidev.h"
#include "uaeserial.h"
#include "fsdb.h"
#include "zfile.h"
#include "zarchive.h"
#include "gui.h"
#include "gayle.h"
#include "idecontrollers.h"
#include "savestate.h"
#ifdef A2091
#include "a2091.h"
#endif
#ifdef NCR
#include "ncr_scsi.h"
#endif
#include "cdtv.h"
#include "sana2.h"
#include "bsdsocket.h"
#include "uaeresource.h"
#include "inputdevice.h"
#include "clipboard.h"
#include "consolehook.h"
#include "blkdev.h"
#include "isofs_api.h"
#include "scsi.h"
#ifdef WITH_UAENATIVE
#include "uaenative.h"
#endif
#include "tabletlibrary.h"
#include "cia.h"
#include "newcpu.h"
#include "picasso96.h"
#include "cpuboard.h"
#include "rommgr.h"
#include "debug.h"
#include "debugmem.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

#define TRACING_ENABLED 1
int log_filesys = 0;

#define TRAPMD 1

#if TRACING_ENABLED
#if 0
#define TRACE(x) if (log_filesys > 0 && (unit->volflags & MYVOLUMEINFO_CDFS)) { write_log x; }
#else
#define TRACE(x) if (log_filesys > 0) { write_log x; }
#endif
#define TRACEI(x) if (log_filesys > 0) { write_log x; }
#define TRACE2(x) if (log_filesys >= 2) { write_log x; }
#define TRACE3(x) if (log_filesys >= 3) { write_log x; }
#define DUMPLOCK(c,u,x) dumplock(c,u,x)
#else
#define TRACE(x)
#define DUMPLOCK(c,u,x)
#define TRACE2(x)
#define TRACE3(x)
#endif

#define KS12_BOOT_HACK 1

#define UNIT_LED(unit) ((unit)->ui.unit_type == UNIT_CDFS ? LED_CD : LED_HD)

#define SE2_MIN_FILE_LENGTH 32

struct ShellExecute2
{
	uae_u32 size;
	char *file, *parms, *currentdir;
	char *fileparms;
	uae_u32 stack;
	uae_s32 priority;
	uae_u32 flags;
	uae_u32 id;
	uae_u32 binsize;
	uae_u8 *bin;
	char *tmpout;
	uaecptr aoutbuf;
	char *binname;

	int aoutlen;
	uaecptr process;
	uaecptr buffer;
	uae_u32 exitcode;
	shellexecute2_callback cb;
	char *outbuf;
	int outlen;
	void *userdata;
	int state;
};

#define SHELLEXEC_MAX 16
static struct ShellExecute2 shellexecute2[SHELLEXEC_MAX];
static volatile int shellexecute2_queued;
static uae_u32 filesys_shellexecute2_process(int mode, TrapContext *ctx);
static void filesys_shellexecute2_run_queue(void);
static void shellexecute2_free(struct ShellExecute2 *se2);

static int bootrom_header;

static uae_u32 dlg (uae_u32 a)
{
	return (dbg (a + 0) << 24) | (dbg (a + 1) << 16) | (dbg (a + 2) << 8) | (dbg (a + 3) << 0);
}

static void aino_test (a_inode *aino)
{
#ifdef AINO_DEBUG
	a_inode *aino2 = aino, *aino3;
	for (;;) {
		if (!aino || !aino->next)
			return;
		if ((aino->checksum1 ^ aino->checksum2) != 0xaaaa5555) {
			write_log (_T("PANIC: corrupted or freed but used aino detected!"), aino);
		}
		aino3 = aino;
		aino = aino->next;
		if (aino->prev != aino3) {
			write_log (_T("PANIC: corrupted aino linking!\n"));
			break;
		}
		if (aino == aino2) break;
	}
#endif
}

static void aino_test_init (a_inode *aino)
{
#ifdef AINO_DEBUG
	aino->checksum1 = (uae_u32)aino;
	aino->checksum2 = aino->checksum1 ^ 0xaaaa5555;
#endif
}

#define UAEFS_VERSION "UAE fs 0.6"

uaecptr filesys_initcode, filesys_initcode_ptr, filesys_initcode_real;
static uaecptr bootrom_start;
static uae_u32 fsdevname, fshandlername, filesys_configdev;
static uae_u32 cdfs_devname, cdfs_handlername;
static uaecptr afterdos_name, afterdos_id, afterdos_initcode;
static uaecptr keymaphook_name, keymaphook_id, keymaphook_initcode;
static uaecptr shell_execute_data, shell_execute_process;
static int filesys_in_interrupt;
static uae_u32 mountertask;
static int automountunit = -1;
static int autocreatedunit;
static int cd_unit_offset, cd_unit_number;
static uaecptr ROM_filesys_doio, ROM_filesys_doio_original;
static uaecptr ROM_filesys_putmsg, ROM_filesys_putmsg_original;
static uaecptr ROM_filesys_putmsg_return;
static uaecptr ROM_filesys_hack_remove;
static smp_comm_pipe shellexecute_pipe;
static uae_u32 segtrack_mode = 0;
static bool gayle_ide_in_use;

#define FS_STARTUP 0
#define FS_GO_DOWN 1

#define DEVNAMES_PER_HDF 32

#define UNIT_FILESYSTEM 0
#define UNIT_CDFS 1

typedef struct {
	int unit_type;
	int open; // >0 start as filesystem, <0 = allocated but do not start
	TCHAR *devname; /* device name, e.g. UAE0: */
	uaecptr devname_amiga;
	uaecptr startup;
	uaecptr devicenode;
	uaecptr parmpacket;
	TCHAR *volname; /* volume name, e.g. CDROM, WORK, etc. */
	int volflags; /* volume flags, readonly, stream uaefsdb support */
	TCHAR *rootdir; /* root native directory/hdf. empty drive if invalid path */
	TCHAR *rootdirdiff; /* "diff" file/directory */
	bool readonly; /* disallow write access? */
	bool locked; /* action write protect */
	bool unknown_media; /* ID_UNREADABLE_DISK */
	int bootpri; /* boot priority. -128 = no autoboot, -129 = no mount */
	int devno;
	bool wasisempty; /* if true, this unit was created empty */
	int canremove; /* if >0, this unit can be safely ejected and remounted */
	bool configureddrive; /* if true, this is drive that was manually configured */
	bool inject_icons; /* inject icons if directory filesystem */

	struct hardfiledata hf;
	struct zvolume *zarchive;

	/* Threading stuff */
	smp_comm_pipe *volatile unit_pipe, *volatile back_pipe;
	uae_thread_id tid;
	struct _unit *self;
	/* Reset handling */
	uae_sem_t reset_sync_sem;
	volatile int reset_state;

	/* RDB stuff */
	uaecptr rdb_devname_amiga[DEVNAMES_PER_HDF];
	int rdb_lowcyl;
	int rdb_highcyl;
	int rdb_cylblocks;
	uae_u8 *rdb_filesysstore;
	int rdb_filesyssize;
	TCHAR *filesysdir;
	/* filesystem seglist */
	uaecptr filesysseg;
	uae_u32 rdb_dostype;

	/* CDFS */
	bool cd_open;
	int cddevno;
	void *cdfs_superblock;

} UnitInfo;

struct uaedev_mount_info {
	UnitInfo ui[MAX_FILESYSTEM_UNITS];
};

static struct uaedev_mount_info mountinfo;


/* minimal AmigaDOS definitions */

/* field offsets in DosPacket */
#define dp_Type 8
#define dp_Res1 12
#define dp_Res2 16
#define dp_Arg1 20
#define dp_Arg2 24
#define dp_Arg3 28
#define dp_Arg4 32
#define dp_Arg5 36

#define DP64_INIT       -3L

#define dp64_Type 8
#define dp64_Res0 12
#define dp64_Res2 16
#define dp64_Res1 24
#define dp64_Arg1 32
#define dp64_Arg2 40
#define dp64_Arg3 48
#define dp64_Arg4 52
#define dp64_Arg5 56

#define dp_Max 60

/* result codes */
#define DOS_TRUE ((uae_u32)-1L)
#define DOS_FALSE (0L)

/* DirEntryTypes */
#define ST_PIPEFILE -5
#define ST_LINKFILE -4
#define ST_FILE -3
#define ST_ROOT 1
#define ST_USERDIR 2
#define ST_SOFTLINK 3
#define ST_LINKDIR 4

#if 1
#define MAXFILESIZE32 (0xffffffff)
#else
/* technically correct but most native
 * filesystems don't enforce it
 */
#define MAXFILESIZE32 (0x7fffffff)
#endif
#define MAXFILESIZE32_2G (0x7fffffff)

 /* Passed as type to Lock() */
#define SHARED_LOCK		-2  /* File is readable by others */
#define ACCESS_READ		-2  /* Synonym */
#define EXCLUSIVE_LOCK	-1  /* No other access allowed  */
#define ACCESS_WRITE	-1  /* Synonym */

/* packet types */
#define ACTION_CURRENT_VOLUME	 7
#define ACTION_LOCATE_OBJECT	 8
#define ACTION_RENAME_DISK		 9
#define ACTION_FREE_LOCK		15
#define ACTION_DELETE_OBJECT	16
#define ACTION_RENAME_OBJECT	17
#define ACTION_MORE_CACHE		18
#define ACTION_COPY_DIR			19
#define ACTION_SET_PROTECT		21
#define ACTION_CREATE_DIR		22
#define ACTION_EXAMINE_OBJECT	23
#define ACTION_EXAMINE_NEXT		24
#define ACTION_DISK_INFO		25
#define ACTION_INFO				26
#define ACTION_FLUSH			27
#define ACTION_SET_COMMENT		28
#define ACTION_PARENT			29
#define ACTION_SET_DATE			34
#define ACTION_FIND_WRITE		1004
#define ACTION_FIND_INPUT		1005
#define ACTION_FIND_OUTPUT		1006
#define ACTION_END				1007
#define ACTION_SEEK				1008
#define ACTION_WRITE_PROTECT	1023
#define ACTION_IS_FILESYSTEM	1027
#define ACTION_READ				 'R'
#define ACTION_WRITE			 'W'

/* 2.0+ packet types */
#define ACTION_INHIBIT			  31
#define ACTION_SET_FILE_SIZE	1022
#define ACTION_LOCK_RECORD		2008
#define ACTION_FREE_RECORD		2009
#define ACTION_SAME_LOCK		  40
#define ACTION_CHANGE_MODE		1028
#define ACTION_FH_FROM_LOCK		1026
#define ACTION_COPY_DIR_FH		1030
#define ACTION_PARENT_FH		1031
#define ACTION_EXAMINE_ALL		1033
#define ACTION_EXAMINE_FH		1034
#define ACTION_EXAMINE_ALL_END	1035

#define ACTION_FORMAT			1020
#define ACTION_IS_FILESYSTEM	1027
#define ACTION_ADD_NOTIFY		4097
#define ACTION_REMOVE_NOTIFY	4098

#define ACTION_READ_LINK		1024

/* OS4 64-bit filesize packets */
#define ACTION_FILESYSTEM_ATTR         3005
#define ACTION_CHANGE_FILE_POSITION64  8001
#define ACTION_GET_FILE_POSITION64     8002
#define ACTION_CHANGE_FILE_SIZE64      8003
#define ACTION_GET_FILE_SIZE64         8004

/* MOS 64-bit filesize packets */
#define ACTION_SEEK64			26400
#define ACTION_SET_FILE_SIZE64	26401
#define ACTION_LOCK_RECORD64	26402
#define ACTION_FREE_RECORD64	26403
#define ACTION_QUERY_ATTR		26407
#define ACTION_EXAMINE_OBJECT64	26408
#define ACTION_EXAMINE_NEXT64	26409
#define ACTION_EXAMINE_FH64		26410


/* not supported */
#define ACTION_MAKE_LINK		1021

#define DISK_TYPE_DOS 0x444f5300 /* DOS\0 */
#define DISK_TYPE_DOS_FFS 0x444f5301 /* DOS\1 */
#define CDFS_DOSTYPE 0x43440000 /* CDxx */

typedef struct _dpacket {
	uaecptr packet_addr;
	uae_u8 *packet_data;
	uae_u8 packet_array[dp_Max];
	bool need_flush;
} dpacket;

typedef struct {
	uae_u32 uniq;
	/* The directory we're going through.  */
	a_inode *aino;
	/* The file we're going to look up next.  */
	a_inode *curr_file;
} ExamineKey;

struct lockrecord
{
	struct lockrecord *next;
	dpacket *packet;
	uae_u64 pos;
	uae_u64 len;
	uae_u32 mode;
	uae_u32 timeout;
	uae_u32 msg;
};

typedef struct key {
	struct key *next;
	a_inode *aino;
	uae_u32 uniq;
	struct fs_filehandle *fd;
	uae_u64 file_pos;
	int dosmode;
	int createmode;
	int notifyactive;
	struct lockrecord *record;
} Key;

typedef struct notify {
	struct notify *next;
	uaecptr notifyrequest;
	TCHAR *fullname;
	TCHAR *partname;
} Notify;

typedef struct exallkey {
	uae_u32 id;
	struct fs_dirhandle *dirhandle;
	TCHAR *fn;
	uaecptr control;
} ExAllKey;

/* Since ACTION_EXAMINE_NEXT is so braindamaged, we have to keep
* some of these around
*/

#define EXKEYS 128
#define EXALLKEYS 100
#define MAX_AINO_HASH 128
#define NOTIFY_HASH_SIZE 127

/* handler state info */

typedef struct _unit {
	struct _unit *next;

	/* Amiga stuff */
	uaecptr dosbase;
	/* volume points to our IO board, always 1:1 mapping */
	uaecptr volume;
	uaecptr port;	/* Our port */
	uaecptr locklist;

	/* Native stuff */
	uae_s32 unit;	/* unit number */
	UnitInfo ui;	/* unit startup info */
	TCHAR tmpbuf3[256];

	/* Dummy message processing */
	uaecptr dummy_message;
	volatile unsigned int cmds_sent;
	volatile unsigned int cmds_complete;
	volatile unsigned int cmds_acked;

	/* ExKeys */
	ExamineKey examine_keys[EXKEYS];
	int next_exkey;
	unsigned int total_locked_ainos;

	/* ExAll */
	ExAllKey exalls[EXALLKEYS];
	int exallid;

	/* Keys */
	struct key *keys;

	struct lockrecord *waitingrecords;

	a_inode rootnode;
	unsigned int aino_cache_size;
	a_inode *aino_hash[MAX_AINO_HASH];
	unsigned int nr_cache_hits;
	unsigned int nr_cache_lookups;

	struct notify *notifyhash[NOTIFY_HASH_SIZE];

	int volflags;
	uae_u32 lockkey;
	bool inhibited;
	bool canremovable;
	/* increase when media is changed.
	 * used to detect if cached aino is valid
	 */
	int mountcount;
	int mount_changed;
	void *cdfs_superblock;

	TCHAR *mount_volume;
	TCHAR *mount_rootdir;
	bool mount_readonly;
	int mount_flags;

	int reinsertdelay;
	TCHAR *newvolume;
	TCHAR *newrootdir;
	bool newreadonly;
	int newflags;

} Unit;

int nr_units (void)
{
	int cnt = 0;
	for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (mountinfo.ui[i].open > 0)
			cnt++;
	}
	return cnt;
}

int nr_directory_units (struct uae_prefs *p)
{
	int cnt = 0;
	if (p) {
		for (int i = 0; i < p->mountitems; i++) {
			if (p->mountconfig[i].ci.controller_type == HD_CONTROLLER_TYPE_UAE)
				cnt++;
		}
	} else {
		for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
			if (mountinfo.ui[i].open > 0 && mountinfo.ui[i].hf.ci.controller_type == HD_CONTROLLER_TYPE_UAE)
				cnt++;
		}
	}
	return cnt;
}

static int is_virtual (int unit_no)
{
	int t = is_hardfile (unit_no);
	return t == FILESYS_VIRTUAL || t == FILESYS_CD;
}

int is_hardfile (int unit_no)
{
	if (mountinfo.ui[unit_no].volname || mountinfo.ui[unit_no].wasisempty || mountinfo.ui[unit_no].unknown_media) {
		if (unit_no >= cd_unit_offset && unit_no < cd_unit_offset + cd_unit_number)
			return FILESYS_CD;
		return FILESYS_VIRTUAL;
	}
	if (mountinfo.ui[unit_no].hf.ci.sectors == 0) {
		if (mountinfo.ui[unit_no].hf.flags & 1)
			return FILESYS_HARDDRIVE;
		return FILESYS_HARDFILE_RDB;
	}
	return FILESYS_HARDFILE;
}

static void close_filesys_unit (UnitInfo *uip)
{
	if (!uip->open)
		return;
	if (uip->hf.handle_valid)
		hdf_close (&uip->hf);
	if (uip->volname != 0)
		xfree (uip->volname);
	if (uip->devname != 0)
		xfree (uip->devname);
	if (uip->rootdir != 0)
		xfree (uip->rootdir);
	if (uip->unit_pipe)
	{
		destroy_comm_pipe(uip->unit_pipe);
		xfree (uip->unit_pipe);
	}
	if (uip->back_pipe)
	{
		destroy_comm_pipe(uip->back_pipe);
		xfree (uip->back_pipe);
	}
	if (uip->cd_open) {
		sys_command_close (uip->cddevno);
		isofs_unmount (uip->cdfs_superblock);
	}

	uip->unit_pipe = 0;
	uip->back_pipe = 0;

	uip->hf.handle_valid = 0;
	uip->volname = 0;
	uip->devname = 0;
	uip->rootdir = 0;
	uip->open = 0;
	uip->cd_open = 0;
}

static uaedev_config_data *getuci (struct uaedev_config_data *uci, int nr)
{
	return &uci[nr];
}


static UnitInfo *getuip (struct uae_prefs *p, int index)
{
	if (index < 0)
		return NULL;
	index = p->mountconfig[index].configoffset;
	if (index < 0)
		return NULL;
	return &mountinfo.ui[index];
}
static int getuindex (struct uae_prefs *p, int index)
{
	if (index < 0)
		return -1;
	return p->mountconfig[index].unitnum;
}

int get_filesys_unitconfig (struct uae_prefs *p, int index, struct mountedinfo *mi)
{
	UnitInfo *ui = getuip (p, index);
	struct uaedev_config_data *uci = &p->mountconfig[index];
	UnitInfo uitmp;
	TCHAR filepath[MAX_DPATH];

	memset (mi, 0, sizeof (struct mountedinfo));
	memset (&uitmp, 0, sizeof uitmp);
	_tcscpy(mi->rootdir, uci->ci.rootdir);
	if (!ui) {
		ui = &uitmp;
		if (uci->ci.type == UAEDEV_DIR) {
			cfgfile_resolve_path_out_load(uci->ci.rootdir, filepath, MAX_DPATH, PATH_DIR);
			_tcscpy(mi->rootdir, filepath);
			mi->ismounted = 1;
			if (filepath[0] == 0)
				return FILESYS_VIRTUAL;
			if (my_existsfile (filepath)) {
				mi->ismedia = 1;
				return FILESYS_VIRTUAL;
			}
			if (my_getvolumeinfo (filepath) < 0)
				return -1;
			mi->ismedia = true;
			return FILESYS_VIRTUAL;
		} else if (uci->ci.type == UAEDEV_HDF) {
			cfgfile_resolve_path_out_load(uci->ci.rootdir, filepath, MAX_DPATH, PATH_HDF);
			_tcscpy(mi->rootdir, filepath);
			ui->hf.ci.readonly = true;
			ui->hf.ci.blocksize = uci->ci.blocksize;
			int err = hdf_open (&ui->hf, filepath);
			if (err <= 0) {
				mi->ismedia = false;
				mi->ismounted = true;
				mi->error = err;
				if (uci->ci.reserved == 0 && uci->ci.sectors == 0 && uci->ci.surfaces == 0) {
					if (ui->hf.flags & 1)
						return FILESYS_HARDDRIVE;
					return FILESYS_HARDFILE_RDB;
				}
				return -1;
			}
			mi->ismedia = true;
			if (ui->hf.drive_empty)
				mi->ismedia = 0;
			hdf_close (&ui->hf);
		} else if (uci->ci.type == UAEDEV_CD) {
			cfgfile_resolve_path_out_load(uci->ci.rootdir, filepath, MAX_DPATH, PATH_CD);
			_tcscpy(mi->rootdir, filepath);
			struct device_info di;
			ui->hf.ci.readonly = true;
			ui->hf.ci.blocksize = uci->ci.blocksize;
			mi->size = -1;
			mi->ismounted = true;
			if (blkdev_get_info (p, ui->hf.ci.device_emu_unit, &di)) {
				mi->ismedia = di.media_inserted != 0;
				_tcscpy (mi->rootdir, di.label);
			}
#if 0
			if (ui->hf.ci.cd_emu_unit == 0)
				_tcscpy (mi->rootdir, _T("CD"));
			else
				_stprintf (mi->rootdir, _T("CD %d"), ui->hf.ci.cd_emu_unit);
#endif
		}
	} else if (uci->ci.type != UAEDEV_TAPE) {
		if (ui->hf.ci.controller_type == HD_CONTROLLER_TYPE_UAE) { // what is this? || (ui->controller && p->cs_ide)) {
			mi->ismounted = 1;
			if (uci->ci.type == UAEDEV_HDF)
				mi->ismedia = ui->hf.drive_empty ? false : true;
			else
				mi->ismedia = true;
		}
	}
	if (uci->ci.type == UAEDEV_TAPE) {
		cfgfile_resolve_path_out_load(uci->ci.rootdir, filepath, MAX_DPATH, PATH_TAPE);
		_tcscpy(mi->rootdir, filepath);
		struct device_info di;
		int unitnum = getuindex (p, index);
		mi->size = -1;
		mi->ismounted = false;
		if (unitnum >= 0) {
			mi->ismounted = true;
			if (tape_get_info (unitnum, &di)) {
				mi->ismedia = di.media_inserted != 0;
				_tcscpy (mi->rootdir, di.label);
			}
		} else {
			struct scsi_data_tape *tape;
			unitnum = 0;
			tape = tape_alloc (unitnum, filepath, uci->ci.readonly);
			if (tape) {
				if (tape_get_info (unitnum, &di)) {
					mi->ismedia = di.media_inserted != 0;
					_tcscpy (mi->rootdir, di.label);
				}
				tape_free (tape);
			}
		}
		return FILESYS_TAPE;
	}

	if (mi->size < 0)
		return -1;
	mi->size = ui->hf.virtsize;
	if (uci->ci.highcyl) {
		uci->ci.cyls = mi->nrcyls = uci->ci.highcyl;
	} else {
		uci->ci.cyls = mi->nrcyls = (int)(uci->ci.sectors * uci->ci.surfaces ? (ui->hf.virtsize / uci->ci.blocksize) / (uci->ci.sectors * uci->ci.surfaces) : 0);
	}
	if (uci->ci.type == UAEDEV_DIR)
		return FILESYS_VIRTUAL;
	if (uci->ci.reserved == 0 && uci->ci.sectors == 0 && uci->ci.surfaces == 0) {
		if (ui->hf.flags & 1)
			return FILESYS_HARDDRIVE;
		return FILESYS_HARDFILE_RDB;
	}
	return FILESYS_HARDFILE;
}

static void stripsemicolon (TCHAR *s)
{
	if (!s)
		return;
	while (uaetcslen(s) > 0 && s[uaetcslen(s) - 1] == ':')
		s[uaetcslen(s) - 1] = 0;
}
static void stripspace (TCHAR *s)
{
	if (!s)
		return;
	for (int i = 0; i < uaetcslen (s); i++) {
		if (s[i] == ' ')
			s[i] = '_';
	}
}
static void striplength (TCHAR *s, int len)
{
	if (!s)
		return;
	if (uaetcslen (s) <= len)
		return;
	s[len] = 0;
}
static void fixcharset(TCHAR *s)
{
	char tmp[MAX_DPATH];
	if (!s)
		return;
	ua_fs_copy(tmp, MAX_DPATH - 1, s, '_');
	au_fs_copy(s, uaestrlen(tmp) + 1, tmp);
}

TCHAR *validatevolumename (TCHAR *s, const TCHAR *def)
{
	stripsemicolon (s);
	fixcharset (s);
	striplength (s, 30);
	if (uaetcslen(s) == 0 && def) {
		xfree(s);
		s = my_strdup(def);
	}
	return s;
}
TCHAR *validatedevicename (TCHAR *s, const TCHAR *def)
{
	stripsemicolon (s);
	stripspace (s);
	fixcharset (s);
	striplength (s, 30);
	if (uaetcslen(s) == 0 && def) {
		xfree(s);
		s = my_strdup(def);
	}
	return s;
}

TCHAR *filesys_createvolname (const TCHAR *volname, const TCHAR *rootdir, struct zvolume *zv, const TCHAR *def)
{
	TCHAR *nvol = NULL;
	int i, archivehd;
	TCHAR *p = NULL;
	TCHAR path[MAX_DPATH];

	cfgfile_resolve_path_out_load(rootdir, path, MAX_DPATH, PATH_DIR);

	archivehd = -1;
	if (my_existsfile (path))
		archivehd = 1;
	else if (my_existsdir (path))
		archivehd = 0;

	if (zv && zv->volumename && uaetcslen(zv->volumename) > 0) {
		nvol = my_strdup(zv->volumename);
		nvol = validatevolumename (nvol, def);
		return nvol;
	}

	if ((!volname || uaetcslen (volname) == 0) && path[0] && archivehd >= 0) {
		p = my_strdup (path);
		for (i = uaetcslen (p) - 1; i >= 0; i--) {
			TCHAR c = p[i];
			if (c == ':' || c == '/' || c == '\\') {
				if (i == uaetcslen (p) - 1)
					continue;
				if (!_tcscmp (p + i, _T(":\\"))) {
					xfree (p);
					p = xmalloc (TCHAR, 10);
					p[0] = path[0];
					p[1] = 0;
					i = 0;
				} else {
					i++;
				}
				break;
			}
		}
		if (i >= 0)
			nvol = my_strdup (p + i);
	}
	if (!nvol && archivehd >= 0) {
		if (volname && uaetcslen (volname) > 0)
			nvol = my_strdup (volname);
		else
			nvol = my_strdup (def);
	}
	if (!nvol) {
		if (volname && uaetcslen (volname))
			nvol = my_strdup (volname);
		else
			nvol = my_strdup (_T(""));
	}
	nvol = validatevolumename (nvol, def);
	xfree (p);
	return nvol;
}

static int set_filesys_volume (const TCHAR *rootdir, int *flags, bool *readonly, bool *emptydrive, struct zvolume **zvp)
{
	*emptydrive = 0;
	if (my_existsfile (rootdir)) {
		struct zvolume *zv;
		zv = zfile_fopen_archive (rootdir);
		if (!zv) {
			error_log (_T("'%s' is not a supported archive file."), rootdir);
			return -1;
		}
		*zvp = zv;
		*flags = MYVOLUMEINFO_ARCHIVE;
		*readonly = 1;
	} else {
		*flags = my_getvolumeinfo (rootdir);
		if (*flags < 0) {
			if (rootdir && rootdir[0])
				error_log (_T("directory '%s' not found, mounting as empty drive."), rootdir);
			*emptydrive = 1;
			*flags = 0;
		} else if ((*flags) & MYVOLUMEINFO_READONLY) {
			error_log (_T("'%s' set to read-only."), rootdir);
			*readonly = 1;
		}
	}
	return 1;
}

void uci_set_defaults (struct uaedev_config_info *uci, bool rdb)
{
	memset (uci, 0, sizeof (struct uaedev_config_info));
	if (!rdb) {
		uci->sectors = 32;
		uci->reserved = 2;
		uci->surfaces = 1;
	}
	uci->blocksize = 512;
	uci->maxtransfer = 0x7fffffff;
	uci->mask = 0xffffffff;
	uci->bufmemtype = 1;
	uci->buffers = 50;
	uci->stacksize = 4000;
	uci->bootpri = 0;
	uci->priority = 10;
	uci->sectorsperblock = 1;
	uci->device_emu_unit = -1;
}

static void get_usedblocks(struct fs_usage *fsu, bool fs, int *pblocksize, uae_s64 *pnumblocks, uae_s64 *pinuse, bool reduce)
{
	uae_s64 numblocks = 0, inuse;
	int blocksize = *pblocksize;

	if (fs && currprefs.filesys_limit) {
		if (fsu->total > (uae_s64)currprefs.filesys_limit * 1024) {
			uae_s64 oldtotal = fsu->total;
			fsu->total = currprefs.filesys_limit * 1024;
			fsu->avail = ((fsu->avail / 1024) * (fsu->total / 1024)) / (oldtotal / 1024);
			fsu->avail *= 1024;
		}
	}
	if (reduce) {
		while (blocksize < 32768 || numblocks == 0) {
			numblocks = fsu->total / blocksize;
			if (numblocks <= 10)
				numblocks = 10;
			// Value that does not overflow when multiplied by 100 (uses 128 to keep it simple)
			if (numblocks < 0x02000000)
				break;
			blocksize *= 2;
		}
	}
	numblocks = fsu->total / blocksize;
	inuse = (numblocks * blocksize - fsu->avail) / blocksize;
	if (inuse > numblocks)
		inuse = numblocks;
	if (pnumblocks)
		*pnumblocks = numblocks;
	if (pinuse)
		*pinuse = inuse;
	if (pblocksize)
		*pblocksize = blocksize;
}

static bool get_blocks(const TCHAR *rootdir, int unit, int flags, int *pblocksize, uae_s64 *pnumblocks, uae_s64 *pinuse, bool reduce)
{
	struct fs_usage fsu;
	int ret;
	bool fs = false;
	int blocksize;

	blocksize = 512;
	if (flags & MYVOLUMEINFO_ARCHIVE) {
		ret = zfile_fs_usage_archive(rootdir, 0, &fsu);
		fs = true;
	} else {
		ret = get_fs_usage(rootdir, 0, &fsu);
		fs = true;
	}
	if (ret)
		return false;
	get_usedblocks(&fsu, fs, &blocksize, pnumblocks, pinuse, reduce);
	if (pblocksize)
		*pblocksize = blocksize;
	return ret == 0;
}

static int set_filesys_unit_1 (int nr, struct uaedev_config_info *ci, bool custom)
{
	UnitInfo *ui;
	int i;
	bool emptydrive = false;
	bool iscd;
	struct uaedev_config_info c;

	memcpy (&c, ci, sizeof (struct uaedev_config_info));

	if (nr < 0) {
		for (nr = 0; nr < MAX_FILESYSTEM_UNITS; nr++) {
			if (!mountinfo.ui[nr].open)
				break;
		}
		if (nr == MAX_FILESYSTEM_UNITS) {
			error_log (_T("No slot allocated for this unit"));
			return -1;
		}
	}

	if (ci->controller_type != HD_CONTROLLER_TYPE_UAE || ci->type == UAEDEV_TAPE) {
		ui = &mountinfo.ui[nr];
		memset (ui, 0, sizeof (UnitInfo));
		memcpy (&ui->hf.ci, &c, sizeof (struct uaedev_config_info));
		ui->readonly = c.readonly;
		ui->unit_type = -1;
		ui->open = -1;
		return nr;
	}

	iscd = nr >= cd_unit_offset && nr < cd_unit_offset + cd_unit_number;

	if (!custom)
		cfgfile_resolve_path_load(c.rootdir, MAX_DPATH, iscd ? PATH_CD : (ci->volname[0] ? PATH_DIR : PATH_HDF));

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (nr == i || !mountinfo.ui[i].open || mountinfo.ui[i].rootdir == NULL || is_hardfile (i) == FILESYS_CD)
			continue;
		if (uaetcslen(c.rootdir) > 0 && samepath(mountinfo.ui[i].rootdir, c.rootdir)) {
			error_log (_T("directory/hardfile '%s' already added."), c.rootdir);
			return -1;
		}
	}
	for (;;) {
		bool retry = false;
		for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
			int hf = is_hardfile(i);
			if (nr == i || !mountinfo.ui[i].open || mountinfo.ui[i].rootdir == NULL)
				continue;
			if (hf != FILESYS_HARDDRIVE && hf != FILESYS_HARDFILE && hf != FILESYS_HARDFILE_RDB)
				continue;
			if (mountinfo.ui[i].hf.ci.controller_unit == c.controller_unit) {
				c.controller_unit++;
				if (c.controller_unit >= MAX_FILESYSTEM_UNITS) {
					error_log (_T("directory/hardfile '%s' invalid unit number."), c.rootdir);
					return -1;
				}
				retry = true;
			}
		}
		if (!retry)
			break;
	}


	ui = &mountinfo.ui[nr];
	memset (ui, 0, sizeof (UnitInfo));
	memcpy(&ui->hf.ci, &c, sizeof c);

	if (iscd) {
		ui->unit_type = UNIT_CDFS;
		emptydrive = 1;
		ui->volflags = MYVOLUMEINFO_CDFS | MYVOLUMEINFO_READONLY;
		c.readonly = true;
	} else if (c.volname[0]) {
		int flags = 0;
		uae_s64 numblocks;

		emptydrive = 1;
		if (c.rootdir[0]) {
			if (set_filesys_volume (c.rootdir, &flags, &c.readonly, &emptydrive, &ui->zarchive) < 0)
				return -1;
		}
		ui->volname = filesys_createvolname (c.volname, c.rootdir, ui->zarchive, _T("harddrive"));
		ui->volflags = flags;
		TCHAR *vs = au(UAEFS_VERSION);
		TCHAR *vsp = vs + uaetcslen(vs) - 1;
		while (vsp != vs) {
			if (*vsp == ' ') {
				*vsp++ = 0;
				break;
			}
			vsp--;
		}
		_tcscpy(ui->hf.vendor_id, _T("UAE"));
		_tcscpy(ui->hf.product_id, vs);
		_tcscpy(ui->hf.product_rev, vsp);
		xfree(vs);
		ui->hf.ci.unit_feature_level = HD_LEVEL_SCSI_2;
		if (get_blocks(c.rootdir, nr, flags, &ui->hf.ci.blocksize, &numblocks, NULL, false))
			ui->hf.ci.max_lba = numblocks > 0xffffffff ? 0xffffffff : numblocks;
		else
			ui->hf.ci.max_lba = 0x00ffffff;

	} else {
		ui->unit_type = UNIT_FILESYSTEM;
		ui->hf.unitnum = nr;
		ui->volname = 0;
		if (ui->hf.ci.rootdir[0]) {
			if (hdf_open (&ui->hf) <= 0 && !c.readonly) {
				write_log (_T("Attempting to open '%s' in read-only mode.\n"), ui->hf.ci.rootdir);
				ui->hf.ci.readonly = c.readonly = true;
				if (hdf_open (&ui->hf) > 0) {
					error_log (_T("'%s' opened in read-only mode.\n"), ui->hf.ci.rootdir);
				}
			}
		} else {
			// empty drive?
			ui->hf.drive_empty = 1;
		}
		if (!ui->hf.drive_empty) {
			if (ui->hf.handle_valid == 0) {
				error_log (_T("Hardfile '%s' not found."), ui->hf.ci.rootdir);
				goto err;
			}
			if (ui->hf.ci.blocksize > ui->hf.virtsize || ui->hf.virtsize == 0) {
				error_log (_T("Hardfile '%s' too small."), ui->hf.ci.rootdir);
				goto err;
			}
		}
		if ((ui->hf.ci.blocksize & (ui->hf.ci.blocksize - 1)) != 0 || ui->hf.ci.blocksize == 0) {
			error_log (_T("Hardfile '%s' bad blocksize %d."), ui->hf.ci.rootdir, ui->hf.ci.blocksize);
			goto err;
		}
		if ((ui->hf.ci.sectors || ui->hf.ci.surfaces || ui->hf.ci.reserved) &&
			(ui->hf.ci.sectors < 1 || ui->hf.ci.surfaces < 1 || ui->hf.ci.surfaces > 1023 ||
			ui->hf.ci.reserved < 0 || ui->hf.ci.reserved > 1023) != 0) {
				error_log (_T("Hardfile '%s' bad hardfile geometry."), ui->hf.ci.rootdir);
				goto err;
		}
		if (!ui->hf.ci.highcyl) {
			ui->hf.ci.cyls = (int)(ui->hf.ci.sectors * ui->hf.ci.surfaces ? (ui->hf.virtsize / ui->hf.ci.blocksize) / (ui->hf.ci.sectors * ui->hf.ci.surfaces) : 0);
		}
		if (!ui->hf.ci.cyls)
			ui->hf.ci.cyls = ui->hf.ci.highcyl;
		if (!ui->hf.ci.cyls)
			ui->hf.ci.cyls = 1;
	}
	ui->self = 0;
	ui->reset_state = FS_STARTUP;
	ui->wasisempty = emptydrive;
	ui->canremove = emptydrive && (ci->flags & MYVOLUMEINFO_REUSABLE);
	ui->rootdir = my_strdup (c.rootdir);
	ui->devname = my_strdup (c.devname);
	stripsemicolon(ui->devname);
	if (c.filesys[0])
		ui->filesysdir = my_strdup (c.filesys);
	ui->readonly = c.readonly;
	if (c.bootpri < -129)
		c.bootpri = -129;
	if (c.bootpri > 127)
		c.bootpri = 127;
	ui->bootpri = c.bootpri;
	ui->inject_icons = c.inject_icons;
	ui->open = 1;

	return nr;
err:
	if (ui->hf.handle_valid)
		hdf_close (&ui->hf);
	return -1;
}

static int set_filesys_unit (int nr, struct uaedev_config_info *ci, bool custom)
{
	int ret;

	ret = set_filesys_unit_1 (nr, ci, custom);
	return ret;
}

// we want to reference this from amiberry_filesys.cpp also
#ifndef AMIBERRY
static
#endif
int add_filesys_unit (struct uaedev_config_info *ci, bool custom)
{
	int nr;

	if (nr_units () >= MAX_FILESYSTEM_UNITS)
		return -1;

	nr = set_filesys_unit_1 (-1, ci, custom);
#ifdef RETROPLATFORM
	if (nr >= 0) {
		UnitInfo *ui = &mountinfo.ui[nr];
		rp_hd_device_enable (nr, true);
		if (ui->unit_type == UNIT_CDFS)
			rp_cd_image_change(nr, ci->rootdir);
		else
			rp_harddrive_image_change(nr, ci->readonly, ci->rootdir);
	}
#endif
	return nr;
}

int kill_filesys_unitconfig (struct uae_prefs *p, int nr)
{
	struct uaedev_config_data *uci;

	if (nr < 0)
		return 0;
	uci = getuci (p->mountconfig, nr);
	hardfile_do_disk_change (uci, 0);
	if (uci->configoffset >= 0 && uci->ci.controller_type == HD_CONTROLLER_TYPE_UAE) {
		filesys_media_change(uci->ci.rootdir, 0, uci);
	} else {
		pcmcia_disk_reinsert(p, &uci->ci, true);
	}
	while (nr < MOUNT_CONFIG_SIZE) {
		memmove (&p->mountconfig[nr], &p->mountconfig[nr + 1], sizeof (struct uaedev_config_data));
		nr++;
	}
	p->mountitems--;
	memset (&p->mountconfig[MOUNT_CONFIG_SIZE - 1], 0, sizeof (struct uaedev_config_data));
	return 1;
}

int move_filesys_unitconfig (struct uae_prefs *p, int nr, int to)
{
	struct uaedev_config_data *uci1, *uci2, tmpuci;

	uci1 = getuci (p->mountconfig, nr);
	uci2 = getuci (p->mountconfig, to);
	if (nr == to)
		return 0;
	memcpy (&tmpuci, uci1, sizeof (struct uaedev_config_data));
	memcpy (uci1, uci2, sizeof (struct uaedev_config_data));
	memcpy (uci2, &tmpuci, sizeof (struct uaedev_config_data));
	return 1;
}

static void allocuci (struct uae_prefs *p, int nr, int idx, int unitnum)
{
	struct uaedev_config_data *uci = &p->mountconfig[nr];
	if (idx >= 0) {
		UnitInfo *ui;
		uci->configoffset = idx;
		ui = &mountinfo.ui[idx];
		ui->configureddrive = 1;
		uci->unitnum = unitnum;
	} else {
		uci->configoffset = -1;
		uci->unitnum = -1;
	}
}
static void allocuci (struct uae_prefs *p, int nr, int idx)
{
	allocuci (p, nr, idx, -1);
}

static const TCHAR *getunittype(struct uaedev_config_info *uci)
{
	return uci->type == UAEDEV_CD ? _T("CD") : (uci->type == UAEDEV_TAPE ? _T("TAPE") : _T("HD"));
}

static int cpuboard_hd;
static romconfig cpuboard_dummy;

void add_cpuboard_unit(int unit, struct uaedev_config_info *uci, struct romconfig *rc)
{
	int flags = (uci->controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && uci->controller_type <= HD_CONTROLLER_TYPE_IDE_LAST) ? EXPANSIONTYPE_IDE : EXPANSIONTYPE_SCSI;
	const struct cpuboardtype *cbt = &cpuboards[currprefs.cpuboard_type];
	cpuboard_hd = 0;
	if (cbt->subtypes) {
		if (cbt->subtypes[currprefs.cpuboard_subtype].add && (cbt->subtypes[currprefs.cpuboard_subtype].deviceflags & flags)) {
			if (unit >= 0) {
				write_log(_T("Adding CPUBoard '%s' %s unit %d ('%s')\n"),
					cbt->subtypes[currprefs.cpuboard_subtype].name,
					getunittype(uci), unit, uci->rootdir);
			}
			cbt->subtypes[currprefs.cpuboard_subtype].add(unit, uci, rc);
			cpuboard_hd = 1;
		}
	}
}

static void add_cpuboard_unit_init(void)
{
	memset(&cpuboard_dummy, 0, sizeof cpuboard_dummy);
	cpuboard_dummy.device_id = 7;
	if (currprefs.cpuboard_type) {
		struct romconfig *rc = get_device_romconfig(&currprefs, ROMTYPE_CPUBOARD, 0);
		if (!rc)
			rc = &cpuboard_dummy;
		const struct cpuboardtype *cbt = &cpuboards[currprefs.cpuboard_type];
		if (cbt->subtypes) {
			if (cbt->subtypes[currprefs.cpuboard_subtype].add) {
				const struct cpuboardsubtype *cst = &cbt->subtypes[currprefs.cpuboard_subtype];
				struct uaedev_config_info ci = { 0 };
				write_log(_T("Initializing CPUBoard '%s' %s controller\n"),
					cst->name, (cst->deviceflags & EXPANSIONTYPE_SCSI) ? _T("SCSI") : _T("IDE"));
				cst->add(-1, &ci, rc);
			}
		}
	}
}

static bool add_ide_unit(int type, int unit, struct uaedev_config_info *uci)
{
	bool added = false;
	if (type >= HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST && type <= HD_CONTROLLER_TYPE_IDE_LAST) {
		for (int i = 0; expansionroms[i].name; i++) {
			if (i == type - HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST) {
				const struct expansionromtype *ert = &expansionroms[i];
				if ((ert->deviceflags & 2) && is_board_enabled(&currprefs, ert->romtype, uci->controller_type_unit)) {
					cpuboard_hd = 1;
					if (ert->add) {
						struct romconfig *rc = get_device_romconfig(&currprefs, ert->romtype, uci->controller_type_unit);
						write_log(_T("Adding IDE %s '%s' unit %d, controller %d ('%s')\n"), getunittype(uci),
							ert->name, unit, uci->controller_type_unit, uci->rootdir);
						ert->add(unit, uci, rc);
						if ((ert->romtype & ROMTYPE_MASK) == ROMTYPE_MB_IDE) {
							gayle_ide_in_use = true;
						}
					}
					if (cpuboard_hd)
						added = true;
				}
			}
		}
	}
	return added;
}

static bool add_scsi_unit(int type, int unit, struct uaedev_config_info *uci)
{
	bool added = false;
	if (type >= HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST && type <= HD_CONTROLLER_TYPE_SCSI_LAST) {
		for (int i = 0; expansionroms[i].name; i++) {
			if (i == type - HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST) {
				const struct expansionromtype *ert = &expansionroms[i];
				if ((ert->deviceflags & 1) && is_board_enabled(&currprefs, ert->romtype, uci->controller_type_unit)) {
					cpuboard_hd = 1;
					if (ert->add) {
						struct romconfig *rc = get_device_romconfig(&currprefs, ert->romtype, uci->controller_type_unit);
						write_log(_T("Adding SCSI %s '%s' unit %d ('%s')\n"), getunittype(uci),
							ert->name, unit, uci->rootdir);
						ert->add(unit, uci, rc);
					}
					if (cpuboard_hd)
						added = true;
				}
			}
		}
	}
	return added;
}


static void initialize_mountinfo (void)
{
	int nr;
	UnitInfo *uip = &mountinfo.ui[0];

	cd_unit_offset = MAX_FILESYSTEM_UNITS;
	autocreatedunit = 0;
	gayle_ide_in_use = false;

	for (nr = 0; nr < currprefs.mountitems; nr++) {
		struct uaedev_config_data *uci = &currprefs.mountconfig[nr];
		if (uci->ci.controller_type == HD_CONTROLLER_TYPE_UAE && (uci->ci.type == UAEDEV_DIR || uci->ci.type == UAEDEV_HDF)) {
			struct uaedev_config_info ci;
			memcpy (&ci, &uci->ci, sizeof (struct uaedev_config_info));
			ci.flags = MYVOLUMEINFO_REUSABLE;
			int idx = set_filesys_unit_1 (-1, &ci, false);
			allocuci (&currprefs, nr, idx);
		}
	}
	filesys_addexternals ();
	nr = nr_units ();
	cd_unit_offset = nr;
	cd_unit_number = 0;
	if (currprefs.scsi && currprefs.automount_cddrives) {
		uae_u32 mask = scsi_get_cd_drive_mask ();
		for (int i = 0; i < 32; i++) {
			if (mask & (1 << i)) {
				struct uaedev_config_info ci = { 0 };
				_sntprintf (ci.devname, sizeof ci.devname, _T("CD%d"), i);
				cd_unit_number++;
				_tcscpy (ci.rootdir, _T("/"));
				ci.readonly = true;
				ci.sectors = 1;
				ci.surfaces = 1;
				ci.blocksize = 2048;
				int idx = set_filesys_unit_1 (i + cd_unit_offset, &ci, true);
				allocuci (&currprefs, nr, idx);
				nr++;
			}
		}
	}

	for (nr = 0; nr < currprefs.mountitems; nr++) {
		struct uaedev_config_data *uci = &currprefs.mountconfig[nr];
		if (uci->ci.controller_type == HD_CONTROLLER_TYPE_UAE) {
			if (uci->ci.type == UAEDEV_TAPE) {
				struct uaedev_config_info ci;
				memcpy (&ci, &uci->ci, sizeof (struct uaedev_config_info));
				int unitnum = scsi_add_tape (&uci->ci);
				if (unitnum >= 0) {
					int idx = set_filesys_unit_1 (-1, &ci, false);
					allocuci (&currprefs, nr, idx, unitnum);
				}
			}
		}
	}

	// init all controllers first
	add_cpuboard_unit_init();
	for (int i = 0; expansionroms[i].name; i++) {
		const struct expansionromtype *ert = &expansionroms[i];
		for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
			struct romconfig *rc = get_device_romconfig(&currprefs, ert->romtype, j);
			if ((ert->deviceflags & 3) && rc) {
				if (ert->add) {
					struct uaedev_config_info ci = { 0 };
					ci.controller_type_unit = j;
					ert->add(-1, &ci, rc);
				}
			}
		}
	}

	for (nr = 0; nr < currprefs.mountitems; nr++) {
		struct uaedev_config_info *uci = &currprefs.mountconfig[nr].ci;
		int type = uci->controller_type;
		int unit = uci->controller_unit;
		bool added = false;
		uci->uae_unitnum = nr;
		if (type == HD_CONTROLLER_TYPE_UAE) {
			continue;
		} else if (type != HD_CONTROLLER_TYPE_IDE_AUTO && type >= HD_CONTROLLER_TYPE_IDE_FIRST && type <= HD_CONTROLLER_TYPE_IDE_LAST) {
			added = add_ide_unit(type, unit, uci);
		} else if (type == HD_CONTROLLER_TYPE_IDE_AUTO) {
			for (int st = HD_CONTROLLER_TYPE_IDE_FIRST; st <= HD_CONTROLLER_TYPE_IDE_LAST; st++) {
				added = add_ide_unit(st, unit, uci);
				if (added)
					break;
			}
		} else if (type != HD_CONTROLLER_TYPE_SCSI_AUTO && type >= HD_CONTROLLER_TYPE_SCSI_FIRST && type <= HD_CONTROLLER_TYPE_SCSI_LAST) {
			added = add_scsi_unit(type, unit, uci);
		} else if (type == HD_CONTROLLER_TYPE_SCSI_AUTO) {
			for (int st = HD_CONTROLLER_TYPE_SCSI_FIRST; st <= HD_CONTROLLER_TYPE_SCSI_LAST; st++) {
				added = add_scsi_unit(st, unit, uci);
				if (added)
					break;
			}
#if 0
		} else if (type == HD_CONTROLLER_TYPE_PCMCIA) {
			if (uci->controller_type_unit == 0) {
				gayle_add_pcmcia_sram_unit (uci);
				added = true;
			} else {
				gayle_add_pcmcia_ide_unit (uci);
				added = true;
			}
#endif
		}
		if (added)
			allocuci (&currprefs, nr, -1);
	}
	

}

int sprintf_filesys_unit (TCHAR *buffer, int num)
{
	UnitInfo *uip = mountinfo.ui;

	if (uip[num].volname != 0)
		_sntprintf (buffer, sizeof buffer, _T("(DH%d:) Filesystem, %s: %s %s"), num, uip[num].volname,
		uip[num].rootdir, uip[num].readonly ? _T("ro") : _T(""));
	else
		_sntprintf (buffer, sizeof buffer, _T("(DH%d:) Hardfile, \"%s\", size %d Mbytes"), num,
		uip[num].rootdir, (int)(uip[num].hf.virtsize / (1024 * 1024)));
	return 0;
}

static void free_mountinfo (void)
{
	int i;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++)
		close_filesys_unit (mountinfo.ui + i);
	gayle_free_units ();
}

struct hardfiledata *get_hardfile_data_controller(int nr)
{
	UnitInfo *uip = mountinfo.ui;
	for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (uip[i].open == 0)
			continue;
		if (uip[i].hf.ci.controller_unit == nr && uip[i].hf.ci.type != UAEDEV_DIR)
			return &uip[i].hf;
	}
	for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (uip[i].open == 0)
			continue;
		if (uip[i].hf.ci.controller_unit == nr && uip[i].hf.ci.type == UAEDEV_DIR)
			return &uip[i].hf;
	}
	return NULL;
}
struct hardfiledata *get_hardfile_data (int nr)
{
	UnitInfo *uip = mountinfo.ui;
	if (nr < 0 || nr >= MAX_FILESYSTEM_UNITS || uip[nr].open == 0 || is_virtual (nr))
		return NULL;
	return &uip[nr].hf;
}

static uae_u32 a_uniq, key_uniq;

#define PUT_PCK_RES1(p,v) do { put_long_host((p)->packet_data + dp_Res1, ((uae_u32)v)); } while (0)
#define PUT_PCK_RES2(p,v) do { put_long_host((p)->packet_data + dp_Res2, ((uae_u32)v)); } while (0)
#define GET_PCK_TYPE(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Type)))
#define GET_PCK_RES1(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Res1)))
#define GET_PCK_RES2(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Res2)))
#define GET_PCK_ARG1(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Arg1)))
#define GET_PCK_ARG2(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Arg2)))
#define GET_PCK_ARG3(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Arg3)))
#define GET_PCK_ARG4(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Arg4)))
#define GET_PCK_ARG5(p) ((uae_s32)(get_long_host((p)->packet_data + dp_Arg5)))

#define PUT_PCK64_RES0(p,v) do { put_long_host((p)->packet_data + dp64_Res0, (v)); } while (0)
#define PUT_PCK64_RES1(p,v) do { put_long_host((p)->packet_data + dp64_Res1, (((uae_u64)v) >> 32)); put_long_host((p)->packet_data + dp64_Res1 + 4, ((uae_u32)v)); } while (0)
#define PUT_PCK64_RES2(p,v) do { put_long_host((p)->packet_data + dp64_Res2, (v)); } while (0)

#define GET_PCK64_TYPE(p) ((uae_s32)(get_long_host((p)->packet_data + dp64_Type)))
#define GET_PCK64_RES0(p) ((uae_s32)(get_long_host((p)->packet_data + dp64_Res0)))
#define GET_PCK64_RES1(p) ( (((uae_s64)(get_long_host((p)->packet_data + dp64_Res1))) << 32) | (((uae_s64)(get_long_host((p)->packet_data + dp64_Res1 + 4))) << 0) )
#define GET_PCK64_ARG1(p) ((uae_s32)(get_long_host((p)->packet_data + dp64_Arg1)))
#define GET_PCK64_ARG2(p) ( (((uae_s64)(get_long_host((p)->packet_data + dp64_Arg2))) << 32) | (((uae_s64)(get_long_host((p)->packet_data + dp64_Arg2 + 4))) << 0) )
#define GET_PCK64_ARG3(p) ((uae_s32)(get_long_host((p)->packet_data + dp64_Arg3)))
#define GET_PCK64_ARG4(p) ((uae_s32)(get_long_host((p)->packet_data + dp64_Arg4)))
#define GET_PCK64_ARG5(p) ( (((uae_s64)(get_long_host((p)->packet_data + dp64_Arg5))) << 32) | (((uae_s64)(get_long_host((p)->packet_data + dp64_Arg5 + 4))) << 0) )

static void readdpacket(TrapContext *ctx, dpacket *packet, uaecptr pck)
{
	// Read enough to get also all 64-bit fields
	packet->packet_addr = pck;
	if (trap_is_indirect() || !valid_address(pck, dp_Max)) {
		trap_get_bytes(ctx, packet->packet_array, pck, dp_Max);
		packet->packet_data = packet->packet_array;
		packet->need_flush = true;
	} else {
		packet->packet_data = get_real_address(pck);
		packet->need_flush = false;
	}
}
static void writedpacket(TrapContext *ctx, dpacket *packet)
{
	if (!packet->need_flush)
		return;
	int type = GET_PCK_TYPE(packet);
	if (type >= 8000 && type < 9000 && GET_PCK64_RES0(packet) == DP64_INIT) {
		// 64-bit RESx fields
		trap_put_bytes(ctx, packet->packet_data + 12, packet->packet_addr + 12, 32 - 12);
	} else {
		// dp_Res1 and dp_Res2
		trap_put_bytes(ctx, packet->packet_data + 12, packet->packet_addr + 12, 20 - 12);
	}
}

static void set_quadp(TrapContext *ctx, uaecptr p, uae_s64 v)
{
	if (!trap_valid_address(ctx, p, 8))
		return;
	trap_put_quad(ctx, p, v);
}
static uae_u64 get_quadp(TrapContext *ctx, uaecptr p)
{
	if (!trap_valid_address(ctx, p, 8))
		return 0;
	return trap_get_quad(ctx, p);
}

static int flush_cache (Unit *unit, int num);

static TCHAR *char1(TrapContext *ctx, uaecptr addr)
{
	uae_char buf[1024];

#if TRAPMD
	trap_get_string(ctx, (uae_u8*)buf, addr, sizeof(buf));
#else
	unsigned int i = 0;
	do {
		buf[i] = trap_get_byte(ctx, addr);
		addr++;
	} while (buf[i++] && i < sizeof (buf));
#endif
	return au_fs(buf);
}

static TCHAR *bstr1(TrapContext *ctx, uaecptr addr)
{
	uae_char buf[257];
	int n = trap_get_byte(ctx, addr);

	addr++;
#if TRAPMD
	trap_get_bytes(ctx, (uae_u8*)buf, addr, n);
#else
	for (int i = 0; i < n; i++, addr++)
		buf[i] = trap_get_byte(ctx, addr);
#endif
	buf[n] = 0;
	return au_fs(buf);
}

static TCHAR *bstr(TrapContext *ctx, Unit *unit, uaecptr addr)
{
	int n = trap_get_byte(ctx, addr);
	uae_char buf[257];

	addr++;
#if TRAPMD
	trap_get_bytes(ctx, (uae_u8*)buf, addr, n);
#else
	for (int i = 0; i < n; i++, addr++)
		buf[i] = trap_get_byte(ctx, addr);
#endif
	buf[n] = 0;
	au_fs_copy (unit->tmpbuf3, sizeof (unit->tmpbuf3) / sizeof (TCHAR), buf);
	return unit->tmpbuf3;
}
static TCHAR *cstr(TrapContext *ctx, Unit *unit, uaecptr addr)
{
	uae_char buf[257];

	trap_get_string(ctx, buf, addr, sizeof buf);
	au_fs_copy (unit->tmpbuf3, sizeof (unit->tmpbuf3) / sizeof (TCHAR), buf);
	return unit->tmpbuf3;
}

static TCHAR *bstr_cut(TrapContext *ctx, Unit *unit, uaecptr addr)
{
	TCHAR *p = unit->tmpbuf3;
	int i, colon_seen = 0, off;
	int n = trap_get_byte(ctx, addr);
	uae_char buf[257];

	off = 0;
	addr++;
	for (i = 0; i < n; i++, addr++) {
		uae_u8 c = trap_get_byte(ctx, addr);
		buf[i] = c;
		if (c == '/' || (c == ':' && colon_seen++ == 0))
			off = i + 1;
	}
	buf[i] = 0;
	au_fs_copy (unit->tmpbuf3, sizeof (unit->tmpbuf3) / sizeof (TCHAR), buf);
	return &p[off];
}

/* convert time_t to/from AmigaDOS time */
static const uae_s64 msecs_per_day = 24 * 60 * 60 * 1000;
static const uae_s64 diff = ((8 * 365 + 2) * (24 * 60 * 60)) * (uae_u64)1000;

void timeval_to_amiga (struct mytimeval *tv, int *days, int *mins, int *ticks, int tickcount)
{
	/* tv.tv_sec is secs since 1-1-1970 */
	/* days since 1-1-1978 */
	/* mins since midnight */
	/* ticks past minute @ 50Hz */

	uae_s64 t = tv->tv_sec * 1000 + tv->tv_usec / 1000;
	t -= diff;
	if (t < 0)
		t = 0;
	*days = (int)(t / msecs_per_day);
	t -= *days * msecs_per_day;
	*mins = (int)(t / (60 * 1000));
	t -= *mins * (60 * 1000);
	*ticks = (int)(t / (1000 / tickcount));
}

void amiga_to_timeval (struct mytimeval *tv, int days, int mins, int ticks, int tickcount)
{
	uae_s64 t;

	if (days < 0)
		days = 0;
	if (days > 9900 * 365)
		days = 9900 * 365; // in future far enough?
	if (mins < 0 || mins >= 24 * 60)
		mins = 0;
	if (ticks < 0 || ticks >= 60 * tickcount)
		ticks = 0;

	t = ticks * 20;
	t += mins * (60 * 1000);
	t += ((uae_u64)days) * msecs_per_day;
	t += diff;

	tv->tv_sec = t / 1000;
	tv->tv_usec = (t % 1000) * 1000;
}

static Unit *units = 0;

static Unit *find_unit (uaecptr port)
{
	Unit* u;
	for (u = units; u; u = u->next)
		if (u->port == port)
			break;

	return u;
}

static struct fs_dirhandle *fs_opendir (Unit *u, a_inode *aino)
{
	struct fs_dirhandle *fsd = xmalloc (struct fs_dirhandle, 1);
	fsd->fstype = (u->volflags & MYVOLUMEINFO_ARCHIVE) ? FS_ARCHIVE : ((u->volflags & MYVOLUMEINFO_CDFS) ? FS_CDFS : FS_DIRECTORY);
	if (fsd->fstype == FS_ARCHIVE) {
		fsd->zd = zfile_opendir_archive (aino->nname);
		if (fsd->zd)
			return fsd;
	} else if (fsd->fstype == FS_DIRECTORY) {
		fsd->od = my_opendir (aino->nname);
		if (fsd->od)
			return fsd;
	} else if (fsd->fstype == FS_CDFS) {
		fsd->isod = isofs_opendir (u->ui.cdfs_superblock, aino->uniq_external);
		if (fsd->isod)
			return fsd;
	}
	xfree (fsd);
	return NULL;
}
static void fs_closedir (struct fs_dirhandle *fsd)
{
	if (!fsd)
		return;
	if (fsd->fstype  == FS_ARCHIVE)
		zfile_closedir_archive (fsd->zd);
	else if (fsd->fstype == FS_DIRECTORY)
		my_closedir (fsd->od);
	else if (fsd->fstype == FS_CDFS)
		isofs_closedir (fsd->isod);
	xfree (fsd);
}
static struct fs_filehandle *fs_openfile (Unit *u, a_inode *aino, int flags)
{
	struct fs_filehandle *fsf = xmalloc (struct fs_filehandle, 1);
	fsf->fstype = (u->volflags & MYVOLUMEINFO_ARCHIVE) ? FS_ARCHIVE : ((u->volflags & MYVOLUMEINFO_CDFS) ? FS_CDFS : FS_DIRECTORY);
	if (fsf->fstype == FS_ARCHIVE) {
		fsf->zf = zfile_open_archive (aino->nname, flags);
		if (fsf->zf)
			return fsf;
	} else if (fsf->fstype == FS_DIRECTORY) {
		fsf->of = my_open (aino->nname, flags);
		if (fsf->of)
			return fsf;
	} else if (fsf->fstype == FS_CDFS) {
		fsf->isof = isofs_openfile (u->ui.cdfs_superblock, aino->uniq_external, flags);
		if (fsf->isof)
			return fsf;
	}
	xfree (fsf);
	return NULL;
}
static void fs_closefile (struct fs_filehandle *fsf)
{
	if (!fsf)
		return;
	if (fsf->fstype == FS_ARCHIVE) {
		zfile_close_archive (fsf->zf);
	} else if (fsf->fstype == FS_DIRECTORY) {
		my_close (fsf->of);
	} else if (fsf->fstype == FS_CDFS) {
		isofs_closefile (fsf->isof);
	}
	xfree (fsf);
}
static unsigned int fs_read (struct fs_filehandle *fsf, void *b, unsigned int size)
{
	if (fsf->fstype == FS_ARCHIVE)
		return zfile_read_archive (fsf->zf, b, size);
	else if (fsf->fstype == FS_DIRECTORY)
		return my_read (fsf->of, b, size);
	else if (fsf->fstype == FS_CDFS)
		return (unsigned int)isofs_read (fsf->isof, b, size);
	return 0;
}
static unsigned int fs_write (struct fs_filehandle *fsf, void *b, unsigned int size)
{
	if (fsf->fstype == FS_DIRECTORY)
		return my_write (fsf->of, b, size);
	return 0;
}

/* return value = old position. -1 = error. */
static uae_s64 fs_lseek64 (struct fs_filehandle *fsf, uae_s64 offset, int whence)
{
	if (fsf->fstype == FS_ARCHIVE)
		return zfile_lseek_archive (fsf->zf, offset, whence);
	else if (fsf->fstype == FS_DIRECTORY)
		return my_lseek (fsf->of, offset, whence);
	else if (fsf->fstype == FS_CDFS)
		return isofs_lseek (fsf->isof, offset, whence);
	return -1;
}
static uae_s32 fs_lseek (struct fs_filehandle *fsf, uae_s64 offset, int whence)
{
	uae_s64 v = fs_lseek64 (fsf, offset, whence);
	if (v < 0 || v > 0x7fffffff)
		return -1;
	return (uae_s32)v;
}
static uae_s64 fs_fsize64 (struct fs_filehandle *fsf)
{
	if (fsf->fstype == FS_ARCHIVE)
		return zfile_fsize_archive (fsf->zf);
	else if (fsf->fstype == FS_DIRECTORY)
		return my_fsize (fsf->of);
	else if (fsf->fstype == FS_CDFS)
		return isofs_fsize (fsf->isof);
	return -1;
}
static uae_u32 fs_fsize (struct fs_filehandle *fsf)
{
	return (uae_u32)fs_fsize64 (fsf);
}

static uae_s64 key_filesize(Key *k)
{
	if (k->aino->vfso)
		return k->aino->vfso->size;
	return fs_fsize64 (k->fd);
}
static uae_s64 key_seek(Key *k, uae_s64 offset, int whence)
{
	if (k->aino->vfso)
		return k->file_pos;
	return fs_lseek64 (k->fd, offset, whence);
}

static void set_highcyl(uaecptr volume, uae_u32 blocks)
{
	put_long(volume + 184 - 32, blocks);
}

static void set_volume_name(Unit *unit, struct mytimeval *tv)
{
	int namelen;
	int i;
	char *s;

	s = ua_fs (unit->ui.volname, -1);
	namelen = uaestrlen(s);
	if (namelen >= 58)
		namelen = 58;
	put_byte(unit->volume + 64, namelen);
	for (i = 0; i < namelen; i++)
		put_byte(unit->volume + 64 + 1 + i, s[i]);
	put_byte(unit->volume + 64 + 1 + namelen, 0);
	if (tv && (tv->tv_sec || tv->tv_usec)) {
		int days, mins, ticks;
		timeval_to_amiga (tv, &days, &mins, &ticks, 50);
		put_long(unit->volume + 16, days);
		put_long(unit->volume + 20, mins);
		put_long(unit->volume + 24, ticks);
	}
	xfree (s);
	unit->rootnode.aname = unit->ui.volname;
	unit->rootnode.nname = unit->ui.rootdir;
	unit->rootnode.mountcount = unit->mountcount;
}

static int filesys_isvolume(Unit *unit)
{
	if (!unit->volume)
		return 0;
	return get_byte(unit->volume + 64) || unit->ui.unknown_media;
}

static void clear_exkeys (Unit *unit)
{
	int i;
	a_inode *a;
	for (i = 0; i < EXKEYS; i++) {
		unit->examine_keys[i].aino = 0;
		unit->examine_keys[i].curr_file = 0;
		unit->examine_keys[i].uniq = 0;
	}
	for (i = 0; i < EXALLKEYS; i++) {
		fs_closedir (unit->exalls[i].dirhandle);
		unit->exalls[i].dirhandle = NULL;
		xfree (unit->exalls[i].fn);
		unit->exalls[i].fn = NULL;
		unit->exalls[i].id = 0;
	}
	unit->exallid = 0;
	unit->next_exkey = 1;
	a = &unit->rootnode;
	while (a) {
		a->exnext_count = 0;
		if (a->locked_children) {
			a->locked_children = 0;
			unit->total_locked_ainos--;
		}
		a = a->next;
		if (a == &unit->rootnode)
			break;
	}
}

static void filesys_delayed_eject(Unit *u)
{
	if (u->reinsertdelay)
		return;
	u->reinsertdelay = 50;
	u->newreadonly = false;
	xfree(u->newrootdir);
	u->newrootdir = NULL;
	write_log(_T("FILESYS: delayed eject %d\n"), u->unit);
}

static void filesys_delayed_change (Unit *u, int frames, const TCHAR *rootdir, const TCHAR *volume, bool readonly, int flags)
{
	if (u->reinsertdelay)
		return;
	u->reinsertdelay = frames;
	u->newflags = flags;
	u->newreadonly = readonly;
	u->newrootdir = my_strdup (rootdir);
	if (volume)
		u->newvolume = my_strdup (volume);
	filesys_eject (u->unit);
	if (!rootdir || uaetcslen (rootdir) == 0)
		u->reinsertdelay = 0;
	if (u->reinsertdelay > 0)
		write_log (_T("FILESYS: delayed insert %d: '%s' ('%s')\n"), u->unit, volume ? volume : _T("<none>"), rootdir);
}

int filesys_eject(int nr)
{
	UnitInfo *ui = &mountinfo.ui[nr];
	Unit *u = ui->self;

	if (!mountertask || u->mount_changed)
		return 0;
	if (ui->open <= 0 || u == NULL)
		return 0;
	if (!is_virtual (nr))
		return 0;
	if (!filesys_isvolume(u))
		return 0;
	u->mount_changed = -1;
	u->mountcount++;
	write_log(_T("FILESYS: volume '%s' removal request\n"), u->ui.volname);
	// -1 = remove, -2 = remove + remove device node
	put_byte(u->volume + 172 - 32, ui->unit_type == UNIT_CDFS ? -1 : -2);
	uae_Signal(get_long(u->volume + 176 - 32), 1 << 13);
	return 1;
}

static uae_u32 heartbeat;
static int heartbeat_count, heartbeat_count_cont;
static int heartbeat_task;

bool filesys_heartbeat(void)
{
	return heartbeat_count_cont > 0;
}

// This uses filesystem process to reduce resource usage
void setsystime (void)
{
	write_log("SETSYSTIME\n");
	if (!currprefs.tod_hack || !rtarea_bank.baseaddr)
		return;
	write_log("SETSYSTIME2\n");
	heartbeat = get_long_host(rtarea_bank.baseaddr + RTAREA_HEARTBEAT);
	heartbeat_task = 1;
	heartbeat_count = 10;
}

static void setsystime_vblank (void)
{
	Unit *u;
	TrapContext *ctx = NULL;

	for (u = units; u; u = u->next) {
		if (is_virtual(u->unit) && filesys_isvolume(u)) {
			put_byte(u->volume + 173 - 32, get_byte(u->volume + 173 - 32) | 1);
			uae_Signal(get_long(u->volume + 176 - 32), 1 << 13);
			break;
		}
	}
}

static uae_u32 REGPARAM2 debugger_helper(TrapContext *ctx)
{
	int mode = trap_get_dreg(ctx, 1);
	switch (mode)
	{
		case 1:
		// Execute debugger_boot() to get here.
		write_log(_T("debugger #1\n"));
		// return RunCommand(() parameters
		// does nothing if D1 == 0.
		break;
		case 2:
		// called when RunCommand() returns
		// D0 = RunCommand() return code.
		write_log(_T("debugger #2\n"));
		break;
		default:
		write_log(_T("Unknown debugger hook %d\n"), mode);
		return 0;
	}
	return 1;
}

static void debugger_boot(void)
{
	Unit *u;
	TrapContext *ctx = NULL;

	for (u = units; u; u = u->next) {
		if (is_virtual(u->unit) && filesys_isvolume(u)) {
			put_byte(u->volume + 173 - 32, get_byte(u->volume + 173 - 32) | 2);
			uae_Signal(get_long(u->volume + 176 - 32), 1 << 13);
			break;
		}
	}
}

int filesys_insert (int nr, const TCHAR *volume, const TCHAR *rootdir, bool readonly, int flags)
{
	UnitInfo *ui;
	Unit *u;
	TrapContext *ctx = NULL;

	if (!mountertask)
		return 0;

	write_log (_T("filesys_insert(%d,'%s','%s','%d','%d)\n"), nr, volume ? volume : _T("<?>"), rootdir, readonly, flags);

	if (nr < 0) {
		for (u = units; u; u = u->next) {
			if (is_virtual (u->unit)) {
				if (!filesys_isvolume(u) && mountinfo.ui[u->unit].canremove > 0)
					break;
			}
		}
		if (!u) {
			for (u = units; u; u = u->next) {
				if (is_virtual (u->unit)) {
					if (mountinfo.ui[u->unit].canremove > 0)
						break;
				}
			}
		}
		if (!u)
			return 0;
		nr = u->unit;
		ui = &mountinfo.ui[nr];
	} else {
		ui = &mountinfo.ui[nr];
		u = ui->self;
	}

	if (ui->open <= 0 || u == NULL)
		return 0;
	if (u->reinsertdelay)
		return -1;
	if (!is_virtual(nr))
		return 0;
	if (filesys_isvolume(u)) {
		filesys_delayed_change (u, 10, rootdir, volume, readonly, flags);
		return 1 + nr;
	}
	u->mountcount++;
	u->mount_changed = 1;
	u->mount_volume = volume ? my_strdup (volume) : NULL;
	u->mount_rootdir = my_strdup (rootdir);
	u->mount_readonly = readonly;
	u->mount_flags = flags;

	write_log (_T("filesys_insert %d done!\n"), nr);

	put_byte(u->volume + 172 - 32, -3); // wait for insert
	uae_Signal (get_long(u->volume + 176 - 32), 1 << 13);

	return 100 + nr;
}

static uae_u32 filesys_media_change_reply (int mode)
{
	int nr;
	UnitInfo *ui = NULL;
	Unit *u;

	for (nr = 0; nr < MAX_FILESYSTEM_UNITS; nr++) {
		ui = &mountinfo.ui[nr];
		u = ui->self;
		if (u && u->mount_changed)
			break;
	}
	if (nr >= MAX_FILESYSTEM_UNITS) {
		write_log (_T("FILESYS: filesys_media_change_reply without mount_changed flag!?\n"));
		return 0;
	}

	if (u->mount_changed < 0) {
		// eject
		if (mode == 0) {
			write_log (_T("FILESYS: got media change reply, '%s' removal finished\n"), u->ui.volname);
			flush_cache (u, -1);
			isofs_unmount (u->ui.cdfs_superblock);
			ui->cdfs_superblock = u->ui.cdfs_superblock = NULL;
			zfile_fclose_archive (ui->zarchive);
			ui->zarchive = NULL;
			u->ui.unknown_media = false;
#ifdef RETROPLATFORM
			if (ui->unit_type == UNIT_CDFS)
				rp_cd_image_change (ui->cddevno, NULL);
			else
				rp_harddrive_image_change (nr, false, NULL);
#endif
		} else {
			u->mount_changed = 0;
		}
		return 1;
	} else if (u->mount_changed > 0) {
		if (mode == 0) {
			// insert
			struct mytimeval ctime = { 0 };
			bool emptydrive = false;
			struct uaedev_config_data *uci = NULL;

			clear_exkeys (u);
			xfree (u->ui.rootdir);
			ui->rootdir = u->ui.rootdir = my_strdup (u->mount_rootdir);
			flush_cache (u, -1);
			xfree (u->ui.volname);
			ui->volname = u->ui.volname = NULL;
			if (ui->unit_type == UNIT_CDFS) {
				uae_u64 uniq;
				ui->cdfs_superblock = u->ui.cdfs_superblock = isofs_mount (ui->cddevno, &uniq);
				u->rootnode.uniq_external = uniq;
				u->ui.unknown_media = true;
				if (!u->ui.cdfs_superblock)
					return 0;
				struct isofs_info ii;
				set_highcyl(u->volume, 0);
				bool r = isofs_mediainfo (ui->cdfs_superblock, &ii);
				if (r && ii.media) {
					u->ui.unknown_media = ii.unknown_media;
					if (!ii.unknown_media) {
						u->ui.volname = ui->volname = my_strdup (ii.volumename);
						ctime.tv_sec = ii.creation;
						ctime.tv_usec = 0;
						set_highcyl(u->volume, ii.blocks);
#ifdef RETROPLATFORM
						rp_cd_image_change (ui->cddevno, ii.devname);
#endif
					}
				}
			} else {
				if (set_filesys_volume (u->mount_rootdir, &u->mount_flags, &u->mount_readonly, &emptydrive, &ui->zarchive) < 0)
					return 0;
				if (emptydrive)
					return 0;
				xfree (u->ui.volname);
				ui->volname = u->ui.volname = filesys_createvolname (u->mount_volume, u->mount_rootdir, ui->zarchive, _T("removable"));
#ifdef RETROPLATFORM
				rp_harddrive_image_change (nr, u->mount_readonly, u->mount_rootdir);
#endif
				uci = getuci (currprefs.mountconfig, nr);
			}
			if (u->ui.unknown_media) {
				write_log (_T("FILESYS: inserted unreadable volume NR=%d RO=%d\n"), nr, u->mount_readonly);
			} else {
				write_log (_T("FILESYS: inserted volume NR=%d RO=%d '%s' ('%s')\n"), nr, u->mount_readonly, ui->volname, u->mount_rootdir);
				set_volume_name(u, &ctime);
				if (u->mount_flags >= 0)
					ui->volflags = u->volflags = u->ui.volflags = u->mount_flags;
				if (uci != NULL) {
					_tcscpy (uci->ci.volname, ui->volname);
					_tcscpy (uci->ci.rootdir, u->mount_rootdir);
				}
				if (u->mount_flags >= 0) {
					ui->readonly = u->ui.readonly = u->mount_readonly;
					if (uci != NULL)
						uci->ci.readonly = u->mount_readonly;
				}
				put_byte(u->volume + 64, 0);
				put_byte(u->volume + 172 - 32, 1);
			}
		
			xfree (u->mount_volume);
			xfree (u->mount_rootdir);
			u->mount_rootdir = NULL;
			u->mount_volume = NULL;
		} else {
			u->mount_changed = 0;
		}
		
		return 1;

	}
	return 0;
}

// if multiple drag&drop mounting, queue them because mounting is async (needs Amiga-side code to run).
static int media_queued_total;
static int media_queued_cnt;
static TCHAR **media_queued_paths;

int filesys_media_change_queue(const TCHAR *rootdir, int total)
{
	if (total < 0) {
		media_queued_cnt = 0;
		for (int i = 0; i < media_queued_total; i++) {
			xfree(media_queued_paths[i]);
		}
		xfree(media_queued_paths);
		media_queued_cnt = 0;
		media_queued_total = 0;
		media_queued_paths = NULL;
		return 0;
	}
	if (total == 0) {
		media_queued_cnt = 0;
		return 0;
	}
	if (media_queued_total < total) {
		media_queued_total = total;
		media_queued_paths = xcalloc(TCHAR*, media_queued_total);
		media_queued_cnt = 0;
	}
	media_queued_paths[media_queued_cnt++] = my_strdup(rootdir);
	return 1;
}

int filesys_media_change (const TCHAR *rootdir, int inserted, struct uaedev_config_data *uci)
{
	Unit *u;
	UnitInfo *ui;
	int nr = -1;
	TCHAR volname[MAX_DPATH], *volptr;
	TCHAR devname[MAX_DPATH];
	TrapContext *ctx = NULL;

	if (!mountertask)
		return 0;
	if (automountunit >= 0)
		return -1;
	
	write_log (_T("filesys_media_change('%s',%d,%p)\n"), rootdir, inserted, uci);
	
	nr = -1;
	for (u = units; u; u = u->next) {
		if (is_virtual (u->unit)) {
			ui = &mountinfo.ui[u->unit];
			// inserted >= 2: drag&drop insert, do not replace existing normal drives
			if (inserted < 2 && ui->rootdir && !memcmp (ui->rootdir, rootdir, uaetcslen (rootdir)) && uaetcslen (rootdir) + 3 >= uaetcslen (ui->rootdir)) {
				if (filesys_isvolume(u) && inserted) {
					if (uci)ctx,
						filesys_delayed_change (u, 50, rootdir, uci->ci.volname, uci->ci.readonly, 0);
					return 0;
				}
				nr = u->unit;
				break;
			}
		}
	}
	ui = NULL;
	if (nr >= 0)
		ui = &mountinfo.ui[nr];
	/* only configured drives have automount support if automount is disabled */
	if (!currprefs.automount_removable && (!ui || !ui->configureddrive) && (inserted == 0 || inserted == 1))
		return 0;
	if (nr < 0 && !inserted)
		return 0;
	/* already mounted volume was ejected? */
	if (nr >= 0 && !inserted)
		return filesys_eject (nr);
	if (inserted) {
		struct uaedev_config_info ci = { 0 };
		if (uci) {
			volptr = my_strdup (uci->ci.volname);
		} else {
			struct uaedev_config_info ci2 = { 0 };
			_tcscpy(ci2.rootdir, rootdir);
			target_get_volume_name (&mountinfo, &ci2, 1, 0, -1);
			_tcscpy(volname, ci2.volname);
			volptr = volname;
			if (!volname[0])
				volptr = NULL;
			if (ui && ui->configureddrive && ui->volname) {
				volptr = volname;
				_tcscpy (volptr, ui->volname);
			}
		}
		if (!volptr) {
			volptr = filesys_createvolname (NULL, rootdir, NULL, _T("removable"));
			_tcscpy (volname, volptr);
			xfree (volptr);
			volptr = volname;
		}

		/* new volume inserted and it was previously mounted? */
		if (nr >= 0) {
			if (!filesys_isvolume(u)) /* not going to mount twice */
				return filesys_insert(nr, volptr, rootdir, false, -1);
			return 0;
		}
		if (inserted < 0) /* -1 = only mount if already exists */
			return 0;
		/* new volume inserted and it was not previously mounted? 
		 * perhaps we have some empty device slots? */
		nr = filesys_insert (-1, volptr, rootdir, 0, 0);
		if (nr >= 100) {
			if (uci)
				uci->configoffset = nr - 100;
			if (inserted == 3 && mountinfo.ui[nr - 100].canremove > 0)
				mountinfo.ui[nr - 100].canremove = -1;
			return nr;
		} else if (nr > 0) {
			if (inserted == 3 && mountinfo.ui[nr - 1].canremove > 0)
				mountinfo.ui[nr - 1].canremove = -1;
			return nr;
		}
		/* nope, uh, need black magic now.. */
		if (uci)
			_tcscpy (devname, uci->ci.devname);
		else
			_sntprintf (devname, sizeof devname, _T("RDH%d"), autocreatedunit++);
		_tcscpy (ci.devname, devname);
		_tcscpy (ci.volname, volptr);
		_tcscpy (ci.rootdir, rootdir);
		ci.flags = MYVOLUMEINFO_REUSABLE;
		nr = add_filesys_unit (&ci, true);
		if (nr < 0)
			return 0;
		if (inserted == 2)
			mountinfo.ui[nr].canremove = 1;
		if (inserted == 3)
			mountinfo.ui[nr].canremove = -1;
		automountunit = nr;
		uae_Signal (mountertask, 1 << 13);
		/* poof */
		if (uci)
			uci->configoffset = nr;
		return 100 + nr;
	}
	return 0;
}

int hardfile_media_change (struct hardfiledata *hfd, struct uaedev_config_info *ci, bool inserted, bool timer)
{
	if (!hfd)
		return 0;
	if (!timer)
		hfd->reinsertdelay = 0;
	if (hfd->reinsertdelay < 0) {
		hfd->reinsertdelay = 0;
		if (!hfd->isreinsert) {
			hdf_close (hfd);
			hardfile_send_disk_change (hfd, false);
			if (hfd->delayedci.rootdir[0]) {
				hfd->reinsertdelay = 50;
				hfd->isreinsert = true;
				write_log (_T("HARDFILE: delayed insert %d: '%s'\n"), hfd->unitnum, ci->rootdir[0] ? ci->rootdir : _T("<none>"));
				return 0;
			} else {
				return 1;
			}
		}
		memcpy (&hfd->ci, &hfd->delayedci, sizeof (struct uaedev_config_info));
		if (hdf_open (hfd) <= 0) {
			write_log (_T("HARDFILE: '%s' failed to open\n"), hfd->ci.rootdir);
			return 0;
		}
		hardfile_send_disk_change (hfd, true);
		return 1;
	}

	if (ci) {
		memcpy (&hfd->delayedci, ci, sizeof (struct uaedev_config_info));
		if (hfd && !hfd->drive_empty) {
			hfd->reinsertdelay = 50;
			hfd->isreinsert = false;
			write_log (_T("HARDFILE: delayed eject %d: '%s'\n"), hfd->unitnum, hfd->ci.rootdir[0] ? hfd->ci.rootdir : _T("<none>"));
			return 0;
		}
		if (!hfd) {
			return 0;
		}
		hfd->reinsertdelay = 2;
		hfd->isreinsert = true;
	} else {
		if (inserted) {
			hfd->reinsertdelay = 2;
			hfd->isreinsert = true;
			memcpy (&hfd->delayedci, &hfd->ci, sizeof (struct uaedev_config_info));
		} else {
			hfd->reinsertdelay = 2;
			hfd->isreinsert = false;
			memcpy (&hfd->delayedci, &hfd->ci, sizeof (struct uaedev_config_info));
			hfd->delayedci.rootdir[0] = 0;
		}
	}
	return 0;
}

#if 0
int hardfile_remount (int nr)
{
	/* this does work but every media reinsert duplicates the device.. */
#if 0
	if (!mountertask)
		return 0;
	automountunit = nr;
	uae_Signal (mountertask, 1 << 13);
#endif
	return 1;
}
#endif

bool filesys_do_disk_change (int cdunitnum, bool insert)
{
	int nr = cdunitnum + cd_unit_offset;
	UnitInfo *ui = &mountinfo.ui[nr];
	Unit *u = ui->self;
	TrapContext *ctx = NULL;

	if (!ui->cd_open)
		return false;
	if (insert) {
		if (filesys_isvolume(u))
			return false;
		filesys_insert (nr, NULL, _T("/"), true, MYVOLUMEINFO_CDFS | MYVOLUMEINFO_READONLY);
		return true;
	} else {
		if (!filesys_isvolume(u))
			return false;
		filesys_eject (nr);
		return true;
	}
}

/* flags and comments supported? */
static int fsdb_cando (Unit *unit)
{
	if (unit->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS))
		return 1;
	if (currprefs.filesys_custom_uaefsdb  && (unit->volflags & MYVOLUMEINFO_STREAMS))
		return 1;
	if (!currprefs.filesys_no_uaefsdb)
		return 1;
	return 0;
}

static void prepare_for_open (TCHAR *name)
{
}

static void de_recycle_aino (Unit *unit, a_inode *aino)
{
	aino_test (aino);
	if (aino->next == 0 || aino == &unit->rootnode)
		return;
	aino->next->prev = aino->prev;
	aino->prev->next = aino->next;
	aino->next = aino->prev = 0;
	unit->aino_cache_size--;
}

static void free_aino(a_inode *aino)
{
	xfree(aino->aname);
	xfree(aino->comment);
	xfree(aino->nname);
	xfree(aino);
}

static void dispose_aino (Unit *unit, a_inode **aip, a_inode *aino)
{
	int hash = aino->uniq % MAX_AINO_HASH;
	if (unit->aino_hash[hash] == aino)
		unit->aino_hash[hash] = 0;

	if (aino->dirty && aino->parent)
		fsdb_dir_writeback (aino->parent);

	*aip = aino->sibling;

	if (unit->volflags & MYVOLUMEINFO_ARCHIVE) {
		;
	} else if (unit->volflags & MYVOLUMEINFO_CDFS) {
		isofs_dispose_inode (unit->ui.cdfs_superblock, aino->uniq_external);
	}

	free_aino(aino);
}

static void free_all_ainos (Unit *u, a_inode *parent)
{
	a_inode *a;
	while ((a = parent->child)) {
		free_all_ainos (u, a);
		dispose_aino (u, &parent->child, a);
	}
}

static int flush_cache (Unit *unit, int num)
{
	int i = 0;
	int cnt = 100;

	//write_log (_T("FILESYS: flushing cache unit %d (max %d items)\n"), unit->unit, num);
	if (num == 0)
		num = -1;
	while (i < num || num < 0) {
		int ii = i;
		a_inode *parent = unit->rootnode.prev->parent;
		a_inode **aip;

		aip = &parent->child;
		aino_test (parent);
		if (parent && !parent->locked_children) {
			for (;;) {
				a_inode *aino = *aip;
				aino_test (aino);
				if (aino == 0)
					break;
				/* Not recyclable if next == 0 (i.e., not chained into
				recyclable list), or if parent directory is being
				ExNext()ed.  */
				if (aino->next == 0) {
					aip = &aino->sibling;
				} else {
					if (aino->shlock > 0 || aino->elock)
						write_log (_T("panic: freeing locked a_inode!\n"));
					de_recycle_aino (unit, aino);
					dispose_aino (unit, aip, aino);
					i++;
				}
			}
		}
		{ //if (unit->rootnode.next != unit->rootnode.prev) {
			/* In the previous loop, we went through all children of one
			parent.  Re-arrange the recycled list so that we'll find a
			different parent the next time around.
			(infinite loop if there is only one parent?)
			*/
			int maxloop = 10000;
			do {
				unit->rootnode.next->prev = unit->rootnode.prev;
				unit->rootnode.prev->next = unit->rootnode.next;
				unit->rootnode.next = unit->rootnode.prev;
				unit->rootnode.prev = unit->rootnode.prev->prev;
				unit->rootnode.prev->next = unit->rootnode.next->prev = &unit->rootnode;
			} while (unit->rootnode.prev->parent == parent && maxloop-- > 0);
		}
		if (i == ii)
			cnt--;
		if (cnt <= 0)
			break;
	}
	return unit->aino_cache_size > 0 ? 0 : 1;
}

static void recycle_aino (Unit *unit, a_inode *new_aino)
{
	aino_test (new_aino);
	if (new_aino->dir || new_aino->shlock > 0
		|| new_aino->elock || new_aino == &unit->rootnode)
		/* Still in use */
		return;

	TRACE3((_T("Recycling; cache size %u, total_locked %d\n"),
		unit->aino_cache_size, unit->total_locked_ainos));
	if (unit->aino_cache_size > 5000 + unit->total_locked_ainos) {
		/* Reap a few. */
		flush_cache (unit, 50);
#if 0
		{
			TCHAR buffer[40];
			_stprintf (buffer, "%d ainos reaped.\n", i);
			TRACE ((buffer));
		}
#endif
	}

	aino_test (new_aino);
	/* Chain it into circular list. */
	new_aino->next = unit->rootnode.next;
	new_aino->prev = &unit->rootnode;
	new_aino->prev->next = new_aino;
	new_aino->next->prev = new_aino;
	aino_test (new_aino->next);
	aino_test (new_aino->prev);

	unit->aino_cache_size++;
}

void filesys_flush_cache (void)
{
}

static void update_child_names (Unit *unit, a_inode *a, a_inode *parent)
{
	int l0 = uaetcslen (parent->nname) + 2;

	while (a != 0) {
		TCHAR *name_start;
		TCHAR *new_name;
		TCHAR dirsep[2] = { FSDB_DIR_SEPARATOR, '\0' };

		a->parent = parent;
		name_start = _tcsrchr (a->nname, FSDB_DIR_SEPARATOR);
		if (name_start == 0) {
			write_log (_T("malformed file name"));
		}
		name_start++;
		new_name = xmalloc (TCHAR, uaetcslen (name_start) + l0);
		_tcscpy (new_name, parent->nname);
		_tcscat (new_name, dirsep);
		_tcscat (new_name, name_start);
		xfree (a->nname);
		a->nname = new_name;
		if (a->child)
			update_child_names (unit, a->child, a);
		a = a->sibling;
	}
}

static void move_aino_children (Unit *unit, a_inode *from, a_inode *to)
{
	aino_test (from);
	aino_test (to);
	to->child = from->child;
	from->child = 0;
	update_child_names (unit, to->child, to);
}

static void delete_aino (Unit *unit, a_inode *aino)
{
	a_inode **aip;

	TRACE((_T("deleting aino %x\n"), aino->uniq));

	aino_test (aino);
	aino->dirty = 1;
	aino->deleted = 1;
	de_recycle_aino (unit, aino);

	/* If any ExKeys are currently pointing at us, advance them.  */
	if (aino->parent->exnext_count > 0) {
		int i;
		TRACE((_T("entering exkey validation\n")));
		for (i = 0; i < EXKEYS; i++) {
			ExamineKey *k = unit->examine_keys + i;
			if (k->uniq == 0)
				continue;
			if (k->aino == aino->parent) {
				TRACE((_T("Same parent found for %d\n"), i));
				if (k->curr_file == aino) {
					k->curr_file = aino->sibling;
					TRACE((_T("Advancing curr_file\n")));
				}
			}
		}
	}

	aip = &aino->parent->child;
	while (*aip != aino && *aip != 0)
		aip = &(*aip)->sibling;
	if (*aip != aino) {
		write_log (_T("Couldn't delete aino.\n"));
		return;
	}
	dispose_aino (unit, aip, aino);
}

static a_inode *lookup_sub (a_inode *dir, uae_u32 uniq)
{
	a_inode **cp = &dir->child;
	a_inode *c, *retval;

	for (;;) {
		c = *cp;
		if (c == 0)
			return 0;

		if (c->uniq == uniq) {
			retval = c;
			break;
		}
		if (c->dir) {
			a_inode *a = lookup_sub (c, uniq);
			if (a != 0) {
				retval = a;
				break;
			}
		}
		cp = &c->sibling;
	}
	if (! dir->locked_children) {
		/* Move to the front to speed up repeated lookups.  Don't do this if
		an ExNext is going on in this directory, or we'll terminally
		confuse it.  */
		*cp = c->sibling;
		c->sibling = dir->child;
		dir->child = c;
	}
	return retval;
}

static a_inode *lookup_aino (Unit *unit, uae_u32 uniq)
{
	a_inode *a;
	int hash = uniq % MAX_AINO_HASH;

	if (uniq == 0)
		return &unit->rootnode;
	a = unit->aino_hash[hash];
	if (a == 0 || a->uniq != uniq)
		a = lookup_sub (&unit->rootnode, uniq);
	else
		unit->nr_cache_hits++;
	unit->nr_cache_lookups++;
	unit->aino_hash[hash] = a;
	aino_test (a);
	return a;
}
static a_inode *aino_from_lock(TrapContext *ctx, Unit *unit, uaecptr lock)
{
	return lookup_aino (unit, trap_get_long(ctx, lock + 4));
}

TCHAR *build_nname (const TCHAR *d, const TCHAR *n)
{
	TCHAR dsep[2] = { FSDB_DIR_SEPARATOR, 0 };
	TCHAR *p = xmalloc (TCHAR, uaetcslen (d) + 1 + uaetcslen (n) + 1);
	_tcscpy (p, d);
	_tcscat (p, dsep);
	_tcscat (p, n);
	return p;
}

TCHAR *build_aname (const TCHAR *d, const TCHAR *n)
{
	TCHAR *p = xmalloc (TCHAR, uaetcslen (d) + 1 + uaetcslen (n) + 1);
	_tcscpy (p, d);
	_tcscat (p, _T("/"));
	_tcscat (p, n);
	return p;
}

/* This gets called to translate an Amiga name that some program used to
* a name that we can use on the native filesystem.  */
static TCHAR *get_nname (Unit *unit, a_inode *base, TCHAR *rel, TCHAR **modified_rel, uae_u64 *uniq_ext)
{
	TCHAR *found;

	*modified_rel = 0;

	if (unit->volflags & MYVOLUMEINFO_ARCHIVE) {
		if (zfile_exists_archive (base->nname, rel))
			return build_nname (base->nname, rel);
		return NULL;
	} else if (unit->volflags & MYVOLUMEINFO_CDFS) {
		if (isofs_exists (unit->ui.cdfs_superblock, base->uniq_external, rel, uniq_ext))
			return build_nname (base->nname, rel);
		return NULL;
	}

	aino_test (base);

	/* If we have a mapping of some other aname to "rel", we must pretend
	* it does not exist.
	* This can happen for example if an Amiga program creates a
	* file called ".".  We can't represent this in our filesystem,
	* so we create a special file "uae_xxx" and record the mapping
	* aname "." -> nname "uae_xxx" in the database.  Then, the Amiga
	* program looks up "uae_xxx" (yes, it's contrived).  The filesystem
	* should not make the uae_xxx file visible to the Amiga side.  */
	if (fsdb_used_as_nname (base, rel))
		return 0;
	/* A file called "." (or whatever else is invalid on this filesystem)
	* does not exist, as far as the Amiga side is concerned.  */
	if (fsdb_name_invalid_dir (base, rel))
		return 0;

	/* See if we have a file that has the same name as the aname we are
	* looking for.  */
#ifdef _WIN32
	TCHAR *relalt = NULL;
	found = fsdb_search_dir (base->nname, rel, &relalt);
#else
	found = fsdb_search_dir (base->nname, rel);
#endif
	if (found == 0) {
		return found;
	}
	if (found == rel) {
#ifdef _WIN32
		if (relalt) {
			TCHAR *v = build_nname(base->nname, relalt);
			xfree(relalt);
			return v;
		}
#endif
		return build_nname(base->nname, rel);
	}
	*modified_rel = found;
#ifdef _WIN32
	if (relalt) {
		TCHAR *v = build_nname(base->nname, relalt);
		xfree(relalt);
		return v;
	}
#endif
	return build_nname (base->nname, found);
}

static TCHAR *create_nname (Unit *unit, a_inode *base, TCHAR *rel)
{
	TCHAR *p;

	aino_test (base);
	/* We are trying to create a file called REL.  */

	/* If the name is used otherwise in the directory (or globally), we
	* need a new unique nname.  */
	if (fsdb_name_invalid (base, rel) || fsdb_used_as_nname (base, rel)) {
#if 0
oh_dear:
#endif
		if (currprefs.filesys_no_uaefsdb && !(base->volflags & MYVOLUMEINFO_STREAMS)) {
			write_log (_T("illegal filename '%s', no stream supporting filesystem and uaefsdb disabled\n"), rel);
			return 0;
		}
		p = fsdb_create_unique_nname (base, rel);
		return p;
	}
	p = build_nname (base->nname, rel);
#if 0
	/* Delete this code once we know everything works.  */
	if (access (p, R_OK) >= 0 || errno != ENOENT) {
		write_log (_T("Filesystem in trouble... please report.\n"));
		xfree (p);
		goto oh_dear;
	}
#endif
	return p;
}

static int fill_file_attrs (Unit *u, a_inode *base, a_inode *c)
{
	if (u->volflags & MYVOLUMEINFO_ARCHIVE) {
		int isdir, flags;
		TCHAR *comment;
		zfile_fill_file_attrs_archive (c->nname, &isdir, &flags, &comment);
		c->dir = isdir;
		c->amigaos_mode = 0;
		if (flags >= 0)
			c->amigaos_mode = flags;
		c->comment = comment;
		return 1;
	} else if (u->volflags & MYVOLUMEINFO_CDFS) {
		int isdir, flags;
		TCHAR *comment;
		isofss_fill_file_attrs (u->ui.cdfs_superblock, base->uniq_external, &isdir, &flags, &comment, c->uniq_external);
		c->dir = isdir;
		c->amigaos_mode = 0;
		if (flags >= 0)
			c->amigaos_mode = flags;
		c->comment = comment;
		return 1;
	} else {
		return fsdb_fill_file_attrs (base, c);
	}
	return 0;
}

static int test_softlink (a_inode *aino)
{
	int err;
	if (aino->softlink && my_resolvesoftlink (aino->nname, -1, false))
		err = ERROR_IS_SOFT_LINK;
	else
		err = ERROR_OBJECT_NOT_AROUND;
	return err;
}
static void handle_softlink(TrapContext *ctx, Unit *unit, dpacket *packet, a_inode *aino)
{
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, test_softlink (aino));
}

/*
* This gets called if an ACTION_EXAMINE_NEXT happens and we hit an object
* for which we know the name on the native filesystem, but no corresponding
* Amiga filesystem name.
* @@@ For DOS filesystems, it might make sense to declare the new name
* "weak", so that it can get overriden by a subsequent call to get_nname().
* That way, if someone does "dir :" and there is a file "foobar.inf", and
* someone else tries to open "foobar.info", get_nname() could maybe made to
* figure out that this is supposed to be the file "foobar.inf".
* DOS sucks...
*/
static TCHAR *get_aname (Unit *unit, a_inode *base, TCHAR *rel)
{
	return my_strdup (rel);
}

static void init_child_aino_tree (Unit *unit, a_inode *base, a_inode *aino)
{
	/* Update tree structure */
	aino->parent = base;
	aino->child = 0;
	aino->sibling = base->child;
	base->child = aino;
	aino->next = aino->prev = 0;
	aino->volflags = unit->volflags;
}

static void init_child_aino (Unit *unit, a_inode *base, a_inode *aino)
{
	aino->uniq = ++a_uniq;
	if (a_uniq == 0xFFFFFFFF) {
		write_log (_T("Running out of a_inodes (prepare for big trouble)!\n"));
	}
	aino->shlock = 0;
	aino->elock = 0;

	aino->dirty = 0;
	aino->deleted = 0;
	aino->mountcount = unit->mountcount;

	/* For directories - this one isn't being ExNext()ed yet.  */
	aino->locked_children = 0;
	aino->exnext_count = 0;
	/* But the parent might be.  */
	if (base->exnext_count) {
		unit->total_locked_ainos++;
		base->locked_children++;
	}
	init_child_aino_tree (unit, base, aino);

	aino_test_init (aino);
	aino_test (aino);
}

static a_inode *new_child_aino(Unit *unit, a_inode *base, TCHAR *rel)
{
	TCHAR *modified_rel;
	TCHAR *nn;
	a_inode *aino = NULL;
	int isvirtual = unit->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS);

	TRACE((_T("new_child_aino %s, %s\n"), base->aname, rel));

	if (!isvirtual) {
		aino = fsdb_lookup_aino_aname(base, rel);
	}

	if (aino == NULL) {
		uae_u64 uniq_ext = 0;
		nn = get_nname(unit, base, rel, &modified_rel, &uniq_ext);
		if (nn == NULL) {
			xfree(modified_rel);
			return 0;
		}

		aino = xcalloc(a_inode, 1);
		if (aino == 0) {
			xfree(modified_rel);
			return 0;
		}

		aino->uniq_external = uniq_ext;
		aino->aname = modified_rel ? modified_rel : my_strdup(rel);
		aino->nname = nn;

		aino->comment = 0;
		aino->has_dbentry = 0;

		if (!fill_file_attrs(unit, base, aino)) {
			free_aino(aino);
			return 0;
		}
		if (aino->dir && !isvirtual) {
			fsdb_clean_dir(aino);
		}
	}
	init_child_aino(unit, base, aino);

	recycle_aino(unit, aino);
	TRACE((_T("created aino %x, lookup, amigaos_mode %d\n"), aino->uniq, aino->amigaos_mode));
	return aino;
}

static a_inode *create_child_aino (Unit *unit, a_inode *base, TCHAR *rel, int isdir)
{
	a_inode *aino = xcalloc (a_inode, 1);
	if (aino == 0)
		return 0;

	aino->nname = create_nname (unit, base, rel);
	if (!aino->nname) {
		free_aino(aino);
		return 0;
	}
	aino->aname = my_strdup (rel);

	init_child_aino (unit, base, aino);
	aino->amigaos_mode = 0;
	aino->dir = isdir;

	aino->comment = 0;
	aino->has_dbentry = 0;
	aino->dirty = 1;

	recycle_aino (unit, aino);
	TRACE((_T("created aino %x, create\n"), aino->uniq));
	return aino;
}

static a_inode *lookup_child_aino (Unit *unit, a_inode *base, TCHAR *rel, int *err)
{
	a_inode *c = base->child;
	int l0 = uaetcslen (rel);

	aino_test (base);
	aino_test (c);

	if (base->dir == 0) {
		*err = ERROR_OBJECT_WRONG_TYPE;
		return 0;
	}

	while (c != 0) {
		int l1 = uaetcslen (c->aname);
		if (l0 <= l1 && same_aname (rel, c->aname + l1 - l0)
			&& (l0 == l1 || c->aname[l1-l0-1] == '/') && c->mountcount == unit->mountcount)
			break;
		c = c->sibling;
	}
	if (c != 0)
		return c;
	c = new_child_aino (unit, base, rel);
	if (c == 0)
		*err = ERROR_OBJECT_NOT_AROUND;
	return c;
}

/* Different version because for this one, REL is an nname.  */
static a_inode *lookup_child_aino_for_exnext (Unit *unit, a_inode *base, TCHAR *rel, uae_u32 *err, uae_u64 uniq_external, struct virtualfilesysobject *vfso)
{
	a_inode *c = base->child;
	int l0 = uaetcslen (rel);
	int isvirtual = unit->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS);

	aino_test (base);
	aino_test (c);

	*err = 0;
	while (c != 0) {
		int l1 = uaetcslen (c->nname);
		/* Note: using _tcscmp here.  */
		if (l0 <= l1 && _tcscmp (rel, c->nname + l1 - l0) == 0
			&& (l0 == l1 || c->nname[l1-l0-1] == FSDB_DIR_SEPARATOR) && c->mountcount == unit->mountcount)
			break;
		c = c->sibling;
	}
	if (c != 0)
		return c;
	if (!isvirtual && !vfso)
		c = fsdb_lookup_aino_nname (base, rel);
	if (c == 0) {
		c = xcalloc (a_inode, 1);
		if (c == 0) {
			*err = ERROR_NO_FREE_STORE;
			return 0;
		}

		c->nname = build_nname (base->nname, rel);
		c->aname = get_aname (unit, base, rel);
		c->comment = 0;
		c->uniq_external = uniq_external;
		c->has_dbentry = 0;
		if (vfso) {
			c->dir = vfso->dir;
			c->comment = my_strdup(vfso->comment);
			c->amigaos_mode = vfso->amigaos_mode;
			c->vfso = vfso;
		} else if (!fill_file_attrs (unit, base, c)) {
			xfree (c);
			*err = ERROR_NO_FREE_STORE;
			return 0;
		}
		if (c->dir && !isvirtual && !vfso)
			fsdb_clean_dir (c);
	}
	init_child_aino (unit, base, c);

	recycle_aino (unit, c);
	TRACE((_T("created aino %s:%d, exnext\n"), c->nname, c->uniq));

	return c;
}

static a_inode *get_aino (Unit *unit, a_inode *base, const TCHAR *rel, int *err)
{
	TCHAR *tmp;
	TCHAR *p;
	a_inode *curr, *prev;
	int i;

	aino_test (base);

	*err = 0;
	TRACE((_T("get_path(%s,%s)\n"), base->aname, rel));

	/* root-relative path? */
	for (i = 0; rel[i] && rel[i] != '/' && rel[i] != ':'; i++)
		;
	if (':' == rel[i])
		rel += i+1;

	tmp = my_strdup (rel);
	p = tmp;
	curr = base;
	prev = NULL;

	while (*p) {
		/* start with a slash? go up a level. */
		if (*p == '/') {
			if (curr->parent != 0)
				curr = curr->parent;
			p++;
		} else {
			a_inode *next;


			if (prev && prev->softlink) {
				*err = test_softlink (prev);
				curr = NULL;
				break;
			}

			TCHAR *component_end;
			component_end = _tcschr (p, '/');
			if (component_end != 0)
				*component_end = '\0';
			next = lookup_child_aino (unit, curr, p, err);
			if (next == 0) {
				/* if only last component not found, return parent dir. */
				if (*err != ERROR_OBJECT_NOT_AROUND || component_end != 0)
					curr = NULL;
				/* ? what error is appropriate? */
				break;
			}
			prev = next;
			curr = next;
			if (component_end)
				p = component_end+1;
			else
				break;
		}
	}
	xfree (tmp);
	return curr;
}


static uae_u32 notifyhash (const TCHAR *s)
{
	uae_u32 hash = 0;
	while (*s)
		hash = (hash << 5) + *s++;
	return hash % NOTIFY_HASH_SIZE;
}

static Notify *new_notify (Unit *unit, TCHAR *name)
{
	Notify *n = xmalloc (Notify, 1);
	uae_u32 hash = notifyhash (name);
	n->next = unit->notifyhash[hash];
	unit->notifyhash[hash] = n;
	n->partname = name;
	return n;
}

#if 0
static void free_notify_item (Notify *n)
{
	xfree (n->fullname);
	xfree (n->partname);
	xfree (n);
}
#endif

static void free_notify (Unit *unit, int hash, Notify *n)
{
	Notify *n1, *prev = 0;
	for (n1 = unit->notifyhash[hash]; n1; n1 = n1->next) {
		if (n == n1) {
			if (prev)
				prev->next = n->next;
			else
				unit->notifyhash[hash] = n->next;
			break;
		}
		prev = n1;
	}
}

static void startup_update_unit (Unit *unit, UnitInfo *uinfo)
{
	if (!unit)
		return;
	xfree (unit->ui.volname);
	memcpy (&unit->ui, uinfo, sizeof (UnitInfo));
	unit->ui.devname = uinfo->devname;
#ifdef AMIBERRY
	if (!uinfo->volname) {
		// prevents a crash on linux/mac, when e.g. a cd-rom
		// image was not found
		uinfo->volname = my_strdup("");
	}
#endif
	unit->ui.volname = my_strdup (uinfo->volname); /* might free later for rename */
}

static Unit *startup_create_unit(TrapContext *ctx, UnitInfo *uinfo, int num)
{
	int i;
	Unit *unit, *u;

	unit = xcalloc (Unit, 1);
	/* keep list in insertion order */
	u = units;
	if (u) {
		while (u->next)
			u = u->next;
		u->next = unit;
	} else {
		units = unit;
	}
	uinfo->self = unit;

	unit->volume = 0;
	unit->port = trap_get_areg(ctx, 5);
	unit->unit = num;

	startup_update_unit (unit, uinfo);

	unit->cmds_complete = 0;
	unit->cmds_sent = 0;
	unit->cmds_acked = 0;
	clear_exkeys (unit);
	unit->total_locked_ainos = 0;
	unit->keys = 0;
	for (i = 0; i < NOTIFY_HASH_SIZE; i++) {
		Notify *n = unit->notifyhash[i];
		while (n) {
			Notify *n2 = n;
			n = n->next;
			xfree (n2->fullname);
			xfree (n2->partname);
			xfree (n2);
		}
		unit->notifyhash[i] = 0;
	}

	unit->rootnode.aname = uinfo->volname;
	unit->rootnode.nname = uinfo->rootdir;
	unit->rootnode.sibling = 0;
	unit->rootnode.next = unit->rootnode.prev = &unit->rootnode;
	unit->rootnode.uniq = 0;
	unit->rootnode.parent = 0;
	unit->rootnode.child = 0;
	unit->rootnode.dir = 1;
	unit->rootnode.amigaos_mode = 0;
	unit->rootnode.shlock = 0;
	unit->rootnode.elock = 0;
	unit->rootnode.comment = 0;
	unit->rootnode.has_dbentry = 0;
	unit->rootnode.volflags = uinfo->volflags;
	aino_test_init (&unit->rootnode);
	unit->aino_cache_size = 0;
	for (i = 0; i < MAX_AINO_HASH; i++)
		unit->aino_hash[i] = 0;
	return unit;
}


static bool mount_cd(UnitInfo *uinfo, int nr, struct mytimeval *ctime, uae_u64 *uniq, uaecptr volume)
{
	uinfo->cddevno = nr - cd_unit_offset;
	if (!sys_command_open (uinfo->cddevno)) {
		write_log (_T("Failed attempt to open CD unit %d\n"), uinfo->cddevno);
		return false;
	}
#ifdef RETROPLATFORM
	rp_cd_device_enable (uinfo->cddevno, true);
#endif
	uinfo->cdfs_superblock = isofs_mount(uinfo->cddevno, uniq);
	uinfo->wasisempty = true;
	struct isofs_info ii;
	if (isofs_mediainfo (uinfo->cdfs_superblock, &ii)) {
		xfree (uinfo->volname);
		if (ii.media) {
			uinfo->wasisempty = false;
			if (!ii.unknown_media) {
				uinfo->volname = my_strdup (ii.volumename);
				if (ctime) {
					ctime->tv_sec = ii.creation;
					ctime->tv_usec = 0;
				}
				set_highcyl(volume, ii.totalblocks);
#ifdef RETROPLATFORM
				rp_cd_image_change (uinfo->cddevno, ii.devname);
#endif
			}
		}
		uinfo->unknown_media = ii.unknown_media;
	}
	uinfo->cd_open = true;
	return true;
}

#ifdef UAE_FILESYS_THREADS
static int filesys_thread (void *unit_v);
#endif
static void filesys_start_thread (UnitInfo *ui, int nr)
{
	ui->unit_pipe = 0;
	ui->back_pipe = 0;
	ui->reset_state = FS_STARTUP;
	if (!isrestore ()) {
		ui->startup = 0;
		ui->self = 0;
	}
#ifdef UAE_FILESYS_THREADS
	if (is_virtual (nr)) {
		ui->unit_pipe = xmalloc (smp_comm_pipe, 1);
		ui->back_pipe = xmalloc (smp_comm_pipe, 1);
		init_comm_pipe (ui->unit_pipe, 400, 3);
		init_comm_pipe (ui->back_pipe, 100, 1);
		uae_start_thread (_T("filesys"), filesys_thread, (void *)ui, &ui->tid);
	}
#endif
	if (isrestore ()) {
		if (ui->unit_type == UNIT_CDFS) {
			mount_cd(ui, nr, NULL, &ui->self->rootnode.uniq_external, ui->self->volume);
		}
		startup_update_unit (ui->self, ui);
	}
}


static uae_u32 REGPARAM2 startup_handler(TrapContext *ctx)
{
	uae_u32 mode = trap_get_dreg(ctx, 0);

	if (mode == 1) {
		uaecptr addr = 0;
		if (currprefs.uaeboard > 1) {
			// return board ram instead of allocating normal RAM
			addr = uaeboard_alloc_ram(trap_get_dreg(ctx, 1));
		}
		return addr;
	}

	/* Just got the startup packet. It's in D3. DosBase is in A2,
	* our allocated volume structure is in A3, A5 is a pointer to
	* our port. */
	uaecptr rootnode = trap_get_long(ctx, trap_get_areg(ctx, 2) + 34);
	uaecptr dos_info = trap_get_long(ctx, rootnode + 24) << 2;
	uaecptr pkt = trap_get_dreg(ctx, 3);
	uaecptr arg1 = trap_get_long(ctx, pkt + dp_Arg1);
	uaecptr arg2 = trap_get_long(ctx, pkt + dp_Arg2);
	uaecptr arg3 = trap_get_long(ctx, pkt + dp_Arg3);
	uaecptr devnode, volume;
	int nr;
	Unit *unit;
	UnitInfo *uinfo;
	int late = 0;
	int ed, ef;
	uae_u64 uniq = 0;
	uae_u32 cdays;
	struct mytimeval ctime = { 0 };

	// 1.3:
	// dp_Arg1 contains crap (Should be name of device)
	// dp_Arg2 = works as documented
	// dp_Arg3 = NULL (!?). (Should be DeviceNode)

	for (nr = 0; nr < MAX_FILESYSTEM_UNITS; nr++) {
		/* Hardfile volume name? */
		if (mountinfo.ui[nr].open <= 0)
			continue;
		if (!is_virtual (nr))
			continue;
		if (mountinfo.ui[nr].startup == arg2)
			break;
	}

	if (nr == MAX_FILESYSTEM_UNITS) {
		write_log (_T("Attempt to mount unknown filesystem device\n"));
		trap_put_long(ctx, pkt + dp_Res1, DOS_FALSE);
		trap_put_long(ctx, pkt + dp_Res2, ERROR_DEVICE_NOT_MOUNTED);
		return 0;
	}
	uinfo = mountinfo.ui + nr;
	//devnode = arg3 << 2;
	devnode = uinfo->devicenode;
	volume = trap_get_areg(ctx, 3) + 32;
	cdays = 3800 + nr;

	if (uinfo->unit_type == UNIT_CDFS) {
		ed = ef = 0;
		if (!mount_cd(uinfo, nr, &ctime, &uniq, volume)) {
			trap_put_long(ctx, pkt + dp_Res1, DOS_FALSE);
			trap_put_long(ctx, pkt + dp_Res2, ERROR_DEVICE_NOT_MOUNTED);
			return 0;
		}
	} else {
		ed = my_existsdir (uinfo->rootdir);
		ef = my_existsfile (uinfo->rootdir);
		if (!uinfo->wasisempty && !ef && !ed) {
			write_log (_T("Failed attempt to mount device '%s' (%s)\n"), uinfo->devname, uinfo->rootdir);
			trap_put_long(ctx, pkt + dp_Res1, DOS_FALSE);
			trap_put_long(ctx, pkt + dp_Res2, ERROR_DEVICE_NOT_MOUNTED);
			return 0;
		}
	}

	if (!uinfo->unit_pipe) {
		late = 1;
		filesys_start_thread(uinfo, nr);
	}
	unit = startup_create_unit(ctx, uinfo, nr);
	unit->volflags = uinfo->volflags;
	unit->rootnode.uniq_external = uniq;

	/*    write_comm_pipe_int (unit->ui.unit_pipe, -1, 1);*/

	write_log (_T("FS: %s (flags=%08X,E=%d,ED=%d,EF=%d,native='%s') starting..\n"),
		unit->ui.volname, unit->volflags, uinfo->wasisempty, ed, ef, unit->ui.rootdir);

	/* fill in our process in the device node */
	trap_put_long(ctx, devnode + 8, unit->port);
	unit->dosbase = trap_get_areg(ctx, 2);

	/* make new volume */
	unit->volume = volume;
	put_long(unit->volume + 180 - 32, devnode);
#ifdef UAE_FILESYS_THREADS
	unit->locklist = trap_get_areg(ctx, 3) + 8;
#else
	unit->locklist = trap_get_areg(ctx, 3);
#endif
	unit->dummy_message = trap_get_areg(ctx, 3) + 12;

	trap_put_long(ctx, unit->dummy_message + 10, 0);

	/* Prepare volume information */
	put_long(unit->volume + 4, 2); /* Type = dt_volume */
	put_long(unit->volume + 12, 0); /* Lock */
	put_long(unit->volume + 16, cdays); /* Creation Date */
	put_long(unit->volume + 20, 0);
	put_long(unit->volume + 24, 0);
	put_long(unit->volume + 28, 0); /* lock list */
	put_long(unit->volume + 40, (unit->volume + 64) >> 2); /* Name */
	//put_long(unit->volume + 44, 68);
	//put_long(unit->volume + 48, 68);

	put_byte(unit->volume + 64, 0);
	if (!uinfo->wasisempty && !uinfo->unknown_media) {
		int isvirtual = unit->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS);
		/* Set volume if non-empty */
		set_volume_name(unit, &ctime);
		if (!isvirtual)
			fsdb_clean_dir (&unit->rootnode);
	}

	put_long(unit->volume + 8, unit->port);
	/* not FFS because it is not understood by WB1.x C:Info */
	put_long(unit->volume + 32, DISK_TYPE_DOS);

	trap_put_long(ctx, pkt + dp_Res1, DOS_TRUE);

	return 1 | (late ? 2 : 0);
}

static bool is_writeprotected(Unit *unit)
{
	return unit->ui.readonly || unit->ui.locked || currprefs.harddrive_read_only;
}

static void	do_info(TrapContext *ctx, Unit *unit, dpacket *packet, uaecptr info, bool disk_info)
{
	struct fs_usage fsu;
	int ret, err = ERROR_NO_FREE_STORE;
	int blocksize, nr;
	uae_u32 dostype;
	bool fs = false, media = false;
	uae_u8 buf[36] =  { 0 }; // InfoData

	blocksize = 512;
	dostype = get_long(unit->volume + 32);
	nr = unit->unit;
	if (unit->volflags & MYVOLUMEINFO_ARCHIVE) {
		ret = zfile_fs_usage_archive (unit->ui.rootdir, 0, &fsu);
		fs = true;
		media = filesys_isvolume(unit) != 0;
	} else if (unit->volflags & MYVOLUMEINFO_CDFS) {
		struct isofs_info ii;
		ret = isofs_mediainfo (unit->ui.cdfs_superblock, &ii) ? 0 : 1;
		if (!ret) {
			media = ii.media;
			nr = unit->unit - cd_unit_offset;
			blocksize = ii.blocksize;
			if (ii.media) {
				fsu.total = ii.blocks * ii.blocksize;
				fsu.avail = 0;
			}
		}
	} else {
		ret = get_fs_usage (unit->ui.rootdir, 0, &fsu);
		if (ret)
			err = dos_errno ();
		fs = true;
		media = filesys_isvolume(unit) != 0;
	}
	if (ret != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	put_long_host(buf, 0); /* errors */
	put_long_host(buf + 4, nr); /* unit number */
	put_long_host(buf + 8, is_writeprotected(unit) ? 80 : 82); /* state  */
	put_long_host(buf + 20, blocksize); /* bytesperblock */
	put_long_host(buf + 32, 0); /* inuse */
	if (unit->ui.unknown_media) {
		if (!disk_info) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_NOT_A_DOS_DISK);
			return;
		}
		put_long_host(buf + 12, 0);
		put_long_host(buf + 16, 0);
		put_long_host(buf + 24, ('B' << 24) | ('A' << 16) | ('D' << 8) | (0 << 0)); /* ID_UNREADABLE_DISK */
		put_long_host(buf + 28, 0);
	} else if (!media) {
		if (!disk_info) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_NO_DISK);
			return;
		}
		put_long_host(buf + 12, 0);
		put_long_host(buf + 16, 0);
		put_long_host(buf + 24, -1); /* ID_NO_DISK_PRESENT */
		put_long_host(buf + 28, 0);
	} else {
		uae_s64 numblocks, inuse;
		get_usedblocks(&fsu, fs, &blocksize, &numblocks, &inuse, true);
		//write_log(_T("total %lld avail %lld Blocks %lld Inuse %lld blocksize %d\n"), fsu.total, fsu.avail, numblocks, inuse, blocksize);
		put_long_host(buf + 12, (uae_u32)numblocks); /* numblocks */
		put_long_host(buf + 16, (uae_u32)inuse); /* inuse */
		put_long_host(buf + 20, blocksize); /* bytesperblock */
		put_long_host(buf + 24, dostype == DISK_TYPE_DOS && kickstart_version >= 36 ? DISK_TYPE_DOS_FFS : dostype); /* disk type */
		put_long_host(buf + 28, unit->volume >> 2); /* volume node */
		put_long_host(buf + 32, (get_long(unit->volume + 28) || unit->keys) ? -1 : 0); /* inuse */
	}
	trap_put_bytes(ctx, buf, info, sizeof buf);
	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void action_disk_info(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	TRACE((_T("ACTION_DISK_INFO\n")));
	do_info(ctx, unit, packet, GET_PCK_ARG1 (packet) << 2, true);
}

static void action_info(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	TRACE((_T("ACTION_INFO\n")));
	do_info(ctx, unit, packet, GET_PCK_ARG2 (packet) << 2, false);
}

static void free_key (Unit *unit, Key *k)
{
	Key *k1;
	Key *prev = 0;
	for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k == k1) {
			if (prev)
				prev->next = k->next;
			else
				unit->keys = k->next;
			break;
		}
		prev = k1;
	}

	for (struct lockrecord *lr = k->record; lr;) {
		struct lockrecord *next = lr->next;
		xfree (lr);
		lr = next;
	}

	if (k->fd != NULL)
		fs_closefile (k->fd);

	xfree(k);
}

static Key *lookup_key (Unit *unit, uae_u32 uniq)
{
	Key *k;
	unsigned int total = 0;
	/* It's hardly worthwhile to optimize this - most of the time there are
	* only one or zero keys. */
	for (k = unit->keys; k; k = k->next) {
		total++;
		if (uniq == k->uniq)
			return k;
	}
	write_log (_T("Error: couldn't find key %u / %u!\n"), uniq, total);
	return 0;
}

static Key *new_key (Unit *unit)
{
	Key *k = xcalloc (Key, 1);
	k->uniq = ++key_uniq;
	k->fd = NULL;
	k->file_pos = 0;
	k->next = unit->keys;
	unit->keys = k;

	return k;
}

#if TRACING_ENABLED
static void dumplock(TrapContext *ctx, Unit *unit, uaecptr lock)
{
	a_inode *a;
	TRACE((_T("LOCK: 0x%x"), lock));
	if (!lock) {
		TRACE((_T("\n")));
		return;
	}
	TRACE((_T("{ next=0x%x, mode=%d, handler=0x%x, volume=0x%x, aino %x "),
		trap_get_long(ctx, lock) << 2, trap_get_long(ctx, lock + 8),
		trap_get_long(ctx, lock + 12), trap_get_long(ctx, lock + 16),
		trap_get_long(ctx, lock + 4)));
	if (log_filesys > 0) {
		a = aino_from_lock(ctx, unit, lock);
		if (a == 0) {
			TRACE((_T("not found!")));
		} else {
			TRACE((_T("%s"), a->nname));
		}
	}
	TRACE((_T(" }\n")));
}
#endif

static a_inode *find_aino(TrapContext* ctx, Unit *unit, uaecptr lock, const TCHAR *name, int *err)
{
	a_inode *a;

	if (lock) {
		a_inode *olda = aino_from_lock(ctx, unit, lock);
		if (olda == 0) {
			/* That's the best we can hope to do. */
			a = get_aino (unit, &unit->rootnode, name, err);
		} else {
			TRACE((_T("aino: 0x%08lx"), (unsigned long int)olda->uniq));
			TRACE((_T(" \"%s\"\n"), olda->nname));
			a = get_aino (unit, olda, name, err);
		}
	} else {
		a = get_aino (unit, &unit->rootnode, name, err);
	}
	if (a) {
		TRACE((_T("aino=\"%s\"\n"), a->nname));
	}
	aino_test (a);
	return a;
}

static uaecptr make_lock(TrapContext *ctx, Unit *unit, uae_u32 uniq, uae_u32 mode)
{
	/* allocate lock from the list kept by the assembly code */
	uaecptr lock;

#if TRAPMD

	struct trapmd md1[] = 
	{
		{ TRAPCMD_GET_LONG, { unit->locklist }, 1, 0 },
		{ TRAPCMD_GET_LONG, { 0 }, 2, 1 },
		{ TRAPCMD_PUT_LONG, { unit->locklist, 0 } },
	};
	trap_multi(ctx, md1, sizeof md1 / sizeof(struct trapmd));
	lock = md1[0].params[0] + 4;

#else

	lock = trap_get_long(ctx, unit->locklist);
	trap_put_long(ctx, unit->locklist, trap_get_long(ctx, lock));
	lock += 4;

#endif

#if TRAPMD
	struct trapmd md2[] =
	{
		{ TRAPCMD_PUT_LONG, { lock + 4, uniq } },
		{ TRAPCMD_PUT_LONG, { lock + 8, mode } },
		{ TRAPCMD_PUT_LONG, { lock + 12, unit->port } },
		{ TRAPCMD_PUT_LONG, { lock + 16, unit->volume >> 2 } },

		/* prepend to lock chain */
		{ TRAPCMD_GET_LONG, { unit->volume + 28 }, 5, 1 },
		{ TRAPCMD_PUT_LONG, { lock, 0 } },
		{ TRAPCMD_PUT_LONG, { unit->volume + 28, lock >> 2 } }
	};
	trap_multi(ctx, md2, sizeof md2 / sizeof(struct trapmd));

#else

	trap_put_long(ctx, lock + 4, uniq);
	trap_put_long(ctx, lock + 8, mode);
	trap_put_long(ctx, lock + 12, unit->port);
	trap_put_long(ctx, lock + 16, unit->volume >> 2);

	/* prepend to lock chain */
	trap_put_long(ctx, lock, trap_get_long(ctx, unit->volume + 28));
	trap_put_long(ctx, unit->volume + 28, lock >> 2);

#endif

	DUMPLOCK(ctx, unit, lock);
	return lock;
}

#define NOTIFY_CLASS	0x40000000
#define NOTIFY_CODE	0x1234

#define NRF_SEND_MESSAGE 1
#define NRF_SEND_SIGNAL 2
#define NRF_WAIT_REPLY 8
#define NRF_NOTIFY_INITIAL 16
#define NRF_MAGIC (1 << 31)

static void notify_send(TrapContext *ctx, Unit *unit, Notify *n)
{
	uaecptr nr = n->notifyrequest;
	int flags = trap_get_long(ctx, nr + 12);

	if (flags & NRF_SEND_MESSAGE) {
		if (!(flags & NRF_WAIT_REPLY) || ((flags & NRF_WAIT_REPLY) && !(flags & NRF_MAGIC))) {
			uae_NotificationHack (unit->port, nr);
		} else if (flags & NRF_WAIT_REPLY) {
			trap_put_long(ctx, nr + 12, trap_get_long(ctx, nr + 12) | NRF_MAGIC);
		}
	} else if (flags & NRF_SEND_SIGNAL) {
		uae_Signal (trap_get_long(ctx, nr + 16), 1 << trap_get_byte(ctx, nr + 20));
	}
}

static void notify_check(TrapContext *ctx, Unit *unit, a_inode *a)
{
	Notify *n;
	int hash = notifyhash (a->aname);
	for (n = unit->notifyhash[hash]; n; n = n->next) {
		uaecptr nr = n->notifyrequest;
		if (same_aname (n->partname, a->aname)) {
			int err;
			a_inode *a2 = find_aino(ctx, unit, 0, n->fullname, &err);
			if (err == 0 && a == a2)
				notify_send(ctx, unit, n);
		}
	}
	if (a->parent) {
		hash = notifyhash (a->parent->aname);
		for (n = unit->notifyhash[hash]; n; n = n->next) {
			uaecptr nr = n->notifyrequest;
			if (same_aname (n->partname, a->parent->aname)) {
				int err;
				a_inode *a2 = find_aino(ctx, unit, 0, n->fullname, &err);
				if (err == 0 && a->parent == a2)
					notify_send(ctx, unit, n);
			}
		}
	}
}

static void action_add_notify(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr nr = GET_PCK_ARG1 (packet);
	int flags;
	Notify *n;
	TCHAR *name, *p, *partname;

	TRACE((_T("ACTION_ADD_NOTIFY\n")));

	name = char1(ctx, trap_get_long(ctx, nr + 4));
	flags = trap_get_long(ctx, nr + 12);

	if (!(flags & (NRF_SEND_MESSAGE | NRF_SEND_SIGNAL))) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_BAD_NUMBER);
		return;
	}

#if 0
	write_log (_T("Notify:\n"));
	write_log (_T("nr_Name '%s'\n"), char1 (trap_get_long(ctx, nr + 0)));
	write_log (_T("nr_FullName '%s'\n"), name);
	write_log (_T("nr_UserData %08X\n"), trap_get_long(ctx, nr + 8));
	write_log (_T("nr_Flags %08X\n"), flags);
	if (flags & NRF_SEND_MESSAGE) {
		write_log (_T("Message NotifyRequest, port = %08X\n"), trap_get_long(ctx, nr + 16));
	} else if (flags & NRF_SEND_SIGNAL) {
		write_log (_T("Signal NotifyRequest, Task = %08X signal = %d\n"), trap_get_long(ctx, nr + 16), trap_get_long(ctx, nr + 20));
	} else {
		write_log (_T("corrupt NotifyRequest\n"));
	}
#endif

	p = name + uaetcslen (name) - 1;
	if (p[0] == ':')
		p--;
	while (p > name && p[0] != ':' && p[0] != '/')
		p--;
	if (p[0] == ':' || p[0] == '/')
		p++;
	partname = my_strdup (p);
	n = new_notify (unit, partname);
	n->notifyrequest = nr;
	n->fullname = name;
	if (flags & NRF_NOTIFY_INITIAL) {
		int err;
		a_inode *a = find_aino(ctx, unit, 0, n->fullname, &err);
		if (err == 0)
			notify_send(ctx, unit, n);
	}
	PUT_PCK_RES1 (packet, DOS_TRUE);
}
static void	action_remove_notify(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr nr = GET_PCK_ARG1 (packet);
	Notify *n;
	int hash;

	TRACE((_T("ACTION_REMOVE_NOTIFY\n")));
	for (hash = 0; hash < NOTIFY_HASH_SIZE; hash++) {
		for (n = unit->notifyhash[hash]; n; n = n->next) {
			if (n->notifyrequest == nr) {
				//write_log (_T("NotifyRequest %08X freed\n"), n->notifyrequest);
				xfree (n->fullname);
				xfree (n->partname);
				free_notify (unit, hash, n);
				PUT_PCK_RES1 (packet, DOS_TRUE);
				return;
			}
		}
	}
	write_log (_T("Tried to free non-existing NotifyRequest %08X\n"), nr);
	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void free_lock(TrapContext *ctx, Unit *unit, uaecptr lock)
{
	if (! lock)
		return;

	uaecptr curlock = get_long(unit->volume + 28);
	if (lock == curlock << 2) {
		put_long(unit->volume + 28, trap_get_long(ctx, lock));
	} else {
		uaecptr current = curlock;
		uaecptr next = 0;
		while (current) {
			next = trap_get_long(ctx, current << 2);
			if (lock == next << 2)
				break;
			current = next;
		}
		if (!current) {
			write_log (_T("tried to unlock non-existing lock %x\n"), lock);
			return;
		}
		trap_put_long(ctx, current << 2, trap_get_long(ctx, lock));
	}
	lock -= 4;
#if TRAPMD
	struct trapmd md2[] =
	{
		{ TRAPCMD_GET_LONG, { unit->locklist }, 1, 1 },
		{ TRAPCMD_PUT_LONG, { lock } },
		{ TRAPCMD_PUT_LONG, { unit->locklist, lock } }
	};
	trap_multi(ctx, md2, sizeof md2 / sizeof(struct trapmd));
#else
	trap_put_long(ctx, lock, trap_get_long(ctx, unit->locklist));
	trap_put_long(ctx, unit->locklist, lock);
#endif
}

static void action_lock(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr name = GET_PCK_ARG2 (packet) << 2;
	int mode = GET_PCK_ARG3 (packet);
	a_inode *a;
	int err;

	if (mode != SHARED_LOCK && mode != EXCLUSIVE_LOCK) {
		TRACE((_T("Bad mode %d (should be %d or %d).\n"), mode, SHARED_LOCK, EXCLUSIVE_LOCK));
		mode = SHARED_LOCK;
	}

	TRACE((_T("ACTION_LOCK(0x%08x, \"%s\", %d)\n"), lock, bstr(ctx, unit, name), mode));
	DUMPLOCK(ctx, unit, lock);

	a = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);
	if (err == 0 && a->softlink) {
		err = test_softlink (a);
	}
	if (err == 0 && (a->elock || (mode != SHARED_LOCK && a->shlock > 0))) {
		err = ERROR_OBJECT_IN_USE;
	}
	/* Lock() doesn't do access checks. */
	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	if (mode == SHARED_LOCK)
		a->shlock++;
	else
		a->elock = 1;
	de_recycle_aino (unit, a);
	PUT_PCK_RES1(packet, make_lock(ctx, unit, a->uniq, mode) >> 2);
}
static void action_read_link_add_parent(Unit *u, a_inode *a, TCHAR *path)
{
	if (a == NULL)
		return;
	action_read_link_add_parent(u, a->parent, path);
	if (a == &u->rootnode) {
		_tcscat (path, a->aname);
		_tcscat (path, _T(":"));
	} else {
		if (path[0] && path[uaetcslen (path) - 1] != ':')
			_tcscat (path, _T("/"));
		_tcscat (path, a->aname);
	}
}

#define LINK_HARD 0
#define LINK_SOFT 1

static void action_make_link(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr name = GET_PCK_ARG2 (packet) << 2;
	uaecptr target = GET_PCK_ARG3 (packet);
	int type = GET_PCK_ARG4 (packet);
	a_inode *a1, *a2;
	int err;
	TCHAR tmp[256], tmp2[MAX_DPATH], tmp3[MAX_DPATH];

	_tcscpy (tmp, bstr(ctx, unit, name));
	a1 = aino_from_lock(ctx, unit, lock);

	if (type == LINK_HARD) {

		// we don't support hard links
		uaecptr tlock = target << 2;
		a2 = aino_from_lock(ctx, unit, tlock);
		write_log (_T("ACTION_MAKE_LINK(HARD,'%s','%s','%s')\n"),
			a1 ? a1->aname : _T("?"), tmp,
			a2 ? a2->aname : _T("?"));

		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_NOT_IMPLEMENTED);

	} else {

		a_inode *a3;
		TCHAR *link = cstr(ctx, unit, target);
		write_log (_T("ACTION_MAKE_LINK(SOFT,'%s','%s','%s')\n"),
			a1 ? a1->aname : _T("?"), tmp, link);
		if (!a1) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
			return;
		}
		// try to find softlink target
		for (Unit *u = units; u; u = u->next) {
			if (u->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS))
				continue;
			a3 = find_aino(ctx, u, 0, link, &err);
			if (err || !a3)
				continue;
			_tcscpy (tmp2, a1->nname);
			_tcscat (tmp2, FSDB_DIR_SEPARATOR_S);
			_tcscat (tmp2, tmp);
			tmp3[0] = 0;
			action_read_link_add_parent (u, a3, tmp3);
			if (!my_createshortcut (tmp2, a3->nname, tmp3)) {
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, dos_errno ());
			}
			return;
		}
		// real Amiga softlinks would accept invalid paths too, we won't.
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
	}
}

static void action_read_link(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr name = GET_PCK_ARG2 (packet);
	uaecptr newname = GET_PCK_ARG3 (packet);
	int size = GET_PCK_ARG4 (packet);
	a_inode *a, *matched_aino;
	Unit *u = NULL, *u2 = NULL, *matched_unit;
	int err, i, matched_len;
	TCHAR tmp[MAX_DPATH];
	TCHAR *namep, *extrapath;

	extrapath = NULL;
	namep = cstr(ctx, unit, name);
	for (;;) {
		a = find_aino(ctx, unit, lock, namep, &err);
		if (err != ERROR_IS_SOFT_LINK)
			break;
		for (i = uaetcslen (namep) - 1; i > 0; i--) {
			if (namep[i] == '/') {
				namep[i] = 0;
				xfree (extrapath);
				extrapath = my_strdup (namep + i + 1);
				break;
			}
		}
	}
	if (!a->softlink)
		err = ERROR_OBJECT_WRONG_TYPE;
	if (err != 0) {
		xfree(extrapath);
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	_tcscpy (tmp, a->nname);
	write_log (_T("Resolving softlink '%s'\n"), tmp);
	if (!my_resolvesoftlink (tmp, sizeof tmp / sizeof (TCHAR), false)) {
		xfree(extrapath);
		PUT_PCK_RES1 (packet, DOS_FALSE);
		//  not sure what to return
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}
	write_log (_T("-> '%s'\n"), tmp);
	matched_aino = NULL;
	matched_unit = NULL;
	err = 0;
	matched_len = 0;
	for (u = units; u; u = u->next) {
		if (!(u->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS))) {
			TCHAR path[MAX_DPATH];
			i = my_issamevolume (u->rootnode.nname, tmp, path);
			if (i > matched_len) {
				a = find_aino(ctx, u, 0, path, &err);
				if (a && !err) {
					write_log (_T("Match found from '%s' (%d)\n"), u->rootnode.aname, i);
					matched_aino = a;
					matched_unit = u;
					matched_len = i;
				}
			}
		}
	}
	if (!matched_aino) {
		xfree(extrapath);
		write_log (_T("Path not found in any mounted drive\n"));
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}
	tmp[0] = 0;
	action_read_link_add_parent (matched_unit, matched_aino, tmp);
	if (extrapath) {
		_tcscat (tmp, _T("/"));
		_tcscat (tmp, extrapath);
	}
	xfree (extrapath);
	write_log (_T("got target '%s'\n"), tmp); 
	char *s = ua_fs (tmp, -1);
	for (i = 0; s[i]; i++) {
		if (i >= size - 1)
			break;
		trap_put_byte(ctx, newname + i, s[i]);
	}
	xfree (s);
	trap_put_byte(ctx, newname + i, 0);
}

static void action_free_lock(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	a_inode *a;
	TRACE((_T("ACTION_FREE_LOCK(0x%x)\n"), lock));
	DUMPLOCK(ctx, unit, lock);

	a = aino_from_lock(ctx, unit, lock);
	if (a == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}
	if (a->elock)
		a->elock = 0;
	else
		a->shlock--;
	recycle_aino (unit, a);
	free_lock(ctx, unit, lock);

	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static uaecptr action_dup_lock_2(TrapContext *ctx, Unit *unit, dpacket *packet, uae_u32 uniq)
{
	uaecptr out;
	a_inode *a;

	a = lookup_aino (unit, uniq);
	if (a == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return 0;
	}
	/* DupLock()ing exclusive locks isn't possible, says the Autodoc, but
	* at least the RAM-Handler seems to allow it. Let's see what happens
	* if we don't. */
	if (a->elock) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
		return 0;
	}
	a->shlock++;
	de_recycle_aino(unit, a);
	out = make_lock(ctx, unit, a->uniq, -2) >> 2;
	PUT_PCK_RES1 (packet, out);
	return out;
}

static void action_dup_lock(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	TRACE((_T("ACTION_DUP_LOCK(0x%x)\n"), lock));
	if (!lock) {
		PUT_PCK_RES1 (packet, 0);
		return;
	}
	action_dup_lock_2(ctx, unit, packet, trap_get_long(ctx, lock + 4));
}


static void action_lock_from_fh(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	TRACE((_T("ACTION_COPY_DIR_FH(0x%x,'%s')\n"), GET_PCK_ARG1 (packet), k ? k->aino->aname : _T("<null>")));
	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		return;
	}
	action_dup_lock_2(ctx, unit, packet, k->aino->uniq);
}

static void free_exkey (Unit *unit, a_inode *aino)
{
	if (--aino->exnext_count == 0) {
		TRACE ((_T("Freeing ExKey and reducing total_locked from %d by %d\n"),
			unit->total_locked_ainos, aino->locked_children));
		unit->total_locked_ainos -= aino->locked_children;
		aino->locked_children = 0;
	}
}

static void move_exkeys (Unit *unit, a_inode *from, a_inode *to)
{
	to->exnext_count = from->exnext_count;
	to->locked_children = from->locked_children;
	from->exnext_count = 0;
	from->locked_children = 0;
}

static bool get_statinfo(Unit *unit, a_inode *aino, struct mystat *statbuf)
{
	bool ok = true;
	memset (statbuf, 0, sizeof(struct mystat));
	/* No error checks - this had better work. */
	if (unit->volflags & MYVOLUMEINFO_ARCHIVE)
		ok = zfile_stat_archive (aino->nname, statbuf) != 0;
	else if (unit->volflags & MYVOLUMEINFO_CDFS)
		ok = isofs_stat (unit->ui.cdfs_superblock, aino->uniq_external, statbuf);
	else
		my_stat (aino->nname, statbuf);
	return ok;
}

static void get_fileinfo(TrapContext *ctx, Unit *unit, dpacket *packet, uaecptr info, a_inode *aino, bool longfilesize)
{
	struct mystat statbuf;
	int days, mins, ticks;
	int i, n, entrytype, blocksize;
	uae_s64 numblocks;
	int fsdb_can = fsdb_cando (unit);
	const TCHAR *xs;
	char *x, *x2;
	uae_u8 *buf;
	uae_u8 buf_array[260] = { 0 };

	if (trap_is_indirect() || !valid_address(info, (sizeof buf_array) - 36) || !real_address_allowed()) {
		buf = buf_array;
	} else {
		buf = get_real_address(info);
	}

	if (aino->vfso) {
		fsdb_can = 1;
		statbuf.mode = aino->vfso->amigaos_mode;
		statbuf.mtime.tv_sec = 0;
		statbuf.mtime.tv_usec = 0;
		statbuf.size = aino->vfso->size;
	} else if (!get_statinfo(unit, aino, &statbuf)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_NOT_A_DOS_DISK);
		return;
	}

	put_long_host(buf + 0, aino->uniq);
	if (aino->parent == 0) {
		/* Guru book says ST_ROOT = 1 (root directory, not currently used)
		* but some programs really expect 2 from root dir..
		*/
		entrytype = ST_USERDIR;
		xs = unit->ui.volname;
	} else {
		entrytype = aino->softlink ? ST_SOFTLINK : (aino->dir ? ST_USERDIR : ST_FILE);
		xs = aino->aname;
	}
	put_long_host(buf + 4, entrytype);
	/* AmigaOS docs say these have to contain the same value. */
	put_long_host(buf + 120, entrytype);

	TRACE((_T("name=\"%s\"\n"), xs));
	x2 = x = ua_fs (xs, -1);
	n = uaestrlen(x);
	if (n > 107)
		n = 107;
	if (n > abs(currprefs.filesys_max_name))
		n = abs(currprefs.filesys_max_name);
	i = 8;
	put_byte_host(buf + i, n); i++;
	while (n--)
		put_byte_host(buf + i, *x), i++, x++;
	while (i < 108)
		put_byte_host(buf + i, 0), i++;
	xfree (x2);

	put_long_host(buf + 116, fsdb_can ? aino->amigaos_mode : fsdb_mode_supported (aino));

	if (kickstart_version >= 36) {
		put_word_host(buf + 224, 0); // OwnerUID
		put_word_host(buf + 226, 0); // OwnerGID
	}

	blocksize = (unit->volflags & MYVOLUMEINFO_CDFS) ? 2048 : 512;
	numblocks = (statbuf.size + blocksize - 1) / blocksize;
	put_long_host(buf + 128, (uae_u32)(numblocks > MAXFILESIZE32 ? MAXFILESIZE32 : numblocks));

	if (longfilesize) {
		/* MorphOS 64-bit file length support */
		put_long_host(buf + 124, statbuf.size > MAXFILESIZE32_2G ? 0 : (uae_u32)statbuf.size);
		put_long_host(buf + 228, statbuf.size >> 32);
		put_long_host(buf + 232, (uae_u32)statbuf.size);
		put_long_host(buf + 236, numblocks >> 32);
		put_long_host(buf + 240, (uae_u32)numblocks);
	} else {
		put_long_host(buf + 124, statbuf.size > MAXFILESIZE32 ? MAXFILESIZE32 : (uae_u32)statbuf.size);
	}

	timeval_to_amiga (&statbuf.mtime, &days, &mins, &ticks, 50);
	put_long_host(buf + 132, days);
	put_long_host(buf + 136, mins);
	put_long_host(buf + 140, ticks);
	if (aino->comment == 0 || !fsdb_can)
		put_long_host(buf + 144, 0);
	else {
		TRACE((_T("comment=\"%s\"\n"), aino->comment));
		i = 144;
		xs = aino->comment;
		if (!xs)
			xs= _T("");
		x2 = x = ua_fs(xs, -1);
		n = uaestrlen(x);
		if (n > 78)
			n = 78;
		put_byte_host(buf + i, n); i++;
		while (n--)
			put_byte_host(buf + i, *x), i++, x++;
		while (i < 224)
			put_byte_host(buf + i, 0), i++;
		xfree (x2);
	}

	if (buf == buf_array) {
		// Must not write Fib_reserved at the end.
		if (kickstart_version >= 36) {
			// FIB + fib_OwnerUID and fib_OwnerGID
			trap_put_bytes(ctx, buf, info, (sizeof buf_array) - 32);
		} else {
			// FIB only
			trap_put_bytes(ctx, buf, info, (sizeof buf_array) - 36);
		}
	}

	PUT_PCK_RES1 (packet, DOS_TRUE);
}

int get_native_path(TrapContext *ctx, uae_u32 lock, TCHAR *out)
{
	int i = 0;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (mountinfo.ui[i].self) {
			a_inode *a = aino_from_lock(ctx, mountinfo.ui[i].self, lock << 2);
			if (a) {
				_tcscpy (out, a->nname);
				return 0;
			}
		}
	}
	return -1;
}

#define REC_EXCLUSIVE 0
#define REC_EXCLUSIVE_IMMED 1
#define REC_SHARED 2
#define REC_SHARED_IMMED 3

static struct lockrecord *new_record (dpacket *packet, uae_u64 pos, uae_u64 len, uae_u32 mode, uae_u32 timeout, uae_u32 msg)
{
	struct lockrecord *lr = xcalloc (struct lockrecord, 1);
	lr->packet = packet;
	lr->pos = pos;
	lr->len = len;
	lr->mode = mode;
	lr->timeout = (uae_u32)(timeout * vblank_hz / 50);
	lr->msg = msg;
	return lr;
}

static bool record_hit (Unit *unit, Key *k, uae_u64 pos, uae_u64 len, uae_u32 mode)
{
	bool exclusive = mode == REC_EXCLUSIVE || mode == REC_EXCLUSIVE_IMMED;
	for (Key *k2 = unit->keys; k2; k2 = k2->next) {
		if (k2->aino->uniq == k->aino->uniq) {
			if (k2 == k)
				continue;
			for (struct lockrecord *lr = k2->record; lr; lr = lr->next) {
				bool exclusive2 = lr->mode == REC_EXCLUSIVE || lr->mode == REC_EXCLUSIVE_IMMED;
				if (exclusive || exclusive2) {
					uae_u64 a1 = pos;
					uae_u64 a2 = pos + len;
					uae_u64 b1 = lr->pos;
					uae_u64 b2 = lr->pos + lr->len;
					if (len && lr->len) {
						bool hit = (a1 >= b1 && a1 < b2) || (a2 > b1 && a2 < b2) || (b1 >= a1 && b1 < a2) || (b2 > a1 && b2 < a2);
						if (hit)
							return true;
					}
				}
			}
		}
	}
	return false;
}

static void record_timeout(TrapContext *ctx, Unit *unit)
{
	bool retry = true;
	while (retry) {
		retry = false;
		struct lockrecord *prev = NULL;
		for (struct lockrecord *lr = unit->waitingrecords; lr; lr = lr->next) {
			lr->timeout--;
			if (lr->timeout == 0) {
				Key *k = lookup_key (unit, GET_PCK_ARG1 (lr->packet));
				PUT_PCK_RES1 (lr->packet, DOS_FALSE);
				PUT_PCK_RES2 (lr->packet, ERROR_LOCK_TIMEOUT);
				// mark packet as complete
				trap_put_long(ctx, lr->msg + 4, 0xfffffffe);
				uae_Signal (get_long(unit->volume + 176 - 32), 1 << 13);
				if (prev)
					prev->next = lr->next;
				else
					unit->waitingrecords = lr->next;
				write_log (_T("queued record timed out '%s',%lld,%lld,%d,%d\n"), k ? k->aino->nname : _T("NULL"), lr->pos, lr->len, lr->mode, lr->timeout);
				xfree (lr);
				retry = true;
				break;
			}
			prev = lr;
		}
	}
}

static void record_check_waiting(TrapContext *ctx, Unit *unit)
{
	bool retry = true;
	while (retry) {
		retry = false;
		struct lockrecord *prev = NULL;
		for (struct lockrecord *lr = unit->waitingrecords; lr; lr = lr->next) {
			Key *k = lookup_key (unit, GET_PCK_ARG1 (lr->packet));
			if (!k || !record_hit (unit, k, lr->pos, lr->len, lr->mode)) {
				if (prev)
					prev->next = lr->next;
				else
					unit->waitingrecords = lr->next;
				write_log (_T("queued record released '%s',%llud,%llu,%d,%d\n"), k->aino->nname, lr->pos, lr->len, lr->mode, lr->timeout);
				// mark packet as complete
				trap_put_long(ctx, lr->msg + 4, 0xffffffff);
				xfree (lr);
				retry = true;
				break;
			}
			prev = lr;
		}
	}
}

static int action_lock_record(TrapContext *ctx, Unit *unit, dpacket *packet, uae_u32 msg)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uae_u32 pos = GET_PCK_ARG2 (packet);
	uae_u32 len = GET_PCK_ARG3 (packet);
	uae_u32 mode = GET_PCK_ARG4 (packet);
	uae_u32 timeout = GET_PCK_ARG5 (packet);

	bool exclusive = mode == REC_EXCLUSIVE || mode == REC_EXCLUSIVE_IMMED;

	write_log (_T("action_lock_record('%s',%d,%d,%d,%d)\n"), k ? k->aino->nname : _T("null"), pos, len, mode, timeout);

	if (!k || mode > REC_SHARED_IMMED) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_WRONG_TYPE);
		return 1;
	}

	if (mode == REC_EXCLUSIVE_IMMED || mode == REC_SHARED_IMMED)
		timeout = 0;

	if (record_hit (unit, k, pos, len, mode)) {
		if (timeout && msg) {
			// queue it and do not reply
			struct lockrecord *lr = new_record (packet, pos, len, mode, timeout, msg);
			if (unit->waitingrecords) {
				lr->next = unit->waitingrecords;
				unit->waitingrecords = lr;
			} else {
				unit->waitingrecords = lr;
			}
			write_log (_T("-> collision, timeout queued\n"));
			return -1;
		}
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_LOCK_COLLISION);
		write_log (_T("-> ERROR_LOCK_COLLISION\n"));
		return 1;
	}

	struct lockrecord *lr = new_record (packet, pos, len, mode, timeout, 0);
	if (k->record) {
		lr->next = k->record;
		k->record = lr;
	} else {
		k->record = lr;
	}
	PUT_PCK_RES1 (packet, DOS_TRUE);
	write_log (_T("-> OK\n"));
	return 1;
}

static void action_free_record(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uae_u32 pos = GET_PCK_ARG2 (packet);
	uae_u32 len = GET_PCK_ARG3 (packet);

	write_log (_T("action_free_record('%s',%d,%d)\n"), k ? k->aino->nname : _T("null"), pos, len);

	if (!k) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_WRONG_TYPE);
		return;
	}

	struct lockrecord *prev = NULL;
	for (struct lockrecord *lr = k->record; lr; lr = lr->next) {
		if (lr->pos == pos && lr->len == len) {
			if (prev)
				prev->next = lr->next;
			else
				k->record = lr->next;
			xfree (lr);
			write_log (_T("->OK\n"));
			record_check_waiting(ctx, unit);
			PUT_PCK_RES1 (packet, DOS_TRUE);
			return;
		}
	}
	write_log (_T("-> ERROR_RECORD_NOT_LOCKED\n"));
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_RECORD_NOT_LOCKED);
}

#define EXALL_DEBUG 0
#define EXALL_END 0xde1111ad

static ExAllKey *getexall (Unit *unit, uaecptr control, int id)
{
	int i;
	if (id < 0) {
		for (i = 0; i < EXALLKEYS; i++) {
			if (unit->exalls[i].id == 0) {
				unit->exallid++;
				if (unit->exallid == EXALL_END)
					unit->exallid++;
				unit->exalls[i].id = unit->exallid;
				unit->exalls[i].control = control;
				return &unit->exalls[i];
			}
		}
	} else if (id > 0) {
		for (i = 0; i < EXALLKEYS; i++) {
			if (unit->exalls[i].id == id)
				return &unit->exalls[i];
		}
	}
	return NULL;
}

static int exalldo(TrapContext *ctx, uaecptr exalldata, uae_u32 exalldatasize, uae_u32 type, uaecptr control, Unit *unit, a_inode *aino)
{
	uaecptr exp = exalldata;
	int i;
	int size, size2;
	int entrytype;
	const TCHAR *xs = NULL, *commentx = NULL;
	uae_u32 flags = 15;
	int days, mins, ticks;
	struct mystat statbuf;
	int fsdb_can = fsdb_cando (unit);
	uae_u16 uid = 0, gid = 0;
	char *x = NULL, *comment = NULL;
	int ret = 0;

	memset (&statbuf, 0, sizeof statbuf);
	if (unit->volflags & MYVOLUMEINFO_ARCHIVE)
		zfile_stat_archive (aino->nname, &statbuf);
	else if (unit->volflags & MYVOLUMEINFO_CDFS)
		isofs_stat (unit->ui.cdfs_superblock, aino->uniq_external, &statbuf);
	else
		my_stat (aino->nname, &statbuf);

	if (aino->parent == 0) {
		entrytype = ST_USERDIR;
		xs = unit->ui.volname;
	} else {
		entrytype = aino->softlink ? ST_SOFTLINK : (aino->dir ? ST_USERDIR : ST_FILE);
		xs = aino->aname;
	}
	x = ua_fs (xs, -1);

	size = 0;
	size2 = 4;
	if (type >= 1) {
		size2 += 4;
		size += uaestrlen(x) + 1;
		size = (size + 3) & ~3;
	}
	if (type >= 2)
		size2 += 4;
	if (type >= 3)
		size2 += 4;
	if (type >= 4) {
		flags = fsdb_can ? aino->amigaos_mode : fsdb_mode_supported (aino);
		size2 += 4;
	}
	if (type >= 5) {
		timeval_to_amiga (&statbuf.mtime, &days, &mins, &ticks, 50);
		size2 += 12;
	}
	if (type >= 6) {
		size2 += 4;
		if (aino->comment == 0 || !fsdb_can)
			commentx = _T("");
		else
			commentx = aino->comment;
		comment = ua_fs(commentx, -1);
		size += uaestrlen(comment) + 1;
		size = (size + 3) & ~3;
	}
	if (type >= 7) {
		size2 += 4;
		uid = 0;
		gid = 0;
	}
	if (type >= 8) {
		size2 += 8;
	}

	i = trap_get_long(ctx, control + 0);
	while (i > 0) {
		exp = trap_get_long(ctx, exp); /* ed_Next */
		i--;
	}

	if (exalldata + exalldatasize - exp < size + size2)
		goto end; /* not enough space */

#if EXALL_DEBUG > 0
	write_log (_T("ID=%d, %d, %08x: '%s'%s\n"),
		trap_get_long(ctx, control + 4), trap_get_long(ctx, control + 0), exp, xs, aino->dir ? _T(" [DIR]") : _T(""));
#endif

	trap_put_long(ctx, exp, exp + size + size2); /* ed_Next */
	if (type >= 1) {
		trap_put_long(ctx, exp + 4, exp + size2);
		for (i = 0; i <= strlen (x); i++) {
			trap_put_byte(ctx, exp + size2, x[i]);
			size2++;
		}
	}
	if (type >= 2)
		trap_put_long(ctx, exp + 8, entrytype);
	if (type >= 3)
		trap_put_long(ctx, exp + 12, (uae_u32)(statbuf.size > MAXFILESIZE32 ? MAXFILESIZE32 : statbuf.size));
	if (type >= 4)
		trap_put_long(ctx, exp + 16, flags);
	if (type >= 5) {
		trap_put_long(ctx, exp + 20, days);
		trap_put_long(ctx, exp + 24, mins);
		trap_put_long(ctx, exp + 28, ticks);
	}
	if (type >= 6) {
		trap_put_long(ctx, exp + 32, exp + size2);
		trap_put_byte(ctx, exp + size2, (uae_u8)strlen(comment));
		for (i = 0; i <= strlen (comment); i++) {
			trap_put_byte(ctx, exp + size2, comment[i]);
			size2++;
		}
	}
	if (type >= 7) {
		trap_put_word(ctx, exp + 36, uid);
		trap_put_word(ctx, exp + 38, gid);
	}
	if (type >= 8) {
		trap_put_long(ctx, exp + 40, statbuf.size >> 32);
		trap_put_long(ctx, exp + 44, (uae_u32)statbuf.size);
	}

	trap_put_long(ctx, control + 0, trap_get_long(ctx, control + 0) + 1);
	ret = 1;
end:
	xfree (x);
	xfree (comment);
	return ret;
}

static bool filesys_name_invalid(const TCHAR *fn)
{
	return uaetcslen (fn) > currprefs.filesys_max_name;
}

static int filesys_readdir(struct fs_dirhandle *d, TCHAR *fn, uae_u64 *uniq)
{
	int ok = 0;
	if (d->fstype == FS_ARCHIVE)
		ok = zfile_readdir_archive(d->zd, fn);
	else if (d->fstype == FS_DIRECTORY)
		ok = my_readdir(d->od, fn);
	else if (d->fstype == FS_CDFS)
		ok = isofs_readdir(d->isod, fn, uniq);
	return ok;
}

static int action_examine_all_do(TrapContext *ctx, Unit *unit, uaecptr lock, ExAllKey *eak, uaecptr exalldata, uae_u32 exalldatasize, uae_u32 type, uaecptr control)
{
	a_inode *aino, *base = NULL;
	int ok;
	uae_u32 err;
	struct fs_dirhandle *d;
	TCHAR fn[MAX_DPATH];

	if (lock != 0)
		base = aino_from_lock(ctx, unit, lock);
	if (base == 0)
		base = &unit->rootnode;
	for (;;) {
		uae_u64 uniq = 0;
		d = eak->dirhandle;
		if (!eak->fn) {
			do {
				ok = filesys_readdir(d, fn, &uniq);
			} while (ok && d->fstype == FS_DIRECTORY && (filesys_name_invalid (fn) || fsdb_name_invalid_dir (NULL, fn)));
			if (!ok)
				return 0;
		} else {
			_tcscpy (fn, eak->fn);
			xfree (eak->fn);
			eak->fn = NULL;
		}
		aino = lookup_child_aino_for_exnext (unit, base, fn, &err, uniq, NULL);
		if (!aino)
			return 0;
		eak->id = unit->exallid++;
		trap_put_long(ctx, control + 4, eak->id);
		if (!exalldo(ctx, exalldata, exalldatasize, type, control, unit, aino)) {
			eak->fn = my_strdup (fn); /* no space in exallstruct, save current entry */
			break;
		}
	}
	return 1;
}

static int action_examine_all_end(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uae_u32 id;
	uae_u32 doserr = 0;
	ExAllKey *eak;
	uaecptr control = GET_PCK_ARG5 (packet);

	if (kickstart_version < 36)
		return 0;
	id = trap_get_long(ctx, control + 4);
	eak = getexall (unit, control, id);
#if EXALL_DEBUG > 0
	write_log (_T("EXALL_END ID=%d %x\n"), id, eak);
#endif
	if (!eak) {
		write_log (_T("FILESYS: EXALL_END non-existing ID %d\n"), id);
		doserr = ERROR_OBJECT_WRONG_TYPE;
	} else {
		eak->id = 0;
		fs_closedir (eak->dirhandle);
		xfree (eak->fn);
		eak->fn = NULL;
		eak->dirhandle = NULL;
	}
	if (doserr) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, doserr);
	} else {
		PUT_PCK_RES1 (packet, DOS_TRUE);
	}
	return 1;
}

static int action_examine_all(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr exalldata = GET_PCK_ARG2 (packet);
	uae_u32 exalldatasize = GET_PCK_ARG3 (packet);
	uae_u32 type = GET_PCK_ARG4 (packet);
	uaecptr control = GET_PCK_ARG5 (packet);

	ExAllKey *eak = NULL;
	a_inode *base = NULL;
	struct fs_dirhandle *d;
	int ok, i;
	uaecptr exp;
	uae_u32 id, doserr = ERROR_NO_MORE_ENTRIES;

	ok = 0;

#if EXALL_DEBUG > 0
	write_log (_T("exall: %08x %08x-%08x %d %d %08x\n"),
		lock, exalldata, exalldata + exalldatasize, exalldatasize, type, control);
	write_log (_T("exall: MatchString %08x, MatchFunc %08x\n"),
		trap_get_long(ctx, control + 8), trap_get_long(ctx, control + 12));
#endif

	trap_put_long(ctx, control + 0, 0); /* eac_Entries */

	/* EXAMINE ALL might use dos.library MatchPatternNoCase() which is >=36 */
	if (kickstart_version < 36)
		return 0;

	if (type == 0 || type > 8) {
		doserr = ERROR_BAD_NUMBER;
		goto fail;
	}

	PUT_PCK_RES1 (packet, DOS_TRUE);
	id = trap_get_long(ctx, control + 4);
	if (id == EXALL_END) {
		write_log (_T("FILESYS: EXALL called twice with ERROR_NO_MORE_ENTRIES\n"));
		goto fail; /* already ended exall() */
	}
	if (id) {
		eak = getexall (unit, control, id);
		if (!eak) {
			write_log (_T("FILESYS: EXALL non-existing ID %d\n"), id);
			doserr = ERROR_OBJECT_WRONG_TYPE;
			goto fail;
		}
		if (!action_examine_all_do(ctx, unit, lock, eak, exalldata, exalldatasize, type, control))
			goto fail;
		if (trap_get_long(ctx, control + 0) == 0) {
			/* uh, no space for first entry.. */
			doserr = ERROR_NO_FREE_STORE;
			goto fail;
		}

	} else {

		eak = getexall (unit, control, -1);
		if (!eak)
			goto fail;
		if (lock != 0)
			base = aino_from_lock(ctx, unit, lock);
		if (base == 0)
			base = &unit->rootnode;
#if EXALL_DEBUG > 0
		write_log("exall: ID=%d '%s'\n", eak->id, base->nname);
#endif
		d = fs_opendir (unit, base);
		if (!d)
			goto fail;
		eak->dirhandle = d;
		trap_put_long(ctx, control + 4, eak->id);
		if (!action_examine_all_do(ctx, unit, lock, eak, exalldata, exalldatasize, type, control))
			goto fail;
		if (trap_get_long(ctx, control + 0) == 0) {
			/* uh, no space for first entry.. */
			doserr = ERROR_NO_FREE_STORE;
			goto fail;
		}

	}
	ok = 1;

fail:
	/* Clear last ed_Next. This "list" is quite non-Amiga like.. */
	exp = exalldata;
	i = trap_get_long(ctx, control + 0);
	for (;;) {
		if (i <= 1) {
			if (exp)
				trap_put_long(ctx, exp, 0);
			break;
		}
		exp = trap_get_long(ctx, exp); /* ed_Next */
		i--;
	}
#if EXALL_DEBUG > 0
	write_log("ok=%d, err=%d, eac_Entries = %d\n", ok, ok ? -1 : doserr, trap_get_long(ctx, control + 0));
#endif

	if (!ok) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, doserr);
		if (eak) {
			eak->id = 0;
			fs_closedir (eak->dirhandle);
			eak->dirhandle = NULL;
			xfree (eak->fn);
			eak->fn = NULL;
		}
		if (doserr == ERROR_NO_MORE_ENTRIES)
			trap_put_long(ctx, control + 4, EXALL_END);
	}
	return 1;
}

static uae_u32 exall_helper(TrapContext *ctx)
{
	int i;
	Unit *u;
	uaecptr pck = trap_get_areg(ctx, 4);

	dpacket packet;
	readdpacket(ctx, &packet, pck);

	uaecptr control = get_long_host(packet.packet_data + dp_Arg5);
	uae_u32 id = trap_get_long(ctx, control + 4);

#if EXALL_DEBUG > 0
	write_log (_T("FILESYS: EXALL extra round ID=%d\n"), id);
#endif
	if (id == EXALL_END)
		return 1;
	for (u = units; u; u = u->next) {
		for (i = 0; i < EXALLKEYS; i++) {
			if (u->exalls[i].id == id && u->exalls[i].control == control) {
				action_examine_all(ctx, u, &packet);
			}
		}
	}
	return 1;
}

static uae_u32 REGPARAM2 fsmisc_helper(TrapContext *ctx)
{
	int mode = trap_get_dreg(ctx, 0);

	switch (mode)
	{
	case 0:
	return exall_helper(ctx);
	case 1:
	return filesys_media_change_reply (0);
	case 2:
	return filesys_media_change_reply (1);
	case 3:
		uae_u32 t = getlocaltime ();
		uae_u32 secs = (uae_u32)t - (8 * 365 + 2) * 24 * 60 * 60;
		return secs;
	}
	return 0;
}

static void action_examine_object(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr info = GET_PCK_ARG2 (packet) << 2;
	a_inode *aino = 0;

	TRACE((_T("ACTION_EXAMINE_OBJECT(0x%x,0x%x)\n"), lock, info));
	DUMPLOCK(ctx, unit, lock);

	if (lock != 0)
		aino = aino_from_lock(ctx, unit, lock);
	if (aino == 0)
		aino = &unit->rootnode;

	get_fileinfo(ctx, unit, packet, info, aino, false);
}

extern unsigned char def_tool[];
extern unsigned int def_tool_len;
extern unsigned char def_project[];
extern unsigned int def_project_len;
extern unsigned char def_drawer[];
extern unsigned int def_drawer_len;
static struct virtualfilesysobject vfso_icon_tool;
static struct virtualfilesysobject vfso_icon_project;
static struct virtualfilesysobject vfso_icon_drawer;

static void load_injected_icon(struct virtualfilesysobject *vfso, const TCHAR *fn, uae_u8 *default_data, int default_size)
{
	uae_u8 *data = NULL;
	int size;

	xfree(vfso->data);
	if (fn && fn[0])
		data = zfile_load_file(fn, &size);
	if (!data) {
		vfso->data = xmalloc(uae_u8, default_size);
		memcpy(vfso->data, default_data, default_size);
		vfso->size = default_size;
		return;
	}
	vfso->data = data;
	vfso->size = size;
}

static void load_injected_icons(void)
{
	load_injected_icon(&vfso_icon_tool, currprefs.filesys_inject_icons_tool, def_tool, def_tool_len);
	load_injected_icon(&vfso_icon_project, currprefs.filesys_inject_icons_project, def_project, def_project_len);
	load_injected_icon(&vfso_icon_drawer, currprefs.filesys_inject_icons_drawer, def_drawer, def_drawer_len);
}

static void inject_icons_to_directory(Unit *unit, a_inode *base)
{
	for (a_inode *aino = base->child; aino; aino = aino->sibling) {
		int len = uaetcslen(aino->aname);
		if (len >= 5 && !_tcsicmp(aino->aname + len - 5, _T(".info")))
			continue;
		TCHAR tmp[256];
		_sntprintf(tmp, sizeof tmp, _T("%s.info"), aino->aname);
		bool match = false;
		for (a_inode *aino2 = base->child; aino2; aino2 = aino2->sibling) {
			if (!_tcsicmp(aino2->aname, tmp))
				match = true;
		}
		if (match)
			continue;
		uae_u32 err;
		struct virtualfilesysobject *vfso;
		if (aino->dir) {
			vfso = &vfso_icon_drawer;
		} else {
			if (aino->amigaos_mode & A_FIBF_EXECUTE)
				vfso = &vfso_icon_project;
			else
				vfso = &vfso_icon_tool;
		}
		lookup_child_aino_for_exnext(unit, base, tmp, &err, 0, vfso);
	}
}

/* Read a directory's contents, create a_inodes for each file, and
mark them as locked in memory so that recycle_aino will not reap
them.
We do this to avoid problems with the host OS: we don't want to
leave the directory open on the host side until all ExNext()s have
finished - they may never finish!  */

static void populate_directory (Unit *unit, a_inode *base)
{
	struct fs_dirhandle *d;
	a_inode *aino;

	d = fs_opendir (unit, base);
	if (!d)
		return;
	for (aino = base->child; aino; aino = aino->sibling) {
		base->locked_children++;
		unit->total_locked_ainos++;
	}
	TRACE3((_T("Populating directory, child %s, locked_children %d\n"),
		base->child ? base->child->nname : _T("<NULL>"), base->locked_children));
	for (;;) {
		uae_u64 uniq = 0;
		TCHAR fn[MAX_DPATH];
		int ok;
		uae_u32 err;

		/* Find next file that belongs to the Amiga fs (skipping things
		like "..", "." etc.  */
		do {
			ok = filesys_readdir(d, fn, &uniq);
		} while (ok && d->fstype == FS_DIRECTORY && (filesys_name_invalid (fn) || fsdb_name_invalid_dir (NULL, fn)));
		if (!ok)
			break;
		/* This calls init_child_aino, which will notice that the parent is
		being ExNext()ed, and it will increment the locked counts.  */
		aino = lookup_child_aino_for_exnext (unit, base, fn, &err, uniq, NULL);
	}
	fs_closedir (d);
	if (currprefs.filesys_inject_icons || unit->ui.inject_icons)
		inject_icons_to_directory(unit, base);
}

static bool do_examine(TrapContext *ctx, Unit *unit, dpacket *packet, a_inode *aino, uaecptr info, bool longfilesize)
{
	for (;;) {
		TCHAR *name;
		if (!aino)
			break;
		name = aino->nname;
		get_fileinfo(ctx, unit, packet, info, aino, longfilesize);
		if (!aino->vfso && !(unit->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS)) && !fsdb_exists(name)) {
			TRACE ((_T("%s orphaned"), name));
			return false;
		}
		return true;
	}
	TRACE((_T("no more entries\n")));
	free_exkey (unit, aino->parent);
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_NO_MORE_ENTRIES);
	return true;
}

#define EXNEXT_DEBUG 0

static void action_examine_next(TrapContext *ctx, Unit *unit, dpacket *packet, bool largefilesize)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr info = GET_PCK_ARG2 (packet) << 2;
	a_inode *aino = 0, *daino = 0;
	uae_u32 uniq;

	TRACE((_T("ACTION_EXAMINE_NEXT(0x%x,0x%x,%d)\n"), lock, info, largefilesize));
	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);
	DUMPLOCK(ctx, unit, lock);

	if (lock != 0)
		aino = aino_from_lock(ctx, unit, lock);
	if (aino == 0)
		aino = &unit->rootnode;
	uniq = trap_get_long(ctx, info);
	for (;;) {
		if (uniq == aino->uniq) {
			// first exnext
			if (!aino->dir) {
				write_log (_T("ExNext called for a file! %s:%d (Houston?)\n"), aino->nname, uniq);
				goto no_more_entries;
			}
			if (aino->exnext_count++ == 0)
				populate_directory (unit, aino);
			if (!aino->child)
				goto no_more_entries;
			daino = aino->child;
		} else {
			daino = lookup_aino(unit, uniq);
			if (!daino) {
#if EXNEXT_DEBUG
				write_log(_T("EXNEXT but next entry is missing! (%d)\n"), uniq);
#endif
				// deleted? Look for next larger uniq in same directory.
				daino = aino->child;
				while (daino && uniq >= daino->uniq) {
					daino = daino->sibling;
				}
#if EXNEXT_DEBUG
				if (daino) {
					write_log(_T("Using next found entry %d\n"), uniq);
				}
#endif
				// didn't find, what about previous?
				if (!daino) {
					daino = aino->child;
					while (daino && uniq >= daino->uniq) {
						if (daino->sibling && daino->sibling->uniq >= uniq) {
#if EXNEXT_DEBUG
							write_log(_T("Using previous entry %d\n"), uniq);
#endif
							break;
						}
						daino = daino->sibling;
					}
				}
				// didn't find any but there are still entries? restart from beginning.
				if (!daino && aino->child) {
					daino = aino->child;
#if EXNEXT_DEBUG
					write_log(_T("Re-starting from beginning %d\n"), daino->uniq);
#endif
				}
			} else {
				daino = daino->sibling;
			}
		}
		if (!daino)
			goto no_more_entries;
		if (daino->parent != aino) {
			write_log(_T("Houston, we have a BIG problem. %s is not parent of %s\n"), daino->nname, aino->nname);
			goto no_more_entries;
		}
		uniq = daino->uniq;
		if (daino->mountcount != unit->mountcount)
			continue;
		if (!do_examine(ctx, unit, packet, daino, info, largefilesize))
			continue;
		return;
	}

no_more_entries:
	free_exkey(unit, aino);
	PUT_PCK_RES1(packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_NO_MORE_ENTRIES);
}

static void do_find(TrapContext *ctx, Unit *unit, dpacket *packet, int mode, int create, int fallback)
{
	uaecptr fh = GET_PCK_ARG1 (packet) << 2;
	uaecptr lock = GET_PCK_ARG2 (packet) << 2;
	uaecptr name = GET_PCK_ARG3 (packet) << 2;
	a_inode *aino;
	Key *k;
	struct fs_filehandle *fd = NULL;
	int err;
	mode_t openmode;
	int aino_created = 0;
	int isvirtual = unit->volflags & (MYVOLUMEINFO_ARCHIVE | MYVOLUMEINFO_CDFS);

	TRACE((_T("ACTION_FIND_*(0x%08x,0x%08x,\"%s\",%d,%d)\n"), fh, lock, bstr(ctx, unit, name), mode, create));
	TRACE((_T("fh=%x lock=%x name=%x\n"), fh, lock, name));
	DUMPLOCK(ctx, unit, lock);

	aino = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);

	if (aino == 0 || (err != 0 && err != ERROR_OBJECT_NOT_AROUND)) {
		/* Whatever it is, we can't handle it. */
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	if (aino->softlink) {
		handle_softlink(ctx, unit, packet, aino);
		return;
	}
	if (err == 0) {
		/* Object exists. */
		if (aino->dir) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_OBJECT_WRONG_TYPE);
			return;
		}
		if (aino->elock || (create == 2 && aino->shlock > 0)) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
			return;
		}
		if (create && aino->vfso) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_DELETE_PROTECTED);
			return;
		}
		if (create == 2 && (aino->amigaos_mode & A_FIBF_DELETE) != 0) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_DELETE_PROTECTED);
			return;
		}
		if (create != 2) {
			if ((((mode & aino->amigaos_mode) & A_FIBF_WRITE) != 0 || is_writeprotected(unit))
				&& fallback)
			{
				mode &= ~A_FIBF_WRITE;
			}
			/* Kick 1.3 doesn't check read and write access bits - maybe it would be
			* simpler just not to do that either. */
			if ((mode & A_FIBF_WRITE) != 0 && is_writeprotected(unit)) {
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
				return;
			}
			if (((mode & aino->amigaos_mode) & A_FIBF_WRITE) != 0
				|| mode == 0)
			{
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, ERROR_WRITE_PROTECTED);
				return;
			}
			if (((mode & aino->amigaos_mode) & A_FIBF_READ) != 0) {
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, ERROR_READ_PROTECTED);
				return;
			}
		}
	} else if (create == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	} else {
		/* Object does not exist. aino points to containing directory. */
		aino = create_child_aino(unit, aino, my_strdup (bstr_cut(ctx, unit, name)), 0);
		if (aino == 0) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
			return;
		}
		aino_created = 1;
	}

	prepare_for_open (aino->nname);

	if (!aino->vfso) {
		openmode = (((mode & A_FIBF_READ) == 0 ? O_WRONLY
			: (mode & A_FIBF_WRITE) == 0 ? O_RDONLY
			: O_RDWR)
			| (create ? O_CREAT : 0)
			| (create == 2 ? O_TRUNC : 0));

		fd = fs_openfile (unit, aino, openmode | O_BINARY);
		if (fd == NULL) {
			if (aino_created)
				delete_aino (unit, aino);
			PUT_PCK_RES1 (packet, DOS_FALSE);
			/* archive and fd == NULL = corrupt archive or out of memory */
			PUT_PCK_RES2 (packet, isvirtual ? ERROR_OBJECT_NOT_AROUND : dos_errno ());
			return;
		}
	}

	k = new_key (unit);
	k->fd = fd;
	k->aino = aino;
	k->dosmode = mode;
	k->createmode = create;
	k->notifyactive = create ? 1 : 0;

	if (create && isvirtual)
		fsdb_set_file_attrs (aino);

	trap_put_long(ctx, fh + 36, k->uniq);
	if (create == 2) {
		aino->elock = 1;
		// clear comment if file already existed
		if (aino->comment) {
			xfree (aino->comment);
			aino->comment = 0;
		}
		fsdb_set_file_attrs (aino);
	} else {
		aino->shlock++;
	}
	de_recycle_aino (unit, aino);

	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void action_fh_from_lock(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr fh = GET_PCK_ARG1 (packet) << 2;
	uaecptr lock = GET_PCK_ARG2 (packet) << 2;
	a_inode *aino;
	Key *k;
	struct fs_filehandle *fd;
	mode_t openmode;
	int mode;

	TRACE((_T("ACTION_FH_FROM_LOCK(0x%x,0x%x)\n"), fh, lock));
	DUMPLOCK(ctx, unit,lock);

	if (!lock) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, 0);
		return;
	}

	aino = aino_from_lock(ctx, unit, lock);
	if (aino == 0)
		aino = &unit->rootnode;
	if (aino->softlink) {
		handle_softlink(ctx, unit, packet, aino);
		return;
	}

	mode = aino->amigaos_mode; /* Use same mode for opened filehandle as existing Lock() */

	prepare_for_open (aino->nname);

	TRACE ((_T("  mode is %d\n"), mode));
	openmode = (((mode & A_FIBF_READ) ? O_WRONLY
		: (mode & A_FIBF_WRITE) ? O_RDONLY
		: O_RDWR));

	/* the files on CD really can have the write-bit set.  */
	if (is_writeprotected(unit))
		openmode = O_RDONLY;

	fd = fs_openfile (unit, aino, openmode | O_BINARY);

	if (fd == NULL) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno ());
		return;
	}
	k = new_key (unit);
	k->fd = fd;
	k->aino = aino;

	trap_put_long(ctx, fh + 36, k->uniq);
	/* I don't think I need to play with shlock count here, because I'm
	opening from an existing lock ??? */

	de_recycle_aino (unit, aino);
	free_lock(ctx, unit, lock); /* lock must be unlocked */
	PUT_PCK_RES1 (packet, DOS_TRUE);
	/* PUT_PCK_RES2 (packet, k->uniq); - this shouldn't be necessary, try without it */
}

static void	action_find_input(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	do_find(ctx, unit, packet, A_FIBF_READ | A_FIBF_WRITE, 0, 1);
}

static void	action_find_output(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}
	do_find(ctx, unit, packet, A_FIBF_READ | A_FIBF_WRITE, 2, 0);
}

static void	action_find_write(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}
	do_find(ctx, unit, packet, A_FIBF_READ | A_FIBF_WRITE, 1, 0);
}

/* change file/dir's parent dir modification time */
static void updatedirtime (a_inode *a1, int now)
{
	struct mystat statbuf;

	if (!a1->parent)
		return;
#ifdef AMIBERRY
	if (!a1->parent->parent) {
		return;
	}
#endif
	if (!now) {
		if (!my_stat (a1->nname, &statbuf))
			return;
		my_utime (a1->parent->nname, &statbuf.mtime);
	} else {
		my_utime (a1->parent->nname, NULL);
	}
}

static void	action_end(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k;
	TRACE((_T("ACTION_END(0x%x)\n"), GET_PCK_ARG1 (packet)));

	k = lookup_key (unit, GET_PCK_ARG1 (packet));
	if (k != 0) {
		if (k->notifyactive) {
			notify_check(ctx, unit, k->aino);
			updatedirtime (k->aino, 1);
		}
		if (k->aino->elock)
			k->aino->elock = 0;
		else
			k->aino->shlock--;
		recycle_aino (unit, k->aino);
		free_key (unit, k);
	}
	PUT_PCK_RES1 (packet, DOS_TRUE);
	PUT_PCK_RES2 (packet, 0);
}

static void	action_read(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uaecptr addr = GET_PCK_ARG2 (packet);
	uae_u32 size = GET_PCK_ARG3 (packet);
	uae_u32 actual = 0;

	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		/* PUT_PCK_RES2 (packet, EINVAL); */
		return;
	}
	TRACE((_T("ACTION_READ(%s,0x%x,%d)\n"), k->aino->nname, addr, size));
	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);

	if (size == 0) {
		PUT_PCK_RES1 (packet, 0);
		PUT_PCK_RES2 (packet, 0);
	} else if (k->aino->vfso) {
		uae_s64 filesize = k->aino->vfso->size;
		for (int i = 0; i < size && k->file_pos < filesize; i++) {
			trap_put_byte(ctx, addr + i, k->aino->vfso->data[k->file_pos]);
			k->file_pos++;
			actual++;
		}
		PUT_PCK_RES1 (packet, actual);
		size = 0;
	} else {
		/* check if filesize < size */
		uae_s64 filesize, cur;

		filesize = key_filesize(k);
		cur = k->file_pos;
		if (size > filesize - cur)
			size = (uae_u32)(filesize - cur);

		if (size == 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, 0);
		} else if (!trap_valid_address(ctx, addr, size)) {
			/* it really crosses memory boundary */
			uae_u8 *buf;
			
			/* ugh this is inefficient but easy */

			if (key_seek(k, k->file_pos, SEEK_SET) < 0) {
				PUT_PCK_RES1 (packet, 0);
				PUT_PCK_RES2 (packet, dos_errno ());
				return;
			}

			buf = xmalloc (uae_u8, size);
			if (!buf) {
				PUT_PCK_RES1 (packet, -1);
				PUT_PCK_RES2 (packet, ERROR_NO_FREE_STORE);
				return;
			}

			actual = fs_read (k->fd, buf, size);

			if ((uae_s32)actual == -1) {
				PUT_PCK_RES1 (packet, 0);
				PUT_PCK_RES2 (packet, dos_errno ());
			} else {
				PUT_PCK_RES1 (packet, actual);
				trap_put_bytes(ctx, buf, addr, actual);
				k->file_pos += actual;
			}
			xfree (buf);
			size = 0;
		}
	}

	if (size) {

		if (key_seek(k, k->file_pos, SEEK_SET) < 0) {
			PUT_PCK_RES1(packet, 0);
			PUT_PCK_RES2(packet, dos_errno());
			return;
		}

		if (trap_is_indirect() || !real_address_allowed()) {

			uae_u8 buf[RTAREA_TRAP_DATA_EXTRA_SIZE];
			actual = 0;
			while (size > 0) {
				int toread = size > RTAREA_TRAP_DATA_EXTRA_SIZE ? RTAREA_TRAP_DATA_EXTRA_SIZE : size;
				int read = fs_read(k->fd, buf, toread);
				if (read < 0) {
					actual = -1;
					break;
				}
				if (read == 0)
					break;
				trap_put_bytes(ctx, buf, addr, read);
				size -= read;
				addr += read;
				actual += read;
				if (read < toread)
					break;
			}

		} else {

			/* normal fast read */
			uae_u8 *realpt = get_real_address (addr);
			actual = fs_read (k->fd, realpt, size);

		}

		if (actual == 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, 0);
		} else if (actual < 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, dos_errno ());
		} else {
			PUT_PCK_RES1 (packet, actual);
			k->file_pos += actual;
		}
	}

	TRACE((_T("=%d\n"), actual));
}

static void action_write(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uaecptr addr = GET_PCK_ARG2 (packet);
	uae_u32 size = GET_PCK_ARG3 (packet);
	uae_u32 actual;
	uae_u8 *buf;

	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		/* PUT_PCK_RES2 (packet, EINVAL); */
		return;
	}

	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
	TRACE((_T("ACTION_WRITE(%s,0x%x,%d)\n"), k->aino->nname, addr, size));

	if (is_writeprotected(unit) || k->aino->vfso) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	if (size == 0) {

		actual = 0;
		PUT_PCK_RES1 (packet, 0);
		PUT_PCK_RES2 (packet, 0);

	} else if (trap_valid_address(ctx, addr, size)) {

		if (key_seek(k, k->file_pos, SEEK_SET) < 0) {
			PUT_PCK_RES1(packet, 0);
			PUT_PCK_RES2(packet, dos_errno());
			return;
		}

		if (trap_is_indirect() || !real_address_allowed()) {

			uae_u8 buf[RTAREA_TRAP_DATA_EXTRA_SIZE];
			actual = 0;
			int sizecnt = size;
			while (sizecnt > 0) {
				int towrite = sizecnt > RTAREA_TRAP_DATA_EXTRA_SIZE ? RTAREA_TRAP_DATA_EXTRA_SIZE : sizecnt;
				trap_get_bytes(ctx, buf, addr, towrite);
				int write = fs_write(k->fd, buf, towrite);
				if (write < 0) {
					actual = -1;
					break;
				}
				if (write == 0)
					break;
				sizecnt -= write;
				addr += write;
				actual += write;
				if (write < towrite)
					break;
			}

		} else {

			uae_u8 *realpt = get_real_address (addr);
			actual = fs_write (k->fd, realpt, size);
		}

	} else {
		/* ugh this is inefficient but easy */

		if (key_seek(k, k->file_pos, SEEK_SET) < 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, dos_errno ());
			return;
		}

		buf = xmalloc (uae_u8, size);
		if (!buf) {
			PUT_PCK_RES1 (packet, -1);
			PUT_PCK_RES2 (packet, ERROR_NO_FREE_STORE);
			return;
		}

		trap_get_bytes(ctx, buf, addr, size);

		actual = fs_write (k->fd, buf, size);
		xfree (buf);
	}

	TRACE((_T("=%d\n"), actual));
	PUT_PCK_RES1 (packet, actual);
	if (actual != size)
		PUT_PCK_RES2 (packet, dos_errno ());
	if ((uae_s32)actual != -1)
		k->file_pos += actual;

	k->notifyactive = 1;
}

static void	action_seek(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	int pos = (uae_s32)GET_PCK_ARG2 (packet);
	int mode = (uae_s32)GET_PCK_ARG3 (packet);
	uae_s64 res;
	uae_s64 cur;
	int whence = SEEK_CUR;
	uae_s64 temppos, filesize;

	if (k == 0) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	cur = k->file_pos;
	TRACE((_T("ACTION_SEEK(%s,%d,%d)=%lld\n"), k->aino->nname, pos, mode, cur));
	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);

	filesize = key_filesize(k);
	if (whence == SEEK_CUR)
		temppos = cur + pos;
	if (whence == SEEK_SET)
		temppos = pos;
	if (whence == SEEK_END)
		temppos = filesize + pos;
	if (filesize < temppos || temppos < 0) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
		return;
	}

	res = key_seek(k, pos, whence);
	if (-1 == res || cur > MAXFILESIZE32) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
		key_seek(k, cur, SEEK_SET);
	} else {
		PUT_PCK_RES1 (packet, cur);
		k->file_pos = key_seek(k, 0, SEEK_CUR);
	}
}

static void	action_set_protect(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG2 (packet) << 2;
	uaecptr name = GET_PCK_ARG3 (packet) << 2;
	uae_u32 mask = GET_PCK_ARG4 (packet);
	a_inode *a;
	int err;

	TRACE((_T("ACTION_SET_PROTECT(0x%x,\"%s\",0x%x)\n"), lock, bstr(ctx, unit, name), mask));

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	a = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);
	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	if (a->softlink) {
		handle_softlink(ctx, unit, packet, a);
		return;
	}

	a->amigaos_mode = mask;
	if (!fsdb_cando (unit))
		a->amigaos_mode = fsdb_mode_supported (a);
	err = fsdb_set_file_attrs (a);
	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
	} else {
		PUT_PCK_RES1 (packet, DOS_TRUE);
	}
	notify_check(ctx, unit, a);
	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
}

static void action_set_comment(TrapContext *ctx, Unit * unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG2 (packet) << 2;
	uaecptr name = GET_PCK_ARG3 (packet) << 2;
	uaecptr comment = GET_PCK_ARG4 (packet) << 2;
	TCHAR *commented = NULL;
	a_inode *a;
	int err;

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	if (fsdb_cando (unit)) {
		commented = bstr(ctx, unit, comment);
		if (uaetcslen (commented) > 80) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_COMMENT_TOO_BIG);
			return;
		}
		if (uaetcslen (commented) > 0) {
			TCHAR *p = commented;
			commented = xmalloc (TCHAR, 81);
			_tcsncpy (commented, p, 80);
			commented[80] = 0;
		} else {
			commented = NULL;
		}
	}
	TRACE ((_T("ACTION_SET_COMMENT(0x%x,\"%s\")\n"), lock, commented));

	a = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);
	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);

maybe_free_and_out:
		if (commented)
			xfree (commented);
		return;
	}
	if (a->softlink) {
		handle_softlink(ctx, unit, packet, a);
		goto maybe_free_and_out;
	}

	PUT_PCK_RES1 (packet, DOS_TRUE);
	PUT_PCK_RES2 (packet, 0);
	if (a->comment == 0 && commented == 0)
		goto maybe_free_and_out;
	if (a->comment != 0 && commented != 0 && _tcscmp (a->comment, commented) == 0)
		goto maybe_free_and_out;
	if (a->comment)
		xfree (a->comment);
	a->comment = commented;
	fsdb_set_file_attrs (a);
	notify_check(ctx, unit, a);
	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
}

static void	action_same_lock(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock1 = GET_PCK_ARG1 (packet) << 2;
	uaecptr lock2 = GET_PCK_ARG2 (packet) << 2;

	TRACE((_T("ACTION_SAME_LOCK(0x%x,0x%x)\n"), lock1, lock2));
	DUMPLOCK(ctx, unit, lock1); DUMPLOCK(ctx, unit, lock2);

	if (!lock1 || !lock2) {
		PUT_PCK_RES1 (packet, lock1 == lock2 ? DOS_TRUE : DOS_FALSE);
	} else {
		PUT_PCK_RES1 (packet, trap_get_long(ctx, lock1 + 4) == trap_get_long(ctx, lock2 + 4) ? DOS_TRUE : DOS_FALSE);
	}
}

static void	action_change_mode(TrapContext *ctx, Unit *unit, dpacket *packet)
{
#define CHANGE_LOCK 0
#define CHANGE_FH 1
	/* will be CHANGE_FH or CHANGE_LOCK value */
	int type = GET_PCK_ARG1 (packet);
	/* either a file-handle or lock */
	uaecptr object = GET_PCK_ARG2 (packet) << 2;
	/* will be EXCLUSIVE_LOCK/SHARED_LOCK if CHANGE_LOCK,
	* or MODE_OLDFILE/MODE_NEWFILE/MODE_READWRITE if CHANGE_FH *
	* Above is wrong, it is always *_LOCK. TW. */
	int mode = GET_PCK_ARG3 (packet);
	unsigned long uniq;
	a_inode *a = NULL, *olda = NULL;
	int err = 0;
	TRACE((_T("ACTION_CHANGE_MODE(0x%x,%d,%d)\n"), object, type, mode));

	if (! object || (type != CHANGE_FH && type != CHANGE_LOCK)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (type == CHANGE_LOCK) {
		uniq = trap_get_long(ctx, object + 4);
	} else {
		Key *k = lookup_key (unit, trap_get_long(ctx, object + 36));
		if (!k) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
			return;
		}
		uniq = k->aino->uniq;
	}
	a = lookup_aino (unit, uniq);

	if (! a) {
		err = ERROR_INVALID_LOCK;
	} else {
		if (mode == -1) {
			if (a->shlock > 1) {
				err = ERROR_OBJECT_IN_USE;
			} else {
				a->shlock = 0;
				a->elock = 1;
			}
		} else { /* Must be SHARED_LOCK == -2 */
			a->elock = 0;
			a->shlock++;
		}
	}

	if (err) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	} else {
		de_recycle_aino (unit, a);
		PUT_PCK_RES1 (packet, DOS_TRUE);
	}
}

static void	action_parent_common(TrapContext *ctx, Unit *unit, dpacket *packet, unsigned long uniq)
{
	a_inode *olda = lookup_aino (unit, uniq);
	if (olda == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (olda->parent == 0) {
		PUT_PCK_RES1 (packet, 0);
		PUT_PCK_RES2 (packet, 0);
		return;
	}
	if (olda->parent->elock) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
		return;
	}
	olda->parent->shlock++;
	de_recycle_aino (unit, olda->parent);
	PUT_PCK_RES1(packet, make_lock(ctx, unit, olda->parent->uniq, -2) >> 2);
}

static void	action_parent_fh(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	if (!k) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}
	action_parent_common (ctx, unit, packet, k->aino->uniq);
}

static void	action_parent(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;

	TRACE((_T("ACTION_PARENT(0x%x)\n"),lock));

	if (!lock) {
		PUT_PCK_RES1 (packet, 0);
		PUT_PCK_RES2 (packet, 0);
	} else {
		action_parent_common(ctx, unit, packet, trap_get_long(ctx, lock + 4));
	}
	TRACE((_T("=%x %d\n"), GET_PCK_RES1 (packet), GET_PCK_RES2 (packet)));
}

static void	action_create_dir(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr name = GET_PCK_ARG2 (packet) << 2;
	a_inode *aino;
	int err;

	TRACE((_T("ACTION_CREATE_DIR(0x%x,\"%s\")\n"), lock, bstr(ctx, unit, name)));

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	aino = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);
	if (aino == 0 || (err != 0 && err != ERROR_OBJECT_NOT_AROUND)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	if (err == 0) {
		/* Object exists. */
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_EXISTS);
		return;
	}
	/* Object does not exist. aino points to containing directory. */
	aino = create_child_aino(unit, aino, my_strdup (bstr_cut(ctx, unit, name)), 1);
	if (aino == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
		return;
	}

	if (my_mkdir (aino->nname) == -1) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno ());
		return;
	}
	aino->shlock = 1;
	fsdb_set_file_attrs (aino);
	de_recycle_aino (unit, aino);
	notify_check(ctx, unit, aino);
	updatedirtime (aino, 0);
	PUT_PCK_RES1(packet, make_lock(ctx, unit, aino->uniq, -2) >> 2);
	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
}

static void	action_examine_fh(TrapContext *ctx, Unit *unit, dpacket *packet, bool largefilesize)
{
	Key *k;
	a_inode *aino = 0;
	uaecptr info = GET_PCK_ARG2 (packet) << 2;

	TRACE((_T("ACTION_EXAMINE_FH(0x%x,0x%x,%d)\n"),
		GET_PCK_ARG1 (packet), GET_PCK_ARG2 (packet), largefilesize ));

	k = lookup_key (unit, GET_PCK_ARG1 (packet));
	if (k != 0)
		aino = k->aino;
	if (aino == 0)
		aino = &unit->rootnode;

	get_fileinfo(ctx, unit, packet, info, aino, largefilesize);
}

/* For a nice example of just how contradictory documentation can be, see the
* Autodoc for DOS:SetFileSize and the Packets.txt description of this packet...
* This implementation tries to mimic the behaviour of the Kick 3.1 ramdisk
* (which seems to match the Autodoc description). */
static void	action_set_file_size(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k, *k1;
	off_t offset = GET_PCK_ARG2 (packet);
	int mode = (uae_s32)GET_PCK_ARG3 (packet);
	int whence = SEEK_CUR;

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE((_T("ACTION_SET_FILE_SIZE(0x%lx, %d, 0x%x)\n"), GET_PCK_ARG1 (packet), offset, mode));

	k = lookup_key (unit, GET_PCK_ARG1 (packet));
	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_TRUE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}
	if (k->aino->vfso) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	/* Fail if file is >=2G, it is not safe operation. */
	if (key_filesize(k) > MAXFILESIZE32_2G) {
		PUT_PCK_RES1 (packet, DOS_TRUE);
		PUT_PCK_RES2 (packet, ERROR_BAD_NUMBER); /* ? */
		return;
	}

	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);
	k->notifyactive = 1;
	/* If any open files have file pointers beyond this size, truncate only
	 * so far that these pointers do not become invalid.  */
	for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k != k1 && k->aino == k1->aino) {
			if (k1->file_pos > offset)
				offset = (off_t)k1->file_pos;
		}
	}

	/* Write one then truncate: that should give the right size in all cases.  */
	fs_lseek (k->fd, offset, whence);
	offset = fs_lseek (k->fd, 0, SEEK_CUR);
	fs_write (k->fd, /* whatever */(uae_u8*)&k1, 1);
	if (k->file_pos > offset)
		k->file_pos = offset;
	fs_lseek (k->fd, (off_t)k->file_pos, SEEK_SET);

	/* Brian: no bug here; the file _must_ be one byte too large after writing
	 * The write is supposed to guarantee that the file can't be smaller than
	 * the requested size, the truncate guarantees that it can't be larger.
	 * If we were to write one byte earlier we'd clobber file data.  */
	if (my_truncate (k->aino->nname, offset) == -1) {
		PUT_PCK_RES1 (packet, DOS_TRUE);
		PUT_PCK_RES2 (packet, dos_errno ());
		return;
	}

	PUT_PCK_RES1 (packet, offset);
	PUT_PCK_RES2 (packet, 0);
}

static int relock_do(Unit *unit, a_inode *a1)
{
	Key *k1, *knext;
	int wehavekeys = 0;

	for (k1 = unit->keys; k1; k1 = knext) {
		knext = k1->next;
		if (k1->aino == a1 && k1->fd) {
			wehavekeys++;
			fs_closefile (k1->fd);
			write_log (_T("handle %p freed\n"), k1->fd);
		}
	}
	return wehavekeys;
}

static void relock_re (Unit *unit, a_inode *a1, a_inode *a2, int failed)
{
	Key *k1, *knext;

	for (k1 = unit->keys; k1; k1 = knext) {
		knext = k1->next;
		if (k1->aino == a1 && k1->fd) {
			int mode = (k1->dosmode & A_FIBF_READ) == 0 ? O_WRONLY : (k1->dosmode & A_FIBF_WRITE) == 0 ? O_RDONLY : O_RDWR;
			mode |= O_BINARY;
			if (failed) {
				/* rename still failed, restore fd */
				k1->fd = fs_openfile (unit, a1, mode);
				write_log (_T("restoring old handle '%s' %d\n"), a1->nname, k1->dosmode);
			} else {
				/* transfer fd to new name */
				if (a2) {
					k1->aino = a2;
					k1->fd = fs_openfile (unit, a2, mode);
					write_log (_T("restoring new handle '%s' %d\n"), a2->nname, k1->dosmode);
				} else {
					write_log (_T("no new handle, deleting old lock(s).\n"));
				}
			}
			if (k1->fd == NULL) {
				write_log (_T("relocking failed '%s' -> '%s'\n"), a1->nname, a2->nname);
				free_key (unit, k1);
			} else {
				key_seek(k1, k1->file_pos, SEEK_SET);
			}
		}
	}
}

static void	action_delete_object(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr name = GET_PCK_ARG2 (packet) << 2;
	a_inode *a;
	int err;

	TRACE((_T("ACTION_DELETE_OBJECT(0x%x,\"%s\")\n"), lock, bstr(ctx, unit, name)));

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	a = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);

	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	if (a->amigaos_mode & A_FIBF_DELETE) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DELETE_PROTECTED);
		return;
	}
	if (a->shlock > 0 || a->elock) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
		return;
	}
	if (!a->vfso) {
		if (a->dir) {
			/* This should take care of removing the fsdb if no files remain.  */
			fsdb_dir_writeback (a);
			if (my_rmdir (a->nname) == -1) {
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, dos_errno ());
				return;
			}
		} else {
#ifdef _WIN32
			if (my_unlink (a->nname, false) == -1) {
#else
			if (my_unlink (a->nname) == -1) {
#endif
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, dos_errno ());
				return;
			}
		}
	}
	notify_check(ctx, unit, a);
	updatedirtime (a, 1);
	if (a->child != 0) {
		write_log (_T("Serious error in action_delete_object.\n"));
		a->deleted = 1;
	} else {
		delete_aino (unit, a);
	}
	PUT_PCK_RES1 (packet, DOS_TRUE);
	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
}

static void	action_set_date(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG2 (packet) << 2;
	uaecptr name = GET_PCK_ARG3 (packet) << 2;
	uaecptr date = GET_PCK_ARG4 (packet);
	a_inode *a;
	struct mytimeval tv;
	int err = 0;

	TRACE((_T("ACTION_SET_DATE(0x%x,\"%s\")\n"), lock, bstr(ctx, unit, name)));

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	a = find_aino(ctx, unit, lock, bstr(ctx, unit, name), &err);
	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	}
	if (a->softlink) {
		handle_softlink(ctx, unit, packet, a);
		return;
	}
	if (!a->vfso) {
		amiga_to_timeval (&tv, trap_get_long(ctx, date), trap_get_long(ctx, date + 4), trap_get_long(ctx, date + 8), 50);
		//write_log (_T("%llu.%u (%d,%d,%d) %s\n"), tv.tv_sec, tv.tv_usec, trap_get_long(ctx, date), trap_get_long(ctx, date + 4), trap_get_long(ctx, date + 8), a->nname);
		if (!my_utime (a->nname, &tv))
			err = dos_errno ();
	}
	if (err != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err);
		return;
	} else {
		notify_check(ctx, unit, a);
		PUT_PCK_RES1 (packet, DOS_TRUE);
	}
	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
}

static void	action_rename_object(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock1 = GET_PCK_ARG1 (packet) << 2;
	uaecptr name1 = GET_PCK_ARG2 (packet) << 2;
	uaecptr lock2 = GET_PCK_ARG3 (packet) << 2;
	uaecptr name2 = GET_PCK_ARG4 (packet) << 2;
	a_inode *a1, *a2;
	int err1, err2;
	Key *k1, *knext;
	int wehavekeys = 0;

	TRACE((_T("ACTION_RENAME_OBJECT(0x%x,\"%s\","), lock1, bstr(ctx, unit, name1)));
	TRACE((_T("0x%x,\"%s\")\n"), lock2, bstr(ctx, unit, name2)));

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	a1 = find_aino(ctx, unit, lock1, bstr(ctx, unit, name1), &err1);
	if (err1 != 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err1);
		return;
	}
	/* rename always fails if file is open for writing */
	for (k1 = unit->keys; k1; k1 = knext) {
		knext = k1->next;
		if (k1->aino == a1 && k1->fd && k1->createmode == 2) {
			PUT_PCK_RES1 (packet, DOS_FALSE);
			PUT_PCK_RES2 (packet, ERROR_OBJECT_IN_USE);
			return;
		}
	}

	/* See whether the other name already exists in the filesystem.  */
	a2 = find_aino(ctx, unit, lock2, bstr(ctx, unit, name2), &err2);

	if (a2 == a1) {
		/* Renaming to the same name, but possibly different case.  */
		if (_tcscmp (a1->aname, bstr_cut(ctx, unit, name2)) == 0) {
			/* Exact match -> do nothing.  */
			notify_check(ctx, unit, a1);
			updatedirtime (a1, 1);
			PUT_PCK_RES1 (packet, DOS_TRUE);
			return;
		}
		a2 = a2->parent;
	} else if (a2 == 0 || err2 != ERROR_OBJECT_NOT_AROUND) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, err2 == 0 ? ERROR_OBJECT_EXISTS : err2);
		return;
	}

	a2 = create_child_aino (unit, a2, bstr_cut(ctx, unit, name2), a1->dir);
	if (a2 == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
		return;
	}

	if (!a1->vfso) {
		if (-1 == my_rename (a1->nname, a2->nname)) {
			int ret = -1;
			/* maybe we have open file handles that caused failure? */
			write_log (_T("rename '%s' -> '%s' failed, trying relocking..\n"), a1->nname, a2->nname);
			wehavekeys = relock_do (unit, a1);
			/* try again... */
			ret = my_rename (a1->nname, a2->nname);
			/* restore locks */
			relock_re (unit, a1, a2, ret == -1 ? 1 : 0);
			if (ret == -1) {
				delete_aino (unit, a2);
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, dos_errno ());
				return;
			}
		}
	}

	notify_check(ctx, unit, a1);
	notify_check(ctx, unit, a2);
	a2->comment = a1->comment;
	a1->comment = 0;
	a2->amigaos_mode = a1->amigaos_mode;
	a2->uniq = a1->uniq;
	a2->elock = a1->elock;
	a2->shlock = a1->shlock;
	a2->has_dbentry = a1->has_dbentry;
	a2->db_offset = a1->db_offset;
	a2->dirty = 0;
	a2->vfso = a1->vfso;
	move_exkeys (unit, a1, a2);
	move_aino_children (unit, a1, a2);
	delete_aino (unit, a1);
	a2->dirty = 1;
	if (a2->parent)
		fsdb_dir_writeback (a2->parent);
	updatedirtime (a2, 1);
	fsdb_set_file_attrs (a2);
	if (a2->elock > 0 || a2->shlock > 0 || wehavekeys > 0)
		de_recycle_aino (unit, a2);
	PUT_PCK_RES1 (packet, DOS_TRUE);
	gui_flicker_led (UNIT_LED(unit), unit->unit, 2);
}

static void	action_current_volume(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	if (filesys_isvolume(unit))
		PUT_PCK_RES1 (packet, unit->volume >> 2);
	else
		PUT_PCK_RES1 (packet, 0);
}

static void	action_rename_disk(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr name = GET_PCK_ARG1 (packet) << 2;

	TRACE((_T("ACTION_RENAME_DISK(\"%s\")\n"), bstr(ctx, unit, name)));

	if (is_writeprotected(unit)) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}

	/* get volume name */
	xfree (unit->ui.volname);
	unit->ui.volname = bstr1(ctx, name);
	set_volume_name(unit, 0);

	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void	action_is_filesystem(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	TRACE((_T("ACTION_IS_FILESYSTEM()\n")));
	PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void	action_flush(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	TRACE((_T("ACTION_FLUSH()\n")));
	PUT_PCK_RES1 (packet, DOS_TRUE);
	flush_cache (unit, 0);
}

static void	action_more_cache(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	TRACE((_T("ACTION_MORE_CACHE()\n")));
	PUT_PCK_RES1 (packet, 50); /* bug but AmigaOS expects it */
	if (GET_PCK_ARG1 (packet) != 0)
		flush_cache (unit, 0);
}

static void	action_inhibit(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	int old = unit->inhibited;
	PUT_PCK_RES1 (packet, DOS_TRUE);
	flush_cache (unit, 0);
	unit->inhibited = GET_PCK_ARG1 (packet) != 0;
	TRACE((_T("ACTION_INHIBIT(%d:%d)\n"), unit->unit, unit->inhibited));
	if (unit->ui.canremove && !unit->ui.configureddrive && unit->ui.open && old && !GET_PCK_ARG1(packet)) {
		filesys_delayed_eject(unit);
	}
}

static void	action_write_protect(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	TRACE((_T("ACTION_WRITE_PROTECT()\n")));
	PUT_PCK_RES1 (packet, DOS_TRUE);
	if (GET_PCK_ARG1 (packet)) {
		if (!unit->ui.locked) {
			unit->ui.locked = true;
			unit->lockkey = GET_PCK_ARG2 (packet);
		}
	} else {
		if (unit->ui.locked) {
			if (unit->lockkey == GET_PCK_ARG2 (packet) || unit->lockkey == 0) {
				unit->ui.locked = false;
			} else {
				PUT_PCK_RES1 (packet, DOS_FALSE);
				PUT_PCK_RES2 (packet, 0);
			}
		}
	}
}

/* OS4 */

#define TAG_DONE   0
#define TAG_IGNORE 1
#define TAG_MORE   2
#define TAG_SKIP   3

static void action_filesystem_attr(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	int versize = 0;
	uaecptr verbuffer = 0;
	uaecptr taglist = GET_PCK_ARG1(packet);
	for (;;) {
		uae_u32 tag = trap_get_long(ctx, taglist);
		uae_u32 tagp = taglist + 4;
		if (tag == TAG_DONE)
			break;
		taglist += 8;
		if (tag == TAG_IGNORE)
			continue;
		if (tag == TAG_MORE) {
			uae_u32 val = trap_get_long(ctx, tagp);
			taglist = val;
			continue;
		}
		if (tag == TAG_SKIP) {
			uae_u32 val = trap_get_long(ctx, tagp);
			taglist += val * 8;
			continue;
		}
		uae_u32 retval = 0;
		bool doret = false;
		switch(tag)
		{
			case 0x80002332: // FSA_MaxFileNameLengthR
			retval = currprefs.filesys_max_name;
			doret = true;
			break;
			case 0x80002334: // FSA_VersionNumberR
			retval = (0 << 16) | (5 << 0);
			doret = true;
			break;
			case 0x80002335: // FSA_DOSTypeR
			retval = get_long(unit->volume + 32);
			doret = true;
			break;
			case 0x80002336: // FSA_ActivityFlushTimeoutR
			case 0x80002338: // FSA_InactivityFlushTimeoutR
			retval = 0;
			doret = true;
			break;
			case 0x8000233a: // FSA_MaxRecycledEntriesR
			case 0x8000233c: // FSA_HasRecycledEntriesR
			retval = 0;
			doret = true;
			break;
			case 0x8000233d: // FSA_VersionStringR
			verbuffer = trap_get_long(ctx, tagp);
			break;
			case 0x8000233e: // FSA_VersionStringR_BufSize
			versize = trap_get_long(ctx, tagp);
			break;
			default:
			write_log(_T("action_filesystem_attr unknown tag %08x\n"), tag);
			PUT_PCK64_RES1(packet, DOS_FALSE);
			PUT_PCK64_RES2(packet, ERROR_NOT_IMPLEMENTED);
			return;
		}
		if (doret)
			trap_put_long(ctx, trap_get_long(ctx, tagp), retval);
		
	}
	if (verbuffer && versize) {
		trap_put_string(ctx, UAEFS_VERSION, verbuffer, versize);
	}
	PUT_PCK_RES1(packet, TRUE);
	PUT_PCK_RES2(packet, 0);
}

static void action_change_file_position64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK64_ARG1 (packet));
	uae_s64 pos = GET_PCK64_ARG2 (packet);
	int mode = (uae_s32)GET_PCK64_ARG3 (packet);
	long whence = SEEK_CUR;
	uae_s64 res, cur;

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (k == 0) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE((_T("ACTION_CHANGE_FILE_POSITION64(%s,%lld,%d)\n"), k->aino->nname, pos, mode));
	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);

	cur = k->file_pos;
	{
		uae_s64 temppos;
		uae_s64 filesize = key_filesize(k);

		if (whence == SEEK_CUR)
			temppos = cur + pos;
		if (whence == SEEK_SET)
			temppos = pos;
		if (whence == SEEK_END)
			temppos = filesize + pos;
		if (filesize < temppos) {
			res = -1;
			PUT_PCK64_RES1 (packet, res);
			PUT_PCK64_RES2 (packet, ERROR_SEEK_ERROR);
			return;
		}
	}
	res = key_seek(k, pos, whence);

	if (-1 == res) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_SEEK_ERROR);
	} else {
		PUT_PCK64_RES1 (packet, TRUE);
		PUT_PCK64_RES2 (packet, 0);
		k->file_pos = key_seek(k, 0, SEEK_CUR);
	}
	TRACE((_T("= oldpos %lld newpos %lld\n"), cur, k->file_pos));
}

static void action_get_file_position64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK64_ARG1 (packet));

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (k == 0) {
		PUT_PCK64_RES1 (packet, -1);
		PUT_PCK64_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}
	TRACE((_T("ACTION_GET_FILE_POSITION64(%s)=%lld\n"), k->aino->nname, k->file_pos));
	PUT_PCK64_RES1 (packet, k->file_pos);
	PUT_PCK64_RES2 (packet, 0);
}

static void action_change_file_size64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k, *k1;
	uae_s64 offset = GET_PCK64_ARG2 (packet);
	int mode = (uae_s32)GET_PCK64_ARG3 (packet);
	int whence = SEEK_CUR;

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE((_T("ACTION_CHANGE_FILE_SIZE64(0x%x, %lld, 0x%x)\n"), GET_PCK64_ARG1 (packet), offset, mode));

	k = lookup_key (unit, GET_PCK64_ARG1 (packet));
	if (k == 0) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}

	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);
	k->notifyactive = 1;
	/* If any open files have file pointers beyond this size, truncate only
	* so far that these pointers do not become invalid.  */
	for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k != k1 && k->aino == k1->aino) {
			if (k1->file_pos > offset)
				offset = k1->file_pos;
		}
	}

	/* Write one then truncate: that should give the right size in all cases.  */
	fs_lseek (k->fd, offset, whence);
	offset = key_seek(k, offset, whence);
	fs_write (k->fd, /* whatever */(uae_u8*)&k1, 1);
	if (k->file_pos > offset)
		k->file_pos = offset;
	key_seek(k, k->file_pos, SEEK_SET);

	if (my_truncate (k->aino->nname, offset) == -1) {
		PUT_PCK64_RES1 (packet, DOS_FALSE);
		PUT_PCK64_RES2 (packet, dos_errno ());
		return;
	}

	PUT_PCK64_RES1 (packet, DOS_TRUE);
	PUT_PCK64_RES2 (packet, 0);
}

static void action_get_file_size64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key (unit, GET_PCK64_ARG1 (packet));
	uae_s64 filesize;

	PUT_PCK64_RES0 (packet, DP64_INIT);

	if (k == 0) {
		PUT_PCK64_RES1 (packet, -1);
		PUT_PCK64_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}
	TRACE((_T("ACTION_GET_FILE_SIZE64(%s)\n"), k->aino->nname));
	filesize = key_filesize(k);
	TRACE((_T("ACTION_GET_FILE_SIZE64(%s)=%lld\n"), k->aino->nname, filesize));
	if (filesize >= 0) {
		PUT_PCK64_RES1 (packet, filesize);
		PUT_PCK64_RES2 (packet, 0);
		return;
	}
	PUT_PCK64_RES1 (packet, -1);
	PUT_PCK64_RES2 (packet, ERROR_SEEK_ERROR);
}

/* MOS */

static void action_examine_object64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	uaecptr lock = GET_PCK_ARG1 (packet) << 2;
	uaecptr info = GET_PCK_ARG2 (packet) << 2;
	a_inode *aino = 0;

	TRACE((_T("ACTION_EXAMINE_OBJECT(0x%x,0x%x)\n"), lock, info));
	DUMPLOCK(ctx, unit, lock);

	if (lock != 0)
		aino = aino_from_lock(ctx, unit, lock);
	if (aino == 0)
		aino = &unit->rootnode;

	get_fileinfo(ctx, unit, packet, info, aino, true);
}

static void action_set_file_size64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k, *k1;
	uae_s64 offset = get_quadp(ctx, GET_PCK_ARG2 (packet));
	int mode = (uae_s32)GET_PCK_ARG3 (packet);
	int whence = SEEK_CUR;

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE((_T("ACTION_SET_FILE_SIZE64(0x%x, %lld, 0x%x)\n"), GET_PCK_ARG1 (packet), offset, mode));

	k = lookup_key (unit, GET_PCK_ARG1 (packet));
	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
		return;
	}

	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);
	k->notifyactive = 1;
	/* If any open files have file pointers beyond this size, truncate only
	* so far that these pointers do not become invalid.  */
	for (k1 = unit->keys; k1; k1 = k1->next) {
		if (k != k1 && k->aino == k1->aino) {
			if (k1->file_pos > offset)
				offset = k1->file_pos;
		}
	}

	/* Write one then truncate: that should give the right size in all cases.  */
	fs_lseek (k->fd, offset, whence);
	offset = key_seek(k, offset, whence);
	fs_write (k->fd, /* whatever */(uae_u8*)&k1, 1);
	if (k->file_pos > offset)
		k->file_pos = offset;
	key_seek(k, k->file_pos, SEEK_SET);

	if (my_truncate (k->aino->nname, offset) == -1) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, dos_errno ());
		return;
	}

	PUT_PCK_RES1 (packet, DOS_TRUE);
	set_quadp(ctx, GET_PCK_ARG4(packet), offset);
}

static void action_seek64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key(unit, GET_PCK_ARG1(packet));
	uae_s64 pos = get_quadp(ctx, GET_PCK64_ARG2(packet));
	int mode = GET_PCK_ARG3(packet);
	long whence = SEEK_CUR;
	uae_s64 res, cur;

	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
		return;
	}

	if (mode > 0)
		whence = SEEK_END;
	if (mode < 0)
		whence = SEEK_SET;

	TRACE((_T("ACTION_SEEK64(%s,%lld,%d)\n"), k->aino->nname, pos, mode));
	gui_flicker_led (UNIT_LED(unit), unit->unit, 1);

	cur = k->file_pos;
	{
		uae_s64 temppos;
		uae_s64 filesize = key_filesize(k);

		if (whence == SEEK_CUR)
			temppos = cur + pos;
		if (whence == SEEK_SET)
			temppos = pos;
		if (whence == SEEK_END)
			temppos = filesize + pos;
		if (filesize < temppos) {
			res = -1;
			PUT_PCK_RES1 (packet, res);
			PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
			return;
		}
	}
	res = key_seek(k, pos, whence);

	if (-1 == res) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
	} else {
		PUT_PCK_RES1 (packet, TRUE);
		set_quadp(ctx, GET_PCK_ARG3(packet), cur);
		k->file_pos = key_seek(k, 0, SEEK_CUR);
	}
	TRACE((_T("= oldpos %lld newpos %lld\n"), cur, k->file_pos));
}

static int action_lock_record64(TrapContext *ctx, Unit *unit, dpacket *packet, uae_u32 msg)
{
	Key *k = lookup_key(unit, GET_PCK_ARG1(packet));
	uae_u64 pos = get_quadp(ctx, GET_PCK_ARG2(packet));
	uae_u64 len = get_quadp(ctx, GET_PCK_ARG3(packet));
	uae_u32 mode = GET_PCK_ARG4(packet);
	uae_u32 timeout = GET_PCK_ARG5(packet);

	bool exclusive = mode == REC_EXCLUSIVE || mode == REC_EXCLUSIVE_IMMED;

	write_log (_T("action_lock_record64('%s',%lld,%lld,%d,%d)\n"), k ? k->aino->nname : _T("null"), pos, len, mode, timeout);

	if (!k || mode > REC_SHARED_IMMED) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_WRONG_TYPE);
		return 1;
	}

	if (mode == REC_EXCLUSIVE_IMMED || mode == REC_SHARED_IMMED)
		timeout = 0;

	if (record_hit (unit, k, pos, len, mode)) {
		if (timeout && msg) {
			// queue it and do not reply
			struct lockrecord *lr = new_record (packet, pos, len, mode, timeout, msg);
			if (unit->waitingrecords) {
				lr->next = unit->waitingrecords;
				unit->waitingrecords = lr;
			} else {
				unit->waitingrecords = lr;
			}
			write_log (_T("-> collision, timeout queued\n"));
			return -1;
		}
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_LOCK_COLLISION);
		write_log (_T("-> ERROR_LOCK_COLLISION\n"));
		return 1;
	}

	struct lockrecord *lr = new_record(packet, pos, len, mode, timeout, 0);
	if (k->record) {
		lr->next = k->record;
		k->record = lr;
	} else {
		k->record = lr;
	}
	PUT_PCK_RES1 (packet, DOS_TRUE);
	write_log (_T("-> OK\n"));
	return 1;
}

static void action_free_record64(TrapContext *ctx, Unit *unit, dpacket *packet)
{
	Key *k = lookup_key(unit, GET_PCK_ARG1(packet));
	uae_u64 pos = get_quadp(ctx, GET_PCK_ARG2(packet));
	uae_u64 len = get_quadp(ctx, GET_PCK_ARG3 (packet));

	write_log (_T("action_free_record('%s',%lld,%lld)\n"), k ? k->aino->nname : _T("null"), pos, len);

	if (!k) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_OBJECT_WRONG_TYPE);
		return;
	}

	struct lockrecord *prev = NULL;
	for (struct lockrecord *lr = k->record; lr; lr = lr->next) {
		if (lr->pos == pos && lr->len == len) {
			if (prev)
				prev->next = lr->next;
			else
				k->record = lr->next;
			xfree (lr);
			write_log (_T("->OK\n"));
			record_check_waiting(ctx, unit);
			PUT_PCK_RES1 (packet, DOS_TRUE);
			return;
		}
	}
	write_log (_T("-> ERROR_RECORD_NOT_LOCKED\n"));
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_RECORD_NOT_LOCKED);
}

/* We don't want multiple interrupts to be active at the same time. I don't
* know whether AmigaOS takes care of that, but this does. */
static uae_sem_t singlethread_int_sem;
static uae_sem_t shellexec_sem;

#define SHELLEXEC_MAX_CMD_LEN 512

static uae_u32 REGPARAM2 exter_int_helper(TrapContext *ctx)
{
	UnitInfo *uip = mountinfo.ui;
	uaecptr port;
	int n = trap_get_dreg(ctx, 0);
	static int unit_no;

	if (n == 20) {
		// d1 = shellexec process
		shell_execute_process = trap_get_dreg(ctx, 1);
		return 0;
	} else if (n == 21) {
		trap_set_areg(ctx, 0, 0);
		if (comm_pipe_has_data(&shellexecute_pipe)) {
			TCHAR *p = (TCHAR*)read_comm_pipe_pvoid_blocking(&shellexecute_pipe);
			if (p) {
				int maxsize = SHELLEXEC_MAX_CMD_LEN - 1;
				if (shell_execute_data) {
					uae_char *src = ua(p);
					uae_u8 *dst = uaeboard_map_ram(shell_execute_data);
					uae_char *srcp = src;
					while (maxsize-- > 0) {
						uae_u8 v = *srcp++;
						*dst++ = v;
						if (!v)
							break;
					}
					*dst = 0;
					xfree(src);
				}
				trap_set_areg(ctx, 0, shell_execute_data);
			}
			xfree(p);
		}
		return 0;
	} else if (n == 22) {
		// ack
		return 0;
	} else if (n >= 30 && n <= 39) {
		return filesys_shellexecute2_process(n, ctx);
	}

	if (n == 1) {
		/* Release a message_lock. This is called as soon as the message is
		* received by the assembly code. We use the opportunity to check
		* whether we have some locks that we can give back to the assembler
		* code.
		* Note that this is called from the main loop, unlike the other cases
		* in this switch statement which are called from the interrupt handler.
		*/
#ifdef UAE_FILESYS_THREADS
		{
			Unit *unit = find_unit(trap_get_areg(ctx, 5));
			uaecptr msg = trap_get_areg(ctx, 4);
			unit->cmds_complete = unit->cmds_acked;
			while (comm_pipe_has_data(unit->ui.back_pipe)) {
				uaecptr locks, lockend, lockv;
				int cnt = 0;
				locks = read_comm_pipe_int_blocking(unit->ui.back_pipe);
				lockend = locks;
				while ((lockv = trap_get_long(ctx, lockend)) != 0) {
					if (lockv == lockend) {
						write_log(_T("filesystem lock queue corrupted!\n"));
						break;
					}
					lockend = lockv;
					cnt++;
				}
				TRACE3((_T("message_lock: %d %x %x %x\n"), cnt, locks, lockend, trap_get_areg(ctx, 3)));
				trap_put_long(ctx, lockend, trap_get_long(ctx, trap_get_areg(ctx, 3)));
				trap_put_long(ctx, trap_get_areg(ctx, 3), locks);
			}
		}
#else
		write_log(_T("exter_int_helper should not be called with arg 1!\n"));
#endif
		return 0;
	}

	if (n == 10) {
		if (uae_sem_trywait(&singlethread_int_sem) != 0) {
			/* Pretend it isn't for us. We might get it again later. */
			do_uae_int_requested();
			return 0;
		}
		filesys_in_interrupt++;
		unit_no = 0;
	}
	if (n >= 10) {

		/* Find work that needs to be done:
		* return d0 = 0: none
		*        d0 = 1: PutMsg(), port in a0, message in a1
		*        d0 = 2: Signal(), task in a1, signal set in d1
		*        d0 = 3: ReplyMsg(), message in a1
		*        d0 = 4: Cause(), interrupt in a1
		*        d0 = 5: Send FileNofication message, port in a0, notifystruct in a1
		*/

#ifdef SUPPORT_THREADS
		/* First, check signals/messages */
		while (comm_pipe_has_data(&native2amiga_pending)) {
			int cmd = read_comm_pipe_int_blocking(&native2amiga_pending);
			if (cmd & 0x80) {
				cmd &= ~0x80;
				// tell the sender that message was processed
				UAE_PROCESSED func = (UAE_PROCESSED)read_comm_pipe_pvoid_blocking(&native2amiga_pending);
				func(cmd);
			}
			switch (cmd) {
				case 0: /* Signal() */
				trap_set_areg(ctx, 1, read_comm_pipe_u32_blocking(&native2amiga_pending));
				trap_set_dreg(ctx, 1, read_comm_pipe_u32_blocking(&native2amiga_pending));
				return 2;

				case 1: /* PutMsg() */
				trap_set_areg(ctx, 0, read_comm_pipe_u32_blocking(&native2amiga_pending));
				trap_set_areg(ctx, 1, read_comm_pipe_u32_blocking(&native2amiga_pending));
				return 1;

				case 2: /* ReplyMsg() */
				trap_set_areg(ctx, 1, read_comm_pipe_u32_blocking(&native2amiga_pending));
				return 3;

				case 3: /* Cause() */
				trap_set_areg(ctx, 1, read_comm_pipe_u32_blocking(&native2amiga_pending));
				return 4;

				case 4: /* NotifyHack() */
				trap_set_areg(ctx, 0, read_comm_pipe_u32_blocking(&native2amiga_pending));
				trap_set_areg(ctx, 1, read_comm_pipe_u32_blocking(&native2amiga_pending));
				return 5;

				case 5: /* shell execute */
				{
					TCHAR *p = (TCHAR*)read_comm_pipe_pvoid_blocking(&native2amiga_pending);
					write_comm_pipe_pvoid(&shellexecute_pipe, p, 0);
					if (shell_execute_data) {
						if (!shell_execute_process)
							break;
						trap_set_areg(ctx, 1, shell_execute_process - 92);
						trap_set_dreg(ctx, 1, 1 << 13);
						return 2; // signal process
					}
					shell_execute_data = uaeboard_alloc_ram(SHELLEXEC_MAX_CMD_LEN);
					return 6; // create process
				}

				case 6: /* shell execute 2 */
				{
					trap_set_areg(ctx, 0, read_comm_pipe_u32_blocking(&native2amiga_pending));
					return 7;
				}

				default:
				write_log(_T("exter_int_helper: unknown native action %X\n"), cmd);
				break;
			}
		}
#endif

		/* Find some unit that needs a message sent, and return its port,
		 * or zero if all are done.
		 * Take care not to dereference self for units that didn't have their
		 * startup packet sent. */
		for (;;) {
			if (unit_no >= MAX_FILESYSTEM_UNITS)
				goto end;

			if (uip[unit_no].open > 0 && uip[unit_no].self != 0
				&& uip[unit_no].self->cmds_acked == uip[unit_no].self->cmds_complete
				&& uip[unit_no].self->cmds_acked != uip[unit_no].self->cmds_sent)
				break;
			unit_no++;
		}
		uip[unit_no].self->cmds_acked = uip[unit_no].self->cmds_sent;
		port = uip[unit_no].self->port;
		if (port) {
			trap_set_areg(ctx, 0, port);
			trap_set_areg(ctx, 1, find_unit(port)->dummy_message);
			unit_no++;
			return 1;
		}

end:
		/* Exit the interrupt, and release the single-threading lock. */
		filesys_in_interrupt--;
		uae_sem_post(&singlethread_int_sem);

	}
	return 0;
}

static int handle_packet(TrapContext *ctx, Unit *unit, dpacket *pck, uae_u32 msg, int isvolume)
{
	bool noidle = false;
	int ret = 1;
	uae_s32 type = GET_PCK_TYPE (pck);
	PUT_PCK_RES2 (pck, 0);

	TRACE((_T("unit=%p packet=%d\n"), unit, type));
	if (unit->inhibited && isvolume
		&& type != ACTION_INHIBIT && type != ACTION_MORE_CACHE
		&& type != ACTION_DISK_INFO) {
			PUT_PCK_RES1 (pck, DOS_FALSE);
			PUT_PCK_RES2 (pck, ERROR_NOT_A_DOS_DISK);
			return 1;
	}
	if (type != ACTION_INHIBIT && type != ACTION_CURRENT_VOLUME
		&& type != ACTION_IS_FILESYSTEM && type != ACTION_MORE_CACHE
		&& type != ACTION_WRITE_PROTECT && type != ACTION_DISK_INFO
		&& !isvolume) {
			PUT_PCK_RES1 (pck, DOS_FALSE);
			PUT_PCK_RES2 (pck, unit->ui.unknown_media ? ERROR_NOT_A_DOS_DISK : ERROR_NO_DISK);
			return 1;
	}

	switch (type) {
	case ACTION_LOCATE_OBJECT: action_lock (ctx, unit, pck); break;
	case ACTION_FREE_LOCK: action_free_lock (ctx, unit, pck); break;
	case ACTION_COPY_DIR: action_dup_lock (ctx, unit, pck); break;
	case ACTION_DISK_INFO: action_disk_info (ctx, unit, pck); break;
	case ACTION_INFO: action_info (ctx, unit, pck); break;
	case ACTION_EXAMINE_OBJECT: action_examine_object (ctx, unit, pck); break;
	case ACTION_EXAMINE_NEXT: noidle = true; action_examine_next (ctx, unit, pck, false); break;
	case ACTION_FIND_INPUT: action_find_input (ctx, unit, pck); break;
	case ACTION_FIND_WRITE: action_find_write (ctx, unit, pck); break;
	case ACTION_FIND_OUTPUT: action_find_output (ctx, unit, pck); break;
	case ACTION_END: action_end (ctx, unit, pck); break;
	case ACTION_READ: noidle = true; action_read(ctx, unit, pck); break;
	case ACTION_WRITE: noidle = true; action_write (ctx, unit, pck); break;
	case ACTION_SEEK: action_seek (ctx, unit, pck); break;
	case ACTION_SET_PROTECT: action_set_protect (ctx, unit, pck); break;
	case ACTION_SET_COMMENT: action_set_comment (ctx, unit, pck); break;
	case ACTION_SAME_LOCK: action_same_lock (ctx, unit, pck); break;
	case ACTION_PARENT: action_parent (ctx, unit, pck); break;
	case ACTION_CREATE_DIR: action_create_dir (ctx, unit, pck); break;
	case ACTION_DELETE_OBJECT: action_delete_object (ctx, unit, pck); break;
	case ACTION_RENAME_OBJECT: action_rename_object (ctx, unit, pck); break;
	case ACTION_SET_DATE: action_set_date (ctx, unit, pck); break;
	case ACTION_CURRENT_VOLUME: action_current_volume (ctx, unit, pck); break;
	case ACTION_RENAME_DISK: action_rename_disk (ctx, unit, pck); break;
	case ACTION_IS_FILESYSTEM: action_is_filesystem (ctx, unit, pck); break;
	case ACTION_FLUSH: action_flush (ctx, unit, pck); break;
	case ACTION_MORE_CACHE: action_more_cache (ctx, unit, pck); break;
	case ACTION_INHIBIT: action_inhibit (ctx, unit, pck); break;
	case ACTION_WRITE_PROTECT: action_write_protect (ctx, unit, pck); break;

		/* 2.0+ packet types */
	case ACTION_SET_FILE_SIZE: action_set_file_size (ctx, unit, pck); break;
	case ACTION_EXAMINE_FH: action_examine_fh (ctx, unit, pck, false); break;
	case ACTION_FH_FROM_LOCK: action_fh_from_lock (ctx, unit, pck); break;
	case ACTION_COPY_DIR_FH: action_lock_from_fh (ctx, unit, pck); break;
	case ACTION_CHANGE_MODE: action_change_mode (ctx, unit, pck); break;
	case ACTION_PARENT_FH: action_parent_fh (ctx, unit, pck); break;
	case ACTION_ADD_NOTIFY: action_add_notify (ctx, unit, pck); break;
	case ACTION_REMOVE_NOTIFY: action_remove_notify (ctx, unit, pck); break;
	case ACTION_EXAMINE_ALL: noidle = true; ret = action_examine_all(ctx, unit, pck); break;
	case ACTION_EXAMINE_ALL_END: ret = action_examine_all_end(ctx, unit, pck); break;
	case ACTION_LOCK_RECORD: ret = action_lock_record (ctx, unit, pck, msg); break;
	case ACTION_FREE_RECORD: action_free_record (ctx, unit, pck); break;
	case ACTION_READ_LINK: action_read_link (ctx, unit, pck); break;
	case ACTION_MAKE_LINK: action_make_link (ctx, unit, pck); break;

		/* OS4 packet types */
	case ACTION_FILESYSTEM_ATTR: action_filesystem_attr(ctx, unit, pck); break;
	case ACTION_CHANGE_FILE_POSITION64: action_change_file_position64 (ctx, unit, pck); break;
	case ACTION_GET_FILE_POSITION64: action_get_file_position64 (ctx, unit, pck); break;
	case ACTION_CHANGE_FILE_SIZE64: action_change_file_size64 (ctx, unit, pck); break;
	case ACTION_GET_FILE_SIZE64: action_get_file_size64 (ctx, unit, pck); break;

		/* MOS packet types */
	case ACTION_SEEK64: action_seek64(ctx, unit, pck); break;
	case ACTION_SET_FILE_SIZE64: action_set_file_size64(ctx, unit, pck); break;
	case ACTION_EXAMINE_OBJECT64: action_examine_object64(ctx, unit, pck); break;
	case ACTION_EXAMINE_NEXT64: noidle = true; action_examine_next(ctx, unit, pck, true); break;
	case ACTION_EXAMINE_FH64: action_examine_fh(ctx, unit, pck, true); break;
	case ACTION_LOCK_RECORD64: ret = action_lock_record64(ctx, unit, pck, msg); break;
	case ACTION_FREE_RECORD64: action_free_record64(ctx, unit, pck); break;

		/* unsupported packets */
	case ACTION_FORMAT:
		write_log (_T("FILESYS: UNSUPPORTED PACKET %x\n"), type);
		return 0;
	default:
		write_log (_T("FILESYS: UNKNOWN PACKET %x\n"), type);
		return 0;
	}
	if (noidle) {
		m68k_cancel_idle();
	}
	return ret;
}

#ifdef UAE_FILESYS_THREADS

static int filesys_iteration(UnitInfo *ui)
{
	uaecptr pck;
	uaecptr msg;
	uae_u32 morelocks;
	TrapContext *ctx;

	ctx = (TrapContext*)read_comm_pipe_pvoid_blocking(ui->unit_pipe);
	pck = read_comm_pipe_u32_blocking(ui->unit_pipe);
	msg = read_comm_pipe_u32_blocking(ui->unit_pipe);
	morelocks = (uae_u32)read_comm_pipe_int_blocking(ui->unit_pipe);

	if (ui->reset_state == FS_GO_DOWN) {
		trap_background_set_complete(ctx);
		if (pck != 0)
		   return 1;
		/* Death message received. */
		uae_sem_post (&ui->reset_sync_sem);
		/* Die.  */
		return 0;
	}

	dpacket packet;
	readdpacket(ctx, &packet, pck);

	int isvolume = 0;
#if TRAPMD
	trapmd md[] = {
		{ TRAPCMD_GET_LONG, { morelocks }, 2, 0 },
		{ TRAPCMD_GET_LONG, { ui->self->locklist }, 2, 1 },
		{ TRAPCMD_PUT_LONG },
		{ TRAPCMD_PUT_LONG, { ui->self->locklist, morelocks }},
		{ (uae_u16)(ui->self->volume ? TRAPCMD_GET_BYTE : TRAPCMD_NOP), { ui->self->volume + 64 }},
	};
	trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));

	if (ui->self->volume) {
		isvolume = md[4].params[0] || ui->self->ui.unknown_media;
	}
#else
	trap_put_long(ctx, trap_get_long(ctx, morelocks), trap_get_long(ctx, ui->self->locklist));
	trap_put_long(ctx, ui->self->locklist, morelocks);
	if (ui->self->volume) {
		isvolume = trap_get_byte(ctx, ui->self->volume + 64) || ui->self->ui.unknown_media;
	}
#endif

	int ret = handle_packet(ctx, ui->self, &packet, msg, isvolume);
	if (!ret) {
		PUT_PCK_RES1 (&packet, DOS_FALSE);
		PUT_PCK_RES2 (&packet, ERROR_ACTION_NOT_KNOWN);
	}
	writedpacket(ctx, &packet);

	trapmd md2[] = {
		{ TRAPCMD_PUT_LONG, { msg + 4, 0xffffffff } },
		{ TRAPCMD_GET_LONG, { ui->self->locklist } },
		{ TRAPCMD_PUT_LONG, { ui->self->locklist, 0 } }
	};
	struct trapmd *mdp;
	int mdcnt;
	if (ret >= 0) {
		mdp = &md2[0];
		mdcnt = 3;
		/* Mark the packet as processed for the list scan in the assembly code. */
		//trap_put_long(ctx, msg + 4, 0xffffffff);
	} else {
		mdp = &md2[1];
		mdcnt = 2;
	}
	/* Acquire the message lock, so that we know we can safely send the message. */
	ui->self->cmds_sent++;

	/* Send back the locks. */
	trap_multi(ctx, mdp, mdcnt);
	if (md2[1].params[0] != 0)
		write_comm_pipe_int(ui->back_pipe, (int)md2[1].params[0], 0);

	/* The message is sent by our interrupt handler, so make sure an interrupt happens. */
	do_uae_int_requested();
#if 0
	uae_u32 v = trap_get_long(ctx, ui->self->locklist);
	if (v != 0)
		write_comm_pipe_int (ui->back_pipe, (int)v, 0);
	trap_put_long(ctx, ui->self->locklist, 0);
#endif

	trap_background_set_complete(ctx);
	return 1;
}


static int filesys_thread (void *unit_v)
{
	UnitInfo *ui = (UnitInfo *)unit_v;

	uae_set_thread_priority (NULL, 1);
	for (;;) {
		if (!filesys_iteration(ui)) {
			return 0;
		}
	}
}
#endif

/* Talk about spaghetti code... */
static uae_u32 REGPARAM2 filesys_handler(TrapContext *ctx)
{
	bool packet_valid = false;
	Unit *unit = find_unit(trap_get_areg(ctx, 5));
	uaecptr packet_addr = trap_get_dreg(ctx, 3);
	uaecptr message_addr = trap_get_areg(ctx, 4);

	if (!trap_valid_address(ctx, packet_addr, 36) || !trap_valid_address(ctx, message_addr, 14)) {
		write_log (_T("FILESYS: Bad address %x/%x passed for packet.\n"), packet_addr, message_addr);
		goto error2;
	}

	if (!unit || !unit->volume) {
		write_log (_T("FILESYS: was not initialized.\n"));
		goto error;
	}
#ifdef UAE_FILESYS_THREADS
	{
		uae_u32 morelocks;
		if (!unit->ui.unit_pipe)
			goto error;
		/* Get two more locks and hand them over to the other thread. */

#if TRAPMD
		struct trapmd md[] = {
			// morelocks = trap_get_long(ctx, trap_get_areg(ctx, 3));
			/* 0 */ { TRAPCMD_GET_LONG, { trap_get_areg(ctx, 3) }, 1, 0 },
			// morelocksptr = trap_get_long(ctx, morelocks)
			/* 1 */ { TRAPCMD_GET_LONG, { 0 } },
			// result 1 to index 4
			/* 2 */ { TRAPCMD_NOP, { 0 }, 4, 0 },
			// result 1 to index 6
			/* 3 */ { TRAPCMD_NOP, { 0 }, 6, 0 },
			// trap_get_long(ctx, morelocksptr)
			/* 4 */ { TRAPCMD_GET_LONG, { 0 }, 5, 1 },
			// trap_put_long(ctx, trap_get_areg(ctx, 3), result 4
			/* 5 */ { TRAPCMD_PUT_LONG, { trap_get_areg(ctx, 3) } },
			// trap_put_long(ctx, morelocksptr, 0);
			/* 6 */ { TRAPCMD_PUT_LONG, { 0, 0 } },
			// trap_put_long(ctx, message_addr + 4, 0);
			/* 7 */ { TRAPCMD_PUT_LONG, { message_addr + 4, 0 } }
		};
		trap_multi(ctx, md, sizeof md / sizeof(struct trapmd));
		morelocks = md[0].params[0];
#else
		uae_u32 morelocksptr;
		morelocks = trap_get_long(ctx, trap_get_areg(ctx, 3));
		morelocksptr = trap_get_long(ctx, morelocks);
		trap_put_long(ctx, trap_get_areg(ctx, 3), trap_get_long(ctx, morelocksptr));
		trap_put_long(ctx, morelocksptr, 0);

		/* The packet wasn't processed yet. */
		trap_put_long(ctx, message_addr + 4, 0);
#endif

		trap_set_background(ctx);
		write_comm_pipe_pvoid(unit->ui.unit_pipe, ctx, 0);
		write_comm_pipe_u32(unit->ui.unit_pipe, packet_addr, 0);
		write_comm_pipe_u32(unit->ui.unit_pipe, message_addr, 0);
		write_comm_pipe_int(unit->ui.unit_pipe, (int)morelocks, 1);
		/* Don't reply yet. */
		return 1;
	}
#endif

	dpacket packet;
	readdpacket(ctx, &packet, packet_addr);
	packet_valid = true;

	if (! handle_packet(ctx, unit, &packet, 0, filesys_isvolume(unit))) {
error:
		if (!packet_valid)
			readdpacket(ctx, &packet, packet_addr);
		PUT_PCK_RES1 (&packet, DOS_FALSE);
		PUT_PCK_RES2 (&packet, ERROR_ACTION_NOT_KNOWN);
	}
	TRACE((_T("reply: %8x, %d\n"), GET_PCK_RES1(&packet), GET_PCK_RES2(&packet)));

	writedpacket(ctx, &packet);

error2:
	trap_put_long(ctx, message_addr + 4, 0xffffffff);

	return 0;
}

void filesys_start_threads (void)
{
	int i;

	filesys_in_interrupt = 0;
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		UnitInfo *ui = &mountinfo.ui[i];
		if (ui->open <= 0)
			continue;
		filesys_start_thread (ui, i);
	}
}

void filesys_free_handles (void)
{
	Unit *u, *u1;
	for (u = units; u; u = u1) {
		Key *k1, *knext;
		u1 = u->next;
		for (k1 = u->keys; k1; k1 = knext) {
			knext = k1->next;
			if (k1->fd)
				fs_closefile (k1->fd);
			xfree (k1);
		}
		u->keys = NULL;
		struct lockrecord *lrnext;
		for (struct lockrecord *lr = u->waitingrecords; lr; lr = lrnext) {
			lrnext = lr->next;
			xfree (lr);
		}
		u->waitingrecords = NULL;
		free_all_ainos (u, &u->rootnode);
		u->rootnode.next = u->rootnode.prev = &u->rootnode;
		u->aino_cache_size = 0;
		xfree (u->newrootdir);
		xfree (u->newvolume);
		u->newrootdir = NULL;
		u->newvolume = NULL;
	}
}

static void free_shellexecute(void)
{
	shell_execute_data = 0;
	shell_execute_process = 0;
	shellexecute2_queued = 0;
	for (int i = 0; i < SHELLEXEC_MAX; i++) {
		struct ShellExecute2 *se2 = &shellexecute2[i];
		shellexecute2_free(se2);
	}
}

static void filesys_reset2 (void)
{
	Unit *u, *u1;


	filesys_free_handles ();
	for (u = units; u; u = u1) {
		u1 = u->next;
		xfree (u);
	}
	units = 0;
	key_uniq = 0;
	a_uniq = 0;
	free_mountinfo ();
}

void filesys_reset (void)
{
	if (isrestore ())
		return;
	load_injected_icons();
	filesys_reset2 ();
	initialize_mountinfo ();
	free_shellexecute();
}

static void filesys_prepare_reset2 (void)
{
	UnitInfo *uip;
	int i;

	uip = mountinfo.ui;
#ifdef UAE_FILESYS_THREADS
	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		if (uip[i].open > 0 && uip[i].unit_pipe != 0) {
			uae_sem_init (&uip[i].reset_sync_sem, 0, 0);
			uip[i].reset_state = FS_GO_DOWN;
			/* send death message */
			write_comm_pipe_pvoid(uip[i].unit_pipe, NULL, 0);
			write_comm_pipe_int(uip[i].unit_pipe, 0, 0);
			write_comm_pipe_int(uip[i].unit_pipe, 0, 0);
			write_comm_pipe_int(uip[i].unit_pipe, 0, 1);
			uae_sem_wait (&uip[i].reset_sync_sem);
			uae_end_thread (&uip[i].tid);
		}
	}
#endif
	filesys_free_handles();
#if 0
	Unit *u;
	u = units;
	while (u != 0) {
		free_all_ainos (u, &u->rootnode);
		u->rootnode.next = u->rootnode.prev = &u->rootnode;
		u->aino_cache_size = 0;
		u = u->next;
	}
#endif
}

void filesys_prepare_reset (void)
{
	if (isrestore ())
		return;
	filesys_prepare_reset2 ();
}


/* don't forget filesys.asm! */
#define PP_MAXSIZE 4 * 96
#define PP_FSSIZE 400
#define PP_FSPTR 404
#define PP_ADDTOFSRES 408
#define PP_FSRES 412
#define PP_FSRES_CREATED 416
#define PP_DEVICEPROC 420
#define PP_EXPLIB 424
#define PP_FSHDSTART 428

static int trackdisk_hack_state;
static int putmsg_hack_state;
static int putmsg_hack_filesystemtask;
static uae_u32 ks12hack_deviceproc;

static bool bcplonlydos(void)
{
	return kickstart_version && kickstart_version < 33;
}

static const uae_u8 bootblock_ofs[] = {
	0x44,0x4f,0x53,0x00,0xc0,0x20,0x0f,0x19,0x00,0x00,0x03,0x70,0x43,0xfa,0x00,0x18,
	0x4e,0xae,0xff,0xa0,0x4a,0x80,0x67,0x0a,0x20,0x40,0x20,0x68,0x00,0x16,0x70,0x00,
	0x4e,0x75,0x70,0xff,0x60,0xfa,0x64,0x6f,0x73,0x2e,0x6c,0x69,0x62,0x72,0x61,0x72,
	0x79
};
static uae_u32 REGPARAM2 filesys_putmsg_return(TrapContext *ctx)
{
	uaecptr message = trap_get_areg(ctx, 1);
	uaecptr dospacket = trap_get_long(ctx, message + 10);
	UnitInfo *uip = mountinfo.ui;
	if (!ks12hack_deviceproc && uip[0].parmpacket)
		ks12hack_deviceproc = trap_get_long(ctx, uip[0].parmpacket + PP_DEVICEPROC);
	if (ks12hack_deviceproc) {
		uae_u32 port = ks12hack_deviceproc;
		if (port) {
			uaecptr proc = trap_get_long(ctx, trap_get_long(ctx, 4) + 276); // ThisTask
			trap_put_long(ctx, proc + 168, port); // pr_FileSystemTask
			trap_set_areg(ctx, 0, port);
			write_log(_T("Pre-KS 1.3 automount hack: patch boot handler process. DP=%08x Proc %08x pr_FileSystemTask=%08x.\n"), dospacket, proc, port);
		}
	}
	return trap_get_dreg(ctx, 0);
}

static uae_u32 REGPARAM2 filesys_putmsg(TrapContext *ctx)
{
	trap_set_areg(ctx, 7, trap_get_areg(ctx, 7) - 4);
	trap_put_long(ctx, trap_get_areg(ctx, 7), trap_get_long(ctx, ROM_filesys_putmsg_original));
	if (putmsg_hack_state) {
		uaecptr message = trap_get_areg(ctx, 1);
		uaecptr dospacket = trap_get_long(ctx, message + 10);
		if (dospacket && !(dospacket & 3) && trap_valid_address(ctx, dospacket, 48)) {
			int type = trap_get_long(ctx, dospacket + 8);
//			write_log(_T("Port=%08x Msg=%08x DP=%08x dp_Link=%08x dp_Port=%08x dp_Type=%d\n"),
//				m68k_areg(regs, 0), m68k_areg(regs, 1), dospacket, get_long(dospacket), get_long(dospacket + 4), type);
			if (type == ACTION_LOCATE_OBJECT) {
				write_log(_T("Pre-KS 1.3 automount hack: init drives.\n"));
				putmsg_hack_state = 0;
				if (putmsg_hack_filesystemtask) {
					trap_set_areg(ctx, 7, trap_get_areg(ctx, 7) - 4);
					trap_put_long(ctx, trap_get_areg(ctx, 7), ROM_filesys_putmsg_return);
				}
				trap_set_areg(ctx, 7, trap_get_areg(ctx, 7) - 4);
				trap_put_long(ctx, trap_get_areg(ctx, 7), filesys_initcode);
				trap_set_areg(ctx, 7, trap_get_areg(ctx, 7) - 4);
				trap_put_long(ctx, trap_get_areg(ctx, 7), ROM_filesys_hack_remove);
				return trap_get_dreg(ctx, 0);
			}
		}
	}
	return trap_get_dreg(ctx, 0);
}

static uae_u32 REGPARAM2 filesys_doio(TrapContext *ctx)
{
	uaecptr ioreq = trap_get_areg(ctx, 1);
	uaecptr unit = trap_get_long(ctx, ioreq + 24); // io_Unit
	if (trackdisk_hack_state && unit && trap_valid_address(ctx, unit, 14)) {
		uaecptr name = trap_get_long(ctx, unit + 10); // ln_Name
		if (name && trap_valid_address(ctx, name, 20)) {
			uae_u8 *addr = get_real_address(name);
			if (!memcmp(addr, "trackdisk.device", 17)) {
				int cmd = trap_get_word(ctx, ioreq + 28); // io_Command
				uaecptr data = trap_get_long(ctx, ioreq + 40);
				int len = trap_get_long(ctx, ioreq + 36);
				//write_log(_T("%08x %d\n"), ioreq, cmd);
				switch (cmd)
				{
				case 2: // CMD_READ
				{
					// trackdisk.device reading boot block
					uae_u8 *d = get_real_address(data);
					memset(d, 0, 1024);
					memcpy(d, bootblock_ofs, sizeof bootblock_ofs);
					trap_put_long(ctx, ioreq + 32, len); // io_Actual
					trackdisk_hack_state = 0;
					write_log(_T("Pre-KS 1.3 automount hack: DF0: boot block faked.\n"));
				}
				break;
				case 9: // TD_MOTOR
					trap_put_long(ctx, ioreq + 32, trackdisk_hack_state < 0 ? 0 : 1);
					trackdisk_hack_state = len ? 1 : -1;
				break;
				case 13: // TD_CHANGENUM
					trap_put_long(ctx, ioreq + 32, 1); // io_Actual
				break;
				case 14: // TD_CHANGESTATE
					trap_put_long(ctx, ioreq + 32, 0);
				break;
				}
				return 0;
			}
		}
	}
	trap_set_areg(ctx, 7, trap_get_areg(ctx, 7) - 4);
	trap_put_long(ctx, trap_get_areg(ctx, 7), trap_get_long(ctx, ROM_filesys_doio_original));
	return 0;
}

static uaecptr add_resident(TrapContext *ctx, uaecptr resaddr, uaecptr myres)
{
	uaecptr sysbase, reslist, prevjmp, resptr;

	uae_s8 myrespri = trap_get_byte(ctx, myres + 13); // rt_Pri

	sysbase = trap_get_long(ctx, 4);
	prevjmp = 0;
	reslist = trap_get_long(ctx, sysbase + 300); // ResModules
	for (;;) {
		resptr = trap_get_long(ctx, reslist);
		if (!resptr)
			break;
		if (resptr & 0x80000000) {
			prevjmp = reslist;
			reslist = resptr & 0x7fffffff;
			continue;
		}
		uae_s8 respri = trap_get_byte(ctx, resptr + 13); // rt_Pri
		uaecptr resname = trap_get_long(ctx, resptr + 14); // rt_Name
		if (resname) {
			uae_char resnamebuf[256];
			trap_get_string(ctx, resnamebuf, resname, sizeof resnamebuf);
			if (myrespri >= respri)
				break;
		}
		prevjmp = 0;
		reslist += 4;
	}
	if (prevjmp) {
		trap_put_long(ctx, prevjmp, 0x80000000 | resaddr);
	} else {
		trap_put_long(ctx, reslist, 0x80000000 | resaddr);
	}
	trap_put_long(ctx, resaddr, myres);
	trap_put_long(ctx, resaddr + 4, resptr);
	trap_put_long(ctx, resaddr + 8, 0x80000000 | (reslist + 4));
	resaddr += 3 * 4;
	return resaddr;
}

static void kill_ide(TrapContext *ctx)
{
	if (!currprefs.scsidevicedisable) {
		return;
	}
	if (gayle_ide_in_use) {
		return;
	}
	if (kickstart_version < 36 || kickstart_version >= 50) {
		return;
	}
	if (trap_is_indirect()) {
		return;
	}
	uaecptr eb = trap_get_areg(ctx, 6);
	uaecptr rm = trap_get_long(ctx, eb + 300);
	for(;;) {
		uaecptr addr = trap_get_long(ctx, rm);
		if (!addr) {
			break;
		}
		if (addr & 0x80000000) {
			rm = addr & 0x7fffffff;
			continue;
		}
		if ((addr >= 0x00a80000 && addr < 0x00b80000) || (addr >= 0x00f00000 && addr < 0x01000000)) {
			uae_u8 res[22];
			if (trap_valid_address(ctx, addr, sizeof(res))) {
				trap_get_bytes(ctx, res, addr, sizeof(res));
				if (res[0] == 0x4a && res[1] == 0xfc && res[10] == 0x01 && res[11] >= 36 && res[11] < 50 && res[12] == 0x03 && res[13] > 0 && res[13] < 128) {
					static const char sd[] = { "scsi.device" };
					uaecptr name = (res[14] << 24) | (res[15] << 16) | (res[16] << 8) | (res[17] << 0);
					int i;
					for (i = 0; i < strlen(sd) + 1; i++) {
						uae_u8 c = trap_get_byte(ctx, name + i);
						if (c != sd[i] || c == 0) {
							break;
						}
					}
					if (i == strlen(sd)) {
						uaecptr desc = (res[18] << 24) | (res[19] << 16) | (res[20] << 8) | (res[21] << 0);
						trap_get_bytes(ctx, res, desc, 4);
						if (res[0] != 'S' || res[1] != 'C' || res[2] != 'S' || res[3] != 'I') {
							write_log(_T("scsi.device resmodules entry disabled\n"));
							trap_put_long(ctx, rm, 0x80000000 | (rm + 4));
							break;
						}
					}
				}
			}
		}
		rm += 4;
	}
}

static uae_u32 REGPARAM2 filesys_diagentry (TrapContext *ctx)
{
	UnitInfo *uip = mountinfo.ui;
	uaecptr resaddr = trap_get_areg(ctx, 2);
	uaecptr expansion = trap_get_areg(ctx, 5);
	uaecptr first_resident, last_resident, tmp;
	uaecptr resaddr_hack = 0;
	uae_u8 *baseaddr;

	filesys_configdev = trap_get_areg(ctx, 3);

	if (currprefs.uaeboard > 1) {
		baseaddr = uaeboard_bank.baseaddr;
		resaddr += 0x100;
	} else {
		baseaddr = filesys_bank.baseaddr;
		resaddr += 0x10;
	}

	if (baseaddr) {
		put_long_host(baseaddr + 0x2100, EXPANSION_explibname);
		put_long_host(baseaddr + 0x2104, filesys_configdev);
		put_long_host(baseaddr + 0x2108, EXPANSION_doslibname);
		put_word_host(baseaddr + 0x210c, 0);
		put_word_host(baseaddr + 0x210e, nr_units());
		put_word_host(baseaddr + 0x2110, 0);
		put_word_host(baseaddr + 0x2112, 1 | (currprefs.uae_hide_autoconfig || currprefs.uaeboard > 1 ? 16 : 0));
	}

	native2amiga_startup();

	write_log (_T("filesystem: diagentry %08x configdev %08x\n"), resaddr, filesys_configdev);

	first_resident = resaddr;
	if (ROM_hardfile_resid != 0) {
		/* Build a struct Resident. This will set up and initialize
		* the uae.device */
		trap_put_word(ctx, resaddr + 0x0, 0x4AFC);
		trap_put_long(ctx, resaddr + 0x2, resaddr);
		trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
		trap_put_word(ctx, resaddr + 0xA, 0x8132); /* RTF_AUTOINIT|RTF_COLDSTART; Version 50 */
		trap_put_word(ctx, resaddr + 0xC, 0x0305); /* NT_DEVICE; pri 05 */
		trap_put_long(ctx, resaddr + 0xE, ROM_hardfile_resname);
		trap_put_long(ctx, resaddr + 0x12, ROM_hardfile_resid);
		trap_put_long(ctx, resaddr + 0x16, ROM_hardfile_init); /* calls filesys_init */
	}
	resaddr += 0x1A;
	if (!KS12_BOOT_HACK || expansion)
		first_resident = resaddr;
	/* The good thing about this function is that it always gets called
	* when we boot. So we could put all sorts of stuff that wants to be done
	* here.
	* We can simply add more Resident structures here. Although the Amiga OS
	* only knows about the one at address DiagArea + 0x10, we scan for other
	* Resident structures and inject them to ResList in priority order
	*/

	// KS 2.x RTF_AFTERDOS is broken
	if (kickstart_version >= 39) {
		trap_put_word(ctx, resaddr + 0x0, 0x4afc);
		trap_put_long(ctx, resaddr + 0x2, resaddr);
		trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A);
		trap_put_word(ctx, resaddr + 0xA, 0x0432); /* RTF_AFTERDOS; Version 50 */
		trap_put_word(ctx, resaddr + 0xC, 0x0000 | AFTERDOS_INIT_PRI); /* NT_UNKNOWN; pri */
		trap_put_long(ctx, resaddr + 0xE, afterdos_name);
		trap_put_long(ctx, resaddr + 0x12, afterdos_id);
		trap_put_long(ctx, resaddr + 0x16, afterdos_initcode);
		resaddr += 0x1A;
	}

	resaddr = uaeres_startup(ctx, resaddr);
#ifdef BSDSOCKET
	resaddr = bsdlib_startup(ctx, resaddr);
#endif
#ifdef WITH_UAENATIVE
	resaddr = uaenative_startup(ctx, resaddr);
#endif
#ifdef SCSIEMU
	resaddr = scsidev_startup(ctx, resaddr);
#endif
#ifdef SANA2
	resaddr = netdev_startup(ctx, resaddr);
#endif
#ifdef UAESERIAL
	resaddr = uaeserialdev_startup(ctx, resaddr);
#endif
#ifdef WITH_TABLETLIBRARY
	resaddr = tabletlib_startup(ctx, resaddr);
#endif
#ifdef RETROPLATFORM
	if (rp_isactive()) {
		trap_put_word(ctx, resaddr + 0x0, 0x4afc);
		trap_put_long(ctx, resaddr + 0x2, resaddr);
		trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A);
		trap_put_word(ctx, resaddr + 0xA, 0x0100); /* RTF_COLDSTART; Version 0 */
		trap_put_word(ctx, resaddr + 0xC, 0x0000); /* NT_UNKNOWN; pri */
		trap_put_long(ctx, resaddr + 0xE, keymaphook_name);
		trap_put_long(ctx, resaddr + 0x12, keymaphook_id);
		trap_put_long(ctx, resaddr + 0x16, keymaphook_initcode);
		resaddr += 0x1A;
	}
#endif

	last_resident = resaddr;

	/* call setup_exter */
	trap_put_word(ctx, resaddr +  0, 0x7000 | (currprefs.uaeboard > 1 ? 3 : 1)); /* moveq #x,d0 */
	trap_put_word(ctx, resaddr +  2, 0x2079); /* move.l RTAREA_BASE+setup_exter,a0 */
	trap_put_long(ctx, resaddr +  4, rtarea_base + bootrom_header + 4 + 5 * 4);
	trap_put_word(ctx, resaddr +  8, 0xd1fc); /* add.l #RTAREA_BASE+bootrom_header,a0 */
	trap_put_long(ctx, resaddr + 10, rtarea_base + bootrom_header);
	trap_put_word(ctx, resaddr + 14, 0x4e90); /* jsr (a0) */
	resaddr += 16;

	trackdisk_hack_state = 0;
	putmsg_hack_state = 0;
	putmsg_hack_filesystemtask = 0;
	ks12hack_deviceproc = 0;

	if (KS12_BOOT_HACK && nr_units() && filesys_configdev == 0 && baseaddr) {
		resaddr_hack = resaddr;
		putmsg_hack_state = -1;
		if (uip[0].bootpri > -128) {
			resaddr += 2 * 22;
			trackdisk_hack_state = -1;
			putmsg_hack_filesystemtask = 1;
		} else {
			resaddr += 1 * 22;
		}
	}

	trap_put_word(ctx, resaddr + 0, 0x7001); /* moveq #1,d0 */
	trap_put_word(ctx, resaddr + 2, RTS);
	resaddr += 4;

	ROM_filesys_doio_original = resaddr;
	ROM_filesys_putmsg_original = resaddr + 4;
	resaddr += 8;
	ROM_filesys_hack_remove = resaddr;

	if (putmsg_hack_state) {

		// remove patches
		put_long(resaddr + 0, 0x48e7fffe); // movem.l d0-d7/a0-a6,-(sp)
		put_word(resaddr + 4, 0x224e); // move.l a6,a1
		resaddr += 6;
		if (trackdisk_hack_state) {
			put_word(resaddr + 0, 0x307c); // move.w #x,a0
			put_word(resaddr + 2, -0x1c8);
			put_word(resaddr + 4, 0x2039);	// move.l x,d0
			put_long(resaddr + 6, ROM_filesys_doio_original);
			put_word(resaddr + 10, 0x4eae);  // jsr x(a6)
			put_word(resaddr + 12, -0x1a4);
			resaddr += 14;
		}
		put_word(resaddr + 0, 0x307c); // move.w #x,a0
		put_word(resaddr + 2, -0x16e);
		put_word(resaddr + 4, 0x2039);	// move.l x,d0
		put_long(resaddr + 6, ROM_filesys_putmsg_original);
		put_word(resaddr + 10, 0x4eae);  // jsr x(a6)
		put_word(resaddr + 12, -0x1a4);
		resaddr += 14;
		put_long(resaddr + 0, 0x4cdf7fff); // movem.l (sp)+,d0-d7/a0-a6
		resaddr += 4;

		uaecptr temp = here();
		org(filesys_initcode_ptr);
		dl(resaddr);
		org(temp);

		put_word(resaddr, 0x4e75); // rts
		resaddr += 2;

		uaecptr resaddr_tmp = resaddr;

		resaddr = resaddr_hack;

		if (trackdisk_hack_state) {
			// Pre-KS 1.3 trackdisk.device boot block injection hack. Patch DoIO()
			put_word(resaddr + 0, 0x224e); // move.l a6,a1
			put_word(resaddr + 2, 0x307c); // move.w #x,a0
			put_word(resaddr + 4, -0x1c8);
			put_word(resaddr + 6, 0x203c);	// move.l #x,d0
			put_long(resaddr + 8, ROM_filesys_doio);
			put_word(resaddr + 12, 0x4eae);  // jsr x(a6)
			put_word(resaddr + 14, -0x1a4);
			put_word(resaddr + 16, 0x23c0); // move.l d0,x
			put_long(resaddr + 18, ROM_filesys_doio_original);
			resaddr += 22;
		}

		// Pre-KS 1.3 automount hack. Patch PutMsg()
		put_word(resaddr + 0, 0x224e); // move.l a6,a1
		put_word(resaddr + 2, 0x307c); // move.w #x,a0
		put_word(resaddr + 4, -0x16e);
		put_word(resaddr + 6, 0x203c);	// move.l #x,d0
		put_long(resaddr + 8, ROM_filesys_putmsg);
		put_word(resaddr + 12, 0x4eae);  // jsr x(a6)
		put_word(resaddr + 14, -0x1a4);
		put_word(resaddr + 16, 0x23c0); // move.l d0,x
		put_long(resaddr + 18, ROM_filesys_putmsg_original);
		resaddr += 22;

		// filesys.asm make_dev D7
		put_word_host(baseaddr + 0x2112, 1 | 2 | 8 | 16);

		resaddr = resaddr_tmp;
	} else {
		// undo possible KS 1.2 hack
		uaecptr temp = here();
		org(filesys_initcode_ptr);
		dl(filesys_initcode_real);
		org(temp);
	}

	trap_set_areg(ctx, 0, last_resident);
	
	if (currprefs.uae_hide_autoconfig && expansion) {
		bool found = true;
		while (found) {
			uaecptr node = trap_get_long(ctx, expansion + 0x3c);
			found = false;
			while (trap_get_long(ctx, node)) {
				if (trap_get_word(ctx, node + 0x10 + 4) == 2011) {
					uae_u8 prod = trap_get_byte(ctx, node + 0x10 + 1);
					if (prod != 2) {
						// remove all 2011 boards except filesystem
						found = true;
						uaecptr succ = trap_get_long(ctx, node);
						uaecptr pred = trap_get_long(ctx, node + 4);
						trap_put_long(ctx, pred,  succ);
						trap_put_long(ctx, succ + 4, pred);
						break;
					}
					// replace filesystem with A590/A2091 IDs..
					trap_put_byte(ctx, node + 0x10 + 1, 3);
					trap_put_word(ctx, node + 0x10 + 4, 514);
				}
				node = trap_get_long(ctx, node);
			}
		}
	}

	kill_ide(ctx);

	tmp = first_resident;
	while (tmp < last_resident && tmp >= first_resident) {
		if (trap_get_word(ctx, tmp) == 0x4AFC && trap_get_long(ctx, tmp + 0x2) == tmp) {
			resaddr = add_resident(ctx, resaddr, tmp);
			tmp = trap_get_long(ctx, tmp + 0x6);
		} else {
			tmp += 2;
		}
	}

	return 1;
}

#define FS_TEST_HACK 0
#if FS_TEST_HACK
static uae_u32 REGPARAM2 ks13patchstartup(TrapContext *ctx)
{
	uaecptr startup = trap_get_areg(ctx, 0);
	uaecptr doslib = 0x00c06230;
	uaecptr root = get_long(doslib + 34);
	uaecptr node = (get_long(root + 24) << 2) + 4;
	uaecptr dp_arg2 = get_long(startup + 24);
	for (;;) {
		node = get_long(node) << 2;
		if (!node)
			break;
		if (get_long(node + 4) != 0)
			continue;
		if (get_long(node + 28) != dp_arg2)
			continue;
		put_long(startup + 20, get_long(node + 40)); // dp_Arg1 = dol_Node
		put_long(startup + 28, node >> 2); // dp_Arg3 = dl
		activate_debugger();
		return dp_arg2 << 2;
	}
	return 0;
}

static void patchhackfs(uaecptr seg)
{
	int o = rtarea_base + 0xFEFA;
	put_long(seg + 9806, 0x70ff70ff);
	seg += 9632;
	put_word(seg, 0x4eb9);
	put_long(seg + 2, o);
	put_word(seg + 6, 0x2440); // move.l d0,a2
	org(o);
	calltrap(deftrap2(ks13patchstartup, 0, _T("ks13patchstartup")));
	dw(RTS);
}
#endif

static uae_u32 REGPARAM2 filesys_dev_bootfilesys (TrapContext *ctx)
{
	uaecptr devicenode = trap_get_areg(ctx, 3);
	uaecptr parmpacket = trap_get_areg(ctx, 1);
	uaecptr fsres = trap_get_long(ctx, parmpacket + PP_FSRES);
	uaecptr fsnode;
	uae_u32 dostype, dostype2;
	int no = trap_get_dreg(ctx, 6) & 0x7fffffff;
	int unit_no = no & 65535;
	UnitInfo *uip = &mountinfo.ui[unit_no];
	int iscd = (trap_get_dreg(ctx, 6) & 0x80000000) != 0 || uip->unit_type == UNIT_CDFS;
	int type;
	
	if (iscd) {
		if (!trap_get_long(ctx, devicenode + 16))
			trap_put_long(ctx, devicenode + 16, cdfs_handlername);
		return 0;
	} else {
		type = is_hardfile (unit_no);
	}

	if (type == FILESYS_VIRTUAL) {
		if (!trap_get_long(ctx, devicenode + 16))
			trap_put_long(ctx, devicenode + 16, fshandlername);
		return 0;
	}

	dostype = trap_get_long(ctx, parmpacket + 80);
	if (trap_get_long(ctx, parmpacket + PP_FSPTR) && !trap_get_long(ctx, parmpacket + PP_ADDTOFSRES)) {
		uaecptr fsptr = trap_get_long(ctx, parmpacket + PP_FSPTR);
		uip->filesysseg = fsptr;
		// filesystem but was not added to fs.resource
		uae_u32 pf = trap_get_long(ctx, parmpacket + PP_FSHDSTART + 8); // fse_PatchFlags
		for (int i = 0; i < 32; i++) {
			if (pf & (1 << i))
				trap_put_long(ctx, devicenode + 4 + i * 4, trap_get_long(ctx, parmpacket + PP_FSHDSTART + 8 + 4 + i * 4));
		}
		uaecptr seglist = fsptr >> 2;
		if (bcplonlydos()) {
			trap_put_long(ctx, devicenode + 4 + 3 * 4, seglist);
			seglist = (trap_get_long(ctx, rtarea_base + bootrom_header + 4 + 6 * 4) + rtarea_base + bootrom_header) >> 2;
		}
		trap_put_long(ctx, devicenode + 4 + 7 * 4, seglist);
		write_log(_T("Filesys %08x entry=%08x\n"), dostype, fsptr);
		return 1;
	}
	fsnode = trap_get_long(ctx, fsres + 18);
	while (trap_get_long(ctx, fsnode)) {
		dostype2 = trap_get_long(ctx, fsnode + 14);
		if (dostype2 == dostype) {
			uae_u32 pf = trap_get_long(ctx, fsnode + 22); // fse_PatchFlags
			for (int i = 0; i < 32; i++) {
				if (pf & (1 << i)) {
					uae_u32 data = trap_get_long(ctx, fsnode + 22 + 4 + i * 4);
					if (i == 7) {
						if (bcplonlydos()) { // seglist
							// point seglist to bcpl wrapper and put original seglist in dn_Handler
							trap_put_long(ctx, devicenode + 4 + 3 * 4, trap_get_long(ctx, fsnode + 22 + 4 + 7 * 4));
							data = (trap_get_long(ctx, rtarea_base + bootrom_header + 4 + 6 * 4) + rtarea_base + bootrom_header) >> 2;
						}
						write_log(_T("Filesys %08x entry=%08x\n"), dostype, data << 2);
#if FS_TEST_HACK
						patchhackfs(data << 2);
#endif
					}
					trap_put_long(ctx, devicenode + 4 + i * 4, data);
				}
			}
			return 1;
		}
		fsnode = trap_get_long(ctx, fsnode);
	}
	if (type == FILESYS_HARDFILE) {
		uae_u32 pf = trap_get_long(ctx, parmpacket + PP_FSHDSTART + 8); // fse_PatchFlags
		for (int i = 0; i < 32; i++) {
			if (pf & (1 << i))
				trap_put_long(ctx, devicenode + 4 + i * 4, trap_get_long(ctx, parmpacket + PP_FSHDSTART + 8 + 4 + i * 4));
		}
		trap_put_long(ctx, devicenode + 4 + 7 * 4, 0); // seglist
	}

	uaecptr file_system_proc = trap_get_dreg(ctx, 1);
	if (bcplonlydos() && file_system_proc && trap_get_long(ctx, devicenode + 4 + 7 * 4) == 0) {
		// 1.1 or older, seglist == 0: get ROM OFS seglist from "File System" process.
		// 1.2 and newer automatically use ROM OFS if seglist is zero.
		// d1 = "File System" process pointer.
		uaecptr p = trap_get_long(ctx, file_system_proc + 0x80) << 2; // pr_SegList
		if (p) {
			uae_u32 cnt = trap_get_long(ctx, p);
			if (cnt > 0 && cnt < 16) {
				uaecptr handlerseg = trap_get_long(ctx, p + cnt * 4);
				write_log(_T("Pre-KS 1.2 handler segment %08x.\n"), handlerseg << 2);
				trap_put_long(ctx, devicenode + 4 + 7 * 4, handlerseg);
			}
		}
	}

	return 0;
}

// called from bcplwrapper
static uae_u32 REGPARAM2 filesys_bcpl_wrapper(TrapContext *ctx)
{
	const int patches[] = { 0x782, 0x7b8, 0x159c, 0x15b4, 0 };
	uaecptr devicenode = trap_get_long(ctx, trap_get_dreg(ctx, 1) + 0x1c) << 2;
	// fetch original seglist from dn_Handler
	uaecptr seglist = trap_get_long(ctx, devicenode + 4 + 3 * 4) << 2;
	uaecptr patchfunc = trap_get_areg(ctx, 1);
	seglist += 4;
	trap_set_areg(ctx, 0, seglist);
	for (int i = 0; patches[i]; i++) {
		int offset = patches[i];
		if (get_long(seglist + offset + 2) != 0x4eaefd90) {
			write_log(_T("FFS patch failed, comparison mismatch.\n"));
			return 0;
		}
	}
	for (int i = 0; patches[i]; i++) {
		int offset = patches[i];
		trap_put_word(ctx, seglist + offset, 0x4eb9);
		trap_put_long(ctx, seglist + offset + 2, patchfunc);
		patchfunc += 4;
	}
	uae_u16 ver = trap_get_word(ctx, trap_get_areg(ctx, 6) + 20);
	if (ver < 31) {
		// OpenLibrary -> OldOpenLibrary
		trap_put_word(ctx, seglist + 0x7f4, -0x198);
		trap_put_word(ctx, seglist + 0x2a6e, -0x198);
	}
	write_log(_T("FFS pre-1.2 patched\n"));
	return 0;
}

static uae_u32 REGPARAM2 filesys_init_storeinfo (TrapContext *ctx)
{
	int ret = -1;
	switch (trap_get_dreg(ctx, 1))
	{
	case 1:
		mountertask = trap_get_areg(ctx, 1);
#ifdef PICASSO96
		picasso96_alloc(ctx);
#endif
		break;
	case 2: // new unit number query
		ret = automountunit;
		automountunit = -2;
		break;
	case 3:
		return 0;
	case 4: // mount done (d0=automountunit)
		automountunit = -1;
		return 0;
	}
	return ret;
}

/* Remember a pointer AmigaOS gave us so we can later use it to identify
* which unit a given startup message belongs to.  */
static uae_u32 REGPARAM2 filesys_dev_remember (TrapContext *ctx)
{
	int no = trap_get_dreg(ctx, 6) & 0x7fffffff;
	int unit_no = no & 65535;
	int sub_no = no >> 16;
	UnitInfo *uip = &mountinfo.ui[unit_no];
	int iscd = (trap_get_dreg(ctx, 6) & 0x80000000) != 0 || uip->unit_type == UNIT_CDFS;
	uaecptr devicenode = trap_get_areg(ctx, 3);
	uaecptr parmpacket = trap_get_areg(ctx, 1);
	int fssize;
	uae_u8 *fs;

	uip->devicenode = devicenode;
	fssize = uip->rdb_filesyssize;
	fs = uip->rdb_filesysstore;

	/* copy filesystem loaded from RDB */
	if (trap_get_long(ctx, parmpacket + PP_FSPTR)) {
		uaecptr addr = trap_get_long(ctx, parmpacket + PP_FSPTR);
		trap_put_bytes(ctx, fs, addr, fssize);
		// filesystem debugging, negative FSSIZE = debug mode.
		if (segtrack_mode & 2) {
			trap_put_long(ctx, parmpacket + PP_FSSIZE, 0 - trap_get_long(ctx, parmpacket + PP_FSSIZE));
		}
	}

	xfree (fs);
	uip->rdb_filesysstore = 0;
	uip->rdb_filesyssize = 0;
	if (trap_get_dreg(ctx, 3) >= 0)
		uip->startup = trap_get_long(ctx, devicenode + 28);

	return devicenode;
}

static int legalrdbblock (UnitInfo *uip, int block)
{
	if (block <= 0)
		return 0;
	if (block >= uip->hf.virtsize / uip->hf.ci.blocksize)
		return 0;
	return 1;
}

static uae_u32 rl (uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}
static uae_u16 rw (uae_u8 *p)
{
	return (p[0] << 8) | (p[1]);
}

static int rdb_checksum (const uae_char *id, uae_u8 *p, int block)
{
	uae_u32 sum = 0;
	int i, blocksize;

	if (memcmp (id, p, 4))
		return 0;
	blocksize = rl (p + 4);
	if (blocksize < 1 || blocksize * 4 > FILESYS_MAX_BLOCKSIZE)
		return 0;
	for (i = 0; i < blocksize; i++)
		sum += rl (p + i * 4);
	sum = 0 - sum;
	if (sum) {
		TCHAR *s = au (id);
		write_log (_T("RDB: block %d ('%s') checksum error\n"), block, s);
		xfree (s);
		return 0;
	}
	return 1;
}

static int device_isdup (TrapContext *ctx, uaecptr expbase, TCHAR *devname)
{
	uaecptr bnode, dnode, name;
	int len, i;
	TCHAR dname[256];

	if (!expbase)
		return 0;
	bnode = trap_get_long(ctx, expbase + 74); /* expansion.library bootnode list */
	while (trap_get_long(ctx, bnode)) {
		dnode = trap_get_long(ctx, bnode + 16); /* device node */
		name = trap_get_long(ctx, dnode + 40) << 2; /* device name BSTR */
		len = trap_get_byte(ctx, name);
		for (i = 0; i < len; i++)
			dname[i] = trap_get_byte(ctx, name + 1 + i);
		dname[len] = 0;
		if (!_tcsicmp(devname, dname))
			return 1;
		bnode = trap_get_long(ctx, bnode);
	}
	return 0;
}

static TCHAR *device_dupfix (TrapContext *ctx, uaecptr expbase, TCHAR *devname)
{
	int modified;
	TCHAR newname[256];

	_tcscpy (newname, devname);
	modified = 1;
	while (modified) {
		modified = 0;
		if (device_isdup (ctx, expbase, newname)) {
			if (uaetcslen (newname) > 2 && newname[uaetcslen (newname) - 2] == '_') {
				newname[uaetcslen (newname) - 1]++;
			} else {
				_tcscat (newname, _T("_0"));
			}
			modified = 1;
		}
	}
	return my_strdup (newname);
}

static const TCHAR *dostypes(TCHAR *dt, uae_u32 dostype)
{
	int j;

	j = 0;
	for (int i = 0; i < 4; i++) {
		uae_u8 c = dostype >> ((3 - i) * 8);
		if (c >= ' ' && c <= 'z') {
			dt[j++] = c;
		} else {
			dt[j++] = '\\';
			_sntprintf (&dt[j], sizeof &dt[j], _T("%d"), c);
			j += uaetcslen (&dt[j]);
		}
	}
	dt[j] = 0;
	return dt;
}

static void dump_partinfo (struct hardfiledata *hfd, uae_u8 *pp)
{
	TCHAR *s;
	uae_u32 dostype;
	uae_u64 size;
	int blocksize, surfaces, spb, spt, reserved;
	int lowcyl, highcyl;
	uae_u32 block, flags;
	uae_u8 buf[512];
	TCHAR dt[32];

	flags = rl (pp + 20);
	pp[37 + pp[36]] = 0;
	s = au ((char*)pp + 37);
	pp += 128;
	dostype = rl (pp + 64);
	spb = rl (pp + 16);
	blocksize = rl (pp + 4) * 4;
	surfaces = rl (pp + 12);
	spt = rl (pp + 20);
	reserved = rl (pp + 24);
	lowcyl = rl (pp + 36);
	highcyl = rl (pp + 40);
	size = ((uae_u64)blocksize) * surfaces * spt * (highcyl - lowcyl + 1);

	write_log (_T("Partition '%s' Dostype=%08X (%s) Flags: %08X\n"), s[0] ? s : _T("_NULL_"), dostype, dostypes (dt, dostype), flags);
	write_log (_T("BlockSize: %d, Surfaces: %d, SectorsPerBlock %d\n"),
		blocksize, surfaces, spb);
	write_log (_T("SectorsPerTrack: %d, Reserved: %d, LowCyl %d, HighCyl %d, Size %dM\n"),
		spt, reserved, lowcyl, highcyl, (uae_u32)(size >> 20));
	write_log (_T("Buffers: %d, BufMemType: %08x, MaxTransfer: %08x, Mask: %08x, BootPri: %d\n"),
		rl (pp + 44), rl (pp + 48), rl (pp + 52), rl (pp + 56), rl (pp + 60));
	write_log (_T("Total blocks: %lld, Total disk blocks: %lld\n"), (uae_s64)surfaces * spt * (highcyl - lowcyl + 1), hfd->virtsize / blocksize);

	if (hfd->drive_empty) {
		write_log (_T("Empty drive\n"));
	} else {
		block = lowcyl * surfaces * spt;
		if (hdf_read (hfd, buf, (uae_u64)blocksize * block, sizeof buf)) {
			write_log (_T("First block %d dostype: %08X (%s)\n"), block, rl (buf), dostypes (dt, rl (buf)));
		} else {
			write_log (_T("First block %d read failed!\n"), block);
		}
		xfree (s);
		if ((uae_u64)highcyl * spt * surfaces * blocksize > hfd->virtsize) {
			write_log (_T("RDB: WARNING: end of partition > size of disk! (%llu > %llu)\n"),
				(uae_u64)highcyl * spt * surfaces * blocksize, hfd->virtsize);
		}
	}
}

static void dumprdbblock(const uae_u8 *buf, int block)
{
	int w = 16;
	write_log(_T("RDB block %d:\n"), block);
	for (int i = 0; i < 512; i += w) {
		TCHAR outbuf[81];
		for (int j = 0; j < w; j++) {
			uae_u8 v = buf[i + j];
			_sntprintf(outbuf + 2 * j, sizeof outbuf + 2 * j, _T("%02X"), v);
			outbuf[2 * w + 1 + j] = (v >= 32 && v <= 126) ? v : '.';
		}
		outbuf[2 * w] = ' ';
		outbuf[2 * w + 1 + w] = 0;
		write_log(_T("%s\n"), outbuf);
	}
}

static void dump_rdb (UnitInfo *uip, struct hardfiledata *hfd, uae_u8 *bufrdb, uae_u8 *buf, int readblocksize)
{
	TCHAR dt[32];

	write_log (_T("RDB: HostID: %08x Flags: %08x\n"),
		rl (bufrdb + 3 * 4), rl (bufrdb + 5 * 4));
	write_log (_T("RDB: BL: %d BH: %d LC: %d HC: %d CB: %d HB: %d\n"),
		rl (bufrdb + 128), rl (bufrdb + 132), rl (bufrdb + 136), rl (bufrdb + 140), rl (bufrdb + 144), rl (bufrdb + 152));
	for (int i = 0; i < 100; i++) {
		int partblock;
		if (i == 0)
			partblock = rl (bufrdb + 28);
		else
			partblock = rl (buf + 4 * 4);
		if (partblock == 0xffffffff)
			break;
		write_log (_T("RDB: PART block %d:\n"), partblock);
		if (!legalrdbblock (uip, partblock)) {
			write_log (_T("RDB: corrupt PART pointer %d\n"), partblock);
			dumprdbblock(i == 0 ? bufrdb : buf, partblock);
			break;
		}
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, partblock * hfd->ci.blocksize, readblocksize);
		if (!rdb_checksum ("PART", buf, partblock)) {
			write_log (_T("RDB: checksum error PART block %d\n"), partblock);
			dumprdbblock(buf, partblock);
			break;
		}
		dump_partinfo (hfd, buf);
	}
	for (int i = 0; i < 100; i++) {
		int fileblock;
		if (i == 0)
			fileblock = rl (bufrdb + 32);
		else
			fileblock = rl (buf + 4 * 4);
		if (fileblock == 0xffffffff)
			break;
		write_log (_T("RDB: LSEG block %d:\n"), fileblock);
		if (!legalrdbblock (uip, fileblock)) {
			write_log (_T("RDB: corrupt FSHD pointer %d\n"), fileblock);
			dumprdbblock(i == 0 ? bufrdb : buf, fileblock);
			break;
		}
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, fileblock * hfd->ci.blocksize, readblocksize);
		if (!rdb_checksum ("FSHD", buf, fileblock)) {
			write_log (_T("RDB: checksum error FSHD block %d\n"), fileblock);
			dumprdbblock(buf, fileblock);
			break;
		}
		uae_u32 dostype = rl (buf + 32);
		int version = (buf[36] << 8) | buf[37];
		int revision = (buf[38] << 8) | buf[39];
		write_log (_T("LSEG: %08x (%s) %d.%d\n"), dostype, dostypes (dt, dostype), version, revision);
	}
}

static void get_new_device (TrapContext *ctx, int type, uaecptr parmpacket, TCHAR **devname, uaecptr *devname_amiga, int unit_no)
{
	TCHAR buffer[80];
	uaecptr expbase = trap_get_long(ctx, parmpacket + PP_EXPLIB);

	if (*devname == 0 || uaetcslen (*devname) == 0) {
		int un = unit_no;
		for (;;) {
			_sntprintf (buffer, sizeof buffer, type == FILESYS_CD ? _T("CD%d") : _T("DH%d"), un++);
			if (type == FILESYS_CD)
				*devname = my_strdup (buffer);
			if (!device_isdup(ctx, expbase, buffer))
				break;
		}
	} else {
		_tcscpy (buffer, *devname);
	}
	*devname_amiga = ds (device_dupfix(ctx, expbase, buffer));
	if (type == FILESYS_CD)
		write_log (_T("FS: mounted CD unit %s\n"), buffer);
	else if (type == FILESYS_VIRTUAL)
		write_log (_T("FS: mounted virtual unit %s (%s)\n"), buffer, mountinfo.ui[unit_no].rootdir);
	else
		write_log (_T("FS: mounted HDF unit %s (%04x-%08x, %s)\n"), buffer,
		(uae_u32)(mountinfo.ui[unit_no].hf.virtsize >> 32),
		(uae_u32)(mountinfo.ui[unit_no].hf.virtsize),
		mountinfo.ui[unit_no].rootdir);
}

#define rdbmnt write_log (_T("Mounting uaehf.device %d (%d) (size=%llu):\n"), unit_no, partnum, hfd->virtsize)

static int pt_babe(TrapContext *ctx, uae_u8 *bufrdb, UnitInfo *uip, int unit_no, int partnum, uaecptr parmpacket)
{
	uae_u8 bufrdb2[FILESYS_MAX_BLOCKSIZE] = { 0 };
	struct hardfiledata *hfd = &uip->hf;
	struct uaedev_config_info *ci = &uip[unit_no].hf.ci;
	uae_u32 bad;
	
	uae_u32 bs = (bufrdb[0x20] << 24) | (bufrdb[0x21] << 16) | (bufrdb[0x22] << 8) | (bufrdb[0x23] << 0);
	int bscnt;
	for (bscnt = 512; bscnt <= 32768; bscnt <<= 1) {
		if (bs == (bscnt >> 2))
			break;
	}
	if (bscnt > 32768)
		return 0;

	bad = rl(bufrdb + 4);
	if (bad) {
		if (bad * hfd->ci.blocksize > FILESYS_MAX_BLOCKSIZE)
			return 0;
		hdf_read_rdb(hfd, bufrdb2, bad * hfd->ci.blocksize, hfd->ci.blocksize);
		if (bufrdb2[0] != 0xBA || bufrdb2[1] != 0xD1)
			return 0;
	}

	if (partnum > 0)
		return -2;

	if (uip->bootpri <= -129) {
		return -1;
	}

	if (bad) {
		uae_u16 badnum = (bufrdb2[2] << 8) | bufrdb2[3];
		if (badnum) {
			write_log(_T("A2090 partition table: bad block table is not empty, bad blocks not yet supported!\n"));
		}
	}

	hfd->rdbcylinders = rl(bufrdb + 0x08);
	hfd->rdbsectors = rw(bufrdb + 0x18);
	hfd->rdbheads = rw(bufrdb + 0x1a);

	write_log (_T("Mounting uaehf.device:%d %d (%d):\n"), uip->hf.ci.controller_unit, unit_no, partnum);
	get_new_device(ctx, FILESYS_HARDFILE_RDB, parmpacket, &uip->devname, &uip->rdb_devname_amiga[partnum], unit_no);
	trap_put_long(ctx, parmpacket, uip->rdb_devname_amiga[partnum]);
	trap_put_long(ctx, parmpacket + 4, ROM_hardfile_resname);
	trap_put_long(ctx, parmpacket + 8, uip->devno);
	trap_put_long(ctx, parmpacket + 12, 0); /* Device flags */
	for (int i = 0; i < 12 * 4; i++)
		trap_put_byte(ctx, parmpacket + 16 + i, bufrdb[0x1c + i]);

	uae_u32 dostype = 0x444f5300;
	if (ci->dostype) // forced dostype?
		dostype = ci->dostype;

	// A2090 Environment Table Size is only 11
	int ts = trap_get_long(ctx, parmpacket + 16 + 0 * 4);
	while (ts < 16) {
		ts++;
		if (ts == 12) // BufMem Type
			trap_put_long(ctx, parmpacket + 16 + 12 * 4, 1);
		if (ts == 13) // Max Transfer
			trap_put_long(ctx, parmpacket + 16 + 13 * 4, 0x7fffffff);
		if (ts == 14) // Mask
			trap_put_long(ctx, parmpacket + 16 + 14 * 4, 0xfffffffe);
		if (ts == 15) // BootPri
			trap_put_long(ctx, parmpacket + 16 + 15 * 4, uip->bootpri);
		if (ts == 16) // DosType
			trap_put_long(ctx, parmpacket + 16 + 16 * 4, dostype);
	}
	trap_put_long(ctx, parmpacket + 16 + 0 * 4, ts);

	uip->rdb_dostype = dostype;

	if (uip->bootpri <= -128) /* not bootable */
		trap_set_dreg(ctx, 7, trap_get_dreg(ctx, 7) & ~1);

	return 2;
}

static int pt_rdsk (TrapContext *ctx, uae_u8 *bufrdb, int rdblock, UnitInfo *uip, int unit_no, int partnum, uaecptr parmpacket)
{
	int blocksize, readblocksize, badblock, driveinitblock;
	TCHAR dt[32];
	uae_u8 *fsmem = 0, *buf = 0;
	int partblock, fileblock, lsegblock;
	uae_u32 flags;
	struct hardfiledata *hfd = &uip->hf;
	uae_u32 dostype;
	uaecptr fsres, fsnode;
	int err = 0;
	int oldversion, oldrevision;
	int newversion, newrevision;
	TCHAR *s;
	bool showdebug = partnum == 0;
	int cnt;

	blocksize = rl (bufrdb + 16);
	readblocksize = blocksize > hfd->ci.blocksize ? blocksize : hfd->ci.blocksize;

	badblock = rl (bufrdb + 24);
	if (badblock != -1) {
		write_log (_T("RDB: badblock list %08x\n"), badblock);
		dumprdbblock(bufrdb, rdblock);
	}

	driveinitblock = rl (bufrdb + 36);
	if (driveinitblock != -1) {
		write_log (_T("RDB: driveinit = %08x\n"), driveinitblock);
		dumprdbblock(bufrdb, rdblock);
	}

	hfd->rdbcylinders = rl (bufrdb + 64);
	hfd->rdbsectors = rl (bufrdb + 68);
	hfd->rdbheads = rl (bufrdb + 72);
#if 0
	{
		int cyls, secs, heads;
		getchsgeometry_hdf (hfd, hfd->virtsize, &cyls, &secs, &heads);
		if (cyls * secs * heads > hfd->cylinders * hfd->sectors * hfd->heads) {
			hfd->cylinders = cyls;
			hfd->sectors = secs;
			hfd->heads = heads;
		}
	}
#endif
	fileblock = rl (bufrdb + 32);

	buf = xmalloc (uae_u8, readblocksize);

	if (showdebug) {
		if ((uae_u64)hfd->rdbcylinders * hfd->rdbsectors * hfd->rdbheads * blocksize > hfd->virtsize)
			write_log (_T("RDB: WARNING: RDSK header disk size > disk size! (%llu > %llu)\n"),
				(uae_u64)hfd->rdbcylinders * hfd->rdbsectors * hfd->rdbheads * blocksize, hfd->virtsize);
		write_log (_T("RDSK dump start\n"));
		write_log (_T("RDSK at %d, C=%d S=%d H=%d\n"),
			rdblock, hfd->rdbcylinders, hfd->rdbsectors, hfd->rdbheads);
		dump_rdb (uip, hfd, bufrdb, buf, readblocksize);
		write_log (_T("RDSK dump end\n"));
	}

	for (int i = 0; i <= partnum; i++) {
		if (i == 0)
			partblock = rl (bufrdb + 28);
		else
			partblock = rl (buf + 4 * 4);
		if (!legalrdbblock (uip, partblock)) {
			err = -2;
			goto error;
		}
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, partblock * hfd->ci.blocksize, readblocksize);
		if (!rdb_checksum ("PART", buf, partblock)) {
			err = -2;
			write_log(_T("RDB: checksum error in PART block %d\n"), partblock);
			dumprdbblock(buf, partblock);
			goto error;
		}
	}

	rdbmnt;
	flags = rl (buf + 20);
	if ((flags & 2) || uip->bootpri <= -129) { /* do not mount */
		err = -1;
		write_log (_T("RDB: Automount disabled, not mounting\n"));
		goto error;
	}

	if (!(flags & 1) || uip->bootpri <= -128) /* not bootable */
		trap_set_dreg(ctx, 7, trap_get_dreg(ctx, 7) & ~1);

	buf[37 + buf[36]] = 0; /* zero terminate BSTR */
	s = au ((char*)buf + 37);
	uip->rdb_devname_amiga[partnum] = ds(device_dupfix(ctx, trap_get_long(ctx, parmpacket + PP_EXPLIB), s));
	xfree (s);
	trap_put_long(ctx, parmpacket, uip->rdb_devname_amiga[partnum]); /* name */
	trap_put_long(ctx, parmpacket + 4, ROM_hardfile_resname);
	trap_put_long(ctx, parmpacket + 8, uip->devno);
	trap_put_long(ctx, parmpacket + 12, 0); /* Device flags */
	for (int i = 0; i < (17 + 15) * 4; i++)
		trap_put_byte(ctx, parmpacket + 16 + i, buf[128 + i]);
	dostype = trap_get_long(ctx, parmpacket + 80);
	uip->rdb_dostype = dostype;

	if (dostype == 0) {
		write_log (_T("RDB: mount failed, dostype=0\n"));
		err = -1;
		goto error;
	}
	if (dostype == 0xffffffff) {
		write_log (_T("RDB: WARNING: dostype = 0xFFFFFFFF. FFS bug can report partition in \"no disk inserted\" state!!\n"));
	}

	err = 2;

	/* load custom filesystems if needed */
	if (fileblock == -1 || !legalrdbblock (uip, fileblock))
		goto error;

	fsres = trap_get_long(ctx, parmpacket + PP_FSRES);
	if (!fsres) {
		write_log (_T("RDB: FileSystem.resource not found, this shouldn't happen!\n"));
		goto error;
	}
	fsnode = trap_get_long(ctx, fsres + 18);
	while (trap_get_long(ctx, fsnode)) {
		if (trap_get_long(ctx, fsnode + 14) == dostype)
			break;
		fsnode = trap_get_long(ctx, fsnode);
	}
	oldversion = oldrevision = -1;
	if (trap_get_long(ctx, fsnode)) {
		oldversion = trap_get_word(ctx, fsnode + 18);
		oldrevision = trap_get_word(ctx, fsnode + 20);
	} else {
		fsnode = 0;
	}

	for (;;) {
		if (fileblock == -1) {
			if (!fsnode)
				write_log (_T("RDB: FS %08X (%s) not in FileSystem.resource or in RDB\n"), dostype, dostypes (dt, dostype));
			goto error;
		}
		if (!legalrdbblock (uip, fileblock)) {
			write_log (_T("RDB: corrupt FSHD pointer %d\n"), fileblock);
			goto error;
		}
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, fileblock * hfd->ci.blocksize, readblocksize);
		if (!rdb_checksum ("FSHD", buf, fileblock)) {
			write_log (_T("RDB: checksum error in FSHD block %d\n"), fileblock);
			dumprdbblock(buf, fileblock);
			goto error;
		}
		fileblock = rl (buf + 16);
		uae_u32 rdbdostype = rl (buf + 32);
		if (((dostype >> 8) == (rdbdostype >> 8) && (dostype != DISK_TYPE_DOS && (dostype & 0xffffff00) == DISK_TYPE_DOS)) || (dostype == rdbdostype))
			break;
	}
	newversion = (buf[36] << 8) | buf[37];
	newrevision = (buf[38] << 8) | buf[39];

	write_log (_T("RDB: RDB filesystem %08X (%s) version %d.%d\n"), dostype, dostypes (dt, dostype), newversion, newrevision);
	if (fsnode) {
		write_log (_T("RDB: %08X (%s) in FileSystem.resource version %d.%d\n"), dostype, dostypes (dt, dostype), oldversion, oldrevision);
	}
	if (newversion * 65536 + newrevision <= oldversion * 65536 + oldrevision && oldversion >= 0) {
		write_log (_T("RDB: FS in FileSystem.resource is newer or same, ignoring RDB filesystem\n"));
		goto error;
	}

	for (int i = 0; i < 140; i++)
		trap_put_byte(ctx, parmpacket + PP_FSHDSTART + i, buf[32 + i]);
	trap_put_long(ctx, parmpacket + PP_FSHDSTART, dostype);
	/* we found required FSHD block */
	fsmem = xmalloc (uae_u8, 262144);
	lsegblock = rl (buf + 72);
	cnt = 0;
	for (;;) {
		int pb = lsegblock;
		if (!legalrdbblock (uip, lsegblock))
			goto error;
		memset (buf, 0, readblocksize);
		hdf_read (hfd, buf, lsegblock * hfd->ci.blocksize, readblocksize);
		if (!rdb_checksum("LSEG", buf, lsegblock)) {
			write_log(_T("RDB: checksum error in LSEG block %d\n"), lsegblock);
			dumprdbblock(buf, lsegblock);
			goto error;
		}
		lsegblock = rl (buf + 16);
		if (lsegblock == pb)
			goto error;
		if ((cnt + 1) * (blocksize - 20) >= 262144)
			goto error;
		memcpy (fsmem + cnt * (blocksize - 20), buf + 20, blocksize - 20);
		cnt++;
		if (lsegblock == -1)
			break;
	}
	write_log (_T("RDB: Filesystem loaded, %d bytes\n"), cnt * (blocksize - 20));
	trap_put_long(ctx, parmpacket + PP_FSSIZE, cnt * (blocksize - 20)); /* RDB filesystem size hack */
	trap_put_long(ctx, parmpacket + PP_ADDTOFSRES, -1);
	uip->rdb_filesysstore = fsmem;
	uip->rdb_filesyssize = cnt * (blocksize - 20);
	xfree (buf);
	return 2;
error:
	xfree (buf);
	xfree (fsmem);
	return err;
}

static int rdb_mount (TrapContext *ctx, UnitInfo *uip, int unit_no, int partnum, uaecptr parmpacket)
{
	struct hardfiledata *hfd = &uip->hf;
	int lastblock = 63, rdblock;
	uae_u8 bufrdb[FILESYS_MAX_BLOCKSIZE];

	write_log (_T("%s:\n"), uip->rootdir);
	if (hfd->drive_empty) {
		rdbmnt;
		write_log (_T("ignored, drive is empty\n"));
		return -2;
	}
	if (hfd->ci.blocksize == 0) {
		rdbmnt;
		write_log (_T("failed, blocksize == 0\n"));
		return -1;
	}
	if (lastblock * hfd->ci.blocksize > hfd->virtsize) {
		rdbmnt;
		write_log (_T("failed, too small (%d*%d > %llu)\n"), lastblock, hfd->ci.blocksize, hfd->virtsize);
		return -2;
	}

	if (hfd->ci.blocksize > FILESYS_MAX_BLOCKSIZE) {
		write_log(_T("failed, too large block size %d\n"), hfd->ci.blocksize);
		return -2;
	}

	for (rdblock = 0; rdblock < lastblock; rdblock++) {
		hdf_read_rdb (hfd, bufrdb, rdblock * hfd->ci.blocksize, hfd->ci.blocksize);
		if (rdblock == 0 && bufrdb[0] == 0xBA && bufrdb[1] == 0xBE) {
				// A2090 "BABE" partition table?
				int v = pt_babe(ctx, bufrdb, uip, unit_no, partnum, parmpacket);
				if (v)
					return v;
				continue;
		}
		if (rdb_checksum ("RDSK", bufrdb, rdblock) || rdb_checksum ("CDSK", bufrdb, rdblock)) {
			return pt_rdsk(ctx, bufrdb, rdblock, uip, unit_no, partnum, parmpacket);
		}
		if (!memcmp ("RDSK", bufrdb, 4)) {
			bufrdb[0xdc] = 0;
			bufrdb[0xdd] = 0;
			bufrdb[0xde] = 0;
			bufrdb[0xdf] = 0;
			if (rdb_checksum ("RDSK", bufrdb, rdblock)) {
				write_log (_T("Windows 95/98/ME trashed RDB detected, fixing..\n"));
				hdf_write (hfd, bufrdb, rdblock * hfd->ci.blocksize, hfd->ci.blocksize);
				return pt_rdsk(ctx, bufrdb, rdblock, uip, unit_no, partnum, parmpacket);
			}
		}
	}

	rdbmnt;
	write_log (_T("failed, no supported partition tables detected\n"));
	return -2;
}


static void addfakefilesys (TrapContext *ctx, uaecptr parmpacket, uae_u32 dostype, int ver, int rev, struct uaedev_config_info *ci)
{
	uae_u32 flags;

	flags = 0x180;
	for (int i = 0; i < 140; i++)
		trap_put_byte(ctx, parmpacket + PP_FSHDSTART + i, 0);
	if (dostype) {
		trap_put_long(ctx, parmpacket + 80, dostype);
		trap_put_long(ctx, parmpacket + PP_FSHDSTART, dostype);
	}
	if (ver >= 0 && rev >= 0)
		trap_put_long(ctx, parmpacket + PP_FSHDSTART + 4, (ver << 16) | rev);

	trap_put_long(ctx, parmpacket + PP_FSHDSTART + 12 + 4 * 4, ci->stacksize);
	flags |= 0x10;

	if (ci->bootpri != -129) {
		trap_put_long(ctx, parmpacket + PP_FSHDSTART + 12 + 5 * 4, ci->bootpri);
		flags |= 0x20;
	}
	trap_put_long(ctx, parmpacket + PP_FSHDSTART + 12 + 8 * 4, dostype == DISK_TYPE_DOS || bcplonlydos() ? 0 : -1); // globvec
	// if OFS = seglist -> NULL
	if (dostype == DISK_TYPE_DOS)
		flags &= ~0x080;
	trap_put_long(ctx, parmpacket + PP_FSHDSTART + 8, flags); // patchflags
}

static uaecptr getfakefilesysseg (UnitInfo *uip)
{
	if (uip->filesysdir && uaetcslen (uip->filesysdir) > 0) {
		for (int i = 0; &mountinfo.ui[i] != uip; i++) {
			UnitInfo *uip2 = &mountinfo.ui[i];
			if (!uip2->filesysdir)
				continue;
			if (_tcsicmp (uip2->filesysdir, uip->filesysdir) != 0)
				continue;
			if (uip2->filesysseg)
				return uip2->filesysseg;
		}
	}
	return 0;
}

static int dofakefilesys (TrapContext *ctx, UnitInfo *uip, uaecptr parmpacket, struct uaedev_config_info *ci)
{
	int size;
	TCHAR tmp[MAX_DPATH];
	TCHAR dt[32];
	uae_u8 buf[512];
	struct zfile *zf;
	int ver = -1, rev = -1;
	uae_u32 dostype;
	bool autofs = false;

	// we already have custom filesystem loaded for earlier hardfile?
	if (!ci->forceload) {
		uaecptr seg = getfakefilesysseg (uip);
		if (seg) {
			// yes, re-use it.
			trap_put_long(ctx, parmpacket + PP_FSSIZE, 0);
			trap_put_long(ctx, parmpacket + PP_FSPTR, seg);
			trap_put_long(ctx, parmpacket + PP_ADDTOFSRES, 0);
			write_log (_T("RDB: faked RDB filesystem '%s' reused, entry=%08x\n"), uip->filesysdir, seg << 2);
			return FILESYS_HARDFILE;
		}
	}

	if (!ci->dostype) {
		memset (buf, 0, 4);
		hdf_read (&uip->hf, buf, 0, 512);
		dostype = (buf[0] << 24) | (buf[1] << 16) |(buf[2] << 8) | buf[3];
	} else {
		dostype = ci->dostype;
	}
	if (dostype == 0) {
		addfakefilesys(ctx, parmpacket, dostype, ver, rev, ci);
		return FILESYS_HARDFILE;
	}
	if (dostype == DISK_TYPE_DOS && (!uip->filesysdir || !uip->filesysdir[0])) {
		write_log (_T("RDB: OFS, using ROM default FS.\n"));
		return FILESYS_HARDFILE;
	}

	tmp[0] = 0;
	if (uip->filesysdir && uaetcslen (uip->filesysdir) > 0) {
		cfgfile_resolve_path_out_load(uip->filesysdir, tmp, MAX_DPATH, PATH_HDF);
	} else if ((dostype & 0xffffff00) == DISK_TYPE_DOS) {
		_tcscpy (tmp, currprefs.romfile);
		int i = uaetcslen (tmp);
		while (i > 0 && tmp[i - 1] != '/' && tmp[i - 1] != '\\')
			i--;
		_tcscpy (tmp + i, _T("FastFileSystem"));
		autofs = true;
	}
	if (tmp[0] == 0) {
		write_log (_T("RDB: no filesystem for dostype 0x%08X (%s)\n"), dostype, dostypes (dt, dostype));
		addfakefilesys(ctx, parmpacket, dostype, ver, rev, ci);
		if ((dostype & 0xffffff00) == DISK_TYPE_DOS)
			return FILESYS_HARDFILE;
		write_log (_T("RDB: mounted without filesys\n"));
		return FILESYS_HARDFILE;
	}
	write_log (_T("RDB: fakefilesys, trying to load '%s', dostype 0x%08X (%s)\n"), tmp, dostype, dostypes (dt, dostype));
	zf = zfile_fopen (tmp, _T("rb"), ZFD_NORMAL);
	if (!zf) {
		addfakefilesys(ctx, parmpacket, dostype, ver, rev, ci);
		write_log (_T("RDB: filesys not found, mounted without forced filesys\n"));
		return FILESYS_HARDFILE;
	}

	uae_u32 fsres, fsnode;
	int oldversion = -1;
	int oldrevision = -1;
	fsres = trap_get_long(ctx, parmpacket + PP_FSRES);
	fsnode = trap_get_long(ctx, fsres + 18);
	while (trap_get_long(ctx, fsnode)) {
		uae_u32 fsdostype = trap_get_long(ctx, fsnode + 14);
		if (fsdostype == dostype) {
			oldversion = trap_get_word(ctx, fsnode + 18);
			oldrevision = trap_get_word(ctx, fsnode + 20);
			write_log (_T("RDB: %08X (%s) in FileSystem.resource version %d.%d\n"), dostype, dostypes(dt, dostype), oldversion, oldrevision);
			break;
		}
		fsnode = trap_get_long(ctx, fsnode);
	}
	// if automatically found FastFileSystem, do not replace matching FileSystem.resource FS
	if (autofs && oldversion >= 0) {
		zfile_fclose (zf);
		addfakefilesys(ctx, parmpacket, dostype, ver, rev, ci);
		write_log (_T("RDB: not replacing FileSystem.resource\n"));
		return FILESYS_HARDFILE;
	}

	zfile_fseek (zf, 0, SEEK_END);
	size = zfile_ftell32(zf);
	if (size > 0) {
		zfile_fseek (zf, 0, SEEK_SET);
		uip->rdb_filesysstore = xmalloc (uae_u8, size);
		zfile_fread (uip->rdb_filesysstore, size, 1, zf);
		for (int i = 0; i < size - 6; i++) {
			uae_u8 *p = uip->rdb_filesysstore + i;
			if (p[0] == 'V' && p[1] == 'E' && p[2] == 'R' && p[3] == ':' && p[4] == ' ') {
				uae_u8 *p2;
				p += 5;
				p2 = p;
				while (*p2 && p2 - uip->rdb_filesysstore < size)
					p2++;
				if (p2[0] == 0) {
					while (*p && (ver < 0 || rev < 0)) {
						if (*p == ' ') {
							p++;
							ver = atol ((char*)p);
							if (ver < 0)
								ver = 0;
							while (*p) {
								if (*p == ' ')
									break;
								if (*p == '.') {
									p++;
									rev = atol ((char*)p);
									if (rev < 0)
										rev = 0;
								} else {
									p++;
								}
							}
							break;
						} else {
							p++;
						}
					}
				}
				break;
			}
		}
	}
	zfile_fclose (zf);
	uip->rdb_filesyssize = size;

	// DOS\0 is not in fs.resource and fs.resource already existed?
	if (dostype == DISK_TYPE_DOS && oldversion < 0)
		oldversion = 0;
	trap_put_long(ctx, parmpacket + PP_FSSIZE, uip->rdb_filesyssize);
	trap_put_long(ctx, parmpacket + PP_ADDTOFSRES, oldversion < 0 ? -1 : 0);
	addfakefilesys(ctx, parmpacket, dostype, ver, rev, ci);
	write_log (_T("RDB: faked RDB filesystem %08X (%s %d.%d) loaded. ADD2FS=%d\n"), dostype, dostypes (dt, dostype), ver, rev, oldversion < 0 ? 1 : 0);
	return FILESYS_HARDFILE;
}
	
/* Fill in per-unit fields of a parampacket */
static uae_u32 REGPARAM2 filesys_dev_storeinfo (TrapContext *ctx)
{
	int no = trap_get_dreg(ctx, 6) & 0x7fffffff;
	int unit_no = no & 65535;
	int sub_no = no >> 16;
	int type;

	if (unit_no >= MAX_FILESYSTEM_UNITS)
		return -2;
	if (sub_no >= MAX_FILESYSTEM_UNITS)
		return -2;

	UnitInfo *uip = mountinfo.ui;
	uaecptr parmpacket = trap_get_areg(ctx, 0);
	int iscd = (trap_get_dreg(ctx, 6) & 0x80000000) != 0 || uip[unit_no].unit_type == UNIT_CDFS;
	struct uaedev_config_info *ci = &uip[unit_no].hf.ci;

	uip[unit_no].parmpacket = parmpacket;
	if (!ks12hack_deviceproc)
		ks12hack_deviceproc = trap_get_long(ctx, parmpacket + PP_DEVICEPROC);
	trap_put_long(ctx, parmpacket + PP_DEVICEPROC, 0);
	trap_put_long(ctx, parmpacket + PP_ADDTOFSRES, 0);
	trap_put_long(ctx, parmpacket + PP_FSSIZE, 0);
	if (iscd) {
		TCHAR *cdname = NULL;
		uaecptr cdname_amiga;
		int cd_unit_no = unit_no - cd_unit_offset;

		if (sub_no)
			return -2;

		type = FILESYS_CD;
		get_new_device(ctx, type, parmpacket, &uip[unit_no].devname, &uip[unit_no].devname_amiga, cd_unit_no);
		cdname_amiga = uip[unit_no].devname_amiga;
		uip[unit_no].devno = unit_no;
		type = FILESYS_VIRTUAL;
		gui_flicker_led (LED_CD, cd_unit_no, 0);

		write_log (_T("Mounting uaescsi.device %d: (%d)\n"), cd_unit_no, unit_no);
		trap_put_long(ctx, parmpacket + 0, cdname_amiga);
		trap_put_long(ctx, parmpacket + 4, cdfs_devname);
		trap_put_long(ctx, parmpacket + 8, cd_unit_no);
		trap_put_long(ctx, parmpacket + 12, 0); /* Device flags */
		trap_put_long(ctx, parmpacket + 16, 19); /* Env. size */
		trap_put_long(ctx, parmpacket + 20, 2048 >> 2); /* longwords per block */
		trap_put_long(ctx, parmpacket + 24, 0); /* unused */
		trap_put_long(ctx, parmpacket + 28, 1); /* heads */
		trap_put_long(ctx, parmpacket + 32, 1); /* sectors per block */
		trap_put_long(ctx, parmpacket + 36, 1); /* sectors per track */
		trap_put_long(ctx, parmpacket + 40, 0); /* reserved blocks */
		trap_put_long(ctx, parmpacket + 44, 0); /* unused */
		trap_put_long(ctx, parmpacket + 48, 0); /* interleave */
		trap_put_long(ctx, parmpacket + 52, 0); /* lowCyl */
		trap_put_long(ctx, parmpacket + 56, 0); /* hiCyl */
		trap_put_long(ctx, parmpacket + 60, 50); /* Number of buffers */
		trap_put_long(ctx, parmpacket + 64, 1); /* Buffer mem type */
		trap_put_long(ctx, parmpacket + 68, 0x7FFFFFFE); /* largest transfer */
		trap_put_long(ctx, parmpacket + 72, 0xFFFFFFFE); /* dma mask */
		trap_put_long(ctx, parmpacket + 76, scsi_get_cd_drive_media_mask () & (1 << cd_unit_no) ? -127 : -128); /* bootPri */
		trap_put_long(ctx, parmpacket + 80, CDFS_DOSTYPE | (((cd_unit_no / 10) + '0') << 8) | ((cd_unit_no % 10) + '0'));
		trap_put_long(ctx, parmpacket + 84, 0); /* baud */
		trap_put_long(ctx, parmpacket + 88, 0); /* control */
		trap_put_long(ctx, parmpacket + 92, 0); /* bootblocks */
		return type;

	} else {
		gui_flicker_led (LED_HD, unit_no, 0);
		type = is_hardfile (unit_no);
		if (type == FILESYS_HARDFILE_RDB || type == FILESYS_HARDDRIVE) {
			/* RDB hardfile */
			uip[unit_no].devno = uip[unit_no].hf.ci.controller_unit;
			return rdb_mount(ctx, &uip[unit_no], unit_no, sub_no, parmpacket);
		}
		if (sub_no)
			return -2;
		write_log (_T("Mounting uaehf.device:%d %d (%d):\n"), uip->hf.ci.controller_unit, unit_no, sub_no);
		get_new_device(ctx, type, parmpacket, &uip[unit_no].devname, &uip[unit_no].devname_amiga, unit_no);
		uip[unit_no].devno = uip[unit_no].hf.ci.controller_unit;
		trap_put_long(ctx, parmpacket, uip[unit_no].devname_amiga);
		trap_put_long(ctx, parmpacket + 8, uip[unit_no].devno);
		trap_put_long(ctx, parmpacket + 12, 0); /* Device flags */
		trap_put_long(ctx, parmpacket + 16, 16); /* Env. size */
		trap_put_long(ctx, parmpacket + 24, 0); /* unused */
		trap_put_long(ctx, parmpacket + 44, 0); /* unused */
		trap_put_long(ctx, parmpacket + 48, 0); /* interleave */
		trap_put_long(ctx, parmpacket + 60, 50); /* Number of buffers */
		trap_put_long(ctx, parmpacket + 64, 1); /* Buffer mem type */
		trap_put_long(ctx, parmpacket + 68, 0x7FFFFFFE); /* largest transfer */
		trap_put_long(ctx, parmpacket + 72, 0xFFFFFFFE); /* dma mask */
		trap_put_long(ctx, parmpacket + 76, uip[unit_no].bootpri); /* bootPri */
		if (type == FILESYS_VIRTUAL) {
			// generate some sane-looking geometry if some program really cares..
			uae_s64 hicyl = 100;
			uae_u32 heads = 16;
			if (currprefs.filesys_limit) {
				hicyl = ((currprefs.filesys_limit * 1024) / 512) / (heads * 127);
			} else {
				struct fs_usage fsu;
				if (!get_fs_usage(uip[unit_no].rootdir, 0, &fsu)) {
					for (;;) {
						hicyl = (fsu.total / 512) / (heads * 127);
						if (hicyl < 65536 || heads == 64)
							break;
						heads *= 2;
					}
				}
			}
			trap_put_long(ctx, parmpacket + 4, fsdevname);
			trap_put_long(ctx, parmpacket + 20, 512 >> 2); /* longwords per block */
			trap_put_long(ctx, parmpacket + 28, heads); /* heads */
			trap_put_long(ctx, parmpacket + 32, 1); /* sectors per block */
			trap_put_long(ctx, parmpacket + 36, 127); /* sectors per track */
			trap_put_long(ctx, parmpacket + 40, 2); /* reserved blocks */
			trap_put_long(ctx, parmpacket + 52, 1); /* lowCyl */
			trap_put_long(ctx, parmpacket + 56, (uae_u32)hicyl); /* hiCyl */
			trap_put_long(ctx, parmpacket + 80, DISK_TYPE_DOS); /* DOS\0 */
		} else {
			uae_u8 buf[512];
			trap_put_long(ctx, parmpacket + 4, ROM_hardfile_resname);
			trap_put_long(ctx, parmpacket + 20, ci->blocksize >> 2); /* longwords per block */
			trap_put_long(ctx, parmpacket + 28, ci->surfaces); /* heads */
			trap_put_long(ctx, parmpacket + 32, ci->sectorsperblock); /* sectors per block */
			trap_put_long(ctx, parmpacket + 36, ci->sectors); /* sectors per track */
			trap_put_long(ctx, parmpacket + 40, ci->reserved); /* reserved blocks */
			trap_put_long(ctx, parmpacket + 52, ci->lowcyl); /* lowCyl */
			trap_put_long(ctx, parmpacket + 56, ci->highcyl <= 0 ? ci->cyls - 1 : ci->highcyl - 1); /* hiCyl */
			trap_put_long(ctx, parmpacket + 48, ci->interleave); /* interleave */
			trap_put_long(ctx, parmpacket + 60, ci->buffers); /* Number of buffers */
			trap_put_long(ctx, parmpacket + 64, ci->bufmemtype); /* Buffer mem type */
			trap_put_long(ctx, parmpacket + 68, ci->maxtransfer); /* largest transfer */
			trap_put_long(ctx, parmpacket + 72, ci->mask); /* dma mask */
			trap_put_long(ctx, parmpacket + 80, DISK_TYPE_DOS); /* DOS\0 */
			memset(buf, 0, sizeof buf);
			if (ci->dostype) { // forced dostype?
				trap_put_long(ctx, parmpacket + 80, ci->dostype); /* dostype */
			} else if (hdf_read (&uip[unit_no].hf, buf, 0, sizeof buf)) {
				uae_u32 dt = rl (buf);
				if (dt != 0x00000000 && dt != 0xffffffff)
					trap_put_long(ctx, parmpacket + 80, dt);
			}
			memset(buf, 0, sizeof buf);
			char *s = ua_fs(uip[unit_no].devname, -1);
			buf[36] = (uae_u8)strlen(s);
			for (int i = 0; i < buf[36]; i++)
				buf[37 + i] = s[i];
			xfree(s);
			for (int i = 0; i < 80; i++)
				buf[i + 128] = trap_get_byte(ctx, parmpacket + 16 + i);
			dump_partinfo (&uip[unit_no].hf, buf);
		}
		if (type == FILESYS_HARDFILE)
			type = dofakefilesys(ctx, &uip[unit_no], parmpacket, ci);
		if (uip[unit_no].bootpri < -127 || (type == FILESYS_HARDFILE && ci->rootdir[0] == 0))
			trap_set_dreg(ctx, 7, trap_get_dreg(ctx, 7) & ~1); /* do not boot */
		if (uip[unit_no].bootpri < -128)
			return -1; /* do not mount */
		return type;
	}
}

static uae_u32 REGPARAM2 mousehack_done (TrapContext *ctx)
{
	int mode = trap_get_dreg(ctx, 1);
	if (mode < 10) {
		uaecptr diminfo = trap_get_areg(ctx, 2);
		uaecptr dispinfo = trap_get_areg(ctx, 3);
		uaecptr vp = trap_get_areg(ctx, 4);
		return input_mousehack_status(ctx, mode, diminfo, dispinfo, vp, trap_get_dreg(ctx, 2));
	} else if (mode == 10) {
		amiga_clipboard_die(ctx);
	} else if (mode == 11) {
		amiga_clipboard_got_data(ctx, trap_get_areg(ctx, 2), trap_get_dreg(ctx, 2), trap_get_dreg(ctx, 0) + 8);
	} else if (mode == 12) {
		return amiga_clipboard_want_data(ctx);
	} else if (mode == 13) {
		return amiga_clipboard_proc_start(ctx);
	} else if (mode == 14) {
		amiga_clipboard_task_start(ctx, trap_get_dreg(ctx, 0));
	} else if (mode == 15) {
		amiga_clipboard_init(ctx);
	} else if (mode == 16) {
		uaecptr a2 = trap_get_areg(ctx, 2);
		input_mousehack_mouseoffset(a2);
	} else if (mode == 17) {
		uae_u32 v = 0;
		if (currprefs.clipboard_sharing)
			v |= 1;
		if (consolehook_activate())
			v |= 2;
		if (currprefs.uaeboard > 2)
			v |= 4;
		trap_dos_active();
		return v;
	} else if (mode == 18) {
		put_long_host(rtarea_bank.baseaddr + RTAREA_EXTERTASK, trap_get_dreg(ctx, 0));
		put_long_host(rtarea_bank.baseaddr + RTAREA_TRAPTASK, trap_get_dreg(ctx, 2));
		return rtarea_base + RTAREA_HEARTBEAT;
	} else if (mode == 19) {
		// boot rom copy
		// d2 = ram address
		return boot_rom_copy(ctx, trap_get_dreg(ctx, 2), 0);
	} else if (mode == 20) {
		// boot rom copy done
		return boot_rom_copy(ctx, trap_get_dreg(ctx, 2), 1);
	} else if (mode == 21) {
		// keymap hook
#ifdef RETROPLATFORM
		rp_keymap(ctx, trap_get_areg(ctx, 1), trap_get_dreg(ctx, 0));
#endif
		return 1;
	} else if (mode == 101) {
		consolehook_ret(ctx, trap_get_areg(ctx, 1), trap_get_areg(ctx, 2));
	} else if (mode == 102) {
		uaecptr ret = consolehook_beginio(ctx, trap_get_areg(ctx, 1));
		trap_put_long(ctx, trap_get_areg(ctx, 7) + 4 * 4, ret);
	} else if (mode == 200) {
#ifdef DEBUGGER
		uae_u32 v;
		// a0 = data, d0 = length, a1 = task, d3 = stack size (in), stack ptr (out)
		// a2 = debugdata, d2 = debuglength
		// d4 = flags
		if ((trap_get_dreg(ctx, 4) & 3) != 1) {
			write_log(_T("unsupported uaedbg version\n"));
			return 0;
		}
		uae_u32 stack = trap_get_dreg(ctx, 3);
		v = debugmem_reloc(trap_get_areg(ctx, 0), trap_get_dreg(ctx, 0),
			trap_get_areg(ctx, 2), trap_get_dreg(ctx, 2),
			trap_get_areg(ctx, 1), &stack);
		trap_set_dreg(ctx, 2, stack);
		return v;
#endif
	} else if (mode == 201) {
#ifdef DEBUGGER
		debugmem_break(8);
#endif
		return 1;
	} else if (mode == 202) {
		// a0 = seglist, a1 = name, d2 = lock
#ifdef DEBUGGER
		debugmem_addsegs(ctx, trap_get_areg(ctx, 0), trap_get_areg(ctx, 1), trap_get_dreg(ctx, 2), true);
#endif
		return 1;
	} else if (mode == 203) {
		// a0 = seglist
#ifdef DEBUGGER
		debugmem_remsegs(trap_get_areg(ctx, 0));
#endif
		return 1;
	} else if (mode == 204 || mode == 206) {
		// d0 = size, a1 = flags
#ifdef DEBUGGER
		uae_u32 v = debugmem_allocmem(mode == 206, trap_get_dreg(ctx, 0), trap_get_areg(ctx, 1), trap_get_areg(ctx, 0));
		if (v) {
			trap_set_areg(ctx, 0, v);
			return v;
		} else {
			trap_set_areg(ctx, 0, 0);
			trap_set_dreg(ctx, 1, trap_get_areg(ctx, 1));
			return trap_get_dreg(ctx, 0);
		}
#endif
	} else if (mode == 205 || mode == 207) {
#ifdef DEBUGGER
		return debugmem_freemem(mode == 207, trap_get_areg(ctx, 1), trap_get_dreg(ctx, 0), trap_get_areg(ctx, 0));
#endif
	} else if (mode == 208) {
		// segtrack: bit 0
		// fsdebug: bit 1
		segtrack_mode = currprefs.debugging_features;
		return segtrack_mode;
	} else if (mode == 209) {
		// called if segtrack was enabled
		return 0;
	} else if (mode == 210) {
		// debug trapcode
#ifdef DEBUGGER
		debugmem_trap(trap_get_areg(ctx, 0));
#endif
	} else if (mode == 212) {
		// a0 = seglist, a1 = name, d2 = lock
#ifdef DEBUGGER
		debugmem_addsegs(ctx, trap_get_areg(ctx, 0), trap_get_areg(ctx, 1), trap_get_dreg(ctx, 2), false);
#endif
		return 1;
	} else if (mode == 213) {
		// a0 = seglist
#ifdef DEBUGGER
		debugmem_remsegs(trap_get_areg(ctx, 0));
#endif
		return 1;
	} else if (mode == 299) {
#ifdef DEBUGGER
		return debugmem_exit();
#endif
	} else {
		write_log (_T("Unknown mousehack hook %d\n"), mode);
	}
	return 1;
}

void filesys_vsync (void)
{
	TrapContext *ctx = NULL;
	Unit *u;

	if (uae_boot_rom_type <= 0)
		return;

	if (shellexecute2_queued > 0) {
		if (!uae_sem_trywait(&shellexec_sem)) {
			filesys_shellexecute2_run_queue();
			uae_sem_post(&shellexec_sem);
		}
	}

	if (currprefs.uaeboard > 1 && !(uae_int_requested & 1)) {
		bool req = false;
		// check if there is waiting requests without signal
		if (!uae_sem_trywait(&singlethread_int_sem)) {
			if (comm_pipe_has_data(&native2amiga_pending))
				req = true;
			if (!req) {
				UnitInfo *uip = mountinfo.ui;
				int unit_no = 0;
				for (;;) {
					if (unit_no >= MAX_FILESYSTEM_UNITS)
						break;
					if (uip[unit_no].open > 0 && uip[unit_no].self != 0
						&& uip[unit_no].self->cmds_acked == uip[unit_no].self->cmds_complete
						&& uip[unit_no].self->cmds_acked != uip[unit_no].self->cmds_sent)
						break;
					unit_no++;
				}
				if (unit_no < MAX_FILESYSTEM_UNITS) {
					if (uip[unit_no].self->port)
						req = true;
				}
			}
			uae_sem_post(&singlethread_int_sem);
			if (req)
				do_uae_int_requested();
		}
	}

	if (media_queued_total > 0) {
		if (automountunit == -1) {
			bool cont = true;
			for (u = units; u; u = u->next) {
				if (u->reinsertdelay > 0) {
					cont = false;
					break;
				}
			}
			if (cont) {
				TCHAR *mountpath = media_queued_paths[media_queued_cnt++];
				if (mountpath) {
					filesys_media_change(mountpath, 3, NULL);
				}
				if (media_queued_cnt >= media_queued_total) {
					for (int i = 0; i < media_queued_total; i++) {
						xfree(media_queued_paths[i]);
					}
					xfree(media_queued_paths);
					media_queued_cnt = 0;
					media_queued_total = 0;
					media_queued_paths = NULL;
					for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
						if (mountinfo.ui[i].canremove < 0)
							mountinfo.ui[i].canremove = 1;
					}
				}
			}
		}
	}

	if (heartbeat == get_long_host(rtarea_bank.baseaddr + RTAREA_HEARTBEAT)) {
		if (heartbeat_count > 0)
			heartbeat_count--;
		if (heartbeat_count_cont > 0)
			heartbeat_count_cont--;
		return;
	}
	heartbeat = get_long_host(rtarea_bank.baseaddr + RTAREA_HEARTBEAT);
	heartbeat_count_cont = 10;
	cia_heartbeat ();

	for (u = units; u; u = u->next) {
		if (u->reinsertdelay > 0) {
			u->reinsertdelay--;
			if (u->reinsertdelay == 0) {
				if (u->newrootdir == NULL && u->newvolume == NULL) {
					filesys_eject(u->unit);
				} else {
					filesys_insert(u->unit, u->newvolume, u->newrootdir, u->newreadonly, u->newflags);
				}
				xfree (u->newvolume);
				u->newvolume = NULL;
				xfree (u->newrootdir);
				u->newrootdir = NULL;
			}
		}
		record_timeout(ctx, u);
	}

	for (int i = 0; i < currprefs.mountitems; i++) {
		struct hardfiledata *hfd = get_hardfile_data (currprefs.mountconfig[i].configoffset);
		if (!hfd)
			continue;
		if (hfd->reinsertdelay > 0) {
			hfd->reinsertdelay--;
			if (hfd->reinsertdelay == 0) {
				hfd->reinsertdelay = -1;
				hardfile_media_change (hfd, &hfd->delayedci, true, true);
			}
		}
	}

	if (heartbeat_count <= 0)
		return;

	if (heartbeat_task & 1) {
		setsystime_vblank ();
		heartbeat_task &= ~1;
	}
}

void filesys_cleanup(void)
{
	filesys_prepare_reset();
	filesys_reset2();
	filesys_free_handles();
	free_mountinfo();
	destroy_comm_pipe(&shellexecute_pipe);
	uae_sem_destroy(&singlethread_int_sem);
	uae_sem_destroy(&shellexec_sem);
	shell_execute_data = 0;
	free_shellexecute();
}

void filesys_install (void)
{
	uaecptr loop;

	TRACEI ((_T("Installing filesystem\n")));

	uae_sem_init(&singlethread_int_sem, 0, 1);
	uae_sem_init(&shellexec_sem, 0, 1);
	init_comm_pipe(&shellexecute_pipe, 100, 1);

	ROM_filesys_resname = ds_ansi ("UAEfs.resource");
	ROM_filesys_resid = ds_ansi (UAEFS_VERSION);

	fsdevname = ROM_hardfile_resname;
	fshandlername = ds_bstr_ansi ("uaefs");

	cdfs_devname = ds_ansi ("uaescsi.device");
	cdfs_handlername = ds_bstr_ansi ("uaecdfs");

	afterdos_name = ds_ansi("UAE afterdos");
	afterdos_id = ds_ansi("UAE afterdos 0.2");

	keymaphook_name = ds_ansi("UAE keymaphook");
	keymaphook_id = ds_ansi("UAE keymaphook 0.1");

	ROM_filesys_diagentry = here ();
	calltrap (deftrap2 (filesys_diagentry, 0, _T("filesys_diagentry")));
	dw(0x4ED0); /* JMP (a0) - jump to code that inits Residents */

	ROM_filesys_doio = here();
	calltrap(deftrap2(filesys_doio, 0, _T("filesys_doio")));
	dw(RTS);

	ROM_filesys_putmsg = here();
	calltrap(deftrap2(filesys_putmsg, 0, _T("filesys_putmsg")));
	dw(RTS);

	ROM_filesys_putmsg_return = here();
	calltrap(deftrap2(filesys_putmsg_return, 0, _T("filesys_putmsg_return")));
	dw(RTS);

	loop = here ();

	org (rtarea_base + RTAREA_HEARTBEAT);
	dl (0);
	heartbeat = 0;
	heartbeat_task = 0;

	org (rtarea_base + 0xFF18);
	calltrap (deftrap2 (filesys_dev_bootfilesys, 0, _T("filesys_dev_bootfilesys")));
	dw (RTS);

	/* Special trap for the assembly make_dev routine */
	org (rtarea_base + 0xFF20);
	calltrap (deftrap2 (filesys_dev_remember, 0, _T("filesys_dev_remember")));
	dw (RTS);

	org (rtarea_base + 0xFF28);
	calltrap (deftrap2 (filesys_dev_storeinfo, 0, _T("filesys_dev_storeinfo")));
	dw (RTS);

	org (rtarea_base + 0xFF30);
	calltrap (deftrap2 (filesys_handler, 0, _T("filesys_handler")));
	dw (RTS);

	org (rtarea_base + 0xFF38);
	calltrap (deftrapres(mousehack_done, 0, _T("misc_funcs")));
	dw (RTS);

	org (rtarea_base + 0xFF40);
	calltrap (deftrap2 (startup_handler, 0, _T("startup_handler")));
	dw (RTS);

	org (rtarea_base + 0xFF48);
	calltrap (deftrap2 (filesys_init_storeinfo, TRAPFLAG_EXTRA_STACK, _T("filesys_init_storeinfo")));
	dw (RTS);

	org (rtarea_base + 0xFF50);
	calltrap (deftrap2 (exter_int_helper, 0, _T("exter_int_helper")));
	dw (RTS);

	org (rtarea_base + 0xFF58);
	calltrap (deftrap2 (fsmisc_helper, 0, _T("fsmisc_helper")));
	dw (RTS);

	org(rtarea_base + 0xFF68);
	calltrap(deftrap2(filesys_bcpl_wrapper, 0, _T("filesys_bcpl_wrapper")));
	dw(RTS);

	org(rtarea_base + 0xFF78);
	calltrap(deftrap2(debugger_helper, 0, _T("debugger_helper")));
	dw(RTS);

	org (loop);

	create_ks12_boot();
	create_68060_nofpu();
}

uaecptr filesys_get_entry(int index)
{
	return bootrom_start + dlg(bootrom_start + bootrom_header + index * 4 - 4) + bootrom_header - 4;
}

void filesys_install_code (void)
{
	uae_u32 b, items;

	bootrom_header = 3 * 4;
	align(4);
	bootrom_start = here ();
#include "filesys_bootrom.cpp.in"

	items = dlg (bootrom_start + 8) & 0xffff;
	/* The last offset comes from the code itself, look for it near the top. */
	EXPANSION_bootcode = bootrom_start + bootrom_header + items * 4 - 4;
	b = bootrom_start + bootrom_header + 3 * 4 - 4;
	filesys_initcode = bootrom_start + dlg (b) + bootrom_header - 4;
	afterdos_initcode = filesys_get_entry(8);
	keymaphook_initcode = filesys_get_entry(11);

	// Fill struct resident
	TCHAR buf[256];
	TCHAR *s = au(UAEFS_VERSION);
	int y, m, d;
	target_getdate(&y, &m, &d);
	_sntprintf(buf, sizeof buf, _T("%s (%d.%d.%d)\r\n"), s, d, m, y);
	uaecptr idstring = ds(buf);
	xfree(s);

	b = here();
	items = dlg(bootrom_start) * 2;
	for (int i = 0; i < items; i += 2) {
		if (((dbg(bootrom_start + i + 0) << 8) | (dbg(bootrom_start + i + 1) << 0)) == 0x4afc) {
			org(bootrom_start + i + 2);
			dl(bootrom_start + i);
			dl(dlg(here()) + bootrom_start + i); // endskip
			db(0); // flags
			db(1); // version
			db(0); // type
			db(0); // pri
			dl(dlg(here()) + bootrom_start + i); // name
			dl(idstring); // idstring
			dl(dlg(here()) + bootrom_start + i); // init
			break;
		}
	}
	org(b);

}

#ifdef _WIN32
#include "od-win32/win32_filesys.cpp"
#endif

static uae_u8 *restore_filesys_hardfile (UnitInfo *ui, uae_u8 *src)
{
	struct hardfiledata *hfd = &ui->hf;
	TCHAR *s;

	hfd->virtsize = restore_u64 ();
	hfd->offset = restore_u64 ();
	hfd->ci.highcyl = restore_u32 ();
	hfd->ci.sectors = restore_u32 ();
	hfd->ci.surfaces = restore_u32 ();
	hfd->ci.reserved = restore_u32 ();
	hfd->ci.blocksize = restore_u32 ();
	hfd->ci.readonly = restore_u32 () != 0;
	hfd->flags = restore_u32 ();
	hfd->rdbcylinders = restore_u32 ();
	hfd->rdbsectors = restore_u32 ();
	hfd->rdbheads = restore_u32 ();
	s = restore_string ();
	_tcscpy (hfd->vendor_id, s);
	xfree (s);
	s = restore_string ();
	_tcscpy (hfd->product_id, s);
	xfree (s);
	s = restore_string ();
	_tcscpy (hfd->product_rev, s);
	xfree (s);
	s = restore_string ();
	_tcscpy (hfd->ci.devname, s);
	xfree (s);
	return src;
}

static uae_u8 *save_filesys_hardfile (UnitInfo *ui, uae_u8 *dst)
{
	struct hardfiledata *hfd = &ui->hf;

	save_u64 (hfd->virtsize);
	save_u64 (hfd->offset);
	save_u32 (hfd->ci.highcyl);
	save_u32 (hfd->ci.sectors);
	save_u32 (hfd->ci.surfaces);
	save_u32 (hfd->ci.reserved);
	save_u32 (hfd->ci.blocksize);
	save_u32 (hfd->ci.readonly);
	save_u32 (hfd->flags);
	save_u32 (hfd->rdbcylinders);
	save_u32 (hfd->rdbsectors);
	save_u32 (hfd->rdbheads);
	save_string (hfd->vendor_id);
	save_string (hfd->product_id);
	save_string (hfd->product_rev);
	save_string (hfd->ci.devname);
	return dst;
}

static a_inode *restore_filesys_get_base (Unit *u, TCHAR *npath)
{
	TCHAR *path, *p, *p2;
	a_inode *a;
	int cnt, err, i;

	/* no '/' = parent is root */
	if (!_tcschr (npath, '/'))
		return &u->rootnode;

	/* iterate from root to last to previous path part,
	* create ainos if not already created.
	*/
	path = xcalloc(TCHAR, uaetcslen (npath) + 2);
	cnt = 1;
	for (;;) {
		_tcscpy (path, npath);
		p = path;
		for (i = 0; i < cnt ;i++) {
			if (i > 0)
				p++;
			while (*p != '/' && *p != 0)
				p++;
		}
		if (*p) {
			*p = 0;
			err = 0;
			get_aino (u, &u->rootnode, path, &err);
			if (err) {
				write_log (_T("*** FS: missing path '%s'!\n"), path);
				return NULL;
			}
			cnt++;
		} else {
			break;
		}
	}

	/* find base (parent) of last path part */
	_tcscpy (path, npath);
	p = path;
	a = u->rootnode.child;
	for (;;) {
		if (*p == 0) {
			write_log (_T("*** FS: base aino NOT found '%s' ('%s')\n"), a->nname, npath);
			xfree (path);
			return NULL;
		}
		p2 = p;
		while(*p2 != '/' && *p2 != '\\' && *p2 != 0)
			p2++;
		*p2 = 0;
		while (a) {
			if (!same_aname(p, a->aname)) {
				a = a->sibling;
				continue;
			}
			p = p2 + 1;
			if (*p == 0) {
				write_log (_T("FS: base aino found '%s' ('%s')\n"), a->nname, npath);
				xfree (path);
				return a;
			}
			a = a->child;
			break;
		}
		if (!a) {
			write_log (_T("*** FS: path part '%s' not found ('%s')\n"), p, npath);
			xfree (path);
			return NULL;
		}
	}
}

static TCHAR *makenativepath (UnitInfo *ui, TCHAR *apath)
{
	TCHAR *pn;
	/* create native path. FIXME: handle 'illegal' characters */
	pn = xcalloc (TCHAR, uaetcslen (apath) + 1 + uaetcslen (ui->rootdir) + 1);
	_sntprintf (pn, sizeof pn, _T("%s/%s"), ui->rootdir, apath);
	if (FSDB_DIR_SEPARATOR != '/') {
		for (int i = 0; i < uaetcslen (pn); i++) {
			if (pn[i] == '/')
				pn[i] = FSDB_DIR_SEPARATOR;
		}
	}
	return pn;
}

static uae_u8 *restore_aino (UnitInfo *ui, Unit *u, uae_u8 *src)
{
	TCHAR *p, *p2, *pn;
	uae_u32 flags;
	int missing;
	a_inode *base, *a;

	missing = 0;
	a = xcalloc (a_inode, 1);
	a->uniq = restore_u64to32();
	a->locked_children = restore_u32();
	a->exnext_count = restore_u32();
	a->shlock = restore_u32();
	flags = restore_u32();
	if (flags & 1)
		a->elock = 1;
	if (flags & 4)
		a->uniq_external = restore_u64 ();
	/* full Amiga-side path without drive, eg. "C/SetPatch" */
	p = restore_string ();
	/* root (p = volume label) */
	if (a->uniq == 0) {
		a->nname = my_strdup (ui->rootdir);
		a->aname = p;
		a->dir = 1;
		if (ui->volflags < 0) {
			write_log (_T("FS: Volume '%s' ('%s') missing!\n"), a->aname, a->nname);
		} else {
			a->volflags = ui->volflags;
			recycle_aino (u, a);
			write_log (_T("FS: Lock (root) '%s' ('%s')\n"), a->aname, a->nname);
		}
		return src;
	}
	p2 = _tcsrchr(p, '/');
	if (p2)
		p2++;
	else
		p2 = p;
	pn = makenativepath (ui, p);
	a->nname = pn;
	a->aname = my_strdup (p2);
	/* create path to parent dir */
	if (p2 != p)
		p2[0] = 0;
	/* find parent of a->aname (Already restored previously. I hope..) */
	base = restore_filesys_get_base (u, p);
	xfree(p);
	if (flags & 2) {
		a->dir = 1;
		if (!my_existsdir(a->nname))
			write_log (_T("*** FS: Directory '%s' missing!\n"), a->nname);
		else
			fsdb_clean_dir (a);
	} else {
		if (!my_existsfile(a->nname))
			write_log (_T("*** FS: File '%s' missing!\n"), a->nname);
	}
	if (base) {
		fill_file_attrs (u, base, a);
		init_child_aino_tree (u, base, a);
	} else {
		write_log (_T("*** FS: parent directory missing '%s' ('%s')\n"), a->aname, a->nname);
		missing = 1;
	}
	if (missing) {
		write_log (_T("*** FS: Lock restore failed '%s' ('%s')\n"), a->aname, a->nname);
		xfree (a->nname);
		xfree (a->aname);
		xfree (a);
	} else {
		write_log (_T("FS: Lock '%s' ('%s')\n"), a->aname, a->nname);
		recycle_aino (u, a);
	}
	return src;
}

static uae_u8 *restore_key (UnitInfo *ui, Unit *u, uae_u8 *src)
{
	int uniq;
	TCHAR *p, *pn;
	mode_t openmode;
	int err;
	int missing;
	a_inode *a;
	Key *k;
	uae_u64 savedsize, size, pos;

	missing = 0;
	k = xcalloc (Key, 1);
	k->uniq = restore_u64to32();
	k->file_pos = restore_u32();
	k->createmode = restore_u32();
	k->dosmode = restore_u32();
	savedsize = restore_u32();
	uniq = restore_u64to32();
	p = restore_string();
	pos = restore_u64();
	size = restore_u64();
	if (size) {
		savedsize = size;
		k->file_pos = pos;
	}
	pn = makenativepath (ui, p);
	openmode = ((k->dosmode & A_FIBF_READ) == 0 ? O_WRONLY
		: (k->dosmode & A_FIBF_WRITE) == 0 ? O_RDONLY
		: O_RDWR);
	write_log (_T("FS: open file '%s' ('%s'), pos=%llu\n"), p, pn, k->file_pos);
	a = get_aino (u, &u->rootnode, p, &err);
	if (!a)
		write_log (_T("*** FS: Open file aino creation failed '%s'\n"), p);
	missing = 1;
	if (a) {
		missing = 0;
		k->aino = a;
		if (a->uniq != uniq)
			write_log (_T("*** FS: Open file '%s' aino id %d != %d\n"), p, uniq, a->uniq);
		if (!my_existsfile (pn)) {
			write_log (_T("*** FS: Open file '%s' is missing, creating dummy file!\n"), p);
			if (savedsize < 10 * 1024 * 1024) {
				k->fd = fs_openfile (u, a, openmode | O_CREAT |O_BINARY);
				if (k->fd) {
					uae_u8 *buf = xcalloc (uae_u8, 10000);
					uae_u64 sp = savedsize;
					while (sp) {
						uae_s64 s = sp >= 10000 ? 10000 : sp;
						fs_write (k->fd, buf, (uae_u32)s);
						sp -= s;
					}
					xfree(buf);
					write_log (_T("*** FS: dummy file created\n"));
				} else {
					write_log (_T("*** FS: Open file '%s', couldn't create dummy file!\n"), p);
				}
			} else {
				write_log (_T("*** FS: Too big, ignored\n"));
			}
		} else {
			k->fd = fs_openfile (u, a, openmode | O_BINARY);
		}
		if (!k->fd) {
			write_log (_T("*** FS: Open file '%s' failed to open!\n"), p);
			missing = 1;
		} else {
			uae_s64 s;
			s = key_filesize(k);
			if (s != savedsize)
				write_log (_T("FS: restored file '%s' size changed! orig=%llu, now=%lld!!\n"), p, savedsize, s);
			if (k->file_pos > s) {
				write_log (_T("FS: restored filepos larger than size of file '%s'!! %llu > %lld\n"), p, k->file_pos, s);
				k->file_pos = s;
			}
			key_seek(k, k->file_pos, SEEK_SET);
		}
	}
	xfree (p);
	if (missing) {
		xfree (k);
	} else {
		k->next = u->keys;
		u->keys = k;
	}
	return src;
}

static uae_u8 *restore_notify (UnitInfo *ui, Unit *u, uae_u8 *src)
{
	Notify *n = xcalloc (Notify, 1);
	uae_u32 hash;
	TCHAR *s;

	n->notifyrequest = restore_u32 ();
	s = restore_string ();
	n->fullname = xmalloc (TCHAR, uaetcslen (ui->volname) + 2 + uaetcslen (s) + 1);
	_sntprintf (n->fullname, sizeof n->fullname, _T("%s:%s"), ui->volname, s);
	xfree(s);
	s = _tcsrchr (n->fullname, '/');
	if (s)
		s++;
	else
		s = n->fullname;
	n->partname = my_strdup (s);
	hash = notifyhash (n->fullname);
	n->next = u->notifyhash[hash];
	u->notifyhash[hash] = n;
	write_log (_T("FS: notify %08X '%s' '%s'\n"), n->notifyrequest, n->fullname, n->partname);
	return src;
}

static uae_u8 *restore_exkey (UnitInfo *ui, Unit *u, uae_u8 *src)
{
	restore_u64 ();
	restore_u64 ();
	restore_u64 ();
	return src;
}

static uae_u8 *restore_filesys_virtual (UnitInfo *ui, uae_u8 *src, int num)
{
	Unit *u = startup_create_unit(NULL, ui, num);
	int cnt;

	u->dosbase = restore_u32();
	u->volume = restore_u32();
	u->port = restore_u32();
	u->locklist = restore_u32();
	u->dummy_message = restore_u32();
	u->cmds_sent = restore_u64to32();
	u->cmds_complete = restore_u64to32();
	u->cmds_acked = restore_u64to32();
	u->next_exkey = restore_u32();
	u->total_locked_ainos = restore_u32();
	u->volflags = ui->volflags;

	cnt = restore_u32();
	write_log (_T("FS: restoring %d locks\n"), cnt);
	while (cnt-- > 0)
		src = restore_aino (ui, u, src);

	cnt = restore_u32();
	write_log (_T("FS: restoring %d open files\n"), cnt);
	while (cnt-- > 0)
		src = restore_key (ui, u, src);

	cnt = restore_u32();
	write_log (_T("FS: restoring %d notifications\n"), cnt);
	while (cnt-- > 0)
		src = restore_notify (ui, u, src);

	cnt = restore_u32();
	write_log (_T("FS: restoring %d exkeys\n"), cnt);
	while (cnt-- > 0)
		src = restore_exkey (ui, u, src);

	return src;
}

static TCHAR *getfullaname (a_inode *a)
{
	TCHAR *p;
	int first = 1;

	p = xcalloc (TCHAR, MAX_DPATH);
	while (a) {
		int len = uaetcslen (a->aname);
		memmove (p + len + 1, p, (uaetcslen (p) + 1) * sizeof (TCHAR));
		memcpy (p, a->aname, len * sizeof (TCHAR));
		if (!first)
			p[len] = '/';
		first = 0;
		a = a->parent;
		if (a && a->uniq == 0)
			return p;
	}
	return p;
}

/* scan and save all Lock()'d files */
static int recurse_aino (UnitInfo *ui, a_inode *a, int cnt, uae_u8 **dstp)
{
	uae_u8 *dst = NULL;
	int dirty = 0;
	a_inode *a2 = a;

	if (dstp)
		dst = *dstp;
	while (a) {
		//write_log("recurse '%s' '%s' %d %08x\n", a->aname, a->nname, a->uniq, a->parent);
		if (a->elock || a->shlock || a->uniq == 0) {
			if (dst) {
				TCHAR *fn = NULL;
				write_log (_T("uniq=%d %lld s=%d e=%d d=%d '%s' '%s'\n"), a->uniq, a->uniq_external, a->shlock, a->elock, a->dir, a->aname, a->nname);
				if (a->aname) {
					fn = getfullaname (a);
					write_log (_T("->'%s'\n"), fn);
				}
				save_u64 (a->uniq);
				save_u32 (a->locked_children);
				save_u32 (a->exnext_count);
				save_u32 (a->shlock);
				save_u32 ((a->elock ? 1 : 0) | (a->dir ? 2 : 0) | 4);
				save_u64 (a->uniq_external);
				save_string (fn);
				xfree(fn);
			}
			cnt++;
		}
		if (a->dirty)
			dirty = 1;
		if (a->child)
			cnt = recurse_aino (ui, a->child, cnt, &dst);
		a = a->sibling;
	}
	if (dirty && a2->parent)
		fsdb_dir_writeback (a2->parent);
	if (dst)
		*dstp = dst;
	return cnt;
}

static uae_u8 *save_key (uae_u8 *dst, Key *k)
{
	TCHAR *fn = getfullaname (k->aino);
	uae_u64 size;
	save_u64 (k->uniq);
	save_u32 ((uae_u32)k->file_pos);
	save_u32 (k->createmode);
	save_u32 (k->dosmode);
	size = fs_fsize (k->fd);
	save_u32 ((uae_u32)size);
	save_u64 (k->aino->uniq);
	save_string (fn);
	save_u64 (k->file_pos);
	save_u64 (size);
	write_log (_T("'%s' uniq=%d size=%lld seekpos=%lld mode=%d dosmode=%d\n"),
		fn, k->uniq, size, k->file_pos, k->createmode, k->dosmode);
	xfree (fn);
	return dst;
}

static uae_u8 *save_notify (UnitInfo *ui, uae_u8 *dst, Notify *n)
{
	TCHAR *s;
	save_u32 (n->notifyrequest);
	s = n->fullname;
	if (uaetcslen (s) >= uaetcslen (ui->volname) && !_tcsncmp (n->fullname, ui->volname, uaetcslen (ui->volname)))
		s = n->fullname + uaetcslen (ui->volname) + 1;
	save_string (s);
	write_log (_T("FS: notify %08X '%s'\n"), n->notifyrequest, n->fullname);
	return dst;
}

static uae_u8 *save_exkey (uae_u8 *dst, ExamineKey *ek)
{
	save_u64 (ek->uniq);
	save_u64 (ek->aino->uniq);
	save_u64 (ek->curr_file->uniq);
	return dst;
}

static uae_u8 *save_filesys_virtual (UnitInfo *ui, uae_u8 *dst)
{
	Unit *u = ui->self;
	Key *k;
	int cnt, i, j;

	write_log (_T("FSSAVE: '%s'\n"), ui->devname);
	save_u32 (u->dosbase);
	save_u32 (u->volume);
	save_u32 (u->port);
	save_u32 (u->locklist);
	save_u32 (u->dummy_message);
	save_u64 (u->cmds_sent);
	save_u64 (u->cmds_complete);
	save_u64 (u->cmds_acked);
	save_u32 (u->next_exkey);
	save_u32 (u->total_locked_ainos);
	cnt = recurse_aino (ui, &u->rootnode, 0, NULL);
	save_u32 (cnt);
	write_log (_T("%d open locks\n"), cnt);
	cnt = recurse_aino (ui, &u->rootnode, 0, &dst);
	cnt = 0;
	for (k = u->keys; k; k = k->next)
		cnt++;
	save_u32 (cnt);
	write_log (_T("%d open files\n"), cnt);
	for (k = u->keys; k; k = k->next)
		dst = save_key (dst, k);
	for (j = 0; j < 2; j++) {
		cnt = 0;
		for (i = 0; i < NOTIFY_HASH_SIZE; i++) {
			Notify *n = u->notifyhash[i];
			while (n) {
				if (j > 0)
					dst = save_notify (ui, dst, n);
				cnt++;
				n = n->next;
			}
		}
		if (j == 0) {
			save_u32 (cnt);
			write_log (_T("%d notify requests\n"), cnt);
		}
	}
	for (j = 0; j < 2; j++) {
		cnt = 0;
		for (i = 0; i < EXKEYS; i++) {
			ExamineKey *ek = &u->examine_keys[i];
			if (ek->uniq) {
				cnt++;
				if (j > 0)
					dst = save_exkey (dst, ek);
			}
		}
		if (j == 0) {
			save_u32 (cnt);
			write_log (_T("%d exkeys\n"), cnt);
		}
	}
	write_log (_T("END\n"));
	return dst;
}


static TCHAR *new_filesys_root_path, *new_filesys_fs_path;

uae_u8 *save_filesys_common(size_t *len)
{
	uae_u8 *dstbak, *dst;
	if (nr_units () == 0)
		return NULL;
	dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (2);
	save_u64 (a_uniq);
	save_u64 (key_uniq);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_filesys_common(uae_u8 *src)
{
	if (restore_u32 () != 2)
		return src;
	cd_unit_offset = MAX_FILESYSTEM_UNITS;
	cd_unit_number = 0;
	filesys_prepare_reset2 ();
	filesys_reset2 ();
	a_uniq = restore_u64to32();
	key_uniq = restore_u64to32();

	xfree(new_filesys_root_path);
	xfree(new_filesys_fs_path);
	new_filesys_root_path = NULL;
	new_filesys_fs_path = NULL;
	return src;
}

uae_u8 *save_filesys_paths(int num, size_t *len)
{
	uae_u8 *dstbak, *dst;
	UnitInfo *ui;
	int type = is_hardfile(num);
	int ptype;

	ui = &mountinfo.ui[num];
	if (ui->open <= 0)
		return NULL;
	dstbak = dst = xmalloc(uae_u8, 4 + 4 + 2 + 2 * 2 * MAX_DPATH);

	if (type == FILESYS_VIRTUAL || type == FILESYS_CD)
		ptype = SAVESTATE_PATH_VDIR;
	else if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB)
		ptype = SAVESTATE_PATH_HDF;
	else if (type == FILESYS_HARDDRIVE)
		ptype = SAVESTATE_PATH_HD;
	else
		ptype = SAVESTATE_PATH;

	save_u32(0);
	save_u32(ui->devno);
	save_u16(type);
	save_path_full(ui->rootdir, ptype);
	save_path_full(ui->filesysdir, SAVESTATE_PATH);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_filesys(int num, size_t *len)
{
	uae_u8 *dstbak, *dst;
	UnitInfo *ui;
	int type = is_hardfile (num);
	int ptype;

	ui = &mountinfo.ui[num];
	if (ui->open <= 0)
		return NULL;
	/* not initialized yet, do not save */
	if ((type == FILESYS_VIRTUAL || type == FILESYS_CD) && ui->self == NULL)
		return NULL;
	write_log (_T("FS_FILESYS: '%s' '%s'\n"), ui->devname, ui->volname ? ui->volname : _T("<no name>"));
	dstbak = dst = xmalloc (uae_u8, 100000);
	save_u32 (2); /* version */
	save_u32 (ui->devno);
	save_u16 (type);
	if (type == FILESYS_VIRTUAL || type == FILESYS_CD)
		ptype = SAVESTATE_PATH_VDIR;
	else if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB)
		ptype = SAVESTATE_PATH_HDF;
	else if (type == FILESYS_HARDDRIVE)
		ptype = SAVESTATE_PATH_HD;
	else
		ptype = SAVESTATE_PATH;
	save_path(ui->rootdir, ptype);
	save_string (ui->devname);
	save_string (ui->volname);
	save_path (ui->filesysdir, SAVESTATE_PATH);
	save_u8 (ui->bootpri);
	save_u8 (ui->readonly);
	save_u32 (ui->startup);
	save_u32 (filesys_configdev);
	if (type == FILESYS_VIRTUAL || type == FILESYS_CD)
		dst = save_filesys_virtual (ui, dst);
	if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB)
		dst = save_filesys_hardfile (ui, dst);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_filesys_paths(uae_u8 *src)
{
	restore_u32();
	restore_u32();
	restore_u16();
	new_filesys_root_path = restore_path_full();
	new_filesys_fs_path = restore_path_full();
	return src;
}

uae_u8 *restore_filesys(uae_u8 *src)
{
	int type, devno;
	UnitInfo *ui;
	TCHAR *devname = 0, *volname = 0, *rootdir = 0, *filesysdir = 0;
	uae_u32 startup;
	struct uaedev_config_info *ci;

	if (restore_u32() != 2)
		goto end2;
	devno = restore_u32 ();
	ui = &mountinfo.ui[devno];
	ci = &ui->hf.ci;
	uci_set_defaults (ci, false);
	type = restore_u16 ();
	if (type == FILESYS_VIRTUAL) {
		rootdir = restore_path (SAVESTATE_PATH_VDIR);
	} else if (type == FILESYS_CD) {
		rootdir = restore_path (SAVESTATE_PATH_VDIR);
		if (cd_unit_offset == MAX_FILESYSTEM_UNITS)
			cd_unit_offset = devno;
		cd_unit_number++;
	} else if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB) {
		rootdir = restore_path (SAVESTATE_PATH_HDF);
	} else if (type == FILESYS_HARDDRIVE) {
		rootdir = restore_path (SAVESTATE_PATH_HD);
	} else {
		rootdir = restore_path (SAVESTATE_PATH);
	}
	devname = restore_string ();
	volname = restore_string ();
	filesysdir = restore_path (SAVESTATE_PATH);
	ci->bootpri = restore_u8 ();
	ci->readonly = restore_u8 () != 0;
	startup = restore_u32 ();
	filesys_configdev = restore_u32 ();

	if (new_filesys_root_path) {
		xfree(rootdir);
		rootdir = new_filesys_root_path;
		new_filesys_root_path = NULL;
	}
	if (new_filesys_fs_path) {
		xfree(filesysdir);
		filesysdir = new_filesys_fs_path;
		new_filesys_fs_path = NULL;
	}

	if (type == FILESYS_HARDFILE || type == FILESYS_HARDFILE_RDB) {
		src = restore_filesys_hardfile (ui, src);
		xfree (volname);
		volname = NULL;
	}
	_tcscpy (ci->rootdir, rootdir);
	_tcscpy (ci->devname, devname);
	_tcscpy (ci->volname, volname ? volname : _T(""));
	_tcscpy (ci->filesys, filesysdir);

	if (set_filesys_unit (devno, ci, false) < 0) {
		write_log (_T("filesys '%s' failed to restore\n"), rootdir);
		goto end;
	}
	ui->devno = devno;
	ui->startup = startup;
	if (type == FILESYS_VIRTUAL || type == FILESYS_CD)
		src = restore_filesys_virtual (ui, src, devno);

	write_log (_T("'%s' restored\n"), rootdir);
end:
	xfree (rootdir);
	xfree (devname);
	xfree (volname);
	xfree (filesysdir);
end2:
	xfree(new_filesys_root_path);
	xfree(new_filesys_fs_path);
	new_filesys_root_path = NULL;
	new_filesys_fs_path = NULL;
	return src;
}

int save_filesys_cando (void)
{
	if (nr_units () == 0)
		return -1;
	return filesys_in_interrupt ? 0 : 1;
}

static void shellexecute2_free(struct ShellExecute2 *se2)
{
	if (!se2) {
		return;
	}
	if (se2->file) {
		write_log(_T("filesys_shellexecute2_process slot %d free\n"), se2 - shellexecute2);
	}
	xfree(se2->file);
	xfree(se2->currentdir);
	xfree(se2->parms);
	xfree(se2->bin);
	xfree(se2->outbuf);
	memset(se2, 0, sizeof(struct ShellExecute2));
}

#define ShellExecute2_Struct_Start (5 * 4)
#define ShellExecute2_Struct_Start2 (9 * 4)

static uae_u32 filesys_shellexecute2_process(int mode, TrapContext *ctx)
{
	struct ShellExecute2 *se2 = NULL;
	char tmp[256];
	uae_u32 ret = 0;
	bool oldks = false;

	uae_sem_wait(&shellexec_sem);

	for (int i = 0; i < SHELLEXEC_MAX; i++) {
		struct ShellExecute2 *setmp = &shellexecute2[i];
		if (setmp->state > 0 && setmp->state <= 4) {
			se2 = setmp;
			break;
		}
	}
	if (!se2) {
		uae_u32 buf = trap_get_areg(ctx, 4);
		for (int i = 0; i < SHELLEXEC_MAX; i++) {
			struct ShellExecute2 *setmp = &shellexecute2[i];
			if (setmp->state > 4 && setmp->buffer == buf) {
				se2 = setmp;
				break;
			}
		}
	}
	if (!se2) {
		goto end;
	}

	oldks = kickstart_version < 37 && se2->currentdir[0];

	write_log(_T("filesys_shellexecute2_process. slot %d, state %d, function %d\n"), se2 - shellexecute2, se2->state, mode);

	if (mode == 30) {
		// request Amiga side buffer size
		se2->state = 2;
		if (se2->bin) {
			xfree(se2->file);
			if (oldks) {
				_sntprintf(tmp, sizeof tmp, "cd \"%s\"\nT:__uae_out_%08X_%08x", se2->currentdir, se2->process, se2->id);
			} else {
				_sntprintf(tmp, sizeof tmp, "T:__uae_bin_%08X_%08x", se2->process, se2->id);
			}
			se2->file = strdup(tmp);
		}
		int size = ShellExecute2_Struct_Start + ShellExecute2_Struct_Start2;
		size += 2 * (uaestrlen(se2->file) + 1);
		if (oldks) {
			size += 4 + uaestrlen(se2->currentdir) + 2; // CD_""\n
		}
		size += uaestrlen(se2->currentdir) + 1;
		size += 2 * (uaestrlen(se2->parms) + 1);
		size += 32; // space for tmp_out
		size += 256; // space for out buffer
		size++;
		size += se2->binsize + 4;
		if (se2->binsize) {
			size += 32; // space for bin name
		}
		ret = size;
		goto end;
	}
	if (mode == 31) {
		// a0 = buffer
		// d1 = process pointer
		se2->state = 3;
		se2->buffer = trap_get_areg(ctx, 0);
		se2->process = trap_get_dreg(ctx, 1);
		if (!se2->buffer) {
			// amiga side out of memory
			shellexecute2_free(se2);
			ret = 0;
			goto end;
		}

		uaecptr dptr = se2->buffer + ShellExecute2_Struct_Start + ShellExecute2_Struct_Start2;
		trap_put_long(ctx, se2->buffer + 4, dptr);
		if (se2->bin) {
			xfree(se2->file);
			_sntprintf(tmp, sizeof tmp, "T:__uae_bin_%08X_%08x", se2->process, se2->id);
			se2->file = strdup(tmp);
			trap_put_long(ctx, se2->buffer + 52, dptr);
		}
		dptr += trap_put_string(ctx, se2->file, dptr, -1) + 1;
		trap_put_long(ctx, se2->buffer + 8, dptr);
		dptr += trap_put_string(ctx, se2->parms, dptr, -1) + 1;
		trap_put_long(ctx, se2->buffer + 12, dptr);
		dptr += trap_put_string(ctx, se2->currentdir, dptr, -1) + 1;

		trap_put_long(ctx, se2->buffer + 16, dptr);
		if (oldks) {
			_sntprintf(tmp, sizeof tmp, "cd \"%s\"\n%s", se2->currentdir, se2->file);
		} else {
			strcpy(tmp, se2->file);
		}
		dptr += trap_put_string(ctx, tmp, dptr, -1) + 1;
		if (se2->parms[0]) {
			trap_put_byte(ctx, dptr - 1, ' ');
			dptr += trap_put_string(ctx, se2->parms, dptr, -1) + 1;
		}

		if (se2->flags & 2) {
			trap_put_long(ctx, se2->buffer + 44, dptr);
			_sntprintf(tmp, sizeof tmp, "T:__uae_out_%08X_%08x", se2->process, se2->id);
			dptr += trap_put_string(ctx, tmp, dptr, -1) + 1;
			se2->aoutbuf = dptr;
			trap_put_long(ctx, se2->buffer + 48, dptr);
			dptr += 128 + 1;
		} else {
			trap_put_long(ctx, se2->buffer + 44, 0);
		}

		dptr += 3;
		dptr &= ~3;

		uaecptr ptr = se2->buffer + ShellExecute2_Struct_Start;
		trap_put_long(ctx, ptr, se2->stack);
		ptr += 4;
		trap_put_long(ctx, ptr, se2->priority);
		ptr += 4;
		trap_put_long(ctx, ptr, se2->flags);
		ptr += 4;
		trap_put_long(ctx, ptr, se2->id);
		ptr += 4;

		if (se2->bin) {
			trap_put_long(ctx, ptr, se2->binsize);
			ptr += 4;
			trap_put_long(ctx, ptr, dptr);
			ptr += 4;
			trap_put_bytes(ctx, se2->bin, dptr, se2->binsize);
			dptr += se2->binsize;
		} else {
			trap_put_long(ctx, ptr, 0);
			ptr += 4;
			trap_put_long(ctx, ptr, 0);
			ptr += 4;
		}
		ret = 1;
		goto end;
	}
	if (mode == 32) {
		// called by new process, requests buffer
		se2->state = 4;
		uaecptr buf = se2->buffer;
		if (!(se2->flags & 3)) {
			// if no output needed, free host side buffers now
			trap_put_long(ctx, buf + 32, 0);
			shellexecute2_free(se2);
		}
		ret = buf;
		goto end;
	}


	if (mode == 34 && (se2->state == 4 || se2->state == 5)) {
		// collect output
		se2->state = 5;
		if (se2->aoutbuf) {
			int len = trap_get_dreg(ctx, 1);
			if (len > 0 && len <= 128) {
				int outoffset = se2->aoutlen;
				trap_get_bytes(ctx, tmp, se2->aoutbuf, 128);
				tmp[len] = 0;
				se2->aoutlen += len;
				if (se2->aoutlen + 1 >= se2->outlen) {
					se2->outlen += 1000;
					if (!se2->outbuf) {
						se2->outbuf = xmalloc(char, se2->outlen);
					} else {
						se2->outbuf = xrealloc(char, se2->outbuf, se2->outlen);
					}
				}
				strcpy(se2->outbuf + outoffset, tmp);
			}
		}
		ret = 1;
		goto end;
	}

	if (mode == 33 && se2->state > 0) {
		// exit status
		// d0 = status
		// a1 = buffer
		se2->exitcode = trap_get_dreg(ctx, 1);
		if (se2->cb) {
			se2->cb(se2->id, se2->exitcode, se2->flags, se2->outbuf, se2->userdata);
		}
		shellexecute2_free(se2);
		ret = 1;
	}

end:
	uae_sem_post(&shellexec_sem);


	return ret;
}

static void shellexec2_boot(struct ShellExecute2 *se2)
{
	if (kickstart_version >= 37) {
		uae_ShellExecute2(se2->id);
	} else {
		for (Unit *u = units; u; u = u->next) {
			if (is_virtual(u->unit) && filesys_isvolume(u)) {
				put_byte(u->volume + 173 - 32, get_byte(u->volume + 173 - 32) | 4);
				uae_Signal(get_long(u->volume + 176 - 32), 1 << 13);
				break;
			}
		}
	}
}

static bool filesys_shellexecute2_canrun(uae_u32 id)
{
	for (int i = 0; i < SHELLEXEC_MAX; i++) {
		struct ShellExecute2 *se2 = &shellexecute2[i];
		if (se2->state > 0 && se2->state <= 4) {
			return false;
		}
		if (se2->state > 0 && se2->id == id) {
			return false;
		}
	}
	return true;
}

void filesys_shellexecute2_run_queue(void)
{
	for (int i = 0; i < SHELLEXEC_MAX; i++) {
		struct ShellExecute2 *se2 = &shellexecute2[i];
		if (se2->state < 0) {
			if (filesys_shellexecute2_canrun(se2->id)) {
				se2->state = 1;
				shellexecute2_queued--;
				shellexec2_boot(se2);
				write_log("filesys_shellexecute2 queued executed: %d\n", shellexecute2_queued);
				return;
			}
		}
	}
}

int filesys_shellexecute2(TCHAR *file, TCHAR *currentdir, TCHAR *parms, uae_u32 stack, uae_s32 priority, uae_u32 id, uae_u32 flags, uae_u8 *bin, uae_u32 binsize, shellexecute2_callback cb, void *userdata)
{
	int newstate = 1;
	int ret = 0;

	if (uae_boot_rom_type <= 0 || id == 0) {
		return 0;
	}

	uae_sem_wait(&shellexec_sem);

	for (int i = 0; i < SHELLEXEC_MAX; i++) {
		struct ShellExecute2 *se2 = &shellexecute2[i];
		if (se2->state > 0 && se2->state <= 4) {
			newstate = -1;
		}
		if (se2->state > 0 && se2->id == id) {
			newstate = -1;
		}
	}

	for (int i = 0; i < SHELLEXEC_MAX; i++) {
		struct ShellExecute2 *se2 = &shellexecute2[i];
		if (!se2->state) {
			se2->file = file ? ua(file) : (char*)"";
			se2->currentdir = currentdir ? ua(currentdir) : (char*)"";
			se2->parms = parms ? ua(parms) : (char*)"";
			se2->id = id;
			se2->stack = stack;
			se2->priority = priority;
			se2->flags = flags;
			if (bin && binsize > 0) {
				se2->bin = xmalloc(uae_u8, binsize);
				if (!se2->bin) {
					shellexecute2_free(se2);
					goto end;
				}
				memcpy(se2->bin, bin, binsize);
				se2->binsize = binsize;
			}
			se2->cb = cb;
			se2->userdata = userdata;
			se2->state = newstate;
			if (newstate > 0) {
				write_log("filesys_shellexecute2 run\n");
				shellexec2_boot(se2);
			} else {
				shellexecute2_queued++;
				write_log("filesys_shellexecute2 queued: %d\n", shellexecute2_queued);
			}
			ret = 1;
			break;
		}
	}

end:

	uae_sem_post(&shellexec_sem);

	return ret;
}
