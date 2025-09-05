/*
* UAE - The Un*x Amiga Emulator
*
* Main program
*
* Copyright 1995 Ed Hanway
* Copyright 1995, 1996, 1997 Bernd Schmidt
*/
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef USE_OLDGCC
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

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
#include "debug.h"
#include "xwin.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "native2amiga.h"
#include "savestate.h"
#include "blkdev.h"
#include "consolehook.h"
#include "gfxboard.h"
#ifdef WITH_LUA
#include "luascript.h"
#endif
#include "cpuboard.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "devices.h"
#ifdef JIT
#include "jit/compemu.h"
#endif
#include "disasm.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

#include <iostream>
#if defined(__linux__)
#include <linux/kd.h>
#endif
#include <sys/ioctl.h>

#include "fsdb.h"
#include "fsdb_host.h"
#include "keyboard.h"

// Special version string so that AmigaOS can detect it
static constexpr char __ver[40] = "$VER: Amiberry v8.0.0 (2025-09-05)";

long int version = 256 * 65536L * UAEMAJOR + 65536L * UAEMINOR + UAESUBREV;

extern int console_logging;
struct uae_prefs currprefs, changed_prefs;
int config_changed, config_changed_flags;

bool no_gui = false, quit_to_gui = false;
bool cloanto_rom = false;
bool kickstart_rom = true;
bool console_emulation = 0;

struct gui_info gui_data;

static TCHAR optionsfile[MAX_DPATH];

static uae_u32 randseed;

static uae_u32 xorshiftstate;
static uae_u32 xorshift32()
{
	uae_u32 x = xorshiftstate;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshiftstate = x;
	return xorshiftstate;
}

uae_u32 uaerand()
{
	if (xorshiftstate == 0) {
		xorshiftstate = randseed;
		if (!xorshiftstate) {
			randseed = 1;
			xorshiftstate = 1;
		}
	}
	uae_u32 r = xorshift32();
	return r;
}

uae_u32 uaerandgetseed()
{
	if (!randseed) {
		randseed = 1;
		xorshiftstate = 1;
	}
	return randseed;
}

void uaerandomizeseed(void)
{
	if (currprefs.seed == 0) {
		uae_u32 t = getlocaltime();
		uaesetrandseed(t);
	} else {
		uaesetrandseed(currprefs.seed);
	}
}

uae_u32 uaesetrandseed(uae_u32 seed)
{
	if (!seed) {
		seed = 1;
	}
	randseed = seed;
	xorshiftstate = seed;
	return randseed;
}

void my_trim(TCHAR *s)
{
	while (uaetcslen(s) > 0 && _tcscspn(s, _T("\t \r\n")) == 0)
		memmove (s, s + 1, (uaetcslen(s + 1) + 1) * sizeof (TCHAR));
	int len = uaetcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

TCHAR *my_strdup_trim (const TCHAR *s)
{
	if (s[0] == 0)
		return my_strdup(s);
	while (_tcscspn(s, _T("\t \r\n")) == 0)
		s++;
	int len = uaetcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		len--;
	auto* out = xmalloc(TCHAR, len + 1);
	memcpy(out, s, len * sizeof (TCHAR));
	out[len] = 0;
	return out;
}

static void fixup_prefs_dim2(int monid, struct wh *wh)
{
	if (wh->special)
		return;
	if (wh->width < 160) {
		if (!monid)
			error_log (_T("Width (%d) must be at least 160."), wh->width);
		wh->width = 160;
	}
	if (wh->height < 128) {
		if (!monid)
			error_log (_T("Height (%d) must be at least 128."), wh->height);
		wh->height = 128;
	}

	wh->width += 3;
	wh->width &= ~3;

	if (wh->width > max_uae_width) {
		if (!monid)
			error_log (_T("Width (%d) max is %d."), wh->width, max_uae_width);
		wh->width = max_uae_width;
	}
	if (wh->height > max_uae_height) {
		if (!monid)
			error_log (_T("Height (%d) max is %d."), wh->height, max_uae_height);
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
		ap->gfx_backbuffers = std::max(ap->gfx_backbuffers, 1);
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
		if (prefs->gf[i].gfx_filter == 0) {
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

	if (p->cpu_model >= 68020 && p->cpuboard_type && p->address_space_24 && cpuboard_32bit(p)) {
		error_log (_T("24-bit address space is not supported with selected accelerator board configuration."));
		p->address_space_24 = false;
	}
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
	case 68060:
		if (p->fpu_model)
			p->fpu_model = 68060;
		break;
	}

	if (p->cpu_thread && (p->cpu_compatible || p->ppc_mode || p->cpu_memory_cycle_exact || p->cpu_model < 68020)) {
		p->cpu_thread = false;
		error_log(_T("Threaded CPU mode is not compatible with PPC emulation, More compatible or Cycle Exact modes. CPU type must be 68020 or higher."));
	}

#ifdef WITH_PPC
	// 1 = "automatic" PPC config
	if (p->ppc_mode == 1) {
		cpuboard_setboard(p,  BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_PPC);
		if (p->cs_compatible == CP_A1200) {
			cpuboard_setboard(p,  BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC);
		} else if (p->cs_compatible != CP_A4000 && p->cs_compatible != CP_A4000T && p->cs_compatible != CP_A3000 && p->cs_compatible != CP_A3000T) {
			if ((p->cs_ide == IDE_A600A1200 || p->cs_pcmcia) && p->cs_mbdmac <= 0) {
				cpuboard_setboard(p,  BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_PPC);
			}
		}
		p->cpuboardmem1.size = std::max<uae_u32>(p->cpuboardmem1.size, 8 * 1024 * 1024);
	}
#endif

	if (p->cachesize_inhibit) {
		p->cachesize = 0;
	}
	if (p->cpu_model < 68020 && p->cachesize) {
		p->cachesize = 0;
		error_log (_T("JIT requires 68020 or better CPU."));
	}
	if ((p->fpu_model == 0 || !p->cachesize) && p->compfpu) {
		p->compfpu = false;
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

#ifdef JIT
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
#endif


	if (p->immediate_blits && p->waiting_blits) {
		error_log (_T("Immediate blitter and waiting blits can't be enabled simultaneously.\n"));
		p->waiting_blits = 0;
	}
	if (p->cpu_memory_cycle_exact && p->cpu_model <= 68010 && p->waiting_blits) {
		error_log(_T("Wait for blitter is not available in 68000/68010 cycle exact modes.\n"));
		p->waiting_blits = 0;
	}

	if (p->blitter_cycle_exact && !p->cpu_memory_cycle_exact) {
		error_log(_T("Blitter cycle-exact requires at least CPU memory cycle-exact.\n"));
		p->blitter_cycle_exact = 0;
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

	// pre-4.4.0 didn't support cpu multiplier in prefetch mode without cycle-exact
	// set pre-4.4.0 defaults first
	if (!p->cpu_cycle_exact && p->cpu_compatible && !p->cpu_clock_multiplier && p->config_version) {
		if (p->cpu_model < 68020) {
			p->cpu_clock_multiplier = 2 * 256;
		} else if (p->cpu_model == 68020) {
			p->cpu_clock_multiplier = 4 * 256;
		} else {
			p->cpu_clock_multiplier = 8 * 256;
		}
	}

}

void fixup_prefs (struct uae_prefs *p, bool userconfig)
{
	built_in_chipset_prefs (p);
	fixup_cpu (p);
	cfgfile_compatibility_rtg(p);
	cfgfile_compatibility_romtype(p);

	read_kickstart_version(p);

	if (p->cpuboard_type && p->cpuboardmem1.size > cpuboard_maxmemory(p)) {
		error_log(_T("Unsupported accelerator board memory size %d (0x%x).\n"), p->cpuboardmem1.size, p->cpuboardmem1.size);
		p->cpuboardmem1.size = cpuboard_maxmemory(p);
	}
	if (cpuboard_memorytype(p) == BOARD_MEMORY_HIGHMEM) {
		p->mbresmem_high.size = p->cpuboardmem1.size;
	} else if (cpuboard_memorytype(p) == BOARD_MEMORY_Z2) {
		p->fastmem[0].size = p->cpuboardmem1.size;
	} else if (cpuboard_memorytype(p) == BOARD_MEMORY_25BITMEM) {
		p->mem25bit.size = p->cpuboardmem1.size;
	} else if (cpuboard_memorytype(p) == BOARD_MEMORY_CUSTOM_32) {
		p->mem25bit.size = 0;
	}

	if (((p->chipmem.size & (p->chipmem.size - 1)) != 0 && p->chipmem.size != 0x180000)
		|| p->chipmem.size < 0x20000
		|| p->chipmem.size > 0x800000)
	{
		error_log (_T("Unsupported chipmem size %d (0x%x)."), p->chipmem.size, p->chipmem.size);
		p->chipmem.size = 0x200000;
	}

	if (p->chipmem.size == 0x180000 && p->cachesize) {
		error_log(_T("JIT unsupported chipmem size %d (0x%x)."), p->chipmem.size, p->chipmem.size);
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
	if (p->bogomem.size == 0x180000 && p->cachesize) {
		error_log(_T("JIT unsupported bogomem size %d (0x%x)."), p->bogomem.size, p->bogomem.size);
		p->bogomem.size = 0x100000;
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
		if (rbc->rtgmem_type >= GFXBOARD_HARDWARE) {
			if (gfxboard_get_vram_min(rbc) > 0 && rbc->rtgmem_size < gfxboard_get_vram_min (rbc)) {
				error_log(_T("Graphics card memory size %d (0x%x) smaller than minimum hardware supported %d (0x%x)."),
					rbc->rtgmem_size, rbc->rtgmem_size, gfxboard_get_vram_min(rbc), gfxboard_get_vram_min(rbc));
				rbc->rtgmem_size = gfxboard_get_vram_min (rbc);
			}
			if (p->address_space_24 && gfxboard_get_configtype(rbc) == 3) {
				rbc->rtgmem_type = GFXBOARD_UAE_Z2;
				rbc->rtgmem_size = 0;
				error_log (_T("Z3 RTG and 24-bit address space are not compatible."));
			}
			if (gfxboard_get_vram_max(rbc) > 0 && rbc->rtgmem_size > gfxboard_get_vram_max(rbc)) {
				error_log(_T("Graphics card memory size %d (0x%x) larger than maximum hardware supported %d (0x%x)."),
					rbc->rtgmem_size, rbc->rtgmem_size, gfxboard_get_vram_max(rbc), gfxboard_get_vram_max(rbc));
				rbc->rtgmem_size = gfxboard_get_vram_max(rbc);
			}
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
			p->cs_fatgaryrev = std::max(p->cs_fatgaryrev, 0);
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
	p->sana2 = false;
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
	p->gfx_framerate = std::max(p->gfx_framerate, 1);
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
	p->x86_speed_throttle = std::max<float>(p->x86_speed_throttle, 0);

	built_in_chipset_prefs (p);
	blkdev_fix_prefs (p);
	inputdevice_fix_prefs(p, userconfig);
	target_fixup_options (p);
#ifndef AMIBERRY
	// This one caused crashes in some cases, and I don't think it's actually needed in Amiberry
	cfgfile_createconfigstore(p);
#endif
}

int quit_program = 0;
static int restart_program;
static TCHAR restart_config[MAX_DPATH];
static int default_config;

void uae_reset (int hardreset, int keyboardreset)
{
#ifdef DEBUGGER
	if (debug_dma) {
		record_dma_reset(0);
		record_dma_reset(0);
	}
#endif
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
#ifdef DEBUGGER
	deactivate_debugger ();
#endif
	if (quit_program != -UAE_QUIT)
		quit_program = -UAE_QUIT;
	target_quit ();
}

/* 0 = normal, 1 = nogui, -1 = disable nogui, -2 = autorestart */
void uae_restart(struct uae_prefs *p, int opengui, const TCHAR *cfgfile)
{
	uae_quit ();
	restart_program = opengui == -2 ? 4 : (opengui > 0 ? 1 : (opengui == 0 ? 2 : 3));
	restart_config[0] = 0;
	default_config = 0;
	if (cfgfile)
		_tcscpy (restart_config, cfgfile);
	target_restart ();
}

void print_version()
{
	std::cout << get_version_string() << "\n" << get_copyright_notice() << '\n';
	exit(0);
}

void usage()
{
	std::cout << __ver << '\n';
	std::cout << "Usage:" << '\n';
	std::cout << " -h                         Show this help." << '\n';
	std::cout << " --help                     \n" << '\n';
	std::cout << " --log                      Show log output to console." << '\n';
	std::cout << " -f <file>                  Load a configuration file." << '\n';
	std::cout << " --config <file>            " << '\n';
	std::cout << " --model <Amiga Model>      Amiga model to emulate, from the QuickStart options." << '\n';
	std::cout << "                            Available options are: A1000, A500, A500P, A600, A2000, A3000, A1200, A4000, CD32 and CDTV.\n" <<
		'\n';
	std::cout << " --autoload <file>          Load an .lha WHDLoad game or a CD32 CD image, using the WHDBooter." << '\n';
	std::cout << " --cdimage <file>           Load the CD image provided when starting emulation." << '\n';
	std::cout << " --statefile <file>         Load a save state file." << '\n';
	std::cout << " -s <option>=<value>        Set one or more configuration options directly, without loading a file." <<
		'\n';
	std::cout << "                            Edit a configuration file in order to know valid parameters and settings." <<
		'\n';
	std::cout << "\nAdditional options:" << '\n';
	std::cout << "amiberry <file>             Auto-detect the type of file and use the default action for it." << '\n';
	std::cout << "                            Supported file types are: .uae config, .lha WHDLoad, CD images and disk images." << '\n';
	std::cout << " -0 <disk.adf>              Insert specified ADF image into emulated floppy drive 0-3." << '\n';
	std::cout << " -1 <disk.adf>              " << '\n';
	std::cout << " -2 <disk.adf>              " << '\n';
	std::cout << " -3 <disk.adf>              \n" << '\n';
	std::cout << " -diskswapper=d1.adf,d2.adf Comma-separated list of disk images to pre-load to the Disk Swapper." <<
		'\n';
	std::cout << " -r <kick.rom>              Load main ROM from the specified path." << '\n';
	std::cout << " -K <kick.rom>              Load extended ROM from the specified path." << '\n';
	std::cout << " -m VOLNAME:mount_point     Attach a volume directly to the specified mount point." << '\n';
	std::cout << " -W DEVNAME:hardfile        Attach a hardfile with the specified device name." << '\n';
	std::cout << " -S <value>                 Sound parameter specification." << '\n';
	std::cout << " -R <value>                 Output framerate in frames per second." << '\n';
	std::cout << " -i                         Enable illegal memory." << '\n';
	std::cout << " -J <xy>                    Specify joystick 0 (x) and 1 (y). Possible values: 0/1 for joystick, M for mouse, and a/b/c." <<
		'\n';
	std::cout << " -w <value>                 CPU emulation speed. Possible values: 0 (Cycle Exact), -1 (Max)." << '\n';
	std::cout << " -G                         Don't show the GUI, start emulation directly." << '\n';
	std::cout << " -n                         Enable Immediate Blits. Only available when illegal memory is not enabled." <<
		'\n';
	std::cout << " -v <value>                 Set Chipset. Possible values: 0 (OCS), 1 (ECS Agnus), 2 (ECS Denise), 3 (Full ECS), 4 (AGA)." <<
		'\n';
	std::cout << " -C <value>                 Set CPU specs." << '\n';
	std::cout << " -Z <value>                 Z3 FastRAM size, value in 1MB blocks, i.e. 2=2MB." << '\n';
	std::cout << " -U <value>                 RTG Memory size, value in 1MB blocks, i.e. 2=2MB." << '\n';
	std::cout << " -F <value>                 Fastmem size, value in 1MB blocks, i.e. 2=2MB." << '\n';
	std::cout << " -b <value>                 Bogomem size, value in 256KB blocks, i.e. 2=512KB." << '\n';
	std::cout << " -c <value>                 Size of chip memory (in number of 512 KBytes chunks)." << '\n';
	std::cout << " -I <value>                 Set keyboard layout language. Possible values: de, dk, us, se, fr, it, es." <<
		'\n';
	std::cout << " -O <value>                 Set graphics specs." << '\n';
	std::cout << " -H <value>                 Color mode." << '\n';
	std::cout << " -o <amiberry cnf>=<value>  Set Amiberry configuration parameter with value." << '\n';
	std::cout << "                            See: https://github.com/BlitterStudio/amiberry/wiki/Amiberry.conf-options" <<
		'\n';
	std::cout << "\nExample 1:" << '\n';
	std::cout << "amiberry --model A1200 -G" << '\n';
	std::cout << "This will use the A1200 default settings as found in the QuickStart panel." << '\n';
	std::cout << "Additionally, it will override 'use_gui' to 'no', so that it enters emulation directly." << '\n';
	std::cout << "\nExample 2:" << '\n';
	std::cout << "amiberry --config conf/A500.uae --statefile savestates/game.uss -s use_gui=no" << '\n';
	std::cout << "This will load the conf/A500.uae configuration file, with the save state named game." << '\n';
	std::cout << "It will override 'use_gui' to 'no', so that it enters emulation directly." << '\n';
	std::cout << "\nExample 3:" << '\n';
	std::cout << "amiberry lha/MyGame.lha" << '\n';
	std::cout << "This will load the WHDLoad game MyGame.lha, using the autoload mechanism." << '\n';
	exit(0);
}

static void parse_cmdline_2 (int argc, TCHAR **argv)
{
	cfgfile_addcfgparam (nullptr);
	for (auto i = 1; i < argc; i++) {
		if (_tcsncmp(argv[i], _T("-cfgparam="), 10) == 0) {
			cfgfile_addcfgparam(argv[i] + 10);
		} else if (_tcscmp(argv[i], _T("-cfgparam")) == 0) {
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
	int type = zfile_gettype(f);
	if (type == ZFILE_DISKIMAGE || type == ZFILE_EXECUTABLE) {
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
		p1 = nullptr;
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
	auto* const s3 = target_expand_environment (s2, nullptr, 0);
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

extern void set_last_active_config(const char* filename);

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
			auto* txt = parsetextpath(argv[i] + 13);
			parse_diskswapper(txt);
			xfree(txt);
		}
		else if (_tcsncmp(argv[i], _T("-cfgparam="), 10) == 0) {
			;
		}
		else if (_tcscmp(argv[i], _T("-cfgparam")) == 0) {
			if (i + 1 < argc)
				i++;
		}
		else if (_tcscmp(argv[i], _T("--config")) == 0 || _tcscmp(argv[i], _T("-f")) == 0) {
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
				config_loaded = true;
			}
			loaded = true;
		}
		else if (_tcscmp(argv[i], _T("--model")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--model' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
				if (_tcscmp(txt, _T("A500")) == 0)
				{
					bip_a500(&currprefs, 130);
				}
				else if (_tcscmp(txt, _T("A500P")) == 0)
				{
					bip_a500plus(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("A600")) == 0)
				{
					bip_a600(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("A1000")) == 0)
				{
					bip_a1000(&currprefs, -1);
				}
				else if (_tcscmp(txt, _T("A2000")) == 0)
				{
					bip_a2000(&currprefs, 130);
				}
				else if (_tcscmp(txt, _T("A3000")) == 0)
				{
					bip_a3000(&currprefs, -1);
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
				else if (_tcscmp(txt, _T("CDTV")) == 0)
				{
					bip_cdtv(&currprefs, -1);
				}
			}
		}
		else if (_tcscmp(argv[i], _T("--statefile")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '--statefile' option.\n"));
			else
			{
				auto* const txt = parsetextpath(argv[++i]);
#ifdef AMIBERRY
				// We have a secondary check in Amiberry for the existence of the file
				// This allows us to pass the filename without the full path, and it will
				// check in the defined savesates directory as well for it.
				if (my_existsfile2(txt))
				{
					savestate_state = STATE_DORESTORE;
					_tcscpy(savestate_fname, txt);
				}
				else
				{
					get_savestate_path(savestate_fname, MAX_DPATH - 1);
					strncat(savestate_fname, txt, MAX_DPATH - 1);
					savestate_state = STATE_DORESTORE;
				}
				set_last_active_config(txt);
				xfree(txt);
#endif
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
					add_file_to_mru_list(lstMRUWhdloadList, std::string(txt));
					whdload_prefs.whdload_filename = std::string(txt);
					whdload_auto_prefs(&currprefs, txt);
					xfree(txt);
				}
				else if (_tcscmp(txt2.c_str(), ".cue") == 0
					|| _tcscmp(txt2.c_str(), ".iso") == 0
					|| _tcscmp(txt2.c_str(), ".chd") == 0)
				{
					write_log("CDTV/CD32... %s\n", txt);
					add_file_to_mru_list(lstMRUCDList, std::string(txt));
					cd_auto_prefs(&currprefs, txt);
					xfree(txt);
				}
				else
					write_log("Unknown extension for autoload... %s\n", txt);
			}
		}
		else if (_tcscmp(argv[i], _T("--cli")) == 0)
			console_emulation = true;
		else if (_tcscmp(argv[i], _T("--log")) == 0)
			console_logging = 1;
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
				auto* const txt2 = xmalloc(TCHAR, _tcslen(txt) + 7);
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
		} else if (!loaded) {
			auto* const txt = parsetextpath(argv[i]);
			const auto txt2 = get_filename_extension(txt); // Extract the extension from the string  (incl '.')
#ifdef AMIBERRY
			if (_tcscmp(txt2.c_str(), ".lha") == 0)
			{
				write_log("WHDLoad... %s\n", txt);
				add_file_to_mru_list(lstMRUWhdloadList, std::string(txt));
				whdload_prefs.whdload_filename = std::string(txt);
				whdload_auto_prefs(&currprefs, txt);
				set_last_active_config(txt);
			}
			else if (_tcscmp(txt2.c_str(), ".uss") == 0)
			{
				write_log("Statefile... %s\n", txt);
				if (my_existsfile2(txt))
				{
					savestate_state = STATE_DORESTORE;
					_tcscpy(savestate_fname, txt);
					currprefs.start_gui = false;
				}
				else
				{
					get_savestate_path(savestate_fname, MAX_DPATH - 1);
					strncat(savestate_fname, txt, MAX_DPATH - 1);
					savestate_state = STATE_DORESTORE;
					currprefs.start_gui = false;
				}
				set_last_active_config(txt);
			}
			else if (_tcscmp(txt2.c_str(), ".cue") == 0
				|| _tcscmp(txt2.c_str(), ".iso") == 0
				|| _tcscmp(txt2.c_str(), ".chd") == 0)
			{
				write_log("CDTV/CD32... %s\n", txt);
				add_file_to_mru_list(lstMRUCDList, std::string(txt));
				cd_auto_prefs(&currprefs, txt);
				set_last_active_config(txt);
			}
			else if (_tcscmp(txt2.c_str(), ".adf") == 0
				|| _tcscmp(txt2.c_str(), ".adz") == 0
				|| _tcscmp(txt2.c_str(), ".dms") == 0
				|| _tcscmp(txt2.c_str(), ".ipf") == 0
				|| _tcscmp(txt2.c_str(), ".zip") == 0
				)
			{
				write_log("Floppy... %s\n", txt);

				// Check if a config file exists with the same name as the disk image
				auto full_path = std::string(txt);
				std::filesystem::path p(full_path);
				std::string filename = p.filename().string();

				std::string config_extension = "uae";
				const std::size_t pos = filename.find_last_of('.');
				if (pos != std::string::npos) {
					filename = filename.substr(0, pos).append(".").append(config_extension);
				}
				else {
					// No extension found
					filename += "." + config_extension;
				}

				std::string config_full_path = get_configuration_path() + filename;
				if (my_existsfile2(config_full_path.c_str()))
				{
					write_log("Loading configuration file %s\n", config_full_path.c_str());
					currprefs.mountitems = 0;
					target_cfgfile_load(&currprefs, config_full_path.c_str(),
						firstconfig
						? CONFIG_TYPE_ALL
						: CONFIG_TYPE_HARDWARE | CONFIG_TYPE_HOST | CONFIG_TYPE_NORESET, 0);
					currprefs.start_gui = false;
					config_loaded = true;
				}
				else
				{
					write_log("No configuration file found for %s, inserting disk in DF0: with default settings\n", txt);
					disk_insert(0, txt);
					set_last_active_config(txt);
					currprefs.start_gui = false;
				}
			}
#endif
			else
			{
				auto* const z = zfile_fopen(txt, _T("rb"), ZFD_NORMAL);
				if (z) {
					const auto type = zfile_gettype(z);
					zfile_fclose(z);
					if (type == ZFILE_CONFIGURATION) {
						currprefs.mountitems = 0;
						target_cfgfile_load(&currprefs, txt, CONFIG_TYPE_ALL, 0);
						config_loaded = true;
					}
					else if (type == ZFILE_STATEFILE) {
						savestate_state = STATE_DORESTORE;
						_tcscpy(savestate_fname, txt);
					}
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

static void parse_cmdline_and_init_file(int argc, TCHAR **argv)
{
	_tcscpy (optionsfile, _T(""));

	write_log("Command line parameters:");
	for (int i = 0; i < argc; i++)
	{
		write_log(" %s", argv[i]);
	}
	write_log("\n");

	parse_cmdline_2(argc, argv);

	_tcscat(optionsfile, restart_config);

	if (my_existsfile2(optionsfile))
	{
		if (!target_cfgfile_load(&currprefs, optionsfile, CONFIG_TYPE_DEFAULT, default_config)) {
			write_log(_T("failed to load config '%s'\n"), optionsfile);
		}
		else
		{
			config_loaded = true;
		}
	}

	parse_cmdline(argc, argv);

	fixup_prefs(&currprefs, false);
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
static void do_start_program ()
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

static void start_program ()
{
	do_start_program ();
}

static void leave_program ()
{
	do_leave_program ();
}

long get_file_size(const std::string& filename)
{
	struct stat stat_buf{};
	const auto rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? static_cast<long>(stat_buf.st_size) : -1;
}

// In case of error, print the error code and close the application
void check_error_sdl(const bool check, const char* message)
{
	if (check)
	{
		std::cout << message << " " << SDL_GetError() << '\n';
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
#ifdef AMIBERRY
	preinit_shm ();
#else
	//preinit_shm ();
#endif
#endif

	event_init();

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
	inputdevice_updateconfig(&currprefs, &changed_prefs);
	inputdevice_config_change();

	no_gui = ! currprefs.start_gui;
	if (restart_program == 2 || restart_program == 4)
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

#ifdef JIT
	compiler_init();
#endif
#ifdef NATMEM_OFFSET
	if (!init_shm ()) {
		if (currprefs.start_gui)
			uae_restart(&currprefs, -1, nullptr);
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
	uaerandomizeseed();
	copy_prefs(&currprefs, &changed_prefs);
	target_run ();
	/* force sound settings change */
	currprefs.produce_sound = 0;

#ifdef SAVESTATE
	savestate_init ();
#endif
	keybuf_init (); /* Must come after init_joystick */

#ifdef DEBUGGER
	disasm_init();
#endif
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

	if (graphics_init (true)) {
	// This never gets triggered anyway
//#ifdef DEBUGGER
//		setup_brkhandler ();
//		if (currprefs.start_debugger && debuggable ())
//			activate_debugger ();
//#endif
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
