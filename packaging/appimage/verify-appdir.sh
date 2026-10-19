#!/usr/bin/env bash
# Validate an AppDir before it is sealed into an AppImage.
set -euo pipefail

fail() {
    printf 'AppDir verification failed: %s\n' "$*" >&2
    exit 1
}

usage() {
    cat >&2 <<'USAGE'
Usage: verify-appdir.sh --arch x86_64|aarch64 APPDIR
USAGE
    exit 2
}

arch=""
while (($#)); do
    case "$1" in
        --arch)
            (($# >= 2)) || usage
            arch="$2"
            shift 2
            ;;
        --help|-h)
            usage
            ;;
        --*)
            fail "unknown option: $1"
            ;;
        *)
            break
            ;;
    esac
done

[[ "$arch" == "x86_64" || "$arch" == "aarch64" ]] || fail "--arch must be x86_64 or aarch64"
(($# == 1)) || usage
appdir="$1"
[[ -d "$appdir" ]] || fail "AppDir does not exist: $appdir"
appdir="$(cd "$appdir" && pwd -P)"

for command in find readelf ldd grep readlink sort; do
    command -v "$command" >/dev/null 2>&1 || fail "required command is unavailable: $command"
done

require_nonempty_directory() {
    local directory="$1"
    [[ -d "$directory" ]] || fail "required directory is missing: ${directory#"$appdir"/}"
    find "$directory" -mindepth 1 -print -quit | grep -q . || fail "required directory is empty: ${directory#"$appdir"/}"
}

require_regular_file() {
    local file="$1"
    [[ -f "$file" ]] || fail "required file is missing: ${file#"$appdir"/}"
}

require_nonempty_directory "$appdir/usr/share/amiberry/data"
require_nonempty_directory "$appdir/usr/share/amiberry/controllers"
require_nonempty_directory "$appdir/usr/share/amiberry/roms"
require_nonempty_directory "$appdir/usr/share/amiberry/whdboot"
require_regular_file "$appdir/usr/bin/amiberry"
[[ -x "$appdir/usr/bin/amiberry" ]] || fail "amiberry is not executable"

shopt -s nullglob
root_desktops=("$appdir"/*.desktop)
shopt -u nullglob
((${#root_desktops[@]} == 1)) || fail "expected exactly one root desktop entry, found ${#root_desktops[@]}"
desktop_file="${root_desktops[0]}"
[[ "$(basename "$desktop_file")" == "com.blitterstudio.amiberry.desktop" ]] || fail "root desktop entry must be com.blitterstudio.amiberry.desktop"
grep -Fx 'Icon=com.blitterstudio.amiberry' "$desktop_file" >/dev/null || fail "root desktop Icon must be com.blitterstudio.amiberry"
grep -Fx 'Exec=amiberry %f' "$desktop_file" >/dev/null || fail "root desktop Exec must be amiberry %f"
require_regular_file "$appdir/com.blitterstudio.amiberry.png"
[[ -e "$appdir/.DirIcon" ]] || fail "missing root .DirIcon"
[[ "$(readlink -f "$appdir/.DirIcon")" == "$(readlink -f "$appdir/com.blitterstudio.amiberry.png")" ]] || fail ".DirIcon does not resolve to the root application icon"
[[ -x "$appdir/AppRun" ]] || fail "missing executable AppRun"
if [[ -f "$appdir/AppRun" ]] && grep -aFq 'LD_LIBRARY_PATH' "$appdir/AppRun"; then
    fail "AppRun must not configure a global LD_LIBRARY_PATH"
fi


mapfile -d '' installed_desktops < <(find "$appdir/usr/share/applications" -maxdepth 1 \( -type f -o -type l \) -name '*.desktop' -print0)
((${#installed_desktops[@]} == 1)) || fail "expected exactly one installed desktop entry, found ${#installed_desktops[@]}"
[[ "$(basename "${installed_desktops[0]}")" == "com.blitterstudio.amiberry.desktop" ]] || fail "installed desktop entry must be com.blitterstudio.amiberry.desktop"

metainfo="$appdir/usr/share/metainfo/com.blitterstudio.amiberry.metainfo.xml"
require_regular_file "$metainfo"
grep -Eq '^[[:space:]]*<id>com\.blitterstudio\.amiberry</id>[[:space:]]*$' "$metainfo" || fail "AppStream id does not match desktop identity"
grep -Eq '^[[:space:]]*<launchable type="desktop-id">com\.blitterstudio\.amiberry\.desktop</launchable>[[:space:]]*$' "$metainfo" || fail "AppStream launchable does not match root desktop entry"

for plugin in libcapsimage.so libfloppybridge.so qemu-uae.so; do
    require_regular_file "$appdir/usr/lib/amiberry/$plugin"
done

for sdl_library in libSDL3.so libSDL3_image.so; do
    if ! find "$appdir/usr/lib/amiberry" -type f -name "${sdl_library}*" -print -quit | grep -q .; then
        fail "bundled ${sdl_library} is missing from usr/lib/amiberry"
    fi
done
for size in 64 128 256 512; do
    require_regular_file "$appdir/usr/share/icons/hicolor/${size}x${size}/apps/com.blitterstudio.amiberry.png"
done

# A relative symlink is fine; a link that escapes the AppDir or dangles is not.
while IFS= read -r -d '' link; do
    target="$(readlink -f -- "$link")" || fail "dangling symlink: ${link#"$appdir"/}"
    case "$target" in
        "$appdir"|"$appdir"/*) ;;
        *) fail "symlink escapes AppDir: ${link#"$appdir"/} -> $target" ;;
    esac
done < <(find "$appdir" -type l -print0)

version_exceeds() {
    local actual="$1" maximum="$2" actual_major actual_minor maximum_major maximum_minor
    IFS=. read -r actual_major actual_minor <<<"$actual"
    IFS=. read -r maximum_major maximum_minor <<<"$maximum"
    actual_minor="${actual_minor:-0}"
    maximum_minor="${maximum_minor:-0}"
    ((10#$actual_major > 10#$maximum_major)) || { ((10#$actual_major == 10#$maximum_major && 10#$actual_minor > 10#$maximum_minor)); }
}

check_elf_architecture() {
    local elf="$1" header
    header="$(readelf -hW "$elf" 2>/dev/null)" || return 1
    case "$arch" in
        x86_64)
            [[ "$header" =~ Class:[[:space:]]+ELF64 && "$header" =~ Machine:[[:space:]]+Advanced[[:space:]]+Micro[[:space:]]+Devices[[:space:]]+X86-64 ]] || fail "wrong ELF architecture for ${elf#"$appdir"/}; expected x86_64"
            ;;
        aarch64)
            [[ "$header" =~ Class:[[:space:]]+ELF64 && "$header" =~ Machine:[[:space:]]+AArch64 ]] || fail "wrong ELF architecture for ${elf#"$appdir"/}; expected aarch64"
            ;;
    esac
    return 0
}

check_runpaths() {
    local elf="$1" dynamic_entry runpath component
    while IFS= read -r dynamic_entry; do
        case "$dynamic_entry" in
            *'(RPATH)'*|*'(RUNPATH)'*)
                runpath="${dynamic_entry#*[}"
                runpath="${runpath%%]*}"
                IFS=: read -r -a runpath_components <<<"$runpath"
                for component in "${runpath_components[@]}"; do
                    [[ "$component" != /* ]] || fail "absolute RPATH/RUNPATH in ${elf#"$appdir"/}: $runpath"
                done
                ;;
        esac
    done < <(readelf -dW "$elf" 2>/dev/null)
}

check_symbol_versions() {
    local elf="$1" version_info symbol_version
    version_info="$(readelf --version-info -W "$elf" 2>/dev/null || true)"
    while IFS= read -r symbol_version; do
        [[ -n "$symbol_version" ]] || continue
        if [[ "$symbol_version" =~ ^GLIBC_([0-9]+\.[0-9]+)$ ]] && version_exceeds "${BASH_REMATCH[1]}" "2.35"; then
            fail "${elf#"$appdir"/} requires ${symbol_version}, above GLIBC_2.35"
        fi
        if [[ "$symbol_version" =~ ^GLIBCXX_([0-9]+\.[0-9]+\.[0-9]+)$ ]]; then
            local glibcxx_version="${BASH_REMATCH[1]}"
            local glibcxx_major_minor="${glibcxx_version%.*}"
            local glibcxx_patch="${glibcxx_version##*.}"
            if version_exceeds "$glibcxx_major_minor" "3.4" || { [[ "$glibcxx_major_minor" == "3.4" ]] && ((10#$glibcxx_patch > 30)); }; then
                fail "${elf#"$appdir"/} requires ${symbol_version}, above GLIBCXX_3.4.30"
            fi
        fi
    done < <(grep -oE 'GLIBC(XX)?_[0-9]+\.[0-9]+(\.[0-9]+)?' <<<"$version_info" | sort -u)
}

check_closure() {
    local elf="$1" closure
    closure="$(env -u LD_LIBRARY_PATH ldd "$elf" 2>&1)" || fail "ldd failed for ${elf#"$appdir"/}: $closure"
    if grep -Fq 'not found' <<<"$closure"; then
        fail "unresolved library closure for ${elf#"$appdir"/}: $closure"
    fi
}

mapfile -d '' candidate_files < <(find "$appdir" -type f -print0)
elf_files=()
for candidate in "${candidate_files[@]}"; do
    if check_elf_architecture "$candidate"; then
        elf_files+=("$candidate")
        check_runpaths "$candidate"
        check_symbol_versions "$candidate"
        check_closure "$candidate"
    fi
done
((${#elf_files[@]} > 0)) || fail "AppDir contains no ELF files"

# Check dynamic plugins separately so a future change cannot accidentally omit them.
for plugin in "$appdir/usr/lib/amiberry/libcapsimage.so" "$appdir/usr/lib/amiberry/libfloppybridge.so" "$appdir/usr/lib/amiberry/qemu-uae.so"; do
    check_elf_architecture "$plugin" || fail "plugin is not an ELF file: ${plugin#"$appdir"/}"
    check_closure "$plugin"
done

while IFS= read -r -d '' bundled_library; do
    case "$(basename "$bundled_library")" in
        libc.so.*|libm.so.*|libpthread.so.*|librt.so.*|libdl.so.*|ld-linux*.so*|libstdc++.so.*|libgcc_s.so.*|libGL.so.*|libGLX.so.*|libEGL.so.*|libGLES*.so.*|libdrm.so.*|libgbm.so.*|libvulkan.so.*|libX*.so.*|libxcb*.so.*|libwayland-*.so.*|libasound.so.*|libpulse.so.*|libjack.so.*|libpipewire-*.so.*|libspa-*.so.*|libsndio.so.*|libdbus-1.so.*)
            fail "forbidden bundled host runtime library: ${bundled_library#"$appdir"/}"
            ;;
    esac
done < <(find "$appdir" \( -type f -o -type l \) -name 'lib*.so*' -print0)

printf 'AppDir verification passed: %s (%s)\n' "$appdir" "$arch"
