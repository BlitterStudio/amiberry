// Platform/device fixtures surround production routing included by test_osk_input.py.
#define AMIBERRY
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include "sdl_compat.h"
#include "imgui_osk.h"

constexpr int MAX_INPUT_DEVICES = 4, REMAP_BUTTONS = 32, analog_upper_bound = 32767;
constexpr int DIR_LEFT = 1, DIR_RIGHT = 2, DIR_UP = 4, DIR_DOWN = 8;
constexpr int IE_CDTV = 256, IE_INVERT = 512, JOYMOUSE_CDTV = 10, CYCLE_UNIT = 1;
#include "osk_mapping_under_test.inc"

struct SDL_Gamepad { Sint16 axes[6]{}; };
struct SDL_Joystick { Sint16 axes[6]{}; Uint8 hats[2]{}; };
struct didata {
    std::string name, guid;
    SDL_JoystickID joystick_id = 0;
    bool is_controller = true, hotkey_held = false;
    int axles = 6, buttons = SDL_GAMEPAD_BUTTON_COUNT;
    SDL_Gamepad* controller = nullptr;
    SDL_Joystick* joystick = nullptr;
    controller_mapping mapping{};
};
struct Prefs {
    bool vkbd_enabled = true, cpu_cycle_exact = false;
    char vkbd_toggle[128] = "leftstick";
    int inactive_input = 0, input_contact_bounce = 0;
    int input_joystick_deadzone = 33, input_joymouse_deadzone = 33, input_joymouse_multiplier = 100;
    struct { int mousemap = 0; } jports[4];
} currprefs, changed_prefs;
struct Hotkey { int button = 0; } quit_key, action_replay_key, fullscreen_key, minimize_key, screenshot_key, debugger_key;
struct Event { int type, data, unit = 1; };
constexpr int MAX_INPUT_SUB_EVENT = 8, ID_AXIS_OFFSET = 64;
using uae_u64 = unsigned long long;
constexpr uae_u64 ID_FLAG_INVERT = 1, ID_FLAG_AUTOFIRE = 2;
constexpr int HANDLE_IE_FLAG_AUTOFIRE = 1, HANDLE_IE_FLAG_CANSTOPPLAYBACK = 2;
constexpr int IDTYPE_JOYSTICK = 0, IDEV_WIDGET_AXIS = 0;
struct uae_input_device {
    bool enabled = true;
    int eventid[ID_AXIS_OFFSET + 6][MAX_INPUT_SUB_EVENT]{};
    uae_u64 flags[ID_AXIS_OFFSET + 6][MAX_INPUT_SUB_EVENT]{};
} joystick_settings[MAX_INPUT_DEVICES];
uae_input_device* joysticks = joystick_settings;
struct uae_input_device2 {
    int states[6][MAX_INPUT_SUB_EVENT + 1]{};
} joysticks2[MAX_INPUT_DEVICES];
Event events[1 + MAX_INPUT_DEVICES * 4]{};
bool testmode = false;
void inputdevice_testrecord(int, int, int, int, int, int) {}
void inprec_realtime() {}
didata di_joystick[MAX_INPUT_DEVICES];
SDL_Gamepad pads[MAX_INPUT_DEVICES];
SDL_Joystick sticks[MAX_INPUT_DEVICES];
int vkbd_button = SDL_GAMEPAD_BUTTON_LEFT_STICK, enter_gui_button = SDL_GAMEPAD_BUTTON_START;
int joystick_dead_zone = 8000, axisold[MAX_INPUT_DEVICES][256]{};
int joybutton[4]{}, joydir[4]{}, oleft[4]{}, oright[4]{}, otop[4]{}, obot[4]{};
int horizclear[4]{}, vertclear[4]{}, relativecount[4][2]{}, mouse_deltanoreset[4][2]{}, mouse_delta[4][2]{};
int input_record = 0, input_play = 0, bouncy = 0, bouncy_cycles = 0;
bool active = false, animating = false, joystick_refresh_needed = false;
int observed_osk = 0;
enum { AKS_ENTERGUI = 1, AKS_OSK, AKS_FREEZEBUTTON, AKS_TOGGLEWINDOWFULLWINDOW, AKS_SCREENSHOT_FILE, AKS_ENTERDEBUGGER };
using TCHAR = char;
constexpr int MAX_PENDING_EVENTS = 16;
struct Pending { int code = 0, state = 0; char* s = nullptr; } inputcode_pending[MAX_PENDING_EVENTS];

bool isfocus() { return true; }
bool vkbd_allowed(int) { return currprefs.vkbd_enabled; }
bool imgui_osk_is_active() { return active; }
bool imgui_osk_should_render() { return active || animating; }
bool imgui_osk_process(int state, int* key, int* pressed) {
    observed_osk = state; *key = *pressed = 0; return false;
}
void imgui_osk_hide() {
    active = false; animating = true;
    osk_control(0, 0, 0, 0, OskInputSource::Gamepad);
    osk_clear_controller_holds();
}
bool inputdevice_handle_inputcode_immediate(int, int) { return false; }
char* my_strdup(const char* text) { return strdup(text); }
void uae_quit() {} void minimizewindow(int) {} void joymousecounter(int) {}
int get_cycles() { return 0; }
int assigned_joyport(int id) { return id; }
int mousemap_mouse_device() { return 0; }
int mapped_mouse_delta = 0;
void setmousestate(int, int, int value, int) { mapped_mouse_delta += value; }
void write_log(const char*, ...) {}
bool inputdevice_devicechange(Prefs*) { return true; }
Sint16 SDL_GetGamepadAxis(SDL_Gamepad* pad, SDL_GamepadAxis axis) { return pad->axes[axis]; }
Sint16 SDL_GetJoystickAxis(SDL_Joystick* stick, int axis) { return stick->axes[axis]; }
int SDL_GetNumJoystickHats(SDL_Joystick*) { return 2; }
Uint8 SDL_GetJoystickHat(SDL_Joystick* stick, int hat) { return stick->hats[hat]; }
void core_button(int, int, int);
void core_direction(int, int, int, int = 32767, bool = false);
void core_mouse(int, int, int, int);
void inputdevice_add_inputcode(int, int, const char*);
void setjoybuttonstate(int id, int button, int state) {
    const int extra = SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT * 2;
    if (button == extra + 1) inputdevice_add_inputcode(AKS_ENTERGUI, state, nullptr);
    else if (button == extra + 4) inputdevice_add_inputcode(AKS_OSK, state, nullptr);
    else if (button == SDL_GAMEPAD_BUTTON_SOUTH) core_button(id, 0, state);
    else if (button == SDL_GAMEPAD_BUTTON_EAST) core_button(id, 1, state);
    else if (button == SDL_GAMEPAD_BUTTON_DPAD_UP) core_direction(id, DIR_UP, state);
    else if (button == SDL_GAMEPAD_BUTTON_DPAD_DOWN) core_direction(id, DIR_DOWN, state);
    else if (button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) core_direction(id, DIR_LEFT, state);
    else if (button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) core_direction(id, DIR_RIGHT, state);
}
void handle_input_event(int event, int state, int max, int) {
    if (!event) return;
    const auto& binding = events[event];
    if (binding.type & 8) core_mouse(binding.unit - 1, binding.data, state, max);
    else core_direction(binding.unit - 1, binding.data, state, max, true);
}
#include "osk_native_under_test.inc"

static void reset_fixture() {
    currprefs = Prefs{};
    osk_controllers.clear();
    osk_hats.clear();
    joysticks = joystick_settings;
    vkbd_button = SDL_GAMEPAD_BUTTON_LEFT_STICK;
    enter_gui_button = SDL_GAMEPAD_BUTTON_START;
    active = animating = false;
    mapped_mouse_delta = 0;
    observed_osk = 0;
    memset(joybutton, 0, sizeof joybutton); memset(joydir, 0, sizeof joydir);
    memset(oleft, 0, sizeof oleft); memset(oright, 0, sizeof oright);
    memset(otop, 0, sizeof otop); memset(obot, 0, sizeof obot);
    memset(axisold, 0, sizeof axisold);
    memset(mouse_delta, 0, sizeof mouse_delta); memset(mouse_deltanoreset, 0, sizeof mouse_deltanoreset);
    for (auto& event : inputcode_pending) event = Pending{};
    for (int id = 0; id < MAX_INPUT_DEVICES; ++id) {
        joysticks[id] = uae_input_device{};
        joysticks2[id] = uae_input_device2{};
        const int first = 1 + id * 4;
        events[first] = {0, DIR_LEFT | DIR_RIGHT, id + 1};
        events[first + 1] = {0, DIR_UP | DIR_DOWN, id + 1};
        events[first + 2] = {8, 0, id + 1};
        events[first + 3] = {8, 1, id + 1};
        joysticks[id].eventid[ID_AXIS_OFFSET][0] = first;
        joysticks[id].eventid[ID_AXIS_OFFSET + 1][0] = first + 1;
        pads[id] = SDL_Gamepad{}; sticks[id] = SDL_Joystick{};
        auto& did = di_joystick[id]; did = didata{};
        did.name = did.guid = "fixture"; did.joystick_id = id + 1;
        did.controller = &pads[id]; did.joystick = &sticks[id];
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) did.mapping.button[b] = did.mapping.button_unmasked[b] = b;
        for (int a = 0; a < SDL_GAMEPAD_AXIS_COUNT; ++a) did.mapping.axis[a] = a;
        did.mapping.hotkey_button = did.mapping.quit_button = did.mapping.reset_button = -1;
        sync_controller_shortcuts(&did);
    }
    osk_control(0, 0, 0, 0, OskInputSource::Gamepad);
    osk_clear_controller_holds();
}
static void map_mouse(int id) {
    joysticks[id].eventid[ID_AXIS_OFFSET][0] = 3 + id * 4;
    joysticks[id].eventid[ID_AXIS_OFFSET + 1][0] = 4 + id * 4;
}
static void button(int id, int button, bool down, bool raw = false) {
    SDL_Event event{};
    if (raw) { event.jbutton.which = id + 1; event.jbutton.button = button; event.jbutton.down = down; handle_joy_button_event(event); }
    else { event.gbutton.which = id + 1; event.gbutton.button = button; event.gbutton.down = down; handle_controller_button_event(event); }
}
static void axis(int id, int axis, int value, bool raw = false) {
    SDL_Event event{};
    if (raw) { sticks[id].axes[axis] = value; event.jaxis.which = id + 1; event.jaxis.axis = axis; event.jaxis.value = value; handle_joy_axis_motion_event(event); }
    else { pads[id].axes[axis] = value; event.gaxis.which = id + 1; event.gaxis.axis = axis; event.gaxis.value = value; handle_controller_axis_motion_event(event); }
}
static void hat(int id, int value) {
    sticks[id].hats[0] = value;
    SDL_Event event{}; event.jhat.which = id + 1; event.jhat.hat = 0; event.jhat.value = value;
    handle_joy_hat_motion_event(event);
}
static int pending(int code) { int count = 0; for (const auto& e : inputcode_pending) count += e.code == code; return count; }

static void test_mousemap_ownership(bool raw) {
    reset_fixture();
    currprefs.jports[0].mousemap = 1;
    currprefs.input_joystick_deadzone = currprefs.input_joymouse_deadzone = 50;
    di_joystick[0].mapping.is_retroarch = raw;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 9000, raw);
    assert(mapped_mouse_delta == (raw ? 9 : 0));
    osk_clear_controller_holds();
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000, raw);
    if (raw) {
        assert(observed_osk == 0 && mapped_mouse_delta == 33);
    } else {
        assert((observed_osk & OSK_RIGHT) && mapped_mouse_delta == 0);
    }
}

static void test_center_drift(bool raw, bool reacquire) {
    reset_fixture();
    currprefs.input_joystick_deadzone = 50;
    currprefs.input_joymouse_deadzone = 1;
    const int physical_axis = raw ? 3 : SDL_GAMEPAD_AXIS_LEFTX;
    if (raw) {
        di_joystick[0].mapping.is_retroarch = true;
        di_joystick[0].mapping.axis[SDL_GAMEPAD_AXIS_LEFTX] = physical_axis;
        di_joystick[0].mapping.axis[SDL_GAMEPAD_AXIS_RIGHTY] = 0;
    }
    if (reacquire) {
        if (raw) sticks[0].axes[physical_axis] = -14000;
        else pads[0].axes[physical_axis] = -14000;
        osk_clear_controller_holds();
    } else {
        axis(0, physical_axis, -14000, raw);
        assert(joydir[0] == 0);
    }
    active = true;
    axis(0, physical_axis, 24000, raw);
    assert(observed_osk & OSK_RIGHT);
    assert(joydir[0] == 0);
}

static void test_osk_axis_handoff(bool raw, bool mouse, bool closed_event) {
    reset_fixture();
    if (mouse) map_mouse(0);
    const int physical_axis = raw ? 3 : SDL_GAMEPAD_AXIS_LEFTX;
    if (raw) {
        di_joystick[0].mapping.is_retroarch = true;
        di_joystick[0].mapping.axis[SDL_GAMEPAD_AXIS_LEFTX] = physical_axis;
        di_joystick[0].mapping.axis[SDL_GAMEPAD_AXIS_RIGHTY] = 0;
    }
    active = true;
    axis(0, physical_axis, 24000, raw);
    assert(observed_osk & OSK_RIGHT);
    imgui_osk_hide();
    if (closed_event) {
        animating = false;
        axis(0, physical_axis, 28000, raw);
        assert(joydir[0] == 0 && mouse_delta[0][0] == 0);
    }
    osk_clear_controller_holds(); // Focus reacquisition with the same held gesture.
    active = true; animating = false;
    axis(0, physical_axis, 30000, raw);
    assert((observed_osk & OSK_RIGHT) && joydir[0] == 0 && mouse_delta[0][0] == 0);

    imgui_osk_hide();
    animating = false;
    axis(0, physical_axis, 0, raw);
    axis(0, physical_axis, 24000, raw);
    if (mouse) assert(mouse_delta[0][0] != 0);
    else assert(joydir[0] & DIR_RIGHT);
}

int main() {
    for (bool raw : {false, true})
        for (bool mouse : {false, true})
            for (bool closed_event : {false, true})
                test_osk_axis_handoff(raw, mouse, closed_event);

    reset_fixture();
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert(joydir[0] & DIR_RIGHT);
    active = true;
    imgui_osk_hide();
    osk_clear_controller_holds();
    active = true; animating = false;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 30000);
    assert(observed_osk == 0 && (joydir[0] & DIR_RIGHT));
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
    assert(joydir[0] == 0);
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert((observed_osk & OSK_RIGHT) && joydir[0] == 0);

    reset_fixture(); di_joystick[0].mapping.is_retroarch = true;
    active = true;
    hat(0, SDL_HAT_UP);
    imgui_osk_hide();
    osk_clear_controller_holds();
    active = true; animating = false;
    hat(0, SDL_HAT_UP | SDL_HAT_RIGHT);
    assert((observed_osk & OSK_RIGHT) && joydir[0] == 0);
    imgui_osk_hide(); animating = false;
    hat(0, SDL_HAT_UP | SDL_HAT_LEFT);
    assert(joydir[0] == DIR_LEFT); // Only the new, unowned direction reaches UAE.
    hat(0, SDL_HAT_CENTERED);
    assert(joydir[0] == 0);
    hat(0, SDL_HAT_UP);
    assert(joydir[0] == DIR_UP);
    reset_fixture();
    debugger_key.button = SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    active = true;
    button(0, SDL_GAMEPAD_BUTTON_RIGHT_STICK, true); // Global hotkeys outrank the open keyboard.
    assert(pending(AKS_ENTERDEBUGGER) == 1);
    debugger_key.button = 0;

    reset_fixture();
    di_joystick[0].mapping.lstick_axis_x_invert = true;
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert((observed_osk & OSK_LEFT) && !(observed_osk & OSK_RIGHT));
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
    assert(observed_osk == 0);
    active = false; animating = false;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert(joydir[0] & DIR_LEFT); // Gameplay inversion is applied by its own reader.
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
    assert(joydir[0] == 0);
    di_joystick[0].mapping.lstick_axis_x_invert = false;


    for (bool raw : {false, true})
        for (bool reacquire : {false, true})
            test_center_drift(raw, reacquire);

    test_mousemap_ownership(false);
    test_mousemap_ownership(true);

    // An unused digital-joystick deadzone must not reserve a mouse-mapped axis.
    reset_fixture(); map_mouse(0);
    currprefs.input_joystick_deadzone = 1;
    currprefs.input_joymouse_deadzone = 50;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 16383); // Mouse boundary is still neutral.
    assert(mouse_delta[0][0] == 0);
    osk_clear_controller_holds();
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert((observed_osk & OSK_RIGHT) && mouse_delta[0][0] == 0);

    // A secondary custom mouse binding still owns input ignored by the first binding.
    reset_fixture();
    currprefs.input_joystick_deadzone = 50;
    currprefs.input_joymouse_deadzone = 1;
    joysticks[0].eventid[ID_AXIS_OFFSET][1] = 3;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 1200);
    assert(joydir[0] == 0 && mouse_delta[0][0] != 0);
    osk_clear_controller_holds();
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert(observed_osk == 0 && mouse_delta[0][0] != 0);
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
    assert(joydir[0] == 0 && mouse_delta[0][0] == 0);

    reset_fixture();
    joysticks[0].eventid[ID_AXIS_OFFSET][0] = 0;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    osk_clear_controller_holds();
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 30000);
    assert((observed_osk & OSK_RIGHT) && joydir[0] == 0);

    // Focus can be acquired before UAE input mappings have been initialized.
    reset_fixture();
    joysticks = nullptr;
    pads[0].axes[SDL_GAMEPAD_AXIS_LEFTX] = 24000;
    osk_clear_controller_holds();
    joysticks = joystick_settings;
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 30000);
    assert((observed_osk & OSK_RIGHT) && joydir[0] == 0);

    reset_fixture();
    button(0, SDL_GAMEPAD_BUTTON_EAST, true);
    button(0, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    active = true;
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, true);
    button(0, SDL_GAMEPAD_BUTTON_EAST, false);
    button(0, SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
    assert(joybutton[0] == 0 && joydir[0] == 0);
    assert(observed_osk & OSK_BUTTON); // Unowned releases must not release South's key.
    button(1, SDL_GAMEPAD_BUTTON_SOUTH, true);
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, false);
    assert(observed_osk & OSK_BUTTON);
    button(1, SDL_GAMEPAD_BUTTON_SOUTH, false);
    assert(!(observed_osk & OSK_BUTTON));

    reset_fixture();
    active = true; di_joystick[0].is_controller = false;
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, true, true);
    handle_joy_device_event(2, true);
    assert(observed_osk & OSK_BUTTON); // An idle gamepad removal cannot erase a plain joystick hold.
    button(1, SDL_GAMEPAD_BUTTON_SOUTH, true);
    button(1, SDL_GAMEPAD_BUTTON_SOUTH, false);
    assert(observed_osk & OSK_BUTTON);
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, false, true);
    assert(!(observed_osk & OSK_BUTTON));

    for (int initial : {12000, 32000}) {
        reset_fixture(); map_mouse(0);
        axis(0, SDL_GAMEPAD_AXIS_LEFTX, initial);
        assert(mouse_delta[0][0] != 0 && mouse_deltanoreset[0][0]);
        osk_clear_controller_holds();
        active = true;
        if (initial == 32000) axis(0, SDL_GAMEPAD_AXIS_LEFTX, 12000);
        axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
        assert(mouse_delta[0][0] == 0 && !mouse_deltanoreset[0][0]);
    }

    reset_fixture(); map_mouse(0);
    currprefs.input_joymouse_deadzone = 1;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 1200);
    assert(mouse_delta[0][0] != 0);
    osk_clear_controller_holds();
    active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert(observed_osk == 0 && mouse_delta[0][0] != 0);
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
    assert(mouse_delta[0][0] == 0 && !mouse_deltanoreset[0][0]);

    reset_fixture(); active = true;
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    imgui_osk_hide();
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, true);
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, false);
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 12000);
    assert(joybutton[0] == 0 && joydir[0] == 0);
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 0);
    assert(joydir[0] == 0);

    reset_fixture(); active = true;
    auto& retro = di_joystick[0]; retro.mapping.is_retroarch = true;
    retro.mapping.axis[SDL_GAMEPAD_AXIS_LEFTX] = 3;
    retro.mapping.axis[SDL_GAMEPAD_AXIS_RIGHTX] = 0;
    retro.mapping.lstick_axis_x_invert = true;
    axis(0, SDL_GAMEPAD_AXIS_RIGHTX, 24000); // SDL copy must not drive navigation.
    assert(observed_osk == 0);
    axis(0, 3, 24000, true);
    assert((observed_osk & OSK_LEFT) && !(observed_osk & OSK_RIGHT));
    axis(0, 3, 400, true); axis(0, 3, 0, true);
    assert(observed_osk == 0 && joydir[0] == 0); // No below-deadzone raw leakage.
    retro.mapping.button_unmasked[SDL_GAMEPAD_BUTTON_SOUTH] = 2;
    retro.mapping.button_unmasked[SDL_GAMEPAD_BUTTON_WEST] = 0;
    retro.mapping.button = retro.mapping.button_unmasked;
    button(0, SDL_GAMEPAD_BUTTON_WEST, true); // SDL copy of raw 2.
    button(0, 2, true, true);
    assert(observed_osk & OSK_BUTTON);
    button(0, 2, false, true);
    assert(!(observed_osk & OSK_BUTTON));

    reset_fixture(); di_joystick[0].mapping.is_retroarch = true;
    hat(0, SDL_HAT_UP); assert(joydir[0] & DIR_UP);
    active = true;
    hat(0, SDL_HAT_UP | SDL_HAT_RIGHT);
    assert((joydir[0] & DIR_UP) && !(joydir[0] & DIR_RIGHT));
    hat(0, SDL_HAT_RIGHT);
    assert(joydir[0] == 0 && (observed_osk & OSK_RIGHT));
    hat(0, SDL_HAT_CENTERED);
    assert(joydir[0] == 0 && observed_osk == 0);

    reset_fixture();
    auto& mapped = di_joystick[0];
    mapped.mapping.button_unmasked[SDL_GAMEPAD_BUTTON_START] = SDL_GAMEPAD_BUTTON_SOUTH;
    mapped.mapping.button_unmasked[SDL_GAMEPAD_BUTTON_LEFT_STICK] = SDL_GAMEPAD_BUTTON_EAST;
    sync_controller_shortcuts(&mapped);
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, true);
    button(0, SDL_GAMEPAD_BUTTON_EAST, true);
    assert(pending(AKS_ENTERGUI) == 0 && pending(AKS_OSK) == 0);
    mapped.is_controller = false;
    sync_controller_shortcuts(&mapped);
    vkbd_button = SDL_GAMEPAD_BUTTON_INVALID; currprefs.vkbd_toggle[0] = 0;
    for (auto& did : di_joystick) sync_controller_shortcuts(&did);
    button(0, SDL_GAMEPAD_BUTTON_EAST, true, true);
    assert(pending(AKS_OSK) == 0);

    reset_fixture(); di_joystick[0].mapping.is_retroarch = true;
    button(0, SDL_GAMEPAD_BUTTON_LEFT_STICK, true);
    button(0, SDL_GAMEPAD_BUTTON_LEFT_STICK, true, true);
    button(0, SDL_GAMEPAD_BUTTON_LEFT_STICK, false);
    button(0, SDL_GAMEPAD_BUTTON_LEFT_STICK, false, true);
    assert(pending(AKS_OSK) == 1);

    reset_fixture(); active = true;
    button(0, SDL_GAMEPAD_BUTTON_SOUTH, true);
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    imgui_osk_hide(); // Same session-ending operation used before GUI handoff.
    pads[0].axes[0] = 400; // GUI consumes neutral; centered hardware still reports drift.
    osk_clear_controller_holds(); // Reacquisition polls current physical state.
    active = true; animating = false;
    button(1, SDL_GAMEPAD_BUTTON_SOUTH, true);
    button(1, SDL_GAMEPAD_BUTTON_SOUTH, false);
    assert(!(observed_osk & OSK_BUTTON));
    axis(0, SDL_GAMEPAD_AXIS_LEFTX, 24000);
    assert(observed_osk & OSK_RIGHT);
    handle_joy_device_event(1, true);
    assert(!(observed_osk & OSK_RIGHT));
    puts("OSK native ownership regressions passed");
}
