/*
* Amiberry Amiga Emulator
*
* SDL3 interface
* 
* Copyright 2025 Dimitris Panokostas
* 
*/

#include <algorithm>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <cstdio>
#include <cmath>
#include <iostream>

#include "sysdeps.h"
#include "options.h"
#include "uae.h"
#include "custom.h"
#include "events.h"
#include "xwin.h"
#include "keyboard.h"
#include "drawing.h"
#include "picasso96.h"
#include "gui.h"
#include "amiberry_gfx.h"
#include "sounddep/sound.h"
#include "inputdevice.h"
#ifdef WITH_MIDI
#include "midi.h"
#endif
#include "serial.h"
#include "parallel.h"
#include "sampler.h"
#include "gfxboard.h"
#include "statusline.h"
#include "devices.h"

#include "threaddep/thread.h"
#include "vkbd/vkbd.h"
#include "on_screen_joystick.h"
#include "fsdb_host.h"
#include "savestate.h"
#include "uae/types.h"

#ifdef WITH_MIDIEMU
#include "midiemu.h"
#endif

#include "dpi_handler.hpp"
#include "registry.h"
#include "target.h"
#include "gfx_window.h"
#include "gl_overlays.h"
#include "gfx_colors.h"
#include "gfx_prefs_check.h"
#include "display_modes.h"
#include "renderer_factory.h"

#ifdef USE_OPENGL
#include "gl_platform.h"
#include "crtemu.h"
#include "crt_frame.h"
#include "external_shader.h"
#include "shader_preset.h"
#include "opengl_renderer.h"
#include <SDL3_image/SDL_image.h>

#ifdef LIBRETRO
#include "libretro_shared.h"
#endif

#ifndef GL_BGRA
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#else
#define GL_BGRA 0x80E1
#endif
#endif

#else // !USE_OPENGL
#include "sdl_renderer.h"
#endif // USE_OPENGL

#ifdef AMIBERRY
bool force_auto_crop = false;
SDL_DisplayMode sdl_mode;
SDL_Surface* amiga_surface = nullptr;

// Cached pixel format details for SYSTEM_*_SHIFT/MASK macros (SDL3: surface->format is enum, not pointer)
int system_red_shift = 0, system_green_shift = 0, system_blue_shift = 0;
uint32_t system_red_mask = 0, system_green_mask = 0, system_blue_mask = 0;

void update_system_pixel_format()
{	if (amiga_surface) {
		const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(amiga_surface->format);
		if (details) {
			system_red_shift = details->Rshift;
			system_green_shift = details->Gshift;
			system_blue_shift = details->Bshift;
			system_red_mask = details->Rmask;
			system_green_mask = details->Gmask;
			system_blue_mask = details->Bmask;
		}
	}
}

static int dx = 0, dy = 0;
const char* sdl_video_driver;
bool kmsdrm_detected = false;

SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_ABGR8888;

static frame_time_t last_synctime;

std::unique_ptr<IRenderer> g_renderer;

static SDL_Surface* current_screenshot = nullptr;
std::string screenshot_filename;
FILE* screenshot_file = nullptr;
int delay_savestate_frame = 0;
#endif

struct MultiDisplay Displays[MAX_DISPLAYS + 1];
struct AmigaMonitor AMonitors[MAX_AMIGAMONITORS];

void updatepicasso96(struct AmigaMonitor* mon);

#include "gfx_platform_internal.h"

int window_led_drives, window_led_drives_end;
int window_led_hd, window_led_hd_end;
int window_led_joys, window_led_joys_end, window_led_joy_start;
int window_led_msg, window_led_msg_end, window_led_msg_start;

int wasfs[2];
const TCHAR* wasfsname[2] = { _T("FullScreenMode"), _T("FullScreenModeRTG") };

int default_freq = 60;

int vsync_activeheight, vsync_totalheight;
float vsync_vblank, vsync_hblank;
bool beamracer_debug;
bool gfx_hdr;

// TODO: These are WinUAE stubs. With threaded Vulkan rendering, surface
// lifecycle in doInit should be protected if concurrent access is possible.
// Currently mitigated by skipping surface recreation when dimensions match.
void gfx_lock()
{
}
void gfx_unlock()
{
}

#ifdef AMIBERRY
float amiberry_getrefreshrate(const int monid)
{
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (!displays || monid >= count)
	{
		SDL_free(displays);
		write_log("SDL_GetDisplays failed or monid out of range\n");
		return 0;
	}
	const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displays[monid]);
	SDL_free(displays);
	if (!mode)
	{
		write_log("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
		return 0;
	}
	return mode->refresh_rate;
}

void GetWindowRect(SDL_Window* window, SDL_Rect* rect)
{
	SDL_GetWindowPosition(window, &rect->x, &rect->y);
	SDL_GetWindowSize(window, &rect->w, &rect->h);
}

bool MonitorFromPoint(const SDL_Point pt)
{
	SDL_Rect display_bounds;
	bool point_in_display = false;
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (displays) {
		for (int i = 0; i < count; ++i) {
			if (SDL_GetDisplayBounds(displays[i], &display_bounds)) {
				if (SDL_PointInRect(&pt, &display_bounds)) {
					point_in_display = true;
					break;
				}
			}
		}
		SDL_free(displays);
	}
	return point_in_display;
}

#endif

#ifdef AMIBERRY
bool vkbd_allowed(const int monid)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	return currprefs.vkbd_enabled && !mon->screen_is_picasso;
}

static bool amiberry_renderframe(const int monid, int mode, int immediate)
{
	// Skip all rendering if in headless mode
	if (currprefs.headless) {
		return amiga_surface != nullptr;
	}
	if (gfx_platform_skip_renderframe(monid, mode, immediate))
		return amiga_surface != nullptr;

	const amigadisplay* ad = &adisplays[monid];
	// Unified OSD update: handle both native (CHIPSET) and RTG modes
	if (((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
		((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on))
	{
		update_leds(monid);
	}
#ifdef WITH_THREADED_CPU
	if (ad->picasso_zero_copy_update_needed) {
		const_cast<amigadisplay*>(ad)->picasso_zero_copy_update_needed = false;
		if (currprefs.rtg_zerocopy) {
			target_graphics_buffer_update(monid, true);
		}
	}
#endif
	IRenderer* renderer = get_renderer(monid);
	return renderer ? renderer->render_frame(monid, mode, immediate) : false;
}

void flush_screen(struct vidbuffer* vb, int y_start, int y_end)
{
	AmigaMonitor* mon = &AMonitors[vb->monitor_id];
	mon->full_render_needed = true;
}

static void amiberry_refresh(const int monid)
{
	amiberry_renderframe(monid, 1, 1);
	IRenderer* renderer = get_renderer(monid);
	if (renderer)
		renderer->present_frame(monid, 0);
}
#endif //AMIBERRY

void getgfxoffset(const int monid, float* dxp, float* dyp, float* mxp, float* myp)
{
	const amigadisplay* ad = &adisplays[monid];
	float dx = 0, dy = 0, mx = 1.0f, my = 1.0f;
	float src_w = 0, src_h = 0;
	float src_x = 0, src_y = 0;

#ifdef AMIBERRY
	IRenderer* renderer = get_renderer(monid);
	if (currprefs.gfx_auto_crop && !ad->picasso_on && renderer) {
		src_w = static_cast<float>(renderer->crop_rect.w);
		src_h = static_cast<float>(renderer->crop_rect.h);
		src_x = static_cast<float>(renderer->crop_rect.x);
		src_y = static_cast<float>(renderer->crop_rect.y);
	}
#endif
	if (src_w <= 0) {
		SDL_Surface* surface = get_amiga_surface(monid);
		if (surface) {
			src_w = static_cast<float>(surface->w);
			src_h = static_cast<float>(surface->h);
			src_x = 0;
			src_y = 0;
		}
	}

	if (renderer)
		renderer->get_gfx_offset(monid, src_w, src_h, src_x, src_y, &dx, &dy, &mx, &my);

	*dxp = dx;
	*dyp = dy;
	*mxp = 1.0f / mx;
	*myp = 1.0f / my;
}


bool render_screen(const int monid, const int mode, const bool immediate)
{
	// Skip if headless mode
	if (currprefs.headless) {
		return true;
	}

	struct AmigaMonitor* mon = &AMonitors[monid];
	const struct amigadisplay* ad = &adisplays[monid];
	int cnt;

	mon->render_ok = false;
	if (ad->picasso_on || monitor_off) {
		return mon->render_ok;
	}
	cnt = 0;
	while (mon->wait_render) {
		sleep_millis(1);
		cnt++;
		if (cnt > 500) {
			return mon->render_ok;
		}
	}
	mon->render_ok = amiberry_renderframe(monid, mode, immediate);
	return mon->render_ok;
}

bool show_screen_maybe(const int monid, const bool show)
{
	const struct amigadisplay* ad = &adisplays[monid];
	const struct apmode* ap = ad->picasso_on ? &currprefs.gfx_apmode[APMODE_RTG] : &currprefs.gfx_apmode[APMODE_NATIVE];
	if (!ap->gfx_vflip || ap->gfx_vsyncmode == 0 || ap->gfx_vsync <= 0) {
		if (show)
			show_screen(monid, 0);
		return false;
	}
	return false;
}

float target_adjust_vblank_hz(const int monid, float hz)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	int maxrate;
	if (!currprefs.lightboost_strobo)
		return hz;
	if (isfullscreen() > 0) {
		maxrate = mon->currentmode.freq;
	} else {
		maxrate = deskhz;
	}
	double nhz = hz * 2.0;
	if (nhz >= maxrate - 1 && nhz < maxrate + 1)
		hz -= 0.5f;
	return hz;
}

float calculate_desired_aspect(const AmigaMonitor* mon)
{
	SDL_Surface* surface = get_amiga_surface(mon->monitor_id);

	if (mon->screen_is_picasso && surface) {
		return (float)surface->w / (float)surface->h;
	}

	if (currprefs.gfx_correct_aspect) {
		return 4.0f / 3.0f;
	}

	if (surface) {
		return (float)surface->w / (float)surface->h;
	}

	return 4.0f / 3.0f;
}

void show_screen(const int monid, int mode)
{
	// Skip all rendering if headless mode
	if (currprefs.headless) {
		return;
	}

	AmigaMonitor* mon = &AMonitors[monid];
	if (gfx_platform_requires_window() && !mon->amiga_window) {
		return;
	}

	if (mode >= 0 && !mon->render_ok) {
		return;
	}

	IRenderer* renderer = get_renderer(monid);
	if (renderer && !renderer->has_valid_shader()) {
		return;
	}

	if (!renderer) {
		return;
	}

	renderer->present_frame(monid, mode);
	mon->render_ok = false;
}

int lockscr(struct vidbuffer* vb, bool fullupdate, bool skip)
{
	const struct AmigaMonitor* mon = &AMonitors[vb->monitor_id];
	int ret = 0;

	SDL_Surface* surface = get_amiga_surface(vb->monitor_id);

	if ((gfx_platform_requires_window() && !mon->amiga_window) || !surface)
		return ret;

	// Ensure blanking limits are open and synchronized at the start of frame locking
	set_custom_limits(-1, -1, -1, -1, false);

	if (vb->vram_buffer) {
		// Benchmarks have shown that Locking and Unlocking the Texture is slower than just calling UpdateTexture
		// Therefore, this is disabled in Amiberry.
		//
		//SDL_LockTexture(amiga_texture, nullptr, reinterpret_cast<void**>(&vb->bufmem), &vb->rowbytes);

		vb->bufmem = static_cast<uae_u8*>(surface->pixels);
		vb->rowbytes = surface->pitch;
		vb->width_allocated = surface->w;
		vb->height_allocated = surface->h;
		if (vb->bufmem) {
			ret = 1;
		}
	} else {
		ret = 1;
	}
	if (ret) {
		vb->locked = true;
	}
	return ret;
}

static void add_dirty_rect(struct AmigaMonitor* mon, const SDL_Rect& new_rect)
{
	// If the whole screen is already marked for update, do nothing.
	if (mon->full_render_needed) {
		return;
	}

	// Attempt to merge with the last rectangle if they are contiguous.
	if (!mon->dirty_rects.empty()) {
		SDL_Rect& last_rect = mon->dirty_rects.back();
		// Check if new_rect starts exactly where last_rect ends and they have the same width.
		if (last_rect.x == new_rect.x && last_rect.w == new_rect.w && (last_rect.y + last_rect.h) == new_rect.y) {
			// Merge by extending the height of the last rectangle.
			last_rect.h += new_rect.h;
			return;
		}
	}

	// If we have too many separate rectangles, it's more efficient to just do a full update.
	if (mon->dirty_rects.size() > 32) {
		mon->full_render_needed = true;
		mon->dirty_rects.clear();
		return;
	}

	mon->dirty_rects.push_back(new_rect);
}

void unlockscr(struct vidbuffer* vb, int y_start, int y_end)
{
	vb->locked = false;
	if (vb->vram_buffer) {
		vb->bufmem = nullptr;
		// Not using SDL_UnlockTexture due to performance reasons, see lockscr for details
		//SDL_UnlockTexture(amiga_texture);

		// Record the dirty rectangle if y_start and y_end are valid.
		if (y_start >= 0 && y_end >= y_start) {
			AmigaMonitor* mon = &AMonitors[vb->monitor_id];
			SDL_Surface* surface = get_amiga_surface(vb->monitor_id);
			
			int clamped_y_end = y_end;
			if (surface && clamped_y_end >= surface->h) {
				clamped_y_end = surface->h - 1;
			}
			
			if (clamped_y_end >= y_start) {
				SDL_Rect dirty_rect;
				dirty_rect.x = 0;
				dirty_rect.y = y_start;
				dirty_rect.w = vb->width_allocated;
				dirty_rect.h = clamped_y_end - y_start + 1;

				add_dirty_rect(mon, dirty_rect);
			}
		}
	}
}

void flush_clear_screen(struct vidbuffer* vb)
{
	if (!vb)
		return;

	// Early exit if buffer is not properly initialized
	if (!vb->bufmem || vb->width_allocated <= 0 || vb->height_allocated <= 0)
		return;

	if (lockscr(vb, true, false)) {
		// Use memset for better performance on aligned memory
		const size_t bytes_per_row = vb->width_allocated * vb->pixbytes;
		const size_t total_bytes = bytes_per_row * vb->height_allocated;

		if (vb->rowbytes == bytes_per_row) {
			// Buffer is contiguous, clear it all at once.
			memset(vb->bufmem, 0, total_bytes);
		} else {
			// Buffer is not contiguous, clear row by row.
			for (int y = 0; y < vb->height_allocated; y++)
				memset(static_cast<uae_u8*>(vb->bufmem) + y * vb->rowbytes, 0, bytes_per_row);
		}
		unlockscr(vb, -1, -1);
	}
}

float filterrectmult(int v1, float v2, int dmode)
{
	const float v = v1 / v2;
	const int vv = static_cast<int>(v + 0.5f);
	if (v > 1.5f && vv * v2 <= v1 && vv * (v2 + vv - 1) >= v1) {
		return static_cast<float>(vv);
	}
	if (!dmode) {
		return v;
	}
	if (v > 0.2f && v < 0.3f) {
		return 0.25f;
	}
	if (v > 0.4f && v < 0.6f) {
		return 0.5f;
	}
	return static_cast<float>(static_cast<int>(v + 0.5f));
}

static uae_u8* gfx_lock_picasso2(int monid, bool fullupdate)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[monid];
	uae_u8* p;

	SDL_Surface* surface = get_amiga_surface(monid);
	if (surface == nullptr) {
		return nullptr;
	}

	if (currprefs.rtg_zerocopy) {
		uae_u8* rtg_vram = p96_get_render_buffer_pointer(monid);
		if (rtg_vram != nullptr && surface->pixels == rtg_vram) {
			return nullptr;
		}
	}


	const auto* fmt = SDL_GetPixelFormatDetails(surface->format);
	if (!fmt) {
		write_log("gfx_lock_picasso: SDL_GetPixelFormatDetails failed\n");
		return nullptr;
	}
	vidinfo->pixbytes = fmt->bytes_per_pixel;
	vidinfo->rowbytes = surface->pitch;
	vidinfo->maxwidth = surface->w;
	vidinfo->maxheight = surface->h;
	p = static_cast<uae_u8*>(surface->pixels);
	return p;
}
uae_u8* gfx_lock_picasso(const int monid, bool fullupdate)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	if (mon->rtg_locked) {
		return mon->rtg_locked_ptr;
	}
	uae_u8* p = gfx_lock_picasso2(monid, fullupdate);
	if (p) {
		mon->rtg_locked = true;
		mon->rtg_locked_ptr = p;
	}
	return p;
}

void gfx_unlock_picasso(const int monid, const bool dorender)
{
	struct AmigaMonitor* mon = &AMonitors[monid];

	mon->rtg_locked = false;
	if (dorender) {
		if (mon->p96_double_buffer_needs_flushing) {
			//D3D_flushtexture(monid, mon->p96_double_buffer_first, mon->p96_double_buffer_last);
			mon->p96_double_buffer_needs_flushing = 0;
		}
	}
	//SDL_UnlockTexture(amiga_texture);
	if (dorender) {
		if (amiberry_renderframe(monid, true, false)) {
			mon->render_ok = true;
			show_screen_maybe(monid, true);
		}
	}
}

#ifdef PICASSO96

void DX_Invalidate(struct AmigaMonitor* mon, int x, int y, int width, int height)
{
	const struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	int last, lastx;

	if (width == 0 || height == 0)
		return;
	if (y < 0 || height < 0) {
		y = 0;
		height = vidinfo->height;
	}
	if (x < 0 || width < 0) {
		x = 0;
		width = vidinfo->width;
	}

	// Performance Optimization: Bridge Picasso96 invalidation to the SDL dirty rect system.
	// This allows the renderer to only upload the modified portion of the RTG screen.
	SDL_Rect dirty_rect;
	dirty_rect.x = x;
	dirty_rect.y = y;
	dirty_rect.w = width;
	dirty_rect.h = height;
	add_dirty_rect(mon, dirty_rect);

	last = y + height - 1;
	lastx = x + width - 1;
	mon->p96_double_buffer_first = y;
	mon->p96_double_buffer_last = last;
	mon->p96_double_buffer_firstx = x;
	mon->p96_double_buffer_lastx = lastx;
	mon->p96_double_buffer_needs_flushing = 1;
}

#endif

bool vsync_switchmode(const int monid, int hz)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	
	// In Full-Window mode or if using Adaptive VSync options, 
	// we do not need to strictly match/switch resolution refresh rates.
	// We accept the current state as valid.
	if (isfullscreen() < 0 || currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync > 1) {
		return true;
	}

	static struct PicassoResolution* oldmode[MAX_AMIGAMONITORS];
	static int oldhz[MAX_AMIGAMONITORS];
	int w = mon->currentmode.native_width;
	int h = mon->currentmode.native_height;
	struct MultiDisplay* md = getdisplay(&currprefs, monid);
	struct PicassoResolution* found;
	int newh, cnt;
	bool preferdouble = false, preferlace = false;
	bool lace = false;

	if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate > 85) {
		preferdouble = true;
	} else if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced) {
		preferlace = true;
	}

	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

	newh = h * (currprefs.ntscmode ? 60 : 50) / hz;

	found = nullptr;
	for (cnt = 0; cnt <= abs(newh - h) + 1 && !found; cnt++) {
		for (int dbl = 0; dbl < 2 && !found; dbl++) {
			bool doublecheck = false;
			bool lacecheck = false;
			if (preferdouble && dbl == 0)
				doublecheck = true;
			else if (preferlace && dbl == 0)
				lacecheck = true;

			for (int extra = 1; extra >= -1 && !found; extra--) {
				for (int i = 0; md->DisplayModes[i].inuse && !found; i++) {
					struct PicassoResolution* r = &md->DisplayModes[i];
					if (r->res.width == w && (r->res.height == newh + cnt || r->res.height == newh - cnt)) {
						for (int j = 0; r->refresh[j] > 0; j++) {
							if (doublecheck) {
								if (r->refreshtype[j] & REFRESH_RATE_LACE)
									continue;
								if (r->refresh[j] == hz * 2 + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							} else if (lacecheck) {
								if (!(r->refreshtype[j] & REFRESH_RATE_LACE))
									continue;
								if (r->refresh[j] * 2 == hz + extra) {
									found = r;
									lace = true;
									hz = r->refresh[j];
									break;
								}
							} else {
								if (r->refresh[j] == hz + extra) {
									found = r;
									hz = r->refresh[j];
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	if (found == oldmode[monid] && hz == oldhz[monid])
		return true;
	oldmode[monid] = found;
	oldhz[monid] = hz;
	if (!found) {
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync = 0;
		if (currprefs.gfx_apmode[APMODE_NATIVE].gfx_vsync != changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_vsync) {
			set_config_changed();
		}
		write_log(_T("refresh rate changed to %d%s but no matching screenmode found, vsync disabled\n"), hz, lace ? _T("i") : _T("p"));
		return false;
	} else {
		newh = static_cast<int>(found->res.height);
		changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height = newh;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate = hz;
		changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_interlaced = lace;
		if (changed_prefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height != currprefs.gfx_monitor[mon->monitor_id].gfx_size_fs.height ||
			changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate != currprefs.gfx_apmode[APMODE_NATIVE].gfx_refreshrate) {
			write_log(_T("refresh rate changed to %d%s, new screenmode %dx%d\n"), hz, lace ? _T("i") : _T("p"), w, newh);
			set_config_changed();
		}
		return true;
	}
}

void vsync_clear()
{
	IRenderer* renderer = get_renderer(0);
	if (renderer)
		renderer->vsync_state().active = false;
}

int vsync_isdone(frame_time_t* dt)
{
	if (isvsync() == 0)
		return -1;
	IRenderer* renderer = get_renderer(0);
	if (!renderer || renderer->vsync_state().waitvblankthread_mode <= 0)
		return -2;
	if (dt)
		*dt = renderer->vsync_state().wait_vblank_timestamp;
	return renderer->vsync_state().active ? 1 : 0;
}

#ifdef PICASSO96

static int modeswitchneeded(const struct AmigaMonitor* mon, const struct winuae_currentmode* wc)
{
	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];

	if (isfullscreen() > 0) {
		/* fullscreen to fullscreen */
		if (mon->screen_is_picasso) {
			if (state->Width < wc->current_width && state->Height < wc->current_height) {
				if (currprefs.gf[GF_RTG].gfx_filter_autoscale == 1 || (currprefs.gf[GF_RTG].gfx_filter_autoscale == 2 && currprefs.rtgallowscaling))
					return 0;
			}
			if (state->Width != wc->current_width ||
				state->Height != wc->current_height)
				return 1;
			if (state->Width == wc->current_width &&
				state->Height == wc->current_height) {
			}
			return 1;
		} else {
			if (mon->currentmode.current_width != wc->current_width ||
				mon->currentmode.current_height != wc->current_height)
				return -1;
		}
	} else if (isfullscreen () == 0) {
		/* windowed to windowed */
		return -2;
	} else {
		/* fullwindow to fullwindow */
		if (mon->screen_is_picasso) {
			if (currprefs.gf[GF_RTG].gfx_filter_autoscale && ((wc->native_width > state->Width && wc->native_height >= state->Height) || (wc->native_height > state->Height && wc->native_width >= state->Width)))
				return -1;
			if (currprefs.rtgallowscaling && (state->Width != wc->native_width || state->Height != wc->native_height))
				return -1;
		}
		return -1;
	}
	return 0;
}

void gfx_set_picasso_state(const int monid, const int on)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct winuae_currentmode wc;
	struct apmode* newmode, * oldmode;
	struct gfx_filterdata* newf, * oldf;
	int mode;

	if (mon->screen_is_picasso == on)
		return;
	mon->screen_is_picasso = on;
#ifdef RETROPLATFORM
	rp_rtg_switch();
#endif
	memcpy(&wc, &mon->currentmode, sizeof(wc));

	newmode = &currprefs.gfx_apmode[on ? 1 : 0];
	oldmode = &currprefs.gfx_apmode[on ? 0 : 1];

	newf = &currprefs.gf[on ? 1 : 0];
	oldf = &currprefs.gf[on ? 0 : 1];

	updatemodes(mon);
	update_gfxparams(mon);

	// if filter changes, need to reset
	mode = 0;
	if (newf->gfx_filter != oldf->gfx_filter)
		mode = -1;
	for (int i = 0; i <= 2 * MAX_FILTERSHADERS; i++) {
		if (_tcscmp(newf->gfx_filtershader[i], oldf->gfx_filtershader[i]))
			mode = -1;
		if (_tcscmp(newf->gfx_filtermask[i], oldf->gfx_filtermask[i]))
			mode = -1;
	}
	const bool differentmonitor = getdisplay2(&currprefs, newmode->gfx_display - 1) != getdisplay2(&currprefs, oldmode->gfx_display - 1);
	// if screen parameter changes, need to reopen window
	if (newmode->gfx_fullscreen != oldmode->gfx_fullscreen ||
		(newmode->gfx_fullscreen && (
			newmode->gfx_backbuffers != oldmode->gfx_backbuffers ||
			differentmonitor ||
			newmode->gfx_refreshrate != oldmode->gfx_refreshrate ||
			newmode->gfx_strobo != oldmode->gfx_strobo ||
			newmode->gfx_vflip != oldmode->gfx_vflip ||
			newmode->gfx_vsync != oldmode->gfx_vsync))) {
		mode = 1;
	}
	if (mode <= 0) {
		const int m = modeswitchneeded(mon, &wc);
		if (m > 0)
			mode = m;
		if (m < 0 && !mode)
			mode = m;
		if (!mode)
			return;
	}
	if (mode < 0) {
		open_windows(mon, true, true);
	} else {
		open_screen(mon); // reopen everything
	}

#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif
}

void updatepicasso96(struct AmigaMonitor* mon)
{
	struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
	vidinfo->rowbytes = 0;
	vidinfo->pixbytes = 32 / 8;
	vidinfo->rgbformat = 0;
	vidinfo->extra_mem = 1;
	vidinfo->height = mon->currentmode.current_height;
	vidinfo->width = mon->currentmode.current_width;
	vidinfo->offset = 0;
	vidinfo->splitypos = -1;
}

void gfx_set_picasso_modeinfo(const int monid, const RGBFTYPE rgbfmt)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct picasso96_state_struct* state = &picasso96_state[mon->monitor_id];
	if (!mon->screen_is_picasso)
		return;
	gfx_set_picasso_colors(monid, rgbfmt);
	const int need = modeswitchneeded(mon, &mon->currentmode);
	update_gfxparams(mon);
	updatemodes(mon);
	if (need > 0) {
		open_screen(mon);
	} else if (need < 0) {
		open_windows(mon, true, true);
	}
	state->ModeChanged = false;
#ifdef RETROPLATFORM
	rp_set_hwnd_delayed();
#endif
	target_graphics_buffer_update(monid, false);
}


static void gfxmode_reset(int monid)
{
}
#endif

int machdep_init()
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
		struct AmigaMonitor* mon = &AMonitors[i];
		struct amigadisplay* ad = &adisplays[i];
		mon->monitor_id = i;
		ad->picasso_requested_on = false;
		ad->picasso_on = false;
		mon->screen_is_picasso = 0;
		memset(&mon->currentmode, 0, sizeof(mon->currentmode));
	}
#ifdef LOGITECHLCD
	lcd_open();
#endif
	return 1;
}

void machdep_free()
{
#ifdef LOGITECHLCD
	lcd_close();
#endif
}

int graphics_init(bool mousecapture)
{
	// Create the renderer backend if it doesn't exist yet
	if (!g_renderer) {
		g_renderer = create_renderer();
	}

	// Skip all graphics initialization if running in headless mode
	if (currprefs.headless) {
		write_log("Headless mode: Skipping graphics initialization.\n");
		g_renderer->vsync_state().wait_vblank_timestamp = read_processor_time();
		update_pixel_format();
		if (amiga_surface == nullptr) {
			amiga_surface = SDL_CreateSurface(1920, 1080, pixel_format);
			if (!amiga_surface) {
				write_log("Failed to create amiga_surface: %s\n", SDL_GetError());
				return false;
			}
			update_system_pixel_format();
		}
		return amiga_surface != nullptr;
	}

	g_renderer->vsync_state().wait_vblank_timestamp = read_processor_time();
	update_pixel_format();
	AMonitors[0].active = true;
	gfxmode_reset(0);
	if (open_windows(&AMonitors[0], mousecapture, false)) {
		if (currprefs.monitoremu_mon > 0 && currprefs.monitoremu) {
			gfxmode_reset(currprefs.monitoremu_mon);
			open_windows(&AMonitors[currprefs.monitoremu_mon], mousecapture, false);
		}
		return true;
	}
	return false;
}

int graphics_setup()
{
#ifdef PICASSO96
	InitPicasso96(0);
#endif
	return 1;
}

void graphics_leave()
{
	for (int i = 0; i < MAX_AMIGAMONITORS; i++)
	{
		close_windows(&AMonitors[i], true);
	}
	g_renderer.reset();
}



// Helper: Compute scaled dimensions for aspect ratio correction
// RTG modes use 1:1 scaling, Native modes scale based on resolution settings
static void compute_scaled_dimensions(const int w, const int h, const bool is_rtg, int& scaled_w, int& scaled_h)
{
	if (is_rtg) {
		scaled_w = w;
		scaled_h = h;
		return;
	}

	scaled_w = w;
	scaled_h = h;

	if (currprefs.gfx_vresolution == VRES_NONDOUBLE) {
		if (currprefs.gfx_resolution == RES_HIRES || currprefs.gfx_resolution == RES_SUPERHIRES)
			scaled_h *= 2;
	} else {
		if (currprefs.gfx_resolution == RES_LORES)
			scaled_w *= 2;
	}

	if (vblank_hz > 55.0f)
		scaled_h = scaled_h * 6 / 5;
}

// Helper: Configure render_quad and crop_rect based on mode and crop settings
// For RTG: direct 1:1 mapping
// For Native: respects manual crop settings, auto_crop is handled elsewhere
static void configure_render_rects(const int monid, const int w, const int h, const int scaled_w, const int scaled_h, const bool is_rtg)
{
	IRenderer* renderer = get_renderer(monid);
	auto& rq = renderer->render_quad;
	auto& cr = renderer->crop_rect;

	if (is_rtg) {
		rq = { dx, dy, w, h };
		cr = { dx, dy, w, h };
		return;
	}

	if (currprefs.gfx_manual_crop) {
		rq = { dx, dy, scaled_w, scaled_h };
		cr = {
			currprefs.gfx_horizontal_offset,
			currprefs.gfx_vertical_offset,
			(currprefs.gfx_manual_crop_width > 0) ? currprefs.gfx_manual_crop_width : w,
			(currprefs.gfx_manual_crop_height > 0) ? currprefs.gfx_manual_crop_height : h
		};
		int crop_scaled_w, crop_scaled_h;
		compute_scaled_dimensions(cr.w, cr.h, false, crop_scaled_w, crop_scaled_h);
		if (currprefs.gfx_correct_aspect == 0) {
			crop_scaled_w = sdl_mode.w;
			crop_scaled_h = sdl_mode.h;
		}
		renderer->crop_aspect = (crop_scaled_h > 0)
			? static_cast<float>(crop_scaled_w) / static_cast<float>(crop_scaled_h) : 0.0f;
	}
	else if (!currprefs.gfx_auto_crop) {
		rq = { dx, dy, scaled_w, scaled_h };
		cr = { dx, dy, w, h };
	}
}

bool target_graphics_buffer_update(const int monid, const bool force)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	struct vidbuf_description* avidinfo = &adisplays[monid].gfxvidinfo;
	const struct picasso96_state_struct* state = &picasso96_state[monid];
	struct vidbuffer *vb = NULL, *vbout = NULL;

	// Use per-monitor surface for monid > 0, global for monid == 0
	SDL_Surface*& surface_ref = (monid > 0) ? mon->amiga_surface : amiga_surface;

	int w, h;

	if (mon->screen_is_picasso) {
		w = state->Width;
		h = state->Height;
		update_pixel_format();
	} else {
		update_pixel_format();
		vb = avidinfo->inbuffer;
		vbout = avidinfo->outbuffer;
		if (!vb) {
			return false;
		}
		w = vb->outwidth;
		h = vb->outheight;
	}

	if (!force && oldtex_w[monid] == w && oldtex_h[monid] == h && oldtex_rtg[monid] == mon->screen_is_picasso && surface_ref && surface_ref->format == pixel_format) {
		bool skip_update = true;
		if (mon->screen_is_picasso) {
			uae_u8* rtg_ptr = p96_get_render_buffer_pointer(mon->monitor_id);
			if (rtg_ptr && surface_ref->pixels != rtg_ptr) {
				skip_update = false;
			} else {
				// write_log("GFX: Skipping update. Pointers match: %p\n", rtg_ptr);
			}
		}

		if (skip_update) {
			IRenderer* renderer = get_renderer(mon->monitor_id);
			if (renderer && renderer->alloc_texture(mon->monitor_id, -w, -h))
			{
				//osk_setup(monid, -2);
				if (vbout) {
					vbout->width_allocated = w;
					vbout->height_allocated = h;
				}
				if (force) {
					mon->full_render_needed = true;
				}
				return false;
			}
		}
	}

	if (!w || !h) {
		oldtex_w[monid] = w;
		oldtex_h[monid] = h;
		oldtex_rtg[monid] = mon->screen_is_picasso;
		
		// Even if buffer dimensions aren't ready yet, we need to ensure the shader is created
		// for native mode. Use the surface dimensions that doInit already set up.
		// This is critical for RTG→Native switches where the shader must be recreated for native mode.
		IRenderer* shader_renderer = get_renderer(mon->monitor_id);
		if (shader_renderer && !mon->screen_is_picasso && surface_ref) {
			shader_renderer->alloc_texture(mon->monitor_id, surface_ref->w, surface_ref->h);
		}
		return false;
	}

	// Ensure surface is in sync with the texture size and format
	// Zero-Copy Eligibility Check
	uae_u8* rtg_render_ptr = p96_get_render_buffer_pointer(mon->monitor_id);
	bool is_zero_copy_eligible = false;
	
	if (mon->screen_is_picasso && currprefs.rtgboards[0].rtgmem_type < GFXBOARD_HARDWARE && currprefs.rtg_zerocopy) {
		int p96_bpp = state->BytesPerPixel;

		int host_bpp = SDL_BYTESPERPIXEL(pixel_format);

		if (rtg_render_ptr != nullptr && p96_bpp == host_bpp && pixel_format == SDL_PIXELFORMAT_ABGR8888) {
			is_zero_copy_eligible = true;
		}
	}

	// Ensure surface is in sync with the texture size, format, and memory pointer (if zero-copy)
	bool recreate_surface = (surface_ref == nullptr || surface_ref->w != w || surface_ref->h != h || surface_ref->format != pixel_format);

	// For zero-copy, also check if pitch matches the RTG state
	if (!recreate_surface && is_zero_copy_eligible && surface_ref->pitch != state->BytesPerRow) {
		recreate_surface = true;
	}
	if (surface_ref && is_zero_copy_eligible && surface_ref->pixels != (void*)rtg_render_ptr) {
		recreate_surface = true;
	}
	// If Zero-Copy is disabled, but we are still pointing to VRAM, we must recreate the surface
	if (surface_ref && !is_zero_copy_eligible && rtg_render_ptr && surface_ref->pixels == rtg_render_ptr) {
		recreate_surface = true;
	}

	if (recreate_surface) {
		if (surface_ref) {
			SDL_DestroySurface(surface_ref);
			surface_ref = nullptr;
		}
		if (is_zero_copy_eligible) {
			// Zero-Copy: Create surface from existing memory (rtg_render_ptr guaranteed non-null)
			surface_ref = SDL_CreateSurfaceFrom(w, h, pixel_format, rtg_render_ptr, state->BytesPerRow);
		} else {
			// Normal copy: Create fresh surface
			surface_ref = SDL_CreateSurface(w, h, pixel_format);
		}

		if (surface_ref) {
			SDL_SetSurfaceBlendMode(surface_ref, SDL_BLENDMODE_NONE);
			update_system_pixel_format();
		} else {
			write_log("!!! Failed to create surface for monid %d.\n", monid);
			return false;
		}
		mon->full_render_needed = true;
		struct picasso_vidbuf_description* vidinfo = &picasso_vidinfo[mon->monitor_id];
		const auto* fmt = SDL_GetPixelFormatDetails(surface_ref->format);
		vidinfo->pixbytes = fmt ? fmt->bytes_per_pixel : 4;
#ifndef PICASSO_STATE_SETDAC
#define PICASSO_STATE_SETDAC 8
#endif
		atomic_or(&vidinfo->picasso_state_change, PICASSO_STATE_SETDAC);
	}

	// Allocate/update texture (single call regardless of surface recreation)
	IRenderer* texture_renderer = get_renderer(mon->monitor_id);
	if (!texture_renderer || !texture_renderer->alloc_texture(mon->monitor_id, w, h)) {
		return false;
	}

	// Update video buffer dimensions (consolidated)
	if (vbout) {
		vbout->width_allocated = w;
		vbout->height_allocated = h;
	}

	if (avidinfo->inbuffer != avidinfo->outbuffer) {
		if (avidinfo->outbuffer) {
			avidinfo->outbuffer->inwidth = w;
			avidinfo->outbuffer->inheight = h;
			avidinfo->outbuffer->width_allocated = w;
			avidinfo->outbuffer->height_allocated = h;
		}
	}

	oldtex_w[monid] = w;
	oldtex_h[monid] = h;
	oldtex_rtg[monid] = mon->screen_is_picasso;
	//osk_setup(monid, -2);

	write_log(_T("Buffer %d size (%d*%d) %s\n"), monid, w, h, mon->screen_is_picasso ? _T("RTG") : _T("Native"));

	// Compute scaled dimensions for aspect ratio correction
	int scaled_width, scaled_height;
	compute_scaled_dimensions(w, h, mon->screen_is_picasso, scaled_width, scaled_height);

	configure_render_rects(monid, w, h, scaled_width, scaled_height, mon->screen_is_picasso);
	IRenderer* renderer = get_renderer(monid);
	if (renderer) {
		renderer->set_scaling(monid, &currprefs, scaled_width, scaled_height);
	}

	return true;
}

static void updatedisplayarea2(const int monid)
{
	const struct AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->screen_is_initialized)
		return;
	amiberry_refresh(monid);
}

void updatedisplayarea(const int monid)
{
	set_custom_limits(-1, -1, -1, -1, false);
	if (monid >= 0) {
		updatedisplayarea2(monid);
	} else {
		for (int i = 0; i < MAX_AMIGAMONITORS; i++) {
			updatedisplayarea2(i);
		}
	}
}

void updatewinfsmode(const int monid, struct uae_prefs* p)
{
	struct MultiDisplay* md;
	struct amigadisplay* ad = &adisplays[monid];

	fixup_prefs_dimensions(p);
	int fs = isfullscreen_2(p);
	if (fs != 0) {
		p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_fs;
	} else {
		p->gfx_monitor[monid].gfx_size = p->gfx_monitor[monid].gfx_size_win;
	}

	int* wfw = &wasfs[ad->picasso_on ? 1 : 0];
	const TCHAR* wfwname = wasfsname[ad->picasso_on ? 1 : 0];
	if (fs != *wfw && fs != 0) {
		*wfw = fs;
		regsetint(NULL, wfwname, *wfw);
	}

	md = getdisplay(p, monid);
	set_config_changed();

#ifdef AMIBERRY
	const AmigaMonitor* mon = &AMonitors[monid];
	if (!mon->screen_is_picasso)
		force_auto_crop = true;
#endif
}

bool toggle_3d_debug(void)
{
	if (isvsync_chipset() < 0) {
		beamracer_debug = !beamracer_debug;
		//if (D3D_debug) {
		//	D3D_debug(0, beamracer_debug);
		//}
		reset_drawing();
		return true;
	}
	return false;
}

int rtg_index = -1;

// -2 = default
// -1 = prev
// 0 = chipset
// 1..4 = rtg
// 5 = next
bool toggle_rtg(const int monid, const int mode)
{
	struct amigadisplay* ad = &adisplays[monid];

	int old_index = rtg_index;

	if (monid > 0) {
		return true;
	}

	if (mode < -1 && rtg_index >= 0)
		return true;

	for (;;) {
		if (mode == -1) {
			rtg_index--;
		} else if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = mode - 1;
		} else {
			rtg_index++;
		}
		if (rtg_index >= MAX_RTG_BOARDS) {
			rtg_index = -1;
		} else if (rtg_index < -1) {
			rtg_index = MAX_RTG_BOARDS - 1;
		}
		if (rtg_index < 0) {
			if (ad->picasso_on) {
				gfxboard_rtg_disable(monid, old_index);
				ad->picasso_requested_on = false;
				statusline_add_message(STATUSTYPE_DISPLAY, _T("Chipset display"));
				set_config_changed();
				return false;
			}
			return false;
		}
		struct rtgboardconfig* r = &currprefs.rtgboards[rtg_index];
		if (r->rtgmem_size > 0 && r->monitor_id == monid) {
			if (r->rtgmem_type >= GFXBOARD_HARDWARE) {
				int idx = gfxboard_toggle(r->monitor_id, rtg_index, mode >= -1);
				if (idx >= 0) {
					rtg_index = idx;
					return true;
				}
				if (idx < -1) {
					rtg_index = -1;
					return false;
				}
			} else {
				gfxboard_toggle(r->monitor_id, -1, -1);
				if (mode < -1)
					return true;
				devices_unsafeperiod();
				gfxboard_rtg_disable(monid, old_index);
				// can always switch from RTG to custom
				if (ad->picasso_requested_on && ad->picasso_on) {
					ad->picasso_requested_on = false;
					rtg_index = -1;
					set_config_changed();
					return true;
				}
				if (ad->picasso_on)
					return false;
				// can only switch from custom to RTG if there is some mode active
				if (picasso_is_active(r->monitor_id)) {
					picasso_enablescreen(r->monitor_id, 1);
					ad->picasso_requested_on = true;
					statusline_add_message(STATUSTYPE_DISPLAY, _T("RTG %d: %s"), rtg_index + 1, _T("UAEGFX"));
					set_config_changed();
					return true;
				}
			}
		}
		if (mode >= 0 && mode <= MAX_RTG_BOARDS) {
			rtg_index = old_index;
			return false;
		}
	}
	//return false;
}

void close_rtg(const int monid, const bool reset)
{
	struct AmigaMonitor* mon = &AMonitors[monid];
	close_windows(mon, false);
	if (reset) {
		struct amigadisplay* ad = &adisplays[monid];
		mon->screen_is_picasso = false;
		ad->picasso_on = false;
		ad->picasso_requested_on = false;
		ad->picasso_requested_forced_on = false;
	}
}

void toggle_fullscreen(const int monid, const int mode)
{
	const amigadisplay* ad = &adisplays[monid];
	auto* p = ad->picasso_on ? &changed_prefs.gfx_apmode[APMODE_RTG].gfx_fullscreen : &changed_prefs.gfx_apmode[APMODE_NATIVE].gfx_fullscreen;
	int* wfw = &wasfs[ad->picasso_on ? 1 : 0];
	auto v = *p;
	//static int prevmode = -1;

	if (mode < 0) {
		// fullwindow->window->fullwindow.
		// fullscreen->window->fullscreen.
		// window->fullscreen->window.
		if (v == GFX_FULLWINDOW) {
			//prevmode = v;
			*wfw = -1;
			v = GFX_WINDOW;
		} else if (v == GFX_WINDOW) {
			if (*wfw >= 0) {
				v = GFX_FULLSCREEN;
			} else {
				v = GFX_FULLWINDOW;
			}
		} else if (v == GFX_FULLSCREEN) {
			//prevmode = v;
			*wfw = 1;
			v = GFX_WINDOW;
		}
	} else if (mode == 0) {
		//prevmode = v;
		// fullscreen <> window
		if (v == GFX_FULLSCREEN)
			v = GFX_WINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 1) {
		//prevmode = v;
		// fullscreen <> fullwindow
		if (v == GFX_FULLSCREEN)
			v = GFX_FULLWINDOW;
		else
			v = GFX_FULLSCREEN;
	} else if (mode == 2) {
		//prevmode = v;
		// window <> fullwindow
		if (v == GFX_FULLWINDOW)
			v = GFX_WINDOW;
		else
			v = GFX_FULLWINDOW;
	} else if (mode == 10) {
		v = GFX_WINDOW;
	}
	*p = v;
	devices_unsafeperiod();
	updatewinfsmode(monid, &changed_prefs);
}


#ifdef AMIBERRY
void auto_crop_image()
{
	const AmigaMonitor* mon = &AMonitors[0];
	static bool last_autocrop;

	if (currprefs.gfx_auto_crop)
	{
		static int last_cw = 0, last_ch = 0, last_cx = 0, last_cy = 0;
		static bool last_is_ntsc = false;
		int cw, ch, cx, cy, crealh = 0;
		int hres = currprefs.gfx_resolution;
		int vres = currprefs.gfx_vresolution;
		get_custom_limits(&cw, &ch, &cx, &cy, &crealh, &hres, &vres);

		// Detect NTSC from actual display timing — more reliable than
		// currprefs.ntscmode which may not reflect the chipset state
		// (e.g. WHDLoad games that switch PAL/NTSC at runtime).
		bool is_ntsc = (vblank_hz > 55.0f);

		if (!force_auto_crop && last_autocrop == currprefs.gfx_auto_crop
			&& last_cw == cw && last_ch == ch && last_cx == cx && last_cy == cy
			&& last_is_ntsc == is_ntsc)
		{
			return;
		}

		last_cw = cw;
		last_ch = ch;
		last_cx = cx;
		last_cy = cy;
		last_is_ntsc = is_ntsc;
		force_auto_crop = false;

		int width = cw;
		int height = ch;
		if (vres == VRES_NONDOUBLE)
		{
			if (hres == RES_HIRES || hres == RES_SUPERHIRES)
				height *= 2;
		}
		else
		{
			if (hres == RES_LORES)
				width *= 2;
		}

		if (is_ntsc)
			height = height * 6 / 5;

		if (currprefs.gfx_correct_aspect == 0)
		{
			width = sdl_mode.w;
			height = sdl_mode.h;
		}
		// SDL software path needs logical size update
		if (mon->amiga_renderer)
			SDL_SetRenderLogicalPresentation(mon->amiga_renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);

		IRenderer* renderer = get_renderer(0);
		auto& rq = renderer->render_quad;
		auto& cr = renderer->crop_rect;
		renderer->crop_aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 0.0f;
		renderer->crop_display_w = width;
		renderer->crop_display_h = height;
		write_log(_T("auto_crop: cw=%d ch=%d cx=%d cy=%d hres=%d vres=%d ntsc=%d (vblank=%.1fHz) => display %dx%d aspect=%.4f\n"),
			cw, ch, cx, cy, hres, vres, is_ntsc, vblank_hz, width, height, renderer->crop_aspect);
		rq = { dx, dy, width, height };
		cr = { cx, cy, cw, ch };
		SDL_Surface* surface = get_amiga_surface(0);
		if (surface) {
			if (cr.x < 0) cr.x = 0;
			if (cr.y < 0) cr.y = 0;
			if (cr.x >= surface->w) cr.x = 0;
			if (cr.y >= surface->h) cr.y = 0;
			if (cr.w <= 0 || cr.x + cr.w > surface->w)
				cr.w = surface->w - cr.x;
			if (cr.h <= 0 || cr.y + cr.h > surface->h)
				cr.h = surface->h - cr.y;
		}

		if (vkbd_allowed(0))
		{
			vkbd_update_position_from_texture();
		}
	}

	last_autocrop = currprefs.gfx_auto_crop;
}
#endif

unsigned long target_lastsynctime()
{
	return last_synctime;
}

static int save_png(const SDL_Surface* surface, const std::string& path)
{
	return gfx_platform_save_png(surface, path);
}

bool create_screenshot()
{
    if (current_screenshot != nullptr)
    {
        SDL_DestroySurface(current_screenshot);
        current_screenshot = nullptr;
    }

    SDL_Surface* surface = get_amiga_surface(0);
    return gfx_platform_create_screenshot(surface, &current_screenshot);
}

int save_thumb(const std::string& path)
{
	auto ret = 0;
	if (current_screenshot != nullptr)
	{
		ret = save_png(current_screenshot, path);
		SDL_DestroySurface(current_screenshot);
		current_screenshot = nullptr;
	}
	return ret;
}

void screenshot(int monid, int mode, int doprepare)
{
	std::string filename;

	if (!create_screenshot())
	{
		write_log(_T("Failed to create screenshot\n"));
		return;
	}

	if (strlen(currprefs.floppyslots[0].df) > 0)
		filename = extract_filename(currprefs.floppyslots[0].df);
	else if (currprefs.cdslots[0].inuse && strlen(currprefs.cdslots[0].name) > 0)
		filename = extract_filename(currprefs.cdslots[0].name);
	else
		filename = "default.uae";

	std::string base_path = get_screenshot_path();
	base_path += remove_file_extension(filename);

	screenshot_filename = base_path + ".png";

	// Avoid overwriting existing screenshots by appending a number
	for (int counter = 1; counter <= 9999 && access(screenshot_filename.c_str(), F_OK) == 0; counter++) {
		screenshot_filename = base_path + "_" + std::to_string(counter) + ".png";
	}

	if (save_thumb(screenshot_filename))
		statusline_add_message(STATUSTYPE_OTHER, _T("Screenshot: %s"), extract_filename(screenshot_filename).c_str());
	else
		statusline_add_message(STATUSTYPE_OTHER, _T("Screenshot failed"));
}
