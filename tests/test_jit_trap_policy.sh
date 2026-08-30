#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/jit_trap_policy_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/include \
	-o "$out" tests/jit_trap_policy_test.cpp
"$out"
