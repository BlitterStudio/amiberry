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

void uae_time_calibrate(void);
typedef uae_time_t frame_time_t;

extern int64_t g_uae_epoch;

/* Returns elapsed time in microseconds since start of emulator. */
static __inline__ frame_time_t read_processor_time(void)
{
    struct timespec ts {};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    // Convert seconds to microseconds using bit shifting
    const int64_t seconds_in_microseconds = ts.tv_sec << 20;

    // Convert nanoseconds to microseconds using bit shifting
    const int64_t nanoseconds_in_microseconds = ts.tv_nsec >> 10;

    // Calculate the total time in microseconds
    const int64_t time = seconds_in_microseconds + nanoseconds_in_microseconds;

    return time - g_uae_epoch;
}

#endif /* _RPT_H_ */
