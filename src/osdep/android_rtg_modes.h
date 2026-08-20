#pragma once

// Android RTG virtual mode table (KTD6). Picasso96 is a virtual framebuffer,
// so the Workbench mode list does not have to be limited to the host's
// fullscreen modes — but Android/ChromeOS hosts often expose only a handful
// of them. This table supplies the common landscape resolutions desktop host
// lists usually contain. It is a separate Android-gated table, deliberately
// not an extension of the desktop missmodes/FAKE synthesis, so desktop mode
// lists stay byte-identical.
//
// Header-only and free of SDL/uae includes, so it compiles and unit-tests
// standalone (tests/android_rtg_modes_test.cpp). The candidates it produces
// are converted to struct PicassoResolution entries at the single call site
// in addresolutions().

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <iterator>

// Depth in bytes per pixel stamped on virtual modes when the host exposed no
// mode to clone one from (32-bit, the usual RTG desktop depth).
inline constexpr int android_rtg_default_depth = 4;

// Refresh rate in Hz stamped on every virtual mode.
inline constexpr int android_rtg_default_refresh = 60;

// Name suffix distinguishing virtual modes from host modes and the desktop
// "FAKE" synthesis in the P96 mode list and logs.
inline constexpr char android_rtg_name_suffix[] = " ANDROID";

// The Android virtual RTG mode table: common landscape resolutions, ascending.
inline constexpr int android_virtual_rtg_modes[][2] = {
	{ 640, 512 },
	{ 800, 600 },
	{ 1024, 600 },
	{ 1024, 768 },
	{ 1152, 864 },
	{ 1280, 720 },
	{ 1280, 768 },
	{ 1280, 800 },
	{ 1280, 960 },
	{ 1280, 1024 },
	{ 1360, 768 },
	{ 1366, 768 },
	{ 1440, 900 },
	{ 1600, 900 },
	{ 1600, 1200 },
	{ 1680, 1050 },
	{ 1920, 1080 },
	{ 1920, 1200 },
	{ 2048, 1152 },
	{ 2560, 1440 }
};

inline constexpr std::size_t android_virtual_rtg_mode_count = std::size(android_virtual_rtg_modes);

// Standalone candidate mirroring the struct PicassoResolution fields the
// virtual-mode synthesis touches. Keeping this POD local is what lets the
// builder below stay independent of amiberry_gfx.h and its SDL includes.
struct android_rtg_candidate {
	unsigned int width;
	unsigned int height;
	int depth;   // bytes per pixel
	int refresh; // Hz
	char name[25];
};

// Build the filtered virtual-mode candidate list from the table.
//
// template_mode: the host mode every entry is cloned from (FAKE-path copy
//   semantics — the template's depth and refresh carry over, the resolution
//   does not), or a zero-initialized candidate when the host exposed no
//   usable mode at all; then depth falls back to android_rtg_default_depth
//   and refresh to android_rtg_default_refresh, so a thin host still yields
//   the full virtual set.
// usable_vram_bytes: RTG VRAM budget with the host-mode guard already applied
//   (gfxmem_bank.allocated_size - 256); entries whose pixel count exceeds it
//   are dropped — the same "at least the 8-bit framebuffer must fit" baseline
//   the host modes use.
// max_modes: remaining slot budget (the MAX_PICASSO_MODES equivalent); at
//   most this many candidates are produced.
// out: array with room for android_virtual_rtg_mode_count entries.
//
// Returns the number of candidates written.
inline int android_rtg_build_modes(const android_rtg_candidate& template_mode,
	const long long usable_vram_bytes, const int max_modes, android_rtg_candidate* out)
{
	const int depth = template_mode.depth > 0 ? template_mode.depth : android_rtg_default_depth;
	const int refresh = template_mode.refresh > 0 ? template_mode.refresh : android_rtg_default_refresh;
	int count = 0;
	for (const auto& mode : android_virtual_rtg_modes) {
		if (count >= max_modes)
			break;
		const long long w = mode[0];
		const long long h = mode[1];
		if (w * h > usable_vram_bytes)
			continue;
		out[count] = template_mode;
		out[count].width = static_cast<unsigned int>(w);
		out[count].height = static_cast<unsigned int>(h);
		out[count].depth = depth;
		out[count].refresh = refresh;
		std::snprintf(out[count].name, sizeof out[count].name, "%ux%u%s",
			out[count].width, out[count].height, android_rtg_name_suffix);
		count++;
	}
	return count;
}

// --- Call-site decision helpers (addresolutions(), picasso96.cpp) ---
//
// The two decisions the call site makes around the builder — how many slots
// remain, and whether a candidate duplicates a host mode already collected —
// live here as pure helpers so the tested code is the shipped code.

// Remaining virtual-mode slots: the raw array capacity minus one reserved
// headroom slot and the host modes already collected. Clamped at zero so an
// overfull host list cannot turn the budget negative.
inline int android_rtg_remaining_mode_budget(const int max_modes, const int existing_count)
{
	return std::max(0, max_modes - 1 - existing_count);
}

// True when (width, height) is already carried by one of the existing host
// modes, i.e. emitting the candidate would duplicate a resolution the host
// list already provides. Templated on the mode entry type (struct
// PicassoResolution at the call site) to keep this header free of SDL/uae
// includes while comparing through the same .res.width/.res.height members
// the call site reads.
template <typename Mode>
inline bool android_rtg_mode_fits_existing(const unsigned int width, const unsigned int height,
	const Mode* existing, const int existing_count)
{
	for (int i = 0; i < existing_count; i++) {
		if (existing[i].res.width == width && existing[i].res.height == height)
			return true;
	}
	return false;
}
