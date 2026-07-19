#include <cstdint>
#include <iostream>
#include <vector>

#include "libretro/libretro_crop_helpers.h"

static int failures;

static void expect_true(const bool actual, const char* message)
{
	if (!actual) {
		std::cerr << message << '\n';
		failures++;
	}
}

template<typename T>
static void expect_eq(const T actual, const T expected, const char* message)
{
	if (actual != expected) {
		std::cerr << message << ": expected " << expected << ", got " << actual << '\n';
		failures++;
	}
}

static LibretroCropPixelBuffer make_buffer(const std::vector<uint32_t>& pixels,
	const int width, const int height)
{
	return {
		reinterpret_cast<const uint8_t*>(pixels.data()),
		width,
		height,
		width * static_cast<int>(sizeof(uint32_t)),
		static_cast<int>(sizeof(uint32_t)),
		0x00ffffffu
	};
}

static void test_expands_to_visible_pixels_below_crop()
{
	constexpr int width = 32;
	constexpr int height = 32;
	std::vector<uint32_t> pixels(width * height, 0);

	for (int y = 8; y < 20; y++) {
		for (int x = 6; x < 26; x++)
			pixels[y * width + x] = 0x00ffffffu;
	}
	for (int y = 26; y < 28; y++) {
		for (int x = 8; x < 24; x++)
			pixels[y * width + x] = 0x0000ff00u;
	}

	auto buffer = make_buffer(pixels, width, height);
	LibretroCropRect crop{ 6, 8, 20, 12 };

	const bool changed = libretro_crop_expand_to_visible_content(buffer, 0, 4, crop);

	expect_true(changed, "Crop should expand when visible content exists below it");
	expect_eq(crop.x, 6, "Crop x should stay anchored");
	expect_eq(crop.y, 8, "Crop y should stay anchored");
	expect_eq(crop.w, 20, "Crop width should not change for bottom-only content");
	expect_eq(crop.h, 20, "Crop bottom should expand to include lower content");
}

static void test_ignores_isolated_outside_pixels()
{
	constexpr int width = 32;
	constexpr int height = 32;
	std::vector<uint32_t> pixels(width * height, 0);

	for (int y = 8; y < 20; y++) {
		for (int x = 6; x < 26; x++)
			pixels[y * width + x] = 0x00ffffffu;
	}
	pixels[27 * width + 16] = 0x00ffffffu;

	auto buffer = make_buffer(pixels, width, height);
	LibretroCropRect crop{ 6, 8, 20, 12 };

	const bool changed = libretro_crop_expand_to_visible_content(buffer, 0, 4, crop);

	expect_true(!changed, "Crop should ignore isolated outside pixels");
	expect_eq(crop.h, 12, "Crop height should remain unchanged for isolated pixels");
}

static void test_fixed_crop_shifts_to_keep_content_visible()
{
	LibretroCropRect content{ 8, 3, 16, 18 };
	LibretroCropRect crop{ 6, 8, 20, 20 };

	const bool changed = libretro_crop_fit_rect_to_visible_content(40, 40, content, crop);

	expect_true(changed, "Fixed crop should shift when visible content is above it");
	expect_eq(crop.x, 6, "Fixed crop x should not move unnecessarily");
	expect_eq(crop.y, 3, "Fixed crop y should move up to include visible content");
	expect_eq(crop.w, 20, "Fixed crop width should remain fixed");
	expect_eq(crop.h, 20, "Fixed crop height should remain fixed");
}

static void test_libretro_stabilizer_uses_short_mode_change_delays()
{
	LibretroCropRect current{ 8, 8, 320, 200 };
	LibretroCropRect expanded{ 6, 6, 324, 204 };
	LibretroCropRect tighter{ 8, 8, 320, 180 };

	expect_eq(libretro_crop_stable_frames_required(current, expanded), 6,
		"Expanding crops should settle quickly");
	expect_eq(libretro_crop_stable_frames_required(current, tighter), 18,
		"Shrinking crops should not wait a full second");
}

static void test_rp9_manifest_clip_owns_automatic_crop()
{
	expect_true(!libretro_should_queue_auto_crop_options(true, true),
		"RP9 automatic crop options must wait until after the manifest is parsed");
	expect_true(libretro_should_queue_auto_crop_options(true, false),
		"Non-RP9 content must keep automatic crop startup options");
	expect_true(!libretro_should_queue_auto_crop_options(false, true),
		"Disabled or fixed crop modes must not queue automatic crop options");

	expect_true(libretro_should_preserve_rp9_clip(true, true),
		"Automatic crop must preserve an RP9 manifest clip");
	expect_true(!libretro_should_preserve_rp9_clip(true, false),
		"RP9 packages without a clip must use normal automatic crop");
	expect_true(!libretro_should_preserve_rp9_clip(false, true),
		"Explicit disabled or fixed crop modes must override an RP9 clip");
}

int main()
{
	test_expands_to_visible_pixels_below_crop();
	test_ignores_isolated_outside_pixels();
	test_fixed_crop_shifts_to_keep_content_visible();
	test_libretro_stabilizer_uses_short_mode_change_delays();
	test_rp9_manifest_clip_owns_automatic_crop();

	return failures == 0 ? 0 : 1;
}
