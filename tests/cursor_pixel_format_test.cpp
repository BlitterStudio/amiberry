#include <cstdint>
#include <cstring>
#include <iostream>

typedef std::uint8_t uae_u8;
typedef std::uint16_t uae_u16;
typedef std::uint32_t uae_u32;
typedef std::uint64_t uae_u64;

#include "amiberry_cursor.h"
#include "amiberry_input_helpers.h"

static int failures;

static void expect_eq(uae_u32 actual, uae_u32 expected, const char *message)
{
	if (actual != expected) {
		std::cerr << message << ": expected 0x" << std::hex << expected
			<< ", got 0x" << actual << std::dec << '\n';
		failures++;
	}
}

static void expect_int_eq(int actual, int expected, const char *message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected
			<< ", got " << actual << '\n';
		failures++;
	}
}

static void expect_true(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

static void test_rgb12_expands_to_rgb24()
{
	expect_eq(amiberry_cursor_rgb12_to_rgb24(0x000), 0x000000, "black must remain black");
	expect_eq(amiberry_cursor_rgb12_to_rgb24(0x123), 0x112233, "12-bit RGB nibbles must duplicate");
	expect_eq(amiberry_cursor_rgb12_to_rgb24(0xabc), 0xaabbcc, "12-bit RGB must preserve channel order");
	expect_eq(amiberry_cursor_rgb12_to_rgb24(0xfff), 0xffffff, "white must expand fully");
}

static void test_rgba32_cursor_pixels_are_opaque()
{
	const uae_u32 pixel = amiberry_cursor_rgba32_from_rgb24(0x112233);
	unsigned char bytes[4] = {};
	std::memcpy(bytes, &pixel, sizeof bytes);

	expect_true(bytes[0] == 0x11, "RGBA32 cursor byte 0 must be red");
	expect_true(bytes[1] == 0x22, "RGBA32 cursor byte 1 must be green");
	expect_true(bytes[2] == 0x33, "RGBA32 cursor byte 2 must be blue");
	expect_true(bytes[3] == 0xff, "RGBA32 cursor byte 3 must be opaque alpha");
}

static void test_rgba32_cursor_pixels_use_raw_rgb_order()
{
	uae_u32 red = amiberry_cursor_rgba32_from_rgb24(0xff0000);
	uae_u32 blue = amiberry_cursor_rgba32_from_rgb24(0x0000ff);
	unsigned char red_bytes[4] = {};
	unsigned char blue_bytes[4] = {};
	std::memcpy(red_bytes, &red, sizeof red_bytes);
	std::memcpy(blue_bytes, &blue, sizeof blue_bytes);

	expect_true(red_bytes[0] == 0xff && red_bytes[1] == 0x00 && red_bytes[2] == 0x00 && red_bytes[3] == 0xff,
		"pure red must not become blue in RGBA32 cursor memory");
	expect_true(blue_bytes[0] == 0x00 && blue_bytes[1] == 0x00 && blue_bytes[2] == 0xff && blue_bytes[3] == 0xff,
		"pure blue must not become red in RGBA32 cursor memory");
}

static void test_rtg_cursor_hotspot_uses_pointer_offset()
{
	expect_int_eq(amiberry_cursor_hotspot_from_offset(25, 51), 25,
		"centered P96 cursor x offset must become the SDL hotspot");
	expect_int_eq(amiberry_cursor_hotspot_from_offset(30, 61), 30,
		"centered P96 cursor y offset must become the SDL hotspot");
	expect_int_eq(amiberry_cursor_hotspot_from_offset(-4, 16), 0,
		"negative P96 cursor offset must clamp to the first pixel");
	expect_int_eq(amiberry_cursor_hotspot_from_offset(80, 16), 15,
		"oversized P96 cursor offset must clamp to the last pixel");
}

static void test_host_only_forces_separate_rtg_sprite()
{
	const int host_only = 2;

	const bool enabled = amiberry_cursor_host_only_enabled(1, host_only, host_only, true);
	expect_true(enabled, "host-only cursor mode must be detected from virtual mouse and cursor selection");
	expect_true(!amiberry_cursor_host_only_enabled(0, host_only, host_only, true),
		"host-only cursor mode must require virtual mouse mode");
	expect_true(!amiberry_cursor_host_only_enabled(1, 0, host_only, true),
		"host-only cursor mode must require host-only cursor selection");
	expect_true(!amiberry_cursor_host_only_enabled(1, host_only, host_only, false),
		"host-only cursor mode must require a visible host cursor path");

	expect_true(amiberry_cursor_rtg_needs_separate_sprite(false, enabled),
		"RTG Host Only must force separate P96 sprite handling");
	expect_true(amiberry_cursor_rtg_needs_separate_sprite(true, false),
		"explicit RTG hardware sprite emulation must still request separate sprite handling");
}

static void test_rtg_cursor_keeps_softsprite_fallback_disabled()
{
	expect_eq(amiberry_cursor_rtg_softsprite_fallback_mask(), 0,
		"RTG cursor must not force P96 software pointer fallback in 32-bit modes");
}

static void test_native_axis_clamp_uses_native_extent()
{
	bool out_of_bounds = true;
	int value = amiberry_input_clamp_native_axis(239, 240, &out_of_bounds);
	expect_eq(value, 239, "last native pixel must stay in range");
	expect_true(!out_of_bounds, "last native pixel must not be treated as out of bounds");

	out_of_bounds = false;
	value = amiberry_input_clamp_native_axis(240, 240, &out_of_bounds);
	expect_eq(value, 239, "one-past-native pixel must clamp to the last pixel");
	expect_true(out_of_bounds, "one-past-native pixel must be reported out of bounds");

	out_of_bounds = false;
	value = amiberry_input_clamp_native_axis(-1, 240, &out_of_bounds);
	expect_eq(value, 0, "negative native pixel must clamp to zero");
	expect_true(out_of_bounds, "negative native pixel must be reported out of bounds");
}

static void test_mousehack_hotspot_matches_pointer_offset()
{
	int hotspot_x = -1;
	int hotspot_y = -1;

	amiberry_input_mousehack_cursor_hotspot(-1, -2, 16, 16, &hotspot_x, &hotspot_y);
	expect_eq(hotspot_x, 0, "default x pointer offset must keep top-left hotspot");
	expect_eq(hotspot_y, 0, "default y pointer offset must keep top-left hotspot");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(-1, 16, 1), 0,
		"default x pointer offset must not leave residual compensation");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(-2, 16, 2), 0,
		"default y pointer offset must not leave residual compensation");

	amiberry_input_mousehack_cursor_hotspot(-26, -32, 51, 61, &hotspot_x, &hotspot_y);
	expect_eq(hotspot_x, 25, "centered cross cursor x offset must become centered host hotspot");
	expect_eq(hotspot_y, 30, "centered cross cursor y offset must become centered host hotspot");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(-26, 51, 1), 0,
		"centered cross cursor x offset must be fully represented by host hotspot");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(-32, 61, 2), 0,
		"centered cross cursor y offset must be fully represented by host hotspot");

	amiberry_input_mousehack_cursor_hotspot(4, 4, 16, 16, &hotspot_x, &hotspot_y);
	expect_eq(hotspot_x, 0, "positive x pointer offset must clamp hotspot to left edge");
	expect_eq(hotspot_y, 0, "positive y pointer offset must clamp hotspot to top edge");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(4, 16, 1), 5,
		"positive x pointer offset must leave residual compensation after hotspot clamping");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(4, 16, 2), 6,
		"positive y pointer offset must leave residual compensation after hotspot clamping");

	amiberry_input_mousehack_cursor_hotspot(-80, -80, 16, 16, &hotspot_x, &hotspot_y);
	expect_eq(hotspot_x, 15, "oversized x pointer offset must clamp hotspot to right edge");
	expect_eq(hotspot_y, 15, "oversized y pointer offset must clamp hotspot to bottom edge");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(-80, 16, 1), -64,
		"oversized x pointer offset must leave negative residual compensation after hotspot clamping");
	expect_int_eq(amiberry_input_mousehack_hotspot_residual_axis(-80, 16, 2), -63,
		"oversized y pointer offset must leave negative residual compensation after hotspot clamping");
}

static void test_native_sprite_position_decoding()
{
	expect_int_eq(amiberry_input_native_sprite_x(0x1234, 0x0000, false), 0xd0,
		"native sprite x must decode the horizontal position byte");
	expect_int_eq(amiberry_input_native_sprite_x(0x1234, 0x0001, false), 0xd2,
		"native sprite x must include the low control bit");
	expect_int_eq(amiberry_input_native_sprite_x(0x1234, 0x0011, true), 0xd3,
		"ECS/AGA sprite x must include the extended low bit");

	expect_int_eq(amiberry_input_native_sprite_y(0x1200, 0x0000, false), 0x24,
		"native sprite y must decode the vertical position byte");
	expect_int_eq(amiberry_input_native_sprite_y(0x1200, 0x0004, false), 0x224,
		"native sprite y must include the first high control bit");
	expect_int_eq(amiberry_input_native_sprite_y(0x1200, 0x0044, true), 0x624,
		"ECS sprite y must include the extended high bit");
}

static void test_native_cursor_height_requires_sscan2()
{
	expect_int_eq(amiberry_input_native_cursor_height(27, false, false), 27,
		"normal native cursor height must remain unchanged");
	expect_int_eq(amiberry_input_native_cursor_height(27, true, false), 27,
		"vertical position bit 7 must not squash the cursor when SSCAN2 is disabled");
	expect_int_eq(amiberry_input_native_cursor_height(27, true, true), 13,
		"active SSCAN2 must halve the extracted cursor data height");
}

static void test_native_mousehack_compensates_cropped_screen_origin()
{
	expect_int_eq(amiberry_input_native_mousehack_origin_compensation(true, 258, 204), 54,
		"native mousehack must compensate the cropped horizontal screen origin");
	expect_int_eq(amiberry_input_native_mousehack_origin_compensation(true, 88, 90), -2,
		"native mousehack must compensate the cropped vertical screen origin");
	expect_int_eq(amiberry_input_native_mousehack_origin_compensation(true, 128, 128), 0,
		"matching native and guest origins must not move the pointer");
	expect_int_eq(amiberry_input_native_mousehack_origin_compensation(false, 258, 0), 0,
		"uncropped native input must not apply guest screen offset compensation");

	expect_int_eq(amiberry_input_native_mousehack_uncropped_lowres_x_compensation(false, true), -2,
		"uncropped LowRes input must move left by one LowRes pixel in hires coordinate units");
	expect_int_eq(amiberry_input_native_mousehack_uncropped_lowres_x_compensation(true, true), 0,
		"Auto Crop must retain its source-origin-based LowRes alignment");
	expect_int_eq(amiberry_input_native_mousehack_uncropped_lowres_x_compensation(false, false), 0,
		"uncropped higher-resolution input must not receive the LowRes correction");
}

static void test_native_mousehack_normalizes_interlace_field_origin()
{
	expect_int_eq(amiberry_input_native_interlace_origin(38, true, false, 2), 38,
		"short interlace field must retain the native source origin");
	expect_int_eq(amiberry_input_native_interlace_origin(40, true, true, 2), 38,
		"long interlace field must normalize to the short-field source origin");
	expect_int_eq(amiberry_input_native_interlace_origin(40, false, true, 2), 40,
		"non-interlaced display must ignore the field flag");
}

static void test_native_cursor_hotspot_learning_accepts_moving_samples()
{
	amiberry_input_cursor_hotspot_tracker tracker;
	int hotspot_x = -1;
	int hotspot_y = -1;

	expect_true(!amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		300, 220, 275, 190, 51, 61, 3, &hotspot_x, &hotspot_y),
		"first hotspot sample must not be trusted immediately");
	expect_true(!amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		304, 224, 279, 194, 51, 61, 3, &hotspot_x, &hotspot_y),
		"second matching hotspot sample must still wait for confirmation");
	expect_true(amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		308, 228, 283, 198, 51, 61, 3, &hotspot_x, &hotspot_y),
		"matching pointer and sprite movement must allow the hotspot to be learned");
	expect_int_eq(hotspot_x, 25, "learned cross cursor hotspot x must use the live sprite displacement");
	expect_int_eq(hotspot_y, 30, "learned cross cursor hotspot y must use the live sprite displacement");
}

static void test_native_cursor_hotspot_uses_hires_y_units()
{
	amiberry_input_cursor_hotspot_tracker tracker;
	int hotspot_x = -1;
	int hotspot_y = -1;

	for (int i = 0; i < 3; i++) {
		amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
			400 + i * 2, 300 + i * 2, 384 + i * 2, 286 + i * 2,
			32, 54, 3, &hotspot_x, &hotspot_y);
	}
	expect_int_eq(hotspot_x, 16,
		"doubled native cursor x hotspot must stay in hires pixel units");
	expect_int_eq(hotspot_y, 14,
		"doubled native cursor y hotspot must stay in hires pixel units");
}

static void test_native_cursor_hotspot_cache_keeps_interleaved_sizes()
{
	amiberry_input_cursor_hotspot_tracker_cache cache;
	int hotspot_x = -1;
	int hotspot_y = -1;
	const uae_u64 tall_signature = 0x12345678;
	const uae_u64 short_signature = 0x87654321;

	for (int i = 0; i < 3; i++) {
		auto* tall = amiberry_input_cursor_hotspot_tracker_cache_acquire(
			&cache, 32, 54, tall_signature);
		amiberry_input_cursor_hotspot_tracker_sample(tall, true,
			400 + i * 2, 300 + i * 2, 384 + i * 2, 286 + i * 2,
			32, 54, 3, &hotspot_x, &hotspot_y);

		auto* short_cursor = amiberry_input_cursor_hotspot_tracker_cache_acquire(
			&cache, 32, 26, short_signature);
		amiberry_input_cursor_hotspot_tracker_sample(short_cursor, true,
			500 + i * 2, 200 + i * 2, 484 + i * 2, 188 + i * 2,
			32, 26, 3, &hotspot_x, &hotspot_y);
	}

	auto* tall = amiberry_input_cursor_hotspot_tracker_cache_acquire(
		&cache, 32, 54, tall_signature);
	auto* short_cursor = amiberry_input_cursor_hotspot_tracker_cache_acquire(
		&cache, 32, 26, short_signature);
	expect_true(tall->learned, "interleaved tall cursor must retain its hotspot samples");
	expect_int_eq(tall->hotspot_x, 16, "interleaved tall cursor hotspot x must be retained");
	expect_int_eq(tall->hotspot_y, 14, "interleaved tall cursor hotspot y must be retained");
	expect_true(short_cursor->learned, "interleaved short cursor must retain its hotspot samples");
	expect_int_eq(short_cursor->hotspot_x, 16, "interleaved short cursor hotspot x must be retained");
	expect_int_eq(short_cursor->hotspot_y, 12, "interleaved short cursor hotspot y must be retained");
}

static void test_native_cursor_hotspot_cache_separates_same_size_bitmaps()
{
	amiberry_input_cursor_hotspot_tracker_cache cache;
	int hotspot_x = -1;
	int hotspot_y = -1;
	const uae_u64 arrow_signature = 0x11111111;
	const uae_u64 crosshair_signature = 0x22222222;

	for (int i = 0; i < 3; i++) {
		auto* arrow = amiberry_input_cursor_hotspot_tracker_cache_acquire(
			&cache, 32, 32, arrow_signature);
		amiberry_input_cursor_hotspot_tracker_sample(arrow, true,
			200 + i, 160 + i, 199 + i, 158 + i,
			32, 32, 3, &hotspot_x, &hotspot_y);
	}

	auto* crosshair = amiberry_input_cursor_hotspot_tracker_cache_acquire(
		&cache, 32, 32, crosshair_signature);
	expect_true(!crosshair->learned,
		"same-sized cursor bitmap must not inherit a previously learned hotspot");

	auto* arrow = amiberry_input_cursor_hotspot_tracker_cache_acquire(
		&cache, 32, 32, arrow_signature);
	expect_true(arrow->learned, "original same-sized bitmap must retain its own hotspot");
	expect_int_eq(arrow->hotspot_x, 1, "original same-sized bitmap hotspot x must be retained");
	expect_int_eq(arrow->hotspot_y, 2, "original same-sized bitmap hotspot y must be retained");
}

static void test_native_cursor_bitmap_signature_includes_pixels_and_colors()
{
	const uae_u8 arrow[] = { 0, 1, 1, 0 };
	const uae_u8 crosshair[] = { 1, 0, 0, 1 };
	const uae_u32 colors[] = { 0, 0xffffff, 0x555555, 0xaaaaaa };
	const uae_u32 changed_colors[] = { 0, 0xff0000, 0x555555, 0xaaaaaa };

	const uae_u64 arrow_signature = amiberry_input_cursor_bitmap_signature(
		arrow, sizeof arrow, colors, 4);
	expect_true(arrow_signature != amiberry_input_cursor_bitmap_signature(
		crosshair, sizeof crosshair, colors, 4),
		"same-sized cursor bitmaps must have distinct cache signatures");
	expect_true(arrow_signature != amiberry_input_cursor_bitmap_signature(
		arrow, sizeof arrow, changed_colors, 4),
		"cursor color changes must have distinct cache signatures");
	expect_true(amiberry_input_cursor_hotspot_signature(arrow_signature, 0, 0)
		!= amiberry_input_cursor_hotspot_signature(arrow_signature, 1, 0),
		"same P96 bitmap with a different declared hotspot must have a distinct cache signature");
}

static void test_native_cursor_hotspot_learning_keeps_last_valid_result()
{
	amiberry_input_cursor_hotspot_tracker tracker;
	int hotspot_x = -1;
	int hotspot_y = -1;

	for (int i = 0; i < 3; i++) {
		amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
			200, 160, 199, 158, 16, 16, 3, &hotspot_x, &hotspot_y);
	}
	expect_int_eq(hotspot_x, 1, "tracker must learn a valid arrow hotspot x");
	expect_int_eq(hotspot_y, 2, "tracker must learn a valid arrow hotspot y");

	expect_true(amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		200, 160, 80, 40, 16, 16, 3, &hotspot_x, &hotspot_y),
		"an unrelated sprite position must retain the last learned hotspot");
	expect_int_eq(hotspot_x, 1, "invalid candidate must not replace learned hotspot x");
	expect_int_eq(hotspot_y, 2, "invalid candidate must not replace learned hotspot y");

	amiberry_input_cursor_hotspot_tracker_reset(&tracker);
	expect_true(!tracker.learned, "reset must discard a hotspot learned for old cursor dimensions");
}

static void test_p96_cursor_swap_waits_for_stable_hotspot()
{
	amiberry_input_cursor_hotspot_tracker tracker;
	int hotspot_x = -1;
	int hotspot_y = -1;

	amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		300, 220, 275, 190, 51, 61, 3, &hotspot_x, &hotspot_y);
	expect_true(amiberry_input_cursor_hotspot_should_defer_swap(true, true, &tracker),
		"changed P96 cursor must keep the previous cursor while its hotspot is pending");

	amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		304, 224, 279, 194, 51, 61, 3, &hotspot_x, &hotspot_y);
	expect_true(amiberry_input_cursor_hotspot_should_defer_swap(true, true, &tracker),
		"second P96 hotspot sample must still defer the cursor swap");

	amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		308, 228, 283, 198, 51, 61, 3, &hotspot_x, &hotspot_y);
	expect_true(!amiberry_input_cursor_hotspot_should_defer_swap(true, true, &tracker),
		"learned P96 hotspot must allow the new cursor to replace the previous cursor");
	expect_true(!amiberry_input_cursor_hotspot_should_defer_swap(true, false, &tracker),
		"initial P96 cursor must not wait for a previous cursor that does not exist");

	amiberry_input_cursor_hotspot_tracker_reset(&tracker);
	amiberry_input_cursor_hotspot_tracker_sample(&tracker, true,
		300, 220, 100, 40, 51, 61, 3, &hotspot_x, &hotspot_y);
	expect_true(!amiberry_input_cursor_hotspot_should_defer_swap(true, true, &tracker),
		"invalid P96 displacement must not leave the previous cursor stuck indefinitely");
}

static void test_native_crosshair_hotspot_snaps_to_bitmap_intersection()
{
	int hotspot_x = -1;
	int hotspot_y = -1;
	expect_true(amiberry_input_cursor_snap_hotspot_to_dense_crossing(
		17, 14, 14, 14, 3, &hotspot_x, &hotspot_y),
		"native hotspot near a dense crossing must snap to the visual intersection");
	expect_int_eq(hotspot_x, 14, "crosshair hotspot x must use the decoded intersection");
	expect_int_eq(hotspot_y, 14, "crosshair hotspot y must use the decoded intersection");

	hotspot_x = 1;
	hotspot_y = 0;
	expect_true(!amiberry_input_cursor_snap_hotspot_to_dense_crossing(
		1, 0, 8, 8, 3, &hotspot_x, &hotspot_y),
		"arrow hotspot far from a dense crossing must not be changed");
	expect_int_eq(hotspot_x, 1, "unrelated arrow hotspot x must remain unchanged");
	expect_int_eq(hotspot_y, 0, "unrelated arrow hotspot y must remain unchanged");
}

int main()
{
	test_rgb12_expands_to_rgb24();
	test_rgba32_cursor_pixels_are_opaque();
	test_rgba32_cursor_pixels_use_raw_rgb_order();
	test_rtg_cursor_hotspot_uses_pointer_offset();
	test_host_only_forces_separate_rtg_sprite();
	test_rtg_cursor_keeps_softsprite_fallback_disabled();
	test_native_axis_clamp_uses_native_extent();
	test_mousehack_hotspot_matches_pointer_offset();
	test_native_sprite_position_decoding();
	test_native_cursor_height_requires_sscan2();
	test_native_mousehack_compensates_cropped_screen_origin();
	test_native_mousehack_normalizes_interlace_field_origin();
	test_native_cursor_hotspot_learning_accepts_moving_samples();
	test_native_cursor_hotspot_uses_hires_y_units();
	test_native_cursor_hotspot_cache_keeps_interleaved_sizes();
	test_native_cursor_hotspot_cache_separates_same_size_bitmaps();
	test_native_cursor_bitmap_signature_includes_pixels_and_colors();
	test_native_cursor_hotspot_learning_keeps_last_valid_result();
	test_p96_cursor_swap_waits_for_stable_hotspot();
	test_native_crosshair_hotspot_snaps_to_bitmap_intersection();
	return failures == 0 ? 0 : 1;
}
