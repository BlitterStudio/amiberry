#!/usr/bin/env bash
# Clipboard sharing: IFF FTXT/ILBM codecs and the clipboard.device handshake
# (shared with WinUAE), the host image formats (BMP, PNG) and the hand-over
# between the host and the guest. Runs again under AddressSanitizer, since
# the decoders read data from the guest and from other host applications.
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/clipboard_codecs_test.$$"
trap 'rm -f "$out" "$out.c"' EXIT

clipboard_compile() {
	# shellcheck disable=SC2046
	${CXX:-c++} -std=c++17 -O1 -pthread -Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep \
		$(pkg-config --cflags sdl3 libpng) -o "$out" tests/clipboard_codecs_test.cpp \
		src/osdep/clipboard.cpp src/osdep/clipboard_common.cpp src/osdep/clipboard_image.cpp \
		$(pkg-config --libs sdl3 libpng)
}

clipboard_compile
"$out"

printf 'int main(void) { return 0; }\n' > "$out.c"
if ${CC:-cc} -fsanitize=address -o "$out" "$out.c" 2> /dev/null; then
	CXX="${CXX:-c++} -fsanitize=address,undefined -fno-omit-frame-pointer" clipboard_compile
	ASAN_OPTIONS=detect_leaks=0 "$out"
else
	echo "clipboard codecs test: no AddressSanitizer runtime, sanitized run skipped" >&2
fi
