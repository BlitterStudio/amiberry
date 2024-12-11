/*
 * compiler/codegen_arm.cpp - ARM code generator
 *
 * Copyright (c) 2013 Jens Heitmann of ARAnyM dev team (see AUTHORS)
 * Copyright (c) 2019 TomB
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

#define FIX_INVERTED_CARRY              \
  if(flags_carry_inverted) {            \
	  MRS_CPSR(REG_WORK1);                \
	  EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);  \
	  MSR_CPSRf_r(REG_WORK1);             \
	flags_carry_inverted = false;       \
  }


STATIC_INLINE void SIGNED8_IMM_2_REG(W4 r, IM8 v) {
	if (v & 0x80) {
		MVN_ri8(r, (uae_u8) ~v);
	} else {
		MOV_ri8(r, (uae_u8) v);
	}
}

STATIC_INLINE void UNSIGNED16_IMM_2_REG(W4 r, IM16 v) {
#ifdef ARMV6T2
  MOVW_ri16(r, v);
#else
	MOV_ri8(r, (uae_u8) v);
	ORR_rri8RORi(r, r, (uae_u8)(v >> 8), 24);
#endif
}

STATIC_INLINE void SIGNED16_IMM_2_REG(W4 r, IM16 v) {
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

STATIC_INLINE void LOAD_U16(int r, uae_u32 val)
{
#ifdef ARMV6T2
  MOVW_ri16(r, val);
#else
  uae_s32 offs = data_long_offs(val);
  LDR_rRI(r, RPC_INDEX, offs);
#endif
}

STATIC_INLINE void LOAD_U32(int r, uae_u32 val)
{
#ifdef ARMV6T2
  MOVW_ri16(r, val);
  if(val >> 16)
	MOVT_ri16(r, val >> 16);
#else
  uae_s32 offs = data_long_offs(val);
  LDR_rRI(r, RPC_INDEX, offs);
#endif
}

STATIC_INLINE void raw_push_regs_to_preserve(void) {
	PUSH_REGS(PRESERVE_MASK);
}

STATIC_INLINE void raw_pop_preserved_regs(void) {
	POP_REGS(PRESERVE_MASK);
}

STATIC_INLINE void raw_flags_to_reg(int r)
{
  uintptr idx = (uintptr) &(regflags.nzcv) - (uintptr) &regs;
	MRS_CPSR(r);
	if(flags_carry_inverted) {
	  EOR_rri(r, r, ARM_C_FLAG);
	  MSR_CPSRf_r(r);
	  flags_carry_inverted = false;
	}
	STR_rRI(r, R_REGSTRUCT, idx);

  live.state[FLAGTMP].status = INMEM;
  live.state[FLAGTMP].realreg = -1;
  /* We just "evicted" FLAGTMP. */
  live.nat[r].nholds = 0;
}

STATIC_INLINE void raw_reg_to_flags(int r)
{
	MSR_CPSRf_r(r);
}


//
// compuemu_support used raw calls
//
LOWFUNC(WRITE,RMW,2,compemu_raw_inc_opcount,(IM16 op))
{
  uintptr idx = (uintptr) &(regs.raw_cputbl_count) - (uintptr) &regs;
  LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
#ifdef ARMV6T2
  MOVW_ri16(REG_WORK3, op);
#else
  uae_s32 offs = data_long_offs(op);
  LDR_rRI(REG_WORK3, RPC_INDEX, offs);
#endif
  LDR_rRR_LSLi(REG_WORK1, REG_WORK2, REG_WORK3, 2);
  ADD_rri(REG_WORK1, REG_WORK1, 1);
  STR_rRR_LSLi(REG_WORK1, REG_WORK2, REG_WORK3, 2);
}
LENDFUNC(WRITE,RMW,1,compemu_raw_inc_opcount,(IM16 op))

LOWFUNC(WRITE,READ,1,compemu_raw_cmp_pc,(IMPTR s))
{
  /* s is always >= NATMEM_OFFSET and < NATMEM_OFFSET + max. Amiga mem */
  clobber_flags();
  
  uintptr idx = (uintptr) &(regs.pc_p) - (uintptr) &regs;
  LDR_rRI(REG_WORK1, R_REGSTRUCT, idx);

  LOAD_U32(REG_WORK2, s);
	CMP_rr(REG_WORK1, REG_WORK2);
}
LENDFUNC(WRITE,READ,1,compemu_raw_cmp_pc,(IMPTR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_set_pc_i,(IMPTR s))
{
  LOAD_U32(REG_WORK2, s);
  uintptr idx = (uintptr) &(regs.pc_p) - (uintptr) &regs;
  STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_set_pc_i,(IMPTR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IM32 s))
{
  /* d points always to memory in regs struct */
  LOAD_U32(REG_WORK2, s);
  uintptr idx = d - (uintptr) &regs;
  STR_rRI(REG_WORK2, R_REGSTRUCT, idx);
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IM32 s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))
{
  /* d points always to memory in regs struct */
  uintptr idx = d - (uintptr) &regs;
  STR_rRI(s, R_REGSTRUCT, idx);
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
	LDR_rRI(d, R_REGSTRUCT, idx);
  } else {
	LOAD_U32(REG_WORK1, s);
	  LDR_rR(d, REG_WORK1);
  }
}
LENDFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))
{
	MOV_rr(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))

LOWFUNC(WRITE,RMW,1,compemu_raw_dec_m,(MEMRW d))
{
  clobber_flags();

  LOAD_U32(REG_WORK1, d);
  LDR_rR(REG_WORK2, REG_WORK1);
  SUBS_rri(REG_WORK2, REG_WORK2, 1);
  STR_rR(REG_WORK2, REG_WORK1);
}
LENDFUNC(WRITE,RMW,1,compemu_raw_dec_m,(MEMRW ds))

STATIC_INLINE void compemu_raw_call(uintptr t)
{
  LOAD_U32(REG_WORK1, t);

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
  FIX_INVERTED_CARRY
  
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

STATIC_INLINE void compemu_raw_handle_except(IM32 cycles)
{
	uae_u32* branchadd;	

  clobber_flags();

  uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
  LDR_rRI(REG_WORK1, R_REGSTRUCT, idx);
	TST_rr(REG_WORK1, REG_WORK1);
	branchadd = (uae_u32*)get_target();
	BEQ_i(0);		// no exception, jump to next instruction
	
  LOAD_U32(REG_PAR1, cycles);
  uae_u32* branchadd2 = (uae_u32*)get_target();
  B_i(0); // <exec_nostats>
  write_jmp_target(branchadd2, (uintptr)popall_execute_exception);
	
	// Write target of next instruction
	write_jmp_target(branchadd, (uintptr)get_target());
}

LOWFUNC(NONE,WRITE,1,compemu_raw_execute_normal,(MEMR s))
{
  LOAD_U32(REG_WORK1, s);
  LDR_rR(REG_WORK1, REG_WORK1);
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <exec_nostats>
  write_jmp_target(branchadd, (uintptr)popall_execute_normal_setpc);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_execute_normal,(MEMR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_check_checksum,(MEMR s))
{
  LOAD_U32(REG_WORK1, s);
  LDR_rR(REG_WORK1, REG_WORK1);
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <exec_nostats>
  write_jmp_target(branchadd, (uintptr)popall_check_checksum_setpc);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_check_checksum,(MEMR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_exec_nostats,(IMPTR s))
{
  LOAD_U32(REG_WORK1, s);
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <exec_nostats>
  write_jmp_target(branchadd, (uintptr)popall_exec_nostats_setpc);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_exec_nostats,(IMPTR s))

STATIC_INLINE void compemu_raw_maybe_recompile(void)
{
  uae_u32* branchadd = (uae_u32*)get_target();
  BLT_i(0);
  write_jmp_target(branchadd, (uintptr)popall_recompile_block);
}

STATIC_INLINE void compemu_raw_jmp(uintptr t)
{
	if(t >= (uintptr)popallspace && t < (uintptr)(popallspace + POPALLSPACE_SIZE + MAX_JIT_CACHE * 1024)) {
		uae_u32* loc = (uae_u32*)get_target();
		B_i(0);
		write_jmp_target(loc, t);
	} else {
	LDR_rRI(RPC_INDEX, RPC_INDEX, -4);
	  emit_long(t);
	}
}

STATIC_INLINE void compemu_raw_jmp_pc_tag(void)
{
  uintptr idx = (uintptr)&regs.pc_p - (uintptr)&regs;
  LDRH_rRI(REG_WORK1, R_REGSTRUCT, idx);
	idx = (uintptr)&regs.cache_tags - (uintptr)&regs;
	LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
	LDR_rRR_LSLi(RPC_INDEX, REG_WORK2, REG_WORK1, 2);
}

STATIC_INLINE void compemu_raw_maybe_cachemiss(void)
{
  uae_u32* branchadd = (uae_u32*)get_target();
  BNE_i(0);
  write_jmp_target(branchadd, (uintptr)popall_cache_miss);
}

STATIC_INLINE void compemu_raw_maybe_do_nothing(IM32 cycles)
{
  clobber_flags();

  uintptr idx = (uintptr)&regs.spcflags - (uintptr)&regs;
  LDR_rRI(REG_WORK1, R_REGSTRUCT, idx);
	TST_rr(REG_WORK1, REG_WORK1);
  uae_s8 *branchadd = (uae_s8 *)get_target();
  BEQ_i(0); // <end>

  idx = (uintptr)&countdown - (uintptr) &regs;
  LDR_rRI(REG_WORK2, R_REGSTRUCT, idx);
  if(CHECK32(cycles)) {
	SUB_rri(REG_WORK2, REG_WORK2, cycles);
  } else {
	LOAD_U32(REG_WORK1, cycles);
	  SUB_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
  }
  STR_rRI(REG_WORK2, R_REGSTRUCT, idx);

  uae_u32* branchadd2 = (uae_u32*)get_target();
  B_i(0);
  write_jmp_target(branchadd2, (uintptr)popall_do_nothing);

  // <end>
  write_jmp_target((uae_u32*)branchadd, (uintptr)get_target());
}

STATIC_INLINE void compemu_raw_branch(IM32 d)
{
	B_i((d >> 2) - 1);
}


// Optimize access to struct regstruct with and memory with fixed registers

LOWFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMPTR s))
{
  LOAD_U32(R_REGSTRUCT, s);
	uintptr offsmem = (uintptr)&NATMEM_OFFSET - (uintptr) &regs;
  LDR_rRI(R_MEMSTART, R_REGSTRUCT, offsmem);
}
LENDFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMPTR s))

// Handle end of compiled block
LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IM32 cycles))
{
  clobber_flags();

  // countdown -= scaled_cycles(totcycles);
  uintptr offs = (uintptr)&countdown - (uintptr)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  if(CHECK32(cycles)) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
	LOAD_U32(REG_WORK2, cycles);
	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);

  uae_u32* branchadd = (uae_u32*)get_target();
	CC_B_i(NATIVE_CC_MI, 0);
  write_jmp_target(branchadd, (uintptr)popall_do_nothing);
#ifdef ARMV6T2
	BFC_rii(rr_pc, 16, 31); // apply TAGMASK
#else
	BIC_rri(rr_pc, rr_pc, 0x00ff0000);
	BIC_rri(rr_pc, rr_pc, 0xff000000);
#endif
	offs = (uintptr)(&regs.cache_tags) - (uintptr)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
	LDR_rRR_LSLi(RPC_INDEX, REG_WORK1, rr_pc, 2);
}
LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IM32 cycles))

STATIC_INLINE uae_u32* compemu_raw_endblock_pc_isconst(IM32 cycles, IMPTR v)
{
  /* v is always >= NATMEM_OFFSET and < NATMEM_OFFSET + max. Amiga mem */
	uae_u32* tba;
  clobber_flags();

  // countdown -= scaled_cycles(totcycles);
  uintptr offs = (uintptr)&countdown - (uintptr)&regs;
	LDR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  if(CHECK32(cycles)) {
	  SUBS_rri(REG_WORK1, REG_WORK1, cycles);
	} else {
	LOAD_U32(REG_WORK2, cycles);
	SUBS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  }
	STR_rRI(REG_WORK1, R_REGSTRUCT, offs);

	tba = (uae_u32*)get_target();
  CC_B_i(NATIVE_CC_MI^1, 0); // <target set by caller>
  
  LDR_rRI(REG_WORK1, RPC_INDEX, 4); // <v>
  offs = (uintptr)&regs.pc_p - (uintptr)&regs;
  STR_rRI(REG_WORK1, R_REGSTRUCT, offs);
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0);
  write_jmp_target(branchadd, (uintptr)popall_do_nothing);

	emit_long(v);

	return tba;  
}

/*************************************************************************
* FPU stuff                                                             *
*************************************************************************/

#ifdef USE_JIT_FPU

LOWFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))
{
	VMOV64_dd(d, s);
}
LENDFUNC(NONE,NONE,2,raw_fmov_rr,(FW d, FR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_fmov_mr_drop,(MEMW mem, FR s))
{
  if(mem >= (uintptr) &regs && mem < (uintptr) &regs + 1020 && ((mem - (uintptr) &regs) & 0x3) == 0) {
	VSTR64_dRi(s, R_REGSTRUCT, (mem - (uintptr) &regs));
  } else {
	LOAD_U32(REG_WORK3, mem);
	if((mem & 0x3) == 0)
	  VSTR64_dRi(s, REG_WORK3, 0);
	else {
	  VMOV64_rrd(REG_WORK1, REG_WORK2, s);
#ifdef ALLOW_UNALIGNED_LDRD
	  STRD_rRI(REG_WORK1, REG_WORK3, 0);
#else
          STR_rRI(REG_WORK1, REG_WORK3, 0);
          STR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
	}
  }
}
LENDFUNC(NONE,WRITE,2,compemu_raw_fmov_mr_drop,(MEMW mem, FR s))

LOWFUNC(NONE,READ,2,compemu_raw_fmov_rm,(FW d, MEMR mem))
{
  if(mem >= (uintptr) &regs && mem < (uintptr) &regs + 1020 && ((mem - (uintptr) &regs) & 0x3) == 0) {
	VLDR64_dRi(d, R_REGSTRUCT, (mem - (uintptr) &regs));
  } else {
	LOAD_U32(REG_WORK3, mem);
	if((mem & 0x3) == 0)
	  VLDR64_dRi(d, REG_WORK3, 0);
	else {
#ifdef ALLOW_UNALIGNED_LDRD
		LDRD_rRI(REG_WORK1, REG_WORK3, 0);
#else
                LDR_rRI(REG_WORK1, REG_WORK3, 0);
                LDR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
		VMOV64_drr(d, REG_WORK1, REG_WORK2);
	}
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
  SIGNED16_REG_2_REG(REG_WORK1, s);
  VMOVi_from_ARM_dr(SCRATCH_F64_1, REG_WORK1, 0);
  VCVTIto64_ds(d, SCRATCH_F32_1);
}
LENDFUNC(NONE,NONE,2,raw_fmov_w_rr,(FW d, RR2 s))

LOWFUNC(NONE,NONE,2,raw_fmov_b_rr,(FW d, RR1 s))
{
  SIGNED8_REG_2_REG(REG_WORK1, s);
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
  LOAD_U32(REG_WORK3, m);
  if((m & 0x3) == 0)
	VLDR64_dRi(r, REG_WORK3, 0);
  else {
#ifdef ALLOW_UNALIGNED_LDRD
	LDRD_rRI(REG_WORK1, REG_WORK3, 0);
#else
        LDR_rRI(REG_WORK1, REG_WORK3, 0);
        LDR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
	VMOV64_drr(r, REG_WORK1, REG_WORK2);
  }
}
LENDFUNC(NONE,READ,2,raw_fmov_d_rm,(FW r, MEMR m))

LOWFUNC(NONE,READ,2,raw_fmovs_rm,(FW r, MEMR m))
{
  LOAD_U32(REG_WORK1, m);
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

  LOAD_U32(REG_WORK1, (uintptr)func);

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

  LOAD_U32(REG_WORK1, (uintptr)func);

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
  uae_u32* branchadd_iszero = (uae_u32*)get_target();
  BEQ_i(0);          // iszero

  UBFX_rrii(REG_WORK2, REG_WORK1, 20, 11);  	// get exponent
	MOVW_ri16(REG_WORK3, 2047);
	CMP_rr(REG_WORK2, REG_WORK3);

  uae_u32* branchadd_isnan = (uae_u32*)get_target();
	BEQ_i(0); 				// isnan

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
#ifdef ALLOW_UNALIGNED_LDRD
  STRD_rRI(REG_WORK1, REG_WORK3, 4);
#else
  STR_rRI(REG_WORK1, REG_WORK3, 4);
  STR_rRI(REG_WORK2, REG_WORK3, 8);
#endif
  uae_u32* branchadd_end = (uae_u32*)get_target();
  B_i(0);            // end_of_op

// isnan
  write_jmp_target(branchadd_isnan, (uintptr)get_target());
  MOVW_ri16(REG_WORK1, 0x7fff);
  LSL_rri(REG_WORK1, REG_WORK1, 16);
  MVN_ri(REG_WORK2, 0);

// iszero
  write_jmp_target(branchadd_iszero, (uintptr)get_target());
  CC_AND_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, 0x80000000); // extract sign
  CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);

	ADD_rrr(REG_WORK3, adr, R_MEMSTART);

  REV_rr(REG_WORK1, REG_WORK1);
#ifdef ALLOW_UNALIGNED_LDRD
  STRD_rR(REG_WORK1, REG_WORK3);
#else
  STR_rR(REG_WORK1, REG_WORK3);
  STR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
  STR_rRI(REG_WORK2, REG_WORK3, 8);

// end_of_op
  write_jmp_target(branchadd_end, (uintptr)get_target());
}
LENDFUNC(NONE,WRITE,2,raw_fp_from_exten_mr,(RR4 adr, FR s))

LOWFUNC(NONE,READ,2,raw_fp_to_exten_rm,(FW d, RR4 adr))
{
	ADD_rrr(REG_WORK3, adr, R_MEMSTART);

#ifdef ALLOW_UNALIGNED_LDRD
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
	uae_u32* branchadd_notzero = (uae_u32*)get_target();
	BNE_i(0);				// not_zero
	VCMP64_d0(d);
	VMRS_CPSR();
	uae_u32* branchadd_notzero2 = (uae_u32*)get_target();
	BNE_i(0);				// not zero

// zero
	VMOV_I64_dimmI(d, 0x00);
	TST_ri(REG_WORK1, 0x8000);								// check sign
	uae_u32* branchadd_end = (uae_u32*)get_target();
	BEQ_i(0);			// end_of_op
	MOV_ri(REG_WORK1, 0x80000000);
	MOV_ri(REG_WORK2, 0);
	VMOV64_drr(d, REG_WORK2, REG_WORK1);
	uae_u32* branchadd_end2 = (uae_u32*)get_target();
	B_i(0);					// end_of_op

// not_zero
  write_jmp_target(branchadd_notzero, (uintptr)get_target());
  write_jmp_target(branchadd_notzero2, (uintptr)get_target());
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
  write_jmp_target(branchadd_end, (uintptr)get_target());
  write_jmp_target(branchadd_end2, (uintptr)get_target());
}
LENDFUNC(NONE,READ,2,raw_fp_to_exten_rm,(FW d, RR4 adr))

LOWFUNC(NONE,WRITE,2,raw_fp_from_double_mr,(RR4 adr, FR s))
{
	ADD_rrr(REG_WORK3, adr, R_MEMSTART);
	VREV64_8_dd(SCRATCH_F64_1, s);
  VMOV64_rrd(REG_WORK1, REG_WORK2, SCRATCH_F64_1);
#ifdef ALLOW_UNALIGNED_LDRD
  STRD_rRI(REG_WORK1, REG_WORK3, 0);
#else
  STR_rRI(REG_WORK1, REG_WORK3, 0);
  STR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
}
LENDFUNC(NONE,WRITE,2,raw_fp_from_double_mr,(RR4 adr, FR s))

LOWFUNC(NONE,READ,2,raw_fp_to_double_rm,(FW d, RR4 adr))
{
	ADD_rrr(REG_WORK3, adr, R_MEMSTART);
#ifdef ALLOW_UNALIGNED_LDRD
	LDRD_rRI(REG_WORK1, REG_WORK3, 0);
#else
        LDR_rRI(REG_WORK1, REG_WORK3, 0);
        LDR_rRI(REG_WORK2, REG_WORK3, 4);
#endif
	VMOV64_drr(d, REG_WORK1, REG_WORK2);
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

#endif // USE_JIT_FPU

