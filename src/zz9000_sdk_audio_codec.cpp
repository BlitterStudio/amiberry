/*
 * FFmpeg-backed FLAC and Vorbis decoder for the ZZ9000 SDK stream service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "zz9000_sdk_audio_codec.h"
#include "zz9000_sdk_internal.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>
#ifdef AMIBERRY_WITH_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}
#endif

namespace zz9000_sdk {

struct AudioCodecDecoder::Impl {
	uint32_t codec;
	uint32_t output_format;
	uint32_t output_limit;
	uint32_t sample_rate = 0;
	uint32_t channels = 0;
	uint16_t status = ok;
	bool eof = false;
	bool flushed = false;
	std::vector<uint8_t> input;
	std::vector<uint8_t> output;
	std::vector<uint8_t> packet;
	uint32_t metadata_remaining = 0;
	uint8_t metadata_type = 0;
	bool metadata_last = false;
	bool flac_marker_seen = false;
	bool flac_metadata_complete = false;
	uint32_t vorbis_serial = 0;
	bool vorbis_serial_set = false;
	uint32_t vorbis_headers = 0;
	std::vector<std::vector<uint8_t>> vorbis_header_packets;
	uint64_t decoded_frames = 0;
	uint64_t frames_consumed = 0;
	uint64_t vorbis_granule = 0;
#ifdef AMIBERRY_WITH_FFMPEG
	AVCodecParserContext *parser = nullptr;
	AVCodecContext *context = nullptr;
	SwrContext *resampler = nullptr;
	AVFrame *frame = nullptr;
#endif
};

AudioCodecDecoder::AudioCodecDecoder(uint32_t codec, uint32_t output_format,
	uint32_t output_limit) : impl_(std::make_unique<Impl>())
{
	impl_->codec = codec;
	impl_->output_format = output_format;
	impl_->output_limit = output_limit;
}

AudioCodecDecoder::~AudioCodecDecoder()
{
#ifdef AMIBERRY_WITH_FFMPEG
	if (impl_) {
		av_parser_close(impl_->parser);
		avcodec_free_context(&impl_->context);
		swr_free(&impl_->resampler);
		av_frame_free(&impl_->frame);
	}
#endif
}

void AudioCodecDecoder::append(const uint8_t *data, size_t length)
{
	if (impl_->input.size() + length > 512 * 1024) {
		impl_->status = io_error;
		return;
	}
	impl_->input.insert(impl_->input.end(), data, data + length);
}

#ifdef AMIBERRY_WITH_FFMPEG
namespace {

uint32_t le32(const uint8_t *data)
{
	return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
		(static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t open_codec(AudioCodecDecoder::Impl &state, const uint8_t *extradata,
	size_t extradata_size)
{
	const AVCodecID id = state.codec == 2 ? AV_CODEC_ID_FLAC : AV_CODEC_ID_VORBIS;
	const AVCodec *codec = avcodec_find_decoder(id);
	if (!codec)
		return unsupported;
	state.context = avcodec_alloc_context3(codec);
	state.frame = av_frame_alloc();
	if (!state.context || !state.frame)
		return no_memory;
	state.context->sample_rate = static_cast<int>(state.sample_rate);
	av_channel_layout_default(&state.context->ch_layout, static_cast<int>(state.channels));
	state.context->extradata = static_cast<uint8_t *>(av_mallocz(extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));
	if (!state.context->extradata)
		return no_memory;
	std::memcpy(state.context->extradata, extradata, extradata_size);
	state.context->extradata_size = static_cast<int>(extradata_size);
	if (avcodec_open2(state.context, codec, nullptr) < 0)
		return io_error;
	if (state.codec == 2) {
		state.parser = av_parser_init(id);
		if (!state.parser)
			return no_memory;
	}
	return ok;
}

uint16_t append_frame(AudioCodecDecoder::Impl &state)
{
	if (!state.frame->nb_samples)
		return ok;
	if (state.frame->sample_rate != static_cast<int>(state.sample_rate) ||
	    state.frame->ch_layout.nb_channels != static_cast<int>(state.channels))
		return unsupported;
	if (!state.resampler) {
		const AVSampleFormat destination_format = state.output_format == pcm_s32be ?
			AV_SAMPLE_FMT_S32 : AV_SAMPLE_FMT_S16;
		AVChannelLayout destination_layout{};
		av_channel_layout_default(&destination_layout, static_cast<int>(state.channels));
		if (swr_alloc_set_opts2(&state.resampler, &destination_layout, destination_format,
			state.sample_rate, &state.frame->ch_layout, state.context->sample_fmt,
			state.context->sample_rate, 0, nullptr) < 0) {
			av_channel_layout_uninit(&destination_layout);
			return io_error;
		}
		av_channel_layout_uninit(&destination_layout);
		if (swr_init(state.resampler) < 0)
			return io_error;
	}
	const int samples = swr_get_out_samples(state.resampler, state.frame->nb_samples);
	const int width = state.output_format == pcm_s32be ? 4 : 2;
	if (samples < 0 || static_cast<uint64_t>(samples) * state.channels * width >
		std::numeric_limits<size_t>::max())
		return io_error;
	std::vector<uint8_t> converted(static_cast<size_t>(samples) * state.channels * width);
	uint8_t *destination[] = {converted.data()};
	const int written = swr_convert(state.resampler, destination, samples,
		const_cast<const uint8_t **>(state.frame->extended_data), state.frame->nb_samples);
	if (written < 0)
		return io_error;
	converted.resize(static_cast<size_t>(written) * state.channels * width);
	if (state.output_format == pcm_s16be || state.output_format == pcm_s32be)
		for (size_t offset = 0; offset < converted.size(); offset += width)
			std::reverse(converted.begin() + static_cast<std::ptrdiff_t>(offset),
				converted.begin() + static_cast<std::ptrdiff_t>(offset + width));
	state.output.insert(state.output.end(), converted.begin(), converted.end());
	state.decoded_frames += written;
	return ok;
}

uint16_t receive_frames(AudioCodecDecoder::Impl &state)
{
	for (;;) {
		const int result = avcodec_receive_frame(state.context, state.frame);
		if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
			return ok;
		if (result < 0)
			return io_error;
		const uint16_t status = append_frame(state);
		av_frame_unref(state.frame);
		if (status != ok)
			return status;
	}
}

uint16_t send_packet(AudioCodecDecoder::Impl &state, const uint8_t *data, int size)
{
	AVPacket packet{};
	packet.data = const_cast<uint8_t *>(data);
	packet.size = size;
	if (avcodec_send_packet(state.context, data ? &packet : nullptr) < 0)
		return io_error;
	return receive_frames(state);
}

uint16_t flac_header(AudioCodecDecoder::Impl &state, const uint8_t *data)
{
	const uint32_t rate = (static_cast<uint32_t>(data[10]) << 12) |
		(static_cast<uint32_t>(data[11]) << 4) | (data[12] >> 4);
	const uint32_t channels = ((data[12] >> 1) & 7) + 1;
	const uint32_t bits = ((static_cast<uint32_t>(data[12] & 1) << 4) |
		(data[13] >> 4)) + 1;
	if (rate < 8000 || rate > 192000 || channels < 1 || channels > 2 || bits > 24)
		return unsupported;
	if (state.output_format == pcm_s16be && bits > 16)
		return unsupported;
	state.sample_rate = rate;
	state.channels = channels;
	return open_codec(state, data, 34);
}

uint16_t process_flac(AudioCodecDecoder::Impl &state)
{
	while (state.status == ok && state.output.size() < state.output_limit) {
		if (!state.flac_marker_seen) {
			if (state.input.size() < 4) {
				if (!state.input.empty() &&
				    std::memcmp(state.input.data(), "fLaC", state.input.size()) != 0)
					return io_error;
				break;
			}
			if (std::memcmp(state.input.data(), "fLaC", 4))
				return io_error;
			state.input.erase(state.input.begin(), state.input.begin() + 4);
			state.flac_marker_seen = true;
		}
		if (!state.flac_metadata_complete) {
			if (!state.metadata_remaining) {
				if (state.input.size() < 4)
					break;
				state.metadata_last = (state.input[0] & 0x80) != 0;
				state.metadata_type = state.input[0] & 0x7f;
				state.metadata_remaining = (static_cast<uint32_t>(state.input[1]) << 16) |
					(static_cast<uint32_t>(state.input[2]) << 8) | state.input[3];
				state.input.erase(state.input.begin(), state.input.begin() + 4);
			}
			if (state.metadata_type != 0) {
				const uint32_t skipped = std::min<uint32_t>(state.metadata_remaining,
					static_cast<uint32_t>(state.input.size()));
				state.input.erase(state.input.begin(), state.input.begin() + skipped);
				state.metadata_remaining -= skipped;
				if (state.metadata_remaining)
					break;
			} else {
				if (state.input.size() < state.metadata_remaining)
					break;
				if (state.metadata_remaining != 34 || state.context)
					return io_error;
				const uint16_t status = flac_header(state, state.input.data());
				if (status != ok)
					return status;
				state.input.erase(state.input.begin(),
					state.input.begin() + state.metadata_remaining);
			}
			const bool last = state.metadata_last;
			state.metadata_remaining = 0;
			if (!last)
				continue;
			if (!state.context)
				return io_error;
			state.flac_metadata_complete = true;
		}
		if (state.input.empty())
			break;
		if (state.input.size() > 128 * 1024)
			return io_error;
		uint8_t *packet = nullptr;
		int packet_size = 0;
		const int consumed = av_parser_parse2(state.parser, state.context, &packet, &packet_size,
			state.input.data(), static_cast<int>(std::min<size_t>(state.input.size(), 4096)), AV_NOPTS_VALUE,
			AV_NOPTS_VALUE, 0);
		if (consumed < 0)
			return io_error;
		if (consumed)
			state.input.erase(state.input.begin(), state.input.begin() + consumed);
		if (packet_size) {
			const uint16_t status = send_packet(state, packet, packet_size);
			if (status != ok)
				return status;
		}
		if (!consumed && !packet_size)
			break;
	}
	if (state.eof && !state.flushed && state.output.size() < state.output_limit) {
		if (!state.input.empty() || !state.context)
			return io_error;
		for (;;) {
			uint8_t *packet = nullptr;
			int packet_size = 0;
			if (av_parser_parse2(state.parser, state.context, &packet, &packet_size,
				nullptr, 0, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0) < 0)
				return io_error;
			if (!packet_size)
				break;
			const uint16_t status = send_packet(state, packet, packet_size);
			if (status != ok)
				return status;
		}
		const uint16_t status = send_packet(state, nullptr, 0);
		if (status != ok)
			return status;
		state.flushed = true;
	}
	return ok;
}

bool xiph_length(std::vector<uint8_t> &destination, size_t length)
{
	while (length >= 255) { destination.push_back(255); length -= 255; }
	destination.push_back(static_cast<uint8_t>(length));
	return true;
}

uint16_t vorbis_header(AudioCodecDecoder::Impl &state, const std::vector<uint8_t> &packet)
{
	if (packet.size() < 30 || std::memcmp(packet.data(), "\x01vorbis", 7) ||
		le32(packet.data() + 7) != 0)
		return unsupported;
	const uint32_t channels = packet[11];
	const uint32_t rate = le32(packet.data() + 12);
	if (channels < 1 || channels > 2 || rate < 8000 || rate > 96000)
		return unsupported;
	state.channels = channels;
	state.sample_rate = rate;
	return ok;
}

uint16_t open_vorbis(AudioCodecDecoder::Impl &state)
{
	if (state.vorbis_header_packets.size() != 3)
		return io_error;
	std::vector<uint8_t> extradata;
	extradata.push_back(2);
	xiph_length(extradata, state.vorbis_header_packets[0].size());
	xiph_length(extradata, state.vorbis_header_packets[1].size());
	for (const auto &header : state.vorbis_header_packets)
		extradata.insert(extradata.end(), header.begin(), header.end());
	return open_codec(state, extradata.data(), extradata.size());
}

uint16_t process_vorbis_packet(AudioCodecDecoder::Impl &state, std::vector<uint8_t> &&packet)
{
	if (state.vorbis_headers < 3) {
		if (state.vorbis_headers == 0) {
			const uint16_t status = vorbis_header(state, packet);
			if (status != ok)
				return status;
		} else if (packet.size() < 7 ||
			packet[0] != (state.vorbis_headers == 1 ? 3 : 5) ||
			std::memcmp(packet.data() + 1, "vorbis", 6)) {
			return io_error;
		}
		state.vorbis_header_packets.emplace_back(std::move(packet));
		if (++state.vorbis_headers == 3)
			return open_vorbis(state);
		return ok;
	}
	return send_packet(state, packet.data(), static_cast<int>(packet.size()));
}

uint16_t process_vorbis(AudioCodecDecoder::Impl &state)
{
	const size_t frame_bytes = state.channels * (state.output_format == pcm_s32be ? 4 : 2);
	const size_t trim_tail = 4096 * (frame_bytes ? frame_bytes : 4);
	const size_t max_output = state.output_limit + trim_tail;
	while (state.status == ok && state.output.size() < max_output) {
		if (state.input.size() < 4) {
			if (!state.input.empty() &&
			    std::memcmp(state.input.data(), "OggS", state.input.size()) != 0)
				return io_error;
			break;
		}
		if (std::memcmp(state.input.data(), "OggS", 4) != 0)
			return io_error;
		if (state.input.size() < 27)
			break;
		const uint8_t segments = state.input[26];
		if (state.input.size() < 27u + segments) {
			if (state.input.size() > 65307)
				return io_error;
			break;
		}
		uint32_t body = 0;
		for (uint32_t i = 0; i < segments; ++i)
			body += state.input[27 + i];
		if (state.input.size() < 27u + segments + body) {
			if (state.input.size() > 65307)
				return io_error;
			break;
		}
		if (state.input[4] != 0)
			return io_error;
		const uint32_t serial = le32(state.input.data() + 14);
		const uint8_t header_type = state.input[5];
		if (header_type & 4)
			state.vorbis_granule = static_cast<uint64_t>(le32(state.input.data() + 6)) |
				(static_cast<uint64_t>(le32(state.input.data() + 10)) << 32);
		if (!state.vorbis_serial_set) {
			if (!(header_type & 2))
				return io_error;
			state.vorbis_serial = serial;
			state.vorbis_serial_set = true;
		} else if (serial != state.vorbis_serial) {
			return unsupported;
		}
		if ((header_type & 1) && state.packet.empty())
			return io_error;
		uint32_t body_offset = 27 + segments;
		for (uint32_t i = 0; i < segments; ++i) {
			const uint8_t length = state.input[27 + i];
			if (state.packet.size() + length > 64 * 1024)
				return unsupported;
			state.packet.insert(state.packet.end(), state.input.begin() + body_offset,
				state.input.begin() + body_offset + length);
			body_offset += length;
			if (length < 255) {
				const uint16_t status = process_vorbis_packet(state, std::move(state.packet));
				state.packet.clear();
				if (status != ok)
					return status;
			}
		}
		state.input.erase(state.input.begin(), state.input.begin() + 27 + segments + body);
	}
	if (state.eof && !state.flushed) {
		if (!state.input.empty() || !state.packet.empty() || state.vorbis_headers != 3)
			return io_error;
		const uint16_t status = send_packet(state, nullptr, 0);
		if (status != ok)
			return status;
		const size_t channels = state.channels ? state.channels : 2;
		const size_t final_frame_bytes = channels * (state.output_format == pcm_s32be ? 4 : 2);
		if (state.vorbis_granule && state.decoded_frames > state.vorbis_granule) {
			const uint64_t keep_frames = state.vorbis_granule > state.frames_consumed ?
				(state.vorbis_granule - state.frames_consumed) : 0;
			const size_t keep_bytes = static_cast<size_t>(keep_frames * final_frame_bytes);
			if (keep_bytes < state.output.size())
				state.output.resize(keep_bytes);
		}
		state.flushed = true;
	}
	return ok;
}
} // namespace
#endif

uint16_t AudioCodecDecoder::decode(bool eof)
{
	impl_->eof = impl_->eof || eof;
#ifdef AMIBERRY_WITH_FFMPEG
	HostNearestRounding host_rounding;
	if (impl_->status == ok)
		impl_->status = impl_->codec == 2 ? process_flac(*impl_) : process_vorbis(*impl_);
	return impl_->status;
#else
	return unsupported;
#endif
}

uint32_t AudioCodecDecoder::sample_rate() const { return impl_->sample_rate; }
uint32_t AudioCodecDecoder::channels() const { return impl_->channels; }
const uint8_t *AudioCodecDecoder::pcm_data() const { return impl_->output.data(); }
size_t AudioCodecDecoder::pcm_size() const
{
	if (impl_->codec == 3 && !impl_->flushed) {
		const size_t frame_bytes = impl_->channels * (impl_->output_format == pcm_s32be ? 4 : 2);
		const size_t trim_tail = 4096 * (frame_bytes ? frame_bytes : 4);
		return impl_->output.size() > trim_tail ? (impl_->output.size() - trim_tail) : 0;
	}
	return impl_->output.size();
}
bool AudioCodecDecoder::has_pending() const
{
	return pcm_size() != 0 || (!impl_->input.empty() && !impl_->flushed);
}
void AudioCodecDecoder::consume_pcm(size_t length)
{
	length = std::min(length, impl_->output.size());
	impl_->output.erase(impl_->output.begin(), impl_->output.begin() + length);
	const size_t frame_bytes = impl_->channels * (impl_->output_format == pcm_s32be ? 4 : 2);
	if (frame_bytes)
		impl_->frames_consumed += length / frame_bytes;
}

} // namespace zz9000_sdk
