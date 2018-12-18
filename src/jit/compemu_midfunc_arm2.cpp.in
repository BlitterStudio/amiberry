/*
 * compiler/compemu_midfunc_arm.cpp - Native MIDFUNCS for ARM (JIT v2)
 *
 * Copyright (c) 2014 Jens Heitmann of ARAnyM dev team (see AUTHORS)
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
		MOV_ri(x, 0);                   \
	  ADC_rri(x, x, 0);               \
    unlock2(x);                     \
  }

#ifdef ARMV6T2
#define DUPLICACTE_CARRY_FROM_REG(r)  \
  if (needed_flags & FLAG_X) {        \
    int x = writereg(FLAGX);       		\
  	UBFX_rrii(x, r, 29, 1);           \
    unlock2(x);                       \
  }
#else
#define DUPLICACTE_CARRY_FROM_REG(r)  \
  if (needed_flags & FLAG_X) {        \
    int x = writereg(FLAGX);       		\
  	LSR_rri(x, r, 29);                \
  	AND_rri(x, x, 1);                 \
    unlock2(x);                       \
  }
#endif

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
MIDFUNC(3,jnf_ADD_im8,(W4 d, RR4 s, IMM v))
{
	if (isconst(s)) {
		set_const(d, live.state[s].val + v);
		return;
	}

	int s_is_d = (s == d);
	if(s_is_d) {
		s = d = rmw(d);
	} else {
		s = readreg(s);
		d = writereg(d);
	}
	
	ADD_rri(d, s, v & 0xff);

	EXIT_REGS(d, s);
}
MENDFUNC(3,jnf_ADD_im8,(W4 d, RR4 s, IMM v))

MIDFUNC(2,jnf_ADD_b_imm,(RW1 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | (((live.state[d].val & 0xff) + (v & 0xff)) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);
	
	if(targetIsReg) {
	  ADD_rri(REG_WORK1, d, v & 0xff);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
	  ADD_rri(d, d, v & 0xff);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_ADD_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jnf_ADD_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADD_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);
	
	if(targetIsReg) {
		ADD_rrr(REG_WORK1, d, s);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		ADD_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADD_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_ADD_w_imm,(RW2 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | (((live.state[d].val & 0xffff) + (v & 0xffff)) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	if(targetIsReg) {
	  if(CHECK32(v & 0xffff)) {
	    ADD_rri(REG_WORK1, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  ADD_rrr(REG_WORK1, d, REG_WORK1);
	  }
	  PKHTB_rrr(d, d, REG_WORK1);
	} else{
	  if(CHECK32(v & 0xffff)) {
	    ADD_rri(d, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  ADD_rrr(d, d, REG_WORK1);
	  }
	}
	
	unlock2(d);
}
MENDFUNC(2,jnf_ADD_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jnf_ADD_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADD_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		ADD_rrr(REG_WORK1, d, s);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		ADD_rrr(d, d, s);
	}
	
	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADD_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_ADD_l_imm,(RW4 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val + v);
		return;
	}

	d = rmw(d);

  if(CHECK32(v)) {
    ADD_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK1, v);
	  ADD_rrr(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_ADD_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jnf_ADD_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_ADD_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ADD_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADD_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_ADD_b_imm,(RW1 d, IMM v))
{
	INIT_REG_b(d);
  
	MOV_ri8RORi(REG_WORK2, v & 0xff, 8);
  ADDS_rrrLSLi(REG_WORK1, REG_WORK2, d, 24);
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}
		
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_ADD_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jff_ADD_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_ADD_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	LSL_rri(REG_WORK2, s, 24);
  ADDS_rrrLSLi(REG_WORK1, REG_WORK2, d, 24);
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}
	
  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADD_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_ADD_w_imm,(RW2 d, IMM v))
{
	INIT_REG_w(d);

#ifdef ARMV6T2
  MOVW_ri16(REG_WORK1, v & 0xffff);
  LSL_rri(REG_WORK1, REG_WORK1, 16);
#else
  uae_s32 offs = data_long_offs(v << 16);
	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	
  ADDS_rrrLSLi(REG_WORK1, REG_WORK1, d, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);
	
  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_ADD_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jff_ADD_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_ADD_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	LSL_rri(REG_WORK1, s, 16);
  ADDS_rrrLSLi(REG_WORK1, REG_WORK1, d, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);

  DUPLICACTE_CARRY

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADD_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_ADD_l_imm,(RW4 d, IMM v))
{
	d = rmw(d);

  if(CHECK32(v)) {
    ADDS_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK2, v);
	  ADDS_rrr(d, d, REG_WORK2);
  }

  DUPLICACTE_CARRY

	unlock2(d);
}
MENDFUNC(2,jff_ADD_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jff_ADD_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_ADD_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ADDS_rrr(d, d, s);

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
MIDFUNC(2,jnf_ADDA_w,(RW4 d, RR2 s))
{
	if (isconst(d) && isconst(s)) {
		set_const(d, live.state[d].val + (uae_s32)(uae_s16)(live.state[s].val & 0xffff));
		return;
	}

	INIT_REGS_w(d, s);

	SXTAH_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_ADDA_w,(RW4 d, RR2 s))

MIDFUNC(2,jnf_ADDA_l,(RW4 d, RR4 s))
{
	if (isconst(d) && isconst(s)) {
		set_const(d, live.state[d].val + live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ADD_rrr(d, d, s);

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

	if(targetIsReg) {
		if(s_is_d) {
			ADD_rrrLSLi(REG_WORK1, x, d, 1);
		} else {
			ADD_rrr(REG_WORK1, d, s);
		  ADD_rrr(REG_WORK1, REG_WORK1, x);
		}
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		if(s_is_d) {
		  ADD_rrrLSLi(d, x, d, 1);
		} else {
			ADD_rrr(d, d, s);
		  ADD_rrr(d, d, x);
		}
	}

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_ADDX_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_ADDX_w,(RW2 d, RR2 s))
{
  int x = readreg(FLAGX);
	INIT_REGS_w(d, s);

	if(targetIsReg) {
		if(s_is_d) {
			ADD_rrrLSLi(REG_WORK1, x, d, 1);
		} else {
			ADD_rrr(REG_WORK1, d, s);
		  ADD_rrr(REG_WORK1, REG_WORK1, x);
		}
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		if(s_is_d) {
		  ADD_rrrLSLi(d, x, d, 1);
		} else {
			ADD_rrr(d, d, s);
		  ADD_rrr(d, d, x);
		}
	}

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_ADDX_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_ADDX_l,(RW4 d, RR4 s))
{
  int x = readreg(FLAGX);
	INIT_REGS_l(d, s);

	if(s_is_d) {
	  ADD_rrrLSLi(d, x, d, 1);
	} else {
	  ADD_rrr(d, d, s);
	  ADD_rrr(d, d, x);
	}

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_ADDX_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_ADDX_b,(RW1 d, RR1 s))
{
	INIT_REGS_b(d, s);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore X to carry (don't care about other flags)
  LSL_rri(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	MVN_ri(REG_WORK1, 0);
#ifdef ARMV6T2
	BFI_rrii(REG_WORK1, s, 24, 31);
#else
	BIC_rri(REG_WORK1, REG_WORK1, 0xff000000);
	ORR_rrrLSLi(REG_WORK1, REG_WORK1, s, 24);
#endif
  ADCS_rrrLSLi(REG_WORK1, REG_WORK1, d, 24);
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}

	MRS_CPSR(REG_WORK1);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADDX_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_ADDX_w,(RW2 d, RR2 s))
{
	INIT_REGS_w(d, s);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore X to carry (don't care about other flags)
  LSL_rri(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	MVN_ri(REG_WORK1, 0);
#ifdef ARMV6T2
	BFI_rrii(REG_WORK1, s, 16, 31);
#else
	BIC_rri(REG_WORK1, REG_WORK1, 0xff000000);
	BIC_rri(REG_WORK1, REG_WORK1, 0x00ff0000);
	ORR_rrrLSLi(REG_WORK1, REG_WORK1, s, 16);
#endif
  ADCS_rrrLSLi(REG_WORK1, REG_WORK1, d, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);

	MRS_CPSR(REG_WORK1);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_ADDX_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_ADDX_l,(W4 d, RR4 s))
{
	INIT_REGS_l(d, s);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore X to carry (don't care about other flags)
  LSL_rri(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	ADCS_rrr(d, d, s);

	MRS_CPSR(REG_WORK1);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

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
MIDFUNC(1,jff_ANDSR,(IMM s, IMM x))
{
	MRS_CPSR(REG_WORK1);
	AND_rri(REG_WORK1, REG_WORK1, s);
	MSR_CPSRf_r(REG_WORK1);

	if (!x) {
		int f = writereg(FLAGX);
		MOV_ri(f, 0);
		unlock2(f);
	}
}
MENDFUNC(1,jff_ANDSR,(IMM s))

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
MIDFUNC(2,jnf_AND_b_imm,(RW1 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | ((live.state[d].val & v) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);

	BIC_rri(d, d, (~v) & 0xff);

	unlock2(d);
}
MENDFUNC(2,jnf_AND_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jnf_AND_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_AND_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		AND_rrr(REG_WORK1, d, s);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		AND_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_AND_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_AND_w_imm,(RW2 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | ((live.state[d].val & v) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	if(targetIsReg) {
	  if(CHECK32(v & 0xffff)) {
	    AND_rri(REG_WORK1, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  AND_rrr(REG_WORK1, d, REG_WORK1);
	  }
	  PKHTB_rrr(d, d, REG_WORK1);
	} else{
	  if(CHECK32(v & 0xffff)) {
	    AND_rri(d, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  AND_rrr(d, d, REG_WORK1);
	  }
	}

	unlock2(d);
}
MENDFUNC(2,jnf_AND_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jnf_AND_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_AND_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		AND_rrr(REG_WORK1, d, s);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		AND_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_AND_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_AND_l_imm,(RW4 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val & v);
		return;
	}

	d = rmw(d);

  if(CHECK32(v)) {
    AND_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK1, v);
	  AND_rrr(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_AND_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jnf_AND_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_AND_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	AND_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_AND_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_AND_b_imm,(RW1 d, IMM v))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_IMM_2_REG(REG_WORK2, v);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ANDS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		ANDS_rrr(d, REG_WORK1, REG_WORK2);
	}

	unlock2(d);
}
MENDFUNC(2,jff_AND_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jff_AND_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_AND_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_REG_2_REG(REG_WORK2, s);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ANDS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		ANDS_rrr(d, REG_WORK1, REG_WORK2);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_AND_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_AND_w_imm,(RW2 d, IMM v))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_IMM_2_REG(REG_WORK2, v);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ANDS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		ANDS_rrr(d, REG_WORK1, REG_WORK2);
	}

	unlock2(d);
}
MENDFUNC(2,jff_AND_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jff_AND_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_AND_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_REG_2_REG(REG_WORK2, s);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ANDS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		ANDS_rrr(d, REG_WORK1, REG_WORK2);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_AND_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_AND_l_imm,(RW4 d, IMM v))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
  if(CHECK32(v)) {
    ANDS_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK2, v);
	  ANDS_rrr(d, d, REG_WORK2);
  }

	unlock2(d);
}
MENDFUNC(2,jff_AND_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jff_AND_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_AND_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	MSR_CPSRf_i(0);
	ANDS_rrr(d, d, s);

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
MIDFUNC(2,jff_ASL_b_imm,(RW1 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);
		
	LSL_rri(REG_WORK3, d, 24);
	if (i) {
		// Calculate V Flag
		MOV_ri8RORi(REG_WORK2, 0x80, 8);
		ASR_rri(REG_WORK2, REG_WORK2, i);
		ANDS_rrr(REG_WORK1, REG_WORK3, REG_WORK2);
		CC_TEQ_rr(NATIVE_CC_NE, REG_WORK1, REG_WORK2);
		MOV_ri(REG_WORK1, 0);
		CC_MOV_ri(NATIVE_CC_NE, REG_WORK1, ARM_V_FLAG);

		MSR_CPSRf_r(REG_WORK1); // store V flag

		LSLS_rri(REG_WORK3, REG_WORK3, i);
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK3, 24);
		DUPLICACTE_CARRY
	} else {
		MSR_CPSRf_i(0);
		TST_rr(REG_WORK3, REG_WORK3);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASL_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jff_ASL_w_imm,(RW2 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);
		
	LSL_rri(REG_WORK3, d, 16);
	if (i) {
		// Calculate V Flag
		MOV_ri8RORi(REG_WORK2, 0x80, 8);
		ASR_rri(REG_WORK2, REG_WORK2, i);
		ANDS_rrr(REG_WORK1, REG_WORK3, REG_WORK2);
		CC_TEQ_rr(NATIVE_CC_NE, REG_WORK1, REG_WORK2);
		MOV_ri(REG_WORK1, 0);
		CC_MOV_ri(NATIVE_CC_NE, REG_WORK1, ARM_V_FLAG);

		MSR_CPSRf_r(REG_WORK1); // store V flag

		LSLS_rri(REG_WORK3, REG_WORK3, i);
	  PKHTB_rrrASRi(d, d, REG_WORK3, 16);
		DUPLICACTE_CARRY
	} else {
		MSR_CPSRf_i(0);
		TST_rr(REG_WORK3, REG_WORK3);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASL_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jff_ASL_l_imm,(RW4 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	if (i) {
		// Calculate V Flag
		MOV_ri8RORi(REG_WORK2, 0x80, 8);
		ASR_rri(REG_WORK2, REG_WORK2, i);
		ANDS_rrr(REG_WORK1, d, REG_WORK2);
		CC_TEQ_rr(NATIVE_CC_NE, REG_WORK1, REG_WORK2);
		MOV_ri(REG_WORK1, 0);
		CC_MOV_ri(NATIVE_CC_NE, REG_WORK1, ARM_V_FLAG);

		MSR_CPSRf_r(REG_WORK1); // store V flag

		LSLS_rri(d, d, i);
		DUPLICACTE_CARRY
	} else {
		MSR_CPSRf_i(0);
		TST_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASL_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jff_ASL_b_reg,(RW1 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);
  
	LSL_rri(REG_WORK3, d, 24);
	// Calculate V Flag
	MOV_ri8RORi(REG_WORK2, 0x80, 8);
	ASR_rrr(REG_WORK2, REG_WORK2, i);
	ANDS_rrr(REG_WORK1, REG_WORK3, REG_WORK2);
	CC_TEQ_rr(NATIVE_CC_NE, REG_WORK1, REG_WORK2);
	MOV_ri(REG_WORK1, 0);
	CC_MOV_ri(NATIVE_CC_NE, REG_WORK1, ARM_V_FLAG);

	MSR_CPSRf_r(REG_WORK1); // store V flag

	ANDS_rri(REG_WORK2, i, 63);
	BEQ_i(5);               // No shift -> X flag unchanged
	LSLS_rrr(REG_WORK3, REG_WORK3, REG_WORK2);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
  BIC_rri(d, d, 0xff);
  ORR_rrrLSRi(d, d, REG_WORK3, 24);
	B_i(0);
	TST_rr(REG_WORK3, REG_WORK3);
  
  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASL_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_ASL_w_reg,(RW2 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	LSL_rri(REG_WORK3, d, 16);
	// Calculate V Flag
	MOV_ri8RORi(REG_WORK2, 0x80, 8);
	ASR_rrr(REG_WORK2, REG_WORK2, i);
	ANDS_rrr(REG_WORK1, REG_WORK3, REG_WORK2);
	CC_TEQ_rr(NATIVE_CC_NE, REG_WORK1, REG_WORK2);
	MOV_ri(REG_WORK1, 0);
	CC_MOV_ri(NATIVE_CC_NE, REG_WORK1, ARM_V_FLAG);

	MSR_CPSRf_r(REG_WORK1); // store V flag

	ANDS_rri(REG_WORK2, i, 63);
	BEQ_i(4);               // No shift -> X flag unchanged
	LSLS_rrr(REG_WORK3, REG_WORK3, REG_WORK2);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
  PKHTB_rrrASRi(d, d, REG_WORK3, 16);
	B_i(0);
	TST_rr(REG_WORK3, REG_WORK3);
	
  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASL_w_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_ASL_l_reg,(RW4 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	// Calculate V Flag
	MOV_ri8RORi(REG_WORK2, 0x80, 8);
	ASR_rrr(REG_WORK2, REG_WORK2, i);
	ANDS_rrr(REG_WORK1, d, REG_WORK2);
	CC_TEQ_rr(NATIVE_CC_NE, REG_WORK1, REG_WORK2);
	MOV_ri(REG_WORK1, 0);
	CC_MOV_ri(NATIVE_CC_NE, REG_WORK1, ARM_V_FLAG);

	MSR_CPSRf_r(REG_WORK1); // store V flag

	ANDS_rri(REG_WORK2, i, 63);
	BEQ_i(3);               // No shift -> X flag unchanged
	LSLS_rrr(d, d, REG_WORK2);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
	B_i(0);
	TST_rr(d, d);

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
	if (isconst(d)) {
		set_const(d, (live.state[d].val << 1) & 0xffff);
		return;
	}

	d = rmw(d);

	LSL_rri(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_ASLW,(RW2 d))

MIDFUNC(1,jff_ASLW,(RW2 d))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
	LSLS_rri(d, d, 17);

	MRS_CPSR(REG_WORK1);
	DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

  // Calculate V flag
	CC_ORR_rri(NATIVE_CC_MI, REG_WORK1, REG_WORK1, ARM_V_FLAG);
	CC_EOR_rri(NATIVE_CC_CS, REG_WORK1, REG_WORK1, ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	ASR_rri(d, d, 16);

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
 * imm version only called with 1 <= i <= 8
 *
 */
MIDFUNC(2,jnf_ASR_b_imm,(RW1 d, IMM i))
{
	if(i) {
		d = rmw(d);

		SIGNED8_REG_2_REG(REG_WORK1, d);
  	ASR_rri(REG_WORK1, REG_WORK1, i);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif

		unlock2(d);
	}
}
MENDFUNC(2,jnf_ASR_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jnf_ASR_w_imm,(RW2 d, IMM i))
{
	if(i) {
		d = rmw(d);

		SIGNED16_REG_2_REG(REG_WORK1, d);
	  PKHTB_rrrASRi(d, d, REG_WORK1, i);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_ASR_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jnf_ASR_l_imm,(RW4 d, IMM i))
{
	if(i) {
		d = rmw(d);

  	ASR_rri(d, d, i);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_ASR_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jff_ASR_b_imm,(RW1 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	if (i) {
		ASRS_rri(REG_WORK1, REG_WORK1, i);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
		DUPLICACTE_CARRY
	} else {
		TST_rr(REG_WORK1, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASR_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jff_ASR_w_imm,(RW2 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	if (i) {
		ASRS_rri(REG_WORK1, REG_WORK1, i);
	  PKHTB_rrr(d, d, REG_WORK1);
		DUPLICACTE_CARRY
	} else {
		TST_rr(REG_WORK1, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASR_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jff_ASR_l_imm,(RW4 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);
	if (i) {
		ASRS_rri(d, d, i);
		DUPLICACTE_CARRY
	} else {
		TST_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ASR_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jnf_ASR_b_reg,(RW1 d, RR4 i)) 
{
	i = readreg(i);
	d = rmw(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	AND_rri(REG_WORK2, i, 63);
	ASR_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  AND_rri(REG_WORK1, REG_WORK1, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK1);
#endif

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jnf_ASR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ASR_w_reg,(RW2 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	AND_rri(REG_WORK2, i, 63);
	ASR_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  PKHTB_rrr(d, d, REG_WORK1);

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jnf_ASR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ASR_l_reg,(RW4 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);

	AND_rri(REG_WORK1, i, 63);
	ASR_rrr(d, d, REG_WORK1);

	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jnf_ASR_l_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_ASR_b_reg,(RW1 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	ANDS_rri(REG_WORK2, i, 63);
	BEQ_i(3);               // No shift -> X flag unchanged
	ASRS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
	B_i(0);
	TST_rr(REG_WORK1, REG_WORK1);

#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  AND_rri(REG_WORK1, REG_WORK1, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK1);
#endif

  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_ASR_w_reg,(RW2 d, RR4 i)) 
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	ANDS_rri(REG_WORK2, i, 63);
	BEQ_i(4);               // No shift -> X flag unchanged
	ASRS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
  PKHTB_rrr(d, d, REG_WORK1);
	B_i(0);
	TST_rr(REG_WORK1, REG_WORK1);

  unlock2(x);
	unlock2(d);
	unlock2(i);
}
MENDFUNC(2,jff_ASR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jff_ASR_l_reg,(RW4 d, RR4 i))
{
	i = readreg(i);
	d = rmw(d);
  int x = writereg(FLAGX);

	MSR_CPSRf_i(0);
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(3);               // No shift -> X flag unchanged
	ASRS_rrr(d, d, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
	B_i(0);
	TST_rr(d, d);

  unlock2(x);
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
	ASR_rri(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_ASRW,(RW2 d))

MIDFUNC(1,jff_ASRW,(RW2 d))
{
	d = rmw(d);

	SIGNED16_REG_2_REG(d, d);
	MSR_CPSRf_i(0);
	ASRS_rri(d, d, 1);
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
 * Z Set if the bit tested was zero. Cleared otherwise.
 * V Not affected.
 * C Not affected.
 *
 */
/* BCHG.B: target is never a register */
/* BCHG.L: target is always a register */
MIDFUNC(2,jnf_BCHG_b_imm,(RW1 d, IMM s))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val ^ (1 << s));
		return;
	}
	d = rmw(d);
	EOR_rri(d, d, (1 << s));
	unlock2(d);
}
MENDFUNC(2,jnf_BCHG_b_imm,(RW1 d, IMM s))

MIDFUNC(2,jnf_BCHG_l_imm,(RW4 d, IMM s))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val ^ (1 << s));
		return;
	}
	d = rmw(d);
	EOR_rri(d, d, (1 << s));
	unlock2(d);
}
MENDFUNC(2,jnf_BCHG_l_imm,(RW4 d, IMM s))

MIDFUNC(2,jnf_BCHG_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCHG_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);

	EOR_rrrLSLr(d, d, REG_WORK2, REG_WORK1);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_BCHG_b,(RW1 d, RR4 s))

MIDFUNC(2,jnf_BCHG_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCHG_l_imm)(d, live.state[s].val & 31);
		return;
	}

	INIT_REGS_l(d,s);

	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);

	EOR_rrrLSLr(d, d, REG_WORK2, REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jnf_BCHG_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_BCHG_b_imm,(RW1 d, IMM s))
{
	d = rmw(d);

	uae_u32 v = (1 << s);
	MRS_CPSR(REG_WORK1);
	EOR_rri(d, d, v);
#ifdef ARMV6T2
  UBFX_rrii(REG_WORK2, d, s, 1);
  BFI_rrii(REG_WORK1, REG_WORK2, 30, 30);
#else
	LSR_rri(REG_WORK2, d, 29);
	AND_rri(REG_WORK2, REG_WORK2, 1);
  BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
  ORR_rrrLSLi(REG_WORK1, REG_WORK1, REG_WORK2, 30);
#endif
  MSR_CPSRf_r(REG_WORK1);
  
	unlock2(d);
}
MENDFUNC(2,jff_BCHG_b_imm,(RW1 d, IMM s))

MIDFUNC(2,jff_BCHG_l_imm,(RW4 d, IMM s))
{
	d = rmw(d);

	uae_u32 v = (1 << s);
	MRS_CPSR(REG_WORK1);
	EOR_rri(d, d, v);
#ifdef ARMV6T2
  UBFX_rrii(REG_WORK2, d, s, 1);
  BFI_rrii(REG_WORK1, REG_WORK2, 30, 30);
#else
	LSR_rri(REG_WORK2, d, 29);
	AND_rri(REG_WORK2, REG_WORK2, 1);
  BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
  ORR_rrrLSLi(REG_WORK1, REG_WORK1, REG_WORK2, 30);
#endif
  MSR_CPSRf_r(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_BCHG_l_imm,(RW4 d, IMM s))

MIDFUNC(2,jff_BCHG_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCHG_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	EOR_rrr(d, d, REG_WORK2);
  
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BCHG_b,(RW1 d, RR4 s))

MIDFUNC(2,jff_BCHG_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCHG_l_imm)(d, live.state[s].val & 31);
		return;
	}

	INIT_REGS_l(d,s);

	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	EOR_rrr(d, d, REG_WORK2);

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
 * Z Set if the bit tested was zero. Cleared otherwise.
 * V Not affected.
 * C Not affected.
 *
 */
/* BCLR.B: target is never a register */
/* BCLR.L: target is always a register */
MIDFUNC(2,jnf_BCLR_b_imm,(RW1 d, IMM s))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val & ~(1 << s));
		return;
	}
	d = rmw(d);
	BIC_rri(d, d, (1 << s));
	unlock2(d);
}
MENDFUNC(2,jnf_BCLR_b_imm,(RW1 d, IMM s))

MIDFUNC(2,jnf_BCLR_l_imm,(RW4 d, IMM s))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val & ~(1 << s));
		return;
	}
	d = rmw(d);
	BIC_rri(d, d, (1 << s));
	unlock2(d);
}
MENDFUNC(2,jnf_BCLR_l_imm,(RW4 d, IMM s))

MIDFUNC(2,jnf_BCLR_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCLR_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);

	BIC_rrrLSLr(d, d, REG_WORK2, REG_WORK1);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_BCLR_b,(RW1 d, RR4 s))

MIDFUNC(2,jnf_BCLR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BCLR_l_imm)(d, live.state[s].val & 31);
		return;
	}

	INIT_REGS_l(d,s);

	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);

	BIC_rrrLSLr(d, d, REG_WORK2, REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jnf_BCLR_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_BCLR_b_imm,(RW1 d, IMM s))
{
	d = rmw(d);

	uae_u32 v = (1 << s);
	MRS_CPSR(REG_WORK1);
	TST_ri(d, v);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	BIC_rri(d, d, v);

	unlock2(d);
}
MENDFUNC(2,jff_BCLR_b_imm,(RW1 d, IMM s))

MIDFUNC(2,jff_BCLR_l_imm,(RW4 d, IMM s))
{
	d = rmw(d);

	uae_u32 v = (1 << s);
	MRS_CPSR(REG_WORK1);
	TST_ri(d, v);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	BIC_rri(d, d, v);

	unlock2(d);
}
MENDFUNC(2,jff_BCLR_l_imm,(RW4 d, IMM s))

MIDFUNC(2,jff_BCLR_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCLR_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d,REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	BIC_rrr(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BCLR_b,(RW1 d, RR4 s))

MIDFUNC(2,jff_BCLR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BCLR_l_imm)(d, live.state[s].val & 31);
		return;
	}

	INIT_REGS_l(d,s);

	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	BIC_rrr(d, d ,REG_WORK2);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_BCLR_l,(RW4 d, RR4 s))

/*
 * BSET
 * Operand Syntax: 	Dn,<ea>
 *					#<data>,<ea>
 *
 *  Operand Size: 8,32
 *
 * X Not affected.
 * N Not affected.
 * Z Set if the bit tested was zero. Cleared otherwise.
 * V Not affected.
 * C Not affected.
 *
 */
/* BSET.B: target is never a register */
/* BSET.L: target is always a register */
MIDFUNC(2,jnf_BSET_b_imm,(RW1 d, IMM s))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val | (1 << s));
		return;
	}
	d = rmw(d);
	ORR_rri(d, d, (1 << s));
	unlock2(d);
}
MENDFUNC(2,jnf_BSET_b_imm,(RW1 d, IMM s))

MIDFUNC(2,jnf_BSET_l_imm,(RW4 d, IMM s))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val | (1 << s));
		return;
	}

	d = rmw(d);
	ORR_rri(d, d, (1 << s));
	unlock2(d);
}
MENDFUNC(2,jnf_BSET_l_imm,(RW4 d, IMM s))

MIDFUNC(2,jnf_BSET_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BSET_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);

	ORR_rrrLSLr(d, d, REG_WORK2, REG_WORK1);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jnf_BSET_b,(RW1 d, RR4 s))

MIDFUNC(2,jnf_BSET_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_BSET_l_imm)(d, live.state[s].val & 31);
		return;
	}

	INIT_REGS_l(d,s);

	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);

	ORR_rrrLSLr(d, d, REG_WORK2, REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jnf_BSET_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_BSET_b_imm,(RW1 d, IMM s))
{
	d = rmw(d);

	uae_u32 v = (1 << s);
	MRS_CPSR(REG_WORK1);
	TST_ri(d, v);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	ORR_rri(d, d, v);

	unlock2(d);
}
MENDFUNC(2,jff_BSET_b_imm,(RW1 d, IMM s))

MIDFUNC(2,jff_BSET_l_imm,(RW4 d, IMM s))
{
	d = rmw(d);

	uae_u32 v = (1 << s);
	MRS_CPSR(REG_WORK1);
	TST_ri(d, v);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	ORR_rri(d, d, v);

	unlock2(d);
}
MENDFUNC(2,jff_BSET_l_imm,(RW4 d, IMM s))

MIDFUNC(2,jff_BSET_b,(RW1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BSET_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = rmw(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	ORR_rrr(d, d, REG_WORK2);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BSET_b,(RW1 d, RR4 s))

MIDFUNC(2,jff_BSET_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BSET_l_imm)(d, live.state[s].val & 31);
		return;
	}

	INIT_REGS_l(d,s);

	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);
	ORR_rrr(d, d, REG_WORK2);

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
MIDFUNC(2,jff_BTST_b_imm,(RR1 d, IMM s))
{
	d = readreg(d);

	MRS_CPSR(REG_WORK1);
	TST_ri(d, (1 << s));
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_BTST_b_imm,(RR1 d, IMM s))

MIDFUNC(2,jff_BTST_l_imm,(RR4 d, IMM s))
{
	d = readreg(d);

	MRS_CPSR(REG_WORK1);
	TST_ri(d, (1 << s));
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_BTST_l_imm,(RR4 d, IMM s))

MIDFUNC(2,jff_BTST_b,(RR1 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BTST_b_imm)(d, live.state[s].val & 7);
		return;
	}
	s = readreg(s);
	d = readreg(d);

	AND_rri(REG_WORK1, s, 7);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,jff_BTST_b,(RR1 d, RR4 s))

MIDFUNC(2,jff_BTST_l,(RR4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_BTST_l_imm)(d, live.state[s].val & 31);
		return;
	}

	int s_is_d = (s == d);
	d = readreg(d);
	if(!s_is_d)
		s = readreg(s);
	else
		s = d;
		
	AND_rri(REG_WORK1, s, 31);
	MOV_ri(REG_WORK2, 1);
	LSL_rrr(REG_WORK2, REG_WORK2, REG_WORK1);

	MRS_CPSR(REG_WORK1);
	TST_rr(d, REG_WORK2);
	BIC_rri(REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	CC_ORR_rri(NATIVE_CC_EQ, REG_WORK1, REG_WORK1, ARM_Z_FLAG);
	MSR_CPSRf_r(REG_WORK1);

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
	if(isconst(d)) {
		set_const(d, live.state[d].val & 0xffffff00);
		return;
	}
	if(d >= 16) {
		set_const(d, 0);
		return;
	}
	INIT_WREG_b(d);
	BIC_rri(d, d, 0xff);
	unlock2(d);
}
MENDFUNC(1,jnf_CLR_b,(W1 d))

MIDFUNC(1,jnf_CLR_w,(W2 d))
{
	if(isconst(d)) {
		set_const(d, live.state[d].val & 0xffff0000);
		return;
	}
	if(d >= 16) {
		set_const(d, 0);
		return;
	}
	INIT_WREG_w(d);
	if(targetIsReg) {
		BIC_rri(d, d, 0x00ff);
		BIC_rri(d, d, 0xff00);
	} else {
		MOV_ri(d, 0);
	}
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
	MSR_CPSRf_i(ARM_Z_FLAG);
  if(isconst(d)) {
    set_const(d, live.state[d].val & 0xffffff00);
    return;
  }
	if(d >= 16) {
		set_const(d, 0);
		return;
	}
	INIT_WREG_b(d);
	BIC_rri(d, d, 0xff);
	unlock2(d);
}
MENDFUNC(1,jff_CLR_b,(W1 d))

MIDFUNC(1,jff_CLR_w,(W2 d))
{
	MSR_CPSRf_i(ARM_Z_FLAG);
  if(isconst(d)) {
    set_const(d, live.state[d].val & 0xffff0000);
    return;
  }
	if(d >= 16) {
		set_const(d, 0);
    return;
  }
	INIT_WREG_w(d);
	if(targetIsReg) {
		BIC_rri(d, d, 0x00ff);
		BIC_rri(d, d, 0xff00);
	} else {
		MOV_ri(d, 0);
	}
	unlock2(d);
}
MENDFUNC(1,jff_CLR_w,(W2 d))

MIDFUNC(1,jff_CLR_l,(W4 d))
{
	MSR_CPSRf_i(ARM_Z_FLAG);
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
MIDFUNC(2,jff_CMP_b,(RR1 d, RR1 s))
{
	INIT_RREGS_b(d, s);
		
	LSL_rri(REG_WORK1, d, 24);
	CMP_rrLSLi(REG_WORK1, s, 24);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_CMP_b,(RR1 d, RR1 s))

MIDFUNC(2,jff_CMP_w,(RR2 d, RR2 s))
{
	INIT_RREGS_w(d, s);

	LSL_rri(REG_WORK1, d, 16);
	CMP_rrLSLi(REG_WORK1, s, 16);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_CMP_w,(RR2 d, RR2 s))

MIDFUNC(2,jff_CMP_l,(RR4 d, RR4 s))
{
	INIT_RREGS_l(d, s);

	CMP_rr(d, s);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);

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
MIDFUNC(2,jff_CMPA_w,(RR2 d, RR2 s))
{
	INIT_RREGS_w(d, s);

  SIGNED16_REG_2_REG(REG_WORK2, s);
  CMP_rr(d, REG_WORK2);
  
	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_CMPA_w,(RR2 d, RR2 s))

MIDFUNC(2,jff_CMPA_l,(RR4 d, RR4 s))
{
	INIT_RREGS_l(d, s);

	CMP_rr(d, s);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	EXIT_REGS(d,s);
}
MENDFUNC(2,jff_CMPA_l,(RR4 d, RR4 s))

/*
 * DBCC
 *
 */
MIDFUNC(2,jff_DBCC,(RW4 d, IMM cc))
{
  d = rmw(d);
  
  // If cc true -> no branch, so we have to clear ARM_C_FLAG
	MOV_ri(REG_WORK1, ARM_C_FLAG);
  switch(cc) {
    case 9: // LS
      CC_MOV_ri(NATIVE_CC_EQ, REG_WORK1, 0);
      CC_MOV_ri(NATIVE_CC_CS, REG_WORK1, 0);
      break;

    case 8: // HI
      CC_MOV_ri(NATIVE_CC_CC, REG_WORK1, 0);
      CC_MOV_ri(NATIVE_CC_EQ, REG_WORK1, ARM_C_FLAG);
      break;

    default:
    	CC_MOV_ri(cc, REG_WORK1, 0);
    	break;
  }
  clobber_flags();
	MSR_CPSRf_r(REG_WORK1);

  BCC_i(2); // If cc true -> no sub
  
  // sub (d, 1)
  LSL_rri(REG_WORK2, d, 16);
	SUBS_rri(REG_WORK2, REG_WORK2, 1 << 16);
  PKHTB_rrrASRi(d, d, REG_WORK2, 16);
  
  // caller can now use register_branch(v1, v2, NATIVE_CC_CS);
    
  unlock2(d);
}
MENDFUNC(2,jff_DBCC,(RW4 d, IMM cc))

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
#ifdef ARMV6T2

MIDFUNC(2,jnf_DIVU,(RW4 d, RR4 s))
{
	if (isconst(d) && isconst(s) && live.state[s].val != 0) {
		uae_u32 newv = (uae_u32)live.state[d].val / (uae_u32)(uae_u16)live.state[s].val;
		uae_u32 rem = (uae_u32)live.state[d].val % (uae_u32)(uae_u16)live.state[s].val;
		set_const(d, (newv & 0xffff) | ((uae_u32)rem << 16));
		return;
	}
	INIT_REGS_l(d, s);
  
  UNSIGNED16_REG_2_REG(REG_WORK3, s);
  TST_rr(REG_WORK3, REG_WORK3);
  BNE_i(4);     // src is not 0

  // Signal exception 5
  MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
#ifdef ARM_HAS_DIV
  B_i(4);        // end_of_op

	// src is not 0  
	UDIV_rrr(REG_WORK1, d, REG_WORK3);
#else
  B_i(10);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, REG_WORK3, 0);
	VCVTIuto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIuto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toIu_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif
	  
  LSRS_rri(REG_WORK2, REG_WORK1, 16); 							// if result of this is not 0, DIVU overflows -> no result
  BNE_i(1);
  
  // Here we have to calc remainder
  MLS_rrrr(REG_WORK2, REG_WORK1, REG_WORK3, d);
	PKHBT_rrrLSLi(d, REG_WORK1, REG_WORK2, 16);
// end_of_op

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_DIVU,(RW4 d, R4 s))

MIDFUNC(2,jff_DIVU,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

  UNSIGNED16_REG_2_REG(REG_WORK3, s);
  TST_rr(REG_WORK3, REG_WORK3);
  BNE_i(6);     // src is not 0

  // Signal exception 5
	MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
  
  // simplified flag handling for div0: set Z and V (for signed DIV: Z only)
  MOV_ri(REG_WORK1, ARM_Z_FLAG | ARM_V_FLAG);
  MSR_CPSRf_r(REG_WORK1);
#ifdef ARM_HAS_DIV
  B_i(11);        // end_of_op

	// src is not 0  
	UDIV_rrr(REG_WORK1, d, REG_WORK3);
#else
  B_i(17);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, REG_WORK3, 0);
	VCVTIuto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIuto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toIu_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif
  
  LSRS_rri(REG_WORK2, REG_WORK1, 16); 							// if result of this is not 0, DIVU overflows
  BEQ_i(2);
  // Here we handle overflow
  MOV_ri(REG_WORK1, ARM_V_FLAG | ARM_N_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  B_i(5);
  
  // Here we have to calc flags and remainder
  LSLS_rri(REG_WORK2, REG_WORK1, 16);  // N and Z ok
	MRS_CPSR(REG_WORK2);
	BIC_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG | ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK2);
  
  MLS_rrrr(REG_WORK2, REG_WORK1, REG_WORK3, d);
  PKHBT_rrrLSLi(d, REG_WORK1, REG_WORK2, 16);
// end_of_op
	
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_DIVU,(RW4 d, RR4 s))
 
#endif
 
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
#ifdef ARMV6T2

MIDFUNC(2,jnf_DIVS,(RW4 d, RR4 s))
{
	if (isconst(d) && isconst(s) && live.state[s].val != 0) {
		uae_s32 newv = (uae_s32)live.state[d].val / (uae_s32)(uae_s16)live.state[s].val;
		uae_u16 rem = (uae_s32)live.state[d].val % (uae_s32)(uae_s16)live.state[s].val;
		if (((uae_s16)rem < 0) != ((uae_s32)live.state[d].val < 0)) 
			rem = -rem;
		set_const(d, (newv & 0xffff) | ((uae_u32)rem << 16));
		return;
	}
	INIT_REGS_l(d, s);
  
  SIGNED16_REG_2_REG(REG_WORK3, s);
  TST_rr(REG_WORK3, REG_WORK3);
  BNE_i(4);     // src is not 0

  // Signal exception 5
  MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
#ifdef ARM_HAS_DIV
  B_i(12);        // end_of_op

	// src is not 0  
	SDIV_rrr(REG_WORK1, d, REG_WORK3);
#else
  B_i(18);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, REG_WORK3, 0);
	VCVTIto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toI_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif
  
  // check for overflow
  MVN_ri(REG_WORK2, 0);
  LSL_rri(REG_WORK2, REG_WORK2, 15); 		// REG_WORK2 is now 0xffff8000
  ANDS_rrr(REG_WORK3, REG_WORK1, REG_WORK2);
  BEQ_i(1); 														// positive result, no overflow
	CMP_rr(REG_WORK3, REG_WORK2);
	BNE_i(5);															// overflow -> end_of_op
	
  // Here we have to calc remainder
  SIGNED16_REG_2_REG(REG_WORK3, s);
  MUL_rrr(REG_WORK2, REG_WORK1, REG_WORK3);
	SUB_rrr(REG_WORK2, d, REG_WORK2);		// REG_WORK2 contains remainder
	
	EORS_rrr(REG_WORK3, REG_WORK2, d);	// If sign of remainder and first operand differs, change sign of remainder
	CC_RSB_rri(NATIVE_CC_MI, REG_WORK2, REG_WORK2, 0);
	
	PKHBT_rrrLSLi(d, REG_WORK1, REG_WORK2, 16);
// end_of_op

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_DIVS,(RW4 d, RR4 s))

MIDFUNC(2,jff_DIVS,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

  SIGNED16_REG_2_REG(REG_WORK3, s);
  TST_rr(REG_WORK3, REG_WORK3);
  BNE_i(6);     // src is not 0

  // Signal exception 5
	MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
  
  // simplified flag handling for div0: set Z and V (for signed DIV: Z only)
  MOV_ri(REG_WORK1, ARM_Z_FLAG);
  MSR_CPSRf_r(REG_WORK1);
#ifdef ARM_HAS_DIV
  B_i(19);        // end_of_op

	// src is not 0  
	SDIV_rrr(REG_WORK1, d, REG_WORK3);
#else
  B_i(25);       // end_of_op

	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, REG_WORK3, 0);
	VCVTIto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toI_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif

  // check for overflow
  MVN_ri(REG_WORK2, 0);
  LSL_rri(REG_WORK2, REG_WORK2, 15); 		// REG_WORK2 is now 0xffff8000
  ANDS_rrr(REG_WORK3, REG_WORK1, REG_WORK2);
  BEQ_i(4); 														// positive result, no overflow
	CMP_rr(REG_WORK3, REG_WORK2);
	BEQ_i(2);															// no overflow
  
  // Here we handle overflow
  MOV_ri(REG_WORK1, ARM_V_FLAG | ARM_N_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  B_i(9);
  
  // calc flags
  LSLS_rri(REG_WORK2, REG_WORK1, 16);  // N and Z ok
	MRS_CPSR(REG_WORK2);
	BIC_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG | ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK2);
  
  // calc remainder
  SIGNED16_REG_2_REG(REG_WORK3, s);
  MUL_rrr(REG_WORK2, REG_WORK1, REG_WORK3);
	SUB_rrr(REG_WORK2, d, REG_WORK2);		// REG_WORK2 contains remainder
	
	EORS_rrr(REG_WORK3, REG_WORK2, d);	// If sign of remainder and first operand differs, change sign of remainder
	CC_RSB_rri(NATIVE_CC_MI, REG_WORK2, REG_WORK2, 0);
	
	PKHBT_rrrLSLi(d, REG_WORK1, REG_WORK2, 16);

// end_of_op
	
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_DIVS,(RW4 d, RR4 s))

MIDFUNC(3,jnf_DIVLU32,(RW4 d, RR4 s1, W4 rem))
{
  s1 = readreg(s1);
  d = rmw(d);
  rem = writereg(rem);

  TST_rr(s1, s1);
  BNE_i(4);     // src is not 0

  // Signal exception 5
	MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
#ifdef ARM_HAS_DIV
	B_i(3);        // end_of_op

	// src is not 0  
	UDIV_rrr(REG_WORK1, d, s1);
#else
	B_i(9);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, s1, 0);
	VCVTIuto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIuto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toIu_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif

  // Here we have to calc remainder
  MUL_rrr(REG_WORK2, s1, REG_WORK1);
  SUB_rrr(rem, d, REG_WORK2);
  MOV_rr(d, REG_WORK1);

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

  TST_rr(s1, s1);
  BNE_i(6);     // src is not 0

  // Signal exception 5
	MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
  
  // simplified flag handling for div0: set Z and V (for signed DIV: Z only)
  MOV_ri(REG_WORK1, ARM_Z_FLAG);
  MSR_CPSRf_r(REG_WORK1);
#ifdef ARM_HAS_DIV
	B_i(7);        // end_of_op

	// src is not 0  
	UDIV_rrr(REG_WORK1, d, s1);
#else
	B_i(13);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, s1, 0);
	VCVTIuto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIuto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toIu_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif

  // Here we have to calc flags and remainder
  TST_rr(REG_WORK1, REG_WORK1);
	MRS_CPSR(REG_WORK2);
	BIC_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG | ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK2);
  
  MUL_rrr(REG_WORK2, s1, REG_WORK1);
  SUB_rrr(rem, d, REG_WORK2);
  MOV_rr(d, REG_WORK1);

// end_of_op
	
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

  TST_rr(s1, s1);
  BNE_i(4);     // src is not 0

  // Signal exception 5
	MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
#ifdef ARM_HAS_DIV
	B_i(5);        // end_of_op

	// src is not 0  
	SDIV_rrr(REG_WORK1, d, s1);
#else
	B_i(11);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, s1, 0);
	VCVTIto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toI_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif

  // Here we have to calc remainder
  MUL_rrr(REG_WORK2, s1, REG_WORK1);
  SUB_rrr(rem, d, REG_WORK2);

	EORS_rrr(REG_WORK3, rem, d);	// If sign of remainder and first operand differs, change sign of remainder
	CC_RSB_rri(NATIVE_CC_MI, rem, rem, 0);
  MOV_rr(d, REG_WORK1);

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

  TST_rr(s1, s1);
  BNE_i(4);     // src is not 0

  // Signal exception 5
	MOV_ri(REG_WORK1, 5);
  MOVW_ri16(REG_WORK2, (uae_u32)(&jit_exception));
  MOVT_ri16(REG_WORK2, ((uae_u32)(&jit_exception)) >> 16);
  STR_rR(REG_WORK1, REG_WORK2);
#ifdef ARM_HAS_DIV
	B_i(3);        // end_of_op

	// src is not 0  
	SDIV_rrr(REG_WORK1, d, s1);
#else
	B_i(9);       // end_of_op
	
	// src is not 0  
	VMOVi_from_ARM_dr(SCRATCH_F64_1, d, 0);
	VMOVi_from_ARM_dr(SCRATCH_F64_2, s1, 0);
	VCVTIto64_ds(SCRATCH_F64_1, SCRATCH_F32_1);
	VCVTIto64_ds(SCRATCH_F64_2, SCRATCH_F32_2);
	VDIV64_ddd(SCRATCH_F64_3, SCRATCH_F64_1, SCRATCH_F64_2);
	VCVT64toI_sd(SCRATCH_F32_1, SCRATCH_F64_3);
	VMOVi_to_ARM_rd(REG_WORK1, SCRATCH_F64_1, 0);
#endif

  // Here we have to calc remainder
  MUL_rrr(REG_WORK2, s1, REG_WORK1);
  SUB_rrr(rem, d, REG_WORK2);

	EORS_rrr(REG_WORK3, rem, d);	// If sign of remainder and first operand differs, change sign of remainder
	CC_RSB_rri(NATIVE_CC_MI, rem, rem, 0);
  MOV_rr(d, REG_WORK1);

// end_of_op
	
  unlock2(rem);
  unlock2(d);
  unlock2(s1);
}
MENDFUNC(3,jff_DIVLS32,(RW4 d, RR4 s1, W4 rem))

#endif

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
MIDFUNC(2,jnf_EOR_b_imm,(RW1 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | ((live.state[d].val ^ v) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);

	EOR_rri(d, d, (v & 0xff));

	unlock2(d);
}
MENDFUNC(2,jnf_EOR_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jnf_EOR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_EOR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		EOR_rrr(REG_WORK1, d, s);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		EOR_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_EOR_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_EOR_w_imm,(RW2 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | ((live.state[d].val ^ v) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	if(targetIsReg) {
	  if(CHECK32(v & 0xffff)) {
	    EOR_rri(REG_WORK1, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  EOR_rrr(REG_WORK1, d, REG_WORK1);
	  }
	  PKHTB_rrr(d, d, REG_WORK1);
	} else{
	  if(CHECK32(v & 0xffff)) {
	    EOR_rri(d, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  EOR_rrr(d, d, REG_WORK1);
	  }
	}

	unlock2(d);
}
MENDFUNC(2,jnf_EOR_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jnf_EOR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_EOR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		EOR_rrr(REG_WORK1, d, s);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		EOR_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_EOR_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_EOR_l_imm,(RW4 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val ^ v);
		return;
	}

	d = rmw(d);

  if(CHECK32(v)) {
    EOR_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK1, v);
	  EOR_rrr(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_EOR_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jnf_EOR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_EOR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	EOR_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_EOR_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_EOR_b_imm,(RW1 d, IMM v))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_IMM_2_REG(REG_WORK2, v);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		EORS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		EORS_rrr(d, REG_WORK1, REG_WORK2);
	}

	unlock2(d);
}
MENDFUNC(2,jff_EOR_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jff_EOR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_EOR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_REG_2_REG(REG_WORK2, s);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		EORS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		EORS_rrr(d, REG_WORK1, REG_WORK2);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_EOR_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_EOR_w_imm,(RW2 d, IMM v))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_IMM_2_REG(REG_WORK2, v);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		EORS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		EORS_rrr(d, REG_WORK1, REG_WORK2);
	}

	unlock2(d);
}
MENDFUNC(2,jff_EOR_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jff_EOR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_EOR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_REG_2_REG(REG_WORK2, s);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		EORS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		EORS_rrr(d, REG_WORK1, REG_WORK2);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_EOR_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_EOR_l_imm,(RW4 d, IMM v))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
  if(CHECK32(v)) {
    EORS_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK2, v);
	  EORS_rrr(d, d, REG_WORK2);
  }

	unlock2(d);
}
MENDFUNC(2,jff_EOR_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jff_EOR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_EOR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	MSR_CPSRf_i(0);
	EORS_rrr(d, d, s);

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
MIDFUNC(1,jff_EORSR,(IMM s, IMM x))
{
	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, s);
	MSR_CPSRf_r(REG_WORK1);

	if (x) {
		int f = rmw(FLAGX);
		EOR_rri(f, f, 1);
		unlock2(f);
	}
}
MENDFUNC(1,jff_EORSR,(IMM s))

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
		set_const(d, (uae_s32)(uae_s8)live.state[d].val);
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
		set_const(d, (live.state[d].val & 0xffff0000) | (((uae_s32)(uae_s8)live.state[d].val) & 0x0000ffff));
		return;
	}

	d = rmw(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	PKHTB_rrr(d, d, REG_WORK1);
	
	unlock2(d);
}
MENDFUNC(1,jnf_EXT_w,(RW4 d))

MIDFUNC(1,jnf_EXT_l,(RW4 d))
{
	if (isconst(d)) {
		set_const(d, (uae_s32)(uae_s16)live.state[d].val);
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

	MSR_CPSRf_i(0);
	TST_rr(d, d);

	unlock2(d);
}
MENDFUNC(1,jff_EXT_b,(RW4 d))

MIDFUNC(1,jff_EXT_w,(RW4 d))
{
	if (isconst(d)) {
    uae_u8 tmp = (uae_u8)live.state[d].val;
	  d = rmw(d);
		SIGNED8_IMM_2_REG(REG_WORK1, tmp);
	} else {
		d = rmw(d);
	  SIGNED8_REG_2_REG(REG_WORK1, d);
	}

	MSR_CPSRf_i(0);
	TST_rr(REG_WORK1, REG_WORK1);
	PKHTB_rrr(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(1,jff_EXT_w,(RW4 d))

MIDFUNC(1,jff_EXT_l,(RW4 d))
{
	if (isconst(d)) {
    uae_u16 tmp = (uae_u8)live.state[d].val;
		d = writereg(d);
		SIGNED16_IMM_2_REG(d, tmp);
	} else {
	  d = rmw(d);
	  SIGNED16_REG_2_REG(d, d);
	}
	MSR_CPSRf_i(0);
	TST_rr(d, d);

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
 * imm version only called with 1 <= i <= 8
 *
 */
MIDFUNC(2,jnf_LSL_b_imm,(RW1 d, IMM i))
{
  if(i) {
		if (isconst(d)) {
			set_const(d, (live.state[d].val & 0xffffff00) | ((live.state[d].val << i) & 0x000000ff));
			return;
		}
		INIT_REG_b(d);
	  
  	LSL_rri(REG_WORK1, d, i);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	
		unlock2(d);
 	}
}
MENDFUNC(2,jnf_LSL_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jnf_LSL_w_imm,(RW2 d, IMM i))
{
  if(i) {
		if (isconst(d)) {
			set_const(d, (live.state[d].val & 0xffff0000) | ((live.state[d].val << i) & 0x0000ffff));
			return;
		}
		INIT_REG_w(d);
	  
  	LSL_rri(REG_WORK1, d, i);
	  PKHTB_rrr(d, d, REG_WORK1);
	
		unlock2(d);
 	}
}
MENDFUNC(2,jnf_LSL_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jnf_LSL_l_imm,(RW4 d, IMM i))
{
  if(i) {
		if (isconst(d)) {
			set_const(d, live.state[d].val << i);
			return;
		}
		d = rmw(d);
		
	  LSL_rri(d, d, i);
		
		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSL_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jnf_LSL_b_reg,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSL_b_imm)(d, live.state[i].val & 15);
		return;
	}
	INIT_REGS_b(d, i);

	AND_rri(REG_WORK1, i, 63);
	LSL_rrr(REG_WORK1, d, REG_WORK1);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  AND_rri(REG_WORK1, REG_WORK1, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK1);
#endif

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSL_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jnf_LSL_w_reg,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSL_w_imm)(d, live.state[i].val & 31);
		return;
	}
	INIT_REGS_w(d, i);

	AND_rri(REG_WORK1, i, 63);
	LSL_rrr(REG_WORK1, d, REG_WORK1);
  PKHTB_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSL_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jnf_LSL_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		if(i > 31)
			set_const(d, 0);
		else
			COMPCALL(jnf_LSL_l_imm)(d, live.state[i].val & 31);
		return;
	}
	INIT_REGS_l(d, i);

	AND_rri(REG_WORK1, i, 63);
	LSL_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSL_l_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_LSL_b_imm,(RW1 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);

	if (i) {
		LSLS_rri(REG_WORK3, d, i + 24);
		DUPLICACTE_CARRY
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK3, 24);
	} else {
		LSL_rri(REG_WORK3, d, 24);
		TST_rr(REG_WORK3, REG_WORK3);
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSL_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jff_LSL_w_imm,(RW2 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);

	if (i) {
		LSLS_rri(REG_WORK3, d, i + 16);
	  PKHTB_rrrASRi(d, d, REG_WORK3, 16);
		DUPLICACTE_CARRY
	} else {
		LSL_rri(REG_WORK3, d, 16);
		TST_rr(REG_WORK3, REG_WORK3);
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSL_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jff_LSL_l_imm,(RW4 d, IMM i))
{
	if(i)
	  d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);
	if (i) {
		LSLS_rri(d, d, i);
		DUPLICACTE_CARRY
	} else {
		TST_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSL_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jff_LSL_b_reg,(RW1 d, RR4 i))
{
	INIT_REGS_b(d, i);
  int x = writereg(FLAGX);

	MSR_CPSRf_i(0);
	LSL_rri(REG_WORK3, d, 24);
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(5);               // No shift -> X flag unchanged
	LSLS_rrr(REG_WORK3, REG_WORK3, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
  BIC_rri(d, d, 0xff);
  ORR_rrrLSRi(d, d, REG_WORK3, 24);
	B_i(0);
	TST_rr(REG_WORK3, REG_WORK3);

  unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSL_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_LSL_w_reg,(RW2 d, RR4 i))
{
	INIT_REGS_w(d, i);
  int x = writereg(FLAGX);

	MSR_CPSRf_i(0);
	LSL_rri(REG_WORK3, d, 16);
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(4);               // No shift -> X flag unchanged
	LSLS_rrr(REG_WORK3, REG_WORK3, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
  PKHTB_rrrASRi(d, d, REG_WORK3, 16);
	B_i(0);
	TST_rr(REG_WORK3, REG_WORK3);

  unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSL_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jff_LSL_l_reg,(RW4 d, RR4 i))
{
	INIT_REGS_l(d, i);
  int x = writereg(FLAGX);

	MSR_CPSRf_i(0);
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(3);               // No shift -> X flag unchanged
	LSLS_rrr(d, d, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
	B_i(0);
	TST_rr(d, d);

  unlock2(x);
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
		set_const(d, (live.state[d].val << 1) & 0xffff);
		return;
	}

	d = rmw(d);

	LSL_rri(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_LSLW,(RW2 d))

MIDFUNC(1,jff_LSLW,(RW2 d))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
	LSLS_rri(d, d, 17);
	LSR_rri(d, d, 16);
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
 * imm version only called with 1 <= i <= 8
 *
 */
MIDFUNC(2,jnf_LSR_b_imm,(RW1 d, IMM i))
{
	if(i) {
		if (isconst(d)) {
			set_const(d, (live.state[d].val & 0xffffff00) | ((live.state[d].val >> i) & 0x000000ff));
			return;
		}
		INIT_REG_b(d);

		UNSIGNED8_REG_2_REG(REG_WORK1, d);
  	LSR_rri(REG_WORK1, REG_WORK1, i);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif

		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSR_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jnf_LSR_w_imm,(RW2 d, IMM i))
{
	if(i) {
		if (isconst(d)) {
			set_const(d, (live.state[d].val & 0xffff0000) | ((live.state[d].val >> i) & 0x0000ffff));
			return;
		}
		INIT_REG_w(d);

		UNSIGNED16_REG_2_REG(REG_WORK1, d);
	  PKHTB_rrrASRi(d, d, REG_WORK1, i);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSR_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jnf_LSR_l_imm,(RW4 d, IMM i))
{
	if(i) {
		if (isconst(d)) {
			set_const(d, live.state[d].val >> i);
			return;
		}
		d = rmw(d);

  	LSR_rri(d, d, i);

		unlock2(d);
	}
}
MENDFUNC(2,jnf_LSR_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jff_LSR_b_imm,(RW1 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	UNSIGNED8_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	if (i) {
		LSRS_rri(REG_WORK1, REG_WORK1, i);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
		DUPLICACTE_CARRY
	} else {
		TST_rr(REG_WORK1, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSR_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jff_LSR_w_imm,(RW2 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	UNSIGNED16_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	if (i) {
		LSRS_rri(REG_WORK1, REG_WORK1, i);
	  PKHTB_rrr(d, d, REG_WORK1);
		DUPLICACTE_CARRY
	} else {
		TST_rr(REG_WORK1, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSR_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jff_LSR_l_imm,(RW4 d, IMM i))
{
	if(i)
	  d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);
	if (i) {
		LSRS_rri(d, d, i);
		DUPLICACTE_CARRY
	} else {
		TST_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(2,jff_LSR_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jnf_LSR_b_reg,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSR_b_imm)(d, live.state[i].val & 15);
		return;
	}
	INIT_REGS_b(d, i);

	UNSIGNED8_REG_2_REG(REG_WORK1, d);
	AND_rri(REG_WORK2, i, 63);
	LSR_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  AND_rri(REG_WORK1, REG_WORK1, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK1);
#endif

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jnf_LSR_w_reg,(RW2 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_LSR_w_imm)(d, live.state[i].val & 31);
		return;
	}
	INIT_REGS_w(d, i);

	UNSIGNED16_REG_2_REG(REG_WORK1, d);
	AND_rri(REG_WORK2, i, 63);
	LSR_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  PKHTB_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jnf_LSR_l_reg,(RW4 d, RR4 i))
{
	if (isconst(i)) {
		if(i > 31)
			set_const(d, 0);
		else
			COMPCALL(jnf_LSR_l_imm)(d, live.state[i].val & 31);
		return;
	}
	INIT_REGS_l(d, i);

	AND_rri(REG_WORK1, i, 63);
	LSR_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_LSR_l_reg,(RW4 d, RR4 i))

MIDFUNC(2,jff_LSR_b_reg,(RW1 d, RR4 i))
{
	INIT_REGS_b(d, i);
  int x = writereg(FLAGX);

	MSR_CPSRf_i(0);
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(4);                       // No shift -> X flag unchanged
  AND_rri(REG_WORK2, d, 0xff);            // Shift count is not 0 -> unsigned required
	LSRS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
	B_i(1);
	SIGNED8_REG_2_REG(REG_WORK2, d);        // Make sure, sign is in MSB if shift count is 0 (to get correct N flag)
	TST_rr(REG_WORK2, REG_WORK2);

#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif

  unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSR_b_reg,(RW1 d, RR4 i))

MIDFUNC(2,jff_LSR_w_reg,(RW2 d, RR4 i))
{
	INIT_REGS_w(d, i);
  int x = writereg(FLAGX);

  MSR_CPSRf_i(0); 
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(5);                       // No shift -> X flag unchanged
	UXTH_rr(REG_WORK2, d);                  // Shift count is not 0 -> unsigned required
	LSRS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
  PKHTB_rrr(d, d, REG_WORK2);
	B_i(1);
	SIGNED16_REG_2_REG(REG_WORK2, d);       // Make sure, sign is in MSB if shift count is 0 (to get correct N flag)
	TST_rr(REG_WORK2, REG_WORK2);

  unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_LSR_w_reg,(RW2 d, RR4 i))

MIDFUNC(2,jff_LSR_l_reg,(RW4 d, RR4 i))
{
	INIT_REGS_l(d, i);
  int x = writereg(FLAGX);

  MSR_CPSRf_i(0); 
	ANDS_rri(REG_WORK1, i, 63);
	BEQ_i(3);                       // No shift -> X flag unchanged
	LSRS_rrr(d, d, REG_WORK1);
	MOV_ri(x, 1);
	CC_MOV_ri(NATIVE_CC_CC, x, 0);
	B_i(0);
	TST_rr(d, d);

  unlock2(x);
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
		set_const(d, (live.state[d].val >> 1) & 0xffff);
		return;
	}
	
	d = rmw(d);

	UNSIGNED16_REG_2_REG(d, d);
	LSR_rri(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_LSRW,(RW2 d))

MIDFUNC(1,jff_LSRW,(RW2 d))
{
	d = rmw(d);

	UNSIGNED16_REG_2_REG(d, d);
	MSR_CPSRf_i(0);
	LSRS_rri(d, d, 1);
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
MIDFUNC(2,jnf_MOVE_b_imm,(W1 d, IMM s))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | (s & 0x000000ff));
		return;
	}
	d = rmw(d);

	MOV_ri(REG_WORK1, s & 0xff);
#ifdef ARMV6T2
	BFI_rrii(d, REG_WORK1, 0, 7);
#else
  AND_rri(REG_WORK1, REG_WORK1, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif

	unlock2(d);
}
MENDFUNC(2,jnf_MOVE_b_imm,(W1 d, IMM s))

MIDFUNC(2,jnf_MOVE_w_imm,(W2 d, IMM s))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | (s & 0x0000ffff));
		return;
	}
	d = rmw(d);

	UNSIGNED16_IMM_2_REG(REG_WORK1, s & 0xffff);
  PKHTB_rrr(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_MOVE_w_imm,(W2 d, IMM s))

MIDFUNC(2,jnf_MOVE_l_imm,(W4 d, IMM s))
{
	set_const(d, s);
}
MENDFUNC(2,jnf_MOVE_l_imm,(W4 d, IMM s))

MIDFUNC(2,jnf_MOVE_b,(W1 d, RR1 s))
{
	if(d >= 16) {
		mov_l_rr(d, s);
		return;
	}
	if(s == d)
		return;
	if (isconst(s)) {
		COMPCALL(jnf_MOVE_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

#ifdef ARMV6T2
  BFI_rrii(d, s, 0, 7);
#else
  AND_rri(REG_WORK1, s, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MOVE_b,(W1 d, RR1 s))

MIDFUNC(2,jnf_MOVE_w,(W2 d, RR2 s))
{
	if(d >= 16) {
		mov_l_rr(d, s);
		return;
	}
	if(s == d)
		return;
	if (isconst(s)) {
		COMPCALL(jnf_MOVE_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

  PKHTB_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MOVE_w,(W2 d, RR2 s))

MIDFUNC(2,jnf_MOVE_l,(W4 d, RR4 s))
{
	mov_l_rr(d, s);
}
MENDFUNC(2,jnf_MOVE_l,(W4 d, RR4 s))

MIDFUNC(2,jff_MOVE_b_imm,(W1 d, IMM s))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
	if (s & 0x80) {
		MVNS_ri(REG_WORK1, (uae_u8) ~s);
	} else {
		MOVS_ri(REG_WORK1, (uae_u8) s);
	}
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  AND_rri(REG_WORK1, REG_WORK1, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif

	unlock2(d);
}
MENDFUNC(2,jff_MOVE_b_imm,(W1 d, IMM s))

MIDFUNC(2,jff_MOVE_w_imm,(W2 d, IMM s))
{
	d = rmw(d);

	SIGNED16_IMM_2_REG(REG_WORK1, (uae_u16)s);
	MSR_CPSRf_i(0);
	TST_rr(REG_WORK1, REG_WORK1);
  PKHTB_rrr(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jff_MOVE_w_imm,(W2 d, IMM s))

MIDFUNC(2,jff_MOVE_l_imm,(W4 d, IMM s))
{
	d = writereg(d);

	compemu_raw_mov_l_ri(d, s);
	MSR_CPSRf_i(0);
	TST_rr(d, d);

	unlock2(d);
}
MENDFUNC(2,jff_MOVE_l_imm,(W4 d, IMM s))

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
	MSR_CPSRf_i(0);
	TST_rr(REG_WORK1, REG_WORK1);
	if(!s_is_d) {
#ifdef ARMV6T2
    BFI_rrii(d, REG_WORK1, 0, 7);
#else
    AND_rri(REG_WORK1, REG_WORK1, 0xff);
    BIC_rri(d, d, 0xff);
    ORR_rrr(d, d, REG_WORK2);
#endif
  }

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
	MSR_CPSRf_i(0);
	TST_rr(REG_WORK1, REG_WORK1);
  if(!s_is_d)
    PKHTB_rrr(d, d, REG_WORK1);

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
	if(!s_is_d)
	  d = writereg(d);

	MSR_CPSRf_i(0);
	if(!s_is_d)
	  MOVS_rr(d, s);
  else
    TST_rr(s, s);

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
MIDFUNC(3,jnf_MVMEL_w,(W4 d, RR4 s, IMM offset))
{
	s = readreg(s);
	d = writereg(d);

	LDRH_rRI(REG_WORK1, s, offset);
  REVSH_rr(d, REG_WORK1);
  SXTH_rr(d, d);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,jnf_MVMEL_w,(W4 d, RR4 s, IMM offset))

MIDFUNC(3,jnf_MVMEL_l,(W4 d, RR4 s, IMM offset))
{
	s = readreg(s);
	d = writereg(d);

	LDR_rRI(REG_WORK1, s, offset);
  REV_rr(d, REG_WORK1);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,jnf_MVMEL_l,(W4 d, RR4 s, IMM offset))

/*
 * MVMLE
 *
 * Flags: Not affected.
 *
 */
MIDFUNC(3,jnf_MVMLE_w,(RR4 d, RR4 s, IMM offset))
{
	s = readreg(s);
	d = readreg(d);

	REV16_rr(REG_WORK1, s);
	if (offset >= 0)
		STRH_rRI(REG_WORK1, d, offset);
	else
		STRH_rRi(REG_WORK1, d, -offset);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,jnf_MVMLE_w,(RR4 d, RR4 s, IMM offset))

MIDFUNC(3,jnf_MVMLE_l,(RR4 d, RR4 s, IMM offset))
{
	s = readreg(s);
	d = readreg(d);

	REV_rr(REG_WORK1, s);
	if (offset >= 0)
		STR_rRI(REG_WORK1, d, offset);
	else
		STR_rRi(REG_WORK1, d, -offset);

	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,jnf_MVMLE_l,(RR4 d, RR4 s, IMM offset))

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

  PUSH_REGS((1 << s) | (1 << d));
  
	BIC_rri(s, s, 0x0000000F);
	BIC_rri(d, d, 0x0000000F);

	ADD_rrr(s, s, R_MEMSTART);
	ADD_rrr(d, d, R_MEMSTART);

#ifdef ARMV6T2
	LDRD_rR(REG_WORK1, s);
	STRD_rR(REG_WORK1, d);

	LDRD_rRI(REG_WORK1, s, 8);
	STRD_rRI(REG_WORK1, d, 8);
#else
	LDR_rR(REG_WORK1, s);
	LDR_rRI(REG_WORK2, s, 4);
	STR_rR(REG_WORK1, d);
	STR_rRI(REG_WORK2, d, 4);

	LDR_rRI(REG_WORK1, s, 8);
	LDR_rRI(REG_WORK2, s, 12);
	STR_rRI(REG_WORK1, d, 8);
	STR_rRI(REG_WORK2, d, 12);
#endif

  POP_REGS((1 << s) | (1 << d));

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
MIDFUNC(2,jnf_MOVEA_w,(W4 d, RR2 s))
{
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
	INIT_REGS_l(d, s);

  SMULxy_rrr(d, d, s, 0, 0);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULS,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULS,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	SIGN_EXTEND_16_REG_2_REG(d, d);
	SIGN_EXTEND_16_REG_2_REG(REG_WORK1, s);

	MSR_CPSRf_i(0);
	MULS_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULS,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULS32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	MUL_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULS32,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULS32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	MSR_CPSRf_i(0);
	// L, H,
	SMULLS_rrrr(d, REG_WORK2, d, s);
	MRS_CPSR(REG_WORK1);
	TEQ_rrASRi(REG_WORK2, d, 31);
	CC_ORR_rri(NATIVE_CC_NE, REG_WORK1, REG_WORK1, ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULS32,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULS64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

	// L, H,
	SMULL_rrrr(d, s, d, s);

	unlock2(s);
	unlock2(d);
}
MENDFUNC(2,jnf_MULS64,(RW4 d, RW4 s))

MIDFUNC(2,jff_MULS64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

	MSR_CPSRf_i(0);
	// L, H,
	SMULLS_rrrr(d, s, d, s);
	MRS_CPSR(REG_WORK1);
	TEQ_rrASRi(s, d, 31);
	CC_ORR_rri(NATIVE_CC_NE, REG_WORK1, REG_WORK1, ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK1);

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
	INIT_REGS_l(d, s);

	ZERO_EXTEND_16_REG_2_REG(d, d);
	ZERO_EXTEND_16_REG_2_REG(REG_WORK1, s);

	MUL_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULU,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULU,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	ZERO_EXTEND_16_REG_2_REG(d, d);
	ZERO_EXTEND_16_REG_2_REG(REG_WORK1, s);

	MSR_CPSRf_i(0);
	MULS_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULU,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULU32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	MUL_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_MULU32,(RW4 d, RR4 s))

MIDFUNC(2,jff_MULU32,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	// L, H,
	MSR_CPSRf_i(0);
	UMULLS_rrrr(d, REG_WORK2, d, s);
	MRS_CPSR(REG_WORK1);
	TST_rr(REG_WORK2, REG_WORK2);
	CC_ORR_rri(NATIVE_CC_NE, REG_WORK1, REG_WORK1, ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_MULU32,(RW4 d, RR4 s))

MIDFUNC(2,jnf_MULU64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

	// L, H,
	UMULL_rrrr(d, s, d, s);

	unlock2(s);
	unlock2(d);
}
MENDFUNC(2,jnf_MULU64,(RW4 d, RW4 s))

MIDFUNC(2,jff_MULU64,(RW4 d, RW4 s))
{
	s = rmw(s);
	d = rmw(d);

	// L, H,
	MSR_CPSRf_i(0);
	UMULLS_rrrr(d, s, d, s);
	MRS_CPSR(REG_WORK1);
	TST_rr(s, s);
	CC_ORR_rri(NATIVE_CC_NE, REG_WORK1, REG_WORK1, ARM_V_FLAG);
	MSR_CPSRf_r(REG_WORK1);

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
 * C Cleared if the result is zero. Set otherwise.
 *
 */
MIDFUNC(1,jnf_NEG_b,(RW1 d))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | ((0 - (uae_u8)live.state[d].val) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSB_rri(REG_WORK1, REG_WORK1, 0);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		RSB_rri(d, REG_WORK1, 0);
	}

	unlock2(d);
}
MENDFUNC(1,jnf_NEG_b,(RW1 d))

MIDFUNC(1,jnf_NEG_w,(RW2 d))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | ((0 - (uae_u16)live.state[d].val) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSB_rri(REG_WORK1, REG_WORK1, 0);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		RSB_rri(d, REG_WORK1, 0);
	}

	unlock2(d);
}
MENDFUNC(1,jnf_NEG_w,(RW2 d))

MIDFUNC(1,jnf_NEG_l,(RW4 d))
{
	if (isconst(d)) {
		set_const(d, 0 - (uae_u32)live.state[d].val);
		return;
	}

	d = rmw(d);

	RSB_rri(d, d, 0);

	unlock2(d);
}
MENDFUNC(1,jnf_NEG_l,(RW4 d))

MIDFUNC(1,jff_NEG_b,(RW1 d))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSBS_rri(REG_WORK1, REG_WORK1, 0);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		RSBS_rri(d, REG_WORK1, 0);
	}
  
	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	unlock2(d);
}
MENDFUNC(1,jff_NEG_b,(RW1 d))

MIDFUNC(1,jff_NEG_w,(RW2 d))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSBS_rri(REG_WORK1, REG_WORK1, 0);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		RSBS_rri(d, REG_WORK1, 0);
	}

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	unlock2(d);
}
MENDFUNC(1,jff_NEG_w,(RW2 d))

MIDFUNC(1,jff_NEG_l,(RW4 d))
{
	d = rmw(d);

	RSBS_rri(d, d, 0);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

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
 * C Cleared if the result is zero. Set otherwise.
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
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSC_rri(REG_WORK1, REG_WORK1, 0);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		RSC_rri(d, REG_WORK1, 0);
	}

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
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSC_rri(REG_WORK1, REG_WORK1, 0);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		RSC_rri(d, REG_WORK1, 0);
	}

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
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	RSC_rri(d, d, 0);

	unlock2(d);
	unlock2(x);
}
MENDFUNC(1,jnf_NEGX_l,(RW4 d))

MIDFUNC(1,jff_NEGX_b,(RW1 d))
{
	INIT_REG_b(d);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore inverted X to carry (don't care about other flags)
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSCS_rri(REG_WORK1, REG_WORK1, 0);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		RSCS_rri(d, REG_WORK1, 0);
	}

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

	unlock2(x);
	unlock2(d);
}
MENDFUNC(1,jff_NEGX_b,(RW1 d))

MIDFUNC(1,jff_NEGX_w,(RW2 d))
{
	INIT_REG_w(d);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore inverted X to carry (don't care about other flags)
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	if(targetIsReg) {
		RSCS_rri(REG_WORK1, REG_WORK1, 0);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		RSCS_rri(d, REG_WORK1, 0);
	}

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

	unlock2(x);
	unlock2(d);
}
MENDFUNC(1,jff_NEGX_w,(RW2 d))

MIDFUNC(1,jff_NEGX_l,(RW4 d))
{
	d = rmw(d);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore inverted X to carry (don't care about other flags)
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	RSCS_rri(d, d, 0);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

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
		set_const(d, (live.state[d].val & 0xffffff00) | ((~live.state[d].val) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);

	if(targetIsReg) {
		MVN_rr(REG_WORK1, d);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		MVN_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(1,jnf_NOT_b,(RW1 d))

MIDFUNC(1,jnf_NOT_w,(RW2 d))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | ((~live.state[d].val) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	if(targetIsReg) {
		MVN_rr(REG_WORK1, d);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		MVN_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(1,jnf_NOT_w,(RW2 d))

MIDFUNC(1,jnf_NOT_l,(RW4 d))
{
	if (isconst(d)) {
		set_const(d, ~live.state[d].val);
		return;
	}

	d = rmw(d);

	MVN_rr(d, d);

	unlock2(d);
}
MENDFUNC(1,jnf_NOT_l,(RW4 d))

MIDFUNC(1,jff_NOT_b,(RW1 d))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		MVNS_rr(REG_WORK1, REG_WORK1);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		MVNS_rr(d, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(1,jff_NOT_b,(RW1 d))

MIDFUNC(1,jff_NOT_w,(RW2 d))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		MVNS_rr(REG_WORK1, REG_WORK1);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		MVNS_rr(d, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(1,jff_NOT_w,(RW2 d))

MIDFUNC(1,jff_NOT_l,(RW4 d))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
	MVNS_rr(d, d);

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
MIDFUNC(2,jnf_OR_b_imm,(RW1 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | ((live.state[d].val | v) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);
	
	if(targetIsReg) {
	  ORR_rri(REG_WORK1, d, v & 0xff);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
	  ORR_rri(d, d, v & 0xff);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_OR_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jnf_OR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_OR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		ORR_rrr(REG_WORK1, d, s);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		ORR_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_OR_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_OR_w_imm,(RW2 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | ((live.state[d].val | v) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	if(targetIsReg) {
	  if(CHECK32(v & 0xffff)) {
	    ORR_rri(REG_WORK1, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  ORR_rrr(REG_WORK1, d, REG_WORK1);
	  }
	  PKHTB_rrr(d, d, REG_WORK1);
	} else{
	  if(CHECK32(v & 0xffff)) {
	    ORR_rri(d, d, (v & 0xffff));
	  } else {
		  compemu_raw_mov_l_ri(REG_WORK1, v & 0xffff);
		  ORR_rrr(d, d, REG_WORK1);
	  }
	}
	
	unlock2(d);
}
MENDFUNC(2,jnf_OR_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jnf_OR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_OR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		ORR_rrr(REG_WORK1, d, s);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		ORR_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_OR_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_OR_l_imm,(RW4 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val | v);
		return;
	}

	d = rmw(d);

  if(CHECK32(v)) {
    ORR_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK1, v);
	  ORR_rrr(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jnf_OR_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jnf_OR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_OR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	ORR_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_OR_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_OR_b_imm,(RW1 d, IMM v))
{
	INIT_REG_b(d);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_IMM_2_REG(REG_WORK2, v);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ORRS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		ORRS_rrr(d, REG_WORK1, REG_WORK2);
	}

	unlock2(d);
}
MENDFUNC(2,jff_OR_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jff_OR_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_OR_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	SIGNED8_REG_2_REG(REG_WORK1, d);
	SIGNED8_REG_2_REG(REG_WORK2, s);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ORRS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		ORRS_rrr(d, REG_WORK1, REG_WORK2);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_OR_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_OR_w_imm,(RW2 d, IMM v))
{
	INIT_REG_w(d);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_IMM_2_REG(REG_WORK2, v);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ORRS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		ORRS_rrr(d, REG_WORK1, REG_WORK2);
	}

	unlock2(d);
}
MENDFUNC(2,jff_OR_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jff_OR_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_OR_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, d);
	SIGNED16_REG_2_REG(REG_WORK2, s);
	MSR_CPSRf_i(0);
	if(targetIsReg) {
		ORRS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else {
		ORRS_rrr(d, REG_WORK1, REG_WORK2);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_OR_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_OR_l_imm,(RW4 d, IMM v))
{
	d = rmw(d);

	MSR_CPSRf_i(0);
  if(CHECK32(v)) {
    ORRS_rri(d, d, v);
  } else {
	  compemu_raw_mov_l_ri(REG_WORK2, v);
	  ORRS_rrr(d, d, REG_WORK2);
  }

	unlock2(d);
}
MENDFUNC(2,jff_OR_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jff_OR_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_OR_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	MSR_CPSRf_i(0);
	ORRS_rrr(d, d, s);

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
MIDFUNC(1,jff_ORSR,(IMM s, IMM x))
{
	MRS_CPSR(REG_WORK1);
	ORR_rri(REG_WORK1, REG_WORK1, s);
	MSR_CPSRf_r(REG_WORK1);

	if (x) {
		int f = writereg(FLAGX);
		MOV_ri(f, 1);
		unlock2(f);
	}
}
MENDFUNC(1,jff_ORSR,(IMM s))

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
MIDFUNC(2,jnf_ROL_b_imm,(RW1 d, IMM i))
{
  if(i & 0x1f) {
		INIT_REG_b(d);

  	LSL_rri(REG_WORK1, d, 24);
  	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
  	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
  	ROR_rri(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
  
		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROL_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jnf_ROL_w_imm,(RW2 d, IMM i))
{
  if(i & 0x1f) {
		INIT_REG_w(d);

  	PKHBT_rrrLSLi(REG_WORK1, d, d, 16);
  	ROR_rri(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
	  PKHTB_rrr(d, d, REG_WORK1);

		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROL_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jnf_ROL_l_imm,(RW4 d, IMM i))
{
  if(i & 0x1f) {
		d = rmw(d);

  	ROR_rri(d, d, (32 - (i & 0x1f)));
	
		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROL_l_imm,(RW4 d, RR4 s, IMM i))

MIDFUNC(2,jff_ROL_b_imm,(RW1 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	LSL_rri(REG_WORK1, d, 24);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	MSR_CPSRf_i(0);
	if (i) {
		if(i & 0x1f)
		  RORS_rri(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
    else
      TST_rr(REG_WORK1, REG_WORK1);     
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif

		MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(REG_WORK2, REG_WORK1, 29, 29); // Handle C flag
#else
		TST_ri(REG_WORK1, 1);
		CC_ORR_rri(NATIVE_CC_NE, REG_WORK2, REG_WORK2, ARM_C_FLAG);
		CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
		MSR_CPSRf_r(REG_WORK2);
	} else {
		TST_rr(REG_WORK1, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ROL_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jff_ROL_w_imm,(RW2 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	PKHBT_rrrLSLi(REG_WORK1, d, d, 16);
	MSR_CPSRf_i(0);
	if (i) {
		if(i & 0x1f)
  		RORS_rri(REG_WORK1, REG_WORK1, (32 - (i & 0x1f)));
    else
      TST_rr(REG_WORK1, REG_WORK1);
	  PKHTB_rrr(d, d, REG_WORK1);

		MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(REG_WORK2, REG_WORK1, 29, 29); // Handle C flag
#else
		TST_ri(d, 1);
		CC_ORR_rri(NATIVE_CC_NE, REG_WORK2, REG_WORK2, ARM_C_FLAG);
		CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
		MSR_CPSRf_r(REG_WORK2);
	} else {
		TST_rr(REG_WORK1, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ROL_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jff_ROL_l_imm,(RW4 d, IMM i))
{
	if(i)
	  d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);
	if (i) {
		if(i & 0x1f)
  		RORS_rri(d, d, (32 - (i & 0x1f)));
    else
      TST_rr(d, d);

		MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
		BFI_rrii(REG_WORK2, d, 29, 29); // Handle C flag
#else
		TST_ri(d, 1);
		CC_ORR_rri(NATIVE_CC_NE, REG_WORK2, REG_WORK2, ARM_C_FLAG);
		CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
		MSR_CPSRf_r(REG_WORK2);
	} else {
		TST_rr(d, d);
	}

	unlock2(d);
}
MENDFUNC(2,jff_ROL_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jnf_ROL_b,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROL_b_imm)(d, (uae_u8)live.state[i].val);
		return;
	}
	INIT_REGS_b(d, i);

	AND_rri(REG_WORK1, i, 0x1f);
	RSB_rri(REG_WORK1, REG_WORK1, 32);

	LSL_rri(REG_WORK2, d, 24);
	ORR_rrrLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 8);
	ORR_rrrLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 16);
	ROR_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif

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

	AND_rri(REG_WORK1, i, 0x1f);
	RSB_rri(REG_WORK1, REG_WORK1, 32);

	PKHBT_rrrLSLi(REG_WORK2, d, d, 16);
	ROR_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
  PKHTB_rrr(d, d, REG_WORK2);

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

	AND_rri(REG_WORK1, i, 0x1f);
	RSB_rri(REG_WORK1, REG_WORK1, 32);

	ROR_rrr(d, d, REG_WORK1);

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

	AND_rri(REG_WORK1, i, 0x1f);
	RSB_rri(REG_WORK1, REG_WORK1, 32);

	LSL_rri(REG_WORK2, d, 24);
	ORR_rrrLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 8);
	ORR_rrrLSRi(REG_WORK2, REG_WORK2, REG_WORK2, 16);
	MSR_CPSRf_i(0);
	RORS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif

	MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, d, 29, 29); // Handle C flag
#else
	TST_ri(d, 1);
	ORR_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG);
	CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
	MSR_CPSRf_r(REG_WORK2);

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

	AND_rri(REG_WORK1, i, 0x1f);
	RSB_rri(REG_WORK1, REG_WORK1, 32);

	PKHBT_rrrLSLi(REG_WORK2, d, d, 16);
	MSR_CPSRf_i(0);
	RORS_rrr(REG_WORK2, REG_WORK2, REG_WORK1);
  PKHTB_rrr(d, d, REG_WORK2);

	MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, d, 29, 29); // Handle C flag
#else
	TST_ri(d, 1);
	ORR_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG);
	CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
	MSR_CPSRf_r(REG_WORK2);

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

	AND_rri(REG_WORK1, i, 0x1f);
	RSB_rri(REG_WORK1, REG_WORK1, 32);

	MSR_CPSRf_i(0);
	RORS_rrr(d, d, REG_WORK1);

	MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, d, 29, 29); // Handle C flag
#else
	TST_ri(d, 1);
	ORR_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG);
	CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
	MSR_CPSRf_r(REG_WORK2);

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

	PKHBT_rrrLSLi(d, d, d, 16);
	ROR_rri(d, d, (32 - 1));

	unlock2(d);
}
MENDFUNC(1,jnf_ROLW,(RW2 d))

MIDFUNC(1,jff_ROLW,(RW2 d))
{
	d = rmw(d);

	PKHBT_rrrLSLi(d, d, d, 16);
	MSR_CPSRf_i(0);
	RORS_rri(d, d, (32 - 1));

	MRS_CPSR(REG_WORK2);
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, d, 29, 29); // Handle C flag
#else
	TST_ri(d, 1);
	ORR_rri(REG_WORK2, REG_WORK2, ARM_C_FLAG);
	CC_BIC_rri(NATIVE_CC_EQ, REG_WORK2, REG_WORK2, ARM_C_FLAG);
#endif
	MSR_CPSRf_r(REG_WORK2);

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

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 35);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 36);
	CMP_ri(REG_WORK1, 17);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 18);
	CMP_ri(REG_WORK1, 8);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 9);
	TST_rr(REG_WORK1, REG_WORK1);
#ifdef ARMV6T2
	BEQ_i(8);			// end of op
#else
	BEQ_i(10);			// end of op
#endif

// need to rotate
	CMP_ri(REG_WORK1, 8);
	CC_MOV_rrLSLr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);
	
	SUB_rri(REG_WORK3, REG_WORK1, 1);	
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);

	RSB_rri(REG_WORK3, REG_WORK1, 9);
	UNSIGNED8_REG_2_REG(REG_WORK1, d);
	ORR_rrrLSRr(REG_WORK2, REG_WORK2, REG_WORK1, REG_WORK3);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXL_b,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ROXL_w,(RW2 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_w(d, i);

	clobber_flags();

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 33);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 34);
	CMP_ri(REG_WORK1, 16);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 17);
	TST_rr(REG_WORK1, REG_WORK1);
	BEQ_i(8);			// end of op

// need to rotate
	CMP_ri(REG_WORK1, 16);
	CC_MOV_rrLSLr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);
	
	SUB_rri(REG_WORK3, REG_WORK1, 1);	
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);

	RSB_rri(REG_WORK3, REG_WORK1, 17);
	UNSIGNED16_REG_2_REG(REG_WORK1, d);
	ORR_rrrLSRr(REG_WORK2, REG_WORK2, REG_WORK1, REG_WORK3);
  PKHTB_rrr(d, d, REG_WORK2);
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXL_w,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ROXL_l,(RW4 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_l(d, i);

	clobber_flags();
	
	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 32);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 33);
	TST_rr(REG_WORK1, REG_WORK1);
	BEQ_i(6);			// end of op

// need to rotate
	CMP_ri(REG_WORK1, 32);
	CC_MOV_rrLSLr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);
	
	SUB_rri(REG_WORK3, REG_WORK1, 1);	
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);

	RSB_rri(REG_WORK3, REG_WORK1, 33);
	ORR_rrrLSRr(d, REG_WORK2, d, REG_WORK3);
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXL_l,(RW4 d, RR4 i))

MIDFUNC(2,jff_ROXL_b,(RW1 d, RR4 i))
{
	INIT_REGS_b(d, i);
  int x = rmw(FLAGX);

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 35);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 36);
	CMP_ri(REG_WORK1, 17);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 18);
	CMP_ri(REG_WORK1, 8);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 9);
	TST_rr(REG_WORK1, REG_WORK1);
	BNE_i(3);			// need to rotate

	MSR_CPSRf_i(0);
	AND_rri(REG_WORK1, d, 0xff); // make sure to clear carry
	MOVS_rrLSLi(REG_WORK1, REG_WORK1, 24);
#ifdef ARMV6T2
	B_i(13);			// end of op
#else
	B_i(16);			// end of op
#endif

// need to rotate
	CMP_ri(REG_WORK1, 8);
	CC_MOV_rrLSLr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);
	
	SUB_rri(REG_WORK3, REG_WORK1, 1);	
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);

	MSR_CPSRf_i(0);
	RSB_rri(REG_WORK3, REG_WORK1, 9);
	UNSIGNED8_REG_2_REG(REG_WORK1, d);
	ORRS_rrrLSRr(REG_WORK2, REG_WORK2, REG_WORK1, REG_WORK3); // this LSR places correct bit in carry
	
  // Duplicate carry
	MOV_ri(x, 1);
  CC_MOV_ri(NATIVE_CC_CC, x, 0);

	// Calc N and Z
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, x, 8, 8); // Make sure to set carry (last bit shifted out)
#else
	BIC_rri(REG_WORK2, REG_WORK2, 0x100);
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 8);
#endif
	LSLS_rri(REG_WORK1, REG_WORK2, 24);

#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXL_b,(RW1 d, RR4 i))

MIDFUNC(2,jff_ROXL_w,(RW2 d, RR4 i))
{
	INIT_REGS_w(d, i);
  int x = rmw(FLAGX);

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 33);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 34);
	CMP_ri(REG_WORK1, 16);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 17);
	TST_rr(REG_WORK1, REG_WORK1);
	BNE_i(3);			// need to rotate

	MSR_CPSRf_i(0);
	BIC_rri(REG_WORK1, d, 0x00ff0000); // make sure to clear carry
	MOVS_rrLSLi(REG_WORK1, REG_WORK1, 16);
#ifdef ARMV6T2
	B_i(13);			// end of op
#else
	B_i(14);			// end of op
#endif

// need to rotate
	CMP_ri(REG_WORK1, 16);
	CC_MOV_rrLSLr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);
	
	SUB_rri(REG_WORK3, REG_WORK1, 1);	
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);

	MSR_CPSRf_i(0);
	RSB_rri(REG_WORK3, REG_WORK1, 17);
	UNSIGNED16_REG_2_REG(REG_WORK1, d);
	ORRS_rrrLSRr(REG_WORK2, REG_WORK2, REG_WORK1, REG_WORK3); // this LSR places correct bit in carry
	
  // Duplicate carry
	MOV_ri(x, 1);
  CC_MOV_ri(NATIVE_CC_CC, x, 0);

	// Calc N and Z
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, x, 16, 16); // Make sure to set carry (last bit shifted out)
#else
	BIC_rri(REG_WORK2, REG_WORK2, 0x10000);
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 16);
#endif
	LSLS_rri(REG_WORK1, REG_WORK2, 16);

  PKHTB_rrr(d, d, REG_WORK2);
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXL_w,(RW2 d, RR4 i))

MIDFUNC(2,jff_ROXL_l,(RW4 d, RR4 i))
{
	INIT_REGS_l(d, i);
  int x = rmw(FLAGX);

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 32);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 33);
	TST_rr(REG_WORK1, REG_WORK1);
	BNE_i(2);			// need to rotate

	MSR_CPSRf_i(0);
	TST_rr(d, d);
	B_i(9);			// end of op

// need to rotate
	CMP_ri(REG_WORK1, 32);
	CC_MOV_rrLSLr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);
	
	SUB_rri(REG_WORK3, REG_WORK1, 1);	
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);

	MSR_CPSRf_i(0);
	RSB_rri(REG_WORK3, REG_WORK1, 33);
	ORRS_rrrLSRr(d, REG_WORK2, d, REG_WORK3); // this LSR places correct bit in carry
	
  // Duplicate carry
	MOV_ri(x, 1);
  CC_MOV_ri(NATIVE_CC_CC, x, 0);

// end of op

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
MIDFUNC(2,jnf_ROR_b_imm,(RW1 d, IMM i))
{
	if(i & 0x07) {
		INIT_REG_b(d);

  	LSL_rri(REG_WORK1, d, 24);
  	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
  	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
  	ROR_rri(REG_WORK1, REG_WORK1, i & 0x07);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  	AND_rri(REG_WORK1, REG_WORK1, 0xff);
  	BIC_rri(d, d, 0xff);
  	ORR_rrr(d, d, REG_WORK1);
#endif
  
		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROR_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jnf_ROR_w_imm,(RW2 d, IMM i))
{
	if(i & 0x0f) {
		INIT_REG_w(d);

  	PKHBT_rrrLSLi(REG_WORK1, d, d, 16);
  	ROR_rri(REG_WORK1, REG_WORK1, i & 0x0f);
	  PKHTB_rrr(d, d, REG_WORK1);

		unlock2(d);
  }
}
MENDFUNC(2,jnf_ROR_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jnf_ROR_l_imm,(RW4 d, IMM i))
{
	if(i & 0x1f) {
		d = rmw(d);
	
		ROR_rri(d, d, i & 31);
	
		unlock2(d);
	}
}
MENDFUNC(2,jnf_ROR_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jff_ROR_b_imm,(RW1 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	LSL_rri(REG_WORK1, d, 24);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	MSR_CPSRf_i(0);
	if(i & 0x07) {
  	RORS_rri(REG_WORK1, REG_WORK1, i & 0x07);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
  	AND_rri(REG_WORK1, REG_WORK1, 0xff);
  	BIC_rri(d, d, 0xff);
  	ORR_rrr(d, d, REG_WORK1);
#endif
  } else if (i > 0x07) {
    TST_rr(REG_WORK1, REG_WORK1);
    // We need to copy MSB to carry
  	MRS_CPSR(REG_WORK1); // carry is cleared
	  CC_ORR_rri(NATIVE_CC_MI, REG_WORK1, REG_WORK1, ARM_C_FLAG);
    MSR_CPSRf_r(REG_WORK1);
  } else {
    TST_rr(REG_WORK1, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jff_ROR_b_imm,(RW1 d, IMM i))

MIDFUNC(2,jff_ROR_w_imm,(RW2 d, IMM i))
{
	if(i)
		d = rmw(d);
	else
		d = readreg(d);

	PKHBT_rrrLSLi(REG_WORK1, d, d, 16);
	MSR_CPSRf_i(0);
	if(i & 0x0f) {
  	RORS_rri(REG_WORK1, REG_WORK1, i & 0x0f);
	  PKHTB_rrr(d, d, REG_WORK1);
  } else if (i > 0x0f) {
    TST_rr(REG_WORK1, REG_WORK1);
    // We need to copy MSB to carry
  	MRS_CPSR(REG_WORK1); // carry is cleared
	  CC_ORR_rri(NATIVE_CC_MI, REG_WORK1, REG_WORK1, ARM_C_FLAG);
    MSR_CPSRf_r(REG_WORK1);
  } else {
    TST_rr(REG_WORK1, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,jff_ROR_w_imm,(RW2 d, IMM i))

MIDFUNC(2,jff_ROR_l_imm,(RW4 d, IMM i))
{
	if(i)
	  d = rmw(d);
	else
		d = readreg(d);

	MSR_CPSRf_i(0);
	if(i & 0x1f) {
	  RORS_rri(d, d, i & 0x1f);
  } else if (i > 0x1f) {
    TST_ri(d, d);
    // We need to copy MSB to carry
  	MRS_CPSR(REG_WORK1); // carry is cleared
	  CC_ORR_rri(NATIVE_CC_MI, REG_WORK1, REG_WORK1, ARM_C_FLAG);
    MSR_CPSRf_r(REG_WORK1);
  } else {
    TST_ri(d, d);
  }

	unlock2(d);
}
MENDFUNC(2,jff_ROR_l_imm,(RW4 d, IMM i))

MIDFUNC(2,jnf_ROR_b,(RW1 d, RR4 i))
{
	if (isconst(i)) {
		COMPCALL(jnf_ROR_b_imm)(d, (uae_u8)live.state[i].val);
		return;
	}
	INIT_REGS_b(d, i);

	LSL_rri(REG_WORK1, d, 24);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	ROR_rrr(REG_WORK1, REG_WORK1, i);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	AND_rri(REG_WORK1, REG_WORK1, 0xff);
	BIC_rri(d, d, 0xff);
	ORR_rrr(d, d, REG_WORK1);
#endif

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

	PKHBT_rrrLSLi(REG_WORK1, d, d, 16);
	ROR_rrr(REG_WORK1, REG_WORK1, i);
  PKHTB_rrr(d, d, REG_WORK1);

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

	ROR_rrr(d, d, i);

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

	LSL_rri(REG_WORK1, d, 24);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 8);
	ORR_rrrLSRi(REG_WORK1, REG_WORK1, REG_WORK1, 16);
	MSR_CPSRf_i(0);
	AND_rri(REG_WORK2, i, 63);
	RORS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	AND_rri(REG_WORK1, REG_WORK1, 0xff);
	BIC_rri(d, d, 0xff);
	ORR_rrr(d, d, REG_WORK1);
#endif

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

	PKHBT_rrrLSLi(REG_WORK1, d, d, 16);
	MSR_CPSRf_i(0);
	AND_rri(REG_WORK2, i, 63);
	RORS_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
  PKHTB_rrr(d, d, REG_WORK1);

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

	MSR_CPSRf_i(0);
	AND_rri(REG_WORK1, i, 63);
	RORS_rrr(d, d, REG_WORK1);

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

	PKHBT_rrrLSLi(d, d, d, 16);
	ROR_rri(d, d, 1);

	unlock2(d);
}
MENDFUNC(1,jnf_RORW,(RW2 d))

MIDFUNC(1,jff_RORW,(RW2 d))
{
	d = rmw(d);

	PKHBT_rrrLSLi(d, d, d, 16);
	MSR_CPSRf_i(0);
	RORS_rri(d, d, 1);

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

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 35);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 36);
	CMP_ri(REG_WORK1, 17);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 18);
	CMP_ri(REG_WORK1, 8);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 9);
	TST_rr(REG_WORK1, REG_WORK1);
#ifdef ARMV6T2
	BEQ_i(4);			// end of op
#else
	BEQ_i(6);			// end of op
#endif

// need to rotate
	AND_rri(REG_WORK2, d, 0xff);						 					// val = val & 0xff
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, REG_WORK2, 9);	// val = val | (val << 9)
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 8); 					// val = val | (x << 8)
	MOV_rrLSRr(REG_WORK2, REG_WORK2, REG_WORK1); 			// val = val >> cnt
	
#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXR_b,(RW1 d, RR4 i))

MIDFUNC(2,jnf_ROXR_w,(RW2 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_w(d, i);

	clobber_flags();

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 33);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 34);
	CMP_ri(REG_WORK1, 16);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 17);
	TST_rr(REG_WORK1, REG_WORK1);
	BEQ_i(5);			// end of op

// need to rotate
	BIC_rri(REG_WORK2, d, 0xff000000);
	BIC_rri(REG_WORK2, REG_WORK2, 0x00ff0000);				// val = val & 0xffff
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, REG_WORK2, 17);	// val = val | (val << 17)
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 16); 				// val = val | (x << 16)
	MOV_rrLSRr(REG_WORK2, REG_WORK2, REG_WORK1); 			// val = val >> cnt

  PKHTB_rrr(d, d, REG_WORK2);
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXR_w,(RW2 d, RR4 i))

MIDFUNC(2,jnf_ROXR_l,(RW4 d, RR4 i))
{
  int x = readreg(FLAGX);
	INIT_REGS_l(d, i);

	clobber_flags();
	
	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 32);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 33);
	TST_rr(REG_WORK1, REG_WORK1);
	BEQ_i(6);			// end of op

// need to rotate
	CMP_ri(REG_WORK1, 32);
	CC_MOV_rrLSRr(NATIVE_CC_NE, REG_WORK2, d, REG_WORK1);
	CC_MOV_ri(NATIVE_CC_EQ, REG_WORK2, 0);

	RSB_rri(REG_WORK3, REG_WORK1, 32);
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);
	
	ADD_rri(REG_WORK3, REG_WORK3, 1);	
	ORR_rrrLSLr(d, REG_WORK2, d, REG_WORK3);

// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jnf_ROXR_l,(RW4 d, RR4 i))

MIDFUNC(2,jff_ROXR_b,(RW1 d, RR4 i))
{
	INIT_REGS_b(d, i);
  int x = rmw(FLAGX);

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 35);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 36);
	CMP_ri(REG_WORK1, 17);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 18);
	CMP_ri(REG_WORK1, 8);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 9);
	TST_rr(REG_WORK1, REG_WORK1);
	BNE_i(3);			// need to rotate

	MSR_CPSRf_i(0);
	BIC_rri(REG_WORK1, d, 0x0000ff00); // make sure to clear carry
	MOVS_rrLSLi(REG_WORK1, REG_WORK1, 24);
#ifdef ARMV6T2
	B_i(9);			  // end of op
#else
	B_i(12);			// end of op
#endif

// need to rotate
	AND_rri(REG_WORK2, d, 0xff);						 					// val = val & 0xff
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, REG_WORK2, 9);	// val = val | (val << 9)
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 8); 					// val = val | (x << 8)
	MSR_CPSRf_i(0);
	MOVS_rrLSRr(REG_WORK2, REG_WORK2, REG_WORK1); 		// val = val >> cnt
	
  // Duplicate carry
	MOV_ri(x, 1);
  CC_MOV_ri(NATIVE_CC_CC, x, 0);

	// Calc N and Z
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, x, 8, 8); // Make sure to set carry (last bit shifted out)
#else
	BIC_rri(REG_WORK2, REG_WORK2, 0x100);
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 8);
#endif
	LSLS_rri(REG_WORK1, REG_WORK2, 24);

#ifdef ARMV6T2
  BFI_rrii(d, REG_WORK2, 0, 7);
#else
  AND_rri(REG_WORK2, REG_WORK2, 0xff);
  BIC_rri(d, d, 0xff);
  ORR_rrr(d, d, REG_WORK2);
#endif
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXR_b,(RW1 d, RR4 i))

MIDFUNC(2,jff_ROXR_w,(RW2 d, RR4 i))
{
	INIT_REGS_w(d, i);
  int x = rmw(FLAGX);

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 33);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 34);
	CMP_ri(REG_WORK1, 16);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 17);
	TST_rr(REG_WORK1, REG_WORK1);
	BNE_i(3);			// need to rotate

	MSR_CPSRf_i(0);
	BIC_rri(REG_WORK1, d, 0x00ff0000); // make sure to clear carry
	MOVS_rrLSLi(REG_WORK1, REG_WORK1, 16);
#ifdef ARMV6T2
	B_i(10);			// end of op
#else
	B_i(11);			// end of op
#endif

// need to rotate
	BIC_rri(REG_WORK2, d, 0xff000000);
	BIC_rri(REG_WORK2, REG_WORK2, 0x00ff0000);				// val = val & 0xffff
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, REG_WORK2, 17);	// val = val | (val << 17)
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 16); 				// val = val | (x << 16)
	MSR_CPSRf_i(0);
	MOVS_rrLSRr(REG_WORK2, REG_WORK2, REG_WORK1); 		// val = val >> cnt

  // Duplicate carry
	MOV_ri(x, 1);
  CC_MOV_ri(NATIVE_CC_CC, x, 0);

	// Calc N and Z
#ifdef ARMV6T2
	BFI_rrii(REG_WORK2, x, 16, 16); // Make sure to set carry (last bit shifted out)
#else
	BIC_rri(REG_WORK2, REG_WORK2, 0x10000);
	ORR_rrrLSLi(REG_WORK2, REG_WORK2, x, 16);
#endif
	LSLS_rri(REG_WORK1, REG_WORK2, 16);

  PKHTB_rrr(d, d, REG_WORK2);
	
// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXR_w,(RW2 d, RR4 i))

MIDFUNC(2,jff_ROXR_l,(RW4 d, RR4 i))
{
	INIT_REGS_l(d, i);
  int x = rmw(FLAGX);

	AND_rri(REG_WORK1, i, 63);
	CMP_ri(REG_WORK1, 32);
	CC_SUB_rri(NATIVE_CC_GT, REG_WORK1, REG_WORK1, 33);
	TST_rr(REG_WORK1, REG_WORK1);
	BNE_i(2);			// need to rotate

	MSR_CPSRf_i(0);
	TST_rr(d, d);
	B_i(13);			// end of op

// need to rotate
	CMP_ri(REG_WORK1, 32);
	BNE_i(3);			// rotate 1-31

// rotate 32
	MSR_CPSRf_i(0);
	LSLS_rri(d, d, 1);
	ORRS_rrr(d, d, x);
	B_i(5);				// duplicate carry

// rotate 1-31
	MSR_CPSRf_i(0);
	MOVS_rrLSRr(REG_WORK2, d, REG_WORK1);

	RSB_rri(REG_WORK3, REG_WORK1, 32);
	ORR_rrrLSLr(REG_WORK2, REG_WORK2, x, REG_WORK3);
	
	ADD_rri(REG_WORK3, REG_WORK3, 1);	
	ORR_rrrLSLr(d, REG_WORK2, d, REG_WORK3);
	
  // Duplicate carry
	MOV_ri(x, 1);
  CC_MOV_ri(NATIVE_CC_CC, x, 0);

// end of op

	unlock2(x);
	EXIT_REGS(d, i);
}
MENDFUNC(2,jff_ROXR_l,(RW4 d, RR4 i))

/*
 * SCC
 *
 */
MIDFUNC(2,jnf_SCC,(W1 d, IMM cc))
{
	INIT_WREG_b(d);
 
 	switch (cc) {
		case 9: // LS
			BIC_rri(d, d, 0xff);
			CC_ORR_rri(NATIVE_CC_CS, d, d, 0xff);
			CC_ORR_rri(NATIVE_CC_EQ, d, d, 0xff);
			break;

		case 8: // HI
      BIC_rri(d, d, 0xff);
      CC_ORR_rri(NATIVE_CC_CC, d, d, 0xff);
      CC_BIC_rri(NATIVE_CC_EQ, d, d, 0xff);
			break;

		default:
			ORR_rri(d, d, 0xff);
			CC_BIC_rri(cc^1, d, d, 0xff);
			break;
	}

  unlock2(d);
}
MENDFUNC(2,jnf_SCC,(W1 d, IMM cc))

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
MIDFUNC(2,jnf_SUB_b_imm,(RW1 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffffff00) | (((live.state[d].val & 0xff) - (v & 0xff)) & 0x000000ff));
		return;
	}

	INIT_REG_b(d);

	if(targetIsReg) {
	  SUB_rri(REG_WORK1, d, v & 0xff);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		SUB_rri(d, d, v & 0xff);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_SUB_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jnf_SUB_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUB_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	if(targetIsReg) {
		SUB_rrr(REG_WORK1, d, s);
#ifdef ARMV6T2
	  BFI_rrii(d, REG_WORK1, 0, 7);
#else
	  AND_rri(REG_WORK1, REG_WORK1, 0xff);
	  BIC_rri(d, d, 0xff);
	  ORR_rrr(d, d, REG_WORK1);
#endif
	} else {
		SUB_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUB_b,(RW1 d, RR1 s))

MIDFUNC(2,jnf_SUB_w_imm,(RW2 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, (live.state[d].val & 0xffff0000) | (((live.state[d].val & 0xffff) - (v & 0xffff)) & 0x0000ffff));
		return;
	}

	INIT_REG_w(d);

	UNSIGNED16_IMM_2_REG(REG_WORK1, (uae_u16)v);
	if(targetIsReg) {
		SUB_rrr(REG_WORK1, d, REG_WORK1);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else{
		SUB_rrr(d, d, REG_WORK1);
	}

	unlock2(d);
}
MENDFUNC(2,jnf_SUB_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jnf_SUB_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUB_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	if(targetIsReg) {
		SUB_rrr(REG_WORK1, d, s);
	  PKHTB_rrr(d, d, REG_WORK1);
	} else{
		SUB_rrr(d, d, s);
	}

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUB_w,(RW2 d, RR2 s))

MIDFUNC(2,jnf_SUB_l_imm,(RW4 d, IMM v))
{
	if (isconst(d)) {
		set_const(d, live.state[d].val - v);
		return;
	}

	d = rmw(d);

	compemu_raw_mov_l_ri(REG_WORK1, v);
	SUB_rrr(d, d, REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,jnf_SUB_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jnf_SUB_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jnf_SUB_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	SUB_rrr(d, d, s);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUB_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_SUB_b_imm,(RW1 d, IMM v))
{
	INIT_REG_b(d);

	LSL_rri(REG_WORK1, d, 24);
	SUBS_rri(REG_WORK1, REG_WORK1, (v << 24));
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	unlock2(d);
}
MENDFUNC(2,jff_SUB_b_imm,(RW1 d, IMM v))

MIDFUNC(2,jff_SUB_b,(RW1 d, RR1 s))
{
	if (isconst(s)) {
		COMPCALL(jff_SUB_b_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_b(d, s);

	LSL_rri(REG_WORK1, d, 24);
	SUBS_rrrLSLi(REG_WORK1, REG_WORK1, s, 24);
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUB_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_SUB_w_imm,(RW2 d, IMM v))
{
	INIT_REG_w(d);

  if(CHECK32(v)) {
    MOV_ri(REG_WORK1, v);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, v);
#else
    uae_s32 offs = data_word_offs(v);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
  }
	LSL_rri(REG_WORK2, d, 16);
	SUBS_rrrLSLi(REG_WORK1, REG_WORK2, REG_WORK1, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	unlock2(d);
}
MENDFUNC(2,jff_SUB_w_imm,(RW2 d, IMM v))

MIDFUNC(2,jff_SUB_w,(RW2 d, RR2 s))
{
	if (isconst(s)) {
		COMPCALL(jff_SUB_w_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_w(d, s);

	LSL_rri(REG_WORK1, d, 16);
  SUBS_rrrLSLi(REG_WORK1, REG_WORK1, s, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUB_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_SUB_l_imm,(RW4 d, IMM v))
{
	d = rmw(d);

	compemu_raw_mov_l_ri(REG_WORK2, v);
	SUBS_rrr(d, d, REG_WORK2);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

	unlock2(d);
}
MENDFUNC(2,jff_SUB_l_imm,(RW4 d, IMM v))

MIDFUNC(2,jff_SUB_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(jff_SUB_l_imm)(d, live.state[s].val);
		return;
	}

	INIT_REGS_l(d, s);

	SUBS_rrr(d, d, s);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);
  DUPLICACTE_CARRY_FROM_REG(REG_WORK1)

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
MIDFUNC(2,jnf_SUBA_w,(RW4 d, RR2 s))
{
	INIT_REGS_w(d, s);

	SIGNED16_REG_2_REG(REG_WORK1, s);
	SUB_rrr(d, d, REG_WORK1);

	EXIT_REGS(d, s);
}
MENDFUNC(2,jnf_SUBA_w,(RW4 d, RR2 s))

MIDFUNC(2,jnf_SUBA_l,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);

	SUB_rrr(d, d, s);

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
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	LSL_rri(REG_WORK1, d, 24);
	SBC_rrrLSLi(REG_WORK1, REG_WORK1, s, 24);
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}

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
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	LSL_rri(REG_WORK1, d, 16);
	SBC_rrrLSLi(REG_WORK1, REG_WORK1, s, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);

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
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	SBC_rrr(d, d, s);

	EXIT_REGS(d, s);
	unlock2(x);
}
MENDFUNC(2,jnf_SUBX_l,(RW4 d, RR4 s))

MIDFUNC(2,jff_SUBX_b,(RW1 d, RR1 s))
{
	INIT_REGS_b(d, s);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore inverted X to carry (don't care about other flags)
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	LSL_rri(REG_WORK1, d, 24);
	SBCS_rrrLSLi(REG_WORK1, REG_WORK1, s, 24);
	if(targetIsReg) {
	  BIC_rri(d, d, 0xff);
	  ORR_rrrLSRi(d, d, REG_WORK1, 24);
	} else {
		LSR_rri(d, REG_WORK1, 24);
	}

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUBX_b,(RW1 d, RR1 s))

MIDFUNC(2,jff_SUBX_w,(RW2 d, RR2 s))
{
	INIT_REGS_w(d, s);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore inverted X to carry (don't care about other flags)
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	LSL_rri(REG_WORK1, d, 16);
	SBCS_rrrLSLi(REG_WORK1, REG_WORK1, s, 16);
  PKHTB_rrrASRi(d, d, REG_WORK1, 16);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

	unlock2(x);
	EXIT_REGS(d, s);
}
MENDFUNC(2,jff_SUBX_w,(RW2 d, RR2 s))

MIDFUNC(2,jff_SUBX_l,(RW4 d, RR4 s))
{
	INIT_REGS_l(d, s);
  int x = rmw(FLAGX);

	MVN_ri(REG_WORK2, 0);
	CC_MVN_ri(NATIVE_CC_NE, REG_WORK2, ARM_Z_FLAG);

	// Restore inverted X to carry (don't care about other flags)
  MVN_rrLSLi(REG_WORK1, x, 29);
	MSR_CPSRf_r(REG_WORK1);

	SBCS_rrr(d, d, s);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	AND_rrr(REG_WORK1, REG_WORK1, REG_WORK2);
#ifdef ARMV6T2
	UBFX_rrii(x, REG_WORK1, 29, 1); // Duplicate carry
#else
	LSR_rri(x, REG_WORK1, 29);
	AND_rri(x, x, 1);
#endif
	MSR_CPSRf_r(REG_WORK1);

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
		set_const(d, (live.state[d].val >> 16) | (live.state[d].val << 16));
		return;
	}

	d = rmw(d);

	ROR_rri(d, d, 16);

	unlock2(d);
}
MENDFUNC(1,jnf_SWAP,(RW4 d))

MIDFUNC(1,jff_SWAP,(RW4 d))
{
	d = rmw(d);

	ROR_rri(d, d, 16);
	MSR_CPSRf_i(0);
	TST_rr(d, d);

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
MIDFUNC(1,jff_TST_b,(RR1 s))
{
	if (isconst(s)) {
		SIGNED8_IMM_2_REG(REG_WORK1, (uae_u8)live.state[s].val);
	} else {
		s = readreg(s);
		SIGNED8_REG_2_REG(REG_WORK1, s);
		unlock2(s);
	}
	MSR_CPSRf_i(0);
	TST_rr(REG_WORK1, REG_WORK1);
}
MENDFUNC(1,jff_TST_b,(RR1 s))

MIDFUNC(1,jff_TST_w,(RR2 s))
{
	if (isconst(s)) {
		SIGNED16_IMM_2_REG(REG_WORK1, (uae_u16)live.state[s].val);
	} else {
		s = readreg(s);
		SIGNED16_REG_2_REG(REG_WORK1, s);
		unlock2(s);
	}
	MSR_CPSRf_i(0);
	TST_rr(REG_WORK1, REG_WORK1);
}
MENDFUNC(1,jff_TST_w,(RR2 s))

MIDFUNC(1,jff_TST_l,(RR4 s))
{
	MSR_CPSRf_i(0);

	if (isconst(s)) {
		compemu_raw_mov_l_ri(REG_WORK1, live.state[s].val);
		TST_rr(REG_WORK1, REG_WORK1);
	}
	else {
		s = readreg(s);
		TST_rr(s, s);
		unlock2(s);
	}
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
  
  STRB_rRR(b, adr, R_MEMSTART);
  
  unlock2(b);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE_OFF_b,(RR4 adr, RR4 b))

MIDFUNC(2,jnf_MEM_WRITE_OFF_w,(RR4 adr, RR4 w))
{
  adr = readreg(adr);
	w = readreg(w);
  
  REV16_rr(REG_WORK1, w);
  STRH_rRR(REG_WORK1, adr, R_MEMSTART);
  
  unlock2(w);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE_OFF_w,(RR4 adr, RR4 w))

MIDFUNC(2,jnf_MEM_WRITE_OFF_l,(RR4 adr, RR4 l))
{
  adr = readreg(adr);
  l = readreg(l);
  
  REV_rr(REG_WORK1, l);
  STR_rRR(REG_WORK1, adr, R_MEMSTART);
  
  unlock2(l);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE_OFF_l,(RR4 adr, RR4 l))


MIDFUNC(2,jnf_MEM_READ_OFF_b,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  LDRB_rRR(d, adr, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ_OFF_b,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ_OFF_w,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  LDRH_rRR(REG_WORK1, adr, R_MEMSTART);
  REV16_rr(d, REG_WORK1);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ_OFF_w,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ_OFF_l,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  LDR_rRR(REG_WORK1, adr, R_MEMSTART);
  REV_rr(d, REG_WORK1);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ_OFF_l,(W4 d, RR4 adr))


MIDFUNC(2,jnf_MEM_WRITE24_OFF_b,(RR4 adr, RR4 b))
{
  adr = readreg(adr);
  b = readreg(b);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  STRB_rRR(b, REG_WORK1, R_MEMSTART);
  
  unlock2(b);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE24_OFF_b,(RR4 adr, RR4 b))

MIDFUNC(2,jnf_MEM_WRITE24_OFF_w,(RR4 adr, RR4 w))
{
  adr = readreg(adr);
  w = readreg(w);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  REV16_rr(REG_WORK3, w);
  STRH_rRR(REG_WORK3, REG_WORK1, R_MEMSTART);
  
  unlock2(w);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE24_OFF_w,(RR4 adr, RR4 w))

MIDFUNC(2,jnf_MEM_WRITE24_OFF_l,(RR4 adr, RR4 l))
{
  adr = readreg(adr);
  l = readreg(l);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  REV_rr(REG_WORK3, l);
  STR_rRR(REG_WORK3, REG_WORK1, R_MEMSTART);
  
  unlock2(l);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_WRITE24_OFF_l,(RR4 adr, RR4 l))


MIDFUNC(2,jnf_MEM_READ24_OFF_b,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  LDRB_rRR(d, REG_WORK1, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ24_OFF_b,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ24_OFF_w,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  LDRH_rRR(REG_WORK1, REG_WORK1, R_MEMSTART);
  REV16_rr(d, REG_WORK1);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ24_OFF_w,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_READ24_OFF_l,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  LDR_rRR(d, REG_WORK1, R_MEMSTART);
  REV_rr(d, d);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_READ24_OFF_l,(W4 d, RR4 adr))


MIDFUNC(2,jnf_MEM_GETADR_OFF,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  ADD_rrr(d, adr, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_GETADR_OFF,(W4 d, RR4 adr))

MIDFUNC(2,jnf_MEM_GETADR24_OFF,(W4 d, RR4 adr))
{
  adr = readreg(adr);
  d = writereg(d);
  
  BIC_rri(REG_WORK1, adr, 0xff000000);
  ADD_rrr(d, REG_WORK1, R_MEMSTART);
  
  unlock2(d);
  unlock2(adr);
}
MENDFUNC(2,jnf_MEM_GETADR24_OFF,(W4 d, RR4 adr))


MIDFUNC(3,jnf_MEM_READMEMBANK,(W4 dest, RR4 adr, IMM offset))
{
  clobber_flags();
  if (dest != adr) {
    COMPCALL(forget_about)(dest);
  }

  adr = readreg_specific(adr, REG_PAR1);
  prepare_for_call_1();
  unlock2(adr);
  prepare_for_call_2();
	
#ifdef ARMV6T2
  MOVW_ri16(REG_WORK2, (uae_u32)mem_banks);
  MOVT_ri16(REG_WORK2, (uae_u32)mem_banks >> 16);
#else
  uae_s32 offs = data_long_offs((uae_u32)mem_banks);
  LDR_rRI(REG_WORK2, RPC_INDEX, offs);
#endif
  LSR_rri(REG_WORK1, adr, 16);
  LDR_rRR_LSLi(REG_WORK3, REG_WORK2, REG_WORK1, 2);
  LDR_rRI(REG_WORK3, REG_WORK3, offset);
   
  compemu_raw_call_r(REG_WORK3);

  live.nat[REG_RESULT].holds[0] = dest;
  live.nat[REG_RESULT].nholds = 1;
  live.nat[REG_RESULT].touched = touchcnt++;

  live.state[dest].realreg = REG_RESULT;
  live.state[dest].realind = 0;
  live.state[dest].val = 0;
  live.state[dest].validsize = 4;
  set_status(dest, DIRTY);
}
MENDFUNC(3,jnf_MEM_READMEMBANK,(W4 dest, RR4 adr, IMM offset))


MIDFUNC(3,jnf_MEM_WRITEMEMBANK,(RR4 adr, RR4 source, IMM offset))
{
  clobber_flags();

  adr = readreg_specific(adr, REG_PAR1);
  source = readreg_specific(source, REG_PAR2);
  prepare_for_call_1();
  unlock2(adr);
  unlock2(source);
  prepare_for_call_2();
	
#ifdef ARMV6T2
  MOVW_ri16(REG_WORK2, (uae_u32)mem_banks);
  MOVT_ri16(REG_WORK2, (uae_u32)mem_banks >> 16);
#else
  uae_s32 offs = data_long_offs((uae_u32)mem_banks);
  LDR_rRI(REG_WORK2, RPC_INDEX, offs);
#endif
  LSR_rri(REG_WORK1, adr, 16);
  LDR_rRR_LSLi(REG_WORK3, REG_WORK2, REG_WORK1, 2);
  LDR_rRI(REG_WORK3, REG_WORK3, offset);
    
  compemu_raw_call_r(REG_WORK3);
}
MENDFUNC(3,jnf_MEM_WRITEMEMBANK,(RR4 adr, RR4 source, IMM offset))

