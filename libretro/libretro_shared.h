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

void libretro_yield(void);

#endif

#endif
