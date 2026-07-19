#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry.cpp"

if ! awk '
	/static void ensure_amiberry_user_directories\(\)/ { in_function = 1 }
	in_function && /ensure_directory_exists_if_missing\(rp9_path\);/ { found = 1; exit }
	in_function && /^}/ { exit }
	END { exit found ? 0 : 1 }
' "$source_file"; then
	echo "The configured RP9 content directory must be created during startup" >&2
	exit 1
fi
