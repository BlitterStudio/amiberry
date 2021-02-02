/* 
  * Sdl sound.c implementation
  * (c) 2015
  */
#include <string.h>
#include <unistd.h>

#include "sysconfig.h"
#include "sysdeps.h"

#include <math.h>

#include "options.h"
#include "audio.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "threaddep/thread.h"
#include "gui.h"
#include "savestate.h"
#ifdef DRIVESOUND
#include "driveclick.h"
#endif
#include "gensound.h"
#include "xwin.h"
#include "sounddep/sound.h"

#include "cda_play.h"

#ifdef ANDROID
#include <android/log.h>
#endif

struct sound_dp
{
	SDL_AudioDeviceID dev;
	int sndbufsize;
	int framesperbuffer;
	int sndbuf;
	int pullmode;
	uae_u8* pullbuffer;
	unsigned int pullbufferlen;
	int pullbuffermaxlen;
	double avg_correct;
	double cnt_correct;
};

#define SND_STATUSCNT 10

#define ADJUST_SIZE 20
#define EXP 1.9

#define ADJUST_VSSIZE 12
#define EXPVS 1.6

static int have_sound = 0;
static int statuscnt;

#define SND_MAX_BUFFER2 524288
#define SND_MAX_BUFFER 65536

uae_u16 paula_sndbuffer[SND_MAX_BUFFER];
uae_u16* paula_sndbufpt;
int paula_sndbufsize;

#ifdef AMIBERRY
void sdl2_audio_callback(void* userdata, Uint8* stream, int len);
#endif

struct sound_device* sound_devices[MAX_SOUND_DEVICES];
struct sound_device* record_devices[MAX_SOUND_DEVICES];

static struct sound_data sdpaula;
static struct sound_data* sdp = &sdpaula;

static uae_u8* extrasndbuf;
static int extrasndbufsize;
static int extrasndbuffered;

int setup_sound(void)
{
	sound_available = 1;
	return 1;
}

float sound_sync_multiplier = 1.0;
float scaled_sample_evtime_orig;
// Originally from sampler.cpp
float sampler_evtime;

void update_sound(double clk)
{
	if (!have_sound)
		return;
	scaled_sample_evtime_orig = clk * CYCLE_UNIT * sound_sync_multiplier / sdp->obtainedfreq;
	scaled_sample_evtime = scaled_sample_evtime_orig;
	sampler_evtime = clk * CYCLE_UNIT * sound_sync_multiplier;
}

extern int vsynctimebase_orig;

#define ADJUST_LIMIT 6
#define ADJUST_LIMIT2 1

void sound_setadjust(double v)
{
	if (v < -ADJUST_LIMIT)
		v = -ADJUST_LIMIT;
	if (v > ADJUST_LIMIT)
		v = ADJUST_LIMIT;

	const float mult = 1000.0f + v;
	if (isvsync_chipset()) {
		vsynctimebase = vsynctimebase_orig;
		scaled_sample_evtime = scaled_sample_evtime_orig * mult / 1000.0f;
	}
	else if (currprefs.cachesize || currprefs.m68k_speed != 0) {
		vsynctimebase = (int)(((double)vsynctimebase_orig) * mult / 1000.0f);
		scaled_sample_evtime = scaled_sample_evtime_orig;
	}
	else {
		vsynctimebase = (int)(((double)vsynctimebase_orig) * mult / 1000.0f);
		scaled_sample_evtime = scaled_sample_evtime_orig;
	}
}

static void docorrection(struct sound_dp* s, int sndbuf, double sync, int granulaty)
{
	static int tfprev;

	s->avg_correct += sync;
	s->cnt_correct++;

	if (granulaty < 10)
		granulaty = 10;

	if (tfprev != timeframes) {
		const auto avg = s->avg_correct / s->cnt_correct;

		auto skipmode = sync / 100.0;
		const auto avgskipmode = avg / (10000.0 / granulaty);

		gui_data.sndbuf = sndbuf;

		if (skipmode > ADJUST_LIMIT2)
			skipmode = ADJUST_LIMIT2;
		if (skipmode < -ADJUST_LIMIT2)
			skipmode = -ADJUST_LIMIT2;

		sound_setadjust(skipmode + avgskipmode);
		tfprev = timeframes;
	}
}

static double sync_sound(double m)
{
	double skipmode;
	if (isvsync()) {

		skipmode = pow(m < 0 ? -m : m, EXPVS) / 2;
		if (m < 0)
			skipmode = -skipmode;
		if (skipmode < -ADJUST_VSSIZE)
			skipmode = -ADJUST_VSSIZE;
		if (skipmode > ADJUST_VSSIZE)
			skipmode = ADJUST_VSSIZE;

	}
	else {

		skipmode = pow(m < 0 ? -m : m, EXP) / 2;
		if (m < 0)
			skipmode = -skipmode;
		if (skipmode < -ADJUST_SIZE)
			skipmode = -ADJUST_SIZE;
		if (skipmode > ADJUST_SIZE)
			skipmode = ADJUST_SIZE;
	}

	return skipmode;
}

static void clearbuffer_sdl2(struct sound_data *sd)
{
	struct sound_dp* s = sd->data;
	
	SDL_LockAudioDevice(s->dev);
	memset(paula_sndbuffer, 0, sizeof paula_sndbuffer);
	SDL_UnlockAudioDevice(s->dev);
}

static void clearbuffer(struct sound_data* sd)
{
	auto* s = sd->data;
	if (sd->devicetype == SOUND_DEVICE_SDL2)
		clearbuffer_sdl2(sd);
	if (s->pullbuffer) {
		memset(s->pullbuffer, 0, s->pullbuffermaxlen);
	}
}

static void set_reset(struct sound_data* sd)
{
	sd->reset = true;
	sd->resetcnt = 10;
	sd->resetframecnt = 0;
}

static void pause_audio_sdl2(struct sound_data* sd)
{
	struct sound_dp* s = sd->data;
	
	sd->waiting_for_buffer = 0;
	SDL_PauseAudioDevice(s->dev, 1);
	if (cdda_dev > 0)
		SDL_PauseAudioDevice(cdda_dev, 1);
	clearbuffer(sd);
}
static void resume_audio_sdl2(struct sound_data* sd)
{
	struct sound_dp* s = sd->data;
	
	clearbuffer(sd);
	sd->waiting_for_buffer = 1;
	s->avg_correct = 0;
	s->cnt_correct = 0;
	SDL_PauseAudioDevice(s->dev, 0);
	sd->paused = 0;
	if (cdda_dev > 0)
		SDL_PauseAudioDevice(cdda_dev, 0);
}

static int restore_sdl2(struct sound_data *sd)
{
	pause_audio_sdl2(sd);
	resume_audio_sdl2(sd);
	return 1;
}

static void close_audio_sdl2(struct sound_data* sd)
{
	auto* s = sd->data;
	SDL_PauseAudioDevice(s->dev, 1);
	
	SDL_LockAudioDevice(s->dev);
	if (s->pullbuffer != nullptr)
	{
		xfree(s->pullbuffer);
		s->pullbuffer = NULL;
	}
	s->pullbufferlen = 0;
	SDL_UnlockAudioDevice(s->dev);
	
	SDL_CloseAudioDevice(s->dev);
	if (cdda_dev > 0)
	{
		SDL_PauseAudioDevice(cdda_dev, 1);
		SDL_CloseAudioDevice(cdda_dev);
	}
}

extern void setvolume_ahi(int);

void set_volume_sound_device(struct sound_data* sd, int volume, int mute)
{
	struct sound_dp* s = sd->data;
	if (sd->devicetype == SOUND_DEVICE_SDL2)
	{
		if (volume < 100 && !mute)
			volume = 100 - volume;
		else if (mute || volume >= 100)
			volume = 0;
		//TODO switch to using SDL_mixer to implement volume control properly
		//SDL_MixAudioFormat(reinterpret_cast<uae_u8*>(s->sndbuf), reinterpret_cast<uae_u8*>(s->sndbuf), AUDIO_S16, sd->sndbufsize, volume);
	}
}

void set_volume(int volume, int mute)
{
	set_volume_sound_device(sdp, volume, mute);
	setvolume_ahi(volume);
	config_changed = 1;
}

static void finish_sound_buffer_pull(struct sound_data* sd, uae_u16* sndbuffer)
{
	auto* s = sd->data;

	if (s->pullbufferlen + sd->sndbufsize > s->pullbuffermaxlen) {
		write_log(_T("pull overflow! %d %d %d\n"), s->pullbufferlen, sd->sndbufsize, s->pullbuffermaxlen);
		s->pullbufferlen = 0;
	}
	memcpy(s->pullbuffer + s->pullbufferlen, sndbuffer, sd->sndbufsize);
	s->pullbufferlen += sd->sndbufsize;
}

static int open_audio_sdl2(struct sound_data* sd, int index)
{
	auto* const s = sd->data;
	const auto freq = sd->freq;
	const auto ch = sd->channels;

	sd->devicetype = SOUND_DEVICE_SDL2;
	if (sd->sndbufsize < 0x80)
		sd->sndbufsize = 0x80;
	s->framesperbuffer = sd->sndbufsize;
	s->sndbufsize = s->framesperbuffer;
	sd->sndbufsize = s->sndbufsize * ch * 2;
	if (sd->sndbufsize > SND_MAX_BUFFER)
		sd->sndbufsize = SND_MAX_BUFFER;
	sd->samplesize = ch * 16 / 8;
	s->pullmode = currprefs.sound_pullmode;

	SDL_AudioSpec want, have;
	memset(&want, 0, sizeof want);
	want.freq = freq;
	want.format = AUDIO_S16SYS;
	want.channels = ch;
	want.samples = s->framesperbuffer;

	if (s->pullmode)
	{
		want.callback = sdl2_audio_callback;
		want.userdata = sd;
	}

	s->dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (s->dev == 0)
	{
		write_log("Failed to open audio: %s\n", SDL_GetError());
		return 0;
	}

	if (s->pullmode)
	{
		s->pullbuffermaxlen = sd->sndbufsize * 2;
		s->pullbuffer = xcalloc(uae_u8, s->pullbuffermaxlen);
		s->pullbufferlen = 0;
	}
	clearbuffer(sd);

	return 1;
}

int open_sound_device(struct sound_data* sd, int index, int bufsize, int freq, int channels)
{
	auto* dp = xcalloc(struct sound_dp, 1);

	sd->data = dp;
	sd->sndbufsize = bufsize;
	sd->freq = freq;
	sd->channels = channels;
	sd->paused = 1;
	sd->index = index;
	const auto ret = open_audio_sdl2(sd, index);
	sd->samplesize = sd->channels * 2;
	sd->sndbufframes = sd->sndbufsize / sd->samplesize;
	return ret;
}

void close_sound_device(struct sound_data* sd)
{
	pause_sound_device(sd);
	close_audio_sdl2(sd);
	xfree(sd->data);
	sd->data = NULL;
	sd->index = -1;
}

void pause_sound_device(struct sound_data* sd)
{
	sd->paused = 1;
	gui_data.sndbuf_status = 0;
	gui_data.sndbuf = 0;
	pause_audio_sdl2(sd);
}
void resume_sound_device(struct sound_data* sd)
{
	resume_audio_sdl2(sd);
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
	stereo would have been enabled and we'd have done the shift by
	two anyway).  */
	size >>= 2;
	size &= ~63;

	sdp->softvolume = -1;
	const auto ch = get_audio_nativechannels(currprefs.sound_stereo);
	const auto ret = open_sound_device(sdp, 0, size, currprefs.sound_freq, ch);
	if (!ret)
		return 0;
	currprefs.sound_freq = changed_prefs.sound_freq = sdp->freq;
	if (ch != sdp->channels)
		currprefs.sound_stereo = changed_prefs.sound_stereo = get_audio_stereomode(sdp->channels);

	set_volume(currprefs.sound_volume_master, sdp->mute);
	if (get_audio_amigachannels(currprefs.sound_stereo) == 4)
		sample_handler = sample16ss_handler;
	else
		sample_handler = get_audio_ismono(currprefs.sound_stereo) ? sample16_handler : sample16s_handler;

	sdp->obtainedfreq = currprefs.sound_freq;

	have_sound = 1;
	sound_available = 1;
	gui_data.sndbuf_avail = audio_is_pull() == 0;
	
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
	//restart_sound_buffer2(sdp);
}

static void finish_sound_buffer_sdl2_push(struct sound_data* sd, uae_u16* sndbuffer)
{
	struct sound_dp* s = sd->data;
	SDL_QueueAudio(s->dev, sndbuffer, sd->sndbufsize);
}

static void finish_sound_buffer_sdl2(struct sound_data *sd, uae_u16 *sndbuffer)
{
	auto* s = sd->data;
	if (!sd->waiting_for_buffer)
		return;
	
	if (s->pullmode)
		finish_sound_buffer_pull(sd, sndbuffer);
	else
		finish_sound_buffer_sdl2_push(sd, sndbuffer);
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
	int type = sd->devicetype;
	if (type == SOUND_DEVICE_SDL2) {
		finish_sound_buffer_pull(sd, paula_sndbuffer);
		return true;
	}
	return false;
}

static void send_sound(struct sound_data* sd, uae_u16* sndbuffer)
{
	int type = sd->devicetype;
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
	if (type == SOUND_DEVICE_SDL2)
		finish_sound_buffer_sdl2(sd, sndbuffer);
}

int get_sound_event(void)
{
	int type = sdp->devicetype;
	if (sdp->paused || sdp->deactive)
		return 0;
	//if (type == SOUND_DEVICE_WASAPI || type == SOUND_DEVICE_WASAPI_EXCLUSIVE || type == SOUND_DEVICE_PA) {
	//	struct sound_dp* s = sdp->data;
	//	if (s && s->pullmode) {
	//		return s->pullevent;
	//	}
	//}
	return 0;
}

bool audio_is_event_frame_possible(int)
{
	if (sdp->paused || sdp->deactive || sdp->reset)
		return false;	
	return false;
}

int audio_is_pull()
{
	if (sdp->reset)
		return 0;
	auto* s = sdp->data;
	if (s && s->pullmode) {
		return sdp->paused || sdp->deactive ? -1 : 1;
	}
	return 0;
}

int audio_pull_buffer()
{
	auto cnt = 0;
	if (sdp->paused || sdp->deactive || sdp->reset)
		return 0;
	auto* s = sdp->data;
	if (s->pullbufferlen > 0) {
		cnt++;
		const auto size = reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer);
		if (size > static_cast<long>(sdp->sndbufsize) * 2 / 3)
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
	int type = sdp->devicetype;
	if (sdp->paused || sdp->deactive || sdp->reset)
		return false;
	if (type != SOUND_DEVICE_SDL2)
		return false;
	if (audio_pull_buffer() && audio_is_pull_event()) {
		return send_sound_do(sdp);
	}
	return false;
}

static void handle_reset()
{
	if (sdp->resetframe == timeframes)
		return;
	sdp->resetframe = timeframes;
	sdp->resetframecnt--;
	if (sdp->resetframecnt > 0)
		return;
	sdp->resetframecnt = 20;

	sdp->reset = false;
	if (!reopen_sound() || sdp->reset) {
		if (sdp->resetcnt <= 0) {
			write_log(_T("Reopen sound failed. Retrying with default device.\n"));
			close_sound();
			currprefs.produce_sound = changed_prefs.produce_sound = 1;
		}
		else {
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
	const auto bufsize = reinterpret_cast<uae_u8*>(paula_sndbufpt) - reinterpret_cast<uae_u8*>(paula_sndbuffer);

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
		if (get_audio_nativechannels(currprefs.sound_stereo) == 2 || get_audio_nativechannels(currprefs.sound_stereo) == 4)
			channelswap((uae_s16*)paula_sndbuffer, bufsize / 2);
		else if (get_audio_nativechannels(currprefs.sound_stereo) == 6)
			channelswap6((uae_s16*)paula_sndbuffer, bufsize / 2);
	}
#ifdef DRIVESOUND
	driveclick_mix((uae_s16*)paula_sndbuffer, bufsize / 2, currprefs.dfxclickchannelmask);
#endif
	// must be after driveclick_mix
	paula_sndbufpt = paula_sndbuffer;

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
			memcpy(extrasndbuf + extrasndbuffered, paula_sndbuffer, copied);
			send_sound(sdp, reinterpret_cast<uae_u16*>(extrasndbuf));
			extrasndbuffered = 0;
		}
		memcpy(extrasndbuf + extrasndbuffered, reinterpret_cast<uae_u8*>(paula_sndbuffer) + copied, bufsize - copied);
		extrasndbuffered += bufsize - copied;
	} else {
		send_sound(sdp, paula_sndbuffer);
	}
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
	if (currprefs.sound_volume_master < 0)
		currprefs.sound_volume_master = 0;
	if (currprefs.sound_volume_master > 100)
		currprefs.sound_volume_master = 100;
	changed_prefs.sound_volume_master = currprefs.sound_volume_master;
	if (currprefs.sound_volume_cd < 0)
		currprefs.sound_volume_cd = 0;
	if (currprefs.sound_volume_cd > 100)
		currprefs.sound_volume_cd = 100;
	changed_prefs.sound_volume_cd = currprefs.sound_volume_cd;
	set_volume(currprefs.sound_volume_master, sdp->mute);
	config_changed = 1;
}

void master_sound_volume(int dir)
{
	int vol, mute;

	const auto r = get_master_volume(&vol, &mute);
	if (!r)
		return;
	if (dir == 0)
		mute = mute ? 0 : 1;
	vol += dir * (65536 / 10);
	if (vol < 0)
		vol = 0;
	if (vol > SDL_MIX_MAXVOLUME)
		vol = SDL_MIX_MAXVOLUME;
	set_master_volume(vol, mute);
	config_changed = 1;
}

// Audio callback function
void sdl2_audio_callback(void* userdata, Uint8* stream, int len)
{
	auto* sd = static_cast<sound_data*>(userdata);
	auto* s = sd->data;

	if (!s->framesperbuffer || sdp->deactive)
		return;
	
	if (s->pullbufferlen <= 0)
		return;

	const unsigned int bytes_to_copy = s->framesperbuffer * sd->samplesize;	
	if (bytes_to_copy > 0) {
		memcpy(stream, s->pullbuffer, bytes_to_copy);
	}

	if (bytes_to_copy < s->pullbufferlen) {
		memmove(s->pullbuffer, s->pullbuffer + bytes_to_copy, s->pullbufferlen - static_cast<size_t>(bytes_to_copy));
	}
	s->pullbufferlen -= bytes_to_copy;
}
