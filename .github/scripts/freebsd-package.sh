#!/bin/sh
# Turn a staged `cmake --install` tree into a FreeBSD pkg(8) package.
#
# Usage: freebsd-package.sh <build-dir> <stage-dir> <output.pkg>
#
# Runs inside the FreeBSD guest after the build. The package depends on the
# installed packages that provide the shared libraries our ELF files link
# directly (DT_NEEDED), so `pkg install ./amiberry-*.pkg` pulls them in from
# the user's configured repository.
set -eu

if [ "$#" -ne 3 ]; then
	echo "usage: $0 <build-dir> <stage-dir> <output.pkg>" >&2
	exit 2
fi

build_dir=$1
stage_dir=$2
output=$3
prefix=/usr/local
root=$stage_dir$prefix
src_dir=$(cd "$(dirname "$0")/../.." && pwd)

if [ ! -d "$root" ]; then
	echo "error: $root does not exist; run cmake --install with DESTDIR=$stage_dir first" >&2
	exit 1
fi
stray=$(find "$stage_dir" -mindepth 1 \( -type f -o -type l \) ! -path "$root/*")
if [ -n "$stray" ]; then
	echo "error: staged files outside $prefix:" >&2
	echo "$stray" >&2
	exit 1
fi

# pkg versions cannot contain '-': a preview build's 8.0.0-31 becomes 8.0.0.pre31.
version=$(sed 's/-/.pre/' "$build_dir/amiberry-product-version.txt")

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

(cd "$root" && find . \( -type f -o -type l \) | sed 's|^\./||' | LC_ALL=C sort) > "$work/plist"
(cd "$root" && find . -mindepth 1 -type d -empty | sed 's|^\./|@dir |' | LC_ALL=C sort -r) >> "$work/plist"

# Strip like CPACK_STRIP_FILES does for the Linux packages, and collect the
# direct shared-library dependencies of every ELF file we ship.
: > "$work/needed"
find "$root" -type f | while IFS= read -r f; do
	case $(file -b "$f") in
	ELF*)
		strip "$f"
		readelf -d "$f" | sed -n 's/.*NEEDED.*\[\(.*\)\]$/\1/p' >> "$work/needed"
		;;
	esac
done

pkg query -a '%b %n' > "$work/shlib-owners"
: > "$work/deps"
for soname in $(LC_ALL=C sort -u "$work/needed"); do
	owner=$(awk -v s="$soname" '$1 == s { print $2; exit }' "$work/shlib-owners")
	if [ -n "$owner" ]; then
		echo "$owner" >> "$work/deps"
	elif [ -e "/lib/$soname" ] || [ -e "/usr/lib/$soname" ]; then
		: # base system
	elif awk -F/ -v s="$soname" '$NF == s { found = 1 } END { exit !found }' "$work/plist"; then
		: # shipped in this package
	else
		echo "error: no installed package provides $soname" >&2
		exit 1
	fi
done

mkdir "$work/meta"
{
	echo 'name: amiberry'
	echo "version: \"$version\""
	echo 'origin: emulators/amiberry'
	echo 'comment: "Optimized Amiga emulator"'
	echo 'www: "https://amiberry.com"'
	echo 'maintainer: "midwan@gmail.com"'
	echo "prefix: \"$prefix\""
	echo 'categories: [emulators]'
	echo 'licenselogic: single'
	echo 'licenses: [GPLv3+]'
	echo 'desc: <<EOD'
	# Description.txt has no trailing newline; EOD must start its own line.
	printf '%s\n' "$(cat "$src_dir/packaging/Description.txt")"
	echo 'EOD'
	echo 'deps: {'
	for dep in $(LC_ALL=C sort -u "$work/deps"); do
		pkg query '  "%n": { origin: "%o", version: "%v" },' "$dep"
	done
	echo '}'
} > "$work/meta/+MANIFEST"

mkdir "$work/out"
pkg create -m "$work/meta" -p "$work/plist" -r "$stage_dir" -o "$work/out"
mv "$work/out/amiberry-$version.pkg" "$output"

pkg info -F "$output"
echo "Dependencies:"
pkg info -d -F "$output"
