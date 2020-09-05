/*
* UAE - The Un*x Amiga Emulator
*
* Event stuff
*
* Copyright 1995-2002 Bernd Schmidt
* Copyright 1995 Alessandro Bissacco
* Copyright 2000-2012 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "events.h"
#include "memory.h"
#include "newcpu.h"
#include "xwin.h"
#include "audio.h"

static const int pissoff_nojit_value = 256 * CYCLE_UNIT;

unsigned long int nextevent, currcycle;
int is_syncline, is_syncline_end;

frame_time_t vsyncmintime, vsyncmaxtime, vsyncwaittime;
int vsynctimebase;

static void events_fast(void)
{
	cycles_do_special();
}

void events_reset_syncline(void)
{
	is_syncline = 0;
	events_fast();
}

void events_schedule(void)
{
	unsigned long int mintime = ~0L;
	for (auto& i : eventtab)
	{
		if (i.active)
		{
			auto eventtime = i.evtime - currcycle;
			if (eventtime < mintime)
				mintime = eventtime;
		}
	}
	nextevent = currcycle + mintime;
}

extern void vsync_event_done(void);

static bool event_check_vsync(void)
{
	/* Keep only CPU emulation running while waiting for sync point. */
	if (is_syncline == -1
		|| is_syncline == -2
		|| is_syncline == -3
		|| is_syncline > 0
		|| is_syncline <= -100) {

		if (!isvsync_chipset()) {
			events_reset_syncline();
			return false;
		}
		// wait for vblank
		audio_finish_pull();
		int done = vsync_isdone(NULL);
		if (!done)
		{
			if (currprefs.cachesize)
				pissoff = pissoff_value;
			else
				pissoff = pissoff_nojit_value;
			return true;
		}
		vsync_event_done();
	}

	else if (is_syncline == -10) {

		// wait is_syncline_end
		//if (event_wait) {
			int rpt = read_processor_time();
			int v = rpt - is_syncline_end;
			if (v < 0)
			{
				if (currprefs.cachesize)
					pissoff = pissoff_value;
				else
					pissoff = pissoff_nojit_value;
				return true;
			}
		//}
		events_reset_syncline();
	}
	else if (is_syncline < -10) {

		// wait is_syncline_end/vsyncmintime
		//if (event_wait) {
			int rpt = read_processor_time();
			int v = rpt - vsyncmintime;
			int v2 = rpt - is_syncline_end;
			if (v > vsynctimebase || v < -vsynctimebase) {
				v = 0;
			}
			if (v < 0 && v2 < 0) {
				if (currprefs.cachesize)
					pissoff = pissoff_value;
				else
					pissoff = pissoff_nojit_value;
				return true;
			}
		//}
		events_reset_syncline();
	}
	return false;
}

void do_cycles_cpu_fastest(uae_u32 cycles_to_add)
{
	if (!currprefs.cpu_thread) {
		if ((pissoff -= cycles_to_add) >= 0)
			return;

		cycles_to_add = -pissoff;
		pissoff = 0;
	} else {
		pissoff = 0x40000000;
	}

	while ((nextevent - currcycle) <= cycles_to_add)
	{
		if (is_syncline)
		{
			if (event_check_vsync())
				return;
		}
		
		cycles_to_add -= (nextevent - currcycle);
		currcycle = nextevent;

		for (auto& i : eventtab)
		{
			if (i.active && i.evtime == currcycle)
			{
				if (i.handler == nullptr)
					i.active = false;
				else
					(*i.handler)();
			}
		}
		events_schedule();
	}
	currcycle += cycles_to_add;
}

void do_cycles_cpu_norm(uae_u32 cycles_to_add)
{
	while ((nextevent - currcycle) <= cycles_to_add)
	{
		cycles_to_add -= (nextevent - currcycle);
		currcycle = nextevent;

		for (auto& i : eventtab)
		{
			if (i.active && i.evtime == currcycle)
			{
				(*i.handler)();
			}
		}
		events_schedule();
	}
	currcycle += cycles_to_add;
}

do_cycles_func do_cycles = do_cycles_cpu_norm;

void MISC_handler(void)
{
	static bool dorecheck;
	bool recheck;
	int i;
	evt mintime;
	evt ct = get_cycles();
	static int recursive;

	if (recursive)
	{
		dorecheck = true;
		return;
	}
	recursive++;
	eventtab[ev_misc].active = false;
	recheck = true;
	while (recheck)
	{
		recheck = false;
		mintime = ~0L;
		for (i = 0; i < ev2_max; i++)
		{
			if (eventtab2[i].active)
			{
				if (eventtab2[i].evtime == ct)
				{
					eventtab2[i].active = false;
					eventtab2[i].handler(eventtab2[i].data);
					if (dorecheck || eventtab2[i].active)
					{
						recheck = true;
						dorecheck = false;
					}
				}
				else
				{
					auto eventtime = eventtab2[i].evtime - ct;
					if (eventtime < mintime)
						mintime = eventtime;
				}
			}
		}
	}
	if (mintime != ~0UL)
	{
		eventtab[ev_misc].active = true;
		eventtab[ev_misc].oldcycles = ct;
		eventtab[ev_misc].evtime = ct + mintime;
		events_schedule();
	}
	recursive--;
}


void event2_newevent_xx(int no, evt t, uae_u32 data, evfunc2 func)
{
	static int next = ev2_misc;

	auto et = t + get_cycles();
	if (no < 0)
	{
		no = next;
		for (;;)
		{
			if (!eventtab2[no].active)
			{
				break;
			}
			if (eventtab2[no].evtime == et && eventtab2[no].handler == func && eventtab2[no].data == data)
				break;
			no++;
			if (no == ev2_max)
				no = ev2_misc;
			if (no == next)
			{
				write_log(_T("out of event2's!\n"));
				return;
			}
		}
		next = no;
	}
	eventtab2[no].active = true;
	eventtab2[no].evtime = et;
	eventtab2[no].handler = func;
	eventtab2[no].data = data;
	MISC_handler();
}

void event2_newevent_x_replace(evt t, uae_u32 data, evfunc2 func)
{
	for (int i = 0; i < ev2_max; i++) {
		if (eventtab2[i].active && eventtab2[i].handler == func) {
			eventtab2[i].active = false;
		}
	}
	if (((int)t) <= 0) {
		func(data);
		return;
	}
	event2_newevent_xx(-1, t * CYCLE_UNIT, data, func);
}
