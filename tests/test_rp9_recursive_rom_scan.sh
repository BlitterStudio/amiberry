#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

if ! awk '
	/int rp9_register_rom_directory\(/ { in_scan = 1 }
	in_scan && /recursive_directory_iterator/ { recursive = 1 }
	in_scan && /directory_options::skip_permission_denied/ { permissions = 1 }
	in_scan && /disable_recursion_pending\(\)/ { hidden = 1 }
	in_scan && /^}/ { exit }
	END { exit recursive && permissions && hidden ? 0 : 1 }
' "$source_file"; then
	echo "RP9 ROM registration must recursively scan accessible, non-hidden subdirectories" >&2
	exit 1
fi

if ! awk '
	/int rp9_register_rom_directory\(/ { in_scan = 1 }
	in_scan && /key_files\.push_back/ { collects_keys = 1 }
	in_scan && /std::sort\(key_files\.begin\(\), key_files\.end\(\)\)/ { sorts_keys = 1 }
	in_scan && /addkeyfile\(key_file\.string\(\)\.c_str\(\)\)/ { loads_keys = 1 }
	in_scan && /std::sort\(candidates\.begin\(\), candidates\.end\(\)\)/ { sorts_roms = 1 }
	in_scan && /^}/ { exit }
	END { exit collects_keys && sorts_keys && loads_keys && sorts_roms ? 0 : 1 }
' "$source_file"; then
	echo "RP9 recursive ROM registration must load keys before deterministically scanning ROM candidates" >&2
	exit 1
fi
