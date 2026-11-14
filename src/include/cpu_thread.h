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

#include <atomic>

/* Thread diagnostics and profiling */
typedef struct {
	// Access counters only with __atomic builtins, including during reset.
	uae_u64 wait_count;
	uae_u64 total_wait_ns;
	uae_u64 longest_wait_ns;
	uae_u64 skipped_cycle_count;
	uae_u64 total_skipped_cycles;
	uae_u64 longest_skipped_cycles;
	std::atomic<bool> enable_diagnostics;
} cpu_thread_diagnostics;

extern cpu_thread_diagnostics cpu_thread_diag;

inline bool cpu_thread_diag_enabled(void)
{
	return cpu_thread_diag.enable_diagnostics.load(std::memory_order_relaxed);
}

// Reads AMIBERRY_CPU_THREAD_DIAGNOSTICS (0/1, false/true, no/yes, off/on).
void cpu_thread_diag_init(void);
// For a coherent new reporting interval, reset while the CPU worker is idle.
void cpu_thread_diag_reset(void);
// Measure an actual mailbox wait with SDL_GetTicksNS(), only when enabled.
void cpu_thread_diag_log_wait(uae_u64 nanoseconds);
// Legacy events/custom hook: skipped guest cycles, NOT elapsed host waits.
void cpu_thread_diag_log_stall(evt_t duration);
void cpu_thread_diag_print_stats(void);


/* Host CPUs this process may run on: the affinity mask and, on Linux, a
 * cgroup v2 CPU quota, not the machine's count. A container, a VM or taskset
 * that leaves one CPU must not get a CPU thread that shares it with the main
 * thread (every hand-off then waits for the scheduler). */
int cpu_thread_usable_host_cpus(void);

#endif /* UAE_CPU_THREAD_H */
