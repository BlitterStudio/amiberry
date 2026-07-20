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

#endif
