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
#include "debug.h"
#include "audio.h"
#include "gensound.h"
#include "sd-pandora/sound.h"
#include "custom.h"
#include <SDL.h>

#ifdef ANDROIDSDL
#include <android/log.h>
#endif

extern unsigned long next_sample_evtime;

int produce_sound=0;
int changed_produce_sound=0;

#define SOUND_USE_SEMAPHORES
#define SOUND_BUFFERS_COUNT 4
uae_u16 sndbuffer[SOUND_BUFFERS_COUNT][(SNDBUFFER_LEN+32)*DEFAULT_SOUND_CHANNELS];
unsigned n_callback_sndbuff, n_render_sndbuff;
uae_u16 *sndbufpt = sndbuffer[0];
uae_u16 *render_sndbuff = sndbuffer[0];
uae_u16 *finish_sndbuff = sndbuffer[0] + SNDBUFFER_LEN*2;

#ifdef NO_SOUND


void finish_sound_buffer (void) {  }

int setup_sound (void) { sound_available = 0; return 0; }

void close_sound (void) { }

void pandora_stop_sound (void) { }

int init_sound (void) { return 0; }

void pause_sound (void) { }

void resume_sound (void) { }

void update_sound (int) { }

void reset_sound (void) { }

void uae4all_init_sound(void) { }

void uae4all_play_click(void) { }

void uae4all_pause_music(void) { }

void uae4all_resume_music(void) { }

void restart_sound_buffer(void) { }

#else 


static int have_sound = 0;
static int lastfreq;

extern uae_u16 new_beamcon0;

void sound_default_evtime(int freq)
{
	int pal = new_beamcon0 & 0x20;

	if (freq < 0)
		freq = lastfreq;
	lastfreq = freq;

#if !( defined(PANDORA) || defined(ANDROIDSDL) )
	switch(m68k_speed)
	{
		case 6:
			scaled_sample_evtime=(unsigned)(MAXHPOS_PAL*MAXVPOS_PAL*VBLANK_HZ_PAL*CYCLE_UNIT/1.86)/sound_rate;
			break;

		case 5:
		case 4: // ~4/3 234
			if (pal)
				scaled_sample_evtime=(MAXHPOS_PAL*244*VBLANK_HZ_PAL*CYCLE_UNIT)/sound_rate; // ???
			else
				scaled_sample_evtime=(MAXHPOS_NTSC*255*VBLANK_HZ_NTSC*CYCLE_UNIT)/sound_rate;
			break;

		case 3:
		case 2: // ~8/7 273
			if (pal)
				scaled_sample_evtime=(MAXHPOS_PAL*270*VBLANK_HZ_PAL*CYCLE_UNIT)/sound_rate;
			else
				scaled_sample_evtime=(MAXHPOS_NTSC*255*VBLANK_HZ_NTSC*CYCLE_UNIT)/sound_rate;
			break;

		case 1:
		default: // MAXVPOS_PAL?
#endif
#if 1
		if (pal)
			scaled_sample_evtime = (MAXHPOS_PAL * MAXVPOS_PAL * freq * CYCLE_UNIT + currprefs.sound_freq - 1) / currprefs.sound_freq;
		else
			scaled_sample_evtime = (MAXHPOS_NTSC * MAXVPOS_NTSC * freq * CYCLE_UNIT + currprefs.sound_freq - 1) / currprefs.sound_freq;
#else
		if (pal)
			scaled_sample_evtime=(MAXHPOS_PAL*313*VBLANK_HZ_PAL*CYCLE_UNIT)/currprefs.sound_freq;
		else
			scaled_sample_evtime=(MAXHPOS_NTSC*MAXVPOS_NTSC*VBLANK_HZ_NTSC*CYCLE_UNIT)/currprefs.sound_freq + 1;
#endif
#if !( defined(PANDORA) || defined(ANDROIDSDL) )
		break;
	}
#endif
}


static int s_oldrate = 0, s_oldbits = 0, s_oldstereo = 0;
static int sound_thread_active = 0, sound_thread_exit = 0;
static sem_t sound_sem, callback_sem;

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
static int cnt = 0;

static void sound_thread_mixer(void *ud, Uint8 *stream, int len)
{
	if (sound_thread_exit) return;
	int sem_val;
	sound_thread_active = 1;
	/*
	// Sound is choppy, arrgh
	sem_getvalue(&sound_sem, &sem_val);
	while (sem_val > 1)
	{
		//printf("skip %i (%i)\n", cnt, sem_val);
		sem_wait(&sound_sem);
		cnt++;
		sem_getvalue(&sound_sem, &sem_val);
	}
	*/

#ifdef SOUND_USE_SEMAPHORES
	sem_wait(&sound_sem);
	sem_post(&callback_sem);
#endif
	cnt++;
	//__android_log_print(ANDROID_LOG_INFO, "UAE4ALL2","Sound callback cnt %d buf %d\n", cnt, cnt%SOUND_BUFFERS_COUNT);
	if(currprefs.sound_stereo)
		memcpy(stream, sndbuffer[cnt%SOUND_BUFFERS_COUNT], MIN(SNDBUFFER_LEN*2, len));
	else
	  	memcpy(stream, sndbuffer[cnt%SOUND_BUFFERS_COUNT], MIN(SNDBUFFER_LEN, len));

}

static int pandora_start_sound(int rate, int bits, int stereo)
{
	int frag = 0, buffers, ret;
	unsigned int bsize;
	static int audioOpened = 0;

	if (!sound_thread_active)
	{
		// init sem, start sound thread
		printf("starting sound thread..\n");
		ret = sem_init(&sound_sem, 0, 0);
		sem_init(&callback_sem, 0, 0);
		if (ret != 0) printf("sem_init() failed: %i, errno=%i\n", ret, errno);
	}

	// if no settings change, we don't need to do anything
	if (rate == s_oldrate && s_oldbits == bits && s_oldstereo == stereo)
	    return 0;

	if( audioOpened ) {
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
	if(currprefs.sound_stereo)
	  as.samples = SNDBUFFER_LEN*2 / as.channels / 2;
	else
	  as.samples = SNDBUFFER_LEN / as.channels / 2;
	as.callback = sound_thread_mixer;
	SDL_OpenAudio(&as, NULL);
	audioOpened = 1;

	s_oldrate = rate; 
	s_oldbits = bits; 
	s_oldstereo = stereo;

	SDL_PauseAudio (0);

	return 0;
}


// this is meant to be called only once on exit
void pandora_stop_sound(void)
{
	if (sound_thread_exit)
		printf("don't call pandora_stop_sound more than once!\n");
	if (sound_thread_active)
	{
		printf("stopping sound thread..\n");
		sound_thread_exit = 1;
		sem_post(&sound_sem);
		//usleep(100*1000);
	}
	SDL_PauseAudio (1);
}

static int wrcnt = 0;
void finish_sound_buffer (void)
{

#ifdef DEBUG_SOUND
	dbg("sound.c : finish_sound_buffer");
#endif

	//printf("finish %i\n", wrcnt);
#if 0
	if(currprefs.sound_stereo)
	write(sounddev, sndbuffer[0], SNDBUFFER_LEN*2);
	else
	write(sounddev, sndbuffer[0], SNDBUFFER_LEN);  
	sndbufpt = render_sndbuff = sndbuffer[0];
#else

#ifdef SOUND_USE_SEMAPHORES
	sem_post(&sound_sem);
	sem_wait(&callback_sem);
#endif
	wrcnt++;
	sndbufpt = render_sndbuff = sndbuffer[wrcnt%SOUND_BUFFERS_COUNT];
	//__android_log_print(ANDROID_LOG_INFO, "UAE4ALL2","Sound buffer write cnt %d buf %d\n", wrcnt, wrcnt%SOUND_BUFFERS_COUNT);
#endif

	if(currprefs.sound_stereo)
	  finish_sndbuff = sndbufpt + SNDBUFFER_LEN;
	else
	  finish_sndbuff = sndbufpt + SNDBUFFER_LEN/2;	

#ifdef DEBUG_SOUND
	dbg(" sound.c : ! finish_sound_buffer");
#endif
}


void restart_sound_buffer(void)
{
	sndbufpt = render_sndbuff = sndbuffer[wrcnt&3];
	if(currprefs.sound_stereo)
	  finish_sndbuff = sndbufpt + SNDBUFFER_LEN;
	else
	  finish_sndbuff = sndbufpt + SNDBUFFER_LEN/2;	  
}

/* Try to determine whether sound is available.  This is only for GUI purposes.  */
int setup_sound (void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : setup_sound");
#endif

     // Android does not like opening sound device several times
     if (pandora_start_sound(currprefs.sound_freq, 16, currprefs.sound_stereo) != 0)
        return 0;

     sound_available = 1;

#ifdef DEBUG_SOUND
    dbg(" sound.c : ! setup_sound");
#endif
    return 1;
}

void update_sound (int freq)
{
  sound_default_evtime(freq);
}


static int open_sound (void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : open_sound");
#endif

	// Android does not like opening sound device several times
    if (pandora_start_sound(currprefs.sound_freq, 16, currprefs.sound_stereo) != 0)
	    return 0;

    init_sound_table16 ();

    have_sound = 1;
    sound_available = 1;

    if(currprefs.sound_stereo)
      sample_handler = sample16s_handler;
    else
      sample_handler = sample16_handler;
 

#ifdef DEBUG_SOUND
    dbg(" sound.c : ! open_sound");
#endif
    return 1;
}

void close_sound (void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : close_sound");
#endif
    if (!have_sound)
	return;

    // testing shows that reopenning sound device is not a good idea (causes random sound driver crashes)
    // we will close it on real exit instead
#ifdef RASPBERRY
    //pandora_stop_sound();
#endif
    have_sound = 0;

#ifdef DEBUG_SOUND
    dbg(" sound.c : ! close_sound");
#endif
}

int init_sound (void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : init_sound");
#endif

    have_sound=open_sound();

#ifdef DEBUG_SOUND
    dbg(" sound.c : ! init_sound");
#endif
    return have_sound;
}

void pause_sound (void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : pause_sound");
#endif

	SDL_PauseAudio (1);
    /* nothing to do */

#ifdef DEBUG_SOUND
    dbg(" sound.c : ! pause_sound");
#endif
}

void resume_sound (void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : resume_sound");
#endif

	SDL_PauseAudio (0);
    /* nothing to do */

#ifdef DEBUG_SOUND
    dbg(" sound.c : ! resume_sound");
#endif
}

void uae4all_init_sound(void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : uae4all_init_sound");
#endif
#ifdef DEBUG_SOUND
    dbg(" sound.c : ! uae4all_init_sound");
#endif
}

void uae4all_pause_music(void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : pause_music");
#endif
#ifdef DEBUG_SOUND
    dbg(" sound.c : ! pause_music");
#endif
}

void uae4all_resume_music(void)
{
#ifdef DEBUG_SOUND
    dbg("sound.c : resume_music");
#endif
#ifdef DEBUG_SOUND
    dbg(" sound.c : ! resume_music");
#endif
}

void uae4all_play_click(void)
{
}

void reset_sound (void)
{
  if (!have_sound)
  	return;

  memset(sndbuffer, 0, 2 * 4 * (SNDBUFFER_LEN+32)*DEFAULT_SOUND_CHANNELS);
}


void sound_volume (int dir)
{
}

#endif

