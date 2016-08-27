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


char keyboard_type = 0;

static struct uae_input_device_kbr_default keytrans_amiga_x11[] = {

	{  9 ,   INPUTEVENT_KEY_ESC},
	{  67,   INPUTEVENT_KEY_F1},
	{  68,   INPUTEVENT_KEY_F2},
	{  69,   INPUTEVENT_KEY_F3},
	{  70,   INPUTEVENT_KEY_F4},
	{  71,   INPUTEVENT_KEY_F5},
	{  72,   INPUTEVENT_KEY_F6},
	{  73,   INPUTEVENT_KEY_F7},
	{  74,   INPUTEVENT_KEY_F8},
	{  75,   INPUTEVENT_KEY_F9},
	{  76,   INPUTEVENT_KEY_F10},
    //{ 95,   INPUTEVENT_KEY_F11},
    //{ 96,   INPUTEVENT_KEY_F12},

	{  49,   INPUTEVENT_KEY_BACKQUOTE},

	{  10,  INPUTEVENT_KEY_1},
	{  11,  INPUTEVENT_KEY_2},
	{  12,  INPUTEVENT_KEY_3},
	{  13,  INPUTEVENT_KEY_4},
	{  14,  INPUTEVENT_KEY_5},
	{  15,  INPUTEVENT_KEY_6},
	{  16,  INPUTEVENT_KEY_7},
	{  17,  INPUTEVENT_KEY_8},
	{  18,  INPUTEVENT_KEY_9},
	{  19,  INPUTEVENT_KEY_0},
	{  20,  INPUTEVENT_KEY_SUB},
	{  21,  INPUTEVENT_KEY_EQUALS},
	{  22,  INPUTEVENT_KEY_BACKSPACE},

	{  23,  INPUTEVENT_KEY_TAB},
	{  24,  INPUTEVENT_KEY_Q},
	{  25,  INPUTEVENT_KEY_W},
	{  26,  INPUTEVENT_KEY_E},
	{  27,  INPUTEVENT_KEY_R},
	{  28,  INPUTEVENT_KEY_T},
	{  29,  INPUTEVENT_KEY_Y},
	{  30,  INPUTEVENT_KEY_U},
	{  31,  INPUTEVENT_KEY_I},
	{  32,  INPUTEVENT_KEY_O},
	{  33,  INPUTEVENT_KEY_P},
	{  34,  INPUTEVENT_KEY_LEFTBRACKET},
	{  35,  INPUTEVENT_KEY_RIGHTBRACKET},
	{  36,  INPUTEVENT_KEY_RETURN},

	{  66,  INPUTEVENT_KEY_CAPS_LOCK},
	{  38,  INPUTEVENT_KEY_A},
	{  39,  INPUTEVENT_KEY_S},
	{  40,  INPUTEVENT_KEY_D},
	{  41,  INPUTEVENT_KEY_F},
	{  42,  INPUTEVENT_KEY_G},
	{  43,  INPUTEVENT_KEY_H},
	{  44,  INPUTEVENT_KEY_J},
	{  45,  INPUTEVENT_KEY_K},
	{  46,  INPUTEVENT_KEY_L},
	{  47,  INPUTEVENT_KEY_SEMICOLON},
	{  48,  INPUTEVENT_KEY_SINGLEQUOTE},
	{  51,  INPUTEVENT_KEY_BACKSLASH},

	{  50,  INPUTEVENT_KEY_SHIFT_LEFT},
	{  94,  INPUTEVENT_KEY_LTGT},
	{  52,  INPUTEVENT_KEY_Z},
	{  53,  INPUTEVENT_KEY_X},
	{  54,  INPUTEVENT_KEY_C},
	{  55,  INPUTEVENT_KEY_V},
	{  56,  INPUTEVENT_KEY_B},
	{  57,  INPUTEVENT_KEY_N},
	{  58,  INPUTEVENT_KEY_M},
	{  59,  INPUTEVENT_KEY_COMMA},
	{  60,  INPUTEVENT_KEY_PERIOD},
	{  61,  INPUTEVENT_KEY_DIV},
	{  62,  INPUTEVENT_KEY_SHIFT_RIGHT},

	{  37,  INPUTEVENT_KEY_CTRL},
	{  64,  INPUTEVENT_KEY_ALT_LEFT},
	{  65,  INPUTEVENT_KEY_SPACE},

	{  108, INPUTEVENT_KEY_ALT_RIGHT},

    //{ 78,  INPUTEVENT_KEY_SCROLLOCK},

    //{ 77,  INPUTEVENT_KEY_NUMLOCK},
	{  106, INPUTEVENT_KEY_NP_DIV},
	{  63,  INPUTEVENT_KEY_NP_MUL},
	{  82,  INPUTEVENT_KEY_NP_SUB},

	{  79,  INPUTEVENT_KEY_NP_7},
	{  80,  INPUTEVENT_KEY_NP_8},
	{  81,  INPUTEVENT_KEY_NP_9},
	{  86,  INPUTEVENT_KEY_NP_ADD},

	{  83,  INPUTEVENT_KEY_NP_4},
	{  84,  INPUTEVENT_KEY_NP_5},
	{  85,  INPUTEVENT_KEY_NP_6},

	{  87,  INPUTEVENT_KEY_NP_1},
	{  88,  INPUTEVENT_KEY_NP_2},
	{  89,  INPUTEVENT_KEY_NP_3},
	{  104, INPUTEVENT_KEY_ENTER},   // The ENT from keypad..

	{  90,  INPUTEVENT_KEY_NP_0},
	{  91,  INPUTEVENT_KEY_NP_PERIOD},

	{ 111,  INPUTEVENT_KEY_CURSOR_UP},
	{ 113,  INPUTEVENT_KEY_CURSOR_LEFT},
	{ 116,  INPUTEVENT_KEY_CURSOR_DOWN},
	{ 114,  INPUTEVENT_KEY_CURSOR_RIGHT},

	{  133,  INPUTEVENT_KEY_AMIGA_LEFT},   // Left amiga mapped to left Windows
	{  134,  INPUTEVENT_KEY_AMIGA_RIGHT},  // Right amiga mapped to right windows key.
	{  135,  INPUTEVENT_KEY_AMIGA_RIGHT},  // Right amiga mapped to Menu key.
	{ -1, 0 }
  };



static struct uae_input_device_kbr_default keytrans_amiga_fbcon[] = {

	{  9  -8 ,   INPUTEVENT_KEY_ESC},
	{  67 -8 ,   INPUTEVENT_KEY_F1},
	{  68 -8 ,   INPUTEVENT_KEY_F2},
	{  69 -8 ,   INPUTEVENT_KEY_F3},
	{  70 -8 ,   INPUTEVENT_KEY_F4},
	{  71 -8 ,   INPUTEVENT_KEY_F5},
	{  72 -8 ,   INPUTEVENT_KEY_F6},
	{  73 -8 ,   INPUTEVENT_KEY_F7},
	{  74 -8 ,   INPUTEVENT_KEY_F8},
	{  75 -8 ,   INPUTEVENT_KEY_F9},
	{  76 -8 ,   INPUTEVENT_KEY_F10},
    // { 95 -8 ,   INPUTEVENT_KEY_F11},
    // { 96 -8 ,   INPUTEVENT_KEY_F12},

	{  49 -8 ,   INPUTEVENT_KEY_BACKQUOTE},

	{  10 -8 ,  INPUTEVENT_KEY_1},
	{  11 -8 ,  INPUTEVENT_KEY_2},
	{  12 -8 ,  INPUTEVENT_KEY_3},
	{  13 -8 ,  INPUTEVENT_KEY_4},
	{  14 -8 ,  INPUTEVENT_KEY_5},
	{  15 -8 ,  INPUTEVENT_KEY_6},
	{  16 -8 ,  INPUTEVENT_KEY_7},
	{  17 -8 ,  INPUTEVENT_KEY_8},
	{  18 -8 ,  INPUTEVENT_KEY_9},
	{  19 -8 ,  INPUTEVENT_KEY_0},
	{  20 -8 ,  INPUTEVENT_KEY_SUB},
	{  21 -8 ,  INPUTEVENT_KEY_EQUALS},
	{  22 -8 ,  INPUTEVENT_KEY_BACKSPACE},

	{  23 -8 ,  INPUTEVENT_KEY_TAB},
	{  24 -8 ,  INPUTEVENT_KEY_Q},
	{  25 -8 ,  INPUTEVENT_KEY_W},
	{  26 -8 ,  INPUTEVENT_KEY_E},
	{  27 -8 ,  INPUTEVENT_KEY_R},
	{  28 -8 ,  INPUTEVENT_KEY_T},
	{  29 -8 ,  INPUTEVENT_KEY_Y},
	{  30 -8 ,  INPUTEVENT_KEY_U},
	{  31 -8 ,  INPUTEVENT_KEY_I},
	{  32 -8 ,  INPUTEVENT_KEY_O},
	{  33 -8 ,  INPUTEVENT_KEY_P},
	{  34 -8 ,  INPUTEVENT_KEY_LEFTBRACKET},
	{  35 -8 ,  INPUTEVENT_KEY_RIGHTBRACKET},
	{  36 -8 ,  INPUTEVENT_KEY_RETURN},

	{  66 -8 ,  INPUTEVENT_KEY_CAPS_LOCK},
	{  38 -8 ,  INPUTEVENT_KEY_A},
	{  39 -8 ,  INPUTEVENT_KEY_S},
	{  40 -8 ,  INPUTEVENT_KEY_D},
	{  41 -8 ,  INPUTEVENT_KEY_F},
	{  42 -8 ,  INPUTEVENT_KEY_G},
	{  43 -8 ,  INPUTEVENT_KEY_H},
	{  44 -8 ,  INPUTEVENT_KEY_J},
	{  45 -8 ,  INPUTEVENT_KEY_K},
	{  46 -8 ,  INPUTEVENT_KEY_L},
	{  47 -8 ,  INPUTEVENT_KEY_SEMICOLON},
	{  48 -8 ,  INPUTEVENT_KEY_SINGLEQUOTE},
	{  51 -8 ,  INPUTEVENT_KEY_BACKSLASH},

	{  50 -8 ,  INPUTEVENT_KEY_SHIFT_LEFT},
	{  94 -8 ,  INPUTEVENT_KEY_LTGT},
	{  52 -8 ,  INPUTEVENT_KEY_Z},
	{  53 -8 ,  INPUTEVENT_KEY_X},
	{  54 -8 ,  INPUTEVENT_KEY_C},
	{  55 -8 ,  INPUTEVENT_KEY_V},
	{  56 -8 ,  INPUTEVENT_KEY_B},
	{  57 -8 ,  INPUTEVENT_KEY_N},
	{  58 -8 ,  INPUTEVENT_KEY_M},
	{  59 -8 ,  INPUTEVENT_KEY_COMMA},
	{  60 -8 ,  INPUTEVENT_KEY_PERIOD},
	{  61 -8 ,  INPUTEVENT_KEY_DIV},
	{  62 -8 ,  INPUTEVENT_KEY_SHIFT_RIGHT},

	{  37 -8 ,  INPUTEVENT_KEY_CTRL},
	{  64 -8 ,  INPUTEVENT_KEY_ALT_LEFT},
	{  65 -8 ,  INPUTEVENT_KEY_SPACE},

	{  108 -8 , INPUTEVENT_KEY_ALT_RIGHT},

    //{ 78 -8 ,  INPUTEVENT_KEY_SCROLLOCK},

    //{ 77 -8 ,  INPUTEVENT_KEY_NUMLOCK},
	{  106 -8 , INPUTEVENT_KEY_NP_DIV},
	{  63 -8 ,  INPUTEVENT_KEY_NP_MUL},
	{  82 -8 ,  INPUTEVENT_KEY_NP_SUB},

	{  79 -8 ,  INPUTEVENT_KEY_NP_7},
	{  80 -8 ,  INPUTEVENT_KEY_NP_8},
	{  81 -8 ,  INPUTEVENT_KEY_NP_9},
	{  86 -8 ,  INPUTEVENT_KEY_NP_ADD},

	{  83 -8 ,  INPUTEVENT_KEY_NP_4},
	{  84 -8 ,  INPUTEVENT_KEY_NP_5},
	{  85 -8 ,  INPUTEVENT_KEY_NP_6},

	{  87 -8 ,  INPUTEVENT_KEY_NP_1},
	{  88 -8 ,  INPUTEVENT_KEY_NP_2},
	{  89 -8 ,  INPUTEVENT_KEY_NP_3},
	{  104 -8 , INPUTEVENT_KEY_ENTER},   // The ENT from keypad..

	{  90 -8 ,  INPUTEVENT_KEY_NP_0},
	{  91 -8 ,  INPUTEVENT_KEY_PERIOD},

	{ 111 -8,  INPUTEVENT_KEY_CURSOR_UP},
	{ 113 -8,  INPUTEVENT_KEY_CURSOR_LEFT},
	{ 116 -8,  INPUTEVENT_KEY_CURSOR_DOWN},
	{ 114 -8,  INPUTEVENT_KEY_CURSOR_RIGHT},


	{  133 -8 ,  INPUTEVENT_KEY_AMIGA_LEFT},   // Left amiga mapped to left Windows
	{  134 -8 ,  INPUTEVENT_KEY_AMIGA_RIGHT},  // Right amiga mapped to right windows key.
	{  135 -8 ,  INPUTEVENT_KEY_AMIGA_RIGHT},  // Right amiga mapped to Menu key.
	{ -1, 0 }
  };


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


static struct uae_input_device_kbr_default *keytrans_x11[] = {
	keytrans_amiga_x11,
	keytrans_amiga_x11,
	keytrans_amiga_x11
};


static struct uae_input_device_kbr_default *keytrans_fbcon[] = {
	keytrans_amiga_fbcon,
	keytrans_amiga_fbcon,
	keytrans_amiga_fbcon
};

static int kb_none[] = { -1 };
static int *kbmaps[] = { kb_none, kb_none, kb_none, kb_none, kb_none,
        kb_none, kb_none, kb_none, kb_none, kb_none };


void keyboard_settrans (void)
{
  char vid_drv_name[32];
  // get display type...
  SDL_VideoDriverName(vid_drv_name, sizeof(vid_drv_name));
  if (strcmp(vid_drv_name, "x11") == 0)
  {
    printf("Will use keycode from x11 mapping.\n");
    keyboard_type = KEYCODE_X11;
    inputdevice_setkeytranslation (keytrans_x11, kbmaps);
  } else  if (strcmp(vid_drv_name, "fbcon") == 0)
  {
    printf("Will use keycode from fbcon mapping.\n");
    keyboard_type = KEYCODE_FBCON;
    inputdevice_setkeytranslation (keytrans_fbcon, kbmaps);
  } else
  {
    printf("Unknown keycode to use, will use keysym\n");
    keyboard_type = KEYCODE_UNK;
    inputdevice_setkeytranslation (keytrans, kbmaps);
  }
}


int translate_pandora_keys(int symbol, int *modifier)
{
#ifndef PANDORA_SPECIFIC
  return 0;
#endif
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
