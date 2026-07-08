#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/play_setup_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-DPLAY_SETUP_NO_AMIBERRY_DEPS \
	-Isrc -Isrc/include -Isrc/osdep \
	-o "$out" \
	tests/play_setup_test.cpp \
	src/osdep/imgui/play_setup.cpp
"$out"
