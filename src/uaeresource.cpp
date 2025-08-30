/*
* UAE - The Un*x Amiga Emulator
*
* uae.resource
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "traps.h"
#include "autoconf.h"
#include "execlib.h"

static uaecptr res_init, res_name, res_id, base;

static uae_u32 REGPARAM2 res_getfunc (TrapContext *ctx)
{
	uaecptr funcname = trap_get_areg(ctx, 0);
	uae_char tmp[256];
	uae_u32 p;
	TCHAR *s;

	if (funcname == 0)
		return 0;
	trap_get_string(ctx, tmp, funcname, sizeof tmp);
	s = au(tmp);
	p = find_trap(s);
	xfree (s);
	return p;
}

static uae_u32 REGPARAM2 res_initcode (TrapContext *ctx)
{
	uaecptr rb;
	base = trap_get_dreg (ctx, 0);
	rb = base + SIZEOF_LIBRARY;
	trap_put_word(ctx, rb + 0, UAEMAJOR);
	trap_put_word(ctx, rb + 2, UAEMINOR);
	trap_put_word(ctx, rb + 4, UAESUBREV);
	trap_put_word(ctx, rb + 6, 0);
	trap_put_long(ctx, rb + 8, rtarea_base);
	return base;
}

uaecptr uaeres_startup (TrapContext *ctx, uaecptr resaddr)
{
	trap_put_word(ctx, resaddr + 0x0, 0x4AFC);
	trap_put_long(ctx, resaddr + 0x2, resaddr);
	trap_put_long(ctx, resaddr + 0x6, resaddr + 0x1A); /* Continue scan here */
	trap_put_word(ctx, resaddr + 0xA, 0x8101); /* RTF_AUTOINIT|RTF_COLDSTART; Version 1 */
	trap_put_word(ctx, resaddr + 0xC, 0x0805); /* NT_DEVICE; pri 05 */
	trap_put_long(ctx, resaddr + 0xE, res_name);
	trap_put_long(ctx, resaddr + 0x12, res_id);
	trap_put_long(ctx, resaddr + 0x16, res_init);
	resaddr += 0x1A;
	return resaddr;
}

void uaeres_install (void)
{
	uae_u32 functable, datatable;
	uae_u32 initcode, getfunc;
	TCHAR tmp[100];

	_sntprintf (tmp, sizeof tmp, _T("UAE resource %d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);
	res_name = ds (_T("uae.resource"));
	res_id = ds (tmp);

	/* initcode */
	initcode = here ();
	calltrap (deftrap (res_initcode)); dw (RTS);
	/* getfunc */
	getfunc = here ();
	calltrap (deftrap (res_getfunc)); dw (RTS);

	/* FuncTable */
	functable = here ();
	dl (getfunc); /* getfunc */
	dl (0xFFFFFFFF); /* end of table */

	/* DataTable */
	datatable = here ();
	dw (0xE000); /* INITBYTE */
	dw (0x0008); /* LN_TYPE */
	dw (0x0800); /* NT_RESOURCE */
	dw (0xC000); /* INITLONG */
	dw (0x000A); /* LN_NAME */
	dl (res_name);
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
	dl (res_id);
	dw (0x0000); /* end of table */

	res_init = here ();
	dl (SIZEOF_LIBRARY + 16); /* size of device base */
	dl (functable);
	dl (datatable);
	dl (initcode);
}
