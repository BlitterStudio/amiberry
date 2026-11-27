// Checks the combined SET_CZNV_{ADD,SUB,LOGICAL}{B,W,L} macros from
// src/machdep/m68k.h against the per-flag expressions gencpu emitted before
// the macros existed. The macro calls below use the argument forms gencpu
// writes into cpuemu_*.cpp. Build variants (see test_cpu_flags_macros.sh):
// x86-64 with the lahf/seto asm, the same layout with the portable fallback
// forced, x86-64 without SAHF_SETO_PROFITABLE, the AArch64 NZCV layout and
// the machine-independent struct layout.
#include "uae/types.h"
#include "machdep/m68k.h"

#ifdef FLAGS_TEST_FALLBACK_BLOCK
// Drop the asm macros and re-read the header's portable fallback block,
// extracted verbatim by the driver script.
#undef SET_CZNV_ADDB
#undef SET_CZNV_ADDW
#undef SET_CZNV_ADDL
#undef SET_CZNV_SUBB
#undef SET_CZNV_SUBW
#undef SET_CZNV_SUBL
#undef SET_CZNV_LOGICALB
#undef SET_CZNV_LOGICALW
#undef SET_CZNV_LOGICALL
#undef SET_CZNV4
#include FLAGS_TEST_FALLBACK_BLOCK
#endif

#include <cstdint>
#include <cstdio>
#include <cstring>

struct flag_struct regflags;

static unsigned long failures;
static unsigned long checks;

struct Ref {
	int c, z, n, v;
};

static void scramble(uint32_t seed)
{
	std::memset(&regflags, 0, sizeof(regflags));
	SET_CFLG(seed & 1);
	SET_ZFLG((seed >> 1) & 1);
	SET_NFLG((seed >> 2) & 1);
	SET_VFLG((seed >> 3) & 1);
	SET_XFLG((seed >> 4) & 1);
}

static void verify(const char *what, uint32_t s, uint32_t d, const Ref &r, int x)
{
	checks++;
	int c = GET_CFLG() & 1, z = GET_ZFLG() & 1, n = GET_NFLG() & 1, v = GET_VFLG() & 1;
	bool ok = c == r.c && z == r.z && n == r.n && v == r.v && (int)(GET_XFLG() & 1) == x;
	const int want[16] = {
		1, 0, !r.c && !r.z, r.c || r.z, !r.c, r.c, !r.z, r.z,
		!r.v, r.v, !r.n, r.n, r.n == r.v, r.n != r.v,
		!r.z && r.n == r.v, r.z || r.n != r.v
	};
	for (int cc = 0; cc < 16; cc++)
		ok = ok && (cctrue(cc) != 0) == (want[cc] != 0);
	if (!ok && failures++ < 20)
		std::printf("FAIL %s s=%08x d=%08x got CZNVX=%d%d%d%d%d want %d%d%d%d%d\n", what, s, d,
			c, z, n, v, (int)(GET_XFLG() & 1), r.c, r.z, r.n, r.v, x);
}

// Each macro below expands to one gencpu-shaped block: the operand and
// flag temporaries named as gencpu names them, then the macro call.
#define ADD_CASE(MACRO, ST, UT) do { \
	ST src = (ST)s_in; ST dst = (ST)d_in; \
	uae_u32 newv = ((UT)(dst)) + ((UT)(src)); \
	int flgs = ((ST)(src)) < 0; int flgo = ((ST)(dst)) < 0; int flgn = ((ST)(newv)) < 0; \
	Ref r = { ((UT)(~dst)) < ((UT)(src)), ((ST)(newv)) == 0, flgn, (flgs ^ flgn) & (flgo ^ flgn) }; \
	scramble(seed); int x = GET_XFLG() & 1; \
	MACRO(((UT)(~dst)) < ((UT)(src)), ((ST)(newv)) == 0, flgn, (flgs ^ flgn) & (flgo ^ flgn), ((ST)(src)), ((ST)(dst))); \
	verify(#MACRO, (UT)src, (UT)dst, r, x); \
} while (0)

#define SUB_CASE(MACRO, ST, UT) do { \
	ST src = (ST)s_in; ST dst = (ST)d_in; \
	uae_u32 newv = ((UT)(dst)) - ((UT)(src)); \
	int flgs = ((ST)(src)) < 0; int flgo = ((ST)(dst)) < 0; int flgn = ((ST)(newv)) < 0; \
	Ref r = { ((UT)(src)) > ((UT)(dst)), ((ST)(newv)) == 0, flgn, (flgs ^ flgo) & (flgn ^ flgo) }; \
	scramble(seed); int x = GET_XFLG() & 1; \
	MACRO(((UT)(src)) > ((UT)(dst)), ((ST)(newv)) == 0, flgn, (flgs ^ flgo) & (flgn ^ flgo), ((ST)(src)), ((ST)(dst))); \
	verify(#MACRO " sub", (UT)src, (UT)dst, r, x); \
	scramble(~seed); x = GET_XFLG() & 1; \
	MACRO(((UT)(src)) > ((UT)(dst)), ((ST)(newv)) == 0, flgn, (flgs != flgo) && (flgn != flgo), ((ST)(src)), ((ST)(dst))); \
	verify(#MACRO " cmp", (UT)src, (UT)dst, r, x); \
} while (0)

#define LOGICAL_CASE(MACRO, ST, UT) do { \
	ST src = (ST)s_in; \
	Ref r = { 0, ((ST)(src)) == 0, ((ST)(src)) < 0, 0 }; \
	scramble(seed); int x = GET_XFLG() & 1; \
	MACRO(((ST)(src))); \
	verify(#MACRO, (UT)src, 0, r, x); \
} while (0)

static void check_b(uint32_t s_in, uint32_t d_in, uint32_t seed)
{
	ADD_CASE(SET_CZNV_ADDB, uae_s8, uae_u8);
	SUB_CASE(SET_CZNV_SUBB, uae_s8, uae_u8);
	LOGICAL_CASE(SET_CZNV_LOGICALB, uae_s8, uae_u8);
}

static void check_w(uint32_t s_in, uint32_t d_in, uint32_t seed)
{
	ADD_CASE(SET_CZNV_ADDW, uae_s16, uae_u16);
	SUB_CASE(SET_CZNV_SUBW, uae_s16, uae_u16);
	LOGICAL_CASE(SET_CZNV_LOGICALW, uae_s16, uae_u16);
}

static void check_l(uint32_t s_in, uint32_t d_in, uint32_t seed)
{
	ADD_CASE(SET_CZNV_ADDL, uae_s32, uae_u32);
	SUB_CASE(SET_CZNV_SUBL, uae_s32, uae_u32);
	LOGICAL_CASE(SET_CZNV_LOGICALL, uae_s32, uae_u32);
}

static uint32_t rng_state = 0x2545f491u;
static uint32_t rnd()
{
	rng_state ^= rng_state << 13;
	rng_state ^= rng_state >> 17;
	rng_state ^= rng_state << 5;
	return rng_state;
}

int main()
{
	uint32_t seed = 0;
	for (uint32_t s = 0; s < 0x100; s++)
		for (uint32_t d = 0; d < 0x100; d++)
			check_b(s, d, seed++);

	static const uint32_t edges[] = {
		0x00000000, 0x00000001, 0x00000002, 0x0000007f, 0x00000080, 0x000000ff,
		0x00007fff, 0x00008000, 0x00008001, 0x0000ffff, 0x00010000, 0x3fffffff,
		0x40000000, 0x7ffffffe, 0x7fffffff, 0x80000000, 0x80000001, 0xbfffffff,
		0xc0000000, 0xfffffffe, 0xffffffff, 0x12345678, 0xedcba987, 0x55555555,
		0xaaaaaaaa,
	};
	const size_t nedges = sizeof(edges) / sizeof(edges[0]);
	for (size_t i = 0; i < nedges; i++)
		for (size_t j = 0; j < nedges; j++) {
			check_w(edges[i], edges[j], seed++);
			check_w(edges[i] >> 16, edges[j], seed++);
			check_l(edges[i], edges[j], seed++);
		}
	for (uint32_t s = 0; s < 0x10000; s += 0x101)
		for (uint32_t d = 0; d < 0x10000; d += 0xff)
			check_w(s, d, seed++);
	for (int i = 0; i < 2000000; i++) {
		uint32_t s = rnd(), d = rnd();
		check_w(s, d, seed);
		check_l(s, d, seed++);
		// Bias towards equal and adjacent operands for the Z and C edges.
		check_l(s, s + (d & 3) - 1, seed++);
	}

	std::printf("%s: %lu checks, %lu failures\n", FLAGS_TEST_VARIANT, checks, failures);
	return failures ? 1 : 0;
}
