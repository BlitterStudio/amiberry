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

codegen = Path("src/jit/arm/codegen_arm64.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


try:
	start = codegen.index("STATIC_INLINE void compemu_raw_store_pc_state_from_work1(")
	end = codegen.index("LOWFUNC(NONE,WRITE,1,compemu_raw_set_pc_i", start)
except ValueError as exc:
	fail(f"Could not find ARM64 PC synchronization code: {exc}")

pc_sync = codegen[start:end]
required = (
	"LOAD_U64(REG_WORK2, (uintptr)&canbang);",
	"LDRB_wXi(REG_WORK2, REG_WORK2, 0);",
	"CBZ_wi(REG_WORK2, 0);",
)
if any(statement not in pc_sync for statement in required):
	fail("ARM64 PC synchronization must branch on the runtime direct-map state")
if "CBZ_xi(R_MEMSTART, 0);" in pc_sync:
	fail("ARM64 PC synchronization must not infer direct-map state from natmem_offset")
PY
