/*
* UAE - The Un*x Amiga Emulator
*
* lowlevel cd device glue, scsi emulator
*
* Copyright 2009-2013 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "include/memory.h"

#include "blkdev.h"
#include "scsidev.h"
#include "savestate.h"
#include "crc32.h"
#include "threaddep/thread.h"
#include "execio.h"
#include "zfile.h"
#include "scsi.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif
int log_scsiemu = 0;

#define PRE_INSERT_DELAY (3 * (currprefs.ntscmode ? 60 : 50))

struct blkdevstate
{
	bool scsiemu;
	int type;
	struct device_functions *device_func;
	int isopen;
	int waspaused;
	int delayed;
	uae_sem_t sema;
	int sema_cnt;
	int current_pos;
	int play_end_pos;
	uae_u8 play_qcode[SUBQ_SIZE];
	TCHAR newimagefile[256];
	int imagechangetime;
	bool cdimagefileinuse;
	int wasopen;
	bool mediawaschanged;
};

struct blkdevstate state[MAX_TOTAL_SCSI_DEVICES];

#if 0
static int scsiemu[MAX_TOTAL_SCSI_DEVICES];

static struct device_functions *device_func[MAX_TOTAL_SCSI_DEVICES];
static int openlist[MAX_TOTAL_SCSI_DEVICES];
static int waspaused[MAX_TOTAL_SCSI_DEVICES];
static int delayed[MAX_TOTAL_SCSI_DEVICES];
static uae_sem_t unitsem[MAX_TOTAL_SCSI_DEVICES];
static int unitsem_cnt[MAX_TOTAL_SCSI_DEVICES];

static int play_end_pos[MAX_TOTAL_SCSI_DEVICES];
static uae_u8 play_qcode[MAX_TOTAL_SCSI_DEVICES][SUBQ_SIZE];

static TCHAR newimagefiles[MAX_TOTAL_SCSI_DEVICES][256];
static int imagechangetime[MAX_TOTAL_SCSI_DEVICES];
static bool cdimagefileinuse[MAX_TOTAL_SCSI_DEVICES];
static int wasopen[MAX_TOTAL_SCSI_DEVICES];
#endif

static bool dev_init;

/* convert minutes, seconds and frames -> logical sector number */
int msf2lsn (int msf)
{
	int sector = (((msf >> 16) & 0xff) * 60 * 75 + ((msf >> 8) & 0xff) * 75 + ((msf >> 0) & 0xff));
	sector -= 150;
	return sector;
}

/* convert logical sector number -> minutes, seconds and frames */
int lsn2msf (int sectors)
{
	int msf;
	sectors += 150;
	msf = (sectors / (75 * 60)) << 16;
	msf |= ((sectors / 75) % 60) << 8;
	msf |= (sectors % 75) << 0;
	return msf;
}

uae_u8 frombcd (uae_u8 v)
{
	return (v >> 4) * 10 + (v & 15);
}
uae_u8 tobcd (uae_u8 v)
{
	return ((v / 10) << 4) | (v % 10);
}
int fromlongbcd (uae_u8 *p)
{
	return (frombcd (p[0]) << 16) | (frombcd (p[1]) << 8) | (frombcd (p[2])  << 0);
}
void tolongbcd (uae_u8 *p, int v)
{
	p[0] = tobcd ((v >> 16) & 0xff);
	p[1] = tobcd ((v >> 8) & 0xff);
	p[2] = tobcd ((v >> 0) & 0xff);
}

static struct cd_toc *gettoc (struct cd_toc_head *th, int block)
{
	for (int i = th->first_track_offset + 1; i <= th->last_track_offset; i++) {
		struct cd_toc *t = &th->toc[i];
		if (block < t->paddress)
				return t - 1;
	}
	return &th->toc[th->last_track_offset];
}

int isaudiotrack (struct cd_toc_head *th, int block)
{
	struct cd_toc *t = gettoc (th, block);
	if (!t)
		return 0;
	return (t->control & 0x0c) != 4;
}
int isdatatrack (struct cd_toc_head *th, int block)
{
	return !isaudiotrack (th, block);
}

static int cdscsidevicetype[MAX_TOTAL_SCSI_DEVICES];

#ifdef _WIN32

#include "od-win32/win32.h"

extern struct device_functions devicefunc_win32_spti;
extern struct device_functions devicefunc_win32_ioctl;

#endif

extern struct device_functions devicefunc_cdimage;

static struct device_functions *devicetable[] = {
	NULL,
	&devicefunc_cdimage,
#ifdef _WIN32
	&devicefunc_win32_ioctl,
	&devicefunc_win32_spti,
#endif
	NULL
};
static int driver_installed[6];

static void install_driver (int flags)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct blkdevstate *st = &state[i];
		st->scsiemu = false;
		st->type = -1;
		st->device_func = NULL;
	}
	if (flags > 0) {
		state[0].device_func = devicetable[flags];
		state[0].scsiemu = true;
	} else {
		for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
			struct blkdevstate *st = &state[i];
			st->scsiemu = false;
			st->device_func = NULL;
			switch (cdscsidevicetype[i])
			{
				case SCSI_UNIT_IMAGE:
				  st->device_func = devicetable[SCSI_UNIT_IMAGE];
				  st->scsiemu = true;
				  break;
			}
		}
	}

	for (int j = 1; devicetable[j]; j++) {
		if (!driver_installed[j]) {
			for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
				struct blkdevstate *st = &state[i];
				if (st->device_func == devicetable[j]) {
					int ok = st->device_func->openbus (0);
					driver_installed[j] = 1;
					write_log (_T("%s driver installed, ok=%d\n"), st->device_func->name, ok);
					break;
				}
			}
		}
	}

}

void blkdev_default_prefs (struct uae_prefs *p)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		p->cdslots[i].name[0] = 0;
		p->cdslots[i].inuse = false;
		p->cdslots[i].type = SCSI_UNIT_DEFAULT;
		cdscsidevicetype[i] = SCSI_UNIT_DEFAULT;
	}
}

void blkdev_fix_prefs (struct uae_prefs *p)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		cdscsidevicetype[i] = p->cdslots[i].type;
		if (p->cdslots[i].inuse == false && p->cdslots[i].name[0] && p->cdslots[i].type != SCSI_UNIT_DISABLED)
			p->cdslots[i].inuse = true;
	}

	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		if (cdscsidevicetype[i] != SCSI_UNIT_DEFAULT)
			continue;
		if (p->cdslots[i].inuse || p->cdslots[i].name[0]) {
			cdscsidevicetype[i] = SCSI_UNIT_IMAGE;
		}
	}

}

static bool getsem (int unitnum, bool dowait)
{
	struct blkdevstate *st = &state[unitnum];
	if (st->sema == NULL)
		uae_sem_init (&st->sema, 0, 1);
	bool gotit = false;
	if (dowait) {
		uae_sem_wait (&st->sema);
		gotit = true;
	} else {
		gotit = uae_sem_trywait (&st->sema) == 0;
	}
	if (gotit)
		st->sema_cnt++;
	if (st->sema_cnt > 1)
		write_log (_T("CD: unitsem%d acquire mismatch! cnt=%d\n"), unitnum, st->sema_cnt);
	return gotit;
}
static bool getsem (int unitnum)
{
	return getsem (unitnum, false);
}
static void freesem (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	st->sema_cnt--;
	if (st->sema_cnt < 0)
		write_log (_T("CD: unitsem%d release mismatch! cnt=%d\n"), unitnum, st->sema_cnt);
	uae_sem_post (&st->sema);
}
static void sys_command_close_internal (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	getsem (unitnum, true);
	st->waspaused = 0;
	if (st->isopen <= 0)
		write_log (_T("BUG unit %d close: opencnt=%d!\n"), unitnum, st->isopen);
	if (st->device_func) {
		state[unitnum].device_func->closedev (unitnum);
		if (st->isopen > 0)
			st->isopen--;
	}
	freesem (unitnum);
	if (st->isopen == 0) {
		uae_sem_destroy (&st->sema);
		st->sema = NULL;
	}
}

static int sys_command_open_internal (int unitnum, const TCHAR *ident, cd_standard_unit csu)
{
	struct blkdevstate *st = &state[unitnum];
	int ret = 0;
	if (st->sema == NULL)
		uae_sem_init (&st->sema, 0, 1);
	getsem (unitnum, true);
	if (st->isopen)
		write_log (_T("BUG unit %d open: opencnt=%d!\n"), unitnum, st->isopen);
	if (st->device_func) {
		ret = state[unitnum].device_func->opendev (unitnum, ident, csu != CD_STANDARD_UNIT_DEFAULT);
		if (ret)
			st->isopen++;
	}
	freesem (unitnum);
	return ret;
}

static int getunitinfo (int unitnum, int drive, cd_standard_unit csu, int *isaudio)
{
	struct device_info di;
	if (sys_command_info (unitnum, &di, 0)) {
		write_log (_T("Scanning drive %s: "), di.label);
		if (di.media_inserted) {
			if (isaudiotrack (&di.toc, 0)) {
				if (*isaudio == 0)
					*isaudio = drive;
				write_log (_T("CDA"));
			}
			uae_u8 buffer[2048];
			if (sys_command_cd_read (unitnum, buffer, 16, 1)) {
				if (!memcmp (buffer + 8, "CDTV", 4) || !memcmp (buffer + 8, "CD32", 4) || !memcmp (buffer + 8, "COMM", 4)) {
					uae_u32 crc;
					write_log (_T("CD32 or CDTV"));
					if (sys_command_cd_read (unitnum, buffer, 21, 1)) {
						crc = get_crc32 (buffer, sizeof buffer);
						if (crc == 0xe56c340f) {
							write_log (_T(" [CD32.TM]"));
							if (csu == CD_STANDARD_UNIT_CD32) {
								write_log (_T("\n"));
								return 1;
							}
						}
					}
					if (csu == CD_STANDARD_UNIT_CDTV || csu == CD_STANDARD_UNIT_CD32) {
						write_log (_T("\n"));
						return 1;
					}
				}
			}
		} else {
			write_log (_T("no media"));
		}
	}
	write_log (_T("\n"));
	return 0;
}

static int get_standard_cd_unit2 (struct uae_prefs *p, cd_standard_unit csu)
{
	int unitnum = 0;
	int isaudio = 0;
	if (p->cdslots[unitnum].name[0] || p->cdslots[unitnum].inuse) {
		if (p->cdslots[unitnum].name[0]) {
			device_func_init (SCSI_UNIT_IMAGE);
			if (!sys_command_open_internal (unitnum, p->cdslots[unitnum].name, csu))
				goto fallback;
		} else {
			goto fallback;
		}
		return unitnum;
	}

fallback:
	device_func_init (SCSI_UNIT_IMAGE);
	if (!sys_command_open_internal (unitnum, _T(""), csu)) {
		write_log (_T("image mounter failed to open as empty!?\n"));
		return -1;
	}
	return unitnum;
}

int get_standard_cd_unit (cd_standard_unit csu)
{
	int unitnum = get_standard_cd_unit2 (&currprefs, csu);
	if (unitnum < 0)
		return -1;
	struct blkdevstate *st = &state[unitnum];
#ifdef RETROPLATFORM
	rp_cd_device_enable (unitnum, true);
#endif
	st->delayed = 0;
	if (currprefs.cdslots[unitnum].delayed) {
		st->delayed = PRE_INSERT_DELAY;
	}
	return unitnum;
}

void close_standard_cd_unit (int unitnum)
{
	sys_command_close (unitnum);
}

int sys_command_isopen (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	return st->isopen;
}

int sys_command_open (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	blkdev_fix_prefs (&currprefs);
	if (!dev_init) {
		device_func_init (0);
		dev_init = true;
	}

	if (st->isopen) {
		st->isopen++;
		return -1;
	}
	st->waspaused = 0;
	int v = sys_command_open_internal (unitnum, currprefs.cdslots[unitnum].name[0] ? currprefs.cdslots[unitnum].name : NULL, CD_STANDARD_UNIT_DEFAULT);
	if (!v)
		return 0;
#ifdef RETROPLATFORM
	rp_cd_device_enable (unitnum, true);
#endif
	return v;
}

void sys_command_close (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	if (st->isopen > 1) {
		st->isopen--;
		return;
	}
#ifdef RETROPLATFORM
	rp_cd_device_enable (unitnum, false);
#endif
	sys_command_close_internal (unitnum);
}

void blkdev_cd_change (int unitnum, const TCHAR *name)
{
	struct device_info di;
	sys_command_info (unitnum, &di, 1);
#ifdef RETROPLATFORM
	rp_cd_image_change (unitnum, name);
#endif
}

void device_func_reset (void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct blkdevstate *st = &state[i];
		st->wasopen = 0;
		st->waspaused = false;
		st->mediawaschanged = false;
		st->imagechangetime = 0;
		st->cdimagefileinuse = false;
		st->newimagefile[0] = 0;
	}
}

int device_func_init (int flags)
{
	blkdev_fix_prefs (&currprefs);
	install_driver (flags);
	return 1;
}

bool blkdev_get_info (struct uae_prefs *p, int unitnum, struct device_info *di)
{
	bool open = true, opened = false, ok = false;
	struct blkdevstate *st = &state[unitnum];
	if (!st->isopen) {
		blkdev_fix_prefs (p);
		install_driver (0);
		opened = true;
		open = sys_command_open_internal (unitnum, p->cdslots[unitnum].name[0] ? p->cdslots[unitnum].name : NULL, CD_STANDARD_UNIT_DEFAULT) != 0;
	}
	if (open) {
		ok = sys_command_info (unitnum, di, true) != 0;
	}
	if (open && opened)
		sys_command_close_internal (unitnum);
	return ok;
}

void blkdev_entergui (void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct blkdevstate *st = &state[i];
		st->waspaused = 0;
		struct device_info di;
		if (sys_command_info (i, &di, 1)) {
			if (sys_command_cd_pause (i, 1) == 0)
				st->waspaused = 1;
		}
	}
}
void blkdev_exitgui (void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct blkdevstate *st = &state[i];
		if (st->waspaused) {
			struct device_info di;
			if (sys_command_info (i, &di, 1)) {
				sys_command_cd_pause (i, 0);
			}
		}
		st->waspaused = 0;
	}
}

void check_prefs_changed_cd (void)
{
	currprefs.sound_volume_cd = changed_prefs.sound_volume_cd;
}

static void check_changes (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	bool changed = false;
	bool gotsem = false;

	if (st->device_func == NULL)
		return;

	if (st->delayed) {
		st->delayed--;
		if (st->delayed == 0)
			write_log (_T("CD: startup delayed insert '%s'\n"), currprefs.cdslots[unitnum].name[0] ? currprefs.cdslots[unitnum].name : _T("<EMPTY>"));
		return;
	}

	if (_tcscmp (changed_prefs.cdslots[unitnum].name, currprefs.cdslots[unitnum].name) != 0)
		changed = true;
	if (!changed && changed_prefs.cdslots[unitnum].name[0] == 0 && changed_prefs.cdslots[unitnum].inuse != currprefs.cdslots[unitnum].inuse)
		changed = true;

	if (changed) {
		bool wasimage = currprefs.cdslots[unitnum].name[0] != 0;
		if (st->sema)
			gotsem = getsem (unitnum, true);
		st->cdimagefileinuse = changed_prefs.cdslots[unitnum].inuse;
		_tcscpy (st->newimagefile, changed_prefs.cdslots[unitnum].name);
		changed_prefs.cdslots[unitnum].name[0] = currprefs.cdslots[unitnum].name[0] = 0;
		currprefs.cdslots[unitnum].inuse = changed_prefs.cdslots[unitnum].inuse;
		int pollmode = 0;
		st->imagechangetime = 3 * 50;
		struct device_info di;
		st->device_func->info (unitnum, &di, 0, -1);
		if (st->wasopen >= 0)
			st->wasopen = di.open ? 1 : 0;
		if (st->wasopen) {
			st->device_func->closedev (unitnum);
			st->wasopen = -1;
		}
		write_log (_T("CD: eject (%s) open=%d\n"), pollmode ? _T("slow") : _T("fast"), st->wasopen ? 1 : 0);
#ifdef RETROPLATFORM
		rp_cd_image_change (unitnum, NULL); 
#endif
		if (gotsem) {
			freesem (unitnum);
			gotsem = false;
		}
	}
	if (st->imagechangetime == 0)
		return;
	st->imagechangetime--;
	if (st->imagechangetime > 0)
		return;
	if (st->sema)
		gotsem = getsem (unitnum, true);
	_tcscpy (currprefs.cdslots[unitnum].name, st->newimagefile);
	_tcscpy (changed_prefs.cdslots[unitnum].name, st->newimagefile);
	currprefs.cdslots[unitnum].inuse = changed_prefs.cdslots[unitnum].inuse = st->cdimagefileinuse;
	st->newimagefile[0] = 0;
	write_log (_T("CD: delayed insert '%s' (open=%d,unit=%d)\n"), currprefs.cdslots[unitnum].name[0] ? currprefs.cdslots[unitnum].name : _T("<EMPTY>"), st->wasopen ? 1 : 0, unitnum);
	device_func_init (0);
	if (st->wasopen) {
		if (!st->device_func->opendev (unitnum, currprefs.cdslots[unitnum].name, 0)) {
			write_log (_T("-> device open failed\n"));
			st->wasopen = 0;
		} else {
			st->wasopen = 1;
			write_log (_T("-> device reopened\n"));
		}
	}
	st->mediawaschanged = true;
#ifdef RETROPLATFORM
	rp_cd_image_change (unitnum, currprefs.cdslots[unitnum].name);
#endif
	if (gotsem) {
		freesem (unitnum);
		gotsem = false;
	}
}

void blkdev_vsync (void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++)
		check_changes (i);
}

static int do_scsi (int unitnum, uae_u8 *cmd, int cmdlen)
{
	uae_u8 *p = state[unitnum].device_func->exec_out (unitnum, cmd, cmdlen);
	return p != NULL;
}
static int do_scsi (int unitnum, uae_u8 *cmd, int cmdlen, uae_u8 *out, int outsize)
{
	uae_u8 *p = state[unitnum].device_func->exec_in (unitnum, cmd, cmdlen, &outsize);
	if (p)
		memcpy (out, p, outsize);
	return p != NULL;
}

static int failunit (int unitnum)
{
	if (unitnum < 0 || unitnum >= MAX_TOTAL_SCSI_DEVICES)
		return 1;
	if (state[unitnum].device_func == NULL)
		return 1;
	return 0;
}

static int audiostatus (int unitnum)
{
	if (!getsem (unitnum))
		return 0;
	uae_u8 cmd[10] = {0x42,2,0x40,1,0,0,0,(uae_u8)(DEVICE_SCSI_BUFSIZE>>8),(uae_u8)(DEVICE_SCSI_BUFSIZE&0xff),0};
	uae_u8 *p = state[unitnum].device_func->exec_in (unitnum, cmd, sizeof (cmd), 0);
	freesem (unitnum);
	if (!p)
		return 0;
	return p[1];
}

/* pause/unpause CD audio */
int sys_command_cd_pause (int unitnum, int paused)
{
	if (failunit (unitnum))
		return -1;
	if (!getsem (unitnum))
		return 0;
	int v;
	if (state[unitnum].device_func->pause == NULL) {
		int as = audiostatus (unitnum);
		uae_u8 cmd[10] = {0x4b,0,0,0,0,0,0,0,paused?0:1,0};
		do_scsi (unitnum, cmd, sizeof cmd);
		v = as == AUDIO_STATUS_PAUSED;
	} else {
		v = state[unitnum].device_func->pause (unitnum, paused);
	}
	freesem (unitnum);
	return v;
}

/* stop CD audio */
void sys_command_cd_stop (int unitnum)
{
	if (failunit (unitnum))
		return;
	if (!getsem (unitnum))
		return;
	if (state[unitnum].device_func->stop == NULL) {
		int as = audiostatus (unitnum);
		uae_u8 cmd[6] = {0x4e,0,0,0,0,0};
		do_scsi (unitnum, cmd, sizeof cmd);
	} else {
		state[unitnum].device_func->stop (unitnum);
	}
	freesem (unitnum);
}

/* play CD audio */
int sys_command_cd_play (int unitnum, int startlsn, int endlsn, int scan)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	state[unitnum].play_end_pos = endlsn;
	if (state[unitnum].device_func->play == NULL) {
		uae_u8 cmd[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int startmsf = lsn2msf (startlsn);
		int endmsf = lsn2msf (endlsn);
		cmd[0] = 0x47;
		cmd[3] = (uae_u8)(startmsf >> 16);
		cmd[4] = (uae_u8)(startmsf >> 8);
		cmd[5] = (uae_u8)(startmsf >> 0);
		cmd[6] = (uae_u8)(endmsf >> 16);
		cmd[7] = (uae_u8)(endmsf >> 8);
		cmd[8] = (uae_u8)(endmsf >> 0);
		v = do_scsi (unitnum, cmd, sizeof cmd) ? 0 : 1;
	} else {
		v = state[unitnum].device_func->play (unitnum, startlsn, endlsn, scan, NULL, NULL);
	}
	freesem (unitnum);
	return v;
}

/* play CD audio with subchannels */
int sys_command_cd_play (int unitnum, int startlsn, int endlsn, int scan, play_status_callback statusfunc, play_subchannel_callback subfunc)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->play == NULL)
		v = sys_command_cd_play (unitnum, startlsn, endlsn, scan);
	else
		v = state[unitnum].device_func->play (unitnum, startlsn, endlsn, scan, statusfunc, subfunc);
	freesem (unitnum);
	return v;
}

/* set CD audio volume */
uae_u32 sys_command_cd_volume (int unitnum, uae_u16 volume_left, uae_u16 volume_right)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->volume == NULL)
		v = -1;
	else
		v = state[unitnum].device_func->volume (unitnum, volume_left, volume_right);
	freesem (unitnum);
	return v;
}

/* read qcode */
int sys_command_cd_qcode (int unitnum, uae_u8 *buf)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->qcode == NULL) {
		uae_u8 cmd[10] = {0x42,2,0x40,1,0,0,0,(uae_u8)(SUBQ_SIZE>>8),(uae_u8)(SUBQ_SIZE&0xff),0};
		v = do_scsi (unitnum, cmd, sizeof cmd, buf, SUBQ_SIZE);
	} else {
		v = state[unitnum].device_func->qcode (unitnum, buf, -1);
	}
	freesem (unitnum);
	return v;
};

/* read table of contents */
int sys_command_cd_toc (int unitnum, struct cd_toc_head *toc)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->toc == NULL) {
		uae_u8 buf[4 + 8 * 103];
		int size = sizeof buf;
		uae_u8 cmd [10] = { 0x43,0,2,0,0,0,0,(uae_u8)(size>>8),(uae_u8)(size&0xff),0};
		if (do_scsi (unitnum, cmd, sizeof cmd, buf, size)) {
			// toc parse to do
			v = 0;
		}
		v = 0;
	} else {
		v = state[unitnum].device_func->toc (unitnum, toc);
	}
	freesem (unitnum);
	return v;
}

/* read one cd sector */
int sys_command_cd_read (int unitnum, uae_u8 *data, int block, int size)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->read == NULL) {
		uae_u8 cmd1[12] = { 0x28, 0, block >> 24, block >> 16, block >> 8, block >> 0, 0, size >> 8, size >> 0, 0, 0, 0 };
		v = do_scsi (unitnum, cmd1, sizeof cmd1, data, size * 2048);
#if 0
		if (!v) {
			uae_u8 cmd2[12] = { 0xbe, 0, block >> 24, block >> 16, block >> 8, block >> 0, size >> 16, size >> 8, size >> 0, 0x10, 0, 0 };
			v = do_scsi (unitnum, cmd2, sizeof cmd2, data, size * 2048);
		}
#endif
	} else {
		v = state[unitnum].device_func->read (unitnum, data, block, size);
	}
	freesem (unitnum);
	return v;
}
int sys_command_cd_rawread (int unitnum, uae_u8 *data, int block, int size, int sectorsize)
{
	int v;
	if (failunit (unitnum))
		return -1;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->rawread == NULL) {
		uae_u8 cmd[12] = { 0xbe, 0, block >> 24, block >> 16, block >> 8, block >> 0, size >> 16, size >> 8, size >> 0, 0x10, 0, 0 };
		v = do_scsi (unitnum, cmd, sizeof cmd, data, size * sectorsize);
	} else {
		v = state[unitnum].device_func->rawread (unitnum, data, block, size, sectorsize, 0xffffffff);
	}
	freesem (unitnum);
	return v;
}
int sys_command_cd_rawread (int unitnum, uae_u8 *data, int block, int size, int sectorsize, uae_u8 sectortype, uae_u8 scsicmd9, uae_u8 subs)
{
	int v;
	if (failunit (unitnum))
		return -1;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->rawread == NULL) {
		uae_u8 cmd[12] = { 0xbe, 0, block >> 24, block >> 16, block >> 8, block >> 0, size >> 16, size >> 8, size >> 0, 0x10, 0, 0 };
		v = do_scsi (unitnum, cmd, sizeof cmd, data, size * sectorsize);
	} else {
		v = state[unitnum].device_func->rawread (unitnum, data, block, size, sectorsize, (sectortype << 16) | (scsicmd9 << 8) | subs);
	}
	freesem (unitnum);
	return v;
}

/* read block */
int sys_command_read (int unitnum, uae_u8 *data, int block, int size)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->read == NULL) {
		uae_u8 cmd[12] = { 0xa8, 0, 0, 0, 0, 0, size >> 24, size >> 16, size >> 8, size >> 0, 0, 0 };
		cmd[2] = (uae_u8)(block >> 24);
		cmd[3] = (uae_u8)(block >> 16);
		cmd[4] = (uae_u8)(block >> 8);
		cmd[5] = (uae_u8)(block >> 0);
		v = do_scsi (unitnum, cmd, sizeof cmd, data, size * 2048);
	} else {
		v = state[unitnum].device_func->read (unitnum, data, block, size);
	}
	freesem (unitnum);
	return v;
}

/* write block */
int sys_command_write (int unitnum, uae_u8 *data, int offset, int size)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->write == NULL) {
		v = 0;
	} else {
		v = state[unitnum].device_func->write (unitnum, data, offset, size);
	}
	freesem (unitnum);
	return v;
}

int sys_command_ismedia (int unitnum, int quick)
{
	int v;
	struct blkdevstate *st = &state[unitnum];
	if (failunit (unitnum))
		return -1;
	if (st->delayed)
		return 0;
	if (!getsem (unitnum))
		return 0;
	if (state[unitnum].device_func->ismedia == NULL) {
		uae_u8 cmd[6] = { 0, 0, 0, 0, 0, 0 };
		v = do_scsi (unitnum, cmd, sizeof cmd);
	} else {
		v = state[unitnum].device_func->ismedia (unitnum, quick);
	}
	freesem (unitnum);
	return v;
}

struct device_info *sys_command_info_session (int unitnum, struct device_info *di, int quick, int session)
{
	struct blkdevstate *st = &state[unitnum];
	if (failunit (unitnum))
		return NULL;
	if (!getsem (unitnum))
		return 0;
	if (st->device_func->info == NULL)
		return 0;
	struct device_info *di2 = st->device_func->info (unitnum, di, quick, -1);
	if (di2)
		st->type = di2->type;
	if (di2 && st->delayed)
		di2->media_inserted = 0;
	freesem (unitnum);
	return di2;
}
struct device_info *sys_command_info (int unitnum, struct device_info *di, int quick)
{
	return sys_command_info_session (unitnum, di, quick, -1);
}

#define MODE_SELECT_6 0x15
#define MODE_SENSE_6 0x1a
#define MODE_SELECT_10 0x55
#define MODE_SENSE_10 0x5a

void scsi_atapi_fixup_pre (uae_u8 *scsi_cmd, int *len, uae_u8 **datap, int *datalenp, int *parm)
{
	uae_u8 cmd, *p, *data = *datap;
	int l, datalen = *datalenp;

	*parm = 0;
	cmd = scsi_cmd[0];
	if (cmd != MODE_SELECT_6 && cmd != MODE_SENSE_6)
		return;
	l = scsi_cmd[4];
	if (l > 4)
		l += 4;
	scsi_cmd[7] = l >> 8;
	scsi_cmd[8] = l;
	if (cmd == MODE_SELECT_6) {
		scsi_cmd[0] = MODE_SELECT_10;
		scsi_cmd[9] = scsi_cmd[5];
		scsi_cmd[2] = scsi_cmd[3] = scsi_cmd[4] = scsi_cmd[5] = scsi_cmd[6] = 0;
		*len = 10;
		p = xmalloc (uae_u8, 8 + datalen + 4);
		if (datalen > 4)
			memcpy (p + 8, data + 4, datalen - 4);
		p[0] = 0;
		p[1] = data[0];
		p[2] = data[1];
		p[3] = data[2];
		p[4] = p[5] = p[6] = 0;
		p[7] = data[3];
		if (l > 8)
			datalen += 4;
		*parm = MODE_SELECT_10;
		*datap = p;
	} else {
		scsi_cmd[0] = MODE_SENSE_10;
		scsi_cmd[9] = scsi_cmd[5];
		scsi_cmd[3] = scsi_cmd[4] = scsi_cmd[5] = scsi_cmd[6] = 0;
		if (l > 8)
			datalen += 4;
		*datap = xmalloc (uae_u8, datalen);
		*len = 10;
		*parm = MODE_SENSE_10;
	}
	*datalenp = datalen;
}

void scsi_atapi_fixup_post (uae_u8 *scsi_cmd, int len, uae_u8 *olddata, uae_u8 *data, int *datalenp, int parm)
{
	int datalen = *datalenp;
	if (!data || !datalen)
		return;
	if (parm == MODE_SENSE_10) {
		olddata[0] = data[1];
		olddata[1] = data[2];
		olddata[2] = data[3];
		olddata[3] = data[7];
		datalen -= 4;
		if (datalen > 4)
			memcpy (olddata + 4, data + 8, datalen - 4);
		*datalenp = datalen;
	}
}

static void scsi_atapi_fixup_inquiry (struct amigascsi *as)
{
	uae_u8 *scsi_data = as->data;
	uae_u32 scsi_len = as->len;
	uae_u8 *scsi_cmd = as->cmd;
	uae_u8 cmd;

	cmd = scsi_cmd[0];
	/* CDROM INQUIRY: most Amiga programs expect ANSI version == 2
	* (ATAPI normally responds with zero)
	*/
	if (cmd == 0x12 && scsi_len > 2 && scsi_data) {
		uae_u8 per = scsi_data[0];
		uae_u8 b = scsi_data[2];
		/* CDROM and ANSI version == 0 ? */
		if ((per & 31) == 5 && (b & 7) == 0) {
			b |= 2;
			scsi_data[2] = b;
		}
	}
}

void scsi_log_before (uae_u8 *cdb, int cdblen, uae_u8 *data, int datalen)
{
	int i;
	for (i = 0; i < cdblen; i++) {
		write_log (_T("%s%02X"), i > 0 ? _T(".") : _T(""), cdb[i]);
	}
	write_log (_T("\n"));
	if (data) {
		write_log (_T("DATAOUT: %d\n"), datalen);
		for (i = 0; i < datalen && i < 100; i++)
			write_log (_T("%s%02X"), i > 0 ? _T(".") : _T(""), data[i]);
		if (datalen > 0)
			write_log (_T("\n"));
	}
}

void scsi_log_after (uae_u8 *data, int datalen, uae_u8 *sense, int senselen)
{
	int i;
	write_log (_T("DATAIN: %d\n"), datalen);
	for (i = 0; i < datalen && i < 100 && data; i++)
		write_log (_T("%s%02X"), i > 0 ? _T(".") : _T(""), data[i]);
	if (data && datalen > 0)
		write_log (_T("\n"));
	if (senselen > 0) {
		write_log (_T("SENSE: %d,"), senselen);
		for (i = 0; i < senselen && i < 32; i++) {
			write_log (_T("%s%02X"), i > 0 ? _T(".") : _T(""), sense[i]);
		}
		write_log (_T("\n"));
	}
}

static bool nodisk (struct device_info *di)
{
	return di->media_inserted == 0;
}
static int cmd_readx (int unitnum, uae_u8 *dataptr, int offset, int len)
{
	if (!getsem (unitnum))
		return 0;
	int v = state[unitnum].device_func->read (unitnum, dataptr, offset, len);
	freesem (unitnum);
	if (v >= 0)
		return len;
	return v;
}

static void wl (uae_u8 *p, int v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v;
}
static void ww (uae_u8 *p, int v)
{
	p[0] = v >> 8;
	p[1] = v;
}
static int rl (uae_u8 *p)
{
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}
static int rw (uae_u8 *p)
{
	return (p[0] << 8) | (p[1]);
}

static void stopplay (int unitnum)
{
	sys_command_cd_stop (unitnum);
}

static int addtocentry (uae_u8 **dstp, int *len, int point, int newpoint, int msf, uae_u8 *head, struct cd_toc_head *th)
{
	uae_u8 *dst = *dstp;

	for (int i = 0; i < th->points; i++) {
		struct cd_toc *t = &th->toc[i];
		if (t->point == point) {
			if (*len < 8)
				return 0;
			int addr = t->paddress;
			if (msf)
				addr = lsn2msf (addr);
			dst[0] = 0;
			dst[1] = (t->adr << 4) | t->control;
			dst[2] = newpoint >= 0 ? newpoint : point;
			dst[3] = 0;
			dst[4] = addr >> 24;
			dst[5] = addr >> 16;
			dst[6] = addr >>  8;
			dst[7] = addr >>  0;

			if (point >= 1 && point <= 99) {
				if (head[2] == 0)
					head[2] = point;
				head[3] = point;
			}

			*len -= 8;
			*dstp = dst + 8;
			return 1;
		}
	}
	return -1;
}

static int scsiemudrv (int unitnum, uae_u8 *cmd)
{
	if (failunit (unitnum))
		return -1;
	if (!getsem (unitnum))
		return 0;
	int v = 0;
	if (state[unitnum].device_func->scsiemu)
		v = state[unitnum].device_func->scsiemu (unitnum, cmd);
	freesem (unitnum);
	return v;
}

static int scsi_read_cd (int unitnum, uae_u8 *cmd, uae_u8 *data, struct device_info *di)
{
	struct blkdevstate *st = &state[unitnum];
	int msf = cmd[0] == 0xb9;
	int start = msf ? msf2lsn (rl (cmd + 2) & 0x00ffffff) : rl (cmd + 2);
	int len = rl (cmd + 5) & 0x00ffffff;
	if (msf) {
		int end = msf2lsn (len);
		len = end - start;
		if (len < 0)
			return -1;
	}
	int subs = cmd[10] & 7;
	if (len == 0)
		return 0;
	int v = sys_command_cd_rawread (unitnum, data, start, len, 0, (cmd[1] >> 2) & 7, cmd[9], subs);
	if (v > 0)
		st->current_pos = start + len;
	return v;
}

static int scsi_read_cd_data (int unitnum, uae_u8 *scsi_data, uae_u32 offset, uae_u32 len, struct device_info *di, int *scsi_len)
{
	struct blkdevstate *st = &state[unitnum];
	if (len == 0) {
		if (offset >= di->sectorspertrack * di->cylinders * di->trackspercylinder)
			return -1;
		*scsi_len = 0;
		return 0;
	} else {
		if (len * di->bytespersector > SCSI_DATA_BUFFER_SIZE)
			return -3;
		if (offset >= di->sectorspertrack * di->cylinders * di->trackspercylinder)
			return -1;
		int v = cmd_readx (unitnum, scsi_data, offset, len) * di->bytespersector;
		if (v > 0) {
			st->current_pos = offset + len;
			*scsi_len = v;
			return 0;
		}
		return -2;
	}
}

int scsi_cd_emulate (int unitnum, uae_u8 *cmdbuf, int scsi_cmd_len,
	uae_u8 *scsi_data, int *data_len, uae_u8 *r, int *reply_len, uae_u8 *s, int *sense_len, bool atapi)
{
	struct blkdevstate *st = &state[unitnum];
	uae_u32 len, offset;
	int lr = 0, ls = 0;
	int scsi_len = -1;
	int v;
	int status = 0;
	struct device_info di;
	uae_u8 cmd = cmdbuf[0];
	int dlen;

	if (cmd == 0x03) { /* REQUEST SENSE */
		st->mediawaschanged = false;
		return 0;
	}
	
	dlen = *data_len;
	*reply_len = *sense_len = 0;

	sys_command_info (unitnum, &di, 1);

	if (log_scsiemu) {
		write_log (_T("SCSIEMU %d: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X CMDLEN=%d DATA=%08X LEN=%d\n"), unitnum,
			cmdbuf[0], cmdbuf[1], cmdbuf[2], cmdbuf[3], cmdbuf[4], cmdbuf[5], cmdbuf[6], 
			cmdbuf[7], cmdbuf[8], cmdbuf[9], cmdbuf[10], cmdbuf[11],
			scsi_cmd_len, scsi_data, dlen);
	}

	// media changed and not inquiry
	if (st->mediawaschanged && cmd != 0x12) {
		if (log_scsiemu) {
			write_log (_T("SCSIEMU %d: MEDIUM MAY HAVE CHANGED STATE\n"));
		}
		lr = -1;
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 6; /* UNIT ATTENTION */
		s[12] = 0x28; /* MEDIUM MAY HAVE CHANGED */
		ls = 0x12;
		if (cmd == 0x00)
			st->mediawaschanged = false;
		goto end;
	}

	switch (cmdbuf[0])
	{
	case 0x00: /* TEST UNIT READY */
		if (nodisk (&di))
			goto nodisk;
		scsi_len = 0;
		break;
	case 0x1e: /* PREVENT/ALLOW MEDIUM REMOVAL */
		scsi_len = 0;
		break;
	case 0xbd: /* MECHANISM STATUS */
		len = (cmdbuf[8] << 8) | cmdbuf[9];
		if (len > 8)
			len = 8;
		scsi_len = len;
		r[2] = st->current_pos >> 16;
		r[3] = st->current_pos >>  8;
		r[4] = st->current_pos >>  0;
		break;
	case 0x12: /* INQUIRY */
	{
		if ((cmdbuf[1] & 1) || cmdbuf[2] != 0)
			goto err;
		len = cmdbuf[4];
		if (cmdbuf[1] >> 5) {
			r[0] = 0x7f;
		} else {
		  r[0] = 5; // CDROM
		}
		r[1] |= 0x80; // removable
		r[2] = 2; /* supports SCSI-2 */
		r[3] = 2; /* response data format */
		if (atapi)
			r[3] |= 3 << 5; // atapi transport version
		r[4] = 32; /* additional length */
		r[7] = 0;
		scsi_len = lr = len < 36 ? len : 36;
		r[2] = 2;
		r[3] = 2;
		char *s = ua (di.vendorid);
		memcpy (r + 8, s, strlen (s));
		xfree (s);
		s = ua (di.productid);
		memcpy (r + 16, s, strlen (s));
		xfree (s);
		s = ua (di.revision);
		memcpy (r + 32, s, strlen (s));
		xfree (s);
		for (int i = 8; i < 36; i++) {
			if (r[i] == 0)
				r[i] = 32;
		}
	}
	break;
	case 0xbe: // READ CD
	case 0xb9: // READ CD MSF
		if (nodisk (&di))
			goto nodisk;
		scsi_len = scsi_read_cd (unitnum, cmdbuf, scsi_data, &di);
		if (scsi_len == -2)
			goto notdatatrack;
		if (scsi_len == -1)
			goto errreq;
		break;
	case 0x55: // MODE SELECT(10)
	case 0x15: // MODE SELECT(6)
	{
		uae_u8 *p;
		bool mode10 = cmdbuf[0] == 0x55;
		p = scsi_data + 4;
		if (mode10)
			p += 4;
		int pcode = p[0] & 0x3f;
		if (pcode == 14) { // CD audio control
			uae_u16 vol_left = (p[9] << 7) | (p[9] >> 1);
			uae_u16 vol_right = (p[11] << 7) | (p[11] >> 1);
			sys_command_cd_volume (unitnum, vol_left, vol_right);
			scsi_len = 0;
		} else {
			if (log_scsiemu)
				write_log (_T("MODE SELECT PC=%d not supported\n"), pcode);
			goto errreq;
		}
	}
	break;
	case 0x5a: // MODE SENSE(10)
	case 0x1a: /* MODE SENSE(6) */
	{
		uae_u8 *p;
		int maxlen;
		bool pcodeloop = false;
		bool sense10 = cmdbuf[0] == 0x5a;
		int psize, totalsize, bdsize;
		int pc = cmdbuf[2] >> 6;
		int pcode = cmdbuf[2] & 0x3f;
		int dbd = cmdbuf[1] & 8;

		if (atapi) {
			if (!sense10)
				goto err;
			dbd = 1;
		}
		if (log_scsiemu)
			write_log (_T("MODE SENSE PC=%d CODE=%d DBD=%d\n"), pc, pcode, dbd);
		p = r;
		if (sense10) {
			totalsize = 8 - 2;
			maxlen = (cmdbuf[7] << 8) | cmdbuf[8];
			p[2] = 0;
			p[3] = 0;
			p[4] = 0;
			p[5] = 0;
			p[6] = 0;
			p[7] = 0;
			p += 8;
		} else {
			totalsize = 4 - 1;
			maxlen = cmdbuf[4];
			p[1] = 0;
			p[2] = 0;
			p[3] = 0;
			p += 4;
		}
		bdsize = 0;
		if (!dbd) {
			wl(p + 0, 0);
			wl(p + 4, di.bytespersector);
			bdsize = 8;
			p += bdsize;
		}
		if (pcode == 0x3f) {
			pcode = 1; // page = 0 must be last
			pcodeloop = true;
		}
		for (;;) {
			psize = 0;
		  if (pcode == 0) {
			  p[0] = 0;
			  p[1] = 0;
			  p[2] = 0x20;
			  p[3] = 0;
				psize = 4;
		  } else if (pcode == 14) { // CD audio control
			  uae_u32 vol = sys_command_cd_volume (unitnum, 0xffff, 0xffff);
			  p[0] = 0x0e;
			  p[1] = 0x0e;
				p[2] = 4|1;
			  p[3] = 4;
			  p[6] = 0;
			  p[7] = 75;
			  p[8] = 1;
			  p[9] = pc == 0 ? (vol >> 7) & 0xff : 0xff;
			  p[10] = 2;
			  p[11] = pc == 0 ? (vol >> (16 + 7)) & 0xff : 0xff;
				psize = p[1] + 2;
			} else if (pcode == 0x2a) {  // cd/dvd capabilities
				p[0] = 0x2a;
				p[1] = 0x18;
				p[2] = 1; // | 0x10 | 0x20; // read: CD-R/DVD-ROM/DVD-R
				p[3] = 0; // write: nothing
				p[4] = 0x40 | 0x20 | 0x10 | 0x01;
				p[5] = 0x08 | 0x04 | 0x02 | 0x01;
				p[6] = (1 << 5) | 0x10; // type = tray, eject supported
				p[7] = 3; // separate channel mute and volume
				p[8] = 2; p[9] = 0;
				p[10] = 0xff; p[11] = 0xff; // number of volume levels
				p[12] = 4; p[13] = 0; // "1M buffer"
				p[14] = 2; p[15] = 0;
				p[16] = 0;
				p[17] = 0;
				p[18] = p[19] = 0;
				p[20] = p[21] = 0;
				p[22] = p[23] = 0;
				psize = p[1] + 2;
		  } else {
				if (!pcodeloop)
			    goto err;
		  }
			totalsize += psize;
			p += psize;
			if (!pcodeloop)
				break;
			if (pcode == 0)
				break;
			pcode++;
			if (pcode == 0x3f)
				pcode = 0;
	  }
		if (sense10) {
			totalsize += bdsize;
			r[6] = bdsize >> 8;
			r[7] = bdsize & 0xff;
			r[0] = totalsize >> 8;
			r[1] = totalsize & 0xff;
		} else {
			totalsize += bdsize;
			r[3] = bdsize & 0xff;
			r[0] = totalsize & 0xff;
		}
		scsi_len = totalsize + 1;
		if (scsi_len > maxlen)
			scsi_len = maxlen;
		lr = scsi_len;
	}
	break;
	case 0x01: /* REZERO UNIT */
		scsi_len = 0;
	break;
	case 0x1d: /* SEND DIAGNOSTICS */
		scsi_len = 0;
		break;
	case 0x25: /* READ CAPACITY */
		{
			int pmi = cmdbuf[8] & 1;
			uae_u32 lba = (cmdbuf[2] << 24) | (cmdbuf[3] << 16) | (cmdbuf[4] << 8) | cmdbuf[5];
			int cyl, cylsec, head, tracksec;
			if (nodisk (&di))
				goto nodisk;
			uae_u32 blocks = di.sectorspertrack * di.cylinders * di.trackspercylinder - 1;
			cyl = di.cylinders;
			head = 1;
			cylsec = tracksec = di.trackspercylinder;
			if (pmi == 0 && lba != 0)
				goto errreq;
			if (pmi) {
				lba += tracksec * head;
				lba /= tracksec * head;
				lba *= tracksec * head;
				if (lba > blocks)
					lba = blocks;
				blocks = lba;
			}
			wl (r, blocks);
			wl (r + 4, di.bytespersector);
			scsi_len = lr = 8;
		}
		break;
	case 0x0b: /* SEEK (6) */
		{
			if (nodisk (&di))
				goto nodisk;
			stopplay (unitnum);
			offset = ((cmdbuf[1] & 31) << 16) | (cmdbuf[2] << 8) | cmdbuf[3];
			struct cd_toc *t = gettoc (&di.toc, offset);
			v = scsi_read_cd_data (unitnum, scsi_data, offset, 0, &di, &scsi_len);
			if (v == -1)
				goto outofbounds;
		}
	break;
	case 0x08: /* READ (6) */
	{
		if (nodisk (&di))
			goto nodisk;
		stopplay (unitnum);
		offset = ((cmdbuf[1] & 31) << 16) | (cmdbuf[2] << 8) | cmdbuf[3];
		struct cd_toc *t = gettoc (&di.toc, offset);
		if ((t->control & 0x0c) == 0x04) {
			len = cmdbuf[4];
			if (!len)
				len = 256;
			v = scsi_read_cd_data (unitnum, scsi_data, offset, len, &di, &scsi_len);
			if (v == -1)
				goto outofbounds;
			if (v == -2)
				goto readerr;
			if (v == -3)
				goto toolarge;
		} else {
			goto notdatatrack;
		}
	}
	break;
	case 0x0a: /* WRITE (6) */
		goto readprot;
	case 0x2b: /* SEEK (10) */
		{
			if (nodisk (&di))
				goto nodisk;
			stopplay (unitnum);
			offset = rl (cmdbuf + 2);
			struct cd_toc *t = gettoc (&di.toc, offset);
			v = scsi_read_cd_data (unitnum, scsi_data, offset, 0, &di, &scsi_len);
			if (v == -1)
				goto outofbounds;
		}
	break;
	case 0x28: /* READ (10) */
	{
		if (nodisk (&di))
			goto nodisk;
		stopplay (unitnum);
		offset = rl (cmdbuf + 2);
		struct cd_toc *t = gettoc (&di.toc, offset);
		if ((t->control & 0x0c) == 0x04) {
			len = rl (cmdbuf + 7 - 2) & 0xffff;
			v = scsi_read_cd_data (unitnum, scsi_data, offset, len, &di, &scsi_len);
			if (v == -1)
				goto outofbounds;
			if (v == -2)
				goto readerr;
			if (v == -3)
				goto toolarge;
		} else {
			goto notdatatrack;
		}
	}
	break;
	case 0x2a: /* WRITE (10) */
		goto readprot;
	case 0xa8: /* READ (12) */
	{
		if (nodisk (&di))
			goto nodisk;
		stopplay (unitnum);
		offset = rl (cmdbuf + 2);
		struct cd_toc *t = gettoc (&di.toc, offset);
		if ((t->control & 0x0c) == 0x04) {
			len = rl (cmdbuf + 6);
			v = scsi_read_cd_data (unitnum, scsi_data, offset, len, &di, &scsi_len);
			if (v == -1)
				goto outofbounds;
			if (v == -2)
				goto readerr;
			if (v == -3)
				goto toolarge;
		} else {
			goto notdatatrack;
		}
	}
	break;
	case 0xaa: /* WRITE (12) */
		goto readprot;
	case 0x51: /* READ DISC INFORMATION */
		{
			struct cd_toc_head ttoc;
			int maxlen = (cmdbuf[7] << 8) | cmdbuf[8];
			if (nodisk (&di))
				goto nodisk;
			if (!sys_command_cd_toc (unitnum, &ttoc))
				goto readerr;
			struct cd_toc_head *toc = &ttoc;
			uae_u8 *p = scsi_data;
			p[0] = 0;
			p[1] = 34 - 2;
			p[2] = 2 | (3 << 2); // complete cd rom, last session is complete
			p[3] = toc->first_track;
			p[4] = 1;
			p[5] = toc->first_track;
			p[6] = toc->last_track;
			wl (p + 16, lsn2msf (toc->lastaddress));
			wl (p + 20, 0x00ffffff);
			scsi_len = p[1] + 2;
			if (scsi_len > maxlen)
				scsi_len = maxlen;
		}
		break;
	case 0x52: /* READ TRACK INFORMATION */
		{
			struct cd_toc_head ttoc;
			int maxlen = (cmdbuf[7] << 8) | cmdbuf[8];
			if (nodisk (&di))
				goto nodisk;
			if (!sys_command_cd_toc (unitnum, &ttoc))
				goto readerr;
			struct cd_toc_head *toc = &ttoc;
			uae_u8 *p = scsi_data;
			int lsn;
			if (cmdbuf[1] & 1) {
				int track = cmdbuf[5];
				lsn = toc->toc[track].address;
			} else {
				lsn = rl (p + 2);
			}
			struct cd_toc *t = gettoc (toc, lsn);
			p[0] = 0;
			p[1] = 28 - 2;
			p[2] = t->track;
			p[3] = 1;
			p[5] = t->control;
			p[6] = 0; // data mode, fixme
			wl (p + 8, t->address);
			wl (p + 24, t[1].address - t->address);
			scsi_len = p[1] + 2;
			if (scsi_len > maxlen)
				scsi_len = maxlen;
		}
		break;
	case 0x43: // READ TOC
		{
			if (nodisk (&di))
				goto nodisk;
			uae_u8 *p = scsi_data;
			int strack = cmdbuf[6];
			int msf = cmdbuf[1] & 2;
			int format = cmdbuf[2] & 7;
			if (format >= 3)
				goto errreq;
			int maxlen = (cmdbuf[7] << 8) | cmdbuf[8];
			int maxlen2 = maxlen;
			struct cd_toc_head ttoc;
			if (!sys_command_cd_toc (unitnum, &ttoc))
				goto readerr;
			struct cd_toc_head *toc = &ttoc;
			if (maxlen < 4)
				goto errreq;
			if (format == 1) {
				p[0] = 0;
				p[1] = 2 + 8;
				p[2] = 1;
				p[3] = 1;
				p[4] = 0;
				p[5] = (toc->toc[0].adr << 4) | toc->toc[0].control;
				p[6] = toc->first_track;
				p[7] = 0;
				if (msf)
					wl (p + 8, lsn2msf (toc->toc[0].address));
				else
					wl (p + 8 , toc->toc[0].address);
				scsi_len = 12;
			} else if (format == 2 || format == 0) {
				if (format == 2 && !msf)
					goto errreq;
				if (strack == 0)
					strack = toc->first_track;
				if (format == 0 && strack >= 100 && strack != 0xaa)
					goto errreq;
				uae_u8 *p2 = p + 4;
				p[2] = 0;
				p[3] = 0;
				maxlen -= 4;
				if (format == 2) {
					if (!addtocentry (&p2, &maxlen, 0xa0, -1, msf, p, toc))
						break;
					if (!addtocentry (&p2, &maxlen, 0xa1, -1, msf, p, toc))
						break;
					if (!addtocentry (&p2, &maxlen, 0xa2, -1, msf, p, toc))
						break;
				}
				while (strack < 100) {
					if (!addtocentry (&p2, &maxlen, strack, -1, msf, p, toc))
						break;
					strack++;
				}
				addtocentry (&p2, &maxlen, 0xa2, 0xaa, msf, p, toc);				
				int tlen = p2 - (p + 2);
				p[0] = tlen >> 8;
				p[1] = tlen >> 0;
				scsi_len = tlen + 2;
				if (scsi_len > maxlen2)
					scsi_len = maxlen2;
			}
		}
		break;
		case 0x42: // READ SUB-CHANNEL
		{
			int msf = cmdbuf[1] & 2;
			int subq = cmdbuf[2] & 0x40;
			int format = cmdbuf[3];
			int track = cmdbuf[6];
			int len = rw (cmdbuf + 7);
			uae_u8 buf[SUBQ_SIZE] = { 0 };

			if (nodisk (&di))
				goto nodisk;
			sys_command_cd_qcode (unitnum, buf);
			if (len < 4)
				goto errreq;
			scsi_len = 4;
			scsi_data[0] = 0;
			scsi_data[1] = buf[1];
			if (subq && format == 1) {
				if (len < 4 + 12)
					goto errreq;
				scsi_data[2] = 0;
				scsi_data[3] = 12;
				scsi_len += 12;
				scsi_data[4] = 1;
				scsi_data[5] = (buf[4 + 0] << 4) | (buf[4 + 0] >> 4);
				scsi_data[6] = frombcd (buf[4 + 1]); // track
				scsi_data[7] = frombcd (buf[4 + 2]); // index
				int reladdr = fromlongbcd (&buf[4 + 3]);
				int absaddr = fromlongbcd (&buf[4 + 7]);
				if (!msf) {
					reladdr = msf2lsn (reladdr);
					absaddr = msf2lsn (absaddr);
				}
				wl (scsi_data +  8, absaddr);
				wl (scsi_data + 12, reladdr);
			} else {
				scsi_data[2] = 0;
				scsi_data[3] = 0;
			}
		}
		break;
		case 0x1b: // START/STOP
			sys_command_cd_stop (unitnum);
			scsiemudrv (unitnum, cmdbuf);
			scsi_len = 0;
		break;
		case 0x4e: // STOP PLAY/SCAN
			if (nodisk (&di))
				goto nodisk;
			sys_command_cd_stop (unitnum);
			scsi_len = 0;
		break;
		case 0xba: // SCAN
		{
			if (nodisk (&di))
				goto nodisk;
			struct cd_toc_head ttoc;
			if (!sys_command_cd_toc (unitnum, &ttoc))
				goto readerr;
			struct cd_toc_head *toc = &ttoc;
			int scan = (cmdbuf[1] & 0x10) ? -1 : 1;
			int start = rl (cmdbuf + 1) & 0x00ffffff;
			int end = scan > 0 ? toc->lastaddress : toc->toc[toc->first_track_offset].paddress;
			int type = cmdbuf[9] >> 6;
			if (type == 1)
				start = lsn2msf (start);
			if (type == 3)
				goto errreq;
			if (type == 2) {
				if (toc->first_track_offset + start >= toc->last_track_offset)
					goto errreq;
				start = toc->toc[toc->first_track_offset + start].paddress;
			}
			sys_command_cd_pause (unitnum, 0);
			sys_command_cd_play (unitnum, start, end, scan);
			scsi_len = 0;
		}
		break;
		case 0x48: // PLAY AUDIO TRACK/INDEX
		{
			if (nodisk (&di))
				goto nodisk;
			int strack = cmdbuf[4];
			int etrack = cmdbuf[7];
			struct cd_toc_head ttoc;
			if (!sys_command_cd_toc (unitnum, &ttoc))
				goto readerr;
			struct cd_toc_head *toc = &ttoc;
			if (strack < toc->first_track || strack > toc->last_track ||
				etrack < toc->first_track || etrack > toc->last_track ||
				strack > etrack)
				goto errreq;
			int start = toc->toc[toc->first_track_offset + strack - 1].paddress;
			int end = etrack == toc->last_track ? toc->lastaddress : toc->toc[toc->first_track_offset + etrack - 1 + 1].paddress;
			sys_command_cd_pause (unitnum, 0);
			if (!sys_command_cd_play (unitnum, start, end, 0))
				goto notdatatrack;
			scsi_len = 0;
		}
		break;
		case 0x49: // PLAY AUDIO TRACK RELATIVE (10)
		case 0xa9: // PLAY AUDIO TRACK RELATIVE (12)
		{
			if (nodisk (&di))
				goto nodisk;
			int len = cmd == 0xa9 ? rl (cmdbuf + 6) : rw (cmdbuf + 7);
			int track = cmd == 0xa9 ? cmdbuf[10] : cmdbuf[6];
			if (track < di.toc.first_track || track > di.toc.last_track)
				goto errreq;
			int start = di.toc.toc[di.toc.first_track_offset + track - 1].paddress;
			int rel = rl (cmdbuf + 2);
			start += rel;
			int end = start + len;
			if (end > di.toc.lastaddress)
				end = di.toc.lastaddress;
			if (len > 0) {
				sys_command_cd_pause (unitnum, 0);
				if (!sys_command_cd_play (unitnum, start, start + len, 0))
					goto notdatatrack;
			}
			scsi_len = 0;
		}
		break;
		case 0x47: // PLAY AUDIO MSF
		{
			if (nodisk (&di))
				goto nodisk;
			int start = rl (cmdbuf + 2) & 0x00ffffff;
			if (start == 0x00ffffff) {
				uae_u8 buf[SUBQ_SIZE] = { 0 };
				sys_command_cd_qcode (unitnum, buf);
				start = fromlongbcd (buf + 4 + 7);
			}
			int end = msf2lsn (rl (cmdbuf + 5) & 0x00ffffff);
			if (end > di.toc.lastaddress)
				end = di.toc.lastaddress;
			start = msf2lsn (start);
			if (start > end)
				goto errreq;
			if (start < end)
				sys_command_cd_pause (unitnum, 0);
				if (!sys_command_cd_play (unitnum, start, end, 0))
					goto notdatatrack;
			scsi_len = 0;
		}
		break;
		case 0x45: // PLAY AUDIO (10)
		case 0xa5: // PLAY AUDIO (12)
		{
			if (nodisk (&di))
				goto nodisk;
			int start = rl (cmdbuf + 2);
			int len;
			if (cmd == 0xa5)
				len = rl (cmdbuf + 6);
			else
				len = rw (cmdbuf + 7);
			if (len > 0) {
				if (start == -1) {
					uae_u8 buf[SUBQ_SIZE] = { 0 };
					sys_command_cd_qcode (unitnum, buf);
					start = msf2lsn (fromlongbcd (buf + 4 + 7));
				}
				int end = start + len;
				if (end > di.toc.lastaddress)
					end = di.toc.lastaddress;
				sys_command_cd_pause (unitnum, 0);
				if (!sys_command_cd_play (unitnum, start, end, 0))
					goto notdatatrack;
			}
			scsi_len = 0;
		}
		break;
		case 0xbc: // PLAY CD
		{
			if (nodisk (&di))
				goto nodisk;
			int start = -1;
			int end = -1;
			if (cmdbuf[1] & 2) {
				start = msf2lsn (rl (cmdbuf + 2) & 0x00ffffff);
				end = msf2lsn (rl (cmdbuf + 5) & 0x00ffffff);
			} else {
				start = rl (cmdbuf + 2);
				end = start + rl (cmdbuf + 6);
			}
			if (end > di.toc.lastaddress)
				end = di.toc.lastaddress;
			if (start > end)
				goto errreq;
			if (start < end) {
				sys_command_cd_pause (unitnum, 0);
				if (!sys_command_cd_play (unitnum, start, end, 0))
					goto notdatatrack;
			}
		}
		break;
		case 0x4b: // PAUSE/RESUME
		{
			if (nodisk (&di))
				goto nodisk;
			uae_u8 buf[SUBQ_SIZE] = { 0 };
			int resume = cmdbuf[8] & 1;
			sys_command_cd_qcode (unitnum, buf);
			if (buf[1] != AUDIO_STATUS_IN_PROGRESS && buf[1] != AUDIO_STATUS_PAUSED)
				goto errreq;
			sys_command_cd_pause (unitnum, resume ? 0 : 1);
			scsi_len = 0;
		}
		break;
		case 0x35: /* SYNCRONIZE CACHE (10) */
			scsi_len = 0;
		break;

		default:
err:
		write_log (_T("CDEMU: unsupported scsi command 0x%02X\n"), cmdbuf[0]);
readprot:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5;
		s[12] = 0x20; /* INVALID COMMAND */
		ls = 0x12;
		break;
nodisk:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 2; /* NOT READY */
		s[12] = 0x3A; /* MEDIUM NOT PRESENT */
		ls = 0x12;
		break;
readerr:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 2; /* NOT READY */
		s[12] = 0x11; /* UNRECOVERED READ ERROR */
		ls = 0x12;
		break;
notdatatrack:
		status = 2;
		s[0] = 0x70;
		s[2] = 5;
		s[12] = 0x64; /* ILLEGAL MODE FOR THIS TRACK */
		ls = 0x12;
		break;
outofbounds:
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x21; /* LOGICAL BLOCK OUT OF RANGE */
		ls = 0x12;
		break;
toolarge:
		write_log (_T("CDEMU: too large scsi data tranfer %d > %d\n"), len, dlen);
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 2; /* NOT READY */
		s[12] = 0x11; /* UNRECOVERED READ ERROR */
		ls = 0x12;
		break;
errreq:
		lr = -1;
		status = 2; /* CHECK CONDITION */
		s[0] = 0x70;
		s[2] = 5; /* ILLEGAL REQUEST */
		s[12] = 0x24; /* ILLEGAL FIELD IN CDB */
		ls = 0x12;
		break;
	}
end:
	*data_len = scsi_len;
	*reply_len = lr;
	*sense_len = ls;
	if (ls) {
		//s[0] |= 0x80;
		s[7] = ls - 7; // additional sense length
		if (log_scsiemu) {
			write_log (_T("-> SENSE STATUS: KEY=%d ASC=%02X ASCQ=%02X\n"), s[2], s[12], s[13]);
		}
	}
	if (cmdbuf[0] && log_scsiemu)
		write_log (_T("-> DATAOUT=%d ST=%d SENSELEN=%d\n"), scsi_len, status, ls);
	return status;
}

static int execscsicmd_direct (int unitnum, int type, struct amigascsi *as)
{
	int io_error = 0;
	uae_u8 *scsi_datap, *scsi_datap_org;
	uae_u32 scsi_cmd_len_orig = as->cmd_len;
	uae_u8 cmd[16] = { 0 };
	uae_u8 replydata[256] = { 0 };
	int datalen = as->len;
	int senselen = as->sense_len;
	int replylen = 0;

	memcpy (cmd, as->cmd, as->cmd_len);
	scsi_datap = scsi_datap_org = as->len ? as->data : 0;
	if (as->sense_len > 32)
		as->sense_len = 32;

	/* never report media change state if uaescsi.device */
	state[unitnum].mediawaschanged = false;

	switch (type)
	{
		case INQ_ROMD:
	    as->status = scsi_cd_emulate (unitnum, cmd, as->cmd_len, scsi_datap, &datalen, replydata, &replylen, as->sensedata, &senselen, false);
  		break;
		default:
	    as->status = 2;
	    break;
  }

	as->cmdactual = as->status != 0 ? 0 : as->cmd_len; /* fake scsi_CmdActual */
	if (as->status) {
		io_error = IOERR_BadStatus;
		as->sactual = senselen;
		as->actual = 0; /* scsi_Actual */
	} else {
		int i;
		if (replylen > 0) {
			for (i = 0; i < replylen; i++)
				scsi_datap[i] = replydata[i];
			datalen = replylen;
		}
		for (i = 0; i < as->sense_len; i++)
			as->sensedata[i] = 0;
		if (datalen < 0) {
			io_error = IOERR_NotSpecified;
			as->actual = 0; /* scsi_Actual */
		} else {
			as->len = datalen;
			io_error = 0;
			as->actual = as->len; /* scsi_Actual */
		}
	}

	return io_error;
}

int sys_command_scsi_direct_native (int unitnum, int type, struct amigascsi *as)
{
	struct blkdevstate *st = &state[unitnum];
	if (st->scsiemu || (type >= 0 && st->type != type)) {
		return execscsicmd_direct (unitnum, type, as);
	} else {
		if (!st->device_func->exec_direct)
			return -1;
	}
	int ret = st->device_func->exec_direct (unitnum, as);
	if (!ret && st->device_func->isatapi(unitnum))
		scsi_atapi_fixup_inquiry (as);
	return ret;
}

int sys_command_scsi_direct (int unitnum, int type, uaecptr acmd)
{
	int ret, i;
	struct amigascsi as = { 0 };
	uaecptr ap;
	addrbank *bank;

	ap = get_long (acmd + 0);
	as.len = get_long (acmd + 4);

	bank = &get_mem_bank (ap);
	if (!bank || !bank->check(ap, as.len))
		return IOERR_BADADDRESS;
	as.data = bank->xlateaddr (ap);

	ap = get_long (acmd + 12);
	as.cmd_len = get_word (acmd + 16);
	if (as.cmd_len > sizeof as.cmd)
		return IOERR_BADLENGTH;
	for (i = 0; i < as.cmd_len; i++)
		as.cmd[i] = get_byte (ap++);
	while (i < sizeof as.cmd)
		as.cmd[i++] = 0;
	as.flags = get_byte (acmd + 20);
	as.sense_len = get_word (acmd + 26);

	ret = sys_command_scsi_direct_native (unitnum, type, &as);

	put_long (acmd + 8, as.actual);
	put_word (acmd + 18, as.cmdactual);
	put_byte (acmd + 21, as.status);
	put_word (acmd + 28, as.sactual);

	if (as.flags & (2 | 4)) { // autosense
  	ap = get_long (acmd + 22);
		for (i = 0; i < as.sactual && i < as.sense_len; i++)
			put_byte (ap + i, as.sensedata[i]);
	}

	return ret;
}

#ifdef SAVESTATE

uae_u8 *save_cd (int num, int *len)
{
	struct blkdevstate *st = &state[num];
	uae_u8 *dstbak, *dst;

	memset(st->play_qcode, 0, SUBQ_SIZE);
	if (!currprefs.cdslots[num].inuse || num >= MAX_TOTAL_SCSI_DEVICES)
		return NULL;
	if (!currprefs.cs_cd32cd)
		return NULL;
	dstbak = dst = xmalloc (uae_u8, 4 + 256 + 4 + 4);
	save_u32 (4 | 8);
	save_path (currprefs.cdslots[num].name, SAVESTATE_PATH_CD);
	save_u32 (currprefs.cdslots[num].type);
	save_u32 (0);
	save_u32 (0);
	sys_command_cd_qcode (num, st->play_qcode);
	for (int i = 0; i < SUBQ_SIZE; i++)
		save_u8 (st->play_qcode[i]);
	save_u32 (st->play_end_pos);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_cd (int num, uae_u8 *src)
{
	struct blkdevstate *st = &state[num];
	uae_u32 flags;
	TCHAR *s;

	if (num >= MAX_TOTAL_SCSI_DEVICES)
		return NULL;
	flags = restore_u32 ();
	s = restore_path (SAVESTATE_PATH_CD);
	int type = restore_u32 ();
	restore_u32 ();
	if (flags & 4) {
		if (currprefs.cdslots[num].name[0] == 0 || zfile_exists (s)) {
		  _tcscpy (changed_prefs.cdslots[num].name, s);
		  _tcscpy (currprefs.cdslots[num].name, s);
		}
		changed_prefs.cdslots[num].type = currprefs.cdslots[num].type = type;
	}
	if (flags & 8) {
		restore_u32 ();
		for (int i = 0; i < SUBQ_SIZE; i++)
			st->play_qcode[i] = restore_u8 ();
		st->play_end_pos = restore_u32 ();
	}
	return src;
}

#endif

