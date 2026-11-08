#pragma once

// Conversions between host file names (UTF-8) and Amiga file names
// (ISO-8859-1). Kept free of emulator headers so tests can compile it alone.

#include <cstddef>
#include <string>
#include <string_view>

[[nodiscard]] inline std::string iso_8859_1_to_utf8(const std::string_view input) noexcept
{
	size_t high = 0;
	for (const char ch : input)
		high += static_cast<unsigned char>(ch) >= 0x80;
	std::string output;
	output.reserve(input.size() + high);
	for (const char ch : input) {
		const auto byte = static_cast<unsigned char>(ch);
		if (byte < 0x80) {
			output += ch;
		} else {
			output += static_cast<char>(0xC0 | (byte >> 6));
			output += static_cast<char>(0x80 | (byte & 0x3F));
		}
	}
	return output;
}

// Decodes UTF-8 into ISO-8859-1 and returns how many characters could not be
// represented. Each code point above U+00FF and each malformed sequence becomes
// one '?'. A malformed sequence takes its lead byte plus whatever continuation
// bytes follow it, up to the length the lead byte announces, so a truncated
// sequence never swallows the next character and every step moves forward.
inline size_t amiberry_utf8_to_latin1(const std::string_view input, std::string& output)
{
	output.clear();
	output.reserve(input.size());
	size_t invalid = 0;
	size_t i = 0;
	while (i < input.size()) {
		const auto lead = static_cast<unsigned char>(input[i]);
		if (lead < 0x80) {
			output += static_cast<char>(lead);
			++i;
			continue;
		}

		size_t length = 1;
		if (lead >= 0xC2 && lead <= 0xDF)
			length = 2;
		else if (lead >= 0xE0 && lead <= 0xEF)
			length = 3;
		else if (lead >= 0xF0 && lead <= 0xF4)
			length = 4;

		size_t taken = 1;
		while (taken < length && i + taken < input.size()
			&& (static_cast<unsigned char>(input[i + taken]) & 0xC0) == 0x80)
			++taken;

		if (length == 2 && taken == 2 && lead <= 0xC3) {
			const auto trail = static_cast<unsigned char>(input[i + 1]);
			output += static_cast<char>(((lead & 0x1F) << 6) | (trail & 0x3F));
		} else {
			output += '?';
			++invalid;
		}
		i += taken;
	}
	return invalid;
}
