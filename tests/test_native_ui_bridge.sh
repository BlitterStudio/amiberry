#!/bin/sh
# Behavioral test of the Swift bridge (apple/Shared/Bridge): runs its XCTest
# bundle (apple/Tests) inside the emulator with --native-ui-tests, where the
# built-in GUI would open, on an isolated home with no ROMs, so the tests
# exercise the real settings layer, actions and media APIs.
#
# Needs a macOS build configured with -DAMIBERRY_NATIVE_APPLE_UI=ON
# -DAMIBERRY_NATIVE_UI_TESTS=ON; with any other binary it is skipped.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> [NATIVE_UI_TESTS_BUNDLE=<bundle>] \
#        tests/test_native_ui_bridge.sh
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN is not set" >&2
	exit 1
fi
case "$AMIBERRY_BIN" in
	/*) ;;
	*) AMIBERRY_BIN="$PWD/$AMIBERRY_BIN" ;;
esac
if [ "$(uname -s)" != "Darwin" ] || ! grep -q -- "--native-ui-tests=<bundle>" "$AMIBERRY_BIN"; then
	echo "native UI bridge test skipped: the binary is not a macOS native UI test build"
	exit 0
fi

# The bundle is built next to the app bundle (build/Amiberry.app/Contents/MacOS/Amiberry).
bundle="${NATIVE_UI_TESTS_BUNDLE:-$(cd "$(dirname "$AMIBERRY_BIN")/../../.." && pwd)/AmiberryNativeUITests.bundle}"
if [ ! -f "$bundle" ]; then
	echo "native UI test bundle not found: $bundle" >&2
	exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
log="$work/tests.log"
status=0
# A scratch home: no user settings, ROMs or media; no window or audio device.
# perl's alarm bounds a hang (a test waiting on an action nobody runs).
HOME="$work" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
	perl -e 'alarm 300; exec @ARGV' "$AMIBERRY_BIN" --native-ui-tests="$bundle" > "$log" 2>&1 || status=$?
grep -E "^(Test Case .*(passed|failed)|Test Suite 'All tests'|.*error: )|Executed [0-9]+ tests" "$log" || true
if [ "$status" -ne 0 ]; then
	echo "native UI bridge tests failed (exit status $status)" >&2
	tail -40 "$log" >&2
	exit 1
fi
if ! grep -q "Test Suite 'All tests' passed" "$log"; then
	echo "native UI bridge tests did not report a passing run" >&2
	tail -40 "$log" >&2
	exit 1
fi
echo "native UI bridge test passed"
