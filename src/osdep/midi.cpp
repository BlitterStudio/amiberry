/*
 * FS-UAE - The Un*x Amiga Emulator
 *
 * Serial-to-MidiPort interface
 *
 * Written by Christian Vogelgsang <chris@vogelgsang.org>
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "thread.h"

#ifdef WITH_MIDI

#include "midi.h"
#include "options.h"
#include "portmidi.h"
#include "porttime.h"

//#define MIDI_TRACING_ENABLED

#ifdef MIDI_TRACING_ENABLED
#define TRACE(x) do { write_log x; } while(0)
#else
#define TRACE(x)
#endif

#define IN_QUEUE_SIZE 1024
#define OUT_QUEUE_SIZE 1024

// local data
static PmDeviceID in_dev_id = pmNoDevice;
static PmDeviceID out_dev_id = pmNoDevice;
static PortMidiStream *out;
static PortMidiStream *in;

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

// The following functions are added for compatibility with WinUAE / PCem code.
// The Midi_Parse function has not been tested yet, so this might not work
// correctly.

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
