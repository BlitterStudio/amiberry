/*
 * UAE - The Un*x Amiga Emulator
 *
 * CPU Threading - Synchronization and Performance Analysis
 *
 * (c) 2026 Amiberry Development Team
 */

#ifndef UAE_CPU_THREAD_H
#define UAE_CPU_THREAD_H

#include "uae/types.h"

#ifdef WITH_THREADED_CPU
/* Thread diagnostics and profiling */
typedef struct {
	// Counters
	volatile uae_atomic stall_count;          // Number of times CPU thread stalled on IO
	volatile uae_atomic stall_cycles_lost;     // Cycles wasted on synchronization
	volatile uae_atomic longest_stall;         // Longest single stall in cycles

	// State tracking
	volatile uae_atomic is_stalled;            // Currently stalled?
	evt_t stall_start_cycle;                   // When current stall started

	// Configuration
	int enable_diagnostics;
} cpu_thread_diagnostics;

extern cpu_thread_diagnostics cpu_thread_diag;

/* Function prototypes */
void cpu_thread_diag_init(void);
void cpu_thread_diag_reset(void);
void cpu_thread_diag_log_stall(evt_t duration);
void cpu_thread_diag_print_stats(void);

void cpu_thread_stall_start(void);
void cpu_thread_stall_end(evt_t duration);

#endif /* UAE_CPU_THREAD_H */

#endif
