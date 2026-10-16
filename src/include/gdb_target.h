/* GDB target backed by the CPU debugger. Used only on the emulation thread. */
#ifndef WINUAE_GDB_TARGET_H
#define WINUAE_GDB_TARGET_H

#include "gdb_protocol.h"

// Remote entries share the debugger tables, but never take over console ones.
class GdbTarget : public winuae_gdb::Target {
public:
	winuae_gdb::Registers registers() override;
	bool write_registers(const winuae_gdb::Registers& values, uint32_t mask) override;
	bool read_memory(uint32_t address, size_t length, std::vector<uint8_t>& bytes) override;
	bool write_memory(uint32_t address, const std::vector<uint8_t>& bytes) override;
	bool breakpoint(bool insert, unsigned type, uint32_t address, uint32_t length) override;
	std::string memory_map() override;
	void interrupt() override;
	void resume(bool step) override;
	bool resume_range(uint32_t start, uint32_t end) override;
	void detach() override;
	// CPU debugger hooks.
	virtual bool check_stop(uaecptr pc, char* reason, size_t size);
	virtual void stopped(char* reason, size_t size);
	virtual void exception(int) {}
	virtual void watch_hit(int, uaecptr, int, int, uae_u32, uae_u32, uae_u32, uae_u32, bool) {}
protected:
	enum WatchOwner : uae_u8 { watch_none, watch_gdb, watch_dma, watch_predicate };
	bool breakpoints[BREAKPOINT_TOTAL] {};
	WatchOwner watch_owner[MEMWATCH_TOTAL] {};
	bool initialized_watchpoints = false;
	bool range_active = false;
	uaecptr range_start = 0, range_end = 0;
	// Extensions that inspect every instruction keep debugger checks active.
	virtual bool trace_checks() { return false; }
	bool start_memwatch();
	int free_watch_slot();
	static uint64_t watch_limit();
};

GdbTarget& gdb_target();

#endif
