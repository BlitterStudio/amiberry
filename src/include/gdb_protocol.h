/* WinUAE GDB remote protocol. Independent of the emulator and host sockets. */
#ifndef WINUAE_GDB_PROTOCOL_H
#define WINUAE_GDB_PROTOCOL_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace winuae_gdb {

using Registers = std::array<uint32_t, 18>;

enum class MonitorResult { unsupported, ok, error };

// Quote and escape a string for monitor command JSON replies.
std::string json_string(const std::string&);

class Target {
public:
	virtual ~Target() = default;
	virtual Registers registers() = 0;
	// mask identifies the registers supplied by the client (SR precedes A7).
	virtual bool write_registers(const Registers&, uint32_t mask) = 0;
	virtual bool read_memory(uint32_t, size_t, std::vector<uint8_t>&) = 0;
	virtual bool write_memory(uint32_t, const std::vector<uint8_t>&) = 0;
	virtual bool breakpoint(bool insert, unsigned type, uint32_t, uint32_t) = 0;
	virtual std::string memory_map() { return {}; }
	virtual bool screenshot(const std::string&) { return false; }
	virtual bool disassemble(uint32_t, unsigned, std::string&) { return false; }
	virtual MonitorResult monitor(const std::string&, std::string&) { return MonitorResult::unsupported; }
	// Monitor commands that may run without stopping the CPU.
	virtual bool monitor_running(const std::string&) { return false; }
	virtual void interrupt() = 0;
	virtual void resume(bool step) = 0;
	virtual bool resume_range(uint32_t, uint32_t) { return false; }
	virtual void detach() = 0;
};

class Session {
public:
	static constexpr size_t packet_size = 65536;
	explicit Session(Target& target) : target(target) {}
	void receive(const char*, size_t);
	void stop(const std::string& reason = "S05");
	std::string take_output();
	bool stopped() const { return halted; }
	bool finished() const { return closing; }
private:
	Target& target;
	std::string input, output, last_reply, stop_reason = "S05";
	size_t checksum_offset = std::string::npos;
	bool no_ack = false, halted = false, await_stop = false, closing = false;
	void reply(const std::string&);
	void command(const std::string&);
};

}
#endif
