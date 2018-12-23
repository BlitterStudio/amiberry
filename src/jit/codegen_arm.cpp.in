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
#define REG_WORK3 R12_INDEX

//#define REG_DATAPTR R10_INDEX

#define REG_PC_TMP R1_INDEX /* Another register that is not the above */

#define R_MEMSTART 10
#define R_REGSTRUCT 11
uae_s8 always_used[]={2,3,R_MEMSTART,R_REGSTRUCT,12,-1}; // r2, r3 and r12 are work register in emitted code

uae_u8 call_saved[]={0,0,0,0, 1,1,1,1, 1,1,1,1, 0,1,1,1};

/* This *should* be the same as call_saved. But:
   - We might not really know which registers are saved, and which aren't,
     so we need to preserve some, but don't want to rely on everyone else
     also saving those registers
   - Special registers (such like the stack pointer) should not be "preserved"
     by pushing, even though they are "saved" across function calls
*/
/* Without save and restore R12, we sometimes get seg faults when entering gui...
   Don't understand why. */
static const uae_u8 need_to_preserve[]={0,0,0,0, 1,1,1,1, 1,1,1,1, 1,0,0,0};
static const uae_u32 PRESERVE_MASK = ((1<<R4_INDEX)|(1<<R5_INDEX)|(1<<R6_INDEX)|(1<<R7_INDEX)|(1<<R8_INDEX)|(1<<R9_INDEX)
		|(1<<R10_INDEX)|(1<<R11_INDEX)|(1<<R12_INDEX));

#include "codegen_arm.h"

STATIC_INLINE void UNSIGNED8_IMM_2_REG(W4 r, IMM v) {
	MOV_ri8(r, (uae_u8) v);
}

STATIC_INLINE void SIGNED8_IMM_2_REG(W4 r, IMM v) {
	if (v & 0x80) {
		MVN_ri8(r, (uae_u8) ~v);
	} else {
		MOV_ri8(r, (uae_u8) v);
	}
}

STATIC_INLINE void UNSIGNED16_IMM_2_REG(W4 r, IMM v) {
#ifdef ARMV6T2
  MOVW_ri16(r, v);
#else
	MOV_ri8(r, (uae_u8) v);
	ORR_rri8RORi(r, r, (uae_u8)(v >> 8), 24);
#endif
}

STATIC_INLINE void SIGNED16_IMM_2_REG(W4 r, IMM v) {
#ifdef ARMV6T2
  MOVW_ri16(r, v);
  SXTH_rr(r, r);
#else
  uae_s32 offs = data_long_offs((uae_s32)(uae_s16) v);
  LDR_rRI(r, RPC_INDEX, offs);
#endif
}

STATIC_INLINE void UNSIGNED8_REG_2_REG(W4 d, RR4 s) {
	UXTB_rr(d, s);
}

STATIC_INLINE void SIGNED8_REG_2_REG(W4 d, RR4 s) {
	SXTB_rr(d, s);
}

STATIC_INLINE void UNSIGNED16_REG_2_REG(W4 d, RR4 s) {
	UXTH_rr(d, s);
}

STATIC_INLINE void SIGNED16_REG_2_REG(W4 d, RR4 s) {
	SXTH_rr(d, s);
}

#define ZERO_EXTEND_8_REG_2_REG(d,s) UNSIGNED8_REG_2_REG(d,s)
#define ZERO_EXTEND_16_REG_2_REG(d,s) UNSIGNED16_REG_2_REG(d,s)
#define SIGN_EXTEND_8_REG_2_REG(d,s) SIGNED8_REG_2_REG(d,s)
#define SIGN_EXTEND_16_REG_2_REG(d,s) SIGNED16_REG_2_REG(d,s)


STATIC_INLINE void raw_push_regs_to_preserve(void) {
	PUSH_REGS(PRESERVE_MASK);
}

STATIC_INLINE void raw_pop_preserved_regs(void) {
	POP_REGS(PRESERVE_MASK);
}

STATIC_INLINE void raw_flags_evicted(int r)
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

STATIC_INLINE void raw_flags_to_reg(int r)
{
	MRS_CPSR(r);
	STR_rRI(r, R_REGSTRUCT, 16 * 4); // Flags are next to 8 Dregs and 8 Aregs in struct regstruct
	raw_flags_evicted(r);
}

STATIC_INLINE void raw_reg_to_flags(int r)
{
	MSR_CPSRf_r(r);
}

//
// compuemu_support used raw calls
//
LOWFUNC(WRITE,RMW,2,compemu_raw_add_l_mi,(IMM d, IMM s))
{
#ifdef ARMV6T2
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
	} else {
    MOVW_ri16(REG_WORK1, d);
    MOVT_ri16(REG_WORK1, d >> 16);
    LDR_rR(REG_WORK2, REG_WORK1);
	}

	if(CHECK32(s)) {
    ADD_rri(REG_WORK2, REG_WORK2, s);
	} else {
    MOVW_ri16(REG_WORK3, s);
    if(s >> 16)
      MOVT_ri16(REG_WORK3, s >> 16);
    ADD_rrr(REG_WORK2, REG_WORK2, REG_WORK3);
	}

  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
	} else {
    STR_rR(REG_WORK2, REG_WORK1);  
	}
#else
  uae_s32 offs = data_long_offs(d);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
	LDR_rR(REG_WORK2, REG_WORK1);

  offs = data_long_offs(s);
	LDR_rRI(REG_WORK3, RPC_INDEX, offs);

	ADD_rrr(REG_WORK2, REG_WORK2, REG_WORK3);

	STR_rR(REG_WORK2, REG_WORK1);
#endif
}
LENDFUNC(WRITE,RMW,2,compemu_raw_add_l_mi,(IMM d, IMM s))

LOWFUNC(WRITE,READ,2,compemu_raw_cmp_l_mi,(MEMR d, IMM s))
{
  clobber_flags();
  
 #ifdef ARMV6T2
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    LDR_rRI(REG_WORK1, R_REGSTRUCT, idx);
    MOVW_ri16(REG_WORK2, s);
    if(s >> 16)
      MOVT_ri16(REG_WORK2, s >> 16);
  } else {
    MOVW_ri16(REG_WORK1, d);
    MOVT_ri16(REG_WORK1, d >> 16);
	  LDR_rR(REG_WORK1, REG_WORK1);
    MOVW_ri16(REG_WORK2, s);
    if(s >> 16)
      MOVT_ri16(REG_WORK2, s >> 16);
  }
#else
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
#endif

	CMP_rr(REG_WORK1, REG_WORK2);
}
LENDFUNC(WRITE,READ,2,compemu_raw_cmp_l_mi,(MEMR d, IMM s))

LOWFUNC(NONE,NONE,3,compemu_raw_lea_l_brr,(W4 d, RR4 s, IMM offset))
{
  if(CHECK32(offset)) {
    ADD_rri(d, s, offset);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, offset);
    if(offset >> 16)
      MOVT_ri16(REG_WORK1, offset >> 16);
#else
    uae_s32 offs = data_long_offs(offset);
  	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
  	ADD_rrr(d, s, REG_WORK1);
  }
}
LENDFUNC(NONE,NONE,3,compemu_raw_lea_l_brr,(W4 d, RR4 s, IMM offset))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IMM s))
{
#ifdef ARMV6T2
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    MOVW_ri16(REG_WORK2, s);
    if(s >> 16)
      MOVT_ri16(REG_WORK2, s >> 16);
    uae_s32 idx = d - (uae_u32) & regs;
    STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
  } else {
    MOVW_ri16(REG_WORK1, d);
    MOVT_ri16(REG_WORK1, d >> 16);
    MOVW_ri16(REG_WORK2, s);
    if(s >> 16)
      MOVT_ri16(REG_WORK2, s >> 16);
	  STR_rR(REG_WORK2, REG_WORK1);
  }
#else
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
#endif
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IMM s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))
{
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) & regs;
    STR_rRI(s, R_REGSTRUCT, idx);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, d);
    MOVT_ri16(REG_WORK1, d >> 16);
#else
    uae_s32 offs = data_long_offs(d);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	  STR_rR(s, REG_WORK1);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IMM s))
{
#ifdef ARMV6T2
  MOVW_ri16(d, s);
  if(s >> 16)
    MOVT_ri16(d, s >> 16);
#else
  uae_s32 offs = data_long_offs(s);
	LDR_rRI(d, RPC_INDEX, offs);
#endif
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IMM s))

LOWFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))
{
  if(s >= (uae_u32) &regs && s < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = s - (uae_u32) & regs;
    LDR_rRI(d, R_REGSTRUCT, idx);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, s);
    MOVT_ri16(REG_WORK1, s >> 16);
#else
    uae_s32 offs = data_long_offs(s);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	  LDR_rR(d, REG_WORK1);
  }
}
LENDFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))
{
	MOV_rr(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))

LOWFUNC(WRITE,RMW,2,compemu_raw_sub_l_mi,(MEMRW d, IMM s))
{
  clobber_flags();

  if(CHECK32(s)) {
    if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
      uae_s32 idx = d - (uae_u32) & regs;
      LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
      SUBS_rri(REG_WORK2, REG_WORK2, s);
      STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
    } else {
#ifdef ARMV6T2
      MOVW_ri16(REG_WORK1, d);
      MOVT_ri16(REG_WORK1, d >> 16);
#else
    	uae_s32 offs = data_long_offs(d);
    	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
      LDR_rR(REG_WORK2, REG_WORK1);
      SUBS_rri(REG_WORK2, REG_WORK2, s);
      STR_rR(REG_WORK2, REG_WORK1);
    }
  } else {
    if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
      uae_s32 idx = d - (uae_u32) & regs;
  	  LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
  
#ifdef ARMV6T2
      MOVW_ri16(REG_WORK1, s);
      if(s >> 16)
        MOVT_ri16(REG_WORK1, s >> 16);
#else
  	  uae_s32 offs = data_long_offs(s);
  	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif

  	  SUBS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
  	  STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
    } else {
#ifdef ARMV6T2
      MOVW_ri16(REG_WORK1, d);
      MOVT_ri16(REG_WORK1, d >> 16);
  	  LDR_rR(REG_WORK2, REG_WORK1);
  
      MOVW_ri16(REG_WORK3, s);
      if(s >> 16)
        MOVT_ri16(REG_WORK3, s >> 16);
#else
      uae_s32 offs = data_long_offs(d);
  	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
  	  LDR_rR(REG_WORK2, REG_WORK1);
  
  	  offs = data_long_offs(s);
  	  LDR_rRI(REG_WORK3, RPC_INDEX, offs);
#endif
  
  	  SUBS_rrr(REG_WORK2, REG_WORK2, REG_WORK3);
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

STATIC_INLINE void compemu_raw_call(uae_u32 t)
{
#ifdef ARMV6T2
  MOVW_ri16(REG_WORK1, t);
  MOVT_ri16(REG_WORK1, t >> 16);
#else
  uae_s32 offs = data_long_offs(t);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	PUSH(RLR_INDEX);
	BLX_r(REG_WORK1);
	POP(RLR_INDEX);
}

STATIC_INLINE void compemu_raw_call_r(RR4 r)
{
	PUSH(RLR_INDEX);
	BLX_r(r);
	POP(RLR_INDEX);
}

STATIC_INLINE void compemu_raw_jcc_l_oponly(int cc)
{
	switch (cc) {
		case NATIVE_CC_HI: // HI
			BEQ_i(0);										// beq no jump
			BCC_i(0);										// bcc jump
			break;

		case NATIVE_CC_LS: // LS
			BEQ_i(0);										// beq jump
			BCC_i(0);										// bcc no jump
			// jump
			B_i(0); 
			// no jump
			break;

		case NATIVE_CC_F_OGT: // Jump if valid and greater than
			BVS_i(0);		// do not jump if NaN
			BGT_i(0);		// jump if greater than
			break;

		case NATIVE_CC_F_OGE: // Jump if valid and greater or equal
			BVS_i(0);		// do not jump if NaN
			BCS_i(0);		// jump if carry set
			break;
			
		case NATIVE_CC_F_OLT: // Jump if vaild and less than
			BVS_i(0);		// do not jump if NaN
			BCC_i(0);		// jump if carry cleared
			break;
			
		case NATIVE_CC_F_OLE: // Jump if valid and less or equal
			BVS_i(0);		// do not jump if NaN
			BLE_i(0);		// jump if less or equal
			break;
			
		case NATIVE_CC_F_OGL: // Jump if valid and greator or less
			BVS_i(0);		// do not jump if NaN
			BNE_i(0);		// jump if not equal
			break;

		case NATIVE_CC_F_OR: // Jump if valid
			BVC_i(0);
			break;
			
		case NATIVE_CC_F_UN: // Jump if NAN
			BVS_i(0); 
			break;

		case NATIVE_CC_F_UEQ: // Jump if NAN or equal
			BVS_i(0); 	// jump if NaN
			BNE_i(0);		// do not jump if greater or less
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_UGT: // Jump if NAN or greater than
			BVS_i(0); 	// jump if NaN
			BLS_i(0);		// do not jump if lower or same
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_UGE: // Jump if NAN or greater or equal
			BVS_i(0); 	// jump if NaN
			BMI_i(0);		// do not jump if lower
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_ULT: // Jump if NAN or less than
			BVS_i(0); 	// jump if NaN
			BGE_i(0);		// do not jump if greater or equal
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_ULE: // Jump if NAN or less or equal
			BVS_i(0); 	// jump if NaN
			BGT_i(0);		// do not jump if greater
			// jump
			B_i(0); 
			break;
	
		default:
	    CC_B_i(cc, 0);
			break;
	}
  // emit of target into last branch will be done by caller
}

STATIC_INLINE void compemu_raw_handle_except(IMM cycles)
{
	uae_u32* branchadd;	
	int offs;

  clobber_flags();

#ifdef ARMV6T2
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
#else
	offs = data_long_offs((uae_u32)(&jit_exception));
	LDR_rRI(REG_WORK2, RPC_INDEX, offs);
#endif
  LDR_rR(REG_WORK1, REG_WORK2);
	TST_rr(REG_WORK1, REG_WORK1);

	branchadd = (uae_u32*)get_target();
	BEQ_i(0);		// no exception, jump to next instruction
	
  // countdown -= scaled_cycles(totcycles);
  offs = (uae_u32)&countdown - (uae_u32)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  if(CHECK32(cycles)) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK2, cycles);
    if(cycles >> 16)
      MOVT_ri16(REG_WORK2, cycles >> 16);
#else
  	int offs2 = data_long_offs(cycles);
  	LDR_rRI(REG_WORK2, RPC_INDEX, offs2);
#endif
  	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);

  raw_pop_preserved_regs();
  LDR_rRI(RPC_INDEX, RPC_INDEX, -4); // <execute_exception>
	emit_long((uintptr)execute_exception);
	
	// Write target of next instruction
	write_jmp_target(branchadd, (uintptr)get_target());
}

STATIC_INLINE void compemu_raw_maybe_recompile(uae_u32 t)
{
  BGE_i(2);
  raw_pop_preserved_regs();
  LDR_rRI(RPC_INDEX, RPC_INDEX, -4);
  emit_long(t);
}

STATIC_INLINE void compemu_raw_jmp(uae_u32 t)
{
	if(t >= (uae_u32)popallspace && t < (uae_u32)(popallspace + POPALLSPACE_SIZE + MAX_JIT_CACHE * 1024)) {
		uae_u32* loc = (uae_u32*)get_target();
		B_i(0);
		write_jmp_target(loc, t);
	} else {
    LDR_rRI(RPC_INDEX, RPC_INDEX, -4);
	  emit_long(t);
	}
}

STATIC_INLINE void compemu_raw_jmp_m_indexed(uae_u32 base, uae_u32 r, uae_u32 m)
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

STATIC_INLINE void compemu_raw_maybe_cachemiss(uae_u32 t)
{
  BEQ_i(2);
  raw_pop_preserved_regs();
  LDR_rRI(RPC_INDEX, RPC_INDEX, -4);
  emit_long(t);
}

STATIC_INLINE void compemu_raw_jz_b_oponly(void)
{
  BEQ_i(0);   // Real distance set by caller
}

STATIC_INLINE void compemu_raw_branch(IMM d)
{
	B_i((d >> 2) - 1);
}


// Optimize access to struct regstruct with r11 and memory with r10

LOWFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMM s))
{
#ifdef ARMV6T2
  MOVW_ri16(R_REGSTRUCT, s);
  MOVT_ri16(R_REGSTRUCT, s >> 16);
#else
  uae_s32 offs = data_long_offs(s);
	LDR_rRI(R_REGSTRUCT, RPC_INDEX, offs);
#endif
	uae_s32 offsmem = (uae_u32)&NATMEM_OFFSETX - (uae_u32) &regs;
  LDR_rRI(R_MEMSTART, R_REGSTRUCT, offsmem);
}
LENDFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMM s))


// Handle end of compiled block
LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IMM cycles))
{
  clobber_flags();

  // countdown -= scaled_cycles(totcycles);
  uae_s32 offs = (uae_u32)&countdown - (uae_u32)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  if(CHECK32(cycles)) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK2, cycles);
    if(cycles >> 16)
      MOVT_ri16(REG_WORK2, cycles >> 16);
#else
  	int offs2 = data_long_offs(cycles);
  	LDR_rRI(REG_WORK2, RPC_INDEX, offs2);
#endif
  	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);

#ifdef ARMV6T2
	CC_B_i(NATIVE_CC_MI, 2);
	BFC_rii(rr_pc, 16, 31); // apply TAGMASK
#else
	CC_B_i(NATIVE_CC_MI, 3);
	BIC_rri(rr_pc, rr_pc, 0x00ff0000);
	BIC_rri(rr_pc, rr_pc, 0xff000000);
#endif
  LDR_rRI(REG_WORK1, RPC_INDEX, 8); // <cache_tags>
	LDR_rRR_LSLi(RPC_INDEX, REG_WORK1, rr_pc, 2);

  raw_pop_preserved_regs();
  LDR_rRI(RPC_INDEX, RPC_INDEX, 0); // <do_nothing>

	emit_long((uintptr)cache_tags);
	emit_long((uintptr)do_nothing);
}
LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IMM cycles))


//LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_isconst,(IMM cycles, IMM v))
STATIC_INLINE uae_u32* compemu_raw_endblock_pc_isconst(IMM cycles, IMM v)
{
	uae_u32* tba;
  clobber_flags();

  // countdown -= scaled_cycles(totcycles);
  uae_s32 offs = (uae_u32)&countdown - (uae_u32)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  if(CHECK32(cycles)) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK2, cycles);
    if(cycles >> 16)
      MOVT_ri16(REG_WORK2, cycles >> 16);
#else
  	int offs2 = data_long_offs(cycles);
  	LDR_rRI(REG_WORK2, RPC_INDEX, offs2);
#endif
  	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);

	tba = (uae_u32*)get_target();
  CC_B_i(NATIVE_CC_MI^1, 0); // <target set by caller>
  
  LDR_rRI(REG_WORK1, RPC_INDEX, 8); // <v>
  offs = (uae_u32)&regs.pc_p - (uae_u32)&regs;
  STR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  raw_pop_preserved_regs();
  LDR_rRI(RPC_INDEX, RPC_INDEX, 0); // <do_nothing>

	emit_long(v);
	emit_long((uintptr)do_nothing);

	return tba;  
}
//LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_isconst,(IMM cycles, IMM v))


LOWFUNC(NONE,READ,2,compemu_raw_tag_pc,(W4 d, MEMR s))
{
  uae_s32 idx = (uae_u32)(s) - (uae_u32)&regs;
  LDRH_rRI(d, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,READ,2,compemu_raw_tag_pc,(W4 d, MEMR s))

/*************************************************************************
* FPU stuff                                                             *
*************************************************************************/

LOWFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))
{
	VMOV64_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_fmov_mr_drop,(MEMW mem, FR s))
{
  if(mem >= (uae_u32) &regs && mem < (uae_u32) &regs + 1020 && ((mem - (uae_u32) &regs) & 0x3) == 0) {
    VSTR64_dRi(s, R_REGSTRUCT, (mem - (uae_u32) &regs));
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, mem);
    MOVT_ri16(REG_WORK1, mem >> 16);
#else
	  auto offs = data_long_offs(mem);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
    VSTR64_dRi(s, REG_WORK1, 0);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_fmov_mr_drop,(MEMW mem, FR s))


LOWFUNC(NONE,READ,2,compemu_raw_fmov_rm,(FW d, MEMR mem))
{
  if(mem >= (uae_u32) &regs && mem < (uae_u32) &regs + 1020 && ((mem - (uae_u32) &regs) & 0x3) == 0) {
    VLDR64_dRi(d, R_REGSTRUCT, (mem - (uae_u32) &regs));
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, mem);
    MOVT_ri16(REG_WORK1, mem >> 16);
#else
	  auto offs = data_long_offs(mem);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
    VLDR64_dRi(d, REG_WORK1, 0);
  }
}
LENDFUNC(NONE,READ,2,compemu_raw_fmov_rm,(FW d, MEMW mem))

LOWFUNC(NONE,NONE,2,raw_fmov_l_rr,(FW d, RR4 s))
{
  VMOVi_from_ARM_dr(SCRATCH_F64_1, s, 0);
  VCVTIto64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_l_rr,(FW d, RR4 s))

LOWFUNC(NONE,NONE,2,raw_fmov_s_rr,(FW d, RR4 s))
{
  VMOV32_sr(SCRATCH_F32_1, s);
  VCVT32to64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_s_rr,(FW d, RR4 s))

LOWFUNC(NONE,NONE,2,raw_fmov_w_rr,(FW d, RR2 s))
{
  SIGN_EXTEND_16_REG_2_REG(REG_WORK1, s);
  VMOVi_from_ARM_dr(SCRATCH_F64_1, REG_WORK1, 0);
  VCVTIto64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_w_rr,(FW d, RR2 s))

LOWFUNC(NONE,NONE,2,raw_fmov_b_rr,(FW d, RR1 s))
{
  SIGN_EXTEND_8_REG_2_REG(REG_WORK1, s);
  VMOVi_from_ARM_dr(SCRATCH_F64_1, REG_WORK1, 0);
  VCVTIto64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_b_rr,(FW d, RR1 s))

LOWFUNC(NONE,NONE,2,raw_fmov_d_rrr,(FW d, RR4 s1, RR4 s2))
{
  VMOV64_drr(d, s1, s2);
}
LENDFUNC(NONE,NONE,2,raw_fmov_d_rrr,(FW d, RR4 s1, RR4 s2))

LOWFUNC(NONE,NONE,2,raw_fmov_to_l_rr,(W4 d, FR s))
{
  VCVTR64toI_sd(SCRATCH_F32_1, s);
  VMOV32_rs(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_to_l_rr,(W4 d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmov_to_s_rr,(W4 d, FR s))
{
  VCVT64to32_sd(SCRATCH_F32_1, s);
  VMOV32_rs(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_to_s_rr,(W4 d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmov_to_w_rr,(W4 d, FR s, int targetIsReg))
{
  VCVTR64toI_sd(SCRATCH_F32_1, s);
  VMOV32_rs(REG_WORK1, SCRATCH_F32_1);
  if(targetIsReg) {
		SSAT_rir(REG_WORK1, 15, REG_WORK1);
	  PKHTB_rrr(d, d, REG_WORK1);
  } else {
		SSAT_rir(d, 15, REG_WORK1);
	}
}
LENDFUNC(NONE,NONE,2,raw_fmov_to_w_rr,(W4 d, FR s, int targetIsReg))

LOWFUNC(NONE,NONE,3,raw_fmov_to_b_rr,(W4 d, FR s, int targetIsReg))
{
  VCVTR64toI_sd(SCRATCH_F32_1, s);
  VMOV32_rs(REG_WORK1, SCRATCH_F32_1);
  if(targetIsReg) {
	  SSAT_rir(REG_WORK1, 7, REG_WORK1);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
  } else {
	  SSAT_rir(d, 7, REG_WORK1);
	}
}
LENDFUNC(NONE,NONE,3,raw_fmov_to_b_rr,(W4 d, FR s, int targetIsReg))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_0,(FW r))
{
	VMOV_I64_dimmI(r, 0x00);		// load imm #0 into reg
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_0,(FW r))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_1,(FW r))
{
  VMOV_F64_dimmF(r, 0x70); // load imm #1 into reg
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_1,(FW r))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_10,(FW r))
{
  VMOV_F64_dimmF(r, 0x24); // load imm #10 into reg
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_10,(FW r))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_100,(FW r))
{
  VMOV_F64_dimmF(r, 0x24); // load imm #10 into reg
  VMUL64_ddd(r, r, r);
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_10,(FW r))

LOWFUNC(NONE,READ,2,raw_fmov_d_rm,(FW r, MEMR m))
{
#ifdef ARMV6T2
  MOVW_ri16(REG_WORK1, m);
  MOVT_ri16(REG_WORK1, m >> 16);
#else
	auto offs = data_long_offs(m);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
  VLDR64_dRi(r, REG_WORK1, 0);
}
LENDFUNC(NONE,READ,2,raw_fmov_d_rm,(FW r, MEMR m))

LOWFUNC(NONE,READ,2,raw_fmovs_rm,(FW r, MEMR m))
{
#ifdef ARMV6T2
	MOVW_ri16(REG_WORK1, m);
	MOVT_ri16(REG_WORK1, m >> 16);
#else
	auto offs = data_long_offs(m);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
  VLDR32_sRi(SCRATCH_F32_1, REG_WORK1, 0);
  VCVT32to64_ds(r, SCRATCH_F32_1);
}
LENDFUNC(NONE,READ,2,raw_fmovs_rm,(FW r, MEMR m))

LOWFUNC(NONE,NONE,3,raw_fmov_to_d_rrr,(W4 d1, W4 d2, FR s))
{
  VMOV64_rrd(d1, d2, s);
}
LENDFUNC(NONE,NONE,3,raw_fmov_to_d_rrr,(W4 d1, W4 d2, FR s))

LOWFUNC(NONE,NONE,2,raw_fsqrt_rr,(FW d, FR s))
{
	VSQRT64_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fsqrt_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fabs_rr,(FW d, FR s))
{
	VABS64_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fabs_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fneg_rr,(FW d, FR s))
{
	VNEG64_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fneg_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fdiv_rr,(FRW d, FR s))
{
	VDIV64_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fdiv_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fadd_rr,(FRW d, FR s))
{
	VADD64_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fadd_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmul_rr,(FRW d, FR s))
{
	VMUL64_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmul_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fsub_rr,(FRW d, FR s))
{
	VSUB64_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fsub_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_frndint_rr,(FW d, FR s))
{
	VCVTR64toI_sd(SCRATCH_F32_1, s);
	VCVTIto64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_frndint_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_frndintz_rr,(FW d, FR s))
{
	VCVT64toI_sd(SCRATCH_F32_1, s);
	VCVTIto64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_frndintz_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmod_rr,(FRW d, FR s))
{
	VDIV64_ddd(SCRATCH_F64_2, d, s);
	VCVT64toI_sd(SCRATCH_F32_1, SCRATCH_F64_2);
	VCVTIto64_ds(SCRATCH_F64_2, SCRATCH_F32_1);
	VMUL64_ddd(SCRATCH_F64_1, SCRATCH_F64_2, s);
	VSUB64_ddd(d, d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fmod_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fsgldiv_rr,(FRW d, FR s))
{
	VCVT64to32_sd(SCRATCH_F32_1, d);
	VCVT64to32_sd(SCRATCH_F32_2, s);
	VDIV32_sss(SCRATCH_F32_1, SCRATCH_F32_1, SCRATCH_F32_2);
	VCVT32to64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fsgldiv_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,1,raw_fcuts_r,(FRW r))
{
	VCVT64to32_sd(SCRATCH_F32_1, r);
	VCVT32to64_ds(r, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,1,raw_fcuts_r,(FRW r))

LOWFUNC(NONE,NONE,2,raw_frem1_rr,(FRW d, FR s))
{
	VMRS_r(REG_WORK1);
	BIC_rri(REG_WORK2, REG_WORK1, 0x00c00000);
	VMSR_r(REG_WORK2);
	
	VDIV64_ddd(SCRATCH_F64_2, d, s);
	VCVTR64toI_sd(SCRATCH_F32_1, SCRATCH_F64_2);
	VCVTIto64_ds(SCRATCH_F64_2, SCRATCH_F32_1);
	VMUL64_ddd(SCRATCH_F64_1, SCRATCH_F64_2, s);
	VSUB64_ddd(d, d, SCRATCH_F64_1);
	
	VMRS_r(REG_WORK2);
	UBFX_rrii(REG_WORK1, REG_WORK1, 22, 2);
	BFI_rrii(REG_WORK2, REG_WORK1, 22, 23);
	VMSR_r(REG_WORK2);
}
LENDFUNC(NONE,NONE,2,raw_frem1_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fsglmul_rr,(FRW d, FR s))
{
	VCVT64to32_sd(SCRATCH_F32_1, d);
	VCVT64to32_sd(SCRATCH_F32_2, s);
	VMUL32_sss(SCRATCH_F32_1, SCRATCH_F32_1, SCRATCH_F32_2);
	VCVT32to64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fsglmul_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmovs_rr,(FW d, FR s))
{
	VCVT64to32_sd(SCRATCH_F32_1, s);
	VCVT32to64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmovs_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,3,raw_ffunc_rr,(double (*func)(double), FW d, FR s))
{
	VMOV64_dd(0, s);
#ifdef ARMV6T2
  MOVW_ri16(REG_WORK1, (uae_u32)func);
  MOVT_ri16(REG_WORK1, ((uae_u32)func) >> 16);
#else
	auto offs = data_long_offs(uae_u32(func));
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	PUSH(RLR_INDEX);
	BLX_r(REG_WORK1);
	POP(RLR_INDEX);

	VMOV64_dd(d, 0);
}
LENDFUNC(NONE,NONE,3,raw_ffunc_rr,(double (*func)(double), FW d, FR s))

LOWFUNC(NONE,NONE,3,raw_fpowx_rr,(uae_u32 x, FW d, FR s))
{
	double (*func)(double,double) = pow;

	if(x == 2) {
		VMOV_F64_dimmF(0, 0x00); // load imm #2 into first reg
	} else {
		VMOV_F64_dimmF(0, 0x24); // load imm #10 into first reg
	}

	VMOV64_dd(1, s);
		
#ifdef ARMV6T2
	MOVW_ri16(REG_WORK1, (uae_u32)func);
	MOVT_ri16(REG_WORK1, ((uae_u32)func) >> 16);
#else
	auto offs = data_long_offs(uae_u32(func));
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif

	PUSH(RLR_INDEX);
	BLX_r(REG_WORK1);
	POP(RLR_INDEX);

	VMOV64_dd(d, 0);
}
LENDFUNC(NONE,NONE,3,raw_fpowx_rr,(uae_u32 x, FW d, FR s))

LOWFUNC(NONE,WRITE,2,raw_fp_from_exten_mr,(RR4 adr, FR s))
{
  VMOVi_to_ARM_rd(REG_WORK1, s, 1);    				// get high part of double
  VCMP64_d0(s);
  VMRS_CPSR();
#ifdef ARMV6T2
  BEQ_i(19);          // iszero
#else
  BEQ_i(20);
#endif

  UBFX_rrii(REG_WORK2, REG_WORK1, 20, 11);  	// get exponent
	MOVW_ri16(REG_WORK3, 2047);
	CMP_rr(REG_WORK2, REG_WORK3);
#ifdef ARMV6T2
	BEQ_i(12); 				// isnan
#else
	BEQ_i(13);
#endif

  MOVW_ri16(REG_WORK3, 15360);              	// diff of bias between double and long double
  ADD_rrr(REG_WORK2, REG_WORK2, REG_WORK3); 	// exponent done
  AND_rri(REG_WORK1, REG_WORK1, 0x80000000);        // extract sign
  ORR_rrrLSLi(REG_WORK2, REG_WORK1, REG_WORK2, 16); // merge sign and exponent

	ADD_rrr(REG_WORK3, adr, R_MEMSTART);

  REV_rr(REG_WORK2, REG_WORK2);
  STRH_rR(REG_WORK2, REG_WORK3);             	// write exponent

  VSHL64_ddi(SCRATCH_F64_1, s, 11);           // shift mantissa to correct position
  VREV64_8_dd(SCRATCH_F64_1, SCRATCH_F64_1);
  VMOV64_rrd(REG_WORK1, REG_WORK2, SCRATCH_F64_1);
  ORR_rri(REG_WORK1, REG_WORK1, 0x80);  			// insert explicit 1
#ifdef ARMV6T2
  STRD_rRI(REG_WORK1, REG_WORK3, 4);
  B_i(8);            // end_of_op
#else
  STR_rRI(REG_WORK1, REG_WORK3, 4);
  STR_rRI(REG_WORK2, REG_WORK3, 8);
  B_i(9);
#endif

// isnan
  MOVW_ri16(REG_WORK1, 0x7fff);
  LSL_rri(REG_WORK1, REG_WORK1, 16);
  MVN_ri(REG_WORK2, 0);

// iszero
  CC_AND_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, 0x80000000); // extract sign
  CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);

	ADD_rrr(REG_WORK3, adr, R_MEMSTART);

  REV_rr(REG_WORK1, REG_WORK1);
#ifdef ARMV6T2
  STRD_rR(REG_WORK1, REG_WORK3);
#else
  STR_rR(REG_WORK1, REG_WORK3);
  STR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
  STR_rRI(REG_WORK2, REG_WORK3, 8);

// end_of_op
 
}
LENDFUNC(NONE,WRITE,2,raw_fp_from_exten_mr,(RR4 adr, FR s))

LOWFUNC(NONE,READ,2,raw_fp_to_exten_rm,(FW d, RR4 adr))
{
	ADD_rrr(REG_WORK3, adr, R_MEMSTART);

#ifdef ARMV6T2
	LDRD_rRI(REG_WORK1, REG_WORK3, 4);
#else
	LDR_rRI(REG_WORK1, REG_WORK3, 4);
	LDR_rRI(REG_WORK2, REG_WORK3, 8);
#endif
	BIC_rri(REG_WORK1, REG_WORK1, 0x80); 	// clear explicit 1
	VMOV64_drr(d, REG_WORK1, REG_WORK2);
  VREV64_8_dd(d, d);

  LDRH_rR(REG_WORK1, REG_WORK3);
  REV16_rr(REG_WORK1, REG_WORK1);				// exponent now in lower half

	MOVW_ri16(REG_WORK2, 0x7fff);
	ANDS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
	BNE_i(9);				// not_zero
	VCMP64_d0(d);
	VMRS_CPSR();
	BNE_i(6);				// not zero
// zero
	VMOV_I64_dimmI(d, 0x00);
	TST_ri(REG_WORK1, 0x8000);								// check sign
	BEQ_i(12);			// end_of_op
	MOV_ri(REG_WORK1, 0x80000000);
	MOV_ri(REG_WORK2, 0);
	VMOV64_drr(d, REG_WORK2, REG_WORK1);
	B_i(8);					// end_of_op

// not_zero
	MOVW_ri16(REG_WORK3, 15360);              // diff of bias between double and long double
	SUB_rrr(REG_WORK2, REG_WORK2, REG_WORK3);	// exponent done, ToDo: check for carry -> result gets Inf in double
	UBFX_rrii(REG_WORK1, REG_WORK1, 15, 1);		// extract sign
	BFI_rrii(REG_WORK2, REG_WORK1, 11, 11);		// insert sign
	VSHR64_ddi(d, d, 11);											// shift mantissa to correct position
	LSL_rri(REG_WORK2, REG_WORK2, 20);
	VMOV_I64_dimmI(SCRATCH_F64_1, 0x00);
	VMOVi_from_ARM_dr(SCRATCH_F64_1, REG_WORK2, 1);
	VORR_ddd(d, d, SCRATCH_F64_1);
// end_of_op

}
LENDFUNC(NONE,READ,2,raw_fp_to_exten_rm,(FW d, RR4 adr))

LOWFUNC(NONE,WRITE,2,raw_fp_from_double_mr,(RR4 adr, FR s))
{
	ADD_rrr(REG_WORK3, adr, R_MEMSTART);
	
  VREV64_8_dd(SCRATCH_F64_1, s);
  VSTR64_dRi(SCRATCH_F64_1, REG_WORK3, 0);
}
LENDFUNC(NONE,WRITE,2,raw_fp_from_double_mr,(RR4 adr, FR s))

LOWFUNC(NONE,READ,2,raw_fp_to_double_rm,(FW d, RR4 adr))
{
	ADD_rrr(REG_WORK3, adr, R_MEMSTART);
	
  VLDR64_dRi(d, REG_WORK3, 0);
  VREV64_8_dd(d, d);
}
LENDFUNC(NONE,READ,2,raw_fp_to_double_rm,(FW d, RR4 adr))

STATIC_INLINE void raw_fflags_into_flags(int r)
{
	VCMP64_d0(r);
	VMRS_CPSR();
}

LOWFUNC(NONE,NONE,2,raw_fp_fscc_ri,(RW4 d, int cc))
{
	switch (cc) {
		case NATIVE_CC_F_NEVER:
			BIC_rri(d, d, 0xff);
			break;
			
		case NATIVE_CC_NE: // Set if not equal
			CC_BIC_rri(NATIVE_CC_EQ, d, d, 0xff); // do not set if equal
			CC_ORR_rri(NATIVE_CC_NE, d, d, 0xff);
			break;

		case NATIVE_CC_EQ: // Set if equal
			CC_BIC_rri(NATIVE_CC_NE, d, d, 0xff); // do not set if not equal
			CC_ORR_rri(NATIVE_CC_EQ, d, d, 0xff);
			break;

		case NATIVE_CC_F_OGT: // Set if valid and greater than
  		BVS_i(2);		// do not set if NaN
			BLE_i(1);		// do not set if less or equal
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;

		case NATIVE_CC_F_OGE: // Set if valid and greater or equal
			BVS_i(2);		// do not set if NaN
			BCC_i(1);		// do not set if carry cleared
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;
			
		case NATIVE_CC_F_OLT: // Set if vaild and less than
			BVS_i(2);		// do not set if NaN
			BCS_i(1);		// do not set if carry set
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;
			
		case NATIVE_CC_F_OLE: // Set if valid and less or equal
			BVS_i(2);		// do not set if NaN
			BGT_i(1);		// do not set if greater than
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;
			
		case NATIVE_CC_F_OGL: // Set if valid and greator or less
			BVS_i(2);		// do not set if NaN
			BEQ_i(1);		// do not set if equal
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;

		case NATIVE_CC_F_OR: // Set if valid
			CC_BIC_rri(NATIVE_CC_VS, d, d, 0xff); // do not set if NaN
			CC_ORR_rri(NATIVE_CC_VC, d, d, 0xff);
			break;
			
		case NATIVE_CC_F_UN: // Set if NAN
			CC_BIC_rri(NATIVE_CC_VC, d, d, 0xff);	// do not set if valid
			CC_ORR_rri(NATIVE_CC_VS, d, d, 0xff);
			break;

		case NATIVE_CC_F_UEQ: // Set if NAN or equal
			BVS_i(0); 	// set if NaN
			BNE_i(1);		// do not set if greater or less
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;

		case NATIVE_CC_F_UGT: // Set if NAN or greater than
			BVS_i(0); 	// set if NaN
			BLS_i(1);		// do not set if lower or same
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;

		case NATIVE_CC_F_UGE: // Set if NAN or greater or equal
			BVS_i(0); 	// set if NaN
			BMI_i(1);		// do not set if lower
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;

		case NATIVE_CC_F_ULT: // Set if NAN or less than
			BVS_i(0); 	// set if NaN
			BGE_i(1);		// do not set if greater or equal
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;

		case NATIVE_CC_F_ULE: // Set if NAN or less or equal
			BVS_i(0); 	// set if NaN
			BGT_i(1);		// do not set if greater
			ORR_rri(d, d, 0xff);
			B_i(0);
			BIC_rri(d, d, 0xff);
			break;
	}
}
LENDFUNC(NONE,NONE,2,raw_fp_fscc_ri,(RW4 d, int cc))

LOWFUNC(NONE,NONE,1,raw_roundingmode,(IMM mode))
{
  VMRS_r(REG_WORK1);
  BIC_rri(REG_WORK1, REG_WORK1, 0x00c00000);
  ORR_rri(REG_WORK1, REG_WORK1, mode);
  VMSR_r(REG_WORK1);
}
LENDFUNC(NONE,NONE,1,raw_roundingmode,(IMM mode))

