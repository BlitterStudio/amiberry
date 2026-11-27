#!/bin/sh
# Combined CZNV flag macros (src/machdep/m68k.h) against the per-flag
# expressions gencpu used to emit, for every host flag layout.
set -eu

cd "$(dirname "$0")/.."

cxx=${CXX:-c++}
work="${TMPDIR:-/tmp}/cpu_flags_macros_test.$$"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work"

# The x86-64 GCC/clang build always takes the lahf/seto asm macros, so the
# portable fallback is checked on that layout by re-reading the header's
# fallback block after the test drops the asm definitions.
awk '
	/^#ifndef SET_CZNV_ADDL/ { on = 1 }
	on { print; if ($1 ~ /^#if/) depth++; else if ($1 == "#endif" && --depth == 0) exit }
' src/machdep/m68k.h > "$work/fallback.h"
if ! grep -q 'SET_CZNV4' "$work/fallback.h"; then
	echo "FAIL: portable SET_CZNV_* fallback block not found in src/machdep/m68k.h" >&2
	exit 1
fi

build_run()
{
	name=$1
	shift
	"$cxx" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-unused-variable \
		-Isrc -Isrc/include -DFLAGS_TEST_VARIANT="\"$name\"" "$@" \
		-o "$work/$name" tests/cpu_flags_macros_test.cpp
	"$work/$name"
}

case "$(uname -m)" in
x86_64 | amd64)
	build_run x86_64-sahf-asm -DCPU_x86_64 -DSAHF_SETO_PROFITABLE
	build_run x86_64-sahf-fallback -DCPU_x86_64 -DSAHF_SETO_PROFITABLE \
		-DFLAGS_TEST_FALLBACK_BLOCK="\"$work/fallback.h\""
	build_run x86_64-pushf -DCPU_x86_64 -Duint64=uae_u64
	;;
esac
# Only the flag_struct layout differs between these hosts, so the AArch64
# (NZCV) layout builds and runs natively here.
build_run nzcv-layout -DCPU_AARCH64
build_run generic-layout
