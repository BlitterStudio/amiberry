/* Bounded inspection of classic AmigaDOS process structures. */
#ifndef WINUAE_GDB_AMIGA_H
#define WINUAE_GDB_AMIGA_H

#include <cstdint>
#include <string>
#include <vector>

namespace winuae_gdb {
class GuestReader {
public:
	virtual ~GuestReader() = default;
	virtual bool read(uint32_t address, uint8_t* bytes, size_t length) = 0;
	bool u32(uint32_t address, uint32_t& value) {
		uint8_t bytes[4];
		if ((address & 1) || address > 0xfffffffc || !read(address, bytes, 4)) return false;
		value = (uint32_t(bytes[0]) << 24) | (uint32_t(bytes[1]) << 16) |
			(uint32_t(bytes[2]) << 8) | bytes[3];
		return true;
	}
	bool bptr(uint32_t address, uint32_t& value) {
		if (!u32(address, value) || value > 0x3fffffff) return false;
		value <<= 2;
		return true;
	}
	bool string(uint32_t address, std::string& value, bool counted) {
		value.clear();
		if (!address || address > 0xfffffeff) return false;
		uint8_t length = 255;
		if (counted && !read(address++, &length, 1)) return false;
		for (unsigned i = 0; i < length; ++i) {
			uint8_t c;
			if (!read(address + i, &c, 1)) return false;
			if (!c) return !counted;
			value += char(c);
		}
		return counted;
	}
};

struct AmigaProcess {
	uint32_t address = 0, segments = 0;
	std::string name, command;
};

inline bool read_process(GuestReader& memory, uint32_t address, AmigaProcess& out)
{
	uint32_t exec = 0, name = 0, cli = 0;
	uint8_t type;
	if (!address && (!memory.u32(4, exec) || !exec || exec > 0xfffffe00 ||
		!memory.u32(exec + 276, address))) return false;
	if (!address || (address & 3) || address > 0xffffff00 ||
		!memory.read(address + 8, &type, 1) || type != 13 ||
		!memory.u32(address + 10, name) || !memory.string(name, out.name, false) ||
		!memory.bptr(address + 172, cli)) return false;
	out.address = address;
	out.command.clear();
	if (cli) {
		if (cli > 0xffffff00 || !memory.bptr(cli + 16, name) ||
			!memory.string(name, out.command, true) ||
			!memory.bptr(cli + 60, out.segments)) return false;
	} else {
		uint32_t table;
		if (!memory.bptr(address + 128, table) || !table || table > 0xfffffff0 ||
			!memory.bptr(table + 12, out.segments)) return false;
	}
	return out.segments >= 4 && out.segments <= 0xfffffff8;
}

struct AmigaSegment {
	uint32_t link, address, size;
};

// LoadSeg headers contain the allocation size, followed by the next BPTR.
// Bound both traversal and guest reads: corrupt lists must never hang a stop.
inline bool read_segments(GuestReader& memory, uint32_t head,
	std::vector<AmigaSegment>& segments)
{
	std::vector<AmigaSegment> found;
	while (head) {
		if ((head & 3) || head < 4 || head > 0xfffffff8 || found.size() == 256) return false;
		for (const auto& segment : found) if (segment.link == head) return false;
		uint32_t size, next;
		if (!memory.u32(head - 4, size) || size < 8 || (size & 3) ||
			uint64_t(head) + size - 4 > (uint64_t(1) << 32) ||
			!memory.bptr(head, next)) return false;
		// Inspect only metadata and the bounds, never allocate guest-sized data.
		uint8_t byte;
		if (size > 8 && (!memory.read(head + 4, &byte, 1) ||
			!memory.read(head + size - 5, &byte, 1))) return false;
		found.push_back({head, head + 4, size - 8});
		head = next;
	}
	if (found.empty()) return false;
	segments.swap(found);
	return true;
}

inline bool process_name_matches(const std::string& actual, const std::string& wanted)
{
	size_t start = wanted.find_first_of(":/") == std::string::npos ? actual.find_last_of(":/") : std::string::npos;
	start = start == std::string::npos ? 0 : start + 1;
	if (actual.size() - start != wanted.size()) return false;
	for (size_t i = 0; i < wanted.size(); ++i) {
		auto fold = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
		if (fold(actual[start + i]) != fold(wanted[i])) return false;
	}
	return true;
}
}
#endif
