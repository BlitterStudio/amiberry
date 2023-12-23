#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"

static int qpcdivisor = 0;

uae_time_t uae_time()
{
	Uint64 t;
	const auto counter = SDL_GetPerformanceCounter();
	if (qpcdivisor == 0)
		t = counter;
	else
		t = counter >> qpcdivisor;
	if (!t)
		t++;
	return static_cast<frame_time_t>(t);
}

void uae_time_calibrate()
{
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
