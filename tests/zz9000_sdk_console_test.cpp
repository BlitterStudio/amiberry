#include "zz9000_sdk.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
uint16_t get16(const uint8_t *p) { return static_cast<uint16_t>(p[0] << 8 | p[1]); }
uint32_t get32(const uint8_t *p) { return static_cast<uint32_t>(get16(p)) << 16 | get16(p + 2); }
void put16(uint8_t *p, uint16_t value) { p[0] = value >> 8; p[1] = value; }
void put32(uint8_t *p, uint32_t value) { put16(p, value >> 16); put16(p + 2, value); }

struct Mailbox {
	std::vector<uint8_t> board;
	zz9000_sdk::Engine sdk;
	uint32_t request_id = 0;
	uint32_t completion_head = 0;

	explicit Mailbox(uint32_t size) : board(size), sdk(board.data(), size) {}

	const uint8_t *call(uint16_t opcode, const uint8_t *payload, uint16_t length,
		uint16_t expected = 0)
	{
		auto *mailbox = board.data() + zz9000_sdk::mailbox_offset;
		const uint32_t tail = get32(mailbox + 24);
		auto *request = mailbox + 128 + tail * 64;
		std::memset(request, 0, 64);
		put32(request, ++request_id);
		put16(request + 4, opcode);
		put16(request + 10, length);
		if (payload)
			std::memcpy(request + 16, payload, length);
		put32(mailbox + 24, (tail + 1) % 64);
		sdk.poll();
		const auto *reply = mailbox + 128 + 64 * 64 + completion_head * 64;
		assert(get32(reply) == request_id);
		assert(get16(reply + 4) == opcode);
		assert(get16(reply + 6) == expected);
		completion_head = (completion_head + 1) % 64;
		put32(mailbox + 36, completion_head);
		return reply;
	}

	uint32_t alloc(uint32_t length, uint32_t flags = 0, uint32_t alignment = 16)
	{
		uint8_t request[12] = {};
		put32(request, length);
		put32(request + 4, alignment);
		put32(request + 8, flags);
		return get32(call(0x0100, request, sizeof(request)) + 16);
	}
};

void console_request(uint8_t *request, uint32_t output, uint16_t width,
	uint16_t height, uint16_t pitch, uint8_t format, uint8_t tile_width,
	uint8_t tile_height, uint16_t flags = 1)
{
	std::memset(request, 0, 48);
	put32(request, 0x80000000);
	put32(request + 4, output);
	put32(request + 8, 4096);
	put16(request + 16, width);
	put16(request + 18, height);
	put16(request + 20, pitch);
	put16(request + 22, 0);
	put16(request + 24, (height + tile_height - 1) / tile_height);
	put16(request + 26, flags);
	request[30] = format == 2 ? 16 : 8;
	request[31] = tile_width;
	request[32] = tile_height;
	request[33] = format;
}
}

int main()
{
	uint8_t request[48] = {};
	Mailbox z2(4 * 1024 * 1024);
	auto *z2_mailbox = z2.board.data() + zz9000_sdk::mailbox_offset;
	assert(z2.sdk.read_register(0x11c) == 0x5a02);
	assert(z2.sdk.read_register(0x11e) == 0x0704);
	assert((get32(z2_mailbox + 44) & ((1u << 20) | (1u << 24))) == 0);
	z2.sdk.write_register(0x11e, 0xa502);
	assert((get32(z2_mailbox + 44) & ((1u << 20) | (1u << 24))) ==
		((1u << 20) | (1u << 24)));
	const auto *reply = z2.call(0x0005, nullptr, 0);
	assert(get32(reply + 16) == 0x0002000f);
	assert(get32(reply + 20) == 0x00400000);
	assert(get32(reply + 48) == 0x003e0000);
	assert(get32(reply + 52) == 0x4000);
	put32(request, 0x100);
	reply = z2.call(0x0004, request, 4);
	assert(get32(reply + 24) == ((1u << 2) | (1u << 10) | (1u << 20)));
	assert(z2.alloc(64, 1));

	Mailbox z3(128 * 1024 * 1024);
	auto *z3_mailbox = z3.board.data() + zz9000_sdk::mailbox_offset;
	assert((get32(z3_mailbox + 44) & ((1u << 10) | (1u << 30))) ==
		((1u << 10) | (1u << 30)));
	put32(request, 0x100);
	reply = z3.call(0x0004, request, 4);
	assert(get32(reply + 24) == ((1u << 2) | (1u << 10) | (1u << 30)));
	assert(get32(reply + 28) == (1u | 1u << 16));
	assert(get32(reply + 36) == 4);

	const uint32_t staging = z3.alloc(64);
	const uint32_t media = z3.alloc(64, 6);
	assert(media && z3.sdk.buffer_length(media) == 64);
	assert(z3.sdk.buffer_data(media));
	std::memset(z3.sdk.buffer_data(staging), 0x3c, 64);
	put32(request, media); put32(request + 4, 0); put32(request + 8, staging);
	put32(request + 12, 0); put32(request + 16, 64);
	z3.call(0x0103, request, 20);
	for (unsigned i = 0; i < 64; ++i)
		assert(z3.sdk.buffer_data(media)[i] == 0x3c);
	put32(request, media); put32(request + 4, 4); put32(request + 8, 4); request[12] = 0x5a;
	z3.call(0x0102, request, 13);
	for (unsigned i = 4; i < 8; ++i)
		assert(z3.sdk.buffer_data(media)[i] == 0x5a);
	put32(request, media); z3.call(0x0101, request, 4);
	z3.call(0x0101, request, 4, 5);
	put32(request, 64); put32(request + 4, 16); put32(request + 8, 4);
	z3.call(0x0100, request, 12, 4);
	put32(request + 8, 6); z2.call(0x0100, request, 12, 3);
	put32(request, 64 * 1024 * 1024 + 1); put32(request + 8, 6);
	z3.call(0x0100, request, 12, 4);
	// All media-input buffers share one 64 MiB budget, as the card's pool
	// class does: a second 40 MiB buffer fits only after the first is freed.
	const uint32_t big = z3.alloc(40 * 1024 * 1024, 6);
	assert(big);
	put32(request, 40 * 1024 * 1024); put32(request + 4, 16); put32(request + 8, 6);
	z3.call(0x0100, request, 12, 6);
	put32(request, big); z3.call(0x0101, request, 4);
	const uint32_t big_again = z3.alloc(40 * 1024 * 1024, 6);
	assert(big_again);
	put32(request, big_again); z3.call(0x0101, request, 4);
	const uint32_t reset_media = z3.alloc(64, 6);
	z3.sdk.reset();
	assert(z3.sdk.buffer_data(reset_media) == nullptr);
	z3.request_id = 0;
	z3.completion_head = 0;

	const uint32_t output = z3.alloc(4096);
	put32(request, 0x8200);
	reply = z3.call(0x0004, request, 4);
	assert(get32(reply + 16) == 0x8200 && get32(reply + 20) == 0x00020000);
	assert(get32(reply + 24) == 0 && get32(reply + 28) == 1);
	assert(get32(reply + 32) == 0x8200 && get32(reply + 36) == 1);
	assert(std::memcmp(reply + 44, "console", 7) == 0);

	const uint8_t bgra[] = {0, 0, 255, 1, 255, 0, 0, 1, 0, 255, 0, 1, 255, 255, 255, 1};
	z3.sdk.set_framebuffer(0x10000, 2, 2, 8, 7);
	auto *pixels = z3.board.data() + 0x10000;
	std::memcpy(pixels, bgra, sizeof(bgra));
	console_request(request, output, 2, 2, 4, 2, 4, 2);
	reply = z3.call(0x8200, request, 34);
	assert(get32(reply + 16) == 17 && get16(reply + 22) == 1);
	const uint8_t expected_bgra[] = {1, 0, 0, 0, 3, 0, 0, 0, 0xf8, 0, 0, 0x1f, 0x07, 0xe0, 0xff, 0xff, 0};
	assert(std::memcmp(z3.sdk.buffer_data(output), expected_bgra, sizeof(expected_bgra)) == 0);
	put16(request + 26, 0);
	reply = z3.call(0x8200, request, 34);
	assert(get32(reply + 16) == 5 && get16(reply + 22) == 0);
	// An ordinary incremental pass must see framebuffer changes made since
	// the previous pass, without a reset.
	pixels[0] = 255;
	pixels[2] = 0;
	reply = z3.call(0x8200, request, 34);
	assert(get32(reply + 16) > 5 && get16(reply + 22) == 0);
	console_request(request, output, 2, 2, 4, 2, 4, 1, 0);
	z3.call(0x8200, request, 34);
	put16(request + 22, 1);
	put16(request + 24, 2);
	put16(request + 26, 1);
	z3.call(0x8200, request, 34);
	// D5: a converted RGB565 row may be wider than the 32-bit source row.
	// The firmware bounds it only by >= 2*width, evenness (D4) and surface
	// reachability, so bpr past the surface pitch is accepted and encoded
	// from row-by-row reads of the surface, never past its length.
	console_request(request, output, 2, 2, 12, 2, 4, 2);
	reply = z3.call(0x8200, request, 34);
	assert(get16(reply + 22) == 1);
	assert(z3.sdk.buffer_data(output)[0] == 1);
	// The same wide geometry again is an empty pass against the shadow.
	console_request(request, output, 2, 2, 12, 2, 4, 2, 0);
	reply = z3.call(0x8200, request, 34);
	assert(get32(reply + 16) == 5 && get16(reply + 22) == 0);
	// A row narrower than two bytes per pixel is still refused.
	console_request(request, output, 2, 2, 2, 2, 4, 2);
	z3.call(0x8200, request, 34, 4);
	// D4: RGB565 needs an even bpr and an even tile width in bytes -- an
	// odd row splits a pixel across rows, an odd tile across tiles.
	console_request(request, output, 2, 2, 5, 2, 4, 2);
	z3.call(0x8200, request, 34, 4);
	console_request(request, output, 2, 2, 4, 2, 3, 2);
	z3.call(0x8200, request, 34, 4);
	// D2: an invalid band is checked after the reconfigure and snapshot, as
	// in the firmware's rfb_encode_band(): INTERNAL_ERROR, session dropped.
	console_request(request, output, 2, 2, 12, 2, 4, 2, 0);
	put16(request + 22, 1);
	put16(request + 24, 0);
	z3.call(0x8200, request, 34, 65535);
	console_request(request, output, 2, 2, 12, 2, 4, 2, 0);
	put16(request + 24, 3);
	z3.call(0x8200, request, 34, 65535);
	// The dropped session restarts: a clean band 0 without a reset flag is
	// still a fresh keyframe at sequence 0, not an incremental pass.
	console_request(request, output, 2, 2, 12, 2, 4, 2, 0);
	reply = z3.call(0x8200, request, 34);
	assert(get16(reply + 22) == 1);
	assert(get16(z3.sdk.buffer_data(output) + 2) == 0);
	// D3: another output buffer is another owner.  The session and snapshot
	// are dropped and the sequence restarts at 0; the new owner's first
	// request may not begin past band 0.
	const uint32_t second = z3.alloc(4096);
	console_request(request, second, 2, 2, 12, 2, 4, 2, 0);
	put16(request + 22, 1);
	z3.call(0x8200, request, 34, 4);
	// Its band 0 encodes a full frame from the zeroed shadow; the configure
	// already happened, so no keyframe flag, but the sequence is new.
	console_request(request, second, 2, 2, 12, 2, 4, 2, 0);
	reply = z3.call(0x8200, request, 34);
	assert(get16(reply + 22) == 0);
	assert(get16(z3.sdk.buffer_data(second) + 2) == 0);
	assert(get32(reply + 16) > 5);
	// Returning to the first owner at band 0 is a reconfigure: a keyframe
	// at sequence 0.
	console_request(request, output, 2, 2, 12, 2, 4, 2, 0);
	reply = z3.call(0x8200, request, 34);
	assert(get16(reply + 22) == 1);
	assert(get16(z3.sdk.buffer_data(output) + 2) == 0);

	z3.sdk.set_framebuffer(0x10000, 2, 1, 4, 1);
	const uint8_t rgb565[] = {0xf8, 0, 0, 0x1f};
	std::memcpy(pixels, rgb565, sizeof(rgb565));
	console_request(request, output, 2, 1, 4, 2, 4, 1);
	reply = z3.call(0x8200, request, 34);
	const uint8_t expected_rgb565[] = {1, 0, 0, 1, 3, 0, 0, 0, 0xf8, 0, 0, 0x1f, 0};
	assert(get32(reply + 16) == sizeof(expected_rgb565));
	assert(std::memcmp(z3.sdk.buffer_data(output), expected_rgb565, sizeof(expected_rgb565)) == 0);
	// D4 on a native RGB565 surface: an odd tile width is refused there too.
	console_request(request, output, 2, 1, 4, 2, 3, 1);
	z3.call(0x8200, request, 34, 4);

	z3.sdk.set_framebuffer(0x10000, 2, 1, 4, 6);
	console_request(request, output, 2, 1, 4, 2, 4, 1);
	z3.call(0x8200, request, 34, 4);

	// D1: an 8-bit CLUT screen is the firmware's INDEX8 surface (4) in the
	// 0x0202 reply, and CLUT8 encodes straight from it.
	z3.sdk.set_framebuffer(0x10000, 2, 2, 2, 4);
	const uint8_t clut[] = {10, 20, 30, 40};
	std::memcpy(pixels, clut, sizeof(clut));
	reply = z3.call(0x0202, request, 0);
	assert(get32(reply + 24) == 2 && get32(reply + 28) == 2 &&
		get32(reply + 32) == 2 && get32(reply + 36) == 4 &&
		get32(reply + 44) == 4);
	const uint32_t clut_output = z3.alloc(4096);
	console_request(request, clut_output, 2, 2, 2, 1, 2, 2);
	reply = z3.call(0x8200, request, 34);
	const uint8_t expected_clut[] = {1, 0, 0, 0, 3, 0, 0, 0, 10, 20, 30, 40, 0};
	assert(get32(reply + 16) == sizeof(expected_clut));
	assert(get16(reply + 22) == 1);
	assert(std::memcmp(z3.sdk.buffer_data(clut_output), expected_clut,
		sizeof(expected_clut)) == 0);
	// CLUT8 is not served from a non-INDEX8 surface.
	z3.sdk.set_framebuffer(0x10000, 2, 1, 4, 1);
	console_request(request, clut_output, 2, 1, 4, 1, 4, 1);
	z3.call(0x8200, request, 34, 4);
	return 0;
}
