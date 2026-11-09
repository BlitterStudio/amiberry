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
#include "events.h"
#include "cpu_thread.h"

#include <cstdlib>
#include <cstring>

#ifdef WITH_THREADED_CPU
cpu_thread_diagnostics cpu_thread_diag = {};

void cpu_thread_diag_init(void)
{
	cpu_thread_diag.enable_diagnostics.store(false, std::memory_order_relaxed);
	cpu_thread_diag_reset();
	const char *value = std::getenv("AMIBERRY_CPU_THREAD_DIAGNOSTICS");
	if (!value)
		return;
	if (!std::strcmp(value, "1") || !std::strcmp(value, "true") ||
		!std::strcmp(value, "yes") || !std::strcmp(value, "on")) {
		cpu_thread_diag.enable_diagnostics.store(true, std::memory_order_relaxed);
		write_log(_T("CPU thread diagnostics enabled (host waits and skipped guest cycles reported separately).\n"));
	} else if (std::strcmp(value, "0") && std::strcmp(value, "false") &&
		std::strcmp(value, "no") && std::strcmp(value, "off")) {
		write_log(_T("Invalid AMIBERRY_CPU_THREAD_DIAGNOSTICS='%s'; expected 0/1, false/true, no/yes or off/on; diagnostics disabled.\n"), value);
	}
}

void cpu_thread_diag_reset(void)
{
	__atomic_store_n(&cpu_thread_diag.wait_count, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&cpu_thread_diag.total_wait_ns, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&cpu_thread_diag.longest_wait_ns, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&cpu_thread_diag.skipped_cycle_count, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&cpu_thread_diag.total_skipped_cycles, 0, __ATOMIC_RELAXED);
	__atomic_store_n(&cpu_thread_diag.longest_skipped_cycles, 0, __ATOMIC_RELAXED);
}

static void cpu_thread_diag_max(uae_u64 *counter, uae_u64 duration)
{
	uae_u64 longest = __atomic_load_n(counter, __ATOMIC_RELAXED);
	while (duration > longest &&
		!__atomic_compare_exchange_n(counter, &longest, duration, true,
			__ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
	}
}

void cpu_thread_diag_log_wait(uae_u64 nanoseconds)
{
	if (!cpu_thread_diag_enabled())
		return;
	__atomic_fetch_add(&cpu_thread_diag.wait_count, 1, __ATOMIC_RELAXED);
	__atomic_fetch_add(&cpu_thread_diag.total_wait_ns, nanoseconds, __ATOMIC_RELAXED);
	cpu_thread_diag_max(&cpu_thread_diag.longest_wait_ns, nanoseconds);
}

void cpu_thread_diag_log_stall(evt_t duration)
{
	if (!cpu_thread_diag_enabled() || duration <= 0)
		return;
	__atomic_fetch_add(&cpu_thread_diag.skipped_cycle_count, 1, __ATOMIC_RELAXED);
	__atomic_fetch_add(&cpu_thread_diag.total_skipped_cycles, duration, __ATOMIC_RELAXED);
	cpu_thread_diag_max(&cpu_thread_diag.longest_skipped_cycles, duration);
}

void cpu_thread_diag_print_stats(void)
{
	if (!cpu_thread_diag_enabled())
		return;

	const uae_u64 waits = __atomic_load_n(&cpu_thread_diag.wait_count, __ATOMIC_RELAXED);
	const uae_u64 nanoseconds = __atomic_load_n(&cpu_thread_diag.total_wait_ns, __ATOMIC_RELAXED);
	const uae_u64 longest = __atomic_load_n(&cpu_thread_diag.longest_wait_ns, __ATOMIC_RELAXED);
	const uae_u64 skips = __atomic_load_n(&cpu_thread_diag.skipped_cycle_count, __ATOMIC_RELAXED);
	const uae_u64 cycles = __atomic_load_n(&cpu_thread_diag.total_skipped_cycles, __ATOMIC_RELAXED);
	const uae_u64 longest_skip = __atomic_load_n(&cpu_thread_diag.longest_skipped_cycles, __ATOMIC_RELAXED);

	write_log(_T("\n================= CPU THREAD DIAGNOSTICS REPORT =================\n"));
	write_log(_T("  CPU threading: %s; CPU model: %d; cycle exact: %s; memory cycle exact: %s\n"),
		cpu_threaded_run ? _T("Yes") : _T("No"), currprefs.cpu_model,
		currprefs.cpu_cycle_exact ? _T("Yes") : _T("No"),
		currprefs.cpu_memory_cycle_exact ? _T("Yes") : _T("No"));
	write_log(_T("Mailbox waits (elapsed host time):\n"));
	write_log(_T("  Count: %llu; total: %llu ns (%.3f ms); longest: %llu ns\n"),
		static_cast<unsigned long long>(waits), static_cast<unsigned long long>(nanoseconds),
		(double)nanoseconds / 1000000.0, static_cast<unsigned long long>(longest));
	if (waits)
		write_log(_T("  Average: %.3f us/wait\n"), (double)nanoseconds / waits / 1000.0);
	write_log(_T("Skipped guest cycles (not host wait time):\n"));
	write_log(_T("  Events: %llu; total: %llu cycles; longest: %llu cycles\n"),
		static_cast<unsigned long long>(skips), static_cast<unsigned long long>(cycles),
		static_cast<unsigned long long>(longest_skip));
	write_log(_T("================= END DIAGNOSTICS REPORT =================\n\n"));
}

#endif
