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
		make_buffer(pixels, width, height), 16, false, crop, state);

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
		make_buffer(pixels, width, height), 16, false, crop, state);

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
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(!changed, "Scattered outside pixels must not expand the crop");
	expect_eq(crop.w, 20, "Scattered pixels must not change crop width");
	expect_eq(crop.h, 12, "Scattered pixels must not change crop height");
}

static void test_preserves_diagonally_connected_content()
{
	constexpr int width = 40;
	constexpr int height = 40;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int i = 0; i < 16; i++) {
		pixels[(22 + i) * width + (1 + i)] = 0x00ffffffu;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 20, 8, 10, 12 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed,
		"Eight-neighbor scanning must keep diagonal content in one component");
	expect_eq(crop.x, 1, "Diagonal content should expand the crop left edge");
	expect_eq(crop.y, 8, "Diagonal content should retain the crop top edge");
	expect_eq(crop.w, 29, "Diagonal content should retain the crop right edge");
	expect_eq(crop.h, 30, "Diagonal content should expand the crop bottom edge");
}

static void test_crop_separates_outside_components()
{
	constexpr int width = 40;
	constexpr int height = 40;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int y = 12; y < 17; y++) {
		for (int x = 12; x < 15; x++) {
			pixels[y * width + x] = 0x00ffffffu;
		}
		for (int x = 25; x < 28; x++) {
			pixels[y * width + x] = 0x00ffffffu;
		}
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 15, 10, 10, 10 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(!changed,
		"The crop must not connect two individually sub-threshold components");
	expect_eq(crop.x, 15, "Separated components must not move the crop left edge");
	expect_eq(crop.w, 10, "Separated components must not widen the crop");
}

static void test_expands_to_visible_sprite_edge_content()
{
	constexpr int width = 40;
	constexpr int height = 40;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int y = 10; y < 18; y++) {
		pixels[y * width + 6] = 0x00ffffffu;
		pixels[y * width + 7] = 0x00ffffffu;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 20, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "A visible sprite strip outside DIW should expand the crop");
	expect_eq(crop.x, 6, "The crop should include the sprite's left edge");
	expect_eq(crop.w, 22, "The crop should preserve its right edge after sprite expansion");
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
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(!changed, "A non-black border must not expand the hardware crop");
	expect_true(state.border.count > 0, "A consistent non-black border should be detected");
	expect_eq(state.border.rgb[0], border_color, "Detected border color should match the perimeter");
	expect_eq(crop.x, 8, "Outer black surface edge must not move the crop left");
	expect_eq(crop.y, 8, "Outer black surface edge must not move the crop top");
	expect_eq(crop.w, 24, "Outer black surface edge must not widen the crop");
	expect_eq(crop.h, 20, "Outer black surface edge must not increase crop height");
}

static void test_uniform_border_avoids_component_scan()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00aaaaaau;
	std::vector<uint32_t> pixels(width * height, border_color);

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(!changed, "A uniform border must not expand the crop");
	expect_true(state.visited.empty(), "A uniform border should bypass component scanning");
	expect_true(state.border.count > 0, "A uniform non-black border should be detected");
	expect_eq(state.border.rgb[0], border_color, "Uniform border color should be preserved");

	std::vector<uint16_t> pixels_16(width * height, 0xaaaau);
	const AmiberryAutoCropPixelBuffer buffer_16 {
		reinterpret_cast<const uint8_t*>(pixels_16.data()), width, height,
		width * static_cast<int>(sizeof(uint16_t)), static_cast<int>(sizeof(uint16_t)), 0xffffu
	};
	state = {};
	crop = { 8, 8, 24, 20 };
	expect_true(!amiberry_auto_crop_expand_to_visible_content(buffer_16, 16, false, crop, state),
		"A 16-bit uniform border must not expand the crop");
	expect_true(state.visited.empty(), "A 16-bit uniform border should use the fast path");
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
		make_buffer(pixels, width, height), 16, false, crop, state);

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
		make_buffer(pixels, width, height), 16, false, crop, state);

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
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(!changed, "An in-crop origin pixel must not become the surface background");
	expect_eq(crop.x, 0, "Origin-anchored crop must keep its x origin");
	expect_eq(crop.y, 0, "Origin-anchored crop must keep its y origin");
	expect_eq(crop.w, 24, "Cleared right edge must not widen an origin-anchored crop");
	expect_eq(crop.h, 20, "Cleared bottom edge must not increase crop height");
}

static void test_one_sided_uniform_content_is_not_border()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t content_color = 0x0000ff00u;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int x = 0; x < width; x++) {
		pixels[20 * width + x] = content_color;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 0, 0, width, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "Uniform content on the only outside edge must remain visible");
	expect_true(state.border.count > 0, "Outside corners should provide a fallback background");
	expect_eq(state.border.rgb[0], 0u, "One-sided content must not become the border color");
	expect_eq(crop.x, 0, "Full-width content must keep the crop x origin");
	expect_eq(crop.y, 0, "Content below the crop must keep its y origin");
	expect_eq(crop.w, width, "Full-width content must keep the crop width");
	expect_eq(crop.h, 21, "Crop should expand through the adjacent content row");
}

static void test_ambiguous_perimeter_preserves_content()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t content_color = 0x0000ff00u;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int x = 4; x < 36; x++) {
		pixels[20 * width + x] = content_color;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 4, 0, 32, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "Content on an ambiguous perimeter must remain visible");
	expect_true(state.border.count > 0, "Ambiguous perimeter should use surface background");
	expect_eq(state.border.rgb[0], 0u, "Ambiguous content must not become the border color");
	expect_eq(crop.x, 4, "Lower content must keep the crop x origin");
	expect_eq(crop.y, 0, "Top-aligned crop must keep its y origin");
	expect_eq(crop.w, 32, "Lower content must keep the crop width");
	expect_eq(crop.h, 21, "Crop should expand through ambiguous lower content");
}

static void test_mixed_perimeter_keeps_non_black_border()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00aaaaaau;
	constexpr uint32_t content_color = 0x0000ff00u;
	std::vector<uint32_t> pixels(width * height, 0);
	add_colored_border(pixels, width, height, border_color);
	for (int x = 8; x < 32; x++) {
		pixels[28 * width + x] = content_color;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "Content on one side should expand a non-black border crop");
	expect_true(state.border.count > 0, "Other sides should confirm the non-black border");
	expect_eq(state.border.rgb[0], border_color,
		"One content side must not replace the non-black border color");
	expect_eq(crop.x, 8, "Mixed perimeter content must keep the crop x origin");
	expect_eq(crop.y, 8, "Mixed perimeter content must keep the crop y origin");
	expect_eq(crop.w, 24, "Mixed perimeter content must keep the crop width");
	expect_eq(crop.h, 21, "Crop should include content without including gray border");
}

static void test_two_sided_color_is_not_perimeter_majority()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t horizontal_color = 0x0000ff00u;
	constexpr uint32_t left_color = 0x00ff0000u;
	constexpr uint32_t right_color = 0x000000ffu;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int x = 8; x < 32; x++) {
		pixels[7 * width + x] = horizontal_color;
		pixels[28 * width + x] = horizontal_color;
	}
	for (int y = 8; y < 28; y++) {
		pixels[y * width + 7] = left_color;
		pixels[y * width + 32] = right_color;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "A two-of-four side color must remain visible content");
	expect_true(state.border.count > 0, "Split perimeter should use surface background");
	expect_eq(state.border.rgb[0], 0u, "A perimeter plurality must not become border color");
	expect_eq(crop.x, 7, "Visible left edge should expand the crop left");
	expect_eq(crop.y, 7, "Visible top edge should expand the crop upward");
	expect_eq(crop.w, 26, "Visible side edges should expand the crop width");
	expect_eq(crop.h, 22, "Visible horizontal edges should expand the crop height");
}

static void test_two_of_three_sides_is_not_border_confidence()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t content_color = 0x0000ff00u;
	std::vector<uint32_t> pixels(width * height, 0);
	for (int x = 0; x < 24; x++) {
		pixels[7 * width + x] = content_color;
		pixels[28 * width + x] = content_color;
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 0, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "A two-of-three side color must remain visible content");
	expect_true(state.border.count > 0, "Low-confidence perimeter should use surface background");
	expect_eq(state.border.rgb[0], 0u, "Two-of-three agreement must not become border color");
	expect_eq(crop.x, 0, "Left-aligned crop must keep its x origin");
	expect_eq(crop.y, 7, "Visible top edge should expand the crop upward");
	expect_eq(crop.w, 24, "Horizontal content must keep the crop width");
	expect_eq(crop.h, 22, "Visible top and bottom edges should expand crop height");
}

static void test_interlaced_border_weave_is_not_content()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t previous_field = 0x00100810u;
	constexpr uint32_t current_field = 0x00100c14u;
	std::vector<uint32_t> pixels(width * height, 0);
	// An interlaced display only redraws every other scanline per field, so a
	// change of the Amiga border color leaves two generations of it woven into
	// alternate rows. Both are border, not visible content.
	for (int y = 2; y < height - 2; y++) {
		for (int x = 2; x < width - 2; x++) {
			pixels[y * width + x] = (y & 1) ? current_field : previous_field;
		}
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, true, crop, state);

	expect_true(!changed, "An interlaced border weave must not expand the crop");
	expect_eq(state.border.count, 2, "Both woven generations must count as border");
	expect_eq(crop.x, 8, "A woven border must keep the crop x origin");
	expect_eq(crop.y, 8, "A woven border must keep the crop y origin");
	expect_eq(crop.w, 24, "A woven border must keep the crop width");
	expect_eq(crop.h, 20, "A woven border must keep the crop height");

	AmiberryAutoCropScanState progressive;
	AmiberryAutoCropRect progressive_crop{ 8, 8, 24, 20 };
	expect_true(amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, progressive_crop, progressive),
		"A progressive scan must still treat alternating rows as content");
}

static void test_settled_perimeter_carries_the_previous_field_border()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t previous_field = 0x00100810u;
	constexpr uint32_t current_field = 0x00100c14u;
	const AmiberryAutoCropRect crop{ 8, 12, 24, 20 };
	// The rows next to the crop settle on the current border a field before the
	// overscan further out does, so the weave is invisible to perimeter samples.
	std::vector<uint32_t> pixels(width * height, 0);
	for (int y = 2; y < height - 2; y++) {
		for (int x = 2; x < width - 2; x++) {
			pixels[y * width + x] = y >= crop.y - 4 && y < crop.y + crop.h + 4
				? current_field
				: ((y & 1) ? current_field : previous_field);
		}
	}

	AmiberryAutoCropScanState state;
	state.previous_border = amiberry_auto_crop_single_border(previous_field);
	AmiberryAutoCropRect carried = crop;
	expect_true(!amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, true, carried, state),
		"The previous field's border must not expand an interlaced crop");
	expect_eq(carried.y, crop.y, "A carried border must keep the crop y origin");
	expect_eq(carried.h, crop.h, "A carried border must keep the crop height");

	AmiberryAutoCropScanState unrelated;
	unrelated.previous_border = amiberry_auto_crop_single_border(0x00ff0000u);
	AmiberryAutoCropRect expanded = crop;
	expect_true(amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, true, expanded, unrelated),
		"An unrelated stale border must not hide the woven overscan");
}

static void test_edge_content_run_is_not_an_interlaced_border()
{
	constexpr int width = 40;
	constexpr int height = 40;
	constexpr uint32_t border_color = 0x00100810u;
	constexpr uint32_t content_color = 0x0000ff00u;
	std::vector<uint32_t> pixels(width * height, 0);
	add_colored_border(pixels, width, height, border_color);
	// Content hugging one edge covers a contiguous run of scanlines instead of
	// alternating ones, so it must not be mistaken for the other field.
	for (int y = 12; y < 24; y++) {
		for (int x = 6; x < 8; x++) {
			pixels[y * width + x] = content_color;
		}
	}

	AmiberryAutoCropScanState state;
	AmiberryAutoCropRect crop{ 8, 8, 24, 20 };
	const bool changed = amiberry_auto_crop_expand_to_visible_content(
		make_buffer(pixels, width, height), 16, false, crop, state);

	expect_true(changed, "Contiguous edge content must still expand the crop");
	expect_eq(crop.x, 6, "Visible left edge content should expand the crop left");
	expect_eq(crop.y, 8, "Left edge content must keep the crop y origin");
	expect_eq(crop.w, 26, "Visible left edge content should widen the crop");
	expect_eq(crop.h, 20, "Left edge content must keep the crop height");
}

static void test_border_state_changes_reset_preserved_crop()
{
	constexpr uint32_t first_color = 0x00112233u;
	constexpr uint32_t second_color = 0x00445566u;
	const AmiberryAutoCropBorderColors none{};
	const AmiberryAutoCropBorderColors first =
		amiberry_auto_crop_single_border(first_color);
	const AmiberryAutoCropBorderColors second =
		amiberry_auto_crop_single_border(second_color);
	const AmiberryAutoCropBorderColors woven{ { first_color, second_color }, 2 };

	expect_true(amiberry_auto_crop_border_state_changed(first, none),
		"Losing a valid border must reset preserved crop bounds");
	expect_true(amiberry_auto_crop_border_state_changed(none, first),
		"Detecting a valid border must reset preserved crop bounds");
	expect_true(amiberry_auto_crop_border_state_changed(first, second),
		"Changing border color must reset preserved crop bounds");
	expect_true(amiberry_auto_crop_border_state_changed(first, woven),
		"Gaining a woven second border color must reset preserved crop bounds");
	expect_true(!amiberry_auto_crop_border_state_changed(first, first),
		"An unchanged valid border must preserve crop bounds");
	expect_true(!amiberry_auto_crop_border_state_changed(woven, woven),
		"An unchanged woven border must preserve crop bounds");
	expect_true(!amiberry_auto_crop_border_state_changed(none, none),
		"Ambiguous border samples must ignore stale colors");
}

static void test_horizontal_edge_jitter_tolerance()
{
	const AmiberryAutoCropRect stable{ 38, 24, 320, 200 };

	expect_true(amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 37, 24, 321, 200 }, 2),
		"A one-pixel left sprite extension should keep the stable crop");
	expect_true(amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 36, 24, 322, 200 }, 2),
		"A two-pixel left sprite extension should keep the stable crop");
	expect_true(amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 38, 24, 322, 200 }, 2),
		"A two-pixel right sprite extension should keep the stable crop");
	expect_true(amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 37, 24, 322, 200 }, 2),
		"A two-sided sprite expansion should keep the stable crop");
	expect_true(amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 39, 24, 318, 200 }, 2),
		"A two-sided sprite contraction should keep the stable crop");
	expect_true(!amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 37, 24, 320, 200 }, 2),
		"A one-pixel left crop translation must update the crop");
	expect_true(!amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 39, 24, 320, 200 }, 2),
		"A one-pixel right crop translation must update the crop");
	expect_true(!amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 36, 24, 321, 200 }, 2),
		"Mixed same-direction edge movement must update the crop");
	expect_true(!amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 35, 24, 323, 200 }, 2),
		"A larger horizontal expansion must update the crop");
	expect_true(!amiberry_auto_crop_rect_within_horizontal_tolerance(
		stable, { 38, 23, 320, 201 }, 2),
		"A vertical crop change must not be treated as horizontal jitter");
}

static void test_horizontal_edge_jitter_requires_sprite_source_attribution()
{
	const AmiberryAutoCropRect source{ 38, 24, 320, 200 };
	const AmiberryAutoCropRect stable{ 38, 24, 320, 200 };
	const AmiberryAutoCropRect left_expanded{ 37, 24, 321, 200 };
	const AmiberryAutoCropRect left_expanded_source{ 37, 24, 321, 200 };
	const AmiberryAutoCropRect right_expanded_source{ 38, 24, 322, 200 };

	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, source, stable, left_expanded,
		true, false, false, false, 2),
		"Previous sprite attribution must not hide a change when the source is unchanged");

	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, left_expanded_source, stable, left_expanded,
		false, false, true, false, 2),
		"An attributed left source extension should preserve the crop");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, left_expanded_source, stable, left_expanded,
		true, false, false, false, 2),
		"An unattributed left source extension must reset stabilization");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, left_expanded_source, stable, left_expanded,
		false, false, false, true, 2),
		"Right attribution must not cover a left source extension");
	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, left_expanded_source, stable, { 36, 24, 322, 200 },
		false, false, true, false, 2),
		"A small scan fringe on an attributed sprite edge should preserve the crop");

	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, right_expanded_source, stable, right_expanded_source,
		false, false, false, true, 2),
		"An attributed right source extension should preserve the crop");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, right_expanded_source, stable, right_expanded_source,
		false, true, false, false, 2),
		"An unattributed right source extension must reset stabilization");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, right_expanded_source, stable, right_expanded_source,
		false, false, true, false, 2),
		"Left attribution must not cover a right source extension");

	const AmiberryAutoCropRect both_expanded_source{ 37, 24, 323, 200 };
	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, both_expanded_source, stable, both_expanded_source,
		false, false, true, true, 2),
		"Two-sided source extensions should preserve the crop when both are attributed");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, both_expanded_source, stable, both_expanded_source,
		false, false, true, false, 2),
		"Two-sided source extensions require right attribution");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, both_expanded_source, stable, both_expanded_source,
		false, false, false, true, 2),
		"Two-sided source extensions require left attribution");

	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		left_expanded_source, source, left_expanded, stable,
		true, false, false, false, 2),
		"A left source contraction should use the previous sprite attribution");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		left_expanded_source, source, left_expanded, stable,
		false, false, false, false, 2),
		"A left source contraction without previous attribution must update the crop");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		left_expanded_source, source, left_expanded, stable,
		false, false, true, false, 2),
		"Current attribution must not cover a left source contraction");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		left_expanded_source, source, left_expanded, stable,
		false, true, false, false, 2),
		"Previous right attribution must not cover a left source contraction");
	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		left_expanded_source, source, { 36, 24, 322, 200 }, stable,
		true, false, false, false, 2),
		"A sprite edge contraction should also absorb its previous scan fringe");

	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		right_expanded_source, source, right_expanded_source, stable,
		false, true, false, false, 2),
		"A right source contraction should use the previous sprite attribution");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		right_expanded_source, source, right_expanded_source, stable,
		false, false, false, false, 2),
		"A right source contraction without previous attribution must update the crop");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		right_expanded_source, source, right_expanded_source, stable,
		true, false, false, false, 2),
		"Previous left attribution must not cover a right source contraction");

	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, { 37, 24, 320, 200 }, stable, left_expanded,
		true, true, true, true, 2),
		"A source translation must reset stabilization despite sprite attribution");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, { 38, 23, 320, 200 }, stable, left_expanded,
		true, true, true, true, 2),
		"A source y change must reset stabilization despite sprite attribution");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, { 38, 24, 320, 201 }, stable, left_expanded,
		true, true, true, true, 2),
		"A source height change must reset stabilization despite sprite attribution");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		source, { 35, 24, 323, 200 }, stable, { 35, 24, 323, 200 },
		false, false, true, false, 2),
		"A source change beyond tolerance must reset stabilization");
}

static void test_sprite_zero_scan_jitter_requires_exact_edge_evidence()
{
	const AmiberryAutoCropRect source{ 38, 24, 320, 200 };
	const AmiberryAutoCropRect stable{ 38, 24, 320, 200 };
	const AmiberryAutoCropHorizontalEvidence no_evidence{ 0, 0, false, false };
	const AmiberryAutoCropHorizontalEvidence left_evidence{ 37, 0, true, false };
	const AmiberryAutoCropHorizontalEvidence right_evidence{ 0, 359, false, true };

	expect_true(amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 37, 24, 321, 200 },
		no_evidence, left_evidence, 2),
		"A scan-only left expansion should use current sprite-0 evidence");
	expect_true(amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, { 37, 24, 321, 200 }, stable,
		left_evidence, no_evidence, 2),
		"A scan-only left contraction should use previous sprite-0 evidence");
	expect_true(amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 38, 24, 321, 200 },
		no_evidence, right_evidence, 2),
		"A scan-only right expansion should use current sprite-0 evidence");
	expect_true(amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, { 38, 24, 321, 200 }, stable,
		right_evidence, no_evidence, 2),
		"A scan-only right contraction should use previous sprite-0 evidence");

	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 37, 24, 321, 200 },
		no_evidence, no_evidence, 2),
		"Arbitrary scan-only content must not be classified as sprite jitter");
	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 36, 24, 322, 200 },
		no_evidence, left_evidence, 2),
		"Scan content beyond the sprite-0 edge must update the crop");
	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 37, 24, 322, 200 },
		no_evidence, left_evidence, 2),
		"Every changed scan edge must have matching sprite-0 evidence");
	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 37, 23, 321, 201 },
		no_evidence, left_evidence, 2),
		"A vertical scan change must update the crop");
	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 37, 24, 320, 200 },
		no_evidence, left_evidence, 2),
		"A scan translation must update the crop");
	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, { 37, 24, 321, 200 }, stable, { 37, 24, 321, 200 },
		no_evidence, left_evidence, 2),
		"A source geometry change must not use the scan-only path");
	expect_true(!amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
		source, source, stable, { 37, 24, 321, 200 },
		no_evidence, { 38, 0, true, false }, 2),
		"Sprite-0 evidence inside the source must not hide scan content");
}

static void test_sprite_jitter_preserves_large_nested_source_changes()
{
	const AmiberryAutoCropRect narrow_source{ 107, 48, 609, 400 };
	const AmiberryAutoCropRect wide_source{ 73, 48, 643, 400 };
	const AmiberryAutoCropRect stable{ 76, 48, 640, 400 };
	const AmiberryAutoCropRect edge_extended{ 72, 48, 644, 400 };

	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		narrow_source, wide_source, stable, edge_extended,
		false, false, true, false, 4),
		"A large attributed sprite-source expansion plus scan fringe must keep the stable crop");
	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		wide_source, narrow_source, edge_extended, stable,
		true, false, false, false, 4),
		"The matching sprite-source contraction must restore no crop movement");
	expect_true(amiberry_auto_crop_should_preserve_horizontal_jitter(
		{ 76, 48, 609, 400 }, { 76, 48, 643, 400 },
		stable, { 76, 48, 644, 400 },
		false, false, false, true, 4),
		"The equivalent attributed right-edge expansion must keep the stable crop");

	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		narrow_source, { 73, 48, 609, 400 }, stable, edge_extended,
		false, false, true, true, 4),
		"A translated source must not be classified as nested sprite jitter");
	expect_true(!amiberry_auto_crop_should_preserve_horizontal_jitter(
		narrow_source, wide_source, stable, { 71, 48, 645, 400 },
		false, false, true, false, 4),
		"A final crop change beyond tolerance must still update presentation");
}

static void test_vertical_transition_replaces_displaced_border_without_resizing()
{
	constexpr int width = 20;
	constexpr int height = 20;
	std::vector<uint32_t> pixels(width * height, 0);
	const AmiberryAutoCropRect previous{ 4, 4, 12, 8 };
	for (int y = 6; y < 14; y++) {
		for (int x = previous.x; x < previous.x + previous.w; x++) {
			pixels[y * width + x] = 0x0000ff00u;
		}
	}

	AmiberryAutoCropRect expanded{ 4, 4, 12, 10 };
	expect_true(amiberry_auto_crop_stabilize_vertical_transition(
		make_buffer(pixels, width, height), 16, previous, expanded, amiberry_auto_crop_single_border(0), 2),
		"New bottom content should replace a border strip without resizing");
	expect_eq(expanded.y, 6,
		"The stable crop should move down to follow the new content");
	expect_eq(expanded.h, 8,
		"Replacing displaced border must preserve the crop height");

	AmiberryAutoCropRect raw_union{ 4, 4, 12, 10 };
	expect_true(amiberry_auto_crop_stabilize_vertical_transition(
		make_buffer(pixels, width, height), 16, expanded, raw_union, amiberry_auto_crop_single_border(0), 2),
		"A repeated raw union should ignore the revealed border above the stable crop");
	expect_eq(raw_union.y, 6,
		"Repeated scan frames should retain the translated origin");
	expect_eq(raw_union.h, 8,
		"Repeated scan frames should retain the stable height");
}

static void test_vertical_transition_keeps_genuine_two_edge_content()
{
	constexpr int width = 20;
	constexpr int height = 20;
	std::vector<uint32_t> pixels(width * height, 0);
	const AmiberryAutoCropRect previous{ 4, 4, 12, 8 };
	AmiberryAutoCropRect expanded{ 4, 4, 12, 10 };
	for (int y = expanded.y; y < expanded.y + expanded.h; y++) {
		for (int x = expanded.x; x < expanded.x + expanded.w; x++) {
			pixels[y * width + x] = 0x0000ff00u;
		}
	}

	expect_true(!amiberry_auto_crop_stabilize_vertical_transition(
		make_buffer(pixels, width, height), 16, previous, expanded, amiberry_auto_crop_single_border(0), 2),
		"Visible pixels at both old and new edges must allow a real crop expansion");
	expect_eq(expanded.y, 4,
		"A genuine expansion should retain its original top edge");
	expect_eq(expanded.h, 10,
		"A genuine expansion should retain its larger height");
}

static void test_repeated_vertical_translation_does_not_form_a_union()
{
	const AmiberryAutoCropRect source{ 76, 34, 640, 400 };
	const AmiberryAutoCropRect translated{ 76, 48, 640, 400 };

	expect_true(amiberry_auto_crop_should_preserve_vertical_translation(
		source, translated, source, 16),
		"The unchanged hardware source must not undo an established crop translation");
	expect_true(amiberry_auto_crop_should_preserve_vertical_translation(
		source, translated, { 76, 34, 640, 414 }, 16),
		"The union of source and translated crop must retain the translated size");
	expect_true(!amiberry_auto_crop_should_preserve_vertical_translation(
		source, translated, { 76, 33, 640, 416 }, 16),
		"Content beyond both known positions must still expand the crop");
	expect_true(!amiberry_auto_crop_should_preserve_vertical_translation(
		source, { 76, 51, 640, 400 }, source, 16),
		"A translation beyond tolerance must not suppress a crop change");
	expect_true(!amiberry_auto_crop_should_preserve_vertical_translation(
		source, { 76, 48, 640, 400 }, { 75, 34, 641, 414 }, 16),
		"A simultaneous horizontal expansion must still update the crop");
	expect_true(!amiberry_auto_crop_should_preserve_vertical_translation(
		source, translated, { 76, 35, 640, 399 }, 16),
		"A candidate that no longer covers the source must not reuse stale geometry");
}

int main()
{
	test_expands_to_connected_visible_content();
	test_ignores_distant_speck_when_content_expands();
	test_ignores_scattered_outside_pixels();
	test_preserves_diagonally_connected_content();
	test_crop_separates_outside_components();
	test_expands_to_visible_sprite_edge_content();
	test_keeps_crop_inside_non_black_border();
	test_uniform_border_avoids_component_scan();
	test_expands_past_non_black_border_for_real_content();
	test_preserves_content_reaching_surface_edge();
	test_chooses_background_outside_origin_anchored_crop();
	test_one_sided_uniform_content_is_not_border();
	test_ambiguous_perimeter_preserves_content();
	test_mixed_perimeter_keeps_non_black_border();
	test_two_sided_color_is_not_perimeter_majority();
	test_two_of_three_sides_is_not_border_confidence();
	test_interlaced_border_weave_is_not_content();
	test_settled_perimeter_carries_the_previous_field_border();
	test_edge_content_run_is_not_an_interlaced_border();
	test_border_state_changes_reset_preserved_crop();
	test_horizontal_edge_jitter_tolerance();
	test_horizontal_edge_jitter_requires_sprite_source_attribution();
	test_sprite_zero_scan_jitter_requires_exact_edge_evidence();
	test_sprite_jitter_preserves_large_nested_source_changes();
	test_vertical_transition_replaces_displaced_border_without_resizing();
	test_vertical_transition_keeps_genuine_two_edge_content();
	test_repeated_vertical_translation_does_not_form_a_union();
	return failures == 0 ? 0 : 1;
}
