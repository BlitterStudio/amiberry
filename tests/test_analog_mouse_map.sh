#!/usr/bin/env bash
set -euo pipefail

# Contract for the analog-stick mouse map (joyportXmousemap):
# - Axis routing must key off the joyport the controller is assigned to,
#   not the di_joystick[] device index (they differ in the default Android
#   arrangement: port 0 = mouse, port 1 = joy0/device 0).
# - Stick movement must feed the mouse device bound to the Amiga mouse
#   (port 0), so the pointer moves even though the pad sits on port 1.
# - Shoulder buttons must become the mouse click events while the map is on.

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

amiberry_input = Path("src/osdep/amiberry_input.cpp").read_text()
cfgfile = Path("src/cfgfile.cpp").read_text()


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


helpers = region_between(
	amiberry_input,
	"static int assigned_joyport(",
	"static bool invert_axis(",
)
if "jsem_isjoy(port, &currprefs) == device_index" not in helpers:
	fail("The assigned joyport must be resolved through jsem_isjoy, not assumed to equal the device index")
if "jsem_ismouse(0, &currprefs)" not in helpers:
	fail("Stick mouse movement must target the mouse device assigned to the Amiga mouse (port 0)")

for start_marker, end_marker, axis in (
	("void read_controller_axis(", "void read_joystick_button_single(", "axis"),
	("void read_joystick_axis(", "void read_joystick_hat(", "did_axis"),
):
	handler = region_between(amiberry_input, start_marker, end_marker)
	if "const int port = assigned_joyport(id);" not in handler:
		fail(f"{start_marker[:-2]} must resolve the assigned joyport before checking the mouse map")
	if f"currprefs.jports[port].mousemap > 0" not in handler:
		fail(f"{start_marker[:-2]} must check the assigned port's mousemap, not jports[id]")
	if "currprefs.jports[id].mousemap" in handler:
		fail(f"{start_marker[:-2]} must not conflate the device index with the joyport index")
	if "setmousestate(mousemap_mouse_device()" not in handler:
		fail(f"{start_marker[:-2]} must route stick movement through the resolved port-0 mouse device")

defaults = region_between(
	amiberry_input,
	"int input_get_default_joystick(",
	"int input_get_default_joystick_analog(",
)
if "currprefs.jports[port].mousemap > 0" not in defaults:
	fail("Shoulder buttons must keep switching to mouse click events while the mouse map is on")
if "currprefs.jports[port].mousemap == 0" not in defaults:
	fail("Shoulder buttons must not stay as Space/Return defaults while the mouse map is on")

for key in ("joyport0mousemap", "joyport1mousemap"):
	if key not in cfgfile:
		fail(f"cfgfile must keep parsing the {key} configuration key")
PY

echo "analog mouse map contract OK"
