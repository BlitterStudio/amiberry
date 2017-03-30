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
#include "include/memory.h"
#include "newcpu.h"
#include "events.h"

static const int pissoff_nojit_value = 256 * CYCLE_UNIT;

unsigned long int event_cycles, nextevent, currcycle;
int is_syncline, is_syncline_end;
long cycles_to_next_event;
long max_cycles_to_next_event;
long cycles_to_hsync_event;
unsigned long start_cycles;
bool event_wait;

frame_time_t vsyncmintime, vsyncmaxtime, vsyncwaittime;
int vsynctimebase;
int event2_count;

void events_schedule (void)
{
    int i;

    unsigned long int mintime = ~0L;
    for (i = 0; i < ev_max; i++)
    {
        if (eventtab[i].active)
        {
            unsigned long int eventtime = eventtab[i].evtime - currcycle;
            if (eventtime < mintime)
                mintime = eventtime;
        }
    }
    nextevent = currcycle + mintime;
}

void do_cycles_cpu_fastest (unsigned long cycles_to_add)
{
    if ((pissoff -= cycles_to_add) > 0)
        return;

    cycles_to_add = -pissoff;
    pissoff = 0;

	if (is_syncline && eventtab[ev_hsync].evtime - currcycle <= cycles_to_add)
	{
		int rpt = read_processor_time();
		int v = rpt - vsyncmintime;
		if (v > syncbase || v < -syncbase)
			vsyncmintime = rpt;
		if (v < speedup_timelimit)
		{
			pissoff = pissoff_value;
			return;
		}
		is_syncline = 0;
	}

	while ((nextevent - currcycle) <= cycles_to_add)
	{
		int i;
		cycles_to_add -= (nextevent - currcycle);
		currcycle = nextevent;

		for (i = 0; i < ev_max; i++)
		{
			if (eventtab[i].active && eventtab[i].evtime == currcycle)
			{
				(*eventtab[i].handler)();
			}
		}
		events_schedule();
	}
    currcycle += cycles_to_add;
}

void do_cycles_cpu_norm (unsigned long cycles_to_add)
{
    while ((nextevent - currcycle) <= cycles_to_add)
    {
        int i;
        cycles_to_add -= (nextevent - currcycle);
        currcycle = nextevent;

        for (i = 0; i < ev_max; i++)
        {
            if (eventtab[i].active && eventtab[i].evtime == currcycle)
            {
                (*eventtab[i].handler)();
            }
        }
        events_schedule();
    }
    currcycle += cycles_to_add;
}

do_cycles_func do_cycles = do_cycles_cpu_norm;

void MISC_handler(void)
{
    int i;
    evt mintime;
    evt ct = get_cycles();
    static int recursive;

    if (recursive)
    {
        return;
    }
    recursive++;
    eventtab[ev_misc].active = 0;

    mintime = ~0L;
    for (i = 0; i < ev2_max; i++)
    {
        if (eventtab2[i].active)
        {
            if (eventtab2[i].evtime == ct)
            {
                eventtab2[i].active = false;
                eventtab2[i].handler(eventtab2[i].data);
                if (eventtab2[i].active)
                {
                    evt eventtime = eventtab2[i].evtime - ct;
                    if (eventtime < mintime)
                        mintime = eventtime;
                }
            }
            else
            {
                evt eventtime = eventtab2[i].evtime - ct;
                if (eventtime < mintime)
                    mintime = eventtime;
            }
        }
    }

    if (mintime != ~0L)
    {
        eventtab[ev_misc].active = true;
        eventtab[ev_misc].evtime = ct + mintime;
        events_schedule();
    }
    recursive--;
}

void event2_newevent_xx(int no, evt t, uae_u32 data, evfunc2 func)
{
	evt et;
	static int next = ev2_misc;

	et = t + get_cycles();
	if (no < 0) {
		no = next;
		for (;;) {
			if (!eventtab2[no].active) {
				event2_count++;
				break;
			}
			if (eventtab2[no].evtime == et && eventtab2[no].handler == func && eventtab2[no].data == data)
				break;
			no++;
			if (no == ev2_max)
				no = ev2_misc;
			if (no == next) {
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
	if (int(t) <= 0) {
		func(data);
		return;
	}
	event2_newevent_xx(-1, t * CYCLE_UNIT, data, func);
}


int current_hpos(void)
{
	int hp = current_hpos_safe();
	if (hp < 0 || hp > 256) {
		gui_message(_T("hpos = %d!?\n"), hp);
		hp = 0;
	}
	return hp;
}