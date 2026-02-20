#pragma once

#include <cstdint>

#include "libretro_shared.h"

static inline bool sound_platform_open_audio(struct sound_data* sd, int index)
{
	auto* s = sd->data;
	const auto freq = sd->freq;
	const auto ch = sd->channels;

	s->dev = 1;
	s->pullmode = 0;
	s->pullbuffer = nullptr;
	s->pullbufferlen = 0;
	s->pullbuffermaxlen = 0;
	write_log("LIBRETRO: CH=%d, FREQ=%d '%s' buffer %d/%d\n", ch, freq, sound_devices[index]->name,
		s->sndbufsize, s->framesperbuffer);
	clearbuffer(sd);
	return true;
}

static inline bool sound_platform_output_audio(struct sound_data* sd, uae_u16* sndbuffer)
{
	const auto frames = sd->sndbufsize / (sd->channels * 2);
	if (audio_batch_cb) {
		audio_batch_cb((const int16_t*)sndbuffer, frames);
	} else if (audio_cb) {
		const auto* samples = reinterpret_cast<const int16_t*>(sndbuffer);
		for (int i = 0; i < frames; i++) {
			const int16_t left = samples[i * sd->channels];
			const int16_t right = (sd->channels > 1) ? samples[i * sd->channels + 1] : left;
			audio_cb(left, right);
		}
	}
	return true;
}

static inline bool sound_platform_enumerate_devices()
{
	if (!num_sound_devices) {
		const char* devname = "Libretro";
		sound_devices[0] = xcalloc(struct sound_device, 1);
		sound_devices[0]->id = 0;
		sound_devices[0]->cfgname = my_strdup(devname);
		sound_devices[0]->type = SOUND_DEVICE_SDL2;
		sound_devices[0]->name = my_strdup(devname);
		sound_devices[0]->alname = my_strdup("0");
		num_sound_devices = 1;

		record_devices[0] = xcalloc(struct sound_device, 1);
		record_devices[0]->id = 0;
		record_devices[0]->cfgname = my_strdup(devname);
		record_devices[0]->type = SOUND_DEVICE_SDL2;
		record_devices[0]->name = my_strdup(devname);
		record_devices[0]->alname = my_strdup("0");
		num_record_devices = 1;
	}
	return true;
}
