// Included after the same production backend and helper fixtures as the regressions.
#include <algorithm>
#include <array>

static volatile uae_u32 benchmark_sink;

template<typename Operation>
static void measure(const char *name, int iterations, Operation operation)
{
	for (int n = 0; n < 10000; ++n)
		operation();
	std::array<double, 7> samples;
	for (double &sample : samples) {
		const auto start = std::chrono::steady_clock::now();
		for (int n = 0; n < iterations; ++n)
			operation();
		const auto end = std::chrono::steady_clock::now();
		sample = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
	}
	std::sort(samples.begin(), samples.end());
	printf("%-30s %8.2f ns/op [%8.2f, %8.2f]\n",
		name, samples[3], samples.front(), samples.back());
}

int main(int argc, char **argv)
{
	const int result = native_regression_main();
	if (result)
		return result;
	const char *count = getenv("AMIBERRY_FPU_BENCH_ITERATIONS");
	const int iterations = count ? atoi(count) : 1000000;
	assert(iterations > 0);
	currprefs.cachesize = 0;
	currprefs.compfpu = false;
	currprefs.fpu_model = 68882;
	currprefs.fpu_no_unimplemented = false;
	fp_init_native();
	mode(0);
	const bool reference = argc == 2 && !strcmp(argv[1], "--libc-clear");
	assert(argc == 1 || reference);
	if (reference) {
		fpp_clear_status = [] {
			feclearexcept(FE_ALL_EXCEPT);
#ifdef SOFTFLOAT_CONVERSIONS
			fs.float_exception_flags = 0;
#endif
		};
	}
	printf("Exception clearing: %s\n", reference ? "original libc reference" : "production");
	FPP_CLEAR_STATUS volatile clear = fpp_clear_status;
	FPP_GET_STATUS volatile status = fpp_get_status;
	measure("clear clean host status", iterations, [&] { clear(); });
	measure("get clean host status", iterations, [&] {
		uae_u32 flags = 0;
		status(&flags);
		benchmark_sink = flags;
	});
	measure("cached FPCR interpreter", iterations, [&] { fpp_set_mode(0); });
	currprefs.cachesize = 1024;
	currprefs.compfpu = true;
	measure("FPCR JIT reestablish", iterations, [&] { fpp_set_mode(0); });
	currprefs.cachesize = 0;
	currprefs.compfpu = false;

	fpdata initial, source, value;
	fpp_from_int(&initial, 1);
	fpp_from_int(&source, 3);
	FPP_ABP volatile add = fpp_add;
	FPP_ABP volatile divide = fpp_div;
	measure("FADD helper only", iterations, [&] {
		value = initial;
		add(&value, &source, PREC_NORMAL);
	});
	measure("FADD clear/helper/status", iterations, [&] {
		clear();
		value = initial;
		add(&value, &source, PREC_NORMAL);
		uae_u32 flags = 0;
		status(&flags);
		benchmark_sink = flags;
	});
	measure("FDIV inexact helper only", iterations, [&] {
		value = initial;
		divide(&value, &source, PREC_NORMAL);
	});
	measure("FDIV clear/helper/status", iterations, [&] {
		clear();
		value = initial;
		divide(&value, &source, PREC_NORMAL);
		uae_u32 flags = 0;
		status(&flags);
		benchmark_sink = flags;
	});

	regs.instruction_pc = 0x100;
	regs.pc = 0x104;
	regs.fp[1] = source;
	jit_setpc(0x104);
	measure("FADD interpreter frontend", iterations, [&] {
		regs.fp[0] = initial;
		fpuop_arithmetic(0xf200, 0x0422);
	});
	measure("FDIV interpreter frontend", iterations, [&] {
		regs.fp[0] = initial;
		fpuop_arithmetic(0xf200, 0x0420);
	});
	currprefs.cachesize = 1024;
	currprefs.compfpu = true;
	measure("FADD decoded JIT helper", iterations, [&] {
		regs.fp[0] = initial;
		fpuop_arithmetic_jit(0xf2000422, 0);
	});
	measure("FDIV decoded JIT helper", iterations, [&] {
		regs.fp[0] = initial;
		fpuop_arithmetic_jit(0xf2000420, 0);
	});
	return 0;
}
