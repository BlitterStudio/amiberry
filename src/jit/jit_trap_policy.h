/*
 * Amiberry - JIT trap-block demotion policy
 *
 * Which trap-capable opcodes force a whole RAM block to be interpreted
 * instead of compiled.
 *
 * This lives in a header, separate from the JIT itself, so the policy can be
 * asserted by a unit test without building or running the emulator. The
 * decision is a pure function of the instruction mnemonic; the caller does
 * the table68k lookup.
 *
 * Structural / supervisor traps demote: mixing interpreted trap opcodes with
 * compiled flag readers inside one block can expose stale lazy-flag state,
 * and these opcodes mark exception, supervisor and CPU-tester code.
 *
 * Arithmetic traps (integer division, CHK) do NOT demote: they are hot in
 * ordinary user code, and the opcode runs via the per-opcode fallback, which
 * syncs the 68k PC before calling the interpreter handler. See #2299 / #2305.
 */

#ifndef AMIBERRY_JIT_TRAP_POLICY_H
#define AMIBERRY_JIT_TRAP_POLICY_H

#include "readcpu.h"

static inline bool jit_trap_demote_mnemo(const int mnemo)
{
	switch (mnemo) {
	case i_ILLG:
	case i_RTE:
	case i_STOP:
	case i_RESET:
	case i_MOVEC2:
	case i_MOVE2C:
	case i_MOVES:
	case i_MV2SR:
	case i_MVSR2:
	case i_MVR2USP:
	case i_MVUSP2R:
	case i_ANDSR:
	case i_ORSR:
	case i_EORSR:
	case i_TRAPcc:
	case i_FTRAPcc:
	case i_TRAPV:
	case i_BKPT:
	case i_LPSTOP:
	case i_MMUOP030:
	case i_PFLUSHN:
	case i_PFLUSH:
	case i_PFLUSHAN:
	case i_PFLUSHA:
	case i_PLPAR:
	case i_PLPAW:
	case i_PTESTR:
	case i_PTESTW:
	case i_CINVL:
	case i_CINVP:
	case i_CINVA:
	case i_CPUSHL:
	case i_CPUSHP:
	case i_CPUSHA:
		return true;
	default:
		return false;
	}
}

#endif /* AMIBERRY_JIT_TRAP_POLICY_H */
