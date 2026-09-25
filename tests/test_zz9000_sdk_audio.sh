#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_audio_test.$$"
mp3="${TMPDIR:-/tmp}/zz9000_sdk_audio_test.$$.mp3"
mono_mp3="${TMPDIR:-/tmp}/zz9000_sdk_audio_mono_test.$$.mp3"
low_mp3="${TMPDIR:-/tmp}/zz9000_sdk_audio_low_test.$$.mp3"
trap 'rm -f "$out" "$mp3" "$mono_mp3" "$low_mp3"' EXIT
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.35' \
  -ar 48000 -ac 2 -b:a 128k "$mp3"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.2' \
  -ar 44100 -ac 1 -b:a 64k "$mono_mp3"
ffmpeg -nostdin -loglevel error -f lavfi -i 'sine=frequency=440:duration=0.2' \
  -ar 22050 -ac 1 -b:a 48k "$low_mp3"
${CXX:-c++} -std=c++17 -DHAVE_MPG123 -Isrc -Isrc/include -Isrc/osdep \
  $(pkg-config --cflags sdl3 libmpg123) \
  -o "$out" tests/zz9000_sdk_test.cpp src/zz9000_sdk.cpp \
  $(pkg-config --libs sdl3 libmpg123)
SDL_AUDIODRIVER=dummy "$out" "$mp3" "$mono_mp3" "$low_mp3"
