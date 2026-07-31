#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/on_screen_joystick_layout_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep \
	-o "$out" tests/on_screen_joystick_layout_test.cpp
"$out"
