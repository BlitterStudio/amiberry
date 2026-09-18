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
 * Structural trap opcodes (illegal sentinels, RTE/STOP, system control moves,
 * MMU/cache control, TRAPcc) only appear in cold exception/supervisor/tester
 * code; interpreting those blocks is cheap and keeps mixed interpreted/
 * compiled flag and PC state maximally safe. The CPU tester drives exactly
 * this shape: every test block ends in an ILLEGAL sentinel, so those blocks
 * stay interpreted like 8.3.0.
 *
 * SR/USP moves do NOT demote: on AmigaOS all tasks run in supervisor mode, so
 * MV2SR/MVSR2/ANDSR/ORSR/EORSR are the implementation of exec's
 * Forbid()/Permit()/Disable()/Enable() and are hot wherever the OS (or a
 * RAM-resident OS like PiMIGA, or an inlined Disable() around a timing loop)
 * executes from RAM (#2315: demoting them cost 28% of SysInfo integer
 * throughput). The immediate forms compile natively (jff_*); the register
 * forms run through the per-opcode fallback, which syncs the 68k PC before
 * the interpreter handler call, like the arithmetic traps below.
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
