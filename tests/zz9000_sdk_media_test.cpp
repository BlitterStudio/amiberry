#include "zz9000_sdk.h"
#include "zz9000_sdk_test_mailbox.h"
#include "zz9000_sdk_media_output.h"

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
	uint8_t *mb;
	uint32_t entries;
	uint32_t request_id = 0, completion_head = 0;
	uint16_t last_status = 0;
	bool last_dirty = false;

	explicit Mailbox(uint32_t size = 128 * 1024 * 1024)
		: board(size), sdk(board.data(), board.size()),
		  mb(zz9000_test_mailbox(sdk, board.data())), entries(get32(mb + 16)) {}

	const uint8_t *call(uint16_t opcode, const uint8_t *payload,
	                    uint16_t length, uint16_t status = 0) {
		const uint32_t tail = get32(mb + 24);
		auto *request = mb + get32(mb + 12) + tail * 64;
		std::fill(request, request + 64, 0);
		put32(request, ++request_id);
		put16(request + 4, opcode);
		put16(request + 10, length);
		if (payload)
			std::copy(payload, payload + length, request + 16);
		put32(mb + 24, (tail + 1) % entries);
		last_dirty = sdk.poll();
		const auto *reply = mb + get32(mb + 28) + completion_head * 64;
		assert(get32(reply) == request_id);
		last_status = get16(reply + 6);
		assert(status == 0xffff || last_status == status);
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
		for (uint32_t page = 0; page <= 7; ++page) {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session); put32(request + 4, page);
			reply = mailbox.call(0x0b0a, request, 16, page <= 6 ? 0 : 4);
			if (page > 6)
				continue; // Past the last page the card answers BAD_REQUEST.
			if (page == 6) {
				// An overlay session has no shared-buffer output: all zero.
				assert(get32(reply + 12) == 0 && get64(reply + 16) == 0 &&
				       get64(reply + 24) == 0 && get64(reply + 32) == 0 &&
				       get64(reply + 40) == 0);
				continue;
			}
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

using zz9000_sdk::Engine;
namespace media_output = zz9000_sdk::media_output;

std::vector<uint8_t> read_all(const char *path)
{
	std::ifstream file(path, std::ios::binary);
	std::vector<uint8_t> data{std::istreambuf_iterator<char>{file}, {}};
	assert(!data.empty());
	return data;
}

struct Play {
	uint32_t video_codec = 1, container = 1, audio_codec = 0;
	uint32_t width = 160, height = 120;
	size_t max_frames = 12;
	uint32_t output_format = 1; // 2: shared buffer, configured with `config`
	media_output::Config config;
	std::vector<uint8_t> palette; // 1024 bytes for a PALETTE_HOST output
};

struct Played {
	uint16_t begin_status = 0;
	// Overlay: the width * height * 2 YUY2 bytes after each PRESENT. Shared
	// buffer: the slot's picture.
	std::vector<std::vector<uint8_t>> frames;
	std::vector<uint32_t> slots; // the slot each PRESENT reported
	std::vector<uint8_t> pcm;
	uint32_t picture_bytes = 0;
};

// Plays `input` through a media session: feeds it in 4 KiB writes, decodes,
// presents every picture and records what each present wrote, and drains
// the PCM ring.
Played play(const std::vector<uint8_t> &input, const Play &p)
{
	Played result;
	Mailbox mailbox;
	const uint32_t staging = mailbox.allocate(65536);
	const uint32_t pcm_capacity = 131072;
	const uint32_t pcm = p.audio_codec ? mailbox.allocate(pcm_capacity) : 0;
	const bool shared = p.output_format == 2;
	media_output::Layout layout;
	uint32_t out_buffer = 0, palette_buffer = 0;
	if (shared) {
		assert(media_output::check(p.config, layout) == 0);
		out_buffer = mailbox.allocate(p.config.buffer_offset + layout.span_bytes + 64);
		if (p.config.flags & media_output::flag_palette_host) {
			assert(p.palette.size() == media_output::palette_bytes);
			palette_buffer = mailbox.allocate(media_output::palette_bytes);
			std::copy(p.palette.begin(), p.palette.end(), mailbox.sdk.buffer_data(palette_buffer));
		}
		result.picture_bytes = layout.picture_bytes;
	} else {
		Engine::OverlayGeometry geometry;
		geometry.dst_w = p.width; geometry.dst_h = p.height;
		geometry.screen_w = p.width; geometry.screen_h = p.height;
		mailbox.sdk.set_overlay(0x03310000, p.width, p.height, p.width * 2, 0, true, true, geometry);
	}
	uint8_t request[48] = {};
	put32(request, p.video_codec); put32(request + 4, p.container);
	put32(request + 8, p.width); put32(request + 12, p.height);
	put32(request + 16, p.output_format); put32(request + 20, p.audio_codec);
	if (p.audio_codec) {
		put32(request + 24, pcm); put32(request + 28, pcm_capacity);
		put32(request + 32, 24576); put32(request + 36, 98304);
	}
	const auto *reply = mailbox.call(0x0b04, request, 44, 0xffff);
	result.begin_status = mailbox.last_status;
	if (mailbox.last_status != 0)
		return result;
	const uint32_t session = get32(reply);
	if (shared) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, out_buffer);
		put32(request + 8, p.config.buffer_offset); put32(request + 12, p.config.slot_stride);
		put32(request + 16, p.config.width); put32(request + 20, p.config.height);
		put32(request + 24, p.config.format); put32(request + 28, p.config.pitch);
		put32(request + 32, p.config.flags); put32(request + 36, palette_buffer);
		reply = mailbox.call(0x0b0e, request, 48);
		assert(get32(reply) == session && get32(reply + 40) == layout.picture_bytes);
	}
	size_t offset = 0;
	uint64_t acknowledged = 0;
	bool done = false;
	for (unsigned guard = 0; !done && result.frames.size() < p.max_frames && guard < 30000; ++guard) {
		if (offset < input.size()) {
			const uint32_t bytes = static_cast<uint32_t>(std::min<size_t>(4096, input.size() - offset));
			std::copy(input.begin() + offset, input.begin() + offset + bytes,
				mailbox.sdk.buffer_data(staging));
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session); put32(request + 4, staging); put32(request + 12, bytes);
			put32(request + 16, offset + bytes == input.size());
			reply = mailbox.call(0x0b05, request, 20, 0xffff);
			if (mailbox.last_status == 0)
				offset += get32(reply + 40);
			else
				assert(mailbox.last_status == 2);
		}
		if (p.audio_codec) {
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session);
			put32(request + 4, static_cast<uint32_t>(acknowledged >> 32));
			put32(request + 8, static_cast<uint32_t>(acknowledged));
			reply = mailbox.call(0x0b07, request, 16);
			const uint64_t produced = get64(reply + 20);
			const auto *ring = mailbox.sdk.buffer_data(pcm);
			for (uint64_t at = acknowledged; at < produced; ++at)
				result.pcm.push_back(ring[at % pcm_capacity]);
			acknowledged = produced;
		}
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		reply = mailbox.call(0x0b06, request, 16, 0xffff);
		if (mailbox.last_status == 2) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		assert(mailbox.last_status == 0);
		const uint32_t state = get32(reply + 4);
		if (state == 3) {
			reply = mailbox.call(0x0b08, request, 16);
			if (shared) {
				const uint32_t flags = get32(reply + 44);
				assert((flags & (1u << 9)) && (flags & (1u << 16)));
				const uint32_t slot = (flags >> 15) & 1;
				assert(get32(reply + 40) == layout.picture_bytes);
				const auto *data = mailbox.sdk.buffer_data(out_buffer) + p.config.buffer_offset +
					(slot ? p.config.slot_stride : 0);
				result.frames.emplace_back(data, data + layout.picture_bytes);
				result.slots.push_back(slot);
			} else {
				const auto *data = mailbox.board.data() + 0x03310000;
				result.frames.emplace_back(data, data + p.width * p.height * 2);
			}
		} else if (state == 4) {
			done = true;
		} else if (state == 1 && offset == input.size()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session);
	mailbox.call(0x0b0d, request, 16, 0xffff);
	return result;
}

// The Transport Stream front end hands the decoder the same elementary
// streams as the Program Stream they were remuxed from, so the pictures and
// the sound come out the same.
void check_ts_matches_ps(const char *ps_path, const char *ts_path, uint32_t video_codec,
	                     uint32_t audio_codec, uint32_t width, uint32_t height,
	                     bool m2ts = false)
{
	Play ps_play;
	ps_play.video_codec = video_codec; ps_play.audio_codec = audio_codec;
	ps_play.width = width; ps_play.height = height;
	ps_play.max_frames = 1000; // to the end of the stream
	Play ts_play = ps_play;
	ts_play.container = 3;
	auto ts_input = read_all(ts_path);
	if (m2ts) {
		// BDAV packets: a 4-byte timestamp before each 188-byte packet.
		// (FFmpeg's M2TS mode marks MPEG-1 as private data, which is not
		// what is being tested.)
		assert(ts_input.size() % 188 == 0);
		std::vector<uint8_t> packets;
		for (size_t at = 0; at < ts_input.size(); at += 188) {
			const uint32_t stamp = 0x0e0000u + static_cast<uint32_t>(at / 188) * 777;
			packets.insert(packets.end(), {static_cast<uint8_t>(stamp >> 24),
				static_cast<uint8_t>(stamp >> 16), static_cast<uint8_t>(stamp >> 8),
				static_cast<uint8_t>(stamp)});
			packets.insert(packets.end(), ts_input.begin() + at, ts_input.begin() + at + 188);
		}
		ts_input = std::move(packets);
	}
	const auto ts = play(ts_input, ts_play);
#ifndef AMIBERRY_WITH_FFMPEG
	// MPEG-2 is decoded by FFmpeg: without it a Transport Stream session
	// is refused like a Program Stream one.
	if (video_codec == 2) {
		assert(ts.begin_status == 3);
		return;
	}
#endif
	const auto ps = play(read_all(ps_path), ps_play);
	assert(ps.begin_status == 0 && ts.begin_status == 0);
	assert(ps.frames.size() >= 8 && ts.frames.size() >= 8);
	const size_t frames = std::min(ps.frames.size(), ts.frames.size());
	for (size_t i = 0; i < frames; ++i)
		assert(ps.frames[i] == ts.frames[i]);
	if (audio_codec) {
		const size_t bytes = std::min(ps.pcm.size(), ts.pcm.size());
		assert(bytes >= 8192);
		assert(std::equal(ps.pcm.begin(), ps.pcm.begin() + bytes, ts.pcm.begin()));
	} else {
		assert(ts.pcm.empty());
	}
}

// What a Transport Stream session refuses at Begin: audio that is not MPEG
// audio, and the codecs that are not MPEG-1 or MPEG-2 video.
void check_ts_begin_rules()
{
	Mailbox mailbox;
	const uint32_t pcm = mailbox.allocate(131072);
	const auto begin = [&](uint32_t video, uint32_t container, uint32_t audio, uint16_t status) {
		uint8_t request[48] = {};
		put32(request, video); put32(request + 4, container);
		put32(request + 8, 160); put32(request + 12, 120);
		put32(request + 16, 1); put32(request + 20, audio);
		if (audio) {
			put32(request + 24, pcm); put32(request + 28, 131072);
			put32(request + 32, 24576); put32(request + 36, 98304);
		}
		const auto *reply = mailbox.call(0x0b04, request, 44, status);
		if (status == 0) {
			const uint32_t session = get32(reply);
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session);
			mailbox.call(0x0b0d, request, 16);
		}
	};
	begin(1, 3, 3, 3);  // AC-3 is not carried
	begin(1, 3, 2, 3);  // nor LPCM
	begin(3, 3, 0, 3);  // VP8 is WebM only
	begin(4, 3, 0, 3);
	begin(1, 4, 0, 3);  // no container 4
	begin(1, 3, 0, 0);  // MPEG-1 video only
	begin(1, 3, 1, 0);  // MPEG-1 with MP2
#ifdef AMIBERRY_WITH_FFMPEG
	begin(2, 3, 0, 0);
	begin(2, 3, 1, 0);
	begin(2, 3, 3, 3);
#else
	begin(2, 3, 0, 3);
#endif
	// A stream with no PAT and PMT, or with no sync bytes at all, produces
	// no pictures: the session fails once the input has ended.
	const uint32_t staging = mailbox.allocate(4096);
	for (uint32_t video : {1u, 2u}) {
#ifndef AMIBERRY_WITH_FFMPEG
		if (video == 2)
			continue;
#endif
		std::fill(mailbox.sdk.buffer_data(staging), mailbox.sdk.buffer_data(staging) + 4096, 0x55);
		uint8_t request[48] = {};
		put32(request, video); put32(request + 4, 3);
		put32(request + 8, 160); put32(request + 12, 120); put32(request + 16, 1);
		const uint32_t session = get32(mailbox.call(0x0b04, request, 44));
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session); put32(request + 4, staging);
		put32(request + 12, 4096); put32(request + 16, 1);
		mailbox.call(0x0b05, request, 20);
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		uint16_t status = 2;
		for (unsigned attempt = 0; attempt < 200 && status == 2; ++attempt) {
			mailbox.call(0x0b06, request, 16, 0xffff);
			status = mailbox.last_status;
		}
		assert(status == 9);
		mailbox.call(0x0b0d, request, 16);
	}
}

// A planar 4:2:0 picture rebuilt from a YUY2 overlay frame, whose chroma
// rows repeat in pairs.
void planes_from_yuy2(const std::vector<uint8_t> &yuy2, uint32_t width, uint32_t height,
	                  std::vector<uint8_t> &planes, media_output::Frame &frame)
{
	const uint32_t cw = width / 2, ch = (height + 1) / 2;
	planes.assign(static_cast<size_t>(width) * height + 2 * static_cast<size_t>(cw) * ch, 0);
	uint8_t *y = planes.data(), *cb = y + static_cast<size_t>(width) * height,
		*cr = cb + static_cast<size_t>(cw) * ch;
	for (uint32_t row = 0; row < height; ++row) {
		const uint8_t *source = yuy2.data() + static_cast<size_t>(row) * width * 2;
		for (uint32_t x = 0; x < width; ++x)
			y[static_cast<size_t>(row) * width + x] = source[x * 2];
		if (row & 1)
			continue;
		for (uint32_t x = 0; x < cw; ++x) {
			cb[static_cast<size_t>(row / 2) * cw + x] = source[x * 4 + 1];
			cr[static_cast<size_t>(row / 2) * cw + x] = source[x * 4 + 3];
		}
	}
	frame.width = width; frame.height = height;
	frame.y = y; frame.cb = cb; frame.cr = cr;
	frame.y_pitch = width; frame.chroma_pitch = cw;
}

// Output format 2: the pictures a shared-buffer session writes are the
// overlay session's pictures of the same stream, converted by the card's
// algorithm, and they alternate between the slots.
void check_shared_output(const char *path, uint32_t video_codec, uint32_t container,
	                     uint32_t width, uint32_t height)
{
	const auto input = read_all(path);
	Play base;
	base.video_codec = video_codec; base.container = container;
	base.width = width; base.height = height;
	const auto reference = play(input, base);
#ifndef AMIBERRY_WITH_FFMPEG
	// MPEG-2 is decoded by FFmpeg, which this build does not have.
	if (video_codec == 2) {
		assert(reference.begin_status == 3);
		return;
	}
#endif
	assert(reference.begin_status == 0 && reference.frames.size() >= 8);
	std::vector<uint8_t> palette(media_output::palette_bytes);
	for (unsigned k = 0; k < 256; ++k) {
		palette[k * 4 + 1] = static_cast<uint8_t>(k);
		palette[k * 4 + 2] = static_cast<uint8_t>(255 - k);
		palette[k * 4 + 3] = static_cast<uint8_t>(k * 7);
	}
	struct Variant {
		uint32_t format, flags, out_width, out_height;
		bool two_slots;
	};
	namespace mo = media_output;
	const Variant variants[] = {
		{ mo::surface_rgb565, 0, width, height, true },
		{ mo::surface_rgb565, mo::flag_dither_ordered | mo::flag_scale_nearest, 96, 72, true },
		{ mo::surface_rgb555, mo::flag_dither_ordered, 200, 150, false },
		{ mo::surface_index8, 6 | mo::flag_dither_ordered, width, height, true },
		{ mo::surface_index8, 8 | mo::flag_palette_host, 80, 60, true },
		{ mo::surface_planar, 5 | mo::flag_planar_interleaved | mo::flag_dither_ordered, width, height, false },
		{ mo::surface_planar, 4 | mo::flag_palette_host | mo::flag_dither_ordered, 100, 61, true },
	};
	for (const auto &variant : variants) {
		Play shared = base;
		shared.output_format = 2;
		auto &c = shared.config;
		c.width = variant.out_width; c.height = variant.out_height;
		c.format = variant.format; c.flags = variant.flags;
		const uint32_t row = variant.format == mo::surface_index8 ? c.width :
			variant.format == mo::surface_planar ? (c.width + 7) / 8 : c.width * 2;
		c.pitch = (row + 3) & ~3u;
		c.buffer_offset = 4;
		mo::Layout layout;
		assert(mo::check(c, layout) == 0);
		c.slot_stride = variant.two_slots ? layout.picture_bytes + 12 : 0;
		if (variant.flags & mo::flag_palette_host)
			shared.palette = palette;
		const auto got = play(input, shared);
		assert(got.begin_status == 0);
		assert(got.frames.size() >= 8);
		assert(got.picture_bytes == layout.picture_bytes);
		mo::Converter converter;
		mo::Job job;
		job.width = c.width; job.height = c.height; job.format = c.format;
		job.pitch = c.pitch; job.flags = c.flags; job.depth = layout.depth;
		const size_t frames = std::min(got.frames.size(), reference.frames.size());
		for (size_t i = 0; i < frames; ++i) {
			std::vector<uint8_t> planes, expected(layout.picture_bytes);
			mo::Frame frame;
			planes_from_yuy2(reference.frames[i], width, height, planes, frame);
			assert(converter.convert(job, frame, expected.data(),
				(c.flags & mo::flag_palette_host) ? palette.data() : nullptr) == 0);
			assert(got.frames[i] == expected);
			assert(got.slots[i] == (variant.two_slots ? i & 1 : 0));
		}
	}
}

// OUTPUT_CONFIG and the rest of the shared-buffer protocol.
void check_shared_protocol(const char *path, uint32_t width, uint32_t height)
{
	namespace mo = media_output;
	const auto input = read_all(path);
	Mailbox mailbox;
	const uint32_t staging = mailbox.allocate(65536);
	const uint32_t slots = mailbox.allocate(200000);
	const uint32_t small = mailbox.allocate(1000);
	const uint32_t palette = mailbox.allocate(1024);
	uint8_t request[48] = {};
	const auto begin = [&](uint32_t output_format, uint16_t status) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, 1); put32(request + 4, 1);
		put32(request + 8, width); put32(request + 12, height);
		put32(request + 16, output_format);
		return mailbox.call(0x0b04, request, 44, status);
	};
	const auto command = [&](uint16_t opcode, uint32_t session, uint32_t page_or_flags,
	                         uint16_t status, uint16_t length = 16) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session);
		put32(request + 4, opcode == 0x0b0a ? page_or_flags : 0);
		return mailbox.call(opcode, request, length, status);
	};
	begin(0, 4);                 // output formats are 1 and 2
	begin(3, 4);
	uint32_t session = get32(begin(2, 0));
	assert(session);
	begin(2, 2);                 // one session at a time
	const auto config = [&](uint32_t buffer, uint32_t offset, uint32_t stride, uint32_t w,
	                        uint32_t h, uint32_t format, uint32_t pitch, uint32_t flags,
	                        uint32_t palette_handle, uint32_t palette_offset, uint16_t status,
	                        uint32_t session_id = 0, uint16_t length = 48, uint32_t reserved = 0) {
		std::fill(std::begin(request), std::end(request), 0);
		put32(request, session_id ? session_id : session); put32(request + 4, buffer);
		put32(request + 8, offset); put32(request + 12, stride);
		put32(request + 16, w); put32(request + 20, h);
		put32(request + 24, format); put32(request + 28, pitch);
		put32(request + 32, flags); put32(request + 36, palette_handle);
		put32(request + 40, palette_offset); put32(request + 44, reserved);
		return mailbox.call(0x0b0e, request, length, status);
	};
	// Page 6 before anything is configured: nothing configured, the flags
	// the emulation accepts in value[3].
	auto *reply = command(0x0b0a, session, 6, 0);
	assert(get32(reply + 8) == 6 && get32(reply + 12) == 0);
	assert(get64(reply + 16) == 0 && get64(reply + 24) == 0 && get64(reply + 32) == 0);
	assert(get64(reply + 40) == mo::flags_supported);
	assert((get64(reply + 40) & mo::flag_dither_diffusion) == 0);
	// An overlay-style session state: the page that says whether the overlay
	// is owned reports no (this session never presents to it).
	reply = command(0x0b0a, session, 4, 0);
	assert((get32(reply + 12) & 8) == 0);
	// Malformed requests.
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4, 0, 44);   // short
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4, 0, 48, 1); // reserved
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 5, session + 1); // no such session
	config(slots, 0, 0, 0, 50, mo::surface_rgb565, 200, 0, 0, 0, 4);
	config(slots, 0, 0, 100, 50, 2, 400, 0, 0, 0, 3);                              // ARGB8888
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, mo::flag_dither_diffusion, 0, 0, 3);
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 199, 0, 0, 0, 4);              // pitch
	config(slots, 2, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4);              // offset
	config(slots, 0, 9996, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4);            // stride
	config(0x7777, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 5);             // no buffer
	config(small, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4);              // too small
	config(slots, 192000, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4);
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, mo::flag_palette_host, 0, 0, 4);
	config(slots, 0, 0, 100, 50, mo::surface_index8, 100, mo::flag_palette_host | 8,
		0x7777, 0, 5);
	config(slots, 0, 0, 100, 50, mo::surface_index8, 100, mo::flag_palette_host | 8,
		palette, 2, 4);                                                           // unaligned
	config(slots, 0, 0, 100, 50, mo::surface_index8, 100, mo::flag_palette_host | 8,
		palette, 4, 4);                                                           // does not fit
	config(slots, 0, 0, 100, 50, mo::surface_index8, 100, 8, palette, 0, 4);        // not host palette
	// A card-only buffer is not guest-visible.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 65536); put32(request + 4, 16); put32(request + 8, 2);
	const uint32_t card_only = get32(mailbox.call(0x0100, request, 12));
	config(card_only, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 4);
	// PRESENT before OUTPUT_CONFIG: a held picture is needed first.
	size_t offset = 0;
	uint32_t state = 1;
	for (unsigned guard = 0; guard < 400 && state != 3; ++guard) {
		if (offset < input.size()) {
			const uint32_t bytes = static_cast<uint32_t>(std::min<size_t>(4096, input.size() - offset));
			std::copy(input.begin() + offset, input.begin() + offset + bytes,
				mailbox.sdk.buffer_data(staging));
			std::fill(std::begin(request), std::end(request), 0);
			put32(request, session); put32(request + 4, staging); put32(request + 12, bytes);
			put32(request + 16, offset + bytes == input.size());
			offset += get32(mailbox.call(0x0b05, request, 20) + 40);
		}
		state = get32(command(0x0b06, session, 0, 0) + 4);
	}
	assert(state == 3);
	command(0x0b08, session, 0, 4);                // not configured: BAD_REQUEST
	command(0x0b0d, session, 0, 2);                // a held picture: CLOSE is BUSY
	// A valid configuration: two slots, the picture size back.
	reply = config(slots, 8, 20000, 100, 50, mo::surface_planar, 16, 5, 0, 0, 0);
	assert(get32(reply) == session && get32(reply + 40) == 16 * 50 * 5);
	reply = command(0x0b0a, session, 6, 0);
	assert(get32(reply + 12) == 1); // CONFIGURED only: no picture yet
	// The buffer is pinned while the output is bound.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, slots);
	mailbox.call(0x0101, request, 4, 2);
	// PRESENT writes slot 0, then DISCARD of the next picture does not
	// move the slots.
	reply = command(0x0b08, session, 0, 0);
	assert((get32(reply + 44) & ((1u << 9) | (1u << 15) | (1u << 16))) == ((1u << 9) | (1u << 16)));
	assert(get32(reply + 40) == 16 * 50 * 5 && get32(reply + 4) == 2);
	assert(!mailbox.last_dirty); // the overlay is not touched
	reply = command(0x0b0a, session, 6, 0);
	assert(get32(reply + 12) == 3); // CONFIGURED | COMPLETE, slot 0, not pending
	assert(get64(reply + 16) == 1 && get64(reply + 24) == 1);
	assert(get64(reply + 32) < 5000000);
	reply = command(0x0b0a, session, 2, 0); // counters: presented
	assert(get64(reply + 32) == 1);
	command(0x0b08, session, 0, 4);               // nothing is held now
	// Reconfiguring drops the finished picture and starts at slot 0 again.
	reply = config(slots, 8, 20000, 100, 50, mo::surface_planar, 16, 5, 0, 0, 0);
	reply = command(0x0b0a, session, 6, 0);
	assert(get32(reply + 12) == 1);
	// A palette buffer is pinned too, and a new binding releases the old one.
	reply = config(slots, 0, 0, 100, 50, mo::surface_index8, 100, mo::flag_palette_host | 8,
		palette, 0, 0);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, palette);
	mailbox.call(0x0101, request, 4, 2);
	reply = config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 0);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, palette);
	mailbox.call(0x0101, request, 4, 0);
	// Close releases the slot buffer.
	command(0x0b09, session, 0, 4);               // nothing held: DISCARD is BAD_REQUEST
	command(0x0b0a, session, 7, 4);               // past the last page
	command(0x0b0d, session, 0, 0);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, slots);
	mailbox.call(0x0101, request, 4, 0);
	config(slots, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 5);               // the session is gone
	// An overlay session has no output to configure.
	session = get32(begin(1, 0));
	const uint32_t again = mailbox.allocate(20000);
	config(again, 0, 0, 100, 50, mo::surface_rgb565, 200, 0, 0, 0, 3, session);
	command(0x0b0d, session, 0, 0);
	// A Zorro II board has no media service at all.
	Mailbox z2(4 * 1024 * 1024);
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, 0x0b00);
	z2.call(0x0004, request, 4, 10);
	std::fill(std::begin(request), std::end(request), 0);
	z2.call(0x0b0e, request, 48, 3);
	z2.call(0x0b04, request, 44, 3);
}

// 0x0b00..0x0b03 were the experimental video-session ops; they are retired
// and answer UNSUPPORTED whatever they carry, as does an opcode past
// OUTPUT_CONFIG.
void check_retired_video_ops()
{
	Mailbox mailbox;
	uint8_t request[48] = {};
	for (uint16_t opcode : {0x0b00, 0x0b01, 0x0b02, 0x0b03, 0x0b0f}) {
		mailbox.call(opcode, request, 0, 3);
		mailbox.call(opcode, request, 16, 3);
		mailbox.call(opcode, request, 44, 3);
	}
	Mailbox z2(4 * 1024 * 1024);
	for (uint16_t opcode : {0x0b00, 0x0b01, 0x0b02, 0x0b03})
		z2.call(opcode, request, 16, 3);
}
}

int main(int argc, char **argv)
{
	const auto number = [&](int i) {
		return static_cast<uint32_t>(std::strtoul(argv[i], nullptr, 10));
	};
	if (argc == 5 && std::strcmp(argv[1], "--protocol") == 0) {
		// A Program Stream file with MPEG-1 video: rules that need no decode.
		check_retired_video_ops();
		check_ts_begin_rules();
		check_shared_protocol(argv[2], number(3), number(4));
		return 0;
	}
	if ((argc == 8 && std::strcmp(argv[1], "--ts") == 0) ||
	    (argc == 8 && std::strcmp(argv[1], "--m2ts") == 0)) {
		// --ts PS TS video_codec audio_codec width height; --m2ts rewrites
		// the packets as 192-byte M2TS ones first.
		check_ts_matches_ps(argv[2], argv[3], number(4), number(5), number(6), number(7),
			std::strcmp(argv[1], "--m2ts") == 0);
		return 0;
	}
	if (argc == 7 && std::strcmp(argv[1], "--shared") == 0) {
		// --shared FILE video_codec container width height
		check_shared_output(argv[2], number(3), number(4), number(5), number(6));
		return 0;
	}
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
	auto *mb = mailbox.mb;
	assert((get32(mb + 44) & ((1u << 21) | (1u << 22))) ==
	       ((1u << 21) | (1u << 22)));
#ifdef HAVE_MPG123
	assert((get32(mb + 44) & (1u << 7)) != 0);
#else
	assert((get32(mb + 44) & (1u << 7)) == 0);
#endif
	uint8_t request[48] = {};
	put32(request, 0x0b00);
	const auto *reply = mailbox.call(0x0004, request, 4);
	assert(get32(reply + 20) == 15 && // 0x0b00..0x0b0e, with OUTPUT_CONFIG
	       (get32(reply + 12) & ((1u << 16) | (1u << 17) | (1u << 22))) ==
	       ((1u << 16) | (1u << 17) | (1u << 22)));
#ifndef HAVE_MPG123
	assert((get32(reply + 8) & (1u << 7)) == 0);
#endif
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
	// Page 6 is the shared-buffer output, all zero for an overlay session;
	// pages past it answer BAD_REQUEST, as on the card.
	std::fill(std::begin(request), std::end(request), 0);
	put32(request, session); put32(request + 4, 6);
	reply = mailbox.call(0x0b0a, request, 16);
	assert(get32(reply + 12) == 0 && get64(reply + 16) == 0 && get64(reply + 40) == 0);
	put32(request + 4, 7);
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
