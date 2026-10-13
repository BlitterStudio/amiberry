/* WinUAE GDB target: registers, physical memory and execution control. */
#include "sysconfig.h"
#include "sysdeps.h"

#ifdef DEBUGGER
#include "options.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "debug.h"
#include "savestate.h"
#include "gdb_server.h"
#include "gdb_target.h"

winuae_gdb::Registers GdbTarget::registers()
{
	MakeSR();
	winuae_gdb::Registers values;
	for (int i = 0; i < 16; i++)
		values[i] = regs.regs[i];
	values[16] = regs.sr;
	values[17] = m68k_getpc();
	return values;
}

bool GdbTarget::write_registers(const winuae_gdb::Registers& values, uint32_t mask)
{
	if ((mask & (1 << 16)) && values[16] > 0xffff)
		return false;
	if ((mask & (1 << 17)) && (values[17] & 1))
		return false;
	// MakeFromSR switches stack banks: apply it before writing the new A7.
	if (mask & (1 << 16)) {
		regs.sr = values[16];
		MakeFromSR();
	}
	for (int i = 0; i < 16; i++) {
		if (mask & (1 << i))
			regs.regs[i] = values[i];
	}
	if (mask & (1 << 17)) {
		m68k_setpc_normal(values[17]);
		fill_prefetch();
	}
	return true;
}

bool GdbTarget::read_memory(uint32_t address, size_t length, std::vector<uint8_t>& bytes)
{
	// Physical memory, independent of the console debugger's MMU mode.
	// Custom registers use the existing snapshot, never live I/O reads.
	uae_u8 snapshot[4 + 512 + 4];
	bool have_snapshot = false;
	size_t snapshot_size = 0;
	for (size_t i = 0; i < length; i++) {
		uaecptr addr = address + static_cast<uaecptr>(i);
		if (addr >= 0xdff000 && addr <= 0xdff1ff) {
			if (!have_snapshot) {
				save_custom(&snapshot_size, snapshot, 1);
				have_snapshot = true;
			}
			size_t offset = 4 + (addr & 511);
			if (offset >= snapshot_size)
				return false;
			bytes.push_back(snapshot[offset]);
		} else {
			addrbank& bank = get_mem_bank(addr);
			if (!(bank.flags & (ABFLAG_RAM | ABFLAG_ROM | ABFLAG_ROMIN)) || !bank.check(addr, 1))
				return false;
			bytes.push_back(static_cast<uint8_t>(bank.bget(addr)));
		}
	}
	return true;
}

bool GdbTarget::write_memory(uint32_t address, const std::vector<uint8_t>& bytes)
{
	// Validate the entire range before changing any guest state. Hardware
	// writes require complete aligned words; ROM writes must report failure.
	if (address >= 0xdff000 && uint64_t(address) + bytes.size() <= 0xdff200) {
		if ((address & 1) || (bytes.size() & 1))
			return false;
		for (size_t i = 0; i < bytes.size(); i += 2)
			debug_write_memory_16(address + static_cast<uaecptr>(i), (bytes[i] << 8) | bytes[i + 1]);
		return true;
	}
	for (size_t i = 0; i < bytes.size(); i++) {
		uaecptr addr = address + static_cast<uaecptr>(i);
		addrbank& bank = get_mem_bank(addr);
		if (!(bank.flags & ABFLAG_RAM) || !bank.check(addr, 1))
			return false;
	}
	for (size_t i = 0; i < bytes.size(); i++) {
		uaecptr addr = address + static_cast<uaecptr>(i);
		get_mem_bank(addr).bput(addr, bytes[i]);
	}
	if (!bytes.empty()) {
		flush_cpu_caches(true);
		flush_icache(3);
		fill_prefetch();
	}
	return true;
}

bool GdbTarget::start_memwatch()
{
	if (!memwatch_enabled) {
		initialize_memwatch(0);
		initialized_watchpoints = true;
	}
	return memwatch_enabled != 0;
}

int GdbTarget::free_watch_slot()
{
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		if (!mwnodes[i].size)
			return i;
	}
	return -1;
}

bool GdbTarget::breakpoint(bool insert, unsigned type, uint32_t address, uint32_t length)
{
	if (type <= 1) {
		if (length != 2 || (address & 1))
			return false;
		int free_slot = -1;
		for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
			if (breakpoints[i] && bpnodes[i].value1 == address) {
				if (!insert) {
					bpnodes[i] = {};
					breakpoints[i] = false;
				}
				return true;
			}
			if (!bpnodes[i].enabled)
				free_slot = i;
		}
		if (!insert)
			return true;
		if (free_slot < 0)
			return false;
		breakpoint_node& bp = bpnodes[free_slot];
		bp = {};
		bp.type = BREAKPOINT_REG_PC;
		bp.value1 = address;
		bp.enabled = 1;
		bp.mask = 0xffffffff;
		bp.chain = -1;
		breakpoints[free_slot] = true;
		return true;
	}
	// The existing watchpoint mapper uses signed bank boundaries. Keep
	// remote ranges within its supported address space and preserve MMU mode.
	if (type > 4 || mmu_enabled || length > 0x10000 || uint64_t(address) + length > 0x7fff0000)
		return false;
	int rwi = type == 2 ? 2 : type == 3 ? 1 : 3;
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		memwatch_node& wp = mwnodes[i];
		if (watch_owner[i] == watch_gdb && wp.addr == address && wp.size == length && wp.rwi == rwi) {
			if (!insert) {
				wp = {};
				watch_owner[i] = watch_none;
				memwatch_setup();
			}
			return true;
		}
	}
	if (!insert)
		return true;
	int slot = free_watch_slot();
	if (slot < 0 || !start_memwatch())
		return false;
	memwatch_node& wp = mwnodes[slot];
	wp = {};
	wp.addr = address;
	wp.size = length;
	wp.rwi = rwi;
	wp.access_mask = MW_MASK_CPU_D_R | MW_MASK_CPU_D_W;
	wp.reg = wp.pc = wp.val_mask = 0xffffffff;
	watch_owner[slot] = watch_gdb;
	memwatch_setup();
	return true;
}

std::string GdbTarget::memory_map()
{
	UaeMemoryMap map;
	uae_memory_map(&map);
	if (map.num_regions == UAE_MEMORY_REGIONS_MAX)
		return {};
	std::string xml = "<?xml version=\"1.0\"?><memory-map>";
	for (int i = 0; i < map.num_regions; i++) {
		const auto& r = map.regions[i];
		uaecptr address = r.start;
		addrbank* bank = &get_mem_bank(address);
		if (bank->sub_banks)
			bank = get_sub_bank(&address);
		const char* type = bank->flags & ABFLAG_ROM ? "rom" : bank->flags & ABFLAG_RAM ? "ram" : NULL;
		if (type && r.size)
			xml += "<memory type=\"" + std::string(type) + "\" start=\"" +
				std::to_string(r.start) + "\" length=\"" + std::to_string(r.size) + "\"/>";
	}
	return xml + "</memory-map>";
}

void GdbTarget::interrupt()
{
	range_active = false;
	debug_remote_interrupt();
}

void GdbTarget::resume(bool step)
{
	range_active = false;
	debug_remote_resume(step, false);
}

bool GdbTarget::resume_range(uint32_t start, uint32_t end)
{
	// Existing trace checks stop as soon as execution leaves the range.
	debug_remote_resume(false, true);
	range_start = start;
	range_end = end;
	range_active = true;
	return true;
}

bool GdbTarget::check_stop(uaecptr pc, char*, size_t)
{
	return range_active && (pc < range_start || pc >= range_end);
}

void GdbTarget::stopped(char*, size_t)
{
	range_active = false;
}

void GdbTarget::detach()
{
	for (int i = 0; i < BREAKPOINT_TOTAL; i++) {
		if (breakpoints[i])
			bpnodes[i] = {};
		breakpoints[i] = false;
	}
	bool changed = false, have_watchpoints = false;
	for (int i = 0; i < MEMWATCH_TOTAL; i++) {
		if (watch_owner[i]) {
			mwnodes[i] = {};
			changed = true;
		}
		watch_owner[i] = watch_none;
		if (mwnodes[i].size)
			have_watchpoints = true;
	}
	if (initialized_watchpoints && !have_watchpoints)
		deinitialize_memwatch();
	else if (changed && memwatch_enabled)
		memwatch_setup();
	initialized_watchpoints = false;
	resume(false);
}

winuae_gdb::Target& debug_gdb_target()
{
	return gdb_target();
}

GdbTarget& gdb_target()
{
	static GdbTarget target;
	return target;
}

bool gdb_target_check_stop(uaecptr pc, char* reason, size_t size)
{
	return gdb_target().check_stop(pc, reason, size);
}

void gdb_target_stopped(char* reason, size_t size)
{
	gdb_target().stopped(reason, size);
}
#endif
