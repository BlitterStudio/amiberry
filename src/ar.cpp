/*
* UAE Action Replay 1/2/3/1200 and HRTMon support
*
* (c) 2000-2015 Toni Wilen <twilen@arabuusimiehet.com>
* (c) 2003 Mark Cox <markcox@email.com>
*
* Action Replay 1200 (basically old version of HRTMon):
*
* 256k ROM at 0x800000
*  64k RAM at 0x880000
* status register at 0x8c0000 (bit 3 = freeze button, bit 4 = hide)
* custom register writes stored at 0x88f000
* CIA-A at 0x88e000
* CIA-B at 0x88d000
* freeze button = bus error + rom mapped at 0x0
*
* 14.06.2006 first implementation
*
* Action Replay 2/3:
*
* Tested with AR3 ROM version 3.09 (10/13/91) and AR2 2.12 (12/24/90)
*
* Found to work for the following roms by Mark Cox:
* (Yes the date format is inconsistent, i just copied it straight from the rom)
* 1.15
* 2.14 22/02/91 dd/mm/yy
* 3.09 10/13/91 mm/dd/yy
* 3.17 12/17/91 mm/dd/yy
*
* This patch also makes AR3 compatible with KickStart's other than 1.3
* (ROM checksum error is normal with KS != 1.3)
* NOTE: AR has problems with 68020+ processors.
* For maximum compatibility select 68000/68010 and A500 speed from UAE
* options.
*
* How to rip Action Replay 1/2/3 ROM:
*
* Find A500 with AR1/2/3, press 'freeze'-button
*
* type following:
*
* AR1:
* lord olaf<RETURN>
*
* AR2 or AR3:
* may<RETURN>
* the<RETURN>
* force<RETURN>
* be<RETURN>
* with<RETURN>
* you<RETURN>
* new<RETURN> (AR3 only)
*
* AR1: 64K ROM is visible at 0xf00000-0xf0ffff
* and 16K RAM at 0x9fc000-0x9fffff
* AR2: 128K ROM is visible at 0x400000-0x41ffff
* AR3: 256K ROM is visible at 0x400000-0x43ffff
* and 64K RAM at 0x440000-0x44ffff
*
* following command writes ROM to disk:
*
* AR1: sm ar1.rom,f00000 f10000
* AR2: sm ar2.rom,400000 420000
* AR3: sm ar3.rom,400000 440000
*
* NOTE: I (mark) could not get the action replay 1 dump to work as above.
* (also, it will only dump to the action replay special disk format)
* To dump the rom i had to :
* 1. Boot the a500 and start a monitor (e.g. cmon).
* 2. Use the monitor to allocate 64k memory.
* 3. Enter the action replay.
* 4. Enter sysop mode.
* 5. Copy the rom into the address the monitor allocated.
* 6. Exit the action replay.
* 7. Save the ram from the monitor to disk.
*
* I DO NOT REPLY MAILS ASKING FOR ACTION REPLAY ROMS!
*
* AR2/3 hardware notes (not 100% correct..)
*
* first 8 bytes of ROM are not really ROM, they are
* used to read/write cartridge's hardware state
*
* Read/write HW state
*
* 0x400001.B READ:
*
* 11 (3) = freeze was caused by reset condition
* 10 (2) = freeze was caused by 0xBFD100.B WRITE
* 01 (1) = freeze was caused by 0xBFE001.B READ
* 00 (0) = freeze was caused by button press
*
* 0x400001.B WRITE
*
* bit 1 set: enable freeze caused by BFE001.B READ
* bit 0 set: enable freeze caused by BFD100.B WRITE 
*
* 0x400002/0x400003: mirrors 0x400000/0x400001
* 0x400006/0x400007: when written to, turns chip-ram overlay off
*
* Reset condition: first chip ram (or non-custom register) write after hardware reset.
*
* cartridge hardware also snoops CPU write accesses to custom chip
* registers (DFF000-DFF1FE). All CPU custom chip write accesses are
* saved to RAM at 0x44f000-0x44f1ff. Note that emulated AR3 also
* saves copper's custom chip accesses. This fix stops programs
* that try to trick AR by using copper to update write-only
* custom registers.
*
* 30.04.2001 - added AR2 support
* 21.07.2001 - patch updated
* 29.07.2002 - added AR1 support
* 11.03.2003 - added AR1 breakpoint support, checksum support and fixes. (Mark Cox)
*
*/

/* AR2/3 'NORES' info.
* On ar2 there is a 'nores' command,
* on ar3, it is accessible using the mouse.
* This command will not work using the current infrastructure,
* so don't use it 8).
*/

/* AR1 Breakpoint info.
* 1.15 If a breakpoint occurred. Its address is stored at 9fe048.
* The 5 breakpoint entries each consisting of 6 bytes are stored at 9fe23e.
* Each entry contains the breakpoint long word followed by 2 bytes of the original contents of memory
* that is replaced by a trap instruction in mem.
* So the table finishes at 9fe25c.
*/

/* How AR1 is entered on reset:
* In the kickstart (1.3) there is the following code:
* I have marked the important lines:
*
* fc00e6 lea f00000,a1     ; address where AR1 rom is located.
* fc00ec cmpa.l a1,a0
* fc00ee beq fc00fe.s
* fc00f0 lea C(pc), a5
* fc00f4 cmpi.w #1111,(a1) ; The first word of the AR1 rom is set to 1111.
* fc00f8 bne fc00fe.s
* fc00fa jmp 2(a1)	    ; This is the entry point of the rom.
*/

/* Flag info:
* AR3:'ARON'. This is unset initially. It is set the first time you enter the AR via a freeze.
* It enables you to keep the keyboard buffer and such.
* If this flag is unset, the keyboard buffer is cleared, the breakpoints are deleted and ... */

/* AR3:'PRIN'. This flag is unset initially. It is set at some point and when you switch to the 2nd screen
* for the first time it displays all the familiar text. Then unsets 'PRIN'.
*/

/* Super IV:
*
* Possible "ROM" addresses ("ROM" is loaded from disk to Amiga memory)
* - 0xd00000
* - 0xc00000
* - 0x080000
*
* CIA-A: 0xb40000 (0x000, 0x100,...)
* CIA-B: 0xb40001 (0x001, 0x101,...)
* Custom: 0xe40000
*
* NOTE: emulation also supports 0xd00000 relocated "rom"-images
*/

/* X-Power 500:
*
* ROM: 0xe20000 (128k)
* RAM: 0xf20000 (64k)
* CIA-A: 0xf2fc00 (00,02,04,...)
* CIA-B: 0xf2fc01 (01,03,05,...)
* Custom: 0xf2fc00 (from 0x20->)
*/

/* Nordic Power:
*
* ROM: 0xf00000 (64k, mirrored at 0xf10000)
* RAM: 0xf40000 (32k, mirrored at 0xf48000 - 0xf5ffff)
* CIA-A: 0xf43c00 (00,02,04,...)
* CIA-B: 0xf43c01 (01,03,05,...)
* Custom: 0xf43c00 (from 0x20->)
* addresses 0 to 1023: 0xf40000 (weird feature..)
*/

/* X-Power and Nordic Power ROM scrambling
*
* Data lines are swapped.
* Address lines are XOR'd (0x817F) and swapped.
*
* Even (middle) ROM
*
* Data: 0-3,1-6,2-0,3-4,4-7,5-5,6-1,7-2
* Addr: 0-7,1-1,2-2,3-11,4-12,5-0,6-13,7-14,8-8,9-3,10-5,11-6,12-4,13-10,14-9,15-15
*
* Odd (corner) ROM
*
* Data: 0-2,1-3,2-4,3-5,4-6,5-7,6-0,7-1
* Addr: 0-3,1-6,2-5,3-7,4-9,5-12,6-14,7-13,8-8,9-11,10-10,11-1,12-0,13-4,14-2,15-15
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "zfile.h"
#include "ar.h"
#include "savestate.h"
#include "crc32.h"
#include "akiko.h"

static const TCHAR *cart_memnames[] = { NULL, _T("hrtmon"), _T("arhrtmon"), _T("superiv") };

// Action Replay 2/3
// $400001.B read values
#define ARMODE_FREEZE 0 /* 'freeze' button has been pressed.  */
#define ARMODE_READ_BFE001 1 /* BFE001 read = freeze */
#define ARMODE_WRITE_BFD100 2 /* BFD100 write = freeze */
#define ARMODE_SOFTRESET 2
#define ARMODE_RESET 3 /* reset state */
// $400001.B written values
#define ARMODE_ACTIVATE_BFE001 2
#define ARMODE_ACTIVATE_BFD100 1
#define ARMODE_ACTIVE_NONE 0

#define CART_AR 0
#define CART_HRTMON 1
#define CART_AR1200 2
#define CART_SUPER4 3

uae_u8 ar_custom[2*256];
uae_u8 ar_ciaa[16], ar_ciab[16];
static int hrtmon_ciadiv = 256;

int hrtmon_flag = ACTION_REPLAY_INACTIVE;
static int cart_type;
static int ar_mapped, ar_hidden;

static uae_u8 *hrtmemory = 0, *hrtmemory2 = 0, *hrtmemory3 = 0;
static uae_u8 *armemory_rom = 0, *armemory_ram = 0;

static uae_u32 hrtmem_mask, hrtmem2_mask, hrtmem3_mask;
static uae_u8 *hrtmon_custom, *hrtmon_ciaa, *hrtmon_ciab, *hrtmon_zeropage;
uae_u32 hrtmem_start, hrtmem2_start, hrtmem3_start, hrtmem_size, hrtmem2_size, hrtmem2_size2, hrtmem3_size;
uae_u32 hrtmem_end, hrtmem2_end;
static int hrtmem_rom;
static int triggered_once;
static bool action_replay_hardreset;

static void hrtmon_unmap_banks (void);

void check_prefs_changed_carts (int in_memory_reset);

static int stored_picasso_on = -1;

static void cartridge_enter (void)
{
#ifdef PICASSO96
	stored_picasso_on = picasso_requested_on;
	picasso_requested_on = 0;
#endif
}
static void cartridge_exit (void)
{
#ifdef PICASSO96
	if (stored_picasso_on > 0)
		picasso_requested_on = 1;
	stored_picasso_on = -1;
#endif
}

static uae_u32 REGPARAM2 hrtmem3_bget (uaecptr addr)
{
	addr -= hrtmem3_start & hrtmem3_mask;
	addr &= hrtmem3_mask;
	return hrtmemory3[addr];
}

static uae_u32 REGPARAM2 hrtmem3_wget (uaecptr addr)
{
	return (hrtmem3_bget (addr) << 8) | hrtmem3_bget (addr + 1);
}

static uae_u32 REGPARAM2 hrtmem3_lget (uaecptr addr)
{
	return (hrtmem3_wget (addr) << 16) | hrtmem3_wget (addr + 2);
}

static void REGPARAM2 hrtmem3_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= hrtmem3_start & hrtmem3_mask;
	addr &= hrtmem3_mask;
	m = (uae_u32 *)(hrtmemory3 + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 hrtmem3_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr -= hrtmem3_start & hrtmem3_mask;
	addr &= hrtmem3_mask;
	m = (uae_u16 *)(hrtmemory3 + addr);
	do_put_mem_word (m, (uae_u16)w);
}

static void REGPARAM2 hrtmem3_bput (uaecptr addr, uae_u32 b)
{
	addr -= hrtmem3_start & hrtmem3_mask;
	addr &= hrtmem3_mask;
	hrtmemory3[addr] = b;
}
static int REGPARAM2 hrtmem3_check (uaecptr addr, uae_u32 size)
{
	addr -= hrtmem3_start & hrtmem3_mask;
	addr &= hrtmem3_mask;
	return (addr + size) <= hrtmem3_size;
}

static uae_u8 *REGPARAM2 hrtmem3_xlate (uaecptr addr)
{
	addr -= hrtmem3_start & hrtmem3_mask;
	addr &= hrtmem3_mask;
	return hrtmemory3 + addr;
}

static uae_u32 REGPARAM2 hrtmem2_bget (uaecptr addr)
{
	if (addr == 0xb8007c && cart_type == CART_SUPER4) {
		static int cnt = 60;
		cnt--;
		if (cnt == 0)
			uae_reset(0, 0);
	}
	addr -= hrtmem2_start & hrtmem2_mask;
	addr &= hrtmem2_mask;
	return hrtmemory2[addr];
}
static uae_u32 REGPARAM2 hrtmem2_wget (uaecptr addr)
{
	return (hrtmem2_bget (addr) << 8) | hrtmem2_bget (addr + 1);
}
static uae_u32 REGPARAM2 hrtmem2_lget (uaecptr addr)
{
	return (hrtmem2_wget (addr) << 16) | hrtmem2_wget (addr + 2);
}

static void REGPARAM2 hrtmem2_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= hrtmem2_start & hrtmem2_mask;
	addr &= hrtmem2_mask;
	m = (uae_u32 *)(hrtmemory2 + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 hrtmem2_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr -= hrtmem2_start & hrtmem2_mask;
	addr &= hrtmem2_mask;
	m = (uae_u16 *)(hrtmemory2 + addr);
	do_put_mem_word (m, (uae_u16)w);
}
static void REGPARAM2 hrtmem2_bput (uaecptr addr, uae_u32 b)
{
	addr -= hrtmem2_start & hrtmem2_mask;
	addr &= hrtmem2_mask;
	hrtmemory2[addr] = b;
}
static int REGPARAM2 hrtmem2_check (uaecptr addr, uae_u32 size)
{
	addr -= hrtmem2_start & hrtmem2_mask;
	addr &= hrtmem2_mask;
	return (addr + size) <= hrtmem2_size;
}

static uae_u8 *REGPARAM2 hrtmem2_xlate (uaecptr addr)
{
	addr -= hrtmem2_start & hrtmem2_mask;
	addr &= hrtmem2_mask;
	return hrtmemory2 + addr;
}

static uae_u32 REGPARAM2 hrtmem_lget (uaecptr addr)
{
	uae_u32 *m;
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	m = (uae_u32 *)(hrtmemory + addr);
	return do_get_mem_long (m);
}
static uae_u32 REGPARAM2 hrtmem_wget (uaecptr addr)
{
	uae_u16 *m;
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	m = (uae_u16 *)(hrtmemory + addr);
	return do_get_mem_word (m);
}
static uae_u32 REGPARAM2 hrtmem_bget (uaecptr addr)
{
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	return hrtmemory[addr];
}

static void REGPARAM2 hrtmem_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	if (cart_type == CART_AR1200 && addr < 0x80000)
		return;
	if (hrtmem_rom)
		return;
	m = (uae_u32 *)(hrtmemory + addr);
	do_put_mem_long (m, l);
}

static void REGPARAM2 hrtmem_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	if (cart_type == CART_AR1200 && addr < 0x80000)
		return;
	if (hrtmem_rom)
		return;
	m = (uae_u16 *)(hrtmemory + addr);
	do_put_mem_word (m, (uae_u16)w);
}
static void REGPARAM2 hrtmem_bput (uaecptr addr, uae_u32 b)
{
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	if (cart_type == CART_AR1200 && addr < 0x80000)
		return;
	if (hrtmem_rom)
		return;
	hrtmemory[addr] = b;
}
static int REGPARAM2 hrtmem_check (uaecptr addr, uae_u32 size)
{
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	return (addr + size) <= hrtmem_size;
}

static uae_u8 *REGPARAM2 hrtmem_xlate (uaecptr addr)
{
	addr -= hrtmem_start & hrtmem_mask;
	addr &= hrtmem_mask;
	return hrtmemory + addr;
}

static addrbank hrtmem_bank = {
	hrtmem_lget, hrtmem_wget, hrtmem_bget,
	hrtmem_lput, hrtmem_wput, hrtmem_bput,
	hrtmem_xlate, hrtmem_check, NULL, NULL, _T("Cartridge Bank"),
	hrtmem_lget, hrtmem_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};
static addrbank hrtmem2_bank = {
	hrtmem2_lget, hrtmem2_wget, hrtmem2_bget,
	hrtmem2_lput, hrtmem2_wput, hrtmem2_bput,
	hrtmem2_xlate, hrtmem2_check, NULL, NULL, _T("Cartridge Bank 2"),
	hrtmem2_lget, hrtmem2_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};
static addrbank hrtmem3_bank = {
	hrtmem3_lget, hrtmem3_wget, hrtmem3_bget,
	hrtmem3_lput, hrtmem3_wput, hrtmem3_bput,
	hrtmem3_xlate, hrtmem3_check, NULL, NULL, _T("Cartridge Bank 3"),
	hrtmem3_lget, hrtmem3_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};

static void copyfromamiga (uae_u8 *dst, uaecptr src, int len)
{
	while (len--) {
		*dst++ = get_byte (src);
		src++;
	}
}

static void copytoamiga (uaecptr dst, uae_u8 *src, int len)
{
	while (len--) {
		put_byte (dst, *src++);
		dst++;
	}
}

int action_replay_flag = ACTION_REPLAY_INACTIVE;
static int ar_rom_file_size;

/* Use this for relocating AR? */
static int ar_rom_location;
/*static*/ int armodel;
static uae_u8 artemp[4]; /* Space to store the 'real' level 7 interrupt */
static uae_u8 armode_read, armode_write;

static uae_u32 arrom_start, arrom_size, arrom_mask;
static uae_u32 arram_start, arram_size, arram_mask;

static int ar_wait_pop = 0; /* bool used by AR1 when waiting for the program counter to exit it's ram. */
uaecptr wait_for_pc = 0;    /* The program counter that we wait for. */

/* returns true if the Program counter is currently in the AR rom. */
int is_ar_pc_in_rom (void)
{
	uaecptr pc = m68k_getpc () & 0xFFFFFF;
	return pc >= arrom_start && pc < arrom_start+arrom_size;
}

/* returns true if the Program counter is currently in the AR RAM. */
int is_ar_pc_in_ram (void)
{
	uaecptr pc = m68k_getpc () & 0xFFFFFF;
	return pc >= arram_start && pc < arram_start+arram_size;
}


/* flag writing == 1 for writing memory, 0 for reading from memory. */
STATIC_INLINE int ar3a (uaecptr addr, uae_u8 b, int writing)
{
	if (armodel == 1) /* With AR1. It is always a read. Actually, it is a strobe on exit of the AR.
					  * but, it is also read during the checksum routine. */
	{
		if (addr < 2) {
			if (is_ar_pc_in_rom()) {
				if (ar_wait_pop) {
					action_replay_flag = ACTION_REPLAY_WAIT_PC;
					ar_wait_pop = 0;
					/* We get (SP+2) here, as the first word on the stack is the status register. */
					/* We want the following long, which is the return program counter. */
					wait_for_pc = longget (m68k_areg (regs, 7) + 2); /* Get (SP+2) */
					set_special (SPCFLAG_ACTION_REPLAY);

					uaecptr pc = m68k_getpc ();
				}
				else
				{
					uaecptr pc = m68k_getpc ();
					action_replay_flag = ACTION_REPLAY_IDLE;
				}
			}
		}
		/* This probably violates the hide_banks thing except ar1 doesn't use that yet .*/
		return armemory_rom[addr];
	}

#ifdef ACTION_REPLAY_HIDE_CARTRIDGE
	if (addr >= 8)
		return armemory_rom[addr];

	if (action_replay_flag == 0)
		return 0;
#endif

	if (!writing) {
		//write_log(_T("READ %x\n"), addr);
		if (addr == 1 || addr == 3) { /* This is necessary because we don't update rom location 0 every time we change armode */
			return armode_read | (regs.irc & ~3);
		} else if (addr < 4) {
			return (addr & 1) ? regs.irc : regs.irc >> 8;
		}
		return armemory_rom[addr];
	} else {
		//write_log(_T("WRITE %x\n"), addr);
		if (addr == 1) {
			armode_write = b;
			armode_read = 0;
			set_special (SPCFLAG_ACTION_REPLAY);
			action_replay_flag = ACTION_REPLAY_HIDE;
		} else if (addr == 6) {
			copytoamiga (regs.vbr + 0x7c, artemp, 4);
		}
	}
	return 0;
}

static void action_replay_chipwrite (void);

void REGPARAM2 chipmem_lput_actionreplay1 (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= chipmem_start_addr & chipmem_bank.mask;
	addr &= chipmem_bank.mask;
	if (addr == 0x60 && !is_ar_pc_in_rom())
		action_replay_chipwrite ();
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_long (m, l);
}
void REGPARAM2 chipmem_wput_actionreplay1 (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr -= chipmem_start_addr & chipmem_bank.mask;
	addr &= chipmem_bank.mask;
	if (addr == 0x60 && !is_ar_pc_in_rom())
		action_replay_chipwrite ();
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word (m, w);
}
void REGPARAM2 chipmem_bput_actionreplay1 (uaecptr addr, uae_u32 b)
{
	addr -= chipmem_start_addr & chipmem_bank.mask;
	addr &= chipmem_bank.mask;
	if (addr >= 0x60 && addr <= 0x63 && !is_ar_pc_in_rom())
		action_replay_chipwrite();
	chipmem_bank.baseaddr[addr] = b;
}
void REGPARAM2 chipmem_lput_actionreplay23 (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;
	addr -= chipmem_start_addr & chipmem_bank.mask;
	addr &= chipmem_bank.mask;
	m = (uae_u32 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_long (m, l);
	if (action_replay_flag == ACTION_REPLAY_WAITRESET)
		action_replay_chipwrite();
}
void REGPARAM2 chipmem_wput_actionreplay23 (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	addr -= chipmem_start_addr & chipmem_bank.mask;
	addr &= chipmem_bank.mask;
	m = (uae_u16 *)(chipmem_bank.baseaddr + addr);
	do_put_mem_word (m, w);
	if (action_replay_flag == ACTION_REPLAY_WAITRESET)
		action_replay_chipwrite();
}


static uae_u32 REGPARAM3 arram_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 arram_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 arram_bget (uaecptr) REGPARAM;
static void  REGPARAM3 arram_lput (uaecptr, uae_u32) REGPARAM;
static void  REGPARAM3 arram_wput (uaecptr, uae_u32) REGPARAM;
static void  REGPARAM3 arram_bput (uaecptr, uae_u32) REGPARAM;
static int  REGPARAM3 arram_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 arram_xlate (uaecptr addr) REGPARAM;

static uae_u32 REGPARAM3 arrom_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 arrom_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 arrom_bget (uaecptr) REGPARAM;
static void REGPARAM3 arrom_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 arrom_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 arrom_bput (uaecptr, uae_u32) REGPARAM;
static int  REGPARAM3 arrom_check (uaecptr addr, uae_u32 size) REGPARAM;
static uae_u8 *REGPARAM3 arrom_xlate (uaecptr addr) REGPARAM;
static void action_replay_unmap_banks (void);

static uae_u32 action_replay_calculate_checksum(void);
static uae_u8* get_checksum_location(void);
static void disable_rom_test(void);

static uae_u32 ar_null(int size)
{
	if (size == 4)
		return dummy_bank.lget(0);
	if (size == 2)
		return dummy_bank.wget(0);
	return dummy_bank.bget(0);
}

static uae_u32 REGPARAM2 arram_lget (uaecptr addr)
{
	uae_u32 *m;
	if (ar_hidden)
		return ar_null(4);
	addr -= arram_start;
	addr &= arram_mask;
	m = (uae_u32 *)(armemory_ram + addr);
	return do_get_mem_long (m);
}

static uae_u32 REGPARAM2 arram_wget (uaecptr addr)
{
	uae_u16 *m;
	if (ar_hidden)
		return ar_null(4);
	addr -= arram_start;
	addr &= arram_mask;
	m = (uae_u16 *)(armemory_ram + addr);
	return do_get_mem_word (m);
}

static uae_u32 REGPARAM2 arram_bget (uaecptr addr)
{
	if (ar_hidden)
		return ar_null(4);
	addr -= arram_start;
	addr &= arram_mask;
	return armemory_ram[addr];
}

void REGPARAM2 arram_lput (uaecptr addr, uae_u32 l)
{
	uae_u32 *m;

	if (ar_hidden)
		return;
	addr -= arram_start;
	addr &= arram_mask;
	m = (uae_u32 *)(armemory_ram + addr);
	do_put_mem_long (m, l);
}

void REGPARAM2 arram_wput (uaecptr addr, uae_u32 w)
{
	uae_u16 *m;

	if (ar_hidden)
		return;
	addr -= arram_start;
	addr &= arram_mask;
	m = (uae_u16 *)(armemory_ram + addr);
	do_put_mem_word (m, w);
}

void REGPARAM2 arram_bput (uaecptr addr, uae_u32 b)
{
	if (ar_hidden)
		return;
	addr -= arram_start;
	addr &= arram_mask;
	armemory_ram[addr] = b;
}

static int REGPARAM2 arram_check (uaecptr addr, uae_u32 size)
{
	addr -= arram_start;
	addr &= arram_mask;
	return (addr + size) <= arram_size;
}

static uae_u8 *REGPARAM2 arram_xlate (uaecptr addr)
{
	addr -= arram_start;
	addr &= arram_mask;
	return armemory_ram + addr;
}

static uae_u32 REGPARAM2 arrom_lget (uaecptr addr)
{
	if (ar_hidden)
		return ar_null(4);
	addr -= arrom_start;
	addr &= arrom_mask;
	return (ar3a (addr, 0, 0) << 24) | (ar3a (addr + 1, 0, 0) << 16) | (ar3a (addr + 2, 0, 0) << 8) | ar3a (addr + 3, 0, 0);
}

static uae_u32 REGPARAM2 arrom_wget (uaecptr addr)
{
	if (ar_hidden)
		return ar_null(2);
	addr -= arrom_start;
	addr &= arrom_mask;
	return (ar3a (addr, 0, 0) << 8) | ar3a (addr + 1, 0, 0);
}

static uae_u32 REGPARAM2 arrom_bget (uaecptr addr)
{
	if (ar_hidden)
		return ar_null(1);
	addr -= arrom_start;
	addr &= arrom_mask;
	return ar3a (addr, 0, 0);
}

static void REGPARAM2 arrom_lput (uaecptr addr, uae_u32 l)
{
	if (ar_hidden)
		return;
	addr -= arrom_start;
	addr &= arrom_mask;
	ar3a (addr + 0,(uae_u8)(l >> 24), 1);
	ar3a (addr + 1,(uae_u8)(l >> 16), 1);
	ar3a (addr + 2,(uae_u8)(l >> 8), 1);
	ar3a (addr + 3,(uae_u8)(l >> 0), 1);
}

static void REGPARAM2 arrom_wput (uaecptr addr, uae_u32 w)
{
	if (ar_hidden)
		return;
	addr -= arrom_start;
	addr &= arrom_mask;
	ar3a (addr + 0,(uae_u8)(w >> 8), 1);
	ar3a (addr + 1,(uae_u8)(w >> 0), 1);
}

static void REGPARAM2 arrom_bput (uaecptr addr, uae_u32 b)
{
	if (ar_hidden)
		return;
	addr -= arrom_start;
	addr &= arrom_mask;
	ar3a (addr, b, 1);
}

static int REGPARAM2 arrom_check (uaecptr addr, uae_u32 size)
{
	addr -= arrom_start;
	addr &= arrom_mask;
	return (addr + size) <= arrom_size;
}

static uae_u8 *REGPARAM2 arrom_xlate (uaecptr addr)
{
	addr -= arrom_start;
	addr &= arrom_mask;
	return armemory_rom + addr;
}

static addrbank arrom_bank = {
	arrom_lget, arrom_wget, arrom_bget,
	arrom_lput, arrom_wput, arrom_bput,
	arrom_xlate, arrom_check, NULL, NULL, _T("Action Replay ROM"),
	arrom_lget, arrom_wget,
	ABFLAG_ROM, S_READ, S_WRITE
};
static addrbank arram_bank = {
	arram_lget, arram_wget, arram_bget,
	arram_lput, arram_wput, arram_bput,
	arram_xlate, arram_check, NULL, NULL, _T("Action Replay RAM"),
	arram_lget, arram_wget,
	ABFLAG_RAM, S_READ, S_WRITE
};


static void action_replay_map_banks (void)
{
	if(!armemory_rom)
		return;
	ar_hidden = false;
	if (ar_mapped > 0)
		return;
	ar_mapped = 1;
	map_banks (&arrom_bank, arrom_start >> 16, arrom_size >> 16, 0);
	map_banks (&arram_bank, arram_start >> 16, arram_size >> 16, 0);
}

static void action_replay_unmap_banks (void)
{
	if(!armemory_rom)
		return;
	if (armodel == 1) {
		action_replay_map_banks ();
		return;
	}
	ar_hidden = true;
	if (ar_mapped == 0)
		return;
	ar_mapped = 0;
	map_banks (&dummy_bank, arrom_start >> 16 , arrom_size >> 16, 0);
	map_banks (&dummy_bank, arram_start >> 16 , arram_size >> 16, 0);
}

static void hide_cart (int hide)
{
#ifdef ACTION_REPLAY_HIDE_CARTRIDGE
	if(hide) {
		action_replay_unmap_banks ();
	} else {
		action_replay_map_banks ();
	}
#endif
}

/* Cartridge activates itself by overlaying its rom
* over chip-ram and then issuing IRQ 7
*
* I just copy IRQ vector 7 from ROM to chip RAM
* instead of fully emulating cartridge's behaviour.
*/

static void action_replay_go (void)
{
	cartridge_enter();
	hide_cart (0);
	memcpy (armemory_ram + 0xf000, ar_custom, 2 * 256);
	action_replay_flag = ACTION_REPLAY_ACTIVE;
	set_special (SPCFLAG_ACTION_REPLAY);
	copyfromamiga (artemp, regs.vbr + 0x7c, 4);
	copytoamiga (regs.vbr + 0x7c, armemory_rom + 0x7c, 4);
	NMI ();
}

static void action_replay_go1 (int irq)
{
	cartridge_enter();
	hide_cart (0);
	action_replay_flag = ACTION_REPLAY_ACTIVE;
	memcpy (armemory_ram + 0xf000, ar_custom, 2 * 256);
	NMI ();
}

typedef struct {
	uae_u8 dummy[4+4];
	uae_u8 jmps[3*4];
	uae_u32 mon_size;
	uae_u8 col0h, col0l, col1h, col1l;
	uae_u8 right;
	uae_u8 keyboard;
	uae_u8 key;
	uae_u8 ide;
	uae_u8 a1200;
	uae_u8 aga;
	uae_u8 insert;
	uae_u8 delay;
	uae_u8 lview;
	uae_u8 cd32;
	uae_u8 screenmode;
	uae_u8 novbr;
	uae_u8 entered;
	uae_u8 hexmode;
	uae_u16 error_sr;
	uae_u32 error_pc;
	uae_u16 error_status;
	uae_u8 newid[6];
	uae_u16 mon_version;
	uae_u16	mon_revision;
	uae_u32	whd_base;
	uae_u16	whd_version;
	uae_u16	whd_revision;
	uae_u32	max_chip;
	uae_u32	whd_expstrt;
	uae_u32	whd_expstop;
} HRTCFG;

static void hrtmon_go (void)
{
	uaecptr old;
	int i;

	triggered_once = 1;
	cartridge_enter();
	hrtmon_flag = ACTION_REPLAY_ACTIVE;
	set_special (SPCFLAG_ACTION_REPLAY);
	if (hrtmon_zeropage)
		memcpy (hrtmon_zeropage, chipmem_bank.baseaddr, 1024);
	if (hrtmon_custom)
		memcpy (hrtmon_custom, ar_custom, 2 * 256);
	for (i = 0; i < 16; i++) {
		if (hrtmon_ciaa)
			hrtmon_ciaa[i * hrtmon_ciadiv + 1] = ar_ciaa[i];
		if (hrtmon_ciab)
			hrtmon_ciab[i * hrtmon_ciadiv + 0] = ar_ciab[i];
	}
	if (cart_type == CART_AR1200) {
		old = get_long ((uaecptr)(regs.vbr + 0x8));
		put_word (hrtmem_start + 0xc0000, 4);
		put_long ((uaecptr)(regs.vbr + 8), get_long (hrtmem_start + 8));
		Exception (2);
		put_long ((uaecptr)(regs.vbr + 8), old);
	} else if (cart_type == CART_SUPER4) {
		uae_u32 v = get_long (hrtmem_start + 0x7c);
		if (v) {
			old = get_long ((uaecptr)(regs.vbr + 0x7c));
			put_long ((uaecptr)(regs.vbr + 0x7c), v);
			NMI ();
			put_long ((uaecptr)(regs.vbr + 0x7c), old);
		}
	} else { // HRTMON
		old = get_long ((uaecptr)(regs.vbr + 0x7c));
		put_long ((uaecptr)(regs.vbr + 0x7c), hrtmem_start + 8 + 4);
		NMI ();
		//put_long ((uaecptr)(regs.vbr + 0x7c), old);
	}
}

void hrtmon_enter (void)
{
	if (!hrtmemory)
		return;
	hrtmon_map_banks ();
	write_log (_T("%s: freeze\n"), cart_memnames[cart_type]);
	hrtmon_go();
}

void action_replay_enter (void)
{
	if (!armemory_rom)
		return;
	triggered_once = 1;
	if (armodel == 1) {
		write_log (_T("AR1: Enter PC:%08x\n"), m68k_getpc ());
		action_replay_go1 (7);
		unset_special (SPCFLAG_ACTION_REPLAY);
		return;
	}
	if (action_replay_flag == ACTION_REPLAY_DORESET) {
		write_log (_T("AR2/3: reset\n"));
		armode_read = action_replay_hardreset ? ARMODE_RESET : ARMODE_SOFTRESET;
		action_replay_hardreset = false;
		ar_mapped = -1;
	}
	action_replay_go();
}

void check_prefs_changed_carts (int in_memory_reset)
{
	if (!config_changed)
		return;
	if (currprefs.cart_internal != changed_prefs.cart_internal)
		currprefs.cart_internal = changed_prefs.cart_internal;
	if (_tcscmp (currprefs.cartfile, changed_prefs.cartfile) != 0) {
		write_log (_T("Cartridge ROM Prefs changed.\n"));
		if (action_replay_unload (in_memory_reset)) {
			memcpy (currprefs.cartfile, changed_prefs.cartfile, sizeof currprefs.cartfile);
#ifdef ACTION_REPLAY
			action_replay_load ();
			action_replay_init (1);
#endif
#ifdef ACTION_REPLAY_HRTMON
			hrtmon_load ();
#endif
		}
	}
}

void action_replay_reset (bool hardreset, bool keyboardreset)
{
	ar_mapped = -1;
	if (hrtmemory) {
		if (isrestore ()) {
			if (m68k_getpc () >= hrtmem_start && m68k_getpc () <= hrtmem_start + hrtmem_size)
				hrtmon_map_banks ();
			else
				hrtmon_unmap_banks ();
		}
	} else {
		if (action_replay_flag == ACTION_REPLAY_INACTIVE)
			return;

		if (isrestore ()) {
			if (m68k_getpc () >= arrom_start && m68k_getpc () <= arrom_start + arrom_size) {
				action_replay_flag = ACTION_REPLAY_ACTIVE;
				hide_cart (0);
			} else {
				action_replay_flag = ACTION_REPLAY_IDLE;
				hide_cart (1);
			}
			return;
		}
		if (armodel == 1) {
			/* We need to mark it as active here, because the kickstart rom jumps directly into it. */
			action_replay_flag = ACTION_REPLAY_ACTIVE;
			hide_cart (0);
		} else {
			armode_read = ARMODE_RESET;
			action_replay_flag = ACTION_REPLAY_WAITRESET;
			if (hardreset || keyboardreset || armodel == 2)
				action_replay_hardreset = true;
			hide_cart (0);
		}
	}
}

void action_replay_cia_access(bool write)
{
	if (armodel < 2)
		return;
	if (action_replay_flag != ACTION_REPLAY_IDLE)
		return;
	if (action_replay_flag == ACTION_REPLAY_INACTIVE)
		return;
	if ((armode_write & ARMODE_ACTIVATE_BFE001) && !write) {
		armode_read = ARMODE_READ_BFE001;
		action_replay_flag = ACTION_REPLAY_ACTIVATE;
		set_special (SPCFLAG_ACTION_REPLAY);
	} else if ((armode_write & ARMODE_ACTIVATE_BFD100) && write) {
		armode_read = ARMODE_WRITE_BFD100;
		action_replay_flag = ACTION_REPLAY_ACTIVATE;
		set_special (SPCFLAG_ACTION_REPLAY);
	}
}

int action_replay_freeze (void)
{
	if (action_replay_flag == ACTION_REPLAY_IDLE) {
		if (armodel == 1) {
			action_replay_chipwrite ();
		} else {
			action_replay_flag = ACTION_REPLAY_ACTIVATE;
			set_special (SPCFLAG_ACTION_REPLAY);
			armode_read = ARMODE_FREEZE;
		}
		return 1;
	} else if (hrtmon_flag) {
		hrtmon_flag = ACTION_REPLAY_ACTIVATE;
		set_special (SPCFLAG_ACTION_REPLAY);
		return 1;
	}
	return 0;
}

static void action_replay_chipwrite (void)
{
	//write_log (_T("AR CW\n"));
	if (armodel == 2 || armodel == 3) {
		action_replay_flag = ACTION_REPLAY_DORESET;
		set_special (SPCFLAG_ACTION_REPLAY);
	} else if (armodel == 1) {
		/* copy 0x60 addr info to level 7 */
		/* This is to emulate the 0x60 interrupt. */
		copyfromamiga (artemp, regs.vbr + 0x60, 4);
		copytoamiga (regs.vbr + 0x7c, artemp, 4);
		ar_wait_pop = 1; /* Wait for stack to pop. */

		action_replay_flag = ACTION_REPLAY_ACTIVATE;
		set_special (SPCFLAG_ACTION_REPLAY);
	}
}

void action_replay_hide(void)
{
	if (armodel >= 2) {
		// Do not unmap if breakpoint mode
		// It would cause dozens of map/unmaps per frame
		// Just "hide" it.
		ar_hidden = true;
		if (armode_read == 0) {
			hide_cart(1);
		}
		action_replay_flag = ACTION_REPLAY_IDLE;
	} else {
		hide_cart(1);
		action_replay_flag = ACTION_REPLAY_IDLE;
	}
}

void hrtmon_hide(void)
{
	HRTCFG *cfg = (HRTCFG*)hrtmemory;
	if (cart_type != CART_AR1200 && cfg->entered)
		return;
	cartridge_exit();
	hrtmon_flag = ACTION_REPLAY_IDLE;
	unset_special (SPCFLAG_ACTION_REPLAY);
	//write_log (_T("HRTMON: Exit\n"));
}

void hrtmon_breakenter(void)
{
	//hrtmon_flag = ACTION_REPLAY_HIDE;
	//set_special (SPCFLAG_ACTION_REPLAY);
}


/* Disabling copperlist processing:
* On: ar317 an rts at 41084c does it.
* On: ar214: an rts at 41068e does it.
*/


/* Original AR3 v3.09 only works with KS 1.3
 * v3.17 supports also KS 2.04 (v37.175)
 * this patch handles other rom versions.
 */

static uae_u8 ar3patch1[] = {0x20,0xc9,0x51,0xc9,0xff,0xfc};
static uae_u8 ar3patch2[] = {0x00,0xfc,0x01,0x44};

static void action_replay_patch (void)
{
	int off1,off2;
	uae_u8 *kickmem = kickmem_bank.baseaddr;

	if (armodel != 3 || !kickmem || !armemory_rom)
		return;
	if (!memcmp (kickmem, kickmem + 262144, 262144)) off1 = 262144; else off1 = 0;
	for (;;) {
		if (!memcmp (kickmem + off1, ar3patch1, sizeof (ar3patch1)) || off1 == 524288 - sizeof (ar3patch1)) break;
		off1++;
	}
	off2 = 0;
	for(;;) {
		if (!memcmp (armemory_rom + off2, ar3patch2, sizeof(ar3patch2)) || off2 == ar_rom_file_size - sizeof (ar3patch2)) break;
		off2++;
	}
	if (off1 == 524288 - sizeof (ar3patch1) || off2 == ar_rom_file_size - sizeof (ar3patch2))
		return;
	armemory_rom[off2 + 0] = (uae_u8)((off1 + kickmem_start_addr + 2) >> 24);
	armemory_rom[off2 + 1] = (uae_u8)((off1 + kickmem_start_addr + 2) >> 16);
	armemory_rom[off2 + 2] = (uae_u8)((off1 + kickmem_start_addr + 2) >> 8);
	armemory_rom[off2 + 3] = (uae_u8)((off1 + kickmem_start_addr + 2) >> 0);
	write_log (_T("AR ROM patched for KS2.0+ (%x)\n"), off2);
}

/* Returns 0 if the checksum is OK.
* Else, it returns the calculated checksum.
* Note: Will be wrong if the checksum is zero, but i'll take my chances on that not happenning ;)
*/
static uae_u32 action_replay_calculate_checksum (void)
{
	uae_u32* checksum_end;
	uae_u32* checksum_start;
	uae_u8   checksum_start_offset[] = { 0, 0, 4, 0x7c };
	uae_u32 checksum = 0;
	uae_u32 stored_checksum;

	/* All models: The checksum is calculated right upto the long checksum in the rom.
	* AR1: The checksum starts at offset 0.
	* AR1: The checksum is the last non-zero long in the rom.
	* AR2: The checksum starts at offset 4.
	* AR2: The checksum is the last Long in the rom.
	* AR3: The checksum starts at offset 0x7c.
	* AR3: The checksum is the last Long in the rom.
	*
	* Checksums: (This is a good way to compare roms. I have two with different md5sums,
	* but the same checksum, so the difference must be in the first four bytes.)
	* 3.17 0xf009bfc9
	* 3.09 0xd34d04a7
	* 2.14 0xad839d36
	* 2.14 0xad839d36
	* 1.15 0xee12116
	*/

	if (!armemory_rom)
		return 0; /* If there is no rom then i guess the checksum is ok */

	checksum_start = (uae_u32*)&armemory_rom[checksum_start_offset[armodel]];
	checksum_end = (uae_u32*)&armemory_rom[ar_rom_file_size];

	/* Search for first non-zero Long starting from the end of the rom. */
	/* Assume long alignment, (will always be true for AR2 and AR3 and the AR1 rom i've got). */
	/* If anyone finds an AR1 rom with a word-aligned checksum, then this code will have to be modified. */
	while (! *(--checksum_end));

	if (armodel == 1)
	{
		uae_u16* rom_ptr_word;
		uae_s16  sign_extended_word;

		rom_ptr_word = (uae_u16*)checksum_start;
		while (rom_ptr_word != (uae_u16*)checksum_end)
		{
			sign_extended_word = (uae_s16)do_get_mem_word (rom_ptr_word);
			/* When the word is cast on the following line, it will get sign-extended. */
			checksum += (uae_u32)sign_extended_word;
			rom_ptr_word++;
		}
	}
	else
	{
		uae_u32* rom_ptr_long;
		rom_ptr_long = checksum_start;
		while (rom_ptr_long != checksum_end)
		{
			checksum += do_get_mem_long (rom_ptr_long);
			rom_ptr_long++;
		}
	}

	stored_checksum = do_get_mem_long (checksum_end);

	return checksum == stored_checksum ? 0 : checksum;
}

/* Returns 0 on error. */
static uae_u8* get_checksum_location (void)
{
	uae_u32* checksum_end;

	/* See action_replay_calculate_checksum() for checksum info. */

	if (!armemory_rom)
		return 0;

	checksum_end = (uae_u32*)&armemory_rom[ar_rom_file_size];

	/* Search for first non-zero Long starting from the end of the rom. */
	while (! *(--checksum_end));

	return (uae_u8*)checksum_end;
}

/* Replaces the existing cart checksum with a correct one. */
/* Useful if you want to patch the rom. */
static void action_replay_fixup_checksum (uae_u32 new_checksum)
{
	uae_u32* checksum = (uae_u32*)get_checksum_location();
	if (checksum)
		do_put_mem_long (checksum, new_checksum);
	else
		write_log (_T("Unable to locate Checksum in ROM.\n"));
	return;
}

/* Longword search on word boundary
* the search_value is assumed to already be in the local endian format
* return 0 on failure
*/
static uae_u8* find_absolute_long (uae_u8* start_addr, uae_u8* end_addr, uae_u32 search_value)
{
	uae_u8* addr;

	for (addr = start_addr; addr < end_addr;) {
		if (do_get_mem_long ((uae_u32*)addr) == search_value) {
			return addr;
		}
		addr += 2;
	}
	return 0;
}

/* word search on word boundary
* the search_addr is assumed to already be in the local endian format
* return 0 on failure
* Currently only tested where the address we are looking for is AFTER the instruction.
* Not sure it works with negative offsets.
*/
static uae_u8* find_relative_word (uae_u8* start_addr, uae_u8* end_addr, uae_u16 search_addr)
{
	uae_u8* addr;

	for (addr = start_addr; addr < end_addr;) {
		if (do_get_mem_word((uae_u16*)addr) == (uae_u16)(search_addr - (uae_u16)(addr - start_addr))) {
			return addr;
		}
		addr += 2;
	}
	return 0;
}

/* Disable rom test */
/* This routine replaces the rom-test routine with a 'rts'.
* It does this in a 'safe' way, by searching for a reference to the checksum
* and only disables it if the surounding bytes are what it expects.
*/

static void disable_rom_test (void)
{
	uae_u8* addr;

	uae_u8* start_addr = armemory_rom;
	uae_u8* end_addr = get_checksum_location();

	/*
	* To see what the routine below is doing. Here is some code from the Action replay rom where it does the
	* checksum test.
	* AR1:
	* F0D4D0 6100 ???? bsr.w   calc_checksum ; calculate the checksum
	* F0D4D4 41FA 147A lea     (0xf0e950,PC),a0   ; load the existing checksum.
	* ; do a comparison.
	* AR2:
	* 40EC92 6100 ???? bsr.w   calc_checksum
	* 40EC96 41F9 0041 FFFC lea (0x41fffc),a0
	*/

	if (armodel == 1) {
		uae_u16 search_value_rel = end_addr - start_addr;
		addr = find_relative_word(start_addr, end_addr, search_value_rel);

		if (addr) {
			if (do_get_mem_word((uae_u16*)(addr-6)) == 0x6100 && /* bsr.w */
				do_get_mem_word((uae_u16*)(addr-2)) == 0x41fa)  /* lea relative */
			{
				write_log (_T("Patching to disable ROM TEST.\n"));
				do_put_mem_word((uae_u16*)(addr-6), 0x4e75); /* rts */
			}
		}
	} else {
		uae_u32 search_value_abs = arrom_start + end_addr - start_addr;
		addr = find_absolute_long (start_addr, end_addr, search_value_abs);

		if (addr) {
			if (do_get_mem_word((uae_u16*)(addr-6)) == 0x6100 && /* bsr.w */
				do_get_mem_word((uae_u16*)(addr-2)) == 0x41f9)  /* lea absolute */
			{
				write_log (_T("Patching to disable ROM TEST.\n"));
				do_put_mem_word((uae_u16*)(addr-6), 0x4e75); /* rts */
			}
		}
	}
}

/* After we have calculated the checksum, and verified the rom is ok,
* we can do two things.
* 1. (optionally)Patch it and then update the checksum.
* 2. Remove the checksum check and (optionally) patch it.
* I have chosen to use no.2 here, because it should speed up the Action Replay slightly (and it was fun).
*/
static void action_replay_checksum_info (void)
{
	if (!armemory_rom)
		return;
	if (action_replay_calculate_checksum() == 0)
		write_log (_T("Action Replay Checksum is OK.\n"));
	else
		write_log (_T("Action Replay Checksum is INVALID.\n"));
	disable_rom_test();
}



static void action_replay_setbanks (void)
{
	if (!savestate_state && chipmem_bank.lput == chipmem_lput) {
		switch (armodel) {
		case 2:
		case 3:
			chipmem_bank.wput = chipmem_wput_actionreplay23;
			chipmem_bank.lput = chipmem_lput_actionreplay23;
			break;
		case 1:
			chipmem_bank.bput = chipmem_bput_actionreplay1;
			chipmem_bank.wput = chipmem_wput_actionreplay1;
			chipmem_bank.lput = chipmem_lput_actionreplay1;
			break;
		}
	}
}

static void action_replay_unsetbanks (void)
{
	chipmem_bank.bput = chipmem_bput;
	chipmem_bank.wput = chipmem_wput;
	chipmem_bank.lput = chipmem_lput;
}

/* param to allow us to unload the cart. Currently we know it is safe if we are doing a reset to unload it.*/
int action_replay_unload (int in_memory_reset)
{
	static const TCHAR *state[] = {
		_T("ACTION_REPLAY_WAIT_PC"),
		_T("ACTION_REPLAY_INACTIVE"),
		_T("ACTION_REPLAY_WAITRESET"),
		_T("0"),
		_T("ACTION_REPLAY_IDLE"),
		_T("ACTION_REPLAY_ACTIVATE"),
		_T("ACTION_REPLAY_ACTIVE"),
		_T("ACTION_REPLAY_DORESET"),
		_T("ACTION_REPLAY_HIDE"),
	};

	if (!armemory_rom && !hrtmemory)
		return 0;

	if (armemory_rom && armodel == 1) {
		if (is_ar_pc_in_ram() || is_ar_pc_in_rom() || action_replay_flag == ACTION_REPLAY_WAIT_PC) {
			write_log (_T("Can't Unload Action Replay 1. It is Active.\n"));
			return 0;
		}
	} else {
		if (action_replay_flag != ACTION_REPLAY_IDLE && action_replay_flag != ACTION_REPLAY_INACTIVE) {
			write_log (_T("Can't Unload Action Replay. It is Active.\n"));
			return 0; /* Don't unload it whilst it's active, or it will crash the amiga if not the emulator */
		}
		if (hrtmon_flag != ACTION_REPLAY_IDLE && hrtmon_flag != ACTION_REPLAY_INACTIVE) {
			write_log (_T("Can't Unload Hrtmon. It is Active.\n"));
			return 0; /* Don't unload it whilst it's active, or it will crash the amiga if not the emulator */
		}
	}

	unset_special (SPCFLAG_ACTION_REPLAY); /* This shouldn't be necessary here, but just in case. */
	action_replay_flag = ACTION_REPLAY_INACTIVE;
	hrtmon_flag = ACTION_REPLAY_INACTIVE;
	action_replay_unsetbanks ();
	action_replay_unmap_banks ();
	hrtmon_unmap_banks ();
	/* Make sure you unmap everything before you call action_replay_cleanup() */
	action_replay_cleanup ();
	return 1;
}

static int superiv_init (struct romdata *rd, struct zfile *f)
{
	uae_u32 chip = currprefs.chipmem_size - 0x10000;
	int subtype = rd->id;
	int flags = rd->type & ROMTYPE_MASK;
	const TCHAR *memname1, *memname2, *memname3;

	memname1 = memname2 = memname3 = NULL;

	cart_type = CART_SUPER4;

	hrtmon_custom = 0;
	hrtmon_ciaa = 0;
	hrtmon_ciab = 0;

	if (flags == ROMTYPE_XPOWER) { /* xpower */
		hrtmem_start = 0xe20000;
		hrtmem_size = 0x20000;
		hrtmem2_start = 0xf20000;
		hrtmem2_size =  0x10000;
		hrtmem_rom = 1;
		memname1 = _T("xpower_e2");
		memname2 = _T("xpower_f2");
	} else if (flags == ROMTYPE_NORDIC) { /* nordic */
		hrtmem_start = 0xf00000;
		hrtmem_size = 0x10000;
		hrtmem_end = 0xf20000;
		hrtmem2_start = 0xf40000;
		hrtmem2_end = 0xf60000;
		hrtmem2_size =  0x10000;
		hrtmem_rom = 1;
		memname1 = _T("nordic_f0");
		memname2 = _T("nordic_f4");
		if (rd->ver >= 3) {
			hrtmem_start += 0x60000;
			hrtmem_end += 0x60000;
			memname1 = _T("nordic_f6");
		}
	} else { /* super4 */
		hrtmem_start = 0xd00000;
		hrtmem_size = 0x40000;
		hrtmem2_start = 0xb00000;
		hrtmem2_size =  0x100000;
		hrtmem2_size2 = 0x0c0000;
		hrtmem3_start = 0xe00000;
		hrtmem3_size = 0x80000;
		memname1 = _T("superiv_d0");
		memname2 = _T("superiv_b0");
		memname3 = _T("superiv_e0");
	}
	if (hrtmem2_size && !hrtmem2_size2)
		hrtmem2_size2 = hrtmem2_size;

	hrtmem_bank.reserved_size = hrtmem_size;
	hrtmem_bank.label = memname1;
	hrtmem_mask = hrtmem_size - 1;
	mapped_malloc (&hrtmem_bank);
	hrtmemory = hrtmem_bank.baseaddr;
	memset (hrtmemory, 0x00, hrtmem_size);
	if (f) {
		zfile_fseek (f, 0, SEEK_SET);
		zfile_fread (hrtmemory, 1, hrtmem_size, f);
		zfile_fclose (f);
	}

	hrtmem2_mask = hrtmem2_size - 1;
	hrtmem2_bank.reserved_size = hrtmem2_size;
	hrtmem2_bank.label = memname2;
	if (hrtmem2_size) {
		mapped_malloc (&hrtmem2_bank);
		hrtmemory2 = hrtmem2_bank.baseaddr;
		memset(hrtmemory2, 0, hrtmem2_size);
	}

	hrtmem3_bank.reserved_size = hrtmem3_size;
	hrtmem3_bank.label = memname3;
	hrtmem3_mask = hrtmem3_size - 1;
	if (hrtmem3_size) {
		mapped_malloc (&hrtmem3_bank);
		hrtmemory3 = hrtmem3_bank.baseaddr;
		memset(hrtmemory3, 0, hrtmem3_size);
	}

	if (flags == ROMTYPE_XPOWER) {
		hrtmon_custom = hrtmemory2 + 0xfc00;
		hrtmon_ciaa = hrtmemory2 + 0xfc00;
		hrtmon_ciab = hrtmemory2 + 0xfc01;
		hrtmon_ciadiv = 2;
		chip += 0x30000;
		hrtmemory2[0xfc80] = chip >> 24;
		hrtmemory2[0xfc81] = chip >> 16;
		hrtmemory2[0xfc82] = chip >> 8;
		hrtmemory2[0xfc83] = chip >> 0;
	} else if (flags == ROMTYPE_NORDIC) {
		hrtmon_custom = hrtmemory2 + 0x3c00;
		hrtmon_ciaa = hrtmemory2 + 0x3c00;
		hrtmon_ciab = hrtmemory2 + 0x3c01;
		hrtmon_ciadiv = 2;
		hrtmon_zeropage = hrtmemory2 + 0; /* eh? why not just use CPU? */
	} else {
		hrtmon_custom = hrtmemory3 + 0x040000;
		hrtmon_ciaa = hrtmemory2 + 0x040000;
		hrtmon_ciab = hrtmemory2 + 0x040001;
		chip += 0x30000;
		hrtmemory2[0x80] = chip >> 24;
		hrtmemory2[0x81] = chip >> 16;
		hrtmemory2[0x82] = chip >> 8;
		hrtmemory2[0x83] = chip >> 0;
	}

	hrtmon_flag = ACTION_REPLAY_IDLE;
	write_log (_T("%s installed at %08X\n"), cart_memnames[cart_type], hrtmem_start);
	return 1;
}

int action_replay_load (void)
{
	struct zfile *f;
	uae_u8 header[8];
	struct romdata *rd;

	armodel = 0;
	action_replay_flag = ACTION_REPLAY_INACTIVE;
	/* Don't load a rom if one is already loaded. Use action_replay_unload () first. */
	if (armemory_rom || hrtmemory) {
		write_log (_T("action_replay_load () ROM already loaded.\n"));
		return 0;
	}
	if (_tcslen (currprefs.cartfile) == 0 || currprefs.cartfile[0] == ':')
		return 0;
	if (currprefs.cs_cd32fmv)
		return 0;

	rd = getromdatabypath (currprefs.cartfile);
	if (rd) {
		if (rd->id == 62)
			return superiv_init (rd, NULL);
		if (rd->type & ROMTYPE_CD32CART)
			return 0;
	}
	f = read_rom_name (currprefs.cartfile);
	if (!f) {
		write_log (_T("failed to load '%s' cartridge ROM\n"), currprefs.cartfile);
		return 0;
	}
	rd = getromdatabyzfile(f);
	if (!rd) {
		write_log (_T("Unknown cartridge ROM '%s'\n"), currprefs.cartfile);
	} else {
		int type = rd->type & ROMTYPE_MASK;
		if (type == ROMTYPE_SUPERIV || rd->type == ROMTYPE_NORDIC || rd->type == ROMTYPE_XPOWER) {
			return superiv_init (rd, f);
		}
	}
	zfile_fseek(f, 0, SEEK_END);
	ar_rom_file_size = zfile_ftell(f);
	zfile_fseek(f, 0, SEEK_SET);
	zfile_fread (header, 1, sizeof header, f);
	zfile_fseek (f, 0, SEEK_SET);
	if (!memcmp (header, "ATZ!HRT!", 8)) {
		zfile_fclose (f);
		return 0;
	}
	if (ar_rom_file_size != 65536 && ar_rom_file_size != 131072 && ar_rom_file_size != 262144) {
		write_log (_T("rom size must be 64KB (AR1), 128KB (AR2) or 256KB (AR3)\n"));
		zfile_fclose(f);
		return 0;
	}
	action_replay_flag = ACTION_REPLAY_INACTIVE;
	armemory_rom = xmalloc (uae_u8, ar_rom_file_size);
	zfile_fread (armemory_rom, 1, ar_rom_file_size, f);
	zfile_fclose (f);
	if (ar_rom_file_size == 65536) {
		// AR1 and Pro Access
		armodel = 1;
		arrom_start = 0xf00000;
		arrom_size = 0x10000;
		/* real AR1 RAM location is 0x9fc000-0x9fffff */
		arram_start = 0x9f0000;
		arram_size = 0x10000;
	} else {
		armodel = ar_rom_file_size / 131072 + 1;
		arrom_start = 0x400000;
		arrom_size = armodel == 2 ? 0x20000 : 0x40000;
		arram_start = 0x440000;
		arram_size = 0x10000;
	}
	arram_mask = arram_size - 1;
	arrom_mask = arrom_size - 1;
	armemory_ram = xcalloc (uae_u8, arram_size);
	write_log (_T("Action Replay %d installed at %08X, size %08X\n"), armodel, arrom_start, arrom_size);
	action_replay_version();
	return armodel;
}

void action_replay_init (int activate)
{
	if (!armemory_rom)
		return;
	hide_cart (0);
	if (armodel > 1)
		hide_cart (1);
	if (activate) {
		if (armodel > 1)
			action_replay_flag = ACTION_REPLAY_WAITRESET;
	}
}

/* This only deallocates memory, it is not suitable for unloading roms and continuing */
void action_replay_cleanup()
{
	if (armemory_rom)
		free (armemory_rom);
	if (armemory_ram)
		free (armemory_ram);
	mapped_free (&hrtmem_bank);
	mapped_free (&hrtmem2_bank);
	mapped_free (&hrtmem3_bank);

	armemory_rom = 0;
	armemory_ram = 0;
	hrtmemory = 0;
	hrtmemory2 = 0;
	hrtmemory3 = 0;
	hrtmem_size = 0;
	hrtmem2_size = 0;
	hrtmem2_size2 = 0;
	hrtmem3_size = 0;
	cart_type = 0;
	hrtmem_rom = 0;
	hrtmon_ciadiv = 256;
	hrtmon_zeropage = 0;
	hrtmem_end = 0;
	hrtmem2_end = 0;
}

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

int hrtmon_lang = 0;

static void hrtmon_configure(void)
{
	HRTCFG *cfg = (HRTCFG*)hrtmemory;
	if (cart_type != CART_HRTMON || armodel || !cfg)
		return;
	cfg->col0h = 0x00; cfg->col0l = 0x5a;
	cfg->col1h = 0x0f; cfg->col1l = 0xff;
	cfg->aga = (currprefs.chipset_mask & CSMASK_AGA) ? 1 : 0;
	cfg->cd32 = currprefs.cs_cd32cd ? 1 : 0;
	cfg->screenmode = currprefs.ntscmode;
	cfg->novbr = TRUE;
	cfg->hexmode = TRUE;
	cfg->entered = 0;
	cfg->keyboard = hrtmon_lang;
	do_put_mem_long (&cfg->max_chip, currprefs.chipmem_size);
	do_put_mem_long (&cfg->mon_size, 0x800000);
	cfg->ide = currprefs.cs_ide ? 1 : 0;
	cfg->a1200 = currprefs.cs_ide == IDE_A600A1200 ? 1 : 0; /* type of IDE interface, not Amiga model */
}

int hrtmon_load (void)
{
	struct zfile *f;
	uae_u32 header[4];
	struct romdata *rd;
	int isinternal = 0;

	/* Don't load a rom if one is already loaded. Use action_replay_unload () first. */
	if (armemory_rom)
		return 0;
	if (hrtmemory)
		return 0;

	triggered_once = 0;
	armodel = 0;
	cart_type = CART_AR;
	hrtmem_start = 0xa10000;
	if(!_tcscmp(currprefs.cartfile, _T(":HRTMon")))
	  rd = getromdatabyid(63);
	else
  	rd = getromdatabypath(currprefs.cartfile);
	if (rd) {
		if (rd->id == 63)
			isinternal = 1;
		if (rd->type & ROMTYPE_CD32CART)
			return 0;
	}

	if (!isinternal) {
		if (_tcslen (currprefs.cartfile) == 0)
			return 0;
		f = read_rom_name (currprefs.cartfile);
		if(!f) {
			write_log (_T("failed to load '%s' cartridge ROM\n"), currprefs.cartfile);
			return 0;
		}
		zfile_fread(header, sizeof header, 1, f);
		if (!memcmp (header, "ATZ!", 4)) {
			cart_type = CART_AR1200;
			armodel = 1200;
			hrtmem_start = 0x800000;
		} else if (!memcmp (header, "HRT!", 4)) {
			cart_type = CART_HRTMON;
		} else {
			zfile_fclose (f);
			return 0;
		}
	}
	hrtmem_size = 0x100000;
	hrtmem_mask = hrtmem_size - 1;
	if (isinternal) {
#ifdef ACTION_REPLAY_HRTMON
		extern unsigned char hrtrom[];
		extern unsigned int hrtrom_len;
		struct zfile *zf;
		zf = zfile_fopen_data (_T("hrtrom.gz"), hrtrom_len, hrtrom);
		//	f = zfile_fopen (_T("d:\\amiga\\amiga\\hrtmon\\src\\hrtmon.rom"), _T("rb"), 0);
		f = zfile_gunzip (zf);
#else
		return 0;
#endif
		cart_type = CART_HRTMON;
	}
	hrtmem_bank.reserved_size = hrtmem_size;
	hrtmem_bank.label = _T("hrtmem");
	mapped_malloc (&hrtmem_bank);
	hrtmemory = hrtmem_bank.baseaddr;
	memset (hrtmemory, 0xff, 0x80000);
	zfile_fseek (f, 0, SEEK_SET);
	zfile_fread (hrtmemory, 1, 524288, f);
	zfile_fclose (f);
	hrtmon_configure ();
	hrtmon_custom = hrtmemory + 0x08f000;
	hrtmon_ciaa = hrtmemory + 0x08e000;
	hrtmon_ciab = hrtmemory + 0x08d000;
	hrtmem_bank.baseaddr = hrtmemory;
	hrtmon_flag = ACTION_REPLAY_IDLE;
	write_log (_T("%s installed at %08X\n"), cart_memnames[cart_type], hrtmem_start);
	return 1;
}

void hrtmon_map_banks (void)
{
	uaecptr addr;

	if (!hrtmemory)
		return;

	addr = hrtmem_start;
	while (addr != hrtmem_end) {
		map_banks (&hrtmem_bank, addr >> 16, hrtmem_size >> 16, 0);
		addr += hrtmem_size;
		if (!hrtmem_end)
			break;
	}
	if (hrtmem2_size) {
		addr = hrtmem2_start;
		while (addr != hrtmem2_end) {
			map_banks (&hrtmem2_bank, addr >> 16, hrtmem2_size2 >> 16, 0);
			addr += hrtmem2_size;
			if (!hrtmem2_end)
				break;
		}
	}
	if (hrtmem3_size)
		map_banks (&hrtmem3_bank, hrtmem3_start >> 16, hrtmem3_size >> 16, 0);
}

static void hrtmon_unmap_banks (void)
{
	uaecptr addr;

	if (!hrtmemory)
		return;

	addr = hrtmem_start;
	while (addr != hrtmem_end) {
		map_banks (&dummy_bank, addr >> 16, hrtmem_size >> 16, 0);
		addr += hrtmem_size;
		if (!hrtmem_end)
			break;
	}
	if (hrtmem2_size) {
		addr = hrtmem2_start;
		while (addr != hrtmem2_end) {
			map_banks (&dummy_bank, addr >> 16, hrtmem2_size2 >> 16, 0);
			addr += hrtmem2_size;
			if (!hrtmem2_end)
				break;
		}
	}
	if (hrtmem3_size)
		map_banks (&dummy_bank, hrtmem3_start >> 16, hrtmem3_size >> 16, 0);
}

#define AR_VER_STR_OFFSET 0x4 /* offset in the rom where the version string begins. */
#define AR_VER_STR_END 0x7c   /* offset in the rom where the version string ends. */
#define AR_VER_STR_LEN (AR_VER_STR_END - AR_VER_STR_OFFSET)
static uae_char arVersionString[AR_VER_STR_LEN+1];

/* This function extracts the version info for AR2 and AR3. */

void action_replay_version(void)
{
	char* tmp;
	int iArVersionMajor = -1 ;
	int iArVersionMinor = -1;
	char* pNext;
	uae_char sArDate[11];

	*sArDate = '\0';
	if (!armemory_rom)
		return;

	if (armodel == 1)
		return; /* no support yet. */

	/* Extract Version string */
	memcpy(arVersionString, armemory_rom+AR_VER_STR_OFFSET, AR_VER_STR_LEN);
	arVersionString[AR_VER_STR_LEN]= '\0';
	tmp = strchr(arVersionString, 0x0d);
	if (tmp) {
		*tmp = '\0';
	}

	tmp = strchr(arVersionString,')');
	if (tmp) {
		*tmp = '\0';
		tmp = strchr(arVersionString, '(');
		if (tmp) {
			if (*(tmp + 1) == 'V') {
				pNext = tmp + 2;
				tmp = strchr(pNext, '.');
				if (tmp) {
					*tmp = '\0';
					iArVersionMajor = atoi(pNext);
					pNext = tmp+1;
					tmp = strchr(pNext, ' ');
					if (tmp) {
						*tmp = '\0';
						iArVersionMinor = atoi(pNext);
					}
					pNext = tmp+1;
					strcpy (sArDate, pNext);
				}

			}
		}
	}

	if (iArVersionMajor > 0) {
		TCHAR *s = au (sArDate);
		write_log (_T("Version of cart is '%d.%.02d', date is '%s'\n"), iArVersionMajor, iArVersionMinor, s);
		xfree (s);
	}
}

/* This function doesn't reset the Cart memory, it is just called during a memory reset */
void action_replay_memory_reset (void)
{
#ifdef ACTION_REPLAY_HRTMON
	if (hrtmemory) {
		hrtmon_hide (); /* It is never really idle */
		hrtmon_flag = ACTION_REPLAY_IDLE;
		if (cart_type == CART_SUPER4)
			hrtmon_map_banks ();
	}
#endif
#ifdef ACTION_REPLAY_COMMON
	check_prefs_changed_carts (1);
#endif
	action_replay_patch ();
	action_replay_checksum_info ();
	action_replay_setbanks ();
	hrtmon_configure ();
	if (armodel == 1)
		action_replay_flag = ACTION_REPLAY_ACTIVE;
}

uae_u8 *save_hrtmon (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (!hrtmemory)
		return 0;
	if (!triggered_once)
		return 0;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = (uae_u8*)malloc (hrtmem_size + hrtmem2_size + sizeof ar_custom + sizeof ar_ciaa + sizeof ar_ciab + 1024);
	save_u8 (cart_type);
	save_u8 (0);
	save_u32 (0);
	save_string (currprefs.cartfile);
	save_u32 (0);
	if (!hrtmem_rom) {
		save_u32 (hrtmem_size);
		memcpy (dst, hrtmemory, hrtmem_size);
		dst += hrtmem_size;
	} else if (hrtmem2_size) {
		save_u32 (hrtmem2_size);
		memcpy (dst, hrtmemory2, hrtmem2_size);
		dst += hrtmem2_size;
	} else {
		save_u32 (0);
	}
	save_u32 (sizeof ar_custom);
	memcpy (dst, ar_custom, sizeof ar_custom);
	dst += sizeof ar_custom;
	save_u32 (sizeof ar_ciaa);
	memcpy (dst, ar_ciaa, sizeof ar_ciaa);
	dst += sizeof ar_ciaa;
	save_u32 (sizeof ar_ciab);
	memcpy (dst, ar_ciab, sizeof ar_ciab);
	dst += sizeof ar_ciab;
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_hrtmon (uae_u8 *src)
{
	uae_u32 size;
	TCHAR *s;

	action_replay_unload (1);
	restore_u8 ();
	restore_u8 ();
	restore_u32 ();
	s = restore_string ();
	_tcsncpy (changed_prefs.cartfile, s, 255);
	xfree (s);
	_tcscpy (currprefs.cartfile, changed_prefs.cartfile);
	hrtmon_load ();
	action_replay_load ();
	if (restore_u32 () != 0)
		return src;
	size = restore_u32 ();
	if (!hrtmem_rom) {
		if (hrtmemory)
			memcpy (hrtmemory, src, size);
	} else if (hrtmem2_size) {
		if (hrtmemory2)
			memcpy (hrtmemory2, src, size);
	}
	src += size;
	restore_u32 ();
	memcpy (ar_custom, src, sizeof ar_custom);
	src += sizeof ar_custom;
	restore_u32 ();
	memcpy (ar_ciaa, src, sizeof ar_ciaa);
	src += sizeof ar_ciaa;
	restore_u32 ();
	memcpy (ar_ciab, src, sizeof ar_ciab);
	src += sizeof ar_ciab;
	return src;
}

uae_u8 *save_action_replay (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (!armemory_ram || !armemory_rom || !armodel)
		return 0;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, arram_size + sizeof ar_custom + sizeof ar_ciaa + sizeof ar_ciab + 1024);
	save_u8 (0);
	save_u8 (armodel);
	save_u32 (get_crc32 (armemory_rom + 4, arrom_size - 4));
	save_string (currprefs.cartfile);
	save_u32 (arrom_size);
	save_u32 (arram_size);
	memcpy (dst, armemory_ram, arram_size);
	dst += arram_size;
	save_u32 (sizeof ar_custom);
	memcpy (dst, ar_custom, sizeof ar_custom);
	dst += sizeof ar_custom;
	save_u32 (sizeof ar_ciaa);
	memcpy (dst, ar_ciaa, sizeof ar_ciaa);
	dst += sizeof ar_ciaa;
	save_u32 (sizeof ar_ciab);
	memcpy (dst, ar_ciab, sizeof ar_ciab);
	dst += sizeof ar_ciab;
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_action_replay (uae_u8 *src)
{
	TCHAR *s;

	action_replay_unload (1);
	restore_u8 ();
	armodel = restore_u8 ();
	if (!armodel)
		return src;
	restore_u32 ();
	s = restore_string ();
	_tcsncpy (changed_prefs.cartfile, s, 255);
	_tcscpy (currprefs.cartfile, changed_prefs.cartfile);
	xfree (s);
	action_replay_load ();
	if (restore_u32 () != arrom_size)
		return src;
	if (restore_u32 () != arram_size)
		return src;
	if (armemory_ram)
		memcpy (armemory_ram, src, arram_size);
	src += arram_size;
	restore_u32 ();
	memcpy (ar_custom, src, sizeof ar_custom);
	src += sizeof ar_custom;
	restore_u32 ();
	src += sizeof ar_ciaa;
	restore_u32 ();
	src += sizeof ar_ciab;
	action_replay_flag = ACTION_REPLAY_IDLE;
	if (is_ar_pc_in_rom ())
		action_replay_flag = ACTION_REPLAY_ACTIVE;
	return src;
}


#define NPSIZE 65536

static uae_u8 bswap (uae_u8 v,int b7,int b6,int b5,int b4,int b3,int b2,int b1,int b0)
{
	uae_u8 b = 0;

	b |= ((v >> b7) & 1) << 7;
	b |= ((v >> b6) & 1) << 6;
	b |= ((v >> b5) & 1) << 5;
	b |= ((v >> b4) & 1) << 4;
	b |= ((v >> b3) & 1) << 3;
	b |= ((v >> b2) & 1) << 2;
	b |= ((v >> b1) & 1) << 1;
	b |= ((v >> b0) & 1) << 0;
	return b;
}

static uae_u16 wswap (uae_u16 v,int b15,int b14,int b13,int b12, int b11, int b10, int b9, int b8, int b7,int b6,int b5,int b4,int b3,int b2,int b1,int b0)
{
	uae_u16 b = 0;

	b |= ((v >> b15) & 1) << 15;
	b |= ((v >> b14) & 1) << 14;
	b |= ((v >> b13) & 1) << 13;
	b |= ((v >> b12) & 1) << 12;
	b |= ((v >> b11) & 1) << 11;
	b |= ((v >> b10) & 1) << 10;
	b |= ((v >> b9) & 1) << 9;
	b |= ((v >> b8) & 1) << 8;
	b |= ((v >> b7) & 1) << 7;
	b |= ((v >> b6) & 1) << 6;
	b |= ((v >> b5) & 1) << 5;
	b |= ((v >> b4) & 1) << 4;
	b |= ((v >> b3) & 1) << 3;
	b |= ((v >> b2) & 1) << 2;
	b |= ((v >> b1) & 1) << 1;
	b |= ((v >> b0) & 1) << 0;
	return b;
}

#define AXOR 0x817f

// middle (even)
static void descramble1 (uae_u8 *buf, int size)
{
	int i;

	for (i = 0; i < size; i++)
		buf[i] = bswap (buf[i], 4, 1, 5, 3, 0, 7, 6, 2);
}
static void descramble1a (uae_u8 *buf, int size)
{
	int i;
	uae_u8 tbuf[NPSIZE];

	memcpy (tbuf, buf, size);
	for (i = 0; i < size; i++) {
		int a = (i ^ AXOR) & (size - 1);
		buf[i] = tbuf[wswap (a, 15, 9, 10, 4, 6, 5, 3, 8, 14, 13, 0, 12, 11, 2, 1, 7)];
	}
}
// corner (odd)
static void descramble2 (uae_u8 *buf, int size)
{
	int i;

	for (i = 0; i < size; i++)
		buf[i] = bswap (buf[i], 5, 4, 3, 2, 1, 0, 7, 6);
}
static void descramble2a (uae_u8 *buf, int size)
{
	int i;
	uae_u8 tbuf[NPSIZE];

	memcpy (tbuf, buf, size);
	for (i = 0; i < size; i++) {
		int a = (i ^ AXOR) & (size - 1);
		buf[i] = tbuf[wswap (a, 15, 2, 4, 0, 1, 10, 11, 8, 13, 14, 12, 9, 7, 5, 6, 3)];
	}
}
void descramble_nordicpro (uae_u8 *buf, int size, int odd)
{
	if (odd) {
		descramble2 (buf, size);
		descramble2a (buf, size);
	} else {
		descramble1 (buf, size);
		descramble1a (buf, size);
	}
}
