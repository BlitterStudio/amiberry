# libretro/ — RetroArch Integration

## OVERVIEW

RetroArch core wrapper enabling Amiberry to run as a libretro core. Provides headless platform replacement (no GUI, no window) with libretro-standard callbacks for video, audio, and input. Dual build system: CMake (integrated) + standalone Makefile.

## STRUCTURE

```
libretro/
├── [Core Wrapper]
│   ├── libretro-core.cpp        # Main libretro API: retro_init/run/deinit
│   ├── libretro.h               # Libretro API definitions
│   ├── libretro-glue.h/cpp      # Amiberry ↔ libretro bridge
│   ├── libretro-mapper.h/cpp    # Input mapping (libretro → UAE)
│   ├── libretro-graph.h/cpp     # Graphics utilities
│   ├── libretro-dc.h/cpp        # Disc control interface
│   └── core-options.h           # Core option definitions
├── [Build]
│   ├── Makefile                 # Standalone build (625 lines)
│   └── CMakeLists.txt           # CMake integration
├── [Platform Replacement]
│   └── (files in src/osdep/libretro/)
│       ├── amiberry_platform_internal.h  # No-op platform stubs
│       ├── gfx_platform_internal.h       # Software buffer + video_cb()
│       ├── input_platform.cpp            # Libretro input
│       ├── mp3decoder.cpp                # Libretro audio decoder
│       └── writelog.cpp                  # Libretro logging
├── libretro-common/             # Shared libretro utilities
│   ├── audio/                   # DSP filters
│   ├── compat/                  # Compatibility layers
│   ├── formats/                 # CHD/XML parsers
│   ├── include/                 # Common headers
│   ├── streams/                 # File/memory streams
│   └── test/                    # Unit tests (Check framework, NOT run in CI)
└── libco/                       # Cooperative threading (coroutines)
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Libretro API implementation | `libretro-core.cpp` | `retro_run()` is the main loop |
| Input mapping | `libretro-mapper.cpp` | Maps RetroArch buttons → UAE input |
| Graphics output | `src/osdep/libretro/gfx_platform_internal.h` | `video_cb()` callback |
| Core options | `core-options.h` | RetroArch UI settings |
| Platform stubs | `src/osdep/libretro/` | Complete replacement of host platform |

## CONVENTIONS

- Outputs: `.so` (Linux), `.dylib` (macOS), `.dll` (Windows)
- JIT **disabled** for x86_64 libretro builds (Makefile lines 361-368)
- Platform files live in `src/osdep/libretro/`, NOT in `libretro/`
- Fake 1920x1080 display enumerated for resolution selection
- `libretro-common/` is shared upstream code — minimize modifications

## ANTI-PATTERNS

- **DO NOT** enable JIT in x86_64 libretro builds
- **DO NOT** add GUI elements — libretro is headless, use core options instead
- Unit tests in `libretro-common/test/` exist but are **NOT executed in CI**
