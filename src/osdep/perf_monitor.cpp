#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "perf_monitor.h"

struct perf_line_counters perf_lines;

// Accumulators for the periodic stats log
static uae_s64 acc_frames;
static uae_s64 acc_frame_ns;
static uae_s64 acc_idle_ns;
static uae_s64 acc_render_ns;
static uae_s64 acc_present_ns;
static uae_s64 acc_lines_full;
static uae_s64 acc_lines_fast;
static uae_s64 acc_lines_skipped;
static int acc_late_frames;

// Rolling window for slow-host detection: one entry per frame,
// 1 = frame exceeded its budget by more than 10%.
#define PERF_OVERRUN_WINDOW 256
#define PERF_OVERRUN_TRIGGER (PERF_OVERRUN_WINDOW * 3 / 4)
static uae_u8 overrun_window[PERF_OVERRUN_WINDOW];
static int overrun_pos;
static int overrun_count;
static bool overrun_window_filled;
static bool warning_pending;
static bool warning_emitted;

void perf_monitor_reset(void)
{
	memset(&perf_lines, 0, sizeof perf_lines);
	acc_frames = acc_frame_ns = acc_idle_ns = acc_render_ns = acc_present_ns = 0;
	acc_lines_full = acc_lines_fast = acc_lines_skipped = 0;
	acc_late_frames = 0;
	memset(overrun_window, 0, sizeof overrun_window);
	overrun_pos = 0;
	overrun_count = 0;
	overrun_window_filled = false;
	warning_pending = false;
	warning_emitted = false;
}

void perf_monitor_add_render(uae_s64 ns)
{
	acc_render_ns += ns;
}

void perf_monitor_add_present(uae_s64 ns)
{
	acc_present_ns += ns;
}

bool perf_monitor_warning_pending(void)
{
	return warning_pending;
}

void perf_monitor_clear_warning(void)
{
	warning_pending = false;
}

void perf_monitor_frame(uae_s64 frame_ns, uae_s64 idle_ns, uae_s64 vsynctime_ns, bool frameok)
{
	// Sample and reset the per-frame line counters regardless of logging state
	const int lines_full = perf_lines.full;
	const int lines_fast = perf_lines.fast;
	const int lines_skipped = perf_lines.skipped;
	memset(&perf_lines, 0, sizeof perf_lines);

	if (vsynctime_ns <= 0 || frame_ns <= 0) {
		return;
	}

	// --- Slow-host detection (always active unless disabled or already fired) ---
	if (amiberry_options.slow_host_warning && !warning_emitted && !currprefs.turbo_emulation) {
		const uae_u8 over = frame_ns > vsynctime_ns + vsynctime_ns / 10 ? 1 : 0;
		overrun_count += over - overrun_window[overrun_pos];
		overrun_window[overrun_pos] = over;
		overrun_pos++;
		if (overrun_pos >= PERF_OVERRUN_WINDOW) {
			overrun_pos = 0;
			overrun_window_filled = true;
		}
		if (overrun_window_filled && overrun_count >= PERF_OVERRUN_TRIGGER) {
			warning_pending = true;
			warning_emitted = true;
			write_log(_T("PERF: sustained frame overruns detected (%d of last %d frames >10%% over budget). ")
				_T("This host cannot maintain full emulation speed with the current settings.\n"),
				overrun_count, PERF_OVERRUN_WINDOW);
		}
	}

	// --- Periodic statistics log ---
	if (!amiberry_options.perf_log) {
		return;
	}

	acc_frames++;
	acc_frame_ns += frame_ns;
	acc_idle_ns += idle_ns;
	acc_lines_full += lines_full;
	acc_lines_fast += lines_fast;
	acc_lines_skipped += lines_skipped;
	if (!frameok) {
		acc_late_frames++;
	}

	if (acc_frames >= 256) {
		const double f = (double)acc_frames;
		const double frame_ms = acc_frame_ns / f / 1e6;
		const double idle_ms = acc_idle_ns / f / 1e6;
		const double render_ms = acc_render_ns / f / 1e6;
		const double present_ms = acc_present_ns / f / 1e6;
		double emu_ms = frame_ms - idle_ms - render_ms - present_ms;
		if (emu_ms < 0) {
			emu_ms = 0;
		}
		write_log(_T("PERF: fps=%.1f frame=%.2fms emu=%.2fms render=%.2fms present=%.2fms idle=%.2fms ")
			_T("late=%d%% lines/frame: full=%.0f fast=%.0f skipped=%.0f\n"),
			1e9 / (acc_frame_ns / f), frame_ms, emu_ms, render_ms, present_ms, idle_ms,
			(int)(acc_late_frames * 100 / acc_frames),
			acc_lines_full / f, acc_lines_fast / f, acc_lines_skipped / f);
		acc_frames = acc_frame_ns = acc_idle_ns = acc_render_ns = acc_present_ns = 0;
		acc_lines_full = acc_lines_fast = acc_lines_skipped = 0;
		acc_late_frames = 0;
	}
}
