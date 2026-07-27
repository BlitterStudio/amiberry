#!/usr/bin/env bash
set -euo pipefail

cxx="${CXX:-c++}"
out="${TMPDIR:-/tmp}/input_hotplug_identity_test.$$"
trap 'rm -f "$out"' EXIT

"$cxx" -std=c++17 -Wall -Wextra -Werror \
	-Isrc/osdep \
	-o "$out" \
	tests/input_hotplug_identity_test.cpp
"$out"

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
cfgfile = Path("src/cfgfile.cpp").read_text()
inputdevice = Path("src/inputdevice.cpp").read_text()


def fail(message: str) -> None:
	print(message, file=sys.stderr)
	sys.exit(1)


def region_between(text: str, start_marker: str, end_marker: str) -> str:
	try:
		start = text.index(start_marker)
		end = text.index(end_marker, start)
	except ValueError as exc:
		fail(f"Could not find source marker: {exc}")
	return text[start:end]


handler = region_between(
	amiberry,
	"void handle_joy_device_event(",
	"static void handle_controller_button_event",
)
if "inputdevice_devicechange(&changed_prefs)" not in handler:
	fail("Joystick hotplug must synchronize changed_prefs and currprefs")
if "inputdevice_devicechange(&currprefs)" in handler:
	fail("Runtime hotplug must not bypass the pending preference state")
if "import_joysticks()" in handler:
	fail("Joystick hotplug must not perform a second SDL re-enumeration")

devicechange = region_between(
	inputdevice,
	"bool inputdevice_devicechange (struct uae_prefs *prefs)",
	"#ifdef AMIBERRY\n// Re-enumerate mouse devices",
)
cache = devicechange.find("amiberry_cache_joystick_custom_mappings();")
close = devicechange.find("idev[IDTYPE_JOYSTICK].close();")
init = devicechange.find("idev[IDTYPE_JOYSTICK].init();")
restore = devicechange.find("amiberry_restore_joystick_custom_mappings();")
rematch = devicechange.find("matchdevices_all(prefs);")
if not (0 <= cache < close < init < restore < rematch):
	fail("Custom mappings must bracket SDL joystick re-enumeration before port rematching")
if "jportscustom[portnum] = -1;" not in devicechange:
	fail("A reinserted device must clear the custom marker for its port, not its device index")
if "unplugged_ports |= inputdevice_store_unplugged_port" not in devicechange:
	fail("Removed controller bindings must be tracked by port")
if """if (unplugged_ports & (1U << i)) {
			_tcsncpy(prefs->jports[i].idc.name, jports_name[i], MAX_JPORT_NAME - 1);
			_tcsncpy(prefs->jports[i].idc.configname, jports_configname[i], MAX_JPORT_CONFIG - 1);""" not in devicechange:
	fail("Pending preferences must retain the configured device identity while it is unplugged")
if """const int configured_joystick = cfgfile_get_joystick_index(jp);
		if (configured_joystick >= 0) {
			_sntprintf(tmp2, sizeof tmp2, _T("joy%d"), configured_joystick);""" not in cfgfile:
	fail("Configuration saves must prefer the retained joystick identity over a transient fallback")
if """if (!changed) {
		if (acc)
			inputdevice_acquire (TRUE);
		return false;
	}""" not in devicechange:
	fail("A no-op re-enumeration must reacquire input before returning")
PY
