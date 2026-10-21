/*
 * Helpers shared by the ZZ9000 SDK mailbox translation units.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_INTERNAL_H
#define AMIBERRY_ZZ9000_SDK_INTERNAL_H

#include <cfenv>
#include <chrono>
#include <cstdint>

namespace zz9000_sdk {

constexpr uint16_t ok = 0;
constexpr uint16_t busy = 2;
constexpr uint16_t unsupported = 3;
constexpr uint16_t bad_request = 4;
constexpr uint16_t bad_handle = 5;
constexpr uint16_t no_memory = 6;
constexpr uint16_t io_error = 9;
constexpr uint16_t not_found = 10;
constexpr uint32_t pcm_s16le = 1;
constexpr uint32_t pcm_s16be = 2;
constexpr uint32_t pcm_s32le = 3;
constexpr uint32_t pcm_s32be = 4;
constexpr uint32_t host_window_begin = 0x3e0000;
constexpr uint32_t host_window_end = 0x3f0000;
constexpr uint32_t z3_heap_begin = 0x06000000;
constexpr uint32_t z3_heap_end = 0x07e00000;
constexpr uint32_t max_image_input = 16 * 1024 * 1024;
constexpr uint32_t max_image_pixels = 24 * 1024 * 1024;
constexpr uint32_t framebuffer_handle = 0x80000000;
constexpr uint32_t surface_handle_base = 0x40000000;
constexpr uint32_t max_surface_bytes = 32 * 1024 * 1024;

inline uint32_t pixel_bytes(uint32_t format)
{
	return format == 7 ? 4 : (format == 1 || format == 6 ? 2 :
		(format == 4 ? 1 : 0));
}

inline bool rect_valid(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
	                   uint32_t bound_width, uint32_t bound_height)
{
	return width && height && x <= bound_width && y <= bound_height &&
		width <= bound_width - x && height <= bound_height - y;
}

inline void write_pixel(uint8_t *dst, uint32_t format, uint8_t r, uint8_t g,
                    uint8_t b, uint8_t a = 255)
{
	if (format == 4) {
		// INDEX8 stores the pen in the low byte, as the card's
		// sdk_surface_put_pixel() does.
		dst[0] = b;
	} else if (format == 7) {
		dst[0] = b; dst[1] = g; dst[2] = r; dst[3] = a;
	} else {
		const uint16_t value = format == 1 ?
			static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)) :
			static_cast<uint16_t>(((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
		dst[0] = static_cast<uint8_t>(value >> 8);
		dst[1] = static_cast<uint8_t>(value);
	}
}

// Guest FPCR can change host rounding; host decoders expect nearest during
// setup and decode. Restore the guest mode before returning to CPU emulation.
class HostNearestRounding {
public:
	HostNearestRounding() : previous_(std::fegetround())
	{
		std::fesetround(FE_TONEAREST);
	}

	~HostNearestRounding()
	{
		if (previous_ >= 0)
			std::fesetround(previous_);
	}

	HostNearestRounding(const HostNearestRounding &) = delete;
	HostNearestRounding &operator=(const HostNearestRounding &) = delete;

private:
	int previous_;
};

inline uint16_t be16(const uint8_t *p)
{
	return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

inline uint32_t be32(const uint8_t *p)
{
	return (static_cast<uint32_t>(be16(p)) << 16) | be16(p + 2);
}

inline void put16(uint8_t *p, uint16_t value)
{
	p[0] = static_cast<uint8_t>(value >> 8);
	p[1] = static_cast<uint8_t>(value);
}

inline void put32(uint8_t *p, uint32_t value)
{
	put16(p, static_cast<uint16_t>(value >> 16));
	put16(p + 2, static_cast<uint16_t>(value));
}

inline void put64(uint8_t *p, uint64_t value)
{
	put32(p, static_cast<uint32_t>(value >> 32));
	put32(p + 4, static_cast<uint32_t>(value));
}

inline uint32_t align_up(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

// Monotonic host clock in microseconds, used by the media pipeline profiling
// stages reported on status page 5.
inline uint64_t monotonic_us()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
}

// Mirrors the card's SDK_MEDIA_PACK_PAIR: two 16-bit geometry facts per
// 64-bit status value. Signed coordinates travel as their two's-complement
// pattern and are sign-extended by the reader (ZZ9K_MEDIA_PAIR_HI_S).
inline uint64_t pack_pair(int32_t hi, int32_t lo)
{
	return (static_cast<uint64_t>(static_cast<uint16_t>(hi)) << 16) |
		static_cast<uint16_t>(lo);
}

} // namespace zz9000_sdk

#endif
