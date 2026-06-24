#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 3 ]; then
	echo "Usage: $0 <ubuntu-series> <package-version> <release-message>" >&2
	exit 2
fi

series="$1"
package_version="$2"
release_message="$3"

source_name="${PPA_SOURCE_NAME:-amiberry}"
archive_api="${PPA_ARCHIVE_API:-https://api.launchpad.net/1.0/~midwan-a/+archive/ubuntu/amiberry}"
signing_key="${PPA_GPG_KEY:-D74D52525340C442A4F8B657A12B57C04E1FE282}"
build_root="${PPA_BUILD_ROOT:-/tmp/ppa-build}"

launchpad_has_source()
{
	python3 - "$archive_api" "$source_name" "$series" "$package_version" <<'PY'
import json
import sys
import urllib.parse
import urllib.request

archive_api, source_name, series, version = sys.argv[1:]
params = urllib.parse.urlencode({
	"ws.op": "getPublishedSources",
	"source_name": source_name,
	"exact_match": "true",
	"version": version,
	"distro_series": f"https://api.launchpad.net/1.0/ubuntu/{series}",
})
with urllib.request.urlopen(f"{archive_api}?{params}", timeout=30) as response:
	payload = json.load(response)

for entry in payload.get("entries", []):
	if entry.get("source_package_version") != version:
		continue
	status = entry.get("status")
	if status in {"Pending", "Published"}:
		print(f"{source_name} {version} for {series} is already {status} in Launchpad")
		sys.exit(0)

sys.exit(1)
PY
}

if launchpad_has_source; then
	exit 0
fi

build_dir="$build_root/$series"
rm -rf "$build_dir"
mkdir -p "$build_dir"
cp -r . "$build_dir/$source_name"
cd "$build_dir/$source_name"

rm -f debian/changelog
dch --create --package "$source_name" \
	--newversion "$package_version" \
	--distribution "$series" \
	"$release_message"

dpkg-buildpackage -S -d -us -uc

shopt -s nullglob
changes_files=( "$build_dir"/"${source_name}"_*_source.changes )
if [ "${#changes_files[@]}" -ne 1 ]; then
	echo "Expected one source .changes file for $series, found ${#changes_files[@]}" >&2
	exit 1
fi
changes_file="${changes_files[0]}"

debsign -k "$signing_key" "$changes_file"

# The package may have appeared while this job was building locally.
if launchpad_has_source; then
	exit 0
fi

for attempt in 1 2 3; do
	if dput amiberry-ppa "$changes_file"; then
		exit 0
	else
		status=$?
	fi

	if launchpad_has_source; then
		exit 0
	fi

	if [ "$attempt" -eq 3 ]; then
		exit "$status"
	fi

	sleep_seconds=$((attempt * 60))
	echo "dput failed for $series on attempt $attempt; retrying in ${sleep_seconds}s" >&2
	sleep "$sleep_seconds"
done
