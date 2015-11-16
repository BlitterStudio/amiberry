/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  */

#ifndef _RPT_H_
#define _RPT_H_

typedef unsigned long frame_time_t;

extern int64_t g_uae_epoch;

/* Returns elapsed time in microseconds since start of emulator. */
static __inline__ frame_time_t read_processor_time (void)
{
  int64_t time;
  struct timespec ts;
  
  clock_gettime (CLOCK_MONOTONIC, &ts);

  time = (((int64_t) ts.tv_sec) * 1000000) + (ts.tv_nsec / 1000);
  return time - g_uae_epoch;
}

#endif /* _RPT_H_ */

