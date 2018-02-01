/*
* UAE - The Un*x Amiga Emulator
*
* MC68881/68882/68040/68060 FPU emulation
*
* Copyright 1996 Herman ten Brugge
* Modified 2005 Peter Keunecke
* 68040+ exceptions and more by Toni Wilen
*
* This is the version for ARM devices. We have these restrictions:
*  - all caclulations are done in double, not in extended format like in MC68881...
*  - rounding is only needed by special opcodes (like FSMOVE or FSADD) or if FPCR is set to single
*/

#define __USE_ISOC9X  /* We might be able to pick up a NaN */

#include <cmath>
#include <cfloat>
#include <cfenv>

#include "sysconfig.h"
#include "sysdeps.h"

#define USE_HOST_ROUNDING 1

#include "options.h"
#include "include/memory.h"
#include "newcpu.h"
#include "fpp.h"

#ifdef JIT
double fp_1e8 = 1.0e8;
#endif

static uae_u32 dhex_nan[]   ={0xffffffff, 0x7fffffff};
static double *fp_nan    = (double *)dhex_nan;
static const double twoto32 = 4294967296.0;

#define	FPCR_ROUNDING_MODE	0x00000030
#define	FPCR_ROUND_NEAR		0x00000000
#define	FPCR_ROUND_ZERO		0x00000010
#define	FPCR_ROUND_MINF		0x00000020
#define	FPCR_ROUND_PINF		0x00000030

#define	FPCR_ROUNDING_PRECISION	0x000000c0
#define	FPCR_PRECISION_SINGLE	0x00000040
#define	FPCR_PRECISION_DOUBLE	0x00000080
#define FPCR_PRECISION_EXTENDED	0x00000000

static uae_u32 fpu_mode_control = 0;
static int fpu_prec;

/* Functions for setting host/library modes and getting status */
static void fpp_set_mode(uae_u32 mode_control)
{
	if (mode_control == fpu_mode_control && !currprefs.compfpu)
		return;
	switch (mode_control & FPCR_ROUNDING_PRECISION) {
	case FPCR_PRECISION_EXTENDED: // X
		fpu_prec = 80;
		break;
	case FPCR_PRECISION_SINGLE:   // S
		fpu_prec = 32;
		break;
	case FPCR_PRECISION_DOUBLE:   // D
	default:                      // undefined
		fpu_prec = 64;
		break;
	}
#if USE_HOST_ROUNDING
	if ((mode_control & FPCR_ROUNDING_MODE) != (fpu_mode_control & FPCR_ROUNDING_MODE)) {
		switch (mode_control & FPCR_ROUNDING_MODE) {
		case FPCR_ROUND_NEAR: // to neareset
			fesetround(FE_TONEAREST);
			break;
		case FPCR_ROUND_ZERO: // to zero
			fesetround(FE_TOWARDZERO);
			break;
		case FPCR_ROUND_MINF: // to minus
			fesetround(FE_DOWNWARD);
			break;
		case FPCR_ROUND_PINF: // to plus
			fesetround(FE_UPWARD);
			break;
		}
	}
#endif
	fpu_mode_control = mode_control;
}


/* Functions for detecting float type */
static bool fpp_is_nan (fpdata *fpd)
{
  return std::isnan(fpd->fp) != 0;
}
static bool fpp_is_infinity (fpdata *fpd)
{
  return std::isinf(fpd->fp) != 0;
}
static bool fpp_is_zero(fpdata *fpd)
{
  return fpd->fp == 0.0;
}
static bool fpp_is_neg(fpdata *fpd)
{
  return signbit(fpd->fp) != 0;
}

/* Functions for converting between float formats */
/* FIXME: how to preserve/fix denormals and unnormals? */

static void fpp_to_native(fptype *fp, fpdata *fpd)
{
	*fp = fpd->fp;
}
static void fpp_from_native(fptype fp, fpdata *fpd)
{
	fpd->fp = fp;
}

static void fpp_to_single(fpdata *fpd, uae_u32 wrd1)
{
	union {
		float f;
		uae_u32 u;
	} val;

	val.u = wrd1;
	fpd->fp = (fptype)val.f;
}
static uae_u32 fpp_from_single(fpdata *fpd)
{
	union {
		float f;
		uae_u32 u;
	} val;

	val.f = (float)fpd->fp;
	return val.u;
}

static void fpp_to_double(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2)
{
#ifdef WORDS_BIGENDIAN
	((uae_u32*)&(fpd->fp))[0] = wrd1;
	((uae_u32*)&(fpd->fp))[1] = wrd2;
#else
	((uae_u32*)&(fpd->fp))[1] = wrd1;
	((uae_u32*)&(fpd->fp))[0] = wrd2;
#endif
}
static void fpp_from_double(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2)
{
#ifdef WORDS_BIGENDIAN
  *wrd1 = ((uae_u32*)&(fpd->fp))[0];
  *wrd2 = ((uae_u32*)&(fpd->fp))[1];
#else
  *wrd1 = ((uae_u32*)&(fpd->fp))[1];
  *wrd2 = ((uae_u32*)&(fpd->fp))[0];
#endif
}
void fpp_to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
  double frac;
  if ((wrd1 & 0x7fff0000) == 0 && wrd2 == 0 && wrd3 == 0) {
      fpd->fp = (wrd1 & 0x80000000) ? -0.0 : +0.0;
      return;
  }
  frac = ((double)wrd2 + ((double)wrd3 / twoto32)) / 2147483648.0;
  if (wrd1 & 0x80000000)
      frac = -frac;
  fpd->fp = ldexp (frac, ((wrd1 >> 16) & 0x7fff) - 16383);
}
static void fpp_from_exten(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2, uae_u32 *wrd3)
{
  int expon;
  double frac;
  fptype v;
  
  v = fpd->fp;
  if (v == 0.0) {
    *wrd1 = signbit(v) ? 0x80000000 : 0;
    *wrd2 = 0;
    *wrd3 = 0;
    return;
  }
  if (v < 0) {
    *wrd1 = 0x80000000;
    v = -v;
  } else {
    *wrd1 = 0;
  }
  frac = frexp (v, &expon);
  frac += 0.5 / (twoto32 * twoto32);
  if (frac >= 1.0) {
    frac /= 2.0;
    expon++;
  }
  *wrd1 |= (((expon + 16383 - 1) & 0x7fff) << 16);
  *wrd2 = (uae_u32) (frac * twoto32);
  *wrd3 = (uae_u32) ((frac * twoto32 - *wrd2) * twoto32);
}

#if USE_HOST_ROUNDING == 0
#define fpp_round_to_minus_infinity(x) floor(x)
#define fpp_round_to_plus_infinity(x) ceil(x)
#define fpp_round_to_zero(x)	((x) >= 0.0 ? floor(x) : ceil(x))
#define fpp_round_to_nearest(x) round(x)
#endif // USE_HOST_ROUNDING

static uae_s64 fpp_to_int(fpdata *src, int size)
{
  static const fptype fxsizes[6] =
  {
           -128.0,        127.0,
         -32768.0,      32767.0,
    -2147483648.0, 2147483647.0
  };

	fptype fp = src->fp;
	if (fpp_is_nan(src)) {
		uae_u32 w1, w2, w3;
		fpp_from_exten(src, &w1, &w2, &w3);
		uae_s64 v = 0;
		// return mantissa
		switch (size)
		{
			case 0:
				v = w2 >> 24;
				break;
			case 1:
				v = w2 >> 16;
				break;
			case 2:
				v = w2 >> 0;
				break;
		}
		return v;
	}
	if (fp < fxsizes[size * 2 + 0]) {
		fp = fxsizes[size * 2 + 0];
	}
	if (fp > fxsizes[size * 2 + 1]) {
		fp = fxsizes[size * 2 + 1];
	}
#if USE_HOST_ROUNDING
	return lrintl(fp);
#else
	uae_s64 result = (int)fp;
	switch (regs.fpcr & 0x30)
	{
		case FPCR_ROUND_ZERO:
			result = (int)fpp_round_to_zero (fp);
			break;
		case FPCR_ROUND_MINF:
			result = (int)fpp_round_to_minus_infinity (fp);
			break;
		case FPCR_ROUND_NEAR:
			result = fpp_round_to_nearest (fp);
			break;
		case FPCR_ROUND_PINF:
			result = (int)fpp_round_to_plus_infinity (fp);
			break;
	}
	return result;
#endif
}
static void fpp_from_int(fpdata *fpd, uae_s32 src)
{
  fpd->fp = (fptype) src;
}


/* Functions for rounding */

// round to float with extended precision exponent
static void fpp_round32(fpdata *fpd)
{
  int expon;
  float mant;
  mant = (float)(frexpl(fpd->fp, &expon) * 2.0);
  fpd->fp = ldexpl((fptype)mant, expon - 1);
}

// round to double with extended precision exponent
static void fpp_round64(fpdata *fpd)
{
#if !defined(CPU_arm)
	int expon;
  double mant;
  mant = (double)(frexpl(fpd->fp, &expon) * 2.0);
  fpd->fp = ldexpl((fptype)mant, expon - 1);
#endif
}

// round to float
static void fpp_round_single(fpdata *fpd)
{
  fpd->fp = (float) fpd->fp;
}

static void fpp_round_prec(fpdata *fpd, int prec)
{
	if (prec == 32) {
		fpp_round_single(fpd);
	}
}

static void fp_round(fpdata *fpd)
{
	fpp_round_prec(fpd, fpu_prec);
}

/* Arithmetic functions */

static void fpp_move(fpdata *a, fpdata *b, int prec)
{
	a->fp = b->fp;
	if (prec == 32)
		fpp_round_single(a);
}

static void fpp_int(fpdata *a, fpdata *b)
{
	fptype bb = b->fp;
#if USE_HOST_ROUNDING
	a->fp = rintl(bb);
#else
  switch (regs.fpcr & FPCR_ROUNDING_MODE)
  {
    case FPCR_ROUND_NEAR:
      a->fp = fpp_round_to_nearest(bb);
      break;
    case FPCR_ROUND_ZERO:
      a->fp = fpp_round_to_zero(bb);
      break;
    case FPCR_ROUND_MINF:
      a->fp = fpp_round_to_minus_infinity(bb);
      break;
    case FPCR_ROUND_PINF:
      a->fp = fpp_round_to_plus_infinity(bb);
      break;
    default: /* never reached */
  		break;
  }
#endif
}

static void fpp_getexp(fpdata *a, fpdata *b)
{
  int expon;
  frexpl(b->fp, &expon);
  a->fp = (fptype) (expon - 1);
	fp_round(a);
}
static void fpp_getman(fpdata *a, fpdata *b)
{
  int expon;
  a->fp = frexpl(b->fp, &expon) * 2.0;
	fp_round(a);
}
static void fpp_div(fpdata *a, fpdata *b, int prec)
{
	a->fp = a->fp / b->fp;
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_mod(fpdata *a, fpdata *b, uae_u64 *q, uae_u8 *s)
{
  fptype quot;
#if USE_HOST_ROUNDING
  quot = truncl(a->fp / b->fp);
#else
  quot = fpp_round_to_zero(a->fp / b->fp);
#endif
  if (quot < 0.0) {
    *s = 1;
    quot = -quot;
  } else {
    *s = 0;
  }
  *q = (uae_u64)quot;
  a->fp = fmodl(a->fp, b->fp);
	fp_round(a);
}

static void fpp_rem(fpdata *a, fpdata *b, uae_u64 *q, uae_u8 *s)
{
  fptype quot;
#if USE_HOST_ROUNDING
  quot = roundl(a->fp / b->fp);
#else
  quot = fpp_round_to_nearest(a->fp / b->fp);
#endif
  if (quot < 0.0) {
    *s = 1;
    quot = -quot;
  } else {
    *s = 0;
  }
  *q = (uae_u64)quot;
  a->fp = remainderl(a->fp, b->fp);
	fp_round(a);
}

static void fpp_scale(fpdata *a, fpdata *b)
{
	a->fp = ldexpl(a->fp, (int)b->fp);
	fp_round(a);
}

static void fpp_sinh(fpdata *a, fpdata *b)
{
	a->fp = sinhl(b->fp);
	fp_round(a);
}
static void fpp_intrz(fpdata *a, fpdata *b)
{
#if USE_HOST_ROUNDING
    a->fp = truncl(b->fp);
#else
    a->fp = fpp_round_to_zero (b->fp);
#endif
	fp_round(a);
}
static void fpp_sqrt(fpdata *a, fpdata *b, int prec)
{
	a->fp = sqrtl(b->fp);
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_lognp1(fpdata *a, fpdata *b)
{
	a->fp = log1pl(b->fp);
	fp_round(a);
}
static void fpp_etoxm1(fpdata *a, fpdata *b)
{
	a->fp = expm1l(b->fp);
	fp_round(a);
}
static void fpp_tanh(fpdata *a, fpdata *b)
{
	a->fp = tanhl(b->fp);
	fp_round(a);
}
static void fpp_atan(fpdata *a, fpdata *b)
{
	a->fp = atanl(b->fp);
	fp_round(a);
}
static void fpp_atanh(fpdata *a, fpdata *b)
{
	a->fp = atanhl(b->fp);
	fp_round(a);
}
static void fpp_sin(fpdata *a, fpdata *b)
{
	a->fp = sinl(b->fp);
	fp_round(a);
}
static void fpp_asin(fpdata *a, fpdata *b)
{
	a->fp = asinl(b->fp);
	fp_round(a);
}
static void fpp_tan(fpdata *a, fpdata *b)
{
	a->fp = tanl(b->fp);
	fp_round(a);
}
static void fpp_etox(fpdata *a, fpdata *b)
{
	a->fp = expl(b->fp);
	fp_round(a);
}
static void fpp_twotox(fpdata *a, fpdata *b)
{
	a->fp = powl(2.0, b->fp);
	fp_round(a);
}
static void fpp_tentox(fpdata *a, fpdata *b)
{
	a->fp = powl(10.0, b->fp);
	fp_round(a);
}
static void fpp_logn(fpdata *a, fpdata *b)
{
	a->fp = logl(b->fp);
	fp_round(a);
}
static void fpp_log10(fpdata *a, fpdata *b)
{
	a->fp = log10l(b->fp);
	fp_round(a);
}
static void fpp_log2(fpdata *a, fpdata *b)
{
	a->fp = log2l(b->fp);
	fp_round(a);
}
static void fpp_abs(fpdata *a, fpdata *b, int prec)
{
	a->fp = b->fp < 0.0 ? -b->fp : b->fp;
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_cosh(fpdata *a, fpdata *b)
{
	a->fp = coshl(b->fp);
	fp_round(a);
}
static void fpp_neg(fpdata *a, fpdata *b, int prec)
{
	a->fp = -b->fp;
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_acos(fpdata *a, fpdata *b)
{
	a->fp = acosl(b->fp);
	fp_round(a);
}
static void fpp_cos(fpdata *a, fpdata *b)
{
	a->fp = cosl(b->fp);
	fp_round(a);
}
static void fpp_sub(fpdata *a, fpdata *b, int prec)
{
	a->fp = a->fp - b->fp;
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_add(fpdata *a, fpdata *b, int prec)
{
	a->fp = a->fp + b->fp;
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_mul(fpdata *a, fpdata *b, int prec)
{
	a->fp = a->fp * b->fp;
	if (prec == 32)
		fpp_round_single(a);
}
static void fpp_sglmul(fpdata *a, fpdata *b)
{
  fptype z;
  float mant;
  int expon;
  /* FIXME: truncate mantissa of a and b to single precision */
  z = a->fp * b->fp;

  mant = (float)(frexpl(z, &expon) * 2.0);
  a->fp = ldexpl((fptype)mant, expon - 1);
}
static void fpp_sgldiv(fpdata *a, fpdata *b)
{
  fptype z;
  float mant;
  int expon;
  z = a->fp / b->fp;

  mant = (float)(frexpl(z, &expon) * 2.0);
  a->fp = ldexpl((fptype)mant, expon - 1);
}

static void fpp_cmp(fpdata *a, fpdata *b)
{
	fptype v = 1.0;
	if (currprefs.fpu_strict) {
		bool a_neg = fpp_is_neg(a);
		bool b_neg = fpp_is_neg(b);
		bool a_inf = fpp_is_infinity(a);
		bool b_inf = fpp_is_infinity(b);
		bool a_zero = fpp_is_zero(a);
		bool b_zero = fpp_is_zero(b);
		bool a_nan = fpp_is_nan(a);
		bool b_nan = fpp_is_nan(b);

		if (a_nan || b_nan) {
			// FCMP never returns N + NaN
			v = *fp_nan;
		} else if (a_zero && b_zero) {
			if ((a_neg && b_neg) || (a_neg && !b_neg))
				v = -0.0;
			else
				v = 0.0;
		} else if (a_zero && b_inf) {
			if (!b_neg)
				v = -1.0;
			else
				v = 1.0;
		} else if (a_inf && b_zero) {
			if (!a_neg)
				v = -1.0;
			else
				v = 1.0;
		} else if (a_inf && b_inf) {
			if (a_neg == b_neg)
				v = 0.0;
			if ((a_neg && b_neg) || (a_neg && !b_neg))
				v = -v;
		} else if (a_inf) {
			if (a_neg)
				v = -1.0;
		} else if (b_inf) {
			if (!b_neg)
				v = -1.0;
		} else {
		  v = a->fp - b->fp;
		}
	} else {
		v = a->fp - b->fp;
	}
	a->fp = v;
}

static void fpp_tst(fpdata *a, fpdata *b)
{
	a->fp = b->fp;
}

static void fpp_from_pack (fpdata *src, uae_u32 *wrd, int kfactor)
{
	int i, j, t;
	int exp;
	int ndigits;
	char *cp, *strp;
	char str[100];
	fptype fp;

  if (fpp_is_nan (src)) {
    // copy bit by bit, handle signaling nan
    fpp_from_exten(src, &wrd[0], &wrd[1], &wrd[2]);
    return;
  }
  if (fpp_is_infinity (src)) {
    // extended exponent and all 0 packed fraction
    fpp_from_exten(src, &wrd[0], &wrd[1], &wrd[2]);
    wrd[1] = wrd[2] = 0;
    return;
  }

	wrd[0] = wrd[1] = wrd[2] = 0;

	fpp_to_native(&fp, src);

	sprintf (str, "%#.17e", fp);
	
	// get exponent
	cp = str;
	while (*cp != 'e') {
		if (*cp == 0)
			return;
		cp++;
	}
	cp++;
	if (*cp == '+')
		cp++;
	exp = atoi (cp);

	// remove trailing zeros
	cp = str;
	while (*cp != 'e') {
		cp++;
	}
	cp[0] = 0;
	cp--;
	while (cp > str && *cp == '0') {
		*cp = 0;
		cp--;
	}

	cp = str;
	// get sign
	if (*cp == '-') {
		cp++;
		wrd[0] = 0x80000000;
	} else if (*cp == '+') {
		cp++;
	}
	strp = cp;

	if (kfactor <= 0) {
		ndigits = abs (exp) + (-kfactor) + 1;
	} else {
		if (kfactor > 17) {
			kfactor = 17;
			fpsr_set_exception(FPSR_OPERR);
		}
		ndigits = kfactor;
	}

	if (ndigits < 0)
		ndigits = 0;
	if (ndigits > 16)
		ndigits = 16;

	// remove decimal point
	strp[1] = strp[0];
	strp++;
	// add trailing zeros
	i = strlen (strp);
	cp = strp + i;
	while (i < ndigits) {
		*cp++ = '0';
		i++;
	}
	i = ndigits + 1;
	while (i < 17) {
		strp[i] = 0;
		i++;
	}
	*cp = 0;
	i = ndigits - 1;
	// need to round?
	if (i >= 0 && strp[i + 1] >= '5') {
		while (i >= 0) {
			strp[i]++;
			if (strp[i] <= '9')
				break;
			if (i == 0) {
				strp[i] = '1';
				exp++;
			} else {
				strp[i] = '0';
			}
			i--;
		}
	}
	strp[ndigits] = 0;

	// store first digit of mantissa
	cp = strp;
	wrd[0] |= *cp++ - '0';

	// store rest of mantissa
	for (j = 1; j < 3; j++) {
		for (i = 0; i < 8; i++) {
			wrd[j] <<= 4;
			if (*cp >= '0' && *cp <= '9')
				wrd[j] |= *cp++ - '0';
		}
	}

	// exponent
	if (exp < 0) {
		wrd[0] |= 0x40000000;
		exp = -exp;
	}
	if (exp > 9999) // ??
		exp = 9999;
	if (exp > 999) {
		int d = exp / 1000;
		wrd[0] |= d << 12;
		exp -= d * 1000;
		fpsr_set_exception(FPSR_OPERR);
	}
	i = 100;
	t = 0;
	while (i >= 1) {
		int d = exp / i;
		t <<= 4;
		t |= d;
		exp -= d * i;
		i /= 10;
	}
	wrd[0] |= t << 16;
}

static void fpp_to_pack (fpdata *fpd, uae_u32 *wrd, int dummy)
{
	fptype d;
	char *cp;
	char str[100];

  if (((wrd[0] >> 16) & 0x7fff) == 0x7fff) {
    // infinity has extended exponent and all 0 packed fraction
    // nans are copies bit by bit
    fpp_to_exten(fpd, wrd[0], wrd[1], wrd[2]);
    return;
  }
  if (!(wrd[0] & 0xf) && !wrd[1] && !wrd[2]) {
    // exponent is not cared about, if mantissa is zero
    wrd[0] &= 0x80000000;
    fpp_to_exten(fpd, wrd[0], wrd[1], wrd[2]);
    return;
  }

	cp = str;
	if (wrd[0] & 0x80000000)
		*cp++ = '-';
	*cp++ = (wrd[0] & 0xf) + '0';
	*cp++ = '.';
	*cp++ = ((wrd[1] >> 28) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 24) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 20) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 16) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 12) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 8) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 4) & 0xf) + '0';
	*cp++ = ((wrd[1] >> 0) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 28) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 24) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 20) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 16) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 12) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 8) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 4) & 0xf) + '0';
	*cp++ = ((wrd[2] >> 0) & 0xf) + '0';
	*cp++ = 'E';
	if (wrd[0] & 0x40000000)
		*cp++ = '-';
	*cp++ = ((wrd[0] >> 24) & 0xf) + '0';
	*cp++ = ((wrd[0] >> 20) & 0xf) + '0';
	*cp++ = ((wrd[0] >> 16) & 0xf) + '0';
	*cp = 0;
	sscanf (str, "%le", &d);
	fpp_from_native(d, fpd);
}


double softfloat_tan(double v)
{
	return tanl(v);
}
