/*
 * compiler/compemu_midfunc_arm.cpp - Native MIDFUNCS for AARCH64 (JIT v2)
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

const uae_u32 ARM_CCR_MAP[] = { 0, ARM_C_FLAG, // 1 C
	ARM_V_FLAG, // 2 V
	ARM_C_FLAG | ARM_V_FLAG, // 3 VC
	ARM_Z_FLAG, // 4 Z
	ARM_Z_FLAG | ARM_C_FLAG, // 5 ZC
	ARM_Z_FLAG | ARM_V_FLAG, // 6 ZV
	ARM_Z_FLAG | ARM_C_FLAG | ARM_V_FLAG, // 7 ZVC
	ARM_N_FLAG, // 8 N
	ARM_N_FLAG | ARM_C_FLAG, // 9 NC
	ARM_N_FLAG | ARM_V_FLAG, // 10 NV
	ARM_N_FLAG | ARM_C_FLAG | ARM_V_FLAG, // 11 NVC
	ARM_N_FLAG | ARM_Z_FLAG, // 12 NZ
	ARM_N_FLAG | ARM_Z_FLAG | ARM_C_FLAG, // 13 NZC
	ARM_N_FLAG | ARM_Z_FLAG | ARM_V_FLAG, // 14 NZV
	ARM_N_FLAG | ARM_Z_FLAG | ARM_C_FLAG | ARM_V_FLAG, // 15 NZVC
};


#define DUPLICACTE_CARRY            \
  if (needed_flags & FLAG_X) {      \
    int x = writereg(FLAGX);     		\
    if (flags_carry_inverted)       \
      CSET_xc(x, NATIVE_CC_CC);     \
    else                            \
      CSET_xc(x, NATIVE_CC_CS);     \
    unlock2(x);                     \
  }

/*
 * ADD
 * Operand Syntax: 	<ea>, Dn
 * 					Dn, <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set the same as the carry bit.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if an overflow is generated. Cleared otherwise.
 * C Set if a carry is generated. Cleared otherwise.
 *
 */
MIDFUNC(3,jnf_ADD_im8,(W4 d, RR4 s, IM8 v))
{
	int s_is_d = (s == d);
	if(s_is_d) {
		s = d = rmw(d);
	} else {
		s = readreg(s);
		d = writereg(d);
	}
	
	ADD_wwi(d, s, v & 0xff);

	EXIT_REGS(d, s);
}
MENDFUNC(3,jnf_ADD_im8,(W4 d, RR4 s, IM8 v))

MIDFUNC(2,jnf_ADD_b_imm,(RW1 d, IM8 v))
{
	if (isconst(d)) {
	  live.state[d].val = (live.state[d].val & 0xffffff00) | ((live.state[d].val + v) & 0x000000ff);
    return;
  }

	INIT_REG_b(d);
	
	if(targetIsReg) {
	  ADD_wwi(REG_WORK1, d, v & 0xff);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
	  ADD_wwi(d, d, v & 0xff);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_ADD_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jnf_ADD_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADD_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);
	
	if(targetIsReg) {
		ADD_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		ADD_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADD_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_ADD_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

	if(targetIsReg) {
	  if(v >= 0 && v <= 0xfff) {
	    ADD_wwi(REG_WORK1, d, v);
	  } else {
		  MOV_xi(REG_WORK1, v & 0xffff);
		  ADD_www(REG_WORK1, d, REG_WORK1);
	  }
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else{
	  if(v >= 0 && v <= 0xfff) {
	    ADD_wwi(d, d, v);
	  } else {
		  MOV_xi(REG_WORK1, v & 0xffff);
		  ADD_www(d, d, REG_WORK1);
	  }
	}
	
	unlock2(d);
}
MENDFUNC(2,jnf_ADD_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jnf_ADD_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADD_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		ADD_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		ADD_www(d, d, s);
	}
	
	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADD_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_ADD_l_imm,(RW4 d, IM32 v))
{
	if (isconst(d)) {
		live.state[d].val = live.state[d].val + v;
		return;
	}

	d = rmw(d);

  if(v >= 0 && v <= 0xfff) {
    ADD_wwi(d, d, v);
  } else {
    // never reached...
    LOAD_U32(REG_WORK1, v);
	  ADD_www(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_ADD_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jnf_ADD_l,(RW4 d, RR4 s))
{
	if (isconst(d) && isconst(s)) {
		COMPCALL(jnf_ADD_l_imm)(d, live.state[s].val);
		return;
	}
	if (isconst(s) && (uae_s32)live.state[s].val >= 0 && (uae_s32)live.state[s].val <= 0xfff) {
		COMPCALL(jnf_ADD_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ADD_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADD_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_ADD_b_imm,(RW1 d, IM8 v))
{
	INIT_REG_b(d);
  
	MOV_xish(REG_WORK2, (v & 0xff) << 8, 16);
  ADDS_wwwLSLi(REG_WORK1, REG_WORK2, d, 24);
  BFXIL_xxii(d, REG_WORK1, 24, 8);
		
  flags_carry_inverted = false;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_ADD_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jff_ADD_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_ADD_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	LSL_wwi(REG_WORK2, s, 24);
  ADDS_wwwLSLi(REG_WORK1, REG_WORK2, d, 24);
  BFXIL_xxii(d, REG_WORK1, 24, 8);
	
  flags_carry_inverted = false;
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADD_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_ADD_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

	MOV_xish(REG_WORK1, v & 0xffff, 16);
  ADDS_wwwLSLi(REG_WORK1, REG_WORK1, d, 16);
  BFXIL_xxii(d, REG_WORK1, 16, 16);
	
  flags_carry_inverted = false;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_ADD_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jff_ADD_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_ADD_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	LSL_wwi(REG_WORK1, s, 16);
  ADDS_wwwLSLi(REG_WORK1, REG_WORK1, d, 16);
  BFXIL_xxii(d, REG_WORK1, 16, 16);

  flags_carry_inverted = false;
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADD_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_ADD_l_imm,(RW4 d, IM32 v))
{
	d = rmw(d);

  if(v >= 0 && v <= 0xfff) {
    ADDS_wwi(d, d, v);
  } else {
    // never reached...
    LOAD_U32(REG_WORK2, v);
	  ADDS_www(d, d, REG_WORK2);
  }

  flags_carry_inverted = false;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_ADD_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jff_ADD_l,(RW4 d, RR4 s))
{
	if (isconst(s) && (uae_s32)live.state[s].val >= 0 && (uae_s32)live.state[s].val <= 0xfff) {
		COMPCALL(jff_ADD_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ADDS_www(d, d, s);

  flags_carry_inverted = false;
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADD_l,(RW4 d, RR4 s))

/*
 * ADDA
 * Operand Syntax: 	<ea>, An
 *
 * Operand Size: 16,32
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(2,jnf_ADDA_w_imm,(RW4 d, IM16 v))
{
	if (isconst(d)) {
		live.state[d].val = live.state[d].val + (uae_s32)(uae_s16)v;
		return;
	}

  uae_s16 tmp = (uae_s16)v;
	d = rmw(d);
	if(tmp >= 0 && tmp <= 0xfff) {
	  ADD_wwi(d, d, tmp);
  } else if (tmp >= -0xfff && tmp < 0) {
	  SUB_wwi(d, d, -tmp);
	} else {
	  SIGNED16_IMM_2_REG(REG_WORK1, tmp);
	  ADD_www(d, d, REG_WORK1);
	}
	unlock2(d);
}
MENDFUNC(2,jnf_ADDA_w_imm,(RW4 d, IM16 v))

MIDFUNC(2,jnf_ADDA_w,(RW4 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADDA_w_imm)(d, live.state[s].val & 0xffff);
		return;
	}

	INIT_REGS_w(d, s);

	ADD_wwwEX(d, d, s, EX_SXTH);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADDA_w,(RW4 d, RR2 s))

MIDFUNC(2,jnf_ADDA_l_imm,(RW4 d, IM32 v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val + v);
		return;
	}

	d = rmw(d);

	if(v >= 0 && v <= 0xfff) {
	  ADD_wwi(d, d, v);
  } else if (v >= -0xfff && v < 0) {
	  SUB_wwi(d, d, -v);
	} else {
    LOAD_U32(REG_WORK1, v);
    ADD_www(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_ADDA_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jnf_ADDA_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADDA_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ADD_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADDA_l,(RW4 d, RR4 s))

/*
 * ADDX
 * Operand Syntax: 	Dy, Dx
 * 					-(Ay), -(Ax)
 *
 * Operand Size: 8,16,32
 *
 * X Set the same as the carry bit.
 * N Set if the result is negative. Cleared otherwise.
 * Z Cleared if the result is nonzero; unchanged otherwise.
 * V Set if an overflow is generated. Cleared otherwise.
 * C Set if a carry is generated. Cleared otherwise.
 *
 * Attention: Z is cleared only if the result is nonzero. Unchanged otherwise
 *
 */
MIDFUNC(2,jnf_ADDX_b,(RW1 d, RR1 s))
{
  int x = readreg(FLAGX);

	INIT_REGS_b(d, s);

	if(s_is_d) {
		ADD_wwwLSLi(REG_WORK1, x, d, 1);
	} else {
		ADD_www(REG_WORK1, d, s);
	  ADD_www(REG_WORK1, REG_WORK1, x);
	}
  BFI_xxii(d, REG_WORK1, 0, 8);

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_ADDX_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_ADDX_w,(RW2 d, RR2 s))
{
  int x = readreg(FLAGX);

	INIT_REGS_w(d, s);

	if(s_is_d) {
		ADD_wwwLSLi(REG_WORK1, x, d, 1);
	} else {
		ADD_www(REG_WORK1, d, s);
	  ADD_www(REG_WORK1, REG_WORK1, x);
	}
  BFI_xxii(d, REG_WORK1, 0, 16);

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_ADDX_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_ADDX_l,(RW4 d, RR4 s))
{
  int x = readreg(FLAGX);

  if(s != d && isconst(s) && live.state[s].val >= 0 && live.state[s].val <= 0xfff) {
    d = rmw(d);
    ADD_wwi(d, d, live.state[s].val);
	  ADD_www(d, d, x);
    unlock2(d);
    unlock2(x);
    return;
  }

	INIT_REGS_l(d, s);

	if(s_is_d) {
	  ADD_wwwLSLi(d, x, d, 1);
	} else {
	  ADD_www(d, d, s);
	  ADD_www(d, d, x);
	}

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_ADDX_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_ADDX_b,(RW1 d, RR1 s))
{
	INIT_REGS_b(d, s);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK1, 0);
	MOVN_xish(REG_WORK2, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK1, NATIVE_CC_NE);

	// Restore X to carry (don't care about other flags)
	SUBS_wwi(REG_WORK3, x, 1);

	BFI_xxii(REG_WORK1, s, 24, 8);
	LSL_wwi(REG_WORK3, d, 24);
  ADCS_www(REG_WORK1, REG_WORK1, REG_WORK3);
  BFXIL_xxii(d, REG_WORK1, 24, 8);

	MRS_NZCV_x(REG_WORK1);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  if (needed_flags & FLAG_X)
  	UBFX_xxii(x, REG_WORK1, 29, 1); // Duplicate carry
	MSR_NZCV_x(REG_WORK1);

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADDX_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_ADDX_w,(RW2 d, RR2 s))
{
	INIT_REGS_w(d, s);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK1, 0);
	MOVN_xish(REG_WORK2, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK1, NATIVE_CC_NE);

	// Restore X to carry (don't care about other flags)
	SUBS_wwi(REG_WORK3, x, 1);

	BFI_xxii(REG_WORK1, s, 16, 16);
	LSL_wwi(REG_WORK3, d, 16);
  ADCS_www(REG_WORK1, REG_WORK1, REG_WORK3);
  BFXIL_xxii(d, REG_WORK1, 16, 16);

	MRS_NZCV_x(REG_WORK1);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	if (needed_flags & FLAG_X)
  	UBFX_xxii(x, REG_WORK1, 29, 1); // Duplicate carry
	MSR_NZCV_x(REG_WORK1);

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADDX_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_ADDX_l,(W4 d, RR4 s))
{
	INIT_REGS_l(d, s);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK1, 0);
	MOVN_xish(REG_WORK2, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK2, REG_WORK1, NATIVE_CC_NE);

	// Restore X to carry (don't care about other flags)
	SUBS_wwi(REG_WORK3, x, 1);

	ADCS_www(d, d, s);

	MRS_NZCV_x(REG_WORK1);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	if (needed_flags & FLAG_X)
  	UBFX_xxii(x, REG_WORK1, 29, 1); // Duplicate carry
	MSR_NZCV_x(REG_WORK1);

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADDX_l,(W4 d, RR4 s))

/*
 * ANDSR
 * Operand Syntax: 	#<data>, CCR
 *
 * Operand Size: 8
 *
 * X Cleared if bit 4 of immediate operand is zero. Unchanged otherwise.
 * N Cleared if bit 3 of immediate operand is zero. Unchanged otherwise.
 * Z Cleared if bit 2 of immediate operand is zero. Unchanged otherwise.
 * V Cleared if bit 1 of immediate operand is zero. Unchanged otherwise.
 * C Cleared if bit 0 of immediate operand is zero. Unchanged otherwise.
 *
 */
MIDFUNC(2,jff_ANDSR,(IM32 s, IM8 x))
{
	MRS_NZCV_x(REG_WORK1);
  if(flags_carry_inverted) {
    EOR_xxCflag(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
  }
	MOV_xish(REG_WORK2, (s >> 16), 16);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	MSR_NZCV_x(REG_WORK1);

	if (!x) {
		int f = writereg(FLAGX);
		MOV_xi(f, 0);
		unlock2(f);
	}
}
MENDFUNC(2,jff_ANDSR,(IM32 s, IM8 x))

/*
 * AND
 * Operand Syntax: 	<ea>, Dn
 * 					Dn, <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_AND_b_imm,(RW1 d, IM8 v))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffffff00) | ((live.state[d].val & v) & 0x000000ff);
		return;
	}

	INIT_REG_b(d);

	MOVN_xi(REG_WORK1, (~v) & 0xff);
	AND_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_AND_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jnf_AND_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_AND_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		AND_www(REG_WORK1, d, s);
		BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		AND_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_AND_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_AND_w_imm,(RW2 d, IM16 v))
{
	if (isconst(d)) {
    live.state[d].val = (live.state[d].val & 0xffff0000) | ((live.state[d].val & v) & 0x0000ffff);
    return;
  }

	INIT_REG_w(d);

  MOVN_xi(REG_WORK1, (~v));
  AND_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_AND_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jnf_AND_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_AND_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		AND_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		AND_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_AND_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_AND_l_imm,(RW4 d, IM32 v))
{
  if(isconst(d)) {
    live.state[d].val = live.state[d].val & v;
    return;
  }
  
	d = rmw(d);

  LOAD_U32(REG_WORK1, v);
	AND_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_AND_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jnf_AND_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_AND_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	AND_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_AND_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_AND_b_imm,(RW1 d, IM8 v))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_IMM_2_REG(REG_WORK2, v);
	if(targetIsReg) {
		ANDS_www(REG_WORK1, REG_WORK1, REG_WORK2);
		BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		ANDS_www(d, REG_WORK1, REG_WORK2);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_AND_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jff_AND_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_AND_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_REG_2_REG(REG_WORK2, s);
	if(targetIsReg) {
		ANDS_www(REG_WORK1, REG_WORK1, REG_WORK2);
		BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		ANDS_www(d, REG_WORK1, REG_WORK2);
	}

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_AND_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_AND_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_IMM_2_REG(REG_WORK2, v);
	if(targetIsReg) {
		ANDS_www(REG_WORK1, REG_WORK1, REG_WORK2);
		BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		ANDS_www(d, REG_WORK1, REG_WORK2);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_AND_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jff_AND_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_AND_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_REG_2_REG(REG_WORK2, s);
	if(targetIsReg) {
		ANDS_www(REG_WORK1, REG_WORK1, REG_WORK2);
		BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		ANDS_www(d, REG_WORK1, REG_WORK2);
	}

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_AND_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_AND_l_imm,(RW4 d, IM32 v))
{
	d = rmw(d);

  LOAD_U32(REG_WORK1, v);
	ANDS_www(d, d, REG_WORK1);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_AND_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jff_AND_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_AND_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ANDS_www(d, d, s);

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_AND_l,(RW4 d, RR4 s))

/*
 * ASL
 * Operand Syntax: 	Dx, Dy
 * 					#<data>, Dy
 *					<ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set according to the last bit shifted out of the operand. Unaffected for a shift count of zero.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if the most significant bit is changed at any time during the shift operation. Cleared otherwise.
 * C Set according to the last bit shifted out of the operand. Cleared for a shift count of zero.
 *
 * imm version only called with 1 <= i <= 8
 *
 */
MIDFUNC(2,jff_ASL_b_imm,(RW1 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);
		
	LSL_wwi(REG_WORK3, d, 24);
	if (i) {
    LSL_xxi(REG_WORK2, REG_WORK3, i);
    BFXIL_xxii(d, REG_WORK2, 24, 8);  // result is ready
    TST_ww(REG_WORK2, REG_WORK2);     // NZ correct, VC cleared
    
    if (needed_flags & FLAG_V) {
		  // Calculate C Flag
      MRS_NZCV_x(REG_WORK4);
      TBZ_xii(REG_WORK2, 32, 2);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      
		  // Calculate V Flag
      CLS_ww(REG_WORK1, REG_WORK3);
      CMP_wi(REG_WORK1, i);
      BGE_i(2);
      SET_xxVflag(REG_WORK4, REG_WORK4);

      MSR_NZCV_x(REG_WORK4);
    } else {
  		// Calculate C Flag
      TBZ_xii(REG_WORK2, 32, 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }    
    
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		TST_ww(REG_WORK3, REG_WORK3);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASL_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jff_ASL_w_imm,(RW2 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);
		
	LSL_wwi(REG_WORK3, d, 16);
	if (i) {
    LSL_xxi(REG_WORK2, REG_WORK3, i);
    BFXIL_xxii(d, REG_WORK2, 16, 16); // result is ready
    TST_ww(REG_WORK2, REG_WORK2);     // NZ correct, VC cleared
    
    if (needed_flags & FLAG_V) {
		  // Calculate C Flag
      MRS_NZCV_x(REG_WORK4);
      TBZ_xii(REG_WORK2, 32, 2);
      SET_xxCflag(REG_WORK4, REG_WORK4);
    
		  // Calculate V Flag
      CLS_ww(REG_WORK1, REG_WORK3);
      CMP_wi(REG_WORK1, i);
      BGE_i(2);
      SET_xxVflag(REG_WORK4, REG_WORK4);

      MSR_NZCV_x(REG_WORK4);
    } else {
  		// Calculate C Flag
      TBZ_xii(REG_WORK2, 32, 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }    
    
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		TST_ww(REG_WORK3, REG_WORK3);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASL_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jff_ASL_l_imm,(RW4 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	if (i) {
    if (needed_flags & FLAG_V)
  	  MOV_ww(REG_WORK3, d);
    LSL_xxi(d, d, i);
    TST_ww(d, d);               // NZ correct, VC cleared
    
    if (needed_flags & FLAG_V) {
		  // Calculate C Flag
      MRS_NZCV_x(REG_WORK4);
      TBZ_xii(d, 32, 2);
      SET_xxCflag(REG_WORK4, REG_WORK4);
    
		  // Calculate V Flag
      CLS_ww(REG_WORK1, REG_WORK3);
      CMP_wi(REG_WORK1, i);
      BGE_i(2);
      SET_xxVflag(REG_WORK4, REG_WORK4);

      MSR_NZCV_x(REG_WORK4);
    } else {
  		// Calculate C Flag
      TBZ_xii(d, 32, 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }    
    
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		TST_ww(d, d);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASL_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jff_ASL_b_reg,(RW1 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);
  
	LSL_wwi(REG_WORK3, d, 24);
	ANDS_ww3f(REG_WORK1, i);
  BNE_i(3);
  
  // shift count is 0
  TST_ww(REG_WORK3, REG_WORK3);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
  // shift count > 0
  LSL_xxx(REG_WORK2, REG_WORK3, REG_WORK1);
  BFXIL_xxii(d, REG_WORK2, 24, 8);  // result is ready
  TST_ww(REG_WORK2, REG_WORK2);     // NZ correct, VC cleared
	
  if (needed_flags & FLAG_V) {
	  // Calculate C Flag
    MRS_NZCV_x(REG_WORK4);
    TBZ_xii(REG_WORK2, 32, 2);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	
	  // Calculate V Flag
	  CLS_ww(REG_WORK2, REG_WORK3);
	  CMP_ww(REG_WORK2, REG_WORK1);
	  BGE_i(2);
    SET_xxVflag(REG_WORK4, REG_WORK4);

    MSR_NZCV_x(REG_WORK4);
  } else {
  	// Calculate C Flag
    TBZ_xii(REG_WORK2, 32, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY
  
  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());
  
  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASL_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_ASL_w_reg,(RW2 d, RR4 i))
{
  if(isconst(i)) {
	  COMPCALL(jff_ASL_w_imm)(d, live.state[i].val & 0x3f);
	  return;
  }
  
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	LSL_wwi(REG_WORK3, d, 16);
	ANDS_ww3f(REG_WORK1, i);
  BNE_i(3);
  
  // shift count is 0
  TST_ww(REG_WORK3, REG_WORK3);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
  // shift count > 0
  LSL_xxx(REG_WORK2, REG_WORK3, REG_WORK1);
  BFXIL_xxii(d, REG_WORK2, 16, 16); // result is ready
  TST_ww(REG_WORK2, REG_WORK2);     // NZ correct, VC cleared
	
  if (needed_flags & FLAG_V) {
	  // Calculate C Flag
    MRS_NZCV_x(REG_WORK4);
    TBZ_xii(REG_WORK2, 32, 2);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	
	  // Calculate V Flag
	  CLS_ww(REG_WORK2, REG_WORK3);
	  CMP_ww(REG_WORK2, REG_WORK1);
	  BGE_i(2);
    SET_xxVflag(REG_WORK4, REG_WORK4);

    MSR_NZCV_x(REG_WORK4);
  } else {
  	// Calculate C Flag
    TBZ_xii(REG_WORK2, 32, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY
  
  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASL_w_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_ASL_l_reg,(RW4 d, RR4 i))
{
  if(isconst(i)) {
	  COMPCALL(jff_ASL_l_imm)(d, live.state[i].val & 0x3f);
	  return;
  }
  
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	ANDS_ww3f(REG_WORK1, i);
  BNE_i(3);
  
  // shift count is 0
  TST_ww(d, d);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
  // shift count > 0
  if (needed_flags & FLAG_V)
	  MOV_ww(REG_WORK3, d);
  LSL_xxx(d, d, REG_WORK1);
  TST_ww(d, d);                     // NZ correct, VC cleared
	
  if (needed_flags & FLAG_V) {
	  // Calculate C Flag
    MRS_NZCV_x(REG_WORK4);
    TBZ_xii(d, 32, 2);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	
	  // Calculate V Flag
	  CLS_ww(REG_WORK2, REG_WORK3);
	  CMP_ww(REG_WORK2, REG_WORK1);
	  BGE_i(2);
    SET_xxVflag(REG_WORK4, REG_WORK4);

    MSR_NZCV_x(REG_WORK4);
  } else {
  	// Calculate C Flag
    TBZ_xii(d, 32, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY
  
  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASL_l_reg,(RW4 d, RR4 i))

/*
 * ASLW
 * Operand Syntax: 	<ea>
 *
 * Operand Size: 16
 *
 * X Set according to the last bit shifted out of the operand.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if the most significant bit is changed at any time during the shift operation. Cleared otherwise.
 * C Set according to the last bit shifted out of the operand.
 *
 * Target is never a register.
 */
MIDFUNC(1,jnf_ASLW,(RW2 d))
{
	d = rmw(d);

	LSL_wwi(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_ASLW,(RW2 d))

MIDFUNC(1,jff_ASLW,(RW2 d))
{
	d = rmw(d);

  LSL_wwi(REG_WORK1, d, 17);
  TST_ww(REG_WORK1, REG_WORK1);

  if (needed_flags & FLAG_V) {
    // Calculate C flag
    MRS_NZCV_x(REG_WORK4);
    TBZ_wii(d, 15, 2);
    SET_xxCflag(REG_WORK4, REG_WORK4);

    // Calculate V flag
    EOR_wwwLSLi(REG_WORK1, d, d, 1); // eor bit15 and bit14 of source
    TBZ_wii(REG_WORK1, 15, 2);
    SET_xxVflag(REG_WORK4, REG_WORK4);

    MSR_NZCV_x(REG_WORK4);
  } else {
    // Calculate C flag
    TBZ_wii(d, 15, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
  LSL_wwi(d, d, 1);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_ASLW,(RW2 d))

/*
 * ASR
 * Operand Syntax: 	Dx, Dy
 * 					#<data>, Dy
 *					<ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set according to the last bit shifted out of the operand. Unaffected for a shift count of zero.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if the most significant bit is changed at any time during the shift operation. Cleared otherwise. Shift right -> always 0
 * C Set according to the last bit shifted out of the operand. Cleared for a shift count of zero.
 *
 */
MIDFUNC(2,jnf_ASR_b_imm,(RW1 d, IM8 i))
{
	if(i) {
		d = rmw(d);

		SIGNED8_REG_2_REG(REG_WORK1, d);
    if(i > 31)
      i = 31;
  	ASR_wwi(REG_WORK1, REG_WORK1, i);
	  BFI_xxii(d, REG_WORK1, 0, 8);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_ASR_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jnf_ASR_w_imm,(RW2 d, IM8 i))
{
	if(i) {
		d = rmw(d);

		SIGNED16_REG_2_REG(REG_WORK1, d);
    if(i > 31)
      i = 31;
  	ASR_wwi(REG_WORK1, REG_WORK1, i);
	  BFI_xxii(d, REG_WORK1, 0, 16);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_ASR_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jnf_ASR_l_imm,(RW4 d, IM8 i))
{
	if(i) {
		d = rmw(d);

    if(i > 31)
      i = 31;
  	ASR_wwi(d, d, i);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_ASR_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jff_ASR_b_imm,(RW1 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	if (i) {
    if(i > 31)
      i = 31;
		ASR_wwi(REG_WORK2, REG_WORK1, i);
	  BFI_wwii(d, REG_WORK2, 0, 8);
	  TST_ww(REG_WORK2, REG_WORK2);
	  
    // Calculate C flag
    TBZ_wii(REG_WORK1, i-1, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	  MSR_NZCV_x(REG_WORK4);
	  
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		TST_ww(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASR_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jff_ASR_w_imm,(RW2 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	if (i) {
    if(i > 31)
      i = 31;
		ASR_wwi(REG_WORK2, REG_WORK1, i);
	  BFI_wwii(d, REG_WORK2, 0, 16);
	  TST_ww(REG_WORK2, REG_WORK2);
	  
    // Calculate C flag
    TBZ_wii(REG_WORK1, i-1, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	  MSR_NZCV_x(REG_WORK4);
	  
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		TST_ww(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASR_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jff_ASR_l_imm,(RW4 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	if (i) {
		SXTW_xw(REG_WORK1, d);
		if(i > 32)
      i = 32;
		ASR_xxi(d, REG_WORK1, i);
		TST_ww(d, d);
		
    // Calculate C flag
    TBZ_wii(REG_WORK1, i-1, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	  MSR_NZCV_x(REG_WORK4);

    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		TST_ww(d, d);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASR_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jnf_ASR_b_reg,(RW1 d, RR4 i)) 
{
	if (isconst(i)) {
	  COMPCALL(jnf_ASR_b_imm)(d, live.state[i].val & 0x3f);
	  return;
	}

	i = readreg(i);
	d = rmw(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	AND_ww3f(REG_WORK2, i);
	ASR_www(REG_WORK1, REG_WORK1, REG_WORK2);
  BFI_wwii(d, REG_WORK1, 0, 8);

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jnf_ASR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ASR_w_reg,(RW2 d, RR4 i))
{
	if (isconst(i)) {
	  COMPCALL(jnf_ASR_w_imm)(d, live.state[i].val & 0x3f);
	  return;
	}

	i = readreg(i);
	d = rmw(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	AND_ww3f(REG_WORK2, i);
	ASR_www(REG_WORK1, REG_WORK1, REG_WORK2);
  BFI_wwii(d, REG_WORK1, 0, 16);

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jnf_ASR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ASR_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
	  COMPCALL(jnf_ASR_l_imm)(d, live.state[i].val & 0x3f);
	  return;
	}

	i = readreg(i);
	d = rmw(d);

	AND_ww3f(REG_WORK1, i);
	ASR_www(d, d, REG_WORK1);

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jnf_ASR_l_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_ASR_b_reg,(RW1 d, RR4 i))
{
	if (isconst(i)) {
	  COMPCALL(jff_ASR_b_imm)(d, live.state[i].val & 0x3f);
	  return;
	}

	i = readreg(i);
	d = rmw(d);

	SIGNED8_REG_2_REG(REG_WORK3, d);
	ANDS_ww3f(REG_WORK1, i);
	BNE_i(3);               // No shift -> X flag unchanged

  // shift count is 0
  TST_ww(REG_WORK3, REG_WORK3);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
  // shift count > 0
	ASR_www(REG_WORK2, REG_WORK3, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 8);
  TST_ww(REG_WORK2, REG_WORK2);
	
	// Calculate C Flag
	SUB_wwi(REG_WORK2, REG_WORK1, 1);
	ASR_www(REG_WORK2, REG_WORK3, REG_WORK2);
  TBZ_wii(REG_WORK2, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY
  
  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_ASR_w_reg,(RW2 d, RR4 i)) 
{
	if (isconst(i)) {
	  COMPCALL(jff_ASR_w_imm)(d, live.state[i].val & 0x3f);
	  return;
	}

	i = readreg(i);
	d = rmw(d);

	SIGNED16_REG_2_REG(REG_WORK3, d);
	ANDS_ww3f(REG_WORK1, i);
	BNE_i(3);               // No shift -> X flag unchanged

  // shift count is 0
  TST_ww(REG_WORK3, REG_WORK3);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
  // shift count > 0
	ASR_www(REG_WORK2, REG_WORK3, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 16);
  TST_ww(REG_WORK2, REG_WORK2);
	
	// Calculate C Flag
	SUB_wwi(REG_WORK2, REG_WORK1, 1);
	ASR_www(REG_WORK2, REG_WORK3, REG_WORK2);
  TBZ_wii(REG_WORK2, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY
  
  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jff_ASR_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
	  COMPCALL(jff_ASR_l_imm)(d, live.state[i].val & 0x3f);
	  return;
	}

	i = readreg(i);
	d = rmw(d);

	ANDS_ww3f(REG_WORK1, i);
	BNE_i(3);               // No shift -> X flag unchanged

  // shift count is 0
  TST_ww(d, d);           // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
  // shift count > 0
	MOV_ww(REG_WORK3, d);
	ASR_www(d, d, REG_WORK1);
  TST_ww(d, d);
	
	// Calculate C Flag
	SUB_wwi(REG_WORK2, REG_WORK1, 1);
	ASR_www(REG_WORK2, REG_WORK3, REG_WORK2);
  TBZ_wii(REG_WORK2, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY
  
  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASR_l_reg,(RW4 d, RR4 i))

/*
 * ASRW
 * Operand Syntax: 	<ea>
 *
 * Operand Size: 16
 *
 * X Set according to the last bit shifted out of the operand.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if the most significant bit is changed at any time during the shift operation. Cleared otherwise. Shift right -> always 0
 * C Set according to the last bit shifted out of the operand.
 *
 * Target is never a register.
 */
MIDFUNC(1,jnf_ASRW,(RW2 d))
{
	d = rmw(d);

	SIGNED16_REG_2_REG(d, d);
	ASR_wwi(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_ASRW,(RW2 d))

MIDFUNC(1,jff_ASRW,(RW2 d))
{
  d = rmw(d);

  SIGNED16_REG_2_REG(REG_WORK1, d);
	ASR_wwi(d, REG_WORK1, 1);
  TST_ww(d, d);

  // Calculate C flag
  TBZ_wii(REG_WORK1, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_ASRW,(RW2 d))

/*
 * BCHG
 * Operand Syntax: 	Dn,<ea>
 *					#<data>,<ea>
 *
 *  Operand Size: 8,32
 *
 * X Not affected.
 * N Not affected.
 * Z Set if the bit changed was zero. Cleared otherwise.
 * V Not affected.
 * C Not affected.
 *
 */
/* BCHG.B: target is never a register */
/* BCHG.L: target is always a register */
MIDFUNC(2,jnf_BCHG_b_imm,(RW1 d, IM8 s))
{
	d = rmw(d);
	EOR_xxbit(d, d, s & 0x7);
	unlock2(d);
}
MENDFUNC(2,jnf_BCHG_b_imm,(RW1 d, IM8 s))

MIDFUNC(2,jnf_BCHG_l_imm,(RW4 d, IM8 s))
{
	d = rmw(d);
	EOR_xxbit(d, d, s & 0x1f);
	unlock2(d);
}
MENDFUNC(2,jnf_BCHG_l_imm,(RW4 d, IM8 s))

MIDFUNC(2,jnf_BCHG_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCHG_b_imm)(d, live.state[s].val);
		return;
	}

	s = readreg(s);
	d = rmw(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	EOR_www(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_BCHG_b,(RW1 d, RR4 s))

MIDFUNC(2,jnf_BCHG_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCHG_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d,s);

	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	EOR_www(d, d, REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jnf_BCHG_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_BCHG_b_imm,(RW1 d, IM8 s))
{
	d = rmw(d);

	MRS_NZCV_x(REG_WORK1);
	EOR_xxbit(d, d, s & 0x7);
  UBFX_xxii(REG_WORK2, d, s & 0x7, 1);
  BFI_xxii(REG_WORK1, REG_WORK2, 30, 1);
  MSR_NZCV_x(REG_WORK1);
  
	unlock2(d);
}
MENDFUNC(2,jff_BCHG_b_imm,(RW1 d, IM8 s))

MIDFUNC(2,jff_BCHG_l_imm,(RW4 d, IM8 s))
{
	d = rmw(d);

	MRS_NZCV_x(REG_WORK1);
	EOR_xxbit(d, d, s & 0x1f);
  UBFX_xxii(REG_WORK2, d, s & 0x1f, 1);
  BFI_xxii(REG_WORK1, REG_WORK2, 30, 1);
  MSR_NZCV_x(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_BCHG_l_imm,(RW4 d, IM8 s))

MIDFUNC(2,jff_BCHG_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCHG_b_imm)(d, live.state[s].val);
		return;
	}

	s = readreg(s);
	d = rmw(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d, REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
  MSR_NZCV_x(REG_WORK1);
	EOR_www(d, d, REG_WORK2);
  
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BCHG_b,(RW1 d, RR4 s))

MIDFUNC(2,jff_BCHG_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCHG_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d,s);

	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d, REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
  MSR_NZCV_x(REG_WORK1);
	EOR_www(d, d, REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_BCHG_l,(RW4 d, RR4 s))

/*
 * BCLR
 * Operand Syntax: 	Dn,<ea>
 *					#<data>,<ea>
 *
 * Operand Size: 8,32
 *
 * X Not affected.
 * N Not affected.
 * Z Set if the bit cleared was zero. Cleared otherwise.
 * V Not affected.
 * C Not affected.
 *
 */
/* BCLR.B: target is never a register */
/* BCLR.L: target is always a register */
MIDFUNC(2,jnf_BCLR_b_imm,(RW1 d, IM8 s))
{
	d = rmw(d);
	CLEAR_xxbit(d, d, s & 0x7);
	unlock2(d);
}
MENDFUNC(2,jnf_BCLR_b_imm,(RW1 d, IM8 s))

MIDFUNC(2,jnf_BCLR_l_imm,(RW4 d, IM8 s))
{
	if(isconst(d)) {
		live.state[d].val = live.state[d].val & ~(1 << (s & 0x1f));
		return;
	}
	d = rmw(d);
	CLEAR_xxbit(d, d, s & 0x1f);
	unlock2(d);
}
MENDFUNC(2,jnf_BCLR_l_imm,(RW4 d, IM8 s))

MIDFUNC(2,jnf_BCLR_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCLR_b_imm)(d, live.state[s].val);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	BIC_www(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_BCLR_b,(RW1 d, RR4 s))

MIDFUNC(2,jnf_BCLR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCLR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d,s);

	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	BIC_www(d, d, REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jnf_BCLR_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_BCLR_b_imm,(RW1 d, IM8 s))
{
	d = rmw(d);

	MRS_NZCV_x(REG_WORK1);
  CLEAR_xxZflag(REG_WORK1, REG_WORK1);
	TBNZ_wii(d, s & 0x7, 2);
  SET_xxZflag(REG_WORK1, REG_WORK1);
  MSR_NZCV_x(REG_WORK1);
	CLEAR_xxbit(d, d, s & 0x7);

	unlock2(d);
}
MENDFUNC(2,jff_BCLR_b_imm,(RW1 d, IM8 s))

MIDFUNC(2,jff_BCLR_l_imm,(RW4 d, IM8 s))
{
	d = rmw(d);

	MRS_NZCV_x(REG_WORK1);
  CLEAR_xxZflag(REG_WORK1, REG_WORK1);
	TBNZ_wii(d, s & 0x1f, 2);
  SET_xxZflag(REG_WORK1, REG_WORK1);
  MSR_NZCV_x(REG_WORK1);
	CLEAR_xxbit(d, d, s & 0x1f);

	unlock2(d);
}
MENDFUNC(2,jff_BCLR_l_imm,(RW4 d, IM8 s))

MIDFUNC(2,jff_BCLR_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCLR_b_imm)(d, live.state[s].val);
		return;
	}

	s = readreg(s);
	d = rmw(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d,REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
	MSR_NZCV_x(REG_WORK1);
	BIC_www(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BCLR_b,(RW1 d, RR4 s))

MIDFUNC(2,jff_BCLR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCLR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d,s);

	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d,REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
	MSR_NZCV_x(REG_WORK1);
	BIC_www(d, d, REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_BCLR_l,(RW4 d, RR4 s))

/*
 * BFINS
 * Operand Syntax: 	#xxx.w,<ea>
 *
 * Operand Size: 32
 *
 * X Not affected.
 * N Set if the most significant bit of the bitfield is set. Cleared otherwise.
 * Z Set if the bitfield is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(4,jnf_BFINS_ii,(RW4 d, RR4 s, IM8 offs, IM8 width))
{
	INIT_REGS_l(d,s);

  BFI_wwii(d, s, (32 - offs - width), width);
  if(32 - offs - width < 0)
    BFI_xxii(d, s, (64 - offs - width), width);
    
	EXIT_REGS(d,s);
}
MENDFUNC(4,jnf_BFINS_ii,(RW4 d, RR4 s, IM8 offs, IM8 width))

MIDFUNC(4,jff_BFINS_ii,(RW4 d, RR4 s, IM8 offs, IM8 width))
{
	INIT_REGS_l(d,s);

  SBFX_wwii(REG_WORK1, s, 0, width);
  BFI_wwii(d, REG_WORK1, (32 - offs - width), width);
  if(32 - offs - width < 0)
    BFI_xxii(d, REG_WORK1, (64 - offs - width), width);
  TST_ww(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
	EXIT_REGS(d,s);
}
MENDFUNC(4,jff_BFINS_ii,(RW4 d, RR4 s, IM8 offs, IM8 width))

MIDFUNC(5,jnf_BFINS2_ii,(RW4 d, RW4 d2, RR4 s, IM8 offs, IM8 width))
{
  d2 = rmw(d2);
	INIT_REGS_l(d,s);

  SBFX_wwii(REG_WORK1, s, 0, width);
  BFI_xxii(d2, d, 32, 32);
  BFI_xxii(d2, REG_WORK1, (64 - offs - width), width);
  LSR_xxi(d, d2, 32);

	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jnf_BFINS2_ii,(RW4 d, RW4 d2, RR4 s, IM8 offs, IM8 width))

MIDFUNC(5,jff_BFINS2_ii,(RW4 d, RW4 d2, RR4 s, IM8 offs, IM8 width))
{
  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  
  SBFX_wwii(REG_WORK1, s, 0, width);
  BFI_xxii(d2, d, 32, 32);
  BFI_xxii(d2, REG_WORK1, (64 - offs - width), width);
  LSR_xxi(d, d2, 32);
  TST_ww(REG_WORK1, REG_WORK1);
  
  flags_carry_inverted = false;
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jff_BFINS2_ii,(RW4 d, RW4 d2, RR4 s, IM8 offs, IM8 width))

// Next only called when dest is D0-D7
MIDFUNC(4,jnf_BFINS_di,(RW4 d, RR4 s, RR4 offs, IM8 width))
{
	INIT_REGS_l(d,s);
  offs = readreg(offs);
  
  AND_xx1f(REG_WORK3, offs);
  MOV_xi(REG_WORK4, width);

  BFI_xxii(d, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_www(REG_WORK2, REG_WORK2, REG_WORK4);
  BFI_xxii(REG_WORK2, REG_WORK2, 32, 32);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d, d, REG_WORK2);

  ROR_www(REG_WORK1, s, REG_WORK4);
  BFI_xxii(REG_WORK1, REG_WORK1, 32, 32);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d, d, REG_WORK1);
  ROR_xxi(d, d, 32);

  unlock2(offs);
	EXIT_REGS(d,s);
}
MENDFUNC(4,jnf_BFINS_di,(RW4 d, RR4 s, RR4 offs, IM8 width))

MIDFUNC(4,jff_BFINS_di,(RW4 d, RR4 s, RR4 offs, IM8 width))
{
	INIT_REGS_l(d,s);
  offs = readreg(offs);
  
  AND_xx1f(REG_WORK3, offs);
  MOV_xi(REG_WORK4, width);

  BFI_xxii(d, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_www(REG_WORK2, REG_WORK2, REG_WORK4);
  BFI_xxii(REG_WORK2, REG_WORK2, 32, 32);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d, d, REG_WORK2);

  ROR_www(REG_WORK1, s, REG_WORK4);
  BFI_xxii(REG_WORK1, REG_WORK1, 32, 32);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d, d, REG_WORK1);
  ROR_xxi(d, d, 32);

  LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  TST_xx(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
  unlock2(offs);
	EXIT_REGS(d,s);
}
MENDFUNC(4,jff_BFINS_di,(RW4 d, RR4 s, RR4 offs, IM8 width))

MIDFUNC(4,jnf_BFINS_id,(RW4 d, RR4 s, IM8 offs, RR4 width))
{
  clobber_flags();

	INIT_REGS_l(d,s);
  width = readreg(width);
  
  MOV_xi(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);
  
  BFI_xxii(d, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_www(REG_WORK2, REG_WORK2, REG_WORK4);
  BFI_xxii(REG_WORK2, REG_WORK2, 32, 32);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d, d, REG_WORK2);

  ROR_www(REG_WORK1, s, REG_WORK4);
  BFI_xxii(REG_WORK1, REG_WORK1, 32, 32);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d, d, REG_WORK1);
  ROR_xxi(d, d, 32);

  unlock2(width);
	EXIT_REGS(d,s);
}
MENDFUNC(4,jnf_BFINS_id,(RW4 d, RR4 s, IM8 offs, RR4 width))

MIDFUNC(4,jff_BFINS_id,(RW4 d, RR4 s, IM8 offs, RR4 width))
{
	INIT_REGS_l(d,s);
  width = readreg(width);
  
  MOV_xi(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);

  BFI_xxii(d, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_www(REG_WORK2, REG_WORK2, REG_WORK4);
  BFI_xxii(REG_WORK2, REG_WORK2, 32, 32);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d, d, REG_WORK2);

  ROR_www(REG_WORK1, s, REG_WORK4);
  BFI_xxii(REG_WORK1, REG_WORK1, 32, 32);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d, d, REG_WORK1);
  ROR_xxi(d, d, 32);

  LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  TST_xx(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
  unlock2(width);
	EXIT_REGS(d,s);
}
MENDFUNC(4,jff_BFINS_id,(RW4 d, RR4 s, IM8 offs, RR4 width))

MIDFUNC(4,jnf_BFINS_dd,(RW4 d, RR4 s, RR4 offs, RR4 width))
{
  clobber_flags();

	INIT_REGS_l(d,s);
  offs = readreg(offs);
  width = readreg(width);

  AND_xx1f(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);
  
  BFI_xxii(d, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_www(REG_WORK2, REG_WORK2, REG_WORK4);
  BFI_xxii(REG_WORK2, REG_WORK2, 32, 32);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d, d, REG_WORK2);

  ROR_www(REG_WORK1, s, REG_WORK4);
  BFI_xxii(REG_WORK1, REG_WORK1, 32, 32);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d, d, REG_WORK1);
  ROR_xxi(d, d, 32);

  unlock2(width);
  unlock2(offs);
	EXIT_REGS(d,s);
}
MENDFUNC(4,jnf_BFINS_dd,(RW4 d, RR4 s, RR4 offs, RR4 width))

MIDFUNC(4,jff_BFINS_dd,(RW4 d, RR4 s, RR4 offs, RR4 width))
{
	INIT_REGS_l(d,s);
  offs = readreg(offs);
  width = readreg(width);
  
  AND_xx1f(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);

  BFI_xxii(d, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_www(REG_WORK2, REG_WORK2, REG_WORK4);
  BFI_xxii(REG_WORK2, REG_WORK2, 32, 32);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d, d, REG_WORK2);

  ROR_www(REG_WORK1, s, REG_WORK4);
  BFI_xxii(REG_WORK1, REG_WORK1, 32, 32);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d, d, REG_WORK1);
  ROR_xxi(d, d, 32);

  LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  TST_xx(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
  unlock2(width);
  unlock2(offs);
	EXIT_REGS(d,s);
}
MENDFUNC(4,jff_BFINS_dd,(RW4 d, RR4 s, RR4 offs, RR4 width))


// Next called when dest is <ea>
MIDFUNC(5,jnf_BFINS2_di,(RW4 d, RW4 d2, RR4 s, RR4 offs, IM8 width))
{
  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  offs = readreg(offs);

  AND_xx1f(REG_WORK3, offs);
  MOV_xi(REG_WORK4, width);

  BFI_xxii(d2, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK4);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d2, d2, REG_WORK2);
  
  ROR_xxx(REG_WORK1, s, REG_WORK4);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d2, d2, REG_WORK1);
  LSR_xxi(d, d2, 32);

  unlock2(offs);
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jnf_BFINS2_di,(RW4 d, RW4 d2, RR4 s, RR4 offs, IM8 width))

MIDFUNC(5,jff_BFINS2_di,(RW4 d, RW4 d2, RR4 s, RR4 offs, IM8 width))
{
  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  offs = readreg(offs);
  
  AND_xx1f(REG_WORK3, offs);
  MOV_xi(REG_WORK4, width);

  BFI_xxii(d2, d, 32, 32);
  
  MOVN_xi(REG_WORK2, 0);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK4);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d2, d2, REG_WORK2);
  
  ROR_xxx(REG_WORK1, s, REG_WORK4);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d2, d2, REG_WORK1);
  LSR_xxi(d, d2, 32);

  LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  TST_xx(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
  unlock2(offs);
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jff_BFINS2_di,(RW4 d, RW4 d2, RR4 s, RR4 offs, IM8 width))

MIDFUNC(5,jnf_BFINS2_id,(RW4 d, RW4 d2, RR4 s, IM8 offs, RR4 width))
{
  clobber_flags();

  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  width = readreg(width);

  MOV_xi(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);

  BFI_xxii(d2, d, 32, 32);

  MOVN_xi(REG_WORK2, 0);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK4);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d2, d2, REG_WORK2);
  
  ROR_xxx(REG_WORK1, s, REG_WORK4);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d2, d2, REG_WORK1);
  LSR_xxi(d, d2, 32);

  unlock2(width);
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jnf_BFINS2_id,(RW4 d, RW4 d2, RR4 s, IM8 offs, RR4 width))

MIDFUNC(5,jff_BFINS2_id,(RW4 d, RW4 d2, RR4 s, IM8 offs, RR4 width))
{
  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  width = readreg(width);
  
  MOV_xi(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);

  BFI_xxii(d2, d, 32, 32);
  
  MOVN_xi(REG_WORK2, 0);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK4);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d2, d2, REG_WORK2);
  
  ROR_xxx(REG_WORK1, s, REG_WORK4);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d2, d2, REG_WORK1);
  LSR_xxi(d, d2, 32);

  LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  TST_xx(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
  unlock2(width);
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jff_BFINS2_id,(RW4 d, RW4 d2, RR4 s, IM8 offs, RR4 width))

MIDFUNC(5,jnf_BFINS2_dd,(RW4 d, RW4 d2, RR4 s, RR4 offs, RR4 width))
{
  clobber_flags();

  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  offs = readreg(offs);
  width = readreg(width);

  AND_xx1f(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);

  BFI_xxii(d2, d, 32, 32);
  
  MOVN_xi(REG_WORK2, 0);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK4);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d2, d2, REG_WORK2);
  
  ROR_xxx(REG_WORK1, s, REG_WORK4);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d2, d2, REG_WORK1);
  LSR_xxi(d, d2, 32);

  unlock2(width);
  unlock2(offs);
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jnf_BFINS2_dd,(RW4 d, RW4 d2, RR4 s, RR4 offs, RR4 width))

MIDFUNC(5,jff_BFINS2_dd,(RW4 d, RW4 d2, RR4 s, RR4 offs, RR4 width))
{
  d2 = rmw(d2);
	INIT_REGS_l(d,s);
  offs = readreg(offs);
  width = readreg(width);
  
  AND_xx1f(REG_WORK3, offs);
  ANDS_xx1f(REG_WORK4, width);
  BNE_i(2);
  MOV_xi(REG_WORK4, 0x20);

  BFI_xxii(d2, d, 32, 32);
  
  MOVN_xi(REG_WORK2, 0);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK4);
  ROR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  AND_xxx(d2, d2, REG_WORK2);
  
  ROR_xxx(REG_WORK1, s, REG_WORK4);
  ROR_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  MVN_xx(REG_WORK2, REG_WORK2);
  AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
  
  ORR_xxx(d2, d2, REG_WORK1);
  LSR_xxi(d, d2, 32);

  LSL_xxx(REG_WORK1, REG_WORK1, REG_WORK3);
  TST_xx(REG_WORK1, REG_WORK1);

  flags_carry_inverted = false;
  unlock2(width);
  unlock2(offs);
	EXIT_REGS(d,s);
  unlock2(d2);
}
MENDFUNC(5,jff_BFINS2_dd,(RW4 d, RW4 d2, RR4 s, RR4 offs, RR4 width))

/*
 * BSET
 * Operand Syntax: 	Dn,<ea>
 *					#<data>,<ea>
 *
 *  Operand Size: 8,32
 *
 * X Not affected.
 * N Not affected.
 * Z Set if the bit set was zero. Cleared otherwise.
 * V Not affected.
 * C Not affected.
 *
 */
/* BSET.B: target is never a register */
/* BSET.L: target is always a register */
MIDFUNC(2,jnf_BSET_b_imm,(RW1 d, IM8 s))
{
	d = rmw(d);
	SET_xxbit(d, d, s & 0x7);
	unlock2(d);
}
MENDFUNC(2,jnf_BSET_b_imm,(RW1 d, IM8 s))

MIDFUNC(2,jnf_BSET_l_imm,(RW4 d, IM8 s))
{
	d = rmw(d);
	SET_xxbit(d, d, s & 0x1f);
	unlock2(d);
}
MENDFUNC(2,jnf_BSET_l_imm,(RW4 d, IM8 s))

MIDFUNC(2,jnf_BSET_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BSET_b_imm)(d, live.state[s].val);
		return;
	}

	s = readreg(s);
	d = rmw(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	ORR_www(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_BSET_b,(RW1 d, RR4 s))

MIDFUNC(2,jnf_BSET_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BSET_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d,s);

	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	ORR_www(d, d, REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jnf_BSET_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_BSET_b_imm,(RW1 d, IM8 s))
{
	d = rmw(d);

	MRS_NZCV_x(REG_WORK1);
  CLEAR_xxZflag(REG_WORK1, REG_WORK1);
	TBNZ_wii(d, s & 0x7, 2);
  SET_xxZflag(REG_WORK1, REG_WORK1);
  MSR_NZCV_x(REG_WORK1);
	SET_xxbit(d, d, s & 0x7);

	unlock2(d);
}
MENDFUNC(2,jff_BSET_b_imm,(RW1 d, IM8 s))

MIDFUNC(2,jff_BSET_l_imm,(RW4 d, IM8 s))
{
	d = rmw(d);

	MRS_NZCV_x(REG_WORK1);
  CLEAR_xxZflag(REG_WORK1, REG_WORK1);
	TBNZ_wii(d, s & 0x1f, 2);
  SET_xxZflag(REG_WORK1, REG_WORK1);
  MSR_NZCV_x(REG_WORK1);
	SET_xxbit(d, d, s & 0x1f);

	unlock2(d);
}
MENDFUNC(2,jff_BSET_l_imm,(RW4 d, IM8 s))

MIDFUNC(2,jff_BSET_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BSET_b_imm)(d, live.state[s].val);
		return;
	}

	s = readreg(s);
	d = rmw(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d,REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
  MSR_NZCV_x(REG_WORK1);
	ORR_www(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BSET_b,(RW1 d, RR4 s))

MIDFUNC(2,jff_BSET_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BSET_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d,s);

	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d,REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
  MSR_NZCV_x(REG_WORK1);
	ORR_www(d, d, REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_BSET_l,(RW4 d, RR4 s))

/*
 * BTST
 * Operand Syntax: 	Dn,<ea>
 *					#<data>,<ea>
 *
 *  Operand Size: 8,32
 *
 * X Not affected
 * N Not affected
 * Z Set if the bit tested is zero. Cleared otherwise
 * V Not affected
 * C Not affected
 *
 */
/* BTST.B: target is never a register */
/* BTST.L: target is always a register */
MIDFUNC(2,jff_BTST_b_imm,(RR1 d, IM8 s))
{
	d = readreg(d);

	MRS_NZCV_x(REG_WORK1);
	CLEAR_xxZflag(REG_WORK1, REG_WORK1);
	TBNZ_wii(d, s & 0x7, 2); // skip next if bit is set
	SET_xxZflag(REG_WORK1, REG_WORK1);
	MSR_NZCV_x(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_BTST_b_imm,(RR1 d, IM8 s))

MIDFUNC(2,jff_BTST_l_imm,(RR4 d, IM8 s))
{
	d = readreg(d);

	MRS_NZCV_x(REG_WORK1);
	CLEAR_xxZflag(REG_WORK1, REG_WORK1);
	TBNZ_wii(d, s & 0x1f, 2); // skip next if bit is set
	SET_xxZflag(REG_WORK1, REG_WORK1);
	MSR_NZCV_x(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_BTST_l_imm,(RR4 d, IM8 s))

MIDFUNC(2,jff_BTST_b,(RR1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BTST_b_imm)(d, live.state[s].val);
		return;
	}

	s = readreg(s);
	d = readreg(d);

	UBFIZ_xxii(REG_WORK1, s, 0, 3); // REG_WORK1 = s & 7
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d, REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
	MSR_NZCV_x(REG_WORK1);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BTST_b,(RR1 d, RR4 s))

MIDFUNC(2,jff_BTST_l,(RR4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BTST_l_imm)(d, live.state[s].val);
		return;
	}

	int s_is_d = (s == d);
	d = readreg(d);
	if(!s_is_d)
		s = readreg(s);
	else
		s = d;
		
	UBFIZ_xxii(REG_WORK1, s, 0, 5); // REG_WORK1 = s & 31
	MOV_xi(REG_WORK2, 1);
	LSL_www(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_NZCV_x(REG_WORK1);
	TST_ww(d, REG_WORK2);
  CSET_xc(REG_WORK3, NATIVE_CC_EQ);
  BFI_xxii(REG_WORK1, REG_WORK3, 30, 1);
	MSR_NZCV_x(REG_WORK1);

	unlock2(d);
	if(!s_is_d)
		unlock2(s);
}
MENDFUNC(2,jff_BTST_l,(RR4 d, RR4 s))

/*
 * CLR
 * Operand Syntax: <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Always cleared.
 * Z Always set.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(1,jnf_CLR_b,(W1 d))
{
	if(d >= 16) {
		set_const(d, 0);
		return;
	}
	INIT_WREG_b(d);
	CLEAR_LOW8_xx(d, d);
	unlock2(d);
}
MENDFUNC(1,jnf_CLR_b,(W1 d))

MIDFUNC(1,jnf_CLR_w,(W2 d))
{
	if(d >= 16) {
		set_const(d, 0);
		return;
	}
	INIT_WREG_w(d);
  CLEAR_LOW16_xx(d, d);
	unlock2(d);
}
MENDFUNC(1,jnf_CLR_w,(W2 d))

MIDFUNC(1,jnf_CLR_l,(W4 d))
{
	set_const(d, 0);
}
MENDFUNC(1,jnf_CLR_l,(W4 d))

MIDFUNC(1,jff_CLR_b,(W1 d))
{
	MOV_xish(REG_WORK1, 0x4000, 16); // set Z flag
	MSR_NZCV_x(REG_WORK1);
  flags_carry_inverted = false;
	if(d >= 16) {
		set_const(d, 0);
		return;
	}
	INIT_WREG_b(d);
	CLEAR_LOW8_xx(d, d);
	unlock2(d);
}
MENDFUNC(1,jff_CLR_b,(W1 d))

MIDFUNC(1,jff_CLR_w,(W2 d))
{
	MOV_xish(REG_WORK1, 0x4000, 16); // set Z flag
	MSR_NZCV_x(REG_WORK1);
  flags_carry_inverted = false;
	if(d >= 16) {
		set_const(d, 0);
    return;
  }
	INIT_WREG_w(d);
	CLEAR_LOW16_xx(d, d);
	unlock2(d);
}
MENDFUNC(1,jff_CLR_w,(W2 d))

MIDFUNC(1,jff_CLR_l,(W4 d))
{
	MOV_xish(REG_WORK1, 0x4000, 16); // set Z flag
	MSR_NZCV_x(REG_WORK1);
  flags_carry_inverted = false;
	set_const(d, 0);
}
MENDFUNC(1,jff_CLR_l,(W4 d))

/*
 * CMP
 * Operand Syntax: <ea>, Dn
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if an overflow occurs. Cleared otherwise.
 * C Set if a borrow occurs. Cleared otherwise.
 *
 */
MIDFUNC(2,jff_CMP_b_imm,(RR1 d, IM8 v))
{
  if (isconst(d)) {
    uae_u8 tmp = (uae_u8)(live.state[d].val & 0xff);
    MOV_wish(REG_WORK1, tmp << 8, 16);
  } else {
   d = readreg(d);
	 LSL_wwi(REG_WORK1, d, 24);
   unlock2(d);
  }

  MOV_wi(REG_WORK2, v & 0xff);
  CMP_wwLSLi(REG_WORK1, REG_WORK2, 24);

  flags_carry_inverted = true;
}
MENDFUNC(2,jff_CMP_b_imm,(RR1 d, IM8 v))

MIDFUNC(2,jff_CMP_b,(RR1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_CMP_b_imm)(d, live.state[s].val);
		return;
	}

	if (isconst(d)) {
	  uae_u8 tmp = (uae_u8)(live.state[d].val & 0xff);
	  s = readreg(s);
	  MOV_wish(REG_WORK1, tmp << 8, 16);
	  CMP_wwLSLi(REG_WORK1, s, 24);
	  unlock2(s);
	} else {
	  INIT_RREGS_b(d, s);
		  
	  LSL_wwi(REG_WORK1, d, 24);
	  CMP_wwLSLi(REG_WORK1, s, 24);

	  EXIT_REGS(d,s);
  }
  flags_carry_inverted = true;
}
MENDFUNC(2,jff_CMP_b,(RR1 d, RR1 s))

MIDFUNC(2,jff_CMP_w_imm,(RR2 d, IM16 v))
{
	if (isconst(d)) {
	  MOV_wish(REG_WORK1, (live.state[d].val & 0xffff), 16);
	  MOV_wish(REG_WORK2, (v & 0xffff), 16);
	  CMP_ww(REG_WORK1, REG_WORK2);
	} else {
	  d = readreg(d);

	  LSL_wwi(REG_WORK1, d, 16);
	  MOV_xi(REG_WORK2, v);
	  CMP_wwLSLi(REG_WORK1, REG_WORK2, 16);

	  unlock2(d);
  }
  flags_carry_inverted = true;
}
MENDFUNC(2,jff_CMP_w_imm,(RR2 d, IM16 v))

MIDFUNC(2,jff_CMP_w,(RR2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_CMP_w_imm)(d, live.state[s].val);
		return;
	}

	if (isconst(d)) {
	  uae_u16 tmp = (uae_u16)(live.state[d].val & 0xffff);
	  s = readreg(s);
	  MOV_wish(REG_WORK1, tmp, 16);
	  CMP_wwLSLi(REG_WORK1, s, 16);
	  unlock2(s);
	} else {
	  INIT_RREGS_w(d, s);

	  LSL_wwi(REG_WORK1, d, 16);
	  CMP_wwLSLi(REG_WORK1, s, 16);

	  EXIT_REGS(d,s);
  }
  flags_carry_inverted = true;
}
MENDFUNC(2,jff_CMP_w,(RR2 d, RR2 s))

MIDFUNC(2,jff_CMP_l_imm,(RR4 d, IM32 v))
{
  if(isconst(d)) {
    uae_u32 newv = ((uae_u32)live.state[d].val) - ((uae_u32)v);
    int flgs = ((uae_s32)v) < 0;
  	int flgo = ((uae_s32)live.state[d].val) < 0;
  	int flgn = ((uae_s32)newv) < 0;
    uae_u32 f = 0;
    if(((uae_s32)newv) == 0)
      f |= (ARM_Z_FLAG >> 16);
    if((flgs != flgo) && (flgn != flgo))
      f |= (ARM_V_FLAG >> 16);
    if(((uae_u32)v) > ((uae_u32)live.state[d].val))
      f |= (ARM_C_FLAG >> 16);
    if(flgn != 0)
      f |= (ARM_N_FLAG >> 16);
  	MOV_xish(REG_WORK1, f, 16);
  	MSR_NZCV_x(REG_WORK1);

    flags_carry_inverted = false;
    return;
  }

	d = readreg(d);

  LOAD_U32(REG_WORK1, v);
	CMP_ww(d, REG_WORK1);

  flags_carry_inverted = true;
	unlock2(d);
}
MENDFUNC(2,jff_CMP_l_imm,(RR4 d, IM32 v))

MIDFUNC(2,jff_CMP_l,(RR4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_CMP_l_imm)(d, live.state[s].val);
    return;
  }

	INIT_RREGS_l(d, s);

	CMP_ww(d, s);

  flags_carry_inverted = true;
	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_CMP_l,(RR4 d, RR4 s))

/*
 * CMPA
 * Operand Syntax: 	<ea>, An
 *
 * Operand Size: 16,32
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if an overflow occurs. Cleared otherwise.
 * C Set if a borrow occurs. Cleared otherwise.
 *
 */
MIDFUNC(2,jff_CMPA_w_imm,(RR2 d, IM16 v))
{
  uae_u16 tmp = (uae_u16)(v & 0xffff);
  d = readreg(d);
  SIGNED16_IMM_2_REG(REG_WORK1, tmp);
  CMP_ww(d, REG_WORK1);
  unlock2(d);

  flags_carry_inverted = true;
}
MENDFUNC(2,jff_CMPA_w_imm,(RR2 d, IM16 v))

MIDFUNC(2,jff_CMPA_w,(RR2 d, RR2 s))
{
  if (isconst(s)) {
		COMPCALL(jff_CMPA_w_imm)(d, live.state[s].val);
		return;
	}

  INIT_RREGS_w(d, s);
  CMP_wwEX(d, s, EX_SXTH);
  EXIT_REGS(d,s);

  flags_carry_inverted = true;
}
MENDFUNC(2,jff_CMPA_w,(RR2 d, RR2 s))

MIDFUNC(2,jff_CMPA_l_imm,(RR4 d, IM32 v))
{
  d = readreg(d);

  LOAD_U32(REG_WORK1, v);
	CMP_ww(d, REG_WORK1);

  flags_carry_inverted = true;
	unlock2(d);
}
MENDFUNC(2,jff_CMPA_l_imm,(RR4 d, IM32 v))

MIDFUNC(2,jff_CMPA_l,(RR4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_CMPA_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_RREGS_l(d, s);

	CMP_ww(d, s);

  flags_carry_inverted = true;
	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_CMPA_l,(RR4 d, RR4 s))

/*
 * DBCC
 *
 */
MIDFUNC(2,jff_DBCC,(RW4 d, IM8 cc))
{
  d = rmw(d);
  
  FIX_INVERTED_CARRY

  // If cc true -> no branch, so we have to clear ARM_C_FLAG
	MOV_xish(REG_WORK1, 0x2000, 16); // set C flag
	MOV_xi(REG_WORK2, 0);
  switch(cc) {
    case 9: // LS
      CSEL_xxxc(REG_WORK1, REG_WORK2, REG_WORK1, NATIVE_CC_EQ);
      CSEL_xxxc(REG_WORK1, REG_WORK2, REG_WORK1, NATIVE_CC_CS);
      break;

    case 8: // HI
      MOV_xish(REG_WORK3, 0x2000, 16);
      CSEL_xxxc(REG_WORK1, REG_WORK2, REG_WORK1, NATIVE_CC_CC);
      CSEL_xxxc(REG_WORK1, REG_WORK3, REG_WORK1, NATIVE_CC_EQ);
      break;

    default:
      CSEL_xxxc(REG_WORK1, REG_WORK2, REG_WORK1, cc);
    	break;
  }
  clobber_flags();
	MSR_NZCV_x(REG_WORK1);

  BCC_i(4); // If cc true -> no sub
  
  // sub (d, 1)
  LSL_wwi(REG_WORK2, d, 16);
	SUBS_wwish(REG_WORK2, REG_WORK2, 0x10, 1);
	BFXIL_xxii(d, REG_WORK2, 16, 16);
  
  // caller can now use register_branch(v1, v2, NATIVE_CC_CS);
 
  unlock2(d);
}
MENDFUNC(2,jff_DBCC,(RW4 d, IM8 cc))

/*
 * DIVU
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set or overflow. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if overflow. Cleared otherwise.
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_DIVU,(RW4 d, RR4 s))
{
	int init_regs_used = 0;
  int targetIsReg;
  int s_is_d;
	if (isconst(s) && (uae_u16)live.state[s].val != 0) {
	  uae_u16 tmp = (uae_u16)live.state[s].val;
	  d = rmw(d);
	  UNSIGNED16_IMM_2_REG(REG_WORK3, tmp);
	} else {
	  targetIsReg = (d < 16);
	  s_is_d = (s == d);
	  if(!s_is_d)
		  s = readreg(s);
	  d = rmw(d);
	  if(s_is_d)
		  s = d;
    init_regs_used = 1;
  
    UNSIGNED16_REG_2_REG(REG_WORK3, s);
    CBNZ_wi(REG_WORK3, 4);    // src is not 0

    // Signal exception 5
    MOV_wi(REG_WORK1, 5);
    uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
    STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
    B_i(7);        // end_of_op
  }

	// src is not 0  
	UDIV_www(REG_WORK1, d, REG_WORK3);

  LSR_wwi(REG_WORK2, REG_WORK1, 16); 							// if result of this is not 0, DIVU overflows -> no result
  CBNZ_wi(REG_WORK2, 4);
  
  // Here we have to calc remainder
  MSUB_wwww(REG_WORK2, REG_WORK1, REG_WORK3, d);
  LSL_wwi(d, REG_WORK2, 16);
  BFI_wwii(d, REG_WORK1, 0, 16);
  // end_of_op

	if (init_regs_used) {
	  EXIT_REGS(d, s);
  } else {
    unlock2(d);
  }
}
MENDFUNC(2,jnf_DIVU,(RW4 d, R4 s))

MIDFUNC(2,jff_DIVU,(RW4 d, RR4 s))
{
	uae_u32* branchadd;
	int init_regs_used = 0;
  int targetIsReg;
  int s_is_d;
	if (isconst(s) && (uae_u16)live.state[s].val != 0) {
	  uae_u16 tmp = (uae_u16)live.state[s].val;
	  d = rmw(d);
	  UNSIGNED16_IMM_2_REG(REG_WORK3, tmp);
	} else {
	  targetIsReg = (d < 16);
	  s_is_d = (s == d);
	  if(!s_is_d)
		  s = readreg(s);
	  d = rmw(d);
	  if(s_is_d)
		  s = d;
    init_regs_used = 1;

    UNSIGNED16_REG_2_REG(REG_WORK3, s);
    uae_u32* branchadd_not0 = (uae_u32*)get_target();
    CBNZ_wi(REG_WORK3, 0);     // src is not 0

    // Signal exception 5
	  MOV_wi(REG_WORK1, 5);
    uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
    STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
    
    // flag handling like divbyzero_special()
  	if (currprefs.cpu_model == 68020 || currprefs.cpu_model == 68030) {
      MOV_wish(REG_WORK1, 0x5000, 16); // Set V and Z (if d >=0)
			TBZ_wii(d, 31, 2);
			MOV_wish(REG_WORK1, 0x9000, 16); // Set V and N (if d < 0)
  	} else if (currprefs.cpu_model >= 68040) {
    	MRS_NZCV_x(REG_WORK1);
    	CLEAR_xxCflag(REG_WORK1, REG_WORK1);
  	} else {
  		// 68000/010
      MOV_wish(REG_WORK1, 0x0000, 16);
   	}
    MSR_NZCV_x(REG_WORK1);
    branchadd = (uae_u32*)get_target();
    B_i(0);        // end_of_op
    write_jmp_target(branchadd_not0, (uintptr)get_target());
  }

	// src is not 0  
	UDIV_www(REG_WORK1, d, REG_WORK3);
  
  LSR_wwi(REG_WORK2, REG_WORK1, 16); 							// if result of this is not 0, DIVU overflows
  CBZ_wi(REG_WORK2, 4);
  // Here we handle overflow
  MOV_wish(REG_WORK1, 0x9000, 16); // set V and N
	MSR_NZCV_x(REG_WORK1);
  B_i(6);
  
  // Here we have to calc flags and remainder
  LSL_wwi(REG_WORK2, REG_WORK1, 16);
  TST_ww(REG_WORK2, REG_WORK2);    // N and Z ok, C and V cleared
  
  MSUB_wwww(REG_WORK2, REG_WORK1, REG_WORK3, d);
  LSL_wwi(d, REG_WORK2, 16);
  BFI_wwii(d, REG_WORK1, 0, 16);

  // end_of_op
  flags_carry_inverted = false;
	if (init_regs_used) {
    write_jmp_target(branchadd, (uintptr)get_target());
	  EXIT_REGS(d, s);
  } else {
    unlock2(d);
  }
}
MENDFUNC(2,jff_DIVU,(RW4 d, RR4 s))
 
/*
 * DIVS
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set or overflow. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if overflow. Cleared otherwise.
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_DIVS,(RW4 d, RR4 s))
{
	uae_u32* branchadd;
	int init_regs_used = 0;
  int targetIsReg;
  int s_is_d;
  uae_s16 tmp;
  if (isconst(s) && (uae_s16)live.state[s].val != 0) {
    tmp = (uae_s16)live.state[s].val;
    d = rmw(d);
    SIGNED16_IMM_2_REG(REG_WORK3, tmp);
  } else {
	  targetIsReg = (d < 16);
	  s_is_d = (s == d);
	  if(!s_is_d)
		  s = readreg(s);
	  d = rmw(d);
	  if(s_is_d)
		  s = d;
    init_regs_used = 1;
  
    SIGNED16_REG_2_REG(REG_WORK3, s);
    CBNZ_wi(REG_WORK3, 4);     // src is not 0

    // Signal exception 5
    MOV_wi(REG_WORK1, 5);
    uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
    STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
    branchadd = (uae_u32*)get_target();
    B_i(0);        // end_of_op
  }

	// src is not 0  
	SDIV_www(REG_WORK1, d, REG_WORK3);
  
  // check for overflow
  MOVN_wi(REG_WORK2, 0x7fff);           // REG_WORK2 is now 0xffff8000
  ANDS_www(REG_WORK3, REG_WORK1, REG_WORK2);
  BEQ_i(3); 														// positive result, no overflow
	CMP_ww(REG_WORK3, REG_WORK2);
	BNE_i(8);															// overflow -> end_of_op
	
  // Here we have to calc remainder
  if (init_regs_used)
    SIGNED16_REG_2_REG(REG_WORK3, s);
  else
    SIGNED16_IMM_2_REG(REG_WORK3, tmp);
  MSUB_wwww(REG_WORK2, REG_WORK1, REG_WORK3, d);		// REG_WORK2 contains remainder
	
	EOR_www(REG_WORK3, REG_WORK2, d);	  // If sign of remainder and first operand differs, change sign of remainder
	TBZ_wii(REG_WORK3, 31, 2);
	NEG_ww(REG_WORK2, REG_WORK2);
	
  LSL_wwi(d, REG_WORK2, 16);
  BFI_wwii(d, REG_WORK1, 0, 16);
	
  // end_of_op
  if (init_regs_used) {
    write_jmp_target(branchadd, (uintptr)get_target());
	  EXIT_REGS(d, s);
  } else {
    unlock2(d);
  }
}
MENDFUNC(2,jnf_DIVS,(RW4 d, RR4 s))

MIDFUNC(2,jff_DIVS,(RW4 d, RR4 s))
{
	uae_u32* branchadd;
	int init_regs_used = 0;
  int targetIsReg;
  int s_is_d;
  uae_s16 tmp;
  if (isconst(s) && (uae_s16)live.state[s].val != 0) {
    tmp = (uae_s16)live.state[s].val;
    d = rmw(d);
    SIGNED16_IMM_2_REG(REG_WORK3, tmp);
  } else {
	  targetIsReg = (d < 16);
	  s_is_d = (s == d);
	  if(!s_is_d)
		  s = readreg(s);
	  d = rmw(d);
	  if(s_is_d)
		  s = d;
    init_regs_used = 1;

    SIGNED16_REG_2_REG(REG_WORK3, s);
    uae_u32* branchadd_not0 = (uae_u32*)get_target();
    CBNZ_wi(REG_WORK3, 0);     // src is not 0

    // Signal exception 5
	  MOV_wi(REG_WORK1, 5);
    uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
    STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
    
    // flag handling like divbyzero_special()
  	if (currprefs.cpu_model == 68020 || currprefs.cpu_model == 68030) {
      MOV_wish(REG_WORK1, 0x4000, 16); // Set Z
  	} else if (currprefs.cpu_model >= 68040) {
    	MRS_NZCV_x(REG_WORK1);
    	CLEAR_xxCflag(REG_WORK1, REG_WORK1);
  	} else {
  		// 68000/010
      MOV_wish(REG_WORK1, 0x0000, 16);
   	}

    MSR_NZCV_x(REG_WORK1);
    branchadd = (uae_u32*)get_target();
    B_i(0);        // end_of_op
    write_jmp_target(branchadd_not0, (uintptr)get_target());
  }

	// src is not 0  
	SDIV_www(REG_WORK1, d, REG_WORK3);

  // check for overflow
  MOVN_wi(REG_WORK2, 0x7fff);           // REG_WORK2 is now 0xffff8000
  ANDS_www(REG_WORK3, REG_WORK1, REG_WORK2);
  BEQ_i(6); 														// positive result, no overflow
	CMP_ww(REG_WORK3, REG_WORK2);
	BEQ_i(4);															// no overflow
  
  // Here we handle overflow
  MOV_wish(REG_WORK1, 0x9000, 16); // set V and N
	MSR_NZCV_x(REG_WORK1);
  B_i(10);
  
  // calc flags
  LSL_wwi(REG_WORK2, REG_WORK1, 16);
  TST_ww(REG_WORK2, REG_WORK2);         // N and Z ok, C and V cleared
  
  // calc remainder
  if (init_regs_used)
    SIGNED16_REG_2_REG(REG_WORK3, s);
  else
    SIGNED16_IMM_2_REG(REG_WORK3, tmp);
	MSUB_wwww(REG_WORK2, REG_WORK1, REG_WORK3, d);		// REG_WORK2 contains remainder

	EOR_www(REG_WORK3, REG_WORK2, d);	  // If sign of remainder and first operand differs, change sign of remainder
	TBZ_wii(REG_WORK3, 31, 2);
	NEG_ww(REG_WORK2, REG_WORK2);
	
  LSL_wwi(d, REG_WORK2, 16);
  BFI_wwii(d, REG_WORK1, 0, 16);

  // end_of_op
  flags_carry_inverted = false;
  if (init_regs_used) {
    write_jmp_target(branchadd, (uintptr)get_target());
	  EXIT_REGS(d, s);
  } else {
    unlock2(d);
  }
}
MENDFUNC(2,jff_DIVS,(RW4 d, RR4 s))

MIDFUNC(3,jnf_DIVLU32,(RW4 d, RR4 s1, W4 rem))
{
  s1 = readreg(s1);
  d = rmw(d);
  rem = writereg(rem);

  CBNZ_wi(s1, 4);     // src is not 0

  // Signal exception 5
	MOV_wi(REG_WORK1, 5);
  uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
  STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
	B_i(4);        // end_of_op

	// src is not 0  
	UDIV_www(REG_WORK1, d, s1);

  // Here we have to calc remainder
  MSUB_wwww(rem, s1, REG_WORK1, d);
  MOV_ww(d, REG_WORK1);

// end_of_op
	
  unlock2(rem);
  unlock2(d);
  unlock2(s1);
}
MENDFUNC(3,jnf_DIVLU32,(RW4 d, RR4 s1, W4 rem))

MIDFUNC(3,jff_DIVLU32,(RW4 d, RR4 s1, W4 rem))
{
  s1 = readreg(s1);
  d = rmw(d);
  rem = writereg(rem);

  CBNZ_wi(s1, 4);     // src is not 0

  // Signal exception 5
	MOV_wi(REG_WORK1, 5);
  uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
  STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
  
	B_i(5);        // end_of_op

	// src is not 0  
	UDIV_www(REG_WORK1, d, s1);

  // Here we have to calc flags and remainder
  TST_ww(REG_WORK1, REG_WORK1);
  
  MSUB_wwww(rem, s1, REG_WORK1, d);
  MOV_ww(d, REG_WORK1);

  // end_of_op

  flags_carry_inverted = false;
  unlock2(rem);
  unlock2(d);
  unlock2(s1);
}
MENDFUNC(3,jff_DIVLU32,(RW4 d, RR4 s1, W4 rem))

MIDFUNC(3,jnf_DIVLS32,(RW4 d, RR4 s1, W4 rem))
{
  s1 = readreg(s1);
  d = rmw(d);
  rem = writereg(rem);

  CBNZ_wi(s1, 4);     // src is not 0

  // Signal exception 5
	MOV_wi(REG_WORK1, 5);
  uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
  STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
	B_i(7);        // end_of_op

	// src is not 0  
	SDIV_www(REG_WORK1, d, s1);

  // Here we have to calc remainder
  MSUB_wwww(rem, s1, REG_WORK1, d);

	EOR_www(REG_WORK3, rem, d);	// If sign of remainder and first operand differs, change sign of remainder
	TBZ_wii(REG_WORK3, 31, 2);
	NEG_ww(rem, rem);
  
  MOV_ww(d, REG_WORK1);

  // end_of_op
	
  unlock2(rem);
  unlock2(d);
  unlock2(s1);
}
MENDFUNC(3,jnf_DIVLS32,(RW4 d, RR4 s1, W4 rem))

MIDFUNC(3,jff_DIVLS32,(RW4 d, RR4 s1, W4 rem))
{
  s1 = readreg(s1);
  d = rmw(d);
  rem = writereg(rem);

  CBNZ_wi(s1, 4);     // src is not 0

  // Signal exception 5
	MOV_wi(REG_WORK1, 5);
  uintptr idx = (uintptr)(&regs.jit_exception) - (uintptr)(&regs);
  STR_wXi(REG_WORK1, R_REGSTRUCT, idx);
	B_i(8);        // end_of_op

	// src is not 0  
	SDIV_www(REG_WORK1, d, s1);

  // Here we have to calc remainder
  MSUB_wwww(rem, s1, REG_WORK1, d);

	EOR_www(REG_WORK3, rem, d);	// If sign of remainder and first operand differs, change sign of remainder
	TBZ_wii(REG_WORK3, 31, 2);
	NEG_ww(REG_WORK2, REG_WORK2);
	
  MOV_ww(d, REG_WORK1);
  TST_ww(d, d);
    
  // end_of_op
	
  flags_carry_inverted = false;
  unlock2(rem);
  unlock2(d);
  unlock2(s1);
}
MENDFUNC(3,jff_DIVLS32,(RW4 d, RR4 s1, W4 rem))

/*
 * EOR
 * Operand Syntax: 	Dn, <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_EOR_b_imm,(RW1 d, IM8 v))
{
	INIT_REG_b(d);

  MOV_xi(REG_WORK1, (v & 0xff));
	EOR_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_EOR_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jnf_EOR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_EOR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		EOR_www(REG_WORK1, d, s);
		BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		EOR_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_EOR_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_EOR_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

  MOV_xi(REG_WORK1, v & 0xffff);
  EOR_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_EOR_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jnf_EOR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_EOR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	EOR_www(REG_WORK1, d, s);
  BFI_xxii(d, REG_WORK1, 0, 16);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_EOR_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_EOR_l_imm,(RW4 d, IM32 v))
{
  if(isconst(d)) {
    live.state[d].val = live.state[d].val ^ v;
    return;
  }
  
	d = rmw(d);

  LOAD_U32(REG_WORK1, v);
	EOR_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_EOR_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jnf_EOR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_EOR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	EOR_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_EOR_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_EOR_b_imm,(RW1 d, IM8 v))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_IMM_2_REG(REG_WORK2, v);
	if(targetIsReg) {
		EOR_www(REG_WORK1, REG_WORK1, REG_WORK2);
		BFI_xxii(d, REG_WORK1, 0, 8);
		TST_ww(REG_WORK1, REG_WORK1);
	} else {
		EOR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_EOR_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jff_EOR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_EOR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_REG_2_REG(REG_WORK2, s);
	if(targetIsReg) {
		EOR_www(REG_WORK1, REG_WORK1, REG_WORK2);
		BFI_xxii(d, REG_WORK1, 0, 8);
		TST_ww(REG_WORK1, REG_WORK1);
	} else {
		EOR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_EOR_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_EOR_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_IMM_2_REG(REG_WORK2, v);
	if(targetIsReg) {
		EOR_www(REG_WORK1, REG_WORK1, REG_WORK2);
	  BFI_xxii(d, REG_WORK1, 0, 16);
		TST_ww(REG_WORK1, REG_WORK1);
	} else {
		EOR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_EOR_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jff_EOR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_EOR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_REG_2_REG(REG_WORK2, s);
	if(targetIsReg) {
		EOR_www(REG_WORK1, REG_WORK1, REG_WORK2);
	  BFI_xxii(d, REG_WORK1, 0, 16);
		TST_ww(REG_WORK1, REG_WORK1);
	} else {
		EOR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_EOR_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_EOR_l_imm,(RW4 d, IM32 v))
{
	d = rmw(d);

  LOAD_U32(REG_WORK1, v);
	EOR_www(d, d, REG_WORK1);
	TST_ww(d, d);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_EOR_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jff_EOR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_EOR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	EOR_www(d, d, s);
	TST_ww(d, d);

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_EOR_l,(RW4 d, RR4 s))

/*
 * EORSR
 * Operand Syntax: 	#<data>, CCR
 *
 * Operand Size: 8
 *
 * X — Changed if bit 4 of immediate operand is one; unchanged otherwise.
 * N — Changed if bit 3 of immediate operand is one; unchanged otherwise.
 * Z — Changed if bit 2 of immediate operand is one; unchanged otherwise.
 * V — Changed if bit 1 of immediate operand is one; unchanged otherwise.
 * C — Changed if bit 0 of immediate operand is one; unchanged otherwise.
 *
 */
MIDFUNC(2,jff_EORSR,(IM32 s, IM8 x))
{
	MRS_NZCV_x(REG_WORK1);
  if(flags_carry_inverted) {
    EOR_xxCflag(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
  }
	MOV_xish(REG_WORK2, (s >> 16), 16);
	EOR_www(REG_WORK1, REG_WORK1, REG_WORK2);
	MSR_NZCV_x(REG_WORK1);

	if (x) {
		int f = rmw(FLAGX);
		EOR_xxbit(f, f, 0);
		unlock2(f);
	}
}
MENDFUNC(2,jff_EORSR,(IM32 s, IM8 x))

/*
 * EXT
 * Operand Syntax: <ea>
 *
 * Operand Size: 16,32
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(1,jnf_EXT_b,(RW4 d))
{
	if (isconst(d)) {
		live.state[d].val = (uae_s32)(uae_s8)live.state[d].val;
		return;
	}

	d = rmw(d);

	SIGNED8_REG_2_REG(d, d);

	unlock2(d);
}
MENDFUNC(1,jnf_EXT_b,(RW4 d))

MIDFUNC(1,jnf_EXT_w,(RW4 d))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffff0000) | (((uae_s32)(uae_s8)live.state[d].val) & 0x0000ffff);
		return;
	}

	d = rmw(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	BFI_xxii(d, REG_WORK1, 0, 16);
	
	unlock2(d);
}
MENDFUNC(1,jnf_EXT_w,(RW4 d))

MIDFUNC(1,jnf_EXT_l,(RW4 d))
{
	if (isconst(d)) {
		live.state[d].val = (uae_s32)(uae_s16)live.state[d].val;
		return;
	}

	d = rmw(d);

	SIGNED16_REG_2_REG(d, d);

	unlock2(d);
}
MENDFUNC(1,jnf_EXT_l,(RW4 d))

MIDFUNC(1,jff_EXT_b,(RW4 d))
{
	if (isconst(d)) {
    uae_u8 tmp = (uae_u8)live.state[d].val;
		d = writereg(d);
		SIGNED8_IMM_2_REG(d, tmp);
	} else {
	  d = rmw(d);
	  SIGNED8_REG_2_REG(d, d);
	}

	TST_ww(d, d);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_EXT_b,(RW4 d))

MIDFUNC(1,jff_EXT_w,(RW4 d))
{
	if (isconst(d)) {
    uae_u8 tmp = (uae_u8)live.state[d].val;
	  d = writereg(d);
		SIGNED8_IMM_2_REG(REG_WORK1, tmp);
	} else {
    d = rmw(d);
    SIGNED8_REG_2_REG(REG_WORK1, d);
  }

	TST_ww(REG_WORK1, REG_WORK1);
	BFI_xxii(d, REG_WORK1, 0, 16);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_EXT_w,(RW4 d))

MIDFUNC(1,jff_EXT_l,(RW4 d))
{
  if(isconst(d)) {
		live.state[d].val = (uae_s32)(uae_s16)live.state[d].val;
    uae_u32 f = 0;
    if(((uae_s32)live.state[d].val) == 0)
      f |= (ARM_Z_FLAG >> 16);
    if(((uae_s32)live.state[d].val) < 0)
      f |= (ARM_N_FLAG >> 16);
  	MOV_xish(REG_WORK1, f, 16);
  	MSR_NZCV_x(REG_WORK1);

    flags_carry_inverted = false;
    return;
  }

  d = rmw(d);
  SIGNED16_REG_2_REG(d, d);
	TST_ww(d, d);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_EXT_l,(RW4 d))

/*
 * LSL
 * Operand Syntax: 	Dx, Dy
 * 					#<data>, Dy
 *					<ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set according to the last bit shifted out of the operand. Unaffected for a shift count of zero.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit shifted out of the operand. Cleared for a shift count of zero.
 *
 */
MIDFUNC(2,jnf_LSL_b_imm,(RW1 d, IM8 i))
{
  if(i) {
		if (isconst(d)) {
			live.state[d].val = (live.state[d].val & 0xffffff00) | ((live.state[d].val << i) & 0x000000ff);
			return;
		}
		
		INIT_REG_b(d);
	  
    if(i > 31)
      i = 31;
  	LSL_wwi(REG_WORK1, d, i);
	  BFI_wwii(d, REG_WORK1, 0, 8);
	
		unlock2(d);
 	}
}
MENDFUNC(2,jnf_LSL_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jnf_LSL_w_imm,(RW2 d, IM8 i))
{
  if(i) {
		if (isconst(d)) {
			live.state[d].val = (live.state[d].val & 0xffff0000) | ((live.state[d].val << i) & 0x0000ffff);
			return;
		}

		INIT_REG_w(d);
	  
    if(i > 31)
      i = 31;
  	LSL_wwi(REG_WORK1, d, i);
	  BFI_wwii(d, REG_WORK1, 0, 16);
	
		unlock2(d);
 	}
}
MENDFUNC(2,jnf_LSL_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jnf_LSL_l_imm,(RW4 d, IM8 i))
{
  if(i) {
		if (isconst(d)) {
			live.state[d].val = live.state[d].val << i;
			return;
		}

		d = rmw(d);
		
	  LSL_wwi(d, d, i);
		
		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSL_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jnf_LSL_b_reg,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSL_b_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_b(d, i);

  AND_ww3f(REG_WORK1, i);
	LSL_www(REG_WORK1, d, REG_WORK1);
  BFI_wwii(d, REG_WORK1, 0, 8);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSL_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jnf_LSL_w_reg,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSL_w_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_w(d, i);

	AND_ww3f(REG_WORK1, i);
	LSL_www(REG_WORK1, d, REG_WORK1);
  BFI_wwii(d, REG_WORK1, 0, 16);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSL_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jnf_LSL_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		if(i > 31)
			set_const(d, 0);
		else
			COMPCALL(jnf_LSL_l_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_l(d, i);

	AND_ww3f(REG_WORK1, i);
	LSL_www(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSL_l_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_LSL_b_imm,(RW1 d, IM8 i))
{
	if (i) {
		d = rmw(d);
    if(i >= 8)
      MOV_wi(REG_WORK3, 0);
    else
  		LSL_wwi(REG_WORK3, d, i + 24);
		TST_ww(REG_WORK3, REG_WORK3);
		
		if(i <= 8) {
      TBZ_wii(d, (8 - i), 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }    
    flags_carry_inverted = false;
  	DUPLICACTE_CARRY
		
    BFXIL_xxii(d, REG_WORK3, 24, 8);
	} else {
		d = readreg(d);
		LSL_wwi(REG_WORK3, d, 24);
		TST_ww(REG_WORK3, REG_WORK3);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSL_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jff_LSL_w_imm,(RW2 d, IM8 i))
{
	if (i) {
		d = rmw(d);
    if(i >= 16)
      MOV_wi(REG_WORK3, 0);
    else
		  LSL_wwi(REG_WORK3, d, i + 16);
		TST_ww(REG_WORK3, REG_WORK3);

		if(i <= 16) {
      TBZ_wii(d, (16 - i), 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }
    flags_carry_inverted = false;
		DUPLICACTE_CARRY

    BFXIL_xxii(d, REG_WORK3, 16, 16);
	} else {
		d = readreg(d);
		LSL_wwi(REG_WORK3, d, 16);
		TST_ww(REG_WORK3, REG_WORK3);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSL_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jff_LSL_l_imm,(RW4 d, IM8 i))
{
	if (i) {
	  d = rmw(d);
    if(i >= 32)
      MOV_wi(REG_WORK3, 0);
    else
		  LSL_wwi(REG_WORK3, d, i);
		TST_ww(REG_WORK3, REG_WORK3);

    if(i <= 32) {
      TBZ_wii(d, (32 - i), 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
		MOV_ww(d, REG_WORK3);
	} else {
		d = readreg(d);
		TST_ww(d, d);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSL_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jff_LSL_b_reg,(RW1 d, RR4 i))
{
  if (isconst(i)) {
    COMPCALL(jff_LSL_b_imm)(d, live.state[i].val & 0x3f);
    return;
  }
  
	INIT_REGS_b(d, i);

	LSL_wwi(REG_WORK3, d, 24);
	ANDS_ww3f(REG_WORK1, i);
  uae_u32* branchadd = (uae_u32*)get_target();
	BEQ_i(0);               // No shift -> X flag unchanged, C cleared
	
  // shift count > 0
  LSL_xxx(REG_WORK2, REG_WORK3, REG_WORK1);
  BFXIL_xxii(d, REG_WORK2, 24, 8);  // result is ready
  TST_ww(REG_WORK2, REG_WORK2);     // NZ correct, VC cleared
	
	// Calculate C Flag
  TBZ_xii(REG_WORK2, 32, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY
	B_i(2);

  // No shift
  write_jmp_target(branchadd, (uintptr)get_target());
	TST_ww(REG_WORK3, REG_WORK3);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSL_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_LSL_w_reg,(RW2 d, RR4 i))
{
  if (isconst(i)) {
    COMPCALL(jff_LSL_w_imm)(d, live.state[i].val & 0x3f);
    return;
  }

	INIT_REGS_w(d, i);

	LSL_wwi(REG_WORK3, d, 16);
	ANDS_ww3f(REG_WORK1, i);
  uae_u32* branchadd = (uae_u32*)get_target();
	BEQ_i(0);               // No shift -> X flag unchanged, C cleared

	LSL_xxx(REG_WORK2, REG_WORK3, REG_WORK1);
  BFXIL_xxii(d, REG_WORK2, 16, 16); // result is ready
  TST_ww(REG_WORK2, REG_WORK2);     // NZ correct, VC cleared
	
	// Calculate C Flag
  TBZ_xii(REG_WORK2, 32, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY
	B_i(2);

  // No shift
  write_jmp_target(branchadd, (uintptr)get_target());
	TST_ww(REG_WORK3, REG_WORK3);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSL_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jff_LSL_l_reg,(RW4 d, RR4 i))
{
  if (isconst(i)) {
    COMPCALL(jff_LSL_l_imm)(d, live.state[i].val & 0x3f);
    return;
  }

	INIT_REGS_l(d, i);

	ANDS_ww3f(REG_WORK1, i);
  uae_u32* branchadd = (uae_u32*)get_target();
	BEQ_i(0);               // No shift -> X flag unchanged, C cleared

	LSL_xxx(d, d, REG_WORK1);
  TST_ww(d, d);     // NZ correct, VC cleared
	
	// Calculate C Flag
  TBZ_xii(d, 32, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY
	B_i(2);

  // No shift
  write_jmp_target(branchadd, (uintptr)get_target());
	TST_ww(d, d);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSL_l_reg,(RW4 d, RR4 i))

/*
 * LSLW
 * Operand Syntax: 	<ea>
 *
 * Operand Size: 16
 *
 * X Set according to the last bit shifted out of the operand. Unaffected for a shift count of zero.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit shifted out of the operand. Cleared for a shift count of zero.
 *
 * Target is never a register.
 */
MIDFUNC(1,jnf_LSLW,(RW2 d))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val << 1) & 0xffff;
		return;
	}

	d = rmw(d);

	LSL_wwi(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_LSLW,(RW2 d))

MIDFUNC(1,jff_LSLW,(RW2 d))
{
	d = rmw(d);

  LSL_wwi(REG_WORK1, d, 17);
  TST_ww(REG_WORK1, REG_WORK1);

  // Calculate C flag
  TBZ_wii(d, 15, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

	LSL_wwi(d, d, 1);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_LSLW,(RW2 d))

/*
 * LSR
 * Operand Syntax: 	Dx, Dy
 * 					#<data>, Dy
 *					<ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set according to the last bit shifted out of the operand. Unaffected for a shift count of zero.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit shifted out of the operand. Cleared for a shift count of zero.
 *
 */
MIDFUNC(2,jnf_LSR_b_imm,(RW1 d, IM8 i))
{
	if(i) {
		if (isconst(d)) {
			live.state[d].val = (live.state[d].val & 0xffffff00) | ((live.state[d].val & 0xff) >> i);
			return;
		}

		INIT_REG_b(d);

		UNSIGNED8_REG_2_REG(REG_WORK1, d);
    if(i > 31)
      i = 31;
  	LSR_wwi(REG_WORK1, REG_WORK1, i);
	  BFI_wwii(d, REG_WORK1, 0, 8);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSR_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jnf_LSR_w_imm,(RW2 d, IM8 i))
{
	if(i) {
		if (isconst(d)) {
			live.state[d].val = (live.state[d].val & 0xffff0000) | ((live.state[d].val & 0x0000ffff) >> i);
			return;
		}

		INIT_REG_w(d);

		UNSIGNED16_REG_2_REG(REG_WORK1, d);
    if(i > 31)
      i = 31;
  	LSR_wwi(REG_WORK1, REG_WORK1, i);
	  BFI_wwii(d, REG_WORK1, 0, 16);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSR_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jnf_LSR_l_imm,(RW4 d, IM8 i))
{
	if(i) {
		if (isconst(d)) {
			live.state[d].val = live.state[d].val >> i;
			return;
		}

		d = rmw(d);

  	LSR_wwi(d, d, i);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSR_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jff_LSR_b_imm,(RW1 d, IM8 i))
{
	if (i) {
		d = rmw(d);
    if(i >= 8)
      MOV_wi(REG_WORK1, 0);
    else {
    	UNSIGNED8_REG_2_REG(REG_WORK1, d);
		  LSR_wwi(REG_WORK1, REG_WORK1, i);
  	}
		TST_ww(REG_WORK1, REG_WORK1);

    if(i <= 8) {
      TBZ_wii(d, i-1, 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }
	  BFI_wwii(d, REG_WORK1, 0, 8);
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		d = readreg(d);
    SIGNED8_REG_2_REG(REG_WORK1, d);
		TST_ww(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSR_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jff_LSR_w_imm,(RW2 d, IM8 i))
{
	if (i) {
		d = rmw(d);
    if(i >= 16)
      MOV_wi(REG_WORK1, 0);
    else {
    	UNSIGNED16_REG_2_REG(REG_WORK1, d);
		  LSR_wwi(REG_WORK1, REG_WORK1, i);
  	}
		TST_ww(REG_WORK1, REG_WORK1);

    if(i <= 16) {
      TBZ_wii(d, i-1, 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }
	  BFI_wwii(d, REG_WORK1, 0, 16);
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		d = readreg(d);
	  SIGNED16_REG_2_REG(REG_WORK1, d);
		TST_ww(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSR_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jff_LSR_l_imm,(RW4 d, IM8 i))
{
	if (i) {
	  d = rmw(d);
	  MOV_ww(REG_WORK1, d);
		LSR_wwi(d, d, i);
		TST_ww(d, d);

    if(i <= 32) {
      TBZ_wii(REG_WORK1, i-1, 4);
      MRS_NZCV_x(REG_WORK4);
      SET_xxCflag(REG_WORK4, REG_WORK4);
      MSR_NZCV_x(REG_WORK4);
    }
    flags_carry_inverted = false;
		DUPLICACTE_CARRY
	} else {
		d = readreg(d);
		TST_ww(d, d);
    flags_carry_inverted = false;
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSR_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jnf_LSR_b_reg,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSR_b_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_b(d, i);

	UNSIGNED8_REG_2_REG(REG_WORK1, d);
	AND_ww3f(REG_WORK2, i);
	LSR_www(REG_WORK1, REG_WORK1, REG_WORK2);
  BFI_wwii(d, REG_WORK1, 0, 8);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jnf_LSR_w_reg,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSR_w_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_w(d, i);

	UNSIGNED16_REG_2_REG(REG_WORK1, d);
	AND_ww3f(REG_WORK2, i);
	LSR_www(REG_WORK1, REG_WORK1, REG_WORK2);
  BFI_wwii(d, REG_WORK1, 0, 16);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jnf_LSR_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		if(i > 31)
			set_const(d, 0);
		else
			COMPCALL(jnf_LSR_l_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_l(d, i);

	AND_ww3f(REG_WORK1, i);
	LSR_www(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSR_l_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_LSR_b_reg,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_LSR_b_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_b(d, i);

	ANDS_ww3f(REG_WORK1, i);
  uae_u32* branchadd = (uae_u32*)get_target();
	BEQ_i(0);                       // No shift -> X flag unchanged
	
	UNSIGNED8_REG_2_REG(REG_WORK3, d);
	LSR_www(REG_WORK2, REG_WORK3, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 8);
	TST_ww(REG_WORK2, REG_WORK2);
	
	// Calculate C Flag
	SUB_wwi(REG_WORK2, REG_WORK1, 1);
	LSR_www(REG_WORK2, REG_WORK3, REG_WORK2);
  TBZ_wii(REG_WORK2, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	B_i(3);

  // No shift
  write_jmp_target(branchadd, (uintptr)get_target());
	SIGNED8_REG_2_REG(REG_WORK2, d);        // Make sure, sign is in MSB if shift count is 0 (to get correct N flag)
	TST_ww(REG_WORK2, REG_WORK2);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_LSR_w_reg,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_LSR_w_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_w(d, i);

	ANDS_ww3f(REG_WORK1, i);
  uae_u32* branchadd = (uae_u32*)get_target();
	BEQ_i(0);                       // No shift -> X flag unchanged

	UXTH_ww(REG_WORK3, d);                  // Shift count is not 0 -> unsigned required
	LSR_www(REG_WORK2, REG_WORK3, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 16);
	TST_ww(REG_WORK2, REG_WORK2);
	
	// Calculate C Flag
	SUB_wwi(REG_WORK2, REG_WORK1, 1);
	LSR_www(REG_WORK2, REG_WORK3, REG_WORK2);
  TBZ_wii(REG_WORK2, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	B_i(3);

  // No shift
  write_jmp_target(branchadd, (uintptr)get_target());
	SIGNED16_REG_2_REG(REG_WORK2, d);       // Make sure, sign is in MSB if shift count is 0 (to get correct N flag)
	TST_ww(REG_WORK2, REG_WORK2);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jff_LSR_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_LSR_l_imm)(d, live.state[i].val & 0x3f);
		return;
	}

	INIT_REGS_l(d, i);

	ANDS_ww3f(REG_WORK1, i);
  uae_u32* branchadd = (uae_u32*)get_target();
	BEQ_i(0);                       // No shift -> X flag unchanged

  MOV_ww(REG_WORK3, d);
	LSR_www(d, d, REG_WORK1);
	TST_ww(d, d);
	
	// Calculate C Flag
	SUB_wwi(REG_WORK2, REG_WORK1, 1);
	LSR_www(REG_WORK2, REG_WORK3, REG_WORK2);
  TBZ_wii(REG_WORK2, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	B_i(2);

  // No shift
  write_jmp_target(branchadd, (uintptr)get_target());
	TST_ww(d, d);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSR_l_reg,(RW4 d, RR4 i))

/*
 * LSRW
 * Operand Syntax: 	<ea>
 *
 * Operand Size: 16
 *
 * X Set according to the last bit shifted out of the operand. Unaffected for a shift count of zero.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit shifted out of the operand. Cleared for a shift count of zero.
 *
 * Target is never a register.
 */
MIDFUNC(1,jnf_LSRW,(RW2 d))
{
	if (isconst(d)) {
		live.state[d].val = ((live.state[d].val & 0xffff) >> 1);
		return;
	}
	
	d = rmw(d);

	UNSIGNED16_REG_2_REG(d, d);
	LSR_wwi(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_LSRW,(RW2 d))

MIDFUNC(1,jff_LSRW,(RW2 d))
{
	d = rmw(d);

	UNSIGNED16_REG_2_REG(REG_WORK3, d);
	LSR_wwi(d, REG_WORK3, 1);
  TST_ww(d, d);

	// Calculate C Flag
  TBZ_wii(REG_WORK3, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);
  
  flags_carry_inverted = false;
	DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_LSRW,(RW2 d))

/*
 * MOVE
 * Operand Syntax: <ea>, <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_MOVE_b_imm,(W1 d, IM8 s))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffffff00) | (s & 0x000000ff);
		return;
	}

	d = rmw(d);

	MOV_xi(REG_WORK1, s & 0xff);
	BFI_xxii(d, REG_WORK1, 0, 8);

	unlock2(d);
}
MENDFUNC(2,jnf_MOVE_b_imm,(W1 d, IM8 s))

MIDFUNC(2,jnf_MOVE_w_imm,(W2 d, IM16 s))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffff0000) | (s & 0x0000ffff);
		return;
	}

	d = rmw(d);

	MOVK_xi(d, s & 0xffff);

	unlock2(d);
}
MENDFUNC(2,jnf_MOVE_w_imm,(W2 d, IM16 s))

MIDFUNC(2,jnf_MOVE_b,(W1 d, RR1 s))
{
	if(s == d)
		return;
	if (isconst(s)) {
		COMPCALL(jnf_MOVE_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

  BFI_xxii(d, s, 0, 8);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MOVE_b,(W1 d, RR1 s))

MIDFUNC(2,jnf_MOVE_w,(W2 d, RR2 s))
{
	if(s == d)
		return;
	if (isconst(s)) {
		COMPCALL(jnf_MOVE_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

  BFI_xxii(d, s, 0, 16);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MOVE_w,(W2 d, RR2 s))

MIDFUNC(2,jnf_MOVE_l,(W4 d, RR4 s))
{
	mov_l_rr(d, s);
}
MENDFUNC(2,jnf_MOVE_l,(W4 d, RR4 s))

MIDFUNC(2,jff_MOVE_b_imm,(W1 d, IM8 s))
{
	d = rmw(d);

	if (s & 0x80) {
		MOVN_wi(REG_WORK1, (uae_u8) ~s);
	} else {
		MOV_xi(REG_WORK1, (uae_u8) s);
	}
	TST_ww(REG_WORK1, REG_WORK1);
  BFI_xxii(d, REG_WORK1, 0, 8);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_MOVE_b_imm,(W1 d, IM8 s))

MIDFUNC(2,jff_MOVE_w_imm,(W2 d, IM16 s))
{
  if(isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffff0000) | (s & 0x0000ffff);
    uae_u32 f = 0;
    if((uae_s16)s == 0)
      f |= (ARM_Z_FLAG >> 16);
    if((uae_s16)s < 0)
      f |= (ARM_N_FLAG >> 16);
  	MOV_xish(REG_WORK1, f, 16);
  	MSR_NZCV_x(REG_WORK1);
    flags_carry_inverted = false;
    return;
  }

	d = rmw(d);

	SIGNED16_IMM_2_REG(REG_WORK1, (uae_u16)s);
	TST_ww(REG_WORK1, REG_WORK1);
  BFI_xxii(d, REG_WORK1, 0, 16);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_MOVE_w_imm,(W2 d, IM16 s))

MIDFUNC(2,jff_MOVE_l_imm,(W4 d, IM32 s))
{
  set_const(d, s);
  uae_u32 f = 0;
  if(s == 0)
    f |= (ARM_Z_FLAG >> 16);
  if((uae_s32)s < 0)
    f |= (ARM_N_FLAG >> 16);
	MOV_xish(REG_WORK1, f, 16);
	MSR_NZCV_x(REG_WORK1);

  flags_carry_inverted = false;
}
MENDFUNC(2,jff_MOVE_l_imm,(W4 d, IM32 s))

MIDFUNC(2,jff_MOVE_b,(W1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_MOVE_b_imm)(d, live.state[s].val);
		return;
	}

	int s_is_d = (s == d);
	if(!s_is_d) {
		s = readreg(s);
	  d = rmw(d);
  } else {
		s = d = readreg(d);
  }

	SIGNED8_REG_2_REG(REG_WORK1, s);
	TST_ww(REG_WORK1, REG_WORK1);
	if(!s_is_d)
    BFI_xxii(d, REG_WORK1, 0, 8);

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MOVE_b,(W1 d, RR1 s))

MIDFUNC(2,jff_MOVE_w,(W2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_MOVE_w_imm)(d, live.state[s].val);
		return;
	}

	int s_is_d = (s == d);
	if(!s_is_d) {
		s = readreg(s);
	  d = rmw(d);
  } else {
		s = d = readreg(d);
  }

	SIGNED16_REG_2_REG(REG_WORK1, s);
	TST_ww(REG_WORK1, REG_WORK1);
  if(!s_is_d)
    BFI_xxii(d, REG_WORK1, 0, 16);

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MOVE_w,(W2 d, RR2 s))

MIDFUNC(2,jff_MOVE_l,(W4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_MOVE_l_imm)(d, live.state[s].val);
		return;
	}

	int s_is_d = (s == d);
	s = readreg(s);
	
	if(!s_is_d) {
	  d = writereg(d);
	  MOV_ww(d, s);
	}
  TST_ww(s, s);

  flags_carry_inverted = false;
	if(!s_is_d)
	  unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_MOVE_l,(W4 d, RR4 s))

/*
 * MVMEL
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(3,jnf_MVMEL_w,(W4 d, RR4 s, IM8 offset))
{
	s = readreg(s);
	d = writereg(d);

	LDRH_wXi(REG_WORK1, s, offset);
  REV16_ww(d, REG_WORK1);
  SXTH_ww(d, d);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,jnf_MVMEL_w,(W4 d, RR4 s, IM8 offset))

MIDFUNC(3,jnf_MVMEL_l,(W4 d, RR4 s, IM8 offset))
{
	if (s == d || isconst(d) || isinreg(d)) {
	  s = readreg(s);
	  d = writereg(d);

	  LDR_wXi(REG_WORK1, s, offset);
    REV32_xx(d, REG_WORK1);

	  unlock2(d);
	  unlock2(s);
  } else {
  	s = readreg(s);
  
  	LDR_wXi(REG_WORK1, s, offset);
    REV32_xx(REG_WORK2, REG_WORK1);
    uintptr idx = (uintptr)(&regs.regs[d]) - (uintptr) &regs;
    STR_wXi(REG_WORK2, R_REGSTRUCT, idx);
  
  	unlock2(s);
  }
}
MENDFUNC(3,jnf_MVMEL_l,(W4 d, RR4 s, IM8 offset))

/*
 * MVMLE
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(3,jnf_MVMLE_w,(RR4 d, RR4 s, IM8 offset))
{
	s = readreg(s);
	d = readreg(d);

	REV16_ww(REG_WORK1, s);
  STURH_wXi(REG_WORK1, d, offset);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,jnf_MVMLE_w,(RR4 d, RR4 s, IM8 offset))

MIDFUNC(3,jnf_MVMLE_l,(RR4 d, RR4 s, IM8 offset))
{
	if (s == d || isconst(s) || isinreg(s)) {
	  s = readreg(s);
	  d = readreg(d);

	  REV32_xx(REG_WORK1, s);
    STUR_wXi(REG_WORK1, d, offset);

	  unlock2(d);
	  unlock2(s);
  } else {
  	d = readreg(d);

    uintptr idx = (uintptr)(&regs.regs[s]) - (uintptr) &regs;
    LDR_wXi(REG_WORK2, R_REGSTRUCT, idx);
  	REV32_xx(REG_WORK1, REG_WORK2);
    STUR_wXi(REG_WORK1, d, offset);

  	unlock2(d);
  }
}
MENDFUNC(3,jnf_MVMLE_l,(RR4 d, RR4 s, IM8 offset))

/*
 * MOVE16
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(2,jnf_MOVE16,(RR4 d, RR4 s))
{
	s = readreg(s);
	d = readreg(d);

	CLEAR_LOW4_xx(REG_WORK3, s);
	CLEAR_LOW4_xx(REG_WORK4, d);

	ADD_www(REG_WORK3, REG_WORK3, R_MEMSTART);
	ADD_www(REG_WORK4, REG_WORK4, R_MEMSTART);

	LDP_xxXi(REG_WORK1, REG_WORK2, REG_WORK3, 0);
	STP_xxXi(REG_WORK1, REG_WORK2, REG_WORK4, 0);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_MOVE16,(RR4 d, RR4 s))

/*
 * MOVEA
 * Operand Syntax: 	<ea>, An
 *
 * Operand Size: 16,32
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(2,jnf_MOVEA_w_imm,(W4 d, IM16 v))
{
  set_const(d, (uae_s32)(uae_s16)v);
}
MENDFUNC(2,jnf_MOVEA_w_imm,(W4 d, IM16 v))

MIDFUNC(2,jnf_MOVEA_w,(W4 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_MOVEA_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);
	
	SIGNED16_REG_2_REG(d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MOVEA_w,(W4 d, RR2 s))

MIDFUNC(2,jnf_MOVEA_l,(W4 d, RR4 s))
{
	mov_l_rr(d, s);
}
MENDFUNC(2,jnf_MOVEA_l,(W4 d, RR4 s))

/*
 * MULS
 * Operand Syntax: 	<ea>, Dn
 *
 * Operand Size: 16
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if overflow. Cleared otherwise. (32 Bit multiply only)
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_MULS,(RW4 d, RR4 s))
{
  if (isconst(s)) {
    uae_s16 tmp = (uae_s16)live.state[s].val;
    d = rmw(d);
    SIGNED16_IMM_2_REG(REG_WORK1, tmp);
    SIGNED16_REG_2_REG(d, d);
    SMULL_xww(d, d, REG_WORK1);
    unlock2(d);
    return; 
  }
    
	INIT_REGS_l(d, s);
  
  SIGNED16_REG_2_REG(d, d);
  SIGNED16_REG_2_REG(REG_WORK1, s);
  SMULL_xww(d, d, REG_WORK1);
  
	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULS,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULS,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	SIGNED16_REG_2_REG(d, d);
	SIGNED16_REG_2_REG(REG_WORK1, s);
	SMULL_xww(d, d, REG_WORK1);
  TST_ww(d, d);
  
  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULS,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULS32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	SMULL_xww(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULS32,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULS32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

  SMULL_xww(d, d, s);
  TST_ww(d, d);

  if (needed_flags & FLAG_V) {
    LSR_xxi(REG_WORK1, d, 32);
    CBZ_wi(REG_WORK1, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxVflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULS32,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULS64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

  SMULL_xww(d, d, s);
  LSR_xxi(s, d, 32);

	unlock2(s);
	unlock2(d);
}
MENDFUNC(2,jnf_MULS64,(RW4 d, RW4 s))

MIDFUNC(2,jff_MULS64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

  SXTW_xw(REG_WORK1, d);
  SXTW_xw(REG_WORK2, s);
  SMULL_xww(d, REG_WORK1, REG_WORK2);
  TST_xx(d, d);
  LSR_xxi(s, d, 32);

  if (needed_flags & FLAG_V) {
    // check overflow: no overflow if high part is 0 or 0xffffffff
    SMULH_xxx(REG_WORK3, REG_WORK1, REG_WORK2);
    CBZ_xi(REG_WORK3, 6);
    ADD_wwi(REG_WORK3, REG_WORK3, 1);
    CBZ_xi(REG_WORK3, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxVflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
    
  flags_carry_inverted = false;
	unlock2(s);
	unlock2(d);
}
MENDFUNC(2,jff_MULS64,(RW4 d, RW4 s))

/*
 * MULU
 * Operand Syntax: 	<ea>, Dn
 *
 * Operand Size: 16
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if overflow. Cleared otherwise. (32 Bit multiply only)
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_MULU,(RW4 d, RR4 s))
{
  if (isconst(s)) {
    uae_u16 tmp = (uae_u16)live.state[s].val;
    d = rmw(d);
    UNSIGNED16_IMM_2_REG(REG_WORK1, tmp);
    UNSIGNED16_REG_2_REG(d, d);
    UMULL_xww(d, d, REG_WORK1);
    unlock2(d);
    return; 
  }

	INIT_REGS_l(d, s);

	UNSIGNED16_REG_2_REG(d, d);
	UNSIGNED16_REG_2_REG(REG_WORK1, s);
	UMULL_xww(d, d, REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULU,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULU,(RW4 d, RR4 s))
{
  if (isconst(s)) {
    uae_u16 tmp = (uae_u16)live.state[s].val;
    d = rmw(d);
    UNSIGNED16_IMM_2_REG(REG_WORK1, tmp);
    UNSIGNED16_REG_2_REG(d, d);
    UMULL_xww(d, d, REG_WORK1);
    TST_ww(d, d);
    flags_carry_inverted = false;
    unlock2(d);
    return; 
  }

	INIT_REGS_l(d, s);

	UNSIGNED16_REG_2_REG(d, d);
	UNSIGNED16_REG_2_REG(REG_WORK1, s);
	UMULL_xww(d, d, REG_WORK1);
  TST_ww(d, d);

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULU,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULU32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	UMULL_xww(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULU32,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULU32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

  UMULL_xww(d, d, s);
  TST_ww(d, d);

  if (needed_flags & FLAG_V) {
    LSR_xxi(REG_WORK1, d, 32);
    CBZ_wi(REG_WORK1, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxVflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULU32,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULU64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

  UMULL_xww(d, d, s);
  LSR_xxi(s, d, 32);

	unlock2(s);
	unlock2(d);
}
MENDFUNC(2,jnf_MULU64,(RW4 d, RW4 s))

MIDFUNC(2,jff_MULU64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

  if (needed_flags & FLAG_V) {
    MOV_ww(REG_WORK1, d);
    MOV_ww(REG_WORK2, s);
    UMULL_xww(d, REG_WORK1, REG_WORK2);
  } else {
    UMULL_xww(d, d, s);
  }
  TST_xx(d, d);
  LSR_xxi(s, d, 32);

  if (needed_flags & FLAG_V) {
    // check overflow: no overflow if high part is 0
    UMULH_xxx(REG_WORK3, REG_WORK1, REG_WORK2);
    CBZ_xi(REG_WORK3, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxVflag(REG_WORK4, REG_WORK4);
    MSR_NZCV_x(REG_WORK4);
  }
	
  flags_carry_inverted = false;
	unlock2(s);
	unlock2(d);
}
MENDFUNC(2,jff_MULU64,(RW4 d, RW4 s))

/*
 * NEG
 * Operand Syntax: <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set the same as the carry bit.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if an overflow occurs. Cleared otherwise.
 * C Set if a borrow occurs. Cleared otherwise.
 *
 */
MIDFUNC(1,jnf_NEG_b,(RW1 d))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	NEG_ww(REG_WORK1, REG_WORK1);
  BFI_xxii(d, REG_WORK1, 0, 8);

	unlock2(d);
}
MENDFUNC(1,jnf_NEG_b,(RW1 d))

MIDFUNC(1,jnf_NEG_w,(RW2 d))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	NEG_ww(REG_WORK1, REG_WORK1);
  BFI_xxii(d, REG_WORK1, 0, 16);

	unlock2(d);
}
MENDFUNC(1,jnf_NEG_w,(RW2 d))

MIDFUNC(1,jnf_NEG_l,(RW4 d))
{
	d = rmw(d);

	NEG_ww(d, d);

	unlock2(d);
}
MENDFUNC(1,jnf_NEG_l,(RW4 d))

MIDFUNC(1,jff_NEG_b,(RW1 d))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	NEGS_ww(REG_WORK1, REG_WORK1);
  BFI_xxii(d, REG_WORK1, 0, 8);
  
  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_NEG_b,(RW1 d))

MIDFUNC(1,jff_NEG_w,(RW2 d))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	NEGS_ww(REG_WORK1, REG_WORK1);
  BFI_xxii(d, REG_WORK1, 0, 16);

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_NEG_w,(RW2 d))

MIDFUNC(1,jff_NEG_l,(RW4 d))
{
	d = rmw(d);

	NEGS_ww(d, d);

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(1,jff_NEG_l,(RW4 d))

/*
 * NEGX
 * Operand Syntax: <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set the same as the carry bit.
 * N Set if the result is negative. Cleared otherwise.
 * Z Cleared if the result is nonzero; unchanged otherwise.
 * V Set if an overflow occurs. Cleared otherwise.
 * C Set if a borrow occurs. Cleared otherwise.
 *
 * Attention: Z is cleared only if the result is nonzero. Unchanged otherwise
 *
 */
MIDFUNC(1,jnf_NEGX_b,(RW1 d))
{
  int x = readreg(FLAGX);
	INIT_REG_b(d);
  clobber_flags();
  
	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	NGC_ww(REG_WORK1, REG_WORK1);
	BFI_xxii(d, REG_WORK1, 0, 8);

	unlock2(d);
	unlock2(x);
}
MENDFUNC(1,jnf_NEGX_b,(RW1 d))

MIDFUNC(1,jnf_NEGX_w,(RW2 d))
{
  int x = readreg(FLAGX);
	INIT_REG_w(d);

  clobber_flags();

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	NGC_ww(REG_WORK1, REG_WORK1);
	BFI_xxii(d, REG_WORK1, 0, 16);

	unlock2(d);
	unlock2(x);
}
MENDFUNC(1,jnf_NEGX_w,(RW2 d))

MIDFUNC(1,jnf_NEGX_l,(RW4 d))
{
  int x = readreg(FLAGX);
	d = rmw(d);

  clobber_flags();

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	NGC_ww(d, d);

	unlock2(d);
	unlock2(x);
}
MENDFUNC(1,jnf_NEGX_l,(RW4 d))

MIDFUNC(1,jff_NEGX_b,(RW1 d))
{
	INIT_REG_b(d);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK2, 0);
	MOVN_xish(REG_WORK1, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK1, REG_WORK2, NATIVE_CC_NE);

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	NGCS_ww(REG_WORK1, REG_WORK1);
  BFI_wwii(d, REG_WORK1, 0, 8);
  
	MRS_NZCV_x(REG_WORK4);
	EOR_xxCflag(REG_WORK4, REG_WORK4);
	AND_www(REG_WORK4, REG_WORK4, REG_WORK2);
	MSR_NZCV_x(REG_WORK4);
  flags_carry_inverted = false;
	if (needed_flags & FLAG_X)
  	UBFX_wwii(x, REG_WORK4, 29, 1); // Duplicate carry

	unlock2(x);
	unlock2(d);
}
MENDFUNC(1,jff_NEGX_b,(RW1 d))

MIDFUNC(1,jff_NEGX_w,(RW2 d))
{
	INIT_REG_w(d);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK2, 0);
	MOVN_xish(REG_WORK1, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK1, REG_WORK2, NATIVE_CC_NE);

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	NGCS_ww(REG_WORK1, REG_WORK1);
  BFI_wwii(d, REG_WORK1, 0, 16);
  
	MRS_NZCV_x(REG_WORK4);
	EOR_xxCflag(REG_WORK4, REG_WORK4);
	AND_www(REG_WORK4, REG_WORK4, REG_WORK2);
	MSR_NZCV_x(REG_WORK4);
  flags_carry_inverted = false;
	if (needed_flags & FLAG_X)
  	UBFX_wwii(x, REG_WORK4, 29, 1); // Duplicate carry

	unlock2(x);
	unlock2(d);
}
MENDFUNC(1,jff_NEGX_w,(RW2 d))

MIDFUNC(1,jff_NEGX_l,(RW4 d))
{
	d = rmw(d);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK2, 0);
	MOVN_xish(REG_WORK1, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK1, REG_WORK2, NATIVE_CC_NE);

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	NGCS_ww(d, d);

	MRS_NZCV_x(REG_WORK4);
	EOR_xxCflag(REG_WORK4, REG_WORK4);
	AND_www(REG_WORK4, REG_WORK4, REG_WORK2);
	MSR_NZCV_x(REG_WORK4);
  flags_carry_inverted = false;
	if (needed_flags & FLAG_X)
  	UBFX_wwii(x, REG_WORK4, 29, 1); // Duplicate carry

	unlock2(x);
	unlock2(d);
}
MENDFUNC(1,jff_NEGX_l,(RW4 d))

/*
 * NOT
 * Operand Syntax: <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(1,jnf_NOT_b,(RW1 d))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffffff00) | ((~live.state[d].val) & 0x000000ff);
		return;
	}

	INIT_REG_b(d);

	if(targetIsReg) {
		MVN_ww(REG_WORK1, d);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		MVN_ww(d, d);
	}

	unlock2(d);
}
MENDFUNC(1,jnf_NOT_b,(RW1 d))

MIDFUNC(1,jnf_NOT_w,(RW2 d))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffff0000) | ((~live.state[d].val) & 0x0000ffff);
		return;
	}

	INIT_REG_w(d);

	if(targetIsReg) {
		MVN_ww(REG_WORK1, d);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		MVN_ww(d, d);
	}

	unlock2(d);
}
MENDFUNC(1,jnf_NOT_w,(RW2 d))

MIDFUNC(1,jnf_NOT_l,(RW4 d))
{
	if (isconst(d)) {
		live.state[d].val = ~live.state[d].val;
		return;
	}

	d = rmw(d);

	MVN_ww(d, d);

	unlock2(d);
}
MENDFUNC(1,jnf_NOT_l,(RW4 d))

MIDFUNC(1,jff_NOT_b,(RW1 d))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		MVN_ww(REG_WORK1, REG_WORK1);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	  TST_ww(REG_WORK1, REG_WORK1);
	} else {
		MVN_ww(d, REG_WORK1);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_NOT_b,(RW1 d))

MIDFUNC(1,jff_NOT_w,(RW2 d))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		MVN_ww(REG_WORK1, REG_WORK1);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	  TST_ww(REG_WORK1, REG_WORK1);
	} else {
		MVN_ww(d, REG_WORK1);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_NOT_w,(RW2 d))

MIDFUNC(1,jff_NOT_l,(RW4 d))
{
	d = rmw(d);

	MVN_ww(d, d);
	TST_ww(d, d);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_NOT_l,(RW4 d))

/*
 * OR
 * Operand Syntax: 	<ea>, Dn
 *  				Dn, <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(2,jnf_OR_b_imm,(RW1 d, IM8 v))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffffff00) | ((live.state[d].val | v) & 0x000000ff);
		return;
	}

	INIT_REG_b(d);
	
  MOV_xi(REG_WORK2, v & 0xff);
  ORR_www(d, d, REG_WORK2);

	unlock2(d);
}
MENDFUNC(2,jnf_OR_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jnf_OR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_OR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		ORR_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		ORR_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_OR_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_OR_w_imm,(RW2 d, IM16 v))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffff0000) | ((live.state[d].val | v) & 0x0000ffff);
		return;
	}

	INIT_REG_w(d);

  MOV_xi(REG_WORK1, v & 0xffff);
  ORR_www(d, d, REG_WORK1);
	
	unlock2(d);
}
MENDFUNC(2,jnf_OR_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jnf_OR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_OR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		ORR_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		ORR_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_OR_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_OR_l_imm,(RW4 d, IM32 v))
{
	if (isconst(d)) {
		live.state[d].val = live.state[d].val | v;
		return;
	}

	d = rmw(d);

  LOAD_U32(REG_WORK1, v);
	ORR_www(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_OR_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jnf_OR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_OR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ORR_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_OR_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_OR_b_imm,(RW1 d, IM8 v))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_IMM_2_REG(REG_WORK2, v);
	if(targetIsReg) {
		ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
		TST_ww(REG_WORK1, REG_WORK1);
		BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		ORR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_OR_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jff_OR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_OR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_REG_2_REG(REG_WORK2, s);
	if(targetIsReg) {
		ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
		TST_ww(REG_WORK1, REG_WORK1);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		ORR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_OR_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_OR_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_IMM_2_REG(REG_WORK2, v);
	if(targetIsReg) {
		ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
		TST_ww(REG_WORK1, REG_WORK1);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		ORR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_OR_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jff_OR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_OR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_REG_2_REG(REG_WORK2, s);
	if(targetIsReg) {
		ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
		TST_ww(REG_WORK1, REG_WORK1);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else {
		ORR_www(d, REG_WORK1, REG_WORK2);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_OR_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_OR_l_imm,(RW4 d, IM32 v))
{
	d = rmw(d);

  LOAD_U32(REG_WORK1, v);
	ORR_www(d, d, REG_WORK1);
  TST_ww(d, d);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_OR_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jff_OR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_OR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ORR_www(d, d, s);
	TST_ww(d, d);

  flags_carry_inverted = false;
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_OR_l,(RW4 d, RR4 s))

/*
 * ORSR
 * Operand Syntax: 	#<data>, CCR
 *
 * Operand Size: 8
 *
 * X — Set if bit 4 of immediate operand is one; unchanged otherwise.
 * N — Set if bit 3 of immediate operand is one; unchanged otherwise.
 * Z — Set if bit 2 of immediate operand is one; unchanged otherwise.
 * V — Set if bit 1 of immediate operand is one; unchanged otherwise.
 * C — Set if bit 0 of immediate operand is one; unchanged otherwise.
 *
 */
MIDFUNC(1,jff_ORSR,(IM32 s, IM8 x))
{
	MRS_NZCV_x(REG_WORK1);
  if (flags_carry_inverted) {
    EOR_xxCflag(REG_WORK1, REG_WORK1);
    flags_carry_inverted = false;
  }
	MOV_xish(REG_WORK2, (s >> 16), 16);
	ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
	MSR_NZCV_x(REG_WORK1);

	if (x) {
		int f = writereg(FLAGX);
		MOV_wi(f, 1);
		unlock2(f);
	}
}
MENDFUNC(2,jff_ORSR,(IM32 s, IM8 x))

/*
 * ROL
 * Operand Syntax: 	Dx, Dy
 * 					#<data>, Dy
 *					<ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit rotated out of the operand. Cleared when the rotate count is zero.
 *
 */
MIDFUNC(2,jnf_ROL_b_imm,(RW1 d, IM8 i))
{
  if(i & 0x1f) {
		INIT_REG_b(d);

  	LSL_wwi(REG_WORK1, d, 24);
  	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
  	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
  	ROR_wwi(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
	  BFI_wwii(d, REG_WORK1, 0, 8);
  
		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROL_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jnf_ROL_w_imm,(RW2 d, IM8 i))
{
  if(i & 0x1f) {
		INIT_REG_w(d);

    MOV_ww(REG_WORK1, d);
  	BFI_wwii(REG_WORK1, REG_WORK1, 16, 16);
  	ROR_wwi(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
	  BFI_wwii(d, REG_WORK1, 0, 16);

		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROL_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jnf_ROL_l_imm,(RW4 d, IM8 i))
{
  if(i & 0x1f) {
    if (isconst(d)) {
      i = i & 31;
      live.state[d].val = (live.state[d].val << i) | (live.state[d].val >> (32-i));
      return;
    }

		d = rmw(d);

  	ROR_wwi(d, d, (32 - (i & 0x1f)));
	
		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROL_l_imm,(RW4 d, RR4 s, IM8 i))

MIDFUNC(2,jff_ROL_b_imm,(RW1 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	LSL_wwi(REG_WORK1, d, 24);
	if (i) {
  	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
  	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
		if(i & 0x1f) {
		  ROR_wwi(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
		}
    TST_ww(REG_WORK1, REG_WORK1);     
	  BFI_wwii(d, REG_WORK1, 0, 8);

		MRS_NZCV_x(REG_WORK4);
		BFI_wwii(REG_WORK4, REG_WORK1, 29, 1); // Handle C flag
		MSR_NZCV_x(REG_WORK4);
	} else {
		TST_ww(REG_WORK1, REG_WORK1);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_ROL_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jff_ROL_w_imm,(RW2 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

  MOV_ww(REG_WORK1, d);
	BFI_wwii(REG_WORK1, REG_WORK1, 16, 16);
	if (i) {
		if(i & 0x1f)
  		ROR_wwi(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
    TST_ww(REG_WORK1, REG_WORK1);
	  BFI_wwii(d, REG_WORK1, 0, 16);

		MRS_NZCV_x(REG_WORK4);
		BFI_wwii(REG_WORK4, REG_WORK1, 29, 1); // Handle C flag
		MSR_NZCV_x(REG_WORK4);
	} else {
		TST_ww(REG_WORK1, REG_WORK1);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_ROL_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jff_ROL_l_imm,(RW4 d, IM8 i))
{
	if (i) {
		if(i & 0x1f) {
  	  d = rmw(d);
  		ROR_wwi(d, d, (32 - (i & 0x1f)));
  	} else {
  		d = readreg(d);
  	}
    TST_ww(d, d);

		MRS_NZCV_x(REG_WORK4);
		BFI_wwii(REG_WORK4, d, 29, 1); // Handle C flag
		MSR_NZCV_x(REG_WORK4);
	} else {
		d = readreg(d);
		TST_ww(d, d);
	}

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_ROL_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jnf_ROL_b,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROL_b_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_b(d, i);

	UBFIZ_xxii(REG_WORK1, i, 0, 5); // AND_rri(REG_WORK1, i, 0x1f);
	MOV_wi(REG_WORK2, 32);
	SUB_www(REG_WORK1, REG_WORK2, REG_WORK1);

	LSL_wwi(REG_WORK2, d, 24);
	ORR_wwwLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 8);
	ORR_wwwLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 16);
	ROR_www(REG_WORK2, REG_WORK2, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 8);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROL_b,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ROL_w,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROL_w_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_w(d, i);

	UBFIZ_xxii(REG_WORK1, i, 0, 5); // AND_rri(REG_WORK1, i, 0x1f);
	MOV_wi(REG_WORK2, 32);
	SUB_www(REG_WORK1, REG_WORK2, REG_WORK1);

  MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, REG_WORK2, 16, 16);
	ROR_www(REG_WORK2, REG_WORK2, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 16);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROL_w,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ROL_l,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROL_l_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_l(d, i);

	UBFIZ_xxii(REG_WORK1, i, 0, 5); // AND_rri(REG_WORK1, i, 0x1f);
	MOV_wi(REG_WORK2, 32);
	SUB_www(REG_WORK1, REG_WORK2, REG_WORK1);

	ROR_www(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROL_l,(RW4 d, RR4 i))

MIDFUNC(2,jff_ROL_b,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_ROL_b_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_b(d, i);

	UBFIZ_xxii(REG_WORK1, i, 0, 5); // AND_rri(REG_WORK1, i, 0x1f);
  CBNZ_wi(REG_WORK1, 4);

  // shift count is 0
	LSL_wwi(REG_WORK3, d, 24);
  TST_ww(REG_WORK3, REG_WORK3);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>
  
	MOV_wi(REG_WORK2, 32);
	SUB_www(REG_WORK1, REG_WORK2, REG_WORK1);

	LSL_wwi(REG_WORK2, d, 24);
	ORR_wwwLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 8);
	ORR_wwwLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 16);
	ROR_www(REG_WORK2, REG_WORK2, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 8);
  TST_ww(REG_WORK2, REG_WORK2);
  
	MRS_NZCV_x(REG_WORK4);
	BFI_wwii(REG_WORK4, d, 29, 1); // Handle C flag
	MSR_NZCV_x(REG_WORK4);

  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROL_b,(RW1 d, RR4 i))

MIDFUNC(2,jff_ROL_w,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_ROL_w_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_w(d, i);

	UBFIZ_xxii(REG_WORK1, i, 0, 5); // AND_rri(REG_WORK1, i, 0x1f);
  CBNZ_wi(REG_WORK1, 4);

  // shift count is 0
	LSL_wwi(REG_WORK3, d, 16);
  TST_ww(REG_WORK3, REG_WORK3);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>

	MOV_wi(REG_WORK2, 32);
	SUB_www(REG_WORK1, REG_WORK2, REG_WORK1);

  MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, REG_WORK2, 16, 16);
	ROR_www(REG_WORK2, REG_WORK2, REG_WORK1);
  BFI_wwii(d, REG_WORK2, 0, 16);
  TST_ww(REG_WORK2, REG_WORK2);
  
	MRS_NZCV_x(REG_WORK4);
	BFI_wwii(REG_WORK4, d, 29, 1); // Handle C flag
	MSR_NZCV_x(REG_WORK4);

  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROL_w,(RW2 d, RR4 i))

MIDFUNC(2,jff_ROL_l,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_ROL_l_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_l(d, i);

	UBFIZ_xxii(REG_WORK1, i, 0, 5); // AND_rri(REG_WORK1, i, 0x1f);
  CBNZ_wi(REG_WORK1, 3);

  // shift count is 0
  TST_ww(d, d);     // NZ correct, VC cleared
  uae_u32* branchadd = (uae_u32*)get_target();
  B_i(0); // <end>

	MOV_wi(REG_WORK2, 32);
	SUB_www(REG_WORK1, REG_WORK2, REG_WORK1);

	ROR_www(d, d, REG_WORK1);
  TST_ww(d, d);
  
	MRS_NZCV_x(REG_WORK4);
	BFI_wwii(REG_WORK4, d, 29, 1); // Handle C flag
	MSR_NZCV_x(REG_WORK4);

  // <end>
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ROL_l,(RW4 d, RR4 i))

/*
 * ROLW
 * Operand Syntax: 	<ea>
 *
 * Operand Size: 16
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit rotated out of the operand. Cleared when the rotate count is zero.
 *
 * Target is never a register.
 */
MIDFUNC(1,jnf_ROLW,(RW2 d))
{
	d = rmw(d);

	BFI_wwii(d, d, 16, 16);
	ROR_wwi(d, d, (32 - 1));

	unlock2(d);
}
MENDFUNC(1,jnf_ROLW,(RW2 d))

MIDFUNC(1,jff_ROLW,(RW2 d))
{
	d = rmw(d);

	BFI_wwii(d, d, 16, 16);
	ROR_wwi(d, d, (32 - 1));
  TST_ww(d, d);
  
	MRS_NZCV_x(REG_WORK4);
	BFI_wwii(REG_WORK4, d, 29, 1); // Handle C flag
	MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_ROLW,(RW2 d))

/*
 * ROXL
 * Operand Syntax: Dx, Dy
 * 				   #<data>, Dy
 *
 * Operand Size: 8,16,32
 *
 * X Set according to the last bit rotated out of the operand. Unchanged when the rotate count is zero.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit rotated out of the operand. Cleared when the rotate count is zero.
 *
 */
MIDFUNC(2,jnf_ROXL_b,(RW1 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_b(d, i);

	clobber_flags();

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 35);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 36);
	CMP_wi(REG_WORK1, 17);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 18);
	CMP_wi(REG_WORK1, 8);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 9);
  uae_u32* branchadd = (uae_u32*)get_target();
	CBZ_wi(REG_WORK1, 0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 8, 1);         // move x to left side of d
	BFI_wwii(REG_WORK2, REG_WORK2, 9, 9); // duplicate 9 bits
	
	MOV_wi(REG_WORK3, 9);
	SUB_www(REG_WORK3, REG_WORK3, REG_WORK1);
	LSR_www(REG_WORK2, REG_WORK2, REG_WORK3);
	
	BFI_wwii(d, REG_WORK2, 0, 8);
	
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXL_b,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ROXL_w,(RW2 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_w(d, i);

	clobber_flags();

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 33);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 34);
	CMP_wi(REG_WORK1, 16);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 17);
  uae_u32* branchadd = (uae_u32*)get_target();
	CBZ_wi(REG_WORK1, 0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 16, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 17, 17); // duplicate 17 bits

	MOV_wi(REG_WORK3, 17);
	SUB_www(REG_WORK3, REG_WORK3, REG_WORK1);
	LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
	
	BFI_wwii(d, REG_WORK2, 0, 16);

  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXL_w,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ROXL_l,(RW4 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_l(d, i);

	clobber_flags();
	
	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 32);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 33);
  uae_u32* branchadd = (uae_u32*)get_target();
	CBZ_wi(REG_WORK1, 0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_xxii(REG_WORK2, x, 32, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 33, 31); // duplicate 31 bits

	MOV_wi(REG_WORK3, 33);
	SUB_www(REG_WORK3, REG_WORK3, REG_WORK1);
	LSR_xxx(d, REG_WORK2, REG_WORK3);

  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXL_l,(RW4 d, RR4 i))

MIDFUNC(2,jff_ROXL_b,(RW1 d, RR4 i))
{
	INIT_REGS_b(d, i);
  int x = rmw(FLAGX);

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 35);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 36);
	CMP_wi(REG_WORK1, 17);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 18);
	CMP_wi(REG_WORK1, 8);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 9);
	CBNZ_wi(REG_WORK1, 4);			// need to rotate

	LSL_wwi(REG_WORK1, d, 24);
	TST_ww(REG_WORK1, REG_WORK1);
  uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 8, 1);         // move x to left side of d
	BFI_wwii(REG_WORK2, REG_WORK2, 9, 9); // duplicate 9 bits
	
	MOV_wi(REG_WORK3, 9);
	SUB_www(REG_WORK3, REG_WORK3, REG_WORK1);
	LSR_www(REG_WORK2, REG_WORK2, REG_WORK3);
	BFI_wwii(d, REG_WORK2, 0, 8);

  // Calculate NZ
	LSL_wwi(REG_WORK1, REG_WORK2, 24);
	TST_ww(REG_WORK1, REG_WORK1);
  
  // Calculate C: bit left of result
  MRS_NZCV_x(REG_WORK4);
  UBFX_wwii(x, REG_WORK2, 8, 1);
  BFI_wwii(REG_WORK4, x, 29, 1);
  MSR_NZCV_x(REG_WORK4);
  
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXL_b,(RW1 d, RR4 i))

MIDFUNC(2,jff_ROXL_w,(RW2 d, RR4 i))
{
	INIT_REGS_w(d, i);
  int x = rmw(FLAGX);

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 33);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 34);
	CMP_wi(REG_WORK1, 16);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 17);
	CBNZ_wi(REG_WORK1, 4);			// need to rotate

	LSL_wwi(REG_WORK1, d, 16);
	TST_ww(REG_WORK1, REG_WORK1);
  uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 16, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 17, 17); // duplicate 17 bits

	MOV_wi(REG_WORK3, 17);
	SUB_www(REG_WORK3, REG_WORK3, REG_WORK1);
	LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
	
	BFI_wwii(d, REG_WORK2, 0, 16);

  // Calculate NZ
	LSL_wwi(REG_WORK1, REG_WORK2, 16);
	TST_ww(REG_WORK1, REG_WORK1);
  
  // Calculate C: bit left of result
  MRS_NZCV_x(REG_WORK4);
  UBFX_wwii(x, REG_WORK2, 16, 1);
  BFI_wwii(REG_WORK4, x, 29, 1);
  MSR_NZCV_x(REG_WORK4);
	
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXL_w,(RW2 d, RR4 i))

MIDFUNC(2,jff_ROXL_l,(RW4 d, RR4 i))
{
	INIT_REGS_l(d, i);
  int x = rmw(FLAGX);

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 32);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 33);
	CBNZ_wi(REG_WORK1, 3);			// need to rotate

	TST_ww(d, d);
  uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_xxii(REG_WORK2, x, 32, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 33, 31); // duplicate 31 bits

	MOV_wi(REG_WORK3, 33);
	SUB_www(REG_WORK3, REG_WORK3, REG_WORK1);
	LSR_xxx(d, REG_WORK2, REG_WORK3);

  // Calculate NZ
	TST_ww(d, d);

  // Calculate C
  MRS_NZCV_x(REG_WORK4);
  SUB_wwi(REG_WORK3, REG_WORK3, 1);
  LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK3);
  UBFX_wwii(x, REG_WORK2, 0, 1);
  BFI_wwii(REG_WORK4, x, 29, 1);
  MSR_NZCV_x(REG_WORK4);

  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXL_l,(RW4 d, RR4 i))

/*
 * ROR
 * Operand Syntax: 	Dx, Dy
 * 					#<data>, Dy
 *					<ea>
 *
 * Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit rotated out of the operand. Cleared when the rotate count is zero.
 *
 */
MIDFUNC(2,jnf_ROR_b_imm,(RW1 d, IM8 i))
{
	if(i & 0x07) {
		INIT_REG_b(d);

  	MOV_ww(REG_WORK1, d);
  	BFI_wwii(REG_WORK1, REG_WORK1, 8, 8);
  	ROR_wwi(REG_WORK1, REG_WORK1, i & 0x07);
	  BFI_wwii(d, REG_WORK1, 0, 8);
  
		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROR_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jnf_ROR_w_imm,(RW2 d, IM8 i))
{
	if(i & 0x0f) {
		INIT_REG_w(d);

  	MOV_ww(REG_WORK1, d);
  	BFI_wwii(REG_WORK1, REG_WORK1, 16, 16);
  	ROR_wwi(REG_WORK1, REG_WORK1, i & 0x0f);
	  BFI_wwii(d, REG_WORK1, 0, 16);

		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROR_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jnf_ROR_l_imm,(RW4 d, IM8 i))
{
	if(i & 0x1f) {
	  if (isconst(d)) {
	    i = i & 31;
	    live.state[d].val = (live.state[d].val >> i) | (live.state[d].val << (32-i));
	    return;
	  }

		d = rmw(d);
	
		ROR_wwi(d, d, i & 31);
	
		unlock2(d);
	}
}
MENDFUNC(2,jnf_ROR_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jff_ROR_b_imm,(RW1 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	LSL_wwi(REG_WORK1, d, 24);
	if(i & 0x07) {
	  ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	  ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
  	ROR_wwi(REG_WORK1, REG_WORK1, i & 0x07);
	  BFI_wwii(d, REG_WORK1, 0, 8);
  }
  TST_ww(REG_WORK1, REG_WORK1);
  if(i) {
	  TBZ_wii(REG_WORK1, 31, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	  MSR_NZCV_x(REG_WORK4);
  }

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_ROR_b_imm,(RW1 d, IM8 i))

MIDFUNC(2,jff_ROR_w_imm,(RW2 d, IM8 i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	LSL_wwi(REG_WORK1, d, 16);
	if(i & 0x0f) {
	  ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
  	ROR_wwi(REG_WORK1, REG_WORK1, i & 0x0f);
	  BFI_wwii(d, REG_WORK1, 0, 16);
  }
  TST_ww(REG_WORK1, REG_WORK1);
  if (i) {
	  TBZ_wii(REG_WORK1, 31, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	  MSR_NZCV_x(REG_WORK4);
  }

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_ROR_w_imm,(RW2 d, IM8 i))

MIDFUNC(2,jff_ROR_l_imm,(RW4 d, IM8 i))
{
	if(i)
	  d = rmw(d);
	else
		d = readreg(d);

	if(i & 0x1f) {
	  ROR_wwi(d, d, i & 0x1f);
  } 
  TST_ww(d, d);
  if (i) {
	  TBZ_wii(d, 31, 4);
    MRS_NZCV_x(REG_WORK4);
    SET_xxCflag(REG_WORK4, REG_WORK4);
	  MSR_NZCV_x(REG_WORK4);
  }

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(2,jff_ROR_l_imm,(RW4 d, IM8 i))

MIDFUNC(2,jnf_ROR_b,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROR_b_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_b(d, i);

	LSL_wwi(REG_WORK1, d, 24);
	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	ROR_www(REG_WORK1, REG_WORK1, i);
  BFI_wwii(d, REG_WORK1, 0, 8);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROR_b,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ROR_w,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROR_w_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_w(d, i);

	LSL_wwi(REG_WORK1, d, 16);
	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	ROR_www(REG_WORK1, REG_WORK1, i);
  BFI_wwii(d, REG_WORK1, 0, 16);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROR_w,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ROR_l,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROR_l_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_l(d, i);

	ROR_www(d, d, i);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROR_l,(RW4 d, RR4 i))

MIDFUNC(2,jff_ROR_b,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_ROR_b_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_b(d, i);

	LSL_wwi(REG_WORK1, d, 24);
	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	ORR_wwwLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	AND_ww3f(REG_WORK2, i);
	ROR_www(REG_WORK1, REG_WORK1, REG_WORK2);
  BFI_wwii(d, REG_WORK1, 0, 8);
	TST_ww(REG_WORK1, REG_WORK1);
  
  CBZ_wi(REG_WORK2, 5); // C cleared if no shift
  TBZ_wii(REG_WORK1, 31, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROR_b,(RW1 d, RR4 i))

MIDFUNC(2,jff_ROR_w,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_ROR_w_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_w(d, i);

	LSL_wwi(REG_WORK1, d, 16);
	BFXIL_wwii(REG_WORK1, REG_WORK1, 16, 16);
	AND_ww3f(REG_WORK2, i);
	ROR_www(REG_WORK1, REG_WORK1, REG_WORK2);
  BFI_wwii(d, REG_WORK1, 0, 16);
	TST_ww(REG_WORK1, REG_WORK1);

  CBZ_wi(REG_WORK2, 5); // C cleared if no shift
  TBZ_wii(REG_WORK1, 31, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROR_w,(RW2 d, RR4 i))

MIDFUNC(2,jff_ROR_l,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jff_ROR_l_imm)(d, (uae_u8)live.state[i].val);
		return;
	}

	INIT_REGS_l(d, i);

	AND_ww3f(REG_WORK1, i);
	ROR_www(d, d, REG_WORK1);
	TST_ww(d, d);

  CBZ_wi(REG_WORK1, 5); // C cleared if no shift
  TBZ_wii(d, 31, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROR_l,(RW4 d, RR4 i))

/*
 * RORW
 * Operand Syntax: 	<ea>
 *
 * Operand Size: 16
 *
 * X Not affected.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit rotated out of the operand.
 *
 * Target is never a register.
 */
MIDFUNC(1,jnf_RORW,(RW2 d))
{
	d = rmw(d);

	BFI_wwii(d, d, 16, 16);
	ROR_wwi(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_RORW,(RW2 d))

MIDFUNC(1,jff_RORW,(RW2 d))
{
	d = rmw(d);

	BFI_wwii(d, d, 16, 16);
	ROR_wwi(d, d, 1);
	TST_ww(d, d);

  TBZ_wii(d, 31, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_RORW,(RW2 d))

/*
 * ROXR
 * Operand Syntax: Dx, Dy
 * 				   #<data>, Dy
 *
 * Operand Size: 8,16,32
 *
 * X Set according to the last bit rotated out of the operand. Unchanged when the rotate count is zero.
 * N Set if the most significant bit of the result is set. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Always cleared.
 * C Set according to the last bit rotated out of the operand. Cleared when the rotate count is zero.
 *
 */
MIDFUNC(2,jnf_ROXR_b,(RW1 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_b(d, i);

	clobber_flags();

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 35);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 36);
	CMP_wi(REG_WORK1, 17);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 18);
	CMP_wi(REG_WORK1, 8);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 9);
  uae_u32* branchadd = (uae_u32*)get_target();
	CBZ_wi(REG_WORK1, 0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 8, 1);         // move x to left side of d
	BFI_wwii(REG_WORK2, REG_WORK2, 9, 9); // duplicate 9 bits
	
	LSR_www(REG_WORK2, REG_WORK2, REG_WORK1);
	BFI_wwii(d, REG_WORK2, 0, 8);
	
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXR_b,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ROXR_w,(RW2 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_w(d, i);

	clobber_flags();

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 33);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 34);
	CMP_wi(REG_WORK1, 16);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 17);
  uae_u32* branchadd = (uae_u32*)get_target();
	CBZ_wi(REG_WORK1, 0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 16, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 17, 17); // duplicate 17 bits

	LSR_xxx(REG_WORK2, REG_WORK2, REG_WORK1);
	BFI_wwii(d, REG_WORK2, 0, 16);
	
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXR_w,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ROXR_l,(RW4 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_l(d, i);

	clobber_flags();
	
	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 32);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 33);
  uae_u32* branchadd = (uae_u32*)get_target();
	CBZ_wi(REG_WORK1, 0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_xxii(REG_WORK2, x, 32, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 33, 31); // duplicate 31 bits

	LSR_xxx(d, REG_WORK2, REG_WORK1);

  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXR_l,(RW4 d, RR4 i))

MIDFUNC(2,jff_ROXR_b,(RW1 d, RR4 i))
{
	INIT_REGS_b(d, i);
  int x = rmw(FLAGX);

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 35);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 36);
	CMP_wi(REG_WORK1, 17);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 18);
	CMP_wi(REG_WORK1, 8);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 9);
	CBNZ_wi(REG_WORK1, 4);			// need to rotate

	LSL_wwi(REG_WORK1, d, 24);
	TST_ww(REG_WORK1, REG_WORK1);
  uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 8, 1);         // move x to left side of d
	BFI_wwii(REG_WORK2, REG_WORK2, 9, 9); // duplicate 9 bits
	
	LSR_www(REG_WORK3, REG_WORK2, REG_WORK1);
	BFI_wwii(d, REG_WORK3, 0, 8);

  // calc N and Z
  LSL_wwi(REG_WORK3, REG_WORK3, 24);
  TST_ww(REG_WORK3, REG_WORK3);
	
	// calc C and X
	SUB_wwi(REG_WORK1, REG_WORK1, 1);
	LSR_www(REG_WORK3, REG_WORK2, REG_WORK1);
	UBFIZ_wwii(x, REG_WORK3, 0, 1);
  TBZ_wii(x, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);
		
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXR_b,(RW1 d, RR4 i))

MIDFUNC(2,jff_ROXR_w,(RW2 d, RR4 i))
{
	INIT_REGS_w(d, i);
  int x = rmw(FLAGX);

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 33);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 34);
	CMP_wi(REG_WORK1, 16);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 17);
	CBNZ_wi(REG_WORK1, 4);			// need to rotate

	LSL_wwi(REG_WORK1, d, 16);
	TST_ww(REG_WORK1, REG_WORK1);
  uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_wwii(REG_WORK2, x, 16, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 17, 17); // duplicate 17 bits

	LSR_xxx(REG_WORK3, REG_WORK2, REG_WORK1);
	BFI_wwii(d, REG_WORK3, 0, 16);

  // calc N and Z
  LSL_wwi(REG_WORK3, REG_WORK3, 16);
  TST_ww(REG_WORK3, REG_WORK3);
	
	// calc C and X
	SUB_wwi(REG_WORK1, REG_WORK1, 1);
	LSR_www(REG_WORK3, REG_WORK2, REG_WORK1);
	UBFIZ_wwii(x, REG_WORK3, 0, 1);
  TBZ_wii(x, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);
	
  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXR_w,(RW2 d, RR4 i))

MIDFUNC(2,jff_ROXR_l,(RW4 d, RR4 i))
{
	INIT_REGS_l(d, i);
  int x = rmw(FLAGX);

	AND_ww3f(REG_WORK1, i);
	CMP_wi(REG_WORK1, 32);
	BLE_i(2);
	SUB_wwi(REG_WORK1, REG_WORK1, 33);
	CBNZ_wi(REG_WORK1, 3);			// need to rotate

	TST_ww(d, d);
  uae_u32* branchadd = (uae_u32*)get_target();
	B_i(0);			// end of op

  // need to rotate
	MOV_ww(REG_WORK2, d);
	BFI_xxii(REG_WORK2, x, 32, 1);          // move x to left side of d
	BFI_xxii(REG_WORK2, REG_WORK2, 33, 31); // duplicate 31 bits

	LSR_xxx(d, REG_WORK2, REG_WORK1);

  // Calculate NZ
	TST_ww(d, d);

  // Calculate C
	SUB_wwi(REG_WORK1, REG_WORK1, 1);
	LSR_xxx(REG_WORK3, REG_WORK2, REG_WORK1);
	UBFIZ_wwii(x, REG_WORK3, 0, 1);
  TBZ_wii(x, 0, 4);
  MRS_NZCV_x(REG_WORK4);
  SET_xxCflag(REG_WORK4, REG_WORK4);
  MSR_NZCV_x(REG_WORK4);

  // end of op
  write_jmp_target(branchadd, (uintptr)get_target());

  flags_carry_inverted = false;
	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXR_l,(RW4 d, RR4 i))

/*
 * SCC
 *
 */
MIDFUNC(2,jnf_SCC,(W1 d, IM8 cc))
{
  FIX_INVERTED_CARRY

	INIT_WREG_b(d);
 
 	switch (cc) {
		case 9: // LS
		  CSETM_wc(REG_WORK1, NATIVE_CC_CS);
			CSETM_wc(REG_WORK2, NATIVE_CC_EQ);
			ORR_www(REG_WORK1, REG_WORK1, REG_WORK2);
			break;

		case 8: // HI
		  CSETM_wc(REG_WORK1, NATIVE_CC_CC);
      CSETM_wc(REG_WORK2, NATIVE_CC_NE);
      AND_www(REG_WORK1, REG_WORK1, REG_WORK2);
			break;

		default:
		  CSETM_wc(REG_WORK1, cc);
			break;
	}
  BFI_wwii(d, REG_WORK1, 0, 8);
  
  unlock2(d);
}
MENDFUNC(2,jnf_SCC,(W1 d, IM8 cc))

/*
 * SUB
 * Operand Syntax: 	<ea>, Dn
 * 					Dn, <ea>
 *
 * Operand Size: 8,16,32
 *
 * X Set the same as the carry bit.
 * N Set if the result is negative. Cleared otherwise.
 * Z Set if the result is zero. Cleared otherwise.
 * V Set if an overflow is generated. Cleared otherwise.
 * C Set if a carry is generated. Cleared otherwise.
 *
 */
MIDFUNC(2,jnf_SUB_b_imm,(RW1 d, IM8 v))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffffff00) | (((live.state[d].val & 0xff) - (v & 0xff)) & 0x000000ff);
		return;
	}

	INIT_REG_b(d);

	if(targetIsReg) {
	  SUB_wwi(REG_WORK1, d, v & 0xff);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		SUB_wwi(d, d, v & 0xff);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_SUB_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jnf_SUB_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUB_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		SUB_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 8);
	} else {
		SUB_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUB_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_SUB_w_imm,(RW2 d, IM16 v))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val & 0xffff0000) | (((live.state[d].val & 0xffff) - (v & 0xffff)) & 0x0000ffff);
		return;
	}

	INIT_REG_w(d);

	UNSIGNED16_IMM_2_REG(REG_WORK1, (uae_u16)v);
	if(targetIsReg) {
		SUB_www(REG_WORK1, d, REG_WORK1);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else{
		SUB_www(d, d, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_SUB_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jnf_SUB_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUB_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		SUB_www(REG_WORK1, d, s);
	  BFI_xxii(d, REG_WORK1, 0, 16);
	} else{
		SUB_www(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUB_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_SUB_l_imm,(RW2 d, IM32 v))
{
	if (isconst(d)) {
		live.state[d].val = live.state[d].val - v;
		return;
	}

	d = rmw(d);

  if(v >= 0 && v < 4096) {
    SUB_wwi(d, d, v);
  } else {
  	LOAD_U32(REG_WORK1, v);
  	SUB_www(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_SUB_l_imm,(RW2 d, IM32 v))

MIDFUNC(2,jnf_SUB_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUB_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	SUB_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUB_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_SUB_b_imm,(RW1 d, IM8 v))
{
	INIT_REG_b(d);

	LSL_wwi(REG_WORK1, d, 24);
	MOV_wish(REG_WORK2, (v & 0xff) << 8, 16);
	SUBS_www(REG_WORK1, REG_WORK1, REG_WORK2);
	BFXIL_xxii(d, REG_WORK1, 24, 8);
	
  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_SUB_b_imm,(RW1 d, IM8 v))

MIDFUNC(2,jff_SUB_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_SUB_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	LSL_wwi(REG_WORK1, d, 24);
	SUBS_wwwLSLi(REG_WORK1, REG_WORK1, s, 24);
	BFXIL_xxii(d, REG_WORK1, 24, 8);

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUB_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_SUB_w_imm,(RW2 d, IM16 v))
{
	INIT_REG_w(d);

  MOV_xi(REG_WORK1, v);
	LSL_wwi(REG_WORK2, d, 16);
	SUBS_wwwLSLi(REG_WORK1, REG_WORK2, REG_WORK1, 16);
  BFXIL_xxii(d, REG_WORK1, 16, 16);

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_SUB_w_imm,(RW2 d, IM16 v))

MIDFUNC(2,jff_SUB_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_SUB_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	LSL_wwi(REG_WORK1, d, 16);
  SUBS_wwwLSLi(REG_WORK1, REG_WORK1, s, 16);
  BFXIL_xxii(d, REG_WORK1, 16, 16);

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUB_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_SUB_l_imm,(RW4 d, IM32 v))
{
	d = rmw(d);

  if(v >= 0 && v < 4096) {
    SUBS_wwi(d, d, v);
  } else {
    LOAD_U32(REG_WORK1, v);
  	SUBS_www(d, d, REG_WORK1);
  }

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_SUB_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jff_SUB_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_SUB_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	SUBS_www(d, d, s);

  flags_carry_inverted = true;
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUB_l,(RW4 d, RR4 s))

/*
 * SUBA
 *
 * Operand Syntax: 	<ea>, Dn
 *
 * Operand Size: 16,32
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(2,jnf_SUBA_w_imm,(RW4 d, IM16 v))
{
	if (isconst(d)) {
		live.state[d].val = live.state[d].val - (uae_s32)(uae_s16)v;
		return;
	}

	d = rmw(d);
	if(v >= 0 && v < 4096) {
	  SUB_wwi(d, d, v);
	} else {
  	SIGNED16_IMM_2_REG(REG_WORK1, v);
  	SUB_www(d, d, REG_WORK1);
  }
	unlock2(d);
}
MENDFUNC(2,jnf_SUBA_w_imm,(RW4 d, IM16 v))

MIDFUNC(2,jnf_SUBA_w,(RW4 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUBA_w_imm)(d, live.state[s].val & 0xffff);
		return;
	}
  
	INIT_REGS_w(d, s);

	SUB_wwwEX(d, d, s, EX_SXTH);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUBA_w,(RW4 d, RR2 s))

MIDFUNC(2,jnf_SUBA_l_imm,(RW4 d, IM32 v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val - v);
		return;
	}

	d = rmw(d);

	if(v >= 0 && v < 4096) {
	  SUB_wwi(d, d, v);
	} else {
  	LOAD_U32(REG_WORK1, v);
  	SUB_www(d, d, REG_WORK1);
  }
  
	unlock2(d);
}
MENDFUNC(2,jnf_SUBA_l_imm,(RW4 d, IM32 v))

MIDFUNC(2,jnf_SUBA_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUBA_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	SUB_www(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUBA_l,(RW4 d, RR4 s))

/*
 * SUBX
 * Operand Syntax: 	Dy, Dx
 * 					-(Ay), -(Ax)
 *
 * Operand Size: 8,16,32
 *
 * X Set the same as the carry bit.
 * N Set if the result is negative. Cleared otherwise.
 * Z Cleared if the result is nonzero. Unchanged otherwise.
 * V Set if an overflow is generated. Cleared otherwise.
 * C Set if a carry is generated. Cleared otherwise.
 *
 * Attention: Z is cleared only if the result is nonzero. Unchanged otherwise
 *
 */
MIDFUNC(2,jnf_SUBX_b,(RW1 d, RR1 s))
{
  int x = readreg(FLAGX);
	INIT_REGS_b(d, s);

  clobber_flags();

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	LSL_wwi(REG_WORK1, d, 24);
	LSL_wwi(REG_WORK2, s, 24);
	SBC_www(REG_WORK1, REG_WORK1, REG_WORK2);
	BFXIL_wwii(d, REG_WORK1, 24, 8);

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUBX_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_SUBX_w,(RW2 d, RR2 s))
{
  int x = readreg(FLAGX);
	INIT_REGS_w(d, s);

  clobber_flags();

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	LSL_wwi(REG_WORK1, d, 16);
	LSL_wwi(REG_WORK2, s, 16);
	SBC_www(REG_WORK1, REG_WORK1, REG_WORK2);
	BFXIL_wwii(d, REG_WORK1, 16, 16);

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUBX_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_SUBX_l,(RW4 d, RR4 s))
{
  int x = readreg(FLAGX);
	INIT_REGS_l(d, s);

  clobber_flags();

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	SBC_www(d, d, s);

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_SUBX_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_SUBX_b,(RW1 d, RR1 s))
{
	INIT_REGS_b(d, s);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK2, 0);
	MOVN_xish(REG_WORK1, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK1, REG_WORK2, NATIVE_CC_NE);

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	LSL_wwi(REG_WORK1, d, 24);
	LSL_wwi(REG_WORK3, s, 24);
	SBCS_www(REG_WORK1, REG_WORK1, REG_WORK3);
	BFXIL_wwii(d, REG_WORK1, 24, 8);

	MRS_NZCV_x(REG_WORK1);
  EOR_xxCflag(REG_WORK1, REG_WORK1);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	MSR_NZCV_x(REG_WORK1);
  flags_carry_inverted = false;
	if (needed_flags & FLAG_X)
  	UBFX_xxii(x, REG_WORK1, 29, 1); // Duplicate carry

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUBX_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_SUBX_w,(RW2 d, RR2 s))
{
	INIT_REGS_w(d, s);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK2, 0);
	MOVN_xish(REG_WORK1, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK1, REG_WORK2, NATIVE_CC_NE);

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	LSL_wwi(REG_WORK1, d, 16);
	LSL_wwi(REG_WORK3, s, 16);
	SBCS_www(REG_WORK1, REG_WORK1, REG_WORK3);
	BFXIL_wwii(d, REG_WORK1, 16, 16);

	MRS_NZCV_x(REG_WORK1);
  EOR_xxCflag(REG_WORK1, REG_WORK1);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	MSR_NZCV_x(REG_WORK1);
  flags_carry_inverted = false;
	if (needed_flags & FLAG_X)
  	UBFX_xxii(x, REG_WORK1, 29, 1); // Duplicate carry

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUBX_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_SUBX_l,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);
  int x = rmw(FLAGX);

	MOVN_xi(REG_WORK2, 0);
	MOVN_xish(REG_WORK1, 0x4000, 16); // inverse Z flag
	CSEL_xxxc(REG_WORK2, REG_WORK1, REG_WORK2, NATIVE_CC_NE);

	// Restore inverted X to carry (don't care about other flags)
  NEGS_ww(REG_WORK1, x);

	SBCS_www(d, d, s);

	MRS_NZCV_x(REG_WORK1);
  EOR_xxCflag(REG_WORK1, REG_WORK1);
	AND_xxx(REG_WORK1, REG_WORK1, REG_WORK2);
	MSR_NZCV_x(REG_WORK1);
  flags_carry_inverted = false;
	if (needed_flags & FLAG_X)
  	UBFX_xxii(x, REG_WORK1, 29, 1); // Duplicate carry

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUBX_l,(RW4 d, RR4 s))

/*
 * SWAP
 * Operand Syntax: Dn
 *
 *  Operand Size: 16
 *
 * X Not affected.
 * N Set if the most significant bit of the 32-bit result is set. Cleared otherwise.
 * Z Set if the 32-bit result is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(1,jnf_SWAP,(RW4 d))
{
	if (isconst(d)) {
		live.state[d].val = (live.state[d].val >> 16) | (live.state[d].val << 16);
		return;
	}

	d = rmw(d);

	ROR_wwi(d, d, 16);

	unlock2(d);
}
MENDFUNC(1,jnf_SWAP,(RW4 d))

MIDFUNC(1,jff_SWAP,(RW4 d))
{
  if(isconst(d)) {
    live.state[d].val = (live.state[d].val >> 16) | (live.state[d].val << 16);
    uae_u32 f = 0;
    if((uae_s32)live.state[d].val == 0)
      f |= (ARM_Z_FLAG >> 16);
    if((uae_s32)live.state[d].val < 0)
      f |= (ARM_N_FLAG >> 16);
  	MOV_xish(REG_WORK1, f, 16);
  	MSR_NZCV_x(REG_WORK1);
    flags_carry_inverted = false;
    return;
  }

	d = rmw(d);

	ROR_wwi(d, d, 16);
	TST_ww(d, d);

  flags_carry_inverted = false;
	unlock2(d);
}
MENDFUNC(1,jff_SWAP,(RW4 d))

/*
 * TST
 * Operand Syntax: <ea>
 *
 *  Operand Size: 8,16,32
 *
 * X Not affected.
 * N Set if the operand is negative. Cleared otherwise.
 * Z Set if the operand is zero. Cleared otherwise.
 * V Always cleared.
 * C Always cleared.
 *
 */
MIDFUNC(1,jff_TST_b_imm,(IM8 v))
{
	SIGNED8_IMM_2_REG(REG_WORK1, (uae_u8)v);
	TST_ww(REG_WORK1, REG_WORK1);
  flags_carry_inverted = false;
}
MENDFUNC(1,jff_TST_b_imm,(IM8 v))

MIDFUNC(1,jff_TST_b,(RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_TST_b_imm)(live.state[s].val);
		return;
	}

	s = readreg(s);
	SIGNED8_REG_2_REG(REG_WORK1, s);
	unlock2(s);

	TST_ww(REG_WORK1, REG_WORK1);
  flags_carry_inverted = false;
}
MENDFUNC(1,jff_TST_b,(RR1 s))

MIDFUNC(1,jff_TST_w_imm,(IM16 v))
{
	SIGNED16_IMM_2_REG(REG_WORK1, (uae_u16)v);
	TST_ww(REG_WORK1, REG_WORK1);
  flags_carry_inverted = false;
}
MENDFUNC(1,jff_TST_w_imm,(IM16 v))

MIDFUNC(1,jff_TST_w,(RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_TST_w_imm)(live.state[s].val);
		return;
	}

	s = readreg(s);
	SIGNED16_REG_2_REG(REG_WORK1, s);
	unlock2(s);

	TST_ww(REG_WORK1, REG_WORK1);
  flags_carry_inverted = false;
}
MENDFUNC(1,jff_TST_w,(RR2 s))

MIDFUNC(1,jff_TST_l_imm,(IM32 v))
{
  LOAD_U32(REG_WORK1, v);
	TST_ww(REG_WORK1, REG_WORK1);
  flags_carry_inverted = false;
}
MENDFUNC(1,jff_TST_l_imm,(IM32 v))

MIDFUNC(1,jff_TST_l,(RR4 s))
{
  if(isconst(s)) {
		COMPCALL(jff_TST_l_imm)(live.state[s].val);
    return;
  }

	s = readreg(s);
	TST_ww(s, s);
	unlock2(s);
  flags_carry_inverted = false;
}
MENDFUNC(1,jff_TST_l,(RR4 s))

/*
 * Memory access functions
 *
 * Two versions: full address range and 24 bit address range
 *
 */
MIDFUNC(2,jnf_MEM_WRITE_OFF_b,(RR4 adr, RR4 b))
{
  adr = readreg(adr);
	b = readreg(b);
  
  STRB_wXx(b, adr, R_MEMSTART);
  
  unlock2(b);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE_OFF_b,(RR4 adr, RR4 b))

MIDFUNC(2,jnf_MEM_WRITE_OFF_w,(RR4 adr, RR4 w))
{
  adr = readreg(adr);
	w = readreg(w);
  
  REV16_ww(REG_WORK1, w);
  STRH_wXx(REG_WORK1, adr, R_MEMSTART);
  
  unlock2(w);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE_OFF_w,(RR4 adr, RR4 w))

MIDFUNC(2,jnf_MEM_WRITE_OFF_l,(RR4 adr, RR4 l))
{
  adr = readreg(adr);
  l = readreg(l);
  
  REV32_xx(REG_WORK1, l);
  STR_wXx(REG_WORK1, adr, R_MEMSTART);
  
  unlock2(l);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE_OFF_l,(RR4 adr, RR4 l))


MIDFUNC(2,jnf_MEM_READ_OFF_b,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  LDRB_wXx(d, adr, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ_OFF_b,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ_OFF_w,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  LDRH_wXx(REG_WORK1, adr, R_MEMSTART);
  REV16_ww(d, REG_WORK1);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ_OFF_w,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ_OFF_l,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  LDR_wXx(REG_WORK1, adr, R_MEMSTART);
  REV32_xx(d, REG_WORK1);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ_OFF_l,(W4 d, RR4 adr))


MIDFUNC(2,jnf_MEM_WRITE24_OFF_b,(RR4 adr, RR4 b))
{
  adr = readreg(adr);
  b = readreg(b);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  STRB_wXx(b, REG_WORK1, R_MEMSTART);
  
  unlock2(b);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE24_OFF_b,(RR4 adr, RR4 b))

MIDFUNC(2,jnf_MEM_WRITE24_OFF_w,(RR4 adr, RR4 w))
{
  adr = readreg(adr);
  w = readreg(w);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  REV16_ww(REG_WORK3, w);
  STRH_wXx(REG_WORK3, REG_WORK1, R_MEMSTART);
  
  unlock2(w);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE24_OFF_w,(RR4 adr, RR4 w))

MIDFUNC(2,jnf_MEM_WRITE24_OFF_l,(RR4 adr, RR4 l))
{
  adr = readreg(adr);
  l = readreg(l);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  REV32_xx(REG_WORK3, l);
  STR_wXx(REG_WORK3, REG_WORK1, R_MEMSTART);
  
  unlock2(l);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE24_OFF_l,(RR4 adr, RR4 l))


MIDFUNC(2,jnf_MEM_READ24_OFF_b,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  LDRB_wXx(d, REG_WORK1, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ24_OFF_b,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ24_OFF_w,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  LDRH_wXx(REG_WORK1, REG_WORK1, R_MEMSTART);
  REV16_ww(d, REG_WORK1);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ24_OFF_w,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ24_OFF_l,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  LDR_wXx(d, REG_WORK1, R_MEMSTART);
  REV32_xx(d, d);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ24_OFF_l,(W4 d, RR4 adr))


MIDFUNC(2,jnf_MEM_GETADR_OFF,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  ADD_www(d, adr, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_GETADR_OFF,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_GETADR24_OFF,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  UBFIZ_xxii(REG_WORK1, adr, 0, 24);
  ADD_www(d, REG_WORK1, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_GETADR24_OFF,(W4 d, RR4 adr))


MIDFUNC(3,jnf_MEM_READMEMBANK,(W4 dest, RR4 adr, IM8 offset))
{
  clobber_flags();
  if (dest != adr) {
    COMPCALL(forget_about)(dest);
  }

  adr = readreg_specific(adr, REG_PAR1);
  prepare_for_call_1();
  unlock2(adr);
  prepare_for_call_2();
	
  uintptr idx = (uintptr)(&regs.mem_banks) - (uintptr)(&regs);
  LDR_xXi(REG_WORK2, R_REGSTRUCT, idx);
  LSR_wwi(REG_WORK1, adr, 16);
  LDR_xXxLSLi(REG_WORK3, REG_WORK2, REG_WORK1, 1); // 1 means shift by 3
  LDR_xXi(REG_WORK3, REG_WORK3, offset);
   
  compemu_raw_call_r(REG_WORK3);

  live.nat[REG_RESULT].holds[0] = dest;
  live.nat[REG_RESULT].nholds = 1;
  live.nat[REG_RESULT].touched = touchcnt++;

  live.state[dest].realreg = REG_RESULT;
  live.state[dest].realind = 0;
  live.state[dest].val = 0;
  set_status(dest, DIRTY);
}
MENDFUNC(3,jnf_MEM_READMEMBANK,(W4 dest, RR4 adr, IM8 offset))


MIDFUNC(3,jnf_MEM_WRITEMEMBANK,(RR4 adr, RR4 source, IM8 offset))
{
  clobber_flags();

  adr = readreg_specific(adr, REG_PAR1);
  source = readreg_specific(source, REG_PAR2);
  prepare_for_call_1();
  unlock2(adr);
  unlock2(source);
  prepare_for_call_2();
	
  uintptr idx = (uintptr)(&regs.mem_banks) - (uintptr)(&regs);
  LDR_xXi(REG_WORK2, R_REGSTRUCT, idx);
  LSR_wwi(REG_WORK1, adr, 16);
  LDR_xXxLSLi(REG_WORK3, REG_WORK2, REG_WORK1, 1); // 1 means shift by 3
  LDR_xXi(REG_WORK3, REG_WORK3, offset);
    
  compemu_raw_call_r(REG_WORK3);
}
MENDFUNC(3,jnf_MEM_WRITEMEMBANK,(RR4 adr, RR4 source, IM8 offset))

