/*
* UAE - The Un*x Amiga Emulator
*
* MC68000 emulation
*
* Copyright 1995 Bernd Schmidt
*/

#ifndef UAE_NEWCPU_H
#define UAE_NEWCPU_H

#include "uae/types.h"
#include "readcpu.h"
#include "machdep/m68k.h"
#include "events.h"
#include "thread.h"

#ifdef WITH_SOFTFLOAT
#include <softfloat/softfloat.h>
#endif

#ifndef SET_CFLG

#define SET_CFLG(x) (CFLG() = (x))
#define SET_NFLG(x) (NFLG() = (x))
#define SET_VFLG(x) (VFLG() = (x))
#define SET_ZFLG(x) (ZFLG() = (x))
#define SET_XFLG(x) (XFLG() = (x))

#define GET_CFLG() CFLG()
#define GET_NFLG() NFLG()
#define GET_VFLG() VFLG()
#define GET_ZFLG() ZFLG()
#define GET_XFLG() XFLG()

#define CLEAR_CZNV() do { \
	SET_CFLG (0); \
	SET_ZFLG (0); \
	SET_NFLG (0); \
	SET_VFLG (0); \
} while (0)

#define COPY_CARRY() (SET_XFLG (GET_CFLG ()))
#endif

extern const int areg_byteinc[];
extern const int imm8_table[];

extern int movem_index1[256];
extern int movem_index2[256];
extern int movem_next[256];

#ifdef FPUEMU
extern int fpp_movem_index1[256];
extern int fpp_movem_index2[256];
extern int fpp_movem_next[256];
#endif

extern int hardware_bus_error;

typedef uae_u32 REGPARAM3 cpuop_func(uae_u32) REGPARAM;
typedef void REGPARAM3 cpuop_func_noret(uae_u32) REGPARAM;

struct cputbl {
	cpuop_func *handler_ff;
#ifdef NOFLAGS_SUPPORT_GENCPU
	cpuop_func_ret *handler_nf;
#endif
	cpuop_func_noret *handler_ff_noret;
#ifdef NOFLAGS_SUPPORT_GENCPU
	cpuop_func_ret *handler_nf_noret;
#endif
	uae_u16 opcode;
	uae_s8 length;
	uae_s8 disp020[2];
	uae_s8 branch;
#ifdef JIT
	uae_u16 specific;
#endif
};

#ifdef JIT
#define MIN_JIT_CACHE 128
#define MAX_JIT_CACHE 16384
typedef uae_u32 REGPARAM3 compop_func (uae_u32) REGPARAM;

#define COMP_OPCODE_ISJUMP      0x0001
#define COMP_OPCODE_LONG_OPCODE 0x0002
#define COMP_OPCODE_CMOV        0x0004
#define COMP_OPCODE_ISADDX      0x0008
#define COMP_OPCODE_ISCJUMP     0x0010
#define COMP_OPCODE_USES_FPU    0x0020

struct comptbl {
	compop_func *handler;
	uae_u32 opcode;
	int specific;
};
#endif

extern cpuop_func *loop_mode_table[];

extern uae_u32 REGPARAM3 op_illg(uae_u32) REGPARAM;
extern void REGPARAM3 op_illg_noret(uae_u32) REGPARAM;
void REGPARAM3 op_illg_1_noret(uae_u32 opcode) REGPARAM;
extern void REGPARAM3 op_unimpl(uae_u32) REGPARAM;
void REGPARAM3 op_unimpl_1_noret(uae_u32 opcode) REGPARAM;

typedef uae_u8 flagtype;

#ifdef FPUEMU

#ifdef USE_LONG_DOUBLE
typedef long double fptype;
#else
typedef double fptype;
#endif
#endif

#define MAX68020CYCLES 4

#define CPU_PIPELINE_MAX 4
#define CPU000_MEM_CYCLE 4
#define CPU000_CLOCK_MULT 2
#define CPU020_MEM_CYCLE 3
#define CPU020_CLOCK_MULT 4

#define CACHELINES020 64
struct cache020
{
	uae_u32 data;
	uae_u32 tag;
	bool valid;
};

#define CACHELINES030 16
struct cache030
{
	uae_u32 data[4];
	bool valid[4];
	uae_u32 tag;
	uae_u8 fc;
};

#define CACHESETS040 64
#define CACHESETS060 128
#define CACHELINES040 4
struct cache040
{
	uae_u32 data[CACHELINES040][4];
	bool dirty[CACHELINES040][4];
	bool gdirty[CACHELINES040];
	bool valid[CACHELINES040];
	uae_u32 tag[CACHELINES040];
};

struct mmufixup
{
	int reg;
	uae_u32 value;
};
extern struct mmufixup mmufixup[2];

#ifdef MSVC_LONG_DOUBLE
typedef struct {
	uae_u64 m;
	uae_u16 e;
	uae_u16 dummy;
} fprawtype;
#endif

typedef struct
{
#ifdef WITH_SOFTFLOAT
	floatx80 fpx;
#endif
#ifdef MSVC_LONG_DOUBLE
	union {
		fptype fp;
		fprawtype rfp;
	};
#else
	fptype fp;
#endif
} fpdata;

#ifdef CPU_AARCH64
#ifdef JIT
#include "jit/comptbl.h"
#include "jit/compemu.h"
#endif
#endif

struct regstruct
{
	uae_u32 regs[16];

	uae_u32 pc;
	uae_u8 *pc_p;
	uae_u8 *pc_oldp;
	uae_u16 opcode;
	uae_u32 instruction_pc;
	uae_u32 instruction_pc_user_exception;
	uae_u32 trace_pc;

	uae_u16 irc, ir, ird;
	volatile uae_atomic spcflags;
	uae_u32 last_prefetch;
	uae_u32 chipset_latch_rw;
	uae_u32 chipset_latch_read;
	uae_u32 chipset_latch_write;
	uae_u16 db, write_buffer, read_buffer;
	int loop_mode;
	int instruction_cnt;

	uaecptr usp, isp, msp;
	uae_u16 sr;
	flagtype t1;
	flagtype t0;
	flagtype s;
	flagtype m;
	flagtype stopped;
	int halted;
	int exception;
	int intmask;
	int ipl[2], ipl_pin, ipl_pin_p;
	int lastipl;
	evt_t ipl_pin_change_evt, ipl_pin_change_evt_p;
	evt_t ipl_evt, ipl_evt_pre;
	int ipl_evt_pre_mode;

	uae_u32 vbr, sfc, dfc;

#ifdef FPUEMU
	fpdata fp[8];
	fpdata fp_result;
	uae_u32 fpcr, fpsr, fpiar;
	uae_u32 fpu_state;
	uae_u32 fpu_exp_state;
	uae_u16 fp_opword;
	uaecptr fp_ea;
	bool fp_ea_set;
	uae_u32 fp_exp_pend, fp_unimp_pend;
	bool fpu_exp_pre;
	bool fp_unimp_ins;
	bool fp_exception;
	bool fp_branch;
#endif
#ifndef CPUEMU_68000_ONLY
	uae_u32 cacr, caar;
	uae_u32 itt0, itt1, dtt0, dtt1;
	uae_u32 tcr, mmusr, urp, srp, buscr;
	uae_u32 mmu_fslw;
	uae_u32 mmu_fault_addr, mmu_effective_addr;
	uae_u16 mmu_ssw;
	uae_u32 wb2_address;
	uae_u32 wb3_data;
	uae_u8 wb3_status, wb2_status;
	int mmu_enabled;
	int mmu_page_size;
#endif

	uae_u32 pcr;
	uae_u32 address_space_mask;

	uae_u16 prefetch020[CPU_PIPELINE_MAX];
	uae_u8 prefetch020_valid[CPU_PIPELINE_MAX];
	uae_u32 prefetch020addr;
	uae_u32 cacheholdingdata020;
	uae_u32 cacheholdingaddr020;
	uae_u8 cacheholdingdata_valid;
	int pipeline_pos;
	int pipeline_r8[2];
	int pipeline_stop;
	uae_u8 fc030;

	uae_u32 prefetch040[CPU_PIPELINE_MAX];

	evt_t ce020endcycle;
	evt_t ce020startcycle;
	evt_t ce020prefetchendcycle;

	evt_t ce020extracycles;
	bool ce020memcycle_data;
	int ce020_tail;
	evt_t ce020_tail_cycles;
	int memory_waitstate_cycles;

#ifdef CPU_AARCH64 // Used by the AARCH64 JIT implementation
#ifdef JIT
	/* store scratch regs also in this struct to avoid load of mem pointer */
	uae_u32 scratchregs[VREGS - S1];
	fpu_register scratchfregs[VFREGS - 8];
	uae_u32 jit_exception;

	/* pointer to real arrays/structs for easier access in JIT */
	uae_u32* raw_cputbl_count;
	uintptr mem_banks;
	uintptr cache_tags;
#endif
#endif
};

extern struct regstruct regs;

#define MAX_CPUTRACESIZE 128
struct cputracememory
{
	uae_u32 addr;
	uae_u32 data;
	int mode;
	uae_u32 flags;
};

struct cputracestruct
{
	uae_u32 regs[16];
	uae_u32 usp, isp, pc;
	uae_u16 ir, irc, ird, sr, opcode;
	int intmask, stopped, state;

	uae_u32 msp, vbr;
	uae_u32 cacr, caar;
	uae_u16 prefetch020[CPU_PIPELINE_MAX];
	uae_u8 prefetch020_valid[CPU_PIPELINE_MAX];
	uae_u32 prefetch020addr;
	uae_u32 cacheholdingdata020;
	uae_u32 cacheholdingaddr020;
	struct cache020 caches020[CACHELINES020];
	int pipeline_pos;
	int pipeline_r8[2];
	int pipeline_stop;
	uae_u16 read_buffer, write_buffer;

	evt_t startcycles;
	int needendcycles;
	int memoryoffset;
	int cyclecounter, cyclecounter_pre, cyclecounter_post;
	int readcounter, writecounter;
	struct cputracememory ctm[MAX_CPUTRACESIZE];
};

STATIC_INLINE uae_u32 munge24 (uae_u32 x)
{
	return x & regs.address_space_mask;
}

extern int mmu_enabled, mmu_triggered;
extern int cpu_cycles;
extern int cpucycleunit, cpuipldelay2, cpuipldelay4;
extern int m68k_pc_indirect;
extern bool m68k_interrupt_delay;

extern void safe_interrupt_set(int, int, bool);

#define SPCFLAG_CPUINRESET 2
#define SPCFLAG_INT 8
#define SPCFLAG_BRK 16
#define SPCFLAG_UAEINT 32
#define SPCFLAG_TRACE 64
#define SPCFLAG_DOTRACE 128
#define SPCFLAG_DOINT 256 /* arg, JIT fails without this.. */
#define SPCFLAG_BLTNASTY 512
#define SPCFLAG_EXEC 1024
#define SPCFLAG_ACTION_REPLAY 2048
#define SPCFLAG_TRAP 4096 /* enforcer-hack */
#define SPCFLAG_MODE_CHANGE 8192
#ifdef JIT
#define SPCFLAG_END_COMPILE 16384
#endif
#define SPCFLAG_CHECK 32768
#define SPCFLAG_MMURESTART 65536

STATIC_INLINE void set_special_exter(uae_u32 x)
{
	atomic_or(&regs.spcflags, x);
}
STATIC_INLINE void set_special (uae_u32 x)
{
	atomic_or(&regs.spcflags, x);
	cycles_do_special();
}

STATIC_INLINE void unset_special (uae_u32 x)
{
	atomic_and(&regs.spcflags, ~x);
}

#define m68k_dreg(r,num) ((r).regs[(num)])
#define m68k_areg(r,num) (((r).regs + 8)[(num)])

// JIT only
#ifdef HAVE_GET_WORD_UNSWAPPED
	#define GET_OPCODE (do_get_mem_word_unswapped((uae_u16*)(pc + pc_offset)));
#else
	#define GET_OPCODE (do_get_mem_word((uae_u16*)(pc + pc_offset)));
#endif

extern uae_u32(*x_prefetch)(int);
extern uae_u32(*x_get_byte)(uaecptr addr);
extern uae_u32(*x_get_word)(uaecptr addr);
extern uae_u32(*x_get_long)(uaecptr addr);
extern void(*x_put_byte)(uaecptr addr, uae_u32 v);
extern void(*x_put_word)(uaecptr addr, uae_u32 v);
extern void(*x_put_long)(uaecptr addr, uae_u32 v);
extern uae_u32(*x_next_iword)(void);
extern uae_u32(*x_next_ilong)(void);
extern uae_u32(*x_get_ilong)(int);
extern uae_u32(*x_get_iword)(int);
extern uae_u32(*x_get_ibyte)(int);

extern uae_u32(*x_cp_get_byte)(uaecptr addr);
extern uae_u32(*x_cp_get_word)(uaecptr addr);
extern uae_u32(*x_cp_get_long)(uaecptr addr);
extern void(*x_cp_put_byte)(uaecptr addr, uae_u32 v);
extern void(*x_cp_put_word)(uaecptr addr, uae_u32 v);
extern void(*x_cp_put_long)(uaecptr addr, uae_u32 v);
extern uae_u32(*x_cp_next_iword)(void);
extern uae_u32(*x_cp_next_ilong)(void);

void mem_access_delay_long_write_ce020 (uaecptr addr, uae_u32 v);
void mem_access_delay_word_write_ce020 (uaecptr addr, uae_u32 v);
void mem_access_delay_byte_write_ce020 (uaecptr addr, uae_u32 v);
uae_u32 mem_access_delay_byte_read_ce020 (uaecptr addr);
uae_u32 mem_access_delay_word_read_ce020 (uaecptr addr);
uae_u32 mem_access_delay_long_read_ce020 (uaecptr addr);
uae_u32 mem_access_delay_longi_read_ce020 (uaecptr addr);
uae_u32 mem_access_delay_wordi_read_ce020 (uaecptr addr);

void mem_access_delay_long_write_c040 (uaecptr addr, uae_u32 v);
void mem_access_delay_word_write_c040 (uaecptr addr, uae_u32 v);
void mem_access_delay_byte_write_c040 (uaecptr addr, uae_u32 v);
uae_u32 mem_access_delay_byte_read_c040 (uaecptr addr);
uae_u32 mem_access_delay_word_read_c040 (uaecptr addr);
uae_u32 mem_access_delay_long_read_c040 (uaecptr addr);
uae_u32 mem_access_delay_longi_read_c040 (uaecptr addr);

extern uae_u32(REGPARAM3 *x_cp_get_disp_ea_020)(uae_u32 base, int idx) REGPARAM;

extern bool debugmem_trace;
extern void branch_stack_push(uaecptr, uaecptr);
extern void branch_stack_pop_rte(uaecptr);
extern void branch_stack_pop_rts(uaecptr);

/* direct (regs.pc_p) access */

STATIC_INLINE void m68k_setpc(uaecptr newpc)
{
	regs.pc_p = regs.pc_oldp = get_real_address(newpc);
	regs.instruction_pc = regs.pc = newpc;
}
STATIC_INLINE void m68k_setpc_j(uaecptr newpc)
{
	regs.pc_p = regs.pc_oldp = get_real_address(newpc);
	regs.pc = newpc;
}
STATIC_INLINE uaecptr m68k_getpc(void)
{
	return (uaecptr)(regs.pc + ((uae_u8*)regs.pc_p - (uae_u8*)regs.pc_oldp));
}
#define M68K_GETPC m68k_getpc()
STATIC_INLINE uaecptr m68k_getpc_p(uae_u8 *p)
{
	return (uaecptr)(regs.pc + ((uae_u8*)p - (uae_u8*)regs.pc_oldp));
}
STATIC_INLINE void m68k_incpc(int o)
{
	regs.pc_p += o;
}

STATIC_INLINE uae_u32 get_dibyte(int o)
{
	return do_get_mem_byte((uae_u8 *)((regs).pc_p + (o) + 1));
}
STATIC_INLINE uae_u32 get_diword(int o)
{
	return do_get_mem_word((uae_u16 *)((regs).pc_p + (o)));
}
STATIC_INLINE uae_u32 get_dilong(int o)
{
	return do_get_mem_long((uae_u32 *)((regs).pc_p + (o)));
}
STATIC_INLINE uae_u32 next_diword(void)
{
	uae_u32 r = do_get_mem_word((uae_u16 *)((regs).pc_p));
	m68k_incpc(2);
	return r;
}
STATIC_INLINE uae_u32 next_dilong(void)
{
	uae_u32 r = do_get_mem_long((uae_u32 *)((regs).pc_p));
	m68k_incpc(4);
	return r;
}

STATIC_INLINE void m68k_do_bsr(uaecptr oldpc, uae_s32 offset)
{
	m68k_areg(regs, 7) -= 4;
	put_long(m68k_areg(regs, 7), oldpc);
	m68k_incpc(offset);
}
STATIC_INLINE void m68k_do_rts(void)
{
	uae_u32 newpc = get_long(m68k_areg(regs, 7));
	m68k_setpc(newpc);
	m68k_areg(regs, 7) += 4;
}

/* indirect (regs.pc) access */

STATIC_INLINE void m68k_setpci(uaecptr newpc)
{
	regs.instruction_pc = regs.pc = newpc;
}
STATIC_INLINE void m68k_setpci_j(uaecptr newpc)
{
	regs.pc = newpc;
}
STATIC_INLINE uaecptr m68k_getpci(void)
{
	return regs.pc;
}
STATIC_INLINE void m68k_incpci(int o)
{
	regs.pc += o;
}

STATIC_INLINE uae_u32 get_iibyte(int o)
{
	return get_wordi(m68k_getpci() + (o)) & 0xff;
}
STATIC_INLINE uae_u32 get_iiword(int o)
{
	return get_wordi(m68k_getpci() + (o));
}
STATIC_INLINE uae_u32 get_iilong(int o)
{
	return get_longi(m68k_getpci () + (o));
}

STATIC_INLINE uae_u32 next_iibyte (void)
{
	uae_u32 r = get_iibyte (0);
	m68k_incpci (2);
	return r;
}
STATIC_INLINE uae_u32 next_iiword (void)
{
	uae_u32 r = get_iiword (0);
	m68k_incpci (2);
	return r;
}
STATIC_INLINE uae_u32 next_iiwordi (void)
{
	uae_u32 r = get_wordi(m68k_getpci());
	m68k_incpci (2);
	return r;
}
STATIC_INLINE uae_u32 next_iilong (void)
{
	uae_u32 r = get_iilong(0);
	m68k_incpci (4);
	return r;
}
STATIC_INLINE uae_u32 next_iilongi (void)
{
	uae_u32 r = get_longi (m68k_getpci ());
	m68k_incpci (4);
	return r;
}

STATIC_INLINE void m68k_do_bsri(uaecptr oldpc, uae_s32 offset)
{
	m68k_areg(regs, 7) -= 4;
	x_put_long(m68k_areg(regs, 7), oldpc);
	m68k_incpci(offset);
}
STATIC_INLINE void m68k_do_rtsi(void)
{
	uae_u32 newpc = x_get_long(m68k_areg(regs, 7));
	m68k_setpci(newpc);
	m68k_areg(regs, 7) += 4;
}

/* indirect jit friendly versions */

STATIC_INLINE uae_u32 get_iibyte_jit(int o)
{
	return get_wordi(m68k_getpc() + (o)) & 0xff;
}
STATIC_INLINE uae_u32 get_iiword_jit(int o)
{
	return get_wordi(m68k_getpc() + (o));
}
STATIC_INLINE uae_u32 get_iilong_jit(int o)
{
	return get_longi(m68k_getpc() + (o));
}
STATIC_INLINE uae_u32 next_iiword_jit(void)
{
	uae_u32 r = get_wordi(m68k_getpc());
	m68k_incpc(2);
	return r;
}
STATIC_INLINE uae_u32 next_iilong_jit(void)
{
	uae_u32 r = get_longi(m68k_getpc());
	m68k_incpc(4);
	return r;
}
STATIC_INLINE void m68k_do_bsri_jit(uaecptr oldpc, uae_s32 offset)
{
	m68k_areg(regs, 7) -= 4;
	x_put_long(m68k_areg(regs, 7), oldpc);
	m68k_incpc(offset);
}
STATIC_INLINE void m68k_do_rtsi_jit(void)
{
	uae_u32 newpc = x_get_long(m68k_areg(regs, 7));
	m68k_setpc(newpc);
	m68k_areg(regs, 7) += 4;
}

/* common access */

STATIC_INLINE void m68k_incpc_normal(int o)
{
	if (m68k_pc_indirect > 0)
		m68k_incpci(o);
	else
		m68k_incpc(o);
}

STATIC_INLINE void m68k_setpc_normal(uaecptr pc)
{
	if (m68k_pc_indirect > 0) {
		regs.pc_p = regs.pc_oldp = 0;
		m68k_setpci(pc);
	} else {
		m68k_setpc(pc);
	}
}

extern void cpu_invalidate_cache(uaecptr, int);

extern bool(*is_super_access)(bool);

extern uae_u32(*read_data_030_bget)(uaecptr);
extern uae_u32(*read_data_030_wget)(uaecptr);
extern uae_u32(*read_data_030_lget)(uaecptr);
extern void(*write_data_030_bput)(uaecptr,uae_u32);
extern void(*write_data_030_wput)(uaecptr,uae_u32);
extern void(*write_data_030_lput)(uaecptr,uae_u32);

extern uae_u32(*read_data_030_fc_bget)(uaecptr, uae_u32);
extern uae_u32(*read_data_030_fc_wget)(uaecptr, uae_u32);
extern uae_u32(*read_data_030_fc_lget)(uaecptr, uae_u32);
extern void(*write_data_030_fc_bput)(uaecptr, uae_u32, uae_u32);
extern void(*write_data_030_fc_wput)(uaecptr, uae_u32, uae_u32);
extern void(*write_data_030_fc_lput)(uaecptr, uae_u32, uae_u32);

extern void write_dcache030_bput(uaecptr, uae_u32, uae_u32);
extern void write_dcache030_wput(uaecptr, uae_u32, uae_u32);
extern void write_dcache030_lput(uaecptr, uae_u32, uae_u32);
extern void write_dcache030_retry(uaecptr addr, uae_u32 v, uae_u32 fc, int size, int flags);
extern uae_u32 read_dcache030_bget(uaecptr, uae_u32);
extern uae_u32 read_dcache030_wget(uaecptr, uae_u32);
extern uae_u32 read_dcache030_lget(uaecptr, uae_u32);
extern uae_u32 read_dcache030_retry(uaecptr addr, uae_u32 fc, int size, int flags);

extern void write_dcache030_mmu_bput(uaecptr, uae_u32);
extern void write_dcache030_mmu_wput(uaecptr, uae_u32);
extern void write_dcache030_mmu_lput(uaecptr, uae_u32);
extern uae_u32 read_dcache030_mmu_bget(uaecptr);
extern uae_u32 read_dcache030_mmu_wget(uaecptr);
extern uae_u32 read_dcache030_mmu_lget(uaecptr);
extern void write_dcache030_lrmw_mmu(uaecptr, uae_u32, uae_u32);
extern void write_dcache030_lrmw_mmu_fcx(uaecptr, uae_u32, uae_u32, int);
extern uae_u32 read_dcache030_lrmw_mmu(uaecptr, uae_u32);
extern uae_u32 read_dcache030_lrmw_mmu_fcx(uaecptr, uae_u32, int);

extern void check_t0_trace(void);
extern uae_u32 get_word_icache030(uaecptr addr);
extern uae_u32 get_long_icache030(uaecptr addr);

uae_u32 fill_icache040(uae_u32 addr);
extern void put_long_cache_040(uaecptr, uae_u32);
extern void put_word_cache_040(uaecptr, uae_u32);
extern void put_byte_cache_040(uaecptr, uae_u32);
extern uae_u32 get_ilong_cache_040(int);
extern uae_u32 get_iword_cache_040(int);
extern uae_u32 get_long_cache_040(uaecptr);
extern uae_u32 get_word_cache_040(uaecptr);
extern uae_u32 get_byte_cache_040(uaecptr);
extern uae_u32 next_iword_cache040(void);
extern uae_u32 next_ilong_cache040(void);
extern uae_u32 get_word_icache040(uaecptr addr);
extern uae_u32 get_long_icache040(uaecptr addr);

extern uae_u32 sfc_nommu_get_byte(uaecptr);
extern uae_u32 sfc_nommu_get_word(uaecptr);
extern uae_u32 sfc_nommu_get_long(uaecptr);
extern void dfc_nommu_put_byte(uaecptr, uae_u32);
extern void dfc_nommu_put_word(uaecptr, uae_u32);
extern void dfc_nommu_put_long(uaecptr, uae_u32);

extern void (*x_do_cycles)(int);
extern void (*x_do_cycles_pre)(int);
extern void (*x_do_cycles_post)(int, uae_u32);

extern uae_u32 REGPARAM3 x_get_disp_ea_020 (uae_u32 base, int idx) REGPARAM;
extern uae_u32 REGPARAM3 x_get_disp_ea_ce020 (uae_u32 base, int idx) REGPARAM;
extern uae_u32 REGPARAM3 x_get_disp_ea_ce030 (uae_u32 base, int idx) REGPARAM;
extern uae_u32 REGPARAM3 x_get_disp_ea_040(uae_u32 base, int idx) REGPARAM;
extern uae_u32 REGPARAM3 x_get_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width) REGPARAM;
extern void REGPARAM3 x_put_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width) REGPARAM;

extern void m68k_setstopped(int stoptype);
extern void m68k_resumestopped(void);
extern void m68k_cancel_idle(void);
extern void do_cycles_stop(int);

extern uae_u32 REGPARAM3 get_disp_ea_020 (uae_u32 base, int idx) REGPARAM;
extern uae_u32 REGPARAM3 get_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width) REGPARAM;
extern void REGPARAM3 put_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width) REGPARAM;

extern void m68k_disasm_ea (uaecptr addr, uaecptr *nextpc, int cnt, uae_u32 *seaddr, uae_u32 *deaddr, uaecptr lastpc);
extern void m68k_disasm (uaecptr addr, uaecptr *nextpc, uaecptr lastpc, int cnt);
extern uae_u32 m68k_disasm_2(TCHAR *buf, int bufsize, uaecptr pc, uae_u16 *bufpc, int bufpccount, uaecptr *nextpc, int cnt, uae_u32 *seaddr, uae_u32 *deaddr, uaecptr lastpc, int safemode);
extern void sm68k_disasm (TCHAR*, TCHAR*, uaecptr addr, uaecptr *nextpc, uaecptr lastpc);
extern int m68k_asm(TCHAR *buf, uae_u16 *out, uaecptr pc);
extern uaecptr ShowEA(void *f, uaecptr pc, uae_u16 opcode, int reg, amodes mode, wordsizes size, TCHAR *buf, uae_u32 *eaddr, int *actualea, int safemode);
extern int get_cpu_model (void);

extern void set_cpu_caches (bool flush);
extern void flush_cpu_caches(bool flush);
extern void flush_cpu_caches_040(uae_u16 opcode);
extern void REGPARAM3 MakeSR (void) REGPARAM;
extern void REGPARAM3 MakeFromSR(void) REGPARAM;
extern void REGPARAM3 MakeFromSR_T0(void) REGPARAM;
extern void REGPARAM3 MakeFromSR_STOP(void) REGPARAM;
extern void REGPARAM3 Exception (int) REGPARAM;
extern void REGPARAM3 Exception_cpu(int) REGPARAM;
extern void REGPARAM3 Exception_cpu_oldpc(int, uaecptr) REGPARAM;
extern void REGPARAM3 ExceptionL (int, uaecptr) REGPARAM;
extern void NMI (void);
extern void IRQ_forced(int, int);
extern void prepare_interrupt (uae_u32);
extern void doint(void);
extern void checkint(void);
extern void intlev_load(void);
extern void ipl_fetch_now_pre(void);
extern void ipl_fetch_next_pre(void);
extern void ipl_fetch_now(void);
extern void ipl_fetch_next(void);
extern void dump_counts (void);
extern int m68k_move2c (int, uae_u32 *);
extern int m68k_movec2 (int, uae_u32 *);
extern int m68k_divl (uae_u32, uae_u32, uae_u16, uaecptr);
extern int m68k_mull (uae_u32, uae_u32, uae_u16);
extern void init_m68k (void);
extern void m68k_go (int);
extern void m68k_dumpstate(uaecptr *, uaecptr);
extern void m68k_dumpcache(bool);
extern int getMulu68kCycles(uae_u16 src);
extern int getMuls68kCycles(uae_u16 src);
extern int getDivu68kCycles (uae_u32 dividend, uae_u16 divisor);
extern int getDivs68kCycles (uae_s32 dividend, uae_s16 divisor, int *extra);
extern void divbyzero_special(bool issigned, uae_s32 dst);
extern void setdivuflags(uae_u32 dividend, uae_u16 divisor);
extern void setdivsflags(uae_s32 dividend, uae_s16 divisor);
extern void setchkundefinedflags(uae_s32 src, uae_s32 dst, int size);
extern void setchk2undefinedflags(uae_s32 lower, uae_s32 upper, uae_s32 val, int size);
extern void protect_roms (bool);
extern void unprotect_maprom (void);
extern bool is_hardreset(void);
extern bool is_keyboardreset(void);
extern void Exception_build_stack_frame_common(uae_u32 oldpc, uae_u32 currpc, uae_u32 ssw, int nr, int vector_nr);
extern void Exception_build_stack_frame(uae_u32 oldpc, uae_u32 currpc, uae_u32 ssw, int nr, int format);
extern void Exception_build_68000_address_error_stack_frame(uae_u16 mode, uae_u16 opcode, uaecptr fault_addr, uaecptr pc);
extern uae_u32 exception_pc(int nr);
extern void cpu_restore_fixup(void);
extern bool privileged_copro_instruction(uae_u16 opcode);
extern bool generates_group1_exception(uae_u16 opcode);

void ccr_68000_long_move_ae_LZN(uae_s32 src);
void ccr_68000_long_move_ae_LN(uae_s32 src);
void ccr_68000_long_move_ae_HNZ(uae_s32 src);
void ccr_68000_long_move_ae_normal(uae_s32 src);
void ccr_68000_word_move_ae_normal(uae_s16 src);
void dreg_68000_long_replace_low(int reg, uae_u16 v);
void areg_68000_long_replace_low(int reg, uae_u16 v);

extern void mmu_op (uae_u32, uae_u32);
extern bool mmu_op30 (uaecptr, uae_u32, uae_u16, uaecptr);

extern void fpuop_arithmetic(uae_u32, uae_u16);
extern void fpuop_dbcc(uae_u32, uae_u16);
extern void fpuop_scc(uae_u32, uae_u16);
extern void fpuop_trapcc(uae_u32, uaecptr, uae_u16);
extern void fpuop_bcc(uae_u32, uaecptr, uae_u32);
extern void fpuop_save(uae_u32);
extern void fpuop_restore(uae_u32);
extern uae_u32 fpp_get_fpsr (void);
extern bool fpu_get_constant(fpdata *fp, int cr);
extern int fpp_cond(int condition);

extern void exception3_read(uae_u32 opcode, uaecptr addr, int size, int fc);
extern void exception3_write(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc);
extern void exception3_read_access(uae_u32 opcode, uaecptr addr, int size, int fc);
extern void exception3_read_access2(uae_u32 opcode, uaecptr addr, int size, int fc);
extern void exception3_write_access(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc);
extern void exception3_read_prefetch(uae_u32 opcode, uaecptr addr);
extern void exception3_read_prefetch_68040bug(uae_u32 opcode, uaecptr addr, uae_u16 secondarysr);
extern void exception3_read_prefetch_only(uae_u32 opcode, uaecptr addr);
extern void exception3_notinstruction(uae_u32 opcode, uaecptr addr);
extern void hardware_exception2(uaecptr addr, uae_u32 v, bool read, bool ins, int size);
extern void exception2_setup(uae_u32 opcode, uaecptr addr, bool read, int size, uae_u32 fc);
extern void exception2_read(uae_u32 opcode, uaecptr addr, int size, int fc);
extern void exception2_write(uae_u32 opcode, uaecptr addr, int size, uae_u32 val, int fc);
extern void exception2_fetch_opcode(uae_u32 opcode, int offset, int pcoffset);
extern void exception2_fetch(uae_u32 opcode, int offset, int pcoffset);
extern void m68k_reset(void);
extern bool cpureset(void);
extern void cpu_halt(int id);
extern void cpu_inreset(void);
extern int cpu_sleep_millis(int ms);
extern void cpu_change(int newmodel);
extern void cpu_fallback(int mode);

extern void fill_prefetch(void);
extern void fill_prefetch_020_ntx(void);
extern void fill_prefetch_030_ntx(void);
extern void fill_prefetch_030_ntx_continue(void);
extern void fill_prefetch_020(void);
extern void fill_prefetch_030(void);

#define CPU_OP_NAME(a) op ## a

/* 68060 */
extern const struct cputbl op_smalltbl_0[];
extern const struct cputbl op_smalltbl_40[];
extern const struct cputbl op_smalltbl_50[];
extern const struct cputbl op_smalltbl_24[]; // CE
extern const struct cputbl op_smalltbl_33[]; // MMU
/* 68040 */
extern const struct cputbl op_smalltbl_1[];
extern const struct cputbl op_smalltbl_41[];
extern const struct cputbl op_smalltbl_51[];
extern const struct cputbl op_smalltbl_25[]; // CE
extern const struct cputbl op_smalltbl_31[]; // MMU
/* 68030 */
extern const struct cputbl op_smalltbl_2[];
extern const struct cputbl op_smalltbl_42[];
extern const struct cputbl op_smalltbl_52[];
extern const struct cputbl op_smalltbl_22[]; // prefetch
extern const struct cputbl op_smalltbl_23[]; // CE
extern const struct cputbl op_smalltbl_32[]; // MMU
extern const struct cputbl op_smalltbl_34[]; // MMU + cache
extern const struct cputbl op_smalltbl_35[]; // MMU + CE + cache
/* 68020 */
extern const struct cputbl op_smalltbl_3[];
extern const struct cputbl op_smalltbl_43[];
extern const struct cputbl op_smalltbl_53[];
extern const struct cputbl op_smalltbl_20[]; // prefetch
extern const struct cputbl op_smalltbl_21[]; // CE
/* 68010 */
extern const struct cputbl op_smalltbl_4[];
extern const struct cputbl op_smalltbl_44[];
extern const struct cputbl op_smalltbl_54[];
extern const struct cputbl op_smalltbl_11[]; // prefetch
extern const struct cputbl op_smalltbl_13[]; // CE
/* 68000 */
extern const struct cputbl op_smalltbl_5[];
extern const struct cputbl op_smalltbl_45[];
extern const struct cputbl op_smalltbl_55[];
extern const struct cputbl op_smalltbl_12[]; // prefetch
extern const struct cputbl op_smalltbl_14[]; // CE

extern cpuop_func_noret *cpufunctbl_noret[65536] ASM_SYM_FOR_FUNC("cpufunctbl_noret");
extern cpuop_func *cpufunctbl[65536] ASM_SYM_FOR_FUNC("cpufunctbl");

#ifdef JIT
extern void (*flush_icache)(int);
extern void compemu_reset(void);
#else
#define flush_icache(int) do {} while (0)
#define flush_icache_hard(int) do {} while (0)
#endif
bool check_prefs_changed_comp (bool);

extern int movec_illg (int regno);
extern uae_u32 val_move2c (int regno);
extern void val_move2c2 (int regno, uae_u32 val);
struct cpum2c {
	int regno;
	int flags;
	const TCHAR *regname;
};
extern struct cpum2c m2cregs[];

extern bool is_cpu_tracer (void);
extern bool set_cpu_tracer (bool force);
extern bool can_cpu_tracer (void);

#define CPU_HALT_PPC_ONLY -1
#define CPU_HALT_BUS_ERROR_DOUBLE_FAULT 1
#define CPU_HALT_DOUBLE_FAULT 2
#define CPU_HALT_OPCODE_FETCH_FROM_NON_EXISTING_ADDRESS 3
#define CPU_HALT_ACCELERATOR_CPU_FALLBACK 4
#define CPU_HALT_ALL_CPUS_STOPPED 5
#define CPU_HALT_FAKE_DMA 6
#define CPU_HALT_AUTOCONFIG_CONFLICT 7
#define CPU_HALT_PCI_CONFLICT 8
#define CPU_HALT_CPU_STUCK 9
#define CPU_HALT_SSP_IN_NON_EXISTING_ADDRESS 10
#define CPU_HALT_INVALID_START_ADDRESS 11
#define CPU_HALT_68060_HALT 12
#define CPU_HALT_BKPT 13

uae_u32 process_cpu_indirect_memory_read(uae_u32 addr, int size);
void process_cpu_indirect_memory_write(uae_u32 addr, uae_u32 data, int size);

const struct cputbl *uaegetjitcputbl(void);
const struct cputbl *getjitcputbl(int cpulvl, int direct);

#endif /* UAE_NEWCPU_H */
