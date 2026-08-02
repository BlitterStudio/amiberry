#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/filesys_permissions_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep -o "$out" tests/filesys_permissions_test.cpp
"$out"
