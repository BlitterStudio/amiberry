/* Bounded, remote-owned guest input. No host window or device dependency. */
#ifndef WINUAE_GDB_INPUT_H
#define WINUAE_GDB_INPUT_H
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace winuae_gdb {
struct InputEvent {
	uint32_t kind, port, code, value;
	bool valid() const {
		if (kind == 0) return port == 0 && code < 0x68 && value <= 1;
		if (kind == 1) return port < 2 && code < 2 &&
			(value <= 127 || value >= 0xffffff81u);
		return kind == 2 && port < 2 && code < 7 && value <= 1;
	}
};
struct InputStep { uint32_t delay; InputEvent event; };
class InputSink {
public:
	virtual ~InputSink() = default;
	virtual void input(const InputEvent&) = 0;
};
class GuestInput {
	InputSink& sink;
	std::array<bool, 104> keys{};
	std::array<std::array<bool, 7>, 2> ports{};
	std::vector<InputStep> steps;
	size_t next = 0;
	uint32_t elapsed = 0, due = 0;
	uint64_t generation = 0;
	std::string state = "idle";
	bool held() const {
		for (bool key : keys) if (key) return true;
		for (const auto& port : ports) for (bool control : port) if (control) return true;
		return false;
	}
	void apply(const InputEvent& e) {
		if (e.kind == 3) return; // Frame delay without input.
		if (e.kind != 1) {
			bool& held = e.kind == 0 ? keys[e.code] : ports[e.port][e.code];
			if (held == (e.value != 0)) return;
			held = e.value != 0;
		}
		sink.input(e);
	}
public:
	bool active() const { return state == "running"; }
	bool start(const std::vector<InputStep>& sequence) {
		if (active() || held() || sequence.empty() || sequence.size() > 256) return false;
		uint64_t total = 0;
		for (const auto& step : sequence) {
			const auto& e = step.event;
			if (!e.valid() && !(e.kind == 3 && !e.port && !e.code && !e.value)) return false;
			total += step.delay;
			if (total > 3600) return false;
		}
		steps = sequence; next = 0; elapsed = 0; due = steps[0].delay;
		++generation; state = "running";
		return true;
	}
	void frame() {
		if (!active()) return;
		while (next < steps.size() && elapsed >= due) {
			apply(steps[next++].event);
			if (next < steps.size()) due += steps[next].delay;
		}
		if (next == steps.size()) { release(); state = "completed"; }
		else ++elapsed;
	}
	void cancel() {
		if (active()) state = "cancelled";
		release();
	}
	explicit GuestInput(InputSink& sink) : sink(sink) {}
	bool event(const InputEvent& e) {
		if (active() || !e.valid()) return false;
		apply(e);
		return true;
	}
	void release() {
		for (unsigned i = 0; i < keys.size(); ++i) if (keys[i]) apply({0, 0, i, 0});
		for (unsigned p = 0; p < ports.size(); ++p)
			for (unsigned i = 0; i < ports[p].size(); ++i) if (ports[p][i]) apply({2, p, i, 0});
	}
	std::string status() const {
		std::string out = "{\"held\":[";
		bool first = true;
		auto add = [&](unsigned kind, unsigned port, unsigned code) {
			if (!first) out += ",";
			first = false;
			out += "{\"kind\":" + std::to_string(kind) + ",\"port\":" + std::to_string(port) +
				",\"code\":" + std::to_string(code) + "}";
		};
		for (unsigned i = 0; i < keys.size(); ++i) if (keys[i]) add(0, 0, i);
		for (unsigned p = 0; p < ports.size(); ++p)
			for (unsigned i = 0; i < ports[p].size(); ++i) if (ports[p][i]) add(2, p, i);
		return out + "],\"sequence\":{\"id\":" + std::to_string(generation) +
			",\"state\":\"" + state + "\",\"elapsed_frames\":" + std::to_string(elapsed) +
			",\"next\":" + std::to_string(next) + ",\"total\":" + std::to_string(steps.size()) + "}}";
	}
};
}
#endif
