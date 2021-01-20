/*
  * UAE - The Un*x Amiga Emulator
  *
  * Definitions for accessing cycle counters on a given machine, if possible.
  *
  * Copyright 1998 Bernd Schmidt
  */

#ifndef _RPT_H_
#define _RPT_H_

#if defined(__PSP2__) // NOT __SWITCH__
#include <psp2/kernel/processmgr.h>
#endif

#if defined(__SWITCH__)
#include <switch.h>
#endif

void uae_time_calibrate(void);
typedef unsigned long frame_time_t;

extern int64_t g_uae_epoch;

/* Returns elapsed time in microseconds since start of emulator. */
static __inline__ frame_time_t read_processor_time (void)
{
  int64_t time;
#if defined(__PSP2__) // NOT __SWITCH__
  time = sceKernelGetProcessTimeWide();
#elif defined(__SWITCH__)
time = (int64_t) ((svcGetSystemTick() * 1000000) / 19200000);
#else
  struct timespec ts{};

  clock_gettime (CLOCK_MONOTONIC, &ts);

  time = (int64_t) ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#endif
  return time - g_uae_epoch;
}


static __inline__ int64_t read_processor_time_ns (void)
{
  int64_t time;
#if defined(__PSP2__) // NOT __SWITCH__
  time = sceKernelGetProcessTimeWide();
#elif defined(__SWITCH__)
time = (int64_t) ((svcGetSystemTick() * 1000000) / 19200000);
#else
  struct timespec ts{};
  
  clock_gettime (CLOCK_MONOTONIC, &ts);

  time = (int64_t) ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
  return time;
}

#endif /* _RPT_H_ */

