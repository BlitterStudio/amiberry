#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
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
	{ SDLK_v, INPUTEVENT_KEY_W },
	{ SDLK_w, INPUTEVENT_KEY_V },
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

  { SDLK_BACKSPACE, INPUTEVENT_KEY_BACKSPACE },
	{ SDLK_TAB, INPUTEVENT_KEY_TAB },
	{ SDLK_RETURN, INPUTEVENT_KEY_RETURN },
	{ SDLK_ESCAPE, INPUTEVENT_KEY_ESC },
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

  { -1, 0 }
};

static struct uae_input_device_kbr_default *keytrans[] = {
	keytrans_amiga,
	keytrans_amiga,
	keytrans_amiga
};

static int kb_none[] = { -1 };
static int *kbmaps[] = { kb_none, kb_none, kb_none, kb_none, kb_none,
        kb_none, kb_none, kb_none, kb_none, kb_none };


void keyboard_settrans (void)
{
	inputdevice_setkeytranslation (keytrans, kbmaps);
}


int translate_pandora_keys(int symbol, int *modifier)
{
  switch(symbol)
  {
    case SDLK_UP:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + dPad -> cursor keys
        *modifier = KMOD_NONE;
        return AK_UP;
      }
      break;
      
    case SDLK_DOWN:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + dPad -> cursor keys
        *modifier = KMOD_NONE;
        return AK_DN;
      }
      break;

    case SDLK_LEFT:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + dPad -> cursor keys
        *modifier = KMOD_NONE;
        return AK_LF;
      }
      break;

    case SDLK_RIGHT:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + dPad -> cursor keys
        *modifier = KMOD_NONE;
        return AK_RT;
      }
      break;

    case SDLK_HOME:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + button A -> CTRL
        *modifier = KMOD_NONE;
        return AK_CTRL;
      }
      break;

    case SDLK_END:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + button B -> left ALT
        *modifier = KMOD_NONE;
        return AK_LALT;
      }
      break;

    case SDLK_PAGEDOWN:
      if(*modifier == KMOD_RCTRL) { // Right shoulder + button X -> HELP
        *modifier = KMOD_NONE;
        return AK_HELP;
      }
      break;

    case SDLK_PAGEUP: // button Y -> Space
      *modifier = KMOD_NONE;
      return AK_SPC;

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
      if(*modifier == KMOD_LSHIFT) { // '{'
        *modifier = KMOD_SHIFT;
        return AK_LBRACKET;
      }
      break;
      
    case SDLK_3:
      if(*modifier == KMOD_LSHIFT) { // '}'
        *modifier = KMOD_SHIFT;
        return AK_RBRACKET;
      }
      break;
      
    case SDLK_4:
      if(*modifier == KMOD_LSHIFT) { // '~'
        *modifier = KMOD_SHIFT;
        return AK_BACKQUOTE;
      }
      break;

    case SDLK_9:
      if(*modifier == KMOD_LSHIFT) { // '['
        *modifier = KMOD_NONE;
        return AK_LBRACKET;
      }
      break;

    case SDLK_0:
      if(*modifier == KMOD_LSHIFT) { // ']'
        *modifier = KMOD_NONE;
        return AK_RBRACKET;
      }
      break;
  }
  return 0;
}
