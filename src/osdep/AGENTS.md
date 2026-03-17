# src/osdep/ — Platform Abstraction Layer

## OVERVIEW

~140 C/C++ files implementing OS-dependent platform abstraction: renderers, input, graphics, GUI, audio, networking, and file system. Supports host (SDL) and libretro (headless) targets.

## STRUCTURE

```
osdep/
├── [Renderer Abstraction — Factory Pattern]
│   ├── irenderer.h              # Abstract base (40+ virtual methods)
│   ├── renderer_factory.h/cpp   # Creates OpenGL or SDL renderer
│   ├── opengl_renderer.h/cpp    # OpenGL backend (1589 lines, shaders/overlays)
│   └── sdl_renderer.h/cpp       # SDL3 software backend (396 lines, minimal)
│
├── [Platform Indirection — Compile-Time Dispatch]
│   ├── amiberry_platform_internal.h      # → includes host or libretro impl
│   ├── gfx_platform_internal.h           # → graphics platform dispatch
│   ├── input_platform_internal.h         # → input platform dispatch
│   └── gui_handling_platform.h           # → GUI platform dispatch
│
├── [Graphics Subsystem]
│   ├── amiberry_gfx.h/cpp      # Main orchestration (AmigaMonitor, frame rendering)
│   ├── gfx_window.h/cpp        # Window lifecycle
│   ├── gl_platform.h/cpp       # GL function loading (desktop) / GLES3 (Android)
│   ├── gl_overlays.h/cpp       # LED/OSD overlay rendering
│   ├── external_shader.h/cpp   # Custom shader support
│   ├── shader_preset.h/cpp     # Multi-pass shader pipelines
│   └── crtemu.h/cpp            # CRT emulation shader
│
├── [Input]
│   ├── amiberry_input.h/cpp    # Device structs (di_joystick[], controller_mapping)
│   └── keyboard.cpp            # Keyboard event handling
│
├── [GUI — see imgui/AGENTS.md]
│   ├── gui/gui_handling.h      # GUI config structures (493 lines)
│   └── imgui/                  # 30+ ImGui panels
│
├── [Touch Controls]
│   ├── on_screen_joystick.h/cpp     # Touch D-pad + buttons
│   ├── on_screen_joystick_gl.h/cpp  # GL rendering for touch joystick
│   ├── vkbd/vkbd.h/cpp             # Virtual keyboard
│   └── vkbd/vkbd_gl.h/cpp          # GL rendering for vkbd
│
├── [File Dialogs]
│   ├── file_dialog.h/cpp       # Native OS file picker (NFD) with ImGuiFileDialog fallback
│   └── nfd_sdl3.h              # SDL3 adapter for NFD parent window handles
│
├── [Platform-Specific]
│   ├── macos_bookmarks.h/mm    # App Store security-scoped bookmarks
│   ├── amiberry_dbus.h/cpp     # Linux DBus interface
│   ├── amiberry_ipc*.cpp       # Unix domain socket IPC (60+ commands)
│   ├── target.h                # Windows headers, version macros
│   └── sysconfig.h             # System configuration flags
│
├── [Core Platform Files]
│   ├── amiberry.cpp            # Main entry + initialization (6171 lines)
│   ├── amiberry_update.cpp     # Self-update mechanism (Windows)
│   ├── amiberry_mem.cpp        # Natmem memory management
│   ├── amiberry_filesys.cpp    # Host filesystem emulation
│   ├── sigsegv_handler.cpp     # SIGSEGV handler for JIT faults
│   └── bsdsocket_host.cpp      # BSD socket (WSAAsyncSelect on Win, select() on POSIX)
│
└── libretro/                   # Headless platform replacement
    ├── amiberry_platform_internal.h  # No-op stubs
    ├── gfx_platform_internal.h       # Software buffer, video_cb()
    └── input_platform.cpp            # Libretro input
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add renderer feature | `irenderer.h` → impl in `opengl_renderer.cpp` | SDL renderer is minimal fallback |
| Platform-specific init | `amiberry_platform_internal_host.h` | Inline functions, compile-time dispatch |
| Window management | `gfx_window.cpp` | Open/close/resize |
| Shader work | `external_shader.cpp` or `shader_preset.cpp` | `filter.cpp` in imgui/ exposes params |
| Touch controls | `on_screen_joystick.cpp` | Registered as virtual joystick device |
| macOS sandbox | `macos_bookmarks.h/mm` | No-op stubs when not App Store build |
| Libretro adaptation | `libretro/` | Complete platform replacement |

## CONVENTIONS

- **Header indirection pattern**: `*_platform_internal.h` includes are driven by build-system macros (`OSDEP_*_PLATFORM_HEADER`)
- **Renderer is compile-time selected**: `USE_OPENGL` → OpenGLRenderer, else SDLRenderer
- **GL platform split**: Desktop loads function pointers via `SDL_GL_GetProcAddress`; Android uses GLES3 directly
- **Touch controls have dual renderers**: Each has `_gl.h/cpp` for OpenGL and fallback SDL path
- **`amiberry_` prefix** on all platform functions

## ANTI-PATTERNS

- **DO NOT** access `OpenGLRenderer` internals except from `imgui/filter.cpp` (known abstraction violation)
- **DO NOT** use `send_input_event()` for new input code — use `setjoystickstate()`/`setjoybuttonstate()`
- On Android, **DO NOT** destroy SDL renderer in `sdl_renderer.cpp` (reused across launches)
- **DO NOT** assume symlinks work — use `std::filesystem::copy()` on Windows/Android
