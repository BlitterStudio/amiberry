#!/usr/bin/env bash
set -euo pipefail

libretro_source="libretro/libretro.cpp"

if ! grep -F -q 'const std::string rom_path = "amiberry.rom_path=" + path;' "$libretro_source"; then
	echo "Libretro ROM directories must use the target-qualified config option" >&2
	exit 1
fi

if grep -F -q 'const std::string rom_path = "rom_path="' "$libretro_source"; then
	echo "Libretro must not emit the unrecognized bare rom_path config option" >&2
	exit 1
fi

if ! grep -F -q 'const std::string primary_rom_path = !system_dir.empty() ? system_dir : save_dir;' "$libretro_source" ||
	! grep -F -q 'push_rom_path(primary_rom_path);' "$libretro_source" ||
	! grep -F -q 'if (rom_path_value != primary_rom_path)' "$libretro_source"; then
	echo "Libretro must keep the frontend system directory and legacy Kickstarts directory in the ROM multipath" >&2
	exit 1
fi

if ! grep -F -q 'deferred_whdload_rom_paths.emplace_back(rom_path);' "$libretro_source"; then
	echo "Libretro must retain ROM directories for reapplication after WHDLoad autoload" >&2
	exit 1
fi

autoload_line=$(grep -n -F 'safe_strdup("--autoload");' "$libretro_source" | head -1 | cut -d: -f1)
whdload_rom_reapply_line=$(grep -n -F 'for (const auto& option : deferred_whdload_rom_paths)' "$libretro_source" | head -1 | cut -d: -f1)
if [ -z "$autoload_line" ] || [ -z "$whdload_rom_reapply_line" ] ||
	[ "$whdload_rom_reapply_line" -le "$autoload_line" ]; then
	echo "Libretro must reapply ROM directories after WHDLoad autoload resets preferences" >&2
	exit 1
fi
