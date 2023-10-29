/* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Linux/USS sound
  * 
  * Copyright 1997 Bernd Schmidt
  */

#pragma once
#include "audio.h"

#define SOUNDSTUFF 1

extern uae_u16 paula_sndbuffer[];
extern uae_u16* paula_sndbufpt;
extern int paula_sndbufsize;

extern void finish_sound_buffer(void);
extern void restart_sound_buffer(void);
extern void pause_sound_buffer(void);
extern int init_sound(void);
extern void close_sound(void);
extern int setup_sound(void);
extern void resume_sound(void);
extern void pause_sound(void);
extern void reset_sound(void);
extern bool sound_paused(void);
extern void sound_setadjust(float);
extern int enumerate_sound_devices(void);
extern void sound_mute(int);
extern void sound_volume(int);
extern void set_volume(int, int);
extern void master_sound_volume(int);

struct sound_dp;

struct sound_data
{
	int waiting_for_buffer;
	int deactive;
	int devicetype;
	int obtainedfreq;
	int paused;
	int mute;
	int channels;
	int freq;
	int samplesize;
	int sndbufsize;
	int sndbufframes;
	int softvolume;
	struct sound_dp* data;
	int index;
	bool reset;
	int resetcnt;
	int resetframe;
	int resetframecnt;
};

int open_sound_device(struct sound_data* sd, int index, int exclusive, int bufsize, int freq, int channels);
void close_sound_device(struct sound_data* sd);
void pause_sound_device(struct sound_data* sd);
void resume_sound_device(struct sound_data* sd);
void set_volume_sound_device(struct sound_data* sd, int volume, int mute);

int sound_get_silence();

extern int active_sound_stereo;

#define PUT_SOUND_WORD(b) do { *(uae_u16 *)paula_sndbufpt = b; paula_sndbufpt = (uae_u16 *)(((uae_u8 *)paula_sndbufpt) + 2); } while (0)
#define PUT_SOUND_WORD_MONO(b) PUT_SOUND_WORD(b)
#define SOUND16_BASE_VAL 0
#define SOUND8_BASE_VAL 128

#define DEFAULT_SOUND_MAXB 16384
#define DEFAULT_SOUND_MINB 16384
#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define HAVE_STEREO_SUPPORT

#define FILTER_SOUND_OFF 0
#define FILTER_SOUND_EMUL 1
#define FILTER_SOUND_ON 2

#define FILTER_SOUND_TYPE_A500 0
#define FILTER_SOUND_TYPE_A1200 1