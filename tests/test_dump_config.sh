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
# The dump must stay byte-exact: LF line endings only. On Windows this pins
# the binary stdout mode (text mode would translate every LF to CRLF); on
# other platforms it passes trivially. Checked first so a regression fails
# with this message rather than a confusing content mismatch.
cr="$(printf '\r')"
if grep -q "$cr" "$work/out.txt"; then
	echo "dump emitted CRLF line endings (Windows stdout left in text mode?)" >&2
	exit 1
fi
check "$work/out.txt" '^cpu_model=68040$'
check "$work/out.txt" '^cpu_24bit_addressing=false$'
check "$work/err.txt" '24-bit address space is not supported with 68040'

# Explicit controller-removal fallbacks must survive config parsing and
# serialization. kbd3 selects keyboard layout C; none disables fallback.
printf 'joyport0=joy0\njoyportdefault0=kbd3\njoyport1=joy1\njoyportdefault1=none\njoyport2=none\njoyportdefault2=none\njoyport3=none\njoyportdefault3=none\n' > "$work/joyport-default.uae"
"$bin" --dump-config -f "$work/joyport-default.uae" > "$work/joyport-default.txt" 2>/dev/null
check "$work/joyport-default.txt" '^joyportdefault0=kbd3$'
check "$work/joyport-default.txt" '^joyportdefault1=none$'
check "$work/joyport-default.txt" '^joyportdefault2=none$'
check "$work/joyport-default.txt" '^joyportdefault3=none$'

# Invalid keyboard-layout fallbacks must not escape the keyboard ID range.
printf 'joyport0=joy0\njoyportdefault0=kbd11\n' > "$work/joyport-invalid-default.uae"
"$bin" --dump-config -f "$work/joyport-invalid-default.uae" > "$work/joyport-invalid-default.txt" 2>/dev/null
if grep -q '^joyportdefault0=' "$work/joyport-invalid-default.txt"; then
	echo "invalid joyport keyboard fallback survived config parsing" >&2
	exit 1
fi

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

# A config file that fails to load must fail the dump: exit non-zero, no
# stdout, and a stderr explanation -- never a defaults dump masquerading as
# the requested configuration.
if "$bin" --dump-config -f "$work/missing.uae" > "$work/fail.txt" 2> "$work/fail_err.txt"; then
	echo "dump of a missing config must exit non-zero" >&2
	exit 1
fi
[ -s "$work/fail.txt" ] && { echo "failed dump must not write stdout" >&2; exit 1; }
check "$work/fail_err.txt" 'failed to load'

# The positional form of a missing configuration must fail the same way.
if "$bin" --dump-config "$work/missing-positional.uae" > "$work/pfail.txt" 2>/dev/null; then
	echo "dump of a missing positional config must exit non-zero" >&2
	exit 1
fi
[ -s "$work/pfail.txt" ] && { echo "failed positional dump must not write stdout" >&2; exit 1; }

# --log must not corrupt the dump: console logging writes to stdout, so it is
# disabled in dump mode; the first stdout line stays the dump header.
"$bin" --dump-config --log -f "$work/probe.uae" > "$work/log.txt" 2>/dev/null
head -n 1 "$work/log.txt" | grep -q '^; --dump-config: resolved configuration' \
	|| { echo "--log corrupted the dump stream" >&2; exit 1; }

# A corrupt positional RP9 must fail the dump the same way (rp9_parse_file
# rejects a non-ZIP payload, so target_cfgfile_load fails).
printf 'not-a-zip' > "$work/bad.rp9"
if "$bin" --dump-config "$work/bad.rp9" > "$work/rp9fail.txt" 2>/dev/null; then
	echo "dump of a corrupt positional RP9 must exit non-zero" >&2
	exit 1
fi
[ -s "$work/rp9fail.txt" ] && { echo "failed RP9 dump must not write stdout" >&2; exit 1; }

# An --autoload media type that cannot provide preferences must fail the
# dump rather than label defaults with its name.
printf 'x' > "$work/media.adf"
if "$bin" --dump-config --autoload "$work/media.adf" > "$work/afail.txt" 2>/dev/null; then
	echo "dump with unsupported --autoload media must exit non-zero" >&2
	exit 1
fi
[ -s "$work/afail.txt" ] && { echo "unsupported autoload dump must not write stdout" >&2; exit 1; }

# A trailing option without its operand must fail the dump instead of
# producing what looks like a valid defaults dump.
if "$bin" --dump-config -f > "$work/operand.txt" 2> "$work/operand_err.txt"; then
	echo "dump with a missing option operand must exit non-zero" >&2
	exit 1
fi
[ -s "$work/operand.txt" ] && { echo "missing-operand dump must not write stdout" >&2; exit 1; }
check "$work/operand_err.txt" 'incomplete command line'

echo "dump-config behavioral test passed"
