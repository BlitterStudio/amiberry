 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Events
  * These are best for low-frequency events. Having too many of them,
  * or using them for events that occur too frequently, can cause massive
  * slowdown.
  *
  * Copyright 1995-1998 Bernd Schmidt
  */

#ifndef UAE_EVENTS_H
#define UAE_EVENTS_H

#include "uae/types.h"

#undef EVENT_DEBUG

#include "machdep/rpt.h"

#define EVT_MAX 0x7fffffffffffffff

extern frame_time_t vsyncmintime, vsyncmintimepre;
extern frame_time_t vsyncmaxtime, vsyncwaittime;
extern frame_time_t vsynctimebase, syncbase;
extern void reset_frame_rate_hack(void);
extern evt_t vsync_cycles;
extern evt_t start_cycles;
extern bool event_wait;

extern void event_init(void);
extern void compute_vsynctime(void);
extern void init_eventtab(void);
extern void do_cycles_ce(int cycles);
extern void do_cycles_ce020(int cycles);
extern void events_schedule(void);
extern void do_cycles_slow(int cycles_to_add);
extern void events_reset_syncline(void);
extern void clear_events(void);

extern bool is_cycle_ce(uaecptr);

extern evt_t currcycle, nextevent;
extern int is_syncline;
extern evt_t is_syncline_end;
typedef void (*evfunc)(void);
typedef void (*evfunc2)(uae_u32);

struct ev
{
	bool active;
	evt_t evtime, oldcycles;
	evfunc handler;
};

struct ev2
{
	bool active;
	evt_t evtime;
	uae_u32 data;
	evfunc2 handler;
	ev2 *next;
};

// hsync handlers must have priority over misc
enum {
	ev_cia, ev_hsync, ev_hsynch, ev_misc,
	ev_audio,
	ev_max
};

enum {
	ev2_blitter, ev2_misc,
	ev2_max = 12
};

extern int pissoff_value;
extern int pissoff;

#define countdown pissoff
#define do_cycles do_cycles_slow

extern struct ev eventtab[ev_max];
extern struct ev2 eventtab2[ev2_max];

extern int maxhpos;

STATIC_INLINE void cycles_do_special (void)
{
#ifdef JIT
	if (currprefs.cachesize) {
		if (pissoff >= 0)
			pissoff = -1;
	} else
#endif
	{
		pissoff = 0;
	}
}

STATIC_INLINE void do_extra_cycles(int cycles_to_add)
{
	pissoff -= cycles_to_add;
}

STATIC_INLINE evt_t get_cycles(void)
{
	return currcycle;
}

STATIC_INLINE void set_cycles (evt_t x)
{
	currcycle = x;
	eventtab[ev_hsync].oldcycles = x;
	eventtab[ev_hsynch].active = 0;
#ifdef EVT_DEBUG
	if (currcycle & (CYCLE_UNIT - 1))
		write_log (_T("%x\n"), currcycle);
#endif
}

STATIC_INLINE int current_hpos_safe(void)
{
    int hp = (int)((get_cycles() - eventtab[ev_hsync].oldcycles)) / CYCLE_UNIT;
	return hp;
}

extern int current_hpos(void);

STATIC_INLINE bool cycles_in_range(evt_t endcycles)
{
	evt_t c = get_cycles();
	return endcycles > c;
}

extern void MISC_handler(void);
extern void event2_newevent_xx(int no, evt_t t, uae_u32 data, evfunc2 func);
extern void event2_newevent_x_replace(evt_t t, uae_u32 data, evfunc2 func);
extern void event2_newevent_x_replace_exists(evt_t t, uae_u32 data, evfunc2 func);
extern void event2_newevent_x_remove(evfunc2 func);
extern void event2_newevent_xx_ce(evt_t t, uae_u32 data, evfunc2 func);

STATIC_INLINE void event2_newevent_x(int no, evt_t t, uae_u32 data, evfunc2 func)
{
	if (t <= 0) {
		func(data);
		return;
	}
	event2_newevent_xx(no, t * CYCLE_UNIT, data, func);
}

STATIC_INLINE void event2_newevent(int no, evt_t t, uae_u32 data)
{
	event2_newevent_x(no, t, data, eventtab2[no].handler);
}
STATIC_INLINE void event2_newevent2(evt_t t, uae_u32 data, evfunc2 func)
{
	event2_newevent_x(-1, t, data, func);
}

STATIC_INLINE void event2_remevent(int no)
{
	eventtab2[no].active = 0;
}

void event_audxdat_func(uae_u32);
void event_setdsr(uae_u32);
void event_CIA_synced_interrupt(uae_u32);
void event_CIA_tod_inc_event(uae_u32);
void event_DISK_handler(uae_u32 data);

#endif /* UAE_EVENTS_H */
