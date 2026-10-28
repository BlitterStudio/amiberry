#!/bin/sh
# Turn a staged `cmake --install` tree into a Haiku .hpkg package.
#
# Usage: haiku-package.sh <build-dir> <stage-dir> <output.hpkg>
#
# Runs inside the Haiku guest after the build. The package requires the
# HaikuPorts packages that provide the shared libraries our ELF files link
# directly (DT_NEEDED), so `pkgman install ./amiberry-*.hpkg` pulls them in
# from the user's configured repository. The Haiku system libraries
# (libroot, libbe, ...) are covered by the `haiku` requirement.
set -eu

if [ "$#" -ne 3 ]; then
	echo "usage: $0 <build-dir> <stage-dir> <output.hpkg>" >&2
	exit 2
fi

build_dir=$1
stage_dir=$2
output=$3
prefix=/boot/system
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

# Haiku package versions are major.minor.micro with an optional pre-release
# component before the revision: 8.5.0 becomes 8.5.0-1, a pre-release
# build's 8.5.0-31 becomes 8.5.0~pre31-1.
product_version=$(cat "$build_dir/amiberry-product-version.txt")
case $product_version in
*-*) hpkg_version="${product_version%-*}~pre${product_version##*-}-1" ;;
*) hpkg_version="${product_version}-1" ;;
esac

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

# Collect the direct shared-library dependencies of every ELF file we ship
# and strip them like CPACK_STRIP_FILES does for the Linux packages. Haiku
# library names carry no version suffix: libSDL3.so provides lib:libSDL3.
if command -v readelf >/dev/null 2>&1; then
	dump_needed() { readelf -d "$1" 2>/dev/null | sed -n 's/.*NEEDED.*\[\(.*\)\]$/\1/p'; }
else
	dump_needed() { objdump -p "$1" 2>/dev/null | sed -n 's/^[[:space:]]*NEEDED[[:space:]][[:space:]]*\(.*\)$/\1/p'; }
fi

: > "$work/needed"
find "$root" -type f | while IFS= read -r f; do
	if dump_needed "$f" | grep -q .; then
		strip "$f"
		dump_needed "$f" >> "$work/needed"
	fi
done

# Libraries of the haiku base package; every other library must be provided
# by an installed HaikuPorts package, which `requires lib:<name>` pulls in.
system_libs=' libroot.so libbe.so libnetwork.so libbsd.so libgcc_s.so libstdc++.so libsupc++.so '
: > "$work/requires"
for soname in $(LC_ALL=C sort -u "$work/needed"); do
	case " $system_libs " in
	*" $soname "*) continue ;;
	esac
	if find "$root/lib" -name "$soname" | grep -q .; then
		continue # shipped inside this package
	fi
	if [ ! -e "/boot/system/lib/$soname" ]; then
		echo "error: no installed package provides $soname" >&2
		exit 1
	fi
	printf 'lib:%s\n' "$(printf '%s' "$soname" | sed 's/\.so\.[0-9.]*$//; s/\.so$//')" >> "$work/requires"
done

description=$(tr '\n' ' ' < "$src_dir/packaging/Description.txt" | tr -s ' ')

{
	echo "name\t\tamiberry"
	echo "version\t\t$hpkg_version"
	echo "architecture\tx86_64"
	echo 'summary\t\t"Optimized Amiga emulator"'
	echo "description\t\"$description\""
	echo 'packager\t"Dimitris Panokostas <midwan@gmail.com>"'
	echo 'vendor\t\t"BlitterStudio"'
	echo 'copyrights\t[ "The Amiberry contributors" ]'
	echo 'licenses\t[ "GNU GPL v3" ]'
	echo 'urls\t\t[ "https://amiberry.com", "https://github.com/BlitterStudio/amiberry" ]'
	echo 'provides\t['
	echo "\t\t\tamiberry = $hpkg_version"
	echo "\t\t\tcmd:amiberry = $hpkg_version"
	echo '\t\t]'
	echo 'requires\t['
	echo '\t\t\thaiku'
	LC_ALL=C sort -u "$work/requires" | sed 's/^/\t\t\t/'
	echo '\t\t]'
} > "$work/.PackageInfo"

echo "--- .PackageInfo ---"
cat "$work/.PackageInfo"
echo "--------------------"

case $output in
/*) ;;
*) output=$PWD/$output ;;
esac
(cd "$root" && package create -i "$work/.PackageInfo" "$output")

echo "--- package list ---"
package list "$output"
echo "--------------------"
ls -l "$output"
