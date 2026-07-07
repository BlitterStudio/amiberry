#ifndef LIBRETRO_CROP_HELPERS_H
#define LIBRETRO_CROP_HELPERS_H

#include <algorithm>
#include <cstdint>
#include <cstring>

struct LibretroCropRect {
	int x;
	int y;
	int w;
	int h;
};

struct LibretroCropPixelBuffer {
	const uint8_t* pixels;
	int width;
	int height;
	int pitch;
	int bytes_per_pixel;
	uint32_t rgb_mask;
};

inline int libretro_crop_rect_right(const LibretroCropRect& rect)
{
	return rect.x + rect.w;
}

inline int libretro_crop_rect_bottom(const LibretroCropRect& rect)
{
	return rect.y + rect.h;
}

inline bool libretro_crop_rect_valid(const LibretroCropRect& rect)
{
	return rect.w > 0 && rect.h > 0;
}

inline bool libretro_crop_rect_equals(const LibretroCropRect& a, const LibretroCropRect& b)
{
	return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

inline bool libretro_crop_buffer_valid(const LibretroCropPixelBuffer& buffer)
{
	return buffer.pixels
		&& buffer.width > 0
		&& buffer.height > 0
		&& buffer.pitch > 0
		&& buffer.bytes_per_pixel > 0
		&& buffer.bytes_per_pixel <= static_cast<int>(sizeof(uint32_t))
		&& buffer.rgb_mask != 0;
}

inline uint32_t libretro_crop_buffer_pixel(const LibretroCropPixelBuffer& buffer,
	const int x, const int y)
{
	uint32_t value = 0;
	const uint8_t* pixel = buffer.pixels + y * buffer.pitch + x * buffer.bytes_per_pixel;
	std::memcpy(&value, pixel, buffer.bytes_per_pixel);
	return value;
}

inline bool libretro_crop_rect_contains_point(const LibretroCropRect& rect,
	const int x, const int y)
{
	return x >= rect.x && x < libretro_crop_rect_right(rect)
		&& y >= rect.y && y < libretro_crop_rect_bottom(rect);
}

inline bool libretro_crop_rect_fits_buffer(const LibretroCropPixelBuffer& buffer,
	const LibretroCropRect& rect)
{
	return libretro_crop_buffer_valid(buffer)
		&& libretro_crop_rect_valid(rect)
		&& rect.x >= 0
		&& rect.y >= 0
		&& libretro_crop_rect_right(rect) <= buffer.width
		&& libretro_crop_rect_bottom(rect) <= buffer.height;
}

inline void libretro_crop_clamp_rect(const int surface_w, const int surface_h,
	LibretroCropRect& rect)
{
	if (surface_w <= 0 || surface_h <= 0 || rect.w <= 0 || rect.h <= 0) {
		rect = {};
		return;
	}

	rect.w = std::min(rect.w, surface_w);
	rect.h = std::min(rect.h, surface_h);
	rect.x = std::clamp(rect.x, 0, surface_w - rect.w);
	rect.y = std::clamp(rect.y, 0, surface_h - rect.h);
}

inline bool libretro_crop_find_visible_content_bounds(const LibretroCropPixelBuffer& buffer,
	const uint32_t border_color, LibretroCropRect& bounds)
{
	if (!libretro_crop_buffer_valid(buffer))
		return false;

	const uint32_t border_rgb = border_color & buffer.rgb_mask;
	int min_x = buffer.width;
	int min_y = buffer.height;
	int max_x = -1;
	int max_y = -1;

	for (int y = 0; y < buffer.height; y++) {
		for (int x = 0; x < buffer.width; x++) {
			if ((libretro_crop_buffer_pixel(buffer, x, y) & buffer.rgb_mask) == border_rgb)
				continue;
			min_x = std::min(min_x, x);
			min_y = std::min(min_y, y);
			max_x = std::max(max_x, x);
			max_y = std::max(max_y, y);
		}
	}

	if (max_x < min_x || max_y < min_y)
		return false;

	bounds = { min_x, min_y, max_x - min_x + 1, max_y - min_y + 1 };
	return true;
}

inline bool libretro_crop_expand_to_visible_content(const LibretroCropPixelBuffer& buffer,
	const uint32_t border_color, const int min_outside_pixels, LibretroCropRect& crop)
{
	if (!libretro_crop_rect_fits_buffer(buffer, crop))
		return false;

	const uint32_t border_rgb = border_color & buffer.rgb_mask;
	const int required_pixels = std::max(1, min_outside_pixels);
	int outside_pixels = 0;
	int min_x = crop.x;
	int min_y = crop.y;
	int max_x = libretro_crop_rect_right(crop) - 1;
	int max_y = libretro_crop_rect_bottom(crop) - 1;

	for (int y = 0; y < buffer.height; y++) {
		for (int x = 0; x < buffer.width; x++) {
			if (libretro_crop_rect_contains_point(crop, x, y))
				continue;
			if ((libretro_crop_buffer_pixel(buffer, x, y) & buffer.rgb_mask) == border_rgb)
				continue;

			outside_pixels++;
			min_x = std::min(min_x, x);
			min_y = std::min(min_y, y);
			max_x = std::max(max_x, x);
			max_y = std::max(max_y, y);
		}
	}

	if (outside_pixels < required_pixels)
		return false;

	LibretroCropRect expanded = { min_x, min_y, max_x - min_x + 1, max_y - min_y + 1 };
	libretro_crop_clamp_rect(buffer.width, buffer.height, expanded);
	if (libretro_crop_rect_equals(expanded, crop))
		return false;

	crop = expanded;
	return true;
}

inline bool libretro_crop_fit_rect_to_visible_content(const int surface_w, const int surface_h,
	const LibretroCropRect& content, LibretroCropRect& crop)
{
	if (surface_w <= 0 || surface_h <= 0
		|| !libretro_crop_rect_valid(content)
		|| !libretro_crop_rect_valid(crop)) {
		return false;
	}

	LibretroCropRect fitted = crop;
	libretro_crop_clamp_rect(surface_w, surface_h, fitted);

	LibretroCropRect clamped_content = content;
	libretro_crop_clamp_rect(surface_w, surface_h, clamped_content);
	if (!libretro_crop_rect_valid(clamped_content))
		return false;

	if (clamped_content.w > fitted.w) {
		fitted.x = clamped_content.x + clamped_content.w / 2 - fitted.w / 2;
	} else {
		if (clamped_content.x < fitted.x)
			fitted.x = clamped_content.x;
		if (libretro_crop_rect_right(clamped_content) > libretro_crop_rect_right(fitted))
			fitted.x = libretro_crop_rect_right(clamped_content) - fitted.w;
	}

	if (clamped_content.h > fitted.h) {
		fitted.y = clamped_content.y + clamped_content.h / 2 - fitted.h / 2;
	} else {
		if (clamped_content.y < fitted.y)
			fitted.y = clamped_content.y;
		if (libretro_crop_rect_bottom(clamped_content) > libretro_crop_rect_bottom(fitted))
			fitted.y = libretro_crop_rect_bottom(clamped_content) - fitted.h;
	}

	libretro_crop_clamp_rect(surface_w, surface_h, fitted);
	if (libretro_crop_rect_equals(fitted, crop))
		return false;

	crop = fitted;
	return true;
}

#endif
