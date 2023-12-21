#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"

static int qpcdivisor = 0;

/* Returns elapsed time in microseconds since start of emulator. */
uae_time_t uae_time()
{
	frame_time_t t;
	auto counter = SDL_GetPerformanceCounter();
	if (qpcdivisor == 0)
		t = static_cast<frame_time_t>(counter);
	else
		t = static_cast<frame_time_t>(counter >> qpcdivisor);
	if (!t)
		t++;
	return t;
}

void uae_time_calibrate()
{
	// Initialize timebase
	auto freq = SDL_GetPerformanceFrequency();
	// limit to 10MHz
	qpcdivisor = 0;
	while (freq > 10000000)
	{
		freq >>= 1;
		qpcdivisor++;
	}

	syncbase = static_cast<frame_time_t>(freq);
}