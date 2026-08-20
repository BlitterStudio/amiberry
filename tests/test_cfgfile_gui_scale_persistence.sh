#!/usr/bin/env bash
set -euo pipefail

if command -v python3 >/dev/null 2>&1; then
	PYTHON=python3
elif command -v python >/dev/null 2>&1; then
	PYTHON=python
elif command -v py >/dev/null 2>&1; then
	PYTHON="py -3"
else
	echo "python3, python, or py is required" >&2
	exit 1
fi

$PYTHON - <<'PY'
from pathlib import Path
import sys

amiberry = Path("src/osdep/amiberry.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


# gui_layout_scale must round-trip through amiberry.conf: save_amiberry_settings
# writes it and parse_amiberry_settings_line reads it back under the exact same
# key string. A key drift on either side silently drops the user's GUI-scale
# preference on the next save. Both sides must also stay float-typed
# (write_float_option / cfgfile_floatval) so fractional percents survive.
save_call = 'write_float_option("gui_layout_scale"'
parse_call = 'cfgfile_floatval(option, value, "gui_layout_scale"'

if amiberry.count(save_call) != 1:
	fail(f"Expected exactly one save-side write for 'gui_layout_scale', found {amiberry.count(save_call)}")

if amiberry.count(parse_call) != 1:
	fail(f"Expected exactly one parse-side read for 'gui_layout_scale', found {amiberry.count(parse_call)}")
PY
