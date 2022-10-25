/*
 * cpummu.cpp -  MMU emulation
 *
 * Copyright (c) 2001-2004 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by UAE MMU patch
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "cpummu.h"
#include "debug.h"

#define MMUDUMP 1

#define DBG_MMU_VERBOSE	1
#define DBG_MMU_SANITY	1
#if 0
#define write_log printf
#endif

#ifdef FULLMMU

uae_u32 mmu_is_super;
uae_u32 mmu_tagmask, mmu_pagemask, mmu_pagemaski;
struct mmu_atc_line mmu_atc_array[ATC_TYPE][ATC_SLOTS][ATC_WAYS];
bool mmu_pagesize_8k;
int mmu_pageshift, mmu_pageshift1m;
uae_u8 mmu_cache_state;
uae_u8 cache_default_ins, cache_default_data;

int mmu060_state;
uae_u16 mmu_opcode;
bool mmu_restart;
static bool locked_rmw_cycle;
bool rmw_cycle;
static bool ismoves;
bool mmu_ttr_enabled, mmu_ttr_enabled_ins, mmu_ttr_enabled_data;
int mmu_atc_ways[2];
int way_random;

int mmu040_movem;
uaecptr mmu040_movem_ea;
uae_u32 mmu040_move16[4];

#if MMU_ICACHE
struct mmu_icache mmu_icache_data[MMU_ICACHE_SZ];
#endif
#if MMU_IPAGECACHE
uae_u32 atc_last_ins_laddr, atc_last_ins_paddr;
uae_u8 atc_last_ins_cache;
#endif
#if MMU_DPAGECACHE
struct mmufastcache atc_data_cache_read[MMUFASTCACHE_ENTRIES];
struct mmufastcache atc_data_cache_write[MMUFASTCACHE_ENTRIES];
#endif

#if CACHE_HIT_COUNT
int mmu_ins_hit, mmu_ins_miss;
int mmu_data_read_hit, mmu_data_read_miss;
int mmu_data_write_hit, mmu_data_write_miss;
#endif

static void mmu_dump_ttr(const TCHAR * label, uae_u32 ttr)
{
	DUNUSED(label);
#if MMUDEBUG > 0
	uae_u32 from_addr, to_addr;

	from_addr = ttr & MMU_TTR_LOGICAL_BASE;
	to_addr = (ttr & MMU_TTR_LOGICAL_MASK) << 8;

	write_log(_T("%s: [%08x] %08x - %08x enabled=%d supervisor=%d wp=%d cm=%02d\n"),
			label, ttr,
			from_addr, to_addr,
			ttr & MMU_TTR_BIT_ENABLED ? 1 : 0,
			(ttr & (MMU_TTR_BIT_SFIELD_ENABLED | MMU_TTR_BIT_SFIELD_SUPER)) >> MMU_TTR_SFIELD_SHIFT,
			ttr & MMU_TTR_BIT_WRITE_PROTECT ? 1 : 0,
			(ttr & MMU_TTR_CACHE_MASK) >> MMU_TTR_CACHE_SHIFT
		  );
#endif
}

void mmu_make_transparent_region(uaecptr baseaddr, uae_u32 size, int datamode)
{
	uae_u32 * ttr;
	uae_u32 * ttr0 = datamode ? &regs.dtt0 : &regs.itt0;
	uae_u32 * ttr1 = datamode ? &regs.dtt1 : &regs.itt1;

	if ((*ttr1 & MMU_TTR_BIT_ENABLED) == 0)
		ttr = ttr1;
	else if ((*ttr0 & MMU_TTR_BIT_ENABLED) == 0)
		ttr = ttr0;
	else
		return;

	*ttr = baseaddr & MMU_TTR_LOGICAL_BASE;
	*ttr |= ((baseaddr + size - 1) & MMU_TTR_LOGICAL_BASE) >> 8;
	*ttr |= MMU_TTR_BIT_ENABLED;

#if MMUDEBUG > 0
	write_log(_T("MMU: map transparent mapping of %08x\n"), *ttr);
#endif
}

void mmu_tt_modified (void)
{
	mmu_ttr_enabled_ins = ((regs.itt0 | regs.itt1) & MMU_TTR_BIT_ENABLED) != 0;
	mmu_ttr_enabled_data = ((regs.dtt0 | regs.dtt1) & MMU_TTR_BIT_ENABLED) != 0;
	mmu_ttr_enabled = mmu_ttr_enabled_ins || mmu_ttr_enabled_data;
}


#if MMUDUMP

/* This dump output makes much more sense than old one */

#define LEVELA_SIZE 7
#define LEVELB_SIZE 7
#define LEVELC_SIZE (mmu_pagesize_8k ? 5 : 6)
#define PAGE_SIZE_4k 12 // = 1 << 12 = 4096
#define PAGE_SIZE_8k 13 // = 1 << 13 = 8192

#define LEVELA_VAL(x) ((((uae_u32)(x)) >> (32 - (LEVELA_SIZE                            ))) & ((1 << LEVELA_SIZE) - 1))
#define LEVELB_VAL(x) ((((uae_u32)(x)) >> (32 - (LEVELA_SIZE + LEVELB_SIZE              ))) & ((1 << LEVELB_SIZE) - 1))
#define LEVELC_VAL(x) ((((uae_u32)(x)) >> (32 - (LEVELA_SIZE + LEVELB_SIZE + LEVELC_SIZE))) & ((1 << LEVELC_SIZE) - 1))

#define LEVELA(root, x) (get_long(root + LEVELA_VAL(x) * 4))
#define LEVELB(a, x) (get_long((((uae_u32)a) & ~((1 << (LEVELB_SIZE + 2)) - 1)) + LEVELB_VAL(x) * 4))
#define LEVELC(b, x) (get_long((((uae_u32)b) & ~((1 << (LEVELC_SIZE + 2)) - 1)) + LEVELC_VAL(x) * 4))

#define ISINVALID(x) ((((unsigned long)x) & 3) == 0)

static uae_u32 getdesc(uae_u32 root, uae_u32 addr)
{
	unsigned long desc;

	desc = LEVELA(root, addr);
	if (ISINVALID(desc))
		return desc;
	desc = LEVELB(desc, addr);
	if (ISINVALID(desc))
		return desc;
	desc = LEVELC(desc, addr);
	return desc;
}
static void mmu_dump_table(const char * label, uaecptr root_ptr)
{
	unsigned long i;
	unsigned long startaddr;
	unsigned long odesc;
	unsigned long totalpages;
	unsigned long page_size = mmu_pagesize_8k ? PAGE_SIZE_8k : PAGE_SIZE_4k;
	unsigned long pagemask = (1 << page_size) - 1;
	unsigned long descmask = pagemask & ~(0x08 | 0x10); // mask out unused and M bits

	root_ptr &= 0xfffffe00;
	write_log(_T("MMU dump start. Root = %08x. Page = %d\n"), root_ptr, 1 << page_size);
	totalpages = 1 << (32 - page_size);
	startaddr = 0;
	odesc = getdesc(root_ptr, startaddr);
	for (i = 0; i <= totalpages; i++) {
		unsigned long addr = i << page_size;
		unsigned long desc = 0;
		if (i < totalpages)
			desc = getdesc(root_ptr, addr);
		if ((desc & descmask) != (odesc & descmask) || i == totalpages) {
			uae_u8 cm, sp;
			cm = (odesc >> 5) & 3;
			sp = (odesc >> 7) & 1;
			write_log(_T("%08x - %08x: %08x WP=%d S=%d CM=%d (%08x)\n"),
				startaddr, addr - 1, odesc & ~((1 << page_size) - 1),
				(odesc & 4) ? 1 : 0, sp, cm, odesc);
			startaddr = addr;
			odesc = desc;
		}
	}
	write_log(_T("MMU dump end\n"));
}			

#else
/* {{{ mmu_dump_table */
static void mmu_dump_table(const char * label, uaecptr root_ptr)
{
	DUNUSED(label);
	const int ROOT_TABLE_SIZE = 128,
		PTR_TABLE_SIZE = 128,
		PAGE_TABLE_SIZE = 64,
		ROOT_INDEX_SHIFT = 25,
		PTR_INDEX_SHIFT = 18;
	// const int PAGE_INDEX_SHIFT = 12;
	int root_idx, ptr_idx, page_idx;
	uae_u32 root_des, ptr_des, page_des;
	uaecptr ptr_des_addr, page_addr,
		root_log, ptr_log, page_log;

	write_log(_T("%s: root=%x\n"), label, root_ptr);

	for (root_idx = 0; root_idx < ROOT_TABLE_SIZE; root_idx++) {
		root_des = phys_get_long(root_ptr + root_idx);

		if ((root_des & 2) == 0)
			continue;	/* invalid */

		write_log(_T("ROOT: %03d U=%d W=%d UDT=%02d\n"), root_idx,
				root_des & 8 ? 1 : 0,
				root_des & 4 ? 1 : 0,
				root_des & 3
			  );

		root_log = root_idx << ROOT_INDEX_SHIFT;

		ptr_des_addr = root_des & MMU_ROOT_PTR_ADDR_MASK;

		for (ptr_idx = 0; ptr_idx < PTR_TABLE_SIZE; ptr_idx++) {
			struct {
				uaecptr	log, phys;
				int start_idx, n_pages;	/* number of pages covered by this entry */
				uae_u32 match;
			} page_info[PAGE_TABLE_SIZE];
			int n_pages_used;

			ptr_des = phys_get_long(ptr_des_addr + ptr_idx);
			ptr_log = root_log | (ptr_idx << PTR_INDEX_SHIFT);

			if ((ptr_des & 2) == 0)
				continue; /* invalid */

			page_addr = ptr_des & (mmu_pagesize_8k ? MMU_PTR_PAGE_ADDR_MASK_8 : MMU_PTR_PAGE_ADDR_MASK_4);

			n_pages_used = -1;
			for (page_idx = 0; page_idx < PAGE_TABLE_SIZE; page_idx++) {

				page_des = phys_get_long(page_addr + page_idx);
				page_log = ptr_log | (page_idx << 2);		// ??? PAGE_INDEX_SHIFT

				switch (page_des & 3) {
					case 0: /* invalid */
						continue;
					case 1: case 3: /* resident */
					case 2: /* indirect */
						if (n_pages_used == -1 || page_info[n_pages_used].match != page_des) {
							/* use the next entry */
							n_pages_used++;

							page_info[n_pages_used].match = page_des;
							page_info[n_pages_used].n_pages = 1;
							page_info[n_pages_used].start_idx = page_idx;
							page_info[n_pages_used].log = page_log;
						} else {
							page_info[n_pages_used].n_pages++;
						}
						break;
				}
			}

			if (n_pages_used == -1)
				continue;

			write_log(_T(" PTR: %03d U=%d W=%d UDT=%02d\n"), ptr_idx,
				ptr_des & 8 ? 1 : 0,
				ptr_des & 4 ? 1 : 0,
				ptr_des & 3
			  );


			for (page_idx = 0; page_idx <= n_pages_used; page_idx++) {
				page_des = page_info[page_idx].match;

				if ((page_des & MMU_PDT_MASK) == 2) {
					write_log(_T("  PAGE: %03d-%03d log=%08x INDIRECT --> addr=%08x\n"),
							page_info[page_idx].start_idx,
							page_info[page_idx].start_idx + page_info[page_idx].n_pages - 1,
							page_info[page_idx].log,
							page_des & MMU_PAGE_INDIRECT_MASK
						  );

				} else {
					write_log(_T("  PAGE: %03d-%03d log=%08x addr=%08x UR=%02d G=%d U1/0=%d S=%d CM=%d M=%d U=%d W=%d\n"),
							page_info[page_idx].start_idx,
							page_info[page_idx].start_idx + page_info[page_idx].n_pages - 1,
							page_info[page_idx].log,
							page_des & (mmu_pagesize_8k ? MMU_PAGE_ADDR_MASK_8 : MMU_PAGE_ADDR_MASK_4),
							(page_des & (mmu_pagesize_8k ? MMU_PAGE_UR_MASK_8 : MMU_PAGE_UR_MASK_4)) >> MMU_PAGE_UR_SHIFT,
							page_des & MMU_DES_GLOBAL ? 1 : 0,
							(page_des & MMU_TTR_UX_MASK) >> MMU_TTR_UX_SHIFT,
							page_des & MMU_DES_SUPER ? 1 : 0,
							(page_des & MMU_TTR_CACHE_MASK) >> MMU_TTR_CACHE_SHIFT,
							page_des & MMU_DES_MODIFIED ? 1 : 0,
							page_des & MMU_DES_USED ? 1 : 0,
							page_des & MMU_DES_WP ? 1 : 0
						  );
				}
			}
		}

	}
}
/* }}} */
#endif

/* {{{ mmu_dump_atc */
static void mmu_dump_atc(void)
{

}
/* }}} */

/* {{{ mmu_dump_tables */
void mmu_dump_tables(void)
{
	write_log(_T("URP: %08x   SRP: %08x  MMUSR: %x  TC: %x\n"), regs.urp, regs.srp, regs.mmusr, regs.tcr);
	mmu_dump_ttr(_T("DTT0"), regs.dtt0);
	mmu_dump_ttr(_T("DTT1"), regs.dtt1);
	mmu_dump_ttr(_T("ITT0"), regs.itt0);
	mmu_dump_ttr(_T("ITT1"), regs.itt1);
	mmu_dump_atc();
#if MMUDUMP
	mmu_dump_table("SRP", regs.srp);
	if (regs.urp != regs.srp)
		mmu_dump_table("URP", regs.urp);
#endif
}
/* }}} */

static void flush_shortcut_cache(uaecptr addr, bool super)
{
#if MMU_IPAGECACHE
	atc_last_ins_laddr = mmu_pagemask;
#endif
#if MMU_DPAGECACHE
	if (addr == 0xffffffff) {
		memset(&atc_data_cache_read, 0xff, sizeof atc_data_cache_read);
		memset(&atc_data_cache_write, 0xff, sizeof atc_data_cache_write);
	} else {
		for (int i = 0; i < MMUFASTCACHE_ENTRIES; i++) {
			uae_u32 idx = ((addr & mmu_pagemaski) >> mmu_pageshift1m) | (super ? 1 : 0);
			if (atc_data_cache_read[i].log == idx)
				atc_data_cache_read[i].log = 0xffffffff;
			if (atc_data_cache_write[i].log == idx)
				atc_data_cache_write[i].log = 0xffffffff;
		}
	}
#endif
}

static ALWAYS_INLINE int mmu_get_fc(bool super, bool data)
{
	return (super ? 4 : 0) | (data ? 1 : 2);
}

void mmu_hardware_bus_error(uaecptr addr, uae_u32 v, bool read, bool ins, int size)
{
	uae_u32 fc;
	
	if (ismoves) {
		fc = read ? regs.sfc : regs.dfc;
	} else {
		fc = (regs.s ? 4 : 0) | (ins ? 2 : 1);
	}
	mmu_bus_error(addr, v, fc, !read, size, 0, true);
}

bool mmu_is_super_access(bool read)
{
	if (!ismoves) {
		return regs.s;
	} else {
		uae_u32 fc = read ? regs.sfc : regs.dfc;
		return (fc & 4) != 0;
	}
}

void mmu_bus_error(uaecptr addr, uae_u32 val, int fc, bool write, int size,uae_u32 status060, bool nonmmu)
{
	if (currprefs.mmu_model == 68040) {
		uae_u16 ssw = 0;

		if (ismoves) {
			// MOVES special behavior
			int fc2 = write ? regs.dfc : regs.sfc;
			if (fc2 == 0 || fc2 == 3 || fc2 == 4 || fc2 == 7)
				ssw |= MMU_SSW_TT1;
			if ((fc2 & 3) != 3)
				fc2 &= ~2;
#if MMUDEBUGMISC > 0
			write_log (_T("040 MMU MOVES fc=%d -> %d\n"), fc, fc2);
#endif
			fc = fc2;
		}

		ssw |= fc & MMU_SSW_TM;				/* TM = FC */

		switch (size) {
		case sz_byte:
			ssw |= MMU_SSW_SIZE_B;
			break;
		case sz_word:
			ssw |= MMU_SSW_SIZE_W;
			break;
		case sz_long:
			ssw |= MMU_SSW_SIZE_L;
			break;
		}

		regs.wb3_status = write ? 0x80 | (ssw & 0x7f) : 0;
		regs.wb3_data = val;
		regs.wb2_status = 0;
		if (!write)
			ssw |= MMU_SSW_RW;

		if (size == 16) { // MOVE16
			ssw |= MMU_SSW_SIZE_CL;
			ssw |= MMU_SSW_TT0;
			regs.mmu_effective_addr &= ~15;
			if (write) {
				// clear normal writeback if MOVE16 write
				regs.wb3_status &= ~0x80;
				// wb2 = cacheline size writeback
				regs.wb2_status = 0x80 | MMU_SSW_SIZE_CL | (ssw & 0x1f);
				regs.wb2_address = regs.mmu_effective_addr;
				write_log (_T("040 MMU MOVE16 WRITE FAULT!\n"));
			}
		}

		if (mmu040_movem) {
			ssw |= MMU_SSW_CM;
			regs.mmu_effective_addr = mmu040_movem_ea;
			mmu040_movem = 0;
#if MMUDEBUGMISC > 0
			write_log (_T("040 MMU_SSW_CM EA=%08X\n"), mmu040_movem_ea);
#endif
		}
		if (locked_rmw_cycle) {
			ssw |= MMU_SSW_LK;
			ssw &= ~MMU_SSW_RW;
#if MMUDEBUGMISC > 0
			write_log (_T("040 MMU_SSW_LK!\n"));
#endif
		}

		if (!nonmmu)
			ssw |= MMU_SSW_ATC;
		regs.mmu_ssw = ssw;

#if MMUDEBUG > 0
		write_log(_T("BF: fc=%d w=%d logical=%08x ssw=%04x PC=%08x INS=%04X\n"), fc, write, addr, ssw, m68k_getpc(), mmu_opcode);
#endif
	} else {
		uae_u32 fslw = 0;

		fslw |= write ? MMU_FSLW_W : MMU_FSLW_R;
		fslw |= fc << 16; /* MMU_FSLW_TM */

		switch (size) {
		case sz_byte:
			fslw |= MMU_FSLW_SIZE_B;
			break;
		case sz_word:
			fslw |= MMU_FSLW_SIZE_W;
			break;
		case sz_long:
			fslw |= MMU_FSLW_SIZE_L;
			break;
		case 16: // MOVE16
			addr &= ~15;
			fslw |= MMU_FSLW_SIZE_D;
			fslw |= MMU_FSLW_TT_16;
			break;
		}
		if ((fc & 3) == 2) {
			// instruction faults always point to opcode address
#if MMUDEBUGMISC > 0
			write_log(_T("INS FAULT %08x %08x %d\n"), addr, regs.instruction_pc, mmu060_state);
#endif
			addr = regs.instruction_pc;
			if (mmu060_state == 0) {
				fslw |= MMU_FSLW_IO; // opword fetch
			} else {
				fslw |= MMU_FSLW_IO | MMU_FSLW_MA; // extension word
			}
		}
		if (rmw_cycle) {
			fslw |=  MMU_FSLW_W | MMU_FSLW_R;
		}
		if (locked_rmw_cycle) {
			fslw |= MMU_FSLW_LK;
			write_log (_T("060 MMU_FSLW_LK!\n"));
		}
		fslw |= status060;
		regs.mmu_fslw = fslw;

#if MMUDEBUG > 0
		write_log(_T("BF: fc=%d w=%d s=%d log=%08x ssw=%08x rmw=%d PC=%08x INS=%04X\n"), fc, write, 1 << size, addr, fslw, rmw_cycle, m68k_getpc(), mmu_opcode);
#endif

	}

	rmw_cycle = false;
	locked_rmw_cycle = false;
	regs.mmu_fault_addr = addr;

#if 0
	activate_debugger ();
#endif

	cache_default_data &= ~CACHE_DISABLE_ALLOCATE;

	THROW(2);
}

/*
 * mmu access is a 4 step process:
 * if mmu is not enabled just read physical
 * check transparent region, if transparent, read physical
 * check ATC (address translation cache), read immediately if HIT
 * read from mmu with the long path (and allocate ATC entry if needed)
 */

/* check if an address matches a ttr */
static int mmu_do_match_ttr(uae_u32 ttr, uaecptr addr, bool super)
{
	if (ttr & MMU_TTR_BIT_ENABLED)	{	/* TTR enabled */
		uae_u8 msb, mask;
		
		msb = ((addr ^ ttr) & MMU_TTR_LOGICAL_BASE) >> 24;
		mask = (ttr & MMU_TTR_LOGICAL_MASK) >> 16;
		
		if (!(msb & ~mask)) {
			
			if ((ttr & MMU_TTR_BIT_SFIELD_ENABLED) == 0) {
				if (((ttr & MMU_TTR_BIT_SFIELD_SUPER) == 0) != (super == 0)) {
					return TTR_NO_MATCH;
				}
			}
			if (ttr & MMU_TTR_CACHE_DISABLE) {
				mmu_cache_state = CACHE_DISABLE_MMU;
			} else {
				mmu_cache_state = CACHE_ENABLE_ALL;
				if (ttr & MMU_TTR_CACHE_MODE) {
					mmu_cache_state |= CACHE_ENABLE_COPYBACK;
				}
			}
			return (ttr & MMU_TTR_BIT_WRITE_PROTECT) ? TTR_NO_WRITE : TTR_OK_MATCH;
		}
	}
	return TTR_NO_MATCH;
}

int mmu_match_ttr_ins(uaecptr addr, bool super)
{
	int res;
	
	if (!mmu_ttr_enabled_ins)
		return TTR_NO_MATCH;
	res = mmu_do_match_ttr(regs.itt0, addr, super);
	if (res == TTR_NO_MATCH)
		res = mmu_do_match_ttr(regs.itt1, addr, super);
	return res;
}

int mmu_match_ttr(uaecptr addr, bool super, bool data)
{
	int res;
	
	if (!mmu_ttr_enabled)
		return TTR_NO_MATCH;
	if (data) {
		res = mmu_do_match_ttr(regs.dtt0, addr, super);
		if (res == TTR_NO_MATCH)
			res = mmu_do_match_ttr(regs.dtt1, addr, super);
	} else {
		res = mmu_do_match_ttr(regs.itt0, addr, super);
		if (res == TTR_NO_MATCH)
			res = mmu_do_match_ttr(regs.itt1, addr, super);
	}
	return res;
}

void mmu_bus_error_ttr_write_fault(uaecptr addr, bool super, bool data, uae_u32 val, int size)
 {
	 uae_u32 status = 0;
	 
	 if (currprefs.mmu_model == 68060) {
		 status |= MMU_FSLW_TTR;
	 }
	mmu_bus_error(addr, val, mmu_get_fc (super, data), true, size, status, false);
}

int mmu_match_ttr_write(uaecptr addr, bool super, bool data, uae_u32 val, int size)
{
	int res = TTR_NO_MATCH;
	if (mmu_ttr_enabled) {
		res = mmu_match_ttr(addr, super, data);
	}
	if (res == TTR_NO_WRITE || (res == TTR_NO_MATCH && !regs.mmu_enabled && (regs.tcr & MMU_TCR_DWO)))
		mmu_bus_error_ttr_write_fault(addr, super, data, val, size);
	return res;
}

int mmu_match_ttr_maybe_write(uaecptr addr, bool super, bool data, int size, bool write)
{
	int res = TTR_NO_MATCH;
	if (mmu_ttr_enabled) {
		res = mmu_match_ttr(addr, super, data);
	}
	if (write && ((res == TTR_NO_WRITE) || (res == TTR_NO_MATCH && !regs.mmu_enabled && (regs.tcr & MMU_TCR_DWO))))
		mmu_bus_error_ttr_write_fault(addr, super, data, 0, size);
	return res;
}

// Descriptor read accesses can use data cache but never allocate new cache lines.
static uae_u32 desc_get_long(uaecptr addr)
{
	mmu_cache_state = ce_cachable[addr >>16] | CACHE_DISABLE_ALLOCATE;
	return x_phys_get_long(addr);
}
// Write accesses probably are always pushed to memomory
static void desc_put_long(uaecptr addr, uae_u32 v)
{
	mmu_cache_state = CACHE_DISABLE_MMU;
	x_phys_put_long(addr, v);
}

/*
 * Lookup the address by walking the page table and updating
 * the page descriptors accordingly. Returns the found descriptor
 * or produces a bus error.
 */
static uae_u32 mmu_fill_atc(uaecptr addr, bool super, uae_u32 tag, bool write, struct mmu_atc_line *l, uae_u32 *status060)
{
    uae_u32 desc, desc_addr, wp;
    uae_u32 status = 0;
    int i;
	int old_s;
    
    // Always use supervisor mode to access descriptors
    old_s = regs.s;
    regs.s = 1;

    wp = 0;
    desc = super ? regs.srp : regs.urp;
    
    /* fetch root table descriptor */
    i = (addr >> 23) & 0x1fc;
    desc_addr = (desc & MMU_ROOT_PTR_ADDR_MASK) | i;
    
    SAVE_EXCEPTION;
    TRY(prb) {
        desc = desc_get_long(desc_addr);
        if ((desc & 2) == 0) {
#if MMUDEBUG > 1
            write_log(_T("MMU: invalid root descriptor %s for %x desc at %x desc=%x\n"), super ? _T("srp"):_T("urp"),
                      addr, desc_addr, desc);
#endif
			*status060 |= MMU_FSLW_PTA;
            goto fail;
        }
        
        wp |= desc;
        if ((desc & MMU_DES_USED) == 0) {
            desc_put_long(desc_addr, desc | MMU_DES_USED);
		}
        
        /* fetch pointer table descriptor */
        i = (addr >> 16) & 0x1fc;
        desc_addr = (desc & MMU_ROOT_PTR_ADDR_MASK) | i;
        desc = desc_get_long(desc_addr);
        if ((desc & 2) == 0) {
#if MMUDEBUG > 1
            write_log(_T("MMU: invalid ptr descriptor %s for %x desc at %x desc=%x\n"), super ? _T("srp"):_T("urp"),
                      addr, desc_addr, desc);
#endif
			*status060 |= MMU_FSLW_PTB;
            goto fail;
        }
        wp |= desc;
        if ((desc & MMU_DES_USED) == 0)
            desc_put_long(desc_addr, desc | MMU_DES_USED);
        
        /* fetch page table descriptor */
        if (mmu_pagesize_8k) {
            i = (addr >> 11) & 0x7c;
            desc_addr = (desc & MMU_PTR_PAGE_ADDR_MASK_8) + i;
        } else {
            i = (addr >> 10) & 0xfc;
            desc_addr = (desc & MMU_PTR_PAGE_ADDR_MASK_4) + i;
        }
        
        desc = desc_get_long(desc_addr);
        if ((desc & 3) == 2) {
            /* indirect */
            desc_addr = desc & MMU_PAGE_INDIRECT_MASK;
            desc = desc_get_long(desc_addr);
        }
        if ((desc & 1) == 1) {
            wp |= desc;
            if (write) {
                if ((wp & MMU_DES_WP) || ((desc & MMU_DES_SUPER) && !super)) {
                    if ((desc & MMU_DES_USED) == 0) {
                        desc |= MMU_DES_USED;
                        desc_put_long(desc_addr, desc);
                    }
                } else if ((desc & (MMU_DES_USED|MMU_DES_MODIFIED)) !=
                           (MMU_DES_USED|MMU_DES_MODIFIED)) {
                    desc |= MMU_DES_USED|MMU_DES_MODIFIED;
                    desc_put_long(desc_addr, desc);
                }
            } else {
                if ((desc & MMU_DES_USED) == 0) {
                    desc |= MMU_DES_USED;
                    desc_put_long(desc_addr, desc);
                }
            }
            desc |= wp & MMU_DES_WP;
        } else {
            if ((desc & 3) == 2) {
				*status060 |= MMU_FSLW_IL;
#if MMUDEBUG > 1
                write_log(_T("MMU: double indirect descriptor log=%0x desc=%08x @%08x\n"), addr, desc, desc_addr);
#endif
            } else {
				*status060 |= MMU_FSLW_PF;
#if MMUDEBUG > 2
				write_log(_T("MMU: invalid page descriptor log=%0x desc=%08x @%08x\n"), addr, desc, desc_addr);
#endif
			}
fail:
            desc = 0;
        }
        
 		/* this will cause a bus error exception */
		if (!super && (desc & MMU_DES_SUPER)) {
			*status060 |= MMU_FSLW_SP;
		} else if (write && (desc & MMU_DES_WP)) {
			*status060 |= MMU_FSLW_WP;
		}

		// 68040 always creates ATC entry. 68060 only if valid descriptor was found.
		if (currprefs.mmu_model == 68040 || (desc & MMU_MMUSR_R)) {
			/* Create new ATC entry and return status */
			l->status = desc & (MMU_MMUSR_G|MMU_MMUSR_Ux|MMU_MMUSR_S|MMU_MMUSR_CM|MMU_MMUSR_M|MMU_MMUSR_W|MMU_MMUSR_R);
			l->phys = desc & mmu_pagemaski;
			l->valid = 1;
			l->tag = tag;
			status = l->phys | l->status;
		}

		RESTORE_EXCEPTION;
    } CATCH(prb) {
        RESTORE_EXCEPTION;

		/* bus error during table search */
		if (currprefs.mmu_model == 68040) {
	        l->status = 0;
	        l->phys = 0;
		    l->valid = 1;
		    l->tag = tag;
		}
        status = MMU_MMUSR_B;
		*status060 |= MMU_FSLW_LK | MMU_FSLW_TWE;

#if MMUDEBUG > 0
        write_log(_T("MMU: bus error during table search.\n"));
#endif
    } ENDTRY;

    // Restore original supervisor state
    regs.s = old_s;

#if MMUDEBUG > 2
    write_log(_T("translate: %x,%u,%u -> %x\n"), addr, super, write, desc);
#endif

	flush_shortcut_cache(addr, super);

	return status;
}

static void mmu_add_cache(uaecptr addr, uaecptr phys, bool super, bool data, bool write)
{
	if (!data) {
#if MMU_IPAGECACHE
		uae_u32 laddr = (addr & mmu_pagemaski) | (super ? 1 : 0);
		atc_last_ins_laddr = laddr;
		atc_last_ins_paddr = phys;
		atc_last_ins_cache = mmu_cache_state;
#else
	;
#endif
#if MMU_DPAGECACHE
	} else {
		uae_u32 idx1 = ((addr & mmu_pagemaski) >> mmu_pageshift1m) | (super ? 1 : 0);
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES - 1);
		if (write) {
			if (idx2 < MMUFASTCACHE_ENTRIES - 1) {
				atc_data_cache_write[idx2].log = idx1;
				atc_data_cache_write[idx2].phys = phys;
				atc_data_cache_write[idx2].cache_state = mmu_cache_state;
			}
		} else {
			if (idx2 < MMUFASTCACHE_ENTRIES - 1) {
				atc_data_cache_read[idx2].log = idx1;
				atc_data_cache_read[idx2].phys = phys;
				atc_data_cache_read[idx2].cache_state = mmu_cache_state;
			}
		}
#endif
	}
}

uaecptr mmu_translate(uaecptr addr, uae_u32 val, bool super, bool data, bool write, int size)
{
	int way, i, index, way_invalid;
	struct mmu_atc_line *l;
	uae_u32 status060 = 0;
	uae_u32 tag = ((super ? 0x80000000 : 0x00000000) | (addr >> 1)) & mmu_tagmask;

	if (mmu_pagesize_8k)
		index=(addr & 0x0001E000)>>13;
	else
		index=(addr & 0x0000F000)>>12;
	way_invalid = ATC_WAYS;
	way_random++;
	way = mmu_atc_ways[data];

	for (i = 0; i < ATC_WAYS; i++) {
		// if we have this
		l = &mmu_atc_array[data][index][way];
		if (l->valid) {
			if (tag == l->tag) {
atc_retry:
				// check if we need to cause a page fault
				if (((l->status&(MMU_MMUSR_W|MMU_MMUSR_S|MMU_MMUSR_R))!=MMU_MMUSR_R)) {
					if (((l->status&MMU_MMUSR_W) && write) ||
						((l->status&MMU_MMUSR_S) && !super) ||
						!(l->status&MMU_MMUSR_R)) {

						if ((l->status&MMU_MMUSR_S) && !super)
							status060 |= MMU_FSLW_SP;
						if ((l->status&MMU_MMUSR_W) && write)
							status060 |= MMU_FSLW_WP;

						mmu_bus_error(addr, val, mmu_get_fc(super, data), write, size, status060, false);
						return 0; // never reach, bus error longjumps out of the function
					}
				}
				// if first write to this page initiate table search to set M bit (but modify this slot)
				if (!(l->status&MMU_MMUSR_M) && write) {
					way_invalid = way;
					break;
				}

				// save way for next access (likely in same page)
				mmu_atc_ways[data] = way;

				if (l->status & MMU_MMUSR_CM_DISABLE) {
					mmu_cache_state = CACHE_DISABLE_MMU;
				} else {
					mmu_cache_state = CACHE_ENABLE_ALL;
					if (l->status & MMU_MMUSR_CM_MODE) {
						mmu_cache_state |= CACHE_ENABLE_COPYBACK;
					}
				}

				mmu_add_cache(addr, l->phys, super, data, write);

				// return translated addr
				return l->phys | (addr & mmu_pagemask);
			} else {
				way_invalid = way;
			}
		}
		way++;
		way %= ATC_WAYS;
	}
	// no entry found, we need to create a new one, first find an atc line to replace
	way_random %= ATC_WAYS;
	way = (way_invalid < ATC_WAYS) ? way_invalid : way_random;
	
	// then initiate table search and create a new entry
	l = &mmu_atc_array[data][index][way];
	mmu_fill_atc(addr, super, tag, write, l, &status060);

	if (status060 && currprefs.mmu_model == 68060) {
		mmu_bus_error(addr, val, mmu_get_fc(super, data), write, size, status060, false);
	}
	
	// and retry the ATC search
	way_random++;
	goto atc_retry;
}

static void misalignednotfirst(uaecptr addr)
{
#if MMUDEBUGMISC > 0
	write_log (_T("misalignednotfirst %08x -> %08x %08X\n"), regs.mmu_fault_addr, addr, regs.instruction_pc);
#endif
	regs.mmu_fault_addr = addr;
	regs.mmu_fslw |= MMU_FSLW_MA;
	regs.mmu_ssw |= MMU_SSW_MA;
}

static void misalignednotfirstcheck(uaecptr addr)
{
	if (regs.mmu_fault_addr == addr)
		return;
	misalignednotfirst (addr);
}

uae_u16 REGPARAM2 mmu_get_word_unaligned(uaecptr addr, bool data)
{
	uae_u16 res;

	res = (uae_u16)mmu_get_byte(addr, data, sz_word) << 8;
	SAVE_EXCEPTION;
	TRY(prb) {
		res |= mmu_get_byte(addr + 1, data, sz_word);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		misalignednotfirst(addr);
		THROW_AGAIN(prb);
	} ENDTRY
	return res;
}

uae_u32 REGPARAM2 mmu_get_long_unaligned(uaecptr addr, bool data)
{
	uae_u32 res;

	if (likely(!(addr & 1))) {
		res = (uae_u32)mmu_get_word(addr, data, sz_long) << 16;
		SAVE_EXCEPTION;
		TRY(prb) {
			res |= mmu_get_word(addr + 2, data, sz_long);
			RESTORE_EXCEPTION;
		}
		CATCH(prb) {
			RESTORE_EXCEPTION;
			misalignednotfirst(addr);
			THROW_AGAIN(prb);
		} ENDTRY
	} else {
		res = (uae_u32)mmu_get_byte(addr, data, sz_long) << 8;
		SAVE_EXCEPTION;
		TRY(prb) {
			res = (res | mmu_get_byte(addr + 1, data, sz_long)) << 8;
			res = (res | mmu_get_byte(addr + 2, data, sz_long)) << 8;
			res |= mmu_get_byte(addr + 3, data, sz_long);
			RESTORE_EXCEPTION;
		}
		CATCH(prb) {
			RESTORE_EXCEPTION;
			misalignednotfirst(addr);
			THROW_AGAIN(prb);
		} ENDTRY
	}
	return res;
}

uae_u32 REGPARAM2 mmu_get_ilong_unaligned(uaecptr addr)
{
	uae_u32 res;

	res = (uae_u32)mmu_get_iword(addr, sz_long) << 16;
	SAVE_EXCEPTION;
	TRY(prb) {
		res |= mmu_get_iword(addr + 2, sz_long);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		misalignednotfirst(addr);
		THROW_AGAIN(prb);
	} ENDTRY
	return res;
}

static uae_u16 REGPARAM2 mmu_get_lrmw_word_unaligned(uaecptr addr)
{
	uae_u16 res;

	res = (uae_u16)mmu_get_user_byte(addr, regs.s != 0, true, sz_word, true) << 8;
	SAVE_EXCEPTION;
	TRY(prb) {
		res |= mmu_get_user_byte(addr + 1, regs.s != 0, true, sz_word, true);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		misalignednotfirst(addr);
		THROW_AGAIN(prb);
	} ENDTRY
	return res;
}

static uae_u32 REGPARAM2 mmu_get_lrmw_long_unaligned(uaecptr addr)
{
	uae_u32 res;

	if (likely(!(addr & 1))) {
		res = (uae_u32)mmu_get_user_word(addr, regs.s != 0, true, sz_long, true) << 16;
		SAVE_EXCEPTION;
		TRY(prb) {
			res |= mmu_get_user_word(addr + 2, regs.s != 0, true, sz_long, true);
			RESTORE_EXCEPTION;
		}
		CATCH(prb) {
			RESTORE_EXCEPTION;
			misalignednotfirst(addr);
			THROW_AGAIN(prb);
		} ENDTRY
	} else {
		res = (uae_u32)mmu_get_user_byte(addr, regs.s != 0, true, sz_long, true) << 8;
		SAVE_EXCEPTION;
		TRY(prb) {
			res = (res | mmu_get_user_byte(addr + 1, regs.s != 0, true, sz_long, true)) << 8;
			res = (res | mmu_get_user_byte(addr + 2, regs.s != 0, true, sz_long, true)) << 8;
			res |= mmu_get_user_byte(addr + 3, regs.s != 0, true, sz_long, true);
			RESTORE_EXCEPTION;
		}
		CATCH(prb) {
			RESTORE_EXCEPTION;
			misalignednotfirst(addr);
			THROW_AGAIN(prb);
		} ENDTRY
	}
	return res;
}


static void REGPARAM2 mmu_put_lrmw_long_unaligned(uaecptr addr, uae_u32 val)
{
	SAVE_EXCEPTION;
	TRY(prb) {
		if (likely(!(addr & 1))) {
			mmu_put_user_word(addr, val >> 16, regs.s != 0, sz_long, true);
			mmu_put_user_word(addr + 2, val, regs.s != 0, sz_long, true);
		} else {
			mmu_put_user_byte(addr, val >> 24, regs.s != 0, sz_long, true);
			mmu_put_user_byte(addr + 1, val >> 16, regs.s != 0, sz_long, true);
			mmu_put_user_byte(addr + 2, val >> 8, regs.s != 0, sz_long, true);
			mmu_put_user_byte(addr + 3, val, regs.s != 0, sz_long, true);
		}
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		misalignednotfirstcheck(addr);
		THROW_AGAIN(prb);
	} ENDTRY
}

static void REGPARAM2 mmu_put_lrmw_word_unaligned(uaecptr addr, uae_u16 val)
{
	SAVE_EXCEPTION;
	TRY(prb) {
		mmu_put_user_byte(addr, val >> 8, regs.s != 0, sz_word, true);
		mmu_put_user_byte(addr + 1, (uae_u8)val, regs.s != 0, sz_word, true);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		misalignednotfirstcheck(addr);
		THROW_AGAIN(prb);
	} ENDTRY
}



void REGPARAM2 mmu_put_long_unaligned(uaecptr addr, uae_u32 val, bool data)
{
	SAVE_EXCEPTION;
	TRY(prb) {
		if (likely(!(addr & 1))) {
			mmu_put_word(addr, val >> 16, data, sz_long);
			mmu_put_word(addr + 2, val, data, sz_long);
		} else {
			mmu_put_byte(addr, val >> 24, data, sz_long);
			mmu_put_byte(addr + 1, val >> 16, data, sz_long);
			mmu_put_byte(addr + 2, val >> 8, data, sz_long);
			mmu_put_byte(addr + 3, val, data, sz_long);
		}
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		misalignednotfirstcheck(addr);
		THROW_AGAIN(prb);
	} ENDTRY
}

void REGPARAM2 mmu_put_word_unaligned(uaecptr addr, uae_u16 val, bool data)
{
	SAVE_EXCEPTION;
	TRY(prb) {
		mmu_put_byte(addr, val >> 8, data, sz_word);
		mmu_put_byte(addr + 1, (uae_u8)val, data, sz_word);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		misalignednotfirstcheck(addr);
		THROW_AGAIN(prb);
	} ENDTRY
}

uae_u32 REGPARAM2 sfc_get_long(uaecptr addr)
{
	bool super = (regs.sfc & 4) != 0;
	uae_u32 res;

	ismoves = true;
	if (likely(!is_unaligned_page(addr, 4))) {
		res = mmu_get_user_long(addr, super, false, sz_long, false);
	} else {
		if (likely(!(addr & 1))) {
			res = (uae_u32)mmu_get_user_word(addr, super, false, sz_long, false) << 16;
			SAVE_EXCEPTION;
			TRY(prb) {
				res |= mmu_get_user_word(addr + 2, super, false, sz_long, false);
				RESTORE_EXCEPTION;
			}
			CATCH(prb) {
				RESTORE_EXCEPTION;
				misalignednotfirst(addr);
				THROW_AGAIN(prb);
			} ENDTRY
		} else {
			res = (uae_u32)mmu_get_user_byte(addr, super, false, sz_long, false) << 8;
			SAVE_EXCEPTION;
			TRY(prb) {
				res = (res | mmu_get_user_byte(addr + 1, super, false, sz_long, false)) << 8;
				res = (res | mmu_get_user_byte(addr + 2, super, false, sz_long, false)) << 8;
				res |= mmu_get_user_byte(addr + 3, super, false, sz_long, false);
				RESTORE_EXCEPTION;
			}
			CATCH(prb) {
				RESTORE_EXCEPTION;
				misalignednotfirst(addr);
				THROW_AGAIN(prb);
			} ENDTRY
		}
	}

	ismoves = false;
	return res;
}

uae_u16 REGPARAM2 sfc_get_word(uaecptr addr)
{
	bool super = (regs.sfc & 4) != 0;
	uae_u16 res;

	ismoves = true;
	if (likely(!is_unaligned_page(addr, 2))) {
		res = mmu_get_user_word(addr, super, false, sz_word, false);
	} else {
		res = (uae_u16)mmu_get_user_byte(addr, super, false, sz_word, false) << 8;
		SAVE_EXCEPTION;
		TRY(prb) {
			res |= mmu_get_user_byte(addr + 1, super, false, sz_word, false);
			RESTORE_EXCEPTION;
		}
		CATCH(prb) {
			RESTORE_EXCEPTION;
			misalignednotfirst(addr);
			THROW_AGAIN(prb);
		} ENDTRY
	}
	ismoves = false;
	return res;
}

uae_u8 REGPARAM2 sfc_get_byte(uaecptr addr)
{
	bool super = (regs.sfc & 4) != 0;
	uae_u8 res;
	
	ismoves = true;
	res = mmu_get_user_byte(addr, super, false, sz_byte, false);
	ismoves = false;
	return res;
}

void REGPARAM2 dfc_put_long(uaecptr addr, uae_u32 val)
{
	bool super = (regs.dfc & 4) != 0;

	ismoves = true;
	SAVE_EXCEPTION;
	TRY(prb) {
		if (likely(!is_unaligned_page(addr, 4))) {
			mmu_put_user_long(addr, val, super, sz_long, false);
		} else if (likely(!(addr & 1))) {
			mmu_put_user_word(addr, val >> 16, super, sz_long, false);
			mmu_put_user_word(addr + 2, val, super, sz_long, false);
		} else {
			mmu_put_user_byte(addr, val >> 24, super, sz_long, false);
			mmu_put_user_byte(addr + 1, val >> 16, super, sz_long, false);
			mmu_put_user_byte(addr + 2, val >> 8, super, sz_long, false);
			mmu_put_user_byte(addr + 3, val, super, sz_long, false);
		}
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		misalignednotfirstcheck(addr);
		THROW_AGAIN(prb);
	} ENDTRY
	ismoves = false;
}

void REGPARAM2 dfc_put_word(uaecptr addr, uae_u16 val)
{
	bool super = (regs.dfc & 4) != 0;

	ismoves = true;
	SAVE_EXCEPTION;
	TRY(prb) {
		if (likely(!is_unaligned_page(addr, 2))) {
			mmu_put_user_word(addr, val, super, sz_word, false);
		} else {
			mmu_put_user_byte(addr, val >> 8, super, sz_word, false);
			mmu_put_user_byte(addr + 1, (uae_u8)val, super, sz_word, false);
		}
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		misalignednotfirstcheck(addr);
		THROW_AGAIN(prb);
	} ENDTRY
	ismoves = false;
}

void REGPARAM2 dfc_put_byte(uaecptr addr, uae_u8 val)
{
	bool super = (regs.dfc & 4) != 0;

	ismoves = true;
	SAVE_EXCEPTION;
	TRY(prb) {
		mmu_put_user_byte(addr, val, super, sz_byte, false);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		regs.wb3_data = val;
		THROW_AGAIN(prb);
	} ENDTRY
	ismoves = false;
}

void mmu_get_move16(uaecptr addr, uae_u32 *v, bool data, int size)
{
	bool super = regs.s != 0;

	addr &= ~15;
	if ((!mmu_ttr_enabled || mmu_match_ttr(addr,super,data) == TTR_NO_MATCH) && regs.mmu_enabled) {
		addr = mmu_translate(addr, 0, super, data, false, size);
	}
	// MOVE16 read and cache miss: do not allocate new cache line
	mmu_cache_state |= CACHE_DISABLE_ALLOCATE;
	for (int i = 0; i < 4; i++) {
		v[i] = x_phys_get_long(addr + i * 4);
	}
}

void mmu_put_move16(uaecptr addr, uae_u32 *val, bool data, int size)
{
	bool super = regs.s != 0;

	addr &= ~15;
	if ((!mmu_ttr_enabled || mmu_match_ttr_write(addr,super,data,val[0],size) == TTR_NO_MATCH) && regs.mmu_enabled) {
		addr = mmu_translate(addr, val[0], super, data, true, size);
	}
	// MOVE16 write invalidates existing line and also does not allocate new cache lines.
	mmu_cache_state = CACHE_DISABLE_MMU;
	for (int i = 0; i < 4; i++) {
		x_phys_put_long(addr + i * 4, val[i]);
	}
}


void REGPARAM2 mmu_op_real(uae_u32 opcode, uae_u16 extra)
{
	bool super = (regs.dfc & 4) != 0;
	DUNUSED(extra);
	if ((opcode & 0xFE0) == 0x0500) { // PFLUSH
		bool glob;
		int regno;
		//D(didflush = 0);
		uae_u32 addr;
		/* PFLUSH */
		regno = opcode & 7;
		glob = (opcode & 8) != 0;

		if (opcode & 16) {
#if MMUINSDEBUG > 1
			write_log(_T("pflusha(%u,%u) PC=%08x\n"), glob, regs.dfc, m68k_getpc ());
#endif
			mmu_flush_atc_all(glob);
		} else {
			addr = m68k_areg(regs, regno);
#if MMUINSDEBUG > 1
			write_log(_T("pflush(%u,%u,%x) PC=%08x\n"), glob, regs.dfc, addr, m68k_getpc ());
#endif
			mmu_flush_atc(addr, super, glob);
		}
		flush_internals();
#ifdef USE_JIT
		flush_icache(0);
#endif
	} else if ((opcode & 0x0FD8) == 0x0548) { // PTEST (68040)
		bool write;
		int regno;
		uae_u32 addr;
		uae_u32 status060 = 0;

		regno = opcode & 7;
		write = (opcode & 32) == 0;
		addr = m68k_areg(regs, regno);
#if MMUINSDEBUG > 0
		write_log(_T("PTEST%c (A%d) %08x DFC=%d\n"), write ? 'W' : 'R', regno, addr, regs.dfc);
#endif
		mmu_flush_atc(addr, super, true);
		bool data = (regs.dfc & 3) != 2;
		int ttr_match = mmu_match_ttr(addr,super,data);
		if (ttr_match != TTR_NO_MATCH) {
			if (ttr_match == TTR_NO_WRITE && write) {
				regs.mmusr = MMU_MMUSR_B;
			} else {
				regs.mmusr = MMU_MMUSR_T | MMU_MMUSR_R;
			}
		} else if (!currprefs.mmu_ec) {
			int way;
			uae_u32 index;
			uae_u32 tag = ((super ? 0x80000000 : 0x00000000) | (addr >> 1)) & mmu_tagmask;
			if (mmu_pagesize_8k)
				index=(addr & 0x0001E000)>>13;
			else
				index=(addr & 0x0000F000)>>12;
			for (way = 0; way < ATC_WAYS; way++) {
				if (!mmu_atc_array[data][index][way].valid)
					break;
			}
			if (way >= ATC_WAYS) {
				way = way_random % ATC_WAYS;
			}
			regs.mmusr = mmu_fill_atc(addr, super, tag, write, &mmu_atc_array[data][index][way], &status060);
#if MMUINSDEBUG > 0
			write_log(_T("PTEST result: mmusr %08x\n"), regs.mmusr);
#endif
		}
	} else if ((opcode & 0xFFB8) == 0xF588) { // PLPA (68060)
		int write = (opcode & 0x40) == 0;
		int regno = opcode & 7;
		uae_u32 addr = m68k_areg (regs, regno);
		bool data = (regs.dfc & 3) != 2;
		int ttr;

#if MMUINSDEBUG > 0
		write_log(_T("PLPA%c param: %08x\n"), write ? 'W' : 'R', addr);
#endif
		if (write)
			ttr = mmu_match_ttr_write(addr, super, data, 0, 1);
		else
			ttr = mmu_match_ttr(addr,super,data);
		if (ttr == TTR_NO_MATCH) {
			if (!currprefs.mmu_ec) {
				m68k_areg (regs, regno) = mmu_translate(addr, 0, super, data, write, 1);
			}
		}
#if MMUINSDEBUG > 0
		write_log(_T("PLPA%c result: %08x\n"), write ? 'W' : 'R', m68k_areg (regs, regno));
#endif
	} else {
		op_illg (opcode);
	}
}

// fixme : global parameter?
void REGPARAM2 mmu_flush_atc(uaecptr addr, bool super, bool global)
{
	int way,type,index;

	uaecptr tag = ((super ? 0x80000000 : 0) | (addr >> 1)) & mmu_tagmask;
	if (mmu_pagesize_8k)
		index=(addr & 0x0001E000)>>13;
	else
		index=(addr & 0x0000F000)>>12;
	for (type=0;type<ATC_TYPE;type++) {
		for (way=0;way<ATC_WAYS;way++) {
			struct mmu_atc_line *l = &mmu_atc_array[type][index][way];
			if (!global && (l->status & MMU_MMUSR_G))
				continue;
			// if we have this 
			if (tag == l->tag && l->valid) {
				l->valid=false;
			}
		}
	}	
	flush_shortcut_cache(addr, super);
	mmu_flush_cache();
}

void REGPARAM2 mmu_flush_atc_all(bool global)
{
	int way,slot,type;
	for (type=0;type<ATC_TYPE;type++) {
		for (slot=0;slot<ATC_SLOTS;slot++) {
			for (way=0;way<ATC_WAYS;way++) {
				struct mmu_atc_line *l = &mmu_atc_array[type][slot][way];
				if (!global && (l->status&MMU_MMUSR_G))
					continue;
				l->valid=false;
			}
		}
	}
	flush_shortcut_cache(0xffffffff, 0);
	mmu_flush_cache();
}

void REGPARAM2 mmu_set_funcs(void)
{
	if (currprefs.mmu_model != 68040 && currprefs.mmu_model != 68060)
		return;

	x_phys_get_iword = phys_get_word;
	x_phys_get_ilong = phys_get_long;
	x_phys_get_byte = phys_get_byte;
	x_phys_get_word = phys_get_word;
	x_phys_get_long = phys_get_long;
	x_phys_put_byte = phys_put_byte;
	x_phys_put_word = phys_put_word;
	x_phys_put_long = phys_put_long;
	if (currprefs.cpu_memory_cycle_exact || currprefs.cpu_compatible) {
		x_phys_get_iword = get_word_icache040;
		x_phys_get_ilong = get_long_icache040;
		if (currprefs.cpu_data_cache) {
			x_phys_get_byte = get_byte_cache_040;
			x_phys_get_word = get_word_cache_040;
			x_phys_get_long = get_long_cache_040;
			x_phys_put_byte = put_byte_cache_040;
			x_phys_put_word = put_word_cache_040;
			x_phys_put_long = put_long_cache_040;
		} else if (currprefs.cpu_memory_cycle_exact) {
			x_phys_get_byte = mem_access_delay_byte_read_c040;
			x_phys_get_word = mem_access_delay_word_read_c040;
			x_phys_get_long = mem_access_delay_long_read_c040;
			x_phys_put_byte = mem_access_delay_byte_write_c040;
			x_phys_put_word = mem_access_delay_word_write_c040;
			x_phys_put_long = mem_access_delay_long_write_c040;
		}
	}
}

void REGPARAM2 mmu_reset(void)
{
	mmu_flush_atc_all(true);
	mmu_set_funcs();
}

uae_u16 REGPARAM2 mmu_set_tc(uae_u16 tc)
{
	if (currprefs.mmu_ec) {
		tc &= ~(0x8000 | 0x4000);
		// at least 68EC040 always returns zero when TC is read.
		if (currprefs.cpu_model == 68040)
			tc = 0;
	}

	regs.mmu_enabled = (tc & 0x8000) != 0;
	mmu_pagesize_8k = (tc & 0x4000) != 0;

	mmu_tagmask  = mmu_pagesize_8k ? 0xFFFF0000 : 0xFFFF8000;
	mmu_pagemask = mmu_pagesize_8k ? 0x00001FFF : 0x00000FFF;
	mmu_pagemaski = ~mmu_pagemask;
	regs.mmu_page_size = mmu_pagesize_8k ? 8192 : 4096;
	mmu_pageshift = mmu_pagesize_8k ? 13 : 12;
	mmu_pageshift1m = mmu_pageshift - 1;

	cache_default_ins = CACHE_ENABLE_ALL;
	cache_default_data = CACHE_ENABLE_ALL;
	if (currprefs.mmu_model == 68060) {
		int dc = (tc >> 3) & 3;
		cache_default_ins = 0;
		if (!(dc & 2))
			cache_default_ins = CACHE_ENABLE_ALL;
		dc = (tc >> 8) & 3;
		cache_default_data = 0;
		if (!(dc & 2))
			cache_default_data = (dc & 1) ? CACHE_ENABLE_COPYBACK | CACHE_ENABLE_ALL : CACHE_ENABLE_ALL;
	}

	mmu_flush_atc_all(true);

	write_log(_T("%d MMU: TC=%04x enabled=%d page8k=%d PC=%08x\n"), currprefs.mmu_model, tc, regs.mmu_enabled, mmu_pagesize_8k, m68k_getpc());
	return tc;
}

void REGPARAM2 mmu_set_super(bool super)
{
	mmu_is_super = super ? 0x80000000 : 0;
}

void REGPARAM2 mmu_flush_cache(void)
{
	if (!currprefs.mmu_model)
		return;
#if MMU_ICACHE
	int len = sizeof(mmu_icache_data);
	memset(&mmu_icache_data, 0xff, sizeof(mmu_icache_data));
#endif
}

void m68k_do_rte_mmu040 (uaecptr a7)
{
	uae_u16 ssr = get_word_mmu040 (a7 + 8 + 4);
	if (ssr & MMU_SSW_CT) {
		uaecptr src_a7 = a7 + 8 - 8;
		uaecptr dst_a7 = a7 + 8 + 52;
		put_word_mmu040 (dst_a7 + 0, get_word_mmu040 (src_a7 + 0));
		put_long_mmu040 (dst_a7 + 2, get_long_mmu040 (src_a7 + 2));
		// skip this word
		put_long_mmu040 (dst_a7 + 8, get_long_mmu040 (src_a7 + 8));
	}
	if (ssr & MMU_SSW_CM) {
		mmu040_movem = 1;
		mmu040_movem_ea = get_long_mmu040 (a7 + 8);
#if MMUDEBUGMISC > 0
		write_log (_T("MMU restarted MOVEM EA=%08X\n"), mmu040_movem_ea);
#endif
	}
}

void m68k_do_rte_mmu060 (uaecptr a7)
{
#if 0
	mmu060_state = 2;
#endif
}

void flush_mmu040 (uaecptr addr, int n)
{
	mmu_flush_cache();
}

void m68k_do_rts_mmu040 (void)
{
	uaecptr stack = m68k_areg (regs, 7);
	uaecptr newpc = get_long_mmu040 (stack);
	m68k_areg (regs, 7) += 4;
	m68k_setpc (newpc);
}
void m68k_do_bsr_mmu040 (uaecptr oldpc, uae_s32 offset)
{
	uaecptr newstack = m68k_areg (regs, 7) - 4;
	put_long_mmu040 (newstack, oldpc);
	m68k_areg (regs, 7) -= 4;
	m68k_incpci (offset);
}

void flush_mmu060 (uaecptr addr, int n)
{
	mmu_flush_cache();
}

void m68k_do_rts_mmu060 (void)
{
	uaecptr stack = m68k_areg (regs, 7);
	uaecptr newpc = get_long_mmu060 (stack);
	m68k_areg (regs, 7) += 4;
	m68k_setpc (newpc);
}
void m68k_do_bsr_mmu060 (uaecptr oldpc, uae_s32 offset)
{
	uaecptr newstack = m68k_areg (regs, 7) - 4;
	put_long_mmu060 (newstack, oldpc);
	m68k_areg (regs, 7) -= 4;
	m68k_incpci (offset);
}

void uae_mmu_put_lrmw (uaecptr addr, uae_u32 v, int size, int type)
{
	locked_rmw_cycle = true;
	if (size == sz_byte) {
		mmu_put_user_byte(addr, v, regs.s, sz_byte, true);
	} else if (size == sz_word) {
		if (unlikely(is_unaligned_page(addr, 2))) {
			mmu_put_lrmw_word_unaligned(addr, v);
		} else {
			mmu_put_user_word(addr, v, regs.s != 0, sz_word, true);
		}
	} else {
		if (unlikely(is_unaligned_page(addr, 4)))
			mmu_put_lrmw_long_unaligned(addr, v);
		else
			mmu_put_user_long(addr, v, regs.s, sz_long, true);
	}
	locked_rmw_cycle = false;
}
uae_u32 uae_mmu_get_lrmw (uaecptr addr, int size, int type)
{
	uae_u32 v;
	locked_rmw_cycle = true;
	if (size == sz_byte) {
		v = mmu_get_user_byte(addr, regs.s != 0, true, sz_byte, true);
	} else if (size == sz_word) {
		if (unlikely(is_unaligned_page(addr, 2))) {
			v = mmu_get_lrmw_word_unaligned(addr);
		} else {
			v = mmu_get_user_word(addr, regs.s != 0, true, sz_word, true);
		}
	} else {
		if (unlikely(is_unaligned_page(addr, 4)))
			v = mmu_get_lrmw_long_unaligned(addr);
		else
			v = mmu_get_user_long(addr, regs.s != 0, true, sz_long, true);
	}
	locked_rmw_cycle = false;
	return v;
}

uae_u32 REGPARAM2 mmu060_get_rmw_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width)
{
	uae_u32 tmp1, tmp2, res, mask;

	offset &= 7;
	mask = 0xffffffffu << (32 - width);
	switch ((offset + width + 7) >> 3) {
	case 1:
		tmp1 = get_rmw_byte_mmu060 (src);
		res = tmp1 << (24 + offset);
		bdata[0] = tmp1 & ~(mask >> (24 + offset));
		break;
	case 2:
		tmp1 = get_rmw_word_mmu060 (src);
		res = tmp1 << (16 + offset);
		bdata[0] = tmp1 & ~(mask >> (16 + offset));
		break;
	case 3:
		tmp1 = get_rmw_word_mmu060 (src);
		tmp2 = get_rmw_byte_mmu060 (src + 2);
		res = tmp1 << (16 + offset);
		bdata[0] = tmp1 & ~(mask >> (16 + offset));
		res |= tmp2 << (8 + offset);
		bdata[1] = tmp2 & ~(mask >> (8 + offset));
		break;
	case 4:
		tmp1 = get_rmw_long_mmu060 (src);
		res = tmp1 << offset;
		bdata[0] = tmp1 & ~(mask >> offset);
		break;
	case 5:
		tmp1 = get_rmw_long_mmu060 (src);
		tmp2 = get_rmw_byte_mmu060 (src + 4);
		res = tmp1 << offset;
		bdata[0] = tmp1 & ~(mask >> offset);
		res |= tmp2 >> (8 - offset);
		bdata[1] = tmp2 & ~(mask << (8 - offset));
		break;
	default:
		/* Panic? */
		write_log (_T("x_get_bitfield() can't happen %d\n"), (offset + width + 7) >> 3);
		res = 0;
		break;
	}
	return res;
}

void REGPARAM2 mmu060_put_rmw_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width)
{
	offset = (offset & 7) + width;
	switch ((offset + 7) >> 3) {
	case 1:
		put_rmw_byte_mmu060 (dst, bdata[0] | (val << (8 - offset)));
		break;
	case 2:
		put_rmw_word_mmu060 (dst, bdata[0] | (val << (16 - offset)));
		break;
	case 3:
		put_rmw_word_mmu060 (dst, bdata[0] | (val >> (offset - 16)));
		put_rmw_byte_mmu060 (dst + 2, bdata[1] | (val << (24 - offset)));
		break;
	case 4:
		put_rmw_long_mmu060 (dst, bdata[0] | (val << (32 - offset)));
		break;
	case 5:
		put_rmw_long_mmu060 (dst, bdata[0] | (val >> (offset - 32)));
		put_rmw_byte_mmu060 (dst + 4, bdata[1] | (val << (40 - offset)));
		break;
	default:
		write_log (_T("x_put_bitfield() can't happen %d\n"), (offset + 7) >> 3);
		break;
	}
}


#ifndef __cplusplus
jmp_buf __exbuf;
int     __exvalue;
#define MAX_TRY_STACK 256
static int s_try_stack_size=0;
static jmp_buf s_try_stack[MAX_TRY_STACK];
jmp_buf* __poptry(void) {
	if (s_try_stack_size>0) {
		s_try_stack_size--;
		if (s_try_stack_size == 0)
			return NULL;
		memcpy(&__exbuf,&s_try_stack[s_try_stack_size-1],sizeof(jmp_buf));
		// fprintf(stderr,"pop jmpbuf=%08x\n",s_try_stack[s_try_stack_size][0]);
		return &s_try_stack[s_try_stack_size-1];
	}
	else {
		fprintf(stderr,"try stack underflow...\n");
		// return (NULL);
		abort();
	}
}
void __pushtry(jmp_buf* j) {
	if (s_try_stack_size<MAX_TRY_STACK) {
		// fprintf(stderr,"push jmpbuf=%08x\n",(*j)[0]);
		memcpy(&s_try_stack[s_try_stack_size],j,sizeof(jmp_buf));
		s_try_stack_size++;
	} else {
		fprintf(stderr,"try stack overflow...\n");
		abort();
	}
}
int __is_catched(void) {return (s_try_stack_size>0); }
#endif

#else

void mmu_op(uae_u32 opcode, uae_u16 /*extra*/)
{
	if ((opcode & 0xFE0) == 0x0500) {
		/* PFLUSH instruction */
		flush_internals();
	} else if ((opcode & 0x0FD8) == 0x548) {
		/* PTEST instruction */
	} else
		op_illg(opcode);
}

#endif


/*
vim:ts=4:sw=4:
*/
