#include "sysdeps.h"

#include <time.h>
#include "uae/time.h"

static uae_time_t g_uae_epoch;

static uae_time_t read_monotonic_nanoseconds(void)
{
#if defined(__APPLE__) && defined(CLOCK_MONOTONIC_RAW)
	return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW) - g_uae_epoch;
#else
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return ((ts.tv_sec * 1000000000LL) + ts.tv_nsec) - g_uae_epoch;
#endif
}

uae_time_t uae_time(void)
{
	return read_monotonic_nanoseconds();
}

void uae_time_calibrate()
{
	g_uae_epoch = 0;
	g_uae_epoch = read_monotonic_nanoseconds();
	syncbase = 1000000000; // Nanoseconds
	write_log(_T("CLOCKFREQ: MONOTONIC %.4fMHz\n"), syncbase / 1000000.0);
}

void uae_time_init(void)
{
	static bool initialized = false;
	if (initialized) return;
	uae_time_calibrate();
	initialized = true;
}
