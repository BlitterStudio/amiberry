/*
* Amiberry
*
* SDL3 AHI interface (V1 compatibility)
*
* Copyright 2025 Dimitris Panokostas <midwan@gmail.com>
* Based on WinUAE code by Toni Wilen
*/

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
#include "uni_common.h"

static long intcount;
static int record_enabled;
int ahi_on;

static uae_u8 *ahisndbuffer; // one guest block, see ahi_write_block()
static int ahitweak;
int ahi_pollrate = 40;

int sound_freq_ahi, sound_channels_ahi, sound_bits_ahi;

static int amigablksize;
// Bytes of audio to keep queued in ahi_stream; set in ahi_init_sound().
static int ahi_queue_target;
// Last mute state passed to setvolume_ahi(), applied again when the stream opens.
static int ahi_master_mute;

static SDL_AudioStream* ahi_stream = nullptr;
static SDL_AudioStream* ahi_rec_stream = nullptr;

static int ahi_record_read_count;
static bool ahi_record_nonzero_seen;

namespace {
struct winuae	//this struct is put in a6 if you call
	//execute native function
{
	uae_u32 amigawnd;    //address of amiga Window Windows Handle (HWND)
	unsigned int changenum;   //number to detect screen close/open
	unsigned int z3offset;    //the offset to add to access Z3 mem from Dll side
};
}
static struct winuae uaevar;

void ahi_close_sound()
{
	if (!ahi_on)
		return;
	ahi_on = 0;
	record_enabled = 0;
	ahi_queue_target = 0;

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
	ahi_record_read_count = 0;
	ahi_record_nonzero_seen = false;
	if (ahisndbuffer)
	{
		xfree(ahisndbuffer);
		ahisndbuffer = nullptr;
	}
}

// The uae.audio driver mixes one block ahead. Its EXTER interrupt server asks
// for a pending request (op 4); if there is one it writes the block it has
// already mixed (op 2) and Causes a soft interrupt that mixes the next one.
// Request a block only while the host queue is below its target, so the guest
// mixes at the rate the audio device plays: no bursts of back-to-back mixing,
// and no more latency than the target.
static bool ahi_queue_wants_block()
{
	const int queued = SDL_GetAudioStreamQueued(ahi_stream);
	const int target = std::max(ahi_queue_target - ahitweak, amigablksize * 4);
	return queued >= 0 && queued < target;
}

static void ahi_poll()
{
	if (ahi_queue_wants_block()) {
		intcount = 1;
		INTREQ(0x8000 | 0x2000);
	} else {
		// EXTER is shared with CIA-B and Zorro cards. Withdraw a request that
		// has not been taken yet, so one of their interrupts cannot make the
		// driver write a block the queue has no room for.
		intcount = 0;
	}
}

static void ahi_write_block(const uaecptr addr)
{
	const int bytes = amigablksize * 4;
	const bool swap = currprefs.sound_stereo_swap_ahi;
	auto* dst = reinterpret_cast<uae_s16*>(ahisndbuffer);
	for (int i = 0; i < amigablksize; i++) {
		// Each long is a big-endian left/right pair, left in the high word.
		const uae_u32 v = get_long(addr + i * 4);
		const auto left = static_cast<uae_s16>(v >> 16);
		const auto right = static_cast<uae_s16>(v);
		dst[i * 2 + 0] = swap ? right : left;
		dst[i * 2 + 1] = swap ? left : right;
	}

	// Always queue what the guest wrote: dropping a block is an audible gap.
	SDL_PutAudioStreamData(ahi_stream, ahisndbuffer, bytes);
	ahi_poll();
}

static int ahi_init_record ()
{
	SDL_AudioSpec spec;
	spec.format = SDL_AUDIO_S16;
	spec.channels = sound_channels_ahi;
	spec.freq = sound_freq_ahi;

	enumerate_sound_devices();

	const int recordcard = currprefs.samplersoundcard;
	const bool use_default_device = recordcard < 0 || recordcard >= MAX_SOUND_DEVICES
		|| record_devices[recordcard] == nullptr;
	const SDL_AudioDeviceID device_id = use_default_device
		? SDL_AUDIO_DEVICE_DEFAULT_RECORDING
		: static_cast<SDL_AudioDeviceID>(record_devices[recordcard]->id);

	ahi_rec_stream = SDL_OpenAudioDeviceStream(device_id, &spec, nullptr, nullptr);

	if (!ahi_rec_stream && !use_default_device) {
		write_log(_T("AHI: Failed to open selected recording device, retrying with default: %s\n"), SDL_GetError());
		ahi_rec_stream = SDL_OpenAudioDeviceStream(
			SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, nullptr, nullptr);
	}
	if (!ahi_rec_stream) {
		write_log(_T("AHI: SDL_OpenAudioDeviceStream() recording failure: %s\n"), SDL_GetError());
		record_enabled = -1;
		return 0;
	}

	SDL_ResumeAudioStreamDevice(ahi_rec_stream); // Start recording
	record_enabled = 1;
	ahi_record_read_count = 0;
	ahi_record_nonzero_seen = false;
	write_log(_T("AHI: Init AHI Audio Recording Rate %d, Channels %d, Device %s\n"),
		sound_freq_ahi, sound_channels_ahi,
		use_default_device ? _T("default") : record_devices[recordcard]->name);
	return 1;
}

void ahi_apply_volume()
{
	if (!ahi_stream)
		return;
	float gain = 0.0f;
	if (!ahi_master_mute) {
		// Read the volumes from the prefs: without Paula sound, set_volume()
		// never runs, but the configured attenuation still applies to AHI.
		const int board = std::clamp(currprefs.sound_volume_board, 0, 100);
		const int master = std::clamp(currprefs.sound_volume_master, 0, 100);
		gain = static_cast<float>((100 - board) * (100 - master)) / 10000.0f;
	}
	if (!SDL_SetAudioStreamGain(ahi_stream, gain))
		write_log(_T("AHI: SDL_SetAudioStreamGain failed: %s\n"), SDL_GetError());
}

void setvolume_ahi(int mute)
{
	ahi_master_mute = mute;
	ahi_apply_volume();
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

	// Keep as much queued as the Paula push path at its high-water mark, three
	// buffers of the Sound buffer setting, but at least two emulated frames:
	// the emulator runs each frame in a burst, and nothing is mixed between
	// bursts. Two guest blocks is the floor, so the guest can mix the next
	// block while the queue still holds one.
	const int frame_bytes = sound_channels_ahi * sound_bits_ahi / 8;
	const int paula_frames = std::max((currprefs.sound_maxbsiz >> 2) & ~63, 0x80);
	const auto buffer_frames = static_cast<int64_t>(paula_frames) * 3 * sound_freq_ahi
		/ std::max(currprefs.sound_freq, 1);
	const auto vsync_frames = static_cast<int64_t>(2.0f * sound_freq_ahi / std::max(vblank_hz, 1.0f));
	ahi_queue_target = static_cast<int>(std::max({ buffer_frames * frame_bytes,
		vsync_frames * frame_bytes, static_cast<int64_t>(amigablksize) * 4 * 2 }));

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
	ahisndbuffer = xmalloc(uae_u8, amigablksize * 4);
	if (!ahisndbuffer) {
		SDL_DestroyAudioStream(ahi_stream);
		ahi_stream = nullptr;
		return 0;
	}
	write_log(_T("AHI: queue target %d bytes\n"), ahi_queue_target);

	SDL_ResumeAudioStreamDevice(ahi_stream);

	ahi_apply_volume();

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
		if (ahi_on)
			ahi_write_block(m68k_areg (regs, 0));
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
			bool has_signal = false;
			for (int i = 0; i < bytes_read / 2; i++) {
				if (sndbufrecpt[i] != 0) {
					has_signal = true;
					break;
				}
			}
			ahi_record_read_count++;
			if (has_signal && !ahi_record_nonzero_seen) {
				ahi_record_nonzero_seen = true;
				write_log(_T("AHI: Recording received first non-silent buffer after %d reads\n"), ahi_record_read_count);
			} else if (!has_signal && ahi_record_read_count <= 5) {
				write_log(_T("AHI: Recording read %d bytes of silence from host input\n"), bytes_read);
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
		ahi_poll();
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

#ifdef WITH_UAENATIVE
	case 100: // open dll
		return uaenative_open_library(context, UNI_FLAG_COMPAT);

	case 101: //get dll label
		return uaenative_get_function(context, UNI_FLAG_COMPAT);

	case 102: //execute native code
		return uaenative_call_function(context, UNI_FLAG_COMPAT);

	case 103: //close dll
		return uaenative_close_library(context, UNI_FLAG_COMPAT);
#else
	case 100: // native libraries are not part of this build
	case 101:
	case 102:
	case 103:
		return UNI_ERROR_NOT_ENABLED;
#endif

	case 104: //screenlost
		{
			static int oldnum = 0;
			if (uaevar.changenum == oldnum)
				return 0;
			oldnum = uaevar.changenum;
			return 1;
		}


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
			ahi_poll();
			count = 0;
		}
	}
#endif
}

#endif
