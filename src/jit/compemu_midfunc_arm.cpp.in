/*
 * compiler/compemu_midfunc_arm.cpp - Native MIDFUNCS for ARM
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

MIDFUNC(2,mov_l_mi,(IMM d, IMM s))
{
#ifdef ARMV6T2
  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    MOVW_ri16(REG_WORK2, s);
    if(s >> 16)
      MOVT_ri16(REG_WORK2, s >> 16);
    uae_s32 idx = d - (uae_u32) &regs;
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
MENDFUNC(2,mov_l_mi,(IMM d, IMM s))

MIDFUNC(4,disp_ea20_target_add,(RW4 target, RR4 reg, IMM shift, IMM extend))
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
		ADD_rrrLSLi(target, target, REG_WORK1, shift & 0x1f);
	} else {
		ADD_rrrLSLi(target, target, reg, shift & 0x1f);
	}
	
	unlock2(target);
	unlock2(reg);
}
MENDFUNC(4,disp_ea20_target_add,(RW4 target, RR4 reg, IMM shift, IMM extend))

MIDFUNC(4,disp_ea20_target_mov,(W4 target, RR4 reg, IMM shift, IMM extend))
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
		LSL_rri(target, REG_WORK1, shift & 0x1f);
	} else {
		LSL_rri(target, reg, shift & 0x1f);
	}
	
	unlock2(target);
	unlock2(reg);
}
MENDFUNC(4,disp_ea20_target_mov,(W4 target, RR4 reg, IMM shift, IMM extend))

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

MIDFUNC(3,lea_l_brr,(W4 d, RR4 s, IMM offset))
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

	EXIT_REGS(d,s);
}
MENDFUNC(3,lea_l_brr,(W4 d, RR4 s, IMM offset))

MIDFUNC(5,lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IMM factor, IMM offset))
{
	if (!offset) {
		COMPCALL(lea_l_rr_indexed)(d, s, index, factor);
		return;
	}
	
	s = readreg(s);
	index = readreg(index);
	d = writereg(d);

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

	unlock2(d);
	unlock2(index);
	unlock2(s);
}
MENDFUNC(5,lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IMM factor, IMM offset))

MIDFUNC(4,lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))
{
	s = readreg(s);
	index = readreg(index);
	d = writereg(d);

	int shft;
	switch(factor) {
  	case 1: shft=0; break;
  	case 2: shft=1; break;
  	case 4: shft=2; break;
  	case 8: shft=3; break;
  	default: abort();
	}

	ADD_rrrLSLi(d, s, index, shft);

	unlock2(d);
	unlock2(index);
	unlock2(s);
}
MENDFUNC(4,lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))

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
	live.state[d].val = live.state[olds].val;
	live.state[d].validsize = 4;
	set_status(d, DIRTY);

	live.nat[s].holds[live.nat[s].nholds] = d;
	live.nat[s].nholds++;
	unlock2(s);
}
MENDFUNC(2,mov_l_rr,(W4 d, RR4 s))

MIDFUNC(2,mov_l_mr,(IMM d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(mov_l_mi)(d, live.state[s].val);
		return;
	}
	
	s = readreg(s);

  if(d >= (uae_u32) &regs && d < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = d - (uae_u32) &regs;
    STR_rRI(s, R_REGSTRUCT, idx);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, d);
    if(d >> 16)
	    MOVT_ri16(REG_WORK1, d >> 16);
#else
    uae_s32 offs = data_long_offs(d);
  	LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
  	STR_rR(s, REG_WORK1);
  }

	unlock2(s);
}
MENDFUNC(2,mov_l_mr,(IMM d, RR4 s))

MIDFUNC(2,mov_l_rm,(W4 d, IMM s))
{
	d = writereg(d);

  if(s >= (uae_u32) &regs && s < ((uae_u32) &regs) + sizeof(struct regstruct)) {
    uae_s32 idx = s - (uae_u32) & regs;
    LDR_rRI(d, R_REGSTRUCT, idx);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, s);
    if(s >> 16)
	    MOVT_ri16(REG_WORK1, s >> 16);
#else
    uae_s32 offs = data_long_offs(s);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	  LDR_rR(d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,mov_l_rm,(W4 d, IMM s))

MIDFUNC(2,mov_l_ri,(W4 d, IMM s))
{
	set_const(d, s);
}
MENDFUNC(2,mov_l_ri,(W4 d, IMM s))

MIDFUNC(2,mov_b_ri,(W1 d, IMM s))
{
  if(d < 16) {
	  if (isconst(d)) {
		  set_const(d, (live.state[d].val & 0xffffff00) | (s & 0x000000ff));
		  return;
	  }
	  d = rmw(d);

		BIC_rri(d, d, 0xff);
		ORR_rri(d, d, (s & 0xff));
	
	  unlock2(d);
  } else {
    set_const(d, s & 0xff);
  }
}
MENDFUNC(2,mov_b_ri,(W1 d, IMM s))

MIDFUNC(2,sub_l_ri,(RW4 d, IMM i))
{
	if (!i)
	  return;
	if (isconst(d)) {
		live.state[d].val -= i;
		return;
	}

	d = rmw(d);

  if(CHECK32(i)) {
    SUB_rri(d, d, i);
  } else {
#ifdef ARMV6T2
    MOVW_ri16(REG_WORK1, i);
    if(i >> 16)
      MOVT_ri16(REG_WORK1, i >> 16);
#else
    uae_s32 offs = data_long_offs(i);
	  LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
	  SUB_rrr(d, d, REG_WORK1);
  }

	unlock2(d);
}
MENDFUNC(2,sub_l_ri,(RW4 d, IMM i))

MIDFUNC(2,sub_w_ri,(RW2 d, IMM i))
{
  // This function is only called with i = 1
	// Caller needs flags...
	clobber_flags();

	d = rmw(d);

	LSL_rri(REG_WORK2, d, 16);

	SUBS_rri(REG_WORK2, REG_WORK2, (i & 0xff) << 16);
  PKHTB_rrrASRi(d, d, REG_WORK2, 16);

	MRS_CPSR(REG_WORK1);
	EOR_rri(REG_WORK1, REG_WORK1, ARM_C_FLAG);
	MSR_CPSRf_r(REG_WORK1);

	unlock2(d);
}
MENDFUNC(2,sub_w_ri,(RW2 d, IMM i))


/* forget_about() takes a mid-layer register */
MIDFUNC(1,forget_about,(W4 r))
{
	if (isinreg(r))
	  disassociate(r);
	live.state[r].val = 0;
	set_status(r, UNDEF);
}
MENDFUNC(1,forget_about,(W4 r))


// ARM optimized functions

MIDFUNC(2,arm_ADD_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(arm_ADD_l_ri)(d,live.state[s].val);
		return;
	}

	s = readreg(s);
	d = rmw(d);
	ADD_rrr(d, d, s);
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,arm_ADD_l,(RW4 d, RR4 s))

MIDFUNC(2,arm_ADD_l_ri,(RW4 d, IMM i))
{
	if (!i) 
	  return;
	if (isconst(d)) {
		live.state[d].val += i;
		return;
	}

	d = rmw(d);

	if(CHECK32(i)) {
		ADD_rri(d, d, i);
	} else {
#ifdef ARMV6T2
	  MOVW_ri16(REG_WORK1, i);
	  if(i >> 16)
	    MOVT_ri16(REG_WORK1, i >> 16);
#else
	  uae_s32 offs = data_long_offs(i);
		LDR_rRI(REG_WORK1, RPC_INDEX, offs);
#endif
		ADD_rrr(d, d, REG_WORK1);
  }
	
	unlock2(d);
}
MENDFUNC(2,arm_ADD_l_ri,(RW4 d, IMM i))

MIDFUNC(2,arm_ADD_l_ri8,(RW4 d, IMM i))
{
	if (!i) 
	  return;
	if (isconst(d)) {
		live.state[d].val += i;
		return;
	}

	d = rmw(d);
	ADD_rri(d, d, i);
	unlock2(d);
}
MENDFUNC(2,arm_ADD_l_ri8,(RW4 d, IMM i))

MIDFUNC(2,arm_SUB_l_ri8,(RW4 d, IMM i))
{
	if (!i) 
	  return;
	if (isconst(d)) {
		live.state[d].val -= i;
		return;
	}

	d = rmw(d);
	SUB_rri(d, d, i);
	unlock2(d);
}
MENDFUNC(2,arm_SUB_l_ri8,(RW4 d, IMM i))


// Other
STATIC_INLINE void flush_cpu_icache(void *start, void *stop)
{
	register void *_beg __asm ("a1") = start;
	register void *_end __asm ("a2") = stop;
	register void *_flg __asm ("a3") = 0;
	#ifdef __ARM_EABI__
	   register unsigned long _scno __asm ("r7") = 0xf0002;
	   __asm __volatile ("swi 0x0		@ sys_cacheflush"
                            : "=r" (_beg)
                            : "0" (_beg), "r" (_end), "r" (_flg), "r" (_scno));
	#else
	   __asm __volatile ("swi 0x9f0002		@ sys_cacheflush"
		   		    : "=r" (_beg)
		   		    : "0" (_beg), "r" (_end), "r" (_flg));
	#endif
}

STATIC_INLINE void write_jmp_target(uae_u32* jmpaddr, uintptr a) {
	uae_s32 off = ((uae_u32)a - (uae_u32)jmpaddr - 8) >> 2;
	*(jmpaddr) = (*(jmpaddr) & 0xff000000) | (off & 0x00ffffff);
  flush_cpu_icache((void *)jmpaddr, (void *)&jmpaddr[1]);
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

MIDFUNC(2,fmov_l_ri,(FW d, IMM i))
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
MENDFUNC(2,fmov_l_ri,(FW d, IMM i))

MIDFUNC(2,fmov_s_ri,(FW d, IMM i))
{
  d = f_writereg(d);
  compemu_raw_mov_l_ri(REG_WORK1, i);
  raw_fmov_s_rr(d, REG_WORK1);
	f_unlock(d);
}
MENDFUNC(2,fmov_s_ri,(FW d, IMM i))

MIDFUNC(2,fmov_to_l_rr,(W4 d, FR s))
{
	s = f_readreg(s);
  d = writereg(d);
  raw_fmov_to_l_rr(d, s);
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
	unlock2(d);
  f_unlock(s);
}
MENDFUNC(2,fmov_to_w_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_b_rr,(W4 d, FR s))
{
	s = f_readreg(s);
	INIT_WREG_b(d);

  raw_fmov_to_b_rr(d, s, targetIsReg);
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

MIDFUNC(2,fp_fscc_ri,(RW4 d, int cc))
{
	d = rmw(d);
	raw_fp_fscc_ri(d, cc);
	unlock2(d);
}
MENDFUNC(2,fp_fscc_ri,(RW4 d, int cc))

MIDFUNC(1,roundingmode,(IMM mode))
{
	raw_roundingmode(mode);
}
MENDFUNC(1,roundingmode,(IMM mode))


#endif // USE_JIT_FPU
