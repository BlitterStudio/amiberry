#!/bin/sh
# Behavioral test for relative path storage (#2407): with relative_paths
# enabled, configuration paths inside the relative path base are saved as
# "./..." paths, and such paths resolve against the base rather than the
# working directory. Requires a built Amiberry binary.
#
# Usage: AMIBERRY_BIN=<path-to-amiberry> tests/test_relative_paths.sh
set -eu

if [ -z "${AMIBERRY_BIN:-}" ]; then
	echo "AMIBERRY_BIN must point at a built amiberry binary" >&2
	exit 1
fi
bin="$AMIBERRY_BIN"

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
host_path()
{
	if command -v cygpath > /dev/null 2>&1; then cygpath -m "$1"; else printf '%s' "$1"; fi
}
export HOME="$work/home"
export LOCALAPPDATA="$(host_path "$work/local-app-data")"
export APPDATA="$(host_path "$work/app-data")"
export AMIBERRY_HOME_DIR="$(host_path "$work/content")"
unset AMIBERRY_CONFIG_DIR XDG_CONFIG_HOME XDG_DATA_HOME

line_value()
{
	grep -m 1 "^$1=" "$2" | cut -d= -f2- | tr -d '\r'
}

fail()
{
	echo "$1" >&2
	[ $# -gt 1 ] && cat "$2" >&2
	exit 1
}

mkdir -p "$work/content/Floppies" "$work/elsewhere" "$work/cwd/Floppies"
printf 'x' > "$work/content/Floppies/disk.adf"
printf 'x' > "$work/elsewhere/other.adf"
printf 'x' > "$work/cwd/local.adf"
# The same relative path also exists in the working directory: configuration
# files must still pick the base copy, command-line arguments the cwd copy.
printf 'x' > "$work/cwd/Floppies/disk.adf"

"$bin" --dump-paths > "$work/paths.txt"
if [ "$(line_value portable_mode "$work/paths.txt")" = "1" ]; then
	echo "relative paths test skipped: binary runs in portable mode"
	exit 0
fi
[ "$(line_value relative_paths "$work/paths.txt")" = "0" ] \
	|| fail "relative paths must default to off outside portable mode" "$work/paths.txt"
settings_file="$(line_value amiberry_conf_file "$work/paths.txt")"
[ -n "$settings_file" ] || fail "settings file was not resolved" "$work/paths.txt"
# The test rewrites this file, so it must live in the scratch tree.
case "$settings_file" in
	*"$(basename "$work")"*) ;;
	*) fail "settings file escaped the scratch directory: $settings_file" ;;
esac
mkdir -p "$(dirname "$settings_file")"

# floppy0 is base-relative, floppy1 lies outside the base, and floppy2 is a
# "./" path that exists only in the working directory.
printf 'floppy0=./Floppies/disk.adf\nfloppy1=%s\nfloppy2=./local.adf\n' \
	"$(host_path "$work/elsewhere/other.adf")" > "$work/media.uae"

dump()
{
	out="$1"
	shift
	(cd "$work/cwd" && "$bin" --dump-config "$@") > "$out" 2>/dev/null \
		|| fail "dump-config failed"
}

printf 'relative_paths=yes\n' > "$settings_file"
"$bin" --dump-paths > "$work/relative-paths.txt"
[ "$(line_value relative_paths "$work/relative-paths.txt")" = "1" ] \
	|| fail "relative_paths=yes was not loaded" "$work/relative-paths.txt"
dump "$work/relative.txt" -f "$(host_path "$work/media.uae")"
[ "$(line_value floppy0 "$work/relative.txt")" = "./Floppies/disk.adf" ] \
	|| fail "path inside the base was not saved relative" "$work/relative.txt"
line_value floppy1 "$work/relative.txt" | grep -Eq '^[^.].*elsewhere[/\\]other\.adf$' \
	|| fail "path outside the base must stay absolute" "$work/relative.txt"
line_value floppy2 "$work/relative.txt" | grep -Eq '^[^.].*cwd[/\\]local\.adf$' \
	|| fail "working-directory path was not kept" "$work/relative.txt"

dump "$work/cmdline.txt" -0 ./Floppies/disk.adf --cdimage ./Floppies/disk.adf
line_value floppy0 "$work/cmdline.txt" | grep -Eq '^[^.].*cwd[/\\]Floppies[/\\]disk\.adf$' \
	|| fail "command-line floppy did not resolve against the working directory" "$work/cmdline.txt"
line_value cdimage0 "$work/cmdline.txt" | grep -Eq '^[^.].*cwd[/\\]Floppies[/\\]disk\.adf(,|$)' \
	|| fail "command-line CD image did not resolve against the working directory" "$work/cmdline.txt"

# A base reached through a symlink: paths derived from it keep the link
# spelling, resolved paths are canonical, and both must count as inside it.
if ! command -v cygpath > /dev/null 2>&1 && ln -s "$work/content" "$work/content-link" 2> /dev/null; then
	(
		export AMIBERRY_HOME_DIR="$work/content-link"
		dump "$work/symlink.txt" -f "$work/media.uae"
	) || exit 1
	[ "$(line_value floppy0 "$work/symlink.txt")" = "./Floppies/disk.adf" ] \
		|| fail "path under a symlinked base was not saved relative" "$work/symlink.txt"
	[ "$(line_value amiberry.floppy_path "$work/symlink.txt")" = "./Floppies/" ] \
		|| fail "search path under a symlinked base was not saved relative" "$work/symlink.txt"
fi

printf 'relative_paths=no\n' > "$settings_file"
dump "$work/absolute.txt" -f "$(host_path "$work/media.uae")"
line_value floppy0 "$work/absolute.txt" | grep -Eq '^[^.].*content[/\\]Floppies[/\\]disk\.adf$' \
	|| fail "base-relative path did not resolve against the base" "$work/absolute.txt"

echo "relative paths behavioral test passed"
