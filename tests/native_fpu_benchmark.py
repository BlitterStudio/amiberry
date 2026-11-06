"""Deterministic host-ns/op FPU microbenchmark, not emulated-cycle throughput.

Run: python3 tests/native_fpu_benchmark.py
Uses the existing CMake compile command unchanged and executes native regressions
before timing. AMIBERRY_BUILD_DIR selects the configured build.
For before/after comparisons retain binaries with --output-directory tests/.fpu-base
and tests/.fpu-after, then alternate those binaries pinned to one idle CPU:
taskset -c CPU tests/.fpu-base/native-fpu-test
The resulting binary also accepts --libc-clear to benchmark the original clearing
implementation against production in the SAME executable. Both modes use exactly
the same timed workloads, configured optimization flags, and regression fixtures.
AMIBERRY_FPU_BENCH_ITERATIONS sets iterations/sample (default 1000000).
Each row is the median of seven samples after warmup. No main build is needed.
"""

import argparse
from pathlib import Path
import subprocess
import tempfile

from native_fpu_regressions_test import ROOT, build_native_test


def run(directory, reference=False):
    binary = build_native_test(
        directory, (ROOT / "tests/native_fpu_benchmark.cpp").read_text())
    subprocess.run([str(binary)] + (["--libc-clear"] if reference else []), check=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", type=Path)
    parser.add_argument("--libc-clear", action="store_true")
    args = parser.parse_args()
    if args.output_directory:
        directory = args.output_directory.resolve()
        directory.relative_to(ROOT)
        directory.mkdir(parents=True, exist_ok=True)
        run(directory, args.libc_clear)
    else:
        with tempfile.TemporaryDirectory(prefix=".native-fpu-bench-", dir=ROOT / "tests") as directory:
            run(directory, args.libc_clear)
