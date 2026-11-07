#!/usr/bin/env bash
# MIDI output stream parser shared with WinUAE: running status, real-time
# bytes inside messages and SysEx, and the bounded SysEx buffer. Runs again
# under AddressSanitizer for the buffer limit.
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/midi_parser_test.$$"
trap 'rm -f "$out" "$out.c"' EXIT

midi_compile() {
	# shellcheck disable=SC2086
	${CXX:-c++} -std=c++17 -O1 -Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep \
		-o "$out" tests/midi_parser_test.cpp src/osdep/midi_parser.cpp
}

midi_compile
"$out"

printf 'int main(void) { return 0; }\n' > "$out.c"
if ${CC:-cc} -fsanitize=address -o "$out" "$out.c" 2> /dev/null; then
	CXX="${CXX:-c++} -fsanitize=address,undefined -fno-omit-frame-pointer" midi_compile
	"$out"
else
	echo "midi parser test: no AddressSanitizer runtime, sanitized run skipped" >&2
fi
