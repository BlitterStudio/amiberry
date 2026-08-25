#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/crt_gpu_allowlist_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/osdep \
	-o "$out" \
	tests/crt_gpu_allowlist_test.cpp \
	src/osdep/crt_gpu_allowlist.cpp
"$out"
