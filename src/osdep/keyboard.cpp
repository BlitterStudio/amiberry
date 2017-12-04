#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "include/memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include <SDL.h>

static struct uae_input_device_kbr_default keytrans_amiga[] = {
	
	{ VK_ESCAPE, INPUTEVENT_KEY_ESC },

	{ SDLK_F1, INPUTEVENT_KEY_F1 },
	{ SDLK_F2, INPUTEVENT_KEY_F2 },
	{ SDLK_F3, INPUTEVENT_KEY_F3 },
	{ SDLK_F4, INPUTEVENT_KEY_F4 },
	
	{ SDLK_F5, INPUTEVENT_KEY_F5 },
	{ SDLK_F6, INPUTEVENT_KEY_F6 },
	{ SDLK_F7, INPUTEVENT_KEY_F7 },
	{ SDLK_F8, INPUTEVENT_KEY_F8 },
	{ SDLK_F9, INPUTEVENT_KEY_F9 },
	{ SDLK_F10, INPUTEVENT_KEY_F10 },

	{ SDLK_1, INPUTEVENT_KEY_1 },
	{ SDLK_2, INPUTEVENT_KEY_2 },
	{ SDLK_3, INPUTEVENT_KEY_3 },
	{ SDLK_4, INPUTEVENT_KEY_4 },
	{ SDLK_5, INPUTEVENT_KEY_5 },
	{ SDLK_6, INPUTEVENT_KEY_6 },
	{ SDLK_7, INPUTEVENT_KEY_7 },
	{ SDLK_8, INPUTEVENT_KEY_8 },
	{ SDLK_9, INPUTEVENT_KEY_9 },
	{ SDLK_0, INPUTEVENT_KEY_0 },

	{ SDLK_TAB, INPUTEVENT_KEY_TAB },

	{ SDLK_a, INPUTEVENT_KEY_A },
	{ SDLK_b, INPUTEVENT_KEY_B },
	{ SDLK_c, INPUTEVENT_KEY_C },
	{ SDLK_d, INPUTEVENT_KEY_D },
	{ SDLK_e, INPUTEVENT_KEY_E },
	{ SDLK_f, INPUTEVENT_KEY_F },
	{ SDLK_g, INPUTEVENT_KEY_G },
	{ SDLK_h, INPUTEVENT_KEY_H },
	{ SDLK_i, INPUTEVENT_KEY_I },
	{ SDLK_j, INPUTEVENT_KEY_J },
	{ SDLK_k, INPUTEVENT_KEY_K },
	{ SDLK_l, INPUTEVENT_KEY_L },
	{ SDLK_m, INPUTEVENT_KEY_M },
	{ SDLK_n, INPUTEVENT_KEY_N },
	{ SDLK_o, INPUTEVENT_KEY_O },
	{ SDLK_p, INPUTEVENT_KEY_P },
	{ SDLK_q, INPUTEVENT_KEY_Q },
	{ SDLK_r, INPUTEVENT_KEY_R },
	{ SDLK_s, INPUTEVENT_KEY_S },
	{ SDLK_t, INPUTEVENT_KEY_T },
	{ SDLK_u, INPUTEVENT_KEY_U },
	{ SDLK_v, INPUTEVENT_KEY_V },
	{ SDLK_w, INPUTEVENT_KEY_W },
	{ SDLK_x, INPUTEVENT_KEY_X },
	{ SDLK_y, INPUTEVENT_KEY_Y },
	{ SDLK_z, INPUTEVENT_KEY_Z },

	{ SDLK_CAPSLOCK, INPUTEVENT_KEY_CAPS_LOCK, ID_FLAG_TOGGLE },

	{ SDLK_KP_1, INPUTEVENT_KEY_NP_1 },
	{ SDLK_KP_2, INPUTEVENT_KEY_NP_2 },
	{ SDLK_KP_3, INPUTEVENT_KEY_NP_3 },
	{ SDLK_KP_4, INPUTEVENT_KEY_NP_4 },
	{ SDLK_KP_5, INPUTEVENT_KEY_NP_5 },
	{ SDLK_KP_6, INPUTEVENT_KEY_NP_6 },
	{ SDLK_KP_7, INPUTEVENT_KEY_NP_7 },
	{ SDLK_KP_8, INPUTEVENT_KEY_NP_8 },
	{ SDLK_KP_9, INPUTEVENT_KEY_NP_9 },
	{ SDLK_KP_0, INPUTEVENT_KEY_NP_0 },
	{ SDLK_KP_PERIOD, INPUTEVENT_KEY_PERIOD },
	{ SDLK_KP_PLUS, INPUTEVENT_KEY_NP_ADD },
	{ SDLK_KP_MINUS, INPUTEVENT_KEY_NP_SUB },
	{ SDLK_KP_MULTIPLY, INPUTEVENT_KEY_NP_MUL },
	{ SDLK_KP_DIVIDE, INPUTEVENT_KEY_NP_DIV },
	{ SDLK_KP_ENTER, INPUTEVENT_KEY_ENTER },         // The ENT from keypad..

	{ SDLK_MINUS, INPUTEVENT_KEY_SUB },
	{ SDLK_EQUALS, INPUTEVENT_KEY_EQUALS },
	{ SDLK_BACKSPACE, INPUTEVENT_KEY_BACKSPACE },	
	{ SDLK_RETURN, INPUTEVENT_KEY_RETURN },
	{ SDLK_SPACE, INPUTEVENT_KEY_SPACE },

	{ SDLK_LSHIFT, INPUTEVENT_KEY_SHIFT_LEFT, 0, INPUTEVENT_SPC_QUALIFIER_SHIFT },
	{ SDLK_LCTRL, INPUTEVENT_KEY_CTRL, 0, INPUTEVENT_SPC_QUALIFIER_CONTROL },
	{ SDLK_LGUI, INPUTEVENT_KEY_AMIGA_LEFT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDLK_LALT, INPUTEVENT_KEY_ALT_LEFT, 0, INPUTEVENT_SPC_QUALIFIER_ALT },
	{ SDLK_RALT, INPUTEVENT_KEY_ALT_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_ALT },
	{ SDLK_RGUI, INPUTEVENT_KEY_AMIGA_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDLK_MENU, INPUTEVENT_KEY_AMIGA_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDLK_RCTRL, INPUTEVENT_KEY_CTRL, 0, INPUTEVENT_SPC_QUALIFIER_CONTROL },
	{ SDLK_RSHIFT, INPUTEVENT_KEY_SHIFT_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_SHIFT },

	{ SDLK_UP, INPUTEVENT_KEY_CURSOR_UP },
	{ SDLK_DOWN, INPUTEVENT_KEY_CURSOR_DOWN },
	{ SDLK_LEFT, INPUTEVENT_KEY_CURSOR_LEFT },
	{ SDLK_RIGHT, INPUTEVENT_KEY_CURSOR_RIGHT },

	{ SDLK_INSERT, INPUTEVENT_KEY_AMIGA_LEFT },
	{ SDLK_DELETE, INPUTEVENT_KEY_DEL },
	{ SDLK_HOME, INPUTEVENT_KEY_AMIGA_RIGHT },
	{ SDLK_PAGEDOWN, INPUTEVENT_KEY_HELP },
	{ SDLK_PAGEUP, INPUTEVENT_SPC_FREEZEBUTTON },
	
	{ SDLK_LEFTBRACKET, INPUTEVENT_KEY_LEFTBRACKET },
	{ SDLK_RIGHTBRACKET, INPUTEVENT_KEY_RIGHTBRACKET },
	{ SDLK_SEMICOLON, INPUTEVENT_KEY_SEMICOLON },
	{ SDLK_QUOTE, INPUTEVENT_KEY_SINGLEQUOTE },
	{ SDLK_BACKQUOTE, INPUTEVENT_KEY_BACKQUOTE },
	{ SDLK_BACKSLASH, INPUTEVENT_KEY_BACKSLASH },
	{ SDLK_COMMA, INPUTEVENT_KEY_COMMA },	
	{ SDLK_PERIOD, INPUTEVENT_KEY_PERIOD },
	{ SDLK_SLASH, INPUTEVENT_KEY_DIV },

	{SDLK_END, INPUTEVENT_SPC_QUALIFIER_SPECIAL },
	{SDLK_PAUSE, INPUTEVENT_SPC_PAUSE },

	{SDLK_F12, INPUTEVENT_SPC_ENTERGUI },

	{ -1, 0 }
};

static struct uae_input_device_kbr_default *keytrans[] =
{
    keytrans_amiga,
	keytrans_amiga,
	keytrans_amiga
};

static int kb_np[] = { SDLK_KP_4, -1, SDLK_KP_6, -1, SDLK_KP_8, -1, SDLK_KP_2, -1, SDLK_KP_0, SDLK_KP_5, -1, SDLK_KP_DECIMAL, -1, SDLK_KP_ENTER, -1, -1 };
static int kb_ck[] = { SDLK_LEFT, -1, SDLK_RIGHT, -1, SDLK_UP, -1, SDLK_DOWN, -1, SDLK_RCTRL, SDLK_RALT, -1, SDLK_RSHIFT, -1, -1 };
static int kb_se[] = { SDLK_a, -1, SDLK_d, -1, SDLK_w, -1, SDLK_s, -1, SDLK_LALT, -1, SDLK_LSHIFT, -1, -1 };
static int kb_np3[] = { SDLK_KP_4, -1, SDLK_KP_6, -1, SDLK_KP_8, -1, SDLK_KP_2, -1, SDLK_KP_0, SDLK_KP_5, -1, SDLK_KP_DECIMAL, -1, SDLK_KP_ENTER, -1, -1 };
static int kb_ck3[] = { SDLK_LEFT, -1, SDLK_RIGHT, -1, SDLK_UP, -1, SDLK_DOWN, -1, SDLK_RCTRL, -1, SDLK_RSHIFT, -1, SDLK_RALT, -1, -1 };
static int kb_se3[] = { SDLK_a, -1, SDLK_d, -1, SDLK_w, -1, SDLK_s, -1, SDLK_LALT, -1, SDLK_LSHIFT, -1, SDLK_LCTRL, -1, -1 };

static int kb_cd32_np[] = { SDLK_KP_4, -1, SDLK_KP_6, -1, SDLK_KP_8, -1, SDLK_KP_2, -1, SDLK_KP_0, SDLK_KP_5, SDLK_KP_1, -1, SDLK_KP_DECIMAL, SDLK_KP_3, -1, SDLK_KP_7, -1, SDLK_KP_9, -1, SDLK_KP_DIVIDE, -1, SDLK_KP_MINUS, -1, SDLK_KP_MULTIPLY, -1, -1 };
static int kb_cd32_ck[] = { SDLK_LEFT, -1, SDLK_RIGHT, -1, SDLK_UP, -1, SDLK_DOWN, -1, SDLK_RCTRL, -1, SDLK_RALT, -1, SDLK_KP_7, -1, SDLK_KP_9, -1, SDLK_KP_DIVIDE, -1, SDLK_KP_MINUS, -1, SDLK_KP_MULTIPLY, -1, -1 };
static int kb_cd32_se[] = { SDLK_a, -1, SDLK_d, -1, SDLK_w, -1, SDLK_s, -1, -1, SDLK_LALT, -1, SDLK_LSHIFT, -1, SDLK_KP_7, -1, SDLK_KP_9, -1, SDLK_KP_DIVIDE, -1, SDLK_KP_MINUS, -1, SDLK_KP_MULTIPLY, -1, -1 };

static int kb_cdtv[] = { SDLK_KP_1, -1, SDLK_KP_3, -1, SDLK_KP_7, -1, SDLK_KP_9, -1, -1 };

static int kb_xa1[] = { SDLK_KP_4, -1, SDLK_KP_6, -1, SDLK_KP_8, -1, SDLK_KP_2, SDLK_KP_5, -1, SDLK_LCTRL, -1, SDLK_LALT, -1, SDLK_SPACE, -1, -1 };
static int kb_xa2[] = { SDLK_d, -1, SDLK_g, -1, SDLK_r, -1, SDLK_f, -1, SDLK_a, -1, SDLK_s, -1, SDLK_q, -1 };
static int kb_arcadia[] = { SDLK_F2, -1, SDLK_1, -1, SDLK_2, -1, SDLK_5, -1, SDLK_6, -1, -1 };
static int kb_arcadiaxa[] = { SDLK_1, -1, SDLK_2, -1, SDLK_3, -1, SDLK_4, -1, SDLK_6, -1, SDLK_LEFTBRACKET, SDLK_LSHIFT, -1, SDLK_RIGHTBRACKET, -1, SDLK_c, -1, SDLK_5, -1, SDLK_z, -1, SDLK_x, -1, -1 };

static int *kbmaps[] = {
	kb_np, kb_ck, kb_se, kb_np3, kb_ck3, kb_se3,
	kb_cd32_np, kb_cd32_ck, kb_cd32_se,
	kb_xa1, kb_xa2, kb_arcadia, kb_arcadiaxa, kb_cdtv
};

void keyboard_settrans(void)
{
	inputdevice_setkeytranslation(keytrans, kbmaps);
}

static bool specialpressed()
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_SPECIAL) != 0;
}
static bool shiftpressed()
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_SHIFT) != 0;
}
static bool altpressed()
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_ALT) != 0;
}
static bool ctrlpressed()
{
	return (input_getqualifiers() & ID_FLAG_QUALIFIER_CONTROL) != 0;
}

static int capslockstate;
static int host_capslockstate, host_numlockstate, host_scrolllockstate;

int getcapslockstate()
{
	return capslockstate;
}
void setcapslockstate(int state)
{
	capslockstate = state;
}

void clearallkeys()
{
	inputdevice_updateconfig(&changed_prefs, &currprefs);
}

static const int np[] = {
	SDLK_KP_0, 0, SDLK_KP_PERIOD, 0, SDLK_KP_1, 1, SDLK_KP_2, 2,
	SDLK_KP_3, 3, SDLK_KP_4, 4, SDLK_KP_5, 5, SDLK_KP_6, 6, SDLK_KP_7, 7,
	SDLK_KP_8, 8, SDLK_KP_9, 9, -1 };

void translate_amiberry_keys(int scancode, int newstate)
{
	int code = 0;
	bool amode = currprefs.input_keyboard_type == 0;
	bool special = false;
	static int swapperdrive = 0;

	int scancode_new = scancode;

	if (newstate) {
		int defaultguikey = SDLK_F12;
		if (currprefs.open_gui != "") {
			if (scancode_new == defaultguikey && SDL_GetKeyFromName(currprefs.open_gui) != scancode_new) {
				scancode = 0;
				if (specialpressed() && ctrlpressed() && shiftpressed() && altpressed())
					inputdevice_add_inputcode(AKS_ENTERGUI, 1);
			}
			else if (scancode_new == SDL_GetKeyFromName(currprefs.open_gui)) {
				inputdevice_add_inputcode(AKS_ENTERGUI, 1);
				scancode = 0;
			}
		}
		else if (!specialpressed() && !ctrlpressed() && !shiftpressed() && !altpressed() && scancode_new == defaultguikey) {
			inputdevice_add_inputcode(AKS_ENTERGUI, 1);
			scancode = 0;
		}
		if (currprefs.quit_amiberry != "" && scancode_new == SDL_GetKeyFromName(currprefs.quit_amiberry))
		{
			inputdevice_add_inputcode(AKS_QUIT, 1);
			scancode = 0;
		}
	}

	if (code) {
		inputdevice_add_inputcode(code, 1);
		return;
	}

	scancode = scancode_new;
	if (!specialpressed() && newstate) {
		if (scancode == SDLK_CAPSLOCK) {
			host_capslockstate = host_capslockstate ? 0 : 1;
			capslockstate = host_capslockstate;
		}
		if (scancode == SDLK_NUMLOCKCLEAR) {
			host_numlockstate = host_numlockstate ? 0 : 1;
			capslockstate = host_numlockstate;
		}
		if (scancode == SDLK_SCROLLLOCK) {
			host_scrolllockstate = host_scrolllockstate ? 0 : 1;
			capslockstate = host_scrolllockstate;
		}
	}

	int translatedScancode = scancode;
	switch (scancode)
	{
	case SDLK_UP:
		translatedScancode = AK_UP;
		break;
	case SDLK_DOWN:
		translatedScancode = AK_DN;
		break;
	case SDLK_LEFT:
		translatedScancode = AK_LF;
		break;
	case SDLK_RIGHT:
		translatedScancode = AK_RT;
		break;
	case SDLK_F1:
		translatedScancode = AK_F1;
		break;
	case SDLK_F2:
		translatedScancode = AK_F2;
		break;
	case SDLK_F3:
		translatedScancode = AK_F3;
		break;
	case SDLK_F4:
		translatedScancode = AK_F4;
		break;
	case SDLK_F5:
		translatedScancode = AK_F5;
		break;
	case SDLK_F6:
		translatedScancode = AK_F6;
		break;
	case SDLK_F7:
		translatedScancode = AK_F7;
		break;
	case SDLK_F8:
		translatedScancode = AK_F8;
		break;
	case SDLK_F9:
		translatedScancode = AK_F9;
		break;
	case SDLK_F10:
		translatedScancode = AK_F10;
		break;
	case SDLK_LSHIFT:
		translatedScancode = AK_LSH;
		break;
	case SDLK_RSHIFT:
		translatedScancode = AK_RSH;
		break;
	case SDLK_RGUI:
	case SDLK_APPLICATION:
		translatedScancode = AK_RAMI;
		break;
	case SDLK_LGUI:
		translatedScancode = AK_LAMI;
		break;
	case SDLK_LALT:
		translatedScancode = AK_LALT;
		break;
	case SDLK_RALT:
		translatedScancode = AK_RALT;
		break;
	case SDLK_LCTRL:
	case SDLK_RCTRL:
		translatedScancode = AK_CTRL;
		break;
	case SDLK_PAGEDOWN:
		translatedScancode = AK_HELP;
		break;
	case SDLK_KP_0:
		translatedScancode = AK_NP0;
		break;
	case SDLK_KP_1:
		translatedScancode = AK_NP1;
		break;
	case SDLK_KP_2:
		translatedScancode = AK_NP2;
		break;
	case SDLK_KP_3:
		translatedScancode = AK_NP3;
		break;
	case SDLK_KP_4:
		translatedScancode = AK_NP4;
		break;
	case SDLK_KP_5:
		translatedScancode = AK_NP5;
		break;
	case SDLK_KP_6:
		translatedScancode = AK_NP6;
		break;
	case SDLK_KP_7:
		translatedScancode = AK_NP7;
		break;
	case SDLK_KP_8:
		translatedScancode = AK_NP8;
		break;
	case SDLK_KP_9:
		translatedScancode = AK_NP9;
		break;
	case SDLK_KP_ENTER:
		translatedScancode = AK_ENT;
		break;
	case SDLK_KP_DIVIDE:
		translatedScancode = AK_NPDIV;
		break;
	case SDLK_KP_MULTIPLY:
		translatedScancode = AK_NPMUL;
		break;
	case SDLK_KP_MINUS:
		translatedScancode = AK_NPSUB;
		break;
	case SDLK_KP_PLUS:
		translatedScancode = AK_NPADD;
		break;
	case SDLK_KP_PERIOD:
		translatedScancode = AK_NPDEL;
		break;
	}

	if (special) {
		inputdevice_checkqualifierkeycode(0, translatedScancode, newstate);
		return;
	}

	if (translatedScancode != scancode)
		inputdevice_do_keyboard(translatedScancode, newstate);
	else
		inputdevice_translatekeycode(0, translatedScancode, newstate);
}

int target_checkcapslock(const int scancode, int *state)
{
	if (scancode != SDLK_CAPSLOCK && scancode != SDLK_NUMLOCKCLEAR && scancode != SDLK_SCROLLLOCK)
		return 0;
	if (*state == 0)
		return -1;
	if (scancode == SDLK_CAPSLOCK)
		*state = host_capslockstate;
	if (scancode == SDLK_NUMLOCKCLEAR)
		*state = host_numlockstate;
	if (scancode == SDLK_SCROLLLOCK)
		*state = host_scrolllockstate;
	return 1;
}
