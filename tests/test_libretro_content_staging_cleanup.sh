#!/usr/bin/env bash
set -euo pipefail

source_file="libretro/libretro.cpp"

if grep -F -q 'filestream_delete(directory.c_str());' "$source_file"; then
	echo "Libretro staging directories must not use the frontend file-removal callback" >&2
	exit 1
fi

if ! awk '
	/static int remove_empty_content_directory\(/ { in_remove = 1 }
	in_remove && /_rmdir\(directory\.c_str\(\)\)/ { windows = 1 }
	in_remove && /::rmdir\(directory\.c_str\(\)\)/ { posix = 1 }
	in_remove && /retro_vfs_file_remove_impl\(directory\.c_str\(\)\)/ { fallback = 1 }
	in_remove && /^}/ { exit }
	END { exit windows && posix && fallback ? 0 : 1 }
' "$source_file"; then
	echo "Libretro staging cleanup must remove empty directories on Windows, POSIX, and virtual paths" >&2
	exit 1
fi

if ! grep -F -q 'remove_empty_content_directory(directory);' "$source_file"; then
	echo "Libretro staging cleanup must use the directory-specific removal helper" >&2
	exit 1
fi
