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
	raw_mov_l_mi(d, s);
}
MENDFUNC(2,mov_l_mi,(IMM d, IMM s))

MIDFUNC(2,shll_l_ri,(RW4 r, IMM i))
{
  // Only used in calc_disp_ea_020() -> flags not relevant and never modified
	if (!i)
	  return;
	if (isconst(r)) {
		live.state[r].val<<=i;
		return;
	}

	r = rmw(r, 4, 4);
	raw_shll_l_ri(r, i);
	unlock2(r);
}
MENDFUNC(2,shll_l_ri,(RW4 r, IMM i))

MIDFUNC(2,sign_extend_16_rr,(W4 d, RR2 s))
{
  // Only used in calc_disp_ea_020() -> flags not relevant and never modified
	int isrmw;

	if (isconst(s)) {
		set_const(d, (uae_s32)(uae_s16)live.state[s].val);
		return;
	}

	isrmw = (s == d);
	if (!isrmw) {
		s = readreg(s, 2);
		d = writereg(d, 4);
	}
	else { /* If we try to lock this twice, with different sizes, we are int trouble! */
		s = d = rmw(s, 4, 2);
	}
	SIGNED16_REG_2_REG(d, s);
	if (!isrmw) {
		unlock2(d);
		unlock2(s);
	}
	else {
		unlock2(s);
	}
}
MENDFUNC(2,sign_extend_16_rr,(W4 d, RR2 s))

MIDFUNC(2,mov_b_rr,(W1 d, RR1 s))
{
	if (d == s)
	  return;
	if (isconst(s)) {
		COMPCALL(mov_b_ri)(d, (uae_u8)live.state[s].val);
		return;
	}

	s = readreg(s, 1);
	d = writereg(d, 1);
	raw_mov_b_rr(d, s);
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,mov_b_rr,(W1 d, RR1 s))

MIDFUNC(2,mov_w_rr,(W2 d, RR2 s))
{
	if (d == s)
	  return;
	if (isconst(s)) {
		COMPCALL(mov_w_ri)(d, (uae_u16)live.state[s].val);
		return;
	}

	s = readreg(s, 2);
	d = writereg(d, 2);
	raw_mov_w_rr(d, s);
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,mov_w_rr,(W2 d, RR2 s))

MIDFUNC(3,lea_l_brr,(W4 d, RR4 s, IMM offset))
{
	if (isconst(s)) {
		COMPCALL(mov_l_ri)(d, live.state[s].val+offset);
		return;
	}

	s = readreg(s, 4);
	d = writereg(d, 4);
	raw_lea_l_brr(d, s, offset);
	unlock2(d);
	unlock2(s);
}
MENDFUNC(3,lea_l_brr,(W4 d, RR4 s, IMM offset))

MIDFUNC(5,lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IMM factor, IMM offset))
{
	if (!offset) {
		COMPCALL(lea_l_rr_indexed)(d, s, index, factor);
		return;
	}
	
	s = readreg(s, 4);
	index = readreg(index, 4);
	d = writereg(d, 4);
	raw_lea_l_brr_indexed(d, s, index, factor, offset);
	unlock2(d);
	unlock2(index);
	unlock2(s);
}
MENDFUNC(5,lea_l_brr_indexed,(W4 d, RR4 s, RR4 index, IMM factor, IMM offset))

MIDFUNC(4,lea_l_rr_indexed,(W4 d, RR4 s, RR4 index, IMM factor))
{
	s = readreg(s, 4);
	index = readreg(index, 4);
	d = writereg(d, 4);
	raw_lea_l_rr_indexed(d, s, index, factor);
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
	s = readreg(s, 4);
	live.state[d].realreg = s;
	live.state[d].realind = live.nat[s].nholds;
	live.state[d].val = live.state[olds].val;
	live.state[d].validsize = 4;
	live.state[d].dirtysize = 4;
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
	
	s = readreg(s, 4);
	raw_mov_l_mr(d, s);
	unlock2(s);
}
MENDFUNC(2,mov_l_mr,(IMM d, RR4 s))

MIDFUNC(2,mov_l_rm,(W4 d, IMM s))
{
	d = writereg(d, 4);
	raw_mov_l_rm(d, s);
	unlock2(d);
}
MENDFUNC(2,mov_l_rm,(W4 d, IMM s))

MIDFUNC(2,mov_l_ri,(W4 d, IMM s))
{
	set_const(d, s);
	return;
}
MENDFUNC(2,mov_l_ri,(W4 d, IMM s))

MIDFUNC(2,mov_w_ri,(W2 d, IMM s))
{
	d = writereg(d, 2);
	raw_mov_w_ri(d, s);
	unlock2(d);
}
MENDFUNC(2,mov_w_ri,(W2 d, IMM s))

MIDFUNC(2,mov_b_ri,(W1 d, IMM s))
{
	d = writereg(d, 1);
	raw_mov_b_ri(d, s);
	unlock2(d);
}
MENDFUNC(2,mov_b_ri,(W1 d, IMM s))

MIDFUNC(2,add_l,(RW4 d, RR4 s))
{
	if (isconst(s)) {
		COMPCALL(add_l_ri)(d, live.state[s].val);
		return;
	}

	s = readreg(s, 4);
	d = rmw(d, 4, 4);
	raw_add_l(d, s);
	unlock2(d);
	unlock2(s);
}
MENDFUNC(2,add_l,(RW4 d, RR4 s))

MIDFUNC(2,sub_l_ri,(RW4 d, IMM i))
{
	if (!i)
	  return;
	if (isconst(d)) {
		live.state[d].val -= i;
		return;
	}

	d = rmw(d, 4, 4);
	raw_sub_l_ri(d, i);
	unlock2(d);
}
MENDFUNC(2,sub_l_ri,(RW4 d, IMM i))

MIDFUNC(2,sub_w_ri,(RW2 d, IMM i))
{
	// Caller needs flags...
	clobber_flags();

	d = rmw(d, 2, 2);
	raw_sub_w_ri(d, i);
	unlock2(d);
}
MENDFUNC(2,sub_w_ri,(RW2 d, IMM i))

MIDFUNC(2,add_l_ri,(RW4 d, IMM i))
{
	if (!i)
	  return;
	if (isconst(d)) {
		live.state[d].val += i;
		return;
	}

	d = rmw(d, 4, 4);
	raw_add_l_ri(d, i);
	unlock2(d);
}
MENDFUNC(2,add_l_ri,(RW4 d, IMM i))

MIDFUNC(5,call_r_02,(RR4 r, RR4 in1, RR4 in2, IMM isize1, IMM isize2))
{
  clobber_flags();
  in1 = readreg_specific(in1, isize1, REG_PAR1);
  in2 = readreg_specific(in2, isize2, REG_PAR2);
  r = readreg(r, 4);
  prepare_for_call_1();
  unlock2(r);
  unlock2(in1);
  unlock2(in2);
  prepare_for_call_2();
  compemu_raw_call_r(r);
}
MENDFUNC(5,call_r_02,(RR4 r, RR4 in1, RR4 in2, IMM isize1, IMM isize2))

MIDFUNC(5,call_r_11,(W4 out1, RR4 r, RR4 in1, IMM osize, IMM isize))
{
  clobber_flags();
  if (osize == 4) {
    if (out1 != in1 && out1 != r) {
      COMPCALL(forget_about)(out1);
    }
  }
  else {
    tomem_c(out1);
  }

  in1 = readreg_specific(in1, isize, REG_PAR1);
  r = readreg(r, 4);
  prepare_for_call_1();

  unlock2(in1);
  unlock2(r);

  prepare_for_call_2();
  compemu_raw_call_r(r);

  live.nat[REG_RESULT].holds[0] = out1;
  live.nat[REG_RESULT].nholds = 1;
  live.nat[REG_RESULT].touched = touchcnt++;

  live.state[out1].realreg = REG_RESULT;
  live.state[out1].realind = 0;
  live.state[out1].val = 0;
  live.state[out1].validsize = osize;
  live.state[out1].dirtysize = osize;
  set_status(out1, DIRTY);
}
MENDFUNC(5,call_r_11,(W4 out1, RR4 r, RR4 in1, IMM osize, IMM isize))

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

	s = readreg(s, 4);
	d = rmw(d, 4, 4);
	raw_ADD_l_rr(d, s);
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

	d = rmw(d, 4, 4);

	if(CHECK32(i)) {
    raw_ADD_l_rri(d, d, i);
	} else {
  	raw_LDR_l_ri(REG_WORK1, i);
  	raw_ADD_l_rr(d, REG_WORK1);
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

	d = rmw(d, 4, 4);
	raw_ADD_l_rri(d, d, i);
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

	d = rmw(d, 4, 4);
	raw_SUB_l_rri(d, d, i);
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

STATIC_INLINE void write_jmp_target(uae_u32* jmpaddr, cpuop_func* a) {
	*(jmpaddr) = (uae_u32)a;
    flush_cpu_icache((void *)jmpaddr, (void *)&jmpaddr[1]);
}

STATIC_INLINE void emit_jmp_target(uae_u32 a) {
	emit_long((uae_u32)a);
}


/*************************************************************************
* FPU stuff                                                             *
*************************************************************************/

MIDFUNC(1,f_forget_about,(FW r))
{
	if (f_isinreg(r))
		f_disassociate(r);
	live.fate[r].status=UNDEF;
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
	s = readreg(s, 4);
  d = f_writereg(d);
  raw_fmov_l_rr(d, s);
	f_unlock(d);
  unlock2(s);
}
MENDFUNC(2,fmov_l_rr,(FW d, RR4 s))

MIDFUNC(2,fmov_s_rr,(FW d, RR4 s))
{
	s = readreg(s, 4);
  d = f_writereg(d);
  raw_fmov_s_rr(d, s);
	f_unlock(d);
  unlock2(s);
}
MENDFUNC(2,fmov_s_rr,(FW d, RR4 s))

MIDFUNC(2,fmov_w_rr,(FW d, RR2 s))
{
	s = readreg(s, 2);
  d = f_writereg(d);
  raw_fmov_w_rr(d, s);
	f_unlock(d);
  unlock2(s);
}
MENDFUNC(2,fmov_w_rr,(FW d, RR2 s))

MIDFUNC(2,fmov_b_rr,(FW d, RR1 s))
{
	s = readreg(s, 1);
  d = f_writereg(d);
  raw_fmov_b_rr(d, s);
	f_unlock(d);
  unlock2(s);
}
MENDFUNC(2,fmov_b_rr,(FW d, RR1 s))

MIDFUNC(3,fmov_d_rrr,(FW d, RR4 s1, RR4 s2))
{
	s1 = readreg(s1, 4);
	s2 = readreg(s2, 4);
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
  d = writereg(d, 4);
  raw_fmov_to_l_rr(d, s);
	unlock2(d);
  f_unlock(s);
}
MENDFUNC(2,fmov_to_l_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_s_rr,(W4 d, FR s))
{
	s = f_readreg(s);
  d = writereg(d, 4);
  raw_fmov_to_s_rr(d, s);
	unlock2(d);
  f_unlock(s);
}
MENDFUNC(2,fmov_to_s_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_w_rr,(W4 d, FR s))
{
	s = f_readreg(s);
  d = rmw(d, 2, 4);
  raw_fmov_to_w_rr(d, s);
	unlock2(d);
  f_unlock(s);
}
MENDFUNC(2,fmov_to_w_rr,(W4 d, FR s))

MIDFUNC(2,fmov_to_b_rr,(W4 d, FR s))
{
	s = f_readreg(s);
  d = rmw(d, 1, 4);
  raw_fmov_to_b_rr(d, s);
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
  d1 = writereg(d1, 4);
  d2 = writereg(d2, 4);
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
  prepare_for_call_1();
  prepare_for_call_2();

	s = f_readreg(s);
	d = f_writereg(d);

	raw_ffunc_rr(func, d, s);

	f_unlock(s);
	f_unlock(d);
}
MENDFUNC(3,ffunc_rr,(double (*func)(double), FW d, FR s))

MIDFUNC(3,fsincos_rr,(FW d, FW c, FR s))
{
  clobber_flags();
  prepare_for_call_1();
  prepare_for_call_2();

	s = f_readreg(s);  /* s for source */
	d = f_writereg(d); /* d for sine   */
	c = f_writereg(c); /* c for cosine */

	raw_ffunc_rr(cos, c, s);
	raw_ffunc_rr(sin, d, s);

	f_unlock(s);
	f_unlock(d);
	f_unlock(c);
}
MENDFUNC(3,fsincos_rr,(FW d, FW c, FR s))

MIDFUNC(3,fpowx_rr,(uae_u32 x, FW d, FR s))
{
  clobber_flags();
  prepare_for_call_1();
  prepare_for_call_2();

	s = f_readreg(s);
	d = f_writereg(d);

	raw_fpowx_rr(x, d, s);

	f_unlock(s);
	f_unlock(d);

}
MENDFUNC(3,fpowx_rr,(uae_u32 x, FW d, FR s))

MIDFUNC(1,fflags_into_flags,())
{
	clobber_flags();
	fflags_into_flags_internal();
}
MENDFUNC(1,fflags_into_flags,())
