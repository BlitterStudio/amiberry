#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/shader_catalog_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc -Isrc/osdep \
	-o "$out" \
	tests/shader_catalog_test.cpp \
	src/osdep/imgui/shader_catalog.cpp
"$out"
