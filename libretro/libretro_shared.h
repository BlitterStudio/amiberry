#ifndef LIBRETRO_SHARED_H
#define LIBRETRO_SHARED_H

#include "libretro.h"

#ifdef LIBRETRO
#include "libco/libco.h"

extern cothread_t main_fiber;
extern cothread_t core_fiber;

extern retro_video_refresh_t video_cb;
extern retro_audio_sample_t audio_cb;
extern retro_audio_sample_batch_t audio_batch_cb;
extern retro_input_poll_t input_poll_cb;
extern retro_input_state_t input_state_cb;
extern retro_log_printf_t log_cb;
extern bool pixel_format_xrgb8888;
extern unsigned libretro_audio_frames_this_run;

void libretro_yield(void);
void reset_parse_cmdline(void);
void libretro_audio_enqueue(const int16_t* stereo_frames, unsigned frames);
void libretro_audio_reset(void);

struct libretro_crop {
	int x;
	int y;
	int w;
	int h;
	float aspect;
	bool active;
};

// Computes the content-aware overscan crop for monitor 0 when the
// amiberry_crop_overscan option is enabled. Returns active=false (and callers
// fall back to the full frame) when cropping is off, no surface exists, RTG is
// active, or the detected limits are degenerate.
libretro_crop libretro_compute_crop(void);

#endif

#endif
