#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "savestate.h"
#include "picasso96.h"
#include "amiberry_gfx.h"

#include <png.h>
#include <SDL.h>

/* SDL variable for output of emulation */
SDL_Surface* screen = nullptr;

//static unsigned int current_vsync_frame = 0;
unsigned long time_per_frame = 20000; // Default for PAL (50 Hz): 20000 microsecs
static unsigned long last_synctime;
static unsigned long next_synctime = 0;

/* Possible screen modes (x and y resolutions) */
#define MAX_SCREEN_MODES 14
static int x_size_table[MAX_SCREEN_MODES] = {640, 640, 720, 800, 800, 960, 1024, 1280, 1280, 1280, 1360, 1366, 1680, 1920};
static int y_size_table[MAX_SCREEN_MODES] = {400, 480, 400, 480, 600, 540, 768, 720, 800, 1024, 768, 768, 1050, 1080};

struct PicassoResolution* DisplayModes;
struct MultiDisplay Displays[MAX_DISPLAYS];

int screen_is_picasso = 0;

static SDL_Surface* current_screenshot = nullptr;
static char screenshot_filename_default[MAX_DPATH] =
{
	'/', 't', 'm', 'p', '/', 'n', 'u', 'l', 'l', '.', 'p', 'n', 'g', '\0'
};
char* screenshot_filename = static_cast<char *>(&screenshot_filename_default[0]);
FILE* screenshot_file = nullptr;
static void CreateScreenshot();
static int save_thumb(char* path);
int delay_savestate_frame = 0;

int graphics_setup(void)
{
#ifdef PICASSO96
	picasso_InitResolutions();
	InitPicasso96();
#endif
	return 1;
}

void InitAmigaVidMode(struct uae_prefs* p)
{
	/* Initialize structure for Amiga video modes */
	gfxvidinfo.drawbuffer.pixbytes = screen->format->BytesPerPixel;
	gfxvidinfo.drawbuffer.bufmem = static_cast<uae_u8 *>(screen->pixels);
	gfxvidinfo.drawbuffer.outwidth = p->gfx_size.width;
	gfxvidinfo.drawbuffer.outheight = p->gfx_size.height;
	gfxvidinfo.drawbuffer.rowbytes = screen->pitch;
}

void graphics_subshutdown()
{
	if (screen != nullptr)
	{
		SDL_FreeSurface(screen);
		screen = nullptr;
	}
	if (texture != nullptr)
	{
		SDL_DestroyTexture(texture);
	}
}

void Create_SDL_Surface(int width, int height, int depth)
{
	screen = SDL_CreateRGBSurface(0, width, height, depth, 0, 0, 0, 0);
	check_error_sdl(screen == nullptr, "Unable to create a surface");
}

void Create_SDL_Texture(int width, int height, int depth)
{
	if (depth == 16) {
		// Initialize SDL Texture for the renderer
		texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_RGB565, // 16-bit
			SDL_TEXTUREACCESS_STREAMING,
			width,
			height);
	}
	else if (depth == 32)
	{
		texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_BGRA32, // 32-bit
			SDL_TEXTUREACCESS_STREAMING,
			width,
			height);
	}
	check_error_sdl(texture == nullptr, "Unable to create texture");
}

// Check if the requested Amiga resolution can be displayed with the current Screen mode as a direct multiple
// Based on this we make the decision to use Linear (smooth) or Nearest Neighbor (pixelated) scaling
bool isModeAspectRatioExact(SDL_DisplayMode* mode, int width, int height)
{
	if (mode->w % width == 0 && mode->h % height == 0)
		return true;
	return false;
}

void updatedisplayarea()
{
	// Update the texture from the surface
	SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	// Copy the texture on the renderer
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	// Update the window surface (show the renderer)
	SDL_RenderPresent(renderer);
}

static void open_screen(struct uae_prefs* p)
{
	int width;
	int height;
	int depth = 16;

#ifdef PICASSO96
	if (screen_is_picasso)
	{
		width = picasso_vidinfo.width;
		height = picasso_vidinfo.height;
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"); // we always use linear for Picasso96 modes
	}
	else
#endif
	{
		p->gfx_resolution = p->gfx_size.width ? (p->gfx_size.width > 600 ? 1 : 0) : 1;
		width = p->gfx_size.width ? p->gfx_size.width : 640;
		height = p->gfx_size.height ? p->gfx_size.height : 256;
		
		if (p->scaling_method == -1)
		{
			if (isModeAspectRatioExact(&sdlMode, width, height))
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
			else
				SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		}
		else if (p->scaling_method == 0)
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
		else if (p->scaling_method == 1)
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	}

	graphics_subshutdown();
	
	Create_SDL_Surface(width, height, depth);

	if (screen_is_picasso)
		SDL_RenderSetLogicalSize(renderer, width, height);
	else
		SDL_RenderSetLogicalSize(renderer, width, height*2);

	Create_SDL_Texture(width, height, depth);

	updatedisplayarea();

	if (screen != nullptr)
	{
		InitAmigaVidMode(p);
		init_row_map();
	}
}

void update_display(struct uae_prefs* p)
{
	SDL_ShowCursor(SDL_DISABLE);
	open_screen(p);
	framecnt = 1; // Don't draw frame before reset done
}

int check_prefs_changed_gfx()
{
	int changed = 0;

	if (currprefs.gfx_size.height != changed_prefs.gfx_size.height ||
		currprefs.gfx_size.width != changed_prefs.gfx_size.width ||
		currprefs.gfx_resolution != changed_prefs.gfx_resolution)
	{
		cfgfile_configuration_change(1);
		currprefs.gfx_size.height = changed_prefs.gfx_size.height;
		currprefs.gfx_size.width = changed_prefs.gfx_size.width;
		currprefs.gfx_resolution = changed_prefs.gfx_resolution;
		update_display(&currprefs);
		changed = 1;
	}
	if (currprefs.leds_on_screen != changed_prefs.leds_on_screen)
	{
		currprefs.leds_on_screen = changed_prefs.leds_on_screen;
		changed = 1;
	}
	if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate)
	{
		currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
		init_hz_normal();
		changed = 1;
	}

	currprefs.filesys_limit = changed_prefs.filesys_limit;
	currprefs.harddrive_read_only = changed_prefs.harddrive_read_only;

	if (changed)
		init_custom();

	return changed;
}


int lockscr()
{
	if(SDL_LockSurface(screen)== -1)
		return 0;
	//init_row_map();
	return 1;
}


void unlockscr()
{
	SDL_UnlockSurface(screen);
}


void wait_for_vsync()
{
}


bool render_screen(bool immediate)
{
	if (savestate_state == STATE_DOSAVE)
	{
		if (delay_savestate_frame > 0)
			--delay_savestate_frame;
		else
		{
			CreateScreenshot();
			save_thumb(screenshot_filename);
			savestate_state = 0;
		}
	}

	return true;
}

void show_screen(int mode)
{
	//unsigned long start = read_processor_time();

	last_synctime = read_processor_time();

	updatedisplayarea();

	//idletime += last_synctime - start;

	if (!screen_is_picasso)
		gfxvidinfo.drawbuffer.bufmem = (uae_u8 *)screen->pixels;

	if (last_synctime - next_synctime > time_per_frame - 5000)
		next_synctime = last_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
	else
		next_synctime = next_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
}

unsigned long target_lastsynctime(void)
{
	return last_synctime;
}

bool show_screen_maybe(bool show)
{
	if (show)
		show_screen(0);
	return false;
}

static void graphics_subinit()
{
	if (screen == nullptr)
	{
		open_screen(&currprefs);
		if (screen == nullptr)
			fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
	}
	else
	{
		SDL_ShowCursor(SDL_DISABLE);
		InitAmigaVidMode(&currprefs);
	}
}

STATIC_INLINE int bitsInMask(unsigned long mask)
{
	/* count bits in mask */
	int n = 0;
	while (mask)
	{
		n += mask & 1;
		mask >>= 1;
	}
	return n;
}

STATIC_INLINE int maskShift(unsigned long mask)
{
	/* determine how far mask is shifted */
	int n = 0;
	while (!(mask & 1))
	{
		n++;
		mask >>= 1;
	}
	return n;
}

static int init_colors()
{
	/* Truecolor: */
	int red_bits = bitsInMask(screen->format->Rmask);
	int green_bits = bitsInMask(screen->format->Gmask);
	int blue_bits = bitsInMask(screen->format->Bmask);
	int red_shift = maskShift(screen->format->Rmask);
	int green_shift = maskShift(screen->format->Gmask);
	int blue_shift = maskShift(screen->format->Bmask);
	alloc_colors64k(red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, 0);
	notice_new_xcolors();
	for (int i = 0; i < 4096; i++)
		xcolors[i] = xcolors[i] * 0x00010001;

	return 1;
}

int target_get_display(const TCHAR *name)
{
	return 0;
}

const TCHAR *target_get_display_name(int num, bool friendlyname)
{
	if (num <= 0)
		return nullptr;
	if (friendlyname)
		return "Raspberry Pi display";
	return "0";
}

/*
 * Find the colour depth of the display
 */
static int get_display_depth()
{
	int depth = screen->format->BytesPerPixel == 4 ? 32 : 16;
	return depth;
}

int GetSurfacePixelFormat()
{
	int depth = get_display_depth();
	int unit = depth + 1 & 0xF8;

	return (unit == 8 ? RGBFB_CHUNKY
		        : depth == 15 && unit == 16 ? RGBFB_R5G5B5
		        : depth == 16 && unit == 16 ? RGBFB_R5G6B5
		        : unit == 24 ? RGBFB_B8G8R8
		        : unit == 32 ? RGBFB_R8G8B8A8
		        : RGBFB_NONE);
}

int graphics_init(bool mousecapture)
{
	graphics_subinit();

	if (!init_colors())
		return 0;

	return 1;
}

void graphics_leave()
{
	graphics_subshutdown();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(sdlWindow);

	SDL_VideoQuit();
}

#define  systemRedShift      (screen->format->Rshift)
#define  systemGreenShift    (screen->format->Gshift)
#define  systemBlueShift     (screen->format->Bshift)
#define  systemRedMask       (screen->format->Rmask)
#define  systemGreenMask     (screen->format->Gmask)
#define  systemBlueMask      (screen->format->Bmask)

static int save_png(SDL_Surface* surface, char* path)
{
	int w = surface->w;
	int h = surface->h;
	unsigned char* pix = static_cast<unsigned char *>(surface->pixels);
	unsigned char writeBuffer[1024 * 3];
	FILE* f = fopen(path, "wb");
	if (!f) return 0;
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
	                                                                   nullptr,
	                                                                   nullptr,
	                                                                   nullptr);
	if (!png_ptr)
	{
		fclose(f);
		return 0;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);

	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,NULL);
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

	unsigned char* b = writeBuffer;

	int sizeX = w;
	int sizeY = h;
	int y;
	int x;

	unsigned short* p = reinterpret_cast<unsigned short *>(pix);
	for (y = 0; y < sizeY; y++)
	{
		for (x = 0; x < sizeX; x++)
		{
			unsigned short v = p[x];

			*b++ = ((v & systemRedMask) >> systemRedShift) << 3; // R
			*b++ = ((v & systemGreenMask) >> systemGreenShift) << 2; // G
			*b++ = ((v & systemBlueMask) >> systemBlueShift) << 3; // B
		}
		p += surface->pitch / 2;
		png_write_row(png_ptr, writeBuffer);
		b = writeBuffer;
	}

	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);
	return 1;
}

static void CreateScreenshot()
{
	if (current_screenshot != nullptr)
	{
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}

	int w = screen->w;
	int h = screen->h;
	current_screenshot = SDL_CreateRGBSurfaceFrom(screen->pixels,
	                                              w,
	                                              h,
	                                              screen->format->BitsPerPixel,
	                                              screen->pitch,
	                                              screen->format->Rmask,
	                                              screen->format->Gmask,
	                                              screen->format->Bmask,
	                                              screen->format->Amask);
}

static int save_thumb(char* path)
{
	int ret = 0;
	if (current_screenshot != nullptr)
	{
		ret = save_png(current_screenshot, path);
		SDL_FreeSurface(current_screenshot);
		current_screenshot = nullptr;
	}
	return ret;
}

bool vsync_switchmode(int hz)
{
	//	int changed_height = changed_prefs.gfx_size.height;
	//	
	//	if (hz >= 55)
	//		hz = 60;
	//	else
	//		hz = 50;
	//
	//  if(hz == 50 && currVSyncRate == 60)
	//  {
	//    // Switch from NTSC -> PAL
	//    switch(changed_height) {
	//      case 200: changed_height = 240; break;
	//      case 216: changed_height = 262; break;
	//      case 240: changed_height = 270; break;
	//      case 256: changed_height = 270; break;
	//      case 262: changed_height = 270; break;
	//      case 270: changed_height = 270; break;
	//    }
	//  }
	//  else if(hz == 60 && currVSyncRate == 50)
	//  {
	//    // Switch from PAL -> NTSC
	//    switch(changed_height) {
	//      case 200: changed_height = 200; break;
	//      case 216: changed_height = 200; break;
	//      case 240: changed_height = 200; break;
	//      case 256: changed_height = 216; break;
	//      case 262: changed_height = 216; break;
	//      case 270: changed_height = 240; break;
	//    }
	//  }
	//
	//  if(changed_height == currprefs.gfx_size.height && hz == currprefs.chipset_refreshrate)
	//    return true;
	//  
	//  changed_prefs.gfx_size.height = changed_height;

	return true;
}

bool target_graphics_buffer_update()
{
	bool rate_changed = false;

	if (currprefs.gfx_size.height != changed_prefs.gfx_size.height)
	{
		update_display(&changed_prefs);
		rate_changed = true;
	}

	if (rate_changed)
	{
		fpscounter_reset();
		time_per_frame = 1000 * 1000 / currprefs.chipset_refreshrate;
	}

	return true;
}

#ifdef PICASSO96

int picasso_palette(struct MyCLUTEntry *CLUT)
{
	int changed = 0;
	for (int i = 0; i < 256; i++) {
		int r = CLUT[i].Red;
		int g = CLUT[i].Green;
		int b = CLUT[i].Blue;
		uae_u32 v = CONVERT_RGB(r << 16 | g << 8 | b);
		if (v != picasso_vidinfo.clut[i]) {
			picasso_vidinfo.clut[i] = v;
			changed = 1;
		}
	}
	return changed;
}

static int resolution_compare(const void* a, const void* b)
{
	struct PicassoResolution* ma = (struct PicassoResolution *)a;
	struct PicassoResolution* mb = (struct PicassoResolution *)b;
	if (ma->res.width < mb->res.width)
		return -1;
	if (ma->res.width > mb->res.width)
		return 1;
	if (ma->res.height < mb->res.height)
		return -1;
	if (ma->res.height > mb->res.height)
		return 1;
	return ma->depth - mb->depth;
}

static void sortmodes()
{
	int i = 0, idx = -1;
	int pw = -1, ph = -1;
	while (DisplayModes[i].depth >= 0)
		i++;
	qsort(DisplayModes, i, sizeof (struct PicassoResolution), resolution_compare);
	for (i = 0; DisplayModes[i].depth >= 0; i++)
	{
		if (DisplayModes[i].res.height != ph || DisplayModes[i].res.width != pw)
		{
			ph = DisplayModes[i].res.height;
			pw = DisplayModes[i].res.width;
			idx++;
		}
		DisplayModes[i].residx = idx;
	}
}

static void modesList()
{
	int i, j;

	i = 0;
	while (DisplayModes[i].depth >= 0)
	{
		write_log ("%d: %s (", i, DisplayModes[i].name);
		j = 0;
		while (DisplayModes[i].refresh[j] > 0)
		{
			if (j > 0)
			write_log (",");
			write_log ("%d", DisplayModes[i].refresh[j]);
			j++;
		}
		write_log (")\n");
		i++;
	}
}

void picasso_InitResolutions()
{
	int count = 0;
	char tmp[200];
	int bits[] = {8, 16, 32};

	Displays[0].primary = 1;
	Displays[0].disabled = 0;
	Displays[0].rect.left = 0;
	Displays[0].rect.top = 0;
	Displays[0].rect.right = 800;
	Displays[0].rect.bottom = 480;
	sprintf(tmp, "%s (%d*%d)", "Display", Displays[0].rect.right, Displays[0].rect.bottom);
	Displays[0].name = my_strdup(tmp);
	Displays[0].name2 = my_strdup("Display");

	struct MultiDisplay * md1 = Displays;
	DisplayModes = md1->DisplayModes = xmalloc (struct PicassoResolution, MAX_PICASSO_MODES);
	for (int i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++)
	{
		for (int bit_idx = 0; bit_idx < 3; ++bit_idx)
		{
			int bitdepth = bits[bit_idx];
			int bit_unit = (bitdepth + 1) & 0xF8;
			int rgbFormat = (bitdepth == 8 ? RGBFB_CLUT : (bitdepth == 16 ? RGBFB_R5G6B5 : RGBFB_R8G8B8A8));
			int pixelFormat = 1 << rgbFormat;
			pixelFormat |= RGBFF_CHUNKY;

			DisplayModes[count].res.width = x_size_table[i];
			DisplayModes[count].res.height = y_size_table[i];
			DisplayModes[count].depth = bit_unit >> 3;
			DisplayModes[count].refresh[0] = 50;
			DisplayModes[count].refresh[1] = 60;
			DisplayModes[count].refresh[2] = 0;
			DisplayModes[count].colormodes = pixelFormat;
			sprintf(DisplayModes[count].name, "%dx%d, %d-bit",
			        DisplayModes[count].res.width, DisplayModes[count].res.height, DisplayModes[count].depth * 8);

			count++;
		}
	}
	DisplayModes[count].depth = -1;
	sortmodes();
	modesList();
	DisplayModes = Displays[0].DisplayModes;
}
#endif

#ifdef PICASSO96
void gfx_set_picasso_state(int on)
{
	if (on == screen_is_picasso)
		return;

	screen_is_picasso = on;
	open_screen(&currprefs);
	picasso_vidinfo.rowbytes = screen->pitch;
}

void gfx_set_picasso_modeinfo(uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
	depth >>= 3;
	if ((unsigned(picasso_vidinfo.width) == w) &&
		(unsigned(picasso_vidinfo.height) == h) &&
		(unsigned(picasso_vidinfo.depth) == depth) &&
		(picasso_vidinfo.selected_rgbformat == rgbfmt))
		return;

	picasso_vidinfo.selected_rgbformat = rgbfmt;
	picasso_vidinfo.width = w;
	picasso_vidinfo.height = h;
	picasso_vidinfo.depth = screen->format->BytesPerPixel; // Native depth
	picasso_vidinfo.extra_mem = 1;

	picasso_vidinfo.pixbytes = screen->format->BytesPerPixel; // Native bytes
	if (screen_is_picasso)
	{
		open_screen(&currprefs);
		picasso_vidinfo.rowbytes = screen->pitch;
		picasso_vidinfo.rgbformat = screen->format->BytesPerPixel == 4 ? RGBFB_R8G8B8A8 : RGBFB_R5G6B5;
	}
}

uae_u8* gfx_lock_picasso()
{
	if (screen == nullptr || screen_is_picasso == 0)
		return nullptr;
	SDL_LockSurface(screen);
	picasso_vidinfo.rowbytes = screen->pitch;
	return static_cast<uae_u8 *>(screen->pixels);
}

void gfx_unlock_picasso(bool dorender)
{
	SDL_UnlockSurface(screen);
	if (dorender)
	{
		render_screen(true);
		show_screen(0);
	}
}

#endif // PICASSO96
