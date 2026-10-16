#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$"
png="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.png"
jpeg="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.jpg"
webp_lossy="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.webp"
webp_lossless="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.lossless.webp"
webp_animation="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.animation.webp"
webp_alpha="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.alpha.webp"
alpha_png="${TMPDIR:-/tmp}/zz9000_sdk_image_test.$$.alpha.png"
trap 'rm -f "$out" "$png" "$jpeg" "$webp_lossy" "$webp_lossless" "$webp_animation" "$webp_alpha" "$alpha_png"' EXIT
python3 - "$png" <<'PY'
import struct, sys, zlib
def chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))
rows = [b'\xff\x00\x00\x00\xff\x00', b'\x00\x00\xff\xff\xff\x00', b'\x00\xff\xff\xff\x00\xff']
raw = b''.join(b'\x00' + row for row in rows)
data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 3, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
open(sys.argv[1], 'wb').write(data)
PY
python3 - "$alpha_png" <<'PY'
import struct, sys, zlib
def chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data))
pixels = [
    (0, 0, 0, 0), (0x44, 0x55, 0x66, 255),
    (0x77, 0x88, 0x99, 128), (0xaa, 0xbb, 0xcc, 64),
    (0x12, 0x34, 0x56, 255), (0x65, 0x43, 0x21, 0),
    (0xde, 0xad, 0xbe, 17), (0xef, 0xca, 0xfe, 238),
    (0x35, 0x79, 0xbd, 85), (0x24, 0x68, 0xac, 170),
    (0x0f, 0x1e, 0x2d, 1), (0x3c, 0x4b, 0x5a, 254),
]
raw = b''.join(b'\x00' + bytes(sum((list(pixel) for pixel in pixels[row * 4:(row + 1) * 4]), []))
               for row in range(3))
data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 4, 3, 8, 6, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')
open(sys.argv[1], 'wb').write(data)
PY
ffmpeg -nostdin -loglevel error -y -i "$png" -frames:v 1 "$jpeg"
ffmpeg -nostdin -loglevel error -y -i "$png" -frames:v 1 -c:v libwebp "$webp_lossy"
ffmpeg -nostdin -loglevel error -y -i "$png" -frames:v 1 -c:v libwebp -lossless 1 "$webp_lossless"
ffmpeg -nostdin -loglevel error -y -i "$alpha_png" -frames:v 1 -c:v libwebp -lossless 1 -pix_fmt bgra "$webp_alpha"
ffmpeg -nostdin -loglevel error -y \
	-f lavfi -i color=c=red:s=2x3:r=10:d=0.1 \
	-f lavfi -i color=c=blue:s=2x3:r=10:d=0.1 \
	-filter_complex '[0:v][1:v]concat=n=2:v=1:a=0' -c:v libwebp_anim -lossless 1 -loop 1 "$webp_animation"
. tests/zz9000_sdk_build.sh
zz9000_sdk_compile "$out" tests/zz9000_sdk_image_test.cpp
"$out" "$png" "$jpeg" "$webp_lossy" "$webp_lossless" "$webp_animation" "$webp_alpha"
