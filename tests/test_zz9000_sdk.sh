#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/zz9000_sdk_test.$$"
trap 'rm -f "$out"' EXIT
. tests/zz9000_sdk_build.sh
zz9000_sdk_compile "$out" tests/zz9000_sdk_test.cpp
"$out"
