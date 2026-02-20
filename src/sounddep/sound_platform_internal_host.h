#pragma once

static inline bool sound_platform_open_audio(struct sound_data* sd, int index)
{
	(void)sd;
	(void)index;
	return false;
}

static inline bool sound_platform_output_audio(struct sound_data* sd, uae_u16* sndbuffer)
{
	(void)sd;
	(void)sndbuffer;
	return false;
}

static inline bool sound_platform_enumerate_devices()
{
	return false;
}
