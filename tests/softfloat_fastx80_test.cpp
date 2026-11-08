// softfloat_fastx80.h against softfloat: for random and edge-case operands,
// every result the fast FADD/FSUB/FMUL give must be the one softfloat gives
// at extended precision, rounding to nearest even, with the same flags
// (inexact only); the FCMP condition codes those of floatx80_cmp(); and the
// single, double and integer conversions those of softfloat, without flags.
// Built and run by tests/softfloat_fastx80_test.py.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include "softfloat/softfloat.h"
#include "softfloat/softfloat_fastx80.h"

#ifdef __SIZEOF_INT128__

static uint64_t state = 0x9e3779b97f4a7c15ull;
static uint64_t rnd()
{
	state ^= state << 13;
	state ^= state >> 7;
	state ^= state << 17;
	return state;
}

// a normalized significand: random, or with a pattern rounding cares about
static uint64_t sig()
{
	const uint64_t top = 0x8000000000000000ull;
	switch (rnd() % 8) {
	case 0: return top;
	case 1: return ~0ull;
	case 2: return top | (rnd() << (rnd() % 64));       // trailing zeros: ties
	case 3: return top | (rnd() >> (rnd() % 64));       // leading zeros below the integer bit
	case 4: return top | 1;
	default: return top | rnd();
	}
}

static floatx80 make(int sign, int32_t e, uint64_t m)
{
	floatx80 v;
	v.high = (uint16_t)((sign ? 0x8000 : 0) | (e & 0x7fff));
	v.low = m;
	return v;
}

// an exponent anywhere, or near the range ends or 1.0
static int32_t expo()
{
	static const int32_t ends[] = {1, 0x7ffe, 0x3fff};
	if (rnd() % 3)
		return (int32_t)(ends[rnd() % 3] + (int32_t)(rnd() % 9) - 4);
	return (int32_t)(rnd() % 0x8000);
}

static floatx80 operand()
{
	const uint64_t k = rnd() % 16;
	if (k == 0)
		return make(rnd() & 1, 0, 0);                    // zero
	if (k == 1)
		return make(rnd() & 1, 0, rnd() >> 1);           // denormal
	if (k == 2)
		return make(rnd() & 1, 0x7fff, rnd() % 2 ? 0 : rnd());
	if (k == 3)
		return make(rnd() & 1, 1 + rnd() % 0x7ffe, rnd() >> 1); // unnormal
	return make(rnd() & 1, expo(), sig());
}

// b relative to a: close (cancellation), at a chosen distance, or anywhere
static floatx80 partner(floatx80 a)
{
	const int32_t ae = a.high & 0x7fff;
	switch (rnd() % 6) {
	case 0: return make(rnd() & 1, ae, a.low + (int64_t)(rnd() % 9) - 4);
	case 1: return make(rnd() & 1, ae - 1, (a.low << 1) | 0x8000000000000000ull | (rnd() % 4));
	case 2: return make(rnd() & 1, ae - (int32_t)(rnd() % 140), sig());
	case 3: return make(rnd() & 1, ae + (int32_t)(rnd() % 140), sig());
	default: return operand();
	}
}

static long failures, handled, total;

static void check(const char *op, floatx80 a, floatx80 b)
{
	const int prec = 80;
	float_status fs;
	memset(&fs, 0, sizeof fs);
	set_float_detect_tininess(float_tininess_before_rounding, &fs);
	set_float_rounding_mode(float_round_nearest_even, &fs);
	set_floatx80_rounding_precision(prec, &fs);
	floatx80 want;
	if (op[0] == 'a')
		want = floatx80_add(a, b, &fs);
	else if (op[0] == 's')
		want = floatx80_sub(a, b, &fs);
	else
		want = floatx80_mul(a, b, &fs);
	floatx80 got;
	bool inexact = false, ok;
	if (op[0] == 'm')
		ok = fx80_mul(a, b, &got, &inexact);
	else
		ok = fx80_addsub(a, b, op[0] == 's', &got, &inexact);
	total++;
	if (!ok)
		return;
	handled++;
	const uint8_t flags = inexact ? float_flag_inexact : 0;
	if (got.high != want.high || got.low != want.low || fs.float_exception_flags != flags) {
		if (failures++ < 20)
			printf("%s prec %d: %04x.%016llx %04x.%016llx -> fast %04x.%016llx inex %d, softfloat %04x.%016llx flags %02x\n",
				op, prec, a.high, (unsigned long long)a.low, b.high, (unsigned long long)b.low,
				got.high, (unsigned long long)got.low, inexact,
				want.high, (unsigned long long)want.low, fs.float_exception_flags);
	}
}

static void report(const char *what, uint64_t in, floatx80 got, floatx80 want, unsigned flags)
{
	if (failures++ < 20)
		printf("%s %016llx: fast %04x.%016llx, softfloat %04x.%016llx flags %02x\n", what,
			(unsigned long long)in, got.high, (unsigned long long)got.low, want.high,
			(unsigned long long)want.low, flags);
}

static void check_cmp(floatx80 a, floatx80 b)
{
	float_status fs;
	memset(&fs, 0, sizeof fs);
	const floatx80 want = floatx80_cmp(a, b, &fs);
	uint32_t cc;
	total++;
	if (!fx80_cmp_cc(a, b, &cc))
		return;
	handled++;
	const uint32_t want_cc = ((want.high & 0x8000) ? 8 : 0)
		| ((want.high & 0x7fff) == 0 && want.low == 0 ? 4 : 0);
	if (cc != want_cc || fs.float_exception_flags) {
		if (failures++ < 20)
			printf("cmp %04x.%016llx %04x.%016llx: fast cc %x, softfloat cc %x flags %02x\n",
				a.high, (unsigned long long)a.low, b.high, (unsigned long long)b.low,
				cc, want_cc, fs.float_exception_flags);
	}
}

// a single or double bit pattern: any, or with an exponent at a range end
static uint64_t pattern(int bits)
{
	uint64_t v = rnd();
	if (bits == 32)
		v &= 0xffffffff;
	if (rnd() % 3 == 0) {
		const int ebits = bits == 32 ? 8 : 11, fbits = bits - 1 - ebits;
		const uint64_t emax = (1ull << ebits) - 1, ends[] = {0, 1, 2, emax - 1, emax};
		v = (v & ~(emax << fbits)) | (ends[rnd() % 5] << fbits);
	}
	return v;
}

static void check_conversions()
{
	float_status fs;
	floatx80 got, want;
	const uint32_t f = (uint32_t)pattern(32);
	memset(&fs, 0, sizeof fs);
	want = float32_to_floatx80_allowunnormal(f, &fs);
	total++;
	if (fx80_from_f32(f, &got)) {
		handled++;
		if (got.high != want.high || got.low != want.low || fs.float_exception_flags)
			report("f32", f, got, want, fs.float_exception_flags);
	}
	const uint64_t d = pattern(64);
	memset(&fs, 0, sizeof fs);
	want = float64_to_floatx80_allowunnormal(d, &fs);
	total++;
	if (fx80_from_f64(d, &got)) {
		handled++;
		if (got.high != want.high || got.low != want.low || fs.float_exception_flags)
			report("f64", d, got, want, fs.float_exception_flags);
	}
	static const int32_t edges[] = {0, 1, -1, 0x7fffffff, (int32_t)0x80000000, 127, -128, 32767, -32768};
	const int32_t i = rnd() % 4 == 0 ? edges[rnd() % 9] : (int32_t)(rnd() >> (rnd() % 64));
	memset(&fs, 0, sizeof fs);
	want = int32_to_floatx80(i);
	total++;
	if (fx80_from_i32(i, &got)) {
		handled++;
		if (got.high != want.high || got.low != want.low)
			report("i32", (uint32_t)i, got, want, 0);
	}
}

#endif

int main(int argc, char **argv)
{
#ifndef __SIZEOF_INT128__
	(void)argc; (void)argv;
	return 77;                                     // the fast paths need a 128-bit integer type
#else
	const long n = argc > 1 ? atol(argv[1]) : 3000000;
	static const char *const ops[] = {"add", "sub", "mul"};
	for (long i = 0; i < n; i++) {
		const floatx80 a = operand();
		const floatx80 b = rnd() % 2 ? partner(a) : operand();
		for (const char *op : ops)
			check(op, a, b);
		check_cmp(a, b);
		check_conversions();
	}
	printf("%ld checks, %ld handled by the fast path, %ld mismatches\n", total, handled, failures);
	// the fast path must cover the common case (normal operands), not just be correct
	return failures || handled < total / 8 ? 1 : 0;
#endif
}
