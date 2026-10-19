#!/bin/sh
# AppImage mounts cannot host Amiberry's marker-based portable mode. The
# runtime-provided sibling .home directory is the writable alternative, even
# when a portable marker is present alongside the image.
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN is required" >&2
	exit 1
fi

case "$(uname -s)" in
	Linux) ;;
	*)
		echo "AppImage portable mode test skipped on this platform"
		exit 0
		;;
esac

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cp "$AMIBERRY_BIN" "$work/Amiberry-test.AppImage"
chmod +x "$work/Amiberry-test.AppImage"
export HOME="$work/home"
export XDG_CONFIG_HOME="$work/config"
export XDG_DATA_HOME="$work/data"
export APPIMAGE="$work/Amiberry-test.AppImage"
unset AMIBERRY_HOME_DIR AMIBERRY_CONFIG_DIR AMIBERRY_DATA_DIR AMIBERRY_PLUGINS_DIR
touch "$work/amiberry.portable"

line_value()
{
	grep -m 1 "^$1=" "$2" | cut -d= -f2- | tr -d '\r'
}

(
	cd "$work"
	"$work/Amiberry-test.AppImage" --dump-paths
) > "$work/paths.txt"

[ "$(line_value portable_mode "$work/paths.txt")" = "0" ] \
	|| { echo "AppImage unexpectedly entered marker-based portable mode" >&2; exit 1; }
[ "$(line_value portable_toggle_available "$work/paths.txt")" = "0" ] \
	|| { echo "AppImage portable toggle remained available" >&2; exit 1; }
case "$(line_value portable_toggle_unavailable_reason "$work/paths.txt")" in
	*".home"*) ;;
	*) echo "AppImage portable guidance did not mention the sibling .home convention" >&2; exit 1 ;;
esac
[ -e "$work/amiberry.portable" ] \
	|| { echo "AppImage portable marker was unexpectedly removed" >&2; exit 1; }

unset APPIMAGE
for seed in controllers roms whdboot; do
	mkdir -p "$work/$seed"
done
"$work/Amiberry-test.AppImage" --dump-paths > "$work/installed-portable-paths.txt"
[ "$(line_value portable_mode "$work/installed-portable-paths.txt")" = "1" ] \
	|| { echo "writable installed portable mode ignored its marker" >&2; exit 1; }
for seed in controllers roms whdboot; do
	[ "$(line_value "${seed}_seed_source_0" "$work/installed-portable-paths.txt")" = "$work/$seed" ] \
		|| { echo "installed portable mode did not prefer executable-relative $seed seeds" >&2; exit 1; }
done

printf 'AppImage portable mode policy test passed\n'
