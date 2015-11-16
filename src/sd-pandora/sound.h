/* 
  * UAE - The Un*x Amiga Emulator
  * 
  * Support for Linux/USS sound
  * 
  * Copyright 1997 Bernd Schmidt
  */

#if defined(PANDORA) || defined(ANDROIDSDL)
#define DEFAULT_SOUND_CHANNELS 2
#else
#define DEFAULT_SOUND_CHANNELS 1
#endif

#define SNDBUFFER_LEN 2048

extern uae_u16 sndbuffer[4][(SNDBUFFER_LEN+32)*DEFAULT_SOUND_CHANNELS];
extern uae_u16 *sndbufpt;
extern uae_u16 *render_sndbuff;
extern uae_u16 *finish_sndbuff;
extern int sndbufsize;
extern void finish_sound_buffer (void);
extern void restart_sound_buffer (void);
extern int init_sound(void);
extern void close_sound (void);
extern int setup_sound (void);
extern void resume_sound (void);
extern void pause_sound (void);
extern void reset_sound (void);
extern void sound_volume (int);

#define check_sound_buffers() { if (sndbufpt >= finish_sndbuff) finish_sound_buffer (); }

STATIC_INLINE void clear_sound_buffers (void)
{
    memset (sndbuffer, 0, 4 * (SNDBUFFER_LEN + 32) * DEFAULT_SOUND_CHANNELS);
}

#define PUT_SOUND_WORD(b) do { *(uae_u16 *)sndbufpt = b; sndbufpt = (uae_u16 *)(((uae_u8 *)sndbufpt) + 2); } while (0)
#define PUT_SOUND_WORD_LEFT(b) do { if (currprefs.sound_filter) b = filter (b, &sound_filter_state[0]); PUT_SOUND_WORD(b); } while (0)
#define PUT_SOUND_WORD_RIGHT(b) do { if (currprefs.sound_filter) b = filter (b, &sound_filter_state[1]); PUT_SOUND_WORD(b); } while (0)
#define PUT_SOUND_WORD_MONO(b) PUT_SOUND_WORD_LEFT(b)
#define SOUND16_BASE_VAL 0

#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100

#define FILTER_SOUND_OFF 0
#define FILTER_SOUND_EMUL 1
#define FILTER_SOUND_ON 2

#define FILTER_SOUND_TYPE_A500 0
#define FILTER_SOUND_TYPE_A1200 1
