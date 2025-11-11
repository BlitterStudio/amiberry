#include <cstring>

#include "sysdeps.h"
#include "gui.h"
#include "options.h"
#include "inputdevice.h"
#include "keyboard.h"

#include "clipboard.h"
#include "config.h"
#include "uae.h"

char keyboard_type = 0;
static struct uae_input_device_kbr_default keytrans_amiga[] = {
	
	{ SDL_SCANCODE_ESCAPE, INPUTEVENT_KEY_ESC },

	{ SDL_SCANCODE_F1, INPUTEVENT_KEY_F1, 0, INPUTEVENT_SPC_FLOPPY0, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_EFLOPPY0, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_F2, INPUTEVENT_KEY_F2, 0, INPUTEVENT_SPC_FLOPPY1, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_EFLOPPY1, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_F3, INPUTEVENT_KEY_F3, 0, INPUTEVENT_SPC_FLOPPY2, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_EFLOPPY2, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_F4, INPUTEVENT_KEY_F4, 0, INPUTEVENT_SPC_FLOPPY3, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_EFLOPPY3, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	
	{ SDL_SCANCODE_F5, INPUTEVENT_KEY_F5, 0, INPUTEVENT_SPC_CD0, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_ECD0, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_F6, INPUTEVENT_KEY_F6, 0, INPUTEVENT_SPC_STATERESTOREDIALOG, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_STATESAVEDIALOG, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_F7, INPUTEVENT_KEY_F7 },
	{ SDL_SCANCODE_F8, INPUTEVENT_KEY_F8 },
	{ SDL_SCANCODE_F9, INPUTEVENT_KEY_F9 },
	{ SDL_SCANCODE_F10, INPUTEVENT_KEY_F10 },

	{ SDL_SCANCODE_1, INPUTEVENT_KEY_1 },
	{ SDL_SCANCODE_2, INPUTEVENT_KEY_2 },
	{ SDL_SCANCODE_3, INPUTEVENT_KEY_3 },
	{ SDL_SCANCODE_4, INPUTEVENT_KEY_4 },
	{ SDL_SCANCODE_5, INPUTEVENT_KEY_5 },
	{ SDL_SCANCODE_6, INPUTEVENT_KEY_6 },
	{ SDL_SCANCODE_7, INPUTEVENT_KEY_7 },
	{ SDL_SCANCODE_8, INPUTEVENT_KEY_8 },
	{ SDL_SCANCODE_9, INPUTEVENT_KEY_9 },
	{ SDL_SCANCODE_0, INPUTEVENT_KEY_0 },

	{ SDL_SCANCODE_TAB, INPUTEVENT_KEY_TAB },

	{ SDL_SCANCODE_A, INPUTEVENT_KEY_A },
	{ SDL_SCANCODE_B, INPUTEVENT_KEY_B },
	{ SDL_SCANCODE_C, INPUTEVENT_KEY_C },
	{ SDL_SCANCODE_D, INPUTEVENT_KEY_D },
	{ SDL_SCANCODE_E, INPUTEVENT_KEY_E },
	{ SDL_SCANCODE_F, INPUTEVENT_KEY_F },
	{ SDL_SCANCODE_G, INPUTEVENT_KEY_G },
	{ SDL_SCANCODE_H, INPUTEVENT_KEY_H },
	{ SDL_SCANCODE_I, INPUTEVENT_KEY_I },
	{ SDL_SCANCODE_J, INPUTEVENT_KEY_J, 0, INPUTEVENT_SPC_SWAPJOYPORTS, ID_FLAG_QUALIFIER_SPECIAL },
	{ SDL_SCANCODE_K, INPUTEVENT_KEY_K },
	{ SDL_SCANCODE_L, INPUTEVENT_KEY_L },
	{ SDL_SCANCODE_M, INPUTEVENT_KEY_M },
	{ SDL_SCANCODE_N, INPUTEVENT_KEY_N },
	{ SDL_SCANCODE_O, INPUTEVENT_KEY_O },
	{ SDL_SCANCODE_P, INPUTEVENT_KEY_P },
	{ SDL_SCANCODE_Q, INPUTEVENT_KEY_Q },
	{ SDL_SCANCODE_R, INPUTEVENT_KEY_R },
	{ SDL_SCANCODE_S, INPUTEVENT_KEY_S },
	{ SDL_SCANCODE_T, INPUTEVENT_KEY_T },
	{ SDL_SCANCODE_U, INPUTEVENT_KEY_U },
	{ SDL_SCANCODE_V, INPUTEVENT_KEY_V },
	{ SDL_SCANCODE_W, INPUTEVENT_KEY_W },
	{ SDL_SCANCODE_X, INPUTEVENT_KEY_X },
	{ SDL_SCANCODE_Y, INPUTEVENT_KEY_Y },
	{ SDL_SCANCODE_Z, INPUTEVENT_KEY_Z },

	{ SDL_SCANCODE_CAPSLOCK, INPUTEVENT_KEY_CAPS_LOCK, ID_FLAG_TOGGLE },

	{ SDL_SCANCODE_KP_1, INPUTEVENT_KEY_NP_1 },
	{ SDL_SCANCODE_KP_2, INPUTEVENT_KEY_NP_2 },
	{ SDL_SCANCODE_KP_3, INPUTEVENT_KEY_NP_3 },
	{ SDL_SCANCODE_KP_4, INPUTEVENT_KEY_NP_4 },
	{ SDL_SCANCODE_KP_5, INPUTEVENT_KEY_NP_5 },
	{ SDL_SCANCODE_KP_6, INPUTEVENT_KEY_NP_6 },
	{ SDL_SCANCODE_KP_7, INPUTEVENT_KEY_NP_7 },
	{ SDL_SCANCODE_KP_8, INPUTEVENT_KEY_NP_8 },
	{ SDL_SCANCODE_KP_9, INPUTEVENT_KEY_NP_9 },
	{ SDL_SCANCODE_KP_0, INPUTEVENT_KEY_NP_0 },
	{ SDL_SCANCODE_KP_PERIOD, INPUTEVENT_KEY_NP_PERIOD },
	{ SDL_SCANCODE_KP_PLUS, INPUTEVENT_KEY_NP_ADD, 0, INPUTEVENT_SPC_VOLUME_UP, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_MASTER_VOLUME_UP, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_CONTROL, INPUTEVENT_SPC_INCREASE_REFRESHRATE, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_KP_MINUS, INPUTEVENT_KEY_NP_SUB, 0, INPUTEVENT_SPC_VOLUME_DOWN, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_MASTER_VOLUME_DOWN, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_CONTROL, INPUTEVENT_SPC_DECREASE_REFRESHRATE, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_KP_MULTIPLY, INPUTEVENT_KEY_NP_MUL, 0, INPUTEVENT_SPC_VOLUME_MUTE, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_MASTER_VOLUME_MUTE, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_CONTROL },
	{ SDL_SCANCODE_KP_DIVIDE, INPUTEVENT_KEY_NP_DIV, 0, INPUTEVENT_SPC_STATEREWIND, ID_FLAG_QUALIFIER_SPECIAL },
	{ SDL_SCANCODE_KP_ENTER, INPUTEVENT_KEY_ENTER },

	{ SDL_SCANCODE_MINUS, INPUTEVENT_KEY_SUB },
	{ SDL_SCANCODE_EQUALS, INPUTEVENT_KEY_EQUALS },
	{ SDL_SCANCODE_BACKSPACE, INPUTEVENT_KEY_BACKSPACE },
	{ SDL_SCANCODE_RETURN, INPUTEVENT_KEY_RETURN },
	{ SDL_SCANCODE_SPACE, INPUTEVENT_KEY_SPACE },

	{ SDL_SCANCODE_LSHIFT, INPUTEVENT_KEY_SHIFT_LEFT, 0, INPUTEVENT_SPC_QUALIFIER_SHIFT },
	{ SDL_SCANCODE_LCTRL, INPUTEVENT_KEY_CTRL, 0, INPUTEVENT_SPC_QUALIFIER_CONTROL },
	{ SDL_SCANCODE_LGUI, INPUTEVENT_KEY_AMIGA_LEFT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDL_SCANCODE_LALT, INPUTEVENT_KEY_ALT_LEFT, 0, INPUTEVENT_SPC_QUALIFIER_ALT },
	{ SDL_SCANCODE_RALT, INPUTEVENT_KEY_ALT_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_ALT },
	{ SDL_SCANCODE_RGUI, INPUTEVENT_KEY_AMIGA_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDL_SCANCODE_MENU, INPUTEVENT_KEY_AMIGA_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDL_SCANCODE_APPLICATION, INPUTEVENT_KEY_AMIGA_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_WIN },
	{ SDL_SCANCODE_RCTRL, INPUTEVENT_KEY_CTRL, 0, INPUTEVENT_SPC_QUALIFIER_CONTROL },
	{ SDL_SCANCODE_RSHIFT, INPUTEVENT_KEY_SHIFT_RIGHT, 0, INPUTEVENT_SPC_QUALIFIER_SHIFT },

	{ SDL_SCANCODE_UP, INPUTEVENT_KEY_CURSOR_UP },
	{ SDL_SCANCODE_DOWN, INPUTEVENT_KEY_CURSOR_DOWN },
	{ SDL_SCANCODE_LEFT, INPUTEVENT_KEY_CURSOR_LEFT, 0, INPUTEVENT_SPC_PAUSE, ID_FLAG_QUALIFIER_SPECIAL },
	{ SDL_SCANCODE_RIGHT, INPUTEVENT_KEY_CURSOR_RIGHT, 0, INPUTEVENT_SPC_WARP, ID_FLAG_QUALIFIER_SPECIAL },

	{ SDL_SCANCODE_INSERT, INPUTEVENT_KEY_AMIGA_LEFT, 0, INPUTEVENT_SPC_PASTE, ID_FLAG_QUALIFIER_SPECIAL },
	{ SDL_SCANCODE_DELETE, INPUTEVENT_KEY_DEL },
	{ SDL_SCANCODE_HOME, INPUTEVENT_KEY_AMIGA_RIGHT },
	{ SDL_SCANCODE_PAGEDOWN, INPUTEVENT_KEY_HELP },
	{ SDL_SCANCODE_PAGEUP, INPUTEVENT_SPC_FREEZEBUTTON },
	
	{ SDL_SCANCODE_LEFTBRACKET, INPUTEVENT_KEY_LEFTBRACKET },
	{ SDL_SCANCODE_RIGHTBRACKET, INPUTEVENT_KEY_RIGHTBRACKET },
	{ SDL_SCANCODE_SEMICOLON, INPUTEVENT_KEY_SEMICOLON },
	{ SDL_SCANCODE_APOSTROPHE, INPUTEVENT_KEY_SINGLEQUOTE },
	{ SDL_SCANCODE_GRAVE, INPUTEVENT_KEY_BACKQUOTE },
	{ SDL_SCANCODE_BACKSLASH, INPUTEVENT_KEY_NUMBERSIGN },
	{ SDL_SCANCODE_COMMA, INPUTEVENT_KEY_COMMA },
	{ SDL_SCANCODE_PERIOD, INPUTEVENT_KEY_PERIOD },
	{ SDL_SCANCODE_SLASH, INPUTEVENT_KEY_DIV },

	{ SDL_SCANCODE_NONUSBACKSLASH, INPUTEVENT_KEY_30 },
	{ SDL_SCANCODE_F11, INPUTEVENT_KEY_BACKSLASH },
	{ SDL_SCANCODE_F13, INPUTEVENT_KEY_BACKSLASH },
	{ SDL_SCANCODE_F14, INPUTEVENT_KEY_NP_LPAREN },
	{ SDL_SCANCODE_F15, INPUTEVENT_KEY_NP_RPAREN },

	{ SDL_SCANCODE_SYSREQ, INPUTEVENT_SPC_SCREENSHOT_CLIPBOARD, 0, INPUTEVENT_SPC_SCREENSHOT, ID_FLAG_QUALIFIER_SPECIAL },

	{ SDL_SCANCODE_END, INPUTEVENT_SPC_QUALIFIER_SPECIAL },
	{ SDL_SCANCODE_PAUSE, INPUTEVENT_SPC_PAUSE, 0, INPUTEVENT_SPC_SINGLESTEP, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_CONTROL, INPUTEVENT_SPC_IRQ7, ID_FLAG_QUALIFIER_SPECIAL | ID_FLAG_QUALIFIER_SHIFT, INPUTEVENT_SPC_WARP, ID_FLAG_QUALIFIER_SPECIAL },

	{ SDL_SCANCODE_F12, INPUTEVENT_SPC_ENTERGUI, 0, INPUTEVENT_SPC_ENTERDEBUGGER, ID_FLAG_QUALIFIER_SPECIAL, INPUTEVENT_SPC_ENTERDEBUGGER, ID_FLAG_QUALIFIER_SHIFT, INPUTEVENT_SPC_TOGGLEDEFAULTSCREEN, ID_FLAG_QUALIFIER_CONTROL },

	{ SDL_SCANCODE_AUDIOSTOP, INPUTEVENT_KEY_CDTV_STOP },
	{ SDL_SCANCODE_AUDIOPLAY, INPUTEVENT_KEY_CDTV_PLAYPAUSE },
	{ SDL_SCANCODE_AUDIOPREV, INPUTEVENT_KEY_CDTV_PREV },
	{ SDL_SCANCODE_AUDIONEXT, INPUTEVENT_KEY_CDTV_NEXT },

	{ -1, 0 }
};

static struct uae_input_device_kbr_default keytrans_pc1[] = {

	{ SDL_SCANCODE_ESCAPE, INPUTEVENT_KEY_ESC },

	{ SDL_SCANCODE_F1, INPUTEVENT_KEY_F1 },
	{ SDL_SCANCODE_F2, INPUTEVENT_KEY_F2 },
	{ SDL_SCANCODE_F3, INPUTEVENT_KEY_F3 },
	{ SDL_SCANCODE_F4, INPUTEVENT_KEY_F4 },
	{ SDL_SCANCODE_F5, INPUTEVENT_KEY_F5 },
	{ SDL_SCANCODE_F6, INPUTEVENT_KEY_F6 },
	{ SDL_SCANCODE_F7, INPUTEVENT_KEY_F7 },
	{ SDL_SCANCODE_F8, INPUTEVENT_KEY_F8 },
	{ SDL_SCANCODE_F9, INPUTEVENT_KEY_F9 },
	{ SDL_SCANCODE_F10, INPUTEVENT_KEY_F10 },
	{ SDL_SCANCODE_F11, INPUTEVENT_KEY_F11 },
	{ SDL_SCANCODE_F12, INPUTEVENT_KEY_F12 },

	{ SDL_SCANCODE_1, INPUTEVENT_KEY_1 },
	{ SDL_SCANCODE_2, INPUTEVENT_KEY_2 },
	{ SDL_SCANCODE_3, INPUTEVENT_KEY_3 },
	{ SDL_SCANCODE_4, INPUTEVENT_KEY_4 },
	{ SDL_SCANCODE_5, INPUTEVENT_KEY_5 },
	{ SDL_SCANCODE_6, INPUTEVENT_KEY_6 },
	{ SDL_SCANCODE_7, INPUTEVENT_KEY_7 },
	{ SDL_SCANCODE_8, INPUTEVENT_KEY_8 },
	{ SDL_SCANCODE_9, INPUTEVENT_KEY_9 },
	{ SDL_SCANCODE_0, INPUTEVENT_KEY_0 },

	{ SDL_SCANCODE_TAB, INPUTEVENT_KEY_TAB },

	{ SDL_SCANCODE_A, INPUTEVENT_KEY_A },
	{ SDL_SCANCODE_B, INPUTEVENT_KEY_B },
	{ SDL_SCANCODE_C, INPUTEVENT_KEY_C },
	{ SDL_SCANCODE_D, INPUTEVENT_KEY_D },
	{ SDL_SCANCODE_E, INPUTEVENT_KEY_E },
	{ SDL_SCANCODE_F, INPUTEVENT_KEY_F },
	{ SDL_SCANCODE_G, INPUTEVENT_KEY_G },
	{ SDL_SCANCODE_H, INPUTEVENT_KEY_H },
	{ SDL_SCANCODE_I, INPUTEVENT_KEY_I },
	{ SDL_SCANCODE_J, INPUTEVENT_KEY_J },
	{ SDL_SCANCODE_K, INPUTEVENT_KEY_K },
	{ SDL_SCANCODE_L, INPUTEVENT_KEY_L },
	{ SDL_SCANCODE_M, INPUTEVENT_KEY_M },
	{ SDL_SCANCODE_N, INPUTEVENT_KEY_N },
	{ SDL_SCANCODE_O, INPUTEVENT_KEY_O },
	{ SDL_SCANCODE_P, INPUTEVENT_KEY_P },
	{ SDL_SCANCODE_Q, INPUTEVENT_KEY_Q },
	{ SDL_SCANCODE_R, INPUTEVENT_KEY_R },
	{ SDL_SCANCODE_S, INPUTEVENT_KEY_S },
	{ SDL_SCANCODE_T, INPUTEVENT_KEY_T },
	{ SDL_SCANCODE_U, INPUTEVENT_KEY_U },
	{ SDL_SCANCODE_W, INPUTEVENT_KEY_W },
	{ SDL_SCANCODE_V, INPUTEVENT_KEY_V },
	{ SDL_SCANCODE_X, INPUTEVENT_KEY_X },
	{ SDL_SCANCODE_Y, INPUTEVENT_KEY_Y },
	{ SDL_SCANCODE_Z, INPUTEVENT_KEY_Z },

	{ SDL_SCANCODE_CAPSLOCK, INPUTEVENT_KEY_CAPS_LOCK, ID_FLAG_TOGGLE },

	{ SDL_SCANCODE_KP_1, INPUTEVENT_KEY_NP_1 },
	{ SDL_SCANCODE_KP_2, INPUTEVENT_KEY_NP_2 },
	{ SDL_SCANCODE_KP_3, INPUTEVENT_KEY_NP_3 },
	{ SDL_SCANCODE_KP_4, INPUTEVENT_KEY_NP_4 },
	{ SDL_SCANCODE_KP_5, INPUTEVENT_KEY_NP_5 },
	{ SDL_SCANCODE_KP_6, INPUTEVENT_KEY_NP_6 },
	{ SDL_SCANCODE_KP_7, INPUTEVENT_KEY_NP_7 },
	{ SDL_SCANCODE_KP_8, INPUTEVENT_KEY_NP_8 },
	{ SDL_SCANCODE_KP_9, INPUTEVENT_KEY_NP_9 },
	{ SDL_SCANCODE_KP_0, INPUTEVENT_KEY_NP_0 },
	{ SDL_SCANCODE_KP_PERIOD, INPUTEVENT_KEY_NP_PERIOD },
	{ SDL_SCANCODE_KP_PLUS, INPUTEVENT_KEY_NP_ADD },
	{ SDL_SCANCODE_KP_MINUS, INPUTEVENT_KEY_NP_SUB },
	{ SDL_SCANCODE_KP_MULTIPLY, INPUTEVENT_KEY_NP_MUL },
	{ SDL_SCANCODE_KP_DIVIDE, INPUTEVENT_KEY_NP_DIV },
	{ SDL_SCANCODE_KP_ENTER, INPUTEVENT_KEY_ENTER },

	{ SDL_SCANCODE_MINUS, INPUTEVENT_KEY_SUB },
	{ SDL_SCANCODE_EQUALS, INPUTEVENT_KEY_EQUALS },
	{ SDL_SCANCODE_BACKSPACE, INPUTEVENT_KEY_BACKSPACE },
	{ SDL_SCANCODE_RETURN, INPUTEVENT_KEY_RETURN },
	{ SDL_SCANCODE_SPACE, INPUTEVENT_KEY_SPACE },

	{ SDL_SCANCODE_LSHIFT, INPUTEVENT_KEY_SHIFT_LEFT },
	{ SDL_SCANCODE_LCTRL, INPUTEVENT_KEY_CTRL },
	{ SDL_SCANCODE_LGUI, INPUTEVENT_KEY_AMIGA_LEFT },
	{ SDL_SCANCODE_LALT, INPUTEVENT_KEY_ALT_LEFT },
	{ SDL_SCANCODE_RALT, INPUTEVENT_KEY_ALT_RIGHT },
	{ SDL_SCANCODE_RGUI, INPUTEVENT_KEY_AMIGA_RIGHT },
	{ SDL_SCANCODE_APPLICATION, INPUTEVENT_KEY_APPS },
	{ SDL_SCANCODE_RCTRL, INPUTEVENT_KEY_CTRL },
	{ SDL_SCANCODE_RSHIFT, INPUTEVENT_KEY_SHIFT_RIGHT },

	{ SDL_SCANCODE_UP, INPUTEVENT_KEY_CURSOR_UP },
	{ SDL_SCANCODE_DOWN, INPUTEVENT_KEY_CURSOR_DOWN },
	{ SDL_SCANCODE_LEFT, INPUTEVENT_KEY_CURSOR_LEFT },
	{ SDL_SCANCODE_RIGHT, INPUTEVENT_KEY_CURSOR_RIGHT },

	{ SDL_SCANCODE_LEFTBRACKET, INPUTEVENT_KEY_LEFTBRACKET },
	{ SDL_SCANCODE_RIGHTBRACKET, INPUTEVENT_KEY_RIGHTBRACKET },
	{ SDL_SCANCODE_SEMICOLON, INPUTEVENT_KEY_SEMICOLON },
	{ SDL_SCANCODE_APOSTROPHE, INPUTEVENT_KEY_SINGLEQUOTE },
	{ SDL_SCANCODE_GRAVE, INPUTEVENT_KEY_BACKQUOTE },
	{ SDL_SCANCODE_BACKSLASH, INPUTEVENT_KEY_2B },
	{ SDL_SCANCODE_COMMA, INPUTEVENT_KEY_COMMA },
	{ SDL_SCANCODE_PERIOD, INPUTEVENT_KEY_PERIOD },
	{ SDL_SCANCODE_SLASH, INPUTEVENT_KEY_DIV },
	{ SDL_SCANCODE_NONUSBACKSLASH, INPUTEVENT_KEY_30 },

	{ SDL_SCANCODE_INSERT, INPUTEVENT_KEY_INSERT },
	{ SDL_SCANCODE_DELETE, INPUTEVENT_KEY_DEL },
	{ SDL_SCANCODE_HOME, INPUTEVENT_KEY_HOME },
	{ SDL_SCANCODE_END, INPUTEVENT_KEY_END },
	{ SDL_SCANCODE_PAGEUP, INPUTEVENT_KEY_PAGEUP },
	{ SDL_SCANCODE_PAGEDOWN, INPUTEVENT_KEY_PAGEDOWN },
	{ SDL_SCANCODE_SCROLLLOCK, INPUTEVENT_KEY_HELP },
	{ SDL_SCANCODE_SYSREQ, INPUTEVENT_KEY_SYSRQ },

	{ SDL_SCANCODE_AUDIOSTOP, INPUTEVENT_KEY_CDTV_STOP },
	{ SDL_SCANCODE_AUDIOPLAY, INPUTEVENT_KEY_CDTV_PLAYPAUSE },
	{ SDL_SCANCODE_AUDIOPREV, INPUTEVENT_KEY_CDTV_PREV },
	{ SDL_SCANCODE_AUDIONEXT, INPUTEVENT_KEY_CDTV_NEXT },

	{ -1, 0 }
};

static struct uae_input_device_kbr_default *keytrans[] =
{
	keytrans_amiga,
	keytrans_pc1,
	keytrans_pc1
};

static int kb_np[] = { SDL_SCANCODE_KP_4, -1, SDL_SCANCODE_KP_6, -1, SDL_SCANCODE_KP_8, -1, SDL_SCANCODE_KP_2, -1, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_5, -1, SDL_SCANCODE_KP_DECIMAL, -1, SDL_SCANCODE_KP_ENTER, -1, -1 };
static int kb_ck[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_RCTRL, SDL_SCANCODE_RALT, -1, SDL_SCANCODE_RSHIFT, -1, -1 };
static int kb_se[] = { SDL_SCANCODE_A, -1, SDL_SCANCODE_D, -1, SDL_SCANCODE_W, -1, SDL_SCANCODE_S, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LSHIFT, -1, -1 };
static int kb_np3[] = { SDL_SCANCODE_KP_4, -1, SDL_SCANCODE_KP_6, -1, SDL_SCANCODE_KP_8, -1, SDL_SCANCODE_KP_2, -1, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_5, -1, SDL_SCANCODE_KP_DECIMAL, -1, SDL_SCANCODE_KP_ENTER, -1, -1 };
static int kb_ck3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_RCTRL, -1, SDL_SCANCODE_RSHIFT, -1, SDL_SCANCODE_RALT, -1, -1 };
static int kb_se3[] = { SDL_SCANCODE_A, -1, SDL_SCANCODE_D, -1, SDL_SCANCODE_W, -1, SDL_SCANCODE_S, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LSHIFT, -1, SDL_SCANCODE_LCTRL, -1, -1 };

static int kb_cd32_np[] = { SDL_SCANCODE_KP_4, -1, SDL_SCANCODE_KP_6, -1, SDL_SCANCODE_KP_8, -1, SDL_SCANCODE_KP_2, -1, SDL_SCANCODE_KP_0, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_1, -1, SDL_SCANCODE_KP_DECIMAL, SDL_SCANCODE_KP_3, -1, SDL_SCANCODE_KP_7, -1, SDL_SCANCODE_KP_9, -1, SDL_SCANCODE_KP_DIVIDE, -1, SDL_SCANCODE_KP_MINUS, -1, SDL_SCANCODE_KP_MULTIPLY, -1, -1 };
static int kb_cd32_ck[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_RCTRL, -1, SDL_SCANCODE_RALT, -1, SDL_SCANCODE_KP_7, -1, SDL_SCANCODE_KP_9, -1, SDL_SCANCODE_KP_DIVIDE, -1, SDL_SCANCODE_KP_MINUS, -1, SDL_SCANCODE_KP_MULTIPLY, -1, -1 };
static int kb_cd32_se[] = { SDL_SCANCODE_A, -1, SDL_SCANCODE_D, -1, SDL_SCANCODE_W, -1, SDL_SCANCODE_S, -1, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LSHIFT, -1, SDL_SCANCODE_KP_7, -1, SDL_SCANCODE_KP_9, -1, SDL_SCANCODE_KP_DIVIDE, -1, SDL_SCANCODE_KP_MINUS, -1, SDL_SCANCODE_KP_MULTIPLY, -1, -1 };

static int kb_cdtv[] = { SDL_SCANCODE_KP_1, -1, SDL_SCANCODE_KP_3, -1, SDL_SCANCODE_KP_7, -1, SDL_SCANCODE_KP_9, -1, -1 };

static int kb_arcadia[] = { SDL_SCANCODE_F2, -1, SDL_SCANCODE_1, -1, SDL_SCANCODE_2, -1, SDL_SCANCODE_5, -1, SDL_SCANCODE_6, -1, -1 };

#ifdef AMIBERRY
static int kb_keyrah[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_SPACE, SDL_SCANCODE_RALT, -1, SDL_SCANCODE_RSHIFT, -1, -1 };
static int kb_keyrah3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_SPACE, -1, SDL_SCANCODE_RSHIFT, -1, SDL_SCANCODE_RALT, -1, -1 };
static int kb_cd32_keyrah[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_SPACE, -1, SDL_SCANCODE_RALT, -1, SDL_SCANCODE_KP_7, -1, SDL_SCANCODE_KP_9, -1, SDL_SCANCODE_KP_DIVIDE, -1, SDL_SCANCODE_KP_MINUS, -1, SDL_SCANCODE_KP_MULTIPLY, -1, -1 };
int kb_retroarch_player1[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_retroarch_player1_3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_cd32_retroarch_player1[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, SDL_SCANCODE_LSHIFT, -1, SDL_SCANCODE_Z, -1, SDL_SCANCODE_X, -1, SDL_SCANCODE_2, -1, -1 };
int kb_retroarch_player2[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_retroarch_player2_3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_cd32_retroarch_player2[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, SDL_SCANCODE_LSHIFT, -1, SDL_SCANCODE_Z, -1, SDL_SCANCODE_X, -1, SDL_SCANCODE_2, -1, -1 };
int kb_retroarch_player3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_retroarch_player3_3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_cd32_retroarch_player3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, SDL_SCANCODE_LSHIFT, -1, SDL_SCANCODE_Z, -1, SDL_SCANCODE_X, -1, SDL_SCANCODE_2, -1, -1 };
int kb_retroarch_player4[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_retroarch_player4_3[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, -1 };
int kb_cd32_retroarch_player4[] = { SDL_SCANCODE_LEFT, -1, SDL_SCANCODE_RIGHT, -1, SDL_SCANCODE_UP, -1, SDL_SCANCODE_DOWN, -1, SDL_SCANCODE_LALT, -1, SDL_SCANCODE_LCTRL, -1, SDL_SCANCODE_SPACE, -1, SDL_SCANCODE_LSHIFT, -1, SDL_SCANCODE_Z, -1, SDL_SCANCODE_X, -1, SDL_SCANCODE_2, -1, -1 };
#endif

static int* kbmaps[] = {
	kb_np, kb_ck, kb_se, kb_np3, kb_ck3, kb_se3,
	kb_keyrah, kb_keyrah3, kb_retroarch_player1, kb_retroarch_player1_3,
	kb_retroarch_player2, kb_retroarch_player2_3, kb_retroarch_player3, kb_retroarch_player3_3,
	kb_retroarch_player4, kb_retroarch_player4_3,
	kb_cd32_np, kb_cd32_ck, kb_cd32_se,
	kb_cd32_keyrah, kb_cd32_retroarch_player1,
	kb_cd32_retroarch_player2, kb_cd32_retroarch_player3,
	kb_cd32_retroarch_player4,
	kb_arcadia, kb_cdtv
};

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

int getcapslock()
{
	int capstable[7];

	// this returns bogus state if caps change when in exclusive mode...
	host_capslockstate = SDL_GetModState() & KMOD_CAPS;
	host_numlockstate = SDL_GetModState() & KMOD_NUM;
	host_scrolllockstate = 0; //SDL_GetModState() & ;
	capstable[0] = SDL_SCANCODE_CAPSLOCK;
	capstable[1] = host_capslockstate;
	capstable[2] = SDL_SCANCODE_NUMLOCKCLEAR;
	capstable[3] = host_numlockstate;
	capstable[4] = SDL_SCANCODE_SCROLLLOCK;
	capstable[5] = host_scrolllockstate;
	capstable[6] = 0;
	capslockstate = inputdevice_synccapslock(capslockstate, capstable);
	if (currprefs.keyboard_mode == 0) {
		gui_data.capslock = host_capslockstate;
		gui_led(LED_CAPS, gui_data.capslock, -1);
	}
	return capslockstate;
}

void clearallkeys()
{
	inputdevice_updateconfig(&changed_prefs, &currprefs);
}

static constexpr int np[] = {
	SDL_SCANCODE_KP_0, 0, SDL_SCANCODE_KP_PERIOD, 0, SDL_SCANCODE_KP_1, 1, SDL_SCANCODE_KP_2, 2,
	SDL_SCANCODE_KP_3, 3, SDL_SCANCODE_KP_4, 4, SDL_SCANCODE_KP_5, 5, SDL_SCANCODE_KP_6, 6, SDL_SCANCODE_KP_7, 7,
	SDL_SCANCODE_KP_8, 8, SDL_SCANCODE_KP_9, 9, -1 };

bool my_kbd_handler(int keyboard, int scancode, int newstate, bool alwaysrelease)
{
	int code = 0;
	int scancode_new;
	bool amode = currprefs.input_keyboard_type == 0;
	bool special = false;
	static int swapperdrive = 0;

	// Amiberry uses qualifiers for Quit also, so we handle this piece below
#ifndef AMIBERRY
	if (amode && scancode == SDL_SCANCODE_F11 && currprefs.ctrl_F11_is_quit && key_ctrlpressed()) {
		if (!quit_ok())
			return true;
		uae_quit();
		return true;
	}

	if (scancode == SDL_SCANCODE_F9 && key_specialpressed()) {
		extern bool toggle_3d_debug(void);
		if (newstate) {
			if (toggle_3d_debug()) {
				return true;
			}
		}
	}
#endif
	scancode_new = scancode;

	const bool ctrl_state = key_ctrlpressed();
	const bool shift_state = key_shiftpressed();
	const bool alt_state = key_altpressed();
	const bool win_state = key_winpressed();
	const bool special_state = key_specialpressed();

	if (newstate) {
		if (quit_key.scancode && scancode == quit_key.scancode)
		{
			if ((quit_key.modifiers.lctrl || quit_key.modifiers.rctrl) == ctrl_state
				&& (quit_key.modifiers.lshift || quit_key.modifiers.rshift) == shift_state
				&& (quit_key.modifiers.lalt || quit_key.modifiers.ralt) == alt_state
				&& (quit_key.modifiers.lgui || quit_key.modifiers.rgui) == win_state)
			{
				uae_quit();
				return true;
			}
		}

		if (enter_gui_key.scancode && scancode == enter_gui_key.scancode)
		{
			if ((enter_gui_key.modifiers.lctrl || enter_gui_key.modifiers.rctrl) == ctrl_state
				&& (enter_gui_key.modifiers.lshift || enter_gui_key.modifiers.rshift) == shift_state
				&& (enter_gui_key.modifiers.lalt || enter_gui_key.modifiers.ralt) == alt_state
				&& (enter_gui_key.modifiers.lgui || enter_gui_key.modifiers.rgui) == win_state)
			{
				inputdevice_add_inputcode(AKS_ENTERGUI, 1, nullptr);
				scancode = 0;
			}
			
		}

		if (action_replay_key.scancode && scancode == action_replay_key.scancode)
		{
			if ((action_replay_key.modifiers.lctrl || action_replay_key.modifiers.rctrl) == ctrl_state
				&& (action_replay_key.modifiers.lshift || action_replay_key.modifiers.rshift) == shift_state
				&& (action_replay_key.modifiers.lalt || action_replay_key.modifiers.ralt) == alt_state
				&& (action_replay_key.modifiers.lgui || action_replay_key.modifiers.rgui) == win_state)
			{
				inputdevice_add_inputcode(AKS_FREEZEBUTTON, 1, nullptr);
				scancode = 0;
			}
		}

		if (fullscreen_key.scancode && scancode == fullscreen_key.scancode)
		{
			if ((fullscreen_key.modifiers.lctrl || fullscreen_key.modifiers.rctrl) == ctrl_state
				&& (fullscreen_key.modifiers.lshift || fullscreen_key.modifiers.rshift) == shift_state
				&& (fullscreen_key.modifiers.lalt || fullscreen_key.modifiers.ralt) == alt_state
				&& (fullscreen_key.modifiers.lgui || fullscreen_key.modifiers.rgui) == win_state)
			{
				inputdevice_add_inputcode(AKS_TOGGLEWINDOWFULLWINDOW, 1, nullptr);
				scancode = 0;
			}
		}

		if (minimize_key.scancode && scancode == minimize_key.scancode)
		{
			if ((minimize_key.modifiers.lctrl || minimize_key.modifiers.rctrl) == ctrl_state
				&& (minimize_key.modifiers.lshift || minimize_key.modifiers.rshift) == shift_state
				&& (minimize_key.modifiers.lalt || minimize_key.modifiers.ralt) == alt_state
				&& (minimize_key.modifiers.lgui || minimize_key.modifiers.rgui) == win_state)
			{
				minimizewindow(0);
				scancode = 0;
			}
		}

		if (scancode == SDL_SCANCODE_SYSREQ)
			clipboard_disable(true);
	}
	
	if (!special_state && inputdevice_iskeymapped(keyboard, scancode))
		scancode = 0;

	if (newstate && code == 0 && amode) {
		switch (scancode)
		{
		case SDL_SCANCODE_1:
		case SDL_SCANCODE_2:
		case SDL_SCANCODE_3:
		case SDL_SCANCODE_4:
		case SDL_SCANCODE_5:
		case SDL_SCANCODE_6:
		case SDL_SCANCODE_7:
		case SDL_SCANCODE_8:
		case SDL_SCANCODE_9:
		case SDL_SCANCODE_0:
			if (special_state) {
				int num = scancode - SDL_SCANCODE_1;
				if (shift_state)
					num += 10;
				if (ctrl_state) {
					swapperdrive = num;
					if (swapperdrive > 3)
						swapperdrive = 0;
				}
				else {
					for (int i = 0; i < 4; i++) {
						if (!_tcscmp(currprefs.floppyslots[i].df, currprefs.dfxlist[num]))
							changed_prefs.floppyslots[i].df[0] = 0;
					}
					_tcscpy(changed_prefs.floppyslots[swapperdrive].df, currprefs.dfxlist[num]);
					set_config_changed();
				}
				special = true;
			}
			break;
		case SDL_SCANCODE_KP_0:
		case SDL_SCANCODE_KP_1:
		case SDL_SCANCODE_KP_2:
		case SDL_SCANCODE_KP_3:
		case SDL_SCANCODE_KP_4:
		case SDL_SCANCODE_KP_5:
		case SDL_SCANCODE_KP_6:
		case SDL_SCANCODE_KP_7:
		case SDL_SCANCODE_KP_8:
		case SDL_SCANCODE_KP_9:
		case SDL_SCANCODE_KP_PERIOD:
			if (special_state) {
				int i = 0, v = -1;
				while (np[i] >= 0) {
					v = np[i + 1];
					if (np[i] == scancode)
						break;
					i += 2;
				}
				if (v >= 0)
					code = AKS_STATESAVEQUICK + v * 2 + ((shift_state || ctrl_state) ? 0 : 1);
				special = true;
			}
			break;
		default: break;
		}
	}

	if (code) {
		inputdevice_add_inputcode(code, 1, nullptr);
		return true;
	}

	scancode = scancode_new;
	if (!special_state && newstate) {
		if (scancode == SDL_SCANCODE_CAPSLOCK) {
			host_capslockstate = host_capslockstate ? 0 : 1;
			capslockstate = host_capslockstate;
		}
		if (scancode == SDL_SCANCODE_NUMLOCKCLEAR) {
			host_numlockstate = host_numlockstate ? 0 : 1;
			capslockstate = host_numlockstate;
		}
		if (scancode == SDL_SCANCODE_SCROLLLOCK) {
			host_scrolllockstate = host_scrolllockstate ? 0 : 1;
			capslockstate = host_scrolllockstate;
		}
	}

	if (special) {
		inputdevice_checkqualifierkeycode(keyboard, scancode, newstate);
		return true;
	}

	return inputdevice_translatekeycode(keyboard, scancode, newstate, alwaysrelease) != 0;
}

void keyboard_settrans()
{
	inputdevice_setkeytranslation(keytrans, kbmaps);
}

int target_checkcapslock(const int scancode, int *state)
{
	if (scancode != SDL_SCANCODE_CAPSLOCK && scancode != SDL_SCANCODE_NUMLOCKCLEAR && scancode != SDL_SCANCODE_SCROLLLOCK) {
		return 0;
	}
	if (currprefs.keyboard_mode > 0) {
		return 1;
	}
	if (*state == 0) {
		return -1;
	}
	if (scancode == SDL_SCANCODE_CAPSLOCK) {
		*state = host_capslockstate;
		if (gui_data.capslock != (host_capslockstate != 0)) {
			gui_data.capslock = host_capslockstate;
			gui_led(LED_CAPS, gui_data.capslock, -1);
		}
	}
	if (scancode == SDL_SCANCODE_NUMLOCKCLEAR) {
		*state = host_numlockstate;
	}
	if (scancode == SDL_SCANCODE_SCROLLLOCK) {
		*state = host_scrolllockstate;
	}
	return 1;
}
