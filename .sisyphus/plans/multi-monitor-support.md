# Implementation Plan: Multi-Monitor Support (#1593)

## Status

**PR #1856 merged** — Display selection and window movement complete. True multi-window rendering deferred to follow-up PR.

### Completed in PR #1856

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Foundation — constants, array expansion | ✅ Done |
| 1 | Host display enumeration with SDL_DisplayID | ✅ Done |
| 2 | Per-monitor window management (partial) | ✅ Single-window positioning done |
| 3 | Per-monitor renderer accessor | ✅ `get_renderer(monid)` added |
| 4 | RTG board → monitor wiring (partial) | ✅ Config plumbing done |
| 5 | Configuration persistence | ✅ Done |
| 6 | GUI panels | ✅ Done |
| 7 | Input routing | ✅ Done |

### Remaining for Follow-up PR

| Phase | Description | Status |
|-------|-------------|--------|
| 2b | Second SDL_Window creation for monid > 0 | 🔲 Not started |
| 3b | Per-monitor renderer instances | 🔲 Not started |
| 4b | RTG pixel routing to secondary window | 🔲 Not started |

---

## Lessons Learned from PR #1856

### 1. P96 State Arrays: Board Index vs Monitor ID

**Critical distinction:** Picasso96 state arrays (`picasso96_state[]`, `picasso_vidinfo[]`, `adisplays[]`) are indexed by **board index** (0 = first RTG board), not by `monitor_id`.

The `monitor_id` setting controls which **window** receives the rendered output, not which state slot the board uses. When `monitor_id=1`, the board still uses `picasso96_state[0]` for its state, but the output should go to `AMonitors[1]`'s window.

**Wrong:**
```cpp
const int monid = currprefs.rtgboards[idx].monitor_id;
const struct picasso96_state_struct *state = &picasso96_state[monid]; // WRONG
```

**Correct:**
```cpp
const struct picasso96_state_struct *state = &picasso96_state[idx]; // Board index
// Later: render to AMonitors[monitor_id].amiga_window
```

### 2. Window Positioning on Mode Switch

The "window already open" path in `create_windows()` kept the current position for windowed mode. When switching from native→RTG with different display assignments, the window stayed on the native display.

**Fix:** Check whether current position falls within target display's bounds. If not, center on target display.

### 3. Config `enabled` Flag Leak

The post-load compatibility `memcpy` from monitor 0 to unconfigured monitors copied `enabled=true`, causing spurious `gfx_monitorN_*` keys on save.

**Fix:** Reset `enabled=false` after the memcpy for unconfigured slots.

### 4. Vblank Timing Needs Display Context

`display_param_init()` and `target_getcurrentvblankrate()` used hardcoded display 0 for vblank queries. With per-monitor display selection, they must resolve the correct display via `getdisplay()`.

### 5. Array Iteration Safety

`Displays[]` now has 11 entries (MAX_DISPLAYS=10 + 1 sentinel). Range-based `for` loops that don't check for null `monitorname` will iterate into empty slots and crash on null `DisplayModes`.

**Pattern:**
```cpp
// WRONG: iterates all 11 entries including empty ones
for (const auto& Display : Displays) { ... }

// CORRECT: stops at first empty slot
for (int i = 0; i < MAX_DISPLAYS && Displays[i].monitorname; i++) { ... }
```

---

## Architecture Overview

### Three-Layer Display Model

```
Host Displays (physical)     Amiga Monitors (windows)     Amiga Displays (state)
┌──────────────────────┐    ┌────────────────────────┐    ┌─────────────────────┐
│ Displays[0..N]       │◄───│ AMonitors[0..3]        │───►│ adisplays[0..3]     │
│ (SDL_DisplayID)      │    │ (SDL_Window per mon)   │    │ (picasso_on, vbuf)  │
│ MAX_DISPLAYS=10      │    │ MAX_AMIGAMONITORS=4    │    │ MAX_AMIGADISPLAYS=4 │
└──────────────────────┘    └────────────────────────┘    └─────────────────────┘
                                     ▲
                                     │ monitor_id
                            ┌────────┴───────────┐
                            │ rtgboards[0..3]     │
                            │ (RTG board → monitor│
                            │  assignment)        │
                            └─────────────────────┘
```

### Key Index Spaces

| Index Space | Meaning | Used For |
|-------------|---------|----------|
| **Board index** (`idx`) | Which RTG hardware board (0 = first) | `picasso96_state[idx]`, `picasso_vidinfo[idx]`, `gfxmem_banks[idx]` |
| **Monitor ID** (`monitor_id`) | Which Amiga monitor window | `AMonitors[monitor_id]`, window placement |
| **Display index** | Which host physical display | `Displays[display_idx]`, `SDL_DisplayID` |

**Critical:** Board state and monitor routing are separate concerns.

---

## Remaining Work: True Multi-Window Rendering

### Phase 2b: Second Window Creation

**Goal:** Create a second SDL_Window when `monid > 0` and a monitor is activated.

**Current state:** `create_windows()` has guards for `monid > 0` (KMSDRM/Android skip), but no code path actually triggers secondary window creation.

**What needs to change:**

1. **Activation trigger** — When RTG board with `monitor_id > 0` becomes active, call `create_windows(&AMonitors[monitor_id])`

2. **Window lifecycle** — Track which monitors have open windows via `AMonitors[i].active`

3. **Platform guards** — KMSDRM and Android skip secondary windows (already implemented)

**Files:**
- `src/osdep/gfx_window.cpp` — `create_windows()` already handles `monid > 0`, just needs callers
- `src/osdep/picasso96.cpp` — `picasso_enablescreen()` should trigger window creation for assigned monitor
- `src/gfxboard.cpp` — `gfxboard_toggle()` activation path

### Phase 3b: Per-Monitor Renderer Instances

**Goal:** Each `AMonitors[i]` has its own `IRenderer` instance with independent texture/shader state.

**Current state:** `get_renderer(monid)` exists but always returns the global `g_renderer`.

**What needs to change:**

1. **Move renderer to AmigaMonitor** — `std::unique_ptr<IRenderer> renderer` in struct

2. **Per-window surface** — `amiga_surface` is currently global; needs to be per-monitor

3. **Renderer factory** — `create_renderer()` takes `AmigaMonitor*` and creates renderer bound to that window

4. **GL context sharing** — For OpenGL renderer, either:
   - Shared GL context across all windows (`SDL_GL_MakeCurrent()` per window)
   - Separate GL context per window (simpler but more resources)

**Files:**
- `src/osdep/amiberry_gfx.h` — Add `renderer` to `AmigaMonitor`
- `src/osdep/amiberry_gfx.cpp` — Remove global `g_renderer`, update `get_renderer()`
- `src/osdep/renderer_factory.cpp` — Accept `AmigaMonitor*` parameter
- `src/osdep/opengl_renderer.cpp` — Handle per-window GL context
- `src/osdep/sdl_renderer.cpp` — Already window-bound, minimal changes

### Phase 4b: RTG Pixel Routing

**Goal:** RTG render thread sends pixel data to the correct monitor's window/surface.

**Current state:** RTG renders to the single window regardless of `monitor_id`.

**What needs to change:**

1. **Render thread routing** — `render_thread()` resolves `monitor_id` from `rtgboards[idx].monitor_id` and uses `AMonitors[monitor_id].renderer`

2. **Surface selection** — `gfx_lock_picasso()` returns the correct monitor's surface

3. **Present to correct window** — `present_frame(monid)` uses the correct window's renderer

**Files:**
- `src/osdep/picasso96.cpp` — `render_thread()`, `picasso_flushpixels()`
- `src/osdep/amiberry_gfx.cpp` — `gfx_lock_picasso()` per-monitor surface

---

## WinUAE Reference for Multi-Window

| Feature | WinUAE File | Key Lines |
|---------|-------------|-----------|
| Per-monitor window creation | `od-win32/win32gfx.cpp` | 3640-3714 |
| Close all monitors (reverse order) | `od-win32/win32gfx.cpp` | 2267 |
| Per-monitor frame iteration | `od-win32/win32gfx.cpp` | 2941-2993 |
| Per-monitor D3D device | `od-win32/render.h` | `AmigaMonitor::dd` |
| RTG board → monitor lookup | `od-win32/picasso96_win.cpp` | 1235 |

---

## Recommended Implementation Order for Follow-up

```
Phase 2b → Phase 3b → Phase 4b
```

1. **Phase 2b first** — Get second window opening/closing correctly before worrying about what renders into it
2. **Phase 3b second** — Per-monitor renderer instances so each window has its own rendering state
3. **Phase 4b last** — Route RTG pixels to the correct window's renderer

---

## Out of Scope

- Special monitors (A2024, genlock) on secondary displays
- Save states with multi-monitor state
- Multiple native (chipset) displays — only one chipset output exists
- Hot-plug display support
- Libretro multi-monitor
