#!/usr/bin/env bash
# Lists and extracts archives made with LZX 1.21 (tests/data/lzx: -1 and -3
# compression, merged groups, a subdirectory and an empty file) through the
# real zfile archive code and the LZX decoder, and checks every member
# against the SHA-256 manifest of the original files. Truncated and
# corrupted copies must fail cleanly; that part runs again under
# AddressSanitizer.
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/lzx_archive_test.$$"
trap 'rm -rf "$out" "$out.objs"' EXIT

# Absolute: archive member paths (zfile_open_archive) start with the
# volume's root name, which is the whole archive path only when absolute.
data="$PWD/tests/data/lzx"
# The archives' entry dates, 27 Oct 1990 18:03:09 local time.
date='1990-10-27 18:03:09'

# The 7-Zip code zfile.cpp and zfile_archive.cpp link against.
mkdir -p "$out.objs"
for f in 7zAlloc 7zArcIn 7zBuf 7zBuf2 7zCrc 7zCrcOpt 7zDec 7zStream Alloc Bcj2 Bra Bra86 BraIA64 \
	CpuArch Delta Lzma2Dec LzmaDec Ppmd7 Ppmd7Dec Sha256 Xz XzCrc64 XzCrc64Opt XzDec; do
	${CC:-cc} -O1 -w -Isrc/archivers/7z -c -o "$out.objs/$f.o" "src/archivers/7z/$f.c"
done

lzx_compile() {
	# shellcheck disable=SC2046
	${CXX:-c++} -std=c++17 -O1 -pthread -Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep -Isrc/archivers \
		$(pkg-config --cflags sdl3) -o "$out" tests/lzx_archive_test.cpp \
		src/zfile.cpp src/zfile_archive.cpp src/archivers/lzx/lzx.cpp src/archivers/zip/unzip.cpp \
		src/crc32.cpp src/osdep/charset.cpp "$out.objs"/*.o $(pkg-config --libs sdl3) -lz
}

lzx_compile
"$out" "$data/originals.sha256" "$date" "$data/l1.lzx" "$data/l3.lzx"

# Once more under AddressSanitizer. Skipped only when the toolchain has no
# runtime.
printf 'int main(void) { return 0; }\n' > "$out.c"
if ${CC:-cc} -fsanitize=address -o "$out" "$out.c" 2> /dev/null; then
	rm -f "$out.c"
	CXX="${CXX:-c++} -fsanitize=address -fno-omit-frame-pointer" lzx_compile
	ASAN_OPTIONS=detect_leaks=0 "$out" "$data/originals.sha256" "$date" "$data/l1.lzx" "$data/l3.lzx"
else
	rm -f "$out.c"
	echo "lzx archive test: no AddressSanitizer runtime, sanitized run skipped" >&2
fi
