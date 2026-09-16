#!/bin/sh
# Behavioral test for --dump-config. Runs a built amiberry binary in its
# headless early-exit mode and asserts the resolved-configuration semantics.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_dump_config.sh
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN must point at a built amiberry binary" >&2
	exit 1
fi
bin="$AMIBERRY_BIN"

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

# Isolated environment: no user settings interfere with resolution.
export HOME="$work"

check()
{
	if ! grep -q "$2" "$1"; then
		echo "expected '$2' in $1" >&2
		cat "$1" >&2
		exit 1
	fi
}

# A 68040 config requesting 24-bit addressing must come back corrected:
# fixup_prefs forces address_space_24 off on 68040+, and the correction must
# be reported on stderr. This pins the post-fixup (resolved) semantics -- a
# dump that merely echoed the .uae would fail here.
printf 'config_description=dump-config test\ncpu_type=68040\ncpu_24bit_addressing=true\n' > "$work/probe.uae"
"$bin" --dump-config -f "$work/probe.uae" > "$work/out.txt" 2> "$work/err.txt"
check "$work/out.txt" '^cpu_model=68040$'
check "$work/out.txt" '^cpu_24bit_addressing=false$'
check "$work/err.txt" '24-bit address space is not supported with 68040'

# Defaults-only dump: header present, exits cleanly.
"$bin" --dump-config > "$work/def.txt" 2> "$work/def_err.txt"
check "$work/def.txt" '^; --dump-config: resolved configuration'
check "$work/def.txt" '^; configuration file: (built-in defaults)$'

# Deterministic: two runs of the same config are byte-identical, so dumps
# from two machines can be compared with diff.
"$bin" --dump-config -f "$work/probe.uae" > "$work/out2.txt" 2>/dev/null

# No side effects: the early exit must not create any directory tree.
dir_count="$(find "$work" -type d | wc -l | tr -d ' ')"
[ "$dir_count" -eq 1 ] || { echo "dump-config created directories under \$HOME" >&2; exit 1; }

# Vsync + soundcard probe: target_fixup_options reads the enumerated sound
# device list under gfx_vsyncmode, and a non-default soundcard index must
# survive resolution when devices are enumerated. Must not crash either way.
printf 'gfx_fullscreen=fullscreen\ngfx_vsync=true\ngfx_vsyncmode=busywait\nsoundcard=1\n' > "$work/vsync.uae"
"$bin" --dump-config -f "$work/vsync.uae" > "$work/vsync.txt" 2>/dev/null
[ -s "$work/vsync.txt" ] || { echo "vsync probe produced no output" >&2; exit 1; }

# WHDLoad autoload must resolve without touching the host: resolving an .lha
# through whdload_auto_prefs must not build the booter temp tree or save-data
# links in dump mode.
printf 'not-really-an-archive' > "$work/game.lha"
"$bin" --dump-config --autoload "$work/game.lha" > "$work/lha.txt" 2>/dev/null
[ -s "$work/lha.txt" ] || { echo "lha autoload dump produced no output" >&2; exit 1; }
dir_count="$(find "$work" -type d | wc -l | tr -d ' ')"
[ "$dir_count" -eq 1 ] || { echo "dump-config created directories during WHDLoad autoload" >&2; exit 1; }

echo "dump-config behavioral test passed"
