#!/usr/bin/env bash
set -euo pipefail

case "$(uname -m)" in
    x86_64|aarch64) ;;
    *)
        echo "AppDir verifier test skipped on unsupported architecture"
        exit 0
        ;;
esac

command -v cc >/dev/null 2>&1 || { echo "cc is required" >&2; exit 1; }
true_bin="$(type -P true)"
[[ -n "$true_bin" ]] || { echo "Cannot locate external true binary" >&2; exit 1; }
ldd_bin="$(type -P ldd)"
[[ -n "$ldd_bin" ]] || { echo "Cannot locate ldd" >&2; exit 1; }

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
appdir="$work/AppDir"
mkdir -p "$work/bin"
cat > "$work/bin/ldd" <<WRAPPER
#!/usr/bin/env bash
exec env LD_LIBRARY_PATH="$work" "$ldd_bin" "\$@"
WRAPPER
chmod 0755 "$work/bin/ldd"

mkdir -p "$appdir/usr/bin" "$appdir/usr/lib/amiberry" \
    "$appdir/usr/share/applications" "$appdir/usr/share/metainfo"
for directory in data controllers roms whdboot; do
    mkdir -p "$appdir/usr/share/amiberry/$directory"
    : > "$appdir/usr/share/amiberry/$directory/fixture"
done

install -m 0755 "$true_bin" "$appdir/usr/bin/amiberry"
install -m 0755 "$true_bin" "$appdir/AppRun"
printf 'Icon=com.blitterstudio.amiberry\nExec=amiberry %%f\n' > "$appdir/com.blitterstudio.amiberry.desktop"
printf 'icon\n' > "$appdir/com.blitterstudio.amiberry.png"
for size in 64 128 256 512; do
    icon_dir="$appdir/usr/share/icons/hicolor/${size}x${size}/apps"
    mkdir -p "$icon_dir"
    cp "$appdir/com.blitterstudio.amiberry.png" "$icon_dir/com.blitterstudio.amiberry.png"
done
ln -s com.blitterstudio.amiberry.png "$appdir/.DirIcon"
cp "$appdir/com.blitterstudio.amiberry.desktop" "$appdir/usr/share/applications/com.blitterstudio.amiberry.desktop"
printf '<id>com.blitterstudio.amiberry</id>\n<launchable type="desktop-id">com.blitterstudio.amiberry.desktop</launchable>\n' \
    > "$appdir/usr/share/metainfo/com.blitterstudio.amiberry.metainfo.xml"
printf 'not an ELF\n' > "$appdir/usr/lib/amiberry/libSDL3.so"
printf 'not an ELF\n' > "$appdir/usr/lib/amiberry/libSDL3_image.so"

cat > "$work/dependency.c" <<'SOURCE'
const char *fixtureDependencyVersion(void) {
    return "1";
}
SOURCE
cat > "$work/plugin.c" <<'SOURCE'
extern const char *fixtureDependencyVersion(void);
const char *appimage_verifier_fixture(void) {
    return fixtureDependencyVersion();
}
SOURCE
cc -shared -fPIC "$work/dependency.c" -o "$work/libfixturedep.so"
cc -shared -fPIC -Wl,--no-as-needed "$work/plugin.c" \
    -L"$work" -lfixturedep -o "$work/plugin.so"
for plugin in libcapsimage.so libfloppybridge.so qemu-uae.so; do
    install -m 0644 "$work/plugin.so" "$appdir/usr/lib/amiberry/$plugin"
done

if PATH="$work/bin:$PATH" bash "$repo_root/packaging/appimage/verify-appdir.sh" --arch "$(uname -m)" "$appdir" > "$work/output" 2>&1; then
    echo "AppDir verifier accepted a plugin dependency resolved from the host" >&2
    exit 1
fi
if ! grep -F 'non-excluded dependency' "$work/output" >/dev/null ||
    ! grep -F 'libfixturedep.so' "$work/output" >/dev/null; then
    cat "$work/output" >&2
    exit 1
fi

echo "AppDir verifier host-leakage test passed"
