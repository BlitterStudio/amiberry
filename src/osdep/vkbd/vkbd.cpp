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
#include <chrono>

using VkbdRect = struct
{
	SDL_Rect rect;
	int up, down, left, right;
	int key;
};

// Configurable options
static bool vkbdHires = true;
static VkbdLanguage vkbdLanguage = VKBD_LANGUAGE_US;
static VkbdStyle vkbdStyle = VKBD_STYLE_ORIG;
static double vkbdTransparency = 0.0;
static int vkbdAlpha = 255;
static double vkbdSlideDurationInSeconds = 1.0;
static bool vkbdKeyboardHasExitButton = true;

// All directions:
const std::vector<int> vkbdDirections{VKBD_UP, VKBD_DOWN, VKBD_LEFT, VKBD_RIGHT};

// Boolean that determines if keyboard is shown.
static bool vkbdShow = false;

// Time when button was pressed to show virtual keyboard.
static std::chrono::time_point<std::chrono::system_clock> vkbdTimeLastRedraw;

// Initialized in vkbd_init.
static std::map<int, int> vkbdStickyKeyToIndex;
static std::set<int> vkbdPressedStickyKeys;
static std::set<int> vkbdStickyKeys;
static SDL_Texture* vkbdTexture = nullptr;
static SDL_Texture* vkbdTextureShift = nullptr;

static VkbdRect* vkbdRect;

// Acutal index in current t_vkbdRect
static int vkbdActualIndex = 0;

// If actual key is pressed.
static bool vkbdActualPressed = false;

// Mask indicating previous event. It's a mask consisting of VKBD_UP, VKBD_DOWN, VKBD_LEFT, VKBD_RIGHT and VKBD_BUTTON
static int vkbdPreviousState = 0;

// Color that is used to color pressed keys.
static SDL_Color vkbdPressedKeyColor;

// Where the keyboard is currently drawn:
static int vkbdCurrentX;
static int vkbdCurrentY;

// Where the keyboard is drawn when it is finished sliding:
static int vkbdEndX;
static int vkbdEndY;

// Where the keyboard is drawn when is it starts sliding in:
static int vkbdStartX;
static int vkbdStartY;

// Following data is used to modify vkbdRect in case an exit button is provided on the keyboard.
// This provides the data for the exit button.
static VkbdRect vkbdExitButtonRect;

// This is used as index for the exit button:
static constexpr int vkbdExitButtonIndex = -1;

// These are other keys from which you can move into the exit button:
const std::vector<std::pair<int, int>> vkbdExitButtonConnections{
	std::make_pair(AK_ESC, VKBD_LEFT),
	std::make_pair(AK_BACKQUOTE, VKBD_LEFT),
	std::make_pair(AK_TAB, VKBD_LEFT),
	std::make_pair(AK_CTRL, VKBD_LEFT),
	std::make_pair(AK_LSH, VKBD_LEFT),
	std::make_pair(AK_LALT, VKBD_LEFT),
	std::make_pair(AK_NPMUL, VKBD_RIGHT),
	std::make_pair(AK_NPSUB, VKBD_RIGHT),
	std::make_pair(AK_NPADD, VKBD_RIGHT),
	std::make_pair(AK_ENT, VKBD_RIGHT)
};

//This is the new US keyboard, based directly on amiga-side keys
//last four numbers: next keys in up,down,left,right directions
static VkbdRect vkbdRectUS[] =
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

//UK KEYBOARD
static VkbdRect vkbdRectUK[] =
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

//GERMAN KEYBOARD
static VkbdRect vkbdRectGER[] =
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

//FRENCH KEYBOARD
static VkbdRect vkbdRectFR[] =
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

static std::string vkbd_get_style_string()
{
	std::string styleString;
	switch (vkbdStyle)
	{
	case VKBD_STYLE_WARM:
		styleString = "Warm";
		break;
	case VKBD_STYLE_COOL:
		styleString = "Cool";
		break;
	case VKBD_STYLE_DARK:
		styleString = "Dark";
		break;
	default:
		styleString = "Orig";
		break;
	}
	return styleString;
}

static std::string vkbd_get_language_string()
{
	std::string languageString;
	switch (vkbdLanguage)
	{
	case VKBD_LANGUAGE_UK:
		languageString = "UK";
		break;
	case VKBD_LANGUAGE_GER:
		languageString = "Ger";
		break;
	case VKBD_LANGUAGE_FR:
		languageString = "FR";
		break;
	default:
		languageString = "US";
		break;
	}
	return languageString;
}

static std::string vkbd_get_hires_string()
{
	return vkbdHires ? "Hires" : "";
}

static std::string vkbd_get_exit_button_filename()
{
	const std::string styleString = vkbd_get_style_string();
	const std::string hiresString = vkbd_get_hires_string();

	std::string fileName = get_data_path();
	fileName += std::string("vkbd/vkbd");
	fileName += styleString;
	fileName += std::string("Quit");
	fileName += hiresString;
	fileName += std::string(".png");

	return fileName;
}

static std::string vkbd_get_keyboard_filename(bool shift)
{
	const std::string styleString = vkbd_get_style_string();
	const std::string languageString = vkbd_get_language_string();
	const std::string hiresString = vkbd_get_hires_string();
	const std::string shiftString = shift ? "Shift" : "";

	std::string fileName = get_data_path();
	fileName += std::string("vkbd/vkbd");
	fileName += styleString;
	fileName += languageString;
	fileName += std::string("Large");
	fileName += shiftString;
	fileName += hiresString;
	fileName += std::string(".png");

	return fileName;
}

static SDL_Surface* vkbd_create_keyboard_surface(bool shift)
{
	const auto fileName = vkbd_get_keyboard_filename(shift);
	const auto surf = IMG_Load(fileName.c_str());

	if (surf == nullptr)
	{
		write_log(_T("Virtual Keyboard Bitmap Error: %s\n"), SDL_GetError());
	}

	return surf;
}

static SDL_Surface* vkbd_create_exit_button_surface()
{
	const auto fileName = vkbd_get_exit_button_filename();
	const auto surf = IMG_Load(fileName.c_str());

	if (surf == nullptr)
	{
		write_log(_T("Virtual Keyboard Bitmap Error: %s\n"), SDL_GetError());
	}

	return surf;
}

static SDL_Surface* vkbd_concat_surfaces(SDL_Surface* keyboard, SDL_Surface* exit)
{
	const int width = keyboard->w + exit->w;
	const int height = std::max(keyboard->h, exit->h);
	const auto surf = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
	SDL_Rect srcRect;
	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.w = keyboard->w;
	srcRect.h = keyboard->h;
	SDL_Rect destRect;
	destRect.x = 0;
	destRect.y = 0;
	destRect.w = keyboard->w;
	destRect.h = keyboard->h;
	SDL_BlitSurface(keyboard, &srcRect, surf, &destRect);
	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.w = exit->w;
	srcRect.h = exit->h;
	destRect.x = keyboard->w;
	destRect.y = 0;
	destRect.w = exit->w;
	destRect.h = exit->h;
	SDL_BlitSurface(exit, &srcRect, surf, &destRect);

	return surf;
}

#ifdef USE_OPENGL
	//TODO needs implementation
#else
static SDL_Texture* vkbd_create_keyboard_texture(bool shift)
{
	AmigaMonitor* mon = &AMonitors[0];

	const auto keyboard = vkbd_create_keyboard_surface(shift);
	if (keyboard == nullptr)
	{
		return nullptr;
	}

	SDL_Surface* surf;
	if (vkbdKeyboardHasExitButton)
	{
		const auto exit = vkbd_create_exit_button_surface();
		if (exit == nullptr)
		{
			return nullptr;
		}

		surf = vkbd_concat_surfaces(keyboard, exit);

		if (surf == nullptr)
		{
			return nullptr;
		}

		SDL_FreeSurface(exit);
	}
	else
	{
		surf = keyboard;
	}

	const auto texture = SDL_CreateTextureFromSurface(mon->amiga_renderer, surf);
	SDL_FreeSurface(surf);
	return texture;
}
#endif

static int vkbd_find_index(int key)
{
	int index = 0;
	while (vkbdRect[index].key != key)
	{
		++index;
	}
	return index;
}

void vkbd_update_position_from_texture()
{
	int width, height;
	SDL_QueryTexture(vkbdTexture, nullptr, nullptr, &width, &height);
	int renderedWidth = crop_rect.w, rendererHeight = crop_rect.h;

	vkbdEndX = (renderedWidth - width) / 2;
	vkbdEndY = rendererHeight - height;

	vkbdStartX = (renderedWidth - width) / 2;
	vkbdStartY = rendererHeight;
}

void vkbd_update(bool createTextures)
{
	switch (vkbdLanguage)
	{
	case VKBD_LANGUAGE_UK:
		vkbdRect = vkbdRectUK;
		break;
	case VKBD_LANGUAGE_GER:
		vkbdRect = vkbdRectGER;
		break;
	case VKBD_LANGUAGE_FR:
		vkbdRect = vkbdRectFR;
		break;
	default:
		vkbdRect = vkbdRectUS;
		break;
	}
#ifdef USE_OPENGL
	//TODO needs implementation
#else
	if (createTextures || vkbdTexture != nullptr)
	{
		if (vkbdTexture != nullptr)
		{
			SDL_DestroyTexture(vkbdTexture);
		}

		vkbdTexture = vkbd_create_keyboard_texture(false);
	}

	if (createTextures || vkbdTextureShift != nullptr)
	{
		if (vkbdTextureShift != nullptr)
		{
			SDL_DestroyTexture(vkbdTextureShift);
		}

		vkbdTextureShift = vkbd_create_keyboard_texture(true);
	}

	if (createTextures)
	{
		vkbd_update_position_from_texture();
	}
#endif

	vkbdPressedKeyColor.r = 200;
	vkbdPressedKeyColor.g = 200;
	vkbdPressedKeyColor.b = 0;
	vkbdPressedKeyColor.a = 100;

	vkbdStickyKeys.clear();
	vkbdStickyKeys.insert(AK_LSH);
	vkbdStickyKeys.insert(AK_RSH);
	vkbdStickyKeys.insert(AK_CTRL);
	vkbdStickyKeys.insert(AK_LALT);
	vkbdStickyKeys.insert(AK_RALT);
	vkbdStickyKeys.insert(AK_LAMI);
	vkbdStickyKeys.insert(AK_RAMI);

	vkbdPressedStickyKeys.clear();

	vkbdStickyKeyToIndex.clear();
	for (auto key : vkbdStickyKeys)
	{
		const auto index = vkbd_find_index(key);
		vkbdStickyKeyToIndex[key] = index;
	}

	vkbdExitButtonRect.rect.x = 301;
	vkbdExitButtonRect.rect.y = 1;
	vkbdExitButtonRect.rect.w = 14;
	vkbdExitButtonRect.rect.h = 72;
	vkbdExitButtonRect.down = vkbdExitButtonIndex;
	vkbdExitButtonRect.up = vkbdExitButtonIndex;
	vkbdExitButtonRect.right = vkbd_find_index(AK_LSH);
	vkbdExitButtonRect.left = vkbd_find_index(AK_ENT);
	vkbdExitButtonRect.key = AKS_QUIT;
}


void vkbd_set_hires(bool hires)
{
	if (vkbdHires == hires)
	{
		return;
	}

	vkbdHires = hires;
	vkbd_update(false);
}

void vkbd_set_language(VkbdLanguage language)
{
	if (vkbdLanguage == language)
	{
		return;
	}

	vkbdLanguage = language;
	vkbd_update(false);
}

void vkbd_set_style(VkbdStyle style)
{
	if (vkbdStyle == style)
	{
		return;
	}

	vkbdStyle = style;
	vkbd_update(false);
}

void vkbd_set_language(const std::string& language)
{
	VkbdLanguage vkbd_language;
	if (language == "FR")
		vkbd_language = VKBD_LANGUAGE_FR;
	else if (language == "UK")
		vkbd_language = VKBD_LANGUAGE_UK;
	else if (language == "DE")
		vkbd_language = VKBD_LANGUAGE_GER;
	else
		vkbd_language = VKBD_LANGUAGE_US;

	vkbd_set_language(vkbd_language);
}

void vkbd_set_style(const std::string& style)
{
	VkbdStyle vkbd_style;
	if (style == "Warm")
		vkbd_style = VKBD_STYLE_WARM;
	else if (style == "Cool")
		vkbd_style = VKBD_STYLE_COOL;
	else if (style == "Dark")
		vkbd_style = VKBD_STYLE_DARK;
	else
		vkbd_style = VKBD_STYLE_ORIG;

	vkbd_set_style(vkbd_style);
}

void vkbd_set_transparency(double transparency)
{
	vkbdTransparency = std::max(std::min(1.0, transparency), 0.0);
	vkbdAlpha = static_cast<int>(255 * (1.0 - transparency));
	vkbd_update(false);
}

void vkbd_set_keyboard_has_exit_button(bool keyboardHasExitButton)
{
	vkbdKeyboardHasExitButton = keyboardHasExitButton;
	vkbd_update(false);
}

void vkbd_init()
{
	vkbd_update(true);
}

void vkbd_quit()
{
	if (vkbdTexture != nullptr)
	{
		SDL_DestroyTexture(vkbdTexture);
	}

	if (vkbdTextureShift != nullptr)
	{
		SDL_DestroyTexture(vkbdTexture);
	}
}

static const VkbdRect& vkbd_get_key_rect(int index)
{
	if (index == vkbdExitButtonIndex)
	{
		return vkbdExitButtonRect;
	}
	return vkbdRect[index];
}

static SDL_Rect vkbd_get_key_drawing_rect(int index)
{
	SDL_Rect rect = vkbd_get_key_rect(index).rect;
	if (vkbdHires)
	{
		rect.x *= 2;
		rect.w *= 2;
	}
	rect.x += vkbdCurrentX;
	rect.y += vkbdCurrentY;
	return rect;
}

static SDL_Texture* vkbd_get_texture_to_draw()
{
	const bool shiftPressed = vkbdPressedStickyKeys.find(AK_LSH) != vkbdPressedStickyKeys.end() ||
		vkbdPressedStickyKeys.find(AK_RSH) != vkbdPressedStickyKeys.end();
	SDL_Texture* toDraw = shiftPressed ? vkbdTextureShift : vkbdTexture;
	return toDraw;
}

static void vkbd_update_current_xy()
{
	const auto now = std::chrono::system_clock::now();
	const auto diff = now - vkbdTimeLastRedraw;
	const double diffSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0;
	const double speedX = (vkbdEndX - vkbdStartX) / vkbdSlideDurationInSeconds;
	const double speedY = (vkbdEndY - vkbdStartY) / vkbdSlideDurationInSeconds;
	const double sign = vkbdShow ? +1.0 : -1.0;
	const int maxX = std::max(vkbdStartX, vkbdEndX);
	const int maxY = std::max(vkbdStartY, vkbdEndY);
	const int minX = std::min(vkbdStartX, vkbdEndX);
	const int minY = std::min(vkbdStartY, vkbdEndY);

	vkbdCurrentX += static_cast<int>(speedX * diffSeconds * sign);
	vkbdCurrentX = std::max(minX, vkbdCurrentX);
	vkbdCurrentX = std::min(maxX, vkbdCurrentX);

	vkbdCurrentY += static_cast<int>(speedY * diffSeconds * sign);
	vkbdCurrentY = std::max(minY, vkbdCurrentY);
	vkbdCurrentY = std::min(maxY, vkbdCurrentY);
}

void vkbd_redraw()
{
	AmigaMonitor* mon = &AMonitors[0];

	if (!vkbdShow && vkbdCurrentX == vkbdStartX && vkbdCurrentY == vkbdStartY)
	{
		return;
	}

	vkbd_update_current_xy();

	SDL_Texture* toDraw = vkbd_get_texture_to_draw();

	if (toDraw == nullptr)
	{
		return;
	}

	int w = 0;
	int h = 0;
	SDL_QueryTexture(toDraw, nullptr, nullptr, &w, &h);
	SDL_Rect rect;
	rect.x = vkbdCurrentX;
	rect.y = vkbdCurrentY;
	rect.w = w;
	rect.h = h;

#ifdef USE_OPENGL
	//TODO needs implementation
#else
	SDL_SetRenderDrawBlendMode(mon->amiga_renderer, SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(toDraw, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(toDraw, vkbdAlpha);
	SDL_RenderCopy(mon->amiga_renderer, toDraw, nullptr, &rect);

	// Store color so we can restore later
	SDL_Color color;
	SDL_GetRenderDrawColor(mon->amiga_renderer, &color.r, &color.g, &color.b, &color.a);

	// Draw currently selected key:
	rect = vkbd_get_key_drawing_rect(vkbdActualIndex);
	const int alpha = static_cast<int>((1.0 - vkbdTransparency) * vkbdPressedKeyColor.a);
	SDL_SetRenderDrawColor(mon->amiga_renderer, vkbdPressedKeyColor.r, vkbdPressedKeyColor.g, vkbdPressedKeyColor.b, alpha);
	SDL_RenderFillRect(mon->amiga_renderer, &rect);

	// Draw sticky keys:
	for (auto key : vkbdPressedStickyKeys)
	{
		const auto index = vkbdStickyKeyToIndex[key];
		rect = vkbd_get_key_drawing_rect(index);
		SDL_RenderFillRect(mon->amiga_renderer, &rect);
	}

	SDL_SetRenderDrawColor(mon->amiga_renderer, color.r, color.g, color.b, color.a);
#endif
}

void vkbd_toggle()
{
	vkbdShow = !vkbdShow;
	vkbdTimeLastRedraw = std::chrono::system_clock::now();
}

bool vkbd_is_active()
{
	return vkbdShow;
}

static bool vkbd_switched_on(int state, int vkbdPreviousState, int mask)
{
	return (state & mask) != (vkbdPreviousState & mask) && (state & mask) != 0;
}

static bool vkbd_switched_off(int state, int vkbdPreviousState, int mask)
{
	return (state & mask) != (vkbdPreviousState & mask) && (state & mask) == 0;
}

static int vkbd_move(const VkbdRect& rect, int direction)
{
	switch (direction)
	{
	case VKBD_UP:
		return rect.up;
	case VKBD_DOWN:
		return rect.down;
	case VKBD_LEFT:
		return rect.left;
	case VKBD_RIGHT:
		return rect.right;
	default:
		return 0;
	}
}

static int vkbd_move(int actualIndex, int direction)
{
	if (actualIndex == vkbdExitButtonIndex)
	{
		return vkbd_move(vkbdExitButtonRect, direction);
	}

	const auto& rect = vkbd_get_key_rect(actualIndex);
	const auto key = rect.key;

	for (const auto kvp : vkbdExitButtonConnections)
	{
		if (kvp.first == key && direction == kvp.second)
		{
			return vkbdExitButtonIndex;
		}
	}

	return vkbd_move(rect, direction);
}

// For simplicity, we don't allow movement (up/down/left/right) when the fire button is pressed.
// For normal (non-sticky) keys button press/release events are send whenever the fire button is released.
// For sticky keys, the key is toggled whenever the fire button goes from not pressed to pressed.
bool vkbd_process(int state, int* keycode, int* pressed)
{
	if (!vkbdShow || vkbdCurrentX != vkbdEndX || vkbdCurrentY != vkbdEndY)
	{
		vkbdPreviousState = 0;
		return false;
	}

	// Check button up event.
	bool result = false;
	if (vkbd_switched_off(state, vkbdPreviousState, VKBD_BUTTON))
	{
		vkbdActualPressed = false;
		const auto actual_key = vkbd_get_key_rect(vkbdActualIndex).key;
		if (vkbdPressedStickyKeys.find(actual_key) == vkbdPressedStickyKeys.end())
		{
			*keycode = actual_key;
			*pressed = 0;
			result = true;
		}
	}

	if ((state & VKBD_BUTTON) == 0)
	{
		for (const auto direction : vkbdDirections)
		{
			const bool on = vkbd_switched_on(state, vkbdPreviousState, direction);
			if (on)
			{
				vkbdActualIndex = vkbd_move(vkbdActualIndex, direction);
			}
		}
	}

	if (vkbd_switched_on(state, vkbdPreviousState, VKBD_BUTTON))
	{
		const auto actual_key = vkbd_get_key_rect(vkbdActualIndex).key;
		if (vkbdStickyKeys.find(actual_key) != vkbdStickyKeys.end())
		{
			vkbdActualPressed = false;
			if (vkbdPressedStickyKeys.find(actual_key) != vkbdPressedStickyKeys.end())
			{
				const auto iter = vkbdPressedStickyKeys.find(actual_key);
				vkbdPressedStickyKeys.erase(iter);
				*keycode = actual_key;
				*pressed = 0;
				result = true;
			}
			else
			{
				vkbdPressedStickyKeys.insert(actual_key);
				*keycode = actual_key;
				*pressed = 1;
				result = true;
			}
		}
		else
		{
			*keycode = actual_key;
			*pressed = 1;
			result = true;
			vkbdActualPressed = true;
		}
	}
	vkbdPreviousState = state;

	return result;
}
