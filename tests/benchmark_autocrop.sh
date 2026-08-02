#!/bin/sh
set -eu

cxx=${CXX:-c++}
frames=${AUTOCROP_BENCH_FRAMES:-1000}
test_out="${TMPDIR:-/tmp}/autocrop_helpers_test.$$"
benchmark_out="${TMPDIR:-/tmp}/autocrop_benchmark.$$"
trap 'rm -f "$test_out" "$benchmark_out"' EXIT

failure_json()
{
	printf '%s\n' '{"autocrop_ns_per_frame":1e30,"focused_tests_passed":'$1',"reference_outputs_match":0,"stable_border_ns_per_frame":1e30,"visible_content_ns_per_frame":1e30,"doubled_resolution_ns_per_frame":1e30,"result_checksum":0,"timed_frames":0}'
}

if ! "$cxx" -std=c++17 -Wall -Wextra -Werror -Isrc/osdep \
	-o "$test_out" tests/autocrop_helpers_test.cpp; then
	failure_json 0
	exit 0
fi

if ! "$test_out"; then
	failure_json 0
	exit 0
fi

if ! "$cxx" -std=c++17 -O3 -DNDEBUG -march=native -Wall -Wextra -Werror \
	-Wno-maybe-uninitialized \
	-Isrc/osdep -o "$benchmark_out" tests/autocrop_benchmark.cpp; then
	failure_json 1
	exit 0
fi

"$benchmark_out" "$frames"
