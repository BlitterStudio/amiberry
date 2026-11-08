#pragma once

// Conversions between host file names (UTF-8) and Amiga file names
// (ISO-8859-1). Kept free of emulator headers so tests can compile it alone.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// Whether text is well-formed UTF-8 (no overlong forms, surrogates or code
// points past U+10FFFF).
[[nodiscard]] inline bool amiberry_valid_utf8(const std::string_view text) noexcept
{
	const auto* p = reinterpret_cast<const unsigned char*>(text.data());
	const auto* end = p + text.size();
	while (p < end) {
		const unsigned char c = *p++;
		if (c < 0x80)
			continue;
		int extra;
		unsigned int cp;
		if (c >= 0xc2 && c <= 0xdf) { extra = 1; cp = c & 0x1f; }
		else if (c >= 0xe0 && c <= 0xef) { extra = 2; cp = c & 0x0f; }
		else if (c >= 0xf0 && c <= 0xf4) { extra = 3; cp = c & 0x07; }
		else return false;
		if (end - p < extra)
			return false;
		for (int i = 0; i < extra; i++) {
			if ((p[i] & 0xc0) != 0x80)
				return false;
			cp = (cp << 6) | (p[i] & 0x3f);
		}
		p += extra;
		if ((extra == 2 && (cp < 0x800 || (cp >= 0xd800 && cp <= 0xdfff)))
			|| (extra == 3 && (cp < 0x10000 || cp > 0x10ffff)))
			return false;
	}
	return true;
}

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

// A directory filesystem name (nname) is the mount root, a host path taken
// from the configuration, followed by Amiga names in ISO-8859-1. Only the
// part after the root is Latin-1, so the root has to be kept as it is.
struct amiberry_mount_root {
	std::string native; // as it appears at the start of nnames
	std::string utf8;   // the same path as UTF-8
};

// The longest root that starts path and ends at a path separator of it.
// Nested mounts are ambiguous in one case: when the inner root's path below
// the outer one is not ASCII, a folder of the outer volume whose name is the
// Latin-1 reading of those bytes (for example "Ã©" for "é") maps to the
// inner root. Telling them apart would need the unit with every nname.
[[nodiscard]] inline const amiberry_mount_root* amiberry_find_mount_root(
	const std::string_view path, const std::vector<amiberry_mount_root>& roots, const bool by_utf8) noexcept
{
	const amiberry_mount_root* best = nullptr;
	for (const auto& root : roots) {
		const std::string& prefix = by_utf8 ? root.utf8 : root.native;
		if (prefix.empty() || path.size() < prefix.size() || path.compare(0, prefix.size(), prefix) != 0)
			continue;
		const bool boundary = path.size() == prefix.size()
			|| prefix.back() == '/' || prefix.back() == '\\'
			|| path[prefix.size()] == '/' || path[prefix.size()] == '\\';
		if (boundary && (!best || prefix.size() > (by_utf8 ? best->utf8 : best->native).size()))
			best = &root;
	}
	return best;
}

// The UTF-8 host path for an nname.
[[nodiscard]] inline std::string amiberry_nname_to_host(
	const std::string_view nname, const std::vector<amiberry_mount_root>& roots)
{
	const amiberry_mount_root* root = amiberry_find_mount_root(nname, roots, false);
	if (!root)
		return iso_8859_1_to_utf8(nname);
	return root->utf8 + iso_8859_1_to_utf8(nname.substr(root->native.size()));
}

// The nname for a UTF-8 host path. Returns false when a name below the root
// has characters outside Latin-1 ('?' stands in for each of them).
[[nodiscard]] inline bool amiberry_host_to_nname(
	const std::string_view host, const std::vector<amiberry_mount_root>& roots, std::string& nname)
{
	const amiberry_mount_root* root = amiberry_find_mount_root(host, roots, true);
	const std::string_view tail = root ? host.substr(root->utf8.size()) : host;
	std::string latin1;
	const size_t invalid = amiberry_utf8_to_latin1(tail, latin1);
	nname = root ? root->native + latin1 : latin1;
	return invalid == 0;
}
