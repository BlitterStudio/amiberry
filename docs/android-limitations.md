# Android Platform Limitations

The Android version of Amiberry has some differences from the desktop (Linux/macOS/Windows) builds due to platform constraints.

The Android version is currently in Closed Testing on Google Play, available only to Ko-fi members. For availability, setup, touch controls, and the launcher, see the [Android wiki page](https://github.com/BlitterStudio/amiberry/wiki/Android).

## Disabled Features

These build options are forced off for Android in `CMakeLists.txt`:

- **CD32 Full Motion Video** — the MPEG decoder (`USE_LIBMPEG2`) is not built.
- **MP3 decoding** — `USE_MPG123` is off, so MP3 audio tracks in CUE images and the ZZ9000 MP3/MHI decoders are unavailable.
- **Genlock video, camera input, and ZZ9000 FFmpeg/WebP codecs** — `USE_VIDEOGRAB`, `USE_FFMPEG`, and `USE_WEBP` are off.
- **uaenet** — the pcap and TAP backends are not available. The built-in SLIRP stack and `bsdsocket.library` emulation are still included.
- **PowerPC** — `USE_PPC` and the QEMU-UAE plugin loader (`USE_QEMU_PPC`) are off.
- **IPC socket and GPIO LEDs** — `USE_IPC_SOCKET` and `USE_GPIOD` are off.

MIDI is built with PortMidi's null backend, so no host MIDI devices are available. Portable mode and the built-in updater are not offered on Android; updates come through Google Play.

### Shaders
The Android launcher's **Native Shader** list offers the built-in shaders (`none`, `tv`, `pc`, `lite`, `1084`) plus any `.glslp` presets and top-level `.glsl` files found in `Visuals/Shaders`. The built-in CRT effect switches to lighter shader and blur variants on mobile GPUs.

### Model presets
The launcher writes the whole machine a Quick Start model builds into its configuration (chipset extras such as the A3000's SCSI and Ramsey or the CD32's Akiko included), at the **Compatibility** level picked next to the model; new models start at the approximate level, as on the desktop. The CD32, CDTV and DraCo NVRAM files are kept in the emulator's `NVRAM` folder between launches. A few parts of a preset are not settings the launcher can write:
- the 64 KB SRAM card of the CDTV "Floppy drive and 64KB SRAM card" configuration;
- the CD drive of a CD32 or CDTV started without a disc.

Changing the model keeps the inserted disks and CD. A medium the new model has no drive for (floppies on a CD32, a CD on an A500) is ejected, and a message offers Undo. Quick Start opens in **Recent** once something was launched, in **WHDLoad** when games are installed, and in **Disk** otherwise.

### Settings coverage
The launcher's Settings tabs show every option of the shared settings layer that means something on Android: each tab's own controls first, then one section per settings group (CPU, MMU, FPU, CPU speed, JIT; Chipset, Advanced Chipset and Genlock, which start collapsed; Memory and ROMs; Display and Filters; Sound; Input and On-screen Controls; Floppy, CD and Filesystem), and a **Misc** tab for the miscellaneous options, expansions and the protection dongle. An option that is unavailable in the current machine is dimmed, and its section says why once. The second to fourth memory boards of a kind appear once the board before is fitted. Configured in the desktop GUI only, for now:
- RTG and accelerator boards (the board type is not yet in the launcher), and their memory and options;
- the disk swapper list, joystick ports 3 and 4, custom bezel images and the RTG shader;
- hotkeys.

Options for host windows, devices, serial ports, MIDI and PowerPC do not apply on Android and are not shown.

### FPU Signaling NaN Detection
Hardware-based signaling NaN (SNAN) detection is disabled. This is a platform-independent limitation but particularly affects Android due to the ARM64 FPU behavior.

## Architecture Support

The Android build supports:
- **arm64-v8a** (64-bit ARM) — primary target, includes JIT compiler
- **x86_64** — for emulators and Chromebooks

32-bit ARM (armeabi-v7a) is not currently supported. The minimum SDK is API 29 (Android 10), which is predominantly 64-bit.

## Storage

Amiberry can use the `MANAGE_EXTERNAL_STORAGE` (All Files Access) permission to access ROM images, disk images, and game archives in user-chosen directories. It is not requested at startup. On Android 11 and newer, the launcher shows a **Limited file access** banner with a **Grant Access** button that opens the system permission screen. Without the permission, the app works within its scoped storage directory, and imports through the system file picker still copy files there:

```
/storage/emulated/0/Android/data/com.blitterstudio.amiberry/files/
```

## Performance Notes

- The JIT compiler is available on ARM64 devices and provides much faster than native M68K speeds
- GPU-intensive configurations (RTG high resolution, multiple bitplanes) may be slower than on desktop
- Floppy drive sounds are supported but may add slight audio latency on some devices

## On-Screen Keyboard Controls

The on-screen keyboard has two independent ways to open it:

- **On-Screen Keyboard Button** shows the green **KB** touch button on the on-screen joystick (`amiberry.vkbd_touch_button`). It only appears while the on-screen joystick is shown.
- **On-Screen Keyboard Toggle Button** picks the controller button that opens and closes the keyboard. **Default** uses the native default binding; **None (disabled)** leaves the button available to the game.

Turning the KB button off keeps the gamepad toggle working, so a TV or other gamepad-only device can hide all touch controls and still type. The keyboard is off entirely only when the KB button is off and the toggle is **None (disabled)**. A gamepad connected at startup hides the on-screen joystick but leaves the keyboard available.

Saved configurations preserve Default, None and explicit button selections independently of remembered settings, both when opened in Settings and when launched from Configurations, Recent or a shared file. The same holds for their port, on-screen joystick, numeric keypad, mouse-map and analog mouse speed settings: remembered controls only fill the ones a configuration does not set, so older configurations without a toggle setting still inherit the remembered selection. Quick Start disk and CD launches and RP9 packages use the remembered controls. Configurations and remembered settings from older versions that had the keyboard switched off load with the KB button off and the toggle set to None.

While the keyboard is open, the D-pad or left stick moves focus, South presses the focused key, and East closes the keyboard. Focus stays fixed while South is held. Opening the emulator GUI closes the keyboard so controller releases consumed by the GUI cannot leave stale keyboard holds when emulation resumes.

A WHDLoad launch follows the same rule. WHDBooter chooses the hardware when the game starts, from its game database, the installed Kickstarts, or the game's own `<game>.uae` configuration. The launcher then applies its ports, on-screen controls, mouse map, analog mouse speed and shader, so its ports replace the ones WHDBooter assigns; whatever the game's own configuration sets explicitly takes precedence. Quick Start therefore shows only these launcher settings for a WHDLoad game, and names the game's configuration when it has one. `.lha`, `.lzh` and `.lzx` archives all boot.

For WHDLoad launches, the launcher's port choices also override the default controllers and mice in `amiberry.conf` (`default_controller1`–`4`, `default_mouse1`/`2`), which WHDBooter would otherwise assign.

## Desktop-Style Use on ChromeOS

On large-screen/desktop-class devices (Chromebooks, freeform windowing), Amiberry launches as a landscape tablet-size window; on phone-standard launches these preferences are inert and behavior is unchanged. Physical-keyboard users on QWERTZ-style layouts get correct key mapping (the physical key positions are passed through and the AmigaOS keymap performs the layout mapping), and synthetic Enter events generated by a light touchpad tap are filtered out so they never reach the emulated machine.

## Launcher Layout on Wide Screens

The launcher adapts to the window width (Material 3 window size classes). From 600 dp (a tablet held upright) the navigation rail replaces the bottom bar, and Settings and Files list their tabs and categories down the left beside the content. From 840 dp (a landscape tablet, a Chromebook window, Android TV) Configurations shows the list beside the selected configuration's summary and actions, and Quick Start shows the launch mode and media beside the notices and current settings. A phone keeps the single-column layout, also when held sideways. With a D-pad, Left and Right move between the panes, and Right from the navigation rail always enters the tab's content. On Android TV, focus starts in Quick Start's content, and OK on a game in the WHDLoad game list starts it.

## RTG Display Modes

When the host exposes only its native fullscreen mode, Picasso96 mode lists on Android are supplemented with a set of virtual resolutions (up to 2560x1440, subject to the configured RTG memory). These are marked with an `ANDROID` suffix.

## GUI Scale

The GUI defaults to roughly 70% of automatic DPI scaling on Android (the desktop default is unchanged). Adjust it with the **GUI scale** setting under Display defaults (50–200%).

## Building the APK on Windows

Windows-hosted Gradle builds relocate the CMake FetchContent dependency cache to a short same-drive path (`.amiberry-fc/<hash>/<abi>-<buildtype>` at the drive root, e.g. `D:/.amiberry-fc/`) to stay under Windows path-length limits. The directory is safe to delete; it is repopulated on the next configure. Other hosts keep the cache inside the source tree (`.fc/`).
