#pragma once
#include "audio.h"
#include "blkdev.h"
#include "thread.h"

extern volatile bool cd_audio_mode_changed;

class cda_audio
{
private:
	int bufsize;
	int num_sectors;
	int sectorsize;
	int volume[2]{};
	bool playing;
	bool active;

public:
	uae_u8* buffers[2]{};

	cda_audio(int num_sectors, int sectorsize, int samplerate);
	~cda_audio();
	void setvolume(int left, int right);
};

#define CDDA_BUFFERS 14

typedef int (*cda_play_read_block)(struct cda_play*, int unitnum, uae_u8* data, int sector, int size, int sectorsize);

struct cda_play
{
	int unitnum;
	int cdda_volume[2];
	Uint8* cdda_wavehandle;
	int cd_last_pos;
	int cdda_play;
	int cdda_play_finished;
	int cdda_scan;
	int cdda_paused;
	int cdda_start, cdda_end;
	play_subchannel_callback cdda_subfunc;
	play_status_callback cdda_statusfunc;
	int cdda_play_state;
	int cdda_delay, cdda_delay_frames;
	cda_audio* cda;
	volatile int cda_bufon[2];
	struct cd_audio_state cas;
	bool subcodevalid;
	uae_sem_t sub_sem, sub_sem2;
	uae_u8 subcode[SUB_CHANNEL_SIZE * CDDA_BUFFERS];
	uae_u8 subcodebuf[SUB_CHANNEL_SIZE];
	struct device_info* di;
	cda_play_read_block read_block;
};

int ciw_cdda_play(void* ciw);
void ciw_cdda_stop(struct cda_play* ciw);
int ciw_cdda_setstate(struct cda_play* ciw, int state, int playpos);