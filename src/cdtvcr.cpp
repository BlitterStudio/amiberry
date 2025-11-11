/*
* UAE - The Un*x Amiga Emulator
*
* CDTV-CR emulation
* (4510 microcontroller only simulated)
*
* Copyright 2014 Toni Wilen
*
*
*/

#define CDTVCR_4510_EMULATION 0

#define CDTVCR_DEBUG 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "debug.h"
#include "zfile.h"
#include "gui.h"
#include "cdtvcr.h"
#include "blkdev.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "custom.h"
#include "devices.h"

#if CDTVCR_4510_EMULATION
static void init_65c02(void);
static void cpu_4510(void);
#endif

#define CDTVCR_MASK 0xffff

#define CDTVCR_RAM_OFFSET 0x1000
#define CDTVCR_RAM_SIZE 4096
#define CDTVCR_RAM_MASK (CDTVCR_RAM_SIZE - 1)

#define CDTVCR_CLOCK 0xc10
#define CDTVCR_ID 0x9dc

#define CDTVCR_RESET 0xc00

#define CDTVCR_CD_CMD 0xc40
#define CDTVCR_CD_CMD_DO 0xc52
#define CDTVCR_CD_CMD_STATUS 0xc4e
#define CDTVCR_CD_CMD_STATUS2 0xc4f

#define CDTVCR_CD_STATE 0xc53
#define CDTVCR_SYS_STATE 0xc54
#define CDTVCR_CD_SPEED 0xc59
#define CDTVCR_CD_PLAYING 0xc5b
#define CDTVCR_CD_SUBCODES 0xc60
#define CDTVCR_CD_ALLOC 0xc61

#define CDTVCR_INTDISABLE 0xc04
#define CDTVCR_INTACK 0xc05
#define CDTVCR_INTENA 0xc55
#define CDTVCR_INTREQ 0xc56
#define CDTVCR_4510_TRIGGER 0x4000

#define CDTVCR_KEYCMD 0xc80

#define CDTVCR_SUBQ 0x906
#define CDTVCR_SUBBANK 0x917
#define CDTVCR_SUBC 0x918
#define CDTVCR_TOC 0xa00

#define CDTVCR_PLAYLIST_TRACK 0xc2f
#define CDTVCR_PLAYLIST_TIME_MINS 0xc35
#define CDTVCR_PLAYLIST_TIME_SECS 0xc36
#define CDTVCR_PLAYLIST_TIME_MODE 0xc22
#define CDTVCR_PLAYLIST_TIME_MODE2 0xc84
#define CDTVCR_PLAYLIST_STATE 0xc86
#define CDTVCR_PLAYLIST_CURRENT 0xc8a
#define CDTVCR_PLAYLIST_ENTRIES 0xc8b
#define CDTVCR_PLAYLIST_DATA 0xc8c


static uae_u8 cdtvcr_4510_ram[CDTVCR_RAM_OFFSET];
static uae_u8 cdtvcr_ram[CDTVCR_RAM_SIZE];
static uae_u8 cdtvcr_clock[2];

static smp_comm_pipe requests;
static uae_sem_t sub_sem;
static volatile int thread_alive;
static int unitnum = -1;
static struct cd_toc_head toc;
static int datatrack;
static int cdtvcr_media;
static int subqcnt;
static int cd_audio_status;
static int cdtvcr_wait_sectors;
static int cd_led;

#define MAX_SUBCODEBUFFER 36
static volatile int subcodebufferoffset, subcodebufferoffsetw;
static uae_u8 subcodebufferinuse[MAX_SUBCODEBUFFER];
static uae_u8 subcodebuffer[MAX_SUBCODEBUFFER * SUB_CHANNEL_SIZE];
static TCHAR flashfilepath[MAX_DPATH];

static void cdtvcr_battram_reset (void)
{
	struct zfile *f;
	size_t v;

	memset (cdtvcr_ram, 0, CDTVCR_RAM_SIZE);
	cfgfile_resolve_path_out_load(currprefs.flashfile, flashfilepath, MAX_DPATH, PATH_ROM);
	f = zfile_fopen (flashfilepath, _T("rb+"), ZFD_NORMAL);
	if (!f) {
		f = zfile_fopen (flashfilepath, _T("wb"), 0);
		if (f) {
			zfile_fwrite (cdtvcr_ram, CDTVCR_RAM_SIZE, 1, f);
			zfile_fclose (f);
		}
		return;
	}
	v = zfile_fread (cdtvcr_ram, 1, CDTVCR_RAM_SIZE, f);
	if (v < CDTVCR_RAM_SIZE)
		zfile_fwrite (cdtvcr_ram + v, 1, CDTVCR_RAM_SIZE - v, f);
	zfile_fclose (f);
}

static void cdtvcr_battram_write (int addr, int v)
{
	struct zfile *f;
	int offset = addr & CDTVCR_RAM_MASK;

	if (offset >= CDTVCR_RAM_SIZE)
		return;
	gui_flicker_led (LED_MD, 0, 2);
	if (cdtvcr_ram[offset] == v)
		return;
	cdtvcr_ram[offset] = v;
	f = zfile_fopen (flashfilepath, _T("rb+"), ZFD_NORMAL);
	if (!f)
		return;
	zfile_fseek (f, offset, SEEK_SET);
	zfile_fwrite (cdtvcr_ram + offset, 1, 1, f);
	zfile_fclose (f);
}

static uae_u8 cdtvcr_battram_read (int addr)
{
	uae_u8 v;
	int offset;
	offset = addr & CDTVCR_RAM_MASK;
	if (offset >= CDTVCR_RAM_SIZE)
		return 0;
	gui_flicker_led (LED_MD, 0, 1);
	v = cdtvcr_ram[offset];
	return v;
}

static int ismedia (void)
{
	cdtvcr_4510_ram[CDTVCR_SYS_STATE] &= ~8;
	if (unitnum < 0)
		return 0;
	if (sys_command_ismedia (unitnum, 0)) {
		cdtvcr_4510_ram[CDTVCR_SYS_STATE] |= 8;
		return 1;
	}
	return 0;
}

static int get_qcode (void)
{
#if 0
	if (!sys_command_cd_qcode (unitnum, cdrom_qcode))
		return 0;
	cdrom_qcode[1] = cd_audio_status;
#endif
	return 1;
}

static int get_toc (void)
{
	uae_u32 msf;
	uae_u8 *p, *pl;

	cdtvcr_4510_ram[CDTVCR_SYS_STATE] &= ~4;
	datatrack = 0;
	if (!sys_command_cd_toc (unitnum, &toc))
		return 0;
	cdtvcr_4510_ram[CDTVCR_SYS_STATE] |= 4 | 8;
	if (toc.first_track == 1 && (toc.toc[toc.first_track_offset].control & 0x0c) == 0x04)
		datatrack = 1;
	p = &cdtvcr_4510_ram[CDTVCR_TOC];
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_CURRENT] = 0;
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_ENTRIES] = 0;
	pl = &cdtvcr_4510_ram[CDTVCR_PLAYLIST_DATA];
	p[0] = toc.first_track;
	p[1] = toc.last_track;
	msf = lsn2msf(toc.lastaddress);
	p[2] = msf >> 16;
	p[3] = msf >>  8;
	p[4] = msf >>  0;
	p += 5;
	for (int j = toc.first_track_offset; j <= toc.last_track_offset; j++) {
		struct cd_toc *s = &toc.toc[j];
		p[0] = (s->adr << 0) | (s->control << 4);
		p[1] = s->track;
		msf = lsn2msf(s->address);
		p[2] = msf >> 16;
		p[3] = msf >>  8;
		p[4] = msf >>  0;
		p += 5;
		*pl++ = s->track | 0x80;
		cdtvcr_4510_ram[CDTVCR_PLAYLIST_ENTRIES]++;
	}
	return 1;
}

static void cdtvcr_4510_reset(uae_u8 v)
{
	cdtvcr_4510_ram[CDTVCR_ID + 0] = 'C';
	cdtvcr_4510_ram[CDTVCR_ID + 1] = 'D';
	cdtvcr_4510_ram[CDTVCR_ID + 2] = 'T';
	cdtvcr_4510_ram[CDTVCR_ID + 3] = 'V';

	write_log(_T("4510 reset %d\n"), v);
	if (v == 3) {
		sys_command_cd_pause (unitnum, 0);
		sys_command_cd_stop (unitnum);
		cdtvcr_4510_ram[CDTVCR_CD_PLAYING] = 0;
		cdtvcr_4510_ram[CDTVCR_CD_STATE] = 0;
		return;
	} else if (v == 2 || v == 1) {
		cdtvcr_4510_ram[CDTVCR_INTENA] = 0;
		cdtvcr_4510_ram[CDTVCR_INTREQ] = 0;
		if (v == 1) {
			memset(cdtvcr_4510_ram, 0, 4096);
		}
		cdtvcr_4510_ram[CDTVCR_INTDISABLE] = 1;
		cdtvcr_4510_ram[CDTVCR_CD_STATE] = 1;
	}
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_TIME_MODE] = 2;
	uae_sem_wait (&sub_sem);
	memset (subcodebufferinuse, 0, sizeof subcodebufferinuse);
	subcodebufferoffsetw = 0;
	subcodebufferoffset = 0;
	uae_sem_post (&sub_sem);

	if (ismedia())
		get_toc();
}

static void rethink_cdtvcr(void)
{
	if ((cdtvcr_4510_ram[CDTVCR_INTREQ] & cdtvcr_4510_ram[CDTVCR_INTENA]) && !cdtvcr_4510_ram[CDTVCR_INTDISABLE]) {
		safe_interrupt_set(IRQ_SOURCE_CD32CDTV, 0, false);
		cd_led ^= LED_CD_ACTIVE2;
	}
}

static void cdtvcr_cmd_done(void)
{
	cdtvcr_4510_ram[CDTVCR_SYS_STATE] &= ~3;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_DO] = 0;
	cdtvcr_4510_ram[CDTVCR_INTREQ] |= 0x40;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS2] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS + 2] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS2 + 2] = 0;
}

static void cdtvcr_play_done(void)
{
	cdtvcr_4510_ram[CDTVCR_SYS_STATE] &= ~3;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_DO] = 0;
	cdtvcr_4510_ram[CDTVCR_INTREQ] |= 0x80;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS2] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS + 2] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS2 + 2] = 0;
}

static void cdtvcr_state_stopped(void)
{
	cdtvcr_4510_ram[CDTVCR_CD_PLAYING] = 0;
	cdtvcr_4510_ram[CDTVCR_CD_STATE] = 1;
}

static void cdtvcr_state_play(void)
{
	cdtvcr_4510_ram[CDTVCR_CD_PLAYING] = 1;
	cdtvcr_4510_ram[CDTVCR_CD_STATE] = 3;
}

static void cdtvcr_cmd_play_started(void)
{
	cdtvcr_cmd_done();
	cdtvcr_state_play();
}

static void cdtvcr_start (void)
{
	if (unitnum < 0)
		return;
	cdtvcr_4510_ram[CDTVCR_CD_STATE] = 0;
}

static void cdtvcr_stop (void)
{
	if (unitnum < 0)
		return;
	sys_command_cd_pause (unitnum, 0);
	sys_command_cd_stop (unitnum);
	if (cdtvcr_4510_ram[CDTVCR_CD_PLAYING]) {
		cdtvcr_4510_ram[CDTVCR_INTREQ] |= 0x80;
	}
	cdtvcr_state_stopped();
}

static void cdtvcr_state_pause(bool pause)
{
	cdtvcr_4510_ram[CDTVCR_CD_STATE] = 3;
	if (pause)
		cdtvcr_4510_ram[CDTVCR_CD_STATE] = 2;
}

static void cdtvcr_pause(bool pause)
{
	sys_command_cd_pause (unitnum, pause ? 1 : 0);
	cdtvcr_state_pause(pause);
	cdtvcr_cmd_done();
}

static void setsubchannel(uae_u8 *s)
{
	uae_u8 *d;

	// q-channel
	d = &cdtvcr_4510_ram[CDTVCR_SUBQ];
	s += SUB_ENTRY_SIZE;
	int track = frombcd(s[1]);
	/* CtlAdr */
	d[0] = s[0];
	/* Track */
	d[1] = s[1];
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_TRACK] = track;
	/* Index */
	d[2] = s[2];
	/* TrackPos */
	d[3] = s[3];
	d[4] = s[4];
	d[5] = s[5];
	/* DiskPos */
	d[6] = s[6];
	d[7] = s[7];
	d[8] = s[8];
	d[9] = s[9];

	uae_u8 mins = 0, secs = 0;
	uae_s32 trackpos = msf2lsn(fromlongbcd(s + 3));
	if (trackpos < 0)
		trackpos = 0;
	uae_s32 diskpos = msf2lsn(fromlongbcd(s + 7));
	if (diskpos < 0)
		diskpos = 0;
	switch (cdtvcr_4510_ram[CDTVCR_PLAYLIST_TIME_MODE2])
	{
		case 0:
		secs = (trackpos / 75) % 60;
		mins = trackpos / (75 * 60);
		break;
		case 1:
		trackpos = toc.toc[toc.first_track_offset + track].paddress - diskpos;
		secs = (trackpos / 75) % 60;
		mins = trackpos / (75 * 60);
		break;
		case 2:
		secs = (diskpos / 75) % 60;
		mins = diskpos / (75 * 60);
		break;
		case 3:
		diskpos = toc.lastaddress - diskpos;
		secs = (diskpos / 75) % 60;
		mins = diskpos / (75 * 60);
		break;
	}
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_TIME_MINS] = mins;
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_TIME_SECS] = secs;

	cdtvcr_4510_ram[CDTVCR_SUBQ - 2] = 1; // qcode valid
	cdtvcr_4510_ram[CDTVCR_SUBQ - 1] = 0;

	if (cdtvcr_4510_ram[CDTVCR_CD_SUBCODES])
		cdtvcr_4510_ram[CDTVCR_INTREQ] |= 4;
}

static void subfunc(uae_u8 *data, int cnt)
{
	uae_u8 out[SUB_CHANNEL_SIZE];
	sub_to_deinterleaved(data, out);
	setsubchannel(out);

	uae_sem_wait(&sub_sem);
	if (subcodebufferinuse[subcodebufferoffsetw]) {
		memset (subcodebufferinuse, 0,sizeof (subcodebufferinuse));
		subcodebufferoffsetw = 0;
		subcodebufferoffset = 0;
	} else {
		int offset = subcodebufferoffsetw;
		while (cnt > 0) {
			if (subcodebufferinuse[offset]) {
				write_log (_T("CD32: subcode buffer overflow 2\n"));
				break;
			}
			subcodebufferinuse[offset] = 1;
			memcpy (&subcodebuffer[offset * SUB_CHANNEL_SIZE], data, SUB_CHANNEL_SIZE);
			data += SUB_CHANNEL_SIZE;
			offset++;
			if (offset >= MAX_SUBCODEBUFFER)
				offset = 0;
			cnt--;
		}
		subcodebufferoffsetw = offset;
	}
	uae_sem_post(&sub_sem);
}

static int statusfunc(int status, int playpos)
{
	if (status == -1)
		return 75;
	if (status == -2)
		return 75;
	if (status < 0)
		return 0;
	if (cd_audio_status != status) {
		if (status == AUDIO_STATUS_PLAY_COMPLETE || status == AUDIO_STATUS_PLAY_ERROR) {
			cdtvcr_play_done();
		} else if (status == AUDIO_STATUS_IN_PROGRESS) {
			cdtvcr_cmd_play_started();
		}
	}
	cd_audio_status = status;
	return 0;
}

static void cdtvcr_play(uae_u32 start, uae_u32 end)
{
	sys_command_cd_pause(unitnum, 0);
	if (!sys_command_cd_play(unitnum, start, end, 0, statusfunc, subfunc))
		cdtvcr_play_done();
}

static void cdtvcr_play_track(uae_u32 track_start, uae_u32 track_end)
{
	int start_found, end_found;
	uae_u32 start, end;

	start_found = end_found = 0;
	for (int j = toc.first_track_offset; j <= toc.last_track_offset; j++) {
		struct cd_toc *s = &toc.toc[j];
		if (track_start == s->track) {
			start_found++;
			start = s->paddress;
			end = toc.toc[toc.last_track_offset].paddress;
		}
		if (track_end == s->track) {
			end = s->paddress;
			end_found++;
		}
	}
	if (start_found == 0) {
		write_log (_T("PLAY CD AUDIO: illegal start track %d\n"), track_start);
		cdtvcr_stop();
		cdtvcr_cmd_done();
		return;
	}
	cdtvcr_play(start, end);
}

static void cdtvcr_read_data(uae_u32 start, uae_u32 addr, uae_u32 len)
{
	uae_u8 buffer[2048];
	int didread;
	
	cdtvcr_wait_sectors = 0;
	while (len) {
		didread = sys_command_cd_read (unitnum, buffer, start, 1);
		if (!didread)
			break;
		for (int i = 0; i < 2048 && len > 0; i++) {
			put_byte(addr + i, buffer[i]);
			len--;
		}
		addr += 2048;
		start++;
		cdtvcr_wait_sectors++;
	}
}

static void cdtvcr_player_state_change(void)
{
	cdtvcr_4510_ram[CDTVCR_INTREQ] |= 0x10;
}

static void cdtvcr_player_pause(void)
{
	if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] == 4)
		cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] = 3;
	else if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] == 3)
		cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] = 4;
	else
		return;
	cdtvcr_pause(cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] == 4);
	cdtvcr_state_pause(cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] == 4);
	cdtvcr_player_state_change();
}

static void cdtvcr_player_stop(void)
{
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] = 1;
	cdtvcr_stop();
	cdtvcr_state_stopped();
	cdtvcr_player_state_change();
}

static void cdtvcr_player_play(void)
{
	int start_found, end_found;
	uae_u32 start, end;

	int entry = cdtvcr_4510_ram[CDTVCR_PLAYLIST_CURRENT];
	int track = cdtvcr_4510_ram[CDTVCR_PLAYLIST_DATA + entry] & 0x7f;
	start_found = end_found = 0;
	for (int j = toc.first_track_offset; j <= toc.last_track_offset; j++) {
		struct cd_toc *s = &toc.toc[j];
		if (track == s->track) {
			start_found++;
			start = s->paddress;
			end = s[1].paddress;
		}
	}
	if (start_found == 0) {
		cdtvcr_player_stop();
		return;
	}
	cdtvcr_play(start, end);
	cdtvcr_state_play();
	cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] = 3;
	cdtvcr_player_state_change();
}

static void cdtvcr_do_cmd(void)
{
	uae_u32 addr, len, start, end, datalen;
	uae_u32 startlsn, endlsn;
	uae_u8 starttrack, endtrack;
	uae_u8 *p = &cdtvcr_4510_ram[CDTVCR_CD_CMD];

	cdtvcr_4510_ram[CDTVCR_SYS_STATE] |= 2;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS] = 2;
	cdtvcr_4510_ram[CDTVCR_CD_CMD_STATUS2] = 0;
	write_log(_T("CDTVCR CD command %02x\n"), p[0]);
	for(int i = 0; i < 14; i++)
		write_log(_T(".%02x"), p[i]);
	write_log(_T("\n"));

	start = (p[1] << 16) | (p[2] << 8) | (p[3] << 0);
	startlsn = msf2lsn(start);
	end = (p[4] << 16) | (p[5] << 8) | (p[6] << 0);
	endlsn = msf2lsn(end);
	addr = (p[7] << 24) | (p[8] << 16) | (p[9] << 8) | (p[10] << 0);
	len = (p[4] << 16) | (p[5] << 8) | (p[6] << 0);
	datalen = (p[11] << 16) | (p[12] << 8) | (p[13] << 0);
	starttrack = p[1];
	endtrack = p[3];

	switch(p[0])
	{
		case 2: // start
		cdtvcr_start();
		cdtvcr_cmd_done();
		break;
		case 3: // stop
		cdtvcr_stop();
		cdtvcr_cmd_done();
		break;
		case 4: // toc
		cdtvcr_stop();
		get_toc();
		cdtvcr_cmd_done();
		break;
		case 6: // play
		cdtvcr_stop();
		cdtvcr_play(startlsn, endlsn);
		break;
		case 7: // play track
		cdtvcr_stop();
		cdtvcr_play_track(starttrack, endtrack);
		break;
		case 8: // read
		cdtvcr_stop();
		cdtvcr_read_data(start, addr, datalen);
		break;
		case 9:
		cdtvcr_pause(true);
		break;
		case 10:
		cdtvcr_pause(false);
		break;
		case 11: // stop play
		cdtvcr_stop();
		cdtvcr_cmd_done();
		break;
		default:
		write_log(_T("unsupported command!\n"));
		break;
	}
}

static void cdtvcr_4510_do_something(void)
{
	if (cdtvcr_4510_ram[CDTVCR_INTACK]) {
		cdtvcr_4510_ram[CDTVCR_INTACK] = 0;
		rethink_cdtvcr();
	}
	if (cdtvcr_4510_ram[CDTVCR_CD_CMD_DO]) {
		cdtvcr_4510_ram[CDTVCR_CD_CMD_DO] = 0;
		cdtvcr_4510_ram[CDTVCR_SYS_STATE] |= 2;
		write_comm_pipe_u32 (&requests, 0x0100, 1);
	}
}

static bool cdtvcr_debug(uaecptr addr)
{
	addr &= CDTVCR_MASK;
	return !(addr >= CDTVCR_RAM_OFFSET && addr < CDTVCR_RAM_OFFSET + CDTVCR_RAM_SIZE);
}

static void cdtvcr_bput2 (uaecptr addr, uae_u8 v)
{
	addr &= CDTVCR_MASK;
	if (addr == CDTVCR_4510_TRIGGER) {
		if (v & 0x80)
			cdtvcr_4510_do_something();
	} else if (addr >= CDTVCR_RAM_OFFSET && addr < CDTVCR_RAM_OFFSET + CDTVCR_RAM_SIZE) {
		cdtvcr_battram_write(addr - CDTVCR_RAM_OFFSET, v);
	} else if (addr >= CDTVCR_CLOCK && addr < CDTVCR_CLOCK + 0x20) {
		int reg = addr - CDTVCR_CLOCK;
		switch (reg)
		{
			case 0:
			cdtvcr_clock[0] = v;
			case 1:
			cdtvcr_clock[1] = v;
			break;
		}
	} else {
		switch(addr)
		{
			case CDTVCR_RESET:
			cdtvcr_4510_reset(v);
			break;
		}
	}
	if (addr >= 0xc00 && addr < CDTVCR_RAM_OFFSET) {
		switch (addr)
		{
			case CDTVCR_KEYCMD:
			{
				uae_u8 keycode = cdtvcr_4510_ram[CDTVCR_KEYCMD + 1];
				write_log(_T("Got keycode %x\n"), keycode);
				cdtvcr_4510_ram[CDTVCR_KEYCMD + 1] = 0;
				switch(keycode)
				{
					case 0x73: // PLAY
					if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] >= 3) {
						cdtvcr_player_pause();
					} else {
						cdtvcr_player_play();
					}
					break;
					case 0x72: // STOP
					cdtvcr_player_stop();
					break;
					case 0x74: // <-
					if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_CURRENT] > 0) {
						cdtvcr_4510_ram[CDTVCR_PLAYLIST_CURRENT]--;
						if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] >= 3) {
							cdtvcr_player_play();
						}
					}
					break;
					case 0x75: // ->
					if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_CURRENT] < cdtvcr_4510_ram[CDTVCR_PLAYLIST_ENTRIES] - 1) {
						cdtvcr_4510_ram[CDTVCR_PLAYLIST_CURRENT]++;
						if (cdtvcr_4510_ram[CDTVCR_PLAYLIST_STATE] >= 3) {
							cdtvcr_player_play();
						}
					}
					break;
				}
				v = 0;
				break;
			}
		}
		cdtvcr_4510_ram[addr] = v;

		if (addr >= 0xc80)
			write_log(_T("%04x %02x\n"), addr, v);

	}
}

static uae_u8 cdtvcr_bget2 (uaecptr addr)
{
	uae_u8 v = 0;
	addr &= CDTVCR_MASK;
	if (addr < CDTVCR_RAM_OFFSET) {
		v = cdtvcr_4510_ram[addr];
	}
	if (addr >= CDTVCR_RAM_OFFSET && addr < CDTVCR_RAM_OFFSET + CDTVCR_RAM_SIZE) {
		v = cdtvcr_battram_read(addr - CDTVCR_RAM_OFFSET);
	} else if (addr >= CDTVCR_CLOCK && addr < CDTVCR_CLOCK + 0x20) {
		int reg = addr - CDTVCR_CLOCK;
		int days, mins, ticks;
		int tickcount = currprefs.ntscmode ? 60 : 50;
		struct timeval tv;
		struct mytimeval mtv;
		gettimeofday (&tv, NULL);
		#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
		// On BSD/macOS use tm_gmtoff
		struct tm *lt = localtime(&tv.tv_sec);
		tv.tv_sec -= lt->tm_gmtoff;
		#elif defined(__linux__)
		// On Linux use timezone variable
		extern long timezone;
		tv.tv_sec -= timezone;
		#else
		// fallback or no adjustment
		#endif
		mtv.tv_sec = tv.tv_sec;
		mtv.tv_usec = tv.tv_usec;
		timeval_to_amiga(&mtv, &days, &mins, &ticks, tickcount);
		switch (reg)
		{
			case 0:
			case 1:
			v = cdtvcr_clock[reg];
			break;
			case 2:
			v = days >> 8;
			break;
			case 3:
			v = days;
			break;
			case 4:
			v = mins / 60;
			break;
			case 5:
			v = mins % 60;
			break;
			case 6:
			v = ticks / tickcount;
			break;
			case 7:
			v = ticks % tickcount;
			break;
			case 8:
			v = tickcount;
			break;

		}
	} else {
		switch(addr)
		{
			case CDTVCR_RESET:
			v = 0;
			break;
		}
	}
	return v;
}

static uae_u32 REGPARAM2 cdtvcr_lget (uaecptr addr)
{
	uae_u32 v;
	v = (cdtvcr_bget2 (addr) << 24) | (cdtvcr_bget2 (addr + 1) << 16) |
		(cdtvcr_bget2 (addr + 2) << 8) | (cdtvcr_bget2 (addr + 3));
#if CDTVCR_DEBUG
	if (cdtvcr_debug(addr))
		write_log (_T("cdtvcr_lget %08X=%08X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 REGPARAM2 cdtvcr_wgeti (uaecptr addr)
{
	uae_u32 v = 0xffff;
	return v;
}
static uae_u32 REGPARAM2 cdtvcr_lgeti (uaecptr addr)
{
	uae_u32 v = 0xffff;
	return v;
}

static uae_u32 REGPARAM2 cdtvcr_wget (uaecptr addr)
{
	uae_u32 v;
	v = (cdtvcr_bget2 (addr) << 8) | cdtvcr_bget2 (addr + 1);
#if CDTVCR_DEBUG
	if (cdtvcr_debug(addr))
		write_log (_T("cdtvcr_wget %08X=%04X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 REGPARAM2 cdtvcr_bget (uaecptr addr)
{
	uae_u32 v;
	v = cdtvcr_bget2 (addr);
#if CDTVCR_DEBUG
	if (cdtvcr_debug(addr))
		write_log (_T("cdtvcr_bget %08X=%02X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static void REGPARAM2 cdtvcr_lput (uaecptr addr, uae_u32 l)
{
#if CDTVCR_DEBUG
	if (cdtvcr_debug(addr))
		write_log (_T("cdtvcr_lput %08X=%08X PC=%08X\n"), addr, l, M68K_GETPC);
#endif
	cdtvcr_bput2 (addr, l >> 24);
	cdtvcr_bput2 (addr + 1, l >> 16);
	cdtvcr_bput2 (addr + 2, l >> 8);
	cdtvcr_bput2 (addr + 3, l);
}

static void REGPARAM2 cdtvcr_wput (uaecptr addr, uae_u32 w)
{
#if CDTVCR_DEBUG
	if (cdtvcr_debug(addr))
		write_log (_T("cdtvcr_wput %08X=%04X PC=%08X\n"), addr, w & 65535, M68K_GETPC);
#endif
	cdtvcr_bput2 (addr, w >> 8);
	cdtvcr_bput2 (addr + 1, w);
}

static void REGPARAM2 cdtvcr_bput (uaecptr addr, uae_u32 b)
{
#if CDTVCR_DEBUG
	if (cdtvcr_debug(addr))
		write_log (_T("cdtvcr_bput %08X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif
	cdtvcr_bput2 (addr, b);
}


static addrbank cdtvcr_bank = {
	cdtvcr_lget, cdtvcr_wget, cdtvcr_bget,
	cdtvcr_lput, cdtvcr_wput, cdtvcr_bput,
	default_xlate, default_check, NULL, NULL, _T("CDTV-CR"),
	cdtvcr_lgeti, cdtvcr_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static int dev_thread (void *p)
{
	write_log (_T("CDTV-CR: CD thread started\n"));
	thread_alive = 1;
	for (;;) {

		uae_u32 b = read_comm_pipe_u32_blocking (&requests);
		if (b == 0xffff) {
			thread_alive = -1;
			return 0;
		}
		if (unitnum < 0)
			continue;
		switch (b)
		{
			case 0x0100:
				cdtvcr_do_cmd();
			break;
			case 0x0101:
			{
				int m = ismedia ();
				if (m < 0) {
					write_log (_T("CDTV: device %d lost\n"), unitnum);
					cdtvcr_4510_ram[CDTVCR_SYS_STATE] &= ~(4 | 8);
					cdtvcr_4510_ram[CDTVCR_INTREQ] |= 0x10 | 8;
					cdtvcr_4510_ram[CDTVCR_CD_STATE] = 0;
				} else if (m != cdtvcr_media) {
					cdtvcr_media = m;
					get_toc ();
					cdtvcr_4510_ram[CDTVCR_INTREQ] |= 0x10 | 8;
					if (cdtvcr_media) {
						cdtvcr_4510_ram[CDTVCR_CD_STATE] = 2;
					} else {
						cdtvcr_4510_ram[CDTVCR_CD_STATE] = 1;
						cdtvcr_4510_ram[CDTVCR_SYS_STATE] &= ~(4 | 8);
					}
				}
				if (cdtvcr_media)
					get_qcode ();
			}
			break;
		}
	}
}

static void CDTVCR_hsync_handler (void)
{
	static int subqcnt, readcnt;

	if (!currprefs.cs_cdtvcr)
		return;

#if CDTVCR_4510_EMULATION
	for (int i = 0; i < 10; i++) {
		cpu_4510();
	}
#endif

	if (cdtvcr_wait_sectors > 0 && currprefs.cd_speed == 0) {
		cdtvcr_wait_sectors = 0;
		cdtvcr_cmd_done();
	}
	readcnt--;
	if (readcnt <= 0) {
		int cdspeed = cdtvcr_4510_ram[CDTVCR_CD_SPEED] ? 150 : 75;
		readcnt = (int)(maxvpos * vblank_hz / cdspeed);
		if (cdtvcr_wait_sectors > 0) {
			cdtvcr_wait_sectors--;
			if (cdtvcr_wait_sectors == 0)
				cdtvcr_cmd_done();
		}
	}
	subqcnt--;
	if (subqcnt <= 0) {
		write_comm_pipe_u32 (&requests, 0x0101, 1);
		subqcnt = (int)(maxvpos * vblank_hz / 75 - 1);
		if (subcodebufferoffset != subcodebufferoffsetw) {
			uae_sem_wait (&sub_sem);
			cdtvcr_4510_ram[CDTVCR_SUBBANK] = cdtvcr_4510_ram[CDTVCR_SUBBANK] ? 0 : SUB_CHANNEL_SIZE + 2;
			uae_u8 *d = &cdtvcr_4510_ram[CDTVCR_SUBC];
			d[cdtvcr_4510_ram[CDTVCR_SUBBANK] + SUB_CHANNEL_SIZE + 0] = 0x1f;
			d[cdtvcr_4510_ram[CDTVCR_SUBBANK] + SUB_CHANNEL_SIZE + 1] = 0x3d;
			for (int i = 0; i < SUB_CHANNEL_SIZE; i++) {
				d[cdtvcr_4510_ram[CDTVCR_SUBBANK] + i] = subcodebuffer[subcodebufferoffset * SUB_CHANNEL_SIZE + i] & 0x3f;
			}
			subcodebufferinuse[subcodebufferoffset] = 0;
			subcodebufferoffset++;
			if (subcodebufferoffset >= MAX_SUBCODEBUFFER)
				subcodebufferoffset -= MAX_SUBCODEBUFFER;
			uae_sem_post (&sub_sem);
			if (cdtvcr_4510_ram[CDTVCR_CD_SUBCODES])
				cdtvcr_4510_ram[CDTVCR_INTREQ] |= 2;
		} else if (cdtvcr_4510_ram[CDTVCR_CD_SUBCODES] && !cdtvcr_4510_ram[CDTVCR_CD_PLAYING]) {
			// want subcodes but not playing?
			// just return fake stuff, for some reason cdtv-cr driver requires something
			// that looks valid, even when not playing or it gets stuck in infinite loop
			uae_u8 dst[SUB_CHANNEL_SIZE];
			// regenerate Q-subchannel
			uae_u8 *s = dst + 12;
			struct cd_toc *cdtoc = &toc.toc[toc.first_track];
			int sector = 150;
			memset (dst, 0, SUB_CHANNEL_SIZE);
			s[0] = (cdtoc->control << 4) | (cdtoc->adr << 0);
			s[1] = tobcd (cdtoc->track);
			s[2] = tobcd (1);
			int msf = lsn2msf (sector);
			tolongbcd (s + 7, msf);
			msf = lsn2msf (sector - cdtoc->address - 150);
			tolongbcd (s + 3, msf);
			setsubchannel(dst);
		}
	}

	if (cdtvcr_wait_sectors)
		cd_led |= LED_CD_ACTIVE;
	else
		cd_led &= ~LED_CD_ACTIVE;
	if ((cd_led & ~LED_CD_ACTIVE2) && !cdtvcr_4510_ram[CDTVCR_CD_PLAYING])
		gui_flicker_led(LED_CD, 0, cd_led);

	rethink_cdtvcr();
}

static void open_unit (void)
{
	struct device_info di;
	unitnum = get_standard_cd_unit (CD_STANDARD_UNIT_CDTV);
	sys_command_info (unitnum, &di, 0);
	write_log (_T("using drive %s (unit %d, media %d)\n"), di.label, unitnum, di.media_inserted);
}

static void close_unit (void)
{
	if (unitnum >= 0)
		sys_command_close (unitnum);
	unitnum = -1;
}

static void cdtvcr_reset(int hardreset)
{
	if (!currprefs.cs_cdtvcr)
		return;
	close_unit ();
	if (!thread_alive) {
		init_comm_pipe (&requests, 100, 1);
		uae_start_thread (_T("cdtv-cr"), dev_thread, NULL, NULL);
		while (!thread_alive)
			sleep_millis (10);
		uae_sem_init (&sub_sem, 0, 1);
	}
	open_unit ();
	gui_flicker_led (LED_CD, 0, -1);

	cdtvcr_4510_reset(0);
	cdtvcr_battram_reset();
	cdtvcr_clock[0] = 0xe3;
	cdtvcr_clock[1] = 0x1b;

#if CDTVCR_4510_EMULATION
	init_65c02();
#endif
}

static void cdtvcr_free(void)
{
	if (thread_alive > 0) {
		write_comm_pipe_u32 (&requests, 0xffff, 1);
		while (thread_alive > 0)
			sleep_millis (10);
		uae_sem_destroy (&sub_sem);
	}
	thread_alive = 0;
	close_unit ();
}

bool cdtvcr_init(struct autoconfig_info *aci)
{
	aci->start = 0xb80000;
	aci->size = 0x10000;
	aci->zorro = 0;
	device_add_reset(cdtvcr_reset);
	if (!aci->doinit)
		return true;
	map_banks(&cdtvcr_bank, 0xB8, 1, 0);

	device_add_hsync(CDTVCR_hsync_handler);
	device_add_rethink(rethink_cdtvcr);
	device_add_exit(cdtvcr_free, NULL);

	return true;
}

#if CDTVCR_4510_EMULATION

// VICE 65C02 emulator, waiting for future full CDTV-CR 4510 emulation.

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int CLOCK;

static void cpu65c02_reset(void)
{
}

#define CLOCK_MAX (~((CLOCK)0))
#define TRAP_OPCODE 0x02
#define STATIC_ASSERT(_x)

#define NUM_MEMSPACES e_invalid_space

enum mon_int {
	MI_NONE = 0,
	MI_BREAK = 1 << 0,
	MI_WATCH = 1 << 1,
	MI_STEP = 1 << 2
};

enum t_memspace {
	e_default_space = 0,
	e_comp_space,
	e_disk8_space,
	e_disk9_space,
	e_disk10_space,
	e_disk11_space,
	e_invalid_space
};
typedef enum t_memspace MEMSPACE;
static unsigned monitor_mask[NUM_MEMSPACES];
static void monitor_check_icount_interrupt(void)
{
}
static void monitor_check_icount(WORD a)
{
}
static int monitor_force_import(int mem)
{
	return 0;
}
static int monitor_check_breakpoints(int mem, WORD addr)
{
	return 0;
}
static void monitor_check_watchpoints(unsigned int lastpc, unsigned int pc)
{
}
static void monitor_startup(int mem)
{
}

#define CYCLE_EXACT_ALARM

#define INTERRUPT_DELAY 2

#define INTRRUPT_MAX_DMA_PER_OPCODE (7+10000)

enum cpu_int {
	IK_NONE = 0,
	IK_NMI = 1 << 0,
	IK_IRQ = 1 << 1,
	IK_RESET = 1 << 2,
	IK_TRAP = 1 << 3,
	IK_MONITOR = 1 << 4,
	IK_DMA = 1 << 5,
	IK_IRQPEND = 1 << 6
};

struct interrupt_cpu_status_s {
	/* Number of interrupt lines.  */
	unsigned int num_ints;

	/* Define, for each interrupt source, whether it has a pending interrupt
	(IK_IRQ, IK_NMI, IK_RESET and IK_TRAP) or not (IK_NONE).  */
	unsigned int *pending_int;

	/* Name for each interrupt source */
	char **int_name;

	/* Number of active IRQ lines.  */
	int nirq;

	/* Tick when the IRQ was triggered.  */
	CLOCK irq_clk;

	/* Number of active NMI lines.  */
	int nnmi;

	/* Tick when the NMI was triggered.  */
	CLOCK nmi_clk;

	/* If an opcode is intercepted by a DMA, save the number of cycles
	left at the start of this particular DMA (needed by *_set_irq() to
	calculate irq_clk).  */
	unsigned int num_dma_per_opcode;
	unsigned int num_cycles_left[INTRRUPT_MAX_DMA_PER_OPCODE];
	CLOCK dma_start_clk[INTRRUPT_MAX_DMA_PER_OPCODE];

	/* counters for delay between interrupt request and handler */
	unsigned int irq_delay_cycles;
	unsigned int nmi_delay_cycles;

	/* If 1, do a RESET.  */
	int reset;

	/* If 1, call the trapping function.  */
	int trap;

	/* Debugging function.  */
	void(*trap_func)(WORD, void *data);

	/* Data to pass to the debugging function when called.  */
	void *trap_data;

	/* Pointer to the last executed opcode information.  */
	unsigned int *last_opcode_info_ptr;

	/* Number of cycles we have stolen to the processor last time.  */
	int num_last_stolen_cycles;

	/* Clock tick at which these cycles have been stolen.  */
	CLOCK last_stolen_cycles_clk;

	/* Clock tick where just ACK'd IRQs may still trigger an interrupt.
	Set to CLOCK_MAX when irrelevant.  */
	CLOCK irq_pending_clk;

	unsigned int global_pending_int;

	void(*nmi_trap_func)(void);

	void(*reset_trap_func)(void);
};
typedef struct interrupt_cpu_status_s interrupt_cpu_status_t;

/* Masks to extract information. */
#define OPINFO_DELAYS_INTERRUPT_MSK     (1 << 8)
#define OPINFO_DISABLES_IRQ_MSK         (1 << 9)
#define OPINFO_ENABLES_IRQ_MSK          (1 << 10)

/* Return nonzero if `opinfo' causes a 1-cycle interrupt delay.  */
#define OPINFO_DELAYS_INTERRUPT(opinfo)         \
    ((opinfo) & OPINFO_DELAYS_INTERRUPT_MSK)

/* Return nonzero if `opinfo' has changed the I flag from 0 to 1, so that an
IRQ that happened 2 or more cycles before the end of the opcode should be
allowed.  */
#define OPINFO_DISABLES_IRQ(opinfo)             \
    ((opinfo) & OPINFO_DISABLES_IRQ_MSK)

/* Return nonzero if `opinfo' has changed the I flag from 1 to 0, so that an
IRQ that happened 2 or more cycles before the end of the opcode should not
be allowed.  */
#define OPINFO_ENABLES_IRQ(opinfo)              \
    ((opinfo) & OPINFO_ENABLES_IRQ_MSK)

/* Set the information for `opinfo'.  `number' is the opcode number,
`delays_interrupt' must be non-zero if it causes a 1-cycle interrupt
delay, `disables_interrupts' must be non-zero if it disabled IRQs.  */
#define OPINFO_SET(opinfo,                                                \
                   number, delays_interrupt, disables_irq, enables_irq)   \
    ((opinfo) = ((number)                                                 \
                 | ((delays_interrupt) ? OPINFO_DELAYS_INTERRUPT_MSK : 0) \
                 | ((disables_irq) ? OPINFO_DISABLES_IRQ_MSK : 0)         \
                 | ((enables_irq) ? OPINFO_ENABLES_IRQ_MSK : 0)))

/* Set whether the opcode causes the 1-cycle interrupt delay according to
`delay'.  */
#define OPINFO_SET_DELAYS_INTERRUPT(opinfo, delay)   \
    do {                                             \
        if ((delay))                                 \
            (opinfo) |= OPINFO_DELAYS_INTERRUPT_MSK; \
    } while (0)

/* Set whether the opcode disables previously enabled IRQs according to
`disable'.  */
#define OPINFO_SET_DISABLES_IRQ(opinfo, disable) \
    do {                                         \
        if ((disable))                           \
            (opinfo) |= OPINFO_DISABLES_IRQ_MSK; \
    } while (0)

/* Set whether the opcode enables previously disabled IRQs according to
`enable'.  */
#define OPINFO_SET_ENABLES_IRQ(opinfo, enable)  \
    do {                                        \
        if ((enable))                           \
            (opinfo) |= OPINFO_ENABLES_IRQ_MSK; \
    } while (0)



typedef struct mos6510_regs_s {
	unsigned int pc;        /* `unsigned int' required by the drive code. */
	BYTE a;
	BYTE x;
	BYTE y;
	BYTE sp;
	BYTE p;
	BYTE n;
	BYTE z;
} mos6510_regs_t;

typedef struct R65C02_regs_s
{
	unsigned int pc;
	BYTE a;
	BYTE x;
	BYTE y;
	BYTE sp;
	BYTE p;
	BYTE n;
	BYTE z;
} R65C02_regs_t;

#define DRIVE_RAM_SIZE 65536

typedef BYTE drive_read_func_t(struct drive_context_s *, WORD);
typedef void drive_store_func_t(struct drive_context_s *, WORD,
	BYTE);

typedef struct drivecpud_context_s {
	/* Drive RAM */
	BYTE drive_ram[DRIVE_RAM_SIZE];

	/* functions */
	drive_read_func_t  *read_func[0x101];
	drive_store_func_t *store_func[0x101];
//	drive_read_func_t  *read_func_watch[0x101];
//	drive_store_func_t *store_func_watch[0x101];
//	drive_read_func_t  *read_func_nowatch[0x101];
//	drive_store_func_t *store_func_nowatch[0x101];

	int sync_factor;
} drivecpud_context_t;

typedef struct drive_context_s {
	int mynumber;         /* init to [0123] */
	CLOCK *clk_ptr;       /* shortcut to drive_clk[mynumber] */

	struct drivecpu_context_s *cpu;
	struct drivecpud_context_s *cpud;
	struct drivefunc_context_s *func;
} drive_context_t;

static int drive_trap_handler(drive_context_t *d)
{
	return -1;
}

typedef struct drivecpu_context_s
{
	int traceflg;
	/* This is non-zero each time a Read-Modify-Write instructions that accesses
	memory is executed.  We can emulate the RMW bug of the 6502 this way.  */
	int rmw_flag; /* init to 0 */

				  /* Interrupt/alarm status.  */
	struct interrupt_cpu_status_s *int_status;

	struct alarm_context_s *alarm_context;

	/* Clk guard.  */
	struct clk_guard_s *clk_guard;

	struct monitor_interface_s *monitor_interface;

	/* Value of clk for the last time mydrive_cpu_execute() was called.  */
	CLOCK last_clk;

	/* Number of cycles in excess we executed last time mydrive_cpu_execute()
	was called.  */
	CLOCK last_exc_cycles;

	CLOCK stop_clk;

	CLOCK cycle_accum;
	BYTE *d_bank_base;
	unsigned int d_bank_start;
	unsigned int d_bank_limit;

	/* Information about the last executed opcode.  */
	unsigned int last_opcode_info;

	/* Address of the last executed opcode. This is used by watchpoints. */
	unsigned int last_opcode_addr;

	/* Public copy of the registers.  */
	mos6510_regs_t cpu_regs;
//	R65C02_regs_t cpu_R65C02_regs;

	BYTE *pageone;        /* init to NULL */

	int monspace;         /* init to e_disk[89]_space */

	char *snap_module_name;

	char *identification_string;
} drivecpu_context_t;

#define LOAD(a)           (drv->cpud->read_func[(a) >> 8](drv, (WORD)(a)))
#define LOAD_ZERO(a)      (drv->cpud->read_func[0](drv, (WORD)(a)))
#define LOAD_ADDR(a)      (LOAD(a) | (LOAD((a) + 1) << 8))
#define LOAD_ZERO_ADDR(a) (LOAD_ZERO(a) | (LOAD_ZERO((a) + 1) << 8))
#define STORE(a, b)       (drv->cpud->store_func[(a) >> 8](drv, (WORD)(a), \
                          (BYTE)(b)))
#define STORE_ZERO(a, b)  (drv->cpud->store_func[0](drv, (WORD)(a), \
                          (BYTE)(b)))

#define JUMP(addr) reg_pc = (unsigned int)(addr); 

#define P_SIGN          0x80
#define P_OVERFLOW      0x40
#define P_UNUSED        0x20
#define P_BREAK         0x10
#define P_DECIMAL       0x08
#define P_INTERRUPT     0x04
#define P_ZERO          0x02
#define P_CARRY         0x01

#define CPU_WDC65C02   0
#define CPU_R65C02     1
#define CPU_65SC02     2

#define CLK (*(drv->clk_ptr))
#define RMW_FLAG (cpu->rmw_flag)
#define PAGE_ONE (cpu->pageone)
#define LAST_OPCODE_INFO (cpu->last_opcode_info)
#define LAST_OPCODE_ADDR (cpu->last_opcode_addr)
#define TRACEFLG (debug.drivecpu_traceflg[drv->mynumber])

#define CPU_INT_STATUS (cpu->int_status)

#define ALARM_CONTEXT (cpu->alarm_context)

#define ROM_TRAP_ALLOWED() 1

#define ROM_TRAP_HANDLER() drive_trap_handler(drv)

#define CALLER (cpu->monspace)

#define DMA_FUNC drive_generic_dma()

#define DMA_ON_RESET

#define cpu_reset() (cpu_reset)(drv)
#define bank_limit (cpu->d_bank_limit)
#define bank_start (cpu->d_bank_start)
#define bank_base (cpu->d_bank_base)

/* WDC_STP() and WDC_WAI() are not used on the R65C02. */
#define WDC_STP()
#define WDC_WAI()

#define GLOBAL_REGS cpu->cpu_R65C02_regs

#define DRIVE_CPU

static void drive_generic_dma(void)
{
}
static void interrupt_trigger_dma(interrupt_cpu_status_t *cs, CLOCK cpu_clk)
{
	cs->global_pending_int = (enum cpu_int)
		(cs->global_pending_int | IK_DMA);
}
static void interrupt_ack_dma(interrupt_cpu_status_t *cs)
{
	cs->global_pending_int = (enum cpu_int)
		(cs->global_pending_int & ~IK_DMA);
}
static void interrupt_do_trap(interrupt_cpu_status_t *cs, WORD address)
{
	cs->global_pending_int &= ~IK_TRAP;
	cs->trap_func(address, cs->trap_data);
}
static void interrupt_ack_reset(interrupt_cpu_status_t *cs)
{
	cs->global_pending_int &= ~IK_RESET;

	if (cs->reset_trap_func)
		cs->reset_trap_func();
}
static void interrupt_ack_nmi(interrupt_cpu_status_t *cs)
{
	cs->global_pending_int =
		(cs->global_pending_int & ~IK_NMI);

	if (cs->nmi_trap_func) {
		cs->nmi_trap_func();
	}
}
static void interrupt_ack_irq(interrupt_cpu_status_t *cs)
{
	cs->global_pending_int =
		(cs->global_pending_int & ~IK_IRQPEND);
	cs->irq_pending_clk = CLOCK_MAX;
}
static int interrupt_check_nmi_delay(interrupt_cpu_status_t *cs,
	CLOCK cpu_clk)
{
	CLOCK nmi_clk = cs->nmi_clk + INTERRUPT_DELAY;

	/* Branch instructions delay IRQs and NMI by one cycle if branch
	is taken with no page boundary crossing.  */
	if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr))
		nmi_clk++;

	if (cpu_clk >= nmi_clk)
		return 1;

	return 0;
}
static int interrupt_check_irq_delay(interrupt_cpu_status_t *cs,
	CLOCK cpu_clk)
{
	CLOCK irq_clk = cs->irq_clk + INTERRUPT_DELAY;

	/* Branch instructions delay IRQs and NMI by one cycle if branch
	is taken with no page boundary crossing.  */
	if (OPINFO_DELAYS_INTERRUPT(*cs->last_opcode_info_ptr))
		irq_clk++;

	/* If an opcode changes the I flag from 1 to 0, the 6510 needs
	one more opcode before it triggers the IRQ routine.  */
	if (cpu_clk >= irq_clk) {
		if (!OPINFO_ENABLES_IRQ(*cs->last_opcode_info_ptr)) {
			return 1;
		} else {
			cs->global_pending_int |= IK_IRQPEND;
		}
	}
	return 0;
}

struct alarm_context_s {
	CLOCK next_pending_alarm_clk;
};
typedef struct alarm_context_s alarm_context_t;

static void alarm_context_dispatch(alarm_context_t *context, CLOCK cpu_clk)
{
}
static CLOCK alarm_context_next_pending_clk(alarm_context_t *context)
{
	return context->next_pending_alarm_clk;
}

static struct drive_context_s m_4510_context;
static struct drivecpu_context_s m_4510_cpu_context;
static struct drivecpud_context_s m_4510_cpud_context;
static CLOCK m_4510_clk;
static struct alarm_context_s m_4510_alarm_context;
static struct interrupt_cpu_status_s m_4510_interrupt;

static void cpu_4510(void)
{
	int cpu_type = CPU_R65C02;
	drivecpu_context_t *cpu = &m_4510_cpu_context;
	drive_context_t *drv = &m_4510_context;

#define reg_a   (cpu->cpu_regs.a)
#define reg_x   (cpu->cpu_regs.x)
#define reg_y   (cpu->cpu_regs.y)
#define reg_pc  (cpu->cpu_regs.pc)
#define reg_sp  (cpu->cpu_regs.sp)
#define reg_p   (cpu->cpu_regs.p)
#define flag_z  (cpu->cpu_regs.z)
#define flag_n  (cpu->cpu_regs.n)

#include "65c02core.cpp"
}

BYTE read_4510(struct drive_context_s *c, WORD offset)
{
	if (offset >= 32768)
		return c->cpud->drive_ram[offset];
	return 0;
}
void write_4510(struct drive_context_s *c, WORD offset, BYTE v)
{
	if (offset >= 32768)
		return;
	c->cpud->drive_ram[offset] = v;
}

static void init_65c02(void)
{
	drive_context_s *d = &m_4510_context;
	drivecpu_context_s *cpu = &m_4510_cpu_context;
	drivecpud_context_s *cpud = &m_4510_cpud_context;

	d->cpu = cpu;
	d->cpud = cpud;
	d->clk_ptr = &m_4510_clk;

	cpu->rmw_flag = 0;
	cpu->d_bank_limit = 0;
	cpu->d_bank_start = 0;
	cpu->pageone = NULL;
	cpu->alarm_context = &m_4510_alarm_context;
	cpu->int_status = &m_4510_interrupt;

	cpu->int_status->global_pending_int = IK_RESET;

	for(int i = 0; i < 256; i++) {
		cpud->read_func[i] = read_4510;
		cpud->store_func[i] = write_4510;
	}

	struct zfile *zf = read_rom_name(_T("d:\\amiga\\roms\\cdtv-cr-4510.bin"));
	if (zf) {
		zfile_fread(cpud->drive_ram + 32768, 1, 32768, zf);
		zfile_fclose(zf);
	}
}

#endif
