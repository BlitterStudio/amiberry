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

static void add_colored_border(std::vector<uint32_t>& pixels, const int width,
	const int height, const uint32_t color)
{
	for (int y = 2; y < height - 2; y++) {
		for (int x = 2; x < width - 2; x++) {
			pixels[y * width + x] = color;
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

static void test_keeps_crop_inside_non_black_border()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00aaaaaau;
	std::vector<uint32_t> pixels(width * height, 0);
	add_colored_border(pixels, width, height, border_color);

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(!changed, "A non-black border must not expand the hardware crop");
	expect_true(state.border_valid, "A consistent non-black border should be detected");
	expect_eq(state.border_rgb, border_color, "Detected border color should match the perimeter");
	expect_eq(crop.x, 8, "Outer black surface edge must not move the crop left");
	expect_eq(crop.y, 8, "Outer black surface edge must not move the crop top");
	expect_eq(crop.w, 24, "Outer black surface edge must not widen the crop");
	expect_eq(crop.h, 20, "Outer black surface edge must not increase crop height");
}

static void test_expands_past_non_black_border_for_real_content()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00aaaaaau;
	std::vector<uint32_t> pixels(width * height, 0);
	add_colored_border(pixels, width, height, border_color);
	for (int y = 30; y < 32; y++) {
		for (int x = 12; x < 28; x++) {
			pixels[y * width + x] = 0x0000ff00u;
		}
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(changed, "Connected content should expand through a non-black border");
	expect_eq(crop.x, 8, "Bottom-only content should keep the crop x origin");
	expect_eq(crop.y, 8, "Bottom-only content should keep the crop y origin");
	expect_eq(crop.w, 24, "Bottom-only content should keep the crop width");
	expect_eq(crop.h, 24, "Crop should expand to the connected content bottom");
}

static void test_preserves_content_reaching_surface_edge()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00aaaaaau;
	std::vector<uint32_t> pixels(width * height, 0);
	add_colored_border(pixels, width, height, border_color);
	for (int y = 30; y < height; y++) {
		for (int x = 12; x < 28; x++) {
			pixels[y * width + x] = 0x0000ff00u;
		}
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(changed, "Content reaching the surface edge should expand the crop");
	expect_eq(crop.x, 8, "Edge-reaching bottom content should keep the crop x origin");
	expect_eq(crop.y, 8, "Edge-reaching bottom content should keep the crop y origin");
	expect_eq(crop.w, 24, "Edge-reaching bottom content should keep the crop width");
	expect_eq(crop.h, 32, "Crop should include content through the surface bottom");
}

static void test_chooses_background_outside_origin_anchored_crop()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00aaaaaau;
	std::vector<uint32_t> pixels(width * height, 0);
	add_colored_border(pixels, width, height, border_color);
	pixels[0] = 0x00ff0000u;

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 0, 0, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, crop, state);

	expect_true(!changed, "An in-crop origin pixel must not become the surface background");
	expect_eq(crop.x, 0, "Origin-anchored crop must keep its x origin");
	expect_eq(crop.y, 0, "Origin-anchored crop must keep its y origin");
	expect_eq(crop.w, 24, "Cleared right edge must not widen an origin-anchored crop");
	expect_eq(crop.h, 20, "Cleared bottom edge must not increase crop height");
}

static void test_border_state_changes_reset_preserved_crop()
{
	constexpr uint32_t first_color = 0x00112233u;
	constexpr uint32_t second_color = 0x00445566u;

	expect_true(amiberry_auto_crop_border_state_changed(first_color, true,
		first_color, false), "Losing a valid border must reset preserved crop bounds");
	expect_true(amiberry_auto_crop_border_state_changed(first_color, false,
		first_color, true), "Detecting a valid border must reset preserved crop bounds");
	expect_true(amiberry_auto_crop_border_state_changed(first_color, true,
		second_color, true), "Changing border color must reset preserved crop bounds");
	expect_true(!amiberry_auto_crop_border_state_changed(first_color, true,
		first_color, true), "An unchanged valid border must preserve crop bounds");
	expect_true(!amiberry_auto_crop_border_state_changed(first_color, false,
		second_color, false), "Ambiguous border samples must ignore stale colors");
}

int main()
{
	test_expands_to_connected_visible_content();
	test_ignores_distant_speck_when_content_expands();
	test_ignores_scattered_outside_pixels();
	test_keeps_crop_inside_non_black_border();
	test_expands_past_non_black_border_for_real_content();
	test_preserves_content_reaching_surface_edge();
	test_chooses_background_outside_origin_anchored_crop();
	test_border_state_changes_reset_preserved_crop();
	return failures == 0 ? 0 : 1;
}
