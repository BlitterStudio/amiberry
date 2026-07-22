#include <cstdint>
#include <iostream>
#include <vector>

#include "amiberry_autocrop_helpers.h"

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

static AmiberryAutoCropPixelBuffer make_buffer(const std::vector<uint32_t>& pixels,
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

static void add_lower_content(std::vector<uint32_t>& pixels, const int width)
{
	for (int y = 26; y < 28; y++) {
		for (int x = 8; x < 24; x++) {
			pixels[y * width + x] = 0x0000ff00u;
		}
	}
}

static void test_expands_to_connected_visible_content()
{
	constexpr int width = 40;
	constexpr int height = 40;
	std::vector<uint32_t> pixels(width * height, 0);
	add_lower_content(pixels, width);

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 6, 8, 20, 12 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(changed, "Crop should expand to connected content below it");
	expect_eq(crop.x, 6, "Crop x should stay anchored");
	expect_eq(crop.y, 8, "Crop y should stay anchored");
	expect_eq(crop.w, 20, "Crop width should not change for bottom-only content");
	expect_eq(crop.h, 20, "Crop bottom should expand to include lower content");
}

static void test_ignores_distant_speck_when_content_expands()
{
	constexpr int width = 40;
	constexpr int height = 40;
	std::vector<uint32_t> pixels(width * height, 0);
	add_lower_content(pixels, width);
	pixels[39 * width + 39] = 0x00ffffffu;

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 6, 8, 20, 12 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(changed, "Connected lower content should still expand the crop");
	expect_eq(crop.x, 6, "A distant speck must not move the crop left edge");
	expect_eq(crop.y, 8, "A distant speck must not move the crop top edge");
	expect_eq(crop.w, 20, "A distant speck must not widen the crop");
	expect_eq(crop.h, 20, "A distant speck must not extend the crop bottom");
}

static void test_ignores_scattered_outside_pixels()
{
	constexpr int width = 40;
	constexpr int height = 40;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int i = 0; i < 16; i++) {
		pixels[(22 + i) * width + (i * 2)] = 0x00ffffffu;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 6, 8, 20, 12 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(!changed, "Scattered outside pixels must not expand the crop");
	expect_eq(crop.w, 20, "Scattered pixels must not change crop width");
	expect_eq(crop.h, 12, "Scattered pixels must not change crop height");
}

int main()
{
	test_expands_to_connected_visible_content();
	test_ignores_distant_speck_when_content_expands();
	test_ignores_scattered_outside_pixels();
	return failures == 0 ? 0 : 1;
}
