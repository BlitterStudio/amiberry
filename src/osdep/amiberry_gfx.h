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
#define GUI_HEIGHT 720

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

#define MAX_PICASSO_MODES 300
#define MAX_REFRESH_RATES 100

#define REFRESH_RATE_RAW 1
#define REFRESH_RATE_LACE 2

struct PicassoResolution
{
	bool inuse;
	struct ScreenResolution res;
	int residx;
	int refresh[MAX_REFRESH_RATES]; /* refresh-rates in Hz */
	int refreshtype[MAX_REFRESH_RATES]; /* 0=normal,1=raw,2=lace */
	TCHAR name[25];
	int rawmode;
	bool lace; // all modes lace
};

struct MultiDisplay {
	int primary;
	int monitor;
	TCHAR* adaptername, *adapterid, *adapterkey;
	TCHAR* monitorname, *monitorid;
	TCHAR* fullname;
	struct PicassoResolution* DisplayModes;
	SDL_Rect rect;
	SDL_Rect workrect;
	bool HasAdapterData;
};
extern struct MultiDisplay Displays[MAX_DISPLAYS];

struct winuae_currentmode {
	unsigned int flags;
	int native_width, native_height;
	int current_width, current_height;
	int amiga_width, amiga_height;
	int initdone;
	int fullfill;
	int freq;
};

#define MAX_AMIGAMONITORS 1
struct AmigaMonitor {
	int monitor_id;
	SDL_Window* amiga_window;
	SDL_Renderer* amiga_renderer;
	SDL_Window* gui_window;
	SDL_Renderer* gui_renderer;
	struct MultiDisplay* md;

	SDL_Rect amigawin_rect, mainwin_rect;
	SDL_Rect amigawinclip_rect;
	int window_extra_width, window_extra_height;
	int window_extra_height_bar;
	int win_x_diff, win_y_diff;
	int setcursoroffset_x, setcursoroffset_y;
	int mouseposx, mouseposy;
	int windowmouse_max_w;
	int windowmouse_max_h;
	int ratio_width, ratio_height;
	int ratio_adjust_x, ratio_adjust_y;
	bool ratio_sizing;
	bool render_ok, wait_render;
	int dpi;

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

#define  SYSTEM_RED_SHIFT      (amiga_surface->format->Rshift)
#define  SYSTEM_GREEN_SHIFT    (amiga_surface->format->Gshift)
#define  SYSTEM_BLUE_SHIFT     (amiga_surface->format->Bshift)
#define  SYSTEM_RED_MASK       (amiga_surface->format->Rmask)
#define  SYSTEM_GREEN_MASK     (amiga_surface->format->Gmask)
#define  SYSTEM_BLUE_MASK      (amiga_surface->format->Bmask)

extern SDL_Texture* amiga_texture;
extern SDL_DisplayMode sdl_mode;
extern SDL_Rect crop_rect;

extern SDL_Surface* amiga_surface;
extern const char* sdl_video_driver;
extern SDL_Rect render_quad;
extern SDL_Cursor* normalcursor;

extern void sortdisplays();
extern void enumeratedisplays();
extern void reenumeratemonitors();

extern bool MonitorFromPoint(SDL_Point pt);
void gfx_DisplayChangeRequested(int);
void DX_Invalidate(AmigaMonitor*, int x, int y, int width, int height);
int gfx_adjust_screenmode(const MultiDisplay* md, int* pwidth, int* pheight);

extern int default_freq;

extern void check_error_sdl(bool check, const char* message);
extern void toggle_fullscreen();
extern void close_windows(AmigaMonitor*);
extern void updatewinfsmode(int monid, uae_prefs* p);
extern void gfx_lock();
extern void gfx_unlock();

extern void destroy_crtemu();

struct MultiDisplay* getdisplay(const uae_prefs* p, int monid);
extern int getrefreshrate(int monid, int width, int height);
void SDL2_guimode(int monid, int guion);
void SDL2_toggle_vsync(bool vsync);
extern void auto_crop_image();
extern bool vkbd_allowed(int monid);
extern void quit_drawing_thread();
extern void start_drawing_thread();

extern SDL_GameControllerButton vkbd_button;
extern void GetWindowRect(SDL_Window* window, SDL_Rect* rect);
extern bool kmsdrm_detected;
