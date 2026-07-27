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
amiberry_input = Path("src/osdep/amiberry_input.cpp").read_text()
cfgfile = Path("src/cfgfile.cpp").read_text()
inputdevice = Path("src/inputdevice.cpp").read_text()
input_panel = Path("src/osdep/imgui/input.cpp").read_text()


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
preserve = devicechange.find("amiberry_preserve_port_joystick_custom_mapping(")
close = devicechange.find("idev[IDTYPE_JOYSTICK].close();")
init = devicechange.find("idev[IDTYPE_JOYSTICK].init();")
restore = devicechange.find("amiberry_restore_joystick_custom_mappings();")
rematch = devicechange.find("matchdevices_all(prefs);")
if not (0 <= preserve < cache < close < init < restore < rematch):
	fail("Custom mappings must bracket SDL joystick re-enumeration before port rematching")
if "jportscustom[portnum] = -1;" not in devicechange:
	fail("A reinserted device must clear the custom marker for its port, not its device index")
if "unplugged_ports |= inputdevice_store_unplugged_port" not in devicechange:
	fail("Removed controller bindings must be tracked by port")
if "uae_u32 unplugged_ports = inputdevice_get_unplugged_ports();" not in devicechange:
	fail("Ports already awaiting reconnection must survive later hotplug passes")
if """if (found) {
				if (unplugged_ports & (1U << i))
					inputdevice_forget_unplugged_device(i);
				unplugged_ports &= ~(1U << i);
			}""" not in devicechange:
	fail("A controller rematched after SDL index compaction must keep its validated identity")
if "identity_bound = amiberry_resolve_port_joystick(i, resolved_joystick);" not in devicechange:
	fail("Port rematching must resolve identity-bound controllers before friendly-name fallback")
if """if (identity_bound) {
					inputdevice_store_unplugged_binding(""" not in devicechange:
	fail("An identity-bound controller with no safe match must remain pending")
if """if (unplugged_ports & (1U << i)) {
			_tcsncpy(prefs->jports[i].idc.name, jports_name[i], MAX_JPORT_NAME - 1);
			_tcsncpy(prefs->jports[i].idc.configname, jports_configname[i], MAX_JPORT_CONFIG - 1);""" not in devicechange:
	fail("Pending preferences must retain the configured device identity while it is unplugged")
save_ports = region_between(
	cfgfile,
	"for (i = 0; i < MAX_JPORTS; i++) {",
	"for (i = 0; i < MAX_JPORTS_CUSTOM; i++) {",
)
if "configured_joystick" in save_ports:
	fail("Saved joyport values must follow the active port selection, not stale identity metadata")
fixjport = region_between(
	inputdevice,
	"static bool fixjport(",
	"static void inputdevice_get_previous_joy",
)
if "if (vv == JPORT_NONE)" in fixjport:
	fail("Explicitly selecting None must clear the previous device identity")
port_selection = region_between(
	input_panel,
	"static void set_port_input_device(",
	"const std::vector<InputDeviceOption>& get_input_device_options()",
)
if "amiberry_clear_port_joystick_custom_mapping(port_idx);" not in port_selection:
	fail("Explicit port changes must discard the old controller mapping snapshot")
if "inputdevice_forget_unplugged_device(port_idx);" not in port_selection:
	fail("Explicit port changes must cancel pending reconnection of the old controller")
if "inputdevice_validate_jports(&changed_prefs, port_idx, nullptr);" not in port_selection:
	fail("Explicit port changes must refresh or clear saved device identity metadata")
if input_panel.count("set_port_input_device(port_idx, selected_idx);") != 2:
	fail("Primary and parallel port selectors must share the identity-safe update path")
if "const int joy_index = amiberry_get_joystick_index(jp->idc.configname);" not in amiberry_input:
	fail("Custom mapping loads must use the same bounded joystick index parser as saves")
if """if (!amiberry_get_port_joystick_custom_mapping(
			i, jp->idc.name, jp->idc.configname, mapping))""" not in cfgfile:
	fail("Configuration saves must use a preserved mapping for an unplugged controller")
if "di_joystick[joy_index]" in cfgfile:
	fail("Configuration saves must not dereference a stale or compacted joystick slot")
apply_mapping = devicechange.find("amiberry_apply_port_joystick_custom_mapping(")
clear_mapping = devicechange.find("amiberry_clear_port_joystick_custom_mapping(i);")
validate_port = devicechange.find("inputdevice_validate_jports(prefs, i, fixedports);")
if not (0 <= apply_mapping < validate_port < clear_mapping):
	fail("A rematched controller must receive its deferred mapping before the rematch is finalized and the snapshot is cleared")
if """controller_mapping mapping{};
			amiberry_get_port_joystick_custom_mapping(
				i, jp->idc.name, jp->idc.configname, mapping);""" not in amiberry_input:
	fail("Custom mapping loads must start from the durable per-port representation")
if "amiberry_set_port_joystick_custom_mapping(" not in amiberry_input:
	fail("Custom mapping loads must persist without requiring a live SDL device")
if """struct preserved_port_joystick_custom_mapping
{
	struct inputdevconfig idc;
	amiberry_joystick_identity identity;""" not in amiberry_input:
	fail("Deferred custom mappings must retain their physical controller identity")
if "preserved.identity = get_joystick_identity(did);" not in amiberry_input:
	fail("Live per-port mapping snapshots must capture physical controller identity")
if "return ambiguous ? -1 : best_index;" not in amiberry_input:
	fail("Indistinguishable controller identities must not be assigned arbitrarily")
apply_custom_mapping = region_between(
	amiberry_input,
	"bool amiberry_apply_port_joystick_custom_mapping(",
	"void amiberry_cache_joystick_custom_mappings()",
)
if "amiberry_resolve_port_joystick(portnum, resolved_joystick)" not in apply_custom_mapping:
	fail("Deferred mappings must be applied only to the resolved physical controller")
custom_loader = region_between(
	amiberry_input,
	"bool load_custom_options(",
	"static void close_joystick()",
)
if "di_joystick[" in custom_loader:
	fail("Custom mapping loads must not write into an unenumerated SDL device slot")
if "copy_custom_mapping(preserved.mapping, did.mapping);" not in amiberry_input:
	fail("Deferred custom mappings must be applied when the configured controller returns")
cfgload = region_between(
	cfgfile,
	"static int cfgfile_load_2",
	"int cfgfile_load (",
)
open_failure = cfgload.find("if (! fh)")
clear_deferred = cfgload.find("amiberry_clear_port_joystick_custom_mappings();")
parse_config = cfgload.find("while (cfg_fgets")
if not (0 <= open_failure < clear_deferred < parse_config):
	fail("A successfully opened host configuration must discard stale deferred mappings before parsing")
if """if (!changed) {
		if (acc)
			inputdevice_acquire (TRUE);
		return false;
	}""" not in devicechange:
	fail("A no-op re-enumeration must reacquire input before returning")
PY
