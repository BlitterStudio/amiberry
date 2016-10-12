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
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"

/* Commonly used autoconfig strings */

uaecptr EXPANSION_explibname, EXPANSION_doslibname, EXPANSION_uaeversion;
uaecptr EXPANSION_uaedevname, EXPANSION_explibbase = 0;
uaecptr EXPANSION_bootcode, EXPANSION_nullfunc;


/* ROM tag area memory access */

uae_u8 *rtarea = 0;
uaecptr rtarea_base = RTAREA_DEFAULT;

static uae_u32 REGPARAM3 rtarea_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 rtarea_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 rtarea_bget (uaecptr) REGPARAM;
static void REGPARAM3 rtarea_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 rtarea_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 rtarea_bput (uaecptr, uae_u32) REGPARAM;
static uae_u8 *REGPARAM3 rtarea_xlate (uaecptr) REGPARAM;
static int REGPARAM3 rtarea_check (uaecptr addr, uae_u32 size) REGPARAM;

addrbank rtarea_bank = {
  rtarea_lget, rtarea_wget, rtarea_bget,
  rtarea_lput, rtarea_wput, rtarea_bput,
  rtarea_xlate, rtarea_check, NULL, _T("UAE Boot ROM"),
  rtarea_lget, rtarea_wget, ABFLAG_ROMIN
};

static uae_u8 *REGPARAM2 rtarea_xlate (uaecptr addr)
{
  addr &= 0xFFFF;
  return rtarea + addr;
}

static int REGPARAM2 rtarea_check (uaecptr addr, uae_u32 size)
{
  addr &= 0xFFFF;
  return (addr + size) <= 0xFFFF;
}

static uae_u32 REGPARAM2 rtarea_lget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  addr &= 0xFFFF;
  return (uae_u32)(rtarea_wget (addr) << 16) + rtarea_wget (addr+2);
}

static uae_u32 REGPARAM2 rtarea_wget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  addr &= 0xFFFF;
  return (rtarea[addr]<<8) + rtarea[addr+1];
}

static uae_u32 REGPARAM2 rtarea_bget (uaecptr addr)
{
#ifdef JIT
  special_mem |= S_READ;
#endif
  addr &= 0xFFFF;
  return rtarea[addr];
}

#define RTAREA_WRITEOFFSET 0xfff0

static void REGPARAM2 rtarea_bput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
	addr &= 0xffff;
	if (addr < RTAREA_WRITEOFFSET)
		return;
	rtarea[addr] = value;
}

static void REGPARAM2 rtarea_wput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
	addr &= 0xffff;
	if (addr < RTAREA_WRITEOFFSET)
		return;
	rtarea_bput (addr, value >> 8);
	rtarea_bput (addr + 1, value & 0xff);
}

static void REGPARAM2 rtarea_lput (uaecptr addr, uae_u32 value)
{
#ifdef JIT
  special_mem |= S_WRITE;
#endif
	addr &= 0xffff;
	if (addr < RTAREA_WRITEOFFSET)
		return;
	rtarea_wput (addr, value >> 16);
	rtarea_wput (addr + 2, value & 0xffff);
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
	rtarea[rt_addr++] = data;
}

void dw (uae_u16 data)
{
	rtarea[rt_addr++] = (uae_u8)(data >> 8);
	rtarea[rt_addr++] = (uae_u8)data;
}

void dl (uae_u32 data)
{
	rtarea[rt_addr++] = data >> 24;
	rtarea[rt_addr++] = data >> 16;
	rtarea[rt_addr++] = data >> 8;
	rtarea[rt_addr++] = data;
}

uae_u8 dbg (uaecptr addr)
{
  addr -= rtarea_base;
  return rtarea[addr];
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
	strcpy ((uae_char*)rtarea + rt_straddr, str);
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
	rtarea[rt_straddr] = len - 2;
	strcpy ((uae_char*)rtarea + rt_straddr + 1, str);
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

static uae_u32 REGPARAM2 nullfunc(TrapContext *context)
{
  write_log (_T("Null function called\n"));
  return 0;
}

static uae_u32 REGPARAM2 getchipmemsize (TrapContext *context)
{
	m68k_dreg (regs, 1) = 0;
	m68k_areg (regs, 1) = 0;
  return chipmem_bank.allocated;
}

static uae_u32 REGPARAM2 uae_puts (TrapContext *context)
{
  puts ((char *)get_real_address (m68k_areg (regs, 0)));
  return 0;
}

void rtarea_init_mem (void)
{
  if(rtarea != 0)
    mapped_free(rtarea);

  rtarea = mapped_malloc (RTAREA_SIZE, _T("rtarea"));
  if (!rtarea) {
  	write_log (_T("virtual memory exhausted (rtarea)!\n"));
		abort ();
  }
  memset(rtarea, 0, RTAREA_SIZE);
  rtarea_bank.baseaddr = rtarea;
}

void rtarea_init (void)
{
  uae_u32 a;
  TCHAR uaever[100];

  rt_straddr = 0xFF00 - 2;
  rt_addr = 0;

  init_traps ();

  rtarea_init_mem ();
  memset (rtarea, 0, RTAREA_SIZE);

  _stprintf (uaever, _T("uae-%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);

  EXPANSION_uaeversion = ds (uaever);
  EXPANSION_explibname = ds (_T("expansion.library"));
  EXPANSION_doslibname = ds (_T("dos.library"));
  EXPANSION_uaedevname = ds (_T("uae.device"));

  deftrap (NULL); /* Generic emulator trap */

  dw (0);
  dw (0);

  a = here();
  /* Dummy trap - removing this breaks the filesys emulation. */
  org (rtarea_base + 0xFF00);
  calltrap (deftrap2 (nullfunc, TRAPFLAG_NO_RETVAL, _T("")));

  org (rtarea_base + 0xFF80);
  calltrap (deftrapres (getchipmemsize, TRAPFLAG_DORET, _T("getchipmemsize")));

  org (rtarea_base + 0xFF10);
  calltrap (deftrapres (uae_puts, TRAPFLAG_NO_RETVAL, _T("uae_puts")));
  dw (RTS);

  org (a);
    
#ifdef FILESYS
  filesys_install_code ();
#endif

	uae_boot_rom_size = here () - rtarea_base;
	if (uae_boot_rom_size >= RTAREA_TRAPS) {
		write_log (_T("RTAREA_TRAPS needs to be increased!"));
		abort ();
	}

#ifdef PICASSO96
	uaegfx_install_code (rtarea_base + RTAREA_RTG);
#endif

	org (RTAREA_TRAPS | rtarea_base);
  init_extended_traps();
}

volatile int uae_int_requested = 0;

void set_uae_int_flag (void)
{
	rtarea[RTAREA_INT] = uae_int_requested & 1;
}

void rtarea_setup(void)
{
  uae_int_requested = 0;
  uaecptr base = need_uae_boot_rom ();
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
