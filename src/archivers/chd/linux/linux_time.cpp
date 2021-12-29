#include "../chdtypes.h"
// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  wintime.c - Linux OSD core timing functions
//
//============================================================

// standard windows headers
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
//#include <mmsystem.h>

// MAME headers
#include "../osdcore.h"



//============================================================
//  GLOBAL VARIABLES
//============================================================

static osd_ticks_t ticks_per_second = 0;
static osd_ticks_t suspend_ticks = 0;
static BOOL using_qpc = TRUE;



//============================================================
//  osd_ticks
//============================================================

osd_ticks_t osd_ticks(void)
{
	Uint64 performance_count;

	// if we're suspended, just return that
	if (suspend_ticks != 0)
		return suspend_ticks;

	// if we have a per second count, just go for it
	if (ticks_per_second != 0)
	{
		performance_count = SDL_GetPerformanceCounter();
		return performance_count - suspend_ticks;
	}

	// if not, we have to determine it
	performance_count = SDL_GetPerformanceFrequency();
	using_qpc = performance_count != 0;
	if (using_qpc)
		ticks_per_second = performance_count;
	else
		ticks_per_second = 1000;

	// call ourselves to get the first value
	return osd_ticks();
}


//============================================================
//  osd_ticks_per_second
//============================================================

osd_ticks_t osd_ticks_per_second(void)
{
	if (ticks_per_second == 0)
		osd_ticks();
	return ticks_per_second;
}


//============================================================
//  osd_sleep
//============================================================

void osd_sleep(osd_ticks_t duration)
{
	// make sure we've computed ticks_per_second
	if (ticks_per_second == 0)
		(void)osd_ticks();

	// convert to milliseconds, rounding down
	auto msec = (duration * 1000 / ticks_per_second);

	// only sleep if at least 2 full milliseconds
	if (msec >= 2)
	{
		// take a couple of msecs off the top for good measure
		msec -= 2;

		// bump our thread priority super high so that we get
		// priority when we need it
		SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);
		Sleep(msec);
		SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
	}
}
