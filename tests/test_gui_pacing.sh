#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/gui_pacing_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep -o "$out" tests/gui_pacing_test.cpp
"$out"
