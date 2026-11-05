#!/bin/sh
# Behavioral test: FScc/FBcc over every FPSR condition code value and
# predicate, interpreter and JIT, with the integer CCR checked around them.
# Starts two emulators with a synthetic ROM; see tests/fpu_predicates_live_test.py.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_fpu_predicates_live.sh
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

exec "$python" "$(dirname "$0")/fpu_predicates_live_test.py" "$AMIBERRY_BIN"
