/*
 * Asserts which trap-capable opcodes force whole-block interpreter demotion
 * in the ARM64 JIT.
 *
 * Why this test exists: a widening of this policy is invisible in correctness
 * terms - the emulator still produces right answers, just far more slowly,
 * because affected blocks stop being compiled. Commit 64a3a9fa demoted every
 * RAM block containing any fl_trap opcode and cost 28% of integer throughput
 * on a Cortex-A76; it was found months later by bisecting (#2315). #2305
 * narrowed the policy to the structural/supervisor set asserted below.
 *
 * The check is deterministic - it compares sets of opcode mnemonics and does
 * no timing at all - so it is stable on any CI runner regardless of load.
 */

/* readcpu.h is reached without sysconfig.h: uae/types.h only needs the integer
 * width macros, which the compiler already provides. This keeps the test free
 * of SDL and of the emulator's platform layer. */
#define SIZEOF_SHORT __SIZEOF_SHORT__
#define SIZEOF_INT __SIZEOF_INT__
#define SIZEOF_LONG __SIZEOF_LONG__

#define ENUMDECL typedef enum
#define ENUMNAME(name) name

#include "jit/jit_trap_policy.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Mnemonic {
	int value;
	const char* name;
};

/* Opcodes that MUST demote: structural, supervisor and CPU-tester shapes.
 * These are cold in ordinary user code, and compiling them alongside
 * interpreted trap opcodes exposes stale lazy-flag state. */
const Mnemonic kMustDemote[] = {
	{ i_ILLG, "ILLG" }, { i_RTE, "RTE" }, { i_STOP, "STOP" },
	{ i_RESET, "RESET" }, { i_MOVEC2, "MOVEC2" }, { i_MOVE2C, "MOVE2C" },
	{ i_MOVES, "MOVES" }, { i_MV2SR, "MV2SR" }, { i_MVSR2, "MVSR2" },
	{ i_MVR2USP, "MVR2USP" }, { i_MVUSP2R, "MVUSP2R" },
	{ i_ANDSR, "ANDSR" }, { i_ORSR, "ORSR" }, { i_EORSR, "EORSR" },
	{ i_TRAPcc, "TRAPcc" }, { i_FTRAPcc, "FTRAPcc" }, { i_TRAPV, "TRAPV" },
	{ i_BKPT, "BKPT" }, { i_LPSTOP, "LPSTOP" }, { i_MMUOP030, "MMUOP030" },
	{ i_PFLUSHN, "PFLUSHN" }, { i_PFLUSH, "PFLUSH" },
	{ i_PFLUSHAN, "PFLUSHAN" }, { i_PFLUSHA, "PFLUSHA" },
	{ i_PLPAR, "PLPAR" }, { i_PLPAW, "PLPAW" },
	{ i_PTESTR, "PTESTR" }, { i_PTESTW, "PTESTW" },
	{ i_CINVL, "CINVL" }, { i_CINVP, "CINVP" }, { i_CINVA, "CINVA" },
	{ i_CPUSHL, "CPUSHL" }, { i_CPUSHP, "CPUSHP" }, { i_CPUSHA, "CPUSHA" },
};

/* Opcodes that MUST NOT demote: hot in ordinary user code. These are the ones
 * #2305 released back to the JIT after #2299; a regression here is the
 * expensive direction. */
const Mnemonic kMustCompile[] = {
	{ i_DIVU, "DIVU" }, { i_DIVS, "DIVS" }, { i_DIVL, "DIVL" },
	{ i_CHK, "CHK" }, { i_CHK2, "CHK2" },
	{ i_MULU, "MULU" }, { i_MULS, "MULS" }, { i_MULL, "MULL" },
	{ i_TRAP, "TRAP" }, { i_LINK, "LINK" }, { i_UNLK, "UNLK" },
	{ i_JSR, "JSR" }, { i_RTS, "RTS" }, { i_MOVE, "MOVE" },
	{ i_ADD, "ADD" }, { i_SUB, "SUB" }, { i_CMP, "CMP" }, { i_CMPM, "CMPM" },
	{ i_MVMEL, "MVMEL" }, { i_MVMLE, "MVMLE" }, { i_TAS, "TAS" },
	{ i_CAS, "CAS" }, { i_CAS2, "CAS2" }, { i_NOP, "NOP" },
};

int failures;

void expect(const bool ok, const std::string& message)
{
	if (!ok) {
		std::cerr << "FAIL: " << message << '\n';
		failures++;
	}
}

} // namespace

int main()
{
	for (const auto& m : kMustDemote)
		expect(jit_trap_demote_mnemo(m.value),
			std::string("i_") + m.name + " must demote its block to the interpreter");

	for (const auto& m : kMustCompile)
		expect(!jit_trap_demote_mnemo(m.value),
			std::string("i_") + m.name + " must stay compiled (hot in user code)");

	/* Nothing outside the declared set may demote. Catches a widening that
	 * happens to miss every opcode named above. */
	std::vector<std::string> unexpected;
	for (int mnemo = 0; mnemo < MAX_OPCODE_FAMILY; ++mnemo) {
		if (!jit_trap_demote_mnemo(mnemo))
			continue;
		const bool declared = std::any_of(
			std::begin(kMustDemote), std::end(kMustDemote),
			[mnemo](const Mnemonic& m) { return m.value == mnemo; });
		if (!declared)
			unexpected.push_back(std::to_string(mnemo));
	}
	if (!unexpected.empty()) {
		std::string list;
		for (const auto& u : unexpected)
			list += (list.empty() ? "" : ", ") + u;
		expect(false, "undeclared mnemonics demote (opcode family index): " + list);
	}

	if (failures == 0)
		std::cout << "jit_trap_policy: OK (" << std::size(kMustDemote) << " demoting, "
			<< std::size(kMustCompile) << " must-compile, "
			<< MAX_OPCODE_FAMILY << " families scanned)\n";
	return failures == 0 ? 0 : 1;
}
