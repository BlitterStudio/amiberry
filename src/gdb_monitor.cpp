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
}

class GdbMonitor : public GdbTarget {
public:
	bool disassemble(uint32_t address, unsigned count, std::string& output) override;
	bool screenshot(const std::string& path) override;
	bool monitor_running(const std::string& command) override;
	MonitorResult monitor(const std::string& command, std::string& result) override;
private:
	bool disassemble_line(uaecptr pc, TCHAR* line, int size, uaecptr& next);
	MonitorResult status_command(const std::string& command, std::string& result);
};

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

static GdbMonitor gdb_monitor;

GdbTarget& gdb_target()
{
	return gdb_monitor;
}
#endif
