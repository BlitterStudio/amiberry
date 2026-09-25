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

const uint8_t *begin_audio(Mailbox &mailbox, uint32_t compressed,
	                       uint32_t compressed_capacity, uint32_t pcm,
	                       uint32_t pcm_capacity, uint32_t format,
	                       uint32_t low_water = 0)
{
	uint8_t request[48] = {};
	put32(request, compressed);
	put32(request + 4, compressed_capacity);
	put32(request + 8, pcm);
	put32(request + 12, pcm_capacity);
	put32(request + 24, format);
	put32(request + 28, low_water);
	return mailbox.call(0x0503, request, 40);
}
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
	uint32_t expected_caps = (1u << 0) | (1u << 2) | (1u << 3) |
		(1u << 4) | (1u << 5) | (1u << 6) | (1u << 13) |
		(1u << 14) | (1u << 15);
#ifdef HAVE_MPG123
	expected_caps |= (1u << 7) | (1u << 19) | (1u << 23);
#endif
	assert(get32(mb + 44) == expected_caps);
	{
		Mailbox doorbell(4 * 1024 * 1024);
		doorbell.sdk.set_framebuffer(0x110000, 2, 2, 8, 7);
		auto *ring = doorbell.board.data() + zz9000_sdk::mailbox_offset;
		auto *request = ring + 128;
		put32(request, 1);
		put16(request + 4, 0x0203); // Fill the visible framebuffer.
		put16(request + 10, 28);
		put32(request + 16, 0x80000000); // Framebuffer handle.
		put32(request + 28, 2); // Width.
		put32(request + 32, 2); // Height.
		put32(request + 36, 0x00ff0000); // Red.
		put32(ring + 24, 1);
		assert(!doorbell.sdk.write_register(0x108, 0));
		assert(get32(ring + 20) == 0);
		assert(doorbell.sdk.write_register(0x108, 1));
		assert(get32(ring + 20) == 1);
		assert(get16(ring + 128 + 64 * 64 + 6) == 0);
		assert(doorbell.board[0x110002] == 0xff);
		assert(!doorbell.sdk.poll()); // The doorbell consumed the request.
		request += 64;
		put32(request, 2);
		put16(request + 4, 0x0001); // Nonvisual query.
		put32(ring + 24, 2);
		assert(!doorbell.sdk.write_register(0x108, 1));
		assert(get32(ring + 20) == 2);
	}
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
	z2.request_id = 0;
	z2.completion_head = 0;
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
	assert(argc == 4);
	std::ifstream file(argv[1], std::ios::binary);
	std::vector<uint8_t> mp3(std::istreambuf_iterator<char>{file}, {});
	assert(!mp3.empty() && mp3.size() < 65536);
	std::ifstream mono_file(argv[2], std::ios::binary);
	std::vector<uint8_t> mono_mp3(std::istreambuf_iterator<char>{mono_file}, {});
	assert(!mono_mp3.empty() && mono_mp3.size() < 65536);
	std::ifstream low_file(argv[3], std::ios::binary);
	std::vector<uint8_t> low_mp3(std::istreambuf_iterator<char>{low_file}, {});
	assert(!low_mp3.empty() && low_mp3.size() < 65536);
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
	const int previous_rounding = std::fegetround();
	assert(std::fesetround(FE_DOWNWARD) == 0);
	reply = begin_audio(z3, compressed, 131072, pcm, 262144, 1, 32768);
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
	assert(get32(reply + 56) >= 256);
	std::vector<uint8_t> little_pcm(z3.sdk.buffer_data(pcm),
	                                z3.sdk.buffer_data(pcm) + 256);
	put32(payload, compressed);
	z3.call(0x0101, payload, 4, 2); // active ring is pinned
	put32(payload, session);
	put32(payload + 4, 0);
	reply = z3.call(0x0507, payload, 8);
	assert(get32(reply + 16) == session);
	z3.call(0x0508, payload, 8);
	z3.call(0x0506, payload, 8);
	assert(std::fesetround(FE_DOWNWARD) == 0);
	reply = begin_audio(z3, compressed, 131072, pcm, 262144, 2);
	assert(std::fegetround() == FE_DOWNWARD);
	assert(std::fesetround(previous_rounding) == 0);
	const uint32_t mpega_session = get32(reply + 16);
	assert(mpega_session);
	put32(payload, mpega_session);
	put32(payload + 4, audio_staging);
	put32(payload + 8, 0);
	put32(payload + 12, 7);
	put32(payload + 16, 0);
	assert(std::fesetround(FE_DOWNWARD) == 0);
	reply = z3.call(0x0504, payload, 20);
	assert(std::fegetround() == FE_DOWNWARD);
	assert(std::fesetround(previous_rounding) == 0);
	assert(get32(reply + 56) == 0); // incomplete MPEG header
	put32(payload + 8, 7);
	put32(payload + 12, static_cast<uint32_t>(mp3.size() - 7));
	reply = z3.call(0x0504, payload, 20);
	assert(get32(reply + 56) >= little_pcm.size());
	assert(get32(reply + 32) == 2);
	const auto *big_pcm = z3.sdk.buffer_data(pcm);
	for (size_t i = 0; i < little_pcm.size(); i += 2) {
		assert(big_pcm[i] == little_pcm[i + 1]);
		assert(big_pcm[i + 1] == little_pcm[i]);
	}
	put32(payload, mpega_session);
	put32(payload + 4, 0);
	z3.call(0x0507, payload, 8, 3); // S16BE is read-only
	put32(payload + 4, 256);
	put32(payload + 8, 0);
	reply = z3.call(0x0505, payload, 12);
	assert(get32(reply + 44) == 256); // PCM read cursor
	put32(payload + 4, 0);
	z3.call(0x0506, payload, 8);
	std::copy(mono_mp3.begin(), mono_mp3.end(), z3.sdk.buffer_data(audio_staging));
	reply = begin_audio(z3, compressed, 131072, pcm, 262144, 2);
	const uint32_t mono_session = get32(reply + 16);
	put32(payload, mono_session);
	put32(payload + 4, audio_staging);
	put32(payload + 8, 0);
	put32(payload + 12, static_cast<uint32_t>(mono_mp3.size()));
	put32(payload + 16, 0);
	reply = z3.call(0x0504, payload, 20);
	assert(get32(reply + 24) == 44100);
	assert(get32(reply + 28) == 1);
	const uint32_t mono_bytes = get32(reply + 56);
	assert(mono_bytes > 0);
	put32(payload + 12, 0);
	put32(payload + 16, 1); // EOF after the final short input
	reply = z3.call(0x0504, payload, 20);
	assert((get32(reply + 60) & 4) == 0); // PCM still available to read
	put32(payload + 4, mono_bytes);
	put32(payload + 8, 0);
	reply = z3.call(0x0505, payload, 12);
	assert(get32(reply + 60) & 4); // consumed PCM and EOF
	put32(payload + 4, 0);
	z3.call(0x0506, payload, 8);
	std::copy(low_mp3.begin(), low_mp3.end(), z3.sdk.buffer_data(audio_staging));
	reply = begin_audio(z3, compressed, 131072, pcm, 262144, 2);
	const uint32_t low_session = get32(reply + 16);
	put32(payload, low_session);
	put32(payload + 4, audio_staging);
	put32(payload + 8, 0);
	put32(payload + 12, static_cast<uint32_t>(low_mp3.size()));
	put32(payload + 16, 0);
	reply = z3.call(0x0504, payload, 20);
	assert(get32(reply + 24) == 22050 && get32(reply + 28) == 1);
	assert(get32(reply + 48) == get32(reply + 56) / (2 * 576));
	put32(payload, low_session);
	put32(payload + 4, 0);
	z3.call(0x0506, payload, 8);
	put32(payload, compressed);
	z3.call(0x0101, payload, 4);
	put32(payload, 128 * 1024);
	put32(payload + 4, 16);
	put32(payload + 8, 2); // Z2 compressed ring is card-only
	reply = z2.call(0x0100, payload, 12);
	const uint32_t z2_compressed = get32(reply + 16);
	put32(payload, 32 * 1024);
	put32(payload + 8, 1); // compact host-visible PCM ring
	reply = z2.call(0x0100, payload, 12);
	const uint32_t z2_pcm = get32(reply + 16);
	put32(payload, 16 * 1024);
	reply = z2.call(0x0100, payload, 12);
	const uint32_t z2_staging = get32(reply + 16);
	std::copy(mono_mp3.begin(), mono_mp3.end(), z2.sdk.buffer_data(z2_staging));
	reply = begin_audio(z2, z2_compressed, 128 * 1024, z2_pcm,
	                    32 * 1024, 2);
	const uint32_t z2_session = get32(reply + 16);
	put32(payload, z2_session);
	put32(payload + 4, z2_staging);
	put32(payload + 8, 0);
	put32(payload + 12, static_cast<uint32_t>(mono_mp3.size()));
	put32(payload + 16, 0);
	reply = z2.call(0x0504, payload, 20);
	assert(get32(reply + 24) == 44100 && get32(reply + 28) == 1);
	assert(get32(reply + 56) > 0);
	put32(payload, z2_session);
	put32(payload + 4, 0);
	z2.call(0x0506, payload, 8);
	SDL_Quit();
#else
	(void)argc;
	(void)argv;
#endif
}
