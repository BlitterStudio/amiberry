/*
 * Shared MIDI stream parser, derived from the Windows and Unix backends.
 * Original MIDI parser: Brian King, Copyright 1999, 2000 under the GPL.
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
		if (parser->sysex_length) {
			parser->sysex[parser->sysex_length++] = 0xf7;
			output(parser->sysex, parser->sysex_length);
			parser->sysex_length = 0;
		}
		parser->message[0] = data;
		parser->position = 0;
		parser->length = message_lengths[(data >> 4) & 7];
		if (data == 0xf0) {
			parser->sysex[0] = data;
			parser->sysex_length = 1;
		} else if (data == 0xf1 || data == 0xf3) {
			parser->length = 1;
		} else if (data == 0xf2) {
			parser->length = 2;
		} else if (data == 0xf6) {
			output(&data, 1);
		}
		return;
	}
	if (parser->sysex_length) {
		/* Reserve one byte for EOX even if an oversized message is cut short. */
		if (parser->sysex_length < MIDI_PARSER_BUFFER_SIZE - 1) {
			parser->sysex[parser->sysex_length++] = data;
		}
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
