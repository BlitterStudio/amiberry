#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/imgui/quickstart.cpp"

if ! awk '
	/ConsumeFileDialogResultKey\("QUICKSTART"/ { in_result = 1 }
	in_result && /!qs_pending_rp9 && !filePath\.empty\(\) && !rp9_get_loaded_path\(\)\.empty\(\)/ { guarded = 1 }
	in_result && guarded && /reset_rp9_to_quickstart_defaults\(\);/ { reset = 1 }
	in_result && /if \(qs_pending_floppy_drive/ { media = 1; exit }
	END { exit guarded && reset && media ? 0 : 1 }
' "$source_file"; then
	echo "Quickstart must reset active RP9 preferences before applying non-RP9 media" >&2
	exit 1
fi

if awk '
	/ConsumeFileDialogResultKey\("QUICKSTART"/ { in_result = 1 }
	in_result && /rp9_clear_loaded_path\(\);/ { found = 1; exit }
	in_result && /qs_pending_floppy_drive = -1;/ { exit }
	END { exit found ? 0 : 1 }
' "$source_file"; then
	echo "Quickstart must not clear RP9 state without resetting its preferences" >&2
	exit 1
fi
