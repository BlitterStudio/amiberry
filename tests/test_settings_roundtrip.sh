#!/usr/bin/env bash
# Behavioral test: every shared-settings option ID is a real .uae key and its
# values use the spelling the configuration parser reads and the writer
# writes back. The settings harness builds configurations through the layer;
# each one is passed to --dump-config as -s overrides and must come back
# unchanged and without corrections. The defaults scenario checks each
# descriptor's default against the built-in configuration.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_settings_roundtrip.sh
set -euo pipefail
if [[ -z "${AMIBERRY_BIN:-}" ]]; then
	echo "AMIBERRY_BIN must point to an amiberry binary" >&2
	exit 2
fi
# The CI passes a relative path; it must survive the cd below.
[[ "$AMIBERRY_BIN" == /* ]] || AMIBERRY_BIN="$PWD/$AMIBERRY_BIN"
cd "$(dirname "$0")/.."

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
# Isolated environment: no user settings interfere with resolution.
export HOME="$work"

# Build the harness with the options the binary has: PPC emulation and the
# JIT are build options (USE_PPC, USE_JIT), and cfgfile or the fixups drop
# their settings when they are compiled out.
SETTINGS_LAYER_DEFINES="-DUSE_PCEM"
"$AMIBERRY_BIN" --dump-config -s cpu_model=68060 -s ppc_model=automatic > "$work/ppc.txt" 2>/dev/null || true
if grep -q '^ppc_model=' "$work/ppc.txt"; then
	SETTINGS_LAYER_DEFINES="$SETTINGS_LAYER_DEFINES -DWITH_PPC"
fi
"$AMIBERRY_BIN" --dump-config -s cycle_exact=false -s cpu_model=68020 -s cpu_24bit_addressing=false \
	-s cachesize=8192 > "$work/jit.txt" 2>/dev/null || true
if ! grep -q '^cachesize=8192$' "$work/jit.txt"; then
	SETTINGS_LAYER_DEFINES="$SETTINGS_LAYER_DEFINES -DAMIBERRY_NO_JIT"
fi
export SETTINGS_LAYER_DEFINES
. tests/lib/settings_layer_build.sh
settings_layer_compile "$work/harness" 2> "$work/build.log" || { cat "$work/build.log" >&2; exit 1; }
"$work/harness" --emit-roundtrip > "$work/scenarios.txt"

failures=0

# check_scenario NAME ARGS_FILE EXPECT_FILE
check_scenario()
{
	local name="$1" args_file="$2" expect_file="$3" args=() line
	while IFS= read -r line; do
		args+=(-s "$line")
	done < "$args_file"
	if ! "$AMIBERRY_BIN" --dump-config ${args[@]+"${args[@]}"} > "$work/dump.txt" 2> "$work/err.txt"; then
		echo "$name: --dump-config failed" >&2
		cat "$work/err.txt" >&2
		failures=$((failures + 1))
		return
	fi
	if grep -q 'unknown configuration entries' "$work/err.txt"; then
		echo "$name: entries no parser took:" >&2
		cat "$work/err.txt" >&2
		failures=$((failures + 1))
	fi
	if grep -q 'corrections applied' "$work/err.txt"; then
		echo "$name: the configuration was corrected:" >&2
		cat "$work/err.txt" >&2
		failures=$((failures + 1))
	fi
	local pair default key value written
	# "id=value<TAB>default": values can have spaces.
	while IFS=$'\t' read -r pair default; do
		key="${pair%%=*}"
		value="${pair#*=}"
		written="$(grep -m1 "^${key//./\\.}=" "$work/dump.txt" | cut -d= -f2- || true)"
		if [[ "$written" == "$value" ]]; then
			continue
		fi
		# A value the writer omits is only acceptable when it is the default.
		if [[ -z "$written" ]] && ! grep -q "^${key//./\\.}=" "$work/dump.txt" && [[ "$value" == "$default" ]]; then
			continue
		fi
		echo "$name: $key=$value came back as '${written}'" >&2
		failures=$((failures + 1))
	done < "$expect_file"
}

scenario=""
flush()
{
	if [[ -n "$scenario" ]]; then
		check_scenario "$scenario" "$work/args.txt" "$work/expect.txt"
	fi
	: > "$work/args.txt"
	: > "$work/expect.txt"
}
flush
while IFS= read -r line; do
	case "$line" in
	"S "*) flush; scenario="${line#S }" ;;
	"A "*) printf '%s\n' "${line#A }" >> "$work/args.txt" ;;
	"E "*) printf '%s\n' "${line#E }" >> "$work/expect.txt" ;;
	esac
done < "$work/scenarios.txt"
flush

if ((failures > 0)); then
	echo "settings round-trip: $failures failure(s)" >&2
	exit 1
fi
echo "settings round-trip test passed"
