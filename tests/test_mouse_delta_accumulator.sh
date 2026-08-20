#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/mouse_delta_accumulator_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc/osdep \
	-o "$out" \
	tests/mouse_delta_accumulator_test.cpp
"$out"
