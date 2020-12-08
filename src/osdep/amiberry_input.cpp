#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sysdeps.h"
#include "options.h"
#include "inputdevice.h"
#include "amiberry_input.h"

#include "fsdb.h"
#include "uae.h"

static struct host_input_button default_controller_map;
struct host_input_button host_input_buttons[MAX_INPUT_DEVICES];
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
static uae_s16 axisold[MAX_INPUT_DEVICES][256], buttonold[MAX_INPUT_DEVICES][256];
static SDL_GameController* controller_table[MAX_INPUT_DEVICES];

static int num_mouse = 1, num_keyboard = 1;
static int num_joystick = 0;

const auto analog_upper_bound = 32767;
const auto analog_lower_bound = -analog_upper_bound;

static void fill_default_controller()
{
	default_controller_map.hotkey_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.quit_button = SDL_CONTROLLER_BUTTON_INVALID;
	default_controller_map.reset_button = SDL_CONTROLLER_BUTTON_INVALID;

	default_controller_map.lstick_axis_y_invert = false;
	default_controller_map.lstick_axis_x_invert = false;
	default_controller_map.rstick_axis_y_invert = false;
	default_controller_map.rstick_axis_x_invert = false;

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

	default_controller_map.lstick_axis_y_invert = false;
	default_controller_map.lstick_axis_x_invert = false;
	default_controller_map.rstick_axis_y_invert = false;
	default_controller_map.rstick_axis_x_invert = false;

	default_controller_map.is_retroarch = false;

	for (auto& b : default_controller_map.button)
		b = SDL_CONTROLLER_BUTTON_INVALID;

	for (auto& axi : default_controller_map.axis)
		axi = SDL_CONTROLLER_AXIS_INVALID;
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
static int keyboard_german;

int is_touch_lightpen(void)
{
	return 0;
}

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

int input_get_default_lightpen(struct uae_input_device* uid, int i, int port, int af, bool gp, bool joymouseswap, int submode)
{
	struct didata* did = NULL;

	//if (!joymouseswap) {
	//	if (i >= num_mouse)
	//		return 0;
	//	did = &di_mouse[i];
	//}
	//else {
	//	if (i >= num_joystick)
	//		return 0;
	//	did = &di_joystick[i];
	//}
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
	return 1;
}

static void close_kb()
{
}

static void release_keys(void)
{
	//int i, j;

	//for (j = 0; j < MAX_INPUT_DEVICES; j++) {
	//	for (i = 0; i < MAX_KEYCODES; i++) {
	//		if (di_keycodes[j][i]) {
	//			di_keycodes[j][i] = 0;
	//			my_kbd_handler(j, i, 0, true);
	//		}
	//	}
	//}
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
	// cap the number of joysticks etc
	num_joystick = SDL_NumJoysticks();
	if (num_joystick > MAX_INPUT_DEVICES)
		num_joystick = MAX_INPUT_DEVICES;

	// set up variables / paths etc.
	char tmp[MAX_DPATH];
	get_controllers_path(tmp, MAX_DPATH);

	char cfg[MAX_DPATH];
	get_configuration_path(cfg, MAX_DPATH);
	strcat(cfg, "gamecontrollerdb.txt");
	SDL_GameControllerAddMappingsFromFile(cfg);
	
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

			if (host_input_buttons[cpt].hotkey_button != SDL_CONTROLLER_BUTTON_INVALID)
			{
				for (auto& k : host_input_buttons[cpt].button)
				{
					if (k == host_input_buttons[cpt].hotkey_button)
						k = SDL_CONTROLLER_BUTTON_INVALID;
				}
				for (auto& k : host_input_buttons[cpt].axis)
				{
					if (k == host_input_buttons[cpt].hotkey_button)
						k = SDL_CONTROLLER_AXIS_INVALID;
				}
			}
		}
		//TODO handle controllers with no mapping as well
		
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
	return joystick_name[joy];
}

static const TCHAR* get_joystick_uniquename(const int joy)
{
	TCHAR tmp[MAX_DPATH];
	_stprintf(tmp, _T("JOY%d"), joy);
	return my_strdup(tmp);
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

static int get_joystick_widget_type(const int joy, const int num, TCHAR* name, uae_u32* code)
{
	auto* joystick = SDL_GameControllerGetJoystick(controller_table[joy]);
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
	auto* joystick = SDL_GameControllerGetJoystick(controller_table[joy]);
	switch (type)
	{
	case IDEV_WIDGET_BUTTON:
		return SDL_JoystickNumAxes(joystick);
	case IDEV_WIDGET_AXIS:
		return 0;
	case IDEV_WIDGET_BUTTONAXIS:
		return SDL_JoystickNumAxes(joystick) + SDL_JoystickNumButtons(joystick);
	default:
		return -1;
	}
}

static int get_joystick_flags(int num)
{
	return 0;
}

static void read_joystick()
{
	for (auto joy_id = 0; joy_id < MAX_JPORTS; joy_id++)
	{
		if (jsem_isjoy(joy_id, &currprefs) != -1)
		{
			const auto host_joy_id = currprefs.jports[joy_id].id - JSEM_JOYS;
			auto* joy = SDL_GameControllerGetJoystick(controller_table[host_joy_id]);
			static auto current_controller_map = host_input_buttons[host_joy_id];
			auto held_offset = 0;

			// detect standalone retroarch hotkeys
			if (current_controller_map.hotkey_button == SDL_CONTROLLER_BUTTON_INVALID)
			{
				if (current_controller_map.button[SDL_CONTROLLER_BUTTON_GUIDE] != SDL_CONTROLLER_BUTTON_INVALID)
				{
					if (current_controller_map.is_retroarch)
					{
						setjoybuttonstate(host_joy_id, 14,
							SDL_JoystickGetButton(joy,	current_controller_map.button[SDL_CONTROLLER_BUTTON_GUIDE]) & 1);
					}
					else
					{
						setjoybuttonstate(host_joy_id, 14,
							SDL_GameControllerGetButton(controller_table[host_joy_id],
								static_cast<SDL_GameControllerButton>(current_controller_map.button[SDL_CONTROLLER_BUTTON_GUIDE])) & 1);
					}
				}
				if (current_controller_map.quit_button != SDL_CONTROLLER_BUTTON_INVALID)
				{
					if (current_controller_map.is_retroarch)
					{
						setjoybuttonstate(host_joy_id, 15,
							SDL_JoystickGetButton(joy,	current_controller_map.quit_button) & 1);
					}
					else
					{
						setjoybuttonstate(host_joy_id, 15,
							SDL_GameControllerGetButton(controller_table[host_joy_id],
								static_cast<SDL_GameControllerButton>(current_controller_map.quit_button)) & 1);
					}
				}
				if (current_controller_map.reset_button != SDL_CONTROLLER_BUTTON_INVALID)
				{
					if (current_controller_map.is_retroarch)
					{
						setjoybuttonstate(host_joy_id, 30,
							SDL_JoystickGetButton(joy,	current_controller_map.reset_button) & 1);
					}
					else
					{
						setjoybuttonstate(host_joy_id, 30,
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
					setjoybuttonstate(host_joy_id, 14,
						SDL_JoystickGetButton(joy, current_controller_map.button[SDL_CONTROLLER_BUTTON_GUIDE]) & 1);
					setjoybuttonstate(host_joy_id, 15,
						SDL_JoystickGetButton(joy, current_controller_map.quit_button) & 1);
					setjoybuttonstate(host_joy_id, 30,
						SDL_JoystickGetButton(joy, current_controller_map.reset_button) & 1);
				}
			}
			else if (SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.hotkey_button)) & 1)
			{
				held_offset = REMAP_BUTTONS;
				setjoybuttonstate(host_joy_id, 14,
					SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.button[SDL_CONTROLLER_BUTTON_GUIDE])) & 1);
				setjoybuttonstate(host_joy_id, 15,
					SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.quit_button)) & 1);
				setjoybuttonstate(host_joy_id, 30,
					SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.reset_button)) & 1);
			}
			else
			{
				held_offset = 0;
			}

			// Check for any Axis movement
			if (SDL_JoystickNumAxes(joy) > 0)
			{
				int val;
				for (auto axis = 0; axis <= SDL_CONTROLLER_AXIS_RIGHTY; axis++)
				{
					if (axis == 0 && currprefs.input_analog_remap && current_controller_map.axis[axis] != SDL_CONTROLLER_AXIS_INVALID)
					{
						int x_state_lower, x_state_upper;
						if (current_controller_map.is_retroarch)
						{
							x_state_lower = SDL_JoystickGetAxis(joy, current_controller_map.axis[axis]) <= analog_lower_bound
								? 1
								: 0;
							x_state_upper = SDL_JoystickGetAxis(joy, current_controller_map.axis[axis]) >= analog_upper_bound
								? 1
								: 0;
						}
						else
						{
							x_state_lower = SDL_GameControllerGetAxis(controller_table[host_joy_id],
								static_cast<SDL_GameControllerAxis>(current_controller_map.axis[axis])) <= analog_lower_bound
								? 1
								: 0;
							x_state_upper = SDL_GameControllerGetAxis(controller_table[host_joy_id],
								static_cast<SDL_GameControllerAxis>(current_controller_map.axis[axis])) >= analog_upper_bound
								? 1
								: 0;
						}
						setjoybuttonstate(host_joy_id, 9 + held_offset, x_state_lower);
						setjoybuttonstate(host_joy_id, 10 + held_offset, x_state_upper);
					}
					else if (axis == 1 && currprefs.input_analog_remap && current_controller_map.axis[axis] != SDL_CONTROLLER_AXIS_INVALID)
					{
						int y_state_lower, y_state_upper;
						if (current_controller_map.is_retroarch)
						{
							if (SDL_JoystickGetAxis(joy, current_controller_map.axis[axis]) <= analog_lower_bound) y_state_lower = 1;
							else y_state_lower = 0;
							y_state_upper = SDL_JoystickGetAxis(joy, current_controller_map.axis[axis]) >= analog_upper_bound
								? 1
								: 0;
						}
						else
						{
							y_state_lower = SDL_GameControllerGetAxis(controller_table[host_joy_id], 
								static_cast<SDL_GameControllerAxis>(current_controller_map.axis[axis])) <= analog_lower_bound
								? 1
								: 0;
							y_state_upper = SDL_GameControllerGetAxis(controller_table[host_joy_id], 
								static_cast<SDL_GameControllerAxis>(current_controller_map.axis[axis])) >= analog_upper_bound
								? 1
								: 0;
						}
						setjoybuttonstate(host_joy_id, 7 + held_offset, y_state_lower);
						setjoybuttonstate(host_joy_id, 8 + held_offset, y_state_upper);
					}
					else if (current_controller_map.axis[axis] != SDL_CONTROLLER_AXIS_INVALID)
					{
						if (current_controller_map.is_retroarch)
							val = SDL_JoystickGetAxis(joy, current_controller_map.axis[axis]);
						else
							val = SDL_GameControllerGetAxis(controller_table[host_joy_id], static_cast<SDL_GameControllerAxis>(current_controller_map.axis[axis]));

						setjoystickstate(host_joy_id, axis, val, analog_upper_bound);
					}
				}
			}
			
			// cd32 red, blue, green, yellow
			for (auto button = 0; button <= SDL_CONTROLLER_BUTTON_Y; button++)
			{
				if (current_controller_map.button[button] != SDL_CONTROLLER_BUTTON_INVALID)
				{
					int state;
					if (current_controller_map.is_retroarch)
						state = SDL_JoystickGetButton(joy, current_controller_map.button[button]) & 1;
					else
						state = SDL_GameControllerGetButton(controller_table[host_joy_id],
							static_cast<SDL_GameControllerButton>(current_controller_map.button[button])) & 1;
					setjoybuttonstate(host_joy_id, button + held_offset, state);
				}
			}

			// cd32  rwd, ffw, start
			for (int button = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; button <= SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; button++)
			{
				if (current_controller_map.button[button] != SDL_CONTROLLER_BUTTON_INVALID)
				{
					int state;
					if (current_controller_map.is_retroarch)
						state = SDL_JoystickGetButton(joy, current_controller_map.button[button]) & 1;
					else
						state = SDL_GameControllerGetButton(controller_table[host_joy_id],
							static_cast<SDL_GameControllerButton>(current_controller_map.button[button])) & 1;
					setjoybuttonstate(host_joy_id, button - 5 + held_offset, state);
				}
			}
			
			// start
			if (current_controller_map.button[SDL_CONTROLLER_BUTTON_START] != SDL_CONTROLLER_BUTTON_INVALID)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.button[SDL_CONTROLLER_BUTTON_START]) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.button[SDL_CONTROLLER_BUTTON_START])) & 1;
				setjoybuttonstate(host_joy_id, 6 + held_offset, state);
			}

			// up down left right
			for (int button = SDL_CONTROLLER_BUTTON_DPAD_UP; button <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; button++)
			{
				int state;
				if (current_controller_map.is_retroarch)
				{
					const int hat = SDL_JoystickGetHat(joy, 0);
					state = current_controller_map.button[button] + 1
						? SDL_JoystickGetButton(joy, current_controller_map.button[button]) & 1
						: button == 11
						? hat & SDL_HAT_UP
						: button == 12
						? hat & SDL_HAT_DOWN
						: button == 13
						? hat & SDL_HAT_LEFT
						: button == 14
						? hat & SDL_HAT_RIGHT
						: 0;
				}
				else
				{
					state = current_controller_map.button[button] + 1
						? SDL_GameControllerGetButton(controller_table[host_joy_id], static_cast<SDL_GameControllerButton>(current_controller_map.button[button])) & 1
						: 0;
				}
				setjoybuttonstate(host_joy_id, button - 4 + held_offset, state);
			}
			
			// stick left/right
			for (int button = SDL_CONTROLLER_BUTTON_LEFTSTICK; button <= SDL_CONTROLLER_BUTTON_RIGHTSTICK; button++)
			{
				if (current_controller_map.button[button] != -1)
				{
					int state;
					if (current_controller_map.is_retroarch)
						state = SDL_JoystickGetButton(joy, current_controller_map.button[button]) & 1;
					else
						state = SDL_GameControllerGetButton(controller_table[host_joy_id],
							static_cast<SDL_GameControllerButton>(current_controller_map.button[button])) & 1;
					setjoybuttonstate(host_joy_id, button + 4 + held_offset, state);
				}
			}

			// select button
			if (current_controller_map.button[SDL_CONTROLLER_BUTTON_BACK] != SDL_CONTROLLER_BUTTON_INVALID)
			{
				int state;
				if (current_controller_map.is_retroarch)
					state = SDL_JoystickGetButton(joy, current_controller_map.button[SDL_CONTROLLER_BUTTON_BACK]) & 1;
				else
					state = SDL_GameControllerGetButton(controller_table[host_joy_id],
						static_cast<SDL_GameControllerButton>(current_controller_map.button[SDL_CONTROLLER_BUTTON_BACK])) & 1;
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

int input_get_default_joystick(struct uae_input_device* uid, int i, int port, int af, int mode, bool gp, bool joymouseswap)
{
	int h, v;

	//if (joymouseswap) {
	//	if (i >= num_mouse)
	//		return 0;
	//	did = &di_mouse[i];
	//}
	//else {
	//	if (i >= num_joystick)
	//		return 0;
	//	did = &di_joystick[i];
	//}
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
	setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, h, gp);
	setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, v, gp);

	// ASSIGN ALL INPUT EVENT ACTIONS, EITHER CUSTOM OR DEFAULT
	//
	std::array<int, 15> thismap[4]{};
	thismap[0] = currprefs.jports[port].amiberry_custom_none;

	if (port < 2) // ports 0, 1 ... 
	{
		if (mode == JSEM_MODE_MOUSE)
		{
			int event = port ? INPUTEVENT_MOUSE2_UP : INPUTEVENT_MOUSE1_UP;
			for (int x = SDL_CONTROLLER_BUTTON_DPAD_UP; x <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}

			event = port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON;
			for (int x = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; x <= SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}
		}
		else
		{
			int event = port ? INPUTEVENT_JOY2_UP : INPUTEVENT_JOY1_UP;
			for (int x = SDL_CONTROLLER_BUTTON_DPAD_UP; x <= SDL_CONTROLLER_BUTTON_DPAD_DOWN; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}

			event = port ? INPUTEVENT_JOY2_LEFT : INPUTEVENT_JOY1_LEFT;
			for (int x = SDL_CONTROLLER_BUTTON_DPAD_LEFT; x <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}
		}
		// standard fire buttons
		if (mode == JSEM_MODE_JOYSTICK_CD32) // CD32 joypad  
		{
			int event = port ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED;
			for (int x = SDL_CONTROLLER_BUTTON_A; x <= SDL_CONTROLLER_BUTTON_B; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}

			event = port ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN;
			for (int x = SDL_CONTROLLER_BUTTON_X; x <= SDL_CONTROLLER_BUTTON_Y; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}

			event = port ? INPUTEVENT_JOY2_CD32_PLAY : INPUTEVENT_JOY1_CD32_PLAY;
			if (!thismap[0][SDL_CONTROLLER_BUTTON_START])
			{
				thismap[0][SDL_CONTROLLER_BUTTON_START] = event;
			}

			event = port ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD;
			for (int x = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; x <= SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}
		}
		else if (currprefs.jports[port].id >= JSEM_JOYS) // default, normal joystick  
		{
			int event = port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON;
			for (int x = SDL_CONTROLLER_BUTTON_A; x <= SDL_CONTROLLER_BUTTON_B; x++)
			{
				if (!thismap[0][x])
				{
					thismap[0][x] = event;
				}
				event++;
			}

			event = port ? INPUTEVENT_JOY2_UP : INPUTEVENT_JOY1_UP;
			if (!thismap[0][SDL_CONTROLLER_BUTTON_X])
			{
				thismap[0][SDL_CONTROLLER_BUTTON_X] = event;
			}

			event = port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON;
			if (!thismap[0][SDL_CONTROLLER_BUTTON_Y])
			{
				thismap[0][SDL_CONTROLLER_BUTTON_Y] = event;
			}

			event = INPUTEVENT_KEY_P;
			if (!thismap[0][SDL_CONTROLLER_BUTTON_START])
			{
				thismap[0][SDL_CONTROLLER_BUTTON_START] = event;
			}

			event = INPUTEVENT_KEY_SPACE;
			if (!thismap[0][SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
			{
				thismap[0][SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = event;
			}

			event = INPUTEVENT_KEY_RETURN;
			if (!thismap[0][SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
			{
				thismap[0][SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = event;
			}
		}
	}
	else // ports 2, 3 ... parallel ports 
	{
		int event = port - 2 ? INPUTEVENT_PAR_JOY2_UP : INPUTEVENT_PAR_JOY1_UP;
		for (int x = SDL_CONTROLLER_BUTTON_DPAD_UP; x <= SDL_CONTROLLER_BUTTON_DPAD_DOWN; x++)
		{
			if (!thismap[0][x])
			{
				thismap[0][x] = event;
			}
			event++;
		}

		event = port - 2 ? INPUTEVENT_PAR_JOY2_LEFT : INPUTEVENT_PAR_JOY1_LEFT;
		for (int x = SDL_CONTROLLER_BUTTON_DPAD_LEFT; x <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; x++)
		{
			if (!thismap[0][x])
			{
				thismap[0][x] = event;
			}
			event++;
		}

		event = port - 2 ? INPUTEVENT_PAR_JOY2_FIRE_BUTTON : INPUTEVENT_PAR_JOY1_FIRE_BUTTON;
		for (int x = SDL_CONTROLLER_BUTTON_A; x <= SDL_CONTROLLER_BUTTON_B; x++)
		{
			if (!thismap[0][x])
			{
				thismap[0][x] = event;
			}
			event++;
		}

		event = INPUTEVENT_KEY_P;
		if (!thismap[0][SDL_CONTROLLER_BUTTON_START])
		{
			thismap[0][SDL_CONTROLLER_BUTTON_START] = event;
		}

		event = INPUTEVENT_KEY_SPACE;
		if (!thismap[0][SDL_CONTROLLER_BUTTON_LEFTSHOULDER])
		{
			thismap[0][SDL_CONTROLLER_BUTTON_LEFTSHOULDER] = event;
		}

		event = INPUTEVENT_KEY_RETURN;
		if (!thismap[0][SDL_CONTROLLER_BUTTON_RIGHTSHOULDER])
		{
			thismap[0][SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] = event;
		}
	}

	thismap[1] = currprefs.jports[port].amiberry_custom_hotkey;

	for (auto n = 0; n < 2; ++n)
	{
		const auto function_offset = n * REMAP_BUTTONS;

		// s/e/w/n
		for (int x = SDL_CONTROLLER_BUTTON_A; x <= SDL_CONTROLLER_BUTTON_Y; x++)
			setid(uid, i, ID_BUTTON_OFFSET + x + function_offset, 0, port, thismap[n][x], af, gp);

		// left shoulder / right shoulder / start  
		for (int x = SDL_CONTROLLER_BUTTON_LEFTSHOULDER; x <= SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; x++)
			setid(uid, i, ID_BUTTON_OFFSET + (x - 5) + function_offset, 0, port, thismap[n][x], af, gp);

		setid(uid, i, ID_BUTTON_OFFSET + 6 + function_offset, 0, port, thismap[n][SDL_CONTROLLER_BUTTON_START], gp);

		// directions
		for (int x = SDL_CONTROLLER_BUTTON_DPAD_UP; x <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT; x++)
			setid(uid, i, ID_BUTTON_OFFSET + (x - 4) + function_offset, 0, port, thismap[n][x], af, gp);

		// stick buttons
		for (int x = SDL_CONTROLLER_BUTTON_LEFTSTICK; x <= SDL_CONTROLLER_BUTTON_RIGHTSTICK; x++)
			setid(uid, i, ID_BUTTON_OFFSET + (x + 4) + function_offset, 0, port, thismap[n][x], af, gp);
		
		setid(uid, i, ID_BUTTON_OFFSET + 13 + function_offset, 0, port, thismap[n][SDL_CONTROLLER_BUTTON_BACK], gp);
	}

	// if using retroarch options
	if (currprefs.use_retroarch_menu)
		setid(uid, i, ID_BUTTON_OFFSET + 14, 0, port, INPUTEVENT_SPC_ENTERGUI, gp);
	
	if (currprefs.use_retroarch_quit)
		setid(uid, i, ID_BUTTON_OFFSET + 15, 0, port, INPUTEVENT_SPC_QUIT, gp);
	
	if (currprefs.use_retroarch_reset)
		setid(uid, i, ID_BUTTON_OFFSET + 30, 0, port, INPUTEVENT_SPC_SOFTRESET, gp);

	if (i >= 0 && i < num_joystick)
		return 1;
	
	return 0;
}

int input_get_default_joystick_analog(struct uae_input_device* uid, int i, int port, int af, bool gp, bool joymouseswap)
{
	//if (joymouseswap) {
	//	if (i >= num_mouse)
	//		return 0;
	//	did = &di_mouse[i];
	//}
	//else {
	//	if (i >= num_joystick)
	//		return 0;
	//	did = &di_joystick[i];
	//}
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