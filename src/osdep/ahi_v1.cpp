/*
* Amiberry
*
* SDL3 AHI interface (V1 compatibility)
*
* Copyright 2025 Dimitris Panokostas <midwan@gmail.com>
* Based on WinUAE code by Toni Wilen
*/

#define NATIVBUFFNUM 4
#define RECORDBUFFER 50 //survive 9 sec of blocking at 44100

#include "sysconfig.h"
#include "sysdeps.h"

#if defined(AHI)

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

#include "options.h"
#include "audio.h"
#include "memory.h"
#include "events.h"
#include "custom.h"
#include "newcpu.h"
#include "traps.h"
#include "autoconf.h"
#include "sounddep/sound.h"
#include "parser.h"
#ifdef ENFORCER
#include "enforcer.h"
#endif
#include "ahi_v1.h"
#include "picasso96.h"
#include "uaenative.h"

static long intcount;
static int record_enabled;
int ahi_on;
static uae_u8 soundneutral;

extern uae_u32 chipmem_mask;
static uae_u8 *ahisndbuffer;
static int ahisndbufsize, ahitweak;
int ahi_pollrate = 40;

int sound_freq_ahi, sound_channels_ahi, sound_bits_ahi;

static int amigablksize;

static uae_u32 sound_flushes2 = 0;

static SDL_AudioStream* ahi_stream = nullptr;
static SDL_AudioStream* ahi_rec_stream = nullptr;

static int ahi_write_pos;

struct winuae	//this struct is put in a6 if you call
	//execute native function
{
	uae_u32 amigawnd;    //address of amiga Window Windows Handle (HWND)
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
	ahi_write_pos = 0;

	if (ahi_stream) {
		SDL_PauseAudioStreamDevice(ahi_stream);
		SDL_DestroyAudioStream(ahi_stream);
		ahi_stream = nullptr;
	} else {
		write_log(_T("AHI: Sound Stopped...\n"));
	}
	if (ahi_rec_stream) {
		SDL_PauseAudioStreamDevice(ahi_rec_stream);
		SDL_DestroyAudioStream(ahi_rec_stream);
		ahi_rec_stream = nullptr;
	}
	if (ahisndbuffer)
	{
		xfree(ahisndbuffer);
		ahisndbuffer = nullptr;
	}
}

void ahi_updatesound(int force)
{
	int pos;
	uint32_t dwBytes1;
	uint8_t *dwData1;
	static int oldpos;

	if (sound_flushes2 == 1) {
		oldpos = 0;
		ahi_write_pos = 0;
		intcount = 1;
		INTREQ(0x8000 | 0x2000);

		// Start playing audio
		SDL_ResumeAudioStreamDevice(ahi_stream);
	}

	SDL_LockAudioStream(ahi_stream);
	pos = SDL_GetAudioStreamQueued(ahi_stream);
	SDL_UnlockAudioStream(ahi_stream);
	// safety check — error or too much data queued
	if (pos < 0 || pos > 65536) return;

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

	if (oldpos == ahi_write_pos)
		return; // buffer is empty

	dwBytes1 = amigablksize * 4;
	dwData1 = ahisndbuffer + oldpos;

	if (currprefs.sound_stereo_swap_ahi) {
		int i;
		auto p = reinterpret_cast<uae_s16*>(dwData1);
		for (i = 0; i < dwBytes1 / 2; i += 2) {
			uae_s16 tmp;
			tmp = p[i + 0];
			p[i + 0] = p[i + 1];
			p[i + 1] = tmp;
		}
	}

	SDL_PutAudioStreamData(ahi_stream, dwData1, dwBytes1);

	oldpos += amigablksize * 4;
	if (oldpos >= ahisndbufsize)
		oldpos = 0;

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
	SDL_AudioSpec spec;
	spec.format = SDL_AUDIO_S16;
	spec.channels = sound_channels_ahi;
	spec.freq = sound_freq_ahi;

	ahi_rec_stream = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, nullptr, nullptr);
	if (!ahi_rec_stream) {
		write_log(_T("AHI: SDL_OpenAudioDeviceStream() recording failure: %s\n"), SDL_GetError());
		record_enabled = -1;
		return 0;
	}

	SDL_ResumeAudioStreamDevice(ahi_rec_stream); // Start recording
	record_enabled = 1;
	write_log(_T("AHI: Init AHI Audio Recording \n"));
	return 1;
}

void setvolume_ahi (int volume)
{
	if (!ahi_stream)
		return;
	// SDL3 AudioStream doesn't support device-level volume control directly.
	// AHI V1 pushes raw buffers from Amiga — volume scaling would need software mixing.
	// For now, no-op.
}

static int ahi_init_sound()
{
	if (ahi_stream)
		return 0;

	enumerate_sound_devices();

	SDL_AudioSpec spec;
	spec.format = sound_bits_ahi == 16 ? SDL_AUDIO_S16 : SDL_AUDIO_U8;
	spec.channels = sound_channels_ahi;
	spec.freq = sound_freq_ahi;

	write_log(_T("AHI: Init AHI Sound Rate %d, Channels %d, Bits %d, Buffsize %d\n"),
	sound_freq_ahi, sound_channels_ahi, sound_bits_ahi, amigablksize);

	if (!amigablksize)
		return 0;
	soundneutral = 0;
	// Use 4 native buffers as per WinUAE
	ahisndbufsize = (amigablksize * 4) * NATIVBUFFNUM;
	ahisndbuffer = xmalloc(uae_u8, ahisndbufsize + 32);
	if (!ahisndbuffer)
		return 0;

	// Honor selected output device, matching the Paula audio path in sound.cpp
	const int sndcard = currprefs.soundcard;
	const bool use_default_device = currprefs.soundcard_default
		|| sndcard < 0 || sndcard >= MAX_SOUND_DEVICES
		|| sound_devices[sndcard] == nullptr;
	const SDL_AudioDeviceID device_id = use_default_device
		? SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK
		: static_cast<SDL_AudioDeviceID>(sound_devices[sndcard]->id);

	// Create a separate SDL3 AudioStream for AHI output (push mode, no callback)
	ahi_stream = SDL_OpenAudioDeviceStream(device_id, &spec, nullptr, nullptr);

	if (!ahi_stream && !use_default_device) {
		write_log(_T("AHI: Failed to open selected audio device, retrying with default: %s\n"), SDL_GetError());
		ahi_stream = SDL_OpenAudioDeviceStream(
			SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	}
	if (!ahi_stream) {
		write_log(_T("AHI: Failed to open SDL3 audio device stream: %s\n"), SDL_GetError());
		return 0;
	}

	SDL_ResumeAudioStreamDevice(ahi_stream);

	setvolume_ahi(0);

	ahi_write_pos = 0;
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
		static const char *clipdat;

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
			auto* ahisndbufpt = reinterpret_cast<int*>(ahisndbuffer + ahi_write_pos);
			for (int i = 0; i < amigablksize * 4; i += 4)
				*ahisndbufpt++ = static_cast<int>(get_long(addr + i));

			ahi_write_pos += amigablksize * 4;
			if (ahi_write_pos >= ahisndbufsize)
				ahi_write_pos = 0;

			ahi_finish_sound_buffer();
		}
		return amigablksize;

	case 3:
		{
			//Recording
			if (!ahi_on)
				return -2;
			if (record_enabled == 0)
				ahi_init_record();
			if (record_enabled < 0)
				return -2;

			const auto queued_bytes = SDL_GetAudioStreamAvailable(ahi_rec_stream);
			if (queued_bytes < static_cast<uint32_t>(amigablksize * 4)) //if no complete buffer ready exit
				return -1;

			uaecptr addr = m68k_areg(regs, 0);
			const int bytes_to_read = amigablksize * 4;
			auto* sndbufrecpt = static_cast<uae_u16*>(malloc(bytes_to_read));
			if (!sndbufrecpt) {
				write_log(_T("AHI: malloc failed in recording path\n"));
				return -1;
			}
			const int bytes_read = SDL_GetAudioStreamData(ahi_rec_stream, sndbufrecpt, bytes_to_read);
			if (bytes_read < 0) {
				free(sndbufrecpt);
				return -1;
			}

			auto* sptr = sndbufrecpt;
			for (int i = 0; i < amigablksize; i++) {
				uae_u32 s1, s2;
				if (currprefs.sound_stereo_swap_ahi) {
					s1 = sptr[1];
					s2 = sptr[0];
				}
				else {
					s1 = sptr[0];
					s2 = sptr[1];
				}
				sptr += 2;
				put_long(addr, (s1 << 16) | s2);
				addr += 4;
			}
			free(sndbufrecpt);
			return (SDL_GetAudioStreamAvailable(ahi_rec_stream)) / (amigablksize * 4);
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
			if (!SDL_SetClipboardText(s)) {
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

#endif
