#!/usr/bin/env bash
set -euo pipefail
source tests/zz9000_sdk_build.sh
zz9000_sdk_compile /tmp/zz9000_sdk_console_test tests/zz9000_sdk_console_test.cpp
/tmp/zz9000_sdk_console_test
