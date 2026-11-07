/*
 * Shared MIDI stream parser, derived from the Windows and Unix backends.
 * Original MIDI parser: Brian King, Copyright 1999, 2000 under the GPL.
 * Amiberry: SysEx longer than the buffer goes out in pieces, not cut short.
 */
#include "sysconfig.h"
#include "sysdeps.h"

#include "midi_parser.h"

static const uae_u8 message_lengths[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

void midi_parser_reset(struct midi_parser *parser)
{
	parser->message[0] = 0;
	parser->length = 0;
	parser->position = 0;
	parser->sysex_length = 0;
	parser->in_sysex = false;
}

/* A SysEx message has no length limit: when the buffer is full, it is
 * handed over and filled again. One byte stays free for the EOX, so every
 * piece after the first starts with a data byte. */
static void midi_parser_sysex_data(struct midi_parser *parser, uae_u8 data,
	midi_parser_output output)
{
	if (parser->sysex_length >= MIDI_PARSER_BUFFER_SIZE - 1) {
		output(parser->sysex, parser->sysex_length);
		parser->sysex_length = 0;
	}
	parser->sysex[parser->sysex_length++] = data;
}

void midi_parser_byte(struct midi_parser *parser, uae_u8 data,
	midi_parser_output output)
{
	/* Real-time messages can occur between any two bytes, including SysEx. */
	if (data >= 0xf8) {
		output(&data, 1);
		return;
	}
	if (data & 0x80) {
		if (parser->in_sysex) {
			/* EOX, or a status that ends the message without one. */
			parser->sysex[parser->sysex_length++] = 0xf7;
			output(parser->sysex, parser->sysex_length);
			parser->sysex_length = 0;
			parser->in_sysex = false;
		}
		parser->message[0] = data;
		parser->position = 0;
		parser->length = message_lengths[(data >> 4) & 7];
		if (data == 0xf0) {
			parser->sysex[0] = data;
			parser->sysex_length = 1;
			parser->in_sysex = true;
		} else if (data == 0xf1 || data == 0xf3) {
			parser->length = 1;
		} else if (data == 0xf2) {
			parser->length = 2;
		} else if (data == 0xf6) {
			output(&data, 1);
		}
		return;
	}
	if (parser->in_sysex) {
		midi_parser_sysex_data(parser, data, output);
		return;
	}
	if (!parser->length) {
		return;
	}
	parser->message[++parser->position] = data;
	if (parser->position == parser->length) {
		output(parser->message, parser->length + 1);
		parser->position = 0;
		/* Running status applies only to channel messages. */
		if (parser->message[0] >= 0xf0) {
			parser->length = 0;
		}
	}
}
