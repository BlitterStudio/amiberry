/*
* UAE - The Un*x Amiga Emulator
*
* CDTV DMAC/CDROM controller emulation
*
* Copyright 2004/2007-2010 Toni Wilen
*
* Thanks to Mark Knibbs <markk@clara.co.uk> for CDTV Technical information
*
*/

//#define CDTV_SUB_DEBUG
//#define CDTV_DEBUG
//#define CDTV_DEBUG_CMD
//#define CDTV_DEBUG_6525

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef CDTV

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "debug.h"
#include "cdtv.h"
#include "blkdev.h"
#include "gui.h"
#include "zfile.h"
#include "threaddep/thread.h"
#ifdef A2091
#include "a2091.h"
#endif
#include "uae.h"
#include "savestate.h"
#include "devices.h"
#include "rommgr.h"

/* DMAC CNTR bits. */
#define CNTR_TCEN               (1<<7)
#define CNTR_PREST              (1<<6)
#define CNTR_PDMD               (1<<5)
#define CNTR_INTEN              (1<<4)
#define CNTR_DDIR               (1<<3)
/* ISTR bits. */
#define ISTR_INTX               (1<<8)
#define ISTR_INT_F              (1<<7)
#define ISTR_INTS               (1<<6)
#define ISTR_E_INT              (1<<5)
#define ISTR_INT_P              (1<<4)
#define ISTR_UE_INT             (1<<3)
#define ISTR_OE_INT             (1<<2)
#define ISTR_FF_FLG             (1<<1)
#define ISTR_FE_FLG             (1<<0)

#define MODEL_NAME "MATSHITA0.96"
/* also MATSHITA0.97 exists but is apparently rare */

#define MAX_SUBCODEBUFFER 36
static volatile int subcodebufferoffset, subcodebufferoffsetw, subcodeoffset;
static uae_u8 subcodebufferinuse[MAX_SUBCODEBUFFER];
static uae_u8 subcodebuffer[MAX_SUBCODEBUFFER * SUB_CHANNEL_SIZE];
static uae_sem_t sub_sem, cda_sem;

static smp_comm_pipe requests;
static volatile int thread_alive;

static int configured;
static int cdtvscsi;
static uae_u8 dmacmemory[128];

static struct cd_toc_head toc;
static uae_u32 last_cd_position, play_start, play_end;
static uae_u8 cdrom_qcode[4 + 12], cd_audio_status;
static int datatrack;

static volatile uae_u8 dmac_istr, dmac_cntr;
static volatile uae_u16 dmac_dawr;
static volatile uae_u32 dmac_acr;
static volatile int dmac_wtc;
static volatile int dmac_dma;
static uae_u32 dma_mask;

static volatile int activate_stch, cdrom_command_done;
static volatile int cdrom_sector, cdrom_sectors, cdrom_length, cdrom_offset;
static volatile int cd_playing, cd_paused, cd_motor, cd_media, cd_error, cd_finished, cd_isready, cd_audio_finished;
static uae_u32 last_play_pos, last_play_end;

static volatile int cdtv_hsync, dma_finished, cdtv_sectorsize;
static volatile uae_u64 dma_wait;
static int cd_volume, cd_volume_stored;
static uae_u16 dac_control_data_format;
static int cd_led;
static int frontpanel;

static uae_u8 cdrom_command_input[16];
static int cdrom_command_cnt_in;

static uae_u8 tp_a, tp_b, tp_c, tp_ad, tp_bd, tp_cd;
static uae_u8 tp_imask, tp_cr, tp_air, tp_ilatch, tp_ilatch2;
static int volstrobe1, volstrobe2;

static void do_stch (void);

static void INT2 (void)
{
	safe_interrupt_set(IRQ_SOURCE_CD32CDTV, 0, false);
	cd_led ^= LED_CD_ACTIVE2;
}

static volatile int cdrom_command_cnt_out, cdrom_command_size_out;
static uae_u8 cdrom_command_output[16];

// scor = subchannel frame
// sbcp = subchannel data byte
static volatile int stch, sten, scor, sbcp;
static volatile int cmd, enable, xaen, dten;

static int unitnum = -1;

static void subreset (void)
{
	uae_sem_wait (&sub_sem);
	memset (subcodebufferinuse, 0, sizeof subcodebufferinuse);
	subcodebufferoffsetw = 0;
	subcodebufferoffset = 0;
	subcodeoffset = -1;
	sbcp = 0;
	scor = 0;
	uae_sem_post (&sub_sem);
}

static int get_toc (void)
{
	datatrack = 0;
	if (!sys_command_cd_toc (unitnum, &toc))
		return 0;
	last_cd_position = toc.lastaddress;
	if (toc.first_track == 1 && (toc.toc[toc.first_track_offset].control & 0x0c) == 0x04)
		datatrack = 1;
	return 1;
}

static int get_qcode (void)
{
	if (!sys_command_cd_qcode (unitnum, cdrom_qcode, -1, false))
		return 0;
	cdrom_qcode[1] = cd_audio_status;
	return 1;
}

static void cdaudiostop_do (void)
{
	if (unitnum < 0)
		return;
	sys_command_cd_pause (unitnum, 0);
	sys_command_cd_stop (unitnum);
}

static void cdaudiostop (void)
{
	cd_finished = 0;
	cd_audio_status = AUDIO_STATUS_NO_STATUS;
	if (cd_playing) {
		cd_audio_status = AUDIO_STATUS_PLAY_COMPLETE;
		cd_finished = 1;
	}
	cd_playing = 0;
	cd_paused = 0;
	cd_motor = 0;
	cd_audio_finished = 0;
	write_comm_pipe_u32 (&requests, 0x0104, 1);
}

static void cdaudiostopfp (void)
{
	cdaudiostop_do ();
	cd_audio_status = AUDIO_STATUS_NO_STATUS;
	activate_stch = 1;
	cd_finished = 0;
	cd_playing = 0;
	cd_paused = 0;
	cd_motor = 0;
}

static int pause_audio (int pause)
{
	sys_command_cd_pause (unitnum, pause);
	if (!cd_playing) {
		cd_paused = 0;
		cd_audio_status = AUDIO_STATUS_NO_STATUS;
		return 0;
	}
	cd_paused = pause;
	cd_audio_status = pause ? AUDIO_STATUS_PAUSED : AUDIO_STATUS_IN_PROGRESS;
	subreset ();
	return 1;
}

static int read_sectors (int start, int length)
{
	if (cd_playing)
		cdaudiostop ();
#ifdef CDTV_DEBUG_CMD
	write_log (_T("READ DATA sector %d, %d sectors (blocksize=%d)\n"), start, length, cdtv_sectorsize);
#endif
	cdrom_sector = start;
	cdrom_sectors = length;
	cdrom_offset = start * cdtv_sectorsize;
	cdrom_length = length * cdtv_sectorsize;
	cd_motor = 1;
	cd_audio_status = AUDIO_STATUS_NOT_SUPPORTED;
	return 0;
}

static int ismedia (void)
{
	if (unitnum < 0)
		return 0;
	return sys_command_ismedia (unitnum, 0);
}

static int issub (void)
{
	return 1;
}

static void subfunc (uae_u8 *data, int cnt)
{
	if (!issub ())
		return;
	uae_sem_wait (&sub_sem);
#ifdef CDTV_SUB_DEBUG
	int total = 0;
	for (int i = 0; i < MAX_SUBCODEBUFFER; i++) {
		if (subcodebufferinuse[i])
			total++;
	}
	write_log (_T("%d "), total);
#endif
	if (subcodebufferinuse[subcodebufferoffsetw]) {
		memset (subcodebufferinuse, 0, sizeof subcodebufferinuse);
		subcodebufferoffsetw = 0;
		subcodebufferoffset = 0;
		subcodeoffset = -1;
		uae_sem_post (&sub_sem);
#ifdef CDTV_SUB_DEBUG
		write_log (_T("CDTV: subcode buffer overflow 1\n"));
#endif
		return;
	}
	int offset = subcodebufferoffsetw;
	while (cnt > 0) {
		if (subcodebufferinuse[offset]) {
#ifdef CDTV_SUB_DEBUG
			write_log (_T("CDTV: subcode buffer overflow 2\n"));
#endif
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
	uae_sem_post (&sub_sem);
}
static int statusfunc (int status, int playpos)
{
	if (status == -1)
		return 150;
	if (status == -2)
		return 20;
	if (status < 0)
		return 0;
	if (cd_audio_status != status) {
		if (status == AUDIO_STATUS_PLAY_COMPLETE || status == AUDIO_STATUS_PLAY_ERROR) {
			cd_audio_finished = 1;
		} else {
			if (status == AUDIO_STATUS_IN_PROGRESS)
				cd_playing = 1;
			activate_stch = 1;
		}
	}
	if (status == AUDIO_STATUS_IN_PROGRESS)
		last_play_pos = playpos;
	cd_audio_status = status;
	return 0;
}

static int statusfunc_imm(int status, int playpos)
{
	if (status == -3 || status > AUDIO_STATUS_IN_PROGRESS)
		uae_sem_post(&cda_sem);
	if (status < 0)
		return 0;
	if (status == AUDIO_STATUS_IN_PROGRESS)
		cd_audio_status = status;
	return statusfunc(status, playpos);
}

static void do_play(bool immediate)
{
	uae_u32 start = read_comm_pipe_u32_blocking (&requests);
	uae_u32 end = read_comm_pipe_u32_blocking (&requests);
	uae_u32 scan = read_comm_pipe_u32_blocking (&requests);
	subreset ();
	sys_command_cd_pause (unitnum, 0);
	sys_command_cd_volume (unitnum, (cd_volume_stored << 5) | (cd_volume_stored >> 5), (cd_volume_stored << 5) | (cd_volume_stored >> 5));
	sys_command_cd_play (unitnum, start, end, 0, immediate ? statusfunc_imm : statusfunc, subfunc);
}

static void startplay (void)
{
	subreset ();
	write_comm_pipe_u32 (&requests, 0x0110, 0);
	write_comm_pipe_u32 (&requests, play_start, 0);
	write_comm_pipe_u32 (&requests, play_end, 0);
	write_comm_pipe_u32 (&requests, 0, 1);
	if (!cd_motor) {
		cd_motor = 1;
		activate_stch = 1;
	}
}

static int play_cdtrack (uae_u8 *p)
{
	int track_start = p[1];
	int index_start = p[2];
	int track_end = p[3];
	int index_end = p[4];
	int start_found, end_found;
	uae_u32 start, end;
	int j;

	if (track_start == 0 && track_end == 0)
		return 0;

	end = last_cd_position;
	start_found = end_found = 0;
	for (j = toc.first_track_offset; j <= toc.last_track_offset; j++) {
		struct cd_toc *s = &toc.toc[j];
		if (track_start == s->track) {
			start_found++;
			start = s->paddress;
		}
		if (track_end == s->track) {
			end = s->paddress;
			end_found++;
		}
	}
	if (start_found == 0) {
		cdaudiostop ();
		cd_error = 1;
		activate_stch = 1;
		write_log (_T("PLAY CD AUDIO: illegal start track %d\n"), track_start);
		return 0;
	}
	play_end = end;
	play_start = start;
	last_play_pos = start;
	last_play_end = end;
#ifdef CDTV_DEBUG_CMD
	write_log (_T("PLAY CD AUDIO from %d-%d, %06X (%d) to %06X (%d)\n"),
		track_start, track_end, start, start, end, end);
#endif
	startplay ();
	return 0;
}


static int play_cd (uae_u8 *p)
{
	uae_u32 start, end;

	start = (p[1] << 16) | (p[2] << 8) | p[3];
	end = (p[4] << 16) | (p[5] << 8) | p[6];
	if (p[0] == 0x09) /* end is length in lsn-mode */
		end += start;
	if (start == 0 && end == 0) {
		cd_finished = 0;
		if (cd_playing)
			cd_finished = 1;
		cd_playing = 0;
		cd_paused = 0;
		cd_motor = 0;
		write_comm_pipe_u32 (&requests, 0x0104, 1);
		cd_audio_status = AUDIO_STATUS_NO_STATUS;
		cd_error = 1;
		activate_stch = 1;
		return 0;
	}
	if (p[0] != 0x09) { /* msf */
		start = msf2lsn (start);
		if (end < 0x00ffffff)
			end = msf2lsn (end);
	}
	if (end >= 0x00ffffff || end > last_cd_position)
		end = last_cd_position;
	play_end = end;
	play_start = start;
	last_play_pos = start;
	last_play_end = end;
#ifdef CDTV_DEBUG_CMD
	write_log (_T("PLAY CD AUDIO from %06X (%d) to %06X (%d)\n"),
		lsn2msf (start), start, lsn2msf (end), end);
#endif
	startplay ();
	return 0;
}

static int cdrom_subq (uae_u8 *out, int msflsn)
{
	uae_u8 *s = cdrom_qcode;
	uae_u32 trackposlsn, trackposmsf;
	uae_u32 diskposlsn, diskposmsf;

	out[0] = cd_audio_status;
	s += 4;
	out[1] = (s[0] >> 4) | (s[0] << 4);
	out[2] = frombcd (s[1]); // track
	out[3] = frombcd (s[2]); // index
	trackposmsf = fromlongbcd (s + 3);
	diskposmsf = fromlongbcd (s + 7);
	trackposlsn = msf2lsn (trackposmsf);
	diskposlsn = msf2lsn (diskposmsf);
	out[4] = 0;
	out[5] = (msflsn ? diskposmsf : diskposlsn) >> 16;
	out[6] = (msflsn ? diskposmsf : diskposlsn) >> 8;
	out[7] = (msflsn ? diskposmsf : diskposlsn) >> 0;
	out[8] = 0;
	out[9] = (msflsn ? trackposmsf : trackposlsn) >> 16;
	out[10] = (msflsn ? trackposmsf : trackposlsn) >> 8;
	out[11] = (msflsn ? trackposmsf : trackposlsn) >> 0;
	out[12] = 0;
	if (cd_audio_status == AUDIO_STATUS_IN_PROGRESS)
		last_play_pos = diskposlsn;
	return 13;
}

static int cdrom_info (uae_u8 *out)
{
	uae_u32 size;

	if (ismedia () <= 0)
		return -1;
	cd_motor = 1;
	out[0] = toc.first_track;
	out[1] = toc.last_track;
	size = lsn2msf (toc.lastaddress);
	out[2] = size >> 16;
	out[3] = size >> 8;
	out[4] = size >> 0;
	cd_finished = 1;
	return 5;
}

static int read_toc (int track, int msflsn, uae_u8 *out)
{
	int j;

	if (ismedia () <= 0)
		return -1;
	if (!out)
		return 0;
	cd_motor = 1;
	for (j = 0; j < toc.points; j++) {
		if (track == toc.toc[j].point) {
			int lsn = toc.toc[j].paddress;
			int msf = lsn2msf (lsn);
			out[0] = 0;
			out[1] = (toc.toc[j].adr << 4) | (toc.toc[j].control << 0);
			out[2] = toc.toc[j].point;
			out[3] = toc.tracks;
			out[4] = 0;
			out[5] = (msflsn ? msf : lsn) >> 16;
			out[6] = (msflsn ? msf : lsn) >> 8;
			out[7] = (msflsn ? msf : lsn) >> 0;
			cd_finished = 1;
			return 8;
		}
	}
	return -1;
}

static int cdrom_modeset (uae_u8 *cmd)
{
	int sectorsize = (cmd[2] << 8) | cmd[3];
	if (sectorsize != 512 && sectorsize != 1024 && sectorsize != 2048 && sectorsize != 2052 && sectorsize != 2336 && sectorsize != 2340) {
		write_log (_T("CDTV: tried to set unknown sector size %d\n"), cdtv_sectorsize);
		cd_error = 1;
		return 0;
	}
	cdtv_sectorsize = sectorsize;
	return 0;
}

static void cdrom_command_accepted (int size, uae_u8 *cdrom_command_input, int *cdrom_command_cnt_in)
{
#ifdef CDTV_DEBUG_CMD
	TCHAR tmp[200];
	int i;
#endif
	cdrom_command_size_out = size;
#ifdef CDTV_DEBUG_CMD
	tmp[0] = 0;
	for (i = 0; i < *cdrom_command_cnt_in; i++)
		_stprintf (tmp + i * 3, _T("%02X%c"), cdrom_command_input[i], i < *cdrom_command_cnt_in - 1 ? '.' : ' ');
	write_log (_T("CD<-: %s\n"), tmp);
	if (size > 0) {
		tmp[0] = 0;
		for (i = 0; i < size; i++)
			_stprintf (tmp + i * 3, _T("%02X%c"), cdrom_command_output[i], i < size - 1 ? '.' : ' ');
		write_log (_T("CD->: %s\n"), tmp);
	}
#endif
	*cdrom_command_cnt_in = 0;
	cdrom_command_cnt_out = 0;
	cdrom_command_done = 1;
}

static void cdrom_command_thread (uae_u8 b)
{
	uae_u8 *s;

	cdrom_command_input[cdrom_command_cnt_in] = b;
	cdrom_command_cnt_in++;
	s = cdrom_command_input;

	switch (cdrom_command_input[0])
	{
	case 0x00:
	case 0x80:
		if (cdrom_command_cnt_in == 2) {
			cdrom_command_output[0] = 0xaa;
			cdrom_command_output[1] = 0x55;
			cdrom_command_accepted(2, s, &cdrom_command_cnt_in);
		}
		break;
	case 0x01: /* seek */
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
			cd_finished = 1;
			if (currprefs.cd_speed)
				sleep_millis (500);
			activate_stch = 1;
		}
		break;
	case 0x02: /* read */
		if (cdrom_command_cnt_in == 7) {
			read_sectors ((s[1] << 16) | (s[2] << 8) | (s[3] << 0), (s[4] << 8) | (s[5] << 0));
			cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
		}
		break;
	case 0x04: /* motor on */
		if (cdrom_command_cnt_in == 7) {
			cd_motor = 1;
			cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	case 0x05: /* motor off */
		if (cdrom_command_cnt_in == 7) {
			cd_motor = 0;
			cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	case 0x09: /* play (lsn) */
	case 0x0a: /* play (msf) */
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (play_cd (cdrom_command_input), s, &cdrom_command_cnt_in);
		}
		break;
	case 0x0b:
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (play_cdtrack (cdrom_command_input), s, &cdrom_command_cnt_in);
		}
		break;
	case 0x81:
		if (cdrom_command_cnt_in == 1) {
			uae_u8 flag = 0;
			if (!cd_isready)
				flag |= 1 << 0; // 01
			if (cd_playing)
				flag |= 1 << 2; // 04
			if (cd_finished)
				flag |= 1 << 3; // 08
			if (cd_error)
				flag |= 1 << 4; // 10
			if (cd_motor)
				flag |= 1 << 5; // 20
			if (cd_media)
				flag |= 1 << 6; // 40
			cdrom_command_output[0] = flag;
			cdrom_command_accepted (1, s, &cdrom_command_cnt_in);
			cd_finished = 0;
		}
		break;
	case 0x82:
		if (cdrom_command_cnt_in == 7) {
			if (cd_error)
				cdrom_command_output[2] |= 1 << 4;
			cd_error = 0;
			cd_isready = 0;
			cdrom_command_accepted (6, s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	case 0x83:
		if (cdrom_command_cnt_in == 7) {
			memcpy (cdrom_command_output, MODEL_NAME, uaestrlen(MODEL_NAME)); 
			cdrom_command_accepted (uaestrlen(MODEL_NAME), s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	case 0x84: /* mode set */
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (cdrom_modeset (cdrom_command_input), s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	case 0x85: /* mode sense */
		if (cdrom_command_cnt_in == 1) {
			cdrom_command_output[0] = cdtv_sectorsize >> 8;
			cdrom_command_output[1] = cdtv_sectorsize >> 0;
			cdrom_command_accepted(2, s, &cdrom_command_cnt_in);
		}
		break;
	case 0x86: /* capacity */
		if (cdrom_command_cnt_in == 1) {
			int size = toc.lastaddress - 1;
			cdrom_command_output[0] = size >> 16;
			cdrom_command_output[1] = size >> 8;
			cdrom_command_output[2] = size >> 0;
			cdrom_command_output[3] = cdtv_sectorsize >> 8;
			cdrom_command_output[4] = cdtv_sectorsize >> 0;
			if (ismedia() <= 0) {
				cd_error = 1;
			}
			cdrom_command_accepted(ismedia() <= 0 ? -1 : 5, s, &cdrom_command_cnt_in);
		}
		break;
	case 0x87: /* subq */
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (cdrom_subq (cdrom_command_output, cdrom_command_input[1] & 2), s, &cdrom_command_cnt_in);
		}
		break;
	case 0x88:
		if (cdrom_command_cnt_in == 1) {
			memset(cdrom_command_output, 0, 14);
			cdrom_command_accepted(14, s, &cdrom_command_cnt_in);
		}
		break;
	case 0x89:
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (cdrom_info (cdrom_command_output), s, &cdrom_command_cnt_in);
		}
		break;
	case 0x8a: /* read toc */
		if (cdrom_command_cnt_in == 7) {
			cdrom_command_accepted (read_toc (cdrom_command_input[2], cdrom_command_input[1] & 2, cdrom_command_output), s, &cdrom_command_cnt_in);
		}
		break;
	case 0x8b:
		if (cdrom_command_cnt_in == 7) {
			pause_audio (s[1] == 0x00 ? 1 : 0);
			cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	case 0xa2:
		if (cdrom_command_cnt_in == 1) {
			cdrom_command_output[0] = 0;
			cdrom_command_output[1] = 0;
			cdrom_command_output[2] = 0;
			cdrom_command_output[3] = 0;
			cdrom_command_accepted(4, s, &cdrom_command_cnt_in);
		}
		break;
	case 0xa3: /* front panel */
		if (cdrom_command_cnt_in == 7) {
			frontpanel = s[1] ? 1 : 0;
			cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
			cd_finished = 1;
		}
		break;
	default:
		write_log (_T("unknown CDROM command %02X!\n"), s[0]);
		cd_error = 1;
		cdrom_command_accepted (0, s, &cdrom_command_cnt_in);
		break;
	}
}

static void dma_do_thread (void)
{
	static int readsector;
	int didread = 0;
	int cnt;

	while (dma_finished)
		sleep_millis (2);

	if (!cdtv_sectorsize)
		return;
	cnt = dmac_wtc;
#ifdef CDTV_DEBUG_CMD
	write_log (_T("DMAC DMA: sector=%d, addr=%08X, words=%d (of %d)\n"),
		cdrom_offset / cdtv_sectorsize, dmac_acr, cnt, cdrom_length / 2);
#endif
	dma_wait += cnt * (uae_u64)312 * 50 / 75 + 1;
	if (currprefs.cd_speed == 0)
		dma_wait = 1;
	while (cnt > 0 && dmac_dma) {
		uae_u8 buffer[2352];
		if (!didread || readsector != (cdrom_offset / cdtv_sectorsize)) {
			readsector = cdrom_offset / cdtv_sectorsize;
			if (cdtv_sectorsize < 2048) {
				didread = 0;
			} else if (cdtv_sectorsize != 2048) {
				didread = sys_command_cd_rawread(unitnum, buffer, readsector, 1, cdtv_sectorsize);
			} else {
				didread = sys_command_cd_read(unitnum, buffer, readsector, 1);
			}
			if (!didread) {
				cd_error = 1;
				activate_stch = 1;
				write_log(_T("CDTV: CD read error, sectorsize=%d\n"), cdtv_sectorsize);;
				break;
			}

		}
		dma_put_byte(dmac_acr & dma_mask, buffer[(cdrom_offset % cdtv_sectorsize) + 0]);
		dma_put_byte((dmac_acr + 1) & dma_mask, buffer[(cdrom_offset % cdtv_sectorsize) + 1]);
		cnt--;
		dmac_acr += 2;
		cdrom_length -= 2;
		cdrom_offset += 2;
	}
	dmac_wtc = 0;
	dmac_dma = 0;
	dma_finished = 1;
	cd_finished = 1;
}

static int dev_thread (void *p)
{
	write_log (_T("CDTV: CD thread started\n"));
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
			dma_do_thread ();
			break;
		case 0x0101:
			{
				int m = ismedia ();
				if (m < 0) {
					write_log (_T("CDTV: device %d lost\n"), unitnum);
					activate_stch = 1;
					cd_media = 0;
				} else if (m != cd_media) {
					cd_media = m;
					get_toc ();
					activate_stch = 1;
					if (cd_playing)
						cd_error = 1;
				}
				if (cd_media)
					get_qcode ();
			}
			break;
		case 0x0102: // pause
			sys_command_cd_pause (unitnum, 1);
			break;
		case 0x0103: // unpause
			sys_command_cd_pause (unitnum, 0);
			break;
		case 0x0104: // stop
			cdaudiostop_do ();
			break;
		case 0x0105: // pause
			pause_audio (1);
			break;
		case 0x0106: // unpause
			pause_audio (0);
			break;
		case 0x0107: // frontpanel stop
			cdaudiostopfp ();
			break;
		case 0x0110: // do_play!
			do_play(false);
			break;
		case 0x0111: // do_play immediate
			do_play(true);
			break;
		default:
			cdrom_command_thread (b);
			break;
		}

	}
}

static void cdrom_command (uae_u8 b)
{
	write_comm_pipe_u32 (&requests, b, 1);
}

static void init_play (int start, int end)
{
	play_end = end;
	play_start = start;
	last_play_pos = start;
	last_play_end = end;
#ifdef CDTV_DEBUG_CMD
	write_log (_T("PLAY CD AUDIO from %06X (%d) to %06X (%d)\n"),
		lsn2msf (start), start, lsn2msf (end), end);
#endif
	startplay ();
}

bool cdtv_front_panel (int button)
{
	if (!frontpanel || configured <= 0)
		return false;
	if (button < 0)
		return true;
	switch (button)
	{
	case 0: // stop
		if (cd_paused)
			write_comm_pipe_u32 (&requests, 0x0106, 1);
		write_comm_pipe_u32 (&requests, 0x0107, 1);
	break;
	case 1: // playpause
		if (cd_playing)  {
			write_comm_pipe_u32 (&requests, cd_paused ? 0x0106 : 0x0105, 1);
		} else if (cd_media) {
			init_play (0, last_cd_position);
		}
	break;
	case 2: // prev
	case 3: // next
	if (!cd_playing)
		return true;
	uae_u8 *sq = cdrom_qcode + 4;
	int track = frombcd (sq[1]);
	int pos = 0;
	for (int j = 0; j < toc.points; j++) {
		int t = toc.toc[j].track;
		pos = toc.toc[j].paddress;
		if (t == 1 && track == 1 && button == 2)
			break;
		else if (j == toc.points - 1 && t == track && button == 3)
			break;
		else if (t == track - 1 && track > 1 && button == 2)
			break;
		else if (t == track + 1 && track < 99 && button == 3)
			break;
	}
	init_play (pos - 150, last_cd_position);
	break;
	}
	return true;
}

static uae_u8 get_tp_c (void)
{
	uae_u8 v = (sbcp ? 0 : (1 << 0)) | (scor ? 0 : (1 << 1)) |
		(stch ? 0 : (1 << 2)) | (sten ? 0 : (1 << 3) | (1 << 4));
	return v;
}
static uae_u8 get_tp_c_level (void)
{
	uae_u8 v = (sbcp == 1 ? 0 : (1 << 0)) | (scor == 1 ? 0 : (1 << 1)) |
		(stch == 1 ? 0 : (1 << 2)) | (sten == 1 ? 0 : (1 << 3)) | (1 << 4);
	if (sten == 1)
		sten = -1;
	if (scor == 1)
		scor = -1;
	if (sbcp == 1)
		sbcp = -1;
	return v;
}

static void tp_check_interrupts (void)
{
	/* MC = 1 ? */
	if ((tp_cr & 1) != 1) {
		get_tp_c_level ();
		return;
	}

	tp_ilatch |= get_tp_c_level () ^ 0x1f;
	stch = 0;
	if (!(tp_ilatch & (1 << 5)) && (tp_ilatch & tp_imask)) {
		tp_air = 0;
		int mask = 0x10;
		while (((tp_ilatch & tp_imask) & mask) == 0)
			mask >>= 1;
		tp_air |= tp_ilatch & mask;
		tp_ilatch |= 1 << 5; // IRQ
		tp_ilatch2 = tp_ilatch & mask;
		tp_ilatch &= ~mask;
	}
	if (tp_ilatch & (1 << 5))
		INT2 ();
}

// MC=1, C lines 0-4 = input irq lines, 5 = irq out, 6-7 IO

static void tp_bput (int addr, uae_u8 v)
{
#ifdef CDTV_DEBUG_6525
	if (addr != 1)
		write_log (_T("6525 write %x=%02X PC=%x %d\n"), addr, v, M68K_GETPC, regs.s);
#endif
	switch (addr)
	{
	case 0:
		tp_a = v;
		break;
	case 1:
		tp_b = v;
		break;
	case 2:
		if (tp_cr & 1) {
			// 0 = clear, 1 = ignored
			tp_ilatch &= 0xe0 | v;
		} else {
			tp_c = get_tp_c () & ~tp_cd;
			tp_c |= v & tp_cd;
			if (tp_c & (1 << 5))
				INT2 ();
		}
		break;
	case 3:
		tp_ad = v;
		break;
	case 4:
		tp_bd = v;
		break;
	case 5:
		// data direction (mode=0), interrupt mask (mode=1)
		if (tp_cr & 1) {
			tp_imask = v & 0x1f;
		} else {
			tp_cd = v;
		}
		break;
	case 6:
		tp_cr = v;
		break;
	case 7:
		tp_air = v;
		break;
	}
	cmd = (tp_b >> 0) & 1;
	enable = (tp_b >> 1) & 1;
	xaen = (tp_b >> 2) & 1;
	dten = (tp_b >> 3) & 1;

	if (!volstrobe1 && ((tp_b >> 6) & 1)) {
		dac_control_data_format >>= 1;
		dac_control_data_format |= ((tp_b >> 5) & 1) << 11;
		volstrobe1 = 1;
	} else if (volstrobe1 && !((tp_b >> 6) & 1)) {
		volstrobe1 = 0;
	}
	if (!volstrobe2 && ((tp_b >> 7) & 1)) {
#ifdef CDTV_DEBUG_CMD
		write_log (_T("CDTV CD volume = %d\n"), cd_volume);
#endif
		cd_volume = dac_control_data_format & 1023;
		if (unitnum >= 0)
			sys_command_cd_volume (unitnum, (cd_volume << 5) | (cd_volume >> 5), (cd_volume << 5) | (cd_volume >> 5));
		cd_volume_stored = cd_volume;
		volstrobe2 = 1;
	} else if (volstrobe2 && !((tp_b >> 7) & 1)) {
		volstrobe2 = 0;
	}
	tp_check_interrupts ();
}

static uae_u8 subtransferbuf[SUB_CHANNEL_SIZE];

#define SUBCODE_CYCLES (460)
static int subcode_activecnt;

static void subcode_interrupt(uae_u32 v)
{
	if (subcodeoffset < -1) {
		return;
	}
	if (scor < 0) {
		scor = 0;
		tp_check_interrupts();
	}
	subcode_activecnt--;
	if (subcode_activecnt > 0) {
		if (sbcp || scor) {
			event2_newevent2(SUBCODE_CYCLES, 0, subcode_interrupt);
			return;
		}
	}
	if (sbcp && scor == 0) {
		sbcp = 0;
		// CD+G interrupt didn't read data fast enough, just abort until next packet
		return;
	}
	if (subcodeoffset >= 0) {
		sbcp = 1;
		tp_check_interrupts();
	}
	subcode_activecnt = 2;
	event2_newevent2(SUBCODE_CYCLES, 0, subcode_interrupt);
}

static uae_u8 tp_bget (int addr)
{
	uae_u8 v = 0;
	switch (addr)
	{
	case 0:
		// A = subchannel byte input from serial to parallel converter
		if (subcodeoffset < 0 || subcodeoffset >= SUB_CHANNEL_SIZE) {
#ifdef CDTV_SUB_DEBUG
			write_log (_T("CDTV: requested non-existing subchannel data!? %d\n"), subcodeoffset);
#endif
			v = 0;
		} else {
			v = subtransferbuf[subcodeoffset];
			tp_a = 0;
			tp_a |= (v >> 7) & 1;
			tp_a |= (v >> 5) & 2;
			tp_a |= (v >> 3) & 4;
			tp_a |= (v >> 1) & 8;
			tp_a |= (v << 1) & 16;
			tp_a |= (v << 3) & 32;
			tp_a |= (v << 5) & 64;
			tp_a |= (v << 7) & 128;
			v = tp_a;
			sbcp = 0;
			subcodeoffset++;
			if (subcodeoffset >= SUB_CHANNEL_SIZE) {
				subcodeoffset = -2;
			}
		}
		break;
	case 1:
		v = tp_b;
		break;
	case 2:
		if (tp_cr & 1) {
			v = tp_ilatch | tp_ilatch2;
		} else {
			v = get_tp_c ();
		}
		break;
	case 3:
		v = tp_ad;
		break;
	case 4:
		v = tp_bd;
		break;
	case 5:
		// data direction (mode=0), interrupt mask (mode=1)
		if (tp_cr & 1)
			v = tp_imask;
		else
			v = tp_cd;
		break;
	case 6:
		v = tp_cr;
		break;
	case 7:
		v = tp_air;
		if (tp_cr & 1) {
			tp_ilatch &= ~(1 << 5);
			tp_ilatch2 = 0;
		}
		tp_air = 0;
		break;
	}

	tp_check_interrupts ();

#ifdef CDTV_DEBUG_6525
	if (addr < 7 && addr != 1)
		write_log (_T("6525 read %x=%02X PC=%x %d\n"), addr, v, M68K_GETPC, regs.s);
#endif
	return v;
}

static uae_u32 REGPARAM3 dmac_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dmac_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 dmac_bget (uaecptr) REGPARAM;
static void REGPARAM3 dmac_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dmac_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 dmac_bput (uaecptr, uae_u32) REGPARAM;

static void dmac_start_dma (void)
{
	if (!(dmac_cntr & CNTR_PDMD)) { // non-scsi dma
		write_comm_pipe_u32 (&requests, 0x0100, 1);
	}
#ifdef A2091
	else {
		scsi_dmac_a2091_start_dma (wd_cdtv);
	}
#endif
}
static void dmac_stop_dma (void)
{
	if (!(dmac_cntr & CNTR_PDMD)) { // non-scsi dma
		;
	}
#ifdef A2091
	else {
		scsi_dmac_a2091_stop_dma (wd_cdtv);
	}
#endif
}

void cdtv_getdmadata (uae_u32 *acr)
{
	*acr = dmac_acr;
}

static void cdtv_checkint(void)
{
	int irq = 0;
#ifdef A2091
	if (cdtvscsi && (wdscsi_getauxstatus (&wd_cdtv->wc) & 0x80)) {
		dmac_istr |= ISTR_INTS;
		if ((dmac_cntr & CNTR_INTEN) && (dmac_istr & ISTR_INTS))
			irq = 1;
	}
#endif
	if ((dmac_cntr & CNTR_INTEN) && (dmac_istr & ISTR_E_INT))
		irq = 1;
	if (irq)
		INT2 ();
}

void cdtv_scsi_int (void)
{
	cdtv_checkint();
}
void cdtv_scsi_clear_int (void)
{
	dmac_istr &= ~ISTR_INTS;
}

static void rethink_cdtv (void)
{
	cdtv_checkint();
	tp_check_interrupts ();
}


static void CDTV_hsync_handler (void)
{
	static int subqcnt;

	if (!currprefs.cs_cdtvcd || configured <= 0 || currprefs.cs_cdtvcr)
		return;

	cdtv_hsync++;

	if (dma_wait >= 1024)
		dma_wait -= 1024;
	if (dma_wait >= 0 && dma_wait < 1024 && dma_finished) {
		if ((dmac_cntr & (CNTR_INTEN | CNTR_TCEN)) == (CNTR_INTEN | CNTR_TCEN)) {
			dmac_istr |= ISTR_INT_P | ISTR_E_INT;
#ifdef CDTV_DEBUG_CMD
			write_log (_T("DMA finished\n"));
#endif
		}
		dma_finished = 0;
		cdtv_hsync = -1;
	}
	cdtv_checkint();

	if (cdrom_command_done) {
		cdrom_command_done = 0;
		sten = 1;
		stch = 0;
		tp_check_interrupts ();
	}

	if (sten < 0) {
		sten--;
		if (sten < -3)
			sten = 0;
	}

	if (cd_audio_finished) {
		cd_audio_finished = 0;
		cd_playing = 0;
		cd_finished = 1;
		cd_paused = 0;
		//cd_error = 1;
		write_log (_T("audio finished\n"));
		activate_stch = 1;
	}

	static int subchannelcounter;
	int cntmax = (int)(maxvpos * vblank_hz / 75 - 2);
	if (subchannelcounter > 0)
		subchannelcounter--;
	if (subchannelcounter <= 0) {
		if (cd_playing || cd_media) {
			if (subcodebufferoffset != subcodebufferoffsetw) {
				uae_sem_wait (&sub_sem);
#ifdef CDTV_SUB_DEBUG
				if (subcodeoffset >= 0)
					write_log (_T("CDTV: frame interrupt, subchannel not empty! %d\n"), subcodeoffset);
#endif
				subcodeoffset = -1;
				if (subcodebufferinuse[subcodebufferoffset]) {
					subcodebufferinuse[subcodebufferoffset] = 0;
					memcpy (subtransferbuf, subcodebuffer + subcodebufferoffset * SUB_CHANNEL_SIZE, SUB_CHANNEL_SIZE);
					subcodebufferoffset++;
					if (subcodebufferoffset >= MAX_SUBCODEBUFFER)
						subcodebufferoffset -= MAX_SUBCODEBUFFER;
					sbcp = 0;
					scor = 1;
					subcodeoffset = 0;
					subcode_activecnt = 5;
					event2_newevent_x_remove(subcode_interrupt);
					event2_newevent2 (SUBCODE_CYCLES, 0, subcode_interrupt);
					tp_check_interrupts ();
				}
				uae_sem_post (&sub_sem);
				subchannelcounter = cntmax;
			}
		}
		if (!scor && !cd_playing) {
			// frame interrupts happen all the time motor is running
			scor = 1;
			tp_check_interrupts ();
			scor = 0;
			subchannelcounter = cntmax;
		}
	}

	if (cdtv_hsync < cntmax && cdtv_hsync >= 0)
		return;
	cdtv_hsync = 0;
#if 0
	if (cd_isready > 0) {
		cd_isready--;
		if (!cd_isready)
			do_stch ();
	}
#endif
	if (dmac_dma || dma_finished)
		cd_led |= LED_CD_ACTIVE;
	else
		cd_led &= ~LED_CD_ACTIVE;
	if ((cd_led & ~LED_CD_ACTIVE2) && !cd_playing)
		gui_flicker_led (LED_CD, 0, cd_led);

	subqcnt--;
	if (subqcnt < 0) {
		write_comm_pipe_u32(&requests, 0x0101, 1);
		if (cd_playing)
			subqcnt = 10;
		else
			subqcnt = 75;
	}

	if (activate_stch)
		do_stch();
}

static void do_stch (void)
{
	if ((tp_cr & 1) && !(tp_air & (1 << 2))) {
		stch = 1;
		activate_stch = 0;
		tp_check_interrupts ();
#ifdef CDTV_DEBUG
		static int stch_cnt;
		write_log (_T("STCH %d\n"), stch_cnt++);
#endif
	}
}

static void cdtv_reset_int (void)
{
	write_log (_T("CDTV: reset\n"));
	cdaudiostop ();
	cd_playing = 0;
	cd_paused = 0;
	cd_motor = 0;
	cd_media = 0;
	cd_error = 0;
	cd_finished = 0;
	cd_led = 0;
	stch = 1;
	frontpanel = 1;
}

static uae_u32 dmac_bget2 (uaecptr addr)
{
	static uae_u8 last_out;
	uae_u8 v = 0;

	if (addr < 0x40)
		return dmacmemory[addr];

	if (addr >= 0xb0 && addr < 0xc0)
		return tp_bget ((addr - 0xb0) / 2);

	switch (addr)
	{
	case 0x41:
		v = dmac_istr;
		if (v)
			v |= ISTR_INT_P;
		dmac_istr &= ~0xf;
		break;
	case 0x43:
		v = dmac_cntr;
		break;
	case 0x91:
#ifdef A2091
		if (cdtvscsi)
			v = wdscsi_getauxstatus (&wd_cdtv->wc);
#endif
		break;
	case 0x93:
#ifdef A2091
		if (cdtvscsi) {
			v = wdscsi_get (&wd_cdtv->wc, wd_cdtv);
			cdtv_checkint();
		}
#endif
		break;
	case 0xa1:
		sten = 0;
		if (cdrom_command_cnt_out >= 0) {
			v = last_out = cdrom_command_output[cdrom_command_cnt_out];
			cdrom_command_output[cdrom_command_cnt_out++] = 0;
			if (cdrom_command_cnt_out >= cdrom_command_size_out) {
				cdrom_command_size_out = 0;
				cdrom_command_cnt_out = -1;
				sten = 0;
				tp_check_interrupts ();
			} else {
				sten = 1;
				tp_check_interrupts ();
			}
		} else {
			write_log (_T("CDTV: command register read while empty\n"));
			v = last_out;
		}
		break;
	case 0xe8:
	case 0xe9:
		dmac_istr |= ISTR_FE_FLG;
		break;
		/* XT IO */
	case 0xa3:
	case 0xa5:
	case 0xa7:
		v = 0xff;
		break;
	}

#ifdef CDTV_DEBUG
	if (addr != 0x41)
		write_log (_T("dmac_bget %04X=%02X PC=%08X\n"), addr, v, M68K_GETPC);
#endif

	return v;
}

static void dmac_bput2 (uaecptr addr, uae_u32 b)
{
	if (addr >= 0xb0 && addr < 0xc0) {
		tp_bput ((addr - 0xb0) / 2, b);
		return;
	}

#ifdef CDTV_DEBUG
	write_log (_T("dmac_bput %04X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif

	switch (addr)
	{
	case 0x43:
		dmac_cntr = b;
		if (dmac_cntr & CNTR_PREST)
			cdtv_reset_int ();
		break;
	case 0x80:
		dmac_wtc &= 0x00ffffff;
		dmac_wtc |= b << 24;
		break;
	case 0x81:
		dmac_wtc &= 0xff00ffff;
		dmac_wtc |= b << 16;
		break;
	case 0x82:
		dmac_wtc &= 0xffff00ff;
		dmac_wtc |= b << 8;
		break;
	case 0x83:
		dmac_wtc &= 0xffffff00;
		dmac_wtc |= b << 0;
		break;
	case 0x84:
		dmac_acr &= 0x00ffffff;
		dmac_acr |= b << 24;
		break;
	case 0x85:
		dmac_acr &= 0xff00ffff;
		dmac_acr |= b << 16;
		break;
	case 0x86:
		dmac_acr &= 0xffff00ff;
		dmac_acr |= b << 8;
		break;
	case 0x87:
		dmac_acr &= 0xffffff01;
		dmac_acr |= (b & ~ 1) << 0;
		break;
	case 0x8e:
		dmac_dawr &= 0x00ff;
		dmac_dawr |= b << 8;
		break;
	case 0x8f:
		dmac_dawr &= 0xff00;
		dmac_dawr |= b << 0;
		break;
	case 0x91:
#ifdef A2091
		if (cdtvscsi) {
			wdscsi_sasr (&wd_cdtv->wc, b);
			cdtv_checkint();
		}
#endif
		break;
	case 0x93:
#ifdef A2091
		if (cdtvscsi) {
			wdscsi_put (&wd_cdtv->wc, wd_cdtv, b);
			cdtv_checkint();
		}
#endif
		break;
	case 0xa1:
		cdrom_command (b);
		break;
	case 0xe0:
	case 0xe1:
		if (!dmac_dma) {
			dmac_dma = 1;
			dmac_start_dma ();
		}
		break;
	case 0xe2:
	case 0xe3:
		if (dmac_dma) {
			dmac_dma = 0;
			dmac_stop_dma ();
		}
		dma_finished = 0;
		break;
	case 0xe4:
	case 0xe5:
		dmac_istr = 0;
		cdtv_checkint();
		break;
	case 0xe8:
	case 0xe9:
		dmac_istr |= ISTR_FE_FLG;
		break;
	}

	tp_check_interrupts ();
}

static uae_u32 REGPARAM2 dmac_lget (uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = (dmac_bget2 (addr) << 24) | (dmac_bget2 (addr + 1) << 16) |
		(dmac_bget2 (addr + 2) << 8) | (dmac_bget2 (addr + 3));
#ifdef CDTV_DEBUG
	write_log (_T("dmac_lget %08X=%08X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 REGPARAM2 dmac_wget (uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = (dmac_bget2 (addr) << 8) | dmac_bget2 (addr + 1);
#ifdef CDTV_DEBUG
	write_log (_T("dmac_wget %08X=%04X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 REGPARAM2 dmac_bget (uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = dmac_bget2 (addr);
	if (configured <= 0)
		return v;
	return v;
}

static void REGPARAM2 dmac_lput (uaecptr addr, uae_u32 l)
{
	addr &= 65535;
#ifdef CDTV_DEBUG
	write_log (_T("dmac_lput %08X=%08X PC=%08X\n"), addr, l, M68K_GETPC);
#endif
	dmac_bput2 (addr, l >> 24);
	dmac_bput2 (addr + 1, l >> 16);
	dmac_bput2 (addr + 2, l >> 8);
	dmac_bput2 (addr + 3, l);
}

static void REGPARAM2 dmac_wput (uaecptr addr, uae_u32 w)
{
	addr &= 65535;
#ifdef CDTV_DEBUG
	write_log (_T("dmac_wput %04X=%04X PC=%08X\n"), addr, w & 65535, M68K_GETPC);
#endif
	dmac_bput2 (addr, w >> 8);
	dmac_bput2 (addr + 1, w);
}

static void REGPARAM2 dmac_bput(uaecptr addr, uae_u32 b);

static uae_u32 REGPARAM2 dmac_wgeti(uaecptr addr)
{
	uae_u32 v = 0xffff;
	return v;
}
static uae_u32 REGPARAM2 dmac_lgeti(uaecptr addr)
{
	uae_u32 v = 0xffff;
	return v;
}

static addrbank dmac_bank = {
	dmac_lget, dmac_wget, dmac_bget,
	dmac_lput, dmac_wput, dmac_bput,
	default_xlate, default_check, NULL, NULL, _T("CDTV DMAC/CD Controller"),
	dmac_lgeti, dmac_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

static void REGPARAM2 dmac_bput (uaecptr addr, uae_u32 b)
{
	addr &= 65535;
	b &= 0xff;
	if (addr == 0x48) {
		map_banks_z2 (&dmac_bank, b, 0x10000 >> 16);
		configured = b;
		expamem_next(&dmac_bank, NULL);
		return;
	}
	if (addr == 0x4c) {
		configured = -1;
		expamem_shutup(&dmac_bank);
		return;
	}
	if (configured <= 0)
		return;
	dmac_bput2 (addr, b);
}

static void open_unit (void)
{
	struct device_info di = { 0 };
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

static void ew (int addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		dmacmemory[addr] = (value & 0xf0);
		dmacmemory[addr + 2] = (value & 0x0f) << 4;
	} else {
		dmacmemory[addr] = ~(value & 0xf0);
		dmacmemory[addr + 2] = ~((value & 0x0f) << 4);
	}
}

/* CDTV batterybacked RAM emulation */
#define CDTV_NVRAM_MASK 16383
#define CDTV_NVRAM_SIZE 32768
static uae_u8 cdtv_battram[CDTV_NVRAM_SIZE];
static TCHAR flashfilepath[MAX_DPATH];

static void cdtv_loadcardmem (uae_u8 *p, int size)
{
	struct zfile *f;

	if (!size)
		return;
	memset (p, 0, size);
	cfgfile_resolve_path_out_load(currprefs.flashfile, flashfilepath, MAX_DPATH, PATH_ROM);
	f = zfile_fopen (flashfilepath, _T("rb"), ZFD_NORMAL);
	if (!f)
		return;
	zfile_fseek (f, CDTV_NVRAM_SIZE, SEEK_SET);
	zfile_fread (p, size, 1, f);
	zfile_fclose (f);
}

static void cdtv_savecardmem (uae_u8 *p, int size)
{
	struct zfile *f;

	if (!size)
		return;
	cfgfile_resolve_path_out_load(currprefs.flashfile, flashfilepath, MAX_DPATH, PATH_ROM);
	f = zfile_fopen (flashfilepath, _T("rb+"), ZFD_NORMAL);
	if (!f)
		return;
	zfile_fseek (f, CDTV_NVRAM_SIZE, SEEK_SET);
	zfile_fwrite (p, size, 1, f);
	zfile_fclose (f);
}

static void cdtv_battram_reset (void)
{
	struct zfile *f;
	size_t v;

	memset (cdtv_battram, 0, CDTV_NVRAM_SIZE);
	cfgfile_resolve_path_out_load(currprefs.flashfile, flashfilepath, MAX_DPATH, PATH_ROM);
	f = zfile_fopen (flashfilepath, _T("rb+"), ZFD_NORMAL);
	if (!f) {
		f = zfile_fopen (flashfilepath, _T("wb"), 0);
		if (f) {
			zfile_fwrite (cdtv_battram, CDTV_NVRAM_SIZE, 1, f);
			zfile_fclose (f);
		}
		return;
	}
	v = zfile_fread (cdtv_battram, 1, CDTV_NVRAM_SIZE, f);
	if (v < CDTV_NVRAM_SIZE)
		zfile_fwrite (cdtv_battram + v, 1, CDTV_NVRAM_SIZE - v, f);
	zfile_fclose (f);
}

void cdtv_battram_write (int addr, int v)
{
	struct zfile *f;
	int offset = addr & CDTV_NVRAM_MASK;

	if (offset >= CDTV_NVRAM_SIZE)
		return;
	gui_flicker_led (LED_MD, 0, 2);
	if (cdtv_battram[offset] == v)
		return;
	cdtv_battram[offset] = v;
	f = zfile_fopen (flashfilepath, _T("rb+"), ZFD_NORMAL);
	if (!f)
		return;
	zfile_fseek (f, offset, SEEK_SET);
	zfile_fwrite (cdtv_battram + offset, 1, 1, f);
	zfile_fclose (f);
}

uae_u8 cdtv_battram_read (int addr)
{
	uae_u8 v;
	int offset;
	offset = addr & CDTV_NVRAM_MASK;
	if (offset >= CDTV_NVRAM_SIZE)
		return 0;
	gui_flicker_led (LED_MD, 0, 1);
	v = cdtv_battram[offset];
	return v;
}

/* CDTV expension memory card memory */

MEMORY_FUNCTIONS(cardmem);

addrbank cardmem_bank = {
	cardmem_lget, cardmem_wget, cardmem_bget,
	cardmem_lput, cardmem_wput, cardmem_bput,
	cardmem_xlate, cardmem_check, NULL, _T("rom_e0"), _T("CDTV memory card"),
	cardmem_lget, cardmem_wget,
	ABFLAG_RAM, 0, 0
};

static void cdtv_free (void)
{
	if (thread_alive > 0) {
		dmac_dma = 0;
		dma_finished = 0;
		cdaudiostop ();
		write_comm_pipe_u32 (&requests, 0xffff, 1);
		while (thread_alive > 0)
			sleep_millis (10);
		uae_sem_destroy(&sub_sem);
		uae_sem_destroy(&cda_sem);
	}
	thread_alive = 0;
	close_unit ();
	if (cardmem_bank.baseaddr) {
		cdtv_savecardmem(cardmem_bank.baseaddr, cardmem_bank.allocated_size);
		mapped_free(&cardmem_bank);
		cardmem_bank.baseaddr = NULL;
	}
	configured = 0;
}

bool cdtvsram_init(struct autoconfig_info *aci)
{
	return true;
}

bool cdtv_init(struct autoconfig_info *aci)
{
	memset(dmacmemory, 0xff, sizeof dmacmemory);
	ew(0x00, 0xc0 | 0x01);
	ew(0x04, 0x03);
	ew(0x08, 0x40);
	ew(0x10, 0x02);
	ew(0x14, 0x02);

	ew(0x18, 0x00); /* ser.no. Byte 0 */
	ew(0x1c, 0x00); /* ser.no. Byte 1 */
	ew(0x20, 0x00); /* ser.no. Byte 2 */
	ew(0x24, 0x00); /* ser.no. Byte 3 */

	if (aci) {
		dma_mask = aci->rc->dma24bit ? 0x00ffffff : 0xffffffff;
		aci->label = dmac_bank.name;
		aci->hardwired = true;
		aci->addrbank = &dmac_bank;
		if (!aci->doinit) {
			memcpy(aci->autoconfig_raw, dmacmemory, sizeof dmacmemory);
			return true;
		}
	}

	close_unit ();
	if (!thread_alive) {
		init_comm_pipe (&requests, 100, 1);
		uae_start_thread (_T("cdtv"), dev_thread, NULL, NULL);
		while (!thread_alive)
			sleep_millis (10);
		uae_sem_init(&sub_sem, 0, 1);
		uae_sem_init(&cda_sem, 0, 0);
	}
	write_comm_pipe_u32 (&requests, 0x0104, 1);

	cdrom_command_cnt_out = -1;
	cmd = 0;
	enable = 0;
	xaen = 0;
	dten = 0;

	/* KS autoconfig handles the rest */
	if (!savestate_state) {
		cdtv_reset_int ();
		configured = 0;
		tp_a = tp_b = tp_c = tp_ad = tp_bd = tp_cd = 0;
		tp_imask = tp_cr = tp_air = tp_ilatch = 0;
		stch = 0;
		sten = 0;
		scor = 0;
		sbcp = 0;
		cdtvscsi = 0;
	}

	int cardsize = 0;
	if (aci && is_board_enabled(aci->prefs, ROMTYPE_CDTVSRAM, 0)) {
		struct romconfig *rc = get_device_romconfig(aci->prefs, ROMTYPE_CDTVSRAM, 0);
		cardsize = 64 << (rc->device_settings & 3);
	}
	if (cardmem_bank.reserved_size != cardsize * 1024) {
		mapped_free(&cardmem_bank);
		cardmem_bank.baseaddr = NULL;

		cardmem_bank.reserved_size = cardsize * 1024;
		cardmem_bank.mask = cardmem_bank.reserved_size - 1;
		cardmem_bank.start = 0xe00000;
		if (cardmem_bank.reserved_size) {
			if (!mapped_malloc(&cardmem_bank)) {
				write_log(_T("Out of memory for cardmem.\n"));
				cardmem_bank.reserved_size = 0;
			}
		}
		cdtv_loadcardmem(cardmem_bank.baseaddr, cardmem_bank.reserved_size);
	}
	if (cardmem_bank.baseaddr)
		map_banks(&cardmem_bank, cardmem_bank.start >> 16, cardmem_bank.allocated_size >> 16, 0);

	cdtv_battram_reset ();
	open_unit ();
	gui_flicker_led (LED_CD, 0, -1);

	device_add_hsync(CDTV_hsync_handler);
	device_add_rethink(rethink_cdtv);
	device_add_exit(cdtv_free, NULL);

	return true;
}

bool cdtvscsi_init(struct autoconfig_info *aci)
{
	aci->parent_name = _T("CDTV DMAC");
	aci->hardwired = true;
	if (!aci->doinit)
		return true;
	cdtvscsi = true;
#ifdef A2091
	init_wd_scsi(wd_cdtv, aci->rc->dma24bit);
	wd_cdtv->dmac_type = COMMODORE_DMAC;
#endif
	if (configured > 0)
		map_banks_z2(&dmac_bank, configured, 0x10000 >> 16);
	return true;
}

#ifdef SAVESTATE

uae_u8 *save_cdtv_dmac (size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	
	if (!currprefs.cs_cdtvcd || currprefs.cs_cdtvcr)
		return NULL;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);

	// model (0=original,1=rev2,2=superdmac)
	save_u32 (1);
	save_u32 (0); // reserved flags
	save_u8 (dmac_istr);
	save_u8 (dmac_cntr);
	save_u32 (dmac_wtc);
	save_u32 (dmac_acr);
	save_u16 (dmac_dawr);
	save_u32 (dmac_dma ? 1 : 0);
	save_u8 (configured);
	*len = dst - dstbak;
	return dstbak;

}

uae_u8 *restore_cdtv_dmac (uae_u8 *src)
{
	restore_u32 ();
	restore_u32 ();
	dmac_istr = restore_u8 ();
	dmac_cntr = restore_u8 ();
	dmac_wtc = restore_u32 ();
	dmac_acr = restore_u32 ();
	dmac_dawr = restore_u16 ();
	restore_u32 ();
	configured = restore_u8 ();
	return src;
}

uae_u8 *save_cdtv (size_t *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (!currprefs.cs_cdtvcd || currprefs.cs_cdtvcr)
		return NULL;

	if (dstptr) 
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);

	save_u32 (1);

	// tri-port
	save_u8 (tp_a);
	save_u8 (tp_b);
	save_u8 (tp_c);
	save_u8 (tp_ad);
	save_u8 (tp_bd);
	save_u8 (tp_cd);
	save_u8 (tp_cr);
	save_u8 (tp_air);
	save_u8 (tp_imask);
	save_u8 (tp_ilatch);
	save_u8 (tp_ilatch2);
	save_u8 (0);
	// misc cd stuff
	save_u32 ((cd_playing ? 1 : 0) | (cd_paused ? 2 : 0) | (cd_media ? 4 : 0) |
		(cd_motor ? 8 : 0) | (cd_error ? 16 : 0) | (cd_finished ? 32 : 0) | (cdrom_command_done ? 64 : 0) |
		(activate_stch ? 128 : 0) | (sten ? 256 : 0) | (stch ? 512 : 0) | (frontpanel ? 1024 : 0));
	save_u8 (cd_isready);
	save_u8 (0);
	save_u16 (dac_control_data_format);
	if (cd_playing)
		get_qcode ();
	save_u32 (last_play_pos);
	save_u32 (last_play_end);
	save_u64 (dma_wait);
	for (int i = 0; i < sizeof (cdrom_command_input); i++)
		save_u8 (cdrom_command_input[i]);
	save_u8 (cdrom_command_cnt_in);
	save_u16 (cdtv_sectorsize);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_cdtv (uae_u8 *src)
{
	cdtv_free ();
	if (!currprefs.cs_cdtvcd) {
		changed_prefs.cs_cdtvcd = changed_prefs.cs_cdtvram = true;
		currprefs.cs_cdtvcd = currprefs.cs_cdtvram = true;
		cdtv_init(NULL);
	}
	restore_u32 ();
	
	// tri-port
	tp_a = restore_u8 ();
	tp_b = restore_u8 ();
	tp_c = restore_u8 ();
	tp_ad = restore_u8 ();
	tp_bd = restore_u8 ();
	tp_cd = restore_u8 ();
	tp_cr = restore_u8 ();
	tp_air = restore_u8 ();
	tp_imask = restore_u8 ();
	tp_ilatch = restore_u8 ();
	tp_ilatch2 = restore_u8 ();
	restore_u8 ();
	// misc cd stuff
	uae_u32 v = restore_u32 ();
	cd_playing = (v & 1) ? 1 : 0;
	cd_paused = (v & 2) ? 1 : 0;
	cd_media = (v & 4) ? 1 : 0;
	cd_motor = (v & 8) ? 1 : 0;
	cd_error = (v & 16) ? 1 : 0;
	cd_finished = (v & 32) ? 1 : 0;
	cdrom_command_done = (v & 64) ? 1 : 0;
	activate_stch = (v & 128) ? 1 : 0;
	sten = (v & 256) ? 1 : 0;
	stch = (v & 512) ? 1 : 0;
	frontpanel = (v & 1024) ? 1 : 0;
	cd_isready = restore_u8 ();
	restore_u8 ();
	dac_control_data_format = restore_u16 ();
	last_play_pos = restore_u32 ();
	last_play_end = restore_u32 ();
	dma_wait = restore_u64 ();
	for (int i = 0; i < sizeof (cdrom_command_input); i++)
		cdrom_command_input[i] = restore_u8 ();
	cdrom_command_cnt_in = restore_u8 ();
	cdtv_sectorsize = restore_u16 ();
	cd_audio_status = 0;
	cd_volume_stored = dac_control_data_format & 1023;
	volstrobe1 = volstrobe2 = 1;

	return src;
}

void restore_cdtv_finish (void)
{
	if (!currprefs.cs_cdtvcd || currprefs.cs_cdtvcr)
		return;
	cdtv_init (0);
	get_toc ();
	configured = 0xe90000;
	map_banks_z2(&dmac_bank, configured >> 16, 0x10000 >> 16);
	write_comm_pipe_u32 (&requests, 0x0104, 1);
}

void restore_cdtv_final(void)
{
	if (!currprefs.cs_cdtvcd || currprefs.cs_cdtvcr)
		return;
	if (cd_playing) {
		cd_volume = cd_volume_stored;
		write_comm_pipe_u32(&requests, 0x0111, 0); // play
		write_comm_pipe_u32(&requests, last_play_pos, 0);
		write_comm_pipe_u32(&requests, last_play_end, 0);
		write_comm_pipe_u32(&requests, 0, 1);
		if (cd_paused) {
			write_comm_pipe_u32(&requests, 0x0105, 1); // paused
		} else {
			uae_sem_wait(&cda_sem);
		}
	}
}

#endif /* SAVESTATE */

#endif /* CDTV */
