/* Emulation of MC68030 MMU
 * This code has been written for Previous - a NeXT Computer emulator
 *
 * This file is distributed under the GNU General Public License, version 2
 * or at your option any later version. Read the file gpl.txt for details.
 *
 *
 * Written by Andreas Grabher
 *
 * Many thanks go to Thomas Huth and the Hatari community for helping
 * to test and debug this code!
 *
 *
 * Release notes:
 * 01-09-2012: First release
 * 29-09-2012: Improved function code handling
 * 16-11-2012: Improved exception handling
 *
 *
 * - Check if read-modify-write operations are correctly detected for
 *   handling transparent access (see TT matching functions)
 * - If possible, test mmu030_table_search with all kinds of translations
 *   (early termination, invalid descriptors, bus errors, indirect
 *   descriptors, PTEST in different levels, etc).
 * - Check which bits of an ATC entry or the status register should be set 
 *   and which should be un-set, if an invalid translation occurs.
 * - Handle cache inhibit bit when accessing ATC entries
 */


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "debug.h"
#include "cpummu030.h"
#include "cputbl.h"
#include "savestate.h"

// Prefetch mode and prefetch bus error: always flush and refill prefetch pipeline
#define MMU030_ALWAYS_FULL_PREFETCH 1
 // if CPU is 68030 and faulted access' addressing mode was -(an) or (an)+
// register content is not restored when exception starts.
#define MMU030_REG_FIXUP 1

#define MMU030_OP_DBG_MSG 0
#define MMU030_ATC_DBG_MSG 0
#define MMU030_REG_DBG_MSG 0

#define TT_FC_MASK      0x00000007
#define TT_FC_BASE      0x00000070
#define TT_RWM          0x00000100
#define TT_RW           0x00000200
#define TT_CI           0x00000400
#define TT_ENABLE       0x00008000

#define TT_ADDR_MASK    0x00FF0000
#define TT_ADDR_BASE    0xFF000000

static int bBusErrorReadWrite;
static int atcindextable[32];
static int tt_enabled;

int mmu030_idx, mmu030_idx_done;

uae_u32 mm030_stageb_address;
bool mmu030_retry;
int mmu030_opcode;
int mmu030_opcode_stageb;

int mmu030_fake_prefetch;
uaecptr mmu030_fake_prefetch_addr;

uae_u16 mmu030_state[3];
uae_u32 mmu030_data_buffer_out;
uae_u32 mmu030_disp_store[2];
uae_u32 mmu030_fmovem_store[2];
uae_u8 mmu030_cache_state;
struct mmu030_access mmu030_ad[MAX_MMU030_ACCESS + 1];
bool ismoves030, islrmw030;

static void mmu030_ptest_atc_search(uaecptr logical_addr, uae_u32 fc, bool write);
static uae_u32 mmu030_table_search(uaecptr addr, uae_u32 fc, bool write, int level);
static TT_info mmu030_decode_tt(uae_u32 TT);

#if MMU_DPAGECACHE030
#define MMUFASTCACHE_ENTRIES030 256
struct mmufastcache030
{
	uae_u32 log;
	uae_u32 phys;
	uae_u8 cs;
};
static struct mmufastcache030 atc_data_cache_read[MMUFASTCACHE_ENTRIES030];
static struct mmufastcache030 atc_data_cache_write[MMUFASTCACHE_ENTRIES030];
#endif

/* for debugging messages */
static char table_letter[4] = {'A','B','C','D'};

static const uae_u32 mmu030_size[3] = { MMU030_SSW_SIZE_B, MMU030_SSW_SIZE_W, MMU030_SSW_SIZE_L };

uae_u64 srp_030, crp_030;
uae_u32 tt0_030, tt1_030, tc_030;
uae_u16 mmusr_030;

/* ATC struct */
#define ATC030_NUM_ENTRIES  22

typedef struct {
    struct {
        uaecptr addr;
        bool modified;
        bool write_protect;
        uae_u8 cache_inhibit;
        bool bus_error;
    } physical;
    
    struct {
        uaecptr addr;
        uae_u32 fc;
        bool valid;
    } logical;
    /* history bit */
    int mru;
} MMU030_ATC_LINE;


/* MMU struct for 68030 */
static struct {
    
    /* Translation tables */
    struct {
        struct {
            uae_u32 mask;
            uae_u8 shift;
        } table[4];
        
        struct {
            uae_u32 mask;
			uae_u32 imask;
            uae_u32 size;
			uae_u32 size3m;
        } page;
        
        uae_u8 init_shift;
        uae_u8 last_table;
    } translation;
    
    /* Transparent translation */
    struct {
        TT_info tt0;
        TT_info tt1;
    } transparent;
    
    /* Address translation cache */
    MMU030_ATC_LINE atc[ATC030_NUM_ENTRIES];
    
    /* Condition */
    bool enabled;
    uae_u16 status;

#if MMU_IPAGECACHE030
	uae_u8 mmu030_cache_state;
#if MMU_DIRECT_ACCESS
	uae_u8 *mmu030_last_physical_address_real;
#else
	uae_u32 mmu030_last_physical_address;
#endif
	uae_u32 mmu030_last_logical_address;
#endif

} mmu030;

/* MMU Status Register
 *
 * ---x ---x x-xx x---
 * reserved (all 0)
 *
 * x--- ---- ---- ----
 * bus error
 *
 * -x-- ---- ---- ----
 * limit violation
 *
 * --x- ---- ---- ----
 * supervisor only
 *
 * ---- x--- ---- ----
 * write protected
 *
 * ---- -x-- ---- ----
 * invalid
 *
 * ---- --x- ---- ----
 * modified
 *
 * ---- ---- -x-- ----
 * transparent access
 *
 * ---- ---- ---- -xxx
 * number of levels (number of tables accessed during search)
 *
 */

#define MMUSR_BUS_ERROR         0x8000
#define MMUSR_LIMIT_VIOLATION   0x4000
#define MMUSR_SUPER_VIOLATION   0x2000
#define MMUSR_WRITE_PROTECTED   0x0800
#define MMUSR_INVALID           0x0400
#define MMUSR_MODIFIED          0x0200
#define MMUSR_TRANSP_ACCESS     0x0040
#define MMUSR_NUM_LEVELS_MASK   0x0007

/* -- ATC flushing functions -- */

static void mmu030_flush_cache(uaecptr addr)
{
#if MMU_IPAGECACHE030
	mmu030.mmu030_last_logical_address = 0xffffffff;
#endif
#if MMU_DPAGECACHE030
	if (addr == 0xffffffff) {
		memset(&atc_data_cache_read, 0xff, sizeof atc_data_cache_read);
		memset(&atc_data_cache_write, 0xff, sizeof atc_data_cache_write);
	} else {
		uae_u32 idx = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | 7;
		for (int i = 0; i < MMUFASTCACHE_ENTRIES030; i++) {
			if ((atc_data_cache_read[i].log | 7) == idx)
				atc_data_cache_read[i].log = 0xffffffff;
			if ((atc_data_cache_write[i].log | 7) == idx)
				atc_data_cache_write[i].log = 0xffffffff;
		}
	}
#endif
}

/* This function flushes ATC entries depending on their function code */
static void mmu030_flush_atc_fc(uae_u32 fc_base, uae_u32 fc_mask) {
    int i;
    for (i=0; i<ATC030_NUM_ENTRIES; i++) {
        if (((fc_base&fc_mask)==(mmu030.atc[i].logical.fc&fc_mask)) &&
            mmu030.atc[i].logical.valid) {
            mmu030.atc[i].logical.valid = false;
#if MMU030_OP_DBG_MSG
            write_log(_T("ATC: Flushing %08X\n"), mmu030.atc[i].physical.addr);
#endif
		}
    }
	mmu030_flush_cache(0xffffffff);
}

/* This function flushes ATC entries depending on their logical address
 * and their function code */
static void mmu030_flush_atc_page_fc(uaecptr logical_addr, uae_u32 fc_base, uae_u32 fc_mask) {
    int i;
	logical_addr &= mmu030.translation.page.imask;
    for (i=0; i<ATC030_NUM_ENTRIES; i++) {
        if (((fc_base&fc_mask)==(mmu030.atc[i].logical.fc&fc_mask)) &&
            (mmu030.atc[i].logical.addr == logical_addr) &&
            mmu030.atc[i].logical.valid) {
            mmu030.atc[i].logical.valid = false;
#if MMU030_OP_DBG_MSG
            write_log(_T("ATC: Flushing %08X\n"), mmu030.atc[i].physical.addr);
#endif
		}
    }
	mmu030_flush_cache(logical_addr);
}

/* This function flushes ATC entries depending on their logical address */
static void mmu030_flush_atc_page(uaecptr logical_addr) {
    int i;
	logical_addr &= mmu030.translation.page.imask;
    for (i=0; i<ATC030_NUM_ENTRIES; i++) {
        if ((mmu030.atc[i].logical.addr == logical_addr) &&
            mmu030.atc[i].logical.valid) {
            mmu030.atc[i].logical.valid = false;
#if MMU030_OP_DBG_MSG
            write_log(_T("ATC: Flushing %08X\n"), mmu030.atc[i].physical.addr);
#endif
		}
    }
	mmu030_flush_cache(logical_addr);
}

/* This function flushes all ATC entries */
void mmu030_flush_atc_all(void) {
#if MMU030_OP_DBG_MSG
	write_log(_T("ATC: Flushing all entries\n"));
#endif
	int i;
    for (i=0; i<ATC030_NUM_ENTRIES; i++) {
        mmu030.atc[i].logical.valid = false;
    }
	mmu030_flush_cache(0xffffffff);
}

/* -- Helper function for MMU instructions -- */
static bool mmu_op30_helper_get_fc(uae_u16 next, uae_u32 *fc) {
	switch (next & 0x0018) {
	case 0x0010:
	*fc = next & 0x7;
	return true;
	case 0x0008:
	*fc = m68k_dreg(regs, next & 0x7) & 0x7;
	return true;
	case 0x0000:
	if (next & 1) {
		*fc = regs.dfc;
	} else {
		*fc = regs.sfc;
	}
	return true;
	default:
	write_log(_T("MMU_OP30 ERROR: bad fc source! (%04X)\n"), next & 0x0018);
	return false;
	}
}

/* -- MMU instructions -- */

static bool mmu_op30_invea(uae_u32 opcode)
{
	int eamode = (opcode >> 3) & 7;
	int rreg = opcode & 7;

	// Dn, An, (An)+, -(An), immediate and PC-relative not allowed
	if (eamode == 0 || eamode == 1 || eamode == 3 || eamode == 4 || (eamode == 7 && rreg > 1))
		return true;
	return false;
}

int mmu_op30_pmove(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	int preg = (next >> 10) & 31;
	int rw = (next >> 9) & 1;
	int fd = (next >> 8) & 1;
 	int unused = (next & 0xff);
   
	if (mmu_op30_invea(opcode))
		return 1;
	// unused low 8 bits must be zeroed
	if (unused)
		return 1;
	// read and fd set?
	if (rw && fd)
		return 1;

#if MMU030_OP_DBG_MSG
    switch (preg) {
        case 0x10:
            write_log(_T("PMOVE: %s TC %08X PC=%08x\n"), rw ? _T("read"):  _T("write"),
                      rw ? tc_030 : x_get_long(extra), m68k_getpc());
            break;
        case 0x12:
            write_log(_T("PMOVE: %s SRP %08X%08X PC=%08x\n"), rw ? _T("read") : _T("write"),
                      rw?(uae_u32)(srp_030>>32)&0xFFFFFFFF:x_get_long(extra),
                      rw?(uae_u32)srp_030&0xFFFFFFFF:x_get_long(extra+4), m68k_getpc());
            break;
        case 0x13:
            write_log(_T("PMOVE: %s CRP %08X%08X PC=%08x\n"), rw ? _T("read") : _T("write"),
                      rw?(uae_u32)(crp_030>>32)&0xFFFFFFFF:x_get_long(extra),
                      rw?(uae_u32)crp_030&0xFFFFFFFF:x_get_long(extra+4), m68k_getpc());
            break;
        case 0x18:
            write_log(_T("PMOVE: %s MMUSR %04X PC=%08x\n"), rw ? _T("read") : _T("write"),
                      rw?mmusr_030:x_get_word(extra), m68k_getpc());
            break;
        case 0x02:
            write_log(_T("PMOVE: %s TT0 %08X PC=%08x\n"), rw ? _T("read") : _T("write"),
                      rw?tt0_030:x_get_long(extra), m68k_getpc());
            break;
        case 0x03:
            write_log(_T("PMOVE: %s TT1 %08X PC=%08x\n"), rw ? _T("read") : _T("write"),
                      rw?tt1_030:x_get_long(extra), m68k_getpc());
            break;
        default:
            break;
    }
    if (!fd && !rw && !(preg==0x18)) {
        write_log(_T("PMOVE: flush ATC\n"));
    }
#endif
    
	switch (preg)
	{
        case 0x10: // TC
            if (rw)
                x_put_long (extra, tc_030);
            else {
                tc_030 = x_get_long (extra);
                if (mmu030_decode_tc(tc_030, true))
					return -1;
            }
            break;
        case 0x12: // SRP
            if (rw) {
                x_put_long (extra, srp_030 >> 32);
                x_put_long (extra + 4, (uae_u32)srp_030);
            } else {
                srp_030 = (uae_u64)x_get_long (extra) << 32;
                srp_030 |= x_get_long (extra + 4);
                if (mmu030_decode_rp(srp_030))
					return -1;
            }
            break;
        case 0x13: // CRP
            if (rw) {
                x_put_long (extra, crp_030 >> 32);
                x_put_long (extra + 4, (uae_u32)crp_030);
            } else {
                crp_030 = (uae_u64)x_get_long (extra) << 32;
                crp_030 |= x_get_long (extra + 4);
                if (mmu030_decode_rp(crp_030))
					return -1;
            }
            break;
        case 0x18: // MMUSR
			if (fd) {
				// FD must be always zero when MMUSR read or write
				return 1;
			}
            if (rw)
                x_put_word (extra, mmusr_030);
            else
                mmusr_030 = x_get_word (extra);
            break;
        case 0x02: // TT0
            if (rw)
                x_put_long (extra, tt0_030);
            else {
                tt0_030 = x_get_long (extra);
                mmu030.transparent.tt0 = mmu030_decode_tt(tt0_030);
            }
            break;
        case 0x03: // TT1
            if (rw)
                x_put_long (extra, tt1_030);
            else {
                tt1_030 = x_get_long (extra);
                mmu030.transparent.tt1 = mmu030_decode_tt(tt1_030);
            }
            break;
        default:
            write_log (_T("Bad PMOVE at %08x\n"),m68k_getpc());
            return 1;
	}
    
    if (!fd && !rw && preg != 0x18) {
        mmu030_flush_atc_all();
    }
	tt_enabled = (tt0_030 & TT_ENABLE) || (tt1_030 & TT_ENABLE);
	return 0;
}

bool mmu_op30_ptest (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
    mmu030.status = mmusr_030 = 0;
    
    int level = (next&0x1C00)>>10;
    int rw = (next >> 9) & 1;
    int a = (next >> 8) & 1;
    int areg = (next&0xE0)>>5;
	uae_u32 fc;
	bool write = rw ? false : true;
    uae_u32 ret = 0;

	if (mmu_op30_invea(opcode))
		return true;
	if (!mmu_op30_helper_get_fc(next, &fc))
		return true;
	if (!level && a) {
        write_log(_T("PTEST: Bad instruction causing F-line unimplemented instruction exception!\n"));
        return true;
    }
        
#if MMU030_OP_DBG_MSG
    write_log(_T("PTEST%c: addr = %08X, fc = %i, level = %i, PC=%08x, "),
              rw?'R':'W', extra, fc, level, m68k_getpc());
    if (a) {
        write_log(_T("return descriptor to register A%i\n"), areg);
    } else {
        write_log(_T("do not return descriptor\n"));
    }
#endif
    
    if (!level) {
        mmu030_ptest_atc_search(extra, fc, write);
    } else {
        ret = mmu030_table_search(extra, fc, write, level);
        if (a) {
            m68k_areg (regs, areg) = ret;
        }
    }
    mmusr_030 = mmu030.status;
    
#if MMU030_OP_DBG_MSG
    write_log(_T("PTEST status: %04X, B = %i, L = %i, S = %i, W = %i, I = %i, M = %i, T = %i, N = %i\n"),
              mmusr_030, (mmusr_030&MMUSR_BUS_ERROR)?1:0, (mmusr_030&MMUSR_LIMIT_VIOLATION)?1:0,
              (mmusr_030&MMUSR_SUPER_VIOLATION)?1:0, (mmusr_030&MMUSR_WRITE_PROTECTED)?1:0,
              (mmusr_030&MMUSR_INVALID)?1:0, (mmusr_030&MMUSR_MODIFIED)?1:0,
              (mmusr_030&MMUSR_TRANSP_ACCESS)?1:0, mmusr_030&MMUSR_NUM_LEVELS_MASK);
#endif
	return false;
}

static bool mmu_op30_pload (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
    int rw = (next >> 9) & 1;
  	int unused = (next & (0x100 | 0x80 | 0x40 | 0x20));
	uae_u32 fc;
    bool write = rw ? false : true;

	if (mmu_op30_invea(opcode))
		return true;
	if (unused)
		return true;
	if (!mmu_op30_helper_get_fc(next, &fc))
		return true;

#if MMU030_OP_DBG_MSG
    write_log (_T("PLOAD%c: Create ATC entry for %08X, FC = %i\n"), write?'W':'R', extra, fc);
#endif

    mmu030_flush_atc_page(extra);
    mmu030_table_search(extra, fc, write, 0);
	return false;
}

bool mmu_op30_pflush (uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra)
{
	uae_u16 mode = (next >> 8) & 31;
    uae_u32 fc_mask = (next & 0x00E0) >> 5;
	uae_u32 fc_base;
	uae_u32 fc_bits = next & 0x7f;
    
#if MMU030_OP_DBG_MSG
    switch (mode) {
        case 0x1:
            write_log(_T("PFLUSH: Flush all entries\n"));
            break;
        case 0x4:
            write_log(_T("PFLUSH: Flush by function code only\n"));
            write_log(_T("PFLUSH: function code: base = %08X, mask = %08X\n"), fc_base, fc_mask);
            break;
        case 0x6:
            write_log(_T("PFLUSH: Flush by function code and effective address\n"));
            write_log(_T("PFLUSH: function code: base = %08X, mask = %08X\n"), fc_base, fc_mask);
            write_log(_T("PFLUSH: effective address = %08X\n"), extra);
            break;
        default:
            break;
    }
#endif
    
    switch (mode) {
		case 0x00: // PLOAD W
		case 0x02: // PLOAD R
			return mmu_op30_pload(pc, opcode, next, extra);
        case 0x04:
			if (fc_bits)
				return true;
            mmu030_flush_atc_all();
            break;
        case 0x10:
			if (!mmu_op30_helper_get_fc(next, &fc_base))
				return true;
            mmu030_flush_atc_fc(fc_base, fc_mask);
            break;
        case 0x18:
			if (mmu_op30_invea(opcode))
				return true;
			if (!mmu_op30_helper_get_fc(next, &fc_base))
				return true;
			mmu030_flush_atc_page_fc(extra, fc_base, fc_mask);
            break;
        default:
            write_log(_T("PFLUSH %04x-%04x ERROR: bad mode! (%i)\n"), opcode, next, mode);
			return true;
    }
	return false;
}


/* Transparent Translation Registers (TT0 and TT1)
 *
 * ---- ---- ---- ---- -xxx x--- x--- x---
 * reserved, must be 0
 *
 * ---- ---- ---- ---- ---- ---- ---- -xxx
 * function code mask (FC bits to be ignored)
 *
 * ---- ---- ---- ---- ---- ---- -xxx ----
 * function code base (FC value for transparent block)
 *
 * ---- ---- ---- ---- ---- ---x ---- ----
 * 0 = r/w field used, 1 = read and write is transparently translated
 *
 * ---- ---- ---- ---- ---- --x- ---- ----
 * r/w field: 0 = write ..., 1 = read access transparent
 *
 * ---- ---- ---- ---- ---- -x-- ---- ----
 * cache inhibit: 0 = caching allowed, 1 = caching inhibited
 *
 * ---- ---- ---- ---- x--- ---- ---- ----
 * 0 = transparent translation enabled disabled, 1 = enabled
 *
 * ---- ---- xxxx xxxx ---- ---- ---- ----
 * logical address mask
 *
 * xxxx xxxx ---- ---- ---- ---- ---- ----
 * logical address base
 *
 */

/* TT comparison results */
#define TT_NO_MATCH	0x1
#define TT_OK_MATCH	0x2
#define TT_NO_READ  0x4
#define TT_NO_WRITE	0x8

TT_info mmu030_decode_tt(uae_u32 TT) {
    
    TT_info ret;

    ret.fc_mask = ~((TT&TT_FC_MASK)|0xFFFFFFF8);
    ret.fc_base = (TT&TT_FC_BASE)>>4;
    ret.addr_base = TT & TT_ADDR_BASE;
    ret.addr_mask = ~(((TT&TT_ADDR_MASK)<<8)|0x00FFFFFF);
   
#if MMU030_OP_DBG_MSG
    if ((TT&TT_ENABLE) && !(TT&TT_RWM)) {
        write_log(_T("MMU Warning: Transparent translation of read-modify-write cycle is not correctly handled!\n"));
    }
#endif

#if MMU030_REG_DBG_MSG /* enable or disable debugging messages */
    write_log(_T("\n"));
    write_log(_T("TRANSPARENT TRANSLATION: %08X\n"), TT);
    write_log(_T("\n"));
    
    write_log(_T("TT: transparent translation "));
    if (TT&TT_ENABLE) {
        write_log(_T("enabled\n"));
    } else {
        write_log(_T("disabled\n"));
        return ret;
    }

    write_log(_T("TT: caching %s\n"), (TT&TT_CI) ? _T("inhibited") : _T("enabled"));
    write_log(_T("TT: read-modify-write "));
    if (TT&TT_RWM) {
        write_log(_T("enabled\n"));
    } else {
        write_log(_T("disabled (%s only)\n"), (TT&TT_RW) ? _T("read") : _T("write"));
    }
    write_log(_T("\n"));
    write_log(_T("TT: function code base: %08X\n"), ret.fc_base);
    write_log(_T("TT: function code mask: %08X\n"), ret.fc_mask);
    write_log(_T("\n"));
    write_log(_T("TT: address base: %08X\n"), ret.addr_base);
    write_log(_T("TT: address mask: %08X\n"), ret.addr_mask);
    write_log(_T("\n"));
#endif
    
    return ret;
}

/* This function checks if an address matches a transparent
 * translation register */

/* FIXME:
 * If !(tt&TT_RMW) neither the read nor the write portion
 * of a read-modify-write cycle is transparently translated! */

static int mmu030_do_match_ttr(uae_u32 tt, TT_info comp, uaecptr addr, uae_u32 fc, bool write)
{
	if (tt & TT_ENABLE)	{	/* transparent translation enabled */

        /* Compare actual function code with function code base using mask */
        if ((comp.fc_base&comp.fc_mask)==(fc&comp.fc_mask)) {

            /* Compare actual address with address base using mask */
            if ((comp.addr_base&comp.addr_mask)==(addr&comp.addr_mask)) {
                if (tt&TT_RWM) {  /* r/w field disabled */
                    return TT_OK_MATCH;
                } else {
                    if (tt&TT_RW) { /* read access transparent */
                        return write ? TT_NO_WRITE : TT_OK_MATCH;
                    } else {        /* write access transparent */
                        return write ? TT_OK_MATCH : TT_NO_READ; /* TODO: check this! */
                    }
                }
            }
		}
	}
	return TT_NO_MATCH;
}

static int mmu030_do_match_lrmw_ttr(uae_u32 tt, TT_info comp, uaecptr addr, uae_u32 fc)
{
	if ((tt & TT_ENABLE) && (tt & TT_RWM))	{	/* transparent translation enabled */

        /* Compare actual function code with function code base using mask */
        if ((comp.fc_base&comp.fc_mask)==(fc&comp.fc_mask)) {

            /* Compare actual address with address base using mask */
            if ((comp.addr_base&comp.addr_mask)==(addr&comp.addr_mask)) {
				return TT_OK_MATCH;
            }
		}
	}
	return TT_NO_MATCH;
}

/* This function compares the address with both transparent
 * translation registers and returns the result */

static int mmu030_match_ttr(uaecptr addr, uae_u32 fc, bool write)
{
    int tt0, tt1;

    tt0 = mmu030_do_match_ttr(tt0_030, mmu030.transparent.tt0, addr, fc, write);
    if (tt0&TT_OK_MATCH) {
		if (tt0_030&TT_CI)
	        mmu030_cache_state = CACHE_DISABLE_MMU;
	}
    tt1 = mmu030_do_match_ttr(tt1_030, mmu030.transparent.tt1, addr, fc, write);
    if (tt1&TT_OK_MATCH) {
		if (tt0_030&TT_CI)
	        mmu030_cache_state = CACHE_DISABLE_MMU;
    }
    
    return (tt0|tt1);
}

static int mmu030_match_ttr_access(uaecptr addr, uae_u32 fc, bool write)
{
    int tt0, tt1;
	if (!tt_enabled)
		return 0;
    tt0 = mmu030_do_match_ttr(tt0_030, mmu030.transparent.tt0, addr, fc, write);
    tt1 = mmu030_do_match_ttr(tt1_030, mmu030.transparent.tt1, addr, fc, write);
    return (tt0|tt1) & TT_OK_MATCH;
}

/* Locked Read-Modify-Write */
static int mmu030_match_lrmw_ttr_access(uaecptr addr, uae_u32 fc)
{
    int tt0, tt1;

 	if (!tt_enabled)
		return 0;
    tt0 = mmu030_do_match_lrmw_ttr(tt0_030, mmu030.transparent.tt0, addr, fc);
    tt1 = mmu030_do_match_lrmw_ttr(tt1_030, mmu030.transparent.tt1, addr, fc);
    return (tt0|tt1) & TT_OK_MATCH;
}

/* Translation Control Register:
 *
 * x--- ---- ---- ---- ---- ---- ---- ----
 * translation: 1 = enable, 0 = disable
 *
 * ---- --x- ---- ---- ---- ---- ---- ----
 * supervisor root: 1 = enable, 0 = disable
 *
 * ---- ---x ---- ---- ---- ---- ---- ----
 * function code lookup: 1 = enable, 0 = disable
 *
 * ---- ---- xxxx ---- ---- ---- ---- ----
 * page size:
 * 1000 = 256 bytes
 * 1001 = 512 bytes
 * 1010 =  1 kB
 * 1011 =  2 kB
 * 1100 =  4 kB
 * 1101 =  8 kB
 * 1110 = 16 kB
 * 1111 = 32 kB
 *
 * ---- ---- ---- xxxx ---- ---- ---- ----
 * initial shift
 *
 * ---- ---- ---- ---- xxxx ---- ---- ----
 * number of bits for table index A
 *
 * ---- ---- ---- ---- ---- xxxx ---- ----
 * number of bits for table index B
 *
 * ---- ---- ---- ---- ---- ---- xxxx ----
 * number of bits for table index C
 *
 * ---- ---- ---- ---- ---- ----- ---- xxxx
 * number of bits for table index D
 *
 */


#define TC_ENABLE_TRANSLATION   0x80000000
#define TC_ENABLE_SUPERVISOR    0x02000000
#define TC_ENABLE_FCL           0x01000000

#define TC_PS_MASK              0x00F00000
#define TC_IS_MASK              0x000F0000

#define TC_TIA_MASK             0x0000F000
#define TC_TIB_MASK             0x00000F00
#define TC_TIC_MASK             0x000000F0
#define TC_TID_MASK             0x0000000F

static void mmu030_do_fake_prefetch(void)
{
	if (currprefs.cpu_compatible)
		return;
	// fetch next opcode before MMU state switches.
	// There are programs that do following:
	// - enable MMU
	// - JMP (An)
	// "enable MMU" unmaps memory under us.
	TRY (prb) {
		uaecptr pc = m68k_getpci();
		mmu030_fake_prefetch = -1;
		mmu030_fake_prefetch_addr = mmu030_translate(pc, regs.s != 0, false, false);
		mmu030_fake_prefetch = x_prefetch(0);
		// A26x0 ROM code switches off rom
		// NOP
		// JMP (a0)
		if (mmu030_fake_prefetch == 0x4e71)
			mmu030_fake_prefetch = x_prefetch(2);
	} CATCH (prb) {
		// didn't work, oh well..
		mmu030_fake_prefetch = -1;
	} ENDTRY
}

bool mmu030_decode_tc(uae_u32 TC, bool check)
{
#if MMU_IPAGECACHE030
	mmu030.mmu030_last_logical_address = 0xffffffff;
#endif

	if (currprefs.mmu_ec)
		TC &= ~TC_ENABLE_TRANSLATION;
    /* Set MMU condition */    
    if (TC & TC_ENABLE_TRANSLATION) {
		if (!mmu030.enabled && check)
			mmu030_do_fake_prefetch();
        mmu030.enabled = true;
    } else {
		if (mmu030.enabled) {
			mmu030_do_fake_prefetch();
			write_log(_T("MMU disabled PC=%08x\n"), M68K_GETPC);
		}
        mmu030.enabled = false;
        return false;
    }
    
    /* Note: 0 = Table A, 1 = Table B, 2 = Table C, 3 = Table D */
    int i, j;
    uae_u8 TI_bits[4] = {0,0,0,0};

    /* Reset variables before extracting new values from TC */
    for (i = 0; i < 4; i++) {
        mmu030.translation.table[i].mask = 0;
        mmu030.translation.table[i].shift = 0;
    }
    
    
    /* Extract initial shift and page size values from TC register */
    mmu030.translation.page.size = (TC & TC_PS_MASK) >> 20;
    mmu030.translation.page.size3m =  mmu030.translation.page.size - 3;
    mmu030.translation.init_shift = (TC & TC_IS_MASK) >> 16;
	regs.mmu_page_size = 1 << mmu030.translation.page.size;

	write_log(_T("68030 MMU enabled. Page size = %d PC=%08x\n"), regs.mmu_page_size, M68K_GETPC);

	if (mmu030.translation.page.size<8) {
        write_log(_T("MMU Configuration Exception: Bad value in TC register! (bad page size: %i byte)\n"),
                  1<<mmu030.translation.page.size);
        Exception(56); /* MMU Configuration Exception */
        return true;
    }
	mmu030.translation.page.mask = regs.mmu_page_size - 1;
	mmu030.translation.page.imask = ~mmu030.translation.page.mask;
    
    /* Calculate masks and shifts for later extracting table indices
     * from logical addresses using: index = (addr&mask)>>shift */
    
    /* Get number of bits for each table index */
    for (i = 0; i < 4; i++) {
        j = (3-i)*4;
        TI_bits[i] = (TC >> j) & 0xF;
    }

    /* Calculate masks and shifts for each table */
    mmu030.translation.last_table = 0;
    uae_u8 shift = 32 - mmu030.translation.init_shift;
    for (i = 0; (i < 4) && TI_bits[i]; i++) {
        /* Get the shift */
        shift -= TI_bits[i];
        mmu030.translation.table[i].shift = shift;
        /* Build the mask */
        for (j = 0; j < TI_bits[i]; j++) {
            mmu030.translation.table[i].mask |= (1<<(mmu030.translation.table[i].shift + j));
        }
        /* Update until reaching the last table */
        mmu030.translation.last_table = i;
    }
    
#if MMU030_REG_DBG_MSG
    /* At least one table has to be defined using at least
     * 1 bit for the index. At least 2 bits are necessary 
     * if there is no second table. If these conditions are
     * not met, it will automatically lead to a sum <32
     * and cause an exception (see below). */
    if (!TI_bits[0]) {
        write_log(_T("MMU Configuration Exception: Bad value in TC register! (no first table index defined)\n"));
    } else if ((TI_bits[0]<2) && !TI_bits[1]) {
        write_log(_T("MMU Configuration Exception: Bad value in TC register! (no second table index defined and)\n"));
        write_log(_T("MMU Configuration Exception: Bad value in TC register! (only 1 bit for first table index)\n"));
    }
#endif
    
    /* TI fields are summed up until a zero field is reached (see above
     * loop). The sum of all TI field values plus page size and initial
     * shift has to be 32: IS + PS + TIA + TIB + TIC + TID = 32 */
    if ((shift-mmu030.translation.page.size)!=0) {
        write_log(_T("MMU Configuration Exception: Bad value in TC register! (bad sum)\n"));
        Exception(56); /* MMU Configuration Exception */
        return true;
    }
    
#if MMU030_REG_DBG_MSG /* enable or disable debugging output */
    write_log(_T("\n"));
    write_log(_T("TRANSLATION CONTROL: %08X\n"), TC);
    write_log(_T("\n"));
    write_log(_T("TC: translation %s\n"), (TC&TC_ENABLE_TRANSLATION ? _T("enabled") : _T("disabled")));
    write_log(_T("TC: supervisor root pointer %s\n"), (TC&TC_ENABLE_SUPERVISOR ? _T("enabled") : _T("disabled")));
    write_log(_T("TC: function code lookup %s\n"), (TC&TC_ENABLE_FCL ? _T("enabled") : _T("disabled")));
    write_log(_T("\n"));
    
    write_log(_T("TC: Initial Shift: %i\n"), mmu030.translation.init_shift);
    write_log(_T("TC: Page Size: %i byte\n"), (1<<mmu030.translation.page.size));
    write_log(_T("\n"));
    
    for (i = 0; i <= mmu030.translation.last_table; i++) {
        write_log(_T("TC: Table %c: mask = %08X, shift = %i\n"), table_letter[i], mmu030.translation.table[i].mask, mmu030.translation.table[i].shift);
    }

    write_log(_T("TC: Page:    mask = %08X\n"), mmu030.translation.page.mask);
    write_log(_T("\n"));

    write_log(_T("TC: Last Table: %c\n"), table_letter[mmu030.translation.last_table]);
    write_log(_T("\n"));
#endif
	return false;
}



/* Root Pointer Registers (SRP and CRP)
 *
 * ---- ---- ---- ---- xxxx xxxx xxxx xx-- ---- ---- ---- ---- ---- ---- ---- xxxx
 * reserved, must be 0
 *
 * ---- ---- ---- ---- ---- ---- ---- ---- xxxx xxxx xxxx xxxx xxxx xxxx xxxx ----
 * table A address
 *
 * ---- ---- ---- ---- ---- ---- ---- --xx ---- ---- ---- ---- ---- ---- ---- ----
 * descriptor type
 *
 * -xxx xxxx xxxx xxxx ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
 * limit
 *
 * x--- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
 * 0 = upper limit, 1 = lower limit
 *
 */


#define RP_ADDR_MASK    (UVAL64(0x00000000FFFFFFF0))
#define RP_DESCR_MASK   (UVAL64(0x0000000300000000))
#define RP_LIMIT_MASK   (UVAL64(0x7FFF000000000000))
#define RP_LOWER_MASK   (UVAL64(0x8000000000000000))

#define RP_ZERO_BITS 0x0000FFFC /* These bits in upper longword of RP must be 0 */

bool mmu030_decode_rp(uae_u64 RP) {
    
    uae_u8 descriptor_type = (uae_u8)((RP & RP_DESCR_MASK) >> 32);
    if (!descriptor_type) { /* If descriptor type is invalid */
        write_log(_T("MMU Configuration Exception: Root Pointer is invalid!\n"));
        Exception(56); /* MMU Configuration Exception */
		return true;
    }
	return false;

#if MMU030_REG_DBG_MSG /* enable or disable debugging output */
    uae_u32 table_limit = (RP & RP_LIMIT_MASK) >> 48;
    uae_u32 first_addr = (RP & RP_ADDR_MASK);
    
    write_log(_T("\n"));
    write_log(_T("ROOT POINTER: %08X%08X\n"), (uae_u32)(RP>>32)&0xFFFFFFFF, (uae_u32)(RP&0xFFFFFFFF));
    write_log(_T("\n"));
    
    write_log(_T("RP: descriptor type = %i "), descriptor_type);
    switch (descriptor_type) {
        case 0:
            write_log(_T("(invalid descriptor)\n"));
            break;
        case 1:
            write_log(_T("(early termination page descriptor)\n"));
            break;
        case 2:
            write_log(_T("(valid 4 byte descriptor)\n"));
            break;
        case 3:
            write_log(_T("(valid 8 byte descriptor)\n"));
            break;
    }
    
    write_log(_T("RP: %s limit = %i\n"), (RP&RP_LOWER_MASK) ? _T("lower") : _T("upper"), table_limit);
    
    write_log(_T("RP: first table address = %08X\n"), first_addr);
    write_log(_T("\n"));
#endif
}

static void mmu030_atc_handle_history_bit(int entry_num) {
    int j;
    mmu030.atc[entry_num].mru = 1;
    for (j=0; j<ATC030_NUM_ENTRIES; j++) {
        if (!mmu030.atc[j].mru)
            break;
    }
    /* If there are no more zero-bits, reset all */
    if (j==ATC030_NUM_ENTRIES) {
        for (j=0; j<ATC030_NUM_ENTRIES; j++) {
            mmu030.atc[j].mru = 0;
        }
        mmu030.atc[entry_num].mru = 1;
#if MMU030_ATC_DBG_MSG > 1
        write_log(_T("ATC: No more history zero-bits. Reset all.\n"));
#endif
	}
}

static void desc_put_long(uaecptr addr, uae_u32 v)
{
	x_phys_put_long(addr, v);
}
static uae_u32 desc_get_long(uaecptr addr)
{
	return x_phys_get_long(addr);
}
static void desc_get_quad(uaecptr addr, uae_u32 *descr)
{
	descr[0] = x_phys_get_long(addr);
	descr[1] = x_phys_get_long(addr + 4);
}

/* Descriptors */

#define DESCR_TYPE_MASK         0x00000003

#define DESCR_TYPE_INVALID      0 /* all tables */

#define DESCR_TYPE_EARLY_TERM   1 /* all but lowest level table */
#define DESCR_TYPE_PAGE         1 /* only lowest level table */
#define DESCR_TYPE_VALID4       2 /* all but lowest level table */
#define DESCR_TYPE_INDIRECT4    2 /* only lowest level table */
#define DESCR_TYPE_VALID8       3 /* all but lowest level table */
#define DESCR_TYPE_INDIRECT8    3 /* only lowest level table */

#define DESCR_TYPE_VALID_MASK       0x2 /* all but lowest level table */
#define DESCR_TYPE_INDIRECT_MASK    0x2 /* only lowest level table */


/* Short format (4 byte):
 *
 * ---- ---- ---- ---- ---- ---- ---- --xx
 * descriptor type:
 * 0 = invalid
 * 1 = page descriptor (early termination)
 * 2 = valid (4 byte)
 * 3 = valid (8 byte)
 *
 *
 * table descriptor:
 * ---- ---- ---- ---- ---- ---- ---- -x--
 * write protect
 *
 * ---- ---- ---- ---- ---- ---- ---- x---
 * update
 *
 * xxxx xxxx xxxx xxxx xxxx xxxx xxxx ----
 * table address
 *
 *
 * (early termination) page descriptor:
 * ---- ---- ---- ---- ---- ---- ---- -x--
 * write protect
 *
 * ---- ---- ---- ---- ---- ---- ---- x---
 * update
 *
 * ---- ---- ---- ---- ---- ---- ---x ----
 * modified
 *
 * ---- ---- ---- ---- ---- ---- -x-- ----
 * cache inhibit
 *
 * ---- ---- ---- ---- ---- ---- x-x- ----
 * reserved (must be 0)
 *
 * xxxx xxxx xxxx xxxx xxxx xxxx ---- ----
 * page address
 *
 *
 * indirect descriptor:
 * xxxx xxxx xxxx xxxx xxxx xxxx xxxx xx--
 * descriptor address
 *
 */

#define DESCR_WP       0x00000004
#define DESCR_U        0x00000008
#define DESCR_M        0x00000010 /* only last level table */
#define DESCR_CI       0x00000040 /* only last level table */

#define DESCR_TD_ADDR_MASK 0xFFFFFFF0
#define DESCR_PD_ADDR_MASK 0xFFFFFF00
#define DESCR_ID_ADDR_MASK 0xFFFFFFFC


/* Long format (8 byte):
 *
 * ---- ---- ---- ---- ---- ---- ---- --xx | ---- ---- ---- ---- ---- ---- ---- ----
 * descriptor type:
 * 0 = invalid
 * 1 = page descriptor (early termination)
 * 2 = valid (4 byte)
 * 3 = valid (8 byte)
 *
 *
 * table desctriptor:
 * ---- ---- ---- ---- ---- ---- ---- -x-- | ---- ---- ---- ---- ---- ---- ---- ----
 * write protect
 *
 * ---- ---- ---- ---- ---- ---- ---- x--- | ---- ---- ---- ---- ---- ---- ---- ----
 * update
 *
 * ---- ---- ---- ---- ---- ---- xxxx ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * reserved (must be 0)
 *
 * ---- ---- ---- ---- ---- ---x ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * supervisor
 *
 * ---- ---- ---- ---- xxxx xxx- ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * reserved (must be 1111 110)
 *
 * -xxx xxxx xxxx xxxx ---- ---- ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * limit
 *
 * x--- ---- ---- ---- ---- ---- ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * 0 = upper limit, 1 = lower limit
 *
 * ---- ---- ---- ---- ---- ---- ---- ---- | xxxx xxxx xxxx xxxx xxxx xxxx xxxx ----
 * table address
 *
 *
 * (early termination) page descriptor:
 * ---- ---- ---- ---- ---- ---- ---- -x-- | ---- ---- ---- ---- ---- ---- ---- ----
 * write protect
 *
 * ---- ---- ---- ---- ---- ---- ---- x--- | ---- ---- ---- ---- ---- ---- ---- ----
 * update
 *
 * ---- ---- ---- ---- ---- ---- ---x ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * modified
 *
 * ---- ---- ---- ---- ---- ---- -x-- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * cache inhibit
 *
 * ---- ---- ---- ---- ---- ---x ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * supervisor
 *
 * ---- ---- ---- ---- ---- ---- x-x- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * reserved (must be 0)
 *
 * ---- ---- ---- ---- xxxx xxx- ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * reserved (must be 1111 110)
 *
 * -xxx xxxx xxxx xxxx ---- ---- ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * limit (only used with early termination page descriptor)
 *
 * x--- ---- ---- ---- ---- ---- ---- ---- | ---- ---- ---- ---- ---- ---- ---- ----
 * 0 = upper limit, 1 = lower limit (only used with early termination page descriptor)
 *
 * ---- ---- ---- ---- ---- ---- ---- ---- | xxxx xxxx xxxx xxxx xxxx xxxx ---- ----
 * page address
 *
 *
 * indirect descriptor:
 * ---- ---- ---- ---- ---- ---- ---- ---- | xxxx xxxx xxxx xxxx xxxx xxxx xxxx xx--
 * descriptor address
 *
 */

/* only for long descriptors */
#define DESCR_S        0x00000100

#define DESCR_LIMIT_MASK   0x7FFF0000
#define DESCR_LOWER_MASK   0x80000000



/* This functions searches through the translation tables. It can be used 
 * for PTEST (levels 1 to 7). Using level 0 creates an ATC entry. */

static uae_u32 mmu030_table_search(uaecptr addr, uae_u32 fc, bool write, int level) {
    /* During table walk up to 7 different descriptors are used:
     * root pointer, descriptors fetched from function code lookup table,
     * tables A, B, C and D and one indirect descriptor */
    uae_u32 descr[2];
    uae_u32 descr_type;
    uaecptr descr_addr[7];
    uaecptr table_addr = 0;
    uaecptr page_addr = 0;
    uaecptr indirect_addr = 0;
    uae_u32 table_index = 0;
    uae_u32 limit = 0;
    uae_u32 unused_fields_mask = 0;
    bool super = (fc&4) ? true : false;
    bool super_violation = false;
    bool write_protected = false;
    uae_u8 cache_inhibit = CACHE_ENABLE_ALL;
    bool descr_modified = false;
        
    mmu030.status = 0; /* Reset status */
        
    /* Initial values for condition variables.
     * Note: Root pointer is long descriptor. */
    int t = 0;
    int addr_position = 1;
    int next_size = 0;
    int descr_size = 8;
    int descr_num = 0;
    bool early_termination = false;
    int old_s;
    int i;
    
	// Always use supervisor mode to access descriptors
    old_s = regs.s;
    regs.s = 1;

	TRY(prb) {
        /* Use super user root pointer if enabled in TC register and access is in
         * super user mode, else use cpu root pointer. */
        if ((tc_030&TC_ENABLE_SUPERVISOR) && super) {
            descr[0] = (srp_030>>32)&0xFFFFFFFF;
            descr[1] = srp_030&0xFFFFFFFF;
#if MMU030_REG_DBG_MSG > 2
            write_log(_T("Supervisor Root Pointer: %08X%08X\n"),descr[0],descr[1]);
#endif // MMU030_REG_DBG_MSG
        } else {
            descr[0] = (crp_030>>32)&0xFFFFFFFF;
            descr[1] = crp_030&0xFFFFFFFF;
#if MMU030_REG_DBG_MSG > 2
            write_log(_T("CPU Root Pointer: %08X%08X\n"),descr[0],descr[1]);
#endif
		}
                
        if (descr[0]&RP_ZERO_BITS) {
#if MMU030_REG_DBG_MSG
			write_log(_T("MMU Warning: Root pointer reserved bits are non-zero! %08X\n"), descr[0]);
#endif
			descr[0] &= (~RP_ZERO_BITS);
        }
        
        /* Check descriptor type of root pointer */
        descr_type = descr[0]&DESCR_TYPE_MASK;
        switch (descr_type) {
            case DESCR_TYPE_INVALID:
                write_log(_T("Fatal error: Root pointer is invalid descriptor!\n"));
                mmu030.status |= MMUSR_INVALID;
                goto stop_search;
            case DESCR_TYPE_EARLY_TERM:
                write_log(_T("Root pointer is early termination page descriptor.\n"));
                early_termination = true;
                goto handle_page_descriptor;
            case DESCR_TYPE_VALID4:
                next_size = 4;
                break;
            case DESCR_TYPE_VALID8:
                next_size = 8;
                break;
        }
        
        /* If function code lookup is enabled in TC register use function code as
         * index for top level table, limit check not required */
        
        if (tc_030&TC_ENABLE_FCL) {
            write_log(_T("Function code lookup enabled, FC = %i\n"), fc);
            
            addr_position = (descr_size==4) ? 0 : 1;
            table_addr = descr[addr_position]&DESCR_TD_ADDR_MASK;
            table_index = fc; /* table index is function code */
            write_log(_T("Table FCL at %08X: index = %i, "),table_addr,table_index);
            
            /* Fetch next descriptor */
            descr_num++;
            descr_addr[descr_num] = table_addr+(table_index*next_size);
            
            if (next_size==4) {
                descr[0] = desc_get_long(descr_addr[descr_num]);
#if MMU030_REG_DBG_MSG > 2
                write_log(_T("Next descriptor: %08X\n"),descr[0]);
#endif
            } else {
				desc_get_quad(descr_addr[descr_num], descr);
#if MMU030_REG_DBG_MSG > 2
                write_log(_T("Next descriptor: %08X%08X\n"),descr[0],descr[1]);
#endif
            }
            
            descr_size = next_size;
            
            /* Check descriptor type */
            descr_type = descr[0]&DESCR_TYPE_MASK;
            switch (descr_type) {
                case DESCR_TYPE_INVALID:
                    write_log(_T("Invalid descriptor!\n"));
                    /* stop table walk */
                    mmu030.status |= MMUSR_INVALID;
                    goto stop_search;
                case DESCR_TYPE_EARLY_TERM:
#if MMU030_REG_DBG_MSG > 2
                    write_log(_T("Early termination page descriptor!\n"));
#endif
					early_termination = true;
                    goto handle_page_descriptor;
                case DESCR_TYPE_VALID4:
                    next_size = 4;
                    break;
                case DESCR_TYPE_VALID8:
                    next_size = 8;
                    break;
            }
        }
        
        
        /* Upper level tables */
        do {
            if (descr_num) { /* if not root pointer */
                /* Check protection */
                if ((descr_size==8) && (descr[0]&DESCR_S) && !super) {
                    super_violation = true;
                }
                if (descr[0]&DESCR_WP) {
                    write_protected = true;
                }
                
                /* Set the updated bit */
                if (!level && !(descr[0]&DESCR_U) && !super_violation) {
                    descr[0] |= DESCR_U;
                    desc_put_long(descr_addr[descr_num], descr[0]);
                }
                
                /* Update status bits */
                mmu030.status |= super_violation ? MMUSR_SUPER_VIOLATION : 0;
                mmu030.status |= write_protected ? MMUSR_WRITE_PROTECTED : 0;                

                /* Check if ptest level is reached */
                if (level && (level==descr_num)) {
                    goto stop_search;
                }
            }
            
            addr_position = (descr_size==4) ? 0 : 1;
            table_addr = descr[addr_position]&DESCR_TD_ADDR_MASK;
            table_index = (addr&mmu030.translation.table[t].mask)>>mmu030.translation.table[t].shift;
#if MMU030_REG_DBG_MSG > 2
            write_log(_T("Table %c at %08X: index = %i, "),table_letter[t],table_addr,table_index);
#endif // MMU030_REG_DBG_MSG
            t++; /* Proceed to the next table */
            
            /* Perform limit check */
            if (descr_size==8) {
                limit = (descr[0]&DESCR_LIMIT_MASK)>>16;
                if ((descr[0]&DESCR_LOWER_MASK) && (table_index<limit)) {
                    mmu030.status |= (MMUSR_LIMIT_VIOLATION|MMUSR_INVALID);
#if MMU030_REG_DBG_MSG
                    write_log(_T("limit violation (lower limit %i)\n"),limit);
#endif
                    goto stop_search;
                }
                if (!(descr[0]&DESCR_LOWER_MASK) && (table_index>limit)) {
                    mmu030.status |= (MMUSR_LIMIT_VIOLATION|MMUSR_INVALID);
#if MMU030_REG_DBG_MSG
                    write_log(_T("limit violation (upper limit %i)\n"),limit);
#endif
                    goto stop_search;
                }
            }
            
            /* Fetch next descriptor */
            descr_num++;
            descr_addr[descr_num] = table_addr+(table_index*next_size);
            
            if (next_size==4) {
                descr[0] = desc_get_long(descr_addr[descr_num]);
#if MMU030_REG_DBG_MSG > 2
                write_log(_T("Next descriptor: %08X\n"),descr[0]);
#endif
            } else {
				desc_get_quad(descr_addr[descr_num], descr);
#if MMU030_REG_DBG_MSG > 2
                write_log(_T("Next descriptor: %08X%08X\n"),descr[0],descr[1]);
#endif
            }
            
            descr_size = next_size;
            
            /* Check descriptor type */
            descr_type = descr[0]&DESCR_TYPE_MASK;
            switch (descr_type) {
                case DESCR_TYPE_INVALID:
#if MMU030_REG_DBG_MSG
                    write_log(_T("Invalid descriptor!\n"));
#endif
					/* stop table walk */
                    mmu030.status |= MMUSR_INVALID;
                    goto stop_search;
                case DESCR_TYPE_EARLY_TERM:
                    /* go to last level table handling code */
                    if (t<=mmu030.translation.last_table) {
#if MMU030_REG_DBG_MSG > 2
                        write_log(_T("Early termination page descriptor!\n"));
#endif
						early_termination = true;
                    }
                    goto handle_page_descriptor;
                case DESCR_TYPE_VALID4:
                    next_size = 4;
                    break;
                case DESCR_TYPE_VALID8:
                    next_size = 8;
                    break;
            }
        } while (t<=mmu030.translation.last_table);
        
        
        /* Handle indirect descriptor */
        
        /* Check if ptest level is reached */
        if (level && (level==descr_num)) {
            goto stop_search;
        }
        
        addr_position = (descr_size==4) ? 0 : 1;
        indirect_addr = descr[addr_position]&DESCR_ID_ADDR_MASK;
#if MMU030_REG_DBG_MSG > 2
        write_log(_T("Page indirect descriptor at %08X: "),indirect_addr);
#endif

        /* Fetch indirect descriptor */
        descr_num++;
        descr_addr[descr_num] = indirect_addr;
        
        if (next_size==4) {
            descr[0] = desc_get_long(descr_addr[descr_num]);
#if MMU030_REG_DBG_MSG > 2
            write_log(_T("descr = %08X\n"),descr[0]);
#endif
		} else {
			desc_get_quad(descr_addr[descr_num], descr);
#if MMU030_REG_DBG_MSG > 2
            write_log(_T("descr = %08X%08X"),descr[0],descr[1]);
#endif
		}
        
        descr_size = next_size;
        
        /* Check descriptor type, only page descriptor is valid */
        descr_type = descr[0]&DESCR_TYPE_MASK;
        if (descr_type!=DESCR_TYPE_PAGE) {
            mmu030.status |= MMUSR_INVALID;
            goto stop_search;
        }
        
    handle_page_descriptor:
        
        if (descr_num) { /* if not root pointer */
            /* check protection */
            if ((descr_size==8) && (descr[0]&DESCR_S) && !super) {
                super_violation = true;
            }
            if (descr[0]&DESCR_WP) {
                write_protected = true;
            }

            if (!level && !super_violation) {
                /* set modified bit */
                if (!(descr[0]&DESCR_M) && write && !write_protected) {
                    descr[0] |= DESCR_M;
                    descr_modified = true;
                }
                /* set updated bit */
                if (!(descr[0]&DESCR_U)) {
                    descr[0] |= DESCR_U;
                    descr_modified = true;
                }
                /* write modified descriptor if necessary */
                if (descr_modified) {
                    desc_put_long(descr_addr[descr_num], descr[0]);
                }
            }
            
            /* update status bits */
            mmu030.status |= super_violation ? MMUSR_SUPER_VIOLATION : 0;
            mmu030.status |= write_protected ? MMUSR_WRITE_PROTECTED : 0;

            /* check if caching is inhibited */
            cache_inhibit = (descr[0]&DESCR_CI) ? CACHE_DISABLE_MMU : CACHE_ENABLE_ALL;
            
            /* check for the modified bit and set it in the status register */
            mmu030.status |= (descr[0]&DESCR_M) ? MMUSR_MODIFIED : 0;
        }
        
        /* Check limit using next index field of logical address.
         * Limit is only checked on early termination. If we are
         * still at root pointer level, only check limit, if FCL
         * is disabled. */
        if (early_termination) {
            if (descr_num || !(tc_030&TC_ENABLE_FCL)) {
                if (descr_size==8) {
                    table_index = (addr&mmu030.translation.table[t].mask)>>mmu030.translation.table[t].shift;
                    limit = (descr[0]&DESCR_LIMIT_MASK)>>16;
                    if ((descr[0]&DESCR_LOWER_MASK) && (table_index<limit)) {
                        mmu030.status |= (MMUSR_LIMIT_VIOLATION|MMUSR_INVALID);
#if MMU030_REG_DBG_MSG
                        write_log(_T("Limit violation (lower limit %i)\n"),limit);
#endif
                        goto stop_search;
                    }
                    if (!(descr[0]&DESCR_LOWER_MASK) && (table_index>limit)) {
                        mmu030.status |= (MMUSR_LIMIT_VIOLATION|MMUSR_INVALID);
#if MMU030_REG_DBG_MSG
                        write_log(_T("Limit violation (upper limit %i)\n"),limit);
#endif
                        goto stop_search;
                    }
                }
            }
            /* Get all unused bits of the logical address table index field.
             * they are added to the page address */
            /* TODO: They should be added via "unsigned addition". How to? */
            do {
                unused_fields_mask |= mmu030.translation.table[t].mask;
                t++;
            } while (t<=mmu030.translation.last_table);
            page_addr = addr&unused_fields_mask;
#if MMU030_REG_DBG_MSG > 1
            write_log(_T("Logical address unused bits: %08X (mask = %08X)\n"),
                      page_addr,unused_fields_mask);
#endif
		}

        /* Get page address */
        addr_position = (descr_size==4) ? 0 : 1;
        page_addr += (descr[addr_position]&DESCR_PD_ADDR_MASK);
#if MMU030_REG_DBG_MSG > 2
        write_log(_T("Page at %08X\n"),page_addr);
#endif // MMU030_REG_DBG_MSG
        
    stop_search:
        ; /* Make compiler happy */
    } CATCH(prb) {
        /* We jump to this place, if a bus error occurred during table search.
         * bBusErrorReadWrite is set in m68000.c, M68000_BusError: read = 1 */
        if (bBusErrorReadWrite) {
            descr_num--;
        }
        mmu030.status |= (MMUSR_BUS_ERROR|MMUSR_INVALID);
        write_log(_T("MMU: Bus error while %s descriptor!\n"),
                  bBusErrorReadWrite?_T("reading"):_T("writing"));
    } ENDTRY;

    // Restore original supervisor state
    regs.s = old_s;

    /* check if we have to handle ptest */
    if (level) {
        /* Note: wp, m and sv bits are undefined if the invalid bit is set */
        mmu030.status = (mmu030.status&~MMUSR_NUM_LEVELS_MASK) | descr_num;

        /* If root pointer is page descriptor (descr_num 0), return 0 */
        return descr_num ? descr_addr[descr_num] : 0;
    }
    
    /* Find an ATC entry to replace */
    /* Search for invalid entry */
    for (i=0; i<ATC030_NUM_ENTRIES; i++) {
        if (!mmu030.atc[i].logical.valid) {
            break;
        }
    }
    /* If there are no invalid entries, replace first entry
     * with history bit not set */
    if (i == ATC030_NUM_ENTRIES) {
        for (i=0; i<ATC030_NUM_ENTRIES; i++) {
            if (!mmu030.atc[i].mru) {
                break;
            }
        }
#if MMU030_REG_DBG_MSG > 2
        write_log(_T("ATC is full. Replacing entry %i\n"), i);
#endif
	}
	if (i >= ATC030_NUM_ENTRIES) {
		i = 0;
		write_log (_T("ATC entry not found!!!\n"));
	}

    mmu030_atc_handle_history_bit(i);
    
    /* Create ATC entry */
    mmu030.atc[i].logical.addr = addr & mmu030.translation.page.imask; /* delete page index bits */
    mmu030.atc[i].logical.fc = fc;
    mmu030.atc[i].logical.valid = true;
    mmu030.atc[i].physical.addr = page_addr & mmu030.translation.page.imask; /* delete page index bits */
    if ((mmu030.status&MMUSR_INVALID) || (mmu030.status&MMUSR_SUPER_VIOLATION)) {
        mmu030.atc[i].physical.bus_error = true;
    } else {
        mmu030.atc[i].physical.bus_error = false;
    }
    mmu030.atc[i].physical.cache_inhibit = cache_inhibit;
    mmu030.atc[i].physical.modified = (mmu030.status&MMUSR_MODIFIED) ? true : false;
    mmu030.atc[i].physical.write_protect = (mmu030.status&MMUSR_WRITE_PROTECTED) ? true : false;

	mmu030_flush_cache(mmu030.atc[i].logical.addr);

#if MMU030_ATC_DBG_MSG > 1
    write_log(_T("ATC create entry(%i): logical = %08X, physical = %08X, FC = %i\n"), i,
              mmu030.atc[i].logical.addr, mmu030.atc[i].physical.addr,
              mmu030.atc[i].logical.fc);
    write_log(_T("ATC create entry(%i): B = %i, CI = %i, WP = %i, M = %i\n"), i,
              mmu030.atc[i].physical.bus_error?1:0,
              mmu030.atc[i].physical.cache_inhibit?1:0,
              mmu030.atc[i].physical.write_protect?1:0,
              mmu030.atc[i].physical.modified?1:0);
#endif // MMU030_ATC_DBG_MSG
    
    return 0;
}

/* This function is used for PTEST level 0. */
static void mmu030_ptest_atc_search(uaecptr logical_addr, uae_u32 fc, bool write) {
    int i;
    mmu030.status = 0;
        
    if (mmu030_match_ttr(logical_addr, fc, write)&TT_OK_MATCH) {
        mmu030.status |= MMUSR_TRANSP_ACCESS;
        return;
    }
    
    for (i = 0; i < ATC030_NUM_ENTRIES; i++) {
        if ((mmu030.atc[i].logical.fc == fc) &&
            (mmu030.atc[i].logical.addr == logical_addr) &&
            mmu030.atc[i].logical.valid) {
            break;
        }
    }
    
    if (i==ATC030_NUM_ENTRIES) {
        mmu030.status |= MMUSR_INVALID;
        return;
    }
    
    mmu030.status |= mmu030.atc[i].physical.bus_error ? (MMUSR_BUS_ERROR|MMUSR_INVALID) : 0;
    /* Note: write protect and modified bits are undefined if the invalid bit is set */
    mmu030.status |= mmu030.atc[i].physical.write_protect ? MMUSR_WRITE_PROTECTED : 0;
    mmu030.status |= mmu030.atc[i].physical.modified ? MMUSR_MODIFIED : 0;
}

/* Address Translation Cache
 *
 * The ATC uses a pseudo-least-recently-used algorithm to keep track of
 * least recently used entries. They are replaced if the cache is full.
 * An internal history-bit (MRU-bit) is used to identify these entries.
 * If an entry is accessed, its history-bit is set to 1. If after that
 * there are no more entries with zero-bits, all other history-bits are
 * set to 0. When no more invalid entries are in the ATC, the first entry
 * with a zero-bit is replaced.
 *
 *
 * Logical Portion (28 bit):
 * oooo ---- xxxx xxxx xxxx xxxx xxxx xxxx
 * logical address (most significant 24 bit)
 *
 * oooo -xxx ---- ---- ---- ---- ---- ----
 * function code
 *
 * oooo x--- ---- ---- ---- ---- ---- ----
 * valid
 *
 *
 * Physical Portion (28 bit):
 * oooo ---- xxxx xxxx xxxx xxxx xxxx xxxx
 * physical address
 *
 * oooo ---x ---- ---- ---- ---- ---- ----
 * modified
 *
 * oooo --x- ---- ---- ---- ---- ---- ----
 * write protect
 *
 * oooo -x-- ---- ---- ---- ---- ---- ----
 * cache inhibit
 *
 * oooo x--- ---- ---- ---- ---- ---- ----
 * bus error
 *
 */

#define ATC030_MASK         0x0FFFFFFF
#define ATC030_ADDR_MASK    0x00FFFFFF /* after masking shift 8 (<< 8) */

#define ATC030_LOG_FC   0x07000000
#define ATC030_LOG_V    0x08000000

#define ATC030_PHYS_M   0x01000000
#define ATC030_PHYS_WP  0x02000000
#define ATC030_PHYS_CI  0x04000000
#define ATC030_PHYS_BE  0x08000000

#if MMUDEBUG
static void dump_opcode(uae_u16 opcode)
{
	struct mnemolookup *lookup;
	struct instr *dp;
	char size = '_';

	dp = table68k + opcode;
	if (dp->mnemo == i_ILLG) {
		dp = table68k + 0x4AFC;
	}
	for (lookup = lookuptab; lookup->mnemo != dp->mnemo; lookup++);

	if (!dp->unsized) {
		switch (dp->size)
		{
		case sz_byte:
		size = 'B';
		break;
		case sz_word:
		size = 'W';
		break;
		case sz_long:
		size = 'L';
		break;
		}
	}
	write_log(_T(" %04x %s.%c"), opcode, lookup->name, size);
}
#endif

static uae_u8 mmu030fixupreg(int i)
{
	uae_u8 v = 0;
#if MMU030_REG_FIXUP
	struct mmufixup *m = &mmufixup[i];
	if (m->reg < 0)
		return v;
	if (!(m->reg & 0x300))
		return v;
	v = m->reg & 7;
	v |= ((m->reg >> 10) & 3) << 3; // size
	if (m->reg & 0x200) // -(an)?
		v |= 1 << 5;
	v |= 1 << 6;
#endif
	return v;
}

static void mmu030fixupmod(uae_u8 data, int dir, int idx)
{
#if MMU030_REG_FIXUP
	if (!data)
		return;
	int reg = data & 7;
	int adj = (data & (1 << 5)) ? -1 : 1;
	if (dir)
		adj = -adj;
	adj <<= (data >> 3) & 3;
	m68k_areg(regs, reg) += adj;
	if (idx >= 0) {
		struct mmufixup *m = &mmufixup[idx];
		m->value += adj;
	}
	write_log("fixup %04x %d %d\n", mmu030_opcode & 0xffff, reg, adj);
#endif
}

void mmu030_page_fault(uaecptr addr, bool read, int flags, uae_u32 fc)
{
	if (flags < 0) {
		read = (regs.mmu_ssw & MMU030_SSW_RW) ? 1 : 0;
		fc = regs.mmu_ssw & MMU030_SSW_FC_MASK;
		flags = regs.mmu_ssw & ~(MMU030_SSW_FC | MMU030_SSW_RC | MMU030_SSW_FB | MMU030_SSW_RB | MMU030_SSW_RW | 7);
	}
	regs.wb3_status = 0;
	regs.wb2_status = 0;
	if (fc & 1) {
		regs.mmu_ssw = MMU030_SSW_DF | (MMU030_SSW_DF << 1);
		if (!(mmu030_state[1] & MMU030_STATEFLAG1_LASTWRITE)) {
			regs.wb2_status = mmu030fixupreg(0);
			mmu030fixupmod(regs.wb2_status, 0, 0);
			regs.wb3_status = mmu030fixupreg(1);
			mmu030fixupmod(regs.wb3_status, 0, 1);
		}
	} else {
		// only used by data fault but word sounds nice
		flags = MMU030_SSW_SIZE_W;
		if (currprefs.cpu_compatible) {
			regs.wb2_status = mmu030fixupreg(0);
			mmu030fixupmod(regs.wb2_status, 0, 0);
			regs.wb3_status = mmu030fixupreg(1);
			mmu030fixupmod(regs.wb3_status, 0, 1);
			regs.mmu_ssw = MMU030_SSW_FB | MMU030_SSW_RB;
		} else {
			regs.mmu_ssw = MMU030_SSW_FB | MMU030_SSW_RB;
		}
	}
	regs.mmu_ssw |= read ? MMU030_SSW_RW : 0;
	regs.mmu_ssw |= flags;
	regs.mmu_ssw |= fc;
	regs.mmu_ssw |= islrmw030 ? MMU030_SSW_RM : 0;
	regs.mmu_fault_addr = addr;
	// temporary store in 68040+ variables because stack frame creation may modify them.
	regs.wb3_data = mmu030_data_buffer_out;
	regs.wb2_address = mmu030_state[1];
    bBusErrorReadWrite = read; 
	mm030_stageb_address = addr;

#if MMUDEBUG
	write_log(_T("MMU: %02x la=%08X SSW=%04x read=%d size=%d fc=%d pc=%08x ob=%08x"),
		(mmu030_state[1] & MMU030_STATEFLAG1_LASTWRITE) ? 0xa : 0xb,
		addr, regs.mmu_ssw, read, (flags & MMU030_SSW_SIZE_B) ? 1 : (flags & MMU030_SSW_SIZE_W) ? 2 : 4, fc,
		regs.instruction_pc, mmu030_data_buffer_out);
	dump_opcode(mmu030_opcode & 0xffff);
	if (regs.opcode != mmu030_opcode)
		dump_opcode(regs.opcode & 0xffff);
	write_log(_T("\n"));
#endif

	ismoves030 = false;
	islrmw030 = false;

#if 0
	if (mmu030_state[1] & MMU030_STATEFLAG1_SUBACCESS0)
		write_log("!");
	if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1)
		write_log("!");
#endif

	THROW(2);
}

void mmu030_hardware_bus_error(uaecptr addr, uae_u32 v, bool read, bool ins, int size)
{
	int flags = size == sz_byte ? MMU030_SSW_SIZE_B : (size == sz_word ? MMU030_SSW_SIZE_W : MMU030_SSW_SIZE_L);
	int fc;
	if (ismoves030) {
		fc = read ? regs.sfc : regs.dfc;
	} else {
		fc = (regs.s ? 4 : 0) | (ins ? 2 : 1);
	}
	if (!read) {
		mmu030_data_buffer_out = v;
	} else {
		flags |= MMU030_SSW_RW;
	}
	mmu030_page_fault(addr, read, flags, fc);
}

bool mmu030_is_super_access(bool read)
{
	if (!ismoves030) {
		return regs.s;
	} else {
		uae_u32 fc = read ? regs.sfc : regs.dfc;
		return (fc & 4) != 0;
	}
}

static void mmu030_add_data_read_cache(uaecptr addr, uaecptr phys, uae_u32 fc)
{
#if MMU_DPAGECACHE030
	uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >>  mmu030.translation.page.size3m) | fc;
	uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
	if (idx2 < MMUFASTCACHE_ENTRIES030 - 1) {
		atc_data_cache_read[idx2].log = idx1;
		atc_data_cache_read[idx2].phys = phys;
		atc_data_cache_read[idx2].cs = mmu030_cache_state;
	}
#endif
}

static void mmu030_add_data_write_cache(uaecptr addr, uaecptr phys, uae_u32 fc)
{
#if MMU_DPAGECACHE030
	uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >>  mmu030.translation.page.size3m) | fc;
	uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
	if (idx2 < MMUFASTCACHE_ENTRIES030 - 1) {
		atc_data_cache_write[idx2].log = idx1;
		atc_data_cache_write[idx2].phys = phys;
		atc_data_cache_write[idx2].cs = mmu030_cache_state;
	}
#endif
}

static uaecptr mmu030_put_atc(uaecptr addr, int l, uae_u32 fc, uae_u32 size) {
    uae_u32 page_index = addr & mmu030.translation.page.mask;
    uae_u32 addr_mask = mmu030.translation.page.imask;
    uae_u32 physical_addr = mmu030.atc[l].physical.addr&addr_mask;

#if MMU030_ATC_DBG_MSG > 1
    write_log(_T("ATC match(%i): page addr = %08X, index = %08X\n"),
              l, physical_addr, page_index);
#endif
    
	if (mmu030.atc[l].physical.bus_error || mmu030.atc[l].physical.write_protect) {
        mmu030_page_fault(addr, false, size, fc);
        return 0;
    }

	mmu030_cache_state = mmu030.atc[l].physical.cache_inhibit;

	mmu030_add_data_write_cache(addr, physical_addr, fc);

	return physical_addr + page_index;
}

static uaecptr mmu030_get_atc(uaecptr addr, int l, uae_u32 fc, uae_u32 size) {
    uae_u32 page_index = addr & mmu030.translation.page.mask;
    uae_u32 addr_mask = mmu030.translation.page.imask;
    uae_u32 physical_addr = mmu030.atc[l].physical.addr&addr_mask;

#if MMU030_ATC_DBG_MSG > 1
    write_log(_T("ATC match(%i): page addr = %08X, index = %08X\n"), l,
              physical_addr, page_index);
#endif
   
	if (mmu030.atc[l].physical.bus_error) {
        mmu030_page_fault(addr, true, size, fc);
        return 0;
    }

	mmu030_cache_state = mmu030.atc[l].physical.cache_inhibit;
	
	mmu030_add_data_read_cache(addr, physical_addr, fc);

	return physical_addr + page_index;
}

static uaecptr mmu030_get_i_atc(uaecptr addr, int l, uae_u32 fc, uae_u32 size) {
	uae_u32 page_index = addr & mmu030.translation.page.mask;
	uae_u32 addr_mask = mmu030.translation.page.imask;
	uae_u32 physical_addr = mmu030.atc[l].physical.addr&addr_mask;

#if MMU030_ATC_DBG_MSG > 1
	write_log(_T("ATC match(%i): page addr = %08X, index = %08X\n"), l,
		physical_addr, page_index);
#endif

	if (mmu030.atc[l].physical.bus_error) {
		mmu030_page_fault(addr, true, size, fc);
		return 0;
	}

#if MMU_IPAGECACHE030
	mmu030.mmu030_cache_state = mmu030.atc[l].physical.cache_inhibit;
#if MMU_DIRECT_ACCESS
	mmu030.mmu030_last_physical_address_real = get_real_address(physical_addr);
#else
	mmu030.mmu030_last_physical_address = physical_addr;
#endif
	mmu030.mmu030_last_logical_address = (addr & mmu030.translation.page.imask) | fc;
#endif

	mmu030_cache_state = mmu030.atc[l].physical.cache_inhibit;

	return physical_addr + page_index;
}

/* Generic versions of above */
static uaecptr mmu030_put_atc_generic(uaecptr addr, int l, uae_u32 fc, int flags) {
    uae_u32 page_index = addr & mmu030.translation.page.mask;
    uae_u32 addr_mask = mmu030.translation.page.imask;
    uae_u32 physical_addr = mmu030.atc[l].physical.addr & addr_mask;

#if MMU030_ATC_DBG_MSG > 1
    write_log(_T("ATC match(%i): page addr = %08X, index = %08X\n"),
              l, physical_addr, page_index);
#endif
    
    if (mmu030.atc[l].physical.write_protect || mmu030.atc[l].physical.bus_error) {
		mmu030_page_fault(addr, false, flags, fc);
        return 0;
    }

	mmu030_add_data_write_cache(addr, physical_addr, fc);

	return physical_addr + page_index;
}

static uae_u32 mmu030_get_atc_generic(uaecptr addr, int l, uae_u32 fc, int flags, bool checkwrite) {
    uae_u32 page_index = addr & mmu030.translation.page.mask;
    uae_u32 addr_mask = mmu030.translation.page.imask;
    uae_u32 physical_addr = mmu030.atc[l].physical.addr & addr_mask;

#if MMU030_ATC_DBG_MSG > 1
    write_log(_T("ATC match(%i): page addr = %08X, index = %08X\n"), l,
              physical_addr, page_index);
#endif
    
	if (mmu030.atc[l].physical.bus_error || (checkwrite && mmu030.atc[l].physical.write_protect)) {
        mmu030_page_fault(addr, true, flags, fc);
        return 0;
    }

	mmu030_add_data_read_cache(addr, physical_addr, fc);

	return physical_addr + page_index;
}


/* This function checks if a certain logical address is in the ATC 
 * by comparing the logical address and function code to the values
 * stored in the ATC entries. If a matching entry is found it sets
 * the history bit and returns the cache index of the entry. */
static int mmu030_logical_is_in_atc(uaecptr addr, uae_u32 fc, bool write) {
    uaecptr logical_addr = 0;
    uae_u32 addr_mask = mmu030.translation.page.imask;
	uae_u32 maddr = addr & addr_mask;
    int offset = (maddr >> mmu030.translation.page.size) & 0x1f;

    int i, index;
	index = atcindextable[offset];
    for (i=0; i<ATC030_NUM_ENTRIES; i++) {
        logical_addr = mmu030.atc[index].logical.addr;
        /* If actual address matches address in ATC */
        if (maddr==(logical_addr&addr_mask) &&
            (mmu030.atc[index].logical.fc==fc) &&
            mmu030.atc[index].logical.valid) {
            /* If access is valid write and M bit is not set, invalidate entry
             * else return index */
            if (!write || mmu030.atc[index].physical.modified ||
                mmu030.atc[index].physical.write_protect ||
                mmu030.atc[index].physical.bus_error) {
                /* Maintain history bit */
					mmu030_atc_handle_history_bit(index);
					atcindextable[offset] = index;
					return index;
				} else {
					mmu030.atc[index].logical.valid = false;
				}
		}
		index++;
		if (index >= ATC030_NUM_ENTRIES)
			index = 0;
    }
    return -1;
}

/* Memory access functions:
 * If the address matches one of the transparent translation registers
 * use it directly as physical address, else check ATC for the
 * logical address. If the logical address is not resident in the ATC
 * create a new ATC entry and then look up the physical address. 
 */

STATIC_INLINE void cacheablecheck(uaecptr addr)
{
	if (mmu030_cache_state == CACHE_ENABLE_ALL) {
		// MMU didn't inhibit caches, use hardware cache state
		mmu030_cache_state = ce_cachable[addr >> 16];
	}
}

void mmu030_put_long(uaecptr addr, uae_u32 val, uae_u32 fc)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,true)) && mmu030.enabled) {
#if MMU_DPAGECACHE030
		uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | fc;
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
		if (atc_data_cache_write[idx2].log == idx1) {
			addr = atc_data_cache_write[idx2].phys | (addr & mmu030.translation.page.mask);
			mmu030_cache_state = atc_data_cache_write[idx2].cs;
		} else
#endif
		{
			int atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
			if (atc_line_num>=0) {
				addr = mmu030_put_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_L);
			} else {
				mmu030_table_search(addr,fc,true,0);
				addr = mmu030_put_atc(addr, mmu030_logical_is_in_atc(addr,fc,true), fc, MMU030_SSW_SIZE_L);
			}
		}
	}
	cacheablecheck(addr);
	x_phys_put_long(addr,val);
}

void mmu030_put_word(uaecptr addr, uae_u16 val, uae_u32 fc)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,true)) && mmu030.enabled) {
#if MMU_DPAGECACHE030
		uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | fc;
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
		if (atc_data_cache_write[idx2].log == idx1) {
			addr = atc_data_cache_write[idx2].phys | (addr & mmu030.translation.page.mask);
			mmu030_cache_state = atc_data_cache_write[idx2].cs;
		} else
#endif
		{
			int atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
			if (atc_line_num>=0) {
				addr = mmu030_put_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_W);
			} else {
				mmu030_table_search(addr, fc, true, 0);
				addr = mmu030_put_atc(addr,  mmu030_logical_is_in_atc(addr,fc,true), fc, MMU030_SSW_SIZE_W);
			}
		}
	}
	cacheablecheck(addr);
	x_phys_put_word(addr,val);
}

void mmu030_put_byte(uaecptr addr, uae_u8 val, uae_u32 fc)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,true)) && mmu030.enabled) {
#if MMU_DPAGECACHE030
		uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | fc;
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
		if (atc_data_cache_write[idx2].log == idx1) {
			addr = atc_data_cache_write[idx2].phys | (addr & mmu030.translation.page.mask);
			mmu030_cache_state = atc_data_cache_write[idx2].cs;
		} else
#endif
		{
			int atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
			if (atc_line_num>=0) {
				addr = mmu030_put_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_B);
			} else {
				mmu030_table_search(addr, fc, true, 0);
				addr = mmu030_put_atc(addr, mmu030_logical_is_in_atc(addr,fc,true), fc, MMU030_SSW_SIZE_B);
			}
		}
	}
	cacheablecheck(addr);
	x_phys_put_byte(addr,val);
}


uae_u32 mmu030_get_long(uaecptr addr, uae_u32 fc)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,false)) && mmu030.enabled) {
#if MMU_DPAGECACHE030
		uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | fc;
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
		if (atc_data_cache_read[idx2].log == idx1) {
			addr = atc_data_cache_read[idx2].phys | (addr & mmu030.translation.page.mask);
			mmu030_cache_state = atc_data_cache_read[idx2].cs;
		} else
#endif
		{
			int atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
			if (atc_line_num>=0) {
				addr = mmu030_get_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_L);
			} else {
				mmu030_table_search(addr, fc, false, 0);
				addr = mmu030_get_atc(addr, mmu030_logical_is_in_atc(addr,fc,false), fc, MMU030_SSW_SIZE_L);
			}
		}
	}
	cacheablecheck(addr);
	uae_u32 v = x_phys_get_long(addr);
	return v;
}

uae_u16 mmu030_get_word(uaecptr addr, uae_u32 fc)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,false)) && mmu030.enabled) {
#if MMU_DPAGECACHE030
		uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | fc;
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
		if (atc_data_cache_read[idx2].log == idx1) {
			addr = atc_data_cache_read[idx2].phys | (addr & mmu030.translation.page.mask);
			mmu030_cache_state = atc_data_cache_read[idx2].cs;
		} else
#endif
		{
			int atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
		    if (atc_line_num>=0) {
				addr = mmu030_get_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_W);
			} else {
				mmu030_table_search(addr, fc, false, 0);
				addr = mmu030_get_atc(addr, mmu030_logical_is_in_atc(addr,fc,false), fc, MMU030_SSW_SIZE_W);
			}
		}
	}
	cacheablecheck(addr);
	uae_u16 v = x_phys_get_word(addr);
	return v;
}

uae_u8 mmu030_get_byte(uaecptr addr, uae_u32 fc)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,false)) && mmu030.enabled) {
#if MMU_DPAGECACHE030
		uae_u32 idx1 = ((addr & mmu030.translation.page.imask) >> mmu030.translation.page.size3m) | fc;
		uae_u32 idx2 = idx1 & (MMUFASTCACHE_ENTRIES030 - 1);
		if (atc_data_cache_read[idx2].log == idx1) {
			addr = atc_data_cache_read[idx2].phys | (addr & mmu030.translation.page.mask);
			mmu030_cache_state = atc_data_cache_read[idx2].cs;
		} else
#endif
		{
			int atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
			if (atc_line_num>=0) {
				addr = mmu030_get_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_B);
			} else {
				mmu030_table_search(addr, fc, false, 0);
				addr = mmu030_get_atc(addr, mmu030_logical_is_in_atc(addr,fc,false), fc, MMU030_SSW_SIZE_B);
			}
		}
	}
	cacheablecheck(addr);
	uae_u8 v = x_phys_get_byte(addr);
	return v;
}


uae_u32 mmu030_get_ilong(uaecptr addr, uae_u32 fc)
{
	uae_u32 v;
#if MMU_IPAGECACHE030
	if (((addr & mmu030.translation.page.imask) | fc) == mmu030.mmu030_last_logical_address) {
#if MMU_DIRECT_ACCESS
		uae_u8 *p = &mmu030.mmu030_last_physical_address_real[addr & mmu030.translation.page.mask];
		return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
#else
		mmu030_cache_state = mmu030.mmu030_cache_state;
		v = x_phys_get_ilong(mmu030.mmu030_last_physical_address + (addr & mmu030.translation.page.mask));
		return v;
#endif
	}
	mmu030.mmu030_last_logical_address = 0xffffffff;
#endif

	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr, fc, false)) && mmu030.enabled) {
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
		if (atc_line_num >= 0) {
			addr = mmu030_get_i_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_L);
		} else {
			mmu030_table_search(addr, fc, false, 0);
			addr = mmu030_get_i_atc(addr, mmu030_logical_is_in_atc(addr, fc, false), fc, MMU030_SSW_SIZE_L);
		}
	}
	cacheablecheck(addr);
	v = x_phys_get_ilong(addr);
	return v;
}

uae_u16 mmu030_get_iword(uaecptr addr, uae_u32 fc)
{
	uae_u16 v;
#if MMU_IPAGECACHE030
	if (((addr & mmu030.translation.page.imask) | fc) == mmu030.mmu030_last_logical_address) {
#if MMU_DIRECT_ACCESS
		uae_u8 *p = &mmu030.mmu030_last_physical_address_real[addr & mmu030.translation.page.mask];
		return (p[0] << 8) | p[1];
#else
		mmu030_cache_state = mmu030.mmu030_cache_state;
		v = x_phys_get_iword(mmu030.mmu030_last_physical_address + (addr & mmu030.translation.page.mask));
		return v;
#endif
	}
	mmu030.mmu030_last_logical_address = 0xffffffff;
#endif

	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr, fc, false)) && mmu030.enabled) {
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
		if (atc_line_num >= 0) {
			addr = mmu030_get_i_atc(addr, atc_line_num, fc, MMU030_SSW_SIZE_W);
		} else {
			mmu030_table_search(addr, fc, false, 0);
			addr = mmu030_get_i_atc(addr, mmu030_logical_is_in_atc(addr, fc, false), fc, MMU030_SSW_SIZE_W);
		}
	}
	cacheablecheck(addr);
	v = x_phys_get_iword(addr);
	return v;
}

/* Not commonly used access function */

static void mmu030_put_generic_lrmw(uaecptr addr, uae_u32 val, uae_u32 fc, int size, int flags)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_lrmw_ttr_access(addr,fc)) && mmu030.enabled) {
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
		if (atc_line_num>=0) {
			addr = mmu030_put_atc_generic(addr, atc_line_num, fc, flags);
		} else {
			mmu030_table_search(addr, fc, true, 0);
			atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
			addr = mmu030_put_atc_generic(addr, atc_line_num, fc, flags);
		}
	}

	cacheablecheck(addr);
	if (size == sz_byte)
		x_phys_put_byte(addr, val);
	else if (size == sz_word)
		x_phys_put_word(addr, val);
	else
		x_phys_put_long(addr, val);
}

void mmu030_put_generic(uaecptr addr, uae_u32 val, uae_u32 fc, int size, int flags)
{
 	mmu030_cache_state = CACHE_ENABLE_ALL;

	if (flags & MMU030_SSW_RM) {
		mmu030_put_generic_lrmw(addr, val, fc, size, flags);
		return;
	}

	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,true)) && mmu030.enabled) {
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
		if (atc_line_num>=0) {
			addr = mmu030_put_atc_generic(addr, atc_line_num, fc, flags);
		} else {
			mmu030_table_search(addr, fc, true, 0);
			atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
			addr = mmu030_put_atc_generic(addr, atc_line_num, fc, flags);
		}
	}

	cacheablecheck(addr);
	if (size == sz_byte)
		x_phys_put_byte(addr, val);
	else if (size == sz_word)
		x_phys_put_word(addr, val);
	else
		x_phys_put_long(addr, val);
}

static uae_u32 mmu030_get_generic_lrmw(uaecptr addr, uae_u32 fc, int size, int flags)
{
	uae_u32 v;
	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_lrmw_ttr_access(addr,fc)) && mmu030.enabled) {
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
		if (atc_line_num>=0) {
			addr = mmu030_get_atc_generic(addr, atc_line_num, fc, flags, true);
		} else {
			mmu030_table_search(addr, fc, true, 0);
			atc_line_num = mmu030_logical_is_in_atc(addr, fc, true);
			addr = mmu030_get_atc_generic(addr, atc_line_num, fc, flags, true);
		}
	}

	cacheablecheck(addr);
	if (size == sz_byte)
		v = x_phys_get_byte(addr);
	else if (size == sz_word)
		v = x_phys_get_word(addr);
	else
		v = x_phys_get_long(addr);

	return v;
}

uae_u32 mmu030_get_generic(uaecptr addr, uae_u32 fc, int size, int flags)
{
	uae_u32 v;
	mmu030_cache_state = CACHE_ENABLE_ALL;

	if (flags & MMU030_SSW_RM) {
		return mmu030_get_generic_lrmw(addr, fc, size, flags);
	}

	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,false)) && mmu030.enabled) {
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
		if (atc_line_num>=0) {
			addr = mmu030_get_atc_generic(addr, atc_line_num, fc, flags, false);
		} else {
			mmu030_table_search(addr, fc, false, 0);
			atc_line_num = mmu030_logical_is_in_atc(addr, fc, false);
			addr = mmu030_get_atc_generic(addr, atc_line_num, fc, flags, false);
		}
	}

	cacheablecheck(addr);
	if (size == sz_byte)
		v = x_phys_get_byte(addr);
	else if (size == sz_word)
		v = x_phys_get_word(addr);
	else
		v = x_phys_get_long(addr);

	return v;
}

uae_u8 uae_mmu030_check_fc(uaecptr addr, bool write, uae_u32 size)
{
	uae_u32 fc = regs.fc030;
 	mmu030_cache_state = CACHE_ENABLE_ALL;
	if (fc != 7 && (!tt_enabled || !mmu030_match_ttr_access(addr,fc,write)) && mmu030.enabled) {
		uae_u32 flags = mmu030_size[size];
		int atc_line_num = mmu030_logical_is_in_atc(addr, fc, write);
		if (atc_line_num>=0) {
			addr = mmu030_get_atc_generic(addr, atc_line_num, fc, flags, write);
		} else {
			mmu030_table_search(addr, fc, write, 0);
			atc_line_num = mmu030_logical_is_in_atc(addr, fc, write);
			addr = mmu030_get_atc_generic(addr, atc_line_num, fc, flags, false);
		}
	}
	// MMU inhibited
	if (mmu030_cache_state != CACHE_ENABLE_ALL)
		return mmu030_cache_state;
	return ce_cachable[addr >> 16];
}

/* Locked RMW is rarely used */
static uae_u32 uae_mmu030_get_lrmw_fcx(uaecptr addr, int size, int fc)
{
	if (size == sz_byte) {
		return mmu030_get_generic(addr, fc, size, MMU030_SSW_RM | MMU030_SSW_SIZE_B);
	} else if (size == sz_word) {
		if (unlikely(is_unaligned_bus(addr, 2)))
			return mmu030_get_word_unaligned(addr, fc, MMU030_SSW_RM);
		else
			return mmu030_get_generic(addr, fc, size, MMU030_SSW_RM | MMU030_SSW_SIZE_W);
	} else {
		if (unlikely(is_unaligned_bus(addr, 4)))
			return mmu030_get_long_unaligned(addr, fc, MMU030_SSW_RM);
		else
			return mmu030_get_generic(addr, fc, size, MMU030_SSW_RM | MMU030_SSW_SIZE_L);
	}
}
uae_u32 uae_mmu030_get_lrmw(uaecptr addr, int size)
{
	uae_u32 fc = (regs.s ? 4 : 0) | 1;
	islrmw030 = true;
	uae_u32 v = uae_mmu030_get_lrmw_fcx(addr, size, fc);
	islrmw030 = false;
	return v;
}

static void uae_mmu030_put_lrmw_fcx(uaecptr addr, uae_u32 val, int size, int fc)
{
	if (size == sz_byte) {
		mmu030_put_generic(addr, val, fc, size, MMU030_SSW_RM | MMU030_SSW_SIZE_B);
	} else if (size == sz_word) {
		if (unlikely(is_unaligned_bus(addr, 2)))
			mmu030_put_word_unaligned(addr, val, fc, MMU030_SSW_RM);
		else
			mmu030_put_generic(addr, val, fc, size, MMU030_SSW_RM | MMU030_SSW_SIZE_W);
	} else {
		if (unlikely(is_unaligned_bus(addr, 4)))
			mmu030_put_long_unaligned(addr, val, fc, MMU030_SSW_RM);
		else
			mmu030_put_generic(addr, val, fc, size, MMU030_SSW_RM | MMU030_SSW_SIZE_L);
	}
}
void uae_mmu030_put_lrmw(uaecptr addr, uae_u32 val, int size)
{
	uae_u32 fc = (regs.s ? 4 : 0) | 1;
	islrmw030 = true;
	uae_mmu030_put_lrmw_fcx(addr, val, size, fc);
	islrmw030 = false;
}

uae_u32 REGPARAM2 mmu030_get_ilong_unaligned(uaecptr addr, uae_u32 fc, int flags)
{
	uae_u32 res;

	res = (uae_u32)mmu030_get_iword(addr, fc) << 16;
	SAVE_EXCEPTION;
	TRY(prb) {
		res |= mmu030_get_iword(addr + 2, fc);
		RESTORE_EXCEPTION;
	}
	CATCH(prb) {
		RESTORE_EXCEPTION;
		THROW_AGAIN(prb);
	} ENDTRY
	return res;
}

static void unalign_init(uaecptr addr, bool l, bool l2)
{
	if (l2)
		mmu030_state[1] |= MMU030_STATEFLAG1_SUBACCESSX;
	if (l)
		mmu030_state[1] |= MMU030_STATEFLAG1_SUBACCESSL;
	mmu030_state[1] |= MMU030_STATEFLAG1_SUBACCESS0;
#if MMU030_DEBUG > 1
	write_log(_T("unalign_init %08x %08x %d %d\n"), addr, mmu030_state[1], l, l2);
#endif
}
static void unalign_set(int state)
{
	mmu030_state[1] |= (1 << state) << (MMU030_STATEFLAG1_SUBACCESS_SHIFT + 1);
#if MMU030_DEBUG > 1
	write_log(_T("unalign_set %d %08x\n"), state, mmu030_state[1]);
#endif
}
static void unalign_clear(void)
{
#if MMU030_DEBUG > 1
	write_log(_T("unalign_clear %08x %08x\n"), mmu030_state[1], mmu030_data_buffer_out);
#endif
	mmu030_state[1] &= ~(MMU030_STATEFLAG1_SUBACCESSL | MMU030_STATEFLAG1_SUBACCESSX |
		MMU030_STATEFLAG1_SUBACCESS0 | MMU030_STATEFLAG1_SUBACCESS1 | MMU030_STATEFLAG1_SUBACCESS2 | MMU030_STATEFLAG1_SUBACCESS3);
}

uae_u16 REGPARAM2 mmu030_get_word_unaligned(uaecptr addr, uae_u32 fc, int flags)
{
	unalign_init(addr, false, false);
	mmu030_data_buffer_out = mmu030_get_generic(addr, fc, sz_byte, flags | MMU030_SSW_SIZE_W) << 8;
	unalign_set(0);
	mmu030_data_buffer_out |= mmu030_get_generic(addr + 1, fc, sz_byte, flags | MMU030_SSW_SIZE_B);
	unalign_clear();
	return mmu030_data_buffer_out;
}

uae_u32 REGPARAM2 mmu030_get_long_unaligned(uaecptr addr, uae_u32 fc, int flags)
{
	if (likely(!(addr & 1))) {
		unalign_init(addr, true, false);
		mmu030_data_buffer_out = mmu030_get_generic(addr, fc, sz_word, flags | MMU030_SSW_SIZE_L) << 16;
		unalign_set(0);
		mmu030_data_buffer_out |= mmu030_get_generic(addr + 2, fc, sz_word, flags | MMU030_SSW_SIZE_W);
	} else {
		unalign_init(addr, true, true);
		mmu030_data_buffer_out = mmu030_get_generic(addr, fc, sz_byte, flags | MMU030_SSW_SIZE_L) << 24;
		unalign_set(0);
		mmu030_data_buffer_out |= mmu030_get_generic(addr + 1, fc, sz_word, flags | MMU030_SSW_SIZE_W) << 8;
		unalign_set(1);
		mmu030_data_buffer_out |= mmu030_get_generic(addr + 3, fc, sz_byte, flags | MMU030_SSW_SIZE_B);
	}
	unalign_clear();
	return mmu030_data_buffer_out;
}

void REGPARAM2 mmu030_put_long_unaligned(uaecptr addr, uae_u32 val, uae_u32 fc, int flags)
{
	if (likely(!(addr & 1))) {
		unalign_init(addr, true, false);
		mmu030_put_generic(addr, val >> 16, fc, sz_word, flags | MMU030_SSW_SIZE_L);
		unalign_set(0);
		mmu030_put_generic(addr + 2, val, fc, sz_word, flags | MMU030_SSW_SIZE_W);
	} else {
		unalign_init(addr, true, true);
		mmu030_put_generic(addr, val >> 24, fc, sz_byte, flags | MMU030_SSW_SIZE_L);
		unalign_set(0);
		mmu030_put_generic(addr + 1, val >> 8, fc, sz_word, flags | MMU030_SSW_SIZE_W);
		unalign_set(1);
		mmu030_put_generic(addr + 3, val, fc, sz_byte, flags | MMU030_SSW_SIZE_B);
	}
	unalign_clear();
}

void REGPARAM2 mmu030_put_word_unaligned(uaecptr addr, uae_u16 val, uae_u32 fc, int flags)
{
	unalign_init(addr, false, false);
	mmu030_put_generic(addr, val >> 8, fc, sz_byte, flags | MMU030_SSW_SIZE_W);
	unalign_set(0);
	mmu030_put_generic(addr + 1, val, fc, sz_byte, flags | MMU030_SSW_SIZE_B);
	unalign_clear();
}


/* Used by debugger */
static uaecptr mmu030_get_addr_atc(uaecptr addr, int l, uae_u32 fc, bool write) {
    uae_u32 page_index = addr & mmu030.translation.page.mask;
    uae_u32 addr_mask = mmu030.translation.page.imask;
    
    uae_u32 physical_addr = mmu030.atc[l].physical.addr&addr_mask;
    physical_addr += page_index;
    
	if (mmu030.atc[l].physical.bus_error || (write && mmu030.atc[l].physical.write_protect)) {
        mmu030_page_fault(addr, write == 0, MMU030_SSW_SIZE_B, fc);
        return 0;
    }

    return physical_addr;
}
uaecptr mmu030_translate(uaecptr addr, bool super, bool data, bool write)
{
	int fc = (super ? 4 : 0) | (data ? 1 : 2);
	if ((fc==7) || (mmu030_match_ttr(addr,fc,write)&TT_OK_MATCH) || (!mmu030.enabled)) {
		return addr;
    }
    int atc_line_num = mmu030_logical_is_in_atc(addr, fc, write);

    if (atc_line_num>=0) {
        return mmu030_get_addr_atc(addr, atc_line_num, fc, write);
    } else {
        mmu030_table_search(addr, fc, false, 0);
        return mmu030_get_addr_atc(addr, mmu030_logical_is_in_atc(addr,fc,write), fc, write);
    }
}

/* MMU Reset */
void mmu030_reset(int hardreset)
{
	if (!savestate_state) {
		/* A CPU reset causes the E-bits of TC and TT registers to be zeroed. */
		mmu030.enabled = false;
	#if MMU_IPAGECACHE030
		mmu030.mmu030_last_logical_address = 0xffffffff;
	#endif
		regs.mmu_page_size = 0;
		if (hardreset >= 0) {
			tc_030 &= ~TC_ENABLE_TRANSLATION;
			tt0_030 &= ~TT_ENABLE;
			tt1_030 &= ~TT_ENABLE;
		}
		if (hardreset > 0) {
			srp_030 = crp_030 = 0;
			tt0_030 = tt1_030 = tc_030 = 0;
			mmusr_030 = 0;
			mmu030_flush_atc_all();
		}
	}
	mmu030_set_funcs();
}

void mmu030_set_funcs(void)
{
	if (currprefs.mmu_model != 68030)
		return;
	if (currprefs.cpu_memory_cycle_exact) {
		x_phys_get_iword = mem_access_delay_wordi_read_ce020;
		x_phys_get_ilong = mem_access_delay_longi_read_ce020;
		x_phys_get_byte = mem_access_delay_byte_read_ce020;
		x_phys_get_word = mem_access_delay_word_read_ce020;
		x_phys_get_long = mem_access_delay_long_read_ce020;
		x_phys_put_byte = mem_access_delay_byte_write_ce020;
		x_phys_put_word = mem_access_delay_word_write_ce020;
		x_phys_put_long = mem_access_delay_long_write_ce020;
	} else {
		x_phys_get_iword = phys_get_word;
		x_phys_get_ilong = phys_get_long;
		x_phys_get_byte = phys_get_byte;
		x_phys_get_word = phys_get_word;
		x_phys_get_long = phys_get_long;
		x_phys_put_byte = phys_put_byte;
		x_phys_put_word = phys_put_word;
		x_phys_put_long = phys_put_long;
	}
}

#define unalign_done(f) \
	st |= f; \
	mmu030_state[1] = st;

typedef uae_u32(*unaligned_read_func)(uaecptr addr, uae_u32 fc, int size, int flags);

static void mmu030_unaligned_read_continue(uaecptr addr, int fc, unaligned_read_func func)
{
	uae_u32 st = mmu030_state[1];

#if MMUDEBUG
	write_log(_T("unaligned_read_continue_s: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif

	if (st & MMU030_STATEFLAG1_SUBACCESSL) {
		if (st & MMU030_STATEFLAG1_SUBACCESSX) {
			// odd long access: byte + word + byte
			if (!(st & MMU030_STATEFLAG1_SUBACCESS1)) {
				mmu030_data_buffer_out &= 0x00ffffff;
				mmu030_data_buffer_out |= func(addr, fc, sz_byte, MMU030_SSW_SIZE_L) << 24;
#if MMUDEBUG
				write_log(_T("unaligned_read_continue_0: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS1);
				addr++;
			}
			if (!(st & MMU030_STATEFLAG1_SUBACCESS2)) {
				mmu030_data_buffer_out &= 0xff0000ff;
				mmu030_data_buffer_out |= func(addr, fc, sz_word, MMU030_SSW_SIZE_W) << 8;
#if MMUDEBUG
				write_log(_T("unaligned_read_continue_1: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS2);
				addr += 2;
			}
			if (!(st & MMU030_STATEFLAG1_SUBACCESS3)) {
				mmu030_data_buffer_out &= 0xffffff00;
				mmu030_data_buffer_out |= func(addr, fc, sz_byte, MMU030_SSW_SIZE_B) << 0;
				unalign_done(MMU030_STATEFLAG1_SUBACCESS3);
				addr++;
			}
		} else {
			// even but unaligned long access: word + word
			if (!(st & MMU030_STATEFLAG1_SUBACCESS1)) {
				mmu030_data_buffer_out &= 0x0000ffff;
				mmu030_data_buffer_out |= func(addr, fc, sz_word, MMU030_SSW_SIZE_L) << 16;
#if MMUDEBUG
				write_log(_T("unaligned_read_continue_0: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS1);
				addr += 2;
			}
			if (!(st & MMU030_STATEFLAG1_SUBACCESS2)) {
				mmu030_data_buffer_out &= 0xffff0000;
				mmu030_data_buffer_out |= func(addr, fc, sz_word, MMU030_SSW_SIZE_W) << 0;
				unalign_done(MMU030_STATEFLAG1_SUBACCESS2);
				addr += 2;
			}
		}
	} else {
		// odd word access: byte + byte
		if (!(st & MMU030_STATEFLAG1_SUBACCESS1)) {
			mmu030_data_buffer_out &= 0x00ff;
			mmu030_data_buffer_out |= func(addr, fc, sz_byte, MMU030_SSW_SIZE_W) << 8;
#if MMUDEBUG
			write_log(_T("unaligned_read_continue_0: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
			unalign_done(MMU030_STATEFLAG1_SUBACCESS1);
			addr++;
		}
		if (!(st & MMU030_STATEFLAG1_SUBACCESS2)) {
			mmu030_data_buffer_out &= 0xff00;
			mmu030_data_buffer_out |= func(addr, fc, sz_byte, MMU030_SSW_SIZE_B) << 0;
			unalign_done(MMU030_STATEFLAG1_SUBACCESS2);
			addr++;
		}
	}

#if MMUDEBUG
	write_log(_T("unaligned_read_continue_e: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
	unalign_clear();
}

typedef void (*unaligned_write_func)(uaecptr addr, uae_u32 val, uae_u32 fc, int size, int flags);

static void mmu030_unaligned_write_continue(uaecptr addr, int fc, unaligned_write_func func)
{
	uae_u32 st = mmu030_state[1];

#if MMUDEBUG
	write_log(_T("unaligned_write_continue_s: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif

	if (st & MMU030_STATEFLAG1_SUBACCESSL) {
		// odd long access: byte + word + byte
		if (st & MMU030_STATEFLAG1_SUBACCESSX) {
			if (!(st & MMU030_STATEFLAG1_SUBACCESS1)) {
				func(addr, mmu030_data_buffer_out >> 24, fc, sz_byte, MMU030_SSW_SIZE_L);
#if MMUDEBUG
				write_log(_T("unaligned_write_continue_0: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS1);
				addr++;
			}
			if (!(st & MMU030_STATEFLAG1_SUBACCESS2)) {
				func(addr, mmu030_data_buffer_out >> 8, fc, sz_word, MMU030_SSW_SIZE_W);
#if MMUDEBUG
				write_log(_T("unaligned_write_continue_1: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS2);
				addr += 2;
			}
			if (!(st & MMU030_STATEFLAG1_SUBACCESS3)) {
				func(addr, mmu030_data_buffer_out >> 0, fc, sz_byte, MMU030_SSW_SIZE_B);
#if MMUDEBUG
				write_log(_T("unaligned_write_continue_2: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS3);
				addr++;
			}
		} else {
			// even but unaligned long access: word + word
			if (!(st & MMU030_STATEFLAG1_SUBACCESS1)) {
				func(addr, mmu030_data_buffer_out >> 16, fc, sz_word, MMU030_SSW_SIZE_L);
#if MMUDEBUG
				write_log(_T("unaligned_write_continue_0: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
				unalign_done(MMU030_STATEFLAG1_SUBACCESS1);
				addr += 2;
			}
			if (!(st & MMU030_STATEFLAG1_SUBACCESS2)) {
				func(addr, mmu030_data_buffer_out >> 0, fc, sz_word, MMU030_SSW_SIZE_W);
				unalign_done(MMU030_STATEFLAG1_SUBACCESS2);
				addr += 2;
			}
		}
	} else {
		// odd word access: byte + byte
		if (!(st & MMU030_STATEFLAG1_SUBACCESS1)) {
			func(addr, mmu030_data_buffer_out >> 8, fc, sz_byte, MMU030_SSW_SIZE_W);
#if MMUDEBUG
			write_log(_T("unaligned_write_continue_0: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
			unalign_done(MMU030_STATEFLAG1_SUBACCESS1);
			addr++;
		}
		if (!(st & MMU030_STATEFLAG1_SUBACCESS2)) {
			func(addr, mmu030_data_buffer_out >> 0, fc, sz_byte, MMU030_SSW_SIZE_B);
			unalign_done(MMU030_STATEFLAG1_SUBACCESS2);
			addr++;
		}
	}

#if MMUDEBUG
	write_log(_T("unaligned_write_continue_e: %08x %d %08x %08x\n"), addr, fc, mmu030_data_buffer_out, st);
#endif
	unalign_clear();
}

void m68k_do_rte_mmu030 (uaecptr a7)
{
	struct mmu030_access mmu030_ad_v[MAX_MMU030_ACCESS + 1];

	// Restore access error exception state

	uae_u16 sr = get_word_mmu030(a7);
	uae_u32 pc = get_long_mmu030(a7 + 2);
	uae_u16 format = get_word_mmu030(a7 + 6);
	uae_u16 frame = format >> 12;
	uae_u16 ssw = get_word_mmu030(a7 + 10);
	uae_u32 fault_addr = get_long_mmu030(a7 + 16);
	// Data output buffer
	uae_u32 mmu030_data_buffer_out_v = get_long_mmu030(a7 + 0x18);
	// Internal register, our opcode storage area
	uae_u32 oc = get_long_mmu030(a7 + 0x14);
	int idxsize = -1, idxsize_done = -1;

	// Fetch last word, real CPU does it to allow OS bus handler to map
	// the page if frame crosses pages and following page is not resident.
	if (frame == 0xb)
		get_word_mmu030(a7 + 92 - 2);
	else
		get_word_mmu030(a7 + 32 - 2);

	// Rerun "mmu030_opcode" using restored state.
	mmu030_retry = true;

	if (frame == 0xa) {
		// this is always last write data write fault

		uae_u32 mmu030_state_1 = get_word_mmu030(a7 + 0x8);

#if MMU030_DEBUG
		if (!(mmu030_state_1 & MMU030_STATEFLAG1_LASTWRITE)) {
			write_log(_T("68030 MMU short bus fault but no lastwrite set!?\n"));
		}
		if (ssw & (MMU030_SSW_FB | MMU030_SSW_FC | MMU030_SSW_FB | MMU030_SSW_RC | MMU030_SSW_RB)) {
			write_log(_T("68030 MMU short bus fault and pipeline fault?\n"));
		}
		if (ssw & MMU030_SSW_RW) {
			write_log(_T("68030 MMU short bus fault but read fault!?\n"));
		}
#endif

		// did we have data fault but DF bit cleared?
		if (ssw & (MMU030_SSW_DF << 1) && !(ssw & MMU030_SSW_DF)) {
			// DF not set: mark access as done
			unalign_clear();
		}

		mmu030_data_buffer_out = mmu030_data_buffer_out_v;
		mmu030_state[0] = 0;
		mmu030_state[1] = mmu030_state_1;
		mmu030_state[2] = 0;
		mmu030_opcode = oc;
		mmu030_idx = mmu030_idx_done = 0;

		m68k_areg(regs, 7) += 32;

	} else if (frame == 0xb) {

		// get_disp_ea_020
		uae_u32 mmu030_disp_store_0 = get_long_mmu030(a7 + 0x1c);
		uae_u32 mmu030_disp_store_1 = get_long_mmu030(a7 + 0x1c + 4);
		// Internal register, misc flags
		uae_u32 ps = get_long_mmu030(a7 + 0x28);
		// Data buffer
		uae_u32 mmu030_data_buffer_in_v = get_long_mmu030(a7 + 0x2c);
		// Misc state data
		uae_u32 mmu030_state_0 = get_word_mmu030(a7 + 0x30);
		uae_u32 mmu030_state_1 = get_word_mmu030(a7 + 0x32);
		uae_u32 mmu030_state_2 = get_word_mmu030(a7 + 0x34);

		uae_u32 mmu030_opcode_v = (ps & 0x80000000) ? 0xffffffff : (oc & 0xffff);

		uae_u32 mmu030_fmovem_store_0 = 0;
		uae_u32 mmu030_fmovem_store_1 = 0;
		if (mmu030_state[1] & MMU030_STATEFLAG1_FMOVEM) {
			mmu030_fmovem_store_0 = get_long_mmu030(a7 + 0x5c - (7 + 1) * 4);
			mmu030_fmovem_store_1 = get_long_mmu030(a7 + 0x5c - (8 + 1) * 4);
		}

		uae_u16 v = get_word_mmu030(a7 + 0x36);
		idxsize = v & 0x0f;
		idxsize_done = (v >> 4) & 0x0f;
		for (int i = 0; i < idxsize_done + 1; i++) {
			mmu030_ad_v[i].val = get_long_mmu030(a7 + 0x5c - (i + 1) * 4);
		}

		regs.wb2_status = v >> 8;
		regs.wb3_status = mmu030_state_2 >> 8;
		mmu030fixupmod(regs.wb2_status, 1, -1);
		mmu030fixupmod(regs.wb3_status, 1, -1);

		// did we have data fault but DF bit cleared?
		if (ssw & (MMU030_SSW_DF << 1) && !(ssw & MMU030_SSW_DF)) {
			// DF not set: mark access as done
			mmu030_data_buffer_out_v = mmu030_data_buffer_in_v;
			if (ssw & MMU030_SSW_RM) {
				// Read-Modify-Write: whole instruction is considered done
				write_log(_T("Read-Modify-Write and DF bit cleared! PC=%08x\n"), regs.instruction_pc);
				mmu030_retry = false;
			} else if (mmu030_state_1 & MMU030_STATEFLAG1_MOVEM1) {
				// if movem, skip next move
				mmu030_state_1 |= MMU030_STATEFLAG1_MOVEM2;
			} else {
				if (ssw & MMU030_SSW_RW) {
					// Read and no DF: use value in data input buffer
					mmu030_ad_v[idxsize_done].val = mmu030_data_buffer_in_v;
				} // else: use value idxsize_done that was saved from regs.wb3_data;
				idxsize_done++;
			}
			unalign_clear();
		}
		// did we have ins fault and RB bit cleared?
		if ((ssw & MMU030_SSW_FB) && !(ssw & MMU030_SSW_RB)) {
			uae_u16 stageb = get_word_mmu030(a7 + 0x0e);
			if (mmu030_opcode_v == 0xffffffff) {
				mmu030_opcode_stageb = stageb;
				write_log(_T("Software fixed stage B! opcode = %04x\n"), stageb);
			} else {
				mmu030_ad_v[idxsize_done].val = stageb;
				idxsize_done++;
				write_log(_T("Software fixed stage B! opcode = %04X, opword = %04x\n"), mmu030_opcode_v, stageb);
			}
		}

#if MMU030_DEBUG
		if (mmu030_state_1 & MMU030_STATEFLAG1_LASTWRITE) {
			write_log(_T("68030 MMU long bus fault but lastwrite set!?\n"));
		}
#endif
		// Retried data access is the only memory access that can be done after this.

		// restore global state variables
		mmu030_opcode = mmu030_opcode_v;
		mmu030_state[0] = mmu030_state_0;
		mmu030_state[1] = mmu030_state_1;
		mmu030_state[2] = mmu030_state_2;
		mmu030_disp_store[0] = mmu030_disp_store_0;
		mmu030_disp_store[1] = mmu030_disp_store_1;
		mmu030_fmovem_store[0] = mmu030_fmovem_store_0;
		mmu030_fmovem_store[1] = mmu030_fmovem_store_1;
		mmu030_data_buffer_out = mmu030_data_buffer_out_v;
		mmu030_idx = idxsize;
		mmu030_idx_done = idxsize_done;
		for (int i = 0; i < idxsize_done + 1; i++) {
			mmu030_ad[i].val = mmu030_ad_v[i].val;
		}

		m68k_areg(regs, 7) += 92;

	}

	regs.sr = sr;
	MakeFromSR_T0();
	if (pc & 1) {
		exception3_read_prefetch(0x4E73, pc);
		return;
	}
	m68k_setpci(pc);

	if ((ssw & MMU030_SSW_DF) && (ssw & MMU030_SSW_RM)) {

		// Locked-Read-Modify-Write restarts whole instruction.
		idxsize_done = 0;

	} else if (ssw & MMU030_SSW_DF) {

		// retry faulted access
		uaecptr addr = fault_addr;
		bool read = (ssw & MMU030_SSW_RW) != 0;
		int size = (ssw & MMU030_SSW_SIZE_B) ? sz_byte : ((ssw & MMU030_SSW_SIZE_W) ? sz_word : sz_long);
		int fc = ssw & MMU030_SSW_FC_MASK;
			
#if MMU030_DEBUG
		if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1) {
			if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM2) {
				write_log(_T("68030 MMU MOVEM %04x retry but MMU030_STATEFLAG1_MOVEM2 was already set!?\n"), mmu030_opcode);
			}
		}
#endif

#if MMU030_DEBUG
		write_log(_T("%08x %08x %08x %08x %08x %d %d %d %08x %08x %04x\n"),
			mmu030_state[1], mmu030_state[2], mmu030_disp_store[0], mmu030_disp_store[1],
			addr, read, size, fc, mmu030_data_buffer_out, idxsize < 0 ? -1 : mmu030_ad[idxsize].val, ssw);
#endif

		if (read) {
			if (mmu030_state[1] & MMU030_STATEFLAG1_SUBACCESS0) {
				mmu030_unaligned_read_continue(addr, fc, mmu030_get_generic);
			} else {
				switch (size)
				{
					case sz_byte:
					mmu030_data_buffer_out = uae_mmu030_get_byte_fcx(addr, fc);
					break;
					case sz_word:
					mmu030_data_buffer_out = uae_mmu030_get_word_fcx(addr, fc);
					break;
					case sz_long:
					mmu030_data_buffer_out = uae_mmu030_get_long_fcx(addr, fc);
					break;
				}
			}
			if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1) {
				mmu030_state[1] |= MMU030_STATEFLAG1_MOVEM2;
			} else if (idxsize >= 0) {
				mmu030_ad[mmu030_idx_done].val = mmu030_data_buffer_out;
				mmu030_idx_done++;
			}
		} else {
			if (mmu030_state[1] & MMU030_STATEFLAG1_SUBACCESS0) {
				mmu030_unaligned_write_continue(addr, fc, mmu030_put_generic);
			} else {
				switch (size)
				{
					case sz_byte:
					uae_mmu030_put_byte_fcx(addr, mmu030_data_buffer_out, fc);
					break;
					case sz_word:
					uae_mmu030_put_word_fcx(addr, mmu030_data_buffer_out, fc);
					break;
					case sz_long:
					uae_mmu030_put_long_fcx(addr, mmu030_data_buffer_out, fc);
					break;
				}
			}
			if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1) {
				mmu030_state[1] |= MMU030_STATEFLAG1_MOVEM2;
			} else if (idxsize >= 0) {
				mmu030_idx_done++;
			}
		}

#if MMU030_DEBUG
		if (mmu030_idx >= MAX_MMU030_ACCESS) {
			write_log(_T("mmu030_idx (RTE) out of bounds! %d >= %d\n"), mmu030_idx, MAX_MMU030_ACCESS);
		}
#endif
	}

	if (mmu030_state[1] & MMU030_STATEFLAG1_LASTWRITE) {
		mmu030_retry = false;
	}
}

void flush_mmu030 (uaecptr addr, int n)
{
}

void m68k_do_rts_mmu030 (void)
{
	m68k_setpc (get_long_mmu030_state (m68k_areg (regs, 7)));
	m68k_areg (regs, 7) += 4;
}

void m68k_do_bsr_mmu030 (uaecptr oldpc, uae_s32 offset)
{
	put_long_mmu030_state (m68k_areg (regs, 7) - 4, oldpc);
	m68k_areg (regs, 7) -= 4;
	m68k_incpci (offset);
}

uae_u32 REGPARAM2 get_disp_ea_020_mmu030 (uae_u32 base, int idx)
{
	uae_u16 dp;
	int reg;
	uae_u32 v;
	int oldidx;
	int pcadd = 0;

	// we need to do this hack here because in worst case we don't have enough
	// stack frame space to store two very large 020 addressing mode access state
	// + whatever the instruction itself does.

	if (mmu030_state[1] & (1 << idx)) {
		m68k_incpci (((mmu030_state[2] >> (idx * 4)) & 15) * 2);
		return mmu030_disp_store[idx];
	}

	oldidx = mmu030_idx;
	dp = next_iword_mmu030_state ();
	pcadd += 1;
	
	reg = (dp >> 12) & 15;
	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	regd <<= (dp >> 9) & 3;
	if (dp & 0x100) {
		uae_s32 outer = 0;
		if (dp & 0x80)
			base = 0;
		if (dp & 0x40)
			regd = 0;

		if ((dp & 0x30) == 0x20) {
			base += (uae_s32)(uae_s16) next_iword_mmu030_state ();
			pcadd += 1;
		}
		if ((dp & 0x30) == 0x30) {
			base += next_ilong_mmu030_state ();
			pcadd += 2;
		}

		if ((dp & 0x3) == 0x2) {
			outer = (uae_s32)(uae_s16) next_iword_mmu030_state ();
			pcadd += 1;
		}
		if ((dp & 0x3) == 0x3) {
			outer = next_ilong_mmu030_state ();
			pcadd += 2;
		}

		if ((dp & 0x4) == 0) {
			base += regd;
		}
		if (dp & 0x3) {
			base = get_long_mmu030_state (base);
		}
		if (dp & 0x4) {
			base += regd;
		}
		v = base + outer;
	} else {
		v = base + (uae_s32)((uae_s8)dp) + regd;
	}

	mmu030_state[1] |= 1 << idx;
	mmu030_state[2] |= pcadd << (idx * 4);
	mmu030_disp_store[idx] = v;
	mmu030_idx = oldidx;
	mmu030_idx_done = oldidx;

	return v;
}

// cache

void m68k_do_rts_mmu030c (void)
{
	m68k_setpc (get_long_mmu030c_state (m68k_areg (regs, 7)));
	m68k_areg (regs, 7) += 4;
}

void m68k_do_bsr_mmu030c (uaecptr oldpc, uae_s32 offset)
{
	put_long_mmu030c_state (m68k_areg (regs, 7) - 4, oldpc);
	m68k_areg (regs, 7) -= 4;
	m68k_incpci (offset);
}


uae_u32 REGPARAM2 get_disp_ea_020_mmu030c (uae_u32 base, int idx)
{
	uae_u16 dp;
	int reg;
	uae_u32 v;
	int oldidx;
	int pcadd = 0;

	// we need to do this hack here because in worst case we don't have enough
	// stack frame space to store two very large 020 addressing mode access state
	// + whatever the instruction itself does.

	if (mmu030_state[1] & (1 << idx)) {
		m68k_incpci(((mmu030_state[2] >> (idx * 4)) & 15) * 2);
		return mmu030_disp_store[idx];
	}

	oldidx = mmu030_idx;
	dp = next_iword_mmu030c_state();
	pcadd += 1;
	
	reg = (dp >> 12) & 15;
	uae_s32 regd = regs.regs[reg];
	if ((dp & 0x800) == 0)
		regd = (uae_s32)(uae_s16)regd;
	regd <<= (dp >> 9) & 3;
	if (dp & 0x100) {
		uae_s32 outer = 0;
		if (dp & 0x80)
			base = 0;
		if (dp & 0x40)
			regd = 0;

		if ((dp & 0x30) == 0x20) {
			base += (uae_s32)(uae_s16) next_iword_mmu030c_state();
			pcadd += 1;
		}
		if ((dp & 0x30) == 0x30) {
			base += next_ilong_mmu030c_state();
			pcadd += 2;
		}

		if ((dp & 0x3) == 0x2) {
			outer = (uae_s32)(uae_s16) next_iword_mmu030c_state();
			pcadd += 1;
		}
		if ((dp & 0x3) == 0x3) {
			outer = next_ilong_mmu030c_state();
			pcadd += 2;
		}

		if ((dp & 0x4) == 0) {
			base += regd;
		}
		if (dp & 0x3) {
			base = get_long_mmu030c_state(base);
		}
		if (dp & 0x4) {
			base += regd;
		}
		v = base + outer;
	} else {
		v = base + (uae_s32)((uae_s8)dp) + regd;
	}

	mmu030_state[1] |= 1 << idx;
	mmu030_state[2] |= pcadd << (idx * 4);
	mmu030_disp_store[idx] = v;
	mmu030_idx = oldidx;
	mmu030_idx_done = oldidx;

	return v;
}

void m68k_do_rte_mmu030c (uaecptr a7)
{
	struct mmu030_access mmu030_ad_v[MAX_MMU030_ACCESS + 1];

	// Restore access error exception state

	uae_u16 sr = get_word_mmu030c(a7);
	uae_u32 pc = get_long_mmu030c(a7 + 2);
	uae_u16 format = get_word_mmu030c(a7 + 6);
	uae_u16 frame = format >> 12;
	uae_u16 ssw = get_word_mmu030c(a7 + 10);
	uae_u32 fault_addr = get_long_mmu030c(a7 + 16);
	// Data output buffer
	uae_u32 mmu030_data_buffer_out_v = get_long_mmu030c(a7 + 0x18);
	// Internal register, our opcode storage area
	uae_u32 oc = get_long_mmu030c(a7 + 0x14);
	uae_u32 stagesbc = get_long_mmu030c(a7 + 12);

	int idxsize = -1, idxsize_done = -1;
	bool doprefetch = true;

	// Fetch last word, real CPU does it to allow OS bus handler to map
	// the page if frame crosses pages and following page is not resident.
	if (frame == 0xb)
		get_word_mmu030c(a7 + 92 - 2);
	else
		get_word_mmu030c(a7 + 32 - 2);

	// Rerun "mmu030_opcode" using restored state.
	mmu030_retry = true;

	if (frame == 0xa) {
		// this is always last write data write fault

		uae_u32 mmu030_state_1 = get_word_mmu030c(a7 + 0x8);
		uae_u32 ps = get_long_mmu030c(a7 + 0x1c);

#if MMU030_DEBUG
		if (!(mmu030_state_1 & MMU030_STATEFLAG1_LASTWRITE)) {
			write_log(_T("68030 MMU short bus fault but no lastwrite set!?\n"));
		}
		if (ssw & (MMU030_SSW_FB | MMU030_SSW_FC | MMU030_SSW_FB | MMU030_SSW_RC | MMU030_SSW_RB)) {
			write_log(_T("68030 MMU short bus fault and pipeline fault?\n"));
		}
		if (ssw & MMU030_SSW_RW) {
			write_log(_T("68030 MMU short bus fault but read fault!?\n"));
		}
#endif
		// did we have data fault but DF bit cleared?
		if (ssw & (MMU030_SSW_DF << 1) && !(ssw & MMU030_SSW_DF)) {
			// DF not set: mark access as done
			unalign_clear();
		}

		regs.prefetch020_valid[0] = (ps & 1) ? 1 : 0;
		regs.prefetch020_valid[1] = (ps & 2) ? 1 : 0;
		regs.prefetch020_valid[2] = (ps & 4) ? 1 : 0;

		regs.pipeline_r8[0] = (ps >> 8) & 7;
		regs.pipeline_r8[1] = (ps >> 11) & 7;
		regs.pipeline_pos = (ps >> 16) & 15;
		regs.pipeline_stop = ((ps >> 20) & 15) == 15 ? -1 : (int)(ps >> 20) & 15;

		regs.prefetch020[2] = stagesbc;
		regs.prefetch020[1] = stagesbc >> 16;
		regs.prefetch020[0] = oc >> 16;
		mmu030_opcode_stageb = (uae_u16)oc;

		mmu030_data_buffer_out = mmu030_data_buffer_out_v;
		mmu030_state[0] = 0;
		mmu030_state[1] = mmu030_state_1;
		mmu030_state[2] = 0;
		mmu030_idx = mmu030_idx_done = 0;

		doprefetch = false;

		m68k_areg(regs, 7) += 32;

	} else if (frame == 0xb) {

		// get_disp_ea_020
		uae_u32 mmu030_disp_store_0 = get_long_mmu030c(a7 + 0x1c);
		uae_u32 mmu030_disp_store_1 = get_long_mmu030c(a7 + 0x1c + 4);
		// Internal register, misc flags
		uae_u32 ps = get_long_mmu030c(a7 + 0x28);
		// Data buffer
		uae_u32 mmu030_data_buffer_in_v = get_long_mmu030c(a7 + 0x2c);;
		// Misc state data
		uae_u32 mmu030_state_0 = get_word_mmu030c(a7 + 0x30);
		uae_u32 mmu030_state_1 = get_word_mmu030c(a7 + 0x32);
		uae_u32 mmu030_state_2 = get_word_mmu030c(a7 + 0x34);

		uae_u32 mmu030_opcode_v = (ps & 0x80000000) ? 0xffffffff : (oc & 0xffff);

		uae_u32 mmu030_fmovem_store_0 = 0;
		uae_u32 mmu030_fmovem_store_1 = 0;
		if (mmu030_state[1] & MMU030_STATEFLAG1_FMOVEM) {
			mmu030_fmovem_store_0 = get_long_mmu030c(a7 + 0x5c - (7 + 1) * 4);
			mmu030_fmovem_store_1 = get_long_mmu030c(a7 + 0x5c - (8 + 1) * 4);
		}

		uae_u16 v = get_word_mmu030c(a7 + 0x36);
		idxsize = v & 0x0f;
		idxsize_done = (v >> 4) & 0x0f;
		for (int i = 0; i < idxsize_done + 1; i++) {
			mmu030_ad_v[i].val = get_long_mmu030c(a7 + 0x5c - (i + 1) * 4);
		}

		regs.wb2_status = v >> 8;
		regs.wb3_status = mmu030_state_2 >> 8;
		mmu030fixupmod(regs.wb2_status, 1, -1);
		mmu030fixupmod(regs.wb3_status, 1, -1);

		// did we have data fault but DF bit cleared?
		if (ssw & (MMU030_SSW_DF << 1) && !(ssw & MMU030_SSW_DF)) {
			// DF not set: mark access as done
			mmu030_data_buffer_out_v = mmu030_data_buffer_in_v;
			if (ssw & MMU030_SSW_RM) {
				// Read-Modify-Write: whole instruction is considered done
				write_log(_T("Read-Modify-Write and DF bit cleared! PC=%08x\n"), regs.instruction_pc);
				mmu030_retry = false;
			} else if (mmu030_state_1 & MMU030_STATEFLAG1_MOVEM1) {
				// if movem, skip next move
				mmu030_state_1 |= MMU030_STATEFLAG1_MOVEM2;
			} else {
				if (ssw & MMU030_SSW_RW) {
					// Read and no DF: use value in data input buffer
					mmu030_ad_v[idxsize_done].val = mmu030_data_buffer_in_v;
				}
				idxsize_done++;
			}
			unalign_clear();
		}

#if MMU030_DEBUG
		if (mmu030_state_1 & MMU030_STATEFLAG1_LASTWRITE) {
			write_log(_T("68030 MMU long bus fault but lastwrite set!?\n"));
		}
#endif
		// Retried data access is the only memory access that can be done after this.

		regs.prefetch020_valid[0] = (ps & 1) ? 1 : 0;
		regs.prefetch020_valid[1] = (ps & 2) ? 1 : 0;
		regs.prefetch020_valid[2] = (ps & 4) ? 1 : 0;
		regs.pipeline_r8[0] = (ps >> 8) & 7;
		regs.pipeline_r8[1] = (ps >> 11) & 7;
		regs.pipeline_pos = (ps >> 16) & 15;
		regs.pipeline_stop = ((ps >> 20) & 15) == 15 ? -1 : (int)(ps >> 20) & 15;

		regs.prefetch020[2] = stagesbc;
		regs.prefetch020[1] = stagesbc >> 16;
		regs.prefetch020[0] = oc >> 16;

		if ((ssw & MMU030_SSW_FB) && !(ssw & MMU030_SSW_RB)) {
			regs.prefetch020_valid[2] = 1;
			write_log(_T("Software fixed stage B! opcode = %04x\n"), regs.prefetch020[2]);
#if 0
			if (!regs.prefetch020_valid[0]) {
				regs.prefetch020[0] = regs.prefetch020[1];
				regs.prefetch020[1] = regs.prefetch020[2];
				regs.prefetch020_valid[0] = regs.prefetch020_valid[1];
				regs.prefetch020_valid[1] = regs.prefetch020_valid[2];
				regs.prefetch020_valid[2] = 0;
			}
#endif
		}
		if ((ssw & MMU030_SSW_FC) && !(ssw & MMU030_SSW_RC)) {
			regs.prefetch020_valid[1] = 1;
			write_log(_T("Software fixed stage C! opcode = %04x\n"), regs.prefetch020[1]);
		}

		// restore global state variables
		mmu030_opcode = mmu030_opcode_v;
		mmu030_state[0] = mmu030_state_0;
		mmu030_state[1] = mmu030_state_1;
		mmu030_state[2] = mmu030_state_2;
		mmu030_disp_store[0] = mmu030_disp_store_0;
		mmu030_disp_store[1] = mmu030_disp_store_1;
		mmu030_fmovem_store[0] = mmu030_fmovem_store_0;
		mmu030_fmovem_store[1] = mmu030_fmovem_store_1;
		mmu030_data_buffer_out = mmu030_data_buffer_out_v;
		mmu030_idx = idxsize;
		mmu030_idx_done = idxsize_done;
		for (int i = 0; i < idxsize_done + 1; i++) {
			mmu030_ad[i].val = mmu030_ad_v[i].val;
		}

		m68k_areg(regs, 7) += 92;

	}

	regs.sr = sr;
	MakeFromSR_T0();
	if (pc & 1) {
		exception3_read_prefetch(0x4E73, pc);
		return;
	}
	m68k_setpci (pc);

	if (!(ssw & (MMU030_SSW_DF << 1))) {
		// software fixed?
		if (((ssw & MMU030_SSW_FB) && !(ssw & MMU030_SSW_RB)) || ((ssw & MMU030_SSW_FC) && !(ssw & MMU030_SSW_RC))) {
			fill_prefetch_030_ntx_continue();
		} else {
			// pipeline refill in progress?
			if (mmu030_opcode == -1) {
#if MMU030_ALWAYS_FULL_PREFETCH
				fill_prefetch_030_ntx();
#else
				fill_prefetch_030_ntx_continue();
#endif
			}
		}
	}

	if ((ssw & MMU030_SSW_DF) && (ssw & MMU030_SSW_RM)) {

		// Locked-Read-Modify-Write restarts whole instruction.
		mmu030_idx_done = 0;

	} else if (ssw & MMU030_SSW_DF) {
		// retry faulted access
		uaecptr addr = fault_addr;
		bool read = (ssw & MMU030_SSW_RW) != 0;
		int size = (ssw & MMU030_SSW_SIZE_B) ? sz_byte : ((ssw & MMU030_SSW_SIZE_W) ? sz_word : sz_long);
		int fc = ssw & 7;

#if MMU030_DEBUG
		if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1) {
			if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM2) {
				write_log(_T("68030 MMU MOVEM %04x retry but MMU030_STATEFLAG1_MOVEM2 was already set!?\n"), mmu030_opcode);
			}
		}
#endif
		if (read) {
			if (mmu030_state[1] & MMU030_STATEFLAG1_SUBACCESS0) {
				mmu030_unaligned_read_continue(addr, fc, read_dcache030_retry);
			} else {
				switch (size)
				{
					case sz_byte:
					mmu030_data_buffer_out = read_data_030_fc_bget(addr, fc);
					break;
					case sz_word:
					mmu030_data_buffer_out = read_data_030_fc_wget(addr, fc);
					break;
					case sz_long:
					mmu030_data_buffer_out = read_data_030_fc_lget(addr, fc);
					break;
				}
			}
			if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1) {
				mmu030_state[1] |= MMU030_STATEFLAG1_MOVEM2;
			} else if (idxsize >= 0) {
				mmu030_ad[mmu030_idx_done].val = mmu030_data_buffer_out;
				mmu030_idx_done++;
			}
		} else {
			if (mmu030_state[1] & MMU030_STATEFLAG1_SUBACCESS0) {
				mmu030_unaligned_write_continue(addr, fc, write_dcache030_retry);
			} else {
				switch (size)
				{
					case sz_byte:
					write_data_030_fc_bput(addr, mmu030_data_buffer_out, fc);
					break;
					case sz_word:
					write_data_030_fc_wput(addr, mmu030_data_buffer_out, fc);
					break;
					case sz_long:
					write_data_030_fc_lput(addr, mmu030_data_buffer_out, fc);
					break;
				}
			}
			if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM1) {
				mmu030_state[1] |= MMU030_STATEFLAG1_MOVEM2;
			} else if (idxsize >= 0) {
				mmu030_idx_done++;
			}
		}
	}

	if (mmu030_state[1] & MMU030_STATEFLAG1_LASTWRITE) {
		mmu030_retry = false;
		if (doprefetch) {
			mmu030_opcode = -1;
			fill_prefetch_030_ntx();
		}
	}
}
