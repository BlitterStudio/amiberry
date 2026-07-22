#ifndef AMIBERRY_AUTOCROP_HELPERS_H
#define AMIBERRY_AUTOCROP_HELPERS_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

struct AmiberryAutoCropRect {
	int x;
	int y;
	int w;
	int h;
};

struct AmiberryAutoCropPixelBuffer {
	const uint8_t* pixels;
	int width;
	int height;
	int pitch;
	int bytes_per_pixel;
	uint32_t rgb_mask;
};

struct AmiberryAutoCropScanState {
	std::vector<uint8_t> visited;
	std::vector<int> pending;
	std::vector<uint32_t> border_samples;
	uint32_t border_rgb = 0;
	bool border_valid = false;
};

static inline bool amiberry_auto_crop_border_state_changed(
	const uint32_t previous_rgb, const bool previous_valid,
	const uint32_t current_rgb, const bool current_valid)
{
	return previous_valid != current_valid
		|| (current_valid && previous_rgb != current_rgb);
}

static inline int amiberry_auto_crop_rect_right(const AmiberryAutoCropRect& rect)
{
	return rect.x + rect.w;
}

static inline int amiberry_auto_crop_rect_bottom(const AmiberryAutoCropRect& rect)
{
	return rect.y + rect.h;
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

static inline size_t amiberry_auto_crop_count_visible_pixels(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const uint32_t border_rgb)
{
	AmiberryAutoCropRect regions[4];
	amiberry_auto_crop_get_outside_regions(buffer, crop, regions);
	size_t visible_pixels = 0;
	for (const auto& region : regions) {
		const int region_bottom = amiberry_auto_crop_rect_bottom(region);
		if (buffer.bytes_per_pixel == static_cast<int>(sizeof(uint32_t))) { // Vectorized SDL path.
			for (int y = region.y; y < region_bottom; y++) {
				const uint8_t* pixel = buffer.pixels + y * buffer.pitch
					+ region.x * sizeof(uint32_t);
				for (int x = 0; x < region.w; x++, pixel += sizeof(uint32_t)) {
					uint32_t value;
					std::memcpy(&value, pixel, sizeof(value));
					visible_pixels += (value & buffer.rgb_mask) != border_rgb;
				}
			}
		} else {
			for (int y = region.y; y < region_bottom; y++) {
				for (int x = region.x; x < amiberry_auto_crop_rect_right(region); x++) {
					visible_pixels += (amiberry_auto_crop_read_pixel(buffer, x, y)
						& buffer.rgb_mask) != border_rgb;
				}
			}
		}
	}
	return visible_pixels;
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
	AmiberryAutoCropScanState& state)
{
	state.border_samples.clear();
	state.border_valid = false;
	const int right = amiberry_auto_crop_rect_right(crop);
	const int bottom = amiberry_auto_crop_rect_bottom(crop);
	const int horizontal_step = std::max(1, crop.w / 64);
	const int vertical_step = std::max(1, crop.h / 64);
	uint32_t side_rgb[4];
	int sampled_sides = 0;
	int side_color_count = 0;
	const auto sample_side = [&](const int x, const int y, const int dx, const int dy,
		const int length, const int step) {
		for (int offset = 0; offset < length; offset += step) {
			state.border_samples.push_back(amiberry_auto_crop_read_pixel(
				buffer, x + dx * offset, y + dy * offset) & buffer.rgb_mask);
		}
		uint32_t dominant_rgb;
		sampled_sides++;
		const size_t dominant_count = amiberry_auto_crop_find_dominant_color(
			state.border_samples.data(), state.border_samples.size(), dominant_rgb);
		if (dominant_count * 4 >= state.border_samples.size() * 3) {
			side_rgb[side_color_count++] = dominant_rgb;
		}
		state.border_samples.clear();
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
		uint32_t most_common;
		const size_t most_common_sides = amiberry_auto_crop_find_dominant_color(
			side_rgb, side_color_count, most_common);
		// Give each side one vote and require 75% confidence before hiding it.
		if (most_common_sides * 4 >= static_cast<size_t>(sampled_sides) * 3) {
			state.border_rgb = most_common;
			state.border_valid = true;
			return true;
		}
	}
	// One or ambiguous sides may be content; fall back to the cleared surface.
	state.border_rgb = background_rgb;
	state.border_valid = background_valid;
	return background_valid;
}

template<bool MatchColor>
static inline size_t amiberry_auto_crop_flood_region(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const int start_x, const int start_y, AmiberryAutoCropScanState& state,
	const uint32_t rgb, AmiberryAutoCropRect* bounds)
{
	if (amiberry_auto_crop_rect_contains(crop, start_x, start_y)) {
		return 0;
	}
	const int start_index = start_y * buffer.width + start_x;
	if (state.visited[start_index]) {
		return 0;
	}
	const bool start_matches = ((amiberry_auto_crop_read_pixel(
		buffer, start_x, start_y) & buffer.rgb_mask) == rgb) == MatchColor;
	if (!start_matches) {
		if constexpr (!MatchColor) {
			state.visited[start_index] = 1;
		}
		return 0;
	}
	state.visited[start_index] = 1;
	state.pending.push_back(start_index);
	size_t pixels = 0;
	if constexpr (!MatchColor) {
		*bounds = { start_x, start_y, start_x, start_y };
	}
	while (!state.pending.empty()) {
		const int index = state.pending.back();
		state.pending.pop_back();
		const int pixel_x = index % buffer.width;
		const int pixel_y = index / buffer.width;
		pixels++;
		if constexpr (!MatchColor) {
			bounds->x = std::min(bounds->x, pixel_x);
			bounds->y = std::min(bounds->y, pixel_y);
			bounds->w = std::max(bounds->w, pixel_x);
			bounds->h = std::max(bounds->h, pixel_y);
		}
		for (int neighbor_y = std::max(0, pixel_y - 1);
			neighbor_y <= std::min(buffer.height - 1, pixel_y + 1); neighbor_y++) {
			for (int neighbor_x = std::max(0, pixel_x - 1);
				neighbor_x <= std::min(buffer.width - 1, pixel_x + 1); neighbor_x++) {
				if (amiberry_auto_crop_rect_contains(crop, neighbor_x, neighbor_y)) {
					continue;
				}
				const int neighbor_index = neighbor_y * buffer.width + neighbor_x;
				if (state.visited[neighbor_index]) {
					continue;
				}
				const bool matches = ((amiberry_auto_crop_read_pixel(
					buffer, neighbor_x, neighbor_y) & buffer.rgb_mask) == rgb) == MatchColor;
				if (matches) {
					state.visited[neighbor_index] = 1;
					state.pending.push_back(neighbor_index);
				} else if constexpr (!MatchColor) {
					state.visited[neighbor_index] = 1;
				}
			}
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
	const auto exclude_from = [&](const int x, const int y) {
		excluded_pixels += amiberry_auto_crop_flood_region<true>(
			buffer, crop, x, y, state, background_rgb, nullptr);
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
	AmiberryAutoCropRect& crop, AmiberryAutoCropScanState& state)
{
	state.border_valid = false;
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
		buffer, crop, background_rgb, background_valid, state)) {
		return false;
	}
	const uint32_t border_rgb = state.border_rgb;
	const size_t required_pixels = static_cast<size_t>(std::max(1, min_outside_pixels));
	// Count first so border-only frames bypass flood-fill and scratch-buffer clearing.
	const size_t visible_pixels = amiberry_auto_crop_count_visible_pixels(
		buffer, crop, border_rgb);
	if (visible_pixels < required_pixels) {
		return false;
	}
	const size_t pixel_count = static_cast<size_t>(buffer.width) * buffer.height;
	state.visited.assign(pixel_count, 0);
	state.pending.clear();
	size_t excluded_pixels = 0;
	if (background_valid && background_rgb != border_rgb) {
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
				AmiberryAutoCropRect component;
				const size_t component_pixels = amiberry_auto_crop_flood_region<false>(
					buffer, crop, x, y, state, border_rgb, &component);
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
