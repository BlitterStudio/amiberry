 /* 
  * Minimalistic sound.c implementation for gp2x
  * (c) notaz, 2007
  */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>

#include "sysconfig.h"
#include "sysdeps.h"
#include "uae.h"
#include "options.h"
#include "memory.h"
#include "audio.h"
#include "gensound.h"
#include "sounddep/sound.h"
#include "custom.h"
#include "savestate.h"


uae_u16 sndbuffer[4][(SNDBUFFER_LEN+32)*DEFAULT_SOUND_CHANNELS];
uae_u16 *sndbufpt = sndbuffer[0];
uae_u16 *render_sndbuff = sndbuffer[0];
uae_u16 *finish_sndbuff = sndbuffer[0] + SNDBUFFER_LEN*2;

static int have_sound = 0;
static int lastfreq;

extern uae_u16 new_beamcon0;

static __inline__ void sound_default_evtime(int freq)
{
	int pal = new_beamcon0 & 0x20;

  if (freq < 0)
  	freq = lastfreq;
  lastfreq = freq;

#ifndef PANDORA
	switch(m68k_speed)
	{
		case 6:
			scaled_sample_evtime=(unsigned)(MAXHPOS_PAL*MAXVPOS_PAL*VBLANK_HZ_PAL*CYCLE_UNIT/1.86)/currprefs.sound_freq;
			break;

		case 5:
		case 4: // ~4/3 234
			if (pal)
				scaled_sample_evtime=(MAXHPOS_PAL*244*VBLANK_HZ_PAL*CYCLE_UNIT)/currprefs.sound_freq; // ???
			else
				scaled_sample_evtime=(MAXHPOS_NTSC*255*VBLANK_HZ_NTSC*CYCLE_UNIT)/currprefs.sound_freq;
			break;

		case 3:
		case 2: // ~8/7 273
			if (pal)
				scaled_sample_evtime=(MAXHPOS_PAL*270*VBLANK_HZ_PAL*CYCLE_UNIT)/currprefs.sound_freq;
			else
				scaled_sample_evtime=(MAXHPOS_NTSC*255*VBLANK_HZ_NTSC*CYCLE_UNIT)/currprefs.sound_freq;
			break;

		case 1:
		default: // MAXVPOS_PAL?
#endif
			if (pal)
				scaled_sample_evtime = (MAXHPOS_PAL * MAXVPOS_PAL * freq * CYCLE_UNIT + currprefs.sound_freq - 1) / currprefs.sound_freq;
			else
				scaled_sample_evtime = (MAXHPOS_NTSC * MAXVPOS_NTSC * freq * CYCLE_UNIT + currprefs.sound_freq - 1) / currprefs.sound_freq;
#ifndef PANDORA
			break;
	}
#endif
}


static int sounddev = -1, s_oldrate = 0, s_oldbits = 0, s_oldstereo = 0;
static int sound_thread_active = 0, sound_thread_exit = 0;
static sem_t sound_sem;
static sem_t sound_out_sem;
static int output_cnt = 0;

static void *sound_thread(void *unused)
{
	int cnt = 0, sem_val = 0;
	sound_thread_active = 1;

	for (;;)
	{
		sem_getvalue(&sound_sem, &sem_val);
		while (sem_val > 1)
		{
			sem_wait(&sound_sem);
			sem_getvalue(&sound_sem, &sem_val);
		}

		sem_wait(&sound_sem);
		if (sound_thread_exit) 
		  break;

    cnt = output_cnt;
    sem_post(&sound_out_sem);
    
		if(currprefs.sound_stereo)
		  write(sounddev, sndbuffer[cnt&3], SNDBUFFER_LEN*2);
		else
		  write(sounddev, sndbuffer[cnt&3], SNDBUFFER_LEN);
	}

  sound_thread_active = 0;
  sem_post(&sound_out_sem);
	return NULL;
}

static int pandora_start_sound(int rate, int bits, int stereo)
{
	int frag = 0, buffers, ret;
	unsigned int bsize;

	if (!sound_thread_active)
	{
		// init sem, start sound thread
		pthread_t thr;

    sndbufpt = sndbuffer[0];
    render_sndbuff = sndbuffer[0];
    finish_sndbuff = sndbuffer[0] + SNDBUFFER_LEN*2;
    s_oldrate = 0;
    s_oldbits = 0;
    s_oldstereo = 0;

		sound_thread_exit = 0;
		ret = sem_init(&sound_sem, 0, 0);
		if (ret != 0) 
		  write_log("sem_init() failed: %i, errno=%i\n", ret, errno);
		sem_init(&sound_out_sem, 0, 0);
		ret = pthread_create(&thr, NULL, sound_thread, NULL);
		if (ret != 0) 
		  write_log("pthread_create() failed: %i\n", ret);
		pthread_detach(thr);
	}

	if (sounddev <= 0)
	{
		sounddev = open("/dev/dsp", O_WRONLY);
		if (sounddev == -1)
		{
			write_log("open(\"/dev/dsp\") failed with %i\n", errno);
			return -1;
		}
	}

	// if no settings change, we don't need to do anything
	if (rate == s_oldrate && s_oldbits == bits && s_oldstereo == stereo) 
	  return 0;

	ioctl(sounddev, SNDCTL_DSP_SPEED,  &rate);
	ioctl(sounddev, SNDCTL_DSP_SETFMT, &bits);
	ioctl(sounddev, SNDCTL_DSP_STEREO, &stereo);
	// calculate buffer size
	buffers = 16;
	bsize = rate / 32;
	if (rate > 22050) { bsize*=4; buffers*=2; } // 44k mode seems to be very demanding
	while ((bsize>>=1)) frag++;
	frag |= buffers<<16; // 16 buffers
	ioctl(sounddev, SNDCTL_DSP_SETFRAGMENT, &frag);

	s_oldrate = rate; 
	s_oldbits = bits; 
	s_oldstereo = stereo;
	usleep(100000);
	return 0;
}


// this is meant to be called only once on exit
void pandora_stop_sound(void)
{
	if (sound_thread_exit)
		printf("don't call pandora_stop_sound more than once!\n");
	if (sound_thread_active)
	{
		sound_thread_exit = 1;
		sem_post(&sound_sem);
		sem_wait(&sound_out_sem);
		sem_destroy(&sound_sem);
		sem_destroy(&sound_out_sem);
	}

	if (sounddev > 0)
		close(sounddev);
	sounddev = -1;
}


static int wrcnt = 0;

void finish_sound_buffer (void)
{
  output_cnt = wrcnt;
	sem_post(&sound_sem);
	sem_wait(&sound_out_sem);
	wrcnt++;
	sndbufpt = render_sndbuff = sndbuffer[wrcnt&3];
	if(currprefs.sound_stereo)
	  finish_sndbuff = sndbufpt + SNDBUFFER_LEN;
	else
	  finish_sndbuff = sndbufpt + SNDBUFFER_LEN/2;	  
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
  if (pandora_start_sound(currprefs.sound_freq, 16, currprefs.sound_stereo) != 0)
    return 0;

  sound_available = 1;
  return 1;
}

void update_sound (int freq)
{
  sound_default_evtime(freq);
}

static int open_sound (void)
{
  if (pandora_start_sound(currprefs.sound_freq, 16, currprefs.sound_stereo) != 0)
	    return 0;

  init_sound_table16 ();

  have_sound = 1;
  sound_available = 1;

  if(currprefs.sound_stereo)
    sample_handler = sample16s_handler;
  else
    sample_handler = sample16_handler;
 
  return 1;
}

void close_sound (void)
{
  if (!have_sound)
	  return;

  // testing shows that reopenning sound device is not a good idea on pandora (causes random sound driver crashes)
  // we will close it on real exit instead
  //pandora_stop_sound();
  have_sound = 0;
}

int init_sound (void)
{
    have_sound=open_sound();
    return have_sound;
}

void pause_sound (void)
{
    /* nothing to do */
}

void resume_sound (void)
{
    /* nothing to do */
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
