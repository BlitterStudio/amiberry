#ifndef UAE_CPUMMU030_H
#define UAE_CPUMMU030_H

#define MMU030_DEBUG 0

#include "uae/types.h"

#include "mmu_common.h"

#define MMU_DPAGECACHE030 1
#define MMU_IPAGECACHE030 1

extern uae_u64 srp_030, crp_030;
extern uae_u32 tt0_030, tt1_030, tc_030;
extern uae_u16 mmusr_030;

#define MAX_MMU030_ACCESS 9
extern uae_u32 mm030_stageb_address;
extern int mmu030_idx, mmu030_idx_done;
extern bool mmu030_retry;
extern int mmu030_opcode, mmu030_opcode_stageb;
extern int mmu030_fake_prefetch;
extern uaecptr mmu030_fake_prefetch_addr;
extern uae_u16 mmu030_state[3];
extern uae_u32 mmu030_data_buffer_out;
extern uae_u32 mmu030_disp_store[2];
extern uae_u32 mmu030_fmovem_store[2];
extern uae_u8 mmu030_cache_state, mmu030_cache_state_default;
extern bool ismoves030, islrmw030;

#define MMU030_STATEFLAG1_FMOVEM 0x2000
#define MMU030_STATEFLAG1_MOVEM1 0x4000
#define MMU030_STATEFLAG1_MOVEM2 0x8000

#define MMU030_STATEFLAG1_DISP0 0x0001
#define MMU030_STATEFLAG1_DISP1 0x0002

#define MMU030_STATEFLAG1_LASTWRITE 0x0100

#define MMU030_STATEFLAG1_SUBACCESS0 0x0004
#define MMU030_STATEFLAG1_SUBACCESS1 0x0008
#define MMU030_STATEFLAG1_SUBACCESS2 0x0010
#define MMU030_STATEFLAG1_SUBACCESS3 0x0020
#define MMU030_STATEFLAG1_SUBACCESSX 0x0040
#define MMU030_STATEFLAG1_SUBACCESSL 0x0080
#define MMU030_STATEFLAG1_SUBACCESS_SHIFT 2

struct mmu030_access
{
	uae_u32 val;
};
extern struct mmu030_access mmu030_ad[MAX_MMU030_ACCESS + 1];

void mmu030_page_fault(uaecptr addr, bool read, int flags, uae_u32 fc);

int mmu_op30_pmove(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra);
bool mmu_op30_ptest(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra);
bool mmu_op30_pflush(uaecptr pc, uae_u32 opcode, uae_u16 next, uaecptr extra);

typedef struct {
    uae_u32 addr_base;
    uae_u32 addr_mask;
    uae_u32 fc_base;
    uae_u32 fc_mask;
} TT_info;

bool mmu030_decode_tc(uae_u32 TC, bool);
bool mmu030_decode_rp(uae_u64 RP);

void mmu030_flush_atc_all(void);
void mmu030_reset(int hardreset);
void mmu030_set_funcs(void);
uaecptr mmu030_translate(uaecptr addr, bool super, bool data, bool write);
void mmu030_hardware_bus_error(uaecptr addr, uae_u32 v, bool read, bool ins, int size);
bool mmu030_is_super_access(bool read);

void mmu030_put_long(uaecptr addr, uae_u32 val, uae_u32 fc);
void mmu030_put_word(uaecptr addr, uae_u16 val, uae_u32 fc);
void mmu030_put_byte(uaecptr addr, uae_u8  val, uae_u32 fc);
uae_u32 mmu030_get_long(uaecptr addr, uae_u32 fc);
uae_u16 mmu030_get_word(uaecptr addr, uae_u32 fc);
uae_u8  mmu030_get_byte(uaecptr addr, uae_u32 fc);
uae_u32 mmu030_get_ilong(uaecptr addr, uae_u32 fc);
uae_u16 mmu030_get_iword(uaecptr addr, uae_u32 fc);

uae_u32 uae_mmu030_get_lrmw(uaecptr addr, int size);
void uae_mmu030_put_lrmw(uaecptr addr, uae_u32 val, int size);

void mmu030_put_generic(uaecptr addr, uae_u32 val, uae_u32 fc, int size, int flags);
uae_u32 mmu030_get_generic(uaecptr addr, uae_u32 fc, int size, int flags);

extern uae_u16 REGPARAM3 mmu030_get_word_unaligned(uaecptr addr, uae_u32 fc, int flags) REGPARAM;
extern uae_u32 REGPARAM3 mmu030_get_long_unaligned(uaecptr addr, uae_u32 fc, int flags) REGPARAM;
extern uae_u32 REGPARAM3 mmu030_get_ilong_unaligned(uaecptr addr, uae_u32 fc, int flags) REGPARAM;
extern uae_u16 REGPARAM3 mmu030_get_lrmw_word_unaligned(uaecptr addr, uae_u32 fc, int flags) REGPARAM;
extern uae_u32 REGPARAM3 mmu030_get_lrmw_long_unaligned(uaecptr addr, uae_u32 fc, int flags) REGPARAM;
extern void REGPARAM3 mmu030_put_word_unaligned(uaecptr addr, uae_u16 val, uae_u32 fc, int flags) REGPARAM;
extern void REGPARAM3 mmu030_put_long_unaligned(uaecptr addr, uae_u32 val, uae_u32 fc, int flags) REGPARAM;

static ALWAYS_INLINE uae_u32 uae_mmu030_get_fc_code(void)
{
	return (regs.s ? 4 : 0) | 2;
}

static ALWAYS_INLINE uae_u32 uae_mmu030_get_fc_data(void)
{
	return (regs.s ? 4 : 0) | 1;
}

static ALWAYS_INLINE uae_u32 uae_mmu030_get_ilong(uaecptr addr)
{
	uae_u32 fc = uae_mmu030_get_fc_code();

	if (unlikely(is_unaligned_bus(addr, 4)))
		return mmu030_get_ilong_unaligned(addr, fc, 0);
	return mmu030_get_ilong(addr, fc);
}
static ALWAYS_INLINE uae_u16 uae_mmu030_get_iword(uaecptr addr)
{
	uae_u32 fc = uae_mmu030_get_fc_code();

	return mmu030_get_iword(addr, fc);
}
static ALWAYS_INLINE uae_u16 uae_mmu030_get_ibyte(uaecptr addr)
{
	uae_u32 fc = uae_mmu030_get_fc_code();

	return mmu030_get_byte(addr, fc);
}

static ALWAYS_INLINE uae_u32 uae_mmu030_get_long(uaecptr addr)
{
	uae_u32 fc = uae_mmu030_get_fc_data();

	if (unlikely(is_unaligned_bus(addr, 4)))
		return mmu030_get_long_unaligned(addr, fc, 0);
	return mmu030_get_long(addr, fc);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_word(uaecptr addr)
{
	uae_u32 fc = uae_mmu030_get_fc_data();

	if (unlikely(is_unaligned_bus(addr, 2)))
		return mmu030_get_word_unaligned(addr, fc, 0);
	return mmu030_get_word(addr, fc);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_byte(uaecptr addr)
{
	uae_u32 fc = uae_mmu030_get_fc_data();

	return mmu030_get_byte(addr, fc);
}

static ALWAYS_INLINE void uae_mmu030_put_long(uaecptr addr, uae_u32 val)
{
	uae_u32 fc = uae_mmu030_get_fc_data();

	if (unlikely(is_unaligned_bus(addr, 4)))
		mmu030_put_long_unaligned(addr, val, fc, 0);
	else
		mmu030_put_long(addr, val, fc);
}
static ALWAYS_INLINE void uae_mmu030_put_word(uaecptr addr, uae_u32 val)
{
	uae_u32 fc = uae_mmu030_get_fc_data();

	if (unlikely(is_unaligned_bus(addr, 2)))
		mmu030_put_word_unaligned(addr, val, fc, 0);
	else
		mmu030_put_word(addr, val, fc);
}
static ALWAYS_INLINE void uae_mmu030_put_byte(uaecptr addr, uae_u32 val)
{
	uae_u32 fc = uae_mmu030_get_fc_data();

	mmu030_put_byte(addr, val, fc);
}

static ALWAYS_INLINE uae_u32 uae_mmu030_get_ilong_fc(uaecptr addr)
{
	if (unlikely(is_unaligned_bus(addr, 4)))
		return mmu030_get_ilong_unaligned(addr, regs.fc030, 0);
	return mmu030_get_ilong(addr, regs.fc030);
}
static ALWAYS_INLINE uae_u16 uae_mmu030_get_iword_fc(uaecptr addr)
{
	return mmu030_get_iword(addr, regs.fc030);
}
static ALWAYS_INLINE uae_u16 uae_mmu030_get_ibyte_fc(uaecptr addr)
{
	return mmu030_get_byte(addr, regs.fc030);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_long_fc(uaecptr addr)
{
	if (unlikely(is_unaligned_bus(addr, 4)))
		return mmu030_get_long_unaligned(addr, regs.fc030, 0);
	return mmu030_get_long(addr, regs.fc030);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_word_fc(uaecptr addr)
{
	if (unlikely(is_unaligned_bus(addr, 2)))
		return mmu030_get_word_unaligned(addr, regs.fc030, 0);
	return mmu030_get_word(addr, regs.fc030);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_byte_fc(uaecptr addr)
{
	return mmu030_get_byte(addr, regs.fc030);
}
static ALWAYS_INLINE void uae_mmu030_put_long_fc(uaecptr addr, uae_u32 val)
{
	if (unlikely(is_unaligned_bus(addr, 4)))
		mmu030_put_long_unaligned(addr, val, regs.fc030, 0);
	else
		mmu030_put_long(addr, val, regs.fc030);
}
static ALWAYS_INLINE void uae_mmu030_put_word_fc(uaecptr addr, uae_u32 val)
{
	if (unlikely(is_unaligned_bus(addr, 2)))
		mmu030_put_word_unaligned(addr, val,  regs.fc030, 0);
	else
		mmu030_put_word(addr, val, regs.fc030);
}
static ALWAYS_INLINE void uae_mmu030_put_byte_fc(uaecptr addr, uae_u32 val)
{
	mmu030_put_byte(addr, val, regs.fc030);
}
uae_u8 uae_mmu030_check_fc(uaecptr addr, bool write, uae_u32 size);

static ALWAYS_INLINE uae_u32 uae_mmu030_get_long_fcx(uaecptr addr, int fc)
{
	if (unlikely(is_unaligned_bus(addr, 4)))
		return mmu030_get_long_unaligned(addr, fc, 0);
	return mmu030_get_long(addr, fc);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_word_fcx(uaecptr addr, int fc)
{
	if (unlikely(is_unaligned_bus(addr, 2)))
		return mmu030_get_word_unaligned(addr, fc, 0);
	return mmu030_get_word(addr, fc);
}
static ALWAYS_INLINE uae_u32 uae_mmu030_get_byte_fcx(uaecptr addr, int fc)
{
	return mmu030_get_byte(addr, fc);
}
static ALWAYS_INLINE void uae_mmu030_put_long_fcx(uaecptr addr, uae_u32 val, int fc)
{
	if (unlikely(is_unaligned_bus(addr, 4)))
		mmu030_put_long_unaligned(addr, val, fc, 0);
	else
		mmu030_put_long(addr, val, fc);
}
static ALWAYS_INLINE void uae_mmu030_put_word_fcx(uaecptr addr, uae_u32 val, int fc)
{
	if (unlikely(is_unaligned_bus(addr, 2)))
		mmu030_put_word_unaligned(addr, val, fc, 0);
	else
		mmu030_put_word(addr, val, fc);
}
static ALWAYS_INLINE void uae_mmu030_put_byte_fcx(uaecptr addr, uae_u32 val, int fc)
{
	mmu030_put_byte(addr, val, fc);
}

#define ACCESS_CHECK_PUT \
	if (mmu030_idx++ < mmu030_idx_done) { \
		return; \
	} else { \
		mmu030_data_buffer_out = v; \
	}

#define ACCESS_CHECK_GET \
	if (mmu030_idx++ < mmu030_idx_done) { \
		v = mmu030_ad[mmu030_idx - 1].val; \
		return v; \
	}

#define ACCESS_CHECK_GET_PC(pc) \
	if (mmu030_idx++ < mmu030_idx_done) { \
		v = mmu030_ad[mmu030_idx - 1].val; \
		m68k_incpci(pc); \
		return v; \
	}

#define ACCESS_EXIT_PUT \
	mmu030_ad[mmu030_idx_done++].val = mmu030_data_buffer_out;

#define ACCESS_EXIT_GET \
	mmu030_ad[mmu030_idx_done++].val = v;

STATIC_INLINE uae_u32 state_store_mmu030(uae_u32 v)
{
	if (mmu030_idx++ < mmu030_idx_done) {
		v = mmu030_ad[mmu030_idx - 1].val;
	} else {
		mmu030_ad[mmu030_idx_done++].val = v;
	}
	return v;
}

// non-cache

static ALWAYS_INLINE uae_u32 sfc030_get_long(uaecptr addr)
{
    uae_u32 fc = regs.sfc;
#if MMUDEBUG > 2
	write_log(_T("sfc030_get_long: FC = %i\n"),fc);
#endif
	if (unlikely(is_unaligned_bus(addr, 4)))
		return mmu030_get_long_unaligned(addr, fc, 0);
	return mmu030_get_long(addr, fc);
}

static ALWAYS_INLINE uae_u16 sfc030_get_word(uaecptr addr)
{
    uae_u32 fc = regs.sfc;
#if MMUDEBUG > 2
	write_log(_T("sfc030_get_word: FC = %i\n"),fc);
#endif
    if (unlikely(is_unaligned_bus(addr, 2)))
		return mmu030_get_word_unaligned(addr, fc, 0);
	return mmu030_get_word(addr, fc);
}

static ALWAYS_INLINE uae_u8 sfc030_get_byte(uaecptr addr)
{
    uae_u32 fc = regs.sfc;
#if MMUDEBUG > 2
	write_log(_T("sfc030_get_byte: FC = %i\n"),fc);
#endif
	return mmu030_get_byte(addr, fc);
}

static ALWAYS_INLINE void dfc030_put_long(uaecptr addr, uae_u32 val)
{
    uae_u32 fc = regs.dfc;
#if MMUDEBUG > 2
	write_log(_T("dfc030_put_long: %08X = %08X FC = %i\n"), addr, val, fc);
#endif
    if (unlikely(is_unaligned_bus(addr, 4)))
		mmu030_put_long_unaligned(addr, val, fc, 0);
	else
		mmu030_put_long(addr, val, fc);
}

static ALWAYS_INLINE void dfc030_put_word(uaecptr addr, uae_u16 val)
{
    uae_u32 fc = regs.dfc;
#if MMUDEBUG > 2
	write_log(_T("dfc030_put_word: %08X = %04X FC = %i\n"), addr, val, fc);
#endif
	if (unlikely(is_unaligned_bus(addr, 2)))
		mmu030_put_word_unaligned(addr, val, fc, 0);
	else
		mmu030_put_word(addr, val, fc);
}

static ALWAYS_INLINE void dfc030_put_byte(uaecptr addr, uae_u8 val)
{
    uae_u32 fc = regs.dfc;
#if MMUDEBUG > 2
	write_log(_T("dfc030_put_byte: %08X = %02X FC = %i\n"), addr, val, fc);
#endif
	mmu030_put_byte(addr, val, fc);
}

static ALWAYS_INLINE uae_u32 sfc030_get_long_state(uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
	ismoves030 = true;
	v = sfc030_get_long(addr);
	ismoves030 = false;
	ACCESS_EXIT_GET
	return v;
}
static ALWAYS_INLINE uae_u16 sfc030_get_word_state(uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
	ismoves030 = true;
	v = sfc030_get_word(addr);
	ismoves030 = false;
	ACCESS_EXIT_GET
	return v;
}
static ALWAYS_INLINE uae_u8 sfc030_get_byte_state(uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
	ismoves030 = true;
	v = sfc030_get_byte(addr);
	ismoves030 = false;
	ACCESS_EXIT_GET
	return v;
}

static ALWAYS_INLINE void dfc030_put_long_state(uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	ismoves030 = true;
	dfc030_put_long(addr, v);
	ismoves030 = false;
	ACCESS_EXIT_PUT
}
static ALWAYS_INLINE void dfc030_put_word_state(uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	ismoves030 = true;
	dfc030_put_word(addr, v);
	ismoves030 = false;
	ACCESS_EXIT_PUT
}
static ALWAYS_INLINE void dfc030_put_byte_state(uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	ismoves030 = true;
	dfc030_put_byte(addr, v);
	ismoves030 = false;
	ACCESS_EXIT_PUT
}


uae_u32 REGPARAM3 get_disp_ea_020_mmu030 (uae_u32 base, int idx) REGPARAM;

STATIC_INLINE void put_byte_mmu030_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
    uae_mmu030_put_byte (addr, v);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_lrmw_byte_mmu030_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
    uae_mmu030_put_lrmw (addr, v, sz_byte);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_word_mmu030_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
    uae_mmu030_put_word (addr, v);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_lrmw_word_mmu030_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
    uae_mmu030_put_lrmw (addr, v, sz_word);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_long_mmu030_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
    uae_mmu030_put_long (addr, v);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_lrmw_long_mmu030_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
    uae_mmu030_put_lrmw (addr, v, sz_long);
	ACCESS_EXIT_PUT
}

STATIC_INLINE uae_u32 get_byte_mmu030_state (uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_byte (addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_lrmw_byte_mmu030_state (uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_lrmw (addr, sz_byte);
	ACCESS_EXIT_GET
	return v;
}

STATIC_INLINE uae_u32 get_word_mmu030_state (uaecptr addr)
{
 	uae_u32 v;
	ACCESS_CHECK_GET
	v = uae_mmu030_get_word (addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_lrmw_word_mmu030_state (uaecptr addr)
{
 	uae_u32 v;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_lrmw (addr, sz_word);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_long_mmu030_state (uaecptr addr)
{
 	uae_u32 v;
	ACCESS_CHECK_GET
	v = uae_mmu030_get_long (addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_lrmw_long_mmu030_state (uaecptr addr)
{
 	uae_u32 v;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_lrmw (addr, sz_long);
	ACCESS_EXIT_GET
	return v;
}

STATIC_INLINE uae_u32 get_ibyte_mmu030_state (int o)
{
 	uae_u32 v;
    uae_u32 addr = m68k_getpci () + o;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_iword (addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_iword_mmu030_state (int o)
{
 	uae_u32 v;
    uae_u32 addr = m68k_getpci () + o;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_iword (addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_ilong_mmu030_state (int o)
{
 	uae_u32 v;
    uae_u32 addr = m68k_getpci () + o;
	ACCESS_CHECK_GET
    v = uae_mmu030_get_ilong(addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 next_iword_mmu030_state (void)
{
 	uae_u32 v;
    uae_u32 addr = m68k_getpci ();
	ACCESS_CHECK_GET_PC(2);
    v = uae_mmu030_get_iword (addr);
    m68k_incpci (2);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 next_ilong_mmu030_state (void)
{
 	uae_u32 v;
    uae_u32 addr = m68k_getpci ();
	ACCESS_CHECK_GET_PC(4);
    v = uae_mmu030_get_ilong(addr);
    m68k_incpci (4);
	ACCESS_EXIT_GET
	return v;
}

STATIC_INLINE uae_u32 get_byte_mmu030 (uaecptr addr)
{
	return uae_mmu030_get_byte (addr);
}
STATIC_INLINE uae_u32 get_word_mmu030 (uaecptr addr)
{
	return uae_mmu030_get_word (addr);
}
STATIC_INLINE uae_u32 get_long_mmu030 (uaecptr addr)
{
	return uae_mmu030_get_long (addr);
}
STATIC_INLINE void put_byte_mmu030 (uaecptr addr, uae_u32 v)
{
    uae_mmu030_put_byte (addr, v);
}
STATIC_INLINE void put_word_mmu030 (uaecptr addr, uae_u32 v)
{
    uae_mmu030_put_word (addr, v);
}
STATIC_INLINE void put_long_mmu030 (uaecptr addr, uae_u32 v)
{
    uae_mmu030_put_long (addr, v);
}

STATIC_INLINE uae_u32 get_ibyte_mmu030 (int o)
{
    uae_u32 pc = m68k_getpci () + o;
    return uae_mmu030_get_iword (pc);
}
STATIC_INLINE uae_u32 get_iword_mmu030 (int o)
{
    uae_u32 pc = m68k_getpci () + o;
    return uae_mmu030_get_iword (pc);
}
STATIC_INLINE uae_u32 get_ilong_mmu030 (int o)
{
    uae_u32 pc = m68k_getpci () + o;
    return uae_mmu030_get_ilong (pc);
}
STATIC_INLINE uae_u32 next_iword_mmu030 (void)
{
 	uae_u32 v;
    uae_u32 pc = m68k_getpci ();
    v = uae_mmu030_get_iword (pc);
    m68k_incpci (2);
	return v;
}
STATIC_INLINE uae_u32 next_ilong_mmu030 (void)
{
 	uae_u32 v;
    uae_u32 pc = m68k_getpci ();
    v = uae_mmu030_get_ilong (pc);
    m68k_incpci (4);
	return v;
}

extern void m68k_do_rts_mmu030 (void);
extern void m68k_do_rte_mmu030 (uaecptr a7);
extern void flush_mmu030 (uaecptr, int);
extern void m68k_do_bsr_mmu030 (uaecptr oldpc, uae_s32 offset);

// more compatible + optional cache

static ALWAYS_INLINE uae_u32 mmu030_get_fc_byte(uaecptr addr, uae_u32 fc)
{
	return mmu030_get_byte(addr, fc);
}
static ALWAYS_INLINE uae_u32 mmu030_get_fc_word(uaecptr addr, uae_u32 fc)
{
	return mmu030_get_word(addr, fc);
}
static ALWAYS_INLINE uae_u32 mmu030_get_fc_long(uaecptr addr, uae_u32 fc)
{
	return mmu030_get_long(addr, fc);
}
static ALWAYS_INLINE void mmu030_put_fc_byte(uaecptr addr, uae_u32 val, uae_u32 fc)
{
	mmu030_put_byte(addr, val, fc);
}
static ALWAYS_INLINE void mmu030_put_fc_word(uaecptr addr, uae_u32 val, uae_u32 fc)
{
	mmu030_put_word(addr, val, fc);
}
static ALWAYS_INLINE void mmu030_put_fc_long(uaecptr addr, uae_u32 val, uae_u32 fc)
{
	mmu030_put_long(addr, val, fc);
}

static ALWAYS_INLINE uae_u32 sfc030c_get_long(uaecptr addr)
{
#if MMUDEBUG > 2
	write_log(_T("sfc030_get_long: FC = %i\n"), regs.sfc);
#endif
	return read_data_030_fc_lget(addr, regs.sfc);
}

static ALWAYS_INLINE uae_u16 sfc030c_get_word(uaecptr addr)
{
#if MMUDEBUG > 2
	write_log(_T("sfc030_get_word: FC = %i\n"), regs.sfc);
#endif
	return read_data_030_fc_wget(addr, regs.sfc);
}

static ALWAYS_INLINE uae_u8 sfc030c_get_byte(uaecptr addr)
{
#if MMUDEBUG > 2
	write_log(_T("sfc030_get_byte: FC = %i\n"), regs.sfc);
#endif
	return read_data_030_fc_bget(addr, regs.sfc);
}

static ALWAYS_INLINE void dfc030c_put_long(uaecptr addr, uae_u32 val)
{
#if MMUDEBUG > 2
	write_log(_T("dfc030_put_long: %08X = %08X FC = %i\n"), addr, val, regs.dfc);
#endif
	write_data_030_fc_lput(addr, val, regs.dfc);
}

static ALWAYS_INLINE void dfc030c_put_word(uaecptr addr, uae_u16 val)
{
#if MMUDEBUG > 2
	write_log(_T("dfc030_put_word: %08X = %04X FC = %i\n"), addr, val, regs.dfc);
#endif
	write_data_030_fc_wput(addr, val, regs.dfc);
}

static ALWAYS_INLINE void dfc030c_put_byte(uaecptr addr, uae_u8 val)
{
#if MMUDEBUG > 2
	write_log(_T("dfc030_put_byte: %08X = %02X FC = %i\n"), addr, val, regs.dfc);
#endif
	write_data_030_fc_bput(addr, val, regs.dfc);
}

static ALWAYS_INLINE uae_u32 sfc030c_get_long_state(uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
	v = sfc030c_get_long(addr);
	ACCESS_EXIT_GET
	return v;
}
static ALWAYS_INLINE uae_u16 sfc030c_get_word_state(uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
	v = sfc030c_get_word(addr);
	ACCESS_EXIT_GET
	return v;
}
static ALWAYS_INLINE uae_u8 sfc030c_get_byte_state(uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
	v = sfc030c_get_byte(addr);
	ACCESS_EXIT_GET
	return v;
}

static ALWAYS_INLINE void dfc030c_put_long_state(uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	dfc030c_put_long(addr, v);
	ACCESS_EXIT_PUT
}
static ALWAYS_INLINE void dfc030c_put_word_state(uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	dfc030c_put_word(addr, v);
	ACCESS_EXIT_PUT
}
static ALWAYS_INLINE void dfc030c_put_byte_state(uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	dfc030c_put_byte(addr, v);
	ACCESS_EXIT_PUT
}

uae_u32 REGPARAM3 get_disp_ea_020_mmu030c (uae_u32 base, int idx) REGPARAM;

STATIC_INLINE void put_byte_mmu030c_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	write_data_030_bput(addr, v);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_lrmw_byte_mmu030c_state (uaecptr addr, uae_u32 v)
{
	islrmw030 = true;
	ACCESS_CHECK_PUT
	write_dcache030_lrmw_mmu(addr, v, 0);
	ACCESS_EXIT_PUT
	islrmw030 = false;
}
STATIC_INLINE void put_word_mmu030c_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	write_data_030_wput(addr, v);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_lrmw_word_mmu030c_state (uaecptr addr, uae_u32 v)
{
	islrmw030 = true;
	ACCESS_CHECK_PUT
	write_dcache030_lrmw_mmu(addr, v, 1);
	ACCESS_EXIT_PUT
	islrmw030 = false;
}
STATIC_INLINE void put_long_mmu030c_state (uaecptr addr, uae_u32 v)
{
	ACCESS_CHECK_PUT
	write_data_030_lput(addr, v);
	ACCESS_EXIT_PUT
}
STATIC_INLINE void put_lrmw_long_mmu030c_state (uaecptr addr, uae_u32 v)
{
	islrmw030 = true;
	ACCESS_CHECK_PUT
	write_dcache030_lrmw_mmu(addr, v, 2);
	ACCESS_EXIT_PUT
	islrmw030 = false;
}

STATIC_INLINE uae_u32 get_byte_mmu030c_state (uaecptr addr)
{
	uae_u32 v;
	ACCESS_CHECK_GET
    v = read_data_030_bget(addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_lrmw_byte_mmu030c_state (uaecptr addr)
{
	uae_u32 v;
	islrmw030 = true;
	ACCESS_CHECK_GET
    v = read_dcache030_lrmw_mmu(addr, 0);
	ACCESS_EXIT_GET
	islrmw030 = false;
	return v;
}

STATIC_INLINE uae_u32 get_word_mmu030c_state (uaecptr addr)
{
 	uae_u32 v;
	ACCESS_CHECK_GET
    v = read_data_030_wget(addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_lrmw_word_mmu030c_state (uaecptr addr)
{
 	uae_u32 v;
	islrmw030 = true;
	ACCESS_CHECK_GET
    v = read_dcache030_lrmw_mmu(addr, 1);
	ACCESS_EXIT_GET
	islrmw030 = false;
	return v;
}
STATIC_INLINE uae_u32 get_long_mmu030c_state (uaecptr addr)
{
 	uae_u32 v;
	ACCESS_CHECK_GET
    v = read_data_030_lget(addr);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_lrmw_long_mmu030c_state (uaecptr addr)
{
 	uae_u32 v;
	islrmw030 = true;
	ACCESS_CHECK_GET
    v = read_dcache030_lrmw_mmu(addr, 2);
	ACCESS_EXIT_GET
	islrmw030 = false;
	return v;
}

STATIC_INLINE uae_u32 get_ibyte_mmu030c_state (int o)
{
 	uae_u32 v;
    uae_u32 addr = m68k_getpci () + o;
	ACCESS_CHECK_GET
	v = get_word_icache030(addr);
	ACCESS_EXIT_GET
	return v;
}
uae_u32 get_word_030_prefetch(int o);
STATIC_INLINE uae_u32 get_iword_mmu030c_state (int o)
{
 	uae_u32 v;
	ACCESS_CHECK_GET;
	v = get_word_030_prefetch(o);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 get_ilong_mmu030c_state (int o)
{
 	uae_u32 v;
	v = get_iword_mmu030c_state(o + 0) << 16;
	v |= get_iword_mmu030c_state(o + 2) & 0xffff;
	return v;
}
STATIC_INLINE uae_u32 get_iword_mmu030c_opcode_state(int o)
{
	return get_iword_mmu030c_state(o);
}


STATIC_INLINE uae_u32 next_iword_mmu030c_state (void)
{
 	uae_u32 v;
	ACCESS_CHECK_GET_PC(2);
	v = get_word_030_prefetch(0);
	m68k_incpci(2);
	ACCESS_EXIT_GET
	return v;
}
STATIC_INLINE uae_u32 next_ilong_mmu030c_state (void)
{
 	uae_u32 v;
	v = next_iword_mmu030c_state() << 16;
	v |= next_iword_mmu030c_state() & 0xffff;
	return v;
}

STATIC_INLINE uae_u32 get_word_mmu030c (uaecptr addr)
{
	return read_data_030_wget(addr);
}
STATIC_INLINE uae_u32 get_long_mmu030c (uaecptr addr)
{
	return read_data_030_lget(addr);
}
STATIC_INLINE void put_word_mmu030c (uaecptr addr, uae_u32 v)
{
    write_data_030_wput(addr, v);
}
STATIC_INLINE void put_long_mmu030c (uaecptr addr, uae_u32 v)
{
    write_data_030_lput(addr, v);
}

extern void m68k_do_rts_mmu030c(void);
extern void m68k_do_rte_mmu030c(uaecptr a7);
extern void m68k_do_bsr_mmu030c(uaecptr oldpc, uae_s32 offset);

#endif /* UAE_CPUMMU030_H */
