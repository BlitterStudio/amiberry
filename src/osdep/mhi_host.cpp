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

constexpr uae_u32 UAE_MHI_QUERY_MPEG1 = 1;
constexpr uae_u32 UAE_MHI_QUERY_MPEG2 = 2;
constexpr uae_u32 UAE_MHI_QUERY_MPEG25 = 3;
constexpr uae_u32 UAE_MHI_QUERY_MPEG4 = 4;
constexpr uae_u32 UAE_MHI_QUERY_LAYER1 = 10;
constexpr uae_u32 UAE_MHI_QUERY_LAYER2 = 11;
constexpr uae_u32 UAE_MHI_QUERY_LAYER3 = 12;
constexpr uae_u32 UAE_MHI_QUERY_VARIABLE_BITRATE = 20;
constexpr uae_u32 UAE_MHI_QUERY_JOINT_STEREO = 21;
constexpr uae_u32 UAE_MHI_QUERY_VOLUME_CONTROL = 40;
constexpr uae_u32 UAE_MHI_QUERY_PANNING_CONTROL = 41;
constexpr uae_u32 UAE_MHI_QUERY_IS_HARDWARE = 1010;
constexpr uae_u32 UAE_MHI_QUERY_IS_68K = 1011;
constexpr uae_u32 UAE_MHI_QUERY_IS_PPC = 1012;

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
	bool decoder_busy = false;
	int streamid = 0;
	int output_rate = MHI_HOST_DEFAULT_RATE;
	int volume = 100;
	int panning = 50;
	size_t encoded_bytes = 0;
	std::deque<MhiEncodedBuffer> encoded;
	std::deque<uae_u32> completed_tokens;
	std::vector<uae_u32> pending_decode_tokens;
	std::vector<uae_s16> pcm;
	std::vector<uae_s16> pending_pcm;
	size_t pcm_read = 0;
	size_t pcm_write = 0;
	size_t pcm_used = 0;
	size_t pending_pcm_read = 0;
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

	const size_t capacity = pcm_capacity_frames(session);
	size_t written = 0;
	while (written < frames) {
		const size_t chunk = std::min(frames - written, capacity - session->pcm_write);
		std::copy_n(samples + written * 2, chunk * 2, session->pcm.data() + session->pcm_write * 2);
		session->pcm_write += chunk;
		if (session->pcm_write >= capacity)
			session->pcm_write = 0;
		written += chunk;
	}
	session->pcm_used += frames;
	return true;
}

static size_t push_pcm_frames_available(MhiHostSession *session, const uae_s16 *samples, size_t frames)
{
	const size_t pushed = std::min(frames, pcm_free_frames(session));
	if (pushed > 0)
		push_pcm_frames(session, samples, pushed);
	return pushed;
}

static bool flush_pending_pcm_locked(MhiHostSession *session)
{
	if (session->pending_pcm_read >= session->pending_pcm.size() / 2) {
		session->pending_pcm.clear();
		session->pending_pcm_read = 0;
		return true;
	}

	const auto *samples = session->pending_pcm.data() + session->pending_pcm_read * 2;
	const size_t frames = session->pending_pcm.size() / 2 - session->pending_pcm_read;
	const size_t pushed = push_pcm_frames_available(session, samples, frames);
	session->pending_pcm_read += pushed;
	if (pushed > 0)
		session->out_of_data = false;
	if (session->pending_pcm_read >= session->pending_pcm.size() / 2) {
		session->pending_pcm.clear();
		session->pending_pcm_read = 0;
		return true;
	}
	return false;
}

static bool has_pending_pcm(const MhiHostSession *session)
{
	return session->pending_pcm_read < session->pending_pcm.size() / 2;
}

static bool playback_drained_locked(const MhiHostSession *session)
{
	return session->encoded_bytes == 0
		&& session->pcm_used == 0
		&& !has_pending_pcm(session)
		&& session->pending_decode_tokens.empty()
		&& !session->decoder_needs_drain
		&& !session->decoder_busy;
}

static bool complete_pending_decode_tokens_locked(MhiHostSession *session)
{
	if (session->pending_decode_tokens.empty())
		return false;
	bool queued = false;
	for (const uae_u32 token : session->pending_decode_tokens) {
		if (token != 0) {
			session->completed_tokens.push_back(token);
			queued = true;
		}
	}
	session->pending_decode_tokens.clear();
	return queued && session->task != 0 && session->sigmask != 0;
}

static void signal_if_needed(MhiHostSession *session, bool signal)
{
	if (signal)
		uae_Signal(session->task, session->sigmask);
}

static bool pop_pcm_frame(MhiHostSession *session, int *samples)
{
	bool ok = false;
	bool signal = false;
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
	}
	if (session->playing && !session->paused && playback_drained_locked(session) && !session->decoder_error && !session->out_of_data) {
		session->out_of_data = true;
		signal = (session->task != 0 && session->sigmask != 0);
	}
	volume = session->volume;
	panning = session->panning;
	unlock_session(session);
	signal_if_needed(session, signal);

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
	session->pending_decode_tokens.clear();
	session->pcm_read = 0;
	session->pcm_write = 0;
	session->pcm_used = 0;
	session->pending_pcm.clear();
	session->pending_pcm_read = 0;
	session->out_of_data = false;
	session->decoder_error = false;
	session->decoder_needs_drain = false;
	session->decoder_busy = false;
}

static bool copy_encoded_chunk(MhiHostSession *session, std::vector<uae_u8> &chunk, std::vector<uae_u32> &tokens)
{
	lock_session(session);
	const size_t count = std::min<size_t>(MHI_HOST_DECODE_CHUNK, session->encoded_bytes);
	if (count > 0 && pcm_free_frames(session) >= min_free_decode_frames()) {
		chunk.clear();
		tokens.clear();
		chunk.reserve(count);
		while (chunk.size() < count && !session->encoded.empty()) {
			MhiEncodedBuffer &front = session->encoded.front();
			const size_t available = front.data.size() - front.offset;
			const size_t wanted = std::min(count - chunk.size(), available);
			chunk.insert(chunk.end(), front.data.begin() + front.offset, front.data.begin() + front.offset + wanted);
			front.offset += wanted;
			session->encoded_bytes -= wanted;
			if (front.offset == front.data.size()) {
				tokens.push_back(front.token);
				session->encoded.pop_front();
			}
		}
		if (!chunk.empty())
			session->decoder_busy = true;
	} else {
		chunk.clear();
		tokens.clear();
	}
	unlock_session(session);
	return !chunk.empty();
}

#if defined(HAVE_MPG123)
static bool decode_chunk(MhiHostSession *session, const std::vector<uae_u8> &chunk, const std::vector<uae_u32> &tokens)
{
	uae_u8 decoded[MHI_HOST_DECODE_OUT];
	const unsigned char *input = chunk.empty() ? nullptr : chunk.data();
	size_t input_size = chunk.size();
	bool result = true;
	bool needs_drain = true;

	lock_decoder(session);
	lock_session(session);
	session->pending_decode_tokens.insert(session->pending_decode_tokens.end(), tokens.begin(), tokens.end());
	if (!flush_pending_pcm_locked(session)) {
		session->decoder_needs_drain = true;
		session->decoder_busy = false;
		unlock_session(session);
		unlock_decoder(session);
		return false;
	}
	unlock_session(session);

	for (;;) {
		size_t done = 0;
		const int ret = mpg123_decode(session->mpg, input, input_size,
			decoded, sizeof(decoded), &done);
		input = nullptr;
		input_size = 0;

		if (done > 0) {
			const size_t frames = done / (sizeof(uae_s16) * 2);
			lock_session(session);
			const auto *samples = reinterpret_cast<uae_s16*>(decoded);
			const size_t pushed = push_pcm_frames_available(session, samples, frames);
			if (pushed > 0)
				session->out_of_data = false;
			if (pushed < frames) {
				session->pending_pcm.assign(samples + pushed * 2, samples + frames * 2);
				session->pending_pcm_read = 0;
			}
			unlock_session(session);
			if (pushed < frames) {
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
	const bool signal = !needs_drain ? complete_pending_decode_tokens_locked(session) : false;
	session->decoder_busy = false;
	unlock_session(session);
	unlock_decoder(session);
	signal_if_needed(session, signal);
	return result;
}
#endif

static int mhi_decode_worker(void *opaque)
{
	auto *session = static_cast<MhiHostSession*>(opaque);
	std::vector<uae_u8> chunk;
	std::vector<uae_u32> tokens;

	for (;;) {
		lock_session(session);
		const bool running = session->worker_running;
		const bool paused = session->paused || !session->playing;
		const bool drain = (session->decoder_needs_drain || has_pending_pcm(session))
			&& pcm_free_frames(session) >= min_free_decode_frames();
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
			tokens.clear();
		} else if (!copy_encoded_chunk(session, chunk, tokens)) {
			sleep_millis(2);
			continue;
		}
		decode_chunk(session, chunk, tokens);
#else
		if (!copy_encoded_chunk(session, chunk, tokens)) {
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

#if defined(HAVE_MPG123)
static bool configure_mpg123_output(MhiHostSession *session)
{
	if (mpg123_param(session->mpg, MPG123_FORCE_RATE, session->output_rate, 0.0) != MPG123_OK
		|| mpg123_format_none(session->mpg) != MPG123_OK
		|| mpg123_format(session->mpg, session->output_rate, MPG123_STEREO, MPG123_ENC_SIGNED_16) != MPG123_OK) {
		write_log(_T("MHI: failed to configure mpg123 output format: %s\n"), mpg123_strerror(session->mpg));
		return false;
	}
	return true;
}
#endif

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
	if (!configure_mpg123_output(session)) {
		destroy_session(session);
		return 0;
	}
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
	if (!playback_drained_locked(session))
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

uae_u32 mhi_host_query(TrapContext*, uae_u32 query)
{
	switch (query) {
		case UAE_MHI_QUERY_MPEG1:
		case UAE_MHI_QUERY_MPEG2:
		case UAE_MHI_QUERY_MPEG25:
		case UAE_MHI_QUERY_LAYER1:
		case UAE_MHI_QUERY_LAYER2:
		case UAE_MHI_QUERY_LAYER3:
		case UAE_MHI_QUERY_VARIABLE_BITRATE:
		case UAE_MHI_QUERY_JOINT_STEREO:
		case UAE_MHI_QUERY_VOLUME_CONTROL:
		case UAE_MHI_QUERY_PANNING_CONTROL:
			return 1;
		case UAE_MHI_QUERY_MPEG4:
		case UAE_MHI_QUERY_IS_HARDWARE:
		case UAE_MHI_QUERY_IS_68K:
		case UAE_MHI_QUERY_IS_PPC:
		default:
			return 0;
	}
}
