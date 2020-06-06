/*
* UAE - The Un*x Amiga Emulator
*
* lowlevel cd device glue, scsi emulator
*
* Copyright 2009-2013 Toni Wilen
*
*/

#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "sysdeps.h"
#include "options.h"

#include "blkdev.h"
#include "savestate.h"
#include "crc32.h"
#include "threaddep/thread.h"
#include "zfile.h"

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
};

static struct blkdevstate state[MAX_TOTAL_SCSI_DEVICES];

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

static struct cd_toc *gettoc (int unitnum, struct cd_toc_head *th, int block)
{
	if (th->lastaddress == 0) {
		if (unitnum < 0)
			return NULL;
		if (!sys_command_cd_toc(unitnum, th))
			return NULL;
	}
	for (int i = th->first_track_offset + 1; i <= th->last_track_offset; i++) {
		struct cd_toc *t = &th->toc[i];
		if (block < t->paddress)
				return t - 1;
	}
	return &th->toc[th->last_track_offset];
}

int isaudiotrack (struct cd_toc_head *th, int block)
{
	struct cd_toc *t = gettoc (-1, th, block);
	if (!t)
		return 0;
	return (t->control & 0x0c) != 4;
}
int isdatatrack (struct cd_toc_head *th, int block)
{
	return !isaudiotrack (th, block);
}

static int cdscsidevicetype[MAX_TOTAL_SCSI_DEVICES];

static struct device_functions *devicetable[] = {
	NULL,
	&devicefunc_cdimage
};
#define NUM_DEVICE_TABLE_ENTRIES 2
static int driver_installed[NUM_DEVICE_TABLE_ENTRIES];

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
		  // use image mode if driver disabled
		  for (int j = 1; j < NUM_DEVICE_TABLE_ENTRIES; j++) {
			  if (devicetable[j] == st->device_func && driver_installed[j] < 0) {
				  st->device_func = devicetable[SCSI_UNIT_IMAGE];
				  st->scsiemu = true;
			  }
		  }
	  }
	}

	for (int j = 1; j < NUM_DEVICE_TABLE_ENTRIES; j++) {
		if (devicetable[j] == NULL) {
		    continue;
		}
		if (!driver_installed[j]) {
			for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
				struct blkdevstate *st = &state[i];
				if (st->device_func == devicetable[j]) {
					int ok = st->device_func->openbus (0);
					if (!ok && st->device_func != devicetable[SCSI_UNIT_IMAGE]) {
						st->device_func = devicetable[SCSI_UNIT_IMAGE];
						st->scsiemu = true;
						write_log (_T("Fallback to image mode, unit %d.\n"), i);
						driver_installed[j] = -1;
					} else {
					  driver_installed[j] = 1;
					}
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
		p->cdslots[i].temporary = false;
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
	if (isaudio) {
		TCHAR vol[100];
		_stprintf (vol, _T("%c:\\"), isaudio);
		if (sys_command_open_internal (unitnum, vol, csu)) 
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
	st->delayed = 0;
	if (currprefs.cdslots[unitnum].delayed) {
		st->delayed = PRE_INSERT_DELAY;
	}
	return unitnum;
}

int sys_command_open (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	blkdev_fix_prefs (&currprefs);
	if (!dev_init) {
		device_func_init (0);
	}

	if (st->isopen) {
		st->isopen++;
		return -1;
	}
	st->waspaused = 0;
	int v = sys_command_open_internal (unitnum, currprefs.cdslots[unitnum].name[0] ? currprefs.cdslots[unitnum].name : NULL, CD_STANDARD_UNIT_DEFAULT);
	if (!v)
		return 0;
	return v;
}

void sys_command_close (int unitnum)
{
	struct blkdevstate *st = &state[unitnum];
	if (st->isopen > 1) {
		st->isopen--;
		return;
	}
	sys_command_close_internal (unitnum);
}

void blkdev_cd_change (int unitnum, const TCHAR *name)
{
	struct device_info di;
	sys_command_info (unitnum, &di, 1);
}

void device_func_reset(void)
{
	// if reset during delayed CD change, re-insert the CD immediately
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct blkdevstate *st = &state[i];
		if (st->imagechangetime > 0 && st->newimagefile[0] && !currprefs.cdslots[i].name[0]) {
			_tcscpy(changed_prefs.cdslots[i].name, st->newimagefile);
			_tcscpy(currprefs.cdslots[i].name, st->newimagefile);
			//cd_statusline_label(i);
		}
		st->imagechangetime = 0;
		st->newimagefile[0] = 0;
		//st->mediawaschanged = false;
		st->waspaused = false;
	}
}

void device_func_free(void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct blkdevstate *st = &state[i];
		st->wasopen = 0;
		st->waspaused = false;
		st->imagechangetime = 0;
		st->cdimagefileinuse = false;
		st->newimagefile[0] = 0;
	}
	dev_init = false;
}

static int device_func_init (int flags)
{
	blkdev_fix_prefs (&currprefs);
	install_driver (flags);
	dev_init = true;
	return 1;
}

bool blkdev_get_info(struct uae_prefs* p, int unitnum, struct device_info* di)
{
	bool open = true, opened = false, ok = false;
	struct blkdevstate* st = &state[unitnum];
	if (!st->isopen) {
		blkdev_fix_prefs(p);
		install_driver(0);
		opened = true;
		open = sys_command_open_internal(unitnum, p->cdslots[unitnum].name[0] ? p->cdslots[unitnum].name : NULL, CD_STANDARD_UNIT_DEFAULT) != 0;
	}
	if (open) {
		ok = sys_command_info(unitnum, di, true) != 0;
	}
	if (open && opened)
		sys_command_close_internal(unitnum);
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

void check_prefs_changed_cd(void)
{
	if (!config_changed)
		return;
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
	if (gotsem) {
		freesem (unitnum);
		gotsem = false;
	}

	set_config_changed ();
}

void blkdev_vsync (void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++)
		check_changes (i);
}

static int failunit (int unitnum)
{
	if (unitnum < 0 || unitnum >= MAX_TOTAL_SCSI_DEVICES)
		return 1;
	if (state[unitnum].device_func == NULL)
		return 1;
	return 0;
}

/* pause/unpause CD audio */
int sys_command_cd_pause (int unitnum, int paused)
{
	if (failunit (unitnum))
		return -1;
	if (!getsem (unitnum))
		return 0;
	int v;
	v = state[unitnum].device_func->pause (unitnum, paused);
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
	state[unitnum].device_func->stop (unitnum);
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
	v = state[unitnum].device_func->play (unitnum, startlsn, endlsn, scan, NULL, NULL);
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
	state[unitnum].play_end_pos = endlsn;
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
	v = state[unitnum].device_func->volume (unitnum, volume_left, volume_right);
	freesem (unitnum);
	return v;
}

/* read qcode */
int sys_command_cd_qcode (int unitnum, uae_u8 *buf, int sector, bool all)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	v = state[unitnum].device_func->qcode (unitnum, buf, sector, all);
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
	v = state[unitnum].device_func->toc (unitnum, toc);
	freesem (unitnum);
	return v;
}

/* read one cd sector */
static int sys_command_cd_read (int unitnum, uae_u8 *data, int block, int size)
{
	int v;
	if (failunit (unitnum))
		return 0;
	if (!getsem (unitnum))
		return 0;
	v = state[unitnum].device_func->read (unitnum, data, block, size);
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
	v = state[unitnum].device_func->rawread (unitnum, data, block, size, sectorsize, 0xffffffff);
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
	v = state[unitnum].device_func->rawread (unitnum, data, block, size, sectorsize, (sectortype << 16) | (scsicmd9 << 8) | subs);
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
	v = state[unitnum].device_func->ismedia (unitnum, quick);
	freesem (unitnum);
	return v;
}

static struct device_info *sys_command_info_session (int unitnum, struct device_info *di, int quick, int session)
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
	struct device_info *dix;

	dix = sys_command_info_session (unitnum, di, quick, -1);
	if (dix && dix->media_inserted && !quick && !dix->audio_playing) {
		TCHAR *name = NULL;
		uae_u8 buf[2048];
		if (sys_command_cd_read(unitnum, buf, 16, 1)) {
			if ((buf[0] == 1 || buf[0] == 2) && !memcmp(buf + 1, "CD001", 5)) {
				TCHAR *p;
				au_copy(dix->volume_id, 32, (uae_char*)buf + 40);
				au_copy(dix->system_id, 32, (uae_char*)buf + 8);
				p = dix->volume_id + _tcslen(dix->volume_id) - 1;
				while (p > dix->volume_id && *p == ' ')
					*p-- = 0;
				p = dix->system_id + _tcslen(dix->system_id) - 1;
				while (p > dix->system_id && *p == ' ')
					*p-- = 0;
			}
		}
	}
	return dix; 
}

#ifdef SAVESTATE

void restore_blkdev_start(void)
{
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct cdslot *cd = &currprefs.cdslots[i];
		if (cd->temporary) {
			memset(cd, 0, sizeof(struct cdslot));
			memset(&changed_prefs.cdslots[i], 0, sizeof(struct cdslot));
		}
	}
}

uae_u8 *save_cd (int num, int *len)
{
	struct blkdevstate *st = &state[num];
	uae_u8 *dstbak, *dst;

	memset(st->play_qcode, 0, SUBQ_SIZE);
	if (!currprefs.cdslots[num].inuse || num >= MAX_TOTAL_SCSI_DEVICES)
		return NULL;
	if (!currprefs.cs_cd32cd)
		return NULL;
	dstbak = dst = xmalloc (uae_u8, 4 + MAX_DPATH + 4 + 4 + 4 + 2 * MAX_DPATH);
	save_u32 (4 | 8 | 16);
	save_path (currprefs.cdslots[num].name, SAVESTATE_PATH_CD);
	save_u32 (currprefs.cdslots[num].type);
	save_u32 (0);
	save_u32 (0);
	sys_command_cd_qcode (num, st->play_qcode, -1, false);
	for (int i = 0; i < SUBQ_SIZE; i++)
		save_u8 (st->play_qcode[i]);
	save_u32 (0);
	save_path_full(currprefs.cdslots[num].name, SAVESTATE_PATH_CD);
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
	if (flags & 8) {
		restore_u32 ();
		for (int i = 0; i < SUBQ_SIZE; i++)
			st->play_qcode[i] = restore_u8 ();
		restore_u32 ();
	}
	if (flags & 16) {
		xfree(s);
		s = restore_path_full();
	}
	if (flags & 4) {
		if (currprefs.cdslots[num].name[0] == 0 || zfile_exists (s)) {
		  _tcscpy (changed_prefs.cdslots[num].name, s);
		  _tcscpy (currprefs.cdslots[num].name, s);
		}
		changed_prefs.cdslots[num].type = currprefs.cdslots[num].type = type;
		changed_prefs.cdslots[num].temporary = currprefs.cdslots[num].temporary = true;
	}
	xfree(s);
	return src;
}

#endif

