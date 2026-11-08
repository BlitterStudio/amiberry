/*
* UAE - The Un*x Amiga Emulator
*
* MC68881/68882/68040/68060 FPU emulation
* Softfloat version
*
* Andreas Grabher and Toni Wilen
*
*/
#define __USE_ISOC9X  /* We might be able to pick up a NaN */

#define SOFTFLOAT_FAST_INT64

#include <math.h>
#include <float.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "memory.h"
#include "newcpu.h"
#include "fpp.h"

#include "softfloat/softfloat-macros.h"
#include "softfloat/softfloat-specialize.h"
#include "softfloat/softfloat_dd.h"
#include "softfloat/softfloat_fastx80.h"
#include <fenv.h>

#define	FPCR_ROUNDING_MODE	0x00000030
#define	FPCR_ROUND_NEAR		0x00000000
#define	FPCR_ROUND_ZERO		0x00000010
#define	FPCR_ROUND_MINF		0x00000020
#define	FPCR_ROUND_PINF		0x00000030

#define	FPCR_ROUNDING_PRECISION	0x000000c0
#define	FPCR_PRECISION_SINGLE	0x00000040
#define	FPCR_PRECISION_DOUBLE	0x00000080
#define FPCR_PRECISION_EXTENDED	0x00000000

static struct float_status fs;

/* Functions for setting host/library modes and getting status */
static void fp_set_mode(uae_u32 mode_control)
{
	set_float_detect_tininess(float_tininess_before_rounding, &fs);

	switch(mode_control & FPCR_ROUNDING_PRECISION) {
		case FPCR_PRECISION_SINGLE: // single
			set_floatx80_rounding_precision(32, &fs);
			break;
		default: // double
		case FPCR_PRECISION_DOUBLE: // double
			set_floatx80_rounding_precision(64, &fs);
			break;
		case FPCR_PRECISION_EXTENDED: // extended
			set_floatx80_rounding_precision(80, &fs);
			break;
	}
	
	switch(mode_control & FPCR_ROUNDING_MODE) {
		case FPCR_ROUND_NEAR: // to neareset
			set_float_rounding_mode(float_round_nearest_even, &fs);
			break;
		case FPCR_ROUND_ZERO: // to zero
			set_float_rounding_mode(float_round_to_zero, &fs);
			break;
		case FPCR_ROUND_MINF: // to minus
			set_float_rounding_mode(float_round_down, &fs);
			break;
		case FPCR_ROUND_PINF: // to plus
			set_float_rounding_mode(float_round_up, &fs);
			break;
	}
}

// FPSR exception status bits for each value of fs.float_exception_flags
static uae_u32 fp_status_bits[256];

static void fp_init_status_bits(void)
{
	for (int f = 0; f < 256; f++) {
		uae_u32 s = 0;
		if (f & float_flag_signaling)
			s |= FPSR_SNAN;
		if (f & float_flag_invalid)
			s |= FPSR_OPERR;
		if (f & float_flag_divbyzero)
			s |= FPSR_DZ;
		if (f & float_flag_overflow)
			s |= FPSR_OVFL;
		if (f & float_flag_underflow)
			s |= FPSR_UNFL;
		if (f & float_flag_inexact)
			s |= FPSR_INEX2;
		if (f & float_flag_decimal)
			s |= FPSR_INEX1;
		fp_status_bits[f] = s;
	}
}

static void fp_get_status(uae_u32 *status)
{
	*status |= fp_status_bits[fs.float_exception_flags];
}


/* a = a + b, a - b or a * b inline (softfloat_fastx80.h) when that gives
 * softfloat's result: rounding to nearest at extended precision and no FPU
 * exception enabled
 * (an enabled one reads the unrounded result softfloat saves). The inexact
 * flag goes to fs as softfloat would raise it. False: use softfloat. */
enum { FD_ADD, FD_SUB, FD_MUL };
#ifdef __SIZEOF_INT128__
static inline __attribute__((always_inline)) bool fp_fast_dyadic(int op, floatx80 *a, floatx80 b)
{
	if (fs.float_rounding_mode != float_round_nearest_even || fs.floatx80_rounding_precision != 80
		|| (regs.fpcr & 0xff00))
		return false;
	floatx80 r;
	bool inexact;
	if (op == FD_MUL ? !fx80_mul(*a, b, &r, &inexact) : !fx80_addsub(*a, b, op == FD_SUB, &r, &inexact))
		return false;
	*a = r;
	if (inexact)
		fs.float_exception_flags |= float_flag_inexact;
	return true;
}
#else
static inline bool fp_fast_dyadic(int, floatx80 *, floatx80)
{
	return false;
}
#endif

/* ---- the exact core's fast path for the common arithmetic instructions,
 * when no FPU exception is enabled: dst = op(dst, src) in place, and the
 * FPSR CC, exception and accrued bytes set as fpp.cpp's generic path does
 * (fpsr_set_result_all() and fpsr_make_status()). The caller cleared the
 * exception flags before converting src. False: not one of these (nothing
 * done). */
static uae_u32 fp_status_fpsr[256]; // exception byte and its accrued bits per flags value

static void fp_init_status_fpsr(void)
{
	for (int f = 0; f < 256; f++) {
		uae_u32 s = fp_status_bits[f];
		if (s & FPSR_OVFL)
			s |= 0x40;                     // AE OVFL
		if (s & (FPSR_OVFL | FPSR_INEX2 | FPSR_INEX1))
			s |= 0x08;                     // AE INEX
		if (s & (FPSR_SNAN | FPSR_OPERR))
			s |= 0x80;                     // AE IOP
		if ((s & FPSR_UNFL) && (s & FPSR_INEX2))
			s |= 0x20;                     // AE UNFL
		if (s & FPSR_DZ)
			s |= 0x10;                     // AE DZ
		fp_status_fpsr[f] = s;
	}
}

static inline floatx80 fp_fast_normalize(floatx80 v)
{
	const int e = v.high & 0x7fff;
	if (e == 0x7fff || (e != 0 && (v.low >> 63)) || (e == 0 && v.low == 0))
		return v;
	return floatx80_normalize(v);          // unnormal or denormal
}

enum { FK_MOVE, FK_ADD, FK_SUB, FK_MUL, FK_DIV, FK_SQRT, FK_ABS, FK_NEG, FK_CMP, FK_TST, FK_INT, FK_INTRZ };

bool fp_exact_fast_arith(fpdata *dst, const fpdata *src, uae_u16 extra)
{
	int kind, prec = 0;
	bool dyadic = true;
	switch (extra & 0x7f) {
	case 0x00: kind = FK_MOVE; dyadic = false; break;
	case 0x40: kind = FK_MOVE; dyadic = false; prec = 32; break;
	case 0x44: kind = FK_MOVE; dyadic = false; prec = 64; break;
	case 0x22: kind = FK_ADD; break;
	case 0x62: kind = FK_ADD; prec = 32; break;
	case 0x66: kind = FK_ADD; prec = 64; break;
	case 0x28: kind = FK_SUB; break;
	case 0x68: kind = FK_SUB; prec = 32; break;
	case 0x6c: kind = FK_SUB; prec = 64; break;
	case 0x23: kind = FK_MUL; break;
	case 0x63: kind = FK_MUL; prec = 32; break;
	case 0x67: kind = FK_MUL; prec = 64; break;
	case 0x20: kind = FK_DIV; break;
	case 0x60: kind = FK_DIV; prec = 32; break;
	case 0x64: kind = FK_DIV; prec = 64; break;
	case 0x04: kind = FK_SQRT; dyadic = false; break;
	case 0x41: kind = FK_SQRT; dyadic = false; prec = 32; break;
	case 0x45: kind = FK_SQRT; dyadic = false; prec = 64; break;
	case 0x18: kind = FK_ABS; dyadic = false; break;
	case 0x58: kind = FK_ABS; dyadic = false; prec = 32; break;
	case 0x5c: kind = FK_ABS; dyadic = false; prec = 64; break;
	case 0x1a: kind = FK_NEG; dyadic = false; break;
	case 0x5a: kind = FK_NEG; dyadic = false; prec = 32; break;
	case 0x5e: kind = FK_NEG; dyadic = false; prec = 64; break;
	case 0x38: kind = FK_CMP; break;
	case 0x3a: kind = FK_TST; dyadic = false; break;
	case 0x01: kind = FK_INT; dyadic = false; break;
	case 0x03: kind = FK_INTRZ; dyadic = false; break;
	default:
		return false;
	}
	const floatx80 s = fp_fast_normalize(src->fpx);
	floatx80 d = dst->fpx;
	if (dyadic)
		d = fp_fast_normalize(d);
	const int8_t oldprec = fs.floatx80_rounding_precision;
	if (prec)
		set_floatx80_rounding_precision(prec, &fs);
	floatx80 r;
	switch (kind) {
	case FK_MOVE: r = floatx80_move(s, &fs); break;
	case FK_ADD: r = d; if (!fp_fast_dyadic(FD_ADD, &r, s)) r = floatx80_add(d, s, &fs); break;
	case FK_SUB: r = d; if (!fp_fast_dyadic(FD_SUB, &r, s)) r = floatx80_sub(d, s, &fs); break;
	case FK_MUL: r = d; if (!fp_fast_dyadic(FD_MUL, &r, s)) r = floatx80_mul(d, s, &fs); break;
	case FK_DIV: r = floatx80_div(d, s, &fs); break;
	case FK_SQRT: r = floatx80_sqrt(s, &fs); break;
	case FK_ABS: r = floatx80_abs(s, &fs); break;
	case FK_NEG: r = floatx80_neg(s, &fs); break;
	case FK_CMP: r = floatx80_cmp(d, s, &fs); break;
	case FK_TST: r = floatx80_tst(s, &fs); break;
	case FK_INT: r = floatx80_round_to_int(s, &fs); break;
	default: r = floatx80_round_to_int_toward_zero(s, &fs); break;
	}
	if (prec)
		set_floatx80_rounding_precision(oldprec, &fs);
	uae_u32 cc = floatx80_is_negative(r) ? 8 : 0;
	if (floatx80_is_any_nan(r))
		cc |= 1;
	else if (floatx80_is_zero(r))
		cc |= 4;
	else if (floatx80_is_infinity(r))
		cc |= 2;
	regs.fpsr = (regs.fpsr & 0x00ff00f8) | (cc << 24) | fp_status_fpsr[fs.float_exception_flags];
	fpu_fpsr_cc_written = false;
	if (kind != FK_CMP && kind != FK_TST)
		dst->fpx = r;
	return true;
}

STATIC_INLINE void fp_clear_status(void)
{
	fs.float_exception_flags = 0;
}

static uae_u32 fp_get_support_flags(void)
{
	return FPU_FEATURE_EXCEPTIONS | FPU_FEATURE_DENORMALS;
}

static const TCHAR *fp_printx80(floatx80 *fx, int mode)
{
	static TCHAR fsout[32];
	flag n, u, d;

	if (mode < 0) {
		_sntprintf(fsout, sizeof fsout / sizeof(TCHAR), _T("%04X-%08X-%08X"), fx->high, (uae_u32)(fx->low >> 32), (uae_u32)fx->low);
		return fsout;
	}

	n = floatx80_is_negative(*fx);
	u = floatx80_is_unnormal(*fx);
	d = floatx80_is_denormal(*fx);
	
	if (floatx80_is_infinity(*fx)) {
		_sntprintf(fsout, sizeof fsout / sizeof(TCHAR), _T("%c%s"), n ? '-' : '+', _T("inf"));
	} else if (floatx80_is_signaling_nan(*fx)) {
		_sntprintf(fsout, sizeof fsout / sizeof(TCHAR), _T("%c%s"), n ? '-' : '+', _T("snan"));
	} else if (floatx80_is_nan(*fx)) {
		_sntprintf(fsout, sizeof fsout / sizeof(TCHAR), _T("%c%s"), n ? '-' : '+', _T("nan"));
	} else {
		int32_t len = 17;
		int8_t save_exception_flags = fs.float_exception_flags;
		fs.float_exception_flags = 0;
		floatx80 x = floatx80_to_floatdecimal(*fx, &len, &fs);
		_sntprintf(fsout, sizeof fsout / sizeof(TCHAR), _T("%c%01lld.%016llde%c%05u%s%s"), n ? '-' : '+',
				x.low / LIT64(10000000000000000), x.low % LIT64(10000000000000000),
				(x.high & 0x4000) ? '-' : '+', x.high & 0x3FFF, d ? _T("D") : u ? _T("U") : _T(""),
				(fs.float_exception_flags & float_flag_inexact) ? _T("~") : _T(""));
		fs.float_exception_flags = save_exception_flags;
	}

	if (mode == 0 || mode > _tcslen(fsout))
		return fsout;
	fsout[mode] = 0;
	return fsout;
}

static const TCHAR *fp_print(fpdata *fpd, int mode)
{
	return fp_printx80(&fpd->fpx, mode);
}

/* Functions for detecting float type */
static bool fp_is_init(fpdata *fpd)
{
	return false;
}
static bool fp_is_snan(fpdata *fpd)
{
	return floatx80_is_signaling_nan(fpd->fpx) != 0;
}
static bool fp_unset_snan(fpdata *fpd)
{
	fpd->fpx.low |= LIT64(0x4000000000000000);
	return false;
}
static bool fp_is_nan(fpdata *fpd)
{
	return floatx80_is_any_nan(fpd->fpx) != 0;
}
static bool fp_is_infinity(fpdata *fpd)
{
	return floatx80_is_infinity(fpd->fpx) != 0;
}
static void fp_fix_infinity(fpdata *fpd)
{
	fpd->fpx.low = 0;
}
static bool fp_is_zero(fpdata *fpd)
{
	return floatx80_is_zero(fpd->fpx) != 0;
}
static bool fp_is_neg(fpdata *fpd)
{
	return floatx80_is_negative(fpd->fpx) != 0;
}
static bool fp_is_denormal(fpdata *fpd)
{
	return floatx80_is_denormal(fpd->fpx) != 0;
}
static bool fp_is_unnormal(fpdata *fpd)
{
	return floatx80_is_unnormal(fpd->fpx) != 0;
}
static bool fp_is_denormal_or_unnormal(fpdata *fpd)
{
	return floatx80_is_denormal(fpd->fpx) || floatx80_is_unnormal(fpd->fpx);
}
static uae_u32 fp_get_cc(fpdata *fpd)
{
	const floatx80 v = fpd->fpx;
	uae_u32 cc = floatx80_is_negative(v) ? 8 : 0;
	if (floatx80_is_any_nan(v))
		cc |= 1;
	else if (floatx80_is_zero(v))
		cc |= 4;
	else if (floatx80_is_infinity(v))
		cc |= 2;
	return cc;
}


static void to_single(fpdata *fpd, uae_u32 wrd1)
{
	float32 f = wrd1;
	fpd->fpx = float32_to_floatx80_allowunnormal(f, &fs);
}
static uae_u32 from_single(fpdata *fpd)
{
	float32 f = floatx80_to_float32(fpd->fpx, &fs);
	return f;
}

static void to_double(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2)
{
	float64 f = ((float64)wrd1 << 32) | wrd2;
	fpd->fpx = float64_to_floatx80_allowunnormal(f, &fs);
}
static void from_double(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2)
{
	float64 f = floatx80_to_float64(fpd->fpx, &fs);
	*wrd1 = f >> 32;
	*wrd2 = (uae_u32)f;
}

static void to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
	fpd->fpx.high = (uae_u16)(wrd1 >> 16);
	fpd->fpx.low = ((uae_u64)wrd2 << 32) | wrd3;
}
static void from_exten(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2, uae_u32 *wrd3)
{
	floatx80 f = floatx80_to_floatx80(fpd->fpx, &fs);
	*wrd1 = (uae_u32)(f.high << 16);
	*wrd2 = f.low >> 32;
	*wrd3 = (uae_u32)f.low;
}

static void to_exten_fmovem(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
	fpd->fpx.high = (uae_u16)(wrd1 >> 16);
	fpd->fpx.low = ((uae_u64)wrd2 << 32) | wrd3;
}
static void from_exten_fmovem(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2, uae_u32 *wrd3)
 {
	*wrd1 = (uae_u32)(fpd->fpx.high << 16);
	*wrd2 = fpd->fpx.low >> 32;
	*wrd3 = (uae_u32)fpd->fpx.low;
 }

static uae_s64 to_int(fpdata *src, int size)
{
	switch (size) {
		case 0: return floatx80_to_int8(src->fpx, &fs);
		case 1: return floatx80_to_int16(src->fpx, &fs);
		case 2: return floatx80_to_int32(src->fpx, &fs);
		default: return 0;
	 }
}
static void from_int(fpdata *fpd, uae_s32 src)
{
	fpd->fpx = int32_to_floatx80(src);
}

/* Functions for returning exception state data */
static void fp_get_internal_overflow(fpdata *fpd)
{
	fpd->fpx = getFloatInternalOverflow();
}
static void fp_get_internal_underflow(fpdata *fpd)
 {
	fpd->fpx = getFloatInternalUnderflow();
}
static void fp_get_internal_round_all(fpdata *fpd)
{
	fpd->fpx = getFloatInternalRoundedAll();
}
static void fp_get_internal_round(fpdata *fpd)
{
	fpd->fpx = getFloatInternalRoundedSome();
}
static void fp_get_internal_round_exten(fpdata *fpd)
{
	fpd->fpx = getFloatInternalFloatx80();
}
static void fp_get_internal(fpdata *fpd)
{
	fpd->fpx = getFloatInternalUnrounded();
}
static uae_u32 fp_get_internal_grs(void)
{
	return (uae_u32)getFloatInternalGRS();
}
/* Function for denormalizing */
static void fp_denormalize(fpdata *fpd, int esign)
{
    fpd->fpx = floatx80_denormalize(fpd->fpx, esign);
}

/* Functions for rounding */

// round to float with extended precision exponent
static void fp_round32(fpdata *fpd)
{
	fpd->fpx = floatx80_round32(fpd->fpx, &fs);
}

// round to double with extended precision exponent
static void fp_round64(fpdata *fpd)
{
	fpd->fpx = floatx80_round64(fpd->fpx, &fs);
}

// round to float
static void fp_round_single(fpdata *fpd)
{
	fpd->fpx = floatx80_round_to_float32(fpd->fpx, &fs);
}

// round to double
static void fp_round_double(fpdata *fpd)
{
	fpd->fpx = floatx80_round_to_float64(fpd->fpx, &fs);
}

#if 0
// round to selected precision
static void fp_round(fpdata *a)
{
	switch(fs.floatx80_rounding_precision) {
		case 32:
			a->fpx = floatx80_round_to_float32(a->fpx, &fs);
		break;
		case 64:
			a->fpx = floatx80_round_to_float64(a->fpx, &fs);
		break;
		default:
		break;
	}
}
#endif

/* Arithmetic functions */

static void fp_int(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_round_to_int(b->fpx, &fs);
}

static void fp_intrz(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_round_to_int_toward_zero(b->fpx, &fs);
}

static void fp_getexp(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_getexp(b->fpx, &fs);
}
static void fp_getman(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_getman(b->fpx, &fs);
}
static void fp_mod(fpdata *a, fpdata *b, uae_u64 *q, uae_u8 *s)
{
	uint64_t quotient = *q;
	a->fpx = floatx80_mod(a->fpx, b->fpx, &quotient, s, &fs);
	*q = quotient;
}
static void fp_sgldiv(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_sgldiv(a->fpx, b->fpx, &fs);
}
static void fp_sglmul(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_sglmul(a->fpx, b->fpx, &fs);
}
static void fp_rem(fpdata *a, fpdata *b, uae_u64 *q, uae_u8 *s)
{
	uint64_t quotient = *q;
	a->fpx = floatx80_rem(a->fpx, b->fpx, &quotient, s, &fs);
	*q = quotient;
}
static void fp_scale(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_scale(a->fpx, b->fpx, &fs);
}
static void fp_cmp(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_cmp(a->fpx, b->fpx, &fs);
}
static void fp_tst(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_tst(b->fpx, &fs);
}

static const uint8_t prectable[] = { 0, 32, 64, 80 };

#define SETPREC \
	uint8_t oldprec = fs.floatx80_rounding_precision; \
	if (prec > PREC_NORMAL) \
		set_floatx80_rounding_precision(prectable[prec], &fs);

#define RESETPREC \
	set_floatx80_rounding_precision(oldprec, &fs);

/* Functions with fixed precision */
static void fp_move(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	a->fpx = floatx80_move(b->fpx, &fs);
	RESETPREC
}
static void fp_abs(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	a->fpx = floatx80_abs(b->fpx, &fs);
	RESETPREC
}
static void fp_neg(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	a->fpx = floatx80_neg(b->fpx, &fs);
	RESETPREC
}
static void fp_add(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	if (!fp_fast_dyadic(FD_ADD, &a->fpx, b->fpx))
		a->fpx = floatx80_add(a->fpx, b->fpx, &fs);
	RESETPREC
}
static void fp_sub(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	if (!fp_fast_dyadic(FD_SUB, &a->fpx, b->fpx))
		a->fpx = floatx80_sub(a->fpx, b->fpx, &fs);
	RESETPREC
}
static void fp_mul(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	if (!fp_fast_dyadic(FD_MUL, &a->fpx, b->fpx))
		a->fpx = floatx80_mul(a->fpx, b->fpx, &fs);
	RESETPREC
}
static void fp_div(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	a->fpx = floatx80_div(a->fpx, b->fpx, &fs);
	RESETPREC
}
static void fp_sqrt(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	a->fpx = floatx80_sqrt(b->fpx, &fs);
	RESETPREC
}


static void fp_sinh(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_sinh(b->fpx, &fs);
}
static void fp_lognp1(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_lognp1(b->fpx, &fs);
}
static void fp_etoxm1(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_etoxm1(b->fpx, &fs);
}
static void fp_tanh(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_tanh(b->fpx, &fs);
}
static void fp_atan(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_atan(b->fpx, &fs);
}
static void fp_asin(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_asin(b->fpx, &fs);
}
static void fp_atanh(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_atanh(b->fpx, &fs);
}
static void fp_sin(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_sin(b->fpx, &fs);
}
static void fp_tan(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_tan(b->fpx, &fs);
}
static void fp_etox(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_etox(b->fpx, &fs);
}
static void fp_twotox(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_twotox(b->fpx, &fs);
}
static void fp_tentox(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_tentox(b->fpx, &fs);
}
static void fp_logn(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_logn(b->fpx, &fs);
}
static void fp_log10(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_log10(b->fpx, &fs);
}
static void fp_log2(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_log2(b->fpx, &fs);
}
static void fp_cosh(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_cosh(b->fpx, &fs);
}
static void fp_acos(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_acos(b->fpx, &fs);
}
static void fp_cos(fpdata *a, fpdata *b)
{
    a->fpx = floatx80_cos(b->fpx, &fs);
}
static void fp_sincos(fpdata *a, fpdata *b, fpdata *c)
{
	a->fpx = floatx80_sincos(b->fpx, &c->fpx, &fs);
}

// the exact FPU core: double-double versions (softfloat/softfloat_dd.cpp)
static void fp_dd_etox(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_etox(b->fpx, &fs);
}
static void fp_dd_etoxm1(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_etoxm1(b->fpx, &fs);
}
static void fp_dd_twotox(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_twotox(b->fpx, &fs);
}
static void fp_dd_tentox(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_tentox(b->fpx, &fs);
}
static void fp_dd_sinh(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_sinh(b->fpx, &fs);
}
static void fp_dd_cosh(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_cosh(b->fpx, &fs);
}
static void fp_dd_tanh(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_tanh(b->fpx, &fs);
}
static void fp_dd_logn(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_logn(b->fpx, &fs);
}
static void fp_dd_lognp1(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_lognp1(b->fpx, &fs);
}
static void fp_dd_log10(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_log10(b->fpx, &fs);
}
static void fp_dd_log2(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_log2(b->fpx, &fs);
}
static void fp_dd_atanh(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_atanh(b->fpx, &fs);
}
static void fp_dd_sin(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_sin(b->fpx, &fs);
}
static void fp_dd_cos(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_cos(b->fpx, &fs);
}
static void fp_dd_tan(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_tan(b->fpx, &fs);
}
static void fp_dd_atan(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_atan(b->fpx, &fs);
}
static void fp_dd_asin(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_asin(b->fpx, &fs);
}
static void fp_dd_acos(fpdata *a, fpdata *b)
{
	a->fpx = floatx80_dd_acos(b->fpx, &fs);
}
static void fp_dd_sincos(fpdata *a, fpdata *b, fpdata *c)
{
	a->fpx = floatx80_dd_sincos(b->fpx, &c->fpx, &fs);
}


#ifdef JIT
/* ---- compiled FPU arithmetic, specialised per operation, precision and
 * source kind at JIT compile time (fp_jit_fast_helper()): the common
 * instructions without any decoding. Same arguments and results as
 * fpp_jit_arith() (fpp.cpp), which they hand over to when the fast path does
 * not apply: an FPU exception enabled or pending, the FPU disabled (68060
 * PCR), or a core other than softfloat active. */
enum { FJS_FPM, FJS_L, FJS_S, FJS_W, FJS_B, FJS_D, FJS_X };
static bool fpj_fpiar_always; // 68040/68060: FPIAR on every instruction

template <int OP, int PREC, int SRC>
static uae_u32 REGPARAM3 fpj_arith(uae_u32 opext, uae_u64 value, uae_u8 *pc_p, uae_u32 pc)
{
	if ((regs.fpcr & 0xff00) | regs.fp_exp_pend | regs.fp_unimp_pend | (regs.pcr & 2) | (fpp_add != fp_add))
		return fpp_jit_arith(opext, value, pc_p, pc);
	const uae_u16 extra = (uae_u16)opext;
	fpdata *const dstp = &regs.fp[(extra >> 7) & 7];
	fs.float_exception_flags = 0;
	floatx80 s;
	if (SRC == FJS_FPM) {
		s = regs.fp[(extra >> 10) & 7].fpx;
	} else {
		const uae_u32 v = (opext & 0x00380000) == 0 ? m68k_dreg(regs, (opext >> 16) & 7) : (uae_u32)value;
		switch (SRC) {
		case FJS_L: s = int32_to_floatx80((uae_s32)v); break;
		case FJS_W: s = int32_to_floatx80((uae_s16)v); break;
		case FJS_B: s = int32_to_floatx80((uae_s8)v); break;
		case FJS_S: s = float32_to_floatx80_allowunnormal(v, &fs); break;
		case FJS_D: s = float64_to_floatx80_allowunnormal(value, &fs); break;
		default: {
			const uaecptr ad = (uaecptr)value;
			s.high = (uae_u16)(x_cp_get_long(ad) >> 16);
			s.low = ((uae_u64)x_cp_get_long(ad + 4) << 32) | x_cp_get_long(ad + 8);
			break;
		}
		}
	}
	s = fp_fast_normalize(s);
	const bool dyadic = OP == FK_ADD || OP == FK_SUB || OP == FK_MUL || OP == FK_DIV || OP == FK_CMP;
	floatx80 d;
	if (dyadic)
		d = fp_fast_normalize(dstp->fpx);
	int8_t oldprec = 0;
	if (PREC) {
		oldprec = fs.floatx80_rounding_precision;
		set_floatx80_rounding_precision(PREC, &fs);
	}
	floatx80 r;
	switch (OP) {
	case FK_MOVE: r = floatx80_move(s, &fs); break;
	case FK_ADD: r = d; if (!fp_fast_dyadic(FD_ADD, &r, s)) r = floatx80_add(d, s, &fs); break;
	case FK_SUB: r = d; if (!fp_fast_dyadic(FD_SUB, &r, s)) r = floatx80_sub(d, s, &fs); break;
	case FK_MUL: r = d; if (!fp_fast_dyadic(FD_MUL, &r, s)) r = floatx80_mul(d, s, &fs); break;
	case FK_DIV: r = floatx80_div(d, s, &fs); break;
	case FK_SQRT: r = floatx80_sqrt(s, &fs); break;
	case FK_ABS: r = floatx80_abs(s, &fs); break;
	case FK_NEG: r = floatx80_neg(s, &fs); break;
	case FK_CMP: r = floatx80_cmp(d, s, &fs); break;
	case FK_TST: r = floatx80_tst(s, &fs); break;
	case FK_INT: r = floatx80_round_to_int(s, &fs); break;
	default: r = floatx80_round_to_int_toward_zero(s, &fs); break;
	}
	if (PREC)
		set_floatx80_rounding_precision(oldprec, &fs);
	uae_u32 cc = floatx80_is_negative(r) ? 8 : 0;
	if (floatx80_is_any_nan(r))
		cc |= 1;
	else if (floatx80_is_zero(r))
		cc |= 4;
	else if (floatx80_is_infinity(r))
		cc |= 2;
	regs.fpsr = (regs.fpsr & 0x00ff00f8) | (cc << 24) | fp_status_fpsr[fs.float_exception_flags];
	fpu_fpsr_cc_written = false;
	if (OP != FK_CMP && OP != FK_TST)
		dstp->fpx = r;
	regs.fpu_state = 1;
	regs.fp_exception = false;
	if (fpj_fpiar_always)
		regs.fpiar = pc;
	return 0;
}

#ifdef __SIZEOF_INT128__
/* The common case of fpj_arith<OP, 0, SRC> as a leaf function, without a
 * stack frame (fpj_arith saves registers for its calls): rounding to nearest
 * at extended precision, no FPU exception enabled, normal operands, and a
 * result softfloat_fastx80.h computes. Anything else goes to fpj_arith. */
template <int SRC>
static inline __attribute__((always_inline)) bool fpj_leaf_source(uae_u32 opext, uae_u64 value, floatx80 *s)
{
	if (SRC == FJS_FPM) {
		*s = regs.fp[(opext >> 10) & 7].fpx;
		return fx80_normal(*s);
	}
	if (SRC == FJS_D)
		return fx80_from_f64(value, s);
	const uae_u32 v = (opext & 0x00380000) == 0 ? m68k_dreg(regs, (opext >> 16) & 7) : (uae_u32)value;
	if (SRC == FJS_S)
		return fx80_from_f32(v, s);
	return fx80_from_i32(SRC == FJS_L ? (uae_s32)v : SRC == FJS_W ? (uae_s16)v : (uae_s8)v, s);
}

template <int OP, int SRC>
static uae_u32 REGPARAM3 fpj_leaf(uae_u32 opext, uae_u64 value, uae_u8 *pc_p, uae_u32 pc)
{
	// FPCR: no exception enabled, extended precision, round to nearest
	if ((regs.fpcr & 0xfff0) | regs.fp_exp_pend | regs.fp_unimp_pend | (regs.pcr & 2) | (fpp_add != fp_add))
		return fpj_arith<OP, 0, SRC>(opext, value, pc_p, pc);
	floatx80 s;
	if (!fpj_leaf_source<SRC>(opext, value, &s))
		return fpj_arith<OP, 0, SRC>(opext, value, pc_p, pc);
	fpdata *const dstp = &regs.fp[(opext >> 7) & 7];
	floatx80 r = s;
	bool inexact = false;
	uae_u32 cc;
	switch (OP) {
	case FK_ADD:
	case FK_SUB:
		if (!fx80_addsub(dstp->fpx, s, OP == FK_SUB, &r, &inexact))
			return fpj_arith<OP, 0, SRC>(opext, value, pc_p, pc);
		break;
	case FK_MUL:
		if (!fx80_mul(dstp->fpx, s, &r, &inexact))
			return fpj_arith<OP, 0, SRC>(opext, value, pc_p, pc);
		break;
	case FK_ABS: r.high &= 0x7fff; break;
	case FK_NEG: r.high ^= 0x8000; break;
	default: break;                                // FK_MOVE, FK_TST, FK_CMP
	}
	if (OP == FK_CMP) {
		if (!fx80_cmp_cc(dstp->fpx, s, &cc))
			return fpj_arith<OP, 0, SRC>(opext, value, pc_p, pc);
	} else {
		cc = (r.high & 0x8000) ? 8 : 0;
	}
	regs.fpsr = (regs.fpsr & 0x00ff00f8) | (cc << 24) | (inexact ? fp_status_fpsr[float_flag_inexact] : 0);
	fpu_fpsr_cc_written = false;
	if (OP != FK_CMP && OP != FK_TST)
		dstp->fpx = r;
	regs.fpu_state = 1;
	regs.fp_exception = false;
	if (fpj_fpiar_always)
		regs.fpiar = pc;
	return 0;
}
#endif

typedef uae_u32 (REGPARAM3 *fpj_fn)(uae_u32, uae_u64, uae_u8 *, uae_u32);

template <int OP, int PREC>
static fpj_fn fpj_pick(int src)
{
#ifdef __SIZEOF_INT128__
	if (PREC == 0 && (OP == FK_MOVE || OP == FK_ADD || OP == FK_SUB || OP == FK_MUL || OP == FK_ABS
		|| OP == FK_NEG || OP == FK_CMP || OP == FK_TST)) {
		switch (src) {
		case FJS_FPM: return fpj_leaf<OP, FJS_FPM>;
		case FJS_L: return fpj_leaf<OP, FJS_L>;
		case FJS_S: return fpj_leaf<OP, FJS_S>;
		case FJS_W: return fpj_leaf<OP, FJS_W>;
		case FJS_B: return fpj_leaf<OP, FJS_B>;
		case FJS_D: return fpj_leaf<OP, FJS_D>;
		default: break;                            // FJS_X: memory reads, fpj_arith
		}
	}
#endif
	switch (src) {
	case FJS_FPM: return fpj_arith<OP, PREC, FJS_FPM>;
	case FJS_L: return fpj_arith<OP, PREC, FJS_L>;
	case FJS_S: return fpj_arith<OP, PREC, FJS_S>;
	case FJS_W: return fpj_arith<OP, PREC, FJS_W>;
	case FJS_B: return fpj_arith<OP, PREC, FJS_B>;
	case FJS_D: return fpj_arith<OP, PREC, FJS_D>;
	default: return fpj_arith<OP, PREC, FJS_X>;
	}
}

// the specialised helper for an instruction jit_fpu_arith_ok() accepted, or nullptr (fpp_jit_arith())
void *fp_jit_fast_helper(uae_u16 opcode, uae_u16 extra)
{
	int src;
	if (!(extra & 0x4000)) {
		src = FJS_FPM;
	} else {
		switch ((extra >> 10) & 7) {
		case 0: src = FJS_L; break;
		case 1: src = FJS_S; break;
		case 4: src = FJS_W; break;
		case 6: src = FJS_B; break;
		case 5: src = FJS_D; break;
		case 2: src = FJS_X; break;
		default: return nullptr;
		}
	}
	switch (extra & 0x7f) {
	case 0x00: return (void *)fpj_pick<FK_MOVE, 0>(src);
	case 0x40: return (void *)fpj_pick<FK_MOVE, 32>(src);
	case 0x44: return (void *)fpj_pick<FK_MOVE, 64>(src);
	case 0x22: return (void *)fpj_pick<FK_ADD, 0>(src);
	case 0x62: return (void *)fpj_pick<FK_ADD, 32>(src);
	case 0x66: return (void *)fpj_pick<FK_ADD, 64>(src);
	case 0x28: return (void *)fpj_pick<FK_SUB, 0>(src);
	case 0x68: return (void *)fpj_pick<FK_SUB, 32>(src);
	case 0x6c: return (void *)fpj_pick<FK_SUB, 64>(src);
	case 0x23: return (void *)fpj_pick<FK_MUL, 0>(src);
	case 0x63: return (void *)fpj_pick<FK_MUL, 32>(src);
	case 0x67: return (void *)fpj_pick<FK_MUL, 64>(src);
	case 0x20: return (void *)fpj_pick<FK_DIV, 0>(src);
	case 0x60: return (void *)fpj_pick<FK_DIV, 32>(src);
	case 0x64: return (void *)fpj_pick<FK_DIV, 64>(src);
	case 0x04: return (void *)fpj_pick<FK_SQRT, 0>(src);
	case 0x41: return (void *)fpj_pick<FK_SQRT, 32>(src);
	case 0x45: return (void *)fpj_pick<FK_SQRT, 64>(src);
	case 0x18: return (void *)fpj_pick<FK_ABS, 0>(src);
	case 0x58: return (void *)fpj_pick<FK_ABS, 32>(src);
	case 0x5c: return (void *)fpj_pick<FK_ABS, 64>(src);
	case 0x1a: return (void *)fpj_pick<FK_NEG, 0>(src);
	case 0x5a: return (void *)fpj_pick<FK_NEG, 32>(src);
	case 0x5e: return (void *)fpj_pick<FK_NEG, 64>(src);
	case 0x38: return (void *)fpj_pick<FK_CMP, 0>(src);
	case 0x3a: return (void *)fpj_pick<FK_TST, 0>(src);
	case 0x01: return (void *)fpj_pick<FK_INT, 0>(src);
	case 0x03: return (void *)fpj_pick<FK_INTRZ, 0>(src);
	default: return nullptr;
	}
}
#endif

/* Functions for converting between float formats */
static const fptype twoto32 = 4294967296.0;

static void to_native(fptype *fp, fpdata *fpd)
{
	int expon;
	fptype frac;
	
	expon = fpd->fpx.high & 0x7fff;
	
	fp_is_init(fpd);
	if (fp_is_zero(fpd)) {
		*fp = fp_is_neg(fpd) ? -0.0 : +0.0;
		return;
	}
	if (fp_is_nan(fpd)) {
#ifdef USE_LONG_DOUBLE
		*fp = sqrtl(-1);
#else
		*fp = sqrt(-1);
#endif
		return;
	}
	if (fp_is_infinity(fpd)) {
		double zero = 0.0;
#ifdef USE_LONG_DOUBLE
		*fp = fp_is_neg(fpd) ? logl(0.0) : (1.0 / zero);
#else
		*fp = fp_is_neg(fpd) ? log(0.0) : (1.0 / zero);
#endif
		return;
	}
	
	frac = (fptype)fpd->fpx.low / (fptype)(twoto32 * 2147483648.0);
	if (fp_is_neg(fpd))
		frac = -frac;
#ifdef USE_LONG_DOUBLE
	*fp = ldexpl (frac, expon - 16383);
#else
	*fp = ldexp (frac, expon - 16383);
#endif
}

static void from_native(fptype fp, fpdata *fpd)
{
	int expon;
	fptype frac;
	
	if (signbit(fp))
		fpd->fpx.high = 0x8000;
	else
		fpd->fpx.high = 0x0000;
	
	if (isnan(fp)) {
		fpd->fpx.high |= 0x7fff;
		fpd->fpx.low = LIT64(0xffffffffffffffff);
		return;
	}
	if (isinf(fp)) {
		fpd->fpx.high |= 0x7fff;
		fpd->fpx.low = LIT64(0x0000000000000000);
		return;
	}
	if (fp == 0.0) {
		fpd->fpx.low = LIT64(0x0000000000000000);
		return;
	}
	if (fp < 0.0)
		fp = -fp;
	
#ifdef USE_LONG_DOUBLE
	 frac = frexpl (fp, &expon);
#else
	 frac = frexp (fp, &expon);
#endif
	frac += 0.5 / (twoto32 * twoto32);
	if (frac >= 1.0) {
		frac /= 2.0;
		expon++;
	}
	fpd->fpx.high |= (expon + 16383 - 1) & 0x7fff;
	fpd->fpx.low = (uint64_t)(frac * (fptype)(twoto32 * twoto32));
	
	while (!(fpd->fpx.low & LIT64( 0x8000000000000000))) {
		if (fpd->fpx.high == 0) {
			break;
		}
		fpd->fpx.low <<= 1;
		fpd->fpx.high--;
	}
}

static void fp_normalize(fpdata *a)
{
	a->fpx = floatx80_normalize(a->fpx);
}

static void fp_to_pack(fpdata *fp, uae_u32 *wrd, int dummy)
{
	floatx80 f;
	int i;
	uae_s32 exp;
	uae_s64 mant;
	uae_u32 pack_exp, pack_int, pack_se, pack_sm;
	uae_u64 pack_frac;

	if (((wrd[0] >> 16) & 0x7fff) == 0x7fff) {
		// infinity has extended exponent and all 0 packed fraction
		// nans are copies bit by bit
		fpp_to_exten(fp, wrd[0], wrd[1], wrd[2]);
		return;
	}
	if (!(wrd[0] & 0xf) && !wrd[1] && !wrd[2]) {
		// exponent is not cared about, if mantissa is zero
		wrd[0] &= 0x80000000;
		fpp_to_exten(fp, wrd[0], wrd[1], wrd[2]);
		return;
	}

	pack_exp = (wrd[0] >> 16) & 0xFFF;              // packed exponent
	pack_int = wrd[0] & 0xF;                        // packed integer part
	pack_frac = ((uae_u64)wrd[1] << 32) | wrd[2];   // packed fraction
	pack_se = (wrd[0] >> 30) & 1;                   // sign of packed exponent
	pack_sm = (wrd[0] >> 31) & 1;                   // sign of packed significand
	exp = 0;
	
	for (i = 0; i < 3; i++) {
		exp *= 10;
		exp += (pack_exp >> (8 - i * 4)) & 0xF;
	}
	
	if (pack_se) {
		exp = -exp;
	}

	exp -= 16;
	
	if (exp < 0) {
		exp = -exp;
		pack_se = 1;
	}
	
	mant = pack_int;

	for (i = 0; i < 16; i++) {
		mant *= 10;
		mant += (pack_frac >> (60 - i * 4)) & 0xF;
	}

	f.high = exp & 0x3FFF;
	f.high |= pack_se ? 0x4000 : 0;
	f.high |= pack_sm ? 0x8000 : 0;
	f.low = mant;
	
	fp->fpx = floatdecimal_to_floatx80(f, &fs);
}


static void fp_from_pack(fpdata *fp, uae_u32 *wrd, int kfactor)
{
	floatx80 f = floatx80_to_floatdecimal(fp->fpx, &kfactor, &fs);
	
	uae_u32 pack_exp, pack_exp4, pack_int, pack_se, pack_sm;
	uae_u64 pack_frac;    

	uae_u32 exponent;
	uae_u64 significand;

	uae_s32 len;
	uae_u64 digit;
 
	if ((f.high & 0x7FFF) == 0x7FFF) {
		wrd[0] = (uae_u32)(f.high << 16);
		wrd[1] = f.low >> 32;
		wrd[2] = (uae_u32)f.low;
	} else {
		exponent = f.high & 0x3FFF;
		significand = f.low;
		
		pack_int = 0;
		pack_frac = 0;
		len = kfactor; // SoftFloat saved len to kfactor variable
		while (len > 0) {
			len--;
			digit = significand % 10;
			significand /= 10;
			if (len == 0) {
				pack_int = (uae_u32)digit;
			} else {
				pack_frac |= digit << (64 - len * 4);
			}
		}

		pack_exp = 0;
		pack_exp4 = 0;
		len = 4;
		while (len > 0) {
			len--;
			digit = exponent % 10;
			exponent /= 10;
			if (len == 0) {
				pack_exp4 = (uae_u32)digit;
			} else {
				pack_exp |= digit << (12 - len * 4);
			}
		}
		
		pack_se = f.high & 0x4000;
		pack_sm = f.high & 0x8000;
		
		wrd[0] = pack_exp << 16;
		wrd[0] |= pack_exp4 << 12;
		wrd[0] |= pack_int;
		wrd[0] |= pack_se ? 0x40000000 : 0;
		wrd[0] |= pack_sm ? 0x80000000 : 0;
		
		wrd[1] = pack_frac >> 32;
		wrd[2] = pack_frac & 0xffffffff;
	}
}

void fp_init_softfloat(int fpu_model)
{
	if (fpu_model == 68040) {
		set_special_flags(cmp_signed_nan, &fs);
	} else if (fpu_model == 68060) {
		set_special_flags(infinity_clear_intbit, &fs);
	} else {
		set_special_flags(addsub_swap_inf, &fs);
	}

	fpp_print = fp_print;
	fpp_unset_snan = fp_unset_snan;

	fpp_is_init = fp_is_init;
	fpp_is_snan = fp_is_snan;
	fpp_is_nan = fp_is_nan;
	fpp_is_infinity = fp_is_infinity;
	fpp_is_zero = fp_is_zero;
	fpp_is_neg = fp_is_neg;
	fpp_is_denormal = fp_is_denormal;
	fpp_is_unnormal = fp_is_unnormal;
	fpp_is_denormal_or_unnormal = fp_is_denormal_or_unnormal;
	fpp_get_cc = fp_get_cc;
	fpp_fix_infinity = fp_fix_infinity;

	fp_init_status_bits();
	fp_init_status_fpsr();
	fpp_get_status = fp_get_status;
	fpp_clear_status = fp_clear_status;
	fpp_set_mode = fp_set_mode;
	fpp_get_support_flags = fp_get_support_flags;

	fpp_to_int = to_int;
	fpp_from_int = from_int;

	fpp_to_pack = fp_to_pack;
	fpp_from_pack = fp_from_pack;

	fpp_to_single = to_single;
	fpp_from_single = from_single;
	fpp_to_double = to_double;
	fpp_from_double = from_double;
	fpp_to_exten = to_exten;
	fpp_from_exten = from_exten;
	fpp_to_exten_fmovem = to_exten_fmovem;
	fpp_from_exten_fmovem = from_exten_fmovem;

	fpp_round_single = fp_round_single;
	fpp_round_double = fp_round_double;
	fpp_round32 = fp_round32;
	fpp_round64 = fp_round64;

	fpp_normalize = fp_normalize;
	fpp_denormalize = fp_denormalize;
	fpp_get_internal_overflow = fp_get_internal_overflow;
	fpp_get_internal_underflow = fp_get_internal_underflow;
	fpp_get_internal_round_all = fp_get_internal_round_all;
	fpp_get_internal_round = fp_get_internal_round;
	fpp_get_internal_round_exten = fp_get_internal_round_exten;
	fpp_get_internal = fp_get_internal;
	fpp_get_internal_grs = fp_get_internal_grs;

	fpp_int = fp_int;
	fpp_sinh = fp_sinh;
	fpp_intrz = fp_intrz;
	fpp_sqrt = fp_sqrt;
	fpp_lognp1 = fp_lognp1;
	fpp_etoxm1 = fp_etoxm1;
	fpp_tanh = fp_tanh;
	fpp_atan = fp_atan;
	fpp_atanh = fp_atanh;
	fpp_sin = fp_sin;
	fpp_asin = fp_asin;
	fpp_tan = fp_tan;
	fpp_etox = fp_etox;
	fpp_twotox = fp_twotox;
	fpp_tentox = fp_tentox;
	fpp_logn = fp_logn;
	fpp_log10 = fp_log10;
	fpp_log2 = fp_log2;
	fpp_abs = fp_abs;
	fpp_cosh = fp_cosh;
	fpp_neg = fp_neg;
	fpp_acos = fp_acos;
	fpp_cos = fp_cos;
	fpp_sincos = fp_sincos;
	fpp_getexp = fp_getexp;
	fpp_getman = fp_getman;
	fpp_div = fp_div;
	fpp_mod = fp_mod;
	fpp_add = fp_add;
	fpp_mul = fp_mul;
	fpp_rem = fp_rem;
	fpp_scale = fp_scale;
	fpp_sub = fp_sub;
	fpp_sgldiv = fp_sgldiv;
	fpp_sglmul = fp_sglmul;
	fpp_cmp = fp_cmp;
	fpp_tst = fp_tst;
	fpp_move = fp_move;
}

/* The exact FPU core: softfloat, with transcendental functions that are faster
 * and more accurate than the FPSP algorithms (results within 1 ulp, mostly
 * correctly rounded; softfloat/softfloat_dd.cpp). They need round-to-nearest
 * host arithmetic and else use the FPSP functions. */
void fp_init_softfloat_exact(int fpu_model)
{
	fp_init_softfloat(fpu_model);
#ifdef JIT
	fpj_fpiar_always = fpu_model == 68040 || fpu_model == 68060;
#endif
	fesetround(FE_TONEAREST);
	fpp_etox = fp_dd_etox;
	fpp_etoxm1 = fp_dd_etoxm1;
	fpp_twotox = fp_dd_twotox;
	fpp_tentox = fp_dd_tentox;
	fpp_sinh = fp_dd_sinh;
	fpp_cosh = fp_dd_cosh;
	fpp_tanh = fp_dd_tanh;
	fpp_logn = fp_dd_logn;
	fpp_lognp1 = fp_dd_lognp1;
	fpp_log10 = fp_dd_log10;
	fpp_log2 = fp_dd_log2;
	fpp_atanh = fp_dd_atanh;
	fpp_sin = fp_dd_sin;
	fpp_cos = fp_dd_cos;
	fpp_tan = fp_dd_tan;
	fpp_atan = fp_dd_atan;
	fpp_asin = fp_dd_asin;
	fpp_acos = fp_dd_acos;
	fpp_sincos = fp_dd_sincos;
}
