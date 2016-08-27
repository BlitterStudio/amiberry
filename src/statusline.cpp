#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "td-sdl/thread.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "autoconf.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
#include "savestate.h"
#include "statusline.h"

extern SDL_Surface *prSDLScreen;
extern int idletime_percent;

/*
 * Some code to put status information on the screen.
 */

static const char *numbers = { /* ugly  0123456789CHD%+- */
"+++++++--++++-+++++++++++++++++-++++++++++++++++++++++++++++++++++++++++++++-++++++-++++----++---+--------------"
"+xxxxx+--+xx+-+xxxxx++xxxxx++x+-+x++xxxxx++xxxxx++xxxxx++xxxxx++xxxxx++xxxx+-+x++x+-+xxx++-+xx+-+x---+----------"
"+x+++x+--++x+-+++++x++++++x++x+++x++x++++++x++++++++++x++x+++x++x+++x++x++++-+x++x+-+x++x+--+x++x+--+x+----+++--"
"+x+-+x+---+x+-+xxxxx++xxxxx++xxxxx++xxxxx++xxxxx+--++x+-+xxxxx++xxxxx++x+----+xxxx+-+x++x+----+x+--+xxx+--+xxx+-"
"+x+++x+---+x+-+x++++++++++x++++++x++++++x++x+++x+--+x+--+x+++x++++++x++x++++-+x++x+-+x++x+---+x+x+--+x+----+++--"
"+xxxxx+---+x+-+xxxxx++xxxxx+----+x++xxxxx++xxxxx+--+x+--+xxxxx++xxxxx++xxxx+-+x++x+-+xxx+---+x++xx--------------"
"+++++++---+++-++++++++++++++----+++++++++++++++++--+++--++++++++++++++++++++-++++++-++++------------------------"
};

STATIC_INLINE void putpixel (uae_u8 *buf, int x, xcolnr c8)
{
	uae_u16 *p = (uae_u16 *)buf + x;
	*p = (uae_u16)c8;
}

static void write_tdnumber (uae_u8 *buf, int x, int y, int num)
{
  int j;
  const char *numptr;

  numptr = numbers + num * TD_NUM_WIDTH + NUMBERS_NUM * TD_NUM_WIDTH * y;
  for (j = 0; j < TD_NUM_WIDTH; j++) {
  	if (*numptr == 'x')
      putpixel (buf, x + j, xcolors[0xfff]);
    else if (*numptr == '+')
      putpixel (buf, x + j, xcolors[0x000]);
	  numptr++;
  }
}

void draw_status_line_single (uae_u8 *buf, int y, int totalwidth)
{
  int x, i, j, led, on;
  int on_rgb, off_rgb, c;

  x = totalwidth - TD_PADX - VISIBLE_LEDS * TD_WIDTH;
	x += 100 - (TD_WIDTH * (currprefs.nr_floppies - 1)) - TD_WIDTH;
  if(nr_units() < 1)
    x += TD_WIDTH;
  if(currprefs.pandora_hide_idle_led)
    x += TD_WIDTH;
    
  if(picasso_on)
    memset (buf + (x - 4) * 2, 0, (prSDLScreen->w - x + 4) * 2);
  else
    memset (buf + (x - 4) * gfxvidinfo.pixbytes, 0, (gfxvidinfo.outwidth - x + 4) * gfxvidinfo.pixbytes);

	for (led = (currprefs.pandora_hide_idle_led == 0) ? -2 : -1; led < (currprefs.nr_floppies+1); led++) {
		int track;
		if(led == 0 && nr_units() < 1)
		  continue; // skip led for HD if not in use
		if (led > 0) {
			/* Floppy */
			track = gui_data.drive_track[led-1];
			on = gui_data.drive_motor[led-1];
			on_rgb = 0x0c0;
			off_rgb = 0x030;
      if (gui_data.drive_writing[led-1])
		    on_rgb = 0xc00;
		} else if (led < -1) {
			/* Idle time */
			track = idletime_percent;
			on = 1;
			on_rgb = 0x666;
			off_rgb = 0x666;
		} else if (led < 0) {
			/* Power */
			track = gui_data.fps;
			on = gui_data.powerled;
			on_rgb = 0xc00;
			off_rgb = 0x300;
		} else {
			/* Hard disk */
			track = -2;
			
			switch (gui_data.hd) {
				case HDLED_OFF:
					on = 0;
					off_rgb = 0x003;
					break;
				case HDLED_READ:
					on = 1;
					on_rgb = 0x00c;
					off_rgb = 0x003;
					break;
				case HDLED_WRITE:
					on = 1;
					on_rgb = 0xc00;
					off_rgb = 0x300;
					break;
			}
		}
  	c = xcolors[on ? on_rgb : off_rgb];

    for (j = 0; j < TD_LED_WIDTH; j++) 
	    putpixel (buf, x + j, c);

	  if (y >= TD_PADY && y - TD_PADY < TD_NUM_HEIGHT) {
	    if (track >= 0) {
        int tn = track >= 100 ? 3 : 2;
	      int offs = (TD_LED_WIDTH - tn * TD_NUM_WIDTH) / 2;
        if(track >= 100)
        {
        	write_tdnumber (buf, x + offs, y - TD_PADY, track / 100);
        	offs += TD_NUM_WIDTH;
        }
	      write_tdnumber (buf, x + offs, y - TD_PADY, (track / 10) % 10);
	      write_tdnumber (buf, x + offs + TD_NUM_WIDTH, y - TD_PADY, track % 10);
	    }
		  else if (nr_units() > 0) {
    		int offs = (TD_LED_WIDTH - 2 * TD_NUM_WIDTH) / 2;
			  //write_tdletter(buf, x + offs, y - TD_PADY, 'H');
			  //write_tdletter(buf, x + offs + TD_NUM_WIDTH, y - TD_PADY, 'D');
			  write_tdnumber (buf, x + offs, y - TD_PADY, 11);
			  write_tdnumber (buf, x + offs + TD_NUM_WIDTH, y - TD_PADY, 12);
	    }
	  }
	  x += TD_WIDTH;
  }
}
