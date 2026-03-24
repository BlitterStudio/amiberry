# Android Platform Limitations

The Android version of Amiberry has some differences from the desktop (Linux/macOS/Windows) builds due to platform constraints.

## Disabled Features

### Serial Port Emulation
Serial port support is excluded on Android. The host serial device APIs are not available.

### Network Stack (bsdsocket)
TCP/IP emulation via bsdsocket is not available. The SLIRP network stack and direct socket emulation are disabled.

### CRT Shader Effects
Six CRT shader effects are disabled on Android due to GPU performance constraints:
- CRT Lottes
- CRT Hyllian
- CRT Caligari
- CRT Easymode Halation
- CRT Guest Advanced
- CRT NewPixie

The remaining shader effects (scanlines, aperture grille, etc.) work normally.

### Fast Memory Mapping (natmem)
The native memory mapping optimization used on desktop platforms is disabled on Android. This means the JIT compiler uses a different, slightly slower memory access path. This is necessary to avoid out-of-memory issues with Android's virtual memory management.

### FPU Signaling NaN Detection
Hardware-based signaling NaN (SNAN) detection is disabled. This is a platform-independent limitation but particularly affects Android due to the ARM64 FPU behavior.

## Architecture Support

The Android build supports:
- **arm64-v8a** (64-bit ARM) — primary target, includes JIT compiler
- **x86_64** — for emulators and Chromebooks

32-bit ARM (armeabi-v7a) is not currently supported. The minimum SDK is API 29 (Android 10), which is predominantly 64-bit.

## Storage

Amiberry requests the `MANAGE_EXTERNAL_STORAGE` permission to access ROM images, disk images, and game archives in user-chosen directories. If denied, the app operates within its scoped storage directory at:

```
/storage/emulated/0/Android/data/com.blitterstudio.amiberry/files/
```

## Performance Notes

- The JIT compiler is available on ARM64 devices and provides much faster than native M68K speeds
- GPU-intensive configurations (RTG high resolution, multiple bitplanes) may be slower than on desktop
- Floppy drive sounds are supported but may add slight audio latency on some devices
