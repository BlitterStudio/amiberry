#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/bsdsocket_blocking_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep -o "$out" tests/bsdsocket_blocking_test.cpp
"$out"
