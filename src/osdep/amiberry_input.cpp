#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"
#include "amiberry_gfx.h"
#include "fsdb.h"
#include "uae.h"
#include "xwin.h"

#define MAX_MOUSE_BUTTONS	  3
#define MAX_MOUSE_AXES        4
#define FIRST_MOUSE_AXIS	  0
#define FIRST_MOUSE_BUTTON	MAX_MOUSE_AXES
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define SET_BIT(var,pos) ((var) |= 1 << (pos))

#define MAX_JOY_BUTTONS	  16
#define MAX_JOY_AXES	   8
#define FIRST_JOY_AXIS	   0
#define FIRST_JOY_BUTTON	MAX_JOY_AXES

static struct didata di_mouse[MAX_INPUT_DEVICES];
static struct didata di_keyboard[MAX_INPUT_DEVICES];
struct didata di_joystick[MAX_INPUT_DEVICES];

static struct host_input_button default_controller_map;

static int num_mouse = 1, num_keyboard = 1, num_joystick = 0, num_retroarch_kbdjoy = 0;
static int joystick_inited, retroarch_inited;
const auto analog_upper_bound = 32767;
const auto analog_lower_bound = -analog_upper_bound;

static int isrealbutton(struct didata* did, int num)
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
	did->buttonname[0] = my_strdup(_T("Button"));
	did->buttons++;
}

static void addplusminus(struct didata* did, int i)
{
	if (did->buttons + 1 >= ID_BUTTON_TOTAL)
		return;
	for (uae_s16 j = 0; j < 2; j++) {
		TCHAR tmp[256];
		_stprintf(tmp, _T("%s [%c]"), did->axisname[i], j ? '+' : '-');
		did->buttonname[did->buttons] = my_strdup(tmp);
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

int find_in_array(const int arr[], int n, int key)
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

static void fill_default_controller()
{
	default_controller_map.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.quit_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.reset_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.menu_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;

	default_controller_map.lstick_axis_y_invert = false;
	default_controller_map.lstick_axis_x_invert = false;
	default_controller_map.rstick_axis_y_invert = false;
	default_controller_map.rstick_axis_x_invert = false;

	default_controller_map.number_of_hats = 1;
	default_controller_map.number_of_axis = -1;
	default_controller_map.is_retroarch = false;

	for (auto b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
		default_controller_map.button[b] = b;

	for (auto a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++)
		default_controller_map.axis[a] = a;
}

static void fill_blank_controller()
{
	default_controller_map.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.quit_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.reset_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.menu_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.vkbd_button = SDL_CONTROLLER_BUTTON_INVALID;

	default_controller_map.lstick_axis_y_invert = false;
	default_controller_map.lstick_axis_x_invert = false;
	default_controller_map.rstick_axis_y_invert = false;
	default_controller_map.rstick_axis_x_invert = false;

	default_controller_map.number_of_hats = -1;
	default_controller_map.number_of_axis = -1;
	default_controller_map.is_retroarch = false;

	for (auto& b : default_controller_map.button)
		b = SDL_CONTROLLER_BUTTON_INVALID;

	for (auto& axi : default_controller_map.axis)
		axi = SDL_CONTROLLER_AXIS_INVALID;
}

constexpr int default_button_mapping[] = {
	INPUTEVENT_JOY2_CD32_RED,  INPUTEVENT_JOY2_CD32_BLUE, INPUTEVENT_JOY2_CD32_GREEN, INPUTEVENT_JOY2_CD32_YELLOW,
	0, 0, INPUTEVENT_JOY2_CD32_PLAY, 0, 0, INPUTEVENT_JOY2_CD32_RWD, INPUTEVENT_JOY2_CD32_FFW,
	INPUTEVENT_JOY2_UP, INPUTEVENT_JOY2_DOWN, INPUTEVENT_JOY2_LEFT, INPUTEVENT_JOY2_RIGHT,
	0, 0, 0, 0, 0, 0
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
	INPUTEVENT_KEY_ESC, INPUTEVENT_KEY_TAB, INPUTEVENT_KEY_CTRL, INPUTEVENT_KEY_CAPS_LOCK, INPUTEVENT_KEY_LTGT,
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

	INPUTEVENT_SPC_DISKSWAPPER_NEXT, INPUTEVENT_SPC_DISKSWAPPER_PREV,
	INPUTEVENT_SPC_DISKSWAPPER_INSERT0, INPUTEVENT_SPC_DISKSWAPPER_INSERT1,
	INPUTEVENT_SPC_DISKSWAPPER_INSERT2, INPUTEVENT_SPC_DISKSWAPPER_INSERT3,

	INPUTEVENT_SPC_MOUSEMAP_PORT0_LEFT, INPUTEVENT_SPC_MOUSEMAP_PORT0_RIGHT,
	INPUTEVENT_SPC_MOUSEMAP_PORT1_LEFT, INPUTEVENT_SPC_MOUSEMAP_PORT1_RIGHT,
	INPUTEVENT_SPC_MOUSE_SPEED_DOWN, INPUTEVENT_SPC_MOUSE_SPEED_UP, INPUTEVENT_SPC_SHUTDOWN,
	INPUTEVENT_SPC_WARP, INPUTEVENT_SPC_TOGGLE_JIT, INPUTEVENT_SPC_TOGGLE_JIT_FPU,
	INPUTEVENT_SPC_AUTO_CROP_IMAGE, INPUTEVENT_SPC_TOGGLE_VIRTUAL_KEYBOARD
};

constexpr int remap_event_list_size = std::size(remap_event_list);

const TCHAR* find_inputevent_name(int key)
{
	const auto* output = "None";

	for (auto i = 0; i < remap_event_list_size; ++i)
	{
		if (remap_event_list[i] == key)
		{
			const auto* tempevent = inputdevice_get_eventinfo(remap_event_list[i]);
			output = _T(tempevent->name);
			break;
		}
		output = "None";
	}
	return output;
}

int find_inputevent(TCHAR* key)
{
	auto index = -1;
	char tmp1[255], tmp2[255];

	for (auto i = 0; i < remap_event_list_size; ++i)
	{
		const auto* tempevent = inputdevice_get_eventinfo(remap_event_list[i]);
		snprintf(tmp1, 255, "%s", tempevent->name);
		snprintf(tmp2, 255, "%s", key);

		if (!_tcscmp(tmp1, tmp2))
		{
			index = i;
			break;
		}
	}
	return index;
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

const char* remap_key_map_list_strings[] = {
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

constexpr int remap_key_map_list_size = std::size(remap_key_map_list);

//#define	MAX_KEYCODES 256
//static uae_u8 di_keycodes[MAX_INPUT_DEVICES][MAX_KEYCODES];
static int keyboard_german;

int keyhack (int scancode, int pressed, int num)
{
	static unsigned char backslashstate, apostrophstate;
	const Uint8* state = SDL_GetKeyboardState(NULL);

	// release mouse if TAB and ALT is pressed (but only if option is enabled)
	if (currprefs.alt_tab_release)
	{
		if (pressed && state[SDL_SCANCODE_LALT] && scancode == SDL_SCANCODE_TAB) {
			disablecapture();
			return -1;
		}
	}

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
	return scancode;
}

static void cleardid(struct didata* did)
{
	memset(did, 0, sizeof(*did));
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
		SDL_GameControllerClose(did->controller);
		did->controller = nullptr;
	}
	if (did->joystick != nullptr && !did->is_controller)
	{
		SDL_JoystickClose(did->joystick);
		did->joystick = nullptr;
	}
	cleardid(did);
}

static void di_free(void)
{
	for (auto i = 0; i < MAX_INPUT_DEVICES; i++) {
		di_dev_free(&di_joystick[i]);
		di_dev_free(&di_mouse[i]);
		di_dev_free(&di_keyboard[i]);
	}
}

int is_touch_lightpen(void)
{
	return 0;
}

int is_tablet(void)
{
	//return (tablet || os_touch) ? 1 : 0;
	return 0;
}

static int init_mouse()
{
	struct didata* did = di_mouse;
	
	num_mouse = 1;
	did->name = "System mouse";
	did->buttons = 3;
	did->axles = 4;
	did->axissort[0] = 0;
	did->axisname[0] = my_strdup(_T("X Axis"));
	did->axissort[1] = 1;
	did->axisname[1] = my_strdup(_T("Y Axis"));
	if (did->axles > 2) {
		did->axissort[2] = 2;
		did->axisname[2] = my_strdup(_T("Wheel"));
		addplusminus(did, 2);
	}
	if (did->axles > 3) {
		did->axissort[3] = 3;
		did->axisname[3] = my_strdup(_T("HWheel"));
		addplusminus(did, 3);
	}
	return 1;
}

static void close_mouse()
{
	for (auto i = 0; i < num_mouse; i++)
		di_dev_free(&di_mouse[i]);
	di_free();
}

static int acquire_mouse(const int num, int flags)
{
	if (num < 0) {
		return 1;
	}

	struct AmigaMonitor* mon = &AMonitors[0];
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
	struct AmigaMonitor* mon = &AMonitors[0];
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
	if (num_mouse > 0 && mouse == 0)
		return "MOUSE0";

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
			_tcscpy(name, did->buttonname[num - axles]);
		return IDEV_WIDGET_BUTTONAXIS;
	}
	if (num >= axles && num < axles + realbuttons) {
		if (name)
			_tcscpy(name, did->buttonname[num - axles]);
		return IDEV_WIDGET_BUTTON;
	}
	if (num < axles) {
		if (name)
			_tcscpy(name, did->axisname[num]);
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
	struct didata* did = NULL;
	
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
	struct didata* did = NULL;

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
	keyboard_german = 0;
	if (SDL_GetKeyFromScancode(SDL_SCANCODE_Y) == SDLK_z)
		keyboard_german = 1;

	if (retroarch_inited) return 1;
	
	// Check if we have a Retroarch file
	char retroarch_file[MAX_DPATH];
	get_retroarch_file(retroarch_file, MAX_DPATH);
	if (my_existsfile2(retroarch_file))
	{
		// Add as many keyboards as joysticks that are setup
		// on arcade machines, you could have a 4 player ipac using all keyboard buttons
		// so you want to have at least 4 keyboards to choose from!
		// once one config is missing, simply stop adding them!
		auto valid = true;
		for (auto kb = 0; kb < 4 && valid; ++kb)
		{
			valid = init_kb_from_retroarch(kb, retroarch_file);
			if (valid) num_retroarch_kbdjoy++;
		}
	}
	retroarch_inited = 1;	
	return 1;
}

static void close_kb()
{
}

void release_keys(void)
{
	SDL_PumpEvents();

	//for (int j = 0; j < MAX_INPUT_DEVICES; j++) {
	//	for (int i = 0; i < MAX_KEYCODES; i++) {
	//		if (di_keycodes[j][i]) {
	//			di_keycodes[j][i] = 0;
	//			my_kbd_handler(j, i, 0, true);
	//		}
	//	}
	//}
}

static int acquire_kb(int num, int flags)
{
	struct AmigaMonitor* mon = &AMonitors[0];
	struct didata* did = &di_keyboard[num];
	did->acquired = 1;
	return did->acquired > 0 ? 1 : 0;
}

static void unacquire_kb(int num)
{
	struct didata* did = &di_keyboard[num];
	struct AmigaMonitor* mon = &AMonitors[0];
	did->acquired = 0;
}

static void read_kb()
{
}

void wait_keyrelease(void)
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

static int init_joystick()
{
	struct didata* did;
	
	if (joystick_inited)
		return 1;
	joystick_inited = 1;
	
	// This disables the use of gyroscopes as axis device
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	
	num_joystick = SDL_NumJoysticks();
	if (num_joystick > MAX_INPUT_DEVICES)
		num_joystick = MAX_INPUT_DEVICES;

	// set up variables / paths etc.
	char controllers_path[MAX_DPATH];
	get_controllers_path(controllers_path, MAX_DPATH);

	char cfg[MAX_DPATH];
	get_configuration_path(cfg, MAX_DPATH);
	strcat(cfg, "gamecontrollerdb.txt");
	SDL_GameControllerAddMappingsFromFile(cfg);

	get_controllers_path(cfg, MAX_DPATH);
	strcat(cfg, "gamecontrollerdb_user.txt");
	SDL_GameControllerAddMappingsFromFile(cfg);
	
	// Possible scenarios:
	// 1 - Controller is an SDL2 Game Controller, no retroarch file: we use the default mapping
	// 2 - Controller is an SDL2 Game Controller, but there's a retroarch file: retroarch overrides default mapping
	// 3 - Controller is not an SDL2 Game Controller, but there's a retroarch file: open it as Joystick, use retroarch mapping
	// 4 - Controller is not an SDL2 Game Controller, no retroarch file: open as Joystick with default map
	
	char guid_str[33];
	// do the loop
	for (auto i = 0; i < num_joystick; i++)
	{
		did = &di_joystick[i];
		// Check if joystick supports SDL's game controller interface (a mapping is available)
		if (SDL_IsGameController(i))
		{
			did->controller = SDL_GameControllerOpen(i);
			if (did->controller == nullptr)
			{
				write_log("Warning: Unable to open game controller! SDL Error: %s\n", SDL_GetError());
				continue;
			}
			did->is_controller = true;
			did->joystick = SDL_GameControllerGetJoystick(did->controller);
			did->joystick_id = SDL_JoystickInstanceID(did->joystick);
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(did->joystick), guid_str, 33);

			if (SDL_GameControllerNameForIndex(i) != nullptr)
				did->controller_name.assign(SDL_GameControllerNameForIndex(i));
			write_log("Controller #%i: %s\n      GUID: %s\n", did->joystick_id, SDL_GameControllerName(did->controller), guid_str);

			// Try to get the Joystick Name as well, we will need it in case of RetroArch mapping files
			if (SDL_JoystickNameForIndex(i) != nullptr)
				did->joystick_name.assign(SDL_JoystickNameForIndex(i));

			if (!did->controller_name.empty())
				did->name = did->controller_name;
			else
				did->name = did->joystick_name;

			auto* const mapping = SDL_GameControllerMapping(did->controller);
			write_log("Controller %i is mapped as \"%s\".\n", i, mapping);
			SDL_free(mapping);
		}
		// Controller interface not supported, try to open as joystick
		else
		{
			did->joystick = SDL_JoystickOpen(i);
			if (did->joystick == nullptr)
			{
				write_log("Warning: Unable to open Joystick! SDL Error: %s\n", SDL_GetError());
				continue;
			}
			did->is_controller = false;
			did->joystick_id = SDL_JoystickInstanceID(did->joystick);
			SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(did->joystick), guid_str, 33);
			
			if (SDL_JoystickNameForIndex(i) != nullptr)
				did->joystick_name.assign(SDL_JoystickNameForIndex(i));
			did->name = did->joystick_name;
			write_log("Joystick #%i: %s\n      GUID: %s\n      Axes: %d\n      Buttons: %d\n      Balls: %d\n",
				did->joystick_id, SDL_JoystickName(did->joystick), guid_str, SDL_JoystickNumAxes(did->joystick),
									SDL_JoystickNumButtons(did->joystick), SDL_JoystickNumBalls(did->joystick));
			write_log("Joystick #%i does not have a mapping available\n", did->joystick_id);
		}

		did->axles = static_cast<uae_s16>(SDL_JoystickNumAxes(did->joystick));
		int hats = SDL_JoystickNumHats(did->joystick);
		if (hats > 0) hats = hats * 4;
		did->buttons_real = did->buttons = static_cast<uae_s16>(SDL_JoystickNumButtons(did->joystick) + hats);
		if (did->is_controller)
		{
			for (uae_s16 b = 0; b < did->buttons; b++)
			{
				did->buttonsort[b] = b;
				did->buttonmappings[b] = b;
				const auto button_name = SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(b));
				if (button_name != nullptr)
					did->buttonname[b] = my_strdup(button_name);
			}
			for (uae_s16 a = 0; a < did->axles; a++)
			{
				did->axissort[a] = a;
				did->axismappings[a] = a;
				const auto axis_name = SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(a));
				if (axis_name != nullptr)
					did->axisname[a] = my_strdup(axis_name);
				if (a == SDL_CONTROLLER_AXIS_LEFTX || a == SDL_CONTROLLER_AXIS_RIGHTX)
					did->axistype[a] = AXISTYPE_POV_X;
				else if (a == SDL_CONTROLLER_AXIS_LEFTY || a == SDL_CONTROLLER_AXIS_RIGHTY)
					did->axistype[a] = AXISTYPE_POV_Y;
				else did->axistype[a] = AXISTYPE_NORMAL;
				if (a >= 2)
					did->analogstick = true;
			}
		}
		else
		{
			TCHAR tmp[100];
			for (uae_s16 b = 0; b < did->buttons; b++)
			{
				did->buttonsort[b] = b;
				did->buttonmappings[b] = b;
				_stprintf(tmp, _T("Button %d"), b);
				did->buttonname[b] = my_strdup(tmp);
			}
			for (uae_s16 a = 0; a < did->axles; a++)
			{
				did->axissort[a] = a;
				did->axismappings[a] = a;
				_stprintf(tmp, _T("Axis %d"), a);
				did->axisname[a] = my_strdup(tmp);
				if (a == SDL_CONTROLLER_AXIS_LEFTX || a == SDL_CONTROLLER_AXIS_RIGHTX)
					did->axistype[a] = AXISTYPE_POV_X;
				else if (a == SDL_CONTROLLER_AXIS_LEFTY || a == SDL_CONTROLLER_AXIS_RIGHTY)
					did->axistype[a] = AXISTYPE_POV_Y;
				else did->axistype[a] = AXISTYPE_NORMAL;
				if (a >= 2)
					did->analogstick = true;
			}
		}

		fixbuttons(did);
		fixthings(did);

		auto retroarch_config_file = string(controllers_path);
		const auto sanitized_name = sanitize_retroarch_name(did->joystick_name);
		retroarch_config_file += sanitized_name + ".cfg";
		write_log("Joystick name: '%s', sanitized to: '%s'\n", did->joystick_name.c_str(), sanitized_name.c_str());

		if (my_existsfile2(retroarch_config_file.c_str()))
		{
			write_log("Retroarch controller cfg file found, using that for mapping\n");
			fill_blank_controller();
			did->mapping = default_controller_map;
			did->mapping = map_from_retroarch(did->mapping, retroarch_config_file.data(), -1);
		}
		else
		{
			write_log("No Retroarch controller cfg file found, checking for mapping in retroarch.cfg\n");
			// Check if values are in retroarch.cfg
			char retroarch_file[MAX_DPATH];
			get_retroarch_file(retroarch_file, MAX_DPATH);
			if (my_existsfile2(retroarch_file))
			{
				int found_player = -1;
				for (auto p = 1; p < 5; p++) 
				{
					const int pindex = find_retroarch(("input_player" + to_string(p) + "_joypad_index").c_str(), retroarch_file);
					if (pindex == i) 
					{
						found_player = p;
						break;
					}
				}
				if (found_player != -1) 
				{
					write_log("Controller index found in retroarch cfg, using that for mapping\n");
					fill_blank_controller();
					did->mapping = default_controller_map;
					did->mapping = map_from_retroarch(did->mapping, retroarch_file, found_player);
				}
				else 
				{
					write_log("No controller index found in retroarch cfg, using the default mapping\n");
					fill_default_controller();
					did->mapping = default_controller_map;
				}
			}
			else
			{
				write_log("No Retroarch controller cfg file found, using the default mapping\n");
				fill_default_controller();
				did->mapping = default_controller_map;
			}
		}

		if (did->mapping.hotkey_button != SDL_CONTROLLER_BUTTON_INVALID)
		{
			for (auto& k : did->mapping.button)
			{
				if (k == did->mapping.hotkey_button)
					k = SDL_CONTROLLER_BUTTON_INVALID;
			}
		}
	}
	return 1;
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

static void unacquire_joystick(int num)
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
	_stprintf(tmp, _T("JOY%d"), joy);
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
				sprintf(name, "Button X/CD32 red");
				break;
			case FIRST_JOY_BUTTON + 1:
				sprintf(name, "Button B/CD32 blue");
				break;
			case FIRST_JOY_BUTTON + 2:
				sprintf(name, "Button A/CD32 green");
				break;
			case FIRST_JOY_BUTTON + 3:
				sprintf(name, "Button Y/CD32 yellow");
				break;
			case FIRST_JOY_BUTTON + 4:
				sprintf(name, "CD32 start");
				break;
			case FIRST_JOY_BUTTON + 5:
				sprintf(name, "CD32 ffw");
				break;
			case FIRST_JOY_BUTTON + 6:
				sprintf(name, "CD32 rwd");
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
				sprintf(name, "X Axis");
			else if (num == 1)
				sprintf(name, "Y Axis");
			else
				sprintf(name, "Axis %d", num + 1);
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

static bool invert_axis(int axis, const didata* did)
{
	if (axis == SDL_CONTROLLER_AXIS_LEFTX && did->mapping.lstick_axis_x_invert != 0)
		return true;
	if (axis == SDL_CONTROLLER_AXIS_LEFTY && did->mapping.lstick_axis_y_invert != 0)
		return true;
	if (axis == SDL_CONTROLLER_AXIS_RIGHTX && did->mapping.rstick_axis_x_invert != 0)
		return true;
	if (axis == SDL_CONTROLLER_AXIS_RIGHTY && did->mapping.rstick_axis_y_invert != 0)
		return true;
	return false;
}

void read_controller_button(const int id, const int button, const int state)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		auto held_offset = 0;
		if (did->mapping.hotkey_button > SDL_CONTROLLER_BUTTON_INVALID
				&& SDL_GameControllerGetButton(did->controller, static_cast<SDL_GameControllerButton>(did->mapping.hotkey_button)) & 1)
				held_offset = REMAP_BUTTONS;

		int retroarch_offset = SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_MAX * 2;

		// detect RetroArch events, with or without Hotkey
		if (did->mapping.menu_button != SDL_CONTROLLER_BUTTON_INVALID)
		{
			setjoybuttonstate(id, retroarch_offset + 1,
				SDL_GameControllerGetButton(did->controller,
					static_cast<SDL_GameControllerButton>(did->mapping.menu_button)) & 1);
		}
		if (did->mapping.quit_button != SDL_CONTROLLER_BUTTON_INVALID)
		{
			setjoybuttonstate(id, retroarch_offset + 2,
				SDL_GameControllerGetButton(did->controller,
					static_cast<SDL_GameControllerButton>(did->mapping.quit_button)) & 1);
		}
		if (did->mapping.reset_button != SDL_CONTROLLER_BUTTON_INVALID)
		{
			setjoybuttonstate(id, retroarch_offset + 3,
				SDL_GameControllerGetButton(did->controller,
					static_cast<SDL_GameControllerButton>(did->mapping.reset_button)) & 1);
		}
		if (did->mapping.vkbd_button != SDL_CONTROLLER_BUTTON_INVALID)
		{
			setjoybuttonstate(id, retroarch_offset + 4,
				SDL_GameControllerGetButton(did->controller,
					static_cast<SDL_GameControllerButton>(did->mapping.vkbd_button)) & 1);
		}

 		setjoybuttonstate(id, button + held_offset, state);
	}
}

void read_controller_axis(const int id, const int axis, int value)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		// If analog mouse mapping is used, the Left stick acts as a mouse
		if (axis <= SDL_CONTROLLER_AXIS_LEFTY && did->mousemap > 0)
		{
			if (value > joystick_dead_zone || value < -joystick_dead_zone)
				setmousestate(id, axis, value / 10000, 0);
		}
		else
		{
			if (invert_axis(axis, did))
			{
				value = value * -1;
			}
			if (axisold[id][axis] != value) {
				setjoystickstate(id, axis, value, analog_upper_bound);
				axisold[id][axis] = value;
			}
		}
	}
}

void read_joystick_button(const int id, const int button, const int state)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		auto held_offset = 0;
		if (did->mapping.hotkey_button > SDL_CONTROLLER_BUTTON_INVALID
			&& SDL_JoystickGetButton(did->joystick, did->mapping.hotkey_button) & 1)
			held_offset = REMAP_BUTTONS;

		int retroarch_offset = SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_MAX * 2;

		// detect RetroArch events, with or without Hotkey
		if (did->mapping.hotkey_button == SDL_CONTROLLER_BUTTON_INVALID
			|| SDL_JoystickGetButton(did->joystick, did->mapping.hotkey_button) & 1)
		{
			if (did->mapping.menu_button != SDL_CONTROLLER_BUTTON_INVALID)
			{
				setjoybuttonstate(id, retroarch_offset + 1,
					SDL_JoystickGetButton(did->joystick, did->mapping.menu_button) & 1);
			}
			if (did->mapping.quit_button != SDL_CONTROLLER_BUTTON_INVALID)
			{
				setjoybuttonstate(id, retroarch_offset + 2,
					SDL_JoystickGetButton(did->joystick, did->mapping.quit_button) & 1);
			}
			if (did->mapping.reset_button != SDL_CONTROLLER_BUTTON_INVALID)
			{
				setjoybuttonstate(id, retroarch_offset + 3,
					SDL_JoystickGetButton(did->joystick, did->mapping.reset_button) & 1);
			}
			if (did->mapping.vkbd_button != SDL_CONTROLLER_BUTTON_INVALID)
			{
				setjoybuttonstate(id, retroarch_offset + 4,
					SDL_JoystickGetButton(did->joystick, did->mapping.vkbd_button) & 1);
			}
		}

		// Check all Joystick buttons, including axes acting as buttons
		for (int did_button = 0; did_button < did->buttons; did_button++)
		{
			if (did->mapping.button[did_button] != SDL_CONTROLLER_BUTTON_INVALID)
			{
				const int did_state = SDL_JoystickGetButton(did->joystick, did->mapping.button[did_button]) & 1;
				if (did->buttonaxisparent[did_button] >= 0)
				{
					int bstate;
					const int axis = did->buttonaxisparent[did_button];
					const int dir = did->buttonaxisparentdir[did_button];

					const int data = SDL_JoystickGetAxis(did->joystick, axis);
					if (dir)
						bstate = data > joystick_dead_zone ? 1 : 0;
					else
						bstate = data < -joystick_dead_zone ? 1 : 0;

					if (axisold[id][did_button] != bstate) {
						setjoybuttonstate(id, did_button, bstate);
						axisold[id][did_button] = bstate;
					}
				}
				else
				{
					setjoybuttonstate(id, did_button + held_offset, did_state);
				}
			}
		}
	}
}

void read_joystick_axis(const int id, const int axis, int value)
{
	const didata* did = &di_joystick[id];

	if (isfocus() || currprefs.inactive_input & 4)
	{
		// Check for any Axis movement
		for (auto did_axis = 0; did_axis < did->axles; did_axis++)
		{
			if (did->mapping.axis[did_axis] != SDL_CONTROLLER_AXIS_INVALID)
			{
				int data = SDL_JoystickGetAxis(did->joystick, did->mapping.axis[did_axis]);

				// If analog mouse mapping is used, the Left stick acts as a mouse
				if (did_axis <= SDL_CONTROLLER_AXIS_LEFTY && did->mousemap > 0)
				{
					if (data > joystick_dead_zone || data < -joystick_dead_zone)
						setmousestate(id, did_axis, data / 1000, 0);
				}
				else
				{
					if (invert_axis(did_axis, did))
					{
						data = data * -1;
					}
					if (axisold[id][did_axis] != data) {
						setjoystickstate(id, did_axis, data, analog_upper_bound);
						axisold[id][did_axis] = data;
					}
				}
			}
		}
	}
}

void read_joystick_hat(const int id, int hat, const int value)
{
	if (isfocus() || currprefs.inactive_input & 4)
	{
		for (int button = SDL_CONTROLLER_BUTTON_DPAD_UP; button <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; button++)
		{
			int state = 0;
			if (button == SDL_CONTROLLER_BUTTON_DPAD_UP)
				state = value & SDL_HAT_UP;
			else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
				state = value & SDL_HAT_DOWN;
			else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
				state = value & SDL_HAT_LEFT;
			else if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
				state = value & SDL_HAT_RIGHT;

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
int input_get_default_joystick(struct uae_input_device* uid, int i, int port, int af, int mode, bool gp, bool joymouseswap)
{
	struct didata* did = NULL;
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
	setid(uid, i, ID_AXIS_OFFSET + SDL_CONTROLLER_AXIS_LEFTX, 0, port, h, gp);
	setid(uid, i, ID_AXIS_OFFSET + SDL_CONTROLLER_AXIS_LEFTY, 0, port, v, gp);

	// Sync mouse map option to did struct, so we can use it while reading
	did->mousemap = currprefs.jports[port].mousemap;

	if (port >= 2) {
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_A, 0, port, port == 3 ? INPUTEVENT_PAR_JOY2_FIRE_BUTTON : INPUTEVENT_PAR_JOY1_FIRE_BUTTON, af, gp);
	}
	else {
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_A, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_B))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_B, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
		if (mode != JSEM_MODE_JOYSTICK) {
			if (isrealbutton(did, SDL_CONTROLLER_BUTTON_X))
				setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_X, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);
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
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_A, port, port ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED, af, gp);
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_B)) {
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_B, 0, port, port ? INPUTEVENT_JOY2_CD32_BLUE : INPUTEVENT_JOY1_CD32_BLUE, gp);
		}
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_X))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_X, 0, port, port ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN, gp);
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_Y))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_Y, 0, port, port ? INPUTEVENT_JOY2_CD32_YELLOW : INPUTEVENT_JOY1_CD32_YELLOW, gp);
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0, port, port ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD, gp);
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0, port, port ? INPUTEVENT_JOY2_CD32_FFW : INPUTEVENT_JOY1_CD32_FFW, gp);
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_START))
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_START, 0, port, port ? INPUTEVENT_JOY2_CD32_PLAY : INPUTEVENT_JOY1_CD32_PLAY, gp);
	}

	// Map D-Pad buttons if they exist. If Mouse Mode is set, they function as mouse movements, otherwise as digital Joystick directions
	if (mode == JSEM_MODE_MOUSE)
	{
		// Map D-Pad buttons to mouse movement
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_UP, 0, port, port ? INPUTEVENT_MOUSE2_UP : INPUTEVENT_MOUSE1_UP, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_DOWN, 0, port, port ? INPUTEVENT_MOUSE2_DOWN : INPUTEVENT_MOUSE1_DOWN, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_LEFT, 0, port, port ? INPUTEVENT_MOUSE2_LEFT : INPUTEVENT_MOUSE1_LEFT, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 0, port, port ? INPUTEVENT_MOUSE2_RIGHT : INPUTEVENT_MOUSE1_RIGHT, gp);
	}
	else
	{
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_UP, 0, port, port ? INPUTEVENT_JOY2_UP : INPUTEVENT_JOY1_UP, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_DOWN, 0, port, port ? INPUTEVENT_JOY2_DOWN : INPUTEVENT_JOY1_DOWN, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_LEFT, 0, port, port ? INPUTEVENT_JOY2_LEFT : INPUTEVENT_JOY1_LEFT, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 0, port, port ? INPUTEVENT_JOY2_RIGHT : INPUTEVENT_JOY1_RIGHT, gp);
	}

	// If mouse map is enabled (we're emulating a mouse on Port 0 from this controller's Analog stick)
	// we need to enable 2 mouse buttons for Port 0 as well. These are currently LShoulder / RShoulder respectively
	if (currprefs.jports[port].mousemap > 0)
	{
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0, port, INPUTEVENT_JOY1_FIRE_BUTTON, gp);
		setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0, port, INPUTEVENT_JOY1_2ND_BUTTON, gp);
	}

	// Configure a few extra default mappings, for convenience
	std::array<int, SDL_CONTROLLER_BUTTON_MAX> button_map[2]{};
	button_map[0] = currprefs.jports[port].amiberry_custom_none;
	button_map[1] = currprefs.jports[port].amiberry_custom_hotkey;

	std::array<int, SDL_CONTROLLER_AXIS_MAX> axis_map[2]{};
	axis_map[0] = currprefs.jports[port].amiberry_custom_axis_none;
	axis_map[1] = currprefs.jports[port].amiberry_custom_axis_hotkey;

	// Only applies for normal joysticks, as CD32 pads will use these buttons already
	if (mode < JSEM_MODE_JOYSTICK_CD32)
	{
		// Key P: commonly used for Pause in many games => START
		if (isrealbutton(did, SDL_CONTROLLER_BUTTON_START) && !button_map[0][SDL_CONTROLLER_BUTTON_START])
			setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_START, 0, port, INPUTEVENT_KEY_P, gp);

		// Mouse map uses these already, so we skip them if that's enabled
		if (currprefs.jports[port].mousemap == 0)
		{
			// Space bar used on many games as a 2nd fire button => LShoulder
			if (isrealbutton(did, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) && !button_map[0][SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
				setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 0, port, INPUTEVENT_KEY_SPACE, gp);

			// Return => RShoulder
			if (isrealbutton(did, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) && !button_map[0][SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
				setid(uid, i, ID_BUTTON_OFFSET + SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 0, port, INPUTEVENT_KEY_RETURN, gp);
		}
	}

	// Set the events for any Custom Controls mapping
	for (auto n = 0; n < 2; ++n)
	{
		const auto function_offset = n * REMAP_BUTTONS;
		for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
		{
			if (button_map[n][button] && isrealbutton(did, button))
				setid(uid, i, ID_BUTTON_OFFSET + button + function_offset, 0, port, button_map[n][button], af, gp);
		}

		// Axis mapping, skip the first 2 (Horizontal / Vertical on Left stick)
		// as they are already handled above
		for (int axis = 2; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
		{
			if (axis_map[n][axis])
				setid(uid, i, ID_AXIS_OFFSET + axis + function_offset, 0, port, axis_map[n][axis], af, gp);
		}
	}

	// We put any RetroArch special mappings above the controller's max value (Max Buttons + Max Axes)
	// to avoid clashing with any custom mappings above, as well as the axis-as-button mappings.
	// Note that newer versions of SDL2 might have a higher value in SDL_CONTROLLER_BUTTON_MAX / SDL_CONTROLLER_AXIS_MAX
	// This is already the case between 2.0.10 and 2.0.18 for example
	int retroarch_offset = SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_MAX * 2;
	if (currprefs.use_retroarch_menu)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 1, 0, port, INPUTEVENT_SPC_ENTERGUI, gp);
	
	if (currprefs.use_retroarch_quit)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 2, 0, port, INPUTEVENT_SPC_QUIT, gp);
	
	if (currprefs.use_retroarch_reset)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 3, 0, port, INPUTEVENT_SPC_SOFTRESET, gp);

	if (currprefs.use_retroarch_vkbd)
		setid(uid, i, ID_BUTTON_OFFSET + retroarch_offset + 4, 0, port, INPUTEVENT_SPC_TOGGLE_VIRTUAL_KEYBOARD, gp);

	if (i >= 0 && i < num_joystick)
		return 1;
	
	return 0;
}

int input_get_default_joystick_analog(struct uae_input_device* uid, int i, int port, int af, bool gp, bool joymouseswap)
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
