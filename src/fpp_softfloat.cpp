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
#include <fenv.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "fpp.h"

#include "softfloat/softfloat-macros.h"
#include "softfloat/softfloat-specialize.h"

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

static void fp_get_status(uae_u32 *status)
{
	if (fs.float_exception_flags & float_flag_signaling)
		*status |= FPSR_SNAN;
	if (fs.float_exception_flags & float_flag_invalid)
		*status |= FPSR_OPERR;
	if (fs.float_exception_flags & float_flag_divbyzero)
		*status |= FPSR_DZ;
	if (fs.float_exception_flags & float_flag_overflow)
		*status |= FPSR_OVFL;
	if (fs.float_exception_flags & float_flag_underflow)
		*status |= FPSR_UNFL;
	if (fs.float_exception_flags & float_flag_inexact)
		*status |= FPSR_INEX2;
	if (fs.float_exception_flags & float_flag_decimal)
		*status |= FPSR_INEX1;
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
		_stprintf(fsout, _T("%04X-%08X-%08X"), fx->high, (uae_u32)(fx->low >> 32), (uae_u32)fx->low);
		return fsout;
	}

	n = floatx80_is_negative(*fx);
	u = floatx80_is_unnormal(*fx);
	d = floatx80_is_denormal(*fx);
	
	if (floatx80_is_infinity(*fx)) {
		_stprintf(fsout, _T("%c%s"), n ? '-' : '+', _T("inf"));
	} else if (floatx80_is_signaling_nan(*fx)) {
		_stprintf(fsout, _T("%c%s"), n ? '-' : '+', _T("snan"));
	} else if (floatx80_is_nan(*fx)) {
		_stprintf(fsout, _T("%c%s"), n ? '-' : '+', _T("nan"));
	} else {
		int32_t len = 17;
		int8_t save_exception_flags = fs.float_exception_flags;
		fs.float_exception_flags = 0;
		floatx80 x = floatx80_to_floatdecimal(*fx, &len, &fs);
		_stprintf(fsout, _T("%c%01lld.%016llde%c%05u%s%s"), n ? '-' : '+',
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
	a->fpx = floatx80_mod(a->fpx, b->fpx, q, s, &fs);
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
	a->fpx = floatx80_rem(a->fpx, b->fpx, q, s, &fs);
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
	a->fpx = floatx80_add(a->fpx, b->fpx, &fs);
	RESETPREC
}
static void fp_sub(fpdata *a, fpdata *b, int prec)
{
	SETPREC
	a->fpx = floatx80_sub(a->fpx, b->fpx, &fs);
	RESETPREC
}
static void fp_mul(fpdata *a, fpdata *b, int prec)
{
	SETPREC
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
	fpp_fix_infinity = fp_fix_infinity;

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

