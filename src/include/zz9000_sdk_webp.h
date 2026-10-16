/*
 * WebP decoder state for the ZZ9000 SDK image service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_WEBP_H
#define AMIBERRY_ZZ9000_SDK_WEBP_H

#include <cstdint>
#include <memory>
#include <vector>

namespace zz9000_sdk {

enum class WebpInputResult {
	ok,
	malformed,
	limit,
	no_memory,
};

class WebpDecoder {
public:
	WebpDecoder();
	~WebpDecoder();
	WebpDecoder(WebpDecoder &&) noexcept;
	WebpDecoder &operator=(WebpDecoder &&) noexcept;
	WebpDecoder(const WebpDecoder &) = delete;
	WebpDecoder &operator=(const WebpDecoder &) = delete;

	WebpInputResult append(const uint8_t *data, uint32_t length);
	WebpInputResult decode_still(uint32_t &width, uint32_t &height,
	                             bool &animated, std::vector<uint8_t> &rgba);
	WebpInputResult begin_animation(uint32_t &width, uint32_t &height,
	                                uint32_t &loop_count);
	WebpInputResult next_animation(const uint8_t *&rgba, uint32_t &timestamp);
	bool animation_has_more() const;
	WebpInputResult reset_animation();

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace zz9000_sdk

#endif
