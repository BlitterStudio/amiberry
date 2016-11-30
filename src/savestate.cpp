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
#include "disk.h"

int savestate_state = 0;

static bool new_blitter = false;

struct zfile *savestate_file;
static int savestate_docompress, savestate_specialdump, savestate_nodialogs;

TCHAR savestate_fname[MAX_DPATH];

static void state_incompatible_warn(void)
{
  static int warned;
  int dowarn = 0;
  int i;

#ifdef BSDSOCKET
	if (currprefs.socket_emu)
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
  for(i = 0; i < currprefs.mountitems; i++) {
    struct mountedinfo mi;
  	int type = get_filesys_unitconfig (&currprefs, i, &mi);
  	if (mi.ismounted && type != FILESYS_VIRTUAL && type != FILESYS_HARDFILE && type != FILESYS_HARDFILE_RDB)
	    dowarn = 1;
  }
#endif
  if (!warned && dowarn) {
  	warned = 1;
  	notify_user (NUMSG_STATEHD);
  }
}

/* functions for reading/writing bytes, shorts and longs in big-endian
 * format independent of host machine's endianess */

void save_u32_func (uae_u8 **dstp, uae_u32 v)
{
  uae_u8 *dst = *dstp;
  *dst++ = (uae_u8)(v >> 24);
  *dst++ = (uae_u8)(v >> 16);
  *dst++ = (uae_u8)(v >> 8);
  *dst++ = (uae_u8)(v >> 0);
  *dstp = dst;
}
void save_u64_func (uae_u8 **dstp, uae_u64 v)
{
  save_u32_func (dstp, (uae_u32)(v >> 32));
  save_u32_func (dstp, (uae_u32)v);
}
void save_u16_func (uae_u8 **dstp, uae_u16 v)
{
  uae_u8 *dst = *dstp;
  *dst++ = (uae_u8)(v >> 8);
  *dst++ = (uae_u8)(v >> 0);
  *dstp = dst;
}
void save_u8_func (uae_u8 **dstp, uae_u8 v)
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
  len = strlen((char *)dst) + 1;
  top = to = xmalloc (char, len);
  do {
  	v = *dst++;
  	*top++ = v;
  } while(v);
  *dstp = dst;
	s = utf8u (to);
	xfree (to);
	return s;
}
TCHAR *restore_path_func (uae_u8 **dstp, int type)
{
	TCHAR *newpath;
	TCHAR *s;
  TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH];

	s = restore_string_func(dstp);
	if (s[0] == 0)
		return s;
	if (zfile_exists (s))
		return s;
	if (type == SAVESTATE_PATH_HD)
		return s;
	getfilepart (tmp, sizeof tmp / sizeof (TCHAR), s);
	if (zfile_exists (tmp)) {
		xfree (s);
		return my_strdup (tmp);
	}

	newpath = NULL;
	if (type == SAVESTATE_PATH_FLOPPY)
		newpath = currprefs.path_floppy;
	else if (type == SAVESTATE_PATH_VDIR || type == SAVESTATE_PATH_HDF)
		newpath = currprefs.path_hardfile;
	else if (type == SAVESTATE_PATH_CD)
		newpath = currprefs.path_cd;
	if (newpath != NULL && newpath[0] != 0) {
		_tcscpy (tmp2, newpath);
		fixtrailing (tmp2);
		_tcscat (tmp2, tmp);
		if (zfile_exists (tmp2)) {
			xfree (s);
			return my_strdup (tmp2);
		}
  }
	getpathpart (tmp2, sizeof tmp2 / sizeof (TCHAR), savestate_fname);
	_tcscat (tmp2, tmp);
	if (zfile_exists (tmp2)) {
		xfree (s);
		return my_strdup (tmp2);
	}
	return s;
}

/* read and write IFF-style hunks */

static void save_chunk (struct zfile *f, uae_u8 *chunk, size_t len, TCHAR *name, int compress)
{
  uae_u8 tmp[8], *dst;
  uae_u8 zero[4]= { 0, 0, 0, 0 };
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
  pos = zfile_ftell (f);
  /* chunk size */
  dst = &tmp[0];
  chunklen = len + 4 + 4 + 4;
  save_u32 (chunklen);
  zfile_fwrite (&tmp[0], 1, 4, f);
  /* chunk flags */
  flags = 0;
  dst = &tmp[0];
  save_u32 (flags | compress);
  zfile_fwrite (&tmp[0], 1, 4, f);
  /* chunk data */
  if (compress) {
  	int tmplen = len;
  	size_t opos;
  	dst = &tmp[0];
  	save_u32 (len);
  	opos = zfile_ftell (f);
  	zfile_fwrite (&tmp[0], 1, 4, f);
  	len = zfile_zcompress (f, chunk, len);
  	if (len > 0) {
	    zfile_fseek (f, pos, SEEK_SET);
	    dst = &tmp[0];
	    save_u32 (len + 4 + 4 + 4 + 4);
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
  if (len2)
  	zfile_fwrite (zero, 1, len2, f);

  write_log (_T("Chunk '%s' chunk size %d (%d)\n"), name, chunklen, len);
}

static uae_u8 *restore_chunk (struct zfile *f, TCHAR *name, size_t *len, size_t *totallen, size_t *filepos)
{
  uae_u8 tmp[6], dummy[4], *mem, *src;
  uae_u32 flags;
  int len2;

  *totallen = 0;
  /* chunk name */
	zfile_fread (tmp, 1, 4, f);
	tmp[4] = 0;
	au_copy (name, 5, (char*)tmp);
  /* chunk size */
  zfile_fread (tmp, 1, 4, f);
  src = tmp;
  len2 = restore_u32 () - 4 - 4 - 4;
  if (len2 < 0)
  	len2 = 0;
  *len = len2;
  if (len2 == 0) {
	  *filepos = zfile_ftell (f);
	  return 0;
  }

  /* chunk flags */
  zfile_fread (tmp, 1, 4, f);
  src = tmp;
  flags = restore_u32 ();
  *totallen = *len;
  if (flags & 1) {
  	zfile_fread (tmp, 1, 4, f);
  	src = tmp;
  	*totallen = restore_u32();
  	*filepos = zfile_ftell (f) - 4 - 4 - 4;
  	len2 -= 4;
  } else {
    *filepos = zfile_ftell (f) - 4 - 4;
  }
  /* chunk data.  RAM contents will be loaded during the reset phase,
     no need to malloc multiple megabytes here.  */
  if (_tcscmp (name, _T("CRAM")) != 0
  	&& _tcscmp (name, _T("BRAM")) != 0
  	&& _tcscmp (name, _T("FRAM")) != 0
  	&& _tcscmp (name, _T("ZRAM")) != 0
		&& _tcscmp (name, _T("ZCRM")) != 0
  	&& _tcscmp (name, _T("PRAM")) != 0
  	&& _tcscmp (name, _T("A3K1")) != 0
		&& _tcscmp (name, _T("A3K2")) != 0
		&& _tcscmp (name, _T("BORO")) != 0
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
  size = restore_u32();
  flags = restore_u32();
  size -= 4 + 4 + 4;
  if (flags & 1) {
    zfile_fread (tmp, 1, 4, savestate_file);
    src = tmp;
    fullsize = restore_u32();
    size -= 4;
    zfile_zuncompress (memory, fullsize, savestate_file, size);
  } else {
    zfile_fread (memory, 1, size, savestate_file);
  }
}

static void restore_header (uae_u8 *src)
{
  TCHAR *emuname, *emuversion, *description;

  restore_u32();
  emuname = restore_string ();
  emuversion = restore_string ();
  description = restore_string ();
	write_log (_T("Saved with: '%s %s', description: '%s'\n"),
  	emuname,emuversion,description);
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
  size_t len, totallen;
  size_t filepos, filesize;
  int z3num;

  chunk = 0;
	f = zfile_fopen (filename, _T("rb"), ZFD_NORMAL);
  if (!f)
  	goto error;
  zfile_fseek (f, 0, SEEK_END);
  filesize = zfile_ftell (f);
  zfile_fseek (f, 0, SEEK_SET);
  savestate_state = STATE_RESTORE;

  chunk = restore_chunk (f, name, &len, &totallen, &filepos);
  if (!chunk || _tcsncmp (name, _T("ASF "), 4)) {
		write_log (_T("%s is not an AmigaStateFile\n"), filename);
  	goto error;
  }
	write_log (_T("STATERESTORE: '%s'\n"), filename);
  savestate_file = f;
  restore_header (chunk);
  xfree (chunk);
	restore_cia_start ();
  changed_prefs.bogomem_size = 0;
  changed_prefs.chipmem_size = 0;
  changed_prefs.fastmem_size = 0;
  changed_prefs.z3fastmem_size = 0;
  z3num = 0;
  for (;;) {
  	name[0] = 0;
  	chunk = end = restore_chunk (f, name, &len, &totallen, &filepos);
  	write_log (_T("Chunk '%s' size %d (%d)\n"), name, len, totallen);
  	if (!_tcscmp (name, _T("END "))) {
	    break;
    }
		if (!_tcscmp (name, _T("CRAM"))) {
	    restore_cram (totallen, filepos);
	    continue;
		} else if (!_tcscmp (name, _T("BRAM"))) {
	    restore_bram (totallen, filepos);
	    continue;
#ifdef AUTOCONFIG
		} else if (!_tcscmp (name, _T("FRAM"))) {
	    restore_fram (totallen, filepos);
	    continue;
		} else if (!_tcscmp (name, _T("ZRAM"))) {
	    restore_zram (totallen, filepos, z3num++);
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
		} else if (!_tcscmp (name, _T("CPU "))) {
	    end = restore_cpu (chunk);
		} else if (!_tcscmp (name, _T("CPUX")))
	  	end = restore_cpu_extra (chunk);
#ifdef FPUEMU
  	else if (!_tcscmp (name, _T("FPU ")))
	    end = restore_fpu (chunk);
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
		else if (!_tcscmp (name, _T("SPR7")))
	    end = restore_custom_sprite (7, chunk);
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
#ifdef FILESYS
		else if (!_tcscmp (name, _T("FSYS")))
	    end = restore_filesys (chunk);
		else if (!_tcscmp (name, _T("FSYC")))
	    end = restore_filesys_common (chunk);
#endif
#ifdef CD32
		else if (!_tcscmp (name, _T("CD32")))
			end = restore_akiko (chunk);
#endif

		else if (!_tcsncmp (name, _T("CDU"), 3))
			end = restore_cd (name[3] - '0', chunk);
	  else {
	    end = chunk + len;
			write_log (_T("unknown chunk '%s' size %d bytes\n"), name, len);
	  }
	  if (end == NULL)
			write_log (_T("Chunk '%s', size %d bytes was not accepted!\n"),
	      name, len);
  	else if (totallen != end - chunk)
  		write_log (_T("Chunk '%s' total size %d bytes but read %d bytes!\n"),
	      name, totallen, end - chunk);
  	xfree (chunk);
  }
  return;

error:
  savestate_state = 0;
  savestate_file = 0;
  if (chunk)
  	xfree (chunk);
  if (f)
  	zfile_fclose (f);
}

void savestate_restore_finish (void)
{
	if (!isrestore ())
  	return;
  zfile_fclose (savestate_file);
  savestate_file = 0;
  restore_cpu_finish();
	restore_audio_finish ();
	restore_disk_finish ();
	restore_akiko_finish ();
#ifdef PICASSO96
	restore_p96_finish ();
#endif
	restore_cia_finish ();
	savestate_state = 0;
  init_hz_full ();
	audio_activate ();
}

/* 1=compressed,2=not compressed,3=ram dump,4=audio dump */
void savestate_initsave (const TCHAR *filename, int mode, int nodialogs, bool save)
{
  if (filename == NULL) {
	  savestate_fname[0] = 0;
	  savestate_docompress = 0;
	  savestate_specialdump = 0;
	  savestate_nodialogs = 0;
	  return;
  }
  _tcscpy (savestate_fname, filename);
  savestate_docompress = (mode == 1) ? 1 : 0;
  savestate_specialdump = (mode == 3) ? 1 : (mode == 4) ? 2 : 0;
  savestate_nodialogs = nodialogs;
	new_blitter = false;
}

static void save_rams (struct zfile *f, int comp)
{
  uae_u8 *dst;
  int len;

  dst = save_cram (&len);
	save_chunk (f, dst, len, _T("CRAM"), comp);
  dst = save_bram (&len);
  save_chunk (f, dst, len, _T("BRAM"), comp);
#ifdef AUTOCONFIG
  dst = save_fram (&len);
	save_chunk (f, dst, len, _T("FRAM"), comp);
  dst = save_zram (&len, 0);
  save_chunk (f, dst, len, _T("ZRAM"), comp);
  dst = save_bootrom (&len);
	save_chunk (f, dst, len, _T("BORO"), comp);
#endif
#ifdef PICASSO96
  dst = save_pram (&len);
	save_chunk (f, dst, len, _T("PRAM"), comp);
#endif
}

/* Save all subsystems  */

static int save_state_internal (struct zfile *f, const TCHAR *description, int comp, bool savepath)
{
  uae_u8 endhunk[] = { 'E', 'N', 'D', ' ', 0, 0, 0, 8 };
  uae_u8 header[1000];
  TCHAR tmp[100];
  uae_u8 *dst;
  TCHAR name[5];
	int i, len;

	write_log (_T("STATESAVE (%s):\n"), f ? zfile_getname (f) : _T("<internal>"));
  dst = header;
  save_u32 (0);
  save_string(_T("UAE"));
  _stprintf (tmp, _T("%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
  save_string (tmp);
  save_string (description);
	save_chunk (f, header, dst-header, _T("ASF "), 0);

  dst = save_cpu (&len, 0);
	save_chunk (f, dst, len, _T("CPU "), 0);
  xfree (dst);

  dst = save_cpu_extra (&len, 0);
  save_chunk (f, dst, len, _T("CPUX"), 0);
  xfree (dst);

#ifdef FPUEMU
  dst = save_fpu (&len, 0);
	save_chunk (f, dst, len, _T("FPU "), 0);
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

  dst = save_blitter_new (&len, 0);
	save_chunk (f, dst, len, _T("BLTX"), 0);
  xfree (dst);
  if (new_blitter == false) {
	  dst = save_blitter (&len, 0);
		save_chunk (f, dst, len, _T("BLIT"), 0);
	  xfree (dst);
  }

	dst = save_input (&len, 0);
	save_chunk (f, dst, len, _T("CINP"), 0);
	xfree (dst);

  dst = save_custom_agacolors (&len, 0);
	save_chunk (f, dst, len, _T("AGAC"), 0);
  xfree (dst);

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

#ifdef AUTOCONFIG
  dst = save_expansion (&len, 0);
  save_chunk (f, dst, len, _T("EXPA"), 0);
  xfree (dst);
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

#ifdef FILESYS
  dst = save_filesys_common (&len);
  if (dst) {
		save_chunk (f, dst, len, _T("FSYC"), 0);
    for (i = 0; i < nr_units (); i++) {
    	dst = save_filesys (i, &len);
    	if (dst) {
				save_chunk (f, dst, len, _T("FSYS"), 0);
  	    xfree (dst);
    	}
    }
  }
#endif

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		dst = save_cd (i, &len);
		if (dst) {
			_stprintf (name, _T("CDU%d"), i);
			save_chunk (f, dst, len, name, 0);
		}
	}

  zfile_fwrite (endhunk, 1, 8, f);

  return 1;
}

int save_state (const TCHAR *filename, const TCHAR *description)
{
	struct zfile *f;
  int comp = savestate_docompress;

  if (!savestate_specialdump && !savestate_nodialogs) {
	  state_incompatible_warn();
	  if (!save_filesys_cando()) {
			gui_message (_T("Filesystem active. Try again later."));
	    return -1;
  	}
  }
	new_blitter = false;
  savestate_nodialogs = 0;
  custom_prepare_savestate ();
	f = zfile_fopen (filename, _T("w+b"), 0);
  if (!f)
  	return 0;
	int v = save_state_internal (f, description, comp, true);
	if (v)
    write_log (_T("Save of '%s' complete\n"), filename);
  zfile_fclose (f);
  savestate_state = 0;
	return v;
}

bool savestate_check (void)
{
	if (savestate_state == STATE_DORESTORE) {
		savestate_state = STATE_RESTORE;
		return true;
	}
	return false;
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
	ROM CRC                 4

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
