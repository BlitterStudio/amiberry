/*
 * UAE - The Un*x Amiga Emulator
 *
 * CPU Threading - Synchronization and Performance Analysis Implementation
 *
 * (c) 2026 Amiberry Development Team
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "cpu_thread.h"
#include "memory.h"
#include "events.h"
#include "newcpu.h"

#ifdef WITH_THREADED_CPU
/* Global diagnostics structure */
cpu_thread_diagnostics cpu_thread_diag = {0};

void cpu_thread_diag_init(void)
{
	memset(&cpu_thread_diag, 0, sizeof(cpu_thread_diag));
	cpu_thread_diag.enable_diagnostics = 0;
}

void cpu_thread_diag_reset(void)
{
	atomic_set(&cpu_thread_diag.stall_count, 0);
	atomic_set(&cpu_thread_diag.stall_cycles_lost, 0);
	atomic_set(&cpu_thread_diag.longest_stall, 0);
	atomic_set(&cpu_thread_diag.is_stalled, 0);
	cpu_thread_diag.stall_start_cycle = 0;
}

void cpu_thread_diag_log_stall(evt_t duration)
{
	if (!cpu_thread_diag.enable_diagnostics)
		return;

	atomic_inc(&cpu_thread_diag.stall_count);
	atomic_add(&cpu_thread_diag.stall_cycles_lost, duration);

	evt_t longest = atomic_read(&cpu_thread_diag.longest_stall);
	while (duration > longest) {
		if (atomic_cas(&cpu_thread_diag.longest_stall, longest, duration)) {
			break;
		}
		longest = atomic_read(&cpu_thread_diag.longest_stall);
	}
}

void cpu_thread_diag_print_stats(void)
{
	if (!cpu_thread_diag.enable_diagnostics)
		return;

	uae_u32 stalls = atomic_read(&cpu_thread_diag.stall_count);
	uae_u32 cycles = atomic_read(&cpu_thread_diag.stall_cycles_lost);
	uae_u32 longest = atomic_read(&cpu_thread_diag.longest_stall);

	write_log(_T("\n"));
	write_log(_T("================= CPU THREAD DIAGNOSTICS REPORT =================\n"));
	write_log(_T("Configuration:\n"));
	write_log(_T("  CPU Threading Enabled: %s\n"), currprefs.cpu_thread ? _T("Yes") : _T("No"));
	write_log(_T("  CPU Model: %d\n"), currprefs.cpu_model);
	write_log(_T("  Cycle Exact: %s\n"), currprefs.cpu_cycle_exact ? _T("Yes") : _T("No"));
	write_log(_T("  Memory Cycle Exact: %s\n"), currprefs.cpu_memory_cycle_exact ? _T("Yes") : _T("No"));
	write_log(_T("\n"));
	write_log(_T("Synchronization Stalls:\n"));
	write_log(_T("  Total stalls: %u\n"), stalls);
	write_log(_T("  Total cycles lost to stalls: %u\n"), cycles);
	if (stalls > 0) {
		write_log(_T("  Average cycles/stall: %.2f\n"), (float)cycles / stalls);
	}
	write_log(_T("  Longest single stall: %u cycles\n"), longest);
	write_log(_T("\n"));

	if (stalls > 0) {
		write_log(_T("ANALYSIS:\n"));
		if (cycles > (stalls * 1000)) {
			write_log(_T("  WARNING: Significant cycle loss due to thread synchronization.\n"));
		}
		if (longest > 5000) {
			write_log(_T("  WARNING: Very long stall detected (%u cycles).\n"), longest);
			write_log(_T("  This may indicate main thread contention or IO bottleneck.\n"));
		}
	} else {
		write_log(_T("  No synchronization stalls detected - CPU threading working well.\n"));
	}
	write_log(_T("================= END DIAGNOSTICS REPORT =================\n"));
	write_log(_T("\n"));
}

void cpu_thread_stall_start(void)
{
	if (!cpu_thread_diag.enable_diagnostics) {
		return;
	}

	atomic_set(&cpu_thread_diag.is_stalled, 1);
	cpu_thread_diag.stall_start_cycle = get_cycles();
}

void cpu_thread_stall_end(evt_t duration)
{
	if (!cpu_thread_diag.enable_diagnostics) {
		return;
	}

	atomic_set(&cpu_thread_diag.is_stalled, 0);
	cpu_thread_diag_log_stall(duration);
}

#endif
