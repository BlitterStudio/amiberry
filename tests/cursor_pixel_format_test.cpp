#include <cstdint>
#include <cstring>
#include <iostream>

typedef std::uint16_t uae_u16;
typedef std::uint32_t uae_u32;

#include "amiberry_cursor.h"

static int failures;

static void expect_eq(uae_u32 actual, uae_u32 expected, const char *message)
{
	if (actual != expected) {
		std::cerr << message << ": expected 0x" << std::hex << expected
			<< ", got 0x" << actual << std::dec << '\n';
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

static void test_host_only_forces_separate_rtg_sprite()
{
	const int host_only = 2;

	const bool enabled = amiberry_cursor_host_only_enabled(1, host_only, host_only);
	expect_true(enabled, "host-only cursor mode must be detected when virtual mouse is active");
	expect_true(!amiberry_cursor_host_only_enabled(0, host_only, host_only),
		"host-only cursor mode must require virtual mouse mode");
	expect_true(!amiberry_cursor_host_only_enabled(1, 0, host_only),
		"host-only cursor mode must require host-only cursor selection");

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

int main()
{
	test_rgb12_expands_to_rgb24();
	test_rgba32_cursor_pixels_are_opaque();
	test_rgba32_cursor_pixels_use_raw_rgb_order();
	test_host_only_forces_separate_rtg_sprite();
	test_rtg_cursor_keeps_softsprite_fallback_disabled();
	return failures == 0 ? 0 : 1;
}
