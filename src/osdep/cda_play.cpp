#include "sysconfig.h"
#include "sysdeps.h"

#include "audio.h"

#include "cda_play.h"
#include "sounddep/sound.h"
#include "uae.h"


cda_audio::~cda_audio() 
{
  cdaudio_active = false;
  wait(0);
  wait(1);
  for (int i = 0; i < 2; i++) {
    xfree (buffers[i]);
    buffers[i] = NULL;
  }
}

cda_audio::cda_audio(int num_sectors, int sectorsize, int samplerate)
{
  active = false;
  playing = false;
  volume[0] = volume[1] = 0;
  currBuf = 1;

  bufsize = num_sectors * sectorsize;
	this->sectorsize = sectorsize;
  for (int i = 0; i < 2; i++) {
    buffers[i] = xcalloc (uae_u8, num_sectors * ((bufsize + 4095) & ~4095));
  }
  this->num_sectors = num_sectors;
  active = true;
  playing = true;
  cdaudio_active = true;
}

void cda_audio::setvolume(int left, int right) 
{
  for (int j = 0; j < 2; j++) {
    volume[j] = j == 0 ? left : right;
		volume[j] = sound_cd_volume[j] * volume[j] / 32768;
    if (volume[j])
      volume[j]++;
    if (volume[j] >= 32768)
      volume[j] = 32768;
  }
}

bool cda_audio::play(int bufnum) 
{
  if (!active) {
    return false;
  }

  currBuf = bufnum;
  uae_s16 *p = (uae_s16*)(buffers[bufnum]);
  for (int i = 0; i < num_sectors * sectorsize / 4; i++) {
    PUT_CDAUDIO_WORD_STEREO(p[i * 2 + 0] * volume[0] / 32768, p[i * 2 + 1] * volume[1] / 32768);
    check_cdaudio_buffers();
  }

  return cdaudio_catchup();
}

void cda_audio::wait(int bufnum) 
{
  if (!active || !playing) {
    return;
  }
}

bool cda_audio::isplaying(int bufnum)
{
	if (!active || !playing)
		return false;
	return bufnum != currBuf;
}
