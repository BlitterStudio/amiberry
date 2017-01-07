 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68881 emulation
  * Support functions for IEEE 754-compatible host CPUs.
  * These functions use a GCC extension (type punning through unions) and
  * should only be compiled with compilers that support this.
  *
  * Copyright 1999 Sam Jordan
  * Copyright 2007 Richard Drummond
  */

#define	FPCR_ROUNDING_MODE	0x00000030
#define	FPCR_ROUND_NEAR		0x00000000
#define	FPCR_ROUND_ZERO		0x00000010
#define	FPCR_ROUND_MINF		0x00000020
#define	FPCR_ROUND_PINF		0x00000030

#define	FPCR_ROUNDING_PRECISION	0x000000c0
#define	FPCR_PRECISION_SINGLE	0x00000040
#define	FPCR_PRECISION_DOUBLE	0x00000080
#define FPCR_PRECISION_EXTENDED	0x00000000

STATIC_INLINE void exten_zeronormalize(uae_u32 *pwrd1, uae_u32 *pwrd2, uae_u32 *pwrd3)
{
	uae_u32 wrd1 = *pwrd1;
	uae_u32 wrd2 = *pwrd2;
	uae_u32 wrd3 = *pwrd3;
	int exp = (wrd1 >> 16) & 0x7fff;
	// Force zero if mantissa is zero but exponent is non-zero
	// M68k FPU automatically convert them to plain zeros.
	// x86 FPU considers them invalid values
	if (exp != 0 && exp != 0x7fff && !wrd2 && !wrd3) {
		*pwrd1 = (wrd1 & 0x80000000);
	}
}

STATIC_INLINE double to_single (uae_u32 value)
{
  union {
    float f;
    uae_u32 u;
  } val;

  val.u = value;
  return val.f;
}

STATIC_INLINE uae_u32 from_single (double src)
{
  union {
    float f;
    uae_u32 u;
  } val;

  val.f = (float) src;
  return val.u;
}

STATIC_INLINE double to_double (uae_u32 wrd1, uae_u32 wrd2)
{
  union {
    double d;
    uae_u32 u[2];
  } val;

  val.u[0] = wrd2; // little endian
  val.u[1] = wrd1;
  return val.d;
}

STATIC_INLINE void from_double (double src, uae_u32 * wrd1, uae_u32 * wrd2)
{
	uae_u32 *longarray = (uae_u32 *)&src;

	*wrd1 = longarray[1]; // little endian
	*wrd2 = longarray[0];
}

static const double twoto32 = 4294967296.0;
STATIC_INLINE void to_exten(fpdata *fpd, uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
  double frac;

	exten_zeronormalize(&wrd1, &wrd2, &wrd3);
  if ((wrd1 & 0x7fff0000) == 0 && wrd2 == 0 && wrd3 == 0) {
		fpd->fp = (wrd1 & 0x80000000) ? -0.0 : +0.0;
  	return;
  }
	frac = ((double)wrd2 + ((double)wrd3 / twoto32)) / 2147483648.0;
  if (wrd1 & 0x80000000)
  	frac = -frac;
  fpd->fp = ldexp (frac, ((wrd1 >> 16) & 0x7fff) - 16383);
}

STATIC_INLINE void from_exten(fpdata *fpd, uae_u32 * wrd1, uae_u32 * wrd2, uae_u32 * wrd3)
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

#define HAVE_from_double
#define HAVE_to_double
#define HAVE_from_exten
#define HAVE_to_exten
#define HAVE_from_single
#define HAVE_to_single
