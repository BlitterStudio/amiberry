#!/usr/bin/env bash
# ROM-free FPU regressions: exception/branch/frame handlers, format-9 stack
# frames, x86 JIT FPU emission and the exact core's inline arithmetic
# against softfloat.
#
# softfloat_fastx80_test.py compiles softfloat with the flags of the
# configured CMake build: AMIBERRY_BUILD_DIR (default build) must hold
# compile_commands.json (CMakeLists.txt exports it). Without one the test is
# skipped locally; in CI (GITHUB_ACTIONS) it is required.
set -euo pipefail
cd "$(dirname "$0")/.."

python3 tests/fpu_exception_regressions_test.py
python3 tests/fpu_frame_regressions_test.py
python3 tests/fpu_jit_compiler_regressions_test.py

database="${AMIBERRY_BUILD_DIR:-build}/compile_commands.json"
if [[ -f "$database" ]]; then
	python3 tests/softfloat_fastx80_test.py
elif [[ "${GITHUB_ACTIONS:-}" == "true" ]]; then
	echo "softfloat_fastx80_test.py needs $database (configure CMake first)" >&2
	exit 1
else
	echo "NOTE: $database not found; softfloat_fastx80_test.py skipped (set AMIBERRY_BUILD_DIR)"
fi
