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
  * Does not support ACTION_INHIBIT (big deal).
  * Does not support several 2.0+ packet types.
  * Does not support removable volumes.
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
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "filesys.h"
#include "autoconf.h"
#include "traps.h"
#include "fsusage.h"
#include "native2amiga.h"
#include "scsidev.h"
#include "fsdb.h"
#include "zfile.h"
#include "gui.h"
#include "cfgfile.h"
#include "savestate.h"

#define TRACING_ENABLED 0
#if TRACING_ENABLED
#define TRACE(x)	do { write_log x; } while(0)
#define DUMPLOCK(u,x)	dumplock(u,x)
#else
#define TRACE(x)
#define DUMPLOCK(u,x)
#endif

static void aino_test (a_inode *aino)
{
#ifdef AINO_DEBUG
    a_inode *aino2 = aino, *aino3;
    for (;;) {
        if (!aino || !aino->next)
	    return;
	if ((aino->checksum1 ^ aino->checksum2) != 0xaaaa5555) {
	    write_log ("PANIC: corrupted or freed but used aino detected!", aino);
	}
	aino3 = aino;
	aino = aino->next;
	if (aino->prev != aino3) {
	    write_log ("PANIC: corrupted aino linking!\n");
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

uaecptr filesys_initcode;
static uae_u32 fsdevname, filesys_configdev;

#define FS_STARTUP 0
#define FS_GO_DOWN 1

typedef struct {
    char *devname; /* device name, e.g. UAE0: */
    uaecptr devname_amiga;
    uaecptr startup;
    char *volname; /* volume name, e.g. CDROM, WORK, etc. */
    int volflags; /* volume flags, readonly, stream uaefsdb support */
    char *rootdir; /* root unix directory */
    int readonly; /* disallow write access? */
    int bootpri; /* boot priority */
    int devno;
    int automounted; /* don't save to config if set */
    
    struct hardfiledata hf;

    /* Threading stuff */
    smp_comm_pipe *volatile unit_pipe, *volatile back_pipe;
    uae_thread_id tid;
    struct _unit *self;
    /* Reset handling */
    volatile uae_sem_t reset_sync_sem;
    volatile int reset_state;
} UnitInfo;

struct uaedev_mount_info {
    int num_units;
    UnitInfo ui[MAX_FILESYSTEM_UNITS];
};

static struct uaedev_mount_info current_mountinfo;
struct uaedev_mount_info options_mountinfo;

int nr_units (struct uaedev_mount_info *mountinfo)
{
    return mountinfo->num_units;
}

int is_hardfile (struct uaedev_mount_info *mountinfo, int unit_no)
{
    if (!mountinfo)
	    mountinfo = &current_mountinfo;
    if (mountinfo->ui[unit_no].volname)
	    return FILESYS_VIRTUAL;
    if (mountinfo->ui[unit_no].hf.secspertrack == 0) {
	      return FILESYS_HARDDRIVE;
    }
    return FILESYS_HARDFILE;
}

static void close_filesys_unit (UnitInfo *uip)
{
    if (uip->hf.fd != 0)
	fclose (uip->hf.fd);
    if (uip->volname != 0)
	xfree (uip->volname);
    if (uip->devname != 0)
	xfree (uip->devname);
    if (uip->rootdir != 0)
	xfree (uip->rootdir);
    if (uip->unit_pipe)
	xfree (uip->unit_pipe);
    if (uip->back_pipe)
	xfree (uip->back_pipe);

    uip->unit_pipe = 0;
    uip->back_pipe = 0;

    uip->hf.fd = 0;
    uip->volname = 0;
    uip->devname = 0;
    uip->rootdir = 0;
}

char *get_filesys_unit (struct uaedev_mount_info *mountinfo, int nr,
			char **devname, char **volname, char **rootdir, int *readonly,
			int *secspertrack, int *surfaces, int *reserved,
			int *cylinders, uae_u64 *size, int *blocksize, int *bootpri, char **filesysdir, int *flags)
{
    UnitInfo *uip = mountinfo->ui + nr;

    if (nr >= mountinfo->num_units)
	return "No slot allocated for this unit";
    
    *volname = uip->volname ? my_strdup (uip->volname) : 0;
    if (uip->devname == 0 || strlen(uip->devname) == 0) {
	    *devname = (char *)xmalloc (10);
	    sprintf (*devname, "DH%d", nr);
    } else {
	    *devname = my_strdup (uip->devname);
    }
    *rootdir = uip->rootdir ? my_strdup (uip->rootdir) : 0;
    *readonly = uip->readonly;
    *secspertrack = uip->hf.secspertrack;
    *surfaces = uip->hf.surfaces;
    *reserved = uip->hf.reservedblocks;
    *cylinders = uip->hf.nrcyls;
    *blocksize = uip->hf.blocksize;
    *size = uip->hf.size;
    *bootpri = uip->bootpri;
    if (flags)
	*flags = uip->automounted ? FILESYS_FLAG_DONOTSAVE : 0;
    return 0;
}

static void stripsemicolon(char *s)
{
    if (!s)
	return;
    while(strlen(s) > 0 && s[strlen(s) - 1] == ':')
	s[strlen(s) - 1] = 0;
}

static char *set_filesys_unit_1 (struct uaedev_mount_info *mountinfo, int nr,
				 char *devname, char *volname, char *rootdir, int readonly,
				 int secspertrack, int surfaces, int reserved,
				 int blocksize, int bootpri, char *filesysdir, int flags)
{
    UnitInfo *ui = mountinfo->ui + nr;
    static char errmsg[1024];
    int i;

    if (nr >= mountinfo->num_units)
	return "No slot allocated for this unit";

    for (i = 0; i < mountinfo->num_units; i++) {
        if (nr == i)
	    continue;
	if (!strcmpi (mountinfo->ui[i].rootdir, rootdir)) {
	    sprintf (errmsg, "directory/hardfile '%s' already added", rootdir);
	    return errmsg;
	}
    }

    ui->devname = 0;
    ui->volname = 0;
    ui->rootdir = 0;
    ui->unit_pipe = 0;
    ui->back_pipe = 0;
    ui->hf.fd = 0;
    ui->bootpri = 0;
    ui->automounted = flags & FILESYS_FLAG_DONOTSAVE;

    if (volname != 0) {
	ui->volname = my_strdup (volname);
	stripsemicolon(ui->volname);
    } else {
  ui->hf.secspertrack = secspertrack;
  ui->hf.surfaces = surfaces;
  ui->hf.reservedblocks = reserved;
  ui->hf.blocksize = blocksize;
	ui->volname = 0;
	ui->hf.readonly = readonly;
	ui->hf.fd = fopen (rootdir, "r+b");
	if (ui->hf.fd == 0 && !readonly) {
	    ui->hf.readonly = readonly = 1;
	    ui->hf.fd = fopen (rootdir, "rb");
	}
	if (ui->hf.fd == 0)
	    return "Hardfile not found";
  fseek(ui->hf.fd, 0, SEEK_END);
  ui->hf.size = ftell(ui->hf.fd);
  ui->hf.size &= ~(ui->hf.blocksize - 1);
  fseek (ui->hf.fd, 0, SEEK_SET);
	if ((ui->hf.blocksize & (ui->hf.blocksize - 1)) != 0 || ui->hf.blocksize == 0)
	    return "Bad blocksize";
	if ((ui->hf.secspertrack || ui->hf.surfaces || ui->hf.reservedblocks) &&
	    (ui->hf.secspertrack < 1 || ui->hf.surfaces < 1 || ui->hf.surfaces > 1023 ||
	    ui->hf.reservedblocks < 0 || ui->hf.reservedblocks > 1023) != 0)
	    return "Bad hardfile geometry";
	if (ui->hf.blocksize > ui->hf.size || ui->hf.size == 0)
	    return "Hardfile too small";
	ui->hf.nrcyls = (int)(ui->hf.secspertrack * ui->hf.surfaces ? (ui->hf.size / ui->hf.blocksize) / (ui->hf.secspertrack * ui->hf.surfaces) : 0);
    }
    ui->self = 0;
    ui->reset_state = FS_STARTUP;
    ui->rootdir = my_strdup (rootdir);
    if(devname != 0) {
      ui->devname = my_strdup (devname);
      stripsemicolon(ui->devname);
    }
    ui->readonly = readonly;
    if (bootpri < -128) bootpri = -128;
    if (bootpri > 127) bootpri = 127;
    ui->bootpri = bootpri;
    
    return 0;
}

char *set_filesys_unit (struct uaedev_mount_info *mountinfo, int nr,
			char *devname, char *volname, char *rootdir, int readonly,
			int secspertrack, int surfaces, int reserved,
			int blocksize, int bootpri, char *filesysdir, int flags)
{
    char *result;
    UnitInfo ui = mountinfo->ui[nr];
    fclose (ui.hf.fd);
    result = set_filesys_unit_1 (mountinfo, nr, devname, volname, rootdir, readonly,
				       secspertrack, surfaces, reserved, blocksize, bootpri, filesysdir, flags);
    if (result)
	mountinfo->ui[nr] = ui;
    else
	close_filesys_unit (&ui);

    return result;
}

char *add_filesys_unit (struct uaedev_mount_info *mountinfo,
			char *devname, char *volname, char *rootdir, int readonly,
			int secspertrack, int surfaces, int reserved,
			int blocksize, int bootpri, char *filesysdir, int flags)
{
    char *retval;
    int nr = mountinfo->num_units;
    UnitInfo *uip = mountinfo->ui + nr;

    if (nr >= MAX_FILESYSTEM_UNITS)
	return "Maximum number of file systems mounted";

    mountinfo->num_units++;
    retval = set_filesys_unit_1 (mountinfo, nr, devname, volname, rootdir, readonly,
				 secspertrack, surfaces, reserved, blocksize, bootpri, filesysdir, flags);
    if (retval)
	mountinfo->num_units--;
    return retval;
}

int kill_filesys_unit (struct uaedev_mount_info *mountinfo, int nr)
{
    UnitInfo *uip = mountinfo->ui;
    if (nr >= mountinfo->num_units || nr < 0)
	return -1;

    close_filesys_unit (mountinfo->ui + nr);

    mountinfo->num_units--;
    for (; nr < mountinfo->num_units; nr++) {
	uip[nr] = uip[nr+1];
    }
    return 0;
}

int move_filesys_unit (struct uaedev_mount_info *mountinfo, int nr, int to)
{
    UnitInfo tmpui;
    UnitInfo *uip = mountinfo->ui;

    if (nr >= mountinfo->num_units || nr < 0
	|| to >= mountinfo->num_units || to < 0
	|| to == nr)
	return -1;
    tmpui = uip[nr];
    if (to > nr) {
	int i;
	for (i = nr; i < to; i++)
	    uip[i] = uip[i + 1];
    } else {
	int i;
	for (i = nr; i > to; i--)
	    uip[i] = uip[i - 1];
    }
    uip[to] = tmpui;
    return 0;
}

int sprintf_filesys_unit (struct uaedev_mount_info *mountinfo, char *buffer, int num)
{
    UnitInfo *uip = mountinfo->ui;
    if (num >= mountinfo->num_units)
	return -1;

    if (uip[num].volname != 0)
	sprintf (buffer, "(DH%d:) Filesystem, %s: %s %s", num, uip[num].volname,
		 uip[num].rootdir, uip[num].readonly ? "ro" : "");
    else
	sprintf (buffer, "(DH%d:) Hardfile, \"%s\", size %d Mbytes", num,
	uip[num].rootdir, uip[num].hf.size / (1024 * 1024));
    return 0;
}

void write_filesys_config (struct uae_prefs *p, struct uaedev_mount_info *mountinfo,
			   const char *unexpanded, const char *default_path, struct zfile *f)
{
    UnitInfo *uip = mountinfo->ui;
    int i;
    char tmp[MAX_DPATH];

    for (i = 0; i < mountinfo->num_units; i++) {
	int dosave = 1;
	char *str;
#ifdef _WIN32
	if (p->win32_automount_drives && uip[i].automounted)
	    dosave = 0;
#endif
	if (!dosave)
	    continue;
	str = cfgfile_subst_path (default_path, unexpanded, uip[i].rootdir);
	if (uip[i].volname != 0) {
	    sprintf (tmp, "filesystem2=%s,%s:%s:%s,%d\n", uip[i].readonly ? "ro" : "rw",
		     uip[i].devname ? uip[i].devname : "", uip[i].volname, str, uip[i].bootpri);
	    zfile_fputs (f, tmp);
	    sprintf (tmp, "filesystem=%s,%s:%s\n", uip[i].readonly ? "ro" : "rw",
		     uip[i].volname, str);
	    zfile_fputs (f, tmp);
	} else {
	    sprintf (tmp, "hardfile2=%s,%s:%s,%d,%d,%d,%d,%d,%s\n",
		     uip[i].readonly ? "ro" : "rw",
		     uip[i].devname ? uip[i].devname : "", str,
		     uip[i].hf.secspertrack, uip[i].hf.surfaces, uip[i].hf.reservedblocks, uip[i].hf.blocksize,
		     uip[i].bootpri,/*uip[i].filesysdir ? uip[i].filesysdir :*/ "");
	    zfile_fputs (f, tmp);
	    sprintf (tmp, "hardfile=%s,%d,%d,%d,%d,%s\n",
		     uip[i].readonly ? "ro" : "rw", uip[i].hf.secspertrack,
		     uip[i].hf.surfaces, uip[i].hf.reservedblocks, uip[i].hf.blocksize, str);
	    zfile_fputs (f, tmp);
	}
	xfree (str);
    }
}

static void dup_mountinfo (struct uaedev_mount_info *mip, struct uaedev_mount_info *mip_d)
{
    int i;
    struct uaedev_mount_info *i2 = mip_d;

    memcpy (i2, mip, sizeof *i2);

    for (i = 0; i < i2->num_units; i++) {
	UnitInfo *uip = i2->ui + i;
	if (uip->volname)
	    uip->volname = my_strdup (uip->volname);
	if (uip->devname)
	    uip->devname = my_strdup (uip->devname);
	if (uip->rootdir)
	    uip->rootdir = my_strdup (uip->rootdir);
	if (uip->hf.fd)
	    uip->hf.fd = fdopen ( dup (fileno (uip->hf.fd)), uip->readonly ? "rb" : "r+b");
    }
}

void free_mountinfo (struct uaedev_mount_info *mip)
{
    int i;
    if (!mip)
	return;
    for (i = 0; i < mip->num_units; i++)
	close_filesys_unit (mip->ui + i);
    mip->num_units = 0;
}

struct hardfiledata *get_hardfile_data (int nr)
{
    UnitInfo *uip = current_mountinfo.ui;
    if (nr < 0 || nr >= current_mountinfo.num_units || uip[nr].volname != 0)
	return 0;
    return &uip[nr].hf;
}

/* minimal AmigaDOS definitions */

/* field offsets in DosPacket */
#define dp_Type 8
#define dp_Res1	12
#define dp_Res2 16
#define dp_Arg1 20
#define dp_Arg2 24
#define dp_Arg3 28
#define dp_Arg4 32

/* result codes */
#define DOS_TRUE ((unsigned long)-1L)
#define DOS_FALSE (0L)

/* Passed as type to Lock() */
#define SHARED_LOCK         -2     /* File is readable by others */
#define ACCESS_READ         -2     /* Synonym */
#define EXCLUSIVE_LOCK      -1     /* No other access allowed    */
#define ACCESS_WRITE        -1     /* Synonym */

/* packet types */
#define ACTION_CURRENT_VOLUME	7
#define ACTION_LOCATE_OBJECT	8
#define ACTION_RENAME_DISK	9
#define ACTION_FREE_LOCK	15
#define ACTION_DELETE_OBJECT	16
#define ACTION_RENAME_OBJECT	17
#define ACTION_COPY_DIR		19
#define ACTION_SET_PROTECT	21
#define ACTION_CREATE_DIR	22
#define ACTION_EXAMINE_OBJECT	23
#define ACTION_EXAMINE_NEXT	24
#define ACTION_DISK_INFO	25
#define ACTION_INFO		26
#define ACTION_FLUSH		27
#define ACTION_SET_COMMENT	28
#define ACTION_PARENT		29
#define ACTION_SET_DATE		34
#define ACTION_FIND_WRITE	1004
#define ACTION_FIND_INPUT	1005
#define ACTION_FIND_OUTPUT	1006
#define ACTION_END		1007
#define ACTION_SEEK		1008
#define ACTION_IS_FILESYSTEM	1027
#define ACTION_READ		'R'
#define ACTION_WRITE		'W'

/* 2.0+ packet types */
#define ACTION_INHIBIT       31
#define ACTION_SET_FILE_SIZE 1022
#define ACTION_LOCK_RECORD   2008
#define ACTION_FREE_RECORD   2009
#define ACTION_SAME_LOCK     40
#define ACTION_CHANGE_MODE   1028
#define ACTION_FH_FROM_LOCK  1026
#define ACTION_COPY_DIR_FH   1030
#define ACTION_PARENT_FH     1031
#define ACTION_EXAMINE_FH    1034
#define ACTION_EXAMINE_ALL   1033
#define ACTION_MAKE_LINK     1021
#define ACTION_READ_LINK     1024
#define ACTION_FORMAT        1020
#define ACTION_IS_FILESYSTEM 1027
#define ACTION_ADD_NOTIFY    4097
#define ACTION_REMOVE_NOTIFY 4098

#define DISK_TYPE		0x444f5301 /* DOS\1 */

typedef struct {
    uae_u32 uniq;
    /* The directory we're going through.  */
    a_inode *aino;
    /* The file we're going to look up next.  */
    a_inode *curr_file;
} ExamineKey;

typedef struct key {
    struct key *next;
    a_inode *aino;
    uae_u32 uniq;
    void * fd;
    off_t file_pos;
    int dosmode;
    int createmode;
    int notifyactive;
} Key;

typedef struct notify {
    struct notify *next;
    uaecptr notifyrequest;
    char *fullname;
    char *partname;
} Notify;

/* Since ACTION_EXAMINE_NEXT is so braindamaged, we have to keep
 * some of these around
 */

#define EXKEYS 100
#define MAX_AINO_HASH 128
#define NOTIFY_HASH_SIZE 127

/* handler state info */

typedef struct _unit {
    struct _unit *next;

    /* Amiga stuff */
    uaecptr dosbase;
    uaecptr volume;
    uaecptr port;	/* Our port */
    uaecptr locklist;

    /* Native stuff */
    uae_s32 unit;	/* unit number */
    UnitInfo ui;	/* unit startup info */
    char tmpbuf3[256];

    /* Dummy message processing */
    uaecptr dummy_message;
    volatile unsigned int cmds_sent;
    volatile unsigned int cmds_complete;
    volatile unsigned int cmds_acked;

    /* ExKeys */
    ExamineKey examine_keys[EXKEYS];
    int next_exkey;
    unsigned long total_locked_ainos;

    /* Keys */
    struct key *keys;
    uae_u32 key_uniq;
    uae_u32 a_uniq;

    a_inode rootnode;
    unsigned long aino_cache_size;
    a_inode *aino_hash[MAX_AINO_HASH];
    unsigned long nr_cache_hits;
    unsigned long nr_cache_lookups;

    struct notify *notifyhash[NOTIFY_HASH_SIZE];

    int volflags;
} Unit;

typedef uae_u8 *dpacket;
#define PUT_PCK_RES1(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp_Res1), (v)); } while (0)
#define PUT_PCK_RES2(p,v) do { do_put_mem_long ((uae_u32 *)((p) + dp_Res2), (v)); } while (0)
#define GET_PCK_TYPE(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Type))))
#define GET_PCK_RES1(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Res1))))
#define GET_PCK_RES2(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Res2))))
#define GET_PCK_ARG1(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg1))))
#define GET_PCK_ARG2(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg2))))
#define GET_PCK_ARG3(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg3))))
#define GET_PCK_ARG4(p) ((uae_s32)(do_get_mem_long ((uae_u32 *)((p) + dp_Arg4))))

static char *char1 (uaecptr addr)
{
    static char buf[1024];
    unsigned int i = 0;
    do {
	buf[i] = get_byte(addr);
	addr++;
    } while (buf[i++] && i < sizeof(buf));
    return buf;
}

static char *bstr1 (uaecptr addr)
{
    static char buf[256];
    int i;
    int n = get_byte(addr);
    addr++;

    for (i = 0; i < n; i++, addr++)
	buf[i] = get_byte(addr);
    buf[i] = 0;
    return buf;
}

static char *bstr (Unit *unit, uaecptr addr)
{
    int i;
    int n = get_byte(addr);

    addr++;
    for (i = 0; i < n; i++, addr++)
	unit->tmpbuf3[i] = get_byte(addr);
    unit->tmpbuf3[i] = 0;
    return unit->tmpbuf3;
}

static char *bstr_cut (Unit *unit, uaecptr addr)
{
    char *p = unit->tmpbuf3;
    int i, colon_seen = 0;
    int n = get_byte (addr);

    addr++;
    for (i = 0; i < n; i++, addr++) {
	uae_u8 c = get_byte(addr);
	unit->tmpbuf3[i] = c;
	if (c == '/' || (c == ':' && colon_seen++ == 0))
	    p = unit->tmpbuf3 + i + 1;
    }
    unit->tmpbuf3[i] = 0;
    return p;
}

static Unit *units = 0;
static int unit_num = 0;

static Unit*
find_unit (uaecptr port)
{
    Unit* u;
    for (u = units; u; u = u->next)
	if (u->port == port)
	    break;

    return u;
}
    
/* flags and comments supported? */
static int fsdb_cando (Unit *unit)
{
//    if (currprefs.filesys_custom_uaefsdb  && (unit->volflags & MYVOLUMEINFO_STREAMS))
//	return 1;
//    if (!currprefs.filesys_no_uaefsdb)
//	return 1;
    return 0;
}

static void prepare_for_open (char *name)
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

static void dispose_aino (Unit *unit, a_inode **aip, a_inode *aino)
{
    int hash = aino->uniq % MAX_AINO_HASH;
    if (unit->aino_hash[hash] == aino)
	unit->aino_hash[hash] = 0;

    if (aino->dirty && aino->parent)
	fsdb_dir_writeback (aino->parent);

    *aip = aino->sibling;
    xfree (aino->aname);
    if (aino->comment)
	xfree (aino->comment);
    xfree (aino->nname);
    xfree (aino);
}

static void recycle_aino (Unit *unit, a_inode *new_aino)
{
    aino_test (new_aino);
    if (new_aino->dir || new_aino->shlock > 0
	|| new_aino->elock || new_aino == &unit->rootnode)
	/* Still in use */
	return;

    TRACE (("Recycling; cache size %d, total_locked %d\n",
	    unit->aino_cache_size, unit->total_locked_ainos));
    if (unit->aino_cache_size > 5000 + unit->total_locked_ainos) {
	/* Reap a few. */
	int i = 0;
	while (i < 50) {
	    a_inode *parent = unit->rootnode.prev->parent;
	    a_inode **aip, *old_prev;
	    aip = &parent->child;

	    aino_test (parent);
	    if (! parent->locked_children) {
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
			        write_log ("panic: freeing locked a_inode!\n");

			    de_recycle_aino (unit, aino);
			    dispose_aino (unit, aip, aino);
			    i++;
		    }
    }
		  }
	    /* In the previous loop, we went through all children of one
	       parent.  Re-arrange the recycled list so that we'll find a
	       different parent the next time around.  */
	    old_prev = unit->rootnode.prev;
	    do {
		unit->rootnode.next->prev = unit->rootnode.prev;
		unit->rootnode.prev->next = unit->rootnode.next;
		unit->rootnode.next = unit->rootnode.prev;
		unit->rootnode.prev = unit->rootnode.prev->prev;
		unit->rootnode.prev->next = unit->rootnode.next->prev = &unit->rootnode;
	    } while (unit->rootnode.prev != old_prev
		     && unit->rootnode.prev->parent == parent);
	}
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

static void update_child_names (Unit *unit, a_inode *a, a_inode *parent)
{
    int l0 = strlen (parent->nname) + 2;

    while (a != 0) {
	char *name_start;
	char *new_name;
	char dirsep[2] = { FSDB_DIR_SEPARATOR, '\0' };
	  
	a->parent = parent;
	name_start = strrchr (a->nname, FSDB_DIR_SEPARATOR);
	if (name_start == 0) {
	    write_log ("malformed file name");
	}
	name_start++;
	new_name = (char *)xmalloc (strlen (name_start) + l0);
	strcpy (new_name, parent->nname);
	strcat (new_name, dirsep);
	strcat (new_name, name_start);
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

    TRACE(("deleting aino %x\n", aino->uniq));

    aino_test (aino);
    aino->dirty = 1;
    aino->deleted = 1;
    de_recycle_aino (unit, aino);

    /* If any ExKeys are currently pointing at us, advance them.  */
    if (aino->parent->exnext_count > 0) {
	int i;
	TRACE(("entering exkey validation\n"));
	for (i = 0; i < EXKEYS; i++) {
	    ExamineKey *k = unit->examine_keys + i;
	    if (k->uniq == 0)
		continue;
	    if (k->aino == aino->parent) {
		TRACE(("Same parent found for %d\n", i));
		if (k->curr_file == aino) {
		    k->curr_file = aino->sibling;
		    TRACE(("Advancing curr_file\n"));
		}
	    }
	}
    }

    aip = &aino->parent->child;
    while (*aip != aino && *aip != 0)
	aip = &(*aip)->sibling;
    if (*aip != aino) {
	write_log ("Couldn't delete aino.\n");
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

char *build_nname (const char *d, const char *n)
{
    char dsep[2] = { FSDB_DIR_SEPARATOR, '\0' };
    char *p = (char *) xmalloc (strlen (d) + strlen (n) + 2);
    strcpy (p, d);
    strcat (p, dsep);
    strcat (p, n);
    return p;
}

char *build_aname (const char *d, const char *n)
{
    char *p = (char *) xmalloc (strlen (d) + strlen (n) + 2);
    strcpy (p, d);
    strcat (p, "/");
    strcat (p, n);
    return p;
}

/* This gets called to translate an Amiga name that some program used to
 * a name that we can use on the native filesystem.  */
static char *get_nname (Unit *unit, a_inode *base, char *rel,
			char **modified_rel)
{
    char *found;
    char *p = 0;

    aino_test (base);

    *modified_rel = 0;
    
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
    if (fsdb_name_invalid (rel))
	return 0;

    /* See if we have a file that has the same name as the aname we are
     * looking for.  */
    found = fsdb_search_dir (base->nname, rel);
    if (found == 0)
	return found;
    if (found == rel)
	return build_nname (base->nname, rel);

    *modified_rel = found;
    return build_nname (base->nname, found);
}

static char *create_nname (Unit *unit, a_inode *base, char *rel)
{
    char *p;

    aino_test (base);
    /* We are trying to create a file called REL.  */
    
    /* If the name is used otherwise in the directory (or globally), we
     * need a new unique nname.  */
    if (fsdb_name_invalid (rel) || fsdb_used_as_nname (base, rel)) {
#if 0
	oh_dear:
#endif
	p = fsdb_create_unique_nname (base, rel);
	return p;
    }
    p = build_nname (base->nname, rel);

#if 0
    /* Delete this code once we know everything works.  */
    if (access (p, R_OK) >= 0 || errno != ENOENT) {
	write_log ("Filesystem in trouble... please report.\n");
	xfree (p);
	goto oh_dear;
    }
#endif
    return p;
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
static char *get_aname (Unit *unit, a_inode *base, char *rel)
{
    return my_strdup (rel);
}

static void init_child_aino (Unit *unit, a_inode *base, a_inode *aino)
{
    aino->uniq = ++unit->a_uniq;
    if (unit->a_uniq == 0xFFFFFFFF) {
	write_log ("Running out of a_inodes (prepare for big trouble)!\n");
    }
    aino->shlock = 0;
    aino->elock = 0;

    aino->dirty = 0;
    aino->deleted = 0;

    /* For directories - this one isn't being ExNext()ed yet.  */
    aino->locked_children = 0;
    aino->exnext_count = 0;
    /* But the parent might be.  */
    if (base->exnext_count) {
	unit->total_locked_ainos++;
	base->locked_children++;
    }
    /* Update tree structure */
    aino->parent = base;
    aino->child = 0;
    aino->sibling = base->child;
    base->child = aino;
    aino->next = aino->prev = 0;
    aino->volflags = unit->volflags;

    aino_test_init (aino);
    aino_test (aino);
}

static a_inode *new_child_aino (Unit *unit, a_inode *base, char *rel)
{
    char *modified_rel;
    char *nn;
    a_inode *aino;

    TRACE(("new_child_aino %s, %s\n", base->aname, rel));

    aino = fsdb_lookup_aino_aname (base, rel);
    if (aino == 0) {
	nn = get_nname (unit, base, rel, &modified_rel);
	if (nn == 0)
	    return 0;

	aino = (a_inode *) xcalloc (sizeof (a_inode), 1);
	if (aino == 0)
	    return 0;
	aino->aname = modified_rel ? modified_rel : my_strdup (rel);
	aino->nname = nn;

	aino->comment = 0;
	aino->has_dbentry = 0;

	if (!fsdb_fill_file_attrs (base, aino)) {
	    xfree (aino);
	    return 0;
	}
	if (aino->dir)
	    fsdb_clean_dir (aino);
    }
    init_child_aino (unit, base, aino);

    recycle_aino (unit, aino);
    TRACE(("created aino %x, lookup, amigaos_mode %d\n", aino->uniq, aino->amigaos_mode));
    return aino;
}

static a_inode *create_child_aino (Unit *unit, a_inode *base, char *rel, int isdir)
{
    a_inode *aino = (a_inode *) xcalloc (sizeof (a_inode), 1);
    if (aino == 0)
	return 0;

    aino->nname = create_nname (unit, base, rel);
    if (!aino->nname) {
    	free (aino);
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
    TRACE(("created aino %x, create\n", aino->uniq));
    return aino;
}

static a_inode *lookup_child_aino (Unit *unit, a_inode *base, char *rel, uae_u32 *err)
{
   a_inode *c = base->child;
   int l0 = strlen (rel);

   aino_test (base);
   aino_test (c);
   
   if (base->dir == 0) {
      *err = ERROR_OBJECT_WRONG_TYPE;
      return 0;
   }
   
   while (c != 0) {
      int l1 = strlen (c->aname);
      if (l0 <= l1 && same_aname (rel, c->aname + l1 - l0)
      && (l0 == l1 || c->aname[l1-l0-1] == '/'))
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
static a_inode *lookup_child_aino_for_exnext (Unit *unit, a_inode *base, char *rel, uae_u32 *err)
{
    a_inode *c = base->child;
    int l0 = strlen (rel);

    aino_test (base);
    aino_test (c);

    *err = 0;
    while (c != 0) {
	int l1 = strlen (c->nname);
	/* Note: using strcmp here.  */
	if (l0 <= l1 && strcmp (rel, c->nname + l1 - l0) == 0
	    && (l0 == l1 || c->nname[l1-l0-1] == FSDB_DIR_SEPARATOR))
	    break;
	c = c->sibling;
    }
    if (c != 0)
	return c;
    c = fsdb_lookup_aino_nname (base, rel);
    if (c == 0) {
	c = (a_inode *)xcalloc (sizeof (a_inode), 1);
	if (c == 0) {
	    *err = ERROR_NO_FREE_STORE;
	    return 0;
	}

	c->nname = build_nname (base->nname, rel);
	c->aname = get_aname (unit, base, rel);
	c->comment = 0;
	c->has_dbentry = 0;
	if (!fsdb_fill_file_attrs (base, c)) {
	    xfree (c);
	    *err = ERROR_NO_FREE_STORE;
	    return 0;
	}
	if (c->dir)
	    fsdb_clean_dir (c);
    }
    init_child_aino (unit, base, c);

    recycle_aino (unit, c);
    TRACE(("created aino %x, exnext\n", c->uniq));

    return c;
}

static a_inode *get_aino (Unit *unit, a_inode *base, const char *rel, uae_u32 *err)
{
   char *tmp;
   char *p;
   a_inode *curr;
   int i;

   aino_test (base);
   
   *err = 0;
   TRACE(("get_path(%s,%s)\n", base->aname, rel));
   
   /* root-relative path? */
   for (i = 0; rel[i] && rel[i] != '/' && rel[i] != ':'; i++)
      ;
   if (':' == rel[i])
      rel += i+1;
   
   tmp = my_strdup (rel);
   p = tmp;
   curr = base;
   
   while (*p) {
      /* start with a slash? go up a level. */
      if (*p == '/') {
         if (curr->parent != 0)
            curr = curr->parent;
         p++;
      } else {
         a_inode *next;
         
         char *component_end;
         component_end = strchr (p, '/');
         if (component_end != 0)
            *component_end = '\0';
         next = lookup_child_aino (unit, curr, p, err);
         if (next == 0) {
            /* if only last component not found, return parent dir. */
            if (*err != ERROR_OBJECT_NOT_AROUND || component_end != 0)
               curr = 0;
            /* ? what error is appropriate? */
            break;
         }
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

static void startup_update_unit (Unit *unit, UnitInfo *uinfo)
{
    if (!unit)
	return;
    unit->ui.devname = uinfo->devname;
    xfree (unit->ui.volname);
    unit->ui.volname = my_strdup (uinfo->volname); /* might free later for rename */
    unit->ui.rootdir = uinfo->rootdir;
    unit->ui.readonly = uinfo->readonly;
    unit->ui.unit_pipe = uinfo->unit_pipe;
    unit->ui.back_pipe = uinfo->back_pipe;
}

static Unit *startup_create_unit (UnitInfo *uinfo)
{
    int i;
    Unit *unit;

    unit = (Unit *) xcalloc (sizeof (Unit), 1);
    unit->next = units;
    units = unit;
    uinfo->self = unit;

    unit->volume = 0;
    unit->port = m68k_areg (&regs, 5);
    unit->unit = unit_num++;

    startup_update_unit (unit, uinfo);

    unit->cmds_complete = 0;
    unit->cmds_sent = 0;
    unit->cmds_acked = 0;
    for (i = 0; i < EXKEYS; i++) {
	unit->examine_keys[i].aino = 0;
	unit->examine_keys[i].curr_file = 0;
	unit->examine_keys[i].uniq = 0;
    }
    unit->total_locked_ainos = 0;
    unit->next_exkey = 1;
    unit->keys = 0;
    unit->a_uniq = unit->key_uniq = 0;

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

static uae_u32 REGPARAM2 startup_handler (TrapContext *context)
{
    /* Just got the startup packet. It's in A4. DosBase is in A2,
     * our allocated volume structure is in D6, A5 is a pointer to
     * our port. */
    uaecptr rootnode = get_long (m68k_areg (&context->regs, 2) + 34);
    uaecptr dos_info = get_long (rootnode + 24) << 2;
    uaecptr pkt = m68k_dreg (&context->regs, 3);
    uaecptr arg2 = get_long (pkt + dp_Arg2);
    int i, namelen;
    char* devname = bstr1 (get_long (pkt + dp_Arg1) << 2);
    char* s;
    Unit *unit;
    UnitInfo *uinfo;

    /* find UnitInfo with correct device name */
    s = strchr (devname, ':');
    if (s)
	*s = '\0';

    for (i = 0; i < current_mountinfo.num_units; i++) {
	/* Hardfile volume name? */
	if (current_mountinfo.ui[i].volname == 0)
	    continue;

	if (current_mountinfo.ui[i].startup == arg2)
	    break;
    }

    if (i == current_mountinfo.num_units
	|| !my_existsdir (current_mountinfo.ui[i].rootdir))
    {
	write_log ("Failed attempt to mount device '%s'\n", devname);
	put_long (pkt + dp_Res1, DOS_FALSE);
	put_long (pkt + dp_Res2, ERROR_DEVICE_NOT_MOUNTED);
	return 1;
    }
    uinfo = current_mountinfo.ui + i;
    unit = startup_create_unit (uinfo);
    unit->volflags = uinfo->volflags;

/*    write_comm_pipe_int (unit->ui.unit_pipe, -1, 1);*/

    write_log ("FS: %s (flags=%08.8X) starting..\n", unit->ui.volname, unit->volflags);

    /* fill in our process in the device node */
    put_long ((get_long (pkt + dp_Arg3) << 2) + 8, unit->port);
    unit->dosbase = m68k_areg (&context->regs, 2);

    /* make new volume */
    unit->volume = m68k_areg (&context->regs, 3) + 32;
#ifdef UAE_FILESYS_THREADS
    unit->locklist = m68k_areg (&context->regs, 3) + 8;
#else
    unit->locklist = m68k_areg (&context->regs, 3);
#endif
    unit->dummy_message = m68k_areg (&context->regs, 3) + 12;

    put_long (unit->dummy_message + 10, 0);

    put_long (unit->volume + 4, 2); /* Type = dt_volume */
    put_long (unit->volume + 12, 0); /* Lock */
    put_long (unit->volume + 16, 3800); /* Creation Date */
    put_long (unit->volume + 20, 0);
    put_long (unit->volume + 24, 0);
    put_long (unit->volume + 28, 0); /* lock list */
    put_long (unit->volume + 40, (unit->volume + 44) >> 2); /* Name */
    namelen = strlen (unit->ui.volname);
    put_byte (unit->volume + 44, namelen);
    for (i = 0; i < namelen; i++)
	put_byte (unit->volume + 45 + i, unit->ui.volname[i]);

    /* link into DOS list */
    put_long (unit->volume, get_long (dos_info + 4));
    put_long (dos_info + 4, unit->volume >> 2);

    put_long (unit->volume + 8, unit->port);
    put_long (unit->volume + 32, DISK_TYPE);

    put_long (pkt + dp_Res1, DOS_TRUE);

    fsdb_clean_dir (&unit->rootnode);

    return 0;
}

static void
do_info (Unit *unit, dpacket packet, uaecptr info)
{
    struct fs_usage fsu;
    
    if (get_fs_usage (unit->ui.rootdir, 0, &fsu) != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, dos_errno ());
	return;
    }

    put_long (info, 0); /* errors */
    put_long (info + 4, unit->unit); /* unit number */
    put_long (info + 8, unit->ui.readonly ? 80 : 82); /* state  */
    put_long (info + 12, fsu.fsu_blocks ); /* numblocks */
    put_long (info + 16, fsu.fsu_blocks - fsu.fsu_bavail); /* inuse */
    put_long (info + 20, 1024); /* bytesperblock */
    put_long (info + 24, DISK_TYPE); /* disk type */
    put_long (info + 28, unit->volume >> 2); /* volume node */
    put_long (info + 32, 0); /* inuse */
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
action_disk_info (Unit *unit, dpacket packet)
{
    TRACE(("ACTION_DISK_INFO\n"));
    do_info(unit, packet, GET_PCK_ARG1 (packet) << 2);
}

static void
action_info (Unit *unit, dpacket packet)
{
    TRACE(("ACTION_INFO\n"));
    do_info(unit, packet, GET_PCK_ARG2 (packet) << 2);
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

    if (k->fd != NULL)
	my_close (k->fd);

    xfree(k);
}

static Key *lookup_key (Unit *unit, uae_u32 uniq)
{
    Key *k;
    /* It's hardly worthwhile to optimize this - most of the time there are
     * only one or zero keys. */
    for (k = unit->keys; k; k = k->next) {
	if (uniq == k->uniq)
	    return k;
    }
    write_log ("Error: couldn't find key!\n");

    /* There isn't much hope we will recover. Unix would kill the process,
     * AmigaOS gets killed by it. */
    write_log ("Better reset that Amiga - the system is messed up.\n");
    return 0;
}

static Key *new_key (Unit *unit)
{
    Key *k = (Key *) xmalloc(sizeof(Key));
    k->uniq = ++unit->key_uniq;
    k->fd = NULL;
    k->file_pos = 0;
    k->next = unit->keys;
    unit->keys = k;

    return k;
}

static void
dumplock (Unit *unit, uaecptr lock)
{
    a_inode *a;
    TRACE(("LOCK: 0x%lx", lock));
    if (!lock) {
	TRACE(("\n"));
	return;
    }
    TRACE(("{ next=0x%lx, mode=%ld, handler=0x%lx, volume=0x%lx, aino %lx ",
	   get_long (lock) << 2, get_long (lock+8),
	   get_long (lock+12), get_long (lock+16),
	   get_long (lock + 4)));
    a = lookup_aino (unit, get_long (lock + 4));
    if (a == 0) {
	TRACE(("not found!"));
    } else {
	TRACE(("%s", a->nname));
    }
    TRACE((" }\n"));
}

static a_inode *find_aino (Unit *unit, uaecptr lock, const char *name, uae_u32 *err)
{
   a_inode *a;
   
   if (lock) {
      a_inode *olda = lookup_aino (unit, get_long (lock + 4));
      if (olda == 0) {
         /* That's the best we can hope to do. */
         a = get_aino (unit, &unit->rootnode, name, err);
      } else {
         TRACE(("aino: 0x%08lx", (unsigned long int)olda->uniq));
         TRACE((" \"%s\"\n", olda->nname));
         a = get_aino (unit, olda, name, err);
      }
   } else {
      a = get_aino (unit, &unit->rootnode, name, err);
   }
   if (a) {
      TRACE(("aino=\"%s\"\n", a->nname));
   }
   aino_test (a);
   return a;
}

static uaecptr make_lock (Unit *unit, uae_u32 uniq, long mode)
{
    /* allocate lock from the list kept by the assembly code */
    uaecptr lock;

    lock = get_long (unit->locklist);
    put_long (unit->locklist, get_long (lock));
    lock += 4;

    put_long (lock + 4, uniq);
    put_long (lock + 8, mode);
    put_long (lock + 12, unit->port);
    put_long (lock + 16, unit->volume >> 2);

    /* prepend to lock chain */
    put_long (lock, get_long (unit->volume + 28));
    put_long (unit->volume + 28, lock >> 2);

    DUMPLOCK(unit, lock);
    return lock;
}

static uae_u32 notifyhash (char *s)
{
    uae_u32 hash = 0;
    while (*s) hash = (hash << 5) + *s++;
    return hash % NOTIFY_HASH_SIZE;
}

static Notify *new_notify (Unit *unit, char *name)
{
    Notify *n = (Notify *)xmalloc(sizeof(Notify));
    int hash = notifyhash (name);
    n->next = unit->notifyhash[hash];
    unit->notifyhash[hash] = n;
    n->partname = name;
    return n;
}

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
    xfree(n);
}

#define NOTIFY_CLASS	0x40000000
#define NOTIFY_CODE	0x1234

#define NRF_SEND_MESSAGE 1
#define NRF_SEND_SIGNAL 2
#define NRF_WAIT_REPLY 8
#define NRF_NOTIFY_INITIAL 16
#define NRF_MAGIC (1 << 31)

static void notify_send (Unit *unit, Notify *n)
{
    uaecptr nr = n->notifyrequest;
    int flags = get_long (nr + 12);

    if (flags & NRF_SEND_MESSAGE) {
	if (!(flags & NRF_WAIT_REPLY) || ((flags & NRF_WAIT_REPLY) && !(flags & NRF_MAGIC))) {
	    uae_NotificationHack (unit->port, nr);
	} else if (flags & NRF_WAIT_REPLY) {
	    put_long (nr + 12, get_long (nr + 12) | NRF_MAGIC);
	}
    } else if (flags & NRF_SEND_SIGNAL) {
	uae_Signal (get_long (nr + 16), 1 << get_byte (nr + 20));
    }
}

static void notify_check (Unit *unit, a_inode *a)
{
  Notify *n;
  int hash = notifyhash (a->aname);
  for (n = unit->notifyhash[hash]; n; n = n->next) {
	  uaecptr nr = n->notifyrequest;
	  if (same_aname(n->partname, a->aname)) {
	    uae_u32 err;
	    a_inode *a2 = find_aino (unit, 0, n->fullname, &err);
	    if (err == 0 && a == a2)
	        notify_send (unit, n);
	  }
  }
  if (a->parent) {
    hash = notifyhash (a->parent->aname);
    for (n = unit->notifyhash[hash]; n; n = n->next) {
	    uaecptr nr = n->notifyrequest;
	    if (same_aname(n->partname, a->parent->aname)) {
	      uae_u32 err;
	      a_inode *a2 = find_aino (unit, 0, n->fullname, &err);
	      if (err == 0 && a->parent == a2)
	      	notify_send (unit, n);
	    }
    }
  }
}

static void
action_add_notify (Unit *unit, dpacket packet)
{
    uaecptr nr = GET_PCK_ARG1 (packet);
    int flags;
    Notify *n;
    char *name, *p, *partname;

    TRACE(("ACTION_ADD_NOTIFY\n"));

    name = my_strdup (char1 (get_long (nr + 4)));
    flags = get_long (nr + 12);

    if (!(flags & (NRF_SEND_MESSAGE | NRF_SEND_SIGNAL))) {
      PUT_PCK_RES1 (packet, DOS_FALSE);
      PUT_PCK_RES2 (packet, ERROR_BAD_NUMBER);
	    return;
    }
#if 0
    write_log ("Notify:\n");
    write_log ("nr_Name '%s'\n", char1 (get_long (nr + 0)));
    write_log ("nr_FullName '%s'\n", name);
    write_log ("nr_UserData %08.8X\n", get_long (nr + 8));
    write_log ("nr_Flags %08.8X\n", flags);
    if (flags & NRF_SEND_MESSAGE) {
	write_log ("Message NotifyRequest, port = %08.8X\n", get_long (nr + 16));
    } else if (flags & NRF_SEND_SIGNAL) {
	write_log ("Signal NotifyRequest, Task = %08.8X signal = %d\n", get_long (nr + 16), get_long (nr + 20));
    } else {
	write_log ("corrupt NotifyRequest\n");
    }
#endif

    p = name + strlen (name) - 1;
    if (p[0] == ':') p--;
    while (p > name && p[0] != ':' && p[0] != '/') p--;
    if (p[0] == ':' || p[0] == '/') p++;
    partname = my_strdup (p);
    n = new_notify (unit, partname);
    n->notifyrequest = nr;
    n->fullname = name;
    if (flags & NRF_NOTIFY_INITIAL) {
	uae_u32 err;
	a_inode *a = find_aino (unit, 0, n->fullname, &err);
	if (err == 0)
	    notify_send (unit, n);
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
}
static void
action_remove_notify (Unit *unit, dpacket packet)
{
    uaecptr nr = GET_PCK_ARG1 (packet);
    Notify *n;
    int hash;

    TRACE(("ACTION_REMOVE_NOTIFY\n"));
    for (hash = 0; hash < NOTIFY_HASH_SIZE; hash++) {
        for (n = unit->notifyhash[hash]; n; n = n->next) {
	    if (n->notifyrequest == nr) {
		//write_log ("NotifyRequest %08.8X freed\n", n->notifyrequest);
		xfree (n->fullname);
		xfree (n->partname);
		free_notify (unit, hash, n);
		PUT_PCK_RES1 (packet, DOS_TRUE);
		return;
	    }
	}
    }
    //write_log ("Tried to free non-existing NotifyRequest %08.8X\n", nr);
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void free_lock (Unit *unit, uaecptr lock)
{
    if (! lock)
	return;

    if (lock == get_long (unit->volume + 28) << 2) {
	put_long (unit->volume + 28, get_long (lock));
    } else {
	uaecptr current = get_long (unit->volume + 28);
	uaecptr next = 0;
	while (current) {
	    next = get_long (current << 2);
	    if (lock == next << 2)
		break;
	    current = next;
	}
	put_long (current << 2, get_long (lock));
    }
    lock -= 4;
    put_long (lock, get_long (unit->locklist));
    put_long (unit->locklist, lock);
}

static void
action_lock (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr name = GET_PCK_ARG2 (packet) << 2;
    long mode = GET_PCK_ARG3 (packet);
    a_inode *a;
    uae_u32 err;

    if (mode != SHARED_LOCK && mode != EXCLUSIVE_LOCK) {
	TRACE(("Bad mode %d (should be %d or %d).\n", mode, SHARED_LOCK, EXCLUSIVE_LOCK));
	mode = SHARED_LOCK;
    }

    TRACE(("ACTION_LOCK(0x%lx, \"%s\", %d)\n", lock, bstr (unit, name), mode));
    DUMPLOCK(unit, lock);

    a = find_aino (unit, lock, bstr (unit, name), &err);
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
    PUT_PCK_RES1 (packet, make_lock (unit, a->uniq, mode) >> 2);
}

static void action_free_lock (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    a_inode *a;
    TRACE(("ACTION_FREE_LOCK(0x%lx)\n", lock));
    DUMPLOCK(unit, lock);

    a = lookup_aino (unit, get_long (lock + 4));
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
    free_lock(unit, lock);

    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static uaecptr
action_dup_lock_2 (Unit *unit, dpacket packet, uaecptr lock)
{
    uaecptr out;
    a_inode *a;
    TRACE(("ACTION_DUP_LOCK(0x%lx)\n", lock));
    DUMPLOCK(unit, lock);

    if (!lock) {
	PUT_PCK_RES1 (packet, 0);
	return 0;
    }
    a = lookup_aino (unit, get_long (lock + 4));
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
    de_recycle_aino (unit, a);
    out = make_lock (unit, a->uniq, -2) >> 2;
    PUT_PCK_RES1 (packet, out);
    return out;
}

static void
action_dup_lock (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    action_dup_lock_2 (unit, packet, lock);
}

/* convert time_t to/from AmigaDOS time */
const int secs_per_day = 24 * 60 * 60;
const int diff = (8 * 365 + 2) * (24 * 60 * 60);

static void
get_time (time_t t, long* days, long* mins, long* ticks)
{
    /* time_t is secs since 1-1-1970 */
    /* days since 1-1-1978 */
    /* mins since midnight */
    /* ticks past minute @ 50Hz */

    t -= diff;
    *days = t / secs_per_day;
    t -= *days * secs_per_day;
    *mins = t / 60;
    t -= *mins * 60;
    *ticks = t * 50;
}

static time_t
put_time (long days, long mins, long ticks)
{
    time_t t;
    t = ticks / 50;
    t += mins * 60;
    t += days * secs_per_day;
    t += diff;

    return t;
}

static void free_exkey (Unit *unit, ExamineKey *ek)
{
    if (--ek->aino->exnext_count == 0) {
	TRACE (("Freeing ExKey and reducing total_locked from %d by %d\n",
		unit->total_locked_ainos, ek->aino->locked_children));
	unit->total_locked_ainos -= ek->aino->locked_children;
	ek->aino->locked_children = 0;
    }
    ek->aino = 0;
    ek->uniq = 0;
}

static ExamineKey *lookup_exkey (Unit *unit, uae_u32 uniq)
{
    ExamineKey *ek;
    int i;

    ek = unit->examine_keys;
    for (i = 0; i < EXKEYS; i++, ek++) {
	/* Did we find a free one? */
	if (ek->uniq == uniq)
	    return ek;
    }
    write_log ("Houston, we have a BIG problem.\n");
    return 0;
}

/* This is so sick... who invented ACTION_EXAMINE_NEXT? What did he THINK??? */
static ExamineKey *new_exkey (Unit *unit, a_inode *aino)
{
    uae_u32 uniq;
    uae_u32 oldest = 0xFFFFFFFE;
    ExamineKey *ek, *oldest_ek = 0;
    int i;

    ek = unit->examine_keys;
    for (i = 0; i < EXKEYS; i++, ek++) {
	/* Did we find a free one? */
	if (ek->aino == 0)
	    continue;
	if (ek->uniq < oldest)
	    oldest = (oldest_ek = ek)->uniq;
    }
    ek = unit->examine_keys;
    for (i = 0; i < EXKEYS; i++, ek++) {
	/* Did we find a free one? */
	if (ek->aino == 0)
	    goto found;
    }
    /* This message should usually be harmless. */
    write_log ("Houston, we have a problem (%s).\n", aino->nname);
    free_exkey (unit, oldest_ek);
    ek = oldest_ek;
    found:

    uniq = unit->next_exkey;
    if (uniq >= 0xFFFFFFFE) {
	/* Things will probably go wrong, but most likely the Amiga will crash
	 * before this happens because of something else. */
	uniq = 1;
    }
    unit->next_exkey = uniq+1;
    ek->aino = aino;
    ek->curr_file = 0;
    ek->uniq = uniq;
    return ek;
}

static void move_exkeys (Unit *unit, a_inode *from, a_inode *to)
{
    int i;
    unsigned long tmp = 0;
    for (i = 0; i < EXKEYS; i++) {
	ExamineKey *k = unit->examine_keys + i;
	if (k->uniq == 0)
	    continue;
	if (k->aino == from) {
	    k->aino = to;
	    tmp++;
	}
    }
    if (tmp != from->exnext_count)
	write_log ("filesys.c: Bug in ExNext bookkeeping.  BAD.\n");
    to->exnext_count = from->exnext_count;
    to->locked_children = from->locked_children;
    from->exnext_count = 0;
    from->locked_children = 0;
}

static void
get_fileinfo (Unit *unit, dpacket packet, uaecptr info, a_inode *aino)
{
    struct stat statbuf;
    long days, mins, ticks;
    int i, n, entrytype;
    int fsdb_can = fsdb_cando (unit);
    char *x;

    /* No error checks - this had better work. */
    stat (aino->nname, &statbuf);

    if (aino->parent == 0) {
	/* Guru book says ST_ROOT = 1 (root directory, not currently used)
	 * and some programs really expect 2 from root dir..
	 */
	entrytype = 2;
	x = unit->ui.volname;
    } else {
	entrytype = aino->dir ? 2 : -3;
	x = aino->aname;
    }
    put_long (info + 4, entrytype);
    /* AmigaOS docs say these have to contain the same value. */
    put_long (info + 120, entrytype);
    TRACE(("name=\"%s\"\n", x));
    n = strlen (x);
    if (n > 106)
	n = 106;
    i = 8;
    put_byte (info + i, n); i++;
    while (n--)
	put_byte (info + i, *x), i++, x++;
    while (i < 108)
	put_byte (info + i, 0), i++;

    put_long (info + 116, fsdb_can ? aino->amigaos_mode : fsdb_mode_supported(aino));
    put_long (info + 124, statbuf.st_size);
#ifdef HAVE_ST_BLOCKS
    put_long (info + 128, statbuf.st_blocks);
#else
    put_long (info + 128, statbuf.st_size / 512 + 1);
#endif
    get_time (statbuf.st_mtime, &days, &mins, &ticks);
    put_long (info + 132, days);
    put_long (info + 136, mins);
    put_long (info + 140, ticks);
    if (aino->comment == 0 || !fsdb_can)
	put_long (info + 144, 0);
    else {
	TRACE(("comment=\"%s\"\n", aino->comment));
	i = 144;
	x = aino->comment;
	if (! x)
	    x = "";
	n = strlen (x);
	if (n > 78)
	    n = 78;
	put_byte (info + i, n); i++;
	while (n--)
	    put_byte (info + i, *x), i++, x++;
	while (i < 224)
	    put_byte (info + i, 0), i++;
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void action_examine_object (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr info = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino = 0;

    TRACE(("ACTION_EXAMINE_OBJECT(0x%lx,0x%lx)\n", lock, info));
    DUMPLOCK(unit, lock);

    if (lock != 0)
	aino = lookup_aino (unit, get_long (lock + 4));
    if (aino == 0)
	aino = &unit->rootnode;

    get_fileinfo (unit, packet, info, aino);
    if (aino->dir) {
	put_long (info, 0xFFFFFFFF);
    } else
	put_long (info, 0);
}

/* Read a directory's contents, create a_inodes for each file, and
   mark them as locked in memory so that recycle_aino will not reap
   them.
   We do this to avoid problems with the host OS: we don't want to
   leave the directory open on the host side until all ExNext()s have
   finished - they may never finish!  */

static void populate_directory (Unit *unit, a_inode *base)
{
    void *d = my_opendir (base->nname);
    a_inode *aino;

    if (!d)
    	return;
    for (aino = base->child; aino; aino = aino->sibling) {
	base->locked_children++;
	unit->total_locked_ainos++;
    }
    TRACE(("Populating directory, child %p, locked_children %d\n",
	   base->child, base->locked_children));
    for (;;) {
	char fn[MAX_DPATH];
	int ok;
	uae_u32 err;

	/* Find next file that belongs to the Amiga fs (skipping things
	   like "..", "." etc.  */
	do {
	    ok = my_readdir (d, fn);
	} while (ok && fsdb_name_invalid (fn));
	if (!ok)
	    break;
	/* This calls init_child_aino, which will notice that the parent is
	   being ExNext()ed, and it will increment the locked counts.  */
	aino = lookup_child_aino_for_exnext (unit, base, fn, &err);
    }
    my_closedir (d);
}

static void do_examine (Unit *unit, dpacket packet, ExamineKey *ek, uaecptr info)
{
    for (;;) {
	char *name;
    if (ek->curr_file == 0)
	    break;
	  name = ek->curr_file->nname;
    get_fileinfo (unit, packet, info, ek->curr_file);
    ek->curr_file = ek->curr_file->sibling;
	if (!fsdb_exists(name)) {
	    TRACE (("%s orphaned", name));
	    continue;
	}
    TRACE (("curr_file set to %p %s\n", ek->curr_file,
	    ek->curr_file ? ek->curr_file->aname : "NULL"));
    return;
    }
    TRACE(("no more entries\n"));
    free_exkey (unit, ek);
    PUT_PCK_RES1 (packet, DOS_FALSE);
    PUT_PCK_RES2 (packet, ERROR_NO_MORE_ENTRIES);
}

static void action_examine_next (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr info = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino = 0;
    ExamineKey *ek;
    uae_u32 uniq;

    TRACE(("ACTION_EXAMINE_NEXT(0x%lx,0x%lx)\n", lock, info));
    gui_hd_led (1);
    DUMPLOCK(unit, lock);

    if (lock != 0)
	aino = lookup_aino (unit, get_long (lock + 4));
    if (aino == 0)
	aino = &unit->rootnode;

    uniq = get_long (info);
    if (uniq == 0) {
	write_log ("ExNext called for a file! (Houston?)\n");
	goto no_more_entries;
    } else if (uniq == 0xFFFFFFFE)
	goto no_more_entries;
    else if (uniq == 0xFFFFFFFF) {
	TRACE(("Creating new ExKey\n"));
	ek = new_exkey (unit, aino);
	if (ek) {
	    if (aino->exnext_count++ == 0)
		populate_directory (unit, aino);
	}
	ek->curr_file = aino->child;
	TRACE(("Initial curr_file: %p %s\n", ek->curr_file,
	       ek->curr_file ? ek->curr_file->aname : "NULL"));
    } else {
	TRACE(("Looking up ExKey\n"));
	ek = lookup_exkey (unit, get_long (info));
    }
    if (ek == 0) {
	write_log ("Couldn't find a matching ExKey. Prepare for trouble.\n");
	goto no_more_entries;
    }
    put_long (info, ek->uniq);
    do_examine (unit, packet, ek, info);
    return;

  no_more_entries:
    PUT_PCK_RES1 (packet, DOS_FALSE);
    PUT_PCK_RES2 (packet, ERROR_NO_MORE_ENTRIES);
}

static void do_find (Unit *unit, dpacket packet, int mode, int create, int fallback)
{
   uaecptr fh = GET_PCK_ARG1 (packet) << 2;
   uaecptr lock = GET_PCK_ARG2 (packet) << 2;
   uaecptr name = GET_PCK_ARG3 (packet) << 2;
   a_inode *aino;
   Key *k;
   void *fd;
   uae_u32 err;
   mode_t openmode;
   int aino_created = 0;
   
   TRACE(("ACTION_FIND_*(0x%lx,0x%lx,\"%s\",%d,%d)\n", fh, lock, bstr (unit, name), mode, create));
   DUMPLOCK(unit, lock);
   
   aino = find_aino (unit, lock, bstr (unit, name), &err);
   
   if (aino == 0 || (err != 0 && err != ERROR_OBJECT_NOT_AROUND)) {
      /* Whatever it is, we can't handle it. */
      PUT_PCK_RES1 (packet, DOS_FALSE);
      PUT_PCK_RES2 (packet, err);
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
      if (create == 2 && (aino->amigaos_mode & A_FIBF_DELETE) != 0) {
         PUT_PCK_RES1 (packet, DOS_FALSE);
         PUT_PCK_RES2 (packet, ERROR_DELETE_PROTECTED);
         return;
      }
      if (create != 2) {
         if ((((mode & aino->amigaos_mode) & A_FIBF_WRITE) != 0 || unit->ui.readonly)
               && fallback)
         {
            mode &= ~A_FIBF_WRITE;
         }
         /* Kick 1.3 doesn't check read and write access bits - maybe it would be
          * simpler just not to do that either. */
         if ((mode & A_FIBF_WRITE) != 0 && unit->ui.readonly) {
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
      aino = create_child_aino (unit, aino, my_strdup (bstr_cut (unit, name)), 0);
      if (aino == 0) {
         PUT_PCK_RES1 (packet, DOS_FALSE);
         PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
         return;
      }
      aino_created = 1;
   }
   
   prepare_for_open (aino->nname);
   
   openmode = (((mode & A_FIBF_READ) == 0 ? O_WRONLY
         : (mode & A_FIBF_WRITE) == 0 ? O_RDONLY
               : O_RDWR)
                 | (create ? O_CREAT : 0)
                 | (create == 2 ? O_TRUNC : 0));
   
   fd = my_open (aino->nname, openmode | O_BINARY);
   
   if (fd == NULL) {
      if (aino_created)
         delete_aino (unit, aino);
      PUT_PCK_RES1 (packet, DOS_FALSE);
      PUT_PCK_RES2 (packet, dos_errno ());
      return;
   }
   k = new_key (unit);
   k->fd = fd;
   k->aino = aino;
   k->dosmode = mode;
   k->createmode = create;
   k->notifyactive = create ? 1 : 0;

   if (create)
     fsdb_set_file_attrs (aino);
   
   put_long (fh+36, k->uniq);
   if (create == 2)
      aino->elock = 1;
   else
      aino->shlock++;
   de_recycle_aino (unit, aino);
   PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
action_lock_from_fh (Unit *unit, dpacket packet)
{
    uaecptr out;
    Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
    write_log("lock_from_fh %x\n", k);
    if (k == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	return;
    }
    out = action_dup_lock_2 (unit, packet, make_lock (unit, k->aino->uniq, -2));
    write_log("=%x\n", out);
}

static void
action_fh_from_lock (Unit *unit, dpacket packet)
{
    uaecptr fh = GET_PCK_ARG1 (packet) << 2;
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino;
    Key *k;
    void *fd;
    mode_t openmode;
    int mode;

    TRACE(("ACTION_FH_FROM_LOCK(0x%lx,0x%lx)\n",fh,lock));
    DUMPLOCK(unit,lock);

    if (!lock) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, 0);
	return;
    }

    aino = lookup_aino (unit, get_long (lock + 4));
    if (aino == 0)
	aino = &unit->rootnode;
    mode = aino->amigaos_mode; /* Use same mode for opened filehandle as existing Lock() */

    prepare_for_open (aino->nname);
    TRACE (("  mode is %d\n", mode));
    openmode = (((mode & A_FIBF_READ) ? O_WRONLY
		 : (mode & A_FIBF_WRITE) ? O_RDONLY
		 : O_RDWR));

   /* the files on CD really can have the write-bit set.  */
    if (unit->ui.readonly)
	openmode = O_RDONLY;

    fd = my_open (aino->nname, openmode | O_BINARY);

    if (fd == NULL) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, dos_errno());
	return;
    }
    k = new_key (unit);
    k->fd = fd;
    k->aino = aino;

    put_long (fh+36, k->uniq);
    /* I don't think I need to play with shlock count here, because I'm
       opening from an existing lock ??? */

    /* Is this right?  I don't completely understand how this works.  Do I
       also need to free_lock() my lock, since nobody else is going to? */
    de_recycle_aino (unit, aino);
    PUT_PCK_RES1 (packet, DOS_TRUE);
    /* PUT_PCK_RES2 (packet, k->uniq); - this shouldn't be necessary, try without it */
}

static void
action_find_input (Unit *unit, dpacket packet)
{
    do_find(unit, packet, A_FIBF_READ|A_FIBF_WRITE, 0, 1);
}

static void
action_find_output (Unit *unit, dpacket packet)
{
    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }
    do_find(unit, packet, A_FIBF_READ|A_FIBF_WRITE, 2, 0);
}

static void
action_find_write (Unit *unit, dpacket packet)
{
    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }
    do_find(unit, packet, A_FIBF_READ|A_FIBF_WRITE, 1, 0);
}

/* change file/dir's parent dir modification time */
static void updatedirtime (a_inode *a1, int now)
{
    struct stat statbuf;
    struct utimbuf ut;
    long days, mins, ticks;

    if (!a1->parent)
	return;
    if (!now) {
	if (stat (a1->nname, &statbuf) == -1)
	    return;
	get_time (statbuf.st_mtime, &days, &mins, &ticks);
	ut.actime = ut.modtime = put_time(days, mins, ticks);
	utime (a1->parent->nname, &ut);
    } else {
	utime (a1->parent->nname, NULL);
    }
}

static void
action_end (Unit *unit, dpacket packet)
{
    Key *k;
    TRACE(("ACTION_END(0x%lx)\n", GET_PCK_ARG1 (packet)));

    k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (k != 0) {
	if (k->notifyactive) {
	    notify_check (unit, k->aino);
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

static void
action_read (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uaecptr addr = GET_PCK_ARG2 (packet);
	long size = (uae_s32)GET_PCK_ARG3 (packet);
	int actual;
	
	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		/* PUT_PCK_RES2 (packet, EINVAL); */
		return;
	}
	TRACE(("ACTION_READ(%s,0x%lx,%ld)\n",k->aino->nname,addr,size));
  gui_hd_led (1);
#ifdef RELY_ON_LOADSEG_DETECTION
	/* HACK HACK HACK HACK
	 * Try to detect a LoadSeg() */
	if (k->file_pos == 0 && size >= 4) {
		unsigned char buf[4];
		off_t currpos = my_lseek(k->fd, 0, SEEK_CUR);
		my_read(k->fd, buf, 4);
		my_lseek(k->fd, currpos, SEEK_SET);
		if (buf[0] == 0 && buf[1] == 0 && buf[2] == 3 && buf[3] == 0xF3)
			possible_loadseg();
	}
#endif
    if (valid_address (addr, size)) {
		uae_u8 *realpt;
		realpt = get_real_address (addr);
		
		actual = my_read(k->fd, (char *) realpt, size);
		
		if (actual == 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, 0);
		} else if (actual < 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, dos_errno());
		} else {
			PUT_PCK_RES1 (packet, actual);
			k->file_pos += actual;
		}
	} else {
		char *buf;
	off_t old, filesize;

	write_log ("unixfs warning: Bad pointer passed for read: %08x, size %d\n", addr, size);
		/* ugh this is inefficient but easy */

	old = my_lseek (k->fd, 0, SEEK_CUR);
	filesize = my_lseek (k->fd, 0, SEEK_END);
	my_lseek (k->fd, old, SEEK_SET);
	  if (size > filesize)
	    size = filesize;

	  buf = (char *)malloc(size);
		if (!buf) {
			PUT_PCK_RES1 (packet, -1);
			PUT_PCK_RES2 (packet, ERROR_NO_FREE_STORE);
			return;
		}
		actual = my_read(k->fd, buf, size);
		
		if (actual < 0) {
			PUT_PCK_RES1 (packet, 0);
			PUT_PCK_RES2 (packet, dos_errno());
		} else {
			int i;
			PUT_PCK_RES1 (packet, actual);
			for (i = 0; i < actual; i++)
				put_byte(addr + i, buf[i]);
			k->file_pos += actual;
		}
		xfree (buf);
	}
}

static void
action_write (Unit *unit, dpacket packet)
{
	Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
	uaecptr addr = GET_PCK_ARG2 (packet);
	long size = GET_PCK_ARG3 (packet);
  long actual;
	char *buf;
	int i;
	
	if (k == 0) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		/* PUT_PCK_RES2 (packet, EINVAL); */
		return;
	}
	
  gui_hd_led (2);
	TRACE(("ACTION_WRITE(%s,0x%lx,%ld)\n",k->aino->nname,addr,size));
	
	if (unit->ui.readonly) {
		PUT_PCK_RES1 (packet, DOS_FALSE);
		PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
		return;
	}
	
	/* ugh this is inefficient but easy */
	buf = (char *)xmalloc(size);
	if (!buf) {
		PUT_PCK_RES1 (packet, -1);
		PUT_PCK_RES2 (packet, ERROR_NO_FREE_STORE);
		return;
	}
	
	for (i = 0; i < size; i++)
		buf[i] = get_byte(addr + i);
	
	actual = my_write(k->fd, buf, size);
	PUT_PCK_RES1 (packet, actual);
	if (actual != size)
		PUT_PCK_RES2 (packet, dos_errno ());
	if (actual >= 0)
		k->file_pos += actual;
	
  k->notifyactive = 1;
	xfree (buf);
}

static void
action_seek (Unit *unit, dpacket packet)
{
    Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
    long pos = (uae_s32)GET_PCK_ARG2 (packet);
    long mode = (uae_s32)GET_PCK_ARG3 (packet);
    off_t res;
    long old;
    int whence = SEEK_CUR;

    if (k == 0) {
	PUT_PCK_RES1 (packet, -1);
	PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
	return;
    }

    if (mode > 0) whence = SEEK_END;
    if (mode < 0) whence = SEEK_SET;

    TRACE(("ACTION_SEEK(%s,%d,%d)\n", k->aino->nname, pos, mode));
    gui_hd_led (1);

    old = my_lseek (k->fd, 0, SEEK_CUR);
    {      
	uae_s32 temppos;
	long filesize = my_lseek (k->fd, 0, SEEK_END);
	my_lseek (k->fd, old, SEEK_SET);

	if (whence == SEEK_CUR) temppos = old + pos;
	if (whence == SEEK_SET) temppos = pos;
	if (whence == SEEK_END) temppos = filesize + pos;
	if (filesize < temppos) {
	    res = -1;
	    PUT_PCK_RES1 (packet,res);
	    PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
	    return;
	}
    }

    res = my_lseek (k->fd, pos, whence);

    if (-1 == res) {
	PUT_PCK_RES1 (packet, res);
	PUT_PCK_RES2 (packet, ERROR_SEEK_ERROR);
    } else
	PUT_PCK_RES1 (packet, old);
    k->file_pos = res;
}

static void
action_set_protect (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    uae_u32 mask = GET_PCK_ARG4 (packet);
    a_inode *a;
    uae_u32 err;

    TRACE(("ACTION_SET_PROTECT(0x%lx,\"%s\",0x%lx)\n", lock, bstr (unit, name), mask));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    a = find_aino (unit, lock, bstr (unit, name), &err);
    if (err != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
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
    notify_check (unit, a);
    gui_hd_led (2);
}

static void action_set_comment (Unit * unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    uaecptr comment = GET_PCK_ARG4 (packet) << 2;
    char *commented = NULL;
    a_inode *a;
    uae_u32 err;

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    if (fsdb_cando (unit)) {
      commented = bstr (unit, comment);
      commented = strlen (commented) > 0 ? my_strdup (commented) : NULL;
    }
    TRACE (("ACTION_SET_COMMENT(0x%lx,\"%s\")\n", lock, commented));

    a = find_aino (unit, lock, bstr (unit, name), &err);
    if (err != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);

	maybe_free_and_out:
	if (commented)
	    xfree (commented);
	return;
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
    PUT_PCK_RES2 (packet, 0);
    if (a->comment == 0 && commented == 0)
	goto maybe_free_and_out;
    if (a->comment != 0 && commented != 0 && strcmp (a->comment, commented) == 0)
	goto maybe_free_and_out;
    if (a->comment)
	xfree (a->comment);
    a->comment = commented;
    fsdb_set_file_attrs (a);
    notify_check (unit, a);
    gui_hd_led (2);
}

static void
action_same_lock (Unit *unit, dpacket packet)
{
    uaecptr lock1 = GET_PCK_ARG1 (packet) << 2;
    uaecptr lock2 = GET_PCK_ARG2 (packet) << 2;

    TRACE(("ACTION_SAME_LOCK(0x%lx,0x%lx)\n",lock1,lock2));
    DUMPLOCK(unit, lock1); DUMPLOCK(unit, lock2);

    if (!lock1 || !lock2) {
	PUT_PCK_RES1 (packet, lock1 == lock2 ? DOS_TRUE : DOS_FALSE);
    } else {
	PUT_PCK_RES1 (packet, get_long (lock1 + 4) == get_long (lock2 + 4) ? DOS_TRUE : DOS_FALSE);
    }
}

static void
action_change_mode (Unit *unit, dpacket packet)
{
#define CHANGE_LOCK 0
#define CHANGE_FH 1
    /* will be CHANGE_FH or CHANGE_LOCK value */
    long type = GET_PCK_ARG1 (packet);
    /* either a file-handle or lock */
    uaecptr object = GET_PCK_ARG2 (packet) << 2; 
    /* will be EXCLUSIVE_LOCK/SHARED_LOCK if CHANGE_LOCK,
     * or MODE_OLDFILE/MODE_NEWFILE/MODE_READWRITE if CHANGE_FH */
    long mode = GET_PCK_ARG3 (packet);
    unsigned long uniq;
    a_inode *a = NULL, *olda = NULL;
    uae_u32 err = 0;
    TRACE(("ACTION_CHANGE_MODE(0x%lx,%d,%d)\n",object,type,mode));

    if (! object
	|| (type != CHANGE_FH && type != CHANGE_LOCK))
    {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_INVALID_LOCK);
	return;
    }

    /* @@@ Brian: shouldn't this be good enough to support
       CHANGE_FH?  */
    if (type == CHANGE_FH)
	mode = (mode == 1006 ? -1 : -2);

    if (type == CHANGE_LOCK)
	uniq = get_long (object + 4);
    else {
	Key *k = lookup_key (unit, object);
	if (!k) {
	    PUT_PCK_RES1 (packet, DOS_FALSE);
	    PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
	    return;
	}
	uniq = k->aino->uniq;
    }
    a = lookup_aino (unit, uniq);

    if (! a)
	err = ERROR_INVALID_LOCK;
    else {
	if (mode == -1) {
	    if (a->shlock > 1)
		err = ERROR_OBJECT_IN_USE;
	    else {
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

static void
action_parent_common (Unit *unit, dpacket packet, unsigned long uniq)
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
    PUT_PCK_RES1 (packet, make_lock (unit, olda->parent->uniq, -2) >> 2);
}

static void
action_parent_fh (Unit *unit, dpacket packet)
{
    Key *k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (!k) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
	return;
    }
    action_parent_common (unit, packet, k->aino->uniq);
}

static void
action_parent (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;

    TRACE(("ACTION_PARENT(0x%lx)\n",lock));

    if (!lock) {
	PUT_PCK_RES1 (packet, 0);
	PUT_PCK_RES2 (packet, 0);
    } else {
    action_parent_common (unit, packet, get_long (lock + 4));
    }
    TRACE(("=%x %d\n", GET_PCK_RES1 (packet), GET_PCK_RES2 (packet)));
}

static void
action_create_dir (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr name = GET_PCK_ARG2 (packet) << 2;
    a_inode *aino;
    uae_u32 err;

    TRACE(("ACTION_CREATE_DIR(0x%lx,\"%s\")\n", lock, bstr (unit, name)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    aino = find_aino (unit, lock, bstr (unit, name), &err);
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
    aino = create_child_aino (unit, aino, my_strdup (bstr_cut (unit, name)), 1);
    if (aino == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
	return;
    }

    if (my_mkdir (aino->nname) == -1) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, dos_errno());
	return;
    }
    aino->shlock = 1;
    fsdb_set_file_attrs (aino);
    de_recycle_aino (unit, aino);
    notify_check (unit, aino);
    updatedirtime (aino, 0);
    PUT_PCK_RES1 (packet, make_lock (unit, aino->uniq, -2) >> 2);
    gui_hd_led (2);
}

static void
action_examine_fh (Unit *unit, dpacket packet)
{
    Key *k;
    a_inode *aino = 0;
    uaecptr info = GET_PCK_ARG2 (packet) << 2;

    TRACE(("ACTION_EXAMINE_FH(0x%lx,0x%lx)\n",
	   GET_PCK_ARG1 (packet), GET_PCK_ARG2 (packet) ));

    k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (k != 0)
	aino = k->aino;
    if (aino == 0)
	aino = &unit->rootnode;

    get_fileinfo (unit, packet, info, aino);
    if (aino->dir)
	put_long (info, 0xFFFFFFFF);
    else
	put_long (info, 0);
}

/* For a nice example of just how contradictory documentation can be, see the
 * Autodoc for DOS:SetFileSize and the Packets.txt description of this packet...
 * This implementation tries to mimic the behaviour of the Kick 3.1 ramdisk
 * (which seems to match the Autodoc description). */
static void
action_set_file_size (Unit *unit, dpacket packet)
{
    Key *k, *k1;
    off_t offset = GET_PCK_ARG2 (packet);
    long mode = (uae_s32)GET_PCK_ARG3 (packet);
    int whence = SEEK_CUR;

    if (mode > 0) whence = SEEK_END;
    if (mode < 0) whence = SEEK_SET;

    TRACE(("ACTION_SET_FILE_SIZE(0x%lx, %d, 0x%x)\n", GET_PCK_ARG1 (packet), offset, mode));

    k = lookup_key (unit, GET_PCK_ARG1 (packet));
    if (k == 0) {
	PUT_PCK_RES1 (packet, DOS_TRUE);
	PUT_PCK_RES2 (packet, ERROR_OBJECT_NOT_AROUND);
	return;
    }

    gui_hd_led (2);
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
    offset = my_lseek (k->fd, offset, whence);
    my_write (k->fd, /* whatever */(char *)&k1, 1);
    if (k->file_pos > offset)
	k->file_pos = offset;
    my_lseek (k->fd, k->file_pos, SEEK_SET);

    /* Brian: no bug here; the file _must_ be one byte too large after writing
       The write is supposed to guarantee that the file can't be smaller than
       the requested size, the truncate guarantees that it can't be larger.
       If we were to write one byte earlier we'd clobber file data.  */
    if (my_truncate (k->aino->nname, offset) == -1) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
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
	    my_close (k1->fd);
	    write_log ("handle %p freed\n", k1->fd);
	}
    }
    return wehavekeys;
}

static void relock_re(Unit *unit, a_inode *a1, a_inode *a2, int failed)
{
    Key *k1, *knext;

    for (k1 = unit->keys; k1; k1 = knext) {
        knext = k1->next;
        if (k1->aino == a1 && k1->fd) {
	    int mode = (k1->dosmode & A_FIBF_READ) == 0 ? O_WRONLY : (k1->dosmode & A_FIBF_WRITE) == 0 ? O_RDONLY : O_RDWR;
	    mode |= O_BINARY;
	    if (failed) {
	        /* rename still failed, restore fd */
	        k1->fd = my_open (a1->nname, mode);
	        write_log ("restoring old handle '%s' %d\n", a1->nname, k1->dosmode);
	    } else {
	        /* transfer fd to new name */
		if (a2) {
		    k1->aino = a2;
		    k1->fd = my_open (a2->nname, mode);
		    write_log ("restoring new handle '%s' %d\n", a2->nname, k1->dosmode);
		} else {
		    write_log ("no new handle, deleting old lock(s).\n");
		}
	    }
	    if (k1->fd == NULL) {
		write_log ("relocking failed '%s' -> '%s'\n", a1->nname, a2->nname);
		free_key (unit, k1);
	    } else {
	        my_lseek (k1->fd, k1->file_pos, SEEK_SET);
	    }
	}
    }
}

static void
action_delete_object (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG1 (packet) << 2;
    uaecptr name = GET_PCK_ARG2 (packet) << 2;
    a_inode *a;
    uae_u32 err;

    TRACE(("ACTION_DELETE_OBJECT(0x%lx,\"%s\")\n", lock, bstr (unit, name)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    a = find_aino (unit, lock, bstr (unit, name), &err);

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
    if (a->dir) {
    	/* This should take care of removing the fsdb if no files remain.  */
    	fsdb_dir_writeback (a);
	  if (my_rmdir (a->nname) == -1) {
    		PUT_PCK_RES1 (packet, DOS_FALSE);
    		PUT_PCK_RES2 (packet, dos_errno());
    		return;
    	}
    } else {
    	if (my_unlink (a->nname) == -1) {
    		PUT_PCK_RES1 (packet, DOS_FALSE);
    		PUT_PCK_RES2 (packet, dos_errno());
    		return;
    	}
    }

    notify_check (unit, a);
    updatedirtime (a, 1);
    if (a->child != 0) {
    	write_log ("Serious error in action_delete_object.\n");
    	a->deleted = 1;
    } else {
    	delete_aino (unit, a);
    }
    PUT_PCK_RES1 (packet, DOS_TRUE);
    gui_hd_led (2);
}

static void
action_set_date (Unit *unit, dpacket packet)
{
    uaecptr lock = GET_PCK_ARG2 (packet) << 2;
    uaecptr name = GET_PCK_ARG3 (packet) << 2;
    uaecptr date = GET_PCK_ARG4 (packet);
    a_inode *a;
    struct utimbuf ut;
    uae_u32 err;

    TRACE(("ACTION_SET_DATE(0x%lx,\"%s\")\n", lock, bstr (unit, name)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    ut.actime = ut.modtime = put_time(get_long (date), get_long (date + 4),
				      get_long (date + 8));
    a = find_aino (unit, lock, bstr (unit, name), &err);
    if (err == 0 && utime (a->nname, &ut) == -1)
	err = dos_errno ();
    if (err != 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, err);
    } else
	PUT_PCK_RES1 (packet, DOS_TRUE);
    notify_check (unit, a);
    gui_hd_led (2);
}

static void
action_rename_object (Unit *unit, dpacket packet)
{
    uaecptr lock1 = GET_PCK_ARG1 (packet) << 2;
    uaecptr name1 = GET_PCK_ARG2 (packet) << 2;
    uaecptr lock2 = GET_PCK_ARG3 (packet) << 2;
    uaecptr name2 = GET_PCK_ARG4 (packet) << 2;
    a_inode *a1, *a2;
    uae_u32 err1, err2;
    Key *k1, *knext;
    int wehavekeys = 0;

    TRACE(("ACTION_RENAME_OBJECT(0x%lx,\"%s\",", lock1, bstr (unit, name1)));
    TRACE(("0x%lx,\"%s\")\n", lock2, bstr (unit, name2)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    a1 = find_aino (unit, lock1, bstr (unit, name1), &err1);
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
    a2 = find_aino (unit, lock2, bstr (unit, name2), &err2);
    if (a2 == a1) {
	/* Renaming to the same name, but possibly different case.  */
	if (strcmp (a1->aname, bstr_cut (unit, name2)) == 0) {
	    /* Exact match -> do nothing.  */
	    notify_check (unit, a1);
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

    a2 = create_child_aino (unit, a2, bstr_cut (unit, name2), a1->dir);
    if (a2 == 0) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_IS_FULL); /* best we can do */
	return;
    }

    if (-1 == my_rename (a1->nname, a2->nname)) {
	int ret = -1;
	/* maybe we have open file handles that caused failure? */
	wehavekeys = relock_do(unit, a1);
	/* try again... */
	ret = my_rename (a1->nname, a2->nname);
	/* restore locks */
	relock_re(unit, a1, a2, ret == -1 ? 1 : 0);
	if (ret == -1) {
	delete_aino (unit, a2);
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, dos_errno ());
	return;
    }
    }
    
    notify_check (unit, a1);
    notify_check (unit, a2);
    a2->comment = a1->comment;
    a1->comment = 0;
    a2->amigaos_mode = a1->amigaos_mode;
    a2->uniq = a1->uniq;
    a2->elock = a1->elock;
    a2->shlock = a1->shlock;
    a2->has_dbentry = a1->has_dbentry;
    a2->db_offset = a1->db_offset;
    a2->dirty = 0;
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
    gui_hd_led (2);
}

static void
action_current_volume (Unit *unit, dpacket packet)
{
    PUT_PCK_RES1 (packet, unit->volume >> 2);
}

static void
action_rename_disk (Unit *unit, dpacket packet)
{
    uaecptr name = GET_PCK_ARG1 (packet) << 2;
    int i;
    int namelen;

    TRACE(("ACTION_RENAME_DISK(\"%s\")\n", bstr (unit, name)));

    if (unit->ui.readonly) {
	PUT_PCK_RES1 (packet, DOS_FALSE);
	PUT_PCK_RES2 (packet, ERROR_DISK_WRITE_PROTECTED);
	return;
    }

    /* get volume name */
    namelen = get_byte (name); name++;
    xfree (unit->ui.volname);
    unit->ui.volname = (char *) xmalloc (namelen + 1);
    for (i = 0; i < namelen; i++, name++)
	unit->ui.volname[i] = get_byte (name);
    unit->ui.volname[i] = 0;

    put_byte (unit->volume + 44, namelen);
    for (i = 0; i < namelen; i++)
	put_byte (unit->volume + 45 + i, unit->ui.volname[i]);

    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
action_is_filesystem (Unit *unit, dpacket packet)
{
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

static void
action_flush (Unit *unit, dpacket packet)
{
    /* sync(); */ /* pretty drastic, eh */
    PUT_PCK_RES1 (packet, DOS_TRUE);
}

int last_n, last_n2;
void blehint(void)
{
    do_uae_int_requested();
}

/* We don't want multiple interrupts to be active at the same time. I don't
 * know whether AmigaOS takes care of that, but this does. */
static uae_sem_t singlethread_int_sem;

static uae_u32 REGPARAM2 exter_int_helper (TrapContext *context)
{
    UnitInfo *uip = current_mountinfo.ui;
    uaecptr port;
    int n = m68k_dreg (&context->regs, 0);
    static int unit_no;

    last_n = n;
    last_n2 = -1;

    switch (n) {
     case 0:
	/* Determine whether a given EXTER interrupt is for us. */
	if (uae_int_requested) {
	    if (uae_sem_trywait (&singlethread_int_sem) != 0)
		/* Pretend it isn't for us. We might get it again later. */
		return 0;
	    /* Clear the interrupt flag _before_ we do any processing.
	     * That way, we can get too many interrupts, but never not
	     * enough. */
	    uae_int_requested = 0;
	    unit_no = 0;
	    return 1;
	}
	return 0;
     case 1:
	/* Release a message_lock. This is called as soon as the message is
	 * received by the assembly code. We use the opportunity to check
	 * whether we have some locks that we can give back to the assembler
	 * code.
	 * Note that this is called from the main loop, unlike the other cases
	 * in this switch statement which are called from the interrupt handler.
	 */
#ifdef UAE_FILESYS_THREADS
	{
	    Unit *unit = find_unit (m68k_areg (&context->regs, 5));
	    uaecptr msg = m68k_areg (&context->regs, 4);
	    unit->cmds_complete = unit->cmds_acked;
	    while (comm_pipe_has_data (unit->ui.back_pipe)) {
		uaecptr locks, lockend;
		int cnt = 0;
		locks = read_comm_pipe_int_blocking (unit->ui.back_pipe);
		lockend = locks;
		while (get_long (lockend) != 0) {
		    if (get_long (lockend) == lockend) {
			write_log ("filesystem lock queue corrupted!\n");
			break;
		    }
		    lockend = get_long (lockend);
		    cnt++;
		}
		TRACE(("%d %x %x %x\n", cnt, locks, lockend, m68k_areg (&context->regs, 3)));
		put_long (lockend, get_long (m68k_areg (&context->regs, 3)));
		put_long (m68k_areg (&context->regs, 3), locks);
	    }
	}
#else
	write_log ("exter_int_helper should not be called with arg 1!\n");
#endif
	break;
     case 2:
	/* Find work that needs to be done:
	 * return d0 = 0: none
	 *        d0 = 1: PutMsg(), port in a0, message in a1
	 *        d0 = 2: Signal(), task in a1, signal set in d1
	 *        d0 = 3: ReplyMsg(), message in a1
	 *        d0 = 4: Cause(), interrupt in a1
	 *        d0 = 5: AllocMem(), size in d0, flags in d1, pointer to address at a0
	 *        d0 = 6: FreeMem(), memory in a1, size in d0
	 */

#ifdef SUPPORT_THREADS
	/* First, check signals/messages */
	while (comm_pipe_has_data (&native2amiga_pending)) {
      int cmd = read_comm_pipe_int_blocking (&native2amiga_pending);
	    switch (cmd) {
	     case 0: /* Signal() */
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		m68k_dreg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 2;

	     case 1: /* PutMsg() */
		m68k_areg (&context->regs, 0) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 1;

	     case 2: /* ReplyMsg() */
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 3;

	     case 3: /* Cause() */
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 4;

	     case 4: /* NotifyHack() */
		m68k_areg (&context->regs, 0) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		m68k_areg (&context->regs, 1) = read_comm_pipe_u32_blocking (&native2amiga_pending);
		return 5;

	     default:
		write_log ("exter_int_helper: unknown native action %X\n", cmd);
		break;
	    }
	}
#endif

	/* Find some unit that needs a message sent, and return its port,
	 * or zero if all are done.
	 * Take care not to dereference self for units that didn't have their
	 * startup packet sent. */
	for (;;) {
	    if (unit_no >= current_mountinfo.num_units)
		return 0;

	    if (uip[unit_no].self != 0
		&& uip[unit_no].self->cmds_acked == uip[unit_no].self->cmds_complete
		&& uip[unit_no].self->cmds_acked != uip[unit_no].self->cmds_sent)
		break;
	    unit_no++;
	}
	uip[unit_no].self->cmds_acked = uip[unit_no].self->cmds_sent;
	port = uip[unit_no].self->port;
	if (port) {
	    m68k_areg (&context->regs, 0) = port;
	    m68k_areg (&context->regs, 1) = find_unit (port)->dummy_message;
	    unit_no++;
	    return 1;
	}
	break;
     case 3:
	uae_sem_wait (&singlethread_int_sem);
	break;
     case 4:
	/* Exit the interrupt, and release the single-threading lock. */
	uae_sem_post (&singlethread_int_sem);
	break;

     default:
	write_log ("Shouldn't happen in exter_int_helper.\n");
	break;
    }
    return 0;
}

static int handle_packet (Unit *unit, dpacket pck)
{
    uae_s32 type = GET_PCK_TYPE (pck);
    PUT_PCK_RES2 (pck, 0);
    
    switch (type) {
     case ACTION_LOCATE_OBJECT: action_lock (unit, pck); break;
     case ACTION_FREE_LOCK: action_free_lock (unit, pck); break;
     case ACTION_COPY_DIR: action_dup_lock (unit, pck); break;
     case ACTION_DISK_INFO: action_disk_info (unit, pck); break;
     case ACTION_INFO: action_info (unit, pck); break;
     case ACTION_EXAMINE_OBJECT: action_examine_object (unit, pck); break;
     case ACTION_EXAMINE_NEXT: action_examine_next (unit, pck); break;
     case ACTION_FIND_INPUT: action_find_input (unit, pck); break;
     case ACTION_FIND_WRITE: action_find_write (unit, pck); break;
     case ACTION_FIND_OUTPUT: action_find_output (unit, pck); break;
     case ACTION_END: action_end (unit, pck); break;
     case ACTION_READ: action_read (unit, pck); break;
     case ACTION_WRITE: action_write (unit, pck); break;
     case ACTION_SEEK: action_seek (unit, pck); break;
     case ACTION_SET_PROTECT: action_set_protect (unit, pck); break;
     case ACTION_SET_COMMENT: action_set_comment (unit, pck); break;
     case ACTION_SAME_LOCK: action_same_lock (unit, pck); break;
     case ACTION_PARENT: action_parent (unit, pck); break;
     case ACTION_CREATE_DIR: action_create_dir (unit, pck); break;
     case ACTION_DELETE_OBJECT: action_delete_object (unit, pck); break;
     case ACTION_RENAME_OBJECT: action_rename_object (unit, pck); break;
     case ACTION_SET_DATE: action_set_date (unit, pck); break;
     case ACTION_CURRENT_VOLUME: action_current_volume (unit, pck); break;
     case ACTION_RENAME_DISK: action_rename_disk (unit, pck); break;
     case ACTION_IS_FILESYSTEM: action_is_filesystem (unit, pck); break;
     case ACTION_FLUSH: action_flush (unit, pck); break;
     
     /* 2.0+ packet types */
     case ACTION_SET_FILE_SIZE: action_set_file_size (unit, pck); break;
     case ACTION_EXAMINE_FH: action_examine_fh (unit, pck); break;
     case ACTION_FH_FROM_LOCK: action_fh_from_lock (unit, pck); break;
     case ACTION_COPY_DIR_FH: action_lock_from_fh (unit, pck); break;
     case ACTION_CHANGE_MODE: action_change_mode (unit, pck); break;
     case ACTION_PARENT_FH: action_parent_fh (unit, pck); break;
     case ACTION_ADD_NOTIFY: action_add_notify (unit, pck); break;
     case ACTION_REMOVE_NOTIFY: action_remove_notify (unit, pck); break;
     
     /* unsupported packets */
     case ACTION_LOCK_RECORD:
     case ACTION_FREE_RECORD:
     case ACTION_EXAMINE_ALL:
     case ACTION_MAKE_LINK:
     case ACTION_READ_LINK:
     case ACTION_FORMAT:
	 return 0;
     default:
	write_log ("FILESYS: UNKNOWN PACKET %x\n", type);
		return 0;
    }
    return 1;
}

#ifdef UAE_FILESYS_THREADS
static void *filesys_thread (void *unit_v)
{
    UnitInfo *ui = (UnitInfo *)unit_v;
    uae_set_thread_priority (2);
    for (;;) {
	uae_u8 *pck;
	uae_u8 *msg;
	uae_u32 morelocks;

	pck = (uae_u8 *)read_comm_pipe_pvoid_blocking (ui->unit_pipe);
	msg = (uae_u8 *)read_comm_pipe_pvoid_blocking (ui->unit_pipe);
	morelocks = (uae_u32)read_comm_pipe_int_blocking (ui->unit_pipe);

	if (ui->reset_state == FS_GO_DOWN) {
	    if (pck != 0)
		continue;
	    /* Death message received. */
	    uae_sem_post (&ui->reset_sync_sem);
	    /* Die.  */
	    return 0;
	}

	put_long (get_long (morelocks), get_long (ui->self->locklist));
	put_long (ui->self->locklist, morelocks);
	if (! handle_packet (ui->self, pck)) {
	    PUT_PCK_RES1 (pck, DOS_FALSE);
	    PUT_PCK_RES2 (pck, ERROR_ACTION_NOT_KNOWN);
	}
	/* Mark the packet as processed for the list scan in the assembly code. */
	do_put_mem_long ((uae_u32 *)(msg + 4), -1);
	/* Acquire the message lock, so that we know we can safely send the
	 * message. */
	ui->self->cmds_sent++;
	/* The message is sent by our interrupt handler, so make sure an interrupt
	 * happens. */
	do_uae_int_requested();
	/* Send back the locks. */
	if (get_long (ui->self->locklist) != 0)
	    write_comm_pipe_int (ui->back_pipe, (int)(get_long (ui->self->locklist)), 0);
	put_long (ui->self->locklist, 0);
    }
    return 0;
}
#endif

/* Talk about spaghetti code... */
static uae_u32 REGPARAM2 filesys_handler (TrapContext *context)
{
    Unit *unit = find_unit (m68k_areg (&context->regs, 5));
    uaecptr packet_addr = m68k_dreg (&context->regs, 3);
    uaecptr message_addr = m68k_areg (&context->regs, 4);
    uae_u8 *pck;
    uae_u8 *msg;
    if (! valid_address (packet_addr, 36) || ! valid_address (message_addr, 14)) {
	write_log ("FILESYS: Bad address %x/%x passed for packet.\n", packet_addr, message_addr);
	goto error2;
    }
    pck = get_real_address (packet_addr);
    msg = get_real_address (message_addr);

    do_put_mem_long ((uae_u32 *)(msg + 4), -1);
    if (!unit || !unit->volume) {
	write_log ("FILESYS: was not initialized.\n");
	goto error;
    }
#ifdef UAE_FILESYS_THREADS
    {
	uae_u32 morelocks;
	if (!unit->ui.unit_pipe)
	    goto error;
	/* Get two more locks and hand them over to the other thread. */
	morelocks = get_long (m68k_areg (&context->regs, 3));
	put_long (m68k_areg (&context->regs, 3), get_long (get_long (morelocks)));
	put_long (get_long (morelocks), 0);

	/* The packet wasn't processed yet. */
	do_put_mem_long ((uae_u32 *)(msg + 4), 0);
	write_comm_pipe_pvoid (unit->ui.unit_pipe, (void *)pck, 0);
	write_comm_pipe_pvoid (unit->ui.unit_pipe, (void *)msg, 0);
	write_comm_pipe_int (unit->ui.unit_pipe, (int)morelocks, 1);
	/* Don't reply yet. */
	return 1;
    }
#endif

    if (! handle_packet (unit, pck)) {
	error:
	PUT_PCK_RES1 (pck, DOS_FALSE);
	PUT_PCK_RES2 (pck, ERROR_ACTION_NOT_KNOWN);
    }
    TRACE(("reply: %8lx, %ld\n", GET_PCK_RES1 (pck), GET_PCK_RES2 (pck)));

    error2:

    return 0;
}

static void init_filesys_diagentry (void)
{
    do_put_mem_long ((uae_u32 *)(filesysory + 0x2100), EXPANSION_explibname);
    do_put_mem_long ((uae_u32 *)(filesysory + 0x2104), filesys_configdev);
    do_put_mem_long ((uae_u32 *)(filesysory + 0x2108), EXPANSION_doslibname);
    do_put_mem_long ((uae_u32 *)(filesysory + 0x210c), current_mountinfo.num_units);
    native2amiga_startup();
    //cartridge_init();
}

void filesys_start_threads (void)
{
  UnitInfo *uip;
  int i;

  free_mountinfo (&current_mountinfo);
  dup_mountinfo (&options_mountinfo, &current_mountinfo);
  uip = current_mountinfo.ui;
  for (i = 0; i < current_mountinfo.num_units; i++) {
    UnitInfo *ui = &uip[i];
    ui->unit_pipe = 0;
    ui->back_pipe = 0;
    ui->reset_state = FS_STARTUP;
  	if (savestate_state != STATE_RESTORE) {
	    ui->startup = 0;
	    ui->self = 0;
    }
#ifdef UAE_FILESYS_THREADS
	  if (is_hardfile (&current_mountinfo, i) == FILESYS_VIRTUAL) {
	    ui->unit_pipe = (smp_comm_pipe *)xmalloc (sizeof (smp_comm_pipe));
	    ui->back_pipe = (smp_comm_pipe *)xmalloc (sizeof (smp_comm_pipe));
	    init_comm_pipe (uip[i].unit_pipe, 100, 3);
	    init_comm_pipe (uip[i].back_pipe, 100, 1);
	    uae_start_thread (filesys_thread, (void *)(uip + i), &uip[i].tid);
  	}
#endif
  	if (savestate_state == STATE_RESTORE)
	    startup_update_unit (uip->self, uip);
  }
}

void filesys_cleanup (void)
{
    free_mountinfo (&current_mountinfo);
}

void filesys_reset (void)
{
    Unit *u, *u1;

    /* We get called once from customreset at the beginning of the program
     * before filesys_start_threads has been called. Survive that.  */
    if (savestate_state == STATE_RESTORE)
	return;

    for (u = units; u; u = u1) {
	u1 = u->next;
	xfree (u);
    }
    unit_num = 0;
    units = 0;
}

static void free_all_ainos (Unit *u, a_inode *parent)
{
  a_inode *a;
  while (a = parent->child) {
      free_all_ainos (u, a);
      dispose_aino (u, &parent->child, a);
    }
}

void filesys_flush_cache (void)
{
}

void filesys_prepare_reset (void)
{
    UnitInfo *uip;
    Unit *u;
    int i;

    if (savestate_state == STATE_RESTORE)
    	return;
    uip = current_mountinfo.ui;
#ifdef UAE_FILESYS_THREADS
    for (i = 0; i < current_mountinfo.num_units; i++) {
	if (uip[i].unit_pipe != 0) {
	    uae_sem_init (&uip[i].reset_sync_sem, 0, 0);
	    uip[i].reset_state = FS_GO_DOWN;
	    /* send death message */
	    write_comm_pipe_int (uip[i].unit_pipe, 0, 0);
	    write_comm_pipe_int (uip[i].unit_pipe, 0, 0);
	    write_comm_pipe_int (uip[i].unit_pipe, 0, 1);
	    uae_sem_wait (&uip[i].reset_sync_sem);
	}
    }
#endif
    u = units;
    while (u != 0) {
	free_all_ainos (u, &u->rootnode);
	u->rootnode.next = u->rootnode.prev = &u->rootnode;
	u->aino_cache_size = 0;
	u = u->next;
    }
}

static uae_u32 REGPARAM2 filesys_diagentry (TrapContext *context)
{
    uaecptr resaddr = m68k_areg (&context->regs, 2) + 0x10;
    uaecptr start = resaddr;
    uaecptr residents, tmp;

    TRACE (("filesystem: diagentry called\n"));

    filesys_configdev = m68k_areg (&context->regs, 3);
    init_filesys_diagentry ();

    uae_sem_init (&singlethread_int_sem, 0, 1);
    if (ROM_hardfile_resid != 0) {
	/* Build a struct Resident. This will set up and initialize
	 * the uae.device */
	put_word (resaddr + 0x0, 0x4AFC);
	put_long (resaddr + 0x2, resaddr);
	put_long (resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	put_word (resaddr + 0xA, 0x8101); /* RTF_AUTOINIT|RTF_COLDSTART; Version 1 */
	put_word (resaddr + 0xC, 0x0305); /* NT_DEVICE; pri 05 */
	put_long (resaddr + 0xE, ROM_hardfile_resname);
	put_long (resaddr + 0x12, ROM_hardfile_resid);
	put_long (resaddr + 0x16, ROM_hardfile_init); /* calls filesys_init */
    }
    resaddr += 0x1A;
    tmp = resaddr;
    
    /* The good thing about this function is that it always gets called
     * when we boot. So we could put all sorts of stuff that wants to be done
     * here.
     * We can simply add more Resident structures here. Although the Amiga OS
     * only knows about the one at address DiagArea + 0x10, we scan for other
     * Resident structures and call InitResident() for them at the end of the
     * diag entry. */

    resaddr = scsidev_startup(resaddr);

    /* scan for Residents and return pointer to array of them */
    residents = resaddr;
    while (tmp < residents && tmp > start) {
	if (get_word (tmp) == 0x4AFC &&
	    get_long (tmp + 0x2) == tmp) {
	    put_word (resaddr, 0x227C);         /* movea.l #tmp,a1 */
	    put_long (resaddr + 2, tmp);
	    put_word (resaddr + 6, 0x7200);     /* moveq.l #0,d1 */
	    put_long (resaddr + 8, 0x4EAEFF9A); /* jsr -$66(a6) ; InitResident */
	    resaddr += 12;
	    tmp = get_long (tmp + 0x6);
	} else {
	    tmp++;
	}
    }
    /* call setup_exter */
    put_word (resaddr +  0, 0x2079);
    put_long (resaddr +  2, RTAREA_BASE + 28 + 4); /* move.l RTAREA_BASE+32,a0 */
    put_word (resaddr +  6, 0xd1fc);
    put_long (resaddr +  8, RTAREA_BASE + 8 + 4); /* add.l #RTAREA_BASE+12,a0 */
    put_word (resaddr + 12, 0x4e90); /* jsr (a0) */

    put_word (resaddr + 14, 0x7001); /* moveq.l #1,d0 */
    put_word (resaddr + 16, RTS);

    m68k_areg (&context->regs, 0) = residents;
    return 1;
}

/* don't forget filesys.asm! */
#define PP_MAXSIZE 4 * 96
#define PP_FSSIZE 400
#define PP_FSPTR 404
#define PP_FSRES 408
#define PP_EXPLIB 412
#define PP_FSHDSTART 416

static uae_u32 REGPARAM2 filesys_dev_bootfilesys (TrapContext *context)
{
    uaecptr devicenode = m68k_areg (&context->regs, 3);
    uaecptr parmpacket = m68k_areg (&context->regs, 1);
    uaecptr fsres = get_long (parmpacket + PP_FSRES);
    uaecptr fsnode;
    uae_u32 dostype, dostype2;
    UnitInfo *uip = current_mountinfo.ui;
    int no = m68k_dreg (&context->regs, 6);
    int unit_no = no & 65535;
    int type = is_hardfile (&current_mountinfo, unit_no);

    if (type == FILESYS_VIRTUAL)
	return 0;
    dostype = get_long (parmpacket + 80);
    fsnode = get_long (fsres + 18);
    while (get_long (fsnode)) {
	dostype2 = get_long (fsnode + 14);
	if (dostype2 == dostype) {
	    if (get_long (fsnode + 22) & (1 << 7)) {
		put_long (devicenode + 32, get_long (fsnode + 54)); /* dn_SegList */
		put_long (devicenode + 36, -1); /* dn_GlobalVec */
	    }
	    return 1;
	}
	fsnode = get_long (fsnode);
    }
    return 0;
}

/* Remember a pointer AmigaOS gave us so we can later use it to identify
 * which unit a given startup message belongs to.  */
static uae_u32 REGPARAM2 filesys_dev_remember (TrapContext *context)
{
    int no = m68k_dreg (&context->regs, 6);
    int unit_no = no & 65535;
    int sub_no = no >> 16;
    UnitInfo *uip = &current_mountinfo.ui[unit_no];
    uaecptr devicenode = m68k_areg (&context->regs, 3);

    if (m68k_dreg (&context->regs, 3) >= 0)
      uip->startup = get_long (devicenode + 28);
    return devicenode;
}

static char *device_dupfix (uaecptr expbase, char *devname)
{
    uaecptr bnode, dnode, name;
    int len, i, modified;
    char dname[256], newname[256];

    strcpy (newname, devname);
    modified = 1;
    while (modified) {
	modified = 0;
	bnode = get_long (expbase + 74); /* expansion.library bootnode list */
	while (get_long (bnode)) {
	    dnode = get_long (bnode + 16); /* device node */
	    name = get_long (dnode + 40) << 2; /* device name BSTR */
	    len = get_byte(name);
	    for (i = 0; i < len; i++)
		dname[i] = get_byte (name + 1 + i);
	    dname[len] = 0;
	    for (;;) {
		if (!strcmpi (newname, dname)) {
		    if (strlen (newname) > 2 && newname[strlen (newname) - 2] == '_') {
			newname[strlen (newname) - 1]++;
		    } else {
			strcat (newname, "_0");
		    }
		    modified = 1;
		} else {
		    break;
		}
	    }
	    bnode = get_long (bnode);
	}
    }
    return strdup (newname);
}

static void get_new_device (int type, uaecptr parmpacket, char **devname, uaecptr *devname_amiga, int unit_no)
{
    char buffer[80];

    if (*devname == 0 || strlen(*devname) == 0) {
        sprintf (buffer, "DH%d", unit_no);
    } else {
	    strcpy (buffer, *devname);
    }
    *devname_amiga = ds (device_dupfix (get_long (parmpacket + PP_EXPLIB), buffer));
    if (type == FILESYS_VIRTUAL)
	write_log ("FS: mounted virtual unit %s (%s)\n", buffer, current_mountinfo.ui[unit_no].rootdir);
    else
	write_log ("FS: mounted HDF unit %s (%04.4x-%08.8x, %s)\n", buffer,
	    (uae_u32)(current_mountinfo.ui[unit_no].hf.size >> 32),
	    (uae_u32)(current_mountinfo.ui[unit_no].hf.size),
	    current_mountinfo.ui[unit_no].rootdir);
}

/* Fill in per-unit fields of a parampacket */
static uae_u32 REGPARAM2 filesys_dev_storeinfo (TrapContext *context)
{
    UnitInfo *uip = current_mountinfo.ui;
    int no = m68k_dreg (&context->regs, 6);
    int unit_no = no & 65535;
    int sub_no = no >> 16;
    int type = is_hardfile (&current_mountinfo, unit_no);
    uaecptr parmpacket = m68k_areg (&context->regs, 0);
    
    if (sub_no)
    	return -2;
    
    write_log("Mounting uaehf.device %d (%d):\n", unit_no, sub_no);
    get_new_device (type, parmpacket, &uip[unit_no].devname, &uip[unit_no].devname_amiga, unit_no);
    uip[unit_no].devno = unit_no;
    put_long (parmpacket, uip[unit_no].devname_amiga);
    put_long (parmpacket + 4, type != FILESYS_VIRTUAL ? ROM_hardfile_resname : fsdevname);
    put_long (parmpacket + 8, uip[unit_no].devno);
    put_long (parmpacket + 12, 0); /* Device flags */
    put_long (parmpacket + 16, 16); /* Env. size */
    put_long (parmpacket + 20, uip[unit_no].hf.blocksize >> 2); /* longwords per block */
    put_long (parmpacket + 24, 0); /* unused */
    put_long (parmpacket + 28, uip[unit_no].hf.surfaces); /* heads */
    put_long (parmpacket + 32, 1); /* sectors per block */
    put_long (parmpacket + 36, uip[unit_no].hf.secspertrack); /* sectors per track */
    put_long (parmpacket + 40, uip[unit_no].hf.reservedblocks); /* reserved blocks */
    put_long (parmpacket + 44, 0); /* unused */
    put_long (parmpacket + 48, 0); /* interleave */
    put_long (parmpacket + 52, 0); /* lowCyl */
    put_long (parmpacket + 56, uip[unit_no].hf.nrcyls <= 0 ? 0 : uip[unit_no].hf.nrcyls - 1); /* hiCyl */
    put_long (parmpacket + 60, 50); /* Number of buffers */
    put_long (parmpacket + 64, 0); /* Buffer mem type */
    put_long (parmpacket + 68, 0x7FFFFFFF); /* largest transfer */
    put_long (parmpacket + 72, ~1); /* addMask (?) */
    put_long (parmpacket + 76, uip[unit_no].bootpri); /* bootPri */
    put_long (parmpacket + 80, 0x444f5300); /* DOS\0 */
    return type;
}

void filesys_install (void)
{
    uaecptr loop;

    TRACE (("Installing filesystem\n"));

    ROM_filesys_resname = ds("UAEunixfs.resource");
    ROM_filesys_resid = ds("UAE unixfs 0.4");

    fsdevname = ds ("uae.device"); /* does not really exist */

    ROM_filesys_diagentry = here();
    calltrap (deftrap2 (filesys_diagentry, 0, "filesys_diagentry"));
    dw(0x4ED0); /* JMP (a0) - jump to code that inits Residents */
    
    loop = here ();
    
    org (RTAREA_BASE + 0xFF18);
    calltrap (deftrap2 (filesys_dev_bootfilesys, 0, "filesys_dev_bootfilesys"));
    dw (RTS);
    
    /* Special trap for the assembly make_dev routine */
    org (RTAREA_BASE + 0xFF20);
    calltrap (deftrap2 (filesys_dev_remember, 0, "filesys_dev_remember"));
    dw (RTS);

    org (RTAREA_BASE + 0xFF28);
    calltrap (deftrap2 (filesys_dev_storeinfo, 0, "filesys_dev_storeinfo"));
    dw (RTS);

    org (RTAREA_BASE + 0xFF30);
    calltrap (deftrap2 (filesys_handler, 0, "filesys_handler"));
    dw (RTS);

    org (RTAREA_BASE + 0xFF40);
    calltrap (deftrap2 (startup_handler, 0, "startup_handler"));
    dw (RTS);

    org (RTAREA_BASE + 0xFF50);
    calltrap (deftrap2 (exter_int_helper, 0, "exter_int_helper"));
    dw (RTS);

    org (loop);
}

void filesys_install_code (void)
{
    align(4);
    
    /* The last offset comes from the code itself, look for it near the top. */
    EXPANSION_bootcode = here () + 8 + 0x1c + 4;
    /* Ouch. Make sure this is _always_ a multiple of two bytes. */
    filesys_initcode = here() + 8 + 0x30 + 4;
    
    #include "filesys_bootrom.c"
}

static uae_u8 *restore_filesys_virtual (UnitInfo *ui, uae_u8 *src)
{
    Unit *u = startup_create_unit (ui);
    int cnt;
 
    u->a_uniq = restore_u64 ();
    u->key_uniq = restore_u64 ();
    u->dosbase = restore_u32 ();
    u->volume = restore_u32 ();
    u->port = restore_u32 ();
    u->locklist = restore_u32 ();
    u->dummy_message = restore_u32 ();
    u->cmds_sent = restore_u64 ();
    u->cmds_complete = restore_u64 ();
    u->cmds_acked = restore_u64 ();
    cnt = restore_u32 ();
    while (cnt-- > 0) {
	restore_u64 ();
	restore_u32 ();
	restore_u32 ();
	restore_u32 ();
	xfree (restore_string ());
    }
    return src;
}

static uae_u8 *save_filesys_virtual (UnitInfo *ui, uae_u8 *dst)
{
    Unit *u = ui->self;
    Key *k;
    int cnt;

    save_u64 (u->a_uniq);
    save_u64 (u->key_uniq);
    save_u32 (u->dosbase);
    save_u32 (u->volume);
    save_u32 (u->port);
    save_u32 (u->locklist);
    save_u32 (u->dummy_message);
    save_u64 (u->cmds_sent);
    save_u64 (u->cmds_complete);
    save_u64 (u->cmds_acked);
    cnt = 0;
    for (k = u->keys; k; k = k->next)
	cnt++;
    save_u32 (cnt);
    for (k = u->keys; k; k = k->next) {
	save_u64 (k->uniq);
	save_u32 (k->file_pos);
	save_u32 (k->createmode);
	save_u32 (k->dosmode);
	save_string (k->aino->nname);
    }
    return dst;
}

uae_u8 *save_filesys (int num, int *len)
{
    uae_u8 *dstbak, *dst;
    UnitInfo *ui;
    int type = is_hardfile (&current_mountinfo, num);

    dstbak = dst = (uae_u8 *) malloc (10000);
    ui = &current_mountinfo.ui[num];
    save_u32 (1); /* version */
    save_u32 (ui->devno);
    save_u16 (type);
    save_string (ui->rootdir);
    save_string (ui->devname);
    save_string (ui->volname);
    save_string (/*ui->filesysdir*/ "");
    save_u8 (ui->bootpri);
    save_u8 (ui->readonly);
    save_u32 (ui->startup);
    save_u32 (filesys_configdev);
    if (type == FILESYS_VIRTUAL)
	dst = save_filesys_virtual (ui, dst);
    *len = dst - dstbak;
    return dstbak;
}

uae_u8 *restore_filesys (uae_u8 *src)
{
    int type, devno;
    UnitInfo *ui;
    char *devname = 0, *volname = 0, *rootdir = 0, *filesysdir = 0;
    int bootpri, readonly;

    if (restore_u32 () != 1)
	return src;
    devno = restore_u32 ();
    if (devno >= current_mountinfo.num_units)
	return src;
    type = restore_u16 ();
    rootdir = restore_string ();
    devname = restore_string ();
    volname = restore_string ();
    filesysdir = restore_string ();
    bootpri = restore_u8 ();
    readonly = restore_u8 ();
    if (set_filesys_unit (&current_mountinfo, devno, devname, volname, rootdir, readonly,
	0, 0, 0, 0, bootpri, filesysdir[0] ? filesysdir : NULL, 0)) {
	write_log ("filesys '%s' failed to restore\n", rootdir);
	goto end;
    }
    ui = &current_mountinfo.ui[devno];
    ui->startup = restore_u32 ();
    filesys_configdev = restore_u32 ();
    if (type == FILESYS_VIRTUAL)
	src = restore_filesys_virtual (ui, src);
end:
    xfree (rootdir);
    xfree (devname);
    xfree (volname);
    xfree (filesysdir);
    return src;
}

#ifdef WIN32
#include <windows.h>
#include <sys/timeb.h>

static DWORD lasterror;

int truncate (const char *name, long int len)
{
    HANDLE hFile;
    BOOL bResult = FALSE;
    int result = -1;

    if ((hFile = CreateFile (name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
	if (SetFilePointer (hFile, len, NULL, FILE_BEGIN) == (DWORD)len) {
	    if (SetEndOfFile (hFile) == TRUE)
		result = 0;
        } else {
	    write_log ("SetFilePointer() failure for %s to posn %d\n", name, len);
	}
	CloseHandle (hFile);
    } else {
	write_log( "CreateFile() failed to open %s\n", name );
    }

    if (result == -1)
	lasterror = GetLastError ();
    return result;
}
#endif
