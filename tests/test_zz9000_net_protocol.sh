#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/zz9000_net_protocol_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/include \
  -o "$out" tests/zz9000_net_protocol_test.cpp
"$out"
