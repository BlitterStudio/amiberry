#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/uaesnd_capture_fifo_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/include -o "$out" tests/uaesnd_capture_fifo_test.cpp
"$out"
