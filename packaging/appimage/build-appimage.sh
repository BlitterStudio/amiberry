#!/usr/bin/env bash
set -euo pipefail

fail() {
    printf 'AppImage build failed: %s\n' "$*" >&2
    exit 1
}

usage() {
    cat >&2 <<'USAGE'
Usage: build-appimage.sh --version VERSION [options]

Options:
  --channel stable|preview       Release update channel (default: stable)
  --arch x86_64|aarch64          Native target architecture (default: uname -m)
  --build-dir PATH               CMake build directory (default: build/appimage)
  --output-dir PATH              Output directory (default: dist)
  --jobs N                       Parallel build jobs (default: 3)
  --cmake-arg ARG                Extra CMake configure argument; may be repeated
  --help                         Show this help
USAGE
    exit 2
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(cd "$script_dir/../.." && pwd -P)"
# shellcheck source-path=SCRIPTDIR
# shellcheck source=dependencies.env
source "$script_dir/dependencies.env"

version=""
channel="stable"
arch="$(uname -m)"
build_dir="$repo_root/build/appimage"
output_dir="$repo_root/dist"
jobs="${APPIMAGE_BUILD_JOBS:-3}"
cmake_args=()

while (($#)); do
    case "$1" in
        --version)
            (($# >= 2)) || usage
            version="$2"
            shift 2
            ;;
        --channel)
            (($# >= 2)) || usage
            channel="$2"
            shift 2
            ;;
        --arch)
            (($# >= 2)) || usage
            arch="$2"
            shift 2
            ;;
        --build-dir)
            (($# >= 2)) || usage
            build_dir="$2"
            shift 2
            ;;
        --output-dir)
            (($# >= 2)) || usage
            output_dir="$2"
            shift 2
            ;;
        --jobs)
            (($# >= 2)) || usage
            jobs="$2"
            shift 2
            ;;
        --cmake-arg)
            (($# >= 2)) || usage
            cmake_args+=("$2")
            shift 2
            ;;
        --help|-h)
            usage
            ;;
        *)
            fail "unknown argument: $1"
            ;;
    esac
done

[[ -n "$version" ]] || fail "--version is required"
[[ "$version" =~ ^[A-Za-z0-9][A-Za-z0-9._+-]*$ ]] || fail "--version contains unsafe filename characters: $version"
[[ "$channel" == "stable" || "$channel" == "preview" ]] || fail "--channel must be stable or preview"
[[ "$arch" == "x86_64" || "$arch" == "aarch64" ]] || fail "--arch must be x86_64 or aarch64"
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || fail "--jobs must be a positive integer"
host_arch="$(uname -m)"
[[ "$host_arch" == "$arch" ]] || fail "--arch $arch requires a native $arch host (uname -m reports $host_arch)"
if [[ -n "${SOURCE_DATE_EPOCH:-}" ]]; then
    [[ "$SOURCE_DATE_EPOCH" =~ ^[0-9]+$ ]] || fail "SOURCE_DATE_EPOCH must be an integer Unix timestamp"
    export SOURCE_DATE_EPOCH
fi


for command in curl sha256sum cmake ninja tar dpkg-deb find install patchelf readelf ldd grep; do
    command -v "$command" >/dev/null 2>&1 || fail "required command is unavailable: $command"
done
[[ -x "$repo_root/tests/run_tests.sh" ]] || fail "missing executable test runner: tests/run_tests.sh"
[[ -f "$script_dir/verify-appdir.sh" && -r "$script_dir/verify-appdir.sh" ]] || fail "missing readable AppDir verifier: $script_dir/verify-appdir.sh"

mkdir -p "$build_dir" "$output_dir"
build_dir="$(cd "$build_dir" && pwd -P)"
output_dir="$(cd "$output_dir" && pwd -P)"
cache_dir="${APPIMAGE_CACHE_DIR:-$build_dir/downloads}"
mkdir -p "$cache_dir"
cache_dir="$(cd "$cache_dir" && pwd -P)"

verify_sha256() {
    local file="$1" expected="$2"
    printf '%s *%s\n' "$expected" "$file" | sha256sum --check --status
}

fetch_pinned() {
    local url="$1" expected="$2" destination="$3"
    if [[ -f "$destination" ]] && verify_sha256 "$destination" "$expected"; then
        return
    fi
    rm -f -- "$destination" "${destination}.part"
    printf 'Downloading %s\n' "$(basename "$destination")"
    curl --fail --location --retry 3 --retry-delay 2 --output "${destination}.part" "$url"
    mv -- "${destination}.part" "$destination"
    verify_sha256 "$destination" "$expected" || {
        rm -f -- "$destination"
        fail "checksum mismatch for immutable download: $url"
    }
}

case "$arch" in
    x86_64)
        linuxdeploy_url="$LINUXDEPLOY_X86_64_URL"
        linuxdeploy_sha256="$LINUXDEPLOY_X86_64_SHA256"
        appimagetool_url="$APPIMAGETOOL_X86_64_URL"
        appimagetool_sha256="$APPIMAGETOOL_X86_64_SHA256"
        runtime_url="$TYPE2_RUNTIME_X86_64_URL"
        runtime_sha256="$TYPE2_RUNTIME_X86_64_SHA256"
        zsync_url="$ZSYNC_AMD64_URL"
        zsync_sha256="$ZSYNC_AMD64_SHA256"
        qemu_url="$QEMU_UAE_X86_64_URL"
        qemu_sha256="$QEMU_UAE_X86_64_SHA256"
        ;;
    aarch64)
        linuxdeploy_url="$LINUXDEPLOY_AARCH64_URL"
        linuxdeploy_sha256="$LINUXDEPLOY_AARCH64_SHA256"
        appimagetool_url="$APPIMAGETOOL_AARCH64_URL"
        appimagetool_sha256="$APPIMAGETOOL_AARCH64_SHA256"
        runtime_url="$TYPE2_RUNTIME_AARCH64_URL"
        runtime_sha256="$TYPE2_RUNTIME_AARCH64_SHA256"
        zsync_url="$ZSYNC_ARM64_URL"
        zsync_sha256="$ZSYNC_ARM64_SHA256"
        qemu_url="$QEMU_UAE_AARCH64_URL"
        qemu_sha256="$QEMU_UAE_AARCH64_SHA256"
        ;;
esac

linuxdeploy="$cache_dir/$(basename "$linuxdeploy_url")"
appimagetool="$cache_dir/$(basename "$appimagetool_url")"
runtime="$cache_dir/$(basename "$runtime_url")"
zsync_deb="$cache_dir/$(basename "$zsync_url")"
qemu_archive="$cache_dir/$(basename "$qemu_url")"
fetch_pinned "$linuxdeploy_url" "$linuxdeploy_sha256" "$linuxdeploy"
fetch_pinned "$appimagetool_url" "$appimagetool_sha256" "$appimagetool"
fetch_pinned "$runtime_url" "$runtime_sha256" "$runtime"
fetch_pinned "$zsync_url" "$zsync_sha256" "$zsync_deb"
fetch_pinned "$qemu_url" "$qemu_sha256" "$qemu_archive"
chmod +x "$linuxdeploy" "$appimagetool"

qemu_extract_dir="$build_dir/qemu-uae-extract"
rm -rf -- "$qemu_extract_dir"
mkdir -p "$qemu_extract_dir"
tar -xJf "$qemu_archive" -C "$qemu_extract_dir"
mapfile -d '' qemu_plugins < <(find "$qemu_extract_dir" -type f -name qemu-uae.so -print0)
((${#qemu_plugins[@]} == 1)) || fail "expected exactly one qemu-uae.so in $qemu_archive, found ${#qemu_plugins[@]}"
qemu_plugin="${qemu_plugins[0]}"

cmake_configure=(
    cmake -S "$repo_root" -B "$build_dir" -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=/usr
    -DCMAKE_INSTALL_LIBDIR=lib
    -DBUNDLE_SDL=ON
    "-DQEMU_UAE_PLUGIN=$qemu_plugin"
)
cmake_configure+=("${cmake_args[@]}")
"${cmake_configure[@]}"
cmake --build "$build_dir" --parallel "$jobs"
(
    cd "$repo_root"
    # The slow settings-layer tests do not depend on the packaging; CI runs
    # them against its Ubuntu builds (see tests/run_tests.sh).
    tests/run_tests.sh --behavioral-only --skip-slow "$build_dir/amiberry"
)

appdir="$build_dir/AppDir"
rm -rf -- "$appdir"
mkdir -p "$appdir"
DESTDIR="$appdir" cmake --install "$build_dir"

metadata_dir="$build_dir/appimage-metadata"
rm -rf -- "$metadata_dir"
mkdir -p "$metadata_dir"
native_desktop="$appdir/usr/share/applications/Amiberry.desktop"
staged_desktop="$metadata_dir/com.blitterstudio.amiberry.desktop"
staged_metainfo="$appdir/usr/share/metainfo/Amiberry.metainfo.xml"
[[ -f "$native_desktop" ]] || fail "CMake install did not stage $native_desktop"
[[ -f "$staged_metainfo" ]] || fail "CMake install did not stage $staged_metainfo"

while IFS= read -r line || [[ -n "$line" ]]; do
    case "$line" in
        Icon=*) printf '%s\n' 'Icon=com.blitterstudio.amiberry' ;;
        *) printf '%s\n' "$line" ;;
    esac
done < "$native_desktop" > "$staged_desktop"
rm -f -- "$native_desktop"

normalized_metainfo="$appdir/usr/share/metainfo/com.blitterstudio.amiberry.metainfo.xml"
while IFS= read -r line || [[ -n "$line" ]]; do
    case "$line" in
        *'<launchable type="desktop-id">'*'</launchable>'*) printf '%s\n' '    <launchable type="desktop-id">com.blitterstudio.amiberry.desktop</launchable>' ;;
        *) printf '%s\n' "$line" ;;
    esac
done < "$staged_metainfo" > "$normalized_metainfo"
rm -f -- "$staged_metainfo"

for size in 64 128 256 512; do
    icon_dir="$appdir/usr/share/icons/hicolor/${size}x${size}/apps"
    native_icon="$icon_dir/amiberry.png"
    [[ -f "$native_icon" ]] || fail "CMake install did not stage the ${size}x${size} application icon"
    install -m 0644 "$native_icon" "$icon_dir/com.blitterstudio.amiberry.png"
    rm -f -- "$native_icon"
done
icon_source="$metadata_dir/com.blitterstudio.amiberry.png"
install -m 0644 "$appdir/usr/share/icons/hicolor/256x256/apps/com.blitterstudio.amiberry.png" "$icon_source"

for plugin in libcapsimage.so libfloppybridge.so qemu-uae.so; do
    [[ -f "$appdir/usr/lib/amiberry/$plugin" ]] || fail "CMake install did not stage plugin: $plugin"
done

linuxdeploy_exclusions=()
for library_pattern in "${APPIMAGE_HOST_LIBRARY_PATTERNS[@]}"; do
    linuxdeploy_exclusions+=(--exclude-library "$library_pattern")
done
APPIMAGE_EXTRACT_AND_RUN=1 "$linuxdeploy" \
    --appdir "$appdir" \
    --desktop-file "$staged_desktop" \
    --icon-file "$icon_source" \
    --executable "$appdir/usr/bin/amiberry" \
    --deploy-deps-only "$appdir/usr/bin/amiberry" \
    --deploy-deps-only "$appdir/usr/lib/amiberry/libcapsimage.so" \
    --deploy-deps-only "$appdir/usr/lib/amiberry/libfloppybridge.so" \
    --deploy-deps-only "$appdir/usr/lib/amiberry/qemu-uae.so" \
    "${linuxdeploy_exclusions[@]}"
rm -f -- "$appdir/.DirIcon"
ln -s com.blitterstudio.amiberry.png "$appdir/.DirIcon"

# Keep the installed plugin paths canonical. --deploy-deps-only updates their
# RUNPATHs relative to usr/lib and deploys their private closures there.
bash "$script_dir/verify-appdir.sh" --arch "$arch" "$appdir"

zsync_root="$build_dir/zsync"
rm -rf -- "$zsync_root"
dpkg-deb -x "$zsync_deb" "$zsync_root"
zsyncmake="$zsync_root/usr/bin/zsyncmake"
[[ -x "$zsyncmake" ]] || fail "pinned zsync package did not provide usr/bin/zsyncmake"

case "$channel" in
    stable) update_tag="latest" ;;
    preview) update_tag="latest-pre" ;;
esac
image_name="Amiberry-${version}-${arch}.AppImage"
image="$output_dir/$image_name"
rm -f -- "$image" "${image}.zsync"
update_information="gh-releases-zsync|BlitterStudio|amiberry|${update_tag}|Amiberry-*-${arch}.AppImage.zsync"
(
    cd "$output_dir"
    PATH="$zsync_root/usr/bin:$PATH" \
        ARCH="$arch" \
        APPIMAGE_EXTRACT_AND_RUN=1 \
        "$appimagetool" --runtime-file "$runtime" \
            --updateinformation "$update_information" \
            "$appdir" "$image_name"
)
[[ -s "$image" ]] || fail "appimagetool did not create $image"
[[ -s "${image}.zsync" ]] || fail "appimagetool did not create ${image}.zsync; ensure update information and pinned zsyncmake are usable"
embedded_update_information="$("$image" --appimage-updateinfo)" \
    || fail "cannot read embedded update information from $image"
[[ "$embedded_update_information" == "$update_information" ]] \
    || fail "embedded update information does not match the requested channel and architecture"
grep -Fx "Filename: $image_name" "${image}.zsync" >/dev/null \
    || fail "zsync sidecar does not describe $image_name"
printf 'Created %s and %s.zsync\n' "$image" "$image"
