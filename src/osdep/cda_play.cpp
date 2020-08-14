#include "sysconfig.h"
#include "sysdeps.h"

#include "cda_play.h"
#include "audio.h"
#include "options.h"

#include "sounddep/sound.h"
#include "uae/uae.h"

static int (*g_audio_callback)(int type, int16_t* buffer, int size) = NULL;

int amiga_set_cd_audio_callback(audio_callback func) {
	g_audio_callback = func;
	return 1;
}

cda_audio::~cda_audio()
{
	wait(0);
	wait(1);
	
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
	for (int i = 0; i < 2; i++)
	{
		buffer_ids[i] = 0;
		buffers[i] = xcalloc(uae_u8, num_sectors * ((bufsize + 4095) & ~4095));
	}
	this->num_sectors = num_sectors;

	if (internalmode)
		return;
	
	active = true;
	playing = true;
}

void cda_audio::setvolume(int left, int right)
{
	for (int j = 0; j < 2; j++)
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

	uae_s16* p = (uae_s16*)(buffers[bufnum]);
	for (int i = 0; i < num_sectors * sectorsize / 4; i++) {
		p[i * 2 + 0] = p[i * 2 + 0] * volume[0] / 32768;
		p[i * 2 + 1] = p[i * 2 + 1] * volume[1] / 32768;
	}

	if (g_audio_callback) {
		int len = num_sectors * sectorsize;
#ifdef WORDS_BIGENDIAN
		int8_t* d = (int8_t*)p;
		int8_t temp = 0;
		for (int i = 0; i < len; i += 2) {
			temp = d[i + 1];
			d[i + 1] = d[i];
			d[i] = temp;
		}
#endif
		buffer_ids[bufnum] = g_audio_callback(3, p, len);
	}
	else {
		buffer_ids[bufnum] = 0;
	}
	
	return true;
}

void cda_audio::wait(int bufnum)
{
	if (!active || !playing)
		return;

	if (buffer_ids[bufnum] == 0) {
		return;
	}

	// calling g_audio_callback with NULL parameter to check status
	while (!g_audio_callback(3, NULL, buffer_ids[bufnum])) {
		Sleep(10);
	}
}

bool cda_audio::isplaying(int bufnum)
{
	if (!active || !playing)
		return false;
	if (buffer_ids[bufnum] == 0) {
		return false;
	}
	return g_audio_callback(3, NULL, buffer_ids[bufnum]);
}
