#!/usr/bin/env bash
# Behavioral test: every Quickstart preset applied through the shared
# settings layer (--apply-preset, the C API path) produces the configuration
# the model is defined to produce. Each line of the expectation table below
# was taken from the preset definitions in src/cfgfile.cpp (bip_*), so a
# regression here means the layer diverged from the engine's presets.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_settings_presets.sh
set -euo pipefail
if [[ -z "${AMIBERRY_BIN:-}" ]]; then
	echo "AMIBERRY_BIN must point to an amiberry binary" >&2
	exit 2
fi
[[ "$AMIBERRY_BIN" == /* ]] || AMIBERRY_BIN="$PWD/$AMIBERRY_BIN"
cd "$(dirname "$0")/.."

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
export HOME="$work" # isolated environment: no user settings

failures=0

# check PRESET_SPEC EXPECTATION... (key=value pairs the dump must contain)
check()
{
	local spec="$1"; shift
	if ! "$AMIBERRY_BIN" --dump-config --apply-preset "$spec" \
		> "$work/dump.txt" 2> "$work/err.txt"; then
		echo "$spec: --dump-config failed" >&2
		cat "$work/err.txt" >&2
		failures=$((failures + 1))
		return
	fi
	local pair key value written
	for pair in "$@"; do
		key="${pair%%=*}"
		value="${pair#*=}"
		written="$(grep -m1 "^${key//./\\.}=" "$work/dump.txt" | cut -d= -f2- || true)"
		if [[ "$written" != "$value" ]]; then
			echo "$spec: $key=$value expected, got '${written}'" >&2
			failures=$((failures + 1))
		fi
	done
}

# Model facts from bip_* in src/cfgfile.cpp. Compatibility level 1 (the
# panel default) unless noted otherwise; the table drives the whole check.
check a500:0:1      cpu_model=68000 chipset=ocs chipset_compatible=A500 chipmem_size=1 bogomem_size=2 collision_level=full
check a500:2:1      chipmem_size=2 bogomem_size=0
check a500:4:1      chipset=ocs
check a500plus:0:1  chipset_compatible=A500+ chipmem_size=2
check a500plus:1:1  chipmem_size=4
check a500plus:2:1  fastmem_size=4
check a600:0:1      chipset_compatible=A600 chipmem_size=2 chipset=ecs
check a600:2:1      chipmem_size=4 fastmem_size=4
check a600:3:1      fastmem_size=8
check a1000:0:1     chipset=a1000 chipset_compatible=A1000
check a1200:0:1     cpu_model=68020 chipset=aga chipset_compatible=A1200 chipmem_size=4
check a1200:1:1     fastmem_size=4
check a3000:0:1     cpu_model=68030 fpu_model=68882 chipset=ecs chipset_compatible=A3000 a3000mem_size=8
check a4000:0:0     cpu_model=68030 chipset=aga chipset_compatible=A4000
check a4000:1:0     cpu_model=68040
check a4000t:1:0    cpu_model=68040 chipset_compatible=A4000T
check cd32:0:1      cpu_model=68020 chipset=aga chipset_compatible=CD32
check cdtv:0:1      cpu_model=68000 chipset_compatible=CDTV
check macrosystem:0:0 cpu_model=68060 chipset_compatible=DraCo
# Note: bip_cd32/bip_cdtv return before their config switch when the
# machine ROMs are not installed, so their per-config extras (FMV,
# Cubo, 8 MB fast RAM) are ROM-dependent and covered by the harness
# preset test with the stubbed engine instead.

# The Quickstart constraints: stock machines keep one drive and drop the
# second (floppy1type=-1; the DD type is the writer's default and omitted),
# HD models get the HD drive; CD machines get no floppy and a CD drive.
check a500:0:1      floppy1type=-1
check a4000:0      floppy0type=1 floppy1type=-1
check cd32:0:1      floppy0type=-1 floppy1type=-1 cdimage0=

# A preset starts from the machine's built-in configuration: options outside
# its model facts go back to their defaults (the ZZ9000 network and INT2
# once survived it).
if "$AMIBERRY_BIN" --dump-config -s zz9000_net=none -s zz9000_int2=true --apply-preset a4000 \
	> "$work/dump.txt" 2> "$work/err.txt"; then
	for expected in zz9000_net=slirp zz9000_int2=false; do
		if ! grep -qx "$expected" "$work/dump.txt"; then
			echo "a4000 preset after other ZZ9000 settings: $expected expected, got '$(grep -m1 "^${expected%%=*}=" "$work/dump.txt")'" >&2
			failures=$((failures + 1))
		fi
	done
else
	echo "a4000 preset after other ZZ9000 settings: --dump-config failed" >&2
	failures=$((failures + 1))
fi

# Unknown presets, configurations and compatibility levels out of the
# preset's range, and malformed numbers must be rejected rather than
# quietly producing another machine.
for bad in nope:0:0 a1200:99:1 a1200:x a1200:1:1junk a4000:0:1 a1200:0:5 a1200: a1200:-1; do
	if "$AMIBERRY_BIN" --dump-config --apply-preset "$bad" > "$work/bad.txt" 2>/dev/null; then
		echo "invalid preset '$bad' was accepted" >&2
		failures=$((failures + 1))
	fi
done

if ((failures > 0)); then
	echo "settings presets: $failures failure(s)" >&2
	exit 1
fi
echo "settings preset test passed"
