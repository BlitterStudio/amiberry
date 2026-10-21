#!/bin/sh
# Compiles amiberry_dbus.cpp the way a USE_DBUS=ON build does. USE_DBUS is
# off in the default configuration on most hosts, so a broken DBus handler
# would otherwise only surface in the FreeBSD CI job. Skips when no D-Bus
# development package is installed.
set -eu

cd "$(dirname "$0")/.."

if ! command -v pkg-config > /dev/null 2>&1 || ! pkg-config dbus-1 > /dev/null 2>&1; then
	echo "SKIP test_dbus_compile.sh (no dbus-1 development package)"
	exit 0
fi

out="${TMPDIR:-/tmp}/amiberry_dbus_compile.$$"
trap 'rm -rf "$out"' EXIT
mkdir -p "$out"

# shellcheck disable=SC2046
${CXX:-c++} -std=c++17 -fsyntax-only \
	-DUSE_DBUS -DAMIBERRY $(pkg-config --cflags dbus-3 2>/dev/null || pkg-config --cflags dbus-1) \
	$(pkg-config --cflags sdl3) \
	-Isrc -Isrc/include -Isrc/osdep -Isrc/threaddep \
	-o "$out/amiberry_dbus.o" src/osdep/amiberry_dbus.cpp

echo "DBus handler compiles with USE_DBUS"
