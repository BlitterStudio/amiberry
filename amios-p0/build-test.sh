#!/usr/bin/env bash
# AmiOS 8.4.0 baseline build and dual-display diagnostic launcher.
# Does not install or enable an AmiOS Wayland exporter.
set -Eeuo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build-amios-p0"
PREFIX="$HOME/.local/opt/amios-amiberry-p0"
LOGDIR="$HOME/.local/state/amios-p0"
mkdir -p "$LOGDIR" "$PREFIX"
echo "AmiOS P0: Amiberry baseline build (no window export)"
echo "Source: $ROOT"
echo "Logs: $LOGDIR"
command -v cmake >/dev/null || { echo "Install cmake and Debian development dependencies first"; exit 2; }
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release 2>&1 | tee "$LOGDIR/configure.log"
cmake --build "$BUILD" -j"$(nproc)" 2>&1 | tee "$LOGDIR/build.log"
BIN="$(find "$BUILD" -type f -name amiberry -executable -print -quit)"
if [[ -z "$BIN" ]]; then echo "Build completed but executable was not found. Inspect $LOGDIR/build.log"; exit 3; fi
cp "$BIN" "$PREFIX/amiberry"
echo "Built: $PREFIX/amiberry"
echo "Test manually using your EXISTING .uae config:"
echo "  $PREFIX/amiberry -f /absolute/path/to/your.uae"
echo "Do not run this simultaneously with your working AmiOS68K session."
