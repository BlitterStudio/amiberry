#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "xwin.h"
#include "custom.h"
#include "drawing.h"
#include "events.h"
#include "osdep/inputmode.h"
#include "savestate.h"

#include <png.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_gfxPrimitives.h>
#ifdef ANDROIDSDL
#include <android/log.h>
#endif

extern int stylusClickOverride;


/* SDL variable for output of emulation and menu */
SDL_Surface *prSDLScreen = NULL;
uae_u16 *prSDLScreenPixels;
static SDL_Surface *current_screenshot = NULL;
static int curr_layer_width = 0;

static char screenshot_filename_default[255]={
	'/', 't', 'm', 'p', '/', 'n', 'u', 'l', 'l', '.', 'p', 'n', 'g', '\0'
};
char *screenshot_filename=(char *)&screenshot_filename_default[0];
FILE *screenshot_file=NULL;
static void CreateScreenshot(void);
static int save_thumb(char *path);
int delay_savestate_frame = 0;

#define VIDEO_FLAGS_INIT SDL_SWSURFACE|SDL_FULLSCREEN
#ifdef ANDROIDSDL
#define VIDEO_FLAGS VIDEO_FLAGS_INIT
#else
#define VIDEO_FLAGS VIDEO_FLAGS_INIT | SDL_DOUBLEBUF
#endif

int justClicked = 0;
int mouseMoving = 0;
int fcounter = 0;
int doStylusRightClick = 0;

static unsigned long previous_synctime = 0;
static unsigned long next_synctime = 0;


int graphics_setup (void)
{
  return 1;
}


void InitAmigaVidMode(struct uae_prefs *p)
{
	/* Initialize structure for Amiga video modes */
	gfxvidinfo.pixbytes = 2;
	gfxvidinfo.bufmem = (uae_u8 *)prSDLScreen->pixels;
	gfxvidinfo.linemem = 0;
  gfxvidinfo.emergmem = 0;
  gfxvidinfo.width = p->gfx_size.width;
  gfxvidinfo.height = p->gfx_size.height;
	gfxvidinfo.maxblocklines = 0;
	gfxvidinfo.rowbytes = prSDLScreen->pitch;
}


void graphics_subshutdown (void)
{
  SDL_FreeSurface(prSDLScreen);
  prSDLScreen = NULL;
}


static void CalcPandoraWidth(struct uae_prefs *p)
{
  int amigaWidth = p->gfx_size.width;
  int amigaHeight = p->gfx_size.height;
  int pandHeight = 480;
  
  if(amigaWidth > 600)
    amigaWidth = amigaWidth / 2; // Hires selected, but we calc in lores
  int pandWidth = (amigaWidth * pandHeight) / amigaHeight;
  pandWidth = pandWidth & (~1);
  if((pandWidth * amigaHeight) / pandHeight < amigaWidth)
    pandWidth += 2;
  if(pandWidth > 800)
    pandWidth = 800;
  p->gfx_size_fs.width = pandWidth;
}


void update_display(struct uae_prefs *p)
{
  if(prSDLScreen != NULL)
    graphics_subshutdown();    

  CalcPandoraWidth(p);
	if(curr_layer_width != p->gfx_size_fs.width)
	{
	  char layersize[20];
	  snprintf(layersize, 20, "%dx480", p->gfx_size_fs.width);

#ifndef WIN32
  	setenv("SDL_OMAP_LAYER_SIZE", layersize, 1);
#endif
  }

#ifdef ANDROIDSDL
	update_onscreen();
#endif

  if(prSDLScreen == NULL || prSDLScreen->w != p->gfx_size.width || prSDLScreen->h != p->gfx_size.height)
  {
#if defined(PANDORA) && !defined(WIN32)
	  prSDLScreen = SDL_SetVideoMode(p->gfx_size.width, p->gfx_size.height, 16, SDL_SWSURFACE|SDL_FULLSCREEN|SDL_DOUBLEBUF);
#elif defined(PANDORA) && defined(WIN32)
	  prSDLScreen = SDL_SetVideoMode(p->gfx_size.width, p->gfx_size.height, 16, SDL_SWSURFACE|SDL_DOUBLEBUF);
#else
	  prSDLScreen = SDL_SetVideoMode(p->gfx_size.width, p->gfx_size.height, 16, SDL_SWSURFACE|SDL_FULLSCREEN);
#endif
  }
  SDL_ShowCursor(SDL_DISABLE);

  InitAmigaVidMode(p);
  init_row_map();
  framecnt = 1; // Don't draw frame before reset done
}


int check_prefs_changed_gfx (void)
{
  int changed = 0;
  
  if(currprefs.gfx_size.height != changed_prefs.gfx_size.height ||
     currprefs.gfx_size.width != changed_prefs.gfx_size.width ||
     currprefs.gfx_size_fs.width != changed_prefs.gfx_size_fs.width)
  {
  	cfgfile_configuration_change(1);
    currprefs.gfx_size.height = changed_prefs.gfx_size.height;
    currprefs.gfx_size.width = changed_prefs.gfx_size.width;
    currprefs.gfx_size_fs.width = changed_prefs.gfx_size_fs.width;
    update_display(&currprefs);
    changed = 1;
  }
  if (currprefs.leds_on_screen != changed_prefs.leds_on_screen ||
      currprefs.pandora_vertical_offset != changed_prefs.pandora_vertical_offset)	
  {
    currprefs.leds_on_screen = changed_prefs.leds_on_screen;
    currprefs.pandora_vertical_offset = changed_prefs.pandora_vertical_offset;
    changed = 1;
  }
  if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate) 
  {
  	currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
	  init_hz ();
	  changed = 1;
  }
  
  return changed;
}


int lockscr (void)
{
    SDL_LockSurface(prSDLScreen);
    return 1;
}


void unlockscr (void)
{
    SDL_UnlockSurface(prSDLScreen);
}


void flush_block ()
{
	SDL_UnlockSurface (prSDLScreen);

	if (show_inputmode)
		inputmode_redraw();	

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
  if(start < next_synctime && next_synctime - start > time_per_frame - 1000)
    usleep((next_synctime - start) - 1000);
  SDL_Flip(prSDLScreen);
  last_synctime = read_processor_time();
  
  if(last_synctime - next_synctime > time_per_frame - 1000)
    adjust_idletime(0);
  else
    adjust_idletime(next_synctime - start);
  
  if(last_synctime - next_synctime > time_per_frame - 5000)
    next_synctime = last_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
  else
    next_synctime = next_synctime + time_per_frame * (1 + currprefs.gfx_framerate);

	SDL_LockSurface (prSDLScreen);

	if(stylusClickOverride)
	{
		justClicked = 0;
		mouseMoving = 0;
	}
	else
	{
		if(justClicked)
		{
			buttonstate[0] = 0;
			buttonstate[2] = 0;
			justClicked = 0;
		}

		if(mouseMoving)
		{
			if(fcounter >= currprefs.pandora_tapDelay)
			{
				if(doStylusRightClick)
			  {
					buttonstate[2] = 1;
        }
				else
			  {
					buttonstate[0] = 1;
  				mouseMoving = 0;
  				justClicked = 1;
  				fcounter = 0;
				}
			}
			fcounter++;
		}
	}
	init_row_map();
}


void black_screen_now(void)
{
	SDL_FillRect(prSDLScreen,NULL,0);
	SDL_Flip(prSDLScreen);
}


static void graphics_subinit (void)
{
	if (prSDLScreen == NULL)
	{
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		return;
	}
	else
	{
		prSDLScreenPixels=(uae_u16 *)prSDLScreen->pixels;
		SDL_LockSurface(prSDLScreen);
		SDL_UnlockSurface(prSDLScreen);
		SDL_Flip(prSDLScreen);
		SDL_ShowCursor(SDL_DISABLE);

    InitAmigaVidMode(&currprefs);
	}
	lastmx = lastmy = 0;
	newmousecounters = 0;
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
	red_bits = bitsInMask(prSDLScreen->format->Rmask);
	green_bits = bitsInMask(prSDLScreen->format->Gmask);
	blue_bits = bitsInMask(prSDLScreen->format->Bmask);
	red_shift = maskShift(prSDLScreen->format->Rmask);
	green_shift = maskShift(prSDLScreen->format->Gmask);
	blue_shift = maskShift(prSDLScreen->format->Bmask);
	alloc_colors64k (red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, 0);
	for (i = 0; i < 4096; i++)
		xcolors[i] = xcolors[i] * 0x00010001;

	return 1;
}


int graphics_init (void)
{
	int i,j;

	graphics_subinit ();

	init_row_map ();

  if (!init_colors ())
		return 0;

  buttonstate[0] = buttonstate[1] = buttonstate[2] = 0;
  keyboard_init();
  
  return 1;
}

void graphics_leave (void)
{
  graphics_subshutdown ();
	SDL_VideoQuit();
  //dumpcustom ();
}


#define  systemRedShift      (prSDLScreen->format->Rshift)
#define  systemGreenShift    (prSDLScreen->format->Gshift)
#define  systemBlueShift     (prSDLScreen->format->Bshift)
#define  systemRedMask       (prSDLScreen->format->Rmask)
#define  systemGreenMask     (prSDLScreen->format->Gmask)
#define  systemBlueMask      (prSDLScreen->format->Bmask)

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
  if(!png_ptr) {
    fclose(f);
    return 0;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr) {
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

	w=prSDLScreen->w;
	h=prSDLScreen->h;

	current_screenshot = SDL_CreateRGBSurface(prSDLScreen->flags,w,h,prSDLScreen->format->BitsPerPixel,prSDLScreen->format->Rmask,prSDLScreen->format->Gmask,prSDLScreen->format->Bmask,prSDLScreen->format->Amask);
  SDL_BlitSurface(prSDLScreen, NULL, current_screenshot, NULL);
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
