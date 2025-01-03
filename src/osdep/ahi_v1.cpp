/*
* UAE - The Un*x Amiga Emulator
*
* SDL2 interface
*
* Copyright 1997 Mathias Ortmann
* Copyright 1997-2001 Brian King
* Copyright 2000-2002 Bernd Roesch
* Copyright 2024 Dimitris Panokostas
*/

#define NATIVBUFFNUM 4
#define RECORDBUFFER 50 //survive 9 sec of blocking at 44100

#include "sysconfig.h"
#include "sysdeps.h"

#if defined(AHI)

#include <cctype>
#include <cassert>

#include <cstdlib>
#include <cstdarg>
#include <cstdio>

#include "options.h"
#include "audio.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "autoconf.h"
#include "newcpu.h"
#include "traps.h"
#include "sounddep/sound.h"
#include "parser.h"
#ifdef ENFORCER
#include "enforcer.h"
#endif
#include "ahi_v1.h"
#include "picasso96.h"
#include "uaenative.h"

static long samples, playchannel, intcount;
static int record_enabled;
int ahi_on;
static uae_u8 *sndptrmax;
static uae_u8 soundneutral;

//static LPSTR lpData,sndptrout;
extern uae_u32 chipmem_mask;
static uae_u8 *ahisndbuffer, *sndrecbuffer;
static int ahisndbufsize, *ahisndbufpt, ahitweak;;
int ahi_pollrate = 40;

int sound_freq_ahi, sound_channels_ahi, sound_bits_ahi;

static int vin, devicenum;
static int amigablksize;

static uae_u32 sound_flushes2 = 0;

SDL_AudioDeviceID ahi_dev;
SDL_AudioDeviceID ahi_dev_rec;
SDL_AudioSpec ahi_want, ahi_have;

Uint32 ahi_buffer_byte_pos = 0;

struct winuae	//this struct is put in a6 if you call
	//execute native function
{
	HWND amigawnd;    //address of amiga Window Windows Handle
	unsigned int changenum;   //number to detect screen close/open
	unsigned int z3offset;    //the offset to add to access Z3 mem from Dll side
};
static struct winuae uaevar;

void ahi_close_sound()
{
	if (!ahi_on)
		return;
	ahi_on = 0;
	record_enabled = 0;
	ahisndbufpt = reinterpret_cast<int*>(ahisndbuffer);

	if (ahi_dev) {
		SDL_PauseAudioDevice(ahi_dev, 1);
		SDL_CloseAudioDevice(ahi_dev);
		ahi_dev = 0;
	} else {
		write_log(_T("AHI: Sound Stopped...\n"));
	}
	if (ahi_dev_rec)
		SDL_CloseAudioDevice(ahi_dev_rec);
	ahi_dev_rec = 0;
	if (ahisndbuffer)
	{
		xfree(ahisndbuffer);
		ahisndbuffer = nullptr;
	}
}

typedef unsigned long DWORD;

void ahi_updatesound(int force)
{
	Uint32 pos;
	Uint32 dwBytes1;
	Uint8 *dwData1;
	static int oldpos;

	if (sound_flushes2 == 1) {
		oldpos = 0;
		intcount = 1;
		INTREQ(0x8000 | 0x2000);

		// Start playing audio
		SDL_PauseAudioDevice(ahi_dev, 0);
	}

	SDL_LockAudioDevice(ahi_dev);
	pos = SDL_GetQueuedAudioSize(ahi_dev);
	SDL_UnlockAudioDevice(ahi_dev);
	// safety check to limit the number of bytes being queued
	if (pos > 65536) return;

	pos -= ahitweak;
	if (pos < 0)
		pos += ahisndbufsize;
	if (pos >= ahisndbufsize)
		pos -= ahisndbufsize;
	pos = (pos / (amigablksize * 4)) * (amigablksize * 4);
	if (force == 1) {
		if (oldpos != pos) {
			intcount = 1;
			INTREQ(0x8000 | 0x2000);
			return; //to generate amiga ints every amigablksize
		}
		else {
			return;
		}
	}

	dwBytes1 = amigablksize * 4;
	dwData1 = ahisndbuffer + oldpos;

	if (currprefs.sound_stereo_swap_ahi) {
		int i;
		auto p = reinterpret_cast<uae_s16*>(ahisndbuffer);
		for (i = 0; i < dwBytes1 / 2; i += 2) {
			uae_s16 tmp;
			tmp = p[i + 0];
			p[i + 0] = p[i + 1];
			p[i + 1] = tmp;
		}
	}

	// Copy the audio data to the buffer
	memcpy(dwData1, ahisndbuffer, dwBytes1);
	
	sndptrmax = ahisndbuffer + ahisndbufsize;
	ahisndbufpt = reinterpret_cast<int*>(ahisndbuffer);

	SDL_QueueAudio(ahi_dev, dwData1, dwBytes1);

	oldpos += amigablksize * 4;
	if (oldpos >= ahisndbufsize)
		oldpos -= ahisndbufsize;
	if (oldpos != pos) {
		intcount = 1;
		INTREQ(0x8000 | 0x2000);
	}
}


void ahi_finish_sound_buffer ()
{
	sound_flushes2++;
	ahi_updatesound(2);
}

static int ahi_init_record ()
{
#ifdef _WIN32
	HRESULT hr;
	DSCBUFFERDESC sound_buffer_rec;
	// Record begin
	hr = DirectSoundCaptureCreate(NULL, &lpDS2r, NULL);
	if (FAILED(hr)) {
		write_log(_T("AHI: DirectSoundCaptureCreate() failure: %s\n"), DXError(hr));
		record_enabled = -1;
		return 0;
	}
	memset(&sound_buffer_rec, 0, sizeof(DSCBUFFERDESC));
	sound_buffer_rec.dwSize = sizeof(DSCBUFFERDESC);
	sound_buffer_rec.dwBufferBytes = amigablksize * 4 * RECORDBUFFER;
	sound_buffer_rec.lpwfxFormat = &wavfmt;
	sound_buffer_rec.dwFlags = 0;

	hr = IDirectSoundCapture_CreateCaptureBuffer(lpDS2r, &sound_buffer_rec, &lpDSB2r, NULL);
	if (FAILED(hr)) {
		write_log(_T("AHI: CreateCaptureSoundBuffer() failure: %s\n"), DXError(hr));
		record_enabled = -1;
		return 0;
	}

	hr = IDirectSoundCaptureBuffer_Start(lpDSB2r, DSCBSTART_LOOPING);
	if (FAILED(hr)) {
		write_log(_T("AHI: DirectSoundCaptureBuffer_Start failed: %s\n"), DXError(hr));
		record_enabled = -1;
		return 0;
	}
	record_enabled = 1;
	write_log(_T("AHI: Init AHI Audio Recording \n"));
	return 1;
#else
	SDL_AudioSpec desired, obtained;
	desired.freq = sound_freq_ahi;
	desired.format = AUDIO_S16SYS;
	desired.channels = sound_channels_ahi;
	desired.samples = amigablksize * 4 * RECORDBUFFER;
	desired.callback = nullptr; // We'll use SDL_QueueAudio and SDL_DequeueAudio instead of a callback

	ahi_dev_rec = SDL_OpenAudioDevice(nullptr, 1, &desired, &obtained, 0);
	if (ahi_dev_rec == 0) {
		write_log(_T("AHI: SDL_OpenAudioDevice() failure: %s\n"), SDL_GetError());
		record_enabled = -1;
		return 0;
	}

	SDL_PauseAudioDevice(ahi_dev_rec, 0); // Start recording
	record_enabled = 1;
	write_log(_T("AHI: Init AHI Audio Recording \n"));
	return 1;
#endif
}

void setvolume_ahi (int vol)
{
	if (!ahi_dev)
		return;
#ifdef WIN32
	HRESULT hr;
	float adjvol = (100.0f - currprefs.sound_volume_board) * (100.0f - volume) / 100.0f;
	LONG vol = DSBVOLUME_MIN;
	if (adjvol > 0) {
		vol = (LONG)((DSBVOLUME_MIN / 2) + (-DSBVOLUME_MIN / 2) * log(1 + (2.718281828 - 1) * (adjvol / 100.0)));
	}
	hr = IDirectSoundBuffer_SetVolume(lpDSB2, vol);
	if (FAILED(hr))
		write_log(_T("AHI: SetVolume(%d) failed: %s\n"), vol, DXError (hr));
#else
	// SDL2 doesn't support volume control on AHI
#endif
}

static int ahi_init_sound()
{
	if (ahi_dev)
		return 0;

	enumerate_sound_devices();
	SDL_zero(ahi_want);
	ahi_want.freq = sound_freq_ahi;
	ahi_want.format = sound_bits_ahi == 16 ? AUDIO_S16SYS : AUDIO_U8;
	ahi_want.channels = sound_channels_ahi;
	ahi_want.samples = amigablksize;

	write_log(_T("AHI: Init AHI Sound Rate %d, Channels %d, Bits %d, Buffsize %d\n"),
	sound_freq_ahi, sound_channels_ahi, sound_bits_ahi, amigablksize);

	if (!amigablksize)
		return 0;
	soundneutral = 0;
	ahisndbufsize = (amigablksize * 4) * NATIVBUFFNUM;  // use 4 native buffer
	ahisndbuffer = xmalloc(uae_u8, ahisndbufsize + 32);
	if (!ahisndbuffer)
		return 0;

	const auto devname = sound_devices[currprefs.soundcard]->name;
	ahi_dev = SDL_OpenAudioDevice(currprefs.soundcard_default ? nullptr : devname, 0, &ahi_want, &ahi_have, 0);

	if (ahi_dev == 0) {
		write_log(_T("AHI: Failed to open selected SDL2 device: %s, retrying with default device\n"), SDL_GetError());
		ahi_dev = SDL_OpenAudioDevice(nullptr, 0, &ahi_want, &ahi_have, 0);
		if (ahi_dev == 0)
		{
			write_log(_T("AHI: Failed to open default SDL2 device: %s:\n"), SDL_GetError());
			return 0;
		}
	}

	SDL_PauseAudioDevice(ahi_dev, 0);

	setvolume_ahi(0);

	ahisndbufpt = reinterpret_cast<int*>(ahisndbuffer);
	sndptrmax = ahisndbuffer + ahisndbufsize;
	memset(ahisndbuffer, soundneutral, static_cast<size_t>(amigablksize) * 8);
	ahi_on = 1;
	return sound_freq_ahi;
}

int ahi_open_sound ()
{
	int rate;

	uaevar.changenum++;
	if (!sound_freq_ahi)
		return 0;
	if (ahi_on)
		ahi_close_sound ();
	sound_flushes2 = 1;
	if ((rate = ahi_init_sound ()))
		return rate;
	return 0;
}

#ifndef AMIBERRY
static void *bswap_buffer = NULL;
static uae_u32 bswap_buffer_size = 0;
static float syncdivisor;
#endif

uae_u32 REGPARAM2 ahi_demux (TrapContext *context)
{
	//use the extern int (6 #13)
	// d0 0=opensound      d1=unit d2=samplerate d3=blksize ret: sound frequency
	// d0 6=opensound_new  d1=unit d2=samplerate d3=blksize ret d4=channels d5=bits d6=zero: sound frequency
	// d0 1=closesound     d1=unit
	// d0 2=writesamples   d1=unit a0=addr     write blksize samples to card
	// d0 3=readsamples    d1=unit a0=addr     read samples from card ret: d0=samples read
	// make sure you have from amigaside blksize*4 mem alloced
	// d0=-1 no data available d0=-2 no recording open
	// d0 > 0 there are more blksize Data in the que
	// do the loop until d0 get 0
	// if d0 is greater than 200 bring a message
	// that show the user that data is lost
	// maximum blocksbuffered are 250 (8,5 sec)
	// d0 4=writeinterrupt d1=unit d0=0 no interrupt happen for this unit
	// d0=-2 no playing open
	//note units for now not support use only unit 0
	// d0 5=?
	// d0=10 get clipboard size  d0=size in bytes
	// d0=11 get clipboard data  a0=clipboarddata
	//Note: a get clipboard size must do before
	// d0=12 write clipboard data	 a0=clipboarddata
	// d0=13 setp96mouserate	 d1=hz value
	// d0=100 open dll		 d1=dll name in windows name conventions
	// d0=101 get dll function addr	 d1=dllhandle a0 function/var name
	// d0=102 exec dllcode		 a0=addr of function (see 101)
	// d0=103 close dll
	// d0=104 screenlost
	// d0=105 mem offset
	// d0=106 16Bit byteswap
	// d0=107 32Bit byteswap
	// d0=108 free swap array
	// d0=200 ahitweak		 d1=offset for dsound position pointer

	int opcode = m68k_dreg (regs, 0);

	switch (opcode)
	{
		static int cap_pos, clipsize;
		static TCHAR *clipdat;

	case 0:
		cap_pos = 0;
		sound_bits_ahi = 16;
		sound_channels_ahi = 2;
		sound_freq_ahi = m68k_dreg (regs, 2);
		amigablksize = m68k_dreg (regs, 3);
		sound_freq_ahi = ahi_open_sound();
		uaevar.changenum--;
		return sound_freq_ahi;
	case 6: /* new open function */
		cap_pos = 0;
		sound_freq_ahi = m68k_dreg (regs, 2);
		amigablksize = m68k_dreg (regs, 3);
		sound_channels_ahi = m68k_dreg (regs, 4);
		sound_bits_ahi = m68k_dreg (regs, 5);
		sound_freq_ahi = ahi_open_sound();
		uaevar.changenum--;
		return sound_freq_ahi;

	case 1:
		ahi_close_sound();
		sound_freq_ahi = 0;
		return 0;

	case 2:
		{
			uaecptr addr = m68k_areg (regs, 0);
			for (int i = 0; i < amigablksize * 4; i += 4)
				*ahisndbufpt++ = static_cast<int>(get_long(addr + i));
			ahi_finish_sound_buffer();
		}
		return amigablksize;

	case 3:
		{
			//Recording
#ifdef _WIN32
		LPVOID pos1, pos2;
		DWORD t, cur_pos;
		uaecptr addr;
		HRESULT hr;
		int i, todo;
		DWORD byte1, byte2;

		if (!ahi_on)
			return -2;
		if (record_enabled == 0)
			ahi_init_record();
		if (record_enabled < 0)
			return -2;
		hr = lpDSB2r->GetCurrentPosition(&t, &cur_pos);
		if (FAILED(hr))
			return -1;

		t = amigablksize * 4;
		if (cap_pos <= cur_pos)
			todo = cur_pos - cap_pos;
		else
			todo = cur_pos + (RECORDBUFFER * t) - cap_pos;
		if (todo < t) //if no complete buffer ready exit
			return -1;
		hr = lpDSB2r->Lock(cap_pos, t, &pos1, &byte1, &pos2, &byte2, 0);
		if (FAILED(hr))
			return -1;
		if ((cap_pos + t) < (t * RECORDBUFFER))
			cap_pos = cap_pos + t;
		else
			cap_pos = 0;
		addr = m68k_areg(regs, 0);
		uae_u16* sndbufrecpt = (uae_u16*)pos1;
		t /= 4;
		for (i = 0; i < t; i++) {
			uae_u32 s1, s2;
			if (currprefs.sound_stereo_swap_ahi) {
				s1 = sndbufrecpt[1];
				s2 = sndbufrecpt[0];
			}
			else {
				s1 = sndbufrecpt[0];
				s2 = sndbufrecpt[1];
			}
			sndbufrecpt += 2;
			put_long(addr, (s1 << 16) | s2);
			addr += 4;
		}
		t *= 4;
		lpDSB2r->Unlock(pos1, byte1, pos2, byte2);
		return (todo - t) / t;
#else
			Uint32 byte1, byte2;

		if (!ahi_on)
			return -2;
		if (record_enabled == 0)
			ahi_init_record();
		if (record_enabled < 0)
			return -2;

		auto todo = SDL_GetQueuedAudioSize(ahi_dev_rec);
		if (todo < amigablksize * 4) //if no complete buffer ready exit
			return -1;

		uaecptr addr = m68k_areg(regs, 0);
		auto* sndbufrecpt = static_cast<uae_u16*>(malloc(todo));
		SDL_DequeueAudio(ahi_dev_rec, sndbufrecpt, todo);

		todo /= 4;
		for (int i = 0; i < todo; i++) {
			uae_u32 s1, s2;
			if (currprefs.sound_stereo_swap_ahi) {
				s1 = sndbufrecpt[1];
				s2 = sndbufrecpt[0];
			}
			else {
				s1 = sndbufrecpt[0];
				s2 = sndbufrecpt[1];
			}
			sndbufrecpt += 2;
			put_long(addr, (s1 << 16) | s2);
			addr += 4;
		}
		todo *= 4;
		free(sndbufrecpt);
		return (SDL_GetQueuedAudioSize(ahi_dev_rec) - todo) / todo;
#endif
		}

	case 4:
		{
			if (!ahi_on)
				return -2;
			int i = intcount;
			intcount = 0;
			return i;
		}

	case 5:
		if (!ahi_on)
			return 0;
		ahi_updatesound ( 1 );
		return 1;

	case 10:
		clipdat = SDL_GetClipboardText();
		if (clipdat) {
			clipsize = _tcslen(clipdat);
			clipsize++;
			return clipsize;
		}

		return 0;

	case 11:
		{
			put_byte (m68k_areg (regs, 0), 0);
			if (clipdat) {
				char *tmp = ua (clipdat);
				int i;
				for (i = 0; i < clipsize && i < strlen (tmp); i++)
					put_byte (m68k_areg (regs, 0) + i, tmp[i]);
				put_byte (m68k_areg (regs, 0) + clipsize - 1, 0);
				xfree (tmp);
			}
		}
		return 0;

	case 12:
		{
			TCHAR* s = au(reinterpret_cast<char*>(get_real_address(m68k_areg(regs, 0))));
			if (SDL_SetClipboardText(s) != 0) {
				// handle error
				write_log("Unable to set clipboard text: %s\n", SDL_GetError());
			}
			xfree(s);
		}
		return 0;

	case 13: /* HACK */
		{ //for higher P96 mouse draw rate
			set_picasso_hack_rate (m68k_dreg (regs, 1) * 2);
		} //end for higher P96 mouse draw rate
		return 0;

	case 20:
#ifdef ENFORCER
		return enforcer_enable(m68k_dreg(regs, 1));
#else
		return 1;
#endif

	case 21:
#ifdef ENFORCER
		return enforcer_disable();
#else
		return 1;
#endif

	case 25:
#if defined (PARALLEL_PORT)
		flushprinter ();
#endif
		return 0;

	case 100: // open dll
		return uaenative_open_library(context, UNI_FLAG_COMPAT);

	case 101: //get dll label
		return uaenative_get_function(context, UNI_FLAG_COMPAT);

	case 102: //execute native code
		return uaenative_call_function(context, UNI_FLAG_COMPAT);

	case 103: //close dll
		return uaenative_close_library(context, UNI_FLAG_COMPAT);

	case 104: //screenlost
		{
			static int oldnum = 0;
			if (uaevar.changenum == oldnum)
				return 0;
			oldnum = uaevar.changenum;
			return 1;
		}

#ifndef CPU_64_BIT
	case 105: //returns memory offset
		return (uae_u32)get_real_address(0);
#endif

	case 110:
	{
		put_long(m68k_areg(regs, 0), 0);
		put_long(m68k_areg(regs, 0) + 4, 1e+9);
	}
	return 1;

	case 111:
	{
		struct timespec ts {};
		clock_gettime(CLOCK_MONOTONIC, &ts);
		const auto time = static_cast<int64_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
		put_long(m68k_areg(regs, 0), static_cast<uae_u32>(time & 0xffffffff));
		put_long(m68k_areg(regs, 0) + 4, static_cast<uae_u32>(time >> 32));
			}
	return 1;

	case 200:
		ahitweak = m68k_dreg (regs, 1);
		ahi_pollrate = m68k_dreg (regs, 2);
		if (ahi_pollrate < 10)
			ahi_pollrate = 10;
		if (ahi_pollrate > 60)
			ahi_pollrate = 60;
		return 1;

	default:
		return 0x12345678;     // Code for not supported function
	}
}

#endif

void init_ahi()
{
#ifdef AHI
	if (uae_boot_rom_type) {
		uaecptr a = here(); //this installs the ahisound
		org(rtarea_base + 0xFFC0);
		calltrap(deftrapres(ahi_demux, 0, _T("ahi_winuae")));
		dw(RTS);
		org(a);
#ifdef AHI_V2
		init_ahi_v2();
#endif
	}
#endif
}

void ahi_hsync()
{
#ifdef AHI
	static int count;
	if (ahi_on) {
		count++;
		//15625/count freebuffer check
		if (count > ahi_pollrate) {
			ahi_updatesound(1);
			count = 0;
		}
	}
#endif
}
