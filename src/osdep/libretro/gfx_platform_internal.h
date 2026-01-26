#pragma once

#include <string>

#include "libretro_shared.h"

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

static inline bool gfx_platform_present_frame(const SDL_Surface* surface)
{
	if (surface) {
		if (video_cb) {
			video_cb(surface->pixels, surface->w, surface->h, surface->pitch);
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
	libretro_init_display(1920, 1080);
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

static inline bool gfx_platform_override_pixel_format(Uint32* format)
{
	if (!format)
		return false;

	*format = SDL_PIXELFORMAT_ARGB8888;
	return true;
}

static inline bool gfx_platform_do_init(AmigaMonitor* mon)
{
	if (!mon)
		return false;

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
		allocsoftbuffer(mon->monitor_id, _T("draw"), &avidinfo->drawbuffer, mon->currentmode.flags,
			1920, 1280);

		allocsoftbuffer(mon->monitor_id, _T("monemu"), &avidinfo->tempbuffer, mon->currentmode.flags,
			mon->currentmode.amiga_width > 2048 ? mon->currentmode.amiga_width : 2048,
			mon->currentmode.amiga_height > 2048 ? mon->currentmode.amiga_height : 2048);
	}

	avidinfo->outbuffer = &avidinfo->drawbuffer;
	avidinfo->inbuffer = &avidinfo->tempbuffer;

	if (amiga_surface) {
		SDL_FreeSurface(amiga_surface);
		amiga_surface = nullptr;
	}
	amiga_surface = SDL_CreateRGBSurfaceWithFormat(0, display_width, display_height, 32, pixel_format);

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
