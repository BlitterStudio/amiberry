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

    val.f = src;
    return val.u;
}

STATIC_INLINE double to_exten(uae_u32 wrd1, uae_u32 wrd2, uae_u32 wrd3)
{
    double frac;

    if ((wrd1 & 0x7fff0000) == 0 && wrd2 == 0 && wrd3 == 0)
	return 0.0;
    frac = (double) wrd2 / 2147483648.0 +
	(double) wrd3 / 9223372036854775808.0;
    if (wrd1 & 0x80000000)
	frac = -frac;
    return ldexp (frac, ((wrd1 >> 16) & 0x7fff) - 16383);
}

STATIC_INLINE double to_double (uae_u32 wrd1, uae_u32 wrd2)
{
    union {
    double d;
    uae_u32 u[2];
    } val;

    val.u[1] = wrd1;
    val.u[0] = wrd2;
    return val.d;
}

STATIC_INLINE void from_exten(double src, uae_u32 * wrd1, uae_u32 * wrd2, uae_u32 * wrd3)
{
    int expon;
    double frac;

    if (src == 0.0) {
	*wrd1 = 0;
	*wrd2 = 0;
	*wrd3 = 0;
	return;
    }
    if (src < 0) {
	*wrd1 = 0x80000000;
	src = -src;
    } else {
	*wrd1 = 0;
    }
    frac = frexp (src, &expon);
    frac += 0.5 / 18446744073709551616.0;
    if (frac >= 1.0) {
	frac /= 2.0;
	expon++;
    }
    *wrd1 |= (((expon + 16383 - 1) & 0x7fff) << 16);
    *wrd2 = (uae_u32) (frac * 4294967296.0);
    *wrd3 = (uae_u32) (frac * 18446744073709551616.0 - *wrd2 * 4294967296.0);
}

STATIC_INLINE void from_double (double src, uae_u32 * wrd1, uae_u32 * wrd2)
{
    union {
    double d;
    uae_u32 u[2];
    } val;

    val.d = src;
    *wrd1 = val.u[1];
    *wrd2 = val.u[0];
}

#define HAVE_from_double
#define HAVE_to_double
#define HAVE_from_exten
#define HAVE_to_exten
#define HAVE_from_single
#define HAVE_to_single
