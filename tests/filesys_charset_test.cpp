// Host file name conversion between UTF-8 and ISO-8859-1
// (src/osdep/amiberry_filesys_charset.h).

#include <cstdio>
#include <string>
#include <vector>

#include "amiberry_filesys_charset.h"

static int failures;

static void expect_true(bool ok, const char* what)
{
	if (!ok) {
		std::fprintf(stderr, "FAIL: %s\n", what);
		++failures;
	}
}

static void expect_host(const std::string& nname, const std::vector<amiberry_mount_root>& roots,
	const std::string& want, const char* what)
{
	expect_true(amiberry_nname_to_host(nname, roots) == want, what);
}

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

	expect_true(amiberry_valid_utf8("caf\xC3\xA9"), "UTF-8 e-acute is valid");
	expect_true(!amiberry_valid_utf8("caf\xE9"), "CP1252 e-acute is not UTF-8");
	expect_true(!amiberry_valid_utf8("\xED\xA0\x80"), "UTF-16 surrogate is rejected");
	expect_true(!amiberry_valid_utf8("\xC0\xAF"), "overlong slash is rejected");

	// A mount root is a host path from the configuration; only what follows
	// it is Latin-1. Converting the whole nname encoded the root twice.
	const std::vector<amiberry_mount_root> roots = {
		{ "/home/jos\xC3\xA9/amiga", "/home/jos\xC3\xA9/amiga", "/home/jos\xC3\xA9/amiga" },
		{ "D:\\M\xFAsica", "D:\\M\xC3\xBAsica", "D:\\M\xC3\xBAsica" },
		{ "/m", "/m", "/m" },
		{ "/m/\xE9", "/m/\xE9", "/m/\xE9" },
		// macOS: decomposed e + U+0301 in the configuration, composed key
		{ "/Users/cafe\xCC\x81", "/Users/cafe\xCC\x81", "/Users/caf\xC3\xA9" },
	};
	expect_host("/home/jos\xC3\xA9/amiga/caf\xE9.txt", roots, "/home/jos\xC3\xA9/amiga/caf\xC3\xA9.txt",
		"UTF-8 root stays, Latin-1 name below it is converted");
	expect_host("/home/jos\xC3\xA9/amiga", roots, "/home/jos\xC3\xA9/amiga", "the root itself");
	expect_host("/home/jos\xC3\xA9/amigaX/f", roots, "/home/jos\xC3\x83\xC2\xA9/amigaX/f",
		"a root only matches up to a path separator");
	expect_host("D:\\M\xFAsica\\\xE9t\xE9", roots, "D:\\M\xC3\xBAsica\\\xC3\xA9t\xC3\xA9",
		"ANSI code page root on Windows");
	expect_host("/m/\xE9/a", roots, "/m/\xE9/a", "longest root wins, its bytes are kept");
	expect_host("rel\xE9", roots, "rel\xC3\xA9", "no root: the whole name is Latin-1");

	std::string nname;
	expect_true(amiberry_host_to_nname("/home/jos\xC3\xA9/amiga/d\xC3\xA9j\xC3\xA0", roots, nname)
		&& nname == "/home/jos\xC3\xA9/amiga/d\xE9j\xE0", "host path below a root maps back to an nname");
	expect_true(amiberry_nname_to_host(nname, roots) == "/home/jos\xC3\xA9/amiga/d\xC3\xA9j\xC3\xA0",
		"nname round trip");
	expect_true(!amiberry_host_to_nname("/home/jos\xC3\xA9/amiga/\xE6\x97\xA5", roots, nname),
		"name below the root outside Latin-1");
	expect_true(amiberry_host_to_nname("/Users/caf\xC3\xA9/l\xC3\xA0", roots, nname)
		&& nname == "/Users/cafe\xCC\x81/l\xE0", "composed host path keeps the decomposed root bytes");

	if (failures)
		return 1;
	std::puts("filesys_charset_test: all checks passed");
	return 0;
}
