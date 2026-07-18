#!/usr/bin/env bash
set -euo pipefail

main_source="src/main.cpp"
libretro_source="libretro/libretro.cpp"
rp9_source="src/osdep/amiberry_rp9.cpp"

if ! grep -F -q 'rp9_register_rom_override(path)' "$main_source"; then
	echo "Command-line -r overrides must be registered for RP9 validation" >&2
	exit 1
fi

prescan_line=$(grep -n -F 'register_cmdline_rom_overrides(argc, argv);' "$main_source" | head -1 | cut -d: -f1)
host_autoload_line=$(grep -n -F '_T("--autoload")' "$main_source" | head -1 | cut -d: -f1)
if [ -z "$prescan_line" ] || [ -z "$host_autoload_line" ] || [ "$prescan_line" -ge "$host_autoload_line" ]; then
	echo "All host -r overrides must be registered before RP9 autoload validation" >&2
	exit 1
fi

if ! grep -F -q 'romlist_add(filename, rom);' "$rp9_source"; then
	echo "RP9 ROM overrides must be published to the detected ROM list" >&2
	exit 1
fi

early_override_line=$(grep -n -F 'safe_strdup(kick_path);' "$libretro_source" | head -1 | cut -d: -f1)
autoload_line=$(grep -n -F 'safe_strdup("--autoload");' "$libretro_source" | head -1 | cut -d: -f1)
late_override_line=$(grep -n -F 'safe_strdup(deferred_rp9_kickstart.c_str());' "$libretro_source" | head -1 | cut -d: -f1)

if [ -z "$early_override_line" ] || [ -z "$autoload_line" ] || [ -z "$late_override_line" ] ||
	[ "$early_override_line" -ge "$autoload_line" ] || [ "$late_override_line" -le "$autoload_line" ]; then
	echo "Libretro RP9 Kickstart overrides must register before autoload and reapply afterward" >&2
	exit 1
fi
