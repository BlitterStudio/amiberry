#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <ubuntu-series>" >&2
	exit 2
fi

series="$1"
case "$series" in
	jammy|noble)
		;;
	*)
		exit 0
		;;
esac

readonly sdl_version="3.4.16"
readonly sdl_sha256="7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68"
readonly sdl_image_version="3.4.6"
readonly sdl_image_sha256="d2e4637ae700f72e5196b8fbd749850ed2e5e1e09c5a5be8d06ff55aaccf3b01"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

temp_dir="$(mktemp -d)"
trap 'rm -rf "$temp_dir"' EXIT

download_and_extract()
{
	local url="$1"
	local sha256="$2"
	local destination="$3"
	local archive="$temp_dir/$(basename "$url")"

	curl --fail --location --retry 3 --output "$archive" "$url"
	printf '%s  %s\n' "$sha256" "$archive" | sha256sum --check --status
	mkdir -p "$destination"
	tar --extract --gzip --file "$archive" --strip-components=1 --directory "$destination"
}

rm -rf debian/vendor/SDL debian/vendor/SDL_image

download_and_extract \
	"https://github.com/libsdl-org/SDL/releases/download/release-${sdl_version}/SDL3-${sdl_version}.tar.gz" \
	"$sdl_sha256" \
	debian/vendor/SDL

download_and_extract \
	"https://github.com/libsdl-org/SDL_image/releases/download/release-${sdl_image_version}/SDL3_image-${sdl_image_version}.tar.gz" \
	"$sdl_image_sha256" \
	debian/vendor/SDL_image

# Keep only SDL_image's Zlib-licensed implementation. The release archive also
# contains third-party codec sources and platform bundles that this PPA build
# neither compiles nor ships.
rm -rf \
	debian/vendor/SDL_image/external \
	debian/vendor/SDL_image/VisualC \
	debian/vendor/SDL_image/Xcode \
	debian/vendor/SDL_image/examples \
	debian/vendor/SDL_image/test
rm -f \
	debian/vendor/SDL_image/src/nanosvg.h \
	debian/vendor/SDL_image/src/nanosvgrast.h \
	debian/vendor/SDL_image/src/qoi.h \
	debian/vendor/SDL_image/src/stb_image.h \
	debian/vendor/SDL_image/src/tiny_jpeg.h

python3 - <<'PY'
from pathlib import Path

control_path = Path("debian/control")
lines = control_path.read_text(encoding="utf-8").splitlines(keepends=True)
targets = {"libsdl3-dev,", "libsdl3-image-dev,"}
found = {line.strip() for line in lines if line.strip() in targets}
missing = targets - found
if missing:
    raise SystemExit(f"Missing expected SDL Build-Depends: {', '.join(sorted(missing))}")
control_path.write_text(
    "".join(line for line in lines if line.strip() not in targets),
    encoding="utf-8",
)
PY

printf 'SDL=%s\nSDL_image=%s\n' "$sdl_version" "$sdl_image_version" > debian/ppa-bundle-sdl
