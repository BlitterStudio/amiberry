/*
 * compiler/codegen_arm.cpp - AARCH64 code generator
 *
 * Copyright (c) 2019 TomB
 * 
 * This file is part of the UAE4ARM project.
 *
 * JIT compiler m68k -> ARMv8.0
 *
 * Original 68040 JIT compiler for UAE, copyright 2000-2002 Bernd Meyer
 * This file is derived from CCG, copyright 1999-2003 Ian Piumarta
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
 * Note: JIT for aarch64 only works if memory for Amiga part (regs.natmen_offset
 *       to additional_mem + ADDITIONAL_MEMSIZE + BARRIER) is allocated in lower
 *       32 bit address space. See alloc_AmigaMem() in generic_mem.cpp.
 */

#include "flags_arm.h"
#include <math.h>

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
#define R16_INDEX 16
#define R17_INDEX 17
#define R18_INDEX 18
#define R27_INDEX 27
#define R28_INDEX 28

#define RSP_INDEX 31
#define RLR_INDEX 30
#define RFP_INDEX 29

/* The register in which subroutines return an integer return value */
#define REG_RESULT R0_INDEX

/* The registers subroutines take their first and second argument in */
#define REG_PAR1 R0_INDEX
#define REG_PAR2 R1_INDEX

#define REG_WORK1 R2_INDEX
#define REG_WORK2 R3_INDEX
#define REG_WORK3 R4_INDEX
#define REG_WORK4 R5_INDEX

#define REG_PC_TMP R1_INDEX /* Another register that is not the above */

#define R_MEMSTART 27
#define R_REGSTRUCT 28
uae_s8 always_used[] = {2,3,4,5,18,R_MEMSTART,R_REGSTRUCT,-1}; // r2-r5 are work register in emitted code, r18 special use reg

uae_u8 call_saved[] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1, 1,1,1,1, 1,1,1,1, 1,0,0,0};

/* This *should* be the same as call_saved. But:
   - We might not really know which registers are saved, and which aren't,
     so we need to preserve some, but don't want to rely on everyone else
     also saving those registers
   - Special registers (such like the stack pointer) should not be "preserved"
     by pushing, even though they are "saved" across function calls
   - r19 - r26 not in use, so no need to preserve
   - if you change need_to_preserve, modify raw_push_regs_to_preserve() and raw_pop_preserved_regs()
*/
static const uae_u8 need_to_preserve[] = {0,0,0,0, 0,0,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0, 0,0,0,1, 1,0,0,0};

#include "codegen_armA64.h"

#define FIX_INVERTED_CARRY              \
  if(flags_carry_inverted) {            \
  	MRS_NZCV_x(REG_WORK1);              \
  	EOR_xxCflag(REG_WORK1, REG_WORK1);  \
  	MSR_NZCV_x(REG_WORK1);              \
    flags_carry_inverted = false;       \
  }


STATIC_INLINE void SIGNED8_IMM_2_REG(W4 r, IM8 v) {
	uae_s16 v16 = (uae_s16)(uae_s8)v;
	if (v16 & 0x8000) {
		MOVN_xi(r, (uae_u16) ~v16);
	} else {
		MOV_xi(r, (uae_u16) v16);
	}
}

STATIC_INLINE void UNSIGNED16_IMM_2_REG(W4 r, IM16 v) {
  MOV_xi(r, v);
}

STATIC_INLINE void SIGNED16_IMM_2_REG(W4 r, IM16 v) {
	if (v & 0x8000) {
		MOVN_xi(r, (uae_u16) ~v);
	} else {
		MOV_xi(r, (uae_u16) v);
	}
}

STATIC_INLINE void UNSIGNED8_REG_2_REG(W4 d, RR4 s) {
	UXTB_xx(d, s);
}

STATIC_INLINE void SIGNED8_REG_2_REG(W4 d, RR4 s) {
	SXTB_xx(d, s);
}

STATIC_INLINE void UNSIGNED16_REG_2_REG(W4 d, RR4 s) {
	UXTH_xx(d, s);
}

STATIC_INLINE void SIGNED16_REG_2_REG(W4 d, RR4 s) {
	SXTH_xx(d, s);
}

STATIC_INLINE void LOAD_U32(int r, uae_u32 val)
{
  if((val & 0xffff0000) == 0xffff0000) {
    MOVN_xi(r, ~val);
  } else {
    MOV_xi(r, val);
    if(val >> 16)
      MOVK_xish(r, val >> 16, 16);
  }
}

STATIC_INLINE void LOAD_U64(int r, uae_u64 val)
{
  MOV_xi(r, val);
  if((val >> 16) & 0xffff)
    MOVK_xish(r, val >> 16, 16);
  if((val >> 32) & 0xffff)
    MOVK_xish(r, val >> 32, 32);
  if(val >> 48)
    MOVK_xish(r, val >> 48, 48);
}


#define NUM_PUSH_CMDS 1
#define NUM_POP_CMDS 1
STATIC_INLINE void raw_push_regs_to_preserve(void) {
	STP_xxXpre(27, 28, RSP_INDEX, -16);
}

STATIC_INLINE void raw_pop_preserved_regs(void) {
  LDP_xxXpost(27, 28, RSP_INDEX, 16);
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
  uintptr idx = (uintptr) &(regs.ccrflags.nzcv) - (uintptr) &regs;
	MRS_NZCV_x(r);
	if(flags_carry_inverted) {
	  EOR_xxCflag(r, r);
	  MSR_NZCV_x(r);
	  flags_carry_inverted = false;
	}
	STR_wXi(r, R_REGSTRUCT, idx);
	raw_flags_evicted(r);
}

STATIC_INLINE void raw_reg_to_flags(int r)
{
	MSR_NZCV_x(r);
}


//
// compuemu_support used raw calls
//
LOWFUNC(WRITE,RMW,2,compemu_raw_inc_opcount,(IM16 op))
{
  uintptr idx = (uintptr) &(regs.raw_cputbl_count) - (uintptr) &regs;
  LDR_xXi(REG_WORK2, R_REGSTRUCT, idx);
  MOV_xi(REG_WORK3, op);
  LDR_wXxLSLi(REG_WORK1, REG_WORK2, REG_WORK3, 1);
  ADD_wwi(REG_WORK1, REG_WORK1, 1);
  STR_wXxLSLi(REG_WORK1, REG_WORK2, REG_WORK3, 1);
}
LENDFUNC(WRITE,RMW,1,compemu_raw_inc_opcount,(IM16 op))

LOWFUNC(WRITE,READ,1,compemu_raw_cmp_pc,(IMPTR s))
{
  /* s is always >= NATMEM_OFFSETX and < NATMEM_OFFSETX + max. Amiga mem */
  clobber_flags();
  
  uintptr idx = (uintptr) &(regs.pc_p) - (uintptr) &regs;
  LDR_xXi(REG_WORK1, R_REGSTRUCT, idx); // regs.pc_p is 64 bit
  
  LOAD_U64(REG_WORK2, s);
	CMP_xx(REG_WORK1, REG_WORK2);
}
LENDFUNC(WRITE,READ,1,compemu_raw_cmp_pc,(IMPTR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_set_pc_m,(MEMR s))
{
  uintptr idx;
  if(s >= (uintptr) &regs && s < ((uintptr) &regs) + sizeof(struct regstruct)) {
    idx = s - (uintptr) &regs;
    if(s == (uintptr) &(regs.pc_p))
      LDR_xXi(REG_WORK1, R_REGSTRUCT, idx);
    else
      LDR_wXi(REG_WORK1, R_REGSTRUCT, idx);
  } else {
    LOAD_U64(REG_WORK1, s);
	  LDR_xXi(REG_WORK1, REG_WORK1, 0);
  }

  idx = (uintptr) &(regs.pc_p) - (uintptr) &regs;
  STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_set_pc_m,(MEMR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_set_pc_i,(IMPTR s))
{
  LOAD_U64(REG_WORK1, s);
  uintptr idx = (uintptr) &(regs.pc_p) - (uintptr) &regs;
  STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_set_pc_i,(IMPTR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IM32 s))
{
  /* d points always to memory in regs struct */
  LOAD_U32(REG_WORK2, s);
  uintptr idx = d - (uintptr) &regs;
  if(d == (uintptr) &(regs.pc_p))
    STR_xXi(REG_WORK2, R_REGSTRUCT, idx);
  else
    STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IM32 s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))
{
  /* d points always to memory in regs struct */
  uintptr idx = d - (uintptr) &regs;
  if(d == (uintptr) &(regs.pc_p))
    STR_xXi(s, R_REGSTRUCT, idx);
  else
    STR_wXi(s, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IM32 s))
{
  LOAD_U32(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IM32 s))

LOWFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))
{
  if(s >= (uintptr) &regs && s < ((uintptr) &regs) + sizeof(struct regstruct)) {
    uintptr idx = s - (uintptr) &regs;
    if(s == (uintptr) &(regs.pc_p))
      LDR_xXi(d, R_REGSTRUCT, idx);
    else
      LDR_wXi(d, R_REGSTRUCT, idx);
  } else {
    LOAD_U64(REG_WORK1, s);
	  LDR_xXi(d, REG_WORK1, 0);
  }
}
LENDFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))
{
	MOV_xx(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))

LOWFUNC(WRITE,RMW,1,compemu_raw_dec_m,(MEMRW d))
{
  clobber_flags();

  LOAD_U64(REG_WORK1, d);
  LDR_wXi(REG_WORK2, REG_WORK1, 0);
  SUBS_wwi(REG_WORK2, REG_WORK2, 1);
  STR_wXi(REG_WORK2, REG_WORK1, 0);
}
LENDFUNC(WRITE,RMW,1,compemu_raw_dec_m,(MEMRW ds))

STATIC_INLINE void compemu_raw_call(uintptr t)
{
  LOAD_U64(REG_WORK1, t);

	STR_xXpre(RLR_INDEX, RSP_INDEX, -16);
	BLR_x(REG_WORK1);
	LDR_xXpost(RLR_INDEX, RSP_INDEX, 16);
}

STATIC_INLINE void compemu_raw_call_r(RR4 r)
{
	STR_xXpre(RLR_INDEX, RSP_INDEX, -16);
	BLR_x(r);
	LDR_xXpost(RLR_INDEX, RSP_INDEX, 16);
}

STATIC_INLINE void compemu_raw_jcc_l_oponly(int cc)
{
  FIX_INVERTED_CARRY
  
	switch (cc) {
		case NATIVE_CC_HI: // HI
			BEQ_i(2);										// beq no jump
 			BCC_i(0);										// bcc jump
			break;

		case NATIVE_CC_LS: // LS
			BEQ_i(2);										// beq jump
 			BCC_i(2);										// bcc no jump
			// jump
			B_i(0); 
			// no jump
			break;

		case NATIVE_CC_F_OGT: // Jump if valid and greater than
			BVS_i(2);		// do not jump if NaN
			BGT_i(0);		// jump if greater than
			break;

		case NATIVE_CC_F_OGE: // Jump if valid and greater or equal
  		BVS_i(2);		// do not jump if NaN
			BCS_i(0);		// jump if carry set
			break;
			
		case NATIVE_CC_F_OLT: // Jump if vaild and less than
			BVS_i(2);		// do not jump if NaN
			BCC_i(0);		// jump if carry cleared
			break;
			
		case NATIVE_CC_F_OLE: // Jump if valid and less or equal
			BVS_i(2);		// do not jump if NaN
			BLE_i(0);		// jump if less or equal
			break;
			
		case NATIVE_CC_F_OGL: // Jump if valid and greator or less
			BVS_i(2);		// do not jump if NaN
			BNE_i(0);		// jump if not equal
			break;

		case NATIVE_CC_F_OR: // Jump if valid
			BVC_i(0);
			break;
			
		case NATIVE_CC_F_UN: // Jump if NAN
			BVS_i(0); 
			break;

		case NATIVE_CC_F_UEQ: // Jump if NAN or equal
			BVS_i(2); 	// jump if NaN
			BNE_i(2);		// do not jump if greater or less
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_UGT: // Jump if NAN or greater than
			BVS_i(2); 	// jump if NaN
			BLS_i(2);		// do not jump if lower or same
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_UGE: // Jump if NAN or greater or equal
			BVS_i(2); 	// jump if NaN
			BMI_i(2);		// do not jump if lower
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_ULT: // Jump if NAN or less than
			BVS_i(2); 	// jump if NaN
			BGE_i(2);		// do not jump if greater or equal
			// jump
			B_i(0); 
			break;

		case NATIVE_CC_F_ULE: // Jump if NAN or less or equal
			BVS_i(2); 	// jump if NaN
			BGT_i(2);		// do not jump if greater
			// jump
			B_i(0); 
			break;
	
		default:
	    CC_B_i(cc, 0);
			break;
	}
  // emit of target into last branch will be done by caller
}

STATIC_INLINE void compemu_raw_handle_except(IM32 cycles)
{
	uae_u32* branchadd;	

  clobber_flags();

  uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
  LDR_wXi(REG_WORK1, R_REGSTRUCT, idx);
	branchadd = (uae_u32*)get_target();
  CBZ_wi(REG_WORK1, 0);  // no exception, jump to next instruction
	
  raw_pop_preserved_regs();
  LOAD_U32(REG_PAR1, cycles);
  LDR_xPCi(REG_WORK1, 8); // <execute_exception>
  BR_x(REG_WORK1);
	emit_longlong((uintptr)execute_exception);
	
	// Write target of next instruction
	write_jmp_target(branchadd, (uintptr)get_target());
}

STATIC_INLINE void compemu_raw_maybe_recompile(uintptr t)
{
  BGE_i(NUM_POP_CMDS + 5);
  raw_pop_preserved_regs();
  LDR_xPCi(REG_WORK1, 8); 
  BR_x(REG_WORK1);
  emit_longlong(t);
}

STATIC_INLINE void compemu_raw_jmp(uintptr t)
{
  LDR_xPCi(REG_WORK1, 8);
  BR_x(REG_WORK1);
  emit_longlong(t);
}

STATIC_INLINE void compemu_raw_jmp_pc_tag(uintptr base)
{
  uintptr idx = (uintptr)&regs.pc_p - (uintptr)&regs;
  LDRH_wXi(REG_WORK1, R_REGSTRUCT, idx);
  LDR_xPCi(REG_WORK2, 12);
	LDR_xXxLSLi(REG_WORK1, REG_WORK2, REG_WORK1, 1);
	BR_x(REG_WORK1);
	emit_longlong(base);
}

STATIC_INLINE void compemu_raw_maybe_cachemiss(uintptr t)
{
  BEQ_i(NUM_POP_CMDS + 5);
  raw_pop_preserved_regs();
  LDR_xPCi(REG_WORK1, 8);
  BR_x(REG_WORK1);
  emit_longlong(t);
}

STATIC_INLINE void compemu_raw_maybe_do_nothing(IM32 cycles, uintptr adr)
{
  uintptr idx = (uintptr)&regs.spcflags - (uintptr) &regs;
  LDR_wXi(REG_WORK1, R_REGSTRUCT, idx);
  uae_s8 *branchadd = (uae_s8 *)get_target();
  CBZ_wi(REG_WORK1, 0);  // <end>

  idx = (uintptr)&countdown - (uintptr) &regs;
  LDR_wXi(REG_WORK2, R_REGSTRUCT, idx);
  if(cycles >= 0 && cycles <= 0xfff) {
    SUB_wwi(REG_WORK2, REG_WORK2, cycles);
  } else {
    LOAD_U32(REG_WORK1, cycles);
	  SUB_www(REG_WORK2, REG_WORK2, REG_WORK1);
  }
  STR_wXi(REG_WORK2, R_REGSTRUCT, idx);

  raw_pop_preserved_regs();
  LDR_xPCi(REG_WORK1, 8);
  BR_x(REG_WORK1);
  emit_longlong(adr);

  // <end>
  write_jmp_target((uae_u32 *)branchadd, (uintptr)get_target());
}

// Optimize access to struct regstruct with and memory with fixed registers

LOWFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMPTR s))
{
  LOAD_U64(R_REGSTRUCT, s);
	uintptr offsmem = (uintptr)&NATMEM_OFFSETX - (uintptr) &regs;
  LDR_xXi(R_MEMSTART, R_REGSTRUCT, offsmem);
}
LENDFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMPTR s))

// Handle end of compiled block
LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IM32 cycles))
{
  // countdown -= scaled_cycles(totcycles);
  uintptr offs = (uintptr)&countdown - (uintptr)&regs;
	LDR_wXi(REG_WORK1, R_REGSTRUCT, offs);
  if(cycles >= 0 && cycles <= 0xfff) {
	  SUB_wwi(REG_WORK1, REG_WORK1, cycles);
	} else {
	  LOAD_U32(REG_WORK2, cycles);
  	SUB_www(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_wXi(REG_WORK1, R_REGSTRUCT, offs);

	TBNZ_xii(REG_WORK1, 31, 7); // test sign and branch if set (negative)
  UBFIZ_xxii(rr_pc, rr_pc, 0, 16);  // apply TAGMASK
  LDR_xPCi(REG_WORK1, 12); // <cache_tags>
	LDR_xXxLSLi(REG_WORK1, REG_WORK1, rr_pc, 3); // cacheline holds pointer -> multiply with 8
  BR_x(REG_WORK1);
	emit_longlong((uintptr)cache_tags);

  raw_pop_preserved_regs();
  LDR_xPCi(REG_WORK1, 8); // <do_nothing>
  BR_x(REG_WORK1);

	emit_longlong((uintptr)do_nothing);
}
LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IM32 cycles))

STATIC_INLINE uae_u32* compemu_raw_endblock_pc_isconst(IM32 cycles, IMPTR v)
{
  /* v is always >= NATMEM_OFFSETX and < NATMEM_OFFSETX + max. Amiga mem */
	uae_u32* tba;
  // countdown -= scaled_cycles(totcycles);
  uintptr offs = (uintptr)&countdown - (uintptr)&regs;
	LDR_wXi(REG_WORK1, R_REGSTRUCT, offs);
  if(cycles >= 0 && cycles <= 0xfff) {
	  SUB_wwi(REG_WORK1, REG_WORK1, cycles);
	} else {
	  LOAD_U32(REG_WORK2, cycles);
  	SUB_www(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_wXi(REG_WORK1, R_REGSTRUCT, offs);

  TBNZ_xii(REG_WORK1, 31, 2); // test sign and branch if set (negative)
	tba = (uae_u32*)get_target();
  B_i(0); // <target set by caller>
  
  LDR_xPCi(REG_WORK1, 16 + 4 * NUM_POP_CMDS); // <v>
  offs = (uintptr)&regs.pc_p - (uintptr)&regs;
  STR_xXi(REG_WORK1, R_REGSTRUCT, offs);
  raw_pop_preserved_regs();
  LDR_xPCi(REG_WORK1, 16); // <do_nothing>
  BR_x(REG_WORK1);

	emit_longlong(v);
	emit_longlong((uintptr)do_nothing);

	return tba;  
}

/*************************************************************************
* FPU stuff                                                             *
*************************************************************************/

#ifdef USE_JIT_FPU

LOWFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))
{
	FMOV_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_fmov_mr_drop,(MEMW mem, FR s))
{
  if(mem >= (uintptr) &regs && mem < (uintptr) &regs + 32760 && ((mem - (uintptr) &regs) & 0x7) == 0) {
    STR_dXi(s, R_REGSTRUCT, (mem - (uintptr) &regs));
  } else {
    LOAD_U64(REG_WORK1, mem);
    STR_dXi(s, REG_WORK1, 0);
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_fmov_mr_drop,(MEMW mem, FR s))

LOWFUNC(NONE,READ,2,compemu_raw_fmov_rm,(FW d, MEMR mem))
{
  if(mem >= (uintptr) &regs && mem < (uintptr) &regs + 32760 && ((mem - (uintptr) &regs) & 0x7) == 0) {
    LDR_dXi(d, R_REGSTRUCT, (mem - (uintptr) &regs));
  } else {
    LOAD_U64(REG_WORK1, mem);
    LDR_dXi(d, REG_WORK1, 0);
  }
}
LENDFUNC(NONE,READ,2,compemu_raw_fmov_rm,(FW d, MEMW mem))

LOWFUNC(NONE,NONE,2,raw_fmov_l_rr,(FW d, RR4 s))
{
  SCVTF_dw(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmov_l_rr,(FW d, RR4 s))

LOWFUNC(NONE,NONE,2,raw_fmov_s_rr,(FW d, RR4 s))
{
  FMOV_sw(SCRATCH_F64_1, s);
  FCVT_ds(d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_s_rr,(FW d, RR4 s))

LOWFUNC(NONE,NONE,2,raw_fmov_w_rr,(FW d, RR2 s))
{
  SIGNED16_REG_2_REG(REG_WORK1, s);
  SCVTF_dw(d, REG_WORK1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_w_rr,(FW d, RR2 s))

LOWFUNC(NONE,NONE,2,raw_fmov_b_rr,(FW d, RR1 s))
{
  SIGNED8_REG_2_REG(REG_WORK1, s);
  SCVTF_dw(d, REG_WORK1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_b_rr,(FW d, RR1 s))

LOWFUNC(NONE,NONE,2,raw_fmov_d_rrr,(FW d, RR4 s1, RR4 s2))
{
  BFI_xxii(s1, s2, 32, 32);
  FMOV_dx(d, s1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_d_rrr,(FW d, RR4 s1, RR4 s2))

LOWFUNC(NONE,NONE,2,raw_fmov_to_l_rr,(W4 d, FR s))
{
  FRINTI_dd(SCRATCH_F64_1, s);
  FCVTAS_wd(d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_to_l_rr,(W4 d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmov_to_s_rr,(W4 d, FR s))
{
  FCVT_sd(SCRATCH_F64_1, s);
  FMOV_ws(d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_to_s_rr,(W4 d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmov_to_w_rr,(W4 d, FR s, int targetIsReg))
{
  FRINTI_dd(SCRATCH_F64_1, s);
  FCVTAS_wd(REG_WORK1, SCRATCH_F64_1);

  // maybe saturate...
  TBZ_xii(REG_WORK1, 31, 6); // positive
  CLS_ww(REG_WORK2, REG_WORK1); // negative: if 17 bits are 1 -> no saturate
  SUB_wwi(REG_WORK2, REG_WORK2, 16);
  TBZ_xii(REG_WORK2, 31, 7); // done
  MOVK_wi(d, 0x8000); // max. negative value in 16 bit
  B_i(6);

  // positive
  CLZ_ww(REG_WORK2, REG_WORK1); // positive: if 17 bits are 0 -> no saturate
  SUB_wwi(REG_WORK2, REG_WORK2, 17);
  TBZ_xii(REG_WORK2, 31, 2);
  MOV_wi(REG_WORK1, 0x7fff); // max. positive value in 16 bit
  
  // done
  BFI_wwii(d, REG_WORK1, 0, 16);
}
LENDFUNC(NONE,NONE,2,raw_fmov_to_w_rr,(W4 d, FR s, int targetIsReg))

LOWFUNC(NONE,NONE,3,raw_fmov_to_b_rr,(W4 d, FR s, int targetIsReg))
{
  FRINTI_dd(SCRATCH_F64_1, s);
  FCVTAS_wd(REG_WORK1, SCRATCH_F64_1);

  // maybe saturate...
  TBZ_xii(REG_WORK1, 31, 6); // positive
  CLS_ww(REG_WORK2, REG_WORK1); // negative: if 25 bits are 1 -> no saturate
  SUB_wwi(REG_WORK2, REG_WORK2, 24);
  TBZ_xii(REG_WORK2, 31, 7); // done
  MOV_wi(REG_WORK1, 0x80); // max. negative value in 8 bit
  B_i(5);

  // positive
  CLZ_ww(REG_WORK2, REG_WORK1); // positive: if 25 bits are 0 -> no saturate
  SUB_wwi(REG_WORK2, REG_WORK2, 25);
  TBZ_xii(REG_WORK2, 31, 2);
  MOV_wi(REG_WORK1, 0x7f); // max. positive value in 8 bit

  // done
  BFI_wwii(d, REG_WORK1, 0, 8);
}
LENDFUNC(NONE,NONE,3,raw_fmov_to_b_rr,(W4 d, FR s, int targetIsReg))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_0,(FW r))
{
	MOVI_di(r, 0);
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_0,(FW r))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_1,(FW r))
{
  FMOV_di(r, 0b01110000);
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_1,(FW r))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_10,(FW r))
{
  FMOV_di(r, 0b00100100);
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_10,(FW r))

LOWFUNC(NONE,NONE,1,raw_fmov_d_ri_100,(FW r))
{
  MOV_wi(REG_WORK1, 100);
  SCVTF_dw(r, REG_WORK1);
}
LENDFUNC(NONE,NONE,1,raw_fmov_d_ri_100,(FW r))

LOWFUNC(NONE,READ,2,raw_fmov_d_rm,(FW r, MEMR m))
{
  LOAD_U64(REG_WORK1, m);
  LDR_dXi(r, REG_WORK1, 0);
}
LENDFUNC(NONE,READ,2,raw_fmov_d_rm,(FW r, MEMR m))

LOWFUNC(NONE,READ,2,raw_fmovs_rm,(FW r, MEMR m))
{
  LOAD_U64(REG_WORK1, m);
  LDR_sXi(r, REG_WORK1, 0);
  FCVT_ds(r, r);
}
LENDFUNC(NONE,READ,2,raw_fmovs_rm,(FW r, MEMR m))

LOWFUNC(NONE,NONE,3,raw_fmov_to_d_rrr,(W4 d1, W4 d2, FR s))
{
  FMOV_xd(d1, s);
  LSR_xxi(d2, d1, 32);
}
LENDFUNC(NONE,NONE,3,raw_fmov_to_d_rrr,(W4 d1, W4 d2, FR s))

LOWFUNC(NONE,NONE,2,raw_fsqrt_rr,(FW d, FR s))
{
	FSQRT_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fsqrt_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fabs_rr,(FW d, FR s))
{
	FABS_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fabs_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fneg_rr,(FW d, FR s))
{
	FNEG_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fneg_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fdiv_rr,(FRW d, FR s))
{
	FDIV_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fdiv_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fadd_rr,(FRW d, FR s))
{
	FADD_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fadd_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmul_rr,(FRW d, FR s))
{
	FMUL_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmul_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fsub_rr,(FRW d, FR s))
{
	FSUB_ddd(d, d, s);
}
LENDFUNC(NONE,NONE,2,raw_fsub_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_frndint_rr,(FW d, FR s))
{
  FRINTI_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_frndint_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_frndintz_rr,(FW d, FR s))
{
  FRINTZ_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_frndintz_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmod_rr,(FRW d, FR s))
{
	FDIV_ddd(SCRATCH_F64_1, d, s);
  FRINTZ_dd(SCRATCH_F64_1, SCRATCH_F64_1);
  FMSUB_dddd(d, SCRATCH_F64_1, s, d);
}
LENDFUNC(NONE,NONE,2,raw_fmod_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fsgldiv_rr,(FRW d, FR s))
{
	FCVT_sd(SCRATCH_F64_1, d);
	FCVT_sd(SCRATCH_F64_2, s);
	FDIV_sss(SCRATCH_F64_1, SCRATCH_F64_1, SCRATCH_F64_2);
	FCVT_ds(d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fsgldiv_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,1,raw_fcuts_r,(FRW r))
{
	FCVT_sd(SCRATCH_F64_1, r);
	FCVT_ds(r, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,1,raw_fcuts_r,(FRW r))

LOWFUNC(NONE,NONE,2,raw_frem1_rr,(FRW d, FR s))
{
  FDIV_ddd(SCRATCH_F64_2, d, s);
  FRINTA_dd(SCRATCH_F64_2, SCRATCH_F64_2);
  FMSUB_dddd(d, SCRATCH_F64_2, s, d);
}
LENDFUNC(NONE,NONE,2,raw_frem1_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fsglmul_rr,(FRW d, FR s))
{
	FCVT_sd(SCRATCH_F64_1, d);
	FCVT_sd(SCRATCH_F64_2, s);
	FMUL_sss(SCRATCH_F64_1, SCRATCH_F64_1, SCRATCH_F64_2);
	FCVT_ds(d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fsglmul_rr,(FRW d, FR s))

LOWFUNC(NONE,NONE,2,raw_fmovs_rr,(FW d, FR s))
{
	FCVT_sd(SCRATCH_F64_1, s);
	FCVT_ds(d, SCRATCH_F64_1);
}
LENDFUNC(NONE,NONE,2,raw_fmovs_rr,(FW d, FR s))

LOWFUNC(NONE,NONE,3,raw_ffunc_rr,(double (*func)(double), FW d, FR s))
{
	FMOV_dd(0, s);

  LOAD_U64(REG_WORK1, (uintptr)func);

	STR_xXpre(RLR_INDEX, RSP_INDEX, -16);
	BLR_x(REG_WORK1);
	LDR_xXpost(RLR_INDEX, RSP_INDEX, 16);

	FMOV_dd(d, 0);
}
LENDFUNC(NONE,NONE,3,raw_ffunc_rr,(double (*func)(double), FW d, FR s))

LOWFUNC(NONE,NONE,3,raw_fpowx_rr,(uae_u32 x, FW d, FR s))
{
	double (*func)(double,double) = pow;

	if(x == 2) {
		FMOV_di(0, 0b00000000); // load imm #2 into first reg
	} else {
		FMOV_di(0, 0b00100100); // load imm #10 into first reg
	}

	FMOV_dd(1, s);
		
  LOAD_U64(REG_WORK1, (uintptr)func);

	STR_xXpre(RLR_INDEX, RSP_INDEX, -16);
	BLR_x(REG_WORK1);
	LDR_xXpost(RLR_INDEX, RSP_INDEX, 16);

	FMOV_dd(d, 0);
}
LENDFUNC(NONE,NONE,3,raw_fpowx_rr,(uae_u32 x, FW d, FR s))

LOWFUNC(NONE,WRITE,2,raw_fp_from_exten_mr,(RR4 adr, FR s))
{
  FMOV_xd(REG_WORK1, s);
  FCMP_d0(s);
	ADD_xxx(REG_WORK4, adr, R_MEMSTART);

  uae_u32* branchadd_iszero = (uae_u32*)get_target();
  BEQ_i(0); // iszero

  UBFX_xxii(REG_WORK2, REG_WORK1, 52, 11); // get exponent 
	CMP_xi(REG_WORK2, 2047);
  
  uae_u32* branchadd_isnan = (uae_u32*)get_target();
	BEQ_i(0); 				// isnan

  MOV_xi(REG_WORK3, 15360);              	    // diff of bias between double and long double
  ADD_xxx(REG_WORK2, REG_WORK2, REG_WORK3); 	// exponent done
  UBFX_xxii(REG_WORK3, REG_WORK1, 63, 1);     // extract sign
  LSL_xxi(REG_WORK3, REG_WORK3, 31);
  ORR_xxxLSLi(REG_WORK2, REG_WORK3, REG_WORK2, 16); // merge sign and exponent

  REV32_xx(REG_WORK2, REG_WORK2);
  STRH_wXi(REG_WORK2, REG_WORK4, 0);         	// write exponent
  ADD_xxi(REG_WORK4, REG_WORK4, 4);

  LSL_xxi(REG_WORK1, REG_WORK1, 11);          // shift mantissa to correct position
  REV_xx(REG_WORK1, REG_WORK1);
  SET_xxbit(REG_WORK1, REG_WORK1, 7);        // insert explicit 1
  STR_xXi(REG_WORK1, REG_WORK4, 0);
  uae_u32* branchadd_end = (uae_u32*)get_target();
  B_i(0);            // end_of_op

  // isnan
  write_jmp_target(branchadd_isnan, (uintptr)get_target());
  MOV_xish(REG_WORK1, 0x7fff, 16);
  MOVN_xi(REG_WORK2, 0);
  B_i(4);
  
  // iszero
  write_jmp_target(branchadd_iszero, (uintptr)get_target());
  UBFX_xxii(REG_WORK1, REG_WORK1, 63, 1);     // extract sign
  LSL_xxi(REG_WORK1, REG_WORK1, 31);
  MOV_xi(REG_WORK2, 0);

  REV32_xx(REG_WORK1, REG_WORK1);
  STR_wXi(REG_WORK1, REG_WORK4, 0);
  STP_wwXi(REG_WORK2, REG_WORK2, REG_WORK4, 4);

  // end_of_op
  write_jmp_target(branchadd_end, (uintptr)get_target());
}
LENDFUNC(NONE,WRITE,2,raw_fp_from_exten_mr,(RR4 adr, FR s))

LOWFUNC(NONE,READ,2,raw_fp_to_exten_rm,(FW d, RR4 adr))
{
	ADD_xxx(REG_WORK3, adr, R_MEMSTART);

  ADD_xxi(REG_WORK1, REG_WORK3, 4);
	LDR_xXi(REG_WORK1, REG_WORK1, 0);
	CLEAR_xxbit(REG_WORK1, REG_WORK1, 7); 	// clear explicit 1
	REV_xx(REG_WORK1, REG_WORK1);

  LDRH_wXi(REG_WORK4, REG_WORK3, 0);
  REV16_xx(REG_WORK4, REG_WORK4);				// exponent now in lower half

  ANDS_xx7fff(REG_WORK2, REG_WORK4);
	uae_u32* branchadd_notzero = (uae_u32*)get_target();
	BNE_i(0);				// not_zero

	uae_u32* branchadd_notzero2 = (uae_u32*)get_target();
  CBNZ_xi(REG_WORK1, 0);          // not zero

  // zero
	MOVI_di(d, 0);
	uae_u32* branchadd_end = (uae_u32*)get_target();
	TBZ_xii(REG_WORK4, 15, 0); // end_of_op
	MOV_xish(REG_WORK1, 0x8000, 48);
	FMOV_dx(d, REG_WORK1);
	uae_u32* branchadd_end2 = (uae_u32*)get_target();
	B_i(0);					// end_of_op

  // not_zero
  write_jmp_target(branchadd_notzero, (uintptr)get_target());
  write_jmp_target(branchadd_notzero2, (uintptr)get_target());
	MOV_xi(REG_WORK3, 15360);                 // diff of bias between double and long double
	SUB_xxx(REG_WORK2, REG_WORK2, REG_WORK3);	// exponent done, ToDo: check for carry -> result gets Inf in double
	UBFX_xxii(REG_WORK4, REG_WORK4, 15, 1);		// extract sign
	BFI_xxii(REG_WORK2, REG_WORK4, 11, 1);		// insert sign
	LSR_xxi(REG_WORK1, REG_WORK1, 11);				// shift mantissa to correct position
	LSL_xxi(REG_WORK2, REG_WORK2, 52);
	ORR_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	FMOV_dx(d, REG_WORK1);

  // end_of_op
  write_jmp_target(branchadd_end, (uintptr)get_target());
  write_jmp_target(branchadd_end2, (uintptr)get_target());
}
LENDFUNC(NONE,READ,2,raw_fp_to_exten_rm,(FW d, RR4 adr))

LOWFUNC(NONE,WRITE,2,raw_fp_from_double_mr,(RR4 adr, FR s))
{
  REV64_dd(SCRATCH_F64_1, s);
  STR_dXx(SCRATCH_F64_1, adr, R_MEMSTART);
}
LENDFUNC(NONE,WRITE,2,raw_fp_from_double_mr,(RR4 adr, FR s))

LOWFUNC(NONE,READ,2,raw_fp_to_double_rm,(FW d, RR4 adr))
{
	LDR_dXx(d, adr, R_MEMSTART);
  REV64_dd(d, d);
}
LENDFUNC(NONE,READ,2,raw_fp_to_double_rm,(FW d, RR4 adr))

STATIC_INLINE void raw_fflags_into_flags(int r)
{
	FCMP_d0(r);
}

LOWFUNC(NONE,NONE,2,raw_fp_fscc_ri,(RW4 d, int cc))
{
	switch (cc) {
		case NATIVE_CC_F_NEVER:
			CLEAR_LOW8_xx(d, d);
			break;
			
		case NATIVE_CC_NE: // Set if not equal
		  CSETM_wc(REG_WORK1, NATIVE_CC_NE);
		  BFXIL_xxii(d, REG_WORK1, 0, 8);
			break;

		case NATIVE_CC_EQ: // Set if equal
		  CSETM_wc(REG_WORK1, NATIVE_CC_EQ);
		  BFXIL_xxii(d, REG_WORK1, 0, 8);
			break;

		case NATIVE_CC_F_OGT: // Set if valid and greater than
  		BVS_i(4);		// do not set if NaN
			BLE_i(3);		// do not set if less or equal
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;

		case NATIVE_CC_F_OGE: // Set if valid and greater or equal
			BVS_i(4);		// do not set if NaN
			BCC_i(3);		// do not set if carry cleared
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;
			
		case NATIVE_CC_F_OLT: // Set if vaild and less than
			BVS_i(4);		// do not set if NaN
			BCS_i(3);		// do not set if carry set
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;
			
		case NATIVE_CC_F_OLE: // Set if valid and less or equal
			BVS_i(4);		// do not set if NaN
			BGT_i(3);		// do not set if greater than
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;
			
		case NATIVE_CC_F_OGL: // Set if valid and greator or less
			BVS_i(4);		// do not set if NaN
			BEQ_i(3);		// do not set if equal
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;

		case NATIVE_CC_F_OR: // Set if valid
		  CSETM_wc(REG_WORK1, NATIVE_CC_VC);    // do not set if NaN
		  BFXIL_xxii(d, REG_WORK1, 0, 8);
			break;
			
		case NATIVE_CC_F_UN: // Set if NAN
		  CSETM_wc(REG_WORK1, NATIVE_CC_VS);    // do not set if valid
		  BFXIL_xxii(d, REG_WORK1, 0, 8);
			break;

		case NATIVE_CC_F_UEQ: // Set if NAN or equal
			BVS_i(2); 	// set if NaN
			BNE_i(3);		// do not set if greater or less
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;

		case NATIVE_CC_F_UGT: // Set if NAN or greater than
			BVS_i(2); 	// set if NaN
			BLS_i(3);		// do not set if lower or same
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;

		case NATIVE_CC_F_UGE: // Set if NAN or greater or equal
			BVS_i(2); 	// set if NaN
			BMI_i(3);		// do not set if lower
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;

		case NATIVE_CC_F_ULT: // Set if NAN or less than
			BVS_i(2); 	// set if NaN
			BGE_i(3);		// do not set if greater or equal
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;

		case NATIVE_CC_F_ULE: // Set if NAN or less or equal
			BVS_i(2); 	// set if NaN
			BGT_i(3);		// do not set if greater
			SET_LOW8_xx(d, d);
			B_i(2);
			CLEAR_LOW8_xx(d, d);
			break;
	}
}
LENDFUNC(NONE,NONE,2,raw_fp_fscc_ri,(RW4 d, int cc))

LOWFUNC(NONE,NONE,1,raw_roundingmode,(IM32 mode))
{
  MOV_xi(REG_WORK2, (mode >> 22));
  MRS_FPCR_x(REG_WORK1);
  BFI_xxii(REG_WORK1, REG_WORK2, 22, 2);
  MSR_FPCR_x(REG_WORK1);
}
LENDFUNC(NONE,NONE,1,raw_roundingmode,(IM32 mode))

#endif // USE_JIT_FPU

