# QEMU-UAE PPC Plugin

Amiberry PPC emulation uses an external `qemu-uae` plugin built from QEMU 11.0.1 plus the UAE PPC patch deck. The plugin is loaded at runtime, so Amiberry can keep the emulator core and the QEMU-derived code in separate build and release pipelines.

## Architecture

- Amiberry owns the host-side ABI, configuration, board selection, runtime plugin discovery, and packaging integration.
- The `qemu-uae` plugin owns the QEMU PowerPC CPU implementation and exports the `qemu_uae_*` and `ppc_cpu_*` symbols used by Amiberry.
- QEMU-UAE PPC support is enabled by the Amiberry CMake options `USE_PPC=ON` and `USE_QEMU_PPC=ON`.
- Amiberry requires qemu-uae API `3.8` or newer. The major version must match exactly.

## Plugin Source

The initial plugin work is based on:

- QEMU release: https://download.qemu.org/qemu-11.0.1.tar.xz
- QEMU-UAE fork: https://github.com/reinauer/qemu-uae/tree/qemu-v11.0.1-uae
- UAE PPC patch deck: https://github.com/reinauer/uae-ppc-plugin
- WinUAE packaging reference: https://github.com/reinauer/WinUAE/commit/35a6df1537961a507bea0f1bd1ab9d0e3f711132

The preferred long-term layout is a separate Amiberry-owned plugin repository that tracks the upstream QEMU release, applies the UAE patch deck in order, and publishes per-platform plugin artifacts.

## Runtime Placement

Amiberry looks for a plugin named `qemu-uae` through the existing plugin loader. Platform packages should install the artifact in the normal Amiberry plugin directory:

- Linux and FreeBSD: `qemu-uae.so`
- macOS: `qemu-uae.dylib`
- Windows: `qemu-uae.dll`

Android and iOS currently build with PPC plugin support disabled because dynamic plugin loading is not wired for those targets.

## Runtime Dependencies

The QEMU-UAE plugin is dynamically loaded at runtime, so packages must ship both the plugin and the libraries needed to load it:

- macOS app bundles include `qemu-uae.dylib` under `Contents/Resources/plugins` and bundle dependent dylibs under `Contents/Frameworks`.
- Windows packages include `plugins/qemu-uae.dll`; plugin runtime DLLs such as GLib, GModule, zlib, and libwinpthread are installed beside `Amiberry.exe`.
- Linux DEB/RPM packages install `qemu-uae.so` under Amiberry's plugin library directory and declare required system packages, including libglib/GLib and zlib.

Package CI must verify both plugin presence and dependency resolution. A package that contains the plugin file but cannot load it is broken.

## Build Notes

Source builds do not download or build QEMU automatically. Build the plugin separately, then place it in the plugin directory or pass `-DQEMU_UAE_PLUGIN=/path/to/qemu-uae` to bundle a prebuilt artifact into the install/package layout.

Useful Amiberry configure combinations:

```bash
cmake -B build -DUSE_PPC=ON -DUSE_QEMU_PPC=ON
cmake -B build -DUSE_PPC=ON -DUSE_QEMU_PPC=ON -DQEMU_UAE_PLUGIN=/path/to/qemu-uae.so
cmake -B build-no-ppc -DUSE_PPC=OFF
cmake -B build-ppc-no-qemu -DUSE_PPC=ON -DUSE_QEMU_PPC=OFF
```

`USE_PPC=ON` enables PPC accelerator board support in Amiberry. `USE_QEMU_PPC=ON` enables runtime loading of the QEMU-UAE PPC implementation. PCem/QEMU device glue remains controlled by `USE_PCEM`.

`QEMU_UAE_PLUGIN` is optional. When set, CMake validates that the artifact exists and that QEMU PPC support is enabled, then installs it beside the other Amiberry plugins.

## Runtime Behavior

With a compatible plugin installed, selecting a CyberStorm PPC or Blizzard PPC board and enabling PPC CPU emulation should load the QEMU implementation. If the plugin is missing, too old, or missing required symbols, Amiberry logs the plugin error and falls back to another available PPC implementation or the dummy implementation.

Non-PPC builds hide PPC accelerator options, clear stale PPC config state, and do not save PPC-only config keys.

## Runtime Smoke Test

Requirements:

- A compatible `qemu-uae` plugin in the Amiberry plugins directory.
- CyberStorm PPC or Blizzard PPC ROMs.
- A PPC-capable AmigaOS setup.

Recommended checks:

1. Start Amiberry with `--log`.
2. Select a CyberStorm PPC or Blizzard PPC accelerator board.
3. Enable PPC CPU emulation and leave the PPC implementation on `Automatic`, or select `QEMU`.
4. Boot the PPC-capable system.
5. Confirm the log contains `PPC: Loading QEmu implementation`.
6. Confirm the log does not fall back to `PPC: Loading dummy implementation`.
7. Confirm the guest sees the expected PPC accelerator.

Missing-plugin check:

1. Remove or rename the `qemu-uae` plugin.
2. Boot the same PPC configuration with `--log`.
3. Confirm Amiberry reports the plugin load failure and falls back cleanly instead of crashing.

## Release Policy

The QEMU-UAE PPC plugin repository should publish artifacts for the desktop platforms that can load Amiberry dynamic plugins:

- Linux x86-64 and ARM64: `qemu-uae.so`
- macOS universal: `qemu-uae.dylib`
- Windows x64 and ARM64: `qemu-uae.dll`

Linux artifacts should be built from an older baseline such as Ubuntu 22.04 so they remain usable on newer supported distributions. macOS artifacts should be merged into a universal dylib before bundling into the universal app. Windows artifacts should be built with a UCRT/Clang toolchain compatible with Amiberry's llvm-mingw Windows builds.

Android and iOS remain unsupported for QEMU-UAE PPC until dynamic plugin loading and platform packaging constraints are solved. FreeBSD and Haiku are not release blockers for the first multi-platform plugin artifact set, but the host build must remain clean without bundled plugin artifacts.
