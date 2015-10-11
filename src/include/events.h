#ifndef EVENTS_H
#define EVENTS_H

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

#include "md-pandora/rpt.h"

extern volatile frame_time_t vsynctime, vsyncmintime;
extern void reset_frame_rate_hack (void);
extern frame_time_t syncbase;
extern int speedup_cycles, speedup_timelimit;

extern void compute_vsynctime (void);
extern void init_eventtab (void);
extern void do_cycles_ce (long cycles);

extern unsigned long currcycle, nextevent, is_lastline;
extern unsigned long last_synctime;
extern unsigned long sample_evtime;
typedef void (*evfunc)(void);
typedef void (*evfunc2)(uae_u32);

typedef unsigned long int evt;

struct ev
{
    int active;
    evt evtime, oldcycles;
    evfunc handler;
};

struct ev2
{
    int active;
    evt evtime;
    uae_u32 data;
    evfunc2 handler;
};

enum {
    ev_hsync, 
    ev_copper, 
    ev_audio, ev_cia, ev_misc,
    ev_max
};

enum {
    ev2_blitter, ev2_disk, ev2_misc,
    ev2_max = 8
};

extern struct ev eventtab[ev_max];
extern struct ev2 eventtab2[ev2_max];

extern void event2_newevent(int, evt);
extern void event2_newevent2(evt, uae_u32, evfunc2);
extern void event2_remevent(int);

#ifdef JIT
#include "events_jit.h"
#else
#include "events_normal.h"
#endif

#endif

