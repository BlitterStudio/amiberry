#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
	echo "Usage: $0 <release-asset-name> [output-directory]" >&2
	exit 2
fi

asset="$1"
repo="${QEMU_UAE_RELEASE_REPO:-BlitterStudio/amiberry-qemu-uae}"
tag="${QEMU_UAE_RELEASE_TAG:-v11.0.1-amiberry.2}"
output_dir="${2:-${GITHUB_WORKSPACE:-$PWD}/.qemu-uae-plugin}"
download_dir="$output_dir/download"
extract_dir="$output_dir/extract"

command -v gh >/dev/null 2>&1 || {
	echo "gh is required to download QEMU-UAE release assets" >&2
	exit 1
}
command -v python3 >/dev/null 2>&1 || {
	echo "python3 is required to verify QEMU-UAE checksums" >&2
	exit 1
}

rm -rf "$output_dir"
mkdir -p "$download_dir" "$extract_dir"

echo "Downloading $asset from $repo@$tag"
gh release download "$tag" \
	--repo "$repo" \
	--pattern "$asset" \
	--pattern SHA256SUMS \
	--dir "$download_dir"

python3 - "$download_dir/SHA256SUMS" "$download_dir/$asset" "$asset" <<'PY'
import hashlib
import pathlib
import sys

sums_path = pathlib.Path(sys.argv[1])
asset_path = pathlib.Path(sys.argv[2])
asset_name = sys.argv[3]

expected = None
for line in sums_path.read_text(encoding="utf-8").splitlines():
	parts = line.strip().split(maxsplit=1)
	if len(parts) != 2:
		continue
	name = parts[1].lstrip("*")
	if name == asset_name or pathlib.Path(name).name == asset_name:
		expected = parts[0].lower()
		break

if expected is None:
	raise SystemExit(f"No SHA256SUMS entry found for {asset_name}")

actual = hashlib.sha256(asset_path.read_bytes()).hexdigest()
if actual != expected:
	raise SystemExit(f"Checksum mismatch for {asset_name}: expected {expected}, got {actual}")

print(f"Verified SHA256 for {asset_name}: {actual}")
PY

case "$asset" in
	*.tar.xz)
		tar -xJf "$download_dir/$asset" -C "$extract_dir"
		;;
	*.zip)
		python3 -m zipfile -e "$download_dir/$asset" "$extract_dir"
		;;
	*)
		echo "Unsupported QEMU-UAE asset type: $asset" >&2
		exit 1
		;;
esac

plugin="$(
	find "$extract_dir" -type f \( \
		-name qemu-uae.so -o \
		-name qemu-uae.dylib -o \
		-name qemu-uae.dll \
	\) -print | head -n 1
)"

if [ -z "$plugin" ]; then
	echo "Could not find qemu-uae plugin in $asset" >&2
	exit 1
fi

abs_plugin="$(python3 -c 'import pathlib, sys; print(pathlib.Path(sys.argv[1]).resolve().as_posix())' "$plugin")"
workspace="${GITHUB_WORKSPACE:-$PWD}"
rel_plugin="$(python3 -c 'import os, pathlib, sys; print(pathlib.Path(os.path.relpath(sys.argv[1], sys.argv[2])).as_posix())' "$abs_plugin" "$workspace")"

if [ -n "${GITHUB_ENV:-}" ]; then
	{
		echo "QEMU_UAE_PLUGIN=$abs_plugin"
		echo "QEMU_UAE_PLUGIN_RELATIVE=$rel_plugin"
	} >> "$GITHUB_ENV"
fi

if [ -n "${GITHUB_OUTPUT:-}" ]; then
	{
		echo "qemu_uae_plugin=$abs_plugin"
		echo "qemu_uae_plugin_relative=$rel_plugin"
	} >> "$GITHUB_OUTPUT"
fi

echo "QEMU-UAE plugin ready: $abs_plugin"
