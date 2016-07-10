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
DECLARE_MIDFUNC(mov_l_mi(IMM d, IMM s));
DECLARE_MIDFUNC(shll_l_ri(RW4 r, IMM i));
DECLARE_MIDFUNC(pop_l(W4 d));
DECLARE_MIDFUNC(push_l(RR4 s));
DECLARE_MIDFUNC(sign_extend_16_rr(W4 d, RR2 s));
DECLARE_MIDFUNC(mov_b_rr(W1 d, RR1 s));
DECLARE_MIDFUNC(mov_w_rr(W2 d, RR2 s));
DECLARE_MIDFUNC(lea_l_brr(W4 d, RR4 s, IMM offset));
DECLARE_MIDFUNC(lea_l_brr_indexed(W4 d, RR4 s, RR4 index, IMM factor, IMM offset));
DECLARE_MIDFUNC(lea_l_rr_indexed(W4 d, RR4 s, RR4 index, IMM factor));
DECLARE_MIDFUNC(mov_l_rr(W4 d, RR4 s));
DECLARE_MIDFUNC(mov_l_mr(IMM d, RR4 s));
DECLARE_MIDFUNC(mov_b_rm(W1 d, IMM s));
DECLARE_MIDFUNC(mov_l_ri(W4 d, IMM s));
DECLARE_MIDFUNC(mov_w_ri(W2 d, IMM s));
DECLARE_MIDFUNC(mov_b_ri(W1 d, IMM s));
DECLARE_MIDFUNC(add_l(RW4 d, RR4 s));
DECLARE_MIDFUNC(sub_l_ri(RW4 d, IMM i));
DECLARE_MIDFUNC(sub_w_ri(RW2 d, IMM i));
DECLARE_MIDFUNC(add_l_ri(RW4 d, IMM i));
DECLARE_MIDFUNC(call_r_02(RR4 r, RR4 in1, RR4 in2, IMM isize1, IMM isize2));
DECLARE_MIDFUNC(call_r_11(W4 out1, RR4 r, RR4 in1, IMM osize, IMM isize));
DECLARE_MIDFUNC(live_flags(void));
DECLARE_MIDFUNC(dont_care_flags(void));
DECLARE_MIDFUNC(make_flags_live(void));
DECLARE_MIDFUNC(forget_about(W4 r));

DECLARE_MIDFUNC(f_forget_about(FW r));
