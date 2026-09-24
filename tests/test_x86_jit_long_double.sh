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

sysconfig = Path("src/osdep/sysconfig.h").read_text()
sysdeps = Path("src/include/sysdeps.h").read_text()
jit_support = Path("src/jit/x86/compemu_support_x86.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


x86_long_double = """#if defined(__x86_64__) || defined(_M_AMD64)
#define SUPPORT_LONG_DOUBLE
#endif"""
if x86_long_double not in sysconfig:
	fail("The x86-64 host must advertise extended-precision JIT spill support")
if "extern bool use_long_double;" not in sysdeps:
	fail("The x86 JIT must see the runtime extended-precision mode flag")
if jit_support.count("#ifdef SUPPORT_LONG_DOUBLE") != 6:
	fail("Every extended-precision x86 JIT spill/load branch must use the host capability")
if "#ifdef USE_LONG_DOUBLE" in jit_support:
	fail("x86 JIT spill support must not depend directly on the FPU storage type")
for operation in ("raw_fmov_ext_mr(", "raw_fmov_ext_mr_drop(", "raw_fmov_ext_rm("):
	if operation not in jit_support:
		fail(f"Missing extended-precision x86 JIT operation: {operation}")
PY
