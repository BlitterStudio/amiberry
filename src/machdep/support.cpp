#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"

#include <time.h>
#include "uae/time.h"

int64_t g_uae_epoch = 0;
static int usedtimermode = 0;

static uae_time_t read_monotonic_nanoseconds(void)
{
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return ((ts.tv_sec * 1000000000LL) + ts.tv_nsec) - g_uae_epoch;
}

uae_time_t uae_time(void)
{
	if (usedtimermode == 1) {
		return read_processor_time_rdtsc();
	}
	return read_monotonic_nanoseconds();
}

uae_s64 read_system_time(void)
{
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return (ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000LL);
}

static void figure_processor_speed_rdtsc(void)
{
	static int freqset;
	if (freqset) return;
	freqset = 1;

	uae_s64 clockrate;
	uae_s64 start = read_processor_time_rdtsc();
	uae_s64 start_sys = read_system_time();
	while (read_system_time() < start_sys + 500);
	clockrate = (read_processor_time_rdtsc() - start) * 2;

	write_log(_T("CLOCKFREQ: RDTSC %.4fMHz\n"), clockrate / 1000000.0);
	syncbase = clockrate;
}

void uae_time_calibrate()
{
	g_uae_epoch = 0;
	if (usedtimermode == 1) {
		figure_processor_speed_rdtsc();
	} else {
		g_uae_epoch = read_monotonic_nanoseconds();
		syncbase = 1000000000; // Nanoseconds
		write_log(_T("CLOCKFREQ: MONOTONIC %.4fMHz\n"), syncbase / 1000000.0);
	}
}

void uae_time_use_mode(int mode)
{
	usedtimermode = mode;
	uae_time_calibrate();
}

void uae_time_init(void)
{
	static bool initialized = false;
	if (initialized) return;
	uae_time_calibrate();
	initialized = true;
}