/* Fixed-size remote watchpoint evidence; never writes host logs. */
#ifndef WINUAE_GDB_WATCH_H
#define WINUAE_GDB_WATCH_H
#include <array>
#include <cstdint>
#include <string>
namespace winuae_gdb {
struct WatchHit {
	uint64_t sequence;
	uint32_t slot, address, size, access, source, reg, pc, value, old_value;
	bool old_valid;
	std::string json() const {
		return "{\"sequence\":" + std::to_string(sequence) + ",\"id\":" + std::to_string(slot) +
			",\"address\":" + std::to_string(address) + ",\"size\":" + std::to_string(size) +
			",\"access\":" + std::to_string(access) + ",\"source\":" + std::to_string(source) +
			",\"register\":" + std::to_string(reg) + ",\"instruction_pc\":" + std::to_string(pc) +
			",\"value\":" + std::to_string(value) + ",\"old_value\":" +
			(old_valid ? std::to_string(old_value) : "null") + "}";
	}
};
class WatchLog {
	std::array<WatchHit, 64> entries{};
	size_t begin = 0, count = 0;
	uint64_t sequence = 0, dropped = 0;
public:
	void capture(WatchHit hit) {
		hit.sequence = ++sequence;
		if (count == entries.size()) { begin = (begin + 1) % entries.size(); --count; ++dropped; }
		entries[(begin + count++) % entries.size()] = hit;
	}
	void clear() { begin = count = 0; dropped = 0; }
	std::string snapshot(bool last = false) const {
		if (last) return count ? entries[(begin + count - 1) % entries.size()].json() : "null";
		std::string out = "{\"dropped\":" + std::to_string(dropped) + ",\"events\":[";
		for (size_t i = 0; i < count; ++i) {
			if (i) out += ",";
			out += entries[(begin + i) % entries.size()].json();
		}
		return out + "]}";
	}
};
}
#endif
