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

typedef uae_u32 REGPARAM3 cpuop_func (uae_u32, struct regstruct &regs) REGPARAM;
typedef void REGPARAM3 cpuop_func_ce (uae_u32, struct regstruct &regs) REGPARAM;

struct cputbl {
    cpuop_func *handler;
    uae_u16 opcode;
};

#ifdef JIT
typedef uae_u32 REGPARAM3 compop_func (uae_u32) REGPARAM;

struct comptbl {
  compop_func *handler;
	uae_u32		specific;
  uae_u32 opcode;
};
#endif

#define op_illg(op) _op_illg(op, regs)
extern uae_u32 REGPARAM3 _op_illg (uae_u32, struct regstruct &regs) REGPARAM;
#define op_unimpl() _op_unimpl(regs)
extern void REGPARAM3 _op_unimpl (struct regstruct &regs) REGPARAM;

typedef uae_u8 flagtype;

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

struct regstruct
{
  uae_u32 regs[16];
  struct flag_struct ccrflags;

  uae_u32 pc;
  uae_u8 *pc_p;
  uae_u8 *pc_oldp;
  uae_u32 instruction_pc;
  
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
#endif

  uae_u32 pcr;
  uae_u32 address_space_mask;

  uae_u8 panic;
  uae_u32 panic_pc, panic_addr;
  signed long pissoff;
};
extern unsigned long nextevent, is_syncline, currcycle;

extern struct regstruct regs;

#include "events.h"

STATIC_INLINE uae_u32 munge24(uae_u32 x)
{
    return x & regs.address_space_mask;
}

extern int cpu_cycles;

#define set_special(x) _set_special(regs, x)
STATIC_INLINE void _set_special (struct regstruct &regs, uae_u32 x)
{
	regs.spcflags |= x;
  cycles_do_special(regs);
}

#define unset_special(x) _unset_special(regs, x)
STATIC_INLINE void _unset_special (struct regstruct &regs, uae_u32 x)
{
	regs.spcflags &= ~x;
}

#define m68k_dreg(r,num) ((r).regs[(num)])
#define m68k_areg(r,num) (((r).regs + 8)[(num)])

#define m68k_setpc(newpc) _m68k_setpc(regs, newpc)
STATIC_INLINE void _m68k_setpc (struct regstruct &regs, uaecptr newpc)
{
  regs.pc_p = regs.pc_oldp = get_real_address (newpc);
  regs.instruction_pc = regs.pc = newpc;
}

#define m68k_getpc() _m68k_getpc(regs)
STATIC_INLINE uaecptr _m68k_getpc (struct regstruct &regs)
{
	return (uaecptr)(regs.pc + ((uae_u8*)regs.pc_p - (uae_u8*)regs.pc_oldp));
}
#define M68K_GETPC _m68k_getpc(regs)

#define m68k_incpc(o) ((regs).pc_p += (o))

#define m68k_do_rts() _m68k_do_rts(regs)
STATIC_INLINE void _m68k_do_rts (struct regstruct &regs)
{
  uae_u32 newpc = get_long (m68k_areg (regs, 7));
  m68k_setpc (newpc);
  m68k_areg(regs, 7) += 4;
}

STATIC_INLINE void m68k_do_bsr (struct regstruct &regs, uaecptr oldpc, uae_s32 offset)
{
  m68k_areg(regs, 7) -= 4;
  put_long(m68k_areg(regs, 7), oldpc);
  m68k_incpc (offset);
}

#define get_ibyte(o) do_get_mem_byte((uae_u8 *)((regs).pc_p + (o) + 1))
#define get_iword(o) do_get_mem_word((uae_u16 *)((regs).pc_p + (o)))
#define get_ilong(o) do_get_mem_long((uae_u32 *)((regs).pc_p + (o)))

#define get_iword2(regs,o) do_get_mem_word((uae_u16 *)((regs).pc_p + (o)))

/* These are only used by the 68020/68881 code, and therefore don't
 * need to handle prefetch.  */
STATIC_INLINE uae_u32 next_iword (struct regstruct &regs)
{
  uae_u32 r = get_iword (0);
  m68k_incpc (2);
  return r;
}
STATIC_INLINE uae_u32 next_ilong (struct regstruct &regs)
{
  uae_u32 r = get_ilong (0);
  m68k_incpc (4);
  return r;
}

#define x_get_word get_word
#define x_get_long get_long
#define x_put_byte put_byte
#define x_put_word put_word
#define x_put_long put_long
#define x_next_iword() next_iword(regs)
#define x_next_ilong() next_ilong(regs)

extern void m68k_setstopped (void);
extern void m68k_resumestopped (void);

#define x_get_disp_ea_020(base,dp) _get_disp_ea_020(regs, base, dp)
#define get_disp_ea_020(base,dp) _get_disp_ea_020(regs, base, dp)
extern uae_u32 REGPARAM3 _get_disp_ea_020 (struct regstruct &regs, uae_u32 base, uae_u32 dp) REGPARAM;

extern uae_u32 REGPARAM3 get_bitfield (uae_u32 src, uae_u32 bdata[2], uae_s32 offset, int width) REGPARAM;
extern void REGPARAM3 put_bitfield (uae_u32 dst, uae_u32 bdata[2], uae_u32 val, uae_s32 offset, int width) REGPARAM;

extern int get_cpu_model(void);

extern void REGPARAM3 MakeSR (struct regstruct &regs) REGPARAM;
extern void REGPARAM3 MakeFromSR (struct regstruct &regs) REGPARAM;
#define Exception(nr) _Exception(nr, regs)
extern void REGPARAM3 _Exception (int, struct regstruct &regs) REGPARAM;
extern void doint (void);
extern void dump_counts (void);
extern int m68k_move2c (int, uae_u32 *);
extern int m68k_movec2 (int, uae_u32 *);
extern void m68k_divl (uae_u32, uae_u32, uae_u16);
extern void m68k_mull (uae_u32, uae_u32, uae_u16);
extern void init_m68k (void);
extern void m68k_go (int);
extern void m68k_reset (int);
extern int getDivu68kCycles(uae_u32 dividend, uae_u16 divisor);
extern int getDivs68kCycles(uae_s32 dividend, uae_s16 divisor);
extern void protect_roms (bool);

STATIC_INLINE int bitset_count16(uae_u16 data)
{
  unsigned int const MASK1  = 0x5555;
  unsigned int const MASK2  = 0x3333;
  unsigned int const MASK4  = 0x0f0f;
  unsigned int const MASK6  = 0x003f;

  unsigned int const w = (data & MASK1) + ((data >> 1) & MASK1);
  unsigned int const x = (w & MASK2) + ((w >> 2) & MASK2);
  unsigned int const y = ((x + (x >> 4)) & MASK4);
  unsigned int const z = (y + (y >> 8)) & MASK6;

  return z;
}

extern void mmu_op (uae_u32, struct regstruct &regs, uae_u32);
extern void mmu_op30 (uaecptr, uae_u32, struct regstruct &regs, uae_u16, uaecptr);

extern void fpuop_arithmetic(uae_u32, struct regstruct &regs, uae_u16);
extern void fpuop_dbcc(uae_u32, struct regstruct &regs, uae_u16);
extern void fpuop_scc(uae_u32, struct regstruct &regs, uae_u16);
extern void fpuop_trapcc(uae_u32, struct regstruct &regs, uaecptr, uae_u16);
extern void fpuop_bcc(uae_u32, struct regstruct &regs, uaecptr, uae_u32);
extern void fpuop_save(uae_u32, struct regstruct &regs);
extern void fpuop_restore(uae_u32, struct regstruct &regs);
extern void fpu_reset (void);

extern void exception3 (uae_u32 opcode, uaecptr addr);
extern void exception3i (uae_u32 opcode, uaecptr addr);
extern void exception3 (uae_u32 opcode, uaecptr addr, int w, int i, uaecptr pc);
extern void exception2 (uaecptr addr);
extern void cpureset (void);

extern void fill_prefetch (void);

#define CPU_OP_NAME(a) op ## a

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
extern const struct cputbl op_smalltbl_11_ff[]; // prefetch

extern cpuop_func *cpufunctbl[65536] ASM_SYM_FOR_FUNC ("cpufunctbl");

#ifdef JIT
extern void (*flush_icache)(uaecptr, int);
extern void compemu_reset(void);
extern bool check_prefs_changed_comp (void);
#else
#define flush_icache(uaecptr, int) do {} while (0)
#endif

extern int movec_illg (int regno);
