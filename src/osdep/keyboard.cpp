#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "inputdevice.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include "SDL.h"

char keyboard_type = 0;

static struct uae_input_device_kbr_default keytrans_amiga[] = {

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
    { SDLK_F11, INPUTEVENT_KEY_F11},
    { SDLK_F12, INPUTEVENT_KEY_F12},
	
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

	{ SDLK_0, INPUTEVENT_KEY_0 },
	{ SDLK_1, INPUTEVENT_KEY_1 },
	{ SDLK_2, INPUTEVENT_KEY_2 },
	{ SDLK_3, INPUTEVENT_KEY_3 },
	{ SDLK_4, INPUTEVENT_KEY_4 },
	{ SDLK_5, INPUTEVENT_KEY_5 },
	{ SDLK_6, INPUTEVENT_KEY_6 },
	{ SDLK_7, INPUTEVENT_KEY_7 },
	{ SDLK_8, INPUTEVENT_KEY_8 },
	{ SDLK_9, INPUTEVENT_KEY_9 },

	{ SDLK_CAPSLOCK, INPUTEVENT_KEY_CAPS_LOCK },
	{ SDLK_BACKSPACE, INPUTEVENT_KEY_BACKSPACE },
	{ SDLK_TAB, INPUTEVENT_KEY_TAB },
	{ SDLK_RETURN, INPUTEVENT_KEY_RETURN },
	{ VK_ESCAPE, INPUTEVENT_KEY_ESC },
	{ SDLK_SPACE, INPUTEVENT_KEY_SPACE },
	{ SDLK_QUOTE, INPUTEVENT_KEY_SINGLEQUOTE },
	{ SDLK_COMMA, INPUTEVENT_KEY_COMMA },
	{ SDLK_MINUS, INPUTEVENT_KEY_SUB },
	{ SDLK_PERIOD, INPUTEVENT_KEY_PERIOD },
	{ SDLK_SLASH, INPUTEVENT_KEY_DIV },

	{ SDLK_SEMICOLON, INPUTEVENT_KEY_SEMICOLON },
	{ SDLK_EQUALS, INPUTEVENT_KEY_EQUALS },
	{ SDLK_LEFTBRACKET, INPUTEVENT_KEY_LEFTBRACKET },
	{ SDLK_BACKSLASH, INPUTEVENT_KEY_BACKSLASH },
	{ SDLK_RIGHTBRACKET, INPUTEVENT_KEY_RIGHTBRACKET },
	{ SDLK_BACKQUOTE, INPUTEVENT_KEY_BACKQUOTE },
	{ SDLK_DELETE, INPUTEVENT_KEY_DEL },

	{SDLK_LSHIFT, INPUTEVENT_KEY_SHIFT_LEFT	},
	{SDLK_RSHIFT, INPUTEVENT_KEY_SHIFT_RIGHT},
	{SDLK_LCTRL, INPUTEVENT_KEY_CTRL},
	{SDLK_RCTRL, INPUTEVENT_KEY_CTRL},
	{SDLK_LALT, INPUTEVENT_KEY_ALT_LEFT},
	{SDLK_RALT, INPUTEVENT_KEY_ALT_RIGHT},
	
//	{ 94 - 8, INPUTEVENT_KEY_LTGT },

	{ SDLK_KP_DIVIDE, INPUTEVENT_KEY_NP_DIV },
	{ SDLK_KP_MULTIPLY, INPUTEVENT_KEY_NP_MUL },
	{ SDLK_KP_MINUS, INPUTEVENT_KEY_NP_SUB },
	{ SDLK_KP_7, INPUTEVENT_KEY_NP_7 },
	{ SDLK_KP_8, INPUTEVENT_KEY_NP_8 },
	{ SDLK_KP_9, INPUTEVENT_KEY_NP_9 },
	{ SDLK_KP_PLUS, INPUTEVENT_KEY_NP_ADD },
	{ SDLK_KP_4, INPUTEVENT_KEY_NP_4 },
	{ SDLK_KP_5, INPUTEVENT_KEY_NP_5 },
	{ SDLK_KP_6, INPUTEVENT_KEY_NP_6 },
	{ SDLK_KP_1, INPUTEVENT_KEY_NP_1 },
	{ SDLK_KP_2, INPUTEVENT_KEY_NP_2 },
	{ SDLK_KP_3, INPUTEVENT_KEY_NP_3 },
	{ SDLK_KP_ENTER, INPUTEVENT_KEY_ENTER },         // The ENT from keypad..
	{ SDLK_KP_0, INPUTEVENT_KEY_NP_0 },
	{ SDLK_KP_PERIOD, INPUTEVENT_KEY_PERIOD },

	{ SDLK_UP, INPUTEVENT_KEY_CURSOR_UP },
	{ SDLK_LEFT, INPUTEVENT_KEY_CURSOR_LEFT },
	{ SDLK_DOWN, INPUTEVENT_KEY_CURSOR_DOWN },
	{ SDLK_RIGHT, INPUTEVENT_KEY_CURSOR_RIGHT },

	{ SDLK_HOME, INPUTEVENT_KEY_NP_LPAREN },     // Map home   to left  parent (as fsuae)
	{ SDLK_PAGEUP, INPUTEVENT_KEY_NP_RPAREN },     // Map pageup to right parent (as fsuae)
	{ SDLK_END, INPUTEVENT_KEY_HELP },          // Help mapped to End key (as fsuae)
	{ SDLK_DELETE, INPUTEVENT_KEY_DEL },

	{ SDLK_LGUI, INPUTEVENT_KEY_AMIGA_LEFT },   // Left amiga mapped to left Windows
	{ SDLK_RGUI, INPUTEVENT_KEY_AMIGA_RIGHT },  // Right amiga mapped to right windows key.
	{ SDLK_MENU, INPUTEVENT_KEY_AMIGA_RIGHT },  // Right amiga mapped to Menu key.
	{ -1, 0 }
};

static struct uae_input_device_kbr_default *keytrans[] =
{
    keytrans_amiga,
    keytrans_amiga,
    keytrans_amiga
};

static int kb_none[] = { -1 };
static int *kbmaps[] = { kb_none, kb_none, kb_none, kb_none, kb_none,
                         kb_none, kb_none, kb_none, kb_none, kb_none
                       };

void keyboard_settrans ()
{
    keyboard_type = KEYCODE_UNK;
    inputdevice_setkeytranslation (keytrans, kbmaps);
}

int translate_amiberry_keys(int symbol, int *modifier)
{
    switch(symbol)
    {
    case VK_UP:
		*modifier = KMOD_NONE;
        return AK_UP;

    case VK_DOWN:
        *modifier = KMOD_NONE;
        return AK_DN;

    case VK_LEFT:
        *modifier = KMOD_NONE;
        return AK_LF;

    case VK_RIGHT:
        *modifier = KMOD_NONE;
        return AK_RT;

    case SDLK_F1:
        *modifier = KMOD_NONE;
        return AK_F1;

    case SDLK_F2:
        *modifier = KMOD_NONE;
        return AK_F2;

    case SDLK_F3:
        *modifier = KMOD_NONE;
        return AK_F3;

    case SDLK_F4:
        *modifier = KMOD_NONE;
        return AK_F4;

    case SDLK_F5:
        *modifier = KMOD_NONE;
        return AK_F5;

    case SDLK_F6:
        *modifier = KMOD_NONE;
        return AK_F6;

    case SDLK_F7:
        *modifier = KMOD_NONE;
        return AK_F7;

    case SDLK_F8:
        *modifier = KMOD_NONE;
        return AK_F8;

    case SDLK_F9:
        *modifier = KMOD_NONE;
        return AK_F9;

    case SDLK_F10:
        *modifier = KMOD_NONE;
        return AK_F10;

    case SDLK_EXCLAIM:
        *modifier = KMOD_SHIFT;
        return AK_1;

    case SDLK_QUOTEDBL:
        *modifier = KMOD_SHIFT;
        return AK_QUOTE;

    case SDLK_HASH:
        *modifier = KMOD_SHIFT;
        return AK_3;

    case SDLK_DOLLAR:
        *modifier = KMOD_SHIFT;
        return AK_4;

    case SDLK_AMPERSAND:
        *modifier = KMOD_SHIFT;
        return AK_7;

    case SDLK_LEFTPAREN:
        *modifier = KMOD_SHIFT;
        return AK_9;

    case SDLK_RIGHTPAREN:
        *modifier = KMOD_SHIFT;
        return AK_0;

    case SDLK_ASTERISK:
        *modifier = KMOD_SHIFT;
        return AK_8;

    case SDLK_PLUS:
        *modifier = KMOD_SHIFT;
        return AK_EQUAL;

    case SDLK_COLON:
        *modifier = KMOD_SHIFT;
        return AK_SEMICOLON;

    case SDLK_QUESTION:
        *modifier = KMOD_SHIFT;
        return AK_SLASH;

    case SDLK_AT:
        *modifier = KMOD_SHIFT;
        return AK_2;

    case SDLK_CARET:
        *modifier = KMOD_SHIFT;
        return AK_6;

    case SDLK_UNDERSCORE:
        *modifier = KMOD_SHIFT;
        return AK_MINUS;

    case 124: // code for '|'
        *modifier = KMOD_SHIFT;
        return AK_BACKSLASH;

    case SDLK_2:
        if(*modifier == KMOD_LSHIFT)   // '{'
        {
            *modifier = KMOD_SHIFT;
            return AK_LBRACKET;
        }
        break;

    case SDLK_3:
        if(*modifier == KMOD_LSHIFT)   // '}'
        {
            *modifier = KMOD_SHIFT;
            return AK_RBRACKET;
        }
        break;

    case SDLK_4:
        if(*modifier == KMOD_LSHIFT)   // '~'
        {
            *modifier = KMOD_SHIFT;
            return AK_BACKQUOTE;
        }
        break;

    case SDLK_9:
        if(*modifier == KMOD_LSHIFT)   // '['
        {
            *modifier = KMOD_NONE;
            return AK_LBRACKET;
        }
        break;

    case SDLK_0:
        if(*modifier == KMOD_LSHIFT)   // ']'
        {
            *modifier = KMOD_NONE;
            return AK_RBRACKET;
        }
        break;
    }
    return 0;
}
