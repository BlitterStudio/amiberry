/*
 * ZZ9000 SDK console encoder service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "zz9000_sdk_console.h"
#include "zz9000_sdk_internal.h"

#include <algorithm>
#include <cstring>
#include <zlib.h>

namespace zz9000_sdk {
namespace {

constexpr uint8_t rfb_version = 1;
constexpr uint8_t rfb_op_end = 0;
constexpr uint8_t rfb_op_tile8 = 3;
constexpr uint8_t rfb_code_raw = 0;
constexpr uint8_t rfb_format_clut8 = 1;
constexpr uint8_t rfb_format_rgb565 = 2;
constexpr uint32_t console_reset = 1;
constexpr uint32_t console_keyframe = 1;
constexpr uint16_t internal_error = 0xffff;


} // namespace

void ConsoleEncoder::reset()
{
	owner_ = 0;
	width_ = height_ = pitch_ = flags_ = 0;
	depth_ = tile_width_ = tile_height_ = format_ = 0;
	sequence_ = 0;
	ready_ = false;
	std::vector<uint8_t>().swap(snapshot_);
	std::vector<uint8_t>().swap(shadow_);
	std::vector<uint8_t>().swap(compressed_);
}

uint16_t ConsoleEncoder::encode(const uint8_t *request, uint8_t *pixels,
	uint32_t surface_width, uint32_t surface_height, uint32_t surface_pitch,
	uint32_t surface_format, uint8_t *output, uint32_t output_length,
	uint8_t *reply, uint16_t *reply_length)
{
	const uint32_t owner = be32(request + 4);
	const uint32_t requested_capacity = be32(request + 8);
	const uint32_t flags = be32(request + 12);
	const uint32_t width = be16(request + 16);
	const uint32_t height = be16(request + 18);
	const uint32_t pitch = be16(request + 20);
	const uint32_t ty0 = be16(request + 22);
	const uint32_t ty1 = be16(request + 24);
	const uint16_t request_flags = be16(request + 26);
	const uint8_t depth = request[30];
	const uint8_t tile_width = request[31];
	const uint8_t tile_height = request[32];
	const uint8_t format = request[33];
	const bool bgra_convert = format == rfb_format_rgb565 && surface_format == 7;
	const uint32_t pixel_bytes = format == rfb_format_clut8 ? 1 :
		format == rfb_format_rgb565 ? 2 : 0;
	const uint32_t tiles_x = tile_width ? (pitch + tile_width - 1) / tile_width : 0;
	const uint32_t tiles_y = tile_height ? (height + tile_height - 1) / tile_height : 0;
	const bool geometry_same = ready_ && width == width_ && height == height_ &&
		pitch == pitch_ && depth == depth_ && tile_width == tile_width_ &&
		tile_height == tile_height_ && format == format_;

	*reply_length = 0;
	if (!pixels || !output || !requested_capacity || requested_capacity > output_length)
		return bad_handle;
	if (width != surface_width || height != surface_height || !width || !height ||
		!pixel_bytes || depth != pixel_bytes * 8 || !tile_width || tile_width > 64 ||
		!tile_height || tile_height > 64 || ty0 > ty1 || ty1 > tiles_y ||
		static_cast<uint64_t>(tiles_x) * tiles_y > 65536 ||
		pitch < width * pixel_bytes ||
		(!bgra_convert && (pitch != surface_pitch ||
			(surface_format != (format == rfb_format_clut8 ? 0u : 1u))) ||
		(bgra_convert && surface_pitch < width * 4)))
		return bad_request;

	const bool owner_changed = ready_ && owner != owner_;
	const bool reconfigure = !geometry_same || owner_changed || flags != flags_ ||
		(request_flags & console_reset);
	if (reconfigure) {
		try {
			shadow_.assign(static_cast<size_t>(pitch) * height, 0);
		} catch (...) {
			reset();
			return no_memory;
		}
		ready_ = true;
		owner_ = owner;
		width_ = width;
		height_ = height;
		pitch_ = pitch;
		flags_ = flags;
		depth_ = depth;
		tile_width_ = tile_width;
		tile_height_ = tile_height;
		format_ = format;
		if (!geometry_same || owner_changed)
			snapshot_.clear();
		try {
			snapshot_.resize(static_cast<size_t>(pitch) * height);
		} catch (...) {
			reset();
			return no_memory;
		}
		for (uint32_t y = 0; y < height; ++y) {
			auto *dst = snapshot_.data() + static_cast<size_t>(y) * pitch;
			const auto *src = pixels + static_cast<size_t>(y) * surface_pitch;
			if (bgra_convert) {
				for (uint32_t x = 0; x < width; ++x) {
					const uint32_t value = (static_cast<uint32_t>(src[x * 4 + 2] & 0xf8) << 8) |
						(static_cast<uint32_t>(src[x * 4 + 1] & 0xfc) << 3) |
						(static_cast<uint32_t>(src[x * 4]) >> 3);
					dst[x * 2] = static_cast<uint8_t>(value >> 8);
					dst[x * 2 + 1] = static_cast<uint8_t>(value);
				}
				std::memset(dst + width * 2, 0, pitch - width * 2);
			} else {
				std::memcpy(dst, src, pitch);
			}
		}
	}
	if (snapshot_.size() != static_cast<size_t>(pitch) * height) {
		return bad_request;
	}

	uint32_t used = 0;
	auto emit = [&](uint8_t value) {
		if (used >= requested_capacity)
			return false;
		output[used++] = value;
		return true;
	};
	auto emit16 = [&](uint16_t value) { return emit(static_cast<uint8_t>(value >> 8)) && emit(static_cast<uint8_t>(value)); };
	if (!emit(rfb_version) || !emit(0) || !emit16(sequence_++)) {
		reset();
		return internal_error;
	}
	for (uint32_t ty = ty0; ty < ty1; ++ty) {
		const uint32_t top = ty * tile_height;
		const uint32_t rows = std::min<uint32_t>(tile_height, height - top);
		for (uint32_t tx = 0; tx < tiles_x; ++tx) {
			const uint32_t left = tx * tile_width;
			const uint32_t columns = std::min<uint32_t>(tile_width, pitch - left);
			bool dirty = false;
			for (uint32_t row = 0; row < rows && !dirty; ++row) {
				const auto offset = static_cast<size_t>(top + row) * pitch + left;
				dirty = std::memcmp(snapshot_.data() + offset, shadow_.data() + offset, columns) != 0;
			}
			if (!dirty)
				continue;
			if (!emit(rfb_op_tile8) || !emit16(static_cast<uint16_t>(ty * tiles_x + tx)) ||
				!emit(rfb_code_raw)) {
				reset();
				return internal_error;
			}
			for (uint32_t row = 0; row < rows; ++row) {
				const auto offset = static_cast<size_t>(top + row) * pitch + left;
				for (uint32_t byte = 0; byte < columns; ++byte) {
					if (!emit(snapshot_[offset + byte])) {
						reset();
						return internal_error;
					}
				}
				std::memcpy(shadow_.data() + offset, snapshot_.data() + offset, columns);
			}
		}
	}
	if (!emit(rfb_op_end)) {
		reset();
		return internal_error;
	}
	if (used > 4) {
		try {
			compressed_.resize(compressBound(used - 4));
		} catch (...) {
			compressed_.clear();
		}
		if (!compressed_.empty()) {
			uLongf compressed_length = compressed_.size();
			if (compress2(compressed_.data(), &compressed_length, output + 4,
				used - 4, 6) == Z_OK && 4 + compressed_length < used) {
				std::memcpy(output + 4, compressed_.data(), compressed_length);
				output[1] |= 1;
				used = static_cast<uint32_t>(4 + compressed_length);
			}
		}
	}
	put32(reply, used);
	put16(reply + 4, 0);
	put16(reply + 6, reconfigure && ty0 == 0 ? console_keyframe : 0);
	*reply_length = 8;
	return ok;
}

} // namespace zz9000_sdk
