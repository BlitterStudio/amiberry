 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68881 emulation
  *
  * Copyright 1996 Herman ten Brugge
  * Modified 2005 Peter Keunecke
   */

#define __USE_ISOC9X  /* We might be able to pick up a NaN */

#include <math.h>
#include <float.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "events.h"
#include "newcpu.h"
#include "ersatz.h"
#include "md-pandora/md-fpp.h"
#include "savestate.h"

#define DEBUG_FPP 0

STATIC_INLINE int isinrom (void)
{
    return (munge24 (m68k_getpc (&regs)) & 0xFFF80000) == 0xF80000;
}

static uae_u32 xhex_pi[]    ={0x2168c235, 0xc90fdaa2, 0x4000};
uae_u32 xhex_exp_1[] ={0xa2bb4a9a, 0xadf85458, 0x4000};
static uae_u32 xhex_l2_e[]  ={0x5c17f0bc, 0xb8aa3b29, 0x3fff};
static uae_u32 xhex_ln_2[]  ={0xd1cf79ac, 0xb17217f7, 0x3ffe};
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
static uae_u32 xhex_inf[]   ={0x00000000, 0x00000000, 0x7fff};
static uae_u32 xhex_nan[]   ={0xffffffff, 0xffffffff, 0x7fff};
#if USE_LONG_DOUBLE
static long double *fp_pi     = (long double *)xhex_pi;
static long double *fp_exp_1  = (long double *)xhex_exp_1;
static long double *fp_l2_e   = (long double *)xhex_l2_e;
static long double *fp_ln_2   = (long double *)xhex_ln_2;
static long double *fp_ln_10  = (long double *)xhex_ln_10;
static long double *fp_l10_2  = (long double *)xhex_l10_2;
static long double *fp_l10_e  = (long double *)xhex_l10_e;
static long double *fp_1e16   = (long double *)xhex_1e16;
static long double *fp_1e32   = (long double *)xhex_1e32;
static long double *fp_1e64   = (long double *)xhex_1e64;
static long double *fp_1e128  = (long double *)xhex_1e128;
static long double *fp_1e256  = (long double *)xhex_1e256;
static long double *fp_1e512  = (long double *)xhex_1e512;
static long double *fp_1e1024 = (long double *)xhex_1e1024;
static long double *fp_1e2048 = (long double *)xhex_1e2048;
static long double *fp_1e4096 = (long double *)xhex_1e4096;
static long double *fp_inf    = (long double *)xhex_inf;
static long double *fp_nan    = (long double *)xhex_nan;
#else
static uae_u32 dhex_pi[]    ={0x54442D18, 0x400921FB};
static uae_u32 dhex_exp_1[] ={0x8B145769, 0x4005BF0A};
static uae_u32 dhex_l2_e[]  ={0x652B82FE, 0x3FF71547};
static uae_u32 dhex_ln_2[]  ={0xFEFA39EF, 0x3FE62E42};
static uae_u32 dhex_ln_10[] ={0xBBB55516, 0x40026BB1};
static uae_u32 dhex_l10_2[] ={0x509F79FF, 0x3FD34413};
static uae_u32 dhex_l10_e[] ={0x1526E50E, 0x3FDBCB7B};
static uae_u32 dhex_1e16[]  ={0x37E08000, 0x4341C379};
static uae_u32 dhex_1e32[]  ={0xB5056E17, 0x4693B8B5};
static uae_u32 dhex_1e64[]  ={0xE93FF9F5, 0x4D384F03};
static uae_u32 dhex_1e128[] ={0xF9301D32, 0x5A827748};
static uae_u32 dhex_1e256[] ={0x7F73BF3C, 0x75154FDD};
static uae_u32 dhex_inf[]   ={0x00000000, 0x7ff00000};
static uae_u32 dhex_nan[]   ={0xffffffff, 0x7fffffff};
static double *fp_pi     = (double *)dhex_pi;
static double *fp_exp_1  = (double *)dhex_exp_1;
static double *fp_l2_e   = (double *)dhex_l2_e;
static double *fp_ln_2   = (double *)dhex_ln_2;
static double *fp_ln_10  = (double *)dhex_ln_10;
static double *fp_l10_2  = (double *)dhex_l10_2;
static double *fp_l10_e  = (double *)dhex_l10_e;
static double *fp_1e16   = (double *)dhex_1e16;
static double *fp_1e32   = (double *)dhex_1e32;
static double *fp_1e64   = (double *)dhex_1e64;
static double *fp_1e128  = (double *)dhex_1e128;
static double *fp_1e256  = (double *)dhex_1e256;
static double *fp_1e512  = (double *)dhex_inf;
static double *fp_1e1024 = (double *)dhex_inf;
static double *fp_1e2048 = (double *)dhex_inf;
static double *fp_1e4096 = (double *)dhex_inf;
static double *fp_inf    = (double *)dhex_inf;
static double *fp_nan    = (double *)dhex_nan;
#endif
double fp_1e8 = 1.0e8;
float  fp_1e0 = 1, fp_1e1 = 10, fp_1e2 = 100, fp_1e4 = 10000;

#define FFLAG_Z	    0x4000
#define FFLAG_N	    0x0100
#define FFLAG_NAN   0x0400

#define MAKE_FPSR(regs, r)  (regs)->fp_result=(r)

static uae_u16 x87_cw_tab[] = {
  0x137f, 0x1f7f, 0x177f, 0x1b7f,  /* Extended */
  0x107f, 0x1c7f, 0x147f, 0x187f,  /* Single */
  0x127f, 0x1e7f, 0x167f, 0x1a7f,  /* Double */
  0x137f, 0x1f7f, 0x177f, 0x1b7f}; /* undefined */
  /* Nearest, toZero, Down, Up */
static __inline__ void native_set_fpucw (uae_u32 m68k_cw)
{
#if USE_X86_FPUCW
  uae_u16 x87_cw = x87_cw_tab[(m68k_cw>>4)&0xf];

#if defined(X86_MSVC_ASSEMBLY)
  __asm {
    fldcw word ptr x87_cw
  }
#elif defined(X86_ASSEMBLY)
  __asm__ ("fldcw %0" : : "m" (*&x87_cw));
#endif
#endif
}

#if defined(uae_s64) /* Close enough for government work? */
typedef uae_s64 tointtype;
#else
typedef uae_s32 tointtype;
#endif

static void fpu_op_illg (uae_u32 opcode, struct regstruct *regs, int pcoffset)
{
    if ((currprefs.cpu_model == 68060 && (currprefs.fpu_model == 0 || (regs->pcr & 2)))
	|| (currprefs.cpu_model == 68040 && currprefs.fpu_model == 0)) {
	/* 68040 unimplemented/68060 FPU disabled exception.
	 * Line F exception with different stack frame.. */
	uaecptr newpc = m68k_getpc(regs);
	uaecptr oldpc = newpc - pcoffset;
	MakeSR(regs);
        if (!regs->s) {
	    regs->usp = m68k_areg(regs, 7);
	    m68k_areg(regs, 7) = regs->isp;
	}
	regs->s = 1;
        m68k_areg(regs, 7) -= 4;
        put_long (m68k_areg(regs, 7), oldpc);
        m68k_areg(regs, 7) -= 4;
        put_long (m68k_areg(regs, 7), oldpc);
	m68k_areg(regs, 7) -= 2;
	put_word (m68k_areg(regs, 7), 0x4000 + 11 * 4);
        m68k_areg(regs, 7) -= 4;
        put_long (m68k_areg(regs, 7), newpc);
	m68k_areg(regs, 7) -= 2;
	put_word (m68k_areg(regs, 7), regs->sr);
        write_log("68040/060 FPU disabled exception PC=%x\n", newpc);
	newpc = get_long (regs->vbr + 11 * 4);
	m68k_setpc(regs, newpc);
	set_special(regs, SPCFLAG_END_COMPILE);
   	return;
    }
    op_illg (opcode, regs);
}

STATIC_INLINE int fault_if_no_fpu(uae_u32 opcode, struct regstruct *regs, int pcoffset)
{
    if ((regs->pcr & 2) || currprefs.fpu_model <= 0) {
	fpu_op_illg (opcode, regs, pcoffset);
	return 1;
    }
    return 0;
}

static int get_fpu_version(void)
{
    int v = 0;
//    if (currprefs.fpu_revision >= 0)
//	return currprefs.fpu_revision;
    switch (currprefs.fpu_model)
    {
	case 68881:
	v = 0x1f;
	break;
	case 68882:
	v = 0x20; /* ??? */
	break;
	case 68040:
	v = 0x41;
	break;
    }
    return v;
}

#define fp_round_to_minus_infinity(x) fp_floor(x)
#define fp_round_to_plus_infinity(x) fp_ceil(x)
#define fp_round_to_zero(x) ((int)(x))
#define fp_round_to_nearest(x) ((int)((x) + 0.5))

STATIC_INLINE tointtype toint(fptype src, fptype minval, fptype maxval)
{
    if (src < minval)
	src = minval;
    if (src > maxval)
	src = maxval;
#if defined(X86_MSVC_ASSEMBLY)
    {
	fptype tmp_fp;
	__asm {
	    fld  LDPTR src
	    frndint
	    fstp LDPTR tmp_fp
	}
	return (tointtype)tmp_fp;
    }
#else /* no X86_MSVC */
  switch ((regs.fpcr >> 4) & 3) {
    case 0: /* to nearest */
	    return (tointtype)floor (src + 0.5);
    case 1: /* to zero */
	return (tointtype) src;
    case 2: /* down */
	return (tointtype)floor (src);
    case 3: /* up */
	return (tointtype)ceil (src);
  }
#endif
}

uae_u32 get_fpsr (void)
{
    uae_u32 answer = regs.fpsr & 0x00ffffff;
#ifdef HAVE_ISNAN
    if (isnan (regs.fp_result))
	answer |= 0x01000000;
    else
#endif
    {
	if (regs.fp_result == 0)
	    answer |= 0x04000000;
	else if (regs.fp_result < 0)
	    answer |= 0x08000000;
#ifdef HAVE_ISINF
	if (isinf (regs.fp_result))
	    answer |= 0x02000000;
#endif
    }
    return answer;
}

STATIC_INLINE void set_fpsr (uae_u32 x)
{
    regs.fpsr = x;

    if (x & 0x01000000) {
	regs.fp_result = *fp_nan;
    }
    else if (x & 0x04000000)
	regs.fp_result = 0;
    else if (x & 0x08000000)
	regs.fp_result = -1;
    else
	regs.fp_result = 1;
}

/* single   : S  8*E 23*F */
/* double   : S 11*E 52*F */
/* extended : S 15*E 64*F */
/* E = 0 & F = 0 -> 0 */
/* E = MAX & F = 0 -> Infin */
/* E = MAX & F # 0 -> NotANumber */
/* E = biased by 127 (single) ,1023 (double) ,16383 (extended) */

STATIC_INLINE fptype to_pack (uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
    fptype d;
    char *cp;
    char str[100];

    cp = str;
    if (wrd1 & 0x80000000)
	*cp++ = '-';
    *cp++ = (wrd1 & 0xf) + '0';
    *cp++ = '.';
    *cp++ = ((wrd2 >> 28) & 0xf) + '0';
    *cp++ = ((wrd2 >> 24) & 0xf) + '0';
    *cp++ = ((wrd2 >> 20) & 0xf) + '0';
    *cp++ = ((wrd2 >> 16) & 0xf) + '0';
    *cp++ = ((wrd2 >> 12) & 0xf) + '0';
    *cp++ = ((wrd2 >> 8) & 0xf) + '0';
    *cp++ = ((wrd2 >> 4) & 0xf) + '0';
    *cp++ = ((wrd2 >> 0) & 0xf) + '0';
    *cp++ = ((wrd3 >> 28) & 0xf) + '0';
    *cp++ = ((wrd3 >> 24) & 0xf) + '0';
    *cp++ = ((wrd3 >> 20) & 0xf) + '0';
    *cp++ = ((wrd3 >> 16) & 0xf) + '0';
    *cp++ = ((wrd3 >> 12) & 0xf) + '0';
    *cp++ = ((wrd3 >> 8) & 0xf) + '0';
    *cp++ = ((wrd3 >> 4) & 0xf) + '0';
    *cp++ = ((wrd3 >> 0) & 0xf) + '0';
    *cp++ = 'E';
    if (wrd1 & 0x40000000)
	*cp++ = '-';
    *cp++ = ((wrd1 >> 24) & 0xf) + '0';
    *cp++ = ((wrd1 >> 20) & 0xf) + '0';
    *cp++ = ((wrd1 >> 16) & 0xf) + '0';
    *cp = 0;
    sscanf (str, "%le", &d);
    return d;
}

STATIC_INLINE void from_pack (fptype src, uae_u32 * wrd1, uae_u32 * wrd2, uae_u32 * wrd3)
{
    int i;
    int t;
    char *cp;
    char str[100];

    sprintf (str, "%.16e", src);
    cp = str;
    *wrd1 = *wrd2 = *wrd3 = 0;
    if (*cp == '-') {
	cp++;
	*wrd1 = 0x80000000;
    }
    if (*cp == '+')
	cp++;
    *wrd1 |= (*cp++ - '0');
    if (*cp == '.')
	cp++;
    for (i = 0; i < 8; i++) {
	*wrd2 <<= 4;
	if (*cp >= '0' && *cp <= '9')
	    *wrd2 |= *cp++ - '0';
    }
    for (i = 0; i < 8; i++) {
	*wrd3 <<= 4;
	if (*cp >= '0' && *cp <= '9')
	    *wrd3 |= *cp++ - '0';
    }
    if (*cp == 'e' || *cp == 'E') {
	cp++;
	if (*cp == '-') {
	    cp++;
	    *wrd1 |= 0x40000000;
	}
	if (*cp == '+')
	    cp++;
	t = 0;
	for (i = 0; i < 3; i++) {
	    if (*cp >= '0' && *cp <= '9')
		t = (t << 4) | (*cp++ - '0');
	}
	*wrd1 |= t << 16;
    }
}

STATIC_INLINE int get_fp_value (uae_u32 opcode, uae_u16 extra, fptype *src)
{
    uaecptr tmppc;
    uae_u16 tmp;
    int size, mode, reg;
    uae_u32 ad = 0;
    static const int sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
    static const int sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };

    if (!(extra & 0x4000)) {
	*src = regs.fp[(extra >> 10) & 7];
	return 1;
    }
    mode = (opcode >> 3) & 7;
    reg = opcode & 7;
    size = (extra >> 10) & 7;
    switch (mode) {
    case 0:
	switch (size) {
	case 6:
	    *src = (fptype) (uae_s8) m68k_dreg (&regs, reg);
	    break;
	case 4:
	    *src = (fptype) (uae_s16) m68k_dreg (&regs, reg);
	    break;
	case 0:
	    *src = (fptype) (uae_s32) m68k_dreg (&regs, reg);
	    break;
	case 1:
	    *src = to_single (m68k_dreg (&regs, reg));
	    break;
	default:
	    return 0;
	}
	return 1;
    case 1:
	return 0;
    case 2:
	ad = m68k_areg (&regs, reg);
	break;
    case 3:
	ad = m68k_areg (&regs, reg);
	m68k_areg (&regs, reg) += reg == 7 ? sz2[size] : sz1[size];
	break;
    case 4:
	m68k_areg (&regs, reg) -= reg == 7 ? sz2[size] : sz1[size];
	ad = m68k_areg (&regs, reg);
	break;
    case 5:
	ad = m68k_areg (&regs, reg) + (uae_s32) (uae_s16) next_iword (&regs);
	break;
    case 6:
	ad = get_disp_ea_020 (&regs, m68k_areg (&regs, reg), next_iword (&regs));
	break;
    case 7:
	switch (reg) {
	case 0:
	    ad = (uae_s32) (uae_s16) next_iword (&regs);
	    break;
	case 1:
	    ad = next_ilong (&regs);
	    break;
	case 2:
	    ad = m68k_getpc (&regs);
	    ad += (uae_s32) (uae_s16) next_iword (&regs);
	    break;
	case 3:
	    tmppc = m68k_getpc (&regs);
	    tmp = next_iword (&regs);
	    ad = get_disp_ea_020 (&regs, tmppc, tmp);
	    break;
	case 4:
	    ad = m68k_getpc (&regs);
	    m68k_setpc (&regs, ad + sz2[size]);
	    if (size == 6)
		ad++;
	    break;
	default:
	    return 0;
	}
    }
    switch (size) {
    case 0:
	*src = (fptype) (uae_s32) get_long (ad);
	break;
    case 1:
	*src = to_single (get_long (ad));
	break;
    case 2:{
	    uae_u32 wrd1, wrd2, wrd3;
	    wrd1 = get_long (ad);
	    ad += 4;
	    wrd2 = get_long (ad);
	    ad += 4;
	    wrd3 = get_long (ad);
	    *src = to_exten (wrd1, wrd2, wrd3);
	}
	break;
    case 3:{
	    uae_u32 wrd1, wrd2, wrd3;
	    wrd1 = get_long (ad);
	    ad += 4;
	    wrd2 = get_long (ad);
	    ad += 4;
	    wrd3 = get_long (ad);
	    *src = to_pack (wrd1, wrd2, wrd3);
	}
	break;
    case 4:
	*src = (fptype) (uae_s16) get_word (ad);
	break;
    case 5:{
	    uae_u32 wrd1, wrd2;
	    wrd1 = get_long (ad);
	    ad += 4;
	    wrd2 = get_long (ad);
	    *src = to_double (wrd1, wrd2);
	}
	break;
    case 6:
	*src = (fptype) (uae_s8) get_byte (ad);
	break;
    default:
	return 0;
    }
    return 1;
}

STATIC_INLINE int put_fp_value (struct regstruct *regs, fptype value, uae_u32 opcode, uae_u16 extra)
{
    uae_u16 tmp;
    uaecptr tmppc;
    int size, mode, reg;
    uae_u32 ad;
    static int sz1[8] = { 4, 4, 12, 12, 2, 8, 1, 0 };
    static int sz2[8] = { 4, 4, 12, 12, 2, 8, 2, 0 };

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("PUTFP: %f %04.4X %04.4X\n", value, opcode, extra);
#endif
    if (!(extra & 0x4000)) {
	regs->fp[(extra >> 10) & 7] = value;
	return 1;
    }
    reg = opcode & 7;
    mode = (opcode >> 3) & 7;
    size = (extra >> 10) & 7;
    ad = -1;
    switch (mode) {
    case 0:
	switch (size) {
	case 6:
	    m68k_dreg (regs, reg) = (uae_u32)(((toint(value, -128.0, 127.0) & 0xff)
		| (m68k_dreg (regs, reg) & ~0xff)));
	    break;
	case 4:
	    m68k_dreg (regs, reg) = (uae_u32)(((toint(value, -32768.0, 32767.0) & 0xffff)
	    | (m68k_dreg (regs, reg) & ~0xffff)));
	    break;
	case 0:
	    m68k_dreg (regs, reg) = (uae_u32)toint(value, -2147483648.0, 2147483647.0);
	    break;
	case 1:
	    m68k_dreg (regs, reg) = from_single (value);
	    break;
	default:
	    return 0;
	}
	return 1;
    case 1:
	return 0;
    case 2:
	ad = m68k_areg (regs, reg);
	break;
    case 3:
	ad = m68k_areg (regs, reg);
	m68k_areg (regs, reg) += reg == 7 ? sz2[size] : sz1[size];
	break;
    case 4:
	m68k_areg (regs, reg) -= reg == 7 ? sz2[size] : sz1[size];
	ad = m68k_areg (regs, reg);
	break;
    case 5:
	ad = m68k_areg (regs, reg) + (uae_s32) (uae_s16) next_iword (regs);
	break;
    case 6:
	ad = get_disp_ea_020 (regs, m68k_areg (regs, reg), next_iword (regs));
	break;
    case 7:
	switch (reg) {
	case 0:
	    ad = (uae_s32) (uae_s16) next_iword (regs);
	    break;
	case 1:
	    ad = next_ilong (regs);
	    break;
	case 2:
	    ad = m68k_getpc (regs);
	    ad += (uae_s32) (uae_s16) next_iword (regs);
	    break;
	case 3:
	    tmppc = m68k_getpc (regs);
	    tmp = next_iword (regs);
	    ad = get_disp_ea_020 (regs, tmppc, tmp);
	    break;
	case 4:
	    ad = m68k_getpc (regs);
	    m68k_setpc (regs, ad + sz2[size]);
	    break;
	default:
	    return 0;
	}
    }
    switch (size) {
    case 0:
	put_long (ad, (uae_u32)toint(value, -2147483648.0, 2147483647.0));
	break;
    case 1:
	put_long (ad, from_single (value));
	break;
    case 2:
	{
	    uae_u32 wrd1, wrd2, wrd3;
	    from_exten (value, &wrd1, &wrd2, &wrd3);
	    put_long (ad, wrd1);
	    ad += 4;
	    put_long (ad, wrd2);
	    ad += 4;
	    put_long (ad, wrd3);
	}
	break;
    case 3:
	{
	    uae_u32 wrd1, wrd2, wrd3;
	    from_pack (value, &wrd1, &wrd2, &wrd3);
	    put_long (ad, wrd1);
	    ad += 4;
	    put_long (ad, wrd2);
	    ad += 4;
	    put_long (ad, wrd3);
	}
	break;
    case 4:
	put_word (ad, (uae_s16) toint(value, -32768.0, 32767.0));
	break;
    case 5:{
	    uae_u32 wrd1, wrd2;
	    from_double (value, &wrd1, &wrd2);
	    put_long (ad, wrd1);
	    ad += 4;
	    put_long (ad, wrd2);
	}
	break;
    case 6:
	put_byte (ad, (uae_s8)toint(value, -128.0, 127.0));
	break;
    default:
	return 0;
    }
    return 1;
}

STATIC_INLINE int get_fp_ad (uae_u32 opcode, uae_u32 * ad)
{
    uae_u16 tmp;
    uaecptr tmppc;
    int mode;
    int reg;

    mode = (opcode >> 3) & 7;
    reg = opcode & 7;
    switch (mode) {
    case 0:
    case 1:
	return 0;
    case 2:
	*ad = m68k_areg (&regs, reg);
	break;
    case 3:
	*ad = m68k_areg (&regs, reg);
	break;
    case 4:
	*ad = m68k_areg (&regs, reg);
	break;
    case 5:
	*ad = m68k_areg (&regs, reg) + (uae_s32) (uae_s16) next_iword (&regs);
	break;
    case 6:
	*ad = get_disp_ea_020 (&regs, m68k_areg (&regs, reg), next_iword (&regs));
	break;
    case 7:
	switch (reg) {
	case 0:
	    *ad = (uae_s32) (uae_s16) next_iword (&regs);
	    break;
	case 1:
	    *ad = next_ilong (&regs);
	    break;
	case 2:
	    *ad = m68k_getpc (&regs);
	    *ad += (uae_s32) (uae_s16) next_iword (&regs);
	    break;
	case 3:
	    tmppc = m68k_getpc (&regs);
	    tmp = next_iword (&regs);
	    *ad = get_disp_ea_020 (&regs, tmppc, tmp);
	    break;
	default:
	    return 0;
	}
    }
    return 1;
}

STATIC_INLINE int fpp_cond (uae_u32 opcode, int contition)
{
    int N = (regs.fp_result < 0.0);
    int Z = (regs.fp_result == 0.0);
    int NotANumber = 0;

#ifdef HAVE_ISNAN
    NotANumber = isnan (regs.fp_result);
#endif

    if (NotANumber)
	N=Z=0;

    switch (contition) {
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
}

void fpuop_dbcc (uae_u32 opcode, struct regstruct *regs, uae_u16 extra)
{
    uaecptr pc = (uae_u32) m68k_getpc (regs);
    uae_s32 disp;
    int cc;

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("fdbcc_opp at %08lx\n", m68k_getpc (regs));
#endif
    if (fault_if_no_fpu (opcode, regs, 4))
	return;

    disp = (uae_s32) (uae_s16) next_iword (regs);
    cc = fpp_cond (opcode, extra & 0x3f);
    if (cc == -1) {
	fpu_op_illg (opcode, regs, 4);
    } else if (!cc) {
	int reg = opcode & 0x7;

	m68k_dreg (regs, reg) = ((m68k_dreg (regs, reg) & 0xffff0000)
	    | (((m68k_dreg (regs, reg) & 0xffff) - 1) & 0xffff));
	if ((m68k_dreg (regs, reg) & 0xffff) != 0xffff)
	    m68k_setpc (regs, pc + disp);
    }
}

void fpuop_scc (uae_u32 opcode, struct regstruct *regs, uae_u16 extra)
{
    uae_u32 ad;
    int cc;

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("fscc_opp at %08lx\n", m68k_getpc (regs));
#endif
    if (fault_if_no_fpu (opcode, regs, 4))
	return;
    cc = fpp_cond (opcode, extra & 0x3f);
     if (cc == -1) {
	fpu_op_illg (opcode, regs, 4);
    } else if ((opcode & 0x38) == 0) {
	m68k_dreg (regs, opcode & 7) = (m68k_dreg (regs, opcode & 7) & ~0xff) | (cc ? 0xff : 0x00);
    } else {
	if (get_fp_ad (opcode, &ad) == 0) {
	    m68k_setpc (regs, m68k_getpc (regs) - 4);
	    op_illg (opcode, regs);
	} else
	    put_byte (ad, cc ? 0xff : 0x00);
    }
}

void fpuop_trapcc (uae_u32 opcode, struct regstruct *regs, uaecptr oldpc)
{
    int cc;

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("ftrapcc_opp at %08lx\n", m68k_getpc (regs));
#endif
    if (fault_if_no_fpu (opcode, regs, m68k_getpc(regs) - oldpc))
	return;

    cc = fpp_cond (opcode, opcode & 0x3f);
    if (cc == -1) {
	fpu_op_illg (opcode, regs, m68k_getpc(regs) - oldpc);
    }
    if (cc)
	Exception (7, regs, oldpc - 2);
}

void fpuop_bcc (uae_u32 opcode, struct regstruct *regs, uaecptr pc, uae_u32 extra)
{
    int cc;

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("fbcc_opp at %08lx\n", m68k_getpc (regs));
#endif
    if (fault_if_no_fpu (opcode, regs, m68k_getpc(regs) - pc))
	return;

    cc = fpp_cond (opcode, opcode & 0x3f);
    if (cc == -1) {
	fpu_op_illg (opcode, regs, m68k_getpc(regs) - pc);
    } else if (cc) {
	if ((opcode & 0x40) == 0)
	    extra = (uae_s32) (uae_s16) extra;
	m68k_setpc (regs, pc + extra);
    }
}

void fpuop_save (uae_u32 opcode, struct regstruct *regs)
{
    uae_u32 ad;
    int incr = (opcode & 0x38) == 0x20 ? -1 : 1;
    int fpu_version = get_fpu_version();
    int i;

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("fsave_opp at %08lx\n", m68k_getpc (regs));
#endif
    if (fault_if_no_fpu (opcode, regs, 2))
	return;

    if (get_fp_ad (opcode, &ad) == 0) {
	fpu_op_illg (opcode, regs, 2);
	return;
    }

    if (currprefs.fpu_model == 68060) {
	/* 12 byte 68060 IDLE frame.  */
	if (incr < 0) {
	    ad -= 4;
	    put_long (ad, 0x00000000);
	    ad -= 4;
	    put_long (ad, 0x00000000);
	    ad -= 4;
	    put_long (ad, 0x00006000);
	} else {
	    put_long (ad, 0x00006000);
	    ad += 4;
	    put_long (ad, 0x00000000);
	    ad += 4;
	    put_long (ad, 0x00000000);
	    ad += 4;
	}
    } else if (currprefs.fpu_model == 68040) {
	/* 4 byte 68040 IDLE frame.  */
	if (incr < 0) {
	    ad -= 4;
	    put_long (ad, fpu_version << 24);
	} else {
	    put_long (ad, fpu_version << 24);
	    ad += 4;
	}
    } else { /* 68881/68882 */
	int idle_size = currprefs.fpu_model == 68882 ? 0x38 : 0x18;
	if (incr < 0) {
	    ad -= 4;
	    put_long (ad, 0x70000000);
	    for (i = 0; i < (idle_size - 1) / 4; i++) {
		ad -= 4;
		put_long (ad, 0x00000000);
	    }
	    ad -= 4;
	    put_long (ad, (fpu_version << 24) | (idle_size << 16));
	} else {
	    put_long (ad, (fpu_version << 24) | (idle_size << 16));
	    ad += 4;
	    for (i = 0; i < (idle_size - 1) / 4; i++) {
		put_long (ad, 0x00000000);
		ad += 4;
	    }
	    put_long (ad, 0x70000000);
	    ad += 4;
	}
    }
    if ((opcode & 0x38) == 0x18)
	m68k_areg (regs, opcode & 7) = ad;
    if ((opcode & 0x38) == 0x20)
	m68k_areg (regs, opcode & 7) = ad;
}

void fpuop_restore (uae_u32 opcode, struct regstruct *regs)
{
    uae_u32 ad;
    uae_u32 d;
    int incr = (opcode & 0x38) == 0x20 ? -1 : 1;

#if DEBUG_FPP
    if (!isinrom ())
	write_log ("frestore_opp at %08lx\n", m68k_getpc (regs));
#endif
    if (fault_if_no_fpu (opcode, regs, 2))
	return;

    if (get_fp_ad (opcode, &ad) == 0) {
	fpu_op_illg (opcode, regs, 2);
	return;
    }
    if (currprefs.fpu_model == 68060) {
	/* all 68060 FPU frames are 12 bytes */
	if (incr < 0) {
	    ad -= 4;
	    d = get_long (ad);
	    ad -= 8;
	} else {
	    d = get_long (ad);
	    ad += 4;
	    ad += 8;
	}

    } else if (currprefs.fpu_model == 68040) {
	/* 68040 */
	if (incr < 0) {
	    /* @@@ This may be wrong.  */
	    ad -= 4;
	    d = get_long (ad);
	    if ((d & 0xff000000) != 0) { /* Not a NULL frame? */
		if ((d & 0x00ff0000) == 0) { /* IDLE */
		} else if ((d & 0x00ff0000) == 0x00300000) { /* UNIMP */
		    ad -= 44;
		} else if ((d & 0x00ff0000) == 0x00600000) { /* BUSY */
		    ad -= 92;
		}
	    }
	} else {
	    d = get_long (ad);
	    ad += 4;
	    if ((d & 0xff000000) != 0) { /* Not a NULL frame? */
		if ((d & 0x00ff0000) == 0) { /* IDLE */
		} else if ((d & 0x00ff0000) == 0x00300000) { /* UNIMP */
		    ad += 44;
		} else if ((d & 0x00ff0000) == 0x00600000) { /* BUSY */
		    ad += 92;
		}
	    }
	}
    } else { /* 68881/68882 */
	if (incr < 0) {
	    ad -= 4;
	    d = get_long (ad);
	    if ((d & 0xff000000) != 0) {
		if ((d & 0x00ff0000) == 0x00180000)
		    ad -= 6 * 4;
		else if ((d & 0x00ff0000) == 0x00380000)
		    ad -= 14 * 4;
		else if ((d & 0x00ff0000) == 0x00b40000)
		    ad -= 45 * 4;
	    }
	} else {
	    d = get_long (ad);
	    ad += 4;
	    if ((d & 0xff000000) != 0) {
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
	m68k_areg (regs, opcode & 7) = ad;
    if ((opcode & 0x38) == 0x20)
	m68k_areg (regs, opcode & 7) = ad;
}

static void fround (int reg)
{
    regs.fp[reg] = (float)regs.fp[reg];
}

void fpuop_arithmetic (uae_u32 opcode, struct regstruct *regs, uae_u16 extra)
{
  int reg;
  fptype src;

#if DEBUG_FPP
  if (!isinrom ())
  	write_log ("FPP %04lx %04x at %08lx\n", opcode & 0xffff, extra, m68k_getpc (regs) - 4);
#endif
  if (fault_if_no_fpu (opcode, regs, 4))
  	return;

  switch ((extra >> 13) & 0x7) {
    case 3:
    	if (put_fp_value (regs, regs->fp[(extra >> 7) & 7], opcode, extra) == 0) {
	      m68k_setpc (regs, m68k_getpc (regs) - 4);
	      op_illg (opcode, regs);
	    }
	    return;
    case 4:
    case 5:
	    if ((opcode & 0x38) == 0) {
	      if (extra & 0x2000) {
		      if (extra & 0x1000)
		        m68k_dreg (regs, opcode & 7) = regs->fpcr & 0xffff;
		      if (extra & 0x0800)
		        m68k_dreg (regs, opcode & 7) = get_fpsr ();
		      if (extra & 0x0400)
		        m68k_dreg (regs, opcode & 7) = regs->fpiar;
	      } else {
		      if (extra & 0x1000) {
		        regs->fpcr = m68k_dreg (regs, opcode & 7);
		        native_set_fpucw (regs->fpcr);
		      }
		      if (extra & 0x0800)
		        set_fpsr (m68k_dreg (regs, opcode & 7));
		      if (extra & 0x0400)
		        regs->fpiar = m68k_dreg (regs, opcode & 7);
	      }
	    } else if ((opcode & 0x38) == 0x08) {
	      if (extra & 0x2000) {
		      if (extra & 0x1000)
		        m68k_areg (regs, opcode & 7) = regs->fpcr & 0xffff;
		      if (extra & 0x0800)
		        m68k_areg (regs, opcode & 7) = get_fpsr ();
		      if (extra & 0x0400)
		        m68k_areg (regs, opcode & 7) = regs->fpiar;
	      } else {
		      if (extra & 0x1000) {
		        regs->fpcr = m68k_areg (regs, opcode & 7);
		        native_set_fpucw (regs->fpcr);
		      }
		      if (extra & 0x0800)
		        set_fpsr (m68k_areg (regs, opcode & 7));
		      if (extra & 0x0400)
		        regs->fpiar = m68k_areg (regs, opcode & 7);
	      }
    	} else if ((opcode & 0x3f) == 0x3c) {
	      if ((extra & 0x2000) == 0) {
		      if (extra & 0x1000) {
		        regs->fpcr = next_ilong (regs);
		        native_set_fpucw (regs->fpcr);
		      }
		      if (extra & 0x0800)
		        set_fpsr (next_ilong (regs));
		      if (extra & 0x0400)
		      regs->fpiar = next_ilong (regs);
	      }
	    } else if (extra & 0x2000) {
	      /* FMOVEM FPP->memory */
	      uae_u32 ad;
	      int incr = 0;

  	    if (get_fp_ad (opcode, &ad) == 0) {
	      	m68k_setpc (regs, m68k_getpc (regs) - 4);
		      op_illg (opcode, regs);
		      return;
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
		      put_long (ad, regs->fpcr & 0xffff);
		      ad += 4;
	      }
	      if (extra & 0x0800) {
		      put_long (ad, get_fpsr());
		      ad += 4;
	      }
	      if (extra & 0x0400) {
		      put_long (ad, regs->fpiar);
		      ad += 4;
	      }
	      ad -= incr;
	      if ((opcode & 0x38) == 0x18)
		      m68k_areg (regs, opcode & 7) = ad;
	      if ((opcode & 0x38) == 0x20)
		      m68k_areg (regs, opcode & 7) = ad;
	    } else {
	      /* FMOVEM memory->FPP */
	      uae_u32 ad;
	      int incr = 0;

	      if (get_fp_ad (opcode, &ad) == 0) {
		      m68k_setpc (regs, m68k_getpc (regs) - 4);
		      op_illg (opcode, regs);
		      return;
	      }
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
		      regs->fpcr = get_long (ad);
		      native_set_fpucw(regs->fpcr);
		      ad += 4;
	      }
	      if (extra & 0x0800) {
		      set_fpsr(get_long (ad));
		      ad += 4;
	      }
	      if (extra & 0x0400) {
		      regs->fpiar = get_long (ad);
		      ad += 4;
	      }
	      if ((opcode & 0x38) == 0x18)
		      m68k_areg (regs, opcode & 7) = ad;
	      if ((opcode & 0x38) == 0x20)
		      m68k_areg (regs, opcode & 7) = ad - incr;
	    }
	    return;
    case 6:
    case 7:
      {
	      uae_u32 ad, list = 0;
	      int incr = 0;
	      if (extra & 0x2000) {
	        /* FMOVEM FPP->memory */
	        if (get_fp_ad (opcode, &ad) == 0) {
		        m68k_setpc (regs, m68k_getpc (regs) - 4);
		        op_illg (opcode, regs);
		        return;
	        }
	        switch ((extra >> 11) & 3) {
	          case 0:	/* static pred */
		          list = extra & 0xff;
		          incr = -1;
		          break;
	          case 1:	/* dynamic pred */
		          list = m68k_dreg (regs, (extra >> 4) & 3) & 0xff;
		          incr = -1;
		          break;
	          case 2:	/* static postinc */
		          list = extra & 0xff;
		          incr = 1;
		          break;
	          case 3:	/* dynamic postinc */
		          list = m68k_dreg (regs, (extra >> 4) & 3) & 0xff;
		          incr = 1;
		          break;
	        }
	        if (incr < 0) {
		        for (reg = 7; reg >= 0; reg--) {
		          uae_u32 wrd1, wrd2, wrd3;
		          if (list & 0x80) {
			          from_exten (regs->fp[reg], &wrd1, &wrd2, &wrd3);
		            ad -= 4;
		            put_long (ad, wrd3);
		            ad -= 4;
		            put_long (ad, wrd2);
		            ad -= 4;
		            put_long (ad, wrd1);
		          }
		          list <<= 1;
		        }
      		} else {
		        for (reg = 0; reg <= 7; reg++) {
		          uae_u32 wrd1, wrd2, wrd3;
		          if (list & 0x80) {
			          from_exten (regs->fp[reg], &wrd1, &wrd2, &wrd3);
        		    put_long (ad, wrd1);
        		    ad += 4;
        		    put_long (ad, wrd2);
        		    ad += 4;
        		    put_long (ad, wrd3);
        		    ad += 4;
        		  }
		          list <<= 1;
		        }
	        }
	        if ((opcode & 0x38) == 0x18)
		        m68k_areg (regs, opcode & 7) = ad;
	        if ((opcode & 0x38) == 0x20)
		        m68k_areg (regs, opcode & 7) = ad;
	      } else {
	        /* FMOVEM memory->FPP */
	        if (get_fp_ad (opcode, &ad) == 0) {
        		m68k_setpc (regs, m68k_getpc (regs) - 4);
        		op_illg (opcode, regs);
        		return;
    	    }
	        switch ((extra >> 11) & 3) {
	          case 0:	/* static pred */
          		list = extra & 0xff;
          		incr = -1;
          		break;
      	    case 1:	/* dynamic pred */
          		list = m68k_dreg (regs, (extra >> 4) & 3) & 0xff;
          		incr = -1;
          		break;
      	    case 2:	/* static postinc */
          		list = extra & 0xff;
          		incr = 1;
          		break;
      	    case 3:	/* dynamic postinc */
          		list = m68k_dreg (regs, (extra >> 4) & 3) & 0xff;
          		incr = 1;
          		break;
    	    }
	        if (incr < 0) {
		        for (reg = 7; reg >= 0; reg--) {
		          uae_u32 wrd1, wrd2, wrd3;
		          if (list & 0x80) {
        		    ad -= 4;
        		    wrd3 = get_long (ad);
        		    ad -= 4;
        		    wrd2 = get_long (ad);
        		    ad -= 4;
        		    wrd1 = get_long (ad);
          			regs->fp[reg] = to_exten(wrd1, wrd2, wrd3);
		          }
		          list <<= 1;
		        }
		      } else {
        		for (reg = 0; reg <= 7; reg++) {
		          uae_u32 wrd1, wrd2, wrd3;
		          if (list & 0x80) {
        		    wrd1 = get_long (ad);
        		    ad += 4;
        		    wrd2 = get_long (ad);
        		    ad += 4;
        		    wrd3 = get_long (ad);
        		    ad += 4;
          			regs->fp[reg] = to_exten(wrd1, wrd2, wrd3);
		          }
		          list <<= 1;
		        }
	        }
	        if ((opcode & 0x38) == 0x18)
		        m68k_areg (regs, opcode & 7) = ad;
	        if ((opcode & 0x38) == 0x20)
		        m68k_areg (regs, opcode & 7) = ad;
	      }
      }
      return;
    case 0:
    case 2: /* Extremely common */
    	reg = (extra >> 7) & 7;
	    if ((extra & 0xfc00) == 0x5c00) {
	      switch (extra & 0x7f) {
	        case 0x00:
		        regs->fp[reg] = *fp_pi;
		        break;
	        case 0x0b:
		        regs->fp[reg] = *fp_l10_2;
		        break;
	        case 0x0c:
		        regs->fp[reg] = *fp_exp_1;
		        break;
	        case 0x0d:
		        regs->fp[reg] = *fp_l2_e;
		        break;
	        case 0x0e:
		        regs->fp[reg] = *fp_l10_e;
		        break;
	        case 0x0f:
		        regs->fp[reg] = 0.0;
		        break;
     	    case 0x30:
        		regs->fp[reg] = *fp_ln_2;
        		break;
     	    case 0x31:
        		regs->fp[reg] = *fp_ln_10;
        		break;
     	    case 0x32:
        		regs->fp[reg] = (fptype)fp_1e0;
        		break;
     	    case 0x33:
        		regs->fp[reg] = (fptype)fp_1e1;
        		break;
     	    case 0x34:
        		regs->fp[reg] = (fptype)fp_1e2;
        		break;
     	    case 0x35:
        		regs->fp[reg] = (fptype)fp_1e4;
        		break;
     	    case 0x36:
        		regs->fp[reg] = (fptype)fp_1e8;
        		break;
     	    case 0x37:
        		regs->fp[reg] = *fp_1e16;
        		break;
     	    case 0x38:
        		regs->fp[reg] = *fp_1e32;
        		break;
     	    case 0x39:
        		regs->fp[reg] = *fp_1e64;
        		break;
    	    case 0x3a:
        		regs->fp[reg] = *fp_1e128;
        		break;
     	    case 0x3b:
        		regs->fp[reg] = *fp_1e256;
        		break;
     	    case 0x3c:
        		regs->fp[reg] = *fp_1e512;
        		break;
     	    case 0x3d:
        		regs->fp[reg] = *fp_1e1024;
        		break;
     	    case 0x3e:
        		regs->fp[reg] = *fp_1e2048;
        		break;
     	    case 0x3f:
        		regs->fp[reg] = *fp_1e4096;
        		break;
    	    default:
		        m68k_setpc (regs, m68k_getpc (regs) - 4);
		        op_illg (opcode, regs);
		        return;
	      }
	      MAKE_FPSR (regs, regs->fp[reg]);
	      return;
	    }
	    if (get_fp_value (opcode, extra, &src) == 0) {
	      m68k_setpc (regs, m68k_getpc (regs) - 4);
	      op_illg (opcode, regs);
	      return;
	    }
    	switch (extra & 0x7f) {
    	case 0x00: /* FMOVE */
    	case 0x40: /* Explicit rounding. This is just a quick fix. */
    	case 0x44: /* Same for all other cases that have three choices */
  	    regs->fp[reg] = src;        /* Brian King was here. */
	      /*<ea> to register needs FPSR updated. See Motorola 68K Manual. */
	      if ((extra & 0x44) == 0x40)
		      fround (reg);
  	    break;
	    case 0x01: /* FINT */
	      /* need to take the current rounding mode into account */
#if defined(X86_MSVC_ASSEMBLY)
        {
	        fptype tmp_fp;

	        __asm {
        		fld  LDPTR src
        		frndint
        		fstp LDPTR tmp_fp
  	      }
	        regs->fp[reg] = tmp_fp;
	      }
#else /* no X86_MSVC */
	      switch ((regs->fpcr >> 4) & 3) {
		      case 0: /* to nearest */
		        regs->fp[reg] = floor (src + 0.5);
		        break;
		      case 1: /* to zero */
		        if (src >= 0.0)
			        regs->fp[reg] = floor (src);
		        else
			        regs->fp[reg] = ceil (src);
		        break;
		      case 2: /* down */
		        regs->fp[reg] = floor (src);
		        break;
		      case 3: /* up */
		        regs->fp[reg] = ceil (src);
		        break;
		      default: /* never reached */
		        regs->fp[reg] = src;
	      }
#endif /* X86_MSVC */
	      break;
	    case 0x02: /* FSINH */
	      regs->fp[reg] = sinh (src);
	      break;
	    case 0x03: /* FINTRZ */
	      regs->fp[reg] = fp_round_to_zero(src);
	      break;
    	case 0x04: /* FSQRT */
    	case 0x41:
    	case 0x45:
  	    regs->fp[reg] = sqrt (src);
	      if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
	    case 0x06: /* FLOGNP1 */
	      regs->fp[reg] = log (src + 1.0);
	      break;
	    case 0x08: /* FETOXM1 */
	      regs->fp[reg] = exp (src) - 1.0;
	      break;
	    case 0x09: /* FTANH */
	      regs->fp[reg] = tanh (src);
	      break;
	    case 0x0a: /* FATAN */
	      regs->fp[reg] = atan (src);
	      break;
	    case 0x0c: /* FASIN */
	      regs->fp[reg] = asin (src);
	      break;
	    case 0x0d: /* FATANH */
#if 1	/* The BeBox doesn't have atanh, and it isn't in the HPUX libm either */
	      regs->fp[reg] = 0.5 * log ((1 + src) / (1 - src));
#else
	      regs->fp[reg] = atanh (src);
#endif
	      break;
	    case 0x0e: /* FSIN */
	      regs->fp[reg] = sin (src);
	      break;
	    case 0x0f: /* FTAN */
	      regs->fp[reg] = tan (src);
	      break;
	    case 0x10: /* FETOX */
	      regs->fp[reg] = exp (src);
	      break;
    	case 0x11: /* FTWOTOX */
	      regs->fp[reg] = pow (2.0, src);
	      break;
	    case 0x12: /* FTENTOX */
	      regs->fp[reg] = pow (10.0, src);
	      break;
	    case 0x14: /* FLOGN */
	      regs->fp[reg] = log (src);
	      break;
	    case 0x15: /* FLOG10 */
	      regs->fp[reg] = log10 (src);
	      break;
	    case 0x16: /* FLOG2 */
	      regs->fp[reg] = *fp_l2_e * log (src);
	      break;
    	case 0x18: /* FABS */
    	case 0x58:
    	case 0x5c:
  	    regs->fp[reg] = src < 0 ? -src : src;
	      if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
	    case 0x19: /* FCOSH */
	      regs->fp[reg] = cosh (src);
	      break;
    	case 0x1a: /* FNEG */
    	case 0x5a:
    	case 0x5e:
	      regs->fp[reg] = -src;
  	    if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
	    case 0x1c: /* FACOS */
	      regs->fp[reg] = acos (src);
	      break;
	    case 0x1d: /* FCOS */
	      regs->fp[reg] = cos (src);
	      break;
	    case 0x1e: /* FGETEXP */
	      {
		      if (src == 0) {
		        regs->fp[reg] = 0;
		      } else {
	          int expon;
	          frexp (src, &expon);
	          regs->fp[reg] = (double) (expon - 1);
	        }
	      }
	      break;
	    case 0x1f: /* FGETMAN */
	      {
		      if (src == 0) {
		        regs->fp[reg] = 0;
		      } else {
	          int expon;
	          regs->fp[reg] = frexp (src, &expon) * 2.0;
	        }
	      }
	      break;
    	case 0x20: /* FDIV */
    	case 0x60:
    	case 0x64:
  	    regs->fp[reg] /= src;
	      if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
	    case 0x21: /* FMOD */
	      {
		      fptype quot = fp_round_to_zero(regs->fp[reg] / src);
		      regs->fp[reg] = regs->fp[reg] - quot * src;
	      }
	      break;
    	case 0x22: /* FADD */
    	case 0x62:
    	case 0x66:
  	    regs->fp[reg] += src;
	      if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
    	case 0x23: /* FMUL */
    	case 0x63:
    	case 0x67:
	      regs->fp[reg] *= src;
  	    if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
	    case 0x24: /* FSGLDIV */
	      regs->fp[reg] /= src;
	      break;
	    case 0x25: /* FREM */
	      {
		      fptype quot = fp_round_to_nearest(regs->fp[reg] / src);
		      regs->fp[reg] = regs->fp[reg] - quot * src;
	      }
	      break;
	    case 0x26: /* FSCALE */
	      if (src != 0) {
#ifdef ldexp
	        regs->fp[reg] = ldexp (regs->fp[reg], (int) src);
#else
	        regs->fp[reg] *= exp (*fp_ln_2 * (int) src);
#endif
	      }
	      break;
	    case 0x27: /* FSGLMUL */
	      regs->fp[reg] *= src;
	      break;
    	case 0x28: /* FSUB */
    	case 0x68:
    	case 0x6c:
  	    regs->fp[reg] -= src;
	      if ((extra & 0x44) == 0x40)
		      fround (reg);
	      break;
    	case 0x30: /* FSINCOS */
    	case 0x31:
    	case 0x32:
    	case 0x33:
    	case 0x34:
    	case 0x35:
    	case 0x36:
    	case 0x37:
  	    regs->fp[extra & 7] = cos (src);
  	    regs->fp[reg] = sin (src);
  	    break;
	    case 0x38: /* FCMP */
	      {
	        fptype tmp = regs->fp[reg] - src;
	        regs->fpsr = 0;
	        MAKE_FPSR (regs, tmp);
	      }
	      return;
	    case 0x3a: /* FTST */
	      regs->fpsr = 0;
	      MAKE_FPSR (regs, src);
	      return;
	    default:
	      m68k_setpc (regs, m68k_getpc (regs) - 4);
	      op_illg (opcode, regs);
	      return;
	  }
	  MAKE_FPSR (regs, regs->fp[reg]);
	  return;
  }
  m68k_setpc (regs, m68k_getpc (regs) - 4);
  op_illg (opcode, regs);
}

uae_u8 *restore_fpu (uae_u8 *src)
{
    int i;
    uae_u32 flags;

    changed_prefs.fpu_model = currprefs.fpu_model = restore_u32();
    flags = restore_u32 ();
    for (i = 0; i < 8; i++) {
	uae_u32 w1 = restore_u32 ();
	uae_u32 w2 = restore_u32 ();
	uae_u32 w3 = restore_u16 ();
	regs.fp[i] = to_exten (w1, w2, w3);
    }
    regs.fpcr = restore_u32 ();
    regs.fpsr = restore_u32 ();
    regs.fpiar = restore_u32 ();
    if (flags & 0x80000000) {
	restore_u32();
	restore_u32();
    }
    write_log("FPU=%d\n", currprefs.fpu_model);
    return src;
}

uae_u8 *save_fpu (int *len, uae_u8 *dstptr)
{
    uae_u8 *dstbak,*dst;
    int i;

    *len = 0;
    if (currprefs.fpu_model == 0)
	return 0;
    if (dstptr)
	dstbak = dst = dstptr;
    else
	dstbak = dst = (uae_u8 *)malloc(4+4+8*10+4+4+4+4+4);
    save_u32 (currprefs.fpu_model);
    save_u32 (0x80000000);
    for (i = 0; i < 8; i++) {
	uae_u32 w1, w2, w3;
	from_exten (regs.fp[i], &w1, &w2, &w3);
	save_u32 (w1);
	save_u32 (w2);
	save_u16 (w3);
    }
    save_u32 (regs.fpcr);
    save_u32 (regs.fpsr);
    save_u32 (regs.fpiar);
    save_u32 (-1);
    save_u32 (0);
    *len = dst - dstbak;
    return dstbak;
}
