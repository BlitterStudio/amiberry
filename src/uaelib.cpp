/*
 * UAE - The U*nix Amiga Emulator
 *
 * UAE Library v0.1
 *
 * (c) 1996 Tauno Taipaleenmaki <tataipal@raita.oulu.fi>
 *
 * Change UAE parameters and other stuff from inside the emulation.
 */

#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
//#include "newcpu.h"
#include "autoconf.h"
#include "disk.h"
#include "gensound.h"
#include "picasso96.h"
#include "filesys.h"

/*
 * Returns UAE Version
 */
static uae_u32 emulib_GetVersion(void)
{
	return version;
}

/*
 * Resets your amiga
 */
static uae_u32 emulib_HardReset(void)
{
	uae_reset(1, 1);
	return 0;
}

static uae_u32 emulib_Reset(void)
{
	uae_reset(0, 0);
	return 0;
}

/*
 * Enables SOUND
 */
static uae_u32 emulib_EnableSound(uae_u32 val)
{
	if (!sound_available || currprefs.produce_sound == 2)
		return 0;

	currprefs.produce_sound = val;
	return 1;
}

/*
 * Enables FAKE JOYSTICK
 */
static uae_u32 emulib_EnableJoystick(uae_u32 val)
{
	currprefs.jports[0].id = val & 255;
	currprefs.jports[1].id = (val >> 8) & 255;
	return 1;
}

/*
 * Sets the framerate
 */
static uae_u32 emulib_SetFrameRate(uae_u32 val)
{
	if (val == 0)
		return 0;
	if (val > 20)
		return 0;
	currprefs.gfx_framerate = val;
	return 1;
}

/*
 * Changes keyboard language settings
 */
static uae_u32 emulib_ChangeLanguage(uae_u32 which)
{
	if (which > 6)
		return 0;
	switch (which)
	{
	case 0:
		currprefs.keyboard_lang = KBD_LANG_US;
		break;
	case 1:
		currprefs.keyboard_lang = KBD_LANG_DK;
		break;
	case 2:
		currprefs.keyboard_lang = KBD_LANG_DE;
		break;
	case 3:
		currprefs.keyboard_lang = KBD_LANG_SE;
		break;
	case 4:
		currprefs.keyboard_lang = KBD_LANG_FR;
		break;
	case 5:
		currprefs.keyboard_lang = KBD_LANG_IT;
		break;
	case 6:
		currprefs.keyboard_lang = KBD_LANG_ES;
		break;
	default:
		break;
	}
	return 1;
}

/* The following ones don't work as we never realloc the arrays... */
/*
 * Changes chip memory size
 *  (reboots)
 */
static uae_u32 REGPARAM2 emulib_ChgCMemSize(TrapContext* ctx, uae_u32 memsize)
{
	if (memsize != 0x80000 && memsize != 0x100000 &&
		memsize != 0x200000)
	{
		memsize = 0x200000;
		write_log (_T("Unsupported chipmem size!\n"));
	}
	trap_set_dreg(ctx, 0, 0);

	changed_prefs.chipmem_size = memsize;
	uae_reset(1, 1);
	return 1;
}

/*
 * Changes slow memory size
 *  (reboots)
 */
static uae_u32 REGPARAM2 emulib_ChgSMemSize(TrapContext* ctx, uae_u32 memsize)
{
	if (memsize != 0x80000 && memsize != 0x100000 &&
		memsize != 0x180000 && memsize != 0x1C0000)
	{
		memsize = 0;
		write_log (_T("Unsupported bogomem size!\n"));
	}

	trap_set_dreg(ctx, 0, 0);
	changed_prefs.bogomem_size = memsize;
	uae_reset(1, 1);
	return 1;
}

/*
 * Changes fast memory size
 *  (reboots)
 */
static uae_u32 REGPARAM2 emulib_ChgFMemSize(TrapContext* ctx, uae_u32 memsize)
{
	if (memsize != 0x100000 && memsize != 0x200000 &&
		memsize != 0x400000 && memsize != 0x800000)
	{
		memsize = 0;
		write_log (_T("Unsupported fastmem size!\n"));
	}
	trap_set_dreg(ctx, 0, 0);
	changed_prefs.fastmem[0].size = memsize;
	uae_reset(1, 1);
	return 0;
}

/*
 * Inserts a disk
 */
static uae_u32 emulib_InsertDisk(TrapContext* ctx, uaecptr name, uae_u32 drive)
{
	char real_name[256];
	TCHAR* s;

	if (drive > 3)
		return 0;

	if (trap_get_string(ctx, real_name, name, sizeof real_name) >= sizeof real_name)
		return 0; /* ENAMETOOLONG */

	s = au(real_name);
	_tcscpy(changed_prefs.floppyslots[drive].df, s);
	xfree (s);

	return 1;
}

/*
 * Exits the emulator
 */
static uae_u32 emulib_ExitEmu(void)
{
	uae_quit();
	return 1;
}

/*
 * Gets UAE Configuration
 */
static uae_u32 emulib_GetUaeConfig(TrapContext* ctx, uaecptr place)
{
	trap_put_long(ctx, place, version);
	trap_put_long(ctx, place + 4, chipmem_bank.allocated_size);
	trap_put_long(ctx, place + 8, bogomem_bank.allocated_size);
	trap_put_long(ctx, place + 12, fastmem_bank[0].allocated_size);
	trap_put_long(ctx, place + 16, currprefs.gfx_framerate + 1);
	trap_put_long(ctx, place + 20, currprefs.produce_sound);
	trap_put_long(ctx, place + 24, currprefs.jports[0].id | (currprefs.jports[1].id << 8));
	trap_put_long(ctx, place + 28, currprefs.keyboard_lang);
	if (disk_empty(0))
		trap_put_byte(ctx, place + 32, 0);
	else
		trap_put_byte(ctx, place + 32, 1);
	if (disk_empty(1))
		trap_put_byte(ctx, place + 33, 0);
	else
		trap_put_byte(ctx, place + 33, 1);
	if (disk_empty(2))
		trap_put_byte(ctx, place + 34, 0);
	else
		trap_put_byte(ctx, place + 34, 1);
	if (disk_empty(3))
		trap_put_byte(ctx, place + 35, 0);
	else
		trap_put_byte(ctx, place + 35, 1);

	for (int i = 0; i < 4; i++)
	{
		char* s = ua(currprefs.floppyslots[i].df);
		trap_put_string(ctx, s, place + 36 + i * 256, 256);
		xfree (s);
	}
	return 1;
}

/*
 * Sets UAE Configuration
 *
 * NOT IMPLEMENTED YET
 */
static uae_u32 emulib_SetUaeConfig(uaecptr place)
{
	return 1;
}

/*
 * Gets the name of the disk in the given drive
 */
static uae_u32 emulib_GetDisk(TrapContext* ctx, uae_u32 drive, uaecptr name)
{
	if (drive > 3)
		return 0;

	char* n = ua(currprefs.floppyslots[drive].df);
	trap_put_string(ctx, (uae_u8*)n, name, 256);
	xfree(n);
	return 1;
}

#define CREATE_NATIVE_FUNC_PTR uae_u32 (* native_func)( uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, \
	 uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32)
#define SET_NATIVE_FUNC(x) native_func = (uae_u32 (*)(uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32, uae_u32))(x)
#define CALL_NATIVE_FUNC( d1,d2,d3,d4,d5,d6,d7,a1,a2,a3,a4,a5,a6) if(native_func) native_func( d1,d2,d3,d4,d5,d6,d7,a1,a2,a3,a4,a5,a6 )
/* A0 - Contains a ptr to the native .obj data.  This ptr is Amiga-based. */
/*      We simply find the first function in this .obj data, and execute it. */

static int native_dos_op(TrapContext* ctx, uae_u32 mode, uae_u32 p1, uae_u32 p2, uae_u32 p3)
{
	TCHAR tmp[MAX_DPATH];
	char* s;
	int v;

	if (mode)
		return -1;
	/* receive native path from lock
	 * p1 = dos.library:Lock, p2 = buffer, p3 = max buffer size 
	 */
	v = get_native_path(ctx, p1, tmp);
	if (v)
		return v;
	s = ua(tmp);
	trap_put_string(ctx, (uae_u8*)s, p2, p3);
	xfree (s);
	return 0;
}

static uae_u32 uaelib_demux_common(TrapContext* ctx, uae_u32 ARG0, uae_u32 ARG1, uae_u32 ARG2, uae_u32 ARG3,
                                   uae_u32 ARG4, uae_u32 ARG5)
{
	switch (ARG0)
	{
	case 0: return emulib_GetVersion();
	case 1: return emulib_GetUaeConfig(ctx, ARG1);
	case 2: return emulib_SetUaeConfig(ARG1);
	case 3: return emulib_HardReset();
	case 4: return emulib_Reset();
	case 5: return emulib_InsertDisk(ctx, ARG1, ARG2);
	case 6: return emulib_EnableSound(ARG1);
	case 7: return emulib_EnableJoystick(ARG1);
	case 8: return emulib_SetFrameRate(ARG1);
	case 9: return emulib_ChgCMemSize(ctx, ARG1);
	case 10: return emulib_ChgSMemSize(ctx, ARG1);
	case 11: return emulib_ChgFMemSize(ctx, ARG1);
	case 12: return emulib_ChangeLanguage(ARG1);
		/* The next call brings bad luck */
	case 13: return emulib_ExitEmu();
	case 14: return emulib_GetDisk(ctx, ARG1, ARG2);
	case 15: return 0;

	case 68: return 0;
	case 69: return 0;

	case 70: return 0; /* RESERVED. Something uses this.. */

	case 80:
		return 0xffffffff;
	case 81: return cfgfile_uaelib(ctx, ARG1, ARG2, ARG3, ARG4);
	case 82: return cfgfile_uaelib_modify(ctx, ARG1, ARG2, ARG3, ARG4, ARG5);
	case 83: return 0;
	case 85: return native_dos_op(ctx, ARG1, ARG2, ARG3, ARG4);
	case 86:
		if (valid_address(ARG1, 1))
		{
			uae_char tmp[MAX_DPATH];
			trap_get_string(ctx, tmp, ARG1, sizeof tmp);
			TCHAR* s = au(tmp);
			write_log (_T("DBG: %s\n"), s);
			xfree (s);
			return 1;
		}
		return 0;
	case 87:
		{
			uae_u32 d0, d1;
			d0 = emulib_target_getcpurate(ARG1, &d1);
			trap_set_dreg(ctx, 1, d1);
			return d0;
		}
	}
	return 0;
}

static uae_u32 REGPARAM2 uaelib_demux2(TrapContext* ctx)
{
#define ARG0 (trap_get_long(ctx, trap_get_areg(ctx, 7) + 4))
#define ARG1 (trap_get_long(ctx, trap_get_areg(ctx, 7) + 8))
#define ARG2 (trap_get_long(ctx, trap_get_areg(ctx, 7) + 12))
#define ARG3 (trap_get_long(ctx, trap_get_areg(ctx, 7) + 16))
#define ARG4 (trap_get_long(ctx, trap_get_areg(ctx, 7) + 20))
#define ARG5 (trap_get_long(ctx, trap_get_areg(ctx, 7) + 24))

#ifdef PICASSO96
	if (ARG0 >= 16 && ARG0 <= 39)
		return picasso_demux(ARG0, ctx);
#endif
	return uaelib_demux_common(ctx, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5);
}

static uae_u32 REGPARAM2 uaelib_demux(TrapContext* ctx)
{
	uae_u32 v;

	v = uaelib_demux2(ctx);
	return v;
}

/*
 * Installs the UAE LIBRARY
 */
void emulib_install(void)
{
	uaecptr a;
	if (!uae_boot_rom_type && !currprefs.uaeboard)
		return;
	a = here();
	org(rtarea_base + 0xFF60);
	calltrap(deftrapres (uaelib_demux, 0, _T("uaelib_demux")));
	dw(RTS);
	org(a);
}
