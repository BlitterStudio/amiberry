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
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "autoconf.h"
#include "gensound.h"
#include "audio.h"
#include "sounddep/sound.h"
#include "events.h"
#include "savestate.h"
#ifdef DRIVESOUND
#include "driveclick.h"
#endif
#include "zfile.h"
#include "uae.h"
#include "gui.h"
#include "xwin.h"
#include "debug.h"
#ifdef WITH_SNDBOARD
#include "sndboard.h"
#endif
#ifdef AVIOUTPUT
#include "avioutput.h"
#endif
#ifdef AHI
#include "traps.h"
#include "ahi_v1.h"
#ifdef AHI_v2
#include "ahi_v2.h"
#endif
#endif
#include "threaddep/thread.h"

#include <math.h>

#define DEBUG_AUDIO 0
#define DEBUG_AUDIO2 0
#define DEBUG_AUDIO_HACK 0
#define DEBUG_CHANNEL_MASK 15
#define TEST_AUDIO 0
#define TEST_MISSED_DMA 0
#define TEST_MANUAL_AUDIO 0

#define PERIOD_MIN 1
#define PERIOD_MIN_NONCE 60
#define PERIOD_MIN_LOOP 16
#define PERIOD_MIN_LOOP_COUNT 64

#define PERIOD_LOW 124

int audio_channel_mask = 15;
volatile bool cd_audio_mode_changed;

STATIC_INLINE bool isaudio (void)
{
	return currprefs.produce_sound != 0;
}

#if DEBUG_AUDIO > 0 || DEBUG_AUDIO_HACK > 0 || DEBUG_AUDIO2 > 0
static bool debugchannel (int ch)
{
	return ((1 << ch) & DEBUG_CHANNEL_MASK) != 0;
}
#endif

STATIC_INLINE bool usehacks(void)
{
	return !(currprefs.cs_hacks & 8) && (currprefs.cpu_model >= 68020 || currprefs.m68k_speed != 0 || (currprefs.cs_hacks & 4));
}

#define SINC_QUEUE_MAX_AGE 2048
/* Queue length 256 implies minimum emulated period of 8. This should be
 * sufficient for all imaginable purposes. This must be power of two. */
#define SINC_QUEUE_LENGTH 256

#include "sinctable.cpp.in"

typedef struct {
	int time, output;
} sinc_queue_t;

struct audio_channel_data2
{
	int current_sample, last_sample;
	uae_u8 new_sample;
	int sample_accum, sample_accum_time;
	int sinc_output_state;
	sinc_queue_t sinc_queue[SINC_QUEUE_LENGTH];
	int sinc_queue_time;
	int sinc_queue_head;
	int audvol;
	int mixvol;
	unsigned int adk_mask;
};
struct audio_stream_data
{
	bool active;
	unsigned int evtime;
	struct audio_channel_data2 data[AUDIO_CHANNEL_MAX_STREAM_CH];
	SOUND_STREAM_CALLBACK cb;
	void *cb_data;
};

#define FIR_WIDTH 512
#define VOLCNT_BUFFER_SIZE 4096
union sIntFlt { uae_u32 U32; float F32; };
static float firmem[2 * FIR_WIDTH + 1];

struct audio_channel_data
{
	uae_u32 evtime;
	bool dmaenstore;
	bool intreq2;
	int irqcheck;
	bool dr;
	bool dsr;
	bool pbufldl;
	int drhpos;
	bool dat_written;
#if TEST_MISSED_DMA
	bool dat_loaded;
#endif
#if TEST_MANUAL_AUDIO
	bool mdat_loaded;
#endif
	uaecptr lc, pt;
	int state;
	int per;
	int len, wlen;
	int volcnt;
	uae_u16 dat, dat2;
	struct audio_channel_data2 data;
#if TEST_AUDIO > 0
	bool hisample, losample;
	bool have_dat;
	int per_original;
#endif
	/* too fast cpu fixes */
	uaecptr ptx;
	bool ptx_written;
	bool ptx_tofetch;
	int dmaofftime_active;
	int dmaofftime_cpu_cnt;
	uaecptr dmaofftime_pc;
	int minperloop;
	int volcntbufcnt;
	float volcntbuf[VOLCNT_BUFFER_SIZE];
};

static int audio_extra_streams[AUDIO_CHANNEL_STREAMS];
static int audio_total_extra_streams;

static int samplecnt;
#if SOUNDSTUFF > 0
static int extrasamples, outputsample, doublesample;
#endif

int sampleripper_enabled;
struct ripped_sample
{
	struct ripped_sample *next;
	uae_u8 *sample;
	int len, per, changed;
};

static struct ripped_sample *ripped_samples;

void write_wavheader (struct zfile *wavfile, size_t size, uae_u32 freq)
{
	uae_u16 tw;
	size_t tl;
	int bits = 8, channels = 1;

	zfile_fseek (wavfile, 0, SEEK_SET);
	zfile_fwrite ("RIFF", 1, 4, wavfile);
	tl = 0;
	if (size)
		tl = size - 8;
	zfile_fwrite (&tl, 1, 4, wavfile);
	zfile_fwrite ("WAVEfmt ", 1, 8, wavfile);
	tl = 16;
	zfile_fwrite (&tl, 1, 4, wavfile);
	tw = 1;
	zfile_fwrite (&tw, 1, 2, wavfile);
	tw = channels;
	zfile_fwrite (&tw, 1, 2, wavfile);
	tl = freq;
	zfile_fwrite (&tl, 1, 4, wavfile);
	tl = freq * channels * bits / 8;
	zfile_fwrite (&tl, 1, 4, wavfile);
	tw = channels * bits / 8;
	zfile_fwrite (&tw, 1, 2, wavfile);
	tw = bits;
	zfile_fwrite (&tw, 1, 2, wavfile);
	zfile_fwrite ("data", 1, 4, wavfile);
	tl = 0;
	if (size)
		tl = size - 44;
	zfile_fwrite (&tl, 1, 4, wavfile);
}

static void convertsample (uae_u8 *sample, int len)
{
	int i;
	for (i = 0; i < len; i++)
		sample[i] += 0x80;
}

static void namesplit (TCHAR *s)
{
	int l;

	l = uaetcslen(s) - 1;
	while (l >= 0) {
		if (s[l] == '.')
			s[l] = 0;
		if (s[l] == '\\' || s[l] == '/' || s[l] == ':' || s[l] == '?') {
			l++;
			break;
		}
		l--;
	}
	if (l > 0)
		memmove (s, s + l, (_tcslen (s + l) + 1) * sizeof (TCHAR));
}

void audio_sampleripper (int mode)
{
	struct ripped_sample *rs = ripped_samples;
	int cnt = 1;
	TCHAR path[MAX_DPATH], name[MAX_DPATH], filename[MAX_DPATH];
	TCHAR underline[] = _T("_");
	TCHAR extension[4];
	struct zfile *wavfile;

	if (mode < 0) {
		while (rs) {
			struct ripped_sample *next = rs->next;
			xfree(rs);
			rs = next;
		}
		ripped_samples = NULL;
		return;
	}

	while (rs) {
		if (rs->changed) {
			int type = -1;
			rs->changed = 0;
			fetch_ripperpath (path, sizeof (path) / sizeof (TCHAR));
			name[0] = 0;
			if (currprefs.floppyslots[0].dfxtype >= 0) {
				_tcscpy(name, currprefs.floppyslots[0].df);
				type = PATH_FLOPPY;
			} else if (currprefs.cdslots[0].inuse) {
				_tcscpy(name, currprefs.cdslots[0].name);
				type = PATH_CD;
			}
			if (!name[0])
				underline[0] = 0;
			if (type >= 0)
				cfgfile_resolve_path_load(name, sizeof(name) / sizeof(TCHAR), type);
			namesplit (name);
			_tcscpy (extension, _T("wav"));
			_sntprintf (filename, sizeof filename, _T("%s%s%s%03d.%s"), path, name, underline, cnt, extension);
			wavfile = zfile_fopen (filename, _T("wb"), 0);
			if (wavfile) {
				int freq = rs->per > 0 ? (currprefs.ntscmode ? 3579545 : 3546895 / rs->per) : 8000;
				write_wavheader (wavfile, 0, 0);
				convertsample (rs->sample, rs->len);
				zfile_fwrite (rs->sample, rs->len, 1, wavfile);
				convertsample (rs->sample, rs->len);
				write_wavheader (wavfile, zfile_ftell32(wavfile), freq);
				zfile_fclose (wavfile);
				write_log (_T("SAMPLERIPPER: %d: %dHz %d bytes\n"), cnt, freq, rs->len);
			} else {
				write_log (_T("SAMPLERIPPER: failed to open '%s'\n"), filename);
			}
		}
		cnt++;
		rs = rs->next;
	}
}

static void do_samplerip (struct audio_channel_data *adp)
{
	struct ripped_sample *rs = ripped_samples, *prev;
	int len = adp->wlen * 2;
	uae_u8 *smp = chipmem_xlate_indirect (adp->pt);
	int cnt = 0, i;

	if (!smp || !chipmem_check_indirect (adp->pt, len))
		return;
	for (i = 0; i < len; i++) {
		if (smp[i] != 0)
			break;
	}
	if (i == len || len <= 2)
		return;
	prev = NULL;
	while(rs) {
		if (rs->sample) {
			if (len == rs->len && !memcmp (rs->sample, smp, len))
				break;
			/* replace old identical but shorter sample */
			if (len > rs->len && !memcmp (rs->sample, smp, rs->len)) {
				xfree (rs->sample);
				rs->sample = xmalloc (uae_u8, len);
				memcpy (rs->sample, smp, len);
				write_log (_T("SAMPLERIPPER: replaced sample %d (%d -> %d)\n"), cnt, rs->len, len);
				rs->len = len;
				rs->per = adp->per / CYCLE_UNIT;
				rs->changed = 1;
				audio_sampleripper (0);
				return;
			}
		}
		prev = rs;
		rs = rs->next;
		cnt++;
	}
	if (rs || cnt > 100)
		return;
	rs = xmalloc (struct ripped_sample ,1);
	if (prev)
		prev->next = rs;
	else
		ripped_samples = rs;
	rs->len = len;
	rs->per = adp->per / CYCLE_UNIT;
	rs->sample = xmalloc (uae_u8, len);
	memcpy (rs->sample, smp, len);
	rs->next = NULL;
	rs->changed = 1;
	write_log (_T("SAMPLERIPPER: sample added (%06X, %d bytes), total %d samples\n"), adp->pt, len, ++cnt);
	audio_sampleripper (0);
}

static struct audio_channel_data audio_channel[AUDIO_CHANNELS_PAULA];
static struct audio_stream_data audio_stream[AUDIO_CHANNEL_STREAMS];
static struct audio_channel_data2 *audio_data[AUDIO_CHANNELS_PAULA + AUDIO_CHANNEL_STREAMS * AUDIO_CHANNEL_MAX_STREAM_CH];
int sound_available = 0;
void (*sample_handler) (void);
static void(*sample_prehandler) (unsigned long best_evtime);
static void(*extra_sample_prehandler) (unsigned long best_evtime);

float sample_evtime;
float scaled_sample_evtime;

int sound_cd_volume[2];
int sound_paula_volume[2];

static evt_t last_cycles;
static float next_sample_evtime;
static int previous_volcnt_update;

typedef uae_s8 sample8_t;
#define DO_CHANNEL_1(v, c) do { (v) *= audio_channel[c].data.mixvol; } while (0)
#define SBASEVAL16(logn) ((logn) == 1 ? SOUND16_BASE_VAL >> 1 : SOUND16_BASE_VAL)

STATIC_INLINE int FINISH_DATA (int data, int bits, int ch)
{
	if (bits < 16) {
		int shift = 16 - bits;
		data <<= shift;
	} else {
		int shift = bits - 16;
		data >>= shift;
	}
	data = data * sound_paula_volume[ch] / 32768;
	return data;
}

static uae_u32 right_word_saved[SOUND_MAX_DELAY_BUFFER];
static uae_u32 left_word_saved[SOUND_MAX_DELAY_BUFFER];
static uae_u32 right2_word_saved[SOUND_MAX_DELAY_BUFFER];
static uae_u32 left2_word_saved[SOUND_MAX_DELAY_BUFFER];
static int saved_ptr, saved_ptr2;

static int mixed_on, mixed_stereo_size, mixed_mul1, mixed_mul2;
static int led_filter_forced, sound_use_filter, sound_use_filter_sinc, led_filter_on;

#define PAULARATE 3740000
static float Sinc(float x)
{
	return x ? sinf(x) / x : 1;
}
static float Hamming(float x)
{
	float pi = 4 * atanf(1);
	float v;
	if (x > -1 && x < 1)
		v = cosf(x * pi / 2);
	else
		v = 0;
	return v * v;
}
static void makefir(void)
{
	float pi = 4 * atanf(1);
	float *FIRTable = firmem + FIR_WIDTH;
	float yscale = float(currprefs.sound_freq) / float(PAULARATE);
	float xscale = pi * yscale;
	for (int i = -FIR_WIDTH; i <= FIR_WIDTH; i++)
		FIRTable[i] = yscale * Sinc(float(i) * xscale) * Hamming(float(i) / float(FIR_WIDTH - 1));
}

/* denormals are very small floating point numbers that force FPUs into slow
mode. All lowpass filters using floats are suspectible to denormals unless
a small offset is added to avoid very small floating point numbers. */
#define DENORMAL_OFFSET (1E-10)

static struct filter_state {
	float rc1, rc2, rc3, rc4, rc5;
} sound_filter_state[AUDIO_CHANNELS_PAULA];

static float a500e_filter1_a0;
static float a500e_filter2_a0;
static float filter_a0; /* a500 and a1200 use the same */

enum {
	FILTER_NONE = 0,
	FILTER_MODEL_A500,
	FILTER_MODEL_A1200,
	FILTER_MODEL_A500_FIXEDONLY
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

static int filter (int input, struct filter_state *fs)
{
	int o;
	float normal_output, led_output;

	input = (uae_s16)input;
	switch (sound_use_filter) {

	case FILTER_MODEL_A500:
		fs->rc1 = (float)(a500e_filter1_a0 * input + (1.0f - a500e_filter1_a0) * fs->rc1 + DENORMAL_OFFSET);
		fs->rc2 = a500e_filter2_a0 * fs->rc1 + (1.0f - a500e_filter2_a0) * fs->rc2;
		normal_output = fs->rc2;

		fs->rc3 = filter_a0 * normal_output + (1 - filter_a0) * fs->rc3;
		fs->rc4 = filter_a0 * fs->rc3       + (1 - filter_a0) * fs->rc4;
		fs->rc5 = filter_a0 * fs->rc4       + (1 - filter_a0) * fs->rc5;

		led_output = fs->rc5;
		break;

	case FILTER_MODEL_A500_FIXEDONLY:
		fs->rc1 = (float)(a500e_filter1_a0 * input + (1.0f - a500e_filter1_a0) * fs->rc1 + DENORMAL_OFFSET);
		fs->rc2 = a500e_filter2_a0 * fs->rc1 + (1.0f - a500e_filter2_a0) * fs->rc2;
		normal_output = fs->rc2;
		led_output = fs->rc2;
		break;

	case FILTER_MODEL_A1200:
		normal_output = (float)input;

		fs->rc2 = (float)(filter_a0 * normal_output + (1 - filter_a0) * fs->rc2 + DENORMAL_OFFSET);
		fs->rc3 = filter_a0 * fs->rc2       + (1 - filter_a0) * fs->rc3;
		fs->rc4 = filter_a0 * fs->rc3       + (1 - filter_a0) * fs->rc4;

		led_output = fs->rc4;
		break;

	case FILTER_NONE:
	default:
		return input;

	}

	if (led_filter_on)
		o = (int)led_output;
	else
		o = (int)normal_output;

	if (o > 32767)
		o = 32767;
	else if (o < -32768)
		o = -32768;

	return o;
}

/* Always put the right word before the left word.  */

static void put_sound_word_right (uae_u32 w)
{
	if (mixed_on) {
		right_word_saved[saved_ptr] = w;
		return;
	}
	PUT_SOUND_WORD(w);
}

static void put_sound_word_left (uae_u32 w)
{
	if (mixed_on) {
		uae_u32 rold, lold, rnew, lnew, tmp;

		left_word_saved[saved_ptr] = w;
		lnew = w - SOUND16_BASE_VAL;
		rnew = right_word_saved[saved_ptr] - SOUND16_BASE_VAL;

		saved_ptr = (saved_ptr + 1) & mixed_stereo_size;

		lold = left_word_saved[saved_ptr] - SOUND16_BASE_VAL;
		tmp = (rnew * mixed_mul2 + lold * mixed_mul1) / MIXED_STEREO_SCALE;
		tmp += SOUND16_BASE_VAL;

		rold = right_word_saved[saved_ptr] - SOUND16_BASE_VAL;
		w = (lnew * mixed_mul2 + rold * mixed_mul1) / MIXED_STEREO_SCALE;

		PUT_SOUND_WORD(tmp);
		PUT_SOUND_WORD(w);
	} else {
		PUT_SOUND_WORD(w);
	}
}

static void put_sound_word_right2 (uae_u32 w)
{
	if (mixed_on) {
		right2_word_saved[saved_ptr2] = w;
		return;
	}
	PUT_SOUND_WORD(w);
}

static void put_sound_word_left2 (uae_u32 w)
{
	if (mixed_on) {
		uae_u32 rold, lold, rnew, lnew, tmp;

		left2_word_saved[saved_ptr2] = w;
		lnew = w - SOUND16_BASE_VAL;
		rnew = right2_word_saved[saved_ptr2] - SOUND16_BASE_VAL;

		saved_ptr2 = (saved_ptr2 + 1) & mixed_stereo_size;

		lold = left2_word_saved[saved_ptr2] - SOUND16_BASE_VAL;
		tmp = (rnew * mixed_mul2 + lold * mixed_mul1) / MIXED_STEREO_SCALE;
		tmp += SOUND16_BASE_VAL;

		rold = right2_word_saved[saved_ptr2] - SOUND16_BASE_VAL;
		w = (lnew * mixed_mul2 + rold * mixed_mul1) / MIXED_STEREO_SCALE;

		PUT_SOUND_WORD(tmp);
		PUT_SOUND_WORD(w);
	} else {
		PUT_SOUND_WORD(w);
	}
}

static void anti_prehandler (unsigned long best_evtime)
{
	int i, output;
	struct audio_channel_data2 *acd;

	/* Handle accumulator antialiasiation */
	for (i = 0; audio_data[i]; i++) {
		acd = audio_data[i];
		output = (acd->current_sample * acd->mixvol) & acd->adk_mask;
		acd->sample_accum += output * best_evtime;
		acd->sample_accum_time += best_evtime;
	}
}

static void samplexx_anti_handler (int *datasp, int ch_start, int ch_num)
{
	int i, j;
	for (i = ch_start, j = 0; j < ch_num; i++, j++) {
		struct audio_channel_data2 *acd = audio_data[i];
		datasp[j] = acd->sample_accum_time ? (acd->sample_accum / acd->sample_accum_time) : 0;
		acd->sample_accum = 0;
		acd->sample_accum_time = 0;
	}
}

static void sinc_prehandler_paula (unsigned long best_evtime)
{
	int i, output;
	struct audio_channel_data2 *acd;

	for (i = 0; i < AUDIO_CHANNELS_PAULA; i++)  {
		acd = audio_data[i];
		int vol = acd->mixvol;
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
static void samplexx_sinc_handler (int *datasp, int ch_start, int ch_num)
{
	int n, i, k;
	int const *winsinc;

	if (sound_use_filter_sinc && ch_start == 0) {
		n = (sound_use_filter_sinc == FILTER_MODEL_A500 || sound_use_filter_sinc == FILTER_MODEL_A500_FIXEDONLY) ? 0 : 2;
		if (led_filter_on)
			n += 1;
	} else {
		n = 4;
	}
	winsinc = winsinc_integral[n];


	for (i = ch_start, k = 0; k < ch_num; i++, k++) {
		int j, v;
		struct audio_channel_data2 *acd = audio_data[i];
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
		datasp[k] = v;
	}
}

static void do_filter(int *data, int num)
{
	if (currprefs.sound_filter)
		*data = filter(*data, &sound_filter_state[num]);
}

static void get_extra_channels(int *data1, int *data2, int sample1, int sample2)
{
	int d1 = *data1 + sample1;
	int d2 = (data2 ? *data2 : 0) + sample2;
	if (d1 < -32768)
		d1 = -32768;
	if (d1 > 32767)
		d1 = 32767;
	if (d2 < -32768)
		d2 = -32768;
	if (d2 > 32767)
		d2 = 32767;
	int needswap = currprefs.sound_stereo_swap_paula ^ currprefs.sound_stereo_swap_ahi;
	if (needswap) {
		*data1 = d2;
		if (data2)
			*data2 = d1;
	} else {
		*data1 = d1;
		if (data2)
			*data2 = d2;
	}
}

static void do_extra_channels(int idx, int ch, int *data1, int *data2, int *data3, int *data4, int *data5, int *data6)
{
	idx += AUDIO_CHANNELS_PAULA;
	if (ch == 2) {
		int datas[2];
		samplexx_anti_handler(datas, idx, 2);
		get_extra_channels(data1, data2, datas[0], datas[1]);
	} else if (ch == 1) {
		int datas[1];
		samplexx_anti_handler(datas, idx, 1);
		int d1 = *data1 + datas[0];
		if (d1 < -32768)
			d1 = -32768;
		if (d1 > 32767)
			d1 = 32767;
		*data1 = d1;
		if (data2)
			*data2 = d1;
	} else if (ch > 2) {
		int datas[AUDIO_CHANNEL_MAX_STREAM_CH];
		samplexx_anti_handler(datas, idx, 6);
		get_extra_channels(data1, data2, datas[0], datas[1]);
		if (data3 && data4)
			get_extra_channels(data3, data4, datas[2], datas[3]);
		if (data5 && data6)
			get_extra_channels(data5, data6, datas[4], datas[5]);
	}
}

static void get_extra_channels_sample2(int *data1, int *data2, int mode)
{
	if (!audio_total_extra_streams)
		return;
	int idx = 0;
	for (int i = 0; i < AUDIO_CHANNEL_STREAMS; i++) {
		int ch = audio_extra_streams[i];
		if (ch) {
			do_extra_channels(idx, ch, data1, data2, NULL, NULL, NULL, NULL);
			idx += ch;
		}
	}
}

static void get_extra_channels_sample6(int *data1, int *data2, int *data3, int *data4, int *data5, int *data6, int mode)
{
	if (!audio_total_extra_streams)
		return;
	int idx = 0;
	for (int i = 0; i < AUDIO_CHANNEL_STREAMS; i++) {
		int ch = audio_extra_streams[i];
		if (ch) {
			do_extra_channels(idx, ch, data1, data2, data3, data4, data5, data6);
			idx += ch;
		}
	}
}

static void set_sound_buffers(void)
{
#if SOUNDSTUFF > 1
	paula_sndbufpt_prev = paula_sndbufpt_start;
	paula_sndbufpt_start = paula_sndbufpt;
#endif
}

static void clear_sound_buffers(void)
{
	memset(paula_sndbuffer, 0, paula_sndbufsize);
	paula_sndbufpt = paula_sndbuffer;
}

static void check_sound_buffers(void)
{
#if SOUNDSTUFF > 1
	int len;
#endif

	if (active_sound_stereo == SND_4CH_CLONEDSTEREO) {
		((uae_u16 *)paula_sndbufpt)[0] = ((uae_u16 *)paula_sndbufpt)[-2];
		((uae_u16 *)paula_sndbufpt)[1] = ((uae_u16 *)paula_sndbufpt)[-1];
		paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 2 * 2);
	} else if (active_sound_stereo == SND_6CH_CLONEDSTEREO) {
		uae_s16 *p = ((uae_s16 *)paula_sndbufpt);
		uae_s32 sum;
		p[2] = p[-2];
		p[3] = p[-1];
		sum = (uae_s32)(p[-2]) + (uae_s32)(p[-1]) + (uae_s32)(p[2]) + (uae_s32)(p[3]);
		p[0] = sum / 8;
		p[1] = sum / 8;
		paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 4 * 2);
	} else if (active_sound_stereo == SND_8CH_CLONEDSTEREO) {
		uae_s16 *p = ((uae_s16 *)paula_sndbufpt);
		uae_s32 sum;
		p[2] = p[-2];
		p[3] = p[-1];
		p[4] = p[-2];
		p[5] = p[-1];
		sum = (uae_s32)(p[-2]) + (uae_s32)(p[-1]) + (uae_s32)(p[2]) + (uae_s32)(p[3]);
		p[0] = sum / 8;
		p[1] = sum / 8;
		paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 6 * 2);
	}
#if SOUNDSTUFF > 1
	if (outputsample == 0)
		return;
	len = paula_sndbufpt - paula_sndbufpt_start;
	if (outputsample < 0) {
		int i;
		uae_s16 *p1 = (uae_s16 *)paula_sndbufpt_prev;
		uae_s16 *p2 = (uae_s16 *)paula_sndbufpt_start;
		for (i = 0; i < len; i++) {
			*p1 = (*p1 + *p2) / 2;
		}
		paula_sndbufpt = paula_sndbufpt_start;
	}
#endif
	if ((uae_u8 *)paula_sndbufpt - (uae_u8 *)paula_sndbuffer >= paula_sndbufsize) {
		finish_sound_buffer();
	}
#if SOUNDSTUFF > 1
	while (doublesample-- > 0) {
		memcpy(paula_sndbufpt, paula_sndbufpt_start, len * 2);
		paula_sndbufpt += len;
		if ((uae_u8 *)paula_sndbufpt - (uae_u8 *)paula_sndbuffer >= paula_sndbufsize) {
			finish_sound_buffer();
			paula_sndbufpt = paula_sndbuffer;
		}
	}
#endif
}

static void sample16i_sinc_handler (void)
{
	int datas[AUDIO_CHANNELS_PAULA], data1;

	samplexx_sinc_handler (datas, 0, AUDIO_CHANNELS_PAULA);
	data1 = datas[0] + datas[3] + datas[1] + datas[2];
	data1 = FINISH_DATA (data1, 18, 0);
	
	do_filter(&data1, 0);

	get_extra_channels_sample2(&data1, NULL, 2);

	set_sound_buffers ();
	PUT_SOUND_WORD_MONO (data1);
	check_sound_buffers ();
}

void sample16_handler (void)
{
	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	int data;

	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);
	data0 &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;
	data0 += data1;
	data0 += data2;
	data0 += data3;
	data = SBASEVAL16(2) + data0;
	data = FINISH_DATA (data, 16, 0);

	do_filter(&data, 0);

	get_extra_channels_sample2(&data, NULL, 0);

	set_sound_buffers ();
	PUT_SOUND_WORD_MONO (data);
	check_sound_buffers ();
}

/* This interpolator examines sample points when Paula switches the output
* voltage and computes the average of Paula's output */
static void sample16i_anti_handler (void)
{
	int datas[AUDIO_CHANNELS_PAULA], data1;

	samplexx_anti_handler (datas, 0, AUDIO_CHANNELS_PAULA);
	data1 = datas[0] + datas[3] + datas[1] + datas[2];
	data1 = FINISH_DATA (data1, 16, 0);

	do_filter(&data1, 0);

	get_extra_channels_sample2(&data1, NULL, 1);

	set_sound_buffers ();
	PUT_SOUND_WORD_MONO (data1);
	check_sound_buffers ();
}

static void sample16i_rh_handler (void)
{
	unsigned long delta, ratio;

	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	int data0p = audio_channel[0].data.last_sample;
	int data1p = audio_channel[1].data.last_sample;
	int data2p = audio_channel[2].data.last_sample;
	int data3p = audio_channel[3].data.last_sample;
	int data;

	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);
	DO_CHANNEL_1 (data0p, 0);
	DO_CHANNEL_1 (data1p, 1);
	DO_CHANNEL_1 (data2p, 2);
	DO_CHANNEL_1 (data3p, 3);

	data0 &= audio_channel[0].data.adk_mask;
	data0p &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data1p &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data2p &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;
	data3p &= audio_channel[3].data.adk_mask;

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
	data = SBASEVAL16(2) + data0;
	data = FINISH_DATA (data, 16, 0);

	do_filter(&data, 0);

	get_extra_channels_sample2(&data, NULL, 0);

	set_sound_buffers ();
	PUT_SOUND_WORD_MONO (data);
	check_sound_buffers ();
}

static void sample16i_crux_handler (void)
{
	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	int data0p = audio_channel[0].data.last_sample;
	int data1p = audio_channel[1].data.last_sample;
	int data2p = audio_channel[2].data.last_sample;
	int data3p = audio_channel[3].data.last_sample;
	int data;

	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);
	DO_CHANNEL_1 (data0p, 0);
	DO_CHANNEL_1 (data1p, 1);
	DO_CHANNEL_1 (data2p, 2);
	DO_CHANNEL_1 (data3p, 3);

	data0 &= audio_channel[0].data.adk_mask;
	data0p &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data1p &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data2p &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;
	data3p &= audio_channel[3].data.adk_mask;

	{
		struct audio_channel_data *cdp;
		unsigned long ratio, ratio1;
#define INTERVAL ((int)(scaled_sample_evtime * 3))
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
	data = SBASEVAL16(2) + data0;
	data = FINISH_DATA (data, 16, 0);

	do_filter(&data, 0);

	get_extra_channels_sample2(&data, NULL, 0);

	set_sound_buffers ();
	PUT_SOUND_WORD_MONO (data);
	check_sound_buffers ();
}

#ifdef HAVE_STEREO_SUPPORT

STATIC_INLINE void make6ch (uae_s32 d0, uae_s32 d1, uae_s32 d2, uae_s32 d3, uae_s32 *d4, uae_s32 *d5)
{
	uae_s32 sum = d0 + d1 + d2 + d3;
	sum /= 8;
	*d4 = sum;
	*d5 = sum;
}

void sample16ss_handler (void)
{
	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	int data4, data5;
	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);

	data0 &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;

	data0 = FINISH_DATA (data0, 14, 0);
	data1 = FINISH_DATA (data1, 14, 0);
	data2 = FINISH_DATA (data2, 14, 1);
	data3 = FINISH_DATA (data3, 14, 1);

	do_filter(&data0, 0);
	do_filter(&data1, 1);
	do_filter(&data2, 3);
	do_filter(&data3, 2);

	if (active_sound_stereo >= SND_6CH)
		make6ch(data0, data1, data2, data3, &data4, &data5);

	get_extra_channels_sample6(&data0, &data1, &data3, &data2, &data4, &data5, 0);

	set_sound_buffers ();
	put_sound_word_right(data0);
	put_sound_word_left(data1);
	if (active_sound_stereo >= SND_6CH) {
		PUT_SOUND_WORD(data4);
		PUT_SOUND_WORD(data5);
	}
	if (active_sound_stereo >= SND_8CH) {
		PUT_SOUND_WORD(data4);
		PUT_SOUND_WORD(data5);
	}
	put_sound_word_right2(data3);
	put_sound_word_left2(data2);
	check_sound_buffers ();
}

/* This interpolator examines sample points when Paula switches the output
* voltage and computes the average of Paula's output */

static void sample16ss_anti_handler (void)
{
	int data0, data1, data2, data3, data4, data5;
	int datas[AUDIO_CHANNELS_PAULA];

	samplexx_anti_handler (datas, 0, AUDIO_CHANNELS_PAULA);
	data0 = FINISH_DATA (datas[0], 14, 0);
	data1 = FINISH_DATA (datas[1], 14, 0);
	data2 = FINISH_DATA (datas[2], 14, 1);
	data3 = FINISH_DATA (datas[3], 14, 1);

	do_filter(&data0, 0);
	do_filter(&data1, 1);
	do_filter(&data2, 3);
	do_filter(&data3, 2);

	if (active_sound_stereo >= SND_6CH)
		make6ch(data0, data1, data2, data3, &data4, &data5);

	get_extra_channels_sample6(&data0, &data1, &data3, &data2, &data4, &data5, 0);

	set_sound_buffers();
	put_sound_word_right(data0);
	put_sound_word_left(data1);
	if (active_sound_stereo >= SND_6CH) {
		PUT_SOUND_WORD(data4);
		PUT_SOUND_WORD(data5);
	}
	if (active_sound_stereo >= SND_8CH) {
		PUT_SOUND_WORD(data4);
		PUT_SOUND_WORD(data5);
	}
	put_sound_word_right2(data3);
	put_sound_word_left2(data2);
	check_sound_buffers();
}

static void sample16si_anti_handler(void)
{
	int datas[AUDIO_CHANNELS_PAULA], data1, data2;

	samplexx_anti_handler(datas, 0, AUDIO_CHANNELS_PAULA);
	data1 = datas[0] + datas[3];
	data2 = datas[1] + datas[2];
	data1 = FINISH_DATA(data1, 15, 0);
	data2 = FINISH_DATA(data2, 15, 1);

	do_filter(&data1, 0);
	do_filter(&data2, 1);

	get_extra_channels_sample2(&data1, &data2, 1);

	set_sound_buffers();
	put_sound_word_right(data1);
	put_sound_word_left(data2);
	check_sound_buffers();
}

static void sample16ss_sinc_handler(void)
{
	int data0, data1, data2, data3, data4, data5;
	int datas[AUDIO_CHANNELS_PAULA];

	samplexx_sinc_handler(datas, 0, AUDIO_CHANNELS_PAULA);
	data0 = FINISH_DATA (datas[0], 16, 0);
	data1 = FINISH_DATA (datas[1], 16, 0);
	data2 = FINISH_DATA (datas[2], 16, 1);
	data3 = FINISH_DATA (datas[3], 16, 1);

	do_filter(&data0, 0);
	do_filter(&data1, 1);
	do_filter(&data2, 3);
	do_filter(&data3, 2);

	if (active_sound_stereo >= SND_6CH)
		make6ch(data0, data1, data2, data3, &data4, &data5);

	get_extra_channels_sample6(&data0, &data1, &data3, &data2, &data4, &data5, 0);

	set_sound_buffers ();
	put_sound_word_right(data0);
	put_sound_word_left (data1);
	if (active_sound_stereo >= SND_6CH) {
		PUT_SOUND_WORD(data4);
		PUT_SOUND_WORD(data5);
	}
	if (active_sound_stereo >= SND_8CH) {
		PUT_SOUND_WORD(data4);
		PUT_SOUND_WORD(data5);
	}
	put_sound_word_right2(data3);
	put_sound_word_left2 (data2);
	check_sound_buffers ();
}

static void sample16si_sinc_handler (void)
{
	int datas[AUDIO_CHANNELS_PAULA], data1, data2;

	samplexx_sinc_handler (datas, 0, AUDIO_CHANNELS_PAULA);
	data1 = datas[0] + datas[3];
	data2 = datas[1] + datas[2];
	data1 = FINISH_DATA (data1, 17, 0);
	data2 = FINISH_DATA (data2, 17, 1);

	do_filter(&data1, 0);
	do_filter(&data2, 1);

	get_extra_channels_sample2(&data1, &data2, 2);

	set_sound_buffers ();
	put_sound_word_right(data1);
	put_sound_word_left(data2);
	check_sound_buffers ();
}

void sample16s_handler (void)
{
	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);

	data0 &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;

	data0 += data3;
	data1 += data2;
	data2 = SBASEVAL16(1) + data0;
	data2 = FINISH_DATA (data2, 15, 0);
	data3 = SBASEVAL16(1) + data1;
	data3 = FINISH_DATA (data3, 15, 1);

	do_filter(&data2, 0);
	do_filter(&data3, 1);

	get_extra_channels_sample2(&data2, &data3, 0);

	set_sound_buffers ();
	put_sound_word_right(data2);
	put_sound_word_left(data3);
	check_sound_buffers ();
}

static void sample16si_crux_handler (void)
{
	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	int data0p = audio_channel[0].data.last_sample;
	int data1p = audio_channel[1].data.last_sample;
	int data2p = audio_channel[2].data.last_sample;
	int data3p = audio_channel[3].data.last_sample;

	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);
	DO_CHANNEL_1 (data0p, 0);
	DO_CHANNEL_1 (data1p, 1);
	DO_CHANNEL_1 (data2p, 2);
	DO_CHANNEL_1 (data3p, 3);

	data0 &= audio_channel[0].data.adk_mask;
	data0p &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data1p &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data2p &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;
	data3p &= audio_channel[3].data.adk_mask;

	{
		struct audio_channel_data *cdp;
		unsigned long ratio, ratio1;
#define INTERVAL ((int)(scaled_sample_evtime * 3))
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
	data2 = SBASEVAL16(1) + data0;
	data2 = FINISH_DATA (data2, 15, 0);
	data3 = SBASEVAL16(1) + data1;
	data3 = FINISH_DATA (data3, 15, 1);

	do_filter(&data2, 0);
	do_filter(&data3, 1);

	get_extra_channels_sample2(&data2, &data3, 0);

	set_sound_buffers ();
	put_sound_word_right(data2);
	put_sound_word_left (data3);
	check_sound_buffers ();
}

static void sample16si_rh_handler (void)
{
	unsigned long delta, ratio;

	int data0 = audio_channel[0].data.current_sample;
	int data1 = audio_channel[1].data.current_sample;
	int data2 = audio_channel[2].data.current_sample;
	int data3 = audio_channel[3].data.current_sample;
	int data0p = audio_channel[0].data.last_sample;
	int data1p = audio_channel[1].data.last_sample;
	int data2p = audio_channel[2].data.last_sample;
	int data3p = audio_channel[3].data.last_sample;

	DO_CHANNEL_1 (data0, 0);
	DO_CHANNEL_1 (data1, 1);
	DO_CHANNEL_1 (data2, 2);
	DO_CHANNEL_1 (data3, 3);
	DO_CHANNEL_1 (data0p, 0);
	DO_CHANNEL_1 (data1p, 1);
	DO_CHANNEL_1 (data2p, 2);
	DO_CHANNEL_1 (data3p, 3);

	data0 &= audio_channel[0].data.adk_mask;
	data0p &= audio_channel[0].data.adk_mask;
	data1 &= audio_channel[1].data.adk_mask;
	data1p &= audio_channel[1].data.adk_mask;
	data2 &= audio_channel[2].data.adk_mask;
	data2p &= audio_channel[2].data.adk_mask;
	data3 &= audio_channel[3].data.adk_mask;
	data3p &= audio_channel[3].data.adk_mask;

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
	data2 = SBASEVAL16(1) + data0;
	data2 = FINISH_DATA (data2, 15, 0);
	data3 = SBASEVAL16(1) + data1;
	data3 = FINISH_DATA (data3, 15, 1);

	do_filter(&data2, 0);
	do_filter(&data3, 1);

	get_extra_channels_sample2(&data2, &data3, 0);

	set_sound_buffers ();
	put_sound_word_right(data2);
	put_sound_word_left (data3);
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

static void zerostate(int nr, bool reset)
{
	struct audio_channel_data *cdp = audio_channel + nr;
#if DEBUG_AUDIO > 0
	if (debugchannel (nr))
		write_log (_T("%d: ZEROSTATE\n"), nr);
#endif
	cdp->state = 0;
	cdp->irqcheck = 0;
	cdp->evtime = MAX_EV;
	if (!currprefs.cpu_memory_cycle_exact || reset) {
		cdp->intreq2 = false;
	}
	cdp->dmaenstore = false;
	cdp->dmaofftime_active = 0;
	cdp->volcnt = 0;
	cdp->volcntbufcnt = 0;
	cdp->minperloop = 0;
	memset(cdp->volcntbuf, 0, sizeof(cdp->volcntbuf));
#if TEST_AUDIO > 0
	cdp->have_dat = false;
#endif

}

static void schedule_audio (void)
{
	unsigned long best = MAX_EV;
	int i;

	eventtab[ev_audio].active = 0;
	eventtab[ev_audio].oldcycles = get_cycles ();
	for (i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
		struct audio_channel_data *cdp = audio_channel + i;
		if (cdp->evtime != MAX_EV) {
			if (best > cdp->evtime) {
				best = cdp->evtime;
				eventtab[ev_audio].active = 1;
			}
		}
	}
	for (i = 0; i < audio_total_extra_streams; i++) {
		struct audio_stream_data *cdp = audio_stream + i;
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
	for (i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
		zerostate(i, true);
	}
	for (i = 0; i < audio_total_extra_streams; i++) {
		audio_stream[i].evtime = MAX_EV;
	}
	schedule_audio ();
	events_schedule ();
	samplecnt = 0;
	extrasamples = 0;
	outputsample = 1;
	doublesample = 0;
}

void audio_deactivate (void)
{
	gui_data.sndbuf_status = 3;
	gui_data.sndbuf = 0;
	audio_work_to_do = 0;
	pause_sound_buffer ();
	clear_sound_buffers ();
	audio_event_reset ();
}

int audio_activate (void)
{
	int ret = 0;

	if (!audio_work_to_do) {
		restart_sound_buffer();
		ret = 1;
		if (!isrestore()) {
			audio_event_reset();
		}
	}
	audio_work_to_do = 4 * maxvpos_nom * 50;
	return ret;
}

STATIC_INLINE int is_audio_active (void)
{
	return audio_work_to_do;
}

static void update_volume(int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	// 7 bit register in Paula.
	v &= 127;
	if (v > 64)
		v = 64;
	cdp->data.audvol = v;
}

uae_u16 audio_dmal(void)
{
	uae_u16 dmal = 0;
	for (int nr = 0; nr < AUDIO_CHANNELS_PAULA; nr++) {
		struct audio_channel_data *cdp = audio_channel + nr;
		if (cdp->dr)
			dmal |= 1 << (nr * 2 + 1);
		if (cdp->dsr)
			dmal |= 1 << (nr * 2 + 0);
		cdp->dr = cdp->dsr = false;
	}
	return dmal;
}

static int isirq(int nr)
{
	return INTREQR() & (0x80 << nr);
}

static void setirq(int nr, int which)
{
#if DEBUG_AUDIO > 0
	struct audio_channel_data *cdp = audio_channel + nr;
	if (debugchannel (nr))
		write_log (_T("SETIRQ%d (%d,%d) PC=%08X\n"), nr, which, isirq (nr) ? 1 : 0, M68K_GETPC);
#endif
	// audio interrupts are delayed by 1 CCK
	INTREQ_INT(nr + 7, CYCLE_UNIT);
}

static void newsample(int nr, sample8_t sample)
{
	struct audio_channel_data *cdp = audio_channel + nr;
#if DEBUG_AUDIO > 0
	if (!debugchannel(nr))
		sample = 0;
#endif
#if DEBUG_AUDIO > 2
	if (debugchannel(nr))
		write_log(_T("SAMPLE%d: %02x\n"), nr, sample & 0xff);
#endif
	if (!(audio_channel_mask & (1 << nr)))
		sample = 0;
	if (currprefs.sound_volcnt) {
		cdp->data.new_sample = sample;
	} else {
		cdp->data.last_sample = cdp->data.current_sample;
		cdp->data.current_sample = sample;
	}
}

void event_setdsr(uae_u32 v)
{
	struct audio_channel_data* cdp = audio_channel + v;
	cdp->dsr = true;
}

static void setdr(int nr, bool startup)
{
	struct audio_channel_data *cdp = audio_channel + nr;
#if TEST_AUDIO > 0
	if (debugchannel (nr) && cdp->dr)
		write_log (_T("%d: DR already active (STATE=%d)\n"), nr, cdp->state);
#endif
	if (dmaen(DMA_MASTER)) {

#if DEBUG_AUDIO > 0
		if (debugchannel(nr) && cdp->wlen <= 2)
			write_log(_T("DR%d=%d LEN=%d/%d PT=%08X PC=%08X\n"), nr, cdp->dr, cdp->wlen, cdp->len, cdp->pt, M68K_GETPC);
#endif
		cdp->drhpos = current_hpos();

		if (!startup && cdp->wlen == 1) {
			if (!currprefs.cachesize && (cdp->per < PERIOD_LOW * CYCLE_UNIT || currprefs.cpu_compatible)) {
				event2_newevent_xx(-1, 1 * CYCLE_UNIT, nr, event_setdsr);
			} else {
				event_setdsr(nr);
			}
#if DEBUG_AUDIO > 0
			if (debugchannel(nr))
				write_log(_T("DSR%d=1 PT=%08X PC=%08X\n"), nr, cdp->pt, M68K_GETPC);
#endif
		} else {
			cdp->dr = true;
		}
	} else {
#if DEBUG_AUDIO > 0
		if (debugchannel(nr))
			write_log(_T("setdr%d ignored, DMA disabled PT=%08X PC=%08X\n"), nr, cdp->pt, M68K_GETPC);
#endif
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
				cdp[1].per = 65536 * CYCLE_UNIT;
			else if (cdp->dat > PERIOD_MIN)
				cdp[1].per = cdp->dat * CYCLE_UNIT;
			else
				cdp[1].per = PERIOD_MIN * CYCLE_UNIT;
		} else	if (audav) {
			update_volume(nr + 1, cdp->dat);
		}
	} else {
#if TEST_AUDIO > 0
		if (debugchannel (nr)) {
			if (cdp->hisample || cdp->losample)
				write_log (_T("%d: high or low sample not used\n"), nr);
			if (!cdp->have_dat)
				write_log (_T("%d: dat not updated. STATE=%d 1=%04x 2=%04x\n"), nr, cdp->state, cdp->dat, cdp->dat2);
		}
		cdp->hisample = cdp->losample = true;
		cdp->have_dat = false;
#endif
#if DEBUG_AUDIO > 2 || DEBUG_AUDIO2
		if (debugchannel (nr))
			write_log (_T("LOAD%dDAT: New:%04x, Old:%04x\n"), nr, cdp->dat, cdp->dat2);
#endif
		cdp->dat2 = cdp->dat;
	}

#if TEST_MANUAL_AUDIO
	if (!cdp->mdat_loaded) {
		write_log("Missed manual AUD%dDAT\n", nr);
	}
	cdp->mdat_loaded = false;
#endif
#if TEST_MISSED_DMA
	if (!cdp->dat_loaded) {
		write_log("Missed DMA %d\n", nr);
	}
	cdp->dat_loaded = false;
#endif
}
static void loaddat (int nr)
{
	loaddat (nr, false);
}

static void loadper1(int nr)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	cdp->evtime = 1 * CYCLE_UNIT;
#if DEBUG_AUDIO2 > 0
	if (debugchannel(nr)) {
		write_log(_T("LOADPERP%d: %d\n"), nr, cdp->evtime / CYCLE_UNIT);
	}
#endif
}

static void loadperm1(int nr)
{
	struct audio_channel_data *cdp = audio_channel + nr;

	if (cdp->per == CYCLE_UNIT) {
		cdp->evtime = CYCLE_UNIT;
		if (isirq(nr)) {
			cdp->irqcheck = 1;
		} else {
			cdp->irqcheck = -1;
		}
	} else if (cdp->per > CYCLE_UNIT) {
		cdp->evtime = cdp->per - 1 * CYCLE_UNIT;
		cdp->state |= 0x10;
	} else {
		cdp->evtime = 65536 * CYCLE_UNIT;
		cdp->state |= 0x10;
	}
#if DEBUG_AUDIO2 > 0
	if (debugchannel(nr)) {
		write_log(_T("LOADPERM%d: %d\n"), nr, cdp->evtime / CYCLE_UNIT);
	}
#endif
}

static void loadper (int nr)
{
	struct audio_channel_data *cdp = audio_channel + nr;

	cdp->evtime = cdp->per;
	cdp->data.mixvol = cdp->data.audvol;
	if (cdp->evtime < CYCLE_UNIT)
		write_log (_T("LOADPER%d bug %d\n"), nr, cdp->evtime);
#if DEBUG_AUDIO2 > 0
	if (debugchannel(nr)) {
		write_log(_T("LOADPER%d: %d\n"), nr, cdp->evtime / CYCLE_UNIT);
	}
#endif
}


static bool audio_state_channel2 (int nr, bool perfin)
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
		zerostate (nr, true);
		return true;
	}
	audio_activate ();

	if ((cdp->state & 15) == 2 || (cdp->state & 15) == 3) {
		if (!chan_ena && old_dma) {
			// DMA switched off, state=2/3 and "too fast CPU": set flag
			cdp->dmaofftime_active = true;
			cdp->dmaofftime_cpu_cnt = regs.instruction_cnt;
			cdp->dmaofftime_pc = M68K_GETPC;
		}
		// check if CPU executed at least 60 instructions (if JIT is off), there are stupid code that
		// disable audio DMA, then set new sample, then re-enable without actually wanting to start
		// new sample immediately.
		if (cdp->dmaofftime_active && !old_dma && chan_ena) {
			static int warned = 100;
			// We are still in state=2/3 and program is going to re-enable
			// DMA. Force state to zero to prevent CPU timed DMA wait
			// routines in common tracker players to lose notes.
			if (usehacks() && (currprefs.cachesize || (regs.instruction_cnt - cdp->dmaofftime_cpu_cnt) >= 60)) {
				if (warned >= 0) {
					warned--;
					write_log(_T("Audio %d DMA wait hack ENABLED. OFF=%08x, ON=%08x, PER=%d\n"), nr, cdp->dmaofftime_pc, M68K_GETPC, cdp->evtime / CYCLE_UNIT);
				}
#if DEBUG_AUDIO_HACK > 0
				if (debugchannel(nr))
					write_log(_T("%d: INSTADMAOFF\n"), nr, M68K_GETPC);
#endif
				newsample(nr, (cdp->dat2 >> 0) & 0xff);
				zerostate(nr, true);
			} else {
				if (warned >= 0) {
					warned--;
					write_log(_T("Audio %d DMA wait hack DISABLED. OFF=%08x, ON=%08x, PER=%d\n"), nr, cdp->dmaofftime_pc, M68K_GETPC, cdp->evtime / CYCLE_UNIT);
				}
			}
			cdp->dmaofftime_active = false;
		}
	}

#if DEBUG_AUDIO > 0
	if (debugchannel (nr) && old_dma != chan_ena) {
		write_log (_T("%d:DMA=%d IRQ=%d PC=%08x\n"), nr, chan_ena, isirq (nr) ? 1 : 0, M68K_GETPC);
	}
#endif

	switch (cdp->state)
	{
	case 0:
		if (chan_ena) {
			cdp->evtime = MAX_EV;
			cdp->state = 1;
			setdr(nr, true);
			cdp->wlen = cdp->len;
			cdp->ptx_written = false;
			/* Some programs first start short empty sample and then later switch to
			 * real sample, we must not enable the hack in this case
			 */
			if (cdp->wlen > 2)
				cdp->ptx_tofetch = true;
			cdp->dsr = true;
#if TEST_AUDIO > 0
			cdp->have_dat = false;
#endif
#if DEBUG_AUDIO > 0
			if (debugchannel(nr)) {
				write_log(_T("%d:0>1: LEN=%d PC=%08x\n"), nr, cdp->wlen, M68K_GETPC);
			}
#endif
		} else if (cdp->dat_written && !isirq (nr)) {
			cdp->state = 2;
			setirq(nr, 0);
			loaddat(nr);
			if (usehacks() && cdp->per < 10 * CYCLE_UNIT) {
				static int warned = 100;
				// make sure audio.device AUDxDAT startup returns to idle state before DMA is enabled
				newsample(nr, (cdp->dat2 >> 0) & 0xff);
				zerostate(nr, true);
				if (warned > 0) {
					write_log(_T("AUD%d: forced idle state PER=%d PC=%08x\n"), nr, cdp->per, M68K_GETPC);
					warned--;
				}
			} else {
				cdp->pbufldl = true;
				audio_state_channel2(nr, false);
			}
		} else {
			zerostate(nr, false);
		}
		break;
	case 1:
		cdp->evtime = MAX_EV;
		if (!chan_ena) {
			zerostate(nr, false);
			return true;
		}
		if (!cdp->dat_written)
			return true;
#if TEST_AUDIO > 0
		if (debugchannel(nr) && !cdp->have_dat)
			write_log(_T("%d: state 1 but no have_dat\n"), nr);
		cdp->have_dat = false;
		cdp->losample = cdp->hisample = false;
#endif
		setirq(nr, 10);
		setdr(nr, false);
		if (cdp->wlen != 1)
			cdp->wlen = (cdp->wlen - 1) & 0xffff;
		cdp->state = 5;
		if (sampleripper_enabled)
			do_samplerip(cdp);
		break;
	case 5:
		cdp->evtime = MAX_EV;
		if (!chan_ena) {
			zerostate(nr, false);
			return true;
		}
		if (!cdp->dat_written)
			return true;
#if DEBUG_AUDIO > 0
		if (debugchannel (nr))
			write_log (_T("%d:>5: LEN=%d PT=%08X PC=%08X\n"), nr, cdp->wlen, cdp->pt, M68K_GETPC);
#endif
		if (cdp->ptx_written) {
			cdp->ptx_written = 0;
			cdp->lc = cdp->ptx;
		}
		loaddat(nr);
		if (napnav)
			setdr(nr, false);
		cdp->state = 2;
		loadper(nr);
		cdp->pbufldl = true;
		cdp->intreq2 = false;
		cdp->volcnt = 0;
		audio_state_channel2(nr, false);
		break;
	case 2:
		if (cdp->pbufldl) {
#if TEST_AUDIO > 0
			if (debugchannel(nr) && cdp->hisample == false)
				write_log(_T("%d: high sample used twice\n"), nr);
			cdp->hisample = false;
#endif
			newsample(nr, (cdp->dat2 >> 8) & 0xff);
			loadper(nr);
			cdp->pbufldl = false;
		}
		if (!perfin)
			return true;


#if DEBUG_AUDIO2 > 0
		if (debugchannel(nr)) {
			write_log(_T("%d_2->3: LEN=%d/%d DSR=%d\n"), nr, cdp->wlen, cdp->len, cdp->dsr);
		}
#endif

#if DEBUG_AUDIO > 0
		if (debugchannel(nr) && (cdp->wlen <= 2 || cdp->wlen >= cdp->len - 1))
			write_log(_T("%d_2->3: LEN=%d/%d DSR=%d\n"), nr, cdp->wlen, cdp->len, cdp->dsr);
#endif

		if (audap)
			loaddat(nr, true);
		if (chan_ena) {
			if (audap) {
				setdr(nr, false);
			}
			if (cdp->intreq2 && audap) {
				setirq(nr, 21);
				cdp->intreq2 = false;
			}
		} else {
			if (audap) {
				setirq(nr, 22);
			}
		}
		cdp->pbufldl = true;
		cdp->irqcheck = 0;
		cdp->state = 3;
		audio_state_channel2(nr, false);
		break;

	case 3 + 0x10: // manual audio period==1 cycle
		if (!perfin) {
			return true;
		}
		cdp->state = 3;
		loadper1(nr);
		if (!chan_ena && isirq(nr)) {
			cdp->irqcheck = 1;
		} else {
			cdp->irqcheck = -1;
		}
		return false;

	case 3:
		if (cdp->pbufldl) {
#if TEST_AUDIO > 0
			if (debugchannel(nr) && cdp->losample == false)
				write_log(_T("%d: low sample used twice\n"), nr);
			cdp->losample = false;
#endif
			newsample(nr, (cdp->dat2 >> 0) & 0xff);
			if (chan_ena) {
				loadper(nr);
			} else {
				loadperm1(nr);
			}
			cdp->pbufldl = false;
		}
		if (!perfin)
			return true;

#if DEBUG_AUDIO2 > 0
		if (debugchannel(nr)) {
			write_log(_T("%d_3->2: LEN=%d/%d DSR=%d\n"), nr, cdp->wlen, cdp->len, cdp->dsr);
		}
#endif


#if DEBUG_AUDIO > 0
		if (debugchannel(nr) && (cdp->wlen <= 2 || cdp->wlen >= cdp->len - 1))
			write_log(_T("%d_3->2: LEN=%d/%d DSR=%d\n"), nr, cdp->wlen, cdp->len, cdp->dsr);
#endif

		if (chan_ena) {
			loaddat (nr);
			if (napnav) {
				setdr(nr, false);
			}
			if (cdp->intreq2 && napnav) {
				setirq(nr, 31);
				cdp->intreq2 = false;
			}
		} else {
			// cycle-accurate period check was not needed, do delayed check
			if (!cdp->irqcheck) {
				cdp->irqcheck = isirq(nr);
			}
			if (napnav) {
				setirq(nr, 32);
			}
			if (cdp->irqcheck > 0) {
#if DEBUG_AUDIO > 0
				if (debugchannel (nr))
					write_log(_T("%d: IDLE\n"), nr);
#endif			
				zerostate(nr, false);
				return true;
			}
			loaddat(nr);
		}
		cdp->pbufldl = true;
		cdp->state = 2;
		audio_state_channel2(nr, false);
		break;
	}
	return true;
}

static void audio_state_channel (int nr, bool perfin)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	if (nr < AUDIO_CHANNELS_PAULA) {
		if (audio_state_channel2(nr, perfin)) {
			cdp->dat_written = false;
		}
	} else {
		bool ok = false;
		int streamid = nr - AUDIO_CHANNELS_PAULA + 1;
		struct audio_stream_data *asd = &audio_stream[nr - AUDIO_CHANNELS_PAULA];
		if (asd->cb) {
			ok = asd->cb(streamid, asd->cb_data);
		}
		if (!ok) {
			audio_state_stream_state(streamid, NULL, 0, MAX_EV);
		}
	}
}

void audio_state_machine (void)
{
	update_audio ();
	for (int nr = 0; nr < AUDIO_CHANNELS_PAULA; nr++) {
		struct audio_channel_data *cdp = audio_channel + nr;
		if (audio_state_channel2(nr, false)) {
			cdp->dat_written = false;
		}
	}
	schedule_audio ();
	events_schedule ();
}

void audio_reset (void)
{
	int i;
	struct audio_channel_data *cdp;

#ifdef AHI
	ahi_close_sound();
#ifdef AHI_v2
	free_ahi_v2();
#endif
#endif
	reset_sound ();
	memset (sound_filter_state, 0, sizeof sound_filter_state);
	if (!isrestore ()) {
		for (i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
			cdp = &audio_channel[i];
			memset (cdp, 0, sizeof *audio_channel);
			cdp->per = static_cast<int>(PERIOD_MAX) - 1;
			cdp->data.mixvol = 0;
			cdp->evtime = MAX_EV;
		}
		for (i = 0; i < AUDIO_CHANNEL_STREAMS; i++) {
			audio_stream[i].evtime = MAX_EV;
		}
	}
	audio_total_extra_streams = 0;
	for (int i = 0; i < AUDIO_CHANNEL_STREAMS; i++) {
		audio_extra_streams[i] = 0;
	}
	last_cycles = get_cycles ();
	next_sample_evtime = scaled_sample_evtime;
	schedule_audio ();
	events_schedule ();
}

static int sound_prefs_changed (void)
{
	if (!config_changed)
		return 0;
	if (changed_prefs.produce_sound != currprefs.produce_sound
		|| changed_prefs.soundcard != currprefs.soundcard
#ifdef AMIBERRY
		|| changed_prefs.soundcard_default != currprefs.soundcard_default
#endif
		|| changed_prefs.sound_stereo != currprefs.sound_stereo
		|| changed_prefs.sound_maxbsiz != currprefs.sound_maxbsiz
		|| changed_prefs.sound_freq != currprefs.sound_freq
		|| changed_prefs.sound_volcnt != currprefs.sound_volcnt
		|| changed_prefs.sound_auto != currprefs.sound_auto)
		return 1;

	if (changed_prefs.sound_stereo_separation != currprefs.sound_stereo_separation
		|| changed_prefs.sound_mixed_stereo_delay != currprefs.sound_mixed_stereo_delay
		|| changed_prefs.sound_interpol != currprefs.sound_interpol
		|| changed_prefs.sound_volume_paula != currprefs.sound_volume_paula
		|| changed_prefs.sound_volume_cd != currprefs.sound_volume_cd
		|| changed_prefs.sound_volume_master != currprefs.sound_volume_master
		|| changed_prefs.sound_volume_board != currprefs.sound_volume_board
		|| changed_prefs.sound_volume_midi != currprefs.sound_volume_midi
		|| changed_prefs.sound_stereo_swap_paula != currprefs.sound_stereo_swap_paula
		|| changed_prefs.sound_stereo_swap_ahi != currprefs.sound_stereo_swap_ahi
		|| changed_prefs.sound_filter != currprefs.sound_filter
		|| changed_prefs.sound_filter_type != currprefs.sound_filter_type)
		return -1;
	return 0;
}

double softfloat_tan(double v);

/* This computes the 1st order low-pass filter term b0.
* The a1 term is 1.0 - b0. The center frequency marks the -3 dB point. */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
static float rc_calculate_a0 (int sample_rate, int cutoff_freq)
{
	float omega;
	/* The BLT correction formula below blows up if the cutoff is above nyquist. */
	if (cutoff_freq >= sample_rate / 2)
		return 1.0;

	omega = 2.0f * M_PI * cutoff_freq / sample_rate;
	/* Compensate for the bilinear transformation. This allows us to specify the
	* stop frequency more exactly, but the filter becomes less steep further
	* from stopband. */
	omega = (float)softfloat_tan (omega / 2.0f) * 2.0f;
	float out = 1.0f / (1.0f + 1.0f / omega);
	return out;
}

void check_prefs_changed_audio (void)
{
	int ch;

	if (sound_available) {
		ch = sound_prefs_changed ();
		if (ch > 0) {
#ifdef AVIOUTPUT
			AVIOutput_Restart(true);
#endif
			clear_sound_buffers ();
		}
		if (ch) {
			set_audio ();
			audio_activate ();
		}
	}
#ifdef DRIVESOUND
	driveclick_check_prefs ();
#endif
}

static void set_extra_prehandler(void)
{
	if (audio_total_extra_streams && sample_prehandler != anti_prehandler) {
		extra_sample_prehandler = anti_prehandler;
	} else {
		extra_sample_prehandler = NULL;
	}
}

void set_audio (void)
{
	int old_mixed_size = mixed_stereo_size;
	int sep, delay;
	int ch;

	ch = sound_prefs_changed ();
	if (ch >= 0)
		close_sound ();

	currprefs.produce_sound = changed_prefs.produce_sound;
	currprefs.soundcard = changed_prefs.soundcard;
#ifdef AMIBERRY
	currprefs.soundcard_default = changed_prefs.soundcard_default;
#endif
	currprefs.sound_stereo = changed_prefs.sound_stereo;
	active_sound_stereo = currprefs.sound_stereo;
	currprefs.sound_auto = changed_prefs.sound_auto;
	currprefs.sound_freq = changed_prefs.sound_freq;
	currprefs.sound_maxbsiz = changed_prefs.sound_maxbsiz;
	currprefs.sound_volcnt = changed_prefs.sound_volcnt;

	currprefs.sound_stereo_separation = changed_prefs.sound_stereo_separation;
	currprefs.sound_mixed_stereo_delay = changed_prefs.sound_mixed_stereo_delay;
	currprefs.sound_interpol = changed_prefs.sound_interpol;
	currprefs.sound_filter = changed_prefs.sound_filter;
	currprefs.sound_filter_type = changed_prefs.sound_filter_type;
	currprefs.sound_volume_paula = changed_prefs.sound_volume_paula;
	currprefs.sound_volume_master = changed_prefs.sound_volume_master;
	currprefs.sound_volume_board = changed_prefs.sound_volume_board;
	currprefs.sound_volume_midi = changed_prefs.sound_volume_midi;
	currprefs.sound_volume_cd = changed_prefs.sound_volume_cd;
	currprefs.sound_stereo_swap_paula = changed_prefs.sound_stereo_swap_paula;
	currprefs.sound_stereo_swap_ahi = changed_prefs.sound_stereo_swap_ahi;

	sound_cd_volume[0] = sound_cd_volume[1] = (100 - (currprefs.sound_volume_cd < 0 ? 0 : currprefs.sound_volume_cd)) * 32768 / 100;
	sound_paula_volume[0] = sound_paula_volume[1] = (100 - currprefs.sound_volume_paula) * 32768 / 100;
#ifdef WITH_SNDBOARD
	sndboard_ext_volume();
#endif

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
		else if (currprefs.sound_filter_type == FILTER_SOUND_TYPE_A500_FIXEDONLY)
			sound_use_filter = FILTER_MODEL_A500_FIXEDONLY;
	}
	a500e_filter1_a0 = rc_calculate_a0 (currprefs.sound_freq, 6200);
	a500e_filter2_a0 = rc_calculate_a0 (currprefs.sound_freq, 20000);
	filter_a0 = rc_calculate_a0 (currprefs.sound_freq, 7000);
	memset (sound_filter_state, 0, sizeof sound_filter_state);
	led_filter_audio ();

	makefir();

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
	} else if (sample_handler == sample16ss_handler
		|| sample_handler == sample16ss_sinc_handler
		|| sample_handler == sample16ss_anti_handler)
	{
		sample_handler = (currprefs.sound_interpol == 0 ? sample16ss_handler
			: currprefs.sound_interpol == 3 ? sample16ss_handler
			: currprefs.sound_interpol == 4 ? sample16ss_handler
			: currprefs.sound_interpol == 2 ? sample16ss_sinc_handler
			: sample16ss_anti_handler);
	}
	sample_prehandler = NULL;
	if (sample_handler == sample16si_sinc_handler || sample_handler == sample16i_sinc_handler || sample_handler == sample16ss_sinc_handler) {
		sample_prehandler = sinc_prehandler_paula;
		sound_use_filter_sinc = sound_use_filter;
		sound_use_filter = 0;
	} else if (sample_handler == sample16si_anti_handler || sample_handler == sample16i_anti_handler || sample_handler == sample16ss_anti_handler) {
		sample_prehandler = anti_prehandler;
	}
	for (int i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
		struct audio_channel_data *cdp = audio_channel + i;
		audio_data[i] = &cdp->data;
		if (currprefs.sound_volcnt) {
			cdp->data.mixvol = 1;
		} else {
			cdp->data.mixvol = 0;
		}
	}
	set_extra_prehandler();

	if (currprefs.produce_sound == 0) {
		eventtab[ev_audio].active = 0;
		events_schedule ();
	} else {
		audio_activate ();
		schedule_audio ();
		events_schedule ();
	}
	set_config_changed ();
	cd_audio_mode_changed = true;
}

static void update_audio_volcnt(int cycles, float evtime, bool nextsmp)
{
	if (cycles) {
		for (int i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
			struct audio_channel_data *cdp = audio_channel + i;
			sIntFlt v;
			v.U32 = ((cdp->data.new_sample ^ 0x80) << 15) | 0x40000000;
			v.F32 -= 3.0;
			int cycs = cycles;
			while (cycs > 0) {
				if (cdp->volcnt < cdp->data.audvol) {
					cdp->volcntbuf[cdp->volcntbufcnt] = v.F32;
				} else {
					cdp->volcntbuf[cdp->volcntbufcnt] = 0;
				}
				cdp->volcntbufcnt++;
				cdp->volcntbufcnt &= (VOLCNT_BUFFER_SIZE - 1);
				cdp->volcnt++;
				cdp->volcnt &= 63;
				cycs -= CYCLE_UNIT;
			}
		}
	}
	if (!nextsmp)
		return;
	float frac = evtime - (int)evtime;
	for (int i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
		struct audio_channel_data *cdp = audio_channel + i;
		float out0 = 0, out1 = 0;
		int offs = (cdp->volcntbufcnt - FIR_WIDTH - 1) & (VOLCNT_BUFFER_SIZE - 1);
		float v = cdp->volcntbuf[offs];
		for (int j = 1; j < 2 * FIR_WIDTH - 1; j++) {
			float w = firmem[j];
			out0 += v * w;
			offs++;
			offs &= (VOLCNT_BUFFER_SIZE - 1);
			v = cdp->volcntbuf[offs];
			out1 += v * w;
		}
		float out = out0 + frac * (out1 - out0);
		out *= 8192;

		cdp->data.last_sample = cdp->data.current_sample;
		cdp->data.current_sample = (int)out;
	}
	(*sample_handler) ();
}

void update_audio (void)
{
	int n_cycles = 0;
#if SOUNDSTUFF > 1
	static int samplecounter;
#endif

	if (!isaudio ())
		goto end;
	if (isrestore ())
		goto end;
	if (!is_audio_active ())
		goto end;

	n_cycles = (int)(get_cycles () - last_cycles);
	while (n_cycles > 0) {
		uae_u32 best_evtime = n_cycles + 1;
		uae_u32 rounded;
		int i;

		for (i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
			if (audio_channel[i].evtime != MAX_EV && best_evtime > audio_channel[i].evtime)
				best_evtime = audio_channel[i].evtime;
		}
		for (i = 0; i < audio_total_extra_streams; i++) {
			if (audio_stream[i].evtime != MAX_EV && best_evtime > audio_stream[i].evtime)
				best_evtime= audio_stream[i].evtime;
		}

		/* next_sample_evtime >= 0 so floor() behaves as expected */
		rounded = (uae_u32)floorf (next_sample_evtime);
		float nevtime = next_sample_evtime;
		if ((next_sample_evtime - rounded) >= 0.5)
			rounded++;

		if (currprefs.produce_sound > 1 && best_evtime > rounded)
			best_evtime = rounded;

		if (best_evtime > n_cycles)
			best_evtime = n_cycles;

		/* Decrease time-to-wait counters */
		next_sample_evtime -= best_evtime;

		if (currprefs.produce_sound > 1) {
			if (sample_prehandler)
				sample_prehandler (best_evtime / CYCLE_UNIT);
			if (extra_sample_prehandler)
				extra_sample_prehandler(best_evtime / CYCLE_UNIT);
		}

		for (i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
			if (audio_channel[i].evtime != MAX_EV)
				audio_channel[i].evtime -= best_evtime;
		}
		for (i = 0; i < audio_total_extra_streams; i++) {
			if (audio_stream[i].evtime != MAX_EV)
				audio_stream[i].evtime -= best_evtime;
		}

		n_cycles -= best_evtime;

		if (currprefs.produce_sound > 1) {
			if (currprefs.sound_volcnt) {
				bool nextsmp = false;
				if (rounded == best_evtime) {
					next_sample_evtime += scaled_sample_evtime;
					nextsmp = true;
				}
				update_audio_volcnt(best_evtime, nevtime, nextsmp);
			} else {
				/* Test if new sample needs to be outputted */
				if (rounded == best_evtime) {
					/* Before the following addition, next_sample_evtime is in range [-0.5, 0.5) */
					next_sample_evtime += scaled_sample_evtime;
#if SOUNDSTUFF > 1
					next_sample_evtime -= extrasamples * 15;
					doublesample = 0;
					if (--samplecounter <= 0) {
						samplecounter = currprefs.sound_freq / 1000;
						if (extrasamples > 0) {
							outputsample = 1;
							doublesample = 1;
							extrasamples--;
						} else if (extrasamples < 0) {
							outputsample = 0;
							doublesample = 0;
							extrasamples++;
						}
					}
#endif
					(*sample_handler) ();
#if SOUNDSTUFF > 1
					if (outputsample == 0)
						outputsample = -1;
					else if (outputsample < 0)
						outputsample = 1;
#endif

				}
			}
		}

		for (i = 0; i < AUDIO_CHANNELS_PAULA; i++) {
			if (audio_channel[i].evtime == 0) {
				audio_state_channel (i, true);
			}
		}
		for (i = 0; i < audio_total_extra_streams; i++) {
			if (audio_stream[i].evtime == 0) {
				audio_state_channel(i + AUDIO_CHANNELS_PAULA, true);
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
	if (!currprefs.produce_sound)
		return;
	if (!isaudio ())
		return;
	if (audio_work_to_do > 0 && currprefs.sound_auto && !audio_total_extra_streams
#ifdef AVIOUTPUT
			&& !avioutput_enabled
#endif
			) {
		audio_work_to_do--;
		if (audio_work_to_do == 0)
			audio_deactivate ();
	}
	update_audio ();
	previous_volcnt_update = 0;
}

void event_audxdat_func(uae_u32 v)
{
	int nr = v & 3;
	int chan_ena = (v & 0x80) != 0;
	struct audio_channel_data *cdp = audio_channel + nr;
	if ((cdp->state & 15) == 2 || (cdp->state & 15) == 3) {
		if (chan_ena) {
#if DEBUG_AUDIO > 0
			if (debugchannel(nr) && (cdp->wlen >= cdp->len - 1 || cdp->wlen <= 2))
				write_log(_T("AUD%d near loop, IRQ=%d, LC=%08X LEN=%d/%d DSR=%d\n"), nr, isirq(nr) ? 1 : 0, cdp->pt, cdp->wlen, cdp->len, cdp->dsr);
#endif
			if (cdp->wlen == 1) {
				cdp->wlen = cdp->len;
				// if very low period sample repeats, set higher period value to not cause huge performance drop
				if (cdp->per < PERIOD_MIN_LOOP * CYCLE_UNIT) {
					cdp->minperloop++;
					if (cdp->minperloop >= PERIOD_MIN_LOOP_COUNT) {
						cdp->per = PERIOD_MIN_LOOP * CYCLE_UNIT;
					}
				}
				if (!(v & 0x80000000)) {
					cdp->intreq2 = true;
				}
				if (sampleripper_enabled)
					do_samplerip(cdp);
#if DEBUG_AUDIO > 0
				if (debugchannel(nr) && cdp->wlen > 1)
					write_log(_T("AUD%d looped, IRQ=%d, LC=%08X LEN=%d DSR=%d\n"), nr, isirq(nr) ? 1 : 0, cdp->pt, cdp->wlen, cdp->dsr);
#endif
			} else {
				cdp->wlen = (cdp->wlen - 1) & 0xffff;
			}
		} else {
			cdp->dat = v >> 8;
			cdp->dat_written = true;
#if TEST_MANUAL_AUDIO
			if (cdp->mdat_loaded) {
				write_log("CH%d double load\n", nr);
			}
			cdp->mdat_loaded = true;
#endif
		}
	} else {
		cdp->dat = v >> 8;
		cdp->dat_written = true;
		audio_activate();
		update_audio();
		audio_state_channel(nr, false);
		schedule_audio();
		events_schedule();
	}
	cdp->dat_written = false;
}

void AUDxDAT(int nr, uae_u16 v, uaecptr addr)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	int chan_ena = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));

#if DEBUG_AUDIO2
	if (debugchannel(nr)) {
		write_log(_T("AUD%dDAT: %04X ADDR=%08X LEN=%d/%d %d,%d,%d %06X\n"), nr,
			v, addr, cdp->wlen, cdp->len, cdp->state, chan_ena, isirq(nr) ? 1 : 0, M68K_GETPC);
	}
#endif

#if DEBUG_AUDIO > 0
	if (debugchannel (nr) && (DEBUG_AUDIO > 1 || (!chan_ena || addr == 0xffffffff || ((cdp->state & 15) != 2 && (cdp->state & 15) != 3)))) {
		write_log (_T("AUD%dDAT: %04X ADDR=%08X LEN=%d/%d %d,%d,%d %06X\n"), nr,
		v, addr, cdp->wlen, cdp->len, cdp->state, chan_ena, isirq (nr) ? 1 : 0, M68K_GETPC);
	}
#endif
#if TEST_MISSED_DMA
	cdp->dat_loaded = true;
#endif
#if TEST_AUDIO > 0
	if (debugchannel (nr) && cdp->have_dat)
		write_log (_T("%d: audxdat 1=%04x 2=%04x but old dat not yet used\n"), nr, cdp->dat, cdp->dat2);
	cdp->have_dat = true;
#endif
	if (chan_ena) {
		cdp->dat = v;
		cdp->dat_written = true;
	}
	uae_u32 vv = nr | (chan_ena ? 0x80 : 0) | (v << 8);
	if (!currprefs.cachesize && (cdp->per < PERIOD_LOW * CYCLE_UNIT || currprefs.cpu_compatible)) {
		int cyc = 0;
		if (chan_ena) {
			// AUDxLEN is processed after 1 CCK delay
			cyc = 1 * CYCLE_UNIT;
			if ((cdp->state & 15) == 2 || (cdp->state & 15) == 3) {
				// But INTREQ2 is set immediately
				if (cdp->wlen == 1) {
					cdp->intreq2 = true;
				}
				vv |= 0x80000000;
			}
		}
		if (cyc > 0) {
			event2_newevent_xx(-1, cyc, vv, event_audxdat_func);
		} else {
			event_audxdat_func(vv);
		}
	} else {
		event_audxdat_func(vv);
	}
}
void AUDxDAT(int nr, uae_u16 v)
{
	AUDxDAT(nr, v, 0xffffffff);
}

uaecptr audio_getpt(int nr, bool reset)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	uaecptr p = cdp->pt;
	cdp->pt += 2;
	if (reset)
		cdp->pt = cdp->lc;
	cdp->ptx_tofetch = false;
	return p & ~1;
}

void AUDxLCH(int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	audio_activate();
	update_audio();

	// someone wants to update PT but DSR has not yet been processed.
	// too fast CPU and some tracker players: enable DMA, CPU delay, update AUDxPT with loop position
	if (usehacks() && ((cdp->ptx_tofetch && cdp->state == 1) || cdp->ptx_written)) {
		static int warned = 100;
		cdp->ptx = cdp->lc;
		cdp->ptx_written = true;
		if (warned > 0) {
			write_log(_T("AUD%dLCH HACK: %04X %08X (%d) (%d %d %08x)\n"), nr, v, M68K_GETPC, cdp->state, cdp->dsr, cdp->ptx_written, cdp->ptx);
			warned--;
		}
#if DEBUG_AUDIO_HACK > 0
		if (debugchannel (nr))
			write_log (_T("AUD%dLCH HACK: %04X %08X (%d) (%d %d %08x)\n"), nr, v, M68K_GETPC, cdp->state, cdp->dsr, cdp->ptx_written, cdp->ptx);
#endif
	} else {
		cdp->lc = (cdp->lc & 0xffff) | ((uae_u32)v << 16);
#if DEBUG_AUDIO > 0
		if (debugchannel (nr))
			write_log (_T("AUD%dLCH: %04X %08X (%d) (%d %d %08x)\n"), nr, v, M68K_GETPC, cdp->state, cdp->dsr, cdp->ptx_written, cdp->ptx);
#endif
	}
}

void AUDxLCL(int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	audio_activate();
	update_audio();
	if (usehacks() && ((cdp->ptx_tofetch && cdp->state == 1) || cdp->ptx_written)) {
		static int warned = 100;
		cdp->ptx = cdp->lc;
		cdp->ptx_written = true;
		if (warned > 0) {
			write_log(_T("AUD%dLCL HACK: %04X %08X (%d) (%d %d %08x)\n"), nr, v, M68K_GETPC, cdp->state, cdp->dsr, cdp->ptx_written, cdp->ptx);
			warned--;
		}
#if DEBUG_AUDIO_HACK > 0
		if (debugchannel (nr))
			write_log (_T("AUD%dLCL HACK: %04X %08X (%d) (%d %d %08x)\n"), nr, v, M68K_GETPC, cdp->state, cdp->dsr, cdp->ptx_written, cdp->ptx);
#endif
	} else {
		cdp->lc = (cdp->lc & ~0xffff) | (v & 0xFFFE);
#if DEBUG_AUDIO > 0
		if (debugchannel (nr))
			write_log (_T("AUD%dLCL: %04X %08X (%d) (%d %d %08x)\n"), nr, v, M68K_GETPC, cdp->state, cdp->dsr, cdp->ptx_written, cdp->ptx);
#endif
	}
}

void AUDxPER (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;

	audio_activate ();
	update_audio ();

	int per = (v ? v : 65536) * CYCLE_UNIT;
	if (per < PERIOD_MIN * CYCLE_UNIT) {
		/* smaller values would cause extremely high cpu usage */
		per = PERIOD_MIN * CYCLE_UNIT;
	}
	if (per < PERIOD_MIN_NONCE * CYCLE_UNIT && !currprefs.cpu_memory_cycle_exact && cdp->dmaenstore) {
		/* DMAL emulation and low period can cause very very high cpu usage on slow performance PCs
		 * Only do this hack if audio DMA is active.
		 */
		per = PERIOD_MIN_NONCE * CYCLE_UNIT;
	}

	if (cdp->per == PERIOD_MAX - 1 && per != PERIOD_MAX - 1) {
		cdp->evtime = CYCLE_UNIT;
		if (isaudio ()) {
			schedule_audio ();
			events_schedule ();
		}
	}
#if TEST_AUDIO > 0
	cdp->per_original = v;
#endif
	cdp->per = per;
#if DEBUG_AUDIO > 0
	if (debugchannel (nr))
		write_log (_T("AUD%dPER: %d %08X\n"), nr, v, M68K_GETPC);
#endif
}

void AUDxLEN (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;
	audio_activate ();
	update_audio ();
	cdp->len = v;
#if DEBUG_AUDIO > 0
	if (debugchannel (nr))
		write_log (_T("AUD%dLEN: %d %08X\n"), nr, v, M68K_GETPC);
#endif
}

void AUDxVOL (int nr, uae_u16 v)
{
	struct audio_channel_data *cdp = audio_channel + nr;

	audio_activate ();
	update_audio ();
	update_volume(nr, v);
#if DEBUG_AUDIO > 0
	if (debugchannel (nr))
		write_log (_T("AUD%dVOL: %d %08X\n"), nr, v, M68K_GETPC);
#endif
}

void audio_update_adkmasks (void)
{
	static int prevcon = -1;
	unsigned long t = adkcon | (adkcon >> 4);

	audio_channel[0].data.adk_mask = (((t >> 0) & 1) - 1);
	audio_channel[1].data.adk_mask = (((t >> 1) & 1) - 1);
	audio_channel[2].data.adk_mask = (((t >> 2) & 1) - 1);
	audio_channel[3].data.adk_mask = (((t >> 3) & 1) - 1);
	if ((prevcon & 0xff) != (adkcon & 0xff)) {
		audio_activate ();
#if DEBUG_AUDIO > 0
		write_log (_T("ADKCON=%02x %08X\n"), adkcon & 0xff, M68K_GETPC);
#endif
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

void audio_vsync (void)
{
#if 0
#if SOUNDSTUFF > 0
	int max, min;
	int vsync = isvsync ();
	static int lastdir;

	if (1 || !vsync) {
		extrasamples = 0;
		return;
	}

	min = -10 * 10;
	max =  vsync ? 10 * 10 : 20 * 10;
	extrasamples = 0;
	if (gui_data.sndbuf < min) { // +1
		extrasamples = (min - gui_data.sndbuf) / 10;
		lastdir = 1;
	} else if (gui_data.sndbuf > max) { // -1
		extrasamples = (max - gui_data.sndbuf) / 10;
	} else if (gui_data.sndbuf > 1 * 50 && lastdir < 0) {
		extrasamples--;
	} else if (gui_data.sndbuf < -1 * 50 && lastdir > 0) {
		extrasamples++;
	} else {
		lastdir = 0;
	}

	if (extrasamples > 99)
		extrasamples = 99;
	if (extrasamples < -99)
		extrasamples = -99;
#endif
#endif
}

void restore_audio_finish (void)
{
	last_cycles = get_cycles ();
	schedule_audio ();
	events_schedule ();
}

void restore_audio_start(void)
{
	audio_event_reset();
}

uae_u8 *restore_audio (int nr, uae_u8 *src)
{
	struct audio_channel_data *acd = audio_channel + nr;

	zerostate(nr, true);
	acd->state = restore_u8 ();
	acd->data.audvol = restore_u8 ();
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
	acd->per = p ? p * CYCLE_UNIT : static_cast<int>(PERIOD_MAX);
	acd->dat = acd->dat2 = restore_u16 ();
	acd->lc = restore_u32 ();
	acd->pt = restore_u32 ();
	acd->evtime = restore_u32 ();
	if (flags & 0x80)
		acd->drhpos = restore_u8 ();
	else
		acd->drhpos = 1;
	acd->dmaenstore = (dmacon & DMA_MASTER) && (dmacon & (1 << nr));
	if (currprefs.sound_volcnt)
		acd->data.mixvol = 1;
	else
		acd->data.mixvol = acd->data.audvol;
	return src;
}

uae_u8 *save_audio (int nr, size_t *len, uae_u8 *dstptr)
{
	struct audio_channel_data *acd = audio_channel + nr;
	uae_u8 *dst, *dstbak;

	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 100);
	save_u8 (acd->state);
	save_u8 (acd->data.audvol);
	save_u8 (acd->intreq2);
	save_u8 ((acd->dr ? 1 : 0) | (acd->dsr ? 2 : 0) | 0x80);
	save_u16 (acd->len);
	save_u16 (acd->wlen);
	save_u16 (acd->per == PERIOD_MAX ? 0 : acd->per / CYCLE_UNIT);
	save_u16 (acd->dat);
	save_u32 (acd->lc);
	save_u32 (acd->pt);
	save_u32 (acd->evtime);
	save_u8 (acd->drhpos);
	*len = dst - dstbak;
	return dstbak;
}

static void audio_set_extra_channels(void)
{
	int index = AUDIO_CHANNELS_PAULA;
	audio_total_extra_streams = 0;
	for (int i = 0; i < AUDIO_CHANNEL_STREAMS; i++) {
		if (audio_extra_streams[i])
			audio_total_extra_streams++;
		for (int j = 0; j < audio_extra_streams[i]; j++) {
			audio_data[index++] = &audio_stream[i].data[j];
		}
	}
	set_extra_prehandler();
}

int audio_enable_stream(bool enable, int streamid, int ch, SOUND_STREAM_CALLBACK cb, void *cb_data)
{
	if (streamid == 0)
		return 0;
	if (!enable) {
		if (streamid <= 0)
			return 0;
		streamid--;
		struct audio_stream_data *asd = audio_stream + streamid;
		audio_extra_streams[streamid] = 0;
		asd->evtime = MAX_EV;
	} else {
		if (streamid < 0) {
			for (int i = 0; i < AUDIO_CHANNEL_STREAMS; i++) {
				if (!audio_extra_streams[i]) {
					streamid = i;
					break;
				}
			}
			if (streamid < 0)
				return 0;
		}
		audio_extra_streams[streamid] = ch;
		struct audio_stream_data *asd = audio_stream + streamid;
		asd->cb = cb;
		asd->cb_data = cb_data;
		asd->evtime = CYCLE_UNIT;
		for (int i = 0; i < ch; i++) {
			struct audio_channel_data2 *acd = &asd->data[i];
			acd->adk_mask = 0xffffffff;
			acd->mixvol = 1;
		}
		audio_activate();
	}
	audio_set_extra_channels();
	return streamid + 1;
}

void audio_state_stream_state(int streamid, int *samplep, int highestch, unsigned int evt)
{
	streamid--;
	struct audio_stream_data *asd = audio_stream + streamid;
	if (highestch > audio_extra_streams[streamid]) {
		audio_extra_streams[streamid] = highestch;
		audio_set_extra_channels();
	}
	for (int i = 0; i < audio_extra_streams[streamid]; i++) {
		struct audio_channel_data2 *acd = &asd->data[i];
		acd->last_sample = acd->current_sample;
		acd->current_sample = samplep ? samplep[i] : 0;
	}
	asd->evtime = evt;
}

static uae_u32 cda_evt;
static uae_s16 dummy_buffer[4] = { 0 };

void update_cda_sound(float clk)
{
	cda_evt = (uae_u32)(clk * CYCLE_UNIT / 44100.0f);
}

void audio_cda_volume(struct cd_audio_state *cas, int left, int right)
{
	for (int j = 0; j < 2; j++) {
		cas->cda_volume[j] = j == 0 ? left : right;
		cas->cda_volume[j] = sound_cd_volume[j] * cas->cda_volume[j] / 32768;
		if (cas->cda_volume[j])
			cas->cda_volume[j]++;
		if (cas->cda_volume[j] >= 32768)
			cas->cda_volume[j] = 32768;
	}
}

static bool audio_state_cda(int streamid, void *state)
{
	struct cd_audio_state *cas = (struct cd_audio_state*)state;
	if (cas->cda_bufptr >= dummy_buffer && cas->cda_bufptr <= dummy_buffer + 4) {
		audio_enable_stream(false, cas->cda_streamid, 0, NULL, NULL);
		cas->cda_streamid = 0;
		return false;
	}
	if (cas->cda_streamid <= 0)
		return false;
	int samples[2];
	samples[0] = cas->cda_bufptr[0] * cas->cda_volume[0] / 32768;
	samples[1] = cas->cda_bufptr[1] * cas->cda_volume[1] / 32768;
	audio_state_stream_state(streamid, samples, 2, cda_evt);
	cas->cda_bufptr += 2;
	cas->cda_length--;
	if (cas->cda_length <= 0 && cas->cda_next_cd_audio_buffer_callback) {
		cas->cda_next_cd_audio_buffer_callback(cas->cda_userdata, cas->cb_data);
	}
	return true;
}

void audio_cda_new_buffer(struct cd_audio_state *cas, uae_s16 *buffer, int length, int userdata, CDA_CALLBACK next_cd_audio_buffer_callback, void *cb_data)
{
	if (length < 0 && cas->cda_streamid > 0) {
		audio_enable_stream(false, cas->cda_streamid, 0, NULL, cas);
		cas->cda_streamid = 0;
		return;
	}
	if (!buffer) {
		cas->cda_bufptr = dummy_buffer;
		cas->cda_length = 0;
	} else {
		cas->cda_bufptr = buffer;
		cas->cda_length = length;
		cas->cda_userdata = userdata;
		if (cas->cda_streamid <= 0)
			cas->cda_streamid = audio_enable_stream(true, -1, 2, audio_state_cda, cas);
	}
	cas->cda_next_cd_audio_buffer_callback = next_cd_audio_buffer_callback;
	cas->cb_data = cb_data;
	if (cas->cda_streamid > 0)
		audio_activate();
}
