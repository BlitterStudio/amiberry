#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="${TMPDIR:-/tmp}/ipc_gui_automation_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -pthread -Wall -Wextra -Werror -Isrc/osdep \
	-o "$out" tests/ipc_gui_automation_test.cpp
"$out"
