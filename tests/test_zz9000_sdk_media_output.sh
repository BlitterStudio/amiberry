#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_media_output_test.$$"
trap 'rm -f "$out"' EXIT
# The converter has no dependencies: it builds on its own.
${CXX:-c++} -std=c++17 -Wall -Wextra -Isrc/include -o "$out" \
	tests/zz9000_sdk_media_output_test.cpp src/zz9000_sdk_media_output.cpp
"$out"
