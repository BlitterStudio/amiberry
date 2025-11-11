/*
 * Parallel port audio digitizer
 *
 * Toni Wilen 2010
 */

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "events.h"
#include "sampler.h"

#include <sounddep/sound.h>

#include <math.h>

//#include "win32.h"

#define SAMPLESIZE 4

//static LPDIRECTSOUNDCAPTURE lpDS2r = NULL;
static SDL_AudioDeviceID dev_rec;
SDL_AudioSpec rec_want, rec_have;
//static LPDIRECTSOUNDCAPTUREBUFFER lpDSBprimary2r = NULL;
//static LPDIRECTSOUNDCAPTUREBUFFER lpDSB2r = NULL;
static Uint8* recording_buffer = nullptr;
static Uint32 buffer_byte_size = 0;
static Uint32 buffer_byte_pos = 0;
static Uint32 buffer_byte_max_pos = 0;

void audioRecordingCallback(void* userdata, Uint8* stream, int len);

//Number of available devices
int recording_device_count = 0;

static int inited;
static uae_u8 *samplebuffer;
static int sampleframes;
static int recordbufferframes;
static float clockspersample;
static int vsynccnt;
static int safediff;
float sampler_evtime;

static int capture_init (void)
{
	//HRESULT hr;
	//DSCBUFFERDESC sound_buffer_rec;
	//WAVEFORMATEX wavfmt;
	TCHAR *name = nullptr;
	int samplerate = 44100;

	if (currprefs.sampler_freq)
		samplerate = currprefs.sampler_freq;

	name = record_devices[currprefs.samplersoundcard]->name;

	//wavfmt.wFormatTag = WAVE_FORMAT_PCM;
	//wavfmt.nChannels = 2;
	//wavfmt.nSamplesPerSec = samplerate;
	//wavfmt.wBitsPerSample = 16;
	//wavfmt.nBlockAlign = wavfmt.wBitsPerSample / 8 * wavfmt.nChannels;
	//wavfmt.nAvgBytesPerSec = wavfmt.nBlockAlign * wavfmt.nSamplesPerSec;
	//wavfmt.cbSize = 0;

	clockspersample = sampler_evtime / samplerate;
	sampleframes = (samplerate + 49) / 50;
	recordbufferframes = 16384;
	if (currprefs.sampler_buffer)
		recordbufferframes = currprefs.sampler_buffer;

	//hr = DirectSoundCaptureCreate (&record_devices[currprefs.win32_samplersoundcard]->guid, &lpDS2r, NULL);
	//if (FAILED (hr)) {
	//	write_log (_T("SAMPLER: DirectSoundCaptureCreate('%s') failure: %s\n"), name, DXError (hr));
	//	return 0;
	//}

	SDL_zero(rec_want);
	rec_want.freq = 44100;
	rec_want.format = AUDIO_S16SYS;
	rec_want.channels = 2;
	rec_want.samples = sampleframes;
	rec_want.callback = audioRecordingCallback;

	//Open recording device
	dev_rec = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(record_devices[currprefs.samplersoundcard]->id, SDL_TRUE), SDL_TRUE, &rec_want, &rec_have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	//Device failed to open
	if (dev_rec == 0)
	{
		//Report error
		write_log("SAMPLER: Failed to open recording device! SDL Error: %s", SDL_GetError());
		return 0;
	}

	//memset (&sound_buffer_rec, 0, sizeof (DSCBUFFERDESC));
	//sound_buffer_rec.dwSize = sizeof (DSCBUFFERDESC);
	//sound_buffer_rec.dwBufferBytes = recordbufferframes * SAMPLESIZE;
	//sound_buffer_rec.lpwfxFormat = &wavfmt;
	//sound_buffer_rec.dwFlags = 0 ;

	//hr = lpDS2r->CreateCaptureBuffer (&sound_buffer_rec, &lpDSB2r, NULL);
	//if (FAILED (hr)) {
	//	write_log (_T("SAMPLER: CreateCaptureSoundBuffer('%s') failure: %s\n"), name, DXError(hr));
	//	return 0;
	//}

	//hr = lpDSB2r->Start (DSCBSTART_LOOPING);
	//if (FAILED (hr)) {
	//	write_log (_T("SAMPLER: DirectSoundCaptureBuffer_Start('%s') failed: %s\n"), name, DXError (hr));
	//	return 0;
	//}

	buffer_byte_size = rec_have.size;
	//Allocate and initialize byte buffer
	recording_buffer = new Uint8[buffer_byte_size];
	memset(recording_buffer, 0, buffer_byte_size);

	samplebuffer = xcalloc (uae_u8, sampleframes * SAMPLESIZE);
	write_log (_T("SAMPLER: Parallel port sampler initialized, CPS=%f, '%s'\n"), clockspersample, name);
	return 1;
}

static void capture_free (void)
{
	if (dev_rec) {
		SDL_CloseAudioDevice(dev_rec);
		write_log (_T("SAMPLER: Parallel port sampler freed\n"));
	}
	if (recording_buffer)
	{
		delete[] recording_buffer;
		recording_buffer = nullptr;
	}
	xfree (samplebuffer);
	samplebuffer = nullptr;
}

static evt_t oldcycles;
static int oldoffset;

uae_u8 sampler_getsample (int channel)
{
#if 0
	int cur_pos;
	static int cap_pos;
	static float diffsample;
#endif
	static float doffset_offset;
	//HRESULT hr;
	//DWORD t;
	void *p1, *p2;
	//DWORD len1, len2;
	evt_t cycles;
	int sample, samplecnt;
	float doffset;
	int offset;

	if (!currprefs.sampler_stereo)
		channel = 0;

	if (!inited) {
		//DWORD pos;
		if (!capture_init ()) {
			capture_free ();
			return 0;
		}
		inited = 1;
		oldcycles = get_cycles ();
		oldoffset = -1;
		doffset_offset = 0;
/*		hr = lpDSB2r->GetCurrentPosition (&t, &pos);
		if (FAILED (hr)) {
			sampler_free ();
			return 0;
		}	*/	
		//if (t >= pos)
		//	safediff = t - pos;
		//else
		//	safediff = recordbufferframes * SAMPLESIZE - pos + t;
		//write_log (_T("SAMPLER: safediff %d %d\n"), safediff, safediff + sampleframes * SAMPLESIZE);
		//safediff += 4 * sampleframes * SAMPLESIZE;

#if 0
		diffsample = 0;
		safepos = -recordbufferframes / 10 * SAMPLESIZE;
		hr = lpDSB2r->GetCurrentPosition (&t, &pos);
		cap_pos = pos;
		cap_pos += safepos;
		if (cap_pos < 0)
			cap_pos += recordbufferframes * SAMPLESIZE;
		if (cap_pos >= recordbufferframes * SAMPLESIZE)
			cap_pos -= recordbufferframes * SAMPLESIZE;
		if (FAILED (hr)) {
			sampler_free ();
			return 0;
		}
#endif
	}
	if (clockspersample < 1)
		return 0;
	uae_s16 *sbuf = (uae_s16*)samplebuffer;

	vsynccnt = 0;
	sample = 0;
	samplecnt = 0;
	cycles = (int)get_cycles () - (int)oldcycles;
	doffset = doffset_offset + cycles / clockspersample;
	offset = (int)doffset;
	if (oldoffset < 0 || offset >= sampleframes || offset < 0) {
		if (offset >= sampleframes) {
			doffset -= offset;
			doffset_offset = doffset;
		}
		if (oldoffset >= 0 && offset >= sampleframes) {
			while (oldoffset < sampleframes) {
				sample += sbuf[oldoffset * SAMPLESIZE / 2 + channel];
				oldoffset++;
				samplecnt++;
			}
		}
		//hr = lpDSB2r->GetCurrentPosition (&t, NULL);
		int pos = buffer_byte_pos;
		pos -= safediff;
		if (pos < 0)
			pos += recordbufferframes * SAMPLESIZE;
		//hr = lpDSB2r->Lock (pos, sampleframes * SAMPLESIZE, &p1, &len1, &p2, &len2, 0);
		//if (FAILED (hr)) {
		//	write_log (_T("SAMPLER: Lock() failed %x\n"), hr);
		//	return 0;
		//}
		SDL_LockAudioDevice(dev_rec);
		memcpy (samplebuffer, recording_buffer, buffer_byte_size);
		//if (p2)
		//	memcpy (samplebuffer + len1, p2, len2);
		//lpDSB2r->Unlock (p1, len1, p2, len2);
		SDL_UnlockAudioDevice(dev_rec);

#if 0
		cap_pos = t;
		cap_pos += sampleframes * SAMPLESIZE;
		if (cap_pos < 0)
			cap_pos += RECORDBUFFER * SAMPLESIZE;
		if (cap_pos >= RECORDBUFFER * SAMPLESIZE)
			cap_pos -= RECORDBUFFER * SAMPLESIZE;

		hr = lpDSB2r->GetCurrentPosition (&t, &pos);
		cur_pos = pos;
		if (FAILED (hr))
			return 0;

		cur_pos += safepos;
		if (cur_pos < 0)
			cur_pos += RECORDBUFFER * SAMPLESIZE;
		if (cur_pos >= RECORDBUFFER * SAMPLESIZE)
			cur_pos -= RECORDBUFFER * SAMPLESIZE;

		int diff;
		if (cur_pos >= cap_pos)
			diff = cur_pos - cap_pos;
		else
			diff = RECORDBUFFER * SAMPLESIZE - cap_pos + cur_pos;
		if (diff > RECORDBUFFER * SAMPLESIZE / 2)
			diff -= RECORDBUFFER * SAMPLESIZE; 
		diff /= SAMPLESIZE;

		int diff2 = 100 * diff / (RECORDBUFFER / 2);
		diffsample = -pow (diff2 < 0 ? -diff2 : diff2, 3.1);
		if (diff2 < 0)
			diffsample = -diffsample;
		write_log (_T("%d\n"), diff);

		write_log (_T("CAP=%05d CUR=%05d (%-05d) OFF=%05d %f\n"),
			cap_pos / SAMPLESIZE, cur_pos / SAMPLESIZE, (cap_pos - cur_pos) / SAMPLESIZE, offset, doffset_offset);
#endif

		if (offset < 0)
			offset = 0;
		if (offset >= sampleframes)
			offset -= sampleframes;

		oldoffset = 0;
		oldcycles = get_cycles ();
	}

	while (oldoffset <= offset) {
		sample += sbuf[oldoffset * SAMPLESIZE / 2 + channel];
		samplecnt++;
		oldoffset++;
	}
	oldoffset = offset;

	if (samplecnt > 0)
		sample /= samplecnt;
#if 1
	 /* yes, not 256, without this max recording volume would still be too quiet on my sound cards */
	sample /= 128;
	if (sample < -128)
		sample = 0;
	else if (sample > 127)
		sample = 127;
	return (uae_u8)(sample - 128);
#else
	return (Uae_u8)((sample / 256) - 128);
#endif
}

int sampler_init (void)
{
	if (currprefs.samplersoundcard < 0)
		return 0;
	return 1;
}

void sampler_free (void)
{
	inited = 0;
	vsynccnt = 0;
	capture_free ();
}

void sampler_vsync (void)
{
	if (!inited)
		return;

	vsynccnt++;
	if (vsynccnt > 1) {
		oldcycles = get_cycles ();
	}
	if (vsynccnt > 50) {
		sampler_free ();
		return;
	}
}

void audioRecordingCallback(void* userdata, Uint8* stream, int len)
{
	//Copy audio from stream
	memcpy(&recording_buffer[buffer_byte_pos], stream, len);

	//Move along buffer
	buffer_byte_pos += len;
}
