#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include <SDL_image.h>

#include "vkbd.h"
#include "keyboard.h"
#include "sysdeps.h"
#include "uae.h"
#include "amiberry_gfx.h"

#include <map>
#include <vector>
#include <set>

#define VKBD_X 20
#define VKBD_Y 200

//TODO these should be configurable of course
static bool mainMenu_displayHires = true;
static int mainMenu_vkbdLanguage = 1;
static int mainMenu_vkbdStyle = 3;

static bool vkbd_show = false;

static int vkbd_x = VKBD_X;
static int vkbd_y = VKBD_Y;
static int vkbd_transparency = 255;

// Initialized in vkbd_init.
static std::map<int,int> keyToIndex; 
static std::set<int> pressed_sticky_keys;
static std::set<int> sticky_keys;

static SDL_Texture* ksur;
static SDL_Texture* ksurHires;
static SDL_Texture* ksurShift;
static SDL_Texture* ksurShiftHires;

static int actual_index = 0;
static bool actual_pressed = false;
static int previous_state = 0;

static SDL_Color pressed_key_color;

using t_vkbd_rect = struct
{
	SDL_Rect rect;
	unsigned char up, down, left, right;
	int key;
};

static t_vkbd_rect* vkbd_rect;
static int vkbd_rect_size;

//This is the new US keyboard, based directly on amiga-side keys
//last four numbers: next keys in up,down,left,right directions
static t_vkbd_rect vkbd_rect_US[] =
{
	{{1, 1, 29, 11}, 85, 17, 16, 1, AK_ESC}, // 0, row 1 start 
	{{31, 1, 14, 11}, 86, 18, 0, 2, AK_F1}, // 1
	{{46, 1, 14, 11}, 87, 19, 1, 3, AK_F2}, // 2
	{{61, 1, 14, 11}, 87, 20, 2, 4, AK_F3}, // 3
	{{76, 1, 14, 11}, 87, 21, 3, 5, AK_F4}, // 4
	{{91, 1, 14, 11}, 87, 22, 4, 6, AK_F5}, // 5 
	{{106, 1, 14, 11}, 87, 23, 5, 7, AK_F6}, // 6
	{{121, 1, 14, 11}, 87, 24, 6, 8, AK_F7}, // 7 
	{{136, 1, 14, 11}, 87, 25, 7, 9, AK_F8}, // 8 
	{{151, 1, 14, 11}, 87, 26, 8, 10, AK_F9}, // 9
	{{166, 1, 14, 11}, 87, 27, 9, 11, AK_F10}, // 10
	{{181, 1, 29, 11}, 88, 28, 10, 12, AK_DEL}, // 11
	{{211, 1, 29, 11}, 90, 30, 11, 13, AK_HELP}, // 12
	{{241, 1, 14, 11}, 92, 32, 12, 14, AK_NPLPAREN}, // 13
	{{256, 1, 14, 11}, 69, 33, 13, 15, AK_NPRPAREN}, // 14
	{{271, 1, 14, 11}, 69, 34, 14, 16, AK_NPDIV}, // 15
	{{286, 1, 14, 11}, 69, 35, 15, 0, AK_NPMUL}, // 16

	{{1, 13, 29, 11}, 0, 36, 35, 18, AK_BACKQUOTE}, // 17, row 2 start 
	{{31, 13, 14, 11}, 1, 37, 17, 19, AK_1}, // 18
	{{46, 13, 14, 11}, 2, 38, 18, 20, AK_2}, // 19
	{{61, 13, 14, 11}, 3, 39, 19, 21, AK_3}, // 20
	{{76, 13, 14, 11}, 4, 40, 20, 22, AK_4}, // 21
	{{91, 13, 14, 11}, 5, 41, 21, 23, AK_5}, // 22
	{{106, 13, 14, 11}, 6, 42, 22, 24, AK_6}, // 23
	{{121, 13, 14, 11}, 7, 43, 23, 25, AK_7}, // 24 
	{{136, 13, 14, 11}, 8, 44, 24, 26, AK_8}, // 25
	{{151, 13, 14, 11}, 9, 45, 25, 27, AK_9}, // 26
	{{166, 13, 14, 11}, 10, 46, 26, 28, AK_0}, // 27
	{{181, 13, 14, 11}, 11, 47, 27, 29, AK_MINUS}, // 28
	{{196, 13, 14, 11}, 11, 48, 28, 30, AK_EQUAL}, // 29
	{{211, 13, 14, 11}, 12, 49, 29, 31, AK_BACKSLASH}, // 30
	{{226, 13, 14, 11}, 12, 49, 30, 32, AK_BS}, // 31
	{{241, 13, 14, 11}, 13, 50, 31, 33, AK_NP7}, // 32
	{{256, 13, 14, 11}, 14, 51, 32, 34, AK_NP8}, // 33
	{{271, 13, 14, 11}, 15, 52, 33, 35, AK_NP9}, // 34
	{{286, 13, 14, 11}, 16, 53, 34, 17, AK_NPSUB}, // 35

	{{1, 25, 29, 11}, 17, 54, 53, 37, AK_TAB}, // 36, row 3 start 
	{{31, 25, 14, 11}, 18, 55, 36, 38, AK_Q}, // 37
	{{46, 25, 14, 11}, 19, 56, 37, 39, AK_W}, // 38
	{{61, 25, 14, 11}, 20, 57, 38, 40, AK_E}, // 39
	{{76, 25, 14, 11}, 21, 58, 39, 41, AK_R}, // 40
	{{91, 25, 14, 11}, 22, 59, 40, 42, AK_T}, // 41
	{{106, 25, 14, 11}, 23, 60, 41, 43, AK_Y}, // 42
	{{121, 25, 14, 11}, 24, 61, 42, 44, AK_U}, // 43 
	{{136, 25, 14, 11}, 25, 62, 43, 45, AK_I}, // 44
	{{151, 25, 14, 11}, 26, 63, 44, 46, AK_O}, // 45
	{{166, 25, 14, 11}, 27, 64, 45, 47, AK_P}, // 46
	{{181, 25, 14, 11}, 28, 65, 46, 48, AK_LBRACKET}, // 47
	{{196, 25, 14, 11}, 29, 49, 47, 49, AK_RBRACKET}, // 48
	{{211, 25, 29, 23}, 30, 81, 48, 50, AK_RET}, // 49
	{{241, 25, 14, 11}, 32, 66, 49, 51, AK_NP4}, // 50
	{{256, 25, 14, 11}, 33, 67, 50, 52, AK_NP5}, // 51
	{{271, 25, 14, 11}, 34, 68, 51, 53, AK_NP6}, // 52
	{{286, 25, 14, 11}, 35, 69, 52, 36, AK_NPADD}, // 53

	{{1, 37, 29, 11}, 36, 70, 69, 55, AK_CTRL}, // 54, row 4 start 
	{{31, 37, 14, 11}, 37, 70, 54, 56, AK_A}, // 55
	{{46, 37, 14, 11}, 38, 71, 55, 57, AK_S}, // 56
	{{61, 37, 14, 11}, 39, 72, 56, 58, AK_D}, // 57
	{{76, 37, 14, 11}, 40, 73, 57, 59, AK_F}, // 58
	{{91, 37, 14, 11}, 41, 74, 58, 60, AK_G}, // 59
	{{106, 37, 14, 11}, 42, 75, 59, 61, AK_H}, // 60
	{{121, 37, 14, 11}, 43, 76, 60, 62, AK_J}, // 61 
	{{136, 37, 14, 11}, 44, 77, 61, 63, AK_K}, // 62
	{{151, 37, 14, 11}, 45, 78, 62, 64, AK_L}, // 63
	{{166, 37, 14, 11}, 46, 79, 63, 65, AK_SEMICOLON}, // 64
	{{181, 37, 14, 11}, 47, 80, 64, 49, AK_QUOTE}, // 65
	{{241, 37, 14, 11}, 50, 83, 49, 67, AK_NP1}, // 66
	{{256, 37, 14, 11}, 51, 83, 66, 68, AK_NP2}, // 67
	{{271, 37, 14, 11}, 52, 84, 67, 69, AK_NP3}, // 68
	{{286, 37, 14, 34}, 53, 16, 68, 54, AK_ENT}, // 69

	{{1, 49, 44, 11}, 54, 85, 84, 71, AK_LSH}, // 70, row 5 start 
	{{46, 49, 14, 11}, 56, 87, 70, 72, AK_Z}, // 71
	{{61, 49, 14, 11}, 57, 87, 71, 73, AK_X}, // 72
	{{76, 49, 14, 11}, 58, 87, 72, 74, AK_C}, // 73
	{{91, 49, 14, 11}, 59, 87, 73, 75, AK_V}, // 74
	{{106, 49, 14, 11}, 60, 87, 74, 76, AK_B}, // 75
	{{121, 49, 14, 11}, 61, 87, 75, 77, AK_N}, // 76 
	{{136, 49, 14, 11}, 62, 87, 76, 78, AK_M}, // 77
	{{151, 49, 14, 11}, 63, 87, 77, 79, AK_COMMA}, // 78
	{{166, 49, 14, 11}, 64, 87, 78, 80, AK_PERIOD}, // 79
	{{181, 49, 14, 11}, 65, 88, 79, 81, AK_SLASH}, // 80
	{{196, 49, 29, 11}, 49, 89, 80, 82, AK_RSH}, // 81
	{{226, 49, 14, 11}, 49, 91, 81, 83, AK_UP}, // 82
	{{241, 49, 29, 11}, 66, 92, 82, 84, AK_NP0}, // 83
	{{271, 49, 14, 11}, 67, 69, 83, 69, AK_NPDEL}, // 84

	{{1, 61, 29, 11}, 70, 0, 69, 86, AK_LALT}, // 85, row 6 start 
	{{31, 61, 14, 11}, 70, 1, 85, 87, AK_LAMI}, // 86
	{{46, 61, 134, 11}, 71, 2, 86, 88, AK_SPC}, // 87
	{{181, 61, 14, 11}, 80, 11, 87, 89, AK_RAMI}, // 88
	{{196, 61, 14, 11}, 81, 11, 88, 90, AK_RALT}, // 89
	{{211, 61, 14, 11}, 81, 12, 89, 91, AK_LF}, // 90
	{{226, 61, 14, 11}, 82, 12, 90, 92, AK_DN}, // 91 
	{{241, 61, 14, 11}, 83, 13, 91, 69, AK_RT}, // 92
};
static int vkbd_rect_US_size = 92;


//UK KEYBOARD
static t_vkbd_rect vkbd_rect_UK[] =
{
	{{1, 1, 29, 11}, 85, 17, 16, 1, AK_ESC}, // 0, row 1 start 
	{{31, 1, 14, 11}, 86, 18, 0, 2, AK_F1}, // 1
	{{46, 1, 14, 11}, 87, 19, 1, 3, AK_F2}, // 2
	{{61, 1, 14, 11}, 87, 20, 2, 4, AK_F3}, // 3
	{{76, 1, 14, 11}, 87, 21, 3, 5, AK_F4}, // 4
	{{91, 1, 14, 11}, 87, 22, 4, 6, AK_F5}, // 5 
	{{106, 1, 14, 11}, 87, 23, 5, 7, AK_F6}, // 6
	{{121, 1, 14, 11}, 87, 24, 6, 8, AK_F7}, // 7 
	{{136, 1, 14, 11}, 87, 25, 7, 9, AK_F8}, // 8 
	{{151, 1, 14, 11}, 87, 26, 8, 10, AK_F9}, // 9
	{{166, 1, 14, 11}, 87, 27, 9, 11, AK_F10}, // 10
	{{181, 1, 29, 11}, 88, 28, 10, 12, AK_DEL}, // 11
	{{211, 1, 29, 11}, 90, 30, 11, 13, AK_HELP}, // 12
	{{241, 1, 14, 11}, 92, 32, 12, 14, AK_NPLPAREN}, // 13
	{{256, 1, 14, 11}, 69, 33, 13, 15, AK_NPRPAREN}, // 14
	{{271, 1, 14, 11}, 69, 34, 14, 16, AK_NPDIV}, // 15
	{{286, 1, 14, 11}, 69, 35, 15, 0, AK_NPMUL}, // 16

	{{1, 13, 29, 11}, 0, 36, 35, 18, AK_BACKQUOTE}, // 17, row 2 start 
	{{31, 13, 14, 11}, 1, 37, 17, 19, AK_1}, // 18
	{{46, 13, 14, 11}, 2, 38, 18, 20, AK_2}, // 19
	{{61, 13, 14, 11}, 3, 39, 19, 21, AK_3}, // 20
	{{76, 13, 14, 11}, 4, 40, 20, 22, AK_4}, // 21
	{{91, 13, 14, 11}, 5, 41, 21, 23, AK_5}, // 22
	{{106, 13, 14, 11}, 6, 42, 22, 24, AK_6}, // 23
	{{121, 13, 14, 11}, 7, 43, 23, 25, AK_7}, // 24 
	{{136, 13, 14, 11}, 8, 44, 24, 26, AK_8}, // 25
	{{151, 13, 14, 11}, 9, 45, 25, 27, AK_9}, // 26
	{{166, 13, 14, 11}, 10, 46, 26, 28, AK_0}, // 27
	{{181, 13, 14, 11}, 11, 47, 27, 29, AK_MINUS}, // 28
	{{196, 13, 14, 11}, 11, 48, 28, 30, AK_EQUAL}, // 29
	{{211, 13, 14, 11}, 12, 49, 29, 31, AK_BACKSLASH}, // 30
	{{226, 13, 14, 11}, 12, 49, 30, 32, AK_BS}, // 31
	{{241, 13, 14, 11}, 13, 50, 31, 33, AK_NP7}, // 32
	{{256, 13, 14, 11}, 14, 51, 32, 34, AK_NP8}, // 33
	{{271, 13, 14, 11}, 15, 52, 33, 35, AK_NP9}, // 34
	{{286, 13, 14, 11}, 16, 53, 34, 17, AK_NPSUB}, // 35

	{{1, 25, 29, 11}, 17, 54, 53, 37, AK_TAB}, // 36, row 3 start 
	{{31, 25, 14, 11}, 18, 55, 36, 38, AK_Q}, // 37
	{{46, 25, 14, 11}, 19, 56, 37, 39, AK_W}, // 38
	{{61, 25, 14, 11}, 20, 57, 38, 40, AK_E}, // 39
	{{76, 25, 14, 11}, 21, 58, 39, 41, AK_R}, // 40
	{{91, 25, 14, 11}, 22, 59, 40, 42, AK_T}, // 41
	{{106, 25, 14, 11}, 23, 60, 41, 43, AK_Y}, // 42
	{{121, 25, 14, 11}, 24, 61, 42, 44, AK_U}, // 43 
	{{136, 25, 14, 11}, 25, 62, 43, 45, AK_I}, // 44
	{{151, 25, 14, 11}, 26, 63, 44, 46, AK_O}, // 45
	{{166, 25, 14, 11}, 27, 64, 45, 47, AK_P}, // 46
	{{181, 25, 14, 11}, 28, 65, 46, 48, AK_LBRACKET}, // 47
	{{196, 25, 14, 11}, 29, 49, 47, 49, AK_RBRACKET}, // 48
	{{211, 25, 29, 23}, 30, 81, 48, 50, AK_RET}, // 49
	{{241, 25, 14, 11}, 32, 66, 49, 51, AK_NP4}, // 50
	{{256, 25, 14, 11}, 33, 67, 50, 52, AK_NP5}, // 51
	{{271, 25, 14, 11}, 34, 68, 51, 53, AK_NP6}, // 52
	{{286, 25, 14, 11}, 35, 69, 52, 36, AK_NPADD}, // 53

	{{1, 37, 29, 11}, 36, 70, 69, 55, AK_CTRL}, // 54, row 4 start 
	{{31, 37, 14, 11}, 37, 93, 54, 56, AK_A}, // 55 *
	{{46, 37, 14, 11}, 38, 71, 55, 57, AK_S}, // 56
	{{61, 37, 14, 11}, 39, 72, 56, 58, AK_D}, // 57
	{{76, 37, 14, 11}, 40, 73, 57, 59, AK_F}, // 58
	{{91, 37, 14, 11}, 41, 74, 58, 60, AK_G}, // 59
	{{106, 37, 14, 11}, 42, 75, 59, 61, AK_H}, // 60
	{{121, 37, 14, 11}, 43, 76, 60, 62, AK_J}, // 61 
	{{136, 37, 14, 11}, 44, 77, 61, 63, AK_K}, // 62
	{{151, 37, 14, 11}, 45, 78, 62, 64, AK_L}, // 63
	{{166, 37, 14, 11}, 46, 79, 63, 65, AK_SEMICOLON}, // 64
	{{181, 37, 14, 11}, 47, 80, 64, 49, AK_QUOTE}, // 65
	{{241, 37, 14, 11}, 50, 83, 49, 67, AK_NP1}, // 66
	{{256, 37, 14, 11}, 51, 83, 66, 68, AK_NP2}, // 67
	{{271, 37, 14, 11}, 52, 84, 67, 69, AK_NP3}, // 68
	{{286, 37, 14, 34}, 53, 16, 68, 54, AK_ENT}, // 69

	{{1, 49, 29, 11}, 54, 85, 84, 93, AK_LSH}, // 70, row 5 start * 
	{{46, 49, 14, 11}, 56, 87, 93, 72, AK_Z}, // 71 *
	{{61, 49, 14, 11}, 57, 87, 71, 73, AK_X}, // 72
	{{76, 49, 14, 11}, 58, 87, 72, 74, AK_C}, // 73
	{{91, 49, 14, 11}, 59, 87, 73, 75, AK_V}, // 74
	{{106, 49, 14, 11}, 60, 87, 74, 76, AK_B}, // 75
	{{121, 49, 14, 11}, 61, 87, 75, 77, AK_N}, // 76 
	{{136, 49, 14, 11}, 62, 87, 76, 78, AK_M}, // 77
	{{151, 49, 14, 11}, 63, 87, 77, 79, AK_COMMA}, // 78
	{{166, 49, 14, 11}, 64, 87, 78, 80, AK_PERIOD}, // 79
	{{181, 49, 14, 11}, 65, 88, 79, 81, AK_SLASH}, // 80
	{{196, 49, 29, 11}, 49, 89, 80, 82, AK_RSH}, // 81
	{{226, 49, 14, 11}, 49, 91, 81, 83, AK_UP}, // 82
	{{241, 49, 29, 11}, 66, 92, 82, 84, AK_NP0}, // 83
	{{271, 49, 14, 11}, 67, 69, 83, 69, AK_NPDEL}, // 84

	{{1, 61, 29, 11}, 70, 0, 69, 86, AK_LALT}, // 85, row 6 start 
	{{31, 61, 14, 11}, 93, 1, 85, 87, AK_LAMI}, // 86 *
	{{46, 61, 134, 11}, 71, 2, 86, 88, AK_SPC}, // 87
	{{181, 61, 14, 11}, 80, 11, 87, 89, AK_RAMI}, // 88
	{{196, 61, 14, 11}, 81, 11, 88, 90, AK_RALT}, // 89
	{{211, 61, 14, 11}, 81, 12, 89, 91, AK_LF}, // 90
	{{226, 61, 14, 11}, 82, 12, 90, 92, AK_DN}, // 91 
	{{241, 61, 14, 11}, 83, 13, 91, 69, AK_RT}, // 92
	// UK extra keys
	{{31, 49, 14, 11}, 55, 86, 70, 71, AK_LTGT}, // 93	*
};
static int vkbd_rect_UK_size = 93;

//GERMAN KEYBOARD
static t_vkbd_rect vkbd_rect_GER[] =
{
	{{1, 1, 29, 11}, 85, 17, 16, 1, AK_ESC}, // 0, row 1 start 
	{{31, 1, 14, 11}, 86, 18, 0, 2, AK_F1}, // 1
	{{46, 1, 14, 11}, 87, 19, 1, 3, AK_F2}, // 2
	{{61, 1, 14, 11}, 87, 20, 2, 4, AK_F3}, // 3
	{{76, 1, 14, 11}, 87, 21, 3, 5, AK_F4}, // 4
	{{91, 1, 14, 11}, 87, 22, 4, 6, AK_F5}, // 5 
	{{106, 1, 14, 11}, 87, 23, 5, 7, AK_F6}, // 6
	{{121, 1, 14, 11}, 87, 24, 6, 8, AK_F7}, // 7 
	{{136, 1, 14, 11}, 87, 25, 7, 9, AK_F8}, // 8 
	{{151, 1, 14, 11}, 87, 26, 8, 10, AK_F9}, // 9
	{{166, 1, 14, 11}, 87, 27, 9, 11, AK_F10}, // 10
	{{181, 1, 29, 11}, 88, 28, 10, 12, AK_DEL}, // 11
	{{211, 1, 29, 11}, 90, 30, 11, 13, AK_HELP}, // 12
	{{241, 1, 14, 11}, 92, 32, 12, 14, AK_NPLPAREN}, // 13
	{{256, 1, 14, 11}, 69, 33, 13, 15, AK_NPRPAREN}, // 14
	{{271, 1, 14, 11}, 69, 34, 14, 16, AK_NPDIV}, // 15
	{{286, 1, 14, 11}, 69, 35, 15, 0, AK_NPMUL}, // 16

	{{1, 13, 29, 11}, 0, 36, 35, 18, AK_BACKQUOTE}, // 17, row 2 start 
	{{31, 13, 14, 11}, 1, 37, 17, 19, AK_1}, // 18
	{{46, 13, 14, 11}, 2, 38, 18, 20, AK_2}, // 19
	{{61, 13, 14, 11}, 3, 39, 19, 21, AK_3}, // 20
	{{76, 13, 14, 11}, 4, 40, 20, 22, AK_4}, // 21
	{{91, 13, 14, 11}, 5, 41, 21, 23, AK_5}, // 22
	{{106, 13, 14, 11}, 6, 42, 22, 24, AK_6}, // 23
	{{121, 13, 14, 11}, 7, 43, 23, 25, AK_7}, // 24 
	{{136, 13, 14, 11}, 8, 44, 24, 26, AK_8}, // 25
	{{151, 13, 14, 11}, 9, 45, 25, 27, AK_9}, // 26
	{{166, 13, 14, 11}, 10, 46, 26, 28, AK_0}, // 27
	{{181, 13, 14, 11}, 11, 47, 27, 29, AK_MINUS}, // 28
	{{196, 13, 14, 11}, 11, 48, 28, 30, AK_EQUAL}, // 29
	{{211, 13, 14, 11}, 12, 49, 29, 31, AK_BACKSLASH}, // 30
	{{226, 13, 14, 11}, 12, 49, 30, 32, AK_BS}, // 31
	{{241, 13, 14, 11}, 13, 50, 31, 33, AK_NP7}, // 32
	{{256, 13, 14, 11}, 14, 51, 32, 34, AK_NP8}, // 33
	{{271, 13, 14, 11}, 15, 52, 33, 35, AK_NP9}, // 34
	{{286, 13, 14, 11}, 16, 53, 34, 17, AK_NPSUB}, // 35

	{{1, 25, 29, 11}, 17, 54, 53, 37, AK_TAB}, // 36, row 3 start 
	{{31, 25, 14, 11}, 18, 55, 36, 38, AK_Q}, // 37
	{{46, 25, 14, 11}, 19, 56, 37, 39, AK_W}, // 38
	{{61, 25, 14, 11}, 20, 57, 38, 40, AK_E}, // 39
	{{76, 25, 14, 11}, 21, 58, 39, 41, AK_R}, // 40
	{{91, 25, 14, 11}, 22, 59, 40, 42, AK_T}, // 41
	{{106, 25, 14, 11}, 23, 60, 41, 43, AK_Y}, // 42
	{{121, 25, 14, 11}, 24, 61, 42, 44, AK_U}, // 43 
	{{136, 25, 14, 11}, 25, 62, 43, 45, AK_I}, // 44
	{{151, 25, 14, 11}, 26, 63, 44, 46, AK_O}, // 45
	{{166, 25, 14, 11}, 27, 64, 45, 47, AK_P}, // 46
	{{181, 25, 14, 11}, 28, 65, 46, 48, AK_LBRACKET}, // 47
	{{196, 25, 14, 11}, 29, 94, 47, 49, AK_RBRACKET}, // 48 *
	{{211, 25, 29, 23}, 30, 81, 48, 50, AK_RET}, // 49
	{{241, 25, 14, 11}, 32, 66, 49, 51, AK_NP4}, // 50
	{{256, 25, 14, 11}, 33, 67, 50, 52, AK_NP5}, // 51
	{{271, 25, 14, 11}, 34, 68, 51, 53, AK_NP6}, // 52
	{{286, 25, 14, 11}, 35, 69, 52, 36, AK_NPADD}, // 53

	{{1, 37, 29, 11}, 36, 70, 69, 55, AK_CTRL}, // 54, row 4 start 
	{{31, 37, 14, 11}, 37, 93, 54, 56, AK_A}, // 55 *
	{{46, 37, 14, 11}, 38, 71, 55, 57, AK_S}, // 56
	{{61, 37, 14, 11}, 39, 72, 56, 58, AK_D}, // 57
	{{76, 37, 14, 11}, 40, 73, 57, 59, AK_F}, // 58
	{{91, 37, 14, 11}, 41, 74, 58, 60, AK_G}, // 59
	{{106, 37, 14, 11}, 42, 75, 59, 61, AK_H}, // 60
	{{121, 37, 14, 11}, 43, 76, 60, 62, AK_J}, // 61 
	{{136, 37, 14, 11}, 44, 77, 61, 63, AK_K}, // 62
	{{151, 37, 14, 11}, 45, 78, 62, 64, AK_L}, // 63
	{{166, 37, 14, 11}, 46, 79, 63, 65, AK_SEMICOLON}, // 64
	{{181, 37, 14, 11}, 47, 80, 64, 94, AK_QUOTE}, // 65 *
	{{241, 37, 14, 11}, 50, 83, 49, 67, AK_NP1}, // 66
	{{256, 37, 14, 11}, 51, 83, 66, 68, AK_NP2}, // 67
	{{271, 37, 14, 11}, 52, 84, 67, 69, AK_NP3}, // 68
	{{286, 37, 14, 34}, 53, 16, 68, 54, AK_ENT}, // 69

	{{1, 49, 29, 11}, 54, 85, 84, 93, AK_LSH}, // 70, row 5 start * 
	{{46, 49, 14, 11}, 56, 87, 93, 72, AK_Z}, // 71 *
	{{61, 49, 14, 11}, 57, 87, 71, 73, AK_X}, // 72
	{{76, 49, 14, 11}, 58, 87, 72, 74, AK_C}, // 73
	{{91, 49, 14, 11}, 59, 87, 73, 75, AK_V}, // 74
	{{106, 49, 14, 11}, 60, 87, 74, 76, AK_B}, // 75
	{{121, 49, 14, 11}, 61, 87, 75, 77, AK_N}, // 76 
	{{136, 49, 14, 11}, 62, 87, 76, 78, AK_M}, // 77
	{{151, 49, 14, 11}, 63, 87, 77, 79, AK_COMMA}, // 78
	{{166, 49, 14, 11}, 64, 87, 78, 80, AK_PERIOD}, // 79
	{{181, 49, 14, 11}, 65, 88, 79, 81, AK_SLASH}, // 80
	{{196, 49, 29, 11}, 49, 89, 80, 82, AK_RSH}, // 81
	{{226, 49, 14, 11}, 49, 91, 81, 83, AK_UP}, // 82
	{{241, 49, 29, 11}, 66, 92, 82, 84, AK_NP0}, // 83
	{{271, 49, 14, 11}, 67, 69, 83, 69, AK_NPDEL}, // 84

	{{1, 61, 29, 11}, 70, 0, 69, 86, AK_LALT}, // 85, row 6 start 
	{{31, 61, 14, 11}, 93, 1, 85, 87, AK_LAMI}, // 86 *
	{{46, 61, 134, 11}, 71, 2, 86, 88, AK_SPC}, // 87
	{{181, 61, 14, 11}, 80, 11, 87, 89, AK_RAMI}, // 88
	{{196, 61, 14, 11}, 81, 11, 88, 90, AK_RALT}, // 89
	{{211, 61, 14, 11}, 81, 12, 89, 91, AK_LF}, // 90
	{{226, 61, 14, 11}, 82, 12, 90, 92, AK_DN}, // 91 
	{{241, 61, 14, 11}, 83, 13, 91, 69, AK_RT}, // 92	
	//German extra keys
	{{31, 49, 14, 11}, 55, 86, 70, 71, AK_LTGT}, // 93	*
	{{196, 37, 14, 11}, 48, 81, 65, 49, AK_NUMBERSIGN}, // 94 *	
};

static int vkbd_rect_GER_size = 94;

//FRENCH KEYBOARD
static t_vkbd_rect vkbd_rect_FR[] =
{
	{{1, 1, 29, 11}, 85, 17, 16, 1, AK_ESC}, // 0, row 1 start 
	{{31, 1, 14, 11}, 86, 18, 0, 2, AK_F1}, // 1
	{{46, 1, 14, 11}, 87, 19, 1, 3, AK_F2}, // 2
	{{61, 1, 14, 11}, 87, 20, 2, 4, AK_F3}, // 3
	{{76, 1, 14, 11}, 87, 21, 3, 5, AK_F4}, // 4
	{{91, 1, 14, 11}, 87, 22, 4, 6, AK_F5}, // 5 
	{{106, 1, 14, 11}, 87, 23, 5, 7, AK_F6}, // 6
	{{121, 1, 14, 11}, 87, 24, 6, 8, AK_F7}, // 7 
	{{136, 1, 14, 11}, 87, 25, 7, 9, AK_F8}, // 8 
	{{151, 1, 14, 11}, 87, 26, 8, 10, AK_F9}, // 9
	{{166, 1, 14, 11}, 87, 27, 9, 11, AK_F10}, // 10
	{{181, 1, 29, 11}, 88, 28, 10, 12, AK_DEL}, // 11
	{{211, 1, 29, 11}, 90, 30, 11, 13, AK_HELP}, // 12
	{{241, 1, 14, 11}, 92, 32, 12, 14, AK_NPLPAREN}, // 13
	{{256, 1, 14, 11}, 69, 33, 13, 15, AK_NPRPAREN}, // 14
	{{271, 1, 14, 11}, 69, 34, 14, 16, AK_NPDIV}, // 15
	{{286, 1, 14, 11}, 69, 35, 15, 0, AK_NPMUL}, // 16

	{{1, 13, 29, 11}, 0, 36, 35, 18, AK_BACKQUOTE}, // 17, row 2 start 
	{{31, 13, 14, 11}, 1, 37, 17, 19, AK_1}, // 18
	{{46, 13, 14, 11}, 2, 38, 18, 20, AK_2}, // 19
	{{61, 13, 14, 11}, 3, 39, 19, 21, AK_3}, // 20
	{{76, 13, 14, 11}, 4, 40, 20, 22, AK_4}, // 21
	{{91, 13, 14, 11}, 5, 41, 21, 23, AK_5}, // 22
	{{106, 13, 14, 11}, 6, 42, 22, 24, AK_6}, // 23
	{{121, 13, 14, 11}, 7, 43, 23, 25, AK_7}, // 24 
	{{136, 13, 14, 11}, 8, 44, 24, 26, AK_8}, // 25
	{{151, 13, 14, 11}, 9, 45, 25, 27, AK_9}, // 26
	{{166, 13, 14, 11}, 10, 46, 26, 28, AK_0}, // 27
	{{181, 13, 14, 11}, 11, 47, 27, 29, AK_MINUS}, // 28
	{{196, 13, 14, 11}, 11, 48, 28, 30, AK_EQUAL}, // 29
	{{211, 13, 14, 11}, 12, 49, 29, 31, AK_BACKSLASH}, // 30
	{{226, 13, 14, 11}, 12, 49, 30, 32, AK_BS}, // 31
	{{241, 13, 14, 11}, 13, 50, 31, 33, AK_NP7}, // 32
	{{256, 13, 14, 11}, 14, 51, 32, 34, AK_NP8}, // 33
	{{271, 13, 14, 11}, 15, 52, 33, 35, AK_NP9}, // 34
	{{286, 13, 14, 11}, 16, 53, 34, 17, AK_NPSUB}, // 35

	{{1, 25, 29, 11}, 17, 54, 53, 37, AK_TAB}, // 36, row 3 start 
	{{31, 25, 14, 11}, 18, 55, 36, 38, AK_Q}, // 37
	{{46, 25, 14, 11}, 19, 56, 37, 39, AK_W}, // 38
	{{61, 25, 14, 11}, 20, 57, 38, 40, AK_E}, // 39
	{{76, 25, 14, 11}, 21, 58, 39, 41, AK_R}, // 40
	{{91, 25, 14, 11}, 22, 59, 40, 42, AK_T}, // 41
	{{106, 25, 14, 11}, 23, 60, 41, 43, AK_Y}, // 42
	{{121, 25, 14, 11}, 24, 61, 42, 44, AK_U}, // 43 
	{{136, 25, 14, 11}, 25, 62, 43, 45, AK_I}, // 44
	{{151, 25, 14, 11}, 26, 63, 44, 46, AK_O}, // 45
	{{166, 25, 14, 11}, 27, 64, 45, 47, AK_P}, // 46
	{{181, 25, 14, 11}, 28, 65, 46, 48, AK_LBRACKET}, // 47
	{{196, 25, 14, 11}, 29, 94, 47, 49, AK_RBRACKET}, // 48 *
	{{211, 25, 29, 23}, 30, 81, 48, 50, AK_RET}, // 49
	{{241, 25, 14, 11}, 32, 66, 49, 51, AK_NP4}, // 50
	{{256, 25, 14, 11}, 33, 67, 50, 52, AK_NP5}, // 51
	{{271, 25, 14, 11}, 34, 68, 51, 53, AK_NP6}, // 52
	{{286, 25, 14, 11}, 35, 69, 52, 36, AK_NPADD}, // 53

	{{1, 37, 29, 11}, 36, 70, 69, 55, AK_CTRL}, // 54, row 4 start 
	{{31, 37, 14, 11}, 37, 93, 54, 56, AK_A}, // 55 *
	{{46, 37, 14, 11}, 38, 71, 55, 57, AK_S}, // 56
	{{61, 37, 14, 11}, 39, 72, 56, 58, AK_D}, // 57
	{{76, 37, 14, 11}, 40, 73, 57, 59, AK_F}, // 58
	{{91, 37, 14, 11}, 41, 74, 58, 60, AK_G}, // 59
	{{106, 37, 14, 11}, 42, 75, 59, 61, AK_H}, // 60
	{{121, 37, 14, 11}, 43, 76, 60, 62, AK_J}, // 61 
	{{136, 37, 14, 11}, 44, 77, 61, 63, AK_K}, // 62
	{{151, 37, 14, 11}, 45, 78, 62, 64, AK_L}, // 63
	{{166, 37, 14, 11}, 46, 79, 63, 65, AK_SEMICOLON}, // 64
	{{181, 37, 14, 11}, 47, 80, 64, 94, AK_QUOTE}, // 65 *
	{{241, 37, 14, 11}, 50, 83, 49, 67, AK_NP1}, // 66
	{{256, 37, 14, 11}, 51, 83, 66, 68, AK_NP2}, // 67
	{{271, 37, 14, 11}, 52, 84, 67, 69, AK_NP3}, // 68
	{{286, 37, 14, 34}, 53, 16, 68, 54, AK_ENT}, // 69

	{{1, 49, 29, 11}, 54, 85, 84, 93, AK_LSH}, // 70, row 5 start * 
	{{46, 49, 14, 11}, 56, 87, 93, 72, AK_Z}, // 71 *
	{{61, 49, 14, 11}, 57, 87, 71, 73, AK_X}, // 72
	{{76, 49, 14, 11}, 58, 87, 72, 74, AK_C}, // 73
	{{91, 49, 14, 11}, 59, 87, 73, 75, AK_V}, // 74
	{{106, 49, 14, 11}, 60, 87, 74, 76, AK_B}, // 75
	{{121, 49, 14, 11}, 61, 87, 75, 77, AK_N}, // 76 
	{{136, 49, 14, 11}, 62, 87, 76, 78, AK_M}, // 77
	{{151, 49, 14, 11}, 63, 87, 77, 79, AK_COMMA}, // 78
	{{166, 49, 14, 11}, 64, 87, 78, 80, AK_PERIOD}, // 79
	{{181, 49, 14, 11}, 65, 88, 79, 81, AK_SLASH}, // 80
	{{196, 49, 29, 11}, 49, 89, 80, 82, AK_RSH}, // 81
	{{226, 49, 14, 11}, 49, 91, 81, 83, AK_UP}, // 82
	{{241, 49, 29, 11}, 66, 92, 82, 84, AK_NP0}, // 83
	{{271, 49, 14, 11}, 67, 69, 83, 69, AK_NPDEL}, // 84

	{{1, 61, 29, 11}, 70, 0, 69, 86, AK_LALT}, // 85, row 6 start 
	{{31, 61, 14, 11}, 93, 1, 85, 87, AK_LAMI}, // 86 *
	{{46, 61, 134, 11}, 71, 2, 86, 88, AK_SPC}, // 87
	{{181, 61, 14, 11}, 80, 11, 87, 89, AK_RAMI}, // 88
	{{196, 61, 14, 11}, 81, 11, 88, 90, AK_RALT}, // 89
	{{211, 61, 14, 11}, 81, 12, 89, 91, AK_LF}, // 90
	{{226, 61, 14, 11}, 82, 12, 90, 92, AK_DN}, // 91 
	{{241, 61, 14, 11}, 83, 13, 91, 69, AK_RT}, // 92	
	//French extra keys
	{{31, 49, 14, 11}, 55, 86, 70, 71, AK_LTGT}, // 93	*
	{{196, 37, 14, 11}, 48, 81, 65, 49, AK_NUMBERSIGN}, // 94 *	
};

static int vkbd_rect_FR_size = 94;


int vkbd_init(void)
{
	char tmpchar[MAX_DPATH];
	char tmpchar2[MAX_DPATH];
	char vkbdFileName[MAX_DPATH];
	char vkbdHiresFileName[MAX_DPATH];
	char vkbdShiftFileName[MAX_DPATH];
	char vkbdShiftHiresFileName[MAX_DPATH];
	char vkdbStyleString[10];
	char vkbdLanguageString[10];
	switch (mainMenu_vkbdStyle)
	{
	case 1:
		snprintf(vkdbStyleString, 10, "Warm");
		break;
	case 2:
		snprintf(vkdbStyleString, 10, "Cool");
		break;
	case 3:
		snprintf(vkdbStyleString, 10, "Dark");
		break;
	default:
		snprintf(vkdbStyleString, 10, "Orig");
		break;
	}
	switch (mainMenu_vkbdLanguage)
	{
	case 1:
		snprintf(vkbdLanguageString, 10, "UK");
		vkbd_rect = vkbd_rect_UK;
		vkbd_rect_size = vkbd_rect_UK_size;
		break;
	case 2:
		snprintf(vkbdLanguageString, 10, "Ger");
		vkbd_rect = vkbd_rect_GER;
		vkbd_rect_size = vkbd_rect_GER_size;
		break;
	case 3:
		snprintf(vkbdLanguageString, 10, "FR");
		vkbd_rect = vkbd_rect_FR;
		vkbd_rect_size = vkbd_rect_FR_size;
		break;
	default:
		snprintf(vkbdLanguageString, 10, "US");
		vkbd_rect = vkbd_rect_US;
		vkbd_rect_size = vkbd_rect_US_size;
		break;
	}
	snprintf(vkbdFileName, MAX_DPATH, "vkbd/vkbd%s%sLarge.png", vkdbStyleString, vkbdLanguageString);
	snprintf(vkbdHiresFileName, MAX_DPATH, "vkbd/vkbd%s%sLargeHires.png", vkdbStyleString, vkbdLanguageString);
	snprintf(vkbdShiftFileName, MAX_DPATH, "vkbd/vkbd%s%sLargeShift.png", vkdbStyleString, vkbdLanguageString);
	snprintf(vkbdShiftHiresFileName, MAX_DPATH, "vkbd/vkbd%s%sLargeShiftHires.png", vkdbStyleString, vkbdLanguageString);

	TCHAR data_dir[MAX_DPATH];
	get_data_path(data_dir, sizeof data_dir / sizeof(TCHAR));

	snprintf(tmpchar, MAX_DPATH, "%s%s", data_dir, vkbdFileName);
	snprintf(tmpchar2, MAX_DPATH, "%s%s", data_dir, vkbdHiresFileName);

	SDL_Surface* tmp = IMG_Load(tmpchar);

	if (tmp == nullptr)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n", SDL_GetError());
		return -1;
	}
	ksur = SDL_CreateTextureFromSurface(sdl_renderer, tmp);
	SDL_FreeSurface(tmp);

	tmp = IMG_Load(tmpchar2);

	if (tmp == nullptr)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n", SDL_GetError());
		return -1;
	}
	ksurHires = SDL_CreateTextureFromSurface(sdl_renderer, tmp);
	SDL_FreeSurface(tmp);

	// intermediate surfaces to highlight sticky keys on
	//canvas = SDL_DisplayFormat(ksur);
	//canvasHires = SDL_DisplayFormat(ksurHires);

	//for large keyboard, load another image for shifted keys, and set transparency
	snprintf(tmpchar, MAX_DPATH, "%s%s", data_dir, vkbdShiftFileName);
	snprintf(tmpchar2, MAX_DPATH, "%s%s", data_dir, vkbdShiftHiresFileName);

	tmp = IMG_Load(tmpchar);

	if (tmp == nullptr)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n", SDL_GetError());
		return -1;
	}
	ksurShift = SDL_CreateTextureFromSurface(sdl_renderer, tmp);
	SDL_FreeSurface(tmp);

	tmp = IMG_Load(tmpchar2);

	if (tmp == nullptr)
	{
		printf("Virtual Keyboard Bitmap Error: %s\n", SDL_GetError());
		return -1;
	}
	ksurShiftHires = SDL_CreateTextureFromSurface(sdl_renderer, tmp);
	SDL_FreeSurface(tmp);

	vkbd_transparency = 128; //default transparency is 128 for keyboard
	SDL_SetTextureAlphaMod(ksur, vkbd_transparency);
	SDL_SetTextureAlphaMod(ksurHires, vkbd_transparency);

	actual_index = 0;
	previous_state = 0;

#if !defined(__PSP2__) && !defined(__SWITCH__) //no need to show keyboard on first startup
	vkbd_redraw();
#endif
	int w, h;
	SDL_QueryTexture(mainMenu_displayHires ? ksurHires : ksur, NULL, NULL, &w, &h);
	int renderedWidth, rendererHeight;
	SDL_RenderGetLogicalSize(sdl_renderer, &renderedWidth, &rendererHeight);

	vkbd_x = (renderedWidth - w) / 2;
	vkbd_y = rendererHeight - h;

	pressed_key_color.r = 200;
	pressed_key_color.g = 200;
	pressed_key_color.b = 0;
	pressed_key_color.a = 100;

	sticky_keys.clear();
	sticky_keys.insert(AK_LSH);
	sticky_keys.insert(AK_RSH);
	sticky_keys.insert(AK_CTRL);
	sticky_keys.insert(AK_LALT);
	sticky_keys.insert(AK_RALT);
	sticky_keys.insert(AK_LAMI);
	sticky_keys.insert(AK_RAMI);

	pressed_sticky_keys.clear();
	
	keyToIndex.clear();
	for(int i = 0; i < vkbd_rect_size; ++i)
	{
		keyToIndex[vkbd_rect[i].key] = i;
	}

	return 0;
}

void vkbd_quit(void)
{
	SDL_DestroyTexture(ksurShift);
	SDL_DestroyTexture(ksurShiftHires);
	SDL_DestroyTexture(ksur);
	SDL_DestroyTexture(ksurHires);
}

static SDL_Rect GetKeyRect(int index)
{
	SDL_Rect rect;
	rect = vkbd_rect[index].rect;
	if (mainMenu_displayHires)
	{
		rect.x *= 2;
		rect.w *= 2;
	}
	rect.x += vkbd_x;
	rect.y += vkbd_y;
	return rect;
}

static SDL_Texture* GetTextureToDraw()
{
	SDL_Texture* toDraw;
	bool shiftPressed = pressed_sticky_keys.find(AK_LSH) != pressed_sticky_keys.end() || 
						pressed_sticky_keys.find(AK_RSH) != pressed_sticky_keys.end();
	if (mainMenu_displayHires)
	{
		if (shiftPressed)
			toDraw = ksurShiftHires;
		else
			toDraw = ksurHires;
	}
	else
	{
		if (shiftPressed)
			toDraw = ksurShift;
		else
			toDraw = ksur;
	}
	return toDraw;
}

void vkbd_redraw(void)
{
	if (!vkbd_show)
	{
		return;
	}

	SDL_Rect r;
	SDL_Texture* toDraw = GetTextureToDraw();

	int w = 0;
	int h = 0;
	SDL_QueryTexture(toDraw, nullptr, nullptr, &w, &h);
	r.x = vkbd_x;
	r.y = vkbd_y;
	r.w = w;
	r.h = h;
	SDL_RenderCopy(sdl_renderer, toDraw, nullptr, &r);

	// Draw currently selected key:
	auto rect = GetKeyRect(actual_index);
	SDL_SetRenderDrawColor(sdl_renderer, pressed_key_color.r, pressed_key_color.g, pressed_key_color.b, pressed_key_color.a);
	SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(sdl_renderer, &rect);

	// Draw sticky keys:
	for(auto key : pressed_sticky_keys)
	{
		auto index = keyToIndex[key];
		auto rect = GetKeyRect(index);
		SDL_RenderFillRect(sdl_renderer, &rect);
	}
}

void vkbd_toggle(void)
{
	vkbd_show = !vkbd_show;
}

bool vkbd_is_active(void)
{
	return vkbd_show;
}

void vkbd_transparency_up(void)
{
	switch (vkbd_transparency)
	{
	case 255:
		vkbd_transparency = 192;
		break;
	case 192:
		vkbd_transparency = 128;
		break;
	case 128:
		vkbd_transparency = 64;
		break;
	case 64:
		vkbd_transparency = 255;
		break;
	default:
		vkbd_transparency = 64;
		break;
	}
	if (vkbd_transparency != 255)
	{
		//SDL_SetAlpha(canvas, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);
		//SDL_SetAlpha(canvasHires, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);
		SDL_SetTextureAlphaMod(ksur, vkbd_transparency);
		SDL_SetTextureAlphaMod(ksurHires, vkbd_transparency);
	}
	else //fully opague
	{
		//SDL_SetAlpha(canvas, 0, 255);
		//SDL_SetAlpha(canvasHires, 0, 255);
		SDL_SetTextureAlphaMod(ksur, 255);
		SDL_SetTextureAlphaMod(ksurHires, 255);
	}
}

void vkbd_transparency_down(void)
{
	switch (vkbd_transparency)
	{
	case 255:
		vkbd_transparency = 64;
		break;
	case 192:
		vkbd_transparency = 255;
		break;
	case 128:
		vkbd_transparency = 192;
		break;
	case 64:
		vkbd_transparency = 128;
		break;
	default:
		vkbd_transparency = 255;
		break;
	}
	if (vkbd_transparency != 255)
	{
		//SDL_SetAlpha(canvas, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);
		//SDL_SetAlpha(canvasHires, SDL_SRCALPHA | SDL_RLEACCEL, vkbd_transparency);
		SDL_SetTextureAlphaMod(ksur, vkbd_transparency);
		SDL_SetTextureAlphaMod(ksurHires, vkbd_transparency);
	}
	else //fully opague
	{
		//SDL_SetAlpha(canvas, 0, 255);
		//SDL_SetAlpha(canvasHires, 0, 255);
		SDL_SetTextureAlphaMod(ksur, 255);
		SDL_SetTextureAlphaMod(ksurHires, 255);
	}
}


void vkbd_displace_up(void)
{
	if (vkbd_y > 3)
		vkbd_y -= 4;
	else
		vkbd_y = 0;
}

void vkbd_displace_down(void)
{
	int w, h;
	SDL_QueryTexture(ksur, NULL, NULL, &w, &h);
	if (vkbd_y < sdl_surface->h - h - 3)
		vkbd_y += 4;
	else
		vkbd_y = sdl_surface->h - h;
}

static bool switched_on(int state, int previous_state, int mask)
{
	return (state & mask) != (previous_state & mask) && (state & mask) != 0;
}

static bool switched_off(int state, int previous_state, int mask)
{
	return (state & mask) != (previous_state & mask) && (state & mask) == 0;
}

// For simplicity we don't allow movement (up/down/left/right) when the fire button is pressed.
// For normal (non-sticky) keys button press/release events are send whenever the fire button is released.
// For sticky keys, the key is toggled whenever the fire button goes from not pressed to pressed.
void vkbd_process(int state, std::vector<int> &keycode, std::vector<int> &pressed)
{
	keycode.clear();
	pressed.clear();

	// Check button up event.
	if (switched_off(state, previous_state, VKBD_BUTTON))
	{
		actual_pressed = false;
		auto actual_key = vkbd_rect[actual_index].key;
		if (pressed_sticky_keys.find(actual_key) == pressed_sticky_keys.end())
		{
			keycode.push_back(actual_key);
			pressed.push_back(0);
		}
	}

	if ((state & VKBD_BUTTON) == 0)
	{
		bool up = switched_on(state, previous_state, VKBD_UP);
		bool down = switched_on(state, previous_state, VKBD_DOWN);
		bool left = switched_on(state, previous_state, VKBD_LEFT);
		bool right = switched_on(state, previous_state, VKBD_RIGHT);
		auto actual_key = vkbd_rect[actual_index].key;

		if (up)
		{
			actual_index = vkbd_rect[actual_index].up;
		}
		if (down)
		{
			actual_index = vkbd_rect[actual_index].down;
		}
		if (left)
		{
			actual_index = vkbd_rect[actual_index].left;
		}
		if (right)
		{
			actual_index = vkbd_rect[actual_index].right;
		}
	}

	if (switched_on(state, previous_state, VKBD_BUTTON))
	{
		auto actual_key = vkbd_rect[actual_index].key;
		if (sticky_keys.find(actual_key) != sticky_keys.end())
		{
			actual_pressed = false;
			if (pressed_sticky_keys.find(actual_key) != pressed_sticky_keys.end())
			{
				auto iter = pressed_sticky_keys.find(actual_key);
				pressed_sticky_keys.erase(iter);
				keycode.push_back(actual_key);
				pressed.push_back(0);
			}
			else
			{
				pressed_sticky_keys.insert(actual_key);
				keycode.push_back(actual_key);
				pressed.push_back(1);
			}
		}
		else
		{
			keycode.push_back(actual_key);
			pressed.push_back(1);
			actual_pressed = true;
		}
	}
	previous_state = state;
}
