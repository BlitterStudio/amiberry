#ifndef UAE_MIDI_PARSER_H
#define UAE_MIDI_PARSER_H

#include "uae/types.h"

#define MIDI_PARSER_BUFFER_SIZE 32766

struct midi_parser {
	uae_u8 message[3];
	int length;
	int position;
	uae_u8 sysex[MIDI_PARSER_BUFFER_SIZE];
	int sysex_length;
	bool in_sysex;
};

/* The receiver must copy data if its host API retains the buffer.
 * A SysEx message longer than the buffer arrives in pieces: the first
 * starts with 0xF0, the others with a data byte, and the last ends with
 * 0xF7. Every other message starts with its status byte, never 0xF0 or a
 * data byte. */
typedef void (*midi_parser_output)(const uae_u8 *data, int length);
void midi_parser_reset(struct midi_parser *parser);
void midi_parser_byte(struct midi_parser *parser, uae_u8 data,
	midi_parser_output output);

#endif
