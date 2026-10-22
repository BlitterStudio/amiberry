#!/bin/sh
# Behavioral test for IPC runtime actions and the settings edit session.
# Starts a headless emulator; see tests/ipc_actions_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_ipc_actions.sh
# Set AMIBERRY_MODAL_REQUESTERS=1 for a native Apple UI build, whose file
# requesters are modal panels that IPC cannot answer.
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

exec "$python" "$(dirname "$0")/ipc_actions_test.py" "$AMIBERRY_BIN"
