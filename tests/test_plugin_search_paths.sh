#!/bin/sh
# Behavioral test for ordered runtime plugin lookup and migration of stale
# computed plugin paths. Requires a built Amiberry binary.
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN is required" >&2
	exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
export HOME="$work/home"
export LOCALAPPDATA="$work/local-app-data"
export APPDATA="$work/app-data"
unset AMIBERRY_PLUGINS_DIR
unset AMIBERRY_HOME_DIR

line_value()
{
	grep -m 1 "^$1=" "$2" | cut -d= -f2-
}

"$AMIBERRY_BIN" --dump-paths > "$work/default-paths.txt"
# An environment path equal to the user path must be represented only once,
# while still taking the first search position.
user_plugin_search_path="$(line_value plugin_search_path_0 "$work/default-paths.txt")"
[ -n "$user_plugin_search_path" ] || { echo "user plugin search path was not resolved" >&2; exit 1; }
export AMIBERRY_PLUGINS_DIR="$user_plugin_search_path"
"$AMIBERRY_BIN" --dump-paths > "$work/deduplicated-paths.txt"
[ "$(line_value plugin_search_path_0 "$work/deduplicated-paths.txt")" = "$user_plugin_search_path" ] \
	|| { echo "environment plugin path did not take precedence" >&2; exit 1; }
count="$(grep -F "=$user_plugin_search_path" "$work/deduplicated-paths.txt" | grep -c '^plugin_search_path_' || true)"
[ "$count" -eq 1 ] || { echo "duplicate plugin search path was retained" >&2; exit 1; }
unset AMIBERRY_PLUGINS_DIR

# A saved custom override remains second, between the environment and user
# folders. The dump normalizes Windows separators, so compare emitted paths.
config_file="$(line_value amiberry_conf_file "$work/default-paths.txt")"
custom_plugins="$work/custom-plugins"
environment_plugins="$work/environment-plugins"
mkdir -p "$(dirname "$config_file")"
printf 'plugins_dir=%s\n' "$custom_plugins" > "$config_file"
export AMIBERRY_PLUGINS_DIR="$environment_plugins"
"$AMIBERRY_BIN" --dump-paths > "$work/precedence-paths.txt"
environment_path="$(line_value plugin_search_path_0 "$work/precedence-paths.txt")"
override_plugins="$(line_value plugins_dir "$work/precedence-paths.txt")"
override_path="$(line_value plugin_search_path_1 "$work/precedence-paths.txt")"
[ "$environment_path" != "$override_path" ] \
	|| { echo "environment path was not first" >&2; exit 1; }
[ "$environment_path" != "$user_plugin_search_path" ] \
	|| { echo "environment path was not first" >&2; exit 1; }
case "$override_path" in
	"$override_plugins"*) ;;
	*) echo "saved plugin override was not second" >&2; exit 1 ;;
esac
[ "$(line_value plugin_search_path_2 "$work/precedence-paths.txt")" = "$user_plugin_search_path" ] \
	|| { echo "user plugin folder was not searched after the override" >&2; exit 1; }
unset AMIBERRY_PLUGINS_DIR

# A saved bundled/system path is a previous computed default, not an override.
# Windows always has this path; platforms without a system plugin path simply do
# not exercise this platform-specific migration case.
system_plugins="$(line_value plugin_search_path_1 "$work/default-paths.txt" || true)"
if [ -n "$system_plugins" ] && [ -n "$config_file" ]; then
	printf 'plugins_dir=%s\n' "$system_plugins" > "$config_file"
	"$AMIBERRY_BIN" --dump-paths > "$work/migrated-paths.txt"
	[ -z "$(line_value plugins_dir "$work/migrated-paths.txt")" ] \
		|| { echo "saved bundled plugin path was retained as an override" >&2; exit 1; }
fi

echo "plugin search paths behavioral test passed"
