#!/usr/bin/env bash
# Checks the shared settings layer's descriptors, rules, edit sessions and
# validation against the core's fixup_cpu().
set -euo pipefail
cd "$(dirname "$0")/.."
out="${TMPDIR:-/tmp}/settings_layer_test.$$"
trap 'rm -f "$out"' EXIT
. tests/lib/settings_layer_build.sh
settings_layer_compile "$out"
"$out"
