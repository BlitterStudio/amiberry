#!/bin/sh
set -eu

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/appimage_asset_selection_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep \
	-o "$out" \
	tests/appimage_asset_selection_test.cpp \
	src/osdep/amiberry_update_asset_policy.cpp
"$out"
