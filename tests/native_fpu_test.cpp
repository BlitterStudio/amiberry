// Included after the actual backend and its exported function-pointer storage.
#include <cassert>
#include <initializer_list>
#include <chrono>

struct uae_prefs currprefs;
struct regstruct regs;

void fpsr_set_exception(uae_u32 exception)
{
	regs.fpsr |= exception;
}

static void check_value(fpdata *value, uae_u32 exponent, uae_u32 high, uae_u32 low)
{
	uae_u32 actual_exponent, actual_high, actual_low;
	fpp_from_exten_fmovem(value, &actual_exponent, &actual_high, &actual_low);
	if (actual_exponent != exponent || actual_high != high || actual_low != low) {
		fprintf(stderr, "%08x.%08x.%08x != %08x.%08x.%08x\n",
			actual_exponent, actual_high, actual_low, exponent, high, low);
		assert(false);
	}
}

static void check_status(uae_u32 expected)
{
	uae_u32 status = 0;
	fpp_get_status(&status);
	if (status != expected) {
		fprintf(stderr, "status %08x != %08x\n", status, expected);
		assert(false);
	}
}

static void mode(uae_u32 value)
{
	regs.fpcr = value;
	regs.fpsr = 0;
	regs.fp_exp_pend = 0;
	fpp_set_mode(value);
	fpp_clear_status();
}

static void check_explicit_fpsr_conditions()
{
	for (const bool *table : {condition_table_6888x, condition_table_040_060}) {
		condition_table = table;
		for (bool jit : {false, true}) {
			currprefs.cachesize = jit ? 1024 : 0;
			currprefs.compfpu = jit;
			for (uae_u32 control = 0; control < 16; ++control) {
				for (int condition = 0; condition < 32; ++condition) {
					const int expected = table[control * 32 + condition];
					assert(((fpp_get_cond_mask(condition) >> control) & 1) == expected);
					for (uae_u32 enable : {0U, uae_u32(FPSR_BSUN)}) {
						mode(enable);
						trapped_vector = 0;
						fpp_set_fpsr(control << 24);
						const bool trap = enable && (condition & 16) && (control & 1);
						assert(fpp_cond(condition) == (trap ? -2 : expected));
						assert(trapped_vector == (trap ? 48 : 0));
						assert((fpp_get_fpsr() >> 24) == control);
						if ((condition & 16) && (control & 1))
							assert((regs.fpsr & (FPSR_BSUN | FPSR_AE_IOP)) ==
								(FPSR_BSUN | FPSR_AE_IOP));
					}
				}
			}
			mode(0);
			fpp_set_fpsr(FPSR_CC_NAN | FPSR_CC_Z);
			assert(fpp_cond(1) == 1);
			fpdata result;
			fpp_from_int(&result, 1);
			fpsr_set_result_always(&result);
			assert(!fpu_fpsr_cc_written && fpp_cond(1) == 0);
		}
	}
	condition_table = condition_table_6888x;
}

static void benchmark_exponentials()
{
	if (!getenv("AMIBERRY_FPU_BENCHMARK"))
		return;
	mode(0);
	currprefs.cachesize = 0;
	fp_init_native();
	mode(0);
	fpdata source, result;
	fpp_to_exten(&source, 0x3ffe0000, 0x80000000, 0);
	const FPP_AB old_pow2 = [](fpdata *a, fpdata *b) {
		fp_normal_prec();
		a->fp = FP_FN(pow)(2.0, b->fp);
		fp_reset_normal_prec();
		fp_round(a);
	};
	const FPP_AB old_pow10 = [](fpdata *a, fpdata *b) {
		fp_normal_prec();
		a->fp = FP_FN(pow)(10.0, b->fp);
		fp_reset_normal_prec();
		fp_round(a);
	};
	const FPP_AB jit_exp2 = [](fpdata *a, fpdata *b) {
		regs.fp[1] = *b;
		fpuop_arithmetic_jit(0xf2000411, 0);
		*a = regs.fp[0];
	};
	const FPP_AB jit_exp10 = [](fpdata *a, fpdata *b) {
		regs.fp[1] = *b;
		fpuop_arithmetic_jit(0xf2000412, 0);
		*a = regs.fp[0];
	};
	const FPP_AB operations[] = {old_pow2, fpp_twotox, old_pow10, fpp_tentox, jit_exp2, jit_exp10};
	const char *names[] = {"pow(2,x)", "FTWOTOX", "pow(10,x)", "FTENTOX",
		"compiled FTWOTOX helper", "compiled FTENTOX helper"};
	currprefs.cachesize = 1024;
	currprefs.compfpu = true;
	regs.instruction_pc = 0x100;
	regs.pc = 0x104;
	for (int i = 0; i < 6; ++i) {
		FPP_AB volatile operation = operations[i];
		const auto start = std::chrono::steady_clock::now();
		for (int n = 0; n < 1000000; ++n)
			operation(&result, &source);
		const auto end = std::chrono::steady_clock::now();
		fprintf(stderr, "%s: %.2f ns/op\n", names[i],
			std::chrono::duration<double, std::nano>(end - start).count() / 1000000);
	}
}

#ifdef NATIVE_EXTENDED
static void check_host_status_clear()
{
#if defined(__GNUC__) && defined(__SSE__) && FE_ALL_EXCEPT == 0x3d
	fenv_t saved;
	assert(fegetenv(&saved) == 0);
	struct host_status {
		uae_u16 control, status;
		uae_u32 mxcsr;
	};
	const auto read = [] {
		host_status result;
		__asm__ volatile ("fnstcw %0" : "=m" (result.control));
		__asm__ volatile ("fnstsw %0" : "=a" (result.status));
		__asm__ volatile ("stmxcsr %0" : "=m" (result.mxcsr));
		return result;
	};
	const auto load = [](const host_status &state) {
		uae_u8 environment[28];
		__asm__ volatile ("fnstenv %0" : "=m" (environment));
		memcpy(environment, &state.control, sizeof(state.control));
		memcpy(environment + 4, &state.status, sizeof(state.status));
		__asm__ volatile ("fldenv %0" : : "m" (environment) : "memory");
		__asm__ volatile ("ldmxcsr %0" : : "m" (state.mxcsr) : "memory");
	};
	const host_status original = read();
	for (int rounding = 0; rounding < 4; ++rounding) {
		for (int precision = 0; precision < 4; ++precision) {
			for (int flags = 0; flags < 128; ++flags) {
				for (int sse_flags = 0; sse_flags < 64; ++sse_flags) {
					for (int flush = 0; flush < 2; ++flush) {
						host_status input = original;
						input.control = (original.control & ~0x0f00) |
							0x3f | (rounding << 10) | (precision << 8);
						input.status = (original.status & ~0x80ff) | flags;
						input.mxcsr = (original.mxcsr & ~0xe03f) |
							0x1f80 | (rounding << 13) | (flush << 15) | sse_flags;
						load(input);
						assert(feclearexcept(FE_ALL_EXCEPT) == 0);
						const host_status expected = read();
						load(input);
						fs.float_exception_flags = 0xff;
						regs.fpsr = 0x0f1234f8;
						fpp_clear_status();
						const host_status actual = read();
						assert(actual.control == expected.control);
						assert(actual.status == expected.status);
						assert(actual.mxcsr == expected.mxcsr);
						assert(fs.float_exception_flags == 0);
						assert(regs.fpsr == 0x0f1234f8);
					}
				}
			}
		}
	}
	assert(fesetenv(&saved) == 0);
#endif
}

static void check_status_lifecycle()
{
	for (bool jit : {false, true}) {
		currprefs.cachesize = jit ? 1024 : 0;
		currprefs.compfpu = jit;
		for (uae_u32 control : {0U, 0x40U, 0x80U}) {
			for (uae_u32 rounding : {0U, 0x10U, 0x20U, 0x30U}) {
				mode(control | rounding);
				fpdata a, b;
				fpp_from_int(&a, 1);
				fpp_from_int(&b, 3);
				fpp_div(&a, &b, PREC_NORMAL);
				fpsr_make_status();
				assert((regs.fpsr & (FPSR_INEX2 | FPSR_AE_INEX)) ==
					(FPSR_INEX2 | FPSR_AE_INEX));
				fpsr_clear_status();
				check_status(0);
				assert(regs.fpsr & FPSR_AE_INEX);
				fpp_from_int(&a, 1);
				fpp_from_int(&b, 0);
				fpp_div(&a, &b, PREC_NORMAL);
				fpsr_make_status();
				assert((regs.fpsr & (FPSR_DZ | FPSR_AE_DZ | FPSR_AE_INEX)) ==
					(FPSR_DZ | FPSR_AE_DZ | FPSR_AE_INEX));
				fpsr_clear_status();
				fpp_from_int(&a, 1);
				fpp_from_int(&b, 2);
				fpp_add(&a, &b, PREC_NORMAL);
				fpsr_make_status();
				assert((regs.fpsr & 0x0000ffff) == (FPSR_AE_DZ | FPSR_AE_INEX));
			}
		}
	}
	currprefs.cachesize = 0;
	currprefs.compfpu = false;
	mode(0);
}

static void check_jit_helper()
{
	currprefs.cachesize = 1024;
	currprefs.compfpu = true;
	currprefs.cpu_model = currprefs.fpu_model = 68040;
	fp_init_native();
	mode(0x50);
	regs.instruction_pc = 0x100;
	regs.pc = 0x104;
	m68k_dreg(regs, 0) = 0x7fffffff;
	fpuop_arithmetic_jit(0xf2004000, 0);
	check_value(&regs.fp[0], 0x401d0000, 0xffffff00, 0);
	assert((fpp_get_fpsr() & 0x0000ffff) == (FPSR_INEX2 | FPSR_AE_INEX));
	assert(regs.fpiar == 0x100 && regs.instruction_pc == 0x100);

	mode(0);
	fpp_to_exten_fmovem(&regs.fp[0], 0x7fff0000, 0, 0);
	fpp_from_int(&regs.fp[1], 0);
	fpuop_arithmetic_jit(0xf2000438, 0);
	assert(fpp_get_fpsr() == 0);
	check_value(&regs.fp[0], 0x7fff0000, 0, 0);
	check_emitted_predicate(&regs.fp_result, 0);

	for (const auto &raw : {
		std::initializer_list<uae_u32>{0x7fff0000, 0x20000000, 7},
		std::initializer_list<uae_u32>{0x40000000, 0x12345678, 0x9abcdef0},
		std::initializer_list<uae_u32>{0x7fff0000, 0, 0}
	}) {
		int i = 0;
		for (uae_u32 word : raw)
			jit_put_long(0x200 + 4 * i++, word);
		m68k_areg(regs, 0) = 0x200;
		m68k_areg(regs, 1) = 0x300;
		fpuop_arithmetic_jit(0xf210d080, 0x200);
		fpuop_arithmetic_jit(0xf211f080, 0x300);
		i = 0;
		for (uae_u32 word : raw)
			assert(read_long(0x300 + 4 * i++) == word);
		assert(fpp_get_fpsr() == 0);
	}

	jit_put_byte(0x200, 0xfe);
	m68k_areg(regs, 7) = 0x200;
	fpuop_arithmetic_jit(0xf21f5800, 0x200);
	assert(m68k_areg(regs, 7) == 0x202);
	check_value(&regs.fp[0], 0xc0000000, 0x80000000, 0);
	fpuop_arithmetic_jit(0xf2275800, 0x200);
	assert(m68k_areg(regs, 7) == 0x200);

	mode(FPSR_OPERR);
	fpp_to_exten(&regs.fp[0], 0x7fff0000, 0, 0);
	regs.instruction_pc = 0x100;
	regs.pc = 0x10a;
	fpuop_arithmetic_jit(0xf2296000, 0x300);
	assert(regs.fp_exception && trapped_vector == 52);
	assert(regs.instruction_pc == 0x100 && regs.fpiar == 0x100);
	assert(m68k_getpc() == 0x10a);

	mode(FPSR_DZ);
	regs.pc = 0x104;
	fpp_from_int(&regs.fp[0], 1);
	fpp_from_int(&regs.fp[1], 0);
	fpuop_arithmetic_jit(0xf2000420, 0);
	assert(regs.fp_exception && trapped_vector == 50);
	assert(regs.fpiar == 0x100);
	mode(0);
	currprefs.cachesize = 0;
	currprefs.compfpu = false;
}
#endif

int main()
{
#ifndef NATIVE_EXTENDED
	fprintf(stderr, "Native extended-register regressions require an x87 host.\n");
	return 77;
#else
	static_assert(sizeof(fptype) >= 10, "The native register must hold all 80 bits");
	currprefs.fpu_model = 68882;
	fp_init_native();
	mode(0);
	assert(fpp_get_support_flags() == (FPU_FEATURE_EXCEPTIONS | FPU_FEATURE_DENORMALS));
	fpdata a, b;

	const uae_u32 raw[][3] = {
		{0, 0, 0}, {0x80000000, 0, 0},
		{0x7fff0000, 0, 0}, {0xffff0000, 0, 0},
		{0x7fff0000, 0xc0000000, 0}, {0x7fff0000, 0xa0000000, 1},
		{0, 0, 1}, {0x3fff0000, 0x40000000, 0},
		{0x7ffe0000, 0xffffffff, 0xffffffff},
		{0x3fff0000, 0x80000000, 8}
	};
	for (const auto &words : raw) {
		fpp_to_exten_fmovem(&a, words[0], words[1], words[2]);
		check_value(&a, words[0], words[1], words[2]);
		check_status(0);
	}

	fpp_to_exten(&a, 0x7fff0000, 0xc0000000, 0x1234);
	assert(fpp_is_nan(&a) && !fpp_is_snan(&a) && !fpp_is_infinity(&a));
	fpp_move(&b, &a, PREC_NORMAL);
	check_value(&b, 0x7fff0000, 0xc0000000, 0x1234);
	check_status(0);
	fpp_to_exten(&a, 0x7fff0000, 0xa0000000, 1);
	assert(fpp_is_snan(&a));
	fpp_move(&b, &a, PREC_NORMAL);
	check_value(&b, 0x7fff0000, 0xe0000000, 1);
	check_status(FPSR_SNAN);

	mode(0x20);
	fpp_from_int(&a, 1);
	fpp_from_int(&b, 3);
	fpp_div(&a, &b, PREC_NORMAL);
	check_value(&a, 0x3ffd0000, 0xaaaaaaaa, 0xaaaaaaaa);
	check_status(FPSR_INEX2);

	mode(0x40);
	fpp_from_int(&b, 0x7fffffff);
	fpp_move(&a, &b, PREC_NORMAL);
	check_value(&a, 0x401e0000, 0x80000000, 0);
	check_status(FPSR_INEX2);

	mode(0);
	fpp_to_exten(&b, 0xbfff0000, 0x80000000, 0);
	fpp_sqrt(&a, &b, PREC_NORMAL);
	check_value(&a, 0x7fff0000, 0xffffffff, 0xffffffff);
	check_status(FPSR_OPERR);

	mode(0);
	fpp_from_int(&a, 1);
	fpp_from_int(&b, 0);
	fpp_div(&a, &b, PREC_NORMAL);
	assert(fpp_is_infinity(&a));
	check_status(FPSR_DZ);

	mode(0);
	fpp_to_exten(&a, 0x7ffe0000, 0xffffffff, 0xffffffff);
	fpp_from_int(&b, 2);
	fpp_mul(&a, &b, PREC_NORMAL);
	assert(fpp_is_infinity(&a));
	check_status(FPSR_OVFL);

	mode(0);
	fpp_to_exten(&a, 0x00010000, 0x80000000, 0);
	fpp_to_exten(&b, 0x3f9b0000, 0x80000000, 0);
	fpp_mul(&a, &b, PREC_NORMAL);
	assert(fpp_is_zero(&a));
	check_status(FPSR_UNFL | FPSR_INEX2);

	mode(0);
	fpp_to_exten(&a, 0x3fff0000, 0x80000080, 0);
	fpp_move(&b, &a, PREC_EXTENDED);
	fpp_sglmul(&a, &b);
	check_value(&a, 0x3fff0000, 0x80000000, 0);
	check_status(0);

	mode(0);
	fpp_to_exten(&a, 0x3fff0000, 0xc0000000, 0);
	assert(fpp_to_int(&a, 2) == 2);
	check_status(FPSR_INEX2);
	mode(0);
	fpp_to_exten(&a, 0x7fff0000, 0, 0);
	fpp_to_int(&a, 2);
	check_status(FPSR_OPERR);

	mode(0);
	fpp_to_exten(&a, 0, 0, 0);
	fpp_getexp(&b, &a);
	check_value(&b, 0, 0, 0);
	check_status(0);

	mode(0);
	fpp_to_exten(&a, 0x3ffe0000, 0x80000000, 0);
	fpp_sin(&b, &a);
	const floatx80 sine = native_raw(&b);
	assert(sine.high == 0x3ffd && sine.low >= LIT64(0xf57743a2582f7f42) &&
		sine.low <= LIT64(0xf57743a2582f7f46));
	check_status(FPSR_INEX2);

	mode(0);
	fpp_to_exten(&a, 0x7fff0000, 0, 0);
	fpp_sin(&b, &a);
	check_value(&b, 0x7fff0000, 0xffffffff, 0xffffffff);
	check_status(FPSR_OPERR);

	mode(0);
	uae_u32 packed[3] = {0x40010001, 0, 0};
	fpp_to_pack(&a, packed, 0);
	check_value(&a, 0x3ffb0000, 0xcccccccc, 0xcccccccd);
	check_status(FPSR_INEX1);

	mode(0);
	fpp_to_exten(&a, 0x3fff0000, 0x9d70a3d7, 0x0a3d70a4);
	fpp_from_pack(&a, packed, 3);
	assert(packed[0] == 1 && packed[1] == 0x23000000 && packed[2] == 0);
	check_status(FPSR_INEX2);
	check_explicit_fpsr_conditions();
	for (bool jit : {false, true}) {
		currprefs.cachesize = jit ? 1024 : 0;
		currprefs.compfpu = jit;
		mode(0);
		fpp_set_fpsr(0xffffffff);
		assert(fpp_get_fpsr() == 0x0ffffff8);
		for (uae_u32 sign : {0U, uae_u32(FPSR_CC_N)}) {
			fpp_set_fpsr(sign | FPSR_CC_I);
			if (jit) {
				// Exercise the actual x87 operand, not the backend's raw-bit classifier.
				assert(!std::isnan(regs.fp_result.fp));
				assert(std::isinf(regs.fp_result.fp));
				assert((regs.fp_result.fp < 0) == bool(sign));
				check_emitted_predicate(&regs.fp_result, sign ? 1 : 0);
			}
			assert(fpp_get_fpsr() == (sign | FPSR_CC_I));
		}
		mode(0);
		regs.fpsr = FPSR_CC_NAN;
		fpp_to_exten(&regs.fp_result, 0x7fff0000, 0xc0000000, 0);
		assert(fpp_cond(0x18) == 1);
		assert((regs.fpsr & (FPSR_BSUN | FPSR_AE_IOP)) == (FPSR_BSUN | FPSR_AE_IOP));
		mode(FPSR_BSUN);
		regs.fpsr = FPSR_CC_NAN;
		trapped_vector = 0;
		assert(fpp_cond(0x18) == -2);
		assert(trapped_vector == 48);
	}
	currprefs.cachesize = 0;
	currprefs.compfpu = false;
	check_host_status_clear();
	check_status_lifecycle();
	check_jit_helper();
	run_generated_cases();
	benchmark_exponentials();
	return 0;
#endif
}
