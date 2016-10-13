/*
 * UAE - The Un*x Amiga Emulator
 *
 * Config file handling
 * This still needs some thought before it's complete...
 *
 * Copyright 1998 Brian King, Bernd Schmidt
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "autoconf.h"
#include "custom.h"
#include "inputdevice.h"
#include "savestate.h"
#include "include/memory.h"
#include "rommgr.h"
#include "gui.h"
#include "newcpu.h"
#include "zfile.h"
#include "filesys.h"
#include "fsdb.h"
#include "disk.h"
#include "blkdev.h"
#include "calc.h"
#include "gfxboard.h"

static int config_newfilesystem;
static struct strlist *temp_lines;
static struct strlist *error_lines;
static struct zfile *default_file, *configstore;
static int uaeconfig;

/* @@@ need to get rid of this... just cut part of the manual and print that
 * as a help text.  */
struct cfg_lines
{
  const TCHAR *config_label, *config_help;
};

static const TCHAR *guimode1[] = { _T("no"), _T("yes"), _T("nowait"), 0 };
static const TCHAR *guimode2[] = { _T("false"), _T("true"), _T("nowait"), 0 };
static const TCHAR *guimode3[] = { _T("0"), _T("1"), _T("nowait"), 0 };
static const TCHAR *csmode[] = { _T("ocs"), _T("ecs_agnus"), _T("ecs_denise"), _T("ecs"), _T("aga"), 0 };
static const TCHAR *speedmode[] = { _T("max"), _T("real"), 0 };
static const TCHAR *soundmode1[] = { _T("none"), _T("interrupts"), _T("normal"), _T("exact"), 0 };
static const TCHAR *soundmode2[] = { _T("none"), _T("interrupts"), _T("good"), _T("best"), 0 };
static const TCHAR *stereomode[] = { _T("mono"), _T("stereo"), _T("clonedstereo"), _T("4ch"), _T("clonedstereo6ch"), _T("6ch"), _T("mixed"), 0 };
static const TCHAR *interpolmode[] = { _T("none"), _T("anti"), _T("sinc"), _T("rh"), _T("crux"), 0 };
static const TCHAR *collmode[] = { _T("none"), _T("sprites"), _T("playfields"), _T("full"), 0 };
static const TCHAR *soundfiltermode1[] = { _T("off"), _T("emulated"), _T("on"), 0 };
static const TCHAR *soundfiltermode2[] = { _T("standard"), _T("enhanced"), 0 };
static const TCHAR *lorestype1[] = { _T("lores"), _T("hires"), _T("superhires"), 0 };
static const TCHAR *lorestype2[] = { _T("true"), _T("false"), 0 };
static const TCHAR *abspointers[] = { _T("none"), _T("mousehack"), _T("tablet"), 0 };
static const TCHAR *joyportmodes[] = { _T(""), _T("mouse"), _T("mousenowheel"), _T("djoy"), _T("gamepad"), _T("ajoy"), _T("cdtvjoy"), _T("cd32joy"), _T("lightpen"), 0 };
static const TCHAR *joyaf[] = { _T("none"), _T("normal"), _T("toggle"), 0 };
static const TCHAR *cdmodes[] = { _T("disabled"), _T(""), _T("image"), _T("ioctl"), _T("spti"), _T("aspi"), 0 };
static const TCHAR *cdconmodes[] = { _T(""), _T("uae"), _T("ide"), _T("scsi"), _T("cdtv"), _T("cd32"), 0 };
static const TCHAR *rtgtype[] = {
	_T("ZorroII"), _T("ZorroIII"),
	_T("PicassoII"),
	_T("PicassoII+"),
	_T("Piccolo_Z2"), _T("Piccolo_Z3"),
	_T("PiccoloSD64_Z2"), _T("PiccoloSD64_Z3"),
	_T("Spectrum28/24_Z2"), _T("Spectrum28/24_Z3"),
	_T("PicassoIV_Z2"), _T("PicassoIV_Z3"),
	0 };
static const TCHAR *waitblits[] = { _T("disabled"), _T("automatic"), _T("noidleonly"), _T("always"), 0 };

static const TCHAR *obsolete[] = {
	_T("accuracy"), _T("gfx_opengl"), _T("gfx_32bit_blits"), _T("32bit_blits"),
	_T("gfx_immediate_blits"), _T("gfx_ntsc"), _T("win32"), _T("gfx_filter_bits"),
	_T("sound_pri_cutoff"), _T("sound_pri_time"), _T("sound_min_buff"), _T("sound_bits"),
	_T("gfx_test_speed"), _T("gfxlib_replacement"), _T("enforcer"), _T("catweasel_io"),
  _T("kickstart_key_file"), _T("sound_adjust"), _T("sound_latency"),
	_T("serial_hardware_dtrdsr"), _T("gfx_filter_upscale"),
	_T("gfx_correct_aspect"), _T("gfx_autoscale"), _T("parallel_sampler"), _T("parallel_ascii_emulation"),
	_T("avoid_vid"), _T("avoid_dga"), _T("z3chipmem_size"), _T("state_replay_buffer"), _T("state_replay"),

	_T("gfx_filter_vert_zoom"),_T("gfx_filter_horiz_zoom"),
	_T("gfx_filter_vert_zoom_mult"), _T("gfx_filter_horiz_zoom_mult"),
	_T("gfx_filter_vert_offset"), _T("gfx_filter_horiz_offset"),
	_T("rtg_vert_zoom_multf"), _T("rtg_horiz_zoom_multf"),

  NULL
};

#define UNEXPANDED _T("$(FILE_PATH)")

static void trimwsa (char *s)
{
  /* Delete trailing whitespace.  */
  int len = strlen (s);
  while (len > 0 && strcspn (s + len - 1, "\t \r\n") == 0)
    s[--len] = '\0';
}

static int match_string (const TCHAR *table[], const TCHAR *str)
{
  int i;
  for (i = 0; table[i] != 0; i++)
  	if (strcasecmp (table[i], str) == 0)
	    return i;
  return -1;
}

// escape config file separators and control characters
static TCHAR *cfgfile_escape (const TCHAR *s, const TCHAR *escstr, bool quote)
{
	bool doquote = false;
	int cnt = 0;
	for (int i = 0; s[i]; i++) {
		TCHAR c = s[i];
		if (c == 0)
			break;
		if (c < 32 || c == '\\' || c == '\"' || c == '\'') {
			cnt++;
		}
		for (int j = 0; escstr && escstr[j]; j++) {
			if (c == escstr[j]) {
				cnt++;
				if (quote) {
					doquote = true;
					cnt++;
				}
			}
		}
	}
	TCHAR *s2 = xmalloc (TCHAR, _tcslen (s) + cnt * 4 + 1);
	TCHAR *p = s2;
	if (doquote)
		*p++ = '\"';
	for (int i = 0; s[i]; i++) {
		TCHAR c = s[i];
		if (c == 0)
			break;
		if (c == '\\' || c == '\"' || c == '\'') {
			*p++ = '\\';
			*p++ = c;
		} else if (c >= 32 && !quote) {
			bool escaped = false;
			for (int j = 0; escstr && escstr[j]; j++) {
				if (c == escstr[j]) {
					*p++ = '\\';
					*p++ = c;
					escaped = true;
					break;
				}
			}
			if (!escaped)
				*p++ = c;
		} else if (c < 32) {
			*p++ = '\\';
			switch (c)
			{
				case '\t':
				*p++ = 't';
				break;
				case '\n':
				*p++ = 'n';
				break;
				case '\r':
				*p++ = 'r';
				break;
				default:
				*p++ = 'x';
				*p++ = (c >> 4) >= 10 ? (c >> 4) + 'a' : (c >> 4) + '0';
				*p++ = (c & 15) >= 10 ? (c & 15) + 'a' : (c & 15) + '0';
				break;
			}
		} else {
			*p++ = c;
		}
	}
	if (doquote)
		*p++ = '\"';
	*p = 0;
	return s2;
}
static TCHAR *cfgfile_unescape (const TCHAR *s, const TCHAR **endpos, TCHAR separator)
{
	bool quoted = false;
	TCHAR *s2 = xmalloc (TCHAR, _tcslen (s) + 1);
	TCHAR *p = s2;
	if (s[0] == '\"') {
		s++;
		quoted = true;
	}
	int i;
	for (i = 0; s[i]; i++) {
		TCHAR c = s[i];
		if (quoted && c == '\"') {
			i++;
			break;
		}
		if (c == separator) {
			i++;
			break;
		}
		if (c == '\\') {
			char v = 0;
			TCHAR c2;
			c = s[i + 1];
			switch (c)
			{
				case 'X':
				case 'x':
				c2 = _totupper (s[i + 2]);
				v = ((c2 >= 'A') ? c2 - 'A' : c2 - '0') << 4;
				c2 = _totupper (s[i + 3]);
				v |= (c2 >= 'A') ? c2 - 'A' : c2 - '0';
				*p++ = c2;
				i += 2;
				break;
				case 'r':
				*p++ = '\r';
				break;
				case '\n':
				*p++ = '\n';
				break;
				default:
				*p++ = c;
				break;
			}
			i++;
		} else {
			*p++ = c;
		}
	}
	*p = 0;
	if (endpos)
		*endpos = &s[i];
	return s2;
}
static TCHAR *cfgfile_unescape (const TCHAR *s, const TCHAR **endpos)
{
	return cfgfile_unescape (s, endpos, 0);
}

static TCHAR *getnextentry (const TCHAR **valuep, const TCHAR separator)
{
	TCHAR *s;
	const TCHAR *value = *valuep;
	if (value[0] == '\"') {
		s = cfgfile_unescape (value, valuep);
		value = *valuep;
		if (*value != 0 && *value != separator) {
			xfree (s);
			return NULL;
		}
		value++;
		*valuep = value;
	} else {
		s = cfgfile_unescape (value, valuep, separator);
	}
	return s;
}

static TCHAR *cfgfile_subst_path2 (const TCHAR *path, const TCHAR *subst, const TCHAR *file)
{
  /* @@@ use strcasecmp for some targets.  */
  if (_tcslen (path) > 0 && _tcsncmp (file, path, _tcslen (path)) == 0) {
  	int l;
  	TCHAR *p2, *p = xmalloc (TCHAR, _tcslen (file) + _tcslen (subst) + 2);
  	_tcscpy (p, subst);
  	l = _tcslen (p);
  	while (l > 0 && p[l - 1] == '/')
	    p[--l] = '\0';
  	l = _tcslen (path);
  	while (file[l] == '/')
	    l++;
		_tcscat (p, _T("/"));
    _tcscat (p, file + l);
  	p2 = target_expand_environment (p);
		xfree (p);
	  return p2;
  }
	return NULL;
}

TCHAR *cfgfile_subst_path (const TCHAR *path, const TCHAR *subst, const TCHAR *file)
{
	TCHAR *s = cfgfile_subst_path2 (path, subst, file);
	if (s)
		return s;
	s = target_expand_environment (file);
	return s;
}

static bool isdefault (const TCHAR *s)
{
  TCHAR tmp[MAX_DPATH];
  if (!default_file || uaeconfig)
  	return false;
  zfile_fseek (default_file, 0, SEEK_SET);
  while (zfile_fgets (tmp, sizeof tmp / sizeof (TCHAR), default_file)) {
  	if (tmp[0] && tmp[_tcslen (tmp) - 1] == '\n')
  		tmp[_tcslen (tmp) - 1] = 0;
  	if (!_tcscmp (tmp, s))
	    return true;
  }
  return false;
}

static size_t cfg_write (const void *b, struct zfile *z)
{
  size_t v;
  TCHAR lf = 10;
  v = zfile_fwrite (b, _tcslen ((TCHAR*)b), sizeof (TCHAR), z);
  zfile_fwrite (&lf, 1, 1, z);
  return v;
}

#define UTF8NAME _T(".utf8")

static void cfg_dowrite (struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *value, int d, int target)
{
  TCHAR tmp[CONFIG_BLEN], tmpext[CONFIG_BLEN];
	const TCHAR *optionp;

	if (value == NULL)
		return;
	if (optionext) {
		_tcscpy (tmpext, option);
		_tcscat (tmpext, optionext);
		optionp = tmpext;
	} else {
		optionp = option;
	}

  if (target)
  	_stprintf (tmp, _T("%s.%s=%s"), TARGET_NAME, optionp, value);
  else
  	_stprintf (tmp, _T("%s=%s"), optionp, value);
  if (d && isdefault (tmp))
  	return;
  cfg_write (tmp, f);
}

static void cfg_dowrite (struct zfile *f, const TCHAR *option, const TCHAR *value, int d, int target)
{
	cfg_dowrite (f, option, NULL, value, d, target);
}
void cfgfile_write_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 0, 0);
}
void cfgfile_dwrite_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 1, 0);
}
void cfgfile_dwrite_bool (struct zfile *f, const TCHAR *option, const TCHAR *optionext, bool b)
{
	cfg_dowrite (f, option, optionext, b ? _T("true") : _T("false"), 1, 0);
}
void cfgfile_dwrite_bool (struct zfile *f, const TCHAR *option, int b)
{
	cfgfile_dwrite_bool (f, option, b != 0);
}
void cfgfile_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 0, 0);
}
void cfgfile_write_str (struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *value)
{
	cfg_dowrite (f, option, optionext, value, 0, 0);
}
void cfgfile_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 1, 0);
}
void cfgfile_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *value)
{
	cfg_dowrite (f, option, optionext, value, 1, 0);
}

void cfgfile_target_write_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 0, 1);
}
void cfgfile_target_dwrite_bool (struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite (f, option, b ? _T("true") : _T("false"), 1, 1);
}
void cfgfile_target_write_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 0, 1);
}
void cfgfile_target_dwrite_str (struct zfile *f, const TCHAR *option, const TCHAR *value)
{
  cfg_dowrite (f, option, value, 1, 1);
}

void cfgfile_write_ext (struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN], tmp2[CONFIG_BLEN];

	if (optionext) {
		_tcscpy (tmp2, option);
		_tcscat (tmp2, optionext);
	}
	va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite (f, optionext ? tmp2 : option, tmp, 0, 0);
	va_end (parms);
}
void cfgfile_write (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 0, 0);
  va_end (parms);
}
void cfgfile_dwrite_ext (struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN], tmp2[CONFIG_BLEN];

	if (optionext) {
		_tcscpy (tmp2, option);
		_tcscat (tmp2, optionext);
	}
	va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite (f, optionext ? tmp2 : option, tmp, 1, 0);
	va_end (parms);
}
void cfgfile_dwrite (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 1, 0);
  va_end (parms);
}
void cfgfile_target_write (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 0, 1);
  va_end (parms);
}
void cfgfile_target_dwrite (struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
  va_list parms;
  TCHAR tmp[CONFIG_BLEN];

  va_start (parms, format);
	_vsntprintf (tmp, CONFIG_BLEN, format, parms);
  cfg_dowrite (f, option, tmp, 1, 1);
  va_end (parms);
}

static void cfgfile_write_rom (struct zfile *f, const TCHAR *path, const TCHAR *romfile, const TCHAR *name)
{
	TCHAR *str = cfgfile_subst_path (path, UNEXPANDED, romfile);
	cfgfile_write_str (f, name, str);
	struct zfile *zf = zfile_fopen (str, _T("rb"), ZFD_ALL);
	if (zf) {
		struct romdata *rd = getromdatabyzfile (zf);
		if (rd) {
			TCHAR name2[MAX_DPATH], str2[MAX_DPATH];
			_tcscpy (name2, name);
			_tcscat (name2, _T("_id"));
			_stprintf (str2, _T("%08X,%s"), rd->crc32, rd->name);
			cfgfile_write_str (f, name2, str2);
		}
		zfile_fclose (zf);
	}
	xfree (str);

}

static void cfgfile_write_path (struct zfile *f, const TCHAR *path, const TCHAR *option, const TCHAR *value)
{
  	TCHAR *s = cfgfile_subst_path (path, UNEXPANDED, value);
  	cfgfile_write_str (f, option, s);
  	xfree (s);
}

static void write_filesys_config (struct uae_prefs *p, const TCHAR *unexpanded,
				  const TCHAR *default_path, struct zfile *f)
{
  int i;
  TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH], tmp3[MAX_DPATH];
  const TCHAR *hdcontrollers[] = { _T("uae"), 
    _T("ide0"), _T("ide1"), _T("ide2"), _T("ide3"),
	  _T("scsi0"), _T("scsi1"), _T("scsi2"), _T("scsi3"), _T("scsi4"), _T("scsi5"), _T("scsi6"),
	  _T("scsram"), _T("scide") }; /* scsram = smart card sram = pcmcia sram card */

  for (i = 0; i < p->mountitems; i++) {
	  struct uaedev_config_data *uci = &p->mountconfig[i];
		struct uaedev_config_info *ci = &uci->ci;
		TCHAR *str1, *str2, *str1b, *str2b;
    int bp = ci->bootpri;

		str2 = _T("");
		if (ci->rootdir[0] == ':') {
			TCHAR *ptr;
			// separate harddrive names
			str1 = my_strdup (ci->rootdir);
			ptr = _tcschr (str1 + 1, ':');
			if (ptr) {
				*ptr++ = 0;
				str2 = ptr;
				ptr = _tcschr (str2, ',');
				if (ptr)
					*ptr = 0;
			}
		} else {
    	str1 = cfgfile_subst_path (default_path, unexpanded, ci->rootdir);
    }
		str1b = cfgfile_escape (str1, _T(":,"), true);
		str2b = cfgfile_escape (str2, _T(":,"), true);
  	if (ci->type == UAEDEV_DIR) {
	    _stprintf (tmp, _T("%s,%s:%s:%s,%d"), ci->readonly ? _T("ro") : _T("rw"),
  		  ci->devname ? ci->devname : _T(""), ci->volname, str1, bp);
	    cfgfile_write_str (f, _T("filesystem2"), tmp);
			_tcscpy (tmp3, tmp);
	  } else  if (ci->type == UAEDEV_HDF || ci->type == UAEDEV_CD || ci->type == UAEDEV_TAPE) {
	    _stprintf (tmp, _T("%s,%s:%s,%d,%d,%d,%d,%d,%s,%s"),
		    ci->readonly ? _T("ro") : _T("rw"),
				ci->devname ? ci->devname : _T(""), str1,
				ci->sectors, ci->surfaces, ci->reserved, ci->blocksize,
				bp, ci->filesys ? ci->filesys : _T(""), hdcontrollers[ci->controller]);
			_stprintf (tmp3, _T("%s,%s:%s%s%s,%d,%d,%d,%d,%d,%s,%s"),
				ci->readonly ? _T("ro") : _T("rw"),
				ci->devname ? ci->devname : _T(""), str1b, str2b[0] ? _T(":") : _T(""), str2b,
		    ci->sectors, ci->surfaces, ci->reserved, ci->blocksize,
		    bp, ci->filesys ? ci->filesys : _T(""), hdcontrollers[ci->controller]);
			if (ci->highcyl) {
				TCHAR *s = tmp + _tcslen (tmp);
				TCHAR *s2 = s;
				_stprintf (s2, _T(",%d"), ci->highcyl);
				if (ci->pcyls && ci->pheads && ci->psecs) {
					TCHAR *s = tmp + _tcslen (tmp);
					_stprintf (s, _T(",%d/%d/%d"), ci->pcyls, ci->pheads, ci->psecs);
				}
				_tcscat (tmp3, s2);
			}
			if (ci->type == UAEDEV_HDF)
	      cfgfile_write_str (f, _T("hardfile2"), tmp);
	  }
	  _stprintf (tmp2, _T("uaehf%d"), i);
		if (ci->type == UAEDEV_CD) {
			cfgfile_write (f, tmp2, _T("cd%d,%s"), ci->device_emu_unit, tmp);
		} else if (ci->type == UAEDEV_TAPE) {
			cfgfile_write (f, tmp2, _T("tape%d,%s"), ci->device_emu_unit, tmp);
		} else {
			cfgfile_write (f, tmp2, _T("%s,%s"), ci->type == UAEDEV_HDF ? _T("hdf") : _T("dir"), tmp3);
		}
		xfree (str1b);
		xfree (str2b);
		xfree (str1);
  }
}

static void write_compatibility_cpu(struct zfile *f, struct uae_prefs *p)
{
  TCHAR tmp[100];
  int model;

  model = p->cpu_model;
  if (model == 68030)
  	model = 68020;
  if (model == 68060)
  	model = 68040;
  if (p->address_space_24 && model == 68020)
  	_tcscpy (tmp, _T("68ec020"));
  else
	  _stprintf(tmp, _T("%d"), model);
  if (model == 68020 && (p->fpu_model == 68881 || p->fpu_model == 68882)) 
  	_tcscat(tmp, _T("/68881"));
  cfgfile_write (f, _T("cpu_type"), tmp);
}

static void write_resolution (struct zfile *f, const TCHAR *ws, const TCHAR *hs, struct wh *wh)
{
	cfgfile_write (f, ws, _T("%d"), wh->width);
	cfgfile_write (f, hs, _T("%d"), wh->height);
}

void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type)
{
  struct strlist *sl;
  TCHAR tmp[MAX_DPATH];
  int i;

  cfgfile_write_str (f, _T("config_description"), p->description);
  cfgfile_write_bool (f, _T("config_hardware"), type & CONFIG_TYPE_HARDWARE);
  cfgfile_write_bool (f, _T("config_host"), !!(type & CONFIG_TYPE_HOST));
  if (p->info[0])
  	cfgfile_write (f, _T("config_info"), p->info);
  cfgfile_write (f, _T("config_version"), _T("%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
   
  for (sl = p->all_lines; sl; sl = sl->next) {
  	if (sl->unknown) {
			if (sl->option)
	      cfgfile_write_str (f, sl->option, sl->value);
    }
  }

  _stprintf (tmp, _T("%s.rom_path"), TARGET_NAME);
  cfgfile_write_str (f, tmp, p->path_rom);
  _stprintf (tmp, _T("%s.floppy_path"), TARGET_NAME);
  cfgfile_write_str (f, tmp, p->path_floppy);
  _stprintf (tmp, _T("%s.hardfile_path"), TARGET_NAME);
  cfgfile_write_str (f, tmp, p->path_hardfile);
	_stprintf (tmp, _T("%s.cd_path"), TARGET_NAME);
	cfgfile_write_str (f, tmp, p->path_cd);

  cfg_write (_T("; host-specific"), f);

  target_save_options (f, p);

  cfg_write (_T("; common"), f);

  cfgfile_write_str (f, _T("use_gui"), guimode1[p->start_gui]);

	cfgfile_write_rom (f, p->path_rom, p->romfile, _T("kickstart_rom_file"));
  cfgfile_write_rom (f, p->path_rom, p->romextfile, _T("kickstart_ext_rom_file"));

	cfgfile_write_str (f, _T("flash_file"), p->flashfile);

  p->nr_floppies = 4;
  for (i = 0; i < 4; i++) {
    _stprintf (tmp, _T("floppy%d"), i);
    cfgfile_write_path(f, p->path_floppy, tmp, p->floppyslots[i].df);
		_stprintf (tmp, _T("floppy%dwp"), i);
		cfgfile_dwrite_bool (f, tmp, p->floppyslots[i].forcedwriteprotect);
  	_stprintf (tmp, _T("floppy%dtype"), i);
  	cfgfile_dwrite (f, tmp, _T("%d"), p->floppyslots[i].dfxtype);
  	if (p->floppyslots[i].dfxtype < 0 && p->nr_floppies > i)
	    p->nr_floppies = i;
  }

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		if (p->cdslots[i].name[0] || p->cdslots[i].inuse) {
			TCHAR tmp2[MAX_DPATH];
			_stprintf (tmp, _T("cdimage%d"), i);
			TCHAR *s = cfgfile_subst_path (p->path_cd, UNEXPANDED, p->cdslots[i].name);
			_tcscpy (tmp2, s);
			xfree (s);
			if (p->cdslots[i].type != SCSI_UNIT_DEFAULT || _tcschr (p->cdslots[i].name, ',') || p->cdslots[i].delayed) {
				_tcscat (tmp2, _T(","));
				if (p->cdslots[i].delayed) {
					_tcscat (tmp2, _T("delay"));
					_tcscat (tmp2, _T(":"));
				}
				if (p->cdslots[i].type != SCSI_UNIT_DEFAULT) {
					_tcscat (tmp2, cdmodes[p->cdslots[i].type + 1]);
				}
			}
			cfgfile_write_str (f, tmp, tmp2);
		}
	}

  cfgfile_write (f, _T("nr_floppies"), _T("%d"), p->nr_floppies);
  cfgfile_write (f, _T("floppy_speed"), _T("%d"), p->floppy_speed);

  cfgfile_write_str (f, _T("sound_output"), soundmode1[p->produce_sound]);
  cfgfile_write_str (f, _T("sound_channels"), stereomode[p->sound_stereo]);
  cfgfile_write (f, _T("sound_stereo_separation"), _T("%d"), p->sound_stereo_separation);
  cfgfile_write (f, _T("sound_stereo_mixing_delay"), _T("%d"), p->sound_mixed_stereo_delay >= 0 ? p->sound_mixed_stereo_delay : 0);
  cfgfile_write (f, _T("sound_frequency"), _T("%d"), p->sound_freq);
  cfgfile_write_str (f, _T("sound_interpol"), interpolmode[p->sound_interpol]);
  cfgfile_write_str (f, _T("sound_filter"), soundfiltermode1[p->sound_filter]);
  cfgfile_write_str (f, _T("sound_filter_type"), soundfiltermode2[p->sound_filter_type]);
	if (p->sound_volume_cd >= 0)
		cfgfile_write (f, _T("sound_volume_cd"), _T("%d"), p->sound_volume_cd);

  cfgfile_write (f, _T("cachesize"), _T("%d"), p->cachesize);

	for (i = 0; i < MAX_JPORTS; i++) {
		struct jport *jp = &p->jports[i];
		int v = jp->id;
		TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		if (v == JPORT_CUSTOM) {
			_tcscpy (tmp2, _T("custom"));
		} else if (v == JPORT_NONE) {
			_tcscpy (tmp2, _T("none"));
		} else if (v < JSEM_JOYS) {
			_stprintf (tmp2, _T("kbd%d"), v + 1);
		} else if (v < JSEM_MICE) {
			_stprintf (tmp2, _T("joy%d"), v - JSEM_JOYS);
		} else {
			_tcscpy (tmp2, _T("mouse"));
			if (v - JSEM_MICE > 0)
				_stprintf (tmp2, _T("mouse%d"), v - JSEM_MICE);
		}
		if (i < 2 || jp->id >= 0) {
			_stprintf (tmp1, _T("joyport%d"), i);
			cfgfile_write (f, tmp1, tmp2);
			_stprintf (tmp1, _T("joyport%dautofire"), i);
			cfgfile_write (f, tmp1, joyaf[jp->autofire]);
			if (i < 2 && jp->mode > 0) {
				_stprintf (tmp1, _T("joyport%dmode"), i);
				cfgfile_write (f, tmp1, joyportmodes[jp->mode]);
			}
			if (jp->name[0]) {
				_stprintf (tmp1, _T("joyportfriendlyname%d"), i);
				cfgfile_write (f, tmp1, jp->name);
			}
			if (jp->configname[0]) {
				_stprintf (tmp1, _T("joyportname%d"), i);
				cfgfile_write (f, tmp1, jp->configname);
			}
			if (jp->nokeyboardoverride) {
				_stprintf (tmp1, _T("joyport%dkeyboardoverride"), i);
				cfgfile_write_bool (f, tmp1, !jp->nokeyboardoverride);
      }
		}
	}

	cfgfile_write_bool (f, _T("bsdsocket_emu"), p->socket_emu);

  cfgfile_write_bool (f, _T("synchronize_clock"), p->tod_hack);
  cfgfile_dwrite_str (f, _T("absolute_mouse"), abspointers[p->input_tablet]);

  cfgfile_write (f, _T("gfx_framerate"), _T("%d"), p->gfx_framerate);
  write_resolution (f, _T("gfx_width"), _T("gfx_height"), &p->gfx_size); /* compatibility with old versions */
	write_resolution (f, _T("gfx_width_windowed"), _T("gfx_height_windowed"), &p->gfx_size_win);
	write_resolution (f, _T("gfx_width_fullscreen"), _T("gfx_height_fullscreen"), &p->gfx_size_fs);

#ifdef RASPBERRY
    cfgfile_write (f, _T("gfx_correct_aspect"), _T("%d"), p->gfx_correct_aspect);
    cfgfile_write (f, _T("gfx_fullscreen_ratio"), _T("%d"), p->gfx_fullscreen_ratio);
    cfgfile_write (f, _T("kbd_led_num"), _T("%d"), p->kbd_led_num);
    cfgfile_write (f, _T("kbd_led_scr"), _T("%d"), p->kbd_led_scr);
    cfgfile_write (f, _T("kbd_led_cap"), _T("%d"), p->kbd_led_cap);
#endif

  cfgfile_write_bool (f, _T("gfx_lores"), p->gfx_resolution == 0);
  cfgfile_write_str (f, _T("gfx_resolution"), lorestype1[p->gfx_resolution]);

  cfgfile_write_bool (f, _T("immediate_blits"), p->immediate_blits);
	cfgfile_dwrite_str (f, _T("waiting_blits"), waitblits[p->waiting_blits]);
  cfgfile_write_bool (f, _T("fast_copper"), p->fast_copper);
  cfgfile_write_bool (f, _T("ntsc"), p->ntscmode);

  cfgfile_dwrite_bool (f, _T("show_leds"), p->leds_on_screen);
  if (p->chipset_mask & CSMASK_AGA)
  	cfgfile_write (f, _T("chipset"), _T("aga"));
  else if ((p->chipset_mask & CSMASK_ECS_AGNUS) && (p->chipset_mask & CSMASK_ECS_DENISE))
  	cfgfile_write (f, _T("chipset"), _T("ecs"));
  else if (p->chipset_mask & CSMASK_ECS_AGNUS)
  	cfgfile_write (f, _T("chipset"), _T("ecs_agnus"));
  else if (p->chipset_mask & CSMASK_ECS_DENISE)
  	cfgfile_write (f, _T("chipset"), _T("ecs_denise"));
  else
  	cfgfile_write (f, _T("chipset"), _T("ocs"));
	if (p->chipset_refreshrate > 0)
		cfgfile_write (f, _T("chipset_refreshrate"), _T("%f"), p->chipset_refreshrate);

  cfgfile_write_str (f, _T("collision_level"), collmode[p->collision_level]);

	cfgfile_dwrite_bool (f, _T("cd32cd"), p->cs_cd32cd);
	cfgfile_dwrite_bool (f, _T("cd32c2p"), p->cs_cd32c2p);
	cfgfile_dwrite_bool (f, _T("cd32nvram"), p->cs_cd32nvram);

  cfgfile_write (f, _T("fastmem_size"), _T("%d"), p->fastmem_size / 0x100000);
  cfgfile_write (f, _T("z3mem_size"), _T("%d"), p->z3fastmem_size / 0x100000);
  cfgfile_write (f, _T("z3mem_start"), _T("0x%x"), p->z3fastmem_start);
  cfgfile_write (f, _T("bogomem_size"), _T("%d"), p->bogomem_size / 0x40000);
  cfgfile_write (f, _T("gfxcard_size"), _T("%d"), p->rtgmem_size / 0x100000);
	cfgfile_write_str (f, _T("gfxcard_type"), rtgtype[p->rtgmem_type]);
  cfgfile_write (f, _T("chipmem_size"), _T("%d"), p->chipmem_size == 0x20000 ? -1 : (p->chipmem_size == 0x40000 ? 0 : p->chipmem_size / 0x80000));

  if (p->m68k_speed > 0) {
  	cfgfile_write (f, _T("finegrain_cpu_speed"), _T("%d"), p->m68k_speed);
	} else {
  	cfgfile_write_str (f, _T("cpu_speed"), p->m68k_speed < 0 ? _T("max") : _T("real"));
  }

  /* do not reorder start */
  write_compatibility_cpu(f, p);
  cfgfile_write (f, _T("cpu_model"), _T("%d"), p->cpu_model);
  if (p->fpu_model)
  	cfgfile_write (f, _T("fpu_model"), _T("%d"), p->fpu_model);
  cfgfile_write_bool (f, _T("cpu_compatible"), p->cpu_compatible);
  cfgfile_write_bool (f, _T("cpu_24bit_addressing"), p->address_space_24);
  /* do not reorder end */

  cfgfile_write (f, _T("rtg_modes"), _T("0x%x"), p->picasso96_modeflags);

#ifdef FILESYS
  write_filesys_config (p, UNEXPANDED, p->path_hardfile, f);
	cfgfile_dwrite (f, _T("filesys_max_size"), _T("%d"), p->filesys_limit);
	cfgfile_dwrite (f, _T("filesys_max_name_length"), _T("%d"), p->filesys_max_name);
#endif
  write_inputdevice_config (p, f);
}

int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, bool numbercheck)
{
  if (name != NULL && _tcscmp (option, name) != 0)
  	return 0;
	if (strcasecmp (value, _T("yes")) == 0 || strcasecmp (value, _T("y")) == 0
		|| strcasecmp (value, _T("true")) == 0 || strcasecmp (value, _T("t")) == 0)
  	*location = 1;
	else if (strcasecmp (value, _T("no")) == 0 || strcasecmp (value, _T("n")) == 0
		|| strcasecmp (value, _T("false")) == 0 || strcasecmp (value, _T("f")) == 0
		|| (numbercheck && strcasecmp (value, _T("0")) == 0))
	  *location = 0;
  else {
	  write_log (_T("Option `%s' requires a value of either `yes' or `no' (was '%s').\n"), option, value);
	  return -1;
  }
  return 1;
}

int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location)
{
	return cfgfile_yesno (option, value, name, location, true);
}
int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location, bool numbercheck)
{
	int val;
	int ret = cfgfile_yesno (option, value, name, &val, numbercheck);
	if (ret == 0)
		return 0;
	if (ret < 0)
		*location = false;
	else
		*location = val != 0;
	return 1;
}
int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location)
{
	return cfgfile_yesno (option, value, name, location, true);
}

int cfgfile_doubleval (const TCHAR *option, const TCHAR *value, const TCHAR *name, double *location)
{
	int base = 10;
	TCHAR *endptr;
	if (name != NULL && _tcscmp (option, name) != 0)
		return 0;
	*location = _tcstod (value, &endptr);
	return 1;
}

int cfgfile_floatval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, float *location)
{
	int base = 10;
	TCHAR *endptr;
	if (name == NULL)
		return 0;
	if (nameext) {
		TCHAR tmp[MAX_DPATH];
		_tcscpy (tmp, name);
		_tcscat (tmp, nameext);
		if (_tcscmp (tmp, option) != 0)
			return 0;
	} else {
		if (_tcscmp (option, name) != 0)
			return 0;
	}
	*location = (float)_tcstod (value, &endptr);
	return 1;
}
int cfgfile_floatval (const TCHAR *option, const TCHAR *value, const TCHAR *name, float *location)
{
	return cfgfile_floatval (option, value, name, NULL, location);
}

int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, unsigned int *location, int scale)
{
  int base = 10;
  TCHAR *endptr;
	TCHAR tmp[MAX_DPATH];

	if (name == NULL)
  	return 0;
	if (nameext) {
		_tcscpy (tmp, name);
		_tcscat (tmp, nameext);
		if (_tcscmp (tmp, option) != 0)
			return 0;
	} else {
		if (_tcscmp (option, name) != 0)
			return 0;
	}
  /* I guess octal isn't popular enough to worry about here...  */
  if (value[0] == '0' && _totupper(value[1]) == 'X')
  	value += 2, base = 16;
  *location = _tcstol (value, &endptr, base) * scale;

  if (*endptr != '\0' || *value == '\0') {
		if (strcasecmp (value, _T("false")) == 0 || strcasecmp (value, _T("no")) == 0) {
			*location = 0;
			return 1;
		}
		if (strcasecmp (value, _T("true")) == 0 || strcasecmp (value, _T("yes")) == 0) {
			*location = 1;
			return 1;
  	}
		write_log (_T("Option '%s' requires a numeric argument but got '%s'\n"), nameext ? tmp : option, value);
  	return -1;
  }
  return 1;
}
int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, unsigned int *location, int scale)
{
	return cfgfile_intval (option, value, name, NULL, location, scale);
}
int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval (option, value, name, NULL, &v, scale);
	if (!r)
		return 0;
	*location = (int)v;
	return r;
}
int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, int *location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval (option, value, name, nameext, &v, scale);
	if (!r)
		return 0;
	*location = (int)v;
	return r;
}

int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, int *location, const TCHAR *table[], int more)
{
  int val;
	TCHAR tmp[MAX_DPATH];
	if (name == NULL)
  	return 0;
	if (nameext) {
		_tcscpy (tmp, name);
		_tcscat (tmp, nameext);
		if (_tcscmp (tmp, option) != 0)
			return 0;
	} else {
		if (_tcscmp (option, name) != 0)
			return 0;
	}
  val = match_string (table, value);
  if (val == -1) {
  	if (more)
	    return 0;
		if (!strcasecmp (value, _T("yes")) || !strcasecmp (value, _T("true"))) {
			val = 1;
		} else if  (!strcasecmp (value, _T("no")) || !strcasecmp (value, _T("false"))) {
			val = 0;
		} else {
			write_log (_T("Unknown value ('%s') for option '%s'.\n"), value, nameext ? tmp : option);
    	return -1;
		}
  }
  *location = val;
  return 1;
}
int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more)
{
	return cfgfile_strval (option, value, name, NULL, location, table, more);
}

int cfgfile_strboolval (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location, const TCHAR *table[], int more)
{
	int locationint;
	if (!cfgfile_strval (option, value, name, &locationint, table, more))
		return 0;
	*location = locationint != 0;
	return 1;
}

int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
  if (_tcscmp (option, name) != 0)
  	return 0;
  _tcsncpy (location, value, maxsz - 1);
  location[maxsz - 1] = '\0';
  return 1;
}
int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, TCHAR *location, int maxsz)
{
	if (nameext) {
		TCHAR tmp[MAX_DPATH];
		_tcscpy (tmp, name);
		_tcscat (tmp, nameext);
		if (_tcscmp (tmp, option) != 0)
			return 0;
	} else {
		if (_tcscmp (option, name) != 0)
			return 0;
	}
	_tcsncpy (location, value, maxsz - 1);
	location[maxsz - 1] = '\0';
	return 1;
}

static int cfgfile_path (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	if (!cfgfile_string (option, value, name, location, maxsz))
		return 0;
	TCHAR *s = target_expand_environment (location);
	_tcsncpy (location, s, maxsz - 1);
	location[maxsz - 1] = 0;
	xfree (s);
	return 1;
}

int cfgfile_rom (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	TCHAR id[MAX_DPATH];
	if (!cfgfile_string (option, value, name, id, sizeof id / sizeof (TCHAR)))
		return 0;
	TCHAR *p = _tcschr (id, ',');
	if (p) {
		TCHAR *endptr, tmp;
		*p = 0;
		tmp = id[4];
		id[4] = 0;
		uae_u32 crc32 = _tcstol (id, &endptr, 16) << 16;
		id[4] = tmp;
		crc32 |= _tcstol (id + 4, &endptr, 16);
		struct romdata *rd = getromdatabycrc (crc32, true);
		if (rd) {
			struct romdata *rd2 = getromdatabyid (rd->id);
			if (rd->group == 0 && rd2 == rd) {
				if (zfile_exists (location))
					return 1;
			}
			if (rd->group && rd2)
				rd = rd2;
			struct romlist *rl = getromlistbyromdata (rd);
			if (rl) {
				write_log (_T("%s: %s -> %s\n"), name, location, rl->path);
				_tcsncpy (location, rl->path, maxsz);
			}
		}
	}
	return 1;
}

static int getintval (TCHAR **p, int *result, int delim)
{
  TCHAR *value = *p;
  int base = 10;
  TCHAR *endptr;
  TCHAR *p2 = _tcschr (*p, delim);

  if (p2 == 0)
  	return 0;

  *p2++ = '\0';

  if (value[0] == '0' && _totupper (value[1]) == 'X')
  	value += 2, base = 16;
  *result = _tcstol (value, &endptr, base);
  *p = p2;

  if (*endptr != '\0' || *value == '\0')
  	return 0;

  return 1;
}

static int getintval2 (TCHAR **p, int *result, int delim)
{
  TCHAR *value = *p;
  int base = 10;
  TCHAR *endptr;
  TCHAR *p2 = _tcschr (*p, delim);

  if (p2 == 0) {
	  p2 = _tcschr (*p, 0);
	  if (p2 == 0) {
	    *p = 0;
	    return 0;
  	}
  }
  if (*p2 != 0)
  	*p2++ = '\0';

  if (value[0] == '0' && _totupper (value[1]) == 'X')
  	value += 2, base = 16;
  *result = _tcstol (value, &endptr, base);
  *p = p2;

  if (*endptr != '\0' || *value == '\0') {
	  *p = 0;
	  return 0;
  }

  return 1;
}

static void set_chipset_mask (struct uae_prefs *p, int val)
{
  p->chipset_mask = (val == 0 ? 0
    : val == 1 ? CSMASK_ECS_AGNUS
    : val == 2 ? CSMASK_ECS_DENISE
    : val == 3 ? CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS
    : CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS);
}

static int cfgfile_parse_host (struct uae_prefs *p, TCHAR *option, TCHAR *value)
{
	int i, v;
  bool vb;
  TCHAR *section = 0;
  TCHAR *tmpp;
  TCHAR tmpbuf[CONFIG_BLEN];

	if (_tcsncmp (option, _T("input."), 6) == 0) {
    read_inputdevice_config (p, option, value);
    return 1;
  }

  for (tmpp = option; *tmpp != '\0'; tmpp++)
  	if (_istupper (*tmpp))
	    *tmpp = _totlower (*tmpp);
  tmpp = _tcschr (option, '.');
  if (tmpp) {
    section = option;
    option = tmpp + 1;
    *tmpp = '\0';
    if (_tcscmp (section, TARGET_NAME) == 0) {
	    /* We special case the various path options here.  */
      if (cfgfile_path (option, value, _T("rom_path"), p->path_rom, sizeof p->path_rom / sizeof (TCHAR))
  		|| cfgfile_path (option, value, _T("floppy_path"), p->path_floppy, sizeof p->path_floppy / sizeof (TCHAR))
  		|| cfgfile_path (option, value, _T("cd_path"), p->path_cd, sizeof p->path_cd / sizeof (TCHAR))
  		|| cfgfile_path (option, value, _T("hardfile_path"), p->path_hardfile, sizeof p->path_hardfile / sizeof (TCHAR)))
	    	return 1;
  	  return target_parse_option (p, option, value);
  	}
  	return 0;
  }

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		TCHAR tmp[20];
		_stprintf (tmp, _T("cdimage%d"), i);
		if (!_tcsicmp (option, tmp)) {
			if (!_tcsicmp (value, _T("autodetect"))) {
				p->cdslots[i].type = SCSI_UNIT_DEFAULT;
				p->cdslots[i].inuse = true;
				p->cdslots[i].name[0] = 0;
			} else {
				p->cdslots[i].delayed = false;
				TCHAR *next = _tcsrchr (value, ',');
				int type = SCSI_UNIT_DEFAULT;
				int mode = 0;
				int unitnum = 0;
				for (;;) {
					if (!next)
						break;
					*next++ = 0;
					TCHAR *next2 = _tcschr (next, ':');
					if (next2)
						*next2++ = 0;
					int tmpval = 0;
					if (!_tcsicmp (next, _T("delay"))) {
						p->cdslots[i].delayed = true;
						next = next2;
						if (!next)
							break;
						next2 = _tcschr (next, ':');
						if (next2)
							*next2++ = 0;
					}
					type = match_string (cdmodes, next);
					if (type < 0)
						type = SCSI_UNIT_DEFAULT;
					else
						type--;
					next = next2;
					if (!next)
						break;
					next2 = _tcschr (next, ':');
					if (next2)
						*next2++ = 0;
					mode = match_string (cdconmodes, next);
					if (mode < 0)
						mode = 0;
					next = next2;
					if (!next)
						break;
					next2 = _tcschr (next, ':');
					if (next2)
						*next2++ = 0;
					cfgfile_intval (option, next, tmp, &unitnum, 1);
				}
				if (_tcslen (value) > 0) {
					TCHAR *s = cfgfile_subst_path (UNEXPANDED, p->path_cd, value);
					_tcsncpy (p->cdslots[i].name, s, sizeof p->cdslots[i].name / sizeof (TCHAR));
					xfree (s);
				}
				p->cdslots[i].name[sizeof p->cdslots[i].name - 1] = 0;
				p->cdslots[i].inuse = true;
				p->cdslots[i].type = type;
			}
			// disable all following units
			i++;
			while (i < MAX_TOTAL_SCSI_DEVICES) {
				p->cdslots[i].type = SCSI_UNIT_DISABLED;
				i++;
			}
			return 1;
		}
	}

  if (cfgfile_intval (option, value, _T("sound_frequency"), &p->sound_freq, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_cd"), &p->sound_volume_cd, 1)
	  || cfgfile_intval (option, value, _T("sound_stereo_separation"), &p->sound_stereo_separation, 1)
	  || cfgfile_intval (option, value, _T("sound_stereo_mixing_delay"), &p->sound_mixed_stereo_delay, 1)

	  || cfgfile_intval (option, value, _T("gfx_framerate"), &p->gfx_framerate, 1)

		|| cfgfile_intval (option, value, _T("filesys_max_size"), &p->filesys_limit, 1)
		|| cfgfile_intval (option, value, _T("filesys_max_name_length"), &p->filesys_max_name, 1)
  )
	  return 1;

#ifdef RASPBERRY
    if (cfgfile_intval (option, value, "gfx_correct_aspect", &p->gfx_correct_aspect, 1))
        return 1;

    if (cfgfile_intval (option, value, "gfx_fullscreen_ratio", &p->gfx_fullscreen_ratio, 1))
        return 1;
    if (cfgfile_intval (option, value, "kbd_led_num", &p->kbd_led_num, 1))
        return 1;
    if (cfgfile_intval (option, value, "kbd_led_scr", &p->kbd_led_scr, 1))
        return 1;
    if (cfgfile_intval (option, value, "kbd_led_cap", &p->kbd_led_cap, 1))
        return 1;
#endif

	if (cfgfile_string (option, value, _T("config_info"), p->info, sizeof p->info / sizeof (TCHAR))
	  || cfgfile_string (option, value, _T("config_description"), p->description, sizeof p->description / sizeof (TCHAR)))
	  return 1;

	if (
		cfgfile_yesno (option, value, _T("floppy0wp"), &p->floppyslots[0].forcedwriteprotect)
		|| cfgfile_yesno (option, value, _T("floppy1wp"), &p->floppyslots[1].forcedwriteprotect)
		|| cfgfile_yesno (option, value, _T("floppy2wp"), &p->floppyslots[2].forcedwriteprotect)
		|| cfgfile_yesno (option, value, _T("floppy3wp"), &p->floppyslots[3].forcedwriteprotect)
    || cfgfile_yesno (option, value, _T("bsdsocket_emu"), &p->socket_emu))
	  return 1;

  if (cfgfile_strval (option, value, _T("sound_output"), &p->produce_sound, soundmode1, 1)
	  || cfgfile_strval (option, value, _T("sound_output"), &p->produce_sound, soundmode2, 0)
	  || cfgfile_strval (option, value, _T("sound_interpol"), &p->sound_interpol, interpolmode, 0)
	  || cfgfile_strval (option, value, _T("sound_filter"), &p->sound_filter, soundfiltermode1, 0)
	  || cfgfile_strval (option, value, _T("sound_filter_type"), &p->sound_filter_type, soundfiltermode2, 0)
	  || cfgfile_strboolval (option, value, _T("use_gui"), &p->start_gui, guimode1, 1)
	  || cfgfile_strboolval (option, value, _T("use_gui"), &p->start_gui, guimode2, 1)
	  || cfgfile_strboolval (option, value, _T("use_gui"), &p->start_gui, guimode3, 0)
	  || cfgfile_strval (option, value, _T("gfx_resolution"), &p->gfx_resolution, lorestype1, 0)
	  || cfgfile_strval (option, value, _T("gfx_lores"), &p->gfx_resolution, lorestype2, 0)
	  || cfgfile_strval (option, value, _T("absolute_mouse"), &p->input_tablet, abspointers, 0))
    return 1;
	
	if (_tcscmp (option, _T("gfx_width_windowed")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_size_win.width = 0;
			p->gfx_size_win.height = 0;
		} else {
			cfgfile_intval (option, value, _T("gfx_width_windowed"), &p->gfx_size_win.width, 1);
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_height_windowed")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_size_win.width = 0;
			p->gfx_size_win.height = 0;
		} else {
			cfgfile_intval (option, value, _T("gfx_height_windowed"), &p->gfx_size_win.height, 1);
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_width_fullscreen")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_size_fs.width = 0;
			p->gfx_size_fs.height = 0;
		} else {
			cfgfile_intval (option, value, _T("gfx_width_fullscreen"), &p->gfx_size_fs.width, 1);
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_height_fullscreen")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_size_fs.width = 0;
			p->gfx_size_fs.height = 0;
		} else {
			cfgfile_intval (option, value, _T("gfx_height_fullscreen"), &p->gfx_size_fs.height, 1);
		}
		return 1;
	}

  if(cfgfile_yesno (option, value, _T("show_leds"), &vb)) {
    p->leds_on_screen = vb;
		return 1;
  }

  if (_tcscmp (option, _T("gfx_width")) == 0 || _tcscmp (option, _T("gfx_height")) == 0) {
	  cfgfile_intval (option, value, _T("gfx_width"), &p->gfx_size.width, 1);
	  cfgfile_intval (option, value, _T("gfx_height"), &p->gfx_size.height, 1);
	  return 1;
  }

	if (_tcscmp (option, _T("joyportfriendlyname0")) == 0 || _tcscmp (option, _T("joyportfriendlyname1")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportfriendlyname0")) == 0 ? 0 : 1, -1, 2, true);
		return 1;
	}
	if (_tcscmp (option, _T("joyportfriendlyname2")) == 0 || _tcscmp (option, _T("joyportfriendlyname3")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportfriendlyname2")) == 0 ? 2 : 3, -1, 2, true);
		return 1;
	}
	if (_tcscmp (option, _T("joyportname0")) == 0 || _tcscmp (option, _T("joyportname1")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportname0")) == 0 ? 0 : 1, -1, 1, true);
		return 1;
	}
	if (_tcscmp (option, _T("joyportname2")) == 0 || _tcscmp (option, _T("joyportname3")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyportname2")) == 0 ? 2 : 3, -1, 1, true);
		return 1;
	}
	if (_tcscmp (option, _T("joyport0")) == 0 || _tcscmp (option, _T("joyport1")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyport0")) == 0 ? 0 : 1, -1, 0, true);
		return 1;
	}
	if (_tcscmp (option, _T("joyport2")) == 0 || _tcscmp (option, _T("joyport3")) == 0) {
		inputdevice_joyport_config (p, value, _tcscmp (option, _T("joyport2")) == 0 ? 2 : 3, -1, 0, true);
		return 1;
	}
	if (cfgfile_strval (option, value, _T("joyport0mode"), &p->jports[0].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport1mode"), &p->jports[1].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport2mode"), &p->jports[2].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport3mode"), &p->jports[3].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport0autofire"), &p->jports[0].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport1autofire"), &p->jports[1].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport2autofire"), &p->jports[2].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("joyport3autofire"), &p->jports[3].autofire, joyaf, 0))
		return 1;
	if (cfgfile_yesno (option, value, _T("joyport0keyboardoverride"), &vb)) {
		p->jports[0].nokeyboardoverride = !vb;
		return 1;
	}
	if (cfgfile_yesno (option, value, _T("joyport1keyboardoverride"), &vb)) {
		p->jports[1].nokeyboardoverride = !vb;
		return 1;
	}
	if (cfgfile_yesno (option, value, _T("joyport2keyboardoverride"), &vb)) {
		p->jports[2].nokeyboardoverride = !vb;
		return 1;
	}
	if (cfgfile_yesno (option, value, _T("joyport3keyboardoverride"), &vb)) {
		p->jports[3].nokeyboardoverride = !vb;
		return 1;
	}

  if (cfgfile_intval (option, value, "key_for_menu", &p->key_for_menu, 1))
        return 1;

  if (cfgfile_string (option, value, _T("statefile"), tmpbuf, sizeof (tmpbuf) / sizeof (TCHAR))) {
	  _tcscpy (savestate_fname, tmpbuf);
	  if (zfile_exists (savestate_fname)) {
	    savestate_state = STATE_DORESTORE;
  	} else {
	    int ok = 0;
	    if (savestate_fname[0]) {
    		for (;;) {
  		    TCHAR *p;
  		    if (my_existsdir (savestate_fname)) {
		        ok = 1;
		        break;
  		    }
		      p = _tcsrchr (savestate_fname, '\\');
		      if (!p)
		        p = _tcsrchr (savestate_fname, '/');
  		    if (!p)
		        break;
  		    *p = 0;
    		}
	    }
	    if (!ok) {
    		savestate_fname[0] = 0;
			}
  	}
	  return 1;
  }

  if (cfgfile_strval (option, value, _T("sound_channels"), &p->sound_stereo, stereomode, 1)) {
	  if (p->sound_stereo == SND_NONE) { /* "mixed stereo" compatibility hack */
	    p->sound_stereo = SND_STEREO;
	    p->sound_mixed_stereo_delay = 5;
	    p->sound_stereo_separation = 7;
	  }
	  return 1;
  }

  if (cfgfile_string (option, value, _T("config_version"), tmpbuf, sizeof (tmpbuf) / sizeof (TCHAR))) {
  	TCHAR *tmpp2;
	  tmpp = _tcschr (value, '.');
	  if (tmpp) {
	    *tmpp++ = 0;
	    tmpp2 = tmpp;
	    p->config_version = _tstol (tmpbuf) << 16;
	    tmpp = _tcschr (tmpp, '.');
	    if (tmpp) {
	    	*tmpp++ = 0;
	    	p->config_version |= _tstol (tmpp2) << 8;
	    	p->config_version |= _tstol (tmpp);
	    }
	  }
	  return 1;
    }

    return 0;
}

static struct uaedev_config_data *getuci(struct uae_prefs *p)
{
  if (p->mountitems < MOUNT_CONFIG_SIZE)
  	return &p->mountconfig[p->mountitems++];
  return NULL;
}

struct uaedev_config_data *add_filesys_config (struct uae_prefs *p, int index, struct uaedev_config_info *ci)
{
	struct uaedev_config_data *uci;
  int i;

	if (index < 0 && (ci->type == UAEDEV_DIR || ci->type == UAEDEV_HDF) && ci->devname && _tcslen (ci->devname) > 0) {
		for (i = 0; i < p->mountitems; i++) {
			if (p->mountconfig[i].ci.devname && !_tcscmp (p->mountconfig[i].ci.devname, ci->devname))
				return NULL;
		}
	}
	if (ci->type == UAEDEV_CD) {
		if (ci->controller > HD_CONTROLLER_SCSI6 || ci->controller < HD_CONTROLLER_IDE0)
			return NULL;
	}
  if (index < 0) {
		if (ci->controller != HD_CONTROLLER_UAE) {
			int ctrl = ci->controller;
			for (;;) {
				for (i = 0; i < p->mountitems; i++) {
					if (p->mountconfig[i].ci.controller == ctrl) {
						ctrl++;
						if (ctrl == HD_CONTROLLER_IDE3 + 1 || ctrl == HD_CONTROLLER_SCSI6 + 1)
							return NULL;
					}
				}
				if (i == p->mountitems) {
					ci->controller = ctrl;
					break;
				}
			}
		}
		if (ci->type == UAEDEV_CD) {
			for (i = 0; i < p->mountitems; i++) {
				if (p->mountconfig[i].ci.type == ci->type)
					return NULL;
			}
		}
	  uci = getuci(p);
	  uci->configoffset = -1;
		uci->unitnum = -1;
  } else {
  	uci = &p->mountconfig[index];
  }
  if (!uci)
		return NULL;
	memcpy (&uci->ci, ci, sizeof (struct uaedev_config_info));
	validatedevicename (uci->ci.devname);
	validatevolumename (uci->ci.volname);
	if (!uci->ci.devname[0] && ci->type != UAEDEV_CD && ci->type != UAEDEV_TAPE) {
  	  TCHAR base[32];
	  TCHAR base2[32];
	  int num = 0;
		if (uci->ci.rootdir[0] == 0 && ci->type == UAEDEV_DIR)
		  _tcscpy (base, _T("RDH"));
	  else
		  _tcscpy (base, _T("DH"));
	  _tcscpy (base2, base);
	  for (i = 0; i < p->mountitems; i++) {
		  _stprintf (base2, _T("%s%d"), base, num);
			if (!_tcsicmp(base2, p->mountconfig[i].ci.devname)) {
		    num++;
		    i = -1;
		    continue;
      }
	  }
		_tcscpy (uci->ci.devname, base2);
		validatedevicename (uci->ci.devname);
  }
	if (ci->type == UAEDEV_DIR) {
		TCHAR *s = filesys_createvolname (uci->ci.volname, uci->ci.rootdir, _T("Harddrive"));
		_tcscpy (uci->ci.volname, s);
    xfree (s);
	}
  return uci;
}

static int get_filesys_controller (const TCHAR *hdc)
{
	int hdcv = HD_CONTROLLER_UAE;
  if(_tcslen(hdc) >= 4 && !_tcsncmp(hdc, _T("ide"), 3)) {
  	hdcv = hdc[3] - '0' + HD_CONTROLLER_IDE0;
  	if (hdcv < HD_CONTROLLER_IDE0 || hdcv > HD_CONTROLLER_IDE3)
	    hdcv = 0;
  }
  if(_tcslen(hdc) >= 5 && !_tcsncmp(hdc, _T("scsi"), 4)) {
		hdcv = hdc[4] - '0' + HD_CONTROLLER_SCSI0;
		if (hdcv < HD_CONTROLLER_SCSI0 || hdcv > HD_CONTROLLER_SCSI6)
	    hdcv = 0;
  }
  if (_tcslen (hdc) >= 6 && !_tcsncmp (hdc, _T("scsram"), 6))
		hdcv = HD_CONTROLLER_PCMCIA_SRAM;
	if (_tcslen (hdc) >= 5 && !_tcsncmp (hdc, _T("scide"), 6))
		hdcv = HD_CONTROLLER_PCMCIA_IDE;
	return hdcv;
}

static bool parse_geo (const TCHAR *tname, struct uaedev_config_info *uci, struct hardfiledata *hfd, bool empty)
{
	struct zfile *f;
	int found;
	TCHAR buf[200];
	
	f = zfile_fopen (tname, _T("r"));
	if (!f)
		return false;
	found = hfd == NULL && !empty ? 2 : 0;
	if (found)
		write_log (_T("Geometry file '%s' detected\n"), tname);
	while (zfile_fgets (buf, sizeof buf / sizeof (TCHAR), f)) {
		int v;
		TCHAR *sep;
		
		my_trim (buf);
		if (_tcslen (buf) == 0)
			continue;
		if (buf[0] == '[' && buf[_tcslen (buf) - 1] == ']') {
			if (found > 1) {
				zfile_fclose (f);
				return true;
			}
			found = 0;
			buf[_tcslen (buf) - 1] = 0;
			my_trim (buf + 1);
			if (!_tcsicmp (buf + 1, _T("empty"))) {
				if (empty)
					found = 1;
			} else if (!_tcsicmp (buf + 1, _T("default"))) {
				if (!empty)
					found = 1;
			} else if (hfd) {
				uae_u64 size = _tstoi64 (buf + 1);
				if (size == hfd->virtsize)
					found = 2;
			}
			if (found)
				write_log (_T("Geometry file '%s', entry '%s' detected\n"), tname, buf + 1);
			continue;
		}
		if (!found)
			continue;

		sep =  _tcschr (buf, '=');
		if (!sep)
			continue;
		sep[0] = 0;

		TCHAR *key = my_strdup_trim (buf);
		TCHAR *val = my_strdup_trim (sep + 1);
		if (val[0] == '0' && _totupper (val[1]) == 'X') {
			TCHAR *endptr;
			v = _tcstol (val, &endptr, 16);
		} else {
			v = _tstol (val);
		}
		if (!_tcsicmp (key, _T("surfaces")))
			uci->surfaces = v;
		if (!_tcsicmp (key, _T("sectorspertrack")) || !_tcsicmp (key, _T("blockspertrack")))
			uci->sectors = v;
		if (!_tcsicmp (key, _T("sectorsperblock")))
			uci->sectorsperblock = v;
		if (!_tcsicmp (key, _T("reserved")))
			uci->reserved = v;
		if (!_tcsicmp (key, _T("lowcyl")))
			uci->lowcyl = v;
		if (!_tcsicmp (key, _T("highcyl")) || !_tcsicmp (key, _T("cyl")))
			uci->highcyl = v;
		if (!_tcsicmp (key, _T("blocksize")) || !_tcsicmp (key, _T("sectorsize")))
			uci->blocksize = v;
		if (!_tcsicmp (key, _T("buffers")))
			uci->buffers = v;
		if (!_tcsicmp (key, _T("maxtransfer")))
			uci->maxtransfer = v;
		if (!_tcsicmp (key, _T("interleave")))
			uci->interleave = v;
		if (!_tcsicmp (key, _T("dostype")))
			uci->dostype = v;
		if (!_tcsicmp (key, _T("bufmemtype")))
			uci->bufmemtype = v;
		if (!_tcsicmp (key, _T("stacksize")))
			uci->stacksize = v;
		if (!_tcsicmp (key, _T("mask")))
			uci->mask = v;
		if (!_tcsicmp (key, _T("unit")))
			uci->unit = v;
		if (!_tcsicmp (key, _T("controller")))
			uci->controller = get_filesys_controller (val);
		if (!_tcsicmp (key, _T("flags")))
			uci->flags = v;
		if (!_tcsicmp (key, _T("priority")))
			uci->priority = v;
		if (!_tcsicmp (key, _T("forceload")))
			uci->forceload = v;
		if (!_tcsicmp (key, _T("bootpri"))) {
			if (v < -129)
				v = -129;
			if (v > 127)
				v = 127;
			uci->bootpri = v;
		}
		if (!_tcsicmp (key, _T("filesystem")))
			_tcscpy (uci->filesys, val);
		if (!_tcsicmp (key, _T("device")))
			_tcscpy (uci->devname, val);
		xfree (val);
		xfree (key);
	}
	zfile_fclose (f);
	return false;
}
bool get_hd_geometry (struct uaedev_config_info *uci)
{
	TCHAR tname[MAX_DPATH];

	fetch_configurationpath (tname, sizeof tname / sizeof (TCHAR));
	_tcscat (tname, _T("default.geo"));
	if (zfile_exists (tname)) {
		struct hardfiledata hfd;
		memset (&hfd, 0, sizeof hfd);
		hfd.ci.readonly = true;
		hfd.ci.blocksize = 512;
		if (hdf_open (&hfd, uci->rootdir)) {
			parse_geo (tname, uci, &hfd, false);
			hdf_close (&hfd);
		} else {
			parse_geo (tname, uci, NULL, true);
		}
	}
	if (uci->rootdir[0]) {
		_tcscpy (tname, uci->rootdir);
		_tcscat (tname, _T(".geo"));
		return parse_geo (tname, uci, NULL, false);
	}
	return false;
}

static int cfgfile_parse_partial_newfilesys (struct uae_prefs *p, int nr, int type, const TCHAR *value, int unit, bool uaehfentry)
{
	TCHAR *tmpp;
	TCHAR *name = NULL, *path = NULL;

	// read only harddrive name
	if (!uaehfentry)
		return 0;
	if (type != 1)
		return 0;
	tmpp = getnextentry (&value, ',');
	if (!tmpp)
		return 0;
	xfree (tmpp);
	tmpp = getnextentry (&value, ':');
	if (!tmpp)
		return 0;
	xfree (tmpp);
	name = getnextentry (&value, ':');
	if (name && _tcslen (name) > 0) {
		path = getnextentry (&value, ',');
		if (path && _tcslen (path) > 0) {
			for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
				struct uaedev_config_info *uci = &p->mountconfig[i].ci;
				if (_tcsicmp (uci->rootdir, name) == 0) {
					_tcscat (uci->rootdir, _T(":"));
					_tcscat (uci->rootdir, path);
				}
			}
		}
	}
	xfree (path);
	xfree (name);
	return 1;
}

static int cfgfile_parse_newfilesys (struct uae_prefs *p, int nr, int type, TCHAR *value, int unit, bool uaehfentry)
{
	struct uaedev_config_info uci;
	TCHAR *tmpp = _tcschr (value, ','), *tmpp2;
  TCHAR *str = NULL;
	TCHAR devname[MAX_DPATH], volname[MAX_DPATH];

	devname[0] = volname[0] = 0;
	uci_set_defaults (&uci, false);

  config_newfilesystem = 1;
  if (tmpp == 0)
    goto invalid_fs;

  *tmpp++ = '\0';
  if (strcasecmp (value, _T("ro")) == 0)
		uci.readonly = true;
  else if (strcasecmp (value, _T("rw")) == 0)
		uci.readonly = false;
	else
    goto invalid_fs;

  value = tmpp;
	if (type == 0) {
		uci.type = UAEDEV_DIR;
    tmpp = _tcschr (value, ':');
    if (tmpp == 0)
  		goto empty_fs;
    *tmpp++ = 0;
		_tcscpy (devname, value);
		tmpp2 = tmpp;
    tmpp = _tcschr (tmpp, ':');
    if (tmpp == 0)
  		goto empty_fs;
    *tmpp++ = 0;
		_tcscpy (volname, tmpp2);
		tmpp2 = tmpp;
    tmpp = _tcschr (tmpp, ',');
    if (tmpp == 0)
  		goto empty_fs;
    *tmpp++ = 0;
		_tcscpy (uci.rootdir, tmpp2);
		_tcscpy (uci.volname, volname);
		_tcscpy (uci.devname, devname);
		if (! getintval (&tmpp, &uci.bootpri, 0))
  		goto empty_fs;
	} else if (type == 1 || ((type == 2 || type == 3) && uaehfentry)) {
    tmpp = _tcschr (value, ':');
    if (tmpp == 0)
	    goto invalid_fs;
    *tmpp++ = '\0';
		_tcscpy (devname, value);
		tmpp2 = tmpp;
    tmpp = _tcschr (tmpp, ',');
    if (tmpp == 0)
	    goto invalid_fs;
    *tmpp++ = 0;
		_tcscpy (uci.rootdir, tmpp2);
		if (uci.rootdir[0] != ':')
			get_hd_geometry (&uci);
		_tcscpy (uci.devname, devname);
		if (! getintval (&tmpp, &uci.sectors, ',')
			|| ! getintval (&tmpp, &uci.surfaces, ',')
			|| ! getintval (&tmpp, &uci.reserved, ',')
			|| ! getintval (&tmpp, &uci.blocksize, ','))
	    goto invalid_fs;
		if (getintval2 (&tmpp, &uci.bootpri, ',')) {
			tmpp2 = tmpp;
	    tmpp = _tcschr (tmpp, ',');
	    if (tmpp != 0) {
	      *tmpp++ = 0;
				_tcscpy (uci.filesys, tmpp2);
				TCHAR *tmpp2 = _tcschr (tmpp, ',');
				if (tmpp2)
					*tmpp2++ = 0;
				uci.controller = get_filesys_controller (tmpp);
				if (tmpp2) {
					if (getintval2 (&tmpp2, &uci.highcyl, ',')) {
						getintval (&tmpp2, &uci.pcyls, '/');
						getintval (&tmpp2, &uci.pheads, '/');
						getintval2 (&tmpp2, &uci.psecs, '/');
					}
				}
  		}
    }
		if (type == 2) {
			uci.device_emu_unit = unit;
			uci.blocksize = 2048;
			uci.readonly = true;
			uci.type = UAEDEV_CD;
		} else {
			uci.type = UAEDEV_HDF;
		}
	} else {
		goto invalid_fs;
	}
empty_fs:
	if (uci.rootdir[0]) {
		if (_tcslen (uci.rootdir) > 3 && uci.rootdir[0] == 'H' && uci.rootdir[1] == 'D' && uci.rootdir[2] == '_') {
			memmove (uci.rootdir, uci.rootdir + 2, (_tcslen (uci.rootdir + 2) + 1) * sizeof (TCHAR));
			uci.rootdir[0] = ':';
		}
    str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, uci.rootdir);
		_tcscpy (uci.rootdir, str);
	}
#ifdef FILESYS
	add_filesys_config (p, nr, &uci);
#endif
  xfree (str);
  return 1;

invalid_fs:
	write_log (_T("Invalid filesystem/hardfile/cd specification.\n"));
	return 1;
}

static int cfgfile_parse_filesys (struct uae_prefs *p, const TCHAR *option, TCHAR *value)
{
	int i;

  for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
	  TCHAR tmp[100];
	  _stprintf (tmp, _T("uaehf%d"), i);
	  if (_tcscmp (option, tmp) == 0) {
			for (;;) {
				int  type = -1;
				int unit = -1;
				TCHAR *tmpp = _tcschr (value, ',');
				if (tmpp == NULL)
	        return 1;
				*tmpp++ = 0;
				if (_tcsicmp (value, _T("hdf")) == 0) {
					type = 1;
					cfgfile_parse_partial_newfilesys (p, -1, type, tmpp, unit, true);
					return 1;
				} else if (_tcsnicmp (value, _T("cd"), 2) == 0 && (value[2] == 0 || value[3] == 0)) {
					unit = 0;
					if (value[2] > 0)
						unit = value[2] - '0';
					if (unit >= 0 && unit <= MAX_TOTAL_SCSI_DEVICES) {
						type = 2;
					}
				} else if (_tcsnicmp (value, _T("tape"), 4) == 0 && (value[4] == 0 || value[5] == 0)) {
					unit = 0;
					if (value[4] > 0)
						unit = value[4] - '0';
					if (unit >= 0 && unit <= MAX_TOTAL_SCSI_DEVICES) {
						type = 3;
				}
				} else if (_tcsicmp (value, _T("dir")) != 0) {
					type = 0;
					return 1;  /* ignore for now */
				}
				if (type >= 0)
					cfgfile_parse_newfilesys (p, -1, type, tmpp, unit, true);
				return 1;
			}
			return 1;
		} else if (!_tcsncmp (option, tmp, _tcslen (tmp)) && option[_tcslen (tmp)] == '_') {
			struct uaedev_config_info *uci = &currprefs.mountconfig[i].ci;
			if (uci->devname) {
				const TCHAR *s = &option[_tcslen (tmp) + 1];
				if (!_tcscmp (s, _T("bootpri"))) {
					getintval (&value, &uci->bootpri, 0);
				} else if (!_tcscmp (s, _T("read-only"))) {
					cfgfile_yesno (NULL, value, NULL, &uci->readonly);
				} else if (!_tcscmp (s, _T("volumename"))) {
					_tcscpy (uci->volname, value);
				} else if (!_tcscmp (s, _T("devicename"))) {
					_tcscpy (uci->devname, value);
				} else if (!_tcscmp (s, _T("root"))) {
					_tcscpy (uci->rootdir, value);
				} else if (!_tcscmp (s, _T("filesys"))) {
					_tcscpy (uci->filesys, value);
				} else if (!_tcscmp (s, _T("controller"))) {
					uci->controller = get_filesys_controller (value);
				}
			}
		}
  }

  if (_tcscmp (option, _T("filesystem")) == 0
	|| _tcscmp (option, _T("hardfile")) == 0)
  {
		struct uaedev_config_info uci;
	  TCHAR *tmpp = _tcschr (value, ',');
	  TCHAR *str;
		bool hdf;

		uci_set_defaults (&uci, false);

	  if (config_newfilesystem)
	    return 1;

	  if (tmpp == 0)
	    goto invalid_fs;

	  *tmpp++ = '\0';
	  if (_tcscmp (value, _T("1")) == 0 || strcasecmp (value, _T("ro")) == 0
    || strcasecmp (value, _T("readonly")) == 0
    || strcasecmp (value, _T("read-only")) == 0)
			uci.readonly = true;
  	else if (_tcscmp (value, _T("0")) == 0 || strcasecmp (value, _T("rw")) == 0
		|| strcasecmp (value, _T("readwrite")) == 0
		|| strcasecmp (value, _T("read-write")) == 0)
			uci.readonly = false;
	  else
	    goto invalid_fs;

  	value = tmpp;
	  if (_tcscmp (option, _T("filesystem")) == 0) {
			hdf = false;
	    tmpp = _tcschr (value, ':');
      if (tmpp == 0)
    		goto invalid_fs;
      *tmpp++ = '\0';
			_tcscpy (uci.volname, value);
			_tcscpy (uci.rootdir, tmpp);
	  } else {
			hdf = true;
			if (! getintval (&value, &uci.sectors, ',')
				|| ! getintval (&value, &uci.surfaces, ',')
				|| ! getintval (&value, &uci.reserved, ',')
				|| ! getintval (&value, &uci.blocksize, ','))
    		goto invalid_fs;
			_tcscpy (uci.rootdir, value);
  	}
  	str = cfgfile_subst_path (UNEXPANDED, p->path_hardfile, uci.rootdir);
#ifdef FILESYS
		uci.type = hdf ? UAEDEV_HDF : UAEDEV_DIR;
		add_filesys_config (p, -1, &uci);
#endif
		xfree (str);
	  return 1;
invalid_fs:
  	write_log (_T("Invalid filesystem/hardfile specification.\n"));
  	return 1;

	}

	if (_tcscmp (option, _T("filesystem2")) == 0)
		return cfgfile_parse_newfilesys (p, -1, 0, value, -1, false);
	if (_tcscmp (option, _T("hardfile2")) == 0)
		return cfgfile_parse_newfilesys (p, -1, 1, value, -1, false);

	return 0;
}

static int cfgfile_parse_hardware (struct uae_prefs *p, const TCHAR *option, TCHAR *value)
{
  int tmpval, dummyint, i;
  TCHAR *section = 0;
  TCHAR tmpbuf[CONFIG_BLEN];

  if (cfgfile_yesno (option, value, _T("immediate_blits"), &p->immediate_blits)
	  || cfgfile_yesno (option, value, _T("fast_copper"), &p->fast_copper)
		|| cfgfile_yesno (option, value, _T("cd32cd"), &p->cs_cd32cd)
		|| cfgfile_yesno (option, value, _T("cd32c2p"), &p->cs_cd32c2p)
		|| cfgfile_yesno (option, value, _T("cd32nvram"), &p->cs_cd32nvram)
    || cfgfile_yesno (option, value, _T("synchronize_clock"), &p->tod_hack)

	  || cfgfile_yesno (option, value, _T("ntsc"), &p->ntscmode)
	  || cfgfile_yesno (option, value, _T("cpu_compatible"), &p->cpu_compatible)
	  || cfgfile_yesno (option, value, _T("cpu_24bit_addressing"), &p->address_space_24))
	  return 1;

  if (cfgfile_intval (option, value, _T("cachesize"), &p->cachesize, 1)
	  || cfgfile_intval (option, value, _T("chipset_refreshrate"), &p->chipset_refreshrate, 1)
	  || cfgfile_intval (option, value, _T("fastmem_size"), &p->fastmem_size, 0x100000)
	  || cfgfile_intval (option, value, _T("z3mem_size"), &p->z3fastmem_size, 0x100000)
	  || cfgfile_intval (option, value, _T("z3mem_start"), &p->z3fastmem_start, 1)
	  || cfgfile_intval (option, value, _T("bogomem_size"), &p->bogomem_size, 0x40000)
	  || cfgfile_intval (option, value, _T("gfxcard_size"), &p->rtgmem_size, 0x100000)
	  || cfgfile_strval (option, value, _T("gfxcard_type"), &p->rtgmem_type, rtgtype, 0)
	  || cfgfile_intval (option, value, _T("rtg_modes"), &p->picasso96_modeflags, 1)
	  || cfgfile_intval (option, value, _T("floppy_speed"), &p->floppy_speed, 1)
	  || cfgfile_intval (option, value, _T("floppy_write_length"), &p->floppy_write_length, 1)
	  || cfgfile_intval (option, value, _T("nr_floppies"), &p->nr_floppies, 1)
	  || cfgfile_intval (option, value, _T("floppy0type"), &p->floppyslots[0].dfxtype, 1)
	  || cfgfile_intval (option, value, _T("floppy1type"), &p->floppyslots[1].dfxtype, 1)
	  || cfgfile_intval (option, value, _T("floppy2type"), &p->floppyslots[2].dfxtype, 1)
	  || cfgfile_intval (option, value, _T("floppy3type"), &p->floppyslots[3].dfxtype, 1))
	  return 1;

  if (cfgfile_strval (option, value, _T("collision_level"), &p->collision_level, collmode, 0)
		|| cfgfile_strval (option, value, _T("waiting_blits"), &p->waiting_blits, waitblits, 0)
    )
  	return 1;
  if (cfgfile_path (option, value, _T("kickstart_rom_file"), p->romfile, sizeof p->romfile / sizeof (TCHAR))
	  || cfgfile_path (option, value, _T("kickstart_ext_rom_file"), p->romextfile, sizeof p->romextfile / sizeof (TCHAR))
    || cfgfile_path (option, value, _T("flash_file"), p->flashfile, sizeof p->flashfile / sizeof (TCHAR)))
	  return 1;

  for (i = 0; i < 4; i++) {
	  _stprintf (tmpbuf, _T("floppy%d"), i);
	  if (cfgfile_path (option, value, tmpbuf, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof (TCHAR)))
	    return 1;
  }

  if (cfgfile_intval (option, value, _T("chipmem_size"), &dummyint, 1)) {
  	if (dummyint < 0)
	    p->chipmem_size = 0x20000; /* 128k, prototype support */
  	else if (dummyint == 0)
	    p->chipmem_size = 0x40000; /* 256k */
  	else
	    p->chipmem_size = dummyint * 0x80000;
  	return 1;
  }

  if (cfgfile_strval (option, value, _T("chipset"), &tmpval, csmode, 0)) {
    set_chipset_mask (p, tmpval);
    return 1;
  }

  if (cfgfile_string (option, value, _T("fpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
	  p->fpu_model = _tstol(tmpbuf);
	  return 1;
  }

	if (cfgfile_string (option, value, _T("cpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
	  p->cpu_model = _tstol(tmpbuf);
	  p->fpu_model = 0;
	  return 1;
  }

    /* old-style CPU configuration */
	if (cfgfile_string (option, value, _T("cpu_type"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
	  p->fpu_model = 0;
	  p->address_space_24 = 0;
	  p->cpu_model = 680000;
		if (!_tcscmp (tmpbuf, _T("68000"))) {
	    p->cpu_model = 68000;
		} else if (!_tcscmp (tmpbuf, _T("68010"))) {
	    p->cpu_model = 68010;
		} else if (!_tcscmp (tmpbuf, _T("68ec020"))) {
	    p->cpu_model = 68020;
	    p->address_space_24 = 1;
		} else if (!_tcscmp (tmpbuf, _T("68020"))) {
	    p->cpu_model = 68020;
		} else if (!_tcscmp (tmpbuf, _T("68ec020/68881"))) {
	    p->cpu_model = 68020;
	    p->fpu_model = 68881;
	    p->address_space_24 = 1;
		} else if (!_tcscmp (tmpbuf, _T("68020/68881"))) {
	    p->cpu_model = 68020;
	    p->fpu_model = 68881;
		} else if (!_tcscmp (tmpbuf, _T("68040"))) {
	    p->cpu_model = 68040;
	    p->fpu_model = 68040;
  	}
	  return 1;
  }
    
	    /* Broken earlier versions used to write this out as a string.  */
	if (cfgfile_strval (option, value, _T("finegraincpu_speed"), &p->m68k_speed, speedmode, 1)) {
    p->m68k_speed--;
    return 1;
  }

	if (cfgfile_strval (option, value, _T("cpu_speed"), &p->m68k_speed, speedmode, 1)) {
		p->m68k_speed--;
		return 1;
	}
  if (cfgfile_intval (option, value, _T("cpu_speed"), &p->m68k_speed, 1)) {
    p->m68k_speed *= CYCLE_UNIT;
  	return 1;
  }
  if (cfgfile_intval (option, value, _T("finegrain_cpu_speed"), &p->m68k_speed, 1)) {
  	if (OFFICIAL_CYCLE_UNIT > CYCLE_UNIT) {
	    int factor = OFFICIAL_CYCLE_UNIT / CYCLE_UNIT;
	    p->m68k_speed = (p->m68k_speed + factor - 1) / factor;
  	}
    if (strcasecmp (value, _T("max")) == 0)
      p->m68k_speed = -1;
  	return 1;
  }

	if (cfgfile_parse_filesys (p, option, value))
		return 1;

  return 0;
}

static bool createconfigstore (struct uae_prefs*);
static int getconfigstoreline (const TCHAR *option, TCHAR *value);

static void calcformula (struct uae_prefs *prefs, TCHAR *in)
{
	TCHAR out[MAX_DPATH], configvalue[CONFIG_BLEN];
	TCHAR *p = out;
	double val;
	int cnt1, cnt2;
	static bool updatestore;

	if (_tcslen (in) < 2 || in[0] != '[' || in[_tcslen (in) - 1] != ']')
		return;
	if (!configstore || updatestore)
		createconfigstore (prefs);
	updatestore = false;
	if (!configstore)
		return;
	cnt1 = cnt2 = 0;
	for (int i = 1; i < _tcslen (in) - 1; i++) {
		TCHAR c = _totupper (in[i]);
		if (c >= 'A' && c <='Z') {
			TCHAR *start = &in[i];
			while (_istalnum (c) || c == '_' || c == '.') {
				i++;
				c = in[i];
			}
			TCHAR store = in[i];
			in[i] = 0;
			//write_log (_T("'%s'\n"), start);
			if (!getconfigstoreline (start, configvalue))
				return;
			_tcscpy (p, configvalue);
			p += _tcslen (p);
			in[i] = store;
			i--;
			cnt1++;
		} else {
			cnt2++;
			*p ++= c;
		}
	}
	*p = 0;
	if (cnt1 == 0 && cnt2 == 0)
		return;
	/* single config entry only? */
	if (cnt1 == 1 && cnt2 == 0) {
		_tcscpy (in, out);
		updatestore = true;
		return;
	}
	if (calc (out, &val)) {
		if (val - (int)val != 0.0f)
			_stprintf (in, _T("%f"), val);
		else
		  _stprintf (in, _T("%d"), (int)val);
		updatestore = true;
		return;
	}
}

int cfgfile_parse_option (struct uae_prefs *p, TCHAR *option, TCHAR *value, int type)
{
	calcformula (p, value);

  if (!_tcscmp (option, _T("config_hardware")))
  	return 1;
  if (!_tcscmp (option, _T("config_host")))
  	return 1;
  if (type == 0 || (type & CONFIG_TYPE_HARDWARE)) {
  	if (cfgfile_parse_hardware (p, option, value))
	    return 1;
  }
  if (type == 0 || (type & CONFIG_TYPE_HOST)) {
  	if (cfgfile_parse_host (p, option, value))
	    return 1;
  }
	if (type > 0 && (type & (CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST)) != (CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST))
  	return 1;
  return 0;
}

static int isutf8ext (TCHAR *s)
{
	if (_tcslen (s) > _tcslen (UTF8NAME) && !_tcscmp (s + _tcslen (s) - _tcslen (UTF8NAME), UTF8NAME)) {
		s[_tcslen (s) - _tcslen (UTF8NAME)] = 0;
		return 1;
	}
	return 0;
}

static int cfgfile_separate_linea (const TCHAR *filename, char *line, TCHAR *line1b, TCHAR *line2b)
{
  char *line1, *line2;
  int i;

  line1 = line;
  line1 += strspn (line1, "\t \r\n");
  if (*line1 == ';')
	  return 0;
  line2 = strchr (line, '=');
  if (! line2) {
		TCHAR *s = au (line1);
		write_log (_T("CFGFILE: '%s', linea was incomplete with only %s\n"), filename, s);
		xfree (s);
  	return 0;
  }
  *line2++ = '\0';

  /* Get rid of whitespace.  */
  i = strlen (line2);
  while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
    || line2[i - 1] == '\r' || line2[i - 1] == '\n'))
	  line2[--i] = '\0';
  line2 += strspn (line2, "\t \r\n");

  i = strlen (line);
  while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
    || line[i - 1] == '\r' || line[i - 1] == '\n'))
  	line[--i] = '\0';
  line += strspn (line, "\t \r\n");

	au_copy (line1b, MAX_DPATH, line);
	if (isutf8ext (line1b)) {
		if (line2[0]) {
			TCHAR *s = utf8u (line2);
			_tcscpy (line2b, s);
			xfree (s);
		}
	} else {
		au_copy (line2b, MAX_DPATH, line2);
	}

  return 1;
}

static int cfgfile_separate_line (TCHAR *line, TCHAR *line1b, TCHAR *line2b)
{
	TCHAR *line1, *line2;
	int i;

	line1 = line;
	line1 += _tcsspn (line1, _T("\t \r\n"));
	if (*line1 == ';')
		return 0;
	line2 = _tcschr (line, '=');
	if (! line2) {
		write_log (_T("CFGFILE: line was incomplete with only %s\n"), line1);
		return 0;
	}
	*line2++ = '\0';

	/* Get rid of whitespace.  */
	i = _tcslen (line2);
	while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		|| line2[i - 1] == '\r' || line2[i - 1] == '\n'))
		line2[--i] = '\0';
	line2 += _tcsspn (line2, _T("\t \r\n"));
	_tcscpy (line2b, line2);
	i = _tcslen (line);
	while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
		|| line[i - 1] == '\r' || line[i - 1] == '\n'))
		line[--i] = '\0';
	line += _tcsspn (line, _T("\t \r\n"));
	_tcscpy (line1b, line);

	if (line2b[0] == '"' || line2b[0] == '\"') {
		TCHAR c = line2b[0];
		int i = 0;
		memmove (line2b, line2b + 1, (_tcslen (line2b) + 1) * sizeof (TCHAR));
		while (line2b[i] != 0 && line2b[i] != c)
			i++;
		line2b[i] = 0;
	}

	if (isutf8ext (line1b))
		return 0;
	return 1;
}

static int isobsolete (TCHAR *s)
{
  int i = 0;
  while (obsolete[i]) {
  	if (!strcasecmp (s, obsolete[i])) {
	    write_log (_T("obsolete config entry '%s'\n"), s);
	    return 1;
  	}
  	i++;
  }
  if (_tcslen (s) > 2 && !_tcsncmp (s, _T("w."), 2))
  	return 1;
  if (_tcslen (s) >= 10 && !_tcsncmp (s, _T("gfx_opengl"), 10)) {
  	write_log (_T("obsolete config entry '%s\n"), s);
  	return 1;
  }
  if (_tcslen (s) >= 6 && !_tcsncmp (s, _T("gfx_3d"), 6)) {
    write_log (_T("obsolete config entry '%s\n"), s);
    return 1;
  }
  return 0;
}

static void cfgfile_parse_separated_line (struct uae_prefs *p, TCHAR *line1b, TCHAR *line2b, int type)
{
  TCHAR line3b[CONFIG_BLEN], line4b[CONFIG_BLEN];
  struct strlist *sl;
  int ret;

  _tcscpy (line3b, line1b);
  _tcscpy (line4b, line2b);
  ret = cfgfile_parse_option (p, line1b, line2b, type);
  if (!isobsolete (line3b)) {
  	for (sl = p->all_lines; sl; sl = sl->next) {
	    if (sl->option && !strcasecmp (line1b, sl->option)) break;
  	}
  	if (!sl) {
	    struct strlist *u = xcalloc (struct strlist, 1);
	    u->option = my_strdup(line3b);
	    u->value = my_strdup(line4b);
	    u->next = p->all_lines;
	    p->all_lines = u;
	    if (!ret) {
		    u->unknown = 1;
		    write_log (_T("unknown config entry: '%s=%s'\n"), u->option, u->value);
	    }
  	}
  }
}

void cfgfile_parse_lines (struct uae_prefs *p, const TCHAR *lines, int type)
{
	TCHAR *buf = my_strdup (lines);
	TCHAR *t = buf;
	for (;;) {
		if (_tcslen (t) == 0)
			break;
		TCHAR *t2 = _tcschr (t, '\n');
		if (t2)
			*t2 = 0;
		cfgfile_parse_line (p, t, type);
		if (!t2)
			break;
		t = t2 + 1;
	}
	xfree (buf);
}

void cfgfile_parse_line (struct uae_prefs *p, TCHAR *line, int type)
{
  TCHAR line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

  if (!cfgfile_separate_line (line, line1b, line2b))
  	return;
  cfgfile_parse_separated_line (p, line1b, line2b, type);
}

static void subst (TCHAR *p, TCHAR *f, int n)
{
  TCHAR *str = cfgfile_subst_path (UNEXPANDED, p, f);
  _tcsncpy (f, str, n - 1);
  f[n - 1] = '\0';
  free (str);
}

static int getconfigstoreline (const TCHAR *option, TCHAR *value)
{
	TCHAR tmp[CONFIG_BLEN * 2], tmp2[CONFIG_BLEN * 2];
	int idx = 0;

	if (!configstore)
		return 0;
	zfile_fseek (configstore, 0, SEEK_SET);
	for (;;) {
		if (!zfile_fgets (tmp, sizeof tmp / sizeof (TCHAR), configstore))
			return 0;
		if (!cfgfile_separate_line (tmp, tmp2, value))
			continue;
		if (!_tcsicmp (option, tmp2))
			return 1;
	}
}

static bool createconfigstore (struct uae_prefs *p)
{
	uae_u8 zeros[4] = { 0 };
	zfile_fclose (configstore);
	configstore = zfile_fopen_empty (NULL, _T("configstore"), 50000);
	if (!configstore)
		return false;
	zfile_fseek (configstore, 0, SEEK_SET);
	uaeconfig++;
	cfgfile_save_options (configstore, p, 0);
	uaeconfig--;
	zfile_fwrite (zeros, 1, sizeof zeros, configstore);
	zfile_fseek (configstore, 0, SEEK_SET);
	return true;
}

static char *cfg_fgets (char *line, int max, struct zfile *fh)
{
#ifdef SINGLEFILE
	extern TCHAR singlefile_config[];
	static TCHAR *sfile_ptr;
	TCHAR *p;
#endif

  if (fh)
  	return zfile_fgetsa (line, max, fh);
#ifdef SINGLEFILE
	if (sfile_ptr == 0) {
		sfile_ptr = singlefile_config;
		if (*sfile_ptr) {
			write_log (_T("singlefile config found\n"));
			while (*sfile_ptr++);
		}
	}
	if (*sfile_ptr == 0) {
		sfile_ptr = singlefile_config;
		return 0;
	}
	p = sfile_ptr;
	while (*p != 13 && *p != 10 && *p != 0) p++;
	memset (line, 0, max);
	memcpy (line, sfile_ptr, (p - sfile_ptr) * sizeof (TCHAR));
	sfile_ptr = p + 1;
	if (*sfile_ptr == 13)
		sfile_ptr++;
	if (*sfile_ptr == 10)
		sfile_ptr++;
	return line;
#endif
  return 0;
}

static int cfgfile_load_2 (struct uae_prefs *p, const TCHAR *filename, bool real, int *type)
{
  int i;
  struct zfile *fh;
  char linea[CONFIG_BLEN];
  TCHAR line[CONFIG_BLEN], line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];
  struct strlist *sl;
  bool type1 = false, type2 = false;
  int askedtype = 0;

  if (type) {
	  askedtype = *type;
	  *type = 0;
  }
  if (real) {
	  p->config_version = 0;
	  config_newfilesystem = 0;
		store_inputdevice_config (p);
	  //reset_inputdevice_config (p);
  }

  fh = zfile_fopen (filename, _T("r"), ZFD_NORMAL);
#ifndef	SINGLEFILE
  if (! fh)
  	return 0;
#endif

  while (cfg_fgets (linea, sizeof (linea), fh) != 0) {
  	trimwsa (linea);
  	if (strlen (linea) > 0) {
			if (linea[0] == '#' || linea[0] == ';') {
				struct strlist *u = xcalloc (struct strlist, 1);
				u->option = NULL;
				TCHAR *com = au (linea);
				u->value = my_strdup (com);
				xfree (com);
				u->unknown = 1;
				u->next = p->all_lines;
				p->all_lines = u;
		    continue;
			}
	    if (!cfgfile_separate_linea (filename, linea, line1b, line2b))
    		continue;
	    type1 = type2 = 0;
	    if (cfgfile_yesno (line1b, line2b, _T("config_hardware"), &type1) ||
        cfgfile_yesno (line1b, line2b, _T("config_host"), &type2)) {
  	    	if (type1 && type)
	    	    *type |= CONFIG_TYPE_HARDWARE;
  	    	if (type2 && type)
    		    *type |= CONFIG_TYPE_HOST;
      		continue;
	    }
	    if (real) {
    		cfgfile_parse_separated_line (p, line1b, line2b, askedtype);
	    } else {
    		cfgfile_string (line1b, line2b, _T("config_description"), p->description, sizeof p->description / sizeof (TCHAR));
	    }
	  }
  }

  if (type && *type == 0)
  	*type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
  zfile_fclose (fh);

  if (!real)
  	return 1;

  for (sl = temp_lines; sl; sl = sl->next) {
  	_stprintf (line, _T("%s=%s"), sl->option, sl->value);
  	cfgfile_parse_line (p, line, 0);
  }

  for (i = 0; i < 4; i++) {
  	subst (p->path_floppy, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof (TCHAR));
  	if(i >= p->nr_floppies)
  	  p->floppyslots[i].dfxtype = DRV_NONE;
  }
  subst (p->path_rom, p->romfile, sizeof p->romfile / sizeof (TCHAR));
  subst (p->path_rom, p->romextfile, sizeof p->romextfile / sizeof (TCHAR));

  return 1;
}

int cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int *type, int ignorelink, int userconfig)
{
  int v;
  TCHAR tmp[MAX_DPATH];
  int type2;
  static int recursive;

  if (recursive > 1)
  	return 0;
  recursive++;
	write_log (_T("load config '%s':%d\n"), filename, type ? *type : -1);
  v = cfgfile_load_2 (p, filename, 1, type);
  if (!v) {
		write_log (_T("load failed\n"));
	  goto end;
  }
end:
  recursive--;
  fixup_prefs (p);
  return v;
}

int cfgfile_save (struct uae_prefs *p, const TCHAR *filename, int type)
{
  struct zfile *fh;

  fh = zfile_fopen (filename, _T("w"), ZFD_NORMAL);
  if (! fh)
  	return 0;

  if (!type)
  	type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
  cfgfile_save_options (fh, p, type);
  zfile_fclose (fh);
  return 1;
}


int cfgfile_get_description (const TCHAR *filename, TCHAR *description)
{
  int result = 0;
  struct uae_prefs *p = xmalloc (struct uae_prefs, 1);

  p->description[0] = 0;
  if (cfgfile_load_2 (p, filename, 0, 0)) {
  	result = 1;
	  if (description)
	    _tcscpy (description, p->description);
  }
  xfree (p);
  return result;
}


int cfgfile_configuration_change(int v)
{
  static int mode;
  if (v >= 0)
  	mode = v;
  return mode;
}

static void parse_sound_spec (struct uae_prefs *p, const TCHAR *spec)
{
  TCHAR *x0 = my_strdup (spec);
  TCHAR *x1, *x2 = NULL, *x3 = NULL, *x4 = NULL, *x5 = NULL;

  x1 = _tcschr (x0, ':');
  if (x1 != NULL) {
	  *x1++ = '\0';
	  x2 = _tcschr (x1 + 1, ':');
	  if (x2 != NULL) {
	    *x2++ = '\0';
	    x3 = _tcschr (x2 + 1, ':');
	    if (x3 != NULL) {
		    *x3++ = '\0';
		    x4 = _tcschr (x3 + 1, ':');
		    if (x4 != NULL) {
		      *x4++ = '\0';
		      x5 = _tcschr (x4 + 1, ':');
    		}
	    }
  	}
  }
  p->produce_sound = _tstoi (x0);
  if (x1) {
  	p->sound_stereo_separation = 0;
  	if (*x1 == 'S') {
	    p->sound_stereo = SND_STEREO;
	    p->sound_stereo_separation = 7;
  	} else if (*x1 == 's')
	    p->sound_stereo = SND_STEREO;
  	else
	    p->sound_stereo = SND_MONO;
  }
  if (x3)
  	p->sound_freq = _tstoi (x3);
  free (x0);
}

static void parse_joy_spec (struct uae_prefs *p, const TCHAR *spec)
{
	int v0 = 2, v1 = 0;
	if (_tcslen(spec) != 2)
		goto bad;

	switch (spec[0]) {
	case '0': v0 = JSEM_JOYS; break;
	case '1': v0 = JSEM_JOYS + 1; break;
	case 'M': case 'm': v0 = JSEM_MICE; break;
	case 'A': case 'a': v0 = JSEM_KBDLAYOUT; break;
	case 'B': case 'b': v0 = JSEM_KBDLAYOUT + 1; break;
	case 'C': case 'c': v0 = JSEM_KBDLAYOUT + 2; break;
	default: goto bad;
	}

	switch (spec[1]) {
	case '0': v1 = JSEM_JOYS; break;
	case '1': v1 = JSEM_JOYS + 1; break;
	case 'M': case 'm': v1 = JSEM_MICE; break;
	case 'A': case 'a': v1 = JSEM_KBDLAYOUT; break;
	case 'B': case 'b': v1 = JSEM_KBDLAYOUT + 1; break;
	case 'C': case 'c': v1 = JSEM_KBDLAYOUT + 2; break;
	default: goto bad;
	}
	if (v0 == v1)
		goto bad;
	/* Let's scare Pascal programmers */
	if (0)
bad:
	write_log (_T("Bad joystick mode specification. Use -J xy, where x and y\n")
		_T("can be 0 for joystick 0, 1 for joystick 1, M for mouse, and\n")
		_T("a, b or c for different keyboard settings.\n"));

	p->jports[0].id = v0;
	p->jports[1].id = v1;
}

static void parse_filesys_spec (struct uae_prefs *p, bool readonly, const TCHAR *spec)
{
	struct uaedev_config_info uci;
  TCHAR buf[256];
  TCHAR *s2;

	uci_set_defaults (&uci, false);
  _tcsncpy (buf, spec, 255); buf[255] = 0;
  s2 = _tcschr (buf, ':');
  if (s2) {
  	*s2++ = '\0';
#ifdef __DOS__
	  {
      TCHAR *tmp;

      while ((tmp = _tcschr (s2, '\\')))
    		*tmp = '/';
	  }
#endif
#ifdef FILESYS
		_tcscpy (uci.volname, buf);
		_tcscpy (uci.rootdir, s2);
		uci.readonly = readonly;
		uci.type = UAEDEV_DIR;
		add_filesys_config (p, -1, &uci);
#endif
  } else {
		write_log (_T("Usage: [-m | -M] VOLNAME:mount_point\n"));
  }
}

static void parse_hardfile_spec (struct uae_prefs *p, const TCHAR *spec)
{
	struct uaedev_config_info uci;
  TCHAR *x0 = my_strdup (spec);
  TCHAR *x1, *x2, *x3, *x4;

	uci_set_defaults (&uci, false);
  x1 = _tcschr (x0, ':');
  if (x1 == NULL)
  	goto argh;
  *x1++ = '\0';
  x2 = _tcschr (x1 + 1, ':');
  if (x2 == NULL)
  	goto argh;
  *x2++ = '\0';
  x3 = _tcschr (x2 + 1, ':');
  if (x3 == NULL)
  	goto argh;
  *x3++ = '\0';
  x4 = _tcschr (x3 + 1, ':');
  if (x4 == NULL)
  	goto argh;
  *x4++ = '\0';
#ifdef FILESYS
	_tcscpy (uci.rootdir, x4);
	//add_filesys_config (p, -1, NULL, NULL, x4, 0, 0, _tstoi (x0), _tstoi (x1), _tstoi (x2), _tstoi (x3), 0, 0, 0, 0, 0, 0, 0);
#endif

  free (x0);
  return;

 argh:
  free (x0);
	write_log (_T("Bad hardfile parameter specified - type \"uae -h\" for help.\n"));
  return;
}

static void parse_cpu_specs (struct uae_prefs *p, const TCHAR *spec)
{
  if (*spec < '0' || *spec > '4') {
		write_log (_T("CPU parameter string must begin with '0', '1', '2', '3' or '4'.\n"));
	  return;
  }

  p->cpu_model = (*spec++) * 10 + 68000;
  p->address_space_24 = p->cpu_model < 68020;
  p->cpu_compatible = 0;
  while (*spec != '\0') {
	  switch (*spec) {
	  case 'a':
	    if (p->cpu_model < 68020)
				write_log (_T("In 68000/68010 emulation, the address space is always 24 bit.\n"));
	    else if (p->cpu_model >= 68040)
				write_log (_T("In 68040/060 emulation, the address space is always 32 bit.\n"));
	    else
		    p->address_space_24 = 1;
	    break;
	  case 'c':
	    if (p->cpu_model != 68000)
				write_log (_T("The more compatible CPU emulation is only available for 68000\n")
				_T("emulation, not for 68010 upwards.\n"));
	    else
		    p->cpu_compatible = 1;
	    break;
	  default:
			write_log (_T("Bad CPU parameter specified - type \"uae -h\" for help.\n"));
	    break;
	  }
	  spec++;
  }
}

static void cmdpath (TCHAR *dst, const TCHAR *src, int maxsz)
{
	TCHAR *s = target_expand_environment (src);
	_tcsncpy (dst, s, maxsz);
	dst[maxsz] = 0;
	xfree (s);
}

/* Returns the number of args used up (0 or 1).  */
int parse_cmdline_option (struct uae_prefs *p, TCHAR c, const TCHAR *arg)
{
  struct strlist *u = xcalloc (struct strlist, 1);
	const TCHAR arg_required[] = _T("0123rKpImWSAJwNCZUFcblOdHRv");

  if (_tcschr (arg_required, c) && ! arg) {
		write_log (_T("Missing argument for option `-%c'!\n"), c);
	  return 0;
  }

  u->option = xmalloc (TCHAR, 2);
  u->option[0] = c;
  u->option[1] = 0;
  u->value = my_strdup(arg);
  u->next = p->all_lines;
  p->all_lines = u;

  switch (c) {
    case '0': cmdpath (p->floppyslots[0].df, arg, 255); break;
    case '1': cmdpath (p->floppyslots[1].df, arg, 255); break;
    case '2': cmdpath (p->floppyslots[2].df, arg, 255); break;
    case '3': cmdpath (p->floppyslots[3].df, arg, 255); break;
    case 'r': cmdpath (p->romfile, arg, 255); break;
    case 'K': cmdpath (p->romextfile, arg, 255); break;
    case 'm': case 'M': parse_filesys_spec (p, c == 'M', arg); break;
    case 'W': parse_hardfile_spec (p, arg); break;
    case 'S': parse_sound_spec (p, arg); break;
    case 'R': p->gfx_framerate = _tstoi (arg); break;
	  case 'J': parse_joy_spec (p, arg); break;

    case 'w': p->m68k_speed = _tstoi (arg); break;

    case 'G': p->start_gui = 0; break;

    case 'n':
	    if (_tcschr (arg, 'i') != 0)
	      p->immediate_blits = 1;
	    break;

    case 'v':
	    set_chipset_mask (p, _tstoi (arg));
	    break;

    case 'C':
	    parse_cpu_specs (p, arg);
	    break;

    case 'Z':
	    p->z3fastmem_size = _tstoi (arg) * 0x100000;
	    break;

    case 'U':
	    p->rtgmem_size = _tstoi (arg) * 0x100000;
	    break;

    case 'F':
	    p->fastmem_size = _tstoi (arg) * 0x100000;
	    break;

    case 'b':
	    p->bogomem_size = _tstoi (arg) * 0x40000;
	    break;

    case 'c':
	    p->chipmem_size = _tstoi (arg) * 0x80000;
	    break;

    default:
		  write_log (_T("Unknown option `-%c'!\n"), c);
	    break;
  }
  return !! _tcschr (arg_required, c);
}

void cfgfile_addcfgparam (TCHAR *line)
{
  struct strlist *u;
  TCHAR line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

  if (!line) {
	  struct strlist **ps = &temp_lines;
	  while (*ps) {
	    struct strlist *s = *ps;
	    *ps = s->next;
	    xfree (s->value);
	    xfree (s->option);
	    xfree (s);
  	}
  	temp_lines = 0;
  	return;
  }
  if (!cfgfile_separate_line (line, line1b, line2b))
  	return;
  u = xcalloc (struct strlist, 1);
  u->option = my_strdup(line1b);
  u->value = my_strdup(line2b);
  u->next = temp_lines;
  temp_lines = u;
}

int cmdlineparser (const TCHAR *s, TCHAR *outp[], int max)
{
	int j, cnt = 0;
	int slash = 0;
	int quote = 0;
	TCHAR tmp1[MAX_DPATH];
	const TCHAR *prev;
	int doout;

	doout = 0;
	prev = s;
	j = 0;
	outp[0] = 0;
	while (cnt < max) {
		TCHAR c = *s++;
		if (!c)
			break;
		if (c < 32)
			continue;
		if (c == '\\')
			slash = 1;
		if (!slash && c == '"') {
			if (quote) {
				quote = 0;
				doout = 1;
			} else {
				quote = 1;
				j = -1;
			}
		}
		if (!quote && c == ' ')
			doout = 1;
		if (!doout) {
			if (j >= 0) {
				tmp1[j] = c;
				tmp1[j + 1] = 0;
			}
			j++;
		}
		if (doout) {
			if (_tcslen (tmp1) > 0) {
				outp[cnt++] = my_strdup (tmp1);
				outp[cnt] = 0;
			}
			tmp1[0] = 0;
			doout = 0;
			j = 0;
		}
		slash = 0;
	}
	if (j > 0 && cnt < max) {
		outp[cnt++] = my_strdup (tmp1);
		outp[cnt] = 0;
	}
	return cnt;
}

#define UAELIB_MAX_PARSE 100

static bool cfgfile_parse_uaelib_option (struct uae_prefs *p, TCHAR *option, TCHAR *value, int type)
{
	return false;
}

int cfgfile_searchconfig(const TCHAR *in, int index, TCHAR *out, int outsize)
{
	TCHAR tmp[CONFIG_BLEN];
	int j = 0;
	int inlen = _tcslen (in);
	int joker = 0;
	uae_u32 err = 0;
	bool configsearchfound = false;

	if (in[inlen - 1] == '*') {
		joker = 1;
		inlen--;
	}
	*out = 0;

	if (!configstore)
		createconfigstore(&currprefs);
	if (!configstore)
		return 20;

	if (index < 0)
		zfile_fseek(configstore, 0, SEEK_SET);

	for (;;) {
		uae_u8 b = 0;

		if (zfile_fread (&b, 1, 1, configstore) != 1) {
			err = 10;
			if (configsearchfound)
				err = 0;
			goto end;
		}
		if (j >= sizeof (tmp) / sizeof (TCHAR) - 1)
			j = sizeof (tmp) / sizeof (TCHAR) - 1;
		if (b == 0) {
			err = 10;
			if (configsearchfound)
				err = 0;
			goto end;
		}
		if (b == '\n') {
			if (!_tcsncmp (tmp, in, inlen) && ((inlen > 0 && _tcslen (tmp) > inlen && tmp[inlen] == '=') || (joker))) {
				TCHAR *p;
				if (joker)
					p = tmp - 1;
				else
					p = _tcschr (tmp, '=');
				if (p) {
					for (int i = 0; out && i < outsize - 1; i++) {
						TCHAR b = *++p;
						out[i] = b;
						out[i + 1] = 0;
						if (!b)
							break;
					}
				}
				err = 0xffffffff;
				configsearchfound = true;
				goto end;
			}
			j = 0;
		} else {
			tmp[j++] = b;
			tmp[j] = 0;
		}
	}
end:
	return err;
}

uae_u32 cfgfile_modify (uae_u32 index, TCHAR *parms, uae_u32 size, TCHAR *out, uae_u32 outsize)
{
	TCHAR *p;
	TCHAR *argc[UAELIB_MAX_PARSE];
	int argv, i;
	uae_u32 err;
	TCHAR zero = 0;
	static TCHAR *configsearch;

	*out = 0;
	err = 0;
	argv = 0;
	p = 0;
	if (index != 0xffffffff) {
		if (!configstore) {
			err = 20;
			goto end;
		}
		if (configsearch) {
			err = cfgfile_searchconfig(configsearch, index,  out, outsize);
			goto end;
		}
		err = 0xffffffff;
		for (i = 0; out && i < outsize - 1; i++) {
			uae_u8 b = 0;
			if (zfile_fread (&b, 1, 1, configstore) != 1)
				err = 0;
			if (b == 0)
				err = 0;
			if (b == '\n')
				b = 0;
			out[i] = b;
			out[i + 1] = 0;
			if (!b)
				break;
		}
		goto end;
	}

	if (size > 10000)
		return 10;
	argv = cmdlineparser (parms, argc, UAELIB_MAX_PARSE);

	if (argv <= 1 && index == 0xffffffff) {
		createconfigstore (&currprefs);
		xfree (configsearch);
		configsearch = NULL;
		if (!configstore) {
			err = 20;
			goto end;
		}
		if (argv > 0 && _tcslen (argc[0]) > 0)
			configsearch = my_strdup (argc[0]);
		err = 0xffffffff;
		goto end;
	}

	for (i = 0; i < argv; i++) {
		if (i + 2 <= argv) {
			if (!inputdevice_uaelib (argc[i], argc[i + 1])) {
				if (!cfgfile_parse_uaelib_option (&changed_prefs, argc[i], argc[i + 1], 0)) {
					if (!cfgfile_parse_option (&changed_prefs, argc[i], argc[i + 1], 0)) {
						err = 5;
						break;
					}
				}
			}
			set_special (SPCFLAG_MODE_CHANGE);
			i++;
		}
	}
end:
	for (i = 0; i < argv; i++)
		xfree (argc[i]);
	xfree (p);
	return err;
}

uae_u32 cfgfile_uaelib_modify (uae_u32 index, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize)
{
	uae_char *p, *parms_p = NULL, *parms_out = NULL;
	int i, ret;
	TCHAR *out_p = NULL, *parms_in = NULL;

	if (out)
		put_byte (out, 0);
	if (size == 0) {
		while (get_byte (parms + size) != 0)
			size++;
	}
	parms_p = xmalloc (uae_char, size + 1);
	if (!parms_p) {
		ret = 10;
		goto end;
	}
	if (out) {
		out_p = xmalloc (TCHAR, outsize + 1);
		if (!out_p) {
			ret = 10;
			goto end;
		}
		out_p[0] = 0;
	}
	p = parms_p;
	for (i = 0; i < size; i++) {
		p[i] = get_byte (parms + i);
		if (p[i] == 10 || p[i] == 13 || p[i] == 0)
			break;
	}
	p[i] = 0;
	parms_in = au (parms_p);
	ret = cfgfile_modify (index, parms_in, size, out_p, outsize);
	xfree (parms_in);
	if (out) {
		parms_out = ua (out_p);
		p = parms_out;
		for (i = 0; i < outsize - 1; i++) {
			uae_u8 b = *p++;
			put_byte (out + i, b);
			put_byte (out + i + 1, 0);
			if (!b)
				break;
		}
	}
	xfree (parms_out);
end:
	xfree (out_p);
	xfree (parms_p);
	return ret;
}

const TCHAR *cfgfile_read_config_value (const TCHAR *option)
{
	struct strlist *sl;
	for (sl = currprefs.all_lines; sl; sl = sl->next) {
		if (sl->option && !strcasecmp (sl->option, option))
			return sl->value;
	}
	return NULL;
}

uae_u32 cfgfile_uaelib (int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen)
{
	TCHAR tmp[CONFIG_BLEN];
	int i;

	if (mode)
		return 0;

	for (i = 0; i < sizeof (tmp) / sizeof (TCHAR); i++) {
		tmp[i] = get_byte (name + i);
		if (tmp[i] == 0)
			break;
	}
	tmp[sizeof(tmp) / sizeof (TCHAR) - 1] = 0;
	if (tmp[0] == 0)
		return 0;
	const TCHAR *value = cfgfile_read_config_value (tmp);
	if (value) {
		char *s = ua (value);
		for (i = 0; i < maxlen; i++) {
			put_byte (dst + i, s[i]);
			if (s[i] == 0)
				break;
		}
		xfree (s);
		return dst;
	}
	return 0;
}

#include "sounddep/sound.h"

void default_prefs (struct uae_prefs *p, int type)
{
  int i;
	int roms[] = { 6, 7, 8, 9, 10, 14, 5, 4, 3, 2, 1, -1 };
  TCHAR zero = 0;
  struct zfile *f;

	reset_inputdevice_config (p);
  memset (p, 0, sizeof (struct uae_prefs));
  _tcscpy (p->description, _T("UAE default configuration"));

  p->start_gui = true;

  p->all_lines = 0;

	p->mountitems = 0;
	for (i = 0; i < MOUNT_CONFIG_SIZE; i++) {
		p->mountconfig[i].configoffset = -1;
		p->mountconfig[i].unitnum = -1;
	}

	memset (&p->jports[0], 0, sizeof (struct jport));
	memset (&p->jports[1], 0, sizeof (struct jport));
	memset (&p->jports[2], 0, sizeof (struct jport));
	memset (&p->jports[3], 0, sizeof (struct jport));
	p->jports[0].id = JSEM_MICE;
	p->jports[1].id = JSEM_JOYS;
	p->jports[2].id = -1;
	p->jports[3].id = -1;

  p->produce_sound = 3;
  p->sound_stereo = SND_STEREO;
  p->sound_stereo_separation = 7;
  p->sound_mixed_stereo_delay = 0;
  p->sound_freq = DEFAULT_SOUND_FREQ;
  p->sound_interpol = 0;
  p->sound_filter = FILTER_SOUND_OFF;
  p->sound_filter_type = 0;
	p->sound_volume_cd = 20;

  p->cachesize = 0;

  for (i = 0;i < 10; i++)
	  p->optcount[i] = -1;
  p->optcount[0] = 4;	/* How often a block has to be executed before it is translated */
  p->optcount[1] = 0;	/* How often to use the naive translation */
  p->optcount[2] = 0;
  p->optcount[3] = 0;
  p->optcount[4] = 0;
  p->optcount[5] = 0;

  p->gfx_framerate = 0;
  p->gfx_size_fs.width = 640;
  p->gfx_size_fs.height = 480;
  p->gfx_size_win.width = 320;
  p->gfx_size_win.height = 240;
#ifdef RASPBERRY
  p->gfx_size.width = 640;
  p->gfx_size.height = 262;
#else
  p->gfx_size.width = 320;
  p->gfx_size.height = 240;
#endif
  p->gfx_resolution = RES_LORES;
#ifdef RASPBERRY
  p->gfx_correct_aspect = 1;
  p->gfx_fullscreen_ratio = 100;
  p->kbd_led_num = -1; // No status on numlock
  p->kbd_led_scr = -1; // No status on scrollock
  p->kbd_led_cap = -1; // No status on capslock
#endif
  p->immediate_blits = 0;
  p->waiting_blits = 0;
  p->chipset_refreshrate = 50;
  p->collision_level = 2;
  p->leds_on_screen = 0;
  p->fast_copper = 1;
  p->tod_hack = 1;

	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = false;

  _tcscpy (p->floppyslots[0].df, _T(""));
  _tcscpy (p->floppyslots[1].df, _T(""));
  _tcscpy (p->floppyslots[2].df, _T(""));
  _tcscpy (p->floppyslots[3].df, _T(""));

	configure_rom (p, roms, 0);
  _tcscpy (p->romextfile, _T(""));
	_tcscpy (p->flashfile, _T(""));

  sprintf (p->path_rom, _T("%s/kickstarts/"), start_path_data);
  sprintf (p->path_floppy, _T("%s/disks/"), start_path_data);
  sprintf (p->path_hardfile, _T("%s/"), start_path_data);
  sprintf (p->path_cd, _T("%s/cd32/"), start_path_data);

  p->fpu_model = 0;
  p->cpu_model = 68000;
  p->m68k_speed = 0;
  p->cpu_compatible = 0;
  p->address_space_24 = 1;
  p->chipset_mask = CSMASK_ECS_AGNUS;
  p->ntscmode = 0;
	p->filesys_limit = 0;
	p->filesys_max_name = 107;

  p->fastmem_size = 0x00000000;
  p->z3fastmem_size = 0x00000000;
  p->z3fastmem_start = z3_start_adr;
  p->chipmem_size = 0x00100000;
  p->bogomem_size = 0x00000000;
  p->rtgmem_size = 0x00000000;
	p->rtgmem_type = GFXBOARD_UAE_Z3;

  p->nr_floppies = 2;
  p->floppyslots[0].dfxtype = DRV_35_DD;
  p->floppyslots[1].dfxtype = DRV_35_DD;
  p->floppyslots[2].dfxtype = DRV_NONE;
  p->floppyslots[3].dfxtype = DRV_NONE;
  p->floppy_speed = 100;
  p->floppy_write_length = 0;
  
	p->socket_emu = 0;

  p->input_tablet = TABLET_OFF;

  inputdevice_default_prefs (p);

	blkdev_default_prefs (p);

  target_default_options (p, type);

    p->key_for_menu = SDLK_F12;

    zfile_fclose (default_file);
	default_file = NULL;
	f = zfile_fopen_empty (NULL, _T("configstore"));
	if (f) {
		uaeconfig++;
		cfgfile_save_options (f, p, 0);
		uaeconfig--;
		cfg_write (&zero, f);
		default_file = f;
	}
}

static void buildin_default_prefs_68020 (struct uae_prefs *p)
{
	p->cpu_model = 68020;
	p->address_space_24 = 1;
	p->cpu_compatible = 0;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->chipmem_size = 0x200000;
	p->bogomem_size = 0;
}


int bip_a4000 (struct uae_prefs *p, int rom)
{
	int roms[4];

	roms[0] = 15;
	roms[1] = 14;
	roms[2] = 11;
	roms[3] = -1;

	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
  p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->cachesize = 8192;

  p->nr_floppies = 2;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;

	return configure_rom (p, roms, 0);
}

int bip_cd32 (struct uae_prefs *p, int rom)
{
	int roms[2];

	buildin_default_prefs_68020 (p);
	p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = 1;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat (p->flashfile, _T("cd32.nvr"));

	p->cdslots[0].inuse = true;
	p->cdslots[0].type = SCSI_UNIT_IMAGE;
	
	p->gfx_size.width = 384;
	p->gfx_size.height = 256;

	roms[0] = 64;
	roms[1] = -1;
	if (!configure_rom (p, roms, 0)) {
		roms[0] = 18;
		roms[1] = -1;
		if (!configure_rom (p, roms, 0))
			return 0;
		roms[0] = 19;
		if (!configure_rom (p, roms, 0))
			return 0;
	}
//	if (config > 0) {
//		roms[0] = 23;
//		if (!configure_rom (p, roms, 0))
//			return 0;
//	}

	return 1;
}

int bip_a1200 (struct uae_prefs *p, int rom)
{
	int roms[4];

	buildin_default_prefs_68020 (p);
	if(rom == 310)
  {
  	roms[0] = 15;
  	roms[1] = 11;
  	roms[2] = 31;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 11;
  	roms[1] = 15;
  	roms[2] = 31;
  	roms[3] = -1;
  }

	p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;

  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;

	return configure_rom (p, roms, 0);
}

int bip_a500plus (struct uae_prefs *p, int rom)
{
  int roms[4];

	if(rom == 130)
  {
  	roms[0] = 6;
  	roms[1] = 5;
  	roms[2] = 4;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 7;
  	roms[1] = 6;
  	roms[2] = 5;
    roms[3] = -1;
  }
	p->bogomem_size = 0;
  p->chipmem_size = 0x100000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
  p->cpu_compatible = 0;
  p->fast_copper = 0;
  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
  return configure_rom (p, roms, 0);
}

int bip_a500 (struct uae_prefs *p, int rom)
{
  int roms[4];

	if(rom == 130)
  {
  	roms[0] = 6;
  	roms[1] = 5;
  	roms[2] = 4;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 5;
  	roms[1] = 4;
  	roms[2] = 3;
    roms[3] = -1;
  }
  p->chipmem_size = 0x00080000;
	p->chipset_mask = 0;
  p->cpu_compatible = 0;
  p->fast_copper = 0;
  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
  return configure_rom (p, roms, 0);
}

int bip_a2000 (struct uae_prefs *p, int rom)
{
  int roms[4];

	if(rom == 130)
  {
  	roms[0] = 6;
  	roms[1] = 5;
  	roms[2] = 4;
    roms[3] = -1;
  }
  else
  {
  	roms[0] = 5;
  	roms[1] = 4;
  	roms[2] = 3;
    roms[3] = -1;
  }
  p->chipmem_size = 0x00080000;
  p->bogomem_size = 0x00080000;
	p->chipset_mask = 0;
  p->cpu_compatible = 0;
  p->fast_copper = 0;
  p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
  return configure_rom (p, roms, 0);
}

bool is_error_log (void)
{
	return error_lines != NULL;
}
TCHAR *get_error_log (void)
{
	strlist *sl;
	int len = 0;
	for (sl = error_lines; sl; sl = sl->next) {
		len += _tcslen (sl->option) + 1;
	}
	if (!len)
		return NULL;
	TCHAR *s = xcalloc (TCHAR, len + 1);
	for (sl = error_lines; sl; sl = sl->next) {
		_tcscat (s, sl->option);
		_tcscat (s, _T("\n"));
	}
	return s;
}
void error_log (const TCHAR *format, ...)
{
	TCHAR buffer[256], *bufp;
	int bufsize = 256;
	va_list parms;

	if (format == NULL) {
		struct strlist **ps = &error_lines;
		while (*ps) {
			struct strlist *s = *ps;
			*ps = s->next;
			xfree (s->value);
			xfree (s->option);
			xfree (s);
		}
		return;
	}

	va_start (parms, format);
	bufp = buffer;
	for (;;) {
		int count = _vsntprintf (bufp, bufsize - 1, format, parms);
		if (count < 0) {
			bufsize *= 10;
			if (bufp != buffer)
				xfree (bufp);
			bufp = xmalloc (TCHAR, bufsize);
			continue;
		}
		break;
	}
	bufp[bufsize - 1] = 0;
	write_log (_T("%s\n"), bufp);
	va_end (parms);

	strlist *u = xcalloc (struct strlist, 1);
	u->option = my_strdup (bufp);
	u->next = error_lines;
	error_lines = u;

	if (bufp != buffer)
		xfree (bufp);
}
