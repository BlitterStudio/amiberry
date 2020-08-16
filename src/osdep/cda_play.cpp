#include "sysconfig.h"
#include "sysdeps.h"

#include "cda_play.h"
#include "audio.h"
#include "options.h"

#include "sounddep/sound.h"
#include "uae/uae.h"

cda_audio::~cda_audio()
{
	wait(0);
	wait(1);

	cdaudio_active = false;
	
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
	for (auto& buffer : buffers)
	{
		buffer = xcalloc(uae_u8, num_sectors * ( bufsize + 4095 & ~4095));
	}
	this->num_sectors = num_sectors;

	if (internalmode)
		return;

	cdaudio_active = true;
	
	active = true;
	playing = true;
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

	auto* const p = reinterpret_cast<uae_s16*>(buffers[bufnum]);
	for (auto i = 0; i < num_sectors * sectorsize / 4; i++) {
		p[i * 2 + 0] = p[i * 2 + 0] * volume[0] / 32768;
		p[i * 2 + 1] = p[i * 2 + 1] * volume[1] / 32768;
		//PUT_CDAUDIO_WORD_STEREO(p[i * 2 + 0] * volume[0] / 32768, p[i * 2 + 1] * volume[1] / 32768);
		//check_cdaudio_buffers();
	}
	
	return true;
	//return cdaudio_catchup();
}

void cda_audio::wait(int bufnum)
{
	if (!active || !playing)
		return;

	while (SDL_GetAudioDeviceStatus(dev) != SDL_AUDIO_PLAYING) {
		Sleep(10);
	}
}

bool cda_audio::isplaying(int bufnum)
{
	if (!active || !playing)
		return false;

	return SDL_GetAudioDeviceStatus(dev) == SDL_AUDIO_PLAYING;
}
