/*
 * Amiberry - Amiga Emulator (libretro core)
 *
 * MIDI support via libretro MIDI interface
 *
 * Implements the 5 core MIDI functions (midi_open, midi_close,
 * midi_send_byte, midi_recv_byte, midi_has_byte) plus the 4 WinUAE
 * compatibility wrappers (Midi_Open, Midi_Close, Midi_Reopen, Midi_Parse)
 * by delegating to the frontend's retro_midi_interface.
 */

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef WITH_MIDI

#include "libretro.h"
#include "midi.h"
#include "options.h"

// Provided by libretro.cpp
extern struct retro_midi_interface *midi_iface_ptr;

// 1-byte lookahead buffer for midi_has_byte() / midi_recv_byte()
// The libretro MIDI interface has no non-destructive peek, so we
// read one byte ahead and buffer it.
static bool lookahead_valid = false;
static uint8_t lookahead_byte = 0;

int midi_open(void)
{
	if (!midi_iface_ptr)
		return 0;
	if (!midi_iface_ptr->output_enabled())
		return 0;

	lookahead_valid = false;
	write_log(_T("MIDI: opened via libretro interface\n"));
	return 1;
}

void midi_close(void)
{
	if (midi_iface_ptr)
		midi_iface_ptr->flush();

	lookahead_valid = false;
	write_log(_T("MIDI: closed\n"));
}

void midi_send_byte(uint8_t ch)
{
	if (midi_iface_ptr)
		midi_iface_ptr->write(ch, 0);
}

int midi_has_byte(void)
{
	if (lookahead_valid)
		return 1;

	if (!midi_iface_ptr || !midi_iface_ptr->input_enabled())
		return 0;

	if (midi_iface_ptr->read(&lookahead_byte)) {
		lookahead_valid = true;
		return 1;
	}
	return 0;
}

int midi_recv_byte(uint8_t *ch)
{
	if (lookahead_valid) {
		*ch = lookahead_byte;
		lookahead_valid = false;
		return 1;
	}

	if (!midi_iface_ptr || !midi_iface_ptr->input_enabled())
		return 0;

	return midi_iface_ptr->read(ch) ? 1 : 0;
}

// WinUAE compatibility wrappers

int Midi_Open(void)
{
	return midi_open();
}

void Midi_Close(void)
{
	midi_close();
}

void Midi_Reopen(void)
{
	if (midi_ready) {
		Midi_Close();
		Midi_Open();
	}
}

int Midi_Parse(midi_direction_e direction, uint8_t *c)
{
	if (direction == midi_input) {
		return midi_recv_byte(c);
	} else {
		midi_send_byte(*c);
		return 1;
	}
}

#endif /* WITH_MIDI */
