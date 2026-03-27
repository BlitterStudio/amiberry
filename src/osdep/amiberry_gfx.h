#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include "rtgmodes.h"
#include "uae/types.h"
#include <vector>

class IRenderer;

#define MAX_DISPLAYS 10

#define RTG_MODE_SCALE 1
#define RTG_MODE_CENTER 2
#define RTG_MODE_INTEGER_SCALE 3

#ifdef AMIBERRY
extern SDL_PixelFormat pixel_format;
extern uae_u8* p96_get_render_buffer_pointer(int monid);
#endif

#define GUI_WIDTH  860
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
	int depth;   /* depth in bytes-per-pixel */
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
	SDL_DisplayID display_id;
	TCHAR* adaptername, *adapterid, *adapterkey;
	TCHAR* monitorname, *monitorid;
	TCHAR* fullname;
	struct PicassoResolution* DisplayModes;
	SDL_Rect rect;
	SDL_Rect workrect;
	bool HasAdapterData;
};
extern struct MultiDisplay Displays[MAX_DISPLAYS + 1]; // +1 sentinel (matches WinUAE)

struct winuae_currentmode {
	unsigned int flags;
	int native_width, native_height;
	int current_width, current_height;
	int amiga_width, amiga_height;
	int initdone;
	int fullfill;
	int freq;
};

#define MAX_AMIGAMONITORS 4
struct AmigaMonitor {
	int monitor_id;
	bool active;
	SDL_DisplayID assigned_display;
	SDL_Window* amiga_window;
	SDL_Renderer* amiga_renderer;
	SDL_Window* gui_window;
	SDL_Renderer* gui_renderer;
	struct MultiDisplay* md;

	SDL_Surface* amiga_surface;
	std::unique_ptr<IRenderer> renderer;

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
	bool focus_transitioning;
	int pre_focus_x, pre_focus_y;
	int manual_painting_needed;
	int minimized;
	int screen_is_picasso;
	int screen_is_initialized;
	int scalepicasso;
	bool rtg_locked;
	int p96_double_buffer_firstx, p96_double_buffer_lastx;
	int p96_double_buffer_first, p96_double_buffer_last;
	int p96_double_buffer_needs_flushing;

	std::vector<SDL_Rect> dirty_rects;
	bool full_render_needed;
	SDL_Surface* statusline_surface;
	SDL_Texture* statusline_texture;
	struct winuae_currentmode currentmode;
	struct uae_filter* usedfilter;

	// Cached HiDPI scale factors (updated on window resize)
	float hidpi_scale_x = 1.0f;
	float hidpi_scale_y = 1.0f;
	bool hidpi_needs_scaling = false;
};
extern struct AmigaMonitor* amon;
extern struct AmigaMonitor AMonitors[MAX_AMIGAMONITORS];

// SDL3: surface->format is an enum, not a pointer. Cache pixel format details in globals.
extern int system_red_shift, system_green_shift, system_blue_shift;
extern uint32_t system_red_mask, system_green_mask, system_blue_mask;
void update_system_pixel_format();

#define  SYSTEM_RED_SHIFT      system_red_shift
#define  SYSTEM_GREEN_SHIFT    system_green_shift
#define  SYSTEM_BLUE_SHIFT     system_blue_shift
#define  SYSTEM_RED_MASK       system_red_mask
#define  SYSTEM_GREEN_MASK     system_green_mask
#define  SYSTEM_BLUE_MASK      system_blue_mask

extern std::unique_ptr<IRenderer> g_renderer;
extern SDL_Surface* amiga_surface;

inline IRenderer* get_renderer(int monid = 0)
{
	if (monid >= 0 && monid < MAX_AMIGAMONITORS && AMonitors[monid].renderer)
		return AMonitors[monid].renderer.get();
	return g_renderer.get();
}

inline SDL_Surface* get_amiga_surface(int monid = 0)
{
	if (monid >= 0 && monid < MAX_AMIGAMONITORS && AMonitors[monid].amiga_surface)
		return AMonitors[monid].amiga_surface;
	return amiga_surface;
}

extern SDL_DisplayMode sdl_mode;
extern const char* sdl_video_driver;
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
extern void reopen_gfx(AmigaMonitor*);
extern int reopen(AmigaMonitor*, int, bool);
extern void updatewinfsmode(int monid, uae_prefs* p);
extern void gfx_lock();
extern void gfx_unlock();


struct MultiDisplay* getdisplay(const uae_prefs* p, int monid);
extern int getrefreshrate(int monid, int width, int height);
extern void auto_crop_image();
extern bool vkbd_allowed(int monid);
extern float calculate_desired_aspect(const AmigaMonitor* mon);
extern void quit_drawing_thread();
extern void start_drawing_thread();
extern bool target_graphics_buffer_update(const int monid, const bool force);

extern bool force_auto_crop;
extern SDL_GamepadButton vkbd_button;
extern void GetWindowRect(SDL_Window* window, SDL_Rect* rect);
extern bool kmsdrm_detected;
