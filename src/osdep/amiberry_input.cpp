#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "input_platform.h"
#include "amiberry_gfx.h"
#include "fsdb.h"
#include "uae.h"
#include "xwin.h"
#include "tabletlibrary.h"

enum
{
	MAX_MOUSE_BUTTONS = 3,
	MAX_MOUSE_AXES = 4,
	FIRST_MOUSE_AXIS = 0
};

#define FIRST_MOUSE_BUTTON	MAX_MOUSE_AXES
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define SET_BIT(var,pos) ((var) |= 1 << (pos))

enum
{
	MAX_JOY_BUTTONS = 16,
	MAX_JOY_AXES = 8,
	FIRST_JOY_AXIS = 0
};

#define FIRST_JOY_BUTTON	MAX_JOY_AXES

int tablet_log = 0;
int key_swap_hack = 0;
int key_swap_end_pgup = 0;

static struct didata di_mouse[MAX_INPUT_DEVICES];
static struct didata di_keyboard[MAX_INPUT_DEVICES];
struct didata di_joystick[MAX_INPUT_DEVICES];

static SDL_MouseID mouse_id_map[MAX_INPUT_DEVICES]{};
static std::string mouse_unique_names[MAX_INPUT_DEVICES];

static int num_mouse = 1, num_keyboard = 1, num_joystick = 0, num_retroarch_kbdjoy = 0;
static int joystick_inited, retroarch_inited;
// Index of the on-screen joystick inside di_joystick[]; -1 when not registered
static int osj_device_index = -1;
constexpr auto analog_upper_bound = 32767;
constexpr auto analog_lower_bound = -analog_upper_bound;

static int isrealbutton(const struct didata* did, const int num)
{
	if (num >= did->buttons)
		return 0;
	if (did->buttonaxisparent[num] >= 0)
		return 0;
	return 1;
}

static void fixbuttons(struct didata* did)
{
	if (did->buttons > 0)
		return;
	write_log(_T("'%s' has no buttons, adding single default button\n"), did->name.c_str());
	did->buttonmappings[0] = 0;
	did->buttonsort[0] = 0;
	did->buttonname[0] = "Button";
	did->buttons++;
}

static void addplusminus(struct didata* did, const int i)
{
	if (did->buttons + 1 >= ID_BUTTON_TOTAL)
		return;
	for (uae_s16 j = 0; j < 2; j++) {
		did->buttonname[did->buttons] = did->axisname[i];
		if (j)
			did->buttonname[did->buttons] += " [+]";
		else
			did->buttonname[did->buttons] += " [-]";
		did->buttonmappings[did->buttons] = did->axismappings[i];
		did->buttonsort[did->buttons] = 1000 + (did->axismappings[i] + did->axistype[i]) * 2 + j;
		did->buttonaxisparent[did->buttons] = i;
		did->buttonaxisparentdir[did->buttons] = j;
		did->buttonaxistype[did->buttons] = did->axistype[i];
		did->buttons++;
	}
}

static void fixthings(struct didata* did)
{
	did->buttons_real = did->buttons;
	for (int i = 0; i < did->axles; i++)
		addplusminus(did, i);
}

int find_in_array(const int arr[], const int n, const int key)
{
	int index = -1;

	for (int i = 0; i < n; i++)
	{
		if (arr[i] == key)
		{
			index = i;
			break;
		}
	}
	return index;
}

void fill_default_controller(controller_mapping& mapping)
{
	mapping.hotkey_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.quit_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.reset_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.menu_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.vkbd_button = SDL_GAMEPAD_BUTTON_INVALID;

	mapping.lstick_axis_y_invert = false;
	mapping.lstick_axis_x_invert = false;
	mapping.rstick_axis_y_invert = false;
	mapping.rstick_axis_x_invert = false;

	mapping.number_of_hats = 1;
	mapping.number_of_axis = -1;
	mapping.is_retroarch = false;

	for (auto b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++)
		mapping.button[b] = b;

	for (auto a = 0; a < SDL_GAMEPAD_AXIS_COUNT; a++)
		mapping.axis[a] = a;
}

void fill_blank_controller(controller_mapping& mapping)
{
	mapping.hotkey_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.quit_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.reset_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.menu_button = SDL_GAMEPAD_BUTTON_INVALID;
	mapping.vkbd_button = SDL_GAMEPAD_BUTTON_INVALID;

	mapping.lstick_axis_y_invert = false;
	mapping.lstick_axis_x_invert = false;
	mapping.rstick_axis_y_invert = false;
	mapping.rstick_axis_x_invert = false;

	mapping.number_of_hats = -1;
	mapping.number_of_axis = -1;
	mapping.is_retroarch = false;

	for (auto& b : mapping.button)
		b = SDL_GAMEPAD_BUTTON_INVALID;

	for (auto& axi : mapping.axis)
		axi = SDL_GAMEPAD_AXIS_INVALID;
}

constexpr int default_button_mapping[] = {
	INPUTEVENT_JOY2_CD32_RED,  INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	0, 0, INPUTEVENT_JOY2_CD32_PLAY, 0, 0, INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW,
	INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

constexpr int default_axis_mapping[] = {
	INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_VERT, INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_VERT,
	0, 0
};

constexpr int remap_event_list[] = {
	// joystick port 1
	INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	INPUTEVENT_JOY2_HORIZ, INPUTEVENT_JOY2_VERT, INPUTEVENT_JOY2_HORIZ_POT, INPUTEVENT_JOY2_VERT_POT,
	INPUTEVENT_JOY2_AUTOFIRE_BUTTON, INPUTEVENT_JOY2_FIRE_BUTTON, INPUTEVENT_JOY2_2ND_BUTTON, INPUTEVENT_JOY2_3RD_BUTTON,
	INPUTEVENT_JOY2_CD32_RED, INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW, INPUTEVENT_JOY2_CD32_PLAY,
	// mouse port 1
	INPUTEVENT_MOUSE2_UP, INPUTEVENT_MOUSE2_DOWN, INPUTEVENT_MOUSE2_LEFT, INPUTEVENT_MOUSE2_RIGHT,
	INPUTEVENT_MOUSE2_HORIZ, INPUTEVENT_MOUSE2_VERT,
	// joystick port 0            
	INPUTEVENT_JOY1_UP, INPUTEVENT_JOY1_DOWN, INPUTEVENT_JOY1_LEFT, INPUTEVENT_JOY1_RIGHT,
	INPUTEVENT_JOY1_HORIZ, INPUTEVENT_JOY1_VERT, INPUTEVENT_JOY1_HORIZ_POT, INPUTEVENT_JOY1_VERT_POT,
	INPUTEVENT_JOY1_AUTOFIRE_BUTTON, INPUTEVENT_JOY1_FIRE_BUTTON, INPUTEVENT_JOY1_2ND_BUTTON, INPUTEVENT_JOY1_3RD_BUTTON,
	INPUTEVENT_JOY1_CD32_RED, INPUTEVENT_JOY1_CD32_BLUE, INPUTEVENT_JOY1_CD32_GREEN, INPUTEVENT_JOY1_CD32_YELLOW,
	INPUTEVENT_JOY1_CD32_RWD, INPUTEVENT_JOY1_CD32_FFW, INPUTEVENT_JOY1_CD32_PLAY,
	// mouse port 0
	INPUTEVENT_MOUSE1_UP, INPUTEVENT_MOUSE1_DOWN, INPUTEVENT_MOUSE1_LEFT, INPUTEVENT_MOUSE1_RIGHT,
	INPUTEVENT_MOUSE1_HORIZ, INPUTEVENT_MOUSE1_VERT,
	// joystick port 2 (parallel 1)
	INPUTEVENT_PAR_JOY1_UP, INPUTEVENT_PAR_JOY1_DOWN, INPUTEVENT_PAR_JOY1_LEFT, INPUTEVENT_PAR_JOY1_RIGHT,
	INPUTEVENT_PAR_JOY1_HORIZ, INPUTEVENT_PAR_JOY1_VERT,
	INPUTEVENT_PAR_JOY1_AUTOFIRE_BUTTON, INPUTEVENT_PAR_JOY1_FIRE_BUTTON, INPUTEVENT_PAR_JOY1_2ND_BUTTON,
	// joystick port 3 (parallel 2)
	INPUTEVENT_PAR_JOY2_UP, INPUTEVENT_PAR_JOY2_DOWN, INPUTEVENT_PAR_JOY2_LEFT, INPUTEVENT_PAR_JOY2_RIGHT,
	INPUTEVENT_PAR_JOY2_HORIZ, INPUTEVENT_PAR_JOY2_VERT,
	INPUTEVENT_PAR_JOY2_AUTOFIRE_BUTTON, INPUTEVENT_PAR_JOY2_FIRE_BUTTON, INPUTEVENT_PAR_JOY2_2ND_BUTTON,
	// keyboard
	INPUTEVENT_SPC_QUALIFIER_SPECIAL,
	INPUTEVENT_KEY_F1, INPUTEVENT_KEY_F2, INPUTEVENT_KEY_F3, INPUTEVENT_KEY_F4, INPUTEVENT_KEY_F5,
	INPUTEVENT_KEY_F6, INPUTEVENT_KEY_F7, INPUTEVENT_KEY_F8, INPUTEVENT_KEY_F9, INPUTEVENT_KEY_F10,
	// special keys
	INPUTEVENT_KEY_ESC, INPUTEVENT_KEY_TAB, INPUTEVENT_KEY_CTRL, INPUTEVENT_KEY_CAPS_LOCK,
	INPUTEVENT_KEY_SHIFT_LEFT, INPUTEVENT_KEY_ALT_LEFT, INPUTEVENT_KEY_AMIGA_LEFT,
	INPUTEVENT_KEY_AMIGA_RIGHT, INPUTEVENT_KEY_ALT_RIGHT, INPUTEVENT_KEY_SHIFT_RIGHT,
	INPUTEVENT_KEY_CURSOR_UP, INPUTEVENT_KEY_CURSOR_DOWN, INPUTEVENT_KEY_CURSOR_LEFT, INPUTEVENT_KEY_CURSOR_RIGHT,
	INPUTEVENT_KEY_SPACE, INPUTEVENT_KEY_HELP, INPUTEVENT_KEY_DEL, INPUTEVENT_KEY_BACKSPACE, INPUTEVENT_KEY_RETURN,
	
	INPUTEVENT_KEY_A, INPUTEVENT_KEY_B, INPUTEVENT_KEY_C, INPUTEVENT_KEY_D,
	INPUTEVENT_KEY_E, INPUTEVENT_KEY_F, INPUTEVENT_KEY_G, INPUTEVENT_KEY_H,
	INPUTEVENT_KEY_I, INPUTEVENT_KEY_J, INPUTEVENT_KEY_K, INPUTEVENT_KEY_L,
	INPUTEVENT_KEY_M, INPUTEVENT_KEY_N, INPUTEVENT_KEY_O, INPUTEVENT_KEY_P,
	INPUTEVENT_KEY_Q, INPUTEVENT_KEY_R, INPUTEVENT_KEY_S, INPUTEVENT_KEY_T,
	INPUTEVENT_KEY_U, INPUTEVENT_KEY_V, INPUTEVENT_KEY_W, INPUTEVENT_KEY_X,
	INPUTEVENT_KEY_Y, INPUTEVENT_KEY_Z,
	// numpad
	INPUTEVENT_KEY_ENTER,
	INPUTEVENT_KEY_NP_0, INPUTEVENT_KEY_NP_1, INPUTEVENT_KEY_NP_2,
	INPUTEVENT_KEY_NP_3, INPUTEVENT_KEY_NP_4, INPUTEVENT_KEY_NP_5,
	INPUTEVENT_KEY_NP_6, INPUTEVENT_KEY_NP_7, INPUTEVENT_KEY_NP_8,
	INPUTEVENT_KEY_NP_9, INPUTEVENT_KEY_NP_PERIOD, INPUTEVENT_KEY_NP_ADD,
	INPUTEVENT_KEY_NP_SUB, INPUTEVENT_KEY_NP_MUL, INPUTEVENT_KEY_NP_DIV,
	INPUTEVENT_KEY_NP_LPAREN, INPUTEVENT_KEY_NP_RPAREN,

	INPUTEVENT_KEY_BACKQUOTE,
	INPUTEVENT_KEY_1, INPUTEVENT_KEY_2, INPUTEVENT_KEY_3, INPUTEVENT_KEY_4,
	INPUTEVENT_KEY_5, INPUTEVENT_KEY_6, INPUTEVENT_KEY_7, INPUTEVENT_KEY_8,
	INPUTEVENT_KEY_9, INPUTEVENT_KEY_0,
	INPUTEVENT_KEY_SUB, INPUTEVENT_KEY_EQUALS, INPUTEVENT_KEY_BACKSLASH,

	INPUTEVENT_KEY_LEFTBRACKET, INPUTEVENT_KEY_RIGHTBRACKET,
	INPUTEVENT_KEY_SEMICOLON, INPUTEVENT_KEY_SINGLEQUOTE,
	INPUTEVENT_KEY_COMMA, INPUTEVENT_KEY_PERIOD, INPUTEVENT_KEY_DIV,
	// mouse wheel "keys"
	INPUTEVENT_MOUSEWHEEL_DOWN, INPUTEVENT_MOUSEWHEEL_UP,
	// Emulator and machine events
	INPUTEVENT_SPC_ENTERGUI, INPUTEVENT_SPC_QUIT, INPUTEVENT_SPC_PAUSE,
	INPUTEVENT_SPC_SOFTRESET, INPUTEVENT_SPC_HARDRESET, INPUTEVENT_SPC_FREEZEBUTTON,
	INPUTEVENT_SPC_STATESAVE, INPUTEVENT_SPC_STATERESTORE,
	INPUTEVENT_SPC_STATESAVEDIALOG, INPUTEVENT_SPC_STATERESTOREDIALOG,
	INPUTEVENT_SPC_MASTER_VOLUME_DOWN, INPUTEVENT_SPC_MASTER_VOLUME_UP, INPUTEVENT_SPC_MASTER_VOLUME_MUTE,
	INPUTEVENT_SPC_TOGGLEWINDOWFULLWINDOW, INPUTEVENT_SPC_TOGGLEMOUSEGRAB,
	INPUTEVENT_SPC_DECREASE_REFRESHRATE, INPUTEVENT_SPC_INCREASE_REFRESHRATE,
	INPUTEVENT_SPC_SWAPJOYPORTS, INPUTEVENT_SPC_PASTE, INPUTEVENT_SPC_SCREENSHOT,

	INPUTEVENT_SPC_DISKSWAPPER_NEXT_INSERT0, INPUTEVENT_SPC_DISKSWAPPER_PREVIOUS_INSERT0,

	INPUTEVENT_SPC_DISKSWAPPER_NEXT, INPUTEVENT_SPC_DISKSWAPPER_PREV,
	INPUTEVENT_SPC_DISKSWAPPER_INSERT0, INPUTEVENT_SPC_DISKSWAPPER_INSERT1,
	INPUTEVENT_SPC_DISKSWAPPER_INSERT2, INPUTEVENT_SPC_DISKSWAPPER_INSERT3,

	INPUTEVENT_SPC_MOUSEMAP_PORT0_LEFT, INPUTEVENT_SPC_MOUSEMAP_PORT0_RIGHT,
	INPUTEVENT_SPC_MOUSEMAP_PORT1_LEFT, INPUTEVENT_SPC_MOUSEMAP_PORT1_RIGHT,
	INPUTEVENT_SPC_MOUSE_SPEED_DOWN, INPUTEVENT_SPC_MOUSE_SPEED_UP, INPUTEVENT_SPC_SHUTDOWN,
	INPUTEVENT_SPC_WARP, INPUTEVENT_SPC_TOGGLE_JIT, INPUTEVENT_SPC_TOGGLE_JIT_FPU,
	INPUTEVENT_SPC_AUTO_CROP_IMAGE, INPUTEVENT_SPC_OSK
};

constexpr int remap_event_list_size = std::size(remap_event_list);

const TCHAR* find_inputevent_name(const int key)
{
	const auto* output = "None";

	for (const int i : remap_event_list)
	{
		if (i == key)
		{
			const auto* input_event = inputdevice_get_eventinfo(i);
			output = _T(input_event->name);
			break;
		}
		output = "None";
	}
	return output;
}

int find_inputevent(const TCHAR* key)
{
	for (auto i = 0; i < remap_event_list_size; ++i)
	{
		const auto* input_event = inputdevice_get_eventinfo(remap_event_list[i]);

		if (!_tcscmp(input_event->name, key))
		{
			return i;
		}
	}
	return -1;
}

//# Keyboard input. Will recognize letters (a to z) and the following special keys (where kp_
//# is for keypad keys):
//#
//#   "left", right, up, down, enter, kp_enter, tab, insert, del, end, home,
//#   rshift, shift, ctrl, alt, space, escape, add, subtract, kp_plus, kp_minus,
//#   f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
//#   num0, num1, num2, num3, num4, num5, num6, num7, num8, num9, pageup, pagedown,
//#   keypad0, keypad1, keypad2, keypad3, keypad4, keypad5, keypad6, keypad7, keypad8, keypad9,
//#   period, capslock, numlock, backspace, multiply, divide, print_screen, scroll_lock,
//#   tilde, backquote, pause, quote, comma, minus, slash, semicolon, equals, leftbracket,
//#   backslash, rightbracket, kp_period, kp_equals, rctrl, ralt

constexpr int remap_key_map_list[] = {
	-1,
	SDL_SCANCODE_A, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E,
	SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J,
	SDL_SCANCODE_K, SDL_SCANCODE_L, SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O,
	SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R, SDL_SCANCODE_S, SDL_SCANCODE_T,
	SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X, SDL_SCANCODE_Y, SDL_SCANCODE_Z,
	SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_RETURN,
	SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_TAB, SDL_SCANCODE_INSERT, SDL_SCANCODE_DELETE, SDL_SCANCODE_END,
	SDL_SCANCODE_HOME,
	SDL_SCANCODE_RSHIFT, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LALT, SDL_SCANCODE_SPACE,
	SDL_SCANCODE_ESCAPE, SDL_SCANCODE_KP_PLUSMINUS, SDL_SCANCODE_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_MINUS,
	SDL_SCANCODE_F1, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5, SDL_SCANCODE_F6,
	SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10, SDL_SCANCODE_F11, SDL_SCANCODE_F12,
	SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6,
	SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_PAGEUP, SDL_SCANCODE_PAGEDOWN,
	SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_4,
	SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_9,
	SDL_SCANCODE_PERIOD, SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_BACKSPACE,
	SDL_SCANCODE_KP_MULTIPLY, SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_PRINTSCREEN, SDL_SCANCODE_SCROLLLOCK,
	SDL_SCANCODE_GRAVE, SDL_SCANCODE_GRAVE, SDL_SCANCODE_PAUSE, SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_COMMA,
	SDL_SCANCODE_MINUS, SDL_SCANCODE_SLASH, SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_EQUALS, SDL_SCANCODE_LEFTBRACKET,
	SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_RIGHTBRACKET,
	SDL_SCANCODE_KP_PERIOD, SDL_SCANCODE_KP_EQUALS, SDL_SCANCODE_RCTRL, SDL_SCANCODE_RALT
};

std::vector<std::string> remap_key_map_list_strings = {
	"nul",
	"a", "b", "c", "d", "e",
	"f", "g", "h", "i", "j",
	"k", "l", "m", "n", "o",
	"p", "q", "r", "s", "t",
	"u", "v", "w", "x", "y", "z",
	"left", "right", "up", "down", "enter",
	"kp_enter", "tab", "insert", "del", "end", "home",
	"rshift", "shift", "ctrl", "alt", "space",
	"escape", "add", "subtract", "kp_plus", "kp_minus",
	"f1", "f2", "f3", "f4", "f5", "f6",
	"f7", "f8", "f9", "f10", "f11", "f12",
	"num0", "num1", "num2", "num3", "num4", "num5",
	"num6", "num7", "num8", "num9", "pageup", "pagedown",
	"keypad0", "keypad1", "keypad2", "keypad3", "keypad4",
	"keypad5", "keypad6", "keypad7", "keypad8", "keypad9",
	"period", "capslock", "numlock", "backspace",
	"multiply", "divide", "print_screen", "scroll_lock",
	"tilde", "backquote", "pause", "quote", "comma",
	"minus", "slash", "semicolon", "equals", "leftbracket",
	"backslash", "rightbracket",
	"kp_period", "kp_equals", "rctrl", "ralt"
};

//#define	MAX_KEYCODES 256
//static uae_u8 di_keycodes[MAX_INPUT_DEVICES][MAX_KEYCODES];
static int keyboard_german;

int keyhack (const int scancode, const int pressed, const int num)
{
	static unsigned char backslashstate, apostrophstate;
	const bool* state = SDL_GetKeyboardState(nullptr);

	// Reserve Alt-Tab for the host when the option is enabled. Focus loss will
	// perform the actual capture teardown and pressed-key cleanup.
	if (currprefs.alt_tab_release)
	{
		if (pressed && scancode == SDL_SCANCODE_TAB && (key_altpressed() || state[SDL_SCANCODE_LALT] || state[SDL_SCANCODE_RALT])) {
			// Workaround: SDL3 < 3.4.2 crashes in Wayland pointer_dispatch_relative_motion
			// with NULL window during focus transition. See SDL fab42a14, #1829.
			const AmigaMonitor* mon = &AMonitors[0];
			if (mon->amiga_window)
				SDL_SetWindowRelativeMouseMode(mon->amiga_window, false);
			return -1;
		}
	}
#if 0
	if (!keyboard_german)
		return scancode;

	if (scancode == SDL_SCANCODE_BACKSLASH)
	{
		if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT] || apostrophstate)
		{
			if (pressed)
			{
				apostrophstate = 1;
				inputdevice_translatekeycode(num, SDL_SCANCODE_RSHIFT, 0, false);
				inputdevice_translatekeycode(num, SDL_SCANCODE_LSHIFT, 0, false);
				return SDL_SCANCODE_APOSTROPHE;           // the german ' key
			}
			else
			{
				apostrophstate = 0;
				inputdevice_translatekeycode(num, SDL_SCANCODE_LALT, 0, true);
				inputdevice_translatekeycode(num, SDL_SCANCODE_LSHIFT, 0, true);
				inputdevice_translatekeycode(num, SDL_SCANCODE_3, 0, true);  // release also the # key
				return SDL_SCANCODE_APOSTROPHE;
			}

		}
		if (pressed)
		{
			inputdevice_translatekeycode(num, SDL_SCANCODE_LALT, 1, false);
			inputdevice_translatekeycode(num, SDL_SCANCODE_LSHIFT, 1, false);
			return SDL_SCANCODE_3;           // the german # key
		}
		else
		{
			inputdevice_translatekeycode(num, SDL_SCANCODE_LALT, 0, true);
			inputdevice_translatekeycode(num, SDL_SCANCODE_LSHIFT, 0, true);
			return SDL_SCANCODE_3;           // the german # key

		}
	}
	if (state[SDL_SCANCODE_RALT] || backslashstate) {
		switch (scancode)
		{
		case SDL_SCANCODE_BACKSLASH: // WinUAE had 12 here -> is this the correct scancode?
			if (pressed)
			{
				backslashstate = 1;
				inputdevice_translatekeycode(num, SDL_SCANCODE_LALT, 0, true);
				return SDL_SCANCODE_BACKSLASH;
			}
			else
			{
				backslashstate = 0;
				return SDL_SCANCODE_BACKSLASH;
			}
		}
	}
#endif
	return scancode;
}

static void cleardid(struct didata* did)
{
	*did = didata{};
	for (int i = 0; i < MAX_MAPPINGS; i++) {
		did->axismappings[i] = -1;
		did->buttonmappings[i] = -1;
		did->buttonaxisparent[i] = -1;
	}
}

static void di_dev_free(struct didata* did)
{
	if (did->controller != nullptr)
	{
		SDL_CloseGamepad(did->controller);
		did->controller = nullptr;
	}
	if (did->joystick != nullptr && !did->is_controller)
	{
		SDL_CloseJoystick(did->joystick);
		did->joystick = nullptr;
	}
	cleardid(did);
}

static void di_free()
{
	for (auto i = 0; i < MAX_INPUT_DEVICES; i++) {
		di_dev_free(&di_joystick[i]);
		di_dev_free(&di_mouse[i]);
		di_dev_free(&di_keyboard[i]);
	}
}

static int tablet_detected;
static int tablet_initialized;
static int axmax, aymax, azmax;
static int xmax, ymax, zmax;
static int xres, yres;
static int maxpres;
static int tablet_x, tablet_y, tablet_z, tablet_pressure, tablet_buttons, tablet_proximity;
static int tablet_ax, tablet_ay, tablet_az, tablet_flags;

#define TABLET_FLAG_ERASER (1 << 0)

static void tablet_send(void)
{
	static int eraser;

	// Button bit 2 = eraser (WinUAE TPS_INVERT compatibility)
	if ((tablet_flags & TABLET_FLAG_ERASER) && tablet_pressure > 0) {
		tablet_buttons |= 2;
		eraser = 1;
	} else if (eraser) {
		tablet_buttons &= ~2;
		eraser = 0;
	}
	if (tablet_x < 0)
		return;
	inputdevice_tablet(tablet_x, tablet_y, tablet_z, tablet_pressure, tablet_buttons, tablet_proximity,
		tablet_ax, tablet_ay, tablet_az, 0);
	tabletlib_tablet(tablet_x, tablet_y, tablet_z, tablet_pressure, maxpres, tablet_buttons, tablet_proximity,
		tablet_ax, tablet_ay, tablet_az);
}

void send_tablet_proximity(int inproxi)
{
	if (tablet_proximity == inproxi)
		return;
	tablet_proximity = inproxi;
	if (!tablet_proximity) {
		tablet_flags &= ~TABLET_FLAG_ERASER;
	}
	if (tablet_log & 4)
		write_log(_T("TABLET: Proximity=%d\n"), inproxi);
	tablet_send();
}

void send_tablet(int x, int y, int z, int pres, uae_u32 buttons, int flags, int ax, int ay, int az, int rx, int ry, int rz, SDL_Rect* r)
{
	if (tablet_log & 4)
		write_log(_T("TABLET: B=%08X F=%08X X=%d Y=%d P=%d (%d,%d,%d)\n"), buttons, flags, x, y, pres, ax, ay, az);

	tablet_x = x;
	tablet_y = y;
	tablet_z = z;
	tablet_pressure = pres;
	tablet_buttons = buttons;
	tablet_ax = abs(ax);
	tablet_ay = abs(ay);
	tablet_az = abs(az);
	tablet_flags = flags;

	tablet_send();
}

static int initialize_tablet(void);

void* open_tablet(SDL_Window* window)
{
	initialize_tablet();

	if (inputdevice_is_tablet() <= 0 && !is_touch_lightpen())
		return nullptr;

#ifndef LIBRETRO
	SDL_SetHint(SDL_HINT_PEN_MOUSE_EVENTS, "0");
	SDL_SetHint(SDL_HINT_PEN_TOUCH_EVENTS, "0");
#endif

	xmax = 4095; // Amiga tablet.library max coordinate range
	ymax = 4095;
	zmax = 0;
	maxpres = 255;
	xres = 100;
	yres = 100;

	tablet_proximity = -1;
	tablet_x = -1;
	tablet_pressure = 0;
	tablet_buttons = 0;
	tablet_flags = 0;

	inputdevice_tablet_info(xmax, ymax, zmax, axmax, aymax, azmax, xres, yres);
	tabletlib_tablet_info(xmax, ymax, zmax, axmax, aymax, azmax, xres, yres);

	write_log(_T("Tablet: SDL3 pen support initialized (Xmax=%d,Ymax=%d,Pmax=%d)\n"), xmax, ymax, maxpres);

	return reinterpret_cast<void*>(1);
}

int close_tablet(void* ctx)
{
	if (ctx == nullptr)
		return 0;
	// Don't reset tablet_detected — it represents hardware availability,
	// not session state. Matches WinUAE behavior where close_tablet()
	// only closes the Wintab context without clearing the detection flag.
#ifndef LIBRETRO
	SDL_SetHint(SDL_HINT_PEN_MOUSE_EVENTS, "1");
	SDL_SetHint(SDL_HINT_PEN_TOUCH_EVENTS, "1");
#endif
	tablet_proximity = -1;
	tablet_x = -1;
	write_log(_T("Tablet: SDL3 pen session closed\n"));
	return 1;
}

int is_touch_lightpen()
{
	return 0;
}

int is_tablet()
{
	return tablet_detected ? 1 : 0;
}

static int initialize_tablet(void)
{
	if (tablet_initialized)
		return tablet_detected;

	tablet_initialized = 1;
	tablet_detected = 1;
	axmax = aymax = azmax = 255;
	write_log(_T("Tablet: SDL3 pen support available\n"));
	return 1;
}

static void setup_mouse_device(struct didata* did, const char* name)
{
	cleardid(did);
	did->name = name ? name : "Mouse";
	did->buttons = 3;
	did->buttons_real = 3;
	did->axles = 4;
	did->axissort[0] = 0;
	did->axisname[0] = "X Axis";
	did->axissort[1] = 1;
	did->axisname[1] = "Y Axis";
	if (did->axles > 2) {
		did->axissort[2] = 2;
		did->axisname[2] = "Wheel";
		addplusminus(did, 2);
	}
	if (did->axles > 3) {
		did->axissort[3] = 3;
		did->axisname[3] = "HWheel";
		addplusminus(did, 3);
	}
}

static int init_mouse()
{
	memset(mouse_id_map, 0, sizeof mouse_id_map);
	for (auto& s : mouse_unique_names)
		s.clear();

#ifndef LIBRETRO
	int count = 0;
	SDL_MouseID* sdl_mice = SDL_GetMice(&count);
	if (sdl_mice && count >= 1) {
		num_mouse = std::min(count, static_cast<int>(MAX_INPUT_DEVICES));
		for (int i = 0; i < num_mouse; i++) {
			mouse_id_map[i] = sdl_mice[i];
			const char* sdl_name = SDL_GetMouseNameForID(sdl_mice[i]);
			std::string device_name;
			if (sdl_name && sdl_name[0]) {
				device_name = sdl_name;
			} else {
				device_name = (num_mouse > 1)
					? "Mouse " + std::to_string(i + 1)
					: "System mouse";
			}
			setup_mouse_device(&di_mouse[i], device_name.c_str());
		}
		SDL_free(sdl_mice);
		write_log(_T("Multi-mouse: %d mice detected\n"), num_mouse);
		for (int i = 0; i < num_mouse; i++) {
			write_log(_T("  Mouse %d: \"%s\" (SDL ID %u)\n"), i, di_mouse[i].name.c_str(),
				static_cast<unsigned>(mouse_id_map[i]));
		}
	} else {
		if (sdl_mice)
			SDL_free(sdl_mice);
#endif
		num_mouse = 1;
		mouse_id_map[0] = 0;
		setup_mouse_device(&di_mouse[0], "System mouse");
#ifndef LIBRETRO
	}
#endif
	return 1;
}

static void close_mouse()
{
	for (auto i = 0; i < num_mouse; i++)
		di_dev_free(&di_mouse[i]);
	memset(mouse_id_map, 0, sizeof mouse_id_map);
	for (auto& s : mouse_unique_names)
		s.clear();
	di_free();
}

static int acquire_mouse(const int num, int flags)
{
	if (num < 0) {
		return 1;
	}

	struct didata* did = &di_mouse[num];
	did->acquired = 1;
	return did->acquired > 0 ? 1 : 0;
}

static void unacquire_mouse(int num)
{
	if (num < 0) {
		return;
	}

	struct didata* did = &di_mouse[num];
	did->acquired = 0;
}

static int get_mouse_num()
{
	return num_mouse;
}

static const TCHAR* get_mouse_friendlyname(const int mouse)
{
	return di_mouse[mouse].name.c_str();
}

static const TCHAR* get_mouse_uniquename(const int mouse)
{
	if (mouse >= 0 && mouse < num_mouse) {
		if (mouse_unique_names[mouse].empty())
			mouse_unique_names[mouse] = "MOUSE" + std::to_string(mouse);
		return mouse_unique_names[mouse].c_str();
	}
	return "";
}

static int get_mouse_widget_num(const int mouse)
{
	return di_mouse[mouse].axles + di_mouse[mouse].buttons;
}

static int get_mouse_widget_first(const int mouse, const int type)
{
	switch (type)
	{
	case IDEV_WIDGET_BUTTON:
		return di_mouse[mouse].axles;
	case IDEV_WIDGET_AXIS:
		return 0;
	case IDEV_WIDGET_BUTTONAXIS:
		return di_mouse[mouse].axles + di_mouse[mouse].buttons_real;
	default:
		return -1;
	}
}

static int get_mouse_widget_type(const int mouse, const int num, TCHAR* name, uae_u32* code)
{
	struct didata* did = &di_mouse[mouse];

	int axles = did->axles;
	int buttons = did->buttons;
	int realbuttons = did->buttons_real;
	if (num >= axles + realbuttons && num < axles + buttons) {
		if (name)
			_tcscpy(name, did->buttonname[num - axles].c_str());
		return IDEV_WIDGET_BUTTONAXIS;
	}
	if (num >= axles && num < axles + realbuttons) {
		if (name)
			_tcscpy(name, did->buttonname[num - axles].c_str());
		return IDEV_WIDGET_BUTTON;
	}
	if (num < axles) {
		if (name)
			_tcscpy(name, did->axisname[num].c_str());
		return IDEV_WIDGET_AXIS;
	}
	return IDEV_WIDGET_NONE;
}

static void read_mouse()
{
}

static int get_mouse_flags(int num)
{
	return 0;
}

struct inputdevice_functions inputdevicefunc_mouse = {
	init_mouse, close_mouse, acquire_mouse, unacquire_mouse, read_mouse,
	get_mouse_num, get_mouse_friendlyname, get_mouse_uniquename,
	get_mouse_widget_num, get_mouse_widget_type,
	get_mouse_widget_first,
	get_mouse_flags
};

int get_mouse_index_from_sdl_id(SDL_MouseID which)
{
#ifndef LIBRETRO
	if (which == 0 || which == SDL_TOUCH_MOUSEID || which == SDL_PEN_MOUSEID)
		return 0;
	for (int i = 0; i < num_mouse; i++) {
		if (mouse_id_map[i] == which)
			return i;
	}
#endif
	return 0;
}

#ifndef LIBRETRO
void handle_sdl_mouse_added(SDL_MouseID which)
{
	if (which == 0 || which == SDL_TOUCH_MOUSEID || which == SDL_PEN_MOUSEID)
		return;

	for (int i = 0; i < num_mouse; i++) {
		if (mouse_id_map[i] == which)
			return;
	}

	int idx = -1;
	for (int i = 0; i < num_mouse; i++) {
		if (mouse_id_map[i] == 0) {
			idx = i;
			break;
		}
	}
	if (idx < 0) {
		if (num_mouse >= MAX_INPUT_DEVICES)
			return;
		idx = num_mouse;
		num_mouse++;
	}

	mouse_id_map[idx] = which;
	const char* sdl_name = SDL_GetMouseNameForID(which);
	std::string device_name;
	if (sdl_name && sdl_name[0]) {
		device_name = sdl_name;
	} else {
		device_name = "Mouse " + std::to_string(idx + 1);
	}
	setup_mouse_device(&di_mouse[idx], device_name.c_str());
	mouse_unique_names[idx] = "MOUSE" + std::to_string(idx);

	write_log(_T("Multi-mouse: added \"%s\" (SDL ID %u) as mouse %d\n"),
		di_mouse[idx].name.c_str(), static_cast<unsigned>(which), idx);
}

void handle_sdl_mouse_removed(SDL_MouseID which)
{
	if (which == 0 || which == SDL_TOUCH_MOUSEID || which == SDL_PEN_MOUSEID)
		return;

	for (int i = 0; i < num_mouse; i++) {
		if (mouse_id_map[i] == which) {
			write_log(_T("Multi-mouse: removed \"%s\" (SDL ID %u, mouse %d)\n"),
				di_mouse[i].name.c_str(), static_cast<unsigned>(which), i);
			for (int b = 0; b < di_mouse[i].buttons; b++)
				setmousebuttonstate(i, b, 0);
			mouse_id_map[i] = 0;
			cleardid(&di_mouse[i]);
			mouse_unique_names[i].clear();
			break;
		}
	}
	while (num_mouse > 1 && mouse_id_map[num_mouse - 1] == 0)
		num_mouse--;
}
#endif

static void setid(struct uae_input_device* uid, const int i, const int slot, const int sub, const int port, int evt, const bool gp)
{
	if (gp)
		inputdevice_sparecopy(&uid[i], slot, 0);
	uid[i].eventid[slot][sub] = evt;
	uid[i].port[slot][sub] = port + 1;
}

static void setid(struct uae_input_device* uid, const int i, const int slot, const int sub, const int port, const int evt, const int af, const bool gp)
{
	setid(uid, i, slot, sub, port, evt, gp);
	uid[i].flags[slot][sub] &= ~ID_FLAG_AUTOFIRE_MASK;
	if (af >= JPORT_AF_NORMAL)
		uid[i].flags[slot][sub] |= ID_FLAG_AUTOFIRE;
	if (af == JPORT_AF_TOGGLE)
		uid[i].flags[slot][sub] |= ID_FLAG_TOGGLE;
	if (af == JPORT_AF_ALWAYS)
		uid[i].flags[slot][sub] |= ID_FLAG_INVERTTOGGLE;
	if (af == JPORT_AF_TOGGLENOAF)
		uid[i].flags[slot][sub] |= ID_FLAG_INVERT;
}

int input_get_default_mouse(struct uae_input_device* uid, const int i, const int port, const int af, const bool gp, bool wheel, bool joymouseswap)
{
	struct didata* did;
	
	if (!joymouseswap) {
		if (i >= num_mouse)
			return 0;
		did = &di_mouse[i];
	}
	else {
		if (i >= num_joystick)
			return 0;
		did = &di_joystick[i];
	}
	setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ, gp);
	setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT, gp);
	if (wheel)
		setid(uid, i, ID_AXIS_OFFSET + 2, 0, port, port ? 0 : INPUTEVENT_MOUSE1_WHEEL, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);
	if (wheel && port == 0) { /* map back and forward to ALT+LCUR and ALT+RCUR */
		if (isrealbutton(did, 3)) {
			setid(uid, i, ID_BUTTON_OFFSET + 3, 0, port, INPUTEVENT_KEY_ALT_LEFT, gp);
			setid(uid, i, ID_BUTTON_OFFSET + 3, 1, port, INPUTEVENT_KEY_CURSOR_LEFT, gp);
			if (isrealbutton(did, 4)) {
				setid(uid, i, ID_BUTTON_OFFSET + 4, 0, port, INPUTEVENT_KEY_ALT_LEFT, gp);
				setid(uid, i, ID_BUTTON_OFFSET + 4, 1, port, INPUTEVENT_KEY_CURSOR_RIGHT, gp);
			}
		}
	}
	if (i == 0)
		return 1;
	return 0;
}

int input_get_default_lightpen(struct uae_input_device* uid, int i, int port, int af, bool gp, bool joymouseswap, int submode)
{
	struct didata* did;

	if (!joymouseswap) {
		if (i >= num_mouse)
			return 0;
		did = &di_mouse[i];
	}
	else {
		if (i >= num_joystick)
			return 0;
		did = &di_joystick[i];
	}
	setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, INPUTEVENT_LIGHTPEN_HORIZ, gp);
	setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, INPUTEVENT_LIGHTPEN_VERT, gp);
	int button = port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON;
	switch (submode)
	{
	case 1:
		button = port ? INPUTEVENT_JOY2_LEFT : INPUTEVENT_JOY1_LEFT;
		break;
	}
	setid(uid, i, ID_BUTTON_OFFSET + 0, 0, port, button, gp);
	if (i == 0)
		return 1;
	return 0;
}

static int get_kb_num()
{
	return num_keyboard;
}

int get_retroarch_kb_num()
{
	return num_retroarch_kbdjoy;
}

static const TCHAR* get_kb_friendlyname(int kb)
{
	return "Default Keyboard";
}

static const TCHAR* get_kb_uniquename(int kb)
{
	return "KEYBOARD0";
}

static int get_kb_widget_num(int kb)
{
	return 255;
}

static int get_kb_widget_first(int kb, int type)
{
	return 0;
}

static int get_kb_widget_type(int kb, int num, TCHAR* name, uae_u32* code)
{
	if (code)
		*code = num;
	return IDEV_WIDGET_KEY;
}

static int init_kb()
{
	osdep_init_keyboard(&keyboard_german, &retroarch_inited, &num_retroarch_kbdjoy);
	return 1;
}

static void close_kb()
{
}

void release_keys()
{
	// Special handling in case Alt-Tab was still stuck in pressed state
	if (currprefs.alt_tab_release && key_altpressed())
	{
		my_kbd_handler(0, SDL_SCANCODE_LALT, 0, true);
		my_kbd_handler(0, SDL_SCANCODE_TAB, 0, true);
	}

	// Always force-release modifier keys, even if SDL doesn't report them as pressed.
	// When the window loses focus (e.g. Windows/GUI key opens the desktop menu),
	// SDL internally clears its keyboard state, so SDL_GetKeyboardState() won't
	// show these keys as pressed. But the emulator's internal qualifier state
	// still has them set from the original KEYDOWN, causing stuck modifiers.
	static const SDL_Scancode modifier_keys[] = {
		SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL,
		SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT,
		SDL_SCANCODE_LALT, SDL_SCANCODE_RALT,
		SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI
	};
	for (const auto mod_scancode : modifier_keys) {
		my_kbd_handler(0, mod_scancode, 0, true);
	}

	const bool* state = SDL_GetKeyboardState(nullptr);
	SDL_Event event;

	for (int i = 0; i < SDL_SCANCODE_COUNT; ++i) {
		if (state[i]) {
			event.type = SDL_EVENT_KEY_UP;
			event.key.scancode = static_cast<SDL_Scancode>(i);
			event.key.key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(i), SDL_KMOD_NONE, false);
			event.key.mod = SDL_KMOD_NONE;
			event.key.down = false;
			SDL_PushEvent(&event);

			my_kbd_handler(0, i, 0, true);
		}
	}
}

static int acquire_kb(const int num, int flags)
{
	struct didata* did = &di_keyboard[num];
	did->acquired = 1;
	return did->acquired > 0 ? 1 : 0;
}

static void unacquire_kb(const int num)
{
	struct didata* did = &di_keyboard[num];
	did->acquired = 0;
}

static void read_kb()
{
}

void wait_keyrelease()
{
	release_keys();
}

static int get_kb_flags(int num)
{
	return 0;
}

struct inputdevice_functions inputdevicefunc_keyboard = {
	init_kb, close_kb, acquire_kb, unacquire_kb, read_kb,
	get_kb_num, get_kb_friendlyname, get_kb_uniquename,
	get_kb_widget_num, get_kb_widget_type,
	get_kb_widget_first,
	get_kb_flags
};

int input_get_default_keyboard(int num)
{
	if (num == 0)
		return 1;

	return 0;
}

static int get_joystick_num()
{
	return num_joystick;
}

// Helper functions
void open_as_game_controller(struct didata* did, const int i)
{
	char guid_str[33];
	did->controller = SDL_OpenGamepad(i);
	if (did->controller == nullptr)
	{
		write_log("Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
		return;
	}
	did->is_controller = true;
	did->joystick = SDL_GetGamepadJoystick(did->controller);
	did->joystick_id = SDL_GetJoystickID(did->joystick);
	SDL_GUIDToString(SDL_GetJoystickGUID(did->joystick), guid_str, 33);
	did->guid = std::string(guid_str);

	if (SDL_GetGamepadNameForID(i) != nullptr)
		did->controller_name.assign(SDL_GetGamepadNameForID(i));
	write_log("Controller #%i: %s\n      GUID: %s\n", did->joystick_id, SDL_GetGamepadName(did->controller), guid_str);

	// Try to get the Joystick Name as well, we will need it in case of RetroArch mapping files
	if (SDL_GetJoystickNameForID(i) != nullptr)
		did->joystick_name.assign(SDL_GetJoystickNameForID(i));
	else
		did->joystick_name = "";

	if (!did->controller_name.empty())
		did->name = did->controller_name;
	else
		did->name = did->joystick_name;
}

void open_as_joystick(struct didata* did, const int i)
{
	char guid_str[33];
	did->joystick = SDL_OpenJoystick(i);
	if (did->joystick == nullptr)
	{
		write_log("Warning: Unable to open Joystick! SDL Error: %s\n", SDL_GetError());
		return;
	}
	did->is_controller = false;
	did->joystick_id = SDL_GetJoystickID(did->joystick);
	SDL_GUIDToString(SDL_GetJoystickGUID(did->joystick), guid_str, 33);
	did->guid = std::string(guid_str);

	if (SDL_GetJoystickNameForID(i) != nullptr)
		did->joystick_name.assign(SDL_GetJoystickNameForID(i));
	else
		did->joystick_name = "";

	did->name = did->joystick_name;
	write_log("Joystick #%i: %s\n      GUID: %s\n      Axes: %d\n      Buttons: %d\n      Balls: %d\n",
		did->joystick_id, SDL_GetJoystickName(did->joystick), guid_str, SDL_GetNumJoystickAxes(did->joystick),
		SDL_GetNumJoystickButtons(did->joystick), SDL_GetNumJoystickBalls(did->joystick));
}

void setup_controller_mappings(const struct didata* did, const int i)
{
	auto* const mapping = SDL_GetGamepadMapping(did->controller);
	write_log("Controller %i is mapped as \"%s\".\n", i, mapping);
	SDL_free(mapping);
}

void fix_didata(struct didata* did)
{
	did->axles = static_cast<uae_s16>(SDL_GetNumJoystickAxes(did->joystick));
	if (did->axles > MAX_MAPPINGS) {
		write_log("Warning: Joystick '%s' reports %d axes, clamping to %d\n",
			did->name.c_str(), did->axles, MAX_MAPPINGS);
		did->axles = MAX_MAPPINGS;
	}
	int hats = SDL_GetNumJoystickHats(did->joystick);
	if (hats > 0) hats = hats * 4;
	did->buttons_real = did->buttons = static_cast<uae_s16>(SDL_GetNumJoystickButtons(did->joystick) + hats);
	if (did->buttons > MAX_MAPPINGS) {
		write_log("Warning: Joystick '%s' reports %d buttons (incl. %d hat switches), clamping to %d\n",
			did->name.c_str(), did->buttons, hats / 4, MAX_MAPPINGS);
		did->buttons_real = did->buttons = MAX_MAPPINGS;
	}
	if (did->is_controller)
	{
		for (uae_s16 b = 0; b < did->buttons; b++)
		{
			did->buttonsort[b] = b;
			did->buttonmappings[b] = b;
			const auto button_name = SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(b));
			if (button_name != nullptr)
				did->buttonname[b] = button_name;
		}
		for (uae_s16 a = 0; a < did->axles; a++)
		{
			did->axissort[a] = a;
			did->axismappings[a] = a;
			const auto axis_name = SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(a));
			if (axis_name != nullptr)
				did->axisname[a] = axis_name;
			else
				did->axisname[a] = std::string("Axis ").append(std::to_string(a));
			if (a == SDL_GAMEPAD_AXIS_LEFTX || a == SDL_GAMEPAD_AXIS_RIGHTX)
				did->axistype[a] = AXISTYPE_POV_X;
			else if (a == SDL_GAMEPAD_AXIS_LEFTY || a == SDL_GAMEPAD_AXIS_RIGHTY)
				did->axistype[a] = AXISTYPE_POV_Y;
			else did->axistype[a] = AXISTYPE_NORMAL;
			if (a >= 2)
				did->analogstick = true;
		}
	}
	else
	{
		for (uae_s16 b = 0; b < did->buttons; b++)
		{
			did->buttonsort[b] = b;
			did->buttonmappings[b] = b;
			did->buttonname[b] = std::string("Button ").append(std::to_string(b));
		}
		for (uae_s16 a = 0; a < did->axles; a++)
		{
			did->axissort[a] = a;
			did->axismappings[a] = a;
			did->axisname[a] = std::string("Axis ").append(std::to_string(a));
			if (a == SDL_GAMEPAD_AXIS_LEFTX || a == SDL_GAMEPAD_AXIS_RIGHTX)
				did->axistype[a] = AXISTYPE_POV_X;
			else if (a == SDL_GAMEPAD_AXIS_LEFTY || a == SDL_GAMEPAD_AXIS_RIGHTY)
				did->axistype[a] = AXISTYPE_POV_Y;
			else did->axistype[a] = AXISTYPE_NORMAL;
			if (a >= 2)
				did->analogstick = true;
		}
	}

	fixbuttons(did);
	fixthings(did);
}

void setup_mapping(didata* did, const std::string& controllers, const int id)
{
	std::string retroarch_config_file;
	if (!did->joystick_name.empty())
	{
		const auto sanitized_name = sanitize_retroarch_name(did->joystick_name);
		retroarch_config_file = controllers + sanitized_name + ".cfg";
		write_log("Joystick name: '%s', sanitized to: '%s'\n", did->joystick_name.c_str(), sanitized_name.c_str());
		write_log("Checking for Retroarch cfg file: '%s'\n", retroarch_config_file.c_str());
	}

	const std::string retroarch_file = get_retroarch_file();
	const bool retroarch_config_exists = !retroarch_config_file.empty() && my_existsfile2(retroarch_config_file.c_str());
	const bool retroarch_file_exists = my_existsfile2(retroarch_file.c_str());

	if (retroarch_config_exists || retroarch_file_exists)
	{
		fill_blank_controller(did->mapping);

		if (retroarch_config_exists)
		{
			write_log("Retroarch controller cfg file found, using that for mapping\n");
			map_from_retroarch(did->mapping, retroarch_config_file, -1);
		}
		else
		{
			write_log("No Retroarch controller cfg file found, checking for mapping in retroarch.cfg\n");
			int found_player = -1;
			for (auto p = 1; p < 5; p++)
			{
				const int pindex = find_retroarch("input_player" + std::to_string(p) + "_joypad_index", retroarch_file);
				if (pindex == id)
				{
					found_player = p;
					break;
				}
			}
			if (found_player != -1)
			{
				write_log("Controller index found in retroarch cfg, using that for mapping\n");
				map_from_retroarch(did->mapping, retroarch_file, found_player);
			}
		}
	}
	else
	{
		const std::string controller_name = did->controller_name;
		const std::string controller_path = get_controllers_path();
		const std::string controller_file = controller_path + controller_name + ".controller";
		if (my_existsfile2(controller_file.c_str()))
		{
			write_log("Controller cfg file found, using that for mapping\n");
			// Seed with current defaults so older SDL2-era files that don't have
			// the newer SDL3 button slots keep sane identity mappings.
			fill_default_controller(did->mapping);
			read_controller_mapping_from_file(did->mapping, controller_file);
		}
		else
		{
			// No retroarch file, no controller file, use the default mapping
			write_log("No Retroarch controller cfg file found, using the default mapping\n");
			fill_default_controller(did->mapping);
		}
	}

	if (did->mapping.hotkey_button != SDL_GAMEPAD_BUTTON_INVALID)
	{
		for (auto& k : did->mapping.button)
		{
			if (k == did->mapping.hotkey_button)
				k = SDL_GAMEPAD_BUTTON_INVALID;
		}
	}
}

#include "input_platform_internal.h"

static int init_joystick()
{
	if (joystick_inited)
		return 1;
	joystick_inited = 1;
	
	input_platform_init_joystick(&num_joystick, di_joystick);
	return 1;
}

bool load_custom_options(uae_prefs* p, const std::string& option, const TCHAR* value)
{
	// Only do this loop if the option starts with "joyport"
	if (option.rfind("joyport", 0) == 0)
	{
		std::string buffer;

		for (int i = 0; i < MAX_JPORTS; ++i)
		{
			const jport *jp = &p->jports[i];

			// Check if configname contains JOY0, JOY1, JOY2, or JOY3
			int joy_index = -1;
			if (jp->idc.configname[0] && strncmp(jp->idc.configname, "JOY", 3) == 0 &&
				jp->idc.configname[4] == '\0' &&
				jp->idc.configname[3] >= '0' && jp->idc.configname[3] <= '3') {
				joy_index = jp->idc.configname[3] - '0';
				}
			if (joy_index == -1)
				continue;

			didata* did = &di_joystick[joy_index];

			for (int m = 0; m < 2; ++m)
			{
				std::string mode = m == 0 ? "none" : "hotkey";
				for (int n = 0; n < SDL_GAMEPAD_BUTTON_COUNT; ++n)
				{
					buffer = "joyport" + std::to_string(i) + "_amiberry_custom_" + mode + "_" + SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(n));
					if (buffer == option)
					{
						const auto b = (find_inputevent(value) > -1) ? remap_event_list[find_inputevent(value)] : 0;
						if (m == 0)
							did->mapping.amiberry_custom_none[n] = b;
						else
							did->mapping.amiberry_custom_hotkey[n] = b;
						return true;
					}
				}

				for (int n = 0; n < SDL_GAMEPAD_AXIS_COUNT; ++n)
				{
					buffer = "joyport" + std::to_string(i) + "_amiberry_custom_axis_" + mode + "_" + SDL_GetGamepadStringForAxis(static_cast<SDL_GamepadAxis>(n));
					if (buffer == option)
					{
						const auto b = (find_inputevent(value) > -1) ? remap_event_list[find_inputevent(value)] : 0;
						if (m == 0)
							did->mapping.amiberry_custom_axis_none[n] = b;
						else
							did->mapping.amiberry_custom_axis_hotkey[n] = b;
						return true;
					}
				}
			}
		}
	}

	return false;
}

void import_joysticks()
{
	joystick_inited = 0;
	init_joystick();
}

static void close_joystick()
{
	if (!joystick_inited)
		return;
	joystick_inited = 0;
	for (auto i = 0; i < num_joystick; i++)
		di_dev_free(&di_joystick[i]);
	num_joystick = 0;
	osj_device_index = -1;
	di_free();
}

static int acquire_joystick(const int num, int flags)
{
	if (num < 0)
		return 1;

	struct didata* did = &di_joystick[num];
	did->acquired = 1;
	
	return did->acquired > 0 ? 1 : 0;
}

static void unacquire_joystick(const int num)
{
	if (num < 0) {
		return;
	}

	struct didata* did = &di_joystick[num];
	did->acquired = 0;
}

static const TCHAR* get_joystick_friendlyname(const int joy)
{
	if (!di_joystick[joy].name.empty())
		return di_joystick[joy].name.c_str();
	return "";
}

static const TCHAR* get_joystick_uniquename(const int joy)
{
	TCHAR tmp[MAX_DPATH];
	_sntprintf(tmp, MAX_DPATH, _T("JOY%d"), joy);
	return my_strdup(tmp);
}

static int get_joystick_widget_num(const int joy)
{
	return di_joystick[joy].axles + di_joystick[joy].buttons;
}

static int get_joystick_widget_type(const int joy, const int num, TCHAR* name, uae_u32* code)
{
	//struct didata* did = &di_joystick[joy];
	if (num >= MAX_JOY_AXES && num < MAX_JOY_AXES + MAX_JOY_BUTTONS)
	{
		if (name)
		{
			switch (num)
			{
			case FIRST_JOY_BUTTON:
				_sntprintf(name, sizeof name, "Button X/CD32 red");
				break;
			case FIRST_JOY_BUTTON + 1:
				_sntprintf(name, sizeof name, "Button B/CD32 blue");
				break;
			case FIRST_JOY_BUTTON + 2:
				_sntprintf(name, sizeof name, "Button A/CD32 green");
				break;
			case FIRST_JOY_BUTTON + 3:
				_sntprintf(name, sizeof name, "Button Y/CD32 yellow");
				break;
			case FIRST_JOY_BUTTON + 4:
				_sntprintf(name, sizeof name, "CD32 start");
				break;
			case FIRST_JOY_BUTTON + 5:
				_sntprintf(name, sizeof name, "CD32 ffw");
				break;
			case FIRST_JOY_BUTTON + 6:
				_sntprintf(name, sizeof name, "CD32 rwd");
				break;
			default:
				break;
			}
		}
		return IDEV_WIDGET_BUTTON;
	}
	if (num < MAX_JOY_AXES)
	{
		if (name)
		{
			if (num == 0)
				_sntprintf(name, sizeof name, "X Axis");
			else if (num == 1)
				_sntprintf(name, sizeof name, "Y Axis");
			else
				_sntprintf(name, sizeof name, "Axis %d", num + 1);
		}
		return IDEV_WIDGET_AXIS;
	}

	return IDEV_WIDGET_NONE;
}

static int get_joystick_widget_first(const int joy, const int type)
{
	switch (type)
	{
	case IDEV_WIDGET_BUTTON:
		return di_joystick[joy].axles;
	case IDEV_WIDGET_AXIS:
		return 0;
	case IDEV_WIDGET_BUTTONAXIS:
		return di_joystick[joy].axles + di_joystick[joy].buttons;
	default:
		return -1;
	}
}

static int get_joystick_flags(int num)
{
	return 0;
}

int get_onscreen_joystick_device_index()
{
	return osj_device_index;
}

static bool invert_axis(int axis, const didata* did)
{
	switch (axis)
	{
	case SDL_GAMEPAD_AXIS_LEFTX:
		return did->mapping.lstick_axis_x_invert != 0;
	case SDL_GAMEPAD_AXIS_LEFTY:
		return did->mapping.lstick_axis_y_invert != 0;
	case SDL_GAMEPAD_AXIS_RIGHTX:
		return did->mapping.rstick_axis_x_invert != 0;
	case SDL_GAMEPAD_AXIS_RIGHTY:
		return did->mapping.rstick_axis_y_invert != 0;
	default:
		return false;
	}
}

void set_button_state(const didata* did, const int id, int button, const int button_offset, const bool is_controller)
{
	if (button != SDL_GAMEPAD_BUTTON_INVALID)
	{
		const auto button_state = is_controller
			? SDL_GetGamepadButton(did->controller, static_cast<SDL_GamepadButton>(button)) & 1
			: SDL_GetJoystickButton(did->joystick, button) & 1;
		setjoybuttonstate(id, button_offset, button_state);
	}
}

void set_axis_state(const int id, const int axis, int value, const bool invert)
{
	if (invert)
	{
		value = value * -1;
	}
	if (axisold[id][axis] != value)
	{
		setjoystickstate(id, axis, value, analog_upper_bound);
		axisold[id][axis] = value;
	}
}

void read_controller_button(const int id, const int button, const int state)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		auto held_offset = 0;
		if (did->mapping.hotkey_button > SDL_GAMEPAD_BUTTON_INVALID
			&& SDL_GetGamepadButton(did->controller, static_cast<SDL_GamepadButton>(did->mapping.hotkey_button)) & 1)
			held_offset = REMAP_BUTTONS;

		int retroarch_offset = SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT * 2;

		// detect RetroArch events, with or without Hotkey
		set_button_state(did, id, did->mapping.menu_button, retroarch_offset + 1, true);
		set_button_state(did, id, did->mapping.quit_button, retroarch_offset + 2, true);
		set_button_state(did, id, did->mapping.reset_button, retroarch_offset + 3, true);
		set_button_state(did, id, did->mapping.vkbd_button, retroarch_offset + 4, true);

		setjoybuttonstate(id, button + held_offset, state);
	}
}

void read_controller_axis(const int id, const int axis, const int value)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		// If analog mouse mapping is used, the Left stick acts as a mouse
		if (axis <= SDL_GAMEPAD_AXIS_LEFTY && currprefs.jports[id].mousemap > 0)
		{
			if (value > joystick_dead_zone || value < -joystick_dead_zone)
				setmousestate(id, axis, value / 10000, 0);
		}
		else
		{
			set_axis_state(id, axis, value, invert_axis(axis, did));
		}
	}
}

void read_joystick_button_single(const int id, const int button, const int state)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		// Per-device hotkey check (avoids global hotkey_pressed leak across devices)
		auto held_offset = 0;
		if (did->mapping.hotkey_button > SDL_GAMEPAD_BUTTON_INVALID
			&& SDL_GetJoystickButton(did->joystick, did->mapping.hotkey_button) & 1)
			held_offset = REMAP_BUTTONS;

		int retroarch_offset = SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT * 2;

		if (did->mapping.hotkey_button == SDL_GAMEPAD_BUTTON_INVALID
			|| SDL_GetJoystickButton(did->joystick, did->mapping.hotkey_button) & 1)
		{
			// detect RetroArch events, with or without Hotkey
			// Only check the specific RetroArch button that matches the event
			if (button == did->mapping.menu_button)
				setjoybuttonstate(id, retroarch_offset + 1, state);
			else if (button == did->mapping.quit_button)
				setjoybuttonstate(id, retroarch_offset + 2, state);
			else if (button == did->mapping.reset_button)
				setjoybuttonstate(id, retroarch_offset + 3, state);
			else if (button == did->mapping.vkbd_button)
				setjoybuttonstate(id, retroarch_offset + 4, state);
		}

		// Find which logical button this physical button maps to and dispatch directly
		int num_buttons = did->buttons > 15 ? did->buttons : 15;
		if (num_buttons > SDL_GAMEPAD_BUTTON_COUNT)
			num_buttons = SDL_GAMEPAD_BUTTON_COUNT;
		for (int did_button = 0; did_button < num_buttons; did_button++)
		{
			if (did->mapping.button[did_button] == button)
			{
				setjoybuttonstate(id, did_button + held_offset, state);
				break;
			}
		}
	}
}

void read_joystick_axis(const int id, const int axis, int value)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		// Find which logical axis this physical axis maps to and dispatch directly
		const int num_axes = did->axles < SDL_GAMEPAD_AXIS_COUNT ? did->axles : SDL_GAMEPAD_AXIS_COUNT;
		for (auto did_axis = 0; did_axis < num_axes; did_axis++)
		{
			if (did->mapping.axis[did_axis] == axis)
			{
				// If analog mouse mapping is used, the Left stick acts as a mouse
				if (did_axis <= SDL_GAMEPAD_AXIS_LEFTY && currprefs.jports[id].mousemap > 0)
				{
					if (value > joystick_dead_zone || value < -joystick_dead_zone)
						setmousestate(id, did_axis, value / 1000, 0);
				}
				else
				{
					set_axis_state(id, did_axis, value, invert_axis(did_axis, did));
				}
				break;
			}
		}
	}
}

void read_joystick_hat(const int id, int hat, const int value)
{
	if (isfocus() || currprefs.inactive_input & 4)
	{
		for (int button = SDL_GAMEPAD_BUTTON_DPAD_UP; button <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT; button++)
		{
			int state = 0;
			switch (button) {
			case SDL_GAMEPAD_BUTTON_DPAD_UP:
				state = value & SDL_HAT_UP;
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
				state = value & SDL_HAT_DOWN;
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
				state = value & SDL_HAT_LEFT;
				break;
			case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
				state = value & SDL_HAT_RIGHT;
				break;
			default: break;
			}
			setjoybuttonstate(id, button, state);
		}
	}
}

static void read_joystick()
{
	// Amiberry does not use this anymore.
	// All input events are handled in process_event()
}

struct inputdevice_functions inputdevicefunc_joystick = {
	init_joystick, close_joystick, acquire_joystick, unacquire_joystick,
	read_joystick, get_joystick_num, get_joystick_friendlyname, get_joystick_uniquename,
	get_joystick_widget_num, get_joystick_widget_type,
	get_joystick_widget_first,
	get_joystick_flags
};

// We use setid to set up custom events
int input_get_default_joystick(struct uae_input_device* uid, int i, int port, int af, int mode, bool gp, bool joymouseswap, bool default_osk)
{
	struct didata* did = nullptr;
	int h, v;

	if (joymouseswap) {
		if (i >= num_mouse)
			return 0;
		did = &di_mouse[i];
	}
	else {
		if (i >= num_joystick)
			return 0;
		did = &di_joystick[i];
	}
	if (mode == JSEM_MODE_MOUSE_CDTV) {
		h = INPUTEVENT_MOUSE_CDTV_HORIZ;
		v = INPUTEVENT_MOUSE_CDTV_VERT;
	} else if (port >= 2) {
		h = port == 3 ? INPUTEVENT_PAR_JOY2_HORIZ : INPUTEVENT_PAR_JOY1_HORIZ;
		v = port == 3 ? INPUTEVENT_PAR_JOY2_VERT : INPUTEVENT_PAR_JOY1_VERT;
	} else {
		// In Amiberry, we treat Mouse type option as real Mouse input events
		if (mode == JSEM_MODE_MOUSE)
		{
			h = port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ;
			v = port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT;
		}
		else
		{
			h = port ? INPUTEVENT_JOY2_HORIZ : INPUTEVENT_JOY1_HORIZ;
			v = port ? INPUTEVENT_JOY2_VERT : INPUTEVENT_JOY1_VERT;
		}
	}
	setid(uid, i, ID_AXIS_OFFSET + SDL_GAMEPAD_AXIS_LEFTX, 0, port, h, gp);
	setid(uid, i, ID_AXIS_OFFSET + SDL_GAMEPAD_AXIS_LEFTY, 0, port, v, gp);

	if (port >= 2) {
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_SOUTH, 0, port, port == 3 ? INPUTEVENT_PAR_JOY2_FIRE_BUTTON : INPUTEVENT_PAR_JOY1_FIRE_BUTTON, af, gp);
	}
	else {
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_SOUTH, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_EAST))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_EAST, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
		if (mode != JSEM_MODE_JOYSTICK) {
			//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_WEST))
				setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_WEST, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);
		}
	}

	for (int j = 2; j < MAX_MAPPINGS - 1; j++) {
		const int type = did->axistype[j];
		if (type == AXISTYPE_POV_X) {
			setid(uid, i, ID_AXIS_OFFSET + j + 0, 0, port, h, gp);
			setid(uid, i, ID_AXIS_OFFSET + j + 1, 0, port, v, gp);
			j++;
		}
	}

	if (mode == JSEM_MODE_JOYSTICK_CD32) {
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_SOUTH, port, port ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED, af, gp);
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_EAST)) {
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_EAST, 0, port, port ? INPUTEVENT_JOY2_CD32_BLUE : INPUTEVENT_JOY1_CD32_BLUE, gp);
		//}
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_WEST))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_WEST, 0, port, port ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN, gp);
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_NORTH))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_NORTH, 0, port, port ? INPUTEVENT_JOY2_CD32_YELLOW : INPUTEVENT_JOY1_CD32_YELLOW, gp);
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0, port, port ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD, gp);
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0, port, port ? INPUTEVENT_JOY2_CD32_FFW : INPUTEVENT_JOY1_CD32_FFW, gp);
		//if (isrealbutton(did, SDL_GAMEPAD_BUTTON_START))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_START, 0, port, port ? INPUTEVENT_JOY2_CD32_PLAY : INPUTEVENT_JOY1_CD32_PLAY, gp);
	}

	// Map D-Pad buttons if they exist. If Mouse Mode is set, they function as mouse movements, otherwise as digital Joystick directions
	if (mode == JSEM_MODE_MOUSE)
	{
		// Map D-Pad buttons to mouse movement
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_UP, 0, port, port ? INPUTEVENT_MOUSE2_UP : INPUTEVENT_MOUSE1_UP, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, port, port ? INPUTEVENT_MOUSE2_DOWN : INPUTEVENT_MOUSE1_DOWN, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, port, port ? INPUTEVENT_MOUSE2_LEFT : INPUTEVENT_MOUSE1_LEFT, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, port, port ? INPUTEVENT_MOUSE2_RIGHT : INPUTEVENT_MOUSE1_RIGHT, gp);
	}
	else
	{
		if (port >= 2)
		{
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_UP, 0, port, port == 3 ? INPUTEVENT_PAR_JOY2_UP : INPUTEVENT_PAR_JOY1_UP, gp);
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, port, port == 3 ? INPUTEVENT_PAR_JOY2_DOWN : INPUTEVENT_PAR_JOY1_DOWN, gp);
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, port, port == 3 ? INPUTEVENT_PAR_JOY2_LEFT : INPUTEVENT_PAR_JOY1_LEFT, gp);
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, port, port == 3 ? INPUTEVENT_PAR_JOY2_RIGHT : INPUTEVENT_PAR_JOY1_RIGHT, gp);
		}
		else
		{
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_UP, 0, port, port ? INPUTEVENT_JOY2_UP : INPUTEVENT_JOY1_UP, gp);
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, port, port ? INPUTEVENT_JOY2_DOWN : INPUTEVENT_JOY1_DOWN, gp);
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, port, port ? INPUTEVENT_JOY2_LEFT : INPUTEVENT_JOY1_LEFT, gp);
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, port, port ? INPUTEVENT_JOY2_RIGHT : INPUTEVENT_JOY1_RIGHT, gp);
		}
	}

	// If mouse map is enabled (we're emulating a mouse on Port 0 from this controller's Analog stick)
	// we need to enable 2 mouse buttons for Port 0 as well. These are currently LShoulder / RShoulder respectively
	if (currprefs.jports[port].mousemap > 0)
	{
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0, port, INPUTEVENT_JOY1_FIRE_BUTTON, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0, port, INPUTEVENT_JOY1_2ND_BUTTON, gp);
	}

	// Configure a few extra default mappings, for convenience
	std::array<int, SDL_GAMEPAD_BUTTON_COUNT> button_map[2]{};
	button_map[0] = did->mapping.amiberry_custom_none;
	button_map[1] = did->mapping.amiberry_custom_hotkey;

	std::array<int, SDL_GAMEPAD_AXIS_COUNT> axis_map[2]{};
	axis_map[0] = did->mapping.amiberry_custom_axis_none;
	axis_map[1] = did->mapping.amiberry_custom_axis_hotkey;

	// Only applies for normal joysticks, as CD32 pads will use these buttons already
	if (mode < JSEM_MODE_JOYSTICK_CD32)
	{
		// Key P: commonly used for Pause in many games => START
		if (!button_map[0][SDL_GAMEPAD_BUTTON_START])
			setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_START, 0, port, INPUTEVENT_KEY_P, gp);

		// Mouse map uses these already, so we skip them if that's enabled
		if (currprefs.jports[port].mousemap == 0)
		{
			// Space bar used on many games as a 2nd fire button => LShoulder
			if (!button_map[0][SDL_GAMEPAD_BUTTON_LEFT_SHOULDER])
				setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 0, port, INPUTEVENT_KEY_SPACE, gp);

			// Return => RShoulder
			if (!button_map[0][SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER])
				setid(uid, i, ID_BUTTON_OFFSET + SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 0, port, INPUTEVENT_KEY_RETURN, gp);
		}
	}

	// Set the events for any Custom Controls mapping
	for (auto n = 0; n < 2; ++n)
	{
		const auto function_offset = n * REMAP_BUTTONS;
		for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
		{
			if (button_map[n][button])
				setid(uid, i, ID_BUTTON_OFFSET + button + function_offset, 0, port, button_map[n][button], af, gp);
		}

		// Axis mapping, skip the first 2 (Horizontal / Vertical on Left stick)
		// as they are already handled above
		for (int axis = 2; axis < SDL_GAMEPAD_AXIS_COUNT; axis++)
		{
			if (axis_map[n][axis])
				setid(uid, i, ID_AXIS_OFFSET + axis + function_offset, 0, port, axis_map[n][axis], af, gp);
		}
	}

	// We put any RetroArch special mappings above the controller's max value (Max Buttons + Max Axes)
	// to avoid clashing with any custom mappings above, as well as the axis-as-button mappings.
	// Note that newer versions of SDL3 might have a higher value in SDL_GAMEPAD_BUTTON_COUNT / SDL_GAMEPAD_AXIS_COUNT
	// This is already the case between 2.0.10 and 2.0.18 for example
	int retroarch_offset = SDL_GAMEPAD_BUTTON_COUNT + SDL_GAMEPAD_AXIS_COUNT * 2;
	if (currprefs.use_retroarch_menu)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 1, 0, port, INPUTEVENT_SPC_ENTERGUI, gp);
	
	if (currprefs.use_retroarch_quit)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 2, 0, port, INPUTEVENT_SPC_QUIT, gp);
	
	if (currprefs.use_retroarch_reset)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 3, 0, port, INPUTEVENT_SPC_SOFTRESET, gp);

	if (currprefs.use_retroarch_vkbd)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 4, 0, port, INPUTEVENT_SPC_OSK, gp);

	if (i >= 0 && i < num_joystick)
		return 1;
	
	return 0;
}

int input_get_default_joystick_analog(struct uae_input_device* uid, int i, int port, int af, bool gp, bool joymouseswap, bool default_osk)
{
	struct didata* did;
	
	if (joymouseswap) {
		if (i >= num_mouse)
			return 0;
		did = &di_mouse[i];
	}
	else {
		if (i >= num_joystick)
			return 0;
		did = &di_joystick[i];
	}
	setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_HORIZ_POT : INPUTEVENT_JOY1_HORIZ_POT, gp);
	setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_VERT_POT : INPUTEVENT_JOY1_VERT_POT, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_LEFT : INPUTEVENT_JOY1_LEFT, af, gp);
	if (isrealbutton(did, 1))
		setid(uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_RIGHT : INPUTEVENT_JOY1_RIGHT, gp);
	if (isrealbutton(did, 2))
		setid(uid, i, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_UP : INPUTEVENT_JOY1_UP, gp);
	if (isrealbutton(did, 3))
		setid(uid, i, ID_BUTTON_OFFSET + 3, 0, port, port ? INPUTEVENT_JOY2_DOWN : INPUTEVENT_JOY1_DOWN, gp);

	for (auto j = 2; j < MAX_MAPPINGS - 1; j++) {
		const int type = did->axistype[j];
		if (type == AXISTYPE_POV_X) {
			setid(uid, i, ID_AXIS_OFFSET + j + 0, 0, port, port ? INPUTEVENT_JOY2_HORIZ_POT : INPUTEVENT_JOY1_HORIZ_POT, gp);
			setid(uid, i, ID_AXIS_OFFSET + j + 1, 0, port, port ? INPUTEVENT_JOY2_VERT_POT : INPUTEVENT_JOY1_VERT_POT, gp);
			j++;
		}
	}
	
	if (i == 0)
		return 1;
	return 0;
}
