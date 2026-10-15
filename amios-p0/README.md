# AmiOS Wayland Foundation — P0 test install

This branch is an **isolated upstream Amiberry baseline** for Debian. It does **not** contain the proposed Wayland-native window exporter or independent retained-RTG/native-screen presentation. Those remain development tasks. The goal of P0 is to verify a known baseline without changing the working R17E7 installation.

## Install/build

```bash
sudo apt update
sudo apt install -y git cmake ninja-build build-essential pkg-config
git clone --branch amios/wayland-foundation-p0 https://github.com/das-portal/amiberry.git ~/AmiOS84-test
cd ~/AmiOS84-test
bash amios-p0/build-test.sh
```

If CMake reports missing dependencies, install the named Debian development packages following upstream Amiberry's build guide. Do not replace the existing AmiOS68K binary.

The script builds under `build-amios-p0`, copies the executable to `~/.local/opt/amios-amiberry-p0/amiberry`, and saves configure/build logs under `~/.local/state/amios-p0/`.

## Controlled manual checks

Use a copy of the existing UAE config. Launch the new binary **after stopping the old emulator**. Verify: RTG Workbench starts, EditPad input works, ProTracker native PAL opens and closes, switching back to Workbench works, and no pointer capture problems occur. Vanilla Amiberry will show normal emulator windows; individual Wayland exports are not expected.

## Evidence to retain

```bash
uname -a
echo "session=$XDG_SESSION_TYPE desktop=$XDG_CURRENT_DESKTOP"
cmake --version
```

Keep `configure.log` and `build.log` if anything fails. No guest OS, ROM, disk image, or user configuration is packaged.

## Next implementation milestone

A separate experimental patch will add independently retained RTG and chipset presentation with explicit state logging, followed by a Wayland exporter proof of concept. This P0 is not that patch.
