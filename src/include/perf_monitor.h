#pragma once

#include "uae/types.h"

// Per-frame line rendering statistics. Incremented from the main emulation
// thread (drawing queue entry points and the custom.cpp line cache),
// sampled and zeroed once per frame by perf_monitor_frame().
struct perf_line_counters
{
	int full;     // type-0 lines: full per-CCK Denise pipeline
	int fast;     // type-1/2 lines: fast bitplane/border rendering
	int skipped;  // border/blank lines reused from the previous field without redraw
};
extern struct perf_line_counters perf_lines;

void perf_monitor_reset(void);

// Called once per emulated frame from fpscounter() in custom.cpp.
// All durations are in nanoseconds.
// frame_ns: measured duration of the frame just completed
// idle_ns:  time the emulator spent sleeping during that frame
// vsynctime_ns: the frame duration budget (vsynctimebase)
// frameok:  frame was rendered on time
void perf_monitor_frame(uae_s64 frame_ns, uae_s64 idle_ns, uae_s64 vsynctime_ns, bool frameok);

// Render/present GPU-path timing, accumulated from osdep/amiberry_gfx.cpp.
void perf_monitor_add_render(uae_s64 ns);
void perf_monitor_add_present(uae_s64 ns);

// Slow-host warning: set internally by perf_monitor_frame() after sustained
// frame overruns; surfaced by the GUI, which clears it after handling.
bool perf_monitor_warning_pending(void);
void perf_monitor_clear_warning(void);
