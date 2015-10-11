/*
 * newcpu.h - CPU emulation
 *
 * Copyright (c) 2009 ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
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
 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68000 emulation
  *
  * Copyright 1995 Bernd Schmidt
  */

#include "readcpu.h"
#include "md-pandora/m68k.h"
#include "events.h"

#ifndef SET_CFLG

#define SET_CFLG(regs, x) (CFLG(regs) = (x))
#define SET_NFLG(regs, x) (NFLG(regs) = (x))
#define SET_VFLG(regs, x) (VFLG(regs) = (x))
#define SET_ZFLG(regs, x) (ZFLG(regs) = (x))
#define SET_XFLG(regs, x) (XFLG(regs) = (x))

#define GET_CFLG(regs) CFLG(regs)
#define GET_NFLG(regs) NFLG(regs)
#define GET_VFLG(regs) VFLG(regs)
#define GET_ZFLG(regs) ZFLG(regs)
#define GET_XFLG(regs) XFLG(regs)

#define CLEAR_CZNV(regs) do { \
 SET_CFLG (regs, 0); \
 SET_ZFLG (regs, 0); \
 SET_NFLG (regs, 0); \
 SET_VFLG (regs, 0); \
} while (0)

#define COPY_CARRY(regs) (SET_XFLG (regs, GET_CFLG (regs)))
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

struct regstruct;

typedef unsigned long REGPARAM3 cpuop_func (uae_u32, struct regstruct *regs) REGPARAM;
typedef void REGPARAM3 cpuop_func_ce (uae_u32, struct regstruct *regs) REGPARAM;

struct cputbl {
    cpuop_func *handler;
//    uae_u16 specific;
    uae_u16 opcode;
};

#ifdef JIT
typedef unsigned long REGPARAM3 compop_func (uae_u32) REGPARAM;

struct comptbl {
  compop_func *handler;
	uae_u32		specific;
  uae_u32 opcode;
};
#endif

extern unsigned long REGPARAM3 op_illg (uae_u32, struct regstruct *regs) REGPARAM;

typedef char flagtype;

#ifdef FPUEMU
/* You can set this to long double to be more accurate. However, the
   resulting alignment issues will cost a lot of performance in some
   apps */
#define USE_LONG_DOUBLE 0

#if USE_LONG_DOUBLE
typedef long double fptype;
#define LDPTR tbyte ptr
#else
typedef double fptype;
#define LDPTR qword ptr
#endif
#endif

extern struct regstruct
{
    uae_u32 regs[16];
    struct flag_struct ccrflags;

    uae_u32 pc;
    uae_u8 *pc_p;
    uae_u8 *pc_oldp;

    uae_u16 irc, ir;
    uae_u32 spcflags;

    uaecptr usp, isp, msp;
    uae_u16 sr;
    flagtype t1;
    flagtype t0;
    flagtype s;
    flagtype m;
    flagtype x;
    flagtype stopped;
    int intmask;

    uae_u32 vbr,sfc,dfc;

#ifdef FPUEMU
    fptype fp[8];
    fptype fp_result;

    uae_u32 fpcr,fpsr, fpiar;
    uae_u32 fpsr_highbyte;
#endif
#ifndef CPUEMU_68000_ONLY
    uae_u32 cacr, caar;
    uae_u32 itt0, itt1, dtt0, dtt1;
    uae_u32 tcr, mmusr, urp, srp, buscr;

    uae_u32 mmu_fslw, mmu_fault_addr;
    uae_u16 mmu_ssw;
    uae_u32 wb3_data;
    uae_u16 wb3_status;
    int mmu_enabled;
    int mmu_pagesize_8k;
#endif

    uae_u32 pcr;
    uae_u32 address_space_mask;

    uae_u8 panic;
    uae_u32 panic_pc, panic_addr;

} regs, lastint_regs;

typedef struct {
  uae_u16* location;
  uae_u8  cycles;
  uae_u8  specmem;
  uae_u8  dummy2;
  uae_u8  dummy3;
} cpu_history;

struct blockinfo_t;

typedef union {
    cpuop_func* handler;
    struct blockinfo_t* bi;
} cacheline;

extern signed long pissoff;

STATIC_INLINE uae_u32 munge24(uae_u32 x)
{
    return x & regs.address_space_mask;
}

extern int mmu_enabled, mmu_triggered;
extern int cpu_cycles;

STATIC_INLINE void set_special (struct regstruct *regs, uae_u32 x)
{
    regs->spcflags |= x;
    cycles_do_special();
}

STATIC_INLINE void unset_special (struct regstruct *regs, uae_u32 x)
{
    regs->spcflags &= ~x;
}

#define m68k_dreg(r,num) ((r)->regs[(num)])
#define m68k_areg(r,num) (((r)->regs + 8)[(num)])

STATIC_INLINE void m68k_setpc (struct regstruct *regs, uaecptr newpc)
{
    regs->pc_p = regs->pc_oldp = get_real_address (newpc);
    regs->pc = newpc;
}

STATIC_INLINE uaecptr m68k_getpc (struct regstruct *regs)
{
    return (uaecptr)(regs->pc + ((char *)regs->pc_p - (char *)regs->pc_oldp));
}
#define M68K_GETPC m68k_getpc(&regs)

STATIC_INLINE uaecptr m68k_getpc_p (struct regstruct *regs, uae_u8 *p)
{
    return (uaecptr)(regs->pc + ((char *)p - (char *)regs->pc_oldp));
}

#define m68k_incpc(regs, o) ((regs)->pc_p += (o))

STATIC_INLINE void m68k_setpci(struct regstruct *regs, uaecptr newpc)
{
    regs->pc = newpc;
}
STATIC_INLINE uaecptr m68k_getpci(struct regstruct *regs)
{
    return regs->pc;
}
STATIC_INLINE void m68k_incpci(struct regstruct *regs, int o)
{
    regs->pc += o;
}

STATIC_INLINE void m68k_do_rts(struct regstruct *regs)
{
    m68k_setpc(regs, get_long(m68k_areg(regs, 7)));
    m68k_areg(regs, 7) += 4;
}
STATIC_INLINE void m68k_do_rtsi(struct regstruct *regs)
{
    m68k_setpci(regs, get_long(m68k_areg(regs, 7)));
    m68k_areg(regs, 7) += 4;
}

STATIC_INLINE void m68k_do_bsr(struct regstruct *regs, uaecptr oldpc, uae_s32 offset)
{
    m68k_areg(regs, 7) -= 4;
    put_long(m68k_areg(regs, 7), oldpc);
    m68k_incpc(regs, offset);
}
STATIC_INLINE void m68k_do_bsri(struct regstruct *regs, uaecptr oldpc, uae_s32 offset)
{
    m68k_areg(regs, 7) -= 4;
    put_long(m68k_areg(regs, 7), oldpc);
    m68k_incpci(regs, offset);
}

STATIC_INLINE void m68k_do_jsr(struct regstruct *regs, uaecptr oldpc, uaecptr dest)
{
    m68k_areg(regs, 7) -= 4;
    put_long(m68k_areg(regs, 7), oldpc);
    m68k_setpc(regs, dest);
}

#define get_ibyte(regs, o) do_get_mem_byte((uae_u8 *)((regs)->pc_p + (o) + 1))
#define get_iword(regs, o) do_get_mem_word((uae_u16 *)((regs)->pc_p + (o)))
#define get_ilong(regs, o) do_get_mem_long((uae_u32 *)((regs)->pc_p + (o)))

#define get_iwordi(o) get_wordi(o)
#define get_ilongi(o) get_longi(o)

/* These are only used by the 68020/68881 code, and therefore don't
 * need to handle prefetch.  */
STATIC_INLINE uae_u32 next_ibyte (struct regstruct *regs)
{
    uae_u32 r = get_ibyte (regs, 0);
    m68k_incpc (regs, 2);
    return r;
}

STATIC_INLINE uae_u32 next_iword (struct regstruct *regs)
{
    uae_u32 r = get_iword (regs, 0);
    m68k_incpc (regs, 2);
    return r;
}
STATIC_INLINE uae_u32 next_iwordi (struct regstruct *regs)
{
    uae_u32 r = get_iwordi (m68k_getpci(regs));
    m68k_incpc (regs, 2);
    return r;
}

STATIC_INLINE uae_u32 next_ilong (struct regstruct *regs)
{
    uae_u32 r = get_ilong (regs, 0);
    m68k_incpc (regs, 4);
    return r;
}
STATIC_INLINE uae_u32 next_ilongi (struct regstruct *regs)
{
    uae_u32 r = get_ilongi (m68k_getpci(regs));
    m68k_incpc (regs, 4);
    return r;
}

STATIC_INLINE void m68k_setstopped (struct regstruct *regs, int stop)
{
    regs->stopped = stop;
    /* A traced STOP instruction drops through immediately without
       actually stopping.  */
    if (stop && (regs->spcflags & SPCFLAG_DOTRACE) == 0)
	set_special (regs, SPCFLAG_STOP);
}

extern uae_u32 REGPARAM3 get_disp_ea_020 (struct regstruct *regs, uae_u32 base, uae_u32 dp) REGPARAM;
extern uae_u32 REGPARAM3 get_disp_ea_020i (struct regstruct *regs, uae_u32 base, uae_u32 dp) REGPARAM;
extern uae_u32 REGPARAM3 get_disp_ea_000 (struct regstruct *regs, uae_u32 base, uae_u32 dp) REGPARAM;
extern int get_cpu_model(void);

extern void REGPARAM3 MakeSR (struct regstruct *regs) REGPARAM;
extern void REGPARAM3 MakeFromSR (struct regstruct *regs) REGPARAM;
extern void REGPARAM3 Exception (int, struct regstruct *regs, uaecptr) REGPARAM;
extern void NMI (void);
extern void doint (void);
extern int m68k_move2c (int, uae_u32 *);
extern int m68k_movec2 (int, uae_u32 *);
extern void m68k_divl (uae_u32, uae_u32, uae_u16, uaecptr);
extern void m68k_mull (uae_u32, uae_u32, uae_u16);
extern void init_m68k (void);
extern void init_m68k_full (void);
extern void m68k_go (int);
extern void m68k_reset (int);
extern int getDivu68kCycles(uae_u32 dividend, uae_u16 divisor);
extern int getDivs68kCycles(uae_s32 dividend, uae_s16 divisor);

STATIC_INLINE int bitset_count(uae_u32 data)
{
    unsigned int const MASK1  = 0x55555555;
    unsigned int const MASK2  = 0x33333333;
    unsigned int const MASK4  = 0x0f0f0f0f;
    unsigned int const MASK6 = 0x0000003f;

    unsigned int const w = (data & MASK1) + ((data >> 1) & MASK1);
    unsigned int const x = (w & MASK2) + ((w >> 2) & MASK2);
    unsigned int const y = ((x + (x >> 4)) & MASK4);
    unsigned int const z = (y + (y >> 8));
    unsigned int const c = (z + (z >> 16)) & MASK6;

    return c;
}

extern void mmu_op       (uae_u32, struct regstruct *regs, uae_u32);
extern void mmu_op30     (uaecptr, uae_u32, struct regstruct *regs, int, uaecptr);

extern void fpuop_arithmetic(uae_u32, struct regstruct *regs, uae_u16);
extern void fpuop_dbcc(uae_u32, struct regstruct *regs, uae_u16);
extern void fpuop_scc(uae_u32, struct regstruct *regs, uae_u16);
extern void fpuop_trapcc(uae_u32, struct regstruct *regs, uaecptr);
extern void fpuop_bcc(uae_u32, struct regstruct *regs, uaecptr, uae_u32);
extern void fpuop_save(uae_u32, struct regstruct *regs);
extern void fpuop_restore(uae_u32, struct regstruct *regs);
extern uae_u32 fpp_get_fpsr (const struct regstruct *regs);

extern void exception3 (uae_u32 opcode, uaecptr addr, uaecptr fault);
extern void exception3i (uae_u32 opcode, uaecptr addr, uaecptr fault);
extern void exception2 (uaecptr addr, uaecptr fault);
extern void cpureset (void);

extern void fill_prefetch_slow (struct regstruct *regs);

#define CPU_OP_NAME(a) op ## a

/* 68060 */
extern const struct cputbl op_smalltbl_0_ff[];
/* 68040 */
extern const struct cputbl op_smalltbl_1_ff[];
/* 68030 */
extern const struct cputbl op_smalltbl_2_ff[];
/* 68020 */
extern const struct cputbl op_smalltbl_3_ff[];
/* 68010 */
extern const struct cputbl op_smalltbl_4_ff[];
/* 68000 */
extern const struct cputbl op_smalltbl_5_ff[];
/* 68000 slow but compatible.  */
extern const struct cputbl op_smalltbl_11_ff[];
/* 68000 slow but compatible and cycle-exact.  */
extern const struct cputbl op_smalltbl_12_ff[];

extern cpuop_func *cpufunctbl[65536] ASM_SYM_FOR_FUNC ("cpufunctbl");


/* Flags for Bernie during development/debugging. Should go away eventually */
#define DISTRUST_CONSISTENT_MEM 0
#define TAGMASK 0x000fffff
#define TAGSIZE (TAGMASK+1)
#define MAXRUN 1024

extern uae_u8* start_pc_p;
extern uae_u32 start_pc;

#define cacheline(x) (((uintptr)x)&TAGMASK)

void newcpu_showstate(void);

#ifdef JIT
extern void (*flush_icache)(int n);
extern void compemu_reset(void);
extern int check_prefs_changed_comp (void);
#else
#define flush_icache(X) do {} while (0)
#endif

extern int movec_illg (int regno);
extern uae_u32 val_move2c (int regno);
struct cpum2c {
    int regno;
    char *regname;
};
extern struct cpum2c m2cregs[];
