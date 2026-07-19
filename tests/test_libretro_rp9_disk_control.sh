#!/usr/bin/env bash
set -euo pipefail

rp9_source="src/osdep/amiberry_rp9.cpp"
rp9_header="src/osdep/amiberry_rp9.h"
libretro_source="libretro/libretro.cpp"

for getter in rp9_get_loaded_floppy_paths rp9_get_loaded_cd_paths
do
	if ! grep -F -q "const std::vector<std::string>& $getter()" "$rp9_header" ||
		! grep -F -q "const std::vector<std::string>& $getter()" "$rp9_source"; then
		echo "RP9 must publish resolved removable-media paths for libretro disk control" >&2
		exit 1
	fi
done

if ! grep -F -q 'floppy_paths.emplace_back(path_string);' "$rp9_source" ||
	! grep -F -q 'loaded_floppy_paths = floppy_paths;' "$rp9_source"; then
	echo "RP9 must retain every resolved manifest floppy in priority order" >&2
	exit 1
fi

if ! awk '
	/bool apply_media\(/ { in_media = 1 }
	in_media && /apply_boot\(prefs, manifest, device_number\)/ { boot_applied = 1 }
	in_media && boot_applied && /floppy_paths\.emplace_back\(prefs->floppyslots\[0\]\.df\)/ { boot_published = 1 }
	in_media && /std::vector<const rp9::Media\*> ordered_media/ { exit }
	END { exit boot_applied && boot_published ? 0 : 1 }
' "$rp9_source"; then
	echo "RP9 must publish an attached boot ADF before manifest floppies" >&2
	exit 1
fi

if ! awk '
	/static void sync_rp9_disk_control_media\(\)/ { in_sync = 1 }
	in_sync && /rp9_get_loaded_floppy_paths\(\)/ { floppies = 1 }
	in_sync && /rp9_get_loaded_cd_paths\(\)/ { cds = 1 }
	in_sync && /disk_images\.push_back/ { publishes = 1 }
	in_sync && /content_is_cd = use_cd_paths/ { media_type = 1 }
	in_sync && /^}/ { exit }
	END { exit floppies && cds && publishes && media_type ? 0 : 1 }
' "$libretro_source"; then
	echo "Libretro must replace RP9 package entries with resolved floppy or CD media" >&2
	exit 1
fi

if ! awk '
	/if \(!package_path\.empty\(\)\)/ { in_package = 1 }
	in_package && /if \(!is_rp9\)/ { excludes_rp9 = 1 }
	in_package && excludes_rp9 && /disk_images\.push_back\(image\)/ { guarded_publish = 1 }
	in_package && /return true;/ { exit }
	END { exit excludes_rp9 && guarded_publish ? 0 : 1 }
' "$libretro_source"; then
	echo "Libretro must not expose the RP9 archive as an insertable disk image" >&2
	exit 1
fi

if ! awk '
	/void retro_run\(void\)/ { in_run = 1 }
	in_run && /co_switch\(core_fiber\)/ { started = 1 }
	in_run && started && /sync_rp9_disk_control_media\(\)/ { synced = 1 }
	in_run && /core_started = true/ { exit }
	END { exit started && synced ? 0 : 1 }
' "$libretro_source"; then
	echo "Libretro must publish RP9 media after manifest parsing completes" >&2
	exit 1
fi
