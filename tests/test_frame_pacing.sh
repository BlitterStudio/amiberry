#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/frame_pacing_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep -o "$out" tests/frame_pacing_test.cpp
"$out"
PYTHONDONTWRITEBYTECODE=1 python3 -B tests/frame_pacing_regressions_test.py
