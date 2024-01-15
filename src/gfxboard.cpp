/*
* UAE - The Un*x Amiga Emulator
*
* Cirrus Logic based graphics board emulation
*
* Copyright 2013 Toni Wilen
*
*/

#define VRAMLOG 0
#define MEMLOGR 0
#define MEMLOGW 0
#define MEMLOGINDIRECT 0
#define REGDEBUG 0
#define MEMDEBUG 0
#define MEMDEBUGMASK 0x7fffff
#define MEMDEBUGTEST 0x3fc000
#define MEMDEBUGCLEAR 0
#define SPCDEBUG 0
#define PICASSOIV_DEBUG_IO 0

#if MEMLOGR
static bool memlogr = true;
static bool memlogw = true;
#endif

#define BYTESWAP_WORD -1
#define BYTESWAP_LONG 1

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "debug.h"
#include "custom.h"
#include "newcpu.h"
#include "picasso96.h"
#include "statusline.h"
#include "rommgr.h"
#include "zfile.h"
#include "gfxboard.h"
#include "rommgr.h"
#include "xwin.h"
#include "devices.h"

int gfxboard_get_index_from_id(int id)
{
	if (id == GFXBOARD_UAE_Z2)
		return GFXBOARD_UAE_Z2;
	if (id == GFXBOARD_UAE_Z3)
		return GFXBOARD_UAE_Z3;
	return 0;
}

int gfxboard_get_id_from_index(int index)
{
	if (index == GFXBOARD_UAE_Z2)
		return GFXBOARD_UAE_Z2;
	if (index == GFXBOARD_UAE_Z3)
		return GFXBOARD_UAE_Z3;
	return 0;
}

const TCHAR *gfxboard_get_name(int type)
{
	if (type == GFXBOARD_UAE_Z2)
		return _T("UAE Zorro II");
	if (type == GFXBOARD_UAE_Z3)
		return _T("UAE Zorro III (*)");
	return NULL;
}

bool gfxboard_set(int monid, bool rtg)
{
	bool r;
	struct amigadisplay *ad = &adisplays[monid];
	r = ad->picasso_on;
	if (rtg) {
		ad->picasso_requested_on = 1;
	} else {
		ad->picasso_requested_on = 0;
	}
	set_config_changed();
	return r;
}

const TCHAR *gfxboard_get_configname(int type)
{
	if (type == GFXBOARD_UAE_Z2)
		return _T("ZorroII");
	if (type == GFXBOARD_UAE_Z3)
		return _T("ZorroIII");
	return NULL;
}

int gfxboard_get_configtype(struct rtgboardconfig *rbc)
{
	int type = rbc->rtgmem_type;
	if (type == GFXBOARD_UAE_Z2)
		return 2;
	if (type == GFXBOARD_UAE_Z3)
		return 3;
	return 0;
}
