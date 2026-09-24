#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/uaenet_host_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep -o "$out" tests/uaenet_host_test.cpp
"$out"
