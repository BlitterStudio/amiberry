#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$"
video="${TMPDIR:-/tmp}/zz9000_sdk_media_test.$$.mpg"
trap 'rm -f "$out" "$video"' EXIT
ffmpeg -nostdin -loglevel error -y \
  -f lavfi -i 'testsrc2=size=160x120:rate=25:duration=1' \
  -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=1' \
  -c:v mpeg1video -q:v 4 -c:a mp2 -b:a 128k -f mpeg "$video"
${CXX:-c++} -std=c++17 -Isrc/include -Isrc/osdep \
  $(pkg-config --cflags sdl3) \
  -o "$out" tests/zz9000_sdk_media_test.cpp src/zz9000_sdk.cpp \
  $(pkg-config --libs sdl3)
"$out" "$video"
