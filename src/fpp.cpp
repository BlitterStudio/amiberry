/*
* UAE - The Un*x Amiga Emulator
*
* MC68881/68882/68040/68060 FPU emulation
*
* Copyright 1996 Herman ten Brugge
* Modified 2005 Peter Keunecke
* 68040+ exceptions and more by Toni Wilen
*/

#define __USE_ISOC9X  /* We might be able to pick up a NaN */

#define FPU_TEST 0
#define FPU_LOG 0

#include <cmath>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "fpp.h"
#include "savestate.h"
#include "cpummu030.h"
#include "uae.h"

#ifndef CPU_TESTER
#define SUPPORT_MMU 1
#else
#define SUPPORT_MMU 0
extern void cputester_fault(void);
#endif

#ifdef WITH_SOFTFLOAT
#include "softfloat/softfloat.h"
#endif

// global variable for JIT FPU
#ifdef USE_LONG_DOUBLE
bool use_long_double = true;
#else
bool use_long_double = false;
#endif

static bool support_exceptions;
static bool support_denormals;
static uae_u32 fpcr_mask, fpsr_mask;
bool fpu_fpsr_cc_written;

FPP_PRINT fpp_print;

FPP_IS fpp_unset_snan;
FPP_IS fpp_is_init;
FPP_IS fpp_is_snan;
FPP_IS fpp_is_nan;
FPP_IS fpp_is_infinity;
FPP_IS fpp_is_zero;
FPP_IS fpp_is_neg;
FPP_IS fpp_is_denormal;
FPP_IS fpp_is_unnormal;
FPP_IS fpp_is_denormal_or_unnormal;
FPP_GET_CC fpp_get_cc;
FPP_A fpp_fix_infinity;

FPP_GET_STATUS fpp_get_status;
FPP_CLEAR_STATUS fpp_clear_status;
FPP_SET_MODE fpp_set_mode;
FPP_SUPPORT_FLAGS fpp_get_support_flags;

FPP_FROM_NATIVE fpp_from_native;
FPP_TO_NATIVE fpp_to_native;

FPP_TO_INT fpp_to_int;
FPP_FROM_INT fpp_from_int;

FPP_PACK fpp_to_pack;
FPP_PACK fpp_from_pack;

FPP_TO_SINGLE fpp_to_single;
FPP_FROM_SINGLE fpp_from_single;
FPP_TO_DOUBLE fpp_to_double;
FPP_FROM_DOUBLE fpp_from_double;
FPP_TO_EXTEN fpp_to_exten;
FPP_FROM_EXTEN fpp_from_exten;
FPP_TO_EXTEN fpp_to_exten_fmovem;
FPP_FROM_EXTEN fpp_from_exten_fmovem;

FPP_A fpp_normalize;
FPP_DENORMALIZE fpp_denormalize;
FPP_A fpp_get_internal_overflow;
FPP_A fpp_get_internal_underflow;
FPP_A fpp_get_internal_round_all;
FPP_A fpp_get_internal_round;
FPP_A fpp_get_internal_round_exten;
FPP_A fpp_get_internal;
FPP_GET32 fpp_get_internal_grs;

FPP_A fpp_round_single;
FPP_A fpp_round_double;
FPP_A fpp_round32;
FPP_A fpp_round64;
FPP_AB fpp_int;
FPP_AB fpp_sinh;
FPP_AB fpp_intrz;
FPP_ABP fpp_sqrt;
FPP_AB fpp_lognp1;
FPP_AB fpp_etoxm1;
FPP_AB fpp_tanh;
FPP_AB fpp_atan;
FPP_AB fpp_atanh;
FPP_AB fpp_sin;
FPP_AB fpp_asin;
FPP_AB fpp_tan;
FPP_AB fpp_etox;
FPP_AB fpp_twotox;
FPP_AB fpp_tentox;
FPP_AB fpp_logn;
FPP_AB fpp_log10;
FPP_AB fpp_log2;
FPP_ABP fpp_abs;
FPP_AB fpp_cosh;
FPP_ABP fpp_neg;
FPP_AB fpp_acos;
FPP_AB fpp_cos;
FPP_ABC fpp_sincos;
FPP_AB fpp_getexp;
FPP_AB fpp_getman;
FPP_ABP fpp_div;
FPP_ABQS fpp_mod;
FPP_ABP fpp_add;
FPP_ABP fpp_mul;
FPP_ABQS fpp_rem;
FPP_AB fpp_scale;
FPP_ABP fpp_sub;
FPP_AB fpp_sgldiv;
FPP_AB fpp_sglmul;
FPP_AB fpp_cmp;
FPP_AB fpp_tst;
FPP_ABP fpp_move;

#define DEBUG_FPP 0
#define EXCEPTION_FPP 0

STATIC_INLINE int isinrom (void)
{
	return (munge24 (m68k_getpc ()) & 0xFFF80000) == 0xF80000 && !currprefs.mmu_model;
}

/* The FPU core (fpu_mode, FPU_MODE_*): exact (80-bit softfloat, with the
 * double-double transcendentals) or host floating point. Automatic picks the
 * exact core on AArch64, which has no extended-precision hardware, unless the
 * JIT compiles FPU instructions to host doubles (JIT with JIT FPU); x86-64's
 * host core is x87 80-bit already. Chosen at FPU reset and when the settings
 * change (fpu_modechange()); with the exact core the JIT compiles FPU
 * instructions as calls into it (jit_fpu_enabled() false). */
static bool fpu_exact_active;
static int fpu_core_active = -2;

bool fpu_softfloat_core(void)
{
	return fpu_exact_active;
}

bool jit_fpu_enabled(void)
{
	return currprefs.compfpu && !fpu_exact_active;
}

static bool jit_fpu(void)
{
	return currprefs.cachesize && jit_fpu_enabled();
}

// 1: the exact core, 0: host floating point, -1: host 80-bit (MSVC long double)
static int fpu_core_wanted(void)
{
	switch (currprefs.fpu_mode) {
	case FPU_MODE_EXACT:
		return 1;
	case FPU_MODE_HOST:
		return 0;
	case FPU_MODE_HOST80:
		return -1;
	default:
#if defined(CPU_AARCH64) && defined(WITH_SOFTFLOAT)
		return currprefs.cachesize && currprefs.compfpu ? 0 : 1;
#else
		return 0;
#endif
	}
}

static void fpu_select_core(void)
{
	int core = fpu_core_wanted();
#ifndef WITH_SOFTFLOAT
	if (core > 0)
		core = 0;
#endif
	fpu_core_active = core;
#ifdef MSVC_LONG_DOUBLE
	use_long_double = false;
	if (core < 0) {
		use_long_double = true;
		if (!fp_init_native_80()) {
			use_long_double = false;
			core = 1;
		}
	}
#endif
	fpu_exact_active = core > 0;
	if (core > 0) {
#ifdef WITH_SOFTFLOAT
		fp_init_softfloat_exact(currprefs.fpu_model);
#endif
	} else if (core == 0) {
		fp_init_native();
	}
}

static int warned = 100;

struct fpp_cr_entry {
	uae_u32 val[3];
	uae_u8 inexact;
	uae_s8 rndoff[4];
};

static const struct fpp_cr_entry fpp_cr[22] = {
	{ {0x40000000, 0xc90fdaa2, 0x2168c235}, 1, {0,-1,-1, 0} }, //  0 = pi
	{ {0x3ffd0000, 0x9a209a84, 0xfbcff798}, 1, {0, 0, 0, 1} }, //  1 = log10(2)
	{ {0x40000000, 0xadf85458, 0xa2bb4a9a}, 1, {0, 0, 0, 1} }, //  2 = e
	{ {0x3fff0000, 0xb8aa3b29, 0x5c17f0bc}, 1, {0,-1,-1, 0} }, //  3 = log2(e)
	{ {0x3ffd0000, 0xde5bd8a9, 0x37287195}, 0, {0, 0, 0, 0} }, //  4 = log10(e)
	{ {0x00000000, 0x00000000, 0x00000000}, 0, {0, 0, 0, 0} }, //  5 = 0.0
	{ {0x3ffe0000, 0xb17217f7, 0xd1cf79ac}, 1, {0,-1,-1, 0} }, //  6 = ln(2)
	{ {0x40000000, 0x935d8ddd, 0xaaa8ac17}, 1, {0,-1,-1, 0} }, //  7 = ln(10)
	{ {0x3fff0000, 0x80000000, 0x00000000}, 0, {0, 0, 0, 0} }, //  8 = 1e0
	{ {0x40020000, 0xa0000000, 0x00000000}, 0, {0, 0, 0, 0} }, //  9 = 1e1
	{ {0x40050000, 0xc8000000, 0x00000000}, 0, {0, 0, 0, 0} }, // 10 = 1e2
	{ {0x400c0000, 0x9c400000, 0x00000000}, 0, {0, 0, 0, 0} }, // 11 = 1e4
	{ {0x40190000, 0xbebc2000, 0x00000000}, 0, {0, 0, 0, 0} }, // 12 = 1e8
	{ {0x40340000, 0x8e1bc9bf, 0x04000000}, 0, {0, 0, 0, 0} }, // 13 = 1e16
	{ {0x40690000, 0x9dc5ada8, 0x2b70b59e}, 1, {0,-1,-1, 0} }, // 14 = 1e32
	{ {0x40d30000, 0xc2781f49, 0xffcfa6d5}, 1, {0, 0, 0, 1} }, // 15 = 1e64
	{ {0x41a80000, 0x93ba47c9, 0x80e98ce0}, 1, {0,-1,-1, 0} }, // 16 = 1e128
	{ {0x43510000, 0xaa7eebfb, 0x9df9de8e}, 1, {0,-1,-1, 0} }, // 17 = 1e256
	{ {0x46a30000, 0xe319a0ae, 0xa60e91c7}, 1, {0,-1,-1, 0} }, // 18 = 1e512
	{ {0x4d480000, 0xc9767586, 0x81750c17}, 1, {0, 0, 0, 1} }, // 19 = 1e1024
	{ {0x5a920000, 0x9e8b3b5d, 0xc53d5de5}, 1, {0,-1,-1, 0} }, // 20 = 1e2048
	{ {0x75250000, 0xc4605202, 0x8a20979b}, 1, {0,-1,-1, 0} }  // 21 = 1e4094
};

#define FPP_CR_PI       0
#define FPP_CR_LOG10_2  1
#define FPP_CR_E        2
#define FPP_CR_LOG2_E   3
#define FPP_CR_LOG10_E  4
#define FPP_CR_ZERO     5
#define FPP_CR_LN_2     6
#define FPP_CR_LN_10    7
#define FPP_CR_1E0      8
#define FPP_CR_1E1      9
#define FPP_CR_1E2      10
#define FPP_CR_1E4      11
#define FPP_CR_1E8      12
#define FPP_CR_1E16     13
#define FPP_CR_1E32     14
#define FPP_CR_1E64     15
#define FPP_CR_1E128    16
#define FPP_CR_1E256    17
#define FPP_CR_1E512    18
#define FPP_CR_1E1024   19
#define FPP_CR_1E2048   20
#define FPP_CR_1E4096   21

struct fpp_cr_entry_undef {
	uae_u32 val[3];
};

#define FPP_CR_NUM_SPECIAL_UNDEFINED 10

// 68881 and 68882 have identical undefined fields
static const struct fpp_cr_entry_undef fpp_cr_undef[] = {
	{ {0x40000000, 0x00000000, 0x00000000} },
	{ {0x40010000, 0xfe000682, 0x00000000} },
	{ {0x40010000, 0xffc00503, 0x80000000} },
	{ {0x20000000, 0x7fffffff, 0x00000000} },
	{ {0x00000000, 0xffffffff, 0xffffffff} },
	{ {0x3c000000, 0xffffffff, 0xfffff800} },
	{ {0x3f800000, 0xffffff00, 0x00000000} },
	{ {0x00010000, 0xf65d8d9c, 0x00000000} },
	{ {0x7fff0000, 0x001e0000, 0x00000000} },
	{ {0x43ff0000, 0x000e0000, 0x00000000} },
	{ {0x407f0000, 0x00060000, 0x00000000} }
};

uae_u32 xhex_nan[]   ={0x7fff0000, 0xffffffff, 0xffffffff};

static bool fpu_mmu_fixup;

/* Floating Point Control Register (FPCR)
 *
 * Exception Enable Byte
 * x--- ---- ---- ----  bit 15: BSUN (branch/set on unordered)
 * -x-- ---- ---- ----  bit 14: SNAN (signaling not a number)
 * --x- ---- ---- ----  bit 13: OPERR (operand error)
 * ---x ---- ---- ----  bit 12: OVFL (overflow)
 * ---- x--- ---- ----  bit 11: UNFL (underflow)
 * ---- -x-- ---- ----  bit 10: DZ (divide by zero)
 * ---- --x- ---- ----  bit 9: INEX 2 (inexact operation)
 * ---- ---x ---- ----  bit 8: INEX 1 (inexact decimal input)
 *
 * Mode Control Byte
 * ---- ---- xx-- ----  bits 7 and 6: PREC (rounding precision)
 * ---- ---- --xx ----  bits 5 and 4: RND (rounding mode)
 * ---- ---- ---- xxxx  bits 3 to 0: all 0
 */

#define FPCR_PREC   0x00C0
#define FPCR_RND    0x0030

/* Floating Point Status Register (FPSR)
 *
 * Condition Code Byte
 * xxxx ---- ---- ---- ---- ---- ---- ----  bits 31 to 28: all 0
 * ---- x--- ---- ---- ---- ---- ---- ----  bit 27: N (negative)
 * ---- -x-- ---- ---- ---- ---- ---- ----  bit 26: Z (zero)
 * ---- --x- ---- ---- ---- ---- ---- ----  bit 25: I (infinity)
 * ---- ---x ---- ---- ---- ---- ---- ----  bit 24: NAN (not a number or unordered)
 *
 * Quotient Byte (set and reset only by FMOD and FREM)
 * ---- ---- x--- ---- ---- ---- ---- ----  bit 23: sign of quotient
 * ---- ---- -xxx xxxx ---- ---- ---- ----  bits 22 to 16: 7 least significant bits of quotient
 *
 * Exception Status Byte
 * ---- ---- ---- ---- x--- ---- ---- ----  bit 15: BSUN (branch/set on unordered)
 * ---- ---- ---- ---- -x-- ---- ---- ----  bit 14: SNAN (signaling not a number)
 * ---- ---- ---- ---- --x- ---- ---- ----  bit 13: OPERR (operand error)
 * ---- ---- ---- ---- ---x ---- ---- ----  bit 12: OVFL (overflow)
 * ---- ---- ---- ---- ---- x--- ---- ----  bit 11: UNFL (underflow)
 * ---- ---- ---- ---- ---- -x-- ---- ----  bit 10: DZ (divide by zero)
 * ---- ---- ---- ---- ---- --x- ---- ----  bit 9: INEX 2 (inexact operation)
 * ---- ---- ---- ---- ---- ---x ---- ----  bit 8: INEX 1 (inexact decimal input)
 *
 * Accrued Exception Byte
 * ---- ---- ---- ---- ---- ---- x--- ----  bit 7: IOP (invalid operation)
 * ---- ---- ---- ---- ---- ---- -x-- ----  bit 6: OVFL (overflow)
 * ---- ---- ---- ---- ---- ---- --x- ----  bit 5: UNFL (underflow)
 * ---- ---- ---- ---- ---- ---- ---x ----  bit 4: DZ (divide by zero)
 * ---- ---- ---- ---- ---- ---- ---- x---  bit 3: INEX (inexact)
 * ---- ---- ---- ---- ---- ---- ---- -xxx  bits 2 to 0: all 0
 */

#define FPSR_ZEROBITS   0xF0000007

#define FPSR_CC_N       0x08000000
#define FPSR_CC_Z       0x04000000
#define FPSR_CC_I       0x02000000
#define FPSR_CC_NAN     0x01000000

#define FPSR_QUOT_SIGN  0x00800000
#define FPSR_QUOT_LSB   0x007F0000

#define FPSR_AE_IOP     0x00000080
#define FPSR_AE_OVFL    0x00000040
#define FPSR_AE_UNFL    0x00000020
#define FPSR_AE_DZ      0x00000010
#define FPSR_AE_INEX    0x00000008

static struct {
	// 6888x and 68060
	uae_u32 ccr;
	uae_u32 eo[3];
	// 68060
	uae_u32 v;
	// 68040
	uae_u32 fpiarcu;
	uae_u32 cmdreg3b;
	uae_u32 cmdreg1b;
	uae_u32 stag, dtag;
	uae_u32 e1, e3, t;
	uae_u32 fpt[3];
	uae_u32 et[3];
	uae_u32 wbt[3];
	uae_u32 grs;
	uae_u32 wbte15;
	uae_u32 wbtm66;
} fsave_data;

static void reset_fsave_data(void)
{
	int i;
	for (i = 0; i < 3; i++) {
		fsave_data.eo[i] = 0;
		fsave_data.fpt[i] = 0;
		fsave_data.et[i] = 0;
		fsave_data.wbt[i] = 0;
	}
	fsave_data.ccr = 0;
	fsave_data.v = 0;
	fsave_data.fpiarcu = 0;
	fsave_data.cmdreg1b = 0;
	fsave_data.cmdreg3b = 0;
	fsave_data.stag = 0;
	fsave_data.dtag = 0;
	fsave_data.e1 = 0;
	fsave_data.e3 = 0;
	fsave_data.t = 0;
	fsave_data.wbte15 = 0;
	fsave_data.wbtm66 = 0;
	fsave_data.grs = 0;
}

static uae_u32 get_ftag(fpdata *src, int size)
{
	fpp_is_init(src);
	if (fpp_is_zero(src)) {
		return 1; // ZERO
	} else if (fpp_is_denormal_or_unnormal(src)) {
		if (size == 1 || size == 5)
			return 5; // Single/double DENORMAL
		return 4; // Extended DENORMAL or UNNORMAL
	} else if  (fpp_is_nan(src)) {
		return 3; // NAN
	} else if (fpp_is_infinity(src)) {
		return 2; // INF
	}
	return 0; // NORMAL
}

STATIC_INLINE bool fp_is_dyadic(uae_u16 extra)
{
	return ((extra & 0x30) == 0x20 || (extra & 0x7f) == 0x38);
}

static bool fp_exception_pending_slow(bool pre)
{
	// first check for pending arithmetic exceptions
	if (support_exceptions) {
		if (regs.fp_exp_pend) {
			if (warned > 0) {
				write_log(_T("FPU ARITHMETIC EXCEPTION (%d) PC=%08x\n"), regs.fp_exp_pend, regs.instruction_pc);
			}
			regs.fpu_exp_pre = pre;
			regs.fp_exception = true;
			Exception(regs.fp_exp_pend);
			if (currprefs.fpu_model != 68882)
				regs.fp_exp_pend = 0;
			return true;
		}
	}
	// no arithmetic exceptions pending, check for unimplemented datatype
	if (regs.fp_unimp_pend) {
		if (warned > 0) {
			write_log(_T("FPU unimplemented datatype exception (%s) PC=%08x\n"), pre ? _T("pre") : _T("mid/post"), regs.instruction_pc);
		}
		if (currprefs.cpu_model == 68060 && fpu_mmu_fixup) {
			m68k_areg(regs, mmufixup[0].reg) = mmufixup[0].value;
			mmufixup[0].reg = -1;
		}
		regs.fpu_exp_pre = pre;
		regs.fp_exception = true;
		Exception(55);
		regs.fp_unimp_pend = 0;

		return true;
	}
	return false;
}

// the common case, nothing pending, without a call
static inline bool fp_exception_pending(bool pre)
{
	if (!((support_exceptions && regs.fp_exp_pend) || regs.fp_unimp_pend))
		return false;
	return fp_exception_pending_slow(pre);
}

static void fp_unimp_instruction_exception_pending(void)
{
	if (regs.fp_unimp_ins) {
		if (warned > 0) {
			write_log(_T("FPU UNIMPLEMENTED INSTRUCTION/FPU DISABLED EXCEPTION PC=%08x\n"), M68K_GETPC);
		}
		if (currprefs.cpu_model == 68060 && fpu_mmu_fixup) {
			m68k_areg(regs, mmufixup[0].reg) = mmufixup[0].value;
			mmufixup[0].reg = -1;
		}
		regs.fpu_exp_pre = true;
		regs.fp_exception = true;
		Exception(11);
		regs.fp_unimp_ins = false;
		regs.fp_unimp_pend = 0;
	}
}

void fpsr_set_exception(uae_u32 exception)
{
	regs.fpsr |= exception;
}

static uae_u32 fpsr_get_vector(uae_u32 exception)
{
	static const int vtable[8] = { 49, 49, 50, 51, 53, 52, 54, 48 };
	int i;
	exception >>= 8;
	for (i = 7; i >= 0; i--) {
		if (exception & (1 << i)) {
			return vtable[i];
		}
	}
	return 0;
}

static bool fpsr_check_arithmetic_exception_slow(uae_u32 mask, fpdata *src, uae_u32 opcode, uae_u16 extra, uae_u32 ea, bool easet, uaecptr oldpc)
{
	if (!support_exceptions)
		return false;

	uae_u32 exception;
	// Any exception status bit and matching exception enable bits set?
	exception = regs.fpsr & regs.fpcr & 0xff00;
	// Add 68040/68060 nonmaskable exceptions. Only if no unimplemented instruction emulation.
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
		exception |= regs.fpsr & (FPSR_OVFL | FPSR_UNFL | mask);
	}

	if (exception) {
		regs.fp_exp_pend = fpsr_get_vector(exception);
		bool nonmaskable = (regs.fp_exp_pend != fpsr_get_vector(regs.fpsr & regs.fpcr));
		if (warned > 0) {
			write_log(_T("FPU %s arithmetic exception pending: FPSR: %08x, FPCR: %04x (vector: %d) PC=%08x!\n"),
				nonmaskable ? _T("nonmaskable") : _T(""), regs.fpsr, regs.fpcr, regs.fp_exp_pend, regs.instruction_pc);
#if EXCEPTION_FPP == 0
			warned--;
#endif	
		}

		regs.fp_opword = opcode;
		regs.fp_ea = ea;
		regs.fp_ea_set = easet;
		if (oldpc != 0xffffffff) {
			regs.fpiar = oldpc;
		}

		// data for FSAVE stack frame
		fpdata eo;
		uae_u32 opclass = (extra >> 13) & 7;

		reset_fsave_data();

		if (currprefs.fpu_model == 68881 || currprefs.fpu_model == 68882) {
			// fsave data for 68881 and 68882

			if (opclass == 3) { // 011
				fsave_data.ccr = ((uae_u32)extra << 16) | extra;
			} else { // 000 or 010
				fsave_data.ccr = ((uae_u32)(opcode | 0x0080) << 16) | extra;
			}
			if (regs.fp_exp_pend == 54 || regs.fp_exp_pend == 52 || regs.fp_exp_pend == 50) { // SNAN, OPERR, DZ
				fpp_from_exten_fmovem(src, &fsave_data.eo[0], &fsave_data.eo[1], &fsave_data.eo[2]);
				if (regs.fp_exp_pend == 52 && opclass == 3) { // OPERR from move to integer or packed
					fsave_data.eo[0] &= 0x4fff0000;
					fsave_data.eo[1] = fsave_data.eo[2] = 0;
				}
			} else if (regs.fp_exp_pend == 53) { // OVFL
				fpp_get_internal_overflow(&eo);
				fpp_from_exten_fmovem(&eo, &fsave_data.eo[0], &fsave_data.eo[1], &fsave_data.eo[2]);
			} else if (regs.fp_exp_pend == 51) { // UNFL
				fpp_get_internal_underflow(&eo);
				fpp_from_exten_fmovem(&eo, &fsave_data.eo[0], &fsave_data.eo[1], &fsave_data.eo[2]);
			} // else INEX1, INEX2: do nothing

		} else if (currprefs.cpu_model == 68060) {
			// fsave data for 68060
			regs.fpu_exp_state = 2; // 68060 EXCP frame
			fsave_data.v = regs.fp_exp_pend & 7;
			fpp_from_exten_fmovem(src, &fsave_data.eo[0], &fsave_data.eo[1], &fsave_data.eo[2]);
		} else {
			// fsave data for 68040
			regs.fpu_exp_state = 1; // 68040 UNIMP frame

			uae_u32 reg = (extra >> 7) & 7;
			int size = (extra >> 10) & 7;

			fsave_data.fpiarcu = regs.fpiar;

			if (regs.fp_exp_pend == 54) { // SNAN (undocumented)
				fsave_data.wbte15 = 1;
				fsave_data.grs = 7;
			} else {
				fsave_data.grs = 1;
			}

			if (opclass == 3) { // OPCLASS 011
				fsave_data.cmdreg1b = extra;
				fsave_data.e1 = 1;
				fsave_data.t = 1;
				fsave_data.wbte15 = (regs.fp_exp_pend == 51 || regs.fp_exp_pend == 54) ? 1 : 0; // UNFL, SNAN

				fpp_is_init(src);
				if (fpp_is_snan(src)) {
					fpp_unset_snan(src);
				}
				fpp_from_exten_fmovem(src, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]);
				fsave_data.stag = get_ftag(src, -1);
			} else { // OPCLASS 000 and 010
				fsave_data.cmdreg1b = extra;
				fsave_data.e1 = 1;
				fsave_data.wbte15 = (regs.fp_exp_pend == 54) ? 1 : 0; // SNAN (undocumented)

				if (regs.fp_exp_pend == 51 || regs.fp_exp_pend == 53 || regs.fp_exp_pend == 49) { // UNFL, OVFL, INEX
					if ((extra & 0x30) == 0x20 || (extra & 0x3f) == 0x04) { // FADD, FSUB, FMUL, FDIV, FSQRT
						regs.fpu_exp_state = 2; // 68040 BUSY frame
						fsave_data.e3 = 1;
						fsave_data.e1 = 0;
						fsave_data.cmdreg3b = (extra & 0x3C3) | ((extra & 0x038)>>1) | ((extra & 0x004)<<3);
						if (regs.fp_exp_pend == 51) { // UNFL
							fpp_get_internal(&eo);
						} else { // OVFL, INEX
							fpp_get_internal_round(&eo);
						}
						fsave_data.grs = fpp_get_internal_grs();
						fpp_from_exten_fmovem(&eo, &fsave_data.wbt[0], &fsave_data.wbt[1], &fsave_data.wbt[2]);
						fsave_data.wbte15 = (regs.fp_exp_pend == 51) ? 1 : 0; // UNFL
						// src and dst is stored (undocumented)
						fpp_from_exten_fmovem(src, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]);
						fsave_data.stag = get_ftag(src, (opclass == 0) ? -1 : size);
						if (fp_is_dyadic(extra)) {
							fpp_from_exten_fmovem(&regs.fp[reg], &fsave_data.fpt[0], &fsave_data.fpt[1], &fsave_data.fpt[2]);
							fsave_data.dtag = get_ftag(&regs.fp[reg], -1);
						}
					} else { // FMOVE to register, FABS, FNEG
						fpp_get_internal_round_exten(&eo);
						fsave_data.grs = fpp_get_internal_grs();
						fpp_from_exten_fmovem(&eo, &fsave_data.fpt[0], &fsave_data.fpt[1], &fsave_data.fpt[2]);
						fpp_get_internal_round_all(&eo); // weird
						fpp_from_exten_fmovem(&eo, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]); // undocumented
						fsave_data.stag = get_ftag(src, (opclass == 0) ? -1 : size);
					}
				} else { // SNAN, OPERR, DZ
					fpp_from_exten_fmovem(src, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]);
					fsave_data.stag = get_ftag(src, (opclass == 0) ? -1 : size);
					if (fp_is_dyadic(extra)) {
						fpp_from_exten_fmovem(&regs.fp[reg], &fsave_data.fpt[0], &fsave_data.fpt[1], &fsave_data.fpt[2]);
						fsave_data.dtag = get_ftag(&regs.fp[reg], -1);
					}
				}
			}
		}
		return nonmaskable;
	}
	return false;
}

// the common case, no enabled exception, without a call
static inline bool fpsr_check_arithmetic_exception(uae_u32 mask, fpdata *src, uae_u32 opcode, uae_u16 extra, uae_u32 ea, bool easet, uaecptr oldpc)
{
	if (!support_exceptions)
		return false;
	uae_u32 exception = regs.fpsr & regs.fpcr & 0xff00;
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented)
		exception |= regs.fpsr & (FPSR_OVFL | FPSR_UNFL | mask);
	if (!exception)
		return false;
	return fpsr_check_arithmetic_exception_slow(mask, src, opcode, extra, ea, easet, oldpc);
}

// Flag that is always set immediately.
static void fpsr_set_result_always(fpdata *result)
{
	fpu_fpsr_cc_written = false;
#ifdef JIT
	regs.fp_result = *result;
#endif
	regs.fpsr &= 0x00fffff8; // clear cc
	fpp_is_init(result);
	if (fpp_is_neg(result)) {
		regs.fpsr |= FPSR_CC_N;
	}
}

// fpsr_set_result_always() and fpsr_set_result() together, with one call into the FPU core
static void fpsr_set_result_all(fpdata *result)
{
	fpu_fpsr_cc_written = false;
#ifdef JIT
	regs.fp_result = *result;
#endif
	regs.fpsr = (regs.fpsr & 0x00fffff8) | (fpp_get_cc(result) << 24);
}

// Flags that are set if instruction didn't generate exception.
static void fpsr_set_result(fpdata *result)
{
	// condition code byte
	if (fpp_is_nan(result)) {
		regs.fpsr |= FPSR_CC_NAN;
	} else if (fpp_is_zero(result)) {
		regs.fpsr |= FPSR_CC_Z;
	} else if (fpp_is_infinity(result)) {
		regs.fpsr |= FPSR_CC_I;
	}
}
static void fpsr_clear_status(void)
{
	// clear exception status byte only
	regs.fpsr &= 0x0fff00f8;
	
	// clear external status
	fpp_clear_status();
}

static void updateaccrued(void)
{
	// update accrued exception byte
	if (regs.fpsr & (FPSR_BSUN | FPSR_SNAN | FPSR_OPERR))
		regs.fpsr |= FPSR_AE_IOP;  // IOP = BSUN || SNAN || OPERR
	if ((regs.fpsr & FPSR_UNFL) && (regs.fpsr & FPSR_INEX2))
		regs.fpsr |= FPSR_AE_UNFL; // UNFL = UNFL && INEX2
	if (regs.fpsr & FPSR_DZ)
		regs.fpsr |= FPSR_AE_DZ;   // DZ = DZ
}

static uae_u32 fpsr_make_status(void)
{
	uae_u32 exception;

	// get external status
	fpp_get_status(&regs.fpsr);
	
	if (regs.fpsr & FPSR_OVFL)
		regs.fpsr |= FPSR_AE_OVFL; // OVFL = OVFL
	if (regs.fpsr & (FPSR_OVFL | FPSR_INEX2 | FPSR_INEX1))
		regs.fpsr |= FPSR_AE_INEX; // INEX = INEX1 || INEX2 || OVFL

	if (!support_exceptions) {
		updateaccrued();
		return 0;
	}

	// return exceptions that interrupt calculation
	exception = regs.fpsr & regs.fpcr & (FPSR_SNAN | FPSR_OPERR | FPSR_DZ);
	updateaccrued();
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
		exception |= regs.fpsr & (FPSR_OVFL | FPSR_UNFL);
	}
	return exception;
}

static int fpsr_set_bsun(void)
{
	regs.fpsr |= FPSR_BSUN;
	regs.fpsr |= FPSR_AE_IOP;
	
	if (regs.fpcr & FPSR_BSUN) {
		write_log(_T("FPU exception: BSUN! (FPSR: %08x, FPCR: %08x)\n"), regs.fpsr, regs.fpcr);
		if (support_exceptions) {
			regs.fp_exp_pend = fpsr_get_vector(FPSR_BSUN);
			fp_exception_pending(true);
			return 1;
		}
	}
	return 0;
}

static void fpsr_set_quotient(uae_u64 quot, uae_u8 sign)
{
	regs.fpsr &= 0x0f00fff8;
	regs.fpsr |= (quot << 16) & FPSR_QUOT_LSB;
	regs.fpsr |= sign ? FPSR_QUOT_SIGN : 0;
}
static void fpsr_get_quotient(uae_u64 *quot, uae_u8 *sign)
{
	*quot = (regs.fpsr & FPSR_QUOT_LSB) >> 16;
	*sign = (regs.fpsr & FPSR_QUOT_SIGN) ? 1 : 0;
}

uae_u32 fpp_get_fpsr (void)
{
#ifdef JIT
	if (jit_fpu() && !fpu_fpsr_cc_written) {
		regs.fpsr &= 0x00fffff8; // clear cc
		fpp_is_init(&regs.fp_result);
		if (fpp_is_nan(&regs.fp_result)) {
			regs.fpsr |= FPSR_CC_NAN;
		} else if (fpp_is_zero(&regs.fp_result)) {
			regs.fpsr |= FPSR_CC_Z;
		} else if (fpp_is_infinity(&regs.fp_result)) {
			regs.fpsr |= FPSR_CC_I;
		}
		if (fpp_is_neg(&regs.fp_result))
			regs.fpsr |= FPSR_CC_N;
	}
#endif
	return regs.fpsr & fpsr_mask;
}

uae_u32 fpp_get_fpcr(void)
{
	return regs.fpcr & fpcr_mask;
}

void fpp_set_fpcr (uae_u32 val)
{
	fpp_set_mode(val);
	regs.fpcr = val & fpcr_mask;
}

static void fpnan (fpdata *fpd)
{
	fpp_to_exten(fpd, xhex_nan[0], xhex_nan[1], xhex_nan[2]);
}

static void fpclear (fpdata *fpd)
{
	fpp_from_int(fpd, 0);
}
static void fpset (fpdata *fpd, uae_s32 val)
{
	fpp_from_int(fpd, val);
}

void fpp_set_fpsr (uae_u32 val)
{
	regs.fpsr = val & fpsr_mask;
#if defined(CPU_x86_64) || defined(CPU_AARCH64)
	// FP_RESULT cannot represent independent N, Z, I and NAN bits: the JIT
	// predicates (x86, ARM64) take them from the FPSR until the next result.
	fpu_fpsr_cc_written = true;
#endif

#ifdef JIT
	// check comment in fpp_cond
	if (jit_fpu()) {
		// fp_result must give back every condition code combination
		uae_u32 sign = (val & FPSR_CC_N) ? 0x80000000 : 0;
		if (val & FPSR_CC_NAN) {
			fpp_to_exten(&regs.fp_result, sign | xhex_nan[0], xhex_nan[1], xhex_nan[2]);
		} else if (val & FPSR_CC_Z) {
			fpp_to_exten(&regs.fp_result, sign, 0, 0);
		} else if (val & FPSR_CC_I) {
			// FP_RESULT is also loaded directly by x87 predicate emitters.
			fpp_to_exten(&regs.fp_result, sign | 0x7fff0000, 0x80000000, 0);
		} else {
			fpset(&regs.fp_result, sign ? -1 : 1);
		}
	}
#endif
}

static void maybe_set_fpiar(uaecptr oldpc)
{
	// if any exception (except BSUN) is enabled: update FPIAR
	// 68040 or 68060: always update FPIAR
	if ((regs.fpcr & 0x00007f00) || currprefs.fpu_model == 68040 || currprefs.fpu_model == 68060) {
		regs.fpiar = oldpc;
	}
}

void fpp_set_fpiar(uae_u32 val)
{
	regs.fpiar = val;
}

uae_u32 fpp_get_fpiar(void)
{
	return regs.fpiar;
}

bool fpu_get_constant(fpdata *fpd, int cr)
{
	uae_u32 f[3] = { 0, 0, 0 };
	int entry = 0;
	bool round = true;
	int mode = (regs.fpcr >> 4) & 3;
	int prec = (regs.fpcr >> 6) & 3;
	
	switch (cr)
	{
		case 0x00: // pi
			entry = FPP_CR_PI;
			break;
		case 0x0b: // log10(2)
			entry = FPP_CR_LOG10_2;
			break;
		case 0x0c: // e
			entry = FPP_CR_E;
			break;
		case 0x0d: // log2(e)
			entry = FPP_CR_LOG2_E;
			break;
		case 0x0e: // log10(e)
			entry = FPP_CR_LOG10_E;
			break;
		case 0x0f: // 0.0
			entry = FPP_CR_ZERO;
			break;
		case 0x30: // ln(2)
			entry = FPP_CR_LN_2;
			break;
		case 0x31: // ln(10)
			entry = FPP_CR_LN_10;
			break;
		case 0x32: // 1e0
			entry = FPP_CR_1E0;
			break;
		case 0x33: // 1e1
			entry = FPP_CR_1E1;
			break;
		case 0x34: // 1e2
			entry = FPP_CR_1E2;
			break;
		case 0x35: // 1e4
			entry = FPP_CR_1E4;
			break;
		case 0x36: // 1e8
			entry = FPP_CR_1E8;
			break;
		case 0x37: // 1e16
			entry = FPP_CR_1E16;
			break;
		case 0x38: // 1e32
			entry = FPP_CR_1E32;
			break;
		case 0x39: // 1e64
			entry = FPP_CR_1E64;
			break;
		case 0x3a: // 1e128
			entry = FPP_CR_1E128;
			break;
		case 0x3b: // 1e256
			entry = FPP_CR_1E256;
			break;
		case 0x3c: // 1e512
			entry = FPP_CR_1E512;
			break;
		case 0x3d: // 1e1024
			entry = FPP_CR_1E1024;
			break;
		case 0x3e: // 1e2048
			entry = FPP_CR_1E2048;
			break;
		case 0x3f: // 1e4096
			entry = FPP_CR_1E4096;
			break;
		default: // undefined
		{
			bool check_f1_adjust = false;
			int f1_adjust = 0;
			uae_u32 sr = 0;

			if (cr > FPP_CR_NUM_SPECIAL_UNDEFINED) {
				cr = 0; // Most undefined fields contain this
			}
			f[0] = fpp_cr_undef[cr].val[0];
			f[1] = fpp_cr_undef[cr].val[1];
			f[2] = fpp_cr_undef[cr].val[2];
			// Rounding mode and precision works very strangely here..
			switch (cr)
			{
				case 1:
				check_f1_adjust = true;
				break;
				case 2:
				if (prec == 1 && mode == 3)
					f1_adjust = -1;
				break;
				case 3:
				if (prec == 1 && (mode == 0 || mode == 3))
					sr |= FPSR_CC_I;
				else
					sr |= FPSR_CC_NAN;
				break;
				case 7:
				sr |= FPSR_CC_NAN;
				check_f1_adjust = true;
				break;
			}
			if (check_f1_adjust) {
				if (prec == 1) {
					if (mode == 0) {
						f1_adjust = -1;
					} else if (mode == 1 || mode == 2) {
						f1_adjust = 1;
					}
				}
			}
			fpp_to_exten_fmovem(fpd, f[0], f[1], f[2]);
			if (prec == 1)
				fpp_round32(fpd);
			if (prec >= 2)
				fpp_round64(fpd);

			if (f1_adjust) {
				fpp_from_exten_fmovem(fpd, &f[0], &f[1], &f[2]);
				f[1] += f1_adjust * 0x80;
				fpp_to_exten_fmovem(fpd, f[0], f[1], f[2]);
			}

			fpsr_set_result_all(fpd);
			regs.fpsr |= sr;
			return false;
		}
	}

	f[0] = fpp_cr[entry].val[0];
	f[1] = fpp_cr[entry].val[1];
	f[2] = fpp_cr[entry].val[2];
	// if constant is inexact, set inexact bit and round
	// note: with valid constants, LSB never wraps
	if (fpp_cr[entry].inexact) {
		fpsr_set_exception(FPSR_INEX2);
		f[2] += fpp_cr[entry].rndoff[mode];
	}

	fpp_to_exten_fmovem(fpd, f[0], f[1], f[2]);
	
	if (prec == 1)
		fpp_round32(fpd);
	if (prec >= 2)
		fpp_round64(fpd);
	
	fpsr_set_result_all(fpd);

	return true;
}

#if 0
static void fpu_format_error (void)
{
	uaecptr newpc;
	regs.t0 = regs.t1 = 0;
	MakeSR ();
	if (!regs.s) {
		regs.usp = m68k_areg (regs, 7);
		m68k_areg (regs, 7) = regs.isp;
	}
	regs.s = 1;
	m68k_areg (regs, 7) -= 2;
	x_cp_put_long (m68k_areg (regs, 7), 0x0000 + 14 * 4);
	m68k_areg (regs, 7) -= 4;
	x_cp_put_long (m68k_areg (regs, 7), m68k_getpc ());
	m68k_areg (regs, 7) -= 2;
	x_cp_put_long (m68k_areg (regs, 7), regs.sr);
	newpc = x_cp_get_long (regs.vbr + 14 * 4);
	m68k_setpc (newpc);
#ifdef JIT
	set_special (SPCFLAG_END_COMPILE);
#endif
	regs.fp_exception = true;
}
#endif

static void fp_unimp_instruction(uae_u16 opcode, uae_u16 extra, uae_u32 ea, bool easet, uaecptr oldpc, fpdata *src, int reg, int size)
{
	if ((extra & 0x7f) == 4) // FSQRT 4->5
		extra |= 1;

	// data for fsave stack frame
	regs.fpu_exp_state = 1; // 68060 IDLE frame, 68040 UNIMP frame
	regs.fpiar = oldpc;

	if (currprefs.cpu_model == 68060) {
		// fsave data for 68060
		reset_fsave_data();
	} else if(currprefs.cpu_model == 68040) {
		// fsave data for 68040
		fsave_data.fpiarcu = regs.fpiar;

		if (regs.fp_unimp_pend == 0) { // else data has been saved by fp_unimp_datatype
			reset_fsave_data();
			fsave_data.cmdreg3b = (extra & 0x3C3) | ((extra & 0x038) >> 1) | ((extra & 0x004) << 3);
			fsave_data.cmdreg1b = extra;
			fpp_from_exten_fmovem(src, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]);
			fsave_data.stag = get_ftag(src, size);
			if (reg >= 0) {
				fpp_from_exten_fmovem(&regs.fp[reg], &fsave_data.fpt[0], &fsave_data.fpt[1], &fsave_data.fpt[2]);
				fsave_data.dtag = get_ftag(&regs.fp[reg], -1);
			}
		}
	}
	if (warned > 0) {
		write_log(_T("FPU unimplemented instruction: OP=%04X-%04X SRC=%08X-%08X-%08X EA=%08X PC=%08X\n"),
			opcode, extra, fsave_data.et[0],fsave_data.et[1],fsave_data.et[2], ea, oldpc);
 #if EXCEPTION_FPP == 0
		warned--;
 #endif
	}
	
	regs.fp_ea = ea;
	regs.fp_ea_set = easet;
	regs.fp_unimp_ins = true;
	fp_unimp_instruction_exception_pending();

	regs.fp_exception = true;

}

static void fp_unimp_datatype(uae_u16 opcode, uae_u16 extra, uae_u32 ea, bool easet, uaecptr oldpc, fpdata *src, uae_u32 *packed, bool predenormal)
{
	uae_u32 reg = (extra >> 7) & 7;
	uae_u32 size = (extra >> 10) & 7;
	uae_u32 opclass = (extra >> 13) & 7;

	regs.fp_opword = opcode;
	regs.fp_ea = ea;
	regs.fp_ea_set = easet;
	regs.fp_unimp_pend = packed ? 2 : (predenormal ? 3 : 1);
	regs.fpiar = oldpc;

	if((extra & 0x7f) == 4) // FSQRT 4->5
		extra |= 1;

	// data for fsave stack frame
	reset_fsave_data();
	regs.fpu_exp_state = 2; // 68060 EXCP frame, 68040 BUSY frame

	if (currprefs.cpu_model == 68060) {
		// fsave data for 68060
		if (packed) {
			regs.fpu_exp_state = 1; // 68060 IDLE frame
		} else {
			fsave_data.v = 7; // vector & 0x7
			fpp_from_exten_fmovem(src, &fsave_data.eo[0], &fsave_data.eo[1], &fsave_data.eo[2]);
		}
	} else if (currprefs.cpu_model == 68040) {
		// fsave data for 68040
		fsave_data.cmdreg1b = extra;
		fsave_data.fpiarcu = regs.fpiar;
		if (packed) {
			fsave_data.e1 = 1; // used to distinguish packed operands
		}
		if (opclass == 3) { // OPCLASS 011
			fsave_data.t = 1;
			fpp_from_exten_fmovem(src, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]);
			fsave_data.stag = get_ftag(src, -1);
			fpp_from_exten_fmovem(src, &fsave_data.fpt[0], &fsave_data.fpt[1], &fsave_data.fpt[2]); // undocumented
			fsave_data.dtag = get_ftag(src, -1); // undocumented
		} else { // OPCLASS 000 and 010
			if (packed) {
				fsave_data.fpt[2] = packed[0]; // yes, this is correct.
				fsave_data.fpt[1] = packed[1]; // undocumented
				fsave_data.et[1] = packed[1];
				fsave_data.et[2] = packed[2];
				fsave_data.stag = 7; // undocumented
			} else {
				fpp_from_exten_fmovem(src, &fsave_data.et[0], &fsave_data.et[1], &fsave_data.et[2]);
				fsave_data.stag = get_ftag(src, (opclass == 0) ? 0xffffffff : size);
				if (fsave_data.stag == 5) {
					fsave_data.et[0] = (size == 1) ? 0x3f800000 : 0x3c000000; // exponent for denormalized single and double
				}
				if (fp_is_dyadic(extra)) {
					fpp_from_exten_fmovem(&regs.fp[reg], &fsave_data.fpt[0], &fsave_data.fpt[1], &fsave_data.fpt[2]);
					fsave_data.dtag = get_ftag(&regs.fp[reg], -1);
				}
			}
		}
	}
	if (warned > 0) {
		write_log(_T("FPU unimplemented datatype (%s): OP=%04X-%04X SRC=%08X-%08X-%08X EA=%08X PC=%08X\n"),
			packed ? _T("packed") : _T("denormal"), opcode, extra,
			packed ? fsave_data.fpt[2] : fsave_data.et[0], fsave_data.et[1], fsave_data.et[2], ea, oldpc);
#if EXCEPTION_FPP == 0
		warned--;
#endif
	}
	regs.fp_exception = true;
}

static void fpu_op_illg(uae_u16 opcode, uae_u32 ea, bool easet, uaecptr oldpc)
{
	if ((currprefs.cpu_model == 68060 && (currprefs.fpu_model == 0 || (regs.pcr & 2)))
		|| (currprefs.cpu_model == 68040 && currprefs.fpu_model == 0)) {
			regs.fp_unimp_ins  = true;
			regs.fp_ea = ea;
			regs.fp_ea_set = easet;
			regs.fpiar = oldpc;
			fp_unimp_instruction_exception_pending();
			return;
	}
	regs.fp_exception = true;
	m68k_setpc(oldpc);
	op_illg(opcode);
}

static void fpu_noinst(uae_u16 opcode, uaecptr pc)
{
#if EXCEPTION_FPP
	write_log(_T("Unknown FPU instruction %04X %08X\n"), opcode, pc);
#endif
	regs.fp_exception = true;
	m68k_setpc(pc);
	op_illg(opcode);
}

static inline bool if_no_fpu(void)
{
	return (regs.pcr & 2) || currprefs.fpu_model <= 0;
}

static bool fault_no_fpu(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc);
static inline bool fault_if_no_fpu(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc)
{
	return if_no_fpu() && fault_no_fpu(opcode, extra, ea, easet, oldpc);
}
static bool fault_no_fpu(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc)
{
	{
#if EXCEPTION_FPP
		write_log(_T("no FPU: %04X-%04X PC=%08X\n"), opcode, extra, oldpc);
#endif
		if (fpu_mmu_fixup) {
			m68k_areg (regs, mmufixup[0].reg) = mmufixup[0].value;
			mmufixup[0].reg = -1;
			fpu_mmu_fixup = false;
		}
		fpu_op_illg(opcode, ea, easet, oldpc);
		return true;
	}
	return false;
}

static bool fault_if_nonexisting_opmode(uae_u16 opcode, uae_u16 extra, uaecptr oldpc)
{
	uae_u16 v = extra & 0x7f;

	// if non-existing FPU instruction (opmode): exit immediately and generate normal frame 0 exception 11.

	if (currprefs.fpu_model == 68881 || currprefs.fpu_model == 68882) {
		if (currprefs.fpu_no_unimplemented) {
			if (v >= 0x40) {
				fpu_noinst(opcode, oldpc);
				return true;
			}
			return false;
		}
		// 6888x undocumented but existing opmodes
		switch (v)
		{
		case 0x05:
		case 0x07:
		case 0x0b:
		case 0x13:
		case 0x17:
		case 0x1b:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
		case 0x39:
		case 0x3b:
		case 0x3c:
		case 0x3d:
		case 0x3e:
		case 0x3f:
			return false;
		}
	}

	switch (v)
	{
		case 0x05:
		case 0x07:
		case 0x0b:
		case 0x13:
		case 0x17:
		case 0x1b:
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
		case 0x39:
		case 0x3b:
		case 0x3c:
		case 0x3d:
		case 0x3e:
		case 0x3f:
		case 0x42:
		case 0x43:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x59:
		case 0x5b:
		case 0x5d:
		case 0x5f:
		case 0x61:
		case 0x65:
		case 0x69:
		case 0x6a:
		case 0x6b:
		case 0x6d:
		case 0x6e:
		case 0x6f:
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
			fpu_noinst(opcode, oldpc);
			return true;
		case 0x78:
		case 0x79:
		case 0x7a:
		case 0x7b:
		case 0x7c:
		case 0x7d:
		case 0x7e:
		case 0x7f:
			// Unexpected, isn't it?!
			Exception(4);
			return true;
		break;
	}
	return false;
}

static bool fault_if_unimplemented_680x0_slow (uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc, fpdata *src, int reg)
{
	if (fault_if_no_fpu (opcode, extra, ea, easet, oldpc))
		return true;
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
		if ((extra & (0x8000 | 0x2000)) != 0)
			return false;
		if ((extra & 0xfc00) == 0x5c00) {
			// FMOVECR
			fp_unimp_instruction(opcode, extra, ea, easet, oldpc, src, reg, -1);
			return true;
		}
		uae_u16 v = extra & 0x7f;
		/* >=0x040 are 68040/68060 only variants. 6888x = F-line exception. */
		switch (v)
		{
			case 0x00: /* FMOVE */
			case 0x04: /* FSQRT */
			case 0x18: /* FABS */
			case 0x1a: /* FNEG */
			case 0x20: /* FDIV */
			case 0x22: /* FADD */
			case 0x23: /* FMUL */
			case 0x24: /* FSGLDIV */
			case 0x27: /* FSGLMUL */
			case 0x28: /* FSUB */
			case 0x38: /* FCMP */
			case 0x3a: /* FTST */
			case 0x40: /* FSMOVE */
			case 0x41: /* FSSQRT */
			case 0x44: /* FDMOVE */
			case 0x45: /* FDSQRT */
			case 0x58: /* FSABS */
			case 0x5c: /* FDABS */
			case 0x5a: /* FSNEG */
			case 0x5e: /* FDNEG */
			case 0x60: /* FSDIV */
			case 0x62: /* FSADD */
			case 0x63: /* FSMUL */
			case 0x64: /* FDDIV */
			case 0x66: /* FDADD */
			case 0x67: /* FDMUL */
			case 0x68: /* FSSUB */
			case 0x6c: /* FDSUB */
				return false;
			case 0x01: /* FINT */
			case 0x03: /* FINTRZ */
			// Unimplemented only in 68040.
			if(currprefs.cpu_model != 68040) {
				return false;
			}
			default:
			fp_unimp_instruction(opcode, extra, ea, easet, oldpc, src, reg, -1);
			return true;
		}
	}
	return false;
}

// 6888x, or 68040/68060 with the unimplemented instructions emulated: only the no-FPU check
static inline bool fault_if_unimplemented_680x0 (uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc, fpdata *src, int reg)
{
	if (!(currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented))
		return fault_if_no_fpu (opcode, extra, ea, easet, oldpc);
	return fault_if_unimplemented_680x0_slow (opcode, extra, ea, easet, oldpc, src, reg);
}

static bool fault_if_unimplemented_6888x (uae_u16 opcode, uae_u16 extra, uaecptr oldpc)
{
	if ((currprefs.fpu_model == 68881 || currprefs.fpu_model == 68882) && currprefs.fpu_no_unimplemented) {
		uae_u16 v = extra & 0x7f;
		switch(v)
		{
			case 0x00: /* FMOVE */
			case 0x01: /* FINT */
			case 0x02: /* FSINH */
			case 0x03: /* FINTRZ */
			case 0x04: /* FSQRT */
			case 0x06: /* FLOGNP1 */
			case 0x08: /* FETOXM1 */
			case 0x09: /* FTANH */
			case 0x0a: /* FATAN */
			case 0x0c: /* FASIN */
			case 0x0d: /* FATANH */
			case 0x0e: /* FSIN */
			case 0x0f: /* FTAN */
			case 0x10: /* FETOX */
			case 0x11: /* FTWOTOX */
			case 0x12: /* FTENTOX */
			case 0x14: /* FLOGN */
			case 0x15: /* FLOG10 */
			case 0x16: /* FLOG2 */
			case 0x18: /* FABS */
			case 0x19: /* FCOSH */
			case 0x1a: /* FNEG */
			case 0x1c: /* FACOS */
			case 0x1d: /* FCOS */
			case 0x1e: /* FGETEXP */
			case 0x1f: /* FGETMAN */
			case 0x20: /* FDIV */
			case 0x21: /* FMOD */
			case 0x22: /* FADD */
			case 0x23: /* FMUL */
			case 0x24: /* FSGLDIV */
			case 0x25: /* FREM */
			case 0x26: /* FSCALE */
			case 0x27: /* FSGLMUL */
			case 0x28: /* FSUB */
			case 0x30: /* FSINCOS */
			case 0x31: /* FSINCOS */
			case 0x32: /* FSINCOS */
			case 0x33: /* FSINCOS */
			case 0x34: /* FSINCOS */
			case 0x35: /* FSINCOS */
			case 0x36: /* FSINCOS */
			case 0x37: /* FSINCOS */
			case 0x38: /* FCMP */
			case 0x3a: /* FTST */

			// 6888x invalid opmodes execute existing FPU instruction.
			// Only opmodes 0x40-0x7f generate F-line exception.
			case 0x05:
			case 0x07:
			case 0x0b:
			case 0x13:
			case 0x17:
			case 0x1b:
			case 0x29:
			case 0x2a:
			case 0x2b:
			case 0x2c:
			case 0x2d:
			case 0x2e:
			case 0x2f:
			case 0x39:
			case 0x3b:
			case 0x3c:
			case 0x3d:
			case 0x3e:
			case 0x3f:
				return false;
			default:
				fpu_noinst (opcode, oldpc);
				return true;
		}
	}
	return false;
}

static bool fault_if_60 (void)
{
	if (currprefs.cpu_model == 68060 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
		Exception(60);
		return true;
	}
	return false;
}

static bool fault_if_no_fpu_u (uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc)
{
	if (fault_if_no_fpu (opcode, extra, ea, easet, oldpc))
		return true;
	if (currprefs.cpu_model == 68060 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
		// 68060 FTRAPcc, FDBcc and FScc are not implemented.
		regs.fp_unimp_ins = true;
		regs.fp_ea = ea;
		regs.fp_ea_set = easet;
		regs.fpiar = oldpc;
		fp_unimp_instruction_exception_pending();
		return true;
	}
	return false;
}

static bool fault_if_no_6888x (uae_u16 opcode, uae_u16 extra, uaecptr oldpc)
{
	if (currprefs.cpu_model < 68040 && currprefs.fpu_model <= 0) {
#if EXCEPTION_FPP
		write_log(_T("6888x no FPU: %04X-%04X PC=%08X\n"), opcode, extra, oldpc);
#endif
		m68k_setpc (oldpc);
		regs.fp_exception = true;
		op_illg (opcode);
		return true;
	}
	return false;
}

static int get_fpu_version (int model)
{
	int v = 0;

	switch (model)
	{
	case 68881:
	case 68882:
		v = 0x1f;
		break;
	case 68040:
		if (currprefs.fpu_revision == 0x40)
			v = 0x40;
		else
			v = 0x41;
		break;
	}
	return v;
}

static void fpu_null (void)
{
	regs.fpu_state = 0;
	regs.fpu_exp_state = 0;
	regs.fpcr = 0;
	fpp_set_fpsr(0);
	regs.fpiar = 0;
	for (int i = 0; i < 8; i++)
		fpnan (&regs.fp[i]);
}

// 68040/060 does not support denormals
static bool normalize_or_fault_if_no_denormal_support(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc, fpdata *src)
{
	if (!support_denormals)
		return false;
	fpp_is_init(src);
	if (fpp_is_denormal_or_unnormal(src)) {
		if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
			if (fpp_is_zero(src)) {
				fpp_normalize(src); // 68040/060 can only fix unnormal zeros
			} else {
				fp_unimp_datatype(opcode, extra, ea, easet, oldpc, src, NULL, true);
				return true;
			}
		} else {
			fpp_normalize(src);
		}
	}
	return false;
}
static bool normalize_or_fault_if_no_denormal_support_dst(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc, fpdata *dst, fpdata *src)
{
	if (!support_denormals)
		return false;
	fpp_is_init(dst);
	if (fpp_is_denormal_or_unnormal(dst)) {
		if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
			if (fpp_is_zero(dst)) {
				fpp_normalize(dst); // 68040/060 can only fix unnormal zeros
			} else {
				fp_unimp_datatype(opcode, extra, ea, easet, oldpc, src, NULL, false);
				return true;
			}
		} else {
			fpp_normalize(dst);
		}
	}
	return false;
}

// 68040/060 does not support packed decimal format
static bool fault_if_no_packed_support(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc, fpdata *src, uae_u32 *packed)
{
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_model && currprefs.fpu_no_unimplemented) {
		fp_unimp_datatype(opcode, extra, ea, easet, oldpc, src, packed, false);
		return true;
	}
	return false;
 }

// 68040 does not support move to integer format
static bool fault_if_68040_integer_nonmaskable(uae_u16 opcode, uae_u16 extra, uaecptr ea, bool easet, uaecptr oldpc, fpdata *src)
{
	if (currprefs.cpu_model == 68040 && currprefs.fpu_model && fpu_softfloat_core()) {
		fpsr_make_status();
		if (regs.fpsr & (FPSR_SNAN | FPSR_OPERR)) {
			fpsr_check_arithmetic_exception(FPSR_SNAN | FPSR_OPERR, src, opcode, extra, ea, easet, oldpc);
			fp_exception_pending(false); // post
			return true;
		}
	}
	return false;
}

static int get_fp_value(uae_u32 opcode, uae_u16 extra, fpdata *src, uaecptr oldpc, uaecptr *adp, bool *adsetp, const uaecptr *jit_ea = nullptr)
{
	int size, mode, reg;
	uae_u32 ad = 0;
	bool adset = false;
	static const int sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
	static const int sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };
	uae_u32 exts[3];
	int doext = 0;

	if (!(extra & 0x4000)) {
		// FPx to FPx
		if (fault_if_no_fpu (opcode, extra, 0, false, oldpc))
			return -1;
		*src = regs.fp[(extra >> 10) & 7];
		normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, oldpc, src);
		return 1;
	}
	mode = (opcode >> 3) & 7;
	reg = opcode & 7;
	size = (extra >> 10) & 7;

	switch (mode) {
		case 0: // Dn
			if ((size == 0 || size == 1 ||size == 4 || size == 6) && fault_if_no_fpu (opcode, extra, 0, false, oldpc))
				return -1;
			switch (size)
			{
				case 6: // B
					fpset(src, (uae_s8) m68k_dreg (regs, reg));
					break;
				case 4: // W
					fpset(src, (uae_s16) m68k_dreg (regs, reg));
					break;
				case 0: // L
					fpset(src, (uae_s32) m68k_dreg (regs, reg));
					break;
				case 1: // S
					fpp_to_single (src, m68k_dreg (regs, reg));
					normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, oldpc, src);
					break;
				case 3: // P
					if (currprefs.cpu_model == 68060) {
						uae_u32 wrd[3];
						if (fault_if_no_packed_support(opcode, extra, 0, false, oldpc, NULL, wrd))
							return 1;
					}
					return 0;
				default:
					if (currprefs.cpu_model >= 68040) {
						if (fault_if_unimplemented_680x0(opcode, extra, ad, adset, oldpc, src, reg))
							return -1;
						regs.fpiar = oldpc;
					}
					return 0;
			}
			return 1;
		case 1: // An
			if (currprefs.cpu_model >= 68040) {
				if (fault_if_unimplemented_680x0(opcode, extra, ad, adset, oldpc, src, reg))
					return -1;
			}
			return 0;
		case 2: // (An)
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg);
			adset = true;
			break;
		case 3: // (An)+
			// Also needed by fault_if_no_fpu 
			mmufixup[0].reg = reg;
			mmufixup[0].value = m68k_areg (regs, reg);
			fpu_mmu_fixup = true;
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg);
			adset = true;
			m68k_areg (regs, reg) += reg == 7 ? sz2[size] : sz1[size];
			break;
		case 4: // -(An)
			// Also needed by fault_if_no_fpu 
			mmufixup[0].reg = reg;
			mmufixup[0].value = m68k_areg (regs, reg);
			fpu_mmu_fixup = true;
			m68k_areg (regs, reg) -= reg == 7 ? sz2[size] : sz1[size];
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg);
			adset = true;
			// 68060 no fpu -(an): EA points to -4, not -12 if extended precision
			// or unsupported packed datatype.
			if (currprefs.cpu_model == 68060 && ((if_no_fpu() && sz1[size] == 12) || size == 3)) {
				ad += 8;
			}
			break;
		case 5: // (d16,An)
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg) + (uae_s32) (uae_s16) x_cp_next_iword ();
			adset = true;
			break;
		case 6: // (d8,An,Xn)+
			ad = jit_ea ? *jit_ea : x_cp_get_disp_ea_020 (m68k_areg (regs, reg), 0);
			adset = true;
			break;
		case 7:
			switch (reg)
			{
				case 0: // (xxx).W
					ad = jit_ea ? *jit_ea : (uae_s32) (uae_s16) x_cp_next_iword ();
					adset = true;
					break;
				case 1: // (xxx).L
					ad = jit_ea ? *jit_ea : x_cp_next_ilong ();
					adset = true;
					break;
				case 2: // (d16,PC)
					if (jit_ea) {
						ad = *jit_ea;
					} else {
						ad = m68k_getpc ();
						ad += (uae_s32) (uae_s16) x_cp_next_iword ();
					}
					adset = true;
					break;
				case 3: // (d8,PC,Xn)+
					ad = jit_ea ? *jit_ea : x_cp_get_disp_ea_020 (m68k_getpc (), 0);
					adset = true;
					break;
				case 4: // #imm
					if (jit_ea) {
						if ((size == 2 || size == 3) && fault_if_60())
							return -1;
						ad = *jit_ea;
						adset = true;
						break;
					}
					doext = 1;
					switch (size)
					{
						case 0: // L
						case 1: // S
						exts[0] = x_cp_next_ilong ();
						break;
						case 2: // X
						case 3: // P
						// 68060 and immediate X or P: unimplemented effective address
						if (fault_if_60())
							return -1;
						exts[0] = x_cp_next_ilong ();
						exts[1] = x_cp_next_ilong ();
						exts[2] = x_cp_next_ilong ();
						break;
						case 4: // W
						exts[0] = x_cp_next_iword ();
						break;
						case 5: // D
						exts[0] = x_cp_next_ilong ();
						exts[1] = x_cp_next_ilong ();
						break;
						case 6: // B
						exts[0] = x_cp_next_iword ();
						break;
					}
					break;
				default:
					return 0;
			}
	}


	if (fault_if_no_fpu (opcode, extra, ad, adset, oldpc))
		return -1;

	*adp = ad;
	*adsetp = adset;
	uae_u32 adold = ad;

	if (currprefs.fpu_model == 68060) {
		// Skip if 68040 because FSAVE frame can store both src and dst
		if (fault_if_unimplemented_680x0(opcode, extra, ad, adset, oldpc, src, -1)) {
			return -1;
		}
	}

	switch (size)
	{
		case 0: // L
			fpset(src, (uae_s32) (doext ? exts[0] : x_cp_get_long (ad)));
			break;
		case 1: // S
			fpp_to_single (src, (doext ? exts[0] : x_cp_get_long (ad)));
			normalize_or_fault_if_no_denormal_support(opcode, extra, adold, adset, oldpc, src);
			break;
		case 2: // X
			{
				uae_u32 wrd1, wrd2, wrd3;
				wrd1 = (doext ? exts[0] : x_cp_get_long (ad));
				ad += 4;
				wrd2 = (doext ? exts[1] : x_cp_get_long (ad));
				ad += 4;
				wrd3 = (doext ? exts[2] : x_cp_get_long (ad));
				fpp_to_exten (src, wrd1, wrd2, wrd3);
				normalize_or_fault_if_no_denormal_support(opcode, extra, adold, adset, oldpc, src);
			}
			break;
		case 3: // P
			{
				uae_u32 wrd[3];
				if (currprefs.cpu_model == 68060) {
					if (fault_if_no_packed_support (opcode, extra, adold, adset, oldpc, NULL, wrd))
						return 1;
				}
				wrd[0] = (doext ? exts[0] : x_cp_get_long (ad));
				ad += 4;
				wrd[1] = (doext ? exts[1] : x_cp_get_long (ad));
				ad += 4;
				wrd[2] = (doext ? exts[2] : x_cp_get_long (ad));
				if (fault_if_no_packed_support (opcode, extra, adold, adset, oldpc, NULL, wrd))
					return 1;
				fpp_to_pack (src, wrd, 0);
				fpp_normalize(src);
				return 1;
			}
			break;
		case 4: // W
			fpset(src, (uae_s16) (doext ? exts[0] : x_cp_get_word (ad)));
			break;
		case 5: // D
			{
				uae_u32 wrd1, wrd2;
				wrd1 = (doext ? exts[0] : x_cp_get_long (ad));
				ad += 4;
				wrd2 = (doext ? exts[1] : x_cp_get_long (ad));
				fpp_to_double (src, wrd1, wrd2);
				normalize_or_fault_if_no_denormal_support(opcode, extra, adold, adset, oldpc, src);
			}
			break;
		case 6: // B
			fpset(src, (uae_s8) (doext ? exts[0] : x_cp_get_byte (ad)));
			break;
		default:
			return 0;
	}
	return 1;
}

static int put_fp_value2(fpdata *value, uae_u32 opcode, uae_u16 extra, uaecptr oldpc, uaecptr *adp, bool *adsetp, const uaecptr *jit_ea)
{
	int size, mode, reg;
	uae_u32 ad = 0;
	bool adset = false;
	static const int sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
	static const int sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };

#if DEBUG_FPP
	if (!isinrom ())
		write_log(_T("PUTFP: %04X %04X\n"), opcode, extra);
#endif
#if 0
	if (!(extra & 0x4000)) {
		if (fault_if_no_fpu (opcode, extra, 0, oldpc))
			return 1;
		regs.fp[(extra >> 10) & 7] = *value;
		return 1;
	}
#endif
	reg = opcode & 7;
	mode = (opcode >> 3) & 7;
	size = (extra >> 10) & 7;
	switch (mode)
	{
		case 0: // Dn

			if ((size == 0 || size == 1 ||size == 4 || size == 6) && fault_if_no_fpu (opcode, extra, 0, false, oldpc))
				return -1;
			switch (size)
			{
				case 6: // B
					if (normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, oldpc, value))
						return 1;
					m68k_dreg (regs, reg) = (uae_u32)(((fpp_to_int (value, 0) & 0xff)
						| (m68k_dreg (regs, reg) & ~0xff)));
					if (fault_if_68040_integer_nonmaskable(opcode, extra, ad, adset, oldpc, value))
						return -1;
					break;
				case 4: // W
					if (normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, oldpc, value))
						return 1;
					m68k_dreg (regs, reg) = (uae_u32)(((fpp_to_int (value, 1) & 0xffff)
						| (m68k_dreg (regs, reg) & ~0xffff)));
					if (fault_if_68040_integer_nonmaskable(opcode, extra, ad, adset, oldpc, value))
						return -1;
					break;
				case 0: // L
					if (normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, oldpc, value))
						return 1;
					m68k_dreg (regs, reg) = (uae_u32)fpp_to_int (value, 2);
					if (fault_if_68040_integer_nonmaskable(opcode, extra, ad, adset, oldpc, value))
						return -1;
					break;
				case 1: // S
					if (normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, oldpc, value))
						return 1;
					m68k_dreg (regs, reg) = fpp_from_single (value);
					break;
				case 3: // packed
				case 7: // packed
				{
					// K-factor size and other errors are checked even if EA is illegal
					if (!currprefs.fpu_no_unimplemented || currprefs.cpu_model < 68040) {
						uae_u32 wrd[3];
						int kfactor = size == 7 ? m68k_dreg(regs, (extra >> 4) & 7) : extra;
						kfactor &= 127;
						if (kfactor & 64)
							kfactor |= ~63;
						fpp_normalize(value);
						fpp_from_pack(value, wrd, kfactor);
						fpp_get_status(&regs.fpsr);
					}
					return -2;
				}
				default:
					return 0;
			}
			return 1;
		case 1: // An
			return 0;
		case 2: // (An)
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg);
			adset = true;
			break;
		case 3: // (An)+
			// Also needed by fault_if_no_fpu 
			mmufixup[0].reg = reg;
			mmufixup[0].value = m68k_areg (regs, reg);
			fpu_mmu_fixup = true;
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg);
			adset = true;
			m68k_areg (regs, reg) += reg == 7 ? sz2[size] : sz1[size];
			break;
		case 4: // -(An)
			// Also needed by fault_if_no_fpu 
			mmufixup[0].reg = reg;
			mmufixup[0].value = m68k_areg (regs, reg);
			fpu_mmu_fixup = true;
			m68k_areg (regs, reg) -= reg == 7 ? sz2[size] : sz1[size];
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg);
			adset = true;
			// 68060 no fpu -(an): EA points to -4, not -12 if extended precision
			// or if packed datatype
			if (currprefs.cpu_model == 68060 && ((if_no_fpu() && sz1[size] == 12) || size == 3 || size == 7)) {
				ad += 8;
			}
			break;
		case 5: // (d16,An)
			ad = jit_ea ? *jit_ea : m68k_areg (regs, reg) + (uae_s32) (uae_s16) x_cp_next_iword ();
			adset = true;
			break;
		case 6: // (d8,An,Xn)+
			ad = jit_ea ? *jit_ea : x_cp_get_disp_ea_020 (m68k_areg (regs, reg), 0);
			adset = true;
			break;
		case 7:
			switch (reg)
			{
				case 0: // (xxx).W
					ad = jit_ea ? *jit_ea : (uae_s32) (uae_s16) x_cp_next_iword ();
					adset = true;
					break;
				case 1: // (xxx).L
					ad = jit_ea ? *jit_ea : x_cp_next_ilong ();
					adset = true;
					break;
				// Immediate and PC-relative modes are not supported
				default:
					return 0;
			}
	}

	*adp = ad;
	*adsetp = adset;

	if (fault_if_no_fpu (opcode, extra, ad, adset, oldpc))
		return -1;

	switch (size)
	{
		case 0: // L
			if (normalize_or_fault_if_no_denormal_support(opcode, extra, ad, adset, oldpc, value))
				return 1;
			x_cp_put_long(ad, (uae_u32)fpp_to_int(value, 2));
			if (fault_if_68040_integer_nonmaskable(opcode, extra, ad, adset, oldpc, value))
				return -1;
			break;
		case 1: // S
			if (normalize_or_fault_if_no_denormal_support(opcode, extra, ad, adset, oldpc, value))
				return 1;
			x_cp_put_long(ad, fpp_from_single(value));
			break;
		case 2: // X
			{
				if (normalize_or_fault_if_no_denormal_support(opcode, extra, ad, adset, oldpc, value))
					return 1;
				uae_u32 wrd1, wrd2, wrd3;
				fpp_from_exten(value, &wrd1, &wrd2, &wrd3);
				x_cp_put_long (ad, wrd1);
				ad += 4;
				x_cp_put_long (ad, wrd2);
				ad += 4;
				x_cp_put_long (ad, wrd3);
			}
			break;
		case 3: // Packed-Decimal Real with Static k-Factor
		case 7: // Packed-Decimal Real with Dynamic k-Factor (P{Dn}) (reg to memory only)
			{
				uae_u32 wrd[3];
				int kfactor;
				if (fault_if_no_packed_support (opcode, extra, ad, adset, oldpc, value, wrd))
					return 1;
				kfactor = size == 7 ? m68k_dreg (regs, (extra >> 4) & 7) : extra;
				kfactor &= 127;
				if (kfactor & 64)
					kfactor |= ~63;
				fpp_normalize(value);
				fpp_from_pack(value, wrd, kfactor);
				x_cp_put_long (ad, wrd[0]);
				ad += 4;
				x_cp_put_long (ad, wrd[1]);
				ad += 4;
				x_cp_put_long (ad, wrd[2]);
			}
			break;
		case 4: // W
			if (normalize_or_fault_if_no_denormal_support(opcode, extra, ad, adset, oldpc, value))
				return 1;
			x_cp_put_word(ad, (uae_s16)fpp_to_int(value, 1));
			if (fault_if_68040_integer_nonmaskable(opcode, extra, ad, adset, oldpc, value))
				return -1;
			break;
		case 5: // D
			{
				if (normalize_or_fault_if_no_denormal_support(opcode, extra, ad, adset, oldpc, value))
					return 1;
				uae_u32 wrd1, wrd2;
				fpp_from_double(value, &wrd1, &wrd2);
				x_cp_put_long (ad, wrd1);
				ad += 4;
				x_cp_put_long (ad, wrd2);
			}
			break;
		case 6: // B
			if (normalize_or_fault_if_no_denormal_support(opcode, extra, ad, adset, oldpc, value))
				return 1;
			x_cp_put_byte(ad, (uae_s8)fpp_to_int(value, 0));
			if (fault_if_68040_integer_nonmaskable(opcode, extra, ad, adset, oldpc, value))
				return -1;
			break;
		default:
			return 0;
	}
	return 1;
}

static int get_fp_ad (uae_u32 opcode, uae_u32 *ad, bool *adset, const uaecptr *jit_ea = nullptr)
{
	if (jit_ea) {
		*ad = *jit_ea;
		*adset = true;
		return 1;
	}
	int mode;
	int reg;

	mode = (opcode >> 3) & 7;
	reg = opcode & 7;
	switch (mode)
	{
		case 0: // Dn
		case 1: // An
			return 0;
		case 2: // (An)
			*ad = m68k_areg (regs, reg);
			*adset = true;
			break;
		case 3: // (An)+
			*ad = m68k_areg (regs, reg);
			*adset = true;
			break;
		case 4: // -(An)
			*ad = m68k_areg (regs, reg);
			*adset = true;
			break;
		case 5: // (d16,An)
			*ad = m68k_areg (regs, reg) + (uae_s32) (uae_s16) x_cp_next_iword ();
			*adset = true;
			break;
		case 6: // (d8,An,Xn)+
			*ad = x_cp_get_disp_ea_020 (m68k_areg (regs, reg), 0);
			*adset = true;
			break;
		case 7:
			switch (reg)
			{
				case 0: // (xxx).W
					*ad = (uae_s32) (uae_s16) x_cp_next_iword ();
					*adset = true;
					break;
				case 1: // (xxx).L
					*ad = x_cp_next_ilong ();
					*adset = true;
					break;
				case 2: // (d16,PC)
					*ad = m68k_getpc ();
					*ad += (uae_s32) (uae_s16) x_cp_next_iword ();
					*adset = true;
					break;
				case 3: // (d8,PC,Xn)+
					*ad = x_cp_get_disp_ea_020 (m68k_getpc (), 0);
					*adset = true;
					break;
				default:
					return 0;
			}
	}
	return 1;
}

static int put_fp_value(fpdata *value, uae_u32 opcode, uae_u16 extra, uaecptr oldpc, uaecptr *adp, bool *adsetp, const uaecptr *jit_ea = nullptr)
{
	int v = put_fp_value2(value, opcode, extra, oldpc, adp, adsetp, jit_ea);
	if (v == -2) {
		int size = (extra >> 10) & 7;
		if (size == 3 || size == 7) {
			// 68040+ generates unimplemented effective mode exception even if destination EA is Dn or An.
			// Always invalid EAs (Destination PC-relative or immediate) generate expected f-line exception.
			uae_u32 wrd[3];
			if (fault_if_no_packed_support(opcode, extra, 0, false, oldpc, value, wrd)) {
				if (regs.fp_unimp_pend) {
					fp_exception_pending(false);
				}
				return -1;
			}
		}
		return 0;
	}
	return v;
}

// 68040/68060
static const bool condition_table_040_060[] =
{
	0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1,
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1
};
// 68882
static const bool condition_table_6888x[] =
{
	0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
	0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
static const bool *condition_table = condition_table_6888x;

uae_u32 fpp_get_cond_mask(int condition)
{
	uae_u32 mask = 0;
	for (int control = 0; control < 16; ++control)
		mask |= uae_u32(condition_table[control * 32 + (condition & 31)]) << control;
	return mask;
}

int fpp_cond(int condition)
{
	condition &= 0x1f;
#ifdef JIT
	if (jit_fpu() && !fpu_fpsr_cc_written) {
		// JIT reads and writes regs.fpu_result
		fpp_is_init(&regs.fp_result);
		int NotANumber = fpp_is_nan(&regs.fp_result);
		int I = fpp_is_infinity(&regs.fp_result);
		int Z = fpp_is_zero(&regs.fp_result);
		int N = fpp_is_neg(&regs.fp_result);
		if ((condition & 0x10) && NotANumber) {
			if (fpsr_set_bsun())
				return -2;
		}
		int control = (N << 3) | (Z << 2) | (I << 1) | (NotANumber << 0);
		return condition_table[control * 32 + condition];
	} else
#endif
	{
		if ((condition & 0x10) && (regs.fpsr & FPSR_CC_NAN)) {
			if (fpsr_set_bsun())
				return -2;
		}
		int control = (regs.fpsr >> 24) & 15;
		return condition_table[control * 32 + condition];
	}

#if 0
	switch (condition)
	{
		case 0x00:
			return 0;
		case 0x01:
			return Z;
		case 0x02:
			return !(NotANumber || Z || N);
		case 0x03:
			return Z || !(NotANumber || N);
		case 0x04:
			return N && !(NotANumber || Z);
		case 0x05:
			return Z || (N && !NotANumber);
		case 0x06:
			return !(NotANumber || Z);
		case 0x07:
			return !NotANumber;
		case 0x08:
			return NotANumber;
		case 0x09:
			return NotANumber || Z;
		case 0x0a:
			return NotANumber || !(N || Z);
		case 0x0b:
			return NotANumber || Z || !N;
		case 0x0c:
			return NotANumber || (N && !Z);
		case 0x0d:
			return NotANumber || Z || N;
		case 0x0e:
			return !Z;
		case 0x0f:
			return 1;
		case 0x10:
			return 0;
		case 0x11:
			return Z;
		case 0x12:
			return !(NotANumber || Z || N);
		case 0x13:
			return Z || !(NotANumber || N);
		case 0x14:
			return N && !(NotANumber || Z);
		case 0x15:
			return Z || (N && !NotANumber);
		case 0x16:
			return !(NotANumber || Z);
		case 0x17:
			return !NotANumber;
		case 0x18:
			return NotANumber;
		case 0x19:
			return NotANumber || Z;
		case 0x1a:
			return NotANumber || !(N || Z);
		case 0x1b:
			return NotANumber || Z || !N;
		case 0x1c:
			return NotANumber || (N && !Z);
		case 0x1d:
			return NotANumber || Z || N;
		case 0x1e:
			return !Z;
		case 0x1f:
			return 1;
	}
	return -1;
#endif
}

static void maybe_idle_state (void)
{
	// conditional floating point instruction does not change state
	// from null to idle on 68040/060.
	if (currprefs.fpu_model == 68881 || currprefs.fpu_model == 68882)
		regs.fpu_state = 1;
}

static void trace_t0_68040(void)
{
	if (regs.t0 && currprefs.cpu_model == 68040)
		check_t0_trace();
}

/* Cycle-exact FPU timing, in clocks, assuming the FPU runs from the CPU clock.
 *
 * 68881/68882: MC68881/MC68882 User's Manual, tables 8-2, 8-3 and 8-6 to 8-8
 * (cache case). 68040: MC68040 User's Manual, section 10.7. 68060: MC68060
 * User's Manual, table 10-25.
 *
 * An instruction holds the CPU for "cpu" clocks from its start, and the first
 * "head" of those can overlap the previous FPU instruction. After that the FPU
 * keeps working for "tail" clocks while the CPU continues with integer
 * instructions. Bus cycles the instruction has already made count towards
 * "cpu". The 68881 has no head (8.2). Its tail is only listed relative to an
 * unspecified table (8-25), so the 68882 tail is used for it.
 */

#ifndef CPU_TESTER

#define FPU_TIMING_MAX_BUSY 4096

static evt_t fpu_timing_start, fpu_busy_until;

struct fpu_op_timing {
	// FPn,FPn: 68881 total, 68882 tail, 68040 FPU latency, 68060 total.
	// -1: not implemented in this FPU.
	uae_s16 total881, tail882, lat040, total060;
	bool dyadic;
};

static const struct fpu_op_timing fpu_op_timings[0x40] = {
	{ 33, 0, 2, 1, false },		// 00 FMOVE
	{ 55, 37, -1, 3, false },	// 01 FINT
	{ 687, 669, -1, -1, false },	// 02 FSINH
	{ 55, 37, -1, 3, false },	// 03 FINTRZ
	{ 107, 89, 107, 68, false },	// 04 FSQRT
	{ 0, 0, 0, 0, false },
	{ 571, 553, -1, -1, false },	// 06 FLOGNP1
	{ 0, 0, 0, 0, false },
	{ 545, 527, -1, -1, false },	// 08 FETOXM1
	{ 661, 643, -1, -1, false },	// 09 FTANH
	{ 403, 385, -1, -1, false },	// 0a FATAN
	{ 0, 0, 0, 0, false },
	{ 581, 563, -1, -1, false },	// 0c FASIN
	{ 693, 675, -1, -1, false },	// 0d FATANH
	{ 391, 373, -1, -1, false },	// 0e FSIN
	{ 473, 455, -1, -1, false },	// 0f FTAN
	{ 497, 479, -1, -1, false },	// 10 FETOX
	{ 567, 549, -1, -1, false },	// 11 FTWOTOX
	{ 567, 549, -1, -1, false },	// 12 FTENTOX
	{ 0, 0, 0, 0, false },
	{ 525, 507, -1, -1, false },	// 14 FLOGN
	{ 581, 563, -1, -1, false },	// 15 FLOG10
	{ 581, 563, -1, -1, false },	// 16 FLOG2
	{ 0, 0, 0, 0, false },
	{ 35, 17, 2, 1, false },	// 18 FABS
	{ 607, 589, -1, -1, false },	// 19 FCOSH
	{ 35, 17, 2, 1, false },	// 1a FNEG
	{ 0, 0, 0, 0, false },
	{ 625, 607, -1, -1, false },	// 1c FACOS
	{ 391, 373, -1, -1, false },	// 1d FCOS
	{ 45, 27, -1, -1, false },	// 1e FGETEXP
	{ 31, 13, -1, -1, false },	// 1f FGETMAN
	{ 103, 87, 42, 37, true },	// 20 FDIV
	{ 70, 54, -1, -1, true },	// 21 FMOD
	{ 51, 35, 7, 3, true },		// 22 FADD
	{ 71, 55, 9, 3, true },		// 23 FMUL
	{ 69, 53, 42, 37, true },	// 24 FSGLDIV
	{ 100, 84, -1, -1, true },	// 25 FREM
	{ 41, 25, -1, -1, true },	// 26 FSCALE
	{ 59, 43, 9, 3, true },		// 27 FSGLMUL
	{ 51, 35, 7, 3, true },		// 28 FSUB
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 451, 433, -1, -1, false },	// 30 FSINCOS
	{ 451, 433, -1, -1, false },
	{ 451, 433, -1, -1, false },
	{ 451, 433, -1, -1, false },
	{ 451, 433, -1, -1, false },
	{ 451, 433, -1, -1, false },
	{ 451, 433, -1, -1, false },
	{ 451, 433, -1, -1, false },
	{ 33, 17, 6, 1, true },		// 38 FCMP
	{ 0, 0, 0, 0, false },
	{ 33, 15, 2, 1, false },	// 3a FTST
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
	{ 0, 0, 0, 0, false },
};

// Source or destination format index: 0 FPn, 1 integer, 2 single, 3 double,
// 4 extended, 5 packed.
static const uae_u8 fpu_timing_format[8] = { 1, 2, 4, 5, 1, 3, 1, 5 };
// Per format: 68882 CPU time and head, added tail (monadic, dyadic), 68881 added total.
static const uae_s16 fpu_cpu882[6] = { 21, 40, 31, 37, 43, 82 };
static const uae_s16 fpu_head882[6] = { 17, 21, 30, 36, 42, 13 };
static const uae_s16 fpu_tail882_monadic[6] = { 0, 11, 3, 3, 3, 794 };
static const uae_s16 fpu_tail882_dyadic[6] = { 0, 19, 3, 3, 3, 792 };
static const uae_s16 fpu_total881[6] = { 0, 27, 19, 25, 23, 837 };
// FMOVE <ea>,FPn
static const uae_s16 fpu_fmove_cpu882[6] = { 21, 40, 34, 40, 46, 82 };
static const uae_s16 fpu_fmove_head882[6] = { 21, 21, 34, 40, 46, 13 };
static const uae_s16 fpu_fmove_tail882[6] = { 0, 8, 0, 0, 0, 809 };
static const uae_s16 fpu_fmove_total881[6] = { 33, 60, 52, 58, 56, 870 };
// FMOVE FPn,<ea>
static const uae_s16 fpu_fmove_out881[6] = { 0, 100, 80, 86, 72, 2002 };
static const uae_s16 fpu_fmove_out882[6] = { 0, 110, 38, 44, 50, 2006 };

static bool fpu_6888x(void)
{
	return currprefs.fpu_model == 68881 || currprefs.fpu_model == 68882;
}

// One CPU clock. 68040 and 68060 cycle-exact code counts in half clocks.
static evt_t fpu_timing_clock(void)
{
	return currprefs.cpu_model >= 68040 ? cpucycleunit * 2 : cpucycleunit;
}

static void fpu_timing_begin(void)
{
	fpu_timing_start = get_cycles();
	if (fpu_busy_until > fpu_timing_start + FPU_TIMING_MAX_BUSY * fpu_timing_clock()) {
		fpu_busy_until = 0;
	}
}

static void fpu_timing_end(int cpu, int head, int tail)
{
	if (!currprefs.cpu_cycle_exact || currprefs.cpu_model < 68020) {
		return;
	}
	evt_t clock = fpu_timing_clock();
	// The overlap is part of the instruction's own time: a longer head
	// would release it before the previous instruction's result exists.
	if (head > cpu) {
		head = cpu;
	}
	evt_t release = fpu_timing_start + cpu * clock;
	evt_t after = fpu_busy_until + (cpu - head) * clock;
	if (after > release) {
		release = after;
	}
	evt_t now = get_cycles();
	if (release > now) {
		x_do_cycles((int)(release - now));
	}
	fpu_busy_until = get_cycles() + tail * clock;
}

// Wait for the FPU, then hold the CPU for cpu clocks: instructions that use
// the result of the previous one, or that the 68040/68060 FPU does not overlap.
static void fpu_timing_sync(int cpu)
{
	fpu_timing_end(cpu, 0, 0);
}

// 6888x conditional instructions, Table 8-7: 6 clocks overlap the previous
// instruction, the condition then needs its result.
static void fpu_timing_cond(int c6888x, int c040)
{
	if (fpu_6888x()) {
		fpu_timing_end(c6888x, 6, 0);
	} else {
		fpu_timing_sync(currprefs.fpu_model == 68040 ? c040 : 0);
	}
}

// An instruction that writes FPU state to the bus (FMOVE, FMOVE(M) and
// FMOVEM to <ea>, FScc, FSAVE) waits for the previous instruction before
// it executes, so the write cannot be seen before the result exists. Only
// the first head clocks of a 6888x instruction overlap it. fpu_timing_end()
// then charges the rest of the instruction from the new start.
static void fpu_timing_wait(int head6888x)
{
	if (!currprefs.cpu_cycle_exact || currprefs.cpu_model < 68020) {
		return;
	}
	evt_t ready = fpu_busy_until - (fpu_6888x() ? head6888x : 0) * fpu_timing_clock();
	evt_t now = get_cycles();
	if (ready > now) {
		x_do_cycles((int)(ready - now));
		fpu_timing_start = get_cycles();
	}
}

static void fpu_timing_store_wait(uae_u16 extra)
{
	switch ((extra >> 13) & 7)
	{
		case 3: // FMOVE FPn,<ea>
			fpu_timing_wait(0);
			break;
		case 5: // FMOVE(M) control registers to <ea>
		case 7: // FMOVEM FPn to <ea>
			fpu_timing_wait(6);
			break;
	}
}

static int fpu_timing_opmode(int opmode)
{
	// 68040/68060 single and double precision rounding variants
	switch (opmode)
	{
		case 0x40: case 0x44: return 0x00;
		case 0x41: case 0x45: return 0x04;
		case 0x58: case 0x5c: return 0x18;
		case 0x5a: case 0x5e: return 0x1a;
		case 0x60: case 0x64: return 0x20;
		case 0x62: case 0x66: return 0x22;
		case 0x63: case 0x67: return 0x23;
		case 0x68: case 0x6c: return 0x28;
		// 6888x undocumented opmodes that execute as their neighbours
		// (see fault_if_nonexisting_opmode() and fp_arithmetic())
		case 0x05: return 0x04;
		case 0x07: return 0x06;
		case 0x0b: return 0x0a;
		case 0x13: return 0x12;
		case 0x17: return 0x16;
		case 0x1b: return 0x1a;
		case 0x29: case 0x2a: case 0x2b: case 0x2c:
		case 0x2d: case 0x2e: case 0x2f: return 0x28;
		case 0x39: case 0x3c: case 0x3d: return 0x38;
		case 0x3b: case 0x3e: case 0x3f: return 0x3a;
	}
	return opmode & 0x3f;
}

static void fpu_timing_arithmetic(uae_u32 opcode, uae_u16 extra)
{
	int opclass = (extra >> 13) & 7;
	int fmt = opclass ? fpu_timing_format[(extra >> 10) & 7] : 0;
	bool dn = opclass && (opcode & 0x38) == 0;
	int fpu_model = currprefs.fpu_model;

	switch (opclass)
	{
		case 0:
		case 2:
		{
			if (opclass == 2 && (extra & 0xfc00) == 0x5c00) {
				// FMOVECR
				if (fpu_model == 68882) {
					fpu_timing_end(32, 10, 0);
				} else if (fpu_model == 68881) {
					fpu_timing_end(29, 0, 0);
				} else {
					fpu_timing_sync(2);
				}
				return;
			}
			const struct fpu_op_timing *t = &fpu_op_timings[fpu_timing_opmode(extra & 0x7f)];
			int cpu, head, tail;
			if (t->total881 == 0) {
				return;
			}
			if ((extra & 0x7f) == 0x00) {
				tail = fpu_fmove_tail882[fmt];
				cpu = fpu_fmove_cpu882[fmt];
				head = fpu_fmove_head882[fmt];
				if (fpu_model == 68881) {
					cpu = fpu_fmove_total881[fmt] - tail;
				}
			} else {
				tail = t->tail882 + (t->dyadic ? fpu_tail882_dyadic[fmt] : fpu_tail882_monadic[fmt]);
				cpu = fpu_cpu882[fmt];
				head = fpu_head882[fmt];
				if (fpu_model == 68881) {
					int total = t->total881 + fpu_total881[fmt] + (t->dyadic && fmt >= 1 && fmt <= 4 ? 2 : 0);
					cpu = total - tail;
				}
			}
			if (fpu_model == 68881) {
				head = 0;
			} else if (fpu_model == 68040) {
				// Integer unit support time, then the FPU pipeline. Instructions
				// emulated instead of trapping to the FPSP use the 68882 tail.
				cpu = 2;
				head = cpu;
				tail = t->lat040 >= 0 ? t->lat040 : t->tail882;
			} else if (fpu_model == 68060) {
				// The 68060 FPU runs alongside the integer unit but does not
				// overlap two floating point instructions.
				cpu = 1;
				head = 0;
				tail = t->total060 >= 0 ? t->total060 - 1 : t->tail882;
			}
			if (dn && fpu_6888x()) {
				cpu -= 5;
			}
			fpu_timing_end(cpu, head, tail);
			return;
		}
		case 3:
		{
			// FMOVE FPn,<ea>: needs the result of the previous instruction.
			int cpu;
			if (fpu_model == 68881) {
				cpu = fpu_fmove_out881[fmt];
			} else if (fpu_model == 68882) {
				cpu = fpu_fmove_out882[fmt];
			} else {
				cpu = fpu_model == 68040 ? 3 : (dn ? 3 : 1);
			}
			if (fpu_6888x()) {
				if (fmt == 5 && (extra & 0x1c00) == 0x1c00) {
					cpu += 14;
				}
				if (dn) {
					cpu -= 2;
				}
			}
			fpu_timing_sync(cpu);
			return;
		}
		case 4:
		case 5:
		{
			// FMOVE(M) control registers, Table 8-6
			if (!fpu_6888x()) {
				fpu_timing_sync(0);
				return;
			}
			int n = ((extra >> 12) & 1) + ((extra >> 11) & 1) + ((extra >> 10) & 1);
			int cpu;
			if ((opcode & 0x30) == 0) {
				// Dn or An
				cpu = (extra & 0x2000) ? 31 : 28;
			} else if ((opcode & 0x3f) == 0x3c) {
				cpu = n > 1 ? 25 + 6 * n : 30;
			} else {
				cpu = n > 1 ? 27 + 6 * n : 33;
			}
			if (fpu_model == 68882) {
				cpu += 2;
			}
			fpu_timing_end(cpu, 6, 0);
			return;
		}
		case 6:
		case 7:
		{
			// FMOVEM data registers, Table 8-6
			int n = 0;
			int list = (extra & 0x0800) ? (m68k_dreg(regs, (extra >> 4) & 7) & 0xff) : (extra & 0xff);
			for (int i = 0; i < 8; i++) {
				n += (list >> i) & 1;
			}
			if (fpu_model == 68040) {
				fpu_timing_sync(2 + 2 * n);
			} else if (fpu_model == 68060) {
				fpu_timing_sync(1 + 3 * n);
			} else {
				int cpu = (extra & 0x2000) ? 37 + 25 * n : 35 + 31 * n;
				if (extra & 0x0800) {
					cpu += 14;
				}
				if (fpu_model == 68882) {
					cpu += 2;
				}
				fpu_timing_end(cpu, 6, 0);
			}
			return;
		}
	}
}

// FSAVE and FRESTORE of null or idle frames, Table 8-8
static void fpu_timing_save_restore(bool restore, bool idle)
{
	if (!fpu_6888x()) {
		fpu_timing_sync(0);
		return;
	}
	int cpu = restore ? 21 : 16;
	if (idle) {
		cpu += currprefs.fpu_model == 68882 ? 84 : 36;
	}
	fpu_timing_sync(cpu);
}

#else

static void fpu_timing_begin(void)
{
}
static void fpu_timing_arithmetic(uae_u32 opcode, uae_u16 extra)
{
}
static void fpu_timing_cond(int c6888x, int c040)
{
}
static void fpu_timing_save_restore(bool restore, bool idle)
{
}
static void fpu_timing_wait(int head6888x)
{
}
static void fpu_timing_store_wait(uae_u16 extra)
{
}

#endif


void fpuop_dbcc (uae_u32 opcode, uae_u16 extra)
{
	uaecptr pc = m68k_getpc ();
	uae_s32 disp;
	int cc;

	fpu_timing_begin();
	if (fp_exception_pending(true))
		return;
	regs.fp_exception = false;

#if FPU_LOG
	write_log(_T("FDBcc %04x %04x %08x\n"), opcode, extra, M68K_GETPC);
#endif

	if (fault_if_no_6888x (opcode, extra, pc - 4))
		return;

	disp = (uae_s32) (uae_s16)x_cp_next_iword();
	// 68060 unimplemented stacked EA contains original extra + disp values
	// not final absolute address.
	if (fault_if_no_fpu_u (opcode, extra, (extra << 16) | (disp & 0xffff), true, pc - 4))
		return;
	maybe_idle_state ();
	cc = fpp_cond (extra);
	if (cc < 0) {
		if (cc == -2)
			return; // BSUN
		fpu_op_illg (opcode, 0, false, pc - 4);
	} else if (!cc) {
		int reg = opcode & 0x7;

		m68k_dreg (regs, reg) = ((m68k_dreg (regs, reg) & 0xffff0000)
			| (((m68k_dreg (regs, reg) & 0xffff) - 1) & 0xffff));
		if ((m68k_dreg (regs, reg) & 0xffff) != 0xffff) {
			m68k_setpc (pc + disp);
			regs.fp_branch = true;
		}
	}
	if (cc >= 0) {
		if (cc) {
			fpu_timing_cond(20, 7);
		} else {
			fpu_timing_cond(regs.fp_branch ? 20 : 24, 9);
		}
	}
	// 68040 FDBCC: T0 always
	trace_t0_68040();
}

void fpuop_scc (uae_u32 opcode, uae_u16 extra)
{
	uae_u32 ad = 0;
	bool adset = false;
	int cc;
	uaecptr pc = m68k_getpc () - 4;
	int mode = (opcode >> 3) & 7;
	int reg = opcode & 7;

	fpu_timing_begin();
	fpu_timing_wait(6);
	if (fp_exception_pending(true))
		return;
	regs.fp_exception = false;

#if FPU_LOG
	write_log(_T("FScc %04x %04x %08x\n"), opcode, extra, M68K_GETPC);
#endif

	if (fault_if_no_6888x (opcode, extra, pc))
		return;

	// EA calculation needed? (Mode != 000)
	if (mode) {
		if (get_fp_ad (opcode, &ad, &adset) == 0) {
			fpu_noinst (opcode, regs.fpiar);
			return;
		}
	}

	// needs to be before 68060 check because
	// unimplemented stack frame stacked EA is adjusted
	// Register is not adjusted.
	if (mode == Apdi) {
		ad -= reg == 7 ? 2 : 1;
	}

	if (fault_if_no_fpu_u (opcode, extra, ad, adset, pc))
		return;

	maybe_idle_state ();
	cc = fpp_cond (extra);
	if (cc < 0) {
		if (cc == -2)
			return; // BSUN
		fpu_op_illg (opcode, 0, false, regs.fpiar);
	} else if (!mode) {
		m68k_dreg (regs, reg) = (m68k_dreg (regs, reg) & ~0xff) | (cc ? 0xff : 0x00);
	} else {
		if (mode == Apdi) {
			mmufixup[0].reg = reg;
			mmufixup[0].value = m68k_areg(regs, reg);
			fpu_mmu_fixup = true;
			m68k_areg(regs, reg) = ad;
		}
		x_cp_put_byte (ad, cc ? 0xff : 0x00);
		if (mode == Aipi) {
			m68k_areg(regs, reg) += reg == 7 ? 2 : 1;
		}
		fpu_mmu_fixup = false;
	}
	if (cc >= 0) {
		fpu_timing_cond(mode ? 22 : 18, 6);
	}
}

void fpuop_trapcc (uae_u32 opcode, uaecptr oldpc, uae_u16 extra)
{
	int cc;

	fpu_timing_begin();
	if (fp_exception_pending(true))
		return;
	regs.fp_exception = false;

#if FPU_LOG
	write_log(_T("FTRAPcc %04x %04x %08x\n"), opcode, extra, M68K_GETPC);
#endif

	if (fault_if_no_fpu_u (opcode, extra, 0, false, oldpc))
		return;

	if (currprefs.fpu_model == 68060) {
		regs.fpiar = oldpc;
	}
	maybe_idle_state ();
	cc = fpp_cond (extra);
	if (cc < 0) {
		if (cc == -2)
			return; // BSUN
		fpu_op_illg (opcode, 0, false, oldpc);
	} else if (cc) {
		regs.fp_exception = true;
		Exception_cpu_oldpc(7, oldpc);
	} else {
		fpu_timing_cond((opcode & 7) == 2 ? 20 : ((opcode & 7) == 3 ? 22 : 18), 6);
	}
}

void fpuop_bcc(uae_u32 opcode, uaecptr oldpc, uae_u32 extra)
{
	int cc;

	fpu_timing_begin();
	if (fp_exception_pending(true))
		return;
	regs.fp_exception = false;

#if FPU_LOG
	write_log(_T("FBcc %04x %04x %08x\n"), opcode, extra, M68K_GETPC);
#endif

	if (fault_if_no_fpu(opcode, extra, 0, false, oldpc - 2))
		return;

	if (currprefs.fpu_model == 68060 && (opcode != 0xf280 || extra != 0)) {
		regs.fpiar = oldpc - 2;
	}
	maybe_idle_state();
	cc = fpp_cond(opcode);
	if (cc < 0) {
		if (cc == -2)
			return; // BSUN
		fpu_op_illg(opcode, 0, false, oldpc - 2);
	} else if (cc) {
		if ((opcode & 0x40) == 0)
			extra = (uae_s32) (uae_s16) extra;
		m68k_setpc(oldpc + extra);
		regs.fp_branch = true;
	}
	if (cc >= 0) {
		fpu_timing_cond(cc ? 20 : 18, cc ? 7 : 6);
	}
}

void fpuop_save (uae_u32 opcode)
{
	uae_u32 ad, adp;
	bool adset = false;
	int incr = (opcode & 0x38) == 0x20 ? -1 : 1;
	int fpu_version = get_fpu_version (currprefs.fpu_model);
	uaecptr pc = m68k_getpc () - 2;
	int i;

#if FPU_LOG
	if (!isinrom())
		write_log(_T("FSAVE %04x %08x\n"), opcode, M68K_GETPC);
#endif

	regs.fp_exception = false;
	fpu_timing_begin();
	fpu_timing_wait(0);

	if (fault_if_no_6888x (opcode, 0, pc))
		return;

	if (get_fp_ad (opcode, &ad, &adset) == 0) {
		fpu_op_illg (opcode, 0, false, pc);
		return;
	}

	if (fault_if_no_fpu (opcode, 0, ad, adset, pc))
		return;

//	write_log(_T("FSAVE %08x %08x\n"), M68K_GETPC, ad);

	if (currprefs.fpu_model == 68060) {

		/* 12 byte 68060 NULL/IDLE/EXCP frame.  */
		int frame_size = 12;
		uae_u32 frame_id;
		
		if (regs.fpu_exp_state > 1) {
			frame_id = 0x0000e000 | fsave_data.v;

#if 0
			write_log(_T("68060 FSAVE EXCP %s\n"), fpp_print(&fsave_data.src));
#endif

		} else {
			frame_id = regs.fpu_state == 0 ? 0x00000000 : 0x00006000;
		}
		if (incr < 0)
			ad -= frame_size;
		adp = ad;
		x_cp_put_long (ad, (fsave_data.eo[0] & 0xffff0000) | frame_id);
		ad += 4;
		x_cp_put_long (ad, fsave_data.eo[1]);
		ad += 4;
		x_cp_put_long (ad, fsave_data.eo[2]);
		ad += 4;

	} else if (currprefs.fpu_model == 68040) {

		if (!regs.fpu_exp_state) {
			/* 4 byte 68040 NULL/IDLE frame.  */
			uae_u32 frame_id = regs.fpu_state == 0 ? 0 : fpu_version << 24;
			if (incr < 0)
				ad -= 4;
			adp = ad;
			x_cp_put_long (ad, frame_id);
			ad += 4;
		} else {
			/* 44 (rev $40) and 52 (rev $41) byte 68040 unimplemented instruction frame */
			/* 96 byte 68040 busy frame */
			int frame_size = regs.fpu_exp_state == 2 ? 0x64 : (fpu_version >= 0x41 ? 0x34 : 0x2c);
			uae_u32 frame_id = ((fpu_version << 8) | (frame_size - 4)) << 16;

#if 0
			write_log(_T("68040 FSAVE %d (%d), CMDREG=%04X"), regs.fp_exp_pend, frame_size, extra);
			if (regs.fp_exp_pend == FPU_EXP_UNIMP_DATATYPE_PACKED_PRE) {
				write_log(_T(" PACKED %08x-%08x-%08x"), fsave_data.pack[0], fsave_data.pack[1], fsave_data.pack[2]);
			} else if (regs.fp_exp_pend == FPU_EXP_UNIMP_DATATYPE_PACKED_POST) {
				write_log(_T(" SRC=%s (%08x-%08x-%08x %d)"),
					fpp_print(&fsave_data.src), src1[0], src1[1], src1[2], stag);
				write_log(_T(" DST=%s (%08x-%08x-%08x %d)"),
					fpp_print(&fsave_data.dst), src2[0], src2[1], src2[2], dtag);
			 }
#endif

			if (incr < 0)
				ad -= frame_size;
			adp = ad;
			x_cp_put_long (ad, frame_id);
			ad += 4;
			if (regs.fpu_exp_state == 2) {
				/* BUSY frame */
				x_cp_put_long(ad, 0);
				ad += 4;
				x_cp_put_long(ad, 0); // CU_SAVEPC (Software shouldn't care)
				ad += 4;
				x_cp_put_long(ad, 0);
				ad += 4;
				x_cp_put_long(ad, 0);
				ad += 4;
				x_cp_put_long(ad, 0);
				ad += 4;
				x_cp_put_long(ad, fsave_data.wbt[0]); // WBTS/WBTE
				ad += 4;
				x_cp_put_long(ad, fsave_data.wbt[1]); // WBTM
				ad += 4;
				x_cp_put_long(ad, fsave_data.wbt[2]); // WBTM
				ad += 4;
				x_cp_put_long(ad, 0);
				ad += 4;
				x_cp_put_long(ad, fsave_data.fpiarcu); // FPIARCU (same as FPU PC or something else?)
				ad += 4;
				x_cp_put_long(ad, 0);
				ad += 4;
				x_cp_put_long(ad, 0);
				ad += 4;
			}
			if (fpu_version >= 0x41 || regs.fpu_exp_state == 2) {
				x_cp_put_long(ad, fsave_data.cmdreg3b << 16); // CMDREG3B
				ad += 4;
				x_cp_put_long (ad, 0);
				ad += 4;
			}
			x_cp_put_long (ad, (fsave_data.stag << 29) | (fsave_data.wbtm66 << 26) | (fsave_data.grs << 23)); // STAG
			ad += 4;
			x_cp_put_long (ad, fsave_data.cmdreg1b << 16); // CMDREG1B
			ad += 4;
			x_cp_put_long (ad, (fsave_data.dtag << 29) | (fsave_data.wbte15 << 20)); // DTAG
			ad += 4;
			x_cp_put_long (ad, (fsave_data.e1 << 26) | (fsave_data.e3 << 25) | (fsave_data.t << 20));
			ad += 4;
			x_cp_put_long(ad, fsave_data.fpt[0]); // FPTS/FPTE
			ad += 4;
			x_cp_put_long(ad, fsave_data.fpt[1]); // FPTM
			ad += 4;
			x_cp_put_long(ad, fsave_data.fpt[2]); // FPTM
			ad += 4;
			x_cp_put_long(ad, fsave_data.et[0]); // ETS/ETE
			ad += 4;
			x_cp_put_long(ad, fsave_data.et[1]); // ETM
			ad += 4;
			x_cp_put_long(ad, fsave_data.et[2]); // ETM
			ad += 4;
		}
		// 68040 FSAVE: T0 always
		trace_t0_68040();
	} else { /* 68881/68882 */
		uae_u32 biu_flags = 0x540effff;
		int frame_size = currprefs.fpu_model == 68882 ? 0x3c : 0x1c;
		uae_u32 frame_id = regs.fpu_state == 0 ? ((frame_size - 4) << 16) : (fpu_version << 24) | ((frame_size - 4) << 16);
		
		regs.fp_exp_pend = 0;
		if (regs.fpu_exp_state) {
			biu_flags |= 0x20000000;
		} else {
			biu_flags |= 0x08000000;
		}
		if (regs.fpu_state == 0)
			frame_size = 4;

		if (currprefs.mmu_model) {
			if (incr < 0)
				ad -= frame_size;
			adp = ad;
			x_cp_put_long(ad, frame_id); // frame id
			ad += 4;
			if (regs.fpu_state != 0) { // idle frame
				x_cp_put_long(ad, fsave_data.ccr); // command/condition register
				ad += 4;
				if (currprefs.fpu_model == 68882) {
					// don't write unused fields to save MMU state space.
					ad += 8 * 4;
				}
				x_cp_put_long(ad, fsave_data.eo[0]); // exceptional operand lo
				ad += 4;
				x_cp_put_long(ad, fsave_data.eo[1]); // exceptional operand mid
				ad += 4;
				x_cp_put_long(ad, fsave_data.eo[2]); // exceptional operand hi
				ad += 4;
				x_cp_put_long(ad, 0x00000000); // operand register
				ad += 4;
				x_cp_put_long(ad, biu_flags); // biu flags
				ad += 4;
			}
		} else {
			if (incr < 0)
				ad -= frame_size;
			adp = ad;
			x_cp_put_long(ad, frame_id); // frame id
			ad += 4;
			if (regs.fpu_state != 0) { // idle frame
				x_cp_put_long(ad, fsave_data.ccr); // command/condition register
				ad += 4;
				if(currprefs.fpu_model == 68882) {
					for(i = 0; i < 32; i += 4) {
						x_cp_put_long(ad, 0x00000000); // internal
						ad += 4;
					}
				}
				x_cp_put_long(ad, fsave_data.eo[0]); // exceptional operand hi
				ad += 4;
				x_cp_put_long(ad, fsave_data.eo[1]); // exceptional operand mid
				ad += 4;
				x_cp_put_long(ad, fsave_data.eo[2]); // exceptional operand lo
				ad += 4;
				x_cp_put_long(ad, 0x00000000); // operand register
				ad += 4;
				x_cp_put_long(ad, biu_flags); // biu flags
				ad += 4;
			}
		}
	}

	if ((opcode & 0x38) == 0x20) // predecrement
		m68k_areg (regs, opcode & 7) = adp;
	regs.fpu_exp_state = 0;
	regs.fp_exp_pend = 0;
	fpu_timing_save_restore(false, regs.fpu_state != 0);
}

static bool fp_arithmetic(fpdata *src, fpdata *dst, int extra);

void fpuop_restore (uae_u32 opcode)
{
	int fpu_version;
	int frame_version;
	int fpu_model = currprefs.fpu_model;
	uaecptr pc = m68k_getpc () - 2;
	uae_u32 ad, ad_orig;
	bool adset = false;
	uae_u32 d;

	regs.fp_exception = false;
	fpu_timing_begin();

#if FPU_LOG
	if (!isinrom())
		write_log(_T("FRESTORE %04x %08x\n"), opcode, M68K_GETPC);
#endif

	if (fault_if_no_6888x (opcode, 0, pc))
		return;

	if (get_fp_ad (opcode, &ad, &adset) == 0) {
		fpu_op_illg (opcode, 0, false, pc);
		return;
	}

	if (fault_if_no_fpu (opcode, 0, ad, adset, pc))
		return;
	ad_orig = ad;

	// write_log(_T("FRESTORE %08x %08x\n"), M68K_GETPC, ad);

	// FRESTORE does not support predecrement

	d = x_cp_get_long(ad);

	frame_version = (d >> 24) & 0xff;

retry:
	ad = ad_orig + 4;
	fpu_version = get_fpu_version(fpu_model);
	if (fpu_model == 68060) {
		int ff = (d >> 8) & 0xff;
		uae_u32 v = d & 0x7;
		fsave_data.eo[0] = d & 0xffff0000;

		fsave_data.eo[1] = x_cp_get_long(ad);
		ad += 4;
		fsave_data.eo[2] = x_cp_get_long(ad);
		ad += 4;

		regs.fp_exp_pend = 0;
		if (ff == 0x60) {
			regs.fpu_state = 1;
			regs.fpu_exp_state = 0;
		} else if (ff == 0xe0) {
			regs.fpu_state = 1;
			regs.fpu_exp_state = 2;
			if (v == 7) {
				regs.fp_unimp_pend = 1;
			} else {
				regs.fp_exp_pend = 48 + v;
			}
		} else if (ff) {
			write_log(_T("FRESTORE invalid frame format %02x %08x ADDR=%08x\n"), ff, d, ad_orig);
			regs.fp_exception = true;
			Exception(14);
			return;
		} else {
			fpu_null();
		}

	} else if (fpu_model == 68040) {

		 if (frame_version == fpu_version) { // not null frame
			uae_u32 frame_size = (d >> 16) & 0xff;

			if (frame_size == 0x60) { // busy
				fpdata src, dst;
				uae_u32 tmp, v, opclass, cmdreg1b, fpte15, et15, cusavepc;

				ad += 0x4; // offset to CU_SAVEPC field
				tmp = x_cp_get_long(ad);
				cusavepc = tmp >> 24;
				ad += 0x34; // offset to ET15 field
				tmp = x_cp_get_long(ad);
				et15 = (tmp & 0x10000000) >> 28;
				ad += 0x4; // offset to CMDREG1B field
				fsave_data.cmdreg1b = x_cp_get_long(ad);
				fsave_data.cmdreg1b >>= 16;
				cmdreg1b = fsave_data.cmdreg1b;
				ad += 0x4; // offset to FPTE15 field
				tmp = x_cp_get_long(ad);
				fpte15 = (tmp & 0x10000000) >> 28;
				ad += 0x8; // offset to FPTE field
				fsave_data.fpt[0] = x_cp_get_long(ad);
				ad += 0x4;
				fsave_data.fpt[1] = x_cp_get_long(ad);
				ad += 0x4;
				fsave_data.fpt[2] = x_cp_get_long(ad);
				ad += 0x4; // offset to ET field
				fsave_data.et[0] = x_cp_get_long(ad);
				ad += 0x4;
				fsave_data.et[1] = x_cp_get_long(ad);
				ad += 0x4;
				fsave_data.et[2] = x_cp_get_long(ad);
				ad += 0x4;
				
				opclass = (cmdreg1b >> 13) & 0x7; // just to be sure
				
				if (cusavepc == 0xFE) {
					if (opclass == 0 || opclass == 2) {
						fpp_to_exten_fmovem(&dst, fsave_data.fpt[0], fsave_data.fpt[1], fsave_data.fpt[2]);
						fpp_denormalize(&dst, fpte15);
						fpp_to_exten_fmovem(&src, fsave_data.et[0], fsave_data.et[1], fsave_data.et[2]);
						fpp_denormalize(&src, et15);
#if EXCEPTION_FPP
						uae_u32 tmpsrc[3], tmpdst[3];
						fpp_from_exten_fmovem(&src, &tmpsrc[0], &tmpsrc[1], &tmpsrc[2]);
						fpp_from_exten_fmovem(&dst, &tmpdst[0], &tmpdst[1], &tmpdst[2]);
						write_log(_T("FRESTORE src = %08X %08X %08X, dst = %08X %08X %08X, extra = %04X\n"),
								   tmpsrc[0], tmpsrc[1], tmpsrc[2], tmpdst[0], tmpdst[1], tmpdst[2], cmdreg1b);
#endif
						fpsr_clear_status();
						
						v = fp_arithmetic(&src, &dst, cmdreg1b);
						
						if (v)
							regs.fp[(cmdreg1b>>7)&7] = dst;
						
						fpsr_check_arithmetic_exception(0, &src, regs.fp_opword, cmdreg1b, regs.fp_ea, regs.fp_ea_set, 0xffffffff);
					} else {
						write_log(_T("FRESTORE resume of opclass %d instruction not supported %08x\n"), opclass, ad_orig);
					}
				}

			} else if (frame_size == 0x30 || frame_size == 0x28) { // unimp

				// TODO: restore frame contents
				ad += frame_size;

			} else if (frame_size == 0x00) { // idle
				regs.fpu_state = 1;
				regs.fpu_exp_state = 0;
			} else {
				write_log(_T("FRESTORE invalid frame size %02x %08x %08x\n"), frame_size, d, ad_orig);

				regs.fp_exception = true;
				Exception(14);
				return;

			}
		} else if (frame_version == 0x00) { // null frame
			fpu_null();
		} else {
			if (!currprefs.fpu_no_unimplemented && frame_version == 0x1f && currprefs.fpu_model == fpu_model) {
				// horrible hack to support on the fly 6888x <> 68040 FPU switching
				fpu_model = 68881;
				goto retry;
			}
			write_log(_T("FRESTORE 68040 (%d) invalid frame version %02x %08x %08x\n"), fpu_model, frame_version, d, ad_orig);
			regs.fp_exception = true;
			Exception(14);
			return;
		}

	} else {

		// 6888x
		if (frame_version == fpu_version) { // not null frame
			uae_u32 biu_flags;
			uae_u32 frame_size = (d >> 16) & 0xff;
			uae_u32 biu_offset = frame_size - 4;
			regs.fpu_state = 1;

			if (frame_size == 0x18 || frame_size == 0x38) { // idle

				fsave_data.ccr = x_cp_get_long(ad);
				ad += 4;
				// 68882 internal registers (32 bytes, unused)
				ad += frame_size - 24;
				fsave_data.eo[0] = x_cp_get_long(ad);
				ad += 4;
				fsave_data.eo[1] = x_cp_get_long(ad);
				ad += 4;
				fsave_data.eo[2] = x_cp_get_long(ad);
				ad += 4;
				// operand register (unused)
				ad += 4;
				biu_flags = x_cp_get_long(ad);
				ad += 4;
				
				if ((biu_flags & 0x08000000) == 0x00000000) {
					regs.fpu_exp_state = 2;
					regs.fp_exp_pend = fpsr_get_vector(regs.fpsr & regs.fpcr & 0xff00);
				} else {
					regs.fpu_exp_state = 0;
					regs.fp_exp_pend = 0;
				}
			} else if (frame_size == 0xB4 || frame_size == 0xD4) {
				write_log(_T("FRESTORE of busy frame not supported %08x\n"), ad_orig);
				ad += frame_size;
			} else {
				write_log(_T("FRESTORE invalid frame size %02x %08x %08x\n"), frame_size, d, ad_orig);
				regs.fp_exception = true;
				Exception(14);
				return;
			}
		} else if (frame_version == 0x00) { // null frame
			fpu_null();
		} else {
			if (!currprefs.fpu_no_unimplemented && (frame_version == 0x40 || frame_version == 0x41) && currprefs.fpu_model == fpu_model) {
				// horrible hack to support on the fly 6888x <> 68040 FPU switching
				fpu_model = 68040;
				goto retry;
			}
			write_log(_T("FRESTORE 6888x (%d) invalid frame version %02x %08x %08x\n"), fpu_model, frame_version, d, ad_orig);
			regs.fp_exception = true;
			Exception(14);
			return;
		}
	}

	if ((opcode & 0x38) == 0x18) /// postincrement
		m68k_areg (regs, opcode & 7) = ad;
	fpu_timing_save_restore(true, regs.fpu_state != 0);
}

static uaecptr fmovem2mem (uaecptr ad, uae_u32 list, int incr, int regdir)
{
	int reg;

	// 68030 MMU state saving is annoying!
	if (currprefs.mmu_model == 68030) {
#if SUPPORT_MMU
		int idx = 0;
		uae_u32 wrd[3];
		mmu030_state[1] |= MMU030_STATEFLAG1_MOVEM1;
		for (int r = 0; r < 8; r++) {
			if (regdir < 0)
				reg = 7 - r;
			else
				reg = r;
			if (list & 0x80) {
				fpp_from_exten_fmovem(&regs.fp[reg], &wrd[0], &wrd[1], &wrd[2]);
				if (incr < 0)
					ad -= 3 * 4;
				for (int i = 0; i < 3; i++) {
					if (mmu030_state[0] == idx * 3 + i) {
						if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM2) {
							mmu030_state[1] &= ~MMU030_STATEFLAG1_MOVEM2;
						} else {
							mmu030_data_buffer_out = wrd[i];
							x_put_long(ad + i * 4, wrd[i]);
						}
						mmu030_state[0]++;
					}
				}
				if (incr > 0)
					ad += 3 * 4;
				idx++;
			}
			list <<= 1;
		}
#endif
	} else {
		for (int r = 0; r < 8; r++) {
			uae_u32 wrd1, wrd2, wrd3;
			if (regdir < 0)
				reg = 7 - r;
			else
				reg = r;
			if (list & 0x80) {
				fpp_from_exten_fmovem(&regs.fp[reg], &wrd1, &wrd2, &wrd3);
				if (incr < 0)
					ad -= 3 * 4;
				if (regdir < -1 || regdir > 1) {
					x_cp_put_long(ad + 0, wrd3);
					x_cp_put_long(ad + 4, wrd2);
					x_cp_put_long(ad + 8, wrd1);
				} else {
					x_cp_put_long(ad + 0, wrd1);
					x_cp_put_long(ad + 4, wrd2);
					x_cp_put_long(ad + 8, wrd3);
				}
				if (incr > 0)
					ad += 3 * 4;
			}
			list <<= 1;
		}
	}
	return ad;
}

static uaecptr fmovem2fpp (uaecptr ad, uae_u32 list, int incr, int regdir)
{
	int reg;

	if (currprefs.mmu_model == 68030) {
#if SUPPORT_MMU
		uae_u32 wrd[3];
		int idx = 0;
		mmu030_state[1] |= MMU030_STATEFLAG1_MOVEM1 | MMU030_STATEFLAG1_FMOVEM;
		for (int r = 0; r < 8; r++) {
			if (regdir < 0)
				reg = 7 - r;
			else
				reg = r;
			if (list & 0x80) {
				if (incr < 0)
					ad -= 3 * 4;
				for (int i = 0; i < 3; i++) {
					if (mmu030_state[0] == idx * 3 + i) {
						if (mmu030_state[1] & MMU030_STATEFLAG1_MOVEM2) {
							mmu030_state[1] &= ~MMU030_STATEFLAG1_MOVEM2;
							wrd[i] = mmu030_data_buffer_out;
						} else {
							wrd[i] = x_get_long (ad + i * 4);
						}
						// save first two entries if 2nd or 3rd get_long() faults.
						if (i == 0 || i == 1)
							mmu030_fmovem_store[i] = wrd[i];
						mmu030_state[0]++;
						if (i == 2)
							fpp_to_exten (&regs.fp[reg], mmu030_fmovem_store[0], mmu030_fmovem_store[1], wrd[2]);
					}
				}
				if (incr > 0)
					ad += 3 * 4;
				idx++;
			}
			list <<= 1;
		}
#endif
	} else {
		for (int r = 0; r < 8; r++) {
			uae_u32 wrd1, wrd2, wrd3;
			if (regdir < 0)
				reg = 7 - r;
			else
				reg = r;
			if (list & 0x80) {
				if (incr < 0)
					ad -= 3 * 4;
				wrd1 = x_cp_get_long(ad + 0);
				wrd2 = x_cp_get_long(ad + 4);
				wrd3 = x_cp_get_long(ad + 8);
				if (incr > 0)
					ad += 3 * 4;
				fpp_to_exten(&regs.fp[reg], wrd1, wrd2, wrd3);

			}
			list <<= 1;
		}
	}
	return ad;
}

static bool fp_arithmetic(fpdata *src, fpdata *dst, int extra)
{
	uae_u64 q = 0;
	uae_u8 s = 0;

	switch (extra & 0x7f)
	{
		case 0x00: /* FMOVE */
			fpp_move(dst, src, PREC_NORMAL);
			break;
		case 0x40: /* FSMOVE */
			fpp_move(dst, src, PREC_FLOAT);
			break;
		case 0x44: /* FDMOVE */
			fpp_move(dst, src, PREC_DOUBLE);
			break;
		case 0x01: /* FINT */
			fpp_int(dst, src);
			break;
		case 0x02: /* FSINH */
			fpp_sinh(dst, src);
			break;
		case 0x03: /* FINTRZ */
			fpp_intrz(dst, src);
			break;
		case 0x04: /* FSQRT */
		case 0x05:
			fpp_sqrt(dst, src, PREC_NORMAL);
			break;
		case 0x41: /* FSSQRT */
			fpp_sqrt(dst, src, PREC_FLOAT);
			break;
		case 0x45: /* FDSQRT */
			fpp_sqrt(dst, src, PREC_DOUBLE);
			break;
		case 0x06: /* FLOGNP1 */
		case 0x07:
			fpp_lognp1(dst, src);
			break;
		case 0x08: /* FETOXM1 */
			fpp_etoxm1(dst, src);
			break;
		case 0x09: /* FTANH */
			fpp_tanh(dst, src);
			break;
		case 0x0a: /* FATAN */
		case 0x0b:
			fpp_atan(dst, src);
			break;
		case 0x0c: /* FASIN */
			fpp_asin(dst, src);
			break;
		case 0x0d: /* FATANH */
			fpp_atanh(dst, src);
			break;
		case 0x0e: /* FSIN */
			fpp_sin(dst, src);
			break;
		case 0x0f: /* FTAN */
			fpp_tan(dst, src);
			break;
		case 0x10: /* FETOX */
			fpp_etox(dst, src);
			break;
		case 0x11: /* FTWOTOX */
			fpp_twotox(dst, src);
			break;
		case 0x12: /* FTENTOX */
		case 0x13:
			fpp_tentox(dst, src);
			break;
		case 0x14: /* FLOGN */
			fpp_logn(dst, src);
			break;
		case 0x15: /* FLOG10 */
			fpp_log10(dst, src);
			break;
		case 0x16: /* FLOG2 */
		case 0x17:
			fpp_log2(dst, src);
			break;
		case 0x18: /* FABS */
			fpp_abs(dst, src, PREC_NORMAL);
			break;
		case 0x58: /* FSABS */
			fpp_abs(dst, src, PREC_FLOAT);
			break;
		case 0x5c: /* FDABS */
			fpp_abs(dst, src, PREC_DOUBLE);
			break;
		case 0x19: /* FCOSH */
			fpp_cosh(dst, src);
			break;
		case 0x1a: /* FNEG */
		case 0x1b:
			fpp_neg(dst, src, PREC_NORMAL);
			break;
		case 0x5a: /* FSNEG */
			fpp_neg(dst, src, PREC_FLOAT);
			break;
		case 0x5e: /* FDNEG */
			fpp_neg(dst, src, PREC_DOUBLE);
			break;
		case 0x1c: /* FACOS */
			fpp_acos(dst, src);
			break;
		case 0x1d: /* FCOS */
			fpp_cos(dst, src);
			break;
		case 0x1e: /* FGETEXP */
			fpp_getexp(dst, src);
			break;
		case 0x1f: /* FGETMAN */
			fpp_getman(dst, src);
			break;
		case 0x20: /* FDIV */
			fpp_div(dst, src, PREC_NORMAL);
			break;
		case 0x60: /* FSDIV */
			fpp_div(dst, src, PREC_FLOAT);
			break;
		case 0x64: /* FDDIV */
			fpp_div(dst, src, PREC_DOUBLE);
			break;
		case 0x21: /* FMOD */
			fpsr_get_quotient(&q, &s);
			fpp_mod(dst, src, &q, &s);
			fpsr_set_quotient(q, s);
			break;
		case 0x22: /* FADD */
			fpp_add(dst, src, PREC_NORMAL);
			break;
		case 0x62: /* FSADD */
			fpp_add(dst, src, PREC_FLOAT);
			break;
		case 0x66: /* FDADD */
			fpp_add(dst, src, PREC_DOUBLE);
			break;
		case 0x23: /* FMUL */
			fpp_mul(dst, src, PREC_NORMAL);
			break;
		case 0x63: /* FSMUL */
			fpp_mul(dst, src, PREC_FLOAT);
			break;
		case 0x67: /* FDMUL */
			fpp_mul(dst, src, PREC_DOUBLE);
			break;
		case 0x24: /* FSGLDIV */
			fpp_sgldiv(dst, src);
			break;
		case 0x25: /* FREM */
			fpsr_get_quotient(&q, &s);
			fpp_rem(dst, src, &q, &s);
			fpsr_set_quotient(q, s);
			break;
		case 0x26: /* FSCALE */
			fpp_scale(dst, src);
			break;
		case 0x27: /* FSGLMUL */
			fpp_sglmul(dst, src);
			break;
		case 0x28: /* FSUB */
		case 0x29:
		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d:
		case 0x2e:
		case 0x2f:
			fpp_sub(dst, src, PREC_NORMAL);
			break;
		case 0x68: /* FSSUB */
			fpp_sub(dst, src, PREC_FLOAT);
			break;
		case 0x6c: /* FDSUB */
			fpp_sub(dst, src, PREC_DOUBLE);
			break;
		case 0x30: /* FSINCOS */
		case 0x31: /* FSINCOS */
		case 0x32: /* FSINCOS */
		case 0x33: /* FSINCOS */
		case 0x34: /* FSINCOS */
		case 0x35: /* FSINCOS */
		case 0x36: /* FSINCOS */
		case 0x37: /* FSINCOS */
			fpp_sincos(dst, src, &regs.fp[extra & 7]);
			break;
		case 0x38: /* FCMP */
		case 0x39:
		case 0x3c:
		case 0x3d:
		{
			fpp_cmp(dst, src);
			fpsr_make_status();
			fpsr_set_result_all(dst);
			return false;
		}
		case 0x3a: /* FTST */
		case 0x3b:
		case 0x3e:
		case 0x3f:
		{
			fpp_tst(dst, src);
			fpsr_make_status();
			fpsr_set_result_all(dst);
			return false;
		}
		default:
			write_log(_T("Unknown FPU arithmetic function (%02x)\n"), extra & 0x7f);
			return false;
	}

	fpsr_set_result_all(dst);

	if (fpsr_make_status()) {
		return false;
	}

	return true;
}

/* The arithmetic instructions: opclass 0 (FPm to FPn) and 2 (<ea> to FPn),
 * FMOVECR included. Called directly by the FPU entry points, so the most
 * common FPU instructions skip the large fpuop_arithmetic2() frame. */
static void fpuop_arith_regmem(uae_u32 opcode, uae_u16 extra, uaecptr pc, const uaecptr *jit_ea)
{
	int reg;
	int v;
	fpdata src, dst;
	uaecptr ad = 0;
	bool adset = false;
	bool nonmaskable = false;


	reg = (extra >> 7) & 7;
	if ((extra & 0xfc00) == 0x5c00) {
		// FMOVECR
		if (fault_if_unimplemented_680x0 (opcode, extra, ad, adset, pc, &src, reg))
			return;
		if (extra & 0x40) {
			// 6888x and ROM constant 0x40 - 0x7f: f-line
			fpu_noinst (opcode, pc);
			return;
		}
		fpsr_clear_status();
		maybe_set_fpiar(pc);
		fpu_get_constant(&regs.fp[reg], extra & 0x7f);
		fpsr_make_status();
		fpsr_check_arithmetic_exception(0, &src, opcode, extra, ad, adset, pc);
		return;
	}

	// 6888x does not have special exceptions, check immediately
	// 68040+ also generate immediate exception 11 if invalid opmode.
	if (fault_if_nonexisting_opmode(opcode, extra, pc))
		return;

	fpsr_clear_status();

	// 68040 and 68060 always set FPIAR
	if (currprefs.fpu_model == 68040 || currprefs.fpu_model == 68060) {
		regs.fpiar = pc;
	}

	if (!(extra & 0x4000)) {
		// FPm to FPn (as get_fp_value(), without its call)
		if (fault_if_no_fpu (opcode, extra, 0, false, pc))
			return;
		src = regs.fp[(extra >> 10) & 7];
		normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, pc, &src);
	} else {
		v = get_fp_value(opcode, extra, &src, pc, &ad, &adset, jit_ea);
		if (v <= 0) {
			if (v == 0) {
				fpu_noinst(opcode, pc);
			}
			return;
		}
	}

	if (fault_if_no_fpu (opcode, extra, ad, adset, pc))
		return;

	dst = regs.fp[reg];

	if (fp_is_dyadic(extra))
		normalize_or_fault_if_no_denormal_support_dst(opcode, extra, ad, adset, pc, &dst, &src);

	// check for 680x0 unimplemented instruction
	if (fault_if_unimplemented_680x0 (opcode, extra, ad, adset, pc, &src, reg))
		return;

	// unimplemented datatype was checked in get_fp_value
	if (regs.fp_unimp_pend) {
		// simplification: always mid/post-instruction exception
		fp_exception_pending(false);
		return;
	}

	maybe_set_fpiar(pc);

	v = fp_arithmetic(&src, &dst, extra);

	nonmaskable = fpsr_check_arithmetic_exception(0, &src, opcode, extra, ad, adset, pc);

	// 68040 does not update destination register if nonmasked exception was generated
	if (v && (currprefs.fpu_model != 68040 || !nonmaskable)) {
		regs.fp[reg] = dst;
	}

	if (nonmaskable) {
		fp_exception_pending(false);
	}
}

// the common prologue of fpuop_arithmetic2(), for the direct calls of fpuop_arith_regmem()
static inline bool fpuop_arith_regmem_direct(uae_u32 opcode, uae_u16 extra, const uaecptr *jit_ea)
{
	if (((extra >> 13) & 5) != 0) // not opclass 0 or 2
		return false;
	const uaecptr pc = jit_ea ? regs.instruction_pc : m68k_getpc () - 4;
	if (fault_if_no_6888x(opcode, extra, pc))
		return true;
	if (fp_exception_pending(true))
		return true;
	fpuop_arith_regmem(opcode, extra, pc, jit_ea);
	return true;
}

// not inlined into the entry points: its frame is what fpuop_arith_regmem() avoids
static NOINLINE void fpuop_arithmetic2 (uae_u32 opcode, uae_u16 extra, const uaecptr *jit_ea = nullptr)
{
	int reg = -1;
	int v;
	fpdata src, dst;
	uaecptr pc = jit_ea ? regs.instruction_pc : m68k_getpc () - 4;
	uaecptr ad = 0;
	bool adset = false;
	bool nonmaskable = false;

#if DEBUG_FPP
	if (!isinrom ())
		write_log(_T("FPP %04x %04x at %08x\n"), opcode & 0xffff, extra, pc);
#endif
	if (fault_if_no_6888x(opcode, extra, pc))
		return;

	if (fp_exception_pending(true))
		return;

	switch ((extra >> 13) & 0x7)
	{
		case 3:
			// FMOVE FPP->EA
			fpsr_clear_status();
			src = regs.fp[(extra >> 7) & 7];
			v = put_fp_value(&src, opcode, extra, pc, &ad, &adset, jit_ea);
			if (v <= 0) {
				if (v == 0) {
					fpu_noinst(opcode, pc);
				}
				return;
			}
			fpsr_make_status();
			maybe_set_fpiar(pc);
			fpsr_check_arithmetic_exception(0, &src, opcode, extra, ad, adset, pc);
			fp_exception_pending(false); // post/mid instruction
			return;

		case 4:
		case 5:
			// FMOVE(M) Control Register(s) <> Data or Address register
			if ((opcode & 0x38) == 0) {
				// FMOVE(M) Control Register(s) <> Dn
				if (fault_if_no_fpu (opcode, extra, 0, false, pc))
					return;
				// Only single selected control register is allowed
				// All control register bits unset = FPIAR
				uae_u16 bits = extra & (0x1000 | 0x0800 | 0x0400);
				if (bits && bits != 0x1000 && bits != 0x0800 && bits != 0x400) {
					// 68060 does not generate f-line if multiple bits are set
					// but it also works unexpectedly, just do nothing for now.
					if (currprefs.fpu_model != 68060) {
						fpu_noinst(opcode, pc);
					}
#ifdef CPU_TESTER
					if (currprefs.fpu_model == 68060) {
						cputester_fault();
					}
#endif
					return;
				}
				if (extra & 0x2000) {
					// CR -> Dn
					if (extra & 0x1000)
						m68k_dreg(regs, opcode & 7) = fpp_get_fpcr();
					if (extra & 0x0800)
						m68k_dreg(regs, opcode & 7) = fpp_get_fpsr();
					if ((extra & 0x0400) || !bits)
						m68k_dreg(regs, opcode & 7) = fpp_get_fpiar();
				} else {
					// Dn -> CR
					if (extra & 0x1000)
						fpp_set_fpcr(m68k_dreg (regs, opcode & 7));
					if (extra & 0x0800)
						fpp_set_fpsr(m68k_dreg (regs, opcode & 7));
					if ((extra & 0x0400) || !bits)
						fpp_set_fpiar(m68k_dreg (regs, opcode & 7));
				}
			} else if ((opcode & 0x38) == 0x08) {
				// FMOVE(M) Control Register(s) <> An
				if (fault_if_no_fpu (opcode, extra, 0, false, pc))
					return;
				// Only FPIAR can be moved to/from address register
				// All bits unset = FPIAR
				uae_u16 bits = extra & (0x1000 | 0x0800 | 0x0400);
				if (bits && bits != 0x0400 && currprefs.fpu_model == 68060) {
					// 68060 does not generate f-line if multiple bits are set
					// but it also works unexpectedly, just do nothing for now.
#ifdef CPU_TESTER
					cputester_fault();
#endif
					return;
				}
				// 68060, An and all bits unset: f-line
				if ((bits && bits != 0x0400) || (!bits && currprefs.fpu_model == 68060)) {
					fpu_noinst(opcode, pc);
					return;
				}
				if (extra & 0x2000) {
					// FPIAR -> An
					m68k_areg (regs, opcode & 7) = regs.fpiar;
				} else {
					// An -> FPIAR
					regs.fpiar = m68k_areg (regs, opcode & 7);
				}
			} else if ((opcode & 0x3f) == 0x3c) {
				if ((extra & 0x2000) == 0) {
					// FMOVE(M) #imm,Control Register(s)
					uae_u32 ext[3];
					// 68060 FMOVEM.L #imm,more than 1 control register: unimplemented EA
					uae_u16 bits = extra & (0x1000 | 0x0800 | 0x0400);
					if (bits && bits != 0x1000 && bits != 0x0800 && bits != 0x400) {
						if (fault_if_60())
							return;
					}
					// No control register bits set: FPIAR
					if (!bits) {
						extra |= 0x0400;
					}
					// fetch first, use only after all data has been fetched
					ext[0] = ext[1] = ext[2] = 0;
					if (extra & 0x1000)
						ext[0] = x_cp_next_ilong ();
					if (extra & 0x0800)
						ext[1] = x_cp_next_ilong ();
					if (extra & 0x0400)
						ext[2] = x_cp_next_ilong ();
					if (fault_if_no_fpu (opcode, extra, 0, false, pc))
						return;
					if (extra & 0x1000)
						fpp_set_fpcr(ext[0]);
					if (extra & 0x0800)
						fpp_set_fpsr(ext[1]);
					if (extra & 0x0400)
						fpp_set_fpiar(ext[2]);
				} else {
					// immediate as destination
					fpu_noinst(opcode, pc);
					return;
				}
			} else if (extra & 0x2000) {
				/* FMOVE(M) Control Register(s),EA */
				uae_u32 ad;
				int incr = 0;

				if (get_fp_ad(opcode, &ad, &adset, jit_ea) == 0) {
					fpu_noinst(opcode, pc);
					return;
				}
				if (fault_if_no_fpu(opcode, extra, ad, adset, pc))
					return;
				if ((opcode & 0x3f) >= 0x3a) {
					// PC relative modes not supported
					fpu_noinst(opcode, pc);
					return;
				}

				// No control register bits set: FPIAR
				if (!(extra & (0x1000 | 0x0800 | 0x0400))) {
					extra |= 0x0400;
				}

				if ((opcode & 0x38) == 0x20) {
					if (extra & 0x1000)
						incr += 4;
					if (extra & 0x0800)
						incr += 4;
					if (extra & 0x0400)
						incr += 4;
				}
				ad -= incr;
				if (extra & 0x1000) {
					x_cp_put_long(ad, fpp_get_fpcr());
					ad += 4;
				}
				if (extra & 0x0800) {
					x_cp_put_long(ad, fpp_get_fpsr());
					ad += 4;
				}
				if (extra & 0x0400) {
					x_cp_put_long(ad, fpp_get_fpiar());
					ad += 4;
				}
				ad -= incr;
				if ((opcode & 0x38) == 0x18) {
					// (An)+
					m68k_areg(regs, opcode & 7) = ad;
				}
				if ((opcode & 0x38) == 0x20) {
					// -(An)
					m68k_areg(regs, opcode & 7) = ad;
				}
				trace_t0_68040();
			} else {
				/* FMOVE(M) EA,Control Register(s) */
				uae_u32 ad;
				int incr = 0;

				if (get_fp_ad (opcode, &ad, &adset, jit_ea) == 0) {
					fpu_noinst (opcode, pc);
					return;
				}
				if (fault_if_no_fpu (opcode, extra, ad, adset, pc))
					return;

				// No control register bits set: FPIAR
				if (!(extra & (0x1000 | 0x0800 | 0x0400))) {
					extra |= 0x0400;
				}

				// -(An)
				if((opcode & 0x38) == 0x20) {
					if (extra & 0x1000)
						incr += 4;
					if (extra & 0x0800)
						incr += 4;
					if (extra & 0x0400)
						incr += 4;
					ad = ad - incr;
				}
				if (extra & 0x1000) {
					fpp_set_fpcr(x_cp_get_long (ad));
					ad += 4;
				}
				if (extra & 0x0800) {
					fpp_set_fpsr(x_cp_get_long (ad));
					ad += 4;
				}
				if (extra & 0x0400) {
					fpp_set_fpiar(x_cp_get_long (ad));
					ad += 4;
				}
				if ((opcode & 0x38) == 0x18) {
					// (An)+
					m68k_areg(regs, opcode & 7) = ad;
				}
				if ((opcode & 0x38) == 0x20) {
					// -(An)
					m68k_areg(regs, opcode & 7) = ad - incr;
				}
			}
			return;

		case 6:
		case 7:
			{
				// FMOVEM FPP<>Memory
				uae_u32 ad, list = 0;
				int incr = 1;
				int regdir = 1;
				if (get_fp_ad (opcode, &ad, &adset, jit_ea) == 0) {
#ifdef CPU_TESTER
					// 68060 does weird things (no exception) if FMOVEM.X #imm to registers
					if ((opcode & 0x3f) == 0x3c && !(extra & 0x2000) && currprefs.fpu_model == 68060) {
						cputester_fault();
						return;
					}
#endif
					fpu_noinst(opcode, pc);
					return;
				}
				if (fault_if_no_fpu (opcode, extra, ad, adset, pc))
					return;

				if ((extra & 0x2000) && ((opcode & 0x38) == 0x18 || (opcode & 0x3f) >= 0x3a)) {
					// FMOVEM FPP->Memory: (An)+ and PC relative modes not supported
					fpu_noinst(opcode, pc);
					return;
				}
				if (!(extra & 0x2000) && (opcode & 0x38) == 0x20) {
					// FMOVEM Memory->FPP: -(An) not supported
					fpu_noinst(opcode, pc);
					return;
				}

				int mode = (extra >> 11) & 3;
				switch (mode)
				{
					case 0:	/* static pred */
					case 2:	/* static postinc */
						list = extra & 0xff;
						break;
					case 1:	/* dynamic pred */
					case 3:	/* dynamic postinc */
						if (fault_if_60())
							return;
						list = m68k_dreg (regs, (extra >> 4) & 7) & 0xff;
						break;
				}

				if (currprefs.fpu_model >= 68881) {
					// 6888x works mostly as documented
					if ((opcode & 0x38) == 0x20) {
						incr = -1;
					}
					switch (mode)
					{
					case 0:	/* static pred */
					case 1:	/* dynamic pred */
						regdir = -1;
						break;
					}
				} else if (currprefs.fpu_model == 68040) {
					// 68040 is weird..
					if ((opcode & 0x38) == 0x20) {
						incr = -1;
					}
					switch (mode)
					{
					case 0:	/* static pred */
						if ((opcode & 0x38) == 0x20 || (extra & 0x2000)) {
							regdir = -1;
						}
						break;
					case 1:	/* dynamic pred */
						if ((opcode & 0x38) == 0x20 || (extra & 0x2000)) {
							regdir = -1;
						}
						break;
					}
					// FMOVEM x,any EA that is not -(An): reversed write order.
					// (low mantissa, high mantissa, exponent)!
					if (list && (extra & 0x2000) && regdir < 0 && incr > 0 && (opcode & 0x38) != 0x20) {
						regdir = -2;
					}
					// FMOVEM x,-(An) but postinc: also reversed.
					if (list && (extra & 0x2000) && regdir > 0 && incr < 0) {
						regdir = 2;
					}
					// 68040 hangs if FMOVEM control registers
					// has undefined bit 10 set.
				} else {
					// 68060 simply ignores MODE field completely.
					if ((opcode & 0x38) == 0x20) {
						incr = -1;
						regdir = -1;
					}
				}

				if (extra & 0x2000) {
					/* FMOVEM FPP->Memory */
					ad = fmovem2mem(ad, list, incr, regdir);
					trace_t0_68040();
				} else {
					/* FMOVEM Memory->FPP */
					ad = fmovem2fpp(ad, list, incr, regdir);
				}
				if ((opcode & 0x38) == 0x18 || (opcode & 0x38) == 0x20) {
					// (an)+ or -(an)
					m68k_areg(regs, opcode & 7) = ad;
				}
			}
			return;

		case 0:
		case 2: /* Extremely common */
			fpuop_arith_regmem(opcode, extra, pc, jit_ea);
			return;
		default:
		break;
	}
	fpu_noinst (opcode, pc);
}

void fpuop_arithmetic (uae_u32 opcode, uae_u16 extra)
{
#ifdef JIT
	if (currprefs.cachesize && fp_jit_stats())
		fp_jit_stats_count(opcode, extra, FPJ_ST_INTERP);
#endif
	regs.fpu_state = 1;
	regs.fp_exception = false;
	fpu_mmu_fixup = false;
	fpu_timing_begin();
	fpu_timing_store_wait(extra);
#if FPU_LOG
	write_log(_T("FPUOP %04x %04x PC=%08x\n"), opcode, extra, M68K_GETPC);
#endif
	if (!fpuop_arith_regmem_direct(opcode, extra, nullptr))
		fpuop_arithmetic2 (opcode, extra);
	if (fpu_mmu_fixup) {
		mmufixup[0].reg = -1;
	}
	if (!regs.fp_exception) {
		fpu_timing_arithmetic(opcode, extra);
	}
}

#ifdef JIT
/* ---- compiled FPU arithmetic for the JIT when it does not compile FPU
 * instructions natively (the exact core): opclass 0 (FPm to FPn) and 2
 * (<ea> to FPn) become a call of fpp_jit_arith(), without the interpreter's
 * decoding. jit_fpu_arith_ok() at compile time says which. */

static bool jit_fpu_arith_opmode(uae_u16 extra)
{
	const uae_u16 v = extra & 0x7f;
	if (v < 0x40) {
		switch (v) {
		case 0x05: case 0x07: case 0x0b: case 0x13: case 0x17: case 0x1b:
		case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
		case 0x39: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			return false; // undocumented opmodes: the interpreter
		default:
			return true;
		}
	}
	if (currprefs.fpu_model != 68040 && currprefs.fpu_model != 68060)
		return false;
	switch (v) { // FSxxx/FDxxx
	case 0x40: case 0x41: case 0x44: case 0x45: case 0x58: case 0x5a: case 0x5c: case 0x5e:
	case 0x60: case 0x62: case 0x63: case 0x64: case 0x66: case 0x67: case 0x68: case 0x6c:
		return true;
	default:
		return false;
	}
}

bool jit_fpu_arith_ok(uae_u16 opcode, uae_u16 extra)
{
	if (jit_fpu_enabled() || !currprefs.fpu_model || currprefs.mmu_model)
		return false;
	// 68040/68060 without the emulated FPSP: unimplemented instructions and data types trap
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_no_unimplemented)
		return false;
	if (((extra >> 13) & 5) != 0) // opclass 0 or 2 only
		return false;
	if ((extra & 0xfc00) == 0x5c00) // FMOVECR
		return false;
	if (!jit_fpu_arith_opmode(extra))
		return false;
	if (!(extra & 0x4000))
		return true;
	const int mode = (opcode >> 3) & 7, reg = opcode & 7, size = (extra >> 10) & 7;
	if (size == 3 || size == 7) // packed, undefined
		return false;
	if (mode == 0) // Dn: .L .S .W .B
		return size == 0 || size == 1 || size == 4 || size == 6;
	if (mode == 1)
		return false;
	if (mode == 7) {
		if (reg == 3 || reg > 4) // (d8,PC,Xn) and invalid modes
			return false;
		if (reg == 4 && size == 2) // #immediate .X
			return false;
	}
	return true;
}

// the interpreter handler for the instruction, with the PC state it expects
static NOINLINE uae_u32 fpp_jit_arith_slow(uae_u16 opcode, uae_u8 *pc_p, uae_u32 pc)
{
	uae_u8 *const save_pc_p = regs.pc_p, *const save_pc_oldp = regs.pc_oldp;
	const uae_u32 save_pc = regs.pc, save_ipc = regs.instruction_pc;
	regs.pc_p = regs.pc_oldp = pc_p;
	regs.pc = regs.instruction_pc = pc;
	(*cpufunctbl[opcode])(opcode);
	if (regs.spcflags || regs.fp_exception)
		return 2; // the compiled code leaves the block: the PC is the interpreter's
	regs.pc_p = save_pc_p;
	regs.pc_oldp = save_pc_oldp;
	regs.pc = save_pc;
	regs.instruction_pc = save_ipc;
	return 1;
}

/* Any FPU general instruction (opclass 3: FMOVE to <ea>) with the effective
 * address computed by the compiled code: the interpreter's handling via
 * jit_ea, no decoding of the instruction stream. lenpc: instruction length
 * << 32 | its address; next_p: the host pointer after it. An (An)+/-(An)
 * update is done here. Returns 1, or 2: leave the block (exception or
 * special flags). All 68k registers are in memory. */
uae_u32 REGPARAM3 fpp_jit_generic(uae_u32 opext, uae_u64 ea, uae_u8 *next_p, uae_u64 lenpc)
{
	const uae_u16 opcode = opext >> 16;
	const uae_u16 extra = opext;
	const uae_u32 pc = (uae_u32)lenpc;
	uae_u8 *const save_pc_p = regs.pc_p, *const save_pc_oldp = regs.pc_oldp;
	const uae_u32 save_pc = regs.pc, save_ipc = regs.instruction_pc;
	regs.pc_p = regs.pc_oldp = next_p;
	regs.pc = pc + (uae_u32)(lenpc >> 32);
	regs.instruction_pc = pc;
	const uaecptr jit_ea = (uaecptr)ea;
	regs.fpu_state = 1;
	regs.fp_exception = false;
	fpu_mmu_fixup = false;
	if (!fpuop_arith_regmem_direct(opcode, extra, &jit_ea))
		fpuop_arithmetic2(opcode, extra, &jit_ea);
	if (fpu_mmu_fixup)
		mmufixup[0].reg = -1;
	if (regs.spcflags || regs.fp_exception)
		return 2;
	regs.pc_p = save_pc_p;
	regs.pc_oldp = save_pc_oldp;
	regs.pc = save_pc;
	regs.instruction_pc = save_ipc;
	return 1;
}

/* FMOVEM FPn<->memory with a static register list, as fpuop_arithmetic2()
 * does it (the same incr and regdir choice, quirks included): the transfers
 * in order, as (register, byte offset from the effective address), whether
 * the words go in reverse order, and the offset (An)+ or -(An) ends at.
 * False: not that case (a dynamic list, an invalid mode). For the JIT's
 * inline FMOVEM (compemu_midfunc_arm64.cpp fpx_emit_fmovem()). */
bool fp_jit_fmovem_plan(uae_u16 opcode, uae_u16 extra, struct fpj_fmovem *p)
{
	if (((extra >> 13) & 6) != 6)
		return false;
	const int mode = (extra >> 11) & 3;
	if (mode == 1 || mode == 3) // dynamic list
		return false;
	if ((extra & 0x2000) && ((opcode & 0x38) == 0x18 || (opcode & 0x3f) >= 0x3a))
		return false;
	if (!(extra & 0x2000) && (opcode & 0x38) == 0x20)
		return false;
	uae_u32 list = extra & 0xff;
	int incr = 1, regdir = 1;
	if (currprefs.fpu_model >= 68881) {
		if ((opcode & 0x38) == 0x20)
			incr = -1;
		if (mode == 0)
			regdir = -1;
	} else if (currprefs.fpu_model == 68040) {
		if ((opcode & 0x38) == 0x20)
			incr = -1;
		if (mode == 0 && ((opcode & 0x38) == 0x20 || (extra & 0x2000)))
			regdir = -1;
		if (list && (extra & 0x2000) && regdir < 0 && incr > 0 && (opcode & 0x38) != 0x20)
			regdir = -2;
		if (list && (extra & 0x2000) && regdir > 0 && incr < 0)
			regdir = 2;
	} else {
		if ((opcode & 0x38) == 0x20) {
			incr = -1;
			regdir = -1;
		}
	}
	p->to_mem = (extra & 0x2000) != 0;
	p->reversed = p->to_mem && (regdir < -1 || regdir > 1);
	p->n = 0;
	int ad = 0;
	for (int r = 0; r < 8; r++) {
		const int reg = regdir < 0 ? 7 - r : r;
		if (list & 0x80) {
			if (incr < 0)
				ad -= 12;
			p->reg[p->n] = reg;
			p->off[p->n] = ad;
			p->n++;
			if (incr > 0)
				ad += 12;
		}
		list <<= 1;
	}
	p->final = ad;
	return true;
}

bool jit_fpu_generic_ok(uae_u16 opcode, uae_u16 extra)
{
	if (jit_fpu_enabled() || !currprefs.fpu_model || currprefs.mmu_model)
		return false;
	if (currprefs.cpu_model >= 68040 && currprefs.fpu_no_unimplemented)
		return false;
	const int opclass = (extra >> 13) & 7, mode = (opcode >> 3) & 7, reg = opcode & 7;
	if (opclass >= 4) {
		// FMOVE(M) of control registers (4, 5) and FMOVEM (6, 7). The
		// compiled code passes An itself for (An)+ and -(An): the handler
		// steps it. PC-relative and immediate sources read the instruction
		// stream, which the call does not give it.
		if (mode == 7 && reg > 1)
			return false;
		if (mode <= 1)
			return opclass <= 5;
		return true;
	}
	if (opclass != 3) // FMOVE to <ea>
		return false;
	const int size = (extra >> 10) & 7;
	if (size == 3 || size == 7) // packed with a static/dynamic k factor
		return false;
	if (mode == 1)
		return false;
	if (mode == 0)
		return size == 0 || size == 1 || size == 4 || size == 6;
	if (mode == 7 && reg > 1) // (d16,PC) etc. are not alterable
		return false;
	return true;
}

/* opext: opcode << 16 | extension word. value: the source operand the
 * compiled code loaded (.L .W .B .S: low 32 bits; .D: both longs; .X: its
 * address); not used for an FPm or Dn source. pc_p, pc: the instruction.
 * Returns 0: done, the compiled code applies a (An)+/-(An) update; 1: the
 * interpreter handler did it all; 2: as 1, and the compiled code must leave
 * the block (an exception, or special flags). All 68k registers are in
 * memory. */
uae_u32 REGPARAM3 fpp_jit_arith(uae_u32 opext, uae_u64 value, uae_u8 *pc_p, uae_u32 pc)
{
	const uae_u16 opcode = opext >> 16;
	const uae_u16 extra = opext;
	if ((regs.fpcr & 0xff00) || regs.fp_exp_pend || regs.fp_unimp_pend || if_no_fpu())
		return fpp_jit_arith_slow(opcode, pc_p, pc); // exceptions possible: as the interpreter does
	fpdata src, dst;
	const int reg = (extra >> 7) & 7;
	regs.fpu_state = 1;
	regs.fp_exception = false;
	fpsr_clear_status();
	if (currprefs.fpu_model == 68040 || currprefs.fpu_model == 68060)
		regs.fpiar = pc;
	const fpdata *srcp = &src;
	if (!(extra & 0x4000)) {
		srcp = &regs.fp[(extra >> 10) & 7];
	} else {
		const uae_u32 v = ((opcode >> 3) & 7) == 0 ? m68k_dreg(regs, opcode & 7) : (uae_u32)value;
		switch ((extra >> 10) & 7) {
		case 0:
			fpset(&src, (uae_s32)v);
			break;
		case 1:
			fpp_to_single(&src, v);
			break;
		case 4:
			fpset(&src, (uae_s16)v);
			break;
		case 6:
			fpset(&src, (uae_s8)v);
			break;
		case 5:
			fpp_to_double(&src, (uae_u32)(value >> 32), (uae_u32)value);
			break;
		default: { // 2: .X from memory
			const uaecptr ad = (uaecptr)value;
			fpp_to_exten(&src, x_cp_get_long(ad), x_cp_get_long(ad + 4), x_cp_get_long(ad + 8));
			break;
		}
		}
	}
	// the softfloat cores (the exact core): the common instructions in place
	if (fpu_softfloat_core() && fp_exact_fast_arith(&regs.fp[reg], srcp, extra))
		return 0;
	if (srcp != &src)
		src = *srcp;
	normalize_or_fault_if_no_denormal_support(opcode, extra, 0, false, pc, &src);
	dst = regs.fp[reg];
	if (fp_is_dyadic(extra))
		normalize_or_fault_if_no_denormal_support_dst(opcode, extra, 0, false, pc, &dst, &src);
	maybe_set_fpiar(pc);
	if (fp_arithmetic(&src, &dst, extra))
		regs.fp[reg] = dst;
	return 0;
}
#endif

void REGPARAM3 fpuop_arithmetic_jit(uae_u32 instruction, uaecptr ea)
{
	const uae_u16 opcode = instruction >> 16;
	const uae_u16 extra = instruction;
	m68k_setpc_j(regs.pc);
	regs.fpu_state = 1;
	regs.fp_exception = false;
	fpu_mmu_fixup = false;
	if (!fpuop_arith_regmem_direct(opcode, extra, &ea))
		fpuop_arithmetic2(opcode, extra, &ea);
	if (fpu_mmu_fixup)
		mmufixup[0].reg = -1;
	// The emitted predicates need an x87-representable CC surrogate, not
	// Motorola extended infinity/unnormal encodings from the raw registers.
	if (!regs.fp_exception)
		fpp_set_fpsr(regs.fpsr);
}

static void get_features(void)
{
	support_exceptions = (fpp_get_support_flags() & FPU_FEATURE_EXCEPTIONS) != 0;
	support_denormals = (fpp_get_support_flags() & FPU_FEATURE_DENORMALS) != 0;
	if (currprefs.fpu_model == 68040 || currprefs.fpu_model == 68060) {
		condition_table = condition_table_040_060;
	} else {
		condition_table = condition_table_6888x;
	}
	fpsr_mask = 0x0ffffff8;
	if (currprefs.fpu_model == 68040) {
		fpcr_mask = 0xffff;
	} else {
		fpcr_mask = 0xfff0;
	}
}

void fpu_clearstatus(void)
{
	fpp_clear_status();
}

#ifndef CPU_TESTER

void fpu_modechange(void)
{
	uae_u32 temp_ext[8][3];

	currprefs.fpu_mode = changed_prefs.fpu_mode;
	// also a JIT setting can change what Automatic picks
	if (fpu_core_wanted() == fpu_core_active)
		return;

#ifdef JIT
	// Compiled code is for the old core: host FPU instructions on host
	// values in regs.fp, or calls into the exact core on 80-bit ones. Throw
	// it away (a cache state change flushes hard; set_cpu_caches() restores
	// the state from the CACR).
	if (currprefs.cachesize)
		set_cache_state(0);
#endif
	set_cpu_caches(true);
	// the condition codes as the old core keeps them (the host core with the
	// JIT FPU computes them from regs.fp_result when read)
	regs.fpsr = fpp_get_fpsr();
	for (int i = 0; i < 8; i++) {
		fpp_from_exten_fmovem(&regs.fp[i], &temp_ext[i][0], &temp_ext[i][1], &temp_ext[i][2]);
	}
	fpu_select_core();
	get_features();
	for (int i = 0; i < 8; i++) {
		fpp_to_exten_fmovem(&regs.fp[i], temp_ext[i][0], temp_ext[i][1], temp_ext[i][2]);
	}
	// regs.fp_result is the old core's: the FPSR holds the condition codes
	fpu_fpsr_cc_written = true;
}

#endif

#if FPU_TEST

static void fpu_test(void)
{
	fpdata testp;
	uae_u32 packed[3];

	fpp_set_fpcr(0x30);
	fpp_to_exten_fmovem(&testp, 0xB4000000, 0x80000000, 0x000003fc);
	write_log(_T("INPUT: %s (%04x %16llx)\n"), fpp_print(&testp, -1), testp.fpx.high, testp.fpx.low);
	fpp_from_pack(&testp, packed, 17);
	fpp_to_pack(&testp, packed, 0);
}

#endif

void fpu_reset (void)
{
#ifndef CPU_TESTER
	currprefs.fpu_mode = changed_prefs.fpu_mode;
	fpu_select_core();

#if defined(CPU_x86_64)
#ifndef __MACH__
#ifndef __ANDROID__
	init_fpucw_x87();
#endif
#endif

#ifdef MSVC_LONG_DOUBLE
	init_fpucw_x87_80();
#endif
#endif
#else
	fp_init_softfloat(currprefs.fpu_model);
	use_long_double = false;
#endif

	regs.fpu_exp_state = 0;
	regs.fp_unimp_pend = 0;
	regs.fp_ea_set = false;
#ifndef CPU_TESTER
	fpu_busy_until = 0;
#endif
	get_features();
	fpp_set_fpcr (0);
	fpp_set_fpsr (0);
	fpp_set_fpiar (0);
#ifndef AMIBERRY
	fpux_restore (NULL);
#endif
	// reset precision
	fpp_set_mode(0x00000080 | 0x00000010);
	fpp_set_mode(0x00000000);

#if FPU_TEST
	fpu_test();
#endif

}

#ifndef CPU_TESTER

uae_u8 *restore_fpu (uae_u8 *src)
{
	uae_u32 w1, w2, w3;
	int i;
	uae_u32 flags;

	changed_prefs.fpu_model = currprefs.fpu_model = restore_u32 ();
	fpu_reset();
	flags = restore_u32 ();
	for (i = 0; i < 8; i++) {
		w1 = restore_u16 () << 16;
		w2 = restore_u32 ();
		w3 = restore_u32 ();
		fpp_to_exten_fmovem(&regs.fp[i], w1, w2, w3);
	}
	regs.fpcr = restore_u32 ();
	regs.fpsr = restore_u32 ();
	regs.fpiar = restore_u32 ();
	regs.fp_ea_set = (flags & 0x00000001) != 0;
	fpsr_make_status();
	if (flags & 0x80000000) {
		restore_u32 ();
		restore_u32 ();
	}
	if (flags & 0x20000000) {
		uae_u32 v = restore_u32();
		regs.fpu_state = (v >> 0) & 15;
		regs.fpu_exp_state = (v >> 4) & 15;
		regs.fp_unimp_pend = (v >> 8) & 15;
		regs.fp_exp_pend = (v >> 16) & 0xff;
		regs.fp_opword = restore_u16();
		regs.fp_ea = restore_u32();
		if (currprefs.fpu_model == 68060 || currprefs.fpu_model >= 68881) {
			fsave_data.ccr = restore_u32();
			fsave_data.eo[0] = restore_u32();
			fsave_data.eo[1] = restore_u32();
			fsave_data.eo[2] = restore_u32();
		}
		if (currprefs.fpu_model == 68060) {
			fsave_data.v = restore_u32();
		}
		if (currprefs.fpu_model == 68040) {
			fsave_data.fpiarcu = restore_u32();
			fsave_data.cmdreg3b = restore_u32();
			fsave_data.cmdreg1b = restore_u32();
			fsave_data.stag = restore_u32();
			fsave_data.dtag = restore_u32();
			fsave_data.e1 = restore_u32();
			fsave_data.e3 = restore_u32();
			fsave_data.t = restore_u32();
			fsave_data.fpt[0] = restore_u32();
			fsave_data.fpt[1] = restore_u32();
			fsave_data.fpt[2] = restore_u32();
			fsave_data.et[0] = restore_u32();
			fsave_data.et[1] = restore_u32();
			fsave_data.et[2] = restore_u32();
			fsave_data.wbt[0] = restore_u32();
			fsave_data.wbt[1] = restore_u32();
			fsave_data.wbt[2] = restore_u32();
			fsave_data.grs = restore_u32();
			fsave_data.wbte15 = restore_u32();
			fsave_data.wbtm66 = restore_u32();
		}
	}
	write_log(_T("FPU: %d\n"), currprefs.fpu_model);
	return src;
}

uae_u8 *save_fpu(size_t *len, uae_u8 *dstptr)
{
	uae_u32 w1, w2, w3, v;
	uae_u8 *dstbak, *dst;
	int i;

	*len = 0;
	if (currprefs.fpu_model == 0)
		return 0;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc(uae_u8, 4 + 4 + 8 * 10 + 6 * 4 + 2 + 6 * 4 + 20 * 4);
	save_u32 (currprefs.fpu_model);
	save_u32 (0x80000000 | 0x20000000 | (regs.fp_ea_set ? 0x00000001 : 0x00000000));
	for (i = 0; i < 8; i++) {
		fpp_from_exten_fmovem(&regs.fp[i], &w1, &w2, &w3);
		save_u16 (w1 >> 16);
		save_u32 (w2);
		save_u32 (w3);
	}
	save_u32 (regs.fpcr);
	save_u32 (regs.fpsr);
	save_u32 (regs.fpiar);

	save_u32 (-1);
	save_u32 (1);

	v = regs.fpu_state;
	v |= regs.fpu_exp_state << 4;
	v |= regs.fp_unimp_pend << 8;
	v |= regs.fp_exp_pend << 16;
	save_u32(v);
	save_u16(regs.fp_opword);
	save_u32(regs.fp_ea);

	if (currprefs.fpu_model == 68060 || currprefs.fpu_model >= 68881) {
		save_u32(fsave_data.ccr);
		save_u32(fsave_data.eo[0]);
		save_u32(fsave_data.eo[1]);
		save_u32(fsave_data.eo[2]);
	}
	if (currprefs.fpu_model == 68060) {
		save_u32(fsave_data.v);
	}
	if (currprefs.fpu_model == 68040) {
		save_u32(fsave_data.fpiarcu);
		save_u32(fsave_data.cmdreg3b);
		save_u32(fsave_data.cmdreg1b);
		save_u32(fsave_data.stag);
		save_u32(fsave_data.dtag);
		save_u32(fsave_data.e1);
		save_u32(fsave_data.e3);
		save_u32(fsave_data.t);
		save_u32(fsave_data.fpt[0]);
		save_u32(fsave_data.fpt[1]);
		save_u32(fsave_data.fpt[2]);
		save_u32(fsave_data.et[0]);
		save_u32(fsave_data.et[1]);
		save_u32(fsave_data.et[2]);
		save_u32(fsave_data.wbt[0]);
		save_u32(fsave_data.wbt[1]);
		save_u32(fsave_data.wbt[2]);
		save_u32(fsave_data.grs);
		save_u32(fsave_data.wbte15);
		save_u32(fsave_data.wbtm66);
	}

	*len = dst - dstbak;
	return dstbak;
}

#endif
