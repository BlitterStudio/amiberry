#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$"
png="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.png"
jpeg="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.jpg"
trap 'rm -f "$out" "$png" "$jpeg"' EXIT
python3 - "$png" <<'PY'
import struct, sys, zlib
def chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))
rows = [b'\xff\x00\x00\x00\xff\x00', b'\x00\x00\xff\xff\xff\x00', b'\x00\xff\xff\xff\x00\xff']
raw = b''.join(b'\x00' + row for row in rows)
data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 3, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
open(sys.argv[1], 'wb').write(data)
PY
ffmpeg -nostdin -loglevel error -y -i "$png" -frames:v 1 "$jpeg"
${CXX:-c++} -std=c++17 -Isrc/include -Isrc/osdep \
  $(pkg-config --cflags sdl3) \
  -o "$out" tests/zz9000_sdk_image_test.cpp src/zz9000_sdk.cpp \
  $(pkg-config --libs sdl3)
"$out" "$png" "$jpeg"
