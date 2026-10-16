/* Bounded guest diagnostics, independent of the host log and emulated state. */
#ifndef WINUAE_GDB_OUTPUT_H
#define WINUAE_GDB_OUTPUT_H

#include "gdb_protocol.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace winuae_gdb {
class GuestOutput {
	struct Record { uint64_t id; std::string text; bool truncated; };
	std::atomic<bool> enabled{false};
	std::mutex mutex;
	std::vector<Record> records;
	size_t bytes = 0;
	uint64_t next = 1, dropped = 0;
public:
	void capture(const char* text, bool truncated = false)
	{
		if (!enabled.load(std::memory_order_relaxed)) return;
		std::lock_guard<std::mutex> lock(mutex);
		if (!enabled.load(std::memory_order_relaxed)) return;
		size_t length = 0;
		while (length < 1024 && text[length]) ++length;
		truncated = truncated || text[length];
		while (!records.empty() && (records.size() >= 64 || bytes + length > 4096)) {
			bytes -= records.front().text.size(); records.erase(records.begin()); ++dropped;
		}
		records.push_back({next++, std::string(text, length), truncated});
		bytes += length;
	}
	bool is_enabled() const { return enabled.load(std::memory_order_relaxed); }
	void enable(bool value)
	{
		std::lock_guard<std::mutex> lock(mutex);
		enabled.store(value, std::memory_order_relaxed);
	}
	void clear(bool disable = false)
	{
		std::lock_guard<std::mutex> lock(mutex);
		if (disable) enabled.store(false, std::memory_order_relaxed);
		records.clear(); bytes = 0; dropped = 0;
	}
	std::string snapshot()
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::string out = "{\"enabled\":" + std::string(enabled.load(std::memory_order_relaxed) ? "true" : "false") +
			",\"dropped\":" + std::to_string(dropped) + ",\"records\":[";
		bool first = true;
		for (const auto& record : records) {
			if (!first) out += ",";
			first = false;
			out += "{\"id\":" + std::to_string(record.id) + ",\"text\":" + json_string(record.text) +
				",\"truncated\":" + (record.truncated ? "true" : "false") + "}";
		}
		return out + "]}";
	}
};
}
#endif
