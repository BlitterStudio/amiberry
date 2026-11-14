#!/bin/sh
# Behavioral test: the CPU thread and the main-thread CPU over synthetic ROMs
# (counters, trace, bus errors, STOP, hardware polling), interpreter and JIT,
# with IPC pauses, memory access, savestates and resets while running.
# See tests/cpu_thread_live_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_cpu_thread_live.sh
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

exec "$python" "$(dirname "$0")/cpu_thread_live_test.py" "$AMIBERRY_BIN"
