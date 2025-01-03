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

#define NEW_TRAP_DEBUG 0

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "autoconf.h"
#include "traps.h"
#include "debug.h"
#include "threaddep/thread.h"
#include "native2amiga.h"
#include "inputdevice.h"
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "devices.h"

/* Commonly used autoconfig strings */

uaecptr EXPANSION_explibname, EXPANSION_doslibname, EXPANSION_uaeversion;
uaecptr EXPANSION_uaedevname, EXPANSION_explibbase = 0;
uaecptr EXPANSION_bootcode, EXPANSION_nullfunc;

/* ROM tag area memory access */

uaecptr rtarea_base = RTAREA_DEFAULT;
uae_sem_t hardware_trap_event[RTAREA_TRAP_DATA_SIZE / RTAREA_TRAP_DATA_SLOT_SIZE];
uae_sem_t hardware_trap_event2[RTAREA_TRAP_DATA_SIZE / RTAREA_TRAP_DATA_SLOT_SIZE];

static uaecptr rt_trampoline_ptr, trap_entry;
static bool rtarea_write_enabled;
extern volatile uae_atomic hwtrap_waiting;
extern volatile int trap_mode;

DECLARE_MEMORY_FUNCTIONS(rtarea);
addrbank rtarea_bank = {
	rtarea_lget, rtarea_wget, rtarea_bget,
	rtarea_lput, rtarea_wput, rtarea_bput,
	rtarea_xlate, rtarea_check, NULL, _T("rtarea"), _T("UAE Boot ROM"),
	rtarea_lget, rtarea_wget,
	ABFLAG_ROMIN | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

#define MAX_ABSOLUTE_ROM_ADDRESS 1024

static int absolute_rom_address;
static uaecptr absolute_rom_addresses[MAX_ABSOLUTE_ROM_ADDRESS];
static uaecptr rombase_new;

static void hwtrap_check_int(void)
{
	if (currprefs.uaeboard < 2)
		return;
	if (hwtrap_waiting == 0) {
		atomic_and(&uae_int_requested, ~0x2000);
	} else {
		atomic_or(&uae_int_requested, 0x2000);
		set_special_exter(SPCFLAG_UAEINT);
	}
}

static bool istrapwait(void)
{
	for (int i = 0; i < RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM; i++) {
		uae_u8 *data = rtarea_bank.baseaddr + RTAREA_TRAP_DATA + i * RTAREA_TRAP_DATA_SLOT_SIZE;
		uae_u8 *status = rtarea_bank.baseaddr + RTAREA_TRAP_STATUS + i * RTAREA_TRAP_STATUS_SIZE;
		if (get_long_host(data + RTAREA_TRAP_DATA_TASKWAIT) && status[3] && status[2] >= 0x80) {
			return true;
		}
	}
	return false;
}

static bool rethink_traps2(void)
{
	if (currprefs.uaeboard < 2)
		return false;
	if (istrapwait()) {
		atomic_or(&uae_int_requested, 0x4000);
		set_special_exter(SPCFLAG_UAEINT);
		return true;
	} else {
		atomic_and(&uae_int_requested, ~0x4000);
		return false;
	}
}

static void rethink_traps(void)
{
	rethink_traps2();
}


#define RTAREA_WRITEOFFSET 0xfff0

static bool rtarea_trap_data(uaecptr addr)
{
	if (currprefs.uaeboard < 2)
		return false;
	if (addr >= RTAREA_TRAP_DATA && addr < RTAREA_TRAP_DATA + (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM) * RTAREA_TRAP_DATA_SLOT_SIZE)
		return true;
	return false;
}
static bool rtarea_trap_status(uaecptr addr)
{
	if (currprefs.uaeboard < 2)
		return false;
	if (addr >= RTAREA_TRAP_STATUS && addr < RTAREA_TRAP_STATUS + (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM) * RTAREA_TRAP_STATUS_SIZE)
		return true;
	return false;
}
static bool rtarea_trap_status_extra(uaecptr addr)
{
	if (currprefs.uaeboard < 2)
		return false;
	if (addr >= RTAREA_TRAP_STATUS + 0x100 && addr < RTAREA_TRAP_STATUS + 0x100 + (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM) * RTAREA_TRAP_STATUS_SIZE)
		return true;
	return false;
}

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

	if (rtarea_trap_status(addr)) {
		int trap_offset = addr2 & (RTAREA_TRAP_STATUS_SIZE - 1);
		int trap_slot = addr2 / RTAREA_TRAP_STATUS_SIZE;
		// lock attempt
		if (trap_offset == 2) {
			if (rtarea_bank.baseaddr[addr + 1] & 0x80) {
				return rtarea_bank.baseaddr[addr + 1];
			}
			rtarea_bank.baseaddr[addr + 1] |= 0x80;
			rtarea_bank.baseaddr[addr + 0] = 0;
			return 0;
		}
	}
	return (rtarea_bank.baseaddr[addr] << 8) + rtarea_bank.baseaddr[addr + 1];
}
static uae_u32 REGPARAM2 rtarea_bget (uaecptr addr)
{
	addr &= 0xFFFF;

	hwtrap_check_int();
	if (rtarea_trap_status(addr)) {
		uaecptr addr2 = addr - RTAREA_TRAP_STATUS;
		int trap_offset = addr2 & (RTAREA_TRAP_STATUS_SIZE - 1);
		int trap_slot = addr2 / RTAREA_TRAP_STATUS_SIZE;
		if (trap_offset == 0) {
			// 0 = busy wait, 1 = Wait()
			rtarea_bank.baseaddr[addr] = trap_mode == 1 ? 1 : 0;
		}
		uae_u8 v = rtarea_bank.baseaddr[addr];
#if NEW_TRAP_DEBUG
		write_log(_T("GET TRAP SLOT %d OFFSET %d: V=%02x\n"), trap_slot, trap_offset, v);
#endif
		return v;
	} else if (addr == RTAREA_INTREQ + 0) {
		rtarea_bank.baseaddr[addr] = atomic_bit_test_and_reset(&uae_int_requested, 0);
		//write_log(rtarea_bank.baseaddr[addr] ? _T("+") : _T("-"));
	} else if (addr == RTAREA_INTREQ + 1) {
		rtarea_bank.baseaddr[addr] = hwtrap_waiting != 0;
	} else if (addr == RTAREA_INTREQ + 2) {
		if (rethink_traps2()) {
			rtarea_bank.baseaddr[addr] = 1;
		} else {
			rtarea_bank.baseaddr[addr] = 0;
		}
	}
	return rtarea_bank.baseaddr[addr];
}

static bool rtarea_write(uaecptr addr)
{
	if (rtarea_write_enabled)
		return true;
	if (addr >= RTAREA_WRITEOFFSET)
		return true;
	if (addr >= RTAREA_SYSBASE && addr < RTAREA_SYSBASE + 4)
		return true;
	return rtarea_trap_data(addr) || rtarea_trap_status(addr) || rtarea_trap_status_extra(addr);
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
	if (!rtarea_trap_status(addr))
		return;
	addr -= RTAREA_TRAP_STATUS;
	int trap_offset = addr & (RTAREA_TRAP_STATUS_SIZE - 1);
	int trap_slot = addr / RTAREA_TRAP_STATUS_SIZE;
#if NEW_TRAP_DEBUG
	write_log(_T("PUT TRAP SLOT %d OFFSET %d: V=%02x\n"), trap_slot, trap_offset, (uae_u8)value);
#endif
	if (trap_offset == RTAREA_TRAP_STATUS_SECOND) {
		write_log(_T("TRAP SLOT %d PRI=%02x\n"), trap_slot, (uae_u8)value);
	}
	if (trap_offset == RTAREA_TRAP_STATUS_SECOND + 3) {
		uae_u8 v = (uae_u8)value;
		if (v != 0xff && v != 0xfd && v != 0x01 && v != 0x02 && v != 0x03 && v != 0x04)
			write_log(_T("TRAP SLOT %d STATUS = %02x\n"), trap_slot, v);
		if (v == 0xfd)
			atomic_dec(&hwtrap_waiting);
		if (v == 0x01)
			atomic_dec(&hwtrap_waiting);
		// 1 = interrupt ack
		// 2 = interrupt -> task ack
		// 3 = hwtrap_entry ack
		if (v == 0x01 || v == 0x02 || v == 0x03) {
			// signal call_hardware_trap_back_back()
			uae_sem_post(&hardware_trap_event[trap_slot]);
		}
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

	if (rtarea_trap_status(addr)) {
		int trap_offset = addr2 & (RTAREA_TRAP_STATUS_SIZE - 1);
		int trap_slot = addr2 / RTAREA_TRAP_STATUS_SIZE;
		if (trap_offset == 2) {
			value = 0;
			if ((rtarea_bank.baseaddr[addr + 1] & 0x80) == 0) {
				write_log(_T("TRAP SLOT %d unlock without allocation!\n"), trap_slot);
			}
			rtarea_bank.baseaddr[addr + 0] = value >> 8;
			rtarea_bank.baseaddr[addr + 1] = (uae_u8)value;
			return;
		}
	}
	
	rtarea_bank.baseaddr[addr + 0] = value >> 8;
	rtarea_bank.baseaddr[addr + 1] = (uae_u8)value;

	if (rtarea_trap_status(addr)) {
		int trap_offset = addr2 & (RTAREA_TRAP_STATUS_SIZE - 1);
		int trap_slot = addr2 / RTAREA_TRAP_STATUS_SIZE;

#if NEW_TRAP_DEBUG
		write_log(_T("PUT TRAP SLOT %d OFFSET %d: V=%04x\n"), trap_slot, trap_offset, (uae_u16)value);
#endif
		if (trap_offset == 0) {
#if NEW_TRAP_DEBUG
			write_log(_T("TRAP SLOT %d NUM=%d\n"), trap_slot, value);
#endif
			call_hardware_trap(rtarea_bank.baseaddr, rtarea_base, trap_slot);
		}
	}
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

	if (rtarea_trap_status(addr)) {
		addr -= RTAREA_TRAP_STATUS;
		int trap_offset = addr & (RTAREA_TRAP_STATUS_SIZE - 1);
		int trap_slot = addr / RTAREA_TRAP_STATUS_SIZE;
#if NEW_TRAP_DEBUG
		write_log(_T("PUT TRAP SLOT %d OFFSET %d: V=%08x\n"), trap_slot, trap_offset, value);
#endif
	}
}

static int rt_addr;
static int rt_straddr;
static int rt_addr_restart;
static bool rt_addr_reset;

void rtarea_reset(void)
{
	uae_u8 *p = rtarea_bank.baseaddr;
	if (p) {
		memset(p + RTAREA_TRAP_DATA, 0, RTAREA_TRAP_DATA_SLOT_SIZE * (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM));
		memset(p + RTAREA_TRAP_STATUS, 0, RTAREA_TRAP_STATUS_SIZE * (RTAREA_TRAP_DATA_NUM + RTAREA_TRAP_DATA_SEND_NUM));
		memset(p + RTAREA_HEARTBEAT, 0, 0x10000 - RTAREA_HEARTBEAT);
		memset(p + RTAREA_VARIABLES, 0, RTAREA_VARIABLES_SIZE);
	}
	if (rt_addr_reset) {
		rt_addr_reset = false;
		rt_addr_restart = rt_addr;
	}
	rt_addr = rt_addr_restart;
	trap_reset();
	absolute_rom_address = 0;
}

/* some quick & dirty code to fill in the rt area and save me a lot of
* scratch paper
*/

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

void df(uae_u8 b, int len)
{
	memset(&rtarea_bank.baseaddr[rt_addr], b, len);
	rt_addr += len;
}

uae_u8 dbg (uaecptr addr)
{
	addr &= 0xffff;
	return rtarea_bank.baseaddr[addr];
}

/* store strings starting at the end of the rt area and working
* backward.  store pointer at current address
*/

uae_u32 ds_ansi(const uae_char *str)
{
	int len;

	if (!str)
		return addr(rt_straddr);
	len = uaestrlen(str) + 1;
	rt_straddr -= len;
	strcpy((uae_char*)rtarea_bank.baseaddr + rt_straddr, str);
	return addr(rt_straddr);
}

uae_u32 ds (const TCHAR *str)
{
	char *s = ua (str);
	uae_u32 v = ds_ansi (s);
	xfree (s);
	return v;
}

uae_u32 dsf(uae_u8 b, int len)
{
	rt_straddr -= len;
	memset(rtarea_bank.baseaddr + rt_straddr, b, len);
	return addr(rt_straddr);
}

uae_u32 ds_bstr_ansi (const uae_char *str)
{
	int len;
 
	len = uaestrlen(str) + 2;
	rt_straddr -= len;
	while (rt_straddr & 3)
		rt_straddr--;
	rtarea_bank.baseaddr[rt_straddr] = len - 2;
	strcpy ((uae_char*)rtarea_bank.baseaddr + rt_straddr + 1, str);
	return addr (rt_straddr) >> 2;
}

void save_rom_absolute(uaecptr addr)
{
	if (rombase_new)
		return;
	if (absolute_rom_address >= MAX_ABSOLUTE_ROM_ADDRESS) {
		write_log(_T("MAX_ABSOLUTE_ROM_ADDRESS is too low!"));
		abort();
	}
	for (int i = 0; i < absolute_rom_address; i++) {
		if (absolute_rom_addresses[i] == addr) {
			write_log(_T("Address %08x already added\n"), addr);
			return;
		}
	}
	absolute_rom_addresses[absolute_rom_address++] = addr;
}

void add_rom_absolute(uaecptr addr)
{
	uaecptr h = here();
	dl(addr);
	save_rom_absolute(h);
}

uae_u32 boot_rom_copy(TrapContext *ctx, uaecptr rombase, int mode)
{
	uaecptr reloc = 0;
	if (currprefs.uaeboard < 3)
		return 0;
	if (!mode) {
		rtarea_write_enabled = true;
		protect_roms(false);
		rombase_new = rombase;
		int size = 4 + 2 + 4;
		for (int i = 0; i < absolute_rom_address; i++) {
			uae_u32 a = absolute_rom_addresses[i];
			if (a >= rtarea_base && a < rtarea_base + 0x10000) {
				size += 2;
			} else {
				size += 4;
			}
		}
		reloc = uaeboard_alloc_ram(size);
		uae_u8 *p = uaeboard_map_ram(reloc);
		put_long_host(p, rtarea_base);
		p += 4;
		for (int i = 0; i < absolute_rom_address; i++) {
			uae_u32 a = absolute_rom_addresses[i];
			if (a >= rtarea_base && a < rtarea_base + 0x10000) {
				put_word_host(p, a & 0xffff);
				p += 2;
			}
		}
		put_word_host(p, 0);
		p += 2;
		for (int i = 0; i < absolute_rom_address; i++) {
			uae_u32 a = absolute_rom_addresses[i];
			if (a < rtarea_base || a >= rtarea_base + 0x10000) {
				put_long_host(p, a);
				p += 4;
			}
		}
		put_long_host(p, 0);
		write_log(_T("ROMBASE %08x RAMBASE %08x RELOC %08x (%d)\n"), rtarea_base, rombase, reloc, absolute_rom_address);
	} else {
		rtarea_write_enabled = false;
		protect_roms(true);
		write_log(_T("ROMBASE changed.\n"), absolute_rom_address);
		reloc = 1;
	}
	return reloc;
}

void calltrap (uae_u32 n)
{
	if (currprefs.uaeboard > 2) {
		dw(0x4eb9); // JSR rt_trampoline_ptr
		add_rom_absolute(rt_trampoline_ptr);
		uaecptr a = here();
		org(rt_trampoline_ptr);
		dw(0x3f3c); // MOVE.W #n,-(SP)
		dw(n);
		dw(0x4ef9); // JMP rt_trampoline_entry
		add_rom_absolute(trap_entry);
		org(a);
		rt_trampoline_ptr += 3 * 2 + 1 * 4;
	} else {
		dw(0xA000 + n);
	}
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

static uae_u32 REGPARAM2 nullfunc (TrapContext *ctx)
{
	write_log (_T("Null function called\n"));
	return 0;
}

static uae_u32 REGPARAM2 getchipmemsize (TrapContext *ctx)
{
	trap_set_dreg(ctx, 1, z3chipmem_bank.allocated_size);
	trap_set_areg(ctx, 1, z3chipmem_bank.start);
	return chipmem_bank.allocated_size;
}

static uae_u32 REGPARAM2 uae_puts (TrapContext *ctx)
{
	uae_char buf[MAX_DPATH];
	trap_get_string(ctx, buf, trap_get_areg(ctx, 0), sizeof (uae_char));
	TCHAR *s = au(buf);
	write_log(_T("%s"), s);
	xfree(s);
	return 0;
}

void rtarea_init_mem (void)
{
	if (need_uae_boot_rom(&currprefs)) {
		rtarea_bank.flags &= ~ABFLAG_ALLOCINDIRECT;
	} else {
		// not enabled and something else may use same address space
		rtarea_bank.flags |= ABFLAG_ALLOCINDIRECT;
	}
	rtarea_bank.reserved_size = RTAREA_SIZE;
	rtarea_bank.start = rtarea_base;
	if (!mapped_malloc (&rtarea_bank)) {
		write_log (_T("virtual memory exhausted (rtarea)!\n"));
		abort ();
	}
}

void rtarea_free(void)
{
	mapped_free(&rtarea_bank);
}

void rtarea_init(void)
{
	uae_u32 a;
	TCHAR uaever[100];

	rt_straddr = 0xFF00 - 2;
	rt_addr = 0;
	rt_addr_reset = true;

	rt_trampoline_ptr = rtarea_base + RTAREA_TRAMPOLINE;
	trap_entry = 0;
	absolute_rom_address = 0;
	rombase_new = 0;

	init_traps ();

	rtarea_init_mem ();
	memset (rtarea_bank.baseaddr, 0, RTAREA_SIZE);

	_sntprintf (uaever, sizeof uaever, _T("uae-%d.%d.%d"), UAEMAJOR, UAEMINOR, UAESUBREV);

	EXPANSION_uaeversion = ds (uaever);
	EXPANSION_explibname = ds (_T("expansion.library"));
	EXPANSION_doslibname = ds (_T("dos.library"));
	EXPANSION_uaedevname = ds (_T("uae.device"));

	dw (0);
	dw (0);

#ifdef FILESYS
	filesys_install_code();

	trap_entry = filesys_get_entry(10);
	write_log(_T("TRAP_ENTRY = %08x\n"), trap_entry);

	for (int i = 0; i < RTAREA_TRAP_DATA_SIZE / RTAREA_TRAP_DATA_SLOT_SIZE; i++) {
		uae_sem_init(&hardware_trap_event[i], 0, 0);
		uae_sem_init(&hardware_trap_event2[i], 0, 0);
	}

#endif

	deftrap(NULL); /* Generic emulator trap */

	a = here ();
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
		abort ();
	}

#ifdef PICASSO96
	uaegfx_install_code (rtarea_base + RTAREA_RTG);
#endif

	org (RTAREA_TRAPS | rtarea_base);
	init_extended_traps ();

	if (currprefs.uaeboard >= 2) {
		device_add_rethink(rethink_traps);
	}
}

volatile uae_atomic uae_int_requested = 0;

void rtarea_setup (void)
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

