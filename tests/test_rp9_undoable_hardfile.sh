#!/usr/bin/env bash
set -euo pipefail

source_file="src/osdep/amiberry_rp9.cpp"

if ! grep -F -q 'prepare_undoable_hardfile(media, path, extraction_directory' "$source_file"; then
	echo "RP9 hard drives must pass through undo preparation before mounting" >&2
	exit 1
fi

if ! awk '
	/prepare_undoable_hardfile\(/ { in_prepare = 1 }
	in_prepare && /!media.undo \|\| media.readonly/ { guarded = 1 }
	in_prepare && guarded && /directory_contains_path\(extraction_directory/ { temporary = 1 }
	in_prepare && temporary && /std::filesystem::copy_file\(source, destination/ { copied = 1; exit }
	in_prepare && /^}/ { exit }
	END { exit copied ? 0 : 1 }
' "$source_file"; then
	echo "Writable persistent RP9 hardfiles with undo enabled must use a temporary copy" >&2
	exit 1
fi
