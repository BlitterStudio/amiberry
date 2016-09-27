#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "include/memory.h"
#include "newcpu.h"
#include "inputdevice.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "savestate.h"
#include "picasso96.h"

#include <png.h>
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>
//#include <SDL_gfxPrimitives.h>
#ifdef ANDROIDSDL
#include <android/log.h>
#endif
#include "td-sdl/thread.h"
#include "bcm_host.h"

/* SDL surface variable for output of emulation */
SDL_Surface *screenSurface = NULL;

static SDL_Surface *current_screenshot = NULL;
/* Possible screen modes (x and y resolutions) */

#define MAX_SCREEN_MODES 10
static int x_size_table[MAX_SCREEN_MODES] = { 640, 640, 720, 800, 800, 960, 1024, 1280, 1280, 1920 };
static int y_size_table[MAX_SCREEN_MODES] = { 400, 480, 400, 480, 600, 540,  768,  720,  800, 1080 };

static int red_bits, green_bits, blue_bits;
static int red_shift, green_shift, blue_shift;

struct PicassoResolution *DisplayModes;
struct MultiDisplay Displays[MAX_DISPLAYS];

int screen_is_picasso = 0;
int result = 0;

static int curr_layer_width = 0;

static char screenshot_filename_default[255]=
{
    '/', 't', 'm', 'p', '/', 'n', 'u', 'l', 'l', '.', 'p', 'n', 'g', '\0'
};
char *screenshot_filename=(char *)&screenshot_filename_default[0];
FILE *screenshot_file=NULL;
static void CreateScreenshot(void);
static int save_thumb(char *path);
int delay_savestate_frame = 0;

int justClicked = 0;
int mouseMoving = 0;
int fcounter = 0;
int doStylusRightClick = 0;

int DispManXElementpresent = 0;
int width;
int height;

static unsigned long previous_synctime = 0;
static unsigned long next_synctime = 0;

uae_sem_t vsync_wait_sem;

DISPMANX_DISPLAY_HANDLE_T   display;
DISPMANX_MODEINFO_T         info;
DISPMANX_RESOURCE_HANDLE_T  bgResource;
DISPMANX_RESOURCE_HANDLE_T  resource;
DISPMANX_ELEMENT_HANDLE_T   element;
DISPMANX_ELEMENT_HANDLE_T	bgElement;
DISPMANX_UPDATE_HANDLE_T    update;
VC_IMAGE_TYPE_T type = VC_IMAGE_RGBA32;
VC_RECT_T       src_rect;
VC_RECT_T       dst_rect;
VC_RECT_T       blit_rect;
VC_DISPMANX_ALPHA_T alpha = {
	(DISPMANX_FLAGS_ALPHA_T)(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
	255, /*alpha 0->255*/
	0
};

unsigned char current_resource_amigafb = 0;

void vsync_callback(unsigned int a, void* b)
{
    uae_sem_post (&vsync_wait_sem);
}

int graphics_setup (void)
{
#ifdef PICASSO96
    picasso_InitResolutions();
    InitPicasso96();
#endif
    bcm_host_init();
    return 1;
}

void InitAmigaVidMode(struct uae_prefs *p)
{
    /* Initialize structure for Amiga video modes */
    gfxvidinfo.pixbytes = 2;
    gfxvidinfo.bufmem = (uae_u8 *)screenSurface->pixels;
    gfxvidinfo.outwidth = p->gfx_size.width;
    gfxvidinfo.outheight = p->gfx_size.height;
#ifdef PICASSO96
    if(screen_is_picasso)
    {
        gfxvidinfo.outwidth  = picasso_vidinfo.width;
        //gfxvidinfo.outheight = picasso_vidinfo.height;
    }
#endif
    //gfxvidinfo.rowbytes = prSDLScreen->pitch;
    gfxvidinfo.rowbytes = blit_rect.width * 2;
}

void graphics_dispmanshutdown (void)
{
    if (DispManXElementpresent == 1)
    {
        DispManXElementpresent = 0;
        update = vc_dispmanx_update_start(0);
	    assert(update != 0);
        result = vc_dispmanx_element_remove(update, element);
	    assert(result == 0);
	    result = vc_dispmanx_update_submit_sync(update);
	    assert(result == 0);
	    
	    result = vc_dispmanx_resource_delete(resource);
	    assert(result == 0);
	    result = vc_dispmanx_resource_delete(bgResource);
	    assert(result == 0);
    }
}

void graphics_subshutdown (void)
{
	if (bgResource != 0)
        graphics_dispmanshutdown();

	SDL_FreeSurface(screenSurface);
	screenSurface = NULL;
}

static void open_screen(struct uae_prefs *p)
{
	uint32_t displayNumber = 0;
    uint32_t vc_image_ptr;

#ifdef PICASSO96
    if (screen_is_picasso)
    {
        width  = picasso_vidinfo.width;
        height = picasso_vidinfo.height;
    }
    else
#endif
    {
        p->gfx_resolution = p->gfx_size.width > 600 ? 1 : 0;
        width  = p->gfx_size.width;
        height = p->gfx_size.height;
    }

	SDL_ShowCursor(SDL_DISABLE);

    // check if resolution hasn't changed in menu, otherwise free the resources so that they will be re-generated with new resolution.
	if ((bgResource != 0) &&
            ((blit_rect.width != width) || (blit_rect.height != height) || (currprefs.gfx_correct_aspect != changed_prefs.gfx_correct_aspect) ||
             (currprefs.gfx_fullscreen_ratio != changed_prefs.gfx_fullscreen_ratio)))
    {
//        printf("Emulation resolution change detected.\n");
        if(screenSurface != NULL )
        {
            SDL_FreeSurface(screenSurface);
            screenSurface = 0;
        }
        graphics_dispmanshutdown();
	    bgResource = 0;
	    resource = 0;
    }

	if (bgResource == 0)
    {
        printf("Emulation resolution: Width %i Height: %i\n", width, height);
        currprefs.gfx_correct_aspect = changed_prefs.gfx_correct_aspect;
        currprefs.gfx_fullscreen_ratio = changed_prefs.gfx_fullscreen_ratio;
        screenSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

	    display = vc_dispmanx_display_open(displayNumber);
	    assert(display != 0);
	    result = vc_dispmanx_display_get_info(display, &info);
	    assert(result == 0);

	    bgResource = vc_dispmanx_resource_create(type, width, height, &vc_image_ptr);
	    assert(bgResource != 0);
	    resource = vc_dispmanx_resource_create(type, width, height, &vc_image_ptr);
	    assert(resource != 0);
	    
//        vc_dispmanx_rect_set( &blit_rect, 0, 0, width, height);
	    vc_dispmanx_rect_set(&dst_rect, 0, 0, width, height);
//	    vc_dispmanx_resource_write_data(bgResource, type, sizeof(width), screenSurface->pixels, &blit_rect);
	    uint32_t background = 0;
	    result = vc_dispmanx_resource_write_data(bgResource, type, sizeof(background), &background, &dst_rect);
	    assert(result == 0);
//        vc_dispmanx_rect_set(&src_rect, 0, 0, width << 16, height << 16);

    }
    // 16/9 to 4/3 ratio adaptation.
    if (currprefs.gfx_correct_aspect == 0)
    {
        // Fullscreen.
        int scaled_width = info.width * currprefs.gfx_fullscreen_ratio/100;
        int scaled_height = info.height * currprefs.gfx_fullscreen_ratio/100;
        vc_dispmanx_rect_set(&dst_rect, (info.width - scaled_width)/2,
                              (info.height - scaled_height)/2,
                              scaled_width,
                              scaled_height );
    }
    else
    {
        // 4/3 shrink.
        int scaled_width = info.width * currprefs.gfx_fullscreen_ratio/100;
        int scaled_height = info.height * currprefs.gfx_fullscreen_ratio/100;
        vc_dispmanx_rect_set(&dst_rect, (info.width - scaled_width/16*12)/2,
                              (info.height - scaled_height)/2,
                              scaled_width/16*12,
                              scaled_height );
    }

    // For debug, in order to avoid full screen.
    //vc_dispmanx_rect_set( &dst_rect, (info.width /2.7),
    //                     0 ,
    //                     (info.width - (info.width * 6)/100 )/1.5,
    //                     (info.height - (info.height * 7)/100 )/1.5);

	result = vc_dispmanx_resource_write_data(resource,
		type,
		16 * sizeof(uint32_t),
		screenSurface->pixels,
		&dst_rect);
	assert(result == 0);
	
    if (DispManXElementpresent == 0)
    {
        DispManXElementpresent = 1;
        update = vc_dispmanx_update_start(0);
	    assert(update != 0);
	    
	    bgElement = vc_dispmanx_element_add(update,
		    display,
		    1, // layer
		    &dst_rect,
		    bgResource,
		    &src_rect,
		    DISPMANX_PROTECTION_NONE,
		    &alpha,
		    NULL, // clamp
		    DISPMANX_NO_ROTATE);
	    assert(bgElement != 0);
	    
	    vc_dispmanx_rect_set(&src_rect, 0, 0, width << 16, height << 16);
	    
	    element = vc_dispmanx_element_add(update,
		    display,
		    2, // layer
		    &dst_rect,
		    resource,
		    &src_rect,
		    DISPMANX_PROTECTION_NONE,
		    &alpha,
		    NULL, // clamp
		    DISPMANX_NO_ROTATE);
	    assert(element != 0);

	    result = vc_dispmanx_update_submit_sync(update);
	    assert(result == 0);
    }

    if(screenSurface != NULL)
    {
        InitAmigaVidMode(p);
        init_row_map();
    }
}

void refresh_display(SDL_Surface* surface)
{
	//Write our Surface pixels to Dispmanx Resource
	vc_dispmanx_rect_set(&dst_rect, 0, 0, width, height);
	result = vc_dispmanx_resource_write_data(resource,
		type,
		16 * sizeof(uint32_t),
		surface->pixels,
		&dst_rect);
	assert(result == 0);
			
	//Start update
	update = vc_dispmanx_update_start(0);
	assert(update != 0);
			
	// Add element
	element = vc_dispmanx_element_add(update,
		display,
		1, // layer
		&dst_rect,
		resource,
		&src_rect,
		DISPMANX_PROTECTION_NONE,
		&alpha,
		NULL, // clamp
		DISPMANX_NO_ROTATE);
	assert(element != 0);
			
	//Update Display
	result = vc_dispmanx_update_submit_sync(update);
	assert(result == 0);
}

void update_display(struct uae_prefs *p)
{
    open_screen(p);
    SDL_ShowCursor(SDL_DISABLE);
    framecnt = 1; // Don't draw frame before reset done
}

int check_prefs_changed_gfx (void)
{
    int changed = 0;

    if(currprefs.gfx_size.height != changed_prefs.gfx_size.height ||
            currprefs.gfx_size.width != changed_prefs.gfx_size.width ||
            currprefs.gfx_size_fs.width != changed_prefs.gfx_size_fs.width ||
            currprefs.gfx_resolution != changed_prefs.gfx_resolution)
    {
        cfgfile_configuration_change(1);
        currprefs.gfx_size.height = changed_prefs.gfx_size.height;
        currprefs.gfx_size.width = changed_prefs.gfx_size.width;
        currprefs.gfx_size_fs.width = changed_prefs.gfx_size_fs.width;
        currprefs.gfx_resolution = changed_prefs.gfx_resolution;
        update_display(&currprefs);
        changed = 1;
    }
    if (currprefs.leds_on_screen != changed_prefs.leds_on_screen ||
            currprefs.pandora_hide_idle_led != changed_prefs.pandora_hide_idle_led ||
            currprefs.pandora_vertical_offset != changed_prefs.pandora_vertical_offset)
    {
        currprefs.leds_on_screen = changed_prefs.leds_on_screen;
        currprefs.pandora_hide_idle_led = changed_prefs.pandora_hide_idle_led;
        currprefs.pandora_vertical_offset = changed_prefs.pandora_vertical_offset;
        changed = 1;
    }
    if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate)
    {
        currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
        init_hz_full ();
        changed = 1;
    }

    return changed;
}


int lockscr (void)
{
    //SDL_LockSurface(prSDLScreen);
    return 1;
}


void unlockscr (void)
{
    //SDL_UnlockSurface(prSDLScreen);
}

void wait_for_vsync(void)
{
    // Temporary
}

void flush_screen ()
{
    if (savestate_state == STATE_DOSAVE)
    {
        if(delay_savestate_frame > 0)
            --delay_savestate_frame;
        else
        {
            CreateScreenshot();
            save_thumb(screenshot_filename);
            savestate_state = 0;
        }
    }

    unsigned long start = read_processor_time();

    if (current_resource_amigafb == 1)
    {
        current_resource_amigafb = 0;
	    vc_dispmanx_resource_write_data(bgResource,
                                          type,
                                          sizeof(gfxvidinfo.outwidth),
                                          gfxvidinfo.bufmem,
                                          &blit_rect );
        update = vc_dispmanx_update_start( 10 );
	    vc_dispmanx_element_change_source(update, element, bgResource);

//        vc_dispmanx_update_submit(update, vsync_callback, NULL);
        vc_dispmanx_update_submit_sync(update);
    }
    else
    {
        current_resource_amigafb = 1;
	    vc_dispmanx_resource_write_data(resource,
                                          type,
                                          sizeof(gfxvidinfo.outwidth),
                                          gfxvidinfo.bufmem,
                                          &blit_rect );
        update = vc_dispmanx_update_start( 10 );
	    vc_dispmanx_element_change_source(update, element, resource);
	    vc_dispmanx_update_submit_sync(update);
//        vc_dispmanx_update_submit(update, vsync_callback,NULL);
    }

    last_synctime = read_processor_time();

    if(last_synctime - next_synctime > time_per_frame - 1000 || next_synctime < start)
        adjust_idletime(0);
    else
        adjust_idletime(next_synctime - start);

    if(last_synctime - next_synctime > time_per_frame - 5000)
        next_synctime = last_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
    else
        next_synctime = next_synctime + time_per_frame * (1 + currprefs.gfx_framerate);

    init_row_map();

}

void black_screen_now(void)
{
	SDL_FillRect(screenSurface, NULL, 0);
	refresh_display(screenSurface);
}

static void graphics_subinit (void)
{
    if (screenSurface == NULL)
    {
        fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
        return;
    }
    else
    {
        SDL_ShowCursor(SDL_DISABLE);
        InitAmigaVidMode(&currprefs);
    }
}

STATIC_INLINE int bitsInMask (unsigned long mask)
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

STATIC_INLINE int maskShift (unsigned long mask)
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

static int init_colors (void)
{
    int i;
    int red_bits, green_bits, blue_bits;
    int red_shift, green_shift, blue_shift;

    /* Truecolor: */
    red_bits = bitsInMask(screenSurface->format->Rmask);
    green_bits = bitsInMask(screenSurface->format->Gmask);
    blue_bits = bitsInMask(screenSurface->format->Bmask);
    red_shift = maskShift(screenSurface->format->Rmask);
    green_shift = maskShift(screenSurface->format->Gmask);
    blue_shift = maskShift(screenSurface->format->Bmask);
    alloc_colors64k (red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, 0);
    notice_new_xcolors();
    for (i = 0; i < 4096; i++)
        xcolors[i] = xcolors[i] * 0x00010001;

    return 1;
}

/*
 * Find the colour depth of the display
 */
static int get_display_depth (void)
{
//    const SDL_VideoInfo *vid_info;
    int depth = 32;

//    if ((vid_info = SDL_GetVideoInfo()))
//    {
//        depth = vid_info->vfmt->BitsPerPixel;
//
//        /* Don't trust the answer if it's 16 bits; the display
//         * could actually be 15 bits deep. We'll count the bits
//         * ourselves */
//        if (depth == 16)
//            depth = bitsInMask (vid_info->vfmt->Rmask) + bitsInMask (vid_info->vfmt->Gmask) + bitsInMask (vid_info->vfmt->Bmask);
//    }
    return depth;
}

int GetSurfacePixelFormat(void)
{
    int depth = get_display_depth();
    int unit = (depth + 1) & 0xF8;

    return (unit == 8 ? RGBFB_CHUNKY
            : depth == 15 && unit == 16 ? RGBFB_R5G5B5
            : depth == 16 && unit == 16 ? RGBFB_R5G6B5
            : unit == 24 ? RGBFB_B8G8R8
            : unit == 32 ? RGBFB_R8G8B8A8
            : RGBFB_NONE);
}

int graphics_init(bool mousecapture)
{
    int i,j;

    uae_sem_init (&vsync_wait_sem, 0, 1);

    graphics_subinit ();

    if (!init_colors ())
        return 0;

    return 1;
}

void graphics_leave (void)
{
    graphics_subshutdown ();
    bcm_host_deinit();
	SDL_VideoQuit();
}

#define  systemRedShift      (screenSurface->format->Rshift)
#define  systemGreenShift    (screenSurface->format->Gshift)
#define  systemBlueShift     (screenSurface->format->Bshift)
#define  systemRedMask       (screenSurface->format->Rmask)
#define  systemGreenMask     (screenSurface->format->Gmask)
#define  systemBlueMask      (screenSurface->format->Bmask)

static int save_png(SDL_Surface* surface, char *path)
{
    int w = surface->w;
    int h = surface->h;
    unsigned char * pix = (unsigned char *)surface->pixels;
    unsigned char writeBuffer[1024 * 3];
    FILE *f  = fopen(path,"wb");
    if(!f) return 0;
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                          NULL,
                          NULL,
                          NULL);
    if(!png_ptr)
    {
        fclose(f);
        return 0;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if(!info_ptr)
    {
        png_destroy_write_struct(&png_ptr,NULL);
        fclose(f);
        return 0;
    }

    png_init_io(png_ptr,f);

    png_set_IHDR(png_ptr,
                 info_ptr,
                 w,
                 h,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr,info_ptr);

    unsigned char *b = writeBuffer;

    int sizeX = w;
    int sizeY = h;
    int y;
    int x;

    unsigned short *p = (unsigned short *)pix;
    for(y = 0; y < sizeY; y++)
    {
        for(x = 0; x < sizeX; x++)
        {
            unsigned short v = p[x];

            *b++ = ((v & systemRedMask  ) >> systemRedShift  ) << 3; // R
            *b++ = ((v & systemGreenMask) >> systemGreenShift) << 2; // G
            *b++ = ((v & systemBlueMask ) >> systemBlueShift ) << 3; // B
        }
        p += surface->pitch / 2;
        png_write_row(png_ptr,writeBuffer);
        b = writeBuffer;
    }

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(f);
    return 1;
}

static void CreateScreenshot(void)
{
    int w, h;

    if(current_screenshot != NULL)
    {
        SDL_FreeSurface(current_screenshot);
        current_screenshot = NULL;
    }

    w=screenSurface->w;
    h=screenSurface->h;
    current_screenshot = SDL_CreateRGBSurfaceFrom(screenSurface->pixels, w, h, screenSurface->format->BitsPerPixel, screenSurface->pitch,
                         screenSurface->format->Rmask, screenSurface->format->Gmask, screenSurface->format->Bmask, screenSurface->format->Amask);
}

static int save_thumb(char *path)
{
    int ret = 0;
    if(current_screenshot != NULL)
    {
        ret = save_png(current_screenshot, path);
        SDL_FreeSurface(current_screenshot);
        current_screenshot = NULL;
    }
    return ret;
}

bool vsync_switchmode(int hz)
{
    int changed_height = changed_prefs.gfx_size.height;

    if (hz >= 55)
        hz = 60;
    else
        hz = 50;

    if (hz == 50 && currVSyncRate == 60)
    {
        // Switch from NTSC -> PAL
        switch (changed_height)
        {
        case 200:
            changed_height = 240;
            break;
        case 216:
            changed_height = 262;
            break;
        case 240:
            changed_height = 270;
            break;
        case 256:
            changed_height = 270;
            break;
        case 262:
            changed_height = 270;
            break;
        case 270:
            changed_height = 270;
            break;
        }
    }
    else if (hz == 60 && currVSyncRate == 50)
    {
        // Switch from PAL -> NTSC
        switch (changed_height)
        {
        case 200:
            changed_height = 200;
            break;
        case 216:
            changed_height = 200;
            break;
        case 240:
            changed_height = 200;
            break;
        case 256:
            changed_height = 216;
            break;
        case 262:
            changed_height = 216;
            break;
        case 270:
            changed_height = 240;
            break;
        }
    }

    if (changed_height == currprefs.gfx_size.height && hz == currprefs.chipset_refreshrate)
        return true;

    changed_prefs.gfx_size.height = changed_height;

    return true;
}

bool target_graphics_buffer_update(void)
{
    bool rate_changed = 0;
    //bool rate_changed = SetVSyncRate(currprefs.chipset_refreshrate);

    if (currprefs.gfx_size.height != changed_prefs.gfx_size.height)
    {
        update_display(&changed_prefs);
        rate_changed = true;
    }

    if (rate_changed)
    {
        black_screen_now();
        fpscounter_reset();
        time_per_frame = 1000 * 1000 / (currprefs.chipset_refreshrate);
    }

    return true;
}

#ifdef PICASSO96

int picasso_palette (void)
{
    int i, changed;

    changed = 0;
    for (i = 0; i < 256; i++)
    {
        int r = picasso96_state.CLUT[i].Red;
        int g = picasso96_state.CLUT[i].Green;
        int b = picasso96_state.CLUT[i].Blue;
        int value = (r << 16 | g << 8 | b);
        uae_u32 v = CONVERT_RGB(value);
        if (v !=  picasso_vidinfo.clut[i])
        {
            picasso_vidinfo.clut[i] = v;
            changed = 1;
        }
    }
    return changed;
}

static int resolution_compare (const void *a, const void *b)
{
    struct PicassoResolution *ma = (struct PicassoResolution *)a;
    struct PicassoResolution *mb = (struct PicassoResolution *)b;
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
static void sortmodes (void)
{
    int	i = 0, idx = -1;
    int pw = -1, ph = -1;
    while (DisplayModes[i].depth >= 0)
        i++;
    qsort (DisplayModes, i, sizeof (struct PicassoResolution), resolution_compare);
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

static void modesList (void)
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

void picasso_InitResolutions (void)
{
    struct MultiDisplay *md1;
    int i, count = 0;
    char tmp[200];
    int bit_idx;
    int bits[] = { 8, 16, 32 };

    Displays[0].primary = 1;
    Displays[0].disabled = 0;
    Displays[0].rect.left = 0;
    Displays[0].rect.top = 0;
    Displays[0].rect.right = 800;
    Displays[0].rect.bottom = 640;
    sprintf (tmp, "%s (%d*%d)", "Display", Displays[0].rect.right, Displays[0].rect.bottom);
    Displays[0].name = my_strdup(tmp);
    Displays[0].name2 = my_strdup("Display");

    md1 = Displays;
    DisplayModes = md1->DisplayModes = xmalloc (struct PicassoResolution, MAX_PICASSO_MODES);
    for (i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++)
    {
        for(bit_idx = 0; bit_idx < 3; ++bit_idx)
        {
            int bitdepth = bits[bit_idx];
            int bit_unit = (bitdepth + 1) & 0xF8;
            int rgbFormat = (bitdepth == 8 ? RGBFB_CLUT : (bitdepth == 16 ? RGBFB_R5G6B5 : RGBFB_R8G8B8A8));
            int pixelFormat = 1 << rgbFormat;
            pixelFormat |= RGBFF_CHUNKY;

            //if (SDL_VideoModeOK (x_size_table[i], y_size_table[i], 16, SDL_SWSURFACE))
            {
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
    }
    DisplayModes[count].depth = -1;
    sortmodes();
    modesList();
    DisplayModes = Displays[0].DisplayModes;
}

void gfx_set_picasso_state (int on)
{
    if (on == screen_is_picasso)
        return;

    screen_is_picasso = on;
    open_screen(&currprefs);
    picasso_vidinfo.rowbytes	= screenSurface->pitch;
}

void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
    depth >>= 3;
    if( ((unsigned)picasso_vidinfo.width == w ) &&
            ( (unsigned)picasso_vidinfo.height == h ) &&
            ( (unsigned)picasso_vidinfo.depth == depth ) &&
            ( picasso_vidinfo.selected_rgbformat == rgbfmt) )
        return;

    picasso_vidinfo.selected_rgbformat = rgbfmt;
    picasso_vidinfo.width = w;
    picasso_vidinfo.height = h;
    picasso_vidinfo.depth = 2; // Native depth
    picasso_vidinfo.extra_mem = 1;

    picasso_vidinfo.pixbytes = 2; // Native bytes
    if (screen_is_picasso)
    {
        open_screen(&currprefs);
        picasso_vidinfo.rowbytes	= screenSurface->pitch;
        picasso_vidinfo.rgbformat = RGBFB_R5G6B5;
    }
}

uae_u8 *gfx_lock_picasso (void)
{
    // We lock the surface directly after create and flip
    picasso_vidinfo.rowbytes = screenSurface->pitch;
    return (uae_u8 *)screenSurface->pixels;
}

void gfx_unlock_picasso (void)
{
    // We lock the surface directly after create and flip, so no unlock here
}

#endif // PICASSO96
