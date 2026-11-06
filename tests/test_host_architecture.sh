#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 tests/host_architecture_test.py
