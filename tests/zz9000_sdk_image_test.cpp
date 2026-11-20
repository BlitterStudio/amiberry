#include "zz9000_sdk.h"
#include "zz9000_sdk_test_mailbox.h"

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
	uint8_t *mb;
	uint32_t entries;
	uint32_t request_id = 0;
	uint32_t completion_head = 0;
	bool last_dirty = false;
	uint16_t last_length = 0; // The last completion's payload length.
	uint16_t client = 0; // Stamped into each entry's flags (bits 5-15).

	explicit Mailbox(uint32_t size)
		: board(size), sdk(board.data(), size),
		  mb(zz9000_test_mailbox(sdk, board.data())), entries(get32(mb + 16)) {}

	const uint8_t *call(uint16_t opcode, const uint8_t *payload,
	                    uint16_t length, uint16_t status = 0,
	                    bool doorbell = false) {
		const uint32_t tail = get32(mb + 24);
		auto *request = mb + get32(mb + 12) + tail * 64;
		std::fill(request, request + 64, 0);
		put32(request, ++request_id);
		put16(request + 4, opcode);
		put16(request + 8, static_cast<uint16_t>(client << 5));
		put16(request + 10, length);
		if (payload)
			std::copy(payload, payload + length, request + 16);
		put32(mb + 24, (tail + 1) % entries);
		last_dirty = doorbell ? sdk.write_register(0x108, 1) : sdk.poll();
		const auto *reply = mb + get32(mb + 28) + completion_head * 64;
		assert(get32(reply) == request_id);
		assert(get16(reply + 6) == status);
		last_length = get16(reply + 10);
		completion_head = (completion_head + 1) % entries;
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
	
	put32(request, 2); put32(request + 4, 2); put32(request + 8, 9);
	put32(request + 12, 16); put32(request + 16, 4);
	const uint32_t surface = get32(mailbox.call(0x0200, request, 20));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, surface); put32(request + 12, 1); put32(request + 16, 1);
	mailbox.call(0x0203, request, 28, 3); // Packed YUV has no per-pixel fill.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 4); put32(request + 4, 1); put32(request + 8, surface);
	put32(request + 20, 2); put32(request + 24, 2); put32(request + 28, 9);
	put32(request + 44, 8);
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size())); put32(request + 16, 1);
	const auto *reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 6 && get32(reply + 8) == 2 && get32(reply + 12) == 2);
		zz9000_sdk::Engine::OverlayGeometry overlay_geometry;
		overlay_geometry.dst_w = 2;
		overlay_geometry.dst_h = 2;
		overlay_geometry.screen_w = 2;
		overlay_geometry.screen_h = 2;
		mailbox.sdk.set_overlay(0x120000, 2, 2, 4, 0, true, true,
			overlay_geometry);
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

// A CPU-visible surface lives in board memory, where the guest reads it
// through the Zorro window: its reported ARM address maps back to the
// board offset its pixels are at, like the card's surface_allocator.
uint8_t *visible_pixels(Mailbox &mailbox, const uint8_t *reply)
{
	assert(get32(reply + 24) == 1); // CPU_VISIBLE, not ARM_LOCAL
	const uint32_t arm = get32(reply + 4);
	assert(arm >= 0x00200000);
	const uint32_t offset = arm - 0x00200000 + 0x10000;
	assert(offset + get32(reply + 28) <= mailbox.board.size());
	return mailbox.board.data() + offset;
}

uint32_t alloc_surface(Mailbox &mailbox, uint32_t width, uint32_t height,
	                   uint32_t format, uint32_t flags, uint8_t **pixels = nullptr)
{
	uint8_t request[48] = {};
	put32(request, width); put32(request + 4, height);
	put32(request + 8, format); put32(request + 12, flags);
	const auto *reply = mailbox.call(0x0200, request, 20);
	if (pixels)
		*pixels = visible_pixels(mailbox, reply);
	return get32(reply);
}

void free_surface(Mailbox &mailbox, uint32_t surface)
{
	uint8_t request[48] = {};
	put32(request, surface);
	mailbox.call(0x0201, request, 4);
}

void check_cpu_visible_surfaces(Mailbox &mailbox)
{
	uint8_t *pixels = nullptr;
	// Flags 0 (the SDK default) and CPU_VISIBLE are both CPU-visible.
	for (uint32_t flags : {0u, 1u}) {
		const uint32_t surface = alloc_surface(mailbox, 3, 2, 7, flags, &pixels);
		assert(std::all_of(pixels, pixels + 24, [](uint8_t b) { return b == 0; }));
		uint8_t request[48] = {};
		put32(request, surface);
		put32(request + 12, 3); put32(request + 16, 2);
		put32(request + 20, 0x00123456);
		mailbox.call(0x0203, request, 28); // FILL
		assert(pixels[0] == 0x56 && pixels[1] == 0x34 && pixels[2] == 0x12);
		assert(pixels[20] == 0x56 && pixels[22] == 0x12);
		free_surface(mailbox, surface);
	}
	// The card's other 32- and 24-bit formats.
	for (uint32_t format : {2u, 3u, 8u}) {
		const uint32_t surface = alloc_surface(mailbox, 2, 2, format, 0, &pixels);
		free_surface(mailbox, surface);
	}
	// A shared buffer and a CPU-visible surface never overlap.
	const uint32_t staging = mailbox.allocate(256);
	const uint32_t surface = alloc_surface(mailbox, 8, 8, 7, 0, &pixels);
	const auto *buffer = mailbox.sdk.buffer_data(staging);
	assert(buffer + 256 <= pixels || pixels + 256 <= buffer);
	const uint32_t staging2 = mailbox.allocate(256);
	const auto *buffer2 = mailbox.sdk.buffer_data(staging2);
	assert(buffer2 + 256 <= pixels || pixels + 256 <= buffer2);
	free_surface(mailbox, surface);
	uint8_t request[48] = {};
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
	put32(request, staging2);
	mailbox.call(0x0101, request, 4);
	// ARM_LOCAL surfaces stay card-only.
	put32(request, 2); put32(request + 4, 2);
	put32(request + 8, 7); put32(request + 12, 16);
	const auto *reply = mailbox.call(0x0200, request, 20);
	assert(get32(reply + 24) == 16);
	free_surface(mailbox, get32(reply));
}

// DECODE_JPEG, as the card's: the picture at its own size into the
// destination rectangle, in the surface's format; never scaled.
void run_one_shot_jpeg(Mailbox &mailbox, const std::vector<uint8_t> &image)
{
	assert(!image.empty() && image.size() < 4096);
	const uint32_t staging = mailbox.allocate(4096);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	uint8_t *argb = nullptr, *bgra = nullptr;
	const uint32_t s_argb = alloc_surface(mailbox, 4, 4, 2, 0, &argb);
	const uint32_t s_bgra = alloc_surface(mailbox, 4, 4, 7, 0, &bgra);
	const auto decode = [&](uint32_t surface, uint32_t format, uint32_t x, uint32_t y,
	                        uint32_t w, uint32_t h, uint32_t flags, uint16_t status) {
		uint8_t request[48] = {};
		put32(request, staging);
		put32(request + 8, static_cast<uint32_t>(image.size()));
		put32(request + 12, surface);
		put32(request + 16, x); put32(request + 20, y);
		put32(request + 24, w); put32(request + 28, h);
		put32(request + 32, format); put32(request + 36, flags);
		return mailbox.call(0x0401, request, 40, status);
	};
	// The 2x3 picture at (1,1); 0 x 0 runs to the surface's edges.
	auto *reply = decode(s_argb, 2, 1, 1, 0, 0, 0, 0);
	assert(get32(reply) == 2 && get32(reply + 4) == 3 && get32(reply + 8) == 2 &&
	       get32(reply + 16) == 24);
	// The SDK's reply decoder takes the whole 48-byte result payload.
	assert(mailbox.last_length == 48);
	decode(s_bgra, 7, 1, 1, 2, 3, 0, 0);
	assert(mailbox.last_length == 48);
	// RGBA8888 and RGB888: the same pixels in their own byte order.
	uint8_t *rgba = nullptr, *rgb = nullptr;
	const uint32_t s_rgba = alloc_surface(mailbox, 4, 4, 3, 0, &rgba);
	const uint32_t s_rgb = alloc_surface(mailbox, 4, 4, 8, 0, &rgb);
	reply = decode(s_rgba, 3, 1, 1, 0, 0, 0, 0);
	assert(get32(reply + 8) == 3 && get32(reply + 16) == 24);
	reply = decode(s_rgb, 8, 1, 1, 0, 0, 0, 0);
	assert(get32(reply + 8) == 8 && get32(reply + 16) == 18);
	for (uint32_t y = 1; y < 4; ++y)
		for (uint32_t x = 1; x < 3; ++x) {
			const uint8_t *a = argb + (y * 4 + x) * 4;
			const uint8_t *r4 = rgba + (y * 4 + x) * 4, *r3 = rgb + (y * 4 + x) * 3;
			assert(r4[0] == a[1] && r4[1] == a[2] && r4[2] == a[3] && r4[3] == 255);
			assert(r3[0] == a[1] && r3[1] == a[2] && r3[2] == a[3]);
		}
	assert(!rgb[0] && !rgb[1] && !rgb[2] && !rgba[3]); // (0,0): untouched
	free_surface(mailbox, s_rgba);
	free_surface(mailbox, s_rgb);
	for (uint32_t y = 0; y < 4; ++y)
		for (uint32_t x = 0; x < 4; ++x) {
			const uint8_t *a = argb + (y * 4 + x) * 4, *b = bgra + (y * 4 + x) * 4;
			if (x >= 1 && x <= 2 && y >= 1) {
				assert(a[0] == 255 && b[3] == 255); // opaque
				assert(a[1] == b[2] && a[2] == b[1] && a[3] == b[0]);
			} else {
				assert(!a[0] && !a[1] && !a[2] && !a[3]); // outside: untouched
			}
		}
	// The top-left pixel is red: the PNG's (255,0,0) through the JPEG.
	assert(argb[(1 * 4 + 1) * 4 + 1] > 128 && argb[(1 * 4 + 1) * 4 + 3] < 128);
	decode(s_argb, 2, 0, 0, 2, 2, 0, 9);       // too small: never scaled (I/O error)
	decode(s_argb, 2, 3, 0, 0, 0, 0, 9);       // 1 column left: too small
	decode(s_argb, 2, 0, 0, 5, 0, 0, 4);       // past the surface: BAD_REQUEST
	decode(s_argb, 7, 0, 0, 0, 0, 0, 3);       // not the surface's format
	decode(s_argb, 2, 0, 0, 0, 0, 1, 3);       // any flag (FIT is a session's)
	uint8_t request[48] = {};
	put32(request, staging);
	put32(request + 8, static_cast<uint32_t>(image.size()));
	put32(request + 12, 0x40ffffff);
	put32(request + 32, 2);
	mailbox.call(0x0401, request, 40, 5);     // no such surface
	// Into the live framebuffer: a visible write.
	mailbox.sdk.set_framebuffer(0x110000, 4, 4, 16, 7);
	reply = decode(0x80000000, 7, 0, 0, 0, 0, 0, 0);
	assert(get32(reply) == 2 && mailbox.last_dirty);
	free_surface(mailbox, s_argb);
	free_surface(mailbox, s_bgra);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

// SCALE_IMAGE: the clipped scaler with the destination as the clip.
void run_scale_image(Mailbox &mailbox)
{
	uint8_t *src = nullptr, *dst = nullptr;
	const uint32_t s = alloc_surface(mailbox, 2, 1, 7, 0, &src);
	const uint32_t d = alloc_surface(mailbox, 4, 2, 7, 0, &dst);
	src[0] = 0x10; src[1] = 0x20; src[2] = 0x30; src[3] = 0xff;
	src[4] = 0x40; src[5] = 0x50; src[6] = 0x60; src[7] = 0xff;
	uint8_t request[48] = {};
	put32(request, s); put32(request + 4, d);
	put32(request + 16, 2); put32(request + 20, 1);
	put32(request + 32, 4); put32(request + 36, 2);
	mailbox.call(0x0400, request, 48); // nearest
	assert(dst[0] == 0x10 && dst[4] == 0x10 && dst[8] == 0x40 && dst[12] == 0x40);
	assert(dst[16] == 0x10 && dst[28] == 0x40);
	assert(!mailbox.last_dirty); // offscreen: no redraw
	put32(request + 40, 2);
	mailbox.call(0x0400, request, 48, 3); // no such filter
	// Bilinear: between the two source pixels in the middle columns.
	put32(request + 40, 1);
	mailbox.call(0x0400, request, 48);
	assert(dst[0] < dst[4] && dst[4] < dst[8] && dst[8] < dst[12]);
	assert(dst[0] >= 0x10 && dst[12] <= 0x40 && dst[3] == 0xff);
	// Into the live framebuffer: a visible write, reported for a redraw.
	mailbox.sdk.set_framebuffer(0x110000, 4, 4, 16, 7);
	put32(request + 4, 0x80000000);
	put32(request + 40, 0);
	mailbox.call(0x0400, request, 48);
	assert(mailbox.last_dirty);
	const auto *fb = mailbox.board.data() + 0x110000;
	assert(fb[0] == 0x10 && fb[12] == 0x40);
	free_surface(mailbox, s);
	free_surface(mailbox, d);
	// One format to itself: nearest for any, bilinear for 4-byte ones;
	// across formats only BGRA8888 to RGB565/RGB555.
	const auto scale = [&](uint32_t from, uint32_t to, uint32_t filter, uint16_t status) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, from); put32(request + 4, to);
		put32(request + 16, 2); put32(request + 20, 1);
		put32(request + 32, 4); put32(request + 36, 2);
		put32(request + 40, filter);
		mailbox.call(0x0400, request, 48, status);
	};
	uint8_t *a = nullptr, *b = nullptr;
	const uint32_t rgb_s = alloc_surface(mailbox, 2, 1, 8, 0, &a);
	const uint32_t rgb_d = alloc_surface(mailbox, 4, 2, 8, 0, &b);
	a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4; a[4] = 5; a[5] = 6;
	scale(rgb_s, rgb_d, 0, 0);
	assert(b[0] == 1 && b[2] == 3 && b[9] == 4 && b[11] == 6 && b[12] == 1);
	scale(rgb_s, rgb_d, 1, 3); // RGB888 bilinear: unsupported
	free_surface(mailbox, rgb_d); // (four surfaces at a time)
	const uint32_t argb_s = alloc_surface(mailbox, 2, 1, 2, 0, &a);
	const uint32_t argb_d = alloc_surface(mailbox, 4, 2, 2, 0, &b);
	a[0] = 0xff; a[1] = 0x00; a[2] = 0x80; a[3] = 0x10;
	a[4] = 0xff; a[5] = 0xf0; a[6] = 0x80; a[7] = 0x50;
	scale(argb_s, argb_d, 1, 0);
	assert(b[0] == 0xff && b[2] == 0x80 && b[1] < b[5] && b[5] < b[9] && b[9] < b[13]);
	uint8_t *c = nullptr;
	const uint32_t rgb565 = alloc_surface(mailbox, 4, 2, 1, 0, &c);
	scale(argb_s, rgb565, 0, 3); // ARGB8888 to RGB565: unsupported
	scale(rgb_s, argb_d, 0, 3);  // RGB888 to ARGB8888: unsupported
	for (uint32_t h : {rgb_s, argb_s, argb_d, rgb565})
		free_surface(mailbox, h);
}

// ALLOC_SURFACE's statuses and flags, as the card's handle_alloc_surface().
void check_alloc_surface_rules(Mailbox &mailbox)
{
	uint8_t request[48] = {};
	const auto alloc = [&](uint32_t w, uint32_t h, uint32_t format, uint32_t flags,
	                       uint16_t status) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, w); put32(request + 4, h);
		put32(request + 8, format); put32(request + 12, flags);
		return mailbox.call(0x0200, request, 20, status);
	};
	alloc(0, 2, 7, 0, 4);  // no width: BAD_REQUEST
	alloc(2, 0, 7, 0, 4);  // no height
	alloc(2, 2, 0, 0, 4);  // UNKNOWN format
	alloc(2, 2, 5, 0, 4);  // PLANAR: no row size
	alloc(2, 2, 99, 0, 4); // not a format
	// FRAMEBUFFER, DISPLAYED and SHARED_BUFFER are kept as given.
	auto *reply = alloc(2, 2, 7, 2 | 4 | 8, 0);
	assert(get32(reply + 24) == (2u | 4u | 8u | 1u));
	free_surface(mailbox, get32(reply));
	reply = alloc(2, 2, 7, 1 | 8 | 16, 0); // ARM_LOCAL wins over CPU_VISIBLE
	assert(get32(reply + 24) == (8u | 16u));
	free_surface(mailbox, get32(reply));
}

// An image session writes its own output format into a surface of it.
void run_session_argb_surface(Mailbox &mailbox, const std::vector<uint8_t> &image)
{
	assert(!image.empty() && image.size() < 4096);
	const uint32_t staging = mailbox.allocate(4096);
	std::copy(image.begin(), image.end(), mailbox.sdk.buffer_data(staging));
	uint8_t *argb = nullptr;
	const uint32_t surface = alloc_surface(mailbox, 2, 3, 2, 0, &argb);
	uint8_t request[48] = {};
	put32(request, 1); put32(request + 4, 1); // JPEG, DECODE_TO_SURFACE
	put32(request + 8, surface);
	put32(request + 20, 2); put32(request + 24, 3);
	put32(request + 28, 7); // BGRA output into an ARGB surface: refused
	mailbox.call(0x0404, request, 48, 3);
	put32(request + 28, 2); // ARGB8888
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging);
	put32(request + 12, static_cast<uint32_t>(image.size()));
	put32(request + 16, 1);
	const auto *reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 4 && get32(reply + 40) == 24);
	assert(argb[0] == 255 && argb[1] > 128 && argb[3] < 128); // A, then a red
	put32(request, session); put32(request + 4, 0);
	mailbox.call(0x0406, request, 48);
	free_surface(mailbox, surface);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

// COPY_SURFACE and QUERY_PALETTE, as handle_copy_surface(),
// sdk_surface_copy_rect(), handle_query_palette() and sdk_palette_pack_be().
void copy_surface_and_query_palette(Mailbox &mailbox)
{
	mailbox.sdk.set_framebuffer(0x110000, 4, 2, 16, 7);
	uint8_t *src_px = nullptr, *dst_px = nullptr;
	const uint32_t src = alloc_surface(mailbox, 4, 2, 7, 0, &src_px);
	const uint32_t dst = alloc_surface(mailbox, 4, 2, 7, 0, &dst_px);
	src_px[0] = 0x11; src_px[1] = 0x22; src_px[2] = 0x33; src_px[3] = 0x44;
	src_px[4] = 0x55;
	src_px[16] = 0xaa; // row 1, pitch 16
	uint8_t request[48] = {};
	const auto copy = [&](uint32_t from, uint32_t to, uint32_t sx, uint32_t sy,
	                      uint32_t dx, uint32_t dy, uint32_t w, uint32_t h,
	                      uint32_t flags, uint16_t status, uint16_t length = 48) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, from); put32(request + 4, to);
		put32(request + 8, sx); put32(request + 12, sy);
		put32(request + 16, dx); put32(request + 20, dy);
		put32(request + 24, w); put32(request + 28, h);
		put32(request + 32, flags);
		return mailbox.call(0x0204, request, length, status);
	};
	copy(src, dst, 0, 0, 0, 0, 4, 2, 0, 0);
	assert(mailbox.last_length == 0); // complete_status: no inline result
	assert(!mailbox.last_dirty);
	assert(dst_px[0] == 0x11 && dst_px[4] == 0x55 && dst_px[16] == 0xaa);
	assert(dst_px[8] == 0);
	uint8_t *rgb565 = nullptr;
	const uint32_t narrow = alloc_surface(mailbox, 4, 2, 1, 0, &rgb565);
	copy(src, narrow, 0, 0, 0, 0, 1, 1, 0, 3); // format mismatch
	free_surface(mailbox, narrow);
	copy(src, dst, 0, 0, 0, 0, 1, 1, 1, 3); // any flag
	copy(src, dst, 0, 0, 3, 0, 2, 1, 0, 4); // past the edge
	copy(src, dst, 0, 0, 0, 0, 0, 1, 0, 4); // empty rectangle
	copy(0x40ffffff, dst, 0, 0, 0, 0, 1, 1, 0, 5);
	copy(src, dst, 0, 0, 0, 0, 1, 1, 0, 4, 20); // shorter than the 48-byte payload
	uint8_t *yuv_a = nullptr, *yuv_b = nullptr;
	const uint32_t y0 = alloc_surface(mailbox, 4, 2, 9, 0, &yuv_a);
	const uint32_t y1 = alloc_surface(mailbox, 4, 2, 9, 0, &yuv_b);
	copy(y0, y1, 0, 0, 0, 0, 2, 1, 0, 4); // YUV422CGX: no byte size
	copy(y0, dst, 0, 0, 0, 0, 2, 1, 0, 3); // mismatch is checked first
	free_surface(mailbox, y0);
	free_surface(mailbox, y1);
	// Horizontal overlap: memmove, not a forward copy that would smear.
	uint8_t *row = nullptr;
	const uint32_t overlap = alloc_surface(mailbox, 4, 1, 4, 0, &row);
	row[0] = 1; row[1] = 2; row[2] = 3; row[3] = 4;
	copy(overlap, overlap, 0, 0, 1, 0, 3, 1, 0, 0);
	assert(row[0] == 1 && row[1] == 1 && row[2] == 2 && row[3] == 3);
	free_surface(mailbox, overlap);
	// Vertical overlap: bottom-up, so the source row is not overwritten first.
	uint8_t *rows = nullptr;
	const uint32_t vertical = alloc_surface(mailbox, 1, 3, 4, 0, &rows);
	rows[0] = 0x0a; rows[1] = 0x0b; rows[2] = 0x0c;
	copy(vertical, vertical, 0, 0, 0, 1, 1, 2, 0, 0);
	assert(rows[0] == 0x0a && rows[1] == 0x0a && rows[2] == 0x0b);
	free_surface(mailbox, vertical);
	// Wider pitch than the minimum row: the copy follows the pitch.
	uint8_t *wide = nullptr;
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 2); put32(request + 4, 2);
	put32(request + 8, 7); put32(request + 16, 16);
	const auto *wide_reply = mailbox.call(0x0200, request, 20);
	wide = visible_pixels(mailbox, wide_reply);
	const uint32_t wide_handle = get32(wide_reply);
	wide[0] = 0x01; wide[16] = 0x02;
	copy(wide_handle, dst, 0, 0, 0, 0, 2, 2, 0, 0);
	assert(dst_px[0] == 0x01 && dst_px[16] == 0x02);
	free_surface(mailbox, wide_handle);
	// ARM-local source into a CPU-visible destination.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 2); put32(request + 4, 1);
	put32(request + 8, 7); put32(request + 12, 16);
	const uint32_t arm = get32(mailbox.call(0x0200, request, 20));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, arm);
	put32(request + 12, 2); put32(request + 16, 1);
	put32(request + 20, 0x00123456);
	mailbox.call(0x0203, request, 28);
	copy(arm, dst, 0, 0, 2, 0, 2, 1, 0, 0);
	assert(dst_px[8] == 0x56 && dst_px[9] == 0x34 && dst_px[10] == 0x12);
	free_surface(mailbox, arm);
	copy(src, 0x80000000, 0, 0, 0, 0, 2, 1, 0, 0);
	assert(mailbox.last_dirty); // framebuffer destination is a redraw
	const auto *fb = mailbox.board.data() + 0x110000;
	assert(fb[0] == 0x11 && fb[1] == 0x22 && fb[2] == 0x33);
	copy(0x80000000, dst, 0, 0, 0, 1, 2, 1, 0, 0);
	assert(!mailbox.last_dirty);
	assert(dst_px[16] == 0x11 && dst_px[17] == 0x22);
	// DECODE_PNG and DECODE_GIF stay unsupported, as on the card.
	mailbox.call(0x0402, request, 40, 3);
	mailbox.call(0x0403, request, 40, 3);
	const uint32_t staging = mailbox.allocate(64);
	auto *packed = mailbox.sdk.buffer_data(staging);
	const auto query = [&](uint32_t start, uint32_t count, uint32_t flags,
	                       uint32_t handle, uint32_t offset, uint16_t status,
	                       uint16_t length = 48) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, 0x40ffffff); // surface is ignored
		put32(request + 4, start);
		put32(request + 8, count);
		put32(request + 12, handle);
		put32(request + 16, offset);
		put32(request + 20, flags);
		return mailbox.call(0x0205, request, length, status);
	};
	query(0, 2, 0, staging, 0, 0);
	assert(mailbox.last_length == 0);
	assert(packed[0] == 0 && packed[7] == 0); // reset shadow is zero
	mailbox.sdk.set_palette_entry(1, 0x11, 0x22, 0x33);
	mailbox.sdk.set_palette_entry(255, 0xaa, 0xbb, 0xcc);
	query(1, 1, 0, staging, 0, 0);
	assert(packed[0] == 0x00 && packed[1] == 0x11 &&
	       packed[2] == 0x22 && packed[3] == 0x33);
	query(255, 1, 0, staging, 4, 0);
	assert(packed[4] == 0x00 && packed[5] == 0xaa &&
	       packed[6] == 0xbb && packed[7] == 0xcc);
	query(0, 0, 0, staging, 0, 4);
	query(256, 1, 0, staging, 0, 4);
	query(200, 100, 0, staging, 0, 4);
	query(0, 1, 2, staging, 0, 4); // flag bit other than SECONDARY
	query(0, 1, 1, staging, 0, 3); // SECONDARY
	query(0, 1, 3, staging, 0, 4); // unknown bit is checked first
	query(0, 1, 0, 0, 0, 5);
	query(0, 1, 0, staging, 62, 5); // 4 bytes do not fit
	query(0, 1, 0, staging, 0, 4, 20); // shorter than 24
	query(1, 1, 0, staging, 8, 0, 24); // minimum payload
	assert(packed[8] == 0x00 && packed[9] == 0x11 && packed[11] == 0x33);
	free_surface(mailbox, src);
	free_surface(mailbox, dst);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}

// CLIENT_RELEASE (0x0006): what the tagged client holds is reclaimed; other
// clients' resources stay. zz9k.library 2.38+ sends it on its last close.
void check_client_release(Mailbox &mailbox)
{
	uint8_t request[48] = {};
	mailbox.client = 3;
	const uint32_t mine = mailbox.allocate(4096);
	put32(request, 2); put32(request + 4, 2);
	put32(request + 8, 7); put32(request + 12, 16);
	const uint32_t surface = get32(mailbox.call(0x0200, request, 20));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 1); // JPEG
	put32(request + 4, 3); // TILE_BUFFER
	put32(request + 28, 7);
	put32(request + 32, mine);
	put32(request + 36, 64); put32(request + 40, 4);
	const uint32_t session = get32(mailbox.call(0x0404, request, 48));
	assert(session);
	mailbox.client = 4;
	const uint32_t theirs = mailbox.allocate(256);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 1); put32(request + 4, 3);
	put32(request + 28, 7); put32(request + 32, theirs);
	put32(request + 36, 64); put32(request + 40, 4);
	mailbox.call(0x0404, request, 48, 2); // the card's one session: BUSY
	// Refused: an untagged entry, and one with a payload.
	mailbox.client = 0;
	mailbox.call(0x0006, nullptr, 0, 4);
	mailbox.client = 3;
	mailbox.call(0x0006, request, 4, 4);
	mailbox.call(0x0006, nullptr, 0);
	assert(mailbox.sdk.buffer_data(mine) == nullptr);
	assert(mailbox.sdk.buffer_data(theirs) != nullptr);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	mailbox.call(0x0406, request, 48, 5); // its session is gone
	put32(request, surface);
	mailbox.call(0x0201, request, 4, 5); // and its surface
	// The card is free again for client 4.
	mailbox.client = 4;
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 1); put32(request + 4, 3);
	put32(request + 28, 7); put32(request + 32, theirs);
	put32(request + 36, 64); put32(request + 40, 4);
	const uint32_t next = get32(mailbox.call(0x0404, request, 48));
	assert(next);
	mailbox.call(0x0006, nullptr, 0);
	assert(mailbox.sdk.buffer_data(theirs) == nullptr);
	// A buffer one client allocated as another client's session's tile
	// buffer: the first client's release leaves it to the session (still
	// pinned: FREE_SHARED is BUSY); it goes with the session's client.
	mailbox.client = 9;
	const uint32_t lent = mailbox.allocate(4096);
	mailbox.client = 10;
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 1); put32(request + 4, 3);
	put32(request + 28, 7); put32(request + 32, lent);
	put32(request + 36, 64); put32(request + 40, 4);
	const uint32_t borrower = get32(mailbox.call(0x0404, request, 48));
	assert(borrower);
	mailbox.client = 9;
	mailbox.call(0x0006, nullptr, 0);
	assert(mailbox.sdk.buffer_data(lent) != nullptr);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, lent);
	mailbox.call(0x0101, request, 4, 2);
	mailbox.client = 10;
	mailbox.call(0x0006, nullptr, 0);
	assert(mailbox.sdk.buffer_data(lent) == nullptr);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, borrower);
	mailbox.call(0x0406, request, 48, 5);
	mailbox.client = 0;
}
} // namespace

int main(int argc, char **argv)
{
	assert(argc == 7);
	Mailbox mailbox(4 * 1024 * 1024);
	assert((get32(mailbox.mb + 44) & (1u << 5)) != 0);
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
	put32(request, 0x200);
	reply = mailbox.call(0x0004, request, 4);
	assert(get32(reply + 20) == 6); // 0x0200..0x0205
	assert((get32(reply + 12) & (1u << 16)) != 0); // PALETTE_QUERY
#ifndef HAVE_MPG123
	assert((get32(mailbox.mb + 44) & (1u << 7)) == 0);
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
	check_cpu_visible_surfaces(mailbox);
	check_alloc_surface_rules(mailbox);
	run_one_shot_jpeg(mailbox, read_file(argv[2]));
	run_scale_image(mailbox);
	run_session_argb_surface(mailbox, read_file(argv[2]));
	{
		// The same on a Zorro III board, from its heap.
		Mailbox z3(0x08000000);
		check_cpu_visible_surfaces(z3);
		run_one_shot_jpeg(z3, read_file(argv[2]));
		run_scale_image(z3);
		copy_surface_and_query_palette(z3);
	}
	{
		Mailbox z2_copy(4 * 1024 * 1024);
		copy_surface_and_query_palette(z2_copy);
	}
	check_client_release(mailbox);
	mailbox.sdk.reset();
	assert(mailbox.sdk.buffer_data(1) == nullptr);
}
