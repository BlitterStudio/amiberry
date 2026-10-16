#include "zz9000_sdk.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

namespace {
uint16_t get16(const uint8_t *p) { return (p[0] << 8) | p[1]; }
uint32_t get32(const uint8_t *p) { return (get16(p) << 16) | get16(p + 2); }
void put16(uint8_t *p, uint16_t n) { p[0] = n >> 8; p[1] = n; }
void put32(uint8_t *p, uint32_t n) { put16(p, n >> 16); put16(p + 2, n); }

struct Mailbox {
	std::vector<uint8_t> board;
	zz9000_sdk::Engine sdk;
	uint32_t request_id = 0;
	uint32_t completion_head = 0;
	bool last_dirty = false;

	explicit Mailbox(uint32_t size) : board(size), sdk(board.data(), size) {}

	const uint8_t *call(uint16_t opcode, const uint8_t *payload,
	                    uint16_t length, uint16_t status = 0,
	                    bool doorbell = false) {
		auto *mb = board.data() + zz9000_sdk::mailbox_offset;
		const uint32_t tail = get32(mb + 24);
		auto *request = mb + 128 + tail * 64;
		std::fill(request, request + 64, 0);
		put32(request, ++request_id);
		put16(request + 4, opcode);
		put16(request + 10, length);
		if (payload)
			std::copy(payload, payload + length, request + 16);
		put32(mb + 24, (tail + 1) % 64);
		last_dirty = doorbell ? sdk.write_register(0x108, 1) : sdk.poll();
		const auto *reply = mb + 128 + 64 * 64 + completion_head * 64;
		assert(get32(reply) == request_id);
		assert(get16(reply + 6) == status);
		completion_head = (completion_head + 1) % 64;
		put32(mb + 36, completion_head);
		return reply + 16;
	}

	uint32_t allocate(uint32_t length) {
		uint8_t request[48] = {};
		put32(request, length);
		put32(request + 4, 16);
		put32(request + 8, 1);
		return get32(call(0x0100, request, 12));
	}
};

std::vector<uint8_t> read_file(const char *path)
{
	std::ifstream file(path, std::ios::binary);
	return {std::istreambuf_iterator<char>{file}, {}};
}

void run_image(Mailbox &mailbox, const std::vector<uint8_t> &image,
	           uint32_t codec, uint32_t format)
{
	assert(!image.empty() && image.size() < 4096);
	const uint32_t staging = mailbox.allocate(4096);
	const uint32_t tile = mailbox.allocate(64);
	auto *staging_data = mailbox.sdk.buffer_data(staging);
	std::copy(image.begin(), image.end(), staging_data);
	uint8_t request[48] = {};
	put32(request, codec);
	put32(request + 4, 3); // TILE_BUFFER
	put32(request + 28, format);
	put32(request + 32, tile);
	put32(request + 36, 16);
	put32(request + 40, 1);
	const auto *reply = mailbox.call(0x0404, request, 48);
	const uint32_t session = get32(reply);
	assert(session && get32(reply + 4) == 1);
	put32(request, tile);
	mailbox.call(0x0101, request, 4, 2); // tile pinned
	put32(request, session);
	put32(request + 4, staging);
	put32(request + 8, 0);
	put32(request + 12, static_cast<uint32_t>(image.size() / 2));
	put32(request + 16, 0);
	reply = mailbox.call(0x0405, request, 48, 0, true);
	assert(get32(reply + 4) == 1 && get32(reply + 36) == image.size() / 2);
	assert(!mailbox.last_dirty);
	put32(request + 8, static_cast<uint32_t>(image.size() / 2));
	put32(request + 12, static_cast<uint32_t>(image.size() - image.size() / 2));
	put32(request + 16, 1); // EOF
	reply = mailbox.call(0x0405, request, 48, 0, true);
	assert(get32(reply + 4) == 3 && get32(reply + 8) == 2 &&
	       get32(reply + 12) == 3 && get32(reply + 24) == 0 &&
	       get32(reply + 28) == 2 && get32(reply + 32) == 1);
	assert(!mailbox.last_dirty);
	if (codec == 2 && format == 8) {
		const auto *data = mailbox.sdk.buffer_data(tile);
		assert(data[0] == 255 && data[1] == 0 && data[2] == 0);
		assert(data[3] == 0 && data[4] == 255 && data[5] == 0);
	}
	put32(request + 12, 0);
	for (uint32_t row = 1; row < 3; ++row) {
		reply = mailbox.call(0x0405, request, 48);
		assert(get32(reply + 4) == 3 && get32(reply + 24) == row &&
		       get32(reply + 32) == 1 && get32(reply + 40) == 2 *
		       (format == 8 ? 3u : 4u));
		assert(!mailbox.last_dirty);
	}
	reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 4); // COMPLETE
	assert(!mailbox.last_dirty);
	put32(request, session);
	put32(request + 4, 0);
	mailbox.call(0x0406, request, 48);
	put32(request, tile);
	mailbox.call(0x0101, request, 4);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

void run_viewer_surface(Mailbox &mailbox, const std::vector<uint8_t> &image,
	                    uint32_t codec, uint32_t framebuffer_format)
{
	// The emulated RTG framebuffer begins after the 64 KiB register aperture.
	mailbox.sdk.set_framebuffer(0x110000, 4, 4,
		framebuffer_format == 7 ? 16 : 8, framebuffer_format);
	uint8_t request[48] = {};
	const auto *reply = mailbox.call(0x0202, request, 0);
	assert(get32(reply) == 0x80000000 && get32(reply + 8) == 4 &&
	       get32(reply + 20) == framebuffer_format &&
	       get32(reply + 4) == 0x300000);
	put32(request, 2); put32(request + 4, 3);
	put32(request + 8, 7); put32(request + 12, 16);
	put32(request + 16, 8);
	reply = mailbox.call(0x0200, request, 20);
	const uint32_t surface = get32(reply);
	assert((surface & 0xc0000000) == 0x40000000);
	const uint32_t staging = mailbox.allocate(4096);
	assert(image.size() < 4096);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, codec); put32(request + 4, 1);
	put32(request + 8, surface); put32(request + 20, 2);
	put32(request + 24, 3); put32(request + 28, 7);
	reply = mailbox.call(0x0404, request, 48);
	const uint32_t session = get32(reply);
	put32(request, surface);
	mailbox.call(0x0201, request, 4, 2); // decode surface pinned
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size() / 2));
	reply = mailbox.call(0x0405, request, 48, 0, true);
	assert(get32(reply + 4) == 1 && !mailbox.last_dirty); // NEED_INPUT
	put32(request + 8, static_cast<uint32_t>(image.size() / 2));
	put32(request + 12, static_cast<uint32_t>(image.size() - image.size() / 2));
	put32(request + 16, 1);
	reply = mailbox.call(0x0405, request, 48, 0, true);
	assert(get32(reply + 4) == 4 && get32(reply + 8) == 2 &&
	       get32(reply + 12) == 3 && get32(reply + 28) == 2 &&
	       get32(reply + 32) == 3 && get32(reply + 40) == 24);
	assert(!mailbox.last_dirty); // Offscreen surface is not yet visible.
	put32(request, session); put32(request + 4, 0);
	mailbox.call(0x0406, request, 48);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 0x80000000);
	put32(request + 12, 4); put32(request + 16, 4);
	mailbox.call(0x0203, request, 28);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, surface); put32(request + 4, 0x80000000);
	put16(request + 12, 2); put16(request + 14, 3);
	put16(request + 20, 4); put16(request + 22, 4);
	put16(request + 28, 2); put16(request + 30, 2);
	put32(request + 32, 1);
	mailbox.call(0x0407, request, 48);
	const auto *fb = mailbox.board.data() + 0x110000;
	if (codec == 2 && framebuffer_format == 7) {
		assert(fb[0] == 0 && fb[1] == 0 && fb[2] == 255);
		assert(fb[12] == 0 && fb[13] == 0 && fb[14] == 0);
	} else if (codec == 2) {
		assert(fb[0] == 0xf8 && fb[1] == 0x00);
	}
	put16(request + 28, 4); put16(request + 30, 4);
	mailbox.call(0x0407, request, 48);
	if (codec == 2 && framebuffer_format == 7)
		assert(fb[12] == 0 && fb[13] == 255 && fb[14] == 0);
	put32(request, surface);
	mailbox.call(0x0201, request, 4);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

void run_framebuffer_decode(Mailbox &mailbox, const std::vector<uint8_t> &image)
{
	assert(!image.empty() && image.size() < 4096);
	mailbox.sdk.set_framebuffer(0x110000, 4, 4, 16, 7);
	const uint32_t staging = mailbox.allocate(4096);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	uint8_t request[48] = {};
	put32(request, 2); // PNG
	put32(request + 4, 1); // DECODE_TO_SURFACE
	put32(request + 8, 0x80000000); // Live framebuffer
	put32(request + 20, 2);
	put32(request + 24, 3);
	put32(request + 28, 7); // BGRA8888
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	assert(session);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size() / 2));
	const auto *reply = mailbox.call(0x0405, request, 48, 0, true);
	assert(get32(reply + 4) == 1 && !mailbox.last_dirty); // NEED_INPUT
	put32(request + 8, static_cast<uint32_t>(image.size() / 2));
	put32(request + 12, static_cast<uint32_t>(image.size() - image.size() / 2));
	put32(request + 16, 1); // EOF
	reply = mailbox.call(0x0405, request, 48, 0, true);
	assert(get32(reply + 4) == 4 && get32(reply + 40) == 24 &&
	       mailbox.last_dirty); // Visible framebuffer write.
	put32(request, session);
	put32(request + 4, 0);
	mailbox.call(0x0406, request, 48);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

void reject_16bit_decode_surface(Mailbox &mailbox)
{
	const uint32_t offset = static_cast<uint32_t>(mailbox.board.size() - 8);
	for (uint32_t format : {1u, 6u}) {
		mailbox.sdk.set_framebuffer(offset, 2, 2, 4, format);
		std::fill(mailbox.board.end() - 16, mailbox.board.end(), 0xa5);
		uint8_t request[48] = {};
		put32(request, 2); // PNG
		put32(request + 4, 1); // DECODE_TO_SURFACE
		put32(request + 8, 0x80000000); // framebuffer
		put32(request + 20, 2);
		put32(request + 24, 2);
		put32(request + 28, 7); // BGRA8888 output
		mailbox.call(0x0404, request, 48, 3); // UNSUPPORTED
		assert(std::all_of(mailbox.board.end() - 16, mailbox.board.end(),
			[](uint8_t byte) { return byte == 0xa5; }));
	}
}

void reject_changed_decode_surface(Mailbox &mailbox,
	                               const std::vector<uint8_t> &image)
{
	assert(!image.empty() && image.size() < 4096);
	for (bool change_format : {true, false}) {
		mailbox.sdk.set_framebuffer(0x110000, 4, 4, 16, 7);
		const uint32_t staging = mailbox.allocate(4096);
		std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
		uint8_t request[48] = {};
		put32(request, 2); // PNG
		put32(request + 4, 1); // DECODE_TO_SURFACE
		put32(request + 8, 0x80000000); // framebuffer
		put32(request + 12, change_format ? 0 : 1);
		put32(request + 16, change_format ? 0 : 1);
		put32(request + 20, 2);
		put32(request + 24, 3);
		put32(request + 28, 7); // BGRA8888 output
		const uint32_t session = get32(mailbox.call(0x0404, request, 48));
		assert(session);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		put32(request + 4, staging);
		put32(request + 12, static_cast<uint32_t>(image.size() / 2));
		const auto *reply = mailbox.call(0x0405, request, 48);
		assert(get32(reply + 4) == 1); // NEED_INPUT

		std::fill(mailbox.board.end() - 32, mailbox.board.end(), 0xa5);
		if (change_format) {
			// The saved rectangle still fits, but four-byte writes do not.
			mailbox.sdk.set_framebuffer(static_cast<uint32_t>(mailbox.board.size() - 12),
				2, 3, 4, 1); // RGB565
		} else {
			// The format still matches, but the saved rectangle no longer fits.
			mailbox.sdk.set_framebuffer(static_cast<uint32_t>(mailbox.board.size() - 16),
				2, 2, 8, 7);
		}
		put32(request + 8, static_cast<uint32_t>(image.size() / 2));
		put32(request + 12, static_cast<uint32_t>(image.size() - image.size() / 2));
		put32(request + 16, 1); // EOF
		mailbox.call(0x0405, request, 48, change_format ? 3 : 4);
		assert(std::all_of(mailbox.board.end() - 32, mailbox.board.end(),
			[](uint8_t byte) { return byte == 0xa5; }));
		put32(request, session);
		put32(request + 4, 0);
		mailbox.call(0x0406, request, 48);
		put32(request, staging);
		mailbox.call(0x0101, request, 4);
	}
}
void check_surface_dirty_reports()
{
	for (bool doorbell : {false, true}) {
		Mailbox mailbox(4 * 1024 * 1024);
		mailbox.sdk.set_framebuffer(0x110000, 2, 2, 8, 7);
		uint8_t request[48] = {};
		const auto create_surface = [&]() {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, 2); put32(request + 4, 2);
			put32(request + 8, 7); put32(request + 12, 16);
			put32(request + 16, 8);
			return get32(mailbox.call(0x0200, request, 20, 0, doorbell));
		};
		const uint32_t source = create_surface();
		const uint32_t destination = create_surface();
		const auto fill = [&](uint32_t handle, uint32_t color) {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, handle);
			put32(request + 12, 2); put32(request + 16, 2);
			put32(request + 20, color);
			mailbox.call(0x0203, request, 28, 0, doorbell);
		};
		const auto scale = [&](uint32_t handle, uint16_t clip_x) {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, source); put32(request + 4, handle);
			put16(request + 12, 2); put16(request + 14, 2);
			put16(request + 20, 2); put16(request + 22, 2);
			put16(request + 24, clip_x);
			put16(request + 28, 2); put16(request + 30, 2);
			mailbox.call(0x0407, request, 40, 0, doorbell);
		};
		fill(source, 0x00ff0000);
		assert(!mailbox.last_dirty); // ARM-local preprocessing.
		fill(destination, 0x000000ff);
		assert(!mailbox.last_dirty);
		scale(destination, 0);
		assert(!mailbox.last_dirty);
		fill(0x80000000, 0x0000ff00);
		assert(mailbox.last_dirty);
		const auto *fb = mailbox.board.data() + 0x110000;
		assert(fb[0] == 0 && fb[1] == 255 && fb[2] == 0);
		scale(0x80000000, 2); // Empty clip intersection.
		assert(!mailbox.last_dirty);
		assert(fb[0] == 0 && fb[1] == 255 && fb[2] == 0);
		scale(0x80000000, 0);
		assert(mailbox.last_dirty);
		assert(fb[0] == 0 && fb[1] == 0 && fb[2] == 255);
	}
}

#ifdef AMIBERRY_WITH_WEBP
std::vector<uint8_t> decode_webp_tile(Mailbox &mailbox,
                                      const std::vector<uint8_t> &image,
                                      uint32_t feed_size)
{
	assert(!image.empty() && image.size() < 4096 && feed_size);
	const uint32_t staging = mailbox.allocate(4096);
	const uint32_t tile = mailbox.allocate(64);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	uint8_t request[48] = {};
	put32(request, 4); put32(request + 4, 3); put32(request + 28, 7);
	put32(request + 32, tile); put32(request + 36, 16); put32(request + 40, 3);
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	for (uint32_t offset = 0; offset < image.size();) {
		const uint32_t length = std::min<uint32_t>(feed_size,
			static_cast<uint32_t>(image.size()) - offset);
		const bool eof = offset + length == image.size();
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, staging);
		put32(request + 8, offset); put32(request + 12, length);
		put32(request + 16, eof);
		const auto *reply = mailbox.call(0x0405, request, 48);
		assert(get32(reply + 4) == (eof ? 3u : 1u) &&
		       get32(reply + 36) == length);
		offset += length;
	}
	const auto *pixels = mailbox.sdk.buffer_data(tile);
	std::vector<uint8_t> result(pixels, pixels + 48);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	mailbox.call(0x0406, request, 48);
	put32(request, tile);
	mailbox.call(0x0101, request, 4);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
	return result;
}

void run_webp_alpha(Mailbox &mailbox, const std::vector<uint8_t> &image)
{
	assert(image.size() > 61);
	const auto single_feed = decode_webp_tile(mailbox, image,
		static_cast<uint32_t>(image.size()));
	const auto chunked_feed = decode_webp_tile(mailbox, image, 61);
	assert(single_feed == chunked_feed);
	assert(single_feed[0] == 0 && single_feed[1] == 0 &&
	       single_feed[2] == 0 && single_feed[3] == 0);
	assert(single_feed[4] == 0x66 && single_feed[5] == 0x55 &&
	       single_feed[6] == 0x44 && single_feed[7] == 255);

	mailbox.sdk.set_framebuffer(0x110000, 4, 3, 16, 7);
	uint8_t request[48] = {};
	const uint32_t staging = mailbox.allocate(4096);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 4); put32(request + 4, 1); put32(request + 8, 0x80000000);
	put32(request + 20, 4); put32(request + 24, 3); put32(request + 28, 7);
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size())); put32(request + 16, 1);
	const auto *reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 4 && get32(reply + 40) == 48);
	const auto *pixels = mailbox.board.data() + 0x110000;
	assert(pixels[0] == 0 && pixels[1] == 0 &&
	       pixels[2] == 0 && pixels[3] == 0);
	assert(pixels[4] == 0x66 && pixels[5] == 0x55 &&
	       pixels[6] == 0x44 && pixels[7] == 255);
	put32(request + 4, 0);
	put32(request, session);
	mailbox.call(0x0406, request, 48);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

void append_le32(std::vector<uint8_t> &data, uint32_t value)
{
	data.push_back(static_cast<uint8_t>(value));
	data.push_back(static_cast<uint8_t>(value >> 8));
	data.push_back(static_cast<uint8_t>(value >> 16));
	data.push_back(static_cast<uint8_t>(value >> 24));
}

void append_le24(std::vector<uint8_t> &data, uint32_t value)
{
	data.push_back(static_cast<uint8_t>(value));
	data.push_back(static_cast<uint8_t>(value >> 8));
	data.push_back(static_cast<uint8_t>(value >> 16));
}

std::vector<uint8_t> webp_with_canvas(const std::vector<uint8_t> &image,
                                      uint32_t width, uint32_t height)
{
	assert(image.size() >= 20 && width && height);
	size_t offset = 12;
	while (offset + 8 <= image.size()) {
		const uint32_t length = image[offset + 4] |
			(static_cast<uint32_t>(image[offset + 5]) << 8) |
			(static_cast<uint32_t>(image[offset + 6]) << 16) |
			(static_cast<uint32_t>(image[offset + 7]) << 24);
		const size_t padded = length + (length & 1u);
		assert(padded >= length && padded <= image.size() - offset - 8);
		if (std::memcmp(image.data() + offset, "VP8L", 4) == 0) {
			std::vector<uint8_t> result{'R', 'I', 'F', 'F', 0, 0, 0, 0,
				'W', 'E', 'B', 'P', 'V', 'P', '8', 'X'};
			append_le32(result, 10);
			result.insert(result.end(), {0x10, 0, 0, 0});
			append_le24(result, width - 1);
			append_le24(result, height - 1);
			result.insert(result.end(), image.begin() + offset,
				image.begin() + offset + 8 + padded);
			const uint32_t riff_length = static_cast<uint32_t>(result.size() - 8);
			result[4] = static_cast<uint8_t>(riff_length);
			result[5] = static_cast<uint8_t>(riff_length >> 8);
			result[6] = static_cast<uint8_t>(riff_length >> 16);
			result[7] = static_cast<uint8_t>(riff_length >> 24);
			return result;
		}
		offset += 8 + padded;
	}
	assert(false);
	return {};
}

void reject_webp_canvas_limits(Mailbox &mailbox, const std::vector<uint8_t> &image)
{
	for (const auto dimensions : {std::pair<uint32_t, uint32_t>{8193, 3},
	                              {3000, 2100}}) {
		const auto oversized = webp_with_canvas(image, dimensions.first, dimensions.second);
		const uint32_t staging = mailbox.allocate(4096);
		const uint32_t tile = mailbox.allocate(64);
		std::copy(oversized.begin(), oversized.end(), mailbox.sdk.buffer_data(staging));
		uint8_t request[48] = {};
		put32(request, 4); put32(request + 4, 3); put32(request + 28, 7);
		put32(request + 32, tile); put32(request + 36, 16); put32(request + 40, 1);
		const uint32_t session = get32(mailbox.call(0x0404, request, 48));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, staging);
		put32(request + 12, static_cast<uint32_t>(oversized.size()));
		put32(request + 16, 1);
		mailbox.call(0x0405, request, 48, 4); // Firmware rejects canvas at IMAGE_FEED.
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		mailbox.call(0x0406, request, 48);
		put32(request, tile);
		mailbox.call(0x0101, request, 4);
		put32(request, staging);
		mailbox.call(0x0101, request, 4);
	}
}

void run_webp_animation(Mailbox &mailbox, const std::vector<uint8_t> &image)
{
	assert(!image.empty() && image.size() < 4096);
	const uint32_t staging = mailbox.allocate(4096);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	// Decode-time canvas dimensions, not only begin geometry, bound animation output.
	uint8_t request[48] = {};
	put32(request, 2); put32(request + 4, 1); put32(request + 8, 9);
	put32(request + 12, 16); put32(request + 16, 4);
	const uint32_t too_short = get32(mailbox.call(0x0200, request, 20));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 4); put32(request + 4, 1); put32(request + 8, too_short);
	put32(request + 20, 2); put32(request + 24, 1); put32(request + 28, 9);
	put32(request + 44, 8);
	const uint32_t rejected_session = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, rejected_session); put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size())); put32(request + 16, 1);
	mailbox.call(0x0405, request, 48, 4);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, rejected_session);
	mailbox.call(0x0406, request, 48);
	put32(request, too_short);
	mailbox.call(0x0201, request, 4);

	std::fill(std::begin(request), std::end(request), 0);
	
	put32(request, 2); put32(request + 4, 3); put32(request + 8, 9);
	put32(request + 12, 16); put32(request + 16, 4);
	const uint32_t surface = get32(mailbox.call(0x0200, request, 20));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, surface); put32(request + 12, 1); put32(request + 16, 1);
	mailbox.call(0x0203, request, 28, 3); // Packed YUV has no per-pixel fill.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 4); put32(request + 4, 1); put32(request + 8, surface);
	put32(request + 20, 2); put32(request + 24, 3); put32(request + 28, 9);
	put32(request + 44, 8);
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size())); put32(request + 16, 1);
	const auto *reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 6 && get32(reply + 8) == 2 && get32(reply + 12) == 3);
		mailbox.sdk.set_overlay(0x120000, 2, 3, 4, 0, true);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		reply = mailbox.call(0x0408, request, 48);
		const uint32_t token = get32(reply + 32);
		assert(get32(reply + 4) == 6 && token && get32(reply + 20) == 100 &&
		       get32(reply + 28) == 1 && get32(reply + 36) == 9 &&
		       (get32(reply + 40) & 1));
		put32(request + 4, token);
		reply = mailbox.call(0x0409, request, 48);
		assert(get32(reply + 40) == 4);
		const auto *pip = mailbox.board.data() + 0x120000;
		assert(pip[0] == 81 && pip[1] == 90 && pip[2] == 81 && pip[3] == 239);
		reply = mailbox.call(0x040a, request, 48);
		assert(!get32(reply + 32));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		reply = mailbox.call(0x0408, request, 48);
		const uint32_t second_token = get32(reply + 32);
		assert(second_token && get32(reply + 16) == 1 && get32(reply + 20) == 100);
		put32(request + 4, second_token);
		mailbox.call(0x0409, request, 48);
		assert(pip[0] == 41 && pip[1] == 240 && pip[2] == 41 && pip[3] == 110);
		mailbox.call(0x040a, request, 48);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		reply = mailbox.call(0x040b, request, 48);
		assert(get32(reply + 4) == 6 && !get32(reply + 32));
	mailbox.sdk.reset(); // An open animation session must release its YUV surface.
	mailbox.request_id = 0;
	mailbox.completion_head = 0;
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 2); put32(request + 4, 3); put32(request + 8, 9);
	put32(request + 12, 16); put32(request + 16, 4);
	const uint32_t replacement = get32(mailbox.call(0x0200, request, 20));
	put32(request, replacement);
	mailbox.call(0x0201, request, 4);
}

void reject_webp_input_limit(Mailbox &mailbox)
{
	const uint32_t staging = mailbox.allocate(64);
	auto *data = mailbox.sdk.buffer_data(staging);
	std::memcpy(data, "RIFF", 4);
	data[4] = 0; data[5] = 0; data[6] = 0x80; data[7] = 0;
	std::memcpy(data + 8, "WEBP", 4);
	const uint32_t tile = mailbox.allocate(64);
	uint8_t request[48] = {};
	put32(request, 4); put32(request + 4, 3); put32(request + 28, 7);
	put32(request + 32, tile); put32(request + 36, 16); put32(request + 40, 1);
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging); put32(request + 12, 12);
	mailbox.call(0x0405, request, 48, 4); // Declared input exceeds 8 MiB.
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		mailbox.call(0x0406, request, 48);
	// A complete but chunkless RIFF is malformed at the EOF decode boundary.
	data[4] = 4; data[5] = data[6] = data[7] = 0;
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 4); put32(request + 4, 3); put32(request + 28, 7);
	put32(request + 32, tile); put32(request + 36, 16); put32(request + 40, 1);
	const uint32_t truncated = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, truncated); put32(request + 4, staging); put32(request + 12, 12);
	put32(request + 16, 1);
	mailbox.call(0x0405, request, 48, 9);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, truncated);
		mailbox.call(0x0406, request, 48);
	put32(request, tile);
	mailbox.call(0x0101, request, 4);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}
#endif
} // namespace

int main(int argc, char **argv)
{
	assert(argc == 7);
	Mailbox mailbox(4 * 1024 * 1024);
	assert((get32(mailbox.board.data() + zz9000_sdk::mailbox_offset + 44) &
	        (1u << 5)) != 0);
	uint8_t request[48] = {};
	put32(request, 0x400);
	const auto *reply = mailbox.call(0x0004, request, 4);
	assert(get32(reply + 8) == ((1u << 5) | (1u << 6)));
	assert((get32(reply + 12) & ((1u << 20) | (1u << 21) | (1u << 26))) ==
	       ((1u << 20) | (1u << 21) | (1u << 26)));
#ifdef AMIBERRY_WITH_WEBP
	assert((get32(reply + 12) & ((1u << 28) | (1u << 29))) ==
	       ((1u << 28) | (1u << 29)));
	assert(get32(reply + 20) == 12);
#endif
	run_image(mailbox, read_file(argv[1]), 2, 8); // PNG RGB888
	run_image(mailbox, read_file(argv[1]), 2, 7); // PNG BGRA8888
	run_image(mailbox, read_file(argv[2]), 1, 8); // JPEG RGB888
#ifdef AMIBERRY_WITH_WEBP
	run_image(mailbox, read_file(argv[3]), 4, 8); // Lossy WebP RGB888
	run_image(mailbox, read_file(argv[4]), 4, 7); // Lossless WebP BGRA8888
	run_webp_animation(mailbox, read_file(argv[5]));
	run_webp_alpha(mailbox, read_file(argv[6]));
	reject_webp_canvas_limits(mailbox, read_file(argv[6]));
	reject_webp_input_limit(mailbox);
#endif
	run_viewer_surface(mailbox, read_file(argv[1]), 2, 7);
	run_viewer_surface(mailbox, read_file(argv[1]), 2, 1);
	run_viewer_surface(mailbox, read_file(argv[2]), 1, 7);
	run_framebuffer_decode(mailbox, read_file(argv[1]));
	reject_16bit_decode_surface(mailbox);
	reject_changed_decode_surface(mailbox, read_file(argv[1]));
	check_surface_dirty_reports();
	mailbox.sdk.reset();
	assert(mailbox.sdk.buffer_data(1) == nullptr);
}
