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
#include "custom.h"
#include "inputdevice.h"
#include "savestate.h"
#include "include/memory.h"
#include "autoconf.h"
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
#include "native2amiga_api.h"

#define cfgfile_warning write_log
#define cfgfile_warning_obsolete write_log

#ifdef USE_SDL1
#include "SDL_keysym.h"
#endif

static int config_newfilesystem;
static struct strlist* temp_lines;
static struct strlist* error_lines;
static struct zfile *default_file, *configstore;
static int uaeconfig;

/* @@@ need to get rid of this... just cut part of the manual and print that
 * as a help text.  */
struct cfg_lines
{
	const TCHAR *config_label, *config_help;
};

static const TCHAR* guimode1[] = {_T("no"), _T("yes"), _T("nowait"), nullptr};
static const TCHAR* guimode2[] = {_T("false"), _T("true"), _T("nowait"), nullptr};
static const TCHAR* guimode3[] = {_T("0"), _T("1"), _T("nowait"), nullptr};
static const TCHAR* csmode[] = {_T("ocs"), _T("ecs_agnus"), _T("ecs_denise"), _T("ecs"), _T("aga"), nullptr};
static const TCHAR* linemode[] = {
	_T("none"),
	_T("double"), _T("scanlines"), _T("scanlines2p"), _T("scanlines3p"),
	_T("double2"), _T("scanlines2"), _T("scanlines2p2"), _T("scanlines2p3"),
	_T("double3"), _T("scanlines3"), _T("scanlines3p2"), _T("scanlines3p3"),
	nullptr
};
static const TCHAR* speedmode[] = {_T("max"), _T("real"), _T("turbo"), nullptr};
static const TCHAR* soundmode1[] = {_T("none"), _T("interrupts"), _T("normal"), _T("exact"), nullptr};
static const TCHAR* soundmode2[] = {_T("none"), _T("interrupts"), _T("good"), _T("best"), nullptr};
static const TCHAR* stereomode[] = {
	_T("mono"), _T("stereo"), _T("clonedstereo"), _T("4ch"), _T("clonedstereo6ch"), _T("6ch"), _T("mixed"), nullptr
};
static const TCHAR* interpolmode[] = {_T("none"), _T("anti"), _T("sinc"), _T("rh"), _T("crux"), nullptr};
static const TCHAR* collmode[] = {_T("none"), _T("sprites"), _T("playfields"), _T("full"), nullptr};
static const TCHAR* soundfiltermode1[] = {_T("off"), _T("emulated"), _T("on"), nullptr};
static const TCHAR* soundfiltermode2[] = {_T("standard"), _T("enhanced"), nullptr};
static const TCHAR* lorestype1[] = {_T("lores"), _T("hires"), _T("superhires"), nullptr};
static const TCHAR* lorestype2[] = {_T("true"), _T("false"), nullptr};
static const TCHAR *loresmode[] = { _T("normal"), _T("filtered"), 0 };
static const TCHAR *horizmode[] = { _T("vertical"), _T("lores"), _T("hires"), _T("superhires"), 0 };
static const TCHAR *vertmode[] = { _T("horizontal"), _T("single"), _T("double"), _T("quadruple"), 0 };
static const TCHAR* cartsmode[] = {_T("none"), _T("hrtmon"), nullptr};
static const TCHAR* idemode[] = {_T("none"), _T("a600/a1200"), _T("a4000"), nullptr};
static const TCHAR* rtctype[] = {_T("none"), _T("MSM6242B"), _T("RP5C01A"), _T("MSM6242B_A2000"), nullptr};
static const TCHAR* ciaatodmode[] = {_T("vblank"), _T("50hz"), _T("60hz"), nullptr};
static const TCHAR* cscompa[] = {
	_T("-"), _T("Generic"), _T("CD32"), _T("A500"), _T("A500+"), _T("A600"),
	_T("A1200"), _T("A2000"), _T("A4000"),
	nullptr
};
static const TCHAR* qsmodes[] = {
	_T("A500"), _T("A500+"), _T("A600"), _T("A1200"), _T("A4000"), _T("CD32"), nullptr
};
static const TCHAR *fullmodes[] = { _T("false"), _T("true"), /* "FILE_NOT_FOUND", */ _T("fullwindow"), nullptr };
static const TCHAR* abspointers[] = {_T("none"), _T("mousehack"), _T("tablet"), nullptr};
static const TCHAR* joyportmodes[] = {
	_T(""), _T("mouse"), _T("mousenowheel"), _T("djoy"), _T("gamepad"), _T("ajoy"), _T("cdtvjoy"), _T("cd32joy"), nullptr
};
static const TCHAR* mousemaps[] = {_T(""), _T("left"), _T("right"), _T("both"), nullptr};
static const TCHAR* joyaf[] = {_T("none"), _T("normal"), _T("toggle"), _T("always"), nullptr};
static const TCHAR* cdmodes[] = {_T("disabled"), _T(""), _T("image"), _T("ioctl"), _T("spti"), _T("aspi"), nullptr};
static const TCHAR* cdconmodes[] = {_T(""), _T("uae"), _T("ide"), _T("scsi"), _T("cdtv"), _T("cd32"), nullptr};
static const TCHAR* waitblits[] = {_T("disabled"), _T("automatic"), _T("noidleonly"), _T("always"), nullptr};
static const TCHAR* autoext2[] = {_T("disabled"), _T("copy"), _T("replace"), nullptr};
static const TCHAR *unmapped[] = { _T("floating"), _T("zero"), _T("one"), 0 };
static const TCHAR *ciatype[] = { _T("default"), _T("391078-01"), 0 };

static const TCHAR* button_remap_name[] = {
	_T("dpad_up"), _T("dpad_down"), _T("dpad_left"), _T("dpad_right"),
	_T("select"), _T("left_shoulder"), _T("left_stick"),
	_T("north"), _T("south"), _T("east"), _T("west"),
	_T("start"), _T("right_shoulder"), _T("right_stick"), nullptr
};


const TCHAR* find_inputevent_name(int key)
{
	int index = -1;
	const struct inputevent* tempevent;

	const TCHAR* output;

	for (int i = 0; i < RemapEventListSize; ++i)
	{
		if (RemapEventList[i] == key)
		{
			index = i;
			tempevent = inputdevice_get_eventinfo(RemapEventList[i]);
			output = _T(tempevent->name);
			break;
		}
		output = "None";
	}
	return output;
}

static int find_inputevent(TCHAR* key)
{
	int index = -1;
	const struct inputevent* tempevent;
	char tmp1[255], tmp2[255];

	for (int i = 0; i < RemapEventListSize; ++i)
	{
		tempevent = inputdevice_get_eventinfo(RemapEventList[i]);
		snprintf(tmp1, 255, "%s", tempevent->name);
		snprintf(tmp2, 255, "%s", key);

		if (!_tcscmp(tmp1, tmp2))
		{
			index = i;
			break;
		}
	}
	return index;
}


struct hdcontrollerconfig
{
	const TCHAR* label;
	int romtype;
};

static const struct hdcontrollerconfig hdcontrollers[] = {
	{_T("uae"), 0},
	{_T("ide%d"), 0},
	{_T("ide%d_mainboard"), ROMTYPE_MB_IDE},

	{nullptr}
};
static const TCHAR* z3mapping[] = {
	_T("auto"),
	_T("uae"),
	_T("real"),
	nullptr
};
static const TCHAR* uaebootrom[] = {
	_T("automatic"),
	_T("disabled"),
	_T("min"),
	_T("full"),
	nullptr
};
static const TCHAR* uaeboard[] = {
	_T("disabled"),
	_T("min"),
	_T("full"),
	_T("full+indirect"),
	nullptr
};

static const TCHAR* obsolete[] = {
	_T("accuracy"), _T("gfx_opengl"), _T("gfx_32bit_blits"), _T("32bit_blits"),
	_T("gfx_immediate_blits"), _T("gfx_ntsc"), _T("win32"), _T("gfx_filter_bits"),
	_T("sound_pri_cutoff"), _T("sound_pri_time"), _T("sound_min_buff"), _T("sound_bits"),
	_T("gfx_test_speed"), _T("gfxlib_replacement"), _T("enforcer"), _T("catweasel_io"),
	_T("kickstart_key_file"), _T("sound_adjust"), _T("sound_latency"),
	_T("serial_hardware_dtrdsr"), _T("gfx_filter_upscale"),
	_T("gfx_autoscale"), _T("parallel_sampler"), _T("parallel_ascii_emulation"),
	_T("avoid_vid"), _T("avoid_dga"), _T("z3chipmem_size"), _T("state_replay_buffer"), _T("state_replay"),
	_T("z3realmapping"), _T("force_0x10000000_z3"),
	_T("fpu_arithmetic_exceptions"),

	_T("gfx_filter_vert_zoom"),_T("gfx_filter_horiz_zoom"),
	_T("gfx_filter_vert_zoom_mult"), _T("gfx_filter_horiz_zoom_mult"),
	_T("gfx_filter_vert_offset"), _T("gfx_filter_horiz_offset"),

	_T("pcibridge_rom_file"),
	_T("pcibridge_rom_options"),

	_T("cpuboard_ext_rom_file"),
	_T("uaeboard_mode"),

	_T("comp_oldsegv"),
	_T("comp_midopt"),
	_T("comp_lowopt"),
	_T("avoid_cmov"),
	_T("compforcesettings"),
	nullptr
};

#define UNEXPANDED _T("$(FILE_PATH)")


static TCHAR* cfgfile_unescape(const TCHAR* s, const TCHAR** endpos, TCHAR separator, bool min)
{
	bool quoted = false;
	TCHAR* s2 = xmalloc (TCHAR, _tcslen (s) + 1);
	TCHAR* p = s2;
	if (s[0] == '\"')
	{
		s++;
		quoted = true;
	}
	int i;
	for (i = 0; s[i]; i++)
	{
		TCHAR c = s[i];
		if (quoted && c == '\"')
		{
			i++;
			break;
		}
		if (c == separator)
		{
			i++;
			break;
		}
		if (c == '\\' && !min)
		{
			char v = 0;
			TCHAR c2;
			c = s[i + 1];
			switch (c)
			{
			case 'X':
			case 'x':
				c2 = _totupper(s[i + 2]);
				v = ((c2 >= 'A') ? c2 - 'A' : c2 - '0') << 4;
				c2 = _totupper(s[i + 3]);
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
		}
		else
		{
			*p++ = c;
		}
	}
	*p = 0;
	if (endpos)
		*endpos = &s[i];
	return s2;
}

static TCHAR* cfgfile_unescape(const TCHAR* s, const TCHAR** endpos)
{
	return cfgfile_unescape(s, endpos, 0, false);
}

static TCHAR* cfgfile_unescape_min(const TCHAR* s)
{
	return cfgfile_unescape(s, nullptr, 0, true);
}

static TCHAR* cfgfile_option_find_it(const TCHAR* s, const TCHAR* option, bool checkequals)
{
	TCHAR buf[MAX_DPATH];
	if (!s)
		return nullptr;
	_tcscpy(buf, s);
	_tcscat(buf, _T(","));
	TCHAR* p = buf;
	for (;;)
	{
		TCHAR* tmpp = _tcschr(p, ',');
		TCHAR* tmpp2 = nullptr;
		if (tmpp == nullptr)
			return nullptr;
		*tmpp++ = 0;
		if (checkequals)
		{
			tmpp2 = _tcschr(p, '=');
			if (tmpp2)
				*tmpp2++ = 0;
		}
		if (!_tcsicmp(p, option))
		{
			if (checkequals && tmpp2)
			{
				if (tmpp2[0] == '"')
				{
					TCHAR* n = cfgfile_unescape_min(tmpp2);
					_tcscpy(tmpp2, n);
					xfree(n);
					return tmpp2;
				}
				return tmpp2;
			}
			return p;
		}
		p = tmpp;
	}
}

bool cfgfile_option_find(const TCHAR *s, const TCHAR *option)
{
	TCHAR *ss = cfgfile_option_find_it(s, option, false);
	xfree(ss);
	return ss != NULL;
}

TCHAR *cfgfile_option_get(const TCHAR *s, const TCHAR *option)
{
	return cfgfile_option_find_it(s, option, true);
}

static void trimwsa(char* s)
{
	/* Delete trailing whitespace.  */
	int len = strlen(s);
	while (len > 0 && strcspn(s + len - 1, "\t \r\n") == 0)
		s[--len] = '\0';
}

static int match_string(const TCHAR* table[], const TCHAR* str)
{
	int i;
	for (i = 0; table[i] != nullptr; i++)
		if (_tcsicmp(table[i], str) == 0)
			return i;
	return -1;
}

// escape config file separators and control characters
static TCHAR* cfgfile_escape(const TCHAR* s, const TCHAR* escstr, bool quote)
{
	bool doquote = false;
	int cnt = 0;
	for (int i = 0; s[i]; i++)
	{
		TCHAR c = s[i];
		if (c == 0)
			break;
		if (c < 32 || c == '\\' || c == '\"' || c == '\'')
		{
			cnt++;
		}
		for (int j = 0; escstr && escstr[j]; j++)
		{
			if (c == escstr[j])
			{
				cnt++;
				if (quote)
				{
					doquote = true;
					cnt++;
				}
			}
		}
	}
	if (escstr == nullptr && quote)
		doquote = true;
	TCHAR* s2 = xmalloc (TCHAR, _tcslen (s) + cnt * 4 + 1);
	TCHAR* p = s2;
	if (doquote)
		*p++ = '\"';
	for (int i = 0; s[i]; i++)
	{
		TCHAR c = s[i];
		if (c == 0)
			break;
		if (c == '\\' || c == '\"' || c == '\'')
		{
			*p++ = '\\';
			*p++ = c;
		}
		else if (c >= 32 && !quote)
		{
			bool escaped = false;
			for (int j = 0; escstr && escstr[j]; j++)
			{
				if (c == escstr[j])
				{
					*p++ = '\\';
					*p++ = c;
					escaped = true;
					break;
				}
			}
			if (!escaped)
				*p++ = c;
		}
		else if (c < 32)
		{
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
		}
		else
		{
			*p++ = c;
		}
	}
	if (doquote)
		*p++ = '\"';
	*p = 0;
	return s2;
}

// escape only , and "
static TCHAR* cfgfile_escape_min(const TCHAR* s)
{
	for (int i = 0; s[i]; i++)
	{
		TCHAR c = s[i];
		if (c == ',' || c == '\"')
		{
			return cfgfile_escape(s, _T(","), true);
		}
	}
	return my_strdup(s);
}

static TCHAR* getnextentry(const TCHAR** valuep, const TCHAR separator)
{
	TCHAR* s;
	const TCHAR* value = *valuep;
	if (value[0] == '\"')
	{
		s = cfgfile_unescape(value, valuep);
		value = *valuep;
		if (*value != 0 && *value != separator)
		{
			xfree (s);
			return nullptr;
		}
		value++;
		*valuep = value;
	}
	else
	{
		s = cfgfile_unescape(value, valuep, separator, false);
	}
	return s;
}

static TCHAR* cfgfile_subst_path2(const TCHAR* path, const TCHAR* subst, const TCHAR* file)
{
	/* @@@ use _tcsicmp for some targets.  */
	if (_tcslen(path) > 0 && _tcsncmp(file, path, _tcslen(path)) == 0)
	{
		int l;
		TCHAR *p2, *p = xmalloc (TCHAR, _tcslen (file) + _tcslen (subst) + 2);
		_tcscpy(p, subst);
		l = _tcslen(p);
		while (l > 0 && p[l - 1] == '/')
			p[--l] = '\0';
		l = _tcslen(path);
		while (file[l] == '/')
			l++;
		_tcscat(p, _T("/"));
		_tcscat(p, file + l);
		p2 = target_expand_environment(p, nullptr, 0);
		xfree (p);
		return p2;
	}
	return nullptr;
}

TCHAR* cfgfile_subst_path(const TCHAR* path, const TCHAR* subst, const TCHAR* file)
{
	TCHAR* s = cfgfile_subst_path2(path, subst, file);
	if (s)
		return s;
	s = target_expand_environment(file, nullptr, 0);
	return s;
}

static bool isdefault(const TCHAR* s)
{
	TCHAR tmp[MAX_DPATH];
	if (!default_file || uaeconfig)
		return false;
	zfile_fseek(default_file, 0, SEEK_SET);
	while (zfile_fgets(tmp, sizeof tmp / sizeof (TCHAR), default_file))
	{
		if (tmp[0] && tmp[_tcslen(tmp) - 1] == '\n')
			tmp[_tcslen(tmp) - 1] = 0;
		if (!_tcscmp(tmp, s))
			return true;
	}
	return false;
}

static size_t cfg_write(const void* b, struct zfile* z)
{
	size_t v;
	TCHAR lf = 10;
	v = zfile_fwrite(b, _tcslen((TCHAR*)b), sizeof (TCHAR), z);
	zfile_fwrite(&lf, 1, 1, z);
	return v;
}

#define UTF8NAME _T(".utf8")

static void cfg_dowrite(struct zfile* f, const TCHAR* option, const TCHAR* optionext, const TCHAR* value, int d,
                        int target)
{
	TCHAR tmp[CONFIG_BLEN], tmpext[CONFIG_BLEN];
	const TCHAR* optionp;

	if (value == nullptr)
		return;
	if (optionext)
	{
		_tcscpy(tmpext, option);
		_tcscat(tmpext, optionext);
		optionp = tmpext;
	}
	else
	{
		optionp = option;
	}

	if (target)
		_stprintf(tmp, _T("%s.%s=%s"), TARGET_NAME, optionp, value);
	else
		_stprintf(tmp, _T("%s=%s"), optionp, value);
	if (d && isdefault(tmp))
		return;
	cfg_write(tmp, f);
}

static void cfg_dowrite(struct zfile* f, const TCHAR* option, const TCHAR* value, int d, int target)
{
	cfg_dowrite(f, option, nullptr, value, d, target);
}

void cfgfile_write_bool(struct zfile* f, const TCHAR* option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 0, 0);
}

void cfgfile_dwrite_bool(struct zfile* f, const TCHAR* option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 1, 0);
}

static void cfgfile_dwrite_bool(struct zfile* f, const TCHAR* option, const TCHAR* optionext, bool b)
{
	cfg_dowrite(f, option, optionext, b ? _T("true") : _T("false"), 1, 0);
}

static void cfgfile_dwrite_bool(struct zfile* f, const TCHAR* option, int b)
{
	cfgfile_dwrite_bool(f, option, b != 0);
}

void cfgfile_write_str(struct zfile* f, const TCHAR* option, const TCHAR* value)
{
	cfg_dowrite(f, option, value, 0, 0);
}

static void cfgfile_write_str(struct zfile* f, const TCHAR* option, const TCHAR* optionext, const TCHAR* value)
{
	cfg_dowrite(f, option, optionext, value, 0, 0);
}

void cfgfile_dwrite_str(struct zfile* f, const TCHAR* option, const TCHAR* value)
{
	cfg_dowrite(f, option, value, 1, 0);
}

static void cfgfile_dwrite_str(struct zfile* f, const TCHAR* option, const TCHAR* optionext, const TCHAR* value)
{
	cfg_dowrite(f, option, optionext, value, 1, 0);
}

void cfgfile_target_write_bool(struct zfile* f, const TCHAR* option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 0, 1);
}

void cfgfile_target_dwrite_bool(struct zfile* f, const TCHAR* option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 1, 1);
}

void cfgfile_target_write_str(struct zfile* f, const TCHAR* option, const TCHAR* value)
{
	cfg_dowrite(f, option, value, 0, 1);
}

void cfgfile_target_dwrite_str(struct zfile* f, const TCHAR* option, const TCHAR* value)
{
	cfg_dowrite(f, option, value, 1, 1);
}

static void cfgfile_write_ext(struct zfile* f, const TCHAR* option, const TCHAR* optionext, const TCHAR* format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN], tmp2[CONFIG_BLEN];

	if (optionext)
	{
		_tcscpy(tmp2, option);
		_tcscat(tmp2, optionext);
	}
	va_start (parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, optionext ? tmp2 : option, tmp, 0, 0);
	va_end (parms);
}

void cfgfile_write(struct zfile* f, const TCHAR* option, const TCHAR* format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start (parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 0, 0);
	va_end (parms);
}

static void cfgfile_dwrite_ext(struct zfile* f, const TCHAR* option, const TCHAR* optionext, const TCHAR* format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN], tmp2[CONFIG_BLEN];

	if (optionext)
	{
		_tcscpy(tmp2, option);
		_tcscat(tmp2, optionext);
	}
	va_start (parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, optionext ? tmp2 : option, tmp, 1, 0);
	va_end (parms);
}

void cfgfile_dwrite(struct zfile* f, const TCHAR* option, const TCHAR* format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start (parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 1, 0);
	va_end (parms);
}

void cfgfile_target_write(struct zfile* f, const TCHAR* option, const TCHAR* format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start (parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 0, 1);
	va_end (parms);
}

void cfgfile_target_dwrite(struct zfile* f, const TCHAR* option, const TCHAR* format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start (parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 1, 1);
	va_end (parms);
}

static void cfgfile_write_rom(struct zfile* f, const TCHAR* path, const TCHAR* romfile, const TCHAR* name)
{
	TCHAR* str = cfgfile_subst_path(path, UNEXPANDED, romfile);
	cfgfile_write_str(f, name, str);
	struct zfile* zf = zfile_fopen(str, _T("rb"), ZFD_ALL);
	if (zf)
	{
		struct romdata* rd = getromdatabyzfile(zf);
		if (rd)
		{
			TCHAR name2[MAX_DPATH], str2[MAX_DPATH];
			_tcscpy(name2, name);
			_tcscat(name2, _T("_id"));
			_stprintf(str2, _T("%08X,%s"), rd->crc32, rd->name);
			cfgfile_write_str(f, name2, str2);
		}
		zfile_fclose(zf);
	}
	xfree (str);
}

static void cfgfile_write_path(struct zfile* f, const TCHAR* path, const TCHAR* option, const TCHAR* value)
{
	TCHAR* s = cfgfile_subst_path(path, UNEXPANDED, value);
	cfgfile_write_str(f, option, s);
	xfree (s);
}

static void write_filesys_config(struct uae_prefs* p, const TCHAR* unexpanded,
                                 const TCHAR* default_path, struct zfile* f)
{
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH], tmp3[MAX_DPATH], hdcs[MAX_DPATH];

	for (int i = 0; i < p->mountitems; i++)
	{
		struct uaedev_config_data* uci = &p->mountconfig[i];
		struct uaedev_config_info* ci = &uci->ci;
		TCHAR *str1, *str1b, *str1c, *str2b;
		const TCHAR* str2;
		int bp = ci->bootpri;

		str2 = _T("");
		if (ci->rootdir[0] == ':')
		{
			TCHAR* ptr;
			// separate harddrive names
			str1 = my_strdup(ci->rootdir);
			ptr = _tcschr(str1 + 1, ':');
			if (ptr)
			{
				*ptr++ = 0;
				str2 = ptr;
				ptr = (TCHAR *)_tcschr(str2, ',');
				if (ptr)
					*ptr = 0;
			}
		}
		else
		{
			str1 = cfgfile_subst_path(default_path, unexpanded, ci->rootdir);
		}
		int ct = ci->controller_type;
		int romtype = 0;
		if (ct >= HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST && ct <= HD_CONTROLLER_TYPE_SCSI_LAST)
		{
			_stprintf(hdcs, _T("scsi%d_%s"), ci->controller_unit,
			          expansionroms[ct - HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST].name);
			romtype = expansionroms[ct - HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST].romtype;
		}
		else if (ct >= HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST && ct <= HD_CONTROLLER_TYPE_IDE_LAST)
		{
			_stprintf(hdcs, _T("ide%d_%s"), ci->controller_unit,
			          expansionroms[ct - HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST].name);
			romtype = expansionroms[ct - HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST].romtype;
		}
		else if (ct == HD_CONTROLLER_TYPE_SCSI_AUTO)
		{
			_stprintf(hdcs, _T("scsi%d"), ci->controller_unit);
		}
		else if (ct == HD_CONTROLLER_TYPE_IDE_AUTO)
		{
			_stprintf(hdcs, _T("ide%d"), ci->controller_unit);
		}
		else if (ct == HD_CONTROLLER_TYPE_PCMCIA)
		{
			if (ci->controller_type_unit == 0)
				_tcscpy(hdcs, _T("scsram"));
			else
				_tcscpy(hdcs, _T("scide"));
		}
		else if (ct == HD_CONTROLLER_TYPE_UAE)
		{
			_stprintf(hdcs, _T("uae%d"), ci->controller_unit);
		}
		if (romtype)
		{
			for (int j = 0; hdcontrollers[j].label; j++)
			{
				if (hdcontrollers[j].romtype == (romtype & ROMTYPE_MASK))
				{
					_stprintf(hdcs, hdcontrollers[j].label, ci->controller_unit);
					break;
				}
			}
		}
		if (ci->controller_type_unit > 0 && ct != HD_CONTROLLER_TYPE_PCMCIA)
			_stprintf(hdcs + _tcslen(hdcs), _T("-%d"), ci->controller_type_unit + 1);

		str1b = cfgfile_escape(str1, _T(":,"), true);
		str1c = cfgfile_escape_min(str1);
		str2b = cfgfile_escape(str2, _T(":,"), true);
		if (ci->type == UAEDEV_DIR)
		{
			_stprintf(tmp, _T("%s,%s:%s:%s,%d"), ci->readonly ? _T("ro") : _T("rw"),
			          ci->devname ? ci->devname : _T(""), ci->volname, str1c, bp);
			cfgfile_write_str(f, _T("filesystem2"), tmp);
			_tcscpy(tmp3, tmp);
		}
		else if (ci->type == UAEDEV_HDF)
		{
			TCHAR filesyspath[MAX_DPATH];
			_tcscpy(filesyspath, ci->filesys);
			TCHAR* sfilesys = cfgfile_escape_min(filesyspath);
			_stprintf(tmp, _T("%s,%s:%s,%d,%d,%d,%d,%d,%s,%s"),
			          ci->readonly ? _T("ro") : _T("rw"),
			          ci->devname ? ci->devname : _T(""), str1c,
			          ci->sectors, ci->surfaces, ci->reserved, ci->blocksize,
			          bp, ci->filesys[0] ? sfilesys : _T(""), hdcs);
			_stprintf(tmp3, _T("%s,%s:%s%s%s,%d,%d,%d,%d,%d,%s,%s"),
			          ci->readonly ? _T("ro") : _T("rw"),
			          ci->devname ? ci->devname : _T(""), str1b, str2b[0] ? _T(":") : _T(""), str2b,
			          ci->sectors, ci->surfaces, ci->reserved, ci->blocksize,
			          bp, ci->filesys[0] ? sfilesys : _T(""), hdcs);
			if (ci->highcyl || ci->physical_geometry)
			{
				TCHAR* s = tmp + _tcslen(tmp);
				TCHAR* s2 = s;
				_stprintf(s2, _T(",%d"), ci->highcyl);
				if (ci->physical_geometry && ci->pheads && ci->psecs)
				{
					s = tmp + _tcslen(tmp);
					_stprintf(s, _T(",%d/%d/%d"), ci->pcyls, ci->pheads, ci->psecs);
				}
				_tcscat(tmp3, s2);
				xfree(sfilesys);
			}
			if (ci->controller_media_type)
			{
				_tcscat(tmp, _T(",CF"));
				_tcscat(tmp3, _T(",CF"));
			}
			const TCHAR* extras = nullptr;
			if (ct >= HD_CONTROLLER_TYPE_IDE_FIRST && ct <= HD_CONTROLLER_TYPE_IDE_LAST)
			{
				if (ci->unit_feature_level == HD_LEVEL_ATA_1)
				{
					extras = _T("ATA1");
				}
				else if (ci->unit_feature_level == HD_LEVEL_ATA_2S)
				{
					extras = _T("ATA2+S");
				}
			}
			if (extras)
			{
				_tcscat(tmp, _T(","));
				_tcscat(tmp3, _T(","));
				_tcscat(tmp, extras);
				_tcscat(tmp3, extras);
			}
			if (ci->unit_special_flags)
			{
				TCHAR tmpx[32];
				_stprintf(tmpx, _T(",flags=0x%x"), ci->unit_special_flags);
				_tcscat(tmp, tmpx);
				_tcscat(tmp3, tmpx);
			}
			if (ci->lock)
			{
				_tcscat(tmp, _T(",lock"));
				_tcscat(tmp3, _T(",lock"));
			}

			if (ci->type == UAEDEV_HDF)
				cfgfile_write_str(f, _T("hardfile2"), tmp);
		}
		_stprintf(tmp2, _T("uaehf%d"), i);
		cfgfile_write(f, tmp2, _T("%s,%s"), ci->type == UAEDEV_HDF ? _T("hdf") : _T("dir"), tmp3);
		if (ci->type == UAEDEV_DIR)
		{
			bool add_extra = false;
			if (ci->inject_icons)
			{
				add_extra = true;
			}
			if (add_extra)
			{
				_stprintf(tmp2, _T("%s,inject_icons=%s"), ci->devname, ci->inject_icons ? _T("true") : _T("false"));
				cfgfile_write(f, _T("filesystem_extra"), tmp2);
			}
		}
		xfree(str1b);
		xfree(str1c);
		xfree(str2b);
		xfree(str1);
	}
}

static void write_compatibility_cpu(struct zfile* f, struct uae_prefs* p)
{
	TCHAR tmp[100];
	int model;

	model = p->cpu_model;
	if (model == 68030)
		model = 68020;
	if (p->address_space_24 && model == 68020)
		_tcscpy(tmp, _T("68ec020"));
	else
		_stprintf(tmp, _T("%d"), model);
	if (model == 68020 && (p->fpu_model == 68881 || p->fpu_model == 68882))
		_tcscat(tmp, _T("/68881"));
	cfgfile_write(f, _T("cpu_type"), tmp);
}

static void write_resolution(struct zfile* f, const TCHAR* ws, const TCHAR* hs, struct wh* wh)
{
	cfgfile_write(f, ws, _T("%d"), wh->width);
	cfgfile_write(f, hs, _T("%d"), wh->height);
}

static void cfgfile_write_board_rom(struct uae_prefs* prefs, struct zfile* f, const TCHAR* path,
                                    struct boardromconfig* br)
{
	TCHAR buf[MAX_DPATH];
	TCHAR name[256];
	const struct expansionromtype* ert;

	if (br->device_type == 0)
		return;
	ert = get_device_expansion_rom(br->device_type);
	if (!ert)
		return;
	for (int i = 0; i < MAX_BOARD_ROMS; i++)
	{
		if (br->device_num == 0)
			_tcscpy(name, ert->name);
		else
			_stprintf(name, _T("%s-%d"), ert->name, br->device_num + 1);
		if (i == 0 || _tcslen(br->roms[i].romfile))
		{
			_stprintf(buf, _T("%s%s_rom_file"), name, i ? _T("_ext") : _T(""));
			cfgfile_write_rom(f, path, br->roms[i].romfile, buf);
			if (br->roms[i].romident[0])
			{
				_stprintf(buf, _T("%s%s_rom"), name, i ? _T("_ext") : _T(""));
				cfgfile_dwrite_str(f, buf, br->roms[i].romident);
			}
			if (br->roms[i].autoboot_disabled || br->device_order > 0)
			{
				TCHAR buf2[256], *p;
				buf2[0] = 0;
				p = buf2;
				_stprintf(buf, _T("%s%s_rom_options"), name, i ? _T("_ext") : _T(""));
				if (br->roms[i].autoboot_disabled)
				{
					if (buf2[0])
						_tcscat(buf2, _T(","));
					_tcscat(buf2, _T("autoboot_disabled=true"));
				}
				if (buf2[0])
					cfgfile_dwrite_str(f, buf, buf2);
			}

			if (br->roms[i].board_ram_size)
			{
				_stprintf(buf, _T("%s%s_mem_size"), name, i ? _T("_ext") : _T(""));
				cfgfile_write(f, buf, _T("%d"), br->roms[i].board_ram_size / 0x40000);
			}
		}
	}
}

static bool cfgfile_readramboard(const TCHAR* option, const TCHAR* value, const TCHAR* name, struct ramboard* rbp)
{
	TCHAR tmp1[MAX_DPATH];
	int v;
	for (int i = 0; i < MAX_RAM_BOARDS; i++)
	{
		struct ramboard* rb = &rbp[i];
		if (i > 0)
			_stprintf(tmp1, _T("%s%d_size"), name, i + 1);
		else
			_stprintf(tmp1, _T("%s_size"), name);
		if (!_tcsicmp(option, tmp1))
		{
			v = 0;
			cfgfile_intval(option, value, tmp1, &v, 0x100000);
			rb->size = v;
			return true;
		}
		if (i > 0)
			_stprintf(tmp1, _T("%s%d_size_k"), name, i + 1);
		else
			_stprintf(tmp1, _T("%s_size_k"), name);
		if (!_tcsicmp(option, tmp1))
		{
			v = 0;
			cfgfile_intval(option, value, tmp1, &v, 1024);
			rb->size = v;
			return true;
		}
	}
	return false;
}

static void cfgfile_writeramboard(struct uae_prefs* prefs, struct zfile* f, const TCHAR* name, int num,
                                  struct ramboard* rb)
{
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	if (num > 0)
		_stprintf(tmp1, _T("%s%d_options"), name, num + 1);
	else
		_stprintf(tmp1, _T("%s_options"), name);
	tmp2[0] = 0;
	TCHAR* p = tmp2;
	if (rb->manufacturer)
	{
		if (tmp2[0])
			*p++ = ',';
		_stprintf(p, _T("mid=%u,pid=%u"), rb->manufacturer, rb->product);
		p += _tcslen(p);
	}
	if (tmp2[0])
	{
		cfgfile_write(f, tmp1, tmp2);
	}
}

void cfgfile_save_options(struct zfile* f, struct uae_prefs* p, int type)
{
	struct strlist* sl;
	TCHAR tmp[MAX_DPATH];
	int i;

	cfgfile_write_str(f, _T("config_description"), p->description);
	cfgfile_write_bool(f, _T("config_hardware"), type & CONFIG_TYPE_HARDWARE);
	cfgfile_write_bool(f, _T("config_host"), !!(type & CONFIG_TYPE_HOST));
	if (p->info[0])
		cfgfile_write(f, _T("config_info"), p->info);
	cfgfile_write(f, _T("config_version"), _T("%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);

	for (sl = p->all_lines; sl; sl = sl->next)
	{
		if (sl->unknown)
		{
			if (sl->option)
				cfgfile_write_str(f, sl->option, sl->value);
		}
	}

	_stprintf(tmp, _T("%s.rom_path"), TARGET_NAME);
	cfgfile_write_str(f, tmp, p->path_rom);
	_stprintf(tmp, _T("%s.floppy_path"), TARGET_NAME);
	cfgfile_write_str(f, tmp, p->path_floppy);
	_stprintf(tmp, _T("%s.hardfile_path"), TARGET_NAME);
	cfgfile_write_str(f, tmp, p->path_hardfile);
	_stprintf(tmp, _T("%s.cd_path"), TARGET_NAME);
	cfgfile_write_str(f, tmp, p->path_cd);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Controller/Input Configuration"), f);
	cfg_write(_T("; "), f);

	for (i = 0; i < MAX_JPORTS; i++)
	{
		struct jport* jp = &p->jports[i];
		int v = jp->id;
		TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		if (v == JPORT_NONE)
		{
			_tcscpy(tmp2, _T("none"));
		}
		else if (v < JSEM_CUSTOM)
		{
			_stprintf(tmp2, _T("kbd%d"), v + 1);
		}
		else if (v < JSEM_JOYS)
		{
			_stprintf(tmp2, _T("custom%d"), v - JSEM_CUSTOM);
		}
		else if (v < JSEM_MICE)
		{
			_stprintf(tmp2, _T("joy%d"), v - JSEM_JOYS);
		}
		else
		{
			_tcscpy(tmp2, _T("mouse"));
			if (v - JSEM_MICE > 0)
				_stprintf(tmp2, _T("mouse%d"), v - JSEM_MICE);
		}
		if (i < 2 || jp->id >= 0)
		{
			_stprintf(tmp1, _T("joyport%d"), i);
			cfgfile_write(f, tmp1, tmp2);

			_stprintf(tmp1, _T("joyport%d_autofire"), i);
			cfgfile_write(f, tmp1, joyaf[jp->autofire]);

			if (jp->mode > 0)
			{
				_stprintf(tmp1, _T("joyport%d_mode"), i);
				cfgfile_dwrite_str(f, tmp1, joyportmodes[jp->mode]);
			}

			if (jp->mousemap > 0)
			{
				_stprintf(tmp1, _T("joyport%d_mousemap"), i);
				cfgfile_dwrite_str(f, tmp1, mousemaps[jp->mousemap]);
			}

			if (jp->idc.name[0])
			{
				_stprintf(tmp1, _T("joyport%d_friendlyname"), i);
				cfgfile_write(f, tmp1, jp->idc.name);
			}

			if (jp->idc.configname[0])
			{
				_stprintf(tmp1, _T("joyport%d_name"), i);
				cfgfile_write(f, tmp1, jp->idc.configname);
			}

			if (jp->nokeyboardoverride)
			{
				_stprintf(tmp1, _T("joyport%d_keyboardoverride"), i);
				cfgfile_write_bool(f, tmp1, !jp->nokeyboardoverride);
			}
		}

		// custom options SAVING
		if (i < 4)
		{
			struct joypad_map_layout tempcustom;
			const TCHAR* namecustom;

			// this allows us to go through the available function keys
			// currently only 'none' and 'hotkey'
			for (int m = 0; m < 2; ++m)
			{
				switch (m)
				{
				case 0:
					{
						tempcustom = jp->amiberry_custom_none;
						namecustom = _T("_amiberry_custom_none_");
						break;
					}
				case 1:
					{
						tempcustom = jp->amiberry_custom_hotkey;
						namecustom = _T("_amiberry_custom_hotkey_");
						break;
					}
				}

				// get all of the custom actions
				for (int n = 0; n < 14; ++n) // loop through all buttons
				{
					int b = 0;
					switch (n)
					{
					case 0:
						{
							b = tempcustom.dpad_up_action;
							break;
						}
					case 1:
						{
							b = tempcustom.dpad_down_action;
							break;
						}
					case 2:
						{
							b = tempcustom.dpad_left_action;
							break;
						}
					case 3:
						{
							b = tempcustom.dpad_right_action;
							break;
						}
					case 4:
						{
							b = tempcustom.select_action;
							break;
						}
					case 5:
						{
							b = tempcustom.left_shoulder_action;
							break;
						}
					case 6:
						{
							b = tempcustom.lstick_select_action;
							break;
						}
					case 7:
						{
							b = tempcustom.north_action;
							break;
						}
					case 8:
						{
							b = tempcustom.south_action;
							break;
						}
					case 9:
						{
							b = tempcustom.east_action;
							break;
						}
					case 10:
						{
							b = tempcustom.west_action;
							break;
						}
					case 11:
						{
							b = tempcustom.start_action;
							break;
						}
					case 12:
						{
							b = tempcustom.right_shoulder_action;
							break;
						}
					case 13:
						{
							b = tempcustom.rstick_select_action;
							break;
						}
					}

					if (b > 0) { _tcscpy(tmp2, _T(find_inputevent_name(b))); }
					else { snprintf(tmp2, 1, "%s", ""); }

					_stprintf(tmp1, "joyport%d%s%s", i, namecustom, button_remap_name[n]);

					// cfgfile_write (f, tmp1, tmp2); }
					cfgfile_dwrite_str(f, tmp1, tmp2);
				}
			}
		}

		cfg_write(_T("; "), f);
	}

	cfgfile_dwrite_str(f, _T("absolute_mouse"), abspointers[p->input_tablet]);

	write_inputdevice_config(p, f);

	cfgfile_write_str(f, _T("kbd_lang"), (p->keyboard_lang == KBD_LANG_DE
		                                      ? _T("de")
		                                      : p->keyboard_lang == KBD_LANG_DK
		                                      ? _T("dk")
		                                      : p->keyboard_lang == KBD_LANG_ES
		                                      ? _T("es")
		                                      : p->keyboard_lang == KBD_LANG_US
		                                      ? _T("us")
		                                      : p->keyboard_lang == KBD_LANG_SE
		                                      ? _T("se")
		                                      : p->keyboard_lang == KBD_LANG_FR
		                                      ? _T("fr")
		                                      : p->keyboard_lang == KBD_LANG_IT
		                                      ? _T("it")
		                                      : _T("FOO")));

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Host-Specific"), f);
	cfg_write(_T("; "), f);

	target_save_options(f, p);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Common / Paths"), f);
	cfg_write(_T("; "), f);

	cfgfile_write_str(f, _T("use_gui"), guimode1[p->start_gui]);
	cfgfile_dwrite_bool(f, _T("show_leds"), p->leds_on_screen);

	cfgfile_write_rom(f, p->path_rom, p->romfile, _T("kickstart_rom_file"));
	cfgfile_write_rom(f, p->path_rom, p->romextfile, _T("kickstart_ext_rom_file"));
	if (p->romident[0])
		cfgfile_dwrite_str(f, _T("kickstart_rom"), p->romident);
	if (p->romextident[0])
		cfgfile_write_str(f, _T("kickstart_ext_rom="), p->romextident);

	for (int i = 0; i < MAX_EXPANSION_BOARDS; i++)
	{
		cfgfile_write_board_rom(p, f, p->path_rom, &p->expansionboard[i]);
	}

	cfgfile_write_str(f, _T("flash_file"), p->flashfile);
	cfgfile_write_str(f, _T("cart_file"), p->cartfile);
	if (p->cartident[0])
		cfgfile_write_str(f, _T("cart"), p->cartident);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Floppy Drives"), f);
	cfg_write(_T("; "), f);

	p->nr_floppies = 4;
	for (i = 0; i < 4; i++)
	{
		_stprintf(tmp, _T("floppy%d"), i);
		cfgfile_write_path(f, p->path_floppy, tmp, p->floppyslots[i].df);
		_stprintf(tmp, _T("floppy%dwp"), i);
		cfgfile_dwrite_bool(f, tmp, p->floppyslots[i].forcedwriteprotect);
		_stprintf(tmp, _T("floppy%dtype"), i);
		cfgfile_dwrite(f, tmp, _T("%d"), p->floppyslots[i].dfxtype);
		if (p->floppyslots[i].dfxtype < 0 && p->nr_floppies > i)
			p->nr_floppies = i;
	}

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++)
	{
		if (p->cdslots[i].name[0] || p->cdslots[i].inuse)
		{
			TCHAR tmp2[MAX_DPATH];
			_stprintf(tmp, _T("cdimage%d"), i);
			TCHAR* s = cfgfile_subst_path(p->path_cd, UNEXPANDED, p->cdslots[i].name);
			_tcscpy(tmp2, s);
			xfree(s);
			if (p->cdslots[i].type != SCSI_UNIT_DEFAULT || _tcschr(p->cdslots[i].name, ',') || p->cdslots[i].delayed)
			{
				_tcscat(tmp2, _T(","));
				if (p->cdslots[i].delayed)
				{
					_tcscat(tmp2, _T("delay"));
					_tcscat(tmp2, _T(":"));
				}
				if (p->cdslots[i].type != SCSI_UNIT_DEFAULT)
				{
					_tcscat(tmp2, cdmodes[p->cdslots[i].type + 1]);
				}
			}
			cfgfile_write_str(f, tmp, tmp2);
		}
	}

	cfgfile_write(f, _T("nr_floppies"), _T("%d"), p->nr_floppies);
	cfgfile_dwrite_bool(f, _T("floppy_write_protect"), p->floppy_read_only);
	cfgfile_write(f, _T("floppy_speed"), _T("%d"), p->floppy_speed);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Hard Drives"), f);
	cfg_write(_T("; "), f);

#ifdef FILESYS
	write_filesys_config(p, UNEXPANDED, p->path_hardfile, f);
	cfgfile_dwrite(f, _T("filesys_max_size"), _T("%d"), p->filesys_limit);
	cfgfile_dwrite(f, _T("filesys_max_name_length"), _T("%d"), p->filesys_max_name);
	cfgfile_dwrite_bool(f, _T("filesys_inject_icons"), p->filesys_inject_icons);
	cfgfile_dwrite_str(f, _T("filesys_inject_icons_drawer"), p->filesys_inject_icons_drawer);
	cfgfile_dwrite_str(f, _T("filesys_inject_icons_project"), p->filesys_inject_icons_project);
	cfgfile_dwrite_str(f, _T("filesys_inject_icons_tool"), p->filesys_inject_icons_tool);
#endif
	cfgfile_dwrite_bool(f, _T("harddrive_write_protect"), p->harddrive_read_only);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** CD / CD32"), f);
	cfg_write(_T("; "), f);

	cfgfile_write(f, _T("cd_speed"), _T("%d"), p->cd_speed);
	cfgfile_dwrite_bool(f, _T("cd32cd"), p->cs_cd32cd);
	cfgfile_dwrite_bool(f, _T("cd32c2p"), p->cs_cd32c2p);
	cfgfile_dwrite_bool(f, _T("cd32nvram"), p->cs_cd32nvram);
	cfgfile_dwrite(f, _T("cd32nvram_size"), _T("%d"), p->cs_cd32nvram_size / 1024);


	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Display / Screen Setup"), f);
	cfg_write(_T("; "), f);

	cfgfile_write(f, _T("gfx_framerate"), _T("%d"), p->gfx_framerate);
	write_resolution(f, _T("gfx_width"), _T("gfx_height"), &p->gfx_size); /* compatibility with old versions */
	cfgfile_write(f, _T("gfx_refreshrate"), _T("%d"), p->gfx_apmode[0].gfx_refreshrate);
	cfgfile_dwrite(f, _T("gfx_refreshrate_rtg"), _T("%d"), p->gfx_apmode[1].gfx_refreshrate);

	cfgfile_write_bool(f, _T("gfx_lores"), p->gfx_resolution == 0);
	cfgfile_write_str(f, _T("gfx_resolution"), lorestype1[p->gfx_resolution]);
	cfgfile_write_str(f, _T("gfx_lores_mode"), loresmode[p->gfx_lores_mode]);
	cfgfile_write_str(f, _T("gfx_linemode"), p->gfx_vresolution > 0 ? linemode[p->gfx_iscanlines * 4 + p->gfx_pscanlines + 1] : linemode[0]);
	cfgfile_write_str(f, _T("gfx_fullscreen_amiga"), fullmodes[p->gfx_apmode[0].gfx_fullscreen]);
	cfgfile_write_str(f, _T("gfx_fullscreen_picasso"), fullmodes[p->gfx_apmode[1].gfx_fullscreen]);

	cfgfile_write_bool(f, _T("ntsc"), p->ntscmode);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** CPU options"), f);
	cfg_write(_T("; "), f);

	if (p->m68k_speed > 0)
	{
		cfgfile_write(f, _T("finegrain_cpu_speed"), _T("%d"), p->m68k_speed);
	}
	else
	{
		cfgfile_write_str(f, _T("cpu_speed"), (p->m68k_speed > -30 ? (p->m68k_speed < 0 ? _T("max") : _T("real")) : "turbo"));
	}

	/* do not reorder start */
	write_compatibility_cpu(f, p);
	cfgfile_write(f, _T("cpu_model"), _T("%d"), p->cpu_model);
	if (p->fpu_model)
		cfgfile_write(f, _T("fpu_model"), _T("%d"), p->fpu_model);
	cfgfile_write_bool(f, _T("cpu_compatible"), p->cpu_compatible);
	cfgfile_write_bool(f, _T("cpu_24bit_addressing"), p->address_space_24);
	/* do not reorder end */

	cfgfile_dwrite_bool(f, _T("fpu_no_unimplemented"), p->fpu_no_unimplemented);
	cfgfile_write_bool(f, _T("fpu_strict"), p->fpu_strict);

#ifdef USE_JIT_FPU
	cfgfile_write_bool(f, _T("compfpu"), p->compfpu);
#endif
	cfgfile_write(f, _T("cachesize"), _T("%d"), p->cachesize);

	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Memory"), f);
	cfg_write(_T("; "), f);

	cfgfile_write(f, _T("chipmem_size"), _T("%d"),
	              p->chipmem_size == 0x20000 ? -1 : (p->chipmem_size == 0x40000 ? 0 : p->chipmem_size / 0x80000));


	cfgfile_dwrite_bool(f, _T("z3_autoconfig"), p->cs_z3autoconfig);
	cfgfile_dwrite_str(f, _T("z3mapping"), z3mapping[p->z3_mapping_mode]);
	for (int i = 0; i < MAX_RAM_BOARDS; i++)
	{
		if (p->fastmem[i].size < 0x100000 && p->fastmem[i].size)
		{
			if (i > 0)
				_stprintf(tmp, _T("fastmem%d_size_k"), i + 1);
			else
				_tcscpy(tmp, _T("fastmem_size_k"));
			cfgfile_write(f, tmp, _T("%d"), p->fastmem[i].size / 1024);
		}
		else if (p->fastmem[i].size || i == 0)
		{
			if (i > 0)
				_stprintf(tmp, _T("fastmem%d_size"), i + 1);
			else
				_tcscpy(tmp, _T("fastmem_size"));
			cfgfile_write(f, tmp, _T("%d"), p->fastmem[i].size / 0x100000);
		}
		cfgfile_writeramboard(p, f, _T("fastmem"), i, &p->fastmem[i]);
	}
	cfgfile_write(f, _T("a3000mem_size"), _T("%d"), p->mbresmem_low_size / 0x100000);
	cfgfile_write(f, _T("mbresmem_size"), _T("%d"), p->mbresmem_high_size / 0x100000);
	for (int i = 0; i < MAX_RAM_BOARDS; i++)
	{
		if (i == 0 || p->z3fastmem[i].size)
		{
			if (i > 0)
				_stprintf(tmp, _T("z3mem%d_size"), i + 1);
			else
				_tcscpy(tmp, _T("z3mem_size"));
			cfgfile_write(f, tmp, _T("%d"), p->z3fastmem[i].size / 0x100000);
		}
		cfgfile_writeramboard(p, f, _T("z3mem"), i, &p->z3fastmem[i]);
	}
	cfgfile_write(f, _T("z3mem_start"), _T("0x%x"), p->z3autoconfig_start);
	cfgfile_write(f, _T("bogomem_size"), _T("%d"), p->bogomem_size / 0x40000);
	for (int i = 0; i < MAX_RTG_BOARDS; i++)
	{
		struct rtgboardconfig* rbc = &p->rtgboards[i];
		if (rbc->rtgmem_size)
		{
			if (i > 0)
				_stprintf(tmp, _T("gfxcard%d_size"), i + 1);
			else
				_tcscpy(tmp, _T("gfxcard_size"));
			cfgfile_write(f, tmp, _T("%d"), rbc->rtgmem_size / 0x100000);
			if (i > 0)
				_stprintf(tmp, _T("gfxcard%d_type"), i + 1);
			else
				_tcscpy(tmp, _T("gfxcard_type"));
			cfgfile_dwrite_str(f, tmp, gfxboard_get_configname(rbc->rtgmem_type));
		}
	}
	cfgfile_write(f, _T("rtg_modes"), _T("0x%x"), p->picasso96_modeflags);


	// do not save aros rom special space
	if (!(p->custom_memory_sizes[0] == 512 * 1024 && p->custom_memory_sizes[1] == 512 * 1024 && p->custom_memory_addrs[0]
		== 0xa80000 && p->custom_memory_addrs[1] == 0xb00000))
	{
		if (p->custom_memory_sizes[0])
			cfgfile_write(f, _T("addmem1"), _T("0x%x,0x%x"), p->custom_memory_addrs[0], p->custom_memory_sizes[0]);
		if (p->custom_memory_sizes[1])
			cfgfile_write(f, _T("addmem2"), _T("0x%x,0x%x"), p->custom_memory_addrs[1], p->custom_memory_sizes[1]);
	}


	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Chipset"), f);
	cfg_write(_T("; "), f);

	if (p->chipset_mask & CSMASK_AGA)
		cfgfile_write(f, _T("chipset"), _T("aga"));
	else if ((p->chipset_mask & CSMASK_ECS_AGNUS) && (p->chipset_mask & CSMASK_ECS_DENISE))
		cfgfile_write(f, _T("chipset"), _T("ecs"));
	else if (p->chipset_mask & CSMASK_ECS_AGNUS)
		cfgfile_write(f, _T("chipset"), _T("ecs_agnus"));
	else if (p->chipset_mask & CSMASK_ECS_DENISE)
		cfgfile_write(f, _T("chipset"), _T("ecs_denise"));
	else
		cfgfile_write(f, _T("chipset"), _T("ocs"));

	if (p->chipset_refreshrate > 0)
		cfgfile_write(f, _T("chipset_refreshrate"), _T("%f"), p->chipset_refreshrate);

	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++)
	{
		struct chipset_refresh* cr = &p->cr[i];
		if (!cr->inuse)
			continue;
		cr->index = i;
		if (cr->rate == 0)
			_tcscpy(tmp, _T("0"));
		else
			_stprintf(tmp, _T("%f"), cr->rate);
		TCHAR* s = tmp + _tcslen(tmp);
		if (cr->label[0] > 0 && i < MAX_CHIPSET_REFRESH)
			s += _stprintf(s, _T(",t=%s"), cr->label);
		if (cr->horiz > 0)
			s += _stprintf(s, _T(",h=%d"), cr->horiz);
		if (cr->vert > 0)
			s += _stprintf(s, _T(",v=%d"), cr->vert);
		if (cr->locked)
			_tcscat(s, _T(",locked"));
		if (cr->ntsc > 0)
			_tcscat(s, _T(",ntsc"));
		else if (cr->ntsc == 0)
			_tcscat(s, _T(",pal"));
		if (cr->lace > 0)
			_tcscat(s, _T(",lace"));
		else if (cr->lace == 0)
			_tcscat(s, _T(",nlace"));
		if ((cr->resolution & 7) != 7)
		{
			if (cr->resolution & 1)
				_tcscat(s, _T(",lores"));
			if (cr->resolution & 2)
				_tcscat(s, _T(",hires"));
			if (cr->resolution & 4)
				_tcscat(s, _T(",shres"));
		}
		if (cr->vsync > 0)
			_tcscat(s, _T(",vsync"));
		else if (cr->vsync == 0)
			_tcscat(s, _T(",nvsync"));
		if (cr->rtg)
			_tcscat(s, _T(",rtg"));
		if (cr->defaultdata)
			_tcscat(s, _T(",default"));
		if (i == CHIPSET_REFRESH_PAL)
		{
			cfgfile_dwrite(f, _T("displaydata_pal"), tmp);
		}
		else if (i == CHIPSET_REFRESH_NTSC)
		{
			cfgfile_dwrite(f, _T("displaydata_ntsc"), tmp);
		}
		else
		{
			cfgfile_dwrite(f, _T("displaydata"), tmp);
		}
	}

	cfgfile_write_str(f, _T("collision_level"), collmode[p->collision_level]);

	cfgfile_write_str(f, _T("chipset_compatible"), cscompa[p->cs_compatible]);
	cfgfile_dwrite_str(f, _T("ciaatod"), ciaatodmode[p->cs_ciaatod]);
	cfgfile_dwrite_str(f, _T("rtc"), rtctype[p->cs_rtc]);
	cfgfile_dwrite_bool(f, _T("ksmirror_e0"), p->cs_ksmirror_e0);
	cfgfile_dwrite_bool(f, _T("ksmirror_a8"), p->cs_ksmirror_a8);
	cfgfile_dwrite_bool(f, _T("cd32cd"), p->cs_cd32cd);
	cfgfile_dwrite_bool(f, _T("cd32c2p"), p->cs_cd32c2p);
	cfgfile_dwrite_bool(f, _T("cd32nvram"), p->cs_cd32nvram);
	cfgfile_dwrite(f, _T("cd32nvram_size"), _T("%d"), p->cs_cd32nvram_size / 1024);
	cfgfile_dwrite(f, _T("fatgary"), _T("%d"), p->cs_fatgaryrev);
	cfgfile_dwrite(f, _T("ramsey"), _T("%d"), p->cs_ramseyrev);
	cfgfile_dwrite_bool(f, _T("pcmcia"), p->cs_pcmcia);
	cfgfile_dwrite_bool(f, _T("cia_todbug"), p->cs_ciatodbug);
	cfgfile_write_bool(f, _T("immediate_blits"), p->immediate_blits);
	cfgfile_dwrite_str(f, _T("waiting_blits"), waitblits[p->waiting_blits]);
	cfgfile_write_bool(f, _T("fast_copper"), p->fast_copper);


	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Sound Options"), f);
	cfg_write(_T("; "), f);

	cfgfile_write_str(f, _T("sound_output"), soundmode1[p->produce_sound]);
	cfgfile_write_str(f, _T("sound_channels"), stereomode[p->sound_stereo]);
	cfgfile_write(f, _T("sound_stereo_separation"), _T("%d"), p->sound_stereo_separation);
	cfgfile_write(f, _T("sound_stereo_mixing_delay"), _T("%d"),
	              p->sound_mixed_stereo_delay >= 0 ? p->sound_mixed_stereo_delay : 0);
	cfgfile_write(f, _T("sound_frequency"), _T("%d"), p->sound_freq);
	cfgfile_write_str(f, _T("sound_interpol"), interpolmode[p->sound_interpol]);
	cfgfile_write_str(f, _T("sound_filter"), soundfiltermode1[p->sound_filter]);
	cfgfile_write_str(f, _T("sound_filter_type"), soundfiltermode2[p->sound_filter_type]);
	if (p->sound_volume_cd >= 0)
		cfgfile_write(f, _T("sound_volume_cd"), _T("%d"), p->sound_volume_cd);


	cfg_write(_T("; "), f);
	cfg_write(_T("; *** Misc. Options"), f);
	cfg_write(_T("; "), f);

	cfgfile_write_bool(f, _T("bsdsocket_emu"), p->socket_emu);
	cfgfile_dwrite_str(f, _T("boot_rom_uae"), uaebootrom[p->boot_rom]);
	cfgfile_dwrite_str(f, _T("uaeboard"), uaeboard[p->uaeboard]);
}

static int cfgfile_yesno(const TCHAR* option, const TCHAR* value, const TCHAR* name, int* location, bool numbercheck)
{
	if (name != nullptr && _tcscmp(option, name) != 0)
		return 0;
	if (_tcsicmp(value, _T("yes")) == 0 || _tcsicmp(value, _T("y")) == 0
		|| _tcsicmp(value, _T("true")) == 0 || _tcsicmp(value, _T("t")) == 0
		|| (numbercheck && _tcsicmp(value, _T("1")) == 0))
		*location = 1;
	else if (_tcsicmp(value, _T("no")) == 0 || _tcsicmp(value, _T("n")) == 0
		|| _tcsicmp(value, _T("false")) == 0 || _tcsicmp(value, _T("f")) == 0
		|| (numbercheck && _tcsicmp(value, _T("0")) == 0))
		*location = 0;
	else
	{
		cfgfile_warning(_T("Option '%s' requires a value of either 'true' or 'false' (was '%s').\n"), option, value);
		return -1;
	}
	return 1;
}

static int cfgfile_yesno(const TCHAR* option, const TCHAR* value, const TCHAR* name, int* location)
{
	return cfgfile_yesno(option, value, name, location, true);
}

static int cfgfile_yesno(const TCHAR* option, const TCHAR* value, const TCHAR* name, bool* location, bool numbercheck)
{
	int val;
	int ret = cfgfile_yesno(option, value, name, &val, numbercheck);
	if (ret == 0)
		return 0;
	if (ret < 0)
		*location = false;
	else
		*location = val != 0;
	return 1;
}

int cfgfile_yesno(const TCHAR* option, const TCHAR* value, const TCHAR* name, bool* location)
{
	return cfgfile_yesno(option, value, name, location, true);
}

static int cfgfile_doubleval(const TCHAR* option, const TCHAR* value, const TCHAR* name, double* location)
{
	TCHAR* endptr;
	if (name != nullptr && _tcscmp(option, name) != 0)
		return 0;
	*location = _tcstod(value, &endptr);
	return 1;
}

static int cfgfile_floatval(const TCHAR* option, const TCHAR* value, const TCHAR* name, const TCHAR* nameext,
                            float* location)
{
	TCHAR* endptr;
	if (name == nullptr)
		return 0;
	if (nameext)
	{
		TCHAR tmp[MAX_DPATH];
		_tcscpy(tmp, name);
		_tcscat(tmp, nameext);
		if (_tcscmp(tmp, option) != 0)
			return 0;
	}
	else
	{
		if (_tcscmp(option, name) != 0)
			return 0;
	}
	*location = (float)_tcstod(value, &endptr);
	return 1;
}

static int cfgfile_floatval(const TCHAR* option, const TCHAR* value, const TCHAR* name, float* location)
{
	return cfgfile_floatval(option, value, name, nullptr, location);
}

static int cfgfile_intval(const TCHAR* option, const TCHAR* value, const TCHAR* name, const TCHAR* nameext,
                          unsigned int* location, int scale)
{
	int base = 10;
	TCHAR* endptr;
	TCHAR tmp[MAX_DPATH];

	if (name == nullptr)
		return 0;
	if (nameext)
	{
		_tcscpy(tmp, name);
		_tcscat(tmp, nameext);
		if (_tcscmp(tmp, option) != 0)
			return 0;
	}
	else
	{
		if (_tcscmp(option, name) != 0)
			return 0;
	}
	/* I guess octal isn't popular enough to worry about here...  */
	if (value[0] == '0' && _totupper(value[1]) == 'X')
		value += 2, base = 16;
	*location = _tcstol(value, &endptr, base) * scale;

	if (*endptr != '\0' || *value == '\0')
	{
		if (_tcsicmp(value, _T("false")) == 0 || _tcsicmp(value, _T("no")) == 0)
		{
			*location = 0;
			return 1;
		}
		if (_tcsicmp(value, _T("true")) == 0 || _tcsicmp(value, _T("yes")) == 0)
		{
			*location = 1;
			return 1;
		}
		cfgfile_warning(_T("Option '%s' requires a numeric argument but got '%s'\n"), nameext ? tmp : option, value);
		return -1;
	}
	return 1;
}

static int cfgfile_intval(const TCHAR* option, const TCHAR* value, const TCHAR* name, unsigned int* location, int scale)
{
	return cfgfile_intval(option, value, name, nullptr, location, scale);
}

int cfgfile_intval(const TCHAR* option, const TCHAR* value, const TCHAR* name, int* location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval(option, value, name, nullptr, &v, scale);
	if (!r)
		return 0;
	*location = (int)v;
	return r;
}

static int cfgfile_intval(const TCHAR* option, const TCHAR* value, const TCHAR* name, const TCHAR* nameext,
                          int* location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval(option, value, name, nameext, &v, scale);
	if (!r)
		return 0;
	*location = (int)v;
	return r;
}

static int cfgfile_strval(const TCHAR* option, const TCHAR* value, const TCHAR* name, const TCHAR* nameext,
                          int* location, const TCHAR* table[], int more)
{
	int val;
	TCHAR tmp[MAX_DPATH];
	if (name == nullptr)
		return 0;
	if (nameext)
	{
		_tcscpy(tmp, name);
		_tcscat(tmp, nameext);
		if (_tcscmp(tmp, option) != 0)
			return 0;
	}
	else
	{
		if (_tcscmp(option, name) != 0)
			return 0;
	}
	val = match_string(table, value);
	if (val == -1)
	{
		if (more)
			return 0;
		if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")))
		{
			val = 1;
		}
		else if (!_tcsicmp(value, _T("no")) || !_tcsicmp(value, _T("false")))
		{
			val = 0;
		}
		else
		{
			cfgfile_warning(_T("Unknown value ('%s') for option '%s'.\n"), value, nameext ? tmp : option);
			return -1;
		}
	}
	*location = val;
	return 1;
}

int cfgfile_strval(const TCHAR* option, const TCHAR* value, const TCHAR* name, int* location, const TCHAR* table[],
                   int more)
{
	return cfgfile_strval(option, value, name, nullptr, location, table, more);
}

static int cfgfile_strboolval(const TCHAR* option, const TCHAR* value, const TCHAR* name, bool* location,
                              const TCHAR* table[], int more)
{
	int locationint;
	if (!cfgfile_strval(option, value, name, &locationint, table, more))
		return 0;
	*location = locationint != 0;
	return 1;
}

int cfgfile_string(const TCHAR* option, const TCHAR* value, const TCHAR* name, TCHAR* location, int maxsz)
{
	if (_tcscmp(option, name) != 0)
		return 0;
	_tcsncpy(location, value, maxsz - 1);
	location[maxsz - 1] = '\0';
	return 1;
}

static int cfgfile_string(const TCHAR* option, const TCHAR* value, const TCHAR* name, const TCHAR* nameext,
                          TCHAR* location, int maxsz)
{
	if (nameext)
	{
		TCHAR tmp[MAX_DPATH];
		_tcscpy(tmp, name);
		_tcscat(tmp, nameext);
		if (_tcscmp(tmp, option) != 0)
			return 0;
	}
	else
	{
		if (_tcscmp(option, name) != 0)
			return 0;
	}
	_tcsncpy(location, value, maxsz - 1);
	location[maxsz - 1] = '\0';
	return 1;
}

static int cfgfile_path(const TCHAR* option, const TCHAR* value, const TCHAR* name, TCHAR* location, int maxsz)
{
	if (!cfgfile_string(option, value, name, location, maxsz))
		return 0;
	TCHAR* s = target_expand_environment(location, nullptr, 0);
	_tcsncpy(location, s, maxsz - 1);
	location[maxsz - 1] = 0;
	xfree (s);
	return 1;
}

static int cfgfile_rom(const TCHAR* option, const TCHAR* value, const TCHAR* name, TCHAR* location, int maxsz)
{
	TCHAR id[MAX_DPATH];
	if (!cfgfile_string(option, value, name, id, sizeof id / sizeof (TCHAR)))
		return 0;
	TCHAR* p = _tcschr(id, ',');
	if (p)
	{
		TCHAR *endptr, tmp;
		*p = 0;
		tmp = id[4];
		id[4] = 0;
		uae_u32 crc32 = _tcstol(id, &endptr, 16) << 16;
		id[4] = tmp;
		crc32 |= _tcstol(id + 4, &endptr, 16);
		struct romdata* rd = getromdatabycrc(crc32, true);
		if (rd)
		{
			struct romdata* rd2 = getromdatabyid(rd->id);
			if (rd->group == 0 && rd2 == rd)
			{
				if (zfile_exists(location))
					return 1;
			}
			if (rd->group && rd2)
				rd = rd2;
			struct romlist* rl = getromlistbyromdata(rd);
			if (rl)
			{
				write_log (_T("%s: %s -> %s\n"), name, location, rl->path);
				_tcsncpy(location, rl->path, maxsz);
			}
		}
	}
	return 1;
}

static int getintval(TCHAR** p, int* result, int delim)
{
	TCHAR* value = *p;
	int base = 10;
	TCHAR* endptr;
	TCHAR* p2 = _tcschr(*p, delim);

	if (p2 == nullptr)
		return 0;

	*p2++ = '\0';

	if (value[0] == '0' && _totupper(value[1]) == 'X')
		value += 2, base = 16;
	*result = _tcstol(value, &endptr, base);
	*p = p2;

	if (*endptr != '\0' || *value == '\0')
		return 0;

	return 1;
}

static int getintval2(TCHAR** p, int* result, int delim, bool last)
{
	TCHAR* value = *p;
	int base = 10;
	TCHAR* endptr;
	TCHAR* p2;

	p2 = _tcschr(*p, delim);
	if (p2 == nullptr)
	{
		if (last)
		{
			if (delim != '.')
				p2 = _tcschr(*p, ',');
			if (p2 == nullptr)
			{
				p2 = *p;
				while (*p2)
					p2++;
				if (p2 == *p)
					return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	if (!_istdigit(**p) && **p != '-' && **p != '+')
		return 0;

	if (*p2 != 0)
		*p2++ = '\0';

	if (value[0] == '0' && _totupper(value[1]) == 'X')
		value += 2, base = 16;
	*result = _tcstol(value, &endptr, base);
	*p = p2;

	if (*endptr != '\0' || *value == '\0')
	{
		*p = nullptr;
		return 0;
	}

	return 1;
}

static int cfgfile_option_select(TCHAR* s, const TCHAR* option, const TCHAR* select)
{
	TCHAR buf[MAX_DPATH];
	if (!s)
		return -1;
	_tcscpy(buf, s);
	_tcscat(buf, _T(","));
	TCHAR* p = buf;
	for (;;)
	{
		TCHAR* tmpp = _tcschr(p, ',');
		if (tmpp == nullptr)
			return -1;
		*tmpp++ = 0;
		TCHAR* tmpp2 = _tcschr(p, '=');
		if (!tmpp2)
			return -1;
		*tmpp2++ = 0;
		if (!_tcsicmp(p, option))
		{
			int idx = 0;
			while (select[0])
			{
				if (!_tcsicmp(select, tmpp2))
					return idx;
				idx++;
				select += _tcslen(select) + 1;
			}
		}
		p = tmpp;
	}
}

static int cfgfile_option_bool(TCHAR* s, const TCHAR* option)
{
	TCHAR buf[MAX_DPATH];
	if (!s)
		return -1;
	_tcscpy(buf, s);
	_tcscat(buf, _T(","));
	TCHAR* p = buf;
	for (;;)
	{
		TCHAR* tmpp = _tcschr(p, ',');
		if (tmpp == nullptr)
			return -1;
		*tmpp++ = 0;
		TCHAR* tmpp2 = _tcschr(p, '=');
		if (tmpp2)
			*tmpp2++ = 0;
		if (!_tcsicmp(p, option))
		{
			if (!tmpp2)
				return 0;
			TCHAR* tmpp3 = _tcschr(tmpp2, ',');
			if (tmpp3)
				*tmpp3 = 0;
			if (tmpp2 && !_tcsicmp(tmpp2, _T("true")))
				return 1;
			if (tmpp2 && !_tcsicmp(tmpp2, _T("false")))
				return 0;
			return 1;
		}
		p = tmpp;
	}
}

static void set_chipset_mask(struct uae_prefs* p, int val)
{
	p->chipset_mask = (val == 0
		                   ? 0
		                   : val == 1
		                   ? CSMASK_ECS_AGNUS
		                   : val == 2
		                   ? CSMASK_ECS_DENISE
		                   : val == 3
		                   ? CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS
		                   : CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS);
}

static int cfgfile_parse_host(struct uae_prefs* p, TCHAR* option, TCHAR* value)
{
	int i;
	bool vb;
	TCHAR* section = nullptr;
	TCHAR* tmpp;
	TCHAR tmpbuf[CONFIG_BLEN];
	const TCHAR* tmp1;
	TCHAR* tmp2;

	if (_tcsncmp(option, _T("input."), 6) == 0)
	{
		read_inputdevice_config(p, option, value);
		return 1;
	}

	// custom options LOADING
	for (int i = 0; i < 4; ++i) // Loop 1 ... all 4 joyports
	{
		struct joypad_map_layout tempcustom = {};

		for (int m = 0; m < 2; ++m) // Loop 2 ... none/hotkey function keys
		{
			if (m == 0)
			{
				tmp1 = "none";
				tempcustom = p->jports[i].amiberry_custom_none;
			}
			else if (m == 1)
			{
				tmp1 = "hotkey";
				tempcustom = p->jports[i].amiberry_custom_hotkey;
			}

			for (int n = 0; n < 14; ++n) // Loop 3 ... all 14 buttons
			{
				_stprintf(tmpbuf, "joyport%d_amiberry_custom_%s_%s", i, tmp1, button_remap_name[n]);

				// this is where we need to check if we have this particular option!!
				if (!_tcsncmp(option, _T(tmpbuf), sizeof(tmpbuf) / sizeof(TCHAR)))
				{
					int b = 0;
					if (find_inputevent(value) > -1) { b = RemapEventList[find_inputevent(value)]; }
					//else {b=0;}

					if (n == 0) { tempcustom.dpad_up_action = b; }
					if (n == 1) { tempcustom.dpad_down_action = b; }
					if (n == 2) { tempcustom.dpad_left_action = b; }
					if (n == 3) { tempcustom.dpad_right_action = b; }
					if (n == 4) { tempcustom.select_action = b; }
					if (n == 5) { tempcustom.left_shoulder_action = b; }
					if (n == 6) { tempcustom.lstick_select_action = b; }
					if (n == 7) { tempcustom.north_action = b; }
					if (n == 8) { tempcustom.south_action = b; }
					if (n == 9) { tempcustom.east_action = b; }
					if (n == 10) { tempcustom.west_action = b; }
					if (n == 11) { tempcustom.start_action = b; }
					if (n == 12) { tempcustom.right_shoulder_action = b; }
					if (n == 13) { tempcustom.rstick_select_action = b; }

					if (m == 0)
					{
						p->jports[i].amiberry_custom_none = tempcustom;
					}

					else if (m == 1)
					{
						p->jports[i].amiberry_custom_hotkey = tempcustom;
					}
					return 1;
				} // close the IF check
			} // close loop 3 
		} // close loop 2
	} // close loop 1

	for (tmpp = option; *tmpp != '\0'; tmpp++)
		if (_istupper(*tmpp))
			*tmpp = _totlower(*tmpp);
	tmpp = _tcschr(option, '.');
	if (tmpp)
	{
		section = option;
		option = tmpp + 1;
		*tmpp = '\0';
		if (_tcscmp(section, TARGET_NAME) == 0)
		{
			/* We special case the various path options here.  */
			if (cfgfile_path(option, value, _T("rom_path"), p->path_rom, sizeof p->path_rom / sizeof(TCHAR))
				|| cfgfile_path(option, value, _T("floppy_path"), p->path_floppy, sizeof p->path_floppy / sizeof(TCHAR))
				|| cfgfile_path(option, value, _T("cd_path"), p->path_cd, sizeof p->path_cd / sizeof(TCHAR))
				|| cfgfile_path(option, value, _T("hardfile_path"), p->path_hardfile, sizeof p->path_hardfile / sizeof(TCHAR)))
				return 1;
			return target_parse_option(p, option, value);
		}
		return 0;
	}

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++)
	{
		TCHAR tmp[20];
		_stprintf(tmp, _T("cdimage%d"), i);
		if (!_tcsicmp(option, tmp))
		{
			if (!_tcsicmp(value, _T("autodetect")))
			{
				p->cdslots[i].type = SCSI_UNIT_DEFAULT;
				p->cdslots[i].inuse = true;
				p->cdslots[i].name[0] = 0;
			}
			else
			{
				p->cdslots[i].delayed = false;
				TCHAR* next = _tcsrchr(value, ',');
				int type = SCSI_UNIT_DEFAULT;
				int mode = 0;
				int unitnum = 0;
				for (;;)
				{
					if (!next)
						break;
					*next++ = 0;
					TCHAR* next2 = _tcschr(next, ':');
					if (next2)
						*next2++ = 0;
					if (!_tcsicmp(next, _T("delay")))
					{
						p->cdslots[i].delayed = true;
						next = next2;
						if (!next)
							break;
						next2 = _tcschr(next, ':');
						if (next2)
							*next2++ = 0;
					}
					type = match_string(cdmodes, next);
					if (type < 0)
						type = SCSI_UNIT_DEFAULT;
					else
						type--;
					next = next2;
					if (!next)
						break;
					next2 = _tcschr(next, ':');
					if (next2)
						*next2++ = 0;
					mode = match_string(cdconmodes, next);
					if (mode < 0)
						mode = 0;
					next = next2;
					if (!next)
						break;
					next2 = _tcschr(next, ':');
					if (next2)
						*next2++ = 0;
					cfgfile_intval(option, next, tmp, &unitnum, 1);
				}
				if (_tcslen(value) > 0)
				{
					TCHAR* s = cfgfile_subst_path(UNEXPANDED, p->path_cd, value);
					_tcsncpy(p->cdslots[i].name, s, sizeof p->cdslots[i].name / sizeof(TCHAR));
					xfree(s);
				}
				p->cdslots[i].name[sizeof p->cdslots[i].name - 1] = 0;
				p->cdslots[i].inuse = true;
				p->cdslots[i].type = type;
			}
			// disable all following units
			i++;
			while (i < MAX_TOTAL_SCSI_DEVICES)
			{
				p->cdslots[i].type = SCSI_UNIT_DISABLED;
				i++;
			}
			return 1;
		}
	}

	if (cfgfile_intval(option, value, _T("sound_frequency"), &p->sound_freq, 1)
		|| cfgfile_intval(option, value, _T("sound_volume_paula"), &p->sound_volume_paula, 1)
		|| cfgfile_intval(option, value, _T("sound_volume_cd"), &p->sound_volume_cd, 1)
		|| cfgfile_intval(option, value, _T("sound_stereo_separation"), &p->sound_stereo_separation, 1)
		|| cfgfile_intval(option, value, _T("sound_stereo_mixing_delay"), &p->sound_mixed_stereo_delay, 1)

		|| cfgfile_intval(option, value, _T("gfx_framerate"), &p->gfx_framerate, 1)
		|| cfgfile_intval(option, value, _T("gfx_refreshrate"), &p->gfx_apmode[APMODE_NATIVE].gfx_refreshrate, 1)
		|| cfgfile_intval(option, value, _T("gfx_refreshrate_rtg"), &p->gfx_apmode[APMODE_RTG].gfx_refreshrate, 1)

		|| cfgfile_intval(option, value, _T("filesys_max_size"), &p->filesys_limit, 1)
		|| cfgfile_intval(option, value, _T("filesys_max_name_length"), &p->filesys_max_name, 1)
		|| cfgfile_yesno(option, value, _T("filesys_inject_icons"), &p->filesys_inject_icons)
		|| cfgfile_string(option, value, _T("filesys_inject_icons_drawer"), p->filesys_inject_icons_drawer,
		                  sizeof p->filesys_inject_icons_drawer / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("filesys_inject_icons_project"), p->filesys_inject_icons_project,
		                  sizeof p->filesys_inject_icons_project / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("filesys_inject_icons_tool"), p->filesys_inject_icons_tool,
		                  sizeof p->filesys_inject_icons_tool / sizeof(TCHAR)))
		return 1;

	if (cfgfile_string(option, value, _T("config_info"), p->info, sizeof p->info / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("config_description"), p->description, sizeof p->description / sizeof(TCHAR)))
		return 1;

	if (cfgfile_yesno(option, value, _T("floppy0wp"), &p->floppyslots[0].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("floppy1wp"), &p->floppyslots[1].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("floppy2wp"), &p->floppyslots[2].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("floppy3wp"), &p->floppyslots[3].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("bsdsocket_emu"), &p->socket_emu))
		return 1;

	if (cfgfile_strval(option, value, _T("sound_output"), &p->produce_sound, soundmode1, 1)
		|| cfgfile_strval(option, value, _T("sound_output"), &p->produce_sound, soundmode2, 0)
		|| cfgfile_strval(option, value, _T("sound_interpol"), &p->sound_interpol, interpolmode, 0)
		|| cfgfile_strval(option, value, _T("sound_filter"), &p->sound_filter, soundfiltermode1, 0)
		|| cfgfile_strval(option, value, _T("sound_filter_type"), &p->sound_filter_type, soundfiltermode2, 0)
		|| cfgfile_strboolval(option, value, _T("use_gui"), &p->start_gui, guimode1, 1)
		|| cfgfile_strboolval(option, value, _T("use_gui"), &p->start_gui, guimode2, 1)
		|| cfgfile_strboolval(option, value, _T("use_gui"), &p->start_gui, guimode3, 0)
		|| cfgfile_strval(option, value, _T("gfx_resolution"), &p->gfx_resolution, lorestype1, 0)
		|| cfgfile_strval(option, value, _T("gfx_lores"), &p->gfx_resolution, lorestype2, 0)
		|| cfgfile_strval(option, value, _T("gfx_lores_mode"), &p->gfx_lores_mode, loresmode, 0)
		|| cfgfile_strval(option, value, _T("gfx_fullscreen_amiga"), &p->gfx_apmode[APMODE_NATIVE].gfx_fullscreen, fullmodes, 0)
		|| cfgfile_strval(option, value, _T("gfx_fullscreen_picasso"), &p->gfx_apmode[APMODE_RTG].gfx_fullscreen, fullmodes, 0)
		|| cfgfile_strval(option, value, _T("absolute_mouse"), &p->input_tablet, abspointers, 0))
		return 1;

	if (_tcscmp(option, _T("gfx_width_windowed")) == 0)
	{
		return 1;
	}
	if (_tcscmp(option, _T("gfx_height_windowed")) == 0)
	{
		return 1;
	}

	if (_tcscmp(option, _T("gfx_width_fullscreen")) == 0)
	{
		return 1;
	}
	if (_tcscmp(option, _T("gfx_height_fullscreen")) == 0)
	{
		return 1;
	}

	if (_tcscmp(option, _T("gfx_linemode")) == 0)
	{
		int v;
		p->gfx_vresolution = VRES_DOUBLE;
		p->gfx_pscanlines = 0;
		p->gfx_iscanlines = 0;
		if (cfgfile_strval(option, value, _T("gfx_linemode"), &v, linemode, 0))
		{
			p->gfx_vresolution = VRES_NONDOUBLE;
			if (v > 0)
			{
				p->gfx_iscanlines = (v - 1) / 4;
				p->gfx_pscanlines = (v - 1) % 4;
				p->gfx_vresolution = VRES_DOUBLE;
			}
		}
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("show_leds"), &vb))
	{
		p->leds_on_screen = vb;
		return 1;
	}

	if (_tcscmp(option, _T("gfx_width")) == 0 || _tcscmp(option, _T("gfx_height")) == 0)
	{
		cfgfile_intval(option, value, _T("gfx_width"), &p->gfx_size.width, 1);
		cfgfile_intval(option, value, _T("gfx_height"), &p->gfx_size.height, 1);
		return 1;
	}

	// olde versions (compatibility)

	if (_tcscmp(option, _T("joyportfriendlyname0")) == 0 || _tcscmp(option, _T("joyportfriendlyname1")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyportfriendlyname0")) == 0 ? 0 : 1, -1, 2);
		return 1;
	}
	if (_tcscmp(option, _T("joyportfriendlyname2")) == 0 || _tcscmp(option, _T("joyportfriendlyname3")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyportfriendlyname2")) == 0 ? 2 : 3, -1, 2);
		return 1;
	}
	if (_tcscmp(option, _T("joyportname0")) == 0 || _tcscmp(option, _T("joyportname1")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyportname0")) == 0 ? 0 : 1, -1, 1);
		return 1;
	}
	if (_tcscmp(option, _T("joyportname2")) == 0 || _tcscmp(option, _T("joyportname3")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyportname2")) == 0 ? 2 : 3, -1, 1);
		return 1;
	}


	// new versions  
	if (_tcscmp(option, _T("joyport0_friendlyname")) == 0 || _tcscmp(option, _T("joyport1_friendlyname")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyport0_friendlyname")) == 0 ? 0 : 1, -1, 2);
		return 1;
	}
	if (_tcscmp(option, _T("joyport2_friendlyname")) == 0 || _tcscmp(option, _T("joyport3_friendlyname")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyport2_friendlyname")) == 0 ? 2 : 3, -1, 2);
		return 1;
	}
	if (_tcscmp(option, _T("joyport0_name")) == 0 || _tcscmp(option, _T("joyport1_name")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyport0_name")) == 0 ? 0 : 1, -1, 1);
		return 1;
	}
	if (_tcscmp(option, _T("joyport2_name")) == 0 || _tcscmp(option, _T("joyport3_name")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyport2_name")) == 0 ? 2 : 3, -1, 1);
		return 1;
	}

	// old version only 
	if (_tcscmp(option, _T("joyport0")) == 0 || _tcscmp(option, _T("joyport1")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyport0")) == 0 ? 0 : 1, -1, 0);
		return 1;
	}
	if (_tcscmp(option, _T("joyport2")) == 0 || _tcscmp(option, _T("joyport3")) == 0)
	{
		inputdevice_joyport_config_store(p, value, _tcscmp(option, _T("joyport2")) == 0 ? 2 : 3, -1, 0);
		return 1;
	}

	// shared version
	if (cfgfile_strval(option, value, _T("joyport0_mode"), &p->jports[0].mode, joyportmodes, 0) ||
		cfgfile_strval(option, value, _T("joyport0mode"), &p->jports[0].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport1_mode"), &p->jports[1].mode, joyportmodes, 0) ||
		cfgfile_strval(option, value, _T("joyport1mode"), &p->jports[1].mode, joyportmodes, 0))
		return 1;

	if (cfgfile_strval(option, value, _T("joyport0autofire"), &p->jports[0].autofire, joyaf, 0) ||
		cfgfile_strval(option, value, _T("joyport0_autofire"), &p->jports[0].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport1autofire"), &p->jports[1].autofire, joyaf, 0) ||
		cfgfile_strval(option, value, _T("joyport1_autofire"), &p->jports[1].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport2autofire"), &p->jports[2].autofire, joyaf, 0) ||
		cfgfile_strval(option, value, _T("joyport2_autofire"), &p->jports[2].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport3autofire"), &p->jports[3].autofire, joyaf, 0) ||
		cfgfile_strval(option, value, _T("joyport3_autofire"), &p->jports[3].autofire, joyaf, 0))
		return 1;

	if (cfgfile_strval(option, value, _T("joyport0_mousemap"), &p->jports[0].mousemap, mousemaps, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport1_mousemap"), &p->jports[1].mousemap, mousemaps, 0))
		return 1;

	// shared version
	if (cfgfile_yesno(option, value, _T("joyport0keyboardoverride"), &vb) ||
		cfgfile_yesno(option, value, _T("joyport0_keyboardoverride"), &vb))
	{
		p->jports[0].nokeyboardoverride = !vb;
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("joyport1keyboardoverride"), &vb) ||
		cfgfile_yesno(option, value, _T("joyport1_keyboardoverride"), &vb))
	{
		p->jports[1].nokeyboardoverride = !vb;
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("joyport2keyboardoverride"), &vb) ||
		cfgfile_yesno(option, value, _T("joyport2_keyboardoverride"), &vb))
	{
		p->jports[2].nokeyboardoverride = !vb;
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("joyport3keyboardoverride"), &vb) ||
		cfgfile_yesno(option, value, _T("joyport3_keyboardoverride"), &vb))
	{
		p->jports[3].nokeyboardoverride = !vb;
		return 1;
	}

	if (cfgfile_string(option, value, _T("statefile"), tmpbuf, sizeof(tmpbuf) / sizeof(TCHAR)))
	{
		_tcscpy(savestate_fname, tmpbuf);
		if (zfile_exists(savestate_fname))
		{
			savestate_state = STATE_DORESTORE;
		}
		else
		{
			int ok = 0;
			if (savestate_fname[0])
			{
				for (;;)
				{
					TCHAR* p;
					if (my_existsdir(savestate_fname))
					{
						ok = 1;
						break;
					}
					p = _tcsrchr(savestate_fname, '\\');
					if (!p)
						p = _tcsrchr(savestate_fname, '/');
					if (!p)
						break;
					*p = 0;
				}
			}
			if (!ok)
			{
				savestate_fname[0] = 0;
			}
		}
		return 1;
	}

	if (cfgfile_strval(option, value, _T("sound_channels"), &p->sound_stereo, stereomode, 1))
	{
		if (p->sound_stereo == SND_NONE)
		{
			/* "mixed stereo" compatibility hack */
			p->sound_stereo = SND_STEREO;
			p->sound_mixed_stereo_delay = 5;
			p->sound_stereo_separation = 7;
		}
		return 1;
	}

	if (_tcscmp(option, _T("kbd_lang")) == 0)
	{
		KbdLang l;
		if ((l = KBD_LANG_DE, _tcsicmp(value, _T("de")) == 0)
			|| (l = KBD_LANG_DK, _tcsicmp(value, _T("dk")) == 0)
			|| (l = KBD_LANG_SE, _tcsicmp(value, _T("se")) == 0)
			|| (l = KBD_LANG_US, _tcsicmp(value, _T("us")) == 0)
			|| (l = KBD_LANG_FR, _tcsicmp(value, _T("fr")) == 0)
			|| (l = KBD_LANG_IT, _tcsicmp(value, _T("it")) == 0)
			|| (l = KBD_LANG_ES, _tcsicmp(value, _T("es")) == 0))
			p->keyboard_lang = l;
		else
		cfgfile_warning(_T("Unknown keyboard language\n"));
		return 1;
	}

	if (cfgfile_string(option, value, _T("config_version"), tmpbuf, sizeof(tmpbuf) / sizeof(TCHAR)))
	{
		TCHAR* tmpp2;
		tmpp = _tcschr(value, '.');
		if (tmpp)
		{
			*tmpp++ = 0;
			tmpp2 = tmpp;
			p->config_version = _tstol(tmpbuf) << 16;
			tmpp = _tcschr(tmpp, '.');
			if (tmpp)
			{
				*tmpp++ = 0;
				p->config_version |= _tstol(tmpp2) << 8;
				p->config_version |= _tstol(tmpp);
			}
		}
		return 1;
	}

	if (_tcscmp(option, _T("displaydata")) == 0 || _tcscmp(option, _T("displaydata_pal")) == 0 || _tcscmp(
		option, _T("displaydata_ntsc")) == 0)
	{
		_tcsncpy(tmpbuf, value, sizeof tmpbuf / sizeof(TCHAR) - 1);
		tmpbuf[sizeof tmpbuf / sizeof(TCHAR) - 1] = '\0';

		int vert = -1, horiz = -1, lace = -1, ntsc = -1, vsync = -1, hres = 0;
		bool locked = false, rtg = false;
		bool defaultdata = false;
		float rate = -1;
		TCHAR label[16] = {0};
		TCHAR* tmpp = tmpbuf;
		TCHAR* end = tmpbuf + _tcslen(tmpbuf);
		for (;;)
		{
			TCHAR* next = _tcschr(tmpp, ',');
			TCHAR* equals = _tcschr(tmpp, '=');

			if (!next)
				next = end;
			if (equals == nullptr || equals > next)
				equals = nullptr;
			else
				equals++;
			*next = 0;

			if (rate < 0)
				rate = _tstof(tmpp);
			else if (!_tcsnicmp(tmpp, _T("v="), 2))
				vert = _tstol(equals);
			else if (!_tcsnicmp(tmpp, _T("h="), 2))
				horiz = _tstol(equals);
			else if (!_tcsnicmp(tmpp, _T("t="), 2))
				_tcsncpy(label, equals, sizeof label / sizeof(TCHAR) - 1);
			if (!_tcsnicmp(tmpp, _T("locked"), 4))
				locked = true;
			if (!_tcsnicmp(tmpp, _T("nlace"), 5))
				lace = 0;
			if (!_tcsnicmp(tmpp, _T("lace"), 4))
				lace = 1;
			if (!_tcsnicmp(tmpp, _T("lores"), 5))
				hres |= 1 << RES_LORES;
			if (!_tcsnicmp(tmpp, _T("hires"), 5))
				hres |= 1 << RES_HIRES;
			if (!_tcsnicmp(tmpp, _T("shres"), 5))
				hres |= 1 << RES_SUPERHIRES;
			if (!_tcsnicmp(tmpp, _T("nvsync"), 5))
				vsync = 0;
			if (!_tcsnicmp(tmpp, _T("vsync"), 4))
				vsync = 1;
			if (!_tcsnicmp(tmpp, _T("ntsc"), 4))
				ntsc = 1;
			if (!_tcsnicmp(tmpp, _T("pal"), 3))
				ntsc = 0;
			if (!_tcsnicmp(tmpp, _T("rtg"), 3))
				rtg = true;
			if (!_tcsnicmp(tmpp, _T("default"), 7))
				defaultdata = true;

			tmpp = next;
			if (tmpp >= end)
				break;
			tmpp++;
		}
		for (int i = 0; i < MAX_CHIPSET_REFRESH; i++)
		{
			struct chipset_refresh* cr = &p->cr[i];
			if (_tcscmp(option, _T("displaydata_pal")) == 0)
			{
				i = CHIPSET_REFRESH_PAL;
				cr = &p->cr[i];
				cr->inuse = false;
				_tcscpy(label, _T("PAL"));
			}
			else if (_tcscmp(option, _T("displaydata_ntsc")) == 0)
			{
				i = CHIPSET_REFRESH_NTSC;
				cr = &p->cr[i];
				cr->inuse = false;
				_tcscpy(label, _T("NTSC"));
			}
			if (!cr->inuse)
			{
				cr->inuse = true;
				cr->horiz = horiz;
				cr->vert = vert;
				cr->lace = lace;
				cr->resolution = hres ? hres : 1 + 2 + 4;
				cr->ntsc = ntsc;
				cr->vsync = vsync;
				cr->locked = locked;
				cr->rtg = rtg;
				cr->rate = rate;
				cr->defaultdata = defaultdata;
				_tcscpy(cr->label, label);
				break;
			}
		}
		return 1;
	}

	return 0;
}

static void decode_rom_ident(TCHAR* romfile, int maxlen, const TCHAR* ident, int romflags)
{
	const TCHAR* p;
	int ver, rev, subver, subrev, round, i;
	TCHAR model[64], *modelp;
	struct romlist** rl;
	TCHAR* romtxt;

	if (!ident[0])
		return;
	romtxt = xmalloc (TCHAR, 10000);
	romtxt[0] = 0;
	for (round = 0; round < 2; round++)
	{
		ver = rev = subver = subrev = -1;
		modelp = nullptr;
		memset(model, 0, sizeof model);
		p = ident;
		while (*p)
		{
			TCHAR c = *p++;
			int *pp1 = nullptr, *pp2 = nullptr;
			if (_totupper(c) == 'V' && _istdigit(*p))
			{
				pp1 = &ver;
				pp2 = &rev;
			}
			else if (_totupper(c) == 'R' && _istdigit(*p))
			{
				pp1 = &subver;
				pp2 = &subrev;
			}
			else if (!_istdigit(c) && c != ' ')
			{
				_tcsncpy(model, p - 1, (sizeof model) / sizeof (TCHAR) - 1);
				p += _tcslen(model);
				modelp = model;
			}
			if (pp1)
			{
				*pp1 = _tstol(p);
				while (*p != 0 && *p != '.' && *p != ' ')
					p++;
				if (*p == '.')
				{
					p++;
					if (pp2)
						*pp2 = _tstol(p);
				}
			}
			if (*p == 0 || *p == ';')
			{
				rl = getromlistbyident(ver, rev, subver, subrev, modelp, romflags, round > 0);
				if (rl)
				{
					for (i = 0; rl[i]; i++)
					{
						if (rl[i]->path && !_tcscmp(rl[i]->path, romfile))
						{
							xfree(rl);
							round = 0;
							goto end;
						}
					}
					if (!rl[i])
					{
						for (i = 0; rl[i]; i++)
						{
							if (round)
							{
								TCHAR romname[MAX_DPATH];
								getromname(rl[i]->rd, romname);
								_tcscat(romtxt, romname);
								_tcscat(romtxt, _T("\n"));
							}
							else
							{
								_tcsncpy(romfile, rl[i]->path, maxlen);
								goto end;
							}
						}
					}
					xfree (rl);
				}
			}
		}
	}
end:
	if (round && romtxt[0])
	{
		notify_user_parms(NUMSG_ROMNEED, romtxt, romtxt);
	}
	xfree (romtxt);
}

static struct uaedev_config_data* getuci(struct uae_prefs* p)
{
	if (p->mountitems < MOUNT_CONFIG_SIZE)
		return &p->mountconfig[p->mountitems++];
	return nullptr;
}

struct uaedev_config_data* add_filesys_config(struct uae_prefs* p, int index, struct uaedev_config_info* ci)
{
	struct uaedev_config_data* uci;
	int i;

	if (index < 0 && (ci->type == UAEDEV_DIR || ci->type == UAEDEV_HDF) && ci->devname && _tcslen(ci->devname) > 0)
	{
		for (i = 0; i < p->mountitems; i++)
		{
			if (p->mountconfig[i].ci.devname && !_tcscmp(p->mountconfig[i].ci.devname, ci->devname))
				return nullptr;
		}
	}

	if (index < 0)
	{
		if (ci->controller_type != HD_CONTROLLER_TYPE_UAE)
		{
			int ctrl = ci->controller_type;
			int ctrlunit = ci->controller_type_unit;
			int cunit = ci->controller_unit;
			for (;;)
			{
				for (i = 0; i < p->mountitems; i++)
				{
					if (p->mountconfig[i].ci.controller_type == ctrl && p->mountconfig[i].ci.controller_type_unit == ctrlunit && p->
						mountconfig[i].ci.controller_unit == cunit)
					{
						cunit++;
						if (ctrl >= HD_CONTROLLER_TYPE_IDE_FIRST && ctrl <= HD_CONTROLLER_TYPE_IDE_LAST && cunit == 4)
							return nullptr;
						if (ctrl >= HD_CONTROLLER_TYPE_SCSI_FIRST && ctrl <= HD_CONTROLLER_TYPE_SCSI_LAST && cunit >= 7)
							return nullptr;
					}
				}
				if (i == p->mountitems)
				{
					ci->controller_unit = cunit;
					break;
				}
			}
		}
		else if (ci->controller_type == HD_CONTROLLER_TYPE_UAE)
		{
			int ctrl = ci->controller_type;
			int ctrlunit = ci->controller_type_unit;
			int cunit = ci->controller_unit;
			for (;;)
			{
				for (i = 0; i < p->mountitems; i++)
				{
					if (p->mountconfig[i].ci.controller_type == ctrl && p->mountconfig[i].ci.controller_type_unit == ctrlunit && p->
						mountconfig[i].ci.controller_unit == cunit)
					{
						cunit++;
						if (cunit >= MAX_FILESYSTEM_UNITS)
							return nullptr;
					}
				}
				if (i == p->mountitems)
				{
					ci->controller_unit = cunit;
					break;
				}
			}
		}
		uci = getuci(p);
		uci->configoffset = -1;
		uci->unitnum = -1;
	}
	else
	{
		uci = &p->mountconfig[index];
	}
	if (!uci)
		return nullptr;

	memcpy(&uci->ci, ci, sizeof (struct uaedev_config_info));
	validatedevicename(uci->ci.devname, nullptr);
	validatevolumename(uci->ci.volname, nullptr);
	if (!uci->ci.devname[0])
	{
		TCHAR base[32];
		TCHAR base2[32];
		int num = 0;
		if (uci->ci.rootdir[0] == 0 && ci->type == UAEDEV_DIR)
			_tcscpy(base, _T("RDH"));
		else
			_tcscpy(base, _T("DH"));
		_tcscpy(base2, base);
		for (i = 0; i < p->mountitems; i++)
		{
			_stprintf(base2, _T("%s%d"), base, num);
			if (!_tcsicmp(base2, p->mountconfig[i].ci.devname))
			{
				num++;
				i = -1;
			}
		}
		_tcscpy(uci->ci.devname, base2);
		validatedevicename(uci->ci.devname, nullptr);
	}
	if (ci->type == UAEDEV_DIR)
	{
		TCHAR* s = filesys_createvolname(uci->ci.volname, uci->ci.rootdir, nullptr, _T("Harddrive"));
		_tcscpy(uci->ci.volname, s);
		xfree (s);
	}
	return uci;
}

static void parse_addmem(struct uae_prefs* p, TCHAR* buf, int num)
{
	int size = 0, addr = 0;

	if (!getintval2(&buf, &addr, ',', false))
		return;
	if (!getintval2(&buf, &size, 0, true))
		return;
	if (addr & 0xffff)
		return;
	if ((size & 0xffff) || (size & 0xffff0000) == 0)
		return;
	p->custom_memory_addrs[num] = addr;
	p->custom_memory_sizes[num] = size;
}

static void get_filesys_controller(const TCHAR* hdc, int* type, int* typenum, int* num)
{
	int hdcv = HD_CONTROLLER_TYPE_UAE;
	int hdunit = 0;
	int idx = 0;
	if (_tcslen(hdc) >= 4 && !_tcsncmp(hdc, _T("ide"), 3))
	{
		hdcv = HD_CONTROLLER_TYPE_IDE_AUTO;
		hdunit = hdc[3] - '0';
		if (hdunit < 0 || hdunit >= 6)
			hdunit = 0;
	}
	else if (_tcslen(hdc) >= 5 && !_tcsncmp(hdc, _T("scsi"), 4))
	{
		hdcv = HD_CONTROLLER_TYPE_SCSI_AUTO;
		hdunit = hdc[4] - '0';
		if (hdunit < 0 || hdunit >= 8 + 2)
			hdunit = 0;
	}
	else if (_tcslen(hdc) >= 6 && !_tcsncmp(hdc, _T("scsram"), 6))
	{
		hdcv = HD_CONTROLLER_TYPE_PCMCIA;
		hdunit = 0;
		idx = 0;
	}
	else if (_tcslen(hdc) >= 5 && !_tcsncmp(hdc, _T("scide"), 5))
	{
		hdcv = HD_CONTROLLER_TYPE_PCMCIA;
		hdunit = 0;
		idx = 1;
	}
	if (hdcv == HD_CONTROLLER_TYPE_UAE)
	{
		hdunit = _tstol(hdc + 3);
		if (hdunit >= MAX_FILESYSTEM_UNITS)
			hdunit = 0;
	}
	else if (hdcv > HD_CONTROLLER_TYPE_UAE)
	{
		TCHAR control[MAX_DPATH];
		bool found = false;
		_tcscpy(control, hdc);
		TCHAR* extend = (TCHAR*)_tcschr(control, ',');
		if (extend)
			extend[0] = 0;
		const TCHAR* ext = _tcsrchr(control, '_');
		if (ext)
		{
			ext++;
			int len = _tcslen(ext);
			if (len > 2 && ext[len - 2] == '-' && ext[len - 1] >= '2' && ext[len - 1] <= '9')
			{
				idx = ext[len - 1] - '1';
				len -= 2;
			}
			for (int i = 0; hdcontrollers[i].label; i++)
			{
				const TCHAR* ext2 = _tcsrchr(hdcontrollers[i].label, '_');
				if (ext2)
				{
					ext2++;
					if (_tcslen(ext2) == len && !_tcsnicmp(ext, ext2, len) && hdc[0] == hdcontrollers[i].label[0])
					{
						if (hdcontrollers[i].romtype)
						{
							for (int j = 0; expansionroms[j].name; j++)
							{
								if ((expansionroms[j].romtype & ROMTYPE_MASK) == hdcontrollers[i].romtype)
								{
									hdcv = hdcv == HD_CONTROLLER_TYPE_IDE_AUTO
										       ? j + HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST
										       : j + HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST;
									break;
								}
							}
						}
						if (hdcv == HD_CONTROLLER_TYPE_IDE_AUTO)
						{
							hdcv = i;
						}
						else if (hdcv == HD_CONTROLLER_TYPE_SCSI_AUTO)
						{
							hdcv = i + HD_CONTROLLER_EXPANSION_MAX;
						}
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				for (int i = 0; expansionroms[i].name; i++)
				{
					const struct expansionromtype* ert = &expansionroms[i];
					if (_tcslen(ert->name) == len && !_tcsnicmp(ext, ert->name, len))
					{
						if (hdcv == HD_CONTROLLER_TYPE_IDE_AUTO)
						{
							hdcv = HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST + i;
						}
						else
						{
							hdcv = HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST + i;
						}
						break;
					}
				}
			}
		}
	}
	if (idx >= MAX_DUPLICATE_EXPANSION_BOARDS)
		idx = MAX_DUPLICATE_EXPANSION_BOARDS - 1;
	if (hdunit < 0)
		hdunit = 0;
	*type = hdcv;
	*typenum = idx;
	*num = hdunit;
}

static int cfgfile_parse_partial_newfilesys(struct uae_prefs* p, int nr, int type, const TCHAR* value, int unit,
                                            bool uaehfentry)
{
	TCHAR* tmpp;
	TCHAR *name = nullptr, *path = nullptr;

	// read only harddrive name
	if (!uaehfentry)
		return 0;
	if (type != 1)
		return 0;
	tmpp = getnextentry(&value, ',');
	if (!tmpp)
		return 0;
	xfree (tmpp);
	tmpp = getnextentry(&value, ':');
	if (!tmpp)
		return 0;
	xfree (tmpp);
	name = getnextentry(&value, ':');
	if (name && _tcslen(name) > 0)
	{
		path = getnextentry(&value, ',');
		if (path && _tcslen(path) > 0)
		{
			for (int i = 0; i < MAX_FILESYSTEM_UNITS; i++)
			{
				struct uaedev_config_info* uci = &p->mountconfig[i].ci;
				if (_tcsicmp(uci->rootdir, name) == 0)
				{
					_tcscat(uci->rootdir, _T(":"));
					_tcscat(uci->rootdir, path);
				}
			}
		}
	}
	xfree (path);
	xfree (name);
	return 1;
}

static int cfgfile_parse_newfilesys(struct uae_prefs* p, int nr, int type, TCHAR* value, int unit, bool uaehfentry)
{
	struct uaedev_config_info uci;
	TCHAR *tmpp = _tcschr(value, ','), *tmpp2;
	TCHAR* str = nullptr;
	TCHAR devname[MAX_DPATH], volname[MAX_DPATH];

	devname[0] = volname[0] = 0;
	uci_set_defaults(&uci, false);

	config_newfilesystem = 1;
	if (tmpp == nullptr)
		goto invalid_fs;

	*tmpp++ = '\0';
	if (_tcsicmp(value, _T("ro")) == 0)
		uci.readonly = true;
	else if (_tcsicmp(value, _T("rw")) == 0)
		uci.readonly = false;
	else
		goto invalid_fs;

	value = tmpp;
	if (type == 0)
	{
		uci.type = UAEDEV_DIR;
		tmpp = _tcschr(value, ':');
		if (tmpp == nullptr)
			goto empty_fs;
		*tmpp++ = 0;
		_tcscpy(devname, value);
		tmpp2 = tmpp;
		tmpp = _tcschr(tmpp, ':');
		if (tmpp == nullptr)
			goto empty_fs;
		*tmpp++ = 0;
		_tcscpy(volname, tmpp2);
		tmpp2 = tmpp;
		// quoted special case
		if (tmpp2[0] == '\"')
		{
			const TCHAR* end;
			TCHAR* n = cfgfile_unescape(tmpp2, &end, 0, false);
			if (!n)
				goto invalid_fs;
			_tcscpy(uci.rootdir, n);
			xfree(n);
			tmpp = (TCHAR*)end;
			*tmpp++ = 0;
		}
		else
		{
			tmpp = _tcschr(tmpp, ',');
			if (tmpp == nullptr)
				goto empty_fs;
			*tmpp++ = 0;
			_tcscpy(uci.rootdir, tmpp2);
		}
		_tcscpy(uci.volname, volname);
		_tcscpy(uci.devname, devname);
		if (! getintval(&tmpp, &uci.bootpri, 0))
			goto empty_fs;
	}
	else if (type == 1 || ((type == 2 || type == 3) && uaehfentry))
	{
		tmpp = _tcschr(value, ':');
		if (tmpp == nullptr)
			goto invalid_fs;
		*tmpp++ = '\0';
		_tcscpy(devname, value);
		tmpp2 = tmpp;
		// quoted special case
		if (tmpp2[0] == '\"')
		{
			const TCHAR* end;
			TCHAR* n = cfgfile_unescape(tmpp2, &end, 0, false);
			if (!n)
				goto invalid_fs;
			_tcscpy(uci.rootdir, n);
			xfree(n);
			tmpp = (TCHAR*)end;
			*tmpp++ = 0;
		}
		else
		{
			tmpp = _tcschr(tmpp, ',');
			if (tmpp == nullptr)
				goto invalid_fs;
			*tmpp++ = 0;
			_tcscpy(uci.rootdir, tmpp2);
		}
		_tcscpy(uci.devname, devname);
		if (! getintval(&tmpp, &uci.sectors, ',')
			|| ! getintval(&tmpp, &uci.surfaces, ',')
			|| ! getintval(&tmpp, &uci.reserved, ',')
			|| ! getintval(&tmpp, &uci.blocksize, ','))
			goto invalid_fs;
		if (getintval2(&tmpp, &uci.bootpri, ',', false))
		{
			tmpp2 = tmpp;
			// quoted special case
			if (tmpp2[0] == '\"')
			{
				const TCHAR* end;
				TCHAR* n = cfgfile_unescape(tmpp2, &end, 0, false);
				if (!n)
					goto invalid_fs;
				_tcscpy(uci.filesys, n);
				xfree(n);
				tmpp = (TCHAR*)end;
				*tmpp++ = 0;
			}
			else
			{
				tmpp = _tcschr(tmpp, ',');
				if (tmpp == nullptr)
					goto empty_fs;
				*tmpp++ = 0;
				_tcscpy(uci.filesys, tmpp2);
			}
			get_filesys_controller(tmpp, &uci.controller_type, &uci.controller_type_unit, &uci.controller_unit);
			tmpp2 = _tcschr(tmpp, ',');
			if (tmpp2)
			{
				tmpp2++;
				if (getintval2(&tmpp2, &uci.highcyl, ',', false))
				{
					getintval(&tmpp2, &uci.pcyls, '/');
					getintval(&tmpp2, &uci.pheads, '/');
					getintval2(&tmpp2, &uci.psecs, '/', true);
					if (uci.pheads && uci.psecs)
					{
						uci.physical_geometry = true;
					}
					else
					{
						uci.pheads = uci.psecs = uci.pcyls = 0;
						uci.physical_geometry = false;
					}
				}
			}
			uci.controller_media_type = 0;
			uci.unit_feature_level = 1;

			if (cfgfile_option_find(tmpp2, _T("CF")))
				uci.controller_media_type = 1;
			else if (cfgfile_option_find(tmpp2, _T("HD")))
				uci.controller_media_type = 0;

			TCHAR* pflags;
			if ((pflags = cfgfile_option_get(tmpp2, _T("flags"))))
			{
				getintval(&pflags, &uci.unit_special_flags, 0);
			}

			if (cfgfile_option_find(tmpp2, _T("lock")))
				uci.lock = true;

			if (cfgfile_option_find(tmpp2, _T("SCSI2")))
				uci.unit_feature_level = HD_LEVEL_SCSI_2;
			else if (cfgfile_option_find(tmpp2, _T("SCSI1")))
				uci.unit_feature_level = HD_LEVEL_SCSI_1;
			else if (cfgfile_option_find(tmpp2, _T("SASIE")))
				uci.unit_feature_level = HD_LEVEL_SASI_ENHANCED;
			else if (cfgfile_option_find(tmpp2, _T("SASI")))
				uci.unit_feature_level = HD_LEVEL_SASI;
			else if (cfgfile_option_find(tmpp2, _T("SASI_CHS")))
				uci.unit_feature_level = HD_LEVEL_SASI_CHS;
			else if (cfgfile_option_find(tmpp2, _T("ATA2+S")))
				uci.unit_feature_level = HD_LEVEL_ATA_2S;
			else if (cfgfile_option_find(tmpp2, _T("ATA2+")))
				uci.unit_feature_level = HD_LEVEL_ATA_2;
			else if (cfgfile_option_find(tmpp2, _T("ATA1")))
				uci.unit_feature_level = HD_LEVEL_ATA_1;
		}
		uci.type = UAEDEV_HDF;
	}
	else
	{
		goto invalid_fs;
	}
empty_fs:
	if (uci.rootdir[0])
	{
		if (_tcslen(uci.rootdir) > 3 && uci.rootdir[0] == 'H' && uci.rootdir[1] == 'D' && uci.rootdir[2] == '_')
		{
			memmove(uci.rootdir, uci.rootdir + 2, (_tcslen(uci.rootdir + 2) + 1) * sizeof (TCHAR));
			uci.rootdir[0] = ':';
		}
		str = cfgfile_subst_path(UNEXPANDED, p->path_hardfile, uci.rootdir);
		_tcscpy(uci.rootdir, str);
	}
#ifdef FILESYS
	add_filesys_config(p, nr, &uci);
#endif
	xfree (str);
	return 1;

invalid_fs:
	cfgfile_warning(_T("Invalid filesystem/hardfile/cd specification.\n"));
	return 1;
}

static int cfgfile_parse_filesys(struct uae_prefs* p, const TCHAR* option, TCHAR* value)
{
	int i;

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++)
	{
		TCHAR tmp[100];
		_stprintf(tmp, _T("uaehf%d"), i);
		if (!_tcscmp(option, tmp))
		{
			for (;;)
			{
				int type = -1;
				int unit = -1;
				TCHAR* tmpp = _tcschr(value, ',');
				if (tmpp == nullptr)
					return 1;
				*tmpp++ = 0;
				if (_tcsicmp(value, _T("hdf")) == 0)
				{
					type = 1;
					cfgfile_parse_partial_newfilesys(p, -1, type, tmpp, unit, true);
					return 1;
				}
				if (_tcsnicmp(value, _T("cd"), 2) == 0 && (value[2] == 0 || value[3] == 0))
				{
					unit = 0;
					if (value[2] > 0)
						unit = value[2] - '0';
					if (unit >= 0 && unit <= MAX_TOTAL_SCSI_DEVICES)
					{
						type = 2;
					}
				}
				else if (_tcsicmp(value, _T("dir")) != 0)
				{
					type = 0;
					return 1; /* ignore for now */
				}
				if (type >= 0)
					cfgfile_parse_newfilesys(p, -1, type, tmpp, unit, true);
				return 1;
			}
			return 1;
		}
		if (!_tcsncmp(option, tmp, _tcslen(tmp)) && option[_tcslen(tmp)] == '_')
		{
			struct uaedev_config_info* uci = &currprefs.mountconfig[i].ci;
			if (uci->devname)
			{
				const TCHAR* s = &option[_tcslen(tmp) + 1];
				if (!_tcscmp(s, _T("bootpri")))
				{
					getintval(&value, &uci->bootpri, 0);
				}
				else if (!_tcscmp(s, _T("read-only")))
				{
					cfgfile_yesno(nullptr, value, nullptr, &uci->readonly);
				}
				else if (!_tcscmp(s, _T("volumename")))
				{
					_tcscpy(uci->volname, value);
				}
				else if (!_tcscmp(s, _T("devicename")))
				{
					_tcscpy(uci->devname, value);
				}
				else if (!_tcscmp(s, _T("root")))
				{
					_tcscpy(uci->rootdir, value);
				}
				else if (!_tcscmp(s, _T("filesys")))
				{
					_tcscpy(uci->filesys, value);
				}
				else if (!_tcscmp(s, _T("controller")))
				{
					get_filesys_controller(value, &uci->controller_type, &uci->controller_type_unit, &uci->controller_unit);
				}
			}
		}
	}

	if (_tcscmp(option, _T("filesystem")) == 0
		|| _tcscmp(option, _T("hardfile")) == 0)
	{
		struct uaedev_config_info uci;
		TCHAR* tmpp = _tcschr(value, ',');
		TCHAR* str;
		bool hdf;

		uci_set_defaults(&uci, false);

		if (config_newfilesystem)
			return 1;

		if (tmpp == nullptr)
			goto invalid_fs;

		*tmpp++ = '\0';
		if (_tcscmp(value, _T("1")) == 0 || _tcsicmp(value, _T("ro")) == 0
			|| _tcsicmp(value, _T("readonly")) == 0
			|| _tcsicmp(value, _T("read-only")) == 0)
			uci.readonly = true;
		else if (_tcscmp(value, _T("0")) == 0 || _tcsicmp(value, _T("rw")) == 0
			|| _tcsicmp(value, _T("readwrite")) == 0
			|| _tcsicmp(value, _T("read-write")) == 0)
			uci.readonly = false;
		else
			goto invalid_fs;

		value = tmpp;
		if (_tcscmp(option, _T("filesystem")) == 0)
		{
			hdf = false;
			tmpp = _tcschr(value, ':');
			if (tmpp == nullptr)
				goto invalid_fs;
			*tmpp++ = '\0';
			_tcscpy(uci.volname, value);
			_tcscpy(uci.rootdir, tmpp);
		}
		else
		{
			hdf = true;
			if (! getintval(&value, &uci.sectors, ',')
				|| ! getintval(&value, &uci.surfaces, ',')
				|| ! getintval(&value, &uci.reserved, ',')
				|| ! getintval(&value, &uci.blocksize, ','))
				goto invalid_fs;
			_tcscpy(uci.rootdir, value);
		}
		str = cfgfile_subst_path(UNEXPANDED, p->path_hardfile, uci.rootdir);
#ifdef FILESYS
		uci.type = hdf ? UAEDEV_HDF : UAEDEV_DIR;
		add_filesys_config(p, -1, &uci);
#endif
		xfree (str);
		return 1;
	invalid_fs:
		cfgfile_warning(_T("Invalid filesystem/hardfile specification.\n"));
		return 1;
	}

	if (_tcscmp(option, _T("filesystem2")) == 0)
		return cfgfile_parse_newfilesys(p, -1, 0, value, -1, false);
	if (_tcscmp(option, _T("hardfile2")) == 0)
		return cfgfile_parse_newfilesys(p, -1, 1, value, -1, false);
	if (_tcscmp(option, _T("filesystem_extra")) == 0)
	{
		int idx = 0;
		TCHAR* s = value;
		_tcscat(s, _T(","));
		struct uaedev_config_info* ci = nullptr;
		for (;;)
		{
			TCHAR* tmpp = _tcschr(s, ',');
			if (tmpp == nullptr)
				return 1;
			*tmpp++ = 0;
			if (idx == 0)
			{
				for (i = 0; i < p->mountitems; i++)
				{
					if (p->mountconfig[i].ci.devname && !_tcscmp(p->mountconfig[i].ci.devname, s))
					{
						ci = &p->mountconfig[i].ci;
						break;
					}
				}
				if (!ci || ci->type != UAEDEV_DIR)
					return 1;
			}
			else
			{
				bool b = true;
				TCHAR* tmpp2 = _tcschr(s, '=');
				if (tmpp2)
				{
					*tmpp2++ = 0;
					if (!_tcsicmp(tmpp2, _T("false")))
						b = false;
				}
				if (!_tcsicmp(s, _T("inject_icons")))
				{
					ci->inject_icons = b;
				}
			}
			idx++;
			s = tmpp;
		}
	}

	return 0;
}

static bool cfgfile_read_board_rom(struct uae_prefs* p, const TCHAR* option, const TCHAR* value)
{
	TCHAR buf[256], buf2[MAX_DPATH], buf3[MAX_DPATH];
	bool dummy;
	int val;
	const struct expansionromtype* ert;

	for (int i = 0; expansionroms[i].name; i++)
	{
		struct boardromconfig* brc;
		int idx;
		ert = &expansionroms[i];

		for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++)
		{
			TCHAR name[256];

			if (j == 0)
				_tcscpy(name, ert->name);
			else
				_stprintf(name, _T("%s-%d"), ert->name, j + 1);

			_stprintf(buf, _T("scsi_%s"), name);
			if (cfgfile_yesno(option, value, buf, &dummy))
			{
				return true;
			}

			_stprintf(buf, _T("%s_rom_file"), name);
			if (cfgfile_path(option, value, buf, buf2, MAX_DPATH / sizeof (TCHAR)))
			{
				if (buf2[0])
				{
					brc = get_device_rom_new(p, ert->romtype, j, &idx);
					_tcscpy(brc->roms[idx].romfile, buf2);
				}
				return true;
			}

			_stprintf(buf, _T("%s_rom_file_id"), name);
			buf2[0] = 0;
			if (cfgfile_rom(option, value, buf, buf2, MAX_DPATH / sizeof (TCHAR)))
			{
				if (buf2[0])
				{
					brc = get_device_rom_new(p, ert->romtype, j, &idx);
					_tcscpy(brc->roms[idx].romfile, buf2);
				}
				return true;
			}

			_stprintf(buf, _T("%s_rom"), name);
			if (cfgfile_string(option, value, buf, buf2, sizeof buf2 / sizeof (TCHAR)))
			{
				if (buf2[0])
				{
					decode_rom_ident(buf3, sizeof(buf3) / sizeof (TCHAR), buf2, ert->romtype);
					if (buf3[0])
					{
						brc = get_device_rom_new(p, ert->romtype, j, &idx);
						_tcscpy(brc->roms[idx].romident, buf3);
					}
				}
				return true;
			}

			_stprintf(buf, _T("%s_rom_options"), name);
			if (cfgfile_string(option, value, buf, buf2, sizeof buf2 / sizeof (TCHAR)))
			{
				brc = get_device_rom(p, ert->romtype, j, &idx);
				if (brc)
				{
					TCHAR* p;
					if (cfgfile_option_bool(buf2, _T("autoboot_disabled")) == 1)
					{
						brc->roms[idx].autoboot_disabled = true;
					}
					p = cfgfile_option_get(buf2, _T("order"));
					if (p)
					{
						brc->device_order = _tstol(p);
					}
				}
				return true;
			}
		}

		_stprintf(buf, _T("%s_mem_size"), ert->name);
		if (cfgfile_intval(option, value, buf, &val, 0x40000))
		{
			if (val)
			{
				brc = get_device_rom_new(p, ert->romtype, 0, &idx);
				brc->roms[idx].board_ram_size = val;
			}
			return true;
		}
	}
	return false;
}

static void addbcromtype(struct uae_prefs* p, int romtype, bool add, const TCHAR* romfile, int devnum)
{
	if (!add)
	{
		clear_device_rom(p, romtype, devnum, true);
	}
	else
	{
		struct boardromconfig* brc = get_device_rom_new(p, romtype, devnum, nullptr);
		if (brc && !brc->roms[0].romfile[0])
		{
			_tcscpy(brc->roms[0].romfile, romfile ? romfile : _T(":ENABLED"));
		}
	}
}

static int cfgfile_parse_hardware(struct uae_prefs* p, const TCHAR* option, TCHAR* value)
{
	int tmpval, dummyint, i;
	TCHAR tmpbuf[CONFIG_BLEN];

	if (cfgfile_yesno(option, value, _T("immediate_blits"), &p->immediate_blits)
		|| cfgfile_yesno(option, value, _T("fast_copper"), &p->fast_copper)
		|| cfgfile_yesno(option, value, _T("fpu_no_unimplemented"), &p->fpu_no_unimplemented)
		|| cfgfile_yesno(option, value, _T("cd32cd"), &p->cs_cd32cd)
		|| cfgfile_yesno(option, value, _T("cd32c2p"), &p->cs_cd32c2p)
		|| cfgfile_yesno(option, value, _T("cd32nvram"), &p->cs_cd32nvram)
		|| cfgfile_yesno(option, value, _T("cia_overlay"), &p->cs_ciaoverlay)
		|| cfgfile_yesno(option, value, _T("ksmirror_e0"), &p->cs_ksmirror_e0)
		|| cfgfile_yesno(option, value, _T("ksmirror_a8"), &p->cs_ksmirror_a8)
		|| cfgfile_yesno(option, value, _T("cia_todbug"), &p->cs_ciatodbug)
		|| cfgfile_yesno(option, value, _T("z3_autoconfig"), &p->cs_z3autoconfig)

		|| cfgfile_yesno(option, value, _T("ntsc"), &p->ntscmode)
		|| cfgfile_yesno(option, value, _T("cpu_compatible"), &p->cpu_compatible)
		|| cfgfile_yesno(option, value, _T("cpu_24bit_addressing"), &p->address_space_24)
		|| cfgfile_yesno(option, value, _T("fpu_strict"), &p->fpu_strict)
#ifdef USE_JIT_FPU
		|| cfgfile_yesno(option, value, _T("compfpu"), &p->compfpu)
#endif
		|| cfgfile_yesno(option, value, _T("floppy_write_protect"), &p->floppy_read_only)
		|| cfgfile_yesno(option, value, _T("harddrive_write_protect"), &p->harddrive_read_only))
		return 1;

	if (cfgfile_intval(option, value, _T("cachesize"), &p->cachesize, 1)
		|| cfgfile_intval(option, value, _T("cd32nvram_size"), &p->cs_cd32nvram_size, 1024)
		|| cfgfile_intval(option, value, _T("fatgary"), &p->cs_fatgaryrev, 1)
		|| cfgfile_intval(option, value, _T("ramsey"), &p->cs_ramseyrev, 1)
		|| cfgfile_doubleval(option, value, _T("chipset_refreshrate"), &p->chipset_refreshrate)
		|| cfgfile_intval(option, value, _T("a3000mem_size"), &p->mbresmem_low_size, 0x100000)
		|| cfgfile_intval(option, value, _T("mbresmem_size"), &p->mbresmem_high_size, 0x100000)
		|| cfgfile_intval(option, value, _T("z3mem_start"), &p->z3autoconfig_start, 1)
		|| cfgfile_intval(option, value, _T("bogomem_size"), &p->bogomem_size, 0x40000)
		|| cfgfile_intval(option, value, _T("rtg_modes"), &p->picasso96_modeflags, 1)
		|| cfgfile_intval(option, value, _T("floppy_speed"), &p->floppy_speed, 1)
		|| cfgfile_intval(option, value, _T("cd_speed"), &p->cd_speed, 1)
		|| cfgfile_intval(option, value, _T("floppy_write_length"), &p->floppy_write_length, 1)
		|| cfgfile_intval(option, value, _T("nr_floppies"), &p->nr_floppies, 1)
		|| cfgfile_intval(option, value, _T("floppy0type"), &p->floppyslots[0].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy1type"), &p->floppyslots[1].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy2type"), &p->floppyslots[2].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy3type"), &p->floppyslots[3].dfxtype, 1))
		return 1;

	if (cfgfile_strval(option, value, _T("rtc"), &p->cs_rtc, rtctype, 0)
		|| cfgfile_strval(option, value, _T("ciaatod"), &p->cs_ciaatod, ciaatodmode, 0)
		|| cfgfile_strval(option, value, _T("collision_level"), &p->collision_level, collmode, 0)
		|| cfgfile_strval(option, value, _T("waiting_blits"), &p->waiting_blits, waitblits, 0)
		|| cfgfile_strval(option, value, _T("floppy_auto_extended_adf"), &p->floppy_auto_ext2, autoext2, 0)
		|| cfgfile_strval(option, value, _T("z3mapping"), &p->z3_mapping_mode, z3mapping, 0)
		|| cfgfile_strval(option, value, _T("boot_rom_uae"), &p->boot_rom, uaebootrom, 0)
		|| cfgfile_strval(option, value, _T("unmapped_address_space"), &p->cs_unmapped_space, unmapped, 0)
		|| cfgfile_strval(option, value, _T("ciaa_type"), &p->cs_ciatype[0], ciatype, 0)
		|| cfgfile_strval(option, value, _T("ciab_type"), &p->cs_ciatype[1], ciatype, 0))
		return 1;

	if (cfgfile_strval(option, value, _T("uaeboard"), &p->uaeboard, uaeboard, 0))
	{
		return 1;
	}

	if (cfgfile_path(option, value, _T("kickstart_rom_file"), p->romfile, sizeof p->romfile / sizeof (TCHAR))
		|| cfgfile_path(option, value, _T("kickstart_ext_rom_file"), p->romextfile, sizeof p->romextfile / sizeof (TCHAR))
		|| cfgfile_path(option, value, _T("flash_file"), p->flashfile, sizeof p->flashfile / sizeof (TCHAR))
		|| cfgfile_path(option, value, _T("cart_file"), p->cartfile, sizeof p->cartfile / sizeof (TCHAR)))
		return 1;

	if (cfgfile_string(option, value, _T("uaeboard_options"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR)))
	{
		TCHAR* s = cfgfile_option_get(value, _T("order"));
		if (s)
			p->uaeboard_order = _tstol(s);
		return 1;
	}

	if (cfgfile_readramboard(option, value, _T("fastmem"), &p->fastmem[0]))
	{
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("z3mem"), &p->z3fastmem[0]))
	{
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("pcmcia"), &p->cs_pcmcia))
	{
		if (p->cs_pcmcia)
			addbcromtype(p, ROMTYPE_MB_PCMCIA, true, nullptr, 0);
		return 1;
	}
	if (cfgfile_strval(option, value, _T("ide"), &p->cs_ide, idemode, 0))
	{
		if (p->cs_ide)
			addbcromtype(p, ROMTYPE_MB_IDE, true, nullptr, 0);
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("cd32fmv"), &p->cs_cd32fmv))
	{
		if (p->cs_cd32fmv)
		{
			addbcromtype(p, ROMTYPE_CD32CART, true, p->cartfile, 0);
		}
		return 1;
	}

	for (int i = 0; i < MAX_RTG_BOARDS; i++)
	{
		struct rtgboardconfig* rbc = &p->rtgboards[i];
		TCHAR tmp[100];
		if (i > 0)
			_stprintf(tmp, _T("gfxcard%d_size"), i + 1);
		else
			_tcscpy(tmp, _T("gfxcard_size"));
		if (cfgfile_intval(option, value, tmp, &rbc->rtgmem_size, 0x100000))
			return 1;
		if (i > 0)
			_stprintf(tmp, _T("gfxcard%d_options"), i + 1);
		else
			_tcscpy(tmp, _T("gfxcard_options"));
		if (!_tcsicmp(option, tmp))
		{
			TCHAR* s = cfgfile_option_get(value, _T("order"));
			if (s)
			{
				rbc->device_order = _tstol(s);
			}
			return 1;
		}
		if (i > 0)
			_stprintf(tmp, _T("gfxcard%d_type"), i + 1);
		else
			_tcscpy(tmp, _T("gfxcard_type"));
		if (cfgfile_string(option, value, tmp, tmpbuf, sizeof tmpbuf / sizeof(TCHAR)))
		{
			rbc->rtgmem_type = 0;
			int j = 0;
			for (;;)
			{
				const TCHAR* t = gfxboard_get_configname(j);
				if (!t)
				{
					break;
				}
				if (!_tcsicmp(t, tmpbuf))
				{
					rbc->rtgmem_type = j;
					break;
				}
				j++;
			}
			return 1;
		}
	}

	if (cfgfile_strval(option, value, _T("chipset_compatible"), &p->cs_compatible, cscompa, 0))
	{
		built_in_chipset_prefs(p);
		return 1;
	}

	if (cfgfile_strval(option, value, _T("cart_internal"), &p->cart_internal, cartsmode, 0))
	{
		if (p->cart_internal)
		{
			struct romdata* rd = getromdatabyid(63);
			if (rd)
				_stprintf(p->cartfile, _T(":%s"), rd->configname);
		}
		return 1;
	}
	if (cfgfile_string(option, value, _T("kickstart_rom"), p->romident, sizeof p->romident / sizeof (TCHAR)))
	{
		decode_rom_ident(p->romfile, sizeof p->romfile / sizeof (TCHAR), p->romident, ROMTYPE_ALL_KICK);
		return 1;
	}
	if (cfgfile_string(option, value, _T("kickstart_ext_rom"), p->romextident, sizeof p->romextident / sizeof (TCHAR)))
	{
		decode_rom_ident(p->romextfile, sizeof p->romextfile / sizeof (TCHAR), p->romextident, ROMTYPE_ALL_EXT);
		return 1;
	}

	if (cfgfile_string(option, value, _T("cart"), p->cartident, sizeof p->cartident / sizeof (TCHAR)))
	{
		decode_rom_ident(p->cartfile, sizeof p->cartfile / sizeof (TCHAR), p->cartident, ROMTYPE_ALL_CART);
		return 1;
	}

	if (cfgfile_read_board_rom(p, option, value))
		return 1;

	for (i = 0; i < 4; i++)
	{
		_stprintf(tmpbuf, _T("floppy%d"), i);
		if (cfgfile_path(option, value, tmpbuf, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof (TCHAR)))
			return 1;
	}

	if (cfgfile_intval(option, value, _T("chipmem_size"), &dummyint, 1))
	{
		if (dummyint < 0)
			p->chipmem_size = 0x20000; /* 128k, prototype support */
		else if (dummyint == 0)
			p->chipmem_size = 0x40000; /* 256k */
		else
			p->chipmem_size = dummyint * 0x80000;
		return 1;
	}

	if (cfgfile_string(option, value, _T("addmem1"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR)))
	{
		parse_addmem(p, tmpbuf, 0);
		return 1;
	}
	if (cfgfile_string(option, value, _T("addmem2"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR)))
	{
		parse_addmem(p, tmpbuf, 1);
		return 1;
	}

	if (cfgfile_strval(option, value, _T("chipset"), &tmpval, csmode, 0))
	{
		set_chipset_mask(p, tmpval);
		return 1;
	}

	if (cfgfile_string(option, value, _T("fpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR)))
	{
		p->fpu_model = _tstol(tmpbuf);
		return 1;
	}

	if (cfgfile_string(option, value, _T("cpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR)))
	{
		p->cpu_model = _tstol(tmpbuf);
		p->fpu_model = 0;
		return 1;
	}

	/* old-style CPU configuration */
	if (cfgfile_string(option, value, _T("cpu_type"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR)))
	{
		// 68000/010 32-bit addressing was not available until 2.8.2
		bool force24bit = p->config_version <= ((2 << 16) | (8 << 8) | (1 << 0));
		p->fpu_model = 0;
		p->address_space_24 = false;
		p->cpu_model = 680000;
		if (!_tcscmp(tmpbuf, _T("68000")))
		{
			p->cpu_model = 68000;
			if (force24bit)
				p->address_space_24 = true;
		}
		else if (!_tcscmp(tmpbuf, _T("68010")))
		{
			p->cpu_model = 68010;
			if (force24bit)
				p->address_space_24 = true;
		}
		else if (!_tcscmp(tmpbuf, _T("68ec020")))
		{
			p->cpu_model = 68020;
		}
		else if (!_tcscmp(tmpbuf, _T("68020")))
		{
			p->cpu_model = 68020;
		}
		else if (!_tcscmp(tmpbuf, _T("68ec020/68881")))
		{
			p->cpu_model = 68020;
			p->fpu_model = 68881;
			p->address_space_24 = true;
		}
		else if (!_tcscmp(tmpbuf, _T("68020/68881")))
		{
			p->cpu_model = 68020;
			p->fpu_model = 68881;
		}
		else if (!_tcscmp(tmpbuf, _T("68040")))
		{
			p->cpu_model = 68040;
			p->fpu_model = 68040;
		}
		return 1;
	}

	/* Broken earlier versions used to write this out as a string.  */
	if (cfgfile_strval(option, value, _T("finegraincpu_speed"), &p->m68k_speed, speedmode, 1))
	{
		p->m68k_speed--;
		return 1;
	}

	if (cfgfile_strval(option, value, _T("cpu_speed"), &p->m68k_speed, speedmode, 1))
	{
		if (_tcsicmp(value, _T("max")) == 0)
			p->m68k_speed = -1;
		else if (_tcsicmp(value, _T("turbo")) == 0)
			p->m68k_speed = -30;
		else
			p->m68k_speed--;
		return 1;
	}
	if (cfgfile_intval(option, value, _T("cpu_speed"), &p->m68k_speed, 1))
	{
		p->m68k_speed *= CYCLE_UNIT;
		return 1;
	}
	if (cfgfile_intval(option, value, _T("finegrain_cpu_speed"), &p->m68k_speed, 1))
	{
		if (OFFICIAL_CYCLE_UNIT > CYCLE_UNIT)
		{
			int factor = OFFICIAL_CYCLE_UNIT / CYCLE_UNIT;
			p->m68k_speed = (p->m68k_speed + factor - 1) / factor;
		}
		if (_tcsicmp(value, _T("max")) == 0)
			p->m68k_speed = -1;
		if (_tcsicmp(value, _T("turbo")) == 0)
			p->m68k_speed = -30;
		return 1;
	}

	if (_tcsicmp(option, _T("quickstart")) == 0)
	{
		int model = 0;
		TCHAR* tmpp = _tcschr(value, ',');
		if (tmpp)
		{
			*tmpp++ = 0;
			TCHAR* tmpp2 = _tcschr(value, ',');
			if (tmpp2)
				*tmpp2 = 0;
			cfgfile_strval(option, value, option, &model, qsmodes, 0);
			if (model >= 0)
			{
				int config = _tstol(tmpp);
				built_in_prefs(p, model, config, 0, 0);
			}
		}
		return 1;
	}

	if (cfgfile_parse_filesys(p, option, value))
		return 1;

	return 0;
}

void cfgfile_compatibility_rtg(struct uae_prefs* p)
{
	int uaegfx = -1;
	// only one uaegfx
	for (int i = 0; i < MAX_RTG_BOARDS; i++)
	{
		struct rtgboardconfig* rbc = &p->rtgboards[i];
		if (rbc->rtgmem_size)
		{
			if (uaegfx >= 0)
			{
				rbc->rtgmem_size = 0;
				rbc->rtgmem_type = 0;
			}
			else
			{
				uaegfx = i;
			}
		}
	}
	// uaegfx must be first
	if (uaegfx > 0)
	{
		struct rtgboardconfig* rbc = &p->rtgboards[uaegfx];
		struct rtgboardconfig* rbc2 = &p->rtgboards[0];
		int size = rbc->rtgmem_size;
		int type = rbc->rtgmem_type;
		rbc->rtgmem_size = rbc2->rtgmem_size;
		rbc->rtgmem_type = rbc2->rtgmem_type;
		rbc2->rtgmem_size = size;
		rbc2->rtgmem_type = type;
	}
	// empty slots last
	bool reorder = true;
	while (reorder)
	{
		reorder = false;
		for (int i = 0; i < MAX_RTG_BOARDS; i++)
		{
			struct rtgboardconfig* rbc = &p->rtgboards[i];
			if (i > 0 && rbc->rtgmem_size && p->rtgboards[i - 1].rtgmem_size == 0)
			{
				struct rtgboardconfig* rbc2 = &p->rtgboards[i - 1];
				rbc2->rtgmem_size = rbc->rtgmem_size;
				rbc2->rtgmem_type = rbc->rtgmem_type;
				rbc2->device_order = rbc->device_order;
				rbc->rtgmem_size = 0;
				rbc->rtgmem_type = 0;
				rbc->device_order = 0;
				reorder = true;
				break;
			}
		}
	}
}

void cfgfile_compatibility_romtype(struct uae_prefs* p)
{
	addbcromtype(p, ROMTYPE_MB_PCMCIA, p->cs_pcmcia, nullptr, 0);

	addbcromtype(p, ROMTYPE_MB_IDE, p->cs_ide != 0, nullptr, 0);

	addbcromtype(p, ROMTYPE_CD32CART, p->cs_cd32fmv, p->cartfile, 0);
}

static bool createconfigstore(struct uae_prefs*);
static int getconfigstoreline(const TCHAR* option, TCHAR* value);

static void calcformula(struct uae_prefs* prefs, TCHAR* in)
{
	TCHAR out[MAX_DPATH], configvalue[CONFIG_BLEN];
	TCHAR* p = out;
	double val;
	int cnt1, cnt2;
	static bool updatestore;

	if (_tcslen(in) < 2 || in[0] != '[' || in[_tcslen(in) - 1] != ']')
		return;
	if (!configstore || updatestore)
		createconfigstore(prefs);
	updatestore = false;
	if (!configstore)
		return;
	cnt1 = cnt2 = 0;
	for (int i = 1; i < _tcslen(in) - 1; i++)
	{
		TCHAR c = _totupper(in[i]);
		if (c >= 'A' && c <= 'Z')
		{
			TCHAR* start = &in[i];
			while (_istalnum (c) || c == '_' || c == '.')
			{
				i++;
				c = in[i];
			}
			TCHAR store = in[i];
			in[i] = 0;
			//write_log (_T("'%s'\n"), start);
			if (!getconfigstoreline(start, configvalue))
				return;
			_tcscpy(p, configvalue);
			p += _tcslen(p);
			in[i] = store;
			i--;
			cnt1++;
		}
		else
		{
			cnt2++;
			*p ++ = c;
		}
	}
	*p = 0;
	if (cnt1 == 0 && cnt2 == 0)
		return;
	/* single config entry only? */
	if (cnt1 == 1 && cnt2 == 0)
	{
		_tcscpy(in, out);
		updatestore = true;
		return;
	}
	if (calc(out, &val))
	{
		if (val - (int)val != 0.0f)
			_stprintf(in, _T("%f"), val);
		else
			_stprintf(in, _T("%d"), (int)val);
		updatestore = true;
	}
}

int cfgfile_parse_option(struct uae_prefs* p, const TCHAR* option, TCHAR* value, int type)
{
	calcformula(p, value);

	if (!_tcscmp(option, _T("config_hardware")))
		return 1;
	if (!_tcscmp(option, _T("config_host")))
		return 1;
	if (type == 0 || (type & CONFIG_TYPE_HARDWARE))
	{
		if (cfgfile_parse_hardware(p, option, value))
			return 1;
	}
	if (type == 0 || (type & CONFIG_TYPE_HOST))
	{
		// cfgfile_parse_host may modify the option (convert to lowercase).
		TCHAR* writable_option = my_strdup(option);
		if (cfgfile_parse_host(p, writable_option, value))
		{
			free(writable_option);
			return 1;
		}
		free(writable_option);
	}
	if (type > 0 && (type & (CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST)) != (CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST))
		return 1;
	return 0;
}

static int isutf8ext(TCHAR* s)
{
	if (_tcslen(s) > _tcslen(UTF8NAME) && !_tcscmp(s + _tcslen(s) - _tcslen(UTF8NAME), UTF8NAME))
	{
		s[_tcslen(s) - _tcslen(UTF8NAME)] = 0;
		return 1;
	}
	return 0;
}

int cfgfile_separate_linea(const TCHAR* filename, char* line, TCHAR* line1b, TCHAR* line2b)
{
	char *line1, *line2;
	int i;

	line1 = line;
	line1 += strspn(line1, "\t \r\n");
	if (*line1 == ';')
		return 0;
	line2 = strchr(line, '=');
	if (! line2)
	{
		TCHAR* s = au(line1);
		cfgfile_warning(_T("CFGFILE: '%s', linea was incomplete with only %s\n"), filename, s);
		xfree (s);
		return 0;
	}
	*line2++ = '\0';

	/* Get rid of whitespace.  */
	i = strlen(line2);
	while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		|| line2[i - 1] == '\r' || line2[i - 1] == '\n'))
		line2[--i] = '\0';
	line2 += strspn(line2, "\t \r\n");

	i = strlen(line);
	while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
		|| line[i - 1] == '\r' || line[i - 1] == '\n'))
		line[--i] = '\0';
	line += strspn(line, "\t \r\n");
	au_copy(line1b, MAX_DPATH, line);
	if (isutf8ext(line1b))
	{
		if (line2[0])
		{
			TCHAR* s = utf8u(line2);
			_tcscpy(line2b, s);
			xfree (s);
		}
	}
	else
	{
		au_copy(line2b, MAX_DPATH, line2);
	}
	return 1;
}

static int cfgfile_separate_line(TCHAR* line, TCHAR* line1b, TCHAR* line2b)
{
	TCHAR *line1, *line2;
	int i;

	line1 = line;
	line1 += _tcsspn(line1, _T("\t \r\n"));
	if (*line1 == ';')
		return 0;
	line2 = _tcschr(line, '=');
	if (! line2)
	{
		cfgfile_warning(_T("CFGFILE: line was incomplete with only %s\n"), line1);
		return 0;
	}
	*line2++ = '\0';

	/* Get rid of whitespace.  */
	i = _tcslen(line2);
	while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		|| line2[i - 1] == '\r' || line2[i - 1] == '\n'))
		line2[--i] = '\0';
	line2 += _tcsspn(line2, _T("\t \r\n"));
	_tcscpy(line2b, line2);
	i = _tcslen(line);
	while (i > 0 && (line[i - 1] == '\t' || line[i - 1] == ' '
		|| line[i - 1] == '\r' || line[i - 1] == '\n'))
		line[--i] = '\0';
	line += _tcsspn(line, _T("\t \r\n"));
	_tcscpy(line1b, line);

	if (line2b[0] == '"' || line2b[0] == '\"')
	{
		TCHAR c = line2b[0];
		int i = 0;
		memmove(line2b, line2b + 1, (_tcslen(line2b) + 1) * sizeof (TCHAR));
		while (line2b[i] != 0 && line2b[i] != c)
			i++;
		line2b[i] = 0;
	}

	if (isutf8ext(line1b))
		return 0;
	return 1;
}

static int isobsolete(TCHAR* s)
{
	int i = 0;
	while (obsolete[i])
	{
		if (!_tcsicmp(s, obsolete[i]))
		{
			cfgfile_warning_obsolete(_T("obsolete config entry '%s'\n"), s);
			return 1;
		}
		i++;
	}
	if (_tcslen(s) > 2 && !_tcsncmp(s, _T("w."), 2))
		return 1;
	if (_tcslen(s) >= 10 && !_tcsncmp(s, _T("gfx_opengl"), 10))
	{
		cfgfile_warning_obsolete(_T("obsolete config entry '%s\n"), s);
		return 1;
	}
	if (_tcslen(s) >= 6 && !_tcsncmp(s, _T("gfx_3d"), 6))
	{
		cfgfile_warning_obsolete(_T("obsolete config entry '%s\n"), s);
		return 1;
	}
	return 0;
}

static void cfgfile_parse_separated_line(struct uae_prefs* p, TCHAR* line1b, TCHAR* line2b, int type)
{
	TCHAR line3b[CONFIG_BLEN], line4b[CONFIG_BLEN];
	struct strlist* sl;
	int ret;

	_tcscpy(line3b, line1b);
	_tcscpy(line4b, line2b);
	ret = cfgfile_parse_option(p, line1b, line2b, type);
	if (!isobsolete(line3b))
	{
		for (sl = p->all_lines; sl; sl = sl->next)
		{
			if (sl->option && !_tcsicmp(line1b, sl->option)) break;
		}
		if (!sl)
		{
			struct strlist* u = xcalloc (struct strlist, 1);
			u->option = my_strdup(line3b);
			u->value = my_strdup(line4b);
			u->next = p->all_lines;
			p->all_lines = u;
			if (!ret)
			{
				u->unknown = 1;
				cfgfile_warning(_T("unknown config entry: '%s=%s'\n"), u->option, u->value);
			}
		}
	}
}

void cfgfile_parse_line(struct uae_prefs* p, TCHAR* line, int type)
{
	TCHAR line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

	if (!cfgfile_separate_line(line, line1b, line2b))
		return;
	cfgfile_parse_separated_line(p, line1b, line2b, type);
}

static void subst(TCHAR* p, TCHAR* f, int n)
{
	if (_tcslen(p) == 0 || _tcslen(f) == 0)
		return;
	TCHAR* str = cfgfile_subst_path(UNEXPANDED, p, f);
	_tcsncpy(f, str, n - 1);
	f[n - 1] = '\0';
	free(str);
}

static int getconfigstoreline(const TCHAR* option, TCHAR* value)
{
	TCHAR tmp[CONFIG_BLEN * 2], tmp2[CONFIG_BLEN * 2];

	if (!configstore)
		return 0;
	zfile_fseek(configstore, 0, SEEK_SET);
	for (;;)
	{
		if (!zfile_fgets(tmp, sizeof tmp / sizeof (TCHAR), configstore))
			return 0;
		if (!cfgfile_separate_line(tmp, tmp2, value))
			continue;
		if (!_tcsicmp(option, tmp2))
			return 1;
	}
}

static bool createconfigstore(struct uae_prefs* p)
{
	uae_u8 zeros[4] = {0};
	zfile_fclose(configstore);
	configstore = zfile_fopen_empty(nullptr, _T("configstore"), 50000);
	if (!configstore)
		return false;
	zfile_fseek(configstore, 0, SEEK_SET);
	uaeconfig++;
	cfgfile_save_options(configstore, p, 0);
	uaeconfig--;
	zfile_fwrite(zeros, 1, sizeof zeros, configstore);
	zfile_fseek(configstore, 0, SEEK_SET);
	return true;
}

static char* cfg_fgets(char* line, int max, struct zfile* fh)
{
	if (fh)
		return zfile_fgetsa(line, max, fh);
	return nullptr;
}

static int cfgfile_load_2(struct uae_prefs* p, const TCHAR* filename, bool real, int* type)
{
	int i;
	struct zfile* fh;
	char linea[CONFIG_BLEN];
	TCHAR line[CONFIG_BLEN], line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];
	struct strlist* sl;
	bool type1 = false, type2 = false;
	int askedtype = 0;

	if (type)
	{
		askedtype = *type;
		*type = 0;
	}
	if (real)
	{
		p->config_version = 0;
		config_newfilesystem = 0;
		//reset_inputdevice_config (p);
	}

	fh = zfile_fopen(filename, _T("r"), ZFD_NORMAL);
	if (! fh)
		return 0;

	while (cfg_fgets(linea, sizeof (linea), fh) != nullptr)
	{
		trimwsa(linea);
		if (strlen(linea) > 0)
		{
			if (linea[0] == '#' || linea[0] == ';')
			{
				struct strlist* u = xcalloc (struct strlist, 1);
				u->option = nullptr;
				TCHAR* com = au(linea);
				u->value = my_strdup(com);
				xfree (com);
				u->unknown = 1;
				u->next = p->all_lines;
				p->all_lines = u;
				continue;
			}
			if (!cfgfile_separate_linea(filename, linea, line1b, line2b))
				continue;
			type1 = type2 = false;
			if (cfgfile_yesno(line1b, line2b, _T("config_hardware"), &type1) ||
				cfgfile_yesno(line1b, line2b, _T("config_host"), &type2))
			{
				if (type1 && type)
					*type |= CONFIG_TYPE_HARDWARE;
				if (type2 && type)
					*type |= CONFIG_TYPE_HOST;
				continue;
			}
			if (real)
			{
				cfgfile_parse_separated_line(p, line1b, line2b, askedtype);
			}
			else
			{
				cfgfile_string(line1b, line2b, _T("config_description"), p->description, sizeof p->description / sizeof (TCHAR));
			}
		}
	}

	if (type && *type == 0)
		*type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
	zfile_fclose(fh);

	if (!real)
		return 1;

	for (sl = temp_lines; sl; sl = sl->next)
	{
		_stprintf(line, _T("%s=%s"), sl->option, sl->value);
		cfgfile_parse_line(p, line, 0);
	}

	for (i = 0; i < 4; i++)
	{
		subst(p->path_floppy, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof (TCHAR));
		if (i >= p->nr_floppies)
			p->floppyslots[i].dfxtype = DRV_NONE;
	}
	subst(p->path_rom, p->romfile, sizeof p->romfile / sizeof (TCHAR));
	subst(p->path_rom, p->romextfile, sizeof p->romextfile / sizeof (TCHAR));

	for (i = 0; i < MAX_EXPANSION_BOARDS; i++)
	{
		for (int j = 0; j < MAX_BOARD_ROMS; j++)
		{
			subst(p->path_rom, p->expansionboard[i].roms[j].romfile, MAX_DPATH / sizeof(TCHAR));
		}
	}

	return 1;
}

int cfgfile_load(struct uae_prefs* p, const TCHAR* filename, int* type, int ignorelink, int userconfig)
{
	int v;
	static int recursive;

	if (recursive > 1)
		return 0;
	recursive++;
	write_log (_T("load config '%s':%d\n"), filename, type ? *type : -1);
	v = cfgfile_load_2(p, filename, true, type);
	if (!v)
	{
		cfgfile_warning(_T("cfgfile_load_2 failed\n"));
		goto end;
	}
	if (userconfig)
		target_addtorecent(filename, 0);
end:
	recursive--;
	fixup_prefs(p, userconfig != 0);
	return v;
}

int cfgfile_save(struct uae_prefs* p, const TCHAR* filename, int type)
{
	struct zfile* fh;

	fh = zfile_fopen(filename, _T("w"), ZFD_NORMAL);
	if (! fh)
		return 0;

	if (!type)
		type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
	cfgfile_save_options(fh, p, type);
	zfile_fclose(fh);
	return 1;
}

int cfgfile_get_description(const TCHAR* filename, TCHAR* description)
{
	int result = 0;
	struct uae_prefs* p = xmalloc (struct uae_prefs, 1);

	p->description[0] = 0;
	if (cfgfile_load_2(p, filename, false, nullptr))
	{
		result = 1;
		if (description)
			_tcscpy(description, p->description);
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

static void parse_sound_spec(struct uae_prefs* p, const TCHAR* spec)
{
	TCHAR* x0 = my_strdup(spec);
	TCHAR *x1, *x2 = nullptr, *x3 = nullptr;

	x1 = _tcschr(x0, ':');
	if (x1 != nullptr)
	{
		*x1++ = '\0';
		x2 = _tcschr(x1 + 1, ':');
		if (x2 != nullptr)
		{
			*x2++ = '\0';
			x3 = _tcschr(x2 + 1, ':');
			if (x3 != nullptr)
			{
				*x3++ = '\0';
			}
		}
	}
	p->produce_sound = _tstoi(x0);
	if (x1)
	{
		p->sound_stereo_separation = 0;
		if (*x1 == 'S')
		{
			p->sound_stereo = SND_STEREO;
			p->sound_stereo_separation = 7;
		}
		else if (*x1 == 's')
			p->sound_stereo = SND_STEREO;
		else
			p->sound_stereo = SND_MONO;
	}
	if (x3)
		p->sound_freq = _tstoi(x3);
	free(x0);
}

static void parse_joy_spec(struct uae_prefs* p, const TCHAR* spec)
{
	int v0 = 2, v1 = 0;
	if (_tcslen(spec) != 2)
		goto bad;

	switch (spec[0])
	{
	case '0': v0 = JSEM_JOYS;
		break;
	case '1': v0 = JSEM_JOYS + 1;
		break;
	case 'M': case 'm': v0 = JSEM_MICE;
		break;
	case 'A': case 'a': v0 = JSEM_KBDLAYOUT;
		break;
	case 'B': case 'b': v0 = JSEM_KBDLAYOUT + 1;
		break;
	case 'C': case 'c': v0 = JSEM_KBDLAYOUT + 2;
		break;
	default: goto bad;
	}

	switch (spec[1])
	{
	case '0': v1 = JSEM_JOYS;
		break;
	case '1': v1 = JSEM_JOYS + 1;
		break;
	case 'M': case 'm': v1 = JSEM_MICE;
		break;
	case 'A': case 'a': v1 = JSEM_KBDLAYOUT;
		break;
	case 'B': case 'b': v1 = JSEM_KBDLAYOUT + 1;
		break;
	case 'C': case 'c': v1 = JSEM_KBDLAYOUT + 2;
		break;
	default: goto bad;
	}
	if (v0 == v1)
		goto bad;
	/* Let's scare Pascal programmers */
	if (false)
	bad:
		write_log (_T("Bad joystick mode specification. Use -J xy, where x and y\n")
			_T("can be 0 for joystick 0, 1 for joystick 1, M for mouse, and\n")
			_T("a, b or c for different keyboard settings.\n"));

	p->jports[0].id = v0;
	p->jports[1].id = v1;
}

static void parse_filesys_spec(struct uae_prefs* p, bool readonly, const TCHAR* spec)
{
	struct uaedev_config_info uci;
	TCHAR buf[256];
	TCHAR* s2;

	uci_set_defaults(&uci, false);
	_tcsncpy(buf, spec, 255);
	buf[255] = 0;
	s2 = _tcschr(buf, ':');
	if (s2)
	{
		*s2++ = '\0';
#ifdef __DOS__
	  {
      TCHAR *tmp;

      while ((tmp = _tcschr (s2, '\\')))
    		*tmp = '/';
	  }
#endif
#ifdef FILESYS
		_tcscpy(uci.volname, buf);
		_tcscpy(uci.rootdir, s2);
		uci.readonly = readonly;
		uci.type = UAEDEV_DIR;
		add_filesys_config(p, -1, &uci);
#endif
	}
	else
	{
		write_log (_T("Usage: [-m | -M] VOLNAME:mount_point\n"));
	}
}

static void parse_hardfile_spec(struct uae_prefs* p, const TCHAR* spec)
{
	struct uaedev_config_info uci;
	TCHAR* x0 = my_strdup(spec);
	TCHAR *x1, *x2, *x3, *x4;

	uci_set_defaults(&uci, false);
	x1 = _tcschr(x0, ':');
	if (x1 == nullptr)
		goto argh;
	*x1++ = '\0';
	x2 = _tcschr(x1 + 1, ':');
	if (x2 == nullptr)
		goto argh;
	*x2++ = '\0';
	x3 = _tcschr(x2 + 1, ':');
	if (x3 == nullptr)
		goto argh;
	*x3++ = '\0';
	x4 = _tcschr(x3 + 1, ':');
	if (x4 == nullptr)
		goto argh;
	*x4++ = '\0';
#ifdef FILESYS
	_tcscpy(uci.rootdir, x4);
	//add_filesys_config (p, -1, NULL, NULL, x4, 0, 0, _tstoi (x0), _tstoi (x1), _tstoi (x2), _tstoi (x3), 0, 0, 0, 0, 0, 0, 0);
#endif
	free(x0);
	return;

argh:
	free(x0);
	cfgfile_warning(_T("Bad hardfile parameter specified\n"));
	return;
}

static void parse_cpu_specs(struct uae_prefs* p, const TCHAR* spec)
{
	if (*spec < '0' || *spec > '4')
	{
		cfgfile_warning(_T("CPU parameter string must begin with '0', '1', '2', '3' or '4'.\n"));
		return;
	}

	p->cpu_model = (*spec++) * 10 + 68000;
	p->address_space_24 = p->cpu_model < 68020;
	p->cpu_compatible = false;
	while (*spec != '\0')
	{
		switch (*spec)
		{
		case 'a':
			if (p->cpu_model < 68020)
			cfgfile_warning(_T("In 68000/68010 emulation, the address space is always 24 bit.\n"));
			else if (p->cpu_model >= 68040)
			cfgfile_warning(_T("In 68040/060 emulation, the address space is always 32 bit.\n"));
			else
				p->address_space_24 = true;
			break;
		case 'c':
			if (p->cpu_model != 68000)
			cfgfile_warning(_T("The more compatible CPU emulation is only available for 68000\n")
				_T("emulation, not for 68010 upwards.\n"));
			else
				p->cpu_compatible = true;
			break;
		default:
			cfgfile_warning(_T("Bad CPU parameter specified.\n"));
			break;
		}
		spec++;
	}
}

static void cmdpath(TCHAR* dst, const TCHAR* src, int maxsz)
{
	TCHAR* s = target_expand_environment(src, nullptr, 0);
	_tcsncpy(dst, s, maxsz);
	dst[maxsz] = 0;
	xfree (s);
}

/* Returns the number of args used up (0 or 1).  */
int parse_cmdline_option(struct uae_prefs* p, TCHAR c, const TCHAR* arg)
{
	struct strlist* u = xcalloc (struct strlist, 1);
	const TCHAR arg_required[] = _T("0123rKpImMWSRJwnvCZUFbclOdH");

	if (_tcschr(arg_required, c) && ! arg)
	{
		write_log (_T("Missing argument for option `-%c'!\n"), c);
		return 0;
	}

	u->option = xmalloc (TCHAR, 2);
	u->option[0] = c;
	u->option[1] = 0;
	if (arg)
		u->value = my_strdup(arg);
	u->next = p->all_lines;
	p->all_lines = u;

	switch (c)
	{
	case '0': cmdpath(p->floppyslots[0].df, arg, 255);
		break;
	case '1': cmdpath(p->floppyslots[1].df, arg, 255);
		break;
	case '2': cmdpath(p->floppyslots[2].df, arg, 255);
		break;
	case '3': cmdpath(p->floppyslots[3].df, arg, 255);
		break;
	case 'r': cmdpath(p->romfile, arg, 255);
		break;
	case 'K': cmdpath(p->romextfile, arg, 255);
		break;
	case 'm': case 'M': parse_filesys_spec(p, c == 'M', arg);
		break;
	case 'W': parse_hardfile_spec(p, arg);
		break;
	case 'S': parse_sound_spec(p, arg);
		break;
	case 'R': p->gfx_framerate = _tstoi(arg);
		break;
	case 'J': parse_joy_spec(p, arg);
		break;

	case 'w': p->m68k_speed = _tstoi(arg);
		break;

	case 'G': p->start_gui = false;
		break;

	case 'n':
		if (_tcschr(arg, 'i') != nullptr)
			p->immediate_blits = true;
		break;

	case 'v':
		set_chipset_mask(p, _tstoi(arg));
		break;

	case 'C':
		parse_cpu_specs(p, arg);
		break;

	case 'Z':
		p->z3fastmem[0].size = _tstoi(arg) * 0x100000;
		break;

	case 'U':
		p->rtgboards[0].rtgmem_size = _tstoi(arg) * 0x100000;
		break;

	case 'F':
		p->fastmem[0].size = _tstoi(arg) * 0x100000;
		break;

	case 'b':
		p->bogomem_size = _tstoi(arg) * 0x40000;
		break;

	case 'c':
		p->chipmem_size = _tstoi(arg) * 0x80000;
		break;

	case 'l':
		if (0 == _tcsicmp(arg, _T("de")))
			p->keyboard_lang = KBD_LANG_DE;
		else if (0 == _tcsicmp(arg, _T("dk")))
			p->keyboard_lang = KBD_LANG_DK;
		else if (0 == _tcsicmp(arg, _T("us")))
			p->keyboard_lang = KBD_LANG_US;
		else if (0 == _tcsicmp(arg, _T("se")))
			p->keyboard_lang = KBD_LANG_SE;
		else if (0 == _tcsicmp(arg, _T("fr")))
			p->keyboard_lang = KBD_LANG_FR;
		else if (0 == _tcsicmp(arg, _T("it")))
			p->keyboard_lang = KBD_LANG_IT;
		else if (0 == _tcsicmp(arg, _T("es")))
			p->keyboard_lang = KBD_LANG_ES;
		break;

	default:
		write_log (_T("Unknown option `-%c'!\n"), c);
		break;
	}
	return !! _tcschr(arg_required, c);
}

void cfgfile_addcfgparam(TCHAR* line)
{
	struct strlist* u;
	TCHAR line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];

	if (!line)
	{
		struct strlist** ps = &temp_lines;
		while (*ps)
		{
			struct strlist* s = *ps;
			*ps = s->next;
			xfree (s->value);
			xfree (s->option);
			xfree (s);
		}
		temp_lines = nullptr;
		return;
	}
	if (!cfgfile_separate_line(line, line1b, line2b))
		return;
	u = xcalloc (struct strlist, 1);
	u->option = my_strdup(line1b);
	u->value = my_strdup(line2b);
	u->next = temp_lines;
	temp_lines = u;
}

static int cmdlineparser(const TCHAR* s, TCHAR* outp[], int max)
{
	int j, cnt = 0;
	int slash = 0;
	int quote = 0;
	TCHAR tmp1[MAX_DPATH];
	const TCHAR* prev;
	int doout;

	doout = 0;
	prev = s;
	j = 0;
	outp[0] = nullptr;
	while (cnt < max)
	{
		TCHAR c = *s++;
		if (!c)
			break;
		if (c < 32)
			continue;
		if (c == '\\')
			slash = 1;
		if (!slash && c == '"')
		{
			if (quote)
			{
				quote = 0;
				doout = 1;
			}
			else
			{
				quote = 1;
				j = -1;
			}
		}
		if (!quote && c == ' ')
			doout = 1;
		if (!doout)
		{
			if (j >= 0)
			{
				tmp1[j] = c;
				tmp1[j + 1] = 0;
			}
			j++;
		}
		if (doout)
		{
			if (_tcslen(tmp1) > 0)
			{
				outp[cnt++] = my_strdup(tmp1);
				outp[cnt] = nullptr;
			}
			tmp1[0] = 0;
			doout = 0;
			j = 0;
		}
		slash = 0;
	}
	if (j > 0 && cnt < max)
	{
		outp[cnt++] = my_strdup(tmp1);
		outp[cnt] = nullptr;
	}
	return cnt;
}

#define UAELIB_MAX_PARSE 100

static bool cfgfile_parse_uaelib_option(struct uae_prefs* p, TCHAR* option, TCHAR* value, int type)
{
	return false;
}

int cfgfile_searchconfig(const TCHAR *in, int index, TCHAR *out, int outsize)
{
	TCHAR tmp[CONFIG_BLEN];
	int j = 0;
	int inlen = _tcslen(in);
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

	if (index < 0) {
		index = 0;
		zfile_fseek(configstore, 0, SEEK_SET);
	} else {
		// if seek position==0: configstore was reset, always start from the beginning.
		if (zfile_ftell(configstore) > 0)
			index = 0;
	}

	tmp[0] = 0;
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
				if (index <= 0) {
				TCHAR* p;
				if (joker)
					p = tmp - 1;
				else
					p = _tcschr(tmp, '=');
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
				} else {
					index--;
				}
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

uae_u32 cfgfile_modify(uae_u32 index, const TCHAR* parms, uae_u32 size, TCHAR* out, uae_u32 outsize)
{
	TCHAR* p;
	TCHAR* argc[UAELIB_MAX_PARSE];
	int argv, i;
	uae_u32 err;
	static TCHAR* configsearch;

	*out = 0;
	err = 0;
	argv = 0;
	p = nullptr;
	if (index != 0xffffffff)
	{
		if (!configstore)
		{
			err = 20;
			goto end;
		}
		if (configsearch)
		{
			err = cfgfile_searchconfig(configsearch, index, out, outsize);
			goto end;
		}
		err = 0xffffffff;
		for (i = 0; out && i < outsize - 1; i++)
		{
			uae_u8 b = 0;
			if (zfile_fread(&b, 1, 1, configstore) != 1)
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

	argv = cmdlineparser(parms, argc, UAELIB_MAX_PARSE);

	if (argv <= 1 && index == 0xffffffff)
	{
		createconfigstore(&currprefs);
		xfree (configsearch);
		configsearch = nullptr;
		if (!configstore)
		{
			err = 20;
			goto end;
		}
		if (argv > 0 && _tcslen(argc[0]) > 0)
			configsearch = my_strdup(argc[0]);
		err = 0xffffffff;
		goto end;
	}

	for (i = 0; i < argv; i++)
	{
		if (i + 2 <= argv)
		{
			if (!inputdevice_uaelib(argc[i], argc[i + 1]))
			{
				if (!cfgfile_parse_uaelib_option(&changed_prefs, argc[i], argc[i + 1], 0))
				{
					if (!cfgfile_parse_option(&changed_prefs, argc[i], argc[i + 1], 0))
					{
						err = 5;
						break;
					}
				}
			}
			inputdevice_fix_prefs(&changed_prefs, false);
			set_config_changed();
			set_special(SPCFLAG_MODE_CHANGE);
			i++;
		}
	}
end:
	for (i = 0; i < argv; i++)
		xfree (argc[i]);
	xfree (p);
	return err;
}

uae_u32 cfgfile_uaelib_modify(TrapContext* ctx, uae_u32 index, uae_u32 parms, uae_u32 size, uae_u32 out,
                              uae_u32 outsize)
{
	uae_char *p, *parms_p = nullptr, *parms_out = nullptr;
	int i, ret;
	TCHAR *out_p = nullptr, *parms_in = nullptr;

	if (out)
		trap_put_byte(ctx, out, 0);
	if (size == 0)
	{
		while (trap_get_byte(ctx, parms + size) != 0)
			size++;
	}
	parms_p = xmalloc (uae_char, size + 1);
	if (!parms_p)
	{
		ret = 10;
		goto end;
	}
	if (out)
	{
		out_p = xmalloc (TCHAR, outsize + 1);
		if (!out_p)
		{
			ret = 10;
			goto end;
		}
		out_p[0] = 0;
	}
	p = parms_p;
	for (i = 0; i < size; i++)
	{
		p[i] = trap_get_byte(ctx, parms + i);
		if (p[i] == 10 || p[i] == 13 || p[i] == 0)
			break;
	}
	p[i] = 0;
	parms_in = au(parms_p);
	ret = cfgfile_modify(index, parms_in, size, out_p, outsize);
	xfree (parms_in);
	if (out)
	{
		parms_out = ua(out_p);
		trap_put_string(ctx, parms_out, out, outsize - 1);
	}
	xfree (parms_out);
end:
	xfree (out_p);
	xfree (parms_p);
	return ret;
}

static const TCHAR* cfgfile_read_config_value(const TCHAR* option)
{
	struct strlist* sl;
	for (sl = currprefs.all_lines; sl; sl = sl->next)
	{
		if (sl->option && !_tcsicmp(sl->option, option))
			return sl->value;
	}
	return nullptr;
}

uae_u32 cfgfile_uaelib(TrapContext* ctx, int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen)
{
	TCHAR* str;
	uae_char tmpa[CONFIG_BLEN];

	if (mode)
		return 0;

	trap_get_string(ctx, tmpa, name, sizeof tmpa);
	str = au(tmpa);
	if (str[0] == 0)
	{
		xfree(str);
		return 0;
	}
	const TCHAR* value = cfgfile_read_config_value(str);
	xfree(str);
	if (value)
	{
		char* s = ua(value);
		trap_put_string(ctx, s, dst, maxlen);
		xfree (s);
		return dst;
	}
	return 0;
}

uae_u8 *restore_configuration (uae_u8 *src)
{
	struct uae_prefs *p = &currprefs;
	TCHAR *sp = au ((char*)src);
	TCHAR *s = sp;
	for (;;) {
		TCHAR option[MAX_DPATH];
		TCHAR value[MAX_DPATH];
		TCHAR tmp[MAX_DPATH];

		TCHAR *ss = s;
		while (*s && *s != 10 && *s != 13)
			s++;
		if (*s == 0) {
			xfree(sp);
			return src += strlen((char*)src) + 1;
		}
		*s++ = 0;
		while (*s == 10 || *s == 13)
			s++;
		if (cfgfile_separate_line(ss, option, value)) {
			if (!_tcsncmp(option, _T("diskimage"), 9)) {
				for (int i = 0; i < MAX_SPARE_DRIVES; i++) {
					_stprintf(tmp, _T("diskimage%d"), i);
					if (!_tcscmp(option, tmp)) {
						cfgfile_string(option, value, tmp, p->dfxlist[i], sizeof p->dfxlist[i] / sizeof(TCHAR));
						break;
					}
				}
			}
		}
	}
}

uae_u8 *save_configuration (int *len, bool fullconfig)
{
	int tmpsize = 100000;
	uae_u8 *dstbak, *dst, *p;
	int index = -1;

	dstbak = dst = xcalloc (uae_u8, tmpsize);
	p = dst;
	for (;;) {
		TCHAR tmpout[1000];
		int ret;
		tmpout[0] = 0;
		ret = cfgfile_modify (index, _T("*"), 1, tmpout, sizeof (tmpout) / sizeof (TCHAR));
		index++;
		if (_tcslen (tmpout) > 0) {
			char *out;
			if (!fullconfig && !_tcsncmp (tmpout, _T("input."), 6))
				continue;
			//write_log (_T("'%s'\n"), tmpout);
			out = uutf8 (tmpout);
			strcpy ((char*)p, out);
			xfree (out);
			strcat ((char*)p, "\n");
			p += strlen ((char*)p);
			if (p - dstbak >= tmpsize - sizeof (tmpout))
				break;
		}
		if (ret >= 0)
			break;
	}
	*len = p - dstbak + 1;
	return dstbak;
}

#ifdef UAE_MINI
static void default_prefs_mini (struct uae_prefs *p, int type)
{
	_tcscpy (p->description, _T("UAE default A500 configuration"));

	p->nr_floppies = 1;
	p->floppyslots[0].dfxtype = DRV_35_DD;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cpu_model = 68000;
	p->address_space_24 = 1;
	p->chipmem_size = 0x00080000;
	p->bogomem_size = 0x00080000;
}
#endif

#include "sounddep/sound.h"

static void copy_inputdevice_settings(struct uae_input_device *src, struct uae_input_device *dst)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			if (src->custom[l][i]) {
				dst->custom[l][i] = my_strdup(src->custom[l][i]);
			} else {
				dst->custom[l][i] = NULL;
			}
		}
	}
}

static void copy_inputdevice_settings_free(struct uae_input_device *src, struct uae_input_device *dst)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			if (dst->custom[l][i] == NULL && src->custom[l][i] == NULL)
				continue;
			// same string in both src and dest: separate them (fixme: this shouldn't happen!)
			if (dst->custom[l][i] == src->custom[l][i]) {
				dst->custom[l][i] = NULL;
			} else {
				if (dst->custom[l][i]) {
					xfree(dst->custom[l][i]);
					dst->custom[l][i] = NULL;
				}
			}
		}
	}
}

void copy_prefs(struct uae_prefs *src, struct uae_prefs *dst)
{
	for (int slot = 0; slot < MAX_INPUT_SETTINGS; slot++) {
		for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
			copy_inputdevice_settings_free(&src->joystick_settings[slot][m], &dst->joystick_settings[slot][m]);
			copy_inputdevice_settings_free(&src->mouse_settings[slot][m], &dst->mouse_settings[slot][m]);
			copy_inputdevice_settings_free(&src->keyboard_settings[slot][m], &dst->keyboard_settings[slot][m]);
		}
	}
	memcpy(dst, src, sizeof(struct uae_prefs));
	for (int slot = 0; slot < MAX_INPUT_SETTINGS; slot++) {
		for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
			copy_inputdevice_settings(&src->joystick_settings[slot][m], &dst->joystick_settings[slot][m]);
			copy_inputdevice_settings(&src->mouse_settings[slot][m], &dst->mouse_settings[slot][m]);
			copy_inputdevice_settings(&src->keyboard_settings[slot][m], &dst->keyboard_settings[slot][m]);
		}
	}
}

void default_prefs(struct uae_prefs* p, bool reset, int type)
{
	int roms[] = {6, 7, 8, 9, 10, 14, 5, 4, 3, 2, 1, -1};
	TCHAR zero = 0;

	reset_inputdevice_config(p, reset);
	memset(p, 0, sizeof(struct uae_prefs));
	_tcscpy(p->description, _T("UAE default configuration"));
	p->config_hardware_path[0] = 0;
	p->config_host_path[0] = 0;

	p->gfx_scandoubler = false;
	p->start_gui = true;
	p->start_debugger = false;

	p->all_lines = nullptr;

	p->z3_mapping_mode = Z3MAPPING_AUTO;

	p->mountitems = 0;
	for (int i = 0; i < MOUNT_CONFIG_SIZE; i++)
	{
		p->mountconfig[i].configoffset = -1;
		p->mountconfig[i].unitnum = -1;
	}

	p->jports[0].id = -1;
	p->jports[1].id = -1;
	p->jports[2].id = -1;
	p->jports[3].id = -1;
	if (reset)
	{
		inputdevice_joyport_config_store(p, _T("mouse"), 0, -1, 0);
		inputdevice_joyport_config_store(p, _T("joy1"), 1, -1, 0);
	}
	p->keyboard_lang = KBD_LANG_US;
	p->keyboard_connected = true;

	p->produce_sound = 3;
	p->sound_stereo = SND_STEREO;
	p->sound_stereo_separation = 7;
	p->sound_mixed_stereo_delay = 0;
	p->sound_freq = DEFAULT_SOUND_FREQ;
	p->sound_interpol = 0;
	p->sound_filter = FILTER_SOUND_OFF;
	p->sound_filter_type = 0;
	p->sound_volume_cd = 20;

	p->comptrustbyte = 0;
	p->comptrustword = 0;
	p->comptrustlong = 0;
	p->comptrustnaddr= 0;
	p->compnf = 1;
	p->comp_hardflush = 0;
	p->comp_constjump = 1;
#ifdef USE_JIT_FPU
	p->compfpu = 1;
#else
	p->compfpu = 0;
#endif
	p->comp_catchfault = true;
	p->cachesize = 0;

	p->gfx_framerate = 0;

	p->gfx_size.width = 640; //TODO: Default WinUAE prefs indicate this should be 720x568
	p->gfx_size.height = 256;
	p->gfx_resolution = RES_HIRES;
	p->gfx_vresolution = VRES_NONDOUBLE;
	p->gfx_iscanlines = 1;
	p->gfx_apmode[0].gfx_fullscreen = GFX_WINDOW;
	p->gfx_apmode[1].gfx_fullscreen = GFX_WINDOW;
	p->gfx_xcenter = 0; p->gfx_ycenter = 0;
	p->gfx_xcenter_pos = -1;
	p->gfx_ycenter_pos = -1;
	p->gfx_xcenter_size = -1;
	p->gfx_ycenter_size = -1;
	p->gfx_max_horizontal = RES_HIRES;
	p->gfx_max_vertical = VRES_DOUBLE;
	p->gfx_autoresolution_minv = 0;
	p->gfx_autoresolution_minh = 0;
	p->color_mode = 2;
	p->gfx_blackerthanblack = 0;
	p->gfx_autoresolution_vga = true;
	p->gfx_apmode[0].gfx_backbuffers = 2;
	p->gfx_apmode[1].gfx_backbuffers = 1;
	p->gfx_display_sections = 4;
	p->gfx_variable_sync = 0;
	p->gfx_windowed_resize = true;

	p->immediate_blits = false;
	p->waiting_blits = 0;
	p->chipset_refreshrate = 50;
	p->collision_level = 2;
	p->leds_on_screen = 0;
	p->leds_on_screen_mask[0] = p->leds_on_screen_mask[1] = (1 << LED_MAX) - 1;
	p->keyboard_leds_in_use = 0;
	p->keyboard_leds[0] = p->keyboard_leds[1] = p->keyboard_leds[2] = 0;
	p->scsi = 0;
	p->uaeserial = 0;
	p->cpu_idle = 0;
	p->turbo_emulation = 0;
	p->turbo_emulation_limit = 0;
	p->headless = 0;
	p->catweasel = 0;
	p->tod_hack = 0;
	p->maprom = 0;
	p->boot_rom = 0;

	p->cart_internal = 1;
	p->sana2 = 0;
	p->clipboard_sharing = false;
	p->native_code = false;
	p->lightpen_crosshair = true;

	p->cs_compatible = CP_GENERIC;
	p->cs_rtc = 2;
	p->cs_df0idhw = true;
	p->cs_fatgaryrev = -1;
	p->cs_ramseyrev = -1;
	p->cs_agnusrev = -1;
	p->cs_deniserev = -1;
	p->cs_mbdmac = 0;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = p->cs_cd32fmv = false;
	p->cs_cd32nvram_size = 1024;
	p->cs_pcmcia = false;
	p->cs_ksmirror_e0 = true;
	p->cs_ksmirror_a8 = false;
	p->cs_ciaoverlay = true;
	p->cs_ciaatod = 0;
	p->cs_df0idhw = true;
	p->cs_ciatodbug = false;
	p->cs_unmapped_space = 0;
	p->cs_color_burst = false;
	p->cs_ciatype[0] = 0;
	p->cs_ciatype[1] = 0;

	_tcscpy(p->floppyslots[0].df, _T(""));
	_tcscpy(p->floppyslots[1].df, _T(""));
	_tcscpy(p->floppyslots[2].df, _T(""));
	_tcscpy(p->floppyslots[3].df, _T(""));

	configure_rom(p, roms, 0);
	_tcscpy(p->romextfile, _T(""));
	_tcscpy (p->romextfile2, _T(""));
	p->romextfile2addr = 0;
	_tcscpy(p->flashfile, _T(""));
	_tcscpy(p->cartfile, _T(""));
	_tcscpy (p->rtcfile, _T(""));

	sprintf(p->path_rom, _T("%s/kickstarts/"), start_path_data);
	sprintf(p->path_floppy, _T("%s/disks/"), start_path_data);
	sprintf(p->path_hardfile, _T("%s/"), start_path_data);
	sprintf(p->path_cd, _T("%s/cd32/"), start_path_data);

	p->fpu_model = 0;
	p->cpu_model = 68000;
	p->m68k_speed_throttle = 0;
	p->cpu_clock_multiplier = 0;
	p->cpu_frequency = 0;
	p->mmu_model = 0;
	p->cpu060_revision = 6;
	p->fpu_revision = 0;
	p->fpu_no_unimplemented = false;
	p->fpu_strict = false;
	p->m68k_speed = 0;
	p->cpu_compatible = false;
	p->address_space_24 = true;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->ntscmode = false;
	p->filesys_limit = 0;
	p->filesys_max_name = 107;
	p->filesys_max_file_size = 0x7fffffff;

	p->fastmem[0].size = 0x00000000;
	p->mbresmem_low_size = 0x00000000;
	p->mbresmem_high_size = 0x00000000;
	p->z3fastmem[0].size = 0x00000000;
	p->z3autoconfig_start = 0x10000000;
	p->chipmem_size = 0x00080000;
	p->bogomem_size = 0x00080000;
	p->rtgboards[0].rtgmem_size = 0x00000000;
	p->rtgboards[0].rtgmem_type = GFXBOARD_UAE_Z3;
	p->custom_memory_addrs[0] = 0;
	p->custom_memory_sizes[0] = 0;
	p->custom_memory_addrs[1] = 0;
	p->custom_memory_sizes[1] = 0;

	p->nr_floppies = 2;
	p->floppy_read_only = false;
	p->floppyslots[0].dfxtype = DRV_35_DD;
	p->floppyslots[1].dfxtype = DRV_35_DD;
	p->floppyslots[2].dfxtype = DRV_NONE;
	p->floppyslots[3].dfxtype = DRV_NONE;
	p->floppy_speed = 100;
	p->floppy_write_length = 0;
	p->floppy_random_bits_min = 1;
	p->floppy_random_bits_max = 3;
	p->dfxclickvolume_disk[0] = 33;
	p->dfxclickvolume_disk[1] = 33;
	p->dfxclickvolume_disk[2] = 33;
	p->dfxclickvolume_disk[3] = 33;
	p->dfxclickvolume_empty[0] = 33;
	p->dfxclickvolume_empty[1] = 33;
	p->dfxclickvolume_empty[2] = 33;
	p->dfxclickvolume_empty[3] = 33;
	p->dfxclickchannelmask = 0xffff;
	p->cd_speed = 100;

	p->socket_emu = false;

	p->input_tablet = TABLET_OFF;
	p->tablet_library = false;
	p->input_mouse_untrap = MOUSEUNTRAP_MIDDLEBUTTON;
	p->input_magic_mouse_cursor = 0;

	inputdevice_default_prefs(p);

	blkdev_default_prefs(p);

	p->cr_selected = -1;
	struct chipset_refresh* cr;
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++)
	{
		cr = &p->cr[i];
		cr->index = i;
		cr->rate = -1;
	}
	cr = &p->cr[CHIPSET_REFRESH_PAL];
	cr->index = CHIPSET_REFRESH_PAL;
	cr->horiz = -1;
	cr->vert = -1;
	cr->lace = -1;
	cr->vsync = -1;
	cr->framelength = -1;
	cr->rate = 50.0;
	cr->ntsc = 0;
	cr->locked = false;
	cr->inuse = true;
	_tcscpy(cr->label, _T("PAL"));
	cr = &p->cr[CHIPSET_REFRESH_NTSC];
	cr->index = CHIPSET_REFRESH_NTSC;
	cr->horiz = -1;
	cr->vert = -1;
	cr->lace = -1;
	cr->vsync = -1;
	cr->framelength = -1;
	cr->rate = 60.0;
	cr->ntsc = 1;
	cr->locked = false;
	cr->inuse = true;
	_tcscpy(cr->label, _T("NTSC"));

	p->lightboost_strobo = false;
	p->lightboost_strobo_ratio = 50;

	savestate_state = 0;

	target_default_options(p, type);

	zfile_fclose(default_file);
	default_file = nullptr;
	struct zfile* f = zfile_fopen_empty(nullptr, _T("configstore"));
	if (f)
	{
		uaeconfig++;
		cfgfile_save_options(f, p, 0);
		uaeconfig--;
		cfg_write(&zero, f);
		default_file = f;
	}
}

static void buildin_default_prefs_68020(struct uae_prefs* p)
{
	p->cpu_model = 68020;
	p->address_space_24 = true;
	p->cpu_compatible = false;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->chipmem_size = 0x200000;
	p->bogomem_size = 0;
	p->m68k_speed = -1;
}

static void buildin_default_prefs(struct uae_prefs* p)
{
	p->floppyslots[0].dfxtype = DRV_35_DD;
	if (p->nr_floppies != 1 && p->nr_floppies != 2)
		p->nr_floppies = 2;
	p->floppyslots[1].dfxtype = p->nr_floppies >= 2 ? DRV_35_DD : DRV_NONE;
	p->floppyslots[2].dfxtype = DRV_NONE;
	p->floppyslots[3].dfxtype = DRV_NONE;
	p->floppy_speed = 100;

	p->fpu_model = 0;
	p->cpu_model = 68000;
	p->cpu_clock_multiplier = 0;
	p->cpu_frequency = 0;
	p->cpu060_revision = 1;
	p->fpu_revision = -1;
	p->m68k_speed = 0;
	p->cpu_compatible = true;
	p->address_space_24 = true;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->immediate_blits = false;
	p->waiting_blits = 0;
	p->collision_level = 2;
	if (p->produce_sound < 1)
		p->produce_sound = 1;
	p->scsi = 0;
	p->uaeserial = 0;
	p->cpu_idle = 0;
	p->turbo_emulation = 0;
	p->turbo_emulation_limit = 0;
	p->catweasel = 0;
	p->tod_hack = 0;
	p->maprom = 0;
	p->cachesize = 0;
	p->socket_emu = false;
	p->sound_volume_cd = 0;

	p->chipmem_size = 0x00080000;
	p->bogomem_size = 0x00080000;
	for (int i = 0; i < MAX_RAM_BOARDS; i++)
	{
		memset(p->fastmem, 0, sizeof(struct ramboard));
		memset(p->z3fastmem, 0, sizeof(struct ramboard));
	}
	p->mbresmem_low_size = 0x00000000;
	p->mbresmem_high_size = 0x00000000;
	for (int i = 0; i < MAX_RTG_BOARDS; i++)
	{
		p->rtgboards[i].rtgmem_size = 0x00000000;
		p->rtgboards[i].rtgmem_type = GFXBOARD_UAE_Z3;
	}
	for (int i = 0; i < MAX_EXPANSION_BOARDS; i++)
	{
		memset(&p->expansionboard[i], 0, sizeof(struct boardromconfig));
	}

	p->cs_rtc = 0;
	p->cs_a1000ram = false;
	p->cs_fatgaryrev = -1;
	p->cs_ramseyrev = -1;
	p->cs_agnusrev = -1;
	p->cs_deniserev = -1;
	p->cs_mbdmac = 0;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = p->cs_cd32fmv = false;
	p->cs_cdtvcd = p->cs_cdtvram = false;
	p->cs_ide = 0;
	p->cs_pcmcia = false;
	p->cs_ksmirror_e0 = true;
	p->cs_ksmirror_a8 = false;
	p->cs_ciaoverlay = true;
	p->cs_ciaatod = 0;
	p->cs_df0idhw = true;
	p->cs_ciatodbug = false;
	p->cs_1mchipjumper = false;

	_tcscpy(p->romextfile, _T(""));
	_tcscpy (p->romextfile2, _T(""));

	p->ne2000pciname[0] = 0;
	p->ne2000pcmcianame[0] = 0;
	p->a2065name[0] = 0;

	p->prtname[0] = 0;
	p->sername[0] = 0;

	p->mountitems = 0;

	target_default_options(p, 1);
	cfgfile_compatibility_romtype(p);
}

static void set_68020_compa(struct uae_prefs* p, int compa, int cd32)
{
	switch (compa)
	{
	case 0:
		p->cpu_compatible = false;
		p->m68k_speed = 0;
		p->cachesize = 0;
		p->compfpu = false;
		break;
	case 1:
		p->cpu_compatible = false;
		p->m68k_speed = 0;
		p->cachesize = 0;
		p->compfpu = false;
		break;
	case 2:
		p->cpu_compatible = false;
		p->m68k_speed = -1;
		p->cachesize = 0;
		p->compfpu = false;
		p->address_space_24 = false;
		break;
	case 3:
		p->cpu_compatible = false;
		p->address_space_24 = false;
		p->cachesize = MAX_JIT_CACHE;
		p->compfpu = true;
		break;
	}
	if (p->cpu_model >= 68030)
		p->address_space_24 = false;
}

/* 0: cycle-exact
* 1: more compatible
* 2: no more compatible, no 100% sound
* 3: no more compatible, waiting blits, no 100% sound
*/

static void set_68000_compa(struct uae_prefs* p, int compa)
{
	p->cpu_clock_multiplier = 2 << 8;
	switch (compa)
	{
	case 0:
		p->cpu_cycle_exact = p->cpu_memory_cycle_exact = p->blitter_cycle_exact = 1;
		break;
	case 1:
		break;
	case 2:
		p->cpu_compatible = false;
		break;
	case 3:
		p->produce_sound = 2;
		p->cpu_compatible = false;
		break;
	}
}

static int bip_a3000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	if (config == 2)
		roms[0] = 61;
	else if (config == 1)
		roms[0] = 71;
	else
		roms[0] = 59;
	roms[1] = -1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->fpu_no_unimplemented = true;
	if (compa == 0)
		p->mmu_model = 68030;
	else
		p->cachesize = MAX_JIT_CACHE;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->produce_sound = 2;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A3000;
	p->mbresmem_low_size = 8 * 1024 * 1024;
	built_in_chipset_prefs (p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom (p, roms, romcheck);
}
static int bip_a4000(struct uae_prefs* p, int config, int compa, int romcheck)
{
	int roms[8];

	roms[0] = 16;
	roms[1] = 31;
	roms[2] = 13;
	roms[3] = 12;
	roms[4] = -1;

	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->mbresmem_low_size = 8 * 1024 * 1024;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	switch (config)
	{
	case 1:
		p->cpu_model = 68040;
		p->fpu_model = 68040;
		break;
	}
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = false;
	p->produce_sound = 2;
	p->cachesize = MAX_JIT_CACHE;
	p->compfpu = true;

	p->nr_floppies = 2;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A4000;
	built_in_chipset_prefs(p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom(p, roms, romcheck);
}
static int bip_a4000t (struct uae_prefs *p, int config, int compa, int romcheck)
{

	int roms[8];

	roms[0] = 16;
	roms[1] = 31;
	roms[2] = 13;
	roms[3] = -1;

	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	p->mbresmem_low_size = 8 * 1024 * 1024;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	if (config > 0) {
		p->cpu_model = 68040;
		p->fpu_model = 68040;
	}
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->produce_sound = 2;
	p->cachesize = MAX_JIT_CACHE;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A4000T;
	built_in_chipset_prefs (p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom (p, roms, romcheck);
}

static void bip_velvet(struct uae_prefs *p, int config, int compa, int romcheck)
{
	p->chipset_mask = 0;
	p->bogomem_size = 0;
	p->sound_filter = FILTER_SOUND_ON;
	set_68000_compa (p, compa);
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_compatible = CP_VELVET;
	p->cs_slowmemisfast = 1;
	p->cs_dipagnus = 1;
	p->cs_agnusbltbusybug = 1;
	built_in_chipset_prefs (p);
	p->cs_denisenoehb = 1;
	p->cs_cia6526 = 1;
	p->chipmem_size = 0x40000;
}

static int bip_a1000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	roms[0] = 24;
	roms[1] = -1;
	p->chipset_mask = 0;
	p->bogomem_size = 0;
	p->sound_filter = FILTER_SOUND_ON;
	set_68000_compa (p, compa);
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_compatible = CP_A1000;
	p->cs_slowmemisfast = 1;
	p->cs_dipagnus = 1;
	p->cs_agnusbltbusybug = 1;
	built_in_chipset_prefs (p);
	if (config > 0)
		p->cs_denisenoehb = 1;
	if (config > 1)
		p->chipmem_size = 0x40000;
	if (config > 2) {
		roms[0] = 125;
		roms[1] = -1;
		bip_velvet(p, config, compa, romcheck);
	}
	return configure_rom (p, roms, romcheck);
}

static int bip_cdtvcr (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cs_cdtvcd = p->cs_cdtvram = true;
	p->cs_cdtvcr = true;
	p->cs_rtc = 1;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	if (config > 0)
		p->floppyslots[0].dfxtype = DRV_35_DD;
	p->floppyslots[1].dfxtype = DRV_NONE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_CDTVCR;
	built_in_chipset_prefs (p);
	fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat (p->flashfile, _T("cdtv-cr.nvr"));
	roms[0] = 9;
	roms[1] = 10;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	roms[0] = 108;
	roms[1] = 107;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	return 1;
}

static int bip_cdtv (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	if (config >= 2)
		return bip_cdtvcr(p, config - 2, compa, romcheck);

	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->cs_cdtvcd = p->cs_cdtvram = 1;
	if (config > 0) {
		addbcromtype(p, ROMTYPE_CDTVSRAM, true, NULL, 0);
	}
	p->cs_rtc = 1;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	if (config > 0)
		p->floppyslots[0].dfxtype = DRV_35_DD;
	p->floppyslots[1].dfxtype = DRV_NONE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_CDTV;
	built_in_chipset_prefs (p);
	fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat (p->flashfile, _T("cdtv.nvr"));
	roms[0] = 6;
	roms[1] = 32;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	roms[0] = 20;
	roms[1] = 21;
	roms[2] = 22;
	roms[3] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	return 1;
}

static int bip_cd32(struct uae_prefs* p, int config, int compa, int romcheck)
{
	int roms[3];

	buildin_default_prefs_68020(p);
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = true;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_unmapped_space = 1;
	set_68020_compa(p, compa, 1);
	p->cs_compatible = CP_CD32;
	built_in_chipset_prefs(p);
	fetch_datapath(p->flashfile, sizeof(p->flashfile) / sizeof(TCHAR));
	_tcscat(p->flashfile, _T("cd32.nvr"));

	roms[0] = 64;
	roms[1] = -1;
	if (!configure_rom(p, roms, 0))
	{
		roms[0] = 18;
		roms[1] = -1;
		if (!configure_rom(p, roms, romcheck))
			return 0;
		roms[0] = 19;
		if (!configure_rom(p, roms, romcheck))
			return 0;
	}
	if (config == 1) {
		p->cs_cd32fmv = true;
		roms[0] = 74;
		roms[1] = 23;
		roms[2] = -1;
		if (!configure_rom(p, roms, romcheck))
			return 0;
	}

	p->cdslots[0].inuse = true;
	p->cdslots[0].type = SCSI_UNIT_IMAGE;
	p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;

	return 1;
}

static int bip_a1200(struct uae_prefs* p, int config, int compa, int romcheck)
{
	int roms[4];

	buildin_default_prefs_68020(p);
	roms[0] = 11;
	roms[1] = 15;
	roms[2] = 31;
	roms[3] = -1;

	p->cs_rtc = 0;
	p->cs_compatible = CP_A1200;
	built_in_chipset_prefs(p);
	switch (config)
	{
	case 1:
		p->fastmem[0].size = 0x400000;
		p->cs_rtc = 1;
		break;
	}
	set_68020_compa(p, compa, 0);

	p->m68k_speed = M68K_SPEED_14MHZ_CYCLES;

	p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;

	return configure_rom(p, roms, romcheck);
}

static int bip_a600(struct uae_prefs* p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = 10;
	roms[1] = 9;
	roms[2] = 8;
	roms[3] = -1;
	set_68000_compa(p, compa);
	p->cs_compatible = CP_A600;
	built_in_chipset_prefs(p);
	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	if (config > 0)
		p->cs_rtc = 1;
	if (config == 1)
		p->chipmem_size = 0x200000;
	if (config == 2)
		p->fastmem[0].size = 0x400000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	return configure_rom(p, roms, romcheck);
}

static int bip_a500p(struct uae_prefs* p, int config, int compa, int romcheck)
{
	int roms[2];

	roms[0] = 7;
	roms[1] = -1;
	set_68000_compa(p, compa);
	p->cs_compatible = CP_A500P;
	built_in_chipset_prefs(p);
	p->bogomem_size = 0;
	p->chipmem_size = 0x100000;
	if (config > 0)
		p->cs_rtc = 1;
	if (config == 1)
		p->chipmem_size = 0x200000;
	if (config == 2)
		p->fastmem[0].size = 0x400000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	return configure_rom(p, roms, romcheck);
}

static int bip_a500(struct uae_prefs* p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = roms[1] = roms[2] = roms[3] = -1;
	switch (config)
	{
	case 0: // KS 1.3, OCS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 6;
		roms[1] = 32;
		p->chipset_mask = 0;
		break;
	case 1: // KS 1.3, ECS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 6;
		roms[1] = 32;
		break;
	case 2: // KS 1.3, ECS Agnus, 1.0M Chip
		roms[0] = 6;
		roms[1] = 32;
		p->bogomem_size = 0;
		p->chipmem_size = 0x100000;
		break;
	case 3: // KS 1.3, OCS Agnus, 0.5M Chip
		roms[0] = 6;
		roms[1] = 32;
		p->bogomem_size = 0;
		p->chipset_mask = 0;
		p->cs_rtc = 0;
		p->floppyslots[1].dfxtype = DRV_NONE;
		break;
	case 4: // KS 1.2, OCS Agnus, 0.5M Chip
		roms[0] = 5;
		roms[1] = 4;
		roms[2] = 3;
		p->bogomem_size = 0;
		p->chipset_mask = 0;
		p->cs_rtc = 0;
		p->floppyslots[1].dfxtype = DRV_NONE;
		break;
	case 5: // KS 1.2, OCS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 5;
		roms[1] = 4;
		roms[2] = 3;
		p->chipset_mask = 0;
		break;
	}
	p->fast_copper = 0;
	set_68000_compa(p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs(p);
	return configure_rom(p, roms, romcheck);
}

static int bip_super (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[7];

	roms[0] = 16;
	roms[1] = 31;
	roms[2] = 15;
	roms[3] = 14;
	roms[4] = 12;
	roms[5] = 11;
	roms[6] = -1;
	p->bogomem_size = 0;
	p->chipmem_size = 0x400000;
	p->z3fastmem[0].size = 8 * 1024 * 1024;
	p->rtgboards[0].rtgmem_size = 16 * 1024 * 1024;
	p->cpu_model = 68040;
	p->fpu_model = 68040;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 1;
	p->produce_sound = 2;
	p->cachesize = MAX_JIT_CACHE;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->scsi = 1;
	p->uaeserial = 1;
	p->socket_emu = 1;
	p->cart_internal = 0;
	p->picasso96_nocustom = 1;
	p->cs_compatible = 1;
	p->cs_unmapped_space = 1;
	built_in_chipset_prefs (p);
	p->cs_ide = -1;
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	//_tcscat(p->flashfile, _T("battclock.nvr"));
	return configure_rom (p, roms, romcheck);
}

static int bip_arcadia (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4], i;
	struct romlist **rl;

	p->bogomem_size = 0;
	p->chipset_mask = 0;
	p->cs_rtc = 0;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs (p);
	fetch_datapath (p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat (p->flashfile, _T("arcadia.nvr"));
	roms[0] = 5;
	roms[1] = 4;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	roms[0] = 51;
	roms[1] = 49;
	roms[2] = -1;
	if (!configure_rom (p, roms, romcheck))
		return 0;
	rl = getarcadiaroms ();
	for (i = 0; rl[i]; i++) {
		if (config-- == 0) {
			roms[0] = rl[i]->rd->id;
			roms[1] = -1;
			configure_rom (p, roms, 0);
			break;
		}
	}
	xfree (rl);
	return 1;
}

static int bip_casablanca(struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[8];

	roms[0] = 231;
	roms[1] = -1;

	p->bogomem_size = 0;
	p->chipmem_size = 0x200000;
	switch (config)
	{
	default:
	case 1:
		p->cpu_model = 68040;
		p->fpu_model = 68040;
		p->mmu_model = 68040;
		break;
	case 2:
		p->cpu_model = 68060;
		p->fpu_model = 68060;
		p->mmu_model = 68040;
		break;
	}
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = 0;
	p->m68k_speed = -1;
	p->immediate_blits = 0;
	p->produce_sound = 2;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_compatible = CP_CASABLANCA;
	built_in_chipset_prefs(p);
	return configure_rom(p, roms, romcheck);
}

int built_in_prefs(struct uae_prefs* p, int model, int config, int compa, int romcheck)
{
	int v = 0;

	buildin_default_prefs(p);
	switch (model)
	{
	case 0:
		v = bip_a500(p, config, compa, romcheck);
		break;
	case 1:
		v = bip_a500p(p, config, compa, romcheck);
		break;
	case 2:
		v = bip_a600(p, config, compa, romcheck);
		break;
	case 3:
		v = bip_a1000 (p, config, compa, romcheck);
		break;
	case 4:
		v = bip_a1200(p, config, compa, romcheck);
		break;
	case 5:
		v = bip_a3000 (p, config, compa, romcheck);
		break;
	case 6:
		v = bip_a4000(p, config, compa, romcheck);
		break;
	case 7:
		v = bip_a4000t (p, config, compa, romcheck);
		break;
	case 8:
		v = bip_cd32(p, config, compa, romcheck);
		break;
	case 9:
		v = bip_cdtv (p, config, compa, romcheck);
		break;
	case 10:
		v = bip_arcadia(p, config, compa, romcheck);
		break;
	case 11:
		v = bip_casablanca(p, config, compa, romcheck);
		break;
	case 12:
		v = bip_super (p, config, compa, romcheck);
		break;
	}
	if (!p->immediate_blits)
		p->waiting_blits = 1;
	if (p->sound_filter_type == FILTER_SOUND_TYPE_A500 && (p->chipset_mask & CSMASK_AGA))
		p->sound_filter_type = FILTER_SOUND_TYPE_A1200;
	else if (p->sound_filter_type == FILTER_SOUND_TYPE_A1200 && !(p->chipset_mask & CSMASK_AGA))
		p->sound_filter_type = FILTER_SOUND_TYPE_A500;
	if (p->cpu_model >= 68040)
		p->cs_bytecustomwritebug = true;
	cfgfile_compatibility_romtype(p);
	return v;
}

int built_in_chipset_prefs(struct uae_prefs* p)
{
	if (!p->cs_compatible)
		return 1;

	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = false;
	p->cs_fatgaryrev = -1;
	p->cs_ide = 0;
	p->cs_ramseyrev = -1;
	p->cs_pcmcia = false;
	p->cs_ksmirror_e0 = true;
	p->cs_ksmirror_a8 = false;
	p->cs_ciaoverlay = true;
	p->cs_ciaatod = 0;
	p->cs_rtc = 0;
	p->cs_df0idhw = true;
	p->cs_ciatodbug = false;
	p->cs_z3autoconfig = false;
	p->cs_bytecustomwritebug = false;
	p->cs_1mchipjumper = false;
	p->cs_unmapped_space = 0;
	p->cs_color_burst = false;
	p->cs_romisslow = false;
	p->cs_toshibagary = false;
	p->cs_ciatype[0] = p->cs_ciatype[1] = 0;

	switch (p->cs_compatible)
	{
	case CP_GENERIC: // generic
		if (p->cpu_model >= 68020)
		{
			// big box-like
			p->cs_rtc = 2;
			p->cs_fatgaryrev = 0;
			p->cs_ide = -1;
			p->cs_ramseyrev = 0x0f;
			p->cs_unmapped_space = 1;
		}
		else if (p->cpu_compatible)
		{
			// very A500-like
			p->cs_df0idhw = 0;
			p->cs_resetwarning = 0;
			if (p->bogomem_size || p->chipmem_size > 0x80000 || p->fastmem[0].size)
				p->cs_rtc = 1;
			p->cs_ciatodbug = true;
		}
		else
		{
			// sort of A500-like
			p->cs_ide = -1;
			p->cs_rtc = 1;
		}
		break;
	case CP_CDTV: // CDTV
		p->cs_rtc = 1;
		p->cs_cdtvcd = p->cs_cdtvram = 1;
		p->cs_df0idhw = 1;
		p->cs_ksmirror_e0 = 0;
		break;
	case CP_CDTVCR: // CDTV-CR
		p->cs_rtc = 1;
		p->cs_cdtvcd = p->cs_cdtvram = 1;
		p->cs_cdtvcr = true;
		p->cs_df0idhw = 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = 1;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		p->cs_resetwarning = 0;
		p->cs_ciatodbug = true;
		break;
	case CP_CD32: // CD32
		p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = true;
		p->cs_ksmirror_e0 = 0;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		p->cs_resetwarning = 0;
		p->cs_unmapped_space = 1;
		break;
	case CP_A500: // A500
		p->cs_df0idhw = 0;
		p->cs_resetwarning = 0;
		if (p->bogomem_size || p->chipmem_size > 0x80000 || p->fastmem[0].size)
			p->cs_rtc = 1;
		p->cs_ciatodbug = true;
		break;
	case CP_A500P: // A500+
		p->cs_rtc = 1;
		p->cs_resetwarning = 0;
		p->cs_ciatodbug = true;
		break;
	case CP_A600: // A600
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = 1;
		p->cs_ksmirror_a8 = 1;
		p->cs_ciaoverlay = 0;
		p->cs_resetwarning = 0;
		p->cs_ciatodbug = true;
		p->cs_ciatype[0] = p->cs_ciatype[1] = 1;
		break;
	case CP_A1000: // A1000
		p->cs_a1000ram = 1;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_agnusbltbusybug = 1;
		p->cs_dipagnus = 1;
		p->cs_ciatodbug = true;
		break;
	case CP_VELVET: // A1000 Prototype
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_agnusbltbusybug = 1;
		p->cs_dipagnus = 1;
		p->cs_denisenoehb = 1;
		break;
	case CP_A1200: // A1200
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = true;
		p->cs_ksmirror_a8 = true;
		p->cs_ciaoverlay = false;
		if (p->fastmem[0].size || p->z3fastmem[0].size)
			p->cs_rtc = 1;
		break;
	case CP_A2000: // A2000
		p->cs_rtc = 1;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_ciatodbug = true;
		p->cs_unmapped_space = 1;
		break;
	case CP_A3000: // A3000
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0d;
		p->cs_mbdmac = 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		break;
	case CP_A3000T: // A3000T
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0d;
		p->cs_mbdmac = 1;
		p->cs_ksmirror_e0 = 0;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		break;
	case CP_A4000: // A4000
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0f;
		p->cs_ide = IDE_A4000;
		p->cs_mbdmac = 0;
		p->cs_ksmirror_a8 = 0;
		p->cs_ksmirror_e0 = 0;
		p->cs_ciaoverlay = 0;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		break;
	case CP_A4000T: // A4000T
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0f;
		p->cs_ide = IDE_A4000;
		p->cs_mbdmac = 2;
		p->cs_ksmirror_a8 = 0;
		p->cs_ksmirror_e0 = 0;
		p->cs_ciaoverlay = 0;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		break;
	case CP_CASABLANCA:
		break;
	case CP_DRACO:
		break;
	}
	if (p->cpu_model >= 68040)
		p->cs_bytecustomwritebug = true;
	return 1;
}

void set_config_changed(void)
{
	config_changed = 1;
}

void config_check_vsync(void)
{
	if (config_changed)
	{
		config_changed++;
		if (config_changed >= 3)
			config_changed = 0;
	}
}

bool is_error_log(void)
{
	return error_lines != nullptr;
}

TCHAR* get_error_log(void)
{
	strlist* sl;
	int len = 0;
	for (sl = error_lines; sl; sl = sl->next)
	{
		len += _tcslen(sl->option) + 1;
	}
	if (!len)
		return nullptr;
	TCHAR* s = xcalloc(TCHAR, len + 1);
	for (sl = error_lines; sl; sl = sl->next)
	{
		_tcscat(s, sl->option);
		_tcscat(s, _T("\n"));
	}
	return s;
}

void error_log(const TCHAR* format, ...)
{
	TCHAR buffer[256], *bufp;
	int bufsize = 256;
	va_list parms;

	if (format == nullptr)
	{
		struct strlist** ps = &error_lines;
		while (*ps)
		{
			struct strlist* s = *ps;
			*ps = s->next;
			xfree(s->value);
			xfree(s->option);
			xfree(s);
		}
		return;
	}

	va_start(parms, format);
	bufp = buffer;
	for (;;)
	{
		int count = _vsntprintf(bufp, bufsize - 1, format, parms);
		if (count < 0)
		{
			bufsize *= 10;
			if (bufp != buffer)
				xfree(bufp);
			bufp = xmalloc(TCHAR, bufsize);
			continue;
		}
		break;
	}
	bufp[bufsize - 1] = 0;
	write_log(_T("%s\n"), bufp);
	va_end(parms);

	strlist* u = xcalloc(struct strlist, 1);
	u->option = my_strdup(bufp);
	u->next = error_lines;
	error_lines = u;

	if (bufp != buffer)
		xfree(bufp);
}

int bip_a4000(struct uae_prefs* p, int rom)
{
	return bip_a4000(p, 0, 0, 0);
}

int bip_cd32(struct uae_prefs* p, int rom)
{
	return bip_cd32(p, 0, 0, 0);
}

int bip_a1200(struct uae_prefs* p, int rom)
{
	int roms[4];

	int v = bip_a1200(p, 0, 0, 0);
	if (rom == 310)
	{
		roms[0] = 15;
		roms[1] = 11;
		roms[2] = 31;
		roms[3] = -1;
		v = configure_rom(p, roms, 0);
	}

	return v;
}

int bip_a500plus(struct uae_prefs* p, int rom)
{
	int roms[4];

	int v = bip_a500p(p, 0, 0, 0);
	if (rom == 130)
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
	return configure_rom(p, roms, 0);
}

int bip_a500(struct uae_prefs* p, int rom)
{
	int roms[4];

	int v = bip_a500(p, 0, 0, 0);
	if (rom == 130)
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
	return configure_rom(p, roms, 0);
}

int bip_a2000(struct uae_prefs* p, int rom)
{
	int roms[4];

	if (rom == 130)
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
	p->cs_compatible = CP_A2000;
	built_in_chipset_prefs(p);
	p->chipmem_size = 0x00080000;
	p->bogomem_size = 0x00080000;
	p->chipset_mask = 0;
	p->cpu_compatible = false;
	p->fast_copper = 0;
	p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
	return configure_rom(p, roms, 0);
}
