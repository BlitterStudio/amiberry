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
#ifdef WITH_PPC
#include "uae/ppc.h"
#endif
#include "xwin.h"
#ifdef WITH_X86
#include "x86.h"
#endif
#include "audio.h"
#include "cia.h"

static const int pissoff_nojit_value = 256 * CYCLE_UNIT;

evt_t event_cycles, nextevent, currcycle;
int is_syncline;
frame_time_t is_syncline_end;
int cycles_to_next_event;
int max_cycles_to_next_event;
int cycles_to_hsync_event;
evt_t start_cycles;
bool event_wait;

frame_time_t vsyncmintime, vsyncmintimepre;
frame_time_t vsyncmaxtime, vsyncwaittime;
frame_time_t vsynctimebase;

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
	int i;

	evt_t mintime = EVT_MAX;
	for (i = 0; i < ev_max; i++) {
		if (eventtab[i].active) {
			evt_t eventtime = eventtab[i].evtime - currcycle;
			if (eventtime < mintime)
				mintime = eventtime;
		}
	}
	if (mintime < EVT_MAX) {
		nextevent = currcycle + mintime;
	} else {
		nextevent = EVT_MAX;
	}
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
			frame_time_t rpt = read_processor_time();
			frame_time_t v = rpt - is_syncline_end;
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
			frame_time_t rpt = read_processor_time();
			frame_time_t v = rpt - vsyncmintime;
			frame_time_t v2 = rpt - is_syncline_end;
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

void do_cycles_slow(int cycles_to_add)
{
#ifdef WITH_X86
#if 0
	if (x86_turbo_on) {
		execute_other_cpu_single();
	}
#endif
#endif

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

		cycles_to_add -= (int)(nextevent - currcycle);
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
		events_schedule();

	}
	currcycle += cycles_to_add;
}

static ev2 *last_event2;
static ev2 dummy_event;

void MISC_handler(void)
{
	static bool dorecheck;
	bool recheck;
	int i;
	evt_t mintime;
	evt_t ct = get_cycles();
	static int recursive;

	if (recursive) {
		dorecheck = true;
		return;
	}
	recursive++;
	eventtab[ev_misc].active = 0;
	recheck = true;
	while (recheck) {
		recheck = false;
		mintime = EVT_MAX;
		for (i = 0; i < ev2_max; i++) {
			ev2 *e = &eventtab2[i];
			if (e->active) {
				if (e->evtime == ct) {
					e->active = false;
					e->handler(e->data);
					ev2 *e2 = e->next;
					if (e2) {
						e->next = NULL;
						if (e2->active && e2->evtime == e->evtime + 1) {
							e2->active = false;
							e2->handler(e2->data);
						}
					}
					if (dorecheck || e->active) {
						recheck = true;
						dorecheck = false;
					}
				} else {
					evt_t eventtime = e->evtime - ct;
					if (eventtime < mintime)
						mintime = eventtime;
				}
			}
		}
	}
	if (mintime < EVT_MAX) {
		ev *e = &eventtab[ev_misc];
		e->active = true;
		e->oldcycles = ct;
		e->evtime = ct + mintime;
		events_schedule();
	}
	recursive--;
}

void event2_newevent_xx_ce(evt_t t, uae_u32 data, evfunc2 func)
{
	if (!currprefs.cpu_memory_cycle_exact) {
		func(data);
		return;
	}
	event2_newevent_xx(-1, t, data, func);
}

void event2_newevent_xx(int no, evt_t t, uae_u32 data, evfunc2 func)
{
	evt_t et;
	static int next = ev2_misc;

	et = t + get_cycles();
	if (no < 0) {
		no = next;
		for (;;) {
			if (!eventtab2[no].active) {
				break;
			}
			if (eventtab2[no].evtime == et && eventtab2[no].handler == func && eventtab2[no].data == data)
				break;
			no++;
			if (no == ev2_max)
				no = ev2_misc;
			if (no == next) {
				write_log (_T("out of event2's!\n"));
				// execute most recent event immediately
				evt_t mintime = EVT_MAX;
				int minevent = -1;
				evt_t ct = get_cycles();
				for (int i = 0; i < ev2_max; i++) {
					if (eventtab2[i].active) {
						evt_t eventtime = eventtab2[i].evtime - ct;
						if (eventtime < mintime) {
							mintime = eventtime;
							minevent = i;
						}
					}
				}
				if (minevent >= 0) {
					eventtab2[minevent].active = false;
					eventtab2[minevent].handler(eventtab2[minevent].data);
				}
				continue;
			}
		}
		next = no;
	}
	ev2 *e = &eventtab2[no];
	// if previous event has same expiry time, make sure it gets executed first.
	if (last_event2->active && last_event2 != e && et == last_event2->evtime) {
		last_event2->next = e;
		et++;
	}
	e->active = true;
	e->evtime = et;
	e->handler = func;
	e->data = data;
	last_event2 = e;
	MISC_handler();
}

void event2_newevent_x_replace_exists(evt_t t, uae_u32 data, evfunc2 func)
{
	for (int i = 0; i < ev2_max; i++) {
		if (eventtab2[i].active && eventtab2[i].handler == func) {
			eventtab2[i].active = false;
			if (t <= 0) {
				func(data);
				return;
			}
			event2_newevent_xx(-1, t * CYCLE_UNIT, data, func);
			return;
		}
	}
}

void event2_newevent_x_remove(evfunc2 func)
{
	for (int i = 0; i < ev2_max; i++) {
		if (eventtab2[i].active && eventtab2[i].handler == func) {
			eventtab2[i].active = false;
		}
	}
}

void event2_newevent_x_replace(evt_t t, uae_u32 data, evfunc2 func)
{
	event2_newevent_x_remove(func);
	if (t <= 0) {
		func(data);
		return;
	}
	event2_newevent_xx(-1, t * CYCLE_UNIT, data, func);
}

void event_init(void)
{
	last_event2 = &dummy_event;
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

void clear_events(void)
{
	nextevent = EVT_MAX;
	for (int i = 0; i < ev_max; i++) {
		eventtab[i].active = 0;
		eventtab[i].oldcycles = get_cycles();
	}
	for (int i = 0; i < ev2_max; i++) {
		eventtab2[i].active = 0;
	}
}
