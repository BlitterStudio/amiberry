/* Inline extended-precision FADD/FSUB/FMUL for the common case: both
 * operands finite and normalized (explicit integer bit, exponent 1..0x7FFE),
 * rounding to nearest even at extended precision, a normal result. The
 * result and the inexact flag are those of softfloat's floatx80_add/
 * floatx80_sub/floatx80_mul/floatx80_div/floatx80_sqrt (SOFTFLOAT_68K). Also FCMP's condition codes of
 * two normal values, normal single, double and integer sources in extended
 * format, and FMOVE of a normal value to an integer, single or double. tests/softfloat_fastx80_test.py compares them all with
 * softfloat. False: not that case, nothing done (softfloat handles it:
 * zeros, denormals, unnormals, infinities, NaNs, an exact zero difference,
 * operands 64 or more binades apart, overflow and underflow). Unlike
 * softfloat they do not save the unrounded result (saveFloatx80Internal()),
 * which only an enabled arithmetic exception reads. */
#ifndef SOFTFLOAT_FASTX80_H
#define SOFTFLOAT_FASTX80_H

#include <stdint.h>
#include "softfloat.h"

#ifdef __SIZEOF_INT128__

/* Always inlined: called out of line, the result returns through memory as
 * a 16-bit and a 64-bit store, and the caller's 128-bit load of it waits
 * for both (no store forwarding), which costs more than the arithmetic. */
#define FX80_INLINE static inline __attribute__((always_inline))

FX80_INLINE bool fx80_normal(floatx80 a)
{
	const uint32_t e = a.high & 0x7fff;
	return e - 1 < 0x7ffe && (int64_t)a.low < 0;
}

/* m: the normalized significand, lost: the bits below it (a sticky bit
 * in bit 0 at most), e: the biased exponent. Rounded to nearest even. */
FX80_INLINE bool fx80_pack(bool sign, int32_t e, uint64_t m, uint64_t lost, floatx80 *r, bool *inexact)
{
	const uint64_t half = 0x8000000000000000ull;
	if (lost > half || (lost == half && (m & 1))) {
		if (++m == 0) {
			m = half;
			e++;
		}
	}
	if ((uint32_t)e - 1 >= 0x7ffe)
		return false;                                  // underflow or overflow
	r->high = (uint16_t)((sign ? 0x8000 : 0) | e);
	r->low = m;
	*inexact = lost != 0;
	return true;
}

/* a + b, or a - b with sub. */
FX80_INLINE bool fx80_addsub(floatx80 a, floatx80 b, bool sub, floatx80 *r, bool *inexact)
{
	if (!fx80_normal(a) || !fx80_normal(b))
		return false;
	bool as = a.high >> 15, bs = (b.high >> 15) ^ sub;
	int32_t ae = a.high & 0x7fff, be = b.high & 0x7fff;
	uint64_t am = a.low, bm = b.low;
	if (ae < be || (ae == be && am < bm)) {           // |a| >= |b| from here
		const bool ts = as; as = bs; bs = ts;
		const int32_t te = ae; ae = be; be = te;
		const uint64_t tm = am; am = bm; bm = tm;
	}
	const int d = ae - be;
	if (d >= 64)
		return false;
	const uint64_t bs_m = bm >> d;                     // b aligned to a, and the bits shifted out
	const uint64_t lost = d ? bm << (64 - d) : 0;
	if (as == bs) {
		uint64_t m = am + bs_m;
		if (m >= am)
			return fx80_pack(as, ae, m, lost, r, inexact);
		// carry: one more binade, the sum's lowest bit joins the lost ones
		return fx80_pack(as, ae + 1, (m >> 1) | 0x8000000000000000ull,
			(m << 63) | (lost >> 1) | (lost & 1), r, inexact);
	}
	// exact: |a| >= |b| and b's lost bits fit the low word
	const unsigned __int128 z = ((unsigned __int128)am << 64) - (((unsigned __int128)bs_m << 64) | lost);
	if (z == 0)
		return false;                                  // exact zero: its sign depends on the mode
	const uint64_t hi = (uint64_t)(z >> 64), lo = (uint64_t)z;
	if ((int64_t)hi < 0)
		return fx80_pack(as, ae, hi, lo, r, inexact);
	// normalize (64-bit shifts: a variable 128-bit one is a libgcc call)
	if (hi) {
		const int lz = __builtin_clzll(hi);    // 1..63: hi's top bit is clear
		return fx80_pack(as, ae - lz, (hi << lz) | (lo >> (64 - lz)), lo << lz, r, inexact);
	}
	const int lz = __builtin_clzll(lo);
	return fx80_pack(as, ae - 64 - lz, lo << lz, 0, r, inexact);
}

FX80_INLINE bool fx80_mul(floatx80 a, floatx80 b, floatx80 *r, bool *inexact)
{
	if (!fx80_normal(a) || !fx80_normal(b))
		return false;
	const unsigned __int128 p = (unsigned __int128)a.low * b.low;   // top bit at 127 or 126
	uint64_t hi = (uint64_t)(p >> 64), lo = (uint64_t)p;
	int32_t e = (a.high & 0x7fff) + (b.high & 0x7fff) - 0x3ffe;
	if ((int64_t)hi >= 0) {
		hi = (hi << 1) | (lo >> 63);
		lo <<= 1;
		e--;
	}
	return fx80_pack((a.high ^ b.high) >> 15, e, hi, lo, r, inexact);
}

/* (u1:u0) / v for u1 < v and a normalized v (top bit set), with the
 * remainder: two 64/32-bit steps on the hardware divider (Hacker's
 * Delight divlu), faster than a 128-bit division from libgcc. */
FX80_INLINE uint64_t fx80_div128(uint64_t u1, uint64_t u0, uint64_t v, uint64_t *rem)
{
	const uint64_t b = 1ull << 32;
	const uint64_t vn1 = v >> 32, vn0 = v & 0xffffffff;
	const uint64_t un1 = u0 >> 32, un0 = u0 & 0xffffffff;
	uint64_t q1 = u1 / vn1, rhat = u1 - q1 * vn1;
	while (q1 >= b || q1 * vn0 > b * rhat + un1) {
		q1--;
		rhat += vn1;
		if (rhat >= b)
			break;
	}
	const uint64_t un21 = u1 * b + un1 - q1 * v;
	uint64_t q0 = un21 / vn1;
	rhat = un21 - q0 * vn1;
	while (q0 >= b || q0 * vn0 > b * rhat + un0) {
		q0--;
		rhat += vn1;
		if (rhat >= b)
			break;
	}
	*rem = un21 * b + un0 - q0 * v;
	return q1 * b + q0;
}

/* a / b, as floatx80_div() at extended precision rounding to nearest:
 * the quotient's 64 bits, rounded from the exact remainder. */
FX80_INLINE bool fx80_div(floatx80 a, floatx80 b, floatx80 *r, bool *inexact)
{
	if (!fx80_normal(a) || !fx80_normal(b))
		return false;
	int32_t e = (a.high & 0x7fff) - (b.high & 0x7fff) + 0x3fff;
	const uint64_t am = a.low, bm = b.low;
	uint64_t q, rem;
	if (am >= bm) {
		q = fx80_div128(am >> 1, am << 63, bm, &rem);  // quotient in [1, 2)
	} else {
		q = fx80_div128(am, 0, bm, &rem);              // in (1/2, 1)
		e--;
	}
	// round to nearest: up when the remainder is above half the divisor (a
	// quotient of two 64-bit significands is never exactly halfway)
	if (rem > bm - rem) {
		if (++q == 0) {
			q = 0x8000000000000000ull;
			e++;
		}
	}
	if ((uint32_t)e - 1 >= 0x7ffe)
		return false;                                  // underflow or overflow
	r->high = (uint16_t)(((a.high ^ b.high) & 0x8000) | e);
	r->low = q;
	*inexact = rem != 0;
	return true;
}

/* sqrt(a) of a normal positive a, as floatx80_sqrt() at extended precision
 * rounding to nearest: s = floor(sqrt(N)) of the 128-bit N holding the
 * significand (the exponent made even), from a host double estimate (about
 * 2^12 off) and one correction, then fixed with the exact remainder
 * R = N - s^2 in [0, 2s]. Rounds up when R > s (never exactly halfway). */
FX80_INLINE bool fx80_sqrt(floatx80 a, floatx80 *r, bool *inexact)
{
	if (!fx80_normal(a) || (a.high & 0x8000))
		return false;
	const int32_t ae = a.high & 0x7fff;
	const bool odd = !((ae - 0x3fff) & 1) ? false : true;
	// N = am * 2^63 (even exponent) or am * 2^64 (odd): sqrt(N) in [2^63, 2^64)
	const unsigned __int128 n = (unsigned __int128)a.low << (odd ? 64 : 63);
	const double dn = (double)a.low * (odd ? 0x1p64 : 0x1p63);
	const double ds = __builtin_sqrt(dn);
	uint64_t s = ds >= 0x1p64 ? ~0ull : (uint64_t)ds;
	// one correction: s += (N - s^2) / (2s), |N - s^2| below about 2^78
	{
		const __int128 rem = (__int128)(n - (unsigned __int128)s * s);
		const double drem = (double)(int64_t)(rem >> 64) * 0x1p64 + (double)(uint64_t)rem;
		const double delta = drem / (2.0 * ds);
		s += (int64_t)delta;
	}
	// exact: floor(sqrt(N)), R = N - s^2 (s^2 can exceed N by a little here)
	unsigned __int128 sq = (unsigned __int128)s * s;
	while (sq > n) {
		s--;
		sq = (unsigned __int128)s * s;
	}
	unsigned __int128 rem = n - sq;
	while (rem > ((unsigned __int128)s << 1)) {    // (s + 1)^2 <= N
		rem -= ((unsigned __int128)s << 1) + 1;
		s++;
	}
	int32_t e = ((ae - 0x3fff) >> 1) + 0x3fff;
	if (rem > s) {
		if (++s == 0) {
			s = 0x8000000000000000ull;
			e++;
		}
	}
	r->high = (uint16_t)e;
	r->low = s;
	*inexact = rem != 0;
	return true;
}

/* floatx80_cmp() of two normal values as the FPSR condition codes it
 * gives: Z (4) when equal, N (8) when a < b, else 0. False: not normal. */
FX80_INLINE bool fx80_cmp_cc(floatx80 a, floatx80 b, uint32_t *cc)
{
	if (!fx80_normal(a) || !fx80_normal(b))
		return false;
	if (a.high == b.high && a.low == b.low) {
		*cc = 4;
		return true;
	}
	const bool an = a.high >> 15;
	const uint32_t ae = a.high & 0x7fff, be = b.high & 0x7fff;
	const bool mag_less = ae != be ? ae < be : a.low < b.low;
	const bool less = an != (bool)(b.high >> 15) ? an : an != mag_less;
	*cc = less ? 8 : 0;
	return true;
}

/* Normal single, double and nonzero integer values in extended format, as
 * float32_to_floatx80_allowunnormal(), float64_to_floatx80_allowunnormal()
 * and int32_to_floatx80() give them (exact, no flags). False: zero,
 * denormal, infinity or NaN. */
FX80_INLINE bool fx80_from_f32(uint32_t v, floatx80 *r)
{
	const uint32_t e = (v >> 23) & 0xff;
	if (e - 1 >= 0xfe)
		return false;
	r->high = (uint16_t)(((v >> 16) & 0x8000) | (e + 0x3f80));
	r->low = ((uint64_t)v << 40) | 0x8000000000000000ull;
	return true;
}

FX80_INLINE bool fx80_from_f64(uint64_t v, floatx80 *r)
{
	const uint32_t e = (uint32_t)(v >> 52) & 0x7ff;
	if (e - 1 >= 0x7fe)
		return false;
	r->high = (uint16_t)(((v >> 48) & 0x8000) | (e + 0x3c00));
	r->low = (v << 11) | 0x8000000000000000ull;
	return true;
}

FX80_INLINE bool fx80_from_i32(int32_t i, floatx80 *r)
{
	if (i == 0)
		return false;
	const uint32_t m = i < 0 ? 0u - (uint32_t)i : (uint32_t)i;
	const int lz = __builtin_clz(m);
	r->high = (uint16_t)((i < 0 ? 0x8000 : 0) | (0x3fff + 31 - lz));
	r->low = (uint64_t)m << (32 + lz);
	return true;
}

/* FMOVE to an integer (bits 32, 16 or 8), single or double, rounding to
 * nearest even, as floatx80_to_int32/16/8(), floatx80_to_float32() and
 * floatx80_to_float64() give it, with their inexact flag. False: not a
 * normal value, or one out of the format's range (or 2^31 and up: -2^31 is
 * left to softfloat as well), or an underflow. */
FX80_INLINE bool fx80_to_int(floatx80 a, int bits, int32_t *r, bool *inexact)
{
	if (!fx80_normal(a))
		return false;
	const int e = (a.high & 0x7fff) - 0x3fff;
	if (e >= bits - 1)
		return false;
	const uint64_t half = 0x8000000000000000ull;
	uint64_t q, rem;
	if (e >= 0) {
		q = a.low >> (63 - e);
		rem = a.low << (e + 1);
	} else {
		q = 0;
		rem = e == -1 ? a.low : 1;                     // 1: below one half, a sticky bit
	}
	if (rem > half || (rem == half && (q & 1)))
		q++;
	const bool neg = a.high >> 15;
	if (q > (neg ? 1ull << (bits - 1) : (1ull << (bits - 1)) - 1))
		return false;
	*r = neg ? (int32_t)(0u - (uint32_t)q) : (int32_t)q;
	*inexact = rem != 0;
	return true;
}

FX80_INLINE bool fx80_to_f32(floatx80 a, uint32_t *r, bool *inexact)
{
	if (!fx80_normal(a))
		return false;
	uint32_t e = (a.high & 0x7fff) - 0x3f80;           // single biased exponent
	if (e - 1 >= 0xfe)
		return false;
	uint64_t q = a.low >> 40;
	const uint64_t rem = a.low << 24, half = 0x8000000000000000ull;
	if (rem > half || (rem == half && (q & 1))) {
		if (++q >> 24) {
			q >>= 1;
			if (++e > 0xfe)
				return false;                          // overflow
		}
	}
	*r = ((uint32_t)(a.high & 0x8000) << 16) | (e << 23) | ((uint32_t)q & 0x7fffff);
	*inexact = rem != 0;
	return true;
}

FX80_INLINE bool fx80_to_f64(floatx80 a, uint64_t *r, bool *inexact)
{
	if (!fx80_normal(a))
		return false;
	uint64_t e = (uint64_t)(a.high & 0x7fff) - 0x3c00;  // double biased exponent
	if (e - 1 >= 0x7fe)
		return false;
	uint64_t q = a.low >> 11;
	const uint64_t rem = a.low << 53, half = 0x8000000000000000ull;
	if (rem > half || (rem == half && (q & 1))) {
		if (++q >> 53) {
			q >>= 1;
			if (++e > 0x7fe)
				return false;
		}
	}
	*r = ((uint64_t)(a.high & 0x8000) << 48) | (e << 52) | (q & 0xfffffffffffffull);
	*inexact = rem != 0;
	return true;
}

#undef FX80_INLINE

#endif /* __SIZEOF_INT128__ */

#endif
