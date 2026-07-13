#pragma once

#include <cstring>
#include <string>
#include <vector>

#include "libretro_shared.h"
#include "custom.h"
#include "drawing.h"
#include "gfx_window.h"
#include "gfx_colors.h"
#include "display_modes.h"
#include "gui.h"
#include "statusline.h"
#ifdef PICASSO96
#include "picasso96.h"
#endif

// updatepicasso96 lives in amiberry_gfx.cpp with no header declaration
extern void updatepicasso96(struct AmigaMonitor* mon);

static constexpr int LIBRETRO_DISPLAY_INIT_WIDTH = 1920;
static constexpr int LIBRETRO_DISPLAY_INIT_HEIGHT = 1080;
static constexpr int LIBRETRO_DRAWBUFFER_WIDTH = 1920;
static constexpr int LIBRETRO_DRAWBUFFER_HEIGHT = 1280;

static inline bool gfx_platform_skip_alloctexture(int monid, int w, int h)
{
	(void)monid;
	(void)w;
	(void)h;
	return true;
}

static inline bool gfx_platform_render_leds()
{
	return false;
}

static inline bool gfx_platform_skip_renderframe(int monid, int mode, int immediate)
{
	(void)monid;
	(void)mode;
	(void)immediate;
	return true;
}

// Composites the statusline into a private buffer so the emulator surface is
// never mutated: crop scanning must not see statusline pixels as content.
static inline const uae_u8* libretro_render_statusline(const uae_u8* src, int w, int h, int pitch)
{
	const int monid = 0;
	const struct amigadisplay* ad = &adisplays[monid];

	const bool show =
		((currprefs.leds_on_screen & STATUSLINE_CHIPSET) && !ad->picasso_on) ||
		((currprefs.leds_on_screen & STATUSLINE_RTG) && ad->picasso_on);
	if (!show || !src || w <= 0 || h <= 0)
		return src;

	static uae_u32 rc[256], gc[256], bc[256], a[256];
	static bool color_tables_initialized = false;
	if (!color_tables_initialized) {
		for (int i = 0; i < 256; i++) {
			rc[i] = static_cast<uae_u32>(i) << 16;
			gc[i] = static_cast<uae_u32>(i) << 8;
			bc[i] = static_cast<uae_u32>(i) << 0;
			a[i] = static_cast<uae_u32>(i) << 24;
		}
		color_tables_initialized = true;
	}

	statusline_set_multiplier(monid, w, h);
	const int m = statusline_get_multiplier(monid) / 100;
	const int led_height = TD_TOTAL_HEIGHT * m;

	const TCHAR* msg = statusline_fetch();
	const int msg_m = m < 2 ? 2 : m;
	const int msg_height = msg ? (LDP_CHAR_HEIGHT + 4) * msg_m : 0;

	static std::vector<uae_u8> composited;
	composited.resize(static_cast<size_t>(h) * pitch);
	uae_u8* pixels = composited.data();
	const int row_bytes = w * 4;
	for (int y = 0; y < h; y++)
		memcpy(pixels + static_cast<size_t>(y) * pitch, src + static_cast<size_t>(y) * pitch, row_bytes);

	int y0 = h - (led_height + msg_height);
	if (y0 < 0)
		y0 = 0;

	if (msg && msg_height > 0 && y0 + msg_height <= h) {
		statusline_render(monid, pixels + y0 * pitch, pitch, w, msg_height, rc, gc, bc, a);
	}

	for (int y = 0; y < led_height; y++) {
		const int dy = y0 + msg_height + y;
		if (dy >= h)
			break;
		draw_status_line_single(monid, pixels + dy * pitch, y, w, rc, gc, bc, a);
	}
	return pixels;
}

static inline bool gfx_platform_present_frame(const SDL_Surface* surface)
{
	if (surface) {
		if (video_cb) {
			const uae_u8* pixels = static_cast<const uae_u8*>(surface->pixels);
			int w = surface->w;
			int h = surface->h;
			libretro_crop crop = libretro_compute_crop();
			if (crop.active
				&& crop.x >= 0 && crop.y >= 0
				&& crop.w > 0 && crop.h > 0
				&& crop.x + crop.w <= surface->w
				&& crop.y + crop.h <= surface->h) {
				pixels += crop.y * surface->pitch
					+ crop.x * SDL_BYTESPERPIXEL(surface->format);
				w = crop.w;
				h = crop.h;
			}
			pixels = libretro_render_statusline(pixels, w, h, surface->pitch);
			video_cb(pixels, w, h, surface->pitch);
		} else {
			static bool warned = false;
			if (!warned) {
				write_log("libretro video_cb is null; skipping frame output.\n");
				warned = true;
			}
		}
	}
	libretro_yield();
	return true;
}

static inline bool gfx_platform_requires_window()
{
	return false;
}

static inline bool gfx_platform_skip_window_activation()
{
	return true;
}

static inline void libretro_init_display_modes(struct MultiDisplay* md, int width, int height)
{
	if (md->DisplayModes)
		return;

	md->DisplayModes = xcalloc(struct PicassoResolution, 2);
	if (!md->DisplayModes)
		return;

	md->DisplayModes[0].inuse = true;
	md->DisplayModes[0].res.width = width;
	md->DisplayModes[0].res.height = height;
	md->DisplayModes[0].refresh[0] = 60;
	md->DisplayModes[0].refresh[1] = 0;
	md->DisplayModes[0].refreshtype[0] = 0;
	md->DisplayModes[0].refreshtype[1] = 0;
	_sntprintf(md->DisplayModes[0].name, sizeof md->DisplayModes[0].name, _T("%dx%d"), width, height);
}

static inline void libretro_init_display(int width, int height)
{
	struct MultiDisplay* md = &Displays[0];
	if (md->monitorname)
		return;

	md->primary = 1;
	md->monitor = 0;
	md->adaptername = my_strdup(_T("Libretro"));
	md->adapterid = my_strdup(_T("Libretro"));
	md->adapterkey = my_strdup(_T("Libretro"));
	md->monitorname = my_strdup(_T("Libretro"));
	md->monitorid = my_strdup(_T("Libretro"));
	md->fullname = my_strdup(_T("Libretro"));
	md->rect.x = 0;
	md->rect.y = 0;
	md->rect.w = width;
	md->rect.h = height;
	md->workrect = md->rect;
	md->HasAdapterData = true;

	libretro_init_display_modes(md, width, height);
}

static inline bool gfx_platform_enumeratedisplays()
{
	libretro_init_display(LIBRETRO_DISPLAY_INIT_WIDTH, LIBRETRO_DISPLAY_INIT_HEIGHT);
	return true;
}

static inline bool gfx_platform_skip_sortdisplays()
{
	return true;
}

static inline void gfx_platform_set_window_icon(SDL_Window* window)
{
	(void)window;
}

static inline bool gfx_platform_override_pixel_format(SDL_PixelFormat* format)
{
	if (!format)
		return false;

	// Always use ARGB8888 for libretro (matches RETRO_PIXEL_FORMAT_XRGB8888).
	// Never return RGB565 here: the drawbuffer is 32-bit and a 16-bit surface
	// would cause a buffer overflow when the drawing code writes 4 bytes/pixel.
	*format = SDL_PIXELFORMAT_ARGB8888;
	return true;
}

static inline void libretro_allocsoftbuffer(int monid, const TCHAR* name, struct vidbuffer* buf, int flags, int width, int height)
{
	struct vidbuf_description* vidinfo = &adisplays[monid].gfxvidinfo;
	int depth = 32;

	if (buf->initialized && buf->vram_buffer)
		return;

	buf->monitor_id = monid;
	buf->pixbytes = (depth + 7) / 8;
	buf->width_allocated = (width + 7) & ~7;
	buf->height_allocated = height;
	buf->initialized = true;
	buf->hardwiredpositioning = false;

	if (buf == &vidinfo->drawbuffer) {
		buf->bufmem = NULL;
		buf->bufmemend = NULL;
		buf->realbufmem = NULL;
		buf->bufmem_allocated = NULL;
		buf->vram_buffer = true;
	} else {
		xfree(buf->realbufmem);
		const int w = buf->width_allocated;
		const int h = buf->height_allocated;
		const int size = (w * 2) * (h * 2) * buf->pixbytes;
		buf->rowbytes = w * 2 * buf->pixbytes;
		buf->realbufmem = xcalloc(uae_u8, size);
		buf->bufmem_allocated = buf->bufmem = buf->realbufmem + (h / 2) * buf->rowbytes + (w / 2) * buf->pixbytes;
		buf->bufmemend = buf->realbufmem + size - buf->rowbytes;
	}
}

static inline bool gfx_platform_do_init(AmigaMonitor* mon)
{
	if (!mon)
		return false;

	int display_width, display_height;

	struct vidbuf_description* avidinfo = &adisplays[mon->monitor_id].gfxvidinfo;
	struct amigadisplay* ad = &adisplays[mon->monitor_id];
	avidinfo->gfx_resolution_reserved = RES_MAX;
	avidinfo->gfx_vresolution_reserved = VRES_MAX;

	mon->amiga_window = nullptr;
	mon->amiga_renderer = nullptr;

	updatemodes(mon);
	update_gfxparams(mon);

#ifdef PICASSO96
	if (mon->screen_is_picasso) {
		display_width = picasso96_state[0].Width ? picasso96_state[0].Width : 640;
		display_height = picasso96_state[0].Height ? picasso96_state[0].Height : 480;
	} else {
#endif
		avidinfo->gfx_resolution_reserved = std::max(currprefs.gfx_resolution, avidinfo->gfx_resolution_reserved);
		avidinfo->gfx_vresolution_reserved = std::max(currprefs.gfx_vresolution, avidinfo->gfx_vresolution_reserved);

		if (!currprefs.gfx_autoresolution) {
			mon->currentmode.amiga_width = AMIGA_WIDTH_MAX << currprefs.gfx_resolution;
			mon->currentmode.amiga_height = AMIGA_HEIGHT_MAX << currprefs.gfx_vresolution;
		} else {
			mon->currentmode.amiga_width = AMIGA_WIDTH_MAX << avidinfo->gfx_resolution_reserved;
			mon->currentmode.amiga_height = AMIGA_HEIGHT_MAX << avidinfo->gfx_vresolution_reserved;
		}
		if (avidinfo->gfx_resolution_reserved == RES_SUPERHIRES)
			mon->currentmode.amiga_height *= 2;
		mon->currentmode.amiga_height = std::min(mon->currentmode.amiga_height, 1280);

		display_width = mon->currentmode.amiga_width;
		display_height = mon->currentmode.amiga_height;
#ifdef PICASSO96
	}
#endif

	updatepicasso96(mon);

	if (!mon->screen_is_picasso) {
		libretro_allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, mon->currentmode.flags,
			LIBRETRO_DRAWBUFFER_WIDTH, LIBRETRO_DRAWBUFFER_HEIGHT);

		libretro_allocsoftbuffer(mon->monitor_id, _T("monemu"), &avidinfo->tempbuffer, mon->currentmode.flags,
			mon->currentmode.amiga_width > 2048 ? mon->currentmode.amiga_width : 2048,
			mon->currentmode.amiga_height > 2048 ? mon->currentmode.amiga_height : 2048);
	}

	avidinfo->outbuffer = &avidinfo->drawbuffer;
	avidinfo->inbuffer = &avidinfo->tempbuffer;

	if (amiga_surface) {
		SDL_DestroySurface(amiga_surface);
		amiga_surface = nullptr;
	}
	// Set pixel_format to match the libretro frontend expectation (XRGB8888)
	// BEFORE creating the surface so init_colors and target_graphics_buffer_update
	// see a consistent format (no surface recreation = no crash).
	gfx_platform_override_pixel_format(&pixel_format);
	amiga_surface = SDL_CreateSurface(display_width, display_height, pixel_format);

	mon->screen_is_initialized = 1;
	init_colors(mon->monitor_id);
	display_param_init(mon);
	target_graphics_buffer_update(mon->monitor_id, false);
	return true;
}

static inline int gfx_platform_save_png(const SDL_Surface* surface, const std::string& path)
{
	(void)surface;
	(void)path;
	return 0;
}

static inline bool gfx_platform_create_screenshot(SDL_Surface* amiga_surface, SDL_Surface** out_surface)
{
	(void)amiga_surface;
	(void)out_surface;
	return false;
}
