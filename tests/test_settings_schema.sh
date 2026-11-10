#!/bin/sh
# Behavioral test of --settings-schema: the export of the Android
# launcher's option list is valid JSON with its options, every preset,
# resolution, availability and transition table; an option list naming an
# unknown option is refused; and the copy the Android launcher bundles
# (android/app/src/main/settings/settings_schema.json) matches what this
# build generates, so a layer change cannot leave it stale. The export runs
# once (it takes minutes) and every check reads it. The export itself fails
# when an availability or transition answer changes with an option its
# table does not have as an input.
#
# Every preset configuration's compatibility levels are the machine: a
# configuration file of a level's values loads as the configuration
# --apply-preset builds at that level, which is how the Android launcher
# starts a preset.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_settings_schema.sh
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN is not set" >&2
	exit 1
fi
python=""
for candidate in python3 python; do
	if command -v "$candidate" > /dev/null 2>&1 && "$candidate" -c '' > /dev/null 2>&1; then
		python="$candidate"
		break
	fi
done
if [ -z "$python" ]; then
	echo "python3 is required" >&2
	exit 1
fi

root="$(cd "$(dirname "$0")/.." && pwd)"
settings="$root/android/app/src/main/settings"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
export HOME="$work"

"$AMIBERRY_BIN" --settings-schema="$root/android/scripts/settings_schema_options.txt" > "$work/android.json"
"$python" - "$work/android.json" <<'PY'
import json, sys
schema = json.load(open(sys.argv[1]))
assert schema["schema_version"] == 1
ids = [o["id"] for o in schema["options"]]
assert len(ids) == len(set(ids)) and "cpu_model" in ids, "options"
for option in schema["options"]:
    assert option["type"] in ("bool", "int", "real", "choice", "path", "string", "hotkey"), option
    assert option["apply"] in ("live", "reset", "restart"), option
    # Host device lists (displays, audio devices) are left empty.
    if option["type"] == "choice":
        assert isinstance(option["choices"], list), option["id"]
assert schema["presets"] and all(p["configs"] for p in schema["presets"]), "presets"
for p in schema["presets"]:
    for c in p["configs"]:
        assert len(c["levels"]) == p["compatibility_max"] + 1, (p["id"], c["label"])
        assert all(level for level in c["levels"]), (p["id"], c["label"], "an empty level")
ids_set = set(ids)
types = {o["id"]: o["type"] for o in schema["options"]}
level_keys = {k for p in schema["presets"] for c in p["configs"] for level in c["levels"] for k in level}
assert level_keys <= ids_set and not any(":" in k for k in level_keys), level_keys
# The only paths in the levels are NVRAM files, spelled relative to the
# content base so each installation keeps them in its own NVRAM directory.
level_paths = {(k, level[k]) for p in schema["presets"] for c in p["configs"] for level in c["levels"]
               for k in level if types[k] == "path"}
assert all(k == "flash_file" and v.startswith("./NVRAM/") for k, v in level_paths), level_paths
assert ("flash_file", "./NVRAM/cd32.nvr") in level_paths, level_paths
assert all(c["rom_sets"] for p in schema["presets"] for c in p["configs"]), "every configuration can be built"
rom_ids = {r["id"] for r in schema["roms"]}
assert all(any(i in rom_ids for i in req["ids"])
           for p in schema["presets"] for c in p["configs"] for s in c["rom_sets"] for req in s), "rom table"
for table in ("resolutions", "memory_resolutions", "sound_resolutions"):
    res = schema[table]
    assert len(res["rows"]) > 0 and all(
        len(r[0]) == len(res["inputs"]) and len(r[1]) == len(res["inputs"]) and len(r[2]) == len(res["free"])
        and all(len(m) == len(f["values"]) for m, f in zip(r[2], res["free"]))
        for r in res["rows"]), table
    combinations = 1
    for count in (len(set(r[0][i] for r in res["rows"])) for i in range(len(res["inputs"]))):
        combinations *= count
    assert len({tuple(r[0]) for r in res["rows"]}) == len(res["rows"]) == combinations, table + " coverage"

def state_count(inputs):
    count = 1
    for f in inputs:
        assert f["values"], f["id"]
        count *= len(f["values"])
    return count

for table in schema["availability"]:
    assert len(table["rows"]) == state_count(table["inputs"]), table["options"]
    for row in table["rows"]:
        assert len(row) == 1 + len(table["choices"]), table["options"]
        assert 0 <= row[0] < 1 << len(table["options"]), table["options"]
        assert all(0 <= m < 1 << len(c["values"]) for m, c in zip(row[1:], table["choices"])), table["options"]
available = {o for t in schema["availability"] for o in t["options"]}
assert {"cpu_compatible", "cachesize", "z3mem_size", "sound_channels", "floppy3soundvolume_empty"} <= available
# The chip and slow RAM sizes against the chipset extra and the JIT.
memory = [t for t in schema["availability"] if {c["id"] for c in t["choices"]} == {"chipmem_size", "bogomem_size"}]
assert len(memory) == 1 and "chipset_compatible" in [f["id"] for f in memory[0]["inputs"]], "memory availability"

def check_transitions(trans):
    columns = sum(len(a["values"]) for a in trans["actions"])
    assert len(trans["rows"]) == state_count(trans["inputs"]), "transition rows"
    assert all(len(r) == columns and all(-1 <= o < len(trans["outcomes"]) for o in r) for r in trans["rows"]), "transitions"
    for outcome in trans["outcomes"]:
        assert len(outcome) % 2 == 0 and all(0 <= i < len(trans["fields"]) for i in outcome[0::2]), outcome


check_transitions(schema["transitions"])
# One table per Zorro II fast RAM board: its MB and KB keys are one size,
# so setting either writes both.
boards = schema["memory_transitions"]
assert [t["fields"][0] for t in boards] == ["fastmem_size"] + [f"fastmem{n}_size" for n in (2, 3, 4)]
for board in boards:
    check_transitions(board)
    assert board["fields"][1] == board["fields"][0] + "_k"

# A configuration that needs a feature some build leaves out comes after
# every one that does not, so a front end that drops it keeps the others'
# configuration indices.
for p in schema["presets"]:
    needs = [bool(c.get("requires")) for c in p["configs"]]
    assert needs == sorted(needs), p["id"] + ": configurations with requirements come last"
ppc = {(p["id"], c["label"]) for p in schema["presets"] for c in p["configs"] if "ppc" in c.get("requires", [])}
assert ppc == {("a1200", "Blizzard PPC"), ("a4000", "CyberStorm PPC")}, ppc
assert {p["id"] for p in schema["presets"] if p.get("arcade")} == {"alg", "arcadia"}, "arcade presets"
android = {o["id"]: o["android_default"] for o in schema["options"] if "android_default" in o}
assert android.get("sound_frequency") == "48000" and android.get("sound_max_buff") == "8192", android
# The genlock sources a build without VIDEOGRAB leaves out are marked, so
# the Android launcher (no VIDEOGRAB) drops them from the desktop export.
genlock = {c["value"]: c.get("requires", []) for c in next(o for o in schema["options"] if o["id"] == "genlockmode")["choices"]}
assert [v for v, r in genlock.items() if not r] == ["none", "noise", "testcard", "image"], genlock
assert all(r == ["videograb"] for r in genlock.values() if r), genlock
assert schema["aros"] == {"rom_file": ":AROS", "main_file": "aros-rom.bin", "ext_file": "aros-ext.bin"}, schema["aros"]

# Host settings (amiberry.conf): the Android build's own, as a desktop
# export describes them. The update checker is left out (no self-update on
# Android) and the Android-only ones (ADPF) are carried.
host = schema["host_settings"]
host_ids = [h["id"] for h in host]
assert len(host_ids) == len(set(host_ids)), "host settings"
assert {"Quickstart", "shader", "input_default_mouse_speed", "gui_layout_scale",
        "default_controller1", "default_soundcard", "use_adpf"} <= set(host_ids), host_ids
folders = {"config_path", "controllers_path", "retroarch_config", "whdboot_path", "whdload_arch_path",
           "floppy_path", "harddrive_path", "cdrom_path", "logfile_path", "rom_path", "rp9_path",
           "floppy_sounds_dir", "saveimage_dir", "savestate_dir", "ripper_path", "inputrecordings_dir",
           "screenshot_dir", "nvram_dir", "plugins_dir", "video_dir", "themes_path", "shaders_path",
           "bezels_path", "base_content_path"}
assert not ({"update_check", "update_channel", "skipped_version", "macos_left_command_q",
             "default_onscreen_joystick", "default_vkbd_enabled", "default_vkbd_toggle",
             "default_width", "default_height", "default_fullscreen_mode"} | folders) & set(host_ids), host_ids
for setting in host:
    assert setting["type"] in ("bool", "int", "real", "choice", "path", "string", "hotkey"), setting
    assert setting["apply"] == "live", setting["id"]
    if setting["type"] == "choice":
        assert isinstance(setting["choices"], list), setting["id"]
host_by_id = {h["id"]: h for h in host}
# The folders group stays for the log switch the launcher does honour.
assert host_by_id["write_logfile"]["type"] == "bool" and host_by_id["write_logfile"]["group"] == "global_paths"
# The lists that name the host's devices, shaders and bezels travel empty.
for dynamic in ("default_controller1", "default_mouse1", "default_soundcard", "shader", "shader_rtg", "custom_bezel"):
    assert host_by_id[dynamic]["type"] == "choice" and host_by_id[dynamic]["choices"] == [], dynamic
# Defaults that differ on the Android build.
android_host = {h["id"]: h["android_default"] for h in host if "android_default" in h}
assert android_host.get("default_sound_frequency") == "48000", android_host
assert android_host.get("default_sound_buffer") == "8192", android_host
assert android_host.get("gui_layout_scale") == "70", android_host
assert host_by_id["gui_layout_scale"]["default"] == "100", host_by_id["gui_layout_scale"]
# A level a front end may show as a slider, with its unit.
assert host_by_id["default_joystick_deadzone"]["level"] is True, host_by_id["default_joystick_deadzone"]
assert host_by_id["default_joystick_deadzone"]["unit"] == "%", host_by_id["default_joystick_deadzone"]
assert host_by_id["use_adpf"]["type"] == "bool" and host_by_id["use_adpf"]["group"] == "global_advanced"
# A default's machine option (target_default_options' copy): every named
# option is one this export describes, and the defaults the launcher writes
# itself are the ones named.
sets = {h["id"]: h["sets_option"] for h in host if "sets_option" in h}
assert sets == {
    "default_controller1": "joyport1",
    "default_mouse1": "joyport0",
    "default_joystick_deadzone": "input.joystick_deadzone",
    "input_default_mouse_speed": "input.mouse_speed",
    "default_open_gui_key": "amiberry.open_gui",
    "default_quit_key": "amiberry.quit_amiberry",
    "default_ar_key": "amiberry.action_replay",
    "default_fullscreen_toggle_key": "amiberry.fullscreen_toggle",
    "default_retroarch_quit": "amiberry.use_retroarch_quit",
    "default_retroarch_menu": "amiberry.use_retroarch_menu",
    "default_retroarch_reset": "amiberry.use_retroarch_reset",
    "default_line_mode": "gfx_linemode",
    "default_horizontal_centering": "gfx_center_horizontal",
    "default_vertical_centering": "gfx_center_vertical",
    "default_scaling_method": "amiberry.scaling_method",
    "default_gfx_autoresolution": "gfx_autoresolution",
    "default_correct_aspect_ratio": "amiberry.gfx_correct_aspect",
    "default_auto_crop": "amiberry.gfx_auto_crop",
    "default_stereo_separation": "sound_stereo_separation",
    "default_sound_frequency": "sound_frequency",
    "default_sound_buffer": "sound_max_buff",
    "default_soundcard": "amiberry.soundcard",
    "shader": "amiberry.shader",
    "shader_rtg": "amiberry.shader_rtg",
    "use_bezel": "amiberry.use_bezel",
    "use_custom_bezel": "amiberry.use_custom_bezel",
    "custom_bezel": "amiberry.custom_bezel",
}, sets
# A default may name an option this export does not describe (the hotkeys,
# the bezel toggles): the launcher's machineOptionDefaults skips those
# itself; the pinned map above keeps a typo from slipping in.
assert all(o in ids_set or o in {"amiberry.open_gui", "amiberry.quit_amiberry",
           "amiberry.action_replay", "amiberry.fullscreen_toggle", "amiberry.soundcard",
           "amiberry.shader_rtg", "amiberry.use_custom_bezel", "amiberry.custom_bezel"}
           for o in sets.values()), "a named option is neither described nor one of the known unlisted ones"

# The per-platform visibility mask travels with every option and host
# setting: iOS has no serial backend or desktop window, Android never
# switches host display modes or traps its pointer, and the classic
# settings window's own settings are the desktop GUI's alone.
platforms = {"desktop", "macos", "ios", "android"}
def hidden_on(entry):
    return set(entry.get("hidden_on", []))
assert all(hidden_on(o) <= platforms for o in schema["options"] + host), "hidden_on names"
by_id = {o["id"]: o for o in schema["options"]}
assert hidden_on(by_id["magic_mouse"]) == {"ios", "android"}
assert hidden_on(by_id["magic_mousecursor"]) == {"ios", "android"}
assert hidden_on(by_id["gfx_monitorblankdelay"]) == {"android"}
assert hidden_on(by_id["show_leds_rtg"]) == {"android"}
assert not hidden_on(by_id["use_gui"]) and not hidden_on(by_id["gfx_vsync"])
gui_only = {"macos", "ios", "android"}
for id in ("Quickstart", "read_config_descriptions", "gui_joystick_control",
           "disable_shutdown_button", "default_quickstart_compatibility", "gui_theme",
           "gui_layout_scale"):
    assert hidden_on(host_by_id[id]) == gui_only, id
PY

printf 'cpu_model\nnot_an_option\n' > "$work/bad.txt"
if "$AMIBERRY_BIN" --settings-schema="$work/bad.txt" > /dev/null 2>&1; then
	echo "an option list with an unknown option was accepted" >&2
	exit 1
fi

# Each level of each preset configuration, written as a configuration file,
# loads as the machine --apply-preset builds. Both start from a loaded
# configuration (the empty file), as a launch does. Paths other than the
# NVRAM file are left out of the levels: the launcher supplies its own ROM
# files. What the layer has no option for is listed below.
#
# A level's NVRAM file is spelled relative to the content base, where the
# Android launcher (like the macOS, Windows and portable layouts) keeps the
# NVRAM directory. A Linux desktop install keeps it in its XDG data
# directory instead, so the check sets it in the content base first.
"$python" - "$AMIBERRY_BIN" "$work/android.json" "$work" <<'PY'
import json, os, subprocess, sys
binary, schema_path, work = sys.argv[1:4]
schema = json.load(open(schema_path))
types = {o["id"]: o["type"] for o in schema["options"]}
# Machine state that is not a layer option, so not part of the levels:
gaps = {
    # The CDTV's 64 KB SRAM card is an expansion board (a ROM-board line),
    # which the layer configures through its board API, not as an option.
    "cdtvsram_rom_file": {("cdtv", 1)},
    # The ALG boards' floppy data pull-up; the arcade boards are not offered
    # where the levels are used (the Android launcher).
    "floppydata_pullup": {("alg", 0)},
    # The A1000 has no slow RAM fitted, so its slow RAM timing is inert.
    "bogomem_options": {("a1000", c) for c in range(4)},
}
assert not set(gaps) & set(types), "a gap became a layer option: export it in the levels"
env = dict(os.environ, HOME=work)
paths = dict(line.split("=", 1) for line in subprocess.run(
    [binary, "--dump-paths"], capture_output=True, text=True, env=env, check=True).stdout.splitlines()
    if "=" in line)
os.makedirs(os.path.dirname(paths["amiberry_conf_file"]), exist_ok=True)
with open(paths["amiberry_conf_file"], "w") as f:
    f.write(f'nvram_dir={os.path.join(paths["relative_path_base"], "NVRAM")}\n')
empty = os.path.join(work, "empty.uae")
open(empty, "w").close()

def dump(args):
    result = subprocess.run([binary, "--config"] + args + ["--dump-config"],
                            capture_output=True, text=True, env=env)
    assert result.returncode == 0, (args, result.stderr)
    values = {}
    for line in result.stdout.splitlines():
        if line.startswith(";") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values.setdefault(key, []).append(value)
    return values

failures = []
count = 0
for preset in schema["presets"]:
    for config, c in enumerate(preset["configs"]):
        for level, values in enumerate(c["levels"]):
            path = os.path.join(work, "level.uae")
            with open(path, "w") as f:
                f.writelines(f"{key}={value}\n" for key, value in values.items())
            spec = f'{preset["id"]}:{config}:{level}'
            built = dump([empty, "--apply-preset", spec])
            loaded = dump([path])
            count += 1
            # gdb_port is not the machine but the GDB server's port, which a
            # preset's reset sets and a configuration load leaves out.
            differ = [k for k in sorted(set(built) | set(loaded)) if built.get(k) != loaded.get(k)
                      and k != "gdb_port" and (types.get(k) != "path" or k in values)
                      and (preset["id"], config) not in gaps.get(k, ())]
            if differ:
                failures.append(f"{spec}: " + ", ".join(f"{k} {built.get(k)} != {loaded.get(k)}" for k in differ))
assert not failures, "levels that do not load as their preset:\n" + "\n".join(failures)
print(f"{count} preset levels load as their preset")
PY

# The directory is an asset source of the Android app: nothing else in it.
if [ "$(ls -A "$settings")" != "settings_schema.json" ]; then
	echo "android/app/src/main/settings must hold only settings_schema.json" >&2
	exit 1
fi
if ! cmp -s "$work/android.json" "$settings/settings_schema.json"; then
	echo "android/app/src/main/settings/settings_schema.json is stale;" >&2
	echo "regenerate it with android/scripts/update_settings_schema.sh" >&2
	diff "$settings/settings_schema.json" "$work/android.json" | head -20 >&2
	exit 1
fi
echo "settings schema test passed"
