#!/bin/sh
set -eu

test ! -e src/uaesnd_ahi.s
test ! -e src/osdep/ahi_v2.cpp
test ! -e src/osdep/ahi_v2.h
if rg -n 'ahi_v2|AHI_v2|AHI_V2|uaesnd_ahi' CMakeLists.txt cmake libretro src; then
	echo "obsolete AHI v2 wrapper references must be removed" >&2
	exit 1
fi

grep -F -q 'static int uaesnd_sample_bytes' src/sndboard.cpp
grep -F -q 'case 3: return 4;' src/sndboard.cpp
grep -F -q 's->framesize = uaesnd_sample_bytes(s->bitmode) * s->ch;' src/sndboard.cpp
grep -F -q 'static void uaesnd_apply_mono_pan' src/sndboard.cpp
grep -F -q 's->sample[0] = smp * (32768 - s->panx) / 32768;' src/sndboard.cpp
grep -F -q 's->sample[1] = smp * (32768 + s->panx) / 32768;' src/sndboard.cpp
grep -F -q 'reg == 36 || reg == 38' src/sndboard.cpp
grep -F -q 's->panx = get_word_host(s->io + 36);' src/sndboard.cpp
grep -F -q 's->pany = get_word_host(s->io + 38);' src/sndboard.cpp
grep -F -q 'highestch = i + 1;' src/sndboard.cpp
grep -F -q '#define UAESND_CAP_24_32BIT' src/sndboard.cpp
grep -F -q '#define UAESND_CAP_MONO_HPAN' src/sndboard.cpp
grep -F -q '#define UAESND_CAP_DIAGNOSTICS' src/sndboard.cpp
grep -F -q '#define UAESND_CAP_CAPTURE' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_CONTROL' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_STATUS' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_AVAILABLE' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_DATA' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_OVERRUNS' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_INTREQ' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_THRESHOLD' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_CONTROL_IRQ_ENABLE 2' src/sndboard.cpp
grep -F -q 'UAESND_CAP_DIAGNOSTICS | UAESND_CAP_CAPTURE' src/sndboard.cpp
grep -F -q 'struct uaesnd_capture_state' src/sndboard.cpp
grep -F -q 'uae_u8 *buffer;' src/sndboard.cpp
grep -F -q 'int buffer_size;' src/sndboard.cpp
grep -F -q 'int read_index;' src/sndboard.cpp
grep -F -q 'int write_index;' src/sndboard.cpp
grep -F -q 'uae_u32 overrun_count;' src/sndboard.cpp
grep -F -q 'uae_u32 intreq;' src/sndboard.cpp
grep -F -q 'uae_u32 threshold;' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_start(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_stop(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_process(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static bool uaesnd_capture_rethink(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static uae_u8 uaesnd_capture_read_byte(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_write_s16be' src/sndboard.cpp
grep -F -q 'uaesnd_capture_write_s16be(&data->capture, sample)' src/sndboard.cpp
grep -F -q 'sndboard_init_capture(data->capture.frequency)' src/sndboard.cpp
grep -F -q 'sndboard_get_buffer(&frames)' src/sndboard.cpp
grep -F -q 'sndboard_release_buffer(buffer, frames)' src/sndboard.cpp
grep -F -q 'sndboard_free_capture()' src/sndboard.cpp
grep -F -q '#include <SDL3/SDL.h>' src/sndboard.cpp
grep -F -q 'static SDL_AudioStream *capture_stream;' src/sndboard.cpp
grep -F -q 'SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING' src/sndboard.cpp
grep -F -q 'SDL_GetAudioStreamAvailable(capture_stream)' src/sndboard.cpp
grep -F -q 'SDL_GetAudioStreamData(capture_stream, capture_buffer, bytes)' src/sndboard.cpp
grep -F -q 'spec.format = SDL_AUDIO_S16LE;' src/sndboard.cpp
grep -F -q 'SDL_DestroyAudioStream(capture_stream)' src/sndboard.cpp
grep -F -q 'uae_u32 invalid_set_count;' src/sndboard.cpp
grep -F -q 'uae_u32 stream_alloc_failure_count;' src/sndboard.cpp
grep -F -q 'uae_u32 stream_start_count;' src/sndboard.cpp
grep -F -q 'uae_u32 stream_stop_count;' src/sndboard.cpp
grep -F -q 'uae_u32 stream_irq_count;' src/sndboard.cpp
grep -F -q 'uae_u32 timer_irq_count;' src/sndboard.cpp
grep -F -q 'uae_u32 last_error_code;' src/sndboard.cpp
grep -F -q 'static void uaesnd_update_info(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'put_long_host(data->info + 24, UAESND_CAP_' src/sndboard.cpp
grep -F -q 'put_long_host(data->info + 32, data->invalid_set_count);' src/sndboard.cpp
grep -F -q 'put_long_host(data->info + 52, data->timer_irq_count);' src/sndboard.cpp
if grep -F -q 'Panning values must be zeros' src/sndboard.cpp; then
	echo "UAESND should accept and apply non-zero panning values" >&2
	exit 1
fi
