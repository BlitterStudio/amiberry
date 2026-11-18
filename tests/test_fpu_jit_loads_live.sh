#!/bin/sh
# Behavioral test: FMOVECR in every rounding mode and FMOVE.X/.D/FMOVEM.X
# memory loads, interpreter against the JIT FPU fast path.
# Starts two emulators with a synthetic ROM; see tests/fpu_jit_loads_live_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_fpu_jit_loads_live.sh
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

exec "$python" "$(dirname "$0")/fpu_jit_loads_live_test.py" "$AMIBERRY_BIN"
