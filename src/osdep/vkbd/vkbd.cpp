#include<stdio.h>
#include<stdlib.h>
#include<SDL.h>

#ifdef USE_SDL2
#include "sdl2_to_sdl1.h"
#endif

#include "vkbd.h"

#include "keyboard.h"

#define VKBD_MIN_HOLDING_TIME 200
#define VKBD_MOVE_DELAY 50

extern int keycode2amiga(SDL_keysym *prKeySym);

extern int mainMenu_displayHires;
extern int mainMenu_vkbdLanguage;

static int vkbd_x=VKBD_X;
static int vkbd_y=VKBD_Y;
static int vkbd_transparency=255;
static int vkbd_just_blinked=0;
static Uint32 vkbd_last_press_time=0;
static Uint32 vkbd_last_move_time=0;

// Keep track of keys that are sticky
t_vkbd_sticky_key vkbd_sticky_key[] = 
{
	{ AK_LSH, false, true, 70},
	{ AK_RSH, false, true, 81},
	{ AK_CTRL, false, true, 54},
	{ AK_LALT, false, true, 85},
	{ AK_RALT, false, true, 89},
	{ AK_LAMI, false, true, 86},
	{ AK_RAMI, false, true, 88}
};

int vkbd_let_go_of_direction=0;
int vkbd_mode=0;
int vkbd_move=0;
int vkbd_key=KEYCODE_NOTHING;
SDLKey vkbd_button2=(SDLKey)0; //not implemented
int vkbd_keysave=KEYCODE_NOTHING;

#if !defined (DREAMCAST) && !defined (GP2X) && !defined (PSP) && !defined (GIZMONDO) 

int vkbd_init(void) { return 0; }
void vkbd_init_button2(void) { }
void vkbd_quit(void) { }
int vkbd_process(void) { return KEYCODE_NOTHING; }
void vkbd_displace_up(void) { };
void vkbd_displace_down(void) { };
void vkbd_transparency_up(void) { };
void vkbd_transparency_down(void) { };
#else

extern SDL_Surface *prSDLScreen;

static SDL_Surface *ksur;
static SDL_Surface *ksurHires;
static SDL_Surface *canvas; // intermediate surface used to highlight sticky keys
static SDL_Surface *canvasHires; // intermediate surface used to highlight sticky keys
static SDL_Surface *ksurShift;
static SDL_Surface *ksurShiftHires;

static int vkbd_actual=0, vkbd_color=0;

#ifdef GP2X
extern char launchDir [256];
#endif

typedef struct{
	SDL_Rect rect;
	unsigned char up,down,left,right;
	int key;
} t_vkbd_rect;

static t_vkbd_rect *vkbd_rect;

//This is the new US keyboard, based directly on amiga-side keys
//last four numbers: next keys in up,down,left,right directions
static t_vkbd_rect vkbd_rect_US[]= 
{
	{{  1,  1, 29, 11 },85,17,16, 1, AK_ESC}, // 0, row 1 start 
	{{ 31,  1, 14, 11 },86,18, 0, 2, AK_F1},	// 1
	{{ 46,  1, 14, 11 },87,19, 1, 3, AK_F2},	// 2
	{{ 61,  1, 14, 11 },87,20, 2, 4, AK_F3},	// 3
	{{ 76,  1, 14, 11 },87,21, 3, 5, AK_F4},	// 4
	{{ 91,  1, 14, 11 },87,22, 4, 6, AK_F5},	// 5 
	{{106,  1, 14, 11 },87,23, 5, 7, AK_F6},	// 6
	{{121,  1, 14, 11 },87,24, 6, 8, AK_F7},	// 7 
	{{136,  1, 14, 11 },87,25, 7, 9, AK_F8},	// 8 
	{{151,  1, 14, 11 },87,26, 8,10, AK_F9},	// 9
	{{166,  1, 14, 11 },87,27, 9,11, AK_F10},	// 10
	{{181,  1, 29, 11 },88,28,10,12, AK_DEL},	// 11
	{{211,  1, 29, 11 },90,30,11,13, AK_HELP},	// 12
	{{241,  1, 14, 11 },92,32,12,14, AK_NPLPAREN},	// 13
	{{256,  1, 14, 11 },69,33,13,15, AK_NPRPAREN},	// 14
	{{271,  1, 14, 11 },69,34,14,16, AK_NPDIV},	// 15
	{{286,  1, 14, 11 },69,35,15,0, AK_NPMUL},	// 16
	
	{{  1, 13, 29, 11 }, 0,36,35,18, AK_BACKQUOTE}, // 17, row 2 start 
	{{ 31, 13, 14, 11 }, 1,37,17,19, AK_1},	// 18
	{{ 46, 13, 14, 11 }, 2,38,18,20, AK_2},	// 19
	{{ 61, 13, 14, 11 }, 3,39,19,21, AK_3},	// 20
	{{ 76, 13, 14, 11 }, 4,40,20,22, AK_4},	// 21
	{{ 91, 13, 14, 11 }, 5,41,21,23, AK_5},	// 22
	{{106, 13, 14, 11 }, 6,42,22,24, AK_6},	// 23
	{{121, 13, 14, 11 }, 7,43,23,25, AK_7},	// 24 
	{{136, 13, 14, 11 }, 8,44,24,26, AK_8},	// 25
	{{151, 13, 14, 11 }, 9,45,25,27, AK_9},	// 26
	{{166, 13, 14, 11 },10,46,26,28, AK_0},	// 27
	{{181, 13, 14, 11 },11,47,27,29, AK_MINUS},	// 28
	{{196, 13, 14, 11 },11,48,28,30, AK_EQUAL},	// 29
	{{211, 13, 14, 11 },12,49,29,31, AK_BACKSLASH},	// 30
	{{226, 13, 14, 11 },12,49,30,32, AK_BS},	// 31
	{{241, 13, 14, 11 },13,50,31,33, AK_NP7},	// 32
	{{256, 13, 14, 11 },14,51,32,34, AK_NP8},	// 33
	{{271, 13, 14, 11 },15,52,33,35, AK_NP9},	// 34
	{{286, 13, 14, 11 },16,53,34,17, AK_NPSUB},	// 35
	
	{{  1, 25, 29, 11 }, 17,54,53,37, AK_TAB}, // 36, row 3 start 
	{{ 31, 25, 14, 11 }, 18,55,36,38, AK_Q},	// 37
	{{ 46, 25, 14, 11 }, 19,56,37,39, AK_W},	// 38
	{{ 61, 25, 14, 11 }, 20,57,38,40, AK_E},	// 39
	{{ 76, 25, 14, 11 }, 21,58,39,41, AK_R},	// 40
	{{ 91, 25, 14, 11 }, 22,59,40,42, AK_T},	// 41
	{{106, 25, 14, 11 }, 23,60,41,43, AK_Y},	// 42
	{{121, 25, 14, 11 }, 24,61,42,44, AK_U},	// 43 
	{{136, 25, 14, 11 }, 25,62,43,45, AK_I},	// 44
	{{151, 25, 14, 11 }, 26,63,44,46, AK_O},	// 45
	{{166, 25, 14, 11 }, 27,64,45,47, AK_P},	// 46
	{{181, 25, 14, 11 }, 28,65,46,48, AK_LBRACKET},	// 47
	{{196, 25, 14, 11 }, 29,49,47,49, AK_RBRACKET},	// 48
	{{211, 25, 29, 23 }, 30,81,48,50, AK_RET},	// 49
	{{241, 25, 14, 11 }, 32,66,49,51, AK_NP4},	// 50
	{{256, 25, 14, 11 }, 33,67,50,52, AK_NP5},	// 51
	{{271, 25, 14, 11 }, 34,68,51,53, AK_NP6},	// 52
	{{286, 25, 14, 11 }, 35,69,52,36, AK_NPADD},	// 53
	
	{{  1, 37, 29, 11 }, 36,70,69,55, AK_CTRL}, // 54, row 4 start 
	{{ 31, 37, 14, 11 }, 37,70,54,56, AK_A},	// 55
	{{ 46, 37, 14, 11 }, 38,71,55,57, AK_S},	// 56
	{{ 61, 37, 14, 11 }, 39,72,56,58, AK_D},	// 57
	{{ 76, 37, 14, 11 }, 40,73,57,59, AK_F},	// 58
	{{ 91, 37, 14, 11 }, 41,74,58,60, AK_G},	// 59
	{{106, 37, 14, 11 }, 42,75,59,61, AK_H},	// 60
	{{121, 37, 14, 11 }, 43,76,60,62, AK_J},	// 61 
	{{136, 37, 14, 11 }, 44,77,61,63, AK_K},	// 62
	{{151, 37, 14, 11 }, 45,78,62,64, AK_L},	// 63
	{{166, 37, 14, 11 }, 46,79,63,65, AK_SEMICOLON},	// 64
	{{181, 37, 14, 11 }, 47,80,64,49, AK_QUOTE},	// 65
	{{241, 37, 14, 11 }, 50,83,49,67, AK_NP1},	// 66
	{{256, 37, 14, 11 }, 51,83,66,68, AK_NP2},	// 67
	{{271, 37, 14, 11 }, 52,84,67,69, AK_NP3},	// 68
	{{286, 37, 14, 34 }, 53,16,68,54, AK_ENT},	// 69
	
	{{  1, 49, 44, 11 }, 54,85,84,71, AK_LSH}, // 70, row 5 start 
	{{ 46, 49, 14, 11 }, 56,87,70,72, AK_Z},	// 71
	{{ 61, 49, 14, 11 }, 57,87,71,73, AK_X},	// 72
	{{ 76, 49, 14, 11 }, 58,87,72,74, AK_C},	// 73
	{{ 91, 49, 14, 11 }, 59,87,73,75, AK_V},	// 74
	{{106, 49, 14, 11 }, 60,87,74,76, AK_B},	// 75
	{{121, 49, 14, 11 }, 61,87,75,77, AK_N},	// 76 
	{{136, 49, 14, 11 }, 62,87,76,78, AK_M},	// 77
	{{151, 49, 14, 11 }, 63,87,77,79, AK_COMMA},	// 78
	{{166, 49, 14, 11 }, 64,87,78,80, AK_PERIOD},	// 79
	{{181, 49, 14, 11 }, 65,88,79,81, AK_SLASH},	// 80
	{{196, 49, 29, 11 }, 49,89,80,82, AK_RSH},	// 81
	{{226, 49, 14, 11 }, 49,91,81,83, AK_UP},	// 82
	{{241, 49, 29, 11 }, 66,92,82,84, AK_NP0},	// 83
	{{271, 49, 14, 11 }, 67,69,83,69, AK_NPDEL},	// 84
	
	{{  1, 61, 29, 11 }, 70,0,69,86, AK_LALT}, // 85, row 6 start 
	{{ 31, 61, 14, 11 }, 70,1,85,87, AK_LAMI},	// 86
	{{ 46, 61,134, 11 }, 71,2,86,88, AK_SPC},	// 87
	{{181, 61, 14, 11 }, 80,11,87,89, AK_RAMI},	// 88
	{{196, 61, 14, 11 }, 81,11,88,90, AK_RALT},	// 89
	{{211, 61, 14, 11 }, 81,12,89,91, AK_LF},	// 90
	{{226, 61, 14, 11 }, 82,12,90,92, AK_DN},	// 91 
	{{241, 61, 14, 11 }, 83,13,91,69, AK_RT},	// 92
};

//UK KEYBOARD
static t_vkbd_rect vkbd_rect_UK[]= 
{
	{{  1,  1, 29, 11 },85,17,16, 1, AK_ESC}, // 0, row 1 start 
	{{ 31,  1, 14, 11 },86,18, 0, 2, AK_F1},	// 1
	{{ 46,  1, 14, 11 },87,19, 1, 3, AK_F2},	// 2
	{{ 61,  1, 14, 11 },87,20, 2, 4, AK_F3},	// 3
	{{ 76,  1, 14, 11 },87,21, 3, 5, AK_F4},	// 4
	{{ 91,  1, 14, 11 },87,22, 4, 6, AK_F5},	// 5 
	{{106,  1, 14, 11 },87,23, 5, 7, AK_F6},	// 6
	{{121,  1, 14, 11 },87,24, 6, 8, AK_F7},	// 7 
	{{136,  1, 14, 11 },87,25, 7, 9, AK_F8},	// 8 
	{{151,  1, 14, 11 },87,26, 8,10, AK_F9},	// 9
	{{166,  1, 14, 11 },87,27, 9,11, AK_F10},	// 10
	{{181,  1, 29, 11 },88,28,10,12, AK_DEL},	// 11
	{{211,  1, 29, 11 },90,30,11,13, AK_HELP},	// 12
	{{241,  1, 14, 11 },92,32,12,14, AK_NPLPAREN},	// 13
	{{256,  1, 14, 11 },69,33,13,15, AK_NPRPAREN},	// 14
	{{271,  1, 14, 11 },69,34,14,16, AK_NPDIV},	// 15
	{{286,  1, 14, 11 },69,35,15,0, AK_NPMUL},	// 16
	
	{{  1, 13, 29, 11 }, 0,36,35,18, AK_BACKQUOTE}, // 17, row 2 start 
	{{ 31, 13, 14, 11 }, 1,37,17,19, AK_1},	// 18
	{{ 46, 13, 14, 11 }, 2,38,18,20, AK_2},	// 19
	{{ 61, 13, 14, 11 }, 3,39,19,21, AK_3},	// 20
	{{ 76, 13, 14, 11 }, 4,40,20,22, AK_4},	// 21
	{{ 91, 13, 14, 11 }, 5,41,21,23, AK_5},	// 22
	{{106, 13, 14, 11 }, 6,42,22,24, AK_6},	// 23
	{{121, 13, 14, 11 }, 7,43,23,25, AK_7},	// 24 
	{{136, 13, 14, 11 }, 8,44,24,26, AK_8},	// 25
	{{151, 13, 14, 11 }, 9,45,25,27, AK_9},	// 26
	{{166, 13, 14, 11 },10,46,26,28, AK_0},	// 27
	{{181, 13, 14, 11 },11,47,27,29, AK_MINUS},	// 28
	{{196, 13, 14, 11 },11,48,28,30, AK_EQUAL},	// 29
	{{211, 13, 14, 11 },12,49,29,31, AK_BACKSLASH},	// 30
	{{226, 13, 14, 11 },12,49,30,32, AK_BS},	// 31
	{{241, 13, 14, 11 },13,50,31,33, AK_NP7},	// 32
	{{256, 13, 14, 11 },14,51,32,34, AK_NP8},	// 33
	{{271, 13, 14, 11 },15,52,33,35, AK_NP9},	// 34
	{{286, 13, 14, 11 },16,53,34,17, AK_NPSUB},	// 35
	
	{{  1, 25, 29, 11 }, 17,54,53,37, AK_TAB}, // 36, row 3 start 
	{{ 31, 25, 14, 11 }, 18,55,36,38, AK_Q},	// 37
	{{ 46, 25, 14, 11 }, 19,56,37,39, AK_W},	// 38
	{{ 61, 25, 14, 11 }, 20,57,38,40, AK_E},	// 39
	{{ 76, 25, 14, 11 }, 21,58,39,41, AK_R},	// 40
	{{ 91, 25, 14, 11 }, 22,59,40,42, AK_T},	// 41
	{{106, 25, 14, 11 }, 23,60,41,43, AK_Y},	// 42
	{{121, 25, 14, 11 }, 24,61,42,44, AK_U},	// 43 
	{{136, 25, 14, 11 }, 25,62,43,45, AK_I},	// 44
	{{151, 25, 14, 11 }, 26,63,44,46, AK_O},	// 45
	{{166, 25, 14, 11 }, 27,64,45,47, AK_P},	// 46
	{{181, 25, 14, 11 }, 28,65,46,48, AK_LBRACKET},	// 47
	{{196, 25, 14, 11 }, 29,49,47,49, AK_RBRACKET},	// 48
	{{211, 25, 29, 23 }, 30,81,48,50, AK_RET},	// 49
	{{241, 25, 14, 11 }, 32,66,49,51, AK_NP4},	// 50
	{{256, 25, 14, 11 }, 33,67,50,52, AK_NP5},	// 51
	{{271, 25, 14, 11 }, 34,68,51,53, AK_NP6},	// 52
	{{286, 25, 14, 11 }, 35,69,52,36, AK_NPADD},	// 53
	
	{{  1, 37, 29, 11 }, 36,70,69,55, AK_CTRL}, // 54, row 4 start 
	{{ 31, 37, 14, 11 }, 37,93,54,56, AK_A},	// 55 *
	{{ 46, 37, 14, 11 }, 38,71,55,57, AK_S},	// 56
	{{ 61, 37, 14, 11 }, 39,72,56,58, AK_D},	// 57
	{{ 76, 37, 14, 11 }, 40,73,57,59, AK_F},	// 58
	{{ 91, 37, 14, 11 }, 41,74,58,60, AK_G},	// 59
	{{106, 37, 14, 11 }, 42,75,59,61, AK_H},	// 60
	{{121, 37, 14, 11 }, 43,76,60,62, AK_J},	// 61 
	{{136, 37, 14, 11 }, 44,77,61,63, AK_K},	// 62
	{{151, 37, 14, 11 }, 45,78,62,64, AK_L},	// 63
	{{166, 37, 14, 11 }, 46,79,63,65, AK_SEMICOLON},	// 64
	{{181, 37, 14, 11 }, 47,80,64,49, AK_QUOTE},	// 65
	{{241, 37, 14, 11 }, 50,83,49,67, AK_NP1},	// 66
	{{256, 37, 14, 11 }, 51,83,66,68, AK_NP2},	// 67
	{{271, 37, 14, 11 }, 52,84,67,69, AK_NP3},	// 68
	{{286, 37, 14, 34 }, 53,16,68,54, AK_ENT},	// 69
	
	{{  1, 49, 29, 11 }, 54,85,84,93, AK_LSH}, // 70, row 5 start * 
	{{ 46, 49, 14, 11 }, 56,87,93,72, AK_Z},	// 71 *
	{{ 61, 49, 14, 11 }, 57,87,71,73, AK_X},	// 72
	{{ 76, 49, 14, 11 }, 58,87,72,74, AK_C},	// 73
	{{ 91, 49, 14, 11 }, 59,87,73,75, AK_V},	// 74
	{{106, 49, 14, 11 }, 60,87,74,76, AK_B},	// 75
	{{121, 49, 14, 11 }, 61,87,75,77, AK_N},	// 76 
	{{136, 49, 14, 11 }, 62,87,76,78, AK_M},	// 77
	{{151, 49, 14, 11 }, 63,87,77,79, AK_COMMA},	// 78
	{{166, 49, 14, 11 }, 64,87,78,80, AK_PERIOD},	// 79
	{{181, 49, 14, 11 }, 65,88,79,81, AK_SLASH},	// 80
	{{196, 49, 29, 11 }, 49,89,80,82, AK_RSH},	// 81
	{{226, 49, 14, 11 }, 49,91,81,83, AK_UP},	// 82
	{{241, 49, 29, 11 }, 66,92,82,84, AK_NP0},	// 83
	{{271, 49, 14, 11 }, 67,69,83,69, AK_NPDEL},	// 84
	
	{{  1, 61, 29, 11 }, 70,0,69,86, AK_LALT}, // 85, row 6 start 
	{{ 31, 61, 14, 11 }, 93,1,85,87, AK_LAMI},	// 86 *
	{{ 46, 61,134, 11 }, 71,2,86,88, AK_SPC},	// 87
	{{181, 61, 14, 11 }, 80,11,87,89, AK_RAMI},	// 88
	{{196, 61, 14, 11 }, 81,11,88,90, AK_RALT},	// 89
	{{211, 61, 14, 11 }, 81,12,89,91, AK_LF},	// 90
	{{226, 61, 14, 11 }, 82,12,90,92, AK_DN},	// 91 
	{{241, 61, 14, 11 }, 83,13,91,69, AK_RT},	// 92
	// UK extra keys
	{{31, 49, 14, 11 }, 55,86,70,71, AK_LTGT},	// 93	*
};

//GERMAN KEYBOARD
static t_vkbd_rect vkbd_rect_GER[]= 
{
	{{  1,  1, 29, 11 },85,17,16, 1, AK_ESC}, // 0, row 1 start 
	{{ 31,  1, 14, 11 },86,18, 0, 2, AK_F1},	// 1
	{{ 46,  1, 14, 11 },87,19, 1, 3, AK_F2},	// 2
	{{ 61,  1, 14, 11 },87,20, 2, 4, AK_F3},	// 3
	{{ 76,  1, 14, 11 },87,21, 3, 5, AK_F4},	// 4
	{{ 91,  1, 14, 11 },87,22, 4, 6, AK_F5},	// 5 
	{{106,  1, 14, 11 },87,23, 5, 7, AK_F6},	// 6
	{{121,  1, 14, 11 },87,24, 6, 8, AK_F7},	// 7 
	{{136,  1, 14, 11 },87,25, 7, 9, AK_F8},	// 8 
	{{151,  1, 14, 11 },87,26, 8,10, AK_F9},	// 9
	{{166,  1, 14, 11 },87,27, 9,11, AK_F10},	// 10
	{{181,  1, 29, 11 },88,28,10,12, AK_DEL},	// 11
	{{211,  1, 29, 11 },90,30,11,13, AK_HELP},	// 12
	{{241,  1, 14, 11 },92,32,12,14, AK_NPLPAREN},	// 13
	{{256,  1, 14, 11 },69,33,13,15, AK_NPRPAREN},	// 14
	{{271,  1, 14, 11 },69,34,14,16, AK_NPDIV},	// 15
	{{286,  1, 14, 11 },69,35,15,0, AK_NPMUL},	// 16
	
	{{  1, 13, 29, 11 }, 0,36,35,18, AK_BACKQUOTE}, // 17, row 2 start 
	{{ 31, 13, 14, 11 }, 1,37,17,19, AK_1},	// 18
	{{ 46, 13, 14, 11 }, 2,38,18,20, AK_2},	// 19
	{{ 61, 13, 14, 11 }, 3,39,19,21, AK_3},	// 20
	{{ 76, 13, 14, 11 }, 4,40,20,22, AK_4},	// 21
	{{ 91, 13, 14, 11 }, 5,41,21,23, AK_5},	// 22
	{{106, 13, 14, 11 }, 6,42,22,24, AK_6},	// 23
	{{121, 13, 14, 11 }, 7,43,23,25, AK_7},	// 24 
	{{136, 13, 14, 11 }, 8,44,24,26, AK_8},	// 25
	{{151, 13, 14, 11 }, 9,45,25,27, AK_9},	// 26
	{{166, 13, 14, 11 },10,46,26,28, AK_0},	// 27
	{{181, 13, 14, 11 },11,47,27,29, AK_MINUS},	// 28
	{{196, 13, 14, 11 },11,48,28,30, AK_EQUAL},	// 29
	{{211, 13, 14, 11 },12,49,29,31, AK_BACKSLASH},	// 30
	{{226, 13, 14, 11 },12,49,30,32, AK_BS},	// 31
	{{241, 13, 14, 11 },13,50,31,33, AK_NP7},	// 32
	{{256, 13, 14, 11 },14,51,32,34, AK_NP8},	// 33
	{{271, 13, 14, 11 },15,52,33,35, AK_NP9},	// 34
	{{286, 13, 14, 11 },16,53,34,17, AK_NPSUB},	// 35
	
	{{  1, 25, 29, 11 }, 17,54,53,37, AK_TAB}, // 36, row 3 start 
	{{ 31, 25, 14, 11 }, 18,55,36,38, AK_Q},	// 37
	{{ 46, 25, 14, 11 }, 19,56,37,39, AK_W},	// 38
	{{ 61, 25, 14, 11 }, 20,57,38,40, AK_E},	// 39
	{{ 76, 25, 14, 11 }, 21,58,39,41, AK_R},	// 40
	{{ 91, 25, 14, 11 }, 22,59,40,42, AK_T},	// 41
	{{106, 25, 14, 11 }, 23,60,41,43, AK_Y},	// 42
	{{121, 25, 14, 11 }, 24,61,42,44, AK_U},	// 43 
	{{136, 25, 14, 11 }, 25,62,43,45, AK_I},	// 44
	{{151, 25, 14, 11 }, 26,63,44,46, AK_O},	// 45
	{{166, 25, 14, 11 }, 27,64,45,47, AK_P},	// 46
	{{181, 25, 14, 11 }, 28,65,46,48, AK_LBRACKET},	// 47
	{{196, 25, 14, 11 }, 29,94,47,49, AK_RBRACKET},	// 48 *
	{{211, 25, 29, 23 }, 30,81,48,50, AK_RET},	// 49
	{{241, 25, 14, 11 }, 32,66,49,51, AK_NP4},	// 50
	{{256, 25, 14, 11 }, 33,67,50,52, AK_NP5},	// 51
	{{271, 25, 14, 11 }, 34,68,51,53, AK_NP6},	// 52
	{{286, 25, 14, 11 }, 35,69,52,36, AK_NPADD},	// 53
	
	{{  1, 37, 29, 11 }, 36,70,69,55, AK_CTRL}, // 54, row 4 start 
	{{ 31, 37, 14, 11 }, 37,93,54,56, AK_A},	// 55 *
	{{ 46, 37, 14, 11 }, 38,71,55,57, AK_S},	// 56
	{{ 61, 37, 14, 11 }, 39,72,56,58, AK_D},	// 57
	{{ 76, 37, 14, 11 }, 40,73,57,59, AK_F},	// 58
	{{ 91, 37, 14, 11 }, 41,74,58,60, AK_G},	// 59
	{{106, 37, 14, 11 }, 42,75,59,61, AK_H},	// 60
	{{121, 37, 14, 11 }, 43,76,60,62, AK_J},	// 61 
	{{136, 37, 14, 11 }, 44,77,61,63, AK_K},	// 62
	{{151, 37, 14, 11 }, 45,78,62,64, AK_L},	// 63
	{{166, 37, 14, 11 }, 46,79,63,65, AK_SEMICOLON},	// 64
	{{181, 37, 14, 11 }, 47,80,64,94, AK_QUOTE},	// 65 *
	{{241, 37, 14, 11 }, 50,83,49,67, AK_NP1},	// 66
	{{256, 37, 14, 11 }, 51,83,66,68, AK_NP2},	// 67
	{{271, 37, 14, 11 }, 52,84,67,69, AK_NP3},	// 68
	{{286, 37, 14, 34 }, 53,16,68,54, AK_ENT},	// 69
	
	{{  1, 49, 29, 11 }, 54,85,84,93, AK_LSH}, // 70, row 5 start * 
	{{ 46, 49, 14, 11 }, 56,87,93,72, AK_Z},	// 71 *
	{{ 61, 49, 14, 11 }, 57,87,71,73, AK_X},	// 72
	{{ 76, 49, 14, 11 }, 58,87,72,74, AK_C},	// 73
	{{ 91, 49, 14, 11 }, 59,87,73,75, AK_V},	// 74
	{{106, 49, 14, 11 }, 60,87,74,76, AK_B},	// 75
	{{121, 49, 14, 11 }, 61,87,75,77, AK_N},	// 76 
	{{136, 49, 14, 11 }, 62,87,76,78, AK_M},	// 77
	{{151, 49, 14, 11 }, 63,87,77,79, AK_COMMA},	// 78
	{{166, 49, 14, 11 }, 64,87,78,80, AK_PERIOD},	// 79
	{{181, 49, 14, 11 }, 65,88,79,81, AK_SLASH},	// 80
	{{196, 49, 27, 11 }, 49,89,80,82, AK_RSH},	// 81
	{{226, 49, 14, 11 }, 49,91,81,83, AK_UP},	// 82
	{{241, 49, 29, 11 }, 66,92,82,84, AK_NP0},	// 83
	{{271, 49, 14, 11 }, 67,69,83,69, AK_NPDEL},	// 84
	
	{{  1, 61, 29, 11 }, 70,0,69,86, AK_LALT}, // 85, row 6 start 
	{{ 31, 61, 14, 11 }, 93,1,85,87, AK_LAMI},	// 86 *
	{{ 46, 61,134, 11 }, 71,2,86,88, AK_SPC},	// 87
	{{181, 61, 14, 11 }, 80,11,87,89, AK_RAMI},	// 88
	{{196, 61, 14, 11 }, 81,11,88,90, AK_RALT},	// 89
	{{211, 61, 14, 11 }, 81,12,89,91, AK_LF},	// 90
	{{226, 61, 14, 11 }, 82,12,90,92, AK_DN},	// 91 
	{{241, 61, 14, 11 }, 83,13,91,69, AK_RT},	// 92	
	//German extra keys
	{{31, 49, 14, 11 }, 55,86,70,71, AK_LTGT},	// 93	*
	{{196, 37, 14, 11 }, 48,81,65,49, AK_NUMBERSIGN}, // 94 *	
};

void vkbd_init_button2(void)
{
	vkbd_button2=(SDLKey)0;
}

void vkbd_reset_sticky_keys(void) 
{
	for (int i=0; i<NUM_STICKY; i++)
	{
		vkbd_sticky_key[i].can_switch=true;
		vkbd_sticky_key[i].stuck=false;
	}
}

int vkbd_init(void)
{
	int i;
	char tmpchar[256];
	char tmpchar2[256];
	char vkbdFileName[256];
	char vkbdHiresFileName[256];
	char vkbdShiftFileName[256];
	char vkbdShiftHiresFileName[256];
	switch (mainMenu_vkbdLanguage)
	{
		case 1:
			snprintf(vkbdFileName, 256, "vkbdUKLarge.bmp");
			snprintf(vkbdHiresFileName, 256, "vkbdUKLargeHires.bmp");
			snprintf(vkbdShiftFileName, 256, "vkbdUKLargeShift.bmp");
			snprintf(vkbdShiftHiresFileName, 256, "vkbdUKLargeShiftHires.bmp");
			vkbd_rect=vkbd_rect_UK;
			break;
		case 2:
			snprintf(vkbdFileName, 256, "vkbdGERLarge.bmp");
			snprintf(vkbdHiresFileName, 256, "vkbdGERLargeHires.bmp");
			snprintf(vkbdShiftFileName, 256, "vkbdGERLargeShift.bmp");
			snprintf(vkbdShiftHiresFileName, 256, "vkbdGERLargeShiftHires.bmp");
			vkbd_rect=vkbd_rect_GER;
			break;
		default:
			snprintf(vkbdFileName, 256, "vkbdUSLarge.bmp");
			snprintf(vkbdHiresFileName, 256, "vkbdUSLargeHires.bmp");
			snprintf(vkbdShiftFileName, 256, "vkbdUSLargeShift.bmp");
			snprintf(vkbdShiftHiresFileName, 256, "vkbdUSLargeShiftHires.bmp");
			vkbd_rect=vkbd_rect_US;
			break;
	}

#if defined(__PSP2__) || defined(__SWITCH__)
	snprintf(tmpchar, 256, "%s%s", DATA_PREFIX, vkbdFileName);
	snprintf(tmpchar2, 256, "%s%s", DATA_PREFIX, vkbdHiresFileName);
#else
#ifdef GP2X
	snprintf(tmpchar, 256, "%s/data/%s", launchDir, vkbdFileName);
	snprintf(tmpchar2, 256, "%s/data/%s", launchDir, vkbdHiresFileName);
#else
#ifdef GIZMONDO
	snprintf(tmpchar, 256, "%s", "\\SD Card\\uae4all\\data\\%s",vkbdFileName);
	snprintf(tmpchar2, 256, "%s", "\\SD Card\\uae4all\\data\\%s",vkbdHiresFileName);
#else
	snprintf(tmpchar, 256, "%s%s", DATA_PREFIX, vkbdFileName);
	snprintf(tmpchar2, 256, "%s%s", DATA_PREFIX, vkbdHiresFileName);
#endif
#endif
#endif //__PSP2__

	SDL_Surface *tmp = SDL_LoadBMP(tmpchar);

	if (tmp==NULL)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n",SDL_GetError());
		return -1;
	}
	ksur=SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);

	tmp = SDL_LoadBMP(tmpchar2);
	
	if (tmp==NULL)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n",SDL_GetError());
		return -1;
	}
	ksurHires=SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);

	// intermediate surfaces to highlight sticky keys on
	canvas=SDL_DisplayFormat(ksur);
	canvasHires=SDL_DisplayFormat(ksurHires);

	//for large keyboard, load another image for shifted keys, and set transparency
#if defined(__PSP2__) || defined(__SWITCH__)
	snprintf(tmpchar, 256, "%s%s", DATA_PREFIX, vkbdShiftFileName);
	snprintf(tmpchar2, 256, "%s%s", DATA_PREFIX, vkbdShiftHiresFileName);
#else
#ifdef GP2X
	snprintf(tmpchar, 256, "%s/data/%s", launchDir, vkbdShiftFileName);
	snprintf(tmpchar2, 256, "%s/data/%s", launchDir, vkbdShiftHiresFileName);
#else
#ifdef GIZMONDO
	snprintf(tmpchar, 256, "%s", "\\SD Card\\uae4all\\data\\%s",vkbdShiftFileName);
	snprintf(tmpchar2, 256, "%s", "\\SD Card\\uae4all\\data\\%s",vkbdShiftHiresFileName);
#else
	snprintf(tmpchar, 256, "%s%s", DATA_PREFIX, vkbdShiftFileName);
	snprintf(tmpchar2, 256, "%s%s", DATA_PREFIX, vkbdShiftHiresFileName);
#endif
#endif
#endif //__PSP2__
	
	tmp = SDL_LoadBMP(tmpchar);

	if (tmp==NULL)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n",SDL_GetError());
		return -1;
	}
	ksurShift=SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);
	
	tmp = SDL_LoadBMP(tmpchar2);

	if (tmp==NULL)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n",SDL_GetError());
		return -1;
	}
	ksurShiftHires=SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);

	vkbd_transparency=128; //default transparency is 128 for keyboard
	SDL_SetAlpha(canvas, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);
	SDL_SetAlpha(canvasHires, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);

	vkbd_actual=0;
#if !defined(__PSP2__) && !defined(__SWITCH__) //no need to show keyboard on first startup
	vkbd_redraw();
#endif
	for (int i=0; i<NUM_STICKY; i++)
	{
		vkbd_sticky_key[i].stuck=false;
	}
	vkbd_x=(prSDLScreen->w-ksur->w)/2;
	vkbd_y=prSDLScreen->h-ksur->h;
	vkbd_mode=0;
	vkbd_move=0;
	vkbd_last_press_time=0;
	vkbd_last_move_time=0;
	vkbd_key=KEYCODE_NOTHING;
	vkbd_button2=(SDLKey)0;
	vkbd_keysave=KEYCODE_NOTHING;
	return 0;
}

void vkbd_quit(void)
{
	int i;
	SDL_FreeSurface(ksurShift);
	SDL_FreeSurface(ksurShiftHires);
	SDL_FreeSurface(ksur);
	SDL_FreeSurface(ksurHires);
	vkbd_mode=0;
	for (int i=0; i<NUM_STICKY; i++)
	{
		vkbd_sticky_key[i].stuck=false;
	}
}

void vkbd_redraw(void)
{
	SDL_Rect r;
	SDL_Surface *toDraw;
	SDL_Surface *myCanvas;
	if (mainMenu_displayHires)
	{
		if (vkbd_sticky_key[0].stuck || vkbd_sticky_key[1].stuck)			
			toDraw=ksurShiftHires;
		else
			toDraw=ksurHires;
			myCanvas=canvasHires;
	}
	else
	{
		if (vkbd_sticky_key[0].stuck || vkbd_sticky_key[1].stuck)
			toDraw=ksurShift;
		else
			toDraw=ksur;
			myCanvas=canvas;
	}

	// blit onto intermediate canvas
	r.x=0;
	r.y=0;
	r.w=toDraw->w;
	r.h=toDraw->h;
	SDL_BlitSurface(toDraw,NULL,myCanvas,&r);

	// highlight sticky keys that are pressed with a green dot
	// do this on a canvas to ensure correct transparency on final blit
	Uint32 sticky_key_color=SDL_MapRGB(myCanvas->format, 0, 255, 0);
	for (int i=0; i<NUM_STICKY; i++) {
		if (vkbd_sticky_key[i].stuck==true) {
			int index = vkbd_sticky_key[i].index;
			if (mainMenu_displayHires)
			{
				r.x=2*vkbd_rect[index].rect.x+2;
				r.w=6;
			}
			else
			{
				r.x=vkbd_rect[index].rect.x+1;
				r.w=3;
			}
			r.y=vkbd_rect[index].rect.y+1;
			r.h=3;
			SDL_FillRect(myCanvas,&r,sticky_key_color);
		}
	}

	if (vkbd_y>prSDLScreen->h-myCanvas->h) 
		vkbd_y=prSDLScreen->h-myCanvas->h;
		
	vkbd_x=(prSDLScreen->w-myCanvas->w)/2;
	
	r.x=vkbd_x;	
	r.y=vkbd_y;	
	r.w=myCanvas->w;
	r.h=myCanvas->h;

	SDL_BlitSurface(myCanvas,NULL,prSDLScreen,&r);
}

void vkbd_transparency_up(void)
{
	switch (vkbd_transparency) 
	{
		case 255:
			vkbd_transparency=192;
			break;
		case 192:
			vkbd_transparency=128;
			break;
		case 128: 
			vkbd_transparency=64;
			break;
		case 64: 
			vkbd_transparency=255;
			break;
		default:
			vkbd_transparency=64;
			break;
	}		
	if (vkbd_transparency != 255)
	{
		SDL_SetAlpha(canvas, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);		
		SDL_SetAlpha(canvasHires, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);		
	}
	else //fully opague
	{
	 	SDL_SetAlpha(canvas, 0, 255);
	 	SDL_SetAlpha(canvasHires, 0, 255);
	}
}	

void vkbd_transparency_down(void)
{
	switch (vkbd_transparency) 
	{
		case 255:
			vkbd_transparency=64;
			break;
		case 192:
			vkbd_transparency=255;
			break;
		case 128: 
			vkbd_transparency=192;
			break;
	 	case 64:
			vkbd_transparency=128;
			break;
		default:
			vkbd_transparency=255;
			break;
	}		
	if (vkbd_transparency != 255)
	{
		SDL_SetAlpha(canvas, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);		
		SDL_SetAlpha(canvasHires, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);		
	}
	else //fully opague
	{
	 	SDL_SetAlpha(canvas, 0, 255);
	 	SDL_SetAlpha(canvasHires, 0, 255);				
	}
}	


void vkbd_displace_up(void)
{
	if (vkbd_y>3)
		vkbd_y-=4;
	else
		vkbd_y=0;
}

void vkbd_displace_down(void)
{
	if (vkbd_y<prSDLScreen->h-ksur->h-3)
		vkbd_y+=4;
	else
		vkbd_y=prSDLScreen->h-ksur->h;
}		

int vkbd_process(void)
{
	Uint32 now=SDL_GetTicks();
	SDL_Rect r;
	
	vkbd_redraw();
	if (vkbd_move&VKBD_BUTTON)
	{
		vkbd_move=0;
		int amigaKeyCode=vkbd_rect[vkbd_actual].key;
		//some keys are sticky
		for (int i=0; i<NUM_STICKY; i++) 
		{
			if (amigaKeyCode == vkbd_sticky_key[i].code) 
			{
				if (vkbd_sticky_key[i].can_switch) 
				{
					vkbd_sticky_key[i].stuck=!vkbd_sticky_key[i].stuck;
					vkbd_sticky_key[i].can_switch=false;
					return amigaKeyCode;
				}
				else return (KEYCODE_NOTHING);
			}
		}
		return amigaKeyCode;
	}
	
	if (vkbd_move&VKBD_BUTTON_BACKSPACE)
	{
		vkbd_move=0;
		return AK_BS;
	}
	if (vkbd_move&VKBD_BUTTON_SHIFT)
	{
		vkbd_move=0;
		// hotkey for shift toggle (arbitrarily use left shift here)
		if (vkbd_sticky_key[0].can_switch)
		{
			vkbd_sticky_key[0].stuck=!vkbd_sticky_key[0].stuck;
			vkbd_sticky_key[0].can_switch=false;
			return(AK_LSH);
		} else
			return(KEYCODE_NOTHING);
	}
	if (vkbd_move&VKBD_BUTTON_RESET_STICKY)
	{
		vkbd_move=0;
		// hotkey to reset all sticky keys at once
		vkbd_reset_sticky_keys();
		return(KEYCODE_STICKY_RESET); // special return code to reset amiga keystate
	}
	if (vkbd_move&VKBD_LEFT || vkbd_move&VKBD_RIGHT || vkbd_move&VKBD_UP || vkbd_move&VKBD_DOWN) 
	{
		if (vkbd_let_go_of_direction) //just pressing down
			vkbd_last_press_time=now;
		if (
				(
				now-vkbd_last_press_time>VKBD_MIN_HOLDING_TIME 
				&& now-vkbd_last_move_time>VKBD_MOVE_DELAY
				) 
				|| vkbd_let_go_of_direction
			) 
		{
			vkbd_last_move_time=now;
			if (vkbd_move&VKBD_LEFT)
				vkbd_actual=vkbd_rect[vkbd_actual].left;
			else if (vkbd_move&VKBD_RIGHT)
				vkbd_actual=vkbd_rect[vkbd_actual].right;
			if (vkbd_move&VKBD_UP)
				vkbd_actual=vkbd_rect[vkbd_actual].up;
			else if (vkbd_move&VKBD_DOWN)
				vkbd_actual=vkbd_rect[vkbd_actual].down;
		}
		vkbd_let_go_of_direction=0;
	}
	else
		vkbd_let_go_of_direction=1;
		
	if (mainMenu_displayHires)
	{
		r.x=vkbd_x+2*vkbd_rect[vkbd_actual].rect.x;
		r.w=2*vkbd_rect[vkbd_actual].rect.w;
	}
	else
	{
		r.x=vkbd_x+vkbd_rect[vkbd_actual].rect.x;
		r.w=vkbd_rect[vkbd_actual].rect.w;
	}
	r.y=vkbd_y+vkbd_rect[vkbd_actual].rect.y;
	r.h=vkbd_rect[vkbd_actual].rect.h;
	if (!vkbd_just_blinked)
	{	
		SDL_FillRect(prSDLScreen,&r,vkbd_color);
		vkbd_just_blinked=1;
		//vkbd_color = ~vkbd_color;
	}
	else
	{
		vkbd_just_blinked=0;
	}
	return KEYCODE_NOTHING; //nothing on the vkbd was pressed
}
#endif
