#!/usr/bin/env bash
set -euo pipefail

if command -v python3 >/dev/null 2>&1; then
	PYTHON=python3
elif command -v python >/dev/null 2>&1; then
	PYTHON=python
elif command -v py >/dev/null 2>&1; then
	PYTHON="py -3"
else
	echo "python3, python, or py is required" >&2
	exit 1
fi

$PYTHON - <<'PY'
from pathlib import Path
import sys

devices = Path("src/devices.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


try:
	start = devices.index("void virtualdevice_free(void)")
	end = devices.index("void do_leave_program", start)
except ValueError as exc:
	fail(f"Could not find virtual-device shutdown code: {exc}")

shutdown = devices[start:end]
rtarea_free = shutdown.find("rtarea_free();")
free_shm = shutdown.find("free_shm();")
if rtarea_free < 0 or free_shm < 0:
	fail("Virtual-device shutdown must release rtarea and shared memory")
if rtarea_free > free_shm:
	fail("rtarea must be released before tracked shared-memory allocations")
PY
