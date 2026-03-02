# Step 07: Event System Migration

## Objective

Migrate all event handling from SDL2 event types and structures to SDL3 equivalents. This is a high-risk step due to the structural change in window events (sub-events become top-level events) and the pervasive rename of all event type constants.

## Prerequisites

- Step 06 complete (window, display, and renderer migrated)

## Files to Modify

| File | Event Handling Role |
|------|-------------------|
| `src/osdep/amiberry.cpp` | Main emulation event loop (~80 event-related lines) |
| `src/osdep/gui/main_window.cpp` | GUI event loop, ImGui event processing |
| `src/osdep/imgui/controller_map.cpp` | Controller mapping event handling |
| `src/osdep/amiberry_platform_internal_host.h` | Platform-specific event handling, drop files |
| `src/osdep/on_screen_joystick.cpp` | Touch event processing |

## Detailed Changes

### 1. Event Type Constant Renames

All SDL event type constants change from `SDL_*` to `SDL_EVENT_*`:

**Keyboard Events**:
| SDL2 | SDL3 |
|------|------|
| `SDL_KEYDOWN` | `SDL_EVENT_KEY_DOWN` |
| `SDL_KEYUP` | `SDL_EVENT_KEY_UP` |
| `SDL_TEXTINPUT` | `SDL_EVENT_TEXT_INPUT` |
| `SDL_TEXTEDITING` | `SDL_EVENT_TEXT_EDITING` |

**Mouse Events**:
| SDL2 | SDL3 |
|------|------|
| `SDL_MOUSEMOTION` | `SDL_EVENT_MOUSE_MOTION` |
| `SDL_MOUSEBUTTONDOWN` | `SDL_EVENT_MOUSE_BUTTON_DOWN` |
| `SDL_MOUSEBUTTONUP` | `SDL_EVENT_MOUSE_BUTTON_UP` |
| `SDL_MOUSEWHEEL` | `SDL_EVENT_MOUSE_WHEEL` |

**Joystick Events**:
| SDL2 | SDL3 |
|------|------|
| `SDL_JOYAXISMOTION` | `SDL_EVENT_JOYSTICK_AXIS_MOTION` |
| `SDL_JOYBUTTONDOWN` | `SDL_EVENT_JOYSTICK_BUTTON_DOWN` |
| `SDL_JOYBUTTONUP` | `SDL_EVENT_JOYSTICK_BUTTON_UP` |
| `SDL_JOYHATMOTION` | `SDL_EVENT_JOYSTICK_HAT_MOTION` |
| `SDL_JOYDEVICEADDED` | `SDL_EVENT_JOYSTICK_ADDED` |
| `SDL_JOYDEVICEREMOVED` | `SDL_EVENT_JOYSTICK_REMOVED` |

**Gamepad Events** (formerly Controller):
| SDL2 | SDL3 |
|------|------|
| `SDL_CONTROLLERBUTTONDOWN` | `SDL_EVENT_GAMEPAD_BUTTON_DOWN` |
| `SDL_CONTROLLERBUTTONUP` | `SDL_EVENT_GAMEPAD_BUTTON_UP` |
| `SDL_CONTROLLERAXISMOTION` | `SDL_EVENT_GAMEPAD_AXIS_MOTION` |
| `SDL_CONTROLLERDEVICEADDED` | `SDL_EVENT_GAMEPAD_ADDED` |
| `SDL_CONTROLLERDEVICEREMOVED` | `SDL_EVENT_GAMEPAD_REMOVED` |

**Touch Events**:
| SDL2 | SDL3 |
|------|------|
| `SDL_FINGERDOWN` | `SDL_EVENT_FINGER_DOWN` |
| `SDL_FINGERUP` | `SDL_EVENT_FINGER_UP` |
| `SDL_FINGERMOTION` | `SDL_EVENT_FINGER_MOTION` |

**Other Events**:
| SDL2 | SDL3 |
|------|------|
| `SDL_QUIT` | `SDL_EVENT_QUIT` |
| `SDL_DROPFILE` | `SDL_EVENT_DROP_FILE` |
| `SDL_DROPCOMPLETE` | `SDL_EVENT_DROP_COMPLETE` |

### 2. Window Events (Major Structural Change)

**SDL2**: All window events are `SDL_WINDOWEVENT` with sub-type in `event.window.event`.
**SDL3**: Each window event is a separate top-level event type.

```cpp
// Before (SDL2):
case SDL_WINDOWEVENT:
    switch (event.window.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            // handle focus gained
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            // handle focus lost
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            // handle minimize
            break;
        case SDL_WINDOWEVENT_RESTORED:
            // handle restore
            break;
        case SDL_WINDOWEVENT_RESIZED:
            // handle resize (event.window.data1, event.window.data2)
            break;
        case SDL_WINDOWEVENT_MOVED:
            // handle move
            break;
        case SDL_WINDOWEVENT_CLOSE:
            // handle close
            break;
        case SDL_WINDOWEVENT_ENTER:
            // handle mouse enter
            break;
        case SDL_WINDOWEVENT_LEAVE:
            // handle mouse leave
            break;
    }
    break;

// After (SDL3):
case SDL_EVENT_WINDOW_FOCUS_GAINED:
    // handle focus gained
    break;
case SDL_EVENT_WINDOW_FOCUS_LOST:
    // handle focus lost
    break;
case SDL_EVENT_WINDOW_MINIMIZED:
    // handle minimize
    break;
case SDL_EVENT_WINDOW_RESTORED:
    // handle restore
    break;
case SDL_EVENT_WINDOW_RESIZED:
    // handle resize (event.window.data1, event.window.data2)
    break;
case SDL_EVENT_WINDOW_MOVED:
    // handle move
    break;
case SDL_EVENT_WINDOW_CLOSE_REQUESTED:  // Note: renamed from CLOSE
    // handle close
    break;
case SDL_EVENT_WINDOW_MOUSE_ENTER:  // Note: renamed from ENTER
    // handle mouse enter
    break;
case SDL_EVENT_WINDOW_MOUSE_LEAVE:  // Note: renamed from LEAVE
    // handle mouse leave
    break;
```

**Key location**: `amiberry.cpp` lines ~1665-1690.

### 3. Event Structure Field Changes

**Keyboard events**:
```cpp
// Before (SDL2):
event.key.keysym.sym      // SDL_Keycode
event.key.keysym.scancode  // SDL_Scancode
event.key.keysym.mod       // modifier flags

// After (SDL3):
event.key.key             // SDL_Keycode (flattened, no keysym sub-struct)
event.key.scancode        // SDL_Scancode
event.key.mod             // modifier flags
```

The `event.key.keysym` sub-struct is removed in SDL3. Fields move directly to `event.key`.

**Mouse events**:
```cpp
// Before (SDL2):
event.button.which        // device ID (Uint32)
event.motion.which        // device ID (Uint32)
SDL_TOUCH_MOUSEID         // constant for synthetic touch events

// After (SDL3):
event.button.which        // SDL_MouseID
event.motion.which        // SDL_MouseID
SDL_TOUCH_MOUSEID         // Check SDL3 — may be replaced with a function or different constant
```

**Touch events**:
```cpp
// Before (SDL2):
event.tfinger.touchId
event.tfinger.fingerId
event.tfinger.x, .y       // normalized 0.0-1.0
event.tfinger.dx, .dy

// After (SDL3):
event.tfinger.touchID      // Note capitalization change
event.tfinger.fingerID     // Note capitalization change
event.tfinger.x, .y       // normalized 0.0-1.0 (unchanged)
event.tfinger.dx, .dy     // unchanged
```

**Window events**:
```cpp
// Before (SDL2):
event.window.windowID
event.window.event         // sub-event type (removed in SDL3)
event.window.data1         // varies by sub-event
event.window.data2

// After (SDL3):
event.window.windowID     // unchanged
// No sub-event field — event type IS the specific window event
event.window.data1        // still available for resize/move events
event.window.data2
```

**Drop events**:
```cpp
// Before (SDL2):
event.drop.file            // char* — caller must SDL_free()
event.drop.windowID

// After (SDL3):
event.drop.data            // const char* — managed by SDL, do NOT free
event.drop.windowID
```

### 4. SDL_TOUCH_MOUSEID Filtering

Amiberry filters synthetic mouse events from touch input using `SDL_TOUCH_MOUSEID`:
```cpp
// Before (SDL2):
if (event.button.which == SDL_TOUCH_MOUSEID) continue;  // skip synthetic
if (event.motion.which == SDL_TOUCH_MOUSEID) continue;

// After (SDL3):
// SDL3 uses SDL_PEN_MOUSEID and SDL_TOUCH_MOUSEID differently
// Check: SDL3 may use SDL_TOUCH_MOUSEID = SDL_MOUSE_TOUCHID
// The exact constant name may differ — check SDL3 headers
if (event.button.which == SDL_TOUCH_MOUSEID) continue;
```

Verify the SDL3 header for the correct constant name. It may be `SDL_TOUCH_MOUSEID` or a different mechanism.

### 5. SDL_PollEvent / SDL_PushEvent

```cpp
// Before and After:
SDL_PollEvent(&event);   // Unchanged
SDL_PushEvent(&event);   // Unchanged
SDL_PeepEvents(...);     // Unchanged in basic usage
```

### 6. Scancode and Keycode Constants

SDL3 renames scancode and keycode constants:
| SDL2 | SDL3 |
|------|------|
| `SDLK_a` | `SDLK_A` (capitalized) |
| `SDLK_RETURN` | `SDLK_RETURN` (unchanged) |
| `SDLK_ESCAPE` | `SDLK_ESCAPE` (unchanged) |
| `SDL_SCANCODE_*` | `SDL_SCANCODE_*` (unchanged for most) |

Check all `SDLK_*` and `SDL_SCANCODE_*` usage for renames. The alphabetic keys changed from lowercase to uppercase: `SDLK_a` → `SDLK_A`.

### 7. Text Input

```cpp
// Before (SDL2):
SDL_StartTextInput();
SDL_StopTextInput();
SDL_IsTextInputActive();

// After (SDL3):
SDL_StartTextInput(window);     // Takes window parameter
SDL_StopTextInput(window);      // Takes window parameter
SDL_TextInputActive(window);    // Renamed + takes window parameter
```

**Location**: `main_window.cpp` lines ~1315-1326 (Android text input management).

## Search Patterns

```bash
# Event type constants
grep -rn 'SDL_KEYDOWN\|SDL_KEYUP\|SDL_MOUSEMOTION\|SDL_MOUSEBUTTONDOWN' src/
grep -rn 'SDL_WINDOWEVENT\|SDL_WINDOWEVENT_' src/
grep -rn 'SDL_JOYBUTTONDOWN\|SDL_JOYAXISMOTION\|SDL_JOYHATMOTION' src/
grep -rn 'SDL_CONTROLLERBUTTON\|SDL_CONTROLLERAXIS' src/
grep -rn 'SDL_FINGERDOWN\|SDL_FINGERUP\|SDL_FINGERMOTION' src/
grep -rn 'SDL_DROPFILE\|SDL_QUIT\|SDL_TEXTINPUT' src/

# Event structure fields
grep -rn 'event\.key\.keysym' src/
grep -rn 'event\.tfinger\.' src/
grep -rn 'event\.drop\.file' src/
grep -rn 'SDL_TOUCH_MOUSEID' src/

# Text input
grep -rn 'SDL_StartTextInput\|SDL_StopTextInput\|SDL_IsTextInputActive' src/
```

## Acceptance Criteria

1. All event type constants use SDL3 `SDL_EVENT_*` naming
2. Window events restructured from nested switch to flat top-level cases
3. Keyboard event field access updated (`event.key.keysym.sym` → `event.key.key`)
4. Touch event field names updated
5. Drop file handling updated (no `SDL_free()`, use `event.drop.data`)
6. Keyboard input works (all keys, modifiers)
7. Mouse input works (movement, buttons, wheel)
8. Joystick and gamepad events processed correctly
9. Touch input works on Android (on-screen joystick, ImGui touch scrolling)
10. File drop works on desktop platforms
11. Window focus/minimize/restore/resize/close events handled correctly
12. Text input works in ImGui text fields
