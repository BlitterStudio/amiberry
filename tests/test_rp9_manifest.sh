#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/rp9_manifest_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc/include -Isrc/osdep \
	-o "$out" \
	tests/rp9_manifest_test.cpp \
	src/osdep/rp9_manifest.cpp \
	src/tinyxml2.cpp
"$out" "$@"
