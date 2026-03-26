/*
* Amiberry
*
* SDL3 sound interface
*
* Copyright 2020-2025 Dimitris Panokostas <midwan@gmail.com>
* Based on WinUAE code by Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "audio.h"
#include "events.h"
#include "custom.h"
#include "gui.h"
#include "savestate.h"
#ifdef DRIVESOUND
#include "driveclick.h"
#endif
#include "gensound.h"
#include "xwin.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>

#include "sounddep/sound.h"

#ifdef LIBRETRO
#include "libretro_shared.h"
#endif

struct sound_dp
{
	SDL_AudioStream* stream;
	int sndbufsize;
	int framesperbuffer;
	int sndbuf;
	int pullmode;
	uae_u8* pullbuffer;
	unsigned int pullbufferlen;
	int pullbuffermaxlen;
	bool gotpullevent;
	float avg_correct;
	float cnt_correct;
	int stream_initialised;
	int silence_written;
	int push_target_buffers;
	int push_stable_writes;
};

#define SND_STATUSCNT 10

#define PUSH_TARGET_BUFFERS_MIN 2
#define PUSH_TARGET_BUFFERS_MAX 3
#define PUSH_TARGET_STABLE_WRITES 48

#define ADJUST_SIZE 20
#define EXP 1.9

#define ADJUST_VSSIZE 12
#define EXPVS 1.6

int sound_debug = 0;
int sound_mode_skip = 0;
int sounddrivermask;

static int have_sound;
static int statuscnt;

#define SND_MAX_BUFFER2 524288
#define SND_MAX_BUFFER 65536

#if SOUND_MODE_NG
uae_u16 paula_sndbuffer[SND_MAX_BUFFER * 2 + 8];
#else
uae_u16 paula_sndbuffer[SND_MAX_BUFFER * 2 + 1024];
#endif
uae_u16 *paula_sndbufpt;
int paula_sndbufsize;
int active_sound_stereo;

struct sound_device *sound_devices[MAX_SOUND_DEVICES];
struct sound_device *record_devices[MAX_SOUND_DEVICES];
static int num_sound_devices, num_record_devices;

static struct sound_data sdpaula;
static struct sound_data *sdp = &sdpaula;

static uae_u8 *extrasndbuf;
static int extrasndbufsize;
static int extrasndbuffered;

static void reset_push_timing_state(struct sound_data* sd)
{
	auto* s = sd->data;
	s->avg_correct = 0;
	s->cnt_correct = 0;
	s->push_target_buffers = PUSH_TARGET_BUFFERS_MIN;
	s->push_stable_writes = 0;
}

static bool lock_audio_stream_sdl(SDL_AudioStream* stream)
{
#ifdef LIBRETRO
	SDL_LockAudioStream(stream);
	return true;
#else
	return SDL_LockAudioStream(stream);
#endif
}

static void clear_audio_stream_queue_sdl(sound_dp* s)
{
	if (!s || !s->stream)
		return;

#ifdef LIBRETRO
	uae_u8 discard[4096];
	while (SDL_GetAudioStreamQueued(s->stream) > 0) {
		const int drained = SDL_GetAudioStreamData(s->stream, discard, sizeof discard);
		if (drained <= 0)
			break;
	}
#else
	if (!SDL_ClearAudioStream(s->stream)) {
		write_log("SDL_ClearAudioStream failed: %s\n", SDL_GetError());
	}
#endif
}

static void clear_stream_state_sdl(struct sound_data* sd)
{
	auto* s = sd->data;
	if (!s || !s->stream)
		return;

	clear_audio_stream_queue_sdl(s);

	if (lock_audio_stream_sdl(s->stream)) {
		s->pullbufferlen = 0;
		s->stream_initialised = 0;
		if (s->pullbuffer) {
			std::memset(s->pullbuffer, 0, s->pullbuffermaxlen);
		}
		SDL_UnlockAudioStream(s->stream);
	}
}

static void update_push_target_latency(struct sound_data* sd, int queued)
{
	auto* s = sd->data;
	if (queued <= sd->sndbufsize) {
		s->push_target_buffers = PUSH_TARGET_BUFFERS_MAX;
		s->push_stable_writes = 0;
		return;
	}

	if (s->push_target_buffers > PUSH_TARGET_BUFFERS_MIN) {
		const int stable_threshold = sd->sndbufsize * PUSH_TARGET_BUFFERS_MIN;
		if (queued >= stable_threshold) {
			s->push_stable_writes++;
			if (s->push_stable_writes >= PUSH_TARGET_STABLE_WRITES) {
				s->push_target_buffers = PUSH_TARGET_BUFFERS_MIN;
				s->push_stable_writes = 0;
			}
		} else {
			s->push_stable_writes = 0;
		}
	}
}

// SDL3 Audio stream get callback (pull mode)
static void SDLCALL sdl3_audio_get_callback(void* userdata, SDL_AudioStream* astream, int additional_amount, int total_amount)
{
	auto* sd = static_cast<sound_data*>(userdata);
	auto* s = sd->data;

	if (!s->stream_initialised || sd->mute) {
		std::vector<uint8_t> silence(additional_amount, 0);
		SDL_PutAudioStreamData(astream, silence.data(), additional_amount);
		if (sd->mute) s->silence_written++;
		s->stream_initialised = 1;
		return;
	}

	if (!s->framesperbuffer || sdp->deactive)
		return;

	if (s->pullbufferlen <= 0) {
		gui_data.sndbuf_status = -1;
		return;
	}

	const auto bytes_to_copy = std::min(static_cast<unsigned int>(additional_amount), static_cast<unsigned int>(s->pullbufferlen));
	if (sd->mute == 0 && bytes_to_copy > 0) {
		SDL_PutAudioStreamData(astream, s->pullbuffer, bytes_to_copy);
	}

	if (bytes_to_copy < s->pullbufferlen) {
		std::memmove(s->pullbuffer, s->pullbuffer + bytes_to_copy, s->pullbufferlen - bytes_to_copy);
	}
	s->pullbufferlen -= bytes_to_copy;
}

int setup_sound (void)
{
	sound_available = 1;
	return 1;
}

float sound_sync_multiplier = 1.0;
float scaled_sample_evtime_orig;
extern float sampler_evtime;

void update_sound (float clk)
{
	if (!have_sound)
		return;
	scaled_sample_evtime_orig = clk * (float)CYCLE_UNIT * sound_sync_multiplier / sdp->obtainedfreq;
	scaled_sample_evtime = scaled_sample_evtime_orig;
	sampler_evtime = clk * CYCLE_UNIT * sound_sync_multiplier;
}

extern frame_time_t vsynctimebase_orig;

#ifndef AVIOUTPUT
static int avioutput_audio;
#endif

#define ADJUST_LIMIT 6
#define ADJUST_LIMIT2 1

void sound_setadjust (float v)
{
	float mult;

	if (v < -ADJUST_LIMIT)
		v = -ADJUST_LIMIT;
	if (v > ADJUST_LIMIT)
		v = ADJUST_LIMIT;

	mult = (1000.0f + v);
#ifdef AVIOUTPUT
	if (avioutput_audio && avioutput_enabled && avioutput_nosoundsync)
		mult = 1000.0f;
	if (isvsync_chipset() || (avioutput_audio && avioutput_enabled && !currprefs.cachesize)) {
#else
	if (isvsync_chipset()) {
#endif
		vsynctimebase = vsynctimebase_orig;
		scaled_sample_evtime = scaled_sample_evtime_orig * mult / 1000.0f;
	} else if (currprefs.cachesize || currprefs.m68k_speed != 0) {
		vsynctimebase = (int)(vsynctimebase_orig * mult / 1000.0f);
		scaled_sample_evtime = scaled_sample_evtime_orig;
	} else {
		vsynctimebase = (int)(vsynctimebase_orig * mult / 1000.0f);
		scaled_sample_evtime = scaled_sample_evtime_orig;
	}
}

static void docorrection (struct sound_dp *s, int sndbuf, float sync, int granulaty)
{
	static int tfprev;

	s->avg_correct += sync;
	s->cnt_correct++;

	if (granulaty < 10)
		granulaty = 10;

	if (tfprev != timeframes) {
		float skipmode, avgskipmode;
		float avg = s->avg_correct / s->cnt_correct;

		skipmode = sync / 100.0f;
		avgskipmode = avg / (10000.0f / granulaty);

		if ((0 || sound_debug) && (tfprev % 10) == 0) {
			write_log (_T("%+05d S=%7.1f AVG=%7.1f (IMM=%7.1f + AVG=%7.1f = %7.1f)\n"), sndbuf, sync, avg, skipmode, avgskipmode, skipmode + avgskipmode);
		}
		gui_data.sndbuf = sndbuf;

		if (skipmode > ADJUST_LIMIT2)
			skipmode = ADJUST_LIMIT2;
		if (skipmode < -ADJUST_LIMIT2)
			skipmode = -ADJUST_LIMIT2;

		sound_setadjust (skipmode + avgskipmode);
		tfprev = timeframes;
	}
}

static float sync_sound (float m)
{
	float skipmode;
	if (isvsync ()) {

		skipmode = (float)pow(m < 0 ? -m : m, EXPVS) / 2.0f;
		if (m < 0)
			skipmode = -skipmode;
		if (skipmode < -ADJUST_VSSIZE)
			skipmode = -ADJUST_VSSIZE;
		if (skipmode > ADJUST_VSSIZE)
			skipmode = ADJUST_VSSIZE;

	} else {

		skipmode = (float)pow(m < 0 ? -m : m, EXP) / 2.0f;
		if (m < 0)
			skipmode = -skipmode;
		if (skipmode < -ADJUST_SIZE)
			skipmode = -ADJUST_SIZE;
		if (skipmode > ADJUST_SIZE)
			skipmode = ADJUST_SIZE;
	}

	return skipmode;
}

static void clearbuffer_sdl(struct sound_data *sd)
{
	const sound_dp* s = sd->data;

	clear_stream_state_sdl(sd);

	if (!s || !s->stream)
		return;
	if (!lock_audio_stream_sdl(s->stream))
		return;
	std::memset(paula_sndbuffer, 0, sizeof paula_sndbuffer);
	SDL_UnlockAudioStream(s->stream);
}

static void clearbuffer (struct sound_data *sd)
{
	const auto* s = sd->data;
	if (sd->devicetype == SOUND_DEVICE_SDL2)
		clearbuffer_sdl(sd);
	else
		std::memset(paula_sndbuffer, 0, sizeof paula_sndbuffer);
	if (s->pullbuffer && (!s->stream || sd->devicetype != SOUND_DEVICE_SDL2)) {
		if (sd->devicetype == SOUND_DEVICE_SDL2)
			SDL_LockAudioStream(s->stream);
		std::memset(s->pullbuffer, 0, s->pullbuffermaxlen);
		if (sd->devicetype == SOUND_DEVICE_SDL2)
			SDL_UnlockAudioStream(s->stream);
	}
}

#include "sound_platform_internal.h"

static void set_reset(struct sound_data *sd)
{
	sd->reset = true;
	sd->resetcnt = 10;
	sd->resetframecnt = 0;
}

static void pause_audio_sdl(struct sound_data* sd)
{
	const sound_dp* s = sd->data;
	
	sd->waiting_for_buffer = 0;
	SDL_PauseAudioStreamDevice(s->stream);
	clearbuffer(sd);
}

static void resume_audio_sdl(struct sound_data* sd)
{
	const auto* s = sd->data;

	clearbuffer(sd);
	sd->waiting_for_buffer = 1;
	reset_push_timing_state(sd);
	SDL_ResumeAudioStreamDevice(s->stream);
	sd->paused = 0;
}

static void close_audio_sdl(struct sound_data* sd)
{
	auto* s = sd->data;
	SDL_PauseAudioStreamDevice(s->stream);
	clear_stream_state_sdl(sd);
	
	SDL_LockAudioStream(s->stream);
	if (s->pullbuffer != nullptr)
	{
		xfree(s->pullbuffer);
		s->pullbuffer = nullptr;
	}
	s->pullbufferlen = 0;
	SDL_UnlockAudioStream(s->stream);
	
	SDL_DestroyAudioStream(s->stream);
	s->stream = nullptr;
}

extern void setvolume_ahi (int);

void set_volume_sound_device (struct sound_data *sd, int volume, int mute)
{
	struct sound_dp *s = sd->data;
	if (!s) {
		return;
	}

	if (sd->devicetype == SOUND_DEVICE_SDL2) {
		sd->softvolume = -1;
		if (volume < 100 && !mute) {
			sd->softvolume = (int)((100.0f - volume) * 32768.0f / 100.0f);
			if (sd->softvolume >= 32768)
				sd->softvolume = -1;
		}
		if (mute || volume >= 100)
			sd->softvolume = 0;
	}
}

void set_volume(int volume, int mute)
{
	set_volume_sound_device(sdp, volume, mute);
	setvolume_ahi(volume);
	config_changed = 1;
}

static void finish_sound_buffer_sdl_push(struct sound_data* sd, uae_u16* sndbuffer)
{
	sound_dp* s = sd->data;
	if (sd->mute) {
		std::memset(sndbuffer, 0, sd->sndbufsize);
		s->silence_written++; // In push mode no sound gen means no audio push so this might not be incremented frequently
	}
	if (!SDL_PutAudioStreamData(s->stream, sndbuffer, sd->sndbufsize)) {
		write_log("SDL_PutAudioStreamData failed: %s\n", SDL_GetError());
		set_reset(sd);
		return;
	}

	// Sync
	const int queued = SDL_GetAudioStreamQueued(s->stream);
	if (queued < 0) return; // stream error
	update_push_target_latency(sd, queued);
	const auto target = sd->sndbufsize * std::max(s->push_target_buffers, PUSH_TARGET_BUFFERS_MIN);
	const auto diff = queued - target;
	const auto val = static_cast<float>(diff) / static_cast<float>(sd->samplesize);
	
	int vis = 0;
	if (target > 0)
		vis = queued * 500 / target; // 500 = 50% (nominal)
	
	docorrection(s, vis, val, 100);
}

static void finish_sound_buffer_pull(struct sound_data* sd, uae_u16* sndbuffer)
{
	auto* s = sd->data;
	static unsigned int pull_overflow_count;

	SDL_LockAudioStream(s->stream);
	if (s->pullbufferlen + sd->sndbufsize > s->pullbuffermaxlen) {
		pull_overflow_count++;
		if (pull_overflow_count <= 8 || (pull_overflow_count % 512) == 0) {
			write_log(_T("pull overflow! %d %d %d (count=%u)\n"),
				s->pullbufferlen, sd->sndbufsize, s->pullbuffermaxlen, pull_overflow_count);
		}
		s->pullbufferlen = 0;
		gui_data.sndbuf_status = 1;
	}
	else 
		gui_data.sndbuf_status = 0;
	
	std::memcpy(s->pullbuffer + s->pullbufferlen, sndbuffer, sd->sndbufsize);
	s->pullbufferlen += sd->sndbufsize;
	
	// Calc sync inside lock to get consistent len
	const auto len = s->pullbufferlen;
	const auto maxlen = s->pullbuffermaxlen;
	SDL_UnlockAudioStream(s->stream);

	if (maxlen > 0)
	{
		const auto target = maxlen / 2;
		const auto diff = len - target;
		const auto val = static_cast<float>(diff) / static_cast<float>(sd->samplesize);
		const auto vis = len * 1000 / maxlen;
		docorrection(s, vis, val, 100);
	}
}

static int open_audio_sdl(struct sound_data* sd, int index)
{
	auto* const s = sd->data;
	const auto freq = sd->freq;
	const auto ch = sd->channels;
	auto devname = sound_devices[index]->name;
	const bool use_default_device = currprefs.soundcard_default || sound_devices[index] == nullptr;
	const SDL_AudioDeviceID device_id = use_default_device
		? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
		: static_cast<SDL_AudioDeviceID>(sound_devices[index]->id);
	sd->devicetype = SOUND_DEVICE_SDL2;
	sd->sndbufsize = std::max(sd->sndbufsize, 0x80);
	s->framesperbuffer = sd->sndbufsize;
	s->sndbufsize = s->framesperbuffer;
	sd->sndbufsize = s->sndbufsize * ch * 2;
	if (sd->sndbufsize > SND_MAX_BUFFER)
		sd->sndbufsize = SND_MAX_BUFFER;
	sd->samplesize = ch * 16 / 8;
	s->pullmode = currprefs.sound_pullmode;

	if (sound_platform_open_audio(sd, index))
		return 1;

	SDL_AudioSpec spec = {};
	spec.format = SDL_AUDIO_S16;
	spec.channels = ch;
	spec.freq = freq;

	if (s->pullmode)
	{
		// Pull mode: use stream with get callback
		s->stream = SDL_OpenAudioDeviceStream(
			device_id, &spec, sdl3_audio_get_callback, sd);
	}
	else
	{
		// Push mode: use stream without callback, we'll push data manually
		s->stream = SDL_OpenAudioDeviceStream(
			device_id, &spec, NULL, NULL);
	}

	if (!s->stream)
	{
		if (!use_default_device)
		{
			write_log("Failed to open selected SDL3 audio device '%s': %s, retrying with default device\n", devname, SDL_GetError());
			if (s->pullmode)
			{
				s->stream = SDL_OpenAudioDeviceStream(
					SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, sdl3_audio_get_callback, sd);
			}
			else
			{
				s->stream = SDL_OpenAudioDeviceStream(
					SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
			}
		}
		if (!s->stream)
		{
			write_log("Failed to open default SDL3 audio device stream: %s\n", SDL_GetError());
			return 0;
		}
	}

	if (s->pullmode)
	{
		s->pullbuffermaxlen = sd->sndbufsize * 2;
		s->pullbuffer = xcalloc(uae_u8, s->pullbuffermaxlen);
		s->pullbufferlen = 0;
	}
	reset_push_timing_state(sd);
	write_log("SDL3: CH=%d, FREQ=%d '%s' buffer %d/%d (%s)\n", ch, freq, sound_devices[index]->name,
		s->sndbufsize, s->framesperbuffer, !s->pullmode ? _T("push") : _T("pull"));
	clearbuffer(sd);

	return 1;
}

int open_sound_device (struct sound_data *sd, int index, int exclusive, int bufsize, int freq, int channels)
{
	auto* dp = xcalloc(struct sound_dp, 1);

	sd->data = dp;
	sd->sndbufsize = bufsize;
	sd->freq = freq;
	sd->channels = channels;
	sd->paused = 1;
	sd->index = index;
	const auto ret = open_audio_sdl(sd, index);
	sd->samplesize = sd->channels * 2;
	sd->sndbufframes = sd->sndbufsize / sd->samplesize;
	return ret;
}

void close_sound_device (struct sound_data *sd)
{
	pause_sound_device(sd);
	close_audio_sdl(sd);
	xfree(sd->data);
	sd->data = NULL;
	sd->index = -1;
}

void pause_sound_device (struct sound_data *sd)
{
	sd->paused = 1;
	gui_data.sndbuf_status = 0;
	gui_data.sndbuf = 0;
	pause_audio_sdl(sd);
}
void resume_sound_device (struct sound_data *sd)
{
	resume_audio_sdl(sd);
	sd->paused = 0;
}

static int open_sound()
{
	auto size = currprefs.sound_maxbsiz;
	
	if (!currprefs.produce_sound)
		return 0;
	config_changed = 1;
	/* Always interpret buffer size as number of samples, not as actual
	buffer size.  Of course, since 8192 is the default, we'll have to
	scale that to a sane value (assuming that otherwise 16 bits and
	stereo would have been enabled, and we'd have done the shift by
	two anyway).  */
	size >>= 2;
	size &= ~63;

	sdp->softvolume = -1;
	int num = enumerate_sound_devices();
	if (currprefs.soundcard >= num)
		currprefs.soundcard = changed_prefs.soundcard = 0;
	if (num == 0)
		return 0;
	const auto ch = get_audio_nativechannels(active_sound_stereo);
	const auto ret = open_sound_device(sdp, currprefs.soundcard, 0, size, currprefs.sound_freq, ch);
	if (!ret)
		return 0;
	currprefs.sound_freq = changed_prefs.sound_freq = sdp->freq;
	if (ch != sdp->channels)
		active_sound_stereo = get_audio_stereomode(sdp->channels);

	set_volume(currprefs.sound_volume_master, sdp->mute);
	if (get_audio_amigachannels(active_sound_stereo) == 4)
		sample_handler = sample16ss_handler;
	else
		sample_handler = get_audio_ismono(active_sound_stereo) ? sample16_handler : sample16s_handler;

	sdp->obtainedfreq = currprefs.sound_freq;

	have_sound = 1;
	sound_available = 1;

	gui_data.sndbuf_avail = true;
	
	paula_sndbufsize = sdp->sndbufsize;
	paula_sndbufpt = paula_sndbuffer;
#ifdef DRIVESOUND
	driveclick_init();
#endif
	return 1;
}

void close_sound()
{
	config_changed = 1;
	gui_data.sndbuf = 0;
	gui_data.sndbuf_status = 3;
	gui_data.sndbuf_avail = false;
	if (!have_sound)
		return;
	close_sound_device(sdp);
	have_sound = 0;
	extrasndbufsize = 0;
	extrasndbuffered = 0;
	xfree(extrasndbuf);
	extrasndbuf = nullptr;
}

bool sound_paused()
{
	return sdp->paused != 0;
}

void pause_sound()
{
	if (sdp->paused)
		return;
	if (!have_sound)
		return;
	pause_sound_device(sdp);
}

void resume_sound()
{
	if (!sdp->paused)
		return;
	if (!have_sound)
		return;
	resume_sound_device(sdp);
}

void reset_sound()
{
	if (!have_sound)
		return;
	clearbuffer(sdp);
}

int init_sound()
{
	bool started = false;
	gui_data.sndbuf_status = 3;
	gui_data.sndbuf = 0;
	gui_data.sndbuf_avail = false;
	if (!sound_available)
		return 0;
	if (currprefs.produce_sound <= 1)
		return 0;
	if (have_sound)
		return 1;
	if (!open_sound())
		return 0;
	sdp->paused = 1;
#ifdef DRIVESOUND
	driveclick_reset();
#endif
	reset_sound();
	resume_sound();
	if (!started &&
		(currprefs.start_minimized && currprefs.minimized_nosound ||
			currprefs.start_uncaptured && currprefs.inactive_nosound))
		pause_sound();
	started = true;
	return 1;
}

static void disable_sound()
{
	close_sound();
	currprefs.produce_sound = changed_prefs.produce_sound = 1;
}

static int reopen_sound(void)
{
	const auto paused = sdp->paused != 0;
	close_sound();
	const auto v = open_sound();
	if (v && !paused)
		resume_sound_device(sdp);
	return v;
}

void pause_sound_buffer()
{
	sdp->deactive = true;
	reset_sound();
}

void restart_sound_buffer()
{
	sdp->deactive = false;
}

static bool send_sound_do(struct sound_data* sd);


int get_sound_event(void)
{
	int type = sdp->devicetype;
	if (sdp->paused || sdp->deactive)
		return 0;
	return 0;
}

bool audio_is_event_frame_possible(int)
{
	const int type = sdp->devicetype;
	if (sdp->paused || sdp->deactive || sdp->reset)
		return false;
	if (type == SOUND_DEVICE_SDL2)
	{
		sound_dp* s = sdp->data;
		int bufsize = static_cast<int>(reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer));
		bufsize /= sdp->samplesize;
		const int todo = s->sndbufsize - bufsize;
		int samplesperframe = sdp->obtainedfreq / static_cast<int>(vblank_hz);
		return samplesperframe >= todo - samplesperframe;
	}
	return false;
}

int audio_is_pull()
{
	if (sdp->reset)
		return 0;
	const auto* s = sdp->data;
	if (s && s->pullmode) {
		return sdp->paused || sdp->deactive ? -1 : 1;
	}
	return 0;
}

int audio_pull_buffer()
{
	auto cnt = 0;
	if (sdp->paused || sdp->deactive || sdp->reset || !sdp->data)
		return 0;
	const auto* s = sdp->data;
	if (s->pullbufferlen > 0) {
		cnt++;
		int size = static_cast<int>(reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer));
		if (size > sdp->sndbufsize * 2 / 3)
			cnt++;
	}
	return cnt;
}

bool audio_is_pull_event()
{
	if (sdp->paused || sdp->deactive || sdp->reset)
		return false;
	return false;
}

bool audio_finish_pull()
{
	const int type = sdp->devicetype;
	if (sdp->paused || sdp->deactive || sdp->reset)
		return false;
	int apb = audio_pull_buffer();
	if (apb >= 2 || (apb == 1 && audio_is_pull_event())) {
		return send_sound_do(sdp);
	}
	return false;
}

static void finish_sound_buffer_sdl(struct sound_data *sd, uae_u16 *sndbuffer)
{
	if (sound_platform_output_audio(sd, sndbuffer))
		return;
	const auto* s = sd->data;
	if (!sd->waiting_for_buffer)
		return;
	
	if (s->pullmode)
		finish_sound_buffer_pull(sd, sndbuffer);
	else
		finish_sound_buffer_sdl_push(sd, sndbuffer);
}

static void channelswap(uae_s16* sndbuffer, int len)
{
	for (auto i = 0; i < len; i += 2) {
		const auto t = sndbuffer[i];
		sndbuffer[i] = sndbuffer[i + 1];
		sndbuffer[i + 1] = t;
	}
}
static void channelswap6(uae_s16* sndbuffer, int len)
{
	for (auto i = 0; i < len; i += 6) {
		auto t = sndbuffer[i + 0];
		sndbuffer[i + 0] = sndbuffer[i + 1];
		sndbuffer[i + 1] = t;
		t = sndbuffer[i + 4];
		sndbuffer[i + 4] = sndbuffer[i + 5];
		sndbuffer[i + 5] = t;
	}
}

static bool send_sound_do(struct sound_data* sd)
{
	if (const int type = sd->devicetype; type == SOUND_DEVICE_SDL2) {
		finish_sound_buffer_pull(sd, paula_sndbuffer);
		return true;
	}
	return false;
}

static void send_sound(struct sound_data* sd, uae_u16* sndbuffer)
{
	const int type = sd->devicetype;
	if (savestate_state)
		return;
	if (sd->paused)
		return;
	if (sd->softvolume >= 0) {
		auto* p = reinterpret_cast<uae_s16*>(sndbuffer);
		for (auto i = 0; i < sd->sndbufsize / 2; i++) {
			p[i] = p[i] * sd->softvolume / 32768;
		}
	}
	finish_sound_buffer_sdl(sd, sndbuffer);
}

static void handle_reset()
{
	if (sdp->resetframe == timeframes)
		return;
	sdp->resetframe = static_cast<int>(timeframes);
	sdp->resetframecnt--;
	if (sdp->resetframecnt > 0)
		return;
	sdp->resetframecnt = 20;

	sdp->reset = false;
	if (!reopen_sound() || sdp->reset) {
		if (sdp->resetcnt <= 0) {
			write_log(_T("Reopen sound failed. Retrying with default device.\n"));
			close_sound();
			int type = sound_devices[currprefs.soundcard]->type;
			int max = enumerate_sound_devices();
			for (int i = 0; i < max; i++) {
				if (sound_devices[i]->alname == NULL && sound_devices[i]->type == type) {
					currprefs.soundcard = changed_prefs.soundcard = i;
					if (open_sound())
						return;
					break;
				}
			}
			currprefs.produce_sound = changed_prefs.produce_sound = 1;
		} else {
			write_log(_T("Retrying sound.. %d..\n"), sdp->resetcnt);
			sdp->resetcnt--;
			sdp->reset = true;
		}
	} else {
		resume_sound_device(sdp);
	}
}

void finish_sound_buffer()
{
	static unsigned long tframe;
	int bufsize = addrdiff((uae_u8*)paula_sndbufpt, (uae_u8*)paula_sndbuffer);

	if (sdp->reset) {
		handle_reset();
		paula_sndbufpt = paula_sndbuffer;
		return;
	}
	
	if (currprefs.turbo_emulation) {
		paula_sndbufpt = paula_sndbuffer;
		return;
	}
	if (currprefs.sound_stereo_swap_paula) {
		if (get_audio_nativechannels(active_sound_stereo) == 2 || get_audio_nativechannels(active_sound_stereo) == 4)
			channelswap(reinterpret_cast<uae_s16*>(paula_sndbuffer), bufsize / 2);
		else if (get_audio_nativechannels(active_sound_stereo) >= 6)
			channelswap6(reinterpret_cast<uae_s16*>(paula_sndbuffer), bufsize / 2);
	}

	// driveclick_mix is called in audio.cpp, same as in WinUAE.
	paula_sndbufpt = paula_sndbuffer;
	
#ifdef AVIOUTPUT
	if (avioutput_audio) {
		if (AVIOutput_WriteAudio((uae_u8*)paula_sndbuffer, bufsize)) {
			if (avioutput_nosoundsync)
				sound_setadjust(0);
		}
	}
	if (avioutput_enabled && (!avioutput_framelimiter || avioutput_nosoundoutput))
		return;
#endif
	if (!have_sound)
		return;

	// we got buffer that was not full (recording active). Need special handling.
	if (bufsize < sdp->sndbufsize && !extrasndbuf) {
		extrasndbufsize = sdp->sndbufsize;
		extrasndbuf = xcalloc(uae_u8, sdp->sndbufsize);
		extrasndbuffered = 0;
	}

	if (statuscnt > 0 && tframe != timeframes) {
		tframe = timeframes;
		statuscnt--;
		if (statuscnt == 0)
			gui_data.sndbuf_status = 0;
	}
	if (gui_data.sndbuf_status == 3)
		gui_data.sndbuf_status = 0;

	if (extrasndbuf) {
		const auto size = extrasndbuffered + bufsize;
		auto copied = 0;
		if (size > extrasndbufsize) {
			copied = extrasndbufsize - extrasndbuffered;
			std::memcpy(extrasndbuf + extrasndbuffered, paula_sndbuffer, copied);
			send_sound(sdp, reinterpret_cast<uae_u16*>(extrasndbuf));
			extrasndbuffered = 0;
		}
		std::memcpy(extrasndbuf + extrasndbuffered, reinterpret_cast<uae_u8*>(paula_sndbuffer) + copied, bufsize - copied);
		extrasndbuffered += bufsize - copied;
	} else {
		send_sound(sdp, paula_sndbuffer);
	}
}

int enumerate_sound_devices()
{
	if (sound_platform_enumerate_devices())
		return num_sound_devices;
	if (!num_sound_devices) {

		write_log("Enumerating SDL3 playback devices...\n");
		int playback_count = 0;
		SDL_AudioDeviceID* playback_devices = SDL_GetAudioPlaybackDevices(&playback_count);
		num_sound_devices = playback_count;
		write_log("Detected %d sound playback devices\n", num_sound_devices);
		for (int i = 0; i < num_sound_devices && i < MAX_SOUND_DEVICES; i++)
		{
			const auto devname = SDL_GetAudioDeviceName(playback_devices[i]);
			write_log("Sound playback device %d: %s\n", i, devname);
			sound_devices[i] = xcalloc(struct sound_device, 1);
			sound_devices[i]->id = static_cast<int>(playback_devices[i]);
			sound_devices[i]->cfgname = my_strdup(devname);
			sound_devices[i]->type = SOUND_DEVICE_SDL2;
			sound_devices[i]->name = my_strdup(devname);
			sound_devices[i]->alname = my_strdup(std::to_string(i).c_str());
		}
		SDL_free(playback_devices);

		write_log("Enumerating SDL3 recording devices...\n");
		int recording_count = 0;
		SDL_AudioDeviceID* recording_devices = SDL_GetAudioRecordingDevices(&recording_count);
		num_record_devices = recording_count;
		write_log("Detected %d sound recording devices\n", num_record_devices);
		for (int i = 0; i < num_record_devices && i < MAX_SOUND_DEVICES; i++)
		{
			const auto devname = SDL_GetAudioDeviceName(recording_devices[i]);
			write_log("Sound recording device %d: %s\n", i, devname);
			record_devices[i] = xcalloc(struct sound_device, 1);
			record_devices[i]->id = static_cast<int>(recording_devices[i]);
			record_devices[i]->cfgname = my_strdup(devname);
			record_devices[i]->type = SOUND_DEVICE_SDL2;
			record_devices[i]->name = my_strdup(devname);
			record_devices[i]->alname = my_strdup(std::to_string(i).c_str());
		}
		SDL_free(recording_devices);

		write_log(_T("Enumeration end\n"));
		for (num_sound_devices = 0; num_sound_devices < MAX_SOUND_DEVICES; num_sound_devices++) {
			if (sound_devices[num_sound_devices] == NULL)
				break;
		}
		for (num_record_devices = 0; num_record_devices < MAX_SOUND_DEVICES; num_record_devices++) {
			if (record_devices[num_record_devices] == NULL)
				break;
		}
	}
	return num_sound_devices;
}

static int set_master_volume(int volume, int mute)
{
	set_volume(volume, mute);
	return 1;
}

static int get_master_volume(int* volume, int* mute)
{
	return currprefs.sound_volume_master;
}

void sound_mute(int newmute)
{
	if (newmute < 0)
		sdp->mute = sdp->mute ? 0 : 1;
	else
		sdp->mute = newmute;
	set_volume(currprefs.sound_volume_master, sdp->mute);
	config_changed = 1;
}

void sound_volume(int dir)
{
	currprefs.sound_volume_master -= dir * 10;
	currprefs.sound_volume_cd -= dir * 10;
	currprefs.sound_volume_master = std::max(currprefs.sound_volume_master, 0);
	currprefs.sound_volume_master = std::min(currprefs.sound_volume_master, 100);
	changed_prefs.sound_volume_master = currprefs.sound_volume_master;
	currprefs.sound_volume_cd = std::max(currprefs.sound_volume_cd, 0);
	currprefs.sound_volume_cd = std::min(currprefs.sound_volume_cd, 100);
	changed_prefs.sound_volume_cd = currprefs.sound_volume_cd;
	set_volume(currprefs.sound_volume_master, sdp->mute);
	config_changed = 1;
}

void master_sound_volume(int dir)
{
	int vol, mute;

	if (const auto r = get_master_volume(&vol, &mute); !r)
		return;
	if (dir == 0)
		mute = mute ? 0 : 1;
	vol += dir * (65536 / 10);
	vol = std::max(vol, 0);
	if (vol > 128) // Legacy SDL_MIX_MAXVOLUME was 128
		vol = 128;
	set_master_volume(vol, mute);
	config_changed = 1;
}

int sound_get_silence()
{
	const auto* s = sdp->data;
	return s->silence_written;
}
