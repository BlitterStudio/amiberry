#!/bin/sh
# Behavioral test: a live FPU core switch keeps the guest FPCR in effect,
# and compiled FPU arithmetic reads a memory operand once, after a pending
# exception (interpreter and JIT).
# Starts four emulators with synthetic ROMs; see tests/fpu_core_switch_live_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_fpu_core_switch_live.sh
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

exec "$python" "$(dirname "$0")/fpu_core_switch_live_test.py" "$AMIBERRY_BIN"
