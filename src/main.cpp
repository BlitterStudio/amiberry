/*
* UAE - The Un*x Amiga Emulator
*
* Main program
*
* Copyright 1995 Ed Hanway
* Copyright 1995, 1996, 1997 Bernd Schmidt
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "disk.h"
#include "xwin.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "picasso96.h"
#include "native2amiga.h"
#include "savestate.h"
#include "filesys.h"
#include "blkdev.h"
#include "gfxboard.h"
#include "devices.h"
#include "jit/compemu.h"
#include <iostream>

#include <linux/kd.h>
#include <sys/ioctl.h>
#include "keyboard.h"

long int version = 256 * 65536L * UAEMAJOR + 65536L * UAEMINOR + UAESUBREV;

struct uae_prefs currprefs, changed_prefs;
int config_changed;

bool no_gui = false, quit_to_gui = false;
bool cloanto_rom = false;
bool kickstart_rom = true;

struct gui_info gui_data;

static TCHAR optionsfile[MAX_DPATH];

void my_trim(TCHAR* s)
{
	while (_tcslen(s) > 0 && _tcscspn(s, _T("\t \r\n")) == 0)
		memmove(s, s + 1, (_tcslen(s + 1) + 1) * sizeof(TCHAR));
	int len = _tcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

TCHAR* my_strdup_trim(const TCHAR* s)
{
	if (s[0] == 0)
		return my_strdup(s);
	while (_tcscspn(s, _T("\t \r\n")) == 0)
		s++;
	int len = _tcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		len--;
	auto* out = xmalloc(TCHAR, len + 1);
	memcpy(out, s, len * sizeof(TCHAR));
	out[len] = 0;
	return out;
}

void discard_prefs(struct uae_prefs* p, int type)
{
	auto* ps = &p->all_lines;
	while (*ps)
	{
		auto* s = *ps;
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
}

static void fixup_prefs_dim2(struct wh* wh)
{
	if (wh->width < 160)
	{
		error_log(_T("Width (%d) must be at least 160."), wh->width);
		wh->width = 160;
	}
	if (wh->height < 128)
	{
		error_log(_T("Height (%d) must be at least 128."), wh->height);
		wh->height = 128;
	}
	if (wh->width > 1920)
	{
		error_log(_T("Width (%d) max is %d."), wh->width, max_uae_width);
		wh->width = 1920;
	}
	if (wh->height > 1080)
	{
		error_log(_T("Height (%d) max is %d."), wh->height, max_uae_height);
		wh->height = 1080;
	}
}

void fixup_prefs_dimensions(struct uae_prefs* prefs)
{
	fixup_prefs_dim2(&prefs->gfx_monitor.gfx_size);
}

void fixup_cpu(struct uae_prefs* p)
{
	if (p->cpu_frequency == 1000000)
		p->cpu_frequency = 0;

	if (p->cpu_model >= 68040 && p->address_space_24)
	{
		error_log(_T("24-bit address space is not supported with 68040/060 configurations."));
		p->address_space_24 = false;
	}
	if (p->cpu_model < 68020 && p->fpu_model && (p->cpu_compatible || p->cpu_memory_cycle_exact))
	{
		error_log(_T("FPU is not supported with 68000/010 configurations."));
		p->fpu_model = 0;
	}

	switch (p->cpu_model)
	{
	case 68000:
		break;
	case 68010:
		break;
	case 68020:
		break;
	case 68030:
		break;
	case 68040:
		if (p->fpu_model)
			p->fpu_model = 68040;
		break;
	default:
		break;
	}

	if (p->cpu_thread && (p->cpu_compatible || p->ppc_mode || p->cpu_memory_cycle_exact || p->cpu_model < 68020))
	{
		p->cpu_thread = false;
		error_log(_T(
			"Threaded CPU mode is not compatible with PPC emulation, More compatible or Cycle Exact modes. CPU type must be 68020 or higher."));
	}

	if (p->cpu_model < 68020 && p->cachesize)
	{
		p->cachesize = 0;
		error_log(_T("JIT requires 68020 or better CPU."));
	}

	if (!p->cpu_memory_cycle_exact && p->cpu_cycle_exact)
		p->cpu_memory_cycle_exact = true;

	if (p->cpu_model >= 68020 && p->cachesize && p->cpu_compatible)
		p->cpu_compatible = false;

	if (p->cachesize && p->cpu_memory_cycle_exact)
	{
		error_log(_T("JIT and cycle-exact can't be enabled simultaneously."));
		p->cachesize = 0;
	}

	if (p->cachesize && (p->fpu_no_unimplemented || p->int_no_unimplemented))
	{
		error_log(_T("JIT is not compatible with unimplemented CPU/FPU instruction emulation."));
		p->fpu_no_unimplemented = p->int_no_unimplemented = false;
	}
	if (p->cachesize && p->compfpu && p->fpu_mode > 0)
	{
		error_log(_T("JIT FPU emulation is not compatible with softfloat FPU emulation."));
		p->fpu_mode = 0;
	}

	if (p->comptrustbyte < 0 || p->comptrustbyte > 3)
	{
		error_log(_T("Bad value for comptrustbyte parameter: value must be within 0..2."));
		p->comptrustbyte = 1;
	}
	if (p->comptrustword < 0 || p->comptrustword > 3)
	{
		error_log(_T("Bad value for comptrustword parameter: value must be within 0..2."));
		p->comptrustword = 1;
	}
	if (p->comptrustlong < 0 || p->comptrustlong > 3)
	{
		error_log(_T("Bad value for comptrustlong parameter: value must be within 0..2."));
		p->comptrustlong = 1;
	}
	if (p->comptrustnaddr < 0 || p->comptrustnaddr > 3)
	{
		error_log(_T("Bad value for comptrustnaddr parameter: value must be within 0..2."));
		p->comptrustnaddr = 1;
	}
	if (p->cachesize < 0 || p->cachesize > MAX_JIT_CACHE || (p->cachesize > 0 && p->cachesize < MIN_JIT_CACHE))
	{
		error_log(_T("JIT Bad value for cachesize parameter: value must zero or within %d..%d."), MIN_JIT_CACHE,
		          MAX_JIT_CACHE);
		p->cachesize = 0;
	}

	if (p->immediate_blits && p->waiting_blits)
	{
		error_log(_T("Immediate blitter and waiting blits can't be enabled simultaneously.\n"));
		p->waiting_blits = 0;
	}
	if (p->cpu_memory_cycle_exact)
		p->cpu_compatible = true;

	if (p->cpu_memory_cycle_exact && p->produce_sound == 0)
	{
		p->produce_sound = 1;
		error_log(_T("Cycle-exact mode requires at least Disabled but emulated sound setting."));
	}

	if (p->cpu_data_cache && (!p->cpu_compatible || p->cachesize || p->cpu_model < 68030))
	{
		p->cpu_data_cache = false;
		error_log(_T("Data cache emulation requires More compatible, is not JIT compatible, 68030+ only."));
	}
	if (p->cpu_data_cache && (p->uaeboard != 3 && need_uae_boot_rom(p)))
	{
		p->cpu_data_cache = false;
		error_log(_T("Data cache emulation requires Indirect UAE Boot ROM."));
	}
}

void fixup_prefs(struct uae_prefs* p, bool userconfig)
{
	built_in_chipset_prefs(p);
	fixup_cpu(p);
	cfgfile_compatibility_rtg(p);
	cfgfile_compatibility_romtype(p);

	read_kickstart_version(p);

	if (((p->chipmem_size & (p->chipmem_size - 1)) != 0 && p->chipmem_size != 0x180000)
		|| p->chipmem_size < 0x20000
		|| p->chipmem_size > 0x800000)
	{
		error_log(_T("Unsupported chipmem size %d (0x%x)."), p->chipmem_size, p->chipmem_size);
		p->chipmem_size = 0x200000;
		//err = 1;
	}

	for (auto& i : p->fastmem)
	{
		if ((i.size & (i.size - 1)) != 0
			|| (i.size != 0 && (i.size < 0x10000 || i.size > 0x800000)))
		{
			error_log(_T("Unsupported fastmem size %d (0x%x)."), i.size, i.size);
			i.size = 0;
			//err = 1;
		}
	}

	for (auto& rtgboard : p->rtgboards)
	{
		auto* const rbc = &rtgboard;
		if (rbc->rtgmem_size > max_z3fastmem && rbc->rtgmem_type == GFXBOARD_UAE_Z3)
		{
			error_log(
				_T("Graphics card memory size %d (0x%x) larger than maximum reserved %d (0x%x)."), rbc->rtgmem_size,
				rbc->rtgmem_size, 0x1000000, 0x1000000);
			rbc->rtgmem_size = 0x1000000;
			//err = 1;
		}

		if ((rbc->rtgmem_size & (rbc->rtgmem_size - 1)) != 0 || (rbc->rtgmem_size != 0 && (rbc->rtgmem_size < 0x100000))
		)
		{
			error_log(_T("Unsupported graphics card memory size %d (0x%x)."), rbc->rtgmem_size, rbc->rtgmem_size);
			if (rbc->rtgmem_size > max_z3fastmem)
				rbc->rtgmem_size = max_z3fastmem;
			else
				rbc->rtgmem_size = 0;
			//err = 1;
		}
	}

	for (auto& i : p->z3fastmem)
	{
		if ((i.size & (i.size - 1)) != 0 || (i.size != 0 && i.size < 0x100000))
		{
			error_log(_T("Unsupported Zorro III fastmem size %d (0x%x)."), i.size, i.size);
			i.size = 0;
			//err = 1;
		}
	}

	p->z3autoconfig_start &= ~0xffff;
	if (p->z3autoconfig_start != 0 && p->z3autoconfig_start < 0x1000000)
		p->z3autoconfig_start = 0x1000000;

	if (p->address_space_24 && (p->z3fastmem[0].size != 0))
	{
		p->z3fastmem[0].size = 0;
		error_log(_T("Can't use 32-bit memory when using a 24 bit address space."));
	}

	if (p->bogomem_size != 0 && p->bogomem_size != 0x80000 && p->bogomem_size != 0x100000 && p->bogomem_size != 0x180000
		&& p->bogomem_size != 0x1c0000)
	{
		error_log(_T("Unsupported bogomem size %d (0x%x)"), p->bogomem_size, p->bogomem_size);
		p->bogomem_size = 0;
		//err = 1;
	}

	if (p->bogomem_size > 0x180000 && (p->cs_fatgaryrev >= 0 || p->cs_ide || p->cs_ramseyrev >= 0))
	{
		p->bogomem_size = 0x180000;
		error_log(_T("Possible Gayle bogomem conflict fixed."));
	}
	if (p->chipmem_size > 0x200000 && p->fastmem[0].size > 262144)
	{
		error_log(_T("You can't use fastmem and more than 2MB chip at the same time."));
		p->chipmem_size = 0x200000;
		//err = 1;
	}
	if (p->mbresmem_low_size > 0x04000000 || (p->mbresmem_low_size & 0xfffff))
	{
		p->mbresmem_low_size = 0;
		error_log(_T("Unsupported Mainboard RAM size"));
	}
	if (p->mbresmem_high_size > 0x08000000 || (p->mbresmem_high_size & 0xfffff))
	{
		p->mbresmem_high_size = 0;
		error_log(_T("Unsupported CPU Board RAM size."));
	}

	for (auto& rtgboard : p->rtgboards)
	{
		auto* const rbc = &rtgboard;
		if (p->chipmem_size > 0x200000 && rbc->rtgmem_size && gfxboard_get_configtype(rbc) == 2)
		{
			error_log(_T("You can't use Zorro II RTG and more than 2MB chip at the same time."));
			p->chipmem_size = 0x200000;
			//err = 1;
		}
		if (p->address_space_24 && rbc->rtgmem_size && rbc->rtgmem_type == GFXBOARD_UAE_Z3)
		{
			error_log(_T("Z3 RTG and 24bit address space are not compatible."));
			rbc->rtgmem_type = GFXBOARD_UAE_Z2;
			rbc->rtgmem_size = 0;
		}
	}

	if (p->cs_z3autoconfig && p->address_space_24)
	{
		p->cs_z3autoconfig = false;
		error_log(_T("Z3 autoconfig and 24bit address space are not compatible."));
	}

	if (p->produce_sound < 0 || p->produce_sound > 3)
	{
		error_log(_T("Bad value for -S parameter: enable value must be within 0..3."));
		p->produce_sound = 0;
		//err = 1;
	}
	if (p->cachesize < 0 || p->cachesize > MAX_JIT_CACHE)
	{
		error_log(_T("Bad value for cachesize parameter: value must be within 0..16384."));
		p->cachesize = 0;
		//err = 1;
	}
	if ((p->z3fastmem[0].size) && p->address_space_24)
	{
		error_log(_T("Z3 fast memory can't be used if address space is 24-bit."));
		p->z3fastmem[0].size = 0;
		//err = 1;
	}
	for (auto& rtgboard : p->rtgboards)
	{
		if ((rtgboard.rtgmem_size > 0 && rtgboard.rtgmem_type == GFXBOARD_UAE_Z3) && p->address_space_24)
		{
			error_log(_T("UAEGFX Z3 RTG can't be used if address space is 24-bit."));
			rtgboard.rtgmem_size = 0;
			//err = 1;
		}
	}

#if !defined (BSDSOCKET)
	if (p->socket_emu) {
		write_log(_T("Compile-time option of BSDSOCKET_SUPPORTED was not enabled.  You can't use bsd-socket emulation.\n"));
		p->socket_emu = 0;
		err = 1;
	}
#endif

	if (p->nr_floppies < 0 || p->nr_floppies > 4)
	{
		error_log(_T("Invalid number of floppies.  Using 2."));
		p->nr_floppies = 2;
		p->floppyslots[0].dfxtype = 0;
		p->floppyslots[1].dfxtype = 0;
		p->floppyslots[2].dfxtype = -1;
		p->floppyslots[3].dfxtype = -1;
		//err = 1;
	}

	if (p->floppy_speed > 0 && p->floppy_speed < 10)
	{
		error_log(_T("Invalid floppy speed."));
		p->floppy_speed = 100;
	}
	if (p->collision_level < 0 || p->collision_level > 3)
	{
		error_log(_T("Invalid collision support level.  Using 1."));
		p->collision_level = 1;
		//err = 1;
	}
	if (p->cs_compatible == CP_GENERIC)
	{
		p->cs_fatgaryrev = p->cs_ramseyrev = -1;
		p->cs_ide = 0;
		if (p->cpu_model >= 68020)
		{
			p->cs_fatgaryrev = 0;
			p->cs_ide = -1;
			p->cs_ramseyrev = 0x0f;
		}
	}
	else if (p->cs_compatible == 0)
	{
		if (p->cs_ide == IDE_A4000)
		{
			if (p->cs_fatgaryrev < 0)
				p->cs_fatgaryrev = 0;
			if (p->cs_ramseyrev < 0)
				p->cs_ramseyrev = 0x0f;
		}
	}

	fixup_prefs_dimensions(p);

#if !defined (JIT)
	p->cachesize = 0;
#endif
#ifdef CPU_68000_ONLY
	p->cpu_model = 68000;
	p->fpu_model = 0;
#endif
#ifndef AGA
	p->chipset_mask &= ~CSMASK_AGA;
#endif
#ifndef AUTOCONFIG
	p->z3fastmem[0].size = 0;
	p->fastmem[0].size = 0;
	p->rtgboards[0].rtgmem_size = 0;
#endif
#if !defined (BSDSOCKET)
	p->socket_emu = 0;
#endif

	if (p->gfx_framerate < 1)
		p->gfx_framerate = 1;

	built_in_chipset_prefs(p);
	blkdev_fix_prefs(p);
	inputdevice_fix_prefs(p, userconfig);
	target_fixup_options(p);
	cfgfile_createconfigstore(p);
}

int quit_program = 0;
static int restart_program;
static TCHAR restart_config[MAX_DPATH];
static int default_config;

void uae_reset(int hardreset, int keyboardreset)
{
	if (quit_program == 0)
	{
		quit_program = -UAE_RESET;
		if (keyboardreset)
			quit_program = -UAE_RESET_KEYBOARD;
		if (hardreset)
			quit_program = -UAE_RESET_HARD;
	}
}

void uae_quit(void)
{
	if (quit_program != -UAE_QUIT)
	{
		quit_program = -UAE_QUIT;
	}
	target_quit();
}

/* 0 = normal, 1 = nogui, -1 = disable nogui */
void uae_restart(int opengui, const TCHAR* cfgfile)
{
	uae_quit();
	restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
	restart_config[0] = 0;
	default_config = 0;
	if (cfgfile)
		_tcscpy(restart_config, cfgfile);
	target_restart();
}

static void parse_cmdline_2(int argc, TCHAR** argv)
{
	cfgfile_addcfgparam(nullptr);
	for (auto i = 1; i < argc; i++)
	{
		if (_tcsncmp(argv[i], _T("-cfgparam="), 10) == 0)
		{
			cfgfile_addcfgparam(argv[i] + 10);
		}
		else if (_tcscmp(argv[i], _T("-cfgparam")) == 0)
		{
			if (i + 1 == argc)
				write_log(_T("Missing argument for '-cfgparam' option.\n"));
			else
				cfgfile_addcfgparam(argv[++i]);
		}
	}
}

static TCHAR* parse_text(const TCHAR* s)
{
	if (*s == '"' || *s == '\'')
	{
		const auto c = *s++;
		auto* const d = my_strdup(s);
		for (unsigned int i = 0; i < _tcslen(d); i++)
		{
			if (d[i] == c)
			{
				d[i] = 0;
				break;
			}
		}
		return d;
	}
	return my_strdup(s);
}

static TCHAR* parse_text_path(const TCHAR* s)
{
	auto* const s2 = parse_text(s);
	auto* const s3 = target_expand_environment(s2, nullptr, 0);
	xfree(s2);
	return s3;
}

void usage()
{
	std::cout << get_version_string() << std::endl;
	std::cout << "Usage:" << std::endl;
	std::cout << " -h                         Show this help." << std::endl;
	std::cout << " --help                     Show this help." << std::endl;
	std::cout << " -f <file>                  Load a configuration file." << std::endl;
	std::cout << " -config=<file>             Load a configuration file." << std::endl;
	std::cout << " -model=<Amiga Model>       Amiga model to emulate, from the QuickStart options." << std::endl;
	std::cout << "                            Available options are: A500, A500P, A1200, A4000 and CD32." << std::endl;
	std::cout << " -autoload=<file>           Load a WHDLoad game or .CUE CD32 image using the WHDBooter." << std::endl;
	std::cout << " -cdimage=<file>            Load the CD image provided when starting emulation (for CD32)." <<
		std::endl;
	std::cout << " -statefile=<file>          Load a save state file." << std::endl;
	std::cout << " -s <config param>=<value>  Set the configuration parameter with value." << std::endl;
	std::cout << "                            Edit a configuration file in order to know valid parameters and settings."
		<< std::endl;
	std::cout << "\nAdditional options:" << std::endl;
	std::cout << " -0 <filename>              Set adf for drive 0." << std::endl;
	std::cout << " -1 <filename>              Set adf for drive 1." << std::endl;
	std::cout << " -2 <filename>              Set adf for drive 2." << std::endl;
	std::cout << " -3 <filename>              Set adf for drive 3." << std::endl;
	std::cout << " -r <filename>              Set kickstart rom file." << std::endl;
	std::cout << " -G                         Start directly into emulation." << std::endl;
	std::cout << " -c <value>                 Size of chip memory (in number of 512 KBytes chunks)." << std::endl;
	std::cout << " -F <value>                 Size of fast memory (in number of 1024 KBytes chunks)." << std::endl;
	std::cout << "\nNote:" << std::endl;
	std::cout <<
		"Parameters are parsed from the beginning of command line, so in case of ambiguity for parameters, last one will be used."
		<< std::endl;
	std::cout << "File names should be with absolute path." << std::endl;
	std::cout << "\nExample:" << std::endl;
	std::cout << "amiberry -model=A1200 -G" << std::endl;
	std::cout << "It will use the A1200 default settings as found in the QuickStart panel." << std::endl;
	std::cout << "It will override use_gui to 'no' so that it enters emulation directly." << std::endl;
	std::cout << "\nExample 2:" << std::endl;
	std::cout << "amiberry -config=conf/A500.uae -statefile=savestates/game.uss -s use_gui=no" << std::endl;
	std::cout << "It will load A500.uae configuration with the save state named game." << std::endl;
	std::cout << "It will override use_gui to 'no' so that it enters emulation directly." << std::endl;
	exit(1);
}

std::string get_filename_extension(const TCHAR* filename)
{
	const std::string fName(filename);
	const auto pos = fName.rfind('.');
	if (pos == std::string::npos) // no extension
		return "";
	if (pos == 0) // . is at the front, not an extension
		return "";
	return fName.substr(pos, fName.length());
}

static void parse_cmdline(int argc, TCHAR** argv)
{
	static bool started;
	auto firstconfig = true;
	auto loaded = false;

	// only parse command line when starting for the first time
	if (started)
		return;
	started = true;

	for (auto i = 1; i < argc; i++)
	{
		if (_tcscmp(argv[i], _T("-cfgparam")) == 0)
		{
			if (i + 1 < argc)
				i++;
		}
		else if (_tcsncmp(argv[i], _T("-config="), 8) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 8);
			currprefs.mountitems = 0;
			target_cfgfile_load(&currprefs, txt,
			                    firstconfig
				                    ? CONFIG_TYPE_ALL
				                    : CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST | CONFIG_TYPE_NORESET, 0);
			xfree(txt);
			firstconfig = false;
			loaded = true;
		}
		else if (_tcsncmp(argv[i], _T("-model="), 7) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 7);
			if (_tcsncmp(txt, _T("A500"), 4) == 0)
			{
				bip_a500(&currprefs, -1);
			}
			else if (_tcsncmp(txt, _T("A500P"), 5) == 0)
			{
				bip_a500plus(&currprefs, -1);
			}
			else if (_tcsncmp(txt, _T("A1200"), 5) == 0)
			{
				bip_a1200(&currprefs, -1);
			}
			else if (_tcsncmp(txt, _T("A4000"), 5) == 0)
			{
				bip_a4000(&currprefs, -1);
			}
			else if (_tcsncmp(txt, _T("CD32"), 4) == 0)
			{
				bip_cd32(&currprefs, -1);
			}
		}
		else if (_tcsncmp(argv[i], _T("-statefile="), 11) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 11);
			savestate_state = STATE_DORESTORE;
			_tcscpy(savestate_fname, txt);
			xfree(txt);
			loaded = true;
		}
			// for backwards compatibility only - WHDLoading
		else if (_tcsncmp(argv[i], _T("-autowhdload="), 13) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 13);
			whdload_auto_prefs(&currprefs, txt);
			xfree(txt);
			firstconfig = false;
			loaded = true;
		}
			// for backwards compatibility only - CDLoading
		else if (_tcsncmp(argv[i], _T("-autocd="), 8) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 8);
			cd_auto_prefs(&currprefs, txt);
			xfree(txt);
			firstconfig = false;
			loaded = true;
		}
			// autoload ....  .cue / .lha  
		else if (_tcsncmp(argv[i], _T("-autoload="), 10) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 10);
			const auto txt2 = get_filename_extension(txt); // Extract the extension from the string  (incl '.')
			if (_tcsncmp(txt2.c_str(), ".lha", 4) == 0)
			{
				write_log("WHDLoad... %s\n", txt);
				whdload_auto_prefs(&currprefs, txt);
				xfree(txt);
			}
			else if (_tcsncmp(txt2.c_str(), ".cue", 4) == 0)
			{
				write_log("CDTV/CD32... %s\n", txt);
				cd_auto_prefs(&currprefs, txt);
				xfree(txt);
			}
			else
				write_log("Can't find extension ... %s\n", txt);
		}
		else if (_tcscmp(argv[i], _T("-f")) == 0)
		{
			/* Check for new-style "-f xxx" argument, where xxx is config-file */
			if (i + 1 == argc)
				write_log(_T("Missing argument for '-f' option.\n"));
			else
			{
				auto* const txt = parse_text_path(argv[++i]);
				currprefs.mountitems = 0;
				target_cfgfile_load(&currprefs, txt,
				                    firstconfig
					                    ? CONFIG_TYPE_ALL
					                    : CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST | CONFIG_TYPE_NORESET, 0);
				xfree(txt);
				firstconfig = false;
			}
			loaded = true;
		}
		else if (_tcscmp(argv[i], _T("-s")) == 0)
		{
			if (i + 1 == argc)
				write_log(_T("Missing argument for '-s' option.\n"));
			else
				cfgfile_parse_line(&currprefs, argv[++i], 0);
		}
		else if (_tcsncmp(argv[i], _T("-cdimage="), 9) == 0)
		{
			auto* const txt = parse_text_path(argv[i] + 9);
			auto* const txt2 = xmalloc(TCHAR, _tcslen(txt) + 5);
			_tcscpy(txt2, txt);
			if (_tcsrchr(txt2, ',') == nullptr)
				_tcscat(txt2, _T(",image"));
			cfgfile_parse_option(&currprefs, _T("cdimage0"), txt2, 0);
			xfree(txt2);
			xfree(txt);
			loaded = true;
		}
		else if (_tcscmp(argv[i], _T("-h")) == 0 || _tcsncmp(argv[i], _T("--help"), 6) == 0)
		{
			usage();
		}
		else if (argv[i][0] == '-' && argv[i][1] != '\0')
		{
			const TCHAR* arg = argv[i] + 2;
			const int extra_arg = *arg == '\0';
			if (extra_arg)
				arg = i + 1 < argc ? argv[i + 1] : nullptr;
			const auto ret = parse_cmdline_option(&currprefs, argv[i][1], arg);
			if (ret == -1)
				usage();
			if (ret && extra_arg)
				i++;
		}
		else if (i == argc - 1)
		{
			// if last config entry is an orphan and nothing else was loaded:
			// check if it is config file or statefile
			if (!loaded)
			{
				auto* const txt = parse_text_path(argv[i]);
				auto* const z = zfile_fopen(txt, _T("rb"), ZFD_NORMAL);
				if (z)
				{
					const auto type = zfile_gettype(z);
					zfile_fclose(z);
					if (type == ZFILE_CONFIGURATION)
					{
						currprefs.mountitems = 0;
						target_cfgfile_load(&currprefs, txt, CONFIG_TYPE_ALL, 0);
					}
					else if (type == ZFILE_STATEFILE)
					{
						savestate_state = STATE_DORESTORE;
						_tcscpy(savestate_fname, txt);
					}
				}
				xfree(txt);
			}
			else
			{
				printf("Unknown option %s\n", argv[i]);
				usage();
			}
		}
	}
}

static void parse_cmdline_and_init_file(int argc, TCHAR** argv)
{
	_tcscpy(optionsfile, _T(""));

	parse_cmdline_2(argc, argv);

	_tcscat(optionsfile, restart_config);

	if (!target_cfgfile_load(&currprefs, optionsfile, CONFIG_TYPE_DEFAULT, default_config))
	{
		write_log(_T("failed to load config '%s'\n"), optionsfile);
	}
	fixup_prefs(&currprefs, false);

	parse_cmdline(argc, argv);
}

/* Okay, this stuff looks strange, but it is here to encourage people who
* port UAE to re-use as much of this code as possible. Functions that you
* should be using are do_start_program() and do_leave_program(), as well
* as real_main(). Some OSes don't call main() (which is braindamaged IMHO,
* but unfortunately very common), so you need to call real_main() from
* whatever entry point you have. You may want to write your own versions
* of start_program() and leave_program() if you need to do anything special.
* Add #ifdefs around these as appropriate.
*/
static void do_start_program(void)
{
	if (quit_program == -UAE_QUIT)
		return;
	/* Do a reset on startup. Whether this is elegant is debatable. */
	inputdevice_updateconfig(&changed_prefs, &currprefs);
	if (quit_program >= 0)
		quit_program = UAE_RESET;
	m68k_go(1);
}

static void start_program(void)
{
	char kbd_flags;
	// set capslock state based upon current "real" state
	ioctl(0, KDGKBLED, &kbd_flags);
	if ((kbd_flags & 07) & LED_CAP)
	{
		// record capslock pressed
		inputdevice_do_keyboard(AK_CAPSLOCK, 1);
	}
	do_start_program();
}

static void leave_program(void)
{
	do_leave_program();
}

long get_file_size(const std::string& filename)
{
	struct stat stat_buf{};
	const auto rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? static_cast<long>(stat_buf.st_size) : -1;
}

bool download_file(const std::string& source, std::string destination)
{
	std::string download_command = "wget -np -nv -O ";
	const auto tmp = destination.append(".tmp");

	download_command.append(tmp);
	download_command.append(" ");
	download_command.append(source);
	download_command.append(" 2>&1");

	// Cleanup if the tmp destination already exists
	if (get_file_size(tmp) > 0)
	{
		if (std::remove(tmp.data()) < 0)
		{
			write_log(strerror(errno) + '\n');
		}
	}

	try
	{
		char buffer[1035];
		const auto output = popen(download_command.data(), "r");
		if (!output)
		{
			write_log("Failed while trying to run wget! Make sure it exists in your system...\n");
			return false;
		}

		while (fgets(buffer, sizeof buffer, output))
		{
			write_log(buffer);
		}
		pclose(output);
	}
	catch (...)
	{
		write_log("An exception was thrown while trying to execute wget!\n");
		return false;
	}

	if (get_file_size(tmp) > 0)
	{
		if (std::rename(tmp.data(), destination.data()) < 0)
		{
			write_log(strerror(errno) + '\n');
		}
		return true;
	}

	if (std::remove(tmp.data()) < 0)
	{
		write_log(strerror(errno) + '\n');
	}
	return false;
}

// In case of error, print the error code and close the application
void check_error_sdl(const bool check, const char* message)
{
	if (check)
	{
		std::cout << message << " " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(-1);
	}
}

static int real_main2(int argc, TCHAR** argv)
{
#ifdef USE_DISPMANX
	int ret = SDL_Init(SDL_INIT_TIMER
		| SDL_INIT_AUDIO
		| SDL_INIT_JOYSTICK
		| SDL_INIT_HAPTIC
		| SDL_INIT_GAMECONTROLLER
		| SDL_INIT_EVENTS) != 0;
#else
	const int ret = SDL_Init(SDL_INIT_EVERYTHING) != 0;
#endif
	if (ret < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		abort();
	}

	keyboard_settrans();
	set_config_changed();
	if (restart_config[0])
	{
		default_prefs(&currprefs, true, 0);
		fixup_prefs(&currprefs, true);
	}

	if (!graphics_setup())
	{
		abort();
	}

	if (restart_config[0])
	{
		parse_cmdline_and_init_file(argc, argv);
		config_loaded = true;
	}
	else
		copy_prefs(&changed_prefs, &currprefs);

	if (!machdep_init())
	{
		restart_program = 0;
		return -1;
	}

	if (!setup_sound())
	{
		write_log(_T("Sound driver unavailable: Sound output disabled\n"));
		currprefs.produce_sound = 0;
	}
	inputdevice_init();

	copy_prefs(&currprefs, &changed_prefs);

	no_gui = !currprefs.start_gui;
	if (restart_program == 2)
		no_gui = true;
	else if (restart_program == 3)
		no_gui = false;
	restart_program = 0;
	if (!no_gui)
	{
		auto err = gui_init();
		copy_prefs(&changed_prefs, &currprefs);
		set_config_changed();
		if (err == -1)
		{
			write_log(_T("Failed to initialize the GUI\n"));
			return -1;
		}
		if (err == -2)
		{
			return 1;
		}
	}
	else
	{
		update_display(&currprefs);
	}
	memset(&gui_data, 0, sizeof gui_data);
	gui_data.cd = -1;
	gui_data.hd = -1;
	gui_data.net = -1;
	gui_data.md = currprefs.cs_cd32nvram ? 0 : -1;

	if (!init_shm())
	{
		if (currprefs.start_gui)
			uae_restart(-1, nullptr);
		return 0;
	}

	fixup_prefs(&currprefs, true);
	copy_prefs(&currprefs, &changed_prefs);
	target_run();
	/* force sound settings change */
	currprefs.produce_sound = 0;

	keybuf_init(); /* Must come after init_joystick */

	memory_hardreset(2);
	memory_reset();

#ifdef AUTOCONFIG
	native2amiga_install();
#endif

	custom_init(); /* Must come after memory_init */
	DISK_init();

	reset_frame_rate_hack();
	init_m68k(); /* must come after reset_frame_rate_hack (); */

	gui_update();

	if (graphics_init(true))
	{
		if (!init_audio())
		{
			if (sound_available && currprefs.produce_sound > 1)
			{
				write_log(_T("Sound driver unavailable: Sound output disabled\n"));
			}
			currprefs.produce_sound = 0;
		}
		start_program();
	}
	return 0;
}

void real_main(int argc, TCHAR** argv)
{
	restart_program = 1;

	get_configuration_path(restart_config, sizeof restart_config / sizeof(TCHAR));
	_tcscat(restart_config, OPTIONSFILENAME);
	_tcscat(restart_config, ".uae");
	default_config = 1;

	while (restart_program)
	{
		copy_prefs(&currprefs, &changed_prefs);
		const auto ret = real_main2(argc, argv);
		if (ret == 0 && quit_to_gui)
			restart_program = 1;
		leave_program();
		quit_program = 0;
	}
	zfile_exit();
}

#ifndef NO_MAIN_IN_MAIN_C
int main(int argc, TCHAR **argv)
{
	real_main(argc, argv);
	return 0;
}
#endif
