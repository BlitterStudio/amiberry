#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

assignment_line=$(grep -n -F 'loaded_cd_paths = cd_paths;' "$source_file" | head -1 | cut -d: -f1)
cleanup_line=$(grep -n -F 'cleanup_unused_temporary_directories(prefs);' "$source_file" | tail -1 | cut -d: -f1)
if [ -z "$assignment_line" ] || [ -z "$cleanup_line" ] || [ "$assignment_line" -ge "$cleanup_line" ]; then
	echo "RP9 CD swap paths must be retained before temporary-directory cleanup" >&2
	exit 1
fi

if ! grep -F -q 'loaded_cd_paths_reference_directory(*directory)' "$source_file"; then
	echo "RP9 temporary-directory cleanup must retain swap-only CD paths" >&2
	exit 1
fi

if ! awk '
	/void rp9_clear_loaded_path\(/ { in_clear = 1 }
	in_clear && /loaded_cd_paths\.clear\(\);/ { cleared = 1; exit }
	in_clear && /^}/ { exit }
	END { exit cleared ? 0 : 1 }
' "$source_file"; then
	echo "RP9 CD swap retention must end when the loaded package is cleared" >&2
	exit 1
fi
