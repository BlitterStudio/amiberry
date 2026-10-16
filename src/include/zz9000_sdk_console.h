/*
 * ZZ9000 SDK console encoder service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_CONSOLE_H
#define AMIBERRY_ZZ9000_SDK_CONSOLE_H

#include <cstdint>
#include <vector>

namespace zz9000_sdk {

class ConsoleEncoder {
public:
	uint16_t encode(const uint8_t *request, uint8_t *pixels, uint32_t surface_width,
	                uint32_t surface_height, uint32_t surface_pitch,
	                uint32_t surface_format, uint8_t *output,
	                uint32_t output_length, uint8_t *reply,
	                uint16_t *reply_length);
	void reset();

private:
	uint32_t owner_ = 0;
	uint32_t width_ = 0, height_ = 0, pitch_ = 0;
	uint32_t flags_ = 0;
	uint8_t depth_ = 0, tile_width_ = 0, tile_height_ = 0, format_ = 0;
	uint16_t sequence_ = 0;
	bool ready_ = false;
	std::vector<uint8_t> snapshot_;
	std::vector<uint8_t> shadow_;
	std::vector<uint8_t> compressed_;
};

} // namespace zz9000_sdk
#endif
