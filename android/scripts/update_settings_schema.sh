#!/bin/sh
# Regenerates android/app/src/main/settings/settings_schema.json from a
# desktop Amiberry build: the options the launcher writes (listed in
# settings_schema_options.txt), the machine presets and the CPU/chipset
# resolutions of the shared settings layer. Run it after changing the layer
# or the list; tests/test_settings_schema.sh fails while the file is stale.
#
# Usage: android/scripts/update_settings_schema.sh <path-to-amiberry>
set -eu

if [ $# -ne 1 ]; then
	echo "usage: $0 <path-to-amiberry>" >&2
	exit 1
fi
dir="$(cd "$(dirname "$0")/../app/src/main/settings" && pwd)"
# A scratch home: the schema comes from the built-in defaults, never from
# the user's amiberry.conf or ROMs.
home="$(mktemp -d)"
trap 'rm -rf "$home"' EXIT
HOME="$home" "$1" --settings-schema="$dir/settings_schema_options.txt" > "$dir/settings_schema.json.tmp"
mv "$dir/settings_schema.json.tmp" "$dir/settings_schema.json"
echo "wrote $dir/settings_schema.json"
