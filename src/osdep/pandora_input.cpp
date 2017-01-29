#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "SDL.h"

static int joyXviaCustom = 0;
static int joyYviaCustom = 0;
static int joyButXviaCustom[7] = {0, 0, 0, 0, 0, 0, 0};
static int mouseBut1viaCustom = 0;
static int mouseBut2viaCustom = 0;

#define MAX_MOUSE_BUTTONS	2
#define MAX_MOUSE_AXES	2
#define FIRST_MOUSE_AXIS	0
#define FIRST_MOUSE_BUTTON	MAX_MOUSE_AXES

static int init_mouse()
{
	return 1;
}

static void close_mouse()
{
}

static int acquire_mouse(int num, int flags)
{
	return 1;
}

static void unacquire_mouse(int num)
{
}

static int get_mouse_num()
{
	return 2;
}

static const char* get_mouse_friendlyname(int mouse)
{
	if (mouse == 0)
		return "Nubs as mouse";
	else
		return "dPad as mouse";
}

static const char* get_mouse_uniquename(int mouse)
{
	if (mouse == 0)
		return "MOUSE0";
	else
		return "MOUSE1";
}

static int get_mouse_widget_num(int mouse)
{
	return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS;
}

static int get_mouse_widget_first(int mouse, int type)
{
	switch (type)
	{
	case IDEV_WIDGET_BUTTON:
		return FIRST_MOUSE_BUTTON;
	case IDEV_WIDGET_AXIS:
		return FIRST_MOUSE_AXIS;
	case IDEV_WIDGET_BUTTONAXIS:
		return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS;
	}
	return -1;
}

static int get_mouse_widget_type(int mouse, int num, TCHAR* name, uae_u32* code)
{
	if (num >= MAX_MOUSE_AXES && num < MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS)
	{
		if (name)
			sprintf(name, "Button %d", num + 1 - MAX_MOUSE_AXES);
		return IDEV_WIDGET_BUTTON;
	}
	else if (num < MAX_MOUSE_AXES)
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

	if (currprefs.jports[0].id == JSEM_MICE + 1 || currprefs.jports[1].id == JSEM_MICE + 1)
	{
		// dPad is mouse
		const Uint8* keystate = SDL_GetKeyboardState(nullptr);
		int mouseScale = currprefs.input_joymouse_multiplier / 4;

		if (keystate[VK_LEFT])
			setmousestate(1, 0, -mouseScale, 0);
		if (keystate[VK_RIGHT])
			setmousestate(1, 0, mouseScale, 0);
		if (keystate[VK_UP])
			setmousestate(1, 1, -mouseScale, 0);
		if (keystate[VK_DOWN])
			setmousestate(1, 1, mouseScale, 0);

		if (!mouseBut1viaCustom)
			setmousebuttonstate(1, 0, keystate[VK_A]); // A button -> left mouse
		if (!mouseBut2viaCustom)
			setmousebuttonstate(1, 1, keystate[VK_B]); // B button -> right mouse
	}

	// Nubs as mouse handled in handle_msgpump()
}


static int get_mouse_flags(int num)
{
	return 0;
}

struct inputdevice_functions inputdevicefunc_mouse = {
	init_mouse,
	close_mouse,
	acquire_mouse,
	unacquire_mouse,
	read_mouse,
	get_mouse_num,
	get_mouse_friendlyname,
	get_mouse_uniquename,
	get_mouse_widget_num,
	get_mouse_widget_type,
	get_mouse_widget_first,
	get_mouse_flags
};

static void setid(struct uae_input_device* uid, int i, int slot, int sub, int port, int evt, bool gp)
{
	if (gp)
		inputdevice_sparecopy(&uid[i], slot, 0);
	uid[i].eventid[slot][sub] = evt;
	uid[i].port[slot][sub] = port + 1;
}

static void setid_af(struct uae_input_device* uid, int i, int slot, int sub, int port, int evt, int af, bool gp)
{
	setid(uid, i, slot, sub, port, evt, gp);
	uid[i].flags[slot][sub] &= ~ID_FLAG_AUTOFIRE_MASK;
	if (af >= JPORT_AF_NORMAL)
		uid[i].flags[slot][sub] |= ID_FLAG_AUTOFIRE;
}

int input_get_default_mouse(struct uae_input_device* uid, int i, int port, int af, bool gp, bool wheel, bool joymouseswap)
{
	setid(uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ, gp);
	setid(uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT, gp);
	setid_af(uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
	setid(uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);

	if (i == 0)
		return 1;
	return 0;
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

static int get_kb_num()
{
	return 1;
}

static const char* get_kb_friendlyname(int kb)
{
	return strdup("Default Keyboard");
}

static const char* get_kb_uniquename(int kb)
{
	return strdup("KEYBOARD0");
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

static int get_kb_flags(int num)
{
	return 0;
}

struct inputdevice_functions inputdevicefunc_keyboard = {
	init_kb,
	close_kb,
	acquire_kb,
	unacquire_kb,
	read_kb,
	get_kb_num,
	get_kb_friendlyname,
	get_kb_uniquename,
	get_kb_widget_num,
	get_kb_widget_type,
	get_kb_widget_first,
	get_kb_flags
};

int input_get_default_keyboard(int num)
{
	if (num == 0)
	{
		return 1;
	}
	return 0;
}

#define MAX_JOY_BUTTONS	7
#define MAX_JOY_AXES	2
#define FIRST_JOY_AXIS	0
#define FIRST_JOY_BUTTON MAX_JOY_AXES

static int nr_joysticks = 0;
static char JoystickName[MAX_INPUT_DEVICES][80];

static char IsPS3Controller[MAX_INPUT_DEVICES];

static SDL_Joystick* Joysticktable[MAX_INPUT_DEVICES];


static int get_joystick_num()
{
	// Keep joystick 0 as Pandora implementation...
	return (nr_joysticks + 1);
}

static int init_joystick()
{
	//This function is called too many times... we can filter if number of joy is good...
	if (nr_joysticks == SDL_NumJoysticks())
		return 1;

	nr_joysticks = SDL_NumJoysticks();
	if (nr_joysticks > MAX_INPUT_DEVICES)
		nr_joysticks = MAX_INPUT_DEVICES;
	for (int cpt = 0; cpt < nr_joysticks; cpt++)
	{
		Joysticktable[cpt] = SDL_JoystickOpen(cpt);
		strncpy(JoystickName[cpt], SDL_JoystickNameForIndex(cpt), 80);
		printf("Joystick %i : %s\n", cpt, JoystickName[cpt]);
		printf("    Buttons: %i Axis: %i Hats: %i\n", SDL_JoystickNumButtons(Joysticktable[cpt]), SDL_JoystickNumAxes(Joysticktable[cpt]), SDL_JoystickNumHats(Joysticktable[cpt]));

		if (strcmp(JoystickName[cpt], "Sony PLAYSTATION(R)3 Controller") == 0 ||
			strcmp(JoystickName[cpt], "PLAYSTATION(R)3 Controller") == 0)
		{
			printf("    Found a dualshock controller: Activating workaround.\n");
			IsPS3Controller[cpt] = 1;
		}
		else
			IsPS3Controller[cpt] = 0;
	}

	return 1;
}

static void close_joystick()
{
	for (int cpt = 0; cpt < nr_joysticks; cpt++)
	{
		SDL_JoystickClose(Joysticktable[cpt]);
	}
}


static int acquire_joystick(int num, int flags)
{
	return 1;
}

static void unacquire_joystick(int num)
{
}

static const char* get_joystick_friendlyname(int joy)
{
	if (joy == 0)
		return "dPad as joystick";
	else
		return JoystickName[joy - 1];
}

static const char* get_joystick_uniquename(int joy)
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


static int get_joystick_widget_num(int joy)
{
	return MAX_JOY_AXES + MAX_JOY_BUTTONS;
}

static int get_joystick_widget_first(int joy, int type)
{
	switch (type)
	{
	case IDEV_WIDGET_BUTTON:
		return FIRST_JOY_BUTTON;
	case IDEV_WIDGET_AXIS:
		return FIRST_JOY_AXIS;
	case IDEV_WIDGET_BUTTONAXIS:
		return MAX_JOY_AXES + MAX_JOY_BUTTONS;
	}
	return -1;
}

static int get_joystick_widget_type(int joy, int num, TCHAR* name, uae_u32* code)
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
			}
		}
		return IDEV_WIDGET_BUTTON;
	}
	else if (num < MAX_JOY_AXES)
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

static int get_joystick_flags(int num)
{
	return 0;
}


static void read_joystick()
{
	for (int joyid = 0; joyid < MAX_JPORTS; joyid++)
	// First handle fake joystick from pandora...
		if (currprefs.jports[joyid].id == JSEM_JOYS)
		{
			const Uint8* keystate = SDL_GetKeyboardState(nullptr);

			if (!keystate[VK_R])
			{ // Right shoulder + dPad -> cursor keys
				int axis = (keystate[VK_LEFT] ? -32767 : (keystate[VK_RIGHT] ? 32767 : 0));
				if (!joyXviaCustom)
					setjoystickstate(0, 0, axis, 32767);
				axis = (keystate[VK_UP] ? -32767 : (keystate[VK_DOWN] ? 32767 : 0));
				if (!joyYviaCustom)
					setjoystickstate(0, 1, axis, 32767);
			}
			if (!joyButXviaCustom[0])
				setjoybuttonstate(0, 0, keystate[VK_X]);
			if (!joyButXviaCustom[1])
				setjoybuttonstate(0, 1, keystate[VK_B]);
			if (!joyButXviaCustom[2])
				setjoybuttonstate(0, 2, keystate[VK_A]);
			if (!joyButXviaCustom[3])
				setjoybuttonstate(0, 3, keystate[VK_Y]);

			int cd32_start = 0, cd32_ffw = 0, cd32_rwd = 0;
			if (keystate[SDL_SCANCODE_LALT])
			{ // Pandora Start button
				if (keystate[VK_L]) // Left shoulder
					cd32_rwd = 1;
				else if (keystate[VK_R]) // Right shoulder
					cd32_ffw = 1;
				else
					cd32_start = 1;
			}
			if (!joyButXviaCustom[6])
				setjoybuttonstate(0, 6, cd32_start);
			if (!joyButXviaCustom[5])
				setjoybuttonstate(0, 5, cd32_ffw);
			if (!joyButXviaCustom[4])
				setjoybuttonstate(0, 4, cd32_rwd);
		}
		else if (jsem_isjoy(joyid, &currprefs) != -1)
		{
			// Now we handle real SDL joystick...
			int hostjoyid = currprefs.jports[joyid].id - JSEM_JOYS - 1;
			int hat = SDL_JoystickGetHat(Joysticktable[hostjoyid], 0);
			int val = SDL_JoystickGetAxis(Joysticktable[hostjoyid], 0);

			if (hat & SDL_HAT_RIGHT)
				setjoystickstate(hostjoyid + 1, 0, 32767, 32767);
			else if (hat & SDL_HAT_LEFT)
				setjoystickstate(hostjoyid + 1, 0, -32767, 32767);
			else
				setjoystickstate(hostjoyid + 1, 0, val, 32767);
			val = SDL_JoystickGetAxis(Joysticktable[hostjoyid], 1);
			if (hat & SDL_HAT_UP)
				setjoystickstate(hostjoyid + 1, 1, -32767, 32767);
			else if (hat & SDL_HAT_DOWN)
				setjoystickstate(hostjoyid + 1, 1, 32767, 32767);
			else
				setjoystickstate(hostjoyid + 1, 1, val, 32767);

			setjoybuttonstate(hostjoyid + 1, 0, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 0) & 1));
			setjoybuttonstate(hostjoyid + 1, 1, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 1) & 1));
			setjoybuttonstate(hostjoyid + 1, 2, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 2) & 1));
			setjoybuttonstate(hostjoyid + 1, 3, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 3) & 1));

			// cd32 start, ffw, rwd
			setjoybuttonstate(hostjoyid + 1, 4, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 4) & 1));
			setjoybuttonstate(hostjoyid + 1, 5, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 5) & 1));
			setjoybuttonstate(hostjoyid + 1, 6, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 6) & 1));

			if (IsPS3Controller[hostjoyid])
			{
				setjoybuttonstate(hostjoyid + 1, 0, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 13) & 1));
				setjoybuttonstate(hostjoyid + 1, 1, (SDL_JoystickGetButton(Joysticktable[hostjoyid], 14) & 1));

				// Simulate a top with button 4
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 4))
					setjoystickstate(hostjoyid + 1, 1, -32767, 32767);
				// Simulate a right with button 5
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 5))
					setjoystickstate(hostjoyid + 1, 0, 32767, 32767);
				// Simulate a bottom with button 6
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 6))
					setjoystickstate(hostjoyid + 1, 1, 32767, 32767);
				// Simulate a left with button 7
				if (SDL_JoystickGetButton(Joysticktable[hostjoyid], 7))
					setjoystickstate(hostjoyid + 1, 0, -32767, 32767);
			}
		}
}

struct inputdevice_functions inputdevicefunc_joystick = {
	init_joystick,
	close_joystick,
	acquire_joystick,
	unacquire_joystick,
	read_joystick,
	get_joystick_num,
	get_joystick_friendlyname,
	get_joystick_uniquename,
	get_joystick_widget_num,
	get_joystick_widget_type,
	get_joystick_widget_first,
	get_joystick_flags
};

int input_get_default_joystick(struct uae_input_device* uid, int num, int port, int af, int mode, bool gp, bool joymouseswap)
{
	int h, v;

	h = port ? INPUTEVENT_JOY2_HORIZ : INPUTEVENT_JOY1_HORIZ;;
	v = port ? INPUTEVENT_JOY2_VERT : INPUTEVENT_JOY1_VERT;

	setid(uid, num, ID_AXIS_OFFSET + 0, 0, port, h, gp);
	setid(uid, num, ID_AXIS_OFFSET + 1, 0, port, v, gp);

	setid_af(uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
	setid(uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
	setid(uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);

	if (mode == JSEM_MODE_JOYSTICK_CD32)
	{
		setid_af(uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED, af, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_CD32_BLUE : INPUTEVENT_JOY1_CD32_BLUE, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 3, 0, port, port ? INPUTEVENT_JOY2_CD32_YELLOW : INPUTEVENT_JOY1_CD32_YELLOW, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 4, 0, port, port ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 5, 0, port, port ? INPUTEVENT_JOY2_CD32_FFW : INPUTEVENT_JOY1_CD32_FFW, gp);
		setid(uid, num, ID_BUTTON_OFFSET + 6, 0, port, port ? INPUTEVENT_JOY2_CD32_PLAY : INPUTEVENT_JOY1_CD32_PLAY, gp);
	}
	if (num == 0)
	{
		return 1;
	}
	return 0;
}

int input_get_default_joystick_analog(struct uae_input_device* uid, int num, int port, int af, bool joymouseswap)
{
	return 0;
}


void SimulateMouseOrJoy(int code, int keypressed)
{
	switch (code)
	{
	case REMAP_MOUSEBUTTON_LEFT:
		mouseBut1viaCustom = keypressed;
		setmousebuttonstate(0, 0, keypressed);
		setmousebuttonstate(1, 0, keypressed);
		break;

	case REMAP_MOUSEBUTTON_RIGHT:
		mouseBut2viaCustom = keypressed;
		setmousebuttonstate(0, 1, keypressed);
		setmousebuttonstate(1, 1, keypressed);
		break;

	case REMAP_JOYBUTTON_ONE:
		joyButXviaCustom[0] = keypressed;
		setjoybuttonstate(0, 0, keypressed);
		break;

	case REMAP_JOYBUTTON_TWO:
		joyButXviaCustom[1] = keypressed;
		setjoybuttonstate(0, 1, keypressed);
		break;

	case REMAP_JOY_UP:
		joyYviaCustom = keypressed;
		setjoystickstate(0, 1, keypressed ? -32767 : 0, 32767);
		break;

	case REMAP_JOY_DOWN:
		joyYviaCustom = keypressed;
		setjoystickstate(0, 1, keypressed ? 32767 : 0, 32767);
		break;

	case REMAP_JOY_LEFT:
		joyXviaCustom = keypressed;
		setjoystickstate(0, 0, keypressed ? -32767 : 0, 32767);
		break;

	case REMAP_JOY_RIGHT:
		joyXviaCustom = keypressed;
		setjoystickstate(0, 0, keypressed ? 32767 : 0, 32767);
		break;

	case REMAP_CD32_GREEN:
		joyButXviaCustom[2] = keypressed;
		setjoybuttonstate(0, 2, keypressed);
		break;

	case REMAP_CD32_YELLOW:
		joyButXviaCustom[3] = keypressed;
		setjoybuttonstate(0, 3, keypressed);
		break;

	case REMAP_CD32_PLAY:
		joyButXviaCustom[6] = keypressed;
		setjoybuttonstate(0, 6, keypressed);
		break;

	case REMAP_CD32_FFW:
		joyButXviaCustom[5] = keypressed;
		setjoybuttonstate(0, 5, keypressed);
		break;

	case REMAP_CD32_RWD:
		joyButXviaCustom[4] = keypressed;
		setjoybuttonstate(0, 4, keypressed);
		break;
	}
}
