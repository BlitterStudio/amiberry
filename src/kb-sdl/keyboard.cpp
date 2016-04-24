#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "events.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include <SDL.h>


#define KEYCODE_UNK   0
#define KEYCODE_X11   1
#define KEYCODE_FBCON 2


char keyboard_type = 2;

void  init_keyboard(void)
{
  char vid_drv_name[32];
  // get display type...
  SDL_VideoDriverName(vid_drv_name, sizeof(vid_drv_name));
  if (strcmp(vid_drv_name, "x11") == 0)
  {
    printf("Will use keycode from x11 mapping.\n");
    keyboard_type = KEYCODE_X11;
  } else  if (strcmp(vid_drv_name, "fbcon") == 0)
  {
    printf("Will use keycode from fbcon mapping.\n");
    keyboard_type = KEYCODE_FBCON;
  } else
  {
    printf("Unknown keycode to use, will use keysym\n");
  }
}

/* Decode KeySyms. This function knows about all keys that are common
 * between different keyboard languages. */
static int kc_decode (SDL_keysym *prKeySym)
{

  int scancode = -1;

  if (keyboard_type == KEYCODE_X11)
    scancode = prKeySym->scancode;

  // In fbcon driver, we have only a small shift by 8 compared with X11...
  if (keyboard_type == KEYCODE_FBCON)
    scancode = prKeySym->scancode + 8;

  switch (scancode)
  {
    // Idea taken from linux.c file in fs-uae...

    case 9 : return  AK_ESC;
    case 67: return  AK_F1;
    case 68: return  AK_F2;
    case 69: return  AK_F3;
    case 70: return  AK_F4;
    case 71: return  AK_F5;
    case 72: return  AK_F6;
    case 73: return  AK_F7;
    case 74: return  AK_F8;
    case 75: return  AK_F9;
    case 76: return  AK_F10;
    //case 95: return  AK_F11;
    //case 96: return  AK_F12;

    case 49: return  AK_BACKQUOTE;

    case 10: return AK_1;
    case 11: return AK_2;
    case 12: return AK_3;
    case 13: return AK_4;
    case 14: return AK_5;
    case 15: return AK_6;
    case 16: return AK_7;
    case 17: return AK_8;
    case 18: return AK_9;
    case 19: return AK_0;
    case 20: return AK_MINUS;
    case 21: return AK_EQUAL;
    case 22: return AK_BS;

    case 23: return AK_TAB;
    case 24: return AK_Q;
    case 25: return AK_W;
    case 26: return AK_E;
    case 27: return AK_R;
    case 28: return AK_T;
    case 29: return AK_Y;
    case 30: return AK_U;
    case 31: return AK_I;
    case 32: return AK_O;
    case 33: return AK_P;
    case 34: return AK_LBRACKET;
    case 35: return AK_RBRACKET;
    case 36: return AK_RET;

    case 66: return AK_CAPSLOCK;
    case 38: return AK_A;
    case 39: return AK_S;
    case 40: return AK_D;
    case 41: return AK_F;
    case 42: return AK_G;
    case 43: return AK_H;
    case 44: return AK_J;
    case 45: return AK_K;
    case 46: return AK_L;
    case 47: return AK_SEMICOLON;
    case 48: return AK_QUOTE;
    case 51: return AK_BACKSLASH;

    case 50: return AK_LSH;
    case 94: return AK_LTGT;
    case 52: return AK_Z;
    case 53: return AK_X;
    case 54: return AK_C;
    case 55: return AK_V;
    case 56: return AK_B;
    case 57: return AK_N;
    case 58: return AK_M;
    case 59: return AK_COMMA;
    case 60: return AK_PERIOD;
    case 61: return AK_SLASH;
    case 62: return AK_RSH;

    case 37: return AK_CTRL;
    case 64: return AK_LALT;
    case 65: return AK_SPC;

    //case 78: return AK_SCROLLOCK;

    //case 77: return AK_NUMLOCK;
    case 106:return AK_NPDIV;
    case 63: return AK_NPMUL;
    case 82: return AK_NPSUB;

    case 79: return AK_NP7;
    case 80: return AK_NP8;
    case 81: return AK_NP9;
    case 86: return AK_NPADD;

    case 83: return AK_NP4;
    case 84: return AK_NP5;
    case 85: return AK_NP6;

    case 87: return AK_NP1;
    case 88: return AK_NP2;
    case 89: return AK_NP3;
    case 104:return AK_ENT;   // The ENT from keypad..

    case 90: return AK_NP0;
    case 91: return AK_NPDEL;

    case 0 : return AK_LAMI;  // Left amiga mapped to left Windows
    case 135:return AK_LAMI;  // Right amiga mapped to Menu key.
  }

  // In case are in unknown driver type... we can still rely on keysym...

#ifdef PANDORA
  // Special handling of Pandora keyboard:
  // Some keys requires shift on Amiga, so we simulate shift...

  switch (prKeySym->sym)
  {
    case SDLK_QUESTION:
      return SIMULATE_SHIFT | AK_SLASH;
    case SDLK_HASH:
      return SIMULATE_SHIFT | AK_3;
    case SDLK_DOLLAR:
      return SIMULATE_SHIFT | AK_4;
    case SDLK_QUOTEDBL:
      return SIMULATE_SHIFT | AK_QUOTE;
    case SDLK_PLUS:
      return SIMULATE_SHIFT | AK_EQUAL;
    case SDLK_AT:
      return SIMULATE_SHIFT | AK_2;
    case SDLK_LEFTPAREN:
      return SIMULATE_SHIFT | AK_9;
    case SDLK_RIGHTPAREN:
      return SIMULATE_SHIFT | AK_0;
    case SDLK_EXCLAIM:
      return SIMULATE_SHIFT | AK_1;
    case SDLK_UNDERSCORE:
      return SIMULATE_SHIFT | AK_MINUS;
    case SDLK_2:
      if(prKeySym->mod == KMOD_LSHIFT)
        return SIMULATE_SHIFT | AK_LBRACKET;
      break;
    case SDLK_3:
      if(prKeySym->mod == KMOD_LSHIFT)
        return SIMULATE_SHIFT | AK_RBRACKET;
      break;
    case SDLK_4:
      if(prKeySym->mod == KMOD_LSHIFT)
        return SIMULATE_SHIFT | AK_BACKQUOTE;
      break;
    case SDLK_9:
      if(prKeySym->mod == KMOD_LSHIFT)
        return SIMULATE_RELEASED_SHIFT | AK_LBRACKET;
      break;
    case SDLK_0:
      if(prKeySym->mod == KMOD_LSHIFT)
        return SIMULATE_RELEASED_SHIFT | AK_RBRACKET;
      break;
    case 124: // code for '|'
      return SIMULATE_SHIFT | AK_BACKSLASH;
  }
#endif
    switch (prKeySym->sym)
    {
    case SDLK_b: return AK_B;
    case SDLK_c: return AK_C;
    case SDLK_d: return AK_D;
    case SDLK_e: return AK_E;
    case SDLK_f: return AK_F;
    case SDLK_g: return AK_G;
    case SDLK_h: return AK_H;
    case SDLK_i: return AK_I;
    case SDLK_j: return AK_J;
    case SDLK_k: return AK_K;
    case SDLK_l: return AK_L;
    case SDLK_n: return AK_N;
    case SDLK_o: return AK_O;
    case SDLK_p: return AK_P;
    case SDLK_r: return AK_R;
    case SDLK_s: return AK_S;
    case SDLK_t: return AK_T;
    case SDLK_u: return AK_U;
    case SDLK_v: return AK_V;
    case SDLK_x: return AK_X;

    case SDLK_0: return AK_0;
    case SDLK_1: return AK_1;
    case SDLK_2: return AK_2;
    case SDLK_3: return AK_3;
    case SDLK_4: return AK_4;
    case SDLK_5: return AK_5;
    case SDLK_6: return AK_6;
    case SDLK_7: return AK_7;
    case SDLK_8: return AK_8;
    case SDLK_9: return AK_9;

    case SDLK_KP0: return AK_NP0;
    case SDLK_KP1: return AK_NP1;
    case SDLK_KP2: return AK_NP2;
    case SDLK_KP3: return AK_NP3;
    case SDLK_KP4: return AK_NP4;
    case SDLK_KP5: return AK_NP5;
    case SDLK_KP6: return AK_NP6;
    case SDLK_KP7: return AK_NP7;
    case SDLK_KP8: return AK_NP8;
    case SDLK_KP9: return AK_NP9;
    case SDLK_KP_DIVIDE: return AK_NPDIV;
    case SDLK_KP_MULTIPLY: return AK_NPMUL;
    case SDLK_KP_MINUS: return AK_NPSUB;
    case SDLK_KP_PLUS: return AK_NPADD;
    case SDLK_KP_PERIOD: return AK_NPDEL;
    case SDLK_KP_ENTER: return AK_RET;

    case SDLK_F1: return AK_F1;
    case SDLK_F2: return AK_F2;
    case SDLK_F3: return AK_F3;
    case SDLK_F4: return AK_F4;
    case SDLK_F5: return AK_F5;
    case SDLK_F6: return AK_F6;
    case SDLK_F7: return AK_F7;
    case SDLK_F8: return AK_F8;
    case SDLK_F9: return AK_F9;
    case SDLK_F10: return AK_F10;

    case SDLK_BACKSPACE: return AK_BS;
    case SDLK_DELETE: return AK_DEL;
    case SDLK_LCTRL: return AK_CTRL;
    case SDLK_RCTRL: return AK_RCTRL;
    case SDLK_TAB: return AK_TAB;
    case SDLK_LALT: return AK_LALT;
    case SDLK_RALT: return AK_RALT;
    case SDLK_RMETA: return AK_RAMI;
    case SDLK_LMETA: return AK_LAMI;
    case SDLK_RETURN: return AK_RET;
    case SDLK_SPACE: return AK_SPC;
    case SDLK_LSHIFT: return AK_LSH;
    case SDLK_RSHIFT: return AK_RSH;
    case SDLK_ESCAPE: return AK_ESC;

    case SDLK_INSERT: return AK_HELP;
    case SDLK_HOME: return AK_NPLPAREN;
    case SDLK_END: return AK_NPRPAREN;
    case SDLK_CAPSLOCK: return AK_CAPSLOCK;

    case SDLK_UP: return AK_UP;
    case SDLK_DOWN: return AK_DN;
    case SDLK_LEFT: return AK_LF;
    case SDLK_RIGHT: return AK_RT;

    case SDLK_PAGEUP: return AK_RAMI;          /* PgUp mapped to right amiga */
    case SDLK_PAGEDOWN: return AK_LAMI;        /* PgDn mapped to left amiga */

    default: return -1;
    }
}

/*
 * Handle keys specific to French (and Belgian) keymaps.
 *
 * Number keys are broken
 */
static int decode_fr (SDL_keysym *prKeySym)
{
    switch(prKeySym->sym) {
	case SDLK_a:		return AK_Q;
	case SDLK_m:		return AK_SEMICOLON;
	case SDLK_q:		return AK_A;
	case SDLK_y:		return AK_Y;
	case SDLK_w:		return AK_Z;
	case SDLK_z:		return AK_W;
	case SDLK_LEFTBRACKET:	return AK_LBRACKET;
	case SDLK_RIGHTBRACKET: return AK_RBRACKET;
	case SDLK_COMMA:	return AK_M;
	case SDLK_LESS:
	case SDLK_GREATER:	return AK_LTGT;
	case SDLK_PERIOD:
	case SDLK_SEMICOLON:	return AK_COMMA;
	case SDLK_RIGHTPAREN:	return AK_MINUS;
	case SDLK_EQUALS:	return AK_SLASH;
	case SDLK_HASH:		return AK_NUMBERSIGN;
	case SDLK_SLASH:	return AK_PERIOD;      // Is it true ?
	case SDLK_MINUS:	return AK_EQUAL;
	case SDLK_BACKSLASH:	return AK_SLASH;
	case SDLK_COLON:	return AK_PERIOD;
	case SDLK_EXCLAIM:	return AK_BACKSLASH;   // Not really...
	default: printf("Unknown key: %i\n",prKeySym->sym); return -1;
    }
}

/*
 * Handle keys specific to German keymaps.
 */
static int decode_de (SDL_keysym *prKeySym)
{
    switch(prKeySym->sym) {
	case SDLK_a:		return AK_A;
	case SDLK_m:		return AK_M;
	case SDLK_q:		return AK_Q;
	case SDLK_w:		return AK_W;
	case SDLK_y:		return AK_Z;
	case SDLK_z:		return AK_Y;
	case SDLK_COLON:	return SIMULATE_SHIFT | AK_SEMICOLON;
	/* German umlaut oe */
	case SDLK_WORLD_86:	return AK_SEMICOLON;
	/* German umlaut ae */
	case SDLK_WORLD_68:	return AK_QUOTE;
	/* German umlaut ue */
	case SDLK_WORLD_92:	return AK_LBRACKET;
	case SDLK_PLUS:
	case SDLK_ASTERISK:	return AK_RBRACKET;
	case SDLK_COMMA:	return AK_COMMA;
	case SDLK_PERIOD:	return AK_PERIOD;
	case SDLK_LESS:
	case SDLK_GREATER:	return AK_LTGT;
	case SDLK_HASH:		return AK_NUMBERSIGN;
	/* German sharp s */
	case SDLK_WORLD_63:	return AK_MINUS;
	case SDLK_QUOTE:	return AK_EQUAL;
	case SDLK_CARET:	return AK_BACKQUOTE;
	case SDLK_MINUS:	return AK_SLASH;
	default: return -1;
    }
}


static int decode_us (SDL_keysym *prKeySym)
{
    switch(prKeySym->sym)
    {
	/* US specific */
    case SDLK_a: return AK_A;
    case SDLK_m: return AK_M;
    case SDLK_q: return AK_Q;
    case SDLK_y: return AK_Y;
    case SDLK_w: return AK_W;
    case SDLK_z: return AK_Z;
    case SDLK_COLON:   return SIMULATE_SHIFT | AK_SEMICOLON;
    case SDLK_LEFTBRACKET: return AK_LBRACKET;
    case SDLK_RIGHTBRACKET: return AK_RBRACKET;
    case SDLK_COMMA: return AK_COMMA;
    case SDLK_PERIOD: return AK_PERIOD;
    case SDLK_SLASH: return AK_SLASH;
    case SDLK_SEMICOLON: return AK_SEMICOLON;
    case SDLK_MINUS: return AK_MINUS;
    case SDLK_EQUALS: return AK_EQUAL;
	/* this doesn't work: */
    case SDLK_BACKQUOTE: return AK_QUOTE;
    case SDLK_QUOTE: return AK_BACKQUOTE;
    case SDLK_BACKSLASH: return AK_BACKSLASH;
    default: return -1;
    }
}

int keycode2amiga(SDL_keysym *prKeySym)
{
  int iAmigaKeycode = kc_decode(prKeySym);
  if (iAmigaKeycode == -1)
    return decode_us(prKeySym);
  return iAmigaKeycode;
}

int getcapslockstate (void)
{
    return 0;
}

void setcapslockstate (int state)
{
}
