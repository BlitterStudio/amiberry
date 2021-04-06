/*
* UAE - The Un*x Amiga Emulator
*
* Main program
*
* Copyright 1995 Ed Hanway
* Copyright 1995, 1996, 1997 Bernd Schmidt
*/
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "events.h"
#include "memory.h"
#include "custom.h"
#ifdef SERIAL_PORT
#include "serial.h"
#endif
#include "newcpu.h"
#include "disk.h"
//#include "debug.h"
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
#include "consolehook.h"
#include "gfxboard.h"
//#include "luascript.h"
//#include "uaenative.h"
#include "tabletlibrary.h"
//#include "cpuboard.h"
//#include "uae/ppc.h"
#include "devices.h"
#include "jit/compemu.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

#include <iostream>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include "keyboard.h"

static const char __ver[40] = "$VER: Amiberry 4.1.2 (2021-04-06)";
long int version = 256 * 65536L * UAEMAJOR + 65536L * UAEMINOR + UAESUBREV;

struct uae_prefs currprefs, changed_prefs;
int config_changed;

bool no_gui = false, quit_to_gui = false;
bool cloanto_rom = false;
bool kickstart_rom = true;
bool console_emulation = 0;

struct gui_info gui_data;

static TCHAR optionsfile[MAX_DPATH];

void my_trim (TCHAR *s)
{
	while (_tcslen (s) > 0 && _tcscspn (s, _T("\t \r\n")) == 0)
		memmove (s, s + 1, (_tcslen (s + 1) + 1) * sizeof (TCHAR));
	int len = _tcslen (s);
	while (len > 0 && _tcscspn (s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

TCHAR *my_strdup_trim (const TCHAR *s)
{
	if (s[0] == 0)
		return my_strdup(s);
	while (_tcscspn (s, _T("\t \r\n")) == 0)
		s++;
	int len = _tcslen (s);
	while (len > 0 && _tcscspn (s + len - 1, _T("\t \r\n")) == 0)
		len--;
	auto* out = xmalloc (TCHAR, len + 1);
	memcpy (out, s, len * sizeof (TCHAR));
	out[len] = 0;
	return out;
}

void discard_prefs(struct uae_prefs *p, int type)
{
	auto* ps = &p->all_lines;
	while (*ps) {
		auto* s = *ps;
		*ps = s->next;
		xfree (s->value);
		xfree (s->option);
		xfree (s);
	}
	p->all_lines = NULL;
	currprefs.all_lines = changed_prefs.all_lines = NULL;
#ifdef FILESYS
	filesys_cleanup ();
#endif
}

static void fixup_prefs_dim2(int monid, struct wh* wh)
{
	if (wh->special)
		return;
	if (wh->width < 160) {
		if (!monid)
			error_log(_T("Width (%d) must be at least 160."), wh->width);
		wh->width = 160;
	}
	if (wh->height < 128) {
		if (!monid)
			error_log(_T("Height (%d) must be at least 128."), wh->height);
		wh->height = 128;
	}
	if (wh->width > max_uae_width) {
		if (!monid)
			error_log(_T("Width (%d) max is %d."), wh->width, max_uae_width);
		wh->width = max_uae_width;
	}
	if (wh->height > max_uae_height) {
		if (!monid)
			error_log(_T("Height (%d) max is %d."), wh->height, max_uae_height);
		wh->height = max_uae_height;
	}
}

void fixup_prefs_dimensions (struct uae_prefs *prefs)
{
	for (int i = 0; i < MAX_AMIGADISPLAYS; i++) {
		fixup_prefs_dim2(i, &prefs->gfx_monitor[i].gfx_size_fs);
		fixup_prefs_dim2(i, &prefs->gfx_monitor[i].gfx_size_win);
	}
	
	if (prefs->gfx_apmode[1].gfx_vsync > 0)
		prefs->gfx_apmode[1].gfx_vsyncmode = 1;

	for (int i = 0; i < 2; i++) {
		struct apmode *ap = &prefs->gfx_apmode[i];
		if (ap->gfx_backbuffers < 1)
			ap->gfx_backbuffers = 1;
		ap->gfx_vflip = 0;
		ap->gfx_strobo = false;
		if (ap->gfx_vsync) {
			if (ap->gfx_vsyncmode) {
				if (ap->gfx_fullscreen != 0) {
					ap->gfx_backbuffers = 1;
					ap->gfx_strobo = prefs->lightboost_strobo;
				} else {
					ap->gfx_vsyncmode = 0;
					ap->gfx_vsync = 0;
				}
			} else {
				// legacy vsync: always wait for flip
				ap->gfx_vflip = -1;
				if (ap->gfx_vflip)
					ap->gfx_strobo = prefs->lightboost_strobo;
			}
		} else {
			if (ap->gfx_backbuffers > 0 && (prefs->gfx_api > 1 || prefs->gfx_variable_sync))
				ap->gfx_strobo = prefs->lightboost_strobo;
			// no vsync: wait if triple bufferirng
			if (ap->gfx_backbuffers >= 2)
				ap->gfx_vflip = -1;
		}
		if (prefs->gf[i].gfx_filter == 0 && ((prefs->gf[i].gfx_filter_autoscale && !prefs->gfx_api) || (prefs->gfx_apmode[APMODE_NATIVE].gfx_vsyncmode))) {
			prefs->gf[i].gfx_filter = 1;
		}
		if (i == 0) {
			if (prefs->gf[i].gfx_filter == 0 && prefs->monitoremu) {
				error_log(_T("Display port adapter emulation require at least null filter enabled."));
				prefs->gf[i].gfx_filter = 1;
			}
			if (prefs->gf[i].gfx_filter == 0 && prefs->cs_cd32fmv) {
				error_log(_T("CD32 MPEG module overlay support require at least null filter enabled."));
				prefs->gf[i].gfx_filter = 1;
			}
			if (prefs->gf[i].gfx_filter == 0 && (prefs->genlock && prefs->genlock_image)) {
				error_log(_T("Genlock emulation require at least null filter enabled."));
				prefs->gf[i].gfx_filter = 1;
			}
		}
	}
}

void fixup_cpu (struct uae_prefs *p)
{
	if (p->cpu_frequency == 1000000)
		p->cpu_frequency = 0;

	if (p->cpu_model >= 68040 && p->address_space_24) {
		error_log (_T("24-bit address space is not supported with 68040/060 configurations."));
		p->address_space_24 = false;
	}
	if (p->cpu_model < 68020 && p->fpu_model && (p->cpu_compatible || p->cpu_memory_cycle_exact)) {
		error_log (_T("FPU is not supported with 68000/010 configurations."));
		p->fpu_model = 0;
	}

	switch (p->cpu_model)
	{
	case 68000:
	case 68010:
	case 68020:
	case 68030:
		break;
	case 68040:
		if (p->fpu_model)
			p->fpu_model = 68040;
		break;
	default:
		break;
	}

	if (p->cpu_thread && (p->cpu_compatible || p->ppc_mode || p->cpu_memory_cycle_exact || p->cpu_model < 68020)) {
		p->cpu_thread = false;
		error_log(_T("Threaded CPU mode is not compatible with PPC emulation, More compatible or Cycle Exact modes. CPU type must be 68020 or higher."));
	}

	if (p->cpu_model < 68020 && p->cachesize) {
		p->cachesize = 0;
		error_log (_T("JIT requires 68020 or better CPU."));
	}

	if (!p->cpu_memory_cycle_exact && p->cpu_cycle_exact)
		p->cpu_memory_cycle_exact = true;

	if (p->cpu_model >= 68040 && p->cachesize && p->cpu_compatible)
		p->cpu_compatible = false;

	if ((p->cpu_model < 68030 || p->cachesize) && p->mmu_model) {
		error_log (_T("MMU emulation requires 68030/040/060 and it is not JIT compatible."));
		p->mmu_model = 0;
	}

	if (p->cachesize && p->cpu_memory_cycle_exact) {
		error_log (_T("JIT and cycle-exact can't be enabled simultaneously."));
		p->cachesize = 0;
	}
	if (p->cachesize && (p->fpu_no_unimplemented || p->int_no_unimplemented)) {
		error_log (_T("JIT is not compatible with unimplemented CPU/FPU instruction emulation."));
		p->fpu_no_unimplemented = p->int_no_unimplemented = false;
	}
	if (p->cachesize && p->compfpu && p->fpu_mode > 0) {
		error_log (_T("JIT FPU emulation is not compatible with softfloat FPU emulation."));
		p->fpu_mode = 0;
	}

	if (p->comptrustbyte < 0 || p->comptrustbyte > 3) {
		error_log(_T("Bad value for comptrustbyte parameter: value must be within 0..2."));
		p->comptrustbyte = 1;
	}
	if (p->comptrustword < 0 || p->comptrustword > 3) {
		error_log(_T("Bad value for comptrustword parameter: value must be within 0..2."));
		p->comptrustword = 1;
	}
	if (p->comptrustlong < 0 || p->comptrustlong > 3) {
		error_log(_T("Bad value for comptrustlong parameter: value must be within 0..2."));
		p->comptrustlong = 1;
	}
	if (p->comptrustnaddr < 0 || p->comptrustnaddr > 3) {
		error_log(_T("Bad value for comptrustnaddr parameter: value must be within 0..2."));
		p->comptrustnaddr = 1;
	}
	if (p->cachesize < 0 || p->cachesize > MAX_JIT_CACHE || (p->cachesize > 0 && p->cachesize < MIN_JIT_CACHE)) {
		error_log(_T("JIT Bad value for cachesize parameter: value must zero or within %d..%d."), MIN_JIT_CACHE, MAX_JIT_CACHE);
		p->cachesize = 0;
	}

	if (p->immediate_blits && p->waiting_blits) {
		error_log (_T("Immediate blitter and waiting blits can't be enabled simultaneously.\n"));
		p->waiting_blits = 0;
	}
	if (p->cpu_memory_cycle_exact)
		p->cpu_compatible = true;

	if (p->cpu_memory_cycle_exact && p->produce_sound == 0) {
		p->produce_sound = 1;
		error_log(_T("Cycle-exact mode requires at least Disabled but emulated sound setting."));
	}

	if (p->cpu_data_cache && (!p->cpu_compatible || p->cachesize || p->cpu_model < 68030)) {
		p->cpu_data_cache = false;
		error_log(_T("Data cache emulation requires More compatible, is not JIT compatible, 68030+ only."));
	}
	if (p->cpu_data_cache && (p->uaeboard != 3 && need_uae_boot_rom(p))) {
		p->cpu_data_cache = false;
		error_log(_T("Data cache emulation requires Indirect UAE Boot ROM."));
	}

	if (p->cpu_memory_cycle_exact && p->fast_copper != 0) {
		p->fast_copper = 0;
		error_log(_T("Cycle-exact mode not compatible with fast copper."));
	}
}

void fixup_prefs (struct uae_prefs *p, bool userconfig)
{
	built_in_chipset_prefs (p);
	fixup_cpu (p);
	cfgfile_compatibility_rtg(p);
	cfgfile_compatibility_romtype(p);

	read_kickstart_version(p);

	if (((p->chipmem.size & p->chipmem.size - 1) != 0 && p->chipmem.size != 0x180000)
		|| p->chipmem.size < 0x20000
		|| p->chipmem.size > 0x800000)
	{
		error_log (_T("Unsupported chipmem size %d (0x%x)."), p->chipmem.size, p->chipmem.size);
		p->chipmem.size = 0x200000;
	}

	for (auto& i : p->fastmem) {
		if ((i.size & i.size - 1) != 0
			|| (i.size != 0 && (i.size < 0x10000 || i.size > 0x800000)))
		{
			error_log (_T("Unsupported fastmem size %d (0x%x)."), i.size, i.size);
			i.size = 0;
		}
	}

	for (auto& rtgboard : p->rtgboards) {
		auto* const rbc = &rtgboard;
		if (rbc->rtgmem_size > max_z3fastmem && rbc->rtgmem_type == GFXBOARD_UAE_Z3)
		{
			error_log(
				_T("Graphics card memory size %d (0x%x) larger than maximum reserved %d (0x%x)."), rbc->rtgmem_size,
				rbc->rtgmem_size, 0x1000000, 0x1000000);
			rbc->rtgmem_size = 0x1000000;
		}

		if ((rbc->rtgmem_size & rbc->rtgmem_size - 1) != 0 || (rbc->rtgmem_size != 0 && rbc->rtgmem_size < 0x100000))
		{
			error_log(_T("Unsupported graphics card memory size %d (0x%x)."), rbc->rtgmem_size, rbc->rtgmem_size);
			if (rbc->rtgmem_size > max_z3fastmem)
				rbc->rtgmem_size = max_z3fastmem;
			else
				rbc->rtgmem_size = 0;
		}
	}

	for (auto& i : p->z3fastmem)
	{
		if ((i.size & i.size - 1) != 0 || (i.size != 0 && i.size < 0x100000))
		{
			error_log (_T("Unsupported Zorro III fastmem size %d (0x%x)."), i.size, i.size);
			i.size = 0;
		}
	}

	p->z3autoconfig_start &= ~0xffff;
	if (p->z3autoconfig_start != 0 && p->z3autoconfig_start < 0x1000000)
		p->z3autoconfig_start = 0x1000000;

	if (p->z3chipmem.size > max_z3fastmem) {
		error_log (_T("Zorro III fake chipmem size %d (0x%x) larger than max reserved %d (0x%x)."), p->z3chipmem.size, p->z3chipmem.size, max_z3fastmem, max_z3fastmem);
		p->z3chipmem.size = max_z3fastmem;
	}
	if (((p->z3chipmem.size & p->z3chipmem.size - 1) != 0 && p->z3chipmem.size != 0x18000000 && p->z3chipmem.size != 0x30000000) || (p->z3chipmem.size != 0 && p->z3chipmem.size < 0x100000))
	{
		error_log (_T("Unsupported 32-bit chipmem size %d (0x%x)."), p->z3chipmem.size, p->z3chipmem.size);
		p->z3chipmem.size = 0;
	}

	if (p->address_space_24 && (p->z3fastmem[0].size != 0 || p->z3fastmem[1].size != 0 || p->z3fastmem[2].size != 0 || p->z3fastmem[3].size != 0 || p->z3chipmem.size != 0)) {
		p->z3fastmem[0].size = p->z3fastmem[1].size = p->z3fastmem[2].size = p->z3fastmem[3].size = 0;
		p->z3chipmem.size = 0;
		error_log (_T("Can't use a Z3 graphics card or 32-bit memory when using a 24 bit address space."));
	}

	if (p->bogomem.size != 0 && p->bogomem.size != 0x80000 && p->bogomem.size != 0x100000 && p->bogomem.size != 0x180000 && p->bogomem.size != 0x1c0000) {
		error_log (_T("Unsupported bogomem size %d (0x%x)"), p->bogomem.size, p->bogomem.size);
		p->bogomem.size = 0;
	}

	if (p->bogomem.size > 0x180000 && (p->cs_fatgaryrev >= 0 || p->cs_ide || p->cs_ramseyrev >= 0)) {
		p->bogomem.size = 0x180000;
		error_log(_T("Possible Gayle bogomem conflict fixed."));
	}
	if (p->chipmem.size > 0x200000 && (p->fastmem[0].size > 262144 || p->fastmem[1].size > 262144)) {
		error_log(_T("You can't use fastmem and more than 2MB chip at the same time."));
		p->chipmem.size = 0x200000;
	}
	if (p->mem25bit.size > 128 * 1024 * 1024 || (p->mem25bit.size & 0xfffff)) {
		p->mem25bit.size = 0;
		error_log(_T("Unsupported 25bit RAM size"));
	}
	if (p->mbresmem_low.size > 0x04000000 || (p->mbresmem_low.size & 0xfffff)) {
		p->mbresmem_low.size = 0;
		error_log (_T("Unsupported Mainboard RAM size"));
	}
	if (p->mbresmem_high.size > 0x08000000 || (p->mbresmem_high.size & 0xfffff)) {
		p->mbresmem_high.size = 0;
		error_log (_T("Unsupported CPU Board RAM size."));
	}

	for (auto& rtgboard : p->rtgboards)
	{
		auto* const rbc = &rtgboard;
		if (p->chipmem.size > 0x200000 && rbc->rtgmem_size && gfxboard_get_configtype(rbc) == 2) {
			error_log(_T("You can't use Zorro II RTG and more than 2MB chip at the same time."));
			p->chipmem.size = 0x200000;
		}
		if (p->address_space_24 && rbc->rtgmem_size && rbc->rtgmem_type == GFXBOARD_UAE_Z3) {
			error_log (_T("Z3 RTG and 24bit address space are not compatible."));
			rbc->rtgmem_type = GFXBOARD_UAE_Z2;
			rbc->rtgmem_size = 0;
		}
	}

	if (p->cs_z3autoconfig && p->address_space_24) {
		p->cs_z3autoconfig = false;
		error_log (_T("Z3 autoconfig and 24bit address space are not compatible."));
	}

	if (p->produce_sound < 0 || p->produce_sound > 3) {
		error_log (_T("Bad value for -S parameter: enable value must be within 0..3."));
		p->produce_sound = 0;
	}

	if ((p->z3fastmem[0].size || p->z3fastmem[1].size || p->z3fastmem[2].size || p->z3fastmem[3].size || p->z3chipmem.size) && p->address_space_24) {
		error_log (_T("Z3 fast memory can't be used if address space is 24-bit."));
		p->z3fastmem[0].size = 0;
		p->z3fastmem[1].size = 0;
		p->z3fastmem[2].size = 0;
		p->z3fastmem[3].size = 0;
		p->z3chipmem.size = 0;
	}
	for (auto& rtgboard : p->rtgboards)
	{
		if (rtgboard.rtgmem_size > 0 && rtgboard.rtgmem_type == GFXBOARD_UAE_Z3 && p->address_space_24) {
			error_log (_T("UAEGFX Z3 RTG can't be used if address space is 24-bit."));
			rtgboard.rtgmem_size = 0;
		}
	}

#if !defined (BSDSOCKET)
	if (p->socket_emu) {
		write_log (_T("Compile-time option of BSDSOCKET_SUPPORTED was not enabled.  You can't use bsd-socket emulation.\n"));
		p->socket_emu = 0;
		err = 1;
	}
#endif
	if (p->socket_emu && p->uaeboard >= 3) {
		write_log(_T("bsdsocket.library is not compatible with indirect UAE Boot ROM.\n"));
		p->socket_emu = false;
	}

	if (p->nr_floppies < 0 || p->nr_floppies > 4) {
		error_log (_T("Invalid number of floppies.  Using 2."));
		p->nr_floppies = 2;
		p->floppyslots[0].dfxtype = 0;
		p->floppyslots[1].dfxtype = 0;
		p->floppyslots[2].dfxtype = -1;
		p->floppyslots[3].dfxtype = -1;
	}
	if (p->floppy_speed > 0 && p->floppy_speed < 10) {
		error_log (_T("Invalid floppy speed."));
		p->floppy_speed = 100;
	}
	if (p->input_mouse_speed < 1 || p->input_mouse_speed > 1000) {
		error_log (_T("Invalid mouse speed."));
		p->input_mouse_speed = 100;
	}
	if (p->collision_level < 0 || p->collision_level > 3) {
		error_log (_T("Invalid collision support level.  Using 1."));
		p->collision_level = 1;
	}
	if (p->parallel_postscript_emulation)
		p->parallel_postscript_detection = 1;
	if (p->cs_compatible == CP_GENERIC) {
		p->cs_fatgaryrev = p->cs_ramseyrev = p->cs_mbdmac = -1;
		p->cs_ide = 0;
		if (p->cpu_model >= 68020) {
			p->cs_fatgaryrev = 0;
			p->cs_ide = -1;
			p->cs_ramseyrev = 0x0f;
			p->cs_mbdmac = 0;
		}
	} else if (p->cs_compatible == 0) {
		if (p->cs_ide == IDE_A4000) {
			if (p->cs_fatgaryrev < 0)
				p->cs_fatgaryrev = 0;
			if (p->cs_ramseyrev < 0)
				p->cs_ramseyrev = 0x0f;
		}
	}
	if (p->chipmem.size >= 0x100000)
		p->cs_1mchipjumper = true;

	/* Can't fit genlock and A2024 or Graffiti at the same time,
	 * also Graffiti uses genlock audio bit as an enable signal
	 */
	if (p->genlock && p->monitoremu) {
		error_log (_T("Genlock and A2024 or Graffiti can't be active simultaneously."));
		p->genlock = false;
	}
	if (p->cs_hacks) {
		error_log (_T("chipset_hacks is nonzero (0x%04x)."), p->cs_hacks);
	}

	fixup_prefs_dimensions (p);

#if !defined (JIT)
	p->cachesize = 0;
#endif
#ifdef CPU_68000_ONLY
	p->cpu_model = 68000;
	p->fpu_model = 0;
#endif
#ifndef CPUEMU_0
	p->cpu_compatible = 1;
	p->address_space_24 = 1;
#endif
#if !defined (CPUEMU_11) && !defined (CPUEMU_13)
	p->cpu_compatible = 0;
	p->address_space_24 = 0;
#endif
#if !defined (CPUEMU_13)
	p->cpu_cycle_exact = p->blitter_cycle_exact = false;
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
#if !defined (SCSIEMU)
	p->scsi = 0;
#endif
#if !defined (SANA2)
	p->sana2 = 0;
#endif
#if !defined (UAESERIAL)
	p->uaeserial = false;
#endif
#if defined (CPUEMU_13)
	if (p->cpu_memory_cycle_exact) {
		if (p->gfx_framerate > 1) {
			error_log (_T("Cycle-exact requires disabled frameskip."));
			p->gfx_framerate = 1;
		}
		if (p->cachesize) {
			error_log (_T("Cycle-exact and JIT can't be active simultaneously."));
			p->cachesize = 0;
		}
	}
#endif
	if (p->gfx_framerate < 1)
		p->gfx_framerate = 1;
	if (p->gfx_display_sections < 1) {
		p->gfx_display_sections = 1;
	} else if (p->gfx_display_sections > 99) {
		p->gfx_display_sections = 99;
	}
	if (p->maprom && !p->address_space_24) {
		p->maprom = 0x0f000000;
	}
	if (((p->maprom & 0xff000000) && p->address_space_24) || (p->maprom && p->mbresmem_high.size >= 0x08000000)) {
		p->maprom = 0x00e00000;
	}
	if (p->maprom && p->cpuboard_type) {
		error_log(_T("UAE Maprom and accelerator board emulation are not compatible."));
		p->maprom = 0;
	}

	if (p->tod_hack && p->cs_ciaatod == 0)
		p->cs_ciaatod = p->ntscmode ? 2 : 1;

	// PCem does not support max speed.
	if (p->x86_speed_throttle < 0)
		p->x86_speed_throttle = 0;

	built_in_chipset_prefs (p);
	blkdev_fix_prefs (p);
	inputdevice_fix_prefs(p, userconfig);
	target_fixup_options (p);
	cfgfile_createconfigstore(p);
}

int quit_program = 0;
static int restart_program;
static TCHAR restart_config[MAX_DPATH];
static int default_config;

void uae_reset (int hardreset, int keyboardreset)
{
	//if (debug_dma) {
	//	record_dma_reset ();
	//	record_dma_reset ();
	//}
	currprefs.quitstatefile[0] = changed_prefs.quitstatefile[0] = 0;

	if (quit_program == 0) {
		quit_program = -UAE_RESET;
		if (keyboardreset)
			quit_program = -UAE_RESET_KEYBOARD;
		if (hardreset)
			quit_program = -UAE_RESET_HARD;
	}

}

void uae_quit (void)
{
	//deactivate_debugger ();
	if (quit_program != -UAE_QUIT)
		quit_program = -UAE_QUIT;
	target_quit ();
}

/* 0 = normal, 1 = nogui, -1 = disable nogui */
void uae_restart (int opengui, const TCHAR *cfgfile)
{
	uae_quit ();
	restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
	restart_config[0] = 0;
	default_config = 0;
	if (cfgfile)
		_tcscpy (restart_config, cfgfile);
	target_restart ();
}

void usage()
{
	std::cout << get_version_string() << std::endl;
	std::cout << "Usage:" << std::endl;
	std::cout << " -h                         Show this help." << std::endl;
	std::cout << " --help                     \n" << std::endl;
	std::cout << " -f <file>                  Load a configuration file." << std::endl;
	std::cout << " --config <file>            \n" << std::endl;
	std::cout << " -m <Amiga Model>           Amiga model to emulate, from the QuickStart options." << std::endl;
	std::cout << " --model <Amiga Model>      Available options are: A500, A500P, A1200, A4000 and CD32.\n" << std::endl;
	std::cout << " --autoload <file>          Load a WHDLoad game or .CUE CD32 image using the WHDBooter." << std::endl;
	std::cout << " --cdimage <file>           Load the CD image provided when starting emulation (for CD32)." << std::endl;
	std::cout << " --statefile <file>         Load a save state file." << std::endl;
	std::cout << " -s <config param>=<value>  Set the configuration parameter with value." << std::endl;
	std::cout << "                            Edit a configuration file in order to know valid parameters and settings." << std::endl;
	std::cout << "\nAdditional options:" << std::endl;
	std::cout << " -0 <filename>              Set adf for drive 0." << std::endl;
	std::cout << " -1 <filename>              Set adf for drive 1." << std::endl;
	std::cout << " -2 <filename>              Set adf for drive 2." << std::endl;
	std::cout << " -3 <filename>              Set adf for drive 3." << std::endl;
	std::cout << " -r <filename>              Set kickstart rom file." << std::endl;
	std::cout << " -G                         Start directly into emulation." << std::endl;
	std::cout << " -c <value>                 Size of chip memory (in number of 512 KBytes chunks)." << std::endl;
	std::cout << " -F <value>                 Size of fast memory (in number of 1024 KBytes chunks)." << std::endl;
	std::cout << "\nNotes:" << std::endl;
	std::cout << "File names should be with absolute path." << std::endl;
	std::cout << "\nExample 1:" << std::endl;
	std::cout << "amiberry --model A1200 -G" << std::endl;
	std::cout << "This will use the A1200 default settings as found in the QuickStart panel." << std::endl;
	std::cout << "Additionally, it will override 'use_gui' to 'no', so that it enters emulation directly." << std::endl;
	std::cout << "\nExample 2:" << std::endl;
	std::cout << "amiberry --config conf/A500.uae --statefile savestates/game.uss -s use_gui=no" << std::endl;
	std::cout << "This will load the conf/A500.uae configuration file, with the save state named game." << std::endl;
	std::cout << "It will override 'use_gui' to 'no', so that it enters emulation directly." << std::endl;
	exit(1);
}

static void parse_cmdline_2 (int argc, TCHAR **argv)
{
	cfgfile_addcfgparam (0);
	for (auto i = 1; i < argc; i++) {
		if (_tcsncmp(argv[i], _T("-cfgparam="), 10) == 0) {
			cfgfile_addcfgparam(argv[i] + 10);
		} else if (_tcscmp(argv[i], _T("-cfgparam")) == 0)
		{
			if (i + 1 == argc)
				write_log (_T("Missing argument for '-cfgparam' option.\n"));
			else
				cfgfile_addcfgparam (argv[++i]);
		}
	}
}

static int diskswapper_cb (struct zfile *f, void *vrsd)
{
	auto* num = static_cast<int*>(vrsd);
	if (*num >= MAX_SPARE_DRIVES)
		return 1;
	if (zfile_gettype (f) ==  ZFILE_DISKIMAGE) {
		_tcsncpy (currprefs.dfxlist[*num], zfile_getname (f), 255);
		(*num)++;
	}
	return 0;
}

static void parse_diskswapper (const TCHAR *s)
{
	auto* const tmp = my_strdup (s);
	const auto* delim = _T(",");
	TCHAR* p2;
	auto num = 0;

	auto* p1 = tmp;
	for (;;) {
		p2 = _tcstok(p1, delim);
		if (!p2)
			break;
		p1 = NULL;
		if (num >= MAX_SPARE_DRIVES)
			break;
		if (!zfile_zopen (p2, diskswapper_cb, &num)) {
			_tcsncpy (currprefs.dfxlist[num], p2, 255);
			num++;
		}
	}
	xfree (tmp);
}

static TCHAR *parsetext (const TCHAR *s)
{
	if (*s == '"' || *s == '\'') {
		const auto c = *s++;
		auto* const d = my_strdup (s);
		for (unsigned int i = 0; i < _tcslen (d); i++) {
			if (d[i] == c) {
				d[i] = 0;
				break;
			}
		}
		return d;
	}
	return my_strdup(s);
}
static TCHAR *parsetextpath (const TCHAR *s)
{
	auto* const s2 = parsetext (s);
	auto* const s3 = target_expand_environment (s2, NULL, 0);
	xfree(s2);
	return s3;
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

static void parse_cmdline (int argc, TCHAR **argv)
{
	static bool started;
	auto firstconfig = true;
	auto loaded = false;

	// only parse command line when starting for the first time
	if (started)
		return;
	started = true;

	for (auto i = 1; i < argc; i++) {
		if (!_tcsncmp(argv[i], _T("-diskswapper="), 13)) {
			auto* txt = parsetextpath (argv[i] + 13);
			parse_diskswapper (txt);
			xfree(txt);
		} else if (_tcsncmp (argv[i], _T("-cfgparam="), 10) == 0) {
			;
		} else if (_tcscmp (argv[i], _T("-cfgparam")) == 0) {
			if (i + 1 < argc)
				i++;
		} else if (_tcscmp(argv[i], _T("--config")) == 0 || _tcscmp(argv[i], _T("-f")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--config' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
				currprefs.mountitems = 0;
				target_cfgfile_load(&currprefs, txt,
					firstconfig
					? CONFIG_TYPE_ALL
					: CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST | CONFIG_TYPE_NORESET, 0);
				xfree(txt);
				firstconfig = false;
			}
			loaded = true;
		} else if (_tcscmp(argv[i], _T("--model")) == 0 || _tcscmp(argv[i], _T("-m")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--model' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
				if (_tcscmp(txt, _T("A500")) == 0)
				{
					bip_a500(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("A500P")) == 0)
				{
					bip_a500plus(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("A1200")) == 0)
				{
					bip_a1200(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("A4000")) == 0)
				{
					bip_a4000(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("CD32")) == 0)
				{
					bip_cd32(&currprefs, -1);
				}
			}
		} else if (_tcscmp(argv[i], _T("--statefile")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--statefile' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
				savestate_state = STATE_DORESTORE;
				_tcscpy(savestate_fname, txt);
				xfree(txt);
			}
			loaded = true;
		}
		// Auto-load .cue / .lha  
		else if (_tcscmp(argv[i], _T("--autoload")) == 0)
		{
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--autoload' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
				const auto txt2 = get_filename_extension(txt); // Extract the extension from the string  (incl '.')
				if (_tcscmp(txt2.c_str(), ".lha") == 0)
				{
					write_log("WHDLoad... %s\n", txt);
					whdload_auto_prefs(&currprefs, txt);
					xfree(txt);
				}
				else if (_tcscmp(txt2.c_str(), ".cue") == 0)
				{
					write_log("CDTV/CD32... %s\n", txt);
					cd_auto_prefs(&currprefs, txt);
					xfree(txt);
				}
				else
					write_log("Can't find extension ... %s\n", txt);
			}
		}
		else if (_tcscmp(argv[i], _T("--cli")) == 0)
			console_emulation = true;
		else if (_tcscmp(argv[i], _T("-s")) == 0)
		{
			if (i + 1 == argc)
				write_log(_T("Missing argument for '-s' option.\n"));
			else
				cfgfile_parse_line(&currprefs, argv[++i], 0);
		} else if (_tcscmp(argv[i], _T("--cdimage")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--cdimage' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
				auto* const txt2 = xmalloc(TCHAR, _tcslen(txt) + 5);
				_tcscpy(txt2, txt);
				if (_tcsrchr(txt2, ',') == nullptr)
					_tcscat(txt2, _T(",image"));
				cfgfile_parse_option(&currprefs, _T("cdimage0"), txt2, 0);
				xfree(txt2);
				xfree(txt);
			}
			loaded = true;
		} else if (_tcscmp(argv[i], _T("-h")) == 0 || _tcscmp(argv[i], _T("--help")) == 0) {
			usage();
		} else if (argv[i][0] == '-' && argv[i][1] != '\0') {
			const TCHAR* arg = argv[i] + 2;
			const int extra_arg = *arg == '\0';
			if (extra_arg)
				arg = i + 1 < argc ? argv[i + 1] : nullptr;
			const auto ret = parse_cmdline_option(&currprefs, argv[i][1], arg);
			if (ret == -1)
				usage();
			if (ret && extra_arg)
				i++;
		} else if (i == argc - 1) {
			// if last config entry is an orphan and nothing else was loaded:
			// check if it is config file or statefile
			if (!loaded) {
				auto* const txt = parsetextpath(argv[i]);
				auto* const z = zfile_fopen(txt, _T("rb"), ZFD_NORMAL);
				if (z) {
					const auto type = zfile_gettype(z);
					zfile_fclose(z);
					if (type == ZFILE_CONFIGURATION) {
						currprefs.mountitems = 0;
						target_cfgfile_load(&currprefs, txt, CONFIG_TYPE_ALL, 0);
					} else if (type == ZFILE_STATEFILE) {
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

static void parse_cmdline_and_init_file (int argc, TCHAR **argv)
{
	_tcscpy (optionsfile, _T(""));

	parse_cmdline_2 (argc, argv);

	_tcscat (optionsfile, restart_config);

	if (! target_cfgfile_load (&currprefs, optionsfile, CONFIG_TYPE_DEFAULT, default_config)) {
		write_log (_T("failed to load config '%s'\n"), optionsfile);
	}
	else
	{
		config_loaded = true;
	}
	fixup_prefs (&currprefs, false);

	parse_cmdline (argc, argv);
}

/* Okay, this stuff looks strange, but it is here to encourage people who
* port UAE to re-use as much of this code as possible. Functions that you
* should be using are do_start_program () and do_leave_program (), as well
* as real_main (). Some OSes don't call main () (which is braindamaged IMHO,
* but unfortunately very common), so you need to call real_main () from
* whatever entry point you have. You may want to write your own versions
* of start_program () and leave_program () if you need to do anything special.
* Add #ifdefs around these as appropriate.
*/
static void do_start_program (void)
{
	if (quit_program == -UAE_QUIT)
		return;
	/* Do a reset on startup. Whether this is elegant is debatable. */
	inputdevice_updateconfig (&changed_prefs, &currprefs);
	if (quit_program >= 0)
		quit_program = UAE_RESET;
#ifdef WITH_LUA
	uae_lua_loadall ();
#endif
	try
	{
		emulating = 1;
		m68k_go (1);
	}
	catch (...)
	{
		write_log("An exception was thrown while running m68k_go!\n");
	}
}

static void start_program (void)
{
	do_start_program ();
}

static void leave_program (void)
{
	do_leave_program ();
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
	auto tmp = destination;
	tmp = tmp.append(".tmp");

	download_command.append(tmp);
	download_command.append(" ");
	download_command.append(source);
	download_command.append(" 2>&1");

	// Cleanup if the tmp destination already exists
	if (get_file_size(tmp) > 0)
	{
		if (std::remove(tmp.c_str()) < 0)
		{
			write_log(strerror(errno) + '\n');
		}
	}

	try
	{
		char buffer[1035];
		const auto output = popen(download_command.c_str(), "r");
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
		if (std::rename(tmp.c_str(), destination.c_str()) < 0)
		{
			write_log(strerror(errno) + '\n');
		}
		return true;
	}

	if (std::remove(tmp.c_str()) < 0)
	{
		write_log(strerror(errno) + '\n');
	}
	return false;
}

void download_rtb(std::string filename)
{
	char destination[MAX_DPATH];
	char url[MAX_DPATH];
	
	snprintf(destination, MAX_DPATH, "%s/whdboot/save-data/Kickstarts/%s", start_path_data, filename.c_str());
	if (get_file_size(destination) <= 0)
	{
		write_log("Downloading %s ...\n", destination);
		snprintf(url, MAX_DPATH, "https://github.com/midwan/amiberry/blob/master/whdboot/save-data/Kickstarts/%s?raw=true", filename.c_str());
		download_file(url,  destination);
	}
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

static int real_main2 (int argc, TCHAR **argv)
{
	keyboard_settrans();
	set_config_changed();
	if (restart_config[0]) {
		default_prefs (&currprefs, true, 0);
		fixup_prefs (&currprefs, true);
	}

#ifdef NATMEM_OFFSET
	//preinit_shm ();
#endif

	if (restart_config[0]) {
		parse_cmdline_and_init_file(argc, argv);
	}
	else
		copy_prefs(&changed_prefs, &currprefs);

	if (!graphics_setup()) {
		abort();
	}

	if (!machdep_init()) {
		restart_program = 0;
		return -1;
	}

	if (console_emulation) {
		consolehook_config (&currprefs);
		fixup_prefs (&currprefs, true);
	}

	if (! setup_sound ()) {
		write_log (_T("Sound driver unavailable: Sound output disabled\n"));
		currprefs.produce_sound = 0;
	}
	inputdevice_init ();

	copy_prefs(&currprefs, &changed_prefs);

	no_gui = ! currprefs.start_gui;
	if (restart_program == 2)
		no_gui = true;
	else if (restart_program == 3)
		no_gui = false;
	restart_program = 0;
	if (! no_gui) {
		const auto err = gui_init ();
		copy_prefs(&changed_prefs, &currprefs);
		set_config_changed ();
		if (err == -1) {
			write_log (_T("Failed to initialize the GUI\n"));
			return -1;
		} else if (err == -2) {
			return 1;
		}
	}

	memset (&gui_data, 0, sizeof gui_data);
	gui_data.cd = -1;
	gui_data.hd = -1;
	gui_data.net = -1;
	gui_data.md = (currprefs.cs_cd32nvram || currprefs.cs_cdtvram) ? 0 : -1;

#ifdef NATMEM_OFFSET
	if (!init_shm ()) {
		if (currprefs.start_gui)
			uae_restart(-1, NULL);
		return 0;
	}
#endif
#ifdef WITH_LUA
	uae_lua_init ();
#endif

	fixup_prefs (&currprefs, true);
#ifdef RETROPLATFORM
	rp_fixup_options (&currprefs);
#endif
	copy_prefs(&currprefs, &changed_prefs);
	target_run ();
	/* force sound settings change */
	currprefs.produce_sound = 0;

	//savestate_init ();
	keybuf_init (); /* Must come after init_joystick */

	memory_hardreset (2);
	memory_reset ();

#ifdef AUTOCONFIG
	native2amiga_install ();
#endif
	custom_init (); /* Must come after memory_init */
#ifdef SERIAL_PORT
	serial_init ();
#endif
	DISK_init ();
#ifdef WITH_PPC
	uae_ppc_reset(true);
#endif

	reset_frame_rate_hack ();
	init_m68k (); /* must come after reset_frame_rate_hack (); */

	gui_update ();

	if (graphics_init (true)) {

		if (!init_audio ()) {
			if (sound_available && currprefs.produce_sound > 1) {
				write_log (_T("Sound driver unavailable: Sound output disabled\n"));
			}
			currprefs.produce_sound = 0;
		}
		start_program ();
	}
	return 0;
}

void real_main (int argc, TCHAR **argv)
{
	restart_program = 1;

	get_configuration_path (restart_config, sizeof restart_config / sizeof (TCHAR));
	_tcscat (restart_config, OPTIONSFILENAME);
	_tcscat(restart_config, ".uae");
	default_config = 1;

	while (restart_program) {
		copy_prefs(&currprefs, &changed_prefs);
		const auto ret = real_main2 (argc, argv);
		if (ret == 0 && quit_to_gui)
			restart_program = 1;
		leave_program ();
		quit_program = 0;
	}
	zfile_exit ();
}

#ifndef NO_MAIN_IN_MAIN_C
int main (int argc, TCHAR **argv)
{
	real_main (argc, argv);
	return 0;
}
#endif
