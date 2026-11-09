/*
 * compiler/compemu_midfunc_armA64.cpp - Native MIDFUNCS for AARCH64
 *
 * Copyright (c) 2019 TomB
 *
 * Inspired by Christian Bauer's Basilisk II
 *
 *  Original 68040 JIT compiler for UAE, copyright 2000-2002 Bernd Meyer
 *
 *  Adaptation for Basilisk II and improvements, copyright 2000-2002
 *    Gwenole Beauchesne
 *
 *  Basilisk II (C) 1997-2002 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Note:
 *  	File is included by compemu_support.cpp
 *
 */

/********************************************************************
 * CPU functions exposed to gencomp. Both CREATE and EMIT time      *
 ********************************************************************/

/*
 *  RULES FOR HANDLING REGISTERS:
 *
 *  * In the function headers, order the parameters
 *     - 1st registers written to
 *     - 2nd read/modify/write registers
 *     - 3rd registers read from
 *  * Before calling raw_*, you must call readreg, writereg or rmw for
 *    each register
 *  * The order for this is
 *     - 1nd call readreg for all registers read without offset
 *     - 2rd call rmw for all rmw registers
 *     - 3th call writereg for all written-to registers
 *     - 4th call raw_*
 *     - 5th unlock2 all registers that were locked
 */

#define INIT_REGS_b(d,s) 					\
	int targetIsReg = (d < 16); 		\
	int s_is_d = (s == d);  				\
	if(!s_is_d) 										\
		s = readreg(s); 							\
	d = rmw(d);											\
	if(s_is_d)											\
		s = d;

#define INIT_RREGS_b(d,s) 				\
	int targetIsReg = (d < 16); 		\
	int s_is_d = (s == d);  				\
	if(!s_is_d) 										\
		s = readreg(s); 							\
	d = readreg(d);									\
	if(s_is_d)											\
		s = d;

#define INIT_REG_b(d) 						\
	int targetIsReg = (d < 16); 		\
	d = rmw(d);

#define INIT_RREG_b(d) 						\
	int targetIsReg = (d < 16); 		\
	d = readreg(d);

#define INIT_WREG_b(d) 						\
	int targetIsReg = (d < 16); 		\
	if(targetIsReg)									\
		d = rmw(d); 									\
	else														\
		d = writereg(d);

#define INIT_REGS_w(d,s) 					\
	int targetIsReg = (d < 16); 		\
	int s_is_d = (s == d);  				\
	if(!s_is_d) 										\
		s = readreg(s); 							\
	d = rmw(d);											\
	if(s_is_d)											\
		s = d;

#define INIT_RREGS_w(d,s)					\
	int targetIsReg = (d < 16); 		\
	int s_is_d = (s == d);  				\
	if(!s_is_d) 										\
		s = readreg(s); 							\
	d = readreg(d);									\
	if(s_is_d)											\
		s = d;

#define INIT_REG_w(d) 						\
	int targetIsReg = (d < 16); 		\
	d = rmw(d);

#define INIT_RREG_w(d) 						\
	int targetIsReg = (d < 16); 		\
	d = readreg(d);

#define INIT_WREG_w(d) 						\
	int targetIsReg = (d < 16); 		\
	if(targetIsReg)									\
		d = rmw(d);										\
	else														\
		d = writereg(d);

#define INIT_REGS_l(d,s) 					\
	int targetIsReg = (d < 16); 		\
	int s_is_d = (s == d);  				\
	if(!s_is_d) 										\
		s = readreg(s); 							\
	d = rmw(d);											\
	if(s_is_d)											\
		s = d;

#define INIT_RREGS_l(d,s) 				\
	int targetIsReg = (d < 16); 		\
	int s_is_d = (s == d);  				\
	if(!s_is_d) 										\
		s = readreg(s); 							\
	d = readreg(d);									\
	if(s_is_d)											\
		s = d;

#define EXIT_REGS(d,s)						\
	unlock2(d);											\
	if(!s_is_d)											\
		unlock2(s);


MIDFUNC(0,live_flags,(void))
{
	live.flags_on_stack = TRASH;
	live.flags_in_flags = VALID;
	live.flags_are_important = 1;
}
MENDFUNC(0,live_flags,(void))

MIDFUNC(0,dont_care_flags,(void))
{
	live.flags_are_important = 0;
}
MENDFUNC(0,dont_care_flags,(void))

MIDFUNC(0,make_flags_live,(void))
{
	make_flags_live_internal();
}
MENDFUNC(0,make_flags_live,(void))

MIDFUNC(0,save_flags,(void))
{
	make_flags_live_internal();
	flush_flags();
}
MENDFUNC(0,save_flags,(void))

MIDFUNC(2,mov_l_mi,(IMPTR d, IMPTR s))
{
	/* d usually points to memory in regs struct, but can also be a global
	   (e.g. regflags.nzcv). Use absolute address if out of LDR/STR range. */
	if(d >= (uintptr)&regs && d < (uintptr)&regs + 32760) {
		uintptr idx = d - (uintptr) &regs;
		if(d == (uintptr) &(regs.pc_p) || d == (uintptr) &(regs.pc_oldp)) {
			LOAD_U64(REG_WORK2, s);  // pc_p/pc_oldp are 64-bit host pointers
			STR_xXi(REG_WORK2, R_REGSTRUCT, idx);
		} else {
			LOAD_U32(REG_WORK2, (uae_u32)s);
			STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
		}
	} else {
		LOAD_U32(REG_WORK2, (uae_u32)s);
		LOAD_U64(REG_WORK1, d);
		STR_wXi(REG_WORK2, REG_WORK1, 0);
	}
}
MENDFUNC(2,mov_l_mi,(IMPTR d, IMPTR s))

MIDFUNC(4,disp_ea20_target_add,(RW4 target, RR4 reg, IM8 shift, IM8 extend))
{
	if(isconst(target) && isconst(reg)) {
		if(extend)
			set_const(target, live.state[target].val + (((uae_s32)(uae_s16)live.state[reg].val) << (shift & 0x1f)));
		else
			set_const(target, live.state[target].val + (live.state[reg].val << (shift & 0x1f)));
		return;
	}

	reg = readreg(reg);
	target = rmw(target);

	if(extend) {
		SIGNED16_REG_2_REG(REG_WORK1, reg);
		ADD_wwwLSLi(target, target, REG_WORK1, shift & 0x1f);
	} else {
		ADD_wwwLSLi(target, target, reg, shift & 0x1f);
	}

	unlock2(target);
	unlock2(reg);
}
MENDFUNC(4,disp_ea20_target_add,(RW4 target, RR4 reg, IM8 shift, IM8 extend))

MIDFUNC(4,disp_ea20_target_mov,(W4 target, RR4 reg, IM8 shift, IM8 extend))
{
	if(isconst(reg)) {
		if(extend)
			set_const(target, ((uae_s32)(uae_s16)live.state[reg].val) << (shift & 0x1f));
		else
			set_const(target, live.state[reg].val << (shift & 0x1f));
		return;
	}

	reg = readreg(reg);
	target = writereg(target);

	if(extend) {
		SIGNED16_REG_2_REG(REG_WORK1, reg);
		LSL_wwi(target, REG_WORK1, shift & 0x1f);
	} else {
		LSL_wwi(target, reg, shift & 0x1f);
	}

	unlock2(target);
	unlock2(reg);
}
MENDFUNC(4,disp_ea20_target_mov,(W4 target, RR4 reg, IM8 shift, IM8 extend))

MIDFUNC(2,sign_extend_16_rr,(W4 d, RR2 s))
{
	// Only used in calc_disp_ea_020() -> flags not relevant and never modified
	if (isconst(s)) {
		set_const(d, (uae_s32)(uae_s16)live.state[s].val);
		return;
	}

	int s_is_d = (s == d);
	if (!s_is_d) {
		s = readreg(s);
		d = writereg(d);
	}	else {
		s = d = rmw(s);
	}
	SIGNED16_REG_2_REG(d, s);
	if (!s_is_d)
		unlock2(d);
	unlock2(s);
}
MENDFUNC(2,sign_extend_16_rr,(W4 d, RR2 s))

MIDFUNC(3,lea_l_brr,(W4 d, RR4 s, IM32 offset))
{
	if (isconst(s)) {
		COMPCALL(mov_l_ri)(d, live.state[s].val+offset);
		return;
	}

	int s_is_d = (s == d);
	if(s_is_d) {
		s = d = rmw(d);
	} else {
		s = readreg(s);
		d = writereg(d);
	}

	if(offset >= 0 && offset <= 0xfff) {
		ADD_wwi(d, s, offset);
	} else if(offset >= -0xfff && offset < 0) {
		SUB_wwi(d, s, -offset);
	} else {
		LOAD_U32(REG_WORK1, offset);
		ADD_www(d, s, REG_WORK1);
	}

	EXIT_REGS(d,s);
}
MENDFUNC(3,lea_l_brr,(W4 d, RR4 s, IM32 offset))

MIDFUNC(5,lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IM8 factor, IM8 offset))
{
	if (!offset) {
		COMPCALL(lea_l_rr_indexed)(d, s, index, factor);
		return;
	}
	if (isconst(s) && isconst(index)) {
		set_const(d, live.state[s].val + (uae_s32)(uae_s8)offset + live.state[index].val * factor);
		return;
	}

	s = readreg(s);
	if(d == index) {
		d = index = rmw(d);
	} else {
		index = readreg(index);
		d = writereg(d);
	}

	int shft;
	switch(factor) {
		case 1: shft=0; break;
		case 2: shft=1; break;
		case 4: shft=2; break;
		case 8: shft=3; break;
		default: abort();
	}

	if(offset >= 0 && offset <= 127) {
		ADD_wwi(REG_WORK1, s, offset);
	} else {
		SUB_wwi(REG_WORK1, s, -offset);
	}
	ADD_wwwLSLi(d, REG_WORK1, index, shft);

	unlock2(d);
	if(d != index)
		unlock2(index);
	unlock2(s);
}
MENDFUNC(5,lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IM8 factor, IM8 offset))

MIDFUNC(4,lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IM8 factor))
{
	if (isconst(s) && isconst(index)) {
		set_const(d, live.state[s].val + live.state[index].val * factor);
		return;
	}

	s = readreg(s);
	if(d == index) {
		d = index = rmw(d);
	} else {
		index = readreg(index);
		d = writereg(d);
	}

	int shft;
	switch(factor) {
		case 1: shft=0; break;
		case 2: shft=1; break;
		case 4: shft=2; break;
		case 8: shft=3; break;
		default: abort();
	}

	ADD_wwwLSLi(d, s, index, shft);

	unlock2(d);
	if(d != index)
		unlock2(index);
	unlock2(s);
}
MENDFUNC(4,lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IM8 factor))

MIDFUNC(2,mov_l_rr,(W4 d, RR4 s))
{
	int olds;

	if (d == s) { /* How pointless! */
		return;
	}
	if (isconst(s)) {
		COMPCALL(mov_l_ri)(d, live.state[s].val);
		return;
	}
	olds = s;
	disassociate(d);
	s = readreg(s);
	live.state[d].realreg = s;
	live.state[d].realind = live.nat[s].nholds;
	live.state[d].val = 0;
	set_status(d, DIRTY);

	live.nat[s].holds[live.nat[s].nholds] = d;
	live.nat[s].nholds++;
	unlock2(s);
}
MENDFUNC(2,mov_l_rr,(W4 d, RR4 s))

MIDFUNC(2,mov_l_mr,(IMPTR d, RR4 s))
{
	/* d usually points to memory in regs struct, but can also be a global
	   (e.g. regflags.nzcv). Use absolute address if out of LDR/STR range. */
	if (isconst(s)) {
		COMPCALL(mov_l_mi)(d, live.state[s].val);
		return;
	}

	s = readreg(s);

	if(d >= (uintptr)&regs && d < (uintptr)&regs + 32760) {
		uintptr idx = d - (uintptr) &regs;
		if(d == (uintptr)&regs.pc_oldp || d == (uintptr)&regs.pc_p)
			STR_xXi(s, R_REGSTRUCT, idx);
		else
			STR_wXi(s, R_REGSTRUCT, idx);
	} else {
		LOAD_U64(REG_WORK1, d);
		STR_wXi(s, REG_WORK1, 0);
	}

	unlock2(s);
}
MENDFUNC(2,mov_l_mr,(IMPTR d, RR4 s))

MIDFUNC(2,mov_l_rm,(W4 d, IMPTR s))
{
	/* s usually points to memory in regs struct, but can also be a global
	   (e.g. regflags.nzcv). Use absolute address if out of LDR/STR range. */
	d = writereg(d);

	if(s >= (uintptr)&regs && s < (uintptr)&regs + 32760) {
		uintptr idx = s - (uintptr) &regs;
		if (s == (uintptr)&regs.pc_p || s == (uintptr)&regs.pc_oldp)
			LDR_xXi(d, R_REGSTRUCT, idx);
		else
			LDR_wXi(d, R_REGSTRUCT, idx);
	} else {
		LOAD_U64(REG_WORK1, s);
		LDR_wXi(d, REG_WORK1, 0);
	}

	unlock2(d);
}
MENDFUNC(2,mov_l_rm,(W4 d, IMPTR s))

MIDFUNC(2,mov_l_ri,(W4 d, IMPTR s))
{
	set_const(d, s);
}
MENDFUNC(2,mov_l_ri,(W4 d, IMPTR s))

MIDFUNC(2,mov_b_ri,(W1 d, IM8 s))
{
	if(d < 16) {
		if (isconst(d)) {
			set_const(d, (live.state[d].val & 0xffffff00) | (s & 0x000000ff));
			return;
		}
		d = rmw(d);

		MOV_wi(REG_WORK1, (s & 0xff));
		BFI_wwii(d, REG_WORK1, 0, 8);

		unlock2(d);
	} else {
		set_const(d, s & 0xff);
	}
}
MENDFUNC(2,mov_b_ri,(W1 d, IM8 s))

MIDFUNC(2,sub_l_ri,(RW4 d, IM8 i))
{
	if (!i)
		return;
	if (isconst(d)) {
		// Preserve full 64-bit if the current value already exceeds 32 bits
		// (e.g. scratch register holding a host pointer for branch target).
		if (live.state[d].val > (uintptr)0xFFFFFFFFULL)
			live.state[d].val = live.state[d].val - i;
		else
			live.state[d].val = (uae_u32)(live.state[d].val - i);
		return;
	}

	d = rmw(d);

	SUB_wwi(d, d, i);

	unlock2(d);
}
MENDFUNC(2,sub_l_ri,(RW4 d, IM8 i))

MIDFUNC(2,sub_w_ri,(RW2 d, IM8 i))
{
	// This function is only called with i = 1
	// Caller needs flags...
	clobber_flags();

	d = rmw(d);

	LSL_wwi(REG_WORK2, d, 16);

	SUBS_wwish(REG_WORK2, REG_WORK2, (i & 0xff) << 4, 1);
	BFXIL_xxii(d, REG_WORK2, 16, 16);

	unlock2(d);
}
MENDFUNC(2,sub_w_ri,(RW2 d, IM8 i))

/* forget_about() takes a mid-layer register */
MIDFUNC(1,forget_about,(W4 r))
{
	if (isinreg(r))
		disassociate(r);
	live.state[r].val = 0;
	set_status(r, UNDEF);
}
MENDFUNC(1,forget_about,(W4 r))


MIDFUNC(2,arm_ADD_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		uintptr val = live.state[s].val;
		// When adding a 32-bit M68K displacement to PC_P (a 64-bit host
		// pointer), sign-extend the displacement first.  M68K branch
		// offsets are signed, but stored as unsigned uintptr in
		// live.state[].val.  Adding an unsigned 0xFFFFFE42 (i.e. -0x1BE)
		// to a 64-bit pointer causes carry into bit 32, producing 0x4...
		// instead of the correct 0x3...
		if (d == PC_P && val <= (uintptr)0xFFFFFFFFULL)
			val = (uintptr)(uae_s64)(uae_s32)val;
		COMPCALL(arm_ADD_l_ri)(d, val);
		return;
	}

	bool is_pcp = (d == PC_P);
	s = readreg(s);
	d = rmw(d);
	if (is_pcp) {
		/* PC_P holds a 64-bit host pointer. src is a signed 32-bit
		   displacement in a W register.  Use ADD Xd, Xd, Ws, SXTW
		   to sign-extend the 32-bit displacement to 64-bit before
		   adding, preserving the upper bits of the host pointer. */
		ADD_xxwEX(d, d, s, 6); // extend option 6 = SXTW
	} else
	{
		ADD_www(d, d, s);
	}
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,arm_ADD_l,(RW4 d, RR4 s))

MIDFUNC(2,arm_ADD_ldiv8,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		// signed, like the register path (ASR 5, LSL 2): a negative bit
		// offset gives a negative byte offset
		COMPCALL(arm_ADD_l_ri)(d,(uae_u32)(((uae_s32)live.state[s].val >> 5) * 4));
		return;
	}

	s = readreg(s);
	d = rmw(d);
	ASR_wwi(REG_WORK1, s, 5);
	ADD_wwwLSLi(d, d, REG_WORK1, 2);
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,arm_ADD_ldiv8,(RW4 d, RR4 s))

MIDFUNC(2,arm_ADD_l_ri,(RW4 d, IMPTR i))
{
	if (!i)
		return;
	if (isconst(d)) {
		// Preserve full 64-bit result when d is PC_P, when i is a 64-bit
		// host pointer, OR when val already exceeds 32 bits (e.g. scratch
		// register that received comp_pc_p in a previous arm_ADD_l_ri).
		if (d == PC_P || i > (IMPTR)0xFFFFFFFFULL || live.state[d].val > (uintptr)0xFFFFFFFFULL) {
			uintptr val = live.state[d].val;
			// When adding a 64-bit host pointer (i) to a 32-bit M68K value
			// (val), sign-extend val first.  M68K branch displacements are
			// signed, so a backward branch like -0x100 is stored as
			// 0xFFFFFF00.  Without sign extension, adding 0xFFFFFF00 to a
			// 64-bit pointer produces carry into bit 32 (0x4... instead of
			// 0x3...).
			if (val <= (uintptr)0xFFFFFFFFULL && i > (IMPTR)0xFFFFFFFFULL)
				val = (uintptr)(uae_s64)(uae_s32)val;
			live.state[d].val = val + i;
		} else {
			live.state[d].val = (uae_u32)(live.state[d].val + i);
		}
		return;
	}

	// Use 64-bit ADD when d is PC_P (always a host pointer) or when
	// the immediate itself exceeds 32 bits (e.g. branch target scratch
	// registers that hold natmem_offset + M68k_addr).
	bool is_ptr = (d == PC_P) || (i > (IMPTR)0xFFFFFFFFULL);
	bool is_pcp = (d == PC_P);
	d = rmw(d);

	if (is_ptr) {
		// 64-bit ADD to preserve upper bits (e.g. 0x300000000 on macOS).
		if (is_pcp) {
			// PC_P already holds a 64-bit host pointer; just add.
			if(i <= 0xfff) {
				ADD_xxi(d, d, i);
			} else {
				LOAD_U64(REG_WORK1, (uintptr)i);
				ADD_xxx(d, d, REG_WORK1);
			}
		} else {
			// d is a scratch holding a 32-bit M68K displacement in Wd.
			// Sign-extend Wd to Xd before adding the 64-bit pointer,
			// because M68K branch displacements are signed.
			LOAD_U64(REG_WORK1, (uintptr)i);
			ADD_xxwEX(d, REG_WORK1, d, 6); // ADD Xd, Ximm, Wd, SXTW
		}
	} else {
		// Use 32-bit ADD (ADD_www/ADD_wwi) to keep upper 32 bits of the X
		// register zeroed.  The result may later be used as an Amiga memory
		// address in [Xn, X27] indexed loads, which read the full 64-bit
		// register.  64-bit ADD_xxx could leave upper bits set on wrap.
		uae_u32 i32 = (uae_u32)i;
		if(i32 <= 0xfff) {
			ADD_wwi(d, d, i32);
		} else {
			LOAD_U32(REG_WORK1, i32);
			ADD_www(d, d, REG_WORK1);
		}
	}

	unlock2(d);
}
MENDFUNC(2,arm_ADD_l_ri,(RW4 d, IMPTR i))

MIDFUNC(2,arm_ADD_l_ri8,(RW4 d, IM8 i))
{
	if (!i)
		return;
	if (isconst(d)) {
		// Preserve full 64-bit if d is PC_P or already holds a 64-bit value
		if (d == PC_P || live.state[d].val > (uintptr)0xFFFFFFFFULL)
			live.state[d].val = live.state[d].val + i;
		else
			live.state[d].val = (uae_u32)(live.state[d].val + i);
		return;
	}

	bool is_ptr = (d == PC_P);
	d = rmw(d);
	if (is_ptr)
		ADD_xxi(d, d, i);  // 64-bit ADD for host pointer
	else
		ADD_wwi(d, d, i);  // 32-bit ADD for M68k values
	unlock2(d);
}
MENDFUNC(2,arm_ADD_l_ri8,(RW4 d, IM8 i))

MIDFUNC(2,arm_SUB_l_ri8,(RW4 d, IM8 i))
{
	if (!i)
		return;
	if (isconst(d)) {
		// Preserve full 64-bit if d is PC_P or already holds a 64-bit value
		if (d == PC_P || live.state[d].val > (uintptr)0xFFFFFFFFULL)
			live.state[d].val = live.state[d].val - i;
		else
			live.state[d].val = (uae_u32)(live.state[d].val - i);
		return;
	}

	bool is_ptr = (d == PC_P);
	d = rmw(d);
	if (is_ptr)
		SUB_xxi(d, d, i);  // 64-bit SUB for host pointer
	else
		SUB_wwi(d, d, i);  // 32-bit SUB for M68k values
	unlock2(d);
}
MENDFUNC(2,arm_SUB_l_ri8,(RW4 d, IM8 i))


#ifdef JIT_DEBUG
#include "aarch64.h"

void disam_range(void *start, void *stop)
{
  char disbuf[256];
  uint64_t disptr = (uint64_t)start;
  while(disptr < (uint64_t)stop) {
    disasm(disptr, disbuf);
    write_log("%016llx  %s\n", disptr, disbuf);
    disptr += 4;
  }
}

#endif

STATIC_INLINE void flush_cpu_icache(void *start, void *stop)
{
#ifdef JIT_DEBUG
	if((uae_u64)stop - (uae_u64)start > 4) {
    if(disasm_this) {
      char disbuf[256];
      uint64_t disptr = (uint64_t)start;
      while(disptr < (uint64_t)stop) {
        disasm(disptr, disbuf);
        write_log("%016llx  %s\n", disptr, disbuf);
        disptr += 4;
      }
      disasm_this = false;
    }
  }
#endif

#if defined(_WIN32) && defined(CPU_AARCH64)
	FlushInstructionCache(GetCurrentProcess(), start, (SIZE_T)((char *)stop - (char *)start));
#elif defined(__APPLE__) && defined(CPU_AARCH64)
	sys_icache_invalidate(start, (char *)stop - (char *)start);
#else
	__builtin___clear_cache((char *)start, (char *)stop);
#endif
}


STATIC_INLINE void write_jmp_target(uae_u32* jmpaddr, uintptr a)
{
	jit_begin_write_window();
	uintptr off = (a - (uintptr)jmpaddr) >> 2;
	if((*(jmpaddr) & 0xfc000000) == 0x14000000) {
		/* branch always */
		off = off & 0x3ffffff;
		*(jmpaddr) = (*(jmpaddr) & 0xfc000000) | off;
	} else if((*(jmpaddr) & 0x7c000000) == 0x34000000) {
		/* TBZ/TBNZ/CBZ/CBNZ */
		if((a > (uintptr)jmpaddr && off > 0x1fff) || (a < (uintptr)jmpaddr && (~off) > 0x1fff))
			write_log("JIT: TBZ/TBNZ branch to target too long.\n");
		off = off & 0x3fff;
		*(jmpaddr) = (*(jmpaddr) & 0xfffc001f) | (off << 5);
	} else {
		/* conditional branch */
		if((a > (uintptr)jmpaddr && off > 0x3ffff) || (a < (uintptr)jmpaddr && (~off) > 0x3ffff))
			write_log("JIT: Branch to target too long.\n");
		off = off & 0x7ffff;
		*(jmpaddr) = (*(jmpaddr) & 0xff00001f) | (off << 5);
	}

	flush_cpu_icache((void *)jmpaddr, (void *)&jmpaddr[1]);
	jit_end_write_window();
}

static inline void emit_jmp_target(uae_u32 a) {
	emit_long(a - (JITPTR target + 4));
}

/*************************************************************************
* FPU stuff                                                             *
*************************************************************************/

#ifdef USE_JIT_FPU

MIDFUNC(1,f_forget_about,(FW r))
{
	if (f_isinreg(r))
		f_disassociate(r);
	live.fate[r].status = UNDEF;
}
MENDFUNC(1,f_forget_about,(FW r))

MIDFUNC(0,dont_care_fflags,(void))
{
	f_disassociate(FP_RESULT);
}
MENDFUNC(0,dont_care_fflags,(void))

MIDFUNC(2,fmov_rr,(FW d, FR s))
{
	if (d == s) { /* How pointless! */
		return;
	}
	s = f_readreg(s);
	d = f_writereg(d);
	raw_fmov_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fmov_rr,(FW d, FR s))

MIDFUNC(2,fmov_l_rr,(FW d, RR4 s))
{
	s = readreg(s);
	d = f_writereg(d);
	raw_fmov_l_rr(d, s);
	f_unlock(d);
	unlock2(s);
}
MENDFUNC(2,fmov_l_rr,(FW d, RR4 s))

MIDFUNC(2,fmov_s_rr,(FW d, RR4 s))
{
	s = readreg(s);
	d = f_writereg(d);
	raw_fmov_s_rr(d, s);
	f_unlock(d);
	unlock2(s);
}
MENDFUNC(2,fmov_s_rr,(FW d, RR4 s))

MIDFUNC(2,fmov_w_rr,(FW d, RR2 s))
{
	s = readreg(s);
	d = f_writereg(d);
	raw_fmov_w_rr(d, s);
	f_unlock(d);
	unlock2(s);
}
MENDFUNC(2,fmov_w_rr,(FW d, RR2 s))

MIDFUNC(2,fmov_b_rr,(FW d, RR1 s))
{
	s = readreg(s);
	d = f_writereg(d);
	raw_fmov_b_rr(d, s);
	f_unlock(d);
	unlock2(s);
}
MENDFUNC(2,fmov_b_rr,(FW d, RR1 s))

MIDFUNC(3,fmov_d_rrr,(FW d, RR4 s1, RR4 s2))
{
	s1 = readreg(s1);
	s2 = readreg(s2);
	d = f_writereg(d);
	raw_fmov_d_rrr(d, s1, s2);
	f_unlock(d);
	unlock2(s2);
	unlock2(s1);
}
MENDFUNC(3,fmov_d_rrr,(FW d, RR4 s1, RR4 s2))

MIDFUNC(2,fmov_l_ri,(FW d, IM32 i))
{
	switch(i) {
		case 0:
			fmov_d_ri_0(d);
			break;
		case 1:
			fmov_d_ri_1(d);
			break;
		case 10:
			fmov_d_ri_10(d);
			break;
		case 100:
			fmov_d_ri_100(d);
			break;
		default:
			d = f_writereg(d);
			compemu_raw_mov_l_ri(REG_WORK1, i);
			raw_fmov_l_rr(d, REG_WORK1);
			f_unlock(d);
	}
}
MENDFUNC(2,fmov_l_ri,(FW d, IM32 i))

MIDFUNC(2,fmov_s_ri,(FW d, IM32 i))
{
	d = f_writereg(d);
	compemu_raw_mov_l_ri(REG_WORK1, i);
	raw_fmov_s_rr(d, REG_WORK1);
	f_unlock(d);
}
MENDFUNC(2,fmov_s_ri,(FW d, IM32 i))

/* FPSR exception byte of an FMOVE FPn to integer (L/W/B), as the
 * interpreter sets it: OPERR (and accrued IOP) when the rounded value does
 * not fit the integer size or is a NaN/infinity, else INEX2 (and accrued
 * INEX) when rounding changed it; the rest of the exception byte is
 * cleared. The rounded value is in SCRATCH_F64_1 (raw_fmov_to_*: FRINTI).
 * Enabled OPERR/INEX traps are not taken in compiled code. Uses the host
 * flags: the 68k CCR is saved first (FMOVE does not change it). */
static void emit_fpsr_int_conv(int s, int size)
{
	clobber_flags();
	FCVTAS_wd(REG_WORK1, SCRATCH_F64_1);        // saturated 32-bit value
	SCVTF_dw(SCRATCH_F64_2, REG_WORK1);
	FCMP_dd(SCRATCH_F64_2, SCRATCH_F64_1);      // != or unordered: out of range
	uae_u32 *branch_operr1 = (uae_u32 *)get_target();
	BNE_i(0);
	uae_u32 *branch_operr2 = NULL;
	if (size < 4) {
		if (size == 2)
			SXTH_ww(REG_WORK2, REG_WORK1);
		else
			SXTB_ww(REG_WORK2, REG_WORK1);
		CMP_ww(REG_WORK1, REG_WORK2);
		branch_operr2 = (uae_u32 *)get_target();
		BNE_i(0);
	}
	MOV_wi(REG_WORK3, 0);
	FCMP_dd(SCRATCH_F64_1, s);                  // rounding changed it: inexact
	uae_u32 *branch_exact = (uae_u32 *)get_target();
	BEQ_i(0);
	MOV_wi(REG_WORK3, 0x0208);                  // INEX2, accrued INEX
	uae_u32 *branch_store = (uae_u32 *)get_target();
	B_i(0);
	write_jmp_target(branch_operr1, (uintptr)get_target());
	if (branch_operr2)
		write_jmp_target(branch_operr2, (uintptr)get_target());
	MOV_wi(REG_WORK3, 0x2080);                  // OPERR, accrued IOP
	write_jmp_target(branch_exact, (uintptr)get_target());
	write_jmp_target(branch_store, (uintptr)get_target());
	LOAD_U64(REG_WORK4, (uintptr)&regs.fpsr);
	LDR_wXi(REG_WORK2, REG_WORK4, 0);
	MOV_wi(REG_WORK1, 0xff00);
	BIC_www(REG_WORK2, REG_WORK2, REG_WORK1);   // exception byte
	ORR_www(REG_WORK2, REG_WORK2, REG_WORK3);
	STR_wXi(REG_WORK2, REG_WORK4, 0);
}

/* FScc/FBcc with an IEEE-nonaware predicate ($10-$1F): with NAN set, BSUN
 * and the accrued IOP are set (fpp_cond()). NAN is the FPSR CC bit when the
 * CC were written directly (fpu_fpsr_cc_written), else FP_RESULT is a NaN.
 * An enabled BSUN trap is not taken in compiled code. */
MIDFUNC(0,fp_bsun_iop,(void))
{
	clobber_flags();
	int r = f_readreg(FP_RESULT);               // before any branch: may load it
	LOAD_U64(REG_WORK4, (uintptr)&regs.fpsr);
	LOAD_U64(REG_WORK1, (uintptr)&fpu_fpsr_cc_written);
	LDRB_wXi(REG_WORK1, REG_WORK1, 0);
	uae_u32 *branch_result = (uae_u32 *)get_target();
	CBZ_wi(REG_WORK1, 0);
	LDR_wXi(REG_WORK2, REG_WORK4, 0);
	uae_u32 *branch_nonan1 = (uae_u32 *)get_target();
	TBZ_wii(REG_WORK2, 24, 0);                  // CC NAN clear
	uae_u32 *branch_set = (uae_u32 *)get_target();
	B_i(0);
	write_jmp_target(branch_result, (uintptr)get_target());
	FCMP_dd(r, r);
	uae_u32 *branch_nonan2 = (uae_u32 *)get_target();
	CC_B_i(NATIVE_CC_VC, 0);                    // ordered: not a NaN
	write_jmp_target(branch_set, (uintptr)get_target());
	LDR_wXi(REG_WORK2, REG_WORK4, 0);
	MOV_wi(REG_WORK3, 0x8080);                  // BSUN, accrued IOP
	ORR_www(REG_WORK2, REG_WORK2, REG_WORK3);
	STR_wXi(REG_WORK2, REG_WORK4, 0);
	write_jmp_target(branch_nonan1, (uintptr)get_target());
	write_jmp_target(branch_nonan2, (uintptr)get_target());
	f_unlock(r);
}
MENDFUNC(0,fp_bsun_iop,(void))

// AMIBERRY_JIT_FPU_STATS: counts[key * 4 + path]++ (x2, x16 clobbered; the registers are flushed)
static void fpx_count(uae_u32 opext, int path)
{
	uae_u64 *s = fp_jit_stats();
	if (!s)
		return;
	LOAD_U64(16, (uintptr)&s[fp_jit_stats_key(opext >> 16, (uae_u16)opext) * 4 + path]);
	LDR_xXi(2, 16, 0);
	ADD_xxi(2, 2, 1);
	STR_xXi(2, 16, 0);
}

/* A call of a helper with up to four arguments in x0-x3: through x16, as
 * compemu_raw_call() loads its target into x2 (REG_WORK1), the third
 * argument. All registers are flushed before. */
static void fpx_call4(uintptr fn)
{
	LOAD_U64(16, fn);
	compemu_raw_call_r(16);
}

/* ---- the exact core's common FPU instructions inline (fp_jit_inline_kind()):
 * the leaf helper fpj_leaf (fpp_softfloat.cpp) as emitted code, decoded at
 * compile time. Runs after the registers are flushed, in x0 and x2-x17
 * (x1 holds the source value for the helper call); every case it does not
 * handle branches to the helper call, which does it. Only the fast path's
 * exit is taken without the call. Same results as fpj_leaf, which
 * softfloat_fastx80.h computes (FADD/FSUB/FMUL: fx80_addsub, fx80_mul). */
#if defined(JIT) && defined(__SIZEOF_INT128__)
#define FPX_OFF(field) ((uae_u32)((uintptr)&(regs.field) - (uintptr)&regs))
#define FPX_TOP(r) MOV_xish(r, 0x8000, 48)

struct fpx_fails {
	uae_u32 *at[32];
	int n;
	void add() { at[n++] = (uae_u32 *)get_target(); }
};

static void fpx_mov32(int r, uae_u32 v)
{
	MOV_wi(r, v & 0xffff);
	if (v >> 16)
		MOVK_wish(r, v >> 16, 16);
}

// a normal value (wh, xl): exponent 1..0x7ffe, the integer bit set; else
// fail.
static void fpx_normal(fpx_fails &f, int wh, int xl)
{
	f.add(); TBZ_xii(xl, 63, 0);
	ADD_wwi(16, wh, 1);                                // exponent 0 or $7FFF: bits 14-1 of it + 1 clear
	TST_wbm(16, 31, 13);                               // $7FFE
	f.add(); BEQ_i(0);
}

/* The inline code's gate: none of the FPCR bits in mask set, no exception
 * pending, and (pcr060) the 68060 FPU enabled; else fail. fp_exp_pend and
 * fp_unimp_pend are one 64-bit load. */
static void fpx_gate(fpx_fails &f, uae_u32 mask, bool pcr060)
{
	static_assert(offsetof(struct regstruct, fp_unimp_pend) == offsetof(struct regstruct, fp_exp_pend) + 4,
		"fp_exp_pend and fp_unimp_pend are adjacent");
	static_assert(offsetof(struct regstruct, fp_exp_pend) % 8 == 0, "fp_exp_pend is 8-byte aligned");
	if (mask) {
		LDR_wXi(16, R_REGSTRUCT, FPX_OFF(fpcr));
		if (mask == 0xfff0) {
			TST_wbm(16, 28, 11);
		} else if (mask == 0xff00) {
			TST_wbm(16, 24, 7);
		} else {
			fpx_mov32(12, mask);
			TST_ww(16, 12);
		}
		f.add(); BNE_i(0);
	}
	LDR_xXi(12, R_REGSTRUCT, FPX_OFF(fp_exp_pend));
	f.add(); CBNZ_xi(12, 0);
	if (pcr060) {
		LDR_wXi(12, R_REGSTRUCT, FPX_OFF(pcr));
		f.add(); TBNZ_wii(12, 1, 0);                   // DFP: the FPU disabled
	}
}

// a normal value or a zero (wh, xl); else fail.
static void fpx_normal_or_zero(fpx_fails &f, int wh, int xl)
{
	uae_u32 *normal = (uae_u32 *)get_target(); TBNZ_xii(xl, 63, 0);
	f.add(); CBNZ_xi(xl, 0);                           // denormal or unnormal
	UBFX_wwii(16, wh, 0, 15);
	f.add(); CBNZ_wi(16, 0);                           // infinity, or an unnormal zero
	uae_u32 *done = (uae_u32 *)get_target(); B_i(0);
	write_jmp_target(normal, (uintptr)get_target());
	ADD_wwi(16, wh, 1);
	TST_wbm(16, 31, 13);
	f.add(); BEQ_i(0);
	write_jmp_target(done, (uintptr)get_target());
}

// kind: fp_jit_inline_kind(); the result is left in x0 (0: the fast path did
// it) when it falls through, else a failure branch was taken
static void fpx_emit_inline(fpx_fails &f, uae_u32 kind, uae_u32 opext, uae_u32 pc)
{
	const int op = (kind & 0xff) - 1, src = (kind >> 8) & 0xf;
	const bool fpiar = (kind >> 12) & 1, pcr060 = (kind >> 13) & 1;
	const uae_u16 opcode = opext >> 16, extra = (uae_u16)opext;
	const int sreg = (extra >> 10) & 7, dreg = (extra >> 7) & 7;
	enum { K_MOVE, K_ADD, K_SUB, K_MUL, K_DIV, K_SQRT, K_ABS, K_NEG, K_CMP, K_TST };   // FK_* (fpp_softfloat.cpp)
	enum { S_FPM, S_L, S_S, S_W, S_B, S_D, S_X };                                    // FJS_*

	// FPCR: no exception enabled, extended precision, round to nearest; nothing pending
	fpx_gate(f, 0xfff0, pcr060);
	MOV_wi(17, 0x7ffd);                                    // w17: the normal exponent limit, throughout

	// zeros too: +-0 operands (FADD/FSUB/FMUL: softfloat's zero rules below)
	const bool zeros = true;
	uae_u32 *src_zero_done = nullptr;
	// the source in (w6, x7)
	if (src == S_FPM || src == S_X) {
		if (src == S_FPM) {
			LDRH_wXi(6, R_REGSTRUCT, FPX_OFF(fp[sreg].fpx.high));
			LDR_xXi(7, R_REGSTRUCT, FPX_OFF(fp[sreg].fpx.low));
		} else {                                           // .X at the address in w1, read directly (fpx_in_direct())
			LDR_wXx(12, 1, R_MEMSTART);
			REV_ww(12, 12);
			LSR_wwi(6, 12, 16);
			ADD_wwi(13, 1, 4);
			LDR_wXx(14, 13, R_MEMSTART);
			REV_ww(14, 14);
			ADD_wwi(13, 1, 8);
			LDR_wXx(15, 13, R_MEMSTART);
			REV_ww(15, 15);
			ORR_xxxLSLi(7, 15, 14, 32);
		}
		if (zeros)
			fpx_normal_or_zero(f, 6, 7);
		else
			fpx_normal(f, 6, 7);
	} else if (src == S_D) {
		MOV_xx(12, 1);
		if (zeros) {                                       // +-0: sign only
			LSL_xxi(13, 12, 1);
			uae_u32 *nonzero = (uae_u32 *)get_target(); CBNZ_xi(13, 0);
			LSR_xxi(6, 12, 48);
			MOV_wi(13, 0x8000);
			AND_www(6, 6, 13);
			MOV_xi(7, 0);
			src_zero_done = (uae_u32 *)get_target(); B_i(0);
			write_jmp_target(nonzero, (uintptr)get_target());
		}
		UBFX_xxii(13, 12, 52, 11);
		SUB_wwi(14, 13, 1);
		MOV_wi(15, 0x7fd);
		CMP_ww(14, 15);
		f.add(); BHI_i(0);                                 // denormal, infinity, NaN
		MOV_wi(14, 0x3c00);
		ADD_www(6, 13, 14);
		LSR_xxi(14, 12, 48);
		MOV_wi(15, 0x8000);
		AND_www(14, 14, 15);
		ORR_www(6, 6, 14);
		LSL_xxi(7, 12, 11);
		FPX_TOP(15);
		ORR_xxx(7, 7, 15);
	} else {
		if (((opcode >> 3) & 7) == 0)
			LDR_wXi(12, R_REGSTRUCT, FPX_OFF(regs[opcode & 7]));
		else
			MOV_ww(12, 1);
		if (src == S_S) {
			if (zeros) {
				LSL_wwi(13, 12, 1);
				uae_u32 *nonzero = (uae_u32 *)get_target(); CBNZ_wi(13, 0);
				LSR_wwi(6, 12, 16);
				MOV_wi(13, 0x8000);
				AND_www(6, 6, 13);
				MOV_xi(7, 0);
				src_zero_done = (uae_u32 *)get_target(); B_i(0);
				write_jmp_target(nonzero, (uintptr)get_target());
			}
			UBFX_wwii(13, 12, 23, 8);
			SUB_wwi(14, 13, 1);
			CMP_wi(14, 0xfd);
			f.add(); BHI_i(0);
			MOV_wi(14, 0x3f80);
			ADD_www(6, 13, 14);
			LSR_wwi(14, 12, 16);
			MOV_wi(15, 0x8000);
			AND_www(14, 14, 15);
			ORR_www(6, 6, 14);
			LSL_xxi(7, 12, 40);
			FPX_TOP(15);
			ORR_xxx(7, 7, 15);
		} else {                                           // integer: int32_to_floatx80()
			if (src == S_W)
				SXTH_ww(12, 12);
			else if (src == S_B)
				SXTB_ww(12, 12);
			if (zeros) {                                   // 0: +0
				uae_u32 *nonzero = (uae_u32 *)get_target(); CBNZ_wi(12, 0);
				MOV_wi(6, 0);
				MOV_xi(7, 0);
				src_zero_done = (uae_u32 *)get_target(); B_i(0);
				write_jmp_target(nonzero, (uintptr)get_target());
			} else {
				f.add(); CBZ_wi(12, 0);                    // zero
			}
			ASR_wwi(13, 12, 31);
			EOR_www(14, 12, 13);
			SUB_www(14, 14, 13);                           // |i|
			CLZ_ww(15, 14);
			MOV_wi(6, 0x401e);
			SUB_www(6, 6, 15);
			MOV_wi(16, 0x8000);
			AND_www(13, 13, 16);
			ORR_www(6, 6, 13);
			ADD_wwi(15, 15, 32);
			LSL_xxx(7, 14, 15);
		}
	}

	if (src_zero_done)
		write_jmp_target(src_zero_done, (uintptr)get_target());
	const bool dyadic = op == K_ADD || op == K_SUB || op == K_MUL || op == K_CMP;
	if (dyadic) {                                          // the destination in (w8, x9)
		LDRH_wXi(8, R_REGSTRUCT, FPX_OFF(fp[dreg].fpx.high));
		LDR_xXi(9, R_REGSTRUCT, FPX_OFF(fp[dreg].fpx.low));
		if (zeros)
			fpx_normal_or_zero(f, 8, 9);
		else
			fpx_normal(f, 8, 9);
	}
	bool inexact = false;                                  // w5: inexact (ADD/SUB/MUL)
	if (op == K_ADD || op == K_SUB || op == K_MUL) {
		inexact = true;
		// to the packing below: w16 sign, w2 exponent, x14 significand, x15 lost bits
		uae_u32 *to_pack[4];
		int npack = 0;
		// zero results, handled after the packing: (w10, x11) set there, inexact (w5) clear
		uae_u32 *z_mul[2] = { nullptr, nullptr }, *z_bzero = nullptr, *z_azero = nullptr, *z_cancel = nullptr;
		if (op == K_MUL) {
			z_mul[0] = (uae_u32 *)get_target(); CBZ_xi(9, 0);
			z_mul[1] = (uae_u32 *)get_target(); CBZ_xi(7, 0);
			EOR_www(16, 8, 6);
			LSR_wwi(16, 16, 15);
			UBFX_wwii(2, 8, 0, 15);
			UBFX_wwii(3, 6, 0, 15);
			ADD_www(2, 2, 3);
			MOV_wi(0, 0x3ffe);
			SUB_www(2, 2, 0);
			MUL_xxx(15, 9, 7);
			UMULH_xxx(14, 9, 7);
			to_pack[npack++] = (uae_u32 *)get_target(); TBNZ_xii(14, 63, 0);
			EXTR_xxxi(14, 14, 15, 63);                     // (hi:lo) << 1
			LSL_xxi(15, 15, 1);
			SUB_wwi(2, 2, 1);
		} else {
			LSR_wwi(12, 8, 15);                            // a: the destination
			LSR_wwi(13, 6, 15);                            // b: the source
			if (op == K_SUB) {
				MOV_wi(0, 1);
				EOR_www(13, 13, 0);
			}
			z_bzero = (uae_u32 *)get_target(); CBZ_xi(7, 0);
			z_azero = (uae_u32 *)get_target(); CBZ_xi(9, 0);
			UBFX_wwii(14, 8, 0, 15);
			UBFX_wwii(15, 6, 0, 15);
			SUBS_xxx(31, 9, 7);                            // C clear: |a| < |b|, swap
			SBCS_www(31, 14, 15);
			CSEL_wwwc(16, 13, 12, NATIVE_CC_CC);           // w16 sign, w10 the other's
			CSEL_wwwc(10, 12, 13, NATIVE_CC_CC);
			CSEL_wwwc(2, 15, 14, NATIVE_CC_CC);            // w2 exponent of the larger, w3 the smaller's
			CSEL_wwwc(3, 14, 15, NATIVE_CC_CC);
			CSEL_xxxc(4, 7, 9, NATIVE_CC_CC);              // x4 larger significand, x5 smaller
			CSEL_xxxc(5, 9, 7, NATIVE_CC_CC);
			SUB_www(0, 2, 3);                              // d
			CMP_wi(0, 64);
			f.add(); BCS_i(0);                             // 64 or more binades apart
			LSR_xxx(14, 5, 0);                             // b aligned
			NEG_xx(15, 0);
			LSL_xxx(15, 5, 15);                            // its bits shifted out
			CMP_wi(0, 0);
			CSEL_xxxc(15, 31, 15, NATIVE_CC_EQ);
			CMP_ww(16, 10);
			uae_u32 *to_sub = (uae_u32 *)get_target(); BNE_i(0);
			ADDS_xxx(14, 4, 14);
			to_pack[npack++] = (uae_u32 *)get_target(); BCC_i(0);
			// carry: one more binade, the sum's lowest bit joins the lost ones
			UBFX_xxii(3, 15, 0, 1);
			LSR_xxi(15, 15, 1);
			ORR_xxx(15, 15, 3);
			ORR_xxxLSLi(15, 15, 14, 63);
			LSR_xxi(14, 14, 1);
			FPX_TOP(3);
			ORR_xxx(14, 14, 3);
			ADD_wwi(2, 2, 1);
			to_pack[npack++] = (uae_u32 *)get_target(); B_i(0);
			write_jmp_target(to_sub, (uintptr)get_target());
			// (x4:0) - (x14:x15), exact
			NEGS_xx(15, 15);
			SBC_xxx(14, 4, 14);
			ORR_xxx(3, 14, 15);
			z_cancel = (uae_u32 *)get_target(); CBZ_xi(3, 0);   // exact zero: +0 (round to nearest)
			to_pack[npack++] = (uae_u32 *)get_target(); TBNZ_xii(14, 63, 0);
			uae_u32 *hi_zero = (uae_u32 *)get_target(); CBZ_xi(14, 0);
			CLZ_xx(3, 14);
			LSL_xxx(14, 14, 3);
			NEG_xx(0, 3);
			LSR_xxx(0, 15, 0);
			ORR_xxx(14, 14, 0);
			LSL_xxx(15, 15, 3);
			SUB_www(2, 2, 3);
			uae_u32 *to_pack2 = (uae_u32 *)get_target(); B_i(0);
			write_jmp_target(hi_zero, (uintptr)get_target());
			CLZ_xx(3, 15);
			LSL_xxx(14, 15, 3);
			MOV_xi(15, 0);
			SUB_www(2, 2, 3);
			SUB_wwi(2, 2, 64);
			write_jmp_target(to_pack2, (uintptr)get_target());
		}
		for (int i = 0; i < npack; i++)
			write_jmp_target(to_pack[i], (uintptr)get_target());
		// round to nearest even (fx80_pack)
		FPX_TOP(3);
		CMP_xx(15, 3);
		CSET_wc(0, NATIVE_CC_HI);
		CSET_wc(4, NATIVE_CC_EQ);
		AND_www(4, 4, 14);
		ORR_www(0, 0, 4);
		ADDS_xxx(14, 14, 0);
		CSEL_xxxc(14, 3, 14, NATIVE_CC_CS);
		CSET_wc(4, NATIVE_CC_CS);
		ADD_www(2, 2, 4);
		SUB_wwi(4, 2, 1);
		CMP_ww(4, 17);
		f.add(); BHI_i(0);                                 // underflow or overflow
		ORR_wwwLSLi(10, 2, 16, 15);
		MOV_xx(11, 14);
		CMP_xi(15, 0);
		CSET_wc(5, NATIVE_CC_NE);
		// zero operands, exact, as softfloat (SOFTFLOAT_68K) rounds to nearest
		uae_u32 *tail[4];
		int ntail = 0;
		tail[ntail++] = (uae_u32 *)get_target(); B_i(0);
		if (op == K_MUL) {                                 // 0 * y: +-0, the signs' exclusive or
			for (int i = 0; i < 2; i++)
				write_jmp_target(z_mul[i], (uintptr)get_target());
			EOR_www(0, 8, 6);
			LSR_wwi(0, 0, 15);
			LSL_wwi(10, 0, 15);
			MOV_xi(11, 0);
			MOV_wi(5, 0);
		} else {
			// b +-0: a, or with a +-0 too: that zero if the signs agree, else +0
			write_jmp_target(z_bzero, (uintptr)get_target());
			uae_u32 *both = (uae_u32 *)get_target(); CBZ_xi(9, 0);
			MOV_ww(10, 8);
			MOV_xx(11, 9);
			MOV_wi(5, 0);
			tail[ntail++] = (uae_u32 *)get_target(); B_i(0);
			write_jmp_target(both, (uintptr)get_target());
			AND_www(0, 12, 13);
			LSL_wwi(10, 0, 15);
			MOV_xi(11, 0);
			MOV_wi(5, 0);
			tail[ntail++] = (uae_u32 *)get_target(); B_i(0);
			// a +-0, b normal: b (its sign as added)
			write_jmp_target(z_azero, (uintptr)get_target());
			UBFX_wwii(10, 6, 0, 15);
			ORR_wwwLSLi(10, 10, 13, 15);
			MOV_xx(11, 7);
			MOV_wi(5, 0);
			tail[ntail++] = (uae_u32 *)get_target(); B_i(0);
			// x - x: +0
			write_jmp_target(z_cancel, (uintptr)get_target());
			MOV_wi(10, 0);
			MOV_xi(11, 0);
			MOV_wi(5, 0);
		}
		for (int i = 0; i < ntail; i++)
			write_jmp_target(tail[i], (uintptr)get_target());
	} else if (op == K_CMP) {
		// floatx80_cmp() of normal values: Z when equal, N when dst < src
		CMP_ww(8, 6);
		CCMP_xxfc(9, 7, 0, NATIVE_CC_EQ);
		CSET_wc(2, NATIVE_CC_EQ);
		UBFX_wwii(3, 8, 0, 15);
		UBFX_wwii(4, 6, 0, 15);
		SUBS_xxx(31, 9, 7);
		SBCS_www(31, 3, 4);
		CSET_wc(0, NATIVE_CC_CC);                          // |dst| < |src|
		LSR_wwi(3, 8, 15);
		LSR_wwi(4, 6, 15);
		EOR_www(4, 3, 4);
		EOR_www(0, 3, 0);
		CMP_wi(4, 0);
		CSEL_wwwc(0, 3, 0, NATIVE_CC_NE);                  // less
		LSL_wwi(0, 0, 3);
		MOV_wi(4, 4);
		CMP_wi(2, 0);
		CSEL_wwwc(0, 4, 0, NATIVE_CC_NE);              // w0: the condition code nibble
		// floatx80_cmp() of two zeros: Z, and N with a negative destination
		ORR_xxx(2, 9, 7);
		uae_u32 *not_zeros = (uae_u32 *)get_target(); CBNZ_xi(2, 0);
		LSR_wwi(3, 8, 15);
		LSL_wwi(3, 3, 3);
		MOV_wi(0, 4);
		ORR_www(0, 0, 3);
		write_jmp_target(not_zeros, (uintptr)get_target());
	} else {
		MOV_ww(10, 6);
		MOV_xx(11, 7);
		if (op == K_ABS) {
			BFI_wwii(10, 31, 15, 1);
		} else if (op == K_NEG) {
			MOV_wi(0, 0x8000);
			EOR_www(10, 10, 0);
		}
	}
	if (op != K_CMP) {                                     // N from the result's sign, Z for a zero
		UBFX_wwii(0, op == K_TST ? 6 : 10, 15, 1);
		LSL_wwi(0, 0, 3);
		if (zeros) {
			CMP_xi(op == K_TST ? 7 : 11, 0);
			CSET_wc(4, NATIVE_CC_EQ);
			ORR_wwwLSLi(0, 0, 4, 2);
		}
	}
	// the condition codes, the exception byte cleared (bits 0-2 and 28-31 are 0)
	LDR_wXi(2, R_REGSTRUCT, FPX_OFF(fpsr));
	BFI_wwii(2, 0, 24, 4);
	BFI_wwii(2, 31, 8, 8);
	if (inexact) {
		const uae_u32 inex = fp_jit_inexact_status();
		if (inex == 0x208) {                               // INEX2 and the accrued INEX
			LSL_wwi(3, 5, 3);
			ORR_wwwLSLi(3, 3, 5, 9);
		} else {
			fpx_mov32(3, inex);
			CMP_wi(5, 0);
			CSEL_wwwc(3, 3, 31, NATIVE_CC_NE);
		}
		ORR_www(2, 2, 3);
	}
	STR_wXi(2, R_REGSTRUCT, FPX_OFF(fpsr));
	if (op != K_CMP && op != K_TST) {
		STRH_wXi(10, R_REGSTRUCT, FPX_OFF(fp[dreg].fpx.high));
		STR_xXi(11, R_REGSTRUCT, FPX_OFF(fp[dreg].fpx.low));
	}
	MOV_wi(3, 1);
	STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpu_state));
	STRB_wXi(31, R_REGSTRUCT, FPX_OFF(fp_exception));
	if (fpiar) {
		fpx_mov32(3, pc);
		STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpiar));
	}
}

/* FMOVE FPn,<ea> inline (fp_jit_out_inline_kind()): fpj_out (fpp_softfloat.cpp)
 * as code. The destination is a data register, or memory the JIT may write
 * directly (as writelong() decides at compile time), at the address in w1;
 * (An)+ and -(An) are stepped here. Every case it does not handle branches
 * to the helper call. */
/* FMOVEM FPn<->memory inline (static register list, fp_jit_fmovem_plan()):
 * the raw words copied, memory read or written directly; the address is in
 * w1, (An)+ and -(An) are stepped here. FMOVEM leaves the FPSR and FPIAR
 * alone. A pending exception, or a disabled 68060 FPU: the helper call. */
static void fpx_emit_fmovem(fpx_fails &f, uae_u32 kind, uae_u32 opext, const struct fpj_fmovem &p)
{
	const uae_u16 opcode = opext >> 16;
	const int mode = (opcode >> 3) & 7, reg = opcode & 7;
	fpx_gate(f, 0, (kind >> 13) & 1);
	for (int i = 0; i < p.n; i++) {
		const int off = p.off[i], r = p.reg[i];
		if (off >= 0)
			ADD_wwi(4, 1, off);
		else
			SUB_wwi(4, 1, -off);
		ADD_wwi(5, 4, 4);
		ADD_wwi(6, 4, 8);
		if (p.to_mem) {
			LDRH_wXi(2, R_REGSTRUCT, FPX_OFF(fp[r].fpx.high));
			LSL_wwi(2, 2, 16);                             // word 1: the exponent
			LDR_xXi(3, R_REGSTRUCT, FPX_OFF(fp[r].fpx.low));
			LSR_xxi(16, 3, 32);                            // word 2, word 3 in w3
			REV_ww(2, 2);
			REV_ww(16, 16);
			REV_ww(3, 3);
			STR_wXx(p.reversed ? 3 : 2, 4, R_MEMSTART);
			STR_wXx(16, 5, R_MEMSTART);
			STR_wXx(p.reversed ? 2 : 3, 6, R_MEMSTART);
		} else {
			LDR_wXx(2, 4, R_MEMSTART);
			LDR_wXx(3, 5, R_MEMSTART);
			LDR_wXx(16, 6, R_MEMSTART);
			REV_ww(2, 2);
			REV_ww(3, 3);
			REV_ww(16, 16);
			LSR_wwi(2, 2, 16);
			ORR_xxxLSLi(3, 16, 3, 32);
			STRH_wXi(2, R_REGSTRUCT, FPX_OFF(fp[r].fpx.high));
			STR_xXi(3, R_REGSTRUCT, FPX_OFF(fp[r].fpx.low));
		}
	}
	if (mode == 3 || mode == 4) {
		if (p.final >= 0)
			ADD_wwi(2, 1, p.final);
		else
			SUB_wwi(2, 1, -p.final);
		STR_wXi(2, R_REGSTRUCT, FPX_OFF(regs[8 + reg]));
	}
	MOV_wi(3, 1);
	STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpu_state));
	STRB_wXi(31, R_REGSTRUCT, FPX_OFF(fp_exception));
}

/* FMOVECR #ofs,FPn inline: the constant and its FPSR bits from
 * fp_jit_fmovecr() at compile time, valid at round to nearest, extended
 * precision; another FPCR, a pending exception or a disabled 68060 FPU:
 * the helper call. */
static void fpx_emit_fmovecr(fpx_fails &f, uae_u32 kind, uae_u32 opext, uae_u32 pc)
{
	uae_u16 hi;
	uae_u64 lo;
	uae_u32 ccexc, accr;
	fp_jit_fmovecr((uae_u16)opext, &hi, &lo, &ccexc, &accr);
	const int dreg = (opext >> 7) & 7;
	fpx_gate(f, 0xfff0, (kind >> 13) & 1);
	MOV_wi(2, hi);
	STRH_wXi(2, R_REGSTRUCT, FPX_OFF(fp[dreg].fpx.high));
	LOAD_U64(3, lo);
	STR_xXi(3, R_REGSTRUCT, FPX_OFF(fp[dreg].fpx.low));
	LDR_wXi(2, R_REGSTRUCT, FPX_OFF(fpsr));
	fpx_mov32(3, (ccexc >> 24) & 0xf);
	BFI_wwii(2, 3, 24, 4);
	fpx_mov32(3, (ccexc >> 8) & 0xff);
	BFI_wwii(2, 3, 8, 8);
	if (accr) {
		fpx_mov32(3, accr);
		ORR_www(2, 2, 3);
	}
	STR_wXi(2, R_REGSTRUCT, FPX_OFF(fpsr));
	MOV_wi(3, 1);
	STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpu_state));
	STRB_wXi(31, R_REGSTRUCT, FPX_OFF(fp_exception));
	if ((kind >> 12) & 1) {
		fpx_mov32(3, pc);
		STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpiar));
	}
}

// the JIT may read a long of this instruction's memory directly (readlong())
static bool fpx_in_direct(void)
{
	return !(special_mem & S_READ) && !distrust_long() && !jit_n_addr_unsafe && !currprefs.address_space_24;
}

static bool fpx_out_direct(int size)
{
	const int ok = size == 6 ? !distrust_byte() : size == 4 ? !distrust_word() : !distrust_long();
	return !(special_mem & S_WRITE) && ok && !jit_n_addr_unsafe && !currprefs.address_space_24;
}

// rounding of q (x14) by rem (x15) at the top: x0 = 1 to round up (nearest even)
static void fpx_round_up_rn(void)
{
	FPX_TOP(4);
	CMP_xx(15, 4);
	CSET_wc(0, NATIVE_CC_HI);
	CSET_wc(2, NATIVE_CC_EQ);
	AND_www(2, 2, 14);
	ORR_www(0, 0, 2);
}

static void fpx_emit_out(fpx_fails &f, uae_u32 kind, uae_u32 opext, uae_u32 pc)
{
	const uae_u16 opcode = opext >> 16, extra = (uae_u16)opext;
	const int size = (extra >> 10) & 7, sreg = (extra >> 7) & 7, mode = (opcode >> 3) & 7, reg = opcode & 7;
	const bool fpiar = (kind >> 12) & 1, pcr060 = (kind >> 13) & 1;
	const bool fltfmt = size == 1 || size == 5;

	// FPCR: no exception enabled; round to nearest for single and double
	fpx_gate(f, fltfmt ? 0xff30 : 0xff00, pcr060);
	LDRH_wXi(6, R_REGSTRUCT, FPX_OFF(fp[sreg].fpx.high));
	LDR_xXi(7, R_REGSTRUCT, FPX_OFF(fp[sreg].fpx.low));
	fpx_normal_or_zero(f, 6, 7);
	uae_u32 *to_zero = size != 2 ? (uae_u32 *)get_target() : nullptr;
	if (to_zero)
		CBZ_xi(7, 0);                                      // +-0: stored as the format's zero

	bool inexact = true;                                   // w5
	if (size == 0 || size == 4 || size == 6) {             // fx80_to_int(), the FPCR's rounding mode
		const int bits = size == 0 ? 32 : size == 4 ? 16 : 8;
		UBFX_wwii(12, 6, 0, 15);
		MOV_wi(13, 0x3fff);
		SUB_www(12, 12, 13);                               // e
		CMP_wi(12, bits - 1);
		f.add(); BGE_i(0);                                 // out of range (or -2^31: softfloat)
		uae_u32 *neg_e = (uae_u32 *)get_target(); TBNZ_wii(12, 31, 0);
		MOV_wi(13, 63);
		SUB_www(13, 13, 12);
		LSR_xxx(14, 7, 13);                                // q
		ADD_wwi(13, 12, 1);
		LSL_xxx(15, 7, 13);                                // rem
		uae_u32 *have = (uae_u32 *)get_target(); B_i(0);
		write_jmp_target(neg_e, (uintptr)get_target());
		MOV_xi(14, 0);
		MOV_xi(15, 1);
		ADDS_wwi(31, 12, 1);                               // e == -1: rem is the significand
		CSEL_xxxc(15, 7, 15, NATIVE_CC_EQ);
		write_jmp_target(have, (uintptr)get_target());
		LDR_wXi(16, R_REGSTRUCT, FPX_OFF(fpcr));
		UBFX_wwii(16, 16, 4, 2);                           // 0 RN, 1 RZ, 2 RM, 3 RP
		LSR_wwi(3, 6, 15);                                 // negative
		fpx_round_up_rn();
		CMP_xi(15, 0);
		CSET_wc(2, NATIVE_CC_NE);
		UBFX_wwii(4, 16, 0, 1);
		EOR_www(4, 4, 3);
		AND_www(2, 2, 4);                                  // RM: negative, RP: positive, inexact
		CMP_wi(16, 1);
		CSEL_wwwc(2, 31, 2, NATIVE_CC_EQ);                 // RZ: never
		CMP_wi(16, 0);
		CSEL_wwwc(0, 0, 2, NATIVE_CC_EQ);
		ADD_xxx(14, 14, 0);
		fpx_mov32(4, (1u << (bits - 1)) - 1);
		ADD_xxx(4, 4, 3);                                  // 2^(bits-1) when negative
		CMP_xx(14, 4);
		f.add(); BHI_i(0);                                 // overflow
		NEG_ww(4, 14);
		CMP_wi(3, 0);
		CSEL_wwwc(10, 4, 14, NATIVE_CC_NE);
	} else if (size == 1 || size == 5) {                   // fx80_to_f32()/fx80_to_f64()
		const bool d = size == 5;
		UBFX_wwii(12, 6, 0, 15);
		MOV_wi(13, d ? 0x3c00 : 0x3f80);
		SUB_www(12, 12, 13);                               // the format's biased exponent
		SUB_wwi(13, 12, 1);
		MOV_wi(0, d ? 0x7fd : 0xfd);
		CMP_ww(13, 0);
		f.add(); BHI_i(0);                                 // overflow or underflow
		LSR_xxi(14, 7, d ? 11 : 40);
		LSL_xxi(15, 7, d ? 53 : 24);
		fpx_round_up_rn();
		ADD_xxx(14, 14, 0);
		uae_u32 *nocarry = (uae_u32 *)get_target(); TBZ_xii(14, d ? 53 : 24, 0);
		LSR_xxi(14, 14, 1);
		ADD_wwi(12, 12, 1);
		MOV_wi(0, d ? 0x7fe : 0xfe);
		CMP_ww(12, 0);
		f.add(); BHI_i(0);
		write_jmp_target(nocarry, (uintptr)get_target());
		LSR_wwi(3, 6, 15);
		if (d) {
			UBFX_xxii(14, 14, 0, 52);
			ORR_xxxLSLi(10, 14, 12, 52);
			ORR_xxxLSLi(10, 10, 3, 63);
		} else {
			UBFX_wwii(14, 14, 0, 23);
			ORR_wwwLSLi(10, 14, 12, 23);
			ORR_wwwLSLi(10, 10, 3, 31);
		}
	} else {
		inexact = false;                                   // X: the register unchanged
	}
	if (inexact) {
		CMP_xi(15, 0);
		CSET_wc(5, NATIVE_CC_NE);
	}
	if (to_zero) {                                         // the format's zero of the sign: exact
		uae_u32 *skip = (uae_u32 *)get_target(); B_i(0);
		write_jmp_target(to_zero, (uintptr)get_target());
		if (size == 1) {
			LSR_wwi(3, 6, 15);
			LSL_wwi(10, 3, 31);
		} else if (size == 5) {
			LSR_wwi(3, 6, 15);
			LSL_xxi(10, 3, 63);
		} else {
			MOV_wi(10, 0);
		}
		MOV_wi(5, 0);
		write_jmp_target(skip, (uintptr)get_target());
	}

	if (mode == 0) {
		const uae_u32 off = FPX_OFF(regs[reg]);
		if (size == 4 || size == 6) {
			LDR_wXi(2, R_REGSTRUCT, off);
			BFI_wwii(2, 10, 0, size == 4 ? 16 : 8);
			STR_wXi(2, R_REGSTRUCT, off);
		} else {
			STR_wXi(10, R_REGSTRUCT, off);
		}
	} else {
		if (mode == 3 || mode == 4) {
			const int bytes = size == 6 ? (reg == 7 ? 2 : 1) : size == 4 ? 2 : size == 5 ? 8 : size == 2 ? 12 : 4;
			const uae_u32 off = FPX_OFF(regs[8 + reg]);
			LDR_wXi(2, R_REGSTRUCT, off);
			if (mode == 3)
				ADD_wwi(2, 2, bytes);
			else
				SUB_wwi(2, 2, bytes);
			STR_wXi(2, R_REGSTRUCT, off);
		}
		switch (size) {
		case 6:
			STRB_wXx(10, 1, R_MEMSTART);
			break;
		case 4:
			REV16_ww(2, 10);
			STRH_wXx(2, 1, R_MEMSTART);
			break;
		case 5:
			LSR_xxi(3, 10, 32);
			REV_ww(2, 3);
			STR_wXx(2, 1, R_MEMSTART);
			ADD_wwi(3, 1, 4);
			REV_ww(2, 10);
			STR_wXx(2, 3, R_MEMSTART);
			break;
		case 2:
			LSL_wwi(2, 6, 16);
			REV_ww(2, 2);
			STR_wXx(2, 1, R_MEMSTART);
			ADD_wwi(3, 1, 4);
			LSR_xxi(4, 7, 32);
			REV_ww(2, 4);
			STR_wXx(2, 3, R_MEMSTART);
			ADD_wwi(3, 1, 8);
			REV_ww(2, 7);
			STR_wXx(2, 3, R_MEMSTART);
			break;
		default:
			REV_ww(2, 10);
			STR_wXx(2, 1, R_MEMSTART);
			break;
		}
	}
	// the exception byte cleared (CC kept), INEX2 and the accrued INEX
	LDR_wXi(2, R_REGSTRUCT, FPX_OFF(fpsr));
	BFI_wwii(2, 31, 8, 8);
	if (inexact) {
		const uae_u32 inex = fp_jit_inexact_status();
		if (inex == 0x208) {
			LSL_wwi(3, 5, 3);
			ORR_wwwLSLi(3, 3, 5, 9);
		} else {
			fpx_mov32(3, inex);
			CMP_wi(5, 0);
			CSEL_wwwc(3, 3, 31, NATIVE_CC_NE);
		}
		ORR_www(2, 2, 3);
	}
	STR_wXi(2, R_REGSTRUCT, FPX_OFF(fpsr));
	MOV_wi(3, 1);
	STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpu_state));
	STRB_wXi(31, R_REGSTRUCT, FPX_OFF(fp_exception));
	if (fpiar) {
		fpx_mov32(3, pc);
		STR_wXi(3, R_REGSTRUCT, FPX_OFF(fpiar));
	}
}
#endif

/* Compiled FPU arithmetic without native FPU code (the exact core): a call of
 * fpp_jit_arith(opext, value, pc_p, pc). nvals: 0 no operand, 1 v1 (32 bits),
 * 2 v1:v2 (a double). On return 0 the (An)+/-(An) update anreg += delta is
 * applied; on 1 nothing; on 2 the block is left (the interpreter handler took
 * an exception or left special flags). All registers are flushed. */
MIDFUNC(10,jitx_fpp_arith,(IMPTR fn, RR4 v1, RR4 v2, IM8 nvals, IM32 opext, IMPTR pcp, IM32 pc, IM8 anreg, IM32 delta, IM32 inl))
{
	clobber_flags();
	int r1 = -1, r2 = -1;
	if (nvals >= 1)
		r1 = readreg_specific(v1, REG_PAR2);
	if (nvals >= 2)
		r2 = readreg(v2);
	prepare_for_call_1();
	if (nvals >= 2) {
		ORR_xxxLSLi(REG_PAR2, r2, REG_PAR2, 32);    // v1 << 32 | v2 (W registers are zero-extended)
	}
	if (r1 >= 0)
		unlock2(r1);
	if (r2 >= 0)
		unlock2(r2);
	prepare_for_call_2();
	uae_u32 *inline_done = nullptr;
#if defined(JIT) && defined(__SIZEOF_INT128__)
	// inl: the instruction inline, the call only when that does not apply
	fpx_fails fails;
	fails.n = 0;
	if (inl && (((inl >> 8) & 0xf) != 6 || fpx_in_direct())) {   // .X from memory: read directly
		const bool verify = fp_jit_inline_verify();
		if (verify) {                                      // the state before it, for the check
			MOV_xx(REG_PAR1, REG_PAR2);
			compemu_raw_call((uintptr)fp_jit_verify_save);
			MOV_xx(REG_PAR2, REG_RESULT);
		}
		fpx_emit_inline(fails, inl, opext, pc);
		fpx_count(opext, FPJ_ST_INLINE);
		if (verify) {                                      // the helper's result from that state must be the same
			LOAD_U32(REG_PAR1, (uae_u32)opext);
			LOAD_U64(R2_INDEX, pcp);
			LOAD_U32(R3_INDEX, (uae_u32)pc);
			LOAD_U64(R4_INDEX, fn);
			fpx_call4((uintptr)fp_jit_verify_check);
		}
		if (anreg >= 0 && delta) {                         // the (An)+/-(An) update, as on a return of 0
			uintptr idx = (uintptr)&regs.regs[8 + anreg] - (uintptr)&regs;
			LDR_wXi(REG_WORK2, R_REGSTRUCT, idx);
			if (delta > 0)
				ADD_wwi(REG_WORK2, REG_WORK2, delta);
			else
				SUB_wwi(REG_WORK2, REG_WORK2, -delta);
			STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
		}
		inline_done = (uae_u32 *)get_target();
		B_i(0);                                            // to the end
		for (int i = 0; i < fails.n; i++)
			write_jmp_target(fails.at[i], (uintptr)get_target());
	}
	fpx_count(opext, inline_done ? FPJ_ST_INLINE_FAIL : FPJ_ST_HELPER);
#endif
	LOAD_U32(REG_PAR1, (uae_u32)opext);
	LOAD_U64(R2_INDEX, pcp);
	LOAD_U32(R3_INDEX, (uae_u32)pc);
	fpx_call4(fn);

	uae_u32 *branch_notfast = (uae_u32 *)get_target();
	CBNZ_wi(REG_RESULT, 0);
	if (anreg >= 0 && delta) {
		uintptr idx = (uintptr)&regs.regs[8 + anreg] - (uintptr)&regs;
		LDR_wXi(REG_WORK2, R_REGSTRUCT, idx);
		if (delta > 0)
			ADD_wwi(REG_WORK2, REG_WORK2, delta);
		else
			SUB_wwi(REG_WORK2, REG_WORK2, -delta);
		STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
	}
	uae_u32 *branch_join = (uae_u32 *)get_target();
	B_i(0);
	write_jmp_target(branch_notfast, (uintptr)get_target());
	uae_u32 *branch_slow_join = (uae_u32 *)get_target();
	TBZ_wii(REG_RESULT, 1, 0);                  // 1: the interpreter did it, continue
	uae_u32 *branch_leave = (uae_u32 *)get_target();
	B_i(0);                                     // 2: leave the block
	write_jmp_target(branch_leave, (uintptr)popall_do_nothing);
	write_jmp_target(branch_join, (uintptr)get_target());
	write_jmp_target(branch_slow_join, (uintptr)get_target());
	if (inline_done)
		write_jmp_target(inline_done, (uintptr)get_target());
}
MENDFUNC(10,jitx_fpp_arith,(IMPTR fn, RR4 v1, RR4 v2, IM8 nvals, IM32 opext, IMPTR pcp, IM32 pc, IM8 anreg, IM32 delta, IM32 inl))

/* Compiled FPU instruction via fn, fpp_jit_generic(opext, ea, next_p, lenpc)
 * or a helper with its arguments and results (fp_jit_out_helper()):
 * ea from the 68k temp a (if hasea). Continue on 1, leave the block on 2. */
MIDFUNC(7,jitx_fpp_generic,(IMPTR fn, RR4 a, IM8 hasea, IM32 opext, IMPTR nextp, IMPTR lenpc, IM32 inl))
{
	clobber_flags();
	int r = -1;
	if (hasea)
		r = readreg_specific(a, REG_PAR2);
	prepare_for_call_1();
	if (r >= 0)
		unlock2(r);
	prepare_for_call_2();
	uae_u32 *inline_done = nullptr;
#if defined(JIT) && defined(__SIZEOF_INT128__)
	// inl: an FMOVE to <ea> inline, the call only when that does not apply
	fpx_fails fails;
	fails.n = 0;
	struct fpj_fmovem fmovem;
	const bool is_fmovem = inl && fp_jit_fmovem_plan(opext >> 16, (uae_u16)opext, &fmovem);
	const bool is_fmovecr = inl && (opext & 0xfc00) == 0x5c00;
	if (inl && (is_fmovecr || (is_fmovem ? (fmovem.to_mem ? fpx_out_direct(4) : fpx_in_direct())
			: ((opext >> 19) & 7) == 0 || fpx_out_direct(((uae_u16)opext >> 10) & 7)))) {
		const bool verify = fp_jit_inline_verify();
		if (verify) {
			MOV_xx(REG_PAR1, REG_PAR2);
			LOAD_U32(REG_PAR2, (uae_u32)opext);
			fpx_call4((uintptr)fp_jit_verify_out_save);
			MOV_xx(REG_PAR2, REG_RESULT);
		}
		if (is_fmovecr)
			fpx_emit_fmovecr(fails, inl, opext, (uae_u32)lenpc);
		else if (is_fmovem)
			fpx_emit_fmovem(fails, inl, opext, fmovem);
		else
			fpx_emit_out(fails, inl, opext, (uae_u32)lenpc);
		fpx_count(opext, FPJ_ST_INLINE);
		if (verify) {
			LOAD_U32(REG_PAR1, (uae_u32)opext);
			LOAD_U64(R2_INDEX, nextp);
			LOAD_U64(R3_INDEX, lenpc);
			LOAD_U64(R4_INDEX, fn);
			fpx_call4((uintptr)fp_jit_verify_out_check);
		}
		inline_done = (uae_u32 *)get_target();
		B_i(0);
		for (int i = 0; i < fails.n; i++)
			write_jmp_target(fails.at[i], (uintptr)get_target());
	}
	fpx_count(opext, inline_done ? FPJ_ST_INLINE_FAIL : FPJ_ST_HELPER);
#endif
	LOAD_U32(REG_PAR1, (uae_u32)opext);
	LOAD_U64(R2_INDEX, nextp);
	LOAD_U64(R3_INDEX, lenpc);
	fpx_call4(fn);
	uae_u32 *branch_cont = (uae_u32 *)get_target();
	TBZ_wii(REG_RESULT, 1, 0);                  // 1: continue
	uae_u32 *branch_leave = (uae_u32 *)get_target();
	B_i(0);                                     // 2: leave the block
	write_jmp_target(branch_leave, (uintptr)popall_do_nothing);
	write_jmp_target(branch_cont, (uintptr)get_target());
	if (inline_done)
		write_jmp_target(inline_done, (uintptr)get_target());
}
MENDFUNC(7,jitx_fpp_generic,(IMPTR fn, RR4 a, IM8 hasea, IM32 opext, IMPTR nextp, IMPTR lenpc, IM32 inl))

/* The exact core: the FPSR CC are always valid. d low byte = $FF/$00 from the
 * predicate's truth mask over the CC nibble (fpp_get_cond_mask()); with bsun
 * (IEEE-nonaware predicate) and NAN set, BSUN and the accrued IOP are set
 * (no BSUN trap in compiled code). */
MIDFUNC(3,fpx_cc_from_fpsr,(RW4 d, IM32 mask, IM8 bsun))
{
	clobber_flags();                            // save the 68k CCR: FBcc's TST follows
	d = rmw(d);
	LOAD_U64(REG_WORK4, (uintptr)&regs.fpsr);
	LDR_wXi(REG_WORK1, REG_WORK4, 0);
	if (bsun) {
		uae_u32 *branch_nonan = (uae_u32 *)get_target();
		TBZ_wii(REG_WORK1, 24, 0);              // CC NAN clear
		MOV_wi(REG_WORK3, 0x8080);              // BSUN, accrued IOP
		ORR_www(REG_WORK1, REG_WORK1, REG_WORK3);
		STR_wXi(REG_WORK1, REG_WORK4, 0);
		write_jmp_target(branch_nonan, (uintptr)get_target());
	}
	UBFX_wwii(REG_WORK1, REG_WORK1, 24, 4);     // CC nibble N Z I NAN
	LOAD_U32(REG_WORK2, mask);
	LSR_www(REG_WORK2, REG_WORK2, REG_WORK1);
	SBFX_wwii(REG_WORK2, REG_WORK2, 0, 1);      // 0 or -1
	BFI_wwii(d, REG_WORK2, 0, 8);
	unlock2(d);
}
MENDFUNC(3,fpx_cc_from_fpsr,(RW4 d, IM32 mask, IM8 bsun))

MIDFUNC(2,fmov_to_l_rr,(W4 d, FR s))
{
	s = f_readreg(s);
	d = writereg(d);
	raw_fmov_to_l_rr(d, s);
	emit_fpsr_int_conv(s, 4);
	unlock2(d);
	f_unlock(s);
}
MENDFUNC(2,fmov_to_l_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_s_rr,(W4 d, FR s))
{
	s = f_readreg(s);
	d = writereg(d);
	raw_fmov_to_s_rr(d, s);
	unlock2(d);
	f_unlock(s);
}
MENDFUNC(2,fmov_to_s_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_w_rr,(W4 d, FR s))
{
	s = f_readreg(s);
	INIT_WREG_w(d);

	raw_fmov_to_w_rr(d, s, targetIsReg);
	emit_fpsr_int_conv(s, 2);
	unlock2(d);
	f_unlock(s);
}
MENDFUNC(2,fmov_to_w_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_b_rr,(W4 d, FR s))
{
	s = f_readreg(s);
	INIT_WREG_b(d);

	raw_fmov_to_b_rr(d, s, targetIsReg);
	emit_fpsr_int_conv(s, 1);
	unlock2(d);
	f_unlock(s);
}
MENDFUNC(2,fmov_to_b_rr,(W4 d, FR s))

MIDFUNC(1,fmov_d_ri_0,(FW r))
{
	r = f_writereg(r);
	raw_fmov_d_ri_0(r);
	f_unlock(r);
}
MENDFUNC(1,fmov_d_ri_0,(FW r))

MIDFUNC(1,fmov_d_ri_1,(FW r))
{
	r = f_writereg(r);
	raw_fmov_d_ri_1(r);
	f_unlock(r);
}
MENDFUNC(1,fmov_d_ri_1,(FW r))

MIDFUNC(1,fmov_d_ri_10,(FW r))
{
	r = f_writereg(r);
	raw_fmov_d_ri_10(r);
	f_unlock(r);
}
MENDFUNC(1,fmov_d_ri_10,(FW r))

MIDFUNC(1,fmov_d_ri_100,(FW r))
{
	r = f_writereg(r);
	raw_fmov_d_ri_100(r);
	f_unlock(r);
}
MENDFUNC(1,fmov_d_ri_100,(FW r))

MIDFUNC(2,fmov_d_rm,(FW r, MEMR m))
{
	r = f_writereg(r);
	raw_fmov_d_rm(r, m);
	f_unlock(r);
}
MENDFUNC(2,fmov_d_rm,(FW r, MEMR m))

MIDFUNC(2,fmovs_rm,(FW r, MEMR m))
{
	r = f_writereg(r);
	raw_fmovs_rm(r, m);
	f_unlock(r);
}
MENDFUNC(2,fmovs_rm,(FW r, MEMR m))

MIDFUNC(2,fmov_rm,(FW r, MEMR m))
{
	r = f_writereg(r);
	raw_fmov_d_rm(r, m);
	f_unlock(r);
}
MENDFUNC(2,fmov_rm,(FW r, MEMR m))

MIDFUNC(3,fmov_to_d_rrr,(W4 d1, W4 d2, FR s))
{
	s = f_readreg(s);
	d1 = writereg(d1);
	d2 = writereg(d2);
	raw_fmov_to_d_rrr(d1, d2, s);
	unlock2(d2);
	unlock2(d1);
	f_unlock(s);
}
MENDFUNC(3,fmov_to_d_rrr,(W4 d1, W4 d2, FR s))

MIDFUNC(2,fsqrt_rr,(FW d, FR s))
{
	s = f_readreg(s);
	d = f_writereg(d);
	raw_fsqrt_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fsqrt_rr,(FW d, FR s))

MIDFUNC(2,fabs_rr,(FW d, FR s))
{
	s = f_readreg(s);
	d = f_writereg(d);
	raw_fabs_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fabs_rr,(FW d, FR s))

MIDFUNC(2,fneg_rr,(FW d, FR s))
{
	s = f_readreg(s);
	d = f_writereg(d);
	raw_fneg_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fneg_rr,(FW d, FR s))

MIDFUNC(2,fdiv_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fdiv_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fdiv_rr,(FRW d, FR s))

MIDFUNC(2,fadd_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fadd_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fadd_rr,(FRW d, FR s))

MIDFUNC(2,fmul_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fmul_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fmul_rr,(FRW d, FR s))

MIDFUNC(2,fsub_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fsub_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fsub_rr,(FRW d, FR s))
MIDFUNC(3,fcmp_result_rrr,(FW r, FR d, FR s))
{
	clobber_flags();
	d = f_readreg(d);
	s = f_readreg(s);
	r = f_writereg(r);
	raw_fcmp_result_rrr(r, d, s);
	f_unlock(r);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(3,fcmp_result_rrr,(FW r, FR d, FR s))


MIDFUNC(2,frndint_rr,(FW d, FR s))
{
	s = f_readreg(s);
	d = f_writereg(d);
	raw_frndint_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,frndint_rr,(FW d, FR s))

MIDFUNC(2,frndintz_rr,(FW d, FR s))
{
	s = f_readreg(s);
	d = f_writereg(d);
	raw_frndintz_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,frndintz_rr,(FW d, FR s))

MIDFUNC(2,fmod_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fmod_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fmod_rr,(FRW d, FR s))

MIDFUNC(2,fsgldiv_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fsgldiv_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fsgldiv_rr,(FRW d, FR s))

MIDFUNC(1,fcuts_r,(FRW r))
{
	r = f_rmw(r);
	raw_fcuts_r(r);
	f_unlock(r);
}
MENDFUNC(1,fcuts_r,(FRW r))

MIDFUNC(2,frem1_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_frem1_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,frem1_rr,(FRW d, FR s))

MIDFUNC(2,fsglmul_rr,(FRW d, FR s))
{
	s = f_readreg(s);
	d = f_rmw(d);
	raw_fsglmul_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fsglmul_rr,(FRW d, FR s))

MIDFUNC(2,fmovs_rr,(FW d, FR s))
{
	s = f_readreg(s);
	d = f_writereg(d);
	raw_fmovs_rr(d, s);
	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(2,fmovs_rr,(FW d, FR s))

MIDFUNC(3,ffunc_rr,(double (*func)(double), FW d, FR s))
{
	clobber_flags();

	s = f_readreg(s);
	int reald = f_writereg(d);

	prepare_for_call_1();

	f_unlock(s);
	f_unlock(reald);

	prepare_for_call_2();

	raw_ffunc_rr(func, reald, s);

	live.fat[reald].holds = d;
	live.fat[reald].nholds = 1;

	live.fate[d].realreg = reald;
	live.fate[d].status = DIRTY;
}
MENDFUNC(3,ffunc_rr,(double (*func)(double), FW d, FR s))

MIDFUNC(3,fpowx_rr,(uae_u32 x, FW d, FR s))
{
	clobber_flags();

	s = f_readreg(s);
	int reald = f_writereg(d);

	prepare_for_call_1();

	f_unlock(s);
	f_unlock(reald);

	prepare_for_call_2();

	raw_fpowx_rr(x, reald, s);

	live.fat[reald].holds = d;
	live.fat[reald].nholds = 1;

	live.fate[d].realreg = reald;
	live.fate[d].status = DIRTY;
}
MENDFUNC(3,fpowx_rr,(uae_u32 x, FW d, FR s))

MIDFUNC(1,fflags_into_flags,())
{
	clobber_flags();
	fflags_into_flags_internal();
}
MENDFUNC(1,fflags_into_flags,())

/* FScc/FBcc: the FPU condition codes in the host flags for one test, the
 * integer CCR kept. fflags_into_flags() + live_flags() made them the 68k
 * CCR, so FScc and FBcc changed the guest's CCR. Save the CCR
 * (clobber_flags), load the FPU flags, and leave the host flags TRASH:
 * the CCR is reloaded from its saved copy when next needed, and the
 * block end of an FBcc (flush, then the conditional jump on these flags)
 * no longer stores the FPU flags as the CCR. */
MIDFUNC(0,fflags_into_flags_keep_ccr,(void))
{
	clobber_flags();
	int r = f_readreg(FP_RESULT);
	raw_fflags_into_flags(r);
	f_unlock(r);
	live.flags_in_flags = TRASH;
}
MENDFUNC(0,fflags_into_flags_keep_ccr,(void))


MIDFUNC(2,fp_from_exten_mr,(RR4 adr, FR s))
{
	clobber_flags();

	adr = readreg(adr);
	s = f_readreg(s);
	raw_fp_from_exten_mr(adr, s);
	f_unlock(s);
	unlock2(adr);
}
MENDFUNC(2,fp_from_exten_mr,(RR4 adr, FR s))

MIDFUNC(2,fp_to_exten_rm,(FW d, RR4 adr))
{
	clobber_flags();

	adr = readreg(adr);
	d = f_writereg(d);
	raw_fp_to_exten_rm(d, adr);
	unlock2(adr);
	f_unlock(d);
}
MENDFUNC(2,fp_to_exten_rm,(FW d, RR4 adr))

MIDFUNC(2,fp_from_double_mr,(RR4 adr, FR s))
{
	adr = readreg(adr);
	s = f_readreg(s);
	raw_fp_from_double_mr(adr, s);
	f_unlock(s);
	unlock2(adr);
}
MENDFUNC(2,fp_from_double_mr,(RR4 adr, FR s))

MIDFUNC(2,fp_to_double_rm,(FW d, RR4 adr))
{
	adr = readreg(adr);
	d = f_writereg(d);
	raw_fp_to_double_rm(d, adr);
	unlock2(adr);
	f_unlock(d);
}
MENDFUNC(2,fp_to_double_rm,(FW d, RR4 adr))

/* FPSR condition codes written directly (FMOVE to FPSR, FRESTORE:
 * fpp_set_fpsr() sets fpu_fpsr_cc_written) can combine N, Z, I and NAN in
 * ways no FP_RESULT value holds, e.g. Z with NAN. While the flag is set the
 * predicate comes from the FPSR CC nibble instead: bit n of mask is the
 * predicate for nibble n (fpp_get_cond_mask()). Low byte of d = $FF/$00.
 * No host flags are changed. */
static void emit_cc_from_fpsr(int d, uae_u32 mask)
{
	LOAD_U64(REG_WORK1, (uintptr)&fpu_fpsr_cc_written);
	LDRB_wXi(REG_WORK1, REG_WORK1, 0);
	uae_u32 *branchadd = (uae_u32 *)get_target();
	CBZ_wi(REG_WORK1, 0);                       // FP_RESULT is valid: keep d
	LOAD_U64(REG_WORK1, (uintptr)&regs.fpsr);
	LDR_wXi(REG_WORK1, REG_WORK1, 0);
	UBFX_wwii(REG_WORK1, REG_WORK1, 24, 4);     // CC nibble N Z I NAN
	LOAD_U32(REG_WORK2, mask);
	LSR_www(REG_WORK2, REG_WORK2, REG_WORK1);
	SBFX_wwii(REG_WORK2, REG_WORK2, 0, 1);      // 0 or -1
	BFI_wwii(d, REG_WORK2, 0, 8);
	write_jmp_target(branchadd, (uintptr)get_target());
}

MIDFUNC(2,fp_cc_from_fpsr,(RW4 d, IM32 mask))
{
	d = rmw(d);
	emit_cc_from_fpsr(d, mask);
	unlock2(d);
}
MENDFUNC(2,fp_cc_from_fpsr,(RW4 d, IM32 mask))

/* FBcc: the host flags for the branch from the predicate byte in d (NE =
 * taken). Not live_flags(): the 68k CCR stays in its saved copy, and the
 * block end (flush, then the conditional jump) keeps these flags. */
MIDFUNC(1,fp_fbcc_test,(RR4 d))
{
	d = readreg(d);
	TST_ww(d, d);
	unlock2(d);
	live.flags_in_flags = TRASH;
}
MENDFUNC(1,fp_fbcc_test,(RR4 d))

/* A compiled FPU operation set FP_RESULT: the FPSR CC come from it again. */
MIDFUNC(0,fpu_cc_written_clear,(void))
{
	LOAD_U64(REG_WORK1, (uintptr)&fpu_fpsr_cc_written);
	MOV_wi(REG_WORK2, 0);
	STRB_wXi(REG_WORK2, REG_WORK1, 0);
}
MENDFUNC(0,fpu_cc_written_clear,(void))

MIDFUNC(2,fp_fscc_ri,(RW4 d, int cc))
{
	d = rmw(d);
	raw_fp_fscc_ri(d, cc);
	// Fix 20: raw_fp_fscc_ri uses CLEAR_LOW8_xx, SET_LOW8_xx, and BFXIL_xxii
	// which are all 64-bit ops that preserve dirty upper 32 bits.
	MOV_ww(d, d);
	unlock2(d);
}
MENDFUNC(2,fp_fscc_ri,(RW4 d, int cc))


#endif // USE_JIT_FPU
