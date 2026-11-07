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
# The checks run on a copy of the body with C comments fully stripped --
# including the conventional multiline form whose "* return ..." middle
# lines otherwise look like live code -- and rejects preprocessor lines,
# since #if 0 can compile the delegation out while it still greps. The
# delegation must be the function's sole policy authority: no mnemonic
# references may remain.
cpp=src/jit/arm/compemu_support_arm.cpp
body=$(sed -n '/^static bool jit_trap_demote_opcode/,/^}/p' "$cpp")
if [ -z "$body" ]; then
	echo "FAIL: jit_trap_demote_opcode not found in $cpp" >&2
	echo "      (moved or renamed? update this guard's extraction pattern)" >&2
	exit 1
fi
clean=$(printf '%s\n' "$body" | awk '
	function strip_line_comments(s,   l) {
		l = index(s, "//")
		return l > 0 ? substr(s, 1, l - 1) : s
	}
	{
		line = $0
		out = ""
		while (length(line) > 0) {
			if (incomment) {
				e = index(line, "*/")
				if (e == 0) {
					line = ""
				} else {
					line = substr(line, e + 2)
					incomment = 0
				}
			} else {
				s = index(line, "/*")
				if (s == 0) {
					out = out strip_line_comments(line)
					line = ""
				} else {
					out = out substr(line, 1, s - 1)
					line = substr(line, s + 2)
					incomment = 1
				}
			}
		}
		print out
	}' )
if ! printf '%s\n' "$clean" | grep -q 'jit_trap_demote_mnemo('; then
	echo "FAIL: $cpp: jit_trap_demote_opcode does not delegate to jit_trap_demote_mnemo()" >&2
	echo "      restore the delegation; the demote policy lives in src/jit/jit_trap_policy.h" >&2
	exit 1
fi
# Pin the index, not just the delegation. op arrives already mapped by
# DO_GET_OPCODE(); mapping it again (the pre-#2342 form) classifies
# table68k[bswap(op)] -- junk entries initialised to i_ILLG, so nearly
# every trap opcode demotes and the narrowing in the header is dead code,
# while every other check here stays green. Requiring the exact index
# also catches a re-swap spelled any other way (uae_bswap_16, a macro).
if printf '%s\n' "$clean" | grep -q 'get_opcode_cft_map'; then
	echo "FAIL: $cpp: jit_trap_demote_opcode maps op a second time (get_opcode_cft_map)" >&2
	echo "      op is already the true opcode; index table68k[op] directly (#2315)" >&2
	exit 1
fi
if ! printf '%s\n' "$clean" | grep -q 'table68k[[]op[]]'; then
	echo "FAIL: $cpp: jit_trap_demote_opcode does not classify table68k[op]" >&2
	echo "      any other index misclassifies the opcode; see #2315" >&2
	exit 1
fi
if printf '%s\n' "$clean" | grep -q 'i_[A-Za-z0-9]'; then
	echo "FAIL: $cpp: jit_trap_demote_opcode carries inline opcode cases or mnemonic special-cases" >&2
	echo "      move them into src/jit/jit_trap_policy.h so this test can assert them" >&2
	exit 1
fi
if printf '%s\n' "$clean" | grep -q '^[[:space:]]*#'; then
	echo "FAIL: $cpp: jit_trap_demote_opcode contains preprocessor directives" >&2
	echo "      (#if 0 can compile out the delegation; flatten it or update this guard)" >&2
	exit 1
fi

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/include \
	-o "$out" tests/jit_trap_policy_test.cpp
"$out"
