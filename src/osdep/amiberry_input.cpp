#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include "fsdb.h"

static struct host_input_button default_controller_map;
struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];

static struct host_keyboard_button default_keyboard_map;
struct host_keyboard_button host_keyboard_buttons[4];

const int remap_buttons = 16;
#define REMAP_BUTTONS        16
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

static char joystick_name[MAX_INPUT_DEVICES][80];
static SDL_GameController* controller_table[MAX_INPUT_DEVICES];

static int num_mouse = 1, num_keyboard = 1;
static int num_joystick = 0;
int num_keys_as_joys = 0;

static void fill_default_controller()
{
	default_controller_map.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.quit_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.reset_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.menu_button = SDL_CONTROLLER_BUTTON_GUIDE;

	default_controller_map.number_of_hats = 1;

	default_controller_map.north_button = SDL_CONTROLLER_BUTTON_Y; // yellow
	default_controller_map.east_button = SDL_CONTROLLER_BUTTON_B; // blue
	default_controller_map.south_button = SDL_CONTROLLER_BUTTON_A; // red
	default_controller_map.west_button = SDL_CONTROLLER_BUTTON_X; // green

	default_controller_map.dpad_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
	default_controller_map.dpad_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
	default_controller_map.dpad_up = SDL_CONTROLLER_BUTTON_DPAD_UP;
	default_controller_map.dpad_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN;

	default_controller_map.select_button = SDL_CONTROLLER_BUTTON_BACK; //
	default_controller_map.start_button = SDL_CONTROLLER_BUTTON_START; // play
	default_controller_map.left_shoulder = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; // rwd
	default_controller_map.right_shoulder = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; // fwd

	default_controller_map.left_trigger = SDL_CONTROLLER_AXIS_TRIGGERLEFT; // alt. fire
	default_controller_map.right_trigger = SDL_CONTROLLER_AXIS_TRIGGERRIGHT; // esc?

	default_controller_map.lstick_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.lstick_axis_y = SDL_CONTROLLER_AXIS_LEFTY;
	default_controller_map.lstick_axis_x = SDL_CONTROLLER_AXIS_LEFTX;
	default_controller_map.lstick_axis_y_invert = false;
	default_controller_map.lstick_axis_x_invert = false;

	default_controller_map.rstick_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.rstick_axis_y = SDL_CONTROLLER_AXIS_RIGHTY;
	default_controller_map.rstick_axis_x = SDL_CONTROLLER_AXIS_RIGHTX;
	default_controller_map.rstick_axis_y_invert = false;
	default_controller_map.rstick_axis_x_invert = false;

	default_controller_map.is_retroarch = false;
}

static void fill_blank_controller()
{
	default_controller_map.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.quit_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.reset_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.menu_button = SDL_CONTROLLER_BUTTON_INVALID;

	default_controller_map.number_of_hats = SDL_CONTROLLER_BUTTON_INVALID;

	default_controller_map.north_button = SDL_CONTROLLER_BUTTON_INVALID; // yellow
	default_controller_map.east_button = SDL_CONTROLLER_BUTTON_INVALID; // blue
	default_controller_map.south_button = SDL_CONTROLLER_BUTTON_INVALID; // red
	default_controller_map.west_button = SDL_CONTROLLER_BUTTON_INVALID; // green

	default_controller_map.dpad_left = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.dpad_right = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.dpad_up = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.dpad_down = SDL_CONTROLLER_BUTTON_INVALID;

	default_controller_map.select_button = SDL_CONTROLLER_BUTTON_INVALID; //                 
	default_controller_map.start_button = SDL_CONTROLLER_BUTTON_INVALID; // play
	default_controller_map.left_shoulder = SDL_CONTROLLER_BUTTON_INVALID; // rwd
	default_controller_map.right_shoulder = SDL_CONTROLLER_BUTTON_INVALID; // fwd

	default_controller_map.left_shoulder = SDL_CONTROLLER_BUTTON_INVALID; // rwd
	default_controller_map.right_shoulder = SDL_CONTROLLER_BUTTON_INVALID; // fwd
	default_controller_map.left_trigger = SDL_CONTROLLER_BUTTON_INVALID; // alt. fire
	default_controller_map.right_trigger = SDL_CONTROLLER_BUTTON_INVALID; // esc?

	default_controller_map.lstick_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.lstick_axis_y = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.lstick_axis_x = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.lstick_axis_y_invert = false;
	default_controller_map.lstick_axis_x_invert = false;

	default_controller_map.rstick_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.rstick_axis_y = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.rstick_axis_x = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.rstick_axis_y_invert = false;
	default_controller_map.rstick_axis_x_invert = false;

	default_controller_map.is_retroarch = false;
}

static void fill_default_keyboard()
{
	// test using iPac layout 
	default_keyboard_map.north_button = SDL_SCANCODE_LSHIFT;
	default_keyboard_map.east_button = SDL_SCANCODE_LCTRL;
	default_keyboard_map.south_button = SDL_SCANCODE_LALT;
	default_keyboard_map.west_button = SDL_SCANCODE_SPACE;
	default_keyboard_map.dpad_left = SDL_SCANCODE_LEFT;
	default_keyboard_map.dpad_right = SDL_SCANCODE_RIGHT;
	default_keyboard_map.dpad_up = SDL_SCANCODE_UP;
	default_keyboard_map.dpad_down = SDL_SCANCODE_DOWN;
	default_keyboard_map.left_shoulder = SDL_SCANCODE_Z;
	default_keyboard_map.right_shoulder = SDL_SCANCODE_X;
	default_keyboard_map.select_button = SDL_SCANCODE_1;
	default_keyboard_map.start_button = SDL_SCANCODE_2;
	default_keyboard_map.lstick_button = SDL_SCANCODE_F1;
	default_keyboard_map.rstick_button = SDL_SCANCODE_F2;

	default_keyboard_map.hotkey_button = -1;
	default_keyboard_map.quit_button = -1;
	default_keyboard_map.menu_button = -1;

	default_keyboard_map.is_retroarch = false;
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

const int remap_key_map_list[] = {
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

const int remap_key_map_list_size = sizeof remap_key_map_list / sizeof remap_key_map_list[0];

static int init_mouse()
{
	return 1;
}

static void close_mouse()
{
}

static int acquire_mouse(const int num, int flags)
{
	if (num >= 0 && num < num_mouse)
		return 1;

	return 0;
}

static void unacquire_mouse(int num)
{
}

static int get_mouse_num()
{
	return num_mouse;
}

static const TCHAR* get_mouse_friendlyname(const int mouse)
{
	if (num_mouse > 0 && mouse == 0)
		return "Mouse";

	return "";
}

static const TCHAR* get_mouse_uniquename(const int mouse)
{
	if (num_mouse > 0 && mouse == 0)
		return "MOUSE0";

	return "";
}

static int get_mouse_widget_num(const int mouse)
{
	if (num_mouse > 0 && mouse == 0)
		return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS;

	return 0;
}

static int get_mouse_widget_first(const int mouse, const int type)
{
	if (num_mouse > 0 && mouse == 0)
	{
		switch (type)
		{
		case IDEV_WIDGET_BUTTON:
			return FIRST_MOUSE_BUTTON;
		case IDEV_WIDGET_AXIS:
			return FIRST_MOUSE_AXIS;
		case IDEV_WIDGET_BUTTONAXIS:
			return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS;
		default:
			return -1;
		}
	}
	return -1;
}

static int get_mouse_widget_type(const int mouse, const int num, TCHAR* name, uae_u32* code)
{
	if (num_mouse > 0 && mouse == 0)
	{
		if (num >= MAX_MOUSE_AXES && num < MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS)
		{
			if (name)
				sprintf(name, "Button %d", num + 1 - MAX_MOUSE_AXES);
			return IDEV_WIDGET_BUTTON;
		}
		if (num < MAX_MOUSE_AXES)
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
	}
	return IDEV_WIDGET_NONE;
}

static void read_mouse()
{
	if (currprefs.input_tablet > TABLET_OFF)
	{
		// Mousehack active
		int x, y;
		SDL_GetMouseState(&x, &y);
		setmousestate(0, 0, x, 1);
		setmousestate(0, 1, y, 1);
	}
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
}

int input_get_default_mouse(struct uae_input_device* uid, const int i, const int port, const int af, const bool gp, bool wheel, bool joymouseswap)
{
	if (currprefs.jports[port].id >= JSEM_MICE && currprefs.jports[port].id < JSEM_END)
	{
		setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ, gp);
		setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT, gp);
		setid(uid, i, ID_AXIS_OFFSET + 2, 0, port, port ? 0 : INPUTEVENT_MOUSE1_WHEEL, gp);
		setid(uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
		setid(uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
		setid(uid, i, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);
	}
	else
		input_get_default_joystick(uid, i, port, af, JSEM_MODE_MOUSE, port, joymouseswap);

	if (i == 0)
		return 1;
	return 0;
}

static int get_kb_num()
{
	return num_keyboard;
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
	return 1;
}

static void close_kb()
{
}

static int acquire_kb(int num, int flags)
{
	return 1;
}

static void unacquire_kb(int num)
{
}

static void read_kb()
{
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
	// Add the Keyboard Arrow keys as Joystick 0, or retroarch keyboards (1-4)
	return num_joystick + num_keys_as_joys;
}

static int init_joystick()
{
	// set up variables / paths etc.
	char retroarch_file[MAX_DPATH];
	get_retroarch_file(retroarch_file, MAX_DPATH);

	if (my_existsfile(retroarch_file))
	{
		// Add as many keyboards as joysticks that are setup
		// on arcade machines, you could have a 4 player ipac using all keyboard buttons
		// so you want to have at least 4 keyboards to choose from!
		// once one config is missing, simply stop adding them!
		auto valid = true;
		for (auto kb = 0; kb < 4 && valid; ++kb)
		{
			valid = init_kb_from_retroarch(kb, retroarch_file);
		}
	}
	else
	{
		fill_default_keyboard();
		host_keyboard_buttons[0] = default_keyboard_map;
		num_keys_as_joys = 1;
	}

	// cap the number of joysticks etc
	num_joystick = SDL_NumJoysticks();
	if (num_joystick > MAX_INPUT_DEVICES)
		num_joystick = MAX_INPUT_DEVICES;

	// set up variables / paths etc.
	char tmp[MAX_DPATH];
	get_controllers_path(tmp, MAX_DPATH);

	// do the loop
	for (auto cpt = 0; cpt < num_joystick; cpt++)
	{
		if (SDL_IsGameController(cpt))
		{
			controller_table[cpt] = SDL_GameControllerOpen(cpt);
			auto* joy = SDL_GameControllerGetJoystick(controller_table[cpt]);

			// Some controllers (e.g. PS4) report a second instance with only axes and no buttons.
			// We ignore these and move on.
			if (SDL_JoystickNumButtons(joy) < 1)
			{
				SDL_GameControllerClose(controller_table[cpt]);
				controller_table[cpt] = nullptr;			
				continue;
			}
			if (SDL_JoystickName(joy))
				strncpy(joystick_name[cpt], SDL_JoystickName(joy), sizeof joystick_name[cpt] - 1);
			write_log("Controller Detection for Device: %s\n", joystick_name[cpt]);
			
			//this now uses controllers path (in tmp)
			char control_config[255];
			strcpy(control_config, tmp);
			auto sanitized_name = sanitize_retroarch_name(joystick_name[cpt]);
			strcat(control_config, sanitized_name.c_str());
			strcat(control_config, ".cfg");

			if (my_existsfile(control_config))
			{
				// retroarch controller cfg file found, we use that for mapping
				fill_blank_controller();
				host_input_buttons[cpt] = default_controller_map;
				map_from_retroarch(cpt, control_config);
			}
			else
			{
				// no retroarch controller cfg file found, use the default mapping
				fill_default_controller();
				host_input_buttons[cpt] = default_controller_map;
			}
		}
	}
	return 1;
}

void import_joysticks()
{
	init_joystick();
}

static void close_joystick()
{
	for (auto cpt = 0; cpt < num_joystick; cpt++)
	{
		if (controller_table[cpt] != nullptr)
			SDL_GameControllerClose(controller_table[cpt]);
	}
	num_joystick = 0;
}

static int acquire_joystick(const int num, int flags)
{
	if (num >= 0 && num < num_joystick)
		return 1;
	return 0;
}

static void unacquire_joystick(int num)
{
}

static const TCHAR* get_joystick_friendlyname(const int joy)
{
	const char fmt[] = "RetroArch Keyboard as Joystick [#%d]";
	char tmp1[255];
	for (auto n = 0; n < num_keys_as_joys; ++n)
	{
		if (joy == n)
		{
			if (host_keyboard_buttons[n].is_retroarch)
			{
				snprintf(tmp1, sizeof tmp1, fmt, n + 1);
				auto* const result = my_strdup(tmp1);
				tmp1[0]= '\0';
				return result;
			}
			return "Keyboard as Joystick [Default]";
		}
	}
	return joystick_name[joy - num_keys_as_joys];
}

static const TCHAR* get_joystick_uniquename(const int joy)
{
	if (joy == 0)
		return "JOY0";
	if (joy == 1)
		return "JOY1";
	if (joy == 2)
		return "JOY2";
	if (joy == 3)
		return "JOY3";
	if (joy == 4)
		return "JOY4";
	if (joy == 5)
		return "JOY5";
	if (joy == 6)
		return "JOY6";

	return "JOY7";
}

static int get_joystick_widget_num(const int joy_id)
{
	if (joy_id >= 0 && joy_id < num_joystick)
	{
		auto* joy = SDL_GameControllerGetJoystick(controller_table[joy_id]);
		return SDL_JoystickNumAxes(joy) + SDL_JoystickNumButtons(joy);
	}
	return 0;
}

static int get_joystick_widget_first(const int joy, const int type)
{
	if (joy >= 0 && joy < num_joystick)
	{
		switch (type)
		{
		case IDEV_WIDGET_BUTTON:
			return FIRST_JOY_BUTTON;
		case IDEV_WIDGET_AXIS:
			return FIRST_JOY_AXIS;
		case IDEV_WIDGET_BUTTONAXIS:
			return MAX_JOY_AXES + MAX_JOY_BUTTONS;
		default:
			return -1;
		}
	}
	return -1;
}

static int get_joystick_widget_type(const int joy, const int num, TCHAR* name, uae_u32* code)
{
	if (joy >= 0 && joy < num_joystick)
	{
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
	}
	return IDEV_WIDGET_NONE;
}

static int get_joystick_flags(int num)
{
	return 0;
}

static void read_joystick()
{
	for (auto joy_id = 0; joy_id < MAX_JPORTS; joy_id++)
	{
		// First handle retroarch (or default) keyboard as Joystick...
		if (currprefs.jports[joy_id].id >= JSEM_JOYS && currprefs.jports[joy_id].id < JSEM_JOYS + num_keys_as_joys)
		{
			const auto host_key_id = currprefs.jports[joy_id].id - JSEM_JOYS;
			const auto* key_state = SDL_GetKeyboardState(nullptr);
			const auto kb = host_key_id;

			// cd32 red, blue, green, yellow
			setjoybuttonstate(kb, 0, key_state[host_keyboard_buttons[kb].south_button] & 1); // b
			setjoybuttonstate(kb, 1, key_state[host_keyboard_buttons[kb].east_button] & 1); // a
			setjoybuttonstate(kb, 2, key_state[host_keyboard_buttons[kb].north_button] & 1); //y 
			setjoybuttonstate(kb, 3, key_state[host_keyboard_buttons[kb].west_button] & 1); // x

			setjoybuttonstate(kb, 4, key_state[host_keyboard_buttons[kb].left_shoulder] & 1); // z 
			setjoybuttonstate(kb, 5, key_state[host_keyboard_buttons[kb].right_shoulder] & 1); // x
			setjoybuttonstate(kb, 6, key_state[host_keyboard_buttons[kb].start_button] & 1); //num1
			// up down left right     
			setjoybuttonstate(kb, 7, key_state[host_keyboard_buttons[kb].dpad_up] & 1);
			setjoybuttonstate(kb, 8, key_state[host_keyboard_buttons[kb].dpad_down] & 1);
			setjoybuttonstate(kb, 9, key_state[host_keyboard_buttons[kb].dpad_left] & 1);
			setjoybuttonstate(kb, 10, key_state[host_keyboard_buttons[kb].dpad_right] & 1);

			// stick left/right     
			setjoybuttonstate(kb, 11, key_state[host_keyboard_buttons[kb].lstick_button] & 1);
			setjoybuttonstate(kb, 12, key_state[host_keyboard_buttons[kb].rstick_button] & 1);
			setjoybuttonstate(kb, 13, key_state[host_keyboard_buttons[kb].select_button] & 1); // num2

			// hotkey?
			if (host_keyboard_buttons[kb].hotkey_button != -1 && key_state[host_keyboard_buttons[kb].hotkey_button] & 1)
			{
				//held_offset = REMAP_BUTTONS;
				// menu button
				if (host_keyboard_buttons[kb].menu_button != -1)
					setjoybuttonstate(kb, 14, key_state[host_keyboard_buttons[kb].menu_button] & 1);
				// quit button
				if (host_keyboard_buttons[kb].quit_button != -1)
					setjoybuttonstate(kb, 15, key_state[host_keyboard_buttons[kb].quit_button] & 1);
				// reset button
				//setjoybuttonstate(kb, 30, keystate[host_keyboard_buttons[kb].reset_button] & 1) ;
			}
		}

		// this is what we actually use for joysticks
		else if (jsem_isjoy(joy_id, &currprefs) != -1)
		{
			const auto host_joy_id = currprefs.jports[joy_id].id - JSEM_JOYS - num_keys_as_joys;
			auto* joy = SDL_GameControllerGetJoystick(controller_table[host_joy_id]);
			static auto current_controller_map = host_input_buttons[host_joy_id];

			if (current_controller_map.hotkey_button != -1)
			{
				if (current_controller_map.east_button == current_controller_map.hotkey_button)
					current_controller_map.east_button = -1;
				if (current_controller_map.south_button == current_controller_map.hotkey_button)
					current_controller_map.south_button = -1;
				if (current_controller_map.north_button == current_controller_map.hotkey_button)
					current_controller_map.north_button = -1;
				if (current_controller_map.west_button == current_controller_map.hotkey_button)
					current_controller_map.west_button = -1;
				if (current_controller_map.dpad_left == current_controller_map.hotkey_button)
					current_controller_map.dpad_left = -1;
				if (current_controller_map.dpad_right == current_controller_map.hotkey_button)
					current_controller_map.dpad_right = -1;
				if (current_controller_map.dpad_up == current_controller_map.hotkey_button)
					current_controller_map.dpad_up = -1;
				if (current_controller_map.dpad_down == current_controller_map.hotkey_button)
					current_controller_map.dpad_down = -1;
				if (current_controller_map.select_button == current_controller_map.hotkey_button)
					current_controller_map.select_button = -1;
				if (current_controller_map.start_button == current_controller_map.hotkey_button)
					current_controller_map.start_button = -1;
				if (current_controller_map.left_trigger == current_controller_map.hotkey_button)
					current_controller_map.left_trigger = -1;
				if (current_controller_map.right_trigger == current_controller_map.hotkey_button)
					current_controller_map.right_trigger = -1;
				if (current_controller_map.lstick_button == current_controller_map.hotkey_button)
					current_controller_map.lstick_button = -1;
				if (current_controller_map.rstick_button == current_controller_map.hotkey_button)
					current_controller_map.rstick_button = -1;
				if (current_controller_map.left_shoulder == current_controller_map.hotkey_button)
					current_controller_map.left_shoulder = -1;
				if (current_controller_map.right_shoulder == current_controller_map.hotkey_button)
					current_controller_map.right_shoulder = -1;
			}

			auto held_offset = 0;

			// detect standalone retroarch hotkeys
			if (current_controller_map.hotkey_button == -1)
			{
				if (current_controller_map.menu_button != -1)
				{
					if (current_controller_map.is_retroarch)
					{
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 14,
							SDL_JoystickGetButton(joy,	current_controller_map.menu_button) & 1);
					}
					else
					{
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 14,
							SDL_GameControllerGetButton(controller_table[host_joy_id],
								static_cast<SDL_GameControllerButton>(current_controller_map.menu_button)) & 1);
					}
				}
				if (current_controller_map.quit_button != -1)
				{
					if (current_controller_map.is_retroarch)
					{
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 15,
							SDL_JoystickGetButton(joy,	current_controller_map.quit_button) & 1);
					}
					else
					{
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 15,
							SDL_GameControllerGetButton(controller_table[host_joy_id],
								static_cast<SDL_GameControllerButton>(current_controller_map.quit_button)) & 1);
					}
				}
				if (current_controller_map.reset_button != -1)
				{
					if (current_controller_map.is_retroarch)
					{
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 30,
							SDL_JoystickGetButton(joy,	current_controller_map.reset_button) & 1);
					}
					else
					{
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 30,
							SDL_GameControllerGetButton(controller_table[host_joy_id],
								static_cast<SDL_GameControllerButton>(current_controller_map.reset_button)) & 1);
					}
				}
			}
			// temporary solution for retroarch buttons inc. HOTKEY
			else if (current_controller_map.is_retroarch)
			{
				if (SDL_JoystickGetButton(joy, current_controller_map.hotkey_button) & 1)
				{
					held_offset = REMAP_BUTTONS;
					setjoybuttonstate(host_joy_id + num_keys_as_joys, 14,
						SDL_JoystickGetButton(joy, current_controller_map.menu_button) & 1);
					setjoybuttonstate(host_joy_id + num_keys_as_joys, 15,
						SDL_JoystickGetButton(joy, current_controller_map.quit_button) & 1);
					setjoybuttonstate(host_joy_id + num_keys_as_joys, 30,
						SDL_JoystickGetButton(joy, current_controller_map.reset_button) & 1);
				}
			}
			else if (SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.hotkey_button)) & 1)
			{
				held_offset = REMAP_BUTTONS;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 14,
					SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.menu_button)) & 1);
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 15,
					SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.quit_button)) & 1);
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 30,
					SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.reset_button)) & 1);
			}
			else
			{
				held_offset = 0;
			}
			
			if (SDL_JoystickNumAxes(joy) > 0)
			{
				int val;

				// this should allow use to change the tolerance
				const auto lower_bound = -32767;
				const auto upper_bound = 32767;

				// left stick   
				if (!currprefs.input_analog_remap)
				{
					// handle the X axis  (left stick)
					if (current_controller_map.lstick_axis_x != -1)
					{
						if (current_controller_map.is_retroarch)
							val = SDL_JoystickGetAxis(joy, current_controller_map.lstick_axis_x);
						else
							val = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.lstick_axis_x));

						if (current_controller_map.lstick_axis_x_invert != 0)
							val = val * -1;

						setjoystickstate(host_joy_id + num_keys_as_joys, 0, val, upper_bound);
					}

					// handle the Y axis
					if (current_controller_map.lstick_axis_y != -1)
					{
						if (current_controller_map.is_retroarch)
							val = SDL_JoystickGetAxis(joy, current_controller_map.lstick_axis_y);
						else
							val = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.lstick_axis_y));
						if (current_controller_map.lstick_axis_y_invert != 0)
							val = val * -1;

						setjoystickstate(host_joy_id + num_keys_as_joys, 1, val, upper_bound);
					}
				}
				else
				{
					// alternative code for custom remapping the left stick  
					// handle the Y axis  (left stick)
					if (current_controller_map.lstick_axis_y != -1)
					{
						int y_state_lower, y_state_upper;
						if (current_controller_map.is_retroarch)
						{
							y_state_lower = SDL_JoystickGetAxis(joy, current_controller_map.lstick_axis_y) <= lower_bound
								? 1
								: 0;
							y_state_upper = SDL_JoystickGetAxis(joy, current_controller_map.lstick_axis_y) >= upper_bound
								? 1
								: 0;
						}
						else
						{
							y_state_lower = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.lstick_axis_y)) <=
							lower_bound
								? 1
								: 0;
							y_state_upper = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.lstick_axis_y)) >=
								upper_bound
								? 1
								: 0;
						}
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 7 + held_offset, y_state_lower);
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 8 + held_offset, y_state_upper);
					}
					// handle the X axis
					if (current_controller_map.lstick_axis_x != -1)
					{
						int x_state_lower, x_state_upper;
						if (current_controller_map.is_retroarch)
						{
							x_state_lower = SDL_JoystickGetAxis(joy, current_controller_map.lstick_axis_x) <= lower_bound
								? 1
								: 0;
							x_state_upper = SDL_JoystickGetAxis(joy, current_controller_map.lstick_axis_x) >= upper_bound
								? 1
								: 0;
						}
						else
						{
							x_state_lower = SDL_GameControllerGetAxis(controller_table[host_joy_id],
								static_cast<SDL_GameControllerAxis>(current_controller_map.lstick_axis_x)) <= lower_bound
								? 1
								: 0;
							x_state_upper = SDL_GameControllerGetAxis(controller_table[host_joy_id],
								static_cast<SDL_GameControllerAxis>(current_controller_map.lstick_axis_x)) >= upper_bound
								? 1
								: 0;
						}
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 9 + held_offset, x_state_lower);
						setjoybuttonstate(host_joy_id + num_keys_as_joys, 10 + held_offset, x_state_upper);
					}
				}

				// right stick
				if (current_controller_map.rstick_axis_x != -1)
				{
					if (current_controller_map.is_retroarch)
						val = SDL_JoystickGetAxis(joy, current_controller_map.rstick_axis_x);
					else
						val = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.rstick_axis_x));
					if (current_controller_map.rstick_axis_x_invert != 0)
						val = val * -1;

					setjoystickstate(host_joy_id + num_keys_as_joys, 2, val, upper_bound);
				}

				if (current_controller_map.rstick_axis_y != -1)
				{
					if (current_controller_map.is_retroarch)
						val = SDL_JoystickGetAxis(joy, current_controller_map.rstick_axis_y);
					else
						val = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.rstick_axis_y));
					if (current_controller_map.rstick_axis_y_invert != 0)
						val = val * -1;

					setjoystickstate(host_joy_id + num_keys_as_joys, 3, val, upper_bound);
				}
			}
			// cd32 red, blue, green, yellow
			// south
			if (current_controller_map.south_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.south_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.south_button)) & 1;				
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 0 + held_offset, state);
			}
				
			// east
			if (current_controller_map.east_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.east_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.east_button)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 1 + held_offset, state);
			}
				
			// west
			if (current_controller_map.west_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.west_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.west_button)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 2 + held_offset, state);
			}
				
			// north
			if (current_controller_map.north_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.north_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.north_button)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 3 + held_offset, state);
			}

			// cd32  rwd, ffw, start
			// left shoulder
			if (current_controller_map.left_shoulder != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.left_shoulder) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.left_shoulder)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 4 + held_offset, state);
			}
				
			// right shoulder
			if (current_controller_map.right_shoulder != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.right_shoulder) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.right_shoulder)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 5 + held_offset, state);
			}
			
			// start
			if (current_controller_map.start_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.start_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.start_button)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 6 + held_offset, state);
			}

			// up down left right
			int state_up, state_down, state_left, state_right;
			if (current_controller_map.is_retroarch)
			{
				const int hat = SDL_JoystickGetHat(joy, 0);
				state_up = current_controller_map.dpad_up + 1
					? SDL_JoystickGetButton(joy, current_controller_map.dpad_up) & 1
					: hat & SDL_HAT_UP;
				state_down = current_controller_map.dpad_down + 1
					? SDL_JoystickGetButton(joy, current_controller_map.dpad_down) & 1
					: hat & SDL_HAT_DOWN;
				state_left = current_controller_map.dpad_left + 1
					? SDL_JoystickGetButton(joy, current_controller_map.dpad_left) & 1
					: hat & SDL_HAT_LEFT;
				state_right = current_controller_map.dpad_right + 1
					? SDL_JoystickGetButton(joy, current_controller_map.dpad_right) & 1
					: hat & SDL_HAT_RIGHT;
			}
			else
			{
				state_up = current_controller_map.dpad_up + 1
					? SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.dpad_up)) & 1
					: 0;
				state_down = current_controller_map.dpad_down + 1
					? SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.dpad_down)) & 1
					: 0;
				state_left = current_controller_map.dpad_left + 1
					? SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.dpad_left)) & 1
					: 0;
				state_right = current_controller_map.dpad_right + 1
					? SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.dpad_right)) & 1
					: 0;
			}
			setjoybuttonstate(host_joy_id + num_keys_as_joys, 7 + held_offset, state_up);			
			setjoybuttonstate(host_joy_id + num_keys_as_joys, 8 + held_offset, state_down);
			setjoybuttonstate(host_joy_id + num_keys_as_joys, 9 + held_offset, state_left);
			setjoybuttonstate(host_joy_id + num_keys_as_joys, 10 + held_offset, state_right);

			// stick left/right/select
			// left stick
			if (current_controller_map.lstick_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.lstick_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.lstick_button)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 11 + held_offset, state);
			}
			// right stick
			if (current_controller_map.rstick_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.rstick_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.rstick_button)) & 1;
				setjoybuttonstate(host_joy_id + num_keys_as_joys, 12 + held_offset, state);
			}

			// select button
			if (current_controller_map.select_button != -1)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.select_button) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.select_button)) & 1;
				setjoybuttonstate(host_joy_id + 1, 13 + held_offset, state);
			}
		}
	}
}

struct inputdevice_functions inputdevicefunc_joystick = {
	init_joystick, close_joystick, acquire_joystick, unacquire_joystick,
	read_joystick, get_joystick_num, get_joystick_friendlyname, get_joystick_uniquename,
	get_joystick_widget_num, get_joystick_widget_type,
	get_joystick_widget_first,
	get_joystick_flags
};

int input_get_default_joystick(struct uae_input_device* uid, int num, int port, int af, int mode, bool gp, bool joymouseswap)
{
	int h, v;

	if (mode == JSEM_MODE_MOUSE_CDTV) {
		h = INPUTEVENT_MOUSE_CDTV_HORIZ;
		v = INPUTEVENT_MOUSE_CDTV_VERT;
	}
	else if (port >= 2) {
		h = port == 3 ? INPUTEVENT_PAR_JOY2_HORIZ : INPUTEVENT_PAR_JOY1_HORIZ;
		v = port == 3 ? INPUTEVENT_PAR_JOY2_VERT : INPUTEVENT_PAR_JOY1_VERT;
	}
	else {
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
	setid(uid, num, ID_AXIS_OFFSET + 0, 0, port, h, gp);
	setid(uid, num, ID_AXIS_OFFSET + 1, 0, port, v, gp);
	
	// ASSIGN ALL INPUT EVENT ACTIONS, EITHER CUSTOM OR DEFAULT
	//
	// set up a temporary control layout/ called 'thismap'
	struct joypad_map_layout thismap[4];

	// here, we will fill thismap with defaults, if a custom value is not set. 
	// this will do a lot of the 'logic' of the original code.

	thismap[0] = currprefs.jports[port].amiberry_custom_none;
	// grab the 'no selection' options for the current map

	// directions

	if (port < 2) // ports 0, 1 ... 
	{
		if (mode == JSEM_MODE_MOUSE)
		{
			thismap[0].dpad_up_action = thismap[0].dpad_up_action
				                            ? thismap[0].dpad_up_action
				                            : port
				                            ? INPUTEVENT_MOUSE2_UP
				                            : INPUTEVENT_MOUSE1_UP;
			thismap[0].dpad_down_action = thismap[0].dpad_down_action
				                              ? thismap[0].dpad_down_action
				                              : port
				                              ? INPUTEVENT_MOUSE2_DOWN
				                              : INPUTEVENT_MOUSE1_DOWN;
			thismap[0].dpad_left_action = thismap[0].dpad_left_action
				                              ? thismap[0].dpad_left_action
				                              : port
				                              ? INPUTEVENT_MOUSE2_LEFT
				                              : INPUTEVENT_MOUSE1_LEFT;
			thismap[0].dpad_right_action = thismap[0].dpad_right_action
				                               ? thismap[0].dpad_right_action
				                               : port
				                               ? INPUTEVENT_MOUSE2_RIGHT
				                               : INPUTEVENT_MOUSE1_RIGHT;
		}
		else
		{
			thismap[0].dpad_up_action = thismap[0].dpad_up_action
				                            ? thismap[0].dpad_up_action
				                            : port
				                            ? INPUTEVENT_JOY2_UP
				                            : INPUTEVENT_JOY1_UP;
			thismap[0].dpad_down_action = thismap[0].dpad_down_action
				                              ? thismap[0].dpad_down_action
				                              : port
				                              ? INPUTEVENT_JOY2_DOWN
				                              : INPUTEVENT_JOY1_DOWN;
			thismap[0].dpad_left_action = thismap[0].dpad_left_action
				                              ? thismap[0].dpad_left_action
				                              : port
				                              ? INPUTEVENT_JOY2_LEFT
				                              : INPUTEVENT_JOY1_LEFT;
			thismap[0].dpad_right_action = thismap[0].dpad_right_action
				                               ? thismap[0].dpad_right_action
				                               : port
				                               ? INPUTEVENT_JOY2_RIGHT
				                               : INPUTEVENT_JOY1_RIGHT;
		}
		// standard fire buttons
		if (mode == JSEM_MODE_JOYSTICK_CD32) // CD32 joypad  
		{
			thismap[0].south_action = thismap[0].south_action
				                          ? thismap[0].south_action
				                          : port
				                          ? INPUTEVENT_JOY2_CD32_RED
				                          : INPUTEVENT_JOY1_CD32_RED;
			thismap[0].east_action = thismap[0].east_action
				                         ? thismap[0].east_action
				                         : port
				                         ? INPUTEVENT_JOY2_CD32_BLUE
				                         : INPUTEVENT_JOY1_CD32_BLUE;
			thismap[0].west_action = thismap[0].west_action
				                         ? thismap[0].west_action
				                         : port
				                         ? INPUTEVENT_JOY2_CD32_GREEN
				                         : INPUTEVENT_JOY1_CD32_GREEN;
			thismap[0].north_action = thismap[0].north_action
				                          ? thismap[0].north_action
				                          : port
				                          ? INPUTEVENT_JOY2_CD32_YELLOW
				                          : INPUTEVENT_JOY1_CD32_YELLOW;
			thismap[0].start_action = thismap[0].start_action
				                          ? thismap[0].start_action
				                          : port
				                          ? INPUTEVENT_JOY2_CD32_PLAY
				                          : INPUTEVENT_JOY1_CD32_PLAY;
		}
		else if (currprefs.jports[port].id >= JSEM_JOYS) // default, normal joystick  
		{
			thismap[0].south_action = thismap[0].south_action
				                          ? thismap[0].south_action
				                          : port
				                          ? INPUTEVENT_JOY2_FIRE_BUTTON
				                          : INPUTEVENT_JOY1_FIRE_BUTTON;
			thismap[0].east_action = thismap[0].east_action
				                         ? thismap[0].east_action
				                         : port
				                         ? INPUTEVENT_JOY2_2ND_BUTTON
				                         : INPUTEVENT_JOY1_2ND_BUTTON;
			thismap[0].west_action = thismap[0].west_action
				                         ? thismap[0].west_action
				                         : port
				                         ? INPUTEVENT_JOY2_UP
				                         : INPUTEVENT_JOY1_UP;
			thismap[0].north_action = thismap[0].north_action
				                          ? thismap[0].north_action
				                          : port
				                          ? INPUTEVENT_JOY2_3RD_BUTTON
				                          : INPUTEVENT_JOY1_3RD_BUTTON;

			thismap[0].start_action = thismap[0].start_action ? thismap[0].start_action : INPUTEVENT_KEY_P;
		}

		// shoulder buttons
		//if (CHECK_BIT(currprefs.jports[port].mousemap,1))
		if (mode == JSEM_MODE_MOUSE)
			// if we use right-analogue as mouse, then we will use shoulder buttons as LMB/RMB
			//if (1==0)
		{
			thismap[0].left_shoulder_action = thismap[0].left_shoulder_action
				                                  ? thismap[0].left_shoulder_action
				                                  : port
				                                  ? INPUTEVENT_JOY2_FIRE_BUTTON
				                                  : INPUTEVENT_JOY1_FIRE_BUTTON;
			thismap[0].right_shoulder_action = thismap[0].right_shoulder_action
				                                   ? thismap[0].right_shoulder_action
				                                   : port
				                                   ? INPUTEVENT_JOY2_2ND_BUTTON
				                                   : INPUTEVENT_JOY1_2ND_BUTTON;
		}

		else if (mode == JSEM_MODE_JOYSTICK_CD32) // CD32 joypad, use RWD/FWD
		{
			thismap[0].left_shoulder_action = thismap[0].left_shoulder_action
				                                  ? thismap[0].left_shoulder_action
				                                  : port
				                                  ? INPUTEVENT_JOY2_CD32_RWD
				                                  : INPUTEVENT_JOY1_CD32_RWD;

			thismap[0].right_shoulder_action = thismap[0].right_shoulder_action
				                                   ? thismap[0].right_shoulder_action
				                                   : port
				                                   ? INPUTEVENT_JOY2_CD32_FFW
				                                   : INPUTEVENT_JOY1_CD32_FFW;
		}

		else if (currprefs.jports[port].id >= JSEM_JOYS) // default, normal joystick
		{
			thismap[0].left_shoulder_action = thismap[0].left_shoulder_action
				                                  ? thismap[0].left_shoulder_action
				                                  : INPUTEVENT_KEY_SPACE;
			thismap[0].right_shoulder_action = thismap[0].right_shoulder_action
				                                   ? thismap[0].right_shoulder_action
				                                   : INPUTEVENT_KEY_RETURN;
		}
	}
	else // ports 2, 3 ... parallel ports 
	{
		thismap[0].dpad_up_action = thismap[0].dpad_up_action
			                            ? thismap[0].dpad_up_action
			                            : port - 2
			                            ? INPUTEVENT_PAR_JOY2_UP
			                            : INPUTEVENT_PAR_JOY1_UP;
		thismap[0].dpad_down_action = thismap[0].dpad_down_action
			                              ? thismap[0].dpad_down_action
			                              : port - 2
			                              ? INPUTEVENT_PAR_JOY2_DOWN
			                              : INPUTEVENT_PAR_JOY1_DOWN;
		thismap[0].dpad_left_action = thismap[0].dpad_left_action
			                              ? thismap[0].dpad_left_action
			                              : port - 2
			                              ? INPUTEVENT_PAR_JOY2_LEFT
			                              : INPUTEVENT_PAR_JOY1_LEFT;
		thismap[0].dpad_right_action = thismap[0].dpad_right_action
			                               ? thismap[0].dpad_right_action
			                               : port - 2
			                               ? INPUTEVENT_PAR_JOY2_RIGHT
			                               : INPUTEVENT_PAR_JOY1_RIGHT;

		thismap[0].south_action = thismap[0].south_action
			                          ? thismap[0].south_action
			                          : port - 2
			                          ? INPUTEVENT_PAR_JOY2_FIRE_BUTTON
			                          : INPUTEVENT_PAR_JOY1_FIRE_BUTTON;
		thismap[0].east_action = thismap[0].east_action
			                         ? thismap[0].east_action
			                         : port - 2
			                         ? INPUTEVENT_PAR_JOY2_2ND_BUTTON
			                         : INPUTEVENT_PAR_JOY1_2ND_BUTTON;

		thismap[0].start_action = thismap[0].start_action ? thismap[0].start_action : INPUTEVENT_KEY_P;
		thismap[0].left_shoulder_action = thismap[0].left_shoulder_action
			                                  ? thismap[0].left_shoulder_action
			                                  : INPUTEVENT_KEY_SPACE;
		thismap[0].right_shoulder_action = thismap[0].right_shoulder_action
			                                   ? thismap[0].right_shoulder_action
			                                   : INPUTEVENT_KEY_RETURN;
	}

	thismap[1] = currprefs.jports[port].amiberry_custom_hotkey;
	// grab the 'select button' options for the current map

	// currently disabled
	//	thismap[2] = currprefs.jports[port].amiberry_custom_left_trigger; // grab the 'left trigger'  options for the current map        
	//	thismap[3] = currprefs.jports[port].amiberry_custom_right_trigger; // grab the 'right trigger' options for the current map

	//  Now assign the actual buttons VALUES (TRUE/FALSE) to trigger the EVENTS
	auto function_offset = 0;

	for (auto n = 0; n < 2; ++n) /// temporarily limited to '2' only
	{
		function_offset = n * REMAP_BUTTONS;

		// s/e/w/n      
		setid(uid, num, ID_BUTTON_OFFSET + 0 + function_offset, 0, port, thismap[n].south_action, af, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 1 + function_offset, 0, port, thismap[n].east_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 2 + function_offset, 0, port, thismap[n].west_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 3 + function_offset, 0, port, thismap[n].north_action, gp);

		// left shoulder / right shoulder / start  
		setid(uid, num, ID_BUTTON_OFFSET + 4 + function_offset, 0, port, thismap[n].left_shoulder_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 5 + function_offset, 0, port, thismap[n].right_shoulder_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 6 + function_offset, 0, port, thismap[n].start_action, gp);

		// directions  
		setid(uid, num, ID_BUTTON_OFFSET + 7 + function_offset, 0, port, thismap[n].dpad_up_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 8 + function_offset, 0, port, thismap[n].dpad_down_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 9 + function_offset, 0, port, thismap[n].dpad_left_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 10 + function_offset, 0, port, thismap[n].dpad_right_action, gp);

		// stick buttons
		setid(uid, num, ID_BUTTON_OFFSET + 11 + function_offset, 0, port, thismap[n].lstick_select_action, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 12 + function_offset, 0, port, thismap[n].rstick_select_action, gp);

		setid(uid, num, ID_BUTTON_OFFSET + 13 + function_offset, 0, port, thismap[n].select_action, gp);
	}

	// if using retroarch options
	if (currprefs.use_retroarch_menu)
	{
		setid(uid, num, ID_BUTTON_OFFSET + 14, 0, port, INPUTEVENT_SPC_ENTERGUI, gp);
	}
	if (currprefs.use_retroarch_quit)
	{
		setid(uid, num, ID_BUTTON_OFFSET + 15, 0, port, INPUTEVENT_SPC_QUIT, gp);
	}
	if (currprefs.use_retroarch_reset)
	{
		setid(uid, num, ID_BUTTON_OFFSET + 30, 0, port, INPUTEVENT_SPC_SOFTRESET, gp);
	}

	if (num >= 0 && num < num_joystick)
	{
		return 1;
	}
	return 0;
}

int input_get_default_joystick_analog(struct uae_input_device* uid, int i, int port, int af, bool gp, bool joymouseswap)
{
	setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_HORIZ_POT : INPUTEVENT_JOY1_HORIZ_POT, gp);
	setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_VERT_POT : INPUTEVENT_JOY1_VERT_POT, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_LEFT : INPUTEVENT_JOY1_LEFT, af, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_RIGHT : INPUTEVENT_JOY1_RIGHT, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_UP : INPUTEVENT_JOY1_UP, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 3, 0, port, port ? INPUTEVENT_JOY2_DOWN : INPUTEVENT_JOY1_DOWN, gp);
	
	if (i == 0)
		return 1;
	return 0;
}

