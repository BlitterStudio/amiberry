#!/usr/bin/env bash
# ROM-free FPU regressions: exception/branch/frame handlers, format-9 stack
# frames, x86 JIT FPU emission and the native FPU translation unit.
#
# native_fpu_regressions_test.py compiles src/fpp_native.cpp with the flags of
# the configured CMake build: AMIBERRY_BUILD_DIR (default build) must hold
# compile_commands.json (CMakeLists.txt exports it). Without one the native
# test is skipped locally; in CI (GITHUB_ACTIONS) it is required. Its x87
# extended-register checks run on x86 hosts only.
set -euo pipefail
cd "$(dirname "$0")/.."

python3 tests/fpu_exception_regressions_test.py
python3 tests/fpu_frame_regressions_test.py
python3 tests/fpu_jit_compiler_regressions_test.py

database="${AMIBERRY_BUILD_DIR:-build}/compile_commands.json"
if [[ -f "$database" ]]; then
	python3 tests/native_fpu_regressions_test.py
elif [[ "${GITHUB_ACTIONS:-}" == "true" ]]; then
	echo "native_fpu_regressions_test.py needs $database (configure CMake first)" >&2
	exit 1
else
	echo "NOTE: $database not found; native_fpu_regressions_test.py skipped (set AMIBERRY_BUILD_DIR)"
fi
