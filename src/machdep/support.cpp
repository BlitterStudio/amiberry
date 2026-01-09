#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"

#include <time.h>
#include "uae/time.h"

int64_t g_uae_epoch = 0;

uae_time_t uae_time(void)
{
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	// CLOCK_MONOTONIC_RAW is faster and not affected by NTP adjustments
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	// Combine calculations to reduce operations
	return ((ts.tv_sec * 1000000000LL) + ts.tv_nsec) - g_uae_epoch;
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

void uae_time_calibrate()
{
	// Initialize timebase
	g_uae_epoch = 0;
	g_uae_epoch = uae_time();
	syncbase = 1000000000; // Nanoseconds
}

void uae_time_init(void)
{
	static bool initialized = false;
	if (initialized) {
		return;
	}
	uae_time_calibrate();
	initialized = true;
}