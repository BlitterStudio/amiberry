#ifndef UAE_TIME_H
#define UAE_TIME_H

#include "uae/types.h"

/* frame_time_t is often cast to int in the code so we use int for now... */
typedef uae_s64 uae_time_t;

void uae_time_init(void);
void uae_time_calibrate(void);
uae_time_t uae_time(void);

void uae_time_use_mode(int mode);
uae_s64 read_system_time(void);

#if defined(_WIN32) && !defined(__GNUC__)
void uae_time_use_rdtsc(bool enable);
uae_s64 read_processor_time_rdtsc(void);
#else
static inline uae_s64 read_processor_time_rdtsc(void)
{
#if defined(__x86_64__) || defined(__i386__)
	unsigned int lo, hi;
	__asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
	return ((uae_u64)hi << 32) | lo;
#elif defined(__aarch64__)
	int64_t v;
	__asm__ volatile("mrs %0, cntvct_el0" : "=r" (v));
	return v;
#elif defined(__arm__)
	unsigned int lo, hi;
	__asm__ volatile("mrrc p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
	return ((uae_u64)hi << 32) | lo;
#else
	return uae_time();
#endif
}
#endif

typedef uae_time_t frame_time_t;

extern int64_t g_uae_epoch;

/* Returns elapsed time in nanoseconds since start of emulator. */
static inline frame_time_t read_processor_time(void)
{
	return uae_time();
}

extern frame_time_t syncbase, cputimebase;

#endif /* UAE_TIME_H */
