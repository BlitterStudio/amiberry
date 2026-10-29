#!/bin/sh
# Behavioral test of --settings-schema: the full export is valid JSON with
# every option, preset and resolution; an option list naming an unknown
# option is refused; and the copy the Android launcher bundles
# (android/app/src/main/settings/settings_schema.json) matches what this
# build generates, so a layer change cannot leave it stale.
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

"$AMIBERRY_BIN" --settings-schema > "$work/full.json"
"$python" - "$work/full.json" <<'PY'
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
assert all(c["values"].get("cpu_model") for p in schema["presets"] for c in p["configs"]), "preset values"
assert all(c["rom_sets"] for p in schema["presets"] for c in p["configs"]), "every configuration can be built"
rom_ids = {r["id"] for r in schema["roms"]}
assert all(any(i in rom_ids for i in req["ids"])
           for p in schema["presets"] for c in p["configs"] for s in c["rom_sets"] for req in s), "rom table"
for table in ("resolutions", "memory_resolutions"):
    res = schema[table]
    assert len(res["rows"]) > 0 and all(
        len(r[0]) == len(res["inputs"]) and len(r[1]) == len(res["inputs"]) and len(r[2]) == len(res["free"])
        and all(len(m) == len(f["values"]) for m, f in zip(r[2], res["free"]))
        for r in res["rows"]), table
    combinations = 1
    for count in (len(set(r[0][i] for r in res["rows"])) for i in range(len(res["inputs"]))):
        combinations *= count
    assert len({tuple(r[0]) for r in res["rows"]}) == len(res["rows"]) == combinations, table + " coverage"
PY

printf 'cpu_model\nnot_an_option\n' > "$work/bad.txt"
if "$AMIBERRY_BIN" --settings-schema="$work/bad.txt" > /dev/null 2>&1; then
	echo "an option list with an unknown option was accepted" >&2
	exit 1
fi

# The directory is an asset source of the Android app: nothing else in it.
if [ "$(ls -A "$settings")" != "settings_schema.json" ]; then
	echo "android/app/src/main/settings must hold only settings_schema.json" >&2
	exit 1
fi
"$AMIBERRY_BIN" --settings-schema="$root/android/scripts/settings_schema_options.txt" > "$work/android.json"
if ! cmp -s "$work/android.json" "$settings/settings_schema.json"; then
	echo "android/app/src/main/settings/settings_schema.json is stale;" >&2
	echo "regenerate it with android/scripts/update_settings_schema.sh" >&2
	diff "$settings/settings_schema.json" "$work/android.json" | head -20 >&2
	exit 1
fi
echo "settings schema test passed"
