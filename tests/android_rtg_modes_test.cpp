#include <cstdio>
#include <cstring>
#include <iostream>

#include "android_rtg_modes.h"

static int failures;

static void expect_true(const bool condition, const char* message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

static void expect_eq(const long long actual, const long long expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

// Every virtual mode must be named "<w>x<h> ANDROID": the suffix has to be
// distinct from the desktop "FAKE" synthesis so the two populations stay
// diagnosable apart in the P96 mode list and logs (KTD6).
static void expect_named(const android_rtg_candidate& candidate, const char* message)
{
	char expected[64];
	std::snprintf(expected, sizeof expected, "%ux%u ANDROID",
		candidate.width, candidate.height);
	if (std::strcmp(candidate.name, expected) != 0) {
		std::cerr << message << ": expected \"" << expected << "\", got \""
			<< candidate.name << "\"\n";
		failures++;
	}
}

static bool contains_resolution(const android_rtg_candidate* modes, const int count,
	const unsigned int w, const unsigned int h)
{
	for (int i = 0; i < count; i++) {
		if (modes[i].width == w && modes[i].height == h)
			return true;
	}
	return false;
}

int main()
{
	android_rtg_candidate out[android_virtual_rtg_mode_count];

	// Table bounds intact: the fork's landscape table, exactly as ported.
	expect_eq(android_virtual_rtg_mode_count, 20, "table must carry the fork's 20 entries");
	expect_eq(android_virtual_rtg_modes[0][0], 640, "first table width");
	expect_eq(android_virtual_rtg_modes[0][1], 512, "first table height");
	expect_eq(android_virtual_rtg_modes[android_virtual_rtg_mode_count - 1][0], 2560,
		"last table width");
	expect_eq(android_virtual_rtg_modes[android_virtual_rtg_mode_count - 1][1], 1440,
		"last table height");

	// Full set: a real host mode as template (FAKE-path clone semantics) and
	// a generous VRAM budget must yield the whole table in table order, each
	// entry carrying the template's depth and refresh, never its resolution.
	const android_rtg_candidate host_template = { 3840, 2160, 4, 60, "3840x2160" };
	const long long plenty_vram = 1LL << 34;
	int n = android_rtg_build_modes(host_template, plenty_vram,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	expect_eq(n, android_virtual_rtg_mode_count, "generous budget must admit the full table");
	for (int i = 0; i < n; i++) {
		expect_eq(out[i].width, android_virtual_rtg_modes[i][0], "entry width must match table");
		expect_eq(out[i].height, android_virtual_rtg_modes[i][1], "entry height must match table");
		expect_eq(out[i].depth, 4, "cloned entry must carry the template depth");
		expect_eq(out[i].refresh, 60, "cloned entry must carry the template refresh");
		expect_named(out[i], "entry name must be table resolution plus ANDROID suffix");
	}

	// A template with unusual depth/refresh clones those through too.
	const android_rtg_candidate odd_template = { 800, 600, 2, 75, "800x600" };
	n = android_rtg_build_modes(odd_template, plenty_vram,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	expect_eq(n, android_virtual_rtg_mode_count, "odd template still yields the full table");
	expect_eq(out[0].depth, 2, "unusual template depth must be cloned");
	expect_eq(out[0].refresh, 75, "unusual template refresh must be cloned");

	// VRAM guard: usable_vram_bytes already carries the host-mode guard
	// (allocated_size - 256). 1024x768 = 786432 pixels: an exactly-fitting
	// mode stays, the next larger one (1152x864 = 995328) is dropped.
	const android_rtg_candidate zero_template = {};
	n = android_rtg_build_modes(zero_template, 1024LL * 768,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	expect_eq(n, 4, "budget of 786432 pixels admits exactly the four smaller modes");
	expect_true(contains_resolution(out, n, 640, 512), "640x512 must fit 786432");
	expect_true(contains_resolution(out, n, 800, 600), "800x600 must fit 786432");
	expect_true(contains_resolution(out, n, 1024, 600), "1024x600 must fit 786432");
	expect_true(contains_resolution(out, n, 1024, 768), "1024x768 must fit its exact budget");
	expect_true(!contains_resolution(out, n, 1152, 864), "1152x864 must exceed 786432");
	expect_true(!contains_resolution(out, n, 1920, 1080), "1920x1080 must exceed 786432");

	// Zero template (empty host-mode list): still the full filtered set, with
	// a sane depth and refresh filled in — the thin-host scenario (R6/AE6).
	n = android_rtg_build_modes(zero_template, plenty_vram,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	expect_eq(n, android_virtual_rtg_mode_count, "zero template must still yield the full set");
	for (int i = 0; i < n; i++) {
		expect_eq(out[i].depth, android_rtg_default_depth,
			"zero template must fall back to the default depth");
		expect_eq(out[i].refresh, android_rtg_default_refresh,
			"zero template must fall back to the default refresh");
		expect_named(out[i], "zero-template entry must still carry the ANDROID suffix");
	}

	// Cap: the MAX_PICASSO_MODES-equivalent budget caps the produced list,
	// taking the table in order.
	n = android_rtg_build_modes(host_template, plenty_vram, 3, out);
	expect_eq(n, 3, "cap of 3 must produce exactly 3 entries");
	expect_eq(out[0].width, 640, "capped list keeps the first table entry");
	expect_eq(out[2].height, 600, "capped list keeps the third table entry");

	// A zero budget (host modes already filled the array) yields nothing.
	n = android_rtg_build_modes(host_template, plenty_vram, 0, out);
	expect_eq(n, 0, "cap of 0 must produce no entries");

	// Degenerate VRAM: nothing survives a zero/absent RTG allocation.
	n = android_rtg_build_modes(host_template, 0,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	expect_eq(n, 0, "zero VRAM must exclude every table entry");

	if (failures == 0)
		std::cout << "android_rtg_modes: all checks passed\n";
	return failures == 0 ? 0 : 1;
}
