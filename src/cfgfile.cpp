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

#include <cctype>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "events.h"
#include "custom.h"
#include "inputdevice.h"
#include "gfxfilter.h"
#include "savestate.h"
#include "memory.h"
#include "autoconf.h"
#include "rommgr.h"
#include "gui.h"
#include "newcpu.h"
#include "zfile.h"
#include "filesys.h"
#include "fsdb.h"
#include "disk.h"
#include "blkdev.h"
#include "statusline.h"
#include "debug.h"
#include "calc.h"
#include "gfxboard.h"
#include "cpuboard.h"
#ifdef WITH_LUA
#include "luascript.h"
#endif
#include "ethernet.h"
#include "native2amiga_api.h"
#include "ini.h"
#ifdef WITH_SPECIALMONITORS
#include "specialmonitors.h"
#endif

#ifdef AMIBERRY
#include "amiberry_input.h"
#include <arpa/inet.h>
#endif

#define cfgfile_warning write_log
#define cfgfile_warning_obsolete write_log

static int config_newfilesystem;
static struct strlist *temp_lines;
static struct strlist *error_lines;
static struct zfile *default_file, *configstore;
static int uaeconfig;
static int unicode_config = 0;

/* @@@ need to get rid of this... just cut part of the manual and print that
* as a help text.  */
struct cfg_lines
{
	const TCHAR *config_label, *config_help;
};

static const struct cfg_lines opttable[] =
{
	{_T("help"), _T("Prints this help") },
	{_T("config_description"), _T("") },
	{_T("config_info"), _T("") },
	{_T("use_gui"), _T("Enable the GUI?  If no, then goes straight to emulator") },
	{_T("use_debugger"), _T("Enable the debugger?") },
	{_T("cpu_speed"), _T("can be max, real, or a number between 1 and 20") },
	{_T("cpu_model"), _T("Can be 68000, 68010, 68020, 68030, 68040, 68060") },
	{_T("fpu_model"), _T("Can be 68881, 68882, 68040, 68060") },
	{_T("cpu_compatible"), _T("yes enables compatibility-mode") },
	{_T("cpu_24bit_addressing"), _T("must be set to 'no' in order for Z3mem or P96mem to work") },
	{_T("autoconfig"), _T("yes = add filesystems and extra ram") },
	{_T("log_illegal_mem"), _T("print illegal memory access by Amiga software?") },
	{_T("fastmem_size"), _T("Size in megabytes of fast-memory") },
	{_T("chipmem_size"), _T("Size in megabytes of chip-memory") },
	{_T("bogomem_size"), _T("Size in megabytes of bogo-memory at 0xC00000") },
	{_T("a3000mem_size"), _T("Size in megabytes of A3000 memory") },
	{_T("gfxcard_size"), _T("Size in megabytes of Picasso96 graphics-card memory") },
	{_T("z3mem_size"), _T("Size in megabytes of Zorro-III expansion memory") },
	{_T("gfx_test_speed"), _T("Test graphics speed?") },
	{_T("gfx_framerate"), _T("Print every nth frame") },
	{_T("gfx_width"), _T("Screen width") },
	{_T("gfx_height"), _T("Screen height") },
	{_T("gfx_refreshrate"), _T("Fullscreen refresh rate") },
	{_T("gfx_vsync"), _T("Sync screen refresh to refresh rate") },
	{_T("gfx_lores"), _T("Treat display as lo-res?") },
	{_T("gfx_linemode"), _T("Can be none, double, or scanlines") },
	{_T("gfx_fullscreen_amiga"), _T("Amiga screens are fullscreen?") },
	{_T("gfx_fullscreen_picasso"), _T("Picasso screens are fullscreen?") },
	{_T("gfx_center_horizontal"), _T("Center display horizontally?") },
	{_T("gfx_center_vertical"), _T("Center display vertically?") },
	{_T("gfx_colour_mode"), _T("") },
	{_T("32bit_blits"), _T("Enable 32 bit blitter emulation") },
	{_T("immediate_blits"), _T("Perform blits immediately") },
	{_T("show_leds"), _T("LED display") },
	{_T("keyboard_leds"), _T("Keyboard LEDs") },
	{_T("gfxlib_replacement"), _T("Use graphics.library replacement?") },
	{_T("sound_output"), _T("") },
	{_T("sound_frequency"), _T("") },
	{_T("sound_bits"), _T("") },
	{_T("sound_channels"), _T("") },
	{_T("sound_max_buff"), _T("") },
	{_T("comp_trustbyte"), _T("How to access bytes in compiler (direct/indirect/indirectKS/afterPic") },
	{_T("comp_trustword"), _T("How to access words in compiler (direct/indirect/indirectKS/afterPic") },
	{_T("comp_trustlong"), _T("How to access longs in compiler (direct/indirect/indirectKS/afterPic") },
	{_T("comp_nf"), _T("Whether to optimize away flag generation where possible") },
	{_T("comp_fpu"), _T("Whether to provide JIT FPU emulation") },
	{_T("cachesize"), _T("How many MB to use to buffer translated instructions")},
	{_T("override_dga_address"),_T("Address from which to map the frame buffer (upper 16 bits) (DANGEROUS!)")},
	{_T("avoid_dga"), _T("Set to yes if the use of DGA extension creates problems") },
	{_T("avoid_vid"), _T("Set to yes if the use of the Vidmode extension creates problems") },
	{_T("parallel_on_demand"), _T("") },
	{_T("serial_on_demand"), _T("") },
	{_T("scsi"), _T("scsi.device emulation") },
	{_T("joyport0"), _T("") },
	{_T("joyport1"), _T("") },
	{_T("pci_devices"), _T("List of PCI devices to make visible to the emulated Amiga") },
	{_T("kickstart_rom_file"), _T("Kickstart ROM image, (C) Copyright Amiga, Inc.") },
	{_T("kickstart_ext_rom_file"), _T("Extended Kickstart ROM image, (C) Copyright Amiga, Inc.") },
	{_T("kickstart_key_file"), _T("Key-file for encrypted ROM images (from Cloanto's Amiga Forever)") },
	{_T("flash_ram_file"), _T("Flash/battery backed RAM image file.") },
	{_T("cart_file"), _T("Freezer cartridge ROM image file.") },
	{_T("floppy0"), _T("Diskfile for drive 0") },
	{_T("floppy1"), _T("Diskfile for drive 1") },
	{_T("floppy2"), _T("Diskfile for drive 2") },
	{_T("floppy3"), _T("Diskfile for drive 3") },
	{_T("hardfile"), _T("access,sectors, surfaces, reserved, blocksize, path format") },
	{_T("filesystem"), _T("access,'Amiga volume-name':'host directory path' - where 'access' can be 'read-only' or 'read-write'") },
	{_T("catweasel"), _T("Catweasel board io base address") }
};

static const TCHAR *guimode1[] = { _T("no"), _T("yes"), _T("nowait"), nullptr };
static const TCHAR *guimode2[] = { _T("false"), _T("true"), _T("nowait"), nullptr };
static const TCHAR *guimode3[] = { _T("0"), _T("1"), _T("nowait"), nullptr };
static const TCHAR *csmode[] = { _T("ocs"), _T("ecs_agnus"), _T("ecs_denise"), _T("ecs"), _T("aga"), nullptr };
static const TCHAR *linemode[] = {
	_T("none"),
	_T("double"), _T("scanlines"), _T("scanlines2p"), _T("scanlines3p"),
	_T("double2"), _T("scanlines2"), _T("scanlines2p2"), _T("scanlines2p3"),
	_T("double3"), _T("scanlines3"), _T("scanlines3p2"), _T("scanlines3p3"),
	nullptr };
static const TCHAR *speedmode[] = { _T("max"), _T("real"), nullptr };
static const TCHAR *colormode1[] = { _T("8bit"), _T("15bit"), _T("16bit"), _T("8bit_dither"), _T("4bit_dither"), _T("32bit"), nullptr };
static const TCHAR *colormode2[] = { _T("8"), _T("15"), _T("16"), _T("8d"), _T("4d"), _T("32"), nullptr };
static const TCHAR *soundmode1[] = { _T("none"), _T("interrupts"), _T("normal"), _T("exact"), nullptr };
static const TCHAR *soundmode2[] = { _T("none"), _T("interrupts"), _T("good"), _T("best"), nullptr };
static const TCHAR *centermode1[] = { _T("none"), _T("simple"), _T("smart"), nullptr };
static const TCHAR *centermode2[] = { _T("false"), _T("true"), _T("smart"), nullptr };
static const TCHAR *stereomode[] = { _T("mono"), _T("stereo"), _T("clonedstereo"), _T("4ch"), _T("clonedstereo6ch"), _T("6ch"),  _T("clonedstereo8ch"), _T("8ch"), _T("mixed"), nullptr };
static const TCHAR *interpolmode[] = { _T("none"), _T("anti"), _T("sinc"), _T("rh"), _T("crux"), nullptr };
static const TCHAR *collmode[] = { _T("none"), _T("sprites"), _T("playfields"), _T("full"), nullptr };
static const TCHAR *compmode[] = { _T("direct"), _T("indirect"), _T("indirectKS"), _T("afterPic"), nullptr };
static const TCHAR *flushmode[] = { _T("soft"), _T("hard"), nullptr };
static const TCHAR *kbleds[] = { _T("none"), _T("POWER"), _T("DF0"), _T("DF1"), _T("DF2"), _T("DF3"), _T("HD"), _T("CD"), _T("DFx"), nullptr };
static const TCHAR *onscreenleds[] = { _T("false"), _T("true"), _T("rtg"), _T("both"), nullptr };
static const TCHAR *soundfiltermode1[] = { _T("off"), _T("emulated"), _T("on"), _T("fixedonly"), nullptr };
static const TCHAR *soundfiltermode2[] = { _T("standard"), _T("enhanced"), nullptr };
static const TCHAR *lorestype1[] = { _T("lores"), _T("hires"), _T("superhires"), nullptr };
static const TCHAR *lorestype2[] = { _T("true"), _T("false"), nullptr };
static const TCHAR *loresmode[] = { _T("normal"), _T("filtered"), nullptr };
static const TCHAR *horizmode[] = { _T("vertical"), _T("lores"), _T("hires"), _T("superhires"), nullptr };
static const TCHAR *vertmode[] = { _T("horizontal"), _T("single"), _T("double"), _T("quadruple"), nullptr };
#ifdef GFXFILTER
static const TCHAR *filtermode2[] = { _T("1x"), _T("2x"), _T("3x"), _T("4x"), 0 };
static const TCHAR *filtermode2v[] = { _T("-"), _T("1x"), _T("2x"), _T("3x"), _T("4x"), 0 };
#endif
static const TCHAR *cartsmode[] = { _T("none"), _T("hrtmon"), nullptr };
static const TCHAR *idemode[] = { _T("none"), _T("a600/a1200"), _T("a4000"), nullptr };
static const TCHAR *rtctype[] = { _T("none"), _T("MSM6242B"), _T("RP5C01A"), _T("MSM6242B_A2000"), nullptr };
static const TCHAR *ciaatodmode[] = { _T("vblank"), _T("50hz"), _T("60hz"), nullptr };
static const TCHAR *ksmirrortype[] = { _T("none"), _T("e0"), _T("a8+e0"), nullptr };
static const TCHAR *cscompa[] = {
	_T("-"), _T("Generic"), _T("CDTV"), _T("CDTV-CR"), _T("CD32"), _T("A500"), _T("A500+"), _T("A600"),
	_T("A1000"), _T("A1200"), _T("A2000"), _T("A3000"), _T("A3000T"), _T("A4000"), _T("A4000T"),
	_T("Velvet"), _T("Casablanca"), _T("DraCo"),
	nullptr
};
static const TCHAR *qsmodes[] = {
	_T("A500"), _T("A500+"), _T("A600"), _T("A1000"), _T("A1200"), _T("A3000"), _T("A4000"), _T(""), _T("CD32"), _T("CDTV"), _T("CDTV-CR"), _T("ARCADIA"), nullptr
};
/* 3-state boolean! */
static const TCHAR *fullmodes[] = { _T("false"), _T("true"), /* "FILE_NOT_FOUND", */ _T("fullwindow"), nullptr };
/* bleh for compatibility */
static const TCHAR *scsimode[] = { _T("false"), _T("true"), _T("scsi"), nullptr };
static const TCHAR *maxhoriz[] = { _T("lores"), _T("hires"), _T("superhires"), nullptr };
static const TCHAR *maxvert[] = { _T("nointerlace"), _T("interlace"), nullptr };
static const TCHAR *abspointers[] = { _T("none"), _T("mousehack"), _T("tablet"), nullptr };
static const TCHAR *magiccursors[] = { _T("both"), _T("native"), _T("host"), nullptr };
static const TCHAR *autoscale[] = { _T("none"), _T("auto"), _T("standard"), _T("max"), _T("scale"), _T("resize"), _T("center"), _T("manual"),
	_T("integer"), _T("integer_auto"), _T("separator"), _T("overscan_blanking"), nullptr };
static const TCHAR *autoscale_rtg[] = { _T("resize"), _T("scale"), _T("center"), _T("integer"), nullptr };
static const TCHAR *autoscalelimit[] = { _T("1/1"), _T("1/2"), _T("1/4"), _T("1/8"), nullptr };
static const TCHAR *joyportmodes[] = { _T(""), _T("mouse"), _T("mousenowheel"), _T("djoy"), _T("gamepad"), _T("ajoy"), _T("cdtvjoy"), _T("cd32joy"), _T("lightpen"), nullptr };
static const TCHAR *joyportsubmodes_lightpen[] = { _T(""), _T("trojan"), nullptr };
static const TCHAR *joyaf[] = { _T("none"), _T("normal"), _T("toggle"), _T("always"), _T("togglebutton"), nullptr };
static const TCHAR *epsonprinter[] = { _T("none"), _T("ascii"), _T("epson_matrix_9pin"), _T("epson_matrix_24pin"), _T("epson_matrix_48pin"), nullptr };
static const TCHAR *aspects[] = { _T("none"), _T("vga"), _T("tv"), nullptr };
static const TCHAR *vsyncmodes[] = { _T("false"), _T("true"), _T("autoswitch"), nullptr };
static const TCHAR *vsyncmodes2[] = { _T("normal"), _T("busywait"), nullptr };
static const TCHAR *filterapi[] = { _T("directdraw"), _T("direct3d"), _T("direct3d11"), _T("direct3d11"), _T("sdl2"), nullptr};
static const TCHAR *filterapiopts[] = { _T("hardware"), _T("software"), nullptr };
static const TCHAR *overscanmodes[] = { _T("tv_narrow"), _T("tv_standard"), _T("tv_wide"), _T("overscan"), _T("broadcast"), _T("extreme"), _T("ultra"), _T("ultra_hv"), _T("ultra_csync"), nullptr};
static const TCHAR *dongles[] =
{
	_T("none"),
	_T("robocop 3"), _T("leaderboard"), _T("b.a.t. ii"), _T("italy'90 soccer"), _T("dames grand maitre"),
	_T("rugby coach"), _T("cricket captain"), _T("leviathan"), _T("musicmaster"),
	_T("logistics"), _T("scala red"), _T("scala green"),
	_T("strikermanager"), _T("multi-player soccer manager"), _T("football director 2"),
	nullptr
};
static const TCHAR *cdmodes[] = { _T("disabled"), _T(""), _T("image"), _T("ioctl"), _T("spti"), _T("aspi"), nullptr };
static const TCHAR *cdconmodes[] = { _T(""), _T("uae"), _T("ide"), _T("scsi"), _T("cdtv"), _T("cd32"), nullptr };
static const TCHAR *genlockmodes[] = { _T("none"), _T("noise"), _T("testcard"), _T("image"), _T("video"), _T("stream"), _T("ld"), _T("sony_ld"), _T("pioneer_ld"), nullptr};
static const TCHAR *ppc_implementations[] = {
	_T("auto"),
	_T("dummy"),
	_T("pearpc"),
	_T("qemu"),
	nullptr
};
static const TCHAR *ppc_cpu_idle[] = {
	_T("disabled"),
	_T("1"),
	_T("2"),
	_T("3"),
	_T("4"),
	_T("5"),
	_T("6"),
	_T("7"),
	_T("8"),
	_T("9"),
	_T("max"),
	nullptr
};
static const TCHAR *waitblits[] = { _T("disabled"), _T("automatic"), _T("noidleonly"), _T("always"), nullptr };
static const TCHAR *autoext2[] = { _T("disabled"), _T("copy"), _T("replace"), nullptr };
static const TCHAR *leds[] = { _T("power"), _T("df0"), _T("df1"), _T("df2"), _T("df3"), _T("hd"), _T("cd"), _T("fps"), _T("cpu"), _T("snd"), _T("md"), _T("net"), nullptr };
static const int leds_order[] = { 3, 6, 7, 8, 9, 4, 5, 2, 1, 0, 9, 10 };
static const TCHAR *lacer[] = { _T("off"), _T("i"), _T("p"), nullptr };
/* another boolean to choice update... */
static const TCHAR *cycleexact[] = { _T("false"), _T("memory"), _T("true"), nullptr  };
static const TCHAR *unmapped[] = { _T("floating"), _T("zero"), _T("one"), nullptr };
static const TCHAR *ciatype[] = { _T("default"), _T("391078-01"), nullptr };
static const TCHAR *debugfeatures[] = { _T("segtracker"), _T("fsdebug"), nullptr };
static const TCHAR *hvcsync[] = { _T("hvcsync"), _T("csync"), _T("hvsync"), nullptr };
static const TCHAR *eclocksync[] = { _T("default"), _T("68000"), _T("Gayle"), _T("68000_opt"), nullptr };
static const TCHAR *agnusmodel[] = { _T("default"), _T("velvet"), _T("a1000"), _T("ocs"), _T("ecs"), _T("aga"), nullptr };
static const TCHAR *agnussize[] = { _T("default"), _T("512k"), _T("1m"), _T("2m"), nullptr };
static const TCHAR *denisemodel[] = { _T("default"), _T("velvet"), _T("a1000_noehb"), _T("a1000"), _T("ocs"), _T("ecs"), _T("aga"), nullptr };
static const TCHAR *kbtype[] = {
	_T("disconnected"),
	_T("UAE"),
	_T("a500_6570-036"),
	_T("a600_6570-036"),
	_T("a1000_6500-1"),
	_T("a1000_6570-036"),
	_T("a1200_6805"),
	_T("a2000_8039"),
	_T("ax000_6570-036"),
	nullptr
};

struct hdcontrollerconfig
{
	const TCHAR *label;
	int romtype;
};

static const struct hdcontrollerconfig hdcontrollers[] = {
	{ _T("uae"), 0 },

	{ _T("ide%d"), 0 },
	{ _T("ide%d_mainboard"), ROMTYPE_MB_IDE },

	{ _T("scsi%d"), 0 },
	{ _T("scsi%d_a3000"), ROMTYPE_SCSI_A3000 },
	{ _T("scsi%d_a4000t"), ROMTYPE_SCSI_A4000T },
	{ _T("scsi%d_cdtv"), ROMTYPE_CDTVSCSI },

	{nullptr}
};
static const TCHAR *z3mapping[] = {
	_T("auto"),
	_T("uae"),
	_T("real"),
	nullptr
};
static const TCHAR *uaescsidevmodes[] = {
	_T("original"),
	_T("rename_scsi"),
	nullptr
};
static const TCHAR *uaebootrom[] = {
	_T("automatic"),
	_T("disabled"),
	_T("min"),
	_T("full"),
	nullptr
};
static const TCHAR *uaeboard[] = {
	_T("disabled"),
	_T("min"),
	_T("full"),
	_T("full+indirect"),
	nullptr
};
static const TCHAR *uaeboard_off[] = {
	_T("disabled_off"),
	_T("min_off"),
	_T("full_off"),
	_T("full+indirect_off"),
	nullptr
};

static const TCHAR *serialcrlf[] = {
	_T("disabled"),
	_T("crlf_cr"),
	nullptr
};
static const TCHAR *threebitcolors[] = {
	_T("disabled"),
	_T("3to4to8bit"),
	_T("3to4bit"),
	_T("3to8bit"),
	nullptr
};

static const TCHAR *obsolete[] = {
	_T("accuracy"), _T("gfx_opengl"), _T("gfx_32bit_blits"), _T("32bit_blits"),
	_T("gfx_immediate_blits"), _T("gfx_ntsc"), _T("win32"), _T("gfx_filter_bits"),
	_T("sound_pri_cutoff"), _T("sound_pri_time"), _T("sound_min_buff"), _T("sound_bits"),
	_T("gfx_test_speed"), _T("gfxlib_replacement"), _T("enforcer"), _T("catweasel_io"),
	_T("kickstart_key_file"), _T("fast_copper"), _T("sound_adjust"), _T("sound_latency"),
	_T("serial_hardware_dtrdsr"), _T("gfx_filter_upscale"),
	_T("gfx_autoscale"), _T("parallel_sampler"), _T("parallel_ascii_emulation"),
	_T("avoid_vid"), _T("avoid_dga"), _T("z3chipmem_size"), _T("state_replay_buffer"), _T("state_replay"),
	_T("z3realmapping"), _T("force_0x10000000_z3"),
	_T("fpu_arithmetic_exceptions"),

	_T("gfx_filter_vert_zoom"),_T("gfx_filter_horiz_zoom"),
	_T("gfx_filter_vert_zoom_mult"), _T("gfx_filter_horiz_zoom_mult"),
	_T("gfx_filter_vert_offset"), _T("gfx_filter_horiz_offset"),
	_T("gfx_tearing"), _T("gfx_tearing_rtg"),

	// created by some buggy beta
	_T("uaehf0%s,%s"),
	_T("uaehf1%s,%s"),
	_T("uaehf2%s,%s"),
	_T("uaehf3%s,%s"),
	_T("uaehf4%s,%s"),
	_T("uaehf5%s,%s"),
	_T("uaehf6%s,%s"),
	_T("uaehf7%s,%s"),

	_T("pcibridge_rom_file"),
	_T("pcibridge_rom_options"),

	_T("cpuboard_ext_rom_file"),
	_T("uaeboard_mode"),

	_T("comp_oldsegv"),
	_T("comp_midopt"),
	_T("comp_lowopt"),
	_T("avoid_cmov"),
	_T("compforcesettings"),
	_T("comp_catchdetect"),

	_T("hblank_glitch"),

	_T("gfx_hdr"),

	_T("bogomem_fast"),
	_T("sound_cdaudio"),

	nullptr
};

#define UNEXPANDED _T("$(FILE_PATH)")

static TCHAR *cfgfile_unescape(const TCHAR *s, const TCHAR **endpos, TCHAR separator, bool min)
{
	bool quoted = false;
	TCHAR *s2 = xmalloc(TCHAR, _tcslen(s) + 1);
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
		if (c == '\\' && !min) {
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
			case 'n':
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
static TCHAR *cfgfile_unescape(const TCHAR *s, const TCHAR **endpos)
{
	return cfgfile_unescape(s, endpos, 0, false);
}
static TCHAR *cfgfile_unescape_min(const TCHAR *s)
{
	return cfgfile_unescape(s, nullptr, 0, true);
}

static void clearmountitems(struct uae_prefs *p)
{
	p->mountitems = 0;
	for (auto & i : p->mountconfig) {
		i.configoffset = -1;
		i.unitnum = -1;
	}
}

void discard_prefs(struct uae_prefs *p, int type)
{
	struct strlist **ps = &p->all_lines;
	while (*ps) {
		struct strlist *s = *ps;
		*ps = s->next;
		xfree(s->value);
		xfree(s->option);
		xfree(s);
	}
	p->all_lines = nullptr;
	currprefs.all_lines = changed_prefs.all_lines = nullptr;
#ifdef FILESYS
	filesys_cleanup();
#endif
	clearmountitems(p);
}

static TCHAR *cfgfile_option_find_it(const TCHAR *s, const TCHAR *option, bool checkequals)
{
	TCHAR buf[MAX_DPATH];
	if (!s)
		return nullptr;
	_tcscpy(buf, s);
	_tcscat(buf, _T(","));
	TCHAR *p = buf;
	for (;;) {
		TCHAR *tmpp = _tcschr(p, ',');
		TCHAR *tmpp2 = nullptr;
		if (tmpp == nullptr)
			return nullptr;
		*tmpp++ = 0;
		if (checkequals) {
			tmpp2 = _tcschr(p, '=');
			if (tmpp2)
				*tmpp2++ = 0;
		}
		if (!_tcsicmp(p, option)) {
			if (checkequals && tmpp2) {
				if (tmpp2[0] == '"') {
					TCHAR *n = cfgfile_unescape_min(tmpp2);
					return n;
				}
				return my_strdup(tmpp2);
			}
			return my_strdup(p);
		}
		p = tmpp;
	}
}

bool cfgfile_option_find(const TCHAR *s, const TCHAR *option)
{
	TCHAR *ss = cfgfile_option_find_it(s, option, false);
	xfree(ss);
	return ss != nullptr;
}

TCHAR *cfgfile_option_get(const TCHAR *s, const TCHAR *option)
{
	return cfgfile_option_find_it(s, option, true);
}

bool cfgfile_option_get_bool(const TCHAR *s, const TCHAR *option)
{
	TCHAR *d = cfgfile_option_find_it(s, option, true);
	bool ret = d && (!_tcsicmp(d, _T("true")) || !_tcsicmp(d, _T("1")));
	xfree(d);
	return ret;
}

bool cfgfile_option_get_nbool(const TCHAR *s, const TCHAR *option)
{
	TCHAR *d = cfgfile_option_find_it(s, option, true);
	bool ret = d && (!_tcsicmp(d, _T("false")) || !_tcsicmp(d, _T("0")));
	xfree(d);
	return ret;
}

static void trimwsa (char *s)
{
	/* Delete trailing whitespace.  */
	int len = uaestrlen(s);
	while (len > 0 && strcspn (s + len - 1, "\t \r\n") == 0)
		s[--len] = '\0';
}

static int match_string (const TCHAR *table[], const TCHAR *str)
{
	for (auto i = 0; table[i] != nullptr; i++)
		if (_tcsicmp(table[i], str) == 0)
			return i;
	return -1;
}

// escape config file separators and control characters
static TCHAR *cfgfile_escape (const TCHAR *s, const TCHAR *escstr, bool quote, bool min)
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
		// always quote if starts or ends with space
		if (c == ' ' && (s[i + 1] == 0 || i == 0)) {
			doquote = true;
		}
	}
	if (escstr == nullptr && quote)
		doquote = true;
	TCHAR *s2 = xmalloc (TCHAR, _tcslen (s) + cnt * 4 + 2 + 1);
	TCHAR *p = s2;
	if (doquote)
		*p++ = '\"';
	for (int i = 0; s[i]; i++) {
		TCHAR c = s[i];
		if (c == 0)
			break;
		if ((c == '\\' && !min) || c == '\"' || (c == '\'' && !min)) {
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
		} else if (c < 32 && !min) {
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

// escape only , and " or if starts or ends with a space
static TCHAR *cfgfile_escape_min(const TCHAR *s)
{
	for (int i = 0; s[i]; i++) {
		TCHAR c = s[i];
		if (c == ',' || c == '\"' || (c == ' ' && (i == 0 || s[i + 1] == 0))) {
			return cfgfile_escape(s, _T(","), true, true);
		}
	}
	return my_strdup(s);
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
			return nullptr;
		}
		value++;
		*valuep = value;
	} else {
		s = cfgfile_unescape (value, valuep, separator, false);
	}
	return s;
}

static TCHAR *cfgfile_subst_path2 (const TCHAR *path, const TCHAR *subst, const TCHAR *file)
{
	/* @@@ use _tcsicmp for some targets.  */
	if (path != nullptr && subst != nullptr && _tcslen (path) > 0 && _tcsncmp (file, path, _tcslen (path)) == 0) {
		int l;
		TCHAR *p2, *p = xmalloc (TCHAR, _tcslen (file) + _tcslen (subst) + 2);
		_tcscpy (p, subst);
		l = uaetcslen(p);
		while (l > 0 && p[l - 1] == '/')
			p[--l] = '\0';
		l = uaetcslen(path);
		while (file[l] == '/')
			l++;
		_tcscat(p, _T("/"));
		_tcscat(p, file + l);
		p2 = target_expand_environment (p, nullptr, 0);
		xfree (p);
		if (p2 && p2[0] == '$') {
			xfree(p2);
			return nullptr;
		}
		return p2;
	}
	return nullptr;
}

TCHAR *cfgfile_subst_path (const TCHAR *path, const TCHAR *subst, const TCHAR *file)
{
	TCHAR *s = cfgfile_subst_path2 (path, subst, file);
	if (s)
		return s;
	s = target_expand_environment (file, nullptr, 0);
	if (s) {
		TCHAR tmp[MAX_DPATH];
		_tcscpy (tmp, s);
		xfree (s);
		fullpath (tmp, sizeof tmp / sizeof (TCHAR));
		s = my_strdup (tmp);
	}
	return s;
}

static TCHAR *cfgfile_get_multipath2 (struct multipath *mp, const TCHAR *path, const TCHAR *file, bool dir)
{
	for (auto& i : mp->path)
	{
		if (i[0] && _tcscmp (i, _T(".\\")) != 0 && _tcscmp (i, _T("./")) != 0 && (file[0] != '/' && file[0] != '\\' && !_tcschr(file, ':'))) {
			TCHAR *s = nullptr;
			if (path)
				s = cfgfile_subst_path2 (path, i, file);
			if (!s) {
				TCHAR np[MAX_DPATH];
				_tcscpy (np, i);
				fix_trailing(np);
				_tcscat (np, file);
				fullpath (np, sizeof np / sizeof (TCHAR));
				s = my_strdup (np);
			}
			if (dir) {
				if (my_existsdir (s))
					return s;
			} else {
				if (zfile_exists (s))
					return s;
			}
			xfree (s);
		}
	}
	return nullptr;
}

static TCHAR *cfgfile_get_multipath (struct multipath *mp, const TCHAR *path, const TCHAR *file, bool dir)
{
	TCHAR *s = cfgfile_get_multipath2 (mp, path, file, dir);
	if (s)
		return s;
	return my_strdup (file);
}

static TCHAR *cfgfile_put_multipath (struct multipath *mp, const TCHAR *s)
{
	for (auto& i : mp->path) {
		if (i[0] && _tcscmp (i, _T(".\\")) != 0 && _tcscmp (i, _T("./")) != 0) {
			if (_tcsnicmp (i, s, _tcslen (i)) == 0) {
				return my_strdup (s + _tcslen (i));
			}
		}
	}
	return my_strdup (s);
}


static TCHAR *cfgfile_subst_path_load (const TCHAR *path, struct multipath *mp, const TCHAR *file, bool dir)
{
	TCHAR *s = cfgfile_get_multipath2 (mp, path, file, dir);
	if (s)
		return s;
	return cfgfile_subst_path (path, mp->path[0], file);
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
	TCHAR lf = 10;
	const auto v = zfile_fwrite(b, _tcslen((TCHAR*)b), sizeof(TCHAR), z);
	zfile_fwrite(&lf, 1, 1, z);
	return v;
}

#define UTF8NAME _T(".utf8")

static void cfg_dowrite(struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *value, int d, int target, int escape)
{
	char lf = 10;
	TCHAR tmp[CONFIG_BLEN], tmpext[CONFIG_BLEN];
	const TCHAR *optionp;
	const TCHAR *new_value = nullptr;
	bool free_value = false;
	char tmpa[CONFIG_BLEN];
	char *tmp1, *tmp2;
	int utf8;

	if (value == nullptr)
		return;
	if (optionext) {
		_tcscpy(tmpext, option);
		_tcscat(tmpext, optionext);
		optionp = tmpext;
	} else {
		optionp = option;
	}
	utf8 = 0;
	tmp1 = ua(value);
	tmp2 = uutf8(value);
	if (strcmp(tmp1, tmp2) && tmp2[0] != 0)
		utf8 = 1;
	if (escape) {
		new_value = cfgfile_escape_min(value);
		free_value = true;
	} else {
		new_value = value;
	}
	if (target)
		_sntprintf(tmp, sizeof tmp, _T("%s.%s=%s"), TARGET_NAME, optionp, new_value);
	else
		_sntprintf(tmp, sizeof tmp, _T("%s=%s"), optionp, new_value);
	if (d && isdefault (tmp))
		goto end;
	cfg_write(tmp, f);
	if (utf8 && !unicode_config) {
		char *opt = ua(optionp);
		if (target) {
			char *tna = ua(TARGET_NAME);
			_sntprintf(tmpa, sizeof tmpa, "%s.%s.utf8=%s", tna, opt, tmp2);
			xfree(tna);
		} else {
			_sntprintf(tmpa, sizeof tmpa, "%s.utf8=%s", opt, tmp2);
		}
		xfree(opt);
		zfile_fwrite(tmpa, strlen (tmpa), 1, f);
		zfile_fwrite(&lf, 1, 1, f);
	}
end:
	if (free_value) {
		xfree((void*)new_value);
	}
	xfree(tmp2);
	xfree(tmp1);
}

static bool checkstrarr(const TCHAR *option, const TCHAR *arr[], int value)
{
	if (value < 0) {
		write_log("Invalid config entry '%s', value %d\n", option, value);
		return false;
	}
	for (int i = 0; i <= value; i++) {
		if (arr[i] == nullptr) {
			write_log("Invalid config entry '%s', value %d\n", option, value);
			return false;
		}
	}
	return true;
}

static void cfgfile_dwrite_coords(struct zfile *f, const TCHAR *option, int x, int y)
{
	if (x || y)
		cfgfile_dwrite(f, option, _T("%d,%d"), x, y);
}
static void cfg_dowrite(struct zfile *f, const TCHAR *option, const TCHAR *value, int d, int target, int escape)
{
	cfg_dowrite(f, option, nullptr, value, d, target, escape);
}
void cfgfile_write_bool(struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 0, 0, 0);
}
void cfgfile_dwrite_bool(struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 1, 0, 0);
}
static void cfgfile_dwrite_bool(struct zfile *f, const TCHAR *option, const TCHAR *optionext, bool b)
{
	cfg_dowrite(f, option, optionext, b ? _T("true") : _T("false"), 1, 0, 0);
}
static void cfgfile_dwrite_bool(struct zfile *f, const TCHAR *option, int b)
{
	cfgfile_dwrite_bool (f, option, b != 0);
}
void cfgfile_write_str(struct zfile *f, const TCHAR *option, const TCHAR *value)
{
	cfg_dowrite(f, option, value, 0, 0, 0);
}
void cfgfile_write_strarr(struct zfile *f, const TCHAR *option, const TCHAR *arr[], int value)
{
	checkstrarr(option, arr, value);
	const TCHAR *v = arr[value];
	cfg_dowrite(f, option, v, 0, 0, 0);
}
static void cfgfile_write_str(struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *value)
{
	cfg_dowrite(f, option, optionext, value, 0, 0, 0);
}
static void cfgfile_write_str_escape(struct zfile *f, const TCHAR *option, const TCHAR *value)
{
	cfg_dowrite(f, option, value, 0, 0, 1);
}
void cfgfile_dwrite_str(struct zfile *f, const TCHAR *option, const TCHAR *value)
{
	cfg_dowrite(f, option, value, 1, 0, 0);
}
static void cfgfile_dwrite_str(struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *value)
{
	cfg_dowrite(f, option, optionext, value, 1, 0, 0);
}
void cfgfile_dwrite_strarr(struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *arr[], int value)
{
	checkstrarr(option, arr, value);
	const TCHAR *v = arr[value];
	cfg_dowrite(f, option, optionext, v, 1, 0, 0);
}
void cfgfile_dwrite_strarr(struct zfile *f, const TCHAR *option, const TCHAR *arr[], int value)
{
	cfgfile_dwrite_strarr(f, option, nullptr, arr, value);
}
void cfgfile_target_write_bool(struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 0, 1, 0);
}
void cfgfile_target_dwrite_bool(struct zfile *f, const TCHAR *option, bool b)
{
	cfg_dowrite(f, option, b ? _T("true") : _T("false"), 1, 1, 0);
}
void cfgfile_target_write_str(struct zfile *f, const TCHAR *option, const TCHAR *value)
{
	cfg_dowrite(f, option, value, 0, 1, 0);
}
void cfgfile_target_dwrite_str(struct zfile *f, const TCHAR *option, const TCHAR *value)
{
	cfg_dowrite(f, option, value, 1, 1, 0);
}
void cfgfile_target_dwrite_str_escape(struct zfile *f, const TCHAR *option, const TCHAR *value)
{
	cfg_dowrite(f, option, value, 1, 1, 1);
}

static void cfgfile_write_ext(struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN], tmp2[CONFIG_BLEN];

	if (optionext) {
		_tcscpy(tmp2, option);
		_tcscat(tmp2, optionext);
	}
	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, optionext ? tmp2 : option, tmp, 0, 0, 0);
	va_end(parms);
}
void cfgfile_write(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 0, 0, 0);
	va_end(parms);
}
void cfgfile_write_escape(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 0, 0, 1);
	va_end(parms);
}
static void cfgfile_dwrite_ext(struct zfile *f, const TCHAR *option, const TCHAR *optionext, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN], tmp2[CONFIG_BLEN];

	if (optionext) {
		_tcscpy(tmp2, option);
		_tcscat(tmp2, optionext);
	}
	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, optionext ? tmp2 : option, tmp, 1, 0, 0);
	va_end(parms);
}
void cfgfile_dwrite(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 1, 0, 0);
	va_end(parms);
}
void cfgfile_dwrite_escape(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 1, 0, 1);
	va_end(parms);
}
void cfgfile_target_write(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 0, 1, 0);
	va_end(parms);
}
void cfgfile_target_write_escape(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 0, 1, 1);
	va_end(parms);
}
void cfgfile_target_dwrite(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 1, 1, 0);
	va_end(parms);
}
void cfgfile_target_dwrite_escape(struct zfile *f, const TCHAR *option, const TCHAR *format,...)
{
	va_list parms;
	TCHAR tmp[CONFIG_BLEN];

	va_start(parms, format);
	_vsntprintf(tmp, CONFIG_BLEN, format, parms);
	cfg_dowrite(f, option, tmp, 1, 1, 1);
	va_end(parms);
}

static void cfgfile_write_rom (struct zfile *f, struct multipath *mp, const TCHAR *romfile, const TCHAR *name)
{
	TCHAR *str = cfgfile_subst_path (mp->path[0], UNEXPANDED, romfile);
	str = cfgfile_put_multipath (mp, str);
	cfgfile_write_str (f, name, str);
	struct zfile *zf = zfile_fopen (str, _T("rb"), ZFD_ALL);
	if (zf) {
		struct romdata *rd = getromdatabyzfile (zf);
		if (rd) {
			TCHAR name2[MAX_DPATH], str2[MAX_DPATH];
			_tcscpy (name2, name);
			_tcscat (name2, _T("_id"));
			_sntprintf (str2, sizeof str2, _T("%08X,%s"), rd->crc32, rd->name);
			cfgfile_write_str (f, name2, str2);
		}
		zfile_fclose (zf);
	}
	xfree (str);

}

static void cfgfile_to_path_save(const TCHAR *in, TCHAR *out, int type)
{
	if (_tcschr(in, '%')) {
		_tcscpy(out, in);
	} else {
		cfgfile_resolve_path_out_save(in, out, MAX_DPATH, type);
	}
}

static void cfgfile_write_path2(struct zfile *f, const TCHAR *option, const TCHAR *value, int type)
{
	if (_tcschr(value, '%')) {
		cfgfile_write_str(f, option, value);
	} else {
		TCHAR path[MAX_DPATH];
		cfgfile_resolve_path_out_save(value, path, MAX_DPATH, type);
		cfgfile_write_str(f, option, path);
	}
}
static void cfgfile_dwrite_path2(struct zfile *f, const TCHAR *option, const TCHAR *value, int type)
{
	if (_tcschr(value, '%')) {
		cfgfile_dwrite_str(f, option, value);
	} else {
		TCHAR path[MAX_DPATH];
		cfgfile_resolve_path_out_save(value, path, MAX_DPATH, type);
		cfgfile_dwrite_str(f, option, path);
	}
}

static void cfgfile_write_path (struct zfile *f, struct multipath *mp, const TCHAR *option, const TCHAR *value)
{
	TCHAR *s = cfgfile_put_multipath (mp, value);
	cfgfile_write_str (f, option, s);
	xfree (s);
}
static void cfgfile_dwrite_path (struct zfile *f, struct multipath *mp, const TCHAR *option, const TCHAR *value)
{
	TCHAR *s = cfgfile_put_multipath (mp, value);
	cfgfile_dwrite_str (f, option, s);
	xfree (s);
}

static void cfgfile_write_multichoice(struct zfile *f, const TCHAR *option, const TCHAR *table[], int value)
{
	TCHAR tmp[MAX_DPATH];
	if (!value)
		return;
	tmp[0] = 0;
	int i = 0;
	while (value && table[i]) {
		if (value & (1 << i) && table[i][0] && table[i][0] != '|') {
			if (tmp[0])
				_tcscat(tmp, _T(","));
			_tcscat(tmp, table[i]);
		}
		i++;
	}
	if (tmp[0])
		cfgfile_write(f, option, tmp);
}


static void cfgfile_adjust_path(TCHAR *path, int maxsz, struct multipath *mp)
{
	if (path[0] == 0)
		return;
	TCHAR *s = target_expand_environment(path, nullptr, 0);
	_tcsncpy(path, s, maxsz - 1);
	path[maxsz - 1] = 0;
	if (mp) {
		for (auto& i : mp->path) {
			if (i[0] && _tcscmp(i, _T(".\\")) != 0 && _tcscmp(i, _T("./")) != 0 && (path[0] != '/' && path[0] != '\\' && !_tcschr(path, ':'))) {
				TCHAR np[MAX_DPATH];
				_tcscpy(np, i);
				fix_trailing(np);
				_tcscat(np, s);
				fullpath(np, sizeof np / sizeof(TCHAR));
				if (zfile_exists(np)) {
					_tcsncpy(path, np, maxsz - 1);
					path[maxsz - 1] = 0;
					xfree(s);
					return;
				}
			}
		}
	}
	fullpath(path, maxsz);
	xfree(s);
}

static void cfgfile_resolve_path_out_all(const TCHAR *path, TCHAR *out, int size, int type, bool save)
{
	struct uae_prefs *p = &currprefs;
	TCHAR *s = nullptr;
	switch (type)
	{
	case PATH_DIR:
		s = cfgfile_subst_path_load(UNEXPANDED, &p->path_hardfile, path, true);
		break;
	case PATH_HDF:
		s = cfgfile_subst_path_load(UNEXPANDED, &p->path_hardfile, path, false);
		break;
	case PATH_CD:
		s = cfgfile_subst_path_load(UNEXPANDED, &p->path_cd, path, false);
		break;
	case PATH_ROM:
		s = cfgfile_subst_path_load(UNEXPANDED, &p->path_rom, path, false);
		break;
	case PATH_FLOPPY:
		if (_tcscmp(out, path) != 0)
			_tcscpy(out, path);
		cfgfile_adjust_path(out, MAX_DPATH, &p->path_floppy);
		break;
	default:
		s = cfgfile_subst_path(nullptr, nullptr, path);
		break;
	}
	if (s) {
		_tcscpy(out, s);
		xfree(s);
	}
	//if (!save) {
	//	my_resolvesoftlink(out, size, true);
	//}
}

void cfgfile_resolve_path_out_load(const TCHAR *path, TCHAR *out, int size, int type)
{
	cfgfile_resolve_path_out_all(path, out, size, type, false);
}
void cfgfile_resolve_path_load(TCHAR *path, int size, int type)
{
	cfgfile_resolve_path_out_all(path, path, size, type, false);
}
void cfgfile_resolve_path_out_save(const TCHAR *path, TCHAR *out, int size, int type)
{
	cfgfile_resolve_path_out_all(path, out, size, type, true);
}
void cfgfile_resolve_path_save(TCHAR *path, int size, int type)
{
	cfgfile_resolve_path_out_all(path, path, size, type, true);
}


static void write_filesys_config (struct uae_prefs *p, struct zfile *f)
{
	TCHAR tmp[MAX_DPATH], tmp2[MAX_DPATH], tmp3[MAX_DPATH], str1[MAX_DPATH], hdcs[MAX_DPATH];

	for (auto i = 0; i < p->mountitems; i++) {
		auto* uci = &p->mountconfig[i];
		auto* ci = &uci->ci;
		const auto bp = ci->bootpri;

		const auto* str2 = "";
		if (ci->rootdir[0] == ':') {
			// separate harddrive names
			_tcscpy(str1, ci->rootdir);
			auto* ptr = _tcschr (str1 + 1, ':');
			if (ptr) {
				*ptr++ = 0;
				str2 = ptr;
				ptr = (TCHAR *) _tcschr (str2, ',');
				if (ptr)
					*ptr = 0;
			}
		} else {
			cfgfile_to_path_save(ci->rootdir, str1, ci->type == UAEDEV_DIR ? PATH_DIR : (ci->type == UAEDEV_CD ? PATH_CD : (ci->type == UAEDEV_HDF ? PATH_HDF : PATH_TAPE)));
		}
		int ct = ci->controller_type;
		int romtype = 0;
		if (ct >= HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST && ct <= HD_CONTROLLER_TYPE_SCSI_LAST) {
			_sntprintf(hdcs, sizeof hdcs, _T("scsi%d_%s"), ci->controller_unit, expansionroms[ct - HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST].name);
			romtype = expansionroms[ct - HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST].romtype;
		} else if (ct >= HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST && ct <= HD_CONTROLLER_TYPE_IDE_LAST) {
			_sntprintf(hdcs, sizeof hdcs, _T("ide%d_%s"), ci->controller_unit, expansionroms[ct - HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST].name);
			romtype = expansionroms[ct - HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST].romtype;
		} else if (ct >= HD_CONTROLLER_TYPE_CUSTOM_FIRST && ct <= HD_CONTROLLER_TYPE_CUSTOM_LAST) {
			_sntprintf(hdcs, sizeof hdcs, _T("custom%d_%s"), ci->controller_unit, expansionroms[ct - HD_CONTROLLER_TYPE_CUSTOM_FIRST].name);
			romtype = expansionroms[ct - HD_CONTROLLER_TYPE_CUSTOM_FIRST].romtype;
		} else if (ct == HD_CONTROLLER_TYPE_SCSI_AUTO) {
			_sntprintf(hdcs, sizeof hdcs, _T("scsi%d"), ci->controller_unit);
		} else if (ct == HD_CONTROLLER_TYPE_IDE_AUTO) {
			_sntprintf(hdcs, sizeof hdcs, _T("ide%d"), ci->controller_unit);
		} else if (ct == HD_CONTROLLER_TYPE_UAE) {
			_sntprintf(hdcs, sizeof hdcs, _T("uae%d"), ci->controller_unit);
		}
		if (romtype) {
			for (int j = 0; hdcontrollers[j].label; j++) {
				if (hdcontrollers[j].romtype == (romtype & ROMTYPE_MASK)) {
					_sntprintf(hdcs, sizeof hdcs, hdcontrollers[j].label, ci->controller_unit);
					break;
				}
			}
		}
		if (ci->controller_type_unit > 0)
			_sntprintf(hdcs + _tcslen(hdcs), sizeof hdcs + _tcslen(hdcs), _T("-%d"), ci->controller_type_unit + 1);

		auto* str1b = cfgfile_escape (str1, _T(":,"), true, false);
		auto* str1c = cfgfile_escape_min(str1);
		auto* str2b = cfgfile_escape (str2, _T(":,"), true, false);
		if (ci->type == UAEDEV_DIR) {
			_sntprintf (tmp, sizeof tmp, _T("%s,%s:%s:%s,%d"), ci->readonly ? _T("ro") : _T("rw"),
				ci->devname[0] ? ci->devname : _T(""), ci->volname, str1c, bp);
			cfgfile_write_str (f, _T("filesystem2"), tmp);
			_tcscpy (tmp3, tmp);
#if 0
			_stprintf (tmp2, _T("filesystem=%s,%s:%s"), uci->readonly ? _T("ro") : _T("rw"),
				uci->volname, str);
			zfile_fputs (f, tmp2);
#endif
		} else if (ci->type == UAEDEV_HDF || ci->type == UAEDEV_CD || ci->type == UAEDEV_TAPE) {
			TCHAR filesyspath[MAX_DPATH];
			cfgfile_to_path_save(ci->filesys, filesyspath, PATH_HDF);
			TCHAR *sfilesys = cfgfile_escape_min(filesyspath);
			TCHAR *sgeometry = cfgfile_escape(ci->geometry, nullptr, true, false);
			_sntprintf (tmp, sizeof tmp, _T("%s,%s:%s,%d,%d,%d,%d,%d,%s,%s"),
				ci->readonly ? _T("ro") : _T("rw"),
				ci->devname[0] ? ci->devname : _T(""), str1c,
				ci->sectors, ci->surfaces, ci->reserved, ci->blocksize,
				bp, ci->filesys[0] ? sfilesys : _T(""), hdcs);
			_sntprintf (tmp3, sizeof tmp3, _T("%s,%s:%s%s%s,%d,%d,%d,%d,%d,%s,%s"),
				ci->readonly ? _T("ro") : _T("rw"),
				ci->devname[0] ? ci->devname : _T(""), str1b, str2b[0] ? _T(":") : _T(""), str2b,
				ci->sectors, ci->surfaces, ci->reserved, ci->blocksize,
				bp, ci->filesys[0] ? sfilesys : _T(""), hdcs);
			if (ci->highcyl || ci->physical_geometry || ci->geometry[0]) {
				TCHAR *s = tmp + _tcslen (tmp);
				TCHAR *s2 = s;
				_sntprintf (s2, sizeof s2, _T(",%d"), ci->highcyl);
				if ((ci->physical_geometry && ci->pheads && ci->psecs) || ci->geometry[0]) {
					s = tmp + _tcslen (tmp);
					_sntprintf (s, sizeof s, _T(",%d/%d/%d"), ci->pcyls, ci->pheads, ci->psecs);
					if (ci->geometry[0]) {
						s = tmp + _tcslen (tmp);
						_sntprintf (s, sizeof s, _T(",%s"), sgeometry);
					}
				}
				_tcscat (tmp3, s2);
				xfree(sfilesys);
				xfree(sgeometry);
			}
			if (ci->controller_media_type) {
				_tcscat(tmp, _T(",CF"));
				_tcscat(tmp3, _T(",CF"));
			}
			const TCHAR *extras = nullptr;
			if (ct >= HD_CONTROLLER_TYPE_SCSI_FIRST && ct <= HD_CONTROLLER_TYPE_SCSI_LAST) {
				if (ci->unit_feature_level == HD_LEVEL_SCSI_1){
					extras = _T("SCSI1");
				}	else if (ci->unit_feature_level == HD_LEVEL_SASI) {
					extras = _T("SASI");
				} else if (ci->unit_feature_level == HD_LEVEL_SASI_ENHANCED) {
					extras = _T("SASIE");
				} else if (ci->unit_feature_level == HD_LEVEL_SASI_CHS) {
					extras = _T("SASI_CHS");
				}
			} else if (ct >= HD_CONTROLLER_TYPE_IDE_FIRST && ct <= HD_CONTROLLER_TYPE_IDE_LAST) {
				if (ci->unit_feature_level == HD_LEVEL_ATA_1) {
					extras = _T("ATA1");
				} else if (ci->unit_feature_level == HD_LEVEL_ATA_2S) {
					extras = _T("ATA2+S");
				}
			}
			if (extras) {
				_tcscat(tmp, _T(","));
				_tcscat(tmp3, _T(","));
				_tcscat(tmp, extras);
				_tcscat(tmp3, extras);
			}
			if (ci->unit_special_flags) {
				TCHAR tmpx[32];
				_sntprintf(tmpx, sizeof tmpx, _T(",flags=0x%x"), ci->unit_special_flags);
				_tcscat(tmp, tmpx);
				_tcscat(tmp3, tmpx);
			}
			if (ci->lock) {
				_tcscat(tmp, _T(",lock"));
				_tcscat(tmp3, _T(",lock"));
			}
			if (ci->loadidentity) {
				_tcscat(tmp, _T(",identity"));
				_tcscat(tmp3, _T(",identity"));
			}

			if (ci->type == UAEDEV_HDF)
				cfgfile_write_str (f, _T("hardfile2"), tmp);
#if 0
			_stprintf (tmp2, _T("hardfile=%s,%d,%d,%d,%d,%s"),
				uci->readonly ? "ro" : "rw", uci->sectors,
				uci->surfaces, uci->reserved, uci->blocksize, str);
			zfile_fputs (f, tmp2);
#endif
		}
		_sntprintf (tmp2, sizeof tmp2, _T("uaehf%d"), i);
		if (ci->type == UAEDEV_CD) {
			cfgfile_write (f, tmp2, _T("cd%d,%s"), ci->device_emu_unit, tmp);
		} else if (ci->type == UAEDEV_TAPE) {
			cfgfile_write (f, tmp2, _T("tape%d,%s"), ci->device_emu_unit, tmp);
		} else {
			cfgfile_write (f, tmp2, _T("%s,%s"), ci->type == UAEDEV_HDF ? _T("hdf") : _T("dir"), tmp3);
		}
		if (ci->type == UAEDEV_DIR) {
			bool add_extra = false;
			if (ci->inject_icons) {
				add_extra = true;
			}
			if (add_extra) {
				_sntprintf(tmp2, sizeof tmp2, _T("%s,inject_icons=%s"), ci->devname, ci->inject_icons ? _T("true") : _T("false"));
				cfgfile_write(f, _T("filesystem_extra"), tmp2);
			}
		}
		xfree (str1b);
		xfree (str1c);
		xfree (str2b);
	}
}

static void write_compatibility_cpu (struct zfile *f, struct uae_prefs *p)
{
	TCHAR tmp[100];

	auto model = p->cpu_model;
	if (model == 68030)
		model = 68020;
	if (model == 68060)
		model = 68040;
	if (p->address_space_24 && model == 68020)
		_tcscpy (tmp, _T("68ec020"));
	else
		_sntprintf (tmp, sizeof tmp, _T("%d"), model);
	if (model == 68020 && (p->fpu_model == 68881 || p->fpu_model == 68882))
		_tcscat (tmp, _T("/68881"));
	cfgfile_write (f, _T("cpu_type"), tmp);
}

static void write_leds (struct zfile *f, const TCHAR *name, int mask)
{
	TCHAR tmp[MAX_DPATH];
	tmp[0] = 0;
	for (int i = 0; leds[i]; i++) {
		bool got = false;
		for (int j = 0; leds[j]; j++) {
			if (leds_order[j] == i) {
				if (mask & (1 << j)) {
					if (got)
						_tcscat (tmp, _T(":"));
					_tcscat (tmp, leds[j]);
					got = true;
				}
			}
		}
		if (leds[i + 1] && got)
			_tcscat (tmp, _T(","));
	}
	while (tmp[0] && tmp[_tcslen (tmp) - 1] == ',')
		tmp[_tcslen (tmp) - 1] = 0;
	cfgfile_dwrite_str (f, name, tmp);
}

static void write_resolution (struct zfile *f, const TCHAR *ws, const TCHAR *hs, struct wh *wh)
{
	if (wh->width <= 0 || wh->height <= 0 || wh->special == WH_NATIVE) {
		cfgfile_write_str (f, ws, _T("native"));
		cfgfile_write_str (f, hs, _T("native"));
	} else {
		cfgfile_write (f, ws, _T("%d"), wh->width);
		cfgfile_write (f, hs, _T("%d"), wh->height);
	}
}

static int cfgfile_read_rom_settings(const struct expansionboardsettings *ebs, const TCHAR *buf, TCHAR *configtext)
{
	int settings = 0;
	int bitcnt = 0;

	if (configtext)
		configtext[0] = 0;
	auto* ct = configtext;
	for (int i = 0; ebs[i].name; i++) {
		const auto* eb = &ebs[i];
		bitcnt += eb->bitshift;
		if (eb->type == EXPANSIONBOARD_STRING) {
			auto* p = cfgfile_option_get(buf, eb->configname);
			if (p) {
				_tcscpy(ct, p);
				ct += _tcslen(ct);
				xfree(p);
			}
			*ct++ = 0;
		} else if (eb->type == EXPANSIONBOARD_MULTI) {
			int itemcnt = -1;
			int itemfound = 0;
			const auto* p = eb->configname;
			while (p[0]) {
				if (itemcnt >= 0) {
					if (cfgfile_option_find(buf, p)) {
						itemfound = itemcnt;
					}
				}
				itemcnt++;
				p += _tcslen(p) + 1;
			}
			int cnt = 1;
			int bits = 1;
			for (int j = 0; j < 8; j++) {
				if ((1 << j) >= itemcnt) {
					cnt = 1 << j;
					bits = j;
					break;
				}
			}
			int multimask = cnt - 1;
			if (eb->invert)
				itemfound ^= 0x7fffffff;
			itemfound &= multimask;
			settings |= itemfound << bitcnt;
			bitcnt += bits;
		} else {
			int mask = 1 << bitcnt;
			if (cfgfile_option_find(buf, eb->configname)) {
				settings |= mask;
			}
			if (eb->invert)
				settings ^= mask;
			bitcnt++;
		}
	}
	return settings;
}

static void cfgfile_write_rom_settings(const struct expansionboardsettings *ebs, TCHAR *buf, int settings, const TCHAR *settingstring)
{
	int bitcnt = 0;
	int sstr = 0;
	for (int j = 0; ebs[j].name; j++) {
		const struct expansionboardsettings *eb = &ebs[j];
		bitcnt += eb->bitshift;
		if (eb->type == EXPANSIONBOARD_STRING) {
			if (settingstring) {
				TCHAR tmp[MAX_DPATH];
				const TCHAR *p = settingstring;
				for (int i = 0; i < sstr; i++) {
					p += _tcslen(p) + 1;
				}
				if (buf[0])
					_tcscat(buf, _T(","));
				TCHAR *cs = cfgfile_escape_min(p);
				_sntprintf(tmp, sizeof tmp, _T("%s=%s"), eb->configname, cs);
				_tcscat(buf, tmp);
				xfree(cs);
				sstr++;
			}
		} else if (eb->type == EXPANSIONBOARD_MULTI) {
			int itemcnt = -1;
			const TCHAR *p = eb->configname;
			while (p[0]) {
				itemcnt++;
				p += _tcslen(p) + 1;
			}
			int cnt = 1;
			int bits = 1;
			for (int i = 0; i < 8; i++) {
				if ((1 << i) >= itemcnt) {
					cnt = 1 << i;
					bits = i;
					break;
				}
			}
			int multimask = cnt - 1;
			int multivalue = settings;
			if (eb->invert)
				multivalue ^= 0x7fffffff;
			multivalue = (multivalue >> bitcnt) & multimask;
			p = eb->configname;
			while (multivalue >= 0) {
				multivalue--;
				p += _tcslen(p) + 1;
			}
			if (buf[0])
				_tcscat(buf, _T(","));
			_tcscat(buf, p);
			bitcnt += bits;
		} else {
			int value = settings;
			if (eb->invert)
				value ^= 0x7fffffff;
			if (value & (1 << bitcnt)) {
				if (buf[0])
					_tcscat(buf, _T(","));
				_tcscat(buf, eb->configname);
			}
			bitcnt++;
		}
	}
}

static bool is_custom_romboard(struct boardromconfig *br)
{
	return ((br->device_type & ROMTYPE_MASK) == ROMTYPE_UAEBOARDZ2 || (br->device_type & ROMTYPE_MASK) == ROMTYPE_UAEBOARDZ3);
}

static void cfgfile_write_board_rom(struct uae_prefs *prefs, struct zfile *f, struct multipath *mp, struct boardromconfig *br)
{
	TCHAR buf[MAX_DPATH];
	TCHAR name[256];

	if (br->device_type == 0)
		return;
	const auto* ert = get_device_expansion_rom(br->device_type);
	if (!ert)
		return;
	for (int i = 0; i < MAX_BOARD_ROMS; i++) {
		struct romconfig *rc = &br->roms[i];
		if (br->device_num == 0)
			_tcscpy(name, ert->name);
		else
			_sntprintf(name, sizeof name, _T("%s-%d"), ert->name, br->device_num + 1);
		if (i == 0 || _tcslen(br->roms[i].romfile)) {
			_sntprintf(buf, sizeof buf, _T("%s%s_rom_file"), name, i ? _T("_ext") : _T(""));
			cfgfile_write_rom (f, mp, br->roms[i].romfile, buf);
			if (rc->romident[0]) {
				_sntprintf(buf, sizeof buf, _T("%s%s_rom"), name, i ? _T("_ext") : _T(""));
				cfgfile_dwrite_str (f, buf, rc->romident);
			}
			if (rc->autoboot_disabled || rc->dma24bit || rc->inserted || ert->subtypes ||
				ert->settings || ert->id_jumper || br->device_order > 0 || is_custom_romboard(br)) {
				TCHAR buf2[256];
				buf2[0] = 0;
				auto* p = buf2;
				_sntprintf(buf, sizeof buf, _T("%s%s_rom_options"), name, i ? _T("_ext") : _T(""));
				if (ert->subtypes) {
					const auto* srt = ert->subtypes;
					int k = rc->subtype;
					while (k && srt[1].name) {
						srt++;
						k--;
					}
					_sntprintf(p, sizeof p, _T("subtype=%s"), srt->configname);
				}
				if (br->device_order > 0 && prefs->autoconfig_custom_sort) {
					if (buf2[0])
						_tcscat(buf2, _T(","));
					TCHAR *p2 = buf2 + _tcslen(buf2);
					_sntprintf(p2, sizeof p2, _T("order=%d"), br->device_order);
				}
				if (rc->autoboot_disabled) {
					if (buf2[0])
						_tcscat(buf2, _T(","));
					_tcscat(buf2, _T("autoboot_disabled=true"));
				}
				if (rc->dma24bit) {
					if (buf2[0])
						_tcscat(buf2, _T(","));
					_tcscat(buf2, _T("dma24bit=true"));
				}
				if (rc->inserted) {
					if (buf2[0])
						_tcscat(buf2, _T(","));
					_tcscat(buf2, _T("inserted=true"));
				}
				if (ert->id_jumper) {
					TCHAR tmp[256];
					_sntprintf(tmp, sizeof tmp, _T("id=%d"), rc->device_id);
					if (buf2[0])
						_tcscat(buf2, _T(","));
					_tcscat(buf2, tmp);
				}
				if ((rc->device_settings || rc->configtext[0]) && ert->settings) {
					cfgfile_write_rom_settings(ert->settings, buf2, rc->device_settings, rc->configtext);
				}
				if (is_custom_romboard(br)) {
					if (rc->manufacturer) {
						if (buf2[0])
							_tcscat(buf2, _T(","));
						TCHAR *p2 = buf2 + _tcslen(buf2);
						_sntprintf(p2, sizeof p2, _T("mid=%u,pid=%u"), rc->manufacturer, rc->product);
					}
					if (rc->autoconfig[0]) {
						uae_u8 *ac = rc->autoconfig;
						if (buf2[0])
							_tcscat(buf2, _T(","));
						TCHAR *p2 = buf2 + _tcslen(buf2);
						_sntprintf(p2, sizeof p2, _T("data=%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x"),
							ac[0], ac[1], ac[2], ac[3], ac[4], ac[5], ac[6], ac[7],
							ac[8], ac[9], ac[10], ac[11], ac[12], ac[13], ac[14], ac[15]);
					}
				}
				if (buf2[0])
					cfgfile_dwrite_str (f, buf, buf2);
			}

			if (rc->board_ram_size) {
				_sntprintf(buf, sizeof buf, _T("%s%s_mem_size"), name, i ? _T("_ext") : _T(""));
				cfgfile_write(f, buf, _T("%d"), rc->board_ram_size / 0x40000);
			}
		}
	}
}

static bool cfgfile_readromboard(const TCHAR *option, const TCHAR *value, struct romboard *rbp)
{
	TCHAR tmp1[MAX_DPATH];
	for (int i = 0; i < MAX_ROM_BOARDS; i++) {
		struct romboard *rb = &rbp[i];
		if (i > 0)
			_sntprintf(tmp1, sizeof tmp1, _T("romboard%d_options"), i + 1);
		else
			_tcscpy(tmp1, _T("romboard_options"));
		if (!_tcsicmp(option, tmp1)) {
			TCHAR *endptr;
			auto* s1 = cfgfile_option_get(value, _T("start"));
			auto* s2 = cfgfile_option_get(value, _T("end"));
			rb->size = 0;
			if (s1 && s2) {
				rb->start_address = _tcstol(s1, &endptr, 16);
				rb->end_address = _tcstol(s2, &endptr, 16);
				if (rb->end_address && rb->end_address > rb->start_address) {
					rb->size = (rb->end_address - rb->start_address + 65535) & ~65535;
				}
			}
			xfree(s1);
			xfree(s2);
			s1 = cfgfile_option_get(value, _T("file"));
			if (s1) {
				TCHAR *p = cfgfile_unescape(s1, nullptr);
				_tcscpy(rb->lf.loadfile, p);
				xfree(p);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("offset"));
			if (s1) {
				rb->lf.loadoffset = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("fileoffset"));
			if (s1) {
				rb->lf.fileoffset = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("filesize"));
			if (s1) {
				rb->lf.filesize = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			return true;
		}
	}
	return false;
}

static bool cfgfile_readramboard(const TCHAR *option, const TCHAR *value, const TCHAR *name, struct ramboard *rbp)
{
	TCHAR tmp1[MAX_DPATH];
	int v;
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		struct ramboard *rb = &rbp[i];
		if (i > 0)
			_sntprintf(tmp1, sizeof tmp1, _T("%s%d_size"), name, i + 1);
		else
			_sntprintf(tmp1, sizeof tmp1, _T("%s_size"), name);
		if (!_tcsicmp(option, tmp1)) {
			v = 0;
			if (!_tcsicmp(tmp1, _T("chipmem_size"))) {
				rb->chipramtiming = true;
				return false;
			}
			if (!_tcsicmp(tmp1, _T("bogomem_size"))) {
				rb->chipramtiming = true;
			}
			cfgfile_intval(option, value, tmp1, &v, 0x100000);
			rb->size = v;
			return true;
		}
		if (i > 0)
			_sntprintf(tmp1, sizeof tmp1, _T("%s%d_size_k"), name, i + 1);
		else
			_sntprintf(tmp1, sizeof tmp1, _T("%s_size_k"), name);
		if (!_tcsicmp(option, tmp1)) {
			v = 0;
			cfgfile_intval(option, value, tmp1, &v, 1024);
			rb->size = v;
			return true;
		}
		if (i > 0)
			_sntprintf(tmp1, sizeof tmp1, _T("%s%d_options"), name, i + 1);
		else
			_sntprintf(tmp1, sizeof tmp1, _T("%s_options"), name);
		if (!_tcsicmp(option, tmp1)) {
			TCHAR *endptr;
			TCHAR *s, *s1, *s2;
			s = cfgfile_option_get(value, _T("order"));
			if (s)
				rb->device_order = _tstol(s);
			xfree(s);
			s = cfgfile_option_get(value, _T("mid"));
			if (s)
				rb->manufacturer = static_cast<uae_u16>(_tstol(s));
			xfree(s);
			s = cfgfile_option_get(value, _T("pid"));
			if (s)
				rb->product = static_cast<uae_u8>(_tstol(s));
			xfree(s);
			s = cfgfile_option_get(value, _T("fault"));
			if (s)
				rb->fault = _tstol(s);
			xfree(s);
			if (cfgfile_option_get_bool(value, _T("no_reset_unmap")))
				rb->no_reset_unmap = true;
			if (cfgfile_option_get_bool(value, _T("nodma")))
				rb->nodma = true;
			if (cfgfile_option_get_bool(value, _T("force16bit")))
				rb->force16bit = true;
			if (cfgfile_option_get_bool(value, _T("slow")))
				rb->chipramtiming = true;
			else if (cfgfile_option_get_nbool(value, _T("slow")))
				rb->chipramtiming = false;
			s = cfgfile_option_get(value, _T("data"));
			if (s && _tcslen(s) >= 3 * 16 - 1) {
				rb->autoconfig_inuse = true;
				for (int j = 0; j < sizeof rb->autoconfig; j++) {
					s2 = &s[j * 3];
					if (j + 1 < sizeof rb->autoconfig && s2[2] != '.')
						break;
					s[2] = 0;
					rb->autoconfig[j] = static_cast<uae_u8>(_tcstol(s2, &endptr, 16));
				}
			}
			xfree(s);
			s1 = cfgfile_option_get(value, _T("start"));
			s2 = cfgfile_option_get(value, _T("end"));
			if (s1 && s2) {
				rb->start_address = _tcstol(s1, &endptr, 16);
				rb->end_address = _tcstol(s2, &endptr, 16);
				if (rb->start_address && rb->end_address > rb->start_address) {
					rb->manual_config = true;
					rb->autoconfig_inuse = false;
				}
			}
			xfree(s1);
			xfree(s2);
			s1 = cfgfile_option_get(value, _T("write_address"));
			if (s1) {
				rb->write_address = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("file"));
			if (s1) {
				const auto p = cfgfile_unescape(s1, nullptr);
				_tcscpy(rb->lf.loadfile, p);
				xfree(p);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("offset"));
			if (s1) {
				rb->lf.loadoffset = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("fileoffset"));
			if (s1) {
				rb->lf.fileoffset = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			s1 = cfgfile_option_get(value, _T("filesize"));
			if (s1) {
				rb->lf.filesize = _tcstol(s1, &endptr, 16);
			}
			xfree(s1);
			rb->readonly = cfgfile_option_find(value, _T("read-only"));
			return true;
		}
	}
	return false;
}

static void cfgfile_writeromboard(struct uae_prefs *prefs, struct zfile *f, int num, struct romboard *rb)
{
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	if (!rb->end_address)
		return;
	if (num > 0)
		_sntprintf(tmp1, sizeof tmp1, _T("romboard%d_options"), num + 1);
	else
		_tcscpy(tmp1, _T("romboard_options"));
	tmp2[0] = 0;
	TCHAR *p = tmp2;
	_sntprintf(p, sizeof p, _T("start=%08x,end=%08x"), rb->start_address, rb->end_address);
	p += _tcslen(p);
	if (rb->lf.loadfile[0]) {
		_tcscat(p, _T(","));
		p += _tcslen(p);
		TCHAR *path = cfgfile_escape(rb->lf.loadfile, nullptr, true, false);
		if (rb->lf.loadoffset || rb->lf.fileoffset || rb->lf.filesize) {
			_sntprintf(p, sizeof p, _T("offset=%u,fileoffset=%u,filesize=%u,"), rb->lf.loadoffset, rb->lf.fileoffset, rb->lf.filesize);
			p += _tcslen(p);
		}
		_sntprintf(p, sizeof p, _T("file=%s"), path);
		xfree(path);
	}
	if (tmp2[0]) {
		cfgfile_write(f, tmp1, tmp2);
	}
}

static void cfgfile_writeramboard(struct uae_prefs *prefs, struct zfile *f, const TCHAR *name, int num, struct ramboard *rb)
{
	TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
	if (num > 0)
		_sntprintf(tmp1, sizeof tmp1, _T("%s%d_options"), name, num + 1);
	else
		_sntprintf(tmp1, sizeof tmp1, _T("%s_options"), name);
	tmp2[0] = 0;
	TCHAR *p = tmp2;
	if (rb->device_order > 0 && prefs->autoconfig_custom_sort) {
		if (tmp2[0])
			*p++ = ',';
		_sntprintf(p, sizeof p, _T("order=%d"), rb->device_order);
		p += _tcslen(p);
	}
	if (rb->manufacturer) {
		if (tmp2[0])
			*p++ = ',';
		_sntprintf(p, sizeof p, _T("mid=%u,pid=%u"), rb->manufacturer, rb->product);
		p += _tcslen(p);
	}
	if (rb->no_reset_unmap) {
		if (tmp2[0])
			*p++ = ',';
		_tcscpy(p, _T("no_reset_unmap=true"));
		p += _tcslen(p);
	}
	if (rb->nodma) {
		if (tmp2[0])
			*p++ = ',';
		_tcscpy(p, _T("nodma=true"));
		p += _tcslen(p);
	}
	if (rb->force16bit) {
		if (tmp2[0])
			*p++ = ',';
		_tcscpy(p, _T("force16bit=true"));
		p += _tcslen(p);
	}
	if (rb->fault) {
		if (tmp2[0])
			*p++ = ',';
		_sntprintf(p, sizeof p, _T("fault=%d"), rb->fault);
		p += _tcslen(p);
	}
	if (!_tcsicmp(tmp1, _T("chipmem_options")) || !_tcsicmp(tmp1, _T("bogomem_options"))) {
		if (!rb->chipramtiming) {
			if (tmp2[0])
				*p++ = ',';
			_tcscpy(p, _T("slow=false"));
			p += _tcslen(p);
		}
	} else {
		if (rb->chipramtiming) {
			if (tmp2[0])
				*p++ = ',';
			_tcscpy(p, _T("slow=true"));
			p += _tcslen(p);
		}
	}
	if (rb->autoconfig_inuse) {
		uae_u8 *ac = rb->autoconfig;
		if (tmp2[0])
			*p++ = ',';
		_sntprintf(p, sizeof p, _T("data=%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x"),
			ac[0], ac[1], ac[2], ac[3], ac[4], ac[5], ac[6], ac[7],
			ac[8], ac[9], ac[10], ac[11], ac[12], ac[13], ac[14], ac[15]);
		p += _tcslen(p);
	}
	if (rb->manual_config && rb->start_address && rb->end_address) {
		if (tmp2[0])
			*p++ = ',';
		_sntprintf(p, sizeof p, _T("start=%08x,end=%08x"), rb->start_address, rb->end_address);
		p += _tcslen(p);
	}
	if (rb->write_address) {
		_sntprintf(p, sizeof p, _T(",write_address=%08x"), rb->write_address);
		p += _tcslen(p);
	}
	if (rb->lf.loadfile[0]) {
		if (tmp2[0]) {
			_tcscat(p, _T(","));
			p += _tcslen(p);
		}
		TCHAR *path = cfgfile_escape(rb->lf.loadfile, nullptr, true, false);
		_sntprintf(p, sizeof p, _T("offset=%u,fileoffset=%u,filesize=%u,file=%s"), rb->lf.loadoffset, rb->lf.fileoffset, rb->lf.filesize, path);
		xfree(path);
	}
	if (rb->readonly) {
		if (tmp2[0])
			_tcscat(p, _T(","));
		_tcscat(p, _T("readonly"));
	}
	if (tmp2[0]) {
		cfgfile_write(f, tmp1, tmp2);
	}
}

void cfgfile_save_options (struct zfile *f, struct uae_prefs *p, int type)
{
	struct strlist *sl;
	TCHAR tmp[MAX_DPATH];
	int i;

	cfgfile_write_str(f, _T("config_description"), p->description);
	if (p->category[0])
		cfgfile_write_str(f, _T("config_category"), p->category);
	if (p->tags[0])
		cfgfile_write_str(f, _T("config_tags"), p->tags);
	cfgfile_write_bool (f, _T("config_hardware"), type & CONFIG_TYPE_HARDWARE);
	cfgfile_write_bool (f, _T("config_host"), !!(type & CONFIG_TYPE_HOST));
	if (p->info[0])
		cfgfile_write (f, _T("config_info"), p->info);
	cfgfile_write (f, _T("config_version"), _T("%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
	cfgfile_write_str (f, _T("config_hardware_path"), p->config_hardware_path);
	cfgfile_write_str (f, _T("config_host_path"), p->config_host_path);
	cfgfile_write_str (f, _T("config_all_path"), p->config_all_path);
	if (p->config_window_title[0])
		cfgfile_write_str (f, _T("config_window_title"), p->config_window_title);

	for (sl = p->all_lines; sl; sl = sl->next) {
		if (sl->unknown) {
			if (sl->option)
				cfgfile_write_str (f, sl->option, sl->value);
		}
	}

	for (i = 0; i < MAX_PATHS; i++) {
		if (p->path_rom.path[i][0]) {
			_sntprintf (tmp, sizeof tmp, _T("%s.rom_path"), TARGET_NAME);
			cfgfile_write_str (f, tmp, p->path_rom.path[i]);
		}
	}
	for (i = 0; i < MAX_PATHS; i++) {
		if (p->path_floppy.path[i][0]) {
			_sntprintf (tmp, sizeof tmp, _T("%s.floppy_path"), TARGET_NAME);
			cfgfile_write_str (f, tmp, p->path_floppy.path[i]);
		}
	}
	for (i = 0; i < MAX_PATHS; i++) {
		if (p->path_hardfile.path[i][0]) {
			_sntprintf (tmp, sizeof tmp, _T("%s.hardfile_path"), TARGET_NAME);
			cfgfile_write_str (f, tmp, p->path_hardfile.path[i]);
		}
	}
	for (i = 0; i < MAX_PATHS; i++) {
		if (p->path_cd.path[i][0]) {
			_sntprintf (tmp, sizeof tmp, _T("%s.cd_path"), TARGET_NAME);
			cfgfile_write_str (f, tmp, p->path_cd.path[i]);
		}
	}

	cfg_write (_T("; host-specific"), f);

	target_save_options (f, p);

	cfg_write (_T("; common"), f);

	cfgfile_write_strarr (f, _T("use_gui"), guimode1, p->start_gui);
	cfgfile_write_bool (f, _T("use_debugger"), p->start_debugger);
	cfgfile_write_multichoice(f, _T("debugging_features"), debugfeatures, p->debugging_features);
	cfgfile_dwrite_str(f, _T("debugging_options"), p->debugging_options);

	cfgfile_write_rom (f, &p->path_rom, p->romfile, _T("kickstart_rom_file"));
	cfgfile_write_rom (f, &p->path_rom, p->romextfile, _T("kickstart_ext_rom_file"));
	if (p->romextfile2addr) {
		cfgfile_write (f, _T("kickstart_ext_rom_file2_address"), _T("%x"), p->romextfile2addr);
		cfgfile_write_rom (f, &p->path_rom, p->romextfile2, _T("kickstart_ext_rom_file2"));
	}
	if (p->romident[0])
		cfgfile_dwrite_str (f, _T("kickstart_rom"), p->romident);
	if (p->romextident[0])
		cfgfile_write_str (f, _T("kickstart_ext_rom="), p->romextident);

	for (int rb = 0; i < MAX_ROM_BOARDS; i++) {
		cfgfile_writeromboard(p, f, rb, &p->romboards[rb]);
	}

	for (auto & eb : p->expansionboard) {
		cfgfile_write_board_rom(p, f, &p->path_rom, &eb);
	}

	cfgfile_write_path2(f, _T("flash_file"), p->flashfile, PATH_ROM);
	cfgfile_write_path (f, &p->path_rom, _T("cart_file"), p->cartfile);
	cfgfile_write_path2(f, _T("rtc_file"), p->rtcfile, PATH_ROM);
	if (p->cartident[0])
		cfgfile_write_str (f, _T("cart"), p->cartident);
	cfgfile_dwrite_path (f, &p->path_rom, _T("picassoiv_rom_file"), p->picassoivromfile);

	cfgfile_write_bool(f, _T("kickshifter"), p->kickshifter);
	cfgfile_write_bool(f, _T("scsidevice_disable"), p->scsidevicedisable);
	cfgfile_dwrite_bool(f, _T("ks_write_enabled"), p->rom_readwrite);

	cfgfile_write (f, _T("floppy_volume"), _T("%d"), p->dfxclickvolume_disk[0]);
	p->nr_floppies = 4;
	for (i = 0; i < 4; i++) {
		_sntprintf (tmp, sizeof tmp, _T("floppy%d"), i);
		cfgfile_write_path2(f, tmp, p->floppyslots[i].df, PATH_FLOPPY);
		_sntprintf (tmp, sizeof tmp, _T("floppy%dwp"), i);
		cfgfile_dwrite_bool (f, tmp, p->floppyslots[i].forcedwriteprotect);
		_sntprintf(tmp, sizeof tmp, _T("floppy%dtype"), i);
		cfgfile_dwrite(f, tmp, _T("%d"), p->floppyslots[i].dfxtype);
#ifdef FLOPPYBRIDGE
		// Check if the dfxtype is a FloppyBridge option, then always save subtype and subtypeid
		// This ensures that when we load a config with these options, the DrawBridge will get initialized
		if (p->floppyslots[i].dfxsubtype || p->floppyslots[i].dfxtype == DRV_FB) {
			_sntprintf(tmp, sizeof tmp, _T("floppy%dsubtype"), i);
			cfgfile_dwrite(f, tmp, _T("%d"), p->floppyslots[i].dfxsubtype);
			if (p->floppyslots[i].dfxsubtypeid[0] || p->floppyslots[i].dfxtype == DRV_FB) {
				_sntprintf(tmp, sizeof tmp, _T("floppy%dsubtypeid"), i);
				cfgfile_dwrite_escape(f, tmp, _T("%s"), p->floppyslots[i].dfxsubtypeid);
			}
			if (p->floppyslots[i].dfxprofile[0]) {
				_sntprintf(tmp, sizeof tmp, _T("floppy%dprofile"), i);
				cfgfile_dwrite_escape(f, tmp, _T("%s"), p->floppyslots[i].dfxprofile);
			}
		}
#endif
		_sntprintf (tmp, sizeof tmp, _T("floppy%dsound"), i);
		cfgfile_dwrite (f, tmp, _T("%d"), p->floppyslots[i].dfxclick);
		if (p->floppyslots[i].dfxclick < 0 && p->floppyslots[i].dfxclickexternal[0]) {
			_sntprintf (tmp, sizeof tmp, _T("floppy%dsoundext"), i);
			cfgfile_dwrite (f, tmp, p->floppyslots[i].dfxclickexternal);
		}
		if (p->floppyslots[i].dfxclick) {
			_sntprintf (tmp, sizeof tmp, _T("floppy%dsoundvolume_disk"), i);
			cfgfile_write (f, tmp, _T("%d"), p->dfxclickvolume_disk[i]);
			_sntprintf (tmp, sizeof tmp, _T("floppy%dsoundvolume_empty"), i);
			cfgfile_write (f, tmp, _T("%d"), p->dfxclickvolume_empty[i]);
		}
		if (p->floppyslots[i].dfxtype < 0 && p->nr_floppies > i)
			p->nr_floppies = i;
	}
	for (i = 0; i < MAX_SPARE_DRIVES; i++) {
		if (p->dfxlist[i][0]) {
			_sntprintf (tmp, sizeof tmp, _T("diskimage%d"), i);
			cfgfile_dwrite_path2(f, tmp, p->dfxlist[i], PATH_FLOPPY);
		}
	}

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		if (p->cdslots[i].name[0] || p->cdslots[i].inuse) {
			TCHAR tmp2[MAX_DPATH];
			_sntprintf (tmp, sizeof tmp, _T("cdimage%d"), i);
			cfgfile_to_path_save(p->cdslots[i].name, tmp2, PATH_CD);
			if (p->cdslots[i].type != SCSI_UNIT_DEFAULT || _tcschr (p->cdslots[i].name, ',') || p->cdslots[i].delayed) {
				//for escaping some day..
				//TCHAR *n = cfgfile_escape(tmp2, _T(","), true, true);
				//_tcscpy(tmp2, n);
				//xfree(n);
				_tcscat(tmp2, _T(","));
				if (p->cdslots[i].delayed) {
					_tcscat(tmp2, _T("delay"));
					_tcscat(tmp2, _T(":"));
				}
				if (p->cdslots[i].type != SCSI_UNIT_DEFAULT) {
					_tcscat (tmp2, cdmodes[p->cdslots[i].type + 1]);
				}
			}
			cfgfile_write_str (f, tmp, tmp2);
		}
	}

#ifdef WITH_LUA
	for (i = 0; i < MAX_LUA_STATES; i++) {
		if (p->luafiles[i][0]) {
			cfgfile_write_path2(f, _T("lua"), p->luafiles[i], PATH_NONE);
		}
	}
#endif

	if (p->trainerfile[0])
		cfgfile_write_path2(f, _T("trainerfile"), p->trainerfile, PATH_NONE);

	if (p->statefile[0])
		cfgfile_write_path2(f, _T("statefile"), p->statefile, PATH_NONE);
	if (p->quitstatefile[0])
		cfgfile_write_path2(f, _T("statefile_quit"), p->quitstatefile, PATH_NONE);
	if (p->statefile_path[0])
		cfgfile_dwrite_path2(f, _T("statefile_path"), p->statefile_path, PATH_NONE);

	cfgfile_write (f, _T("nr_floppies"), _T("%d"), p->nr_floppies);
	cfgfile_dwrite_bool (f, _T("floppy_write_protect"), p->floppy_read_only);
	cfgfile_write (f, _T("floppy_speed"), _T("%d"), p->floppy_speed);
	cfgfile_dwrite (f, _T("floppy_channel_mask"), _T("0x%x"), p->dfxclickchannelmask);
	cfgfile_write (f, _T("cd_speed"), _T("%d"), p->cd_speed);
	cfgfile_write_bool (f, _T("parallel_on_demand"), p->parallel_demand);
	if (p->sername[0])
		cfgfile_write_str (f, _T("serial_port"), p->sername);
	cfgfile_write_bool (f, _T("serial_on_demand"), p->serial_demand);
	cfgfile_write_bool(f, _T("serial_hardware_ctsrts"), p->serial_hwctsrts);
	cfgfile_write_bool(f, _T("serial_status"), p->serial_rtsctsdtrdtecd);
	cfgfile_dwrite_bool(f, _T("serial_ri"), p->serial_ri);
	cfgfile_write_bool (f, _T("serial_direct"), p->serial_direct);
	cfgfile_dwrite (f, _T("serial_stopbits"), _T("%d"), p->serial_stopbits);
	cfgfile_dwrite_strarr(f, _T("serial_translate"), serialcrlf, p->serial_crlf);
	cfgfile_write_strarr (f, _T("scsi"), scsimode, p->scsi);
	cfgfile_write_bool (f, _T("uaeserial"), p->uaeserial);
	cfgfile_write_bool (f, _T("sana2"), p->sana2);

	cfgfile_write_strarr(f, _T("sound_output"), soundmode1, p->produce_sound);
	cfgfile_write_strarr(f, _T("sound_channels"), stereomode, p->sound_stereo);
	cfgfile_write (f, _T("sound_stereo_separation"), _T("%d"), p->sound_stereo_separation);
	cfgfile_write (f, _T("sound_stereo_mixing_delay"), _T("%d"), p->sound_mixed_stereo_delay >= 0 ? p->sound_mixed_stereo_delay : 0);
	cfgfile_write (f, _T("sound_max_buff"), _T("%d"), p->sound_maxbsiz);
	cfgfile_write (f, _T("sound_frequency"), _T("%d"), p->sound_freq);
	cfgfile_write_strarr (f, _T("sound_interpol"), interpolmode, p->sound_interpol);
	cfgfile_write_strarr (f, _T("sound_filter"), soundfiltermode1, p->sound_filter);
	cfgfile_write_strarr (f, _T("sound_filter_type"), soundfiltermode2, p->sound_filter_type);
	cfgfile_write (f, _T("sound_volume"), _T("%d"), p->sound_volume_master);
	cfgfile_write (f, _T("sound_volume_paula"), _T("%d"), p->sound_volume_paula);
	if (p->sound_volume_cd >= 0)
		cfgfile_write (f, _T("sound_volume_cd"), _T("%d"), p->sound_volume_cd);
	if (p->sound_volume_board >= 0)
		cfgfile_write (f, _T("sound_volume_ahi"), _T("%d"), p->sound_volume_board);
	if (p->sound_volume_midi >= 0)
		cfgfile_write (f, _T("sound_volume_midi"), _T("%d"), p->sound_volume_midi);
	if (p->sound_volume_genlock >= 0)
		cfgfile_write (f, _T("sound_volume_genlock"), _T("%d"), p->sound_volume_genlock);
	cfgfile_write_bool (f, _T("sound_auto"), p->sound_auto);
	cfgfile_write_bool (f, _T("sound_stereo_swap_paula"), p->sound_stereo_swap_paula);
	cfgfile_write_bool (f, _T("sound_stereo_swap_ahi"), p->sound_stereo_swap_ahi);
	cfgfile_dwrite_bool(f, _T("sound_volcnt"), p->sound_volcnt);
	cfgfile_dwrite (f, _T("sampler_frequency"), _T("%d"), p->sampler_freq);
	cfgfile_dwrite (f, _T("sampler_buffer"), _T("%d"), p->sampler_buffer);
	cfgfile_dwrite_bool (f, _T("sampler_stereo"), p->sampler_stereo);

	cfgfile_write_strarr(f, _T("comp_trustbyte"), compmode, p->comptrustbyte);
	cfgfile_write_strarr(f, _T("comp_trustword"), compmode, p->comptrustword);
	cfgfile_write_strarr(f, _T("comp_trustlong"), compmode, p->comptrustlong);
	cfgfile_write_strarr(f, _T("comp_trustnaddr"), compmode, p->comptrustnaddr);
	cfgfile_write_bool (f, _T("comp_nf"), p->compnf);
	cfgfile_write_bool (f, _T("comp_constjump"), p->comp_constjump);
	cfgfile_write_strarr(f, _T("comp_flushmode"), flushmode, p->comp_hardflush);
#ifdef USE_JIT_FPU
	cfgfile_write_bool (f, _T("compfpu"), p->compfpu);
#endif
	cfgfile_write_bool(f, _T("comp_catchfault"), p->comp_catchfault);
	cfgfile_write(f, _T("cachesize"), _T("%d"), p->cachesize);
	cfgfile_dwrite_str(f, _T("jit_blacklist"), p->jitblacklist);
	cfgfile_dwrite_bool(f, _T("jit_inhibit"), p->cachesize_inhibit);

	for (i = 0; i < MAX_JPORTS; i++) {
		struct jport *jp = &p->jports[i];
		int v = jp->id;
		TCHAR tmp1[MAX_DPATH], tmp2[MAX_DPATH];
		if (v == JPORT_NONE) {
			_tcscpy (tmp2, _T("none"));
		} else if (v < JSEM_CUSTOM) {
			_sntprintf(tmp2, sizeof tmp2, _T("kbd%d"), v + 1);
		} else if (v < JSEM_JOYS) {
			_sntprintf(tmp2, sizeof tmp2, _T("custom%d"), v - JSEM_CUSTOM);
		} else if (v < JSEM_MICE) {
			_sntprintf (tmp2, sizeof tmp2, _T("joy%d"), v - JSEM_JOYS);
		} else {
			_tcscpy (tmp2, _T("mouse"));
			if (v - JSEM_MICE > 0)
				_sntprintf (tmp2, sizeof tmp2, _T("mouse%d"), v - JSEM_MICE);
		}
		if (i < 2 || jp->id >= 0) {
			_sntprintf (tmp1, sizeof tmp1, _T("joyport%d"), i);
			cfgfile_write (f, tmp1, tmp2);
			_sntprintf (tmp1, sizeof tmp1, _T("joyport%dautofire"), i);
			cfgfile_write_strarr(f, tmp1, joyaf, jp->autofire);
			if (i < 2 && jp->mode > 0) {
				_sntprintf (tmp1, sizeof tmp1, _T("joyport%dmode"), i);
				cfgfile_write_strarr(f, tmp1, joyportmodes, jp->mode);
				if (jp->submode > 0 && jp->mode == 8) {
					_sntprintf(tmp1, sizeof tmp1, _T("joyport%dsubmode"), i);
					cfgfile_write_strarr(f, tmp1, joyportsubmodes_lightpen, jp->submode);
				}
			}
#ifdef AMIBERRY
			if (jp->mousemap > 0) {
				_sntprintf(tmp1, sizeof tmp1, _T("joyport%dmousemap"), i);
				cfgfile_write(f, tmp1, _T("%d"), jp->mousemap);
			}
#endif
			if (jp->idc.name[0]) {
				_sntprintf (tmp1, sizeof tmp1, _T("joyportfriendlyname%d"), i);
				cfgfile_write (f, tmp1, jp->idc.name);
			}
			if (jp->idc.configname[0]) {
				_sntprintf (tmp1, sizeof tmp1, _T("joyportname%d"), i);
				cfgfile_write (f, tmp1, jp->idc.configname);
			}
			if (jp->nokeyboardoverride) {
				_sntprintf (tmp1, sizeof tmp1, _T("joyport%dkeyboardoverride"), i);
				cfgfile_write_bool (f, tmp1, !jp->nokeyboardoverride);
			}
		}
#ifdef AMIBERRY
		// custom controls SAVING
		std::string mode;
		std::string buffer;

		const auto host_joy_id = jp->id - JSEM_JOYS;
		// skip non-joystick ports
		if (host_joy_id < 0 || host_joy_id > MAX_INPUT_DEVICES)
			continue;

		didata* did = &di_joystick[host_joy_id];

		for (int m = 0; m < 2; ++m)
		{
			mode = m == 0 ? "none" : "hotkey";
			for (int n = 0; n < SDL_CONTROLLER_BUTTON_MAX; ++n) // loop through all buttons
			{
				buffer = "joyport" + std::to_string(i) + "_amiberry_custom_" + mode + "_" + SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(n));
				const auto b = m == 0 ? did->mapping.amiberry_custom_none[n] : did->mapping.amiberry_custom_hotkey[n];

				_tcscpy(tmp2, b > 0 ? _T(find_inputevent_name(b)) : _T(""));
				cfgfile_dwrite_str(f, buffer.c_str(), tmp2);
			}

			for (int n = 0; n < SDL_CONTROLLER_AXIS_MAX; ++n)
			{
				buffer = "joyport" + std::to_string(i) + "_amiberry_custom_axis_" + mode + "_" + SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(n));
				const auto b = m == 0 ? did->mapping.amiberry_custom_axis_none[n] : did->mapping.amiberry_custom_axis_hotkey[n];

				_tcscpy(tmp2, b > 0 ? _T(find_inputevent_name(b)) : _T(""));
				cfgfile_dwrite_str(f, buffer.c_str(), tmp2);
			}
		}
#endif
	}
	for (i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		struct jport_custom *jp = &p->jports_custom[i];
		if (jp->custom[0]) {
			TCHAR tmp1[MAX_DPATH];
			_sntprintf(tmp1, sizeof tmp1, _T("joyportcustom%d"), i);
			cfgfile_write(f, tmp1, jp->custom);
		}
	}

	if (p->dongle) {
		if (p->dongle + 1 >= sizeof (dongles) / sizeof (TCHAR*))
			cfgfile_write (f, _T("dongle"), _T("%d"), p->dongle);
		else
			cfgfile_write_strarr(f, _T("dongle"), dongles, p->dongle);
	}

	cfgfile_write_bool (f, _T("bsdsocket_emu"), p->socket_emu);

	{
		// backwards compatibility
		const TCHAR *name;
		struct romconfig *rc;
		rc = get_device_romconfig(p, ROMTYPE_A2065, 0);
		if (rc) {
			name = ethernet_getselectionname(rc ? rc->device_settings : 0);
			cfgfile_write_str(f, _T("a2065"), name);
		}
		rc = get_device_romconfig(p, ROMTYPE_NE2KPCMCIA, 0);
		if (rc) {
			name = ethernet_getselectionname(rc ? rc->device_settings : 0);
			cfgfile_write_str(f, _T("ne2000_pcmcia"), name);
		}
		rc = get_device_romconfig(p, ROMTYPE_NE2KPCI, 0);
		if (rc) {
			name = ethernet_getselectionname(rc ? rc->device_settings : 0);
			cfgfile_write_str(f, _T("ne2000_pci"), name);
		}
	}

#ifdef WITH_SLIRP
	tmp[0] = 0;
	for (i = 0; i < MAX_SLIRP_REDIRS; i++) {
		struct slirp_redir *sr = &p->slirp_redirs[i];
		if (sr->proto && !sr->srcport) {
			TCHAR *ptmp = tmp + _tcslen (tmp);
			if (ptmp > tmp)
				*ptmp++ = ',';
			_sntprintf (ptmp, sizeof ptmp, _T("%d"), sr->dstport);
		}
	}
	if (tmp[0])
		cfgfile_write_str (f, _T("slirp_ports"), tmp);
	for (i = 0; i < MAX_SLIRP_REDIRS; i++) {
		struct slirp_redir *sr = &p->slirp_redirs[i];
		if (sr->proto && sr->srcport) {
			struct in_addr addr{};
			addr.s_addr = sr->addr;
			const char* addr_str = inet_ntoa(addr);
			if (addr_str) {
				_sntprintf(tmp, sizeof tmp, _T("%s:%d:%d:%s"),
					sr->proto == 1 ? _T("tcp") : _T("udp"),
					sr->dstport, sr->srcport, addr_str);
			}
			else {
				_sntprintf(tmp, sizeof tmp, _T("%s:%d:%d"),
					sr->proto == 1 ? _T("tcp") : _T("udp"),
					sr->dstport, sr->srcport);
			}
			cfgfile_write_str(f, _T("slirp_redir"), tmp);
		}

	}
#endif /* WITH_SLIRP */

	cfgfile_write_bool (f, _T("synchronize_clock"), p->tod_hack);
	cfgfile_write (f, _T("maprom"), _T("0x%x"), p->maprom);
	cfgfile_dwrite_strarr(f, _T("boot_rom_uae"), uaebootrom, p->boot_rom);
	if (p->uaeboard_nodiag)
		cfgfile_write_strarr(f, _T("uaeboard"), uaeboard_off, p->uaeboard);
	else
		cfgfile_dwrite_strarr(f, _T("uaeboard"), uaeboard, p->uaeboard);
	if (p->autoconfig_custom_sort)
		cfgfile_dwrite(f, _T("uaeboard_options"), _T("order=%d"), p->uaeboard_order);
	cfgfile_dwrite_strarr(f, _T("parallel_matrix_emulation"), epsonprinter, p->parallel_matrix_emulation);
	cfgfile_write_bool (f, _T("parallel_postscript_emulation"), p->parallel_postscript_emulation);
	cfgfile_write_bool (f, _T("parallel_postscript_detection"), p->parallel_postscript_detection);
	cfgfile_write_str (f, _T("ghostscript_parameters"), p->ghostscript_parameters);
	cfgfile_write (f, _T("parallel_autoflush"), _T("%d"), p->parallel_autoflush_time);
	cfgfile_dwrite (f, _T("uae_hide"), _T("%d"), p->uae_hide);
	cfgfile_dwrite_bool (f, _T("uae_hide_autoconfig"), p->uae_hide_autoconfig);
	cfgfile_dwrite_bool (f, _T("magic_mouse"), (p->input_mouse_untrap & MOUSEUNTRAP_MAGIC) != 0);
	cfgfile_dwrite_strarr(f, _T("magic_mousecursor"), magiccursors, p->input_magic_mouse_cursor);
	cfgfile_dwrite_strarr(f, _T("absolute_mouse"), abspointers, p->input_tablet);
	cfgfile_dwrite_bool (f, _T("tablet_library"), p->tablet_library);
	cfgfile_dwrite_bool (f, _T("clipboard_sharing"), p->clipboard_sharing);
	cfgfile_dwrite_bool(f, _T("native_code"), p->native_code);
	cfgfile_dwrite_bool(f, _T("cputester"), p->cputester);

	cfgfile_write (f, _T("gfx_display"), _T("%d"), p->gfx_apmode[APMODE_NATIVE].gfx_display);
	cfgfile_write_str (f, _T("gfx_display_friendlyname"), target_get_display_name (p->gfx_apmode[APMODE_NATIVE].gfx_display, true));
	cfgfile_write_str (f, _T("gfx_display_name"), target_get_display_name (p->gfx_apmode[APMODE_NATIVE].gfx_display, false));
	cfgfile_write (f, _T("gfx_display_rtg"), _T("%d"), p->gfx_apmode[APMODE_RTG].gfx_display);
	cfgfile_write_str (f, _T("gfx_display_friendlyname_rtg"), target_get_display_name (p->gfx_apmode[APMODE_RTG].gfx_display, true));
	cfgfile_write_str (f, _T("gfx_display_name_rtg"), target_get_display_name (p->gfx_apmode[APMODE_RTG].gfx_display, false));

	cfgfile_write(f, _T("gfx_framerate"), _T("%d"), p->gfx_framerate);
	write_resolution(f, _T("gfx_width"), _T("gfx_height"), &p->gfx_monitor[0].gfx_size_win); /* compatibility with old versions */
	cfgfile_write(f, _T("gfx_x_windowed"), _T("%d"), p->gfx_monitor[0].gfx_size_win.x);
	cfgfile_write(f, _T("gfx_y_windowed"), _T("%d"), p->gfx_monitor[0].gfx_size_win.y);
	cfgfile_dwrite_bool(f, _T("gfx_resize_windowed"), p->gfx_windowed_resize);
	write_resolution(f, _T("gfx_width_windowed"), _T("gfx_height_windowed"), &p->gfx_monitor[0].gfx_size_win);
	write_resolution(f, _T("gfx_width_fullscreen"), _T("gfx_height_fullscreen"), &p->gfx_monitor[0].gfx_size_fs);
	cfgfile_write(f, _T("gfx_refreshrate"), _T("%d"), p->gfx_apmode[0].gfx_refreshrate);
	cfgfile_dwrite(f, _T("gfx_refreshrate_rtg"), _T("%d"), p->gfx_apmode[1].gfx_refreshrate);

	cfgfile_write (f, _T("gfx_autoresolution"), _T("%d"), p->gfx_autoresolution);
	cfgfile_dwrite (f, _T("gfx_autoresolution_delay"), _T("%d"), p->gfx_autoresolution_delay);
	cfgfile_dwrite_strarr(f, _T("gfx_autoresolution_min_vertical"), vertmode, p->gfx_autoresolution_minv + 1);
	cfgfile_dwrite_strarr(f, _T("gfx_autoresolution_min_horizontal"), horizmode, p->gfx_autoresolution_minh + 1);
	cfgfile_write_bool (f, _T("gfx_autoresolution_vga"), p->gfx_autoresolution_vga);

	cfgfile_write (f, _T("gfx_backbuffers"), _T("%d"), p->gfx_apmode[0].gfx_backbuffers);
	cfgfile_write (f, _T("gfx_backbuffers_rtg"), _T("%d"), p->gfx_apmode[1].gfx_backbuffers);
	if (p->gfx_apmode[APMODE_NATIVE].gfx_interlaced)
		cfgfile_write_bool (f, _T("gfx_interlace"), p->gfx_apmode[APMODE_NATIVE].gfx_interlaced);
	cfgfile_write_strarr(f, _T("gfx_vsync"), vsyncmodes, p->gfx_apmode[0].gfx_vsync);
	cfgfile_write_strarr(f, _T("gfx_vsyncmode"), vsyncmodes2, p->gfx_apmode[0].gfx_vsyncmode);
	cfgfile_write_strarr(f, _T("gfx_vsync_picasso"), vsyncmodes, p->gfx_apmode[1].gfx_vsync);
	cfgfile_write_strarr(f, _T("gfx_vsyncmode_picasso"), vsyncmodes2, p->gfx_apmode[1].gfx_vsyncmode);
	cfgfile_write_bool (f, _T("gfx_lores"), p->gfx_resolution == 0);
	cfgfile_write_strarr(f, _T("gfx_resolution"), lorestype1, p->gfx_resolution);
	cfgfile_write_strarr(f, _T("gfx_lores_mode"), loresmode, p->gfx_lores_mode);
	cfgfile_write_bool (f, _T("gfx_flickerfixer"), p->gfx_scandoubler);
	cfgfile_write_strarr(f, _T("gfx_linemode"), linemode, p->gfx_vresolution > 0 ? p->gfx_iscanlines * 4 + p->gfx_pscanlines + 1 : 0);
	cfgfile_write_strarr(f, _T("gfx_fullscreen_amiga"), fullmodes, p->gfx_apmode[0].gfx_fullscreen);
	cfgfile_write_strarr(f, _T("gfx_fullscreen_picasso"), fullmodes, p->gfx_apmode[1].gfx_fullscreen);
	cfgfile_write_strarr(f, _T("gfx_center_horizontal"), centermode1, p->gfx_xcenter);
	cfgfile_write_strarr(f, _T("gfx_center_vertical"), centermode1, p->gfx_ycenter);
	cfgfile_write_strarr(f, _T("gfx_colour_mode"), colormode1, p->color_mode);
	cfgfile_write_bool(f, _T("gfx_blacker_than_black"), p->gfx_blackerthanblack);
	cfgfile_dwrite_bool(f, _T("gfx_monochrome"), p->gfx_grayscale);
	cfgfile_dwrite_strarr(f, _T("gfx_atari_palette_fix"), threebitcolors, p->gfx_threebitcolors);
	cfgfile_dwrite_bool(f, _T("gfx_black_frame_insertion"), p->lightboost_strobo);
	cfgfile_dwrite(f, _T("gfx_black_frame_insertion_ratio"), _T("%d"), p->lightboost_strobo_ratio);
	cfgfile_write_strarr(f, _T("gfx_api"), filterapi, p->gfx_api);
	cfgfile_dwrite_bool(f, _T("gfx_api_hdr"), p->gfx_api == 3);
	cfgfile_write_strarr(f, _T("gfx_api_options"), filterapiopts, p->gfx_api_options);
	cfgfile_dwrite(f, _T("gfx_horizontal_extra"), _T("%d"), p->gfx_extrawidth);
	cfgfile_dwrite(f, _T("gfx_vertical_extra"), _T("%d"), p->gfx_extraheight);
	cfgfile_dwrite(f, _T("gfx_frame_slices"), _T("%d"), p->gfx_display_sections);
	cfgfile_dwrite_bool(f, _T("gfx_vrr_monitor"), p->gfx_variable_sync != 0);
	cfgfile_dwrite_strarr(f, _T("gfx_overscanmode"), overscanmodes, p->gfx_overscanmode);
	cfgfile_dwrite(f, _T("gfx_monitorblankdelay"), _T("%d"), p->gfx_monitorblankdelay);
	cfgfile_dwrite(f, _T("gfx_rotation"), _T("%d"), p->gfx_rotation);
	cfgfile_dwrite(f, _T("gfx_bordercolor"), _T("0x%08x"), p->gfx_bordercolor);

#ifdef GFXFILTER
	for (int j = 0; j < MAX_FILTERDATA; j++) {
		struct gfx_filterdata *gf = &p->gf[j];
		const TCHAR *ext = j == 0 ? NULL : (j == 1 ? _T("_rtg") : _T("_lace"));
		if (j == 2) {
			cfgfile_dwrite_bool(f, _T("gfx_filter_enable"), ext, gf->enable);
		}
		for (int i = 0; i <MAX_FILTERSHADERS; i++) {
			if (gf->gfx_filtershader[i][0])
				cfgfile_write_ext (f, _T("gfx_filter_pre"), ext, _T("D3D:%s"), gf->gfx_filtershader[i]);
			if (gf->gfx_filtermask[i][0])
				cfgfile_write_str (f, _T("gfx_filtermask_pre"), ext, gf->gfx_filtermask[i]);
		}
		for (int i = 0; i < MAX_FILTERSHADERS; i++) {
			if (gf->gfx_filtershader[i + MAX_FILTERSHADERS][0])
				cfgfile_write_ext (f, _T("gfx_filter_post"), ext, _T("D3D:%s"), gf->gfx_filtershader[i + MAX_FILTERSHADERS]);
			if (gf->gfx_filtermask[i + MAX_FILTERSHADERS][0])
				cfgfile_write_str (f, _T("gfx_filtermask_post"), ext, gf->gfx_filtermask[i + MAX_FILTERSHADERS]);
		}
		cfgfile_dwrite_str (f, _T("gfx_filter_mask"), ext, gf->gfx_filtermask[2 * MAX_FILTERSHADERS]);
		{
			bool d3dfound = false;
			if (gf->gfx_filtershader[2 * MAX_FILTERSHADERS][0] && p->gfx_api) {
				cfgfile_dwrite_ext (f, _T("gfx_filter"), ext, _T("D3D:%s"), gf->gfx_filtershader[2 * MAX_FILTERSHADERS]);
				d3dfound = true;
			}
			if (!d3dfound) {
				if (gf->gfx_filter > 0) {
					int i = 0;
					struct uae_filter *uf;
					while (uaefilters[i].name) {
						uf = &uaefilters[i];
						if (uf->type == gf->gfx_filter) {
							cfgfile_dwrite_str (f, _T("gfx_filter"), ext, uf->cfgname);
						}
						i++;
					}
				} else {
					cfgfile_dwrite_ext (f, _T("gfx_filter"), ext, _T("no"));
				}
			}
		}
		cfgfile_dwrite_strarr(f, _T("gfx_filter_mode"), ext, filtermode2, gf->gfx_filter_filtermodeh);
		cfgfile_dwrite_strarr(f, _T("gfx_filter_mode2"), ext, filtermode2v, gf->gfx_filter_filtermodev);
		cfgfile_dwrite_ext(f, _T("gfx_filter_vert_zoomf"), ext, _T("%f"), gf->gfx_filter_vert_zoom);
		cfgfile_dwrite_ext(f, _T("gfx_filter_horiz_zoomf"), ext, _T("%f"), gf->gfx_filter_horiz_zoom);
		cfgfile_dwrite_ext(f, _T("gfx_filter_vert_zoom_multf"), ext, _T("%f"), gf->gfx_filter_vert_zoom_mult);
		cfgfile_dwrite_ext(f, _T("gfx_filter_horiz_zoom_multf"), ext, _T("%f"), gf->gfx_filter_horiz_zoom_mult);
		cfgfile_dwrite_ext(f, _T("gfx_filter_vert_offsetf"), ext, _T("%f"), gf->gfx_filter_vert_offset);
		cfgfile_dwrite_ext(f, _T("gfx_filter_horiz_offsetf"), ext, _T("%f"), gf->gfx_filter_horiz_offset);

		cfgfile_dwrite_ext(f, _T("gfx_filter_left_border"), ext, _T("%d"), gf->gfx_filter_left_border);
		cfgfile_dwrite_ext(f, _T("gfx_filter_right_border"), ext, _T("%d"), gf->gfx_filter_right_border);
		cfgfile_dwrite_ext(f, _T("gfx_filter_top_border"), ext, _T("%d"), gf->gfx_filter_top_border);
		cfgfile_dwrite_ext(f, _T("gfx_filter_bottom_border"), ext, _T("%d"), gf->gfx_filter_bottom_border);

		cfgfile_dwrite_ext(f, _T("gfx_filter_scanlines"), ext, _T("%d"), gf->gfx_filter_scanlines);
		cfgfile_dwrite_ext(f, _T("gfx_filter_scanlinelevel"), ext, _T("%d"), gf->gfx_filter_scanlinelevel);
		cfgfile_dwrite_ext(f, _T("gfx_filter_scanlineratio"), ext, _T("%d"), gf->gfx_filter_scanlineratio);
		cfgfile_dwrite_ext(f, _T("gfx_filter_scanlineoffset"), ext, _T("%d"), gf->gfx_filter_scanlineoffset);

		cfgfile_dwrite_ext(f, _T("gfx_filter_luminance"), ext, _T("%d"), gf->gfx_filter_luminance);
		cfgfile_dwrite_ext(f, _T("gfx_filter_contrast"), ext, _T("%d"), gf->gfx_filter_contrast);
		cfgfile_dwrite_ext(f, _T("gfx_filter_saturation"), ext, _T("%d"), gf->gfx_filter_saturation);
		cfgfile_dwrite_ext(f, _T("gfx_filter_gamma"), ext, _T("%d"), gf->gfx_filter_gamma);
		cfgfile_dwrite_ext(f, _T("gfx_filter_gamma_r"), ext, _T("%d"), gf->gfx_filter_gamma_ch[0]);
		cfgfile_dwrite_ext(f, _T("gfx_filter_gamma_g"), ext, _T("%d"), gf->gfx_filter_gamma_ch[1]);
		cfgfile_dwrite_ext(f, _T("gfx_filter_gamma_b"), ext, _T("%d"), gf->gfx_filter_gamma_ch[2]);

		cfgfile_dwrite_ext(f, _T("gfx_filter_blur"), ext, _T("%d"), gf->gfx_filter_blur);
		cfgfile_dwrite_ext(f, _T("gfx_filter_noise"), ext, _T("%d"), gf->gfx_filter_noise);
		cfgfile_dwrite_bool(f, _T("gfx_filter_bilinear"), ext, gf->gfx_filter_bilinear != 0);

		cfgfile_dwrite_ext(f, _T("gfx_filter_keep_autoscale_aspect"), ext, _T("%d"), gf->gfx_filter_keep_autoscale_aspect);
		cfgfile_dwrite_strarr(f, _T("gfx_filter_keep_aspect"), ext, aspects, gf->gfx_filter_keep_aspect);
		cfgfile_dwrite_strarr(f, _T("gfx_filter_autoscale"), ext, j != GF_RTG ? autoscale : autoscale_rtg, gf->gfx_filter_autoscale);
		cfgfile_dwrite_strarr(f, _T("gfx_filter_autoscale_limit"), ext, autoscalelimit, gf->gfx_filter_integerscalelimit);
		cfgfile_dwrite_ext(f, _T("gfx_filter_aspect_ratio"), ext, _T("%d:%d"),
			gf->gfx_filter_aspect >= 0 ? (gf->gfx_filter_aspect / ASPECTMULT) : -1,
			gf->gfx_filter_aspect >= 0 ? (gf->gfx_filter_aspect & (ASPECTMULT - 1)) : -1);
		if (gf->gfx_filteroverlay[0]) {
			cfgfile_dwrite_ext(f, _T("gfx_filter_overlay"), ext, _T("%s%s"),
				gf->gfx_filteroverlay, _tcschr (gf->gfx_filteroverlay, ',') ? _T(",") : _T(""));
		}
		cfgfile_dwrite_ext(f, _T("gfx_filter_rotation"), ext, _T("%d"), gf->gfx_filter_rotation);
	}
	cfgfile_dwrite (f, _T("gfx_luminance"), _T("%d"), p->gfx_luminance);
	cfgfile_dwrite (f, _T("gfx_contrast"), _T("%d"), p->gfx_contrast);
	cfgfile_dwrite (f, _T("gfx_gamma"), _T("%d"), p->gfx_gamma);
	cfgfile_dwrite (f, _T("gfx_gamma_r"), _T("%d"), p->gfx_gamma_ch[0]);
	cfgfile_dwrite (f, _T("gfx_gamma_g"), _T("%d"), p->gfx_gamma_ch[1]);
	cfgfile_dwrite (f, _T("gfx_gamma_b"), _T("%d"), p->gfx_gamma_ch[2]);

	cfgfile_dwrite (f, _T("gfx_center_horizontal_position"), _T("%d"), p->gfx_xcenter_pos);
	cfgfile_dwrite (f, _T("gfx_center_vertical_position"), _T("%d"), p->gfx_ycenter_pos);
	cfgfile_dwrite (f, _T("gfx_center_horizontal_size"), _T("%d"), p->gfx_xcenter_size);
	cfgfile_dwrite (f, _T("gfx_center_vertical_size"), _T("%d"), p->gfx_ycenter_size);

	cfgfile_dwrite (f, _T("rtg_vert_zoom_multf"), _T("%.f"), p->rtg_vert_zoom_mult);
	cfgfile_dwrite (f, _T("rtg_horiz_zoom_multf"), _T("%.f"), p->rtg_horiz_zoom_mult);

#endif

	cfgfile_write_bool (f, _T("immediate_blits"), p->immediate_blits);
	cfgfile_dwrite_strarr(f, _T("waiting_blits"), waitblits, p->waiting_blits);
	cfgfile_dwrite (f, _T("blitter_throttle"), _T("%.8f"), p->blitter_speed_throttle);
#ifdef AMIBERRY
	cfgfile_write_bool(f, _T("multithreaded_drawing"), p->multithreaded_drawing);
#endif
	cfgfile_write_bool (f, _T("ntsc"), p->ntscmode);

	cfgfile_write_bool(f, _T("genlock"), p->genlock);
	cfgfile_dwrite_bool(f, _T("genlock_alpha"), p->genlock_alpha);
	cfgfile_dwrite_bool(f, _T("genlock_aspect"), p->genlock_aspect);
	cfgfile_dwrite_strarr(f, _T("genlockmode"), genlockmodes, p->genlock_image);
	cfgfile_dwrite_str(f, _T("genlock_image"), p->genlock_image_file);
	cfgfile_dwrite_str(f, _T("genlock_video"), p->genlock_video_file);
	cfgfile_dwrite_str(f, _T("genlock_font"), p->genlock_font);
	cfgfile_dwrite(f, _T("genlock_mix"), _T("%d"), p->genlock_mix);
	cfgfile_dwrite(f, _T("genlock_scale"), _T("%d"), p->genlock_scale);
	cfgfile_dwrite(f, _T("genlock_offset_x"), _T("%d"), p->genlock_offset_x);
	cfgfile_dwrite(f, _T("genlock_offset_y"), _T("%d"), p->genlock_offset_y);
	if (p->genlock_effects) {
		tmp[0] = 0;
		if (p->genlock_effects & (1 << 4)) {
			_tcscat(tmp, _T("brdntran"));
		}
		if (p->genlock_effects & (1 << 5)) {
			if (tmp[0]) {
				_tcscat(tmp, _T(","));
			}
			_tcscat(tmp, _T("brdrblnk"));
		}
		for (int i = 0; i < 256; i++) {
			uae_u64 v = p->ecs_genlock_features_colorkey_mask[i / 64];
			if (v & (1LL << (i & 63))) {
				if (tmp[0]) {
					_tcscat(tmp, _T(","));
				}
				_sntprintf(tmp + _tcslen(tmp), sizeof tmp + _tcslen(tmp), _T("%d"), i);
			}
		}
		for (int i = 0; i < 8; i++) {
			if (p->ecs_genlock_features_plane_mask & (1 << i)) {
				if (tmp[0]) {
					_tcscat(tmp, _T(","));
				}
				_sntprintf(tmp + _tcslen(tmp), sizeof tmp + _tcslen(tmp), _T("p%d"), i);
			}
		}
		cfgfile_dwrite_str(f, _T("genlock_effects"), tmp);
	}
#ifdef WITH_SPECIALMONITORS
	cfgfile_dwrite_strarr(f, _T("monitoremu"), specialmonitorconfignames, p->monitoremu);
#endif
	cfgfile_dwrite(f, _T("monitoremu_monitor"), _T("%d"), p->monitoremu_mon);
	cfgfile_dwrite_coords(f, _T("lightpen_offset"), p->lightpen_offset[0], p->lightpen_offset[1]);
	cfgfile_dwrite_bool(f, _T("lightpen_crosshair"), p->lightpen_crosshair);

	cfgfile_dwrite_bool (f, _T("show_leds"), !!(p->leds_on_screen & STATUSLINE_CHIPSET));
	cfgfile_dwrite_bool (f, _T("show_leds_rtg"), !!(p->leds_on_screen & STATUSLINE_RTG));
	write_leds(f, _T("show_leds_enabled"), p->leds_on_screen_mask[0]);
	write_leds(f, _T("show_leds_enabled_rtg"), p->leds_on_screen_mask[1]);
	for (int i = 0; i < 2; i++) {
		if (p->leds_on_screen_multiplier[i] > 0) {
			cfgfile_dwrite(f, i ? _T("show_leds_size_rtg") : _T("show_leds_size"), _T("%.2f"), p->leds_on_screen_multiplier[i] / 100.0);
		}
	}
	cfgfile_dwrite_bool(f, _T("show_refresh_indicator"), p->refresh_indicator);
	cfgfile_dwrite(f, _T("power_led_dim"), _T("%d"), p->power_led_dim);

	if (p->osd_pos.y || p->osd_pos.x) {
		cfgfile_dwrite (f, _T("osd_position"), _T("%.1f%s:%.1f%s"),
			p->osd_pos.x >= 20000 ? (p->osd_pos.x - 30000) / 10.0 : static_cast<float>(p->osd_pos.x), p->osd_pos.x >= 20000 ? _T("%") : _T(""),
			p->osd_pos.y >= 20000 ? (p->osd_pos.y - 30000) / 10.0 : static_cast<float>(p->osd_pos.y), p->osd_pos.y >= 20000 ? _T("%") : _T(""));
	}
	cfgfile_dwrite (f, _T("keyboard_leds"), _T("numlock:%s,capslock:%s,scrolllock:%s"),
		kbleds[p->keyboard_leds[0]], kbleds[p->keyboard_leds[1]], kbleds[p->keyboard_leds[2]]);
	if (p->chipset_mask & CSMASK_AGA)
		cfgfile_write (f, _T("chipset"),_T("aga"));
	else if ((p->chipset_mask & CSMASK_ECS_AGNUS) && (p->chipset_mask & CSMASK_ECS_DENISE))
		cfgfile_write (f, _T("chipset"),_T("ecs"));
	else if (p->chipset_mask & CSMASK_ECS_AGNUS)
		cfgfile_write (f, _T("chipset"),_T("ecs_agnus"));
	else if (p->chipset_mask & CSMASK_ECS_DENISE)
		cfgfile_write (f, _T("chipset"),_T("ecs_denise"));
	else
		cfgfile_write (f, _T("chipset"), _T("ocs"));
	if (p->chipset_refreshrate > 0)
		cfgfile_write (f, _T("chipset_refreshrate"), _T("%f"), p->chipset_refreshrate);
	cfgfile_dwrite_bool(f, _T("chipset_subpixel"), p->chipset_hr);

	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		struct chipset_refresh *cr = &p->cr[i];
		if (!cr->inuse)
			continue;
		cr->index = i;
		if (cr->rate == 0)
			_tcscpy(tmp, _T("0"));
		else
			_sntprintf (tmp, sizeof tmp, _T("%f"), cr->rate);
		TCHAR *s = tmp + _tcslen (tmp);
		if (cr->label[0] > 0 && i < MAX_CHIPSET_REFRESH)
			s += _sntprintf (s, sizeof s, _T(",t=%s"), cr->label);
		if (cr->horiz > 0)
			s += _sntprintf (s, sizeof s, _T(",h=%d"), cr->horiz);
		if (cr->vert > 0)
			s += _sntprintf (s, sizeof s, _T(",v=%d"), cr->vert);
		if (cr->locked)
			_tcscat (s, _T(",locked"));
		if (cr->ntsc == 1)
			_tcscat (s, _T(",ntsc"));
		else if (cr->ntsc == 0)
			_tcscat (s, _T(",pal"));
		else if (cr->ntsc == 2)
			_tcscat(s, _T(",custom"));
		if (cr->lace > 0)
			_tcscat (s, _T(",lace"));
		else if (cr->lace == 0)
			_tcscat (s, _T(",nlace"));
		if ((cr->resolution & 7) != 7) {
			if (cr->resolution & 1)
				_tcscat(s, _T(",lores"));
			if (cr->resolution & 2)
				_tcscat(s, _T(",hires"));
			if (cr->resolution & 4)
				_tcscat(s, _T(",shres"));
			if (cr->resolution_pct > 0 && cr->resolution_pct < 100)
				s += _sntprintf(s, sizeof s, _T("rpct=%d"), cr->resolution_pct);
		}
		if (cr->framelength > 0)
			_tcscat (s, _T(",lof"));
		else if (cr->framelength == 0)
			_tcscat (s, _T(",shf"));
		if (cr->vsync > 0)
			_tcscat (s, _T(",vsync"));
		else if (cr->vsync == 0)
			_tcscat (s, _T(",nvsync"));
		if (cr->rtg)
			_tcscat (s, _T(",rtg"));
		if (cr->exit)
			_tcscat(s, _T(",exit"));
		if (cr->defaultdata)
			_tcscat(s, _T(",default"));
		if (cr->filterprofile[0]) {
			TCHAR *se = cfgfile_escape(cr->filterprofile, _T(","), true, false);
			s += _sntprintf(s, sizeof s, _T(",filter=%s"), se);
			xfree(se);
		}
		if (cr->commands[0]) {
			_tcscat (s, _T(",cmd="));
			_tcscat (s, cr->commands);
			for (int j = 0; j < _tcslen (s); j++) {
				if (s[j] == '\n')
					s[j] = ',';
			}
			s[_tcslen (s) - 1] = 0;
		}
		if (i == CHIPSET_REFRESH_PAL) {
			if (cr->locked)
				cfgfile_dwrite (f, _T("displaydata_pal"), tmp);
		} else if (i == CHIPSET_REFRESH_NTSC) {
			if (cr->locked)
				cfgfile_dwrite (f, _T("displaydata_ntsc"), tmp);
		} else {
			cfgfile_dwrite (f, _T("displaydata"), tmp);
		}
	}

	cfgfile_write_strarr(f, _T("collision_level"), collmode, p->collision_level);

	cfgfile_write_strarr(f, _T("chipset_compatible"), cscompa, p->cs_compatible);
	cfgfile_dwrite_strarr(f, _T("ciaatod"), ciaatodmode, p->cs_ciaatod);
	cfgfile_dwrite_strarr(f, _T("rtc"), rtctype, p->cs_rtc);
	cfgfile_dwrite(f, _T("chipset_rtc_adjust"), _T("%d"), p->cs_rtc_adjust);
	cfgfile_dwrite_bool(f, _T("cia_overlay"), p->cs_ciaoverlay);
	cfgfile_dwrite_bool(f, _T("ksmirror_e0"), p->cs_ksmirror_e0);
	cfgfile_dwrite_bool(f, _T("ksmirror_a8"), p->cs_ksmirror_a8);
	cfgfile_dwrite_bool(f, _T("cd32cd"), p->cs_cd32cd);
	cfgfile_dwrite_bool(f, _T("cd32c2p"), p->cs_cd32c2p);
	cfgfile_dwrite_bool(f, _T("cd32nvram"), p->cs_cd32nvram);
	cfgfile_dwrite(f, _T("cd32nvram_size"), _T("%d"), p->cs_cd32nvram_size / 1024);
	cfgfile_dwrite_bool(f, _T("cdtvcd"), p->cs_cdtvcd);
	cfgfile_dwrite_bool(f, _T("cdtv-cr"), p->cs_cdtvcr);
	cfgfile_dwrite_bool(f, _T("cdtvram"), p->cs_cdtvram);
	cfgfile_dwrite_bool(f, _T("a1000ram"), p->cs_a1000ram);
	cfgfile_dwrite(f, _T("fatgary"), _T("%d"), p->cs_fatgaryrev);
	cfgfile_dwrite(f, _T("ramsey"), _T("%d"), p->cs_ramseyrev);
	cfgfile_dwrite_bool(f, _T("pcmcia"), p->cs_pcmcia);
	cfgfile_dwrite_bool(f, _T("resetwarning"), p->cs_resetwarning);
	cfgfile_dwrite_bool(f, _T("denise_noehb"), p->cs_denisemodel == DENISEMODEL_VELVET || p->cs_denisemodel == DENISEMODEL_A1000NOEHB);
	cfgfile_dwrite_bool(f, _T("agnus_bltbusybug"), p->cs_agnusmodel == AGNUSMODEL_A1000);
	cfgfile_dwrite_bool(f, _T("bkpt_halt"), p->cs_bkpthang);
	cfgfile_dwrite_bool(f, _T("ics_agnus"), p->cs_agnusmodel == AGNUSMODEL_A1000);
	cfgfile_dwrite_bool(f, _T("cia_todbug"), p->cs_ciatodbug);
	cfgfile_dwrite_bool(f, _T("z3_autoconfig"), p->cs_z3autoconfig);
	cfgfile_dwrite_bool(f, _T("1mchipjumper"), p->cs_1mchipjumper);
	cfgfile_dwrite_bool(f, _T("color_burst"), p->cs_color_burst);
	cfgfile_dwrite_strarr(f, _T("hvcsync"), hvcsync, p->cs_hvcsync);
	cfgfile_dwrite_bool(f, _T("toshiba_gary"), p->cs_toshibagary);
	cfgfile_dwrite_bool(f, _T("rom_is_slow"), p->cs_romisslow);
	cfgfile_dwrite_strarr(f, _T("ciaa_type"), ciatype, p->cs_ciatype[0]);
	cfgfile_dwrite_strarr(f, _T("ciab_type"), ciatype, p->cs_ciatype[1]);
	cfgfile_dwrite_strarr(f, _T("unmapped_address_space"), unmapped, p->cs_unmapped_space);
	cfgfile_dwrite_bool(f, _T("memory_pattern"), p->cs_memorypatternfill);
	cfgfile_dwrite_bool(f, _T("ipl_delay"), p->cs_ipldelay);
	cfgfile_dwrite_bool(f, _T("floppydata_pullup"), p->cs_floppydatapullup);
	cfgfile_dwrite(f, _T("keyboard_handshake"), _T("%d"), currprefs.cs_kbhandshake);
	cfgfile_dwrite(f, _T("chipset_hacks"), _T("0x%x"), p->cs_hacks);
	cfgfile_dwrite(f, _T("eclockphase"), _T("%d"), p->cs_eclockphase);
	cfgfile_dwrite_strarr(f, _T("eclocksync"), eclocksync, p->cs_eclocksync);
	cfgfile_dwrite_strarr(f, _T("agnusmodel"), agnusmodel, p->cs_agnusmodel);
	cfgfile_dwrite_strarr(f, _T("agnussize"), agnussize, p->cs_agnussize);
	cfgfile_dwrite_strarr(f, _T("denisemodel"), denisemodel, p->cs_denisemodel);
	if (p->seed) {
		cfgfile_write(f, _T("rndseed"), _T("%d"), p->seed);
	}

	if (is_board_enabled(p, ROMTYPE_CD32CART, 0)) {
		cfgfile_dwrite_bool(f, _T("cd32fmv"), true);
	}
	if (is_board_enabled(p, ROMTYPE_MB_IDE, 0) && p->cs_ide == 1) {
		cfgfile_dwrite_str(f, _T("ide"), _T("a600/a1200"));
	}
	if (is_board_enabled(p, ROMTYPE_MB_IDE, 0) && p->cs_ide == 2) {
		cfgfile_dwrite_str(f, _T("ide"), _T("a4000"));
	}
	if (is_board_enabled(p, ROMTYPE_CDTVSCSI, 0)) {
		cfgfile_dwrite_bool(f, _T("scsi_cdtv"), true);
	}
	if (is_board_enabled(p, ROMTYPE_SCSI_A3000, 0)) {
		cfgfile_dwrite_bool(f, _T("scsi_a3000"), true);
	}
	if (is_board_enabled(p, ROMTYPE_SCSI_A4000T, 0)) {
		cfgfile_dwrite_bool(f, _T("scsi_a4000t"), true);
	}

	cfgfile_dwrite_strarr(f, _T("z3mapping"), z3mapping, p->z3_mapping_mode);
	cfgfile_dwrite_bool(f, _T("board_custom_order"), p->autoconfig_custom_sort);
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		if (p->fastmem[i].size < 0x100000 && p->fastmem[i].size) {
			if (i > 0)
				_sntprintf(tmp, sizeof tmp, _T("fastmem%d_size_k"), i + 1);
			else
				_tcscpy(tmp, _T("fastmem_size_k"));
			cfgfile_write(f, tmp, _T("%d"), p->fastmem[i].size / 1024);
		} else if (p->fastmem[i].size || i == 0) {
			if (i > 0)
				_sntprintf(tmp, sizeof tmp, _T("fastmem%d_size"), i + 1);
			else
				_tcscpy(tmp, _T("fastmem_size"));
			cfgfile_write(f, tmp, _T("%d"), p->fastmem[i].size / 0x100000);
		}
		cfgfile_writeramboard(p, f, _T("fastmem"), i, &p->fastmem[i]);
	}
#ifdef DEBUGGER
	cfgfile_write(f, _T("debugmem_start"), _T("0x%x"), p->debugmem_start);
	cfgfile_write(f, _T("debugmem_size"), _T("%d"), p->debugmem_size / 0x100000);
#endif
	cfgfile_write(f, _T("mem25bit_size"), _T("%d"), p->mem25bit.size / 0x100000);
	cfgfile_writeramboard(p, f, _T("mem25bit"), 0, &p->mem25bit);
	cfgfile_write(f, _T("a3000mem_size"), _T("%d"), p->mbresmem_low.size / 0x100000);
	cfgfile_writeramboard(p, f, _T("a3000mem"), 0, &p->mbresmem_low);
	cfgfile_write(f, _T("mbresmem_size"), _T("%d"), p->mbresmem_high.size / 0x100000);
	cfgfile_writeramboard(p, f, _T("mbresmem"), 0, &p->mbresmem_high);
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		if (i == 0 || p->z3fastmem[i].size) {
			if (i > 0)
				_sntprintf(tmp, sizeof tmp, _T("z3mem%d_size"), i + 1);
			else
				_tcscpy(tmp, _T("z3mem_size"));
			cfgfile_write(f, tmp, _T("%d"), p->z3fastmem[i].size / 0x100000);
		}
		cfgfile_writeramboard(p, f, _T("z3mem"), i, &p->z3fastmem[i]);
	}
	cfgfile_write(f, _T("z3mem_start"), _T("0x%x"), p->z3autoconfig_start);

	cfgfile_write(f, _T("bogomem_size"), _T("%d"), p->bogomem.size / 0x40000);
	cfgfile_writeramboard(p, f, _T("bogomem"), 0, &p->bogomem);

	if (p->cpuboard_type) {
		const struct cpuboardtype *cbt = &cpuboards[p->cpuboard_type];
		const struct cpuboardsubtype *cbst = &cbt->subtypes[p->cpuboard_subtype];
		const struct expansionboardsettings *cbs = cbst->settings;
		cfgfile_dwrite_str(f, _T("cpuboard_type"), cbst->configname);
		if (cbs && p->cpuboard_settings) {
			tmp[0] = 0;
			cfgfile_write_rom_settings(cbs, tmp, p->cpuboard_settings, nullptr);
			cfgfile_dwrite_str(f, _T("cpuboard_settings"), tmp);
		}
	} else {
		cfgfile_dwrite_str(f, _T("cpuboard_type"), _T("none"));
	}
	cfgfile_dwrite(f, _T("cpuboardmem1_size"), _T("%d"), p->cpuboardmem1.size / 0x100000);
	cfgfile_writeramboard(p, f, _T("cpuboardmem1"), 0, &p->cpuboardmem1);
	cfgfile_dwrite(f, _T("cpuboardmem2_size"), _T("%d"), p->cpuboardmem2.size / 0x100000);
	cfgfile_writeramboard(p, f, _T("cpuboardmem2"), 0, &p->cpuboardmem2);
	cfgfile_write_bool(f, _T("gfxcard_hardware_vblank"), p->rtg_hardwareinterrupt);
	cfgfile_write_bool(f, _T("gfxcard_hardware_sprite"), p->rtg_hardwaresprite);
	cfgfile_dwrite_bool(f, _T("gfxcard_overlay"), p->rtg_overlay);
	cfgfile_dwrite_bool(f, _T("gfxcard_screensplit"), p->rtg_vgascreensplit);
	cfgfile_dwrite_bool(f, _T("gfxcard_paletteswitch"), p->rtg_paletteswitch);
	cfgfile_dwrite_bool(f, _T("gfxcard_dacswitch"), p->rtg_dacswitch);
	cfgfile_write_bool(f, _T("gfxcard_multithread"), p->rtg_multithread);
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		TCHAR tmp2[100];
		struct rtgboardconfig *rbc = &p->rtgboards[i];
		if (rbc->rtgmem_size) {
			if (i > 0)
				_sntprintf(tmp, sizeof tmp, _T("gfxcard%d_size"), i + 1);
			else
				_tcscpy(tmp, _T("gfxcard_size"));
			cfgfile_write(f, tmp, _T("%d"), rbc->rtgmem_size / 0x100000);
			if (i > 0)
				_sntprintf(tmp, sizeof tmp, _T("gfxcard%d_type"), i + 1);
			else
				_tcscpy(tmp, _T("gfxcard_type"));
			cfgfile_dwrite_str(f, tmp, gfxboard_get_configname(rbc->rtgmem_type));
			tmp2[0] = 0;
			if (rbc->device_order > 0 && p->autoconfig_custom_sort) {
				_sntprintf(tmp2, sizeof tmp2, _T("order=%d"), rbc->device_order);
			}
			if (rbc->monitor_id) {
				if (tmp2[0])
					_tcscat(tmp2, _T(","));
				_sntprintf(tmp2 + _tcslen(tmp2), sizeof tmp2 + _tcslen(tmp2), _T("monitor=%d"), rbc->monitor_id);
			}
			if (tmp2[0]) {
				if (i > 0)
					_sntprintf(tmp, sizeof tmp, _T("gfxcard%d_options"), i + 1);
				else
					_tcscpy(tmp, _T("gfxcard_options"));
				cfgfile_dwrite_str(f, tmp, tmp2);
			}
		}
	}
	cfgfile_write (f, _T("chipmem_size"), _T("%d"), p->chipmem.size == 0x20000 ? -1 : (p->chipmem.size == 0x40000 ? 0 : p->chipmem.size / 0x80000));
	cfgfile_writeramboard(p, f, _T("chipmem"), 0, &p->chipmem);

	cfgfile_dwrite (f, _T("megachipmem_size"), _T("%d"), p->z3chipmem.size / 0x100000);
	cfgfile_writeramboard(p, f, _T("megachipmem"), 0, &p->z3chipmem);

	// do not save aros rom special space
	if (!(p->custom_memory_sizes[0] == 512 * 1024 && p->custom_memory_sizes[1] == 512 * 1024 && p->custom_memory_addrs[0] == 0xa80000 && p->custom_memory_addrs[1] == 0xb00000)) {
		if (p->custom_memory_sizes[0])
			cfgfile_write (f, _T("addmem1"), _T("0x%x,0x%x"), p->custom_memory_addrs[0], p->custom_memory_sizes[0]);
		if (p->custom_memory_sizes[1])
			cfgfile_write (f, _T("addmem2"), _T("0x%x,0x%x"), p->custom_memory_addrs[1], p->custom_memory_sizes[1]);
	}

	if (p->m68k_speed > 0) {
		cfgfile_write (f, _T("finegrain_cpu_speed"), _T("%d"), p->m68k_speed);
	} else {
		cfgfile_write_str (f, _T("cpu_speed"), p->m68k_speed < 0 ? _T("max") : _T("real"));
	}
	cfgfile_write (f, _T("cpu_throttle"), _T("%.1f"), p->m68k_speed_throttle);
	cfgfile_dwrite(f, _T("cpu_x86_throttle"), _T("%.1f"), p->x86_speed_throttle);

	/* do not reorder start */
	write_compatibility_cpu(f, p);
	cfgfile_write (f, _T("cpu_model"), _T("%d"), p->cpu_model);
	if (p->fpu_model)
		cfgfile_write (f, _T("fpu_model"), _T("%d"), p->fpu_model);
	if (p->mmu_model) {
		if (p->mmu_ec)
			cfgfile_write (f, _T("mmu_model"), _T("68ec0%d"), p->mmu_model % 100);
		else
			cfgfile_write (f, _T("mmu_model"), _T("%d"), p->mmu_model);
	}
	if (p->ppc_mode) {
		cfgfile_write_str(f, _T("ppc_model"), p->ppc_model[0] ? p->ppc_model : (p->ppc_mode == 1 ? _T("automatic") : _T("manual")));
		cfgfile_write_strarr(f, _T("ppc_cpu_idle"), ppc_cpu_idle, p->ppc_cpu_idle);
	}
	cfgfile_write_bool (f, _T("cpu_compatible"), p->cpu_compatible);
	cfgfile_write_bool (f, _T("cpu_24bit_addressing"), p->address_space_24);
	cfgfile_write_bool (f, _T("cpu_data_cache"), p->cpu_data_cache);
	/* do not reorder end */
	cfgfile_dwrite_bool(f, _T("cpu_reset_pause"), p->reset_delay);
	cfgfile_dwrite_bool(f, _T("cpu_halt_auto_reset"), p->crash_auto_reset);
	cfgfile_dwrite_bool(f, _T("cpu_threaded"), p->cpu_thread);
	if (p->ppc_mode)
		cfgfile_write_strarr(f, _T("ppc_implementation"), ppc_implementations, p->ppc_implementation);

	if (p->cpu_cycle_exact) {
		if (p->cpu_frequency)
			cfgfile_write(f, _T("cpu_frequency"), _T("%d"), p->cpu_frequency);
	}
	if (p->cpu_compatible) {
		if (p->m68k_speed == 0 && p->cpu_clock_multiplier >= 256) {
			cfgfile_write(f, _T("cpu_multiplier"), _T("%d"), p->cpu_clock_multiplier / 256);
		}
	}

	cfgfile_write_bool (f, _T("cpu_cycle_exact"), p->cpu_cycle_exact);
	// must be after cpu_cycle_exact
	cfgfile_write_bool (f, _T("cpu_memory_cycle_exact"), p->cpu_memory_cycle_exact);
	cfgfile_write_bool (f, _T("blitter_cycle_exact"), p->blitter_cycle_exact);
	// must be after cpu_cycle_exact, cpu_memory_cycle_exact and blitter_cycle_exact
	if (p->cpu_cycle_exact && p->blitter_cycle_exact)
		cfgfile_write_str (f, _T("cycle_exact"), cycleexact[2]);
	else if (p->cpu_memory_cycle_exact && p->blitter_cycle_exact)
		cfgfile_write_str (f, _T("cycle_exact"), cycleexact[1]);
	else
		cfgfile_write_str (f, _T("cycle_exact"), cycleexact[0]);

	cfgfile_dwrite_bool (f, _T("fpu_no_unimplemented"), p->fpu_no_unimplemented);
	cfgfile_dwrite_bool (f, _T("cpu_no_unimplemented"), p->int_no_unimplemented);
	cfgfile_write_bool (f, _T("fpu_strict"), p->fpu_strict);
	cfgfile_dwrite_bool (f, _T("fpu_softfloat"), p->fpu_mode > 0);
#ifdef MSVC_LONG_DOUBLE
	cfgfile_dwrite_bool(f, _T("fpu_msvc_long_double"), p->fpu_mode < 0);
#endif

	cfgfile_write_bool (f, _T("rtg_nocustom"), p->picasso96_nocustom);
	cfgfile_write (f, _T("rtg_modes"), _T("0x%x"), p->picasso96_modeflags);

	cfgfile_write_bool(f, _T("debug_mem"), p->debug_mem);
	cfgfile_write_bool(f, _T("log_illegal_mem"), p->illegal_mem);

#if 0
	if (p->catweasel >= 100)
		cfgfile_dwrite (f, _T("catweasel"), _T("0x%x"), p->catweasel);
	else
		cfgfile_dwrite (f, _T("catweasel"), _T("%d"), p->catweasel);
	cfgfile_write_bool(f, _T("toccata"), p->obs_sound_toccata);
	if (p->obs_sound_toccata_mixer)
		cfgfile_write_bool(f, _T("toccata_mixer"), p->obs_sound_toccata_mixer);
	cfgfile_write_bool(f, _T("es1370_pci"), p->obs_sound_es1370);
	cfgfile_write_bool(f, _T("fm801_pci"), p->obs_sound_fm801);
#endif

	cfgfile_dwrite_bool(f, _T("keyboard_connected"), p->keyboard_mode >= 0);
	cfgfile_dwrite_bool(f, _T("keyboard_nkro"), p->keyboard_nkro);
	cfgfile_dwrite_strarr(f, _T("keyboard_type"), kbtype, p->keyboard_mode + 1);
	cfgfile_write_str (f, _T("kbd_lang"), (p->keyboard_lang == KBD_LANG_DE ? _T("de")
		: p->keyboard_lang == KBD_LANG_DK ? _T("dk")
		: p->keyboard_lang == KBD_LANG_ES ? _T("es")
		: p->keyboard_lang == KBD_LANG_US ? _T("us")
		: p->keyboard_lang == KBD_LANG_SE ? _T("se")
		: p->keyboard_lang == KBD_LANG_FR ? _T("fr")
		: p->keyboard_lang == KBD_LANG_IT ? _T("it")
		: _T("FOO")));

	cfgfile_dwrite(f, _T("state_replay_rate"), _T("%d"), p->statecapturerate);
	cfgfile_dwrite(f, _T("state_replay_buffers"), _T("%d"), p->statecapturebuffersize);
	cfgfile_dwrite_bool(f, _T("state_replay_autoplay"), p->inprec_autoplay);
	cfgfile_dwrite_bool(f, _T("warp"), p->turbo_emulation);
	cfgfile_dwrite(f, _T("warp_limit"), _T("%d"), p->turbo_emulation_limit);
	cfgfile_dwrite_bool(f, _T("warpboot"), p->turbo_boot);
	cfgfile_dwrite(f, _T("warpboot_delay"), _T("%d"), p->turbo_boot_delay);

#ifdef FILESYS
	write_filesys_config (p, f);
	if (p->filesys_no_uaefsdb)
		cfgfile_write_bool (f, _T("filesys_no_fsdb"), p->filesys_no_uaefsdb);
	cfgfile_dwrite (f, _T("filesys_max_size"), _T("%d"), p->filesys_limit);
	cfgfile_dwrite (f, _T("filesys_max_name_length"), _T("%d"), p->filesys_max_name);
	cfgfile_dwrite (f, _T("filesys_max_file_size"), _T("%d"), p->filesys_max_file_size);
	cfgfile_dwrite_bool (f, _T("filesys_inject_icons"), p->filesys_inject_icons);
	cfgfile_dwrite_str (f, _T("filesys_inject_icons_drawer"), p->filesys_inject_icons_drawer);
	cfgfile_dwrite_str (f, _T("filesys_inject_icons_project"), p->filesys_inject_icons_project);
	cfgfile_dwrite_str (f, _T("filesys_inject_icons_tool"), p->filesys_inject_icons_tool);
	cfgfile_dwrite_strarr(f, _T("scsidev_mode"), uaescsidevmodes, p->uaescsidevmode);
#endif
	cfgfile_dwrite_bool(f, _T("harddrive_write_protect"), p->harddrive_read_only);

	write_inputdevice_config (p, f);

#ifdef AMIBERRY
	cfg_write(_T("; *** WHDLoad Booter. Options"), f);

	cfgfile_write_str(f, _T("whdload_filename"), whdload_prefs.whdload_filename.c_str());
	cfgfile_write_str(f, _T("whdload_game_name"), whdload_prefs.game_name.c_str());
	cfgfile_write_str(f, _T("whdload_uuid"), whdload_prefs.variant_uuid.c_str());
	cfgfile_write_str(f, _T("whdload_slave_default"), whdload_prefs.slave_default.c_str());
	cfgfile_write_bool(f, _T("whdload_slave_libraries"), whdload_prefs.slave_libraries);
	cfgfile_write_str(f, _T("whdload_slave"), whdload_prefs.selected_slave.filename.c_str());
	cfgfile_write_str(f, _T("whdload_slave_data_path"), whdload_prefs.selected_slave.data_path.c_str());
	cfgfile_write(f, _T("whdload_custom1"), _T("%d"), whdload_prefs.selected_slave.custom1.value);
	cfgfile_write(f, _T("whdload_custom2"), _T("%d"), whdload_prefs.selected_slave.custom2.value);
	cfgfile_write(f, _T("whdload_custom3"), _T("%d"), whdload_prefs.selected_slave.custom3.value);
	cfgfile_write(f, _T("whdload_custom4"), _T("%d"), whdload_prefs.selected_slave.custom4.value);
	cfgfile_write(f, _T("whdload_custom5"), _T("%d"), whdload_prefs.selected_slave.custom5.value);
	cfgfile_write_str(f, _T("whdload_custom"), whdload_prefs.custom.c_str());
	cfgfile_write_bool(f, _T("whdload_buttonwait"), whdload_prefs.button_wait);
	cfgfile_write_bool(f, _T("whdload_showsplash"), whdload_prefs.show_splash);
	cfgfile_dwrite(f, _T("whdload_configdelay"), _T("%d"), whdload_prefs.config_delay);
	cfgfile_write_bool(f, _T("whdload_writecache"), whdload_prefs.write_cache);
	cfgfile_write_bool(f, _T("whdload_quit_on_exit"), whdload_prefs.quit_on_exit);
#endif
}

static int cfgfile_coords(const TCHAR *option, const TCHAR *value, const TCHAR *name, int *x, int *y)
{
	if (name != nullptr && _tcscmp (option, name) != 0)
		return 0;
	TCHAR tmp[MAX_DPATH];
	_tcscpy(tmp, value);
	auto* p = _tcschr(tmp, ',');
	if (!p)
		return 0;
	*p++ = 0;
	*x = _tstol(tmp);
	*y = _tstol(p);
	return 1;
}

static int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, bool numbercheck)
{
	if (name != nullptr && _tcscmp (option, name) != 0)
		return 0;
	if (_tcsicmp(value, _T("yes")) == 0 || _tcsicmp(value, _T("y")) == 0
		|| _tcsicmp(value, _T("true")) == 0 || _tcsicmp(value, _T("t")) == 0
		|| (numbercheck && _tcsicmp(value, _T("1")) == 0))
		*location = 1;
	else if (_tcsicmp(value, _T("no")) == 0 || _tcsicmp(value, _T("n")) == 0
		|| _tcsicmp(value, _T("false")) == 0 || _tcsicmp(value, _T("f")) == 0
		|| (numbercheck && _tcsicmp(value, _T("0")) == 0))
		*location = 0;
	else {
		cfgfile_warning(_T("Option '%s' requires a value of either 'true' or 'false' (was '%s').\n"), option, value);
		return -1;
	}
	return 1;
}

static int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location)
{
	return cfgfile_yesno (option, value, name, location, true);
}

static int cfgfile_yesno (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location, bool numbercheck)
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

static int cfgfile_floatval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, float *location)
{
	TCHAR *endptr;
	if (name == nullptr)
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
	*location = static_cast<float>(_tcstod(value, &endptr));
	return 1;
}

#ifdef AMIBERRY
int cfgfile_floatval(const TCHAR* option, const TCHAR* value, const TCHAR* name, float* location)
#else
static int cfgfile_floatval (const TCHAR *option, const TCHAR *value, const TCHAR *name, float *location)
#endif
{
	return cfgfile_floatval (option, value, name, nullptr, location);
}

static int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, unsigned int *location, int scale)
{
	int base = 10;
	TCHAR *endptr;
	TCHAR tmp[MAX_DPATH];

	if (name == nullptr)
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
	if (value[0] == '0' && _totupper (value[1]) == 'X')
		value += 2, base = 16;
	*location = _tcstol (value, &endptr, base) * scale;

	if (*endptr != '\0' || *value == '\0') {
		if (_tcsicmp (value, _T("false")) == 0 || _tcsicmp (value, _T("no")) == 0) {
			*location = 0;
			return 1;
		}
		if (_tcsicmp (value, _T("true")) == 0 || _tcsicmp (value, _T("yes")) == 0) {
			*location = 1;
			return 1;
		}
		cfgfile_warning(_T("Option '%s' requires a numeric argument but got '%s'\n"), nameext ? tmp : option, value);
		return -1;
	}
	return 1;
}
static int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, unsigned int *location, int scale)
{
	return cfgfile_intval (option, value, name, nullptr, location, scale);
}
int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval (option, value, name, nullptr, &v, scale);
	if (!r)
		return 0;
	*location = static_cast<int>(v);
	return r;
}
static int cfgfile_intval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, int *location, int scale)
{
	unsigned int v = 0;
	int r = cfgfile_intval (option, value, name, nameext, &v, scale);
	if (!r)
		return 0;
	*location = static_cast<int>(v);
	return r;
}

static int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, int *location, const TCHAR *table[], int more)
{
	int val;
	TCHAR tmp[MAX_DPATH];
	if (name == nullptr)
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
		if (!_tcsicmp (value, _T("yes")) || !_tcsicmp (value, _T("true"))) {
			val = 1;
		} else if (!_tcsicmp (value, _T("no")) || !_tcsicmp (value, _T("false"))) {
			val = 0;
		} else {
			cfgfile_warning(_T("Unknown value ('%s') for option '%s'.\n"), value, nameext ? tmp : option);
			return -1;
		}
	}
	*location = val;
	return 1;
}
int cfgfile_strval (const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[], int more)
{
	return cfgfile_strval (option, value, name, nullptr, location, table, more);
}

static int cfgfile_strboolval (const TCHAR *option, const TCHAR *value, const TCHAR *name, bool *location, const TCHAR *table[], int more)
{
	int locationint;
	if (!cfgfile_strval (option, value, name, &locationint, table, more))
		return 0;
	*location = locationint != 0;
	return 1;
}

int cfgfile_string_escape(const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	if (_tcscmp(option, name) != 0)
		return 0;
	TCHAR *s = cfgfile_unescape_min(value);
	_tcsncpy(location, s, maxsz - 1);
	xfree(s);
	location[maxsz - 1] = '\0';
	return 1;
}

int cfgfile_string(const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	if (_tcscmp (option, name) != 0)
		return 0;
	_tcsncpy (location, value, maxsz - 1);
	location[maxsz - 1] = '\0';
	return 1;
}

#ifdef AMIBERRY
// Similar to the above, but using strings instead
int cfgfile_string(const std::string& option, const std::string& value, const std::string& name, std::string& location)
{
	if (option != name)
		return 0;
	location = value;
	return 1;
}
#endif

static int cfgfile_string (const TCHAR *option, const TCHAR *value, const TCHAR *name, const TCHAR *nameext, TCHAR *location, int maxsz)
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

static bool cfgfile_multichoice(const TCHAR *option, const TCHAR *value, const TCHAR *name, int *location, const TCHAR *table[])
{
	if (_tcscmp(option, name) != 0)
		return false;
	int v = 0;
	for (int i = 0; table[i]; i++) {
		if (cfgfile_option_find(value, table[i])) {
			v |= 1 << i;
		}
	}
	*location = v;
	return true;
}

static int cfgfile_path (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz, struct multipath *mp)
{
	if (!cfgfile_string (option, value, name, location, maxsz))
		return 0;
	cfgfile_adjust_path(location, maxsz, mp);
	return 1;
}

static int cfgfile_path (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	return cfgfile_path (option, value, name, location, maxsz, nullptr);
}

static int cfgfile_multipath (const TCHAR *option, const TCHAR *value, const TCHAR *name, struct multipath *mp, struct uae_prefs *p)
{
	TCHAR tmploc[MAX_DPATH];
	if (!cfgfile_string (option, value, name, tmploc, MAX_DPATH))
		return 0;
	for (int i = 0; i < MAX_PATHS; i++) {
		if (mp->path[i][0] == 0 || (i == 0 && (!_tcscmp (mp->path[i], _T(".\\")) || !_tcscmp (mp->path[i], _T("./"))))) {
			const auto s = target_expand_environment (tmploc, nullptr, 0);
			_tcsncpy (mp->path[i], s, MAX_DPATH - 1);
			mp->path[i][MAX_DPATH - 1] = 0;
			fix_trailing (mp->path[i]);
			xfree (s);
			//target_multipath_modified(p);
			return 1;
		}
	}
	return 1;
}

static int cfgfile_rom (const TCHAR *option, const TCHAR *value, const TCHAR *name, TCHAR *location, int maxsz)
{
	TCHAR id[MAX_DPATH];
	if (!cfgfile_string (option, value, name, id, sizeof id / sizeof (TCHAR)))
		return 0;
	TCHAR *p = _tcschr (id, ',');
	if (p) {
		TCHAR *endptr;
		*p = 0;
		auto tmp = id[4];
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

	if (p2 == nullptr)
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

static int getintval2 (TCHAR **p, int *result, int delim, bool last)
{
	auto* value = *p;
	auto base = 10;
	TCHAR* endptr;

	auto p2 = _tcschr (*p, delim);
	if (p2 == nullptr) {
		if (last) {
			if (delim != '.')
				p2 = _tcschr (*p, ',');
			if (p2 == nullptr) {
				p2 = *p;
				while(*p2)
					p2++;
				if (p2 == *p)
					return 0;
			}
		} else {
			return 0;
		}
	}
	if (!_istdigit(**p) && **p != '-' && **p != '+')
		return 0;

	if (*p2 != 0)
		*p2++ = '\0';

	if (value[0] == '0' && _totupper (value[1]) == 'X')
		value += 2, base = 16;
	*result = _tcstol (value, &endptr, base);
	*p = p2;

	if (*endptr != '\0' || *value == '\0') {
		*p = nullptr;
		return 0;
	}

	return 1;
}

static int cfgfile_option_select(TCHAR *s, const TCHAR *option, const TCHAR *select)
{
	TCHAR buf[MAX_DPATH];
	if (!s)
		return -1;
	_tcscpy(buf, s);
	_tcscat(buf, _T(","));
	TCHAR *p = buf;
	for (;;) {
		TCHAR *tmpp = _tcschr (p, ',');
		if (tmpp == nullptr)
			return -1;
		*tmpp++ = 0;
		TCHAR *tmpp2 = _tcschr(p, '=');
		if (!tmpp2)
			return -1;
		*tmpp2++ = 0;
		if (!_tcsicmp(p, option)) {
			int idx = 0;
			while (select[0]) {
				if (!_tcsicmp(select, tmpp2))
					return idx;
				idx++;
				select += _tcslen(select) + 1;
			}
		}
		p = tmpp;
	}
}

static int cfgfile_option_bool(TCHAR *s, const TCHAR *option)
{
	TCHAR buf[MAX_DPATH];
	if (!s)
		return -1;
	_tcscpy(buf, s);
	_tcscat(buf, _T(","));
	TCHAR *p = buf;
	for (;;) {
		TCHAR *tmpp = _tcschr (p, ',');
		if (tmpp == nullptr)
			return -1;
		*tmpp++ = 0;
		TCHAR *tmpp2 = _tcschr(p, '=');
		if (tmpp2)
			*tmpp2++ = 0;
		if (!_tcsicmp(p, option)) {
			if (!tmpp2)
				return 0;
			TCHAR *tmpp3 = _tcschr (tmpp2, ',');
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
	TCHAR *tmpp;
	TCHAR tmpbuf[CONFIG_BLEN];

	if (_tcsncmp (option, _T("input."), 6) == 0 || _tcsncmp(option, _T("input_"), 6) == 0) {
		read_inputdevice_config (p, option, value);
		return 1;
	}

	for (tmpp = option; *tmpp != '\0'; tmpp++)
		if (_istupper (*tmpp))
			*tmpp = _totlower (*tmpp);
	tmpp = _tcschr (option, '.');
	if (tmpp) {
		TCHAR *section = option;
		option = tmpp + 1;
		*tmpp = '\0';
		if (_tcscmp (section, TARGET_NAME) == 0) {
			/* We special case the various path options here.  */
			if (cfgfile_multipath(option, value, _T("rom_path"), &p->path_rom, p)
				|| cfgfile_multipath(option, value, _T("floppy_path"), &p->path_floppy, p)
				|| cfgfile_multipath(option, value, _T("cd_path"), &p->path_cd, p)
				|| cfgfile_multipath(option, value, _T("hardfile_path"), &p->path_hardfile, p))
				return 1;
			return target_parse_option(p, option, value, CONFIG_TYPE_HOST);
		}
		return 0;
	}

	for (i = 0; i < MAX_SPARE_DRIVES; i++) {
		_sntprintf (tmpbuf, sizeof tmpbuf, _T("diskimage%d"), i);
		if (cfgfile_string(option, value, tmpbuf, p->dfxlist[i], sizeof p->dfxlist[i] / sizeof(TCHAR))) {
			return 1;
		}
	}

	for (i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		TCHAR tmp[20];
		_sntprintf (tmp, sizeof tmp, _T("cdimage%d"), i);
		if (!_tcsicmp (option, tmp)) {
			if (!_tcsicmp (value, _T("autodetect"))) {
				p->cdslots[i].type = SCSI_UNIT_DEFAULT;
				p->cdslots[i].inuse = true;
				p->cdslots[i].name[0] = 0;
			} else {
				p->cdslots[i].delayed = false;
				TCHAR *path = value;
				TCHAR *next = _tcsrchr(value, ',');
				if (value[0] == '\"') {
					const TCHAR *end;
					TCHAR *n = cfgfile_unescape(value, &end, 0, true);
					if (n) {
						path = n;
						next = _tcsrchr(value + (end - value), ',');
					}
				} else {
					if (next) {
						TCHAR *dot = _tcsrchr(value, '.');
						// assume '.' = file extension, if ',' is earlier, assume it is part of file name, not extra string.
						if (dot && dot > value && dot > next) {
							next = nullptr;
						}
					}
				}
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
				if (_tcslen(path) > 0) {
					_tcsncpy (p->cdslots[i].name, path, sizeof(p->cdslots[i].name) / sizeof (TCHAR));
				}
				p->cdslots[i].name[sizeof(p->cdslots[i].name) / sizeof(TCHAR) - 1] = 0;
				p->cdslots[i].inuse = true;
				p->cdslots[i].type = type;
				if (path[0] == 0 || !_tcsicmp(path, _T("empty")) || !_tcscmp(path, _T("."))) {
					p->cdslots[i].name[0] = 0;
#ifndef AMIBERRY // Don't disable the CD drive
					p->cdslots[i].inuse = false;
#endif
				}
				if (path != value) {
					xfree(path);
				}
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

#ifdef WITH_LUA
	if (!_tcsicmp (option, _T("lua"))) {
		for (i = 0; i < MAX_LUA_STATES; i++) {
			if (!p->luafiles[i][0]) {
				_tcscpy (p->luafiles[i], value);
				break;
			}
		}
		return 1;
	}
#endif

	if (cfgfile_strval (option, value, _T("gfx_autoresolution_min_vertical"), &p->gfx_autoresolution_minv, vertmode, 0)) {
		p->gfx_autoresolution_minv--;
		return 1;
	}
	if (cfgfile_strval (option, value, _T("gfx_autoresolution_min_horizontal"), &p->gfx_autoresolution_minh, horizmode, 0)) {
		p->gfx_autoresolution_minh--;
		return 1;
	}
	if (!_tcsicmp (option, _T("gfx_autoresolution"))) {
		p->gfx_autoresolution = 0;
		cfgfile_intval (option, value, _T("gfx_autoresolution"), &p->gfx_autoresolution, 1);
		if (!p->gfx_autoresolution) {
			v = cfgfile_yesno (option, value, _T("gfx_autoresolution"), &vb);
			if (v > 0)
				p->gfx_autoresolution = vb ? 10 : 0;
		}
		return 1;
	}

	if (cfgfile_intval (option, value, _T("sound_frequency"), &p->sound_freq, 1)
		|| cfgfile_intval (option, value, _T("sound_max_buff"), &p->sound_maxbsiz, 1)
		|| cfgfile_intval (option, value, _T("state_replay_rate"), &p->statecapturerate, 1)
		|| cfgfile_intval (option, value, _T("state_replay_buffers"), &p->statecapturebuffersize, 1)
		|| cfgfile_yesno (option, value, _T("state_replay_autoplay"), &p->inprec_autoplay)
		|| cfgfile_intval (option, value, _T("sound_frequency"), &p->sound_freq, 1)
		|| cfgfile_intval (option, value, _T("sound_volume"), &p->sound_volume_master, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_paula"), &p->sound_volume_paula, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_cd"), &p->sound_volume_cd, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_ahi"), &p->sound_volume_board, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_midi"), &p->sound_volume_midi, 1)
		|| cfgfile_intval (option, value, _T("sound_volume_genlock"), &p->sound_volume_genlock, 1)
		|| cfgfile_intval (option, value, _T("sound_stereo_separation"), &p->sound_stereo_separation, 1)
		|| cfgfile_intval (option, value, _T("sound_stereo_mixing_delay"), &p->sound_mixed_stereo_delay, 1)
		|| cfgfile_intval (option, value, _T("sampler_frequency"), &p->sampler_freq, 1)
		|| cfgfile_intval (option, value, _T("sampler_buffer"), &p->sampler_buffer, 1)
		|| cfgfile_intval(option, value, _T("warp_limit"), &p->turbo_emulation_limit, 1)
		|| cfgfile_intval(option, value, _T("power_led_dim"), &p->power_led_dim, 1)
		|| cfgfile_intval(option, value, _T("warpboot_delay"), &p->turbo_emulation_limit, 1)

		|| cfgfile_intval(option, value, _T("gfx_frame_slices"), &p->gfx_display_sections, 1)
		|| cfgfile_intval(option, value, _T("gfx_framerate"), &p->gfx_framerate, 1)
		|| cfgfile_intval(option, value, _T("gfx_x_windowed"), &p->gfx_monitor[0].gfx_size_win.x, 1)
		|| cfgfile_intval(option, value, _T("gfx_y_windowed"), &p->gfx_monitor[0].gfx_size_win.y, 1)
		|| cfgfile_intval(option, value, _T("gfx_left_windowed"), &p->gfx_monitor[0].gfx_size_win.y, 1)
		|| cfgfile_intval(option, value, _T("gfx_top_windowed"), &p->gfx_monitor[0].gfx_size_win.x, 1)
		|| cfgfile_intval(option, value, _T("gfx_refreshrate"), &p->gfx_apmode[APMODE_NATIVE].gfx_refreshrate, 1)
		|| cfgfile_intval(option, value, _T("gfx_refreshrate_rtg"), &p->gfx_apmode[APMODE_RTG].gfx_refreshrate, 1)
		|| cfgfile_intval(option, value, _T("gfx_autoresolution_delay"), &p->gfx_autoresolution_delay, 1)
		|| cfgfile_intval(option, value, _T("gfx_backbuffers"), &p->gfx_apmode[APMODE_NATIVE].gfx_backbuffers, 1)
		|| cfgfile_intval(option, value, _T("gfx_backbuffers_rtg"), &p->gfx_apmode[APMODE_RTG].gfx_backbuffers, 1)
		|| cfgfile_yesno(option, value, _T("gfx_interlace"), &p->gfx_apmode[APMODE_NATIVE].gfx_interlaced)
		|| cfgfile_yesno(option, value, _T("gfx_interlace_rtg"), &p->gfx_apmode[APMODE_RTG].gfx_interlaced)
		|| cfgfile_yesno(option, value, _T("gfx_vrr_monitor"), &p->gfx_variable_sync)
		|| cfgfile_yesno(option, value, _T("gfx_resize_windowed"), &p->gfx_windowed_resize)
		|| cfgfile_intval(option, value, _T("gfx_black_frame_insertion_ratio"), &p->lightboost_strobo_ratio, 1)

		|| cfgfile_intval (option, value, _T("gfx_center_horizontal_position"), &p->gfx_xcenter_pos, 1)
		|| cfgfile_intval (option, value, _T("gfx_center_vertical_position"), &p->gfx_ycenter_pos, 1)
		|| cfgfile_intval (option, value, _T("gfx_center_horizontal_size"), &p->gfx_xcenter_size, 1)
		|| cfgfile_intval (option, value, _T("gfx_center_vertical_size"), &p->gfx_ycenter_size, 1)

		|| cfgfile_intval (option, value, _T("filesys_max_size"), &p->filesys_limit, 1)
		|| cfgfile_intval (option, value, _T("filesys_max_name_length"), &p->filesys_max_name, 1)
		|| cfgfile_intval (option, value, _T("filesys_max_file_size"), &p->filesys_max_file_size, 1)
		|| cfgfile_yesno (option, value, _T("filesys_inject_icons"), &p->filesys_inject_icons)
		|| cfgfile_string (option, value, _T("filesys_inject_icons_drawer"), p->filesys_inject_icons_drawer, sizeof p->filesys_inject_icons_drawer / sizeof (TCHAR))
		|| cfgfile_string (option, value, _T("filesys_inject_icons_project"), p->filesys_inject_icons_project, sizeof p->filesys_inject_icons_project / sizeof (TCHAR))
		|| cfgfile_string (option, value, _T("filesys_inject_icons_tool"), p->filesys_inject_icons_tool, sizeof p->filesys_inject_icons_tool / sizeof (TCHAR))

		|| cfgfile_intval(option, value, _T("gfx_luminance"), &p->gfx_luminance, 1)
		|| cfgfile_intval(option, value, _T("gfx_contrast"), &p->gfx_contrast, 1)
		|| cfgfile_intval(option, value, _T("gfx_gamma"), &p->gfx_gamma, 1)
		|| cfgfile_intval(option, value, _T("gfx_gamma_r"), &p->gfx_gamma_ch[0], 1)
		|| cfgfile_intval(option, value, _T("gfx_gamma_g"), &p->gfx_gamma_ch[1], 1)
		|| cfgfile_intval(option, value, _T("gfx_gamma_b"), &p->gfx_gamma_ch[2], 1)
		|| cfgfile_floatval(option, value, _T("rtg_vert_zoom_multf"), &p->rtg_vert_zoom_mult)
		|| cfgfile_floatval(option, value, _T("rtg_horiz_zoom_multf"), &p->rtg_horiz_zoom_mult)
		|| cfgfile_intval(option, value, _T("gfx_horizontal_extra"), &p->gfx_extrawidth, 1)
		|| cfgfile_intval(option, value, _T("gfx_vertical_extra"), &p->gfx_extraheight, 1)
		|| cfgfile_intval(option, value, _T("gfx_monitorblankdelay"), &p->gfx_monitorblankdelay, 1)
		|| cfgfile_intval(option, value, _T("gfx_bordercolor"), &p->gfx_bordercolor, 1)

		|| cfgfile_intval (option, value, _T("floppy0sound"), &p->floppyslots[0].dfxclick, 1)
		|| cfgfile_intval (option, value, _T("floppy1sound"), &p->floppyslots[1].dfxclick, 1)
		|| cfgfile_intval (option, value, _T("floppy2sound"), &p->floppyslots[2].dfxclick, 1)
		|| cfgfile_intval (option, value, _T("floppy3sound"), &p->floppyslots[3].dfxclick, 1)
		|| cfgfile_intval (option, value, _T("floppy0soundvolume_disk"), &p->dfxclickvolume_disk[0], 1)
		|| cfgfile_intval (option, value, _T("floppy1soundvolume_disk"), &p->dfxclickvolume_disk[1], 1)
		|| cfgfile_intval (option, value, _T("floppy2soundvolume_disk"), &p->dfxclickvolume_disk[2], 1)
		|| cfgfile_intval (option, value, _T("floppy3soundvolume_disk"), &p->dfxclickvolume_disk[3], 1)
		|| cfgfile_intval (option, value, _T("floppy0soundvolume_empty"), &p->dfxclickvolume_empty[0], 1)
		|| cfgfile_intval (option, value, _T("floppy1soundvolume_empty"), &p->dfxclickvolume_empty[1], 1)
		|| cfgfile_intval (option, value, _T("floppy2soundvolume_empty"), &p->dfxclickvolume_empty[2], 1)
		|| cfgfile_intval (option, value, _T("floppy3soundvolume_empty"), &p->dfxclickvolume_empty[3], 1)
		|| cfgfile_intval (option, value, _T("floppy_channel_mask"), &p->dfxclickchannelmask, 1))
		return 1;

	if (cfgfile_string(option, value, _T("floppy0soundext"), p->floppyslots[0].dfxclickexternal, sizeof p->floppyslots[0].dfxclickexternal / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("floppy1soundext"), p->floppyslots[1].dfxclickexternal, sizeof p->floppyslots[1].dfxclickexternal / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("floppy2soundext"), p->floppyslots[2].dfxclickexternal, sizeof p->floppyslots[2].dfxclickexternal / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("floppy3soundext"), p->floppyslots[3].dfxclickexternal, sizeof p->floppyslots[3].dfxclickexternal / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("debugging_options"), p->debugging_options, sizeof p->debugging_options / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("config_window_title"), p->config_window_title, sizeof p->config_window_title / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("config_info"), p->info, sizeof p->info / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("config_description"), p->description, sizeof p->description / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("config_category"), p->category, sizeof p->category / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("config_tags"), p->tags, sizeof p->tags / sizeof(TCHAR)))
		return 1;

	if (cfgfile_yesno(option, value, _T("use_debugger"), &p->start_debugger)
		|| cfgfile_yesno(option, value, _T("floppy0wp"), &p->floppyslots[0].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("floppy1wp"), &p->floppyslots[1].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("floppy2wp"), &p->floppyslots[2].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("floppy3wp"), &p->floppyslots[3].forcedwriteprotect)
		|| cfgfile_yesno(option, value, _T("sampler_stereo"), &p->sampler_stereo)
		|| cfgfile_yesno(option, value, _T("sound_auto"), &p->sound_auto)
		|| cfgfile_yesno(option, value, _T("sound_volcnt"), &p->sound_volcnt)
		|| cfgfile_yesno(option, value, _T("sound_stereo_swap_paula"), &p->sound_stereo_swap_paula)
		|| cfgfile_yesno(option, value, _T("sound_stereo_swap_ahi"), &p->sound_stereo_swap_ahi)
		|| cfgfile_yesno(option, value, _T("debug_mem"), &p->debug_mem)
		|| cfgfile_yesno(option, value, _T("log_illegal_mem"), &p->illegal_mem)
		|| cfgfile_yesno(option, value, _T("filesys_no_fsdb"), &p->filesys_no_uaefsdb)
		|| cfgfile_yesno(option, value, _T("gfx_monochrome"), &p->gfx_grayscale)
		|| cfgfile_yesno(option, value, _T("gfx_blacker_than_black"), &p->gfx_blackerthanblack)
		|| cfgfile_yesno(option, value, _T("gfx_black_frame_insertion"), &p->lightboost_strobo)
		|| cfgfile_yesno(option, value, _T("gfx_flickerfixer"), &p->gfx_scandoubler)
		|| cfgfile_yesno(option, value, _T("gfx_autoresolution_vga"), &p->gfx_autoresolution_vga)
		|| cfgfile_yesno(option, value, _T("show_refresh_indicator"), &p->refresh_indicator)
		|| cfgfile_yesno(option, value, _T("warp"), &p->turbo_emulation)
		|| cfgfile_yesno(option, value, _T("warpboot"), &p->turbo_boot)
		|| cfgfile_yesno(option, value, _T("headless"), &p->headless)
		|| cfgfile_yesno(option, value, _T("clipboard_sharing"), &p->clipboard_sharing)
		|| cfgfile_yesno(option, value, _T("native_code"), &p->native_code)
		|| cfgfile_yesno(option, value, _T("tablet_library"), &p->tablet_library)
		|| cfgfile_yesno(option, value, _T("cputester"), &p->cputester)
		|| cfgfile_yesno(option, value, _T("bsdsocket_emu"), &p->socket_emu))
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
		|| cfgfile_strval (option, value, _T("gfx_lores_mode"), &p->gfx_lores_mode, loresmode, 0)
		|| cfgfile_strval (option, value, _T("gfx_fullscreen_amiga"), &p->gfx_apmode[APMODE_NATIVE].gfx_fullscreen, fullmodes, 0)
		|| cfgfile_strval (option, value, _T("gfx_fullscreen_picasso"), &p->gfx_apmode[APMODE_RTG].gfx_fullscreen, fullmodes, 0)
		|| cfgfile_strval (option, value, _T("gfx_center_horizontal"), &p->gfx_xcenter, centermode1, 1)
		|| cfgfile_strval (option, value, _T("gfx_center_vertical"), &p->gfx_ycenter, centermode1, 1)
		|| cfgfile_strval (option, value, _T("gfx_center_horizontal"), &p->gfx_xcenter, centermode2, 0)
		|| cfgfile_strval (option, value, _T("gfx_center_vertical"), &p->gfx_ycenter, centermode2, 0)
		|| cfgfile_strval (option, value, _T("gfx_colour_mode"), &p->color_mode, colormode1, 1)
		|| cfgfile_strval (option, value, _T("gfx_colour_mode"), &p->color_mode, colormode2, 0)
		|| cfgfile_strval (option, value, _T("gfx_color_mode"), &p->color_mode, colormode1, 1)
		|| cfgfile_strval (option, value, _T("gfx_color_mode"), &p->color_mode, colormode2, 0)
		|| cfgfile_strval (option, value, _T("gfx_max_horizontal"), &p->gfx_max_horizontal, maxhoriz, 0)
		|| cfgfile_strval (option, value, _T("gfx_max_vertical"), &p->gfx_max_vertical, maxvert, 0)
		|| cfgfile_strval(option, value, _T("gfx_api"), &p->gfx_api, filterapi, 0)
		|| cfgfile_strval(option, value, _T("gfx_api_options"), &p->gfx_api_options, filterapiopts, 0)
		|| cfgfile_strval(option, value, _T("gfx_atari_palette_fix"), &p->gfx_threebitcolors, threebitcolors, 0)
		|| cfgfile_strval(option, value, _T("gfx_overscanmode"), &p->gfx_overscanmode, overscanmodes, 0)
		|| cfgfile_strval(option, value, _T("magic_mousecursor"), &p->input_magic_mouse_cursor, magiccursors, 0)
		|| cfgfile_strval (option, value, _T("absolute_mouse"), &p->input_tablet, abspointers, 0))
		return 1;

	if (cfgfile_intval(option, value, _T("gfx_rotation"), &p->gfx_rotation, 1)) {
		p->gf[GF_NORMAL].gfx_filter_rotation = p->gfx_rotation;
		p->gf[GF_INTERLACE].gfx_filter_rotation = p->gfx_rotation;
		return 1;
	}

#ifdef DEBUGGER
	if (cfgfile_multichoice(option, value, _T("debugging_features"), &p->debugging_features, debugfeatures))
		return 1;
#endif

	if (cfgfile_yesno(option, value, _T("gfx_api_hdr"), &vb)) {
		if (vb && p->gfx_api == 2) {
			p->gfx_api = 3;
		}
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("magic_mouse"), &vb)) {
		if (vb)
			p->input_mouse_untrap |= MOUSEUNTRAP_MAGIC;
		else
			p->input_mouse_untrap &= ~MOUSEUNTRAP_MAGIC;
		return 1;
	}

#ifdef GFXFILTER
	for (int j = 0; j < MAX_FILTERDATA; j++) {
		struct gfx_filterdata *gf = &p->gf[j];
		const TCHAR *ext = j == 0 ? NULL : (j == 1 ? _T("_rtg") : _T("_lace"));
		if (cfgfile_strval (option, value, _T("gfx_filter_autoscale"), ext, &gf->gfx_filter_autoscale, j != GF_RTG ? autoscale : autoscale_rtg, 0)
			|| cfgfile_strval (option, value, _T("gfx_filter_keep_aspect"), ext, &gf->gfx_filter_keep_aspect, aspects, 0)
			|| cfgfile_strval (option, value, _T("gfx_filter_autoscale_limit"), ext, &gf->gfx_filter_integerscalelimit, autoscalelimit, 0)
			|| cfgfile_floatval(option, value, _T("gfx_filter_vert_zoomf"), ext, &gf->gfx_filter_vert_zoom)
			|| cfgfile_floatval(option, value, _T("gfx_filter_horiz_zoomf"), ext, &gf->gfx_filter_horiz_zoom)
			|| cfgfile_floatval(option, value, _T("gfx_filter_vert_zoom_multf"), ext, &gf->gfx_filter_vert_zoom_mult)
			|| cfgfile_floatval(option, value, _T("gfx_filter_horiz_zoom_multf"), ext, &gf->gfx_filter_horiz_zoom_mult)
			|| cfgfile_floatval(option, value, _T("gfx_filter_vert_offsetf"), ext, &gf->gfx_filter_vert_offset)
			|| cfgfile_floatval(option, value, _T("gfx_filter_horiz_offsetf"), ext, &gf->gfx_filter_horiz_offset)
			|| cfgfile_intval(option, value, _T("gfx_filter_left_border"), ext, &gf->gfx_filter_left_border, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_right_border"), ext, &gf->gfx_filter_right_border, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_top_border"), ext, &gf->gfx_filter_top_border, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_bottom_border"), ext, &gf->gfx_filter_bottom_border, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_scanlines"), ext, &gf->gfx_filter_scanlines, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_scanlinelevel"), ext, &gf->gfx_filter_scanlinelevel, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_scanlineratio"), ext, &gf->gfx_filter_scanlineratio, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_scanlineoffset"), ext, &gf->gfx_filter_scanlineoffset, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_luminance"), ext, &gf->gfx_filter_luminance, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_contrast"), ext, &gf->gfx_filter_contrast, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_saturation"), ext, &gf->gfx_filter_saturation, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_gamma"), ext, &gf->gfx_filter_gamma, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_gamma_r"), ext, &gf->gfx_filter_gamma_ch[0], 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_gamma_g"), ext, &gf->gfx_filter_gamma_ch[1], 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_gamma_b"), ext, &gf->gfx_filter_gamma_ch[2], 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_blur"), ext, &gf->gfx_filter_blur, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_noise"), ext, &gf->gfx_filter_noise, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_bilinear"), ext, &gf->gfx_filter_bilinear, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_keep_autoscale_aspect"), ext, &gf->gfx_filter_keep_autoscale_aspect, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_enable"), ext, &gf->enable, 1)
			|| cfgfile_intval(option, value, _T("gfx_filter_rotation"), ext, &gf->gfx_filter_rotation, 1)
			|| cfgfile_string(option, value, _T("gfx_filter_mask"), ext, gf->gfx_filtermask[2 * MAX_FILTERSHADERS], sizeof gf->gfx_filtermask[2 * MAX_FILTERSHADERS] / sizeof (TCHAR)))
			{
				return 1;
			}
	}
#endif

	if (cfgfile_intval (option, value, _T("floppy_volume"), &v, 1)) {
		for (int i = 0; i < 4; i++) {
			p->dfxclickvolume_disk[i] = v;
			p->dfxclickvolume_empty[i] = v;
		}
		return 1;
	}

	if (_tcscmp (option, _T("gfx_width_windowed")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_monitor[0].gfx_size_win.width = 0;
			p->gfx_monitor[0].gfx_size_win.height = 0;
		} else {
			cfgfile_intval (option, value, _T("gfx_width_windowed"), &p->gfx_monitor[0].gfx_size_win.width, 1);
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_height_windowed")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_monitor[0].gfx_size_win.width = 0;
			p->gfx_monitor[0].gfx_size_win.height = 0;
		} else {
			cfgfile_intval (option, value, _T("gfx_height_windowed"), &p->gfx_monitor[0].gfx_size_win.height, 1);
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_width_fullscreen")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_monitor[0].gfx_size_fs.width = 0;
			p->gfx_monitor[0].gfx_size_fs.height = 0;
			p->gfx_monitor[0].gfx_size_fs.special = WH_NATIVE;
		} else {
			cfgfile_intval (option, value, _T("gfx_width_fullscreen"), &p->gfx_monitor[0].gfx_size_fs.width, 1);
			p->gfx_monitor[0].gfx_size_fs.special = 0;
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_height_fullscreen")) == 0) {
		if (!_tcscmp (value, _T("native"))) {
			p->gfx_monitor[0].gfx_size_fs.width = 0;
			p->gfx_monitor[0].gfx_size_fs.height = 0;
			p->gfx_monitor[0].gfx_size_fs.special = WH_NATIVE;
		} else {
			cfgfile_intval (option, value, _T("gfx_height_fullscreen"), &p->gfx_monitor[0].gfx_size_fs.height, 1);
			p->gfx_monitor[0].gfx_size_fs.special = 0;
		}
		return 1;
	}

	if (cfgfile_intval (option, value, _T("gfx_display"), &p->gfx_apmode[APMODE_NATIVE].gfx_display, 1)) {
		p->gfx_apmode[APMODE_RTG].gfx_display = p->gfx_apmode[APMODE_NATIVE].gfx_display;
		return 1;
	}
	if (cfgfile_intval (option, value, _T("gfx_display_rtg"), &p->gfx_apmode[APMODE_RTG].gfx_display, 1)) {
		return 1;
	}
	if (_tcscmp (option, _T("gfx_display_friendlyname")) == 0 || _tcscmp (option, _T("gfx_display_name")) == 0) {
		TCHAR tmp[MAX_DPATH];
		if (cfgfile_string (option, value, _T("gfx_display_friendlyname"), tmp, sizeof tmp / sizeof (TCHAR))) {
			int num = target_get_display (tmp);
			if (num >= 0)
				p->gfx_apmode[APMODE_RTG].gfx_display = p->gfx_apmode[APMODE_NATIVE].gfx_display = num;
		}
		if (cfgfile_string (option, value, _T("gfx_display_name"), tmp, sizeof tmp / sizeof (TCHAR))) {
			int num = target_get_display (tmp);
			if (num >= 0)
				p->gfx_apmode[APMODE_RTG].gfx_display = p->gfx_apmode[APMODE_NATIVE].gfx_display = num;
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_display_friendlyname_rtg")) == 0 || _tcscmp (option, _T("gfx_display_name_rtg")) == 0) {
		TCHAR tmp[MAX_DPATH];
		if (cfgfile_string (option, value, _T("gfx_display_friendlyname_rtg"), tmp, sizeof tmp / sizeof (TCHAR))) {
			int num = target_get_display (tmp);
			if (num >= 0)
				p->gfx_apmode[APMODE_RTG].gfx_display = num;
		}
		if (cfgfile_string (option, value, _T("gfx_display_name_rtg"), tmp, sizeof tmp / sizeof (TCHAR))) {
			int num = target_get_display (tmp);
			if (num >= 0)
				p->gfx_apmode[APMODE_RTG].gfx_display = num;
		}
		return 1;
	}

	if (_tcscmp (option, _T("gfx_linemode")) == 0) {
		int v = 0;
		p->gfx_vresolution = VRES_DOUBLE;
		p->gfx_pscanlines = 0;
		p->gfx_iscanlines = 0;
		if (cfgfile_strval(option, value, _T("gfx_linemode"), &v, linemode, 0)) {
			p->gfx_vresolution = VRES_NONDOUBLE;
			if (v > 0) {
				p->gfx_iscanlines = (v - 1) / 4;
				p->gfx_pscanlines = (v - 1) % 4;
				p->gfx_vresolution = VRES_DOUBLE;
			}
		}
		return 1;
	}
	if (_tcscmp (option, _T("gfx_vsync")) == 0) {
		if (cfgfile_strval (option, value, _T("gfx_vsync"), &p->gfx_apmode[APMODE_NATIVE].gfx_vsync, vsyncmodes, 0) >= 0) {
			return 1;
		}
		return cfgfile_yesno (option, value, _T("gfx_vsync"), &p->gfx_apmode[APMODE_NATIVE].gfx_vsync);
	}
	if (_tcscmp (option, _T("gfx_vsync_picasso")) == 0) {
		if (cfgfile_strval (option, value, _T("gfx_vsync_picasso"), &p->gfx_apmode[APMODE_RTG].gfx_vsync, vsyncmodes, 0) >= 0) {
			return 1;
		}
		return cfgfile_yesno (option, value, _T("gfx_vsync_picasso"), &p->gfx_apmode[APMODE_RTG].gfx_vsync);
	}
	if (cfgfile_strval (option, value, _T("gfx_vsyncmode"), &p->gfx_apmode[APMODE_NATIVE].gfx_vsyncmode, vsyncmodes2, 0))
		return 1;
	if (cfgfile_strval (option, value, _T("gfx_vsyncmode_picasso"), &p->gfx_apmode[APMODE_RTG].gfx_vsyncmode, vsyncmodes2, 0))
		return 1;

	if (cfgfile_yesno (option, value, _T("show_leds"), &vb)) {
		if (vb)
			p->leds_on_screen |= STATUSLINE_CHIPSET;
		else
			p->leds_on_screen &= ~STATUSLINE_CHIPSET;
		return 1;
	}
	if (cfgfile_yesno (option, value, _T("show_leds_rtg"), &vb)) {
		if (vb)
			p->leds_on_screen |= STATUSLINE_RTG;
		else
			p->leds_on_screen &= ~STATUSLINE_RTG;
		return 1;
	}
	if (_tcscmp(option, _T("show_leds_size")) == 0 || _tcscmp(option, _T("show_leds_size_rtg")) == 0) {
		int idx = _tcscmp(option, _T("show_leds_size")) == 0 ? 0 : 1;
		p->leds_on_screen_multiplier[idx] = 0;
		float f = 0;
		if (cfgfile_floatval(option, value, option, &f)) {
			p->leds_on_screen_multiplier[idx] = static_cast<int>(f * 100);
		}
		return 1;
	}
	if (_tcscmp (option, _T("show_leds_enabled")) == 0 || _tcscmp (option, _T("show_leds_enabled_rtg")) == 0) {
		TCHAR tmp[MAX_DPATH];
		int idx = _tcscmp (option, _T("show_leds_enabled")) == 0 ? 0 : 1;
		p->leds_on_screen_mask[idx] = 0;
		_tcscpy (tmp, value);
		_tcscat (tmp, _T(","));
		TCHAR *s = tmp;
		for (;;) {
			TCHAR *s2 = s;
			TCHAR *s3 = _tcschr (s, ':');
			s = _tcschr (s, ',');
			if (!s)
				break;
			if (s3 && s3 < s)
				s = s3;
			*s = 0;
			for (int i = 0; leds[i]; i++) {
				if (!_tcsicmp (s2, leds[i])) {
					p->leds_on_screen_mask[idx] |= 1 << i;
				}
			}
			s++;
		}
		return 1;
	}

	if (!_tcscmp (option, _T("osd_position"))) {
		TCHAR *s = value;
		p->osd_pos.x = 0;
		p->osd_pos.y = 0;
		while (s) {
			if (!_tcschr (s, ':'))
				break;
			p->osd_pos.x =  static_cast<int>((_tstof(s) * 10.0f));
			s = _tcschr (s, ':');
			if (!s)
				break;
			if (s[-1] == '%')
				p->osd_pos.x += 30000;
			s++;
			p->osd_pos.y = static_cast<int>((_tstof(s) * 10.0f));
			s += _tcslen (s);
			if (s[-1] == '%')
				p->osd_pos.y += 30000;
			break;
		}
		return 1;
	}

#ifdef GFXFILTER
	for (int j = 0; j < MAX_FILTERDATA; j++) {
		struct gfx_filterdata *gf = &p->gf[j];
		if ((j == 0 && _tcscmp (option, _T("gfx_filter_overlay")) == 0) || (j == 1 && _tcscmp(option, _T("gfx_filter_overlay_rtg")) == 0) || (j == 2 && _tcscmp(option, _T("gfx_filter_overlay_lace")) == 0)) {
			TCHAR *s = _tcschr (value, ',');
			gf->gfx_filteroverlay_overscan = 0;
			gf->gfx_filteroverlay_pos.x = 0;
			gf->gfx_filteroverlay_pos.y = 0;
			gf->gfx_filteroverlay_pos.width = 0;
			gf->gfx_filteroverlay_pos.height = 0;
			if (s)
				*s = 0;
			while (s) {
				*s++ = 0;
				gf->gfx_filteroverlay_overscan = _tstol (s);
				s = _tcschr (s, ':');
				if (!s)
					break;
				break;
			}
			_tcsncpy (gf->gfx_filteroverlay, value, sizeof gf->gfx_filteroverlay / sizeof (TCHAR) - 1);
			gf->gfx_filteroverlay[sizeof gf->gfx_filteroverlay / sizeof (TCHAR) - 1] = 0;
			return 1;
		}

		if ((j == 0 && (_tcscmp (option, _T("gfx_filtermask_pre")) == 0 || _tcscmp (option, _T("gfx_filtermask_post")) == 0)) ||
			(j == 1 && (_tcscmp(option, _T("gfx_filtermask_pre_rtg")) == 0 || _tcscmp(option, _T("gfx_filtermask_post_rtg")) == 0)) ||
			(j == 2 && (_tcscmp(option, _T("gfx_filtermask_pre_lace")) == 0 || _tcscmp(option, _T("gfx_filtermask_post_lace")) == 0))) {
			if (_tcscmp(option, _T("gfx_filtermask_pre")) == 0 || _tcscmp(option, _T("gfx_filtermask_pre_rtg")) == 0 || _tcscmp(option, _T("gfx_filtermask_pre_lace")) == 0) {
				for (int i = 0; i < MAX_FILTERSHADERS; i++) {
					if (gf->gfx_filtermask[i][0] == 0) {
						_tcscpy(gf->gfx_filtermask[i], value);
						break;
					}
				}
			} else {
				for (int i = 0; i < MAX_FILTERSHADERS; i++) {
					if (gf->gfx_filtermask[i + MAX_FILTERSHADERS][0] == 0) {
						_tcscpy(gf->gfx_filtermask[i + MAX_FILTERSHADERS], value);
						break;
					}
				}
			}
			return 1;
		}

		if ((j == 0 && (_tcscmp (option, _T("gfx_filter_pre")) == 0 || _tcscmp (option, _T("gfx_filter_post")) == 0)) ||
			(j == 1 && (_tcscmp (option, _T("gfx_filter_pre_rtg")) == 0 || _tcscmp (option, _T("gfx_filter_post_rtg")) == 0)) ||
			(j == 2 && (_tcscmp(option, _T("gfx_filter_pre_lace")) == 0 || _tcscmp(option, _T("gfx_filter_post_lace")) == 0))) {
			TCHAR *s = _tcschr(value, ':');
			if (s) {
				*s++ = 0;
				if (!_tcscmp (value, _T("D3D"))) {
					if (!p->gfx_api)
						p->gfx_api = 1;
					if (_tcscmp(option, _T("gfx_filter_pre")) == 0 || _tcscmp(option, _T("gfx_filter_pre_rtg")) == 0 || _tcscmp(option, _T("gfx_filter_pre_lace")) == 0) {
						for (int i = 0; i < MAX_FILTERSHADERS; i++) {
							if (gf->gfx_filtershader[i][0] == 0) {
								_tcscpy(gf->gfx_filtershader[i], s);
								break;
							}
						}
					} else {
						for (int i = 0; i < MAX_FILTERSHADERS; i++) {
							if (gf->gfx_filtershader[i + MAX_FILTERSHADERS][0] == 0) {
								_tcscpy(gf->gfx_filtershader[i + MAX_FILTERSHADERS], s);
								break;
							}
						}
					}
				}
			}
			return 1;
		}

		if ((j == 0 && _tcscmp (option, _T("gfx_filter")) == 0) || (j == 1 && _tcscmp(option, _T("gfx_filter_rtg")) == 0) || (j == 2 && _tcscmp(option, _T("gfx_filter_lace")) == 0)) {
			TCHAR *s = _tcschr (value, ':');
			gf->gfx_filter = 0;
			if (s) {
				*s++ = 0;
				if (!_tcscmp (value, _T("D3D"))) {
					if (!p->gfx_api)
						p->gfx_api = 1;
					_tcscpy (gf->gfx_filtershader[2 * MAX_FILTERSHADERS], s);
					for (int i = 0; i < 2 * MAX_FILTERSHADERS; i++) {
						if (!_tcsicmp (gf->gfx_filtershader[i], s)) {
							gf->gfx_filtershader[i][0] = 0;
							gf->gfx_filtermask[i][0] = 0;
						}
					}
				}
			}
			if (!_tcscmp(value, _T("none"))) {
				gf->gfx_filtershader[2 * MAX_FILTERSHADERS][0] = 0;
				for (int i = 0; i < 2 * MAX_FILTERSHADERS; i++) {
					gf->gfx_filtershader[i][0] = 0;
					gf->gfx_filtermask[i][0] = 0;
				}
			} else if (!_tcscmp (value, _T("direct3d"))) {
				if (!p->gfx_api)
					p->gfx_api = 1; // forwards compatibiity
			} else {
				int i = 0;
				while(uaefilters[i].name) {
					if (!_tcscmp (uaefilters[i].cfgname, value)) {
						gf->gfx_filter = uaefilters[i].type;
						break;
					}
					i++;
				}
			}
			return 1;
		}
		if (j == 0 && _tcscmp(option, _T("gfx_filter_mode")) == 0) {
			cfgfile_strval(option, value, _T("gfx_filter_mode"), &gf->gfx_filter_filtermodeh, filtermode2, 0);
			return 1;
		}
		if (j == 1 && _tcscmp(option, _T("gfx_filter_mode_rtg")) == 0) {
			cfgfile_strval(option, value, _T("gfx_filter_mode_rtg"), &gf->gfx_filter_filtermodeh, filtermode2, 0);
			return 1;
		}
		if (j == 2 && _tcscmp(option, _T("gfx_filter_mode_lace")) == 0) {
			cfgfile_strval(option, value, _T("gfx_filter_mode_lace"), &gf->gfx_filter_filtermodeh, filtermode2, 0);
			return 1;
		}
		if (j == 0 && _tcscmp(option, _T("gfx_filter_mode2")) == 0) {
			cfgfile_strval(option, value, _T("gfx_filter_mode2"), &gf->gfx_filter_filtermodev, filtermode2v, 0);
			return 1;
		}
		if (j == 1 && _tcscmp(option, _T("gfx_filter_mode2_rtg")) == 0) {
			cfgfile_strval(option, value, _T("gfx_filter_mode2_rtg"), &gf->gfx_filter_filtermodev, filtermode2v, 0);
			return 1;
		}
		if (j == 2 && _tcscmp(option, _T("gfx_filter_mode2_lace")) == 0) {
			cfgfile_strval(option, value, _T("gfx_filter_mode2_lace"), &gf->gfx_filter_filtermodev, filtermode2v, 0);
			return 1;
		}

		if ((j == 0 && cfgfile_string (option, value, _T("gfx_filter_aspect_ratio"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) ||
			(j == 1 && cfgfile_string(option, value, _T("gfx_filter_aspect_ratio_rtg"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) ||
			(j == 2 && cfgfile_string(option, value, _T("gfx_filter_aspect_ratio_lace"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR)))) {
			int v1, v2;
			TCHAR *s;

			gf->gfx_filter_aspect = -1;
			v1 = _tstol (tmpbuf);
			s = _tcschr (tmpbuf, ':');
			if (s) {
				v2 = _tstol (s + 1);
				if (v1 < 0 || v2 < 0)
					gf->gfx_filter_aspect = -1;
				else if (v1 == 0 || v2 == 0)
					gf->gfx_filter_aspect = 0;
				else
					gf->gfx_filter_aspect = v1 * ASPECTMULT + v2;
			}
			return 1;
		}
	}
#endif

	if (_tcscmp (option, _T("gfx_width")) == 0 || _tcscmp (option, _T("gfx_height")) == 0) {
		cfgfile_intval (option, value, _T("gfx_width"), &p->gfx_monitor[0].gfx_size_win.width, 1);
		cfgfile_intval (option, value, _T("gfx_height"), &p->gfx_monitor[0].gfx_size_win.height, 1);
		p->gfx_monitor[0].gfx_size_fs.width = p->gfx_monitor[0].gfx_size_win.width;
		p->gfx_monitor[0].gfx_size_fs.height = p->gfx_monitor[0].gfx_size_win.height;
		return 1;
	}

	if (_tcscmp (option, _T("gfx_fullscreen_multi")) == 0 || _tcscmp (option, _T("gfx_windowed_multi")) == 0) {
		TCHAR tmp[256], *tmpp, *tmpp2;
		struct wh *wh = p->gfx_monitor[0].gfx_size_win_xtra;
		if (_tcscmp (option, _T("gfx_fullscreen_multi")) == 0)
			wh = p->gfx_monitor[0].gfx_size_fs_xtra;
		_sntprintf (tmp, sizeof tmp, _T(",%s,"), value);
		tmpp2 = tmp;
		for (i = 0; i < 4; i++) {
			tmpp = _tcschr (tmpp2, ',');
			tmpp++;
			wh[i].width = _tstol (tmpp);
			while (*tmpp != ',' && *tmpp != 'x' && *tmpp != '*')
				tmpp++;
			wh[i].height = _tstol (tmpp + 1);
			tmpp2 = tmpp;
		}
		return 1;
	}

	if (cfgfile_string(option, value, _T("joyportcustom0"), p->jports_custom[0].custom, sizeof p->jports_custom[0].custom / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("joyportcustom1"), p->jports_custom[1].custom, sizeof p->jports_custom[1].custom / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("joyportcustom2"), p->jports_custom[2].custom, sizeof p->jports_custom[2].custom / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("joyportcustom3"), p->jports_custom[3].custom, sizeof p->jports_custom[3].custom / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("joyportcustom4"), p->jports_custom[4].custom, sizeof p->jports_custom[4].custom / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("joyportcustom5"), p->jports_custom[5].custom, sizeof p->jports_custom[5].custom / sizeof(TCHAR)))
		return 1;

	if (_tcscmp (option, _T("joyportfriendlyname0")) == 0 || _tcscmp (option, _T("joyportfriendlyname1")) == 0) {
		inputdevice_joyport_config_store(p, value, _tcscmp (option, _T("joyportfriendlyname0")) == 0 ? 0 : 1, -1, -1, 2);
		return 1;
	}
	if (_tcscmp (option, _T("joyportfriendlyname2")) == 0 || _tcscmp (option, _T("joyportfriendlyname3")) == 0) {
		inputdevice_joyport_config_store(p, value, _tcscmp (option, _T("joyportfriendlyname2")) == 0 ? 2 : 3, -1, -1, 2);
		return 1;
	}
	if (_tcscmp (option, _T("joyportname0")) == 0 || _tcscmp (option, _T("joyportname1")) == 0) {
		inputdevice_joyport_config_store(p, value, _tcscmp (option, _T("joyportname0")) == 0 ? 0 : 1, -1, -1, 1);
		return 1;
	}
	if (_tcscmp (option, _T("joyportname2")) == 0 || _tcscmp (option, _T("joyportname3")) == 0) {
		inputdevice_joyport_config_store(p, value, _tcscmp (option, _T("joyportname2")) == 0 ? 2 : 3, -1, -1, 1);
		return 1;
	}
	if (_tcscmp (option, _T("joyport0")) == 0 || _tcscmp (option, _T("joyport1")) == 0) {
		int port = _tcscmp (option, _T("joyport0")) == 0 ? 0 : 1;
		inputdevice_joyport_config_store(p, _T(""), port, -1, -1, 1);
		inputdevice_joyport_config_store(p, _T(""), port, -1, -1, 2);
		inputdevice_joyport_config_store(p, value, port, -1, -1, 0);
		return 1;
	}
	if (_tcscmp (option, _T("joyport2")) == 0 || _tcscmp (option, _T("joyport3")) == 0) {
		int port = _tcscmp (option, _T("joyport2")) == 0 ? 2 : 3;
		inputdevice_joyport_config_store(p, _T(""), port, -1, -1, 1);
		inputdevice_joyport_config_store(p, _T(""), port, -1, -1, 2);
		inputdevice_joyport_config_store(p, value, port, -1, -1, 0);
		return 1;
	}
	if (cfgfile_strval(option, value, _T("joyport0mode"), &p->jports[0].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport1mode"), &p->jports[1].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport2mode"), &p->jports[2].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport3mode"), &p->jports[3].mode, joyportmodes, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport0submode"), &p->jports[0].submode, joyportsubmodes_lightpen, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport1submode"), &p->jports[1].submode, joyportsubmodes_lightpen, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport2submode"), &p->jports[2].submode, joyportsubmodes_lightpen, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport3submode"), &p->jports[3].submode, joyportsubmodes_lightpen, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport0autofire"), &p->jports[0].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport1autofire"), &p->jports[1].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport2autofire"), &p->jports[2].autofire, joyaf, 0))
		return 1;
	if (cfgfile_strval(option, value, _T("joyport3autofire"), &p->jports[3].autofire, joyaf, 0))
		return 1;
#ifdef AMIBERRY
	if (cfgfile_intval(option, value, _T("joyport0mousemap"), &p->jports[0].mousemap, 1)) {
		return 1;
	}
	if (cfgfile_intval(option, value, _T("joyport1mousemap"), &p->jports[1].mousemap, 1)) {
		return 1;
	}
#endif
	if (cfgfile_yesno(option, value, _T("joyport0keyboardoverride"), &vb)) {
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

	if (cfgfile_path(option, value, _T("trainerfile"), p->trainerfile, sizeof p->trainerfile / sizeof(TCHAR)))
		return 1;

	if (cfgfile_path(option, value, _T("statefile_quit"), p->quitstatefile, sizeof p->quitstatefile / sizeof (TCHAR)))
		return 1;

	if (cfgfile_path(option, value, _T("statefile_path"), p->statefile_path, sizeof p->statefile_path / sizeof(TCHAR))) {
		_tcscpy(path_statefile, p->statefile_path);
		target_setdefaultstatefilename(path_statefile);
		return 1;
	}

	if (cfgfile_string (option, value, _T("statefile_name"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		get_savestate_path (savestate_fname, sizeof savestate_fname / sizeof (TCHAR));
		_tcscat (savestate_fname, tmpbuf);
		if (_tcslen (savestate_fname) >= 4 && _tcsicmp (savestate_fname + _tcslen (savestate_fname) - 4, _T(".uss")))
			_tcscat (savestate_fname, _T(".uss"));
		return 1;
	}

	if (cfgfile_path(option, value, _T("statefile"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		_tcscpy (p->statefile, tmpbuf);
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
				TCHAR tmp[MAX_DPATH];
				get_savestate_path (tmp, sizeof tmp / sizeof (TCHAR));
				_tcscat (tmp, savestate_fname);
				if (zfile_exists (tmp)) {
					_tcscpy (savestate_fname, tmp);
					savestate_state = STATE_DORESTORE;
				} else {
					savestate_fname[0] = 0;
				}
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


	if (_tcscmp (option, _T("kbd_lang")) == 0) {
		KbdLang l;
		if ((l = KBD_LANG_DE, _tcsicmp (value, _T("de")) == 0)
			|| (l = KBD_LANG_DK, _tcsicmp (value, _T("dk")) == 0)
			|| (l = KBD_LANG_SE, _tcsicmp (value, _T("se")) == 0)
			|| (l = KBD_LANG_US, _tcsicmp (value, _T("us")) == 0)
			|| (l = KBD_LANG_FR, _tcsicmp (value, _T("fr")) == 0)
			|| (l = KBD_LANG_IT, _tcsicmp (value, _T("it")) == 0)
			|| (l = KBD_LANG_ES, _tcsicmp (value, _T("es")) == 0))
			p->keyboard_lang = l;
		else
			cfgfile_warning(_T("Unknown keyboard language\n"));
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

	if (cfgfile_string (option, value, _T("keyboard_leds"), tmpbuf, sizeof (tmpbuf) / sizeof (TCHAR))) {
		TCHAR *tmpp2 = tmpbuf;
		int i, num;
		p->keyboard_leds[0] = p->keyboard_leds[1] = p->keyboard_leds[2] = 0;
		p->keyboard_leds_in_use = false;
		_tcscat (tmpbuf, _T(","));
		for (i = 0; i < 3; i++) {
			tmpp = _tcschr (tmpp2, ':');
			if (!tmpp)
				break;
			*tmpp++= 0;
			num = -1;
			if (!strcasecmp (tmpp2, _T("numlock")))
				num = 0;
			if (!strcasecmp (tmpp2, _T("capslock")))
				num = 1;
			if (!strcasecmp (tmpp2, _T("scrolllock")))
				num = 2;
			tmpp2 = tmpp;
			tmpp = _tcschr (tmpp2, ',');
			if (!tmpp)
				break;
			*tmpp++= 0;
			if (num >= 0) {
				p->keyboard_leds[num] = match_string (kbleds, tmpp2);
				if (p->keyboard_leds[num])
					p->keyboard_leds_in_use = true;
			}
			tmpp2 = tmpp;
		}
		return 1;
	}

	if (_tcscmp (option, _T("displaydata")) == 0 || _tcscmp (option, _T("displaydata_pal")) == 0 || _tcscmp (option, _T("displaydata_ntsc")) == 0) {
		_tcsncpy (tmpbuf, value, sizeof tmpbuf / sizeof (TCHAR) - 1);
		tmpbuf[sizeof tmpbuf / sizeof (TCHAR) - 1] = '\0';

		int vert = -1, horiz = -1, lace = -1, ntsc = -1, framelength = -1, vsync = -1, hres = 0;
		bool locked = false, rtg = false, exit = false;
		bool cmdmode = false, defaultdata = false;
		float rate = -1;
		int rpct = 0;
		TCHAR cmd[MAX_DPATH], filter[64] = { 0 }, label[16] = { 0 };
		TCHAR *tmpp = tmpbuf;
		TCHAR *end = tmpbuf + _tcslen (tmpbuf);
		cmd[0] = 0;
		for (;;) {
			TCHAR *next = _tcschr (tmpp, ',');
			TCHAR *equals = _tcschr (tmpp, '=');

			if (!next)
				next = end;
			if (equals == nullptr || equals > next)
				equals = nullptr;
			else
				equals++;
			*next = 0;

			if (cmdmode) {
				if (_tcslen(cmd) + _tcslen(tmpp) + 2 < sizeof(cmd) / sizeof(TCHAR)) {
					_tcscat(cmd, tmpp);
					_tcscat(cmd, _T("\n"));
				}
			} else {
				if (!_tcsnicmp(tmpp, _T("cmd="), 4)) {
					cmdmode = true;
					tmpp += 4;
				}
				if (rate < 0)
					rate = static_cast<float>(_tstof(tmpp));
				else if (!_tcsnicmp(tmpp, _T("v="), 2))
					vert = _tstol(equals);
				else if (!_tcsnicmp(tmpp, _T("h="), 2))
					horiz = _tstol(equals);
				else if (!_tcsnicmp(tmpp, _T("t="), 2))
					_tcsncpy(label, equals, sizeof label / sizeof(TCHAR) - 1);
				else if (!_tcsnicmp(tmpp, _T("filter="), 7))
					_tcsncpy(filter, equals, sizeof filter / sizeof(TCHAR) - 1);
				else if (!_tcsnicmp(tmpp, _T("rpct="), 5))
					rpct = _tstol(equals);
				else if (equals) {
					if (_tcslen(cmd) + _tcslen (tmpp) + 2 < sizeof (cmd) / sizeof (TCHAR)) {
						_tcscat(cmd, tmpp);
						_tcscat(cmd, _T("\n"));
					}
				}
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
				if (!_tcsnicmp(tmpp, _T("custom"), 6))
					ntsc = 2;
				if (!_tcsnicmp(tmpp, _T("lof"), 3))
					framelength = 1;
				if (!_tcsnicmp(tmpp, _T("shf"), 3))
					framelength = 0;
				if (!_tcsnicmp(tmpp, _T("rtg"), 3))
					rtg = true;
				if (!_tcsnicmp(tmpp, _T("exit"), 4))
					exit = true;
				if (!_tcsnicmp(tmpp, _T("default"), 7))
					defaultdata = true;
			}
			tmpp = next;
			if (tmpp >= end)
				break;
			tmpp++;
		}
		for (int i = 0; i < MAX_CHIPSET_REFRESH; i++) {
			struct chipset_refresh *cr = &p->cr[i];
			if (_tcscmp (option, _T("displaydata_pal")) == 0) {
				i = CHIPSET_REFRESH_PAL;
				cr = &p->cr[i];
				cr->inuse = false;
				_tcscpy (label, _T("PAL"));
			} else if (_tcscmp (option, _T("displaydata_ntsc")) == 0) {
				i = CHIPSET_REFRESH_NTSC;
				cr = &p->cr[i];
				cr->inuse = false;
				_tcscpy (label, _T("NTSC"));
			}
			if (!cr->inuse) {
				cr->inuse = true;
				cr->horiz = horiz;
				cr->vert = vert;
				cr->lace = lace;
				cr->resolution = hres ? hres : 1 + 2 + 4;
				cr->resolution_pct = rpct;
				cr->ntsc = ntsc;
				cr->vsync = vsync;
				cr->locked = locked;
				cr->rtg = rtg;
				cr->exit = exit;
				cr->framelength = framelength;
				cr->rate = rate;
				cr->defaultdata = defaultdata;
				_tcscpy(cr->commands, cmd);
				_tcscpy(cr->label, label);
				TCHAR *se = cfgfile_unescape(filter, nullptr);
				_tcscpy(cr->filterprofile, se);
				xfree(se);
				break;
			}
		}
		return 1;
	}

#ifdef WITH_SLIRP
	if (cfgfile_string (option, value, _T("slirp_ports"), tmpbuf, sizeof (tmpbuf) / sizeof (TCHAR))) {
		TCHAR *tmpp2 = tmpbuf;
		_tcscat (tmpbuf, _T(","));
		for (;;) {
			tmpp = _tcschr (tmpp2, ',');
			if (!tmpp)
				break;
			*tmpp++= 0;
			for (i = 0; i < MAX_SLIRP_REDIRS; i++) {
				struct slirp_redir *sr = &p->slirp_redirs[i];
				if (sr->proto == 0) {
					sr->dstport = _tstol (tmpp2);
					sr->proto = 1;
					break;
				}
			}
			tmpp2 = tmpp;
		}
		return 1;
	}
	if (cfgfile_string (option, value, _T("slirp_redir"), tmpbuf, sizeof (tmpbuf) / sizeof(TCHAR))) {
		TCHAR *tmpp2 = tmpbuf;
		_tcscat (tmpbuf, _T(":"));
		for (i = 0; i < MAX_SLIRP_REDIRS; i++) {
			struct slirp_redir *sr = &p->slirp_redirs[i];
			if (sr->proto == 0) {
				char *s;
				tmpp = _tcschr (tmpp2, ':');
				if (!tmpp)
					break;
				*tmpp++= 0;
				if (!_tcsicmp (tmpp2, _T("tcp")))
					sr->proto = 1;
				else if (!_tcsicmp (tmpp2, _T("udp")))
					sr->proto = 2;
				else
					break;
				tmpp2 = tmpp;
				tmpp = _tcschr (tmpp2, ':');
				if (!tmpp) {
					sr->proto = 0;
					break;
				}
				*tmpp++= 0;
				sr->dstport = _tstol (tmpp2);
				tmpp2 = tmpp;
				tmpp = _tcschr (tmpp2, ':');
				if (!tmpp) {
					sr->proto = 0;
					break;
				}
				*tmpp++= 0;
				sr->srcport = _tstol (tmpp2);
				tmpp2 = tmpp;
				tmpp = _tcschr (tmpp2, ':');
				if (!tmpp)
					break;
				*tmpp++= 0;
				s = ua (tmpp2);
				sr->addr = inet_addr (s);
				xfree (s);
			}
		}
		return 1;
	}
#endif

#ifdef AMIBERRY
	auto option_string = string(option);

	if (load_custom_options(p, option_string, value))
		return 1;

	if (option_string.rfind("whdload_", 0) == 0)
	{
		/* Read in WHDLoad Options  */
		if (cfgfile_string(option, value, _T("whdload_filename"), whdload_prefs.whdload_filename)
			|| cfgfile_string(option, value, _T("whdload_game_name"), whdload_prefs.game_name)
			|| cfgfile_string(option, value, _T("whdload_uuid"), whdload_prefs.variant_uuid)
			|| cfgfile_string(option, value, _T("whdload_slave_default"), whdload_prefs.slave_default)
			|| cfgfile_yesno(option, value, _T("whdload_slave_libraries"), &whdload_prefs.slave_libraries)
			|| cfgfile_string(option, value, _T("whdload_slave"), whdload_prefs.selected_slave.filename)
			|| cfgfile_string(option, value, _T("whdload_slave_data_path"), whdload_prefs.selected_slave.data_path)
			|| cfgfile_intval(option, value, _T("whdload_custom1"), &whdload_prefs.selected_slave.custom1.value, 1)
			|| cfgfile_intval(option, value, _T("whdload_custom2"), &whdload_prefs.selected_slave.custom2.value, 1)
			|| cfgfile_intval(option, value, _T("whdload_custom3"), &whdload_prefs.selected_slave.custom3.value, 1)
			|| cfgfile_intval(option, value, _T("whdload_custom4"), &whdload_prefs.selected_slave.custom4.value, 1)
			|| cfgfile_intval(option, value, _T("whdload_custom5"), &whdload_prefs.selected_slave.custom5.value, 1)
			|| cfgfile_string(option, value, _T("whdload_custom"), whdload_prefs.custom)
			|| cfgfile_yesno(option, value, _T("whdload_buttonwait"), &whdload_prefs.button_wait)
			|| cfgfile_yesno(option, value, _T("whdload_showsplash"), &whdload_prefs.show_splash)
			|| cfgfile_intval(option, value, _T("whdload_configdelay"), &whdload_prefs.config_delay, 1)
			|| cfgfile_yesno(option, value, _T("whdload_writecache"), &whdload_prefs.write_cache)
			|| cfgfile_yesno(option, value, _T("whdload_quit_on_exit"), &whdload_prefs.quit_on_exit)
			)
		{
			return 1;
		}
	}
#endif

	return 0;
}

static void decode_rom_ident (TCHAR *romfile, int maxlen, const TCHAR *ident, int romflags)
{
	const TCHAR *p;
	int ver, rev, subver, subrev, round, i;
	TCHAR model[64], *modelp;
	struct romlist **rl;
	TCHAR *romtxt;

	if (!ident[0])
		return;
	romtxt = xmalloc (TCHAR, 10000);
	romtxt[0] = 0;
	for (round = 0; round < 2; round++) {
		ver = rev = subver = subrev = -1;
		modelp = nullptr;
		memset (model, 0, sizeof model);
		p = ident;
		while (*p) {
			TCHAR c = *p++;
			int *pp1 = nullptr, *pp2 = nullptr;
			if (_totupper (c) == 'V' && _istdigit (*p)) {
				pp1 = &ver;
				pp2 = &rev;
			} else if (_totupper (c) == 'R' && _istdigit (*p)) {
				pp1 = &subver;
				pp2 = &subrev;
			} else if (!_istdigit (c) && c != ' ') {
				_tcsncpy (model, p - 1, (sizeof model) / sizeof (TCHAR) - 1);
				p += _tcslen (model);
				modelp = model;
			}
			if (pp1) {
				*pp1 = _tstol (p);
				while (*p != 0 && *p != '.' && *p != ' ')
					p++;
				if (*p == '.') {
					p++;
					if (pp2)
						*pp2 = _tstol (p);
				}
			}
			if (*p == 0 || *p == ';') {
				rl = getromlistbyident (ver, rev, subver, subrev, modelp, romflags, round > 0);
				if (rl) {
					for (i = 0; rl[i]; i++) {
						if (rl[i]->path && !_tcscmp(rl[i]->path, romfile)) {
							xfree(rl);
							round = 0;
							goto end;
						}
					}
					if (!rl[i]) {
						for (i = 0; rl[i]; i++) {
							if (round) {
								TCHAR romname[MAX_DPATH];
								getromname(rl[i]->rd, romname);
								_tcscat (romtxt, romname);
								_tcscat (romtxt, _T("\n"));
							} else {
								_tcsncpy (romfile, rl[i]->path, maxlen);
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
	if (round && romtxt[0]) {
		notify_user_parms (NUMSG_ROMNEED, romtxt, romtxt);
	}
	xfree (romtxt);
}

static struct uaedev_config_data *getuci (struct uae_prefs *p)
{
	if (p->mountitems < MOUNT_CONFIG_SIZE)
		return &p->mountconfig[p->mountitems++];
	return nullptr;
}

struct uaedev_config_data *add_filesys_config (struct uae_prefs *p, int index, struct uaedev_config_info *ci)
{
	struct uaedev_config_data *uci;
	int i;

	if (index < 0 && (ci->type == UAEDEV_DIR || ci->type == UAEDEV_HDF) && ci->devname[0] && _tcslen (ci->devname) > 0) {
		for (i = 0; i < p->mountitems; i++) {
			if (p->mountconfig[i].ci.devname[0] && !_tcscmp (p->mountconfig[i].ci.devname, ci->devname))
				return nullptr;
		}
	}
	for (;;) {
		if (ci->type == UAEDEV_CD) {
			if (ci->controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && ci->controller_type <= HD_CONTROLLER_TYPE_IDE_LAST)
				break;
			if (ci->controller_type >= HD_CONTROLLER_TYPE_SCSI_FIRST && ci->controller_type <= HD_CONTROLLER_TYPE_SCSI_LAST)
				break;
		} else if (ci->type == UAEDEV_TAPE) {
			if (ci->controller_type == HD_CONTROLLER_TYPE_UAE)
				break;
			if (ci->controller_type >= HD_CONTROLLER_TYPE_IDE_FIRST && ci->controller_type <= HD_CONTROLLER_TYPE_IDE_LAST)
				break;
			if (ci->controller_type >= HD_CONTROLLER_TYPE_SCSI_FIRST && ci->controller_type <= HD_CONTROLLER_TYPE_SCSI_LAST)
				break;
		} else {
			break;
		}
		return nullptr;
	}

	if (index < 0) {
		if (ci->controller_type != HD_CONTROLLER_TYPE_UAE) {
			int ctrl = ci->controller_type;
			int ctrlunit = ci->controller_type_unit;
			int cunit = ci->controller_unit;
			for (;;) {
				for (i = 0; i < p->mountitems; i++) {
					if (p->mountconfig[i].ci.controller_type == ctrl && p->mountconfig[i].ci.controller_type_unit == ctrlunit && p->mountconfig[i].ci.controller_unit == cunit) {
						cunit++;
						if (ctrl >= HD_CONTROLLER_TYPE_IDE_FIRST && ctrl <= HD_CONTROLLER_TYPE_IDE_LAST && cunit == 4)
							return nullptr;
						if (ctrl >= HD_CONTROLLER_TYPE_SCSI_FIRST && ctrl <= HD_CONTROLLER_TYPE_SCSI_LAST && cunit >= 7)
							return nullptr;
					}
				}
				if (i == p->mountitems) {
					ci->controller_unit = cunit;
					break;
				}
			}
		} else if (ci->controller_type == HD_CONTROLLER_TYPE_UAE) {
			int ctrl = ci->controller_type;
			int ctrlunit = ci->controller_type_unit;
			int cunit = ci->controller_unit;
			for (;;) {
				for (i = 0; i < p->mountitems; i++) {
					if (p->mountconfig[i].ci.controller_type == ctrl && p->mountconfig[i].ci.controller_type_unit == ctrlunit && p->mountconfig[i].ci.controller_unit == cunit) {
						cunit++;
						if (cunit >= MAX_FILESYSTEM_UNITS)
							return nullptr;
					}
				}
				if (i == p->mountitems) {
					ci->controller_unit = cunit;
					break;
				}
			}
		}
		if (ci->type == UAEDEV_CD) {
			for (i = 0; i < p->mountitems; i++) {
				if (p->mountconfig[i].ci.type == ci->type)
					return nullptr;
			}
		}
		uci = getuci (p);
		if (!uci) {
			return nullptr;
		}
		uci->configoffset = -1;
		uci->unitnum = -1;
	} else {
		if (index >= MAX_FILESYSTEM_UNITS) {
			return nullptr;
		}
		uci = &p->mountconfig[index];
	}
	if (!uci)
		return nullptr;

	memcpy (&uci->ci, ci, sizeof (struct uaedev_config_info));
	validatedevicename (uci->ci.devname, nullptr);
	validatevolumename (uci->ci.volname, nullptr);
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
			_sntprintf (base2, sizeof base2, _T("%s%d"), base, num);
			if (!_tcsicmp(base2, p->mountconfig[i].ci.devname)) {
				num++;
				i = -1;
			}
		}
		_tcscpy (uci->ci.devname, base2);
		validatedevicename (uci->ci.devname, nullptr);
	}
	if (ci->type == UAEDEV_DIR) {
		TCHAR *s = filesys_createvolname (uci->ci.volname, uci->ci.rootdir, nullptr, _T("Harddrive"));
		_tcscpy (uci->ci.volname, s);
		xfree (s);
	}
	return uci;
}

static void parse_addmem (struct uae_prefs *p, TCHAR *buf, int num)
{
	int size = 0, addr = 0;

	if (!getintval2 (&buf, &addr, ',', false))
		return;
	if (!getintval2 (&buf, &size, 0, true))
		return;
	if (addr & 0xffff)
		return;
	if ((size & 0xffff) || (size & 0xffff0000) == 0)
		return;
	p->custom_memory_addrs[num] = addr;
	p->custom_memory_sizes[num] = size;
}

static void get_filesys_controller (const TCHAR *hdc, int *type, int *typenum, int *num)
{
	int hdcv = HD_CONTROLLER_TYPE_UAE;
	int hdunit = 0;
	int idx = 0;
	if(_tcslen (hdc) >= 4 && !_tcsncmp (hdc, _T("ide"), 3)) {
		hdcv = HD_CONTROLLER_TYPE_IDE_AUTO;
		hdunit = hdc[3] - '0';
		if (hdunit < 0 || hdunit >= 6)
			hdunit = 0;
	} else if (_tcslen(hdc) >= 5 && !_tcsncmp(hdc, _T("scsi"), 4)) {
		hdcv = HD_CONTROLLER_TYPE_SCSI_AUTO;
		hdunit = hdc[4] - '0';
		if (hdunit < 0 || hdunit >= 8 + 2)
			hdunit = 0;
	} else if (_tcslen(hdc) >= 7 && !_tcsncmp(hdc, _T("custom"), 6)) {
		hdcv = HD_CONTROLLER_TYPE_CUSTOM_FIRST;
		hdunit = hdc[6] - '0';
		if (hdunit < 0 || hdunit >= 8)
			hdunit = 0;
	}
	if (hdcv == HD_CONTROLLER_TYPE_UAE) {
		hdunit = _tstol(hdc + 3);
		if (hdunit >= MAX_FILESYSTEM_UNITS)
			hdunit = 0;
	} else if (hdcv > HD_CONTROLLER_TYPE_UAE) {
		TCHAR control[MAX_DPATH];
		bool found = false;
		_tcscpy(control, hdc);
		const auto extend = _tcschr(control, ',');
		if (extend)
			extend[0] = 0;
		const TCHAR *ext = _tcsrchr(control, '_');
		if (ext) {
			ext++;
			int len = uaetcslen(ext);
			if (len > 2 && ext[len - 2] == '-' && ext[len - 1] >= '2' && ext[len - 1] <= '9') {
				idx = ext[len - 1] - '1';
				len -= 2;
			}
			for (int i = 0; hdcontrollers[i].label; i++) {
				const TCHAR *ext2 = _tcsrchr(hdcontrollers[i].label, '_');
				if (ext2) {
					ext2++;
					if (_tcslen(ext2) == len && !_tcsnicmp(ext, ext2, len) && hdc[0] == hdcontrollers[i].label[0]) {
						if (hdcontrollers[i].romtype) {
							for (int j = 0; expansionroms[j].name; j++) {
								if ((expansionroms[j].romtype & ROMTYPE_MASK) == hdcontrollers[i].romtype) {
									if (hdcv == HD_CONTROLLER_TYPE_IDE_AUTO) {
										hdcv = j + HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST;
									} else if (hdcv == HD_CONTROLLER_TYPE_SCSI_AUTO) {
										hdcv = j + HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST;
									} else {
										hdcv = j + HD_CONTROLLER_TYPE_CUSTOM_FIRST;
									}
									break;
								}
							}
						}
						if (hdcv == HD_CONTROLLER_TYPE_IDE_AUTO) {
							hdcv = i;
						} else if (hdcv == HD_CONTROLLER_TYPE_SCSI_AUTO) {
							hdcv = i + HD_CONTROLLER_EXPANSION_MAX;
						}
						found = true;
						break;
					}
				}
			}
			if (!found) {
				for (int i = 0; expansionroms[i].name; i++) {
					const struct expansionromtype *ert = &expansionroms[i];
					if (_tcslen(ert->name) == len && !_tcsnicmp(ext, ert->name, len)) {
						if (hdcv == HD_CONTROLLER_TYPE_IDE_AUTO) {
							hdcv = HD_CONTROLLER_TYPE_IDE_EXPANSION_FIRST + i;
						} else if (hdcv == HD_CONTROLLER_TYPE_SCSI_AUTO) {
							hdcv = HD_CONTROLLER_TYPE_SCSI_EXPANSION_FIRST + i;
						} else {
							hdcv = HD_CONTROLLER_TYPE_CUSTOM_FIRST + i;
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

static bool parse_geo (const TCHAR *tname, struct uaedev_config_info *uci, const struct hardfiledata *hfd, bool empty, bool addgeom)
{
	int found = 0;
	TCHAR tmp[200], section[200];
	struct ini_data *ini;
	bool ret = false;
	TCHAR tgname[MAX_DPATH];

	cfgfile_resolve_path_out_load(tname, tgname, MAX_DPATH, PATH_HDF);
	ini = ini_load(tgname, true);
	if (!ini)
		return ret;

	_tcscpy(tmp, _T("empty"));
	section[0] = 0;
	if (empty && ini_getstring(ini, tmp, nullptr, nullptr)) {
		_tcscpy(section, tmp);
		found = 1;
	}
	_tcscpy(tmp, _T("default"));
	if (!empty && ini_getstring(ini, tmp, nullptr, nullptr)) {
		_tcscpy(section, tmp);
		found = 1;
	}
	_tcscpy(tmp, _T("geometry"));
	if (ini_getstring(ini, tmp, nullptr, nullptr)) {
		_tcscpy(section, tmp);
		found = 1;
	}
	if (hfd) {
		_sntprintf(tmp, sizeof tmp, _T("%llu"), hfd->virtsize);
		if (ini_getstring(ini, tmp, nullptr, nullptr)) {
			_tcscpy(section, tmp);
			found = 1;
		}
	}

	if (found) {
		ret = true;
		write_log(_T("Geometry file '%s' section '%s' found\n"), tgname, section);
		if (addgeom) {
			_tcscpy(uci->geometry, tname);
		}

		int idx = 0;
		for (;;) {
			TCHAR *key = nullptr, *val = nullptr;
			int v;

			if (!ini_getsectionstring(ini, section, idx, &key, &val))
				break;

			//write_log(_T("%s:%s\n"), key, val);

			if (val[0] == '0' && _totupper (val[1]) == 'X') {
				TCHAR *endptr;
				v = _tcstol (val, &endptr, 16);
			} else {
				v = _tstol (val);
			}
			if (!_tcsicmp (key, _T("psurfaces")))
				uci->pheads = v;
			if (!_tcsicmp (key, _T("psectorspertrack")) || !_tcsicmp (key, _T("pblockspertrack")))
				uci->psecs = v;
			if (!_tcsicmp (key, _T("pcyls")))
				uci->pcyls = v;
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
			if (!_tcsicmp (key, _T("highcyl")) || !_tcsicmp (key, _T("cyl")) || !_tcsicmp (key, _T("cyls")))
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
				get_filesys_controller (val, &uci->controller_type, &uci->controller_type_unit, &uci->controller_unit);
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
			if (!_tcsicmp(key, _T("badblocks"))) {
				TCHAR *p = val;
				while (p && *p && uci->badblock_num < MAX_UAEDEV_BADBLOCKS) {
					struct uaedev_badblock *bb = &uci->badblocks[uci->badblock_num];
					if (!_istdigit(*p))
						break;
					bb->first = _tstol(p);
					bb->last = bb->first;
					TCHAR *p1 = _tcschr(p, ',');
					TCHAR *p2 = nullptr;
					if (p1) {
						p2 = p1 + 1;
						*p1 = 0;
					}
					p1 = _tcschr(p, '-');
					if (p1) {
						bb->last = _tstol(p1 + 1);
					}
					uci->badblock_num++;
					p = p2;
				}
			}
			xfree (val);
			xfree (key);
			idx++;
		}
	}

	uae_u8 *out;
	int outsize;

	if (ini_getdata(ini, _T("MODE SENSE"), _T("03"), &out, &outsize) && outsize >= 16) {
		uci->psecs = (out[10] << 8) | (out[11] << 0);
		uci->blocksize = (out[12] << 8) | (out[13] << 0);
		xfree(out);
		ret = true;
	}

	if (ini_getdata(ini, _T("MODE SENSE"), _T("04"), &out, &outsize) && outsize >= 20) {
		uci->pheads = out[5];
		uci->pcyls = (out[2] << 16) | (out[3] << 8) | (out[4] << 0);
		xfree(out);
		ret = true;
	}

	if (ini_getdata(ini, _T("READ CAPACITY"), _T("DATA"), &out, &outsize) && outsize >= 8) {
		uci->blocksize = (out[4] << 24) | (out[5] << 16) | (out[6] << 8) | (out[7] << 0);
		uci->max_lba = ((out[0] << 24) | (out[1] << 16) | (out[2] << 8) | (out[3] << 0)) + 1;
		xfree(out);
		ret = true;
	}

	void ata_parse_identity(uae_u8 *out, struct uaedev_config_info *uci, bool *lba, bool *lba48, int *max_multiple);
	bool ata_get_identity(struct ini_data *ini, uae_u8 *out, bool overwrite);

	uae_u8 ident[512];
	if (ata_get_identity(ini, ident, true)) {
		bool lba, lba48;
		int max_multiple;
		ata_parse_identity(ident, uci, &lba, &lba48, &max_multiple);
		ret = true;
	}

	ini_free(ini);
	return ret;
}

bool get_hd_geometry (struct uaedev_config_info *uci)
{
	TCHAR tname[MAX_DPATH];

	get_configuration_path (tname, sizeof tname / sizeof (TCHAR));
	_tcscat (tname, _T("default.geo"));
	if (zfile_exists (tname)) {
		struct hardfiledata hfd{};
		memset (&hfd, 0, sizeof hfd);
		hfd.ci.readonly = true;
		hfd.ci.blocksize = 512;
		if (hdf_open (&hfd, uci->rootdir) > 0) {
			parse_geo (tname, uci, &hfd, false, false);
			hdf_close (&hfd);
		} else {
			parse_geo (tname, uci, nullptr, true, false);
		}
	}
	if (uci->geometry[0]) {
		return parse_geo (uci->geometry, uci, nullptr, false, true);
	} else if (uci->rootdir[0]) {
		_tcscpy (tname, uci->rootdir);
		_tcscat (tname, _T(".geo"));
		return parse_geo (tname, uci, nullptr, false, true);
	}
	return false;
}

static int cfgfile_parse_partial_newfilesys (struct uae_prefs *p, int nr, int type, const TCHAR *value, int unit, bool uaehfentry)
{
	TCHAR *path = nullptr;

	// read only harddrive name
	if (!uaehfentry)
		return 0;
	if (type != 1)
		return 0;
	auto* tmpp = getnextentry (&value, ',');
	if (!tmpp)
		return 0;
	xfree (tmpp);
	tmpp = getnextentry (&value, ':');
	if (!tmpp)
		return 0;
	xfree (tmpp);
	auto* name = getnextentry (&value, ':');
	if (name && _tcslen (name) > 0) {
		path = getnextentry (&value, ',');
		if (path && _tcslen (path) > 0) {
			for (auto& i : p->mountconfig) {
				auto* uci = &i.ci;
				if (_tcsicmp(uci->rootdir, name) == 0) {
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
	struct uaedev_config_info uci{};
	TCHAR *tmpp = _tcschr (value, ','), *tmpp2;
	TCHAR *str = nullptr;
	TCHAR devname[MAX_DPATH], volname[MAX_DPATH];

	devname[0] = volname[0] = 0;
	uci_set_defaults (&uci, false);

	config_newfilesystem = 1;
	if (tmpp == nullptr)
		goto invalid_fs;

	*tmpp++ = '\0';
	if (_tcsicmp (value, _T("ro")) == 0)
		uci.readonly = true;
	else if (_tcsicmp (value, _T("rw")) == 0)
		uci.readonly = false;
	else
		goto invalid_fs;

	value = tmpp;
	if (type == 0) {
		uci.type = UAEDEV_DIR;
		tmpp = _tcschr (value, ':');
		if (tmpp == nullptr)
			goto empty_fs;
		*tmpp++ = 0;
		_tcscpy (devname, value);
		tmpp2 = tmpp;
		tmpp = _tcschr (tmpp, ':');
		if (tmpp == nullptr)
			goto empty_fs;
		*tmpp++ = 0;
		_tcscpy (volname, tmpp2);
		tmpp2 = tmpp;
		// quoted special case
		if (tmpp2[0] == '\"') {
			const TCHAR *end;
			TCHAR *n = cfgfile_unescape (tmpp2, &end, 0, true);
			if (!n)
				goto invalid_fs;
			_tcscpy (uci.rootdir, n);
			xfree(n);
			tmpp = const_cast<TCHAR*>(end);
			*tmpp++ = 0;
		} else {
			tmpp = _tcschr (tmpp, ',');
			if (tmpp == nullptr)
				goto empty_fs;
			*tmpp++ = 0;
			_tcscpy (uci.rootdir, tmpp2);
		}
		_tcscpy (uci.volname, volname);
		_tcscpy (uci.devname, devname);
		if (! getintval (&tmpp, &uci.bootpri, 0))
			goto empty_fs;
	} else if (type == 1 || ((type == 2 || type == 3) && uaehfentry)) {
		tmpp = _tcschr (value, ':');
		if (tmpp == nullptr)
			goto invalid_fs;
		*tmpp++ = '\0';
		_tcscpy (devname, value);
		tmpp2 = tmpp;
		// quoted special case
		if (tmpp2[0] == '\"') {
			const TCHAR *end;
			TCHAR *n = cfgfile_unescape (tmpp2, &end, 0, true);
			if (!n)
				goto invalid_fs;
			_tcscpy (uci.rootdir, n);
			xfree(n);
			tmpp = const_cast<TCHAR*>(end);
			*tmpp++ = 0;
		} else {
			tmpp = _tcschr (tmpp, ',');
			if (tmpp == nullptr)
				goto invalid_fs;
			*tmpp++ = 0;
			_tcscpy (uci.rootdir, tmpp2);
		}
		if (uci.rootdir[0] != ':')
			get_hd_geometry (&uci);
		_tcscpy (uci.devname, devname);
		if (! getintval (&tmpp, &uci.sectors, ',')
			|| ! getintval (&tmpp, &uci.surfaces, ',')
			|| ! getintval (&tmpp, &uci.reserved, ',')
			|| ! getintval (&tmpp, &uci.blocksize, ','))
			goto invalid_fs;
		if (getintval2 (&tmpp, &uci.bootpri, ',', false)) {
			const TCHAR *end;
			TCHAR *n;
			tmpp2 = tmpp;
			// quoted special case
			if (tmpp2[0] == '\"') {
				const TCHAR *end;
				TCHAR *n = cfgfile_unescape (tmpp2, &end, 0, false);
				if (!n)
					goto invalid_fs;
				_tcscpy (uci.filesys, n);
				xfree(n);
				tmpp = const_cast<TCHAR*>(end);
				*tmpp++ = 0;
			} else {
				tmpp = _tcschr (tmpp, ',');
				if (tmpp == nullptr)
					goto empty_fs;
				*tmpp++ = 0;
				_tcscpy (uci.filesys, tmpp2);
			}
			get_filesys_controller (tmpp, &uci.controller_type, &uci.controller_type_unit, &uci.controller_unit);
			tmpp2 = _tcschr (tmpp, ',');
			if (tmpp2) {
				tmpp2++;
				if (getintval2 (&tmpp2, &uci.highcyl, ',', false)) {
					getintval (&tmpp2, &uci.pcyls, '/');
					getintval (&tmpp2, &uci.pheads, '/');
					getintval2 (&tmpp2, &uci.psecs, ',', true);
					if (uci.pheads && uci.psecs) {
						uci.physical_geometry = true;
					} else {
						uci.pheads = uci.psecs = uci.pcyls = 0;
						uci.physical_geometry = false;
					}
					if (tmpp2 && tmpp2[0]) {
						if (tmpp2[0] == '\"') {
							n = cfgfile_unescape (tmpp2, &end, 0, false);
							if (!n)
								goto invalid_fs;
							_tcscpy(uci.geometry, n);
							xfree(n);
						}
					}
				}
			}
			uci.controller_media_type = 0;
			uci.unit_feature_level = 1;

			if (cfgfile_option_find(tmpp2, _T("CF")))
				uci.controller_media_type = 1;
			else if (cfgfile_option_find(tmpp2, _T("HD")))
				uci.controller_media_type = 0;

			TCHAR *pflags;
			if ((pflags = cfgfile_option_get(tmpp2, _T("flags")))) {
				getintval(&pflags, &uci.unit_special_flags, 0);
				xfree(pflags);
			}

			if (cfgfile_option_find(tmpp2, _T("lock")))
				uci.lock = true;
			if (cfgfile_option_find(tmpp2, _T("identity")))
				uci.loadidentity = true;

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
		if (type == 2) {
			uci.device_emu_unit = unit;
			uci.blocksize = 2048;
			uci.readonly = true;
			uci.type = UAEDEV_CD;
		} else if (type == 3) {
			uci.device_emu_unit = unit;
			uci.blocksize = 512;
			uci.type = UAEDEV_TAPE;
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
	}
	if (uci.geometry[0]) {
		parse_geo(uci.geometry, &uci, nullptr, false, true);
	}
#ifdef FILESYS
	add_filesys_config (p, nr, &uci);
#endif
	xfree (str);
	return 1;

invalid_fs:
	cfgfile_warning(_T("Invalid filesystem/hardfile/cd specification.\n"));
	return 1;
}

static int cfgfile_parse_filesys (struct uae_prefs *p, const TCHAR *option, TCHAR *value)
{
	int i;

	for (i = 0; i < MAX_FILESYSTEM_UNITS; i++) {
		TCHAR tmp[100];
		_sntprintf (tmp, sizeof tmp, _T("uaehf%d"), i);
		if (!_tcscmp (option, tmp)) {
			for (;;) {
				int type = -1;
				int unit = -1;
				TCHAR *tmpp = _tcschr (value, ',');
				if (tmpp == nullptr)
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
					return 1; /* ignore for now */
				}
				if (type >= 0)
					cfgfile_parse_newfilesys (p, -1, type, tmpp, unit, true);
				return 1;
			}
			return 1;
		} else if (!_tcsncmp (option, tmp, _tcslen (tmp)) && option[_tcslen (tmp)] == '_') {
			struct uaedev_config_info *uci = &currprefs.mountconfig[i].ci;
			if (uci->devname[0]) {
				const TCHAR *s = &option[_tcslen (tmp) + 1];
				if (!_tcscmp (s, _T("bootpri"))) {
					getintval (&value, &uci->bootpri, 0);
				} else if (!_tcscmp (s, _T("read-only"))) {
					cfgfile_yesno (nullptr, value, nullptr, &uci->readonly);
				} else if (!_tcscmp (s, _T("volumename"))) {
					_tcscpy (uci->volname, value);
				} else if (!_tcscmp (s, _T("devicename"))) {
					_tcscpy (uci->devname, value);
				} else if (!_tcscmp (s, _T("root"))) {
					_tcscpy (uci->rootdir, value);
				} else if (!_tcscmp (s, _T("filesys"))) {
					_tcscpy (uci->filesys, value);
				} else if (!_tcscmp (s, _T("geometry"))) {
					_tcscpy (uci->geometry, value);
				} else if (!_tcscmp (s, _T("controller"))) {
					get_filesys_controller (value, &uci->controller_type, &uci->controller_type_unit, &uci->controller_unit);
				}
			}
		}
	}

	if (_tcscmp (option, _T("filesystem")) == 0
		|| _tcscmp (option, _T("hardfile")) == 0)
	{
		struct uaedev_config_info uci{};
		TCHAR *tmpp = _tcschr (value, ',');
		TCHAR *str;
		bool hdf;

		uci_set_defaults (&uci, false);

		if (config_newfilesystem)
			return 1;

		if (tmpp == nullptr)
			goto invalid_fs;

		*tmpp++ = '\0';
		if (_tcscmp(value, _T("1")) == 0 || _tcsicmp (value, _T("ro")) == 0
			|| _tcsicmp (value, _T("readonly")) == 0
			|| _tcsicmp (value, _T("read-only")) == 0)
			uci.readonly = true;
		else if (_tcscmp (value, _T("0")) == 0 || _tcsicmp (value, _T("rw")) == 0
			|| _tcsicmp(value, _T("readwrite")) == 0
			|| _tcsicmp(value, _T("read-write")) == 0)
			uci.readonly = false;
		else
			goto invalid_fs;

		value = tmpp;
		if (_tcscmp (option, _T("filesystem")) == 0) {
			hdf = false;
			tmpp = _tcschr (value, ':');
			if (tmpp == nullptr)
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
		str = cfgfile_subst_path_load (UNEXPANDED, &p->path_hardfile, uci.rootdir, true);
		if (uci.geometry[0])
			parse_geo(uci.geometry, &uci, nullptr, false, false);
#ifdef FILESYS
		uci.type = hdf ? UAEDEV_HDF : UAEDEV_DIR;
		add_filesys_config (p, -1, &uci);
#endif
		xfree (str);
		return 1;
invalid_fs:
		cfgfile_warning(_T("Invalid filesystem/hardfile specification.\n"));
		return 1;

	}

	if (_tcscmp (option, _T("filesystem2")) == 0)
		return cfgfile_parse_newfilesys (p, -1, 0, value, -1, false);
	if (_tcscmp (option, _T("hardfile2")) == 0)
		return cfgfile_parse_newfilesys (p, -1, 1, value, -1, false);
	if (_tcscmp (option, _T("filesystem_extra")) == 0) {
		int idx = 0;
		TCHAR *s = value;
		_tcscat(s, _T(","));
		struct uaedev_config_info *ci = nullptr;
		for (;;) {
			TCHAR *tmpp = _tcschr (s, ',');
			if (tmpp == nullptr)
				return 1;
			*tmpp++ = 0;
			if (idx == 0) {
				for (i = 0; i < p->mountitems; i++) {
					if (p->mountconfig[i].ci.devname[0] && !_tcscmp (p->mountconfig[i].ci.devname, s)) {
						ci = &p->mountconfig[i].ci;
						break;
					}
				}
				if (!ci || ci->type != UAEDEV_DIR)
					return 1;
			} else {
				bool b = true;
				TCHAR *tmpp2 = _tcschr(s, '=');
				if (tmpp2) {
					*tmpp2++ = 0;
					if (!_tcsicmp(tmpp2, _T("false")))
						b = false;
				}
				if (!_tcsicmp(s, _T("inject_icons"))) {
					ci->inject_icons = b;
				}
			}
			idx++;
			s = tmpp;
		}
	}

	return 0;
}

static bool cfgfile_read_board_rom(struct uae_prefs *p, const TCHAR *option, const TCHAR *value, struct multipath *mp)
{
	TCHAR buf[256], buf2[MAX_DPATH], buf3[MAX_DPATH];
	bool dummy;
	int val;
	const struct expansionromtype *ert;

	for (int i = 0; expansionroms[i].name; i++) {
		struct boardromconfig *brc;
		int idx;
		ert = &expansionroms[i];

		for (int j = 0; j < MAX_DUPLICATE_EXPANSION_BOARDS; j++) {
			TCHAR name[256];

			if (j == 0)
				_tcscpy(name, ert->name);
			else
				_sntprintf(name, sizeof name, _T("%s-%d"), ert->name, j + 1);

			_sntprintf(buf, sizeof buf, _T("scsi_%s"), name);
			if (cfgfile_yesno(option, value, buf, &dummy)) {
				return true;
			}

			_sntprintf(buf, sizeof buf, _T("%s_rom_file"), name);
			if (cfgfile_path(option, value, buf, buf2, MAX_DPATH / sizeof (TCHAR), mp)) {
				if (buf2[0]) {
					if (ert->deviceflags & EXPANSIONTYPE_NET) {
						// make sure network settings are available before parsing net "rom" entries
						ethernet_updateselection();
					}
					brc = get_device_rom_new(p, ert->romtype, j, &idx);
					_tcscpy(brc->roms[idx].romfile, buf2);
				}
				return true;
			}

			_sntprintf(buf, sizeof buf, _T("%s_rom_file_id"), name);
			buf2[0] = 0;
			if (cfgfile_rom (option, value, buf, buf2, MAX_DPATH / sizeof (TCHAR))) {
				if (buf2[0]) {
					brc = get_device_rom_new(p, ert->romtype, j, &idx);
					_tcscpy(brc->roms[idx].romfile, buf2);
				}
				return true;
			}

			_sntprintf(buf, sizeof buf, _T("%s_rom"), name);
			if (cfgfile_string (option, value, buf, buf2, sizeof buf2 / sizeof (TCHAR))) {
				if (buf2[0]) {
					decode_rom_ident (buf3, sizeof(buf3) / sizeof (TCHAR), buf2, ert->romtype);
					if (buf3[0]) {
						brc = get_device_rom_new(p, ert->romtype, j, &idx);
						_tcscpy(brc->roms[idx].romident, buf3);
					}
				}
				return true;
			}

			_sntprintf(buf, sizeof buf, _T("%s_rom_options"), name);
			if (cfgfile_string (option, value, buf, buf2, sizeof buf2 / sizeof (TCHAR))) {
				brc = get_device_rom(p, ert->romtype, j, &idx);
				if (brc) {
					TCHAR *p;
					if (cfgfile_option_bool(buf2, _T("autoboot_disabled")) == 1) {
						brc->roms[idx].autoboot_disabled = true;
					}
					if (cfgfile_option_bool(buf2, _T("dma24bit")) == 1) {
						brc->roms[idx].dma24bit = true;
					}
					if (cfgfile_option_bool(buf2, _T("inserted")) == 1) {
						brc->roms[idx].inserted = true;
					}
					p = cfgfile_option_get(buf2, _T("order"));
					if (p) {
						brc->device_order = _tstol(p);
					}
					xfree(p);
					if (ert->settings) {
						brc->roms[idx].device_settings = cfgfile_read_rom_settings(ert->settings, buf2, brc->roms[idx].configtext);
					}
					if (ert->id_jumper) {
						p = cfgfile_option_get(buf2, _T("id"));
						if (p) {
							brc->roms[idx].device_id = _tstol(p);
							xfree(p);
						}
					}
					if (ert->subtypes) {
						const struct expansionsubromtype *srt = ert->subtypes;
						TCHAR tmp[MAX_DPATH];
						p = tmp;
						*p = 0;
						while (srt->name) {
							_tcscpy(p, srt->configname);
							p += _tcslen(p) + 1;
							p[0] = 0;
							srt++;
						}
						int v = cfgfile_option_select(buf2, _T("subtype"), tmp);
						if (v >= 0)
							brc->roms[idx].subtype = v;
					}
					p = cfgfile_option_get(buf2, _T("mid"));
					if (p) {
						brc->roms[idx].manufacturer = static_cast<uae_u16>(_tstol(p));
						xfree(p);
					}
					p = cfgfile_option_get(buf2, _T("pid"));
					if (p) {
						brc->roms[idx].product = static_cast<uae_u8>(_tstol(p));
						xfree(p);
					}
					p = cfgfile_option_get(buf2, _T("data"));
					if (p && _tcslen(p) >= 3 * 16 - 1) {
						for (int i = 0; i < sizeof brc->roms[idx].autoconfig; i++) {
							TCHAR *s2 = &p[i * 3];
							if (i + 1 < sizeof brc->roms[idx].autoconfig && s2[2] != '.')
								break;
							TCHAR *endptr;
							p[2] = 0;
							brc->roms[idx].autoconfig[i] = static_cast<uae_u8>(_tcstol(s2, &endptr, 16));
						}
					}
					xfree(p);
				}
				return true;
			}
		}

		_sntprintf(buf, sizeof buf, _T("%s_mem_size"), ert->name);
		if (cfgfile_intval (option, value, buf, &val, 0x40000)) {
			if (val) {
				brc = get_device_rom_new(p, ert->romtype, 0, &idx);
				brc->roms[idx].board_ram_size = val;
			}
			return true;
		}
	}
	return false;
}

static void addbcromtype(struct uae_prefs *p, int romtype, bool add, const TCHAR *romfile, int devnum)
{
	if (!add) {
		clear_device_rom(p, romtype, devnum, true);
	} else {
		struct boardromconfig *brc = get_device_rom_new(p, romtype, devnum, nullptr);
		if (brc && !brc->roms[0].romfile[0]) {
			_tcscpy(brc->roms[0].romfile, romfile ? romfile : _T(":ENABLED"));
		}
	}
}

static void addbcromtypenet(struct uae_prefs *p, int romtype, const TCHAR *netname, int devnum)
{
	int is = is_device_rom(p, romtype, devnum);
	if (netname == nullptr || netname[0] == 0) {
		if (is < 0)
			clear_device_rom(p, romtype, devnum, true);
	} else {
		if (is < 0) {
			struct boardromconfig *brc = get_device_rom_new(p, romtype, devnum, nullptr);
			if (brc) {
				if (!brc->roms[0].romfile[0]) {
					_tcscpy(brc->roms[0].romfile, _T(":ENABLED"));
				}
				ethernet_updateselection();
				if (!brc->roms[0].device_settings)
					brc->roms[0].device_settings = ethernet_getselection(netname);
			}
		}
	}
}

static int cfgfile_parse_hardware (struct uae_prefs *p, const TCHAR *option, TCHAR *value)
{
	int tmpval, dummyint, i;
	uae_u32 utmpval;
	bool dummybool;
	TCHAR tmpbuf[CONFIG_BLEN];

	const TCHAR *tmpp = _tcschr(option, '.');
	if (tmpp) {
		_tcscpy(tmpbuf, option);
		tmpbuf[tmpp - option] = 0;
		if (_tcscmp(tmpbuf, TARGET_NAME) == 0) {
			option = tmpp + 1;
			return target_parse_option(p, option, value, CONFIG_TYPE_HARDWARE);
		}
	}

	if (cfgfile_yesno(option, value, _T("cpu_compatible"), &p->cpu_compatible)) {
		return 1;
	}

	if (cfgfile_yesno (option, value, _T("cpu_cycle_exact"), &p->cpu_cycle_exact)) {
		/* we don't want cycle-exact in 68020/40+JIT modes */
		if (p->cpu_model >= 68020 && p->cachesize > 0)
			p->cpu_cycle_exact = p->cpu_memory_cycle_exact = p->blitter_cycle_exact = false;
		p->cpu_memory_cycle_exact = p->cpu_cycle_exact;
		return 1;
	}
	if (cfgfile_yesno (option, value, _T("blitter_cycle_exact"), &p->blitter_cycle_exact)) {
		if (p->cpu_model >= 68020 && p->cachesize > 0)
			p->cpu_cycle_exact = p->cpu_memory_cycle_exact = p->blitter_cycle_exact = false;
		return 1;
	}
	if (cfgfile_yesno (option, value, _T("cpu_memory_cycle_exact"), &p->cpu_memory_cycle_exact)) {
		if (!p->cpu_memory_cycle_exact)
			p->blitter_cycle_exact = p->cpu_cycle_exact = false;
		return 1;
	}
	if (cfgfile_strval (option, value, _T("cycle_exact"), &tmpval, cycleexact, 0)) {
		if (tmpval > 0) {
			p->blitter_cycle_exact = true;
			p->cpu_cycle_exact = tmpval > 1;
			p->cpu_memory_cycle_exact = true;
		} else {
			p->blitter_cycle_exact = false;
			p->cpu_cycle_exact = false;
			p->cpu_memory_cycle_exact = false;
		}
		if (p->cpu_model >= 68020 && p->cachesize > 0)
			p->cpu_cycle_exact = p->cpu_memory_cycle_exact = p->blitter_cycle_exact = false;
		// if old version and CE and fastest possible: set to approximate
		if (p->cpu_cycle_exact && p->config_version < ((2 << 16) | (8 << 8) | (2 << 0)) && p->m68k_speed < 0)
			p->m68k_speed = 0;
		return 1;
	}

	if (cfgfile_string (option, value, _T("cpu_multiplier"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		p->cpu_clock_multiplier = static_cast<int>((_tstof(tmpbuf) * 256.0));
		return 1;
	}


	if (cfgfile_string(option, value, _T("a2065"), p->a2065name, sizeof p->a2065name / sizeof(TCHAR))) {
		if (p->a2065name[0])
			addbcromtype(p, ROMTYPE_A2065, true, nullptr, 0);
		return 1;
	}

	if (cfgfile_string(option, value, _T("ne2000_pci"), p->ne2000pciname, sizeof p->ne2000pciname / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("ne2000_pcmcia"), p->ne2000pcmcianame, sizeof p->ne2000pcmcianame / sizeof(TCHAR)))
		return 1;
	if (cfgfile_string(option, value, _T("jit_blacklist"), p->jitblacklist, sizeof p->jitblacklist / sizeof(TCHAR)))
		return 1;

	if (cfgfile_yesno(option, value, _T("denise_noehb"), &dummybool)) {
		if (dummybool) {
			p->cs_denisemodel = DENISEMODEL_A1000NOEHB;
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("ics_agnus"), &dummybool)) {
		if (dummybool) {
			p->cs_agnusmodel = AGNUSMODEL_A1000;
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("agnus_bltbusybug"), &dummybool)) {
		if (dummybool) {
			p->cs_agnusmodel = AGNUSMODEL_A1000;
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("keyboard_connected"), &dummybool)) {
		p->keyboard_mode = dummybool ? 0 : -1;
		return 1;
	}
	if (cfgfile_strval(option, value, _T("keyboard_type"), &p->keyboard_mode, kbtype, 0)) {
		p->keyboard_mode--;
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("immediate_blits"), &p->immediate_blits)
#ifdef AMIBERRY
		|| cfgfile_yesno(option, value, _T("multithreaded_drawing"), &p->multithreaded_drawing)
#endif
		|| cfgfile_yesno(option, value, _T("fpu_no_unimplemented"), &p->fpu_no_unimplemented)
		|| cfgfile_yesno(option, value, _T("cpu_no_unimplemented"), &p->int_no_unimplemented)
		|| cfgfile_yesno(option, value, _T("cd32cd"), &p->cs_cd32cd)
		|| cfgfile_yesno(option, value, _T("cd32c2p"), &p->cs_cd32c2p)
		|| cfgfile_yesno(option, value, _T("cd32nvram"), &p->cs_cd32nvram)
		|| cfgfile_yesno(option, value, _T("cdtvcd"), &p->cs_cdtvcd)
		|| cfgfile_yesno(option, value, _T("cdtv-cr"), &p->cs_cdtvcr)
		|| cfgfile_yesno(option, value, _T("cdtvram"), &p->cs_cdtvram)
		|| cfgfile_yesno(option, value, _T("a1000ram"), &p->cs_a1000ram)
		|| cfgfile_yesno(option, value, _T("cia_overlay"), &p->cs_ciaoverlay)
		|| cfgfile_yesno(option, value, _T("ksmirror_e0"), &p->cs_ksmirror_e0)
		|| cfgfile_yesno(option, value, _T("ksmirror_a8"), &p->cs_ksmirror_a8)
		|| cfgfile_yesno(option, value, _T("resetwarning"), &p->cs_resetwarning)
		|| cfgfile_yesno(option, value, _T("cia_todbug"), &p->cs_ciatodbug)
		|| cfgfile_yesno(option, value, _T("z3_autoconfig"), &p->cs_z3autoconfig)
		|| cfgfile_yesno(option, value, _T("color_burst"), &p->cs_color_burst)
		|| cfgfile_yesno(option, value, _T("toshiba_gary"), &p->cs_toshibagary)
		|| cfgfile_yesno(option, value, _T("rom_is_slow"), &p->cs_romisslow)
		|| cfgfile_yesno(option, value, _T("1mchipjumper"), &p->cs_1mchipjumper)
		|| cfgfile_yesno(option, value, _T("bkpt_halt"), &p->cs_bkpthang)
		|| cfgfile_yesno(option, value, _T("gfxcard_hardware_vblank"), &p->rtg_hardwareinterrupt)
		|| cfgfile_yesno(option, value, _T("gfxcard_hardware_sprite"), &p->rtg_hardwaresprite)
		|| cfgfile_yesno(option, value, _T("gfxcard_overlay"), &p->rtg_overlay)
		|| cfgfile_yesno(option, value, _T("gfxcard_screensplit"), &p->rtg_vgascreensplit)
		|| cfgfile_yesno(option, value, _T("gfxcard_paletteswitch"), &p->rtg_paletteswitch)
		|| cfgfile_yesno(option, value, _T("gfxcard_dacswitch"), &p->rtg_dacswitch)
		|| cfgfile_yesno(option, value, _T("gfxcard_multithread"), &p->rtg_multithread)
		|| cfgfile_yesno(option, value, _T("synchronize_clock"), &p->tod_hack)
		|| cfgfile_coords(option, value, _T("lightpen_offset"), &p->lightpen_offset[0], &p->lightpen_offset[1])
		|| cfgfile_yesno(option, value, _T("lightpen_crosshair"), &p->lightpen_crosshair)

		|| cfgfile_yesno(option, value, _T("kickshifter"), &p->kickshifter)
		|| cfgfile_yesno(option, value, _T("scsidevice_disable"), &p->scsidevicedisable)
		|| cfgfile_yesno(option, value, _T("ks_write_enabled"), &p->rom_readwrite)
		|| cfgfile_yesno(option, value, _T("ntsc"), &p->ntscmode)
		|| cfgfile_yesno(option, value, _T("sana2"), &p->sana2)
		|| cfgfile_yesno(option, value, _T("genlock"), &p->genlock)
		|| cfgfile_yesno(option, value, _T("genlock_alpha"), &p->genlock_alpha)
		|| cfgfile_yesno(option, value, _T("genlock_aspect"), &p->genlock_aspect)
		|| cfgfile_yesno(option, value, _T("cpu_data_cache"), &p->cpu_data_cache)
		|| cfgfile_yesno(option, value, _T("cpu_threaded"), &p->cpu_thread)
		|| cfgfile_yesno(option, value, _T("cpu_24bit_addressing"), &p->address_space_24)
		|| cfgfile_yesno(option, value, _T("cpu_reset_pause"), &p->reset_delay)
		|| cfgfile_yesno(option, value, _T("cpu_halt_auto_reset"), &p->crash_auto_reset)
		|| cfgfile_yesno(option, value, _T("parallel_on_demand"), &p->parallel_demand)
		|| cfgfile_yesno(option, value, _T("parallel_postscript_emulation"), &p->parallel_postscript_emulation)
		|| cfgfile_yesno(option, value, _T("parallel_postscript_detection"), &p->parallel_postscript_detection)
		|| cfgfile_yesno(option, value, _T("serial_on_demand"), &p->serial_demand)
		|| cfgfile_yesno(option, value, _T("serial_hardware_ctsrts"), &p->serial_hwctsrts)
		|| cfgfile_yesno(option, value, _T("serial_status"), &p->serial_rtsctsdtrdtecd)
		|| cfgfile_yesno(option, value, _T("serial_ri"), &p->serial_ri)
		|| cfgfile_yesno(option, value, _T("serial_direct"), &p->serial_direct)
		|| cfgfile_yesno(option, value, _T("fpu_strict"), &p->fpu_strict)
		|| cfgfile_yesno(option, value, _T("comp_nf"), &p->compnf)
		|| cfgfile_yesno(option, value, _T("comp_constjump"), &p->comp_constjump)
		|| cfgfile_yesno(option, value, _T("comp_catchfault"), &p->comp_catchfault)
#ifdef USE_JIT_FPU
		|| cfgfile_yesno (option, value, _T("compfpu"), &p->compfpu)
#endif
		|| cfgfile_yesno(option, value, _T("jit_inhibit"), &p->cachesize_inhibit)
		|| cfgfile_yesno(option, value, _T("rtg_nocustom"), &p->picasso96_nocustom)
		|| cfgfile_yesno(option, value, _T("floppy_write_protect"), &p->floppy_read_only)
		|| cfgfile_yesno(option, value, _T("harddrive_write_protect"), &p->harddrive_read_only)
		|| cfgfile_yesno(option, value, _T("uae_hide_autoconfig"), &p->uae_hide_autoconfig)
		|| cfgfile_yesno(option, value, _T("board_custom_order"), &p->autoconfig_custom_sort)
		|| cfgfile_yesno(option, value, _T("keyboard_nkro"), &p->keyboard_nkro)
		|| cfgfile_yesno(option, value, _T("uaeserial"), &p->uaeserial))
		return 1;

#ifdef SERIAL_PORT
	if (cfgfile_string(option, value, _T("serial_port"), p->sername, sizeof p->sername / sizeof(TCHAR)))
	{
		if (p->sername[0])
			p->use_serial=true;
		else
			p->use_serial=false;
		return 1;
	}
#endif

	if (cfgfile_intval(option, value, _T("cachesize"), &p->cachesize, 1)
		|| cfgfile_intval(option, value, _T("cd32nvram_size"), &p->cs_cd32nvram_size, 1024)
		|| cfgfile_intval(option, value, _T("chipset_hacks"), &p->cs_hacks, 1)
		|| cfgfile_intval(option, value, _T("serial_stopbits"), &p->serial_stopbits, 1)
		|| cfgfile_intval(option, value, _T("cpu060_revision"), &p->cpu060_revision, 1)
		|| cfgfile_intval(option, value, _T("fpu_revision"), &p->fpu_revision, 1)
		|| cfgfile_intval(option, value, _T("fatgary"), &p->cs_fatgaryrev, 1)
		|| cfgfile_intval(option, value, _T("ramsey"), &p->cs_ramseyrev, 1)
		|| cfgfile_floatval(option, value, _T("chipset_refreshrate"), &p->chipset_refreshrate)
		|| cfgfile_intval(option, value, _T("cpuboardmem1_size"), &p->cpuboardmem1.size, 0x100000)
		|| cfgfile_intval(option, value, _T("cpuboardmem2_size"), &p->cpuboardmem2.size, 0x100000)
		|| cfgfile_intval(option, value, _T("debugmem_size"), &p->debugmem_size, 0x100000)
		|| cfgfile_intval(option, value, _T("mem25bit_size"), &p->mem25bit.size, 0x100000)
		|| cfgfile_intval(option, value, _T("a3000mem_size"), &p->mbresmem_low.size, 0x100000)
		|| cfgfile_intval(option, value, _T("mbresmem_size"), &p->mbresmem_high.size, 0x100000)
		|| cfgfile_intval(option, value, _T("megachipmem_size"), &p->z3chipmem.size, 0x100000)
		|| cfgfile_intval(option, value, _T("z3mem_start"), &p->z3autoconfig_start, 1)
		|| cfgfile_intval(option, value, _T("debugmem_start"), &p->debugmem_start, 1)
		|| cfgfile_intval(option, value, _T("bogomem_size"), &p->bogomem.size, 0x40000)
		|| cfgfile_intval(option, value, _T("rtg_modes"), &p->picasso96_modeflags, 1)
		|| cfgfile_intval(option, value, _T("floppy_speed"), &p->floppy_speed, 1)
		|| cfgfile_intval(option, value, _T("cd_speed"), &p->cd_speed, 1)
		|| cfgfile_intval(option, value, _T("floppy_write_length"), &p->floppy_write_length, 1)
		|| cfgfile_intval(option, value, _T("floppy_random_bits_min"), &p->floppy_random_bits_min, 1)
		|| cfgfile_intval(option, value, _T("floppy_random_bits_max"), &p->floppy_random_bits_max, 1)
		|| cfgfile_intval(option, value, _T("nr_floppies"), &p->nr_floppies, 1)
		|| cfgfile_intval(option, value, _T("floppy0type"), &p->floppyslots[0].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy1type"), &p->floppyslots[1].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy2type"), &p->floppyslots[2].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy3type"), &p->floppyslots[3].dfxtype, 1)
		|| cfgfile_intval(option, value, _T("floppy0subtype"), &p->floppyslots[0].dfxsubtype, 1)
		|| cfgfile_intval(option, value, _T("floppy1subtype"), &p->floppyslots[1].dfxsubtype, 1)
		|| cfgfile_intval(option, value, _T("floppy2subtype"), &p->floppyslots[2].dfxsubtype, 1)
		|| cfgfile_intval(option, value, _T("floppy3subtype"), &p->floppyslots[3].dfxsubtype, 1)
		|| cfgfile_intval(option, value, _T("maprom"), &p->maprom, 1)
		|| cfgfile_intval(option, value, _T("parallel_autoflush"), &p->parallel_autoflush_time, 1)
		|| cfgfile_intval(option, value, _T("uae_hide"), &p->uae_hide, 1)
		|| cfgfile_intval(option, value, _T("cpu_frequency"), &p->cpu_frequency, 1)
		|| cfgfile_intval(option, value, _T("kickstart_ext_rom_file2addr"), &p->romextfile2addr, 1)
		|| cfgfile_intval(option, value, _T("monitoremu_monitor"), &p->monitoremu_mon, 1)
		|| cfgfile_intval(option, value, _T("genlock_scale"), &p->genlock_scale, 1)
		|| cfgfile_intval(option, value, _T("genlock_mix"), &p->genlock_mix, 1)
		|| cfgfile_intval(option, value, _T("genlock_offset_x"), &p->genlock_offset_x, 1)
		|| cfgfile_intval(option, value, _T("genlock_offset_y"), &p->genlock_offset_y, 1)
		|| cfgfile_intval(option, value, _T("keyboard_handshake"), &p->cs_kbhandshake, 1)
		|| cfgfile_intval(option, value, _T("eclockphase"), &p->cs_eclockphase, 1)
		|| cfgfile_intval(option, value, _T("chipset_rtc_adjust"), &p->cs_rtc_adjust, 1)
		|| cfgfile_intval(option, value, _T("rndseed"), &p->seed, 1))
		return 1;

	if (cfgfile_strval(option, value, _T("comp_trustbyte"), &p->comptrustbyte, compmode, 0)
		|| cfgfile_strval(option, value, _T("rtc"), &p->cs_rtc, rtctype, 0)
		|| cfgfile_strval(option, value, _T("ciaatod"), &p->cs_ciaatod, ciaatodmode, 0)
		|| cfgfile_strval(option, value, _T("scsi"), &p->scsi, scsimode, 0)
		|| cfgfile_strval(option, value, _T("comp_trustword"), &p->comptrustword, compmode, 0)
		|| cfgfile_strval(option, value, _T("comp_trustlong"), &p->comptrustlong, compmode, 0)
		|| cfgfile_strval(option, value, _T("comp_trustnaddr"), &p->comptrustnaddr, compmode, 0)
		|| cfgfile_strval(option, value, _T("collision_level"), &p->collision_level, collmode, 0)
		|| cfgfile_strval(option, value, _T("parallel_matrix_emulation"), &p->parallel_matrix_emulation, epsonprinter, 0)
#ifdef WITH_SPECIALMONITORS
		|| cfgfile_strval(option, value, _T("monitoremu"), &p->monitoremu, specialmonitorconfignames, 0)
#endif
		|| cfgfile_strval(option, value, _T("genlockmode"), &p->genlock_image, genlockmodes, 0)
		|| cfgfile_strval(option, value, _T("waiting_blits"), &p->waiting_blits, waitblits, 0)
		|| cfgfile_strval(option, value, _T("floppy_auto_extended_adf"), &p->floppy_auto_ext2, autoext2, 0)
		|| cfgfile_strval(option, value,  _T("z3mapping"), &p->z3_mapping_mode, z3mapping, 0)
		|| cfgfile_strval(option, value,  _T("scsidev_mode"), &p->uaescsidevmode, uaescsidevmodes, 0)
		|| cfgfile_strval(option, value, _T("boot_rom_uae"), &p->boot_rom, uaebootrom, 0)
		|| cfgfile_strval(option, value,  _T("serial_translate"), &p->serial_crlf, serialcrlf, 0)
		|| cfgfile_strval(option, value, _T("hvcsync"), &p->cs_hvcsync, hvcsync, 0)
		|| cfgfile_strval(option, value, _T("unmapped_address_space"), &p->cs_unmapped_space, unmapped, 0)
		|| cfgfile_yesno(option, value, _T("memory_pattern"), &p->cs_memorypatternfill)
		|| cfgfile_yesno(option, value, _T("ipl_delay"), &p->cs_ipldelay)
		|| cfgfile_yesno(option, value, _T("floppydata_pullup"), &p->cs_floppydatapullup)
		|| cfgfile_strval(option, value, _T("ciaa_type"), &p->cs_ciatype[0], ciatype, 0)
		|| cfgfile_strval(option, value, _T("ciab_type"), &p->cs_ciatype[1], ciatype, 0)
		|| cfgfile_strboolval(option, value, _T("comp_flushmode"), &p->comp_hardflush, flushmode, 0)
		|| cfgfile_strval(option, value, _T("agnusmodel"), &p->cs_agnusmodel, agnusmodel, 0)
		|| cfgfile_strval(option, value, _T("agnussize"), &p->cs_agnussize, agnussize, 0)
		|| cfgfile_strval(option, value, _T("denisemodel"), &p->cs_denisemodel, denisemodel, 0)
		|| cfgfile_strval(option, value, _T("eclocksync"), &p->cs_eclocksync, eclocksync, 0))
		return 1;

	if (cfgfile_strval(option, value, _T("uaeboard"), &p->uaeboard, uaeboard_off, 1)) {
		p->uaeboard_nodiag = true;
		return 1;
	}
	if (cfgfile_strval(option, value, _T("uaeboard"), &p->uaeboard, uaeboard, 0)) {
		p->uaeboard_nodiag = false;
		return 1;
	}

	if (cfgfile_path (option, value, _T("kickstart_rom_file"), p->romfile, sizeof p->romfile / sizeof (TCHAR), &p->path_rom)
		|| cfgfile_path (option, value, _T("kickstart_ext_rom_file"), p->romextfile, sizeof p->romextfile / sizeof (TCHAR), &p->path_rom)
		|| cfgfile_path (option, value, _T("kickstart_ext_rom_file2"), p->romextfile2, sizeof p->romextfile2 / sizeof (TCHAR), &p->path_rom)
		|| cfgfile_rom(option, value, _T("kickstart_rom_file_id"), p->romfile, sizeof p->romfile / sizeof(TCHAR))
		|| cfgfile_rom (option, value, _T("kickstart_ext_rom_file_id"), p->romextfile, sizeof p->romextfile / sizeof (TCHAR))
		|| cfgfile_string(option, value, _T("flash_file"), p->flashfile, sizeof p->flashfile / sizeof (TCHAR))
		|| cfgfile_path (option, value, _T("cart_file"), p->cartfile, sizeof p->cartfile / sizeof (TCHAR), &p->path_rom)
		|| cfgfile_string(option, value, _T("rtc_file"), p->rtcfile, sizeof p->rtcfile / sizeof (TCHAR))
		|| cfgfile_path(option, value, _T("picassoiv_rom_file"), p->picassoivromfile, sizeof p->picassoivromfile / sizeof(TCHAR), &p->path_rom)
		|| cfgfile_string(option, value, _T("genlock_image"), p->genlock_image_file, sizeof p->genlock_image_file / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("genlock_video"), p->genlock_video_file, sizeof p->genlock_video_file / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T("genlock_font"), p->genlock_font, sizeof p->genlock_font / sizeof(TCHAR))
		|| cfgfile_string(option, value, _T ("pci_devices"), p->pci_devices, sizeof p->pci_devices / sizeof(TCHAR))
		|| cfgfile_string (option, value, _T("ghostscript_parameters"), p->ghostscript_parameters, sizeof p->ghostscript_parameters / sizeof (TCHAR)))
		return 1;

	if (cfgfile_yesno(option, value, _T("fpu_softfloat"), &dummybool)) {
		if (dummybool)
			p->fpu_mode = 1;
		return 1;
	}
#ifdef MSVC_LONG_DOUBLE
	if (cfgfile_yesno(option, value, _T("fpu_msvc_long_double"), &dummybool)) {
		if (dummybool)
			p->fpu_mode = -1;
		else if (p->fpu_mode < 0)
			p->fpu_mode = 0;
		return 1;
	}
#endif

	if (cfgfile_string(option, value, _T("uaeboard_options"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		TCHAR *s = cfgfile_option_get(value, _T("order"));
		if (s)
			p->uaeboard_order = _tstol(s);
		xfree(s);
		return 1;
	}

	if (cfgfile_readromboard(option, value, &p->romboards[0])) {
		return 1;
	}

	if (cfgfile_readramboard(option, value, _T("chipmem"), &p->chipmem)) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("bogomem"), &p->bogomem)) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("fastmem"), &p->fastmem[0])) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("mem25bit"), &p->mem25bit)) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("a3000mem"), &p->mbresmem_low)) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("mbresmem"), &p->mbresmem_high)) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("z3mem"), &p->z3fastmem[0])) {
		return 1;
	}
	if (cfgfile_readramboard(option, value, _T("megachipmem"), &p->z3chipmem)) {
		return 1;
	}

	if (cfgfile_intval(option, value, _T("cdtvramcard"), &utmpval, 1)) {
		if (utmpval)
			addbcromtype(p, ROMTYPE_CDTVSRAM, true, nullptr, 0);
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("scsi_cdtv"), &tmpval)) {
		if (tmpval)
			addbcromtype(p, ROMTYPE_CDTVSCSI, true, nullptr, 0);
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("pcmcia"), &p->cs_pcmcia)) {
		if (p->cs_pcmcia)
			addbcromtype(p, ROMTYPE_MB_PCMCIA, true, nullptr, 0);
		return 1;
	}
	if (cfgfile_strval(option, value, _T("ide"), &p->cs_ide, idemode, 0)) {
		if (p->cs_ide)
			addbcromtype(p, ROMTYPE_MB_IDE, true, nullptr, 0);
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("scsi_a3000"), &dummybool)) {
		if (dummybool) {
			addbcromtype(p, ROMTYPE_SCSI_A3000, true, nullptr, 0);
			p->cs_mbdmac = 1;
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("scsi_a4000t"), &dummybool)) {
		if (dummybool) {
			addbcromtype(p, ROMTYPE_SCSI_A4000T, true, nullptr, 0);
			p->cs_mbdmac = 2;
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("cd32fmv"), &p->cs_cd32fmv)) {
		if (p->cs_cd32fmv) {
			addbcromtype(p, ROMTYPE_CD32CART, true, p->cartfile, 0);
		}
		return 1;
	}
	if (cfgfile_intval(option, value, _T("catweasel"), &p->catweasel, 1)) {
		if (p->catweasel) {
			addbcromtype(p, ROMTYPE_CATWEASEL, true, nullptr, 0);
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("toccata"), &dummybool))
	{
		if (dummybool) {
			addbcromtype(p, ROMTYPE_TOCCATA, true, nullptr, 0);
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("es1370_pci"), &dummybool))
	{
		if (dummybool) {
			addbcromtype(p, ROMTYPE_ES1370, true, nullptr, 0);
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("fm801_pci"), &dummybool))
	{
		if (dummybool) {
			addbcromtype(p, ROMTYPE_FM801, true, nullptr, 0);
		}
		return 1;
	}
	if (cfgfile_yesno(option, value, _T("toccata_mixer"), &dummybool))
	{
		if (dummybool) {
			addbcromtype(p, ROMTYPE_TOCCATA, true, nullptr, 0);
		}
		return 1;
	}

	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtgboardconfig *rbc = &p->rtgboards[i];
		TCHAR tmp[100];
		if (i > 0)
			_sntprintf(tmp, sizeof tmp, _T("gfxcard%d_size"), i + 1);
		else
			_tcscpy(tmp, _T("gfxcard_size"));
		if (cfgfile_intval(option, value, tmp, &rbc->rtgmem_size, 0x100000))
			return 1;
		if (i > 0)
			_sntprintf(tmp, sizeof tmp, _T("gfxcard%d_options"), i + 1);
		else
			_tcscpy(tmp, _T("gfxcard_options"));
		if (!_tcsicmp(option, tmp)) {
			TCHAR *s = cfgfile_option_get(value, _T("order"));
			if (s) {
				rbc->device_order = _tstol(s);
				xfree(s);
			}
			s = cfgfile_option_get(value, _T("monitor"));
			if (s) {
				rbc->monitor_id = _tstol(s);
				xfree(s);
			}
			return 1;
		}
		if (i > 0)
			_sntprintf(tmp, sizeof tmp, _T("gfxcard%d_type"), i + 1);
		else
			_tcscpy(tmp, _T("gfxcard_type"));
		if (cfgfile_string(option, value, tmp, tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
			rbc->rtgmem_type = 0;
			rbc->rtg_index = i;
			int j = 0;
			for (;;) {
				const TCHAR *t = gfxboard_get_configname(j);
				if (!t) {
					break;
				}
				if (!_tcsicmp(t, tmpbuf)) {
					rbc->rtgmem_type = j;
					break;
				}
				j++;
			}
			return 1;
		}
	}

	if (cfgfile_string(option, value, _T("cpuboard_type"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		p->cpuboard_type = 0;
		p->cpuboard_subtype = 0;
		for (i = 0; cpuboards[i].name && !p->cpuboard_type; i++) {
			const struct cpuboardtype *cbt = &cpuboards[i];
			if (cbt->subtypes) {
				for (int j = 0; cbt->subtypes[j].name; j++) {
					if (!_tcsicmp(cbt->subtypes[j].configname, tmpbuf)) {
						p->cpuboard_type = i;
						p->cpuboard_subtype = j;
					}
				}
			}
		}
		return 1;
	}
	if (cfgfile_string(option, value, _T("cpuboard_settings"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		p->cpuboard_settings = 0;
		const struct cpuboardsubtype *cbst = &cpuboards[p->cpuboard_type].subtypes[p->cpuboard_subtype];
		if (cbst->settings) {
			p->cpuboard_settings = cfgfile_read_rom_settings(cbst->settings, tmpbuf, nullptr);
		}
		return 1;
	}

	if (cfgfile_strval (option, value, _T("chipset_compatible"), &p->cs_compatible, cscompa, 0)) {
		built_in_chipset_prefs (p);
		return 1;
	}

	if (cfgfile_strval (option, value, _T("cart_internal"), &p->cart_internal, cartsmode, 0)) {
		if (p->cart_internal) {
			struct romdata *rd = getromdatabyid (63);
			if (rd)
				_sntprintf (p->cartfile, sizeof p->cartfile, _T(":%s"), rd->configname);
		}
		return 1;
	}
	if (cfgfile_string (option, value, _T("kickstart_rom"), p->romident, sizeof p->romident / sizeof (TCHAR))) {
		decode_rom_ident (p->romfile, sizeof p->romfile / sizeof (TCHAR), p->romident, ROMTYPE_ALL_KICK);
		return 1;
	}
	if (cfgfile_string (option, value, _T("kickstart_ext_rom"), p->romextident, sizeof p->romextident / sizeof (TCHAR))) {
		decode_rom_ident (p->romextfile, sizeof p->romextfile / sizeof (TCHAR), p->romextident, ROMTYPE_ALL_EXT);
		return 1;
	}

	if (cfgfile_string (option, value, _T("cart"), p->cartident, sizeof p->cartident / sizeof (TCHAR))) {
		decode_rom_ident (p->cartfile, sizeof p->cartfile / sizeof (TCHAR), p->cartident, ROMTYPE_ALL_CART);
		return 1;
	}

	if (cfgfile_read_board_rom(p, option, value, &p->path_rom))
		return 1;

	for (i = 0; i < 4; i++) {
		_sntprintf (tmpbuf, sizeof tmpbuf, _T("floppy%d"), i);
		if (cfgfile_string(option, value, tmpbuf, p->floppyslots[i].df, sizeof p->floppyslots[i].df / sizeof(TCHAR))) {
			if (!_tcscmp(p->floppyslots[i].df, _T(".")))
				p->floppyslots[i].df[0] = 0;
			return 1;
		}
		_sntprintf(tmpbuf, sizeof tmpbuf, _T("floppy%dsubtypeid"), i);
		if (cfgfile_string_escape(option, value, tmpbuf, p->floppyslots[i].dfxsubtypeid, sizeof p->floppyslots[i].dfxsubtypeid / sizeof(TCHAR))) {
			return 1;
		}
		_sntprintf(tmpbuf, sizeof tmpbuf, _T("floppy%dprofile"), i);
		if (cfgfile_string_escape(option, value, tmpbuf, p->floppyslots[i].dfxprofile, sizeof p->floppyslots[i].dfxprofile / sizeof(TCHAR))) {
			return 1;
		}
	}

	if (cfgfile_intval (option, value, _T("chipmem_size"), &dummyint, 1)) {
		if (dummyint < 0)
			p->chipmem.size = 0x20000; /* 128k, prototype support */
		else if (dummyint == 0)
			p->chipmem.size = 0x40000; /* 256k */
		else
			p->chipmem.size = dummyint * 0x80000;
		return 1;
	}

	if (cfgfile_string (option, value, _T("addmem1"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		parse_addmem (p, tmpbuf, 0);
		return 1;
	}
	if (cfgfile_string (option, value, _T("addmem2"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		parse_addmem (p, tmpbuf, 1);
		return 1;
	}

	if (cfgfile_strval (option, value, _T("chipset"), &tmpval, csmode, 0)) {
		set_chipset_mask (p, tmpval);
		return 1;
	}

	if (cfgfile_yesno(option, value, _T("chipset_subpixel"), &p->chipset_hr)) {
		return 1;
	}

	if (cfgfile_string (option, value, _T("mmu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		TCHAR *s =_tcsstr(tmpbuf, _T("ec"));
		if (s) {
			p->mmu_ec = true;
			p->mmu_model = 68000 + _tstol(s + 2);
		} else {
			p->mmu_ec = false;
			p->mmu_model = _tstol(tmpbuf);
		}
		return 1;
	}

	if (cfgfile_string (option, value, _T("fpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		p->fpu_model = _tstol (tmpbuf);
		return 1;
	}

	if (cfgfile_string (option, value, _T("cpu_model"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		p->cpu_model = _tstol (tmpbuf);
		p->fpu_model = 0;
		return 1;
	}

	if (cfgfile_strval(option, value, _T("ppc_implementation"), &p->ppc_implementation, ppc_implementations, 0))
		return 1;
	if (cfgfile_string(option, value, _T("ppc_model"), tmpbuf, sizeof tmpbuf / sizeof(TCHAR))) {
		p->ppc_mode = 0;
		p->ppc_model[0] = 0;
		if (!_tcsicmp(tmpbuf, _T("automatic"))) {
			p->ppc_mode = 1;
		} else if (!_tcsicmp(tmpbuf, _T("manual"))) {
			p->ppc_mode = 2;
		} else {
			if (tmpbuf[0] && _tcslen(tmpbuf) < sizeof(p->ppc_model) / sizeof(TCHAR)) {
				_tcscpy(p->ppc_model, tmpbuf);
				p->ppc_mode = 2;
			}
		}
		return 1;
	}
	if (cfgfile_strval(option, value, _T("ppc_cpu_idle"), &p->ppc_cpu_idle, ppc_cpu_idle, 0))
		return 1;

	/* old-style CPU configuration */
	if (cfgfile_string (option, value, _T("cpu_type"), tmpbuf, sizeof tmpbuf / sizeof (TCHAR))) {
		// 68000/010 32-bit addressing was not available until 2.8.2
		bool force24bit = p->config_version <= ((2 << 16) | (8 << 8) | (1 << 0));
		p->fpu_model = 0;
		p->address_space_24 = false;
		p->cpu_model = 680000;
		if (!_tcscmp (tmpbuf, _T("68000"))) {
			p->cpu_model = 68000;
			if (force24bit)
				p->address_space_24 = true;
		} else if (!_tcscmp (tmpbuf, _T("68010"))) {
			p->cpu_model = 68010;
			if (force24bit)
				p->address_space_24 = true;
		} else if (!_tcscmp (tmpbuf, _T("68ec020"))) {
			p->cpu_model = 68020;
		} else if (!_tcscmp (tmpbuf, _T("68020"))) {
			p->cpu_model = 68020;
		} else if (!_tcscmp (tmpbuf, _T("68ec020/68881"))) {
			p->cpu_model = 68020;
			p->fpu_model = 68881;
			p->address_space_24 = true;
		} else if (!_tcscmp (tmpbuf, _T("68020/68881"))) {
			p->cpu_model = 68020;
			p->fpu_model = 68881;
		} else if (!_tcscmp (tmpbuf, _T("68040"))) {
			p->cpu_model = 68040;
			p->fpu_model = 68040;
		} else if (!_tcscmp (tmpbuf, _T("68060"))) {
			p->cpu_model = 68060;
			p->fpu_model = 68060;
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
	if (cfgfile_floatval(option, value, _T("cpu_throttle"), &p->m68k_speed_throttle)) {
		return 1;
	}
	if (cfgfile_floatval(option, value, _T("cpu_x86_throttle"), &p->x86_speed_throttle)) {
		return 1;
	}
	if (cfgfile_intval (option, value, _T("finegrain_cpu_speed"), &p->m68k_speed, 1)) {
		if (OFFICIAL_CYCLE_UNIT > CYCLE_UNIT) {
			int factor = OFFICIAL_CYCLE_UNIT / CYCLE_UNIT;
			p->m68k_speed = (p->m68k_speed + factor - 1) / factor;
		}
		if (_tcsicmp (value, _T("max")) == 0)
			p->m68k_speed = -1;
		return 1;
	}
	if (cfgfile_floatval(option, value, _T("blitter_throttle"), &p->blitter_speed_throttle)) {
		return 1;
	}

	if (cfgfile_intval (option, value, _T("dongle"), &p->dongle, 1)) {
		if (p->dongle == 0)
			cfgfile_strval (option, value, _T("dongle"), &p->dongle, dongles, 0);
		return 1;
	}

	if (_tcsicmp (option, _T("quickstart")) == 0) {
		int model = 0;
		TCHAR *tmpp = _tcschr (value, ',');
		if (tmpp) {
			*tmpp++ = 0;
			TCHAR *tmpp2 = _tcschr (value, ',');
			if (tmpp2)
				*tmpp2 = 0;
			cfgfile_strval (option, value, option, &model, qsmodes, 0);
			if (model >= 0) {
				int config = _tstol (tmpp);
				built_in_prefs (p, model, config, 0, 0);
			}
		}
		return 1;
	}

	if (cfgfile_string(option, value, _T("genlock_effects"), tmpbuf, sizeof(tmpbuf) / sizeof(TCHAR))) {
		TCHAR *s = tmpbuf;
		TCHAR *endptr;
		if (cfgfile_option_get(value, _T("brdntran"))) {
			p->genlock_effects |= 1 << 4;
		}
		if (cfgfile_option_get(value, _T("brdrblnk"))) {
			p->genlock_effects |= 1 << 5;
		}
		while (s && s[0]) {
			TCHAR *tmpp = _tcschr (s, ',');
			if (tmpp) {
				*tmpp++ = 0;
			}
			int plane = 0;
			if (s[0] == 'p' || s[0] == 'P') {
				s++;
				plane = 1;
			}
			int val = _tcstol(s, &endptr, 10);
			if (val > 0 || (val == 0 && s[0] == '0')) {
				if (plane == 1) {
					p->ecs_genlock_features_plane_mask |= 1 << val;
					p->genlock_effects |= 2;
				} else {
					for (int i = 0; i < 4 && val >= 0; i++, val -= 64) {
						if (val < 64) {
							p->ecs_genlock_features_colorkey_mask[i] |= 1LL << val;
							p->genlock_effects |= 1;
						}
					}
				}
			}
			s = tmpp;
		}
		return 1;
	}

	if (cfgfile_parse_filesys (p, option, value))
		return 1;

	return 0;
}

static void romtype_restricted(struct uae_prefs *p, const int *list)
{
	for (int i = 0; list[i]; i++) {
		int romtype = list[i];
		if (is_board_enabled(p, romtype, 0)) {
			i++;
			while (list[i]) {
				romtype = list[i];
				if (is_board_enabled(p, romtype, 0)) {
					write_log(_T("ROMTYPE %08x removed\n"), romtype);
					addbcromtype(p, romtype, false, nullptr, 0);
				}
				i++;
			}
			return;
		}
	}
}

void cfgfile_compatibility_rtg(struct uae_prefs *p)
{
	int uaegfx = -1;
	// only one uaegfx
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtgboardconfig *rbc = &p->rtgboards[i];
		if (rbc->rtgmem_size && rbc->rtgmem_type < GFXBOARD_HARDWARE) {
			if (uaegfx >= 0) {
				rbc->rtgmem_size = 0;
				rbc->rtgmem_type = 0;
			} else {
				uaegfx = i;
			}
		}
	}
	// uaegfx must be first
	if (uaegfx > 0) {
		struct rtgboardconfig *rbc = &p->rtgboards[uaegfx];
		struct rtgboardconfig *rbc2 = &p->rtgboards[0];
		int size = rbc->rtgmem_size;
		int type = rbc->rtgmem_type;
		rbc->rtgmem_size = rbc2->rtgmem_size;
		rbc->rtgmem_type = rbc2->rtgmem_type;
		rbc2->rtgmem_size = size;
		rbc2->rtgmem_type = type;
	}
	// allow only one a2410 and vga
	int a2410 = -1;
	int vga = -1;
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtgboardconfig *rbc = &p->rtgboards[i];
		if (rbc->rtgmem_type == GFXBOARD_ID_A2410) {
			if (a2410 >= 0) {
				rbc->rtgmem_size = 0;
				rbc->rtgmem_type = 0;
			} else {
				a2410 = i;
			}
		}
		if (rbc->rtgmem_type == GFXBOARD_ID_VGA) {
			if (vga >= 0) {
				rbc->rtgmem_size = 0;
				rbc->rtgmem_type = 0;
			} else {
				vga = i;
			}
		}
	}
	// empty slots last
	bool reorder = true;
	while (reorder) {
		reorder = false;
		for (int i = 0; i < MAX_RTG_BOARDS; i++) {
			struct rtgboardconfig *rbc = &p->rtgboards[i];
			if (i > 0 && rbc->rtgmem_size && p->rtgboards[i - 1].rtgmem_size == 0) {
				struct rtgboardconfig *rbc2 = &p->rtgboards[i - 1];
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
	int rtgs[MAX_RTG_BOARDS] = { 0 };
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if (p->rtgboards[i].rtgmem_size && !rtgs[i]) {
			uae_u32 romtype = gfxboard_get_romtype(&p->rtgboards[i]);
			if (romtype) {
				int devnum = 0;
				for (int j = i; j < MAX_RTG_BOARDS; j++) {
					rtgs[j] = 1;
					if (gfxboard_get_romtype(&p->rtgboards[j]) == romtype) {
						const TCHAR *romname = nullptr;
						if (romtype == ROMTYPE_PICASSOIV) {
							romname = p->picassoivromfile;
						} else if (romtype == ROMTYPE_x86_VGA) {
							romname = _T("");
						}
						addbcromtype(p, romtype, true, romname, devnum);
						devnum++;
					}
				}
				while (devnum < MAX_DUPLICATE_EXPANSION_BOARDS) {
					addbcromtype(p, romtype, false, nullptr, devnum);
					devnum++;
				}
			}
		}
	}
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if (!rtgs[i]) {
			uae_u32 romtype = gfxboard_get_romtype(&p->rtgboards[i]);
			if (romtype) {
				for (int devnum = 0; devnum < MAX_DUPLICATE_EXPANSION_BOARDS; devnum++) {
					addbcromtype(p, romtype, false, nullptr, devnum);
				}
			}
		}
	}
}

void cfgfile_compatibility_romtype(struct uae_prefs *p)
{
	addbcromtype(p, ROMTYPE_MB_PCMCIA, p->cs_pcmcia, nullptr, 0);

	addbcromtype(p, ROMTYPE_MB_IDE, p->cs_ide != 0, nullptr, 0);

	if (p->cs_mbdmac == 1) {
		addbcromtype(p, ROMTYPE_SCSI_A4000T, false, nullptr, 0);
		addbcromtype(p, ROMTYPE_SCSI_A3000, true, nullptr, 0);
	} else if (p->cs_mbdmac == 2) {
		addbcromtype(p, ROMTYPE_SCSI_A3000, false, nullptr, 0);
		addbcromtype(p, ROMTYPE_SCSI_A4000T, true, nullptr, 0);
	} else {
		addbcromtype(p, ROMTYPE_SCSI_A3000, false, nullptr, 0);
		addbcromtype(p, ROMTYPE_SCSI_A4000T, false, nullptr, 0);
	}

	addbcromtype(p, ROMTYPE_CDTVDMAC, p->cs_cdtvcd && !p->cs_cdtvcr, nullptr, 0);

	addbcromtype(p, ROMTYPE_CDTVCR, p->cs_cdtvcr, nullptr, 0);

	if (p->cs_cd32fmv) {
		addbcromtype(p, ROMTYPE_CD32CART, p->cs_cd32fmv, p->cartfile, 0);
	}
	p->cs_cd32fmv = get_device_romconfig(p, ROMTYPE_CD32CART, 0) != nullptr;

#ifndef AMIBERRY
	if (p->config_version < ((3 << 16) | (4 << 8) | (0 << 0))) {
		// 3.3.0 or older
		addbcromtypenet(p, ROMTYPE_A2065, p->a2065name, 0);
		addbcromtypenet(p, ROMTYPE_NE2KPCMCIA, p->ne2000pcmcianame, 0);
		addbcromtypenet(p, ROMTYPE_NE2KPCI, p->ne2000pciname, 0);
	}
#endif

	static const int restricted_net[] = {
		ROMTYPE_A2065, ROMTYPE_NE2KPCMCIA, ROMTYPE_NE2KPCI, ROMTYPE_NE2KISA,
		ROMTYPE_ARIADNE2, ROMTYPE_XSURF, ROMTYPE_XSURF100Z2, ROMTYPE_XSURF100Z3,
		ROMTYPE_HYDRA, ROMTYPE_LANROVER,
		0 };
	static const int restricted_x86[] = { ROMTYPE_A1060, ROMTYPE_A2088, ROMTYPE_A2088T, ROMTYPE_A2286, ROMTYPE_A2386, 0 };
	static const int restricted_pci[] = { ROMTYPE_GREX, ROMTYPE_MEDIATOR, ROMTYPE_PROMETHEUS, ROMTYPE_PROMETHEUSFS, 0 };
	romtype_restricted(p, restricted_net);
	romtype_restricted(p, restricted_x86);
	romtype_restricted(p, restricted_pci);
}

static int getconfigstoreline (const TCHAR *option, TCHAR *value);

static void calcformula (struct uae_prefs *prefs, TCHAR *in)
{
	TCHAR out[MAX_DPATH], configvalue[CONFIG_BLEN], tmp[MAX_DPATH];
	TCHAR *p = out;
	double val;
	int cnt1, cnt2;
	static bool updatestore;
	int escaped, quoted;

	if (_tcslen (in) < 2 || in[0] != '[' || in[_tcslen (in) - 1] != ']')
		return;
	if (!configstore || updatestore)
		cfgfile_createconfigstore (prefs);
	updatestore = false;
	if (!configstore)
		return;
	cnt1 = cnt2 = 0;
	escaped = 0;
	quoted = 0;
	for (int i = 1; i < _tcslen (in) - 1; i++) {
		TCHAR c = _totupper (in[i]);
		if (c == '\\') {
			escaped = 1;
		} else if (c == '\'') {
			quoted = escaped + 1;
			escaped = 0;
			cnt2++;
			*p++ = c;
		} else if (c >= 'A' && c <='Z') {
			TCHAR *start = &in[i];
			while (_istalnum (c) || c == '_' || c == '.') {
				i++;
				c = in[i];
			}
			TCHAR store = in[i];
			in[i] = 0;
			//write_log (_T("'%s'\n"), start);
			if ((quoted == 0 || quoted == 2) && getconfigstoreline (start, configvalue)) {
				_tcscpy(p, configvalue);
			} else {
				_tcscpy(p, start);
			}
			p += _tcslen (p);
			in[i] = store;
			i--;
			cnt1++;
			escaped = 0;
			quoted = 0;
		} else {
			cnt2++;
			*p++= c;
			escaped = 0;
			quoted = 0;
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
	int v = calc(out, &val, tmp, sizeof(tmp) / sizeof(TCHAR));
	if (v > 0) {
		if (val - static_cast<int>(val) != 0.0f)
			_sntprintf (in, sizeof in, _T("%f"), val);
		else
			_sntprintf (in, sizeof in, _T("%d"), static_cast<int>(val));
		updatestore = true;
	} else if (v < 0) {
		_tcscpy(in, tmp);
		updatestore = true;
	}
}

int cfgfile_parse_option (struct uae_prefs *p, const TCHAR *option, TCHAR *value, int type)
{
	calcformula (p, value);

	if (!_tcscmp (option, _T("debug"))) {
		write_log (_T("CONFIG DEBUG: '%s'\n"), value);
		return 1;
	}
	if (!_tcscmp (option, _T("config_hardware")))
		return 1;
	if (!_tcscmp (option, _T("config_host")))
		return 1;
	if (cfgfile_path (option, value, _T("config_all_path"), p->config_all_path, sizeof p->config_all_path / sizeof(TCHAR)))
		return 1;
	if (cfgfile_path (option, value, _T("config_hardware_path"), p->config_hardware_path, sizeof p->config_hardware_path / sizeof (TCHAR)))
		return 1;
	if (cfgfile_path (option, value, _T("config_host_path"), p->config_host_path, sizeof p->config_host_path / sizeof(TCHAR)))
		return 1;
	if (type == 0 || (type & CONFIG_TYPE_HARDWARE)) {
		if (cfgfile_parse_hardware (p, option, value))
			return 1;
	}
	if (type == 0 || (type & CONFIG_TYPE_HOST)) {
		// cfgfile_parse_host may modify the option (convert to lowercase).
		TCHAR* writable_option = my_strdup(option);
		if (cfgfile_parse_host (p, writable_option, value)) {
			free(writable_option);
			return 1;
		}
		free(writable_option);
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

int cfgfile_separate_linea (const TCHAR *filename, char *line, TCHAR *line1b, TCHAR *line2b)
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
		cfgfile_warning(_T("CFGFILE: '%s', linea was incomplete with only %s\n"), filename, s);
		xfree (s);
		return 0;
	}
	*line2++ = '\0';

	/* Get rid of whitespace.  */
	i = uaestrlen(line2);
	while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		|| line2[i - 1] == '\r' || line2[i - 1] == '\n'))
		line2[--i] = '\0';
	line2 += strspn (line2, "\t \r\n");

	i = uaestrlen(line);
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
		cfgfile_warning(_T("CFGFILE: line was incomplete with only %s\n"), line1);
		return 0;
	}
	*line2++ = '\0';

	/* Get rid of whitespace.  */
	i = uaetcslen(line2);
	while (i > 0 && (line2[i - 1] == '\t' || line2[i - 1] == ' '
		|| line2[i - 1] == '\r' || line2[i - 1] == '\n'))
		line2[--i] = '\0';
	line2 += _tcsspn (line2, _T("\t \r\n"));
	_tcscpy (line2b, line2);
	i = uaetcslen(line);
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
		if (!_tcsicmp (s, obsolete[i])) {
			cfgfile_warning_obsolete(_T("obsolete config entry '%s'\n"), s);
			return 1;
		}
		i++;
	}
	if (_tcslen (s) > 2 && !_tcsncmp (s, _T("w."), 2))
		return 1;
	if (_tcslen (s) >= 10 && !_tcsncmp (s, _T("gfx_opengl"), 10)) {
		cfgfile_warning_obsolete(_T("obsolete config entry '%s\n"), s);
		return 1;
	}
	if (_tcslen (s) >= 6 && !_tcsncmp (s, _T("gfx_3d"), 6)) {
		cfgfile_warning_obsolete(_T("obsolete config entry '%s\n"), s);
		return 1;
	}
	return 0;
}

static void cfgfile_parse_separated_line (struct uae_prefs *p, const TCHAR *line1b, TCHAR *line2b, int type)
{
	TCHAR line3b[CONFIG_BLEN], line4b[CONFIG_BLEN];
	struct strlist *sl;
	int ret;

	_tcscpy (line3b, line1b);
	_tcscpy (line4b, line2b);
	ret = cfgfile_parse_option (p, line1b, line2b, type);
	if (!isobsolete (line3b)) {
		for (sl = p->all_lines; sl; sl = sl->next) {
			if (sl->option && !_tcsicmp (line1b, sl->option)) break;
		}
		if (!sl) {
			struct strlist *u = xcalloc (struct strlist, 1);
			u->option = my_strdup (line3b);
			u->value = my_strdup (line4b);
			u->next = p->all_lines;
			p->all_lines = u;
			if (!ret) {
				u->unknown = 1;
				cfgfile_warning(_T("unknown config entry: '%s=%s'\n"), u->option, u->value);
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
	if (_tcslen(p) == 0 || _tcslen(f) == 0)
		return;
	TCHAR *str = cfgfile_subst_path (UNEXPANDED, p, f);
	_tcsncpy (f, str, n - 1);
	f[n - 1] = '\0';
	free (str);
}

const TCHAR *cfgfile_getconfigdata(size_t *len)
{
	*len = 0;
	if (!configstore)
		return nullptr;
	return reinterpret_cast<TCHAR*>(zfile_get_data_pointer(configstore, len));
}

static int getconfigstoreline (const TCHAR *option, TCHAR *value)
{
	TCHAR tmp[CONFIG_BLEN * 2], tmp2[CONFIG_BLEN * 2];

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

bool cfgfile_createconfigstore(struct uae_prefs *p)
{
	uae_u8 zeros[4] = { 0 };
	zfile_fclose (configstore);
	configstore = zfile_fopen_empty (nullptr, _T("configstore"), 50000);
	if (!configstore)
		return false;
	zfile_fseek (configstore, 0, SEEK_SET);
	uaeconfig++;
	cfgfile_save_options (configstore, p, 0);
	uaeconfig--;
	zfile_fwrite (zeros, 1, sizeof zeros, configstore);
	zfile_truncate(configstore, zfile_ftell(configstore));
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
	return nullptr;
}

static int cfgfile_load_2 (struct uae_prefs *p, const TCHAR *filename, bool real, int *type)
{
	struct zfile *fh;
	char linea[CONFIG_BLEN];
	TCHAR line[CONFIG_BLEN], line1b[CONFIG_BLEN], line2b[CONFIG_BLEN];
	bool type1 = false, type2 = false;
	int askedtype = 0;

	if (type) {
		askedtype = *type;
		*type = 0;
	}
	if (real) {
		p->config_version = 0;
		config_newfilesystem = 0;
		//reset_inputdevice_config (p);
	}

	fh = zfile_fopen (filename, _T("r"), ZFD_NORMAL);
#ifndef	SINGLEFILE
	if (! fh)
		return 0;
#endif

	while (cfg_fgets (linea, sizeof (linea), fh) != nullptr) {
		trimwsa (linea);
		if (strlen (linea) > 0) {
			if (linea[0] == '#' || linea[0] == ';') {
				struct strlist *u = xcalloc (struct strlist, 1);
				u->option = nullptr;
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
			type1 = type2 = false;
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
				// metadata
				cfgfile_string(line1b, line2b, _T("config_description"), p->description, sizeof p->description / sizeof(TCHAR));
				cfgfile_string(line1b, line2b, _T("config_category"), p->category, sizeof p->category / sizeof(TCHAR));
				cfgfile_string(line1b, line2b, _T("config_tags"), p->tags, sizeof p->tags / sizeof(TCHAR));
				cfgfile_path (line1b, line2b, _T("config_hardware_path"), p->config_hardware_path, sizeof p->config_hardware_path / sizeof (TCHAR));
				cfgfile_path (line1b, line2b, _T("config_host_path"), p->config_host_path, sizeof p->config_host_path / sizeof(TCHAR));
				cfgfile_path (line1b, line2b, _T("config_all_path"), p->config_all_path, sizeof p->config_all_path / sizeof(TCHAR));
				cfgfile_string (line1b, line2b, _T("config_window_title"), p->config_window_title, sizeof p->config_window_title / sizeof (TCHAR));
				// boxart checks
				cfgfile_string(line1b, line2b, _T("floppy0"), p->floppyslots[0].df, sizeof p->floppyslots[0].df / sizeof(TCHAR));
				cfgfile_resolve_path_load(p->floppyslots[0].df, MAX_DPATH, PATH_FLOPPY);
				TCHAR tmp[MAX_DPATH];
				if (!p->mountitems && (cfgfile_string(line1b, line2b, _T("hardfile2"), tmp, sizeof tmp / sizeof(TCHAR)) || cfgfile_string(line1b, line2b, _T("filesystem2"), tmp, sizeof tmp / sizeof(TCHAR)))) {
					const TCHAR *s = _tcschr(tmp, ':');
					if (s) {
						bool isvsys = false;
						if (!_tcscmp(line1b, _T("filesystem2"))) {
							s++;
							s = _tcschr(s, ':');
							isvsys = true;
						}
						if (s) {
							s++;
							if (s[0] == '"') {
								const TCHAR *end;
								TCHAR *n = cfgfile_unescape(s, &end, 0, true);
								_tcscpy(tmp, n);
								s = tmp;
								xfree(n);
							} else {
								const TCHAR *se = _tcschr(s, ',');
								if (se) {
									tmp[se - tmp] = 0;
								}
							}
							_tcscpy(p->mountconfig[0].ci.rootdir, s);
							cfgfile_resolve_path_load(p->mountconfig[0].ci.rootdir, MAX_DPATH, isvsys ? PATH_DIR : PATH_HDF);
							p->mountitems = 1;
						}
					}
				}
				if (!p->cdslots[0].inuse && cfgfile_path(line1b, line2b, _T("cdimage0"), tmp, sizeof tmp / sizeof(TCHAR))) {
					TCHAR *s = tmp;
					if (s[0] == '"') {
						s++;
						const TCHAR *se = _tcschr(s, '"');
						if (se)
							tmp[se - tmp] = 0;
					} else {
						const TCHAR *se = _tcsrchr(s, ',');
						const TCHAR *se2 = _tcsrchr(s, '.');
						if (se && (se2 == nullptr || se > se2)) {
							tmp[se - tmp] = 0;
						}
					}
					_tcscpy(p->cdslots[0].name, s);
					cfgfile_resolve_path_load(p->cdslots[0].name, MAX_DPATH, PATH_CD);
					p->cdslots[0].inuse = true;
				}
			}
		}
	}

	if (type && *type == 0)
		*type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
	zfile_fclose (fh);

	if (!real)
		return 1;

	for (auto* sl = temp_lines; sl; sl = sl->next) {
		_sntprintf (line, sizeof line, _T("%s=%s"), sl->option, sl->value);
		cfgfile_parse_line (p, line, 0);
	}

	subst (p->path_rom.path[0], p->romfile, sizeof p->romfile / sizeof (TCHAR));
	subst (p->path_rom.path[0], p->romextfile, sizeof p->romextfile / sizeof (TCHAR));
	subst (p->path_rom.path[0], p->romextfile2, sizeof p->romextfile2 / sizeof (TCHAR));

	for (auto & i : p->expansionboard) {
		for (auto & rom : i.roms) {
			subst(p->path_rom.path[0], rom.romfile, MAX_DPATH / sizeof(TCHAR));
		}
	}

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
	v = cfgfile_load_2 (p, filename, true, type);
	if (!v) {
		cfgfile_warning(_T("cfgfile_load_2 failed, retrying with defined config path\n"));
		// Do another attempt with the configuration path
		get_configuration_path(tmp, sizeof(tmp) / sizeof(TCHAR));
		std::string full_path = std::string(tmp) + std::string(filename);
		v = cfgfile_load_2(p, full_path.c_str(), true, type);
		if (!v)
		{
			cfgfile_warning(_T("cfgfile_load_2 failed, giving up\n"));
			goto end;
		}
	}
	// In Amiberry, we only use this function for adding recent disks, not configs
#ifndef AMIBERRY
	if (userconfig)
		target_addtorecent (filename, 0);
#endif
	if (!ignorelink) {
		if (p->config_all_path[0]) {
			get_configuration_path(tmp, sizeof(tmp) / sizeof(TCHAR));
			_tcsncat(tmp, p->config_all_path, sizeof(tmp) / sizeof(TCHAR) - _tcslen(tmp) - 1);
			type2 = CONFIG_TYPE_HOST | CONFIG_TYPE_HARDWARE;
			cfgfile_load(p, tmp, &type2, 1, 0);
		}
		if (p->config_hardware_path[0]) {
			get_configuration_path (tmp, sizeof (tmp) / sizeof (TCHAR));
			_tcsncat (tmp, p->config_hardware_path, sizeof (tmp) / sizeof (TCHAR) - _tcslen(tmp) - 1);
			type2 = CONFIG_TYPE_HARDWARE;
			cfgfile_load (p, tmp, &type2, 1, 0);
		}
		if (p->config_host_path[0]) {
			get_configuration_path (tmp, sizeof (tmp) / sizeof (TCHAR));
			_tcsncat (tmp, p->config_host_path, sizeof (tmp) / sizeof (TCHAR) - _tcslen(tmp) - 1);
			type2 = CONFIG_TYPE_HOST;
			cfgfile_load (p, tmp, &type2, 1, 0);
		}
	}
end:
	recursive--;
	for (int i = 1; i < MAX_AMIGADISPLAYS; i++) {
		memcpy(&p->gfx_monitor[i], &p->gfx_monitor[0], sizeof(struct monconfig));
	}
	fixup_prefs (p, userconfig != 0);
	for (int i = 0; i < MAX_JPORTS_CUSTOM; i++) {
		inputdevice_jportcustom_fixup(p, p->jports_custom[i].custom, i);
	}
	return v;
}

void cfgfile_backup(const TCHAR *path)
{
	TCHAR dpath[MAX_DPATH];

	get_configuration_path(dpath, sizeof(dpath) / sizeof(TCHAR));
	_tcscat (dpath, _T("configuration.backup"));
	bool hidden = my_isfilehidden (dpath);
	my_unlink (dpath);
	my_rename (path, dpath);
	if (hidden)
		my_setfilehidden (dpath, hidden);
}

int cfgfile_save (struct uae_prefs *p, const TCHAR *filename, int type)
{
	struct zfile *fh;

	cfgfile_backup (filename);
	fh = zfile_fopen (filename, _T("w, ccs=UTF-8"), ZFD_NORMAL);
	if (! fh)
		return 0;

	if (!type)
		type = CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST;
	cfgfile_save_options (fh, p, type);
	zfile_fclose (fh);
	return 1;
}

struct uae_prefs *cfgfile_open(const TCHAR *filename, int *type)
{
	struct uae_prefs *p = xcalloc(struct uae_prefs, 1);
	if (cfgfile_load_2(p, filename, false, type))
		return p;
	xfree(p);
	return nullptr;
}

void cfgfile_close(struct uae_prefs *p)
{
	xfree(p);
}

int cfgfile_get_description (struct uae_prefs *p, const TCHAR *filename, TCHAR *description, TCHAR *category, TCHAR *tags, TCHAR *hostlink, TCHAR *hardwarelink, int *type)
{
	bool alloc = false;

	if (!p) {
		p = xmalloc(struct uae_prefs, 1);
		alloc = true;
	}
	if (!p) {
		alloc = true;
		p = cfgfile_open(filename, type);
	}
	if (!p)
		return 0;
	if (description)
		_tcscpy(description, p->description);
	if (category)
		_tcscpy(category, p->category);
	if (tags)
		_tcscpy(tags, p->tags);
	if (hostlink)
		_tcscpy (hostlink, p->config_host_path);
	if (hardwarelink)
		_tcscpy (hardwarelink, p->config_hardware_path);
	if (alloc) {
		cfgfile_close(p);
	}
	return 1;
}

bool cfgfile_detect_art_path(const TCHAR *path, TCHAR *outpath)
{
	TCHAR tmp[MAX_DPATH];
	const TCHAR *p;
	if (!path[0])
		return false;
#if 0
	if (path[0] == '\\')
		return false;
#endif
	write_log(_T("Possible boxart path: '%s'\n"), path);
	_tcscpy(tmp, path);
	p = _tcsrchr(tmp, '\\');
	if (!p)
		p = _tcsrchr(tmp, '/');
	if (!p)
		return false;
	tmp[p - tmp] = 0;
	_tcscat(tmp, FSDB_DIR_SEPARATOR_S);
	_tcscat(tmp, _T("___Title.png"));
	if (!zfile_exists(tmp))
		return false;
	tmp[p - tmp + 1] = 0;
	_tcscpy(outpath, tmp);
	write_log(_T("Detected!\n"));
	return true;
}

bool cfgfile_detect_art(struct uae_prefs *p, TCHAR *path)
{
	if (cfgfile_detect_art_path(p->floppyslots[0].df, path))
		return true;
	if (p->mountitems > 0 && cfgfile_detect_art_path(p->mountconfig[0].ci.rootdir, path))
		return true;
	if (p->cdslots[0].inuse && cfgfile_detect_art_path(p->cdslots[0].name, path))
		return true;
	return false;
}

void cfgfile_show_usage ()
{
	write_log (_T("UAE Configuration Help:\n") \
		_T("=======================\n"));
	for (auto i : opttable)
		write_log(_T("%s: %s\n"), i.config_label, i.config_help);
}

/* This implements the old commandline option parsing. I've re-added this
because the new way of doing things is painful for me (it requires me
to type a couple of hundred characters when invoking UAE).  The following
is far less annoying to use.  */
static void parse_gfx_specs (struct uae_prefs *p, const TCHAR *spec)
{
	TCHAR *x0 = my_strdup (spec);
	TCHAR *x1, *x2;

	x1 = _tcschr (x0, ':');
	if (x1 == nullptr)
		goto argh;
	x2 = _tcschr (x1+1, ':');
	if (x2 == nullptr)
		goto argh;
	*x1++ = 0; *x2++ = 0;

	p->gfx_monitor[0].gfx_size_win.width = p->gfx_monitor[0].gfx_size_fs.width = _tstoi (x0);
	p->gfx_monitor[0].gfx_size_win.height = p->gfx_monitor[0].gfx_size_fs.height = _tstoi (x1);
	p->gfx_resolution = _tcschr (x2, 'l') != nullptr ? 1 : 0;
	p->gfx_xcenter = _tcschr (x2, 'x') != nullptr ? 1 : _tcschr (x2, 'X') != nullptr ? 2 : 0;
	p->gfx_ycenter = _tcschr (x2, 'y') != nullptr ? 1 : _tcschr (x2, 'Y') != nullptr ? 2 : 0;
	p->gfx_vresolution = _tcschr (x2, 'd') != nullptr ? VRES_DOUBLE : VRES_NONDOUBLE;
	p->gfx_pscanlines = _tcschr (x2, 'D') != nullptr;
	if (p->gfx_pscanlines)
		p->gfx_vresolution = VRES_DOUBLE;
	p->gfx_apmode[0].gfx_fullscreen = _tcschr (x2, 'a') != nullptr;
	p->gfx_apmode[1].gfx_fullscreen = _tcschr (x2, 'p') != nullptr;

	free (x0);
	return;

argh:
	write_log (_T("Bad display mode specification.\n"));
	write_log (_T("The format to use is: \"width:height:modifiers\"\n"));
	write_log (_T("Type \"uae -h\" for detailed help.\n"));
	free (x0);
}

static void parse_sound_spec (struct uae_prefs *p, const TCHAR *spec)
{
	TCHAR *x0 = my_strdup (spec);
	TCHAR *x1, *x2 = nullptr, *x3 = nullptr, *x4 = nullptr, *x5 = nullptr;

	x1 = _tcschr (x0, ':');
	if (x1 != nullptr) {
		*x1++ = '\0';
		x2 = _tcschr (x1 + 1, ':');
		if (x2 != nullptr) {
			*x2++ = '\0';
			x3 = _tcschr (x2 + 1, ':');
			if (x3 != nullptr) {
				*x3++ = '\0';
				x4 = _tcschr (x3 + 1, ':');
				if (x4 != nullptr) {
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
	if (x4)
		p->sound_maxbsiz = _tstoi (x4);
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
	if (false)
bad:
	write_log (_T("Bad joystick mode specification. Use -J xy, where x and y\n")
		_T("can be 0 for joystick 0, 1 for joystick 1, M for mouse, and\n")
		_T("a, b or c for different keyboard settings.\n"));

	p->jports[0].id = v0;
	p->jports[1].id = v1;
}

static void parse_filesys_spec (struct uae_prefs *p, bool readonly, const TCHAR *spec)
{
	struct uaedev_config_info uci{};
	TCHAR buf[256];
	TCHAR *s2;

	uci_set_defaults (&uci, false);
	_tcsncpy (buf, spec, 255);
	buf[255] = 0;
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
	uaedev_config_data* uci;
	auto parameter = std::string(spec);
	std::string x1;

	const auto pos = parameter.find(':');
	if (pos != std::string::npos)
	{
		x1 = parameter.substr(0, pos);
		x1 += ':';
	}
	const std::string x2 = parameter.substr(pos + 1, parameter.length());
#ifdef FILESYS
	default_hfdlg(&current_hfdlg, false);
	std::string txt1, txt2;
	updatehdfinfo(true, false, false, txt1, txt2);

	current_hfdlg.ci.type = UAEDEV_HDF;
	_tcscpy(current_hfdlg.ci.devname, x1.c_str());
	_tcscpy(current_hfdlg.ci.rootdir, x2.c_str());
	hardfile_testrdb(&current_hfdlg);

	uaedev_config_info ci{};
	memcpy(&ci, &current_hfdlg.ci, sizeof(uaedev_config_info));
	uci = add_filesys_config(p, -1, &ci);
	if (uci) {
		if (auto* const hfd = get_hardfile_data(uci->configoffset))
			hardfile_media_change(hfd, &ci, true, false);
	}
#endif
}

static void parse_cpu_specs (struct uae_prefs *p, const TCHAR *spec)
{
	if (*spec < '0' || *spec > '4') {
		cfgfile_warning(_T("CPU parameter string must begin with '0', '1', '2', '3' or '4'.\n"));
		return;
	}

	p->cpu_model = (*spec++) * 10 + 68000;
	p->address_space_24 = p->cpu_model < 68020;
	p->cpu_compatible = false;
	while (*spec != '\0') {
		switch (*spec) {
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

static void cmdpath (TCHAR *dst, const TCHAR *src, int maxsz)
{
	TCHAR *s = target_expand_environment (src, nullptr, 0);
	_tcsncpy (dst, s, maxsz);
	dst[maxsz] = 0;
	xfree (s);
}

/* Returns the number of args used up (0 or 1).  */
int parse_cmdline_option (struct uae_prefs *p, TCHAR c, const TCHAR *arg)
{
	struct strlist *u = xcalloc (struct strlist, 1);
	constexpr TCHAR arg_required[] = _T("0123rKpImMWSRJwnvCZUFbclOdH");

	if (_tcschr (arg_required, c) && ! arg) {
		write_log (_T("Missing argument for option `-%c'!\n"), c);
		return 0;
	}

	u->option = xmalloc (TCHAR, 2);
	u->option[0] = c;
	u->option[1] = 0;
	if (arg)
		u->value = my_strdup (arg);
	u->next = p->all_lines;
	p->all_lines = u;

	switch (c) {
	case 'h': usage (); exit (0);

	case '0':
		cmdpath (p->floppyslots[0].df, arg, 255);
#ifdef AMIBERRY
		target_addtorecent(arg, 0);
#endif
		break;
	case '1':
		cmdpath (p->floppyslots[1].df, arg, 255);
#ifdef AMIBERRY
		target_addtorecent(arg, 0);
#endif
		break;
	case '2':
		cmdpath (p->floppyslots[2].df, arg, 255);
#ifdef AMIBERRY
		target_addtorecent(arg, 0);
#endif
		break;
	case '3':
		cmdpath (p->floppyslots[3].df, arg, 255);
#ifdef AMIBERRY
		target_addtorecent(arg, 0);
#endif
		break;

	case 'r': cmdpath (p->romfile, arg, 255); break;
	case 'K': cmdpath (p->romextfile, arg, 255); break;
	case 'p': _tcsncpy (p->prtname, arg, 255); p->prtname[255] = 0; break;
	/*  case 'I': _tcsncpy (p->sername, arg, 255); p->sername[255] = 0; currprefs.use_serial = 1; break; */
	case 'm': case 'M': parse_filesys_spec (p, c == 'M', arg); break;
	case 'W': parse_hardfile_spec (p, arg); break;
	case 'S': parse_sound_spec (p, arg); break;
	case 'R': p->gfx_framerate = _tstoi (arg); break;
	case 'i': p->illegal_mem = true; break;
	case 'J': parse_joy_spec (p, arg); break;

	case 'w': p->m68k_speed = _tstoi (arg); break;

		/* case 'g': p->use_gfxlib = 1; break; */
	case 'G': p->start_gui = false; break;
#ifdef DEBUGGER
	case 'D': p->start_debugger = true; break;
#endif

	case 'n':
		if (_tcschr (arg, 'i') != nullptr)
			p->immediate_blits = true;
		break;

	case 'v':
		set_chipset_mask (p, _tstoi (arg));
		break;

	case 'C':
		parse_cpu_specs (p, arg);
		break;

	case 'Z':
		p->z3fastmem[0].size = _tstoi (arg) * 0x100000;
		break;

	case 'U':
		p->rtgboards[0].rtgmem_size = _tstoi (arg) * 0x100000;
		break;

	case 'F':
		p->fastmem[0].size = _tstoi (arg) * 0x100000;
		break;

	case 'b':
		p->bogomem.size = _tstoi (arg) * 0x40000;
		break;

	case 'c':
		p->chipmem.size = _tstoi (arg) * 0x80000;
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

	case 'O': parse_gfx_specs (p, arg); break;
	case 'd':
		if (_tcschr (arg, 'S') != nullptr || _tcschr (arg, 's')) {
			write_log (_T("  Serial on demand.\n"));
			p->serial_demand = true;
		}
		if (_tcschr (arg, 'P') != nullptr || _tcschr (arg, 'p')) {
			write_log (_T("  Parallel on demand.\n"));
			p->parallel_demand = true;
		}

		break;

	case 'H':
		p->color_mode = _tstoi (arg);
		if (p->color_mode < 0) {
			write_log (_T("Bad color mode selected. Using default.\n"));
			p->color_mode = 0;
		}
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
		temp_lines = nullptr;
		return;
	}
	if (!cfgfile_separate_line (line, line1b, line2b))
		return;
	if (!_tcsnicmp(line1b, _T("input."), 6)) {
		line1b[5] = '_';
	}
	u = xcalloc (struct strlist, 1);
	if (u) {
		u->option = my_strdup(line1b);
		u->value = my_strdup(line2b);
		u->next = temp_lines;
		temp_lines = u;
	}
}

#if 0
static int cfgfile_handle_custom_event (TCHAR *custom, int mode)
{
	TCHAR option[CONFIG_BLEN], value[CONFIG_BLEN];
	TCHAR option2[CONFIG_BLEN], value2[CONFIG_BLEN];
	TCHAR *tmp, *p, *nextp;
	struct zfile *configstore = NULL;
	int cnt = 0, cnt_ok = 0;

	if (!mode) {
		TCHAR zero = 0;
		configstore = zfile_fopen_empty ("configstore", 50000);
		cfgfile_save_options (configstore, &currprefs, 0);
		cfg_write (&zero, configstore);
	}

	nextp = NULL;
	tmp = p = xcalloc (TCHAR, _tcslen (custom) + 2);
	_tcscpy (tmp, custom);
	while (p && *p) {
		if (*p == '\"') {
			TCHAR *p2;
			p++;
			p2 = p;
			while (*p2 != '\"' && *p2 != 0)
				p2++;
			if (*p2 == '\"') {
				*p2++ = 0;
				nextp = p2 + 1;
				if (*nextp == ' ')
					nextp++;
			}
		}
		if (cfgfile_separate_line (p, option, value)) {
			cnt++;
			if (mode) {
				cfgfile_parse_option (&changed_prefs, option, value, 0);
			} else {
				zfile_fseek (configstore, 0, SEEK_SET);
				for (;;) {
					if (!getconfigstoreline (configstore, option2, value2))
						break;
					if (!_tcscmpi (option, option2) && !_tcscmpi (value, value2)) {
						cnt_ok++;
						break;
					}
				}
			}
		}
		p = nextp;
	}
	xfree (tmp);
	zfile_fclose (configstore);
	if (cnt > 0 && cnt == cnt_ok)
		return 1;
	return 0;
}
#endif

static int cmdlineparser (const TCHAR *s, TCHAR *outp[], int max)
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
	outp[0] = nullptr;
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
				outp[cnt] = nullptr;
			}
			tmp1[0] = 0;
			doout = 0;
			j = 0;
		}
		slash = 0;
	}
	if (j > 0 && cnt < max) {
		outp[cnt++] = my_strdup (tmp1);
		outp[cnt] = nullptr;
	}
	return cnt;
}

#define UAELIB_MAX_PARSE 100

static bool cfgfile_parse_uaelib_option (struct uae_prefs *p, const TCHAR *option, const TCHAR *value, int type)
{
	TCHAR tmp[MAX_DPATH];
	if (cfgfile_path(option, value, _T("statefile_save"), tmp, sizeof(tmp) / sizeof(TCHAR))) {
		if (!savestate_state) {
			savestate_state = STATE_SAVE;
			get_savestate_path(savestate_fname, sizeof(savestate_fname) / sizeof(TCHAR));
			_tcscat(savestate_fname, tmp);
			if (_tcslen(savestate_fname) >= 4 && _tcsicmp(savestate_fname + _tcslen(savestate_fname) - 4, _T(".uss"))) {
				_tcscat(savestate_fname, _T(".uss"));
			}
		}
		return true;
	}

	return false;
}

int cfgfile_searchconfig(const TCHAR *in, int index, TCHAR *out, int outsize)
{
	TCHAR tmp[CONFIG_BLEN];
	int j = 0;
	int inlen = uaetcslen(in);
	int joker = 0;
	uae_u32 err = 0;
	bool configsearchfound = false;
	int index2 = index;

	if (in[inlen - 1] == '*') {
		joker = 1;
		inlen--;
	}
	*out = 0;

	if (!configstore)
		cfgfile_createconfigstore(&currprefs);
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
			if (configsearchfound || index2 > 0)
				err = 0;
			goto end;
		}
		if (j >= sizeof (tmp) / sizeof (TCHAR) - 1)
			j = sizeof (tmp) / sizeof (TCHAR) - 1;
		if (b == 0) {
			err = 10;
			if (configsearchfound || index2 > 0)
				err = 0;
			goto end;
		}
		if (b == '\n') {
			if (!_tcsncmp (tmp, in, inlen) && ((inlen > 0 && _tcslen (tmp) > inlen && tmp[inlen] == '=') || (joker))) {
				if (index <= 0) {
					TCHAR *p;
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

static void shellexec_cb(uae_u32 id, uae_u32 status, uae_u32 flags, const char *outbuf, void *userdata)
{
	if (flags & 1) {
		write_log("Return status code: %d\n", status);
	}
	if (flags & 2) {
		if (outbuf) {
			write_log("%s\n", outbuf);
		}
	}
}

static int execcmdline(struct uae_prefs *prefs, int argv, TCHAR **argc, TCHAR *out, int outsize, bool confonly)
{
	int ret = 0;
	bool changed = false;
	for (int i = 0; i < argv; i++) {
		if (i + 2 <= argv) {
			if (!confonly) {
				if (!_tcsicmp(argc[i], _T("shellexec"))) {
					TCHAR *cmd = argc[i + 1];
					uae_u32 flags = 0;
					i++;
					while (argv > i + 1) {
						TCHAR *next = argc[i + 1];
						if (!_tcsicmp(next, _T("out"))) {
							flags |= 2;
						}
						if (!_tcsicmp(next, _T("res"))) {
							flags |= 1;
						}
					}
					filesys_shellexecute2(cmd, nullptr, nullptr, 0, 0, 0, flags, nullptr, 0, shellexec_cb, nullptr);
				}
#ifdef DEBUGGER
		else if (!_tcsicmp(argc[i], _T("dbg"))) {
					debug_parser(argc[i + 1], out, outsize);
				}
#endif
				else if (!inputdevice_uaelib(argc[i], argc[i + 1])) {
					if (!cfgfile_parse_uaelib_option(prefs, argc[i], argc[i + 1], 0)) {
						if (!cfgfile_parse_option(prefs, argc[i], argc[i + 1], 0)) {
							ret = 5;
							break;
						}
					}
					changed = true;
				}
			} else {
				if (!cfgfile_parse_option(prefs, argc[i], argc[i + 1], 0)) {
					ret = 5;
					break;
				}
				changed = true;
			}
			i++;
		}
	}
	if (changed) {
		inputdevice_fix_prefs(prefs, false);
		set_config_changed();
		set_special(SPCFLAG_MODE_CHANGE);
	}
	return 0;
}

uae_u32 cfgfile_modify (uae_u32 index, const TCHAR *parms, uae_u32 size, TCHAR *out, uae_u32 outsize)
{
	TCHAR *p;
	TCHAR *argc[UAELIB_MAX_PARSE];
	int argv, i;
	uae_u32 err;
	static TCHAR *configsearch;

	if (out)
		*out = 0;
	err = 0;
	argv = 0;
	p = nullptr;
	if (index != 0xffffffff) {
		if (!configstore) {
			err = 20;
			goto end;
		}
		if (configsearch) {
			err = cfgfile_searchconfig(configsearch, index, out, outsize);
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
		cfgfile_createconfigstore (&currprefs);
		xfree (configsearch);
		configsearch = nullptr;
		if (!configstore) {
			err = 20;
			goto end;
		}
		if (argv > 0 && _tcslen (argc[0]) > 0)
			configsearch = my_strdup (argc[0]);
		err = 0xffffffff;
		goto end;
	}

	err = execcmdline(&changed_prefs, argv, argc, out, outsize, false);
end:
	for (i = 0; i < argv; i++)
		xfree (argc[i]);
	xfree (p);
	return err;
}

uae_u32 cfgfile_uaelib_modify(TrapContext *ctx, uae_u32 index, uae_u32 parms, uae_u32 size, uae_u32 out, uae_u32 outsize)
{
	uae_char *p, *parms_p = nullptr, *parms_out = nullptr;
	int i, ret;
	TCHAR *out_p = nullptr, *parms_in = nullptr;

	if (outsize >= 32768)
		return 0;
	if (out && outsize > 0) {
		if (!trap_valid_address(ctx, out, 1))
			return 0;
		trap_put_byte(ctx, out, 0);
	}
	if (size == 0) {
		for (;;) {
			if (!trap_valid_address(ctx, parms + size, 1))
				return 0;
			if (trap_get_byte(ctx, parms + size) == 0)
				break;
			size++;
			if (size >= 32768)
				return 0;
		}
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
		p[i] = trap_get_byte(ctx, parms + i);
		if (p[i] == 10 || p[i] == 13 || p[i] == 0)
			break;
	}
	p[i] = 0;
	parms_in = au (parms_p);
	ret = cfgfile_modify (index, parms_in, size, out_p, outsize);
	xfree (parms_in);
	if (out && outsize > 0) {
		parms_out = ua (out_p);
		if (!trap_valid_address(ctx, out, strlen(parms_out) + 1 > outsize ? outsize : uaestrlen(parms_out) + 1))
			return 0;
		trap_put_string(ctx, parms_out, out, outsize - 1);
	}
	xfree (parms_out);
end:
	xfree (out_p);
	xfree (parms_p);
	return ret;
}

static const TCHAR *cfgfile_read_config_value (const TCHAR *option)
{
	struct strlist *sl;
	for (sl = currprefs.all_lines; sl; sl = sl->next) {
		if (sl->option && !_tcsicmp (sl->option, option))
			return sl->value;
	}
	return nullptr;
}

uae_u32 cfgfile_uaelib(TrapContext *ctx, int mode, uae_u32 name, uae_u32 dst, uae_u32 maxlen)
{
	TCHAR *str;
	uae_char tmpa[CONFIG_BLEN];

	if (mode || maxlen > CONFIG_BLEN)
		return 0;

	if (!trap_valid_string(ctx, name, CONFIG_BLEN))
		return 0;
	if (!trap_valid_address(ctx, dst, maxlen))
		return 0;

	trap_get_string(ctx, tmpa, name, sizeof tmpa);
	str = au(tmpa);
	if (str[0] == 0) {
		xfree(str);
		return 0;
	}
	const TCHAR *value = cfgfile_read_config_value(str);
	xfree(str);
	if (value) {
		char *s = ua(value);
		trap_put_string(ctx, s, dst, maxlen);
		xfree (s);
		return dst;
	}
	return 0;
}

uae_u8 *restore_configuration (uae_u8 *src)
{
	struct uae_prefs *p = &currprefs;
	TCHAR *sp = au (reinterpret_cast<char*>(src));
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
			return src += strlen(reinterpret_cast<char*>(src)) + 1;
		}
		*s++ = 0;
		while (*s == 10 || *s == 13)
			s++;
		if (cfgfile_separate_line(ss, option, value)) {
			if (!_tcsncmp(option, _T("diskimage"), 9)) {
				for (int i = 0; i < MAX_SPARE_DRIVES; i++) {
					_sntprintf(tmp, sizeof tmp, _T("diskimage%d"), i);
					if (!_tcscmp(option, tmp)) {
						cfgfile_string(option, value, tmp, p->dfxlist[i], sizeof p->dfxlist[i] / sizeof(TCHAR));
						break;
					}
				}
			}
		}
	}
}

uae_u8 *save_configuration (size_t *len, bool fullconfig)
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
			strcpy (reinterpret_cast<char*>(p), out);
			xfree (out);
			strcat (reinterpret_cast<char*>(p), "\n");
			p += strlen (reinterpret_cast<char*>(p));
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
	p->chipmem.size = 0x00080000;
	p->bogomem.size = 0x00080000;
}
#endif

#include "sounddep/sound.h"

static void copy_inputdevice_settings(const struct uae_input_device *src, struct uae_input_device *dst)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			if (src->custom[l][i]) {
				dst->custom[l][i] = my_strdup(src->custom[l][i]);
			} else {
				dst->custom[l][i] = nullptr;
			}
		}
	}
}

static void copy_inputdevice_settings_free(const struct uae_input_device *src, struct uae_input_device *dst)
{
	for (int l = 0; l < MAX_INPUT_DEVICE_EVENTS; l++) {
		for (int i = 0; i < MAX_INPUT_SUB_EVENT_ALL; i++) {
			if (dst->custom[l][i] == nullptr && src->custom[l][i] == nullptr)
				continue;
			// same string in both src and dest: separate them (fixme: this shouldn't happen!)
			if (dst->custom[l][i] == src->custom[l][i]) {
				dst->custom[l][i] = nullptr;
			} else {
				if (dst->custom[l][i]) {
					xfree(dst->custom[l][i]);
					dst->custom[l][i] = nullptr;
				}
			}
		}
	}
}

void copy_prefs(const struct uae_prefs *src, struct uae_prefs *dst)
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

void copy_inputdevice_prefs(const struct uae_prefs *src, struct uae_prefs *dst)
{
	for (int slot = 0; slot < MAX_INPUT_SETTINGS; slot++) {
		for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
			copy_inputdevice_settings_free(&src->joystick_settings[slot][m], &dst->joystick_settings[slot][m]);
			copy_inputdevice_settings_free(&src->mouse_settings[slot][m], &dst->mouse_settings[slot][m]);
			copy_inputdevice_settings_free(&src->keyboard_settings[slot][m], &dst->keyboard_settings[slot][m]);
		}
	}
	for (int i = 0; i < MAX_INPUT_SETTINGS; i++) {
		for (int j = 0; j < MAX_INPUT_DEVICES; j++) {
			memcpy(&dst->joystick_settings[i][j], &src->joystick_settings[i][j], sizeof(struct uae_input_device));
			memcpy(&dst->mouse_settings[i][j], &src->mouse_settings[i][j], sizeof(struct uae_input_device));
			memcpy(&dst->keyboard_settings[i][j], &src->keyboard_settings[i][j], sizeof(struct uae_input_device));
		}
	}
	for (int slot = 0; slot < MAX_INPUT_SETTINGS; slot++) {
		for (int m = 0; m < MAX_INPUT_DEVICES; m++) {
			copy_inputdevice_settings(&src->joystick_settings[slot][m], &dst->joystick_settings[slot][m]);
			copy_inputdevice_settings(&src->mouse_settings[slot][m], &dst->mouse_settings[slot][m]);
			copy_inputdevice_settings(&src->keyboard_settings[slot][m], &dst->keyboard_settings[slot][m]);
		}
	}
}

void default_prefs (struct uae_prefs *p, bool reset, int type)
{
	int roms[] = { 6, 7, 8, 9, 10, 14, 5, 4, 3, 2, 1, -1 };
	TCHAR zero = 0;

	reset_inputdevice_config (p, reset);
	memset (p, 0, sizeof (struct uae_prefs));
	_tcscpy (p->description, _T("UAE default configuration"));
	p->config_hardware_path[0] = 0;
	p->config_host_path[0] = 0;

	p->gfx_scandoubler = false;
	p->start_gui = true;
	p->start_debugger = false;

	p->all_lines = nullptr;
	/* Note to porters: please don't change any of these options! UAE is supposed
	* to behave identically on all platforms if possible.
	* (TW says: maybe it is time to update default config...) */
	p->illegal_mem = false;
	p->use_serial = false;
	p->serial_demand = false;
	p->serial_hwctsrts = true;
	p->serial_ri = false;
	p->serial_rtsctsdtrdtecd = true;
	p->serial_stopbits = 0;
	p->parallel_demand = false;
	p->parallel_matrix_emulation = 0;
	p->parallel_postscript_emulation = false;
	p->parallel_postscript_detection = false;
	p->parallel_autoflush_time = 5;
	p->ghostscript_parameters[0] = 0;
	p->uae_hide = 0;
	p->uae_hide_autoconfig = false;
	p->z3_mapping_mode = Z3MAPPING_AUTO;

	clearmountitems(p);

	p->jports[0].id = -1;
	p->jports[1].id = -1;
	p->jports[2].id = -1;
	p->jports[3].id = -1;
	if (reset) {
		inputdevice_joyport_config_store(p, _T("mouse"), 0, -1, -1, 0);
		inputdevice_joyport_config_store(p, _T("joy0"), 1, -1, -1, 0);
	}
	p->keyboard_lang = KBD_LANG_US;
	p->keyboard_mode = 0;
	p->keyboard_nkro = true;

	p->produce_sound = 3;
	p->sound_stereo = SND_STEREO;
	p->sound_stereo_separation = 7;
	p->sound_mixed_stereo_delay = 0;
	p->sound_freq = DEFAULT_SOUND_FREQ;
	p->sound_maxbsiz = DEFAULT_SOUND_MAXB;
	p->sound_interpol = 1;
	p->sound_filter = FILTER_SOUND_EMUL;
	p->sound_filter_type = 0;
	p->sound_auto = false;
	p->sampler_stereo = false;
	p->sampler_buffer = 0;
	p->sampler_freq = 0;
#ifdef AMIBERRY
	p->sound_volume_cd = 20;
	whdload_prefs.write_cache = false;
#endif
	p->comptrustbyte = 0;
	p->comptrustword = 0;
	p->comptrustlong = 0;
	p->comptrustnaddr= 0;
	p->compnf = true;
	p->comp_hardflush = false;
	p->comp_constjump = true;
#ifdef USE_JIT_FPU
	p->compfpu = 1;
#else
	p->compfpu = false;
#endif
	p->comp_catchfault = true;
	p->cachesize = 0;

	p->gfx_framerate = 1;
	p->gfx_autoframerate = 50;
	p->gfx_monitor[0].gfx_size_fs.width = 800;
	p->gfx_monitor[0].gfx_size_fs.height = 600;
	p->gfx_monitor[0].gfx_size_win.width = 720;
	p->gfx_monitor[0].gfx_size_win.height = 568;
	for (int i = 0; i < GFX_SIZE_EXTRA_NUM; i++) {
		p->gfx_monitor[0].gfx_size_fs_xtra[i].width = 0;
		p->gfx_monitor[0].gfx_size_fs_xtra[i].height = 0;
		p->gfx_monitor[0].gfx_size_win_xtra[i].width = 0;
		p->gfx_monitor[0].gfx_size_win_xtra[i].height = 0;
	}
	p->gfx_resolution = RES_HIRES;
	p->gfx_vresolution = VRES_DOUBLE;
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
	p->gfx_blackerthanblack = false;
	p->gfx_autoresolution_vga = true;
	p->gfx_apmode[0].gfx_backbuffers = 2;
	p->gfx_apmode[1].gfx_backbuffers = 1;
	p->gfx_display_sections = 4;
	p->gfx_variable_sync = 0;
	p->gfx_windowed_resize = true;
	p->gfx_overscanmode = 3;

	p->immediate_blits = false;
	p->waiting_blits = 0;
	p->collision_level = 2;
	p->leds_on_screen = 0;
	p->leds_on_screen_mask[0] = p->leds_on_screen_mask[1] = (1 << LED_MAX) - 1;
	p->keyboard_leds_in_use = false;
	p->keyboard_leds[0] = p->keyboard_leds[1] = p->keyboard_leds[2] = 0;
	p->scsi = 0;
	p->uaeserial = false;
	p->cpu_idle = 0;
	p->turbo_emulation = 0;
	p->turbo_emulation_limit = 0;
	p->turbo_boot = false;
	p->turbo_boot_delay = 100;
	p->headless = false;
	p->catweasel = 0;
	p->tod_hack = false;
	p->maprom = 0;
	p->boot_rom = 0;
	p->filesys_no_uaefsdb = false;
	p->filesys_custom_uaefsdb = true;
	p->picasso96_nocustom = true;
	p->cart_internal = 1;
	p->sana2 = false;
	p->clipboard_sharing = false;
	p->native_code = false;
	p->lightpen_crosshair = true;
	p->gfx_monitorblankdelay = 0;

	p->cs_compatible = CP_GENERIC;
	p->cs_rtc = 2;
	p->cs_df0idhw = true;
	p->cs_a1000ram = false;
	p->cs_fatgaryrev = -1;
	p->cs_ramseyrev = -1;
	p->cs_agnusrev = -1;
	p->cs_deniserev = -1;
	p->cs_mbdmac = 0;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = p->cs_cd32fmv = false;
	p->cs_cd32nvram_size = 1024;
	p->cs_cdtvcd = p->cs_cdtvram = false;
	p->cs_pcmcia = false;
	p->cs_ksmirror_e0 = true;
	p->cs_ksmirror_a8 = false;
	p->cs_ciaoverlay = true;
	p->cs_ciaatod = 0;
	p->cs_df0idhw = true;
	p->cs_resetwarning = true;
	p->cs_ciatodbug = false;
	p->cs_unmapped_space = 0;
	p->cs_color_burst = false;
	p->cs_hvcsync = false;
	p->cs_ciatype[0] = 0;
	p->cs_ciatype[1] = 0;
	p->cs_memorypatternfill = true;
	p->cs_ipldelay = false;
	p->cs_floppydatapullup = false;

	for (int i = 0; i < MAX_FILTERDATA; i++) {
		struct gfx_filterdata *f = &p->gf[i];
		f->gfx_filter = 0;
		f->gfx_filter_scanlineratio = (1 << 4) | 1;
		f->gfx_filter_scanlineoffset = 1;
		for (int j = 0; j <= 2 * MAX_FILTERSHADERS; j++) {
			f->gfx_filtershader[i][0] = 0;
			f->gfx_filtermask[i][0] = 0;
		}
		f->gfx_filter_horiz_zoom_mult = 1.0;
		f->gfx_filter_vert_zoom_mult = 1.0;
		f->gfx_filter_bilinear = 0;
		f->gfx_filter_filtermodeh = 0;
		f->gfx_filter_filtermodev = 0;
		f->gfx_filter_keep_aspect = 0;
// this one would cancel auto-centering if enabled
#ifndef AMIBERRY
		f->gfx_filter_autoscale = AUTOSCALE_STATIC_AUTO;
#endif
		f->gfx_filter_keep_autoscale_aspect = false;
		f->gfx_filteroverlay_overscan = 0;
		f->gfx_filter_left_border = -1;
		f->gfx_filter_top_border = -1;
		f->enable = true;
	}
	p->gf[2].enable = false;

	p->rtg_horiz_zoom_mult = 1.0;
	p->rtg_vert_zoom_mult = 1.0;

	_tcscpy (p->floppyslots[0].df, _T(""));
	_tcscpy (p->floppyslots[1].df, _T(""));
	_tcscpy (p->floppyslots[2].df, _T(""));
	_tcscpy (p->floppyslots[3].df, _T(""));
#ifdef WITH_LUA
	for (int i = 0; i < MAX_LUA_STATES; i++) {
		p->luafiles[i][0] = 0;
	}
#endif
	configure_rom (p, roms, 0);
	_tcscpy (p->romextfile, _T(""));
	_tcscpy (p->romextfile2, _T(""));
	p->romextfile2addr = 0;
	_tcscpy (p->flashfile, _T(""));
	_tcscpy (p->cartfile, _T(""));
	_tcscpy (p->rtcfile, _T(""));

	_tcscpy (p->path_rom.path[0], _T("./"));
	_tcscpy (p->path_floppy.path[0], _T("./"));
	_tcscpy (p->path_hardfile.path[0], _T("./"));

	p->prtname[0] = 0;
	p->sername[0] = 0;

	p->cpu_thread = false;

	p->fpu_model = 0;
	p->cpu_model = 68000;
	p->m68k_speed_throttle = 0;
	p->cpu_clock_multiplier = 0;
	p->cpu_frequency = 0;
	p->mmu_model = 0;
	p->cpu060_revision = 6;
	p->fpu_revision = 0;
	p->fpu_no_unimplemented = false;
	p->int_no_unimplemented = false;
	p->fpu_strict = false;
	p->fpu_mode = 0;
	p->m68k_speed = 0;
	p->cpu_compatible = true;
	p->address_space_24 = true;
	p->cpu_cycle_exact = false;
	p->cpu_memory_cycle_exact = false;
	p->blitter_cycle_exact = false;
	p->chipset_mask = 0;
	p->chipset_hr = false;
	p->genlock = false;
	p->genlock_image = 0;
	p->genlock_mix = 0;
	p->genlock_offset_x = 0;
	p->genlock_offset_y = 0;
	p->ntscmode = false;
	p->filesys_limit = 0;
	p->filesys_max_name = 107;
	p->filesys_max_file_size = 0x7fffffff;

	p->z3autoconfig_start = 0x10000000;
	p->chipmem.size = 0x00080000;
	p->chipmem.chipramtiming = true;
	p->bogomem.size = 0x00080000;
	p->bogomem.chipramtiming = true;
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		p->rtgboards[i].rtg_index = i;
	}
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

	p->statecapturebuffersize = 100;
	p->statecapturerate = 5 * 50;
	p->inprec_autoplay = true;
	p->statefile_path[0] = 0;

#ifdef UAE_MINI
	default_prefs_mini (p, 0);
#endif

	p->input_tablet = TABLET_OFF;
	p->tablet_library = false;
	p->input_mouse_untrap = MOUSEUNTRAP_MIDDLEBUTTON;
	p->input_magic_mouse_cursor = 0;

	inputdevice_default_prefs (p);

	blkdev_default_prefs (p);

	p->cr_selected = -1;
	struct chipset_refresh *cr;
	for (int i = 0; i < MAX_CHIPSET_REFRESH_TOTAL; i++) {
		cr = &p->cr[i];
		cr->index = i;
		cr->rate = -1;
	}
	cr = &p->cr[CHIPSET_REFRESH_PAL];
	cr->index = CHIPSET_REFRESH_PAL;
	cr->horiz = -1;
	cr->vert = -1;
	cr->lace = -1;
	cr->vsync = - 1;
	cr->framelength = -1;
	cr->rate = 50.0f;
	cr->ntsc = 0;
	cr->locked = false;
	cr->inuse = true;
	_tcscpy (cr->label, _T("PAL"));
	cr = &p->cr[CHIPSET_REFRESH_NTSC];
	cr->index = CHIPSET_REFRESH_NTSC;
	cr->horiz = -1;
	cr->vert = -1;
	cr->lace = -1;
	cr->vsync = - 1;
	cr->framelength = -1;
	cr->rate = 60.0f;
	cr->ntsc = 1;
	cr->locked = false;
	cr->inuse = true;
	_tcscpy (cr->label, _T("NTSC"));

	p->lightboost_strobo = false;
	p->lightboost_strobo_ratio = 50;

	savestate_state = 0;

	target_default_options (p, type);

	zfile_fclose (default_file);
	default_file = nullptr;
	struct zfile* f = zfile_fopen_empty (nullptr, _T("configstore"));
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
	p->address_space_24 = true;
	p->cpu_compatible = true;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE | CSMASK_AGA;
	p->chipmem.size = 0x200000;
	p->bogomem.size = 0;
	p->m68k_speed = -1;
}

static void buildin_default_host_prefs (struct uae_prefs *p)
{
#if 0
	p->sound_filter = FILTER_SOUND_OFF;
	p->sound_stereo = SND_STEREO;
	p->sound_stereo_separation = 7;
	p->sound_mixed_stereo = 0;
#endif
}

static void buildin_default_prefs (struct uae_prefs *p)
{
	//buildin_default_host_prefs (p); //no-op

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
	p->cpu_cycle_exact = false;
	p->cpu_memory_cycle_exact = false;
	p->blitter_cycle_exact = false;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->immediate_blits = false;
	p->waiting_blits = 0;
	p->collision_level = 2;
	if (p->produce_sound < 1)
		p->produce_sound = 1;
	p->scsi = 0;
	p->uaeserial = false;
	p->cpu_idle = 0;
	p->turbo_emulation = 0;
	p->turbo_emulation_limit = 0;
	p->catweasel = 0;
	p->tod_hack = false;
	p->maprom = 0;
	p->cachesize = 0;
	p->socket_emu = false;
	p->clipboard_sharing = false;
	p->ppc_mode = 0;
	p->ppc_model[0] = 0;
	p->cpuboard_type = 0;
	p->cpuboard_subtype = 0;
#ifdef AMIBERRY
	p->sound_volume_cd = 0;
#endif
	p->chipmem.size = 0x00080000;
	p->chipmem.chipramtiming = true;
	p->bogomem.size = 0x00080000;
	p->bogomem.chipramtiming = true;
	p->cpuboardmem1.size = 0;
	p->cpuboardmem2.size = 0;
	p->mbresmem_low.size = 0;
	p->mbresmem_high.size = 0;
	p->z3chipmem.size = 0;
	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		memset(p->fastmem, 0, sizeof(struct ramboard));
		memset(p->z3fastmem, 0, sizeof(struct ramboard));
	}
	for (auto& rtgboard : p->rtgboards) {
		rtgboard.rtgmem_size = 0x00000000;
		rtgboard.rtgmem_type = GFXBOARD_UAE_Z3;
	}
	for (auto& i : p->expansionboard) {
		memset(&i, 0, sizeof(struct boardromconfig));
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
	p->cs_resetwarning = false;
	p->cs_ciatodbug = false;
	p->cs_1mchipjumper = false;

	_tcscpy (p->romextfile, _T(""));
	_tcscpy (p->romextfile2, _T(""));

	p->genlock = false;
	p->genlock_image = 0;
	p->genlock_image_file[0] = 0;
	p->genlock_font[0] = 0;

	p->ne2000pciname[0] = 0;
	p->ne2000pcmcianame[0] = 0;
	p->a2065name[0] = 0;

	p->prtname[0] = 0;
	p->sername[0] = 0;

	p->mountitems = 0;

	p->debug_mem = false;

	target_default_options (p, 1);
	cfgfile_compatibility_romtype(p);
}

static void set_68020_compa (struct uae_prefs *p, int compa, int cd32)
{
	switch (compa)
	{
	case 0:
		p->m68k_speed = 0;
		if ((p->cpu_model == 68020 || p->cpu_model == 68030) && p->cachesize == 0) {
			p->cpu_cycle_exact = true;
			p->cpu_memory_cycle_exact = true;
			p->blitter_cycle_exact = true;
			if (p->cpu_model == 68020)
				p->cpu_clock_multiplier = 4 << 8;
			else
				p->cpu_clock_multiplier = 5 << 8;
		}
	break;
	case 1:
		p->m68k_speed = 0;
		if ((p->cpu_model == 68020 || p->cpu_model == 68030) && p->cachesize == 0) {
			p->blitter_cycle_exact = true;
			p->cpu_memory_cycle_exact = true;
			if (p->cpu_model == 68020)
				p->cpu_clock_multiplier = 4 << 8;
			else
				p->cpu_clock_multiplier = 5 << 8;
		}
	break;
	case 2:
		p->cpu_compatible = true;
		p->m68k_speed = 0;
		if (p->cpu_model == 68020)
			p->cpu_clock_multiplier = 4 << 8;
		else
			p->cpu_clock_multiplier = 5 << 8;
		break;
	case 3:
		p->cpu_compatible = false;
		p->m68k_speed = -1;
		p->address_space_24 = false;
		break;
	case 4:
		p->cpu_compatible = false;
		p->address_space_24 = false;
#ifdef JIT
		p->cachesize = MAX_JIT_CACHE;
#else
		p->cachesize = 0;
#endif
		break;
	default: break;
	}
	if (p->cpu_model >= 68030)
		p->address_space_24 = false;
}

/* 0: cycle-exact
* 1: more compatible
* 2: no more compatible, no 100% sound
* 3: no more compatible, waiting blits, no 100% sound
*/

static void set_68000_compa (struct uae_prefs *p, int compa)
{
	p->cpu_clock_multiplier = 2 << 8;
	switch (compa)
	{
	case 0:
		p->cpu_cycle_exact = p->cpu_memory_cycle_exact = p->blitter_cycle_exact = true;
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
	default: break;
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
	p->bogomem.size = 0;
	p->chipmem.size = 0x200000;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->fpu_no_unimplemented = true;
	if (compa == 0) {
		p->mmu_model = 68030;
	} else {
#ifdef JIT
		p->mmu_model = 0;
		p->cachesize = MAX_JIT_CACHE;
#else
		p->mmu_model = 0;
		p->cachesize = 0;
#endif
	}
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = false;
	p->produce_sound = 2;
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A3000;
	p->mbresmem_low.size = 8 * 1024 * 1024;
	built_in_chipset_prefs (p);
	p->cs_ciaatod = p->ntscmode ? 2 : 1;
	return configure_rom (p, roms, romcheck);
}
static int bip_a4000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[11];

	roms[0] = 16;
	roms[1] = 31;
	roms[2] = 13;
	roms[3] = 12;
	roms[4] = 46;
	roms[5] = 278;
	roms[6] = 283;
	roms[7] = 288;
	roms[8] = 293;
	roms[9] = 306;
	roms[10] = -1;

	p->bogomem.size = 0;
	p->chipmem.size = 0x200000;
	p->mbresmem_low.size = 8 * 1024 * 1024;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->mmu_model = 0;
	switch (config)
	{
		case 1:
		p->cpu_model = 68040;
		p->fpu_model = 68040;
		break;
		case 2:
		p->cpu_model = 68060;
		p->fpu_model = 68060;
#ifdef WITH_PPC
		p->ppc_mode = 1;
		cpuboard_setboard(p, BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_PPC);
		p->cpuboardmem1.size = 128 * 1024 * 1024;
		int roms_ppc[] = { 98, -1 };
		configure_rom(p, roms_ppc, romcheck);
#endif
		break;
	}
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = false;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = MAX_JIT_CACHE;
#else
	p->cachesize = 0;
#endif
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A4000;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
}
static int bip_a4000t (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[7];

	roms[0] = 17;
	roms[1] = 279;
	roms[2] = 284;
	roms[3] = 289;
	roms[4] = 294;
	roms[5] = 307;
	roms[6] = -1;

	p->bogomem.size = 0;
	p->chipmem.size = 0x200000;
	p->mbresmem_low.size = 8 * 1024 * 1024;
	p->cpu_model = 68030;
	p->fpu_model = 68882;
	p->mmu_model = 0;
	if (config > 0) {
		p->cpu_model = 68040;
		p->fpu_model = 68040;
	}
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = false;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = MAX_JIT_CACHE;
#else
	p->cachesize = 0;
#endif
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->cs_compatible = CP_A4000T;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
}

static void bip_velvet(struct uae_prefs *p, int config, int compa, int romcheck)
{
	p->mmu_model = 0;
	p->chipset_mask = 0;
	p->bogomem.size = 0;
	p->sound_filter = FILTER_SOUND_ON;
	set_68000_compa (p, compa);
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_compatible = CP_VELVET;
	p->bogomem.chipramtiming = false;
	p->chipmem.size = 0x40000;
	built_in_chipset_prefs (p);
	p->cs_agnusmodel = AGNUSMODEL_VELVET;
	p->cs_denisemodel = DENISEMODEL_VELVET;
	p->cs_cia6526 = true;
}

static int bip_a1000 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	roms[0] = 24;
	roms[1] = -1;
	p->mmu_model = 0;
	p->chipset_mask = 0;
	p->bogomem.size = 0;
	p->sound_filter = FILTER_SOUND_ON;
	set_68000_compa (p, compa);
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_compatible = CP_A1000;
	p->bogomem.chipramtiming = false;
	p->cs_agnusmodel = AGNUSMODEL_A1000;
	p->cs_denisemodel = DENISEMODEL_A1000;
	built_in_chipset_prefs (p);
	if (config > 0) {
		p->cs_denisemodel = DENISEMODEL_A1000NOEHB;
	}
	if (config > 1)
		p->chipmem.size = 0x40000;
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

	p->mmu_model = 0;
	p->bogomem.size = 0;
	p->chipmem.size = 0x100000;
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
	get_nvram_path(p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
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

	p->mmu_model = 0;
	p->bogomem.size = 0;
	p->chipmem.size = 0x100000;
	p->chipset_mask = CSMASK_ECS_AGNUS;
	p->cs_cdtvcd = p->cs_cdtvram = true;
	if (config > 0) {
		addbcromtype(p, ROMTYPE_CDTVSRAM, true, nullptr, 0);
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
	get_nvram_path(p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
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

static int bip_cd32 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[3];

	buildin_default_prefs_68020 (p);
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = true;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_unmapped_space = 1;
	p->mmu_model = 0;
	set_68020_compa (p, compa, 1);
	p->cs_compatible = CP_CD32;
	built_in_chipset_prefs (p);
	get_nvram_path(p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat (p->flashfile, _T("cd32.nvr"));
	roms[0] = 64;
	roms[1] = -1;
	if (!configure_rom (p, roms, 0)) {
		roms[0] = 18;
		roms[1] = -1;
		if (!configure_rom (p, roms, romcheck))
			return 0;
		roms[0] = 19;
		if (!configure_rom (p, roms, romcheck))
			return 0;
	}
#ifdef AMIBERRY
	// Amiberry offers one more config option: CD32 with 8MB Fast RAM
	switch (config)
	{
	case 1:
		p->cs_cd32fmv = true;
		roms[0] = 74;
		roms[1] = 23;
		roms[2] = -1;
		if (!configure_rom (p, roms, romcheck))
			return 0;
		break;
	case 2:
		addbcromtype(p, ROMTYPE_CUBO, true, nullptr, 0);
		get_nvram_path(p->flashfile, sizeof(p->flashfile) / sizeof(TCHAR));
		_tcscat(p->flashfile, _T("cd32cubo.nvr"));
		break;
	case 3:
		p->fastmem[0].size = 0x800000;
		p->cs_rtc = 1;
		break;
	default: break;
	}
#endif
	return 1;
}

static int bip_a1200 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[8];
	int roms_bliz[2];

	buildin_default_prefs_68020 (p);
	roms[0] = 11;
	roms[1] = 15;
	roms[2] = 276;
	roms[3] = 281;
	roms[4] = 286;
	roms[5] = 291;
	roms[6] = 304;
	roms[7] = -1;
	roms_bliz[0] = -1;
	roms_bliz[1] = -1;
	p->cs_rtc = 0;
	p->mmu_model = 0;
	p->cs_compatible = CP_A1200;
	built_in_chipset_prefs (p);
    switch (config)
    {
        case 1:
            p->fastmem[0].size = 0x400000;
            p->cs_rtc = 1;
            break;
#ifdef WITH_CPUBOARD
        case 2:
            cpuboard_setboard(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230IV);
            p->cpuboardmem1.size = 32 * 1024 * 1024;
            p->cpu_model = 68030;
            p->cs_rtc = 1;
            roms_bliz[0] = 89;
            configure_rom(p, roms_bliz, romcheck);
            break;
        case 3:
            cpuboard_setboard(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1260);
            p->cpuboardmem1.size = 32 * 1024 * 1024;
            p->cpu_model = 68040;
            p->fpu_model = 68040;
            p->cs_rtc = 1;
            roms_bliz[0] = 90;
            configure_rom(p, roms_bliz, romcheck);
            break;
        case 4:
            cpuboard_setboard(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1260);
            p->cpuboardmem1.size = 32 * 1024 * 1024;
            p->cpu_model = 68060;
            p->fpu_model = 68060;
            p->cs_rtc = 1;
            roms_bliz[0] = 90;
            configure_rom(p, roms_bliz, romcheck);
            break;
#ifdef WITH_PPC
        case 5:
            cpuboard_setboard(p, BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC);
            p->cpuboardmem1.size = 256 * 1024 * 1024;
            p->cpu_model = 68060;
            p->fpu_model = 68060;
            p->ppc_mode = 1;
            p->cs_rtc = 1;
            roms[0] = 15;
            roms[1] = 11;
            roms[2] = -1;
            roms_bliz[0] = 100;
            configure_rom(p, roms_bliz, romcheck);
            break;
#endif
#else
        case 2:
            p->fastmem[0].size = 0x800000;
            p->cs_rtc = 1;
            break;
#endif
	default: break;
    }
	set_68020_compa (p, compa, 0);
	return configure_rom (p, roms, romcheck);
}

static int bip_a600 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4];

	roms[0] = 10;
	roms[1] = 9;
	roms[2] = 8;
	roms[3] = -1;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A600;
	p->bogomem.size = 0;
	p->chipmem.size = 0x100000;
	p->mmu_model = 0;
	if (config > 0)
		p->cs_rtc = 1;
	switch (config)
	{
	case 1:
		p->chipmem.size = 0x200000;
		break;
	case 2:
		p->chipmem.size = 0x200000;
		p->fastmem[0].size = 0x400000;
		break;
	case 3:
		p->chipmem.size = 0x200000;
		p->fastmem[0].size = 0x800000;
		break;
	default: break;
	}
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	built_in_chipset_prefs(p);
	return configure_rom (p, roms, romcheck);
}

static int bip_a500p (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[2];

	roms[0] = 7;
	roms[1] = -1;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500P;
	p->bogomem.size = 0;
	p->chipmem.size = 0x100000;
	if (config > 0)
		p->cs_rtc = 1;
	if (config == 1)
		p->chipmem.size = 0x200000;
	if (config == 2)
		p->fastmem[0].size = 0x400000;
	p->chipset_mask = CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	built_in_chipset_prefs(p);
	return configure_rom (p, roms, romcheck);
}
static int bip_a500 (struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[3];

	roms[0] = roms[1] = roms[2] = -1;
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
		p->bogomem.size = 0;
		p->chipmem.size = 0x100000;
		break;
	case 3: // KS 1.3, OCS Agnus, 0.5M Chip
		roms[0] = 6;
		roms[1] = 32;
		p->bogomem.size = 0;
		p->chipset_mask = 0;
		p->cs_rtc = 0;
		p->floppyslots[1].dfxtype = DRV_NONE;
		break;
	case 4: // KS 1.2, OCS Agnus, 0.5M Chip
		roms[0] = 5;
		roms[1] = 4;
		p->bogomem.size = 0;
		p->chipset_mask = 0;
		p->cs_rtc = 0;
		p->floppyslots[1].dfxtype = DRV_NONE;
		break;
	case 5: // KS 1.2, OCS Agnus, 0.5M Chip + 0.5M Slow
		roms[0] = 5;
		roms[1] = 4;
		p->chipset_mask = 0;
		break;
	default: break;
	}
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs (p);
	return configure_rom (p, roms, romcheck);
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
	p->bogomem.size = 0;
	p->chipmem.size = 0x400000;
	p->z3fastmem[0].size = 8 * 1024 * 1024;
	p->rtgboards[0].rtgmem_size = 16 * 1024 * 1024;
	p->cpu_model = 68040;
	p->fpu_model = 68040;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = true;
	p->produce_sound = 2;
#ifdef JIT
	p->cachesize = MAX_JIT_CACHE;
#else
	p->cachesize = 0;
#endif
	p->floppyslots[0].dfxtype = DRV_35_HD;
	p->floppyslots[1].dfxtype = DRV_35_HD;
	p->floppy_speed = 0;
	p->cpu_idle = 150;
	p->scsi = 1;
	p->uaeserial = true;
	p->socket_emu = true;
	p->cart_internal = 0;
	p->picasso96_nocustom = true;
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

	p->bogomem.size = 0;
	p->chipset_mask = 0;
	p->cs_rtc = 0;
	p->nr_floppies = 0;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	set_68000_compa (p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs (p);
	get_nvram_path(p->flashfile, sizeof (p->flashfile) / sizeof (TCHAR));
	_tcscat(p->flashfile, _T("arcadia.nvr"));
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
	rl = getarcadiaroms(0);
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

static int bip_alg(struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[4], i;
	struct romlist** rl;

	p->bogomem.size = 0;
	p->chipset_mask = 0;
	p->cs_rtc = 0;
	p->nr_floppies = 0;
	p->genlock = true;
	p->genlock_image = 6;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->cs_floppydatapullup = true;
	set_68000_compa(p, compa);
	p->cs_compatible = CP_A500;
	built_in_chipset_prefs(p);
	get_nvram_path(p->flashfile, sizeof(p->flashfile) / sizeof(TCHAR));
	_tcscat(p->flashfile, _T("alg.nvr"));
	roms[0] = 5;
	roms[1] = 4;
	roms[2] = -1;
	if (!configure_rom(p, roms, romcheck))
		return 0;
	rl = getarcadiaroms(1);
	for (i = 0; rl[i]; i++) {
		if (config-- == 0) {
			roms[0] = rl[i]->rd->id;
			roms[1] = -1;
			configure_rom(p, roms, 0);
			const TCHAR *name = rl[i]->rd->name;
			p->ntscmode = true;
			if (_tcsstr(name, _T("Picmatic"))) {
				p->ntscmode = false;
			}
			break;
		}
	}
	xfree(rl);
	return 1;
}

static int bip_casablanca(struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[8];

	roms[0] = 231;
	roms[1] = -1;

	p->bogomem.size = 0;
	p->chipmem.size = 0x200000;
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
		p->mmu_model = 68060;
		break;
	}
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = false;
	p->produce_sound = 2;
	p->nr_floppies = 0;
	p->keyboard_mode = -1;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->floppyslots[2].dfxtype = DRV_PC_35_ONLY_80;
	p->cs_compatible = CP_CASABLANCA;
	built_in_chipset_prefs(p);
	return configure_rom(p, roms, romcheck);
}

static int bip_draco(struct uae_prefs *p, int config, int compa, int romcheck)
{
	int roms[8];

	p->bogomem.size = 0;
	p->chipmem.size = 0x200000;
	p->cpu_model = 68060;
	p->fpu_model = 68060;
	p->mmu_model = 68060;
	p->cpuboardmem1.size = 128 * 1024 * 1024;
	p->chipset_mask = CSMASK_AGA | CSMASK_ECS_AGNUS | CSMASK_ECS_DENISE;
	p->cpu_compatible = p->address_space_24 = false;
	p->m68k_speed = -1;
	p->immediate_blits = false;
	p->produce_sound = 2;
	p->nr_floppies = 0;
	p->keyboard_mode = -1;
	p->cpuboard_settings |= 0x10;
	p->floppyslots[0].dfxtype = DRV_NONE;
	p->floppyslots[1].dfxtype = DRV_NONE;
	p->floppyslots[2].dfxtype = DRV_PC_35_ONLY_80;
	p->cs_compatible = CP_DRACO;
	p->cpuboard_type = BOARD_MACROSYSTEM;
	p->cpuboard_subtype = BOARD_MACROSYSTEM_SUB_DRACO;
	p->rtgboards[0].rtgmem_type = GFXBOARD_ID_ALTAIS_Z3;
	p->rtgboards[0].rtgmem_size = 4 * 1024 * 1024;
	built_in_chipset_prefs(p);
	get_nvram_path(p->flashfile, sizeof(p->flashfile) / sizeof(TCHAR));
	_tcscat(p->flashfile, _T("draco.nvr"));
	roms[0] = 61;
	roms[1] = -1;
	if (!configure_rom(p, roms, romcheck)) {
		return 0;
	}
	roms[0] = 234;
	roms[1] = 311;
	roms[2] = -1;
	if (!configure_rom(p, roms, romcheck)) {
		return 0;
	}
	return 1;
}

static int bip_macrosystem(struct uae_prefs *p, int config, int compa, int romcheck)
{
	if (config == 0) {
		return bip_draco(p, config, compa, romcheck);
	} else {
		return bip_casablanca(p, config - 1, compa, romcheck);
	}
}

int built_in_prefs (struct uae_prefs *p, int model, int config, int compa, int romcheck)
{
	int v = 0;

	buildin_default_prefs (p);
	switch (model)
	{
	case 0:
		v = bip_a500 (p, config, compa, romcheck);
		break;
	case 1:
		v = bip_a500p (p, config, compa, romcheck);
		break;
	case 2:
		v = bip_a600 (p, config, compa, romcheck);
		break;
	case 3:
		v = bip_a1000 (p, config, compa, romcheck);
		break;
	case 4:
		v = bip_a1200 (p, config, compa, romcheck);
		break;
	case 5:
		v = bip_a3000 (p, config, compa, romcheck);
		break;
	case 6:
		v = bip_a4000 (p, config, compa, romcheck);
		break;
	case 7:
		v = bip_a4000t (p, config, compa, romcheck);
		break;
	case 8:
		v = bip_cd32 (p, config, compa, romcheck);
		break;
	case 9:
		v = bip_cdtv (p, config, compa, romcheck);
		break;
	case 10:
		v = bip_alg(p, config, compa, romcheck);
		break;
	case 11:
		v = bip_arcadia(p, config, compa, romcheck);
		break;
	case 12:
		v = bip_macrosystem(p, config, compa, romcheck);
		break;
	case 13:
		v = bip_super (p, config, compa, romcheck);
		break;
	default: break;
	}
	if ((p->cpu_model >= 68020 || !p->cpu_memory_cycle_exact) && !p->immediate_blits)
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

static bool has_expansion_with_rtc(struct uae_prefs* p, int chiplimit)
{
	if (p->bogomem.size ||
		p->chipmem.size > chiplimit ||
		p->cpuboard_type) {
		return true;
	}

	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		if (p->fastmem[i].size || p->z3fastmem[i].size) {
			return true;
		}
	}
	return false;
}

int built_in_chipset_prefs (struct uae_prefs *p)
{
	if (!p->cs_compatible)
		return 1;

	p->cs_a1000ram = false;
	p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = false;
	p->cs_cdtvcd = p->cs_cdtvram = p->cs_cdtvcr = false;
	p->cs_fatgaryrev = -1;
	p->cs_ide = 0;
	p->cs_ramseyrev = -1;
	p->cs_deniserev = -1;
	p->cs_agnusrev = -1;
	p->cs_agnusmodel = 0;
	p->cs_agnussize = 0;
	p->cs_denisemodel = 0;
	p->cs_bkpthang = false;
	p->cs_mbdmac = 0;
	p->cs_pcmcia = false;
	p->cs_ksmirror_e0 = true;
	p->cs_ksmirror_a8 = false;
	p->cs_ciaoverlay = true;
	p->cs_ciaatod = 0;
	p->cs_rtc = 0;
	p->cs_rtc_adjust_mode = p->cs_rtc_adjust = 0;
	p->cs_df0idhw = true;
	p->cs_resetwarning = true;
	p->cs_ciatodbug = false;
	p->cs_z3autoconfig = false;
	p->cs_bytecustomwritebug = false;
	p->cs_1mchipjumper = false;
	p->cs_unmapped_space = 0;
	p->cs_color_burst = false;
	p->cs_romisslow = false;
	p->cs_toshibagary = false;
	p->cs_eclocksync = 0;
	p->cs_ciatype[0] = p->cs_ciatype[1] = 0;

	switch (p->cs_compatible)
	{
	case CP_GENERIC: // generic
		if (p->cpu_model >= 68020) {
			// big box-like
			p->cs_rtc = 2;
			p->cs_fatgaryrev = 0;
			p->cs_ide = -1;
			p->cs_mbdmac = 0;
			p->cs_ramseyrev = 0x0f;
			p->cs_unmapped_space = 1;
		} else if (p->cpu_compatible) {
			// very A500-like
			p->cs_df0idhw = false;
			p->cs_resetwarning = false;
			if (has_expansion_with_rtc(p, 0x80000))
				p->cs_rtc = 1;
			p->cs_ciatodbug = true;
		} else {
			// sort of A500-like
			p->cs_ide = -1;
			p->cs_rtc = 1;
		}
		break;
	case CP_CDTV: // CDTV
		p->cs_rtc = 1;
		p->cs_cdtvcd = p->cs_cdtvram = true;
		p->cs_df0idhw = true;
		p->cs_ksmirror_e0 = false;
		break;
	case CP_CDTVCR: // CDTV-CR
		p->cs_rtc = 1;
		p->cs_cdtvcd = p->cs_cdtvram = true;
		p->cs_cdtvcr = true;
		p->cs_df0idhw = true;
		p->cs_ksmirror_e0 = false;
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = true;
		p->cs_ksmirror_a8 = true;
		p->cs_ciaoverlay = false;
		p->cs_resetwarning = false;
		p->cs_ciatodbug = true;
		break;
	case CP_CD32: // CD32
		p->cs_cd32c2p = p->cs_cd32cd = p->cs_cd32nvram = true;
		p->cs_ksmirror_e0 = false;
		p->cs_ksmirror_a8 = true;
		p->cs_ciaoverlay = false;
		p->cs_resetwarning = false;
		p->cs_unmapped_space = 1;
		p->cs_eclocksync = 2;
		if (has_expansion_with_rtc(p, 0x200000))
			p->cs_rtc = 1;
		break;
	case CP_A500: // A500
		p->cs_df0idhw = false;
		p->cs_resetwarning = false;
		if (has_expansion_with_rtc(p, 0x100000))
			p->cs_rtc = 1;
		p->cs_ciatodbug = true;
		break;
	case CP_A500P: // A500+
		p->cs_rtc = 1;
		p->cs_resetwarning = false;
		p->cs_ciatodbug = true;
		break;
	case CP_A600: // A600
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = true;
		p->cs_ksmirror_a8 = true;
		p->cs_ciaoverlay = false;
		p->cs_resetwarning = false;
		p->cs_ciatodbug = true;
		p->cs_ciatype[0] = p->cs_ciatype[1] = 1;
		p->cs_eclocksync = 2;
		if (has_expansion_with_rtc(p, 0x100000))
			p->cs_rtc = 1;
		break;
	case CP_A1000: // A1000
		p->cs_a1000ram = true;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_ksmirror_e0 = false;
		p->cs_agnusmodel = AGNUSMODEL_A1000;
		p->cs_denisemodel = DENISEMODEL_A1000;
		p->cs_ciatodbug = true;
		if (has_expansion_with_rtc(p, 0x80000))
			p->cs_rtc = 1;
		break;
	case CP_VELVET: // A1000 Prototype
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_ksmirror_e0 = false;
		p->cs_agnusmodel = AGNUSMODEL_A1000;
		p->cs_denisemodel = DENISEMODEL_A1000NOEHB;
		break;
	case CP_A1200: // A1200
		p->cs_ide = IDE_A600A1200;
		p->cs_pcmcia = true;
		p->cs_ksmirror_a8 = true;
		p->cs_ciaoverlay = false;
		p->cs_eclocksync = 2;
		if (has_expansion_with_rtc(p, 0x200000))
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
		p->cs_ksmirror_e0 = false;
		p->cs_ciaatod = p->ntscmode ? 2 : 1;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		break;
	case CP_A3000T: // A3000T
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0d;
		p->cs_mbdmac = 1;
		p->cs_ksmirror_e0 = false;
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
		p->cs_ksmirror_a8 = false;
		p->cs_ksmirror_e0 = false;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		p->cs_eclocksync = 2;
		break;
	case CP_A4000T: // A4000T
		p->cs_rtc = 2;
		p->cs_fatgaryrev = 0;
		p->cs_ramseyrev = 0x0f;
		p->cs_ide = IDE_A4000;
		p->cs_mbdmac = 2;
		p->cs_ksmirror_a8 = false;
		p->cs_ksmirror_e0 = false;
		p->cs_z3autoconfig = true;
		p->cs_unmapped_space = 1;
		p->cs_eclocksync = 2;
		break;
	case CP_CASABLANCA:
		break;
	case CP_DRACO:
		break;
	default: break;
	}
	if (p->cpu_model >= 68040)
		p->cs_bytecustomwritebug = true;
	return 1;
}

#ifdef _WIN32
#define SHADERPARM "string winuae_config : WINUAE_CONFIG ="

// Parse early because actual shader parsing happens after screen mode
// is already open and if shader config modifies screen parameters,
// it would cause annoying flickering.
void cfgfile_get_shader_config(struct uae_prefs *prefs, int rtg)
{
	TCHAR pluginpath[MAX_DPATH];
	if (!get_plugin_path(pluginpath, sizeof pluginpath / sizeof(TCHAR), _T("filtershaders\\direct3d")))
		return;
	for (int i = 0; i < 2 * MAX_FILTERSHADERS + 1; i++) {
		TCHAR tmp[MAX_DPATH];
		if (!prefs->gf[rtg].gfx_filtershader[i][0])
			continue;
		_tcscpy(tmp, pluginpath);
		_tcscat(tmp, prefs->gf[rtg].gfx_filtershader[i]);
		struct zfile *z = zfile_fopen(tmp, _T("r"));
		if (!z)
			continue;
		bool started = false;
		bool quoted = false;
		bool done = false;
		TCHAR *cmd = NULL;
		int len = 0;
		int totallen = 0;
		int linecnt = 15;
		while (!done) {
			char linep[MAX_DPATH], *line;
			if (!zfile_fgetsa(linep, MAX_DPATH, z))
				break;
			if (!started) {
				linecnt--;
				if (linecnt < 0)
					break;
			}
			line = linep + strspn(linep, "\t \r\n");
			trimwsa(line);
			char *p = line;
			if (p[0] == '/' && p[1] == '/')
				continue;
			if (p[0] == ';')
				continue;
			if (!started) {
				if (!strnicmp(line, SHADERPARM, strlen(SHADERPARM))) {
					started = true;
					p += strlen(SHADERPARM);
					totallen = 1000;
					cmd = xcalloc(TCHAR, totallen);
				}
			} else {
				while (!done && *p) {
					if (*p == '\"') {
						quoted = !quoted;
					} else if (!quoted && *p == ';') {
						done = true;
					} else if (quoted) {
						if (len + 2 >= totallen) {
							totallen += 1000;
							cmd = xrealloc(TCHAR, cmd, totallen);
						}
						cmd[len++] = *p;
					}
					p++;
				}
			}
		}
		if (cmd) {
			TCHAR *argc[UAELIB_MAX_PARSE];
			cmd[len] = 0;
			write_log(_T("Shader '%s' config '%s'\n"), tmp, cmd);
			int argv = cmdlineparser(cmd, argc, UAELIB_MAX_PARSE);
			if (argv > 0) {
				execcmdline(prefs, argv, argc, NULL, 0, true);
			}
			for (int i = 0; i < argv; i++) {
				xfree(argc[i]);
			}
		}
		xfree(cmd);
		zfile_fclose(z);
	}
}
#else
void cfgfile_get_shader_config(struct uae_prefs *p, int rtg)
{
}
#endif

void set_config_changed(int flags)
{
	if (!config_changed) {
		config_changed_flags = 0;
	}
	config_changed = 1;
	config_changed_flags |= flags;
}

void config_check_vsync ()
{
	if (config_changed) {
#if 0
		if (config_changed == 1) {
			createconfigstore (&currprefs);
			uae_lua_run_handler ("on_uae_config_changed");
		}
#endif
		config_changed++;
		if (config_changed >= 3) {
			config_changed = 0;
			config_changed_flags = 0;
		}
	}
}

bool is_error_log ()
{
	return error_lines != nullptr;
}
TCHAR *get_error_log ()
{
	strlist *sl;
	int len = 0;
	for (sl = error_lines; sl; sl = sl->next) {
		len += uaetcslen(sl->option) + 1;
	}
	if (!len)
		return nullptr;
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

	if (format == nullptr) {
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
	if (u) {
		u->option = my_strdup(bufp);
		if (u->option) {
			u->next = error_lines;
			error_lines = u;
		}
	}

	if (bufp != buffer)
		xfree (bufp);
}

#ifdef AMIBERRY
int bip_a4000(struct uae_prefs* p, int rom)
{
	return bip_a4000(p, 0, 0, 0);
}

int bip_cd32(struct uae_prefs* p, int rom)
{
	return bip_cd32(p, 0, 0, 0);
}

int bip_cdtv(struct uae_prefs* p, int rom)
{
	return bip_cdtv(p, 0, 0, 0);
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

int bip_a600(struct uae_prefs* p, int rom)
{
	return bip_a600(p, 0, 0, 0);
}

int bip_a1000(struct uae_prefs* p, int rom)
{
	return bip_a1000(p, 0, 0, 0);
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
	p->chipmem.size = 0x00080000;
	p->bogomem.size = 0x00080000;
	p->chipset_mask = 0;
	p->cpu_compatible = false;
	p->nr_floppies = 1;
	p->floppyslots[1].dfxtype = DRV_NONE;
	return configure_rom(p, roms, 0);
}

int bip_a3000(struct uae_prefs* p, int rom)
{
	return bip_a3000(p, 0, 0, 0);
}
#endif
