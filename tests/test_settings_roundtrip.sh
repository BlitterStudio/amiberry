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
cd "$(dirname "$0")/.."

if [[ -z "${AMIBERRY_BIN:-}" ]]; then
	echo "AMIBERRY_BIN must point to an amiberry binary" >&2
	exit 2
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
# Isolated environment: no user settings interfere with resolution.
export HOME="$work"

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
	if grep -q 'corrections applied' "$work/err.txt"; then
		echo "$name: the configuration was corrected:" >&2
		cat "$work/err.txt" >&2
		failures=$((failures + 1))
	fi
	local pair default key value written
	while read -r pair default; do
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
