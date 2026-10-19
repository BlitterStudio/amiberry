# shellcheck shell=sh
# Behavioral tests that copy AMIBERRY_BIN elsewhere lose its $ORIGIN-relative
# RUNPATH, such as the lib/amiberry directory that holds bundled SDL3. Keep
# those directories reachable through LD_LIBRARY_PATH so the copy still loads.
keep_copied_binary_libraries()
{
	command -v readelf >/dev/null 2>&1 || return 0
	origin="$(cd "$(dirname "$AMIBERRY_BIN")" && pwd -P)"
	runpath="$(readelf -d "$AMIBERRY_BIN" 2>/dev/null | sed -n 's/.*Library [a-z]*path: \[\(.*\)\]$/\1/p')"
	[ -n "$runpath" ] || return 0
	# shellcheck disable=SC2016
	runpath="$(printf '%s\n' "$runpath" | sed -e "s|\\\$ORIGIN|$origin|g" -e "s|\\\${ORIGIN}|$origin|g")"
	LD_LIBRARY_PATH="$runpath${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
	export LD_LIBRARY_PATH
}
