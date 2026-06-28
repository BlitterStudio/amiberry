#ifndef AMIBERRY_CURSOR_H
#define AMIBERRY_CURSOR_H

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

static inline bool amiberry_cursor_host_only_enabled(int input_tablet,
	int input_mouse_untrap, int magic_mouse_untrap_value,
	int input_magic_mouse_cursor, int host_only_cursor_value)
{
	return input_tablet > 0
		&& (input_mouse_untrap & magic_mouse_untrap_value) != 0
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

#endif
