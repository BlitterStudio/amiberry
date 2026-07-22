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

static inline bool amiberry_auto_crop_pixel_is_visible(
	const AmiberryAutoCropPixelBuffer& buffer, const int x, const int y,
	const uint32_t border_rgb)
{
	return (amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask)
		!= border_rgb;
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
	if (corner_count == 0) {
		return false;
	}

	background_rgb = corner_rgb[0];
	int background_count = 0;
	for (int i = 0; i < corner_count; i++) {
		int matches = 0;
		for (int j = 0; j < corner_count; j++) {
			matches += corner_rgb[i] == corner_rgb[j];
		}
		if (matches > background_count) {
			background_rgb = corner_rgb[i];
			background_count = matches;
		}
	}
	return true;
}

static inline bool amiberry_auto_crop_find_dominant_color(
	std::vector<uint32_t>& samples, uint32_t& dominant_rgb)
{
	if (samples.empty()) {
		return false;
	}
	std::sort(samples.begin(), samples.end());
	dominant_rgb = samples.front();
	size_t dominant_count = 1;
	size_t run_start = 0;
	for (size_t i = 1; i <= samples.size(); i++) {
		if (i < samples.size() && samples[i] == samples[run_start]) {
			continue;
		}
		const size_t run_count = i - run_start;
		if (run_count > dominant_count) {
			dominant_rgb = samples[run_start];
			dominant_count = run_count;
		}
		run_start = i;
	}
	return dominant_count * 4 >= samples.size() * 3;
}

static inline bool amiberry_auto_crop_detect_border_color(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
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
	const auto add_sample = [&](const int x, const int y) {
		state.border_samples.push_back(
			amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask);
	};
	const auto finish_side = [&]() {
		uint32_t dominant_rgb;
		sampled_sides++;
		if (amiberry_auto_crop_find_dominant_color(
			state.border_samples, dominant_rgb)) {
			side_rgb[side_color_count++] = dominant_rgb;
		}
		state.border_samples.clear();
	};

	if (crop.y > 0) {
		for (int x = crop.x; x < right; x += horizontal_step) {
			add_sample(x, crop.y - 1);
		}
		finish_side();
	}
	if (bottom < buffer.height) {
		for (int x = crop.x; x < right; x += horizontal_step) {
			add_sample(x, bottom);
		}
		finish_side();
	}
	if (crop.x > 0) {
		for (int y = crop.y; y < bottom; y += vertical_step) {
			add_sample(crop.x - 1, y);
		}
		finish_side();
	}
	if (right < buffer.width) {
		for (int y = crop.y; y < bottom; y += vertical_step) {
			add_sample(right, y);
		}
		finish_side();
	}

	if (sampled_sides == 0) {
		return false;
	}
	const auto use_surface_background = [&]() {
		if (!amiberry_auto_crop_detect_surface_background_color(
			buffer, crop, state.border_rgb)) {
			return false;
		}
		state.border_valid = true;
		return true;
	};
	// A single uniform edge may be real overscan content. Without a second
	// side to corroborate it, use the cleared surface background instead.
	if (sampled_sides < 2) {
		return use_surface_background();
	}

	uint32_t most_common = 0;
	int most_common_sides = 0;
	bool tied = false;
	for (int i = 0; i < side_color_count; i++) {
		int matches = 0;
		for (int j = 0; j < side_color_count; j++) {
			matches += side_rgb[i] == side_rgb[j];
		}
		if (matches > most_common_sides) {
			most_common = side_rgb[i];
			most_common_sides = matches;
			tied = false;
		} else if (matches == most_common_sides && side_rgb[i] != most_common) {
			tied = true;
		}
	}

	// Give each side one vote so a long content edge cannot outweigh a
	// consistent border on multiple shorter edges.
	if (most_common_sides < 2 || tied) {
		return use_surface_background();
	}
	state.border_rgb = most_common;
	state.border_valid = true;
	return true;
}

static inline void amiberry_auto_crop_exclude_surface_background(
	const AmiberryAutoCropPixelBuffer& buffer, const AmiberryAutoCropRect& crop,
	const uint32_t border_rgb, AmiberryAutoCropScanState& state)
{
	// Pixels outside the emulated raster are cleared to an outside corner color.
	// Exclude only edge-connected regions of that exact color so real content
	// in another color can still extend all the way to a surface edge.
	uint32_t background_rgb;
	if (!amiberry_auto_crop_detect_surface_background_color(
		buffer, crop, background_rgb) || background_rgb == border_rgb) {
		return;
	}

	const auto add_background_pixel = [&](const int x, const int y) {
		if (amiberry_auto_crop_rect_contains(crop, x, y)) {
			return;
		}
		const int index = y * buffer.width + x;
		if (state.visited[index]
			|| (amiberry_auto_crop_read_pixel(buffer, x, y) & buffer.rgb_mask)
				!= background_rgb) {
			return;
		}
		state.visited[index] = 1;
		state.pending.push_back(index);
	};

	for (int x = 0; x < buffer.width; x++) {
		add_background_pixel(x, 0);
		add_background_pixel(x, buffer.height - 1);
	}
	for (int y = 1; y < buffer.height - 1; y++) {
		add_background_pixel(0, y);
		add_background_pixel(buffer.width - 1, y);
	}

	while (!state.pending.empty()) {
		const int index = state.pending.back();
		state.pending.pop_back();
		const int pixel_x = index % buffer.width;
		const int pixel_y = index / buffer.width;
		for (int neighbor_y = std::max(0, pixel_y - 1);
			neighbor_y <= std::min(buffer.height - 1, pixel_y + 1); neighbor_y++) {
			for (int neighbor_x = std::max(0, pixel_x - 1);
				neighbor_x <= std::min(buffer.width - 1, pixel_x + 1); neighbor_x++) {
				add_background_pixel(neighbor_x, neighbor_y);
			}
		}
	}
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

	if (!amiberry_auto_crop_detect_border_color(buffer, crop, state)) {
		return false;
	}
	const uint32_t border_rgb = state.border_rgb;

	const size_t pixel_count = static_cast<size_t>(buffer.width) * buffer.height;
	state.visited.assign(pixel_count, 0);
	state.pending.clear();
	amiberry_auto_crop_exclude_surface_background(buffer, crop, border_rgb, state);
	const int required_pixels = std::max(1, min_outside_pixels);
	AmiberryAutoCropRect expanded = crop;
	bool changed = false;

	for (int y = 0; y < buffer.height; y++) {
		for (int x = 0; x < buffer.width; x++) {
			if (amiberry_auto_crop_rect_contains(crop, x, y)) {
				continue;
			}

			const int start_index = y * buffer.width + x;
			if (state.visited[start_index]) {
				continue;
			}
			state.visited[start_index] = 1;
			if (!amiberry_auto_crop_pixel_is_visible(buffer, x, y, border_rgb)) {
				continue;
			}

			state.pending.push_back(start_index);
			int component_pixels = 0;
			int component_min_x = x;
			int component_min_y = y;
			int component_max_x = x;
			int component_max_y = y;
			while (!state.pending.empty()) {
				const int index = state.pending.back();
				state.pending.pop_back();
				const int pixel_x = index % buffer.width;
				const int pixel_y = index / buffer.width;
				component_pixels++;
				component_min_x = std::min(component_min_x, pixel_x);
				component_min_y = std::min(component_min_y, pixel_y);
				component_max_x = std::max(component_max_x, pixel_x);
				component_max_y = std::max(component_max_y, pixel_y);

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
						state.visited[neighbor_index] = 1;
						if (amiberry_auto_crop_pixel_is_visible(
							buffer, neighbor_x, neighbor_y, border_rgb)) {
							state.pending.push_back(neighbor_index);
						}
					}
				}
			}

			if (component_pixels < required_pixels) {
				continue;
			}
			const int right = std::max(amiberry_auto_crop_rect_right(expanded),
				component_max_x + 1);
			const int bottom = std::max(amiberry_auto_crop_rect_bottom(expanded),
				component_max_y + 1);
			expanded.x = std::min(expanded.x, component_min_x);
			expanded.y = std::min(expanded.y, component_min_y);
			expanded.w = right - expanded.x;
			expanded.h = bottom - expanded.y;
			changed = true;
		}
	}

	if (changed) {
		crop = expanded;
	}
	return changed;
}

#endif // AMIBERRY_AUTOCROP_HELPERS_H
