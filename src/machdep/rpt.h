/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  */

#ifndef _RPT_H_
#define _RPT_H_

typedef uae_s64 uae_time_t;

void uae_time_init(void);
void uae_time_calibrate(void);
uae_time_t uae_time(void);

typedef uae_time_t frame_time_t;

static inline frame_time_t read_processor_time(void)
{
	return uae_time();
}

#endif /* _RPT_H_ */

