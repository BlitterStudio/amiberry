/*
 * FFmpeg-backed media session decoder.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_MEDIA_AV_H
#define AMIBERRY_ZZ9000_SDK_MEDIA_AV_H

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace zz9000_sdk {

struct MediaAvFrame {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t frame_rate_num = 0;
	uint32_t frame_rate_den = 1;
	uint64_t pts = UINT64_MAX;
	bool interlaced = false;
	bool keyframe = false;
	std::vector<uint8_t> yuy2;
};

struct MediaAvAudio {
	uint32_t sample_rate = 0;
	uint32_t channels = 0;
	uint64_t pts = UINT64_MAX;
	std::vector<uint8_t> s16be;
};

class MediaAv {
public:
	MediaAv(uint32_t video_codec, uint32_t container, uint32_t audio_codec);
	~MediaAv();
	MediaAv(const MediaAv &) = delete;
	MediaAv &operator=(const MediaAv &) = delete;

	/* Copies up to the bounded input window and returns that byte count. */
	uint32_t append(const uint8_t *data, uint32_t length);
	void finish_input();
	/* -1 malformed/unsupported, 0 need input, 1 frame, 2 end of stream. */
	int decode(MediaAvFrame &frame, uint32_t flags = 0);
	bool pop_audio(MediaAvAudio &audio);
	/* True while decoded PCM is still queued for the guest ring. */
	bool has_audio() const;
	/* Cumulative per-stage pipeline timing for media status page 5, counted
	 * by the worker around the FFmpeg work it performs: video decode,
	 * YUY2 pack and audio decode, in ZZ9KMediaProfileStage order. The
	 * present stage stays zero here; the caller times its own copy. */
	static constexpr uint32_t profile_stages = 4;
	void profile_totals(uint64_t microseconds[profile_stages],
	                    uint64_t calls[profile_stages]) const;
	bool header_ready() const;
	bool failed() const;
	bool worker_waiting() const;

	struct State;

private:
	static constexpr uint32_t input_capacity = 4 * 1024 * 1024;
	static constexpr uint32_t frame_capacity = 3;
	static constexpr uint32_t audio_capacity = 512 * 1024;

	uint32_t video_codec_;
	uint32_t container_;
	uint32_t audio_codec_;
	mutable std::mutex mutex_;
	std::condition_variable input_ready_;
	std::condition_variable output_ready_;
	std::deque<uint8_t> input_;
	std::deque<MediaAvFrame> frames_;
	std::deque<MediaAvAudio> audio_;
	uint32_t audio_bytes_ = 0;
	State *state_ = nullptr;
	std::thread worker_;
	uint64_t profile_us_[profile_stages] = {};
	uint64_t profile_calls_[profile_stages] = {};
	bool eof_ = false;
	bool started_ = false;
	bool header_ready_ = false;
	bool waiting_input_ = false;
	bool failed_ = false;
	bool done_ = false;
	bool abort_ = false;

	static int read_packet(void *opaque, uint8_t *buffer, int size);
	void start_locked();
	void worker_loop();
};

} // namespace zz9000_sdk
#endif
