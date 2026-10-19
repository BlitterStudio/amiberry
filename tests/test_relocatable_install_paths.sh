#!/bin/sh
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN is required" >&2
	exit 1
fi

case "$(uname -s)" in
	Linux|FreeBSD) ;;
	*)
		echo "relocatable install paths test skipped on this platform"
		exit 0
		;;
esac

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
export HOME="$work/home"
export XDG_CONFIG_HOME="$work/config"
export XDG_DATA_HOME="$work/user-data"
unset AMIBERRY_DATA_DIR AMIBERRY_HOME_DIR AMIBERRY_PLUGINS_DIR

line_value()
{
	grep -m 1 "^$1=" "$2" | cut -d= -f2- | tr -d '\r'
}

"$AMIBERRY_BIN" --dump-paths > "$work/original-paths.txt"
bindir="$(line_value install_bindir_relative "$work/original-paths.txt")"
datadir="$(line_value install_datadir_relative "$work/original-paths.txt")"
libdir="$(line_value install_libdir_relative "$work/original-paths.txt")"
compiled_data_dir="$(line_value compiled_data_dir "$work/original-paths.txt")"
compiled_data_dir="${compiled_data_dir%/}"
compiled_plugins_dir="$(line_value system_plugins_dir "$work/original-paths.txt")"
[ -n "$bindir" ] || { echo "relative install bindir was not reported" >&2; exit 1; }
[ -n "$datadir" ] || { echo "relative install datadir was not reported" >&2; exit 1; }
[ -n "$libdir" ] || { echo "relative install libdir was not reported" >&2; exit 1; }

prefix="$work/relocated-prefix"
installed_data="$prefix/$datadir/amiberry"
installed_plugins="$prefix/$libdir/amiberry"
mkdir -p "$prefix/$bindir" "$installed_data/data" "$installed_data/controllers" \
	"$installed_data/roms" "$installed_data/whdboot" "$installed_plugins"
cp "$AMIBERRY_BIN" "$prefix/$bindir/amiberry"
chmod +x "$prefix/$bindir/amiberry"

(
	cd "$work"
	"$prefix/$bindir/amiberry" --dump-paths
) > "$work/relocated-paths.txt"

[ "$(line_value install_prefix "$work/relocated-paths.txt")" = "$prefix" ] \
	|| { echo "install prefix was not derived from the relocated executable" >&2; exit 1; }
[ "$(line_value relocated_data_root "$work/relocated-paths.txt")" = "$installed_data" ] \
	|| { echo "relocated data root was not selected" >&2; exit 1; }
[ "$(line_value data_dir "$work/relocated-paths.txt")" = "$installed_data/data" ] \
	|| { echo "data directory did not follow the relocated prefix" >&2; exit 1; }
[ "$(line_value abr_path "$work/relocated-paths.txt")" = "$installed_data/data/abr/" ] \
	|| { echo "ABR database path did not follow the relocated data directory" >&2; exit 1; }
[ "$(line_value relocated_plugins_root "$work/relocated-paths.txt")" = "$installed_plugins" ] \
	|| { echo "relocated plugin root was not selected" >&2; exit 1; }
user_plugins="$(line_value user_plugins_dir "$work/relocated-paths.txt")"
plugin_0="$(line_value plugin_search_path_0 "$work/relocated-paths.txt")"
plugin_1="$(line_value plugin_search_path_1 "$work/relocated-paths.txt")"
case "$plugin_0" in
	"$user_plugins"*) ;;
	*) echo "user plugin directory lost precedence" >&2; exit 1 ;;
esac
case "$plugin_1" in
	"$installed_plugins"*) ;;
	*) echo "relocated plugin root was not searched after the user directory" >&2; exit 1 ;;
esac

for seed in controllers roms whdboot; do
	expected="$installed_data/$seed"
	actual="$(line_value "${seed}_seed_source_0" "$work/relocated-paths.txt")"
	[ "$actual" = "$expected" ] \
		|| { echo "$seed seed source did not follow the relocated prefix" >&2; exit 1; }
done

# A previously saved computed plugin root must not become a persistent override;
# AppImage mount points change between launches.
config_file="$(line_value amiberry_conf_file "$work/relocated-paths.txt")"
mkdir -p "$(dirname "$config_file")"
printf 'plugins_dir=%s\n' "$installed_plugins" > "$config_file"
"$prefix/$bindir/amiberry" --dump-paths > "$work/migrated-paths.txt"
[ -z "$(line_value plugins_dir "$work/migrated-paths.txt")" ] \
	|| { echo "relocated plugin root was retained as a saved override" >&2; exit 1; }
rm -f "$config_file"

# A bundled floppy-sound path from a previous relocated root is a stale
# computed default, whereas a user-selected directory remains an override.
stale_floppy_sounds="$work/previous-prefix/$datadir/amiberry/data/floppy_sounds"
custom_floppy_sounds="$work/custom-floppy-sounds"
mkdir -p "$stale_floppy_sounds" "$custom_floppy_sounds"
printf 'floppy_sounds_dir=%s\n' "$stale_floppy_sounds" > "$config_file"
"$prefix/$bindir/amiberry" --dump-paths > "$work/stale-floppy-sounds-paths.txt"
[ "$(line_value floppy_sounds_dir "$work/stale-floppy-sounds-paths.txt")" = "$installed_data/data/floppy_sounds/" ] \
	|| { echo "stale bundled floppy-sound default was retained" >&2; exit 1; }
printf 'floppy_sounds_dir=%s\n' "$custom_floppy_sounds" > "$config_file"
"$prefix/$bindir/amiberry" --dump-paths > "$work/explicit-floppy-sounds-paths.txt"
[ "$(line_value floppy_sounds_dir "$work/explicit-floppy-sounds-paths.txt")" = "$custom_floppy_sounds" ] \
	|| { echo "explicit floppy-sound directory was not retained" >&2; exit 1; }
rm -f "$config_file"

mkdir -p "$work/environment-data" "$work/environment-plugins"
AMIBERRY_DATA_DIR="$work/environment-data" \
AMIBERRY_PLUGINS_DIR="$work/environment-plugins" \
	"$prefix/$bindir/amiberry" --dump-paths > "$work/override-paths.txt"
[ "$(line_value data_dir "$work/override-paths.txt")" = "$work/environment-data" ] \
	|| { echo "AMIBERRY_DATA_DIR lost precedence" >&2; exit 1; }
[ "$(line_value plugin_search_path_0 "$work/override-paths.txt")" = "$work/environment-plugins/" ] \
	|| { echo "AMIBERRY_PLUGINS_DIR lost precedence" >&2; exit 1; }

rm -rf "$installed_plugins"
"$prefix/$bindir/amiberry" --dump-paths > "$work/missing-plugin-paths.txt"
[ -z "$(line_value relocated_plugins_root "$work/missing-plugin-paths.txt" || true)" ] \
	|| { echo "missing relocated plugin root was reported as usable" >&2; exit 1; }
if [ -d "$compiled_plugins_dir" ]; then
	grep -F "=${compiled_plugins_dir%/}/" "$work/missing-plugin-paths.txt" | grep -q '^plugin_search_path_' \
		|| { echo "missing relocated plugins did not fall back to the retained compiled installation" >&2; exit 1; }
fi
[ "$(line_value data_dir "$work/missing-plugin-paths.txt")" = "$installed_data/data" ] \
	|| { echo "missing plugin root affected relocated data lookup" >&2; exit 1; }

mkdir -p "$installed_plugins"
rm -rf "$installed_data"
"$prefix/$bindir/amiberry" --dump-paths > "$work/missing-data-paths.txt"
[ -z "$(line_value relocated_data_root "$work/missing-data-paths.txt" || true)" ] \
	|| { echo "missing relocated data root was reported as usable" >&2; exit 1; }
[ "$(line_value relocated_plugins_root "$work/missing-data-paths.txt")" = "$installed_plugins" ] \
	|| { echo "missing data root affected relocated plugin lookup" >&2; exit 1; }
if [ -d "$compiled_data_dir" ]; then
	fallback_data_dir="$(line_value data_dir "$work/missing-data-paths.txt")"
	[ "${fallback_data_dir%/}" = "$compiled_data_dir" ] \
		|| { echo "missing relocated data did not fall back to the retained compiled installation" >&2; exit 1; }
	for seed in controllers roms whdboot; do
		case "$(line_value "${seed}_seed_source_0" "$work/missing-data-paths.txt")" in
			"${compiled_data_dir%/}/$seed") ;;
			*) echo "missing relocated data did not fall back to the compiled $seed seed source" >&2; exit 1 ;;
		esac
	done
fi

printf 'relocatable install paths behavioral test passed\n'
