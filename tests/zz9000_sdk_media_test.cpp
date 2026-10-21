#include "zz9000_sdk.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <thread>
#include <vector>

namespace {
uint16_t get16(const uint8_t *p) { return (p[0] << 8) | p[1]; }
uint32_t get32(const uint8_t *p) { return (get16(p) << 16) | get16(p + 2); }
uint64_t get64(const uint8_t *p)
{
	return (static_cast<uint64_t>(get32(p)) << 32) | get32(p + 4);
}
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

// 16-bit halves of a packed presentation pair, as ZZ9K_MEDIA_PAIR_* unpack
// them on the guest.
uint16_t pair_hi(uint64_t value) { return static_cast<uint16_t>(value >> 16); }
uint16_t pair_lo(uint64_t value) { return static_cast<uint16_t>(value); }
int16_t pair_hi_s(uint64_t value) { return static_cast<int16_t>(pair_hi(value)); }
int16_t pair_lo_s(uint64_t value) { return static_cast<int16_t>(pair_lo(value)); }
uint32_t profile_calls(uint64_t value)
{
	return static_cast<uint32_t>(value & 0xffffffffu);
}

// Mirrors zzplay_present_classify() from the guest's zzplay-path.c, so the
// values the emulator reports are validated against the player's own path
// naming instead of re-derived facts.
enum PresentPath {
	PathUnknown, PathInactive, PathNative1To1, PathNativeScaled, PathSoftware
};
PresentPath classify_present(int status, uint32_t flags, const uint64_t *value)
{
	if (status != 0 || !value)
		return PathUnknown;
	const uint16_t src_w = pair_hi(value[0]), src_h = pair_lo(value[0]);
	const uint16_t dst_w = pair_hi(value[1]), dst_h = pair_lo(value[1]);
	const int16_t dst_x = pair_hi_s(value[2]), dst_y = pair_lo_s(value[2]);
	const uint16_t screen_w = pair_hi(value[3]), screen_h = pair_lo(value[3]);
	bool clipped = false;
	if (screen_w && screen_h)
		clipped = dst_x < 0 || dst_y < 0 ||
			static_cast<int32_t>(dst_x) + dst_w > screen_w ||
			static_cast<int32_t>(dst_y) + dst_h > screen_h;
	if ((flags & 1u) == 0 || (flags & 2u) == 0)
		return PathInactive;
	if ((flags & 4u) == 0)
		return PathSoftware;
	if (dst_w == src_w && dst_h == src_h && !clipped)
		return PathNative1To1;
	return PathNativeScaled;
}
#ifdef AMIBERRY_WITH_FFMPEG
void run_ffmpeg_media(const char *path, uint32_t video_codec, uint32_t container,
	uint32_t audio_codec, bool truncated, bool expect_video, uint32_t width, uint32_t height)
{
	std::ifstream file(path, std::ios::binary);
	const std::vector<uint8_t> input{std::istreambuf_iterator<char>{file}, {}};
	assert(!input.empty() && input.size() < 8 * 1024 * 1024);
	Mailbox mailbox;
	const uint32_t staging = mailbox.allocate(65536);
	const uint32_t pcm = audio_codec ? mailbox.allocate(131072) : 0;
	const uint32_t pip_width = width > 720 ? ((width / 2) + 1) & ~1u : width;
	const uint32_t pip_height = height > 720 ? height / 2 : height;
	// Destination rect equals the source at (0,0): the classification a
	// real card reports for a 1:1 PIP window.
	zz9000_sdk::Engine::OverlayGeometry overlay_geometry;
	overlay_geometry.dst_w = pip_width;
	overlay_geometry.dst_h = pip_height;
	overlay_geometry.screen_w = pip_width;
	overlay_geometry.screen_h = pip_height;
	mailbox.sdk.set_overlay(0x03310000, pip_width, pip_height, pip_width * 2, 0,
		true, true, overlay_geometry);
	// Presentation must stay inside the video window.
	auto *guard = mailbox.board.data() + 0x03310000 + static_cast<size_t>(pip_width) * pip_height * 2;
	std::fill(guard, guard + 16, 0xa5);
	uint8_t request[48] = {};
	put32(request, video_codec); put32(request + 4, container);
	put32(request + 8, width); put32(request + 12, height); put32(request + 16, 1);
	put32(request + 20, audio_codec);
	if (audio_codec) {
		put32(request + 24, pcm); put32(request + 28, 131072);
		put32(request + 32, 24576); put32(request + 36, 98304);
	}
	const auto *reply = mailbox.call(0x0b04, request, 44);
	assert(mailbox.last_status == 0);
	const uint32_t session = get32(reply);
	assert(session && get32(reply + 4) == 1);
	uint32_t profile_frames[4] = {};
	bool before_eof = false;
	for (size_t offset = 0; offset < input.size();) {
		const uint32_t bytes = static_cast<uint32_t>(std::min<size_t>(4096, input.size() - offset));
		std::copy(input.begin() + offset, input.begin() + offset + bytes,
			mailbox.sdk.buffer_data(staging));
		if (expect_video && !truncated && offset + bytes == input.size()) {
			for (unsigned attempt = 0; attempt < 20 && !before_eof; ++attempt) {
				std::fill(std::begin(request), std::end(request), 0);
				put32(request, session);
				reply = mailbox.call(0x0b06, request, 16);
				assert(mailbox.last_status == 0);
			if (get32(reply + 4) == 3) {
				mailbox.call(0x0b08, request, 16);
				before_eof = true;
				// First profiling snapshot: one frame has been decoded,
				// packed and presented already.
				std::fill(std::begin(request), std::end(request), 0);
				put32(request, session); put32(request + 4, 5);
				const auto *profile = mailbox.call(0x0b0a, request, 16);
				profile_frames[0] = profile_calls(get64(profile + 16));
				profile_frames[1] = profile_calls(get64(profile + 24));
				profile_frames[2] = profile_calls(get64(profile + 32));
			}
		}
	}
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, staging); put32(request + 12, bytes);
	put32(request + 16, offset + bytes == input.size());
	reply = mailbox.call(0x0b05, request, 20);
	assert(mailbox.last_status == 0);
	assert(get32(reply + 40) == bytes && get32(reply + 36) == offset + bytes);
	offset += bytes;
}
	uint64_t acknowledged = 0;
	uint64_t last_pts = 0;
	uint64_t last_audio_pts = 0;
	uint32_t frames = 0;
	uint32_t discards = 0;
	bool done = false;
	for (unsigned i = 0; i < 1000 && !done; ++i) {
		if (audio_codec) {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session); put32(request + 4, static_cast<uint32_t>(acknowledged >> 32));
			put32(request + 8, static_cast<uint32_t>(acknowledged));
			reply = mailbox.call(0x0b07, request, 16);
			assert(mailbox.last_status == 0);
			acknowledged = (static_cast<uint64_t>(get32(reply + 20)) << 32) | get32(reply + 24);
			// The audio clock never runs backward, even across a change of
			// sample rate or channel count.
			const uint64_t audio_pts = (static_cast<uint64_t>(get32(reply + 36)) << 32) |
				get32(reply + 40);
			if (audio_pts != UINT64_MAX) {
				assert(audio_pts >= last_audio_pts);
				last_audio_pts = audio_pts;
			}
		}
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		reply = mailbox.call(0x0b06, request, 16, 0xffff);
		if (truncated) {
			if (mailbox.last_status == 9) break;
			continue;
		}
		// BUSY: decoded PCM is waiting for ring space; read audio and retry.
		if (mailbox.last_status == 2)
			continue;
		assert(mailbox.last_status == 0);
		// All input and EOF have been sent: NEED_INPUT now reads as a
		// truncated stream to ZZPlay, even while FFmpeg is still decoding.
		assert(get32(reply + 4) != 1);
		if (get32(reply + 4) == 3) {
			const uint64_t pts = (static_cast<uint64_t>(get32(reply + 28)) << 32) | get32(reply + 32);
			assert(frames == 0 || pts >= last_pts);
			last_pts = pts;
			assert(get32(reply + 8) == width && get32(reply + 12) == height);
			// Present most frames but discard a few, so page 2 has separate
			// presented and discarded counts to report.
			if (frames % 50 == 1) {
				mailbox.call(0x0b09, request, 16);
				discards++;
			} else {
				mailbox.call(0x0b08, request, 16);
			}
			frames++;
		} else if (get32(reply + 4) == 4) {
			done = true;
			if (audio_codec) {
				// DONE means every decoded sample is already in the ring:
				// draining it afterwards must not produce anything new.
				std::fill(std::begin(request), std::end(request), 0);
				put32(request, session); put32(request + 4, 1);
				reply = mailbox.call(0x0b0a, request, 16);
				const uint64_t produced_at_done =
					(static_cast<uint64_t>(get32(reply + 16)) << 32) | get32(reply + 20);
				uint64_t produced = 0;
				for (unsigned pass = 0; pass < 8; ++pass) {
					std::fill(std::begin(request), std::end(request), 0);
					put32(request, session); put32(request + 4, static_cast<uint32_t>(acknowledged >> 32));
					put32(request + 8, static_cast<uint32_t>(acknowledged));
					reply = mailbox.call(0x0b07, request, 16);
					produced = (static_cast<uint64_t>(get32(reply + 20)) << 32) | get32(reply + 24);
					acknowledged = produced;
				}
				assert(produced == produced_at_done);
			}
		}
	}
	if (truncated)
		assert(mailbox.last_status == 9);
	else {
		assert(done);
		if (audio_codec) {
			put32(request, session); put32(request + 4, static_cast<uint32_t>(acknowledged >> 32));
			put32(request + 8, static_cast<uint32_t>(acknowledged));
			reply = mailbox.call(0x0b07, request, 16);
			assert(mailbox.last_status == 0);
			acknowledged = (static_cast<uint64_t>(get32(reply + 20)) << 32) | get32(reply + 24);
		}
		if (expect_video) {
			assert(before_eof && frames > 1);
			assert(std::any_of(mailbox.board.data() + 0x03310000,
				mailbox.board.data() + 0x03310000 + static_cast<size_t>(pip_width) * pip_height * 2,
				[](uint8_t value) { return value != 0; }));
		}
		if (expect_video && width > 720 && ((width / 2) & 1)) {
			// An odd half width ends on the last whole YUY2 pair; the spare
			// column of every row stays untouched.
			for (uint32_t y = 0; y < height / 2; ++y) {
				const auto *row = mailbox.board.data() + 0x03310000 + static_cast<size_t>(y) * pip_width * 2;
				assert(std::all_of(row + (width / 2 - 1) * 2, row + pip_width * 2,
					[](uint8_t value) { return value == 0; }));
			}
		}
		assert(std::all_of(guard, guard + 16, [](uint8_t value) { return value == 0xa5; }));
		if (audio_codec)
			assert(acknowledged > 0);
		for (uint32_t page = 0; page <= 6; ++page) {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session); put32(request + 4, page);
			reply = mailbox.call(0x0b0a, request, 16, page <= 5 ? 0 : 4);
			if (page > 5)
				continue; // Past the last page the card answers BAD_REQUEST.
			assert(get32(reply) == session);
			assert(get32(reply + 8) == page);
			if (page == 0 && expect_video) {
				// The timeline origin is the first container PTS; for the
				// offset fixture that is 5 s in, never NO_PTS.
				const uint64_t origin = get64(reply + 16);
				assert(origin != UINT64_MAX && origin <= last_pts);
			} else if (page == 1 && audio_codec) {
				// The decoded-audio clock runs from the first timestamp.
				const uint64_t audio_pts = get64(reply + 32);
				const uint64_t first = get64(reply + 40);
				assert(first != UINT64_MAX && audio_pts >= first);
			} else if (page == 2) {
				const uint64_t decoded = get64(reply + 24);
				const uint64_t presented = get64(reply + 32);
				const uint64_t discarded = get64(reply + 40);
				// Presented and discarded are counted separately from
				// decoded, and every held frame went to exactly one of them.
				if (expect_video) {
					assert(discards > 0);
					assert(discarded == discards);
					assert(presented + discarded == decoded);
				} else {
					assert(presented == 0 && discarded == 0);
				}
			} else if (page == 3) {
				assert(get64(reply + 40) == 0); // no card-side pump: no underruns
				if (audio_codec)
					assert(get64(reply + 32) > 0);
			} else if (page == 4) {
				const uint32_t flags = get32(reply + 12);
				const uint64_t values[4] = {get64(reply + 16), get64(reply + 24),
					get64(reply + 32), get64(reply + 40)};
				if (expect_video) {
					// Active native overlay fed by this session: ZZPlay
					// reports "native 1:1", not UNKNOWN or inactive.
					assert(flags == 0xfu);
					assert(classify_present(0, flags, values) == PathNative1To1);
					assert(pair_hi(values[0]) == pip_width &&
						pair_lo(values[0]) == pip_height);
				} else {
					// An audio-only session never presents, so the path is
					// configured, active and native but not owned.
					assert(flags == 0x7u);
					assert(classify_present(0, flags, values) == PathNative1To1);
				}
			} else if (page == 5) {
				// Scalar host pack: the NEON bit never appears.
				assert(get32(reply + 12) == 0);
				const uint64_t decode = get64(reply + 16);
				const uint64_t pack = get64(reply + 24);
				const uint64_t present = get64(reply + 32);
				if (expect_video) {
					// Calls advanced since the first frame: the worker
					// decoded and packed more frames, and each present
					// copied them into the overlay plane.
					assert(profile_calls(decode) > profile_frames[0]);
					assert(profile_calls(pack) > profile_frames[1]);
					assert(profile_calls(present) > profile_frames[2]);
				}
				if (audio_codec)
					assert(profile_calls(get64(reply + 40)) > 0);
			}
		}
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		mailbox.call(0x0b0b, request, 16, 3);
		reply = mailbox.call(0x0b0c, request, 16, 0);
		assert(get32(reply) == session);
	}
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	mailbox.call(0x0b0d, request, 16);
}

// A Program Stream can carry the audio for the picture on show well after
// the next pictures. While the guest holds video (it does not DECODE until
// the audio clock reaches the frame), the session must still decode that
// audio, or ZZPlay underruns.
void run_held_video_audio(const char *path)
{
	std::ifstream file(path, std::ios::binary);
	const std::vector<uint8_t> input{std::istreambuf_iterator<char>{file}, {}};
	assert(!input.empty() && input.size() < 4 * 1024 * 1024);
	Mailbox mailbox;
	const uint32_t staging = mailbox.allocate(65536);
	const uint32_t pcm = mailbox.allocate(131072);
	zz9000_sdk::Engine::OverlayGeometry overlay_geometry;
	overlay_geometry.dst_w = 160;
	overlay_geometry.dst_h = 120;
	overlay_geometry.screen_w = 160;
	overlay_geometry.screen_h = 120;
	mailbox.sdk.set_overlay(0x03310000, 160, 120, 320, 0, true, true,
		overlay_geometry);
	uint8_t request[48] = {};
	put32(request, 2); put32(request + 4, 1);
	put32(request + 8, 160); put32(request + 12, 120); put32(request + 16, 1);
	put32(request + 20, 3);
	put32(request + 24, pcm); put32(request + 28, 131072);
	put32(request + 32, 24576); put32(request + 36, 98304);
	const uint32_t session = get32(mailbox.call(0x0b04, request, 44));
	const uint64_t wanted = 48000ull * 4 * 2; // two seconds of 48 kHz stereo
	size_t offset = 0;
	uint64_t acknowledged = 0;
	for (unsigned round = 0; round < 400 && acknowledged < wanted; ++round) {
		while (offset < input.size()) {
			const uint32_t bytes = static_cast<uint32_t>(std::min<size_t>(4096, input.size() - offset));
			std::copy(input.begin() + offset, input.begin() + offset + bytes,
				mailbox.sdk.buffer_data(staging));
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session); put32(request + 4, staging); put32(request + 12, bytes);
			put32(request + 16, offset + bytes == input.size());
			const auto *reply = mailbox.call(0x0b05, request, 20, 0xffff);
			const uint32_t accepted = mailbox.last_status == 0 ? get32(reply + 40) : 0;
			offset += accepted;
			if (accepted < bytes)
				break;
		}
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, static_cast<uint32_t>(acknowledged >> 32));
		put32(request + 8, static_cast<uint32_t>(acknowledged));
		const auto *reply = mailbox.call(0x0b07, request, 16);
		acknowledged = (static_cast<uint64_t>(get32(reply + 20)) << 32) | get32(reply + 24);
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	assert(acknowledged >= wanted);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	mailbox.call(0x0b0d, request, 16, 0xffff);
}
#endif
}

int main(int argc, char **argv)
{
#ifdef AMIBERRY_WITH_FFMPEG
	if (argc == 3 && std::strcmp(argv[1], "--held-video") == 0) {
		run_held_video_audio(argv[2]);
		return 0;
	}
	if (argc == 10 && std::strcmp(argv[1], "--av") == 0) {
		run_ffmpeg_media(argv[2], static_cast<uint32_t>(std::strtoul(argv[3], nullptr, 10)),
			static_cast<uint32_t>(std::strtoul(argv[4], nullptr, 10)),
			static_cast<uint32_t>(std::strtoul(argv[5], nullptr, 10)),
			std::strtoul(argv[6], nullptr, 10) != 0,
			std::strtoul(argv[7], nullptr, 10) != 0,
			static_cast<uint32_t>(std::strtoul(argv[8], nullptr, 10)),
			static_cast<uint32_t>(std::strtoul(argv[9], nullptr, 10)));
		return 0;
	}
#endif
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
	// Before any overlay exists the presentation page reports an
	// unconfigured path, which ZZPlay classifies as inactive.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 4);
	reply = mailbox.call(0x0b0a, request, 16);
	assert(get32(reply + 8) == 4 && get32(reply + 12) == 0);
	put32(request, pcm);
	mailbox.call(0x0101, request, 4, 2);
	zz9000_sdk::Engine::OverlayGeometry overlay_geometry;
	overlay_geometry.dst_w = width;
	overlay_geometry.dst_h = height;
	overlay_geometry.screen_w = width;
	overlay_geometry.screen_h = height;
	mailbox.sdk.set_overlay(0x03310000, width, height, width * 2, 0,
		true, true, overlay_geometry);
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
		// The MPEG-1 session derived its timeline from the frame count, so
		// page 0 reports a zero origin, and page 2 splits presented and
		// discarded frames out of the decoded count.
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, 0);
		reply = mailbox.call(0x0b0a, request, 16);
		assert(get64(reply + 16) == 0);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, 2);
		reply = mailbox.call(0x0b0a, request, 16);
		assert(get64(reply + 24) == frames); // decoded
		assert(get64(reply + 32) >= 1 && get64(reply + 40) >= 1); // presented/discarded
		assert(get64(reply + 32) + get64(reply + 40) == frames);
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
	// DECODE flags follow the 64-bit value cursor. Unknown flags and a nonzero
	// cursor are refused, as on the card.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 12, 4);
	mailbox.call(0x0b06, request, 16, 4);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 1);
	mailbox.call(0x0b06, request, 16, 4);
	bool shown = false;
	for (int i = 0; i < 100 && !shown; ++i) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		// A late client asks to skip to the next keyframe and I picture.
		put32(request + 12, 3);
		reply = mailbox.call(0x0b06, request, 16);
		if (get32(reply + 4) == 3) {
			assert(get32(reply + 8) == width && get32(reply + 12) == height);
			put32(request + 12, 0);
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
	// The MPEG-1 timeline is derived from the frame count, so page 0 reports
	// a zero origin like the card's derived clocks.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 0);
	reply = mailbox.call(0x0b0a, request, 16);
	assert(get64(reply + 16) == 0);
	// Page 4 with a live overlay: configured, active, native, and owned by
	// this session after the present above. The geometry is exactly the
	// ZZPlay "native 1:1" case.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 4);
	reply = mailbox.call(0x0b0a, request, 16);
	{
		const uint32_t flags = get32(reply + 12);
		const uint64_t values[4] = {get64(reply + 16), get64(reply + 24),
			get64(reply + 32), get64(reply + 40)};
		assert(flags == 0xfu);
		assert(classify_present(0, flags, values) == PathNative1To1);
		assert(pair_hi(values[2]) == 0 && pair_lo_s(values[2]) == 0);
	}
	// A window offset partly off-screen keeps the native scaler/clipper
	// path: ZZPlay reports "native scaled", with the signed origin packed
	// as its 16-bit two's-complement pattern.
	overlay_geometry.dst_x = -8;
	overlay_geometry.dst_y = 4;
	mailbox.sdk.set_overlay(0x03310000, width, height, width * 2, 0,
		true, true, overlay_geometry);
	reply = mailbox.call(0x0b0a, request, 16);
	{
		const uint32_t flags = get32(reply + 12);
		const uint64_t values[4] = {get64(reply + 16), get64(reply + 24),
			get64(reply + 32), get64(reply + 40)};
		assert(flags == 0xfu); // re-SET keeps the session binding
		assert(classify_present(0, flags, values) == PathNativeScaled);
		assert(pair_hi_s(values[2]) == -8 && pair_lo_s(values[2]) == 4);
	}
	// Disabling the overlay clears the whole path: inactive.
	mailbox.sdk.set_overlay(0x03310000, width, height, width * 2, 0,
		true, false, overlay_geometry);
	reply = mailbox.call(0x0b0a, request, 16);
	{
		const uint32_t flags = get32(reply + 12);
		const uint64_t values[4] = {get64(reply + 16), get64(reply + 24),
			get64(reply + 32), get64(reply + 40)};
		assert(flags == 1u); // configured, not active
		assert(classify_present(0, flags, values) == PathInactive);
	}
	// Page 5 after decode, present and audio decode: the NEON bit never
	// appears for the scalar host pack, and every stage advanced.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 5);
	reply = mailbox.call(0x0b0a, request, 16);
	assert(get32(reply + 12) == 0);
	assert(profile_calls(get64(reply + 16)) >= 1); // video decode
	assert(profile_calls(get64(reply + 24)) >= 1); // YUY2 pack
	assert(profile_calls(get64(reply + 32)) >= 1); // present
	assert(profile_calls(get64(reply + 40)) >= 1); // audio decode
	// Pages past the last one answer BAD_REQUEST, as on the card.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 6);
	mailbox.call(0x0b0a, request, 16, 4);
	std::fill(std::begin(request), std::end(request), 0);
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
