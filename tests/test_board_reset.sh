#!/bin/sh
# Behavioral test: expansion boards a reset brings in get memory of their own.
# Starts emulators (on macOS one opens a window); see tests/board_reset_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_board_reset.sh
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

exec "$python" "$(dirname "$0")/board_reset_test.py" "$AMIBERRY_BIN"
