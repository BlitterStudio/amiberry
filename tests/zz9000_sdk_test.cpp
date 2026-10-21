#include "zz9000_sdk.h"

#include <cassert>
#include <cstdio>
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
// Recomputes the CRC of the Ogg page at offset after a test edits it, so the
// edit reaches the codec instead of failing the page checksum.
void reseal_ogg_page(std::vector<uint8_t> &bytes, size_t offset)
{
	size_t length = 27u + bytes[offset + 26];
	for (uint32_t i = 0; i < bytes[offset + 26]; ++i)
		length += bytes[offset + 27 + i];
	for (size_t i = 22; i < 26; ++i)
		bytes[offset + i] = 0;
	uint32_t crc = 0;
	for (size_t i = 0; i < length; ++i) {
		crc ^= static_cast<uint32_t>(bytes[offset + i]) << 24;
		for (int bit = 0; bit < 8; ++bit)
			crc = (crc & 0x80000000u) ? (crc << 1) ^ 0x04c11db7u : crc << 1;
	}
	for (size_t i = 0; i < 4; ++i)
		bytes[offset + 22 + i] = static_cast<uint8_t>(crc >> (8 * i));
}

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
const uint8_t *begin_audio_ex(Mailbox &mailbox, uint32_t codec, uint32_t input,
	                          uint32_t input_capacity, uint32_t pcm,
	                          uint32_t pcm_capacity, uint32_t format)
{
	uint8_t request[48] = {};
	put32(request, codec);
	put32(request + 4, input);
	put32(request + 8, input_capacity);
	put32(request + 12, pcm);
	put32(request + 16, pcm_capacity);
	put32(request + 28, format);
	return mailbox.call(0x0516, request, 48);
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
		(1u << 4) | (1u << 5) | (1u << 6) | (1u << 10) |
		(1u << 13) | (1u << 14) | (1u << 15);
#ifdef HAVE_MPG123
	expected_caps |= (1u << 7) | (1u << 19) | (1u << 23);
#endif
#ifdef AMIBERRY_WITH_FFMPEG
	expected_caps |= (1u << 7) | (1u << 28) | (1u << 29);
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
	{
		std::vector<uint8_t> io(0x10000);
		std::vector<uint8_t> vram(4 * 1024 * 1024 - 0x10000);
		zz9000_sdk::Engine split_memory(io.data(), vram.data(), 4 * 1024 * 1024);
		auto *ring = io.data() + zz9000_sdk::mailbox_offset;
		assert(get32(ring) == 0x5a5a394b);
		split_memory.set_framebuffer(0x110000, 2, 2, 8, 7);
		auto *request = ring + 128;
		put32(request, 1);
		put16(request + 4, 0x0203); // Fill the visible framebuffer.
		put16(request + 10, 28);
		put32(request + 16, 0x80000000); // Framebuffer handle.
		put32(request + 28, 2); // Width.
		put32(request + 32, 2); // Height.
		put32(request + 36, 0x00ff0000); // Red.
		put32(ring + 24, 1);
		assert(split_memory.write_register(0x108, 1));
		assert(vram[0x110002 - 0x10000] == 0xff);
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
#if defined(HAVE_MPG123) || defined(AMIBERRY_WITH_FFMPEG)
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
#ifdef AMIBERRY_WITH_FFMPEG
	assert(argc == 15);
#else
	assert(argc == 4);
#endif
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
	put32(payload, 512 * 1024);
	put32(payload + 4, 16);
	put32(payload + 8, 0);
	reply = z3.call(0x0100, payload, 12);
	const uint32_t audio_staging = get32(reply + 16);
	std::copy(mp3.begin(), mp3.end(), z3.sdk.buffer_data(audio_staging));
	put32(payload, 131072);
	reply = z3.call(0x0100, payload, 12);
	uint32_t compressed = get32(reply + 16);
	put32(payload, 262144);
	reply = z3.call(0x0100, payload, 12);
	uint32_t pcm = get32(reply + 16);
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
	z3.call(0x0101, payload, 4); // audio rings are never pinned on the card
	put32(payload, pcm);
	z3.call(0x0101, payload, 4);
	put32(payload, session);
	put32(payload + 4, 0);
	reply = z3.call(0x0507, payload, 8);
	assert(get32(reply + 16) == session);
	z3.call(0x0508, payload, 8);
	z3.call(0x0506, payload, 8);
	// The freed rings must be allocated again for the sessions below.
	put32(payload, 131072);
	put32(payload + 4, 16);
	put32(payload + 8, 0);
	reply = z3.call(0x0100, payload, 12);
	compressed = get32(reply + 16);
	put32(payload, 262144);
	reply = z3.call(0x0100, payload, 12);
	pcm = get32(reply + 16);
	reply = begin_audio_ex(z3, 1, compressed, 131072, pcm, 262144, 1);
	const uint32_t begin_ex_mp3_session = get32(reply + 16);
	put32(payload, begin_ex_mp3_session);
	put32(payload + 4, audio_staging);
	put32(payload + 8, 0);
	put32(payload + 12, static_cast<uint32_t>(mp3.size()));
	put32(payload + 16, 0);
	reply = z3.call(0x0504, payload, 20);
	assert(get32(reply + 24) == 48000 && get32(reply + 56) >= little_pcm.size());
	assert(std::equal(little_pcm.begin(), little_pcm.end(), z3.sdk.buffer_data(pcm)));
	put32(payload, begin_ex_mp3_session);
	put32(payload + 4, 0);
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
	// A card-only PCM ring freed under an open session stays usable: the
	// card keeps the ring address it resolved at BEGIN, and playback keeps
	// pumping from it after the free.
	put32(payload, 32 * 1024);
	put32(payload + 4, 16);
	put32(payload + 8, 2); // card-only PCM ring
	reply = z2.call(0x0100, payload, 12);
	const uint32_t z2_card_pcm = get32(reply + 16);
	reply = begin_audio(z2, z2_compressed, 128 * 1024, z2_card_pcm,
	                    32 * 1024, 1);
	const uint32_t z2_card_session = get32(reply + 16);
	assert(z2_card_session);
	put32(payload, z2_card_session);
	put32(payload + 4, z2_staging);
	put32(payload + 8, 0);
	put32(payload + 12, static_cast<uint32_t>(mono_mp3.size()));
	put32(payload + 16, 0);
	reply = z2.call(0x0504, payload, 20);
	assert(get32(reply + 24) == 48000 && get32(reply + 28) == 2);
	put32(payload, z2_card_pcm);
	z2.call(0x0101, payload, 4);
	put32(payload, z2_card_session);
	put32(payload + 4, 0);
	z2.call(0x0507, payload, 8); // bound; the pump reads the freed ring
	z2.call(0x0508, payload, 8);
	z2.call(0x0506, payload, 8);
	{
		// Firmware session semantics: four concurrent sessions, a fifth
		// BEGIN answers NO_MEMORY; undecodable MP3 input never faults a
		// session (minimp3 consumes nothing, the input ring fills and later
		// feeds answer OK with BACKPRESSURE); FREE never pins audio rings;
		// and PLAY binds one session at a time, answering BUSY for another.
		std::vector<uint8_t> garbage(256 * 1024, 0xaa);
		std::copy(garbage.begin(), garbage.end(),
			z3.sdk.buffer_data(audio_staging));
		put32(payload, 131072);
		put32(payload + 4, 16);
		put32(payload + 8, 0);
		reply = z3.call(0x0100, payload, 12);
		const uint32_t garbage_input = get32(reply + 16);
		put32(payload, 262144);
		reply = z3.call(0x0100, payload, 12);
		const uint32_t garbage_pcm = get32(reply + 16);
		reply = begin_audio(z3, garbage_input, 131072, garbage_pcm, 262144, 1);
		const uint32_t garbage_session = get32(reply + 16);
		assert(garbage_session);
		for (uint32_t offset = 0; offset < garbage.size(); offset += 65536) {
			put32(payload, garbage_session);
			put32(payload + 4, audio_staging);
			put32(payload + 8, offset);
			put32(payload + 12, 65536);
			put32(payload + 16, 0);
			reply = z3.call(0x0504, payload, 20);
			assert(get32(reply + 52) == 0); // nothing consumed
			assert(get32(reply + 56) == 0); // nothing decoded
			assert(offset < 131072 ? (get32(reply + 60) & 8) == 0 :
				(get32(reply + 60) & 8) != 0);
		}
		put32(payload, garbage_input);
		z3.call(0x0101, payload, 4);
		put32(payload, garbage_pcm);
		z3.call(0x0101, payload, 4);
		auto alloc_rings = [&](uint32_t input_size) {
			put32(payload, input_size);
			put32(payload + 4, 16);
			put32(payload + 8, 0);
			reply = z3.call(0x0100, payload, 12);
			const uint32_t input = get32(reply + 16);
			put32(payload, 262144);
			reply = z3.call(0x0100, payload, 12);
			return std::make_pair(input, get32(reply + 16));
		};
		const auto [s2_input, s2_pcm] = alloc_rings(65536);
		reply = begin_audio(z3, s2_input, 65536, s2_pcm, 262144, 1);
		const uint32_t s2 = get32(reply + 16);
		assert(s2 && s2 != garbage_session);
		const auto [s3_input, s3_pcm] = alloc_rings(65536);
#ifdef AMIBERRY_WITH_FFMPEG
		const uint32_t s3_codec = 2; // FLAC through BeginEx
#else
		const uint32_t s3_codec = 1;
#endif
		reply = begin_audio_ex(z3, s3_codec, s3_input, 65536, s3_pcm, 262144, 1);
		const uint32_t s3 = get32(reply + 16);
		assert(s3 && s3 != s2 && s3 != garbage_session);
		const auto [s4_input, s4_pcm] = alloc_rings(65536);
		reply = begin_audio(z3, s4_input, 65536, s4_pcm, 262144, 1);
		const uint32_t s4 = get32(reply + 16);
		assert(s4);
		const auto [s5_input, s5_pcm] = alloc_rings(65536);
		uint8_t fifth[48] = {};
		put32(fifth, s5_input);
		put32(fifth + 4, 65536);
		put32(fifth + 8, s5_pcm);
		put32(fifth + 12, 262144);
		put32(fifth + 24, 1);
		z3.call(0x0503, fifth, 40, 6); // all four slots taken: NO_MEMORY
		// Two sessions decode concurrently and independently: an MP3 fed
		// in two chunks beside a full feed of another stream.
		std::copy(mp3.begin(), mp3.end(), z3.sdk.buffer_data(audio_staging));
		put32(payload, s2);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, 7);
		put32(payload + 16, 0);
		reply = z3.call(0x0504, payload, 20);
		assert(get32(reply + 56) == 0);
#ifdef AMIBERRY_WITH_FFMPEG
		std::ifstream s3_file(argv[4], std::ios::binary);
		const std::vector<uint8_t> s3_bytes(std::istreambuf_iterator<char>{s3_file}, {});
		std::copy(s3_bytes.begin(), s3_bytes.end(),
			z3.sdk.buffer_data(audio_staging));
		put32(payload, s3);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(s3_bytes.size()));
		put32(payload + 16, 1);
		reply = z3.call(0x0504, payload, 20);
		assert(get32(reply + 24) == 44100 && get32(reply + 28) == 1);
		assert(get32(reply + 56) > 0);
#else
		std::copy(mono_mp3.begin(), mono_mp3.end(),
			z3.sdk.buffer_data(audio_staging));
		put32(payload, s3);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(mono_mp3.size()));
		put32(payload + 16, 1);
		reply = z3.call(0x0504, payload, 20);
		assert(get32(reply + 24) == 48000 && get32(reply + 28) == 2);
		assert(get32(reply + 56) > 0);
#endif
		std::copy(mp3.begin(), mp3.end(), z3.sdk.buffer_data(audio_staging));
		put32(payload, s2);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 7);
		put32(payload + 12, static_cast<uint32_t>(mp3.size() - 7));
		put32(payload + 16, 1);
		reply = z3.call(0x0504, payload, 20);
		assert(get32(reply + 24) == 48000 && get32(reply + 28) == 2);
		assert(get32(reply + 52) == mp3.size());
		assert(get32(reply + 56) >= little_pcm.size());
		assert(std::equal(little_pcm.begin(), little_pcm.end(),
			z3.sdk.buffer_data(s2_pcm)));
		// Playback binds one session; PLAY for another answers BUSY.
		put32(payload, s2);
		put32(payload + 4, 0);
		z3.call(0x0507, payload, 8);
		put32(payload, s3);
		z3.call(0x0507, payload, 8, 2);
		put32(payload, s2);
		z3.call(0x0507, payload, 8); // idempotent for the bound session
		z3.call(0x0508, payload, 8);
		put32(payload, s3);
		z3.call(0x0507, payload, 8);
		z3.call(0x0508, payload, 8);
		// Closing a session frees its slot even while others stay open.
		put32(payload, garbage_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
		reply = begin_audio(z3, s5_input, 65536, s5_pcm, 262144, 1);
		assert(get32(reply + 16));
		for (const uint32_t session : {s2, s3, s4, get32(reply + 16)}) {
			put32(payload, session);
			put32(payload + 4, 0);
			z3.call(0x0506, payload, 8);
		}
		for (const uint32_t handle : {s2_input, s2_pcm, s3_input, s3_pcm,
			s4_input, s4_pcm, s5_input, s5_pcm}) {
			put32(payload, handle);
			z3.call(0x0101, payload, 4);
		}
	}
#ifdef AMIBERRY_WITH_FFMPEG
	const uint32_t ex_compressed = audio_staging;

	auto decode_ex = [&](const char *path, const char *reference_path, uint32_t codec,
	                     uint32_t format, uint32_t rate, uint32_t channels,
	                     bool chunked = false) {
		std::ifstream input_file(path, std::ios::binary);
		std::vector<uint8_t> input_bytes(std::istreambuf_iterator<char>{input_file}, {});
		std::ifstream reference_file(reference_path, std::ios::binary);
		std::vector<uint8_t> reference(std::istreambuf_iterator<char>{reference_file}, {});
		assert(!input_bytes.empty() && input_bytes.size() <= 512 * 1024);
		std::copy(input_bytes.begin(), input_bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, codec, ex_compressed, 65536, pcm, 262144, format);
		const uint32_t ex_session = get32(reply + 16);
		assert(ex_session);
		if (chunked) {
			for (uint32_t offset = 0; offset < input_bytes.size();) {
				const uint32_t length = std::min<uint32_t>(997, input_bytes.size() - offset);
				put32(payload, ex_session);
				put32(payload + 4, audio_staging);
				put32(payload + 8, offset);
				put32(payload + 12, length);
				put32(payload + 16, offset + length == input_bytes.size() ? 1 : 0);
				reply = z3.call(0x0504, payload, 20);
				offset += length;
			}
		} else {
			put32(payload, ex_session);
			put32(payload + 4, audio_staging);
			put32(payload + 8, 0);
			put32(payload + 12, static_cast<uint32_t>(input_bytes.size()));
			put32(payload + 16, 1);
			reply = z3.call(0x0504, payload, 20);
		}
		assert(get32(reply + 24) == rate && get32(reply + 28) == channels);
		const uint32_t produced = get32(reply + 56);
		assert(produced && produced % (channels * (format == 4 ? 4 : 2)) == 0);
		const auto *decoded = z3.sdk.buffer_data(pcm);
		assert(std::any_of(decoded, decoded + std::min<uint32_t>(produced, 128),
		                   [](uint8_t value) { return value != 0; }));
		std::vector<uint8_t> pcm_bytes(decoded, decoded + produced);
		if (codec == 2)
			assert(pcm_bytes.size() == reference.size());
		else
			assert(pcm_bytes.size() >= reference.size());
		if (codec == 2) {
			assert(pcm_bytes == reference);
		} else {
			size_t alignment = pcm_bytes.size();
			for (size_t candidate = 0; candidate + 128 <= pcm_bytes.size() &&
			     candidate < 4096; candidate += 2) {
				bool matches = true;
				for (size_t offset = 0; offset < 128; offset += 2) {
					const int sample = static_cast<int16_t>(pcm_bytes[candidate + offset] |
						(static_cast<uint16_t>(pcm_bytes[candidate + offset + 1]) << 8));
					const int expected = static_cast<int16_t>(reference[offset] |
						(static_cast<uint16_t>(reference[offset + 1]) << 8));
					if (std::abs(sample - expected) > 4) {
						matches = false;
						break;
					}
				}
				if (matches) {
					alignment = candidate;
					break;
				}
			}
			assert(alignment != pcm_bytes.size());
			assert(alignment + reference.size() <= pcm_bytes.size());
			for (size_t offset = 0; offset < reference.size(); offset += 2) {
				const int sample = static_cast<int16_t>(pcm_bytes[alignment + offset] |
					(static_cast<uint16_t>(pcm_bytes[alignment + offset + 1]) << 8));
				const int expected = static_cast<int16_t>(reference[offset] |
					(static_cast<uint16_t>(reference[offset + 1]) << 8));
				assert(std::abs(sample - expected) <= 4);
			}
		}
		put32(payload, ex_session);
		put32(payload + 4, 64);
		put32(payload + 8, 0);
		reply = z3.call(0x0515, payload, 48);
		assert(get32(reply + 16) == ex_session);
		put32(payload + 4, produced);
		put32(payload + 8, 0);
		reply = z3.call(0x0505, payload, 12);
		assert(get32(reply + 60) & 4);
		put32(payload, ex_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
		return pcm_bytes;
	};
	const auto flac_once = decode_ex(argv[4], argv[7], 2, 2, 44100, 1);
	const auto flac_chunked = decode_ex(argv[4], argv[7], 2, 2, 44100, 1, true);
	assert(flac_once == flac_chunked);
	decode_ex(argv[5], argv[8], 2, 4, 48000, 2);
	// 997-byte feeds end inside Ogg pages; each must still be accepted.
	const auto vorbis_once = decode_ex(argv[6], argv[9], 3, 1, 32000, 2);
	const auto vorbis_chunked = decode_ex(argv[6], argv[9], 3, 1, 32000, 2, true);
	assert(vorbis_once == vorbis_chunked);
	const auto flac_picture = decode_ex(argv[10], argv[7], 2, 2, 44100, 1, true);
	assert(flac_picture == flac_once);
	auto reject_flac = [&](auto mutate) {
		std::ifstream flac_file(argv[4], std::ios::binary);
		std::vector<uint8_t> bytes(std::istreambuf_iterator<char>{flac_file}, {});
		assert(bytes.size() > 21 && !std::memcmp(bytes.data(), "fLaC", 4));
		mutate(bytes);
		std::copy(bytes.begin(), bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, 2, ex_compressed, 65536, pcm, 262144, 2);
		const uint32_t rejected_session = get32(reply + 16);
		put32(payload, rejected_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(bytes.size()));
		put32(payload + 16, 0);
		z3.call(0x0504, payload, 20, 3);
		put32(payload, rejected_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
	};
	reject_flac([](std::vector<uint8_t> &bytes) {
		const uint32_t rate = 7000;
		bytes[18] = static_cast<uint8_t>(rate >> 12);
		bytes[19] = static_cast<uint8_t>(rate >> 4);
		bytes[20] = static_cast<uint8_t>((bytes[20] & 15) | (rate << 4));
	});
	reject_flac([](std::vector<uint8_t> &bytes) {
		bytes[20] = static_cast<uint8_t>((bytes[20] & 0xf1) | (2 << 1));
	});
	reject_flac([](std::vector<uint8_t> &bytes) {
		bytes[20] |= 1;
		bytes[21] = static_cast<uint8_t>((bytes[21] & 15) | 0xf0);
	});
	{
		std::ifstream vorbis_file(argv[6], std::ios::binary);
		std::vector<uint8_t> bytes(std::istreambuf_iterator<char>{vorbis_file}, {});
		auto header = std::search(bytes.begin(), bytes.end(), "\x01vorbis",
			"\x01vorbis" + 7);
		assert(header != bytes.end() && bytes.end() - header >= 16);
		const uint32_t rate = 192000;
		header[12] = static_cast<uint8_t>(rate);
		header[13] = static_cast<uint8_t>(rate >> 8);
		header[14] = static_cast<uint8_t>(rate >> 16);
		header[15] = static_cast<uint8_t>(rate >> 24);
		reseal_ogg_page(bytes, 0);
		std::copy(bytes.begin(), bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, 3, ex_compressed, 65536, pcm, 262144, 1);
		const uint32_t rejected_session = get32(reply + 16);
		put32(payload, rejected_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(bytes.size()));
		put32(payload + 16, 0);
		z3.call(0x0504, payload, 20, 3);
		put32(payload, rejected_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
	}
	{
		reply = begin_audio_ex(z3, 3, ex_compressed, 65536, pcm, 262144, 1);
		const uint32_t garbage_session = get32(reply + 16);
		assert(garbage_session);
		std::vector<uint8_t> garbage(256, 0xaa);
		std::copy(garbage.begin(), garbage.end(), z3.sdk.buffer_data(audio_staging));
		put32(payload, garbage_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(garbage.size()));
		put32(payload + 16, 0);
		z3.call(0x0504, payload, 20, 9);
		for (int i = 0; i < 5; ++i)
			z3.call(0x0504, payload, 20, 9);
		put32(payload, garbage_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
	}
	{
		// A Vorbis file cut at a page boundary before its EOS page is a
		// truncated track: the EOF-marked feed fails like on the card.
		std::ifstream vorbis_file(argv[11], std::ios::binary);
		std::vector<uint8_t> bytes(std::istreambuf_iterator<char>{vorbis_file}, {});
		size_t last_page = 0;
		for (size_t offset = 0; offset + 27 <= bytes.size();) {
			assert(!std::memcmp(bytes.data() + offset, "OggS", 4));
			size_t page = 27u + bytes[offset + 26];
			for (uint32_t i = 0; i < bytes[offset + 26]; ++i)
				page += bytes[offset + 27 + i];
			last_page = offset;
			offset += page;
		}
		assert(last_page && (bytes[last_page + 5] & 4));
		bytes.resize(last_page);
		std::copy(bytes.begin(), bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, 3, ex_compressed, 65536, pcm, 262144, 1);
		const uint32_t cut_session = get32(reply + 16);
		assert(cut_session);
		put32(payload, cut_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(bytes.size()));
		put32(payload + 16, 1);
		z3.call(0x0504, payload, 20, 9);
		put32(payload, cut_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
	}
	{
		// A lost audio page (the EOS page still present) and a corrupted
		// page both make the EOF-marked feed fail, as the card's page layer
		// does; the intact file decodes.
		std::ifstream vorbis_file(argv[11], std::ios::binary);
		const std::vector<uint8_t> original(std::istreambuf_iterator<char>{vorbis_file}, {});
		std::vector<std::pair<size_t, size_t>> pages;
		for (size_t offset = 0; offset + 27 <= original.size();) {
			size_t page = 27u + original[offset + 26];
			for (uint32_t i = 0; i < original[offset + 26]; ++i)
				page += original[offset + 27 + i];
			pages.emplace_back(offset, page);
			offset += page;
		}
		// Pages 0 and 1 carry the headers; the one before the EOS page is audio.
		assert(pages.size() >= 4);
		const auto audio_page = pages[pages.size() - 2];
		auto feed_whole = [&](const std::vector<uint8_t> &bytes, uint16_t status) {
			std::copy(bytes.begin(), bytes.end(), z3.sdk.buffer_data(audio_staging));
			reply = begin_audio_ex(z3, 3, ex_compressed, 512 * 1024, pcm, 262144, 1);
			const uint32_t session = get32(reply + 16);
			assert(session);
			put32(payload, session);
			put32(payload + 4, audio_staging);
			put32(payload + 8, 0);
			put32(payload + 12, static_cast<uint32_t>(bytes.size()));
			put32(payload + 16, 1);
			z3.call(0x0504, payload, 20, status);
			put32(payload, session);
			put32(payload + 4, 0);
			z3.call(0x0506, payload, 8);
		};
		std::vector<uint8_t> lost(original);
		lost.erase(lost.begin() + static_cast<std::ptrdiff_t>(audio_page.first),
			lost.begin() + static_cast<std::ptrdiff_t>(audio_page.first + audio_page.second));
		feed_whole(lost, 9);
		std::vector<uint8_t> corrupt(original);
		corrupt[audio_page.first + audio_page.second - 1] ^= 0x55;
		feed_whole(corrupt, 9);
	}
	{
		// A FLAC stream that ends on a frame boundary but short of the
		// STREAMINFO sample count is truncated: declare 4096 more samples.
		std::ifstream flac_file(argv[4], std::ios::binary);
		std::vector<uint8_t> bytes(std::istreambuf_iterator<char>{flac_file}, {});
		assert(bytes.size() > 26 && !std::memcmp(bytes.data(), "fLaC", 4) && !(bytes[4] & 0x7f));
		uint8_t *total_field = bytes.data() + 8 + 13;
		uint64_t total = (static_cast<uint64_t>(total_field[0] & 0x0f) << 32) |
			(static_cast<uint64_t>(total_field[1]) << 24) | (total_field[2] << 16) |
			(total_field[3] << 8) | total_field[4];
		assert(total);
		total += 4096;
		total_field[0] = static_cast<uint8_t>((total_field[0] & 0xf0) | ((total >> 32) & 0x0f));
		total_field[1] = static_cast<uint8_t>(total >> 24);
		total_field[2] = static_cast<uint8_t>(total >> 16);
		total_field[3] = static_cast<uint8_t>(total >> 8);
		total_field[4] = static_cast<uint8_t>(total);
		std::copy(bytes.begin(), bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, 2, ex_compressed, 65536, pcm, 262144, 1);
		const uint32_t short_session = get32(reply + 16);
		assert(short_session);
		put32(payload, short_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(bytes.size()));
		put32(payload + 16, 1);
		z3.call(0x0504, payload, 20, 9);
		put32(payload, short_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
	}
	{
		std::ifstream flac16_file(argv[4], std::ios::binary);
		std::vector<uint8_t> flac16_bytes(std::istreambuf_iterator<char>{flac16_file}, {});
		std::copy(flac16_bytes.begin(), flac16_bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, 2, ex_compressed, 65536, pcm, 262144, 1);
		const uint32_t play_session = get32(reply + 16);
		put32(payload, play_session);
		put32(payload + 4, 0);
		z3.call(0x0507, payload, 8, 4);
		put32(payload, play_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(flac16_bytes.size()));
		put32(payload + 16, 0);
		reply = z3.call(0x0504, payload, 20);
		assert(get32(reply + 24) == 44100 && get32(reply + 28) == 1);
		put32(payload, play_session);
		put32(payload + 4, 0);
		z3.call(0x0507, payload, 8, 0);
		z3.call(0x0508, payload, 8, 0);
		z3.call(0x0506, payload, 8, 0);
	}
	// A whole stream fed in one EOF-marked feed must drain completely through
	// a PCM ring much smaller than its decoded size.
	auto drain_small_ring = [&](uint32_t codec, const char *path, uint32_t ring_size = 16384) {
		std::ifstream long_file(path, std::ios::binary);
		std::vector<uint8_t> long_bytes(std::istreambuf_iterator<char>{long_file}, {});
		assert(!long_bytes.empty() && long_bytes.size() <= 512 * 1024);
		put32(payload, ring_size);
		put32(payload + 4, 16);
		put32(payload + 8, 0);
		reply = z3.call(0x0100, payload, 12);
		const uint32_t small_pcm = get32(reply + 16);
		std::copy(long_bytes.begin(), long_bytes.end(), z3.sdk.buffer_data(audio_staging));
		reply = begin_audio_ex(z3, codec, ex_compressed, 512 * 1024, small_pcm, ring_size, 1);
		const uint32_t long_session = get32(reply + 16);
		assert(long_session);
		put32(payload, long_session);
		put32(payload + 4, audio_staging);
		put32(payload + 8, 0);
		put32(payload + 12, static_cast<uint32_t>(long_bytes.size()));
		put32(payload + 16, 1);
		reply = z3.call(0x0504, payload, 20);
		std::vector<uint8_t> drained_pcm;
		for (;;) {
			const uint32_t pcm_write = get32(reply + 40);
			const uint32_t pcm_read = get32(reply + 44);
			const uint32_t flags = get32(reply + 60);
			uint32_t available = 0;
			if (pcm_write == pcm_read)
				available = (flags & 2) ? ring_size : 0;
			else if (pcm_write > pcm_read)
				available = pcm_write - pcm_read;
			else
				available = ring_size - pcm_read + pcm_write;
			if (available > 0) {
				const auto *ring = z3.sdk.buffer_data(small_pcm);
				for (uint32_t i = 0; i < available; ++i)
					drained_pcm.push_back(ring[(pcm_read + i) % ring_size]);
				put32(payload, long_session);
				put32(payload + 4, available);
				put32(payload + 8, 0);
				reply = z3.call(0x0505, payload, 12);
			} else if (flags & 4) {
				break;
			} else {
				put32(payload, long_session);
				put32(payload + 4, 0);
				put32(payload + 8, 0);
				reply = z3.call(0x0505, payload, 12);
			}
		}
		put32(payload, long_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
		put32(payload, small_pcm);
		z3.call(0x0101, payload, 4);
		return drained_pcm;
	};
	{
		const auto drained_pcm = drain_small_ring(3, argv[11]);
		// The EOS granule trims the stream to an exact length; a ring that
		// is not a whole number of stereo frames must not change it.
		std::ifstream vorbis_long_file(argv[11], std::ios::binary);
		const std::vector<uint8_t> vorbis_long_bytes(std::istreambuf_iterator<char>{vorbis_long_file}, {});
		size_t last_page = 0;
		for (size_t offset = 0; offset + 27 <= vorbis_long_bytes.size();) {
			size_t page = 27u + vorbis_long_bytes[offset + 26];
			for (uint32_t i = 0; i < vorbis_long_bytes[offset + 26]; ++i)
				page += vorbis_long_bytes[offset + 27 + i];
			last_page = offset;
			offset += page;
		}
		uint64_t granule = 0;
		for (int i = 7; i >= 0; --i)
			granule = (granule << 8) | vorbis_long_bytes[last_page + 6 + i];
		assert(drained_pcm.size() == granule * 4);
		assert(drain_small_ring(3, argv[11], 16386) == drained_pcm);
		std::ifstream vorbis_long_ref_file(argv[12], std::ios::binary);
		std::vector<uint8_t> vorbis_long_ref(std::istreambuf_iterator<char>{vorbis_long_ref_file}, {});
		assert(drained_pcm.size() >= vorbis_long_ref.size());
		size_t alignment = drained_pcm.size();
		for (size_t candidate = 0; candidate + 128 <= drained_pcm.size() && candidate < 4096; candidate += 2) {
			bool matches = true;
			for (size_t offset = 0; offset < 128; offset += 2) {
				const int sample = static_cast<int16_t>(drained_pcm[candidate + offset] |
					(static_cast<uint16_t>(drained_pcm[candidate + offset + 1]) << 8));
				const int expected = static_cast<int16_t>(vorbis_long_ref[offset] |
					(static_cast<uint16_t>(vorbis_long_ref[offset + 1]) << 8));
				if (std::abs(sample - expected) > 4) { matches = false; break; }
			}
			if (matches) { alignment = candidate; break; }
		}
		assert(alignment != drained_pcm.size());
		assert(alignment + vorbis_long_ref.size() <= drained_pcm.size());
		for (size_t offset = 0; offset < vorbis_long_ref.size(); offset += 2) {
			const int sample = static_cast<int16_t>(drained_pcm[alignment + offset] |
				(static_cast<uint16_t>(drained_pcm[alignment + offset + 1]) << 8));
			const int expected = static_cast<int16_t>(vorbis_long_ref[offset] |
				(static_cast<uint16_t>(vorbis_long_ref[offset + 1]) << 8));
			assert(std::abs(sample - expected) <= 4);
		}
	}
	{
		// Over 128 KiB of FLAC in one feed: parsed incrementally, not refused.
		std::ifstream flac_long_file(argv[13], std::ios::binary);
		std::vector<uint8_t> flac_long_bytes(std::istreambuf_iterator<char>{flac_long_file}, {});
		assert(flac_long_bytes.size() > 128 * 1024);
		const auto drained_pcm = drain_small_ring(2, argv[13]);
		std::ifstream flac_long_ref_file(argv[14], std::ios::binary);
		std::vector<uint8_t> flac_long_ref(std::istreambuf_iterator<char>{flac_long_ref_file}, {});
		assert(flac_long_ref.size() > 4 * 16384 && drained_pcm == flac_long_ref);
	}
	{
		// While the PCM ring is full the card keeps FLAC input in its input
		// ring, refuses only a feed that does not fit, and reports as consumed
		// only the input it has decoded. ZZPlay's on-card playback waits on
		// that count before it feeds again.
		std::ifstream flac_long_file(argv[13], std::ios::binary);
		std::vector<uint8_t> flac_long_bytes(std::istreambuf_iterator<char>{flac_long_file}, {});
		std::copy(flac_long_bytes.begin(), flac_long_bytes.end(), z3.sdk.buffer_data(audio_staging));
		put32(payload, 16384);
		put32(payload + 4, 16);
		put32(payload + 8, 0);
		reply = z3.call(0x0100, payload, 12);
		const uint32_t small_pcm = get32(reply + 16);
		reply = begin_audio_ex(z3, 2, ex_compressed, 65536, small_pcm, 16384, 1);
		const uint32_t ring_session = get32(reply + 16);
		uint32_t fed = 0;
		bool refused = false;
		for (uint32_t offset = 0; offset + 4096 <= flac_long_bytes.size() && !refused;
		     offset += 4096) {
			put32(payload, ring_session);
			put32(payload + 4, audio_staging);
			put32(payload + 8, offset);
			put32(payload + 12, 4096);
			put32(payload + 16, 0);
			reply = z3.call(0x0504, payload, 20);
			if (get32(reply + 60) & 8)
				refused = true;
			else
				fed += 4096;
		}
		assert(refused && fed > 65536 - 4096);
		const uint32_t consumed = get32(reply + 52);
		assert(consumed < fed && fed - consumed <= 65536);
		const uint32_t pcm_write = get32(reply + 40);
		const uint32_t pcm_read = get32(reply + 44);
		const uint32_t used = pcm_write == pcm_read ? 16384 : (pcm_write + 16384 - pcm_read) % 16384;
		put32(payload, ring_session);
		put32(payload + 4, used);
		put32(payload + 8, 0);
		reply = z3.call(0x0505, payload, 12);
		assert(get32(reply + 52) > consumed);
		put32(payload, ring_session);
		put32(payload + 4, 0);
		z3.call(0x0506, payload, 8);
		put32(payload, small_pcm);
		z3.call(0x0101, payload, 4);
	}
	uint8_t invalid_begin[48] = {};
	put32(invalid_begin, 2);
	put32(invalid_begin + 4, ex_compressed);
	put32(invalid_begin + 8, 65536);
	put32(invalid_begin + 12, pcm);
	put32(invalid_begin + 16, 262144);
	put32(invalid_begin + 28, 3);
	z3.call(0x0516, invalid_begin, 48, 3);
#endif
	put32(payload, z2_session);
	put32(payload + 4, 0);
	z2.call(0x0506, payload, 8);
	SDL_Quit();
#else
	(void)argc;
	(void)argv;
#endif
}
