#!/bin/sh
set -eu

cd "$(dirname "$0")/.."

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/jit_trap_policy_test.$$"
trap 'rm -f "$out"' EXIT

# The set assertions below cover src/jit/jit_trap_policy.h; this guard
# checks that the emulator still takes its policy from that header. If
# jit_trap_demote_opcode in src/jit/arm/compemu_support_arm.cpp stops
# delegating (partial revert, conflict resolution), the set test stays
# green while the emulator re-widens demotion -- the #2315 regression
# class. Policy changes belong in jit_trap_policy.h, not in the .cpp.
# Both checks run on a comment-stripped copy of the body, so commented-
# out code cannot mask a missing delegation; the delegation must be the
# function's sole policy authority -- no mnemonic references may remain.
cpp=src/jit/arm/compemu_support_arm.cpp
body=$(sed -n '/^static bool jit_trap_demote_opcode/,/^}/p' "$cpp")
if [ -z "$body" ]; then
	echo "FAIL: jit_trap_demote_opcode not found in $cpp" >&2
	echo "      (moved or renamed? update this guard's extraction pattern)" >&2
	exit 1
fi
clean=$(printf '%s\n' "$body" | sed -e 's:/\*[^*]*\*/::g' -e 's://.*$::' -e 's:/\*.*$::' -e 's:^.*\*/::')
if ! printf '%s\n' "$clean" | grep -q 'jit_trap_demote_mnemo('; then
	echo "FAIL: $cpp: jit_trap_demote_opcode does not delegate to jit_trap_demote_mnemo()" >&2
	echo "      restore the delegation; the demote policy lives in src/jit/jit_trap_policy.h" >&2
	exit 1
fi
if printf '%s\n' "$clean" | grep -q 'i_[A-Za-z0-9]'; then
	echo "FAIL: $cpp: jit_trap_demote_opcode carries inline opcode cases or mnemonic special-cases" >&2
	echo "      move them into src/jit/jit_trap_policy.h so this test can assert them" >&2
	exit 1
fi

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/include \
	-o "$out" tests/jit_trap_policy_test.cpp
"$out"
