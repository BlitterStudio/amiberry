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
//#include "uae/ppc.h"
#include "xwin.h"
//#include "x86.h"
#include "audio.h"

static const int pissoff_nojit_value = 256 * CYCLE_UNIT;

unsigned long int event_cycles, nextevent, currcycle;
int is_syncline, is_syncline_end;
long cycles_to_next_event;
long max_cycles_to_next_event;
long cycles_to_hsync_event;
unsigned long start_cycles;
bool event_wait;

frame_time_t vsyncmintime, vsyncmintimepre;
frame_time_t vsyncmaxtime, vsyncwaittime;
int vsynctimebase;
int event2_count;

static void events_fast(void)
{
	cycles_do_special();
}

void events_reset_syncline(void)
{
	is_syncline = 0;
	events_fast();
}

void events_schedule (void)
{
	int i;

	unsigned long int mintime = ~0L;
	for (i = 0; i < ev_max; i++) {
		if (eventtab[i].active) {
			unsigned long int eventtime = eventtab[i].evtime - currcycle;
			if (eventtime < mintime)
				mintime = eventtime;
		}
	}
	nextevent = currcycle + mintime;
}

extern void vsync_event_done(void);
extern int vsync_activeheight;

static bool event_check_vsync(void)
{
	/* Keep only CPU emulation running while waiting for sync point. */
	if (is_syncline == -1) {

		if (!isvsync_chipset()) {
			events_reset_syncline();
			return false;
		}
		// wait for vblank
		audio_finish_pull();
		int done = vsync_isdone(NULL);
		if (done == -2) {
			// if no vsync thread
			int vp = target_get_display_scanline(-1);
			if (vp < is_syncline_end)
				done = 1;
			else if (vp > is_syncline_end)
				is_syncline_end = vp;
		}
		if (!done) {
#ifdef WITH_PPC
			if (ppc_state) {
				uae_ppc_execute_quick();
			}
#endif
			if (currprefs.cachesize)
				pissoff = pissoff_value;
			else
				pissoff = pissoff_nojit_value;
			return true;
		}
		vsync_clear();
		vsync_event_done();

	} else if (is_syncline == -2) {

		if (!isvsync_chipset()) {
			events_reset_syncline();
			return false;
		}
		// wait for vblank or early vblank
		audio_finish_pull();
		int done = vsync_isdone(NULL);
		if (done == -2)
			done = 0;
		int vp = target_get_display_scanline(-1);
		if (vp < 0 || vp >= is_syncline_end)
			done = 1;
		if (!done) {
#ifdef WITH_PPC
			if (ppc_state) {
				uae_ppc_execute_quick();
			}
#endif
			if (currprefs.cachesize)
				pissoff = pissoff_value;
			else
				pissoff = pissoff_nojit_value;
			return true;
		}
		vsync_clear();
		vsync_event_done();

	} else if (is_syncline == -3) {
		if (!isvsync_chipset()) {
			events_reset_syncline();
			return false;
		}
		// not vblank
		audio_finish_pull();
		int vp = target_get_display_scanline(-1);
		if (vp <= 0) {
#ifdef WITH_PPC
			if (ppc_state) {
				uae_ppc_execute_quick();
			}
#endif
			if (currprefs.cachesize)
				pissoff = pissoff_value;
			else
				pissoff = pissoff_nojit_value;
			return true;
		}
		vsync_clear();
		vsync_event_done();

	} else if (is_syncline > 0) {

		if (!isvsync_chipset()) {
			events_reset_syncline();
			return false;
		}
		audio_finish_pull();
		// wait for specific scanline
		int vp = target_get_display_scanline(-1);
		if (vp < 0 || is_syncline > vp) {
#ifdef WITH_PPC
			if (ppc_state) {
				uae_ppc_execute_check();
			}
#endif
			if (currprefs.cachesize)
				pissoff = pissoff_value;
			else
				pissoff = pissoff_nojit_value;
			return true;
		}
		vsync_event_done();

	}
	else if (is_syncline <= -100) {

		if (!isvsync_chipset()) {
			events_reset_syncline();
			return false;
		}
		audio_finish_pull();
		// wait for specific scanline
		int vp = target_get_display_scanline(-1);
		if (vp < 0 || vp >= (-(is_syncline + 100))) {
#ifdef WITH_PPC
			if (ppc_state) {
				uae_ppc_execute_check();
			}
#endif
			if (currprefs.cachesize)
				pissoff = pissoff_value;
			else
				pissoff = pissoff_nojit_value;
			return true;
		}
		vsync_event_done();

	} else if (is_syncline == -10) {

		// wait is_syncline_end
		if (event_wait) {
			int rpt = read_processor_time();
			int v = rpt - is_syncline_end;
			if (v < 0) {
#ifdef WITH_PPC
				if (ppc_state) {
					uae_ppc_execute_check();
				}
#endif
				if (currprefs.cachesize)
					pissoff = pissoff_value;
				else
					pissoff = pissoff_nojit_value;
				return true;
			}
		}
		events_reset_syncline();

	} else if (is_syncline < -10) {

		// wait is_syncline_end/vsyncmintime
		if (event_wait) {
			int rpt = read_processor_time();
			int v = rpt - vsyncmintime;
			int v2 = rpt - is_syncline_end;
			if (v > vsynctimebase || v < -vsynctimebase) {
				v = 0;
			}
			if (v < 0 && v2 < 0) {
#ifdef WITH_PPC
				if (ppc_state) {
					if (is_syncline == -11) {
						uae_ppc_execute_check();
					} else {
						uae_ppc_execute_quick();
					}
				}
#endif
				if (currprefs.cachesize)
					pissoff = pissoff_value;
				else
					pissoff = pissoff_nojit_value;
				return true;
			}
		}
		events_reset_syncline();
	}
	return false;
}

void do_cycles_cpu_fastest(unsigned long cycles_to_add)
{
	if (!currprefs.cpu_thread) {
		if ((pissoff -= cycles_to_add) >= 0)
			return;

		cycles_to_add = -pissoff;
		pissoff = 0;
	} else {
		pissoff = 0x40000000;
	}

	while ((nextevent - currcycle) <= cycles_to_add) {

		if (is_syncline) {
			if (event_check_vsync())
				return;
		}

		cycles_to_add -= nextevent - currcycle;
		currcycle = nextevent;

		for (int i = 0; i < ev_max; i++) {
			if (eventtab[i].active && eventtab[i].evtime == currcycle) {
				if (eventtab[i].handler == NULL) {
					gui_message(_T("eventtab[%d].handler is null!\n"), i);
					eventtab[i].active = 0;
				} else {
					(*eventtab[i].handler)();
				}
			}
		}
		events_schedule ();
	}
	currcycle += cycles_to_add;
}

void do_cycles_cpu_norm(unsigned long cycles_to_add)
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

void MISC_handler (void)
{
	static bool dorecheck;
	bool recheck;
	int i;
	evt mintime;
	evt ct = get_cycles ();
	static int recursive;

	if (recursive) {
		dorecheck = true;
		return;
	}
	recursive++;
	eventtab[ev_misc].active = false;
	recheck = true;
	while (recheck) {
		recheck = false;
		mintime = ~0L;
		for (i = 0; i < ev2_max; i++) {
			if (eventtab2[i].active) {
				if (eventtab2[i].evtime == ct) {
					eventtab2[i].active = false;
					event2_count--;
					eventtab2[i].handler (eventtab2[i].data);
					if (dorecheck || eventtab2[i].active) {
						recheck = true;
						dorecheck = false;
					}
				} else {
					auto eventtime = eventtab2[i].evtime - ct;
					if (eventtime < mintime)
						mintime = eventtime;
				}
			}
		}
	}
	if (mintime != ~0UL) {
		eventtab[ev_misc].active = true;
		eventtab[ev_misc].oldcycles = ct;
		eventtab[ev_misc].evtime = ct + mintime;
		events_schedule ();
	}
	recursive--;
}


void event2_newevent_xx (int no, evt t, uae_u32 data, evfunc2 func)
{
	static int next = ev2_misc;

	auto et = t + get_cycles ();
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
				write_log (_T("out of event2's!\n"));
				return;
			}
		}
		next = no;
	}
	eventtab2[no].active = true;
	eventtab2[no].evtime = et;
	eventtab2[no].handler = func;
	eventtab2[no].data = data;
	MISC_handler ();
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


int current_hpos (void)
{
	int hp = current_hpos_safe ();
	if (hp < 0 || hp > 256) {
		gui_message(_T("hpos = %d!?\n"), hp);
		hp = 0;
	}
	return hp;
}

