/*
  * UAE - The Un*x Amiga Emulator
  *
  * Main program
  *
  * Copyright 1995 Ed Hanway
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */
#include "sysconfig.h"
#include "sysdeps.h"
#include <assert.h>

#include "options.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "gensound.h"
#include "audio.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "disk.h"
#include "xwin.h"
#include "inputdevice.h"
#include "keybuf.h"
#include "gui.h"
#include "zfile.h"
#include "autoconf.h"
#include "traps.h"
#include "osemu.h"
#include "picasso96.h"
#include "bsdsocket.h"
#include "native2amiga.h"
#include "akiko.h"
#include "savestate.h"
#include "blkdev.h"
#include "uaeresource.h"
#include "gfxboard.h"

#ifdef JIT
#include "jit/compemu.h"
#endif

#ifdef USE_SDL
#include "SDL.h"
#include <iostream>
#include "amiberry_gfx.h"
SDL_Window* sdlWindow;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_DisplayMode sdlMode;
#endif

#include <linux/kd.h>
#include <sys/ioctl.h>
#include "keyboard.h"

long int version = 256 * 65536L*UAEMAJOR + 65536L*UAEMINOR + UAESUBREV;

struct uae_prefs currprefs, changed_prefs; 
int config_changed;

bool no_gui = false, quit_to_gui = false;
bool cloanto_rom = false;
bool kickstart_rom = true;
bool console_emulation = false;

struct gui_info gui_data;

TCHAR warning_buffer[256];

TCHAR optionsfile[256];

static uae_u32 randseed;
static int oldhcounter;

uae_u32 uaesrand(uae_u32 seed)
{
	oldhcounter = -1;
	randseed = seed;
	//randseed = 0x12345678;
	//write_log (_T("seed=%08x\n"), randseed);
	return randseed;
}
uae_u32 uaerand()
{
	if (oldhcounter != hsync_counter) {
		srand(hsync_counter ^ randseed);
		oldhcounter = hsync_counter;
	}
	uae_u32 r = rand();
	//write_log (_T("rand=%08x\n"), r);
	return r;
}
uae_u32 uaerandgetseed()
{
	return randseed;
}

void my_trim(TCHAR *s)
{
	int len;
	while (_tcslen(s) > 0 && _tcscspn(s, _T("\t \r\n")) == 0)
		memmove(s, s + 1, (_tcslen(s + 1) + 1) * sizeof(TCHAR));
	len = _tcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		s[--len] = '\0';
}

TCHAR *my_strdup_trim(const TCHAR *s)
{
	TCHAR *out;
	int len;

	while (_tcscspn(s, _T("\t \r\n")) == 0)
		s++;
	len = _tcslen(s);
	while (len > 0 && _tcscspn(s + len - 1, _T("\t \r\n")) == 0)
		len--;
	out = xmalloc(TCHAR, len + 1);
	memcpy(out, s, len * sizeof(TCHAR));
	out[len] = 0;
	return out;
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
#ifdef FILESYS
	filesys_cleanup();
#endif
}

static void fixup_prefs_dim2(struct wh *wh)
{
	if (wh->special)
		return;
	if (wh->width < 160) {
		error_log(_T("Width (%d) must be at least 128."), wh->width);
		wh->width = 160;
	}
	if (wh->height < 128) {
		error_log(_T("Height (%d) must be at least 128."), wh->height);
		wh->height = 128;
	}
	if (wh->width > max_uae_width) {
		error_log(_T("Width (%d) max is %d."), wh->width, max_uae_width);
		wh->width = max_uae_width;
	}
	if (wh->height > max_uae_height) {
		error_log(_T("Height (%d) max is %d."), wh->height, max_uae_height);
		wh->height = max_uae_height;
	}
}

void fixup_prefs_dimensions(struct uae_prefs *prefs)
{
	fixup_prefs_dim2(&prefs->gfx_size_fs);
	fixup_prefs_dim2(&prefs->gfx_size_win);
	if (prefs->gfx_apmode[1].gfx_vsync)
		prefs->gfx_apmode[1].gfx_vsyncmode = 1;

	for (int i = 0; i < 2; i++) {
		struct apmode *ap = &prefs->gfx_apmode[i];
		ap->gfx_vflip = 0;
		ap->gfx_strobo = false;
		if (ap->gfx_vsync) {
			if (ap->gfx_vsyncmode) {
				// low latency vsync: no flip only if no-buffer
				if (ap->gfx_backbuffers >= 1)
					ap->gfx_vflip = 1;
				if (!i && ap->gfx_backbuffers == 2)
					ap->gfx_vflip = 1;
				ap->gfx_strobo = prefs->lightboost_strobo;
			}
			else {
				// legacy vsync: always wait for flip
				ap->gfx_vflip = -1;
				if (prefs->gfx_api && ap->gfx_backbuffers < 1)
					ap->gfx_backbuffers = 1;
				if (ap->gfx_vflip)
					ap->gfx_strobo = prefs->lightboost_strobo;;
			}
		}
		else {
			// no vsync: wait if triple bufferirng
			if (ap->gfx_backbuffers >= 2)
				ap->gfx_vflip = -1;
		}
		if (prefs->gf[i].gfx_filter == 0 && ((prefs->gf[i].gfx_filter_autoscale && !prefs->gfx_api) || (prefs->gfx_apmode[APMODE_NATIVE].gfx_vsyncmode))) {
			prefs->gf[i].gfx_filter = 1;
		}
		if (i == 0 && prefs->gf[i].gfx_filter == 0 && prefs->monitoremu) {
			error_log(_T("A2024 and Graffiti require at least null filter enabled."));
			prefs->gf[i].gfx_filter = 1;
		}
	}
}

void fixup_cpu(struct uae_prefs *p)
{
	if (p->cpu_frequency == 1000000)
		p->cpu_frequency = 0;

	if (p->cpu_model >= 68030 && p->address_space_24) {
		error_log(_T("24-bit address space is not supported in 68030/040/060 configurations."));
		p->address_space_24 = false;
	}
	if (p->cpu_model < 68020 && p->fpu_model && (p->cpu_compatible || p->cpu_cycle_exact)) {
		error_log(_T("FPU is not supported in 68000/010 configurations."));
		p->fpu_model = 0;
	}

	switch (p->cpu_model)
	{
	case 68000:
		p->address_space_24 = true;
		break;
	case 68010:
		p->address_space_24 = true;
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

	if (p->cpu_model < 68020 && p->cachesize) {
		p->cachesize = 0;
		error_log(_T("JIT requires 68020 or better CPU."));
	}

	if (p->cpu_model >= 68040 && p->cachesize && p->cpu_compatible)
		p->cpu_compatible = false;

	if (p->cpu_model >= 68040 && p->cpu_cycle_exact) {
		p->cpu_cycle_exact = false;
		error_log(_T("68040/060 cycle-exact is not supported."));
	}

	if ((p->cpu_model < 68030 || p->cachesize) && p->mmu_model) {
		error_log(_T("MMU emulation requires 68030/040/060 and it is not JIT compatible."));
		p->mmu_model = 0;
	}

	if (p->cachesize && p->cpu_cycle_exact) {
		error_log(_T("JIT and cycle-exact can't be enabled simultaneously."));
		p->cachesize = 0;
	}
	if (p->cachesize && (p->fpu_no_unimplemented || p->int_no_unimplemented)) {
		error_log(_T("JIT is not compatible with unimplemented CPU/FPU instruction emulation."));
		p->fpu_no_unimplemented = p->int_no_unimplemented = false;
	}

	if (p->cpu_cycle_exact && p->m68k_speed < 0)
		p->m68k_speed = 0;

	if (p->immediate_blits && p->blitter_cycle_exact) {
		error_log(_T("Cycle-exact and immediate blitter can't be enabled simultaneously.\n"));
		p->immediate_blits = false;
	}
	if (p->immediate_blits && p->waiting_blits) {
		error_log(_T("Immediate blitter and waiting blits can't be enabled simultaneously.\n"));
		p->waiting_blits = 0;
	}
	if (p->cpu_cycle_exact)
		p->cpu_compatible = true;
}


void fixup_prefs(struct uae_prefs *p, bool userconfig)
{
	int err = 0;

	built_in_chipset_prefs(p);
	fixup_cpu(p);
	cfgfile_compatibility_rtg(p);
	cfgfile_compatibility_romtype(p);

	read_kickstart_version(p);

	//if (p->cpuboard_type && p->cpuboardmem1_size > cpuboard_maxmemory(p)) {
	//	error_log(_T("Unsupported accelerator board memory size %d (0x%x).\n"), p->cpuboardmem1_size, p->cpuboardmem1_size);
	//	p->cpuboardmem1_size = cpuboard_maxmemory(p);
	//}
	//if (cpuboard_memorytype(p) == BOARD_MEMORY_HIGHMEM) {
	//	p->mbresmem_high_size = p->cpuboardmem1_size;
	//}
	//else if (cpuboard_memorytype(p) == BOARD_MEMORY_Z2) {
	//	p->fastmem[0].size = p->cpuboardmem1_size;
	//}
	//else if (cpuboard_memorytype(p) == BOARD_MEMORY_25BITMEM) {
	//	p->mem25bit_size = p->cpuboardmem1_size;
	//}

	if (((p->chipmem_size & (p->chipmem_size - 1)) != 0 && p->chipmem_size != 0x180000)
		|| p->chipmem_size < 0x20000
		|| p->chipmem_size > 0x800000)
	{
		error_log(_T("Unsupported chipmem size %d (0x%x)."), p->chipmem_size, p->chipmem_size);
		p->chipmem_size = 0x200000;
		err = 1;
	}

	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		if ((p->fastmem[i].size & (p->fastmem[i].size - 1)) != 0
			|| (p->fastmem[i].size != 0 && (p->fastmem[i].size < 0x10000 || p->fastmem[i].size > 0x800000)))
		{
			error_log(_T("Unsupported fastmem size %d (0x%x)."), p->fastmem[i].size, p->fastmem[i].size);
			p->fastmem[i].size = 0;
			err = 1;
		}
	}

	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct rtgboardconfig *rbc = &p->rtgboards[i];
		if (rbc->rtgmem_size > max_z3fastmem && rbc->rtgmem_type == GFXBOARD_UAE_Z3) {
			error_log(_T("Graphics card memory size %d (0x%x) larger than maximum reserved %d (0x%x)."), rbc->rtgmem_size, rbc->rtgmem_size, max_z3fastmem, max_z3fastmem);
			rbc->rtgmem_size = max_z3fastmem;
			err = 1;
		}

		if ((rbc->rtgmem_size & (rbc->rtgmem_size - 1)) != 0 || (rbc->rtgmem_size != 0 && (rbc->rtgmem_size < 0x100000))) {
			error_log(_T("Unsupported graphics card memory size %d (0x%x)."), rbc->rtgmem_size, rbc->rtgmem_size);
			if (rbc->rtgmem_size > max_z3fastmem)
				rbc->rtgmem_size = max_z3fastmem;
			else
				rbc->rtgmem_size = 0;
			err = 1;
		}
	}

	for (int i = 0; i < MAX_RAM_BOARDS; i++) {
		if ((p->z3fastmem[i].size & (p->z3fastmem[i].size - 1)) != 0 || (p->z3fastmem[i].size != 0 && p->z3fastmem[i].size < 0x100000))
		{
			error_log(_T("Unsupported Zorro III fastmem size %d (0x%x)."), p->z3fastmem[i].size, p->z3fastmem[i].size);
			p->z3fastmem[i].size = 0;
			err = 1;
		}
	}

	p->z3autoconfig_start &= ~0xffff;
	if (p->z3autoconfig_start < 0x1000000)
		p->z3autoconfig_start = 0x1000000;

	if (p->z3chipmem_size > max_z3fastmem) {
		error_log(_T("Zorro III fake chipmem size %d (0x%x) larger than max reserved %d (0x%x)."), p->z3chipmem_size, p->z3chipmem_size, max_z3fastmem, max_z3fastmem);
		p->z3chipmem_size = max_z3fastmem;
		err = 1;
	}
	if (((p->z3chipmem_size & (p->z3chipmem_size - 1)) != 0 && p->z3chipmem_size != 0x18000000 && p->z3chipmem_size != 0x30000000) || (p->z3chipmem_size != 0 && p->z3chipmem_size < 0x100000))
	{
		error_log(_T("Unsupported 32-bit chipmem size %d (0x%x)."), p->z3chipmem_size, p->z3chipmem_size);
		p->z3chipmem_size = 0;
		err = 1;
	}

	if (p->address_space_24 && (p->z3fastmem[0].size != 0 || p->z3fastmem[1].size != 0 || p->z3fastmem[2].size != 0 || p->z3fastmem[3].size != 0 || p->z3chipmem_size != 0)) {
		p->z3fastmem[0].size = p->z3fastmem[1].size = p->z3fastmem[2].size = p->z3fastmem[3].size = 0;
		p->z3chipmem_size = 0;
		error_log(_T("Can't use a Z3 graphics card or 32-bit memory when using a 24 bit address space."));
	}

	if (p->bogomem_size != 0 && p->bogomem_size != 0x80000 && p->bogomem_size != 0x100000 && p->bogomem_size != 0x180000 && p->bogomem_size != 0x1c0000) {
		error_log(_T("Unsupported bogomem size %d (0x%x)"), p->bogomem_size, p->bogomem_size);
		p->bogomem_size = 0;
		err = 1;
	}

	if (p->bogomem_size > 0x180000 && (p->cs_fatgaryrev >= 0 || p->cs_ide || p->cs_ramseyrev >= 0)) {
		p->bogomem_size = 0x180000;
		error_log(_T("Possible Gayle bogomem conflict fixed."));
	}
	if (p->chipmem_size > 0x200000 && (p->fastmem[0].size > 262144 || p->fastmem[1].size > 262144)) {
		error_log(_T("You can't use fastmem and more than 2MB chip at the same time."));
		p->chipmem_size = 0x200000;
		err = 1;
	}
	if (p->mem25bit_size > 128 * 1024 * 1024 || (p->mem25bit_size & 0xfffff)) {
		p->mem25bit_size = 0;
		error_log(_T("Unsupported 25bit RAM size"));
	}
	if (p->mbresmem_low_size > 0x04000000 || (p->mbresmem_low_size & 0xfffff)) {
		p->mbresmem_low_size = 0;
		error_log(_T("Unsupported Mainboard RAM size"));
	}
	if (p->mbresmem_high_size > 0x08000000 || (p->mbresmem_high_size & 0xfffff)) {
		p->mbresmem_high_size = 0;
		error_log(_T("Unsupported CPU Board RAM size."));
	}

	//for (int i = 0; i < MAX_RTG_BOARDS; i++) {
	//	struct rtgboardconfig *rbc = &p->rtgboards[i];
	//	if (p->chipmem_size > 0x200000 && rbc->rtgmem_size && gfxboard_get_configtype(rbc) == 2) {
	//		error_log(_T("You can't use Zorro II RTG and more than 2MB chip at the same time."));
	//		p->chipmem_size = 0x200000;
	//		err = 1;
	//	}
	//	if (rbc->rtgmem_type >= GFXBOARD_HARDWARE) {
	//		if (gfxboard_get_vram_min(rbc) > 0 && rbc->rtgmem_size < gfxboard_get_vram_min(rbc)) {
	//			error_log(_T("Graphics card memory size %d (0x%x) smaller than minimum hardware supported %d (0x%x)."),
	//				rbc->rtgmem_size, rbc->rtgmem_size, gfxboard_get_vram_min(rbc), gfxboard_get_vram_min(rbc));
	//			rbc->rtgmem_size = gfxboard_get_vram_min(rbc);
	//		}
	//		if (p->address_space_24 && gfxboard_get_configtype(rbc) == 3) {
	//			rbc->rtgmem_type = GFXBOARD_UAE_Z2;
	//			rbc->rtgmem_size = 0;
	//			error_log(_T("Z3 RTG and 24-bit address space are not compatible."));
	//		}
	//		if (gfxboard_get_vram_max(rbc) > 0 && rbc->rtgmem_size > gfxboard_get_vram_max(rbc)) {
	//			error_log(_T("Graphics card memory size %d (0x%x) larger than maximum hardware supported %d (0x%x)."),
	//				rbc->rtgmem_size, rbc->rtgmem_size, gfxboard_get_vram_max(rbc), gfxboard_get_vram_max(rbc));
	//			rbc->rtgmem_size = gfxboard_get_vram_max(rbc);
	//		}
	//	}
	//	if (p->address_space_24 && rbc->rtgmem_size && rbc->rtgmem_type == GFXBOARD_UAE_Z3) {
	//		error_log(_T("Z3 RTG and 24bit address space are not compatible."));
	//		rbc->rtgmem_type = GFXBOARD_UAE_Z2;
	//		rbc->rtgmem_size = 0;
	//	}
	//}

	if (p->cs_z3autoconfig && p->address_space_24) {
		p->cs_z3autoconfig = false;
		error_log(_T("Z3 autoconfig and 24bit address space are not compatible."));
	}

#if 0
	if (p->m68k_speed < -1 || p->m68k_speed > 20) {
		write_log(_T("Bad value for -w parameter: must be -1, 0, or within 1..20.\n"));
		p->m68k_speed = 4;
		err = 1;
	}
#endif

	if (p->produce_sound < 0 || p->produce_sound > 3) {
		error_log(_T("Bad value for -S parameter: enable value must be within 0..3."));
		p->produce_sound = 0;
		err = 1;
	}
	if (p->comptrustbyte < 0 || p->comptrustbyte > 3) {
		error_log(_T("Bad value for comptrustbyte parameter: value must be within 0..2."));
		p->comptrustbyte = 1;
		err = 1;
	}
	if (p->comptrustword < 0 || p->comptrustword > 3) {
		error_log(_T("Bad value for comptrustword parameter: value must be within 0..2."));
		p->comptrustword = 1;
		err = 1;
	}
	if (p->comptrustlong < 0 || p->comptrustlong > 3) {
		error_log(_T("Bad value for comptrustlong parameter: value must be within 0..2."));
		p->comptrustlong = 1;
		err = 1;
	}
	if (p->comptrustnaddr < 0 || p->comptrustnaddr > 3) {
		error_log(_T("Bad value for comptrustnaddr parameter: value must be within 0..2."));
		p->comptrustnaddr = 1;
		err = 1;
	}
	if (p->cachesize < 0 || p->cachesize > 16384) {
		error_log(_T("Bad value for cachesize parameter: value must be within 0..16384."));
		p->cachesize = 0;
		err = 1;
	}
	if ((p->z3fastmem[0].size || p->z3fastmem[1].size || p->z3fastmem[2].size || p->z3fastmem[3].size || p->z3chipmem_size) && p->address_space_24) {
		error_log(_T("Z3 fast memory can't be used if address space is 24-bit."));
		p->z3fastmem[0].size = 0;
		p->z3fastmem[1].size = 0;
		p->z3fastmem[2].size = 0;
		p->z3fastmem[3].size = 0;
		p->z3chipmem_size = 0;
		err = 1;
	}
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if ((p->rtgboards[i].rtgmem_size > 0 && p->rtgboards[i].rtgmem_type == GFXBOARD_UAE_Z3) && p->address_space_24) {
			error_log(_T("UAEGFX Z3 RTG can't be used if address space is 24-bit."));
			p->rtgboards[i].rtgmem_size = 0;
			err = 1;
		}
	}

#if !defined (BSDSOCKET)
	if (p->socket_emu) {
		write_log(_T("Compile-time option of BSDSOCKET_SUPPORTED was not enabled.  You can't use bsd-socket emulation.\n"));
		p->socket_emu = 0;
		err = 1;
	}
#endif
	if (p->socket_emu && p->uaeboard >= 3) {
		write_log(_T("bsdsocket.library is not compatible with indirect UAE Boot ROM.\n"));
		p->socket_emu = 0;
	}

	if (p->nr_floppies < 0 || p->nr_floppies > 4) {
		error_log(_T("Invalid number of floppies.  Using 2."));
		p->nr_floppies = 2;
		p->floppyslots[0].dfxtype = 0;
		p->floppyslots[1].dfxtype = 0;
		p->floppyslots[2].dfxtype = -1;
		p->floppyslots[3].dfxtype = -1;
		err = 1;
	}
	if (p->floppy_speed > 0 && p->floppy_speed < 10) {
		error_log(_T("Invalid floppy speed."));
		p->floppy_speed = 100;
	}
	if (p->input_mouse_speed < 1 || p->input_mouse_speed > 1000) {
		error_log(_T("Invalid mouse speed."));
		p->input_mouse_speed = 100;
	}
	if (p->collision_level < 0 || p->collision_level > 3) {
		error_log(_T("Invalid collision support level.  Using 1."));
		p->collision_level = 1;
		err = 1;
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
}
	else if (p->cs_compatible == 0) {
		if (p->cs_ide == IDE_A4000) {
			if (p->cs_fatgaryrev < 0)
				p->cs_fatgaryrev = 0;
			if (p->cs_ramseyrev < 0)
				p->cs_ramseyrev = 0x0f;
		}
	}
	if (p->chipmem_size >= 0x100000)
		p->cs_1mchipjumper = true;

	/* Can't fit genlock and A2024 or Graffiti at the same time,
	* also Graffiti uses genlock audio bit as an enable signal
	*/
	if (p->genlock && p->monitoremu) {
		error_log(_T("Genlock and A2024 or Graffiti can't be active simultaneously."));
		p->genlock = false;
	}
	if (p->cs_hacks) {
		error_log(_T("chipset_hacks is nonzero (0x%04x)."), p->cs_hacks);
	}

	fixup_prefs_dimensions(p);

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
	p->cpu_cycle_exact = p->blitter_cycle_exact = 0;
#endif
#ifndef AGA
	p->chipset_mask &= ~CSMASK_AGA;
#endif
#ifndef AUTOCONFIG
	p->z3fastmem_size = 0;
	p->fastmem_size = 0;
	p->rtgmem_size = 0;
#endif
#if !defined (BSDSOCKET)
	p->socket_emu = 0;
#endif
#if !defined (SCSIEMU)
	p->scsi = 0;
//#ifdef _WIN32
//	p->win32_aspi = 0;
//#endif
#endif
#if !defined (SANA2)
	p->sana2 = 0;
#endif
#if !defined (UAESERIAL)
	p->uaeserial = 0;
#endif
#if defined (CPUEMU_13)
	if (p->cpu_memory_cycle_exact) {
		if (p->gfx_framerate > 1) {
			error_log(_T("Cycle-exact requires disabled frameskip."));
			p->gfx_framerate = 1;
		}
		if (p->cachesize) {
			error_log(_T("Cycle-exact and JIT can't be active simultaneously."));
			p->cachesize = 0;
		}
#if 0
		if (p->m68k_speed) {
			error_log(_T("Adjustable CPU speed is not available in cycle-exact mode."));
			p->m68k_speed = 0;
		}
#endif
	}
#endif
	if (p->gfx_framerate < 1)
		p->gfx_framerate = 1;
	if (p->maprom && !p->address_space_24) {
		p->maprom = 0x0f000000;
	}
	if (((p->maprom & 0xff000000) && p->address_space_24) || (p->maprom && p->mbresmem_high_size >= 0x08000000)) {
		p->maprom = 0x00e00000;
	}
	if (p->maprom && p->cpuboard_type) {
		error_log(_T("UAE Maprom and accelerator board emulation are not compatible."));
		p->maprom = 0;
	}

	if (p->tod_hack && p->cs_ciaatod == 0)
		p->cs_ciaatod = p->ntscmode ? 2 : 1;

	built_in_chipset_prefs(p);
	blkdev_fix_prefs(p);
	//inputdevice_fix_prefs(p, userconfig);
	target_fixup_options(p);
}

int quit_program = 0;
static int restart_program;
static TCHAR restart_config[MAX_DPATH];
static int default_config;

void uae_reset(int hardreset, int keyboardreset)
{
	currprefs.quitstatefile[0] = changed_prefs.quitstatefile[0] = 0;

	if (quit_program == 0) {
		quit_program = -UAE_RESET;
		if (keyboardreset)
			quit_program = -UAE_RESET_KEYBOARD;
		if (hardreset)
			quit_program = -UAE_RESET_HARD;
	}
}

void uae_quit()
{
	if (quit_program != -UAE_QUIT) {
		quit_program = -UAE_QUIT;
	}
	target_quit();
}

void host_shutdown()
{
	system("sudo poweroff");
}

/* 0 = normal, 1 = nogui, -1 = disable nogui */
void uae_restart(int opengui, const TCHAR *cfgfile)
{
	uae_quit();
	restart_program = opengui > 0 ? 1 : (opengui == 0 ? 2 : 3);
	restart_config[0] = 0;
	default_config = 0;
	if (cfgfile)
		_tcscpy(restart_config, cfgfile);
	target_restart();
}

void usage()
{
}

static void parse_cmdline_2(int argc, TCHAR **argv)
{
	int i;

	cfgfile_addcfgparam(nullptr);
	for (i = 1; i < argc; i++) {
		if (_tcsncmp(argv[i], _T("-cfgparam="), 10) == 0) {
			cfgfile_addcfgparam(argv[i] + 10);
		}
		else if (_tcscmp(argv[i], _T("-cfgparam")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '-cfgparam' option.\n"));
			else
				cfgfile_addcfgparam(argv[++i]);
		}
	}
}

static int diskswapper_cb(struct zfile *f, void *vrsd)
{
	int *num = static_cast<int*>(vrsd);
	if (*num >= MAX_SPARE_DRIVES)
		return 1;
	if (zfile_gettype(f) == ZFILE_DISKIMAGE) {
		_tcsncpy(currprefs.dfxlist[*num], zfile_getname(f), 255);
		(*num)++;
	}
	return 0;
}

static void parse_diskswapper(const TCHAR *s)
{
	TCHAR *tmp = my_strdup(s);
	TCHAR *delim = _T(",");
	TCHAR *p1, *p2;
	int num = 0;

	p1 = tmp;
	for (;;) {
		p2 = strtok(p1, delim);
		if (!p2)
			break;
		p1 = nullptr;
		if (num >= MAX_SPARE_DRIVES)
			break;
		if (!zfile_zopen(p2, diskswapper_cb, &num)) {
			_tcsncpy(currprefs.dfxlist[num], p2, 255);
			num++;
		}
	}
	free(tmp);
}

static TCHAR *parsetext(const TCHAR *s)
{
	if (*s == '"' || *s == '\'') {
		TCHAR *d;
		TCHAR c = *s++;
		int i;
		d = my_strdup(s);
		for (i = 0; i < _tcslen(d); i++) {
			if (d[i] == c) {
				d[i] = 0;
				break;
			}
		}
		return d;
	}
	else {
		return my_strdup(s);
	}
}
static TCHAR *parsetextpath(const TCHAR *s)
{
	TCHAR *s2 = parsetext(s);
	TCHAR *s3 = target_expand_environment(s2);
	xfree(s2);
	return s3;
}

static void parse_cmdline(int argc, TCHAR **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!_tcsncmp(argv[i], _T("-diskswapper="), 13)) {
			TCHAR *txt = parsetextpath(argv[i] + 13);
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
		else if (_tcsncmp(argv[i], _T("-config="), 8) == 0) {
			TCHAR *txt = parsetextpath(argv[i] + 8);
			currprefs.mountitems = 0;
			target_cfgfile_load(&currprefs, txt, -1, 0);
			xfree(txt);
		}
		else if (_tcsncmp(argv[i], _T("-statefile="), 11) == 0) {
			TCHAR *txt = parsetextpath(argv[i] + 11);
			savestate_state = STATE_DORESTORE;
			_tcscpy(savestate_fname, txt);
			xfree(txt);
		}
		else if (_tcscmp(argv[i], _T("-f")) == 0) {
			/* Check for new-style "-f xxx" argument, where xxx is config-file */
			if (i + 1 == argc) {
				write_log(_T("Missing argument for '-f' option.\n"));
			}
			else {
				TCHAR *txt = parsetextpath(argv[++i]);
				currprefs.mountitems = 0;
				target_cfgfile_load(&currprefs, txt, -1, 0);
				xfree(txt);
			}
		}
		else if (_tcscmp(argv[i], _T("-s")) == 0) {
			if (i + 1 == argc)
				write_log(_T("Missing argument for '-s' option.\n"));
			else
				cfgfile_parse_line(&currprefs, argv[++i], 0);
		}
		else if (_tcscmp(argv[i], _T("-h")) == 0 || _tcscmp(argv[i], _T("-help")) == 0) {
			usage();
			exit(0);
		}
		else if (_tcsncmp(argv[i], _T("-cdimage="), 9) == 0) {
			TCHAR *txt = parsetextpath(argv[i] + 9);
			TCHAR *txt2 = xmalloc(TCHAR, _tcslen(txt) + 2);
			_tcscpy(txt2, txt);
			if (_tcsrchr(txt2, ',') != NULL)
				_tcscat(txt2, _T(","));
			cfgfile_parse_option(&currprefs, _T("cdimage0"), txt2, 0);
			xfree(txt2);
			xfree(txt);
		}
		else {
			if (argv[i][0] == '-' && argv[i][1] != '\0') {
				const TCHAR *arg = argv[i] + 2;
				int extra_arg = *arg == '\0';
				if (extra_arg)
					arg = i + 1 < argc ? argv[i + 1] : 0;
				if (parse_cmdline_option(&currprefs, argv[i][1], arg) && extra_arg)
					i++;
			}
		}
	}
}

static void parse_cmdline_and_init_file(int argc, TCHAR **argv)
{
	_tcscpy(optionsfile, _T(""));

	parse_cmdline_2(argc, argv);

	_tcscat(optionsfile, restart_config);

	if (!target_cfgfile_load(&currprefs, optionsfile, 0, default_config)) {
		write_log(_T("failed to load config '%s'\n"), optionsfile);
	}
	fixup_prefs(&currprefs, false);

	parse_cmdline(argc, argv);
}

void reset_all_systems()
{
	init_eventtab();

#ifdef PICASSO96
	picasso_reset();
#endif
#ifdef SCSIEMU
	scsi_reset();
	scsidev_reset();
	scsidev_start_threads();
#endif
#ifdef A2065
	a2065_reset();
#endif
#ifdef SANA2
	netdev_reset();
	netdev_start_threads();
#endif
#ifdef FILESYS
	filesys_prepare_reset();
	filesys_reset();
#endif
	//TODO
	//init_shm();
	memory_reset();
#if defined (BSDSOCKET)
	bsdlib_reset();
#endif
#ifdef FILESYS
	filesys_start_threads();
	hardfile_reset();
#endif
#ifdef UAESERIAL
	uaeserialdev_reset();
	uaeserialdev_start_threads();
#endif
#if defined (PARALLEL_PORT)
	initparallel();
#endif
	native2amiga_reset();
	//dongle_reset();
	//sampler_init();
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
void do_start_program()
{
	if (quit_program == -UAE_QUIT)
		return;

	/* Do a reset on startup. Whether this is elegant is debatable. */
	inputdevice_updateconfig(&changed_prefs, &currprefs);
	if (quit_program >= 0)
		quit_program = UAE_RESET;
#ifdef WITH_LUA
	uae_lua_loadall();
#endif
	m68k_go(1);
}

void do_leave_program()
{
#ifdef JIT
	compiler_exit();
#endif
	//sampler_free ();
	graphics_leave();
	inputdevice_close();
	DISK_free();
	close_sound();
	dump_counts();
#ifdef SERIAL_PORT
	serial_exit();
#endif
#ifdef CDTV
	cdtv_free();
#endif
#ifdef A2091
	a2091_free();
	a3000scsi_free();
#endif
#ifdef NCR
	ncr_free();
#endif
	#ifdef CD32
	akiko_free();
#endif
	if (!no_gui)
		gui_exit();
#ifdef USE_SDL
	SDL_Quit();
#endif
	hardfile_reset();
#ifdef AUTOCONFIG
	expansion_cleanup();
#endif
#ifdef FILESYS
	filesys_cleanup();
#endif
#ifdef BSDSOCKET
	bsdlib_reset();
#endif
	device_func_reset();
#ifdef WITH_LUA
	uae_lua_free();
#endif
	memory_cleanup();
	//TODO
	//free_shm();
	cfgfile_addcfgparam(nullptr);
	machdep_free();
}

void start_program()
{
#ifdef CAPSLOCK_DEBIAN_WORKAROUND
	char kbd_flags;
	// set capslock state based upon current "real" state
	ioctl(0, KDGKBLED, &kbd_flags);
	if ((kbd_flags & 07) & LED_CAP)
	{
	   // record capslock pressed
		inputdevice_do_keyboard(AK_CAPSLOCK, 1);
	}
#endif
	do_start_program();
}

void leave_program()
{
	do_leave_program();
}

void virtualdevice_init()
{
#ifdef AUTOCONFIG
	rtarea_setup();
#endif
#ifdef FILESYS
	rtarea_init();
	uaeres_install();
	hardfile_install();
#endif
#ifdef SCSIEMU
	scsi_reset();
	scsidev_install();
#endif
#ifdef SANA2
	netdev_install();
#endif
#ifdef UAESERIAL
	uaeserialdev_install();
#endif
#ifdef AUTOCONFIG
	expansion_init();
	emulib_install();
#endif
#ifdef FILESYS
	filesys_install();
#endif
#if defined (BSDSOCKET)
	bsdlib_install();
#endif
#ifdef WITH_UAENATIVE
	uaenative_install();
#endif
#ifdef WITH_TABLETLIBRARY
	tabletlib_install();
#endif
}

// In case of error, print the error code and close the application
void check_error_sdl(bool check, const char* message) {
	if (check) {
		cout << message << " " << SDL_GetError() << endl;
		SDL_Quit();
		exit(-1);
	}
}

static int real_main2 (int argc, TCHAR **argv)
{
	printf("Amiberry-SDL2 by Dimitris (MiDWaN) Panokostas\n");

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		abort();
	}

	sdlWindow = SDL_CreateWindow("Amiberry-SDL2 v2",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		0,
		0,
		SDL_WINDOW_FULLSCREEN_DESKTOP);
	check_error_sdl(sdlWindow == nullptr, "Unable to create window");
		
	renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	check_error_sdl(renderer == nullptr, "Unable to create a renderer");

	if (SDL_GetWindowDisplayMode(sdlWindow, &sdlMode) != 0)
	{
		SDL_Log("Could not get information about SDL Mode! SDL_Error: %s\n", SDL_GetError());
	}
	
	if (SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1") != SDL_TRUE)
		SDL_Log("SDL could not grab the keyboard");

	set_config_changed();
	if (restart_config[0]) {
		default_prefs(&currprefs, true, 0);
		fixup_prefs(&currprefs, true);
	}

	if (!graphics_setup()) {
		abort();
	}

	if (restart_config[0])
		parse_cmdline_and_init_file(argc, argv);
	else
		currprefs = changed_prefs;

	if (!machdep_init()) {
		restart_program = 0;
		return -1;
	}

	if (!setup_sound()) {
		write_log(_T("Sound driver unavailable: Sound output disabled\n"));
		currprefs.produce_sound = 0;
	}

	inputdevice_init();

	changed_prefs = currprefs;
	no_gui = !currprefs.start_gui;
	if (restart_program == 2)
		no_gui = true;
	else if (restart_program == 3)
		no_gui = false;
	restart_program = 0;
	if (!no_gui) {
		int err = gui_init();
		currprefs = changed_prefs;
		set_config_changed();
		if (err == -1) {
			write_log(_T("Failed to initialize the GUI\n"));
			return -1;
		}
		else if (err == -2) {
			return 1;
		}
	}

	memset(&gui_data, 0, sizeof gui_data);
	gui_data.cd = -1;
	gui_data.hd = -1;
	gui_data.md = -1;

#ifdef NATMEM_OFFSET
	if (!init_shm()) {
		if (currprefs.start_gui)
			uae_restart(-1, NULL);
		return 0;
}
#endif
#ifdef WITH_LUA
	uae_lua_init();
#endif
#ifdef PICASSO96
	picasso_reset();
#endif

	fixup_prefs(&currprefs, true);
#ifdef RETROPLATFORM
	rp_fixup_options(&currprefs);
#endif
	changed_prefs = currprefs;
	target_run();
  /* force sound settings change */
	currprefs.produce_sound = 0;

	//savestate_init();
	keybuf_init(); /* Must come after init_joystick */

	memory_hardreset(2);
	memory_reset();

#ifdef AUTOCONFIG
	native2amiga_install();
#endif
	custom_init(); /* Must come after memory_init */
#ifdef SERIAL_PORT
	serial_init();
#endif
	DISK_init();
#ifdef WITH_PPC
	uae_ppc_reset(true);
#endif

	reset_frame_rate_hack();
	init_m68k(); /* must come after reset_frame_rate_hack (); */

	gui_update();

	if (graphics_init(true)) {

		if (!init_audio()) {
			if (sound_available && currprefs.produce_sound > 1) {
				write_log(_T("Sound driver unavailable: Sound output disabled\n"));
			}
			currprefs.produce_sound = 0;
		}
		start_program();
	}
	return 0;
}

void real_main(int argc, TCHAR **argv)
{
	restart_program = 1;

	fetch_configurationpath(restart_config, sizeof(restart_config) / sizeof(TCHAR));
	_tcscat(restart_config, OPTIONSFILENAME);
	default_config = 1;

	while (restart_program) {
		int ret;
		changed_prefs = currprefs;
		ret = real_main2(argc, argv);
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

#ifdef SINGLEFILE
uae_u8 singlefile_config[50000] = { "_CONFIG_STARTS_HERE" };
uae_u8 singlefile_data[1500000] = { "_DATA_STARTS_HERE" };
#endif
