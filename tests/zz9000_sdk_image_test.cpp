#include "zz9000_sdk.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
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

	explicit Mailbox(uint32_t size) : board(size), sdk(board.data(), size) {}

	const uint8_t *call(uint16_t opcode, const uint8_t *payload,
	                    uint16_t length, uint16_t status = 0) {
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
		sdk.poll();
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
	reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 1 && get32(reply + 36) == image.size() / 2);
	put32(request + 8, static_cast<uint32_t>(image.size() / 2));
	put32(request + 12, static_cast<uint32_t>(image.size() - image.size() / 2));
	put32(request + 16, 1); // EOF
	reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 3 && get32(reply + 8) == 2 &&
	       get32(reply + 12) == 3 && get32(reply + 24) == 0 &&
	       get32(reply + 28) == 2 && get32(reply + 32) == 1);
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
	}
	reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 4); // COMPLETE
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
	put32(request + 12, static_cast<uint32_t>(image.size()));
	put32(request + 16, 1);
	reply = mailbox.call(0x0405, request, 48);
	assert(get32(reply + 4) == 4 && get32(reply + 8) == 2 &&
	       get32(reply + 12) == 3 && get32(reply + 28) == 2 &&
	       get32(reply + 32) == 3 && get32(reply + 40) == 24);
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
} // namespace

int main(int argc, char **argv)
{
	assert(argc == 3);
	Mailbox mailbox(4 * 1024 * 1024);
	assert((get32(mailbox.board.data() + zz9000_sdk::mailbox_offset + 44) &
	        (1u << 5)) != 0);
	uint8_t request[48] = {};
	put32(request, 0x400);
	const auto *reply = mailbox.call(0x0004, request, 4);
	assert(get32(reply + 8) == ((1u << 5) | (1u << 6)));
	assert((get32(reply + 12) & ((1u << 20) | (1u << 21) | (1u << 26))) ==
	       ((1u << 20) | (1u << 21) | (1u << 26)));
	run_image(mailbox, read_file(argv[1]), 2, 8); // PNG RGB888
	run_image(mailbox, read_file(argv[1]), 2, 7); // PNG BGRA8888
	run_image(mailbox, read_file(argv[2]), 1, 8); // JPEG RGB888
	run_viewer_surface(mailbox, read_file(argv[1]), 2, 7);
	run_viewer_surface(mailbox, read_file(argv[1]), 2, 1);
	run_viewer_surface(mailbox, read_file(argv[2]), 1, 7);
	reject_16bit_decode_surface(mailbox);
	mailbox.sdk.reset();
	assert(mailbox.sdk.buffer_data(1) == nullptr);
}
