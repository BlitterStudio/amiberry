#ifndef AMIBERRY_AUTOCROP_HELPERS_H
#define AMIBERRY_AUTOCROP_HELPERS_H

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

struct AmiberryAutoCropRect {
	int x;
	int y;
	int w;
	int h;
};

struct AmiberryAutoCropHorizontalEvidence {
	int left;
	int right;
	bool left_valid;
	bool right_valid;
};

struct AmiberryAutoCropPixelBuffer {
	const uint8_t* pixels;
	int width;
	int height;
	int pitch;
	int bytes_per_pixel;
	uint32_t rgb_mask;
};

// Interlaced fields only redraw every other scanline, so a change of the Amiga
// border color leaves the previous generation of it woven into the rows of the
// other field. Both colors are border for as long as that weave lasts.
struct AmiberryAutoCropBorderColors {
	uint32_t rgb[2];
	int count;
};

static inline bool amiberry_auto_crop_border_matches(
	const AmiberryAutoCropBorderColors& border, const uint32_t rgb)
{
	return (border.count > 0 && border.rgb[0] == rgb)
		|| (border.count > 1 && border.rgb[1] == rgb);
}

static inline AmiberryAutoCropBorderColors amiberry_auto_crop_single_border(
	const uint32_t rgb)
{
	return { { rgb, rgb }, 1 };
}

struct AmiberryAutoCropScanState {
	std::vector<uint8_t> visited;
	std::vector<int> pending;
	std::vector<uint32_t> border_samples;
	std::vector<uint32_t> border_field_samples[2];
	AmiberryAutoCropBorderColors border{};
	// The last perimeter-detected border, kept so an interlaced scan can still
	// recognize the generation the other field was drawn with.
	AmiberryAutoCropBorderColors previous_border{};
};

static inline bool amiberry_auto_crop_border_state_changed(
	const AmiberryAutoCropBorderColors& previous,
	const AmiberryAutoCropBorderColors& current)
{
	if (previous.count != current.count) {
		return true;
	}
	for (int i = 0; i < current.count; i++) {
		if (previous.rgb[i] != current.rgb[i]) {
			return true;
		}
	}
	return false;
}

static inline int amiberry_auto_crop_rect_right(const AmiberryAutoCropRect& rect)
{
	return rect.x + rect.w;
}

static inline int amiberry_auto_crop_rect_bottom(const AmiberryAutoCropRect& rect)
{
	return rect.y + rect.h;
}

static inline bool amiberry_auto_crop_rect_within_horizontal_tolerance(
	const AmiberryAutoCropRect& previous, const AmiberryAutoCropRect& current,
	const int tolerance)
{
	if (tolerance < 0 || previous.w <= 0 || previous.h <= 0
		|| current.w <= 0 || current.h <= 0
		|| previous.y != current.y || previous.h != current.h) {
		return false;
	}

	return std::abs(previous.x - current.x) <= tolerance
		&& std::abs(amiberry_auto_crop_rect_right(previous)
			- amiberry_auto_crop_rect_right(current)) <= tolerance
		&& ((previous.x <= current.x
				&& amiberry_auto_crop_rect_right(previous)
					>= amiberry_auto_crop_rect_right(current))
			|| (current.x <= previous.x
				&& amiberry_auto_crop_rect_right(current)
					>= amiberry_auto_crop_rect_right(previous)));
}

static inline bool amiberry_auto_crop_should_preserve_horizontal_jitter(
	const AmiberryAutoCropRect& previous_source,
	const AmiberryAutoCropRect& current_source,
	const AmiberryAutoCropRect& previous_visible,
	const AmiberryAutoCropRect& current_visible,
	const bool previous_source_left_is_sprite,
	const bool previous_source_right_is_sprite,
	const bool current_source_left_is_sprite,
	const bool current_source_right_is_sprite, const int tolerance)
{
	if (previous_source.w <= 0 || previous_source.h <= 0
		|| current_source.w <= 0 || current_source.h <= 0
		|| previous_source.y != current_source.y
		|| previous_source.h != current_source.h
		|| !amiberry_auto_crop_rect_within_horizontal_tolerance(
			previous_visible, current_visible, tolerance)) {
		return false;
	}

	const int previous_source_right = amiberry_auto_crop_rect_right(previous_source);
	const int current_source_right = amiberry_auto_crop_rect_right(current_source);
	const bool previous_contains_current = previous_source.x <= current_source.x
		&& previous_source_right >= current_source_right;
	const bool current_contains_previous = current_source.x <= previous_source.x
		&& current_source_right >= previous_source_right;
	if (!previous_contains_current && !current_contains_previous) {
		return false;
	}

	const int previous_visible_right = amiberry_auto_crop_rect_right(previous_visible);
	const int current_visible_right = amiberry_auto_crop_rect_right(current_visible);
	const bool source_left_changed = previous_source.x != current_source.x;
	const bool source_right_changed = previous_source_right != current_source_right;
	if (!source_left_changed && !source_right_changed) {
		return false;
	}

	if (source_left_changed) {
		const bool expands = current_source.x < previous_source.x;
		if ((expands && !current_source_left_is_sprite)
			|| (!expands && !previous_source_left_is_sprite)) {
			return false;
		}
	}
	if (source_right_changed) {
		const bool expands = current_source_right > previous_source_right;
		if ((expands && !current_source_right_is_sprite)
			|| (!expands && !previous_source_right_is_sprite)) {
			return false;
		}
	}

	if (previous_visible.x != current_visible.x) {
		const bool source_expands = current_source.x < previous_source.x;
		const bool visible_expands = current_visible.x < previous_visible.x;
		const int source_edge = source_expands
			? current_source.x : previous_source.x;
		const int visible_edge = source_expands
			? current_visible.x : previous_visible.x;
		if (!source_left_changed || source_expands != visible_expands
			|| visible_edge > source_edge
			|| source_edge - visible_edge > tolerance) {
			return false;
		}
	}
	if (previous_visible_right != current_visible_right) {
		const bool source_expands = current_source_right > previous_source_right;
		const bool visible_expands = current_visible_right > previous_visible_right;
		const int source_edge = source_expands
			? current_source_right : previous_source_right;
		const int visible_edge = source_expands
			? current_visible_right : previous_visible_right;
		if (!source_right_changed || source_expands != visible_expands
			|| visible_edge < source_edge
			|| visible_edge - source_edge > tolerance) {
			return false;
		}
	}

	return true;
}

static inline bool amiberry_auto_crop_should_preserve_vertical_translation(
	const AmiberryAutoCropRect& source,
	const AmiberryAutoCropRect& previous_visible,
	const AmiberryAutoCropRect& current_visible, const int tolerance)
{
	if (tolerance < 0
		|| source.w <= 0 || source.h <= 0
		|| previous_visible.w <= 0 || previous_visible.h <= 0
		|| current_visible.w <= 0 || current_visible.h <= 0
		|| source.x != previous_visible.x
		|| source.w != previous_visible.w
		|| source.h != previous_visible.h
		|| source.y == previous_visible.y
		|| std::abs(source.y - previous_visible.y) > tolerance
		|| current_visible.x != source.x
		|| current_visible.w != source.w) {
		return false;
	}

	const int union_top = std::min(source.y, previous_visible.y);
	const int union_bottom = std::max(amiberry_auto_crop_rect_bottom(source),
		amiberry_auto_crop_rect_bottom(previous_visible));
	return current_visible.y <= source.y
		&& amiberry_auto_crop_rect_bottom(current_visible)
			>= amiberry_auto_crop_rect_bottom(source)
		&& current_visible.y >= union_top
		&& amiberry_auto_crop_rect_bottom(current_visible) <= union_bottom;
}

static inline bool amiberry_auto_crop_should_preserve_sprite_zero_scan_jitter(
	const AmiberryAutoCropRect& previous_source,
	const AmiberryAutoCropRect& current_source,
	const AmiberryAutoCropRect& previous_visible,
	const AmiberryAutoCropRect& current_visible,
	const AmiberryAutoCropHorizontalEvidence& previous_sprite_zero,
	const AmiberryAutoCropHorizontalEvidence& current_sprite_zero,
	const int tolerance)
{
	if (previous_source.x != current_source.x
		|| previous_source.y != current_source.y
		|| previous_source.w != current_source.w
		|| previous_source.h != current_source.h
		|| !amiberry_auto_crop_rect_within_horizontal_tolerance(
			previous_visible, current_visible, tolerance)) {
		return false;
	}

	const int source_right = amiberry_auto_crop_rect_right(current_source);
	const int previous_visible_right = amiberry_auto_crop_rect_right(previous_visible);
	const int current_visible_right = amiberry_auto_crop_rect_right(current_visible);
	const bool left_changed = previous_visible.x != current_visible.x;
	const bool right_changed = previous_visible_right != current_visible_right;
	if (!left_changed && !right_changed) {
		return false;
	}

	if (left_changed) {
		const bool expands = current_visible.x < previous_visible.x;
		const AmiberryAutoCropHorizontalEvidence& evidence = expands
			? current_sprite_zero : previous_sprite_zero;
		const int visible_left = expands ? current_visible.x : previous_visible.x;
		if (!evidence.left_valid || evidence.left >= current_source.x
			|| evidence.left != visible_left) {
			return false;
		}
	}
	if (right_changed) {
		const bool expands = current_visible_right > previous_visible_right;
		const AmiberryAutoCropHorizontalEvidence& evidence = expands
			? current_sprite_zero : previous_sprite_zero;
		const int visible_right = expands ? current_visible_right : previous_visible_right;
		if (!evidence.right_valid || evidence.right <= source_right
			|| evidence.right != visible_right) {
			return false;
		}
	}

	return true;
}

static inline bool amiberry_auto_crop_rect_contains(const AmiberryAutoCropRect& rect,
	const int x, const int y)
{
	return x >= rect.x && x < amiberry_auto_crop_rect_right(rect)
		&& y >= rect.y && y < amiberry_auto_crop_rect_bottom(rect);
}

static inline bool amiberry_auto_crop_buffer_valid(const AmiberryAutoCropPixelBuffer& buffer)
{
	return buffer.pixels
		&& buffer.width > 0
		&& buffer.height > 0
		&& buffer.pitch >= buffer.width * buffer.bytes_per_pixel
		&& buffer.bytes_per_pixel > 0
		&& buffer.bytes_per_pixel <= static_cast<int>(sizeof(uint32_t))
		&& buffer.rgb_mask != 0;
}

static inline uint32_t amiberry_auto_crop_read_pixel(
	const AmiberryAutoCropPixelBuffer& buffer, const int x, const int y)
{
	uint32_t pixel = 0;
	std::memcpy(&pixel,
		buffer.pixels + y * buffer.pitch + x * buffer.bytes_per_pixel,
		buffer.bytes_per_pixel);
	return pixel;
}

static inline void amiberry_auto_crop_get_outside_regions(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	AmiberryAutoCropRect (&regions)[4])
{
	const int right = amiberry_auto_crop_rect_right(crop);
	const int bottom = amiberry_auto_crop_rect_bottom(crop);
	regions[0] = { 0, 0, buffer.width, crop.y };
	regions[1] = { 0, crop.y, crop.x, crop.h };
	regions[2] = { right, crop.y, buffer.width - right, crop.h };
	regions[3] = { 0, bottom, buffer.width, buffer.height - bottom };
}

// True when the one-pixel band just outside the rect holds a pixel that is
// not a known border color. Crop expansion flood-fills outward from the
// rect's edges, so content can only enlarge the rect by crossing this band;
// a non-border pixel there means an immediate rescan is due. Sampled every
// 2nd pixel so the per-frame cost stays a few hundred reads.
static inline bool amiberry_auto_crop_edge_band_has_content(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& rect,
	const AmiberryAutoCropBorderColors& border)
{
	if (!amiberry_auto_crop_buffer_valid(buffer) || border.count <= 0) {
		return true;
	}
	const int right = amiberry_auto_crop_rect_right(rect);
	const int bottom = amiberry_auto_crop_rect_bottom(rect);
	auto sample_row = [&](const int y, const int x0, const int x1) {
		for (int x = x0; x < x1; x += 2) {
			if (!amiberry_auto_crop_border_matches(border,
				amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask)) {
				return true;
			}
		}
		return false;
	};
	auto sample_col = [&](const int x, const int y0, const int y1) {
		for (int y = y0; y < y1; y += 2) {
			if (!amiberry_auto_crop_border_matches(border,
				amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask)) {
				return true;
			}
		}
		return false;
	};
	if (rect.y > 0 && sample_row(rect.y - 1, rect.x, right)) {
		return true;
	}
	if (bottom < buffer.height && sample_row(bottom, rect.x, right)) {
		return true;
	}
	if (rect.x > 0 && sample_col(rect.x - 1, rect.y, bottom)) {
		return true;
	}
	if (right < buffer.width && sample_col(right, rect.y, bottom)) {
		return true;
	}
	return false;
}

// Sparse counterpart for content DISCONNECTED from the crop: expansion
// unions every non-border component in the outside regions, not only ones
// adjacent to the rect's edge. Sample those regions on a coarse grid (every
// 4th pixel) so a disconnected component large enough to expand the crop is
// normally hit within a frame; anything thinner than the stride falls back
// to the periodic interval scan.
static inline bool amiberry_auto_crop_outside_regions_have_content(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& rect,
	const AmiberryAutoCropBorderColors& border)
{
	if (!amiberry_auto_crop_buffer_valid(buffer) || border.count <= 0) {
		return true;
	}
	AmiberryAutoCropRect regions[4];
	amiberry_auto_crop_get_outside_regions(buffer, rect, regions);
	for (const auto& region : regions) {
		for (int y = region.y; y < amiberry_auto_crop_rect_bottom(region); y += 4) {
			for (int x = region.x; x < amiberry_auto_crop_rect_right(region); x += 4) {
				if (!amiberry_auto_crop_border_matches(border,
					amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask)) {
					return true;
				}
			}
		}
	}
	return false;
}

// Woven is a template parameter so the far more common single-color border
// keeps one comparison per pixel in this scan's hottest loop.
template<bool Woven>
static inline size_t amiberry_auto_crop_count_region_pixels(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& region,
	const uint32_t first_rgb, const uint32_t second_rgb)
{
	size_t visible_pixels = 0;
	const int region_bottom = amiberry_auto_crop_rect_bottom(region);
	const int region_right = amiberry_auto_crop_rect_right(region);
	if (buffer.bytes_per_pixel == static_cast<int>(sizeof(uint32_t))) { // Vectorized SDL path.
		for (int y = region.y; y < region_bottom; y++) {
			const uint8_t* pixel = buffer.pixels + y * buffer.pitch
				+ region.x * sizeof(uint32_t);
			for (int x = 0; x < region.w; x++, pixel += sizeof(uint32_t)) {
				uint32_t value;
				std::memcpy(&value, pixel, sizeof(value));
				value &= buffer.rgb_mask;
				visible_pixels += Woven
					? (value != first_rgb && value != second_rgb)
					: (value != first_rgb);
			}
		}
	} else {
		for (int y = region.y; y < region_bottom; y++) {
			for (int x = region.x; x < region_right; x++) {
				const uint32_t value = amiberry_auto_crop_read_pixel(buffer, x, y)
					& buffer.rgb_mask;
				visible_pixels += Woven
					? (value != first_rgb && value != second_rgb)
					: (value != first_rgb);
			}
		}
	}
	return visible_pixels;
}

static inline size_t amiberry_auto_crop_count_region_visible_pixels(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& region,
	const AmiberryAutoCropBorderColors& border)
{
	if (!amiberry_auto_crop_buffer_valid(buffer)
		|| region.x < 0 || region.y < 0 || region.w <= 0 || region.h <= 0
		|| amiberry_auto_crop_rect_right(region) > buffer.width
		|| amiberry_auto_crop_rect_bottom(region) > buffer.height) {
		return 0;
	}

	// No border color at all still leaves every pixel visible.
	const uint32_t first_rgb = border.count > 0 ? border.rgb[0] : ~0u;
	return border.count > 1
		? amiberry_auto_crop_count_region_pixels<true>(
			buffer, region, first_rgb, border.rgb[1])
		: amiberry_auto_crop_count_region_pixels<false>(
			buffer, region, first_rgb, first_rgb);
}

static inline size_t amiberry_auto_crop_count_visible_pixels(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const AmiberryAutoCropBorderColors& border)
{
	AmiberryAutoCropRect regions[4];
	amiberry_auto_crop_get_outside_regions(buffer, crop, regions);
	size_t visible_pixels = 0;
	for (const auto& region : regions) {
		visible_pixels += amiberry_auto_crop_count_region_visible_pixels(
			buffer, region, border);
	}
	return visible_pixels;
}

static inline bool amiberry_auto_crop_stabilize_vertical_transition(
	const AmiberryAutoCropPixelBuffer& buffer, const int min_visible_pixels,
	const AmiberryAutoCropRect& previous, AmiberryAutoCropRect& current,
	const AmiberryAutoCropBorderColors& border, const int tolerance)
{
	if (!amiberry_auto_crop_buffer_valid(buffer)
		|| tolerance < 0
		|| previous.x < 0 || previous.y < 0
		|| previous.w <= 0 || previous.h <= 0
		|| current.x < 0 || current.y < 0
		|| current.w <= 0 || current.h <= 0
		|| amiberry_auto_crop_rect_right(previous) > buffer.width
		|| amiberry_auto_crop_rect_bottom(previous) > buffer.height
		|| amiberry_auto_crop_rect_right(current) > buffer.width
		|| amiberry_auto_crop_rect_bottom(current) > buffer.height
		|| current.x != previous.x || current.w != previous.w
		|| current.y > previous.y
		|| amiberry_auto_crop_rect_bottom(current)
			< amiberry_auto_crop_rect_bottom(previous)) {
		return false;
	}

	const int top_growth = previous.y - current.y;
	const int bottom_growth = amiberry_auto_crop_rect_bottom(current)
		- amiberry_auto_crop_rect_bottom(previous);
	if ((top_growth == 0) == (bottom_growth == 0)
		|| top_growth + bottom_growth > tolerance) {
		return false;
	}

	const auto strip_is_border = [&](const AmiberryAutoCropRect& strip) {
		const size_t area = static_cast<size_t>(strip.w) * strip.h;
		const size_t required = std::min(area,
			static_cast<size_t>(std::max(1, min_visible_pixels)));
		return amiberry_auto_crop_count_region_visible_pixels(
			buffer, strip, border) < required;
	};

	if (bottom_growth > 0) {
		const AmiberryAutoCropRect revealed = {
			previous.x, amiberry_auto_crop_rect_bottom(previous),
			previous.w, bottom_growth
		};
		if (strip_is_border(revealed)) {
			current = previous;
			return true;
		}
		const AmiberryAutoCropRect displaced = {
			previous.x, previous.y, previous.w, bottom_growth
		};
		if (strip_is_border(displaced)) {
			current = { previous.x, previous.y + bottom_growth,
				previous.w, previous.h };
			return true;
		}
		return false;
	}

	const AmiberryAutoCropRect revealed = {
		current.x, current.y, current.w, top_growth
	};
	if (strip_is_border(revealed)) {
		current = previous;
		return true;
	}
	const AmiberryAutoCropRect displaced = {
		previous.x, amiberry_auto_crop_rect_bottom(previous) - top_growth,
		previous.w, top_growth
	};
	if (strip_is_border(displaced)) {
		current = { current.x, current.y, previous.w, previous.h };
		return true;
	}
	return false;
}

static inline size_t amiberry_auto_crop_find_dominant_color(
	const uint32_t* colors, const size_t color_count, uint32_t& dominant_rgb)
{
	if (color_count == 0) {
		return 0;
	}
	size_t dominant_count = 0;
	for (size_t i = 0; i < color_count; i++) {
		size_t matches = 0;
		for (size_t j = 0; j < color_count; j++) {
			matches += colors[i] == colors[j];
		}
		if (matches > dominant_count) {
			dominant_rgb = colors[i];
			dominant_count = matches;
		}
	}
	return dominant_count;
}

static inline bool amiberry_auto_crop_detect_surface_background_color(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	uint32_t& background_rgb)
{
	const int corner_x[] = { 0, buffer.width - 1, 0, buffer.width - 1 };
	const int corner_y[] = { 0, 0, buffer.height - 1, buffer.height - 1 };
	uint32_t corner_rgb[4];
	int corner_count = 0;
	for (int i = 0; i < 4; i++) {
		if (!amiberry_auto_crop_rect_contains(crop, corner_x[i], corner_y[i])) {
			corner_rgb[corner_count++] = amiberry_auto_crop_read_pixel(
				buffer, corner_x[i], corner_y[i]) & buffer.rgb_mask;
		}
	}
	return amiberry_auto_crop_find_dominant_color(
		corner_rgb, corner_count, background_rgb) > 0;
}

static inline bool amiberry_auto_crop_detect_border_color(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const uint32_t background_rgb, const bool background_valid,
	const bool interlaced, AmiberryAutoCropScanState& state)
{
	state.border = {};
	const int right = amiberry_auto_crop_rect_right(crop);
	const int bottom = amiberry_auto_crop_rect_bottom(crop);
	const int horizontal_step = std::max(1, crop.w / 64);
	// An odd step keeps a sampled column crossing both interlaced fields, so a
	// woven border cannot hide one of its two colors from the vote below.
	const int vertical_step = std::max(1, crop.h / 64) | 1;
	// A side that weaves two fields contributes both of their colors.
	uint32_t side_rgb[8];
	uint32_t woven_rgb[8];
	int sampled_sides = 0;
	int side_color_count = 0;
	const auto sample_side = [&](const int x, const int y, const int dx, const int dy,
		const int length, const int step) {
		state.border_samples.clear();
		state.border_field_samples[0].clear();
		state.border_field_samples[1].clear();
		for (int offset = 0; offset < length; offset += step) {
			const int sample_y = y + dy * offset;
			const uint32_t rgb = amiberry_auto_crop_read_pixel(
				buffer, x + dx * offset, sample_y) & buffer.rgb_mask;
			state.border_samples.push_back(rgb);
			state.border_field_samples[sample_y & 1].push_back(rgb);
		}
		uint32_t dominant_rgb;
		sampled_sides++;
		const size_t dominant_count = amiberry_auto_crop_find_dominant_color(
			state.border_samples.data(), state.border_samples.size(), dominant_rgb);
		if (dominant_count * 4 >= state.border_samples.size() * 3) {
			woven_rgb[side_color_count] = dominant_rgb;
			side_rgb[side_color_count++] = dominant_rgb;
			return;
		}
		if (!interlaced) {
			return;
		}
		// Only a strict scanline alternation is an interlaced border weave.
		// Content hugging the crop covers a run of lines, not every other one.
		uint32_t field_rgb[2];
		for (int field = 0; field < 2; field++) {
			const std::vector<uint32_t>& samples = state.border_field_samples[field];
			const size_t count = amiberry_auto_crop_find_dominant_color(
				samples.data(), samples.size(), field_rgb[field]);
			if (count == 0 || count * 4 < samples.size() * 3) {
				return;
			}
		}
		if (field_rgb[0] == field_rgb[1]) {
			return;
		}
		for (int field = 0; field < 2; field++) {
			woven_rgb[side_color_count] = field_rgb[!field];
			side_rgb[side_color_count++] = field_rgb[field];
		}
	};

	if (crop.y > 0) {
		sample_side(crop.x, crop.y - 1, 1, 0, crop.w, horizontal_step);
	}
	if (bottom < buffer.height) {
		sample_side(crop.x, bottom, 1, 0, crop.w, horizontal_step);
	}
	if (crop.x > 0) {
		sample_side(crop.x - 1, crop.y, 0, 1, crop.h, vertical_step);
	}
	if (right < buffer.width) {
		sample_side(right, crop.y, 0, 1, crop.h, vertical_step);
	}

	if (sampled_sides >= 2) {
		// Give each side one vote per color it contributes and require 75%
		// confidence before hiding it.
		for (int i = 0; i < side_color_count && state.border.count < 2; i++) {
			const uint32_t candidate = side_rgb[i];
			if (amiberry_auto_crop_border_matches(state.border, candidate)) {
				continue;
			}
			size_t votes = 0;
			for (int j = 0; j < side_color_count; j++) {
				votes += side_rgb[j] == candidate;
			}
			if (votes * 4 >= static_cast<size_t>(sampled_sides) * 3) {
				state.border.rgb[state.border.count++] = candidate;
			}
		}
		// A confirmed border color carries its weave partner: the other field
		// still shows the generation it was drawn with.
		for (int i = 0; i < side_color_count && state.border.count == 1; i++) {
			if (woven_rgb[i] != side_rgb[i]
				&& amiberry_auto_crop_border_matches(state.border, side_rgb[i])) {
				state.border.rgb[state.border.count++] = woven_rgb[i];
			}
		}
		if (state.border.count > 0) {
			// Keep the pair ordered so a field flip is not a border change.
			if (state.border.count > 1 && state.border.rgb[1] < state.border.rgb[0]) {
				std::swap(state.border.rgb[0], state.border.rgb[1]);
			}
			state.previous_border = state.border;
			return true;
		}
	}
	// One or ambiguous sides may be content; fall back to the cleared surface.
	// Nothing was learned about the border, so no generation is worth carrying.
	state.previous_border = {};
	if (background_valid) {
		state.border = amiberry_auto_crop_single_border(background_rgb);
	}
	return background_valid;
}

template<bool MatchColor>
static inline size_t amiberry_auto_crop_flood_region(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const int start_x, const int start_y, AmiberryAutoCropScanState& state,
	const AmiberryAutoCropBorderColors& colors, AmiberryAutoCropRect* bounds)
{
	if (amiberry_auto_crop_rect_contains(crop, start_x, start_y)) {
		return 0;
	}
	const int start_index = start_y * buffer.width + start_x;
	if (state.visited[start_index]) {
		return 0;
	}
	const auto pixel_matches = [&](const int x, const int y) {
		const int index = y * buffer.width + x;
		const bool matches = amiberry_auto_crop_border_matches(colors,
			amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask) == MatchColor;
		if (!matches) {
			if constexpr (!MatchColor) {
				state.visited[index] = 1;
			}
			return false;
		}
		return true;
	};
	if (!pixel_matches(start_x, start_y)) {
		return 0;
	}

	state.pending.push_back(start_index);
	size_t pixels = 0;
	if constexpr (!MatchColor) {
		*bounds = { start_x, start_y, start_x, start_y };
	}
	const auto queue_adjacent_runs = [&](const int y, const int left, const int right) {
		bool run_queued = false;
		for (int x = std::max(0, left - 1);
			x <= std::min(buffer.width - 1, right + 1); x++) {
			if (amiberry_auto_crop_rect_contains(crop, x, y)) {
				run_queued = false;
				continue;
			}
			const int index = y * buffer.width + x;
			if (state.visited[index]) {
				run_queued = false;
				continue;
			}
			if (!pixel_matches(x, y)) {
				run_queued = false;
				continue;
			}
			if (!run_queued) {
				state.pending.push_back(index);
				run_queued = true;
			}
		}
	};
	while (!state.pending.empty()) {
		const int index = state.pending.back();
		state.pending.pop_back();
		if (state.visited[index]) {
			continue;
		}
		const int pixel_x = index % buffer.width;
		const int pixel_y = index / buffer.width;
		int left = pixel_x;
		while (left > 0
			&& !amiberry_auto_crop_rect_contains(crop, left - 1, pixel_y)) {
			const int candidate = index - (pixel_x - left) - 1;
			if (state.visited[candidate] || !pixel_matches(left - 1, pixel_y)) {
				break;
			}
			left--;
		}
		int right = pixel_x;
		while (right + 1 < buffer.width
			&& !amiberry_auto_crop_rect_contains(crop, right + 1, pixel_y)) {
			const int candidate = index + (right - pixel_x) + 1;
			if (state.visited[candidate] || !pixel_matches(right + 1, pixel_y)) {
				break;
			}
			right++;
		}
		const int row_start = pixel_y * buffer.width;
		std::fill(state.visited.begin() + row_start + left,
			state.visited.begin() + row_start + right + 1, 1);
		pixels += static_cast<size_t>(right - left + 1);
		if constexpr (!MatchColor) {
			bounds->x = std::min(bounds->x, left);
			bounds->y = std::min(bounds->y, pixel_y);
			bounds->w = std::max(bounds->w, right);
			bounds->h = std::max(bounds->h, pixel_y);
		}
		if (pixel_y > 0) {
			queue_adjacent_runs(pixel_y - 1, left, right);
		}
		if (pixel_y + 1 < buffer.height) {
			queue_adjacent_runs(pixel_y + 1, left, right);
		}
	}
	if constexpr (!MatchColor) {
		bounds->w -= bounds->x - 1;
		bounds->h -= bounds->y - 1;
	}
	return pixels;
}

static inline size_t amiberry_auto_crop_exclude_surface_background(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const uint32_t background_rgb, AmiberryAutoCropScanState& state)
{
	// Exclude only edge-connected regions of the cleared surface color so
	// enclosed pixels of the same color can still be real Amiga content.
	size_t excluded_pixels = 0;
	const AmiberryAutoCropBorderColors background =
		amiberry_auto_crop_single_border(background_rgb);
	const auto exclude_from = [&](const int x, const int y) {
		excluded_pixels += amiberry_auto_crop_flood_region<true>(
			buffer, crop, x, y, state, background, nullptr);
	};

	for (int x = 0; x < buffer.width; x++) {
		exclude_from(x, 0);
		exclude_from(x, buffer.height - 1);
	}
	for (int y = 1; y < buffer.height - 1; y++) {
		exclude_from(0, y);
		exclude_from(buffer.width - 1, y);
	}
	return excluded_pixels;
}

static inline bool amiberry_auto_crop_expand_to_visible_content(
	const AmiberryAutoCropPixelBuffer& buffer, const int min_outside_pixels,
	const bool interlaced, AmiberryAutoCropRect& crop,
	AmiberryAutoCropScanState& state)
{
	const AmiberryAutoCropBorderColors carried_border = state.previous_border;
	state.border = {};
	if (!amiberry_auto_crop_buffer_valid(buffer)
		|| crop.w <= 0 || crop.h <= 0
		|| crop.x < 0 || crop.y < 0
		|| amiberry_auto_crop_rect_right(crop) > buffer.width
		|| amiberry_auto_crop_rect_bottom(crop) > buffer.height) {
		return false;
	}

	uint32_t background_rgb = 0;
	const bool background_valid = amiberry_auto_crop_detect_surface_background_color(
		buffer, crop, background_rgb);
	if (!amiberry_auto_crop_detect_border_color(
		buffer, crop, background_rgb, background_valid, interlaced, state)) {
		return false;
	}
	// The perimeter can be settled while the overscan further out still holds
	// the border of the field before it. Interlace only ever weaves those two
	// generations, so carrying the previous one covers what sampling cannot see.
	for (int i = 0; interlaced && i < carried_border.count && state.border.count < 2; i++) {
		if (!amiberry_auto_crop_border_matches(state.border, carried_border.rgb[i])) {
			state.border.rgb[state.border.count++] = carried_border.rgb[i];
		}
	}
	const AmiberryAutoCropBorderColors border = state.border;
	const size_t required_pixels = static_cast<size_t>(std::max(1, min_outside_pixels));
	// Count first so border-only frames bypass flood-fill and scratch-buffer clearing.
	const size_t visible_pixels = amiberry_auto_crop_count_visible_pixels(
		buffer, crop, border);
	if (visible_pixels < required_pixels) {
		return false;
	}
	const size_t pixel_count = static_cast<size_t>(buffer.width) * buffer.height;
	state.visited.assign(pixel_count, 0);
	state.pending.clear();
	size_t excluded_pixels = 0;
	if (background_valid && !amiberry_auto_crop_border_matches(border, background_rgb)) {
		excluded_pixels = amiberry_auto_crop_exclude_surface_background(
			buffer, crop, background_rgb, state);
	}
	if (visible_pixels < excluded_pixels + required_pixels) {
		return false;
	}
	AmiberryAutoCropRect expanded = crop;
	bool changed = false;

	AmiberryAutoCropRect regions[4];
	amiberry_auto_crop_get_outside_regions(buffer, crop, regions);
	for (const auto& region : regions) {
		for (int y = region.y; y < amiberry_auto_crop_rect_bottom(region); y++) {
			for (int x = region.x; x < amiberry_auto_crop_rect_right(region); x++) {
				const int start_index = y * buffer.width + x;
				if (state.visited[start_index]) {
					continue;
				}
				if (amiberry_auto_crop_border_matches(border,
					amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask)) {
					state.visited[start_index] = 1;
					continue;
				}
				AmiberryAutoCropRect component;
				const size_t component_pixels = amiberry_auto_crop_flood_region<false>(
					buffer, crop, x, y, state, border, &component);
				if (component_pixels < required_pixels) {
					continue;
				}
				const int right = std::max(amiberry_auto_crop_rect_right(expanded),
					amiberry_auto_crop_rect_right(component));
				const int bottom = std::max(amiberry_auto_crop_rect_bottom(expanded),
					amiberry_auto_crop_rect_bottom(component));
				expanded.x = std::min(expanded.x, component.x);
				expanded.y = std::min(expanded.y, component.y);
				expanded.w = right - expanded.x;
				expanded.h = bottom - expanded.y;
				changed = true;
			}
		}
	}

	if (changed) {
		crop = expanded;
	}
	return changed;
}

#endif // AMIBERRY_AUTOCROP_HELPERS_H
