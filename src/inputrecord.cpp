/*
* UAE - The Un*x Amiga Emulator
*
* Input record/playback
*
* Copyright 2010-2023 Toni Wilen
*
*/

#define INPUTRECORD_DEBUG 1
#define ENABLE_DEBUGGER 0

#define HEADERSIZE 16

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "inputrecord.h"
#include "inputdevice.h"
#include "zfile.h"
#include "custom.h"
#include "savestate.h"
#include "events.h"
#include "uae.h"
#include "disk.h"
#include "fsdb.h"
#include "xwin.h"

#if INPUTRECORD_DEBUG > 0
#include "memory.h"
#include "newcpu.h"
#endif

int inputrecord_debug = 1 | 2;

extern int inputdevice_logging;

#define INPREC_BUFFER_SIZE 10000

static uae_u8 *inprec_buffer, *inprec_p;
static struct zfile *inprec_zf;
static int inprec_size;
int input_record = 0;
int input_play = 0;
static uae_u8 *inprec_plast, *inprec_plastptr;
static int header_end, header_end2;
static int replaypos;
static int lasthsync, endhsync;
static TCHAR inprec_path[MAX_DPATH];
static uae_u32 seed;
static frame_time_t lastcycle;
static uae_u32 cycleoffset;

static uae_u32 pcs[16];
static uae_u64 pcs2[16];
#ifdef DEBUGGER
extern void activate_debugger (void);
#endif
static int warned;

static void setlasthsync (void)
{
	if (lasthsync / current_maxvpos () != hsync_counter / current_maxvpos ()) {
		lasthsync = hsync_counter;
		refreshtitle ();
	}
}

static void flush (void)
{
	if (inprec_p > inprec_buffer) {
		zfile_fwrite (inprec_buffer, inprec_p - inprec_buffer, 1, inprec_zf);
		inprec_p = inprec_buffer;
	}
}

static void inprec_ru8 (uae_u8 v)
{
	if (!input_record || !inprec_zf)
		return;
	*inprec_p++= v;
}
static void inprec_ru16 (uae_u16 v)
{
	if (!input_record || !inprec_zf)
		return;
	inprec_ru8 ((uae_u8)(v >> 8));
	inprec_ru8 ((uae_u8)v);
}
void inprec_ru32 (uae_u32 v)
{
	if (!input_record || !inprec_zf)
		return;
	inprec_ru16 ((uae_u16)(v >> 16));
	inprec_ru16 ((uae_u16)v);
}
void inprec_ru64(uae_u64 v)
{
	if (!input_record || !inprec_zf)
		return;
	inprec_ru32((uae_u32)(v >> 32));
	inprec_ru32((uae_u32)v);
}
static void inprec_rstr (const TCHAR *src)
{
	if (!input_record || !inprec_zf)
		return;
	char *s = uutf8 (src);
	char *ss = s;
	while (*s) {
		inprec_ru8 (*s);
		s++;
	}
	inprec_ru8 (0);
	xfree (ss);
}

static bool inprec_rstart (uae_u8 type)
{
	if (!input_record || !inprec_zf || input_record == INPREC_RECORD_PLAYING)
		return false;
	int hpos = current_hpos();
	lastcycle = get_cycles ();
	int mvp = current_maxvpos ();
	if ((type < INPREC_DEBUG_START || type > INPREC_DEBUG_END) || (0 && vsync_counter >= 49 && vsync_counter <= 51))
		write_log (_T("INPREC: %010d/%03d: %d (%d/%d) %08x\n"), hsync_counter, hpos, type, hsync_counter % mvp, mvp, lastcycle);
	inprec_plast = inprec_p;
	inprec_ru8 (type);
	inprec_ru16 (0xffff);
	inprec_ru32 (hsync_counter);
	inprec_ru8 (hpos);
	inprec_ru64 (lastcycle);
	return true;
}

static void inprec_rend (void)
{
	if (!input_record || !inprec_zf)
		return;
	int size = addrdiff(inprec_p, inprec_plast);
	inprec_plast[1] = size >> 8;
	inprec_plast[2] = size >> 0;
	flush ();
	endhsync = hsync_counter;
	setlasthsync ();
}

static bool inprec_realtime (bool stopstart)
{
	if (input_record == INPREC_RECORD_RERECORD)
		gui_message (_T("INPREC error"));
	write_log (_T("INPREC: play -> record\n"));
	input_record = INPREC_RECORD_RERECORD;
	input_play = 0;
	int offset = addrdiff(inprec_p, inprec_buffer);
	zfile_fseek (inprec_zf, offset, SEEK_SET);
	zfile_truncate (inprec_zf, offset);
	xfree (inprec_buffer);
	inprec_size = INPREC_BUFFER_SIZE;
	inprec_buffer = inprec_p = xmalloc (uae_u8, inprec_size);
	clear_inputstate ();
	return true;
}

static void inprec_event(uae_u32 v)
{
	inputdevice_playevents();
}

static int inprec_pstart (uae_u8 type)
{
	uae_u8 *p = inprec_p;
	uae_u32 hc = hsync_counter;
	uae_u8 hpos = current_hpos ();
	frame_time_t cycles = get_cycles ();
	static uae_u8 *lastp;
	uae_u32 hc_orig, hc2_orig;
	int mvp = current_maxvpos ();

	if (!input_play || !inprec_zf)
		return 0;
	if (savestate_state || hsync_counter > 0xffff0000)
		return 0;
	if (p == inprec_buffer + inprec_size) {
		write_log (_T("INPREC: STOP\n"));
		if (input_play == INPREC_PLAY_RERECORD) {
			input_play = 0;
			inprec_realtime (true);
		} else {
			inprec_close (true);
		}
		return 0;
	} else if (p > inprec_buffer + inprec_size) {
		write_log (_T("INPREC: buffer error\n"));
		gui_message (_T("INPREC error"));
	}
	if (p[0] == INPREC_END) {
		inprec_close (true);
		return 0;
	} else if (p[0] == INPREC_QUIT) {
		inprec_close (true);
		uae_quit ();
		return 0;
	}
	hc_orig = hc;
	for (;;) {
		uae_u32 type2 = p[0];
		uae_u32 hc2 = (p[3] << 24) | (p[4] << 16) | (p[5] << 8) | p[6];
		uae_u32 hpos2 = p[7];
		uae_u32 chi = (p[8] << 24) | (p[9] << 16) | (p[10] << 8) | p[11];
		uae_u32 clo = (p[12] << 24) | (p[13] << 16) | (p[14] << 8) | p[15];
		frame_time_t cycles2 = (((uae_u64)chi) << 32) | clo;

		if (p >= inprec_buffer + inprec_size)
			break;
#if 0
		if (p > lastp) {
			write_log (_T("INPREC: Next %010d/%03d, %010d/%03d (%d/%d): %d (%d)\n"),
				hc2, hpos2, hc, hpos, hc2 - hc, hpos2 - hpos, p[5 + 1], p[5]);
			lastp = p;
		}
#endif
		hc2_orig = hc2;
		if (type2 == type && cycles > cycles2) {
			int diff = (int)((cycles2 - cycles) / CYCLE_UNIT);
			write_log (_T("INPREC: %010d/%03d > %010d/%03d: %d %d missed!\n"), hc, hpos, hc2, hpos2, diff, p[0]);
#if ENABLE_DEBUGGER == 0
			gui_message (_T("INPREC missed error"));
#else
			activate_debugger ();
#endif
			lastcycle = cycles;
			inprec_plast = p;
			inprec_plastptr = p + HEADERSIZE;
			setlasthsync ();
			return 1;
		}
		if (cycles < cycles2) {
			if (type2 < INPREC_DEBUG_START || type2 > INPREC_DEBUG_END) {
				int diff = (uae_u32)((cycles2 - cycles) / CYCLE_UNIT);
				if (diff < maxhpos) {
					event2_newevent_x_replace(diff, 0, inprec_event);
				}
			}
			lastp = p;
			break;
		}
		if (type2 == type) {
			if ((type < INPREC_DEBUG_START || type > INPREC_DEBUG_END) && cycles != cycles2)
				write_log (_T("INPREC: %010d/%03d: %d (%d/%d) (%d/%d) %08X/%08X\n"), hc, hpos, type, hc % mvp, mvp, hc_orig - hc2_orig, hpos - hpos2, cycles, cycles2);
			if (cycles != cycles2 + cycleoffset) {
				if (warned > 0) {
					warned--;
					for (int i = 0; i < 7; i++)
						write_log (_T("%08x (%016llx) "), pcs[i], pcs2[i]);
					write_log (_T("\n"));
				}
				uae_u32 fixedcycleoffset = (uae_u32)(cycles - cycles2);
#if ENABLE_DEBUGGER == 0
				gui_message (_T("INPREC OFFSET=%d\n"), fixedcycleoffset / CYCLE_UNIT);
#else
				activate_debugger ();
#endif
				cycleoffset = fixedcycleoffset;
			}
			lastcycle = cycles;
			inprec_plast = p;
			inprec_plastptr = p + HEADERSIZE;
			setlasthsync();
			//write_log(_T("INPREC: %010d/%03d %llx %d\n"), hc, hpos, cycles, p[0]);
			return 1;
		}
		if (type2 == INPREC_END || type2 == INPREC_QUIT)
			break;
		p += (p[1] << 8) | (p[2] << 0);
	}
	inprec_plast = NULL;
	return 0;
}

static void inprec_pend (void)
{
	uae_u8 *p = inprec_p;
	uae_u32 hc = hsync_counter;
	uae_u32 hpos = current_hpos ();

	if (!input_play || !inprec_zf)
		return;
	if (!inprec_plast)
		return;
	inprec_plast[0] |= 0x80;
	inprec_plast = NULL;
	inprec_plastptr = NULL;
	for (;;) {
		uae_u32 hc2 = (p[3] << 24) | (p[4] << 16) | (p[5] << 8) | p[6];
		uae_u32 hpos2 = p[7];
		if (hc2 != hc)
			break;
		if ((p[0] & 0x80) == 0)
			return;
		p += (p[1] << 8) | (p[2] << 0);
		inprec_p = p;
	}
}

static uae_u8 inprec_pu8 (void)
{
	return *inprec_plastptr++;
}
static uae_u16 inprec_pu16 (void)
{
	uae_u16 v = inprec_pu8 () << 8;
	v |= inprec_pu8 ();
	return v;
}
static uae_s16 inprec_ps16 (void)
{
	uae_u16 v = inprec_pu8 () << 8;
	v |= inprec_pu8 ();
	return (uae_s16)v;
}
static uae_u32 inprec_pu32 (void)
{
	uae_u32 v = inprec_pu16 () << 16;
	v |= inprec_pu16 ();
	return v;
}
static uae_u64 inprec_pu64(void)
{
	uae_u64 v = (uae_u64)inprec_pu32() << 32;
	v |= inprec_pu32();
	return v;
}
static int inprec_pstr (TCHAR *dst)
{
	char tmp[MAX_DPATH];
	char *s;
	int len = 0;

	*dst = 0;
	s = tmp;
	for(;;) {
		char v = inprec_pu8 ();
		*s++ = v;
		if (!v)
			break;
		len++;
	}
	if (tmp[0]) {
		TCHAR *d = utf8u (tmp);
		_tcscpy (dst, d);
		xfree (d);
	}
	return len;
}

static void findlast (void)
{
	uae_u32 hsync = 0;
	uae_u8 *p = inprec_p;
	while (p < inprec_buffer + inprec_size) {
		hsync = (p[3] << 24) | (p[4] << 16) | (p[5] << 8) | p[6];
		uae_u16 len = (p[1] << 8) | (p[2] << 0);
		p += len;
	}
	endhsync = hsync;
}


int inprec_open (const TCHAR *fname, const TCHAR *statefilename)
{
	int i;

	inprec_close (false);
	if (fname == NULL)
		inprec_zf = zfile_fopen_empty (NULL, _T("inp"));
	else
		inprec_zf = zfile_fopen (fname, input_record ? _T("wb") : _T("rb"), ZFD_NORMAL);
	if (inprec_zf == NULL)
		return 0;

	currprefs.cs_rtc = changed_prefs.cs_rtc = 0;

	inprec_path[0] = 0;
	if (fname)
		getpathpart (inprec_path, sizeof inprec_path / sizeof (TCHAR), fname);
	seed = (uae_u32)time(0);
	inprec_size = INPREC_BUFFER_SIZE;
	lasthsync = 0;
	endhsync = 0;
	warned = 10;
	cycleoffset = 0;
	header_end2 = 0;
	if (input_play) {
		uae_u32 id;
		zfile_fseek(inprec_zf, 0, SEEK_END);
		inprec_size = zfile_ftell32(inprec_zf);
		zfile_fseek(inprec_zf, 0, SEEK_SET);
		inprec_buffer = inprec_p = xmalloc (uae_u8, inprec_size);
		zfile_fread(inprec_buffer, inprec_size, 1, inprec_zf);
		inprec_plastptr = inprec_buffer;
		id = inprec_pu32();
		if (id != 'UAE\0') {
			inprec_close(true);
			return 0;
		}
		int v = inprec_pu8();
		if (v != 3) {
			inprec_close(true);
			return 0;
		}
		inprec_pu8();
		inprec_pu8();
		inprec_pu8();
		seed = inprec_pu32();
		seed = uaesetrandseed(seed);
		vsync_counter = inprec_pu32();
		hsync_counter = inprec_pu32();
		i = inprec_pu32();
		while (i-- > 0)
			inprec_pu8();
		header_end = addrdiff(inprec_plastptr, inprec_buffer);
		inprec_pstr (savestate_fname);
		if (savestate_fname[0]) {
			savestate_state = STATE_RESTORE;
			for (;;) {
				TCHAR tmp[MAX_DPATH];
				_tcscpy (tmp, fname);
				_tcscat (tmp, _T(".uss"));
				if (zfile_exists (tmp)) {
					_tcscpy (savestate_fname, tmp);
					break;
				}
				if (zfile_exists (savestate_fname))
					break;
				TCHAR *p = _tcsrchr (savestate_fname, '\\');
				if (!p)
					p = _tcsrchr (savestate_fname, '/');
				if (!p)
					p = savestate_fname;
				else
					p++;
				if (zfile_exists (p)) {
					_tcscpy (savestate_fname, p);
					break;
				}
				get_savestate_path (tmp, sizeof tmp / sizeof (TCHAR));
				_tcscat (tmp, p);
				if (zfile_exists (tmp)) {
					_tcscpy (savestate_fname, tmp);
					break;
				}
				fetch_inputfilepath (tmp, sizeof tmp / sizeof (TCHAR));
				_tcscat (tmp, p);
				if (zfile_exists (tmp)) {
					_tcscpy (savestate_fname, tmp);
					break;
				}
				write_log (_T("Failed to open linked statefile '%s'\n"), savestate_fname);
				savestate_fname[0] = 0;
				savestate_state = 0;
				break;
			}
		}
		inprec_p = inprec_plastptr;
		header_end2 = addrdiff(inprec_plastptr,  inprec_buffer);
		findlast ();
	} else if (input_record) {
		seed = uaesetrandseed(seed);
		inprec_buffer = inprec_p = xmalloc(uae_u8, inprec_size);
		inprec_ru32('UAE\0');
		inprec_ru8(3);
		inprec_ru8(UAEMAJOR);
		inprec_ru8(UAEMINOR);
		inprec_ru8(UAESUBREV);
		inprec_ru32(seed);
		inprec_ru32(vsync_counter);
		inprec_ru32(hsync_counter);
		inprec_ru32(0); // extra header size
		flush ();
		header_end2 = header_end = zfile_ftell32(inprec_zf);
	} else {
		input_record = input_play = 0;
		return 0;
	}
	if (inputrecord_debug) {
		if (disk_debug_logging < 1)
			disk_debug_logging = 1 | 2;
	}
	write_log (_T("inprec initialized '%s', play=%d rec=%d\n"), fname ? fname : _T("<internal>"), input_play, input_record);
	refreshtitle ();
	return 1;
}

void inprec_startup (void)
{
	uaesetrandseed(seed);
}

bool inprec_prepare_record (const TCHAR *statefilename)
{
	TCHAR state[MAX_DPATH];
	int mode = statefilename ? 2 : 1;
	state[0] = 0;
	if (statefilename)
		_tcscpy (state, statefilename);
	if (hsync_counter > 0 && savestate_state == 0) {
		TCHAR *s = _tcsrchr (changed_prefs.inprecfile, '\\');
		if (!s)
			s = _tcsrchr (changed_prefs.inprecfile, '/');
		if (s) {
			get_savestate_path (state, sizeof state / sizeof (TCHAR));
			_tcscat (state, s + 1);
		} else {
			_tcscpy (state, changed_prefs.inprecfile);
		}
		_tcscat (state, _T(".uss"));
		savestate_initsave (state, 1, 1, true); 
		save_state (state, _T("input recording test"));
		mode = 2;
	}
	input_record = INPREC_RECORD_NORMAL;
	inprec_open (changed_prefs.inprecfile, state);
	changed_prefs.inprecfile[0] = currprefs.inprecfile[0] = 0;
	return true;
}


void inprec_close (bool clear)
{
	if (clear)
		input_play = input_record = 0;
	if (!inprec_zf)
		return;
	if (inprec_buffer && input_record) {
		if (inprec_rstart (INPREC_END))
			inprec_rend ();
	}
	zfile_fclose (inprec_zf);
	inprec_zf = NULL;
	xfree (inprec_buffer);
	inprec_buffer = NULL;
	input_play = input_record = 0;
	write_log (_T("inprec finished\n"));
	refreshtitle ();
}

static void setwriteprotect (const TCHAR *fname, bool readonly)
{
	struct mystat st;
	int mode, oldmode;
	if (!my_stat (fname, &st))
		return;
	oldmode = mode = st.mode;
	mode &= ~FILEFLAG_WRITE;
	if (!readonly)
		mode |= FILEFLAG_WRITE;
	if (mode != oldmode)
		my_chmod (fname, mode);
}

void inprec_playdiskchange (void)
{
	if (!input_play)
		return;
	while (inprec_pstart (INPREC_DISKREMOVE)) {
		int drv = inprec_pu8 ();
		inprec_pend ();
		write_log (_T("INPREC: disk eject drive %d\n"), drv);
		disk_eject (drv);
	}
	while (inprec_pstart (INPREC_DISKINSERT)) {
		int drv = inprec_pu8 ();
		bool wp = inprec_pu8 () != 0;
		TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];
		inprec_pstr (tmp);
		_tcscpy (tmp2, tmp);
		if (!zfile_exists (tmp)) {
			TCHAR tmp3[MAX_DPATH];
			_tcscpy (tmp3, inprec_path);
			_tcscat (tmp3, tmp);
			_tcscpy (tmp, tmp3);
		}
		if (!zfile_exists (tmp)) {
			gui_message (_T("INPREC: Disk image\n'%s'\nnot found!\n"), tmp2);
		}
		_tcscpy (currprefs.floppyslots[drv].df, tmp);
		_tcscpy (changed_prefs.floppyslots[drv].df, tmp);
		setwriteprotect (tmp, wp);
		disk_insert_force (drv, tmp, wp);
		write_log (_T("INPREC: disk insert drive %d '%s'\n"), drv, tmp);
		inprec_pend ();
	}
}

bool inprec_playevent (int *nr, int *state, int *max, int *autofire)
{
	if (inprec_pstart (INPREC_EVENT)) {
		*nr = inprec_ps16 ();
		*state = inprec_ps16 ();
		*max = inprec_pu16 ();
		*autofire = inprec_ps16 () & 1;
		inprec_pend ();
		return true;
	}
	return false;
}

void inprec_recorddebug_cia (uae_u32 v1, uae_u32 v2, uae_u32 v3)
{
#if INPUTRECORD_DEBUG > 0
	if (inprec_rstart (INPREC_CIADEBUG)) {
		inprec_ru32 (v1);
		inprec_ru32 (v2);
		inprec_ru32 (v3);
		inprec_rend ();
	}
#endif
}
void inprec_playdebug_cia (uae_u32 v1, uae_u32 v2, uae_u32 v3)
{
#if INPUTRECORD_DEBUG > 0
	int err = 0;
	if (inprec_pstart (INPREC_CIADEBUG)) {
		uae_u32 vv1 = inprec_pu32 ();
		uae_u32 vv2 = inprec_pu32 ();
		uae_u32 vv3 = inprec_pu32 ();
		if (vv1 != v1 || vv2 != v2 || vv3 != v3)
			write_log (_T("CIA SYNC ERROR %08x,%08x %08x,%08x %08x,%08x\n"), vv1, v1, vv2, v2, vv3, v3);
		inprec_pend ();
	}
#endif
}

void inprec_recorddebug_cpu (int mode, uae_u16 data)
{
#if INPUTRECORD_DEBUG > 0
	if (inprec_rstart (INPREC_CPUDEBUG + mode)) {
		inprec_ru32(m68k_getpc());
		inprec_ru16(data);
		inprec_rend ();
	}
#endif
}
void inprec_playdebug_cpu (int mode, uae_u16 data)
{
#if INPUTRECORD_DEBUG > 0
	int err = 0;
	if (inprec_pstart (INPREC_CPUDEBUG + mode)) {
		uae_u32 pc1 = m68k_getpc();
		uae_u32 pc2 = inprec_pu32();
		uae_u16 data2 = inprec_pu16();
		if (pc1 != pc2 || data != data2) {
			if (warned > 0) {
				warned--;
				write_log (_T("SYNC ERROR2 PC %08x - %08x, D %04x - %04x, M %d\n"), pc1, pc2, data, data2, mode);
				for (int i = 0; i < 15; i++)
					write_log (_T("%08x "), pcs[i]);
				write_log (_T("\n"));

			}
			err = 1;
		} else {
			memmove(pcs + 1, pcs, 15 * sizeof(uae_u32));
			pcs[0] = pc1;
			memmove(pcs2 + 1, pcs2, 15 * sizeof(uae_u64));
			pcs2[0] = get_cycles();
		}
		inprec_pend ();
	} else if (input_play > 0) {
		if (warned > 0) {
			warned--;
			write_log (_T("SYNC ERROR2 debug event missing!?\n"));
		}
	}
#endif
}

void inprec_recorddebug (uae_u32 val)
{
#if INPUTRECORD_DEBUG > 0
	if (inprec_rstart (INPREC_DEBUG)) {
		inprec_ru32 (uaerandgetseed ());
		inprec_ru32 (val);
		inprec_rend ();
	}
#endif
}
void inprec_playdebug (uae_u32 val)
{
#if INPUTRECORD_DEBUG > 0
	static uae_u32 pcs[16];
	int err = 0;
	if (inprec_pstart (INPREC_DEBUG)) {
		uae_u32 seed1 = uaerandgetseed ();
		uae_u32 seed2 = inprec_pu32 ();
		if (seed1 != seed2) {
			write_log (_T("SYNC ERROR seed %08x != %08x\n"), seed1, seed2);
			err = 1;
		}
		uae_u32 val2 = inprec_pu32 ();
		if (val != val2) {
			write_log (_T("SYNC ERROR val %08x != %08x\n"), val, val2);
			err = 1;
		}
		inprec_pend ();
	} else if (input_play > 0) {
		gui_message (_T("SYNC ERROR debug event missing!?\n"));
	}
#endif
}


void inprec_recordevent (int nr, int state, int max, int autofire)
{
	if (savestate_state)
		return;
	if (input_record < INPREC_RECORD_NORMAL)
		return;
	if (inprec_rstart (INPREC_EVENT)) {
		inprec_ru16 (nr);
		inprec_ru16 (state);
		inprec_ru16 (max);
		inprec_ru16 (autofire ? 1 : 0);
		inprec_rend ();
		if (input_record == INPREC_RECORD_NORMAL)
			input_record = INPREC_RECORD_RERECORD;
	}
}

void inprec_recorddiskchange (int nr, const TCHAR *fname, bool writeprotected)
{
	if (savestate_state)
		return;
	if (input_record < INPREC_RECORD_NORMAL)
		return;
	if (fname && fname[0]) {
		if (inprec_rstart (INPREC_DISKINSERT)) {
			inprec_ru8 (nr);
			inprec_ru8 (writeprotected ? 1 : 0);
			inprec_rstr (fname);
			write_log (_T("INPREC: disk insert %d '%s'\n"), nr, fname);
			inprec_rend ();
		}
	} else {
		if (inprec_rstart (INPREC_DISKREMOVE)) {
			inprec_ru8 (nr);
			write_log (_T("INPREC: disk eject %d\n"), nr);
			inprec_rend ();
		}
	}
}

int inprec_getposition (void)
{
	int pos = -1;
	if (input_play == INPREC_PLAY_RERECORD) {
		pos = addrdiff(inprec_p, inprec_buffer);
	} else if (input_record) {
		pos = zfile_ftell32(inprec_zf);
	}
	write_log (_T("INPREC: getpos=%d cycles=%08X\n"), pos, lastcycle);
	if (pos < 0) {
		write_log (_T("INPREC: getpos failure\n"));
		gui_message (_T("INPREC error"));
	}
	return pos;
}

// normal play to re-record
void inprec_playtorecord (void)
{
	write_log (_T("INPREC: PLAY to RE-RECORD\n"));
	replaypos = 0;
	findlast ();
	input_play = INPREC_PLAY_RERECORD;
	input_record = INPREC_RECORD_PLAYING;
	zfile_fclose (inprec_zf);
	inprec_zf = zfile_fopen_empty (NULL, _T("inp"));
	zfile_fwrite (inprec_buffer, header_end2, 1, inprec_zf);
	uae_u8 *p = inprec_buffer + header_end2;
	uae_u8 *end = inprec_buffer + inprec_size;
	while (p < end) {
		int len = (p[1] << 8) | (p[2] << 0);
		p[0] &= ~0x80;
		p += len;
	}
	zfile_fwrite (inprec_buffer + header_end2, inprec_size - header_end2, 1, inprec_zf);
	inprec_realtime (false);
	savestate_capture_request ();
}

void inprec_setposition (int offset, int replaycounter)
{
	if (!inprec_buffer)
		return;
	replaypos = replaycounter;
	write_log (_T("INPREC: setpos=%d\n"), offset);
	if (offset < header_end || offset > zfile_size (inprec_zf)) {
		write_log (_T("INPREC: buffer corruption. offset=%d, size=%d\n"), offset, zfile_size (inprec_zf));
		gui_message (_T("INPREC error"));
	}
	zfile_fseek (inprec_zf, 0, SEEK_SET);
	xfree (inprec_buffer);
	inprec_size = zfile_size32(inprec_zf);
	inprec_buffer = xmalloc (uae_u8, inprec_size);
	zfile_fread (inprec_buffer, inprec_size, 1, inprec_zf);
	inprec_p = inprec_plastptr = inprec_buffer + offset;
	findlast ();
	input_play = INPREC_PLAY_RERECORD;
	input_record = INPREC_RECORD_PLAYING;
	if (currprefs.inprec_autoplay == false)
		inprec_realtime (false);
}

static void savelog (const TCHAR *path, const TCHAR *file)
{
	TCHAR tmp[MAX_DPATH];

	_tcscpy (tmp, path);
	_tcscat (tmp, file);
	_tcscat (tmp, _T(".log.txt"));
	struct zfile *zfd = zfile_fopen (tmp, _T("wb"));
	if (zfd) {
		size_t loglen;
		uae_u8 *log;
		loglen = 0;
		log = save_log (TRUE, &loglen);
		if (log)
			zfile_fwrite (log, loglen, 1, zfd);
		xfree (log);
		loglen = 0;
		log = save_log (FALSE, &loglen);
		if (log)
			zfile_fwrite (log, loglen, 1, zfd);
		xfree (log);
		zfile_fclose (zfd);
		write_log (_T("log '%s' saved\n"), tmp);
	}
}

static int savedisk (const TCHAR *path, const TCHAR *file, uae_u8 *data, uae_u8 *outdata)
{
	int len = 0;
	TCHAR *fname = utf8u ((const char*)data + 2);
	if (fname[0]) {
		TCHAR tmp[MAX_DPATH];
		TCHAR filename[MAX_DPATH];
		filename[0] = 0;
		struct zfile *zf = zfile_fopen (fname, _T("rb"), ZFD_NORMAL);
		if (!zf) {
			_tcscpy (tmp, path);
			_tcscat (tmp, fname);
			zf = zfile_fopen (tmp, _T("rb"), ZFD_NORMAL);
			if (!zf)
				write_log (_T("failed to open '%s'\n"), tmp);
		}
		if (zf) {
			_tcscpy (tmp, path);
			_tcscpy (filename, file);
			_tcscat (filename, _T("."));
			getfilepart (filename + _tcslen (filename), MAX_DPATH, zfile_getname (zf));
			_tcscat (tmp, filename);
			struct zfile *zfd = zfile_fopen (tmp, _T("wb"));
			if (zfd) {
				int size = zfile_size32(zf);
				uae_u8 *data = zfile_getdata (zf, 0, size, NULL);
				zfile_fwrite (data, size, 1, zfd);
				zfile_fclose (zfd);
				xfree (data);
			}
			zfile_fclose (zf);
			setwriteprotect (fname, data[1] != 0);
		}
		if (filename[0]) {
			outdata[0] = data[0];
			char *fn = uutf8 (filename);
			strcpy ((char*)outdata + 2, fn);
			xfree (fn);
			len = 2 + uaestrlen((char*)outdata + 2) + 1;
		}
	}
	xfree (fname);
	return len;
}

void inprec_save (const TCHAR *filename, const TCHAR *statefilename)
{
	TCHAR path[MAX_DPATH], file[MAX_DPATH];
	if (!inprec_buffer)
		return;
	getpathpart (path, sizeof path / sizeof (TCHAR), filename);
	getfilepart (file, sizeof file / sizeof (TCHAR), filename);
	struct zfile *zf = zfile_fopen (filename, _T("wb"), 0);
	if (zf) {
		TCHAR fn[MAX_DPATH];
		uae_u8 *data;
		data = zfile_getdata (inprec_zf, 0, header_end, NULL);
		zfile_fwrite (data, header_end, 1, zf);
		xfree (data);
		getfilepart (fn, MAX_DPATH, statefilename);
		char *s = uutf8 (fn);
		zfile_fwrite (s, strlen (s) + 1, 1, zf);
		int len = zfile_size32(inprec_zf) -  header_end2;
		data = zfile_getdata (inprec_zf, header_end2, len, NULL);
		uae_u8 *p = data;
		uae_u8 *end = data + len;
		while (p < end) {
			uae_u8 tmp[MAX_DPATH];
			int plen = (p[1] << 8) | (p[2] << 0);
			int wlen = plen - HEADERSIZE;
			memcpy (tmp, p + HEADERSIZE, wlen);
			if (p[0] == INPREC_DISKINSERT) {
				wlen = savedisk (path, file, p + HEADERSIZE, tmp);
			}
			if (wlen) {
				wlen += HEADERSIZE;
				p[1] = wlen >> 8;
				p[2] = wlen;
				zfile_fwrite (p, HEADERSIZE, 1, zf);
				zfile_fwrite (tmp, wlen - HEADERSIZE, 1, zf);
			} else {
				zfile_fwrite (p, plen, 1, zf);
			}
			p += plen;
		}
		xfree (data);
		zfile_fclose (zf);
		savelog (path, file);
		write_log (_T("inputfile '%s' saved\n"), filename);
	} else {
		write_log (_T("failed to open '%s'\n"), filename);
	}
}

bool inprec_realtime (void)
{
	if (input_record != INPREC_RECORD_PLAYING || input_play != INPREC_PLAY_RERECORD)
		return false;
	//clear_inputstate ();
	return inprec_realtime (false);
}

void inprec_getstatus (TCHAR *title)
{
	TCHAR *p;
	if (!input_record && !input_play)
		return;
	_tcscat (title, _T("["));
	if (input_record) {
		if (input_record != INPREC_RECORD_PLAYING)
			_tcscat (title, _T("-REC-"));
		else
			_tcscat (title, _T("REPLAY"));
	} else if (input_play) {
		_tcscat (title, _T("PLAY-"));
	}
	_tcscat (title, _T(" "));
	p = title + _tcslen (title);
	int mvp = current_maxvpos ();
	_sntprintf (p, sizeof p, _T("%03d %02d:%02d:%02d/%02d:%02d:%02d"), replaypos,
		(int)(lasthsync / (vblank_hz * mvp * 60)), ((int)(lasthsync / (vblank_hz * mvp)) % 60), (lasthsync / mvp) % (int)vblank_hz,
		(int)(endhsync / (vblank_hz * mvp * 60)), ((int)(endhsync / (vblank_hz * mvp)) % 60), (endhsync / mvp) % (int)vblank_hz);
	p += _tcslen (p);
	_tcscat (p, _T("] "));

}
