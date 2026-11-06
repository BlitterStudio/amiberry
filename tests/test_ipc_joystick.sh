#!/bin/sh
set -eu

cxx=${CXX:-c++}
out="tests/.ipc_joystick_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep -Isrc/include \
	-o "$out" tests/ipc_joystick_test.cpp
"$out"
python3 -B tests/ipc_joystick_state_test.py

if [ -n "${AMIBERRY_BIN:-}" ]; then
	python3 tests/ipc_joystick_live_test.py "$AMIBERRY_BIN"
fi
