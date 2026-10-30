#ifndef AMIBERRY_CURSOR_H
#define AMIBERRY_CURSOR_H

#include <algorithm>
#include <cmath>
#include <cstdint>

static inline std::uint32_t amiberry_cursor_rgb12_to_rgb24(std::uint16_t color)
{
	const std::uint32_t r = (color >> 8) & 0x0f;
	const std::uint32_t g = (color >> 4) & 0x0f;
	const std::uint32_t b = color & 0x0f;

	return ((r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b);
}

static inline std::uint32_t amiberry_cursor_rgba32_from_rgb24(std::uint32_t rgb)
{
	const std::uint32_t r = (rgb >> 16) & 0xff;
	const std::uint32_t g = (rgb >> 8) & 0xff;
	const std::uint32_t b = rgb & 0xff;

#ifdef WORDS_BIGENDIAN
	return (r << 24) | (g << 16) | (b << 8) | 0xff;
#else
	return 0xff000000 | (b << 16) | (g << 8) | r;
#endif
}

static inline int amiberry_cursor_hotspot_from_p96_offset(std::uint8_t raw_pointer_offset, int extent)
{
	if (extent <= 0) {
		return 0;
	}

	// P96 stores this displacement in an unsigned BoardInfo byte, but applies
	// it as a signed BYTE when positioning the sprite. SDL instead expects the
	// hotspot's positive offset from the cursor bitmap's top-left corner.
	const int pointer_offset = raw_pointer_offset & 0x80
		? static_cast<int>(raw_pointer_offset) - 0x100
		: static_cast<int>(raw_pointer_offset);
	const int hotspot_offset = -pointer_offset;
	if (hotspot_offset < 0) {
		return 0;
	}
	if (hotspot_offset >= extent) {
		return extent - 1;
	}
	return hotspot_offset;
}

static inline int amiberry_cursor_hotspot_from_zz9000_offset(int pointer_offset, int extent)
{
	if (extent <= 0) {
		return 0;
	}

	// ZZ9000 stores the pointer-to-sprite displacement as hotspot + 1.
	const int hotspot_offset = pointer_offset - 1;
	if (hotspot_offset < 0) {
		return 0;
	}
	if (hotspot_offset >= extent) {
		return extent - 1;
	}
	return hotspot_offset;
}

static inline bool amiberry_cursor_p96_hotspot_needs_tracking(
	std::uint8_t raw_x_offset, std::uint8_t raw_y_offset)
{
	// A non-zero BoardInfo displacement is authoritative. Live position
	// sampling is only a fallback for P96 versions that omit both offsets.
	return raw_x_offset == 0 && raw_y_offset == 0;
}

static inline bool amiberry_cursor_host_only_enabled(int input_tablet,
	int input_magic_mouse_cursor, int host_only_cursor_value, bool host_cursor_available)
{
	// Cursor visibility is independent of the selected mouse-untrap method.
	return host_cursor_available
		&& input_tablet > 0
		&& input_magic_mouse_cursor == host_only_cursor_value;
}

static inline bool amiberry_cursor_rtg_needs_separate_sprite(bool rtg_hardware_sprite, bool host_only_cursor)
{
	return rtg_hardware_sprite || host_only_cursor;
}

static inline std::uint16_t amiberry_cursor_rtg_softsprite_fallback_mask()
{
	return 0;
}

// Host cursor image laid out to match the size the Amiga display presents its
// pointer at. width/height and the hotspot are in host cursor units (window
// coordinates); pixel_width/pixel_height size the backing image for HiDPI
// windows and equal width/height at a pixel density of 1.
struct AmiberryCursorHostImage
{
	int width;
	int height;
	int hot_x;
	int hot_y;
	int pixel_width;
	int pixel_height;
};

static inline bool amiberry_cursor_host_image_equal(
	const AmiberryCursorHostImage& a, const AmiberryCursorHostImage& b)
{
	return a.width == b.width && a.height == b.height
		&& a.hot_x == b.hot_x && a.hot_y == b.hot_y
		&& a.pixel_width == b.pixel_width && a.pixel_height == b.pixel_height;
}

// Nearest-neighbour source pixel for destination pixel `dst`.
static inline int amiberry_cursor_scaled_source_index(int dst, int dst_extent, int src_extent)
{
	return dst * src_extent / dst_extent;
}

// Centre of the destination run that samples source pixel `hot`, so the
// hotspot stays on the pixel the Amiga treats as the pointer position.
// Requires dst_extent >= src_extent, which keeps every run non-empty.
static inline int amiberry_cursor_scaled_hotspot(int hot, int src_extent, int dst_extent)
{
	const int first = (hot * dst_extent + src_extent - 1) / src_extent;
	const int end = ((hot + 1) * dst_extent + src_extent - 1) / src_extent;
	return std::clamp((first + end - 1) / 2, 0, dst_extent - 1);
}

static inline int amiberry_cursor_scaled_extent(int extent, float scale)
{
	return std::max(extent, static_cast<int>(std::lround(extent * scale)));
}

// scale_x/scale_y: host cursor units per source cursor pixel. The cursor never
// shrinks below 1:1, and both the cursor and its backing image are capped so
// they stay within what host cursor implementations accept.
static inline AmiberryCursorHostImage amiberry_cursor_host_image(int src_width, int src_height,
	int src_hot_x, int src_hot_y, float scale_x, float scale_y, float pixel_density)
{
	if (src_width <= 0 || src_height <= 0) {
		return {};
	}
	constexpr float max_scale = 8.0f;
	constexpr int max_pixel_extent = 512;
	const auto clamp_scale = [](float scale, float max) {
		return scale >= 1.0f ? std::min(scale, max) : 1.0f;
	};
	const float sx = clamp_scale(scale_x, max_scale);
	const float sy = clamp_scale(scale_y, max_scale);
	const float density = clamp_scale(pixel_density, max_scale);
	const int largest = std::max(src_width, src_height);
	const float pixel_limit = static_cast<float>(max_pixel_extent) / largest;

	AmiberryCursorHostImage image;
	image.width = amiberry_cursor_scaled_extent(src_width, sx);
	image.height = amiberry_cursor_scaled_extent(src_height, sy);
	image.hot_x = amiberry_cursor_scaled_hotspot(src_hot_x, src_width, image.width);
	image.hot_y = amiberry_cursor_scaled_hotspot(src_hot_y, src_height, image.height);
	image.pixel_width = std::max(image.width,
		amiberry_cursor_scaled_extent(src_width, std::min(sx * density, pixel_limit)));
	image.pixel_height = std::max(image.height,
		amiberry_cursor_scaled_extent(src_height, std::min(sy * density, pixel_limit)));
	return image;
}

#endif
