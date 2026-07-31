#!/bin/sh
set -eu

cxx=${CXX:-c++}
iterations=${GFX_GEOMETRY_BENCH_ITERATIONS:-100000000}
test_out="${TMPDIR:-/tmp}/gfx_geometry_test.$$"
benchmark_out="${TMPDIR:-/tmp}/gfx_geometry_benchmark.$$"
trap 'rm -f "$test_out" "$benchmark_out"' EXIT

failure_json()
{
	printf '%s\n' '{"geometry_ns_per_scenario":1e30,"focused_tests_passed":'$1',"reference_outputs_match":0,"result_checksum":0,"timed_scenarios":0}'
}

if ! "$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep \
	-o "$test_out" tests/gfx_geometry_test.cpp; then
	failure_json 0
	exit 0
fi

if ! "$test_out"; then
	failure_json 0
	exit 0
fi

if ! "$cxx" -std=c++17 -O3 -DNDEBUG -march=native -Wall -Wextra -Werror \
	-Isrc/osdep -o "$benchmark_out" tests/gfx_geometry_benchmark.cpp; then
	failure_json 1
	exit 0
fi

"$benchmark_out" "$iterations" 37
