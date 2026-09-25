#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_test.$$"
trap 'rm -f "$out"' EXIT
${CXX:-c++} -std=c++17 -Isrc/include -Isrc/osdep \
  -o "$out" tests/zz9000_sdk_test.cpp src/zz9000_sdk.cpp \
  $(pkg-config --libs sdl3)
"$out"
