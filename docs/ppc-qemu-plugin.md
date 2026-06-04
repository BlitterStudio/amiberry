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
