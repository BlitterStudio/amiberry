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

#include <cmath>
#include <cfloat>
#include <cfenv>

#include "sysconfig.h"
#include "sysdeps.h"

#define USE_HOST_ROUNDING 1
#ifndef AMIBERRY
#define SOFTFLOAT_CONVERSIONS 0
#endif

#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "fpp.h"
#include "uae/attributes.h"
#include "uae/vm.h"
#include "newcpu.h"

#ifdef JIT
uae_u32 xhex_exp_1[] ={0xa2bb4a9a, 0xadf85458, 0x4000};
uae_u32 xhex_ln_10[] ={0xaaa8ac17, 0x935d8ddd, 0x4000};
uae_u32 xhex_l10_2[] ={0xfbcff798, 0x9a209a84, 0x3ffd};
uae_u32 xhex_l10_e[] ={0x37287195, 0xde5bd8a9, 0x3ffd};
uae_u32 xhex_1e16[]  ={0x04000000, 0x8e1bc9bf, 0x4034};
uae_u32 xhex_1e32[]  ={0x2b70b59e, 0x9dc5ada8, 0x4069};
uae_u32 xhex_1e64[]  ={0xffcfa6d5, 0xc2781f49, 0x40d3};
uae_u32 xhex_1e128[] ={0x80e98ce0, 0x93ba47c9, 0x41a8};
uae_u32 xhex_1e256[] ={0x9df9de8e, 0xaa7eebfb, 0x4351};
uae_u32 xhex_1e512[] ={0xa60e91c7, 0xe319a0ae, 0x46a3};
uae_u32 xhex_1e1024[]={0x81750c17, 0xc9767586, 0x4d48};
uae_u32 xhex_1e2048[]={0xc53d5de5, 0x9e8b3b5d, 0x5a92};
uae_u32 xhex_1e4096[]={0x8a20979b, 0xc4605202, 0x7525};
double fp_1e8 = 1.0e8;
float  fp_1e0 = 1, fp_1e1 = 10, fp_1e2 = 100, fp_1e4 = 10000;
#endif

#ifdef USE_LONG_DOUBLE
static uae_u32 xhex_nan[]   ={0xffffffff, 0xffffffff, 0x7fff};
static long double *fp_nan    = (long double *)xhex_nan;
#else
static uae_u32 dhex_nan[]   ={0xffffffff, 0x7fffffff};
static double *fp_nan    = (double *)dhex_nan;
#endif
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

#ifdef SOFTFLOAT_CONVERSIONS
static struct float_status fs;
#endif
static uae_u32 fpu_mode_control = 0;
static int fpu_prec;
static int temp_prec;

#if defined(CPU_i386) || defined(CPU_x86_64)

/* The main motivation for dynamically creating an x86(-64) function in
 * memory is because MSVC (x64) does not allow you to use inline assembly,
 * and the x86-64 versions of _control87/_controlfp functions only modifies
 * SSE2 registers. */

static uae_u16 x87_cw = 0;
static uae_u8 *x87_fldcw_code = NULL;
typedef void (uae_cdecl *x87_fldcw_function)(void);

void init_fpucw_x87(void)
{
	if (x87_fldcw_code) {
		return;
	}
	x87_fldcw_code = (uae_u8 *) uae_vm_alloc(
		uae_vm_page_size(), UAE_VM_32BIT, UAE_VM_READ_WRITE_EXECUTE);
	uae_u8 *c = x87_fldcw_code;
	/* mov eax,0x0 */
	*(c++) = 0xb8;
	*(c++) = 0x00;
	*(c++) = 0x00;
	*(c++) = 0x00;
	*(c++) = 0x00;
#ifdef CPU_x86_64
	/* Address override prefix */
	*(c++) = 0x67;
#endif
	/* fldcw WORD PTR [eax+addr] */
	*(c++) = 0xd9;
	*(c++) = 0xa8;
	*(c++) = (((uintptr_t) &x87_cw)      ) & 0xff;
	*(c++) = (((uintptr_t) &x87_cw) >>  8) & 0xff;
	*(c++) = (((uintptr_t) &x87_cw) >> 16) & 0xff;
	*(c++) = (((uintptr_t) &x87_cw) >> 24) & 0xff;
	/* ret */
	*(c++) = 0xc3;
	/* Write-protect the function */
	uae_vm_protect(x87_fldcw_code, uae_vm_page_size(), UAE_VM_READ_EXECUTE);
}

static void set_fpucw_x87(uae_u32 m68k_cw)
{
#ifdef _MSC_VER
	static int ex = 0;
	// RN, RZ, RM, RP
	static const unsigned int fp87_round[4] = { _RC_NEAR, _RC_CHOP, _RC_DOWN, _RC_UP };
	// Extend X, Single S, Double D, Undefined
	static const unsigned int fp87_prec[4] = { _PC_53, _PC_24, _PC_53, 0 };
	int round = (m68k_cw >> 4) & 3;
#ifdef WIN64
	// x64 only sets SSE2, must also call x87_fldcw_code() to set FPU rounding mode.
	_controlfp(ex | fp87_round[round], _MCW_RC);
#else
	int prec = (m68k_cw >> 6) & 3;
	// x86 sets both FPU and SSE2 rounding mode, don't need x87_fldcw_code()
	_control87(ex | fp87_round[round] | fp87_prec[prec], _MCW_RC | _MCW_PC);
	return;
#endif
#endif
	static const uae_u16 x87_cw_tab[] = {
#ifdef USE_LONG_DOUBLE
		0x137f, 0x1f7f, 0x177f, 0x1b7f,	/* Extended */
#else
		0x127f, 0x1e7f, 0x167f, 0x1a7f,	/* Double */
#endif
		0x107f, 0x1c7f, 0x147f, 0x187f,	/* Single */
		0x127f, 0x1e7f, 0x167f, 0x1a7f,	/* Double */
		0x127f, 0x1e7f, 0x167f, 0x1a7f,	/* undefined (Double) */
	};
	x87_cw = x87_cw_tab[(m68k_cw >> 4) & 0xf];
#if defined(X86_MSVC_ASSEMBLY) && 0
	__asm { fldcw word ptr x87_cw }
#elif defined(__GNUC__) && 0
	__asm__("fldcw %0" : : "m" (*&x87_cw));
#else
	((x87_fldcw_function) x87_fldcw_code)();
#endif
}

#endif /* defined(CPU_i386) || defined(CPU_x86_64) */

static void native_set_fpucw(uae_u32 m68k_cw)
{
#if defined(CPU_i386) || defined(CPU_x86_64)
	set_fpucw_x87(m68k_cw);
#endif
}

/* Functions for setting host/library modes and getting status */
static void fp_set_mode(uae_u32 mode_control)
{
	if (mode_control == fpu_mode_control && !currprefs.compfpu)
		return;
    switch(mode_control & FPCR_ROUNDING_PRECISION) {
        case FPCR_PRECISION_EXTENDED: // X
			fpu_prec = PREC_EXTENDED;
            break;
        case FPCR_PRECISION_SINGLE:   // S
			fpu_prec = PREC_FLOAT;
            break;
        case FPCR_PRECISION_DOUBLE:   // D
        default:                      // undefined
			fpu_prec = PREC_DOUBLE;
            break;
    }
#if USE_HOST_ROUNDING
	if ((mode_control & FPCR_ROUNDING_MODE) != (fpu_mode_control & FPCR_ROUNDING_MODE)) {
		switch(mode_control & FPCR_ROUNDING_MODE) {
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
#ifndef AMIBERRY
	native_set_fpucw(mode_control);
#endif
#endif
	fpu_mode_control = mode_control;
}


static void fp_get_status(uae_u32 *status)
{
	// These can't be properly emulated using host FPU.
#if 0
    int exp_flags = fetestexcept(FE_ALL_EXCEPT);
    if (exp_flags) {
        if (exp_flags & FE_INEXACT)
            *status |= FPSR_INEX2;
        if (exp_flags & FE_DIVBYZERO)
            *status |= FPSR_DZ;
        if (exp_flags & FE_UNDERFLOW)
            *status |= FPSR_UNFL;
        if (exp_flags & FE_OVERFLOW)
            *status |= FPSR_OVFL;
        if (exp_flags & FE_INVALID)
            *status |= FPSR_OPERR;
    }
	/* FIXME: how to detect SNAN? */
#endif
}

static uae_u32 fp_get_support_flags(void)
{
	return 0;
}

static void fp_clear_status(void)
{
#if 0
    feclearexcept (FE_ALL_EXCEPT);
#endif
}

/* Functions for detecting float type */
static bool fp_is_init(fpdata *fpd)
{
	return false;
}
static bool fp_is_snan(fpdata *fpd)
{
    return 0; /* FIXME: how to detect SNAN */
}
static bool fp_unset_snan(fpdata *fpd)
{
    /* FIXME: how to unset SNAN */
	return 0;
}
static bool fp_is_nan(fpdata *fpd)
{
    return isnan(fpd->fp) != 0;
}
static bool fp_is_infinity(fpdata *fpd)
{
    return isinf(fpd->fp) != 0;
}
static bool fp_is_zero(fpdata *fpd)
{
    return fpd->fp == 0.0;
}
static bool fp_is_neg(fpdata *fpd)
{
    return signbit(fpd->fp) != 0;
}
static bool fp_is_denormal(fpdata *fpd)
{
    return false;
	//return (isnormal(fpd->fp) == 0); /* FIXME: how to differ denormal/unnormal? */
}
static bool fp_is_unnormal(fpdata *fpd)
{
	return false;
    //return (isnormal(fpd->fp) == 0); /* FIXME: how to differ denormal/unnormal? */
}

/* Functions for converting between float formats */
/* FIXME: how to preserve/fix denormals and unnormals? */

static void fp_to_native(fptype *fp, fpdata *fpd)
{
    *fp = fpd->fp;
}
static void fp_from_native(fptype fp, fpdata *fpd)
{
    fpd->fp = fp;
}

static void fp_to_single(fpdata *fpd, uae_u32 wrd1)
{
    union {
        float f;
        uae_u32 u;
    } val;
    
    val.u = wrd1;
    fpd->fp = (fptype) val.f;
}
static uae_u32 fp_from_single(fpdata *fpd)
{
    union {
        float f;
        uae_u32 u;
    } val;
    
    val.f = (float) fpd->fp;
    return val.u;
}

static void fp_to_double(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2)
{
    union {
        double d;
        uae_u32 u[2];
    } val;
    
#ifdef WORDS_BIGENDIAN
    val.u[0] = wrd1;
    val.u[1] = wrd2;
#else
    val.u[1] = wrd1;
    val.u[0] = wrd2;
#endif
    fpd->fp = (fptype) val.d;
}
static void fp_from_double(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2)
{
    union {
        double d;
        uae_u32 u[2];
    } val;
    
    val.d = (double) fpd->fp;
#ifdef WORDS_BIGENDIAN
    *wrd1 = val.u[0];
    *wrd2 = val.u[1];
#else
    *wrd1 = val.u[1];
    *wrd2 = val.u[0];
#endif
}
#ifdef USE_LONG_DOUBLE
static void fp_to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
    union {
        long double ld;
        uae_u32 u[3];
    } val;

#if WORDS_BIGENDIAN
    val.u[0] = (wrd1 & 0xffff0000) | ((wrd2 & 0xffff0000) >> 16);
    val.u[1] = (wrd2 & 0x0000ffff) | ((wrd3 & 0xffff0000) >> 16);
    val.u[2] = (wrd3 & 0x0000ffff) << 16;
#else
    val.u[0] = wrd3;
    val.u[1] = wrd2;
    val.u[2] = wrd1 >> 16;
#endif
    fpd->fp = val.ld;
}
static void fp_from_exten(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2, uae_u32 *wrd3)
{
    union {
        long double ld;
        uae_u32 u[3];
    } val;
    
    val.ld = fpd->fp;
#if WORDS_BIGENDIAN
    *wrd1 = val.u[0] & 0xffff0000;
    *wrd2 = ((val.u[0] & 0x0000ffff) << 16) | ((val.u[1] & 0xffff0000) >> 16);
    *wrd3 = ((val.u[1] & 0x0000ffff) << 16) | ((val.u[2] & 0xffff0000) >> 16);
#else
    *wrd3 = val.u[0];
    *wrd2 = val.u[1];
    *wrd1 = val.u[2] << 16;
#endif
}
#else // if !USE_LONG_DOUBLE
#ifdef CPU_AARCH64
void fp_to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
#else
static void fp_to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
#endif
{
#if SOFTFLOAT_CONVERSIONS
	floatx80 fx80;
	fx80.high = wrd1 >> 16;
	fx80.low = (((uae_u64)wrd2) << 32) | wrd3;
	fs.float_exception_flags = 0;
	float64 f = floatx80_to_float64(fx80, &fs);
	// overflow -> infinity
	if (fs.float_exception_flags & float_flag_overflow)
		f = 0x7ff0000000000000 | (f & 0x8000000000000000);
	fp_to_double(fpd, f >> 32, (uae_u32)f);
#else
    double frac;
    if ((wrd1 & 0x7fff0000) == 0 && wrd2 == 0 && wrd3 == 0) {
        fpd->fp = (wrd1 & 0x80000000) ? -0.0 : +0.0;
        return;
    }
    frac = ((double)wrd2 + ((double)wrd3 / twoto32)) / 2147483648.0;
    if (wrd1 & 0x80000000)
        frac = -frac;
    fpd->fp = ldexp (frac, ((wrd1 >> 16) & 0x7fff) - 16383);
#endif
}
static void fp_from_exten(fpdata *fpd, uae_u32 *wrd1, uae_u32 *wrd2, uae_u32 *wrd3)
{
#if SOFTFLOAT_CONVERSIONS
	uae_u32 w1, w2;
	fp_from_double(fpd, &w1, &w2);
	floatx80 f = float64_to_floatx80(((uae_u64)w1 << 32) | w2, &fs);
	*wrd1 = f.high << 16;
	*wrd2 = f.low >> 32;
	*wrd3 = (uae_u32)f.low;
#else
    int expon;
    double frac;
    fptype v;
    
    if (fp_is_zero(fpd)) {
        *wrd1 = signbit(fpd->fp) ? 0x80000000 : 0;
        *wrd2 = 0;
        *wrd3 = 0;
        return;
	} else if (fp_is_nan(fpd)) {
		*wrd1 = 0x7fff0000;
		*wrd2 = 0xffffffff;
		*wrd3 = 0xffffffff;
		return;
	}
	v = fpd->fp;
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
#endif
}
#endif // !USE_LONG_DOUBLE

#if USE_HOST_ROUNDING == 0
#ifdef USE_LONG_DOUBLE
#define fp_round_to_minus_infinity(x) floorl(x)
#define fp_round_to_plus_infinity(x) ceill(x)
#define fp_round_to_zero(x)	((x) >= 0.0 ? floorl(x) : ceill(x))
#define fp_round_to_nearest(x) roundl(x)
#else // if !USE_LONG_DOUBLE
#define fp_round_to_minus_infinity(x) floor(x)
#define fp_round_to_plus_infinity(x) ceil(x)
#define fp_round_to_zero(x)	((x) >= 0.0 ? floor(x) : ceil(x))
#define fp_round_to_nearest(x) round(x)
#endif // !USE_LONG_DOUBLE
#endif // USE_HOST_ROUNDING

static uae_s64 fp_to_int(fpdata *src, int size)
{
    static const fptype fxsizes[6] =
    {
               -128.0,        127.0,
             -32768.0,      32767.0,
        -2147483648.0, 2147483647.0
    };

	fptype fp = src->fp;
	fp_is_init(src);
	if (fp_is_nan(src)) {
		uae_u32 w1, w2, w3;
		fp_from_exten(src, &w1, &w2, &w3);
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
			result = (int)fp_round_to_zero (fp);
			break;
		case FPCR_ROUND_MINF:
			result = (int)fp_round_to_minus_infinity (fp);
			break;
		case FPCR_ROUND_NEAR:
			result = fp_round_to_nearest (fp);
			break;
		case FPCR_ROUND_PINF:
			result = (int)fp_round_to_plus_infinity (fp);
			break;
	}
	return result;
#endif
}
static void fp_from_int(fpdata *fpd, uae_s32 src)
{
    fpd->fp = (fptype) src;
}


/* Functions for rounding */

// round to float with extended precision exponent
static void fp_round32(fpdata *fpd)
{
    int expon;
    float mant;
    mant = (float)(frexpl(fpd->fp, &expon) * 2.0);
    fpd->fp = ldexpl((fptype)mant, expon - 1);
}

// round to double with extended precision exponent
static void fp_round64(fpdata *fpd)
{
    int expon;
    double mant;
    mant = (double)(frexpl(fpd->fp, &expon) * 2.0);
    fpd->fp = ldexpl((fptype)mant, expon - 1);
}

// round to float
static void fp_round_single(fpdata *fpd)
{
    fpd->fp = (float) fpd->fp;
}

// round to double
static void fp_round_double(fpdata *fpd)
{
#ifdef USE_LONG_DOUBLE
	fpd->fp = (double) fpd->fp;
#endif
}

static const TCHAR *fp_print(fpdata *fpd, int mode)
{
	static TCHAR fsout[32];
	bool n;

	if (mode < 0) {
		uae_u32 w1, w2, w3;
		fp_from_exten(fpd, &w1, &w2, &w3);
		_stprintf(fsout, _T("%04X-%08X-%08X"), w1 >> 16, w2, w3);
		return fsout;
	}

	n = signbit(fpd->fp) ? 1 : 0;

	if(isinf(fpd->fp)) {
		_stprintf(fsout, _T("%c%s"), n ? '-' : '+', _T("inf"));
	} else if(isnan(fpd->fp)) {
		_stprintf(fsout, _T("%c%s"), n ? '-' : '+', _T("nan"));
	} else {
#ifdef USE_LONG_DOUBLE
		_stprintf(fsout, _T("#%Le"), fpd->fp);
#else
		_stprintf(fsout, _T("#%e"), fpd->fp);
#endif
	}
	if (mode == 0 || mode > _tcslen(fsout))
		return fsout;
	fsout[mode] = 0;
	return fsout;
}

static void fp_round_prec(fpdata *fpd, int prec)
{
	if (prec == PREC_DOUBLE) {
		fp_round_double(fpd);
	} else if (prec == PREC_FLOAT) {
		fp_round_single(fpd);
	}
}

static void fp_round(fpdata *fpd)
{
	if (!currprefs.fpu_strict)
		return;
	fp_round_prec(fpd, fpu_prec);
}


static void fp_set_prec(int prec)
{
#if 0
	temp_fpu_mode_control = fpu_mode_control;
	if (prec && fpu_prec > prec) {
		fpu_mode_control &= ~FPCR_ROUNDING_PRECISION;
		switch (prec)
		{
			case PREC_EXTENDED:
			fpu_mode_control |= FPCR_PRECISION_EXTENDED;
			break;
			case PREC_DOUBLE:
			default:
			fpu_mode_control |= FPCR_PRECISION_DOUBLE;
			break;
			case PREC_FLOAT:
			fpu_mode_control |= FPCR_PRECISION_SINGLE;
			break;
		}
		fp_set_mode(fpu_mode_control);
	}
#endif
	temp_prec = prec;
}
static void fp_reset_prec(fpdata *fpd)
{
#if 0
	fp_set_mode(temp_fpu_mode_control);
#else
	int prec = temp_prec;
	if (temp_prec == PREC_NORMAL)
		prec = fpu_prec;
	fp_round_prec(fpd, prec);
#endif
}

// Use default precision/rounding mode when calling C-library math functions.
static void fp_normal_prec(void)
{
	temp_prec = fpu_mode_control;
#ifdef USE_LONG_DOUBLE
	if ((fpu_mode_control & FPCR_ROUNDING_PRECISION) != FPCR_PRECISION_EXTENDED || (fpu_mode_control & FPCR_ROUNDING_MODE) != FPCR_ROUND_NEAR) {
		fp_set_mode(FPCR_PRECISION_EXTENDED | FPCR_ROUND_NEAR);
	}
#else
	if ((fpu_mode_control & FPCR_ROUNDING_PRECISION) == FPCR_PRECISION_SINGLE || (fpu_mode_control & FPCR_ROUNDING_MODE) != FPCR_ROUND_NEAR) {
		fp_set_mode(FPCR_PRECISION_DOUBLE | FPCR_ROUND_NEAR);
	}
#endif
}

static void fp_reset_normal_prec(void)
{
	fp_set_mode(temp_prec);
}

/* Arithmetic functions */

static void fp_move(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = b->fp;
	fp_reset_prec(a);
}

static void fp_int(fpdata *a, fpdata *b)
{
	fptype bb = b->fp;
#if USE_HOST_ROUNDING
	a->fp = rintl(bb);
#else
    switch (regs.fpcr & FPCR_ROUNDING_MODE)
    {
        case FPCR_ROUND_NEAR:
            a->fp = fp_round_to_nearest(bb);
        case FPCR_ROUND_ZERO:
            a->fp = fp_round_to_zero(bb);
        case FPCR_ROUND_MINF:
            a->fp = fp_round_to_minus_infinity(bb);
        case FPCR_ROUND_PINF:
            a->fp = fp_round_to_plus_infinity(bb);
        default: /* never reached */
		break;
    }
#endif
}

static void fp_getexp(fpdata *a, fpdata *b)
{
    int expon;
	fp_normal_prec();
	frexpl(b->fp, &expon);
	a->fp = (fptype)expon - 1;
	fp_reset_normal_prec();
}
static void fp_getman(fpdata *a, fpdata *b)
{
    int expon;
	fp_normal_prec();
	a->fp = frexpl(b->fp, &expon) * 2.0;
	fp_reset_normal_prec();
}
static void fp_div(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = a->fp / b->fp;
	fp_reset_prec(a);
}
static void fp_mod(fpdata *a, fpdata *b, uae_u64 *q, uae_u8 *s)
{
    fptype quot;
#if USE_HOST_ROUNDING
    quot = truncl(a->fp / b->fp);
#else
    quot = fp_round_to_zero(a->fp / b->fp);
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

static void fp_rem(fpdata *a, fpdata *b, uae_u64 *q, uae_u8 *s)
{
    fptype quot;
#if USE_HOST_ROUNDING
    quot = roundl(a->fp / b->fp);
#else
    quot = fp_round_to_nearest(a->fp / b->fp);
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

static void fp_scale(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = ldexpl(a->fp, (int)b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}

static void fp_sinh(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = sinhl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_intrz(fpdata *a, fpdata *b)
{
#if USE_HOST_ROUNDING
    a->fp = truncl(b->fp);
#else
    a->fp = fp_round_to_zero (b->fp);
#endif
	fp_round(a);
}
static void fp_sqrt(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = sqrtl(b->fp);
	fp_reset_prec(a);
}
static void fp_lognp1(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = log1pl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_etoxm1(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = expm1l(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_tanh(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = tanhl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_atan(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = atanl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_atanh(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = atanhl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_sin(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = sinl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_asin(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = asinl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_tan(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = tanl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_etox(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = expl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_twotox(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = powl(2.0, b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_tentox(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = powl(10.0, b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_logn(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = logl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_log10(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = log10l(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_log2(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = log2l(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_abs(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = fabsl(b->fp);
	fp_reset_prec(a);
}
static void fp_cosh(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = coshl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_neg(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = -b->fp;
	fp_reset_prec(a);
}
static void fp_acos(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = acosl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_cos(fpdata *a, fpdata *b)
{
	fp_normal_prec();
	a->fp = cosl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
}
static void fp_sincos(fpdata *a, fpdata *b, fpdata *c)
{
	fp_normal_prec();
	c->fp = cosl(b->fp);
	a->fp = sinl(b->fp);
	fp_reset_normal_prec();
	fp_round(a);
	fp_round(c);
}

static void fp_sub(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = a->fp - b->fp;
	fp_reset_prec(a);
}
static void fp_add(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = a->fp + b->fp;
	fp_reset_prec(a);
}
static void fp_mul(fpdata *a, fpdata *b, int prec)
{
	fp_set_prec(prec);
	a->fp = a->fp * b->fp;
	fp_reset_prec(a);
}
static void fp_sglmul(fpdata *a, fpdata *b)
{
    fptype z;
    float mant;
    int expon;
    /* FIXME: truncate mantissa of a and b to single precision */
    z = a->fp * b->fp;

    mant = (float)(frexpl(z, &expon) * 2.0);
    a->fp = ldexpl((fptype)mant, expon - 1);
}
static void fp_sgldiv(fpdata *a, fpdata *b)
{
    fptype z;
    float mant;
    int expon;
    z = a->fp / b->fp;
    
    mant = (float)(frexpl(z, &expon) * 2.0);
    a->fp = ldexpl((fptype)mant, expon - 1);
}

static void fp_normalize(fpdata *a)
{
}

static void fp_cmp(fpdata *a, fpdata *b)
{
	fptype v = 1.0;
	if (currprefs.fpu_strict) {
		fp_is_init(a);
		bool a_neg = fp_is_neg(a);
		bool a_inf = fp_is_infinity(a);
		bool a_zero = fp_is_zero(a);
		bool a_nan = fp_is_nan(a);
		fp_is_init(b);
		bool b_neg = fp_is_neg(b);
		bool b_inf = fp_is_infinity(b);
		bool b_zero = fp_is_zero(b);
		bool b_nan = fp_is_nan(b);

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
			fp_clear_status();
		}
	} else {
		v = a->fp - b->fp;
		fp_clear_status();
	}
	a->fp = v;
}

static void fp_tst(fpdata *a, fpdata *b)
{
	a->fp = b->fp;
}

/* Functions for returning exception state data */

static void fp_get_internal_overflow(fpdata *fpd)
{
	fpd->fp = 0;
}
static void fp_get_internal_underflow(fpdata *fpd)
{
	fpd->fp = 0;
}
static void fp_get_internal_round_all(fpdata *fpd)
{
	fpd->fp = 0;
}
static void fp_get_internal_round(fpdata *fpd)
{
	fpd->fp = 0;
}
static void fp_get_internal_round_exten(fpdata *fpd)
{
	fpd->fp = 0;
}
static void fp_get_internal(fpdata *fpd)
{
	fpd->fp = 0;
}
static uae_u32 fp_get_internal_grs(void)
{
	return 0;
}

/* Function for denormalizing */
static void fp_denormalize(fpdata *fpd, int esign)
{
}

static void fp_from_pack (fpdata *src, uae_u32 *wrd, int kfactor)
{
	int i, j, t;
	int exp;
	int ndigits;
	char *cp, *strp;
	char str[100];
	fptype fp;

	fp_is_init(src);
	if (fp_is_nan(src)) {
		// copy bit by bit, handle signaling nan
		fpp_from_exten(src, &wrd[0], &wrd[1], &wrd[2]);
		return;
	}
	if (fp_is_infinity(src)) {
		// extended exponent and all 0 packed fraction
		fpp_from_exten(src, &wrd[0], &wrd[1], &wrd[2]);
		wrd[1] = wrd[2] = 0;
		return;
	}

	wrd[0] = wrd[1] = wrd[2] = 0;

	fp_to_native(&fp, src);

#ifdef USE_LONG_DOUBLE
	sprintf (str, "%#.17Le", fp);
#else
	sprintf (str, "%#.17e", fp);
#endif
	
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
	i = uaestrlen(strp);
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

static void fp_to_pack (fpdata *fpd, uae_u32 *wrd, int dummy)
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
#ifdef USE_LONG_DOUBLE
	sscanf (str, "%Le", &d);
#else
	sscanf (str, "%le", &d);
#endif
	fp_from_native(d, fpd);
}


void fp_init_native(void)
{
#ifdef SOFTFLOAT_CONVERSIONS
	set_floatx80_rounding_precision(80, &fs);
	set_float_rounding_mode(float_round_to_zero, &fs);
#endif
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
	fpp_fix_infinity = NULL;

	fpp_get_status = fp_get_status;
	fpp_clear_status = fp_clear_status;
	fpp_set_mode = fp_set_mode;
	fpp_get_support_flags = fp_get_support_flags;

	fpp_to_int = fp_to_int;
	fpp_from_int = fp_from_int;

	fpp_to_pack = fp_to_pack;
	fpp_from_pack = fp_from_pack;

	fpp_to_single = fp_to_single;
	fpp_from_single = fp_from_single;
	fpp_to_double = fp_to_double;
	fpp_from_double = fp_from_double;
	fpp_to_exten = fp_to_exten;
	fpp_from_exten = fp_from_exten;
	fpp_to_exten_fmovem = fp_to_exten;
	fpp_from_exten_fmovem = fp_from_exten;

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

double softfloat_tan(double v)
{
#if SOFTFLOAT_CONVERSIONS
	struct float_status f = { 0 };
	uae_u32 w1, w2;
	fpdata fpd = { 0 };

	fpd.fp = v;
	set_floatx80_rounding_precision(80, &f);
	set_float_rounding_mode(float_round_to_zero, &f);
	fp_from_double(&fpd, &w1, &w2);
	floatx80 fv = float64_to_floatx80(((uae_u64)w1 << 32) | w2, &fs);
	fv = floatx80_tan(fv, &fs);
	float64 f64 = floatx80_to_float64(fv, &fs);
	fp_to_double(&fpd, f64 >> 32, (uae_u32)f64);
	return fpd.fp;
#else
	return tanl(v);
#endif
}
