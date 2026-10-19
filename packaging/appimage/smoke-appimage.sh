#!/usr/bin/env bash
# Runtime smoke coverage for a packaged AppImage. It deliberately checks the
# executable's reported behavior rather than its packaged source layout.
set -euo pipefail

usage() {
    echo "Usage: $0 --image <path> --mode mount|extract" >&2
    exit 2
}

image=
mode=
while [[ $# -gt 0 ]]; do
    case "$1" in
        --image)
            [[ $# -ge 2 ]] || usage
            image=$2
            shift 2
            ;;
        --mode)
            [[ $# -ge 2 ]] || usage
            mode=$2
            shift 2
            ;;
        *)
            usage
            ;;
    esac
done

[[ -n "$image" && -f "$image" ]] || { echo "AppImage does not exist: $image" >&2; exit 1; }
case "$mode" in
    mount|extract) ;;
    *) usage ;;
esac

image="$(cd "$(dirname "$image")" && pwd)/$(basename "$image")"
chmod +x "$image"

work="$(mktemp -d)"
sibling_home=
cleanup() {
    rm -rf "$work" "${sibling_home:-}"
}
trap cleanup EXIT
mkdir -p "$work/unrelated" "$work/initial-home"

line_value() {
    local key=$1 output=$2
    grep -m 1 "^${key}=" "$output" | cut -d= -f2- | tr -d '\r'
}

require_value() {
    local key=$1 output=$2 value
    value="$(line_value "$key" "$output" || true)"
    [[ -n "$value" ]] || { echo "Missing $key in $output" >&2; cat "$output" >&2; exit 1; }
    printf '%s\n' "$value"
}

assert_within() {
    local value=$1 root=$2 description=$3
    case "$value" in
        "$root"|"$root"/*) ;;
        *) echo "$description escaped $root: $value" >&2; exit 1 ;;
    esac
}

assert_not_mount_path() {
    local key=$1 output=$2 value
    value="$(require_value "$key" "$output")"
    case "$value" in
        *'/.mount_'*) echo "$key retained a transient AppImage mount path: $value" >&2; exit 1 ;;
    esac
}

assert_image_root() {
    local key=$1 output=$2 image_prefix=$3 value
    value="$(require_value "$key" "$output")"
    case "$value" in
        /usr|/usr/*) echo "$key resolved to host package root: $value" >&2; exit 1 ;;
    esac
    assert_within "$value" "$image_prefix" "$key"
}

assert_appimage_paths() {
    local output=$1 check_data_root=${2:-true} image_prefix key value

    [[ "$(require_value portable_mode "$output")" == "0" ]] \
        || { echo "AppImage unexpectedly enabled marker portable mode" >&2; exit 1; }
    [[ "$(require_value portable_toggle_available "$output")" == "0" ]] \
        || { echo "AppImage portable toggle remained available" >&2; exit 1; }
    value="$(require_value portable_toggle_unavailable_reason "$output")"
    [[ "$value" == *AppImage* ]] \
        || { echo "Portable toggle was not disabled for the AppImage: $value" >&2; exit 1; }

    image_prefix="$(require_value install_prefix "$output")"
    case "$image_prefix" in
        /usr|/usr/*) echo "install_prefix resolved to host package root: $image_prefix" >&2; exit 1 ;;
    esac
    assert_image_root relocated_data_root "$output" "$image_prefix"
    assert_image_root relocated_plugins_root "$output" "$image_prefix"
    if [[ "$check_data_root" == true ]]; then
        assert_image_root data_dir "$output" "$image_prefix"
    fi
    for key in controllers roms whdboot; do
        assert_image_root "${key}_seed_source_0" "$output" "$image_prefix"
    done

    value="$(require_value effective_ca_bundle "$output")"
    [[ -r "$value" ]] || { echo "effective_ca_bundle is not readable: $value" >&2; exit 1; }

    for key in home_dir settings_dir config_path user_plugins_dir controllers_path whdboot_path \
        rom_path floppy_path harddrive_path cdrom_path; do
        assert_not_mount_path "$key" "$output"
    done
}


run_dump() {
    local output=$1 executable=$2 home=$3
    local runtime_args=()
    [[ "$mode" == extract ]] && runtime_args+=(--appimage-extract-and-run)
    (
        cd "$work/unrelated"
        env -u AMIBERRY_HOME_DIR -u AMIBERRY_CONFIG_DIR -u AMIBERRY_DATA_DIR \
            -u AMIBERRY_PLUGINS_DIR -u XDG_CONFIG_HOME -u XDG_DATA_HOME \
            -u XDG_CACHE_HOME -u XDG_STATE_HOME -u XDG_RUNTIME_DIR \
            HOME="$home" "$executable" "${runtime_args[@]}" --dump-paths
    ) > "$output"
}

run_override_dump() {
    local output=$1
    local runtime_args=()
    [[ "$mode" == extract ]] && runtime_args+=(--appimage-extract-and-run)
    (
        cd "$work/unrelated"
        env AMIBERRY_HOME_DIR="$explicit_home" AMIBERRY_CONFIG_DIR="$explicit_config" \
            AMIBERRY_DATA_DIR="$explicit_data" AMIBERRY_PLUGINS_DIR="$explicit_plugins" \
            XDG_CONFIG_HOME="$xdg_config" XDG_DATA_HOME="$xdg_data" XDG_CACHE_HOME="$xdg_cache" \
            XDG_STATE_HOME="$xdg_state" XDG_RUNTIME_DIR="$xdg_runtime" HOME="$work/initial-home" \
            "$image" "${runtime_args[@]}" --dump-paths
    ) > "$output"
}

# Exercise the AppImage runtime through a symlink from an unrelated working
# directory. Initial HOME/XDG locations are temporary and the AppImage's own
# environment must supply no Amiberry/XDG override.
link="$work/Amiberry-link.AppImage"
ln -s "$image" "$link"
run_dump "$work/default-paths.txt" "$link" "$work/initial-home"
assert_appimage_paths "$work/default-paths.txt"
for key in home_dir settings_dir config_path user_plugins_dir controllers_path whdboot_path \
    rom_path floppy_path harddrive_path cdrom_path; do
    assert_within "$(require_value "$key" "$work/default-paths.txt")" "$work" "$key"
done

# AppImage's documented sibling .home convention must redirect all writable
# state when no explicit Amiberry or XDG variables are present.
sibling_home="${image}.home"
[[ ! -e "$sibling_home" ]] \
    || { echo "Refusing to reuse existing AppImage home directory: $sibling_home" >&2; exit 1; }
mkdir -p "$sibling_home"
run_dump "$work/home-paths.txt" "$image" "$work/initial-home"
assert_appimage_paths "$work/home-paths.txt"
for key in home_dir settings_dir config_path user_plugins_dir controllers_path whdboot_path \
    rom_path floppy_path harddrive_path cdrom_path; do
    assert_within "$(require_value "$key" "$work/home-paths.txt")" "$sibling_home" "$key"
done

# Explicit Amiberry variables must override the sibling home for their
# corresponding paths. Keep the XDG fixtures distinct so precedence regressions
# cannot accidentally pass.
fixtures="$work/overrides"
explicit_home="$fixtures/amiberry-home"
explicit_config="$fixtures/amiberry-config"
explicit_data="$fixtures/amiberry-data"
explicit_plugins="$fixtures/amiberry-plugins"
xdg_config="$fixtures/xdg-config"
xdg_data="$fixtures/xdg-data"
xdg_cache="$fixtures/xdg-cache"
xdg_state="$fixtures/xdg-state"
xdg_runtime="$fixtures/xdg-runtime"
mkdir -p "$explicit_home" "$explicit_config" "$explicit_data" "$explicit_plugins" \
    "$xdg_config" "$xdg_data" "$xdg_cache" "$xdg_state" "$xdg_runtime"
run_override_dump "$work/override-paths.txt"
assert_appimage_paths "$work/override-paths.txt" false
[[ "$(require_value home_dir "$work/override-paths.txt")" == "$explicit_home" ]] \
    || { echo "AMIBERRY_HOME_DIR lost precedence" >&2; exit 1; }
[[ "$(require_value settings_dir "$work/override-paths.txt")" == "$xdg_config/amiberry" ]] \
    || { echo "XDG_CONFIG_HOME lost precedence for settings_dir" >&2; exit 1; }
[[ "$(require_value config_path "$work/override-paths.txt")" == "$explicit_config" ]] \
    || { echo "AMIBERRY_CONFIG_DIR lost precedence" >&2; exit 1; }
[[ "$(require_value data_dir "$work/override-paths.txt")" == "$explicit_data" ]] \
    || { echo "AMIBERRY_DATA_DIR lost precedence" >&2; exit 1; }
plugin_path="$(require_value plugin_search_path_0 "$work/override-paths.txt")"
case "$plugin_path" in
    "$explicit_plugins"|"$explicit_plugins"/*) ;;
    *) echo "AMIBERRY_PLUGINS_DIR lost precedence: $plugin_path" >&2; exit 1 ;;
esac

echo "AppImage $mode runtime smoke test passed"
