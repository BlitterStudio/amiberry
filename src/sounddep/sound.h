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
extern void pause_sound_buffer (void);
extern void finish_cdaudio_buffer (void);
extern bool cdaudio_catchup(void);
extern int init_sound(void);
extern void close_sound (void);
extern int setup_sound (void);
extern void resume_sound (void);
extern void pause_sound (void);
extern void reset_sound (void);
extern void sound_volume (int);

STATIC_INLINE void set_sound_buffers (void)
{
}

#define check_sound_buffers() { if (sndbufpt >= finish_sndbuff) finish_sound_buffer (); }

STATIC_INLINE void clear_sound_buffers (void)
{
    memset (sndbuffer, 0, 4 * (SNDBUFFER_LEN + 32) * DEFAULT_SOUND_CHANNELS);
}

#define PUT_SOUND_WORD_MONO(x) put_sound_word_mono_func(x)

#define PUT_SOUND_WORD(b) do { *sndbufpt = b; sndbufpt = sndbufpt + 1; } while (0)
#define PUT_SOUND_WORD_STEREO(l,r) do { *((uae_u32 *)sndbufpt) = (r << 16) | (l & 0xffff); sndbufpt = sndbufpt + 2; } while (0)

#define DEFAULT_SOUND_BITS 16
#define DEFAULT_SOUND_FREQ 44100
#define HAVE_STEREO_SUPPORT

#define FILTER_SOUND_OFF 0
#define FILTER_SOUND_EMUL 1
#define FILTER_SOUND_ON 2

#define FILTER_SOUND_TYPE_A500 0
#define FILTER_SOUND_TYPE_A1200 1


#define CDAUDIO_BUFFERS 32
#define CDAUDIO_BUFFER_LEN 2048
extern uae_u16 cdaudio_buffer[CDAUDIO_BUFFERS][(CDAUDIO_BUFFER_LEN + 32) * 2];
extern uae_u16 *cdbufpt;
extern uae_u16 *render_cdbuff;
extern uae_u16 *finish_cdbuff;
extern bool cdaudio_active;

#define check_cdaudio_buffers() { if (cdbufpt >= finish_cdbuff) finish_cdaudio_buffer (); }

STATIC_INLINE void clear_cdaudio_buffers (void)
{
    memset (cdaudio_buffer, 0, CDAUDIO_BUFFERS * (CDAUDIO_BUFFER_LEN + 32) * 2);
}

#define PUT_CDAUDIO_WORD_STEREO(l,r) do { *((uae_u32 *)cdbufpt) = (r << 16) | (l & 0xffff); cdbufpt = cdbufpt + 2; } while (0)
