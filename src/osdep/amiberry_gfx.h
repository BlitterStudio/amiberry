#pragma once
#include <SDL.h>

#define GUI_WIDTH  800
#define GUI_HEIGHT 600

#define DISPLAY_SIGNAL_SETUP 				1
#define DISPLAY_SIGNAL_SUBSHUTDOWN 			2
#define DISPLAY_SIGNAL_OPEN 				3
#define DISPLAY_SIGNAL_SHOW 				4
#define DISPLAY_SIGNAL_QUIT 				5

#ifdef USE_DISPMANX
#include <bcm_host.h>
extern DISPMANX_DISPLAY_HANDLE_T displayHandle;
extern DISPMANX_MODEINFO_T modeInfo;
extern DISPMANX_UPDATE_HANDLE_T updateHandle;
extern DISPMANX_ELEMENT_HANDLE_T blackscreen_element;
extern VC_RECT_T src_rect;
extern VC_RECT_T dst_rect;
extern VC_RECT_T blit_rect;
extern VC_RECT_T black_rect;
extern VC_IMAGE_TYPE_T rgb_mode;
extern void change_layer_number(int layer);
#else
extern SDL_Surface* screen;
extern SDL_Texture* texture;
extern SDL_Cursor* cursor;
extern SDL_Texture* gui_texture;
extern SDL_DisplayMode sdlMode;
extern const char* sdl_video_driver;
#endif
extern SDL_Renderer* renderer;
extern SDL_Window* sdl_window;
extern SDL_Surface* gui_screen;
extern SDL_Rect renderQuad;

extern bool can_have_linedouble;
extern void check_error_sdl(bool check, const char* message);
extern void toggle_fullscreen();
extern void DX_Fill(int dstx, int dsty, int width, int height, uae_u32 color);