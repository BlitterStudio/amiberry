#include "sysconfig.h"
#include "sysdeps.h"

#include "cda_play.h"
#include "audio.h"
#include "options.h"

#include "sounddep/sound.h"
#include "uae/uae.h"

SDL_AudioDeviceID cdda_dev;

cda_audio::~cda_audio()
{
	wait(0);
	wait(1);

	if (SDL_GetAudioDeviceStatus(cdda_dev) != SDL_AUDIO_STOPPED)
		SDL_CloseAudioDevice(cdda_dev);

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

	SDL_AudioSpec cdda_want, cdda_have;
	memset(&cdda_want, 0, sizeof cdda_want);
	cdda_want.freq = samplerate;
	cdda_want.format = AUDIO_S16;
	cdda_want.channels = 2;
	cdda_want.samples = num_sectors * sectorsize;

	cdda_dev = SDL_OpenAudioDevice(nullptr, 0, &cdda_want, &cdda_have, 0);
	if (cdda_dev == 0)
		write_log("Failed to open SDL2 device for CD-Audio: %s", SDL_GetError());
	else
	{
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

	SDL_QueueAudio(cdda_dev, p, num_sectors * sectorsize);
	
	return true;
}

void cda_audio::wait(int bufnum)
{
	if (!active || !playing)
		return;

	while (SDL_GetQueuedAudioSize(cdda_dev) > 0) {
		Sleep(10);
	}
}

bool cda_audio::isplaying(int bufnum)
{
	if (!active || !playing)
		return false;
	
	return SDL_GetQueuedAudioSize(cdda_dev) > 0;
}
