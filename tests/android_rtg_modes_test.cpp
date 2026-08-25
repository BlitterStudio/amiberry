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

// Mirrors the .res.width/.res.height access shape of struct PicassoResolution
// (amiberry_gfx.h) at the addresolutions() call site, without pulling its SDL
// includes into this standalone test.
struct host_mode {
	struct {
		unsigned int width;
		unsigned int height;
	} res;
};

// Mirrors the addresolutions() emission loop (picasso96.cpp): a table
// candidate is emitted only when no existing host mode already carries its
// resolution, so an overlapping mode stays a single emission — the host's.
static int emit_non_existing(const android_rtg_candidate* candidates, const int count,
	const host_mode* existing, const int existing_count, android_rtg_candidate* emitted)
{
	int n = 0;
	for (int k = 0; k < count; k++) {
		if (android_rtg_mode_fits_existing(candidates[k].width, candidates[k].height,
			existing, existing_count))
			continue;
		emitted[n++] = candidates[k];
	}
	return n;
}

int main()
{
	android_rtg_candidate out[android_virtual_rtg_mode_count];

	// Table bounds intact: the fork's landscape table, exactly as ported.
	expect_eq(android_virtual_rtg_mode_count, 26, "table must carry the fork's 20 entries plus the LowRes and sub-VGA additions");
	expect_eq(android_virtual_rtg_modes[0][0], 320, "first table width");
	expect_eq(android_virtual_rtg_modes[0][1], 200, "first table height");
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
	expect_eq(n, 10, "budget of 786432 pixels admits exactly the ten smaller modes");
	expect_true(contains_resolution(out, n, 320, 200), "320x200 must fit 786432");
	expect_true(contains_resolution(out, n, 320, 256), "320x256 must fit 786432");
	expect_true(contains_resolution(out, n, 640, 400), "640x400 must fit 786432");
	expect_true(contains_resolution(out, n, 640, 480), "640x480 must fit 786432");
	expect_true(contains_resolution(out, n, 640, 512), "640x512 must fit 786432");
	expect_true(contains_resolution(out, n, 720, 400), "720x400 must fit 786432");
	expect_true(contains_resolution(out, n, 800, 480), "800x480 must fit 786432");
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
	expect_eq(out[0].width, 320, "capped list keeps the first table entry");
	expect_eq(out[2].height, 400, "capped list keeps the third table entry");

	// A zero budget (host modes already filled the array) yields nothing.
	n = android_rtg_build_modes(host_template, plenty_vram, 0, out);
	expect_eq(n, 0, "cap of 0 must produce no entries");

	// Degenerate VRAM: nothing survives a zero/absent RTG allocation.
	n = android_rtg_build_modes(host_template, 0,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	expect_eq(n, 0, "zero VRAM must exclude every table entry");

	// --- Dedup vs existing host modes (lifted from the call site) ---

	// Overlap: the host already exposes 1920x1080 (plus two more table
	// resolutions), so the table's copies of those three are emitted zero
	// times — each stays a single emission, the host's own entry.
	host_mode host_list[] = {
		{ { 1920, 1080 } },
		{ { 1600, 900 } },
		{ { 1280, 720 } }
	};
	expect_true(android_rtg_mode_fits_existing(1920, 1080, host_list, 3),
		"a resolution the host already has must fit the existing list");
	expect_true(!android_rtg_mode_fits_existing(1920, 1079, host_list, 3),
		"a differing height must not fit the existing list");
	expect_true(!android_rtg_mode_fits_existing(1919, 1080, host_list, 3),
		"a differing width must not fit the existing list");
	expect_true(!android_rtg_mode_fits_existing(640, 512, host_list, 0),
		"an empty host list must fit nothing");
	n = android_rtg_build_modes(host_template, plenty_vram,
		static_cast<int>(android_virtual_rtg_mode_count), out);
	android_rtg_candidate emitted[android_virtual_rtg_mode_count];
	int emitted_n = emit_non_existing(out, n, host_list, 3, emitted);
	expect_eq(emitted_n, n - 3,
		"each overlapping host mode must suppress exactly its table twin");
	expect_true(!contains_resolution(emitted, emitted_n, 1920, 1080),
		"table's 1920x1080 must be emitted zero times when the host already has it");
	expect_true(!contains_resolution(emitted, emitted_n, 1600, 900),
		"table's 1600x900 must be emitted zero times when the host already has it");
	expect_true(!contains_resolution(emitted, emitted_n, 1280, 720),
		"table's 1280x720 must be emitted zero times when the host already has it");
	expect_true(contains_resolution(emitted, emitted_n, 640, 512),
		"non-overlapping table entries must pass through");
	expect_true(contains_resolution(emitted, emitted_n, 2560, 1440),
		"the largest non-overlapping entry must pass through");

	// Non-overlap: host modes absent from the table suppress nothing.
	host_mode odd_host_list[] = {
		{ { 3840, 2160 } },
		{ { 720, 480 } }
	};
	emitted_n = emit_non_existing(out, n, odd_host_list, 2, emitted);
	expect_eq(emitted_n, n, "non-overlapping host modes must not suppress any table entry");

	// --- Remaining-slot budget (lifted from the call site) ---

	// max(0, MAX_PICASSO_MODES - 1 - cnt) with MAX_PICASSO_MODES = 300.
	expect_eq(android_rtg_remaining_mode_budget(300, 0), 299,
		"empty host list leaves capacity minus the reserved headroom slot");
	expect_eq(android_rtg_remaining_mode_budget(300, 100), 199,
		"a partly filled list spends its slots one-for-one");
	expect_eq(android_rtg_remaining_mode_budget(300, 299), 0,
		"boundary: filling all but the headroom slot yields exactly zero");
	expect_eq(android_rtg_remaining_mode_budget(300, 300), 0,
		"a negative remainder must clamp to zero");
	expect_eq(android_rtg_remaining_mode_budget(300, 400), 0,
		"a far negative remainder must clamp to zero");

	// Wiring: the boundary budget feeds android_rtg_build_modes unchanged —
	// the "host modes already filled the array" path at the call site.
	n = android_rtg_build_modes(host_template, plenty_vram,
		android_rtg_remaining_mode_budget(300, 299), out);
	expect_eq(n, 0, "zero remaining budget must produce no candidates");

	if (failures == 0)
		std::cout << "android_rtg_modes: all checks passed\n";
	return failures == 0 ? 0 : 1;
}
