/* WinUAE monitor commands for remote clients, reached through GDB qRcmd. */
#include "sysconfig.h"
#include "sysdeps.h"

#if defined(DEBUGGER) && defined(GDBSERVER)
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "events.h"
#include "debug.h"
#include "disasm.h"
#include "inputdevice.h"
#include "inputrecord.h"
#include "savestate.h"
#include "xwin.h"
#include "drawing.h"
#include "zfile.h"
#include "fsdb.h"
#include "gdb_server.h"
#include "gdb_target.h"
#include "gdb_amiga.h"
#include "gdb_input.h"
#include "gdb_output.h"
#include "gdb_watch.h"
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

class GdbMonitor : public GdbTarget, private winuae_gdb::GuestReader, private winuae_gdb::InputSink {
public:
	winuae_gdb::GuestOutput guest_output;
	bool disassemble(uint32_t address, unsigned count, std::string& output) override;
	bool screenshot(const std::string& path) override;
	bool monitor_running(const std::string& command) override;
	MonitorResult monitor(const std::string& command, std::string& result) override;
	bool check_stop(uaecptr pc, char* reason, size_t size) override;
	void stopped(char* reason, size_t size) override;
	void finish_checkpoint();
	void exception(int nr) override;
	void watch_hit(int slot, uaecptr addr, int size, int rwi, uae_u32 source, uae_u32 reg,
		uae_u32 value, uae_u32 old_value, bool old_valid) override;
	void detach() override;
	void input_frame();
private:
	winuae_gdb::GuestInput guest_input { *this };
	winuae_gdb::WatchLog watch_log;
	std::string process_name;
	uaecptr process_address = 0;
	bool process_armed = false;
	bool remote_history = false;
	bool conditional[BREAKPOINT_TOTAL] {};
	uae_u64 exception_mask = 0;
	int exception_vector = -1;
	bool exception_pending = false;
	bool checkpoint_saving = false;
	uaecptr exception_instruction = 0;
	winuae_gdb::Registers exception_registers {};
	std::string exception_details;

	bool trace_checks() override { return process_armed || remote_history; }
	bool read(uint32_t address, uint8_t* bytes, size_t length) override;
	void input(const winuae_gdb::InputEvent& e) override;
	bool disassemble_line(uaecptr pc, TCHAR* line, int size, uaecptr& next);
	MonitorResult status_command(const std::string& command, std::string& result);
	MonitorResult process_command(const std::string& command, std::string& result);
	MonitorResult exception_command(const std::string& command, std::string& result);
	MonitorResult watch_command(const std::string& command, std::string& result);
	MonitorResult output_command(const std::string& command, std::string& result);
	MonitorResult checkpoint_command(const std::string& command, std::string& result);
	MonitorResult task_command(const std::string& command, std::string& result);
	MonitorResult input_command(const std::string& command, std::string& result);
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
	draw_denise_line_queue_flush();
	TCHAR* filename = utf8u(path.c_str());
	if (!filename)
		return false;
	bool ok = screenshot_save(0, filename);
	xfree(filename);
	return ok;
}

bool GdbMonitor::monitor_running(const std::string& command)
{
	return command == "capabilities" || command == "execution-status" || command.compare(0, 6, "input ") == 0;
}

MonitorResult GdbMonitor::monitor(const std::string& command, std::string& result)
{
	using Handler = MonitorResult (GdbMonitor::*)(const std::string&, std::string&);
	static const Handler handlers[] = {
		&GdbMonitor::status_command,
		&GdbMonitor::process_command,
		&GdbMonitor::exception_command,
		&GdbMonitor::watch_command,
		&GdbMonitor::output_command,
		&GdbMonitor::checkpoint_command,
		&GdbMonitor::task_command,
		&GdbMonitor::input_command,
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
		commands += ",\"exception\",\"exception-mask\"";
		if (!mmu_enabled)
			commands += ",\"dma-watch\",\"watch\",\"watch-range\"";
		commands += ",\"guest-output\"";
#ifdef SAVESTATE
		commands += ",\"checkpoint\"";
#endif
		commands += ",\"tasks\",\"history\",\"condition\",\"step-over\"";
		commands += ",\"input\",\"input-sequence\"";
		result = "{\"protocol\":1,\"cpu_model\":" + std::to_string(currprefs.cpu_model) +
			",\"mmu_model\":" + std::to_string(currprefs.mmu_model) +
			",\"memory_addressing\":\"physical\",\"commands\":[" + commands +
			"],\"execution\":[\"continue\",\"step\",\"range-step\",\"step-over\"],\"exception_details\":true}";
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
			const char* type = bank->flags & (ABFLAG_ROM | ABFLAG_ROMIN) ? "rom" : bank->flags & ABFLAG_RAM ? "ram" : "io";
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

MonitorResult GdbMonitor::exception_command(const std::string& command, std::string& result)
{
	if (command == "exception") {
		char mask[17];
		snprintf(mask, sizeof(mask), "%016llx", static_cast<unsigned long long>(exception_mask));
		result = "{\"mask\":\"" + std::string(mask) + "\",\"last\":";
		if (exception_vector < 0) {
			result += "null";
		} else {
			result += "{\"vector\":" + std::to_string(exception_vector) +
				",\"instruction_pc\":" + std::to_string(exception_instruction) + ",\"registers\":[";
			for (size_t i = 0; i < exception_registers.size(); i++) {
				if (i)
					result += ",";
				result += std::to_string(exception_registers[i]);
			}
			result += "]" + exception_details + "}";
		}
		result += "}";
		return MonitorResult::ok;
	}
	if (command.compare(0, 15, "exception-mask ") == 0) {
		auto text = command.substr(15);
		if (text.empty() || text.size() > 16 || text.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
			return MonitorResult::error;
		uae_u64 mask = strtoull(text.c_str(), NULL, 16);
		if (mask & 3)
			return MonitorResult::error;
		exception_mask = mask;
		return MonitorResult::ok;
	}
	return MonitorResult::unsupported;
}

MonitorResult GdbMonitor::watch_command(const std::string& command, std::string& result)
{
	if (command == "watch events" || command == "watch last" || command == "watch clear-events") {
		if (command == "watch clear-events")
			watch_log.clear();
		result = watch_log.snapshot(command == "watch last");
		return MonitorResult::ok;
	}
	if (command == "watch list" || command == "dma-watch list") {
		bool dma = command[0] == 'd';
		result = "[";
		for (int i = 0; i < MEMWATCH_TOTAL; i++) {
			if (watch_owner[i] != (dma ? watch_dma : watch_predicate))
				continue;
			const auto& wp = mwnodes[i];
			if (result.size() > 1)
				result += ",";
			if (dma) {
				result += "{\"id\":" + std::to_string(i) + ",\"address\":" + std::to_string(wp.addr) +
					",\"length\":" + std::to_string(wp.size) + ",\"mode\":" + std::to_string(wp.rwi) +
					",\"mask\":" + std::to_string(wp.access_mask) + "}";
			} else {
				result += "{\"id\":" + std::to_string(i) + ",\"address\":" + std::to_string(wp.addr) +
					",\"size\":" + std::to_string(wp.size) + ",\"access\":" + std::to_string(wp.rwi) +
					",\"sources\":" + std::to_string(wp.access_mask) + ",\"value\":" +
					(wp.val_enabled ? std::to_string(wp.val) : "null") + ",\"mask\":" + std::to_string(wp.val_mask) +
					",\"change_only\":" + (wp.mustchange ? "true" : "false") +
					",\"log_only\":" + (wp.nobreak ? "true" : "false") +
					",\"range\":" + ((wp.remote & MW_REMOTE_EXACT) ? "false" : "true") + "}";
			}
		}
		result += "]";
		return MonitorResult::ok;
	}
	if (command.compare(0, 13, "watch remove ") == 0 || command.compare(0, 17, "dma-watch remove ") == 0) {
		bool dma = command[0] == 'd';
		uint32_t id;
		if (!hex_words(command.substr(dma ? 17 : 13), 1, &id) || id >= MEMWATCH_TOTAL ||
			watch_owner[id] != (dma ? watch_dma : watch_predicate))
			return MonitorResult::error;
		mwnodes[id] = {};
		watch_owner[id] = watch_none;
		memwatch_setup();
		return MonitorResult::ok;
	}
	bool range = command.compare(0, 12, "watch range ") == 0;
	if (range || command.compare(0, 10, "watch add ") == 0) {
		// address, access width or range length, access kind, source mask,
		// value enabled, value, value mask, change-only, log-only.
		uint32_t a[9];
		if (mmu_enabled || !hex_words(command.substr(range ? 12 : 10), 9, a) ||
			uint64_t(a[0]) + a[1] > watch_limit() || !a[3] || (a[3] & ~static_cast<uint32_t>(MW_MASK_ALL)) ||
			a[4] > 1 || a[7] > 1 || a[8] > 1 || (a[7] && a[2] != 2))
			return MonitorResult::error;
		if (range) {
			// Any access overlapping the range matches, as with console
			// watchpoints. Instruction fetches (4) need the CPU fetch source.
			// memwatch takes the value width from the watch size.
			if (!a[1] || a[1] > 0x10000 || a[2] < 1 || a[2] > 7 ||
				!(a[2] & 4) != !(a[3] & MW_MASK_CPU_I) || ((a[2] & 3) && !(a[3] & ~MW_MASK_CPU_I)) ||
				(a[4] && a[1] != 1 && a[1] != 2 && a[1] != 4))
				return MonitorResult::error;
		} else if ((a[1] != 1 && a[1] != 2 && a[1] != 4) || (a[0] & (a[1] - 1)) ||
			a[2] < 1 || a[2] > 3 || (a[3] & MW_MASK_CPU_I)) {
			return MonitorResult::error;
		}
		// Old values must be readable RAM, never device register reads.
		if (a[7]) {
			addrbank& bank = get_mem_bank(a[0]);
			if (!(bank.flags & ABFLAG_RAM) || !bank.check(a[0], a[1]))
				return MonitorResult::error;
		}
		int slot = free_watch_slot();
		if (slot < 0 || !start_memwatch())
			return MonitorResult::error;
		auto& wp = mwnodes[slot];
		wp = {};
		wp.addr = a[0];
		wp.size = a[1];
		wp.rwi = a[2];
		wp.access_mask = a[3];
		wp.val_enabled = a[4];
		wp.val = a[5];
		wp.val_mask = a[6];
		wp.mustchange = a[7];
		wp.nobreak = a[8];
		wp.reg = wp.pc = 0xffffffff;
		wp.remote = range ? MW_REMOTE_LOG : MW_REMOTE_LOG | MW_REMOTE_EXACT;
		watch_owner[slot] = watch_predicate;
		memwatch_setup();
		result = "{\"id\":" + std::to_string(slot) + "}";
		return MonitorResult::ok;
	}
	if (command.compare(0, 14, "dma-watch add ") == 0) {
		uint32_t args[4];
		if (!hex_words(command.substr(14), 4, args) || !args[1] || args[1] > 0x10000 ||
			uint64_t(args[0]) + args[1] > watch_limit() || args[2] < 1 || args[2] > 3 ||
			!args[3] || (args[3] & ~static_cast<uint32_t>(MW_MASK_ALL & ~7)) || mmu_enabled)
			return MonitorResult::error;
		for (int i = 0; i < MEMWATCH_TOTAL; i++) {
			const auto& wp = mwnodes[i];
			if (watch_owner[i] == watch_dma && wp.addr == args[0] && wp.size == args[1] &&
				wp.rwi == args[2] && wp.access_mask == args[3]) {
				result = std::to_string(i);
				return MonitorResult::ok;
			}
		}
		int slot = free_watch_slot();
		if (slot < 0 || !start_memwatch())
			return MonitorResult::error;
		auto& wp = mwnodes[slot];
		wp = {};
		wp.addr = args[0];
		wp.size = args[1];
		wp.rwi = args[2];
		wp.access_mask = args[3];
		wp.reg = wp.pc = wp.val_mask = 0xffffffff;
		wp.remote = MW_REMOTE_LOG;
		watch_owner[slot] = watch_dma;
		memwatch_setup();
		result = std::to_string(slot);
		return MonitorResult::ok;
	}
	return MonitorResult::unsupported;
}

MonitorResult GdbMonitor::output_command(const std::string& command, std::string& result)
{
	if (command.compare(0, 13, "guest-output ") != 0)
		return MonitorResult::unsupported;
	auto action = command.substr(13);
	if (action == "on")
		guest_output.enable(true);
	else if (action == "off")
		guest_output.enable(false);
	else if (action == "clear")
		guest_output.clear();
	else if (action != "read")
		return MonitorResult::error;
	result = guest_output.snapshot();
	return MonitorResult::ok;
}

MonitorResult GdbMonitor::checkpoint_command(const std::string& command, std::string&)
{
	bool restore = command.compare(0, 19, "checkpoint restore ") == 0;
	if (!restore && command.compare(0, 16, "checkpoint save ") != 0)
		return MonitorResult::unsupported;
#ifdef SAVESTATE
	auto path = command.substr(restore ? 19 : 16);
	if (path.empty() || savestate_state || quit_program || input_record || input_play ||
		is_savestate_incompatible() || !save_filesys_cando())
		return MonitorResult::error;
	TCHAR* filename = utf8u(path.c_str());
	if (!filename)
		return MonitorResult::error;
	if (_tcslen(filename) >= MAX_DPATH) {
		xfree(filename);
		return MonitorResult::error;
	}
	if (restore) {
		// Reject missing files and obvious non-state input before reset.
		struct zfile* file = zfile_fopen(filename, _T("rb"), 0);
		char header[4];
		bool valid = file && zfile_fread(header, 1, 4, file) == 4 && !memcmp(header, "ASF ", 4);
		if (file)
			zfile_fclose(file);
		if (!valid) {
			xfree(filename);
			return MonitorResult::error;
		}
		savestate_initsave(filename, 1, true, false);
		xfree(filename);
		// Remote ownership cannot survive memory remapping. Clear it
		// before loading saved console watches, while keeping transport.
		detach();
		debug_remote_hold();
		gdb_server_begin_restore();
		savestate_state = STATE_DORESTORE;
		uae_reset(1, 0);
		return MonitorResult::pending;
	}
	// A save that cannot open its file would be retried every frame.
	bool existed = zfile_exists(filename) != 0;
	struct zfile* file = zfile_fopen(filename, _T("ab"), 0);
	if (file)
		zfile_fclose(file);
	if (file && !existed)
		my_unlink(filename);
	if (!file) {
		xfree(filename);
		return MonitorResult::error;
	}
	// State files are only valid at the start of a frame. Run there and
	// stop right after the save; an earlier stop cancels it.
	savestate_initsave(filename, 1, true, false);
	xfree(filename);
	savestate_last_save_result = -1;
	checkpoint_saving = true;
	debug_remote_frame_save(true);
	resume(false);
	return MonitorResult::pending;
#else
	return MonitorResult::unsupported;
#endif
}

MonitorResult GdbMonitor::task_command(const std::string& command, std::string& result)
{
	if (command == "tasks") {
		std::vector<winuae_gdb::AmigaTask> tasks;
		if (!winuae_gdb::read_tasks(*this, tasks))
			return MonitorResult::error;
		const std::string suffix = "],\"truncated\":false}";
		result = "{\"tasks\":[";
		bool truncated = false;
		for (const auto& task : tasks) {
			std::string entry = "{\"address\":" + std::to_string(task.address) + ",\"name\":" + json_string(task.name) +
				",\"list\":" + json_string(task.list) + ",\"type\":" + std::to_string(task.type) +
				",\"state\":" + std::to_string(task.state) + ",\"saved_sp\":" + std::to_string(task.saved_sp) +
				",\"stack_lower\":" + std::to_string(task.stack_lower) + ",\"stack_upper\":" + std::to_string(task.stack_upper) +
				",\"signals_wait\":" + std::to_string(task.signals_wait) + "}";
			const size_t separator = result.back() == '[' ? 0 : 1;
			if (result.size() + separator + entry.size() + suffix.size() > winuae_gdb::Session::packet_size / 2) {
				truncated = true;
				break;
			}
			if (separator)
				result += ",";
			result += entry;
		}
		result += "],\"truncated\":" + std::string(truncated ? "true" : "false") + "}";
		return MonitorResult::ok;
	}
	if (command == "history on" || command == "history off") {
		remote_history = command == "history on";
		return MonitorResult::ok;
	}
	if (command.compare(0, 13, "history read ") == 0) {
		uint32_t count;
		if (!hex_words(command.substr(13), 1, &count) || !count || count > 128)
			return MonitorResult::error;
		int size = debug_history_size();
		const std::string suffix = "],\"truncated\":false}";
		result = "{\"recording\":" + std::string(remote_history ? "true" : "false") + ",\"entries\":[";
		// Collect from the newest entry back so a truncated reply keeps the
		// most recent instructions, then emit them oldest first.
		std::vector<std::string> entries;
		size_t length = result.size() + suffix.size();
		bool truncated = false;
		for (int i = size - 1; i >= (std::max)(0, size - static_cast<int>(count)); i--) {
			int frame, entry_vpos, entry_hpos;
			const struct regstruct* r = debug_history_get(i, &frame, &entry_vpos, &entry_hpos);
			std::string entry = "{\"pc\":" + std::to_string(r->pc) + ",\"frame\":" + std::to_string(frame) +
				",\"vpos\":" + std::to_string(entry_vpos) + ",\"hpos\":" + std::to_string(entry_hpos) + ",\"registers\":[";
			for (int j = 0; j < 16; j++) {
				if (j)
					entry += ",";
				entry += std::to_string(r->regs[j]);
			}
			entry += "]}";
			const size_t separator = entries.empty() ? 0 : 1;
			if (length + separator + entry.size() > winuae_gdb::Session::packet_size / 2) {
				truncated = true;
				break;
			}
			length += separator + entry.size();
			entries.push_back(std::move(entry));
		}
		for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
			if (it != entries.rbegin())
				result += ",";
			result += *it;
		}
		result += "],\"truncated\":" + std::string(truncated ? "true" : "false") + "}";
		return MonitorResult::ok;
	}
	if (command == "condition list") {
		result = "[";
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (!conditional[i])
				continue;
			const auto& bp = bpnodes[i];
			if (result.size() > 1)
				result += ",";
			result += "{\"id\":" + std::to_string(i) + ",\"register\":" + std::to_string(bp.type) +
				",\"operator\":" + std::to_string(bp.oper) + ",\"value\":" + std::to_string(bp.value1) +
				",\"mask\":" + std::to_string(bp.mask) + ",\"signed\":" + (bp.opersigned ? "true" : "false") + "}";
		}
		result += "]";
		return MonitorResult::ok;
	}
	if (command.compare(0, 17, "condition remove ") == 0) {
		uint32_t id;
		if (!hex_words(command.substr(17), 1, &id) || id >= BREAKPOINT_TOTAL || !conditional[id])
			return MonitorResult::error;
		bpnodes[id] = {};
		conditional[id] = false;
		return MonitorResult::ok;
	}
	if (command.compare(0, 14, "condition add ") == 0) {
		uint32_t args[5];
		if (!hex_words(command.substr(14), 5, args) || args[0] > BREAKPOINT_REG_SR || args[0] == BREAKPOINT_REG_PC ||
			args[1] > BREAKPOINT_CMP_LARGER || args[4] > 1)
			return MonitorResult::error;
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (bpnodes[i].enabled)
				continue;
			auto& bp = bpnodes[i];
			bp = {};
			bp.type = args[0];
			bp.oper = args[1];
			bp.value1 = args[2];
			bp.mask = args[3];
			bp.opersigned = args[4] != 0;
			bp.enabled = 1;
			bp.chain = -1;
			conditional[i] = true;
			result = "{\"id\":" + std::to_string(i) + "}";
			return MonitorResult::ok;
		}
		return MonitorResult::error;
	}
	if (command == "step-over") {
		TCHAR line[1024];
		uaecptr next;
		if (!disassemble_line(m68k_getpc(), line, 1024, next))
			return MonitorResult::error;
		range_active = false;
		debug_remote_run_to(next);
		return MonitorResult::running;
	}
	return MonitorResult::unsupported;
}

void GdbMonitor::input(const winuae_gdb::InputEvent& e)
{
	if (e.kind == 0) {
		inputdevice_do_keyboard(e.code, e.value);
		return;
	}
	static const int axes[2][2] = {
		{ INPUTEVENT_MOUSE1_HORIZ, INPUTEVENT_MOUSE1_VERT },
		{ INPUTEVENT_MOUSE2_HORIZ, INPUTEVENT_MOUSE2_VERT }
	};
	static const int controls[2][7] = {
		{ INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT, INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN,
		  INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON },
		{ INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT, INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN,
		  INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON }
	};
	// Relative mouse counts bypass host scaling and window coordinates.
	int value = e.value <= 127 ? static_cast<int>(e.value) : -static_cast<int>(0u - e.value);
	send_input_event(e.kind == 1 ? axes[e.port][e.code] : controls[e.port][e.code], value, 0, 0);
}

void GdbMonitor::input_frame()
{
	if (input_record || input_play)
		guest_input.cancel();
	else
		guest_input.frame();
}

MonitorResult GdbMonitor::input_command(const std::string& command, std::string& result)
{
	if (command == "input status" || command == "input release") {
		if (command == "input release")
			guest_input.cancel();
		result = guest_input.status();
		return MonitorResult::ok;
	}
	if (command.compare(0, 15, "input sequence ") == 0) {
		if (input_record || input_play)
			return MonitorResult::error;
		std::vector<winuae_gdb::InputStep> steps;
		size_t start = 15;
		while (start < command.size()) {
			auto end = command.find(';', start);
			if (end == std::string::npos)
				end = command.size();
			uint32_t args[5];
			if (steps.size() == 256 || !hex_words(command.substr(start, end - start), 5, args))
				return MonitorResult::error;
			steps.push_back({ args[0], { args[1], args[2], args[3], args[4] } });
			if (end + 1 == command.size())
				return MonitorResult::error;
			start = end + 1;
		}
		if (!guest_input.start(steps))
			return MonitorResult::error;
		result = guest_input.status();
		return MonitorResult::ok;
	}
	if (command.compare(0, 12, "input event ") == 0) {
		uint32_t args[4];
		if (input_record || input_play || !hex_words(command.substr(12), 4, args) ||
			!guest_input.event({ args[0], args[1], args[2], args[3] }))
			return MonitorResult::error;
		result = guest_input.status();
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

void GdbMonitor::exception(int nr)
{
	if (nr < 2 || nr > 63 || !(exception_mask & (static_cast<uae_u64>(1) << nr)) || exception_pending)
		return;
	// Capture before the CPU builds its exception frame, but enter the
	// remote event loop only at the next normal debugger boundary.
	exception_vector = nr;
	exception_instruction = regs.instruction_pc;
	exception_registers = registers();
	auto fault = cpu_get_debug_fault(nr);
	exception_details = ",\"cpu_model\":" + std::to_string(currprefs.cpu_model) +
		",\"mmu_model\":" + std::to_string(currprefs.mmu_model) +
		",\"vbr\":" + std::to_string(regs.vbr) +
		",\"usp\":" + std::to_string(regs.s ? regs.usp : m68k_areg(regs, 7)) +
		",\"isp\":" + std::to_string(regs.s && (!regs.m || currprefs.cpu_model < 68020) ? m68k_areg(regs, 7) : regs.isp) +
		",\"msp\":" + std::to_string(regs.s && regs.m && currprefs.cpu_model >= 68020 ? m68k_areg(regs, 7) : regs.msp) +
		",\"memory_fault\":";
	if (!fault.memory) {
		exception_details += "null";
	} else {
		static const char* access[] = { "unknown", "read", "write", "read-modify-write" };
		exception_details += "{\"address\":" + std::to_string(fault.address) +
			",\"access\":\"" + access[fault.access] + "\",\"size\":" +
			(fault.size ? std::to_string(fault.size) : "null") +
			",\"function_code\":" + std::to_string(fault.function_code) +
			",\"mmu_status\":" + (nr == 2 && currprefs.mmu_model ? std::to_string(fault.status) : "null") + "}";
	}
	exception_pending = true;
	interrupt();
}

void GdbMonitor::stopped(char* reason, size_t size)
{
	GdbTarget::stopped(reason, size);
	finish_checkpoint();
	if (!exception_pending)
		return;
	exception_pending = false;
	unsigned signal = exception_vector == 2 || exception_vector == 3 ? 10 :
		exception_vector == 4 || exception_vector == 10 || exception_vector == 11 ? 4 :
		exception_vector >= 5 && exception_vector <= 7 ? 8 : 5;
	snprintf(reason, size, "T%02xwinuae-exception:%02x;winuae-faultpc:%08x;11:%08x;",
		signal, exception_vector, exception_instruction, m68k_getpc());
}

void GdbMonitor::watch_hit(int slot, uaecptr addr, int size, int rwi, uae_u32 source, uae_u32 reg,
	uae_u32 value, uae_u32 old_value, bool old_valid)
{
	watch_log.capture({ 0, static_cast<uint32_t>(slot), addr, static_cast<uint32_t>(size),
		static_cast<uint32_t>(rwi), source, reg, regs.instruction_pc, value, old_value, old_valid });
}

void GdbMonitor::finish_checkpoint()
{
	if (!checkpoint_saving)
		return;
	checkpoint_saving = false;
	bool saved = !debug_remote_frame_save(false) && savestate_last_save_result > 0;
	if (savestate_state == STATE_SAVE)
		savestate_state = 0;
	savestate_last_save_result = -1;
	gdb_server_monitor_complete(saved);
}

void GdbMonitor::detach()
{
	finish_checkpoint();
	guest_input.cancel();
	watch_log.clear();
	guest_output.clear(true);
	remote_history = false;
	exception_mask = 0;
	exception_vector = -1;
	exception_pending = false;
	process_armed = false;
	process_name.clear();
	process_address = 0;
	for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (conditional[i])
			bpnodes[i] = {};
		conditional[i] = false;
	}
	GdbTarget::detach();
}

static GdbMonitor gdb_monitor;

GdbTarget& gdb_target()
{
	return gdb_monitor;
}

void gdb_server_input_frame()
{
	if (gdb_server_connected())
		gdb_monitor.input_frame();
}

void gdb_server_guest_output(const char* text, bool truncated)
{
	gdb_monitor.guest_output.capture(text, truncated);
}
#endif
