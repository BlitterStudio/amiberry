#!/bin/sh
# Behavioral test: an RTG board that changes bus is brought in by a reset.
# Starts a headless emulator; see tests/rtg_reset_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_rtg_reset.sh
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

exec "$python" "$(dirname "$0")/rtg_reset_test.py" "$AMIBERRY_BIN"
