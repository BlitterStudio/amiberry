 /* 
  * Sdl sound.c implementation
  * (c) 2015
  */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "audio.h"
#include "gensound.h"
#include "sd-pandora/sound.h"
#include <SDL.h>

#ifdef ANDROID
#include <android/log.h>
#endif

uae_u16 sndbuffer[SOUND_BUFFERS_COUNT][(SNDBUFFER_LEN + 32)*DEFAULT_SOUND_CHANNELS];
unsigned n_callback_sndbuff, n_render_sndbuff;
uae_u16 *sndbufpt = sndbuffer[0];
uae_u16 *render_sndbuff = sndbuffer[0];
uae_u16 *finish_sndbuff = sndbuffer[0] + SNDBUFFER_LEN * DEFAULT_SOUND_CHANNELS;

uae_u16 cdaudio_buffer[CDAUDIO_BUFFERS][(CDAUDIO_BUFFER_LEN + 32) * DEFAULT_SOUND_CHANNELS];
uae_u16 *cdbufpt = cdaudio_buffer[0];
uae_u16 *render_cdbuff = cdaudio_buffer[0];
uae_u16 *finish_cdbuff = cdaudio_buffer[0] + CDAUDIO_BUFFER_LEN * DEFAULT_SOUND_CHANNELS;
bool cdaudio_active = false;
static int cdwrcnt = 0;
static int cdrdcnt = 0;

extern int screen_is_picasso;

#ifdef NO_SOUND

void finish_sound_buffer(void) {}

int setup_sound(void) { sound_available = 0; return 0; }

void close_sound(void) {}

void pandora_stop_sound(void) {}

int init_sound(void) { return 0; }

void pause_sound(void) {}

void resume_sound(void) {}

void update_sound(int) {}

void reset_sound(void) {}

void restart_sound_buffer(void) {}

#else 

static int have_sound = 0;
static int lastfreq;

void update_sound(float clk)
{
	float evtime;
  
	evtime = clk * CYCLE_UNIT / (float)currprefs.sound_freq;
	scaled_sample_evtime = (int)evtime;
}


static int s_oldrate = 0, s_oldbits = 0, s_oldstereo = 0;
static int sound_thread_active = 0, sound_thread_exit = 0;
static sem_t sound_sem, sound_out_sem;
static int output_cnt = 0;
static int wrcnt = 0;

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

static void sound_thread_mixer(void *ud, Uint8 *stream, int len)
{
	int cnt = 0, sem_val = 0;
	sound_thread_active = 1;
	
	sem_getvalue(&sound_sem, &sem_val);
	
	while (sem_val > 1)
	{
		sem_wait(&sound_sem);
		sem_getvalue(&sound_sem, &sem_val);
	}

	sem_wait(&sound_sem);
	
	if (sound_thread_exit) 
		return;

	cnt = output_cnt;
	sem_post(&sound_out_sem);
    
	if (currprefs.sound_stereo) {
		if (cdaudio_active && currprefs.sound_freq == 44100 && cdrdcnt < cdwrcnt) {
			for (int i = 0; i < SNDBUFFER_LEN * 2; ++i)
				sndbuffer[cnt & 3][i] += cdaudio_buffer[cdrdcnt & (CDAUDIO_BUFFERS - 1)][i];
			cdrdcnt++;
		}
		memcpy(stream, sndbuffer[cnt % SOUND_BUFFERS_COUNT], MIN(SNDBUFFER_LEN * 2, len));
	}
	else
		memcpy(stream, sndbuffer[cnt % SOUND_BUFFERS_COUNT], MIN(SNDBUFFER_LEN, len));

}


static void init_soundbuffer_usage(void)
{
	sndbufpt = sndbuffer[0];
	render_sndbuff = sndbuffer[0];
	finish_sndbuff = sndbuffer[0] + SNDBUFFER_LEN * 2;
	output_cnt = 0;
	wrcnt = 0;
  
	cdbufpt = cdaudio_buffer[0];
	render_cdbuff = cdaudio_buffer[0];
	finish_cdbuff = cdaudio_buffer[0] + CDAUDIO_BUFFER_LEN * 2;
	cdrdcnt = 0;
	cdwrcnt = 0;
}


static int pandora_start_sound(int rate, int bits, int stereo)
{
	int frag = 0, buffers, ret;
	unsigned int bsize;
	static int audioOpened = 0;

	if (!sound_thread_active)
	{
		// init sem, start sound thread
#ifdef DEBUG
		printf("starting sound thread..\n");
#endif		init_soundbuffer_usage();
		ret = sem_init(&sound_sem, 0, 0);
		sem_init(&sound_out_sem, 0, 0);
		if (ret != 0) printf("sem_init() failed: %i, errno=%i\n", ret, errno);
	}

	// if no settings change, we don't need to do anything
	if (rate == s_oldrate && s_oldbits == bits && s_oldstereo == stereo)
		return 0;

	if (audioOpened) {
		// __android_log_print(ANDROID_LOG_INFO, "UAE4ALL2", "UAE tries to open SDL sound device 2 times, ignoring that.");
		//	SDL_CloseAudio();
		return 0;
	}

	SDL_AudioSpec as;
	memset(&as, 0, sizeof(as));

	// __android_log_print(ANDROID_LOG_INFO, "UAE4ALL2", "Opening audio: rate %d bits %d stereo %d", rate, bits, stereo);
	as.freq = rate;
	as.format = (bits == 8 ? AUDIO_S8 : AUDIO_S16);
	as.channels = (stereo ? 2 : 1);
	if (currprefs.sound_stereo)
		as.samples = SNDBUFFER_LEN * 2 / as.channels / 2;
	else
		as.samples = SNDBUFFER_LEN / as.channels / 2;
	as.callback = sound_thread_mixer;
	SDL_OpenAudio(&as, NULL);
	audioOpened = 1;

	s_oldrate = rate; 
	s_oldbits = bits; 
	s_oldstereo = stereo;

	SDL_PauseAudio(0);

	return 0;
}


// this is meant to be called only once on exit
void pandora_stop_sound(void)
{
	if (sound_thread_exit)
		printf("don't call pandora_stop_sound more than once!\n");
	if (sound_thread_active)
	{
#ifdef DEBUG
		printf("stopping sound thread..\n");
#endif
		sound_thread_exit = 1;
		sem_post(&sound_sem);
	}
	SDL_PauseAudio(1);
}

void finish_sound_buffer(void)
{
	output_cnt = wrcnt;

	sem_post(&sound_sem);
	sem_wait(&sound_out_sem);
  
	wrcnt++;
	sndbufpt = render_sndbuff = sndbuffer[wrcnt & 3];
	if (currprefs.sound_stereo)
		finish_sndbuff = sndbufpt + SNDBUFFER_LEN;
	else
		finish_sndbuff = sndbufpt + SNDBUFFER_LEN / 2;	  
}

void pause_sound_buffer(void)
{
	reset_sound();
}

void restart_sound_buffer(void)
{
	sndbufpt = render_sndbuff = sndbuffer[wrcnt & 3];
	if (currprefs.sound_stereo)
		finish_sndbuff = sndbufpt + SNDBUFFER_LEN;
	else
		finish_sndbuff = sndbufpt + SNDBUFFER_LEN / 2;
	
	cdbufpt = render_cdbuff = cdaudio_buffer[cdwrcnt & (CDAUDIO_BUFFERS - 1)];
	finish_cdbuff = cdbufpt + CDAUDIO_BUFFER_LEN * 2;
}

void finish_cdaudio_buffer(void)
{
	cdwrcnt++;
	cdbufpt = render_cdbuff = cdaudio_buffer[cdwrcnt & (CDAUDIO_BUFFERS - 1)];
	finish_cdbuff = cdbufpt + CDAUDIO_BUFFER_LEN;
	audio_activate();
}


bool cdaudio_catchup(void)
{
	while ((cdwrcnt > cdrdcnt + CDAUDIO_BUFFERS - 10) && (sound_thread_active != 0) && (quit_program == 0)) {
		sleep_millis(10);
	}
	return (sound_thread_active != 0);
}

/* Try to determine whether sound is available.  This is only for GUI purposes.  */
int setup_sound(void)
{
	if (pandora_start_sound(currprefs.sound_freq, 16, currprefs.sound_stereo) != 0)
		return 0;

	sound_available = 1;
	return 1;
}

static int open_sound(void)
{
	if (pandora_start_sound(currprefs.sound_freq, 16, currprefs.sound_stereo) != 0)
		return 0;

	have_sound = 1;
	sound_available = 1;

	if (currprefs.sound_stereo)
		sample_handler = sample16s_handler;
	else
		sample_handler = sample16_handler;
 
	return 1;
}

void close_sound(void)
{
	if (!have_sound)
		return;

		  // testing shows that reopenning sound device is not a good idea on pandora (causes random sound driver crashes)
		  // we will close it on real exit instead
		  //pandora_stop_sound();
	have_sound = 0;
}

int init_sound(void)
{
	have_sound = open_sound();
	return have_sound;
}

void pause_sound(void)
{
	SDL_PauseAudio(1);
    /* nothing to do */
}

void resume_sound(void)
{
	SDL_PauseAudio(0);
    /* nothing to do */
}

void reset_sound(void)
{
	if (!have_sound)
		return;

	init_soundbuffer_usage();

	clear_sound_buffers();
	clear_cdaudio_buffers();
}

void sound_volume(int dir)
{
}

#endif

