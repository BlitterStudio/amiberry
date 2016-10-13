#include "sysconfig.h"
#include "sysdeps.h"

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

cda_audio::cda_audio(int num_sectors) 
{
  active = false;
  playing = false;
  volume[0] = volume[1] = 0;

  bufsize = num_sectors * 2352;
  for (int i = 0; i < 2; i++) {
    buffers[i] = xcalloc (uae_u8, num_sectors * 4096);
  }
  this->num_sectors = num_sectors;
  active = true;
  playing = true;
  cdaudio_active = true;
}

void cda_audio::setvolume(int master, int left, int right) 
{
  for (int j = 0; j < 2; j++) {
    volume[j] = j == 0 ? left : right;
    volume[j] = (100 - master) * volume[j] / 100;
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

  uae_s16 *p = (uae_s16*)(buffers[bufnum]);
  for (int i = 0; i < num_sectors * 2352 / 4; i++) {
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
