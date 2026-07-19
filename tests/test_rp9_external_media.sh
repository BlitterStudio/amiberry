#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

if grep -F -q 'RP9 media uses a disallowed external host path' "$source_file"; then
	echo "RP9 Absolute/External media roots must not be rejected unconditionally" >&2
	exit 1
fi

if ! awk '
	/std::filesystem::path resolve_media_path\(/ { in_resolve = 1 }
	in_resolve && /root == "absolute" \|\| root == "external"/ { in_external = 1 }
	in_external && /std::filesystem::path path\(media\.path\)/ { path = 1 }
	in_external && /!path\.is_absolute\(\)/ { absolute = 1 }
	in_external && /std::filesystem::status\(path, error\)/ { status = 1 }
	in_external && /std::filesystem::is_regular_file\(status\)/ { regular = 1 }
	in_external && /std::filesystem::is_directory\(status\)/ { directory = 1 }
	in_external && /return resolved;/ { returned = 1; exit }
	in_resolve && /^}/ { exit }
	END { exit path && absolute && status && regular && directory && returned ? 0 : 1 }
' "$source_file"; then
	echo "RP9 external media must resolve existing absolute regular files and directories" >&2
	exit 1
fi
