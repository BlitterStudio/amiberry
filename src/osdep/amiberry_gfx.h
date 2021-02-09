#pragma once
#include <SDL.h>
#include "rtgmodes.h"
#include "uae/types.h"
#include "traps.h"

#define MAX_DISPLAYS 1

#define RTG_MODE_SCALE 1
#define RTG_MODE_CENTER 2
#define RTG_MODE_INTEGER_SCALE 3

#define GUI_WIDTH  800
#define GUI_HEIGHT 600

#define DISPLAY_SIGNAL_SETUP 				1
#define DISPLAY_SIGNAL_SUBSHUTDOWN 			2
#define DISPLAY_SIGNAL_OPEN 				3
#define DISPLAY_SIGNAL_SHOW 				4
#define DISPLAY_SIGNAL_QUIT 				5

struct ScreenResolution
{
	uae_u32 width;  /* in pixels */
	uae_u32 height; /* in pixels */
};

#define MAX_PICASSO_MODES 100
#define MAX_REFRESH_RATES 10

#define REFRESH_RATE_RAW 1
#define REFRESH_RATE_LACE 2

struct PicassoResolution
{
	struct ScreenResolution res;
	int depth;   /* depth in bytes-per-pixel */
	int residx;
	int refresh[MAX_REFRESH_RATES]; /* refresh-rates in Hz */
	int refreshtype[MAX_REFRESH_RATES]; /* 0=normal,1=raw,2=lace */
	TCHAR name[25];
	/* Bit mask of RGBFF_xxx values.  */
	uae_u32 colormodes;
	int rawmode;
	bool lace; // all modes lace
};

struct MultiDisplay {
	int primary;
	TCHAR* adaptername, * adapterid, * adapterkey;
	TCHAR* monitorname, * monitorid;
	TCHAR* fullname;
	struct PicassoResolution* DisplayModes;
	SDL_Rect rect;
	SDL_Rect workrect;
};
extern struct MultiDisplay Displays[MAX_DISPLAYS];

struct winuae_currentmode {
	unsigned int flags;
	int native_width, native_height, native_depth, pitch;
	int current_width, current_height, current_depth;
	int amiga_width, amiga_height;
	int initdone;
	int fullfill;
	int vsync;
	int freq;
};

#define MAX_AMIGAMONITORS 4
struct AmigaMonitor {
	int monitor_id;
	SDL_Window* sdl_window;
	struct MultiDisplay* md;

	SDL_Rect amigawin_rect, mainwin_rect;
	SDL_Rect amigawinclip_rect;
	int window_extra_width, window_extra_height;
	int win_x_diff, win_y_diff;
	int setcursoroffset_x, setcursoroffset_y;
	int mouseposx, mouseposy;
	int windowmouse_max_w;
	int windowmouse_max_h;
	int ratio_width, ratio_height;
	int ratio_adjust_x, ratio_adjust_y;
	bool ratio_sizing;
	int prevsbheight;
	bool render_ok, wait_render;

	int in_sizemove;
	int manual_painting_needed;
	int minimized;
	int screen_is_picasso;
	int screen_is_initialized;
	int scalepicasso;
	bool rtg_locked;
	int p96_double_buffer_firstx, p96_double_buffer_lastx;
	int p96_double_buffer_first, p96_double_buffer_last;
	int p96_double_buffer_needs_flushing;

	struct winuae_currentmode currentmode;
	struct uae_filter* usedfilter;
};
extern struct AmigaMonitor* amon;
extern struct AmigaMonitor AMonitors[MAX_AMIGAMONITORS];

#define  SYSTEM_RED_SHIFT      (sdl_surface->format->Rshift)
#define  SYSTEM_GREEN_SHIFT    (sdl_surface->format->Gshift)
#define  SYSTEM_BLUE_SHIFT     (sdl_surface->format->Bshift)
#define  SYSTEM_RED_MASK       (sdl_surface->format->Rmask)
#define  SYSTEM_GREEN_MASK     (sdl_surface->format->Gmask)
#define  SYSTEM_BLUE_MASK      (sdl_surface->format->Bmask)

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
extern SDL_Texture* amiga_texture;
extern SDL_Cursor* cursor;
extern SDL_DisplayMode sdlMode;
#endif
extern SDL_Surface* sdl_surface;
extern const char* sdl_video_driver;
extern SDL_Renderer* sdl_renderer;
extern SDL_Rect renderQuad;
extern SDL_Cursor* normalcursor;

extern void sortdisplays();
extern void enumeratedisplays(void);

void Display_change_requested(int);
void DX_Invalidate(struct AmigaMonitor*, int x, int y, int width, int height);

extern int default_freq;

extern void check_error_sdl(bool check, const char* message);
extern void toggle_fullscreen();
extern void close_windows(struct AmigaMonitor*);
extern void update_win_fs_mode(int monid, struct uae_prefs* p);
extern void gfx_lock(void);
extern void gfx_unlock(void);

void DX_Fill(struct AmigaMonitor*, int dstx, int dsty, int width, int height, uae_u32 color);
void DX_Blit(int x, int y, int w, int h);
struct MultiDisplay* getdisplay(struct uae_prefs* p, int monid);
extern int getrefreshrate(int monid, int width, int height);
void SDL2_guimode(int monid, int guion);