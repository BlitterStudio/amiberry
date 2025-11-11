/*
* UAE - The Un*x Amiga Emulator
*
* Save/restore emulator state
*
* (c) 1999-2001 Toni Wilen
*
* see below for ASF-structure
*/

/* Features:
*
* - full CPU state (68000/68010/68020/68030/68040/68060)
* - FPU (68881/68882/68040/68060)
* - full CIA-A and CIA-B state (with all internal registers)
* - saves all custom registers and audio internal state.
* - Chip, Bogo, Fast, Z3 and Picasso96 RAM supported
* - disk drive type, imagefile, track and motor state
* - Kickstart ROM version, address and size is saved. This data is not used during restore yet.
* - Action Replay state is saved
*/

/* Notes:
*
* - blitter state is not saved, blitter is forced to finish immediately if it
*   was active
* - disk DMA state is completely saved
* - does not ask for statefile name and description. Currently uses DF0's disk
*   image name (".adf" is replaced with ".asf")
* - only Amiga state is restored, harddisk support, autoconfig, expansion boards etc..
*   are not saved/restored (and probably never will).
* - use this for saving games that can't be saved to disk
*/

/* Usage :
*
* save:
*
* set savestate_state = STATE_DOSAVE, savestate_filename = "..."
*
* restore:
*
* set savestate_state = STATE_DORESTORE, savestate_filename = "..."
*
*/

#define OPEN_LOG 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "zfile.h"
#include "autoconf.h"
#include "custom.h"
#include "newcpu.h"
#include "savestate.h"
#include "uae.h"
#include "gui.h"
#include "audio.h"
#include "filesys.h"
#include "inputrecord.h"
#include "disk.h"
#include "devices.h"
#include "fsdb.h"
#include "gfxboard.h"

int savestate_state = 0;
static int savestate_first_capture;

static bool new_blitter = false;

static int replaycounter;

struct zfile *savestate_file;

#define SAVESTATE_DOCOMPRESS 1
#define SAVESTATE_NODIALOGS 2
#define SAVESTATE_SPECIALDUMP1 4
#define SAVESTATE_SPECIALDUMP2 8
#define SAVESTATE_ALWAYSUSEPATH 16
#define SAVESTATE_SPECIALDUMP (SAVESTATE_SPECIALDUMP1 | SAVESTATE_SPECIALDUMP2)
static int savestate_flags;

TCHAR savestate_fname[MAX_DPATH];
TCHAR path_statefile[MAX_DPATH];

#define STATEFILE_ALLOC_SIZE 600000
static int statefile_alloc;
static int staterecords_max = 1000;
static int staterecords_first = 0;
static struct zfile *staterecord_statefile;
struct staterecord
{
	int len;
	int inuse;
	uae_u8 *cpu;
	uae_u8 *data;
	uae_u8 *end;
	int inprecoffset;
};

static struct staterecord **staterecords;

bool is_savestate_incompatible(void)
{
	int dowarn = 0;

#ifdef BSDSOCKET
	if (currprefs.socket_emu)
		dowarn = 1;
#endif
#ifdef UAESERIAL
	if (currprefs.uaeserial)
		dowarn = 1;
#endif
#ifdef SCSIEMU
	if (currprefs.scsi)
		dowarn = 1;
#endif
#ifdef CATWEASEL
	if (currprefs.catweasel)
		dowarn = 1;
#endif
#ifdef FILESYS
	for(int i = 0; i < currprefs.mountitems; i++) {
		struct mountedinfo mi;
		int type = get_filesys_unitconfig (&currprefs, i, &mi);
		if (mi.ismounted && type != FILESYS_VIRTUAL && type != FILESYS_HARDFILE && type != FILESYS_HARDFILE_RDB)
			dowarn = 1;
	}
	if (currprefs.rtgboards[0].rtgmem_type >= GFXBOARD_HARDWARE) {
		dowarn = 1;
	}
	if (currprefs.rtgboards[1].rtgmem_size > 0) {
		dowarn = 1;
	}
#endif
#ifdef WITH_PPC
	if (currprefs.ppc_model[0]) {
		dowarn = 1;
	}
#endif
	return dowarn != 0;
}

/* functions for reading/writing bytes, shorts and longs in big-endian
* format independent of host machine's endianness */

static uae_u8 *storepos;
void save_store_pos_func(uae_u8 **dstp)
{
	storepos = *dstp;
	*dstp += 4;
}
void save_store_size_func(uae_u8 **dstp)
{
	uae_u8 *p = storepos;
	save_u32t_func(&p, *dstp - storepos);
}
void restore_store_pos_func(uae_u8 **srcp)
{
	storepos = *srcp;
	*srcp += 4;
}
void restore_store_size_func(uae_u8 **srcp)
{
	uae_u8 *p = storepos;
	uae_u32 len = restore_u32_func (&p);
	*srcp = storepos + len;
}

void save_u32t_func(uae_u8** dstp, size_t vv)
{
	uae_u32 v = (uae_u32)vv;
	uae_u8* dst = *dstp;
	*dst++ = (uae_u8)(v >> 24);
	*dst++ = (uae_u8)(v >> 16);
	*dst++ = (uae_u8)(v >> 8);
	*dst++ = (uae_u8)(v >> 0);
	*dstp = dst;
}
void save_u32_func(uae_u8 **dstp, uae_u32 v)
{
	uae_u8 *dst = *dstp;
	*dst++ = (uae_u8)(v >> 24);
	*dst++ = (uae_u8)(v >> 16);
	*dst++ = (uae_u8)(v >> 8);
	*dst++ = (uae_u8)(v >> 0);
	*dstp = dst;
}
void save_u64_func(uae_u8 **dstp, uae_u64 v)
{
	save_u32_func (dstp, (uae_u32)(v >> 32));
	save_u32_func (dstp, (uae_u32)v);
}
void save_u16_func(uae_u8 **dstp, uae_u16 v)
{
	uae_u8 *dst = *dstp;
	*dst++ = (uae_u8)(v >> 8);
	*dst++ = (uae_u8)(v >> 0);
	*dstp = dst;
}
void save_u8_func(uae_u8 **dstp, uae_u8 v)
{
	uae_u8 *dst = *dstp;
	*dst++ = v;
	*dstp = dst;
}
void save_string_func (uae_u8 **dstp, const TCHAR *from)
{
	uae_u8 *dst = *dstp;
	char *s, *s2;
	s2 = s = uutf8 (from);
	while (s && *s)
		*dst++ = *s++;
	*dst++ = 0;
	*dstp = dst;
	xfree (s2);
}
void save_path_func (uae_u8 **dstp, const TCHAR *from, int type)
{
	save_string_func (dstp, from);
}
void save_path_full_func(uae_u8 **dstp, const TCHAR *spath, int type)
{
	TCHAR path[MAX_DPATH];
	save_u32_func(dstp, type);
	_tcscpy(path, spath ? spath : _T(""));
	fullpath(path, MAX_DPATH, false);
	save_string_func(dstp, path);
	_tcscpy(path, spath ? spath : _T(""));
	fullpath(path, MAX_DPATH, true);
	save_string_func(dstp, path);
}

uae_u32 restore_u32_func (uae_u8 **dstp)
{
	uae_u32 v;
	uae_u8 *dst = *dstp;
	v = (dst[0] << 24) | (dst[1] << 16) | (dst[2] << 8) | (dst[3]);
	*dstp = dst + 4;
	return v;
}
uae_u64 restore_u64_func (uae_u8 **dstp)
{
	uae_u64 v;

	v = restore_u32_func (dstp);
	v <<= 32;
	v |= restore_u32_func (dstp);
	return v;
}
uae_u16 restore_u16_func (uae_u8 **dstp)
{
	uae_u16 v;
	uae_u8 *dst = *dstp;
	v=(dst[0] << 8) | (dst[1]);
	*dstp = dst + 2;
	return v;
}
uae_u8 restore_u8_func (uae_u8 **dstp)
{
	uae_u8 v;
	uae_u8 *dst = *dstp;
	v = dst[0];
	*dstp = dst + 1;
	return v;
}
TCHAR *restore_string_func (uae_u8 **dstp)
{
	int len;
	uae_u8 v;
	uae_u8 *dst = *dstp;
	char *top, *to;
	TCHAR *s;

	len = uaestrlen((char*)dst) + 1;
	top = to = xmalloc (char, len);
	do {
		v = *dst++;
		*top++ = v;
	} while (v);
	*dstp = dst;
	s = utf8u (to);
	xfree (to);
	return s;
}

static bool state_path_exists(const TCHAR *path, int type)
{
	if (type == SAVESTATE_PATH_VDIR)
		return my_existsdir(path);
	return my_existsfile2(path);
}

static TCHAR *state_resolve_path(TCHAR *s, int type, bool newmode)
{
	TCHAR *newpath;
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];

	if (s[0] == 0)
		return s;
	if (!newmode && state_path_exists(s, type))
		return s;
	if (type == SAVESTATE_PATH_HD)
		return s;
	if (newmode) {
		_tcscpy(tmp, s);
		fullpath(tmp, sizeof(tmp) / sizeof(TCHAR));
		if (state_path_exists(tmp, type)) {
			xfree(s);
			return my_strdup(tmp);
		}
		getfilepart(tmp, sizeof tmp / sizeof(TCHAR), s);
	} else {
		getfilepart(tmp, sizeof tmp / sizeof(TCHAR), s);
		if (state_path_exists(tmp, type)) {
			xfree(s);
			return my_strdup(tmp);
		}
	}
	for (int i = 0; i < MAX_PATHS; i++) {
		newpath = NULL;
		if (type == SAVESTATE_PATH_FLOPPY)
			newpath = currprefs.path_floppy.path[i];
		else if (type == SAVESTATE_PATH_VDIR || type == SAVESTATE_PATH_HDF)
			newpath = currprefs.path_hardfile.path[i];
		else if (type == SAVESTATE_PATH_CD)
			newpath = currprefs.path_cd.path[i];
		if (newpath == NULL || newpath[0] == 0)
			break;
		_tcscpy (tmp2, newpath);
		fix_trailing (tmp2);
		_tcscat (tmp2, tmp);
		fullpath (tmp2, sizeof tmp2 / sizeof (TCHAR));
		if (state_path_exists(tmp2, type)) {
			xfree (s);
			return my_strdup (tmp2);
		}
	}
	getpathpart (tmp2, sizeof tmp2 / sizeof (TCHAR), savestate_fname);
	_tcscat (tmp2, tmp);
	if (state_path_exists(tmp2, type)) {
		xfree (s);
		return my_strdup (tmp2);
	}
	return s;
}

TCHAR *restore_path_func(uae_u8 **dstp, int type)
{
	TCHAR *s = restore_string_func(dstp);
	return state_resolve_path(s, type, false);
}

TCHAR *restore_path_full_func(uae_u8 **dstp)
{
	int type = restore_u32_func(dstp);
	TCHAR *a = restore_string_func(dstp);
	TCHAR *r = restore_string_func(dstp);
	if (target_isrelativemode()) {
		xfree(a);
		return state_resolve_path(r, type, true);
	} else {
		TCHAR tmp[MAX_DPATH];
		_tcscpy(tmp, a);
		fullpath(tmp, sizeof(tmp) / sizeof(TCHAR));
		if (state_path_exists(tmp, type)) {
			xfree(r);
			xfree(a);
			return my_strdup(tmp);
		}
		_tcscpy(tmp, r);
		fullpath(tmp, sizeof(tmp) / sizeof(TCHAR));
		if (state_path_exists(tmp, type)) {
			xfree(r);
			xfree(a);
			return my_strdup(tmp);
		}
		xfree(r);
		return state_resolve_path(a, type, true);
	}
	return NULL;
}


/* read and write IFF-style hunks */

static void save_chunk (struct zfile *f, uae_u8 *chunk, size_t len, const TCHAR *name, int compress)
{
	uae_u8 tmp[8], *dst;
	uae_u32 flags;
	size_t pos;
	size_t chunklen, len2;
	char *s;

	if (!chunk)
		return;

	if (compress < 0) {
		zfile_fwrite (chunk, 1, len, f);
		return;
	}

	/* chunk name */
	s = ua (name);
	zfile_fwrite (s, 1, 4, f);
	xfree (s);
	pos = zfile_ftell32(f);
	/* chunk size */
	dst = &tmp[0];
	chunklen = len + 4 + 4 + 4;
	save_u32t(chunklen);
	zfile_fwrite (&tmp[0], 1, 4, f);
	/* chunk flags */
	flags = 0;
	dst = &tmp[0];
	save_u32(flags | compress);
	zfile_fwrite (&tmp[0], 1, 4, f);
	/* chunk data */
	if (compress) {
		size_t tmplen = len;
		size_t opos;
		dst = &tmp[0];
		save_u32t(len);
		opos = zfile_ftell32(f);
		zfile_fwrite(&tmp[0], 1, 4, f);
		len = zfile_zcompress(f, chunk, len);
		if (len > 0) {
			zfile_fseek (f, pos, SEEK_SET);
			dst = &tmp[0];
			save_u32t(len + 4 + 4 + 4 + 4);
			zfile_fwrite (&tmp[0], 1, 4, f);
			zfile_fseek (f, 0, SEEK_END);
		} else {
			len = tmplen;
			compress = 0;
			zfile_fseek (f, opos, SEEK_SET);
			dst = &tmp[0];
			save_u32 (flags);
			zfile_fwrite (&tmp[0], 1, 4, f);
		}
	}
	if (!compress)
		zfile_fwrite (chunk, 1, len, f);
	/* alignment */
	len2 = 4 - (len & 3);
	if (len2) {
		uae_u8 zero[4] = { 0, 0, 0, 0 };
		zfile_fwrite(zero, 1, len2, f);
	}

	write_log (_T("Chunk '%s' chunk size %u (%u)\n"), name, chunklen, len);
}

static uae_u8 *restore_chunk (struct zfile *f, TCHAR *name, unsigned int *len, unsigned int *totallen, size_t *filepos)
{
	uae_u8 tmp[6], dummy[4], *mem, *src;
	uae_u32 flags;
	int len2;

	*totallen = 0;
	*filepos = 0;
	*name = 0;
	/* chunk name */
	if (zfile_fread(tmp, 1, 4, f) != 4)
		return NULL;
	tmp[4] = 0;
	au_copy (name, 5, (char*)tmp);
	/* chunk size */
	if (zfile_fread(tmp, 1, 4, f) != 4) {
		*name = 0;
		return NULL;
	}
	src = tmp;
	len2 = restore_u32 () - 4 - 4 - 4;
	if (len2 < 0)
		len2 = 0;
	*len = len2;
	if (len2 == 0) {
		*filepos = zfile_ftell32(f);
		return 0;
	}

	/* chunk flags */
	if (zfile_fread(tmp, 1, 4, f) != 4) {
		*name = 0;
		return NULL;
	}
	src = tmp;
	flags = restore_u32 ();
	*totallen = *len;
	if (flags & 1) {
		zfile_fread (tmp, 1, 4, f);
		src = tmp;
		*totallen = restore_u32();
		*filepos = zfile_ftell32(f) - 4 - 4 - 4;
		len2 -= 4;
	} else {
		*filepos = zfile_ftell32(f) - 4 - 4;
	}
	/* chunk data.  RAM contents will be loaded during the reset phase,
	   no need to malloc multiple megabytes here.  */
	if (_tcscmp (name, _T("CRAM")) != 0
		&& _tcscmp(name, _T("BRAM")) != 0
		&& _tcscmp(name, _T("FRAM")) != 0
		&& _tcscmp(name, _T("ZRAM")) != 0
		&& _tcscmp(name, _T("FRA2")) != 0
		&& _tcscmp(name, _T("ZRA2")) != 0
		&& _tcscmp(name, _T("FRA3")) != 0
		&& _tcscmp(name, _T("ZRA3")) != 0
		&& _tcscmp(name, _T("FRA4")) != 0
		&& _tcscmp(name, _T("ZRA4")) != 0
		&& _tcscmp(name, _T("ZCRM")) != 0
		&& _tcscmp(name, _T("PRAM")) != 0
		&& _tcscmp(name, _T("A3K1")) != 0
		&& _tcscmp(name, _T("A3K2")) != 0
		&& _tcscmp(name, _T("BORO")) != 0
	)
	{
		/* extra bytes at the end needed to handle old statefiles that now have new fields */
		mem = xcalloc (uae_u8, *totallen + 100);
		if (!mem)
			return NULL;
		if (flags & 1) {
			zfile_zuncompress (mem, *totallen, f, len2);
		} else {
			zfile_fread (mem, 1, len2, f);
		}
	} else {
		mem = 0;
		zfile_fseek (f, len2, SEEK_CUR);
	}

	/* alignment */
	len2 = 4 - (len2 & 3);
	if (len2)
		zfile_fread (dummy, 1, len2, f);
	return mem;
}

void restore_ram (size_t filepos, uae_u8 *memory)
{
	uae_u8 tmp[8];
	uae_u8 *src = tmp;
	int size, fullsize;
	uae_u32 flags;

	if (filepos == 0 || memory == NULL)
		return;
	zfile_fseek (savestate_file, filepos, SEEK_SET);
	zfile_fread (tmp, 1, sizeof tmp, savestate_file);
	size = restore_u32 ();
	flags = restore_u32 ();
	size -= 4 + 4 + 4;
	if (flags & 1) {
		zfile_fread (tmp, 1, 4, savestate_file);
		src = tmp;
		fullsize = restore_u32 ();
		size -= 4;
		zfile_zuncompress (memory, fullsize, savestate_file, size);
	} else {
		zfile_fread (memory, 1, size, savestate_file);
	}
}

static uae_u8 *restore_log (uae_u8 *src)
{
#if OPEN_LOG > 0
	TCHAR *s = utf8u (src);
	write_log (_T("%s\n"), s);
	xfree (s);
#endif
	src += strlen ((char*)src) + 1;
	return src;
}

static void restore_header (uae_u8 *src)
{
	TCHAR *emuname, *emuversion, *description;

	restore_u32 ();
	emuname = restore_string ();
	emuversion = restore_string ();
	description = restore_string ();
	write_log (_T("Saved with: '%s %s', description: '%s'\n"),
		emuname, emuversion, description);
	xfree (description);
	xfree (emuversion);
	xfree (emuname);
}

/* restore all subsystems */

void restore_state (const TCHAR *filename)
{
	struct zfile *f;
	uae_u8 *chunk,*end;
	TCHAR name[5];
	unsigned int len, totallen;
	size_t filepos, filesize;
	int z3num, z2num;
	bool end_found = false;

	chunk = 0;
	f = zfile_fopen (filename, _T("rb"), ZFD_NORMAL);
	if (!f)
		goto error;
	zfile_fseek (f, 0, SEEK_END);
	filesize = zfile_ftell32(f);
	zfile_fseek (f, 0, SEEK_SET);
	savestate_state = STATE_RESTORE;
	savestate_init ();

	chunk = restore_chunk (f, name, &len, &totallen, &filepos);
	if (!chunk || _tcsncmp (name, _T("ASF "), 4)) {
		write_log (_T("%s is not an AmigaStateFile\n"), filename);
		goto error;
	}
	write_log (_T("STATERESTORE: '%s'\n"), filename);
	set_config_changed ();
	savestate_file = f;
	restore_header (chunk);
	xfree (chunk);
	devices_restore_start();
	clear_events();
	z2num = z3num = 0;
	for (;;) {
		name[0] = 0;
		chunk = end = restore_chunk (f, name, &len, &totallen, &filepos);
		write_log (_T("Chunk '%s' size %u (%u)\n"), name, len, totallen);
		if (!_tcscmp (name, _T("END "))) {
			if (end_found)
				break;
			end_found = true;
			continue;
		}
		if (!_tcscmp (name, _T("CRAM"))) {
			restore_cram (totallen, filepos);
			continue;
		} else if (!_tcscmp (name, _T("BRAM"))) {
			restore_bram (totallen, filepos);
			continue;
		} else if (!_tcscmp (name, _T("A3K1"))) {
			restore_a3000lram (totallen, filepos);
			continue;
		} else if (!_tcscmp (name, _T("A3K2"))) {
			restore_a3000hram (totallen, filepos);
			continue;
#ifdef AUTOCONFIG
		} else if (!_tcscmp (name, _T("FRAM"))) {
			restore_fram (totallen, filepos, z2num++);
			continue;
		} else if (!_tcscmp (name, _T("ZRAM"))) {
			restore_zram (totallen, filepos, z3num++);
			continue;
		} else if (!_tcscmp (name, _T("ZCRM"))) {
			restore_zram (totallen, filepos, -1);
			continue;
		} else if (!_tcscmp (name, _T("BORO"))) {
			restore_bootrom (totallen, filepos);
			continue;
#endif
#ifdef PICASSO96
		} else if (!_tcscmp (name, _T("PRAM"))) {
			restore_pram (totallen, filepos);
			continue;
#endif
		} else if (!_tcscmp (name, _T("CYCS"))) {
			end = restore_cycles (chunk);
		} else if (!_tcscmp (name, _T("CPU "))) {
			end = restore_cpu (chunk);
		} else if (!_tcscmp (name, _T("CPUX")))
			end = restore_cpu_extra (chunk);
		else if (!_tcscmp (name, _T("CPUT")))
			end = restore_cpu_trace (chunk);
#ifdef FPUEMU
		else if (!_tcscmp (name, _T("FPU ")))
			end = restore_fpu (chunk);
#endif
#ifdef MMUEMU
		else if (!_tcscmp (name, _T("MMU ")))
			end = restore_mmu (chunk);
#endif
		else if (!_tcscmp (name, _T("AGAC")))
			end = restore_custom_agacolors (chunk);
		else if (!_tcscmp (name, _T("SPR0")))
			end = restore_custom_sprite (0, chunk);
		else if (!_tcscmp (name, _T("SPR1")))
			end = restore_custom_sprite (1, chunk);
		else if (!_tcscmp (name, _T("SPR2")))
			end = restore_custom_sprite (2, chunk);
		else if (!_tcscmp (name, _T("SPR3")))
			end = restore_custom_sprite (3, chunk);
		else if (!_tcscmp (name, _T("SPR4")))
			end = restore_custom_sprite (4, chunk);
		else if (!_tcscmp (name, _T("SPR5")))
			end = restore_custom_sprite (5, chunk);
		else if (!_tcscmp (name, _T("SPR6")))
			end = restore_custom_sprite (6, chunk);
		else if (!_tcscmp(name, _T("SPR7")))
			end = restore_custom_sprite(7, chunk);
		else if (!_tcscmp(name, _T("BPLX")))
			end = restore_custom_bpl(chunk);
		else if (!_tcscmp (name, _T("CIAA")))
			end = restore_cia (0, chunk);
		else if (!_tcscmp (name, _T("CIAB")))
			end = restore_cia (1, chunk);
		else if (!_tcscmp (name, _T("CHIP")))
			end = restore_custom (chunk);
		else if (!_tcscmp (name, _T("CINP")))
			end = restore_input (chunk);
		else if (!_tcscmp (name, _T("CHPX")))
			end = restore_custom_extra (chunk);
		else if (!_tcscmp (name, _T("CHPD")))
			end = restore_custom_event_delay (chunk);
		else if (!_tcscmp (name, _T("CHSL")))
			end = restore_custom_slots (chunk);
		else if (!_tcscmp (name, _T("AUD0")))
			end = restore_audio (0, chunk);
		else if (!_tcscmp (name, _T("AUD1")))
			end = restore_audio (1, chunk);
		else if (!_tcscmp (name, _T("AUD2")))
			end = restore_audio (2, chunk);
		else if (!_tcscmp (name, _T("AUD3")))
			end = restore_audio (3, chunk);
		else if (!_tcscmp (name, _T("BLIT")))
			end = restore_blitter (chunk);
		else if (!_tcscmp (name, _T("BLTX")))
			end = restore_blitter_new (chunk);
		else if (!_tcscmp (name, _T("DISK")))
			end = restore_floppy (chunk);
		else if (!_tcscmp (name, _T("DSK0")))
			end = restore_disk (0, chunk);
		else if (!_tcscmp (name, _T("DSK1")))
			end = restore_disk (1, chunk);
		else if (!_tcscmp (name, _T("DSK2")))
			end = restore_disk (2, chunk);
		else if (!_tcscmp (name, _T("DSK3")))
			end = restore_disk (3, chunk);
		else if (!_tcscmp (name, _T("DSD0")))
			end = restore_disk2 (0, chunk);
		else if (!_tcscmp (name, _T("DSD1")))
			end = restore_disk2 (1, chunk);
		else if (!_tcscmp (name, _T("DSD2")))
			end = restore_disk2 (2, chunk);
		else if (!_tcscmp (name, _T("DSD3")))
			end = restore_disk2 (3, chunk);
		else if (!_tcscmp (name, _T("KEYB")))
			end = restore_keyboard (chunk);
		else if (!_tcscmp (name, _T("KBM1")))
			end = restore_kbmcu(chunk);
		else if (!_tcscmp (name, _T("KBM2")))
			end = restore_kbmcu2(chunk);
		else if (!_tcscmp (name, _T("KBM3")))
			end = restore_kbmcu3(chunk);
#ifdef AUTOCONFIG
		else if (!_tcscmp (name, _T("EXPA")))
			end = restore_expansion (chunk);
#endif
		else if (!_tcscmp (name, _T("ROM ")))
			end = restore_rom (chunk);
#ifdef PICASSO96
		else if (!_tcscmp (name, _T("P96 ")))
			end = restore_p96 (chunk);
#endif
#ifdef ACTION_REPLAY
		else if (!_tcscmp (name, _T("ACTR")))
			end = restore_action_replay (chunk);
		else if (!_tcscmp (name, _T("HRTM")))
			end = restore_hrtmon (chunk);
#endif
#ifdef FILESYS
		else if (!_tcscmp(name, _T("FSYP")))
			end = restore_filesys_paths(chunk);
		else if (!_tcscmp(name, _T("FSYS")))
			end = restore_filesys(chunk);
		else if (!_tcscmp(name, _T("FSYC")))
			end = restore_filesys_common(chunk);
#endif
#ifdef CD32
		else if (!_tcscmp (name, _T("CD32")))
			end = restore_akiko (chunk);
#endif
#ifdef CDTV
		else if (!_tcscmp (name, _T("CDTV")))
			end = restore_cdtv (chunk);
		else if (!_tcscmp (name, _T("DMAC")))
			end = restore_cdtv_dmac (chunk);
#endif
#if 0
		else if (!_tcscmp (name, _T("DMC2")))
			end = restore_scsi_dmac (WDTYPE_A3000, chunk);
		else if (!_tcscmp (name, _T("DMC3")))
			end = restore_scsi_dmac (WDTYPE_A2091, chunk);
		else if (!_tcscmp (name, _T("DMC3")))
			end = restore_scsi_dmac (WDTYPE_A2091_2, chunk);
		else if (!_tcscmp (name, _T("SCS2")))
			end = restore_scsi_device (WDTYPE_A3000, chunk);
		else if (!_tcscmp (name, _T("SCS3")))
			end = restore_scsi_device (WDTYPE_A2091, chunk);
		else if (!_tcscmp (name, _T("SCS4")))
			end = restore_scsi_device (WDTYPE_A2091_2, chunk);
#endif
		else if (!_tcscmp (name, _T("SCSD")))
			end = restore_scsidev (chunk);
		else if (!_tcscmp (name, _T("GAYL")))
			end = restore_gayle (chunk);
		else if (!_tcscmp (name, _T("IDE ")))
			end = restore_gayle_ide (chunk);
		else if (!_tcsncmp (name, _T("CDU"), 3))
			end = restore_cd (name[3] - '0', chunk);
		else if (!_tcsncmp(name, _T("ALG "), 4))
			end = restore_alg(chunk);
#ifdef A2065
		else if (!_tcsncmp (name, _T("2065"), 4))
			end = restore_a2065 (chunk);
#endif
#if 0
		else if (!_tcsncmp(name, _T("EXPI"), 4))
			end = restore_expansion_info_old(chunk);
#endif
		else if (!_tcsncmp (name, _T("EXPB"), 4))
			end = restore_expansion_boards(chunk);
#ifdef DEBUGGER
		else if (!_tcsncmp (name, _T("DMWP"), 4))
			end = restore_debug_memwatch (chunk);
#endif
		else if (!_tcsncmp(name, _T("PIC0"), 4))
			end = chunk + len;

		else if (!_tcscmp (name, _T("CONF")))
			end = restore_configuration (chunk);
		else if (!_tcscmp (name, _T("LOG ")))
			end = restore_log (chunk);
		else {
			end = chunk + len;
			write_log (_T("unknown chunk '%s' size %d bytes\n"), name, len);
		}
		if (end == NULL)
			write_log (_T("Chunk '%s', size %d bytes was not accepted!\n"),
			name, len);
		else if (totallen != end - chunk)
			write_log (_T("Chunk '%s' total size %d bytes but read %ld bytes!\n"),
			name, totallen, end - chunk);
		xfree (chunk);
		if (name[0] == 0)
			break;
	}
	target_addtorecent (filename, 0);
	DISK_history_add(filename, -1, HISTORY_STATEFILE, 0);
	return;

error:
	savestate_state = 0;
	savestate_file = 0;
	if (chunk)
		xfree (chunk);
	if (f)
		zfile_fclose (f);
}

void savestate_restore_final(void)
{
	restore_akiko_final();
	restore_cdtv_final();
}

bool savestate_restore_finish(void)
{
	if (!isrestore())
		return false;
	zfile_fclose(savestate_file);
	savestate_file = 0;
	restore_cpu_finish();
	restore_audio_finish();
	restore_disk_finish();
	restore_blitter_finish();
	restore_expansion_finish();
	restore_akiko_finish();
	restore_custom_finish();
#ifdef CDTV
	restore_cdtv_finish();
#endif
#ifdef PICASSO96
	restore_p96_finish();
#endif
#ifdef A2065
	restore_a2065_finish();
#endif
	restore_cia_finish();
#ifdef ACTION_REPLAY
	restore_ar_finish();
#endif
#ifdef DEBUGGER
	restore_debug_memwatch_finish();
#endif
	savestate_state = 0;
	init_hz();
	audio_activate();
	return true;
}

/* 1=compressed,2=not compressed,3=ram dump,4=audio dump */
void savestate_initsave (const TCHAR *filename, int mode, int nodialogs, bool save)
{
	savestate_flags = 0;
	if (filename == NULL) {
		savestate_fname[0] = 0;
		return;
	}
	_tcscpy (savestate_fname, filename);
	savestate_flags |= (mode == 1) ? SAVESTATE_DOCOMPRESS : 0;
	savestate_flags |= (mode == 3) ? SAVESTATE_SPECIALDUMP1 : (mode == 4) ? SAVESTATE_SPECIALDUMP2 : 0;
	savestate_flags |= nodialogs ? SAVESTATE_NODIALOGS : 0;
	new_blitter = false;
	if (save) {
		savestate_free ();
		inprec_close (true);
	}
}

static void save_rams (struct zfile *f, int comp)
{
	uae_u8 *dst;
	size_t len;

	dst = save_cram (&len);
	save_chunk (f, dst, len, _T("CRAM"), comp);
	dst = save_bram (&len);
	save_chunk (f, dst, len, _T("BRAM"), comp);
	dst = save_a3000lram (&len);
	save_chunk (f, dst, len, _T("A3K1"), comp);
	dst = save_a3000hram (&len);
	save_chunk (f, dst, len, _T("A3K2"), comp);
#ifdef AUTOCONFIG
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		dst = save_fram(&len, i);
		save_chunk(f, dst, len, _T("FRAM"), comp);
	}
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		dst = save_zram(&len, i);
		save_chunk(f, dst, len, _T("ZRAM"), comp);
	}
	dst = save_zram (&len, -1);
	save_chunk (f, dst, len, _T("ZCRM"), comp);
	dst = save_bootrom (&len);
	save_chunk (f, dst, len, _T("BORO"), comp);
#endif
#ifdef PICASSO96
	dst = save_pram (&len);
	save_chunk (f, dst, len, _T("PRAM"), comp);
#endif
}

/* Save all subsystems */

static int save_state_internal (struct zfile *f, const TCHAR *description, int comp, bool savepath)
{
	uae_u8 endhunk[] = { 'E', 'N', 'D', ' ', 0, 0, 0, 8 };
	uae_u8 header[1000];
	TCHAR tmp[100];
	uae_u8 *dst;
	TCHAR name[5];
	int i;
	size_t len;

	write_log (_T("STATESAVE (%s):\n"), f ? zfile_getname (f) : _T("<internal>"));
	dst = header;
	save_u32 (0);
	save_string (_T("UAE"));
	_sntprintf (tmp, sizeof tmp, _T("%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
	save_string (tmp);
	save_string (description);
	save_chunk (f, header, dst-header, _T("ASF "), 0);

	dst = save_cycles (&len, 0);
	save_chunk (f, dst, len, _T("CYCS"), 0);
	xfree (dst);

	dst = save_cpu (&len, 0);
	save_chunk (f, dst, len, _T("CPU "), 0);
	xfree (dst);

	dst = save_cpu_extra (&len, 0);
	save_chunk (f, dst, len, _T("CPUX"), 0);
	xfree (dst);

	dst = save_cpu_trace (&len, 0);
	save_chunk (f, dst, len, _T("CPUT"), 0);
	xfree (dst);

#ifdef FPUEMU
	dst = save_fpu (&len,0 );
	save_chunk (f, dst, len, _T("FPU "), 0);
	xfree (dst);
#endif

#ifdef MMUEMU
	dst = save_mmu (&len, 0);
	save_chunk (f, dst, len, _T("MMU "), 0);
	xfree (dst);
#endif

	_tcscpy(name, _T("DSKx"));
	for (i = 0; i < 4; i++) {
		dst = save_disk (i, &len, 0, savepath);
		if (dst) {
			name[3] = i + '0';
			save_chunk (f, dst, len, name, 0);
			xfree (dst);
		}
	}
	_tcscpy(name, _T("DSDx"));
	for (i = 0; i < 4; i++) {
		dst = save_disk2 (i, &len, 0);
		if (dst) {
			name[3] = i + '0';
			save_chunk (f, dst, len, name, comp);
			xfree (dst);
		}
	}


	dst = save_floppy (&len, 0);
	save_chunk (f, dst, len, _T("DISK"), 0);
	xfree (dst);

	dst = save_custom (&len, 0, 0);
	save_chunk (f, dst, len, _T("CHIP"), 0);
	xfree (dst);

	dst = save_custom_extra (&len, 0);
	save_chunk (f, dst, len, _T("CHPX"), 0);
	xfree (dst);

	dst = save_custom_event_delay (&len, 0);
	save_chunk (f, dst, len, _T("CHPD"), 0);
	xfree (dst);

	dst = save_blitter_new (&len, 0);
	save_chunk (f, dst, len, _T("BLTX"), 0);
	xfree (dst);
	dst = save_blitter (&len, 0, new_blitter);
	save_chunk (f, dst, len, _T("BLIT"), 0);
	xfree (dst);

	dst = save_custom_slots(&len, 0);
	save_chunk(f, dst, len, _T("CHSL"), 0);
	xfree(dst);

	dst = save_input (&len, 0);
	save_chunk (f, dst, len, _T("CINP"), 0);
	xfree (dst);

	dst = save_custom_agacolors(&len, 0);
	if (dst) {
		save_chunk(f, dst, len, _T("AGAC"), 0);
		xfree(dst);
	}

	dst = save_custom_bpl(&len, NULL);
	if (dst) {
		save_chunk(f, dst, len, _T("BPLX"), 0);
		xfree(dst);
	}
	_tcscpy (name, _T("SPRx"));
	for (i = 0; i < 8; i++) {
		dst = save_custom_sprite (i, &len, 0);
		name[3] = i + '0';
		save_chunk (f, dst, len, name, 0);
		xfree (dst);
	}

	_tcscpy (name, _T("AUDx"));
	for (i = 0; i < 4; i++) {
		dst = save_audio (i, &len, 0);
		name[3] = i + '0';
		save_chunk (f, dst, len, name, 0);
		xfree (dst);
	}

	dst = save_cia (0, &len, 0);
	save_chunk (f, dst, len, _T("CIAA"), 0);
	xfree (dst);

	dst = save_cia (1, &len, 0);
	save_chunk (f, dst, len, _T("CIAB"), 0);
	xfree (dst);

	dst = save_keyboard (&len, NULL);
	save_chunk (f, dst, len, _T("KEYB"), 0);
	xfree (dst);

	dst = save_kbmcu(&len, NULL);
	save_chunk(f, dst, len, _T("KBM1"), 0);
	xfree (dst);
	dst = save_kbmcu2(&len, NULL);
	save_chunk(f, dst, len, _T("KBM2"), 0);
	xfree (dst);
	dst = save_kbmcu3(&len, NULL);
	save_chunk(f, dst, len, _T("KBM3"), 0);
	xfree (dst);

#ifdef AUTOCONFIG
	// new
	i = 0;
	for (;;) {
		dst = save_expansion_boards(&len, 0, i);
		if (!dst)
			break;
		save_chunk(f, dst, len, _T("EXPB"), 0);
		i++;
	}
#if 0
	// old
	dst = save_expansion_info_old(&len, 0);
	save_chunk(f, dst, len, _T("EXPI"), 0);
#endif
	dst = save_expansion(&len, 0);
	save_chunk(f, dst, len, _T("EXPA"), 0);
#endif
#ifdef A2065
	dst = save_a2065 (&len, NULL);
	save_chunk (f, dst, len, _T("2065"), 0);
#endif
#ifdef PICASSO96
	dst = save_p96 (&len, 0);
	save_chunk (f, dst, len, _T("P96 "), 0);
#endif
	save_rams (f, comp);

	dst = save_rom (1, &len, 0);
	do {
		if (!dst)
			break;
		save_chunk (f, dst, len, _T("ROM "), 0);
		xfree (dst);
	} while ((dst = save_rom (0, &len, 0)));

#ifdef CD32
	dst = save_akiko (&len, NULL);
	save_chunk (f, dst, len, _T("CD32"), 0);
	xfree (dst);
#endif
#ifdef ARCADIA
	dst = save_alg(&len);
	save_chunk(f, dst, len, _T("ALG "), 0);
	xfree(dst);
#endif
#ifdef CDTV
	dst = save_cdtv (&len, NULL);
	save_chunk (f, dst, len, _T("CDTV"), 0);
	xfree (dst);
	dst = save_cdtv_dmac (&len, NULL);
	save_chunk (f, dst, len, _T("DMAC"), 0);
	xfree (dst);
#endif
#if 0
	dst = save_scsi_dmac (WDTYPE_A3000, &len, NULL);
	save_chunk (f, dst, len, _T("DMC2"), 0);
	xfree (dst);
	for (i = 0; i < 8; i++) {
		dst = save_scsi_device (WDTYPE_A3000, i, &len, NULL);
		save_chunk (f, dst, len, _T("SCS2"), 0);
		xfree (dst);
	}
	for (int ii = 0; ii < 2; ii++) {
		dst = save_scsi_dmac (ii == 0 ? WDTYPE_A2091 : WDTYPE_A2091_2, &len, NULL);
		save_chunk (f, dst, len, ii == 0 ? _T("DMC3") : _T("DMC4"), 0);
		xfree (dst);
		for (i = 0; i < 8; i++) {
			dst = save_scsi_device (ii == 0 ? WDTYPE_A2091 : WDTYPE_A2091_2, i, &len, NULL);
			save_chunk (f, dst, len, ii == 0 ? _T("SCS3") : _T("SCS4"), 0);
			xfree (dst);
		}
	}
#endif
	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		dst = save_scsidev (i, &len, NULL);
		save_chunk (f, dst, len, _T("SCSD"), 0);
		xfree (dst);
	}
#ifdef ACTION_REPLAY
	dst = save_action_replay (&len, NULL);
	save_chunk (f, dst, len, _T("ACTR"), comp);
	dst = save_hrtmon (&len, NULL);
	save_chunk (f, dst, len, _T("HRTM"), comp);
#endif
#ifdef FILESYS
	dst = save_filesys_common (&len);
	if (dst) {
		save_chunk (f, dst, len, _T("FSYC"), 0);
		for (i = 0; i < nr_units (); i++) {
			dst = save_filesys_paths(i, &len);
			if (dst) {
				save_chunk(f, dst, len, _T("FSYP"), 0);
				xfree(dst);
			}
			dst = save_filesys(i, &len);
			if (dst) {
				save_chunk(f, dst, len, _T("FSYS"), 0);
				xfree(dst);
			}
		}
	}
#endif
	dst = save_gayle (&len, NULL);
	if (dst) {
		save_chunk (f, dst, len, _T("GAYL"), 0);
		xfree(dst);
	}
	for (i = 0; i < 4; i++) {
		dst = save_gayle_ide (i, &len, NULL);
		if (dst) {
			save_chunk (f, dst, len, _T("IDE "), 0);
			xfree (dst);
		}
	}

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		dst = save_cd (i, &len);
		if (dst) {
			_sntprintf (name, sizeof name, _T("CDU%d"), i);
			save_chunk (f, dst, len, name, 0);
		}
	}

#ifdef DEBUGGER
	dst = save_debug_memwatch (&len, NULL);
	if (dst) {
		save_chunk (f, dst, len, _T("DMWP"), 0);
		xfree(dst);
	}
#endif

#ifndef AMIBERRY // this doesn't do anything in WinUAE either
	dst = save_screenshot(0, &len);
	if (dst) {
		save_chunk(f, dst, len, _T("PIC0"), 0);
		xfree(dst);
	}
#endif

	/* add fake END tag, makes it easy to strip CONF and LOG hunks */
	/* move this if you want to use CONF or LOG hunks when restoring state */
	zfile_fwrite (endhunk, 1, 8, f);

	dst = save_configuration (&len, false);
	if (dst) {
		save_chunk (f, dst, len, _T("CONF"), comp);
		xfree(dst);
	}
	len = 30000;
	dst = save_log (TRUE, &len);
	if (dst) {
		save_chunk (f, dst, len, _T("LOG "), comp);
		xfree (dst);
	}

	zfile_fwrite (endhunk, 1, 8, f);

	return 1;
}

int save_state (const TCHAR *filename, const TCHAR *description)
{
	struct zfile *f;
	int comp = (savestate_flags & SAVESTATE_DOCOMPRESS) != 0;

	if (!(savestate_flags & SAVESTATE_SPECIALDUMP) && !(savestate_flags & SAVESTATE_NODIALOGS)) {
		if (is_savestate_incompatible()) {
			static int warned;
			if (!warned) {
				warned = 1;
				notify_user(NUMSG_STATEHD);
			}
		}
		if (!save_filesys_cando ()) {
			gui_message (_T("Filesystem active. Try again later."));
			return -1;
		}
	}
	new_blitter = false;
	savestate_flags &= ~SAVESTATE_NODIALOGS;
	custom_prepare_savestate();
	f = zfile_fopen (filename, _T("w+b"), 0);
	if (!f)
		return 0;
	if (savestate_flags & SAVESTATE_SPECIALDUMP) {
		size_t pos;
		if (savestate_flags & SAVESTATE_SPECIALDUMP2) {
			write_wavheader (f, 0, 22050);
		}
		pos = zfile_ftell32(f);
		save_rams (f, -1);
		if (savestate_flags & SAVESTATE_SPECIALDUMP2) {
			size_t len, len2, i;
			uae_u8 *tmp;
			len = zfile_ftell32(f) - pos;
			tmp = xmalloc (uae_u8, len);
			zfile_fseek(f, pos, SEEK_SET);
			len2 = zfile_fread (tmp, 1, len, f);
			for (i = 0; i < len2; i++)
				tmp[i] += 0x80;
			write_wavheader (f, len, 22050);
			zfile_fwrite (tmp, len2, 1, f);
			xfree (tmp);
		}
		zfile_fclose (f);
		return 1;
	}
	int v = save_state_internal (f, description, comp, true);
	if (v)
		write_log (_T("Save of '%s' complete\n"), filename);
	zfile_fclose (f);
	DISK_history_add(filename, -1, HISTORY_STATEFILE, 0);
	savestate_state = 0;
	return v;
}

void savestate_quick(int slot, int save)
{
	if (path_statefile[0]) {
		_tcscpy(savestate_fname, path_statefile);
	}
	int i, len = uaetcslen(savestate_fname);
	i = len - 1;
	while (i >= 0 && savestate_fname[i] != '_') {
		i--;
	}
	if (i < len - 6 || i <= 0) { /* "_?.uss" */
		i = len - 1;
		while (i >= 0 && savestate_fname[i] != '.') {
			i--;
		}
		if (i <= 0) {
			write_log (_T("savestate name skipped '%s'\n"), savestate_fname);
			return;
		}
	}
	_tcscpy (savestate_fname + i, _T(".uss"));
	if (slot > 0) {
		_sntprintf (savestate_fname + i, sizeof savestate_fname + i, _T("_%d.uss"), slot);
	}
	savestate_flags = 0;
	if (save) {
		write_log (_T("saving '%s'\n"), savestate_fname);
		savestate_flags |= SAVESTATE_DOCOMPRESS;
		savestate_flags |= SAVESTATE_NODIALOGS;
		savestate_flags |= SAVESTATE_ALWAYSUSEPATH;
		save_state (savestate_fname, _T(""));
#ifdef AMIBERRY
		if (create_screenshot())
			save_thumb(screenshot_filename);
#endif
	} else {
		if (!zfile_exists (savestate_fname)) {
			write_log (_T("staterestore, file '%s' not found\n"), savestate_fname);
			return;
		}
		savestate_state = STATE_DORESTORE;
		savestate_flags |= SAVESTATE_ALWAYSUSEPATH;
		write_log (_T("staterestore starting '%s'\n"), savestate_fname);
	}
}

bool savestate_check(void)
{
	if (vpos == 0 && !savestate_state) {
		if (hsync_counter == 0 && input_play == INPREC_PLAY_NORMAL)
			savestate_memorysave ();
		savestate_capture (0);
	}
	if (savestate_state == STATE_DORESTORE) {
		savestate_state = STATE_RESTORE;
		return true;
	} else if (savestate_state == STATE_DOREWIND) {
		savestate_state = STATE_REWIND;
		return true;
	} else if (savestate_state == STATE_SAVE) {
		savestate_initsave(savestate_fname, 1, true, true);
		save_state(savestate_fname, STATE_SAVE_DESCRIPTION);
		return false;
	}
	return false;
}

static int rewindmode;


static struct staterecord *canrewind (int pos)
{
	if (pos < 0)
		pos += staterecords_max;
	if (!staterecords)
		return 0;
	if (staterecords[pos] == NULL)
		return NULL;
	if (staterecords[pos]->inuse == 0)
		return NULL;
	if ((pos + 1) % staterecords_max  == staterecords_first)
		return NULL;
	return staterecords[pos];
}

int savestate_dorewind (int pos)
{
	rewindmode = pos;
	if (pos < 0)
		pos = replaycounter - 1;
	if (canrewind (pos)) {
		savestate_state = STATE_DOREWIND;
		write_log (_T("dorewind %d (%010ld/%03ld) -> %d\n"), replaycounter - 1, hsync_counter, vsync_counter, pos);
		return 1;
	}
	return 0;
}
#if 0
void savestate_listrewind (void)
{
	int i = replaycounter;
	int cnt;
	uae_u8 *p;
	uae_u32 pc;

	cnt = 1;
	for (;;) {
		struct staterecord *st;
		st = &staterecords[i];
		if (!st->start)
			break;
		p = st->cpu + 17 * 4;
		pc = restore_u32_func (&p);
		console_out_f (_T("%d: PC=%08X %c\n"), cnt, pc, regs.pc == pc ? '*' : ' ');
		cnt++;
		i--;
		if (i < 0)
			i += MAX_STATERECORDS;
	}
}
#endif

void savestate_rewind (void)
{
	int len, i;
	uae_u8 *p, *p2;
	struct staterecord *st;
	int pos;
	bool rewind = false;
	size_t dummy;

	if (hsync_counter % currprefs.statecapturerate <= 25 && rewindmode <= -2) {
		pos = replaycounter - 2;
		rewind = true;
	} else {
		pos = replaycounter - 1;
	}
	st = canrewind (pos);
	if (!st) {
		rewind = false;
		pos = replaycounter - 1;
		st = canrewind (pos);
		if (!st)
			return;
	}
	p = st->data;
	p2 = st->end;
	write_log (_T("rewinding %d -> %d\n"), replaycounter - 1, pos);
	hsync_counter = restore_u32_func (&p);
	vsync_counter = restore_u32_func (&p);
	p = restore_cpu (p);
	p = restore_cycles (p);
	p = restore_cpu_extra (p);
	if (restore_u32_func (&p))
		p = restore_cpu_trace (p);
#ifdef FPUEMU
	if (restore_u32_func (&p))
		p = restore_fpu (p);
#endif
	for (i = 0; i < 4; i++) {
		p = restore_disk (i, p);
		if (restore_u32_func (&p))
			p = restore_disk2 (i, p);
	}
	p = restore_floppy (p);
	p = restore_custom (p);
	p = restore_custom_extra (p);
	if (restore_u32_func (&p))
		p = restore_custom_event_delay (p);
	p = restore_blitter_new (p);
	p = restore_custom_agacolors (p);
	for (i = 0; i < 8; i++) {
		p = restore_custom_sprite (i, p);
	}
	for (i = 0; i < 4; i++) {
		p = restore_audio (i, p);
	}
	p = restore_cia (0, p);
	p = restore_cia (1, p);
	p = restore_keyboard (p);
	p = restore_inputstate (p);
#ifdef AUTOCONFIG
	p = restore_expansion (p);
#endif
#ifdef PICASSO96
	if (restore_u32_func (&p))
		p = restore_p96 (p);
#endif
	len = restore_u32_func (&p);
	memcpy (chipmem_bank.baseaddr, p, currprefs.chipmem.size > len ? len : currprefs.chipmem.size);
	p += len;
	len = restore_u32_func (&p);
	memcpy (save_bram (&dummy), p, currprefs.bogomem.size > len ? len : currprefs.bogomem.size);
	p += len;
#ifdef AUTOCONFIG
	len = restore_u32_func (&p);
	memcpy (save_fram (&dummy, 0), p, currprefs.fastmem[0].size > len ? len : currprefs.fastmem[0].size);
	p += len;
	len = restore_u32_func (&p);
	memcpy (save_zram (&dummy, 0), p, currprefs.z3fastmem[0].size > len ? len : currprefs.z3fastmem[0].size);
	p += len;
#endif
#ifdef ACTION_REPLAY
	if (restore_u32_func (&p))
		p = restore_action_replay (p);
	if (restore_u32_func (&p))
		p = restore_hrtmon (p);
#endif
#ifdef CD32
	if (restore_u32_func (&p))
		p = restore_akiko (p);
#endif
#ifdef CDTV
	if (restore_u32_func (&p))
		p = restore_cdtv (p);
	if (restore_u32_func (&p))
		p = restore_cdtv_dmac (p);
#endif
#if 0
	if (restore_u32_func (&p))
		p = restore_scsi_dmac (WDTYPE_A2091, p);
	if (restore_u32_func (&p))
		p = restore_scsi_dmac (WDTYPE_A3000, p);
#endif
	if (restore_u32_func (&p))
		p = restore_gayle (p);
	for (i = 0; i < 4; i++) {
		if (restore_u32_func (&p))
			p = restore_gayle_ide (p);
	}
	p += 4;
	if (p != p2) {
		gui_message (_T("reload failure, address mismatch %p != %p"), p, p2);
		uae_reset (0, 0);
		return;
	}
	inprec_setposition (st->inprecoffset, pos);
	write_log (_T("state %d restored.  (%010ld/%03ld)\n"), pos, hsync_counter, vsync_counter);
	if (rewind) {
		replaycounter--;
		if (replaycounter < 0)
			replaycounter += staterecords_max;
		st = canrewind (replaycounter);
		st->inuse = 0;
	}

}

#define BS 10000

STATIC_INLINE int bufcheck(struct staterecord *sr, uae_u8 *p, size_t len)
{
	if (p - sr->data + BS + len >= sr->len)
		return 1;
	return 0;
}

void savestate_memorysave (void)
{
	new_blitter = true;
	// create real statefile in memory too for later saving
	zfile_fclose (staterecord_statefile);
	staterecord_statefile = zfile_fopen_empty (NULL, _T("statefile.inp.uss"));
	if (staterecord_statefile)
		save_state_internal (staterecord_statefile, _T("rerecording"), 1, false);
}

void savestate_capture (int force)
{
	uae_u8 *p, *p2, *p3, *dst;
	size_t len, tlen;
	int i, retrycnt;
	struct staterecord *st;
	bool firstcapture = false;

	if (!staterecords)
		return;
	if (!input_record)
		return;
#ifdef FILESYS
	if (nr_units())
		return;
#endif
	if (currprefs.statecapturerate && hsync_counter == 0 && input_record == INPREC_RECORD_START && savestate_first_capture > 0) {
		// first capture
		force = true;
		firstcapture = true;
	} else if (savestate_first_capture < 0) {
		force = true;
		firstcapture = false;
	}
	if (!force) {
		if (currprefs.statecapturerate <= 0)
			return;
		if (hsync_counter % currprefs.statecapturerate)
			return;
	}
	savestate_first_capture = false;

	retrycnt = 0;
retry2:
	st = staterecords[replaycounter];
	if (st == NULL) {
		st = (struct staterecord*)xmalloc (uae_u8, statefile_alloc);
		st->len = statefile_alloc;
	} else if (retrycnt > 0) {
		write_log (_T("realloc %d -> %d\n"), st->len, st->len + STATEFILE_ALLOC_SIZE);
		st->len += STATEFILE_ALLOC_SIZE;
		st = (struct staterecord*)xrealloc (uae_u8, st, st->len);
	}
	if (st->len > statefile_alloc)
		statefile_alloc = st->len;
	st->inuse = 0;
	st->data = (uae_u8*)(st + 1);
	staterecords[replaycounter] = st;
	retrycnt++;
	p = p2 = st->data;
	tlen = 0;
	save_u32_func (&p, hsync_counter);
	save_u32_func (&p, vsync_counter);
	tlen += 8;

	if (bufcheck (st, p, 0))
		goto retry;
	st->cpu = p;
	save_cpu (&len, p);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	save_cycles (&len, p);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	save_cpu_extra (&len, p);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_cpu_trace (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}

#ifdef FPUEMU
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_fpu (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
#endif
	for (i = 0; i < 4; i++) {
		if (bufcheck (st, p, 0))
			goto retry;
		save_disk (i, &len, p, true);
		tlen += len;
		p += len;
		p3 = p;
		save_u32_func (&p, 0);
		tlen += 4;
		if (save_disk2 (i, &len, p)) {
			save_u32_func (&p3, 1);
			tlen += len;
			p += len;
		}
	}

	if (bufcheck (st, p, 0))
		goto retry;
	save_floppy (&len, p);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	save_custom (&len, p, 0);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	save_custom_extra (&len, p);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_custom_event_delay (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}

	if (bufcheck (st, p, 0))
		goto retry;
	save_blitter_new (&len, p);
	tlen += len;
	p += len;

	if (bufcheck (st, p, 0))
		goto retry;
	save_custom_agacolors (&len, p);
	tlen += len;
	p += len;
	for (i = 0; i < 8; i++) {
		if (bufcheck (st, p, 0))
			goto retry;
		save_custom_sprite(i, &len, p);
		tlen += len;
		p += len;
	}

	for (i = 0; i < 4; i++) {
		if (bufcheck (st, p, 0))
			goto retry;
		save_audio (i, &len, p);
		tlen += len;
		p += len;
	}

	if (bufcheck(st, p, len))
		goto retry;
	save_cia (0, &len, p);
	tlen += len;
	p += len;

	if (bufcheck(st, p, len))
		goto retry;
	save_cia (1, &len, p);
	tlen += len;
	p += len;

	if (bufcheck(st, p, len))
		goto retry;
	save_keyboard (&len, p);
	tlen += len;
	p += len;

	if (bufcheck(st, p, len))
		goto retry;
	save_inputstate (&len, p);
	tlen += len;
	p += len;

#ifdef AUTOCONFIG
	if (bufcheck (st, p, len))
		goto retry;
	save_expansion (&len, p);
	tlen += len;
	p += len;
#endif

#ifdef PICASSO96
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_p96 (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
#endif

	dst = save_cram(&len);
	if (bufcheck(st, p, len))
		goto retry;
	save_u32t_func(&p, len);
	memcpy(p, dst, len);
	tlen += len + 4;
	p += len;
	dst = save_bram(&len);
	if (bufcheck(st, p, len))
		goto retry;
	save_u32t_func(&p, len);
	memcpy(p, dst, len);
	tlen += len + 4;
	p += len;
#ifdef AUTOCONFIG
	dst = save_fram(&len, 0);
	if (bufcheck(st, p, len))
		goto retry;
	save_u32t_func(&p, len);
	memcpy(p, dst, len);
	tlen += len + 4;
	p += len;
	dst = save_zram(&len, 0);
	if (bufcheck(st, p, len))
		goto retry;
	save_u32t_func(&p, len);
	memcpy(p, dst, len);
	tlen += len + 4;
	p += len;
#endif
#ifdef ACTION_REPLAY
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_action_replay (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_hrtmon (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
#endif
#ifdef CD32
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_akiko (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
#endif
#ifdef CDTV
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_cdtv (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_cdtv_dmac (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
#endif
#if 0
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_scsi_dmac (WDTYPE_A2091, &len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_scsi_dmac (WDTYPE_A3000, &len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
#endif
	if (bufcheck (st, p, 0))
		goto retry;
	p3 = p;
	save_u32_func (&p, 0);
	tlen += 4;
	if (save_gayle (&len, p)) {
		save_u32_func (&p3, 1);
		tlen += len;
		p += len;
	}
	for (i = 0; i < 4; i++) {
		if (bufcheck (st, p, 0))
			goto retry;
		p3 = p;
		save_u32_func (&p, 0);
		tlen += 4;
		if (save_gayle_ide (i, &len, p)) {
			save_u32_func (&p3, 1);
			tlen += len;
			p += len;
		}
	}
	save_u32t_func(&p, tlen);
	st->end = p;
	st->inuse = 1;
	st->inprecoffset = inprec_getposition ();

	replaycounter++;
	if (replaycounter >= staterecords_max)
		replaycounter -= staterecords_max;
	if (replaycounter == staterecords_first) {
		staterecords_first++;
		if (staterecords_first >= staterecords_max)
			staterecords_first -= staterecords_max;
	}

	write_log (_T("state capture %d (%010ld/%03ld,%ld/%d) (%ld bytes, alloc %d)\n"),
		replaycounter, hsync_counter, vsync_counter,
		hsync_counter % current_maxvpos (), current_maxvpos (),
		st->end - st->data, statefile_alloc);

	if (firstcapture) {
		savestate_memorysave ();
		input_record++;
		for (i = 0; i < 4; i++) {
			bool wp = true;
			DISK_validate_filename (&currprefs, currprefs.floppyslots[i].df, i, NULL, false, &wp, NULL, NULL);
			inprec_recorddiskchange (i, currprefs.floppyslots[i].df, wp);
		}
		input_record--;
	}


	return;
retry:
	if (retrycnt < 10)
		goto retry2;
	write_log (_T("can't save, too small capture buffer or out of memory\n"));
	return;
}

void savestate_free (void)
{
	xfree (staterecords);
	staterecords = NULL;
}

void savestate_capture_request (void)
{
	savestate_first_capture = -1;
}

void savestate_init (void)
{
	savestate_free ();
	replaycounter = 0;
	staterecords_max = currprefs.statecapturebuffersize;
	staterecords = xcalloc (struct staterecord*, staterecords_max);
	statefile_alloc = STATEFILE_ALLOC_SIZE;
	if (input_record && savestate_state != STATE_DORESTORE) {
		zfile_fclose (staterecord_statefile);
		staterecord_statefile = NULL;
		inprec_close (false);
		inprec_open (NULL, NULL);
		savestate_first_capture = 1;
	}
}


void statefile_save_recording (const TCHAR *filename)
{
	if (!staterecord_statefile)
		return;
	struct zfile *zf = zfile_fopen(filename, _T("wb"), 0);
	if (zf) {
		int len = zfile_size32(staterecord_statefile);
		uae_u8 *data = zfile_getdata(staterecord_statefile, 0, len, NULL);
		zfile_fwrite(data, len, 1, zf);
		xfree (data);
		zfile_fclose(zf);
		write_log (_T("input statefile '%s' saved\n"), filename);
	}
}


/*

My (Toni Wilen <twilen@arabuusimiehet.com>)
proposal for Amiga-emulators' state-save format

Feel free to comment...

This is very similar to IFF-fileformat
Every hunk must end to 4 byte boundary,
fill with zero bytes if needed

version 0.8

HUNK HEADER (beginning of every hunk)

hunk name (4 ascii-characters)
hunk size (including header)
hunk flags

bit 0 = chunk contents are compressed with zlib (maybe RAM chunks only?)

HEADER

"ASF " (AmigaStateFile)

statefile version
emulator name ("uae", "fellow" etc..)
emulator version string (example: "0.8.15")
free user writable comment string

CPU

"CPU "

CPU model               4 (68000,68010,68020,68030,68040,68060)
CPU typeflags           bit 0=EC-model or not, bit 31 = clock rate included
D0-D7                   8*4=32
A0-A6                   7*4=32
PC                      4
unused			4
68000 prefetch (IRC)    2
68000 prefetch (IR)     2
USP                     4
ISP                     4
SR/CCR                  2
flags                   4 (bit 0=CPU was HALTed)

CPU specific registers

68000: SR/CCR is last saved register
68010: save also DFC,SFC and VBR
68020: all 68010 registers and CAAR,CACR and MSP
etc..

68010+:

DFC                     4
SFC                     4
VBR                     4

68020+:

CAAR                    4
CACR                    4
MSP                     4

68030+:

AC0                     4
AC1                     4
ACUSR                   2
TT0                     4
TT1                     4

68040+:

ITT0                    4
ITT1                    4
DTT0                    4
DTT1                    4
TCR                     4
URP                     4
SRP                     4

68060:

BUSCR                   4
PCR                     4

All:

Clock in KHz            4 (only if bit 31 in flags)
4 (spare, only if bit 31 in flags)


FPU (only if used)

"FPU "

FPU model               4 (68881/68882/68040/68060)
FPU typeflags           4 (bit 31 = clock rate included)
FP0-FP7                 4+4+2 (80 bits)
FPCR                    4
FPSR                    4
FPIAR                   4

Clock in KHz            4 (only if bit 31 in flags)
4 (spare, only if bit 31 in flags)

MMU (when and if MMU is supported in future..)

"MMU "

MMU model               4 (68040)
flags			4 (none defined yet)

CUSTOM CHIPS

"CHIP"

chipset flags   4      OCS=0,ECSAGNUS=1,ECSDENISE=2,AGA=4
ECSAGNUS and ECSDENISE can be combined

DFF000-DFF1FF   352    (0x120 - 0x17f and 0x0a0 - 0xdf excluded)

sprite registers (0x120 - 0x17f) saved with SPRx chunks
audio registers (0x0a0 - 0xdf) saved with AUDx chunks

AGA COLORS

"AGAC"

AGA color               8 banks * 32 registers *
registers               LONG (XRGB) = 1024

SPRITE

"SPR0" - "SPR7"


SPRxPT                  4
SPRxPOS                 2
SPRxCTL                 2
SPRxDATA                2
SPRxDATB                2
AGA sprite DATA/DATB    3 * 2 * 2
sprite "armed" status   1

sprites maybe armed in non-DMA mode
use bit 0 only, other bits are reserved


AUDIO
"AUD0" "AUD1" "AUD2" "AUD3"

audio state             1
machine mode
AUDxVOL                 1
irq?                    1
data_written?           1
internal AUDxLEN        2
AUDxLEN                 2
internal AUDxPER        2
AUDxPER                 2
internal AUDxLC         4
AUDxLC                  4
evtime?                 4

BLITTER

"BLIT"

internal blitter state

flags                   4
bit 0=blitter active
bit 1=fill carry bit
internal ahold          4
internal bhold          4
internal hsize          2
internal vsize          2

CIA

"CIAA" and "CIAB"

BFE001-BFEF01   16*1 (CIAA)
BFD000-BFDF00   16*1 (CIAB)

internal registers

IRQ mask (ICR)  1 BYTE
timer latches   2 timers * 2 BYTES (LO/HI)
latched tod     3 BYTES (LO/MED/HI)
alarm           3 BYTES (LO/MED/HI)
flags           1 BYTE
bit 0=tod latched (read)
bit 1=tod stopped (write)
div10 counter	1 BYTE

FLOPPY DRIVES

"DSK0" "DSK1" "DSK2" "DSK3"

drive state

drive ID-word           4
state                   1 (bit 0: motor on, bit 1: drive disabled, bit 2: current id bit)
rw-head track           1
dskready                1
id-mode                 1 (ID mode bit number 0-31)
floppy information

bits from               4
beginning of track
CRC of disk-image       4 (used during restore to check if image
is correct)
disk-image              null-terminated
file name

INTERNAL FLOPPY	CONTROLLER STATUS

"DISK"

current DMA word        2
DMA word bit offset     1
WORDSYNC found          1 (no=0,yes=1)
hpos of next bit        1
DSKLENGTH status        0=off,1=written once,2=written twice
unused                  2

RAM SPACE

"xRAM" (CRAM = chip, BRAM = bogo, FRAM = fast, ZRAM = Z3, P96 = RTG RAM, A3K1/A3K2 = MB RAM)

start address           4 ("bank"=chip/slow/fast etc..)
of RAM "bank"
RAM "bank" size         4
RAM flags               4 (bit 0 = zlib compressed)
RAM "bank" contents

ROM SPACE

"ROM "

ROM start               4
address
size of ROM             4
ROM type                4 KICK=0
ROM flags               4
ROM version             2
ROM revision            2
ROM CRC                 4 see below
ROM-image ID-string     null terminated, see below
path to rom image
ROM contents            (Not mandatory, use hunk size to check if
this hunk contains ROM data or not)

Kickstart ROM:
ID-string is "Kickstart x.x"
ROM version: version in high word and revision in low word
Kickstart ROM version and revision can be found from ROM start
+ 12 (version) and +14 (revision)

ROM version and CRC is only meant for emulator to automatically
find correct image from its ROM-directory during state restore.

Usually saving ROM contents is not good idea.

ACTION REPLAY

"ACTR"

Model (1,2,3)		4
path to rom image
RAM space		(depends on model)
ROM CRC             4

"CDx "

Flags               4 (bit 0 = scsi, bit 1 = ide, bit 2 = image)
Path                  (for example image file or drive letter)

END
hunk "END " ends, remember hunk size 8!


EMULATOR SPECIFIC HUNKS

Read only if "emulator name" in header is same as used emulator.
Maybe useful for configuration?

misc:

- save only at position 0,0 before triggering VBLANK interrupt
- all data must be saved in bigendian format
- should we strip all paths from image file names?

*/
