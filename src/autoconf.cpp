 /*
  * UAE - The Un*x Amiga Emulator
  *
  * AutoConfig devices
  *
  * Copyright 1995, 1996 Bernd Schmidt
  * Copyright 1996 Ed Hanway
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "inputdevice.h"

/* Commonly used autoconfig strings */

uaecptr EXPANSION_explibname, EXPANSION_doslibname, EXPANSION_uaeversion;
uaecptr EXPANSION_uaedevname, EXPANSION_explibbase = 0;
uaecptr EXPANSION_bootcode, EXPANSION_nullfunc;

/* ROM tag area memory access */

uaecptr rtarea_base = RTAREA_DEFAULT;

DECLARE_MEMORY_FUNCTIONS(rtarea);
addrbank rtarea_bank = {
  rtarea_lget, rtarea_wget, rtarea_bget,
  rtarea_lput, rtarea_wput, rtarea_bput,
	rtarea_xlate, rtarea_check, NULL, _T("rtarea"), _T("UAE Boot ROM"),
	rtarea_lget, rtarea_wget,
	ABFLAG_ROMIN, S_READ, S_WRITE
};

#define RTAREA_WRITEOFFSET 0xfff0

static uae_u8 *REGPARAM2 rtarea_xlate (uaecptr addr)
{
  addr &= 0xFFFF;
	return rtarea_bank.baseaddr + addr;
}

static int REGPARAM2 rtarea_check (uaecptr addr, uae_u32 size)
{
  addr &= 0xFFFF;
  return (addr + size) <= 0xFFFF;
}

static uae_u32 REGPARAM2 rtarea_lget (uaecptr addr)
{
  addr &= 0xFFFF;
	if (addr & 1)
		return 0;
	if (addr >= 0xfffd)
		return 0;
	return (rtarea_bank.baseaddr[addr + 0] << 24) | (rtarea_bank.baseaddr[addr + 1] << 16) |
		(rtarea_bank.baseaddr[addr + 2] << 8) | (rtarea_bank.baseaddr[addr + 3] << 0);
}
static uae_u32 REGPARAM2 rtarea_wget (uaecptr addr)
{
  addr &= 0xFFFF;
	if (addr & 1)
		return 0;

	uaecptr addr2 = addr - RTAREA_TRAP_STATUS;

	return (rtarea_bank.baseaddr[addr] << 8) + rtarea_bank.baseaddr[addr + 1];
}
static uae_u32 REGPARAM2 rtarea_bget (uaecptr addr)
{
  addr &= 0xFFFF;

  if (addr == RTAREA_INTREQ + 0) {
		rtarea_bank.baseaddr[addr] = atomic_bit_test_and_reset(&uae_int_requested, 0);
		//write_log(rtarea_bank.baseaddr[addr] ? _T("+") : _T("-"));
	} else if (addr == RTAREA_INTREQ + 1) {
		rtarea_bank.baseaddr[addr] = false;
	} else if (addr == RTAREA_INTREQ + 2) {
			rtarea_bank.baseaddr[addr] = 0;
	}	
  return rtarea_bank.baseaddr[addr];
}

static bool rtarea_write(uaecptr addr)
{
	if (addr >= RTAREA_WRITEOFFSET)
		return true;
	if (addr >= RTAREA_SYSBASE && addr < RTAREA_SYSBASE + 4)
		return true;
  return false;
}

static void REGPARAM2 rtarea_bput (uaecptr addr, uae_u32 value)
{
	addr &= 0xffff;
	if (!rtarea_write(addr))
		return;
	rtarea_bank.baseaddr[addr] = value;
	if (addr == RTAREA_INTREQ + 3) {
		mousehack_wakeup();
	}
}

static void REGPARAM2 rtarea_wput (uaecptr addr, uae_u32 value)
{
	addr &= 0xffff;
	value &= 0xffff;

	if (addr & 1)
		return;

	if (!rtarea_write(addr))
		return;

	uaecptr addr2 = addr - RTAREA_TRAP_STATUS;

	rtarea_bank.baseaddr[addr + 0] = value >> 8;
	rtarea_bank.baseaddr[addr + 1] = (uae_u8)value;
}

static void REGPARAM2 rtarea_lput (uaecptr addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr & 1)
		return;
	if (addr >= 0xfffd)
		return;
	if (!rtarea_write(addr))
		return;
	rtarea_bank.baseaddr[addr + 0] = value >> 24;
	rtarea_bank.baseaddr[addr + 1] = value >> 16;
	rtarea_bank.baseaddr[addr + 2] = value >> 8;
	rtarea_bank.baseaddr[addr + 3] = value >> 0;
}

void rtarea_reset(void)
{
	uae_u8 *p = rtarea_bank.baseaddr;
	if (p) {
		memset(p + RTAREA_TRAP_DATA, 0, RTAREA_TRAP_DATA_SLOT_SIZE * (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM));
		memset(p + RTAREA_TRAP_STATUS, 0, RTAREA_TRAP_STATUS_SIZE * (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM));
		memset(p + RTAREA_HEARTBEAT, 0, 0x10000 - RTAREA_HEARTBEAT);
		memset(p + RTAREA_VARIABLES, 0, RTAREA_VARIABLES_SIZE);
  }
}

/* some quick & dirty code to fill in the rt area and save me a lot of
 * scratch paper
 */

static int rt_addr;
static int rt_straddr;

uae_u32 addr (int ptr)
{
  return (uae_u32)ptr + rtarea_base;
}

void db (uae_u8 data)
{
	rtarea_bank.baseaddr[rt_addr++] = data;
}

void dw (uae_u16 data)
{
	rtarea_bank.baseaddr[rt_addr++] = (uae_u8)(data >> 8);
	rtarea_bank.baseaddr[rt_addr++] = (uae_u8)data;
}

void dl (uae_u32 data)
{
	rtarea_bank.baseaddr[rt_addr++] = data >> 24;
	rtarea_bank.baseaddr[rt_addr++] = data >> 16;
	rtarea_bank.baseaddr[rt_addr++] = data >> 8;
	rtarea_bank.baseaddr[rt_addr++] = data;
}

uae_u8 dbg (uaecptr addr)
{
  addr -= rtarea_base;
	return rtarea_bank.baseaddr[addr];
}

/* store strings starting at the end of the rt area and working
 * backward.  store pointer at current address
 */

uae_u32 ds_ansi (const uae_char *str)
{
  int len;

  if (!str)
  	return addr (rt_straddr);
  len = strlen (str) + 1;
  rt_straddr -= len;
	strcpy ((uae_char*)rtarea_bank.baseaddr + rt_straddr, str);
  return addr (rt_straddr);
}

uae_u32 ds (const TCHAR *str)
{
	char *s = ua (str);
	uae_u32 v = ds_ansi (s);
	xfree (s);
	return v;
}

uae_u32 ds_bstr_ansi (const uae_char *str)
{
	int len;
 
	len = strlen (str) + 2;
	rt_straddr -= len;
	while (rt_straddr & 3)
		rt_straddr--;
	rtarea_bank.baseaddr[rt_straddr] = len - 2;
	strcpy ((uae_char*)rtarea_bank.baseaddr + rt_straddr + 1, str);
	return addr (rt_straddr) >> 2;
}

void calltrap (uae_u32 n)
{
  dw (0xA000 + n);
}

void org (uae_u32 a)
{
	if ( ((a & 0xffff0000) != 0x00f00000) && ((a & 0xffff0000) != rtarea_base) )
		write_log (_T("ORG: corrupt address! %08X"), a);
  rt_addr = a & 0xffff;
}

uae_u32 here (void)
{
  return addr (rt_addr);
}

void align (int b)
{
  rt_addr = (rt_addr + b - 1) & ~(b - 1);
}

static uae_u32 REGPARAM2 nullfunc(TrapContext *ctx)
{
  write_log (_T("Null function called\n"));
  return 0;
}

static uae_u32 REGPARAM2 getchipmemsize (TrapContext *ctx)
{
	trap_set_dreg(ctx, 1, 0);
	trap_set_areg(ctx, 1, 0);
  return chipmem_bank.allocated_size;
}

static uae_u32 REGPARAM2 uae_puts (TrapContext *ctx)
{
	uae_char buf[MAX_DPATH];
	trap_get_string(ctx, buf, trap_get_areg(ctx, 0), sizeof (uae_char));
	write_log(_T("%s"), buf);
  return 0;
}

void rtarea_init_mem (void)
{
	rtarea_bank.reserved_size = RTAREA_SIZE;
	rtarea_bank.start = rtarea_base;
	if (!mapped_malloc (&rtarea_bank)) {
  	write_log (_T("virtual memory exhausted (rtarea)!\n"));
    target_startup_msg(_T("Internal error"), _T("Virtual memory exhausted (rtarea)."));
    uae_restart(1, NULL);
    return;
  }
}

void rtarea_free(void)
{
	mapped_free(&rtarea_bank);
}

void rtarea_init (void)
{
  uae_u32 a;
  TCHAR uaever[100];

  rt_straddr = 0xFF00 - 2;
  rt_addr = 0;

  init_traps ();

  rtarea_init_mem ();
	memset (rtarea_bank.baseaddr, 0, RTAREA_SIZE);

  _stprintf (uaever, _T("uae-%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);

  EXPANSION_uaeversion = ds (uaever);
  EXPANSION_explibname = ds (_T("expansion.library"));
  EXPANSION_doslibname = ds (_T("dos.library"));
  EXPANSION_uaedevname = ds (_T("uae.device"));

  dw (0);
  dw (0);

#ifdef FILESYS
  filesys_install_code ();
#endif

  deftrap (NULL); /* Generic emulator trap */

  a = here();
  /* Dummy trap - removing this breaks the filesys emulation. */
  org (rtarea_base + 0xFF00);
  calltrap (deftrap2 (nullfunc, TRAPFLAG_NO_RETVAL, _T("")));

  org (rtarea_base + 0xFF80);
  calltrap (deftrapres (getchipmemsize, TRAPFLAG_DORET, _T("getchipmemsize")));
	dw(RTS);

  org (rtarea_base + 0xFF10);
  calltrap (deftrapres (uae_puts, TRAPFLAG_NO_RETVAL, _T("uae_puts")));
  dw (RTS);

  org (a);
    
	uae_boot_rom_size = here () - rtarea_base;
	if (uae_boot_rom_size >= RTAREA_TRAPS) {
		write_log (_T("RTAREA_TRAPS needs to be increased!"));
    target_startup_msg(_T("Internal error"), _T("RTAREA_TRAPS needs to be increased."));
    uae_restart(1, NULL);
    return;
	}

#ifdef PICASSO96
	uaegfx_install_code (rtarea_base + RTAREA_RTG);
#endif

	org (RTAREA_TRAPS | rtarea_base);
  init_extended_traps();
}

volatile uae_atomic uae_int_requested = 0;

void rtarea_setup(void)
{
  uaecptr base = need_uae_boot_rom (&currprefs);
  if (base) {
  	write_log (_T("RTAREA located at %08X\n"), base);
	  rtarea_base = base;
  }
}

uaecptr makedatatable (uaecptr resid, uaecptr resname, uae_u8 type, uae_s8 priority, uae_u16 ver, uae_u16 rev)
{
  uaecptr datatable = here ();

  dw (0xE000);		/* INITBYTE */
  dw (0x0008);		/* LN_TYPE */
  dw (type << 8);
  dw (0xE000);		/* INITBYTE */
  dw (0x0009);		/* LN_PRI */
  dw (priority << 8);
  dw (0xC000);		/* INITLONG */
  dw (0x000A);		/* LN_NAME */
  dl (resname);
  dw (0xE000);		/* INITBYTE */
  dw (0x000E);		/* LIB_FLAGS */
  dw (0x0600);		/* LIBF_SUMUSED | LIBF_CHANGED */
  dw (0xD000);		/* INITWORD */
  dw (0x0014);		/* LIB_VERSION */
  dw (ver);
  dw (0xD000);		/* INITWORD */
  dw (0x0016);		/* LIB_REVISION */
  dw (rev);
  dw (0xC000);		/* INITLONG */
  dw (0x0018);		/* LIB_IDSTRING */
  dl (resid);
  dw (0x0000);		/* end of table */
  return datatable;
}
