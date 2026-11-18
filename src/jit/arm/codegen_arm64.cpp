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
#include <cmath>

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

#include "codegen_arm64.h"

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
		// Use 32-bit MOVN to keep upper 32 bits clean.
		// MOVN_xi produces a 64-bit result with dirty upper bits for negative values
		// (e.g., MOVN_xi for byte -1 -> 0xFFFFFFFFFFFFFFFF).
		MOVN_wi(r, (uae_u16) ~v16);
	} else {
		MOV_wi(r, (uae_u16) v16);
	}
}

STATIC_INLINE void UNSIGNED16_IMM_2_REG(W4 r, IM16 v) {
	// Use 32-bit MOV to keep upper 32 bits clean.
	MOV_wi(r, v);
}

STATIC_INLINE void SIGNED16_IMM_2_REG(W4 r, IM16 v) {
	if (v & 0x8000) {
		// Use 32-bit MOVN to keep upper 32 bits clean.
		MOVN_wi(r, (uae_u16) ~v);
	} else {
		MOV_wi(r, (uae_u16) v);
	}
}

STATIC_INLINE void UNSIGNED8_REG_2_REG(W4 d, RR4 s) {
	UXTB_xx(d, s);
}

STATIC_INLINE void SIGNED8_REG_2_REG(W4 d, RR4 s) {
	// Use 32-bit sign extension to keep upper 32 bits clean.
	// SXTB_xx sign-extends to 64 bits, leaving dirty upper 32 bits
	// for negative values (e.g., SXTB_xx of 0xFF -> 0xFFFFFFFFFFFFFFFF).
	SXTB_ww(d, s);
}

STATIC_INLINE void UNSIGNED16_REG_2_REG(W4 d, RR4 s) {
	UXTH_xx(d, s);
}

STATIC_INLINE void SIGNED16_REG_2_REG(W4 d, RR4 s) {
	// Use 32-bit sign extension to keep upper 32 bits clean.
	// SXTH_xx sign-extends to 64 bits, leaving dirty upper 32 bits
	// for negative values (e.g., SXTH_xx of 0xFFFF -> 0xFFFFFFFFFFFFFFFF).
	SXTH_ww(d, s);
}

STATIC_INLINE void LOAD_U32(int r, uae_u32 val)
{
	// Use 32-bit W-register instructions so upper 32 bits are always zeroed.
	// The 64-bit variants (MOVN_xi, MOV_xi) would sign-extend values like
	// 0xFFFFxxxx to 0xFFFFFFFFFFFFxxxx, corrupting Amiga addresses used
	// in register-indexed addressing [Xn, X27].
	if((val & 0xffff0000) == 0xffff0000) {
		MOVN_wi(r, ~val);
	} else {
		MOV_wi(r, val);
		if(val >> 16)
			MOVK_wish(r, val >> 16, 16);
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

STATIC_INLINE void raw_flags_to_reg(int r)
{
	MRS_NZCV_x(r);
	if(flags_carry_inverted) {
		EOR_xxCflag(r, r);
		MSR_NZCV_x(r);
		flags_carry_inverted = false;
	}
	// Use absolute address for regflags.nzcv instead of offset from regs,
	// because LDR/STR unsigned immediate truncates offsets >32760 bytes.
	LOAD_U64(REG_WORK3, (uintptr)&(regflags.nzcv));
	STR_wXi(r, REG_WORK3, 0);

	live.state[FLAGTMP].status = INMEM;
	live.state[FLAGTMP].realreg = -1;
	/* We just "evicted" FLAGTMP. */
	live.nat[r].nholds = 0;
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
	/* s is always >= NATMEM_OFFSET and < NATMEM_OFFSET + max. Amiga mem */
	clobber_flags();

	uintptr idx = (uintptr) &(regs.pc_p) - (uintptr) &regs;
	LDR_xXi(REG_WORK1, R_REGSTRUCT, idx); // regs.pc_p is 64 bit

	LOAD_U64(REG_WORK2, s);
	CMP_xx(REG_WORK1, REG_WORK2);
}
LENDFUNC(WRITE,READ,1,compemu_raw_cmp_pc,(IMPTR s))

STATIC_INLINE void compemu_raw_store_pc_state_from_work1(void)
{
	uintptr idx;

#ifdef NATMEM_OFFSET
	uae_u32 *branchadd = NULL;
	uae_u32 *branchdone = NULL;
	idx = (uintptr)&regs.pc_oldp - (uintptr)&regs;

	LOAD_U64(REG_WORK2, (uintptr)&canbang);
	LDRB_wXi(REG_WORK2, REG_WORK2, 0);
	CBZ_wi(REG_WORK2, 0);
	branchadd = (uae_u32 *)get_target() - 1;
	SUB_xxx(REG_WORK2, REG_WORK1, R_MEMSTART);
	B_i(0);
	branchdone = (uae_u32 *)get_target() - 1;

	write_jmp_target(branchadd, (uintptr)get_target());
	LDR_xXi(REG_WORK2, R_REGSTRUCT, idx);
	SUB_xxx(REG_WORK2, REG_WORK1, REG_WORK2);
	idx = (uintptr)&regs.pc - (uintptr)&regs;
	LDR_wXi(REG_WORK3, R_REGSTRUCT, idx);
	ADD_www(REG_WORK2, REG_WORK3, REG_WORK2);

	write_jmp_target(branchdone, (uintptr)get_target());
	STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
	idx = (uintptr)&regs.instruction_pc - (uintptr)&regs;
	STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
#endif

	idx = (uintptr)&regs.pc_p - (uintptr)&regs;
	STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
	idx = (uintptr)&regs.pc_oldp - (uintptr)&regs;
	STR_xXi(REG_WORK1, R_REGSTRUCT, idx);
}

LOWFUNC(NONE,WRITE,1,compemu_raw_set_pc_i,(IMPTR s))
{
	LOAD_U64(REG_WORK1, s);
	compemu_raw_store_pc_state_from_work1();
}
LENDFUNC(NONE,WRITE,1,compemu_raw_set_pc_i,(IMPTR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IMPTR s))
{
	uintptr idx = d - (uintptr) &regs;
	if(d == (uintptr) &(regs.pc_p) || d == (uintptr) &(regs.pc_oldp)) {
		LOAD_U64(REG_WORK2, s);  // pc_p/pc_oldp are 64-bit host pointers
		STR_xXi(REG_WORK2, R_REGSTRUCT, idx);
	} else if(idx <= 16380 && (idx & 3) == 0) {
		// Within R_REGSTRUCT unsigned-offset range (covers regs + nearby
		// globals like regflags that the linker places close by).
		LOAD_U32(REG_WORK2, (uae_u32)s);
		STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
	} else {
		// Address too far from R_REGSTRUCT - use absolute address.
		LOAD_U64(REG_WORK1, d);
		LOAD_U32(REG_WORK2, (uae_u32)s);
		STR_wXi(REG_WORK2, REG_WORK1, 0);
	}
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mi,(MEMW d, IMPTR s))

LOWFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))
{
	uintptr idx = d - (uintptr) &regs;
	if(d == (uintptr) &(regs.pc_p) || d == (uintptr) &(regs.pc_oldp)) {
		STR_xXi(s, R_REGSTRUCT, idx);
	} else if(idx <= 16380 && (idx & 3) == 0) {
		// Within R_REGSTRUCT unsigned-offset range (covers regs + nearby
		// globals like regflags that the linker places close by).
		STR_wXi(s, R_REGSTRUCT, idx);
	} else {
		// Address too far from R_REGSTRUCT - use absolute address.
		LOAD_U64(REG_WORK1, d);
		STR_wXi(s, REG_WORK1, 0);
	}
}
LENDFUNC(NONE,WRITE,2,compemu_raw_mov_l_mr,(MEMW d, RR4 s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IM32 s))
{
	LOAD_U32(d, s);
}
LENDFUNC(NONE,NONE,2,compemu_raw_mov_l_ri,(W4 d, IM32 s))

LOWFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))
{
	uintptr idx = s - (uintptr) &regs;
	if(s == (uintptr) &(regs.pc_p) || s == (uintptr) &(regs.pc_oldp)) {
		LDR_xXi(d, R_REGSTRUCT, idx);
	} else if(idx <= 16380 && (idx & 3) == 0) {
		// Within R_REGSTRUCT unsigned-offset range.
		LDR_wXi(d, R_REGSTRUCT, idx);
	} else {
		// Address too far from R_REGSTRUCT - use absolute address.
		LOAD_U64(REG_WORK1, s);
		LDR_wXi(d, REG_WORK1, 0);
	}
}
LENDFUNC(NONE,READ,2,compemu_raw_mov_l_rm,(W4 d, MEMR s))

LOWFUNC(NONE,NONE,2,compemu_raw_mov_l_rr,(W4 d, RR4 s))
{
	// Use 64-bit MOV to preserve full register width.  PC_P holds a
	// 64-bit host pointer; all other virtual registers hold 32-bit M68k
	// values whose upper 32 bits are already zeroed by W-register ops
	// upstream, so MOV_xx is safe for both cases.
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

#define FPU_JIT_HOST_FPSR_OFFS ((int)offsetof(struct regstruct, fpu_jit_host_fpsr))
#define FPU_JIT_HOST_STATE_OFFS ((int)offsetof(struct regstruct, fpu_jit_host_state))
static_assert(offsetof(struct regstruct, fpu_jit_host_state) < 16384,
	"fpu_jit_host_state is in reach of LDR w, [x28, #imm]");

/* regs.fpu_jit_host_fpsr |= host FPSR: the exceptions compiled host FPU code
   raised since an FPU block was entered or last called C code. x9-x10
   (caller-saved, neither arguments nor call targets here) */
STATIC_INLINE void compemu_raw_fpsr_collect(void)
{
	MRS_FPSR_x(9);
	LDR_wXi(10, R_REGSTRUCT, FPU_JIT_HOST_FPSR_OFFS);
	ORR_www(10, 10, 9);
	STR_wXi(10, R_REGSTRUCT, FPU_JIT_HOST_FPSR_OFFS);
}

/* if live (regs.fpu_jit_host_state bit 0) { clear it; collect } - code
   without host FPU instructions is about to run C code, which may clear the
   host FPSR or add its own exceptions. x9-x11 */
STATIC_INLINE void compemu_raw_fpsr_settle(void)
{
	LDR_wXi(11, R_REGSTRUCT, FPU_JIT_HOST_STATE_OFFS);
	uae_u32* branchadd = (uae_u32*)get_target();
	TBZ_wii(11, 0, 0);
	SUB_wwi(11, 11, 1);
	STR_wXi(11, R_REGSTRUCT, FPU_JIT_HOST_STATE_OFFS);
	compemu_raw_fpsr_collect();
	write_jmp_target(branchadd, (uintptr)get_target());
}

/* Entry of a block with host FPU instructions compiled for FPU JIT mode
   <mode> (-1: any mode, no FPCR check): the hot path is one compare of
   regs.fpu_jit_host_state with live in that mode, and the rest is out of line
   (compemu_raw_fpu_enter_stub()). Unless the host FPSR still holds the
   uncollected exceptions of compiled FPU code, it holds those of C code, which
   do not count: clear it. Entered with an FPCR of another mode, recompile
   the block. Leaves NZCV alone. */
STATIC_INLINE uae_u32* compemu_raw_fpu_enter(int mode)
{
	LDR_wXi(REG_WORK1, R_REGSTRUCT, FPU_JIT_HOST_STATE_OFFS);
	uae_u32* branchadd;
	if (mode < 0) {
		UBFX_wwii(REG_WORK2, REG_WORK1, 0, 1);
		branchadd = (uae_u32*)get_target();
		CBZ_wi(REG_WORK2, 0);
	} else {
		SUB_wwi(REG_WORK2, REG_WORK1, mode * 2 + 1);
		branchadd = (uae_u32*)get_target();
		CBNZ_wi(REG_WORK2, 0);
	}
	return branchadd;
}

STATIC_INLINE void compemu_raw_fpu_enter_stub(uae_u32* branch, uae_u32* resume, int mode, uintptr pc)
{
	write_jmp_target(branch, (uintptr)get_target());
	uae_u32* branch_mode = NULL;
	if (mode >= 0) {
		ADD_wwi(REG_WORK2, REG_WORK2, 1);	// zero: in the mode, not live
		branch_mode = (uae_u32*)get_target();
		CBNZ_wi(REG_WORK2, 0);
	}
	MSR_FPSR_x(31);
	MOV_wi(REG_WORK2, 1);
	ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
	STR_wXi(REG_WORK1, R_REGSTRUCT, FPU_JIT_HOST_STATE_OFFS);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);
	write_jmp_target(branchadd, (uintptr)resume);
	if (branch_mode) {
		write_jmp_target(branch_mode, (uintptr)get_target());
		compemu_raw_set_pc_i(pc);
		branchadd = (uae_u32*)get_target();
		B_i(0);
		write_jmp_target(branchadd, (uintptr)popall_fpu_mode_recompile);
	}
}

/* C code must not clear the exceptions of compiled host FPU code (the host
   FPU core's fp_clear_status()), nor add its own to them. A block without
   host FPU instructions only settles those of the blocks that ran before. */
STATIC_INLINE void compemu_raw_call_fpsr_before(void)
{
	if (jit_fpu_host_flags)
		compemu_raw_fpsr_collect();
	else if (jit_fpu_host_settle)
		compemu_raw_fpsr_settle();
}

STATIC_INLINE void compemu_raw_call_fpsr_after(void)
{
	if (jit_fpu_host_flags)
		MSR_FPSR_x(31);
}

STATIC_INLINE void compemu_raw_call(uintptr t)
{
	compemu_raw_call_fpsr_before();
	LOAD_U64(REG_WORK1, t);

	STR_xXpre(RLR_INDEX, RSP_INDEX, -16);
	BLR_x(REG_WORK1);
	LDR_xXpost(RLR_INDEX, RSP_INDEX, 16);
	compemu_raw_call_fpsr_after();
}

STATIC_INLINE void compemu_raw_call_r(RR4 r)
{
	compemu_raw_call_fpsr_before();
	STR_xXpre(RLR_INDEX, RSP_INDEX, -16);
	BLR_x(r);
	LDR_xXpost(RLR_INDEX, RSP_INDEX, 16);
	compemu_raw_call_fpsr_after();
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

	LOAD_U32(REG_PAR1, cycles);
	uae_u32* branchadd2 = (uae_u32*)get_target();
	B_i(0); // <exec_nostats>
	write_jmp_target(branchadd2, (uintptr)popall_execute_exception);

	// Write target of next instruction
	write_jmp_target(branchadd, (uintptr)get_target());
}

LOWFUNC(NONE,WRITE,1,compemu_raw_execute_normal,(MEMR s))
{
	LOAD_U64(REG_WORK1, s);
	LDR_xXi(REG_WORK1, REG_WORK1, 0);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0); // <exec_nostats>
	write_jmp_target(branchadd, (uintptr)popall_execute_normal_setpc);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_execute_normal,(MEMR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_check_checksum,(MEMR s))
{
	LOAD_U64(REG_WORK1, s);
	LDR_xXi(REG_WORK1, REG_WORK1, 0);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0); // <exec_nostats>
	write_jmp_target(branchadd, (uintptr)popall_check_checksum_setpc);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_check_checksum,(MEMR s))

LOWFUNC(NONE,WRITE,1,compemu_raw_exec_nostats,(IMPTR s))
{
	LOAD_U64(REG_WORK1, s);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0); // <exec_nostats>
	write_jmp_target(branchadd, (uintptr)popall_exec_nostats_setpc);
}
LENDFUNC(NONE,WRITE,1,compemu_raw_exec_nostats,(IMPTR s))

STATIC_INLINE void compemu_raw_maybe_recompile(void)
{
	BGE_i(2);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);
	write_jmp_target(branchadd, (uintptr)popall_recompile_block);
}

STATIC_INLINE void compemu_raw_jmp(uintptr t)
{
	uintptr loc = (uintptr)get_target();
	if(t > loc - 127 * 1024 * 1024 && t < loc + 127 * 1024 * 1024) {
		B_i(0);
		write_jmp_target((uae_u32*)loc, t);
	} else {
		LDR_xPCi(REG_WORK1, 8);
		BR_x(REG_WORK1);
		emit_quad(t);
	}
}

STATIC_INLINE void compemu_raw_jmp_pc_tag(void)
{
	uintptr idx = (uintptr)&regs.pc_p - (uintptr)&regs;
	LDRH_wXi(REG_WORK1, R_REGSTRUCT, idx);
	idx = (uintptr)&regs.cache_tags - (uintptr)&regs;
	LDR_xXi(REG_WORK2, R_REGSTRUCT, idx);
	LDR_xXxLSLi(REG_WORK1, REG_WORK2, REG_WORK1, 1);
	BR_x(REG_WORK1);
}

STATIC_INLINE void compemu_raw_maybe_cachemiss(void)
{
	BEQ_i(2);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);
	write_jmp_target(branchadd, (uintptr)popall_cache_miss);
}

STATIC_INLINE void compemu_raw_maybe_do_nothing(IM32 cycles)
{
	uintptr idx = (uintptr)&regs.spcflags - (uintptr) &regs;
	LDR_wXi(REG_WORK1, R_REGSTRUCT, idx);
	uae_s8 *branchadd = (uae_s8 *)get_target();
	CBZ_wi(REG_WORK1, 0);  // <end>

	// Use absolute address for countdown (global variable, may be >32KB from regs)
	LOAD_U64(REG_WORK3, (uintptr)&countdown);
	LDR_wXi(REG_WORK2, REG_WORK3, 0);
	if(cycles >= 0 && cycles <= 0xfff) {
		SUB_wwi(REG_WORK2, REG_WORK2, cycles);
	} else {
		LOAD_U32(REG_WORK1, cycles);
		SUB_www(REG_WORK2, REG_WORK2, REG_WORK1);
	}
	STR_wXi(REG_WORK2, REG_WORK3, 0);

	uae_u32* branchadd2 = (uae_u32*)get_target();
	B_i(0);
	write_jmp_target(branchadd2, (uintptr)popall_do_nothing);

	// <end>
	write_jmp_target((uae_u32 *)branchadd, (uintptr)get_target());
}

// Optimize access to struct regstruct with and memory with fixed registers

LOWFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMPTR s))
{
	LOAD_U64(R_REGSTRUCT, s);
	// Load NATMEM_OFFSET via its absolute address instead of offset from regs.
	// The old approach used LDR_xXi with offset = &NATMEM_OFFSET - &regs, but
	// LDR_xXi silently truncates offsets >32760 bytes (12-bit field), which
	// corrupts R27 when the global natmem_offset is far from regs in memory.
	LOAD_U64(R_MEMSTART, (uintptr)&NATMEM_OFFSET);
	LDR_xXi(R_MEMSTART, R_MEMSTART, 0);
}
LENDFUNC(NONE,NONE,1,compemu_raw_init_r_regstruct,(IMPTR s))

// Handle end of compiled block
LOWFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IM32 cycles))
{
	// countdown -= scaled_cycles(totcycles);
	// Use absolute address for countdown (global variable, may be >32KB from regs)
	LOAD_U64(REG_WORK3, (uintptr)&countdown);
	LDR_wXi(REG_WORK1, REG_WORK3, 0);
	if(cycles >= 0 && cycles <= 0xfff) {
		SUB_wwi(REG_WORK1, REG_WORK1, cycles);
	} else {
		LOAD_U32(REG_WORK2, cycles);
		SUB_www(REG_WORK1, REG_WORK1, REG_WORK2);
	}
	STR_wXi(REG_WORK1, REG_WORK3, 0);

	TBNZ_xii(REG_WORK1, 31, 5); // test sign and branch if set (negative)
	UBFIZ_xxii(rr_pc, rr_pc, 0, 16);  // apply TAGMASK
	uintptr offs = (uintptr)(&regs.cache_tags) - (uintptr)&regs;
	LDR_xXi(REG_WORK1, R_REGSTRUCT, offs);
	LDR_xXxLSLi(REG_WORK1, REG_WORK1, rr_pc, 3); // cacheline holds pointer -> multiply with 8
	BR_x(REG_WORK1);

	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);
	write_jmp_target(branchadd, (uintptr)popall_do_nothing);
}
LENDFUNC(NONE,NONE,2,compemu_raw_endblock_pc_inreg,(RR4 rr_pc, IM32 cycles))

STATIC_INLINE uae_u32* compemu_raw_endblock_pc_isconst(IM32 cycles, IMPTR v)
{
	/* v is always >= NATMEM_OFFSET and < NATMEM_OFFSET + max. Amiga mem */
	uae_u32* tba;

	// countdown -= scaled_cycles(totcycles);
	// Use absolute address for countdown (global variable, may be >32KB from regs)
	LOAD_U64(REG_WORK3, (uintptr)&countdown);
	LDR_wXi(REG_WORK1, REG_WORK3, 0);
	if(cycles >= 0 && cycles <= 0xfff) {
		SUB_wwi(REG_WORK1, REG_WORK1, cycles);
	} else {
		LOAD_U32(REG_WORK2, cycles);
		SUB_www(REG_WORK1, REG_WORK1, REG_WORK2);
	}
	STR_wXi(REG_WORK1, REG_WORK3, 0);

	TBNZ_xii(REG_WORK1, 31, 2); // test sign and branch if set (negative)
	tba = (uae_u32*)get_target();
	B_i(0); // <target set by caller>

	LDR_xPCi(REG_WORK1, 12); // <v>
	uintptr offs = (uintptr)&regs.pc_p - (uintptr)&regs;
	STR_xXi(REG_WORK1, R_REGSTRUCT, offs);
	uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);
	write_jmp_target(branchadd, (uintptr)popall_do_nothing);

	emit_quad(v);

	return tba;
}

/*************************************************************************
* FPU stuff                                                             *
*************************************************************************/

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
// FCMP: r = d - s, but equal infinities compare equal although their
// difference is NaN. They give a zero with the sign of d instead.
LOWFUNC(NONE,NONE,3,raw_fcmp_result_rrr,(FW r, FR d, FR s))
{
	FSUB_ddd(r, d, s);
	FCMP_dd(r, r);
	BVC_i(7);		// result is a number
	FCMP_dd(d, s);
	BNE_i(5);		// not equal or unordered
	FMOV_xd(REG_WORK1, d);
	LSR_xxi(REG_WORK1, REG_WORK1, 63);
	LSL_xxi(REG_WORK1, REG_WORK1, 63);
	FMOV_dx(r, REG_WORK1);
}
LENDFUNC(NONE,NONE,3,raw_fcmp_result_rrr,(FW r, FR d, FR s))


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
	STR_wXi(REG_WORK2, REG_WORK4, 0);         	// write exponent and the zero padding word
	ADD_xxi(REG_WORK4, REG_WORK4, 4);

	LSL_xxi(REG_WORK1, REG_WORK1, 11);          // shift mantissa to correct position
	REV_xx(REG_WORK1, REG_WORK1);
	SET_xxbit(REG_WORK1, REG_WORK1, 7);        // insert explicit 1
	STR_xXi(REG_WORK1, REG_WORK4, 0);
	uae_u32* branchadd_end = (uae_u32*)get_target();
	B_i(0);            // end_of_op

	// isnan or infinity: keep the sign, mantissa all ones for NaN, zero for infinity
	write_jmp_target(branchadd_isnan, (uintptr)get_target());
	LSL_xxi(REG_WORK2, REG_WORK1, 12);          // fraction
	CMP_xi(REG_WORK2, 0);
	CSETM_wc(REG_WORK2, NATIVE_CC_NE);
	UBFX_xxii(REG_WORK1, REG_WORK1, 63, 1);     // extract sign
	LSL_xxi(REG_WORK1, REG_WORK1, 31);
	MOV_xish(REG_WORK3, 0x7fff, 16);
	ORR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
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
	/* Rounded once, in the FPCR rounding mode, after the sign is applied.
	 * The mantissa is normalised: value = M * 2^K, M in [2^63, 2^64).
	 * Normal result: SCVTF rounds M to a double (the low bit made sticky as
	 * one bit is shifted out to fit a signed integer) and the scale 2^(K+1)
	 * is applied exactly in two multiplications; k is clamped where the
	 * result overflows whatever its exact value is.
	 * Denormal result (value < 2^-1022): the multiple N of 2^-1074 below it
	 * and two rounding bits (the second one sticky) are formed in integers,
	 * and N_even * 2^-1074 + (N & 1, rounding bits) * 2^-1076 is rounded by
	 * one fused multiply-add. */
	ADD_xxx(REG_WORK3, adr, R_MEMSTART);

	ADD_xxi(REG_WORK1, REG_WORK3, 4);
	LDR_xXi(REG_WORK1, REG_WORK1, 0);
	REV_xx(REG_WORK1, REG_WORK1);				// mantissa with the explicit bit

	LDRH_wXi(REG_WORK4, REG_WORK3, 0);
	REV16_xx(REG_WORK4, REG_WORK4);				// sign and exponent

	ANDS_xx7fff(REG_WORK2, REG_WORK4);

	/* exact in a double (as FMOVEM.X saved it): normalised, the low 11 bits of
	   the mantissa zero and the exponent in the range of normal doubles */
	UBFX_xxii(REG_WORK3, REG_WORK1, 0, 11);
	uae_u32* branch_inexact = (uae_u32*)get_target();
	CBNZ_xi(REG_WORK3, 0);
	uae_u32* branch_unnormal = (uae_u32*)get_target();
	TBZ_xii(REG_WORK1, 63, 0);
	MOV_xi(REG_WORK3, 15361);
	SUB_xxx(REG_WORK3, REG_WORK2, REG_WORK3);
	CMP_xi(REG_WORK3, 2045);
	uae_u32* branch_range = (uae_u32*)get_target();
	BHI_i(0);
	ADD_xxi(REG_WORK3, REG_WORK3, 1);			// double exponent
	UBFX_xxii(REG_WORK1, REG_WORK1, 11, 52);	// fraction
	BFI_xxii(REG_WORK1, REG_WORK3, 52, 11);
	UBFX_xxii(REG_WORK3, REG_WORK4, 15, 1);
	BFI_xxii(REG_WORK1, REG_WORK3, 63, 1);
	FMOV_dx(d, REG_WORK1);
	uae_u32* branch_end5 = (uae_u32*)get_target();
	B_i(0);

	write_jmp_target(branch_inexact, (uintptr)get_target());
	write_jmp_target(branch_unnormal, (uintptr)get_target());
	write_jmp_target(branch_range, (uintptr)get_target());
	MOV_xi(REG_WORK3, 0x7fff);
	CMP_xx(REG_WORK2, REG_WORK3);
	uae_u32* branch_special = (uae_u32*)get_target();
	BEQ_i(0);				// infinity or NaN
	uae_u32* branch_zero = (uae_u32*)get_target();
	CBZ_xi(REG_WORK1, 0);			// zero mantissa

	MOV_xi(REG_WORK3, 1);
	CMP_xi(REG_WORK2, 1);
	CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK3, NATIVE_CC_CS);	// denormals have exponent 1
	CLZ_xx(REG_WORK3, REG_WORK1);
	LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);	// M
	SUB_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
	MOV_xi(REG_WORK3, 16446);
	SUB_xxx(REG_WORK2, REG_WORK2, REG_WORK3);	// K
	MOV_xi(REG_WORK3, 1086);
	NEG_xx(REG_WORK3, REG_WORK3);
	CMP_xx(REG_WORK2, REG_WORK3);
	uae_u32* branch_denormal = (uae_u32*)get_target();
	BLE_i(0);				// value < 2^-1022

	UBFX_xxii(REG_WORK3, REG_WORK1, 0, 1);
	LSR_xxi(REG_WORK1, REG_WORK1, 1);
	ORR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);	// M >> 1, sticky
	uae_u32* branch_positive = (uae_u32*)get_target();
	TBZ_xii(REG_WORK4, 15, 0);
	NEG_xx(REG_WORK1, REG_WORK1);
	write_jmp_target(branch_positive, (uintptr)get_target());

	ADD_xxi(REG_WORK2, REG_WORK2, 1);			// k
	MOV_xi(REG_WORK3, 2000);
	CMP_xx(REG_WORK2, REG_WORK3);
	CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK3, NATIVE_CC_LE);
	ASR_xxi(REG_WORK3, REG_WORK2, 1);			// k1
	SUB_xxx(REG_WORK2, REG_WORK2, REG_WORK3);	// k2

	SCVTF_dx(d, REG_WORK1);
	ADD_xxi(REG_WORK3, REG_WORK3, 1023);
	LSL_xxi(REG_WORK3, REG_WORK3, 52);
	FMOV_dx(SCRATCH_F64_1, REG_WORK3);
	FMUL_ddd(d, d, SCRATCH_F64_1);
	ADD_xxi(REG_WORK2, REG_WORK2, 1023);
	LSL_xxi(REG_WORK2, REG_WORK2, 52);
	FMOV_dx(SCRATCH_F64_1, REG_WORK2);
	FMUL_ddd(d, d, SCRATCH_F64_1);
	uae_u32* branch_end = (uae_u32*)get_target();
	B_i(0);

	// denormal or zero result: s = -(K + 1074) bits below the last place
	write_jmp_target(branch_denormal, (uintptr)get_target());
	ADD_xxi(REG_WORK2, REG_WORK2, 1074);
	NEG_xx(REG_WORK2, REG_WORK2);				// s >= 12
	MOV_xi(REG_WORK3, 65);
	CMP_xx(REG_WORK2, REG_WORK3);
	CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK3, NATIVE_CC_LE);	// below half the last place either way
	MOV_xi(REG_WORK3, 66);
	SUB_xxx(REG_WORK3, REG_WORK3, REG_WORK2);
	LSL_xxx(REG_WORK3, REG_WORK1, REG_WORK3);
	CMP_xi(REG_WORK3, 0);
	CSET_xc(REG_WORK3, NATIVE_CC_NE);			// sticky: bits below s - 2
	SUB_xxi(REG_WORK4, REG_WORK2, 2);
	LSR_xxx(REG_WORK4, REG_WORK1, REG_WORK4);
	UBFX_xxii(REG_WORK4, REG_WORK4, 0, 2);
	ORR_xxx(REG_WORK4, REG_WORK4, REG_WORK3);	// rounding bits
	LSR_xxx(REG_WORK3, REG_WORK1, REG_WORK2);
	CMP_xi(REG_WORK2, 64);
	CSEL_xxxc(REG_WORK3, REG_WORK3, 31, NATIVE_CC_LT);	// N
	UBFX_xxii(REG_WORK1, REG_WORK3, 0, 1);
	ORR_xxxLSLi(REG_WORK4, REG_WORK4, REG_WORK1, 2);	// N & 1, rounding bits
	LSR_xxi(REG_WORK3, REG_WORK3, 1);
	LSL_xxi(REG_WORK3, REG_WORK3, 1);			// N_even
	ADD_xxx(REG_WORK1, adr, R_MEMSTART);
	LDRB_wXi(REG_WORK1, REG_WORK1, 0);
	uae_u32* branch_dpositive = (uae_u32*)get_target();
	TBZ_xii(REG_WORK1, 7, 0);
	NEG_xx(REG_WORK3, REG_WORK3);
	NEG_xx(REG_WORK4, REG_WORK4);
	write_jmp_target(branch_dpositive, (uintptr)get_target());

	SCVTF_dx(d, REG_WORK3);
	MOV_xish(REG_WORK1, 0x1E60, 48);			// 2^-537
	FMOV_dx(SCRATCH_F64_1, REG_WORK1);
	FMUL_ddd(d, d, SCRATCH_F64_1);
	FMUL_ddd(d, d, SCRATCH_F64_1);				// N_even * 2^-1074, exact
	SCVTF_dx(SCRATCH_F64_2, REG_WORK4);
	MOV_xish(REG_WORK1, 0x1E50, 48);			// 2^-538
	FMOV_dx(SCRATCH_F64_1, REG_WORK1);
	FMUL_ddd(SCRATCH_F64_2, SCRATCH_F64_2, SCRATCH_F64_1);
	FMADD_dddd(d, SCRATCH_F64_2, SCRATCH_F64_1, d);	// the only rounding
	uae_u32* branch_end4 = (uae_u32*)get_target();
	B_i(0);

	// zero, keeping the sign
	write_jmp_target(branch_zero, (uintptr)get_target());
	MOVI_di(d, 0);
	uae_u32* branch_end2 = (uae_u32*)get_target();
	TBZ_xii(REG_WORK4, 15, 0);
	MOV_xish(REG_WORK1, 0x8000, 48);
	FMOV_dx(d, REG_WORK1);
	uae_u32* branch_end3 = (uae_u32*)get_target();
	B_i(0);

	// infinity, or NaN if the fraction is not zero
	write_jmp_target(branch_special, (uintptr)get_target());
	LSL_xxi(REG_WORK1, REG_WORK1, 1);			// without the explicit bit
	MOV_xish(REG_WORK3, 0x7ff0, 48);
	uae_u32* branch_inf = (uae_u32*)get_target();
	CBZ_xi(REG_WORK1, 0);
	MOV_xish(REG_WORK3, 0x7ff8, 48);			// NaN
	write_jmp_target(branch_inf, (uintptr)get_target());
	UBFX_xxii(REG_WORK4, REG_WORK4, 15, 1);		// extract sign
	ORR_xxxLSLi(REG_WORK3, REG_WORK3, REG_WORK4, 63);
	FMOV_dx(d, REG_WORK3);

	write_jmp_target(branch_end, (uintptr)get_target());
	write_jmp_target(branch_end2, (uintptr)get_target());
	write_jmp_target(branch_end3, (uintptr)get_target());
	write_jmp_target(branch_end4, (uintptr)get_target());
	write_jmp_target(branch_end5, (uintptr)get_target());
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

		case NATIVE_CC_AL: // FST/ST: always set (emitted nothing before)
			SET_LOW8_xx(d, d);
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
