/*
 * compiler/codegen_arm.cpp - ARM code generator
 *
 * Copyright (c) 2013 Jens Heitmann of ARAnyM dev team (see AUTHORS)
 *
 * Inspired by Christian Bauer's Basilisk II
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * JIT compiler m68k -> ARM
 *
 * Original 68040 JIT compiler for UAE, copyright 2000-2002 Bernd Meyer
 * Adaptation for Basilisk II and improvements, copyright 2000-2004 Gwenole Beauchesne
 * Portions related to CPU detection come from linux/arch/i386/kernel/setup.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Current state:
 * 	- Experimental
 *	- Still optimizable
 *	- Not clock cycle optimized
 *	- as a first step this compiler emulates x86 instruction to be compatible
 *	  with gencomp. Better would be a specialized version of gencomp compiling
 *	  68k instructions to ARM compatible instructions. This is a step for the
 *	  future
 *
 */

#include "flags_arm.h"

// Declare the built-in __clear_cache function.
extern void __clear_cache (char*, char*);

/*************************************************************************
 * Some basic information about the the target CPU                       *
 *************************************************************************/

#define R0_INDEX 0
#define R1_INDEX 1
#define R2_INDEX 2
#define R3_INDEX 3
#define R4_INDEX 4
#define R5_INDEX 5
#define R6_INDEX 6
#define R7_INDEX 7
#define R8_INDEX  8
#define R9_INDEX  9
#define R10_INDEX 10
#define R11_INDEX 11
#define R12_INDEX 12
#define R13_INDEX 13
#define R14_INDEX 14
#define R15_INDEX 15

#define RSP_INDEX 13
#define RLR_INDEX 14
#define RPC_INDEX 15

/* The register in which subroutines return an integer return value */
#define REG_RESULT R0_INDEX

/* The registers subroutines take their first and second argument in */
#define REG_PAR1 R0_INDEX
#define REG_PAR2 R1_INDEX

#define REG_WORK1 R2_INDEX
#define REG_WORK2 R3_INDEX

//#define REG_DATAPTR R10_INDEX

#define REG_PC_PRE R0_INDEX /* The register we use for preloading regs.pc_p */
#define REG_PC_TMP R1_INDEX /* Another register that is not the above */

#define MUL_NREG1 R0_INDEX /* %r4 will hold the low 32 bits after a 32x32 mul */
#define MUL_NREG2 R1_INDEX /* %r5 will hold the high 32 bits */

#define STACK_ALIGN		4
#define STACK_OFFSET	sizeof(void *)

#define R_REGSTRUCT 11
uae_s8 always_used[]={2,3,R_REGSTRUCT,12,-1}; // r12 is scratch register in C functions calls, I don't think it's save to use it here...

uae_u8 call_saved[]={0,0,0,0, 1,1,1,1, 1,1,1,1, 0,1,1,1};

/* This *should* be the same as call_saved. But:
   - We might not really know which registers are saved, and which aren't,
     so we need to preserve some, but don't want to rely on everyone else
     also saving those registers
   - Special registers (such like the stack pointer) should not be "preserved"
     by pushing, even though they are "saved" across function calls
*/
static const uae_u8 need_to_preserve[]={0,0,0,0, 1,1,1,1, 1,1,1,1, 1,0,0,0};
static const uae_u32 PRESERVE_MASK = ((1<<R4_INDEX)|(1<<R5_INDEX)|(1<<R6_INDEX)|(1<<R7_INDEX)|(1<<R8_INDEX)|(1<<R9_INDEX)
		|(1<<R10_INDEX)|(1<<R11_INDEX)|(1<<R12_INDEX));

#include "codegen_arm.h"


static inline void UNSIGNED8_IMM_2_REG(W4 r, IMM v) {
	MOV_ri8(r, (uae_u8) v);
}

static inline void SIGNED8_IMM_2_REG(W4 r, IMM v) {
	if (v & 0x80) {
		MVN_ri8(r, (uae_u8) ~v);
	} else {
		MOV_ri8(r, (uae_u8) v);
	}
}

static inline void UNSIGNED16_IMM_2_REG(W4 r, IMM v) {
	MOV_ri8(r, (uae_u8) v);
	ORR_rri8RORi(r, r, (uae_u8)(v >> 8), 24);
}

static inline void SIGNED16_IMM_2_REG(W4 r, IMM v) {
  uae_s32 offs = data_long_offs((uae_s32)(uae_s16) v);
  LDR_rRI(r, RPC_INDEX, offs);
}

static inline void UNSIGNED8_REG_2_REG(W4 d, RR4 s) {
	UXTB_rr(d, s);
}

static inline void SIGNED8_REG_2_REG(W4 d, RR4 s) {
	SXTB_rr(d, s);
}

static inline void UNSIGNED16_REG_2_REG(W4 d, RR4 s) {
	UXTH_rr(d, s);
}

static inline void SIGNED16_REG_2_REG(W4 d, RR4 s) {
	SXTH_rr(d, s);
}

#define ZERO_EXTEND_8_REG_2_REG(d,s) UNSIGNED8_REG_2_REG(d,s)
#define ZERO_EXTEND_16_REG_2_REG(d,s) UNSIGNED16_REG_2_REG(d,s)
#define SIGN_EXTEND_8_REG_2_REG(d,s) SIGNED8_REG_2_REG(d,s)
#define SIGN_EXTEND_16_REG_2_REG(d,s) SIGNED16_REG_2_REG(d,s)


#define jit_unimplemented(fmt, ...) do{ panicbug("**** Unimplemented ****\n"); panicbug(fmt, ## __VA_ARGS__); abort(); }while (0)

LOWFUNC(WRITE,NONE,2,raw_add_l,(RW4 d, RR4 s))
{
	ADD_rrr(d, d, s);
}
LENDFUNC(WRITE,NONE,2,raw_add_l,(RW4 d, RR4 s))

LOWFUNC(WRITE,NONE,2,raw_add_l_ri,(RW4 d, IMM i))
{
  if(i >= 0 && i < 256) {
    ADD_rri(d, d, i);
  } else {
    uae_s32 offs = data_long_offs(i);
  	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  	ADD_rrr(d, d, REG_WORK1);
  }
}
LENDFUNC(WRITE,NONE,2,raw_add_l_ri,(RW4 d, IMM i))

LOWFUNC(NONE,NONE,3,raw_lea_l_brr,(W4 d, RR4 s, IMM offset))
{
  uae_s32 offs = data_long_offs(offset);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs); 	// ldr     r2, [pc, #offs]
	ADD_rrr(d, s, REG_WORK1);         	  // add     r7, r6, r2
}
LENDFUNC(NONE,NONE,3,raw_lea_l_brr,(W4 d, RR4 s, IMM offset))

LOWFUNC(NONE,NONE,5,raw_lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IMM factor, IMM offset))
{
	int shft;
	switch(factor) {
  	case 1: shft=0; break;
  	case 2: shft=1; break;
  	case 4: shft=2; break;
  	case 8: shft=3; break;
  	default: abort();
	}

  SIGNED8_IMM_2_REG(REG_WORK1, offset);
  
	ADD_rrr(REG_WORK1, s, REG_WORK1);
	ADD_rrrLSLi(d, REG_WORK1, index, shft);
}
LENDFUNC(NONE,NONE,5,raw_lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IMM factor, IMM offset))

LOWFUNC(NONE,NONE,4,raw_lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))
{
	int shft;
	switch(factor) {
  	case 1: shft=0; break;
  	case 2: shft=1; break;
  	case 4: shft=2; break;
  	case 8: shft=3; break;
  	default: abort();
	}

	ADD_rrrLSLi(d, s, index, shft);
}
LENDFUNC(NONE,NONE,4,raw_lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))

LOWFUNC(NONE,NONE,2,raw_mov_b_ri,(W1 d, IMM s))
{
	BIC_rri(d, d, 0xff);      	// bic	%[d], %[d], #0xff
	ORR_rri(d, d, (s & 0xff)); 	// orr	%[d], %[d], #%[s]
}
LENDFUNC(NONE,NONE,2,raw_mov_b_ri,(W1 d, IMM s))

LOWFUNC(NONE,NONE,2,raw_mov_b_rr,(W1 d, RR1 s))
{
  BFI_rrii(d, s, 0, 7);             // bfi  %[d], %[s], 0, 7
}
LENDFUNC(NONE,NONE,2,raw_mov_b_rr,(W1 d, RR1 s))

LOWFUNC(NONE,WRITE,2,raw_mov_l_mi,(MEMW d, IMM s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 offs = data_long_offs(s);
    LDR_rRI(REG_WORK2, RPC_INDEX, offs);
    uae_s32 idx = d - (uae_u32) & regs;
    STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
  } else {
    data_check_end(8, 12);
    uae_s32 offs = data_long_offs(d);
  
  	LDR_rRI(REG_WORK1, RPC_INDEX, offs); 	// ldr    r2, [pc, #offs]    ; d
  
  	offs = data_long_offs(s);
  	LDR_rRI(REG_WORK2, RPC_INDEX, offs); 	// ldr    r3, [pc, #offs]    ; s
  
  	STR_rR(REG_WORK2, REG_WORK1);      	  // str    r3, [r2]
  }
}
LENDFUNC(NONE,WRITE,2,raw_mov_l_mi,(MEMW d, IMM s))

LOWFUNC(NONE,NONE,2,raw_mov_w_ri,(W2 d, IMM s))
{
  uae_s32 offs = data_word_offs(s);
	LDR_rRI(REG_WORK2, RPC_INDEX, offs);

  PKHBT_rrr(d, REG_WORK2, d);
}
LENDFUNC(NONE,NONE,2,raw_mov_w_ri,(W2 d, IMM s))

LOWFUNC(NONE,WRITE,2,raw_mov_l_mr,(IMM d, RR4 s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) &regs;
    STR_rRI(s, R_REGSTRUCT, idx);
  } else {
    uae_s32 offs = data_long_offs(d);
  	LDR_rRI(REG_WORK1, RPC_INDEX, offs); 	// ldr     r2, [pc, #offs]
  	STR_rR(s, REG_WORK1);             	  // str     r3, [r2]
  }
}
LENDFUNC(NONE,WRITE,2,raw_mov_l_mr,(IMM d, RR4 s))

LOWFUNC(NONE,NONE,2,raw_mov_w_rr,(W2 d, RR2 s))
{
  PKHBT_rrr(d, s, d);
}
LENDFUNC(NONE,NONE,2,raw_mov_w_rr,(W2 d, RR2 s))

LOWFUNC(WRITE,NONE,2,raw_shll_l_ri,(RW4 r, IMM i))
{
	LSL_rri(r,r, i & 0x1f);
}
LENDFUNC(WRITE,NONE,2,raw_shll_l_ri,(RW4 r, IMM i))

LOWFUNC(WRITE,NONE,2,raw_sub_l_ri,(RW4 d, IMM i))
{
  if(i >= 0 && i<256) {
    SUB_rri(d, d, i);
  } else {
    uae_s32 offs = data_long_offs(i);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);

	  SUB_rrr(d, d, REG_WORK1);
  }
}
LENDFUNC(WRITE,NONE,2,raw_sub_l_ri,(RW4 d, IMM i))

LOWFUNC(WRITE,NONE,2,raw_sub_w_ri,(RW2 d, IMM i))
{
  // This function is only called with i = 1
  // Caller needs flags...
  
	LSL_rri(REG_WORK2, d, 16);

	SUBS_rri(REG_WORK2, REG_WORK2, (i & 0xff) << 16);
  PKHTB_rrrASRi(d, d, REG_WORK2, 16);

	MRS_CPSR(REG_WORK1);                       	// mrs	r2, CPSR
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);	// eor	r2, r2, #0x20000000
	MSR_CPSR_r(REG_WORK1);                     	// msr	CPSR_fc, r2
}
LENDFUNC(WRITE,NONE,2,raw_sub_w_ri,(RW2 d, IMM i))

LOWFUNC(WRITE,NONE,2,raw_test_eq_w_rr,(RR2 d, RR2 s))
{
  // Only EQ/NE relevant for caller -> compare without handle carry
	LSL_rri(REG_WORK2, d, 16);
	TST_rrLSLi(REG_WORK2, s, 16);
}
LENDFUNC(WRITE,NONE,2,raw_test_eq_w_rr,(RR2 d, RR2 s))


static inline void raw_dec_sp(int off)
{
	if (off) {
    if(off >= 0 && off < 256) {
      SUB_rri(RSP_INDEX, RSP_INDEX, off);
    } else {
  		LDR_rRI(REG_WORK1, RPC_INDEX, 4);
  		SUB_rrr(RSP_INDEX, RSP_INDEX, REG_WORK1);
  		B_i(0);
  		//<value>:
  		emit_long(off);
  	}
	}
}

static inline void raw_inc_sp(int off)
{
	if (off) {
    if(off >= 0 && off < 256) {
      ADD_rri(RSP_INDEX, RSP_INDEX, off);
    } else {
  		LDR_rRI(REG_WORK1, RPC_INDEX, 4);
  		ADD_rrr(RSP_INDEX, RSP_INDEX, REG_WORK1);
  		B_i(0);
  		//<value>:
  		emit_long(off);
  	}
	}
}

static inline void raw_push_regs_to_preserve(void) {
	PUSH_REGS(PRESERVE_MASK);
}

static inline void raw_pop_preserved_regs(void) {
	POP_REGS(PRESERVE_MASK);
}

static inline void raw_load_flagx(uae_u32 t, uae_u32 r)
{
  LDR_rRI(t, R_REGSTRUCT, 17 * 4); // X flag are next to 8 Dregs, 8 Aregs and CPSR in struct regstruct
}

static inline void raw_flags_evicted(int r)
{
  live.state[FLAGTMP].status = INMEM;
  live.state[FLAGTMP].realreg = -1;
  /* We just "evicted" FLAGTMP. */
  if (live.nat[r].nholds != 1) {
      /* Huh? */
      abort();
  }
  live.nat[r].nholds = 0;
}

static inline void raw_flags_init(void) {
}

static inline void raw_flags_to_reg(int r)
{
	MRS_CPSR(r);
	STR_rRI(r, R_REGSTRUCT, 16 * 4);
	raw_flags_evicted(r);
}

static inline void raw_reg_to_flags(int r)
{
	MSR_CPSR_r(r);
}

static inline void raw_load_flagreg(uae_u32 t, uae_u32 r)
{
	LDR_rRI(t, R_REGSTRUCT, 16 * 4); // Flags are next to 8 Dregs and 8 Aregs in struct regstruct
}

/* %eax register is clobbered if target processor doesn't support fucomi */
#define FFLAG_NREG_CLOBBER_CONDITION 0
#define FFLAG_NREG R0_INDEX
#define FLAG_NREG2 -1
#define FLAG_NREG1 -1
#define FLAG_NREG3 -1

static inline void raw_fflags_into_flags(int r)
{
	jit_unimplemented("raw_fflags_into_flags %x", r);
}

static inline void raw_fp_init(void)
{
#ifdef USE_JIT_FPU
    int i;

    for (i=0;i<N_FREGS;i++)
	live.spos[i]=-2;
    live.tos=-1;  /* Stack is empty */
#endif
}

// Verify
static inline void raw_fp_cleanup_drop(void)
{
#ifdef USE_JIT_FPU
D(panicbug("raw_fp_cleanup_drop"));

    while (live.tos>=1) {
//	emit_byte(0xde);
//	emit_byte(0xd9);
	live.tos-=2;
    }
    while (live.tos>=0) {
//	emit_byte(0xdd);
//	emit_byte(0xd8);
	live.tos--;
    }
    raw_fp_init();
#endif
}

LOWFUNC(NONE,WRITE,2,raw_fmov_mr_drop,(MEMW m, FR r))
{
	jit_unimplemented("raw_fmov_mr_drop %x %x", m, r);
}
LENDFUNC(NONE,WRITE,2,raw_fmov_mr_drop,(MEMW m, FR r))

LOWFUNC(NONE,WRITE,2,raw_fmov_mr,(MEMW m, FR r))
{
	jit_unimplemented("raw_fmov_mr %x %x", m, r);
}
LENDFUNC(NONE,WRITE,2,raw_fmov_mr,(MEMW m, FR r))

LOWFUNC(NONE,READ,2,raw_fmov_rm,(FW r, MEMR m))
{
	jit_unimplemented("raw_fmov_rm %x %x", r, m);
}
LENDFUNC(NONE,READ,2,raw_fmov_rm,(FW r, MEMR m))

LOWFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))
{
	jit_unimplemented("raw_fmov_rr %x %x", d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))

static inline void raw_emit_nop_filler(int nbytes)
{
	nbytes >>= 2;
	while(nbytes--) { NOP(); }
}

static inline void raw_emit_nop(void)
{
  NOP();
}

static void raw_init_cpu(void)
{
	align_loops = 0;
	align_jumps = 0;

	raw_flags_init();
}

//
// Arm instructions
//
LOWFUNC(WRITE,NONE,2,raw_ADD_l_rr,(RW4 d, RR4 s))
{
	ADD_rrr(d, d, s);
}
LENDFUNC(WRITE,NONE,2,raw_ADD_l_rr,(RW4 d, RR4 s))

LOWFUNC(WRITE,NONE,2,raw_ADD_l_rri,(RW4 d, RR4 s, IMM i))
{
	ADD_rri(d, s, i);
}
LENDFUNC(WRITE,NONE,2,raw_ADD_l_rri,(RW4 d, RR4 s, IMM i))

LOWFUNC(WRITE,NONE,2,raw_SUB_l_rri,(RW4 d, RR4 s, IMM i))
{
	SUB_rri(d, s, i);
}
LENDFUNC(WRITE,NONE,2,raw_SUB_l_rri,(RW4 d, RR4 s, IMM i))

LOWFUNC(WRITE,NONE,2,raw_LDR_l_ri,(RW4 d, IMM i))
{
  uae_s32 offs = data_long_offs(i);
	LDR_rRI(d, RPC_INDEX, offs);
}
LENDFUNC(WRITE,NONE,2,raw_LDR_l_ri,(RW4 d, IMM i))

//
// compuemu_support used raw calls
//
LOWFUNC(WRITE,RMW,2,compemu_raw_add_l_mi,(IMM d, IMM s))
{
  data_check_end(8, 24);
  uae_s32 target = data_long(d, 24);
  uae_s32 offs = get_data_offset(target);
  
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);         		// ldr	r2, [pc, #offs]	; d
	LDR_rR(REG_WORK2, REG_WORK1);             			// ldr	r3, [r2]

  offs = data_long_offs(s);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);         		// ldr	r2, [pc, #offs]	; s

	ADD_rrr(REG_WORK2, REG_WORK2, REG_WORK1); 			// adds	r3, r3, r2

	offs = get_data_offset(target);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);          	// ldr	r2, [pc, #offs]	; d
	STR_rR(REG_WORK2, REG_WORK1);              			// str	r3, [r2]
}
LENDFUNC(WRITE,RMW,2,compemu_raw_add_l_mi,(IMM d, IMM s))

LOWFUNC(WRITE,NONE,2,compemu_raw_and_TAGMASK,(RW4 d))
{
  // TAGMASK is 0x0000ffff
  BFC_rii(d, 16, 31);
}
LENDFUNC(WRITE,NONE,2,compemu_raw_and_TAGMASK,(RW4 d))

LOWFUNC(NONE,NONE,1,compemu_raw_bswap_32,(RW4 r))
{
	REV_rr(r,r);								// rev 	   %[r],%[r]
}
LENDFUNC(NONE,NONE,1,compemu_raw_bswap_32,(RW4 r))

LOWFUNC(WRITE,READ,2,compemu_raw_cmp_l_mi,(MEMR d, IMM s))
{
  clobber_flags();
  
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 offs = data_long_offs(s);
    LDR_rRI(REG_WORK2, RPC_INDEX, offs);
    uae_s32 idx = d - (uae_u32) & regs;
    LDR_rRI(REG_WORK1, R_REGSTRUCT, idx);
  } else {
    data_check_end(8, 16);
    uae_s32 offs = data_long_offs(d);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	  LDR_rR(REG_WORK1, REG_WORK1);

	  offs = data_long_offs(s);
	  LDR_rRI(REG_WORK2, RPC_INDEX, offs);
  }
	CMP_rr(REG_WORK1, REG_WORK2);
}
LENDFUNC(WRITE,READ,2,compemu_raw_cmp_l_mi,(MEMR d, IMM s))

LOWFUNC(NONE,NONE,3,compemu_raw_lea_l_brr,(W4 d, RR4 s, IMM offset))
{
  if(offset >= 0 && offset < 256) {
    ADD_rri(d, s, offset);
  } else {
    uae_s32 offs = data_long_offs(offset);
  	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  	ADD_rrr(d, s, REG_WORK1);
  }
}
LENDFUNC(NONE,NONE,3,compemu_raw_lea_l_brr,(W4 d, RR4 s, IMM offset))

LOWFUNC(NONE,NONE,4,compemu_raw_lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))
{
	int shft;
	switch(factor) {
  	case 1: shft=0; break;
  	case 2: shft=1; break;
  	case 4: shft=2; break;
  	case 8: shft=3; break;
  	default: abort();
	}

	ADD_rrrLSLi(d, s, index, shft);
}
LENDFUNC(NONE,NONE,4,compemu_raw_lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_b_mr,(IMM d, RR1 s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    STRB_rRI(s, R_REGSTRUCT, idx);
  } else {
    uae_s32 offs = data_long_offs(d);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	  STRB_rR(s, REG_WORK1);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_b_mr,(IMM d, RR1 s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IMM s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 offs = data_long_offs(s);
    LDR_rRI(REG_WORK2, RPC_INDEX, offs);
    uae_s32 idx = d - (uae_u32) & regs;
    STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
  } else {
    data_check_end(8, 12);
    uae_s32 offs = data_long_offs(d);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	  offs = data_long_offs(s);
	  LDR_rRI(REG_WORK2, RPC_INDEX, offs);
	  STR_rR(REG_WORK2, REG_WORK1);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IMM s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(IMM d, RR4 s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    STR_rRI(s, R_REGSTRUCT, idx);
  } else {
    uae_s32 offs = data_long_offs(d);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	  STR_rR(s, REG_WORK1);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(IMM d, RR4 s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IMM s))
{
  uae_s32 offs = data_long_offs(s);
	LDR_rRI(d, RPC_INDEX, offs);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IMM s))

LOWFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))
{
  if(s >= (uae_u32) &regs && s < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = s - (uae_u32) & regs;
    LDR_rRI(d, R_REGSTRUCT, idx);
  } else {
    uae_s32 offs = data_long_offs(s);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	  LDR_rR(d, REG_WORK1);
  }
}
LENDFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))
{
	MOV_rr(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_w_mr,(IMM d, RR2 s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    STRH_rRI(s, R_REGSTRUCT, idx);
  } else {
    uae_s32 offs = data_long_offs(d);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	  STRH_rR(s, REG_WORK1);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_w_mr,(IMM d, RR2 s))

LOWFUNC(WRITE,RMW,2,compemu_raw_sub_l_mi,(MEMRW d, IMM s))
{
  clobber_flags();

  if(s >= 0 && s < 254) {
    if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
      uae_s32 idx = d - (uae_u32) & regs;
      LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
      SUBS_rri(REG_WORK2, REG_WORK2, s);
      STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
    } else {
    	uae_s32 offs = data_long_offs(d);
    	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
      LDR_rR(REG_WORK2, REG_WORK1);
      SUBS_rri(REG_WORK2, REG_WORK2, s);
      STR_rR(REG_WORK2, REG_WORK1);
    }
  } else {
    if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
      uae_s32 idx = d - (uae_u32) & regs;
  	  LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
  
  	  uae_s32 offs = data_long_offs(s);
  	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  	  SUBS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
  
  	  STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
    } else {
      data_check_end(8, 24);
      uae_s32 target = data_long(d, 24);
      uae_s32 offs = get_data_offset(target);
  	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  	  LDR_rR(REG_WORK2, REG_WORK1);
  
  	  offs = data_long_offs(s);
  	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  
  	  SUBS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
  
  	  offs = get_data_offset(target);
  	  LDR_rRI(REG_WORK1, RPC_INDEX, offs); 
  	  STR_rR(REG_WORK2, REG_WORK1);
    }
  }
}
LENDFUNC(WRITE,RMW,2,compemu_raw_sub_l_mi,(MEMRW d, IMM s))

LOWFUNC(WRITE,NONE,2,compemu_raw_test_l_rr,(RR4 d, RR4 s))
{
  clobber_flags();
	TST_rr(d, s);
}
LENDFUNC(WRITE,NONE,2,compemu_raw_test_l_rr,(RR4 d, RR4 s))

LOWFUNC(NONE,NONE,2,compemu_raw_zero_extend_16_rr,(W4 d, RR2 s))
{
	UXTH_rr(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_zero_extend_16_rr,(W4 d, RR2 s))

static inline void compemu_raw_call(uae_u32 t)
{
  uae_s32 offs = data_long_offs(t);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	PUSH(RLR_INDEX);
	BLX_r(REG_WORK1);
	POP(RLR_INDEX);
}

static inline void compemu_raw_call_r(RR4 r)
{
	PUSH(RLR_INDEX);
	BLX_r(r);
	POP(RLR_INDEX);
}

static inline void compemu_raw_jcc_l_oponly(int cc)
{
	switch (cc) {
	case 9: // LS
		BEQ_i(0);										// beq <dojmp>
		BCC_i(2);										// bcc <jp>

		//<dojmp>:
		LDR_rR(REG_WORK1, RPC_INDEX); 	        		// ldr	r2, [pc]	; <value>
		BX_r(REG_WORK1);								// bx r2
		break;

	case 8: // HI
		BEQ_i(3);										// beq <jp>
		BCS_i(2);										// bcs <jp>

		//<dojmp>:
		LDR_rR(REG_WORK1, RPC_INDEX);  			       	// ldr	r2, [pc]	; <value>
		BX_r(REG_WORK1);								// bx r2
		break;

	default:
		CC_LDR_rRI(cc, REG_WORK1, RPC_INDEX, 4); 		// ldrlt	r2, [pc, #4]	; <value>
		CC_BX_r(cc, REG_WORK1);							// bxlt 	r2
		B_i(0);                                         // b		<jp>
		break;
	}
  // emit of target will be done by caller
}

static inline void compemu_raw_jl(uae_u32 t)
{
  uae_s32 offs = data_long_offs(t);
	CC_LDR_rRI(NATIVE_CC_LT, RPC_INDEX, RPC_INDEX, offs);
}

static inline void compemu_raw_jmp(uae_u32 t)
{
	LDR_rR(REG_WORK1, RPC_INDEX);
	BX_r(REG_WORK1);
	emit_long(t);
}

static inline void compemu_raw_jmp_m_indexed(uae_u32 base, uae_u32 r, uae_u32 m)
{
	int shft;
	switch(m) {
  	case 1: shft=0; break;
  	case 2: shft=1; break;
  	case 4: shft=2; break;
  	case 8: shft=3; break;
  	default: abort();
	}

	LDR_rR(REG_WORK1, RPC_INDEX);
	LDR_rRR_LSLi(RPC_INDEX, REG_WORK1, r, shft);
	emit_long(base);
}

static inline void compemu_raw_jmp_r(RR4 r)
{
	BX_r(r);
}

static inline void compemu_raw_jnz(uae_u32 t)
{
  uae_s32 offs = data_long_offs(t);
	CC_LDR_rRI(NATIVE_CC_NE, RPC_INDEX, RPC_INDEX, offs); 		// ldrne   pc, [pc, offs]
}

static inline void compemu_raw_jz_b_oponly(void)
{
	BNE_i(2);									// bne jp
	LDRSB_rRI(REG_WORK1, RPC_INDEX, 3);			// ldrsb	r2,[pc,#3]
	ADD_rrr(RPC_INDEX, RPC_INDEX, REG_WORK1); 	// add		pc,pc,r2

	emit_byte(0);
	emit_byte(0);
	emit_byte(0);

	// <jp:>
}

static inline void compemu_raw_branch(IMM d)
{
	B_i((d >> 2) - 1);
}


// Optimize access to struct regstruct with r11

LOWFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMM s))
{
  uae_s32 offs = data_long_offs(s);
	LDR_rRI(R_REGSTRUCT, RPC_INDEX, offs);
}
LENDFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMM s))


// Handle end of compiled block
LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IMM cycles))
{
  clobber_flags();

  // countdown -= scaled_cycles(totcycles);
  uae_s32 offs = (uae_u32)&countdown - (uae_u32)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
	if(cycles >= 0 && cycles < 256) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
  	int offs2 = data_long_offs(cycles);
  	LDR_rRI(REG_WORK2, RPC_INDEX, offs2);
  	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);
	CC_B_i(NATIVE_CC_MI, 3);

	BFC_rii(rr_pc, 16, 31); // apply TAGMASK
	offs = data_long_offs((uintptr)cache_tags);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	LDR_rRR_LSLi(REG_WORK1, REG_WORK1, rr_pc, 2);
	BX_r(REG_WORK1);

	offs = data_long_offs((uintptr)popall_do_nothing);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	BX_r(REG_WORK1);
}
LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IMM cycles))


LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_isconst,(IMM cycles, IMM v))
{
  clobber_flags();

  // countdown -= scaled_cycles(totcycles);
  uae_s32 offs = (uae_u32)&countdown - (uae_u32)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
	if(cycles >= 0 && cycles < 256) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
  	int offs2 = data_long_offs(cycles);
  	LDR_rRI(REG_WORK2, RPC_INDEX, offs2);
  	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  CC_B_i(NATIVE_CC_MI, 1);

  LDR_rRI(REG_WORK1, RPC_INDEX, 16); // get target
  BX_r(REG_WORK1);
  
  offs = data_long_offs(v);
  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  offs = (uae_u32)&regs.pc_p - (uae_u32)&regs;
  STR_rRI(REG_WORK1, R_REGSTRUCT, offs);
	offs = data_long_offs((uintptr)popall_do_nothing);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	BX_r(REG_WORK1);
  
  // <target emitted by caller>
}
LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_isconst,(IMM cycles, IMM v))
