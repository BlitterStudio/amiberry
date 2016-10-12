 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Sound emulation stuff
  *
  * Copyright 1995, 1996, 1997 Bernd Schmidt
  */

#define PERIOD_MAX ULONG_MAX

extern void aud0_handler (void);
extern void aud1_handler (void);
extern void aud2_handler (void);
extern void aud3_handler (void);

extern void AUDxDAT (int nr, uae_u16 value);
extern void AUDxVOL (int nr, uae_u16 value);
extern void AUDxPER (int nr, uae_u16 value);
extern void AUDxLCH (int nr, uae_u16 value);
extern void AUDxLCL (int nr, uae_u16 value);
extern void AUDxLEN (int nr, uae_u16 value);

extern uae_u16 audio_dmal (void);
extern void audio_state_machine (void);
extern void audio_dmal_do (int nr, bool reset);

extern int init_audio (void);
extern void audio_reset (void);
extern void update_audio (void);
extern void audio_evhandler (void);
extern void audio_hsync (void);
extern void audio_update_adkmasks (void);
extern void update_sound (float clk);
extern void led_filter_audio (void);
extern void set_audio(void);
extern int audio_activate(void);
extern void audio_deactivate (void);

enum {
    SND_MONO, SND_STEREO, SND_4CH_CLONEDSTEREO, SND_4CH, SND_6CH_CLONEDSTEREO, SND_6CH, SND_NONE };
STATIC_INLINE int get_audio_ismono (int stereomode)
{
    if (stereomode == 0)
	return 1;
    return 0;
}

#define SOUND_MAX_DELAY_BUFFER 1024
#define SOUND_MAX_LOG_DELAY 10
#define MIXED_STEREO_MAX 16
#define MIXED_STEREO_SCALE 32
