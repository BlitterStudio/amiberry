#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/gl_context_ladder_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/osdep \
	-o "$out" \
	tests/gl_context_ladder_test.cpp \
	src/osdep/gl_context_ladder.cpp
"$out"
