// Host file name conversion between UTF-8 and ISO-8859-1
// (src/osdep/amiberry_filesys_charset.h).

#include <cstdio>
#include <string>

#include "amiberry_filesys_charset.h"

static int failures;

static void expect_decode(const std::string& input, const std::string& want, size_t want_invalid, const char* what)
{
	std::string got = "stale";
	const size_t invalid = amiberry_utf8_to_latin1(input, got);
	if (got != want || invalid != want_invalid) {
		std::fprintf(stderr, "FAIL: %s: got %zu invalid, \"", what, invalid);
		for (const unsigned char c : got)
			std::fprintf(stderr, c >= 0x20 && c < 0x7F ? "%c" : "\\x%02X", c);
		std::fprintf(stderr, "\"\n");
		++failures;
	}
}

int main()
{
	expect_decode("", "", 0, "empty name");
	expect_decode("Workbench.info", "Workbench.info", 0, "ASCII is copied");
	expect_decode("caf\xC3\xA9", "caf\xE9", 0, "UTF-8 e-acute becomes Latin-1");
	expect_decode("\xC2\x80\xC2\xA0\xC3\xBF", "\x80\xA0\xFF", 0, "first and last two-byte Latin-1 code points");

	// Windows names in CP1252 are not UTF-8. A lead byte at the end used to
	// leave SDL_iconv stuck on it forever.
	expect_decode("A Jacarand\xE1", "A Jacarand?", 1, "truncated sequence at the end");
	expect_decode("caf\xE9", "caf?", 1, "two-byte lead at the end");
	expect_decode("Jacarand\xE1 Bar", "Jacarand? Bar", 1, "truncated sequence keeps the next character");
	expect_decode("\xF0\x9F", "?", 1, "truncated four-byte sequence");

	expect_decode("\xE2\x82\xAC 5", "? 5", 1, "euro sign is outside Latin-1");
	expect_decode("\xE6\x97\xA5\xE6\x9C\xAC", "??", 2, "one mark per CJK character");
	expect_decode("\xF0\x9F\x8E\xB5.mod", "?.mod", 1, "four-byte character");
	expect_decode("\xC1\xA9", "??", 2, "overlong form is not decoded");
	expect_decode("\xA9x", "?x", 1, "stray continuation byte");
	expect_decode("\xFFok", "?ok", 1, "byte that never starts UTF-8");

	for (int c = 0; c < 256; ++c) {
		const std::string latin1(1, static_cast<char>(c));
		std::string back;
		const size_t invalid = amiberry_utf8_to_latin1(iso_8859_1_to_utf8(latin1), back);
		if (invalid != 0 || back != latin1) {
			std::fprintf(stderr, "FAIL: Latin-1 0x%02X does not survive a UTF-8 round trip\n", c);
			++failures;
		}
	}

	if (failures)
		return 1;
	std::puts("filesys_charset_test: all checks passed");
	return 0;
}
