#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/gdb_protocol_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc \
  -o "$out" src/gdb_protocol.cpp tests/gdb_protocol_test.cpp
"$out"
