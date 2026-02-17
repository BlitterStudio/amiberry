/*
 * FS-UAE - The Un*x Amiga Emulator
 *
 * Serial-to-MidiPort interface
 *
 * Written by Christian Vogelgsang <chris@vogelgsang.org>
 * Windows native MIDI support added for Amiberry
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "thread.h"

#ifdef WITH_MIDI

#include "midi.h"
#include "options.h"

//#define MIDI_TRACING_ENABLED

#ifdef MIDI_TRACING_ENABLED
#define TRACE(x) do { write_log x; } while(0)
#else
#define TRACE(x)
#endif

// parameter length of midi commands
static const uae_u8 param_len[128] = {
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	1,1,2,1,0,0,0,0,0,0,0,0,0,0,0,0
};

#if defined(_WIN32)
// ========================================================================
// Windows native MIDI implementation using Windows Multimedia API
// ========================================================================

#include <mmsystem.h>

#define IN_QUEUE_SIZE 256

static HMIDIOUT out_handle;
static HMIDIIN in_handle;
static int out_dev_id = -1;
static int in_dev_id = -1;

// Input ring buffer
static uae_u8 in_ring[IN_QUEUE_SIZE];
static volatile int in_ring_wr = 0;
static volatile int in_ring_rd = 0;
static uae_sem_t in_ring_sem;

// Sysex output buffer
#define SYSEX_BUFLEN 65536
static uae_u8 sysex_buf[SYSEX_BUFLEN];
static int sysex_len = 0;
static MIDIHDR sysex_hdr;

static void in_ring_add(uae_u8 byte)
{
	uae_sem_wait(&in_ring_sem);
	in_ring[in_ring_wr] = byte;
	in_ring_wr = (in_ring_wr + 1) % IN_QUEUE_SIZE;
	if (in_ring_wr == in_ring_rd) {
		// overflow: advance read pointer
		in_ring_rd = (in_ring_rd + 1) % IN_QUEUE_SIZE;
	}
	uae_sem_post(&in_ring_sem);
}

static void CALLBACK midi_in_callback(HMIDIIN hMidiIn, UINT wMsg,
	DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (wMsg == MIM_DATA) {
		DWORD msg = (DWORD)dwParam1;
		uae_u8 status = msg & 0xFF;
		int bytes = 1;
		if (status < 0xF0) {
			bytes = 1 + param_len[status & 0x7F];
		} else if (status == 0xF1 || status == 0xF3) {
			bytes = 2;
		} else if (status == 0xF2) {
			bytes = 3;
		}
		for (int i = 0; i < bytes; i++) {
			in_ring_add((uae_u8)(msg & 0xFF));
			msg >>= 8;
		}
	} else if (wMsg == MIM_LONGDATA) {
		MIDIHDR* hdr = (MIDIHDR*)dwParam1;
		if (hdr->dwBytesRecorded > 0) {
			for (DWORD i = 0; i < hdr->dwBytesRecorded; i++) {
				in_ring_add((uae_u8)hdr->lpData[i]);
			}
		}
		// Re-add the buffer for more sysex data
		if (in_handle) {
			midiInAddBuffer(in_handle, hdr, sizeof(MIDIHDR));
		}
	}
}

static void parse_config(const uae_prefs* cfg)
{
	out_dev_id = -1;
	in_dev_id = -1;

	UINT num_out = midiOutGetNumDevs();
	UINT num_in = midiInGetNumDevs();

	for (UINT i = 0; i < num_out; i++) {
		MIDIOUTCAPS caps;
		if (midiOutGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
			if (cfg->midioutdev[0] && _tcscmp(cfg->midioutdev, caps.szPname) == 0) {
				out_dev_id = i;
			}
		}
	}
	for (UINT i = 0; i < num_in; i++) {
		MIDIINCAPS caps;
		if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
			if (cfg->midiindev[0] && _tcscmp(cfg->midiindev, caps.szPname) == 0) {
				in_dev_id = i;
			}
		}
	}

	// Fall back to first available device
	if (out_dev_id < 0 && num_out > 0)
		out_dev_id = 0;
	if (in_dev_id < 0 && num_in > 0)
		in_dev_id = 0;
}

int midi_open()
{
	write_log(_T("midi_open() [Windows native]\n"));

	in_ring_sem = nullptr;
	uae_sem_init(&in_ring_sem, 0, 1);
	in_ring_wr = 0;
	in_ring_rd = 0;

	parse_config(&currprefs);

	bool opened_any = false;

	// Open output
	if (out_dev_id >= 0) {
		MMRESULT res = midiOutOpen(&out_handle, out_dev_id, 0, 0, CALLBACK_NULL);
		if (res == MMSYSERR_NOERROR) {
			write_log(_T("MIDI OUT: opened device #%d\n"), out_dev_id);
			opened_any = true;
		} else {
			write_log(_T("MIDI OUT: ERROR opening device #%d, code=%d\n"), out_dev_id, res);
			out_handle = nullptr;
		}
	}

	// Open input
	if (in_dev_id >= 0) {
		MMRESULT res = midiInOpen(&in_handle, in_dev_id,
			(DWORD_PTR)midi_in_callback, 0, CALLBACK_FUNCTION);
		if (res == MMSYSERR_NOERROR) {
			write_log(_T("MIDI IN: opened device #%d\n"), in_dev_id);
			midiInStart(in_handle);
			opened_any = true;
		} else {
			write_log(_T("MIDI IN: ERROR opening device #%d, code=%d\n"), in_dev_id, res);
			in_handle = nullptr;
		}
	}

	if (opened_any) {
		write_log(_T("midi_open(): ok\n"));
		return 1;
	}

	write_log(_T("midi_open(): failed!\n"));
	return 0;
}

void midi_close()
{
	write_log(_T("midi_close() [Windows native]\n"));

	if (in_handle) {
		midiInStop(in_handle);
		midiInReset(in_handle);
		midiInClose(in_handle);
		in_handle = nullptr;
	}

	if (out_handle) {
		midiOutReset(out_handle);
		midiOutClose(out_handle);
		out_handle = nullptr;
	}

	uae_sem_destroy(&in_ring_sem);
	write_log(_T("midi_close(): done\n"));
}

// ----- MIDI OUT -----

static DWORD out_msg;
static int out_msg_bits;
static int out_msg_bytes;
static uae_u8 out_msg_last_command;
static int out_sysex_mode;

static void write_short_msg(DWORD msg)
{
	if (!out_handle)
		return;
	MMRESULT err = midiOutShortMsg(out_handle, msg);
	if (err != MMSYSERR_NOERROR) {
		static MMRESULT last_err;
		if (err != last_err) {
			write_log(_T("MIDI OUT: midiOutShortMsg error %d\n"), err);
		}
		last_err = err;
	}
	TRACE((_T("-> midi_out: %08x\n"), msg));
}

static void flush_sysex()
{
	if (!out_handle || sysex_len == 0)
		return;

	memset(&sysex_hdr, 0, sizeof(sysex_hdr));
	sysex_hdr.lpData = (LPSTR)sysex_buf;
	sysex_hdr.dwBufferLength = sysex_len;
	sysex_hdr.dwBytesRecorded = sysex_len;

	MMRESULT res = midiOutPrepareHeader(out_handle, &sysex_hdr, sizeof(sysex_hdr));
	if (res == MMSYSERR_NOERROR) {
		res = midiOutLongMsg(out_handle, &sysex_hdr, sizeof(sysex_hdr));
		if (res != MMSYSERR_NOERROR) {
			write_log(_T("MIDI OUT: midiOutLongMsg error %d\n"), res);
		}
		// Wait for completion then unprepare
		while ((sysex_hdr.dwFlags & MHDR_DONE) == 0)
			Sleep(1);
		midiOutUnprepareHeader(out_handle, &sysex_hdr, sizeof(sysex_hdr));
	} else {
		write_log(_T("MIDI OUT: midiOutPrepareHeader error %d\n"), res);
	}
	sysex_len = 0;
}

void midi_send_byte(uint8_t ch)
{
	TRACE((_T("midi_send_byte(%02x)\n"), ch));

	if ((ch & 0x80) == 0x80) {
		// Status byte
		if (ch == 0xf0) {
			// SYSEX begin
			out_sysex_mode = 1;
			sysex_len = 0;
			sysex_buf[sysex_len++] = ch;
		} else if (ch == 0xf7) {
			// SYSEX end
			if (out_sysex_mode) {
				sysex_buf[sysex_len++] = ch;
				flush_sysex();
				out_sysex_mode = 0;
			}
		} else {
			if ((ch < 0xf8) && out_sysex_mode) {
				// Abort sysex
				sysex_buf[sysex_len++] = 0xf7;
				flush_sysex();
				out_sysex_mode = 0;
			}
			out_msg_bytes = param_len[ch & 0x7f];
			out_msg = ch;
			out_msg_last_command = ch;
			out_msg_bits = 8;
			if (out_msg_bytes == 0) {
				write_short_msg(out_msg);
			}
		}
	} else {
		// Data byte
		if (out_sysex_mode) {
			if (sysex_len < SYSEX_BUFLEN)
				sysex_buf[sysex_len++] = ch;
		} else if (out_msg_bytes > 0) {
			out_msg |= ch << out_msg_bits;
			out_msg_bits += 8;
			out_msg_bytes--;
			if (out_msg_bytes == 0) {
				write_short_msg(out_msg);
			}
		} else {
			// Running status
			out_msg = out_msg_last_command & 0xff;
			out_msg |= ch << 8;
			out_msg_bits = 16;
			out_msg_bytes = param_len[out_msg_last_command & 0x7f] - 1;
			if (out_msg_bytes == 0) {
				write_short_msg(out_msg);
			}
		}
	}
}

// ----- MIDI IN -----

int midi_has_byte()
{
	uae_sem_wait(&in_ring_sem);
	int res = (in_ring_rd != in_ring_wr);
	uae_sem_post(&in_ring_sem);
	return res;
}

int midi_recv_byte(uint8_t *ch)
{
	uae_sem_wait(&in_ring_sem);
	if (in_ring_rd != in_ring_wr) {
		*ch = in_ring[in_ring_rd];
		in_ring_rd = (in_ring_rd + 1) % IN_QUEUE_SIZE;
		uae_sem_post(&in_ring_sem);
		TRACE((_T("midi_recv: %02x\n"), *ch));
		return 1;
	}
	uae_sem_post(&in_ring_sem);
	return 0;
}

#elif defined(USE_PORTMIDI)
// ========================================================================
// PortMidi implementation (Linux, macOS, FreeBSD)
// ========================================================================

#include "portmidi.h"
#include "porttime.h"

#define IN_QUEUE_SIZE 1024
#define OUT_QUEUE_SIZE 1024

// local data
static PmDeviceID in_dev_id = pmNoDevice;
static PmDeviceID out_dev_id = pmNoDevice;
static PortMidiStream *out;
static PortMidiStream *in;

#define MY_IN_QUEUE_SIZE 64
static int timer_active = 0;

typedef struct {
	uae_sem_t   sem;
	PmMessage  *buffer;
	int         wr_off;
	int         rd_off;
} msg_buffer_t;
msg_buffer_t *in_buf;

static msg_buffer_t *buffer_init(int size)
{
	const auto buf = static_cast<msg_buffer_t*>(malloc(sizeof(msg_buffer_t)));
	if(buf == nullptr) {
		return nullptr;
	}
	buf->buffer = static_cast<PmMessage*>(malloc(sizeof(PmMessage) * size));
	if(buf->buffer == nullptr) {
		free(buf);
		return nullptr;
	}
	buf->sem = nullptr;
	uae_sem_init(&buf->sem, 0, 1);
	buf->wr_off = 0;
	buf->rd_off = 0;
	return buf;
}

static void buffer_free(msg_buffer_t *buf)
{
	free(buf->buffer);
	free(buf);
}

static int buffer_add_msg(msg_buffer_t *buf, PmMessage msg)
{
	int overflow = 0;
	uae_sem_wait(&buf->sem);
	buf->buffer[buf->wr_off] = msg;
	int new_pos = (buf->wr_off + 1) % MY_IN_QUEUE_SIZE;
	if(new_pos == buf->rd_off) {
		overflow = 1;
		buf->rd_off = (buf->rd_off + 1) & MY_IN_QUEUE_SIZE;
	}
	buf->wr_off = new_pos;
	uae_sem_post(&buf->sem);
	return overflow;
}

static int buffer_has_msg(const msg_buffer_t *buf)
{
	uae_sem_wait(&buf->sem);
	const int res = buf->rd_off != buf->wr_off;
	uae_sem_post(&buf->sem);
	return res;
}

static int buffer_get_msg(msg_buffer_t *buf, PmMessage *msg)
{
	int res = 0;
	uae_sem_wait(&buf->sem);
	if(buf->rd_off != buf->wr_off) {
		*msg = buf->buffer[buf->rd_off];
		buf->rd_off = (buf->rd_off + 1) % MY_IN_QUEUE_SIZE;
		res = 1;
	}
	uae_sem_post(&buf->sem);
	return res;
}

// timer callback
static void timer_callback(PtTimestamp timestamp, void *userData)
{
	if(timer_active && (in != nullptr)) {
		// read incoming midi command
		if(Pm_Poll(in) == TRUE) {
			PmEvent ev;
			const int err = Pm_Read(in, &ev, 1);
			if(err == 1) {
				TRACE((_T("<- midi_in:  %08x\n"),ev.message));
				// add to in queue
				int overflow = buffer_add_msg(in_buf, ev.message);
				if(overflow) {
					write_log(_T("MIDI IN: buffer overflow!\n"));
				}
			} else {
				write_log(_T("MIDI IN: read error: %d!\n"), err);
			}
		}
	}
}

static void parse_config(const uae_prefs *cfg)
{
	// set defaults
	in_dev_id = Pm_GetDefaultInputDeviceID();
	out_dev_id = Pm_GetDefaultOutputDeviceID();

	int num = Pm_CountDevices();

	// search device
	for(int i = 0; i < num; i++) {
		const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
		if(info->input) {
			if(cfg->midiindev[0]) {
				if(strcmp(cfg->midiindev, info->name) == 0) {
					in_dev_id = i;
				}
			}
		}
		if(info->output) {
			if(cfg->midioutdev[0]) {
				if(strcmp(cfg->midioutdev, info->name) == 0) {
					out_dev_id = i;
				}
			}
		}
	}

}

static PortMidiStream *open_out_stream()
{
	PortMidiStream *out;

	if(out_dev_id == pmNoDevice) {
		write_log(_T("MIDI OUT: ERROR: no device found!\n"));
	} else {
		const PmError err = Pm_OpenOutput(&out, out_dev_id, nullptr, OUT_QUEUE_SIZE,
		                            nullptr, nullptr, 0);
		if(err != pmNoError) {
			write_log(_T("MIDI OUT: ERROR: can't open device #%d! code=%d\n"),
			 	out_dev_id, err);
		} else {
			write_log(_T("MIDI OUT: opened device #%d.\n"), out_dev_id);
			return out;
		}
	}
	return nullptr;
}

static PortMidiStream *open_in_stream()
{
	PortMidiStream *in;

	if(in_dev_id == pmNoDevice) {
		write_log(_T("MIDI IN: ERROR: no device found!\n"));
	} else {
		const PmError err = Pm_OpenInput(&in, in_dev_id, nullptr, IN_QUEUE_SIZE,
		                           nullptr, nullptr);
		if(err != pmNoError) {
			write_log(_T("MIDI IN: ERROR: can't open device #%d! code=%d\n"),
			 	in_dev_id, err);
		} else {
			write_log(_T("MIDI IN: opened device #%d.\n"), in_dev_id);
			return in;
		}
	}
	return nullptr;
}

static int midi_open_with_config(const uae_prefs *cfg)
{
	write_log(_T("midi_open()\n"));

	// setup timer
	PtError err = Pt_Start(1, timer_callback, nullptr);
	if(err != ptNoError) {
		write_log(_T("MIDI: ERROR: no timer!\n"));
		return 0;
	}

	// setup input buffer
	in_buf = buffer_init(MY_IN_QUEUE_SIZE);
	if(in_buf == nullptr) {
		write_log(_T("MIDI: ERROR: no buffer!\n"));
		return 0;
	}

	// init port midi
	Pm_Initialize();

	parse_config(cfg);

	// open streams
	out = open_out_stream();
	in = open_in_stream();

	timer_active = 1;

	// ok if any direction was opened
	if((out != nullptr) || (in != nullptr)) {
		write_log(_T("midi_open(): ok\n"));
		return 1;
	} else {
		write_log(_T("midi_open(): failed!\n"));
		return 0;
	}
}

int midi_open()
{
	return midi_open_with_config(&currprefs);
}

void midi_close()
{
	write_log(_T("midi_close()\n"));

	// stop timer
	timer_active = 0;
	Pt_Stop();

	// close output
	if(out != nullptr) {
		Pm_Close(out);
		out = nullptr;
	}
	// close input
	if(in != nullptr) {
		Pm_Close(in);
		in = nullptr;
	}

	Pm_Terminate();

	// free input buffer
	buffer_free(in_buf);
	in_buf = nullptr;

	write_log(_T("midi_close(): done\n"));
}

// ----- MIDI OUT -----

static PmMessage out_msg;
static int out_msg_bits;
static int out_msg_bytes;

static char out_msg_last_command;

static PmMessage out_sysex_msg;
static int out_sysex_mode;
static int out_sysex_bits;

static void write_msg(PmMessage msg)
{
	static int last_result;
	// FIXME: Should we use timestamps here? Calculate when a midi events based
	// on frame start time + active line (fraction of frame time)?
	PmError err = Pm_WriteShort(out, 0, msg);
	if(err != pmNoError) {
		// Throttle the errors by not logging repeated identical errors.
		if (err != last_result) {
			write_log(_T("MIDI OUT: Write error: %d!\n"), err);
		}
	}
	last_result = err;
	TRACE((_T("-> midi_out: %04x\n"), msg));
}

static void out_add_sysex_byte(uint8_t data)
{
	out_sysex_msg |= data << out_sysex_bits;
	out_sysex_bits += 8;
	TRACE((_T("sysex: %02x -> %08x\n"), data, out_sysex_msg));
	if(out_sysex_bits == 32) {
		write_msg(out_sysex_msg);
		out_sysex_bits = 0;
		out_sysex_msg = 0;
	}
}

static void out_flush_sysex()
{
	if(out_sysex_bits != 0) {
		write_msg(out_sysex_msg);
	}
}

void midi_send_byte(uint8_t ch)
{
	TRACE((_T("midi_send_byte(%02x)\n"), ch));

	// is a status byte (begin of command?)
	if((ch & 0x80) == 0x80) {
		// SYSEX begin
		if(ch == 0xf0) {
			TRACE((_T("midi out sysex: begin\n")));
			out_sysex_mode = 1;
			out_sysex_msg = ch;
			out_sysex_bits = 8;
		}
		// SYSEX end / EOX
		else if(ch == 0xf7) {
			TRACE((_T("midi out sysex: end\n")));
			out_add_sysex_byte(0xf7);
			out_flush_sysex();
			out_sysex_mode = 0;
		}
		// other command
		else {
			// auto-terminate sysex if no sys realtime message
			if((ch < 0xf8) && out_sysex_mode) {
				TRACE((_T("midi out sysex: abort!\n")));
				out_add_sysex_byte(0xf7);
				out_flush_sysex();
				out_sysex_mode = 0;
			}
			// get number of parameter bytes for this message
			out_msg_bytes = param_len[ch & 0x7f];
			// prepare msg
			out_msg = ch;
			out_msg_last_command = ch;
			// setup bit shift for next byte
			out_msg_bits = 8;
			// send single byte msg
			if(out_msg_bytes == 0) {
				write_msg(out_msg);
			}
		}
	}
	// data byte
	else {
		// fill in command parameters (1 or 2)
		if(out_msg_bytes > 0) {
			// store next byte in msg
			out_msg |= ch << out_msg_bits;
			out_msg_bits += 8;
			out_msg_bytes--;
			// msg ready to be sent
			if(out_msg_bytes == 0) {
				write_msg(out_msg);
			}
		}
		// store bytes in sysex transfer
		else if(out_sysex_mode) {
			out_add_sysex_byte(ch);
		}

		// unexpected data bytes after complete message, so repeat last command
		else {
			// initialize message with last command
			out_msg = out_msg_last_command &0xff;

			// add current byte to message
			out_msg |= ch << 8;

			// set current number of bits
			out_msg_bits = 16;

			// set remaining number of bytes according to last command
			out_msg_bytes = param_len[out_msg_last_command & 0x7f] - 1;
		}
	}
}

// ----- MIDI IN -----

static PmMessage in_msg;
static int in_msg_bytes;
static int in_msg_bits;

static int in_sysex_mode;

int midi_has_byte()
{
	// still have to send parameter bytes
	if(in_msg_bytes > 0) {
		return 1;
	}

	// are there more commands in the queue?
	return buffer_has_msg(in_buf);
}

static int in_handle_sysex(PmMessage msg, int check_status)
{
	if(check_status) {
		// is a status?
		uint8_t status = Pm_MessageStatus(msg);
		if(status >= 0x80) {
			// system realtime message is allowed
			if(status >= 0xf8) {
				TRACE((_T("midi in sysex: rt msg!\n")));
				in_msg_bytes = 0;
				return 0;
			}
			// invalid other message -> end sysex
			else {
				TRACE((_T("midi in sysex: abort!\n")));
				in_msg_bytes = param_len[status & 0x7f];
				return 1;
			}
		}
	}

	// check for a possible EOX
	if((msg & 0x80808080) != 0) {
		for(int i=0;i<4;i++) {
			if((msg & 0xff) == 0xf7) {
				in_msg_bytes = i;
				return 1;
			}
			msg >>= 8;
		}
	}

	// keep full u32 of data
	in_msg_bytes = 3;
	return 0;
}

int midi_recv_byte(uint8_t *ch)
{
	int res = 0;
	// sitll have to send parameter bytes
	if(in_msg_bytes > 0) {
		*ch = static_cast<uint8_t>(in_msg >> in_msg_bits);
		in_msg_bits += 8;
		in_msg_bytes--;
		res = 1;
	}
	else {
		// fetch a new command from buffer
		PmMessage msg;
		if(buffer_get_msg(in_buf, &msg)) {
			in_msg = msg;
			uint8_t status = Pm_MessageStatus(msg);
			// sysex data or possible end EOX?
			if(in_sysex_mode) {
				if(in_handle_sysex(msg, 1)) {
					TRACE((_T("midi in sysex: end\n")));
					in_sysex_mode = 0;
				}
			}
			// SYSEX start
			else if(status == 0xf0) {
				if(!in_handle_sysex(msg, 0)) {
					TRACE((_T("midi in sysex: begin\n")));
					in_sysex_mode = 1;
				} else {
					// sysex fits into u32
					TRACE((_T("midi in sysex: tiny\n")));
				}
			}
			// regular cmd
			else {
				in_msg_bytes = param_len[status & 0x7f];
			}

			in_msg_bits = 8;
			*ch = status;
			res = 1;
			TRACE((_T("in %02x bytes %d\n"), status, in_msg_bytes));
		}
	}
	if(res) {
		TRACE((_T("midi_recv: %02x\n"), *ch));
	}
	return res;
}

#else
// No MIDI backend available
int midi_open() { return 0; }
void midi_close() {}
void midi_send_byte(uint8_t ch) {}
int midi_has_byte() { return 0; }
int midi_recv_byte(uint8_t *ch) { return 0; }
#endif // _WIN32 / USE_PORTMIDI

// ========================================================================
// Common compatibility wrappers (shared by all backends)
// ========================================================================

int Midi_Open()
{
	return midi_open();
}

void Midi_Close()
{
	midi_close();
}

void Midi_Reopen()
{
	if (midi_ready) {
		Midi_Close();
		Midi_Open();
	}
}

int Midi_Parse(midi_direction_e direction, uint8_t* c)
{
	if (direction == midi_input) {
		return midi_recv_byte(c);
	}
	else {
		midi_send_byte(*c);
		return 1;
	}
}

#endif /* WITH_MIDI */
