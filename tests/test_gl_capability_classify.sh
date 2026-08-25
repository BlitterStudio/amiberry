#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/gl_capability_classify_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/osdep \
	-o "$out" \
	tests/gl_capability_classify_test.cpp \
	src/osdep/gl_capability_classify.cpp
"$out"
