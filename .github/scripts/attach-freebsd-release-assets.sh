#!/usr/bin/env bash
# Attach FreeBSD packages to the GitHub release for a tag and merge their
# checksums into the release's SHA256SUMS.
#
# Usage: attach-freebsd-release-assets.sh <tag> <package.pkg>...
#
# c-cpp.yml creates the release and its SHA256SUMS in a separate workflow run,
# so wait for that asset before touching the release. A later re-run of
# c-cpp.yml's create-release job replaces SHA256SUMS without the FreeBSD
# entries; re-run this job afterwards to restore them.
set -euo pipefail

if [ "$#" -lt 2 ]; then
	echo "usage: $0 <tag> <package.pkg>..." >&2
	exit 2
fi

tag=$1
shift
wait_minutes=${RELEASE_WAIT_MINUTES:-120}
poll_seconds=${RELEASE_POLL_SECONDS:-60}
deadline=$((SECONDS + wait_minutes * 60))

while :; do
	assets=$(gh release view "$tag" --json assets --jq '.assets[].name' 2>/dev/null || true)
	if grep -qx SHA256SUMS <<<"$assets"; then
		break
	fi
	if [ "$SECONDS" -ge "$deadline" ]; then
		echo "::error::Release $tag has no SHA256SUMS after ${wait_minutes} minutes; re-run this job once c-cpp.yml has published the release." >&2
		exit 1
	fi
	echo "Waiting for release $tag and its SHA256SUMS..."
	sleep "$poll_seconds"
done

gh release upload "$tag" "$@" --clobber

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
gh release download "$tag" --pattern SHA256SUMS --dir "$work"

{
	# Drop entries left by an earlier run of this job, then add the current packages.
	grep -Ev '[[:space:]]\*?amiberry-.*-freebsd[0-9]+-[[:alnum:]_]+\.pkg$' "$work/SHA256SUMS" || true
	for pkg in "$@"; do
		(cd "$(dirname "$pkg")" && sha256sum "$(basename "$pkg")")
	done
} > "$work/SHA256SUMS.new"
mv "$work/SHA256SUMS.new" "$work/SHA256SUMS"

gh release upload "$tag" "$work/SHA256SUMS" --clobber
cat "$work/SHA256SUMS"
