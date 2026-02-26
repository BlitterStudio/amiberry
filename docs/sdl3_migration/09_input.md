# Step 09: Joystick, Gamepad, and Input Migration

## Objective

Migrate from SDL2's GameController/Joystick API to SDL3's unified Gamepad API. This affects ~130 call sites across 6 files, with the controller mapping GUI (`controller_map.cpp`, 79 calls) being the heaviest.

## Prerequisites

- Step 08 complete (audio migrated)

## Files to Modify

| File | Call Sites | Key Operations |
|------|-----------|----------------|
| `src/osdep/amiberry_input.cpp` | ~42 | Device enumeration, open/close, axis/button queries |
| `src/osdep/amiberry_input.h` | ~5 | Type declarations |
| `src/osdep/imgui/controller_map.cpp` | ~79 | Controller mapping GUI, rendering, event handling |
| `src/osdep/imgui/input.cpp` | ~1 | Input configuration panel |
| `src/osdep/input_platform_internal_host.h` | ~5 | Hints, accelerometer |
| `src/osdep/on_screen_joystick.cpp` | ~0 | Minimal impact (uses Amiberry's input injection API) |

## Detailed Changes

### 1. GameController → Gamepad Rename

All `SDL_GameController*` functions and types are renamed to `SDL_Gamepad*`:

**Types**:
| SDL2 | SDL3 |
|------|------|
| `SDL_GameController*` | `SDL_Gamepad*` |
| `SDL_GameControllerButton` | `SDL_GamepadButton` |
| `SDL_GameControllerAxis` | `SDL_GamepadAxis` |

**Functions**:
| SDL2 | SDL3 |
|------|------|
| `SDL_GameControllerOpen(index)` | `SDL_OpenGamepad(instance_id)` |
| `SDL_GameControllerClose(gc)` | `SDL_CloseGamepad(gamepad)` |
| `SDL_GameControllerName(gc)` | `SDL_GetGamepadName(gamepad)` |
| `SDL_GameControllerNameForIndex(i)` | `SDL_GetGamepadNameForID(instance_id)` |
| `SDL_GameControllerGetJoystick(gc)` | `SDL_GetGamepadJoystick(gamepad)` |
| `SDL_GameControllerMapping(gc)` | `SDL_GetGamepadMapping(gamepad)` |
| `SDL_GameControllerGetButton(gc, btn)` | `SDL_GetGamepadButton(gamepad, btn)` |
| `SDL_GameControllerGetAxis(gc, axis)` | `SDL_GetGamepadAxis(gamepad, axis)` |
| `SDL_GameControllerGetStringForButton(btn)` | `SDL_GetGamepadStringForButton(btn)` |
| `SDL_GameControllerGetStringForAxis(axis)` | `SDL_GetGamepadStringForAxis(axis)` |
| `SDL_GameControllerGetButtonFromString(s)` | `SDL_GetGamepadButtonFromString(s)` |
| `SDL_GameControllerGetAxisFromString(s)` | `SDL_GetGamepadAxisFromString(s)` |
| `SDL_IsGameController(index)` | `SDL_IsGamepad(instance_id)` |
| `SDL_GameControllerAddMappingsFromFile(f)` | `SDL_AddGamepadMappingsFromFile(f)` |
| `SDL_GameControllerAddMapping(s)` | `SDL_AddGamepadMapping(s)` |

**Button constants**:
| SDL2 | SDL3 |
|------|------|
| `SDL_CONTROLLER_BUTTON_A` | `SDL_GAMEPAD_BUTTON_SOUTH` |
| `SDL_CONTROLLER_BUTTON_B` | `SDL_GAMEPAD_BUTTON_EAST` |
| `SDL_CONTROLLER_BUTTON_X` | `SDL_GAMEPAD_BUTTON_WEST` |
| `SDL_CONTROLLER_BUTTON_Y` | `SDL_GAMEPAD_BUTTON_NORTH` |
| `SDL_CONTROLLER_BUTTON_BACK` | `SDL_GAMEPAD_BUTTON_BACK` |
| `SDL_CONTROLLER_BUTTON_GUIDE` | `SDL_GAMEPAD_BUTTON_GUIDE` |
| `SDL_CONTROLLER_BUTTON_START` | `SDL_GAMEPAD_BUTTON_START` |
| `SDL_CONTROLLER_BUTTON_LEFTSTICK` | `SDL_GAMEPAD_BUTTON_LEFT_STICK` |
| `SDL_CONTROLLER_BUTTON_RIGHTSTICK` | `SDL_GAMEPAD_BUTTON_RIGHT_STICK` |
| `SDL_CONTROLLER_BUTTON_LEFTSHOULDER` | `SDL_GAMEPAD_BUTTON_LEFT_SHOULDER` |
| `SDL_CONTROLLER_BUTTON_RIGHTSHOULDER` | `SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER` |
| `SDL_CONTROLLER_BUTTON_DPAD_UP` | `SDL_GAMEPAD_BUTTON_DPAD_UP` |
| `SDL_CONTROLLER_BUTTON_DPAD_DOWN` | `SDL_GAMEPAD_BUTTON_DPAD_DOWN` |
| `SDL_CONTROLLER_BUTTON_DPAD_LEFT` | `SDL_GAMEPAD_BUTTON_DPAD_LEFT` |
| `SDL_CONTROLLER_BUTTON_DPAD_RIGHT` | `SDL_GAMEPAD_BUTTON_DPAD_RIGHT` |

**Axis constants**:
| SDL2 | SDL3 |
|------|------|
| `SDL_CONTROLLER_AXIS_LEFTX` | `SDL_GAMEPAD_AXIS_LEFTX` |
| `SDL_CONTROLLER_AXIS_LEFTY` | `SDL_GAMEPAD_AXIS_LEFTY` |
| `SDL_CONTROLLER_AXIS_RIGHTX` | `SDL_GAMEPAD_AXIS_RIGHTX` |
| `SDL_CONTROLLER_AXIS_RIGHTY` | `SDL_GAMEPAD_AXIS_RIGHTY` |
| `SDL_CONTROLLER_AXIS_TRIGGERLEFT` | `SDL_GAMEPAD_AXIS_LEFT_TRIGGER` |
| `SDL_CONTROLLER_AXIS_TRIGGERRIGHT` | `SDL_GAMEPAD_AXIS_RIGHT_TRIGGER` |

### 2. Joystick API Changes

**Functions**:
| SDL2 | SDL3 |
|------|------|
| `SDL_JoystickOpen(index)` | `SDL_OpenJoystick(instance_id)` |
| `SDL_JoystickClose(joy)` | `SDL_CloseJoystick(joy)` |
| `SDL_JoystickName(joy)` | `SDL_GetJoystickName(joy)` |
| `SDL_JoystickNumAxes(joy)` | `SDL_GetNumJoystickAxes(joy)` |
| `SDL_JoystickNumButtons(joy)` | `SDL_GetNumJoystickButtons(joy)` |
| `SDL_JoystickNumHats(joy)` | `SDL_GetNumJoystickHats(joy)` |
| `SDL_JoystickNumBalls(joy)` | `SDL_GetNumJoystickBalls(joy)` |
| `SDL_JoystickGetButton(joy, btn)` | `SDL_GetJoystickButton(joy, btn)` |
| `SDL_JoystickGetAxis(joy, axis)` | `SDL_GetJoystickAxis(joy, axis)` |
| `SDL_JoystickGetHat(joy, hat)` | `SDL_GetJoystickHat(joy, hat)` |
| `SDL_JoystickGetGUID(joy)` | `SDL_GetJoystickGUID(joy)` |
| `SDL_JoystickGetAxisInitialState(joy, axis, &state)` | Check SDL3 equivalent |
| `SDL_JoystickInstanceID(joy)` | `SDL_GetJoystickID(joy)` |

**Types**:
| SDL2 | SDL3 |
|------|------|
| `SDL_JoystickGUID` | `SDL_GUID` |
| `SDL_JoystickID` | `SDL_JoystickID` (unchanged) |

### 3. Device Enumeration (Critical Change)

**SDL2**: Index-based — iterate `0` to `SDL_NumJoysticks()-1`.
**SDL3**: ID-based — get array of instance IDs.

```cpp
// Before (SDL2):
int num = SDL_NumJoysticks();
for (int i = 0; i < num; i++) {
    if (SDL_IsGameController(i)) {
        SDL_GameController* gc = SDL_GameControllerOpen(i);
        const char* name = SDL_GameControllerNameForIndex(i);
    } else {
        SDL_Joystick* joy = SDL_JoystickOpen(i);
        const char* name = SDL_JoystickNameForIndex(i);
    }
}

// After (SDL3):
int count;
SDL_JoystickID* joysticks = SDL_GetJoysticks(&count);
for (int i = 0; i < count; i++) {
    SDL_JoystickID id = joysticks[i];
    if (SDL_IsGamepad(id)) {
        SDL_Gamepad* gp = SDL_OpenGamepad(id);
        const char* name = SDL_GetGamepadNameForID(id);
    } else {
        SDL_Joystick* joy = SDL_OpenJoystick(id);
        const char* name = SDL_GetJoystickNameForID(id);
    }
}
SDL_free(joysticks);
```

**Key location**: `init_joystick()` in `amiberry_input.cpp` — this is the central device enumeration function.

### 4. GUID String Conversion

```cpp
// Before (SDL2):
char guid_str[64];
SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid_str, sizeof(guid_str));

// After (SDL3):
char guid_str[64];
SDL_GUIDToString(SDL_GetJoystickGUID(joy), guid_str, sizeof(guid_str));
```

### 5. Controller Mapping File Loading

```cpp
// Before:
SDL_GameControllerAddMappingsFromFile("controllers/gamecontrollerdb.txt");

// After:
SDL_AddGamepadMappingsFromFile("controllers/gamecontrollerdb.txt");
```

The mapping file format is the same. SDL3 is backward-compatible with SDL2 mapping files.

### 6. Joystick Axis/Hat Constants

| SDL2 | SDL3 |
|------|------|
| `SDL_JOYSTICK_AXIS_MIN` | `SDL_JOYSTICK_AXIS_MIN` (unchanged: -32768) |
| `SDL_JOYSTICK_AXIS_MAX` | `SDL_JOYSTICK_AXIS_MAX` (unchanged: 32767) |
| `SDL_HAT_UP` | `SDL_HAT_UP` (unchanged) |
| `SDL_HAT_DOWN` | `SDL_HAT_DOWN` (unchanged) |
| `SDL_HAT_LEFT` | `SDL_HAT_LEFT` (unchanged) |
| `SDL_HAT_RIGHT` | `SDL_HAT_RIGHT` (unchanged) |
| `SDL_HAT_CENTERED` | `SDL_HAT_CENTERED` (unchanged) |

### 7. Accelerometer Hint

```cpp
// Before (SDL2):
SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");

// After (SDL3):
// This hint is REMOVED in SDL3. Accelerometers are no longer exposed as joystick devices.
// Simply remove this line.
```

**Location**: `input_platform_internal_host.h`

### 8. Hot-Plug Events

Joystick/gamepad add/remove events use instance IDs in SDL3:

```cpp
// Before (SDL2):
case SDL_JOYDEVICEADDED:
    index = event.jdevice.which;  // This is a device index
    SDL_JoystickOpen(index);
    break;

// After (SDL3):
case SDL_EVENT_JOYSTICK_ADDED:
    instance_id = event.jdevice.which;  // This is an instance ID
    SDL_OpenJoystick(instance_id);
    break;
```

### 9. Controller Map GUI (`controller_map.cpp`)

This file has 79 SDL2 call sites. Changes needed:
1. All `SDL_GameController*` → `SDL_Gamepad*` function renames (~30 sites)
2. All `SDL_CONTROLLER_BUTTON_*` → `SDL_GAMEPAD_BUTTON_*` constants (~20 sites)
3. All `SDL_CONTROLLER_AXIS_*` → `SDL_GAMEPAD_AXIS_*` constants (~10 sites)
4. Rendering calls: `SDL_RenderCopy` → `SDL_RenderTexture` (already handled in Step 06, but verify)
5. Surface calls: already handled in Step 05
6. Event handling: already handled in Step 07

## Search Patterns

```bash
grep -rn 'SDL_GameController' src/
grep -rn 'SDL_CONTROLLER_BUTTON_\|SDL_CONTROLLER_AXIS_' src/
grep -rn 'SDL_JoystickOpen\|SDL_JoystickClose\|SDL_JoystickName' src/
grep -rn 'SDL_NumJoysticks\|SDL_IsGameController' src/
grep -rn 'SDL_JoystickNum\|SDL_JoystickGet' src/
grep -rn 'SDL_JoystickGUID\|SDL_JoystickGetGUID' src/
grep -rn 'SDL_HINT_ACCELEROMETER' src/
grep -rn 'SDL_JoystickInstanceID\|SDL_JoystickFromInstanceID' src/
```

## Acceptance Criteria

1. Physical joysticks detected and functional (axes, buttons, hats)
2. Physical gamepads detected with correct button/axis mapping
3. Controller mapping GUI (`controller_map.cpp`) displays correctly
4. Controller mapping GUI shows correct button/axis names
5. Hot-plug works (add/remove joystick during runtime)
6. Controller mapping files load correctly from `controllers/` directory
7. Custom button/axis mappings saved and restored correctly
8. On-screen joystick still works (uses Amiberry input injection, minimal SDL dependency)
9. Multiple controllers can be connected simultaneously
10. GUID-based device identification works correctly
