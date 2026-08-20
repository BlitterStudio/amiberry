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

#include <cstddef>

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

inline constexpr std::size_t android_virtual_rtg_mode_count =
	sizeof(android_virtual_rtg_modes) / sizeof(android_virtual_rtg_modes[0]);

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

// Append the decimal digits of value at dst[pos]; returns the new position.
// Never writes past N - 1 so the caller can terminate the string.
template <std::size_t N>
inline std::size_t android_rtg_append_uint(char (&dst)[N], std::size_t pos, unsigned int value)
{
	char digits[10];
	int n = 0;
	do {
		digits[n++] = static_cast<char>('0' + value % 10);
		value /= 10;
	} while (value != 0);
	while (n > 0 && pos + 1 < N)
		dst[pos++] = digits[--n];
	return pos;
}

// Format "<w>x<h> ANDROID" into name. The longest table entry ("2560x1440
// ANDROID", 17 chars) fits name[25] comfortably; the bounded writers keep any
// input safe regardless.
template <std::size_t N>
inline void android_rtg_make_name(char (&name)[N], const unsigned int w, const unsigned int h)
{
	std::size_t pos = 0;
	pos = android_rtg_append_uint(name, pos, w);
	if (pos + 1 < N)
		name[pos++] = 'x';
	pos = android_rtg_append_uint(name, pos, h);
	for (const char* s = android_rtg_name_suffix; *s != '\0' && pos + 1 < N; s++)
		name[pos++] = *s;
	name[pos] = '\0';
}

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
		android_rtg_make_name(out[count].name, out[count].width, out[count].height);
		count++;
	}
	return count;
}
