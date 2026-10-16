/* WinUAE monitor commands for remote clients, reached through GDB qRcmd. */
#include "sysconfig.h"
#include "sysdeps.h"

#if defined(DEBUGGER) && defined(GDBSERVER)
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "events.h"
#include "debug.h"
#include "disasm.h"
#include "inputdevice.h"
#include "xwin.h"
#include "gdb_server.h"
#include "gdb_target.h"
#include "gdb_amiga.h"
#include <algorithm>

using winuae_gdb::MonitorResult;
using winuae_gdb::json_string;

namespace {
bool hex_words(const std::string& text, unsigned count, uint32_t* words)
{
	size_t start = 0;
	for (unsigned i = 0; i < count; i++) {
		auto end = text.find(' ', start);
		if (end == std::string::npos)
			end = text.size();
		auto word = text.substr(start, end - start);
		if (word.empty() || word.size() > 8 || word.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
			return false;
		words[i] = static_cast<uint32_t>(strtoul(word.c_str(), NULL, 16));
		if (i + 1 == count)
			return end == text.size();
		if (end == text.size())
			return false;
		start = end + 1;
	}
	return false;
}

// Exec structures are longword aligned and never at address zero.
bool structure_address(const std::string& text, uaecptr& address)
{
	uint32_t value;
	if (!hex_words(text, 1, &value) || !value || (value & 3))
		return false;
	address = value;
	return true;
}
}

class GdbMonitor : public GdbTarget, private winuae_gdb::GuestReader {
public:
	bool disassemble(uint32_t address, unsigned count, std::string& output) override;
	bool screenshot(const std::string& path) override;
	bool monitor_running(const std::string& command) override;
	MonitorResult monitor(const std::string& command, std::string& result) override;
	bool check_stop(uaecptr pc, char* reason, size_t size) override;
	void detach() override;
private:
	std::string process_name;
	uaecptr process_address = 0;
	bool process_armed = false;

	bool trace_checks() override { return process_armed; }
	bool read(uint32_t address, uint8_t* bytes, size_t length) override;
	bool disassemble_line(uaecptr pc, TCHAR* line, int size, uaecptr& next);
	MonitorResult status_command(const std::string& command, std::string& result);
	MonitorResult process_command(const std::string& command, std::string& result);
};

bool GdbMonitor::read(uint32_t address, uint8_t* bytes, size_t length)
{
	if (uint64_t(address) + length > (uint64_t(1) << 32))
		return false;
	for (size_t i = 0; i < length; i++) {
		uaecptr addr = address + static_cast<uaecptr>(i);
		addrbank& bank = get_mem_bank(addr);
		// Exec task names may point at constant strings in Kickstart ROM.
		// Permit ordinary ROM reads, but never follow metadata into I/O.
		if (!(bank.flags & (ABFLAG_RAM | ABFLAG_ROM | ABFLAG_ROMIN)) || !bank.check(addr, 1))
			return false;
		bytes[i] = static_cast<uint8_t>(bank.bget(addr));
	}
	return true;
}

bool GdbMonitor::disassemble_line(uaecptr pc, TCHAR* line, int size, uaecptr& next)
{
	// Check instruction and extension words before calling the existing
	// debugger disassembler. Never disassemble hardware registers.
	if ((pc & 1) || pc > 0xffffffe0)
		return false;
	addrbank& bank = get_mem_bank(pc);
	if (!(bank.flags & (ABFLAG_RAM | ABFLAG_ROM | ABFLAG_ROMIN)) || !bank.check(pc, 32))
		return false;
	int flags = disasm_flags;
	disasm_flags &= ~(DISASM_FLAG_VAL | DISASM_FLAG_VAL_FORCE);
	next = pc;
	m68k_disasm_2(line, size, pc, NULL, 0, &next, 1, NULL, NULL, 0xffffffff, 1);
	disasm_flags = flags;
	return next > pc;
}

bool GdbMonitor::disassemble(uint32_t address, unsigned count, std::string& output)
{
	for (unsigned i = 0; i < count; i++) {
		TCHAR line[1024] = {};
		uaecptr next;
		if (!disassemble_line(address, line, 1024, next))
			return false;
		char* text = uutf8(line);
		if (!text)
			return false;
		output += text;
		xfree(text);
		if (output.size() > winuae_gdb::Session::packet_size / 2)
			return false;
		address = next;
	}
	return true;
}

bool GdbMonitor::screenshot(const std::string& path)
{
	TCHAR* filename = utf8u(path.c_str());
	if (!filename)
		return false;
	bool ok = screenshot_save(0, filename);
	xfree(filename);
	return ok;
}

bool GdbMonitor::monitor_running(const std::string& command)
{
	return command == "capabilities" || command == "execution-status";
}

MonitorResult GdbMonitor::monitor(const std::string& command, std::string& result)
{
	using Handler = MonitorResult (GdbMonitor::*)(const std::string&, std::string&);
	static const Handler handlers[] = {
		&GdbMonitor::status_command,
		&GdbMonitor::process_command,
	};
	for (auto handler : handlers) {
		auto status = (this->*handler)(command, result);
		if (status != MonitorResult::unsupported)
			return status;
	}
	return MonitorResult::unsupported;
}

MonitorResult GdbMonitor::status_command(const std::string& command, std::string& result)
{
	if (command == "capabilities") {
		std::string commands = "\"capabilities\",\"execution-status\",\"memory-map\",\"memory-check\",\"disasm\",\"screenshot\"";
		commands += ",\"process-break\",\"segments\"";
		result = "{\"protocol\":1,\"cpu_model\":" + std::to_string(currprefs.cpu_model) +
			",\"mmu_model\":" + std::to_string(currprefs.mmu_model) +
			",\"memory_addressing\":\"physical\",\"commands\":[" + commands +
			"],\"execution\":[\"continue\",\"step\",\"range-step\"]}";
		return MonitorResult::ok;
	}
	if (command == "execution-status") {
		bool stopped = gdb_server_halted();
		result = "{\"execution\":\"" + std::string(pause_emulation ? "host-paused" : stopped ? "stopped" : "running") +
			"\",\"debugger_stopped\":" + (stopped ? "true" : "false") +
			",\"host_pause\":" + std::to_string(pause_emulation) +
			",\"frame\":" + std::to_string(vsync_counter) + ",\"vpos\":" + std::to_string(vpos) +
			",\"hpos\":" + std::to_string(current_hpos()) + ",\"cycles\":\"" + std::to_string(get_cycles()) + "\"}";
		return MonitorResult::ok;
	}
	if (command == "memory-map") {
		UaeMemoryMap map;
		uae_memory_map(&map);
		result = "{\"addressing\":\"physical\",\"truncated\":" +
			std::string(map.num_regions == UAE_MEMORY_REGIONS_MAX ? "true" : "false") + ",\"regions\":[";
		for (int i = 0; i < map.num_regions; i++) {
			const auto& r = map.regions[i];
			uaecptr address = r.start;
			addrbank* bank = &get_mem_bank(address);
			if (bank->sub_banks)
				bank = get_sub_bank(&address);
			const char* type = bank->flags & ABFLAG_ROM ? "rom" : bank->flags & ABFLAG_RAM ? "ram" : "io";
			char* name = uutf8(r.name);
			if (i)
				result += ",";
			result += "{\"address\":" + std::to_string(r.start) + ",\"size\":" + std::to_string(r.size) +
				",\"type\":\"" + type + "\",\"chip\":" + (bank->flags & ABFLAG_CHIPRAM ? "true" : "false") +
				",\"name\":" + json_string(name ? name : "") + "}";
			xfree(name);
		}
		result += "]}";
		return MonitorResult::ok;
	}
	if (command.compare(0, 13, "memory-check ") == 0) {
		uint32_t args[2];
		if (!hex_words(command.substr(13), 2, args) || !args[1] || args[1] > 8 * 1024 * 1024 ||
			uint64_t(args[0]) + args[1] > (uint64_t(1) << 32))
			return MonitorResult::error;
		int kind = -1;
		for (uint64_t p = args[0], end = p + args[1]; p < end;) {
			uaecptr address = static_cast<uaecptr>(p);
			addrbank& bank = get_mem_bank(address);
			auto n = static_cast<uint32_t>((std::min)(end - p, uint64_t(0x10000 - (address & 0xffff))));
			// Match the physical RAM write path. Mixed and indirect banks
			// cannot be certified merely from their displayed map label.
			if (!(bank.flags & ABFLAG_RAM) || bank.flags & (ABFLAG_ROM | ABFLAG_IO) ||
				bank.sub_banks || !bank.check(address, n))
				return MonitorResult::error;
			int chip = (bank.flags & ABFLAG_CHIPRAM) != 0;
			if (kind >= 0 && chip != kind)
				return MonitorResult::error;
			kind = chip;
			p += n;
		}
		result = "{\"writable_ram\":true,\"memory\":\"" + std::string(kind ? "chip" : "fast") + "\"}";
		return MonitorResult::ok;
	}
	return MonitorResult::unsupported;
}

MonitorResult GdbMonitor::process_command(const std::string& command, std::string& result)
{
	if (command == "segments" || command.compare(0, 9, "segments ") == 0) {
		uaecptr address = 0;
		if (command.size() > 8 && !structure_address(command.substr(9), address))
			return MonitorResult::error;
		winuae_gdb::AmigaProcess process;
		std::vector<winuae_gdb::AmigaSegment> segments;
		if (!winuae_gdb::read_process(*this, address, process) ||
			!winuae_gdb::read_segments(*this, process.segments, segments))
			return MonitorResult::error;
		result = "{\"process\":" + std::to_string(process.address) + ",\"name\":" + json_string(process.name) +
			",\"command\":" + json_string(process.command) + ",\"segments\":[";
		for (size_t i = 0; i < segments.size(); i++) {
			if (i)
				result += ",";
			result += "{\"index\":" + std::to_string(i) + ",\"address\":" + std::to_string(segments[i].address) +
				",\"size\":" + std::to_string(segments[i].size) + "}";
		}
		result += "]}";
		return MonitorResult::ok;
	}
	if (command == "process-break clear") {
		process_armed = false;
		process_name.clear();
		process_address = 0;
		return MonitorResult::ok;
	}
	if (command == "process-break status") {
		result = "{\"armed\":" + std::string(process_armed ? "true" : "false") +
			",\"name\":" + json_string(process_name) + ",\"address\":" + std::to_string(process_address) + "}";
		return MonitorResult::ok;
	}
	if (command.compare(0, 19, "process-break name ") == 0) {
		auto name = command.substr(19);
		if (name.empty() || name.size() > 255)
			return MonitorResult::error;
		for (unsigned char c : name) {
			if (c < 32 || c >= 127)
				return MonitorResult::error;
		}
		process_name = name;
		process_address = 0;
		process_armed = true;
		return MonitorResult::ok;
	}
	if (command.compare(0, 22, "process-break address ") == 0) {
		uaecptr address;
		if (!structure_address(command.substr(22), address))
			return MonitorResult::error;
		process_address = address;
		process_name.clear();
		process_armed = true;
		return MonitorResult::ok;
	}
	return MonitorResult::unsupported;
}

bool GdbMonitor::check_stop(uaecptr pc, char* reason, size_t size)
{
	if (process_armed && !(get_mem_bank(pc).flags & ABFLAG_ROM)) {
		winuae_gdb::AmigaProcess process;
		if (winuae_gdb::read_process(*this, 0, process) && pc == process.segments + 4 &&
			(process_address ? process.address == process_address :
			winuae_gdb::process_name_matches(process.name, process_name) ||
			winuae_gdb::process_name_matches(process.command, process_name))) {
			// This is a one-shot entry breakpoint, not an Amiga task switch stop.
			process_armed = false;
			snprintf(reason, size, "T05winuae-entry:%08x;11:%08x;", process.address, pc);
			return true;
		}
	}
	return GdbTarget::check_stop(pc, reason, size);
}

void GdbMonitor::detach()
{
	process_armed = false;
	process_name.clear();
	process_address = 0;
	GdbTarget::detach();
}

static GdbMonitor gdb_monitor;

GdbTarget& gdb_target()
{
	return gdb_monitor;
}
#endif
