#include "zz9000_sdk.h"

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <vector>
#ifdef HAVE_MPG123
#include <cfenv>
#include <fstream>
#include <iterator>
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "target.h"
#include <SDL3/SDL.h>
uae_prefs currprefs{};
sound_device *sound_devices[MAX_SOUND_DEVICES]{};
bool sound_paused() { return false; }
bool sound_muted() { return false; }
#endif

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

	const uint8_t *call(uint16_t opcode, const uint8_t *payload, uint16_t length,
	                    uint16_t expected_status = 0) {
		auto *mb = board.data() + zz9000_sdk::mailbox_offset;
		const uint32_t tail = get32(mb + 24);
		uint8_t *request = mb + 128 + tail * 64;
		for (int i = 0; i < 64; ++i)
			request[i] = 0;
		put32(request, ++request_id);
		put16(request + 4, opcode);
		put16(request + 10, length);
		for (uint16_t i = 0; i < length && i < 48; ++i)
			request[16 + i] = payload[i];
		put32(mb + 24, (tail + 1) % 64);
		sdk.poll();
		const auto *reply = mb + 128 + 64 * 64 + completion_head * 64;
		assert(get32(reply) == request_id);
		assert(get16(reply + 4) == opcode);
		assert(get16(reply + 6) == expected_status);
		completion_head = (completion_head + 1) % 64;
		put32(mb + 36, completion_head);
		return reply;
	}
};
}

int main(int argc, char **argv)
{
	Mailbox z2(4 * 1024 * 1024);
	auto *mb = z2.board.data() + zz9000_sdk::mailbox_offset;
	assert(z2.sdk.read_register(0x100) == 0x5a39);
	const uint32_t advertised_mailbox =
		(static_cast<uint32_t>(z2.sdk.read_register(0x104)) << 16) |
		z2.sdk.read_register(0x106);
	assert(advertised_mailbox == 0x3fe40000u +
	       (zz9000_sdk::mailbox_offset - 0xa000u));
	assert(get32(mb) == 0x5a5a394b);
	uint32_t expected_caps = (1u << 0) | (1u << 2) | (1u << 13) | (1u << 14);
#ifdef HAVE_MPG123
	expected_caps |= (1u << 19) | (1u << 23);
#endif
	assert(get32(mb + 44) == expected_caps);
	uint8_t payload[48] = {};
	const auto *reply = z2.call(0x0001, nullptr, 0);
	assert(get32(reply + 16) == 0x5a5a394b);
	assert(get32(reply + 52) == 0x10000);
	put32(payload, 65536);
	put32(payload + 4, 16);
	put32(payload + 8, 1);
	reply = z2.call(0x0100, payload, 12);
	const uint32_t staging = get32(reply + 16);
	assert(staging != 0);
	assert(get32(reply + 20) == 0x005d0000);
	assert(z2.sdk.buffer_length(staging) == 65536);
	assert(z2.sdk.buffer_data(staging) == z2.board.data() + 0x3e0000);
	put32(payload, staging);
	z2.call(0x0101, payload, 4);
	assert(z2.sdk.buffer_data(staging) == nullptr);
	put32(payload, 0);
	z2.call(0x0101, payload, 4, 5);
	put32(payload, 0x500);
	z2.call(0x0004, payload, 4,
#ifdef HAVE_MPG123
		0
#else
		10
#endif
	);
	z2.call(0x0002, payload, 49, 4);
	for (int i = 0; i < 140; ++i)
		z2.call(0x0002, payload, 4);
	z2.sdk.reset();
	assert(get32(mb + 20) == 0 && get32(mb + 40) == 0);
	assert(z2.sdk.buffer_data(staging) == nullptr);
	Mailbox z3(128 * 1024 * 1024);
	put32(payload, 256 * 1024);
	put32(payload + 4, 16);
	put32(payload + 8, 0);
	reply = z3.call(0x0100, payload, 12);
	assert(get32(reply + 20) == 0x061f0000);
	assert(z3.sdk.buffer_data(get32(reply + 16)) == z3.board.data() + 0x06000000);
#ifdef HAVE_MPG123
	assert(argc == 2);
	std::ifstream file(argv[1], std::ios::binary);
	std::vector<uint8_t> mp3(std::istreambuf_iterator<char>{file}, {});
	assert(!mp3.empty() && mp3.size() < 65536);
	assert(SDL_Init(SDL_INIT_AUDIO));
	put32(payload, 65536);
	put32(payload + 4, 16);
	put32(payload + 8, 0);
	reply = z3.call(0x0100, payload, 12);
	const uint32_t audio_staging = get32(reply + 16);
	std::copy(mp3.begin(), mp3.end(), z3.sdk.buffer_data(audio_staging));
	put32(payload, 131072);
	reply = z3.call(0x0100, payload, 12);
	const uint32_t compressed = get32(reply + 16);
	put32(payload, 262144);
	reply = z3.call(0x0100, payload, 12);
	const uint32_t pcm = get32(reply + 16);
	for (int i = 0; i < 48; ++i) payload[i] = 0;
	put32(payload, compressed);
	put32(payload + 4, 131072);
	put32(payload + 8, pcm);
	put32(payload + 12, 262144);
	put32(payload + 24, 1);
	put32(payload + 28, 32768);
	const int previous_rounding = std::fegetround();
	assert(std::fesetround(FE_DOWNWARD) == 0);
	reply = z3.call(0x0503, payload, 40);
	assert(std::fegetround() == FE_DOWNWARD);
	assert(std::fesetround(previous_rounding) == 0);
	const uint32_t session = get32(reply + 16);
	assert(session);
	put32(payload, session);
	put32(payload + 4, audio_staging);
	put32(payload + 8, 0);
	put32(payload + 12, static_cast<uint32_t>(mp3.size()));
	put32(payload + 16, 0);
	assert(std::fesetround(FE_DOWNWARD) == 0);
	reply = z3.call(0x0504, payload, 20);
	assert(std::fegetround() == FE_DOWNWARD);
	assert(std::fesetround(previous_rounding) == 0);
	assert(get32(reply + 24) == 48000);
	assert(get32(reply + 52) == mp3.size());
	assert(get32(reply + 60) & 2);
	put32(payload, compressed);
	z3.call(0x0101, payload, 4, 2); // active ring is pinned
	put32(payload, session);
	put32(payload + 4, 0);
	reply = z3.call(0x0507, payload, 8);
	assert(get32(reply + 16) == session);
	z3.call(0x0508, payload, 8);
	z3.call(0x0506, payload, 8);
	put32(payload, compressed);
	z3.call(0x0101, payload, 4);
	SDL_Quit();
#else
	(void)argc;
	(void)argv;
#endif
}
