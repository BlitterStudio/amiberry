/*
* UAE - The Un*x Amiga Emulator
*
* tablet.library
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "traps.h"
#include "autoconf.h"
#include "execlib.h"
#include "tabletlibrary.h"

static uaecptr lib_init, lib_name, lib_id, base;
static uaecptr tablettags;
#define TAGS_SIZE (12 * 4)

static int tablet_x, tablet_y, tablet_resx, tablet_resy;
static int tablet_pressure, tablet_buttonbits, tablet_inproximity;
static int tablet_maxx, tablet_maxy, tablet_maxz;
static int ksversion;

void tabletlib_tablet (int x, int y, int z, int pressure, int maxpres, uae_u32 buttonbits, int inproximity, int ax, int ay, int az)
{
	tablet_x = x;
	tablet_y = y;
	tablet_pressure = pressure << 15;
	tablet_buttonbits = buttonbits;
	tablet_inproximity = inproximity;
}

void tabletlib_tablet_info (int maxx, int maxy, int maxz, int maxax, int maxay, int maxaz, int xres, int yres)
{
	tablet_maxx = maxx;
	tablet_maxy = maxy;
	tablet_resx = xres;
	tablet_resy = yres;
}

static void filltags (TrapContext *ctx, uaecptr tabletdata)
{
	uaecptr p = tablettags;
	if (!p)
		return;
	trap_put_word(ctx, tabletdata + 0, 0);
	trap_put_word(ctx, tabletdata + 2, 0);
	trap_put_long(ctx, tabletdata + 4, tablet_x);
	trap_put_long(ctx, tabletdata + 8, tablet_y);
	trap_put_long(ctx, tabletdata + 12, tablet_maxx);
	trap_put_long(ctx, tabletdata + 16, tablet_maxy);

	//write_log(_T("P=%08X BUT=%08X\n"), tablet_pressure, tablet_buttonbits);

	// pressure
	trap_put_long(ctx, p, 0x8003a000 + 6);
	p += 4;
	trap_put_long(ctx, p, tablet_pressure);
	p += 4;
	// buttonbits
	trap_put_long(ctx, p, 0x8003a000 + 7);
	p += 4;
	trap_put_long(ctx, p, tablet_buttonbits);
	p += 4;
	// resolutionx
	trap_put_long(ctx, p, 0x8003a000 + 9);
	p += 4;
	trap_put_long(ctx, p, tablet_resx);
	p += 4;
	// resolutiony
	trap_put_long(ctx, p, 0x8003a000 + 10);
	p += 4;
	trap_put_long(ctx, p, tablet_resy);
	p += 4;
	if (tablet_inproximity == 0) {
		// inproximity
		trap_put_long(ctx, p, 0x8003a000 + 8);
		p += 4;
		trap_put_long(ctx, p, 0);
		p += 4;
	}
	trap_put_long(ctx, p, 0);
}

static uae_u32 REGPARAM2 lib_initcode (TrapContext *ctx)
{
	base = trap_get_dreg(ctx, 0);
	tablettags = base + SIZEOF_LIBRARY;
	tablet_inproximity = -1;
	tablet_x = tablet_y = 0;
	tablet_buttonbits = tablet_pressure = 0;
	ksversion = trap_get_word(ctx, trap_get_areg(ctx, 6) + 20);
	return base;
}
static uae_u32 REGPARAM2 lib_openfunc (TrapContext *ctx)
{
	trap_put_word(ctx, trap_get_areg(ctx, 6) + 32, trap_get_word(ctx, trap_get_areg(ctx, 6) + 32) + 1);
	return trap_get_areg(ctx, 6);
}
static uae_u32 REGPARAM2 lib_closefunc (TrapContext *ctx)
{
	trap_put_word(ctx, trap_get_areg(ctx, 6) + 32, trap_get_word(ctx, trap_get_areg(ctx, 6) + 32) - 1);
	return 0;
}
static uae_u32 REGPARAM2 lib_expungefunc (TrapContext *context)
{
	return 0;
}

#define TAG_DONE   (0L)		/* terminates array of TagItems. ti_Data unused */
#define TAG_IGNORE (1L)		/* ignore this item, not end of array */
#define TAG_MORE   (2L)		/* ti_Data is pointer to another array of TagItems */
#define TAG_SKIP   (3L)		/* skip this and the next ti_Data items */

static uae_u32 REGPARAM2 lib_allocfunc (TrapContext *ctx)
{
	uae_u32 tags = trap_get_areg(ctx, 0);
	uae_u32 mem;

	trap_call_add_dreg(ctx, 0, 24);
	trap_call_add_dreg(ctx, 1, 65536 + 1);
	mem = trap_call_lib(ctx, trap_get_long(ctx, 4), -0xC6); /* AllocMem */
	if (!mem)
		return 0;
	for (;;) {
		uae_u32 t = trap_get_long(ctx, tags);
		if (t == TAG_DONE)
			break;
		if (t == TAG_SKIP) {
			tags += 8 + trap_get_long(ctx, tags + 4) * 8;
		} else if (t == TAG_MORE) {
			tags = trap_get_long(ctx, tags + 4);
		} else if (t == TAG_IGNORE) {
			tags += 8;
		} else {
			t -= 0x8003a000;
			// clear "unknown" tags
			if (t != 6 && t != 8)
				trap_put_long(ctx, tags, TAG_IGNORE);
			tags += 8;
		}
	}
	trap_put_long(ctx, mem + 20, tablettags);
	filltags(ctx, mem);
	return mem;
}
static uae_u32 REGPARAM2 lib_freefunc (TrapContext *ctx)
{
	trap_call_add_areg(ctx, 1, trap_get_areg(ctx, 0));
	trap_call_add_dreg(ctx, 0, 24);
	trap_call_lib(ctx, trap_get_long(ctx, 4), -0xD2);
	return 0;
}
static uae_u32 REGPARAM2 lib_dofunc (TrapContext *ctx)
{
	uaecptr im = trap_get_areg(ctx, 0);
	uaecptr td = trap_get_areg(ctx, 1);
	filltags(ctx, td);
	if (ksversion < 39)
		return 0;
	td = trap_get_long(ctx, im + 52);
	if (!td)
		return 0;
	return 1;
}
static uae_u32 REGPARAM2 lib_unkfunc (TrapContext *context)
{
	write_log (_T("tablet.library unknown function called\n"));
	return 0;
}

uaecptr tabletlib_startup(TrapContext *ctx, uaecptr resaddr)
{
	if (!currprefs.tablet_library)
		return resaddr;
	trap_put_word(ctx, resaddr + 0x0, 0x4AFC);
	trap_put_long(ctx, resaddr + 0x2, resaddr);
	trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	if (kickstart_version >= 37) {
		trap_put_long(ctx, resaddr + 0xA, 0x84270900 | AFTERDOS_PRI); /* RTF_AUTOINIT, RT_VERSION NT_LIBRARY, RT_PRI */
	} else {
		trap_put_long(ctx, resaddr + 0xA, 0x81270905); /* RTF_AUTOINIT, RT_VERSION NT_LIBRARY, RT_PRI */
	}
	trap_put_long(ctx, resaddr + 0xE, lib_name);
	trap_put_long(ctx, resaddr + 0x12, lib_id);
	trap_put_long(ctx, resaddr + 0x16, lib_init);
	resaddr += 0x1A;
	return resaddr;
}

void tabletlib_install (void)
{
	uae_u32 functable, datatable;
	uae_u32 initcode, openfunc, closefunc, expungefunc;
	uae_u32 allocfunc, freefunc, dofunc, unkfunc;
	TCHAR tmp[100];

	if (!currprefs.tablet_library)
		return;

	_stprintf (tmp, _T("UAE tablet.library %d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
	lib_name = ds (_T("tablet.library"));
	lib_id = ds (tmp);

	initcode = here ();
	calltrap (deftrap (lib_initcode)); dw (RTS);
	openfunc = here ();
	calltrap (deftrap (lib_openfunc)); dw (RTS);
	closefunc = here ();
	calltrap (deftrap (lib_closefunc)); dw (RTS);
	expungefunc = here ();
	calltrap (deftrap (lib_expungefunc)); dw (RTS);
	allocfunc = here ();
	calltrap (deftrap2 (lib_allocfunc, TRAPFLAG_EXTRA_STACK, _T("tablet_alloc"))); dw (RTS);
	freefunc = here ();
	calltrap (deftrap2 (lib_freefunc, TRAPFLAG_EXTRA_STACK, _T("tablet_free"))); dw (RTS);
	dofunc = here ();
	calltrap (deftrap (lib_dofunc)); dw (RTS);
	unkfunc = here ();
	calltrap (deftrap (lib_unkfunc)); dw (RTS);

	/* FuncTable */
	functable = here ();
	dl (openfunc);
	dl (closefunc);
	dl (expungefunc);
	dl (EXPANSION_nullfunc);
	dl (allocfunc);
	dl (freefunc);
	dl (dofunc);
	dl (0xFFFFFFFF); /* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000); /* INITBYTE */
	dw (0x0008); /* LN_TYPE */
	dw (0x0900); /* NT_LIBRARY */
	dw (0xC000); /* INITLONG */
	dw (0x000A); /* LN_NAME */
	dl (lib_name);
	dw (0xE000); /* INITBYTE */
	dw (0x000E); /* LIB_FLAGS */
	dw (0x0600); /* LIBF_SUMUSED | LIBF_CHANGED */
	dw (0xD000); /* INITWORD */
	dw (0x0014); /* LIB_VERSION */
	dw (UAEMAJOR);
	dw (0xD000); /* INITWORD */
	dw (0x0016); /* LIB_REVISION */
	dw (UAEMINOR);
	dw (0xC000); /* INITLONG */
	dw (0x0018); /* LIB_IDSTRING */
	dl (lib_id);
	dw (0x0000); /* end of table */

	lib_init = here ();
	dl (SIZEOF_LIBRARY + TAGS_SIZE); /* size of lib base */
	dl (functable);
	dl (datatable);
	dl (initcode);
}
