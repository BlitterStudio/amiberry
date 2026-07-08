#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/play_content_detection_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/include -Isrc/osdep \
	-o "$out" \
	tests/play_content_detection_test.cpp \
	src/osdep/imgui/play_content_detection.cpp
"$out"
