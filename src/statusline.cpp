#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "autoconf.h"
#include "gui.h"
#include "picasso96.h"
#include "drawing.h"
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
		int num1 = -1, num2 = -1, num3 = -1;
		
		if(led == 0 && nr_units() < 1)
		  continue; // skip led for HD if not in use
		if (led > 0) {
			/* Floppy */
			int track = gui_data.drive_track[led-1];
			num1 = -1;
			num2 = track / 10;
			num3 = track % 10;
			on = gui_data.drive_motor[led-1];
			on_rgb = 0x0c0;
			off_rgb = 0x030;
      if (gui_data.drive_writing[led-1])
		    on_rgb = 0xc00;
		} else if (led < -1) {
			/* Idle time */
			int track = idletime_percent;
			num1 = track / 100;
			num2 = (track - num1 * 100) / 10;
			num3 = track % 10;
			on = 1;
			on_rgb = 0x666;
			off_rgb = 0x666;
		} else if (led < 0) {
			/* Power */
			int track = gui_data.fps;
			num1 = track / 100;
			num2 = (track - num1 * 100) / 10;
			num3 = track % 10;
			on = gui_data.powerled;
			on_rgb = 0xc00;
			off_rgb = 0x300;
			if(gui_data.cpu_halted) {
			  on = 1;
			  num1 = -1;
			  num2 = 11;
			  num3 = gui_data.cpu_halted;
			}
		} else {
			/* Hard disk */
			num1 = -1;
			num2 = 11;
			num3 = 12;
			
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
      int tn = num1 > 0 ? 3 : 2;
      int offs = (TD_LED_WIDTH - tn * TD_NUM_WIDTH) / 2;
      if(num1 > 0)
      {
      	write_tdnumber (buf, x + offs, y - TD_PADY, num1);
      	offs += TD_NUM_WIDTH;
      }
      write_tdnumber (buf, x + offs, y - TD_PADY, num2);
      write_tdnumber (buf, x + offs + TD_NUM_WIDTH, y - TD_PADY, num3);
	  }
	  x += TD_WIDTH;
  }
}
