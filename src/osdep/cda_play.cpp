#include "sysconfig.h"
#include "sysdeps.h"

#include "cda_play.h"
#include "audio.h"
#include "options.h"
#include "uae.h"

#include "sounddep/sound.h"
#include "uae/uae.h"

bool pull_mode = true;

void sdl2_cdaudio_callback(void* userdata, Uint8* stream, int len);
SDL_AudioDeviceID cdda_dev;
int pull_buffer_len[2];
uae_u8* pull_buffer[2];

cda_audio::~cda_audio()
{
	wait(0);
	wait(1);

	SDL_LockAudioDevice(cdda_dev);
	for (auto& i : pull_buffer_len)
		i = 0;
	SDL_UnlockAudioDevice(cdda_dev);
	
	for (auto& buffer : buffers)
	{
		xfree(buffer);
		buffer = nullptr;
	}
}

cda_audio::cda_audio(int num_sectors, int sectorsize, int samplerate, bool internalmode)
{
	active = false;
	playing = false;
	volume[0] = volume[1] = 0;

	bufsize = num_sectors * sectorsize;
	this->sectorsize = sectorsize;
	for (auto i = 0; i < 2; i++) {
		buffers[i] = xcalloc(uae_u8, num_sectors * ((bufsize + 4095) & ~4095));
	}
	this->num_sectors = num_sectors;

	if (internalmode)
		return;

	const Uint8 channels = 2;
	SDL_AudioSpec cdda_want, cdda_have;
	memset(&cdda_want, 0, sizeof cdda_want);
	cdda_want.freq = samplerate;
	cdda_want.format = AUDIO_S16;
	cdda_want.channels = channels;
	cdda_want.samples = static_cast<Uint16>(bufsize);
	if (pull_mode)
		cdda_want.callback = sdl2_cdaudio_callback;

	cdda_dev = SDL_OpenAudioDevice(nullptr, 0, &cdda_want, &cdda_have, 0);
	if (cdda_dev == 0)
	{
		write_log("Failed to open SDL2 device for CD-Audio: %s", SDL_GetError());
	}
	else
	{
		for (auto i = 0; i < 2; i++)
		{
			pull_buffer[i] = buffers[i];
			pull_buffer_len[i] = 0;
		}
		
		SDL_PauseAudioDevice(cdda_dev, 0);
		active = true;
		playing = true;
	}
}

void cda_audio::setvolume(int left, int right)
{
	for (auto j = 0; j < 2; j++)
	{
		volume[j] = j == 0 ? left : right;
		volume[j] = sound_cd_volume[j] * volume[j] / 32768;
		if (volume[j])
			volume[j]++;
		volume[j] = volume[j] * (100 - currprefs.sound_volume_master) / 100;
		if (volume[j] >= 32768)
			volume[j] = 32768;
	}
}

bool cda_audio::play(int bufnum)
{
	if (!active)
		return false;

	auto* p = reinterpret_cast<uae_s16*>(buffers[bufnum]);
	if (volume[0] != 32768 || volume[1] != 32768) {
		for (int i = 0; i < num_sectors * sectorsize / 4; i++) {
			p[i * 2 + 0] = p[i * 2 + 0] * volume[0] / 32768;
			p[i * 2 + 1] = p[i * 2 + 1] * volume[1] / 32768;
		}
	}

	if (pull_mode)
	{
		SDL_LockAudioDevice(cdda_dev);
		pull_buffer_len[bufnum] += bufsize * 2;
		SDL_UnlockAudioDevice(cdda_dev);
	}		
	else
	{
		SDL_QueueAudio(cdda_dev, p, bufsize * 2);
	}
	
	return true;
}

void cda_audio::wait(int bufnum)
{
	if (!active || !playing)
		return;

	if (pull_mode)
	{
		while (pull_buffer_len[bufnum] > 0 && quit_program == 0)
			Sleep(10);
	}
	else
	{
		while (SDL_GetQueuedAudioSize(cdda_dev) > 0 && quit_program == 0)
			Sleep(10);
	}
}

bool cda_audio::isplaying(int bufnum)
{
	if (!active || !playing)
		return false;
	
	if (pull_mode)
		return pull_buffer_len[bufnum] > 0;
	
	return SDL_GetQueuedAudioSize(cdda_dev) > 0;
}

void sdl2_cdaudio_callback(void* userdata, Uint8* stream, int len)
{
	for (auto i = 0; i < 2; ++i)
	{
		if (pull_buffer_len[i] == 0)
			continue;
		
		memcpy(stream, pull_buffer[i], len / 2);
		stream += len / 2;

		pull_buffer_len[i] = 0;
	}
}