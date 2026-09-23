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
memory = Path("src/osdep/amiberry_mem.cpp").read_text()


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

try:
	release_start = memory.index("static bool release_nondirect_shmid(")
	release_end = memory.index("static int find_shmid_by_address(", release_start)
	clear_start = memory.index("static void clear_shm ()")
	clear_end = memory.index("bool preinit_shm ()", clear_start)
	alloc_start = memory.index("bool uae_mman_alloc_nodirect(")
	alloc_end = memory.index("void *uae_shmat", alloc_start)
except ValueError as exc:
	fail(f"Could not find shared-memory lifecycle code: {exc}")

release = memory[release_start:release_end]
clear = memory[clear_start:clear_end]
allocation = memory[alloc_start:alloc_end]
if "shm_heapowners[shmid]" not in release or "owner->baseaddr = nullptr;" not in release:
	fail("Non-direct allocation release must invalidate its owning bank")
if "owner->allocated_size = 0;" not in release:
	fail("Non-direct allocation release must clear the owning bank size")
if "release_nondirect_shmid(i)" not in clear:
	fail("Shared-memory registry cleanup must release tracked non-direct allocations")
if "shm_heapowners[shmid] = ab;" not in allocation:
	fail("Non-direct allocations must retain their owning bank")
PY
