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
	grep -m 1 "^$1=" "$2" | cut -d= -f2- | tr -d '\r'
}

"$AMIBERRY_BIN" --dump-paths > "$work/default-paths.txt"
portable_mode="$(line_value portable_mode "$work/default-paths.txt")"
if [ "$portable_mode" = "1" ]; then
	# Portable mode deliberately ignores environment and saved overrides: its
	# portable plugins folder is the sole search location.
	user_plugins="$(line_value user_plugins_dir "$work/default-paths.txt")"
	portable_plugins="$(line_value plugin_search_path_0 "$work/default-paths.txt")"
	[ -n "$user_plugins" ] || { echo "portable plugin path was not resolved" >&2; exit 1; }
	[ -z "$(line_value system_plugins_dir "$work/default-paths.txt")" ] \
		|| { echo "portable mode retained a system plugin path" >&2; exit 1; }
	case "$portable_plugins" in
		"$user_plugins"*) ;;
		*) echo "portable plugin path did not use the portable plugins folder" >&2; exit 1 ;;
	esac
	export AMIBERRY_PLUGINS_DIR="$work/environment-plugins"
	"$AMIBERRY_BIN" --dump-paths > "$work/portable-paths.txt"
	[ "$(line_value plugin_search_path_0 "$work/portable-paths.txt")" = "$portable_plugins" ] \
		|| { echo "portable mode honored an environment plugin override" >&2; exit 1; }
	count="$(grep -c '^plugin_search_path_' "$work/portable-paths.txt" || true)"
	[ "$count" -eq 1 ] || { echo "portable mode searched more than its plugins folder" >&2; exit 1; }
	[ -z "$(line_value plugins_dir "$work/portable-paths.txt")" ] \
		|| { echo "portable mode retained a plugins override" >&2; exit 1; }
	echo "plugin search paths behavioral test passed"
	exit 0
fi
# On Linux and FreeBSD, a non-portable run without either home variable keeps
# the legacy executable-relative plugins folder as its final fallback.
case "$(uname -s)" in
	Linux|FreeBSD)
		env -u HOME -u AMIBERRY_HOME_DIR "$AMIBERRY_BIN" --dump-paths > "$work/no-home-paths.txt"
		# SDL_GetBasePath() reports the executable directory with a trailing
		# separator; drop it before building the expected fallback path.
		fallback_root="$(line_value portable_root "$work/no-home-paths.txt")"
		fallback_root="${fallback_root%/}"
		fallback_path="$(grep '^plugin_search_path_' "$work/no-home-paths.txt" | tail -n 1 | cut -d= -f2- | tr -d '\r')"
		[ -n "$fallback_root" ] || { echo "executable directory was not resolved" >&2; exit 1; }
		case "$fallback_path" in
			"$fallback_root/plugins"*) ;;
			*) echo "missing executable-relative plugin fallback without a home directory" >&2; exit 1 ;;
		esac
		;;
esac

# An environment path equal to the user path must be represented only once,
# while still taking the first search position.
user_plugin_search_path="$(line_value user_plugins_dir "$work/default-paths.txt")"
[ -n "$user_plugin_search_path" ] || { echo "user plugin search path was not resolved" >&2; exit 1; }
export AMIBERRY_PLUGINS_DIR="$user_plugin_search_path"
"$AMIBERRY_BIN" --dump-paths > "$work/deduplicated-paths.txt"
case "$(line_value plugin_search_path_0 "$work/deduplicated-paths.txt")" in
	"$user_plugin_search_path"*) ;;
	*) echo "environment plugin path did not take precedence" >&2; exit 1 ;;
esac
count="$(grep -F "=$user_plugin_search_path" "$work/deduplicated-paths.txt" | grep -c '^plugin_search_path_' || true)"
[ "$count" -eq 1 ] || { echo "duplicate plugin search path was retained" >&2; exit 1; }
unset AMIBERRY_PLUGINS_DIR

# A saved sibling-folder override remains second, between the environment and
# user folders. On Windows its parent contains Amiberry.exe, exercising the
# legacy-default migration boundary. The dump normalizes separators, so compare
# emitted paths.
config_file="$(line_value amiberry_conf_file "$work/default-paths.txt")"
custom_plugins="$(line_value portable_root "$work/default-paths.txt")/third-party-plugins"
environment_plugins="$work/environment-plugins"
mkdir -p "$(dirname "$config_file")"
printf 'plugins_dir=%s\n' "$custom_plugins" > "$config_file"
export AMIBERRY_PLUGINS_DIR="$environment_plugins"
"$AMIBERRY_BIN" --dump-paths > "$work/precedence-paths.txt"
environment_path="$(line_value plugin_search_path_0 "$work/precedence-paths.txt")"
override_plugins="$(line_value plugins_dir "$work/precedence-paths.txt")"
override_path="$(line_value plugin_search_path_1 "$work/precedence-paths.txt")"
[ -n "$override_plugins" ] || { echo "sibling plugin override was discarded" >&2; exit 1; }
[ "$environment_path" != "$override_path" ] \
	|| { echo "environment path was not first" >&2; exit 1; }
[ "$environment_path" != "$user_plugin_search_path" ] \
	|| { echo "environment path was not first" >&2; exit 1; }
case "$override_path" in
	"$override_plugins"*) ;;
	*) echo "saved plugin override was not second" >&2; exit 1 ;;
esac
case "$(line_value plugin_search_path_2 "$work/precedence-paths.txt")" in
	"$user_plugin_search_path"*) ;;
	*) echo "user plugin folder was not searched after the override" >&2; exit 1 ;;
esac
unset AMIBERRY_PLUGINS_DIR

# A saved bundled/system path is a previous computed default, not an override.
# On Windows, case variation must still match the executable's plugins folder.
system_plugins="$(line_value system_plugins_dir "$work/default-paths.txt" || true)"
if [ -n "$system_plugins" ] && [ -n "$config_file" ]; then
	legacy_system_plugins="$system_plugins"
	case "$legacy_system_plugins" in
		[A-Za-z]:*) legacy_system_plugins="$(printf '%s' "$legacy_system_plugins" | tr '[:lower:]' '[:upper:]')" ;;
	esac
	printf 'plugins_dir=%s\n' "$legacy_system_plugins" > "$config_file"
	"$AMIBERRY_BIN" --dump-paths > "$work/migrated-paths.txt"
	[ -z "$(line_value plugins_dir "$work/migrated-paths.txt")" ] \
		|| { echo "saved bundled plugin path was retained as an override" >&2; exit 1; }
fi

echo "plugin search paths behavioral test passed"
