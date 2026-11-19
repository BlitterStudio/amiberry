/*
 * compiler/compemu_fpp.cpp - Dynamic translation of FPU instructions
 *
 * Copyright (c) 2001-2004 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Christian Bauer's Basilisk II
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * JIT compiler m68k -> IA-32 and AMD64
 *
 * Original 68040 JIT compiler for UAE, copyright 2000-2002 Bernd Meyer
 * Adaptation for Basilisk II and improvements, copyright 2000-2004 Gwenole Beauchesne
 * Portions related to CPU detection come from linux/arch/i386/kernel/setup.c
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * UAE - The Un*x Amiga Emulator
 *
 * MC68881 emulation
 *
 * Copyright 1996 Herman ten Brugge
 * Adapted for JIT compilation (c) Bernd Meyer, 2000
 */

#include "sysdeps.h"

#ifdef JIT

#include <cmath>
#include <cstdio>
#include <cassert>

#include "memory-uae.h"
#include "readcpu.h"
#include "newcpu.h"
#include "compemu_x86_64.h"
#include "../compemu_fpp_common.h"
//#include "fpu/fpu.h"
//#include "fpu/flags.h"
//#include "fpu/exceptions.h"
//#include "fpu/rounding.h"

//#define DEBUG 0
#include "debug.h"

struct jit_disable_opcodes jit_disable;

// gb-- WARNING: get_fpcr() and set_fpcr() support is experimental
#define HANDLE_FPCR 0

// - IEEE-based fpu core must be used
#if defined(FPU_IEEE)
# define CAN_HANDLE_FPCR
#endif

// - Generic rounding mode and precision modes are supported if set together
#if defined(FPU_USE_GENERIC_ROUNDING_MODE) && defined(FPU_USE_GENERIC_ROUNDING_PRECISION)
# define CAN_HANDLE_FPCR
#endif

// - X86 rounding mode and precision modes are *not* supported but might work (?!)
#if defined(FPU_USE_X86_ROUNDING_MODE) && defined(FPU_USE_X86_ROUNDING_PRECISION)
# define CAN_HANDLE_FPCR
#endif

#if HANDLE_FPCR && !defined(CAN_HANDLE_FPCR)
# warning "Can't handle FPCR, will FAIL(1) at runtime"
# undef HANDLE_FPCR
# define HANDLE_FPCR 0
#endif

//#define STATIC_INLINE static inline
/* The condition codes come from FP_RESULT. Its writeback (f_tomem) clears
   fpu_fpsr_cc_written, so the per-instruction store is not needed. */
#define MAKE_FPSR(r) do { \
	fmov_rr(FP_RESULT, r); \
	jit_fpu_cc_fpsr = false; \
} while (0)

#if 0
#define delay   nop() ;nop()  
#define delay2  nop() ;nop()   
#else
#define delay
#define delay2
#endif

#define UNKNOWN_EXTRA 0xFFFFFFFF
#if 0
static void fpuop_illg(uae_u32 opcode, uae_u32 /* extra */)
{
/*
	if (extra == UNKNOWN_EXTRA)
		printf("FPU opcode %x, extra UNKNOWN_EXTRA\n",opcode & 0xFFFF);
	else
		printf("FPU opcode %x, extra %x\n",opcode & 0xFFFF,extra & 0xFFFF);
*/
	op_illg(opcode);
}   
#endif

uae_s32 temp_fp[4];  /* To convert between FP/integer */
/* Read a 68k extended value at ad into temp_fp as an x87 extended value.
   The x87 needs the explicit integer bit in normal numbers, infinities and
   NaNs, but a zero or denormal with it set is a different, nonzero value.
   The 68k accepts either, so set it if the exponent is not zero. The mantissa
   is stored with one 64-bit write so that the FLDT can forward it. */
STATIC_INLINE void read_fp_ext_to_temp(int ad, bool advance)
{
	readword(ad, S2, S3);
	mov_w_mr((JITPTR temp_fp) + 8, S2);
	and_l_ri(S2, 0x7fff);
	add_l_ri(S2, 0x7fff);	/* bit 15 is set if the exponent is not zero */
	and_l_ri(S2, 0x8000);
	shll_l_ri(S2, 16);
	add_l_ri(ad, 4);
	readlong(ad, S4, S3);
	or_l(S4, S2);
	add_l_ri(ad, 4);
	readlong(ad, S2, S3);
	if (advance)
		add_l_ri(ad, 4);
	mov_q_mrr_pair(JITPTR temp_fp, S4, S2);
}

/* return register number, or -1 for failure */
STATIC_INLINE int get_fp_value(uae_u32 opcode, uae_u16 extra)
{
	int size;
	int mode;
	int reg;
	uae_u32 ad = 0;

	if ((extra & 0x4000) == 0)
	{
		return ((extra >> 10) & 7);
	}

	mode = (opcode >> 3) & 7;
	reg = opcode & 7;
	size = (extra >> 10) & 7;
	switch (mode)
	{
	case 0: /* Dn */
		switch (size)
		{
		case 6: /* byte */
			sign_extend_8_rr(S1, reg);
			mov_l_mr(JITPTR temp_fp, S1);
			delay2;
			fmovi_rm(FS1, JITPTR temp_fp);
			return FS1;
		case 4: /* word */
			sign_extend_16_rr(S1, reg);
			mov_l_mr(JITPTR temp_fp, S1);
			delay2;
			fmovi_rm(FS1, JITPTR temp_fp);
			return FS1;
		case 0: /* long */
			mov_l_mr(JITPTR temp_fp, reg);
			delay2;
			fmovi_rm(FS1, JITPTR temp_fp);
			return FS1;
		case 1: /* single precision */
			mov_l_mr(JITPTR temp_fp, reg);
			delay2;
			fmovs_rm(FS1, JITPTR temp_fp);
			return FS1;
		default:
			return -1;
		}
		return -1;						/* Should be unreachable */
	case 1: /* An */
		return -1;						/* Genuine invalid instruction */
	default:
		break;
	}

	/* OK, we *will* have to load something from an address. Let's make
	   sure we know how to handle that, or quit early --- i.e. *before*
	   we do any postincrement/predecrement that we may regret */
	switch (size)
	{
	case 0: /* long */
	case 1: /* single precision */
	case 2: /* extended precision */
	case 4: /* word */
	case 5: /* double precision */
	case 6: /* byte */
		break;
	case 3: /* packed decimal static */
	default:
		return -1;
	}

	switch (mode)
	{
	case 2: /* (An) */
		ad = S1;						/* We will change it, anyway ;-) */
		mov_l_rr(ad, reg + 8);
		break;
	case 3: /* (An)+ */
		ad = S1;
		mov_l_rr(ad, reg + 8);
		lea_l_brr(reg + 8, reg + 8, (reg == 7 ? fpp_ea_sz2[size] : fpp_ea_sz1[size]));
		break;
	case 4: /* -(An) */
		ad = S1;
		lea_l_brr(reg + 8, reg + 8, -(reg == 7 ? fpp_ea_sz2[size] : fpp_ea_sz1[size]));
		mov_l_rr(ad, reg + 8);
		break;
	case 5: /* d16(An) */
		{
			uae_u32 off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);

			ad = S1;
			mov_l_rr(ad, reg + 8);
			lea_l_brr(ad, ad, off);
		}
		break;
	case 6: /* d8(An,Xn) */
		{
			uae_u32 dp = comp_get_iword((m68k_pc_offset += 2) - 2);

			ad = S1;
			calc_disp_ea_020(reg + 8, dp, ad, S2);
		}
		break;
	case 7:
		switch (reg)
		{
		case 0: /* abs.w */
			{
				uae_u32 off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);

				ad = S1;
				mov_l_ri(ad, off);
			}
			break;
		case 1: /* abs.l */
			{
				uae_u32 off = comp_get_ilong((m68k_pc_offset += 4) - 4);

				ad = S1;
				mov_l_ri(ad, off);
			}
			break;
		case 2: /* d16(pc) */
			{
				uae_u32 address = (uae_u32)(start_pc + ((char *) comp_pc_p - (char *) start_pc_p) + m68k_pc_offset);
				uae_s32 PC16off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);

				ad = S1;
				mov_l_ri(ad, address + PC16off);
			}
			break;
		case 3: /* d8(pc,Xn) */
			return -1;
		case 4: /* #imm */
			{
				uae_u32 address = (uae_u32)(start_pc + ((char *) comp_pc_p - (char *) start_pc_p) + m68k_pc_offset);

				ad = S1;
				// Immediate addressing mode && Operation Length == Byte -> 
				// Use the low-order byte of the extension word.
				if (size == 6)
					address++;
				mov_l_ri(ad, address);
				m68k_pc_offset += fpp_ea_sz2[size];
			}
			break;
		default:
			return -1;
		}
	}

	switch (size)
	{
	case 0: /* long */
		readlong(ad, S2, S3);
		mov_l_mr(JITPTR temp_fp, S2);
		delay2;
		fmovi_rm(FS1, JITPTR temp_fp);
		break;
	case 1: /* single precision */
		readlong(ad, S2, S3);
		mov_l_mr(JITPTR temp_fp, S2);
		delay2;
		fmovs_rm(FS1, JITPTR temp_fp);
		break;
	case 2: /* extended precision */
		read_fp_ext_to_temp(ad, false);
		delay2;
		fmov_ext_rm(FS1, JITPTR (temp_fp));
		break;
	case 3: /* packed decimal static */
		return -1;						/* Some silly "packed" stuff */
	case 4: /* word */
		readword(ad, S2, S3);
		sign_extend_16_rr(S2, S2);
		mov_l_mr(JITPTR temp_fp, S2);
		delay2;
		fmovi_rm(FS1, JITPTR temp_fp);
		break;
	case 5: /* double precision */
		readlong(ad, S4, S3);
		add_l_ri(ad, 4);
		readlong(ad, S2, S3);
		mov_q_mrr_pair(JITPTR temp_fp, S4, S2);
		delay2;
		fmov_rm(FS1, JITPTR (temp_fp));
		break;
	case 6: /* byte */
		readbyte(ad, S2, S3);
		sign_extend_8_rr(S2, S2);
		mov_l_mr(JITPTR temp_fp, S2);
		delay2;
		fmovi_rm(FS1, JITPTR temp_fp);
		break;
	default:
		return -1;
	}
	return FS1;
}

static struct {
	double b[2];
	double w[2];
	double l[2];
} clamp_bounds = {
	{ -128.0, 127.0 },
	{ -32768.0, 32767.0 },
	{ -2147483648.0, 2147483647.0 }
};

/* return of -1 means failure, >=0 means OK */
STATIC_INLINE int put_fp_value(int val, uae_u32 opcode, uae_u16 extra)
{
	int size;
	int mode;
	int reg;
	uae_u32 ad;

	if ((extra & 0x4000) == 0)
	{
		const int dest_reg = (extra >> 10) & 7;

		fmov_rr(dest_reg, val);
		// gb-- status register is affected
		MAKE_FPSR(dest_reg);
		return 0;
	}

	mode = (opcode >> 3) & 7;
	reg = opcode & 7;
	size = (extra >> 10) & 7;
	ad = (uae_u32) -1;
	switch (mode)
	{
	case 0: /* Dn */
		switch (size)
		{
		case 6: /* byte */
			fmovi_mrb(JITPTR temp_fp, val, clamp_bounds.b);
			delay;
			mov_b_rm(reg, JITPTR temp_fp);
			return 0;
		case 4: /* word */
			fmovi_mrb(JITPTR temp_fp, val, clamp_bounds.w);
			delay;
			mov_w_rm(reg, JITPTR temp_fp);
			return 0;
		case 0: /* long */
			fmovi_mrb(JITPTR temp_fp, val, clamp_bounds.l);
			delay;
			mov_l_rm(reg, JITPTR temp_fp);
			return 0;
		case 1: /* single precision */
			fmovs_mr(JITPTR temp_fp, val);
			delay;
			mov_l_rm(reg, JITPTR temp_fp);
			return 0;
		default:
			return -1;
		}
	case 1: /* An */
		return -1;						/* genuine invalid instruction */
	default:
		break;
	}

	/* Let's make sure we get out *before* doing something silly if
	   we can't handle the size */
	switch (size)
	{
	case 0: /* long */
	case 1: /* single precision */
	case 2: /* extended precision */
	case 4: /* word */
	case 5: /* double precision */
	case 6: /* byte */
		break;
	case 3: /* packed decimal static */
	default:
		return -1;
	}

	switch (mode)
	{
	case 2: /* (An) */
		ad = S1;
		mov_l_rr(ad, reg + 8);
		break;
	case 3: /* (An)+ */
		ad = S1;
		mov_l_rr(ad, reg + 8);
		lea_l_brr(reg + 8, reg + 8, (reg == 7 ? fpp_ea_sz2[size] : fpp_ea_sz1[size]));
		break;
	case 4: /* -(An) */
		ad = S1;
		lea_l_brr(reg + 8, reg + 8, -(reg == 7 ? fpp_ea_sz2[size] : fpp_ea_sz1[size]));
		mov_l_rr(ad, reg + 8);
		break;
	case 5: /* d16(An) */
		{
			uae_u32 off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);

			ad = S1;
			mov_l_rr(ad, reg + 8);
			add_l_ri(ad, off);
		}
		break;
	case 6: /* d8(An,Xn) */
		{
			uae_u32 dp = comp_get_iword((m68k_pc_offset += 2) - 2);

			ad = S1;
			calc_disp_ea_020(reg + 8, dp, ad, S2);
		}
		break;
	case 7:
		switch (reg)
		{
		case 0: /* abs.w */
			{
				uae_u32 off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);

				ad = S1;
				mov_l_ri(ad, off);
			}
			break;
		case 1: /* abs.l */
			{
				uae_u32 off = comp_get_ilong((m68k_pc_offset += 4) - 4);

				ad = S1;
				mov_l_ri(ad, off);
			}
			break;
		case 2: /* d16(pc) */
			{
				uae_u32 address = (uae_u32)(start_pc + ((char *) comp_pc_p - (char *) start_pc_p) + m68k_pc_offset);
				uae_s32 PC16off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);

				ad = S1;
				mov_l_ri(ad, address + PC16off);
			}
			break;
		case 3: /* d8(pc,Xn) */
			return -1;
		case 4: /* #imm */
			{
				uae_u32 address = (uae_u32)(start_pc + ((char *) comp_pc_p - (char *) start_pc_p) + m68k_pc_offset);

				ad = S1;
				mov_l_ri(ad, address);
				m68k_pc_offset += fpp_ea_sz2[size];
			}
			break;
		default:
			return -1;
		}
	}

	switch (size)
	{
	case 0: /* long */
		fmovi_mrb(JITPTR temp_fp, val, clamp_bounds.l);
		delay;
		mov_l_rm(S2, JITPTR temp_fp);
		writelong_clobber(ad, S2, S3);
		break;
	case 1: /* single precision */
		fmovs_mr(JITPTR temp_fp, val);
		delay;
		mov_l_rm(S2, JITPTR temp_fp);
		writelong_clobber(ad, S2, S3);
		break;
	case 2: /* extended precision */
		fmov_ext_mr(JITPTR temp_fp, val);
		delay;
		zero_extend_16_rm(S2, JITPTR temp_fp + 8);
		shll_l_ri(S2, 16);	/* the exponent and a zero pad word */
		writelong_clobber(ad, S2, S3);
		add_l_ri(ad, 4);
		mov_l_rm(S2, JITPTR temp_fp + 4);
		writelong_clobber(ad, S2, S3);
		add_l_ri(ad, 4);
		mov_l_rm(S2, JITPTR temp_fp);
		writelong_clobber(ad, S2, S3);
		break;
	case 3: /* packed decimal static */
		return -1;						/* Packed */
	case 4: /* word */
		fmovi_mrb(JITPTR temp_fp, val, clamp_bounds.w);
		delay;
		mov_l_rm(S2, JITPTR temp_fp);
		writeword_clobber(ad, S2, S3);
		break;
	case 5: /* double precision */
		fmov_mr(JITPTR temp_fp, val);
		delay;
		mov_l_rm(S2, JITPTR temp_fp + 4);
		writelong_clobber(ad, S2, S3);
		add_l_ri(ad, 4);
		mov_l_rm(S2, JITPTR temp_fp);
		writelong_clobber(ad, S2, S3);
		break;
	case 6: /* byte */
		fmovi_mrb(JITPTR temp_fp, val, clamp_bounds.b);
		delay;
		mov_l_rm(S2, JITPTR temp_fp);
		writebyte(ad, S2, S3);
		break;
	default:
		return -1;
	}
	return 0;
}


/* return -1 for failure, or register number for success */
STATIC_INLINE int get_fp_ad(uae_u32 opcode)
{
	int mode;
	int reg;
	uae_s32 off;

	mode = (opcode >> 3) & 7;
	reg = opcode & 7;
	switch (mode)
	{
	case 0: /* Dn */
	case 1: /* An */
		return -1;
	case 2: /* (An) */
	case 3: /* (An)+ */
	case 4: /* -(An) */
		mov_l_rr(S1, 8 + reg);
		return S1;
	case 5: /* d16(An) */
		off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);
		mov_l_rr(S1, 8 + reg);
		add_l_ri(S1, off);
		return S1;
	case 6: /* d8(An,Xn) */
		return -1;
		break;
	case 7:
		switch (reg)
		{
		case 0: /* abs.w */
			off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);
			mov_l_ri(S1, off);
			return S1;
		case 1: /* abs.l */
			off = comp_get_ilong((m68k_pc_offset += 4) - 4);
			mov_l_ri(S1, off);
			return S1;
		case 2: /* d16(pc) */
			off = (uae_s32)(start_pc + ((char *) comp_pc_p - (char *) start_pc_p) + m68k_pc_offset);
			off += (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);
			mov_l_ri(S1, off);
			return S1;
		case 3: /* d8(pc,Xn) */
			return -1;
		default:
			return -1;
		}
	}
	abort();
}


/* return -1 for failure, or register number for success */
void comp_fdbcc_opp (uae_u32 /* opcode */, uae_u16 /* extra */)
{
	if (jit_disable.fdbcc)
	{
		FAIL(1);
		return;
	}
    FAIL(1);
    return;
}


static void fp_signal_bsun(void);

/* S4 = all ones if the predicate holds. A signalling predicate also sets
   BSUN and AE_IOP on a NaN (blocks guarded against the BSUN trap enable). */
static void get_fp_condition(int condition, bool signalling = false)
{
	if (!jit_fp_result_live()) {
		fpu_cond_from_fpsr(S4, S3, fpp_get_cond_mask(condition),
			(jit_fpu_cc_fpsr ? 0 : FPU_COND_CHECK_RESULT) | (signalling ? FPU_COND_SIGNAL : 0));
		jit_fpu_cc_fpsr = true;
		return;
	}
	if (signalling)
		fp_signal_bsun();
	fflags_into_flags(S2);
	mov_l_ri(S1, 0xffffffff);
	mov_l_ri(S4, 0);
	switch (condition & 0x0f)
	{
	case 0:
		break;							/* set never */
	case 1:
		mov_l_rr(S2, S4);
		cmov_l_rr(S4, S1, 4);
		cmov_l_rr(S4, S2, 10);
		break;
	case 2:
		cmov_l_rr(S4, S1, 7);
		break;
	case 3:
		cmov_l_rr(S4, S1, 3);
		break;
	case 4:
		mov_l_rr(S2, S4);
		cmov_l_rr(S4, S1, 2);
		cmov_l_rr(S4, S2, 10);
		break;
	case 5:
		mov_l_rr(S2, S4);
		cmov_l_rr(S4, S1, 6);
		cmov_l_rr(S4, S2, 10);
		break;
	case 6:
		cmov_l_rr(S4, S1, 5);
		break;
	case 7:
		cmov_l_rr(S4, S1, 11);
		break;
	case 8:
		cmov_l_rr(S4, S1, 10);
		break;
	case 9:
		cmov_l_rr(S4, S1, 4);
		break;
	case 10:
		cmov_l_rr(S4, S1, 10);
		cmov_l_rr(S4, S1, 7);
		break;
	case 11:
		cmov_l_rr(S4, S1, 4);
		cmov_l_rr(S4, S1, 3);
		break;
	case 12:
		cmov_l_rr(S4, S1, 2);
		break;
	case 13:
		cmov_l_rr(S4, S1, 6);
		break;
	case 14:
		cmov_l_rr(S4, S1, 5);
		cmov_l_rr(S4, S1, 10);
		break;
	case 15:
		mov_l_rr(S4, S1);
		break;
	}
	/* Discard the temporary x87 comparison flags, not the saved integer CCR */
	dont_care_flags_x87();
}

/* BSUN and AE_IOP if FP_RESULT, in an x87 register, is a NaN */
static void fp_signal_bsun(void)
{
	get_fp_condition(0x08);	/* NaN: all ones */
	and_l_ri(S4, 0x00008080);	/* BSUN | AE_IOP */
	mov_l_rm(S1, JITPTR &regs.fpsr);
	or_l(S1, S4);
	mov_l_mr(JITPTR &regs.fpsr, S1);
}

void comp_fscc_opp(uae_u32 opcode, uae_u16 extra)
{
	const bool signalling = (extra & 0x10) != 0;
	if (!jit_fpu_enabled() || jit_disable.fscc || (extra & 0x20) || (opcode & 0x38) ||
		(signalling && !jit_fpu_fast_block)) {
		FAIL(1);
		return;
	}
	get_fp_condition(extra, signalling);
	mov_b_rr(opcode & 7, S4);
}


void comp_ftrapcc_opp (uae_u32 /* opcode */, uaecptr /* oldpc */)
{
	FAIL(1);
	return;
}


void comp_fbcc_opp(uae_u32 opcode)
{
	uae_u32 start_68k_offset = m68k_pc_offset;
	uae_s32 off;
	uintptr v1;
	uintptr v2;

	if (!jit_fpu_enabled())
	{
		FAIL(1);
		return;
	}
	if (jit_disable.fbcc)
	{
		FAIL(1);
		return;
	}
	if ((opcode & 0x20) || ((opcode & 0x10) && !jit_fpu_fast_block))
	{
		FAIL(1);
		return;
	}
	get_fp_condition(opcode, (opcode & 0x10) != 0);
	if ((opcode & 0x40) == 0)
	{
		off = (uae_s32) (uae_s16) comp_get_iword((m68k_pc_offset += 2) - 2);
	} else
	{
		off = (uae_s32) comp_get_ilong((m68k_pc_offset += 4) - 4);
	}
	/* Match the ARM64 FBcc fix: keep the signed displacement separate
	 * from the 64-bit host pointer so non-PC_P set_const paths do not
	 * lose pointer width or zero-extend backward branches. */
	{
		uae_s32 displacement = off - (uae_s32)(m68k_pc_offset - start_68k_offset);
		mov_l_ri(S1, (uintptr)(uae_s32)displacement);
		add_l_ri(S1, (uintptr)comp_pc_p);
	}
	mov_l_ri(PC_P, JITPTR comp_pc_p);

	/* Now they are both constant. Might as well fold in m68k_pc_offset */
	add_l_ri(S1, m68k_pc_offset);
	add_l_ri(PC_P, m68k_pc_offset);
	m68k_pc_offset = 0;

	v1 = get_const(PC_P);
	v2 = get_const(S1);
	test_l_rr(S4, S4);
	register_branch(v1, v2, 5);
}


    /* Floating point conditions 
       The "NotANumber" part could be problematic; Howver, when NaN is
       encountered, the ftst instruction sets bot N and Z to 1 on the x87,
       so quite often things just fall into place. This is probably not
       accurate wrt the 68k FPU, but it is *as* accurate as this was before.
       However, some more thought should go into fixing this stuff up so
       it accurately emulates the 68k FPU.
>=<U 
0000    0x00: 0                        ---   Never jump
0101    0x01: Z                        ---   jump if zero (x86: 4)
1000    0x02: !(NotANumber || Z || N)  --- Neither Z nor N set (x86: 7)
1101    0x03: Z || !(NotANumber || N); --- Z or !N (x86: 4 and 3)
0010    0x04: N && !(NotANumber || Z); --- N and !Z (x86: hard!)
0111    0x05: Z || (N && !NotANumber); --- Z or N (x86: 6)
1010    0x06: !(NotANumber || Z);      --- not Z (x86: 5)
1110    0x07: !NotANumber;             --- not NaN (x86: 11, not parity)
0001    0x08: NotANumber;              --- NaN (x86: 10)
0101    0x09: NotANumber || Z;         --- Z (x86: 4)
1001    0x0a: NotANumber || !(N || Z); --- NaN or neither N nor Z (x86: 10 and 7)
1101    0x0b: NotANumber || Z || !N;   --- Z or !N (x86: 4 and 3)
0011    0x0c: NotANumber || (N && !Z); --- N (x86: 2)
0111    0x0d: NotANumber || Z || N;    --- Z or N (x86: 6)
1010    0x0e: !Z;                      --- not Z (x86: 5)
1111    0x0f: 1;                       --- always

This is not how the 68k handles things, though --- it sets Z to 0 and N
to the NaN's sign.... ('o' and 'i' denote differences from the above
table)

>=<U 
0000    0x00: 0                        ---   Never jump
010o    0x01: Z                        ---   jump if zero (x86: 4, not 10)
1000    0x02: !(NotANumber || Z || N)  --- Neither Z nor N set (x86: 7)
110o    0x03: Z || !(NotANumber || N); --- Z or !N (x86: 3)
0010    0x04: N && !(NotANumber || Z); --- N and !Z (x86: 2, not 10)
011o    0x05: Z || (N && !NotANumber); --- Z or N (x86: 6, not 10)
1010    0x06: !(NotANumber || Z);      --- not Z (x86: 5)
1110    0x07: !NotANumber;             --- not NaN (x86: 11, not parity)
0001    0x08: NotANumber;              --- NaN (x86: 10)
0101    0x09: NotANumber || Z;         --- Z (x86: 4)
1001    0x0a: NotANumber || !(N || Z); --- NaN or neither N nor Z (x86: 10 and 7)
1101    0x0b: NotANumber || Z || !N;   --- Z or !N (x86: 4 and 3)
0011    0x0c: NotANumber || (N && !Z); --- N (x86: 2)
0111    0x0d: NotANumber || Z || N;    --- Z or N (x86: 6)
101i    0x0e: !Z;                      --- not Z (x86: 5 and 10)
1111    0x0f: 1;                       --- always

Of course, this *still* doesn't mean that the x86 and 68k conditions are
equivalent --- the handling of infinities is different, for one thing.
On the 68k, +infinity minus +infinity is NotANumber (as it should be). On
the x86, it is +infinity, and some exception is raised (which I suspect
is promptly ignored) STUPID! 
The more I learn about their CPUs, the more I detest Intel....

You can see this in action if you have "Benoit" (see Aminet) and
set the exponent to 16. Wait for a long time, and marvel at the extra black
areas outside the center one. That's where Benoit expects NaN, and the x86
gives +infinity. [Ooops --- that must have been some kind of bug in my code.
it no longer happens, and the resulting graphic looks much better, too]

x86 conditions
0011    : 2
1100    : 3
0101    : 4
1010    : 5
0111    : 6
1000    : 7
0001    : 10
1110    : 11
    */

#ifndef UAE
void comp_fsave_opp(uae_u32 opcode)
{
	int incr = (opcode & 0x38) == 0x20 ? -1 : 1;
	int i;
	int ad;

	if (jit_disable.fsave)
	{
		FAIL(1);
		return;
	}
	FAIL(1);
	return;

	if ((ad = get_fp_ad(opcode)) < 0)
	{
		FAIL(1);
		return;
	}

	if (currprefs.fpu_model == 68040)
	{
		/* 4 byte 68040 IDLE frame.  */
		if (incr < 0)
		{
			ad -= 4;
			put_long(ad, 0x41000000);
		} else
		{
			put_long(ad, 0x41000000);
			ad += 4;
		}
	} else
	{
		if (incr < 0)
		{
			ad -= 4;
			put_long(ad, 0x70000000);
			for (i = 0; i < 5; i++)
			{
				ad -= 4;
				put_long(ad, 0x00000000);
			}
			ad -= 4;
			put_long(ad, 0x1f180000);
		} else
		{
			put_long(ad, 0x1f180000);
			ad += 4;
			for (i = 0; i < 5; i++)
			{
				put_long(ad, 0x00000000);
				ad += 4;
			}
			put_long(ad, 0x70000000);
			ad += 4;
		}
	}
	if ((opcode & 0x38) == 0x18)
		m68k_areg(regs, opcode & 7) = ad;
	if ((opcode & 0x38) == 0x20)
		m68k_areg(regs, opcode & 7) = ad;
}


void comp_frestore_opp(uae_u32 opcode)
{
	uae_u32 d;
	int incr = (opcode & 0x38) == 0x20 ? -1 : 1;
	int ad;

	if (jit_disable.frestore)
	{
		FAIL(1);
		return;
	}
	FAIL(1);
	return;

	if ((ad = get_fp_ad(opcode)) < 0)
	{
		FAIL(1);
		return;
	}
	if (currprefs.fpu_model == 68040)
	{
		/* 68040 */
		if (incr < 0)
		{
			/* @@@ This may be wrong.  */
			ad -= 4;
			d = get_long(ad);
			if ((d & 0xff000000) != 0)
			{							/* Not a NULL frame? */
				if ((d & 0x00ff0000) == 0)
				{						/* IDLE */
				} else if ((d & 0x00ff0000) == 0x00300000)
				{						/* UNIMP */
					ad -= 44;
				} else if ((d & 0x00ff0000) == 0x00600000)
				{						/* BUSY */
					ad -= 92;
				}
			}
		} else
		{
			d = get_long(ad);
			ad += 4;
			if ((d & 0xff000000) != 0)
			{							/* Not a NULL frame? */
				if ((d & 0x00ff0000) == 0)
				{						/* IDLE */
				} else if ((d & 0x00ff0000) == 0x00300000)
				{						/* UNIMP */
					ad += 44;
				} else if ((d & 0x00ff0000) == 0x00600000)
				{						/* BUSY */
					ad += 92;
				}
			}
		}
	} else
	{
		if (incr < 0)
		{
			ad -= 4;
			d = get_long(ad);
			if ((d & 0xff000000) != 0)
			{
				if ((d & 0x00ff0000) == 0x00180000)
					ad -= 6 * 4;
				else if ((d & 0x00ff0000) == 0x00380000)
					ad -= 14 * 4;
				else if ((d & 0x00ff0000) == 0x00b40000)
					ad -= 45 * 4;
			}
		} else
		{
			d = get_long(ad);
			ad += 4;
			if ((d & 0xff000000) != 0)
			{
				if ((d & 0x00ff0000) == 0x00180000)
					ad += 6 * 4;
				else if ((d & 0x00ff0000) == 0x00380000)
					ad += 14 * 4;
				else if ((d & 0x00ff0000) == 0x00b40000)
					ad += 45 * 4;
			}
		}
	}
	if ((opcode & 0x38) == 0x18)
		m68k_areg(regs, opcode & 7) = ad;
	if ((opcode & 0x38) == 0x20)
		m68k_areg(regs, opcode & 7) = ad;
}
#endif

/* x87 control words, indexed with fldcw_m_indexed() by the rounding precision
   and mode bits of the 68k FPCR (byte offset fpcr & 0xf0): those
   set_fpucw_x87() (fpp_native.cpp) loads for the interpreted instructions, so
   that every precision is the double precision: like the host core, single
   precision rounds the double result (the store after the operation) */
static uae_u16 x86_fpucw[]={
    0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, /* p0r0 */
    0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, /* p0r1 */
    0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, /* p0r2 */
    0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, /* p0r3 */

    0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, /* p1r0 */
    0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, /* p1r1 */
    0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, /* p1r2 */
    0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, /* p1r3 */

    0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, /* p2r0 */
    0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, /* p2r1 */
    0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, /* p2r2 */
    0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, /* p2r3 */

    0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, 0x127f, /* p3r0 */
    0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, 0x1e7f, /* p3r1 */
    0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, 0x167f, /* p3r2 */
    0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f, 0x1a7f /* p3r3 */
};


static bool fp_native_disabled(uae_u16 extra)
{
	const int opclass = extra >> 13;
	if (opclass == 6 || opclass == 7)
		return jit_disable.fmovem;
	if (opclass == 3)
		return jit_disable.fmove;
	if ((extra & 0xfc00) == 0x5c00)
		return jit_disable.fmovecr;
	int op = extra & 0x7f;
	if (op & 0x40) {
		switch (op) {
		case 0x40: case 0x44: op = 0x00; break;
		case 0x41: case 0x45: op = 0x04; break;
		case 0x58: case 0x5c: op = 0x18; break;
		case 0x5a: case 0x5e: op = 0x1a; break;
		case 0x60: case 0x64: op = 0x20; break;
		case 0x62: case 0x66: op = 0x22; break;
		case 0x63: case 0x67: op = 0x23; break;
		case 0x68: case 0x6c: op = 0x28; break;
		default: return false;
		}
	}
	static const bool jit_disable_opcodes::*const flags[64] = {
		&jit_disable_opcodes::fmove, &jit_disable_opcodes::fint,
		&jit_disable_opcodes::fsinh, &jit_disable_opcodes::fintrz,
		&jit_disable_opcodes::fsqrt, &jit_disable_opcodes::fsqrt,
		&jit_disable_opcodes::flognp1, &jit_disable_opcodes::flognp1,
		&jit_disable_opcodes::fetoxm1, &jit_disable_opcodes::ftanh,
		&jit_disable_opcodes::fatan, &jit_disable_opcodes::fatan,
		&jit_disable_opcodes::fasin, &jit_disable_opcodes::fatanh,
		&jit_disable_opcodes::fsin, &jit_disable_opcodes::ftan,
		&jit_disable_opcodes::fetox, &jit_disable_opcodes::ftwotox,
		&jit_disable_opcodes::ftentox, &jit_disable_opcodes::ftentox,
		&jit_disable_opcodes::flogn, &jit_disable_opcodes::flog10,
		&jit_disable_opcodes::flog2, &jit_disable_opcodes::flog2,
		&jit_disable_opcodes::fabs, &jit_disable_opcodes::fcosh,
		&jit_disable_opcodes::fneg, &jit_disable_opcodes::fneg,
		&jit_disable_opcodes::facos, &jit_disable_opcodes::fcos,
		&jit_disable_opcodes::fgetexp, &jit_disable_opcodes::fgetman,
		&jit_disable_opcodes::fdiv, &jit_disable_opcodes::fmod,
		&jit_disable_opcodes::fadd, &jit_disable_opcodes::fmul,
		&jit_disable_opcodes::fsgldiv, &jit_disable_opcodes::frem,
		&jit_disable_opcodes::fscale, &jit_disable_opcodes::fsglmul,
		&jit_disable_opcodes::fsub, &jit_disable_opcodes::fsub,
		&jit_disable_opcodes::fsub, &jit_disable_opcodes::fsub,
		&jit_disable_opcodes::fsub, &jit_disable_opcodes::fsub,
		&jit_disable_opcodes::fsub, &jit_disable_opcodes::fsub,
		&jit_disable_opcodes::fsincos, &jit_disable_opcodes::fsincos,
		&jit_disable_opcodes::fsincos, &jit_disable_opcodes::fsincos,
		&jit_disable_opcodes::fsincos, &jit_disable_opcodes::fsincos,
		&jit_disable_opcodes::fsincos, &jit_disable_opcodes::fsincos,
		&jit_disable_opcodes::fcmp, &jit_disable_opcodes::fcmp,
		&jit_disable_opcodes::ftst, &jit_disable_opcodes::ftst,
		&jit_disable_opcodes::fcmp, &jit_disable_opcodes::fcmp,
		&jit_disable_opcodes::ftst, &jit_disable_opcodes::ftst,
	};
	return jit_disable.*flags[op];
}

static int get_fp_native_ea(uae_u32 opcode, uae_u16 extra)
{
	const int opclass = extra >> 13;
	if (opclass == 6 || opclass == 7)
		return get_fp_ad(opcode);
	if (!(extra & 0x4000) || (extra & 0xfc00) == 0x5c00) {
		mov_l_ri(S1, 0);
		return S1;
	}
	const int mode = (opcode >> 3) & 7;
	const int reg = opcode & 7;
	const int size = (extra >> 10) & 7;
	static const int sizes[8] = {4, 4, 12, 12, 2, 8, 1, 12};
	const int bytes = reg == 7 && size == 6 ? 2 : sizes[size];
	switch (mode) {
	case 0:
		if (size != 0 && size != 1 && size != 4 && size != 6)
			return -1;
		mov_l_ri(S1, 0);
		return S1;
	case 1:
		return -1;
	case 2:
	case 3:
		mov_l_rr(S1, reg + 8);
		return S1;
	case 4:
		lea_l_brr(S1, reg + 8, -bytes);
		return S1;
	case 5: {
		const uae_s32 displacement = (uae_s16)comp_get_iword((m68k_pc_offset += 2) - 2);
		lea_l_brr(S1, reg + 8, displacement);
		return S1;
	}
	case 6: {
		const uae_u16 extension = comp_get_iword((m68k_pc_offset += 2) - 2);
		calc_disp_ea_020(reg + 8, extension, S1, S2);
		return S1;
	}
	case 7:
		switch (reg) {
		case 0:
			mov_l_ri(S1, (uae_s32)(uae_s16)comp_get_iword((m68k_pc_offset += 2) - 2));
			return S1;
		case 1:
			mov_l_ri(S1, comp_get_ilong((m68k_pc_offset += 4) - 4));
			return S1;
		case 2: {
			if (opclass == 3)
				return -1;
			const uaecptr pc = start_pc + (comp_pc_p - start_pc_p) + m68k_pc_offset;
			const uae_s32 displacement = (uae_s16)comp_get_iword((m68k_pc_offset += 2) - 2);
			mov_l_ri(S1, pc + displacement);
			return S1;
		}
		case 4: {
			if (opclass == 3)
				return -1;
			uaecptr pc = start_pc + (comp_pc_p - start_pc_p) + m68k_pc_offset;
			if (size == 6)
				++pc;
			mov_l_ri(S1, pc);
			m68k_pc_offset += size == 6 ? 2 : sizes[size];
			return S1;
		}
		}
	}
	return -1;
}

/* Inline x87 code: an FPU instruction becomes a few x87 instructions instead
   of a call into the FPU core. Used in blocks that compile_block() compiles
   for an FPCR without the bits it cannot follow (fpu_jit_fpcr_mode()); at
   single precision it rounds the results as the host core does. The x87
   sticky flags
   give the accrued exception byte (fpp.cpp); the exception byte is not
   updated. Everything else goes through the native helper. */
bool jit_fpu_fast_enabled(void)
{
	return jit_fpu_enabled() && !currprefs.fpu_strict && currprefs.fpu_model > 0 &&
		currprefs.fpu_mode != FPU_MODE_EXACT && !(regs.pcr & 2);
}

static bool fp_fast_ea(uae_u32 opcode, uae_u16 extra, bool store)
{
	const int mode = (opcode >> 3) & 7;
	const int reg = opcode & 7;
	const int size = (extra >> 10) & 7;
	if (size == 3 || size == 7)
		return false;	/* packed */
	switch (mode) {
	case 0:
		return size == 0 || size == 1 || size == 4 || size == 6;
	case 1:
		return false;
	case 7:
		return reg <= 1 || (!store && (reg == 2 || reg == 4));
	default:
		return true;
	}
}

/* FMOVECR results for each FPCR rounding mode, 16 bytes apart so that the
   FPCR rounding bits index them */
static double fp_fast_fmovecr_tab[64][8] __attribute__((aligned(16)));

/* Fills the table entry for FMOVECR #ofs; the accrued bits it sets in *accr.
   False: the inline code cannot do it */
static bool fp_fast_fmovecr(uae_u16 extra, uae_u32 *accr, bool *by_mode)
{
	if ((extra & 0x40) || (currprefs.cpu_model >= 68040 && currprefs.fpu_no_unimplemented))
		return false;
	double val[4];
	uae_u32 acc[4];
	if (!fp_jit_fmovecr_host(extra, val, acc))
		return false;
	/* where the x87 control word does not follow the FPCR rounding mode,
	   compiled code runs only at round to nearest */
	const int modes = FPU_JIT_ROUNDING_MASK ? 1 : 4;
	*by_mode = false;
	for (int i = 0; i < modes; i++) {
		if (acc[i] != acc[0])
			return false;
		if (memcmp(&val[i], &val[0], sizeof(double)))
			*by_mode = true;
		fp_fast_fmovecr_tab[extra & 0x3f][i * 2] = val[i];
	}
	*accr = acc[0];
	return true;
}

/* Decides before any code or extension word is consumed */
static bool fp_fast_supported(uae_u32 opcode, uae_u16 extra)
{
	const int opclass = extra >> 13;
	const int mode = (opcode >> 3) & 7;
	const int reg = opcode & 7;
	const bool fpu_040_060 = currprefs.fpu_model == 68040 || currprefs.fpu_model == 68060;

	switch (opclass) {
	case 0:
	case 2:
		if ((extra & 0xfc00) == 0x5c00) {
			uae_u32 accr;
			bool by_mode;
			/* the precision changes the constants, see fpu_get_constant() */
			return !jit_fpu_single_block && fp_fast_fmovecr(extra, &accr, &by_mode);
		}
		if (opclass == 2 && !fp_fast_ea(opcode, extra, false))
			return false;
		switch (extra & 0x7f) {
		case 0x00: case 0x04: case 0x18: case 0x1a: case 0x20:
		case 0x22: case 0x23: case 0x28: case 0x38: case 0x3a:
			return true;
		case 0x01: case 0x03:	/* FINT, FINTRZ: unimplemented in the 68040 */
			return !(currprefs.cpu_model == 68040 && currprefs.fpu_no_unimplemented);
		case 0x40: case 0x44: case 0x41: case 0x45: case 0x58: case 0x5c:
		case 0x5a: case 0x5e: case 0x60: case 0x64: case 0x62: case 0x66:
		case 0x63: case 0x67: case 0x68: case 0x6c:
			return fpu_040_060;
		}
		return false;
	case 3:
		return fp_fast_ea(opcode, extra, true);
	case 6:
	case 7: {
		const int list_mode = (extra >> 11) & 3;
		if (list_mode & 1)
			return false;	/* dynamic register list */
		if (opclass == 7) {
			if (list_mode == 0)
				return mode == 4;
			return mode == 2 || mode == 5 || (mode == 7 && reg <= 1);
		}
		return list_mode == 2 &&
			(mode == 2 || mode == 3 || mode == 5 || (mode == 7 && reg <= 2));
	}
	}
	return false;
}

/* The FPIAR store of an inline instruction waits until an instruction that can
   see the FPIAR: one that is not inline FPU code without a memory access (which
   may fault), or the end of the block */
void comp_fpp_fpiar_flush(void)
{
	if (jit_fpu_fpiar_pending) {
		mov_l_mi(JITPTR &regs.fpiar, jit_fpu_fpiar_pending_pc);
		jit_fpu_fpiar_pending = false;
	}
}

/* Before the instruction at location is compiled */
void comp_fpp_fpiar_next(uae_u32 opcode, const uae_u16 *location)
{
	if (!jit_fpu_fpiar_pending)
		return;
	if ((opcode & 0xffc0) == 0xf200 && !special_mem && jit_fpu_fast_block &&
		jit_fpu_enabled() && currprefs.fpu_mode != FPU_MODE_EXACT) {
		const uae_u16 extra = do_get_mem_word((uae_u16 *)location + 1);
		const int opclass = extra >> 13;
		const int mode = (opcode >> 3) & 7;
		const bool no_mem = opclass == 0 ||
			((opclass == 2 || opclass == 3) && (mode == 0 || (mode == 7 && (opcode & 7) == 4)));
		if (no_mem && !fp_native_disabled(extra) && fp_fast_supported(opcode, extra))
			return;
	}
	comp_fpp_fpiar_flush();
}

/* x87 control word for the FPCR's rounding mode and precision */
static void fp_fast_set_cw(void)
{
	mov_l_rm(S1, JITPTR &regs.fpcr);
	and_l_ri(S1, 0xf0);
	fldcw_m_indexed(S1, JITPTR x86_fpucw);
}

static void comp_fmovem_fast(uae_u32 opcode, uae_u16 extra)
{
	uae_u32 list = extra & 0xff;
	int reg;
	const int ad = get_fp_ad(opcode);

	if (extra & 0x2000) {
		/* FMOVEM FPP->memory */
		if (((extra >> 11) & 3) == 0) {
			/* predecrement: the list is FP7..FP0 */
			for (reg = 7; reg >= 0; reg--) {
				if (list & 0x80) {
					fmov_ext_mr(JITPTR temp_fp, reg);
					delay;
					sub_l_ri(ad, 4);
					mov_l_rm(S2, JITPTR temp_fp);
					writelong_clobber(ad, S2, S3);
					sub_l_ri(ad, 4);
					mov_l_rm(S2, JITPTR temp_fp + 4);
					writelong_clobber(ad, S2, S3);
					sub_l_ri(ad, 4);
					zero_extend_16_rm(S2, JITPTR temp_fp + 8);
					shll_l_ri(S2, 16);	/* the exponent and a zero pad word */
					writelong_clobber(ad, S2, S3);
				}
				list <<= 1;
			}
		} else {
			for (reg = 0; reg < 8; reg++) {
				if (list & 0x80) {
					fmov_ext_mr(JITPTR temp_fp, reg);
					delay;
					zero_extend_16_rm(S2, JITPTR temp_fp + 8);
					shll_l_ri(S2, 16);
					writelong_clobber(ad, S2, S3);
					add_l_ri(ad, 4);
					mov_l_rm(S2, JITPTR temp_fp + 4);
					writelong_clobber(ad, S2, S3);
					add_l_ri(ad, 4);
					mov_l_rm(S2, JITPTR temp_fp);
					writelong_clobber(ad, S2, S3);
					add_l_ri(ad, 4);
				}
				list <<= 1;
			}
		}
	} else {
		/* FMOVEM memory->FPP, postincrement or control */
		for (reg = 0; reg < 8; reg++) {
			if (list & 0x80) {
				read_fp_ext_to_temp(ad, true);
				delay2;
				fmov_ext_rm(reg, JITPTR (temp_fp));
			}
			list <<= 1;
		}
	}
	if ((opcode & 0x38) == 0x18 || (opcode & 0x38) == 0x20)
		mov_l_rr((opcode & 7) + 8, ad);
}

static void comp_fpp_fast(uae_u32 opcode, uae_u16 extra)
{
	const int opclass = extra >> 13;
	const uaecptr pc = start_pc + (comp_pc_p - start_pc_p) + m68k_pc_offset - 4;

	if (!jit_fpu_fast_state) {
		/* constant for the rest of the block, unless an instruction runs in
		   the interpreter or the helper; also stands for regs.fpu_state = 1
		   (fpp.cpp sets it when it reads the flag) */
		jit_fpu_mark_x87_dirty();
		jit_fpu_fast_state = true;
	}
	if (opclass == 6 || opclass == 7) {
		comp_fmovem_fast(opcode, extra);
		return;
	}
	if (currprefs.fpu_model == 68040 || currprefs.fpu_model == 68060) {
		jit_fpu_fpiar_pending = true;
		jit_fpu_fpiar_pending_pc = pc;
	}
	if (opclass == 3) {
		put_fp_value((extra >> 7) & 7, opcode, extra);
		return;
	}

	const int reg = (extra >> 7) & 7;
	if ((extra & 0xfc00) == 0x5c00) {	/* FMOVECR */
		uae_u32 accr = 0;
		bool by_mode = false;
		fp_fast_fmovecr(extra, &accr, &by_mode);
		double *tab = fp_fast_fmovecr_tab[extra & 0x3f];
		dont_care_fflags();
		if (by_mode) {
			mov_l_rm(S1, JITPTR &regs.fpcr);
			and_l_ri(S1, 0x30);
			fmov_rm_indexed(reg, S1, JITPTR tab);
		} else {
			fmov_rm(reg, JITPTR tab);
		}
		if (accr) {
			mov_l_rm(S2, JITPTR &regs.fpsr);
			or_l_ri(S2, accr);
			mov_l_mr(JITPTR &regs.fpsr, S2);
		}
		MAKE_FPSR(reg);
		return;
	}
	const int op = extra & 0x7f;
	int base = op;
	switch (op) {
	case 0x40: case 0x44: base = 0x00; break;
	case 0x41: case 0x45: base = 0x04; break;
	case 0x58: case 0x5c: base = 0x18; break;
	case 0x5a: case 0x5e: base = 0x1a; break;
	case 0x60: case 0x64: base = 0x20; break;
	case 0x62: case 0x66: base = 0x22; break;
	case 0x63: case 0x67: base = 0x23; break;
	case 0x68: case 0x6c: base = 0x28; break;
	}
	/* FSxxx and FDxxx round to single or double precision, and so does
	   everything but FINT and FINTRZ at FPCR single precision */
	int prec = (op & 0x40) ? ((op & 0x04) ? 2 : 1) : 0;
	if (!prec && jit_fpu_single_block && base != 0x01 && base != 0x03)
		prec = 1;
	/* the x87 control word gives double precision (x86_fpucw[]): the store
	   after the operation rounds to single as the host core does after its
	   double operation, and double needs nothing */
	/* moves of a source the precision holds exactly need no rounding */
	const int size = (extra >> 10) & 7;
	const bool exact_src = opclass == 2 && (base == 0x00 || base == 0x18 || base == 0x1a) &&
		(size == 1 || size == 4 || size == 6 || (prec == 2 && (size == 0 || size == 5)));

	if (base != 0x38 && base != 0x3a)
		dont_care_fflags();
	const int src = get_fp_value(opcode, extra);
	switch (base) {
	case 0x00:	/* FMOVE */
		fmov_rr(reg, src);
		break;
	case 0x01:	/* FINT */
		frndint_rr(reg, src);
		break;
	case 0x03:	/* FINTRZ */
		mov_l_ri(S1, 0x10);	/* round to zero */
		fldcw_m_indexed(S1, JITPTR x86_fpucw);
		frndint_rr(reg, src);
		fp_fast_set_cw();
		break;
	case 0x04:	/* FSQRT */
		fsqrt_rr(reg, src);
		break;
	case 0x18:	/* FABS */
		fabs_rr(reg, src);
		break;
	case 0x1a:	/* FNEG */
		fneg_rr(reg, src);
		break;
	case 0x20:	/* FDIV */
		fdiv_rr(reg, src);
		break;
	case 0x22:	/* FADD */
		fadd_rr(reg, src);
		break;
	case 0x23:	/* FMUL */
		fmul_rr(reg, src);
		break;
	case 0x28:	/* FSUB */
		fsub_rr(reg, src);
		break;
	case 0x38:	/* FCMP */
		/* the condition codes straight into the FPSR, not regs.fp_result */
		fcmp_cc_rrr(S1, reg, src, currprefs.fpu_model == 68040);
		dont_care_fflags();
		mov_l_rm(S2, JITPTR &regs.fpsr);
		and_l_ri(S2, 0x00fffff8);
		or_l(S2, S1);
		mov_l_mr(JITPTR &regs.fpsr, S2);
		mov_b_mi(JITPTR &fpu_fpsr_cc_written, 1);
		jit_fpu_cc_fpsr = true;
		return;
	case 0x3a:	/* FTST */
		MAKE_FPSR(src);
		return;
	}
	if (prec && !exact_src) {
		/* the store also limits the exponent range */
		if (prec == 1) {
			fmovs_mr(JITPTR temp_fp, reg);
			fmovs_rm(reg, JITPTR temp_fp);
		} else {
			fmov_mr(JITPTR temp_fp, reg);
			fmov_rm(reg, JITPTR temp_fp);
		}
	}
	MAKE_FPSR(reg);
}

void comp_fpp_opp(uae_u32 opcode, uae_u16 extra)
{
	if (special_mem)
	{
		FAIL(1);
		return;
	}
	if (!jit_fpu_enabled())
	{
		FAIL(1);
		return;
	}
	if (currprefs.fpu_mode != FPU_MODE_EXACT) { // the host core
		const int opclass = extra >> 13;
		if (opclass == 0 || opclass == 2 || opclass == 3 || opclass == 6 || opclass == 7) {
			if (fp_native_disabled(extra)) {
				FAIL(1);
				return;
			}
			if (jit_fpu_fast_block && fp_fast_supported(opcode, extra)) {
				comp_fpp_fast(opcode, extra);
				return;
			}
			const uaecptr pc = start_pc + (comp_pc_p - start_pc_p) + m68k_pc_offset - 4;
			const int ea = get_fp_native_ea(opcode, extra);
			if (ea < 0) {
				FAIL(1);
				return;
			}
			// Keep EA calculation and the surrounding block compiled. The native
			// operation helper supplies per-instruction status, rounding and traps.
			const uaecptr nextpc = start_pc + (comp_pc_p - start_pc_p) + m68k_pc_offset;
			fpu_native_call((opcode << 16) | extra, ea, pc, nextpc);
			mov_l_ri(PC_P, (uintptr)comp_pc_p);
			jit_fpu_fast_state = false;
			jit_fpu_loop_ok = false;
			return;
		}
	}
	switch ((extra >> 13) & 0x7)
	{
	case 4:							/* FMOVEM <ea>,<control> */
	case 5:							/* FMOVEM <control>,<ea> */
		if (jit_disable.fmovec)
		{
			FAIL(1);
			return;
		}

		/* rare */
		if ((opcode & 0x30) == 0)
		{
			/* <ea> = Dn or An */
			if (extra & 0x2000)
			{
				if (extra & 0x1000)
				{
#if HANDLE_FPCR
					mov_l_rm(opcode & 15, (uintptr) & fpu.fpcr.rounding_mode);
					or_l_rm(opcode & 15, (uintptr) & fpu.fpcr.rounding_precision);
#else
					FAIL(1);
					return;
#endif
				}
				if (extra & 0x0800)
				{
					FAIL(1);
					return;
				}
				if (extra & 0x0400)
				{
					/* FPIAR: fixme; we cannot correctly return the address from compiled code */
#ifdef UAE
					mov_l_rm(opcode & 15, JITPTR &regs.fpiar);
#else
					mov_l_rm(opcode & 15, JITPTR &fpu.instruction_address);
#endif
					return;
				}
			} else
			{
				// gb-- moved here so that we may FAIL() without generating any code
				if (extra & 0x0800)
				{
					// set_fpsr(m68k_dreg (regs, opcode & 15));
					FAIL(1);
					return;
				}
				if (extra & 0x1000)
				{
#if HANDLE_FPCR
#if defined(FPU_USE_X86_ROUNDING_MODE) && defined(FPU_USE_X86_ROUNDING_PRECISION)
					FAIL(1);
					return;
#endif
					mov_l_rr(S1, opcode & 15);
					mov_l_rr(S2, opcode & 15);
					and_l_ri(S1, FPCR_ROUNDING_PRECISION);
					and_l_ri(S2, FPCR_ROUNDING_MODE);
					mov_l_mr((uintptr) & fpu.fpcr.rounding_precision, S1);
					mov_l_mr((uintptr) & fpu.fpcr.rounding_mode, S2);
#else
					FAIL(1);
					return;
#endif
				}
				if (extra & 0x0400)
				{
					/* FPIAR: does that make sense at all? */
#ifdef UAE
					mov_l_mr(JITPTR &regs.fpiar, opcode & 15);
#else
					mov_l_mr(JITPTR &fpu.instruction_address, opcode & 15);
#endif
				}
				return;
			}
		} else if ((opcode & 0x3f) == 0x3c)
		{
			/* <ea> = #imm */
			if ((extra & 0x2000) == 0)
			{
				// gb-- moved here so that we may FAIL() without generating any code
				if (extra & 0x0800)
				{
					FAIL(1);
					return;
				}
				if (extra & 0x1000)
				{
					comp_get_ilong((m68k_pc_offset += 4) - 4);
#if HANDLE_FPCR
#if defined(FPU_USE_X86_ROUNDING_MODE) && defined(FPU_USE_X86_ROUNDING_PRECISION)
					FAIL(1);
					return;
#endif
					// mov_l_mi((uintptr)&regs.fpcr,val);
					mov_l_ri(S1, val);
					mov_l_ri(S2, val);
					and_l_ri(S1, FPCR_ROUNDING_PRECISION);
					and_l_ri(S2, FPCR_ROUNDING_MODE);
					mov_l_mr((uintptr) & fpu.fpcr.rounding_precision, S1);
					mov_l_mr((uintptr) & fpu.fpcr.rounding_mode, S2);
#else
					FAIL(1);
					return;
#endif
				}
				if (extra & 0x0400)
				{
					uae_u32 val = comp_get_ilong((m68k_pc_offset += 4) - 4);
#ifdef UAE
					mov_l_mi(JITPTR &regs.fpiar, val);
#else
					mov_l_mi(JITPTR &fpu.instruction_address, val);
#endif
				}
				return;
			}
			FAIL(1);
			return;
		} else if (extra & 0x2000)
		{
			FAIL(1);
			return;
		} else
		{
			FAIL(1);
			return;
		}
		FAIL(1);
		return;

	default:
		break;
	}
	FAIL(1);
}

#endif
