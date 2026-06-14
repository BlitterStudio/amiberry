/*
 * SPDX-FileCopyrightText: 2026 Dimitris Panokostas
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <utility>
#include <vector>

#if defined(HAVE_MPG123)
#include <mpg123.h>
#endif

#include "audio.h"
#include "gensound.h"
#include "mhi_host.h"
#include "native2amiga_api.h"
#include "options.h"
#include "threaddep/thread.h"
#include "traps.h"
#include "uae.h"

namespace {

constexpr uae_u32 MHI_HOST_HANDLE = 1;
constexpr uae_u32 MHI_HOST_MAX_QUEUE = 512 * 1024;
constexpr uae_u32 MHI_HOST_DECODE_CHUNK = 16 * 1024;
constexpr uae_u32 MHI_HOST_DECODE_OUT = 32 * 1024;
constexpr int MHI_HOST_MIN_RATE = 8000;
constexpr int MHI_HOST_DEFAULT_RATE = 44100;

constexpr uae_u32 MHIF_PLAYING = 0;
constexpr uae_u32 MHIF_STOPPED = 1;
constexpr uae_u32 MHIF_OUT_OF_DATA = 2;
constexpr uae_u32 MHIF_PAUSED = 3;

constexpr uae_u32 UAE_MHI_PARAM_VOLUME = 0;
constexpr uae_u32 UAE_MHI_PARAM_PANNING = 1;

struct MhiEncodedBuffer {
	std::vector<uae_u8> data;
	size_t offset = 0;
	uae_u32 token = 0;
};

struct MhiHostSession {
	uae_u32 handle = 0;
	uaecptr task = 0;
	uae_u32 sigmask = 0;
	uae_sem_t lock = 0;
	uae_sem_t decoder_lock = 0;
	uae_thread_id worker = 0;
	bool worker_running = false;
	bool playing = false;
	bool paused = false;
	bool out_of_data = false;
	bool decoder_error = false;
	bool decoder_needs_drain = false;
	int streamid = 0;
	int output_rate = MHI_HOST_DEFAULT_RATE;
	int volume = 100;
	int panning = 50;
	size_t encoded_bytes = 0;
	std::deque<MhiEncodedBuffer> encoded;
	std::deque<uae_u32> completed_tokens;
	std::vector<uae_s16> pcm;
	size_t pcm_read = 0;
	size_t pcm_write = 0;
	size_t pcm_used = 0;
#if defined(HAVE_MPG123)
	mpg123_handle *mpg = nullptr;
#endif
};

MhiHostSession *active_session;

static void lock_session(MhiHostSession *session)
{
	uae_sem_wait(&session->lock);
}

static void unlock_session(MhiHostSession *session)
{
	uae_sem_post(&session->lock);
}

static void lock_decoder(MhiHostSession *session)
{
	uae_sem_wait(&session->decoder_lock);
}

static void unlock_decoder(MhiHostSession *session)
{
	uae_sem_post(&session->decoder_lock);
}

static MhiHostSession *find_session(uae_u32 handle)
{
	if (handle != MHI_HOST_HANDLE)
		return nullptr;
	return active_session;
}

static size_t min_free_decode_frames()
{
	return MHI_HOST_DECODE_OUT / (sizeof(uae_s16) * 2);
}

static size_t pcm_capacity_frames(const MhiHostSession *session)
{
	return session->pcm.size() / 2;
}

static size_t pcm_free_frames(const MhiHostSession *session)
{
	return pcm_capacity_frames(session) - session->pcm_used;
}

static bool push_pcm_frames(MhiHostSession *session, const uae_s16 *samples, size_t frames)
{
	if (frames == 0)
		return true;
	if (frames > pcm_free_frames(session))
		return false;
	for (size_t i = 0; i < frames; i++) {
		const size_t dst = session->pcm_write * 2;
		session->pcm[dst] = samples[i * 2];
		session->pcm[dst + 1] = samples[i * 2 + 1];
		session->pcm_write++;
		if (session->pcm_write >= pcm_capacity_frames(session))
			session->pcm_write = 0;
	}
	session->pcm_used += frames;
	return true;
}

static bool complete_token_locked(MhiHostSession *session, uae_u32 token)
{
	if (token != 0)
		session->completed_tokens.push_back(token);
	return token != 0 && session->task != 0 && session->sigmask != 0;
}

static void signal_if_needed(MhiHostSession *session, bool signal)
{
	if (signal)
		uae_Signal(session->task, session->sigmask);
}

static bool pop_pcm_frame(MhiHostSession *session, int *samples)
{
	bool ok = false;
	int volume = 100;
	int panning = 50;

	lock_session(session);
	if (!session->paused && session->pcm_used > 0) {
		const size_t src = session->pcm_read * 2;
		samples[0] = session->pcm[src];
		samples[1] = session->pcm[src + 1];
		session->pcm_read++;
		if (session->pcm_read >= pcm_capacity_frames(session))
			session->pcm_read = 0;
		session->pcm_used--;
		ok = true;
	} else if (session->playing && session->encoded_bytes == 0 && !session->decoder_needs_drain && !session->decoder_error) {
		session->out_of_data = true;
	}
	volume = session->volume;
	panning = session->panning;
	unlock_session(session);

	if (ok) {
		const int left = std::clamp(volume * (100 - std::max(0, panning - 50)) / 100, 0, 100);
		const int right = std::clamp(volume * (100 - std::max(0, 50 - panning)) / 100, 0, 100);
		samples[0] = samples[0] * left / 100;
		samples[1] = samples[1] * right / 100;
	}
	return ok;
}

static bool mhi_audio_state(int streamid, void *opaque)
{
	auto *session = static_cast<MhiHostSession*>(opaque);
	int samples[2] = { 0, 0 };
	pop_pcm_frame(session, samples);
	const uae_u32 evt = (uae_u32)std::max(1.0f, std::floor(scaled_sample_evtime + 0.5f));
	audio_state_stream_state(streamid, samples, 2, evt);

	lock_session(session);
	const bool active = session->playing || session->paused;
	unlock_session(session);
	return active;
}

static void clear_queues_locked(MhiHostSession *session)
{
	session->encoded.clear();
	session->encoded_bytes = 0;
	session->completed_tokens.clear();
	session->pcm_read = 0;
	session->pcm_write = 0;
	session->pcm_used = 0;
	session->out_of_data = false;
	session->decoder_error = false;
	session->decoder_needs_drain = false;
}

static bool copy_encoded_chunk(MhiHostSession *session, std::vector<uae_u8> &chunk)
{
	bool signal = false;

	lock_session(session);
	const size_t count = std::min<size_t>(MHI_HOST_DECODE_CHUNK, session->encoded_bytes);
	if (count > 0 && pcm_free_frames(session) >= min_free_decode_frames()) {
		chunk.clear();
		chunk.reserve(count);
		while (chunk.size() < count && !session->encoded.empty()) {
			MhiEncodedBuffer &front = session->encoded.front();
			const size_t available = front.data.size() - front.offset;
			const size_t wanted = std::min(count - chunk.size(), available);
			chunk.insert(chunk.end(), front.data.begin() + front.offset, front.data.begin() + front.offset + wanted);
			front.offset += wanted;
			session->encoded_bytes -= wanted;
			if (front.offset == front.data.size()) {
				signal = complete_token_locked(session, front.token) || signal;
				session->encoded.pop_front();
			}
		}
	} else {
		chunk.clear();
	}
	unlock_session(session);
	signal_if_needed(session, signal);
	return !chunk.empty();
}

#if defined(HAVE_MPG123)
static bool decode_chunk(MhiHostSession *session, const std::vector<uae_u8> &chunk)
{
	uae_u8 decoded[MHI_HOST_DECODE_OUT];
	const unsigned char *input = chunk.empty() ? nullptr : chunk.data();
	size_t input_size = chunk.size();
	bool result = true;
	bool needs_drain = true;

	lock_decoder(session);
	for (;;) {
		size_t done = 0;
		const int ret = mpg123_decode(session->mpg, input, input_size,
			decoded, sizeof(decoded), &done);
		input = nullptr;
		input_size = 0;

		if (done > 0) {
			const size_t frames = done / (sizeof(uae_s16) * 2);
			lock_session(session);
			const bool pushed = push_pcm_frames(session, reinterpret_cast<uae_s16*>(decoded), frames);
			if (pushed)
				session->out_of_data = false;
			unlock_session(session);
			if (!pushed) {
				result = false;
				needs_drain = true;
				break;
			}
		}

		if (ret == MPG123_NEW_FORMAT)
			continue;
		if (ret == MPG123_NEED_MORE) {
			needs_drain = false;
			break;
		}
		if (ret == MPG123_OK) {
			if (done > 0)
				continue;
			needs_drain = false;
			break;
		}
		if (ret == MPG123_ERR) {
			write_log(_T("MHI: mpg123 decode failed: %s\n"), mpg123_strerror(session->mpg));
			lock_session(session);
			session->decoder_error = true;
			session->out_of_data = true;
			unlock_session(session);
			result = false;
			needs_drain = false;
			break;
		}
		needs_drain = false;
		break;
	}
	lock_session(session);
	session->decoder_needs_drain = needs_drain;
	unlock_session(session);
	unlock_decoder(session);
	return result;
}
#endif

static int mhi_decode_worker(void *opaque)
{
	auto *session = static_cast<MhiHostSession*>(opaque);
	std::vector<uae_u8> chunk;

	for (;;) {
		lock_session(session);
		const bool running = session->worker_running;
		const bool paused = session->paused || !session->playing;
		const bool drain = session->decoder_needs_drain && pcm_free_frames(session) >= min_free_decode_frames();
		unlock_session(session);
		if (!running)
			break;
		if (paused) {
			sleep_millis(2);
			continue;
		}
#if defined(HAVE_MPG123)
		if (drain) {
			chunk.clear();
		} else if (!copy_encoded_chunk(session, chunk)) {
			sleep_millis(2);
			continue;
		}
		decode_chunk(session, chunk);
#else
		if (!copy_encoded_chunk(session, chunk)) {
			sleep_millis(2);
			continue;
		}
		sleep_millis(10);
#endif
	}
	return 0;
}

static void stop_stream(MhiHostSession *session)
{
	if (session->streamid > 0) {
		audio_enable_stream(false, session->streamid, 0, nullptr, nullptr);
		session->streamid = 0;
	}
}

static void destroy_session(MhiHostSession *session)
{
	if (!session)
		return;

	lock_session(session);
	session->worker_running = false;
	session->playing = false;
	session->paused = false;
	unlock_session(session);
	if (session->worker)
		uae_wait_thread(&session->worker);
	stop_stream(session);
#if defined(HAVE_MPG123)
	if (session->mpg) {
		lock_decoder(session);
		mpg123_close(session->mpg);
		unlock_decoder(session);
		mpg123_delete(session->mpg);
		session->mpg = nullptr;
	}
	mpg123_exit();
#endif
	uae_sem_destroy(&session->decoder_lock);
	uae_sem_destroy(&session->lock);
	delete session;
}

static int clamp_percent(uae_u32 value)
{
	return (int)std::clamp<uae_u32>(value, 0, 100);
}

} // namespace

uae_u32 mhi_host_alloc(TrapContext*, uaecptr task, uae_u32 sigmask)
{
#if !defined(HAVE_MPG123)
	write_log(_T("MHI: mpg123 support disabled at build time\n"));
	return 0;
#else
	if (active_session) {
		write_log(_T("MHI: decoder already allocated\n"));
		return 0;
	}

	auto *session = new MhiHostSession();
	session->handle = MHI_HOST_HANDLE;
	session->task = task;
	session->sigmask = sigmask;
	session->output_rate = currprefs.sound_freq >= MHI_HOST_MIN_RATE
		? currprefs.sound_freq : MHI_HOST_DEFAULT_RATE;
	session->pcm.resize(std::max((size_t)session->output_rate / 4, min_free_decode_frames()) * 2);

	if (uae_sem_init(&session->lock, 0, 1)) {
		delete session;
		return 0;
	}
	if (uae_sem_init(&session->decoder_lock, 0, 1)) {
		uae_sem_destroy(&session->lock);
		delete session;
		return 0;
	}
	if (mpg123_init() != MPG123_OK) {
		uae_sem_destroy(&session->decoder_lock);
		uae_sem_destroy(&session->lock);
		delete session;
		return 0;
	}
	session->mpg = mpg123_new(nullptr, nullptr);
	if (!session->mpg) {
		mpg123_exit();
		uae_sem_destroy(&session->decoder_lock);
		uae_sem_destroy(&session->lock);
		delete session;
		return 0;
	}
	mpg123_param(session->mpg, MPG123_FORCE_RATE, session->output_rate, 0.0);
	mpg123_format_none(session->mpg);
	mpg123_format(session->mpg, session->output_rate, MPG123_STEREO, MPG123_ENC_SIGNED_16);
	if (mpg123_open_feed(session->mpg) != MPG123_OK) {
		destroy_session(session);
		return 0;
	}

	session->worker_running = true;
	if (!uae_start_thread(_T("mhi-mp3"), mhi_decode_worker, session, &session->worker)) {
		destroy_session(session);
		return 0;
	}

	active_session = session;
	write_log(_T("MHI: allocated decoder handle %u at %d Hz\n"), session->handle, session->output_rate);
	return session->handle;
#endif
}

uae_u32 mhi_host_free(TrapContext*, uae_u32 handle)
{
	auto *session = find_session(handle);
	if (!session)
		return 0;
	active_session = nullptr;
	write_log(_T("MHI: free decoder handle %u\n"), handle);
	destroy_session(session);
	return 1;
}

uae_u32 mhi_host_queue(TrapContext *ctx, uae_u32 handle, uaecptr buffer, uae_u32 size, uae_u32 token)
{
	auto *session = find_session(handle);
	if (!session || !buffer || size == 0 || size > MHI_HOST_MAX_QUEUE)
		return 0;
	if (!trap_valid_address(ctx, buffer, size))
		return 0;

	std::vector<uae_u8> data(size);
	trap_get_bytes(ctx, data.data(), buffer, size);

	lock_session(session);
	if (session->encoded_bytes + data.size() > MHI_HOST_MAX_QUEUE) {
		unlock_session(session);
		return 0;
	}
	session->encoded.push_back({ std::move(data), 0, token });
	session->encoded_bytes += size;
	session->out_of_data = false;
	session->decoder_error = false;
	unlock_session(session);
	return 1;
}

uae_u32 mhi_host_get_empty(TrapContext*, uae_u32 handle)
{
	auto *session = find_session(handle);
	if (!session)
		return 0;
	lock_session(session);
	uae_u32 token = 0;
	if (!session->completed_tokens.empty()) {
		token = session->completed_tokens.front();
		session->completed_tokens.pop_front();
	}
	unlock_session(session);
	return token;
}

uae_u32 mhi_host_status(TrapContext*, uae_u32 handle)
{
	auto *session = find_session(handle);
	if (!session)
		return MHIF_STOPPED;
	lock_session(session);
	uae_u32 status = MHIF_STOPPED;
	if (session->paused)
		status = MHIF_PAUSED;
	else if (session->out_of_data || session->decoder_error)
		status = MHIF_OUT_OF_DATA;
	else if (session->playing)
		status = MHIF_PLAYING;
	unlock_session(session);
	return status;
}

uae_u32 mhi_host_play(TrapContext*, uae_u32 handle)
{
	auto *session = find_session(handle);
	if (!session)
		return 0;
	lock_session(session);
	if (session->streamid <= 0)
		session->streamid = audio_enable_stream(true, -1, 2, mhi_audio_state, session);
	session->playing = session->streamid > 0;
	session->paused = false;
	session->out_of_data = false;
	unlock_session(session);
	if (session->streamid > 0) {
		audio_activate();
		write_log(_T("MHI: play\n"));
		return 1;
	}
	return 0;
}

uae_u32 mhi_host_stop(TrapContext*, uae_u32 handle)
{
	auto *session = find_session(handle);
	if (!session)
		return 0;
	lock_session(session);
	session->playing = false;
	session->paused = false;
	unlock_session(session);
#if defined(HAVE_MPG123)
	lock_decoder(session);
	lock_session(session);
	clear_queues_locked(session);
	unlock_session(session);
	mpg123_close(session->mpg);
	mpg123_open_feed(session->mpg);
	unlock_decoder(session);
#else
	lock_session(session);
	clear_queues_locked(session);
	unlock_session(session);
#endif
	stop_stream(session);
	write_log(_T("MHI: stop\n"));
	return 1;
}

uae_u32 mhi_host_pause(TrapContext*, uae_u32 handle)
{
	auto *session = find_session(handle);
	if (!session)
		return 0;
	lock_session(session);
	if (session->playing)
		session->paused = true;
	unlock_session(session);
	write_log(_T("MHI: pause\n"));
	return 1;
}

uae_u32 mhi_host_set_param(TrapContext*, uae_u32 handle, uae_u32 param, uae_u32 value)
{
	auto *session = find_session(handle);
	if (!session)
		return 0;
	lock_session(session);
	switch (param) {
		case UAE_MHI_PARAM_VOLUME:
			session->volume = clamp_percent(value);
			break;
		case UAE_MHI_PARAM_PANNING:
			session->panning = clamp_percent(value);
			break;
		default:
			unlock_session(session);
			return 0;
	}
	unlock_session(session);
	return 1;
}

uae_u32 mhi_host_query(TrapContext*, uae_u32)
{
	return 0;
}
