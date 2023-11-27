/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  */

#ifndef _RPT_H_
#define _RPT_H_

/* frame_time_t is often cast to int in the code so we use int for now... */
typedef uae_s64 uae_time_t;

void uae_time_calibrate();
typedef uae_time_t frame_time_t;

extern int64_t g_uae_epoch;
/* Returns elapsed time in microseconds since start of emulator. */
static __inline__ frame_time_t read_processor_time()
{
	return frame_time_t((SDL_GetPerformanceCounter() / 1000) - g_uae_epoch);
}

#endif /* _RPT_H_ */

