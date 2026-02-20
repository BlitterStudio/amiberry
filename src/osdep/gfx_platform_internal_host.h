#pragma once

#include <png.h>
#include <SDL_image.h>

#include <string>

static inline bool gfx_platform_skip_alloctexture(int monid, int w, int h)
{
	(void)monid;
	(void)w;
	(void)h;
	return false;
}

static inline bool gfx_platform_render_leds()
{
	return true;
}

static inline bool gfx_platform_skip_renderframe(int monid, int mode, int immediate)
{
	(void)monid;
	(void)mode;
	(void)immediate;
	return false;
}

static inline bool gfx_platform_present_frame(const SDL_Surface* surface)
{
	(void)surface;
	return false;
}

static inline bool gfx_platform_requires_window()
{
	return true;
}

static inline bool gfx_platform_skip_window_activation()
{
	return false;
}

static inline bool gfx_platform_enumeratedisplays()
{
	return false;
}

static inline bool gfx_platform_skip_sortdisplays()
{
	return false;
}

static inline void gfx_platform_set_window_icon(SDL_Window* window)
{
	auto* const icon_surface = IMG_Load(prefix_with_data_path("amiberry.png").c_str());
	if (icon_surface != nullptr)
	{
		SDL_SetWindowIcon(window, icon_surface);
		SDL_FreeSurface(icon_surface);
	}
}

static inline bool gfx_platform_override_pixel_format(Uint32* format)
{
	(void)format;
	return false;
}

static inline bool gfx_platform_do_init(AmigaMonitor* mon)
{
	(void)mon;
	return false;
}

static inline int gfx_platform_save_png(const SDL_Surface* surface, const std::string& path)
{
	const auto w = surface->w;
	const auto h = surface->h;
	auto* const pix = static_cast<unsigned char*>(surface->pixels);
	unsigned char writeBuffer[1920 * 3]{};

	auto* const f = fopen(path.c_str(), "wbe");
	if (!f)
	{
		write_log(_T("Failed to open file for writing: %s\n"), path.c_str());
		return 0;
	}

	auto* png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr)
	{
		write_log(_T("Failed to create PNG write structure\n"));
		fclose(f);
		return 0;
	}

	auto* info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, nullptr);
		fclose(f);
		return 0;
	}

	png_init_io(png_ptr, f);
	png_set_IHDR(png_ptr,
		info_ptr,
		w,
		h,
		8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);

	auto* b = writeBuffer;
	const auto sizeX = w;
	const auto sizeY = h;

	auto* p = reinterpret_cast<unsigned int*>(pix);
	for (auto y = 0; y < sizeY; y++) {
		for (auto x = 0; x < sizeX; x++) {
			auto v = p[x];
			*b++ = ((v & SYSTEM_RED_MASK) >> SYSTEM_RED_SHIFT);
			*b++ = ((v & SYSTEM_GREEN_MASK) >> SYSTEM_GREEN_SHIFT);
			*b++ = ((v & SYSTEM_BLUE_MASK) >> SYSTEM_BLUE_SHIFT);
		}
		p += surface->pitch / 4;
		png_write_row(png_ptr, writeBuffer);
		b = writeBuffer;
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);
	return 1;
}

static inline bool gfx_platform_create_screenshot(SDL_Surface* amiga_surface, SDL_Surface** out_surface)
{
	if (!out_surface)
		return false;

	if (amiga_surface != nullptr) {
		*out_surface = SDL_CreateRGBSurfaceFrom(amiga_surface->pixels,
			AMIGA_WIDTH_MAX << currprefs.gfx_resolution,
			AMIGA_HEIGHT_MAX << currprefs.gfx_vresolution,
			amiga_surface->format->BitsPerPixel,
			amiga_surface->pitch,
			amiga_surface->format->Rmask,
			amiga_surface->format->Gmask,
			amiga_surface->format->Bmask,
			amiga_surface->format->Amask);
	}
	return *out_surface != nullptr;
}
