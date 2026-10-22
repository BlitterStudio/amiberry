#!/bin/sh
# Behavioral test of the settings layer's apply classes: every option the
# running emulation lets the IPC settings session change must reach the
# running emulation at the step its descriptor declares (live, reset or
# restart), and validation must explain every adjustment. Starts a headless
# emulator per machine; see tests/settings_apply_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_settings_apply.sh
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN is not set" >&2
	exit 1
fi

python=""
for candidate in python3 python; do
	if command -v "$candidate" > /dev/null 2>&1 && "$candidate" -c '' > /dev/null 2>&1; then
		python="$candidate"
		break
	fi
done
if [ -z "$python" ]; then
	echo "python3 is required" >&2
	exit 1
fi

test="$(dirname "$0")/settings_apply_test.py"
export PYTHONDONTWRITEBYTECODE=1
# Three machines, so options that depend on the CPU, chipset or memory are
# available in at least one; the third starts with the JIT, genlock and the
# on-screen controls on for the options that depend on those.
"$python" "$test" "$AMIBERRY_BIN" A500
"$python" "$test" "$AMIBERRY_BIN" A4000
"$python" "$test" "$AMIBERRY_BIN" A1200 cachesize=16384 genlock=true \
	amiberry.vkbd_enabled=true amiberry.onscreen_joystick=true
echo "settings apply test passed"
