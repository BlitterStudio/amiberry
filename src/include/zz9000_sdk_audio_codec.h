/*
 * Host decoder for ZZ9000 SDK codec-aware audio streams.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_AUDIO_CODEC_H
#define AMIBERRY_ZZ9000_SDK_AUDIO_CODEC_H

#include <cstddef>
#include <cstdint>
#include <memory>

namespace zz9000_sdk {

class AudioCodecDecoder {
public:
	AudioCodecDecoder(uint32_t codec, uint32_t output_format, uint32_t output_limit);
	~AudioCodecDecoder();
	AudioCodecDecoder(const AudioCodecDecoder &) = delete;
	AudioCodecDecoder &operator=(const AudioCodecDecoder &) = delete;

	void append(const uint8_t *data, size_t length);
	uint16_t decode(bool eof);
	uint32_t sample_rate() const;
	uint32_t channels() const;
	const uint8_t *pcm_data() const;
	size_t pcm_size() const;
	bool has_pending() const;
	void consume_pcm(size_t length);
	struct Impl;

private:
	std::unique_ptr<Impl> impl_;
};

} // namespace zz9000_sdk

#endif
