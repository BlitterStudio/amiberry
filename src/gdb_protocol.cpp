#include "include/gdb_protocol.h"

namespace winuae_gdb {
namespace {
const char hex[] = "0123456789abcdef";
int digit(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}
bool number(const std::string& s, size_t begin, size_t end, uint32_t& value)
{
	if (begin >= end || end > s.size() || end - begin > 8) return false;
	value = 0;
	for (size_t i = begin; i < end; ++i) {
		int d = digit(s[i]);
		if (d < 0) return false;
		value = (value << 4) | d;
	}
	return true;
}
std::string encode(uint32_t value)
{
	std::string s(8, '0');
	for (int i = 7; i >= 0; --i, value >>= 4) s[i] = hex[value & 15];
	return s;
}
bool range(uint32_t address, uint32_t length)
{
	return uint64_t(address) + length <= (uint64_t(1) << 32);
}
}

void Session::reply(const std::string& data)
{
	unsigned sum = 0;
	for (unsigned char c : data) sum += c;
	last_reply = "$" + data + "#" + hex[(sum >> 4) & 15] + hex[sum & 15];
	output += last_reply;
}

std::string Session::take_output()
{
	std::string result;
	result.swap(output);
	return result;
}

void Session::stop(const std::string& reason)
{
	halted = true;
	stop_reason = reason;
	if (await_stop) reply(stop_reason);
	await_stop = false;
}

void Session::receive(const char* data, size_t length)
{
	// Bound storage even for clients that never finish a frame or read replies.
	for (size_t i = 0; i < length && !closing; ++i) {
		char c = data[i];
		if (input.empty()) {
			if (c == '\003') {
				if (halted) reply(stop_reason);
				else { await_stop = true; target.interrupt(); }
			} else if (c == '-' && !no_ack) output += last_reply;
			else if (c == '$') { input += c; checksum_offset = std::string::npos; }
		} else {
			if (c == '$') { input.clear(); checksum_offset = std::string::npos; }
			input += c;
			if (c == '#' && checksum_offset == std::string::npos) checksum_offset = input.size() - 1;
			auto hash = checksum_offset;
			if (hash != std::string::npos && input.size() == hash + 3) {
				if (hash - 1 > packet_size) { closing = true; break; }
				unsigned sum = 0;
				for (size_t j = 1; j < hash; ++j) sum += static_cast<unsigned char>(input[j]);
				int hi = digit(input[hash + 1]), lo = digit(input[hash + 2]);
				if (hi >= 0 && lo >= 0 && (sum & 255) == unsigned(hi * 16 + lo)) {
					if (!no_ack) output += '+';
					std::string request = input.substr(1, hash - 1);
					input.clear();
					command(request);
				} else {
					if (!no_ack) output += '-';
					input.clear();
				}
			}
			if (input.size() > packet_size + 4) closing = true;
		}
		if (output.size() > packet_size * 4) closing = true;
	}
}

void Session::command(const std::string& s)
{
	if (s.empty()) { reply(""); return; }
	if (s == "qSupported" || s.compare(0, 11, "qSupported:") == 0) {
		reply("PacketSize=10000;QStartNoAckMode+;vContSupported+" + std::string(target.memory_map().empty() ? "" : ";qXfer:memory-map:read+")); return;
	}
	if (s.compare(0, 23, "qXfer:memory-map:read::") == 0) {
		auto comma = s.find(',', 23);
		uint32_t offset, length;
		if (comma == std::string::npos || !number(s, 23, comma, offset) ||
			!number(s, comma + 1, s.size(), length) || !length || length > packet_size - 1) {
			reply("E01"); return;
		}
		auto map = target.memory_map();
		if (map.empty()) { reply(""); return; }
		if (offset >= map.size()) { reply("l"); return; }
		auto part = map.substr(offset, length);
		reply(std::string(offset + part.size() < map.size() ? "m" : "l") + part);
		return;
	}

	if (s == "QStartNoAckMode") { reply("OK"); no_ack = true; return; }
	if (s == "qAttached") { reply("1"); return; }
	if (s == "qC") { reply("QC1"); return; }
	if (s == "qfThreadInfo") { reply("m1"); return; }
	if (s == "qsThreadInfo") { reply("l"); return; }
	if (s == "Hc-1" || s == "Hc0" || s == "Hc1" || s == "Hg0" || s == "Hg1" || s == "T1") {
		reply("OK"); return;
	}
	if (s == "vCont?") { reply("vCont;c;s;r"); return; }
	if (s == "?") {
		if (halted) reply(stop_reason);
		else { await_stop = true; target.interrupt(); }
		return;
	}
	if (s == "D") {
		reply("OK"); closing = true; return;
	}
	if (s.compare(0, 7, "vCont;r") == 0) {
		if (!halted) { reply("E16"); return; }
		auto comma = s.find(',', 7), colon = s.find(':', 7);
		auto end = colon == std::string::npos ? s.size() : colon;
		uint32_t start, limit;
		if (comma == std::string::npos || !number(s, 7, comma, start) ||
			!number(s, comma + 1, end, limit) || start > limit ||
			(colon != std::string::npos && s.substr(colon) != ":1" && s.substr(colon) != ":-1") ||
			!target.resume_range(start, limit)) { reply("E01"); return; }
		halted = false; await_stop = true; return;
	}
	bool step = s == "s" || s == "vCont;s" || s == "vCont;s:1";
	bool run = step || s == "c" || s == "vCont;c" || s == "vCont;c:1" || s == "vCont;c:-1";
	if (run) {
		if (!halted) { reply("E16"); return; }
		halted = false; await_stop = true; target.resume(step); return;
	}
	if (s[0] == 'g' || s[0] == 'G' || s[0] == 'p' || s[0] == 'P' ||
		s[0] == 'm' || s[0] == 'M' || s[0] == 'z' || s[0] == 'Z') {
		if (!halted) { reply("E16"); return; }
	} else { reply(""); return; }

	if (s == "g") {
		std::string data;
		for (uint32_t value : target.registers()) data += encode(value);
		reply(data); return;
	}
	if (s[0] == 'p' || s[0] == 'P') {
		auto eq = s.find('=');
		uint32_t reg, value;
		if (!number(s, 1, s[0] == 'p' ? s.size() : eq, reg) || reg >= 18) { reply("E01"); return; }
		if (s[0] == 'p') { reply(encode(target.registers()[reg])); return; }
		if (eq == std::string::npos || s.size() - eq != 9 || !number(s, eq + 1, s.size(), value)) {
			reply("E01"); return;
		}
		Registers registers{}; registers[reg] = value;
		reply(target.write_registers(registers, uint32_t(1) << reg) ? "OK" : "E01"); return;
	}
	if (s[0] == 'G') {
		Registers registers{};
		if (s.size() != 145) { reply("E01"); return; }
		for (size_t i = 0; i < registers.size(); ++i) {
			if (!number(s, 1 + i * 8, 9 + i * 8, registers[i])) { reply("E01"); return; }
		}
		reply(target.write_registers(registers, (1u << 18) - 1) ? "OK" : "E01"); return;
	}
	if (s[0] == 'm' || s[0] == 'M') {
		auto comma = s.find(','), colon = s.find(':');
		uint32_t address, length;
		if (comma == std::string::npos || !number(s, 1, comma, address) ||
			!number(s, comma + 1, s[0] == 'm' ? s.size() : colon, length) ||
			length > packet_size / 2 || !range(address, length)) { reply("E01"); return; }
		std::vector<uint8_t> bytes;
		if (s[0] == 'm') {
			if (!target.read_memory(address, length, bytes) || bytes.size() != length) { reply("E01"); return; }
			std::string data;
			for (uint8_t b : bytes) { data += hex[b >> 4]; data += hex[b & 15]; }
			reply(data); return;
		}
		if (colon == std::string::npos || s.size() - colon - 1 != size_t(length) * 2) { reply("E01"); return; }
		bytes.reserve(length);
		for (size_t i = colon + 1; i < s.size(); i += 2) {
			uint32_t value;
			if (!number(s, i, i + 2, value)) { reply("E01"); return; }
			bytes.push_back(static_cast<uint8_t>(value));
		}
		reply(target.write_memory(address, bytes) ? "OK" : "E01"); return;
	}
	if (s[0] == 'Z' || s[0] == 'z') {
		auto first = s.find(','), second = s.find(',', first == std::string::npos ? 0 : first + 1);
		uint32_t type, address, length;
		if (first == std::string::npos || second == std::string::npos ||
			!number(s, 1, first, type) || !number(s, first + 1, second, address) ||
			!number(s, second + 1, s.size(), length) || !length || !range(address, length)) {
			reply("E01"); return;
		}
		if (type > 4) { reply(""); return; }
		reply(target.breakpoint(s[0] == 'Z', type, address, length) ? "OK" : "E01"); return;
	}
	reply("");
}
}
