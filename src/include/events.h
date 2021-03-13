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

#include "machdep/rpt.h"

extern frame_time_t vsyncmintime, vsyncmintimepre;
extern frame_time_t vsyncmaxtime, vsyncwaittime;
extern int vsynctimebase, syncbase;
extern void reset_frame_rate_hack (void);
extern unsigned long int vsync_cycles;
extern unsigned long start_cycles;
extern int event2_count;
extern bool event_wait;

extern void compute_vsynctime (void);
extern void init_eventtab (void);
extern void do_cycles_ce (unsigned long cycles);
extern void do_cycles_ce020 (unsigned long cycles);
extern void events_schedule (void);
extern void do_cycles_slow (unsigned long cycles_to_add);
extern void events_reset_syncline(void);

extern bool is_cycle_ce(uaecptr);

extern unsigned long currcycle, nextevent;
extern int is_syncline, is_syncline_end;
typedef void (*evfunc)(void);
typedef void (*evfunc2)(uae_u32);

typedef void (*do_cycles_func)(unsigned long);
extern do_cycles_func do_cycles;
void do_cycles_cpu_fastest(unsigned long cycles_to_add);
void do_cycles_cpu_norm(unsigned long cycles_to_add);

typedef unsigned long int evt;

struct ev
{
    bool active;
    evt evtime, oldcycles;
    evfunc handler;
};

struct ev2
{
    bool active;
    evt evtime;
    uae_u32 data;
    evfunc2 handler;
};

enum {
    ev_copper, 
    ev_cia, ev_audio, ev_misc, ev_hsync,
    ev_max
};

enum {
    ev2_blitter, ev2_disk, ev2_misc,
    ev2_max = 12
};

extern int pissoff_value;
extern uae_s32 pissoff;

#define countdown pissoff

extern struct ev eventtab[ev_max];
extern struct ev2 eventtab2[ev2_max];

extern int hpos_offset;
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

STATIC_INLINE void do_extra_cycles (unsigned long cycles_to_add)
{
	pissoff -= cycles_to_add;
}

STATIC_INLINE unsigned long int get_cycles (void)
{
	return currcycle;
}

STATIC_INLINE void set_cycles (unsigned long int x)
{
	currcycle = x;
	eventtab[ev_hsync].oldcycles = x;
}

STATIC_INLINE int current_hpos_safe (void)
{
    int hp = (get_cycles () - eventtab[ev_hsync].oldcycles) / CYCLE_UNIT;
	return hp;
}

extern int current_hpos(void);

STATIC_INLINE bool cycles_in_range (unsigned long endcycles)
{
	const signed long c = get_cycles ();
	return static_cast<signed long>(endcycles) - c > 0;
}

extern void MISC_handler (void);
extern void event2_newevent_xx (int no, evt t, uae_u32 data, evfunc2 func);
extern void event2_newevent_x_replace(evt t, uae_u32 data, evfunc2 func);

STATIC_INLINE void event2_newevent_x (int no, evt t, uae_u32 data, evfunc2 func)
{
	if (static_cast<int>(t) <= 0) {
		func (data);
		return;
	}
	event2_newevent_xx (no, t * CYCLE_UNIT, data, func);
}

STATIC_INLINE void event2_newevent (int no, evt t, uae_u32 data)
{
	event2_newevent_x (no, t, data, eventtab2[no].handler);
}
STATIC_INLINE void event2_newevent2 (evt t, uae_u32 data, evfunc2 func)
{
	event2_newevent_x (-1, t, data, func);
}

STATIC_INLINE void event2_remevent (int no)
{
	eventtab2[no].active = false;
}

#endif /* UAE_EVENTS_H */
