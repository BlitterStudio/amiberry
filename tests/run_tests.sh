#!/usr/bin/env bash
# Run the Amiberry test suite. Invoke from anywhere; it changes to the
# repository root itself.
#
# Usage: tests/run_tests.sh [--behavioral-only] [path-to-amiberry-binary]
#
# Without a binary path (or AMIBERRY_BIN), only the static source checks and
# self-contained harnesses run. With one, the behavioral tests run too. The
# --behavioral-only mode requires a binary and skips static checks that need
# development dependencies outside the configured build.
#
# Requirements: bash, a C++17 compiler, grep, pkg-config, libcurl and OpenSSL;
# ripgrep and python3 for the tests that use them.
set -u
# bash 3.2 (stock macOS) treats empty-array expansion under set -u as an
# unbound variable; the ${arr[@]+...} idiom keeps the runner portable.

cd "$(dirname "$0")/.." || exit
# Known-red tests. Each entry is "test|reason". A test listed here is a
# standing bug tracked elsewhere, not a permanent exemption: keep this empty.
SKIPPED_TESTS=()

# Tests that run a built emulator binary (via AMIBERRY_BIN).
BEHAVIORAL_TESTS=(
	"test_dump_config.sh"
	"test_appimage_portable_mode.sh"
	"test_ipc_transport.sh"
	"test_ipc_actions.sh"
	"test_plugin_search_paths.sh"
	"test_relocatable_install_paths.sh"
	"test_relative_paths.sh"
	"test_settings_roundtrip.sh"
	"test_settings_presets.sh"
	"test_settings_apply.sh"
	"test_settings_schema.sh"
	"test_native_ui_bridge.sh"
)

behavioral_only=0
if [[ "${1:-}" == "--behavioral-only" ]]; then
	behavioral_only=1
	shift
fi
binary="${1:-${AMIBERRY_BIN:-}}"
if [[ "$behavioral_only" == 1 && -z "$binary" ]]; then
	echo "--behavioral-only requires an amiberry binary" >&2
	exit 2
fi
if [[ -n "$binary" && ! -x "$binary" ]]; then
	echo "amiberry binary '$binary' is not executable" >&2
	exit 1
fi

pass=0
fail=0
failed_tests=()

skip_reason_for()
{
	local wanted="$1" entry
	for entry in ${SKIPPED_TESTS[@]+"${SKIPPED_TESTS[@]}"}; do
		if [[ "${entry%%|*}" == "$wanted" ]]; then
			printf '%s' "${entry#*|}"
			return
		fi
	done
}
if [[ "$behavioral_only" == 0 ]]; then
for test_path in tests/*.sh; do
	test_name="$(basename "$test_path")"
	# This runner is not a test; behavioral tests run separately below with
	# the binary wired up.
	[[ "$test_name" == "run_tests.sh" ]] && continue
	seen_in_behavioral=0
	for behavioral in ${BEHAVIORAL_TESTS[@]+"${BEHAVIORAL_TESTS[@]}"}; do
		[[ "$behavioral" == "$test_name" ]] && seen_in_behavioral=1
	done
	[[ "$seen_in_behavioral" == 1 ]] && continue
	reason="$(skip_reason_for "$test_name")"
	if [[ -n "$reason" ]]; then
		echo "SKIP $test_name ($reason)"
		continue
	fi
	if bash "$test_path"; then
		echo "PASS $test_name"
		pass=$((pass + 1))
	else
		echo "FAIL $test_name"
		failed_tests+=("$test_name")
		fail=$((fail + 1))
	fi
done
fi

if [[ -z "$binary" ]]; then
	echo "NOTE: no amiberry binary given; behavioral tests skipped"
else
	for test_name in ${BEHAVIORAL_TESTS[@]+"${BEHAVIORAL_TESTS[@]}"}; do
		if AMIBERRY_BIN="$binary" bash "tests/$test_name"; then
			echo "PASS $test_name"
			pass=$((pass + 1))
		else
			echo "FAIL $test_name"
			failed_tests+=("$test_name")
			fail=$((fail + 1))
		fi
	done
fi

echo "----"
echo "passed: $pass, failed: $fail"
if ((fail > 0)); then
	printf 'failed: %s\n' ${failed_tests[@]+"${failed_tests[@]}"}
	exit 1
fi
