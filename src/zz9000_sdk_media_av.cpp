/*
 * FFmpeg-backed media session decoder.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "zz9000_sdk_media_av.h"
#include "zz9000_sdk_internal.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>

#ifdef AMIBERRY_WITH_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
}
#endif

namespace zz9000_sdk {
namespace {
constexpr uint32_t codec_mpeg1 = 1;
constexpr uint32_t codec_mpeg2 = 2;
constexpr uint32_t codec_vp8 = 3;
constexpr uint32_t codec_vp9 = 4;
constexpr uint32_t container_mpeg_ps = 1;
constexpr uint32_t container_webm = 2;
constexpr uint32_t audio_none = 0;
constexpr uint32_t audio_mp2 = 1;
constexpr uint32_t audio_lpcm = 2;
constexpr uint32_t audio_ac3 = 3;
constexpr uint32_t audio_opus = 4;
constexpr uint32_t audio_vorbis = 5;

#ifdef AMIBERRY_WITH_FFMPEG
AVCodecID video_id(uint32_t codec)
{
	switch (codec) {
	case codec_mpeg1: return AV_CODEC_ID_MPEG1VIDEO;
	case codec_mpeg2: return AV_CODEC_ID_MPEG2VIDEO;
	case codec_vp8: return AV_CODEC_ID_VP8;
	case codec_vp9: return AV_CODEC_ID_VP9;
	default: return AV_CODEC_ID_NONE;
	}
}

// Which demuxed audio tracks a session's audio codec selects, per container,
// as the card's demuxers do: any MPEG audio layer for MP2 (the card decodes
// with minimp3), DVD LPCM in Program Streams, and A_PCM/INT/LIT (16-bit,
// 48 kHz, mono or stereo) for LPCM in Matroska.
bool audio_track_matches(uint32_t codec, uint32_t container,
	const AVCodecParameters *parameters)
{
	const AVCodecID id = parameters->codec_id;
	switch (codec) {
	case audio_mp2:
		return id == AV_CODEC_ID_MP1 || id == AV_CODEC_ID_MP2 || id == AV_CODEC_ID_MP3;
	case audio_lpcm:
		if (container == container_mpeg_ps)
			return id == AV_CODEC_ID_PCM_DVD;
		return id == AV_CODEC_ID_PCM_S16LE && parameters->sample_rate == 48000 &&
			(parameters->ch_layout.nb_channels == 1 || parameters->ch_layout.nb_channels == 2);
	case audio_ac3: return id == AV_CODEC_ID_AC3;
	case audio_opus: return id == AV_CODEC_ID_OPUS;
	case audio_vorbis: return id == AV_CODEC_ID_VORBIS;
	default: return false;
	}
}

bool expected_combination(uint32_t video, uint32_t container, uint32_t audio)
{
	if (container == container_mpeg_ps)
		return video == codec_mpeg2 && audio <= audio_ac3;
	if (container == container_webm) {
		if (video == codec_vp8 || video == codec_vp9)
			return audio == audio_none || audio == audio_opus || audio == audio_vorbis;
		return (video == codec_mpeg1 || video == codec_mpeg2) && audio <= audio_ac3;
	}
	return false;
}

uint64_t stream_pts(const AVFrame *frame, const AVStream *stream)
{
	const int64_t value = frame->best_effort_timestamp == AV_NOPTS_VALUE ?
		frame->pts : frame->best_effort_timestamp;
	return value == AV_NOPTS_VALUE ? UINT64_MAX : static_cast<uint64_t>(
		std::max<int64_t>(0, av_rescale_q(value, stream->time_base, AVRational{1, 90000})));
}

bool frame_is_420(const AVFrame *frame)
{
	const AVPixFmtDescriptor *description = av_pix_fmt_desc_get(
		static_cast<AVPixelFormat>(frame->format));
	return description && description->nb_components >= 3 &&
		(description->flags & AV_PIX_FMT_FLAG_RGB) == 0 &&
		description->comp[0].depth == 8 && description->log2_chroma_w == 1 &&
		description->log2_chroma_h == 1;
}
#endif
} // namespace

struct MediaAv::State {
#ifdef AMIBERRY_WITH_FFMPEG
	MediaAv *owner = nullptr;
	AVFormatContext *format = nullptr;
	AVIOContext *io = nullptr;
	AVCodecContext *video = nullptr;
	AVCodecContext *audio = nullptr;
	AVFrame *frame = nullptr;
	AVPacket *packet = nullptr;
	SwrContext *swr = nullptr;
	AVChannelLayout in_layout{};
	int in_sample_rate = 0;
	AVSampleFormat in_sample_fmt = AV_SAMPLE_FMT_NONE;
	int video_stream = -1;
	int audio_stream = -1;
#endif
};

MediaAv::MediaAv(uint32_t video_codec, uint32_t container, uint32_t audio_codec)
	: video_codec_(video_codec), container_(container), audio_codec_(audio_codec),
	  state_(new State)
{
#ifdef AMIBERRY_WITH_FFMPEG
	state_->owner = this;
#endif
}

MediaAv::~MediaAv()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		abort_ = true;
	}
	input_ready_.notify_all();
	output_ready_.notify_all();
	if (worker_.joinable())
		worker_.join();
#ifdef AMIBERRY_WITH_FFMPEG
	HostNearestRounding host_rounding;
	swr_free(&state_->swr);
	av_channel_layout_uninit(&state_->in_layout);
	av_frame_free(&state_->frame);
	av_packet_free(&state_->packet);
	avcodec_free_context(&state_->video);
	avcodec_free_context(&state_->audio);
	if (state_->format)
		avformat_close_input(&state_->format);
	if (state_->io) {
		av_freep(&state_->io->buffer);
		avio_context_free(&state_->io);
	}
#endif
	delete state_;
}

void MediaAv::start_locked()
{
	if (!started_) {
		started_ = true;
		worker_ = std::thread(&MediaAv::worker_loop, this);
	}
}

uint32_t MediaAv::append(const uint8_t *data, uint32_t length)
{
	if (!data || !length)
		return 0;
	std::lock_guard<std::mutex> lock(mutex_);
	if (eof_ || abort_ || failed_ || done_)
		return 0;
	start_locked();
	const uint32_t accepted = std::min<uint32_t>(length,
		input_capacity - static_cast<uint32_t>(input_.size()));
	input_.insert(input_.end(), data, data + accepted);
	input_ready_.notify_all();
	return accepted;
}

void MediaAv::finish_input()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (abort_ || eof_)
			return;
		start_locked();
		eof_ = true;
	}
	input_ready_.notify_all();
}

bool MediaAv::header_ready() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return header_ready_;
}

bool MediaAv::failed() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return failed_;
}

bool MediaAv::worker_waiting() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return waiting_input_;
}

int MediaAv::read_packet(void *opaque, uint8_t *buffer, int size)
{
#ifdef AMIBERRY_WITH_FFMPEG
	auto *state = static_cast<State *>(opaque);
	MediaAv *owner = state->owner;
	std::unique_lock<std::mutex> lock(owner->mutex_);
	owner->waiting_input_ = true;
	owner->output_ready_.notify_all();
	owner->input_ready_.wait(lock, [owner] {
		return owner->abort_ || owner->eof_ || !owner->input_.empty();
	});
	owner->waiting_input_ = false;
	if (owner->abort_ || owner->input_.empty())
		return AVERROR_EOF;
	const int bytes = std::min<int>(size, static_cast<int>(owner->input_.size()));
	for (int i = 0; i < bytes; ++i) {
		buffer[i] = owner->input_.front();
		owner->input_.pop_front();
	}
	owner->output_ready_.notify_all();
	return bytes;
#else
	(void)opaque; (void)buffer; (void)size;
	return -1;
#endif
}

bool MediaAv::pop_audio(MediaAvAudio &audio)
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (audio_.empty())
		return false;
	audio = std::move(audio_.front());
	audio_bytes_ -= static_cast<uint32_t>(audio.s16be.size());
	audio_.pop_front();
	output_ready_.notify_all();
	return true;
}

bool MediaAv::has_audio() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return !audio_.empty();
}

void MediaAv::profile_totals(uint64_t microseconds[profile_stages],
                             uint64_t calls[profile_stages]) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (uint32_t stage = 0; stage < profile_stages; ++stage) {
		microseconds[stage] = profile_us_[stage];
		calls[stage] = profile_calls_[stage];
	}
}

int MediaAv::decode(MediaAvFrame &frame, uint32_t flags)
{
	std::unique_lock<std::mutex> lock(mutex_);
	if (!started_)
		return 0;
	/* A bounded wait prevents a false NEED_INPUT while FFmpeg is consuming
	 * queued bytes or flushing after EOF, without stalling a hungry client. */
	if (frames_.empty() && !failed_ && !done_)
		output_ready_.wait_for(lock, std::chrono::milliseconds(100));
	bool selected = false;
	while (!frames_.empty()) {
		frame = std::move(frames_.front());
		frames_.pop_front();
		if (!flags || frame.keyframe) {
			selected = true;
			break;
		}
	}
	output_ready_.notify_all();
	if (selected)
		return 1;
	if (failed_)
		return -1;
	return done_ ? 2 : 0;
}

void MediaAv::worker_loop()
{
#ifndef AMIBERRY_WITH_FFMPEG
	std::lock_guard<std::mutex> lock(mutex_);
	failed_ = true;
	output_ready_.notify_all();
	return;
#else
	HostNearestRounding host_rounding;
	auto fail = [this] {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!abort_)
			failed_ = true;
		output_ready_.notify_all();
	};
	if (!expected_combination(video_codec_, container_, audio_codec_)) {
		fail();
		return;
	}
	state_->format = avformat_alloc_context();
	if (!state_->format) {
		fail();
		return;
	}
	uint8_t *io_buffer = static_cast<uint8_t *>(av_malloc(32768));
	if (!io_buffer) {
		avformat_free_context(state_->format);
		state_->format = nullptr;
		fail();
		return;
	}
	state_->io = avio_alloc_context(io_buffer, 32768, 0, state_, read_packet, nullptr, nullptr);
	if (!state_->io) {
		av_free(io_buffer);
		avformat_free_context(state_->format);
		state_->format = nullptr;
		fail();
		return;
	}
	state_->format->pb = state_->io;
	state_->format->flags |= AVFMT_FLAG_CUSTOM_IO;
	state_->format->probesize = 4096;
	state_->format->max_analyze_duration = 0;
	const AVInputFormat *forced = container_ == container_mpeg_ps ?
		av_find_input_format("mpeg") : av_find_input_format("matroska");
	AVDictionary *options = nullptr;
	av_dict_set_int(&options, "probesize", 4096, 0);
	av_dict_set_int(&options, "analyzeduration", 0, 0);
	if (avformat_open_input(&state_->format, nullptr, forced, &options) < 0) {
		av_dict_free(&options);
		fail();
		return;
	}
	av_dict_free(&options);
	/* Matroska/WebM headers name and configure every track. Do not let
	 * find_stream_info wait for EOF on a live HTTP-style input. */
	if (container_ == container_mpeg_ps &&
	    avformat_find_stream_info(state_->format, nullptr) < 0) {
		fail();
		return;
	}
	const AVCodecID wanted_video = video_id(video_codec_);
	for (unsigned i = 0; i < state_->format->nb_streams; ++i) {
		const AVCodecParameters *parameters = state_->format->streams[i]->codecpar;
		if (parameters->codec_type == AVMEDIA_TYPE_VIDEO &&
			parameters->codec_id == wanted_video && state_->video_stream < 0)
			state_->video_stream = static_cast<int>(i);
		if (audio_codec_ != audio_none && parameters->codec_type == AVMEDIA_TYPE_AUDIO &&
			audio_track_matches(audio_codec_, container_, parameters) && state_->audio_stream < 0)
			state_->audio_stream = static_cast<int>(i);
	}
	if ((state_->video_stream < 0 && state_->audio_stream < 0) ||
		(audio_codec_ != audio_none && state_->audio_stream < 0)) {
		fail();
		return;
	}
	auto open_decoder = [](const AVCodecParameters *parameters) -> AVCodecContext * {
		const AVCodec *codec = avcodec_find_decoder(parameters->codec_id);
		AVCodecContext *context = codec ? avcodec_alloc_context3(codec) : nullptr;
		if (!context || avcodec_parameters_to_context(context, parameters) < 0 ||
			avcodec_open2(context, codec, nullptr) < 0) {
			avcodec_free_context(&context);
			return nullptr;
		}
		return context;
	};
	if (state_->video_stream >= 0)
		state_->video = open_decoder(state_->format->streams[state_->video_stream]->codecpar);
	if (state_->audio_stream >= 0)
		state_->audio = open_decoder(state_->format->streams[state_->audio_stream]->codecpar);
	state_->frame = av_frame_alloc();
	state_->packet = av_packet_alloc();
	if ((state_->video_stream >= 0 && !state_->video) ||
		(state_->audio_stream >= 0 && !state_->audio) || !state_->frame || !state_->packet) {
		fail();
		return;
	}
	{
		std::lock_guard<std::mutex> lock(mutex_);
		header_ready_ = true;
		output_ready_.notify_all();
	}
	auto push_frame = [this](MediaAvFrame &&frame) -> bool {
		std::unique_lock<std::mutex> lock(mutex_);
		output_ready_.wait(lock, [this] { return abort_ || frames_.size() < frame_capacity; });
		if (abort_) return false;
		frames_.push_back(std::move(frame));
		output_ready_.notify_all();
		return true;
	};
	auto push_audio = [this](MediaAvAudio &&audio) -> bool {
		const uint32_t bytes = static_cast<uint32_t>(audio.s16be.size());
		std::unique_lock<std::mutex> lock(mutex_);
		// A single decoded frame larger than the queue (a big Matroska PCM
		// block) is admitted once the queue is empty, or it could never be.
		output_ready_.wait(lock, [this, bytes] {
			return abort_ || audio_.empty() || audio_bytes_ + bytes <= audio_capacity;
		});
		if (abort_) return false;
		audio_bytes_ += bytes;
		audio_.push_back(std::move(audio));
		output_ready_.notify_all();
		return true;
	};
	// Per-stage pipeline timing for media status page 5, matching the card's
	// (microseconds << 32) | calls accumulators: 0 video decode, 1 YUY2 pack,
	// 3 audio decode. Stage 2 (present) is timed by the caller around the
	// 0x0b08 copy, not by this worker. A decode stage counts one call per
	// decoded frame but includes the avcodec_send_packet() time as well,
	// because many FFmpeg decoders do their work there rather than in
	// avcodec_receive_frame().
	auto profile_add = [this](uint32_t stage, uint64_t start_us, bool call) {
		const uint64_t elapsed = monotonic_us() - start_us;
		std::lock_guard<std::mutex> lock(mutex_);
		profile_us_[stage] += elapsed;
		if (call)
			profile_calls_[stage]++;
	};
	auto profile_record = [&](uint32_t stage, uint64_t start_us) {
		profile_add(stage, start_us, true);
	};
	auto send_timed = [&](AVCodecContext *codec, const AVPacket *packet, uint32_t stage) {
		const uint64_t start = monotonic_us();
		const int result = avcodec_send_packet(codec, packet);
		profile_add(stage, start, false);
		return result;
	};
	// Only EAGAIN and EOF end a drain cleanly; any other negative return from
	// FFmpeg is a decoding or demuxing error and fails the session.
	auto drained = [](int result) {
		return result == AVERROR(EAGAIN) || result == AVERROR_EOF;
	};
	auto drain_video = [&]() -> bool {
		if (!state_->video) return true;
		int result;
		while (true) {
			const uint64_t decode_start = monotonic_us();
			result = avcodec_receive_frame(state_->video, state_->frame);
			if (result != 0)
				break;
			profile_record(0, decode_start);
			if (!frame_is_420(state_->frame) || state_->frame->width <= 0 ||
				state_->frame->height <= 0 || state_->frame->width > 1920 ||
				state_->frame->height > 1920 || (state_->frame->width & 1) ||
				static_cast<uint64_t>(state_->frame->width) * state_->frame->height > 1920u * 1088u) {
				av_frame_unref(state_->frame);
				return false;
			}
			MediaAvFrame frame;
			frame.width = static_cast<uint32_t>(state_->frame->width);
			frame.height = static_cast<uint32_t>(state_->frame->height);
			AVStream *stream = state_->format->streams[state_->video_stream];
			const AVRational rate = av_guess_frame_rate(state_->format, stream, state_->frame);
			if (rate.num > 0 && rate.den > 0) {
				frame.frame_rate_num = static_cast<uint32_t>(rate.num);
				frame.frame_rate_den = static_cast<uint32_t>(rate.den);
			}
			frame.pts = stream_pts(state_->frame, stream);
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 7, 100)
			frame.interlaced = (state_->frame->flags & AV_FRAME_FLAG_INTERLACED) != 0;
			frame.keyframe = (state_->frame->flags & AV_FRAME_FLAG_KEY) != 0;
#else
			frame.interlaced = state_->frame->interlaced_frame != 0;
			frame.keyframe = state_->frame->key_frame != 0;
#endif
			{
				const uint64_t pack_start = monotonic_us();
				frame.yuy2.resize(static_cast<size_t>(frame.width) * frame.height * 2);
				for (uint32_t y = 0; y < frame.height; ++y) {
					const uint8_t *luma = state_->frame->data[0] +
						static_cast<ptrdiff_t>(y) * state_->frame->linesize[0];
					const uint8_t *cb = state_->frame->data[1] +
						static_cast<ptrdiff_t>(y / 2) * state_->frame->linesize[1];
					const uint8_t *cr = state_->frame->data[2] +
						static_cast<ptrdiff_t>(y / 2) * state_->frame->linesize[2];
					auto *dst = frame.yuy2.data() + static_cast<size_t>(y) * frame.width * 2;
					for (uint32_t x = 0; x < frame.width; x += 2) {
						dst[x * 2] = luma[x]; dst[x * 2 + 1] = cb[x / 2];
						dst[x * 2 + 2] = luma[x + 1]; dst[x * 2 + 3] = cr[x / 2];
					}
				}
				profile_record(1, pack_start);
			}
			av_frame_unref(state_->frame);
			if (!push_frame(std::move(frame))) return false;
		}
		return drained(result);
	};
	auto drain_audio = [&]() -> bool {
		if (!state_->audio) return true;
		int result;
		while (true) {
			const uint64_t audio_decode_start = monotonic_us();
			result = avcodec_receive_frame(state_->audio, state_->frame);
			if (result != 0)
				break;
			// As on the card, AC-3 is always downmixed to stereo (liba52 with
			// A52_STEREO); the other codecs accept only mono or stereo.
			const bool ac3 = audio_codec_ == audio_ac3;
			const int channels = state_->frame->ch_layout.nb_channels;
			if (channels < 1 || (!ac3 && channels > 2) || state_->frame->sample_rate <= 0) {
				av_frame_unref(state_->frame);
				return false;
			}
			const int in_rate = state_->frame->sample_rate;
			const auto in_fmt = static_cast<AVSampleFormat>(state_->frame->format);
			if (state_->swr && (state_->in_sample_rate != in_rate ||
			    state_->in_sample_fmt != in_fmt ||
			    av_channel_layout_compare(&state_->in_layout, &state_->frame->ch_layout) != 0)) {
				swr_free(&state_->swr);
				av_channel_layout_uninit(&state_->in_layout);
			}
			const int out_channels = ac3 ? 2 : channels;
			const int out_rate = in_rate;
			AVChannelLayout output_layout;
			av_channel_layout_default(&output_layout, out_channels);
			if (!state_->swr) {
				av_channel_layout_copy(&state_->in_layout, &state_->frame->ch_layout);
				state_->in_sample_rate = in_rate;
				state_->in_sample_fmt = in_fmt;
				if (swr_alloc_set_opts2(&state_->swr, &output_layout,
					AV_SAMPLE_FMT_S16, out_rate, &state_->frame->ch_layout,
					in_fmt, in_rate, 0, nullptr) < 0 || !state_->swr || swr_init(state_->swr) < 0) {
					av_channel_layout_uninit(&output_layout);
					av_frame_unref(state_->frame);
					return false;
				}
			}
			const int samples = av_rescale_rnd(swr_get_delay(state_->swr, in_rate) +
				state_->frame->nb_samples, out_rate, in_rate, AV_ROUND_UP);
			MediaAvAudio audio;
			audio.sample_rate = static_cast<uint32_t>(out_rate);
			audio.channels = static_cast<uint32_t>(out_channels);
			audio.pts = stream_pts(state_->frame, state_->format->streams[state_->audio_stream]);
			audio.s16be.resize(static_cast<size_t>(samples) * out_channels * 2);
			uint8_t *output[1] = { audio.s16be.data() };
			const int planes = av_sample_fmt_is_planar(in_fmt) ? channels : 1;
			const uint8_t *input[AV_NUM_DATA_POINTERS] = {};
			for (int i = 0; i < planes && i < AV_NUM_DATA_POINTERS; ++i)
				input[i] = state_->frame->extended_data[i];
			const int count = swr_convert(state_->swr, output, samples, input, state_->frame->nb_samples);
			av_channel_layout_uninit(&output_layout);
			av_frame_unref(state_->frame);
			if (count <= 0) return false;
			audio.s16be.resize(static_cast<size_t>(count) * out_channels * 2);
			for (size_t i = 0; i < audio.s16be.size(); i += 2)
				std::swap(audio.s16be[i], audio.s16be[i + 1]);
			profile_record(3, audio_decode_start);
			if (!push_audio(std::move(audio))) return false;
		}
		return drained(result);
	};
	// Like the card's 1 MiB video FIFO, keep demuxing past a full frame
	// queue: a Program Stream can carry the audio for the picture on show
	// up to ~0.7 s further on, and blocking on video there starves audio.
	constexpr size_t video_packet_budget = 1024 * 1024;
	std::deque<AVPacket *> pending_video;
	size_t pending_video_bytes = 0;
	auto free_pending = [&] {
		for (AVPacket *packet : pending_video)
			av_packet_free(&packet);
		pending_video.clear();
	};
	auto frame_room = [this] {
		std::lock_guard<std::mutex> lock(mutex_);
		return frames_.size() < frame_capacity;
	};
	// Sends queued video while the frame queue has room, or until the queue
	// is back under its budget when force is set (that may block on output).
	auto send_pending_video = [&](bool force) -> bool {
		while (!pending_video.empty() &&
			(force ? pending_video_bytes >= video_packet_budget : frame_room())) {
			AVPacket *packet = pending_video.front();
			pending_video.pop_front();
			pending_video_bytes -= static_cast<size_t>(packet->size);
			const int send = send_timed(state_->video, packet, 0);
			av_packet_free(&packet);
			if (send < 0 || !drain_video()) return false;
		}
		return true;
	};
	for (;;) {
		if (!send_pending_video(false)) { free_pending(); fail(); return; }
		const int read = av_read_frame(state_->format, state_->packet);
		if (read == AVERROR_EOF) break;
		if (read < 0) { free_pending(); fail(); return; }
		int send = 0;
		if (state_->packet->stream_index == state_->video_stream) {
			AVPacket *queued = av_packet_clone(state_->packet);
			if (!queued) send = AVERROR(ENOMEM);
			else {
				pending_video_bytes += static_cast<size_t>(queued->size);
				pending_video.push_back(queued);
			}
		} else if (state_->packet->stream_index == state_->audio_stream) {
			send = send_timed(state_->audio, state_->packet, 3);
		}
		av_packet_unref(state_->packet);
		if (send < 0 || !send_pending_video(true) || !drain_audio()) {
			free_pending();
			fail();
			return;
		}
	}
	while (!pending_video.empty()) {
		AVPacket *packet = pending_video.front();
		pending_video.pop_front();
		const int send = send_timed(state_->video, packet, 0);
		av_packet_free(&packet);
		if (send < 0 || !drain_video()) { free_pending(); fail(); return; }
	}
	if (state_->video) send_timed(state_->video, nullptr, 0);
	if (state_->audio) send_timed(state_->audio, nullptr, 3);
	if (!drain_video() || !drain_audio()) { fail(); return; }
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!abort_) done_ = true;
		output_ready_.notify_all();
	}
#endif
}

} // namespace zz9000_sdk
