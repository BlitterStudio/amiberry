 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Paula audio emulation
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  * Copyright 1996 Marcus Sundberg
  * Copyright 1996 Manfred Thole
  * Copyright 2006 Toni Wilen
  *
  * new filter algorithm and anti&sinc interpolators by Antti S. Lankila
  *
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "include/memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "gensound.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "events.h"
#include "savestate.h"
#include "gui.h"

#include <math.h>

#define MAX_EV ~0u

#define PERIOD_MIN 4
#define PERIOD_MIN_NONCE 60

STATIC_INLINE bool isaudio(void)
{
  return currprefs.produce_sound != 0;
}

STATIC_INLINE bool usehacks (void)
{
	return currprefs.cpu_model >= 68020 || currprefs.m68k_speed != 0;
}

#define SINC_QUEUE_MAX_AGE 2048
/* Queue length 256 implies minimum emulated period of 8. This should be
 * sufficient for all imaginable purposes. This must be power of two. */
#define SINC_QUEUE_LENGTH 256

#include "sinctable.cpp"

typedef struct {
	int time, output;
} sinc_queue_t;

struct audio_channel_data{
  unsigned int adk_mask;
  unsigned int evtime;
	bool dmaenstore;
	bool intreq2;
	bool dr;
	bool dsr;
	bool pbufldl;
	bool dat_written;
  uaecptr lc, pt;
  int current_sample, last_sample;
  int state;
  int per;
  int vol;
  int len, wlen;
  uae_u16 dat, dat2;
  int sample_accum, sample_accum_time;
  int sinc_output_state;
  sinc_queue_t sinc_queue[SINC_QUEUE_LENGTH];
  int sinc_queue_time;
  int sinc_queue_head;
	/* too fast cpu fixes */
	uaecptr ptx;
	bool ptx_written;
	bool ptx_tofetch;
	int dmaofftime_active;
};

static struct audio_channel_data audio_channel[4];
int sound_available = 0;
void (*sample_handler) (void);
static void (*sample_prehandler) (unsigned long best_evtime);

unsigned long scaled_sample_evtime;

static unsigned long last_cycles;
static unsigned long next_sample_evtime;

typedef uae_s8 sample8_t;
#define DO_CHANNEL_1(v, c) do { (v) *= audio_channel[c].vol; } while (0)

STATIC_INLINE int FINISH_DATA (int data, int bits)
{
	if (bits == 16) {
		return data;
	} else if (bits - 16 > 0) {
		data >>=  bits - 16;
	} else {
		int shift = 16 - bits;
		data <<= shift;
	}
	return data;
}

static uae_u32 right_word_saved[SOUND_MAX_DELAY_BUFFER];
static uae_u32 left_word_saved[SOUND_MAX_DELAY_BUFFER];
static int saved_ptr;

static int mixed_on, mixed_stereo_size, mixed_mul1, mixed_mul2;
static int led_filter_forced, sound_use_filter, sound_use_filter_sinc, led_filter_on;

/* denormals are very small floating point numbers that force FPUs into slow
   mode. All lowpass filters using floats are suspectible to denormals unless
   a small offset is added to avoid very small floating point numbers. */
#define DENORMAL_OFFSET (1E-10)

static struct filter_state {
    float rc1, rc2, rc3, rc4, rc5;
} sound_filter_state[2];

static float a500e_filter1_a0;
static float a500e_filter2_a0;
static float filter_a0; /* a500 and a1200 use the same */

enum {
  FILTER_NONE = 0,
  FILTER_MODEL_A500,
  FILTER_MODEL_A1200
};

/* Amiga has two separate filtering circuits per channel, a static RC filter
 * on A500 and the LED filter. This code emulates both.
 * 
 * The Amiga filtering circuitry depends on Amiga model. Older Amigas seem
 * to have a 6 dB/oct RC filter with cutoff frequency such that the -6 dB
 * point for filter is reached at 6 kHz, while newer Amigas have no filtering.
 *
 * The LED filter is complicated, and we are modelling it with a pair of
 * RC filters, the other providing a highboost. The LED starts to cut
 * into signal somewhere around 5-6 kHz, and there's some kind of highboost
 * in effect above 12 kHz. Better measurements are required.
 *
 * The current filtering should be accurate to 2 dB with the filter on,
 * and to 1 dB with the filter off.
*/

static int filter(int input, struct filter_state *fs)
{
  int o;
  float normal_output, led_output;

  input = (uae_s16)input;
  switch (sound_use_filter) {
    	
    case FILTER_MODEL_A500: 
    	fs->rc1 = a500e_filter1_a0 * input + (1 - a500e_filter1_a0) * fs->rc1 + DENORMAL_OFFSET;
    	fs->rc2 = a500e_filter2_a0 * fs->rc1 + (1-a500e_filter2_a0) * fs->rc2;
    	normal_output = fs->rc2;

    	fs->rc3 = filter_a0 * normal_output + (1 - filter_a0) * fs->rc3;
    	fs->rc4 = filter_a0 * fs->rc3       + (1 - filter_a0) * fs->rc4;
    	fs->rc5 = filter_a0 * fs->rc4       + (1 - filter_a0) * fs->rc5;

    	led_output = fs->rc5;
      break;
        
    case FILTER_MODEL_A1200:
      normal_output = input;

      fs->rc2 = filter_a0 * normal_output + (1 - filter_a0) * fs->rc2 + DENORMAL_OFFSET;
      fs->rc3 = filter_a0 * fs->rc2       + (1 - filter_a0) * fs->rc3;
      fs->rc4 = filter_a0 * fs->rc3       + (1 - filter_a0) * fs->rc4;

      led_output = fs->rc4;
      break;
	case FILTER_NONE:
	default:
		return input;

  }

  if (led_filter_on) 
  	o = led_output;
  else
	  o = normal_output;

  if (o > 32767)
	  o = 32767;
  else if (o < -32768)
	  o = -32768;

  return o;
}

/* Always put the right word before the left word.  */

static void (*put_sound_word_mono_func)(uae_u32 w);
static void (*put_sound_word_stereo_func)(uae_u32 left, uae_u32 right);

static void put_sound_word_stereo_func_filter_mixed(uae_u32 left, uae_u32 right)
{
	uae_u32 rold, lold, tmp;

  left = filter (left, &sound_filter_state[0]); 
  right = filter (right, &sound_filter_state[1]); 

  left_word_saved[saved_ptr] = left;
  right_word_saved[saved_ptr] = right;

  saved_ptr = (saved_ptr + 1) & mixed_stereo_size;

  lold = left_word_saved[saved_ptr];
  tmp = (right * mixed_mul2 + lold * mixed_mul1) / MIXED_STEREO_SCALE;

  rold = right_word_saved[saved_ptr];
  left = (left * mixed_mul2 + rold * mixed_mul1) / MIXED_STEREO_SCALE;

  PUT_SOUND_WORD_STEREO(left, tmp);
}

static void put_sound_word_stereo_func_filter_notmixed(uae_u32 left, uae_u32 right)
{
  left = filter (left, &sound_filter_state[0]); 
  right = filter (right, &sound_filter_state[1]); 
  PUT_SOUND_WORD_STEREO(left, right);
}

static void put_sound_word_stereo_func_nofilter_mixed(uae_u32 left, uae_u32 right)
{
	uae_u32 rold, lold, tmp;

  left_word_saved[saved_ptr] = left;
  right_word_saved[saved_ptr] = right;

  saved_ptr = (saved_ptr + 1) & mixed_stereo_size;

  lold = left_word_saved[saved_ptr];
  tmp = (right * mixed_mul2 + lold * mixed_mul1) / MIXED_STEREO_SCALE;

  rold = right_word_saved[saved_ptr];
  left = (left * mixed_mul2 + rold * mixed_mul1) / MIXED_STEREO_SCALE;

  PUT_SOUND_WORD_STEREO(left, tmp);
}

static void put_sound_word_stereo_func_nofilter_notmixed(uae_u32 left, uae_u32 right)
{
  PUT_SOUND_WORD_STEREO(left, right);
}

static void put_sound_word_mono_func_filter(uae_u32 data)
{
  data = filter (data, &sound_filter_state[0]); 
  PUT_SOUND_WORD(data);
}

static void put_sound_word_mono_func_nofilter(uae_u32 data)
{
  PUT_SOUND_WORD(data);
}


static void anti_prehandler(unsigned long best_evtime)
{
  int i, output;
  struct audio_channel_data *acd;

  /* Handle accumulator antialiasiation */
  for (i = 0; i < 4; i++) {
  	acd = &audio_channel[i];
  	output = (acd->current_sample * acd->vol) & acd->adk_mask;
  	acd->sample_accum += output * best_evtime;
  	acd->sample_accum_time += best_evtime;
  }
}

STATIC_INLINE void samplexx_anti_handler (int *datasp)
{
  int i;
  for (i = 0; i < 4; i++) {
  	datasp[i] = audio_channel[i].sample_accum_time ? (audio_channel[i].sample_accum / audio_channel[i].sample_accum_time) : 0;
    audio_channel[i].sample_accum = 0;
	  audio_channel[i].sample_accum_time = 0;
  }
}

static void sinc_prehandler(unsigned long best_evtime)
{
	int i, output;
  struct audio_channel_data *acd;

  for (i = 0; i < 4; i++) {
  	acd = &audio_channel[i];
		int vol = acd->vol;
		output = (acd->current_sample * vol) & acd->adk_mask;
         
    /* if output state changes, record the state change and also
     * write data into sinc queue for mixing in the BLEP */
    if (acd->sinc_output_state != output) {
			acd->sinc_queue_head = (acd->sinc_queue_head - 1) & (SINC_QUEUE_LENGTH - 1);
			acd->sinc_queue[acd->sinc_queue_head].time = acd->sinc_queue_time;
			acd->sinc_queue[acd->sinc_queue_head].output = output - acd->sinc_output_state;
      acd->sinc_output_state = output;
    }

		acd->sinc_queue_time += best_evtime;
  }
}


/* this interpolator performs BLEP mixing (bleps are shaped like integrated sinc
 * functions) with a type of BLEP that matches the filtering configuration. */
STATIC_INLINE void samplexx_sinc_handler (int *datasp)
{
  int i, n;
  int const *winsinc;

  if (sound_use_filter_sinc) {
  	n = (sound_use_filter_sinc == FILTER_MODEL_A500) ? 0 : 2;
    if (led_filter_on)
      n += 1;
  } else {
  	n = 4;
  }
  winsinc = winsinc_integral[n];

  for (i = 0; i < 4; i += 1) {
    int j, v;
    struct audio_channel_data *acd = &audio_channel[i];
    /* The sum rings with harmonic components up to infinity... */
  	int sum = acd->sinc_output_state << 17;
    /* ...but we cancel them through mixing in BLEPs instead */
		int offsetpos = acd->sinc_queue_head & (SINC_QUEUE_LENGTH - 1);
		for (j = 0; j < SINC_QUEUE_LENGTH; j += 1) {
			int age = acd->sinc_queue_time - acd->sinc_queue[offsetpos].time;
			if (age >= SINC_QUEUE_MAX_AGE || age < 0)
				break;
			sum -= winsinc[age] * acd->sinc_queue[offsetpos].output;
			offsetpos = (offsetpos + 1) & (SINC_QUEUE_LENGTH - 1);
		}
		v = sum >> 15;
  	if (v > 32767)
	    v = 32767;
	  else if (v < -32768)
	    v = -32768;
	  datasp[i] = v;
  }
}

static void sample16i_sinc_handler (void)
{
  int datas[4], data1;
  
  samplexx_sinc_handler (datas);
  data1 = datas[0] + datas[3] + datas[1] + datas[2];
	data1 = FINISH_DATA (data1, 18);
	set_sound_buffers ();
  PUT_SOUND_WORD_MONO (data1);
  check_sound_buffers ();
}

void sample16_handler (void)
{
	uae_u32 data;
  if(audio_channel[0].adk_mask)
    data = audio_channel[0].current_sample * audio_channel[0].vol;
  else
    data = 0;
  if(audio_channel[1].adk_mask)
    data += audio_channel[1].current_sample * audio_channel[1].vol;
  if(audio_channel[2].adk_mask)
    data += audio_channel[2].current_sample * audio_channel[2].vol;
  if(audio_channel[3].adk_mask)
    data += audio_channel[3].current_sample * audio_channel[3].vol;
    
	data = FINISH_DATA (data, 16);
	set_sound_buffers ();
  PUT_SOUND_WORD_MONO (data);
  check_sound_buffers ();
}
   
/* This interpolator examines sample points when Paula switches the output
 * voltage and computes the average of Paula's output */
static void sample16i_anti_handler (void)
{
  int datas[4], data1;

  samplexx_anti_handler (datas);
  data1 = datas[0] + datas[3] + datas[1] + datas[2];
	data1 = FINISH_DATA (data1, 16);
	set_sound_buffers ();
  PUT_SOUND_WORD_MONO (data1);
  check_sound_buffers ();
}

static void sample16i_rh_handler (void)
{
  unsigned long delta, ratio;

	uae_u32 data0 = audio_channel[0].current_sample;
	uae_u32 data1 = audio_channel[1].current_sample;
	uae_u32 data2 = audio_channel[2].current_sample;
	uae_u32 data3 = audio_channel[3].current_sample;
  uae_u32 data0p = audio_channel[0].last_sample;
  uae_u32 data1p = audio_channel[1].last_sample;
  uae_u32 data2p = audio_channel[2].last_sample;
  uae_u32 data3p = audio_channel[3].last_sample;
	uae_u32 data;

  DO_CHANNEL_1 (data0, 0);
  DO_CHANNEL_1 (data1, 1);
  DO_CHANNEL_1 (data2, 2);
  DO_CHANNEL_1 (data3, 3);
  DO_CHANNEL_1 (data0p, 0);
  DO_CHANNEL_1 (data1p, 1);
  DO_CHANNEL_1 (data2p, 2);
  DO_CHANNEL_1 (data3p, 3);

	data0 &= audio_channel[0].adk_mask;
  data0p &= audio_channel[0].adk_mask;
	data1 &= audio_channel[1].adk_mask;
  data1p &= audio_channel[1].adk_mask;
	data2 &= audio_channel[2].adk_mask;
  data2p &= audio_channel[2].adk_mask;
	data3 &= audio_channel[3].adk_mask;
  data3p &= audio_channel[3].adk_mask;

  /* linear interpolation and summing up... */
  delta = audio_channel[0].per;
  ratio = ((audio_channel[0].evtime % delta) << 8) / delta;
  data0 = (data0 * (256 - ratio) + data0p * ratio) >> 8;
  delta = audio_channel[1].per;
  ratio = ((audio_channel[1].evtime % delta) << 8) / delta;
  data0 += (data1 * (256 - ratio) + data1p * ratio) >> 8;
  delta = audio_channel[2].per;
  ratio = ((audio_channel[2].evtime % delta) << 8) / delta;
  data0 += (data2 * (256 - ratio) + data2p * ratio) >> 8;
  delta = audio_channel[3].per;
  ratio = ((audio_channel[3].evtime % delta) << 8) / delta;
  data0 += (data3 * (256 - ratio) + data3p * ratio) >> 8;
	data = data0;
	data = FINISH_DATA (data, 16);
	set_sound_buffers ();
  PUT_SOUND_WORD_MONO (data);
  check_sound_buffers();
}

static void sample16i_crux_handler (void)
{
  uae_u32 data0 = audio_channel[0].current_sample;
  uae_u32 data1 = audio_channel[1].current_sample;
  uae_u32 data2 = audio_channel[2].current_sample;
  uae_u32 data3 = audio_channel[3].current_sample;
  uae_u32 data0p = audio_channel[0].last_sample;
  uae_u32 data1p = audio_channel[1].last_sample;
  uae_u32 data2p = audio_channel[2].last_sample;
  uae_u32 data3p = audio_channel[3].last_sample;
	uae_u32 data;

  DO_CHANNEL_1 (data0, 0);
  DO_CHANNEL_1 (data1, 1);
  DO_CHANNEL_1 (data2, 2);
  DO_CHANNEL_1 (data3, 3);
  DO_CHANNEL_1 (data0p, 0);
  DO_CHANNEL_1 (data1p, 1);
  DO_CHANNEL_1 (data2p, 2);
  DO_CHANNEL_1 (data3p, 3);

  data0 &= audio_channel[0].adk_mask;
  data0p &= audio_channel[0].adk_mask;
  data1 &= audio_channel[1].adk_mask;
  data1p &= audio_channel[1].adk_mask;
  data2 &= audio_channel[2].adk_mask;
  data2p &= audio_channel[2].adk_mask;
  data3 &= audio_channel[3].adk_mask;
  data3p &= audio_channel[3].adk_mask;

  {    
    struct audio_channel_data *cdp;
    unsigned long ratio, ratio1;
#define INTERVAL (scaled_sample_evtime * 3)
    cdp = audio_channel + 0;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data0 = (data0 * ratio + data0p * (4096 - ratio)) >> 12;

  	cdp = audio_channel + 1;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data1 = (data1 * ratio + data1p * (4096 - ratio)) >> 12;

    cdp = audio_channel + 2;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data2 = (data2 * ratio + data2p * (4096 - ratio)) >> 12;

    cdp = audio_channel + 3;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data3 = (data3 * ratio + data3p * (4096 - ratio)) >> 12;
  }
  data1 += data2;
  data0 += data3;
  data0 += data1;
	data = data0;
	data = FINISH_DATA (data, 16);
	set_sound_buffers ();
  PUT_SOUND_WORD_MONO (data);
  check_sound_buffers ();
}

#ifdef HAVE_STEREO_SUPPORT

/* This interpolator examines sample points when Paula switches the output
 * voltage and computes the average of Paula's output */

static void sample16si_anti_handler (void)
{
  int datas[4], data1, data2;

  samplexx_anti_handler (datas);
  data1 = datas[0] + datas[3];
  data2 = datas[1] + datas[2];
  data1 = FINISH_DATA (data1, 15);
  data2 = FINISH_DATA (data2, 15);
	set_sound_buffers ();
  put_sound_word_stereo_func(data1, data2);
  check_sound_buffers ();
}

static void sample16si_sinc_handler (void)
{
  int datas[4], data1, data2;

  samplexx_sinc_handler (datas);
  data1 = datas[0] + datas[3];
  data2 = datas[1] + datas[2];
  data1 = FINISH_DATA (data1, 17);
  data2 = FINISH_DATA (data2, 17);
	set_sound_buffers ();
  put_sound_word_stereo_func(data1, data2);
  check_sound_buffers ();
}

void sample16s_handler (void)
{
  uae_u32 data_l = audio_channel[0].adk_mask ? audio_channel[0].current_sample * audio_channel[0].vol : 0;
  uae_u32 data_r = audio_channel[1].adk_mask ? audio_channel[1].current_sample * audio_channel[1].vol : 0;
  if(audio_channel[2].adk_mask)
    data_r += audio_channel[2].current_sample * audio_channel[2].vol;
  if(audio_channel[3].adk_mask)
    data_l += audio_channel[3].current_sample * audio_channel[3].vol;
  data_l = FINISH_DATA(data_l, 15);
  data_r = FINISH_DATA(data_r, 15);

	set_sound_buffers ();
  put_sound_word_stereo_func(data_l, data_r);
  check_sound_buffers();
}

static void sample16si_crux_handler (void)
{
  uae_u32 data0 = audio_channel[0].current_sample;
  uae_u32 data1 = audio_channel[1].current_sample;
  uae_u32 data2 = audio_channel[2].current_sample;
  uae_u32 data3 = audio_channel[3].current_sample;
  uae_u32 data0p = audio_channel[0].last_sample;
  uae_u32 data1p = audio_channel[1].last_sample;
  uae_u32 data2p = audio_channel[2].last_sample;
  uae_u32 data3p = audio_channel[3].last_sample;

  DO_CHANNEL_1 (data0, 0);
  DO_CHANNEL_1 (data1, 1);
  DO_CHANNEL_1 (data2, 2);
  DO_CHANNEL_1 (data3, 3);
  DO_CHANNEL_1 (data0p, 0);
  DO_CHANNEL_1 (data1p, 1);
  DO_CHANNEL_1 (data2p, 2);
  DO_CHANNEL_1 (data3p, 3);

  data0 &= audio_channel[0].adk_mask;
  data0p &= audio_channel[0].adk_mask;
  data1 &= audio_channel[1].adk_mask;
  data1p &= audio_channel[1].adk_mask;
  data2 &= audio_channel[2].adk_mask;
  data2p &= audio_channel[2].adk_mask;
  data3 &= audio_channel[3].adk_mask;
  data3p &= audio_channel[3].adk_mask;

  {    
    struct audio_channel_data *cdp;
    unsigned long ratio, ratio1;
#define INTERVAL (scaled_sample_evtime * 3)
    cdp = audio_channel + 0;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
      data0 = (data0 * ratio + data0p * (4096 - ratio)) >> 12;

    cdp = audio_channel + 1;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data1 = (data1 * ratio + data1p * (4096 - ratio)) >> 12;

    cdp = audio_channel + 2;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data2 = (data2 * ratio + data2p * (4096 - ratio)) >> 12;

    cdp = audio_channel + 3;
    ratio1 = cdp->per - cdp->evtime;
    ratio = (ratio1 << 12) / INTERVAL;
    if (cdp->evtime < scaled_sample_evtime || ratio1 >= INTERVAL)
	    ratio = 4096;
    data3 = (data3 * ratio + data3p * (4096 - ratio)) >> 12;
  }
  data1 += data2;
  data0 += data3;
  data0 = FINISH_DATA (data0, 15);
  data1 = FINISH_DATA (data1, 15);
	set_sound_buffers ();
  put_sound_word_stereo_func(data0, data1);
  check_sound_buffers ();
}

static void sample16si_rh_handler (void)
{
  unsigned long delta, ratio;

  uae_u32 data0 = audio_channel[0].current_sample;
  uae_u32 data1 = audio_channel[1].current_sample;
  uae_u32 data2 = audio_channel[2].current_sample;
  uae_u32 data3 = audio_channel[3].current_sample;
  uae_u32 data0p = audio_channel[0].last_sample;
  uae_u32 data1p = audio_channel[1].last_sample;
  uae_u32 data2p = audio_channel[2].last_sample;
  uae_u32 data3p = audio_channel[3].last_sample;

  DO_CHANNEL_1 (data0, 0);
  DO_CHANNEL_1 (data1, 1);
  DO_CHANNEL_1 (data2, 2);
  DO_CHANNEL_1 (data3, 3);
  DO_CHANNEL_1 (data0p, 0);
  DO_CHANNEL_1 (data1p, 1);
  DO_CHANNEL_1 (data2p, 2);
  DO_CHANNEL_1 (data3p, 3);

  data0 &= audio_channel[0].adk_mask;
  data0p &= audio_channel[0].adk_mask;
  data1 &= audio_channel[1].adk_mask;
  data1p &= audio_channel[1].adk_mask;
  data2 &= audio_channel[2].adk_mask;
  data2p &= audio_channel[2].adk_mask;
  data3 &= audio_channel[3].adk_mask;
  data3p &= audio_channel[3].adk_mask;

  /* linear interpolation and summing up... */
  delta = audio_channel[0].per;
  ratio = ((audio_channel[0].evtime % delta) << 8) / delta;
  data0 = (data0 * (256 - ratio) + data0p * ratio) >> 8;
  delta = audio_channel[1].per;
  ratio = ((audio_channel[1].evtime % delta) << 8) / delta;
  data1 = (data1 * (256 - ratio) + data1p * ratio) >> 8;
  delta = audio_channel[2].per;
  ratio = ((audio_channel[2].evtime % delta) << 8) / delta;
  data1 += (data2 * (256 - ratio) + data2p * ratio) >> 8;
  delta = audio_channel[3].per;
  ratio = ((audio_channel[3].evtime % delta) << 8) / delta;
  data0 += (data3 * (256 - ratio) + data3p * ratio) >> 8;
  data0 = FINISH_DATA (data0, 15);
  data1 = FINISH_DATA (data1, 15);
	set_sound_buffers ();
  put_sound_word_stereo_func(data0, data1);
  check_sound_buffers ();
}

#else
void sample16s_handler (void)
{
	sample16_handler ();
}
static void sample16si_crux_handler (void)
{
	sample16i_crux_handler ();
}
static void sample16si_rh_handler (void)
{
	sample16i_rh_handler ();
}
#endif

static int audio_work_to_do;

static void zerostate (int nr)
{
	struct audio_channel_data *cdp = audio_channel + nr;
  cdp->state = 0;
  cdp->evtime = MAX_EV;
	cdp->intreq2 = 0;
	cdp->dmaenstore = false;
	cdp->dmaofftime_active = 0;
}

static void schedule_audio (void)
{
	unsigned long best = MAX_EV;
  int i;

	eventtab[ev_audio].active = 0;
  for (i = 0; i < 4; i++) {
  	struct audio_channel_data *cdp = audio_channel + i;
		if (cdp->evtime != MAX_EV) {
			if (best > cdp->evtime) {
				best = cdp->evtime;
        eventtab[ev_audio].active = 1;
      }
    }
  }

	eventtab[ev_audio].evtime = get_cycles () + best;
}

static void audio_event_reset (void)
{
	int i;

	last_cycles = get_cycles ();
	next_sample_evtime = scaled_sample_evtime;
	if (!isrestore ()) {
	  for (i = 0; i < 4; i++)
			zerostate (i);
	}
	schedule_audio ();
	events_schedule ();
}

void audio_deactivate(void)
{
  gui_data.sndbuf_status = 3;
  gui_data.sndbuf = 0;
	audio_work_to_do = 0;
	pause_sound_buffer ();
  clear_sound_buffers();
  audio_event_reset();
}

int audio_activate(void)
{
  int ret = 0;
  if (!audio_work_to_do) {
	  restart_sound_buffer();
	  ret = 1;
	  audio_event_reset();
  }
  audio_work_to_do = 4 * maxvpos_nom * 50;
  return ret;
}
STATIC_INLINE int is_audio_active(void)
{
  return audio_work_to_do;
}

uae_u16 audio_dmal (void)
{
	uae_u16 dmal = 0;
	for (int nr = 0; nr < 4; nr++) {
		struct audio_channel_data *cdp = audio_channel + nr;
		if (cdp->dr)
			dmal |= 1 << (nr * 2);
		if (cdp->dsr)
			dmal |= 1 << (nr * 2 + 1);
		cdp->dr = cdp->dsr = false;
	}
	return dmal;
}

static int isirq (int nr)
{
  return INTREQR() & (0x80 << nr);
}

static void setirq (int nr, int which)
{
  INTREQ_0 (0x8000 | (0x80 << nr));
}

static void newsample (int nr, sample8_t sample)
{
	struct audio_channel_data *cdp = audio_channel + nr;
  cdp->last_sample = cdp->current_sample;
  cdp->current_sample = sample;
}    

STATIC_INLINE void setdr (int nr)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	cdp->dr = true;
	if (cdp->wlen == 1) {
		cdp->dsr = true;
	}
}

static void loaddat (int nr, bool modper)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	int audav = adkcon & (0x01 << nr);
	int audap = adkcon & (0x10 << nr);
	if (audav || (modper && audap)) {
		if (nr >= 3)
			return;
		if (modper && audap) {
			if (cdp->dat == 0)
				cdp[1].per = PERIOD_MAX;
			else if (cdp->dat > PERIOD_MIN)
				cdp[1].per = cdp->dat * CYCLE_UNIT;
			else
				cdp[1].per = PERIOD_MIN * CYCLE_UNIT;
		} else	if (audav) {
			cdp[1].vol = cdp->dat;
			cdp[1].vol &= 127;
			if (cdp[1].vol > 64)
				cdp[1].vol = 64;
		}
	} else {
		cdp->dat2 = cdp->dat;
	}
}
static void loaddat (int nr)
{
	loaddat (nr, false);
}

STATIC_INLINE void loadper (int nr)
{
	struct audio_channel_data *cdp = audio_channel + nr;

	cdp->evtime = cdp->per;
	if (cdp->evtime < CYCLE_UNIT)
		write_log (_T("LOADPER%d bug %d\n"), nr, cdp->evtime);
}


static void audio_state_channel2 (int nr, bool perfin)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	bool chan_ena = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));
	bool old_dma = cdp->dmaenstore;
	int audav = adkcon & (0x01 << nr);
	int audap = adkcon & (0x10 << nr);
	int napnav = (!audav && !audap) || audav;
	int hpos = current_hpos ();

	cdp->dmaenstore = chan_ena;

	if (currprefs.produce_sound == 0) {
		zerostate (nr);
		return;
	}
	audio_activate ();

	if ((cdp->state == 2 || cdp->state == 3) && usehacks()) {
    if (!chan_ena && old_dma) {
		// DMA switched off, state=2/3 and "too fast CPU":  set flag
			cdp->dmaofftime_active = true;
		}
		if (cdp->dmaofftime_active && !old_dma && chan_ena) {
			// We are still in state=2/3 and program is going to re-enable
			// DMA. Force state to zero to prevent CPU timed DMA wait
			// routines in common tracker players to lose notes.
		  newsample (nr, (cdp->dat2 >> 0) & 0xff);
//		  if (napnav)
//		  	setirq (nr, 91);
		  zerostate (nr);
		}
	}

	switch (cdp->state)
	{
	case 0:
		if (chan_ena) {
			cdp->evtime = MAX_EV;
			cdp->state = 1;
			cdp->dr = true;
			cdp->wlen = cdp->len;
			cdp->ptx_written = false;
			/* Some programs first start short empty sample and then later switch to
			 * real sample, we must not enable the hack in this case
			 */
			if (cdp->wlen > 2)
				cdp->ptx_tofetch = true;
			cdp->dsr = true;
		} else if (cdp->dat_written && !isirq (nr)) {
			cdp->state = 2;
			setirq (nr, 0);
			loaddat (nr);
			if (usehacks() && cdp->per < 10 * CYCLE_UNIT) {
				// make sure audio.device AUDxDAT startup returns to idle state before DMA is enabled
				newsample (nr, (cdp->dat2 >> 0) & 0xff);
				zerostate (nr);
			} else {
				cdp->pbufldl = true;
				audio_state_channel2 (nr, false);
			}
		} else {
			zerostate (nr);
		}
		break;
	case 1:
		cdp->evtime = MAX_EV;
		if (!chan_ena) {
			zerostate (nr);
			return;
		}
		if (!cdp->dat_written)
			return;
		setirq (nr, 10);
		setdr (nr);
		if (cdp->wlen != 1)
			cdp->wlen = (cdp->wlen - 1) & 0xffff;
		cdp->state = 5;
		break;
	case 5:
		cdp->evtime = MAX_EV;
		if (!chan_ena) {
			zerostate (nr);
			return;
		}
		if (!cdp->dat_written)
			return;
		if (cdp->ptx_written) {
			cdp->ptx_written = 0;
			cdp->lc = cdp->ptx;
		}
		loaddat (nr);
		if (napnav)
			setdr (nr);
		cdp->state = 2;
		loadper (nr);
		cdp->pbufldl = true;
		cdp->intreq2 = 0;
		audio_state_channel2 (nr, false);
		break;
	case 2:
		if (cdp->pbufldl) {
			newsample (nr, (cdp->dat2 >> 8) & 0xff);
			loadper (nr);
			cdp->pbufldl = false;
		}
		if (!perfin)
			return;
		if (audap)
			loaddat (nr, true);
		if (chan_ena) {
			if (audap)
				setdr (nr);
			if (cdp->intreq2 && audap)
				setirq (nr, 21);
		} else {
			if (audap)
				setirq (nr, 22);
		}
		cdp->pbufldl = true;
		cdp->state = 3;
		audio_state_channel2 (nr, false);
		break;
	case 3:
		if (cdp->pbufldl) {
			newsample (nr, (cdp->dat2 >> 0) & 0xff);
			loadper (nr);
			cdp->pbufldl = false;
		}
		if (!perfin)
			return;
		if (chan_ena) {
			loaddat (nr);
			if (cdp->intreq2 && napnav)
				setirq (nr, 31);
			if (napnav)
				setdr (nr);
		} else {
			if (isirq (nr)) {
				zerostate (nr);
				return;
			}
			loaddat (nr);
			if (napnav)
				setirq (nr, 32);
		}
		cdp->intreq2 = 0;
		cdp->pbufldl = true;
		cdp->state = 2;
		audio_state_channel2 (nr, false);
		break;
	}
}

static void audio_state_channel (int nr, bool perfin)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	audio_state_channel2 (nr, perfin);
	cdp->dat_written = false;
}

void audio_state_machine (void)
{
	update_audio ();
	for (int nr = 0; nr < 4; nr++) {
		struct audio_channel_data *cdp = audio_channel + nr;
		audio_state_channel2 (nr, false);
		cdp->dat_written = false;
	}
	schedule_audio ();
	events_schedule ();
}

void audio_reset (void)
{
  int i;
  struct audio_channel_data *cdp;

  reset_sound ();
  memset(sound_filter_state, 0, sizeof sound_filter_state);
	if (!isrestore ()) {
	  for (i = 0; i < 4; i++) {
	    cdp = &audio_channel[i];
	    memset (cdp, 0, sizeof *audio_channel);
	    cdp->per = PERIOD_MAX - 1;
	    cdp->vol = 0;
	    cdp->evtime = MAX_EV;
    }
  }

	last_cycles = get_cycles ();
  next_sample_evtime = scaled_sample_evtime;

  schedule_audio ();
  events_schedule ();
}

static int sound_prefs_changed (void)
{
  if (changed_prefs.produce_sound != currprefs.produce_sound
    || changed_prefs.sound_stereo != currprefs.sound_stereo
    || changed_prefs.sound_freq != currprefs.sound_freq)
    return 1;

  if (changed_prefs.sound_stereo_separation != currprefs.sound_stereo_separation
    || changed_prefs.sound_mixed_stereo_delay != currprefs.sound_mixed_stereo_delay
    || changed_prefs.sound_interpol != currprefs.sound_interpol
    || changed_prefs.sound_filter != currprefs.sound_filter
    || changed_prefs.sound_filter_type != currprefs.sound_filter_type)
    return -1;
  return 0;
}

/* This computes the 1st order low-pass filter term b0.
 * The a1 term is 1.0 - b0. The center frequency marks the -3 dB point. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static float rc_calculate_a0(int sample_rate, int cutoff_freq)
{
  float omega;
  /* The BLT correction formula below blows up if the cutoff is above nyquist. */
  if (cutoff_freq >= sample_rate / 2)
    return 1.0;

  omega = 2 * M_PI * cutoff_freq / sample_rate;
  /* Compensate for the bilinear transformation. This allows us to specify the
   * stop frequency more exactly, but the filter becomes less steep further
   * from stopband. */
  omega = tan(omega / 2) * 2;
  return 1 / (1 + 1 / omega);
}

void check_prefs_changed_audio (void)
{
  int ch;

 	if (sound_available) {
		ch = sound_prefs_changed ();
		if (ch > 0) {
			clear_sound_buffers ();
		}
		if (ch) {
			set_audio ();
			audio_activate ();
		}
	}
}

void set_audio(void)
{
  int old_mixed_size = mixed_stereo_size;
  int sep, delay;
  int ch;

  ch = sound_prefs_changed ();
  if (ch >= 0)
	  close_sound ();

	currprefs.produce_sound = changed_prefs.produce_sound;
	currprefs.sound_stereo = changed_prefs.sound_stereo;
	currprefs.sound_freq = changed_prefs.sound_freq;

  currprefs.sound_stereo_separation = changed_prefs.sound_stereo_separation;
  currprefs.sound_mixed_stereo_delay = changed_prefs.sound_mixed_stereo_delay;
	currprefs.sound_interpol = changed_prefs.sound_interpol;
  currprefs.sound_filter = changed_prefs.sound_filter;
  currprefs.sound_filter_type = changed_prefs.sound_filter_type;

  if (ch >= 0) {
	  if (currprefs.produce_sound >= 2) {
      if (!init_audio ()) {
	      if (! sound_available) {
	          write_log (_T("Sound is not supported.\n"));
	      } else {
	        write_log (_T("Sorry, can't initialize sound.\n"));
	        currprefs.produce_sound = 1;
	        /* So we don't do this every frame */
	        changed_prefs.produce_sound = 1;
	      }
      }
	  }
	  next_sample_evtime = scaled_sample_evtime;
    last_cycles = get_cycles ();
	  compute_vsynctime ();
  } else {
    sound_volume (0);
  }

  sep = (currprefs.sound_stereo_separation = changed_prefs.sound_stereo_separation) * 3 / 2;
  if (sep >= 15)
	  sep = 16;
  delay = currprefs.sound_mixed_stereo_delay = changed_prefs.sound_mixed_stereo_delay;
  mixed_mul1 = MIXED_STEREO_SCALE / 2 - sep;
  mixed_mul2 = MIXED_STEREO_SCALE / 2 + sep;
	mixed_stereo_size = delay > 0 ? (1 << delay) - 1 : 0;
	mixed_on = sep < MIXED_STEREO_MAX || mixed_stereo_size > 0;
  if (mixed_on && old_mixed_size != mixed_stereo_size) {
  	saved_ptr = 0;
  	memset (right_word_saved, 0, sizeof right_word_saved);
  }

  led_filter_forced = -1; // always off
  sound_use_filter = sound_use_filter_sinc = 0;
  if (currprefs.sound_filter) {
  	if (currprefs.sound_filter == FILTER_SOUND_ON)
	    led_filter_forced = 1;
	  if (currprefs.sound_filter == FILTER_SOUND_EMUL)
	    led_filter_forced = 0;
	  if (currprefs.sound_filter_type == FILTER_SOUND_TYPE_A500)
	    sound_use_filter = FILTER_MODEL_A500;
	  else if (currprefs.sound_filter_type == FILTER_SOUND_TYPE_A1200)
	    sound_use_filter = FILTER_MODEL_A1200;
  }
  a500e_filter1_a0 = rc_calculate_a0(currprefs.sound_freq, 6200);
  a500e_filter2_a0 = rc_calculate_a0(currprefs.sound_freq, 20000);
  filter_a0 = rc_calculate_a0(currprefs.sound_freq, 7000);
  led_filter_audio();

  /* Select the right interpolation method.  */
  if (sample_handler == sample16_handler
	|| sample_handler == sample16i_crux_handler
	|| sample_handler == sample16i_rh_handler	
	|| sample_handler == sample16i_sinc_handler
	|| sample_handler == sample16i_anti_handler)
	{
  	sample_handler = (currprefs.sound_interpol == 0 ? sample16_handler
		  : currprefs.sound_interpol == 3 ? sample16i_rh_handler
		  : currprefs.sound_interpol == 4 ? sample16i_crux_handler
		  : currprefs.sound_interpol == 2 ? sample16i_sinc_handler
		  : sample16i_anti_handler);
  } else if (sample_handler == sample16s_handler
     || sample_handler == sample16si_crux_handler
     || sample_handler == sample16si_rh_handler
     || sample_handler == sample16si_sinc_handler
     || sample_handler == sample16si_anti_handler)
  {
  	sample_handler = (currprefs.sound_interpol == 0 ? sample16s_handler
		  : currprefs.sound_interpol == 3 ? sample16si_rh_handler
		  : currprefs.sound_interpol == 4 ? sample16si_crux_handler
		  : currprefs.sound_interpol == 2 ? sample16si_sinc_handler
		  : sample16si_anti_handler);
  }
  sample_prehandler = NULL;
  if (sample_handler == sample16si_sinc_handler || sample_handler == sample16i_sinc_handler) {
  	sample_prehandler = sinc_prehandler;
	  sound_use_filter_sinc = sound_use_filter;
	  sound_use_filter = 0;
  } else if (sample_handler == sample16si_anti_handler || sample_handler == sample16i_anti_handler) {
	  sample_prehandler = anti_prehandler;
  }

  if(currprefs.sound_stereo) {
    if(currprefs.sound_filter) {
      if(mixed_on)
        put_sound_word_stereo_func = put_sound_word_stereo_func_filter_mixed;
      else
        put_sound_word_stereo_func = put_sound_word_stereo_func_filter_notmixed;
    }
    else { 
      if(mixed_on)
        put_sound_word_stereo_func = put_sound_word_stereo_func_nofilter_mixed;
      else
        put_sound_word_stereo_func = put_sound_word_stereo_func_nofilter_notmixed;
    }
  }
  else {
    if(currprefs.sound_filter) {
      put_sound_word_mono_func = put_sound_word_mono_func_filter;
    }
    else { 
      put_sound_word_mono_func = put_sound_word_mono_func_nofilter;
    }
  }
 
  if (currprefs.produce_sound == 0) {
	  eventtab[ev_audio].active = 0;
		events_schedule ();
	} else {
		audio_activate ();
		schedule_audio ();
	  events_schedule ();
  }
}

void update_audio (void)
{
  unsigned long int n_cycles = 0;

  if (!isaudio())
  	goto end;
  if (isrestore ())
  	goto end;
  if (!is_audio_active())
  	goto end;

  n_cycles = get_cycles () - last_cycles;
  while (n_cycles > 0) {
		unsigned long int best_evtime = n_cycles;
	  unsigned long rounded;
    int i;

  	for (i = 0; i < 4; i++) {
	    if (audio_channel[i].evtime != MAX_EV && best_evtime > audio_channel[i].evtime)
		    best_evtime = audio_channel[i].evtime;
	  }

    rounded = next_sample_evtime;

  	if (currprefs.produce_sound > 1 && best_evtime > rounded)
	    best_evtime = rounded;
 
	  /* Decrease time-to-wait counters */
    next_sample_evtime -= best_evtime;

  	if (sample_prehandler && (currprefs.produce_sound > 1)) {
  		sample_prehandler(best_evtime / CYCLE_UNIT);
    }

	  for (i = 0; i < 4; i++) {
      if (audio_channel[i].evtime != MAX_EV)
	  	  audio_channel[i].evtime -= best_evtime;
	  }

  	n_cycles -= best_evtime;

    /* Test if new sample needs to be outputted */
  	if ((rounded == best_evtime) && (currprefs.produce_sound > 1)) {
  		next_sample_evtime += scaled_sample_evtime;
      (*sample_handler) ();
  	}

  	for (i = 0; i < 4; i++) {
			if (audio_channel[i].evtime == 0) {
				audio_state_channel (i, true);
				if (audio_channel[i].evtime == 0) {
					write_log (_T("evtime==0 sound bug channel %d\n"), i);
					audio_channel[i].evtime = MAX_EV;
				}
			}
  	}
	}
end:
	last_cycles = get_cycles () - n_cycles;
}

void audio_evhandler (void)
{
	update_audio ();
	schedule_audio ();
}

void audio_hsync (void)
{
  if (!isaudio())
    return;
	if (audio_work_to_do > 0) {
	  audio_work_to_do--;
	  if (audio_work_to_do == 0)
	    audio_deactivate();
  }

  update_audio();
}

void AUDxDAT (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	int chan_ena = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));

	cdp->dat = v;
	cdp->dat_written = true;
	if (cdp->state == 2 || cdp->state == 3) {
		if (chan_ena) {
			if (cdp->wlen == 1) {
				cdp->wlen = cdp->len;
				cdp->intreq2 = true;
			} else {
				cdp->wlen = (cdp->wlen - 1) & 0xffff;
			}
		}
	} else {
		audio_activate ();
		update_audio ();
		audio_state_channel (nr, false);
		schedule_audio ();
		events_schedule ();
	}
	cdp->dat_written = false;
}

void audio_dmal_do (int nr, bool reset)
{
	struct audio_channel_data *cdp = audio_channel + nr;
  uae_u16 dat = chipmem_wget_indirect (cdp->pt);
  cdp->pt += 2;
	if (reset)
		cdp->pt = cdp->lc;
	cdp->ptx_tofetch = false;
	AUDxDAT (nr, dat);
}

void AUDxLCH (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
  audio_activate();
  update_audio ();
	// someone wants to update PT but DSR has not yet been processed.
	// too fast CPU and some tracker players: enable DMA, CPU delay, update AUDxPT with loop position
	if (usehacks() && ((cdp->ptx_tofetch && cdp->state == 1) || cdp->ptx_written)) {
		cdp->ptx = cdp->lc;
		cdp->ptx_written = true;
	} else {
    cdp->lc = (cdp->lc & 0xffff) | ((uae_u32)v << 16);
  }
}

void AUDxLCL (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
  audio_activate();
  update_audio ();
	if (usehacks() && ((cdp->ptx_tofetch && cdp->state == 1) || cdp->ptx_written)) {
		cdp->ptx = cdp->lc;
		cdp->ptx_written = true;
	} else {
    cdp->lc = (cdp->lc & ~0xffff) | (v & 0xFFFE);
  }
}

void AUDxPER (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	unsigned long per;

  audio_activate();
  update_audio ();

	per = v * CYCLE_UNIT;
  if (per == 0)
	  per = PERIOD_MAX - 1;

	if (per < PERIOD_MIN * CYCLE_UNIT) {
    /* smaller values would cause extremely high cpu usage */
		per = PERIOD_MIN * CYCLE_UNIT;
	}
	if (per < PERIOD_MIN_NONCE * CYCLE_UNIT && cdp->dmaenstore) {
		/* DMAL emulation and low period can cause very very high cpu usage on slow performance PCs
		 * Only do this hack if audio DMA is active.
		 */
		per = PERIOD_MIN_NONCE * CYCLE_UNIT;
	}

  if (cdp->per == PERIOD_MAX - 1 && per != PERIOD_MAX - 1) {
   	cdp->evtime = CYCLE_UNIT;
   	if (isaudio()) {
      schedule_audio ();
      events_schedule ();
   	}
  }
  cdp->per = per;
}

void AUDxLEN (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
  audio_activate();
  update_audio ();
  cdp->len = v;
}

void AUDxVOL (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	 // 7 bit register in Paula.
	v &= 127;
	if (v > 64)
		v = 64;
  audio_activate();
  update_audio ();
  cdp->vol = v;
}

void audio_update_adkmasks (void)
{
  static int prevcon = -1;
  unsigned long t = adkcon | (adkcon >> 4);

  audio_channel[0].adk_mask = (((t >> 0) & 1) - 1);
  audio_channel[1].adk_mask = (((t >> 1) & 1) - 1);
  audio_channel[2].adk_mask = (((t >> 2) & 1) - 1);
  audio_channel[3].adk_mask = (((t >> 3) & 1) - 1);
  if ((prevcon & 0xff) != (adkcon & 0xff)) {
	  audio_activate();
	  prevcon = adkcon;
  }
}

int init_audio (void)
{
  return init_sound ();
}

void led_filter_audio (void)
{
  led_filter_on = 0;
  if (led_filter_forced > 0 || (gui_data.powerled && led_filter_forced >= 0))
  	led_filter_on = 1;
}

void restore_audio_finish (void)
{
	last_cycles = get_cycles ();
	schedule_audio ();
	events_schedule ();
}

uae_u8 *restore_audio (int nr, uae_u8 *src)
{
  struct audio_channel_data *acd = audio_channel + nr;

	zerostate (nr);
  acd->state = restore_u8 ();
  acd->vol = restore_u8 ();
	acd->intreq2 = restore_u8 () ? true : false;
	uae_u8 flags = restore_u8 ();
	acd->dr = acd->dsr = false;
	if (flags & 1)
		acd->dr = true;
	if (flags & 2)
		acd->dsr = true;
  acd->len = restore_u16 ();
  acd->wlen = restore_u16 ();
	uae_u16 p = restore_u16 ();
  acd->per = p ? p * CYCLE_UNIT : PERIOD_MAX;
	acd->dat = acd->dat2 = restore_u16 ();
  acd->lc = restore_u32 ();
  acd->pt = restore_u32 ();
  acd->evtime = restore_u32 ();
	acd->dmaenstore = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));
  return src;
}

uae_u8 *save_audio (int nr, int *len, uae_u8 *dstptr)
{
	struct audio_channel_data *acd = audio_channel + nr;
  uae_u8 *dst, *dstbak;

  if (dstptr)
  	dstbak = dst = dstptr;
  else
		dstbak = dst = xmalloc (uae_u8, 100);
	save_u8 (acd->state);
  save_u8 (acd->vol);
  save_u8 (acd->intreq2);
	save_u8 ((acd->dr ? 1 : 0) | (acd->dsr ? 2 : 0) | 0x80);
  save_u16 (acd->len);
  save_u16 (acd->wlen);
	save_u16 (acd->per == PERIOD_MAX ? 0 : acd->per / CYCLE_UNIT);
	save_u16 (acd->dat);
  save_u32 (acd->lc);
  save_u32 (acd->pt);
  save_u32 (acd->evtime);
  *len = dst - dstbak;
  return dstbak;
}
