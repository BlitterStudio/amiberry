#!/bin/sh
set -eu

sh tests/test_uaesnd_capture_fifo.sh

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
grep -F -q '#define UAESND_CAP_CAPTURE_BLOCK' src/sndboard.cpp
grep -F -q '#define UAESND_ERR_CAPTURE_BLOCK_ADDRESS 3' src/sndboard.cpp
grep -F -q '#define SNDBOARD_CAPTURE_OWNER_NONE 0' src/sndboard.cpp
grep -F -q '#define SNDBOARD_CAPTURE_OWNER_TOCCATA 1' src/sndboard.cpp
grep -F -q '#define SNDBOARD_CAPTURE_OWNER_UAESND 2' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_CONTROL' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_STATUS' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_AVAILABLE' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_DATA' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_OVERRUNS' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_INTREQ' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_REG_THRESHOLD' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_CONTROL_IRQ_ENABLE 2' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_BLOCK_REG_ADDRESS 0x900' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_BLOCK_REG_FRAMES 0x904' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_BLOCK_REG_DONE 0x908' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_BLOCK_REG_COMMAND 0x90c' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_BLOCK_COMMAND_COPY 1' src/sndboard.cpp
grep -F -q '#define UAESND_CAPTURE_STATUS_OVERRUN 4' src/include/uaesnd_capture_fifo.h
grep -F -q 'UAESND_CAP_DIAGNOSTICS | UAESND_CAP_CAPTURE' src/sndboard.cpp
grep -F -q 'UAESND_CAP_CAPTURE_BLOCK' src/sndboard.cpp
grep -F -q '#include "uaesnd_capture_fifo.h"' src/sndboard.cpp
grep -F -q 'struct uaesnd_capture_state' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u8 *buffer;' src/include/uaesnd_capture_fifo.h
grep -F -q 'int buffer_size;' src/include/uaesnd_capture_fifo.h
grep -F -q 'int read_index;' src/include/uaesnd_capture_fifo.h
grep -F -q 'int write_index;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 overrun_count;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 intreq;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 threshold;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 frame_count;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 dropped_byte_count;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 block_address;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 block_frames;' src/include/uaesnd_capture_fifo.h
grep -F -q 'uae_u32 block_done;' src/include/uaesnd_capture_fifo.h
grep -F -q 'static inline void uaesnd_capture_fifo_write_s16be' src/include/uaesnd_capture_fifo.h
grep -F -q 'static inline bool uaesnd_capture_fifo_read_byte' src/include/uaesnd_capture_fifo.h
grep -F -q 'static inline uae_u32 uaesnd_capture_fifo_copy_block' src/include/uaesnd_capture_fifo.h
grep -F -q 'static void uaesnd_capture_start(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_stop(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_process(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static bool uaesnd_capture_rethink(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_block_copy(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static bool uaesnd_capture_block_addr(uaecptr addr)' src/sndboard.cpp
grep -F -q 'static void uaesnd_capture_refresh_if_empty(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'static uae_u8 uaesnd_capture_read_byte(struct uaesndboard_data *data)' src/sndboard.cpp
grep -F -q 'uaesnd_capture_refresh_if_empty(data);' src/sndboard.cpp
grep -F -q 'uaesnd_capture_fifo_write_s16be(&data->capture, sample)' src/sndboard.cpp
grep -F -q 'sndboard_init_capture(data->capture.frequency, SNDBOARD_CAPTURE_OWNER_UAESND)' src/sndboard.cpp
grep -F -q 'sndboard_free_capture(SNDBOARD_CAPTURE_OWNER_UAESND)' src/sndboard.cpp
grep -F -q 'sndboard_init_capture(data->freq_adjusted, SNDBOARD_CAPTURE_OWNER_TOCCATA)' src/sndboard.cpp
grep -F -q 'sndboard_free_capture(SNDBOARD_CAPTURE_OWNER_TOCCATA)' src/sndboard.cpp
grep -F -q 'static int capture_owner = SNDBOARD_CAPTURE_OWNER_NONE;' src/sndboard.cpp
grep -F -q 'if (capture_started && capture_owner != owner)' src/sndboard.cpp
grep -F -q 'if (capture_owner != owner)' src/sndboard.cpp
grep -F -q 'sndboard_get_buffer(&frames)' src/sndboard.cpp
grep -F -q 'sndboard_release_buffer(buffer, frames)' src/sndboard.cpp
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
grep -F -q 'put_long_host(data->info + 60, data->capture.frame_count);' src/sndboard.cpp
grep -F -q 'put_long_host(data->info + 64, data->capture.dropped_byte_count);' src/sndboard.cpp
grep -F -q 'uaesnd_capture_fifo_clear_status(&data->capture, value);' src/sndboard.cpp
grep -F -q 'uaesnd_set_error(data, UAESND_ERR_CAPTURE_BLOCK_ADDRESS);' src/sndboard.cpp
grep -F -q 'data->capture.block_done = uaesnd_capture_fifo_copy_block' src/sndboard.cpp
grep -F -q 'uaesnd_capture_block_lget(data, addr)' src/sndboard.cpp
grep -F -q 'uaesnd_capture_block_put(data, addr, b, 4)' src/sndboard.cpp
grep -F -q 'uaesnd_capture_fifo_ack_intreq(&data->capture, value);' src/sndboard.cpp
grep -F -q 'bool any_irq = false;' src/sndboard.cpp
grep -F -q 'bool capture_irq = uaesnd_capture_rethink(data);' src/sndboard.cpp
grep -F -q 'bool stream_irq = false;' src/sndboard.cpp
grep -F -q 'if (capture_irq || stream_irq)' src/sndboard.cpp
grep -F -q 'return any_irq;' src/sndboard.cpp
if grep -F -q 'if (!irq)' src/sndboard.cpp; then
	echo "UAESND stream interrupt bookkeeping must not depend on capture IRQ state" >&2
	exit 1
fi
if grep -F -q 'Panning values must be zeros' src/sndboard.cpp; then
	echo "UAESND should accept and apply non-zero panning values" >&2
	exit 1
fi
