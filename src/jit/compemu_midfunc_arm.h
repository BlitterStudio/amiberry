/*
 *  compiler/compemu_midfunc_arm.h - Native MIDFUNCS for ARM
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
 *  	File is included by compemu.h
 *
 */

// Arm optimized midfunc
DECLARE_MIDFUNC(arm_ADD_l(RW4 d, RR4 s));
DECLARE_MIDFUNC(arm_ADD_l_ri(RW4 d, IMM i));
DECLARE_MIDFUNC(arm_ADD_l_ri8(RW4 d, IMM i));
DECLARE_MIDFUNC(arm_SUB_l_ri8(RW4 d, IMM i));

// Emulated midfunc
DECLARE_MIDFUNC(disp_ea20_target_add(RW4 target, RR4 reg, IMM shift, IMM extend));
DECLARE_MIDFUNC(disp_ea20_target_mov(W4 target, RR4 reg, IMM shift, IMM extend));

DECLARE_MIDFUNC(mov_l_mi(IMM d, IMM s));
DECLARE_MIDFUNC(pop_l(W4 d));
DECLARE_MIDFUNC(push_l(RR4 s));
DECLARE_MIDFUNC(sign_extend_16_rr(W4 d, RR2 s));
DECLARE_MIDFUNC(lea_l_brr(W4 d, RR4 s, IMM offset));
DECLARE_MIDFUNC(lea_l_brr_indexed(W4 d, RR4 s, RR4 index, IMM factor, IMM offset));
DECLARE_MIDFUNC(lea_l_rr_indexed(W4 d, RR4 s, RR4 index, IMM factor));
DECLARE_MIDFUNC(mov_l_rr(W4 d, RR4 s));
DECLARE_MIDFUNC(mov_l_mr(IMM d, RR4 s));
DECLARE_MIDFUNC(mov_l_rm(W4 d, IMM s));
DECLARE_MIDFUNC(mov_b_rm(W1 d, IMM s));
DECLARE_MIDFUNC(mov_l_ri(W4 d, IMM s));
DECLARE_MIDFUNC(mov_b_ri(W1 d, IMM s));
DECLARE_MIDFUNC(sub_l_ri(RW4 d, IMM i));
DECLARE_MIDFUNC(sub_w_ri(RW2 d, IMM i));
DECLARE_MIDFUNC(live_flags(void));
DECLARE_MIDFUNC(dont_care_flags(void));
DECLARE_MIDFUNC(make_flags_live(void));
DECLARE_MIDFUNC(forget_about(W4 r));

DECLARE_MIDFUNC(f_forget_about(FW r));
DECLARE_MIDFUNC(dont_care_fflags(void));
DECLARE_MIDFUNC(fmov_rr(FW d, FR s));

DECLARE_MIDFUNC(fmov_l_rr(FW d, RR4 s));
DECLARE_MIDFUNC(fmov_s_rr(FW d, RR4 s));
DECLARE_MIDFUNC(fmov_w_rr(FW d, RR2 s));
DECLARE_MIDFUNC(fmov_b_rr(FW d, RR1 s));
DECLARE_MIDFUNC(fmov_d_rrr(FW d, RR4 s1, RR4 s2));
DECLARE_MIDFUNC(fmov_l_ri(FW d, IMM i));
DECLARE_MIDFUNC(fmov_s_ri(FW d, IMM i));
DECLARE_MIDFUNC(fmov_to_l_rr(W4 d, FR s));
DECLARE_MIDFUNC(fmov_to_s_rr(W4 d, FR s));
DECLARE_MIDFUNC(fmov_to_w_rr(W4 d, FR s));
DECLARE_MIDFUNC(fmov_to_b_rr(W4 d, FR s));
DECLARE_MIDFUNC(fmov_d_ri_0(FW d));
DECLARE_MIDFUNC(fmov_d_ri_1(FW d));
DECLARE_MIDFUNC(fmov_d_ri_10(FW d));
DECLARE_MIDFUNC(fmov_d_ri_100(FW d));
DECLARE_MIDFUNC(fmov_d_rm(FW r, MEMR m));
DECLARE_MIDFUNC(fmovs_rm(FW r, MEMR m));
DECLARE_MIDFUNC(fmov_rm(FW r, MEMR m));
DECLARE_MIDFUNC(fmov_to_d_rrr(W4 d1, W4 d2, FR s));
DECLARE_MIDFUNC(fsqrt_rr(FW d, FR s));
DECLARE_MIDFUNC(fabs_rr(FW d, FR s));
DECLARE_MIDFUNC(fneg_rr(FW d, FR s));
DECLARE_MIDFUNC(fdiv_rr(FRW d, FR s));
DECLARE_MIDFUNC(fadd_rr(FRW d, FR s));
DECLARE_MIDFUNC(fmul_rr(FRW d, FR s));
DECLARE_MIDFUNC(fsub_rr(FRW d, FR s));
DECLARE_MIDFUNC(frndint_rr(FW d, FR s));
DECLARE_MIDFUNC(frndintz_rr(FW d, FR s));
DECLARE_MIDFUNC(fmod_rr(FRW d, FR s));
DECLARE_MIDFUNC(fsgldiv_rr(FRW d, FR s));
DECLARE_MIDFUNC(fcuts_r(FRW r));
DECLARE_MIDFUNC(frem1_rr(FRW d, FR s));
DECLARE_MIDFUNC(fsglmul_rr(FRW d, FR s));
DECLARE_MIDFUNC(fmovs_rr(FW d, FR s));
DECLARE_MIDFUNC(ffunc_rr(double (*func)(double), FW d, FR s));
DECLARE_MIDFUNC(fsincos_rr(FW d, FW c, FR s));
DECLARE_MIDFUNC(fpowx_rr(uae_u32 x, FW d, FR s));
DECLARE_MIDFUNC(fflags_into_flags());
DECLARE_MIDFUNC(fp_from_exten_mr(RR4 adr, FR s));
DECLARE_MIDFUNC(fp_to_exten_rm(FW d, RR4 adr));
DECLARE_MIDFUNC(fp_from_double_mr(RR4 adr, FR s));
DECLARE_MIDFUNC(fp_to_double_rm(FW d, RR4 adr));
DECLARE_MIDFUNC(fp_fscc_ri(RW4, int cc));
DECLARE_MIDFUNC(roundingmode(IMM mode));
