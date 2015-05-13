
STATIC_INLINE void cycles_do_special (void)
{
}
STATIC_INLINE void set_cycles (int c)
{
}

STATIC_INLINE void events_schedule (void)
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

STATIC_INLINE void do_cycles_slow (unsigned long cycles_to_add)
{
    if (is_lastline && eventtab[ev_hsync].evtime - currcycle <= cycles_to_add
	&& (long int)(read_processor_time () - vsyncmintime) < 0)
	return;

    while ((nextevent - currcycle) <= cycles_to_add) {
        int i;
        cycles_to_add -= (nextevent - currcycle);
        currcycle = nextevent;

        for (i = 0; i < ev_max; i++) {
	    if (eventtab[i].active && eventtab[i].evtime == currcycle) {
		(*eventtab[i].handler)();
	    }
	}
        events_schedule();
    }
    currcycle += cycles_to_add;
}

/* This is a special-case function.  Normally, all events should lie in the
   future; they should only ever be active at the current cycle during
   do_cycles.  However, a snapshot is saved during do_cycles, and so when
   restoring it, we may have other events pending.  */
STATIC_INLINE void handle_active_events (void)
{
    int i;
    for (i = 0; i < ev_max; i++) {
	if (eventtab[i].active && eventtab[i].evtime == currcycle) {
	    (*eventtab[i].handler)();
	}
    }
}

STATIC_INLINE unsigned long get_cycles (void)
{
    return currcycle;
}

#define do_cycles do_cycles_slow
