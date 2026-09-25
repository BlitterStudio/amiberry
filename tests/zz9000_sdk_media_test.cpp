#include "zz9000_sdk.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
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
	uint32_t request_id = 0, completion_head = 0;
	uint16_t last_status = 0;
	bool last_dirty = false;

	Mailbox() : board(128 * 1024 * 1024), sdk(board.data(), board.size()) {}

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
		last_dirty = sdk.poll();
		const auto *reply = mb + 128 + 64 * 64 + completion_head * 64;
		assert(get32(reply) == request_id);
		last_status = get16(reply + 6);
		assert(status == 0xffff || last_status == status);
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
}

int main(int argc, char **argv)
{
	assert(argc == 2 || argc == 3 || argc == 4 || argc == 5);
	const uint32_t width = argc >= 4 ? static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10)) : 160;
	const uint32_t height = argc >= 4 ? static_cast<uint32_t>(std::strtoul(argv[3], nullptr, 10)) : 120;
	const bool full = argc == 5;
	std::ifstream file(argv[1], std::ios::binary);
	std::vector<uint8_t> video{std::istreambuf_iterator<char>{file}, {}};
	assert(!video.empty() && (full || video.size() < 256 * 1024));
	Mailbox mailbox;
	auto *mb = mailbox.board.data() + zz9000_sdk::mailbox_offset;
	assert((get32(mb + 44) & ((1u << 7) | (1u << 21) | (1u << 22))) ==
	       ((1u << 7) | (1u << 21) | (1u << 22)));
	uint8_t request[48] = {};
	put32(request, 0x0b00);
	const auto *reply = mailbox.call(0x0004, request, 4);
	assert(get32(reply + 20) == 14 &&
	       (get32(reply + 12) & ((1u << 16) | (1u << 17) | (1u << 22))) ==
	       ((1u << 16) | (1u << 17) | (1u << 22)));
	const uint32_t staging = mailbox.allocate(65536);
	const uint32_t pcm = mailbox.allocate(131072);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 1); put32(request + 4, 1);
	put32(request + 8, width); put32(request + 12, height);
	put32(request + 16, 1); put32(request + 20, 1);
	put32(request + 24, pcm); put32(request + 28, 131072);
	put32(request + 32, 24576); put32(request + 36, 98304);
	reply = mailbox.call(0x0b04, request, 44);
	const uint32_t session = get32(reply);
	assert(session && get32(reply + 4) == 1);
	assert(get32(reply + 36) == 0);
	mailbox.call(0x0b04, request, 44, 2); // one decoder owns the media service
	put32(request, pcm);
	mailbox.call(0x0101, request, 4, 2);
	mailbox.sdk.set_overlay(0x03310000, width, height, width * 2, 0, true);
	if (full) {
		size_t offset = 0;
		uint64_t acknowledged = 0;
		uint64_t produced = 0;
		uint32_t frames = 0;
		bool done = false;
		for (unsigned iteration = 0; iteration < 200000 && !done; ++iteration) {
			if (offset < video.size()) {
				const uint32_t length = static_cast<uint32_t>(
					std::min<size_t>(16384, video.size() - offset));
				std::copy(video.begin() + offset, video.begin() + offset + length,
					mailbox.sdk.buffer_data(staging));
				std::fill(std::begin(request), std::end(request), 0);
				put32(request, session); put32(request + 4, staging);
				put32(request + 12, length);
				put32(request + 16, offset + length == video.size() ? 1 : 0);
				reply = mailbox.call(0x0b05, request, 20, 0xffff);
				assert(mailbox.last_status == 0 || mailbox.last_status == 2);
				if (mailbox.last_status == 0) {
					assert(get32(reply + 36) == offset + get32(reply + 40));
					offset += get32(reply + 40);
				}
			}
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session);
			put32(request + 4, static_cast<uint32_t>(acknowledged >> 32));
			put32(request + 8, static_cast<uint32_t>(acknowledged));
			reply = mailbox.call(0x0b07, request, 16);
			produced = (static_cast<uint64_t>(get32(reply + 20)) << 32) |
				get32(reply + 24);
			acknowledged = produced;
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session);
			reply = mailbox.call(0x0b06, request, 16, 0xffff);
			assert(mailbox.last_status == 0 || mailbox.last_status == 2);
			if (mailbox.last_status == 0 && get32(reply + 4) == 3) {
				const bool present = frames % 100 == 0;
				mailbox.call(present ? 0x0b08 : 0x0b09, request, 16);
				assert(mailbox.last_dirty == present);
				frames++;
			} else if (mailbox.last_status == 0 && get32(reply + 4) == 4) {
				done = true;
			}
		}
		assert(done && offset == video.size() && frames > 10 && produced > 0);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		mailbox.call(0x0b0d, request, 16);
		assert(std::any_of(mailbox.board.data() + 0x03310000,
			mailbox.board.data() + 0x03310000 + width * height * 2,
			[](uint8_t value) { return value != 0; }));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, 1); put32(request + 4, 1);
		put32(request + 8, width); put32(request + 12, height);
		put32(request + 16, 1); put32(request + 20, 1);
		put32(request + 24, pcm); put32(request + 28, 131072);
		put32(request + 32, 24576); put32(request + 36, 98304);
		reply = mailbox.call(0x0b04, request, 44);
		assert(get32(reply) != session);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, get32(reply));
		mailbox.call(0x0b0d, request, 16);
		return 0;
	}
	for (size_t offset = 0; offset < video.size();) {
		const uint32_t length = static_cast<uint32_t>(
			std::min<size_t>(16384, video.size() - offset));
		std::copy(video.begin() + offset, video.begin() + offset + length,
			mailbox.sdk.buffer_data(staging));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, staging);
		put32(request + 12, length);
		put32(request + 16, offset + length == video.size() ? 1 : 0);
		reply = mailbox.call(0x0b05, request, 20);
		assert(get32(reply + 40) == length);
		assert(get32(reply + 36) == offset + length);
		offset += length;
	}
	bool shown = false;
	for (int i = 0; i < 100 && !shown; ++i) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		reply = mailbox.call(0x0b06, request, 16);
		if (get32(reply + 4) == 3) {
			assert(get32(reply + 8) == width && get32(reply + 12) == height);
			reply = mailbox.call(0x0b08, request, 16);
			assert(get32(reply + 44) & (1u << 9));
			assert(mailbox.last_dirty);
			shown = true;
		}
	}
	assert(shown);
	const auto *overlay = mailbox.board.data() + 0x03310000;
	assert(std::any_of(overlay, overlay + width * height * 2,
		[](uint8_t value) { return value != 0; }));
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	reply = mailbox.call(0x0b07, request, 16);
	assert(get32(reply + 8) == 44100 && get32(reply + 12) == 2);
	assert(get32(reply + 16) == 2 && get32(reply + 24) > 0);
	put32(request, session);
	mailbox.call(0x0b0d, request, 16);
	if (argc == 3) {
		std::ifstream audio_file(argv[2], std::ios::binary);
		const std::vector<uint8_t> audio_only{
			std::istreambuf_iterator<char>{audio_file}, {}};
		assert(!audio_only.empty() && audio_only.size() <= 65536);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, 1); put32(request + 4, 1);
		put32(request + 8, width); put32(request + 12, height);
		put32(request + 16, 1); // MP2 output disabled; video is still required.
		reply = mailbox.call(0x0b04, request, 44);
		const uint32_t audio_only_session = get32(reply);
		std::copy(audio_only.begin(), audio_only.end(),
			mailbox.sdk.buffer_data(staging));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, audio_only_session); put32(request + 4, staging);
		put32(request + 12, static_cast<uint32_t>(audio_only.size()));
		mailbox.call(0x0b05, request, 20);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, audio_only_session);
		mailbox.call(0x0b06, request, 16, 9); // Ready PS headers contain no video.
		put32(request + 4, staging); put32(request + 16, 1);
		mailbox.call(0x0b05, request, 20); // EOF cannot supply video.
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, audio_only_session);
		mailbox.call(0x0b06, request, 16, 9);
		mailbox.call(0x0b0d, request, 16);
	}
	for (const std::vector<uint8_t> invalid_stream : {
		std::vector<uint8_t>{'n', 'o', 't', ' ', 'm', 'p', 'e', 'g'},
		std::vector<uint8_t>{0, 0, 1, 0xba}
	}) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, 1); put32(request + 4, 1);
		put32(request + 8, width); put32(request + 12, height);
		put32(request + 16, 1);
		reply = mailbox.call(0x0b04, request, 44);
		const uint32_t invalid_session = get32(reply);
		std::copy(invalid_stream.begin(), invalid_stream.end(),
			mailbox.sdk.buffer_data(staging));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, invalid_session); put32(request + 4, staging);
		put32(request + 12, static_cast<uint32_t>(invalid_stream.size()));
		mailbox.call(0x0b05, request, 20);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, invalid_session);
		reply = mailbox.call(0x0b06, request, 16);
		assert(get32(reply + 4) == 1); // More input could still complete the headers.
		put32(request + 4, staging); put32(request + 16, 1);
		mailbox.call(0x0b05, request, 20); // EOF with no further bytes.
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, invalid_session);
		mailbox.call(0x0b06, request, 16, 9); // EOF cannot complete these headers.
		mailbox.call(0x0b06, request, 16, 9);
		mailbox.call(0x0b0d, request, 16);
	}
	put32(request, pcm);
	mailbox.call(0x0101, request, 4);
	put32(request, staging);
	mailbox.call(0x0101, request, 4);
}
