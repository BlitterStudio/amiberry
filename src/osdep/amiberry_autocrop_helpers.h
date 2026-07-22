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
};

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

static inline bool amiberry_auto_crop_expand_to_visible_content(
	const AmiberryAutoCropPixelBuffer& buffer, const int min_outside_pixels,
	AmiberryAutoCropRect& crop, AmiberryAutoCropScanState& state)
{
	if (!amiberry_auto_crop_buffer_valid(buffer)
		|| crop.w <= 0 || crop.h <= 0
		|| crop.x < 0 || crop.y < 0
		|| amiberry_auto_crop_rect_right(crop) > buffer.width
		|| amiberry_auto_crop_rect_bottom(crop) > buffer.height) {
		return false;
	}

	const uint32_t border_rgb = amiberry_auto_crop_read_pixel(buffer, 0, 0)
		& buffer.rgb_mask;
	if (border_rgb != 0) {
		return false;
	}

	const size_t pixel_count = static_cast<size_t>(buffer.width) * buffer.height;
	state.visited.assign(pixel_count, 0);
	state.pending.clear();
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
