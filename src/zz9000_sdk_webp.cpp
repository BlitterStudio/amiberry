/*
 * WebP decoder state for the ZZ9000 SDK image service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "include/zz9000_sdk_webp.h"
#include "include/zz9000_sdk_internal.h"

#include <cstring>
#include <limits>

#ifdef AMIBERRY_WITH_WEBP
#include <webp/decode.h>
#include <webp/demux.h>
#endif

namespace zz9000_sdk {
namespace {
constexpr uint32_t webp_max_input = 8 * 1024 * 1024;
constexpr uint32_t webp_max_side = 8192;
constexpr uint32_t webp_max_pixels = 6 * 1024 * 1024;
constexpr uint32_t webp_max_chunks = 1024;
constexpr uint32_t webp_max_frames = 256;

uint32_t le24(const uint8_t *p)
{
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
		(static_cast<uint32_t>(p[2]) << 16);
}

uint32_t le32(const uint8_t *p)
{
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
		(static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

bool anmf_valid(const uint8_t *data, uint32_t length)
{
	uint32_t offset = 16, chunks = 0;
	bool alpha = false, image = false;
	while (offset < length) {
		if (++chunks > webp_max_chunks || length - offset < 8)
			return false;
		const uint32_t size = le32(data + offset + 4);
		const uint32_t padded = size + (size & 1u);
		if (padded < size || padded > length - offset - 8)
			return false;
		if (!std::memcmp(data + offset, "ALPH", 4)) {
			if (alpha || image || !size)
				return false;
			alpha = true;
		} else if (!std::memcmp(data + offset, "VP8 ", 4)) {
			if (image || size < 10)
				return false;
			image = true;
		} else if (!std::memcmp(data + offset, "VP8L", 4)) {
			if (alpha || image || size < 5)
				return false;
			image = true;
		}
		offset += 8 + padded;
	}
	return offset == length && image;
}

bool preflight(const std::vector<uint8_t> &data, uint32_t &width,
               uint32_t &height, bool &animated)
{
	uint32_t offset = 12, chunks = 0, frames = 0;
	bool vp8x = false, anim = false, anim_header = false, image = false;
	bool alpha = false, iccp = false, exif = false, xmp = false;
	width = height = 0;
	animated = false;
	if (data.size() < 12 || std::memcmp(data.data(), "RIFF", 4) ||
	    std::memcmp(data.data() + 8, "WEBP", 4) ||
	    le32(data.data() + 4) != data.size() - 8)
		return false;
	while (offset < data.size()) {
		if (++chunks > webp_max_chunks || data.size() - offset < 8)
			return false;
		const auto *chunk = data.data() + offset + 8;
		const uint32_t length = le32(data.data() + offset + 4);
		const uint32_t padded = length + (length & 1u);
		if (padded < length || padded > data.size() - offset - 8)
			return false;
		const auto is = [&](const char *name) { return !std::memcmp(data.data() + offset, name, 4); };
		if (is("VP8X")) {
			if (chunks != 1 || vp8x || length != 10)
				return false;
			vp8x = true;
			width = le24(chunk + 4) + 1;
			height = le24(chunk + 7) + 1;
			animated = (chunk[0] & 2) != 0;
		} else if (is("ANIM")) {
			if (!vp8x || !animated || anim_header || frames || length != 6)
				return false;
			anim_header = true;
		} else if (is("ANMF")) {
			if (!animated || !anim_header || image || length < 16 ||
			    ++frames > webp_max_frames || !width || !height)
				return false;
			const uint32_t x = 2 * le24(chunk), y = 2 * le24(chunk + 3);
			const uint32_t fw = le24(chunk + 6) + 1, fh = le24(chunk + 9) + 1;
			if (x > width || y > height || fw > width - x || fh > height - y ||
			    !anmf_valid(chunk, length))
				return false;
		} else if (is("VP8 ") || is("VP8L")) {
			if ((is("VP8 ") && length < 10) || (is("VP8L") && length < 5) ||
			    animated || image)
				return false;
			image = true;
		} else if (is("ALPH")) {
			if (!vp8x || animated || alpha || image || !length)
				return false;
			alpha = true;
		} else if (is("ICCP")) {
			if (!vp8x || iccp || alpha || image || anim_header || frames)
				return false;
			iccp = true;
		} else if (is("EXIF")) {
			if (!vp8x || exif)
				return false;
			exif = true;
		} else if (is("XMP ")) {
			if (!vp8x || xmp)
				return false;
			xmp = true;
		}
		offset += 8 + padded;
	}
	return offset == data.size() && ((animated && anim_header && frames) || (!animated && image));
}

bool dimensions_valid(uint32_t width, uint32_t height)
{
	return width && height && width <= webp_max_side && height <= webp_max_side &&
		width <= webp_max_pixels / height;
}
} // namespace

struct WebpDecoder::Impl {
	std::vector<uint8_t> input;
	uint32_t declared_length = 0;
#ifdef AMIBERRY_WITH_WEBP
	WebPAnimDecoder *animation = nullptr;
#endif

	~Impl()
	{
#ifdef AMIBERRY_WITH_WEBP
		if (animation)
			WebPAnimDecoderDelete(animation);
#endif
	}
};

WebpDecoder::WebpDecoder() : impl_(std::make_unique<Impl>()) {}
WebpDecoder::~WebpDecoder() = default;
WebpDecoder::WebpDecoder(WebpDecoder &&) noexcept = default;
WebpDecoder &WebpDecoder::operator=(WebpDecoder &&) noexcept = default;

WebpInputResult WebpDecoder::append(const uint8_t *data, uint32_t length)
{
	if (!data && length)
		return WebpInputResult::malformed;
	if (length > webp_max_input - impl_->input.size())
		return WebpInputResult::limit;
	try {
		impl_->input.insert(impl_->input.end(), data, data + length);
	} catch (const std::bad_alloc &) {
		return WebpInputResult::no_memory;
	}
	if (!impl_->declared_length && impl_->input.size() >= 12) {
		if (std::memcmp(impl_->input.data(), "RIFF", 4) ||
		    std::memcmp(impl_->input.data() + 8, "WEBP", 4))
			return WebpInputResult::malformed;
		const uint32_t riff_length = le32(impl_->input.data() + 4);
		if (riff_length > std::numeric_limits<uint32_t>::max() - 8 ||
		    riff_length + 8 < 12)
			return WebpInputResult::malformed;
		impl_->declared_length = riff_length + 8;
		if (impl_->declared_length > webp_max_input)
			return WebpInputResult::limit;
	}
	if (impl_->declared_length && impl_->input.size() > impl_->declared_length)
		return WebpInputResult::limit;
	return WebpInputResult::ok;
}

WebpInputResult WebpDecoder::decode_still(uint32_t &width, uint32_t &height,
                                          bool &animated, std::vector<uint8_t> &rgba)
{
	if (!preflight(impl_->input, width, height, animated))
		return WebpInputResult::malformed;
	if ((animated || (width && height)) && !dimensions_valid(width, height))
		return WebpInputResult::limit;
#ifndef AMIBERRY_WITH_WEBP
	return WebpInputResult::malformed;
#else
	HostNearestRounding rounding;
	if (animated) {
		WebPData data{impl_->input.data(), impl_->input.size()};
		WebPAnimDecoderOptions options;
		WebPAnimInfo info;
		if (!WebPAnimDecoderOptionsInit(&options))
			return WebpInputResult::malformed;
		options.color_mode = MODE_RGBA;
		WebPAnimDecoder *decoder = WebPAnimDecoderNew(&data, &options);
		uint8_t *pixels = nullptr;
		int timestamp = 0;
		if (!decoder || !WebPAnimDecoderGetInfo(decoder, &info) ||
		    !WebPAnimDecoderGetNext(decoder, &pixels, &timestamp)) {
			if (decoder)
				WebPAnimDecoderDelete(decoder);
			return WebpInputResult::malformed;
		}
		width = info.canvas_width;
		height = info.canvas_height;
		if (!dimensions_valid(width, height)) {
			WebPAnimDecoderDelete(decoder);
			return WebpInputResult::malformed;
		}
		try {
			rgba.assign(pixels, pixels + static_cast<size_t>(width) * height * 4);
		} catch (const std::bad_alloc &) {
			WebPAnimDecoderDelete(decoder);
			return WebpInputResult::no_memory;
		}
		WebPAnimDecoderDelete(decoder);
		return WebpInputResult::ok;
	}
	WebPBitstreamFeatures features;
	if (WebPGetFeatures(impl_->input.data(), impl_->input.size(), &features) != VP8_STATUS_OK ||
	    features.width <= 0 || features.height <= 0)
		return WebpInputResult::malformed;
	width = static_cast<uint32_t>(features.width);
	height = static_cast<uint32_t>(features.height);
	if (!dimensions_valid(width, height))
		return WebpInputResult::malformed;
	uint8_t *pixels = WebPDecodeRGBA(impl_->input.data(), impl_->input.size(),
	                                 &features.width, &features.height);
	if (!pixels)
		return WebpInputResult::no_memory;
	try {
		rgba.assign(pixels, pixels + static_cast<size_t>(width) * height * 4);
	} catch (const std::bad_alloc &) {
		WebPFree(pixels);
		return WebpInputResult::no_memory;
	}
	WebPFree(pixels);
	return WebpInputResult::ok;
#endif
}

WebpInputResult WebpDecoder::begin_animation(uint32_t &width, uint32_t &height,
                                              uint32_t &loop_count)
{
	bool animated = false;
	if (!preflight(impl_->input, width, height, animated) || !animated)
		return WebpInputResult::malformed;
	if (!dimensions_valid(width, height))
		return WebpInputResult::limit;
#ifndef AMIBERRY_WITH_WEBP
	return WebpInputResult::malformed;
#else
	HostNearestRounding rounding;
	WebPData data{impl_->input.data(), impl_->input.size()};
	WebPAnimDecoderOptions options;
	WebPAnimInfo info;
	if (!WebPAnimDecoderOptionsInit(&options))
		return WebpInputResult::malformed;
	options.color_mode = MODE_RGBA;
	if (impl_->animation) {
		WebPAnimDecoderDelete(impl_->animation);
		impl_->animation = nullptr;
	}
	impl_->animation = WebPAnimDecoderNew(&data, &options);
	if (!impl_->animation || !WebPAnimDecoderGetInfo(impl_->animation, &info))
		return WebpInputResult::malformed;
	width = info.canvas_width;
	height = info.canvas_height;
	loop_count = info.loop_count;
	return dimensions_valid(width, height) ? WebpInputResult::ok : WebpInputResult::malformed;
#endif
}

WebpInputResult WebpDecoder::next_animation(const uint8_t *&rgba, uint32_t &timestamp)
{
#ifndef AMIBERRY_WITH_WEBP
	(void)rgba; (void)timestamp;
	return WebpInputResult::malformed;
#else
	if (!impl_->animation)
		return WebpInputResult::malformed;
	HostNearestRounding rounding;
	uint8_t *pixels = nullptr;
	int time = 0;
	if (!WebPAnimDecoderGetNext(impl_->animation, &pixels, &time))
		return WebpInputResult::malformed;
	rgba = pixels;
	timestamp = time < 0 ? 0 : static_cast<uint32_t>(time);
	return WebpInputResult::ok;
#endif
}

bool WebpDecoder::animation_has_more() const
{
#ifdef AMIBERRY_WITH_WEBP
	return impl_->animation && WebPAnimDecoderHasMoreFrames(impl_->animation);
#else
	return false;
#endif
}

WebpInputResult WebpDecoder::reset_animation()
{
#ifndef AMIBERRY_WITH_WEBP
	return WebpInputResult::malformed;
#else
	if (!impl_->animation)
		return WebpInputResult::malformed;
	HostNearestRounding rounding;
	WebPAnimDecoderReset(impl_->animation);
	return WebpInputResult::ok;
#endif
}

} // namespace zz9000_sdk
