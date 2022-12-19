#pragma once
#include "audio.h"

extern volatile bool cd_audio_mode_changed;
extern SDL_AudioDeviceID cdda_dev;

class cda_audio
{
private:
	int bufsize;
	int num_sectors;
	int sectorsize;
	int volume[2];

	bool playing;
	bool active;

public:
	uae_u8* buffers[2];

	cda_audio(int num_sectors, int sectorsize, int samplerate, bool internalmode);
	~cda_audio();
	void setvolume(int left, int right);
	bool play(int bufnum);
	void wait(void);
	void wait(int bufnum);
	bool isplaying(int bufnum);
};

#define CDDA_BUFFERS 14