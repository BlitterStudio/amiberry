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
#include "memory.h"
#include "newcpu.h"
#include "events.h"

frame_time_t vsynctimebase, vsyncmintime;

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

void do_cycles_slow (unsigned long cycles_to_add)
{
  if ((pissoff -= cycles_to_add) > 0)
  	return;

  cycles_to_add = -pissoff;
  pissoff = 0;

  if (is_syncline && eventtab[ev_hsync].evtime - currcycle <= cycles_to_add) {
	  int rpt = read_processor_time ();
	  int v = rpt - vsyncmintime;
	  if (v > (int)syncbase || v < -((int)syncbase))
	    vsyncmintime = rpt;
  	if (v < speedup_timelimit) {
	    pissoff = pissoff_value;
	    return;
  	}
  }

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

void MISC_handler(void)
{
	int i;
  evt mintime;
  evt ct = get_cycles();
  static int recursive;

	if (recursive) {
	  return;
	}
  recursive++;
  eventtab[ev_misc].active = 0;

  mintime = ~0L;
  for (i = 0; i < ev2_max; i++) {
    if (eventtab2[i].active) {
	    if (eventtab2[i].evtime == ct) {
				eventtab2[i].active = false;
        eventtab2[i].handler(eventtab2[i].data);
				if (eventtab2[i].active) {
  	      evt eventtime = eventtab2[i].evtime - ct;
  	      if (eventtime < mintime)
      			mintime = eventtime;
				}
	    } else {
	      evt eventtime = eventtab2[i].evtime - ct;
	      if (eventtime < mintime)
    			mintime = eventtime;
  		}
    }
	}

  if (mintime != ~0L) {
		eventtab[ev_misc].active = true;
	  eventtab[ev_misc].evtime = ct + mintime;
	  events_schedule();
  }
  recursive--;
}
