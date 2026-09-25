/*
 * ZZ9000 SDK v2 mailbox bootstrap and bounded shared-buffer service.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "sysconfig.h"
#include "sysdeps.h"
#include "zz9000_sdk.h"

#include <algorithm>
#include <chrono>
#include <cfenv>
#include <cstring>
#include <limits>

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
constexpr uint32_t base_capabilities = (1u << 0) | (1u << 2) | (1u << 13) | (1u << 14);
#ifdef ZZ9000_SDK_AUDIO
constexpr uint32_t capabilities = base_capabilities | (1u << 19) | (1u << 23);
#else
constexpr uint32_t capabilities = base_capabilities;
#endif
constexpr uint16_t ok = 0;
constexpr uint16_t busy = 2;
constexpr uint16_t unsupported = 3;
constexpr uint16_t bad_request = 4;
constexpr uint16_t bad_handle = 5;
constexpr uint16_t no_memory = 6;
constexpr uint16_t io_error = 9;
constexpr uint16_t not_found = 10;
constexpr uint32_t pcm_s16le = 1;
constexpr uint32_t pcm_s16be = 2;
constexpr uint32_t host_window_begin = 0x3e0000;
constexpr uint32_t host_window_end = 0x3f0000;
constexpr uint32_t z3_heap_begin = 0x06000000;
constexpr uint32_t z3_heap_end = 0x07e00000;

#ifdef ZZ9000_SDK_AUDIO
// Guest FPCR can change host rounding; mpg123 expects nearest during setup
// and decode. Restore the guest mode before returning to CPU emulation.
class HostNearestRounding {
public:
	HostNearestRounding() : previous_(std::fegetround())
	{
		std::fesetround(FE_TONEAREST);
	}

	~HostNearestRounding()
	{
		if (previous_ >= 0)
			std::fesetround(previous_);
	}

private:
	int previous_;
};
#endif

uint16_t be16(const uint8_t *p)
{
	return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

uint32_t be32(const uint8_t *p)
{
	return (static_cast<uint32_t>(be16(p)) << 16) | be16(p + 2);
}

void put16(uint8_t *p, uint16_t value)
{
	p[0] = static_cast<uint8_t>(value >> 8);
	p[1] = static_cast<uint8_t>(value);
}

void put32(uint8_t *p, uint32_t value)
{
	put16(p, static_cast<uint16_t>(value >> 16));
	put16(p + 2, static_cast<uint16_t>(value));
}

uint32_t align_up(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

struct Engine::Audio {
	uint32_t session = 0;
	uint32_t mp3_handle = 0;
	uint32_t pcm_handle = 0;
	uint32_t mp3_capacity = 0;
	uint32_t pcm_capacity = 0;
	uint32_t pcm_written = 0;
	uint32_t pcm_read = 0;
	uint32_t bytes_consumed = 0;
	uint32_t bytes_produced = 0;
	uint32_t frames_decoded = 0;
	uint32_t sample_rate = 0;
	uint32_t channels = 0;
	uint32_t sample_format = pcm_s16le;
	uint32_t samples_per_frame = 1152;
	bool playing = false;
	bool eof = false;
	bool drain = false;
	bool decoder_pending = false;
	bool backpressure = false;
	bool faulted = false;
	bool paused = false;
	std::chrono::steady_clock::time_point next_pump{};
	float gain = -1.0f;
#ifdef ZZ9000_SDK_AUDIO
	mpg123_handle *decoder = nullptr;
	SDL_AudioStream *output = nullptr;
#endif
};

Engine::Engine(uint8_t *board_memory, uint32_t board_size)
	: memory_(board_memory), board_size_(board_size)
{
	reset();
}

Engine::~Engine()
{
	close_audio();
}

void Engine::reset()
{
	close_audio();
	for (auto &buffer : buffers_)
		buffer = {};
	next_handle_ = 1;
	next_audio_session_ = 1;
	last_status_ = ok;
	auto *mb = memory_ + mailbox_offset;
	std::memset(mb, 0, mailbox_size);
	put32(mb + 0, abi_magic);
	put16(mb + 4, 2);
	put16(mb + 6, 0);
	put32(mb + 8, 128);
	put32(mb + 12, request_offset);
	put32(mb + 16, ring_entries);
	put32(mb + 28, completion_offset);
	put32(mb + 32, ring_entries);
	put32(mb + 44, capabilities);
}

uint16_t Engine::read_register(uint32_t offset) const
{
	switch (offset) {
		case 0x100: return 0x5a39;
		case 0x102: return 0x0200;
		case 0x104: return mailbox_arm_address >> 16;
		case 0x106: return mailbox_arm_address & 0xffff;
		case 0x10a: return last_status_;
		default: return 0;
	}
}

void Engine::write_register(uint32_t offset, uint16_t value)
{
	if (offset == 0x108 && value)
		poll();
	// Completion IRQ is deliberately unadvertised: SDK clients poll the ring.
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
	return buffer->card_only.empty()
		? memory_ + buffer->board_offset : buffer->card_only.data();
}

uint32_t Engine::buffer_length(uint32_t handle) const
{
	const auto *buffer = find_buffer(handle);
	return buffer ? buffer->length : 0;
}

void Engine::close_audio()
{
#ifdef ZZ9000_SDK_AUDIO
	if (audio_ && audio_->output)
		SDL_DestroyAudioStream(audio_->output);
	if (audio_ && audio_->decoder)
		mpg123_delete(audio_->decoder);
#endif
	audio_.reset();
}

void Engine::decode_audio(const uint8_t *input, uint32_t length)
{
#ifdef ZZ9000_SDK_AUDIO
	if (!audio_ || !audio_->decoder || audio_->faulted)
		return;
	HostNearestRounding host_rounding;
	uint8_t decoded[16384];
	const unsigned char *source = input;
	size_t source_length = length;
	for (;;) {
		const uint32_t used = audio_->pcm_written - audio_->pcm_read;
		const uint32_t free = audio_->pcm_capacity - used;
		if (free < 8192) {
			audio_->decoder_pending = true;
			break;
		}
		size_t done = 0;
		const size_t output_size = std::min<size_t>(free & ~3u, sizeof decoded);
		const int rc = mpg123_decode(audio_->decoder, source, source_length,
			decoded, output_size, &done);
		source = nullptr;
		source_length = 0;
		if (rc == MPG123_NEW_FORMAT) {
			long rate = 0;
			int channels = 0;
			int encoding = 0;
			if (mpg123_getformat(audio_->decoder, &rate, &channels, &encoding) != MPG123_OK ||
			    rate <= 0 || (channels != 1 && channels != 2) ||
			    encoding != MPG123_ENC_SIGNED_16 ||
			    (audio_->sample_rate &&
			     (audio_->sample_rate != static_cast<uint32_t>(rate) ||
			      audio_->channels != static_cast<uint32_t>(channels)))) {
				audio_->faulted = true;
				break;
			}
			audio_->sample_rate = static_cast<uint32_t>(rate);
			audio_->channels = static_cast<uint32_t>(channels);
			if (audio_->sample_format == pcm_s16be) {
				mpg123_frameinfo2 info{};
				if (mpg123_info2(audio_->decoder, &info) != MPG123_OK) {
					audio_->faulted = true;
					break;
				}
				audio_->samples_per_frame = info.layer == 1 ? 384 :
					(info.layer == 3 && info.version != MPG123_1_0 ? 576 : 1152);
			}
		}
		if (done) {
			if (!audio_->channels) {
				audio_->faulted = true;
				break;
			}
			if (audio_->sample_format == pcm_s16be)
				for (size_t i = 0; i + 1 < done; i += 2)
					std::swap(decoded[i], decoded[i + 1]);
			auto *ring = buffer_data(audio_->pcm_handle);
			if (!ring) {
				audio_->faulted = true;
				break;
			}
			const uint32_t pos = audio_->pcm_written % audio_->pcm_capacity;
			const uint32_t first = std::min<uint32_t>(done, audio_->pcm_capacity - pos);
			std::memcpy(ring + pos, decoded, first);
			if (done > first)
				std::memcpy(ring, decoded + first, done - first);
			audio_->pcm_written += static_cast<uint32_t>(done);
			audio_->bytes_produced += static_cast<uint32_t>(done);
			audio_->frames_decoded = audio_->bytes_produced /
				(audio_->channels * 2 * audio_->samples_per_frame);
		}
		if (rc == MPG123_NEW_FORMAT)
			continue;
		if (rc == MPG123_NEED_MORE || (rc == MPG123_OK && !done)) {
			audio_->decoder_pending = false;
			break;
		}
		if (rc == MPG123_OK)
			continue;
		if (rc == MPG123_DONE) {
			audio_->decoder_pending = false;
			break;
		}
		audio_->faulted = true;
		break;
	}
#else
	(void)input;
	(void)length;
#endif
}

void Engine::pump_audio()
{
#ifdef ZZ9000_SDK_AUDIO
	if (!audio_)
		return;
	const auto now = std::chrono::steady_clock::now();
	if (now < audio_->next_pump)
		return;
	audio_->next_pump = now + std::chrono::milliseconds(10);
	if (audio_->output) {
		const bool paused = sound_paused();
		if (paused != audio_->paused) {
			SDL_ClearAudioStream(audio_->output);
			if (paused)
				SDL_PauseAudioStreamDevice(audio_->output);
			else
				SDL_ResumeAudioStreamDevice(audio_->output);
			audio_->paused = paused;
		}
		const float master = (100 - std::clamp(currprefs.sound_volume_master, 0, 100)) / 100.0f;
		const float board = (100 - std::clamp(currprefs.sound_volume_board, 0, 100)) / 100.0f;
		const float gain = sound_muted() ? 0.0f : master * board;
		if (gain != audio_->gain && SDL_SetAudioStreamGain(audio_->output, gain))
			audio_->gain = gain;
	}
	if (audio_->decoder_pending &&
	    audio_->pcm_capacity - (audio_->pcm_written - audio_->pcm_read) >= 8192)
		decode_audio(nullptr, 0);
	if (!audio_->playing || !audio_->output)
		return;
	const int queued = SDL_GetAudioStreamQueued(audio_->output);
	if (queued < 0) {
		audio_->faulted = true;
		return;
	}
	if (queued >= 48000 / 2 * 4)
		return;
	uint32_t available = audio_->pcm_written - audio_->pcm_read;
	available = std::min<uint32_t>(available, 16384);
	available = std::min<uint32_t>(available, 48000 / 2 * 4 - queued);
	if (!available)
		return;
	const uint32_t pos = audio_->pcm_read % audio_->pcm_capacity;
	const uint32_t first = std::min(available, audio_->pcm_capacity - pos);
	const auto *ring = buffer_data(audio_->pcm_handle);
	if (!ring || !SDL_PutAudioStreamData(audio_->output, ring + pos, first)) {
		audio_->faulted = true;
		return;
	}
	audio_->pcm_read += first;
	if (available > first) {
		if (!SDL_PutAudioStreamData(audio_->output, ring, available - first))
			audio_->faulted = true;
		else
			audio_->pcm_read += available - first;
	}
#endif
}

void Engine::audio_result(uint8_t *reply, uint16_t *reply_length)
{
	if (!audio_)
		return;
	const uint32_t used = audio_->pcm_written - audio_->pcm_read;
	bool output_drained = true;
#ifdef ZZ9000_SDK_AUDIO
	if (audio_->output)
		output_drained = SDL_GetAudioStreamQueued(audio_->output) <= 0;
#endif
	const bool drained = !used && !audio_->decoder_pending && output_drained;
	const bool done = audio_->eof && drained;
	const uint32_t state = audio_->faulted ? 4 : done ? 3 : used ? 2 : 1;
	uint32_t flags = ((used || !output_drained) ? 2u : 0u) | (done ? 4u : 0u) |
		(audio_->backpressure ? 8u : 0u) | (audio_->drain && drained ? 16u : 0u);
	if (!used && !audio_->eof && !audio_->decoder_pending)
		flags |= 1u;
	put32(reply + 0, audio_->session);
	put32(reply + 4, state);
	put32(reply + 8, audio_->sample_rate);
	put32(reply + 12, audio_->channels);
	put32(reply + 16, audio_->sample_format);
	put32(reply + 20, audio_->bytes_consumed % audio_->mp3_capacity);
	put32(reply + 24, audio_->pcm_written % audio_->pcm_capacity);
	put32(reply + 28, audio_->pcm_read % audio_->pcm_capacity);
	put32(reply + 32, audio_->frames_decoded);
	put32(reply + 36, audio_->bytes_consumed);
	put32(reply + 40, audio_->bytes_produced);
	put32(reply + 44, flags);
	*reply_length = 48;
}

uint16_t Engine::dispatch(uint16_t opcode, const uint8_t *request,
	                       uint16_t request_length, uint8_t *reply,
	                       uint16_t *reply_length)
{
	*reply_length = 0;
	if (request_length > 48)
		return bad_request;
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
		put32(reply + 8, capabilities);
		put32(reply + 12, 48);
		put32(reply + 16, buffers_.size());
		put32(reply + 20, 0);
		put32(reply + 24, 0);
		put32(reply + 28, ring_entries);
		put32(reply + 32, ring_entries);
		put32(reply + 36, board_size_ == 0x00400000 ?
			 host_window_end - host_window_begin : 0);
		*reply_length = 48;
		return ok;
	}
	if (opcode == 0x0004) {
		if (request_length < 4)
			return bad_request;
		const uint32_t service = be32(request);
		if (service != 0 && service != 0x100
#ifdef ZZ9000_SDK_AUDIO
		    && service != 0x500
#endif
		)
			return not_found;
		put32(reply + 0, service);
		put32(reply + 4, 0x00020000);
		put32(reply + 8, service == 0 ? capabilities & ~(1u << 2) :
			service == 0x100 ? (1u << 2) : capabilities & ((1u << 19) | (1u << 23)));
		put32(reply + 12, service == 0x500 ?
			(1u | (1u << 16) | (1u << 18) | (1u << 19) | (1u << 20)) : 1u);
		put32(reply + 16, service);
		put32(reply + 20, service == 0 ? 4 : service == 0x100 ? 2 : 6);
		put32(reply + 24, 48);
		const char *name = service == 0 ? "core" : service == 0x100 ? "memory" : "audio";
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
		if (alignment == 0)
			alignment = 16;
		if (!length || length > 4 * 1024 * 1024 || alignment > 4096 ||
			(alignment & (alignment - 1)) || (flags & ~3u))
			return bad_request;
		auto free_slot = std::find_if(buffers_.begin(), buffers_.end(),
			[](const Buffer &buffer) { return buffer.handle == 0; });
		if (free_slot == buffers_.end())
			return no_memory;
		const bool z2 = board_size_ == 0x00400000;
		const bool card_only = z2 && (flags & 2);
		const uint32_t start = z2 && !card_only ? host_window_begin : z3_heap_begin;
		const uint32_t end = z2 && !card_only ? host_window_end : z3_heap_end;
		uint32_t board_offset = 0;
		if (!card_only) {
			uint32_t candidate = align_up(start, alignment);
			while (candidate <= end && length <= end - candidate) {
				bool overlap = false;
				for (const auto &buffer : buffers_) {
					if (buffer.handle && buffer.card_only.empty() &&
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
		if (card_only)
			free_slot->card_only.resize(length);
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
		if (audio_ && (audio_->mp3_handle == buffer->handle ||
		              audio_->pcm_handle == buffer->handle))
			return busy;
		*buffer = {};
		return ok;
	}
#ifdef ZZ9000_SDK_AUDIO
	if (opcode == 0x0503) {
		if (request_length < 40)
			return bad_request;
		if (audio_)
			return busy;
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
		HostNearestRounding host_rounding;
		static const bool mpg123_ready = mpg123_init() == MPG123_OK;
		if (!mpg123_ready)
			return io_error;
		int decoder_error = MPG123_OK;
		auto *decoder = mpg123_new(nullptr, &decoder_error);
		if (!decoder)
			return no_memory;
		const uint32_t sample_format = be32(request + 24);
		bool configured = mpg123_format_none(decoder) == MPG123_OK;
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
		audio_ = std::make_unique<Audio>();
		audio_->session = next_audio_session_++;
		audio_->mp3_handle = mp3->handle;
		audio_->pcm_handle = pcm->handle;
		audio_->mp3_capacity = mp3_capacity;
		audio_->pcm_capacity = pcm_capacity;
		audio_->sample_format = sample_format;
		audio_->decoder = decoder;
		audio_result(reply, reply_length);
		return ok;
	}
	if (opcode >= 0x0504 && opcode <= 0x0508) {
		const uint16_t needed = opcode == 0x0504 ? 20 :
			                         opcode == 0x0505 ? 12 : 8;
		if (request_length < needed)
			return bad_request;
		if (!audio_ || be32(request) != audio_->session)
			return bad_handle;
		if (opcode == 0x0504) {
			const uint32_t length = be32(request + 12);
			const uint32_t flags = be32(request + 16);
			if ((flags & ~3u) || flags == 3 || ((flags & 2) && length))
				return bad_request;
			if (audio_->faulted)
				return io_error;
			if (length) {
				const auto *source = find_buffer(be32(request + 4));
				const uint32_t offset = be32(request + 8);
				if (!source || offset > source->length || length > source->length - offset)
					return bad_handle;
				if (length > audio_->mp3_capacity || audio_->decoder_pending ||
				    audio_->pcm_written - audio_->pcm_read >
				        audio_->pcm_capacity - 8192) {
					audio_->backpressure = true;
					audio_result(reply, reply_length);
					return ok;
				}
				audio_->backpressure = false;
				audio_->drain = false;
				audio_->bytes_consumed += length;
				decode_audio(buffer_data(source->handle) + offset, length);
			}
			if (flags & 1)
				audio_->eof = true;
			if (flags & 2)
				audio_->drain = true;
		} else if (opcode == 0x0505) {
			const uint32_t count = be32(request + 4);
			if (be32(request + 8) || audio_->playing ||
			    count > audio_->pcm_written - audio_->pcm_read)
				return bad_request;
			audio_->pcm_read += count;
			decode_audio(nullptr, 0);
		} else {
			if (be32(request + 4))
				return bad_request;
			if (opcode == 0x0507) {
				if (audio_->faulted)
					return io_error;
				if (!audio_->sample_rate)
					return bad_request;
				if (audio_->sample_format != pcm_s16le)
					return unsupported;
				if (!audio_->output) {
					SDL_AudioSpec spec = { SDL_AUDIO_S16LE, 2, 48000 };
					const int selected = currprefs.soundcard;
					const bool use_default = currprefs.soundcard_default || selected < 0 ||
					    selected >= MAX_SOUND_DEVICES || !sound_devices[selected];
					const auto device = use_default ? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK :
					    static_cast<SDL_AudioDeviceID>(sound_devices[selected]->id);
					audio_->output = SDL_OpenAudioDeviceStream(device, &spec, nullptr, nullptr);
					if (!audio_->output && !use_default)
						audio_->output = SDL_OpenAudioDeviceStream(
							SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
					if (!audio_->output)
						return io_error;
					audio_->paused = sound_paused();
					if (!audio_->paused)
						SDL_ResumeAudioStreamDevice(audio_->output);
				}
				audio_->playing = true;
			} else if (opcode == 0x0508) {
				audio_->playing = false;
				if (audio_->output) {
					SDL_DestroyAudioStream(audio_->output);
					audio_->output = nullptr;
				}
			} else {
				audio_result(reply, reply_length);
				close_audio();
				return ok;
			}
		}
		pump_audio();
		audio_result(reply, reply_length);
		return ok;
	}
#endif
	return unsupported;
}

void Engine::poll()
{
	pump_audio();
	auto *mb = memory_ + mailbox_offset;
	uint32_t head = be32(mb + 20);
	const uint32_t tail = be32(mb + 24);
	uint32_t completion_tail = be32(mb + 40);
	const uint32_t completion_head = be32(mb + 36);
	if (head >= ring_entries || tail >= ring_entries ||
		completion_tail >= ring_entries || completion_head >= ring_entries) {
		last_status_ = bad_request;
		return;
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
}

} // namespace zz9000_sdk
