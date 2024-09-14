/*
 *	PearPC
 *	ppc_mmu.cc
 *
 *	Copyright (C) 2003, 2004 Sebastian Biallas (sb@biallas.net)
 *	Portions Copyright (C) 2004 Daniel Foesch (dfoesch@cs.nmsu.edu)
 *	Portions Copyright (C) 2004 Apple Computer, Inc.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License version 2 as
 *	published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*	Pages marked: v.???
 *	From: IBM PowerPC MicroProcessor Family: Altivec(tm) Technology...
 *		Programming Environments Manual
 */

#include <cstdlib>
#include <cstring>
#include "system/arch/sysendian.h"
//#include "tools/snprintf.h"
#include "tracers.h"
//#include "io/prom/prom.h"
#include "io/io.h"
#include "ppc_cpu.h"
#include "ppc_fpu.h"
#include "ppc_vec.h"
#include "ppc_mmu.h"
#include "ppc_exc.h"
#include "ppc_tools.h"

#if 0
byte *gMemory = NULL;
uint32 gMemorySize;
#endif

#undef TLB

static int ppc_pte_protection[] = {
	// read(0)/write(1) key pp
	
	// read
	1, // r/w
	1, // r/w
	1, // r/w
	1, // r
	0, // -
	1, // r
	1, // r/w
	1, // r
	
	// write
	1, // r/w
	1, // r/w
	1, // r/w
	0, // r
	0, // -
	0, // r
	1, // r/w
	0, // r
};

inline int FASTCALL ppc_effective_to_physical(uint32 addr, int flags, uint32 &result)
{
	static int lastibatcnt;
	static int lastdbatcnt;

	if (flags & PPC_MMU_CODE) {
		if (!(gCPU.msr & MSR_IR)) {
			result = addr;
			return PPC_MMU_OK;
		}
		/*
		 * BAT translation .329
		 */

		uint32 batu = (gCPU.msr & MSR_PR ? BATU_Vp : BATU_Vs);		

		for (int ii=0; ii<4; ii++) {
			int i = lastibatcnt;
			uint32 bl17 = gCPU.ibat_bl17[i] | 0xf001ffff;
			uint32 addr2 = addr & bl17;
			uint32 addr3 = gCPU.ibatu[i] & bl17;
			if (BATU_BEPI(addr2) == BATU_BEPI(addr3)) {
				// bat applies to this address
				if (gCPU.ibatu[i] & batu) {
					// bat entry valid
					uint32 offset = BAT_EA_OFFSET(addr);
					uint32 page = BAT_EA_11(addr);
					page &= ~gCPU.ibat_bl17[i];
					page |= BATL_BRPN(gCPU.ibatl[i] & bl17);
					// fixme: check access rights
					result = page | offset;
					return PPC_MMU_OK;
				}
			}
			lastibatcnt++;
			lastibatcnt &= 3;
		}
	} else {
		if (!(gCPU.msr & MSR_DR)) {
			result = addr;
			return PPC_MMU_OK;
		}
		/*
		 * BAT translation .329
		 */

		uint32 batu = (gCPU.msr & MSR_PR ? BATU_Vp : BATU_Vs);

		for (int ii=0; ii<4; ii++) {
			int i = lastdbatcnt;
			uint32 bl17 = gCPU.dbat_bl17[i] | 0xf001ffff;
			uint32 addr2 = addr & bl17;
			uint32 addr3 = gCPU.dbatu[i] & bl17;
			if (BATU_BEPI(addr2) == BATU_BEPI(addr3)) {
				// bat applies to this address
				if (gCPU.dbatu[i] & batu) {
					// bat entry valid
					uint32 offset = BAT_EA_OFFSET(addr);
					uint32 page = BAT_EA_11(addr);
					page &= ~gCPU.dbat_bl17[i];
					page |= BATL_BRPN(gCPU.dbatl[i] & bl17);
					// fixme: check access rights
					result = page | offset;
					return PPC_MMU_OK;
				}
			}
			lastdbatcnt++;
			lastdbatcnt &= 3;
		}
	}
	
	/*
	 * Address translation with segment register
	 */
	uint32 sr = gCPU.sr[EA_SR(addr)];

	if (sr & SR_T) {
		// woea
		// FIXME: implement me
		PPC_MMU_ERR("sr & T\n");
	} else {
#ifdef TLB	
		for (int i=0; i<4; i++) {
			if ((addr & ~0xfff) == (gCPU.tlb_va[i])) {
				gCPU.tlb_last = i;
//				ht_printf("TLB: %d: %08x -> %08x\n", i, addr, gCPU.tlb_pa[i] | (addr & 0xfff));
				result = gCPU.tlb_pa[i] | (addr & 0xfff);
				return PPC_MMU_OK;
			}
		}
#endif
		// page address translation
		if ((flags & PPC_MMU_CODE) && (sr & SR_N)) {
			// segment isnt executable
			if (!(flags & PPC_MMU_NO_EXC)) {
				ppc_exception(PPC_EXC_ISI, PPC_EXC_SRR1_GUARD);
				return PPC_MMU_EXC;
			}
			return PPC_MMU_FATAL;
		}
		uint32 offset = EA_Offset(addr);         // 12 bit
		uint32 page_index = EA_PageIndex(addr);  // 16 bit
		uint32 VSID = SR_VSID(sr);               // 24 bit
		uint32 api = EA_API(addr);               //  6 bit (part of page_index)
		// VSID.page_index = Virtual Page Number (VPN)

		// Hashfunction no 1 "xor" .360
		uint32 hash1 = (VSID ^ page_index);
		uint32 pteg_addr = ((hash1 & gCPU.pagetable_hashmask)<<6) | gCPU.pagetable_base;
		int key;
		if (gCPU.msr & MSR_PR) {
			key = (sr & SR_Kp) ? 4 : 0;
		} else {
			key = (sr & SR_Ks) ? 4 : 0;
		}

		uint32 pte_protection_offset = ((flags&PPC_MMU_WRITE) ? 8:0) + key;

		for (int i=0; i<8; i++) {
			uint32 pte;
			if (ppc_read_physical_word(pteg_addr, pte)) {
				if (!(flags & PPC_MMU_NO_EXC)) {
					PPC_MMU_ERR("read physical in address translate failed\n");
					return PPC_MMU_EXC;
				}
				return PPC_MMU_FATAL;
			}
			if ((pte & PTE1_V) && (!(pte & PTE1_H))) {
				if (VSID == PTE1_VSID(pte) && (api == PTE1_API(pte))) {
					// page found
					if (ppc_read_physical_word(pteg_addr+4, pte)) {
						if (!(flags & PPC_MMU_NO_EXC)) {
							PPC_MMU_ERR("read physical in address translate failed\n");
							return PPC_MMU_EXC;
						}
						return PPC_MMU_FATAL;
					}
					// check accessmode .346
					if (!ppc_pte_protection[pte_protection_offset + PTE2_PP(pte)]) {
						if (!(flags & PPC_MMU_NO_EXC)) {
							if (flags & PPC_MMU_CODE) {
								PPC_MMU_WARN("correct impl? code + read protection\n");
								ppc_exception(PPC_EXC_ISI, PPC_EXC_SRR1_PROT, addr);
								return PPC_MMU_EXC;
							} else {
								if (flags & PPC_MMU_WRITE) {
									ppc_exception(PPC_EXC_DSI, PPC_EXC_DSISR_PROT | PPC_EXC_DSISR_STORE, addr);
								} else {
									ppc_exception(PPC_EXC_DSI, PPC_EXC_DSISR_PROT, addr);
								}
								return PPC_MMU_EXC;
							}
						}
						return PPC_MMU_FATAL;
					}
					// ok..
					uint32 pap = PTE2_RPN(pte);
					result = pap | offset;
#ifdef TLB
					gCPU.tlb_last++;
					gCPU.tlb_last &= 3;
					gCPU.tlb_pa[gCPU.tlb_last] = pap;
					gCPU.tlb_va[gCPU.tlb_last] = addr & ~0xfff;					
//					ht_printf("TLB: STORE %d: %08x -> %08x\n", gCPU.tlb_last, addr, pap);
#endif
					// update access bits
					uint32 opte = pte;
					if (flags & PPC_MMU_WRITE) {
						pte |= PTE2_C | PTE2_R;
					} else {
						pte |= PTE2_R;
					}
					if (pte != opte)
						ppc_write_physical_word(pteg_addr+4, pte);
					return PPC_MMU_OK;
				}
			}
			pteg_addr+=8;
		}
		
		// Hashfunction no 2 "not" .360
		hash1 = ~hash1;
		pteg_addr = ((hash1 & gCPU.pagetable_hashmask)<<6) | gCPU.pagetable_base;
		for (int i=0; i<8; i++) {
			uint32 pte;
			if (ppc_read_physical_word(pteg_addr, pte)) {
				if (!(flags & PPC_MMU_NO_EXC)) {
					PPC_MMU_ERR("read physical in address translate failed\n");
					return PPC_MMU_EXC;
				}
				return PPC_MMU_FATAL;
			}
			if ((pte & PTE1_V) && (pte & PTE1_H)) {
				if (VSID == PTE1_VSID(pte) && (api == PTE1_API(pte))) {
					// page found
					if (ppc_read_physical_word(pteg_addr+4, pte)) {
						if (!(flags & PPC_MMU_NO_EXC)) {
							PPC_MMU_ERR("read physical in address translate failed\n");
							return PPC_MMU_EXC;
						}
						return PPC_MMU_FATAL;
					}
					// check accessmode
					int key;
					if (gCPU.msr & MSR_PR) {
						key = (sr & SR_Kp) ? 4 : 0;
					} else {
						key = (sr & SR_Ks) ? 4 : 0;
					}
					if (!ppc_pte_protection[((flags&PPC_MMU_WRITE)?8:0) + key + PTE2_PP(pte)]) {
						if (!(flags & PPC_MMU_NO_EXC)) {
							if (flags & PPC_MMU_CODE) {
								PPC_MMU_WARN("correct impl? code + read protection\n");
								ppc_exception(PPC_EXC_ISI, PPC_EXC_SRR1_PROT, addr);
								return PPC_MMU_EXC;
							} else {
								if (flags & PPC_MMU_WRITE) {
									ppc_exception(PPC_EXC_DSI, PPC_EXC_DSISR_PROT | PPC_EXC_DSISR_STORE, addr);
								} else {
									ppc_exception(PPC_EXC_DSI, PPC_EXC_DSISR_PROT, addr);
								}
								return PPC_MMU_EXC;
							}
						}
						return PPC_MMU_FATAL;
					}
					// ok..
					result = PTE2_RPN(pte) | offset;
					
					// update access bits
					uint32 opte = pte;
					if (flags & PPC_MMU_WRITE) {
						pte |= PTE2_C | PTE2_R;
					} else {
						pte |= PTE2_R;
					}
					if (pte != opte)
						ppc_write_physical_word(pteg_addr+4, pte);
//					PPC_MMU_WARN("hash function 2 used!\n");
//					gSinglestep = true;
					return PPC_MMU_OK;
				}
			}
			pteg_addr+=8;
		}
	}
	// page fault
	if (!(flags & PPC_MMU_NO_EXC)) {
		if (flags & PPC_MMU_CODE) {
			ppc_exception(PPC_EXC_ISI, PPC_EXC_SRR1_PAGE);
		} else {
			if (flags & PPC_MMU_WRITE) {
				ppc_exception(PPC_EXC_DSI, PPC_EXC_DSISR_PAGE | PPC_EXC_DSISR_STORE, addr);
			} else {
				ppc_exception(PPC_EXC_DSI, PPC_EXC_DSISR_PAGE, addr);
			}
		}
		return PPC_MMU_EXC;
	}
	return PPC_MMU_FATAL;
}

void ppc_mmu_tlb_invalidate()
{
	gCPU.effective_code_page = 0xffffffff;
}

/*
pagetable:
min. 2^10 (64k) PTEGs
PTEG = 64byte
The page table can be any size 2^n where 16 <= n <= 25.

A PTEG contains eight
PTEs of eight bytes each; therefore, each PTEG is 64 bytes long.
*/

bool FASTCALL ppc_mmu_set_sdr1(uint32 newval, bool quiesce)
{
	/* if (newval == gCPU.sdr1)*/ quiesce = false;
	PPC_MMU_TRACE("new pagetable: sdr1 = 0x%08x\n", newval);
	uint32 htabmask = SDR1_HTABMASK(newval);
	uint32 x = 1;
	uint32 xx = 0;
	int n = 0;
	while ((htabmask & x) && (n < 9)) {
		n++;
		xx|=x;
		x<<=1;
	}
	if (htabmask & ~xx) {
		PPC_MMU_TRACE("new pagetable: broken htabmask (%05x)\n", htabmask);
		return false;
	}
	uint32 htaborg = SDR1_HTABORG(newval);
	if (htaborg & xx) {
		PPC_MMU_TRACE("new pagetable: broken htaborg (%05x)\n", htaborg);
		return false;
	}
	gCPU.pagetable_base = htaborg<<16;
	gCPU.sdr1 = newval;
	gCPU.pagetable_hashmask = ((xx<<10)|0x3ff);
	PPC_MMU_TRACE("new pagetable: sdr1 accepted\n");
	PPC_MMU_TRACE("number of pages: 2^%d pagetable_start: 0x%08x size: 2^%d\n", n+13, gCPU.pagetable_base, n+16);
#if 0
	if (quiesce) {
		prom_quiesce();
	}
#endif
	return true;
}

bool FASTCALL ppc_mmu_page_create(uint32 ea, uint32 pa)
{
	uint32 sr = gCPU.sr[EA_SR(ea)];
	uint32 page_index = EA_PageIndex(ea);  // 16 bit
	uint32 VSID = SR_VSID(sr);             // 24 bit
	uint32 api = EA_API(ea);               //  6 bit (part of page_index)
	uint32 hash1 = (VSID ^ page_index);
	uint32 pte, pte2;
	uint32 h = 0;
	for (int j=0; j<2; j++) {
		uint32 pteg_addr = ((hash1 & gCPU.pagetable_hashmask)<<6) | gCPU.pagetable_base;
		for (int i=0; i<8; i++) {
			if (ppc_read_physical_word(pteg_addr, pte)) {
				PPC_MMU_ERR("read physical in address translate failed\n");
				return false;
			}
			if (!(pte & PTE1_V)) {
				// free pagetable entry found
				pte = PTE1_V | (VSID << 7) | h | api;
				pte2 = (PA_RPN(pa) << 12) | 0;
				if (ppc_write_physical_word(pteg_addr, pte)
				 || ppc_write_physical_word(pteg_addr+4, pte2)) {
					return false;
				} else {
					// ok
					return true;
				}
			}
			pteg_addr+=8;
		}
		hash1 = ~hash1;
		h = PTE1_H;
	}
	return false;
}

inline bool FASTCALL ppc_mmu_page_free(uint32 ea)
{
	return true;
}

extern bool uae_ppc_direct_physical_memory_handle(uint32, byte *&ptr);

inline int FASTCALL ppc_direct_physical_memory_handle(uint32 addr, byte *&ptr)
{
	if (uae_ppc_direct_physical_memory_handle(addr, ptr))
		return PPC_MMU_OK;
	return PPC_MMU_FATAL;
}

int FASTCALL ppc_direct_effective_memory_handle(uint32 addr, byte *&ptr)
{
	uint32 ea;
	int r;
	if (!((r = ppc_effective_to_physical(addr, PPC_MMU_READ, ea)))) {
		return ppc_direct_physical_memory_handle(ea, ptr);
	}
	return r;
}

int FASTCALL ppc_direct_effective_memory_handle_code(uint32 addr, byte *&ptr)
{
	uint32 ea;
	int r;
	if (!((r = ppc_effective_to_physical(addr, PPC_MMU_READ | PPC_MMU_CODE, ea)))) {
		return ppc_direct_physical_memory_handle(ea, ptr);
	}
	return r;
}

inline int FASTCALL ppc_read_physical_qword(uint32 addr, Vector_t &result)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		VECT_D(result,0) = ppc_dword_from_BE(*((uint64*)(gMemory+addr)));
		VECT_D(result,1) = ppc_dword_from_BE(*((uint64*)(gMemory+addr+8)));
		return PPC_MMU_OK;
	}
#endif
	return io_mem_read128(addr, (uint128 *)&result);
}

inline int FASTCALL ppc_read_physical_dword(uint32 addr, uint64 &result)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		result = ppc_dword_from_BE(*((uint64*)(gMemory+addr)));
		return PPC_MMU_OK;
	}
#endif
	int ret = io_mem_read64(addr, result);
	//result = ppc_bswap_dword(result);
	return ret;
}

inline int FASTCALL ppc_read_physical_word(uint32 addr, uint32 &result)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		result = ppc_word_from_BE(*((uint32*)(gMemory+addr)));
		return PPC_MMU_OK;
	}
#endif
	int ret = io_mem_read(addr, result, 4);
	//result = ppc_bswap_word(result);
	return ret;
}

inline int FASTCALL ppc_read_physical_half(uint32 addr, uint16 &result)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		result = ppc_half_from_BE(*((uint16*)(gMemory+addr)));
		return PPC_MMU_OK;
	}
#endif
	uint32 r;
	int ret = io_mem_read(addr, r, 2);
	//result = ppc_bswap_half(r);
	result = r;
	return ret;
}

inline int FASTCALL ppc_read_physical_byte(uint32 addr, uint8 &result)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		result = gMemory[addr];
		return PPC_MMU_OK;
	}
#endif
	uint32 r;
	int ret = io_mem_read(addr, r, 1);
	result = r;
	return ret;
}

inline int FASTCALL ppc_read_effective_code(uint32 addr, uint32 &result)
{
	if (addr & 3) {
		// EXC..bla
		return PPC_MMU_FATAL;
	}
	uint32 p;
	int r;
	if (!((r=ppc_effective_to_physical(addr, PPC_MMU_READ | PPC_MMU_CODE, p)))) {
		return ppc_read_physical_word(p, result);
	}
	return r;
}

inline int FASTCALL ppc_read_effective_qword(uint32 addr, Vector_t &result)
{
	uint32 p;
	int r;

	addr &= ~0x0f;

	if (!(r = ppc_effective_to_physical(addr, PPC_MMU_READ, p))) {
		return ppc_read_physical_qword(p, result);
	}

	return r;
}

inline int FASTCALL ppc_read_effective_dword(uint32 addr, uint64 &result)
{
	uint32 p;
	int r;
	if (!(r = ppc_effective_to_physical(addr, PPC_MMU_READ, p))) {
		if (EA_Offset(addr) > 4088) {
			// read overlaps two pages.. tricky
			byte *r1, *r2;
			byte b[14];
			ppc_effective_to_physical((addr & ~0xfff)+4089, PPC_MMU_READ, p);
			if ((r = ppc_direct_physical_memory_handle(p, r1))) return r;
			if ((r = ppc_effective_to_physical((addr & ~0xfff)+4096, PPC_MMU_READ, p))) return r;
			if ((r = ppc_direct_physical_memory_handle(p, r2))) return r;
			memmove(&b[0], r1, 7);
			memmove(&b[7], r2, 7);
			memmove(&result, &b[EA_Offset(addr)-4089], 8);
			result = ppc_dword_from_BE(result);
			return PPC_MMU_OK;
		} else {
			return ppc_read_physical_dword(p, result);
		}
	}
	return r;
}

inline int FASTCALL ppc_read_effective_word(uint32 addr, uint32 &result)
{
	uint32 p;
	int r;
	if (!(r = ppc_effective_to_physical(addr, PPC_MMU_READ, p))) {
		if (EA_Offset(addr) > 4092) {
			// read overlaps two pages.. tricky
			byte *r1, *r2;
			byte b[6];
			ppc_effective_to_physical((addr & ~0xfff)+4093, PPC_MMU_READ, p);
			if ((r = ppc_direct_physical_memory_handle(p, r1))) return r;
			if ((r = ppc_effective_to_physical((addr & ~0xfff)+4096, PPC_MMU_READ, p))) return r;
			if ((r = ppc_direct_physical_memory_handle(p, r2))) return r;
			memmove(&b[0], r1, 3);
			memmove(&b[3], r2, 3);
			memmove(&result, &b[EA_Offset(addr)-4093], 4);
			result = ppc_word_from_BE(result);
			return PPC_MMU_OK;
		} else {
			return ppc_read_physical_word(p, result);
		}
	}
	return r;
}

inline int FASTCALL ppc_read_effective_half(uint32 addr, uint16 &result)
{
	uint32 p;
	int r;
	if (!((r = ppc_effective_to_physical(addr, PPC_MMU_READ, p)))) {
		if (EA_Offset(addr) > 4094) {
			// read overlaps two pages.. tricky
			byte b1, b2;
			ppc_effective_to_physical((addr & ~0xfff)+4095, PPC_MMU_READ, p);
			if ((r = ppc_read_physical_byte(p, b1))) return r;
			if ((r = ppc_effective_to_physical((addr & ~0xfff)+4096, PPC_MMU_READ, p))) return r;
			if ((r = ppc_read_physical_byte(p, b2))) return r;
			result = (b1<<8)|b2;
			return PPC_MMU_OK;
		} else {
			return ppc_read_physical_half(p, result);
		}
	}
	return r;
}

inline int FASTCALL ppc_read_effective_byte(uint32 addr, uint8 &result)
{
	uint32 p;
	int r;
	if (!((r = ppc_effective_to_physical(addr, PPC_MMU_READ, p)))) {
		return ppc_read_physical_byte(p, result);
	}
	return r;
}

inline int FASTCALL ppc_write_physical_qword(uint32 addr, Vector_t data)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		*((uint64*)(gMemory+addr)) = ppc_dword_to_BE(VECT_D(data,0));
		*((uint64*)(gMemory+addr+8)) = ppc_dword_to_BE(VECT_D(data,1));
		return PPC_MMU_OK;
	}
#endif
	if (io_mem_write128(addr, (uint128 *)&data) == IO_MEM_ACCESS_OK) {
		return PPC_MMU_OK;
	} else {
		return PPC_MMU_FATAL;
	}
}

inline int FASTCALL ppc_write_physical_dword(uint32 addr, uint64 data)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		*((uint64*)(gMemory+addr)) = ppc_dword_to_BE(data);
		return PPC_MMU_OK;
	}
#endif
//	if (io_mem_write64(addr, ppc_bswap_dword(data)) == IO_MEM_ACCESS_OK) {
	if (io_mem_write64(addr, data) == IO_MEM_ACCESS_OK) {
			return PPC_MMU_OK;
	} else {
		return PPC_MMU_FATAL;
	}
}

inline int FASTCALL ppc_write_physical_word(uint32 addr, uint32 data)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		*((uint32*)(gMemory+addr)) = ppc_word_to_BE(data);
		return PPC_MMU_OK;
	}
#endif
	//return io_mem_write(addr, ppc_bswap_word(data), 4);
	return io_mem_write(addr, data, 4);
}

inline int FASTCALL ppc_write_physical_half(uint32 addr, uint16 data)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		*((uint16*)(gMemory+addr)) = ppc_half_to_BE(data);
		return PPC_MMU_OK;
	}
#endif
	//return io_mem_write(addr, ppc_bswap_half(data), 2);
	return io_mem_write(addr, data, 2);
}

inline int FASTCALL ppc_write_physical_byte(uint32 addr, uint8 data)
{
#if 0
	if (addr < gMemorySize) {
		// big endian
		gMemory[addr] = data;
		return PPC_MMU_OK;
	}
#endif
	return io_mem_write(addr, data, 1);
}

inline int FASTCALL ppc_write_effective_qword(uint32 addr, Vector_t data)
{
	uint32 p;
	int r;

	addr &= ~0x0f;

	if (!((r=ppc_effective_to_physical(addr, PPC_MMU_WRITE, p)))) {
		return ppc_write_physical_qword(p, data);
	}
	return r;
}

inline int FASTCALL ppc_write_effective_dword(uint32 addr, uint64 data)
{
	uint32 p;
	int r;
	if (!((r=ppc_effective_to_physical(addr, PPC_MMU_WRITE, p)))) {
		if (EA_Offset(addr) > 4088) {
			// write overlaps two pages.. tricky
			byte *r1, *r2;
			byte b[14];
			ppc_effective_to_physical((addr & ~0xfff)+4089, PPC_MMU_WRITE, p);
			if ((r = ppc_direct_physical_memory_handle(p, r1))) return r;
			if ((r = ppc_effective_to_physical((addr & ~0xfff)+4096, PPC_MMU_WRITE, p))) return r;
			if ((r = ppc_direct_physical_memory_handle(p, r2))) return r;
			data = ppc_dword_to_BE(data);
			memmove(&b[0], r1, 7);
			memmove(&b[7], r2, 7);
			memmove(&b[EA_Offset(addr)-4089], &data, 8);
			memmove(r1, &b[0], 7);
			memmove(r2, &b[7], 7);
			return PPC_MMU_OK;
		} else {
			return ppc_write_physical_dword(p, data);
		}
	}
	return r;
}

inline int FASTCALL ppc_write_effective_word(uint32 addr, uint32 data)
{
	uint32 p;
	int r;
	if (!((r=ppc_effective_to_physical(addr, PPC_MMU_WRITE, p)))) {
		if (EA_Offset(addr) > 4092) {
			// write overlaps two pages.. tricky
			byte *r1, *r2;
			byte b[6];
			ppc_effective_to_physical((addr & ~0xfff)+4093, PPC_MMU_WRITE, p);
			if ((r = ppc_direct_physical_memory_handle(p, r1))) return r;
			if ((r = ppc_effective_to_physical((addr & ~0xfff)+4096, PPC_MMU_WRITE, p))) return r;
			if ((r = ppc_direct_physical_memory_handle(p, r2))) return r;
			data = ppc_word_to_BE(data);
			memmove(&b[0], r1, 3);
			memmove(&b[3], r2, 3);
			memmove(&b[EA_Offset(addr)-4093], &data, 4);
			memmove(r1, &b[0], 3);
			memmove(r2, &b[3], 3);
			return PPC_MMU_OK;
		} else {
			return ppc_write_physical_word(p, data);
		}
	}
	return r;
}

inline int FASTCALL ppc_write_effective_half(uint32 addr, uint16 data)
{
	uint32 p;
	int r;
	if (!((r=ppc_effective_to_physical(addr, PPC_MMU_WRITE, p)))) {
		if (EA_Offset(addr) > 4094) {
			// write overlaps two pages.. tricky
			ppc_effective_to_physical((addr & ~0xfff)+4095, PPC_MMU_WRITE, p);
			if ((r = ppc_write_physical_byte(p, data>>8))) return r;
			if ((r = ppc_effective_to_physical((addr & ~0xfff)+4096, PPC_MMU_WRITE, p))) return r;
			if ((r = ppc_write_physical_byte(p, data))) return r;
			return PPC_MMU_OK;
		} else {
			return ppc_write_physical_half(p, data);
		}
	}
	return r;
}

inline int FASTCALL ppc_write_effective_byte(uint32 addr, uint8 data)
{
	uint32 p;
	int r;
	if (!((r=ppc_effective_to_physical(addr, PPC_MMU_WRITE, p)))) {
		return ppc_write_physical_byte(p, data);
	}
	return r;
}

bool FASTCALL ppc_init_physical_memory(uint size)
{
#if 0
	if (size < 64*1024*1024) {
		PPC_MMU_ERR("Main memory size must >= 64MB!\n");
	}
	gMemory = (byte*)malloc(size);
	gMemorySize = size;
	return gMemory != NULL;
#else
	return NULL;
#endif
}

uint32  ppc_get_memory_size()
{
	return 0; //return gMemorySize;
}

/***************************************************************************
 *	DMA Interface
 */

bool	ppc_dma_write(uint32 dest, const void *src, uint32 size)
{
#if 0
	if (dest > gMemorySize || (dest+size) > gMemorySize) return false;
#endif	
	byte *ptr;
	ppc_direct_physical_memory_handle(dest, ptr);
	
	memcpy(ptr, src, size);
	return true;
}

bool	ppc_dma_read(void *dest, uint32 src, uint32 size)
{
#if 0
	if (src > gMemorySize || (src + size) > gMemorySize) return false;
#endif	
	byte *ptr;
	ppc_direct_physical_memory_handle(src, ptr);
	
	memcpy(dest, ptr, size);
	return true;
}

bool	ppc_dma_set(uint32 dest, int c, uint32 size)
{
#if 0
	if (dest > gMemorySize || (dest+size) > gMemorySize) return false;
#endif	
	byte *ptr;
	ppc_direct_physical_memory_handle(dest, ptr);
	
	memset(ptr, c, size);
	return true;
}

/***************************************************************************
 *	MMU Opcodes
 */

#include "ppc_dec.h"

/*
 *	dcbz		Data Cache Clear to Zero
 *	.464
 */
void ppc_opc_dcbz()
{
	//PPC_L1_CACHE_LINE_SIZE
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	// assert rD=0
	uint32 a = (rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB];
	// BAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
	ppc_write_effective_dword(a, 0)
	|| ppc_write_effective_dword(a+8, 0)
	|| ppc_write_effective_dword(a+16, 0)
	|| ppc_write_effective_dword(a+24, 0);
}

/*
 *	lbz		Load Byte and Zero
 *	.521
 */
void ppc_opc_lbz()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint8 r;
	int ret = ppc_read_effective_byte((rA?gCPU.gpr[rA]:0)+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lbzu		Load Byte and Zero with Update
 *	.522
 */
void ppc_opc_lbzu()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	// FIXME: check rA!=0 && rA!=rD
	uint8 r;
	int ret = ppc_read_effective_byte(gCPU.gpr[rA]+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
		gCPU.gpr[rD] = r;
	}	
}
/*
 *	lbzux		Load Byte and Zero with Update Indexed
 *	.523
 */
void ppc_opc_lbzux()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	// FIXME: check rA!=0 && rA!=rD
	uint8 r;
	int ret = ppc_read_effective_byte(gCPU.gpr[rA]+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lbzx		Load Byte and Zero Indexed
 *	.524
 */
void ppc_opc_lbzx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint8 r;
	int ret = ppc_read_effective_byte((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lfd		Load Floating-Point Double
 *	.530
 */
void ppc_opc_lfd()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frD, rA, imm);
	uint64 r;
	int ret = ppc_read_effective_dword((rA?gCPU.gpr[rA]:0)+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.fpr[frD] = r;
	}	
}
/*
 *	lfdu		Load Floating-Point Double with Update
 *	.531
 */
void ppc_opc_lfdu()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frD, rA, imm);
	// FIXME: check rA!=0
	uint64 r;
	int ret = ppc_read_effective_dword(gCPU.gpr[rA]+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.fpr[frD] = r;
		gCPU.gpr[rA] += imm;
	}	
}
/*
 *	lfdux		Load Floating-Point Double with Update Indexed
 *	.532
 */
void ppc_opc_lfdux()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, rA, rB);
	// FIXME: check rA!=0
	uint64 r;
	int ret = ppc_read_effective_dword(gCPU.gpr[rA]+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
		gCPU.fpr[frD] = r;
	}	
}
/*
 *	lfdx		Load Floating-Point Double Indexed
 *	.533
 */
void ppc_opc_lfdx()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, rA, rB);
	uint64 r;
	int ret = ppc_read_effective_dword((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.fpr[frD] = r;
	}	
}
/*
 *	lfs		Load Floating-Point Single
 *	.534
 */
void ppc_opc_lfs()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frD, rA, imm);
	uint32 r;
	int ret = ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+imm, r);
	if (ret == PPC_MMU_OK) {
		ppc_single s;
		ppc_double d;
		ppc_fpu_unpack_single(s, r);
		ppc_fpu_single_to_double(s, d);
		ppc_fpu_pack_double(d, gCPU.fpr[frD]);
	}	
}
/*
 *	lfsu		Load Floating-Point Single with Update
 *	.535
 */
void ppc_opc_lfsu()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frD, rA, imm);
	// FIXME: check rA!=0
	uint32 r;
	int ret = ppc_read_effective_word(gCPU.gpr[rA]+imm, r);
	if (ret == PPC_MMU_OK) {
		ppc_single s;
		ppc_double d;
		ppc_fpu_unpack_single(s, r);
		ppc_fpu_single_to_double(s, d);
		ppc_fpu_pack_double(d, gCPU.fpr[frD]);
		gCPU.gpr[rA] += imm;
	}	
}
/*
 *	lfsux		Load Floating-Point Single with Update Indexed
 *	.536
 */
void ppc_opc_lfsux()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, rA, rB);
	// FIXME: check rA!=0
	uint32 r;
	int ret = ppc_read_effective_word(gCPU.gpr[rA]+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
		ppc_single s;
		ppc_double d;
		ppc_fpu_unpack_single(s, r);
		ppc_fpu_single_to_double(s, d);
		ppc_fpu_pack_double(d, gCPU.fpr[frD]);
	}	
}
/*
 *	lfsx		Load Floating-Point Single Indexed
 *	.537
 */
void ppc_opc_lfsx()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frD, rA, rB);
	uint32 r;
	int ret = ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		ppc_single s;
		ppc_double d;
		ppc_fpu_unpack_single(s, r);
		ppc_fpu_single_to_double(s, d);
		ppc_fpu_pack_double(d, gCPU.fpr[frD]);
	}	
}
/*
 *	lha		Load Half Word Algebraic
 *	.538
 */
void ppc_opc_lha()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint16 r;
	int ret = ppc_read_effective_half((rA?gCPU.gpr[rA]:0)+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = (r&0x8000)?(r|0xffff0000):r;
	}
}
/*
 *	lhau		Load Half Word Algebraic with Update
 *	.539
 */
void ppc_opc_lhau()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint16 r;
	// FIXME: rA != 0
	int ret = ppc_read_effective_half(gCPU.gpr[rA]+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
		gCPU.gpr[rD] = (r&0x8000)?(r|0xffff0000):r;
	}
}
/*
 *	lhaux		Load Half Word Algebraic with Update Indexed
 *	.540
 */
void ppc_opc_lhaux()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint16 r;
	// FIXME: rA != 0
	int ret = ppc_read_effective_half(gCPU.gpr[rA]+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
		gCPU.gpr[rD] = (r&0x8000)?(r|0xffff0000):r;
	}
}
/*
 *	lhax		Load Half Word Algebraic Indexed
 *	.541
 */
void ppc_opc_lhax()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint16 r;
	// FIXME: rA != 0
	int ret = ppc_read_effective_half((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = (r&0x8000) ? (r|0xffff0000):r;
	}
}
/*
 *	lhbrx		Load Half Word Byte-Reverse Indexed
 *	.542
 */
void ppc_opc_lhbrx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint16 r;
	int ret = ppc_read_effective_half((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = ppc_bswap_half(r);
	}
}
/*
 *	lhz		Load Half Word and Zero
 *	.543
 */
void ppc_opc_lhz()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint16 r;
	int ret = ppc_read_effective_half((rA?gCPU.gpr[rA]:0)+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lhzu		Load Half Word and Zero with Update
 *	.544
 */
void ppc_opc_lhzu()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint16 r;
	// FIXME: rA!=0
	int ret = ppc_read_effective_half(gCPU.gpr[rA]+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
		gCPU.gpr[rA] += imm;
	}
}
/*
 *	lhzux		Load Half Word and Zero with Update Indexed
 *	.545
 */
void ppc_opc_lhzux()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint16 r;
	// FIXME: rA != 0
	int ret = ppc_read_effective_half(gCPU.gpr[rA]+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lhzx		Load Half Word and Zero Indexed
 *	.546
 */
void ppc_opc_lhzx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint16 r;
	int ret = ppc_read_effective_half((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lmw		Load Multiple Word
 *	.547
 */
void ppc_opc_lmw()
{
	int rD, rA;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint32 ea = (rA ? gCPU.gpr[rA] : 0) + imm;
	while (rD <= 31) {
		if (ppc_read_effective_word(ea, gCPU.gpr[rD])) {
			return;
		}
		rD++;
		ea += 4;
	}
}
/*
 *	lswi		Load String Word Immediate
 *	.548
 */
void ppc_opc_lswi()
{
	int rA, rD, NB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, NB);
	if (NB==0) NB=32;
	uint32 ea = rA ? gCPU.gpr[rA] : 0;
	uint32 r = 0;
	int i = 4;
	uint8 v;
	while (NB > 0) {
		if (!i) {
			i = 4;
			gCPU.gpr[rD] = r;
			rD++;
			rD%=32;
			r = 0;
		}
		if (ppc_read_effective_byte(ea, v)) {
			return;
		}
		r<<=8;
		r|=v;
		ea++;
		i--;
		NB--;
	}
	while (i) { r<<=8; i--; }
	gCPU.gpr[rD] = r;
}
/*
 *	lswx		Load String Word Indexed
 *	.550
 */
void ppc_opc_lswx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	int NB = XER_n(gCPU.xer);
	uint32 ea = gCPU.gpr[rB] + (rA ? gCPU.gpr[rA] : 0);

	uint32 r = 0;
	int i = 4;
	uint8 v;
	while (NB > 0) {
		if (!i) {
			i = 4;
			gCPU.gpr[rD] = r;
			rD++;
			rD%=32;
			r = 0;
		}
		if (ppc_read_effective_byte(ea, v)) {
			return;
		}
		r<<=8;
		r|=v;
		ea++;
		i--;
		NB--;
	}
	while (i) { r<<=8; i--; }
	gCPU.gpr[rD] = r;
}
/*
 *	lwarx		Load Word and Reserve Indexed
 *	.553
 */
void ppc_opc_lwarx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint32 r;
	int ret = ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
		gCPU.reserve = r;
		gCPU.have_reservation = 1;
	}
}
/*
 *	lwbrx		Load Word Byte-Reverse Indexed
 *	.556
 */
void ppc_opc_lwbrx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint32 r;
	int ret = ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = ppc_bswap_word(r);
	}
}
/*
 *	lwz		Load Word and Zero
 *	.557
 */
void ppc_opc_lwz()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	uint32 r;
	int ret = ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
	}	
}
/*
 *	lbzu		Load Word and Zero with Update
 *	.558
 */
void ppc_opc_lwzu()
{
	int rA, rD;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rD, rA, imm);
	// FIXME: check rA!=0 && rA!=rD
	uint32 r;
	int ret = ppc_read_effective_word(gCPU.gpr[rA]+imm, r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
		gCPU.gpr[rD] = r;
	}	
}
/*
 *	lwzux		Load Word and Zero with Update Indexed
 *	.559
 */
void ppc_opc_lwzux()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	// FIXME: check rA!=0 && rA!=rD
	uint32 r;
	int ret = ppc_read_effective_word(gCPU.gpr[rA]+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
		gCPU.gpr[rD] = r;
	}
}
/*
 *	lwzx		Load Word and Zero Indexed
 *	.560
 */
void ppc_opc_lwzx()
{
	int rA, rD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rD, rA, rB);
	uint32 r;
	int ret = ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], r);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rD] = r;
	}
}

/*      lvx	     Load Vector Indexed
 *      v.127
 */
void ppc_opc_lvx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrD, rA, rB);
	Vector_t r;

	int ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]);

	int ret = ppc_read_effective_qword(ea, r);
	if (ret == PPC_MMU_OK) {
		gCPU.vr[vrD] = r;
	}
}

/*      lvxl	    Load Vector Index LRU
 *      v.128
 */
void ppc_opc_lvxl()
{
	ppc_opc_lvx();
	/* This instruction should hint to the cache that the value won't be
	 *   needed again in memory anytime soon.  We don't emulate the cache,
	 *   so this is effectively exactly the same as lvx.
	 */
}

/*      lvebx	   Load Vector Element Byte Indexed
 *      v.119
 */
void ppc_opc_lvebx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrD, rA, rB);
	uint32 ea;
	uint8 r;
	ea = (rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB];
	int ret = ppc_read_effective_byte(ea, r);
	if (ret == PPC_MMU_OK) {
		VECT_B(gCPU.vr[vrD], ea & 0xf) = r;
	}
}

/*      lvehx	   Load Vector Element Half Word Indexed
 *      v.121
 */
void ppc_opc_lvehx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrD, rA, rB);
	uint32 ea;
	uint16 r;
	ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]) & ~1;
	int ret = ppc_read_effective_half(ea, r);
	if (ret == PPC_MMU_OK) {
		VECT_H(gCPU.vr[vrD], (ea & 0xf) >> 1) = r;
	}
}

/*      lvewx	   Load Vector Element Word Indexed
 *      v.122
 */
void ppc_opc_lvewx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrD, rA, rB);
	uint32 ea;
	uint32 r;
	ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]) & ~3;
	int ret = ppc_read_effective_word(ea, r);
	if (ret == PPC_MMU_OK) {
		VECT_W(gCPU.vr[vrD], (ea & 0xf) >> 2) = r;
	}
}

#if HOST_ENDIANESS == HOST_ENDIANESS_LE
static byte lvsl_helper[] = {
	0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
	0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
	0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};
#elif HOST_ENDIANESS == HOST_ENDIANESS_BE
static byte lvsl_helper[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};
#else
#error Endianess not supported!
#endif

/*
 *      lvsl	    Load Vector for Shift Left
 *      v.123
 */
void ppc_opc_lvsl()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrD, rA, rB);
	uint32 ea;
	ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]);
#if HOST_ENDIANESS == HOST_ENDIANESS_LE
	memmove(&gCPU.vr[vrD], lvsl_helper+0x10-(ea & 0xf), 16);
#elif HOST_ENDIANESS == HOST_ENDIANESS_BE
	memmove(&gCPU.vr[vrD], lvsl_helper+(ea & 0xf), 16);
#else
#error Endianess not supported!
#endif
}

/*
 *      lvsr	    Load Vector for Shift Right
 *      v.125
 */
void ppc_opc_lvsr()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrD, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrD, rA, rB);
	uint32 ea;
	ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]);
#if HOST_ENDIANESS == HOST_ENDIANESS_LE
	memmove(&gCPU.vr[vrD], lvsl_helper+(ea & 0xf), 16);
#elif HOST_ENDIANESS == HOST_ENDIANESS_BE
	memmove(&gCPU.vr[vrD], lvsl_helper+0x10-(ea & 0xf), 16);
#else
#error Endianess not supported!
#endif
}

/*
 *      dst	     Data Stream Touch
 *      v.115
 */
void ppc_opc_dst()
{
	VECTOR_DEBUG;
	/* Since we are not emulating the cache, this is a nop */
}

/*
 *	stb		Store Byte
 *	.632
 */
void ppc_opc_stb()
{
	int rA, rS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	ppc_write_effective_byte((rA?gCPU.gpr[rA]:0)+imm, (uint8)gCPU.gpr[rS]) != PPC_MMU_FATAL;
}
/*
 *	stbu		Store Byte with Update
 *	.633
 */
void ppc_opc_stbu()
{
	int rA, rS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_byte(gCPU.gpr[rA]+imm, (uint8)gCPU.gpr[rS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
	}
}
/*
 *	stbux		Store Byte with Update Indexed
 *	.634
 */
void ppc_opc_stbux()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_byte(gCPU.gpr[rA]+gCPU.gpr[rB], (uint8)gCPU.gpr[rS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
	}
}
/*
 *	stbx		Store Byte Indexed
 *	.635
 */
void ppc_opc_stbx()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	ppc_write_effective_byte((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], (uint8)gCPU.gpr[rS]) != PPC_MMU_FATAL;
}
/*
 *	stfd		Store Floating-Point Double
 *	.642
 */
void ppc_opc_stfd()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frS, rA, imm);
	ppc_write_effective_dword((rA?gCPU.gpr[rA]:0)+imm, gCPU.fpr[frS]) != PPC_MMU_FATAL;
}
/*
 *	stfdu		Store Floating-Point Double with Update
 *	.643
 */
void ppc_opc_stfdu()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frS, rA, imm);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_dword(gCPU.gpr[rA]+imm, gCPU.fpr[frS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
	}
}
/*
 *	stfd		Store Floating-Point Double with Update Indexed
 *	.644
 */
void ppc_opc_stfdux()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frS, rA, rB);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_dword(gCPU.gpr[rA]+gCPU.gpr[rB], gCPU.fpr[frS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
	}
}
/*
 *	stfdx		Store Floating-Point Double Indexed
 *	.645
 */
void ppc_opc_stfdx()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frS, rA, rB);
	ppc_write_effective_dword((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], gCPU.fpr[frS]) != PPC_MMU_FATAL;
}
/*
 *	stfiwx		Store Floating-Point as Integer Word Indexed
 *	.646
 */
void ppc_opc_stfiwx()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frS, rA, rB);
	ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], (uint32)gCPU.fpr[frS]) != PPC_MMU_FATAL;
}
/*
 *	stfs		Store Floating-Point Single
 *	.647
 */
void ppc_opc_stfs()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frS, rA, imm);
	uint32 s;
	ppc_double d;
	ppc_fpu_unpack_double(d, gCPU.fpr[frS]);
	ppc_fpu_pack_single(d, s);
	ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+imm, s) != PPC_MMU_FATAL;
}
/*
 *	stfsu		Store Floating-Point Single with Update
 *	.648
 */
void ppc_opc_stfsu()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, frS, rA, imm);
	// FIXME: check rA!=0
	uint32 s;
	ppc_double d;
	ppc_fpu_unpack_double(d, gCPU.fpr[frS]);
	ppc_fpu_pack_single(d, s);
	int ret = ppc_write_effective_word(gCPU.gpr[rA]+imm, s);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
	}
}
/*
 *	stfsux		Store Floating-Point Single with Update Indexed
 *	.649
 */
void ppc_opc_stfsux()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frS, rA, rB);
	// FIXME: check rA!=0
	uint32 s;
	ppc_double d;
	ppc_fpu_unpack_double(d, gCPU.fpr[frS]);
	ppc_fpu_pack_single(d, s);
	int ret = ppc_write_effective_word(gCPU.gpr[rA]+gCPU.gpr[rB], s);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
	}
}
/*
 *	stfsx		Store Floating-Point Single Indexed
 *	.650
 */
void ppc_opc_stfsx()
{
	if ((gCPU.msr & MSR_FP) == 0) {
		ppc_exception(PPC_EXC_NO_FPU);
		return;
	}
	int rA, frS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, frS, rA, rB);
	uint32 s;
	ppc_double d;
	ppc_fpu_unpack_double(d, gCPU.fpr[frS]);
	ppc_fpu_pack_single(d, s);
	ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], s) != PPC_MMU_FATAL;
}
/*
 *	sth		Store Half Word
 *	.651
 */
void ppc_opc_sth()
{
	int rA, rS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	ppc_write_effective_half((rA?gCPU.gpr[rA]:0)+imm, (uint16)gCPU.gpr[rS]) != PPC_MMU_FATAL;
}
/*
 *	sthbrx		Store Half Word Byte-Reverse Indexed
 *	.652
 */
void ppc_opc_sthbrx()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	ppc_write_effective_half((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], ppc_bswap_half(gCPU.gpr[rS])) != PPC_MMU_FATAL;
}
/*
 *	sthu		Store Half Word with Update
 *	.653
 */
void ppc_opc_sthu()
{
	int rA, rS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_half(gCPU.gpr[rA]+imm, (uint16)gCPU.gpr[rS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
	}
}
/*
 *	sthux		Store Half Word with Update Indexed
 *	.654
 */
void ppc_opc_sthux()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_half(gCPU.gpr[rA]+gCPU.gpr[rB], (uint16)gCPU.gpr[rS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
	}
}
/*
 *	sthx		Store Half Word Indexed
 *	.655
 */
void ppc_opc_sthx()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	ppc_write_effective_half((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], (uint16)gCPU.gpr[rS]) != PPC_MMU_FATAL;
}
/*
 *	stmw		Store Multiple Word
 *	.656
 */
void ppc_opc_stmw()
{
	int rS, rA;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	uint32 ea = (rA ? gCPU.gpr[rA] : 0) + imm;
	while (rS <= 31) {
		if (ppc_write_effective_word(ea, gCPU.gpr[rS])) {
			return;
		}
		rS++;
		ea += 4;
	}
}
/*
 *	stswi		Store String Word Immediate
 *	.657
 */
void ppc_opc_stswi()
{
	int rA, rS, NB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, NB);
	if (NB==0) NB=32;
	uint32 ea = rA ? gCPU.gpr[rA] : 0;
	uint32 r = 0;
	int i = 0;
	
	while (NB > 0) {
		if (!i) {
			r = gCPU.gpr[rS];
			rS++;
			rS%=32;
			i = 4;
		}
		if (ppc_write_effective_byte(ea, (r>>24))) {
			return;
		}
		r<<=8;
		ea++;
		i--;
		NB--;
	}
}
/*
 *	stswx		Store String Word Indexed
 *	.658
 */
void ppc_opc_stswx()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	int NB = XER_n(gCPU.xer);
	uint32 ea = gCPU.gpr[rB] + (rA ? gCPU.gpr[rA] : 0);
	uint32 r = 0;
	int i = 0;
	
	while (NB > 0) {
		if (!i) {
			r = gCPU.gpr[rS];
			rS++;
			rS%=32;
			i = 4;
		}
		if (ppc_write_effective_byte(ea, (r>>24))) {
			return;
		}
		r<<=8;
		ea++;
		i--;
		NB--;
	}
}
/*
 *	stw		Store Word
 *	.659
 */
void ppc_opc_stw()
{
	int rA, rS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+imm, gCPU.gpr[rS]) != PPC_MMU_FATAL;
}
/*
 *	stwbrx		Store Word Byte-Reverse Indexed
 *	.660
 */
void ppc_opc_stwbrx()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	// FIXME: doppelt gemoppelt
	ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], ppc_bswap_word(gCPU.gpr[rS])) != PPC_MMU_FATAL;
}
/*
 *	stwcx.		Store Word Conditional Indexed
 *	.661
 */
void ppc_opc_stwcx_()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	gCPU.cr &= 0x0fffffff;
	if (gCPU.have_reservation) {
		gCPU.have_reservation = false;
		uint32 v;
		if (ppc_read_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], v)) {
			return;
		}
		if (v==gCPU.reserve) {
			if (ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], gCPU.gpr[rS])) {
				return;
			}
			gCPU.cr |= CR_CR0_EQ;
		}
		if (gCPU.xer & XER_SO) {
			gCPU.cr |= CR_CR0_SO;
		}
	}
}
/*
 *	stwu		Store Word with Update
 *	.663
 */
void ppc_opc_stwu()
{
	int rA, rS;
	uint32 imm;
	PPC_OPC_TEMPL_D_SImm(gCPU.current_opc, rS, rA, imm);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_word(gCPU.gpr[rA]+imm, gCPU.gpr[rS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += imm;
	}
}
/*
 *	stwux		Store Word with Update Indexed
 *	.664
 */
void ppc_opc_stwux()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	// FIXME: check rA!=0
	int ret = ppc_write_effective_word(gCPU.gpr[rA]+gCPU.gpr[rB], gCPU.gpr[rS]);
	if (ret == PPC_MMU_OK) {
		gCPU.gpr[rA] += gCPU.gpr[rB];
	}
}
/*
 *	stwx		Store Word Indexed
 *	.665
 */
void ppc_opc_stwx()
{
	int rA, rS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, rS, rA, rB);
	ppc_write_effective_word((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB], gCPU.gpr[rS]) != PPC_MMU_FATAL;
}

/*      stvx	    Store Vector Indexed
 *      v.134
 */
void ppc_opc_stvx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrS, rA, rB);

	int ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]);

	ppc_write_effective_qword(ea, gCPU.vr[vrS]) != PPC_MMU_FATAL;
}

/*      stvxl	   Store Vector Indexed LRU
 *      v.135
 */
void ppc_opc_stvxl()
{
	ppc_opc_stvx();
	/* This instruction should hint to the cache that the value won't be
	 *   needed again in memory anytime soon.  We don't emulate the cache,
	 *   so this is effectively exactly the same as lvx.
	 */
}

/*      stvebx	  Store Vector Element Byte Indexed
 *      v.131
 */
void ppc_opc_stvebx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrS, rA, rB);
	uint32 ea;
	ea = (rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB];
	ppc_write_effective_byte(ea, VECT_B(gCPU.vr[vrS], ea & 0xf));
}

/*      stvehx	  Store Vector Element Half Word Indexed
 *      v.132
 */
void ppc_opc_stvehx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrS, rA, rB);
	uint32 ea;
	ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]) & ~1;
	ppc_write_effective_half(ea, VECT_H(gCPU.vr[vrS], (ea & 0xf) >> 1));
}

/*      stvewx	  Store Vector Element Word Indexed
 *      v.133
 */
void ppc_opc_stvewx()
{
#ifndef __VEC_EXC_OFF__
	if ((gCPU.msr & MSR_VEC) == 0) {
		ppc_exception(PPC_EXC_NO_VEC);
		return;
	}
#endif
	VECTOR_DEBUG;
	int rA, vrS, rB;
	PPC_OPC_TEMPL_X(gCPU.current_opc, vrS, rA, rB);
	uint32 ea;
	ea = ((rA?gCPU.gpr[rA]:0)+gCPU.gpr[rB]) & ~3;
	ppc_write_effective_word(ea, VECT_W(gCPU.vr[vrS], (ea & 0xf) >> 2));
}

/*      dstst	   Data Stream Touch for Store
 *      v.117
 */
void ppc_opc_dstst()
{
	VECTOR_DEBUG;
	/* Since we are not emulating the cache, this is a nop */
}

/*      dss	     Data Stream Stop
 *      v.114
 */
void ppc_opc_dss()
{
	VECTOR_DEBUG;
	/* Since we are not emulating the cache, this is a nop */
}
