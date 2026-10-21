/*
 * ZZ9000 SDK v2 mailbox bootstrap and bounded shared-buffer service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "sysconfig.h"
#include "sysdeps.h"
#include "zz9000_sdk.h"
#include "zz9000_sdk_internal.h"
#include "zz9000_sdk_audio_codec.h"
#include "zz9000_sdk_media_av.h"
#include "zz9000_sdk_console.h"
#include "zz9000_sdk_webp.h"

#include <algorithm>
#include <chrono>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <limits>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#include "../external/ImGuiFileDialog/stb/stb_image.h"


#define PLM_NO_STDIO 1
#define PL_MPEG_IMPLEMENTATION
#include "../external/pl_mpeg/pl_mpeg.h"

#if defined(HAVE_MPG123) && defined(AHI) && !defined(LIBRETRO)
#define ZZ9000_SDK_AUDIO
#include <mpg123.h>
#include <SDL3/SDL.h>
#include "options.h"
#include "sounddep/sound.h"
#include "xwin.h"
#endif

namespace zz9000_sdk {
namespace {

constexpr uint32_t abi_magic = 0x5a5a394b;
constexpr uint32_t request_offset = 128;
constexpr uint32_t completion_offset = request_offset + 64 * 64;
constexpr uint32_t ring_entries = 64;
constexpr uint32_t entry_size = 64;
constexpr uint32_t base_capabilities = (1u << 0) | (1u << 2) | (1u << 3) |
	(1u << 4) | (1u << 5) | (1u << 6) | (1u << 10) | (1u << 13) |
	(1u << 14) | (1u << 15);
constexpr uint32_t image_service_flags = 1u | (1u << 16) | (1u << 17) |
	(1u << 18) | (1u << 19) | (1u << 20) | (1u << 21) |
	(1u << 23) | (1u << 24) | (1u << 25) |
	(1u << 26) | (1u << 27)
#ifdef AMIBERRY_WITH_WEBP
	| (1u << 28) | (1u << 29)
#endif
	;
#if defined(ZZ9000_SDK_AUDIO) || defined(AMIBERRY_WITH_FFMPEG)
constexpr uint32_t audio_capabilities =
	(1u << 7)
#ifdef ZZ9000_SDK_AUDIO
	| (1u << 19) | (1u << 23)
#endif
#ifdef AMIBERRY_WITH_FFMPEG
	| (1u << 28) | (1u << 29)
#endif
	;
#else
constexpr uint32_t audio_capabilities = 0;
#endif
#if defined(ZZ9000_SDK_AUDIO) || defined(AMIBERRY_WITH_FFMPEG)
constexpr uint32_t audio_service_flags =
	1u
#ifdef ZZ9000_SDK_AUDIO
	| (1u << 16) | (1u << 18) | (1u << 19) | (1u << 20) | (1u << 24)
#endif
#ifdef AMIBERRY_WITH_FFMPEG
	| (1u << 25) | (1u << 26)
#endif
	;
#else
constexpr uint32_t audio_service_flags = 0;
#endif
// CARD_POOL_LIMIT_MEDIA_INPUT in the firmware's card_pool.h.
constexpr uint32_t media_input_limit = 64 * 1024 * 1024;
constexpr uint32_t video_capabilities = (1u << 7) | (1u << 21) | (1u << 22);
constexpr uint32_t video_service_flags = 1u | (1u << 16) | (1u << 17) |
	(1u << 18) | (1u << 19) | (1u << 21) | (1u << 22) |
	(1u << 23) | (1u << 24) | (1u << 25)
#ifdef AMIBERRY_WITH_FFMPEG
	| (1u << 27) | (1u << 28) | (1u << 29) | (1u << 30) | (1u << 31)
#endif
	;

uint8_t clamp_byte(int32_t value)
{
	return static_cast<uint8_t>(std::clamp(value, 0, 255));
}

void rgba_to_yuv422cgx(const uint8_t *rgba, uint32_t width, uint32_t height,
	                  uint8_t *dst, uint32_t pitch)
{
	for (uint32_t y = 0; y < height; ++y) {
		const auto *src = rgba + static_cast<size_t>(y) * width * 4;
		auto *row = dst + static_cast<size_t>(y) * pitch;
		for (uint32_t x = 0; x < width; x += 2) {
			const auto *a = src + x * 4;
			const auto *b = src + std::min(x + 1, width - 1) * 4;
			const int32_t r = (a[0] + b[0]) / 2, g = (a[1] + b[1]) / 2;
			const int32_t blue = (a[2] + b[2]) / 2;
			row[0] = clamp_byte(((66 * a[0] + 129 * a[1] + 25 * a[2] + 128) >> 8) + 16);
			row[1] = clamp_byte(((-38 * r - 74 * g + 112 * blue + 128) >> 8) + 128);
			row[2] = clamp_byte(((66 * b[0] + 129 * b[1] + 25 * b[2] + 128) >> 8) + 16);
			row[3] = clamp_byte(((112 * r - 94 * g - 18 * blue + 128) >> 8) + 128);
			row += 4;
		}
	}
}

} // namespace

struct Engine::Audio {
	uint32_t session = 0;
	uint32_t codec = 1;
	uint32_t pcm_capacity = 0;
	uint32_t mp3_capacity = 0;
	// PCM ring resolved once at begin, like the card's pcm_ring_addr, so a
	// FREE_SHARED of the ring cannot dangle it. pcm_ring_owned keeps
	// card-only storage alive for the session's lifetime.
	uint8_t *pcm_ring = nullptr;
	std::shared_ptr<std::vector<uint8_t>> pcm_ring_owned;
	uint32_t pcm_written = 0;
	uint32_t pcm_read = 0;
	// Compressed bytes the decoder has taken out of the input ring.
	// MP3 counts input as consumed only when it produced PCM, as the
	// card's minimp3 loop does (no frame found -> nothing consumed);
	// FLAC/Vorbis count input as it is decoded. bytes_fed - bytes_consumed
	// is the input waiting in the ring.
	uint32_t bytes_consumed = 0;
	uint32_t bytes_fed = 0;
	uint32_t bytes_produced = 0;
	uint32_t frames_decoded = 0;
	uint32_t sample_rate = 0;
	uint32_t channels = 0;
	uint32_t sample_format = pcm_s16le;
	uint32_t samples_per_frame = 1152;
	uint32_t stream_gain = 128;
	bool playing = false;
	bool eof = false;
	bool drain = false;
	bool decoder_pending = false;
	bool backpressure = false;
	bool faulted = false;
	uint16_t fault_status = io_error;
	bool paused = false;
	std::chrono::steady_clock::time_point next_pump{};
	float gain = -1.0f;
	std::unique_ptr<AudioCodecDecoder> codec_decoder;
#ifdef ZZ9000_SDK_AUDIO
	mpg123_handle *decoder = nullptr;
	SDL_AudioStream *output = nullptr;
#endif
};

struct Engine::Image {
	uint32_t session = 0;
	uint32_t codec = 0;
	uint32_t output_format = 0;
	uint32_t tile_handle = 0;
	uint32_t output_mode = 0;
	uint32_t dst_surface = 0, dst_x = 0, dst_y = 0;
	uint32_t dst_width = 0, dst_height = 0, flags = 0;
	uint32_t tile_stride = 0;
	uint32_t tile_rows = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t next_row = 0;
	uint32_t animation_loop_count = 0;
	uint32_t animation_loop_index = 0;
	uint32_t animation_frame_index = 0;
	uint32_t animation_token = 0;
	uint32_t animation_token_sequence = 0;
	uint32_t animation_previous_timestamp = 0;
	bool animation = false;
	bool animation_presented = false;
	bool animation_ended = false;
	bool input_eof = false;
	std::vector<uint8_t> compressed;
	std::vector<uint8_t> rgba;
	std::unique_ptr<WebpDecoder> webp;
};

struct Engine::Media {
	uint32_t session = 0;
	uint32_t width = 0, height = 0;
	uint32_t frame_rate_num = 0, frame_rate_den = 1;
	uint32_t frame_number = 0;
	uint32_t bytes_accepted = 0;
	uint32_t pcm_handle = 0, pcm_capacity = 0;
	uint32_t pcm_low_water = 0, pcm_high_water = 0;
	uint64_t pcm_produced = 0, pcm_acknowledged = 0;
	uint64_t video_pts = UINT64_MAX, audio_pts = UINT64_MAX;
	uint64_t first_audio_pts = UINT64_MAX; // origin of the acknowledged-PCM clock
	uint64_t format_base = 0; // pcm_produced when the current PCM format began
	// Timeline origin (first container PTS) for status page 0; zero for the
	// derived pl_mpeg clock, as the card's derived timelines report.
	uint64_t pts_origin = UINT64_MAX;
	uint64_t decoded_audio_pts = UINT64_MAX; // newest decoded audio, pages 0/1
	uint64_t last_raw_audio_pts = UINT64_MAX; // newest raw chunk PTS, page 0
	uint64_t presented_frames = 0, discarded_frames = 0; // status page 2
	uint32_t sample_rate = 0, channels = 0;
	uint32_t state = 1; // NEED_INPUT
	uint32_t video_codec = 0, container = 0;
	bool audio_enabled = false;
	bool eof = false;
	bool header_ready = false;
	bool held = false;
	bool held_interlaced = false;
	bool av = false;
	plm_buffer_t *input = nullptr;
	plm_t *decoder = nullptr;
	plm_frame_t *held_frame = nullptr;
	std::unique_ptr<MediaAv> av_decoder;
	MediaAvAudio av_audio;
	size_t av_audio_offset = 0;
	std::vector<uint8_t> held_yuy2;
	// Per-stage pipeline timing for status page 5, in ZZ9KMediaProfileStage
	// order, cumulative for the session (a new Media starts at zero, like
	// the card's profile reset at session begin). FFmpeg sessions merge the
	// worker's own counters at status time; these hold the host-side work:
	// the 0x0b08 present copy and the pl_mpeg decode and pack paths.
	uint64_t profile_us[4] = {};
	uint64_t profile_calls[4] = {};
};

Engine::Engine(uint8_t *board_memory, uint32_t board_size)
	: Engine(board_memory, board_memory + memory_base, board_size)
{
}

Engine::Engine(uint8_t *io_memory, uint8_t *vram_memory, uint32_t board_size)
	: io_memory_(io_memory), vram_memory_(vram_memory), board_size_(board_size)
{
	reset();
}

Engine::~Engine()
{
	close_audio();
	close_media();
	image_.reset();
}

uint8_t *Engine::memory_at(uint32_t offset) const
{
	return offset < memory_base ? io_memory_ + offset :
		vram_memory_ + offset - memory_base;
}

uint32_t Engine::supported_capabilities() const
{
	return base_capabilities | audio_capabilities |
		(board_size_ == 0x08000000 ? video_capabilities | (1u << 30) :
		 (aperture_acked_ ? (1u << 20) | (1u << 24) : 0));
}

void Engine::reset()
{
	close_audio();
	close_media();
	image_.reset();
	if (console_)
		console_->reset();
	for (auto &buffer : buffers_)
		buffer = {};
	for (auto &surface : surfaces_)
		surface = {};
	framebuffer_ = {};
	next_surface_handle_ = 1;
	next_handle_ = 1;
	next_audio_session_ = 1;
	next_image_session_ = 1;
	next_media_session_ = 1;
	overlay_ = {};
	aperture_acked_ = false;
	last_status_ = ok;
	auto *mb = memory_at(mailbox_offset);
	std::memset(mb, 0, mailbox_size);
	put32(mb + 0, abi_magic);
	put16(mb + 4, 2);
	put16(mb + 6, 0);
	put32(mb + 8, 128);
	put32(mb + 12, request_offset);
	put32(mb + 16, ring_entries);
	put32(mb + 28, completion_offset);
	put32(mb + 32, ring_entries);
	put32(mb + 44, supported_capabilities());
}

uint16_t Engine::read_register(uint32_t offset) const
{
	switch (offset) {
		case 0x100: return 0x5a39;
		case 0x102: return 0x0200;
		case 0x104: return mailbox_arm_address >> 16;
		case 0x106: return mailbox_arm_address & 0xffff;
		case 0x10a: return last_status_;
		case 0x11c: return board_size_ == 0x00400000 ? 0x5a02 : 0;
		case 0x11e: return board_size_ == 0x00400000 ? 0x0704 : 0;
		default: return 0;
	}
}

bool Engine::write_register(uint32_t offset, uint16_t value)
{
	if (offset == 0x108 && value)
		return poll();
	if (offset == 0x11e && board_size_ == 0x00400000 && value == 0xa502) {
		aperture_acked_ = true;
		put32(memory_at(mailbox_offset) + 44, supported_capabilities());
		return false;
	}
	// Completion IRQ is deliberately unadvertised: SDK clients poll the ring.
	return false;
}

void Engine::set_framebuffer(uint32_t offset, uint32_t width, uint32_t height,
	                         uint32_t pitch, uint32_t format)
{
	const uint32_t bpp = pixel_bytes(format);
	if (!bpp || !width || !height || pitch < width * bpp ||
	    offset > board_size_ ||
	    static_cast<uint64_t>(pitch) * height > board_size_ - offset) {
		framebuffer_ = {};
		return;
	}
	framebuffer_ = {offset, width, height, pitch, format};
}

void Engine::set_overlay(uint32_t offset, uint32_t width, uint32_t height,
                         uint32_t pitch, uint32_t variant, bool configured,
                         bool active, const OverlayGeometry &geometry)
{
	const uint64_t size = static_cast<uint64_t>(pitch) * height;
	const bool geometry_valid = configured && width && height &&
		variant == 0 && pitch >= ((width + 1) / 2) * 4 &&
		offset <= board_size_ && size <= board_size_ - offset;
	// Like a card overlay SET: geometry that cannot be scanned clears every
	// fact, and an active overlay additionally needs a usable destination
	// rect. A disabled but otherwise valid overlay keeps its geometry with
	// the enable bit off, and releases the media session binding just as the
	// card's SET does for an inactive overlay.
	if (!geometry_valid || (active && (geometry.dst_w < 1 || geometry.dst_h < 1))) {
		overlay_ = {};
		return;
	}
	overlay_.offset = offset;
	overlay_.width = width;
	overlay_.height = height;
	overlay_.pitch = pitch;
	overlay_.variant = variant;
	overlay_.dst_x = geometry.dst_x;
	overlay_.dst_y = geometry.dst_y;
	overlay_.dst_w = geometry.dst_w;
	overlay_.dst_h = geometry.dst_h;
	overlay_.screen_w = geometry.screen_w;
	overlay_.screen_h = geometry.screen_h;
	overlay_.configured = configured;
	overlay_.active = configured && active;
	if (!overlay_.active)
		overlay_.presenting_session = 0;
}

Engine::Surface *Engine::find_surface(uint32_t handle)
{
	for (auto &surface : surfaces_)
		if (surface.handle == handle && handle)
			return &surface;
	return nullptr;
}

uint8_t *Engine::surface_pixels(uint32_t handle)
{
	if (handle == framebuffer_handle)
		return framebuffer_.width ? memory_at(framebuffer_.offset) : nullptr;
	auto *surface = find_surface(handle);
	return surface ? surface->pixels.data() : nullptr;
}

uint32_t Engine::surface_width(uint32_t handle) const
{
	if (handle == framebuffer_handle)
		return framebuffer_.width;
	for (const auto &surface : surfaces_)
		if (surface.handle == handle && handle)
			return surface.width;
	return 0;
}

uint32_t Engine::surface_height(uint32_t handle) const
{
	if (handle == framebuffer_handle)
		return framebuffer_.height;
	for (const auto &surface : surfaces_)
		if (surface.handle == handle && handle)
			return surface.height;
	return 0;
}

uint32_t Engine::surface_pitch(uint32_t handle) const
{
	if (handle == framebuffer_handle)
		return framebuffer_.pitch;
	for (const auto &surface : surfaces_)
		if (surface.handle == handle && handle)
			return surface.pitch;
	return 0;
}

uint32_t Engine::surface_format(uint32_t handle) const
{
	if (handle == framebuffer_handle)
		return framebuffer_.format;
	for (const auto &surface : surfaces_)
		if (surface.handle == handle && handle)
			return surface.format;
	return 0;
}

Engine::Buffer *Engine::find_buffer(uint32_t handle)
{
	for (auto &buffer : buffers_)
		if (buffer.handle == handle && handle != 0)
			return &buffer;
	return nullptr;
}

const Engine::Buffer *Engine::find_buffer(uint32_t handle) const
{
	for (const auto &buffer : buffers_)
		if (buffer.handle == handle && handle != 0)
			return &buffer;
	return nullptr;
}

uint8_t *Engine::buffer_data(uint32_t handle)
{
	auto *buffer = find_buffer(handle);
	if (!buffer)
		return nullptr;
	return buffer->card_only
		? buffer->card_only->data() : memory_at(buffer->board_offset);
}

uint32_t Engine::buffer_length(uint32_t handle) const
{
	const auto *buffer = find_buffer(handle);
	return buffer ? buffer->length : 0;
}

Engine::Audio *Engine::find_audio(uint32_t session)
{
	if (!session)
		return nullptr;
	for (auto &audio : audios_)
		if (audio && audio->session == session)
			return audio.get();
	return nullptr;
}

void Engine::close_audio(Audio *audio)
{
	if (!audio)
		return;
#ifdef ZZ9000_SDK_AUDIO
	if (audio->output)
		SDL_DestroyAudioStream(audio->output);
	if (audio->decoder)
		mpg123_delete(audio->decoder);
#endif
	// Closing a bound stream implies stop, as the card's CLOSE does.
	if (audio_playing_session_ == audio->session)
		audio_playing_session_ = 0;
	const auto slot = std::find_if(audios_.begin(), audios_.end(),
		[audio](const std::unique_ptr<Audio> &a) { return a.get() == audio; });
	if (slot != audios_.end())
		slot->reset();
}

void Engine::close_audio()
{
	for (auto &audio : audios_)
		close_audio(audio.get());
	audio_playing_session_ = 0;
}

void Engine::close_media()
{
	if (media_ && media_->decoder)
		plm_destroy(media_->decoder);
	// The card's overlay drops its direct-session binding when the media
	// session closes (overlay_video_session_closed).
	if (media_ && overlay_.presenting_session == media_->session)
		overlay_.presenting_session = 0;
	media_.reset();
}

void Engine::decode_audio(Audio &audio, const uint8_t *input, uint32_t length)
{
	if (audio.faulted)
		return;
	if (audio.codec_decoder) {
		if (input && length)
			audio.codec_decoder->append(input, length);
		// The decoder keeps up to one ring of PCM of its own. When the ring
		// had room, that PCM moves first and a second pass lets the decoder
		// take more input, so freed ring space reaches the input ring at once,
		// as it does on the card.
		for (int pass = 0; pass < 2; ++pass) {
			const uint16_t status = audio.codec_decoder->decode(audio.eof);
			if (status != ok) {
				audio.faulted = true;
				audio.fault_status = status;
				return;
			}
			if (audio.codec_decoder->sample_rate()) {
				audio.sample_rate = audio.codec_decoder->sample_rate();
				audio.channels = audio.codec_decoder->channels();
			}
			const uint32_t used = audio.pcm_written - audio.pcm_read;
			const uint32_t free = audio.pcm_capacity - used;
			const uint32_t bytes = static_cast<uint32_t>(
				std::min<size_t>(audio.codec_decoder->pcm_size(), free));
			if (!bytes)
				break;
			const uint32_t pos = audio.pcm_written % audio.pcm_capacity;
			const uint32_t first = std::min<uint32_t>(bytes, audio.pcm_capacity - pos);
			const auto *decoded = audio.codec_decoder->pcm_data();
			std::memcpy(audio.pcm_ring + pos, decoded, first);
			if (bytes > first)
				std::memcpy(audio.pcm_ring, decoded + first, bytes - first);
			audio.codec_decoder->consume_pcm(bytes);
			audio.pcm_written += bytes;
			audio.bytes_produced += bytes;
			const uint32_t frame_bytes = audio.channels *
				(audio.sample_format == pcm_s32be ? 4 : 2);
			audio.frames_decoded = frame_bytes ? audio.bytes_produced / frame_bytes : 0;
		}
		audio.decoder_pending = audio.codec_decoder->has_pending();
		audio.bytes_consumed = audio.bytes_fed -
			static_cast<uint32_t>(audio.codec_decoder->input_size());
		return;
	}
#ifdef ZZ9000_SDK_AUDIO
	if (!audio.decoder)
		return;
	HostNearestRounding host_rounding;
	// Accepted input goes to mpg123's own staging, as the card's decode
	// loop reads from its input ring; consumption is read back from
	// mpg123 after the decode pass below.
	if (input && length)
		(void)mpg123_feed(audio.decoder, input, length);
	uint8_t decoded[16384];
	for (;;) {
		const uint32_t used = audio.pcm_written - audio.pcm_read;
		const uint32_t free = audio.pcm_capacity - used;
		if (free < 8192) {
			audio.decoder_pending = true;
			break;
		}
		size_t done = 0;
		const size_t output_size = std::min<size_t>(free & ~3u, sizeof decoded);
		const int rc = mpg123_decode(audio.decoder, nullptr, 0,
			decoded, output_size, &done);
		if (rc == MPG123_NEW_FORMAT) {
			long rate = 0;
			int channels = 0;
			int encoding = 0;
			if (mpg123_getformat(audio.decoder, &rate, &channels, &encoding) != MPG123_OK ||
			    rate <= 0 || (channels != 1 && channels != 2) ||
			    encoding != MPG123_ENC_SIGNED_16 ||
			    (audio.sample_rate &&
			     (audio.sample_rate != static_cast<uint32_t>(rate) ||
			      audio.channels != static_cast<uint32_t>(channels)))) {
				audio.faulted = true;
				break;
			}
			audio.sample_rate = static_cast<uint32_t>(rate);
			audio.channels = static_cast<uint32_t>(channels);
			if (audio.sample_format == pcm_s16be) {
				mpg123_frameinfo2 info{};
				if (mpg123_info2(audio.decoder, &info) != MPG123_OK) {
					audio.faulted = true;
					break;
				}
				audio.samples_per_frame = info.layer == 1 ? 384 :
					(info.layer == 3 && info.version != MPG123_1_0 ? 576 : 1152);
			}
		}
		if (done) {
			if (!audio.channels) {
				audio.faulted = true;
				break;
			}
			if (audio.sample_format == pcm_s16be)
				for (size_t i = 0; i + 1 < done; i += 2)
					std::swap(decoded[i], decoded[i + 1]);
			const uint32_t pos = audio.pcm_written % audio.pcm_capacity;
			const uint32_t first = std::min<uint32_t>(done, audio.pcm_capacity - pos);
			std::memcpy(audio.pcm_ring + pos, decoded, first);
			if (done > first)
				std::memcpy(audio.pcm_ring, decoded + first, done - first);
			audio.pcm_written += static_cast<uint32_t>(done);
			audio.bytes_produced += static_cast<uint32_t>(done);
			audio.frames_decoded = audio.bytes_produced /
				(audio.channels * 2 * audio.samples_per_frame);
		}
		if (rc == MPG123_NEW_FORMAT)
			continue;
		if (rc == MPG123_NEED_MORE || (rc == MPG123_OK && !done)) {
			audio.decoder_pending = false;
			break;
		}
		if (rc == MPG123_OK)
			continue;
		if (rc == MPG123_DONE) {
			audio.decoder_pending = false;
			break;
		}
		// A decode error on content (a failed resync and friends) cannot
		// fault an MP3 session on the card: minimp3 keeps scanning for a
		// frame and waits for more input.
		audio.decoder_pending = false;
		break;
	}
	// The card consumes scanned junk rather than staging it: with no
	// frame found, mp3d_find_frame returns the bytes it walked
	// (minimp3.h:1705) and mp3dec_decode_frame reports them consumed
	// (minimp3.h:1734), so garbage, ID3v2 data and junk tails all count
	// as consumed. Account everything mpg123 has taken, every pass.
	long fill = 0;
	double fill_real = 0.0;
	if (mpg123_getstate(audio.decoder, MPG123_BUFFERFILL, &fill,
		&fill_real) == MPG123_OK && fill >= 0 &&
		static_cast<uint64_t>(fill) <= audio.bytes_fed) {
		const uint32_t consumed = audio.bytes_fed -
			static_cast<uint32_t>(fill);
		if (consumed > audio.bytes_consumed)
			audio.bytes_consumed = consumed;
	}
	// An unparsable tail is dropped rather than held forever: at EOF the
	// card discards input that will not decode (sdk_mailbox.c:3839-3844),
	// and a no-progress pass whose retained input already satisfies the
	// card's 4 KiB decode gate (audio_stream_decode_may_run) can only be
	// holding junk -- a complete frame is never that large. Reopen the
	// feed so mpg123 forgets it.
	const uint32_t retained = audio.bytes_fed - audio.bytes_consumed;
	if (retained && !audio.decoder_pending &&
	    (audio.eof || retained >= 4096)) {
		audio.bytes_consumed = audio.bytes_fed;
		mpg123_open_feed(audio.decoder);
	}
#else
	(void)input;
	(void)length;
#endif
}

void Engine::pump_audio(Audio &audio)
{
#ifdef ZZ9000_SDK_AUDIO
	const auto now = std::chrono::steady_clock::now();
	if (now < audio.next_pump)
		return;
	audio.next_pump = now + std::chrono::milliseconds(10);
	if (audio.output) {
		const bool paused = sound_paused();
		if (paused != audio.paused) {
			SDL_ClearAudioStream(audio.output);
			if (paused)
				SDL_PauseAudioStreamDevice(audio.output);
			else
				SDL_ResumeAudioStreamDevice(audio.output);
			audio.paused = paused;
		}
		const float master = (100 - std::clamp(currprefs.sound_volume_master, 0, 100)) / 100.0f;
		const float board = (100 - std::clamp(currprefs.sound_volume_board, 0, 100)) / 100.0f;
		const float gain = sound_muted() ? 0.0f :
			master * board * static_cast<float>(audio.stream_gain) / 128.0f;
		if (gain != audio.gain && SDL_SetAudioStreamGain(audio.output, gain))
			audio.gain = gain;
	}
	if (audio.decoder_pending &&
	    audio.pcm_capacity - (audio.pcm_written - audio.pcm_read) >= 8192)
		decode_audio(audio, nullptr, 0);
	if (!audio.playing || !audio.output)
		return;
	const int queued = SDL_GetAudioStreamQueued(audio.output);
	if (queued < 0) {
		audio.faulted = true;
		return;
	}
	const int rate = audio.sample_rate ? static_cast<int>(audio.sample_rate) : 48000;
	const int channels = audio.channels ? static_cast<int>(audio.channels) : 2;
	const int max_queued = rate * channels;
	if (queued >= max_queued)
		return;
	uint32_t available = audio.pcm_written - audio.pcm_read;
	available = std::min<uint32_t>(available, 16384);
	available = std::min<uint32_t>(available, static_cast<uint32_t>(max_queued - queued));
	if (!available)
		return;
	const uint32_t pos = audio.pcm_read % audio.pcm_capacity;
	const uint32_t first = std::min(available, audio.pcm_capacity - pos);
	if (!SDL_PutAudioStreamData(audio.output, audio.pcm_ring + pos, first)) {
		audio.faulted = true;
		return;
	}
	audio.pcm_read += first;
	if (available > first) {
		if (!SDL_PutAudioStreamData(audio.output, audio.pcm_ring, available - first))
			audio.faulted = true;
		else
			audio.pcm_read += available - first;
	}
#endif
}

void Engine::pump_audio()
{
	for (auto &audio : audios_)
		if (audio)
			pump_audio(*audio);
}

void Engine::audio_result(const Audio &audio, uint8_t *reply,
                          uint16_t *reply_length)
{
	const uint32_t used = audio.pcm_written - audio.pcm_read;
	// Input still waiting in the session's input ring; like the card's
	// audio_stream_needs_more_input, MP3 waits for 4 KiB before decoding.
	const uint32_t retained = audio.codec_decoder ?
		static_cast<uint32_t>(audio.codec_decoder->input_size()) :
		audio.bytes_fed - audio.bytes_consumed;
	bool output_drained = true;
#ifdef ZZ9000_SDK_AUDIO
	if (audio.output)
		output_drained = SDL_GetAudioStreamQueued(audio.output) <= 0;
#endif
	// EOF/DONE needs the input ring empty, as on the card. A resumable
	// drain completes while a partial frame is still retained: the card
	// sets drain_input_complete as soon as the decoder blocks on an
	// incomplete frame (audio_stream_drain_input_done) and reports
	// DRAINED with the NEED_INPUT state (audio_stream_transient_drained).
	const bool idle = !used && !audio.decoder_pending && output_drained;
	const bool drained = audio.drain && idle;
	const bool done = audio.eof && idle && retained == 0;
	// Like audio_stream_needs_more_input, input demand ignores the
	// playback tail still queued on the host: a playing client must be
	// asked for more while the output is still sounding.
	const bool needs_input = !audio.eof && !audio.drain && !used &&
		!audio.decoder_pending && (audio.codec_decoder || retained < 4096);
	const uint32_t state = audio.faulted ? 4 : done ? 3 :
		(needs_input || drained) ? 1 : 2;
	uint32_t flags = ((used || !output_drained) ? 2u : 0u) | (done ? 4u : 0u) |
		(audio.backpressure ? 8u : 0u) | (drained ? 16u : 0u);
	if (needs_input)
		flags |= 1u;
	put32(reply + 0, audio.session);
	put32(reply + 4, state);
	put32(reply + 8, audio.sample_rate);
	put32(reply + 12, audio.channels);
	put32(reply + 16, audio.sample_format);
	put32(reply + 20, audio.bytes_consumed % audio.mp3_capacity);
	put32(reply + 24, audio.pcm_written % audio.pcm_capacity);
	put32(reply + 28, audio.pcm_read % audio.pcm_capacity);
	put32(reply + 32, audio.frames_decoded);
	put32(reply + 36, audio.bytes_consumed);
	put32(reply + 40, audio.bytes_produced);
	put32(reply + 44, flags);
	*reply_length = 48;
}

void Engine::media_result(uint8_t *reply, uint16_t *reply_length,
	                      uint32_t bytes_written, uint32_t event_flags)
{
	if (!media_)
		return;
	put32(reply, media_->session);
	put32(reply + 4, media_->state);
	put32(reply + 8, media_->width);
	put32(reply + 12, media_->height);
	put32(reply + 16, media_->frame_rate_num);
	put32(reply + 20, media_->frame_rate_den);
	put32(reply + 24, media_->frame_number);
	put64(reply + 28, media_->video_pts);
	put32(reply + 36, media_->bytes_accepted);
	put32(reply + 40, bytes_written);
	uint32_t flags = event_flags;
	if (media_->header_ready) flags |= 1;
	if (media_->state == 1) flags |= 2;
	if (media_->held) flags |= 4;
	if (media_->state == 4) flags |= 8;
	if (media_->sample_rate) flags |= 1u << 7;
	put32(reply + 44, flags);
	*reply_length = 48;
}

void Engine::media_audio_result(uint8_t *reply, uint16_t *reply_length)
{
	if (!media_)
		return;
	put32(reply, media_->session);
	put32(reply + 4, media_->state);
	put32(reply + 8, media_->sample_rate);
	put32(reply + 12, media_->channels);
	put32(reply + 16, 2); // S16BE
	put64(reply + 20, media_->pcm_produced);
	put64(reply + 28, media_->pcm_acknowledged);
	put64(reply + 36, media_->audio_pts);
	put32(reply + 44, media_->sample_rate ? 1u << 7 : 0);
	*reply_length = 48;
}

void Engine::pump_media_audio()
{
	if (!media_ || !media_->audio_enabled || !media_->pcm_capacity)
		return;
	auto *ring = buffer_data(media_->pcm_handle);
	if (!ring)
		return;
	if (media_->av) {
		while (media_->pcm_produced - media_->pcm_acknowledged < media_->pcm_high_water) {
			if (media_->av_audio_offset == media_->av_audio.s16be.size()) {
				media_->av_audio = {};
				media_->av_audio_offset = 0;
				if (!media_->av_decoder->pop_audio(media_->av_audio))
					break;
				if (media_->av_audio.pts != UINT64_MAX) {
					media_->decoded_audio_pts = media_->av_audio.pts;
					media_->last_raw_audio_pts = media_->av_audio.pts;
					if (media_->pts_origin == UINT64_MAX)
						media_->pts_origin = media_->av_audio.pts;
				}
			}
			// A rate or channel change takes effect only once the guest has
			// acknowledged all PCM in the old format; the clock then restarts
			// from the new chunk's timestamp.
			if (media_->av_audio.sample_rate != media_->sample_rate ||
			    media_->av_audio.channels != media_->channels) {
				if (media_->pcm_produced != media_->pcm_acknowledged)
					break;
				media_->sample_rate = media_->av_audio.sample_rate;
				media_->channels = media_->av_audio.channels;
				media_->format_base = media_->pcm_produced;
				media_->first_audio_pts = media_->audio_pts = media_->av_audio.pts;
			}
			const uint64_t used = media_->pcm_produced - media_->pcm_acknowledged;
			const uint32_t space = static_cast<uint32_t>(media_->pcm_capacity - used);
			const uint32_t bytes = std::min<uint32_t>(space,
				static_cast<uint32_t>(media_->av_audio.s16be.size() - media_->av_audio_offset));
			if (!bytes)
				break;
			for (uint32_t i = 0; i < bytes; ++i)
				ring[(media_->pcm_produced + i) % media_->pcm_capacity] =
					media_->av_audio.s16be[media_->av_audio_offset + i];
			media_->pcm_produced += bytes;
			media_->av_audio_offset += bytes;
		}
		return;
	}
	if (!media_->decoder)
		return;
	HostNearestRounding host_rounding;
	for (unsigned block = 0; block < 4; ++block) {
		if (media_->decoder->video_buffer &&
		    plm_buffer_get_remaining(media_->decoder->video_buffer) > 256 * 1024)
			break;
		if (media_->pcm_produced - media_->pcm_acknowledged + 4608 >
		    media_->pcm_capacity || media_->pcm_produced - media_->pcm_acknowledged >=
		    media_->pcm_high_water)
			break;
		const uint64_t audio_decode_start = monotonic_us();
		plm_samples_t *samples = plm_decode_audio(media_->decoder);
		if (samples) {
			media_->profile_us[3] += monotonic_us() - audio_decode_start;
			media_->profile_calls[3]++;
			// pl_mpeg's audio clock is derived from the sample count; that
			// is the newest-decoded-audio fact this decoder can report.
			media_->decoded_audio_pts = static_cast<uint64_t>(
				samples->time * 90000.0 + 0.5);
		}
		if (!samples)
			break;
		media_->sample_rate = static_cast<uint32_t>(plm_get_samplerate(media_->decoder));
		media_->channels = 2;
		if (!media_->sample_rate)
			break;
		if (media_->first_audio_pts == UINT64_MAX)
			media_->first_audio_pts = media_->audio_pts = 0;
		for (uint32_t i = 0; i < samples->count * 2; ++i) {
			const auto scaled = std::lround(std::clamp(samples->interleaved[i],
				-1.0f, 1.0f) * 32767.0f);
			const auto value = static_cast<uint16_t>(static_cast<int16_t>(scaled));
			const uint32_t pos = static_cast<uint32_t>(
				media_->pcm_produced % media_->pcm_capacity);
			ring[pos] = static_cast<uint8_t>(value >> 8);
			ring[(pos + 1) % media_->pcm_capacity] = static_cast<uint8_t>(value);
			media_->pcm_produced += 2;
		}
	}
}

uint16_t Engine::dispatch_media(uint16_t opcode, const uint8_t *request,
	                            uint16_t request_length, uint8_t *reply,
	                            uint16_t *reply_length)
{
	if (board_size_ != 0x08000000)
		return unsupported; // Zorro II PIP aperture is not mapped yet.
	if (opcode == 0x0b04) {
		if (request_length < 44)
			return bad_request;
		if (media_)
			return busy;
		const uint32_t video_codec = be32(request);
		const uint32_t container = be32(request + 4);
		const uint32_t width = be32(request + 8), height = be32(request + 12);
		const uint32_t audio_codec = be32(request + 20);
		if (be32(request + 16) != 1 || be32(request + 40))
			return unsupported;
		const bool legacy_mpeg1 = video_codec == 1 && container == 1 &&
			(audio_codec == 0 || audio_codec == 1);
		const bool av_media =
#ifdef AMIBERRY_WITH_FFMPEG
			(video_codec == 2 && container == 1 && audio_codec <= 3) ||
			((video_codec == 3 || video_codec == 4) && container == 2 &&
			 (audio_codec == 0 || audio_codec == 4 || audio_codec == 5)) ||
			((video_codec == 1 || video_codec == 2) && container == 2 &&
			 audio_codec <= 3);
#else
			false;
#endif
		if (!legacy_mpeg1 && !av_media)
			return unsupported;
		if (!width || !height || width > 1920 || height > 1920 ||
		    static_cast<uint64_t>(width) * height > 1920u * 1088u || (width & 1))
			return bad_request;
		const uint32_t pcm_handle = be32(request + 24);
		const uint32_t pcm_capacity = be32(request + 28);
		const uint32_t low = be32(request + 32), high = be32(request + 36);
		if (audio_codec) {
			const auto *ring = find_buffer(pcm_handle);
			if (!ring)
				return bad_handle;
			if (pcm_capacity < 4608 || pcm_capacity > 256 * 1024 ||
			    pcm_capacity > ring->length || (pcm_capacity & 3) || low >= high ||
			    high < 4608 || high > pcm_capacity || (low & 3) || (high & 3))
				return bad_request;
		} else if (pcm_handle || pcm_capacity || low || high) {
			return bad_request;
		}
		media_ = std::make_unique<Media>();
		media_->session = next_media_session_++;
		media_->width = width; media_->height = height;
		media_->video_codec = video_codec; media_->container = container;
		media_->audio_enabled = audio_codec != 0;
		media_->pcm_handle = pcm_handle;
		media_->pcm_capacity = pcm_capacity;
		media_->pcm_low_water = low;
		media_->pcm_high_water = high;
		if (av_media) {
			media_->av = true;
			media_->av_decoder = std::make_unique<MediaAv>(video_codec, container, audio_codec);
		} else {
			HostNearestRounding host_rounding;
			auto *input = plm_buffer_create_with_capacity(256 * 1024);
			if (!input) { media_.reset(); return no_memory; }
			auto *decoder = plm_create_with_buffer(input, TRUE);
			if (!decoder) {
				plm_buffer_destroy(input);
				media_.reset();
				return no_memory;
			}
			plm_set_audio_enabled(decoder, audio_codec == 1);
			media_->input = input;
			media_->decoder = decoder;
			// pl_mpeg carries no container timestamps; the card's derived
			// timelines report a zero origin on status page 0.
			media_->pts_origin = 0;
		}
		media_result(reply, reply_length);
		return ok;
	}
	if (request_length < 16 || !media_ || be32(request) != media_->session)
		return request_length < 16 ? bad_request : bad_handle;
	if (opcode == 0x0b05) {
		const uint32_t handle = be32(request + 4);
		const uint32_t offset = be32(request + 8);
		const uint32_t length = be32(request + 12);
		const uint32_t flags = be32(request + 16);
		if (request_length < 20 || (flags & ~1u) || (!length && !flags))
			return bad_request;
		const auto *source = find_buffer(handle);
		if (!source)
			return bad_handle;
		if (offset > source->length || length > source->length - offset ||
		    media_->eof)
			return bad_request;
		if (media_->av) {
			const uint32_t accepted = length ? media_->av_decoder->append(
				buffer_data(handle) + offset, length) : 0;
			if (!accepted && length) {
				if (media_->av_decoder->failed())
					return io_error;
				media_result(reply, reply_length, 0, 1u << 8);
				return busy;
			}
			media_->bytes_accepted = accepted > UINT32_MAX - media_->bytes_accepted ?
				UINT32_MAX : media_->bytes_accepted + accepted;
			if ((flags & 1) && accepted == length) {
				media_->av_decoder->finish_input();
				media_->eof = true;
			}
			media_result(reply, reply_length, accepted,
				accepted < length ? 1u << 8 : 0);
			return ok;
		}
		const size_t remaining = plm_buffer_get_remaining(media_->input);
		const bool es_backpressure =
			(media_->decoder->video_buffer &&
			 plm_buffer_get_remaining(media_->decoder->video_buffer) >
			 256 * 1024) ||
			(media_->decoder->audio_buffer &&
			 plm_buffer_get_remaining(media_->decoder->audio_buffer) >
			 128 * 1024);
		const uint32_t available = es_backpressure || remaining >= 256 * 1024 ? 0 :
			static_cast<uint32_t>(256 * 1024 - remaining);
		const uint32_t accepted = std::min(length, available);
		if (!accepted && length) {
			media_result(reply, reply_length, 0, 1u << 8);
			return busy;
		}
		if (accepted) {
			HostNearestRounding host_rounding;
			if (plm_buffer_write(media_->input,
				buffer_data(handle) + offset, accepted) != accepted)
				return io_error;
			media_->bytes_accepted = accepted > UINT32_MAX - media_->bytes_accepted
				? UINT32_MAX : media_->bytes_accepted + accepted;
		}
		if ((flags & 1) && accepted == length) {
			plm_buffer_signal_end(media_->input);
			media_->eof = true;
		}
		media_result(reply, reply_length, accepted,
			accepted < length ? 1u << 8 : 0);
		return ok;
	}
	if (opcode == 0x0b06) {
		// Command payload: session, 64-bit value cursor (zero for DECODE),
		// then flags. ZZPlay sets the late-frame skip flags when it falls
		// behind, for example while switching to fullscreen.
		const uint32_t flags = be32(request + 12);
		if (be32(request + 4) || be32(request + 8) || (flags & ~3u))
			return bad_request;
		if (media_->held)
			return busy;
		if (media_->av) {
			MediaAvFrame frame;
			const int result = media_->av_decoder->decode(frame, flags);
			if (result < 0) {
				media_->state = 5;
				return io_error;
			}
			media_->header_ready = media_->av_decoder->header_ready();
			if (result == 1) {
				if (frame.width != media_->width || frame.height != media_->height) {
					media_->state = 5;
					return io_error;
				}
				media_->held_yuy2 = std::move(frame.yuy2);
				media_->held_interlaced = frame.interlaced;
				media_->held = true;
				media_->state = 3;
				media_->frame_number++;
				media_->frame_rate_num = frame.frame_rate_num;
				media_->frame_rate_den = frame.frame_rate_den;
				media_->video_pts = frame.pts;
				// The WebM backend's raw PTS tracks every accepted
				// frame, video included (sdk_video_webm.c:763).
				if (media_->video_codec == 3 || media_->video_codec == 4)
					media_->last_raw_audio_pts = frame.pts;
				if (media_->pts_origin == UINT64_MAX && frame.pts != UINT64_MAX)
					media_->pts_origin = frame.pts;
			} else if (result == 0 && media_->eof) {
				// All input has arrived and the worker is still decoding it
				// (a short file can be fed whole before FFmpeg finishes
				// probing). NEED_INPUT after EOF means a truncated stream to
				// ZZPlay, so ask the guest to retry instead.
				pump_media_audio();
				media_->state = 2;
				media_result(reply, reply_length, 0, 1u << 8);
				return busy;
			} else {
				media_->state = result == 2 ? 4 : 1;
			}
			pump_media_audio();
			// DONE is final for the guest: hold it back while decoded PCM is
			// still waiting for room in the guest ring.
			if (result == 2 && (media_->av_audio_offset < media_->av_audio.s16be.size() ||
			    media_->av_decoder->has_audio())) {
				media_->state = 2;
				media_result(reply, reply_length, 0, 1u << 8);
				return busy;
			}
			media_result(reply, reply_length);
			return ok;
		}
		HostNearestRounding host_rounding;
		media_->header_ready = plm_has_headers(media_->decoder);
		// The system header can be ready while a disabled audio decoder is not.
		if (plm_demux_has_headers(media_->decoder->demux) &&
		    !plm_get_num_video_streams(media_->decoder))
			return io_error;
		if (media_->header_ready) {
			const int width = plm_get_width(media_->decoder);
			const int height = plm_get_height(media_->decoder);
			if ((width && static_cast<uint32_t>(width) != media_->width) ||
			    (height && static_cast<uint32_t>(height) != media_->height))
				return io_error;
		} else if (media_->eof) {
			return io_error;
		}
		pump_media_audio();
		if (media_->audio_enabled && media_->decoder->audio_buffer &&
		    plm_buffer_get_remaining(media_->decoder->audio_buffer) >
		    128 * 1024 &&
		    media_->pcm_produced - media_->pcm_acknowledged >=
		    media_->pcm_high_water) {
			media_result(reply, reply_length, 0, 1u << 8);
			return busy;
		}
		const uint64_t decode_start = monotonic_us();
		auto *frame = plm_decode_video(media_->decoder);
		media_->profile_us[0] += monotonic_us() - decode_start;
		media_->profile_calls[0]++;
		if (frame) {
			if (frame->width != media_->width || frame->height != media_->height)
				return io_error;
			media_->held_frame = frame;
			media_->held = true;
			media_->state = 3;
			media_->frame_number++;
			const double rate = plm_get_framerate(media_->decoder);
			if (rate > 0 && rate < 121) {
				media_->frame_rate_num = static_cast<uint32_t>(std::lround(rate * 1000));
				media_->frame_rate_den = 1000;
			}
			media_->video_pts = static_cast<uint64_t>(
				std::max(0.0, frame->time) * 90000.0 + 0.5);
		} else {
			media_->state = media_->eof && plm_has_ended(media_->decoder) ? 4 : 1;
		}
		media_result(reply, reply_length);
		return ok;
	}
	if (opcode == 0x0b07) {
		if (!media_->audio_enabled)
			return unsupported;
		if (be32(request + 12))
			return bad_request;
		const uint64_t acknowledged =
			(static_cast<uint64_t>(be32(request + 4)) << 32) | be32(request + 8);
		const uint32_t frame_bytes = media_->channels ? media_->channels * 2 : 4;
		if (acknowledged < media_->pcm_acknowledged ||
		    acknowledged > media_->pcm_produced ||
		    ((acknowledged - media_->format_base) % frame_bytes))
			return bad_request;
		media_->pcm_acknowledged = acknowledged;
		pump_media_audio();
		// As on the card, the audio clock is the first decoded timestamp plus
		// the acknowledged PCM duration in the current format.
		if (media_->sample_rate && media_->first_audio_pts != UINT64_MAX) {
			const uint64_t frames = (acknowledged - media_->format_base) /
				(media_->channels * 2);
			media_->audio_pts = media_->first_audio_pts +
				(frames / media_->sample_rate) * 90000 +
				((frames % media_->sample_rate) * 90000) / media_->sample_rate;
		}
		media_audio_result(reply, reply_length);
		return ok;
	}
	if (opcode == 0x0b08 || opcode == 0x0b09) {
		if (be32(request + 4) || be32(request + 8) || be32(request + 12))
			return bad_request;
		if (!media_->held || (!media_->av && !media_->held_frame))
			return bad_request;
		if (opcode == 0x0b08) {
			if (!overlay_.active)
				return busy;
			const bool half = overlay_.width < media_->width ||
				overlay_.height < media_->height;
			// YUY2 pairs pixels, so a half-size row keeps an even width.
			const uint32_t packed_width = half ? (media_->width / 2) & ~1u : media_->width;
			const uint32_t packed_height = half ? media_->height / 2 : media_->height;
			if (!packed_width || !packed_height || overlay_.width < packed_width ||
			    overlay_.height < packed_height || (!media_->av && half))
				return busy;
			if (media_->av) {
				// The card's PRESENT stage covers the core-0 hand-off of a
				// decoded frame to the overlay; for an already-packed frame
				// that hand-off is this copy into the emulated overlay plane.
				const uint64_t present_start = monotonic_us();
				for (uint32_t y = 0; y < packed_height; ++y) {
					auto *dst = memory_at(overlay_.offset +
						static_cast<size_t>(y) * overlay_.pitch);
					const uint8_t *src = media_->held_yuy2.data() +
						static_cast<size_t>(half ? y * 2 : y) * media_->width * 2;
					if (half) {
						for (uint32_t x = 0; x < packed_width; x += 2) {
							const size_t input = static_cast<size_t>(x / 2) * 8;
							dst[x * 2] = src[input];
							dst[x * 2 + 1] = src[input + 1];
							dst[x * 2 + 2] = src[input + 4];
							dst[x * 2 + 3] = src[input + 3];
						}
					} else if (media_->held_interlaced && y + 1 < media_->height) {
						const uint8_t *next = src + static_cast<size_t>(media_->width) * 2;
						for (uint32_t x = 0; x < media_->width * 2; ++x)
							dst[x] = static_cast<uint8_t>((src[x] + next[x] + 1) >> 1);
					} else {
						std::memcpy(dst, src, static_cast<size_t>(media_->width) * 2);
					}
				}
				media_->profile_us[2] += monotonic_us() - present_start;
				media_->profile_calls[2]++;
			} else {
				// A pl_mpeg frame is packed to YUY2 while presenting: the
				// planar conversion below is the work the card times on
				// core 1 as its pack stage.
				const auto *frame = media_->held_frame;
				const uint64_t pack_start = monotonic_us();
				for (uint32_t y = 0; y < media_->height; ++y) {
					auto *dst = memory_at(overlay_.offset +
						static_cast<size_t>(y) * overlay_.pitch);
					for (uint32_t x = 0; x < media_->width; x += 2) {
						const uint32_t luma = y * frame->y.width + x;
						const uint32_t chroma = (y / 2) * frame->cb.width + x / 2;
						dst[0] = frame->y.data[luma];
						dst[1] = frame->cb.data[chroma];
						dst[2] = frame->y.data[luma + 1];
						dst[3] = frame->cr.data[chroma];
						dst += 4;
					}
				}
				media_->profile_us[1] += monotonic_us() - pack_start;
				media_->profile_calls[1]++;
			}
		}
		// The card's PRESENT stage excludes the compose; for a pl_mpeg frame
		// the hand-off that remains is the state flip below, timed here so
		// present calls advance per frame as they do on the card.
		if (opcode == 0x0b08 && !media_->av) {
			const uint64_t present_start = monotonic_us();
			media_->held = false;
			media_->held_interlaced = false;
			media_->held_frame = nullptr;
			media_->state = 2;
			media_->presented_frames++;
			// This session now feeds the overlay, the fact the card reports
			// as the presentation page's OWNED bit.
			overlay_.presenting_session = media_->session;
			media_->profile_us[2] += monotonic_us() - present_start;
			media_->profile_calls[2]++;
			media_result(reply, reply_length, 0, 1u << 9);
			return ok;
		}
		media_->held = false;
		media_->held_interlaced = false;
		media_->held_frame = nullptr;
		media_->state = 2;
		if (opcode == 0x0b08) {
			media_->presented_frames++;
			overlay_.presenting_session = media_->session;
		} else {
			media_->discarded_frames++;
		}
		media_result(reply, reply_length, 0,
			opcode == 0x0b08 ? 1u << 9 : 1u << 10);
		return ok;
	}
	if (opcode == 0x0b0a) {
		if (request_length < 12 || be32(request + 8))
			return bad_request;
		const uint32_t page = be32(request + 4);
		if (page > 5)
			return bad_request;
		put32(reply, media_->session);
		put32(reply + 4, media_->state);
		put32(reply + 8, page);
		// Pages 0, 1 and 3 share the card's audio fact flags: AUDIO_READY
		// once the decoder has produced PCM, BACKPRESSURE while staged PCM
		// waits for the guest at or above the high-water mark. Media AX
		// binding is not advertised here, so no BOUND/PLAYING/DRAINED/
		// UNDERRUN bits appear.
		const uint32_t audio_flags = (media_->sample_rate ? 1u << 7 : 0) |
			(media_->audio_enabled && media_->pcm_capacity &&
			 media_->pcm_produced - media_->pcm_acknowledged >=
				media_->pcm_high_water ? 1u << 8 : 0);
		if (page == 0) {
			put32(reply + 12, audio_flags);
			put64(reply + 16, media_->pts_origin);
			put64(reply + 24, media_->video_pts);
			// The WebM backend reports the acknowledged-PCM audio clock
			// here (audio_pts_at, sdk_video_webm.c:978-1012); the MPEG
			// backends report the newest decoded audio PTS
			// (sdk_video_mpeg2.c:2304).
			put64(reply + 32, media_->video_codec == 3 ||
				media_->video_codec == 4 ?
				media_->audio_pts : media_->decoded_audio_pts);
			// pl_mpeg exposes no container timestamps; its derived audio
			// clock runs from a zero basis, as the card's derived timelines
			// do, so only an FFmpeg session can report the raw chunk PTS.
			put64(reply + 40, media_->av ? media_->last_raw_audio_pts :
				(media_->sample_rate ? 0 : UINT64_MAX));
		} else if (page == 1) {
			put32(reply + 12, audio_flags);
			put64(reply + 16, media_->pcm_produced);
			put64(reply + 24, media_->pcm_acknowledged);
			put64(reply + 32, media_->decoded_audio_pts);
			put64(reply + 40, media_->first_audio_pts);
		} else if (page == 2) {
			put32(reply + 12, 0);
			put64(reply + 16, media_->bytes_accepted);
			put64(reply + 24, media_->frame_number);
			put64(reply + 32, media_->presented_frames);
			put64(reply + 40, media_->discarded_frames);
		} else if (page == 3) {
			const uint32_t frame_bytes = media_->channels ? media_->channels * 2 : 4;
			const uint64_t retired = frame_bytes ? media_->pcm_acknowledged / frame_bytes : 0;
			const uint64_t staged = frame_bytes ? media_->pcm_produced / frame_bytes : 0;
			const uint64_t queued = staged >= retired ? staged - retired : 0;
			put32(reply + 12, audio_flags);
			put64(reply + 16, retired);
			put64(reply + 24, queued);
			put64(reply + 32, staged);
			// The emulator has no card-side audio DMA pump that could
			// starve, so a session in this state reports zero underruns,
			// exactly like a card whose pump never ran dry.
			put64(reply + 40, 0);
		} else if (page == 4) {
			// The PIP emulates the FPGA overlay plane itself: zz9000.cpp
			// scales the YUY2 source during scanout exactly as the PL plane
			// does, and no card-local ARM shadow compositor exists here, so
			// an active overlay always presents through the native path.
			const bool active = overlay_.configured && overlay_.active;
			put32(reply + 12, (overlay_.configured ? 1u : 0u) |
				(active ? 2u | 4u : 0u) |
				(overlay_.presenting_session != 0 &&
				 overlay_.presenting_session == media_->session ? 8u : 0u));
			put64(reply + 16, pack_pair(overlay_.width, overlay_.height));
			put64(reply + 24, pack_pair(overlay_.dst_w, overlay_.dst_h));
			// Signed origins travel as their 16-bit two's-complement
			// pattern and are sign-extended by the reader.
			put64(reply + 32, pack_pair(overlay_.dst_x, overlay_.dst_y));
			put64(reply + 40, pack_pair(overlay_.screen_w, overlay_.screen_h));
		} else {
			// Per-stage timing as (microseconds << 32) | calls, cumulative
			// for the session. NEON_PACK never applies: the host packs in
			// scalar C++, unlike the card's optional NEON kernel.
			put32(reply + 12, 0);
			uint64_t av_us[4] = {}, av_calls[4] = {};
			if (media_->av)
				media_->av_decoder->profile_totals(av_us, av_calls);
			for (uint32_t stage = 0; stage < 4; ++stage)
				put64(reply + 16 + stage * 8,
					(static_cast<uint64_t>(static_cast<uint32_t>(
						media_->profile_us[stage] + av_us[stage])) << 32) |
					static_cast<uint32_t>(media_->profile_calls[stage] +
						av_calls[stage]));
		}
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0b0b) {
		return unsupported;
	}
	if (opcode == 0x0b0c) {
		if (be32(request + 4) || be32(request + 8) || be32(request + 12))
			return bad_request;
		media_audio_result(reply, reply_length);
		return ok;
	}
	if (opcode == 0x0b0d) {
		if (be32(request + 4) || be32(request + 8) || be32(request + 12))
			return bad_request;
		if (media_->held)
			return busy;
		media_result(reply, reply_length);
		close_media();
		return ok;
	}
	return unsupported;
}

uint16_t Engine::dispatch_console(const uint8_t *request, uint16_t request_length,
	                              uint8_t *reply, uint16_t *reply_length)
{
	if (request_length < 34)
		return bad_request;
	auto *pixels = surface_pixels(be32(request));
	auto *output = buffer_data(be32(request + 4));
	const auto *buffer = find_buffer(be32(request + 4));
	if (!pixels || !output || !buffer)
		return bad_handle;
	if (!console_)
		console_ = std::make_unique<ConsoleEncoder>();
	return console_->encode(request, pixels, surface_width(be32(request)),
		surface_height(be32(request)), surface_pitch(be32(request)),
		surface_format(be32(request)), output, buffer->length, reply, reply_length);
}

uint16_t Engine::dispatch(uint16_t opcode, const uint8_t *request,
	                       uint16_t request_length, uint8_t *reply,
	                       uint16_t *reply_length)
{
	*reply_length = 0;
	if (request_length > 48)
		return bad_request;
	if (opcode == 0x8200)
		return dispatch_console(request, request_length, reply, reply_length);
	if (opcode >= 0x0b04 && opcode <= 0x0b0d)
		return dispatch_media(opcode, request, request_length, reply, reply_length);
	if (opcode == 0x0000)
		return ok;
	if (opcode == 0x0002) {
		std::memcpy(reply, request, request_length);
		*reply_length = request_length;
		return ok;
	}
	if (opcode == 0x0001) {
		put32(reply + 0, abi_magic);
		put16(reply + 4, 2);
		put16(reply + 6, 0);
		put32(reply + 8, supported_capabilities());
		put32(reply + 12, 48);
		put32(reply + 16, buffers_.size());
		put32(reply + 20, 0);
		put32(reply + 24, 0);
		put32(reply + 28, ring_entries);
		put32(reply + 32, ring_entries);
		put32(reply + 36, board_size_ == 0x00400000 ?
			(aperture_acked_ ? 0x4000 : host_window_end - host_window_begin) : 0);
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0005) {
		if (board_size_ != 0x00400000 || !aperture_acked_)
			return unsupported;
		put32(reply + 0, 0x0002000f);
		put32(reply + 4, 0x00400000);
		put32(reply + 8, 0x00010000);
		put32(reply + 12, 0x00388000);
		put32(reply + 16, 0x00398000);
		put32(reply + 20, 0x00038000);
		put32(reply + 24, 0x003d0000);
		put32(reply + 28, 0x00010000);
		put32(reply + 32, 0x003e0000);
		put32(reply + 36, 0x00004000);
		put32(reply + 40, 0x003f0000);
		put32(reply + 44, 0x00010000);
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0004) {
		if (request_length < 4)
			return bad_request;
		const uint32_t service = be32(request);
		if (service != 0 && service != 0x100 && service != 0x200 &&
		    service != 0x400 && service != 0x8200 &&
		    (service != 0x0b00 || board_size_ != 0x08000000)
#if defined(ZZ9000_SDK_AUDIO) || defined(AMIBERRY_WITH_FFMPEG)
		    && service != 0x500
#endif
		)
			return not_found;
		put32(reply + 0, service);
		put32(reply + 4, 0x00020000);
		put32(reply + 8, service == 0 ? supported_capabilities() & ~((1u << 2) |
			(1u << 3) | (1u << 4) | (1u << 5) | (1u << 6) |
			(1u << 7) | (1u << 15) | (1u << 19) | (1u << 21) |
			(1u << 22) | (1u << 30)) :
			service == 0x100 ? ((1u << 2) | (1u << 10) |
				(board_size_ == 0x08000000 ? (1u << 30) :
				 (aperture_acked_ ? (1u << 20) : 0))) :
			service == 0x200 ? ((1u << 3) | (1u << 4) | (1u << 15)) :
			service == 0x400 ? ((1u << 5) | (1u << 6)) :
			service == 0x0b00 ? video_capabilities :
			service == 0x8200 ? 0 : audio_capabilities);
		put32(reply + 12, service == 0x100 ?
			(1u | (board_size_ == 0x08000000 ? (1u << 16) : 0)) :
			service == 0x400 ? image_service_flags :
			service == 0x0b00 ? video_service_flags :
			service == 0x500 ? audio_service_flags : 1u);
		put32(reply + 16, service);
		put32(reply + 20, service == 0 ? 6 : service == 0x100 ? 4 :
			service == 0x200 ? 5 : service == 0x400 ?
#ifdef AMIBERRY_WITH_WEBP
			12 :
#else
			8 :
#endif
			service == 0x0b00 ? 14 : service == 0x8200 ? 1 : 23);
		put32(reply + 24, 48);
		const char *name = service == 0 ? "core" : service == 0x100 ?
			"memory" : service == 0x200 ? "surface" : service == 0x400 ? "image" :
			service == 0x0b00 ? "video" : service == 0x8200 ? "console" : "audio";
		std::memcpy(reply + 28, name, std::strlen(name));
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0100) {
		if (request_length < 12)
			return bad_request;
		const uint32_t length = be32(request);
		uint32_t alignment = be32(request + 4);
		const uint32_t flags = be32(request + 8);
		const bool media = (flags & 4) != 0;
		const bool z2 = board_size_ == 0x00400000;
		if (alignment == 0)
			alignment = 16;
		if (!length || (alignment & (alignment - 1)) || (flags & ~7u))
			return bad_request;
		if (media) {
			if (board_size_ != 0x08000000)
				return unsupported;
			if (!(flags & 2) || (flags & 1) || length > media_input_limit ||
				alignment > 64 * 1024)
				return bad_request;
			// Like the card's MEDIA_INPUT pool class, all media-input
			// buffers together share one 64 MiB budget.
			uint32_t media_used = 0;
			for (const auto &buffer : buffers_)
				if (buffer.handle && (buffer.flags & 4))
					media_used += buffer.length;
			if (length > media_input_limit - media_used)
				return no_memory;
		} else if (alignment > 4096 || length > 0x003f0000 ||
			(z2 && (flags & 1) &&
			 length > (aperture_acked_ ? 0x4000 : host_window_end - host_window_begin))) {
			return bad_request;
		}
		auto free_slot = std::find_if(buffers_.begin(), buffers_.end(),
			[](const Buffer &buffer) { return buffer.handle == 0; });
		if (free_slot == buffers_.end())
			return no_memory;
		const bool card_only = media || (z2 && (flags & 2));
		const uint32_t start = z2 && !card_only ? host_window_begin : z3_heap_begin;
		const uint32_t end = z2 && !card_only ?
			(aperture_acked_ ? host_window_begin + 0x4000 : host_window_end) : z3_heap_end;
		uint32_t board_offset = 0;
		if (!card_only) {
			uint32_t candidate = align_up(start, alignment);
			while (candidate <= end && length <= end - candidate) {
				bool overlap = false;
				for (const auto &buffer : buffers_) {
					if (buffer.handle && !buffer.card_only &&
					    candidate < buffer.board_offset + buffer.length &&
					    buffer.board_offset < candidate + length) {
						candidate = align_up(buffer.board_offset + buffer.length, alignment);
						overlap = true;
						break;
					}
				}
				if (!overlap) {
					board_offset = candidate;
					break;
				}
			}
			if (!board_offset)
				return no_memory;
		}
		try {
			if (card_only) {
				free_slot->card_only = std::make_shared<std::vector<uint8_t>>();
				free_slot->card_only->resize(length);
			}
		} catch (...) {
			*free_slot = {};
			return no_memory;
		}
		free_slot->handle = next_handle_++;
		free_slot->board_offset = board_offset;
		free_slot->length = length;
		free_slot->flags = flags;
		put32(reply + 0, free_slot->handle);
		put32(reply + 4, card_only ? 0x02000000 + free_slot->handle * 0x400000 :
			0x00200000 + board_offset - 0x00010000);
		put32(reply + 8, length);
		put32(reply + 12, flags);
		*reply_length = 16;
		return ok;
	}
	if (opcode == 0x0101) {
		if (request_length < 4)
			return bad_request;
		auto *buffer = find_buffer(be32(request));
		if (!buffer)
			return bad_handle;
		if (image_ && image_->tile_handle == buffer->handle)
			return busy;
		// The card refuses only pinned buffers here (sdk_mailbox.c:2510);
		// audio stream rings are never pinned, and the media PCM ring and
		// the image tile buffer are.
		if (media_ && media_->pcm_handle == buffer->handle)
			return busy;
		*buffer = {};
		return ok;
	}
	if (opcode == 0x0102) {
		if (request_length < 13)
			return bad_request;
		auto *buffer = find_buffer(be32(request));
		const uint32_t offset = be32(request + 4);
		const uint32_t length = be32(request + 8);
		if (!buffer)
			return bad_handle;
		if (offset > buffer->length || length > buffer->length - offset)
			return bad_request;
		if (length)
			std::memset(buffer_data(buffer->handle) + offset, request[12], length);
		return ok;
	}
	if (opcode == 0x0103) {
		if (request_length < 20)
			return bad_request;
		auto *dst = find_buffer(be32(request));
		const uint32_t dst_offset = be32(request + 4);
		auto *src = find_buffer(be32(request + 8));
		const uint32_t src_offset = be32(request + 12);
		const uint32_t length = be32(request + 16);
		if (!dst || !src)
			return bad_handle;
		if (dst_offset > dst->length || length > dst->length - dst_offset ||
			src_offset > src->length || length > src->length - src_offset)
			return bad_request;
		if (length)
			std::memmove(buffer_data(dst->handle) + dst_offset,
				buffer_data(src->handle) + src_offset, length);
		return ok;
	}
	if (opcode == 0x0200) {
		if (request_length < 20)
			return bad_request;
		const uint32_t width = be32(request), height = be32(request + 4);
		const uint32_t format = be32(request + 8), flags = be32(request + 12);
		uint32_t pitch = be32(request + 16);
		if (!width || !height || width > 8192 || height > 8192 ||
		    (format != 7
#ifdef AMIBERRY_WITH_WEBP
		     && format != 9
#endif
		    ) || (flags & ~17u) || !(flags & 16u))
			return unsupported;
		const uint32_t minimum_pitch = format == 9 ? ((width + 1) / 2) * 4 : width * 4;
		if (!pitch)
			pitch = minimum_pitch;
		if (pitch < minimum_pitch ||
		    static_cast<uint64_t>(pitch) * height > max_surface_bytes)
			return bad_request;
		auto slot = std::find_if(surfaces_.begin(), surfaces_.end(),
			[](const Surface &surface) { return !surface.handle; });
		if (slot == surfaces_.end())
			return no_memory;
		size_t used = 0;
		for (const auto &surface : surfaces_)
			used += surface.pixels.size();
		const size_t length = static_cast<size_t>(pitch) * height;
		if (length > max_surface_bytes - used)
			return no_memory;
		*slot = {};
		slot->handle = surface_handle_base | next_surface_handle_++;
		slot->width = width; slot->height = height;
		slot->pitch = pitch; slot->format = format;
		slot->pixels.resize(length);
		put32(reply, slot->handle);
		put32(reply + 4, 0x06000000 +
			static_cast<uint32_t>(slot - surfaces_.begin()) * 0x01000000);
		put32(reply + 8, width); put32(reply + 12, height);
		put32(reply + 16, pitch); put32(reply + 20, format);
		put32(reply + 24, 16); put32(reply + 28, static_cast<uint32_t>(length));
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0201) {
		if (request_length < 4)
			return bad_request;
		auto *surface = find_surface(be32(request));
		if (!surface)
			return bad_handle;
		if (image_ && image_->dst_surface == surface->handle)
			return busy;
		*surface = {};
		return ok;
	}
	if (opcode == 0x0202) {
		if (!framebuffer_.width)
			return io_error;
		put32(reply, framebuffer_handle);
		put32(reply + 4, 0x00200000 + framebuffer_.offset - 0x10000);
		put32(reply + 8, framebuffer_.width);
		put32(reply + 12, framebuffer_.height);
		put32(reply + 16, framebuffer_.pitch);
		put32(reply + 20, framebuffer_.format);
		put32(reply + 24, 7);
		put32(reply + 28, framebuffer_.pitch * framebuffer_.height);
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0203) {
		if (request_length < 28)
			return bad_request;
		const uint32_t handle = be32(request);
		auto *pixels = surface_pixels(handle);
		if (!pixels)
			return bad_handle;
		const uint32_t x = be32(request + 4), y = be32(request + 8);
		const uint32_t width = be32(request + 12), height = be32(request + 16);
		const uint32_t color = be32(request + 20), flags = be32(request + 24);
		if (flags)
			return unsupported;
		if (!rect_valid(x, y, width, height, surface_width(handle),
		                surface_height(handle)))
			return bad_request;
		const uint32_t pitch = surface_pitch(handle), format = surface_format(handle);
		const uint32_t bpp = pixel_bytes(format);
		if (!bpp)
			return unsupported;
		for (uint32_t row = y; row < y + height; ++row)
			for (uint32_t col = x; col < x + width; ++col)
				write_pixel(pixels + static_cast<size_t>(row) * pitch + col * bpp,
					format, color >> 16, color >> 8, color);
		return ok;
	}
	if (opcode == 0x0404) {
		if (request_length < 48)
			return bad_request;
		if (image_)
			return busy;
		const uint32_t codec = be32(request);
		const uint32_t output_mode = be32(request + 4);
		const uint32_t dst_surface = be32(request + 8);
		const uint32_t dst_x = be32(request + 12), dst_y = be32(request + 16);
		const uint32_t dst_width = be32(request + 20);
		const uint32_t dst_height = be32(request + 24);
		const uint32_t format = be32(request + 28);
		const uint32_t tile_handle = be32(request + 32);
		const uint32_t stride = be32(request + 36);
		const uint32_t rows = be32(request + 40);
		const uint32_t flags = be32(request + 44);
		if (codec != 1 && codec != 2 && codec != 4)
			return codec == 3 ? unsupported : bad_request;
#ifndef AMIBERRY_WITH_WEBP
		if (codec == 4)
			return unsupported;
#endif
		const bool animation = (flags & 8u) != 0;
		if ((animation && (codec != 4 || flags != 8u)) ||
		    (!animation && (flags & ~3u)))
			return animation ? bad_request : unsupported;
		if (codec == 4 && (flags & 1u))
			return unsupported;
		if ((output_mode != 1 && output_mode != 2 && output_mode != 3) ||
		    (animation && output_mode != 1))
			return animation ? bad_request : unsupported;
		if (animation) {
			if (format != 9 || !surface_pixels(dst_surface))
				return format == 9 ? bad_handle : unsupported;
			if (surface_format(dst_surface) != 9 ||
			    dst_x || dst_y || dst_width != surface_width(dst_surface) ||
			    dst_height != surface_height(dst_surface) || tile_handle || stride || rows)
				return unsupported;
		} else if (format != 2 && format != 3 && format != 7 && format != 8) {
			return unsupported;
		} else if (output_mode == 3) {
			const auto *tile = find_buffer(tile_handle);
			if (!tile)
				return bad_handle;
			if (!stride || !rows || stride > tile->length ||
			    rows > tile->length / stride)
				return bad_request;
		} else {
			if (!surface_pixels(dst_surface))
				return bad_handle;
			if ((output_mode == 2 && dst_surface != framebuffer_handle) ||
			    (codec != 4 && (format != 7 || surface_format(dst_surface) != 7)) ||
			    (codec == 4 && surface_format(dst_surface) != format))
				return unsupported;
			if (!rect_valid(dst_x, dst_y, dst_width, dst_height,
			                surface_width(dst_surface), surface_height(dst_surface)) ||
			    tile_handle || stride || rows)
				return bad_request;
		}
		image_ = std::make_unique<Image>();
		image_->session = next_image_session_++;
		image_->codec = codec;
		image_->output_format = format;
		image_->tile_handle = tile_handle;
		image_->output_mode = output_mode;
		image_->dst_surface = output_mode != 3 ? dst_surface : 0;
		image_->dst_x = dst_x; image_->dst_y = dst_y;
		image_->dst_width = dst_width; image_->dst_height = dst_height;
		image_->flags = flags;
		image_->tile_stride = stride;
		image_->tile_rows = rows;
		image_->animation = animation;
		if (codec == 4)
			image_->webp = std::make_unique<WebpDecoder>();
		put32(reply, image_->session);
		put32(reply + 4, 1); // NEED_INPUT
		put32(reply + 16, format);
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0405) {
		if (request_length < 48)
			return bad_request;
		if (!image_ || be32(request) != image_->session)
			return bad_handle;
		const uint32_t source_handle = be32(request + 4);
		const uint32_t source_offset = be32(request + 8);
		const uint32_t source_length = be32(request + 12);
		const uint32_t flags = be32(request + 16);
		if ((flags & ~1u) || (!source_length && !flags))
			return bad_request;
		const auto *source = find_buffer(source_handle);
		if (!source)
			return bad_handle;
		if (source_offset > source->length ||
		    source_length > source->length - source_offset)
			return bad_request;
		uint32_t state = 1; // NEED_INPUT
		uint32_t tile_y = 0, tile_height = 0, bytes_written = 0;
		uint32_t result_flags = 0;
		if ((!image_->rgba.empty() || image_->input_eof) && source_length)
			return bad_request;
		if (image_->codec == 4) {
			const auto map_webp_result = [](WebpInputResult result) {
				return result == WebpInputResult::limit ? bad_request :
					result == WebpInputResult::no_memory ? no_memory : io_error;
			};
			if (source_length) {
				const auto result = image_->webp->append(
					buffer_data(source_handle) + source_offset, source_length);
				if (result != WebpInputResult::ok)
					return map_webp_result(result);
			}
			if (flags & 1u) {
				image_->input_eof = true;
				// Later EOF-marked feeds (one per tile band) reuse what the
				// first one decoded, as the JPEG/PNG path does.
				if (image_->animation) {
					if (!image_->width) {
						uint32_t width = 0, height = 0, loops = 0;
						const auto result = image_->webp->begin_animation(width, height, loops);
						if (result != WebpInputResult::ok)
							return map_webp_result(result);
						if (width > surface_width(image_->dst_surface) ||
						    height > surface_height(image_->dst_surface) ||
						    surface_pitch(image_->dst_surface) < ((width + 1) / 2) * 4)
							return bad_request;
						image_->width = width;
						image_->height = height;
						image_->animation_loop_count = loops;
					}
					put32(reply, image_->session);
					put32(reply + 4, 6); // ANIMATION_READY
					put32(reply + 8, image_->width);
					put32(reply + 12, image_->height);
					put32(reply + 16, image_->output_format);
					put32(reply + 36, source_length);
					*reply_length = 48;
					return ok;
				}
				if (image_->rgba.empty()) {
					bool animated = false;
					const auto result = image_->webp->decode_still(image_->width, image_->height,
						animated, image_->rgba);
					if (result != WebpInputResult::ok)
						return map_webp_result(result);
					const uint32_t bpp = image_->output_format == 8 ? 3 : 4;
					if (image_->output_mode == 3 &&
					    static_cast<uint64_t>(image_->width) * bpp > image_->tile_stride)
						return bad_request;
				}
			}
		} else {
			if (image_->rgba.empty() && source_length) {
				if (source_length > max_image_input - image_->compressed.size())
					return no_memory;
				const auto *data = buffer_data(source_handle) + source_offset;
				image_->compressed.insert(image_->compressed.end(), data,
					data + source_length);
			}
			if ((flags & 1) && image_->rgba.empty()) {
				const auto &data = image_->compressed;
				const bool signature = image_->codec == 1 ?
					(data.size() >= 2 && data[0] == 0xff && data[1] == 0xd8) :
					(data.size() >= 8 && std::memcmp(data.data(),
						"\x89PNG\r\n\x1a\n", 8) == 0);
				if (!signature || data.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
					return io_error;
				int width = 0, height = 0, channels = 0;
				if (!stbi_info_from_memory(data.data(), static_cast<int>(data.size()),
					&width, &height, &channels) || width <= 0 || height <= 0 ||
					static_cast<uint64_t>(width) * height > max_image_pixels)
					return io_error;
				const uint32_t bpp = image_->output_format == 8 ? 3 : 4;
				if (image_->output_mode == 3 &&
				    static_cast<uint64_t>(width) * bpp > image_->tile_stride)
					return bad_request;
				int decoded_width = 0, decoded_height = 0;
				auto *decoded = stbi_load_from_memory(data.data(),
					static_cast<int>(data.size()), &decoded_width, &decoded_height,
					&channels, 4);
				if (!decoded || decoded_width != width || decoded_height != height) {
					stbi_image_free(decoded);
					return io_error;
				}
				image_->width = static_cast<uint32_t>(width);
				image_->height = static_cast<uint32_t>(height);
				image_->rgba.assign(decoded, decoded + static_cast<size_t>(width) * height * 4);
				stbi_image_free(decoded);
				image_->compressed.clear();
			}
		}
		if (!image_->rgba.empty()) {
			const uint32_t bpp = image_->output_format == 8 ? 3 : 4;
			const uint32_t width = image_->width;
			if (image_->output_mode != 3 && image_->next_row == 0) {
				auto *dst = surface_pixels(image_->dst_surface);
				if (!dst)
					return bad_handle;
				if (surface_format(image_->dst_surface) != 7)
					return unsupported;
				if (!rect_valid(image_->dst_x, image_->dst_y,
				                image_->dst_width, image_->dst_height,
				                surface_width(image_->dst_surface),
				                surface_height(image_->dst_surface)))
					return bad_request;
				uint32_t out_width = width, out_height = image_->height;
				if (image_->flags & 1) {
					const uint64_t x_ratio = (static_cast<uint64_t>(image_->dst_width) << 16) / width;
					const uint64_t y_ratio = (static_cast<uint64_t>(image_->dst_height) << 16) / image_->height;
					const uint64_t ratio = std::min<uint64_t>(1u << 16,
						std::min(x_ratio, y_ratio));
					out_width = std::max<uint32_t>(1, static_cast<uint32_t>(
						(static_cast<uint64_t>(width) * ratio) >> 16));
					out_height = std::max<uint32_t>(1, static_cast<uint32_t>(
						(static_cast<uint64_t>(image_->height) * ratio) >> 16));
				}
				if (out_width > image_->dst_width || out_height > image_->dst_height)
					return bad_request;
				const uint32_t pitch = surface_pitch(image_->dst_surface);
				for (uint32_t y = 0; y < out_height; ++y) {
					const uint32_t sy = static_cast<uint32_t>(
						static_cast<uint64_t>(y) * image_->height / out_height);
					for (uint32_t x = 0; x < out_width; ++x) {
						const uint32_t sx = static_cast<uint32_t>(
							static_cast<uint64_t>(x) * width / out_width);
						const auto *src = image_->rgba.data() +
							(static_cast<size_t>(sy) * width + sx) * 4;
						write_pixel(dst + static_cast<size_t>(image_->dst_y + y) * pitch +
							static_cast<size_t>(image_->dst_x + x) * 4, 7,
							src[0], src[1], src[2], src[3]);
					}
				}
				tile_height = out_height;
				bytes_written = out_width * out_height * 4;
				image_->next_row = image_->height;
				state = 4; // COMPLETE
				result_flags = 1 | ((out_width != width || out_height != image_->height) ? 4u : 0u);
				put32(reply + 28, out_width);
			} else if (image_->output_mode == 3) {
				tile_y = image_->next_row;
				tile_height = std::min(image_->tile_rows, image_->height - tile_y);
				auto *tile = buffer_data(image_->tile_handle);
				if (!tile)
					return bad_handle;
				for (uint32_t row = 0; row < tile_height; ++row) {
				const auto *src = image_->rgba.data() +
					static_cast<size_t>(tile_y + row) * width * 4;
				auto *dst = tile + static_cast<size_t>(row) * image_->tile_stride;
				for (uint32_t x = 0; x < width; ++x) {
					const uint8_t red = src[x * 4];
					const uint8_t green = src[x * 4 + 1];
					const uint8_t blue = src[x * 4 + 2];
					const uint8_t alpha = src[x * 4 + 3];
					if (image_->output_format == 8) {
						dst[x * 3] = red; dst[x * 3 + 1] = green; dst[x * 3 + 2] = blue;
					} else if (image_->output_format == 7) {
						dst[x * 4] = blue; dst[x * 4 + 1] = green;
						dst[x * 4 + 2] = red; dst[x * 4 + 3] = alpha;
					} else if (image_->output_format == 2) {
						dst[x * 4] = alpha; dst[x * 4 + 1] = red;
						dst[x * 4 + 2] = green; dst[x * 4 + 3] = blue;
					} else {
						std::memcpy(dst + x * 4, src + x * 4, 4);
					}
				}
				}
				image_->next_row += tile_height;
				state = tile_height ? 3 : 4; // TILE_READY or COMPLETE
				bytes_written = width * tile_height * bpp;
				result_flags = 1 | (image_->next_row < image_->height ? 2 : 0);
			}
		}
		put32(reply, image_->session);
		put32(reply + 4, state);
		put32(reply + 8, image_->width);
		put32(reply + 12, image_->height);
		put32(reply + 16, image_->output_format);
		put32(reply + 24, tile_y);
		if (image_->output_mode == 3)
			put32(reply + 28, tile_height ? image_->width : 0);
		put32(reply + 32, tile_height);
		put32(reply + 36, source_length);
		put32(reply + 40, bytes_written);
		put32(reply + 44, result_flags);
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0406) {
		if (request_length < 48)
			return bad_request;
		if (!image_ || be32(request) != image_->session)
			return bad_handle;
		if (be32(request + 4))
			return bad_request;
		image_.reset();
		return ok;
	}
	if (opcode >= 0x0408 && opcode <= 0x040b) {
#ifndef AMIBERRY_WITH_WEBP
		return unsupported;
#else
		if (request_length < 48)
			return bad_request;
		const uint32_t session = be32(request), token = be32(request + 4);
		const uint32_t flags = be32(request + 8);
		if ((opcode == 0x0408 || opcode == 0x040b) ? token != 0 : token == 0)
			return bad_request;
		if (!image_ || image_->session != session)
			return bad_handle;
		if (!image_->animation)
			return bad_request;
		const auto result = [&](uint32_t state, uint32_t result_flags,
			uint32_t duration, uint32_t result_token) {
			put32(reply, image_->session); put32(reply + 4, state);
			put32(reply + 8, image_->width); put32(reply + 12, image_->height);
			put32(reply + 16, image_->animation_frame_index ?
				image_->animation_frame_index - 1 : 0);
			put32(reply + 20, duration); put32(reply + 24, image_->animation_loop_index);
			put32(reply + 28, image_->animation_loop_count); put32(reply + 32, result_token);
			put32(reply + 36, 9); put32(reply + 40, result_flags);
			*reply_length = 48;
		};
		if (opcode == 0x0408) {
			if (flags || image_->animation_token || (!image_->animation_ended && !image_->input_eof))
				return bad_request;
			if (image_->animation_ended) {
				result(7, 8, 0, 0);
				return ok;
			}
			if (!image_->webp->animation_has_more()) {
				if (image_->animation_loop_count &&
				    image_->animation_loop_index + 1 >= image_->animation_loop_count) {
					image_->animation_ended = true;
					result(7, 8, 0, 0);
					return ok;
				}
				if (image_->webp->reset_animation() != WebpInputResult::ok)
					return io_error;
				++image_->animation_loop_index;
				image_->animation_frame_index = 0;
				image_->animation_previous_timestamp = 0;
			}
			const uint8_t *rgba = nullptr;
			uint32_t timestamp = 0;
			if (image_->webp->next_animation(rgba, timestamp) != WebpInputResult::ok)
				return io_error;
			auto *dst = surface_pixels(image_->dst_surface);
			if (!dst || surface_format(image_->dst_surface) != 9 ||
			    surface_pitch(image_->dst_surface) < ((image_->width + 1) / 2) * 4)
				return bad_request;
			rgba_to_yuv422cgx(rgba, image_->width, image_->height, dst,
				surface_pitch(image_->dst_surface));
			const uint32_t duration = timestamp >= image_->animation_previous_timestamp ?
				timestamp - image_->animation_previous_timestamp : 0;
			image_->animation_previous_timestamp = timestamp;
			if (++image_->animation_token_sequence == 0)
				++image_->animation_token_sequence;
			image_->animation_token = image_->animation_token_sequence;
			const bool last = !image_->webp->animation_has_more() &&
				image_->animation_loop_count &&
				image_->animation_loop_index + 1 >= image_->animation_loop_count;
			++image_->animation_frame_index;
			result(6, 1 | (last ? 2u : 0u), duration, image_->animation_token);
			return ok;
		}
		if (opcode == 0x0409) {
			if (flags || token != image_->animation_token || image_->animation_presented)
				return bad_request;
			image_->animation_presented = true;
			if (overlay_.active && overlay_.variant == 0) {
				const uint32_t width = std::min(image_->width, overlay_.width);
				const uint32_t rows = std::min(image_->height, overlay_.height);
				const uint32_t bytes = std::min({((width + 1) / 2) * 4,
					surface_pitch(image_->dst_surface), overlay_.pitch});
				const auto *src = surface_pixels(image_->dst_surface);
				for (uint32_t y = 0; y < rows; ++y)
					std::memcpy(memory_at(overlay_.offset + static_cast<size_t>(y) *
						overlay_.pitch), src + static_cast<size_t>(y) *
						surface_pitch(image_->dst_surface), bytes);
			}
			result(6, 4, 0, token);
			return ok;
		}
		if (opcode == 0x040a) {
			if (flags || token != image_->animation_token || !image_->animation_presented)
				return bad_request;
			image_->animation_token = 0;
			image_->animation_presented = false;
			if (!image_->webp->animation_has_more() && image_->animation_loop_count &&
			    image_->animation_loop_index + 1 >= image_->animation_loop_count)
				image_->animation_ended = true;
			result(image_->animation_ended ? 7 : 6, image_->animation_ended ? 8 : 0, 0, 0);
			return ok;
		}
		if (flags || image_->animation_token || (!image_->animation_ended && !image_->input_eof))
			return bad_request;
		if (image_->webp->reset_animation() != WebpInputResult::ok)
			return io_error;
		image_->animation_loop_index = 0;
		image_->animation_frame_index = 0;
		image_->animation_previous_timestamp = 0;
		image_->animation_ended = false;
		result(6, 0, 0, 0);
		return ok;
#endif
	}
	if (opcode == 0x0407) {
		if (request_length < 40)
			return bad_request;
		const uint32_t src_handle = be32(request), dst_handle = be32(request + 4);
		const uint32_t sx = be16(request + 8), sy = be16(request + 10);
		const uint32_t sw = be16(request + 12), sh = be16(request + 14);
		const uint32_t dx = be16(request + 16), dy = be16(request + 18);
		const uint32_t dw = be16(request + 20), dh = be16(request + 22);
		const uint32_t cx = be16(request + 24), cy = be16(request + 26);
		const uint32_t cw = be16(request + 28), ch = be16(request + 30);
		const uint32_t filter = be32(request + 32), flags = be32(request + 36);
		auto *src = surface_pixels(src_handle);
		auto *dst = surface_pixels(dst_handle);
		if (!src || !dst)
			return bad_handle;
		if (flags || filter > 1 || surface_format(src_handle) != 7 ||
		    (surface_format(dst_handle) != 7 &&
		     surface_format(dst_handle) != 1 &&
		     surface_format(dst_handle) != 6) || src_handle == dst_handle)
			return unsupported;
		if (!rect_valid(sx, sy, sw, sh, surface_width(src_handle),
		                surface_height(src_handle)) ||
		    !rect_valid(dx, dy, dw, dh, surface_width(dst_handle),
		                surface_height(dst_handle)) || !cw || !ch)
			return bad_request;
		const uint32_t x0 = std::max(dx, cx), y0 = std::max(dy, cy);
		const uint32_t x1 = std::min(dx + dw, cx + cw);
		const uint32_t y1 = std::min(dy + dh, cy + ch);
		if (x0 >= x1 || y0 >= y1)
			return ok;
		const uint32_t src_pitch = surface_pitch(src_handle);
		const uint32_t dst_pitch = surface_pitch(dst_handle);
		const uint32_t dst_format = surface_format(dst_handle);
		const uint32_t dst_bpp = pixel_bytes(dst_format);
		for (uint32_t y = y0; y < y1; ++y) {
			int64_t yf = ((static_cast<int64_t>(2 * (y - dy) + 1) * sh) << 15) /
				static_cast<int64_t>(dh) - 32768;
			yf = std::clamp<int64_t>(yf, 0, static_cast<int64_t>(sh - 1) << 16);
			const uint32_t iy = sy + static_cast<uint32_t>(yf >> 16);
			const uint32_t jy = std::min(iy + 1, sy + sh - 1);
			const uint32_t wy = static_cast<uint32_t>(yf & 65535);
			for (uint32_t x = x0; x < x1; ++x) {
				int64_t xf = ((static_cast<int64_t>(2 * (x - dx) + 1) * sw) << 15) /
					static_cast<int64_t>(dw) - 32768;
				xf = std::clamp<int64_t>(xf, 0, static_cast<int64_t>(sw - 1) << 16);
				const uint32_t ix = sx + static_cast<uint32_t>(xf >> 16);
				const uint32_t jx = std::min(ix + 1, sx + sw - 1);
				const uint32_t wx = static_cast<uint32_t>(xf & 65535);
				const auto *p00 = src + static_cast<size_t>(iy) * src_pitch + ix * 4;
				const auto *p10 = src + static_cast<size_t>(iy) * src_pitch + jx * 4;
				const auto *p01 = src + static_cast<size_t>(jy) * src_pitch + ix * 4;
				const auto *p11 = src + static_cast<size_t>(jy) * src_pitch + jx * 4;
				uint8_t color[4];
				for (int component = 0; component < 4; ++component) {
					if (filter == 0) {
						color[component] = (wy >= 32768 ?
							(wx >= 32768 ? p11 : p01) :
							(wx >= 32768 ? p10 : p00))[component];
					} else {
						const uint32_t top = (p00[component] * (65536u - wx) +
							p10[component] * wx + 32768u) >> 16;
						const uint32_t bottom = (p01[component] * (65536u - wx) +
							p11[component] * wx + 32768u) >> 16;
						color[component] = static_cast<uint8_t>((top * (65536u - wy) +
							bottom * wy + 32768u) >> 16);
					}
				}
				write_pixel(dst + static_cast<size_t>(y) * dst_pitch + x * dst_bpp,
					dst_format, color[2], color[1], color[0], color[3]);
			}
		}
		return ok;
	}
#if defined(ZZ9000_SDK_AUDIO) || defined(AMIBERRY_WITH_FFMPEG)
#ifdef ZZ9000_SDK_AUDIO
	if (opcode == 0x0503) {
		if (request_length < 40)
			return bad_request;
		const auto *mp3 = find_buffer(be32(request));
		const auto *pcm = find_buffer(be32(request + 8));
		if (!mp3 || !pcm || mp3 == pcm)
			return bad_handle;
		const uint32_t mp3_capacity = be32(request + 4);
		const uint32_t pcm_capacity = be32(request + 12);
		if (!mp3_capacity || mp3_capacity > mp3->length || pcm_capacity < 8192 ||
		    pcm_capacity > pcm->length || (pcm_capacity & 3) ||
		    be32(request + 28) >= pcm_capacity || be32(request + 32) >= pcm_capacity)
			return bad_request;
		if (be32(request + 16) || be32(request + 20) || be32(request + 36) ||
		    (be32(request + 24) != pcm_s16le &&
		     be32(request + 24) != pcm_s16be))
			return unsupported;
		// The card keeps four stream slots and answers NO_MEMORY when they
		// are all taken (sdk_mailbox.c:4950); it never answers BUSY here.
		auto slot = std::find_if(audios_.begin(), audios_.end(),
			[](const std::unique_ptr<Audio> &audio) { return !audio; });
		if (slot == audios_.end())
			return no_memory;
		HostNearestRounding host_rounding;
		static const bool mpg123_ready = mpg123_init() == MPG123_OK;
		if (!mpg123_ready)
			return io_error;
		int decoder_error = MPG123_OK;
		auto *decoder = mpg123_new(nullptr, &decoder_error);
		if (!decoder)
			return no_memory;
		const uint32_t sample_format = be32(request + 24);
		// minimp3 on the card scans any amount of junk for a frame header
		// and never reports it; let mpg123 resync without a limit and keep
		// its diagnostics off the host's stderr.
		bool configured = mpg123_format_none(decoder) == MPG123_OK &&
			mpg123_param(decoder, MPG123_RESYNC_LIMIT, -1, 0.0) == MPG123_OK &&
			mpg123_param(decoder, MPG123_ADD_FLAGS, MPG123_QUIET, 0.0) == MPG123_OK;
		if (sample_format == pcm_s16le) {
			configured = configured &&
				mpg123_param(decoder, MPG123_FORCE_RATE, 48000, 0.0) == MPG123_OK &&
				mpg123_format(decoder, 48000, MPG123_STEREO,
				              MPG123_ENC_SIGNED_16) == MPG123_OK;
		} else {
			for (const long rate : {8000L, 11025L, 12000L, 16000L, 22050L,
			                       24000L, 32000L, 44100L, 48000L})
				configured = configured &&
					mpg123_format(decoder, rate, MPG123_MONO | MPG123_STEREO,
					              MPG123_ENC_SIGNED_16) == MPG123_OK;
		}
		configured = configured && mpg123_open_feed(decoder) == MPG123_OK;
		if (!configured) {
			mpg123_delete(decoder);
			return io_error;
		}
		*slot = std::make_unique<Audio>();
		(*slot)->session = next_audio_session_++;
		if (next_audio_session_ == 0)
			next_audio_session_ = 1;
		(*slot)->mp3_capacity = mp3_capacity;
		(*slot)->pcm_capacity = pcm_capacity;
		(*slot)->sample_format = sample_format;
		(*slot)->pcm_ring = pcm->card_only ? pcm->card_only->data() :
			memory_at(pcm->board_offset);
		(*slot)->pcm_ring_owned = pcm->card_only;
		(*slot)->decoder = decoder;
		audio_result(**slot, reply, reply_length);
		return ok;
	}
#endif
	if (opcode == 0x0516) {
		if (request_length < 48)
			return bad_request;
		const uint32_t codec = be32(request);
		if (!codec || codec > 3)
			return bad_request;
		if (codec == 1) {
			uint8_t legacy[48] = {};
			std::memcpy(legacy, request + 4, 36);
			return dispatch(0x0503, legacy, 40, reply, reply_length);
		}
#ifndef AMIBERRY_WITH_FFMPEG
		return unsupported;
#else
		const auto *input = find_buffer(be32(request + 4));
		const auto *pcm = find_buffer(be32(request + 12));
		if (!input || !pcm || input == pcm)
			return bad_handle;
		const uint32_t input_capacity = be32(request + 8);
		const uint32_t pcm_capacity = be32(request + 16);
		const uint32_t format = be32(request + 28);
		if (be32(request + 20) || be32(request + 24) || be32(request + 40))
			return unsupported;
		if ((codec == 2 && format != pcm_s16le && format != pcm_s16be &&
		     format != pcm_s32be) ||
		    (codec == 3 && format != pcm_s16le && format != pcm_s16be)) {
			if (format == 3 || format == 4)
				return unsupported;
			return bad_request;
		}
		if (!input_capacity || input_capacity > input->length || pcm_capacity < 8192 ||
		    pcm_capacity > pcm->length || (pcm_capacity & 1) ||
		    be32(request + 32) >= pcm_capacity || be32(request + 36) >= pcm_capacity)
			return bad_request;
		// Four slots, as the card's open_audio_stream has; a full table
		// answers NO_MEMORY, never BUSY.
		auto slot = std::find_if(audios_.begin(), audios_.end(),
			[](const std::unique_ptr<Audio> &audio) { return !audio; });
		if (slot == audios_.end())
			return no_memory;
		*slot = std::make_unique<Audio>();
		(*slot)->session = next_audio_session_++;
		if (next_audio_session_ == 0)
			next_audio_session_ = 1;
		(*slot)->codec = codec;
		(*slot)->mp3_capacity = input_capacity;
		(*slot)->pcm_capacity = pcm_capacity;
		(*slot)->sample_format = format;
		(*slot)->pcm_ring = pcm->card_only ? pcm->card_only->data() :
			memory_at(pcm->board_offset);
		(*slot)->pcm_ring_owned = pcm->card_only;
		(*slot)->codec_decoder = std::make_unique<AudioCodecDecoder>(codec, format,
			(*slot)->pcm_capacity, input_capacity);
		audio_result(**slot, reply, reply_length);
		return ok;
#endif
	}
	if (opcode >= 0x0504 && opcode <= 0x0508) {
		const uint16_t needed = opcode == 0x0504 ? 20 :
			                         opcode == 0x0505 ? 12 : 8;
		if (request_length < needed)
			return bad_request;
		auto *audio = find_audio(be32(request));
		if (!audio)
			return bad_handle;
		if (opcode == 0x0504) {
			const uint32_t length = be32(request + 12);
			const uint32_t flags = be32(request + 16);
			if ((flags & ~3u) || flags == 3 || ((flags & 2) && length))
				return bad_request;
			if (audio->faulted)
				return audio->fault_status;
			if (length) {
				const auto *source = find_buffer(be32(request + 4));
				const uint32_t offset = be32(request + 8);
				if (!source || offset > source->length || length > source->length - offset)
					return bad_handle;
				// The card refuses a feed only when its input ring has no
				// room; it then answers OK with BACKPRESSURE, never an
				// error (audio_stream_feed_compute).
				const uint32_t retained = audio->codec_decoder ?
					static_cast<uint32_t>(audio->codec_decoder->input_size()) :
					audio->bytes_fed - audio->bytes_consumed;
				if (length > audio->mp3_capacity -
				    std::min(retained, audio->mp3_capacity)) {
					audio->backpressure = true;
					audio_result(*audio, reply, reply_length);
					return ok;
				}
				audio->backpressure = false;
				audio->drain = false;
				if (audio->codec_decoder)
					audio->codec_decoder->set_draining(false);
				audio->bytes_fed += length;
				if (flags & 1)
					audio->eof = true;
				decode_audio(*audio, buffer_data(source->handle) + offset, length);
			} else if (flags & 1) {
				audio->eof = true;
				decode_audio(*audio, nullptr, 0);
			}
			// EOF ends any resumable drain (sdk_mailbox.c:4042-4045).
			if (flags & 1)
				audio->drain = false;
			if (flags & 2) {
				audio->drain = true;
				// The card holds no decoded PCM back; let the decoder
				// publish what it kept for end-of-stream trimming.
				if (audio->codec_decoder) {
					audio->codec_decoder->set_draining(true);
					decode_audio(*audio, nullptr, 0);
				}
			}
			if (audio->codec_decoder && audio->faulted)
				return audio->fault_status;
		} else if (opcode == 0x0505) {
			const uint32_t count = be32(request + 4);
			if (be32(request + 8) || count > audio->pcm_written - audio->pcm_read)
				return bad_request;
			if (audio->faulted)
				return audio->fault_status;
			if (audio->playing)
				return bad_request;
			audio->pcm_read += count;
			decode_audio(*audio, nullptr, 0);
		} else {
			if (be32(request + 4))
				return bad_request;
			if (opcode == 0x0507) {
#ifndef ZZ9000_SDK_AUDIO
				return unsupported;
#else
				if (audio->faulted) {
					// An envelope rejection is the client's AHI fallback
					// signal; corruption stays IO_ERROR.
					if ((audio->codec == 2 || audio->codec == 3) &&
					    audio->fault_status == unsupported)
						return unsupported;
					return io_error;
				}
				// Already bound to this session: idempotent. Another
				// session holds the single playback binding: BUSY, as the
				// card's pump answers before any admission check
				// (sdk_mailbox.c:4614).
				if (audio_playing_session_ && audio_playing_session_ != audio->session)
					return busy;
				if (!audio->sample_rate)
					return bad_request;
				if (audio->sample_format != pcm_s16le)
					return unsupported;
				if (audio->codec == 2 || audio->codec == 3) {
					if (audio->channels != 1 && audio->channels != 2)
						return unsupported;
					const uint32_t r = audio->sample_rate;
					if (r != 8000 && r != 11025 && r != 12000 && r != 16000 && r != 22050 &&
					    r != 24000 && r != 32000 && r != 44100 && r != 48000 && r != 88200 && r != 96000)
						return unsupported;
				}
				if (!audio->output) {
					SDL_AudioSpec spec = {
						SDL_AUDIO_S16LE,
						static_cast<int>(audio->channels ? audio->channels : 2),
						static_cast<int>(audio->sample_rate ? audio->sample_rate : 48000)
					};
					const int selected = currprefs.soundcard;
					const bool use_default = currprefs.soundcard_default || selected < 0 ||
					    selected >= MAX_SOUND_DEVICES || !sound_devices[selected];
					const auto device = use_default ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK :
					    static_cast<SDL_AudioDeviceID>(sound_devices[selected]->id);
					audio->output = SDL_OpenAudioDeviceStream(device, &spec, nullptr, nullptr);
					if (!audio->output && !use_default)
						audio->output = SDL_OpenAudioDeviceStream(
							SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
					if (!audio->output)
						return io_error;
					// A new stream starts at unity gain; force pump_audio()
					// to apply the session's gain to it.
					audio->gain = -1.0f;
					audio->paused = sound_paused();
					if (!audio->paused)
						SDL_ResumeAudioStreamDevice(audio->output);
				}
				audio->playing = true;
				audio_playing_session_ = audio->session;
#endif
			} else if (opcode == 0x0508) {
				audio->playing = false;
				if (audio_playing_session_ == audio->session)
					audio_playing_session_ = 0;
#ifdef ZZ9000_SDK_AUDIO
				if (audio->output) {
					SDL_DestroyAudioStream(audio->output);
					audio->output = nullptr;
				}
#endif
			} else {
				// CLOSE always succeeds, even for a faulted or
				// backpressured session, and a bound session stops first.
				audio_result(*audio, reply, reply_length);
				close_audio(audio);
				return ok;
			}
		}
		pump_audio();
		audio_result(*audio, reply, reply_length);
		return ok;
	}
	if (opcode == 0x0515) {
		if (request_length < 48)
			return bad_request;
		const uint32_t gain = be32(request + 4);
		if (be32(request + 8) || gain > 128)
			return bad_request;
		auto *audio = find_audio(be32(request));
		if (!audio)
			return bad_request;
		audio->stream_gain = gain;
		audio_result(*audio, reply, reply_length);
		return ok;
	}
#endif
	return unsupported;
}

bool Engine::poll()
{
	bool dirty = false;
	pump_audio();
	auto *mb = memory_at(mailbox_offset);
	uint32_t head = be32(mb + 20);
	const uint32_t tail = be32(mb + 24);
	uint32_t completion_tail = be32(mb + 40);
	const uint32_t completion_head = be32(mb + 36);
	if (head >= ring_entries || tail >= ring_entries ||
		completion_tail >= ring_entries || completion_head >= ring_entries) {
		last_status_ = bad_request;
		return false;
	}
	while (head != tail) {
		const uint32_t next_completion = (completion_tail + 1) % ring_entries;
		if (next_completion == completion_head) {
			last_status_ = busy;
			break;
		}
		const uint8_t *request = mb + request_offset + head * entry_size;
		uint8_t *reply = mb + completion_offset + completion_tail * entry_size;
		uint8_t payload[48] = {};
		uint16_t payload_length = 0;
		const uint16_t opcode = be16(request + 4);
		last_status_ = dispatch(opcode, request + 16, be16(request + 10),
			payload, &payload_length);
		const bool image_framebuffer_write = opcode == 0x0405 && image_ &&
			image_->output_mode != 3 &&
			image_->dst_surface == framebuffer_handle && payload_length >= 44 &&
			be32(payload + 4) == 4 && be32(payload + 40) != 0;
		bool surface_framebuffer_write = false;
		if (last_status_ == ok && opcode == 0x0203)
			surface_framebuffer_write = be32(request + 16) == framebuffer_handle;
		if (last_status_ == ok && opcode == 0x0407 &&
		    be32(request + 20) == framebuffer_handle) {
			const auto *args = request + 16;
			const uint32_t dx = be16(args + 16), dy = be16(args + 18);
			const uint32_t dw = be16(args + 20), dh = be16(args + 22);
			const uint32_t cx = be16(args + 24), cy = be16(args + 26);
			const uint32_t cw = be16(args + 28), ch = be16(args + 30);
			surface_framebuffer_write =
				std::max(dx, cx) < std::min(dx + dw, cx + cw) &&
				std::max(dy, cy) < std::min(dy + dh, cy + ch);
		}
		const bool image_overlay_write = opcode == 0x0409 && overlay_.active;
		if (last_status_ == ok && (surface_framebuffer_write ||
		    image_framebuffer_write || image_overlay_write || opcode == 0x0b08))
			dirty = true;
		std::memset(reply, 0, entry_size);
		std::memcpy(reply, request, 4); // request id
		put16(reply + 4, opcode);
		put16(reply + 6, last_status_);
		put16(reply + 8, be16(request + 8));
		put16(reply + 10, payload_length);
		std::memcpy(reply + 12, request + 12, 4); // cookie
		std::memcpy(reply + 16, payload, payload_length);
		head = (head + 1) % ring_entries;
		completion_tail = next_completion;
		put32(mb + 20, head);
		put32(mb + 40, completion_tail);
	}
	return dirty;
}

} // namespace zz9000_sdk
