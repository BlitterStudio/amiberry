// MIDI output stream parser (src/osdep/midi_parser.cpp, shared with WinUAE;
// tests ported from WinUAE's od-generic/midi_parser_test.cpp).

#include "sysconfig.h"
#include "sysdeps.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "midi_parser.h"

static std::vector<std::vector<uae_u8>> packets;

static void output(const uae_u8 *data, int length)
{
	packets.emplace_back(data, data + length);
}

static void check(bool condition)
{
	if (!condition) {
		fprintf(stderr, "MIDI parser regression\n");
		exit(1);
	}
}

static void feed(midi_parser *parser, std::initializer_list<uae_u8> bytes)
{
	for (uae_u8 byte : bytes) {
		midi_parser_byte(parser, byte, output);
	}
}

int main(void)
{
	midi_parser parser;
	midi_parser_reset(&parser);
	feed(&parser, { 0x3c, 0x7f });
	check(packets.empty());
	/* Clock between the data bytes must preserve the incomplete note. */
	feed(&parser, { 0x90, 0x3c, 0xf8, 0x7f, 0x3d, 0x40 });
	check(packets == std::vector<std::vector<uae_u8>>({
		{ 0xf8 }, { 0x90, 0x3c, 0x7f }, { 0x90, 0x3d, 0x40 }
	}));
	packets.clear();
	/* System common packets cancel running status; channel pressure does not. */
	feed(&parser, { 0xf2, 1, 2, 3, 4, 0xd0, 5, 6, 0xf6, 7 });
	check(packets == std::vector<std::vector<uae_u8>>({
		{ 0xf2, 1, 2 }, { 0xd0, 5 }, { 0xd0, 6 }, { 0xf6 }
	}));
	packets.clear();
	feed(&parser, { 0xf0, 0x7d, 1, 0xf8, 2, 0xf7, 3 });
	check(packets == std::vector<std::vector<uae_u8>>({
		{ 0xf8 }, { 0xf0, 0x7d, 1, 2, 0xf7 }
	}));
	packets.clear();
	/* A new non-real-time status terminates SysEx. */
	feed(&parser, { 0xf0, 0x7d, 1, 0xc0, 5 });
	check(packets == std::vector<std::vector<uae_u8>>({
		{ 0xf0, 0x7d, 1, 0xf7 }, { 0xc0, 5 }
	}));
	packets.clear();
	feed(&parser, { 0x90, 0x3c });
	midi_parser_reset(&parser);
	feed(&parser, { 0x7f });
	check(packets.empty());
	/* A SysEx message that fills the buffer exactly, EOX included, is one
	   packet. */
	feed(&parser, { 0xf0 });
	for (int i = 0; i < MIDI_PARSER_BUFFER_SIZE - 2; i++) {
		midi_parser_byte(&parser, 0x12, output);
	}
	feed(&parser, { 0xf7 });
	check(packets.size() == 1);
	check(packets[0].size() == MIDI_PARSER_BUFFER_SIZE);
	check(packets[0].front() == 0xf0 && packets[0].back() == 0xf7);
	/* A longer one goes out in pieces with nothing lost: the first starts
	   with 0xF0, the others with a data byte, the last ends with EOX. A
	   clock byte in the middle still goes out on its own. */
	packets.clear();
	feed(&parser, { 0xf0 });
	for (int i = 0; i < MIDI_PARSER_BUFFER_SIZE * 2; i++) {
		midi_parser_byte(&parser, (uae_u8)(i & 0x7f), output);
		if (i == MIDI_PARSER_BUFFER_SIZE) {
			midi_parser_byte(&parser, 0xf8, output);
		}
	}
	feed(&parser, { 0xf7 });
	{
		std::vector<uae_u8> sysex;
		int pieces = 0, clocks = 0;
		for (const auto &packet : packets) {
			if (packet == std::vector<uae_u8>{ 0xf8 }) {
				clocks++;
				continue;
			}
			check(packet.size() <= MIDI_PARSER_BUFFER_SIZE);
			check(pieces ? packet.front() < 0x80 : packet.front() == 0xf0);
			sysex.insert(sysex.end(), packet.begin(), packet.end());
			pieces++;
		}
		check(clocks == 1 && pieces == 3);
		check(sysex.size() == (size_t)MIDI_PARSER_BUFFER_SIZE * 2 + 2);
		check(sysex.front() == 0xf0 && sysex.back() == 0xf7);
		for (int i = 0; i < MIDI_PARSER_BUFFER_SIZE * 2; i++) {
			check(sysex[i + 1] == (uae_u8)(i & 0x7f));
		}
	}
	/* A new status ends a long message the same way. */
	packets.clear();
	feed(&parser, { 0xf0 });
	for (int i = 0; i < MIDI_PARSER_BUFFER_SIZE; i++) {
		midi_parser_byte(&parser, 0x12, output);
	}
	feed(&parser, { 0x90, 0x3c, 0x7f });
	check(packets.size() == 3);
	check(packets[1].back() == 0xf7);
	check(packets[2] == std::vector<uae_u8>({ 0x90, 0x3c, 0x7f }));
	/* All channel statuses: one- and two-data-byte packets, running status. */
	for (int status = 0x80; status < 0xf0; status++) {
		packets.clear();
		midi_parser_reset(&parser);
		int length = (status & 0xe0) == 0xc0 ? 1 : 2;
		midi_parser_byte(&parser, (uae_u8)status, output);
		for (int repeat = 0; repeat < 2; repeat++) {
			for (int i = 0; i < length; i++) {
				midi_parser_byte(&parser, (uae_u8)(i + 1), output);
			}
		}
		check(packets.size() == 2 && packets[0] == packets[1]);
		check(packets[0].size() == (size_t)length + 1);
		check(packets[0][0] == status);
	}
	puts("MIDI parser tests passed");
	return 0;
}
