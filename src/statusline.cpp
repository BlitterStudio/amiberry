#include "sysconfig.h"
#include "sysdeps.h"

#include <ctype.h>
#include <assert.h>

#include "options.h"
#include "uae.h"
#include "xwin.h"
#include "gui.h"
#include "custom.h"
#include "drawing.h"
#include "inputdevice.h"
#include "statusline.h"

#define STATUSLINE_MS 3000

/*
* Some code to put status information on the screen.
*/

static const char* numbers = {
	/* ugly  0123456789CHD%+-PNK */
	"+++++++--++++-+++++++++++++++++-++++++++++++++++++++++++++++++++++++++++++++-++++++-++++----++---+--------------+++++++++++++++++++++"
	"+xxxxx+--+xx+-+xxxxx++xxxxx++x+-+x++xxxxx++xxxxx++xxxxx++xxxxx++xxxxx++xxxx+-+x++x+-+xxx++-+xx+-+x---+----------+xxxxx++x+++x++x++x++"
	"+x+++x+--++x+-+++++x++++++x++x+++x++x++++++x++++++++++x++x+++x++x+++x++x++++-+x++x+-+x++x+--+x++x+--+x+----+++--+x---x++xx++x++x+x+++"
	"+x+-+x+---+x+-+xxxxx++xxxxx++xxxxx++xxxxx++xxxxx+--++x+-+xxxxx++xxxxx++x+----+xxxx+-+x++x+----+x+--+xxx+--+xxx+-+xxxxx++x+x+x++xx++++"
	"+x+++x+---+x+-+x++++++++++x++++++x++++++x++x+++x+--+x+--+x+++x++++++x++x++++-+x++x+-+x++x+---+x+x+--+x+----+++--+x++++++x+x+x++x+x+++"
	"+xxxxx+---+x+-+xxxxx++xxxxx+----+x++xxxxx++xxxxx+--+x+--+xxxxx++xxxxx++xxxx+-+x++x+-+xxx+---+x++xx--------------+x+----+x++xx++x++x++"
	"+++++++---+++-++++++++++++++----+++++++++++++++++--+++--++++++++++++++++++++-++++++-++++------------------------+++----++++++++++++++"
};

STATIC_INLINE uae_u32 ledcolor(uae_u32 c, uae_u32* rc, uae_u32* gc, uae_u32* bc, uae_u32* a)
{
	uae_u32 v = rc[(c >> 16) & 0xff] | gc[(c >> 8) & 0xff] | bc[(c >> 0) & 0xff];
	if (a)
		v |= a[255 - ((c >> 24) & 0xff)];
	return v;
}

static void write_tdnumber(uae_u8* buf, int bpp, int x, int y, int num, uae_u32 c1, uae_u32 c2)
{
	int j;
	const char* numptr;

	numptr = numbers + num * TD_NUM_WIDTH + NUMBERS_NUM * TD_NUM_WIDTH * y;
	for (j = 0; j < TD_NUM_WIDTH; j++)
	{
		if (*numptr == 'x')
			putpixel(buf, nullptr, bpp, x + j, c1, 1);
		else if (*numptr == '+')
			putpixel(buf, nullptr, bpp, x + j, c2, 0);
		numptr++;
	}
}

void draw_status_line_single(uae_u8* buf, int bpp, int y, int totalwidth, uae_u32* rc, uae_u32* gc, uae_u32* bc,
                             uae_u32* alpha)
{
	int x_start, j, led, border;
	uae_u32 c1, c2, cb;

	c1 = ledcolor(0x00ffffff, rc, gc, bc, alpha);
	c2 = ledcolor(0x00000000, rc, gc, bc, alpha);
	cb = ledcolor(TD_BORDER, rc, gc, bc, alpha);

	if (td_pos & TD_RIGHT)
		x_start = totalwidth - TD_PADX - VISIBLE_LEDS * TD_WIDTH;
	else
		x_start = TD_PADX;

	for (led = 0; led < LED_MAX; led++)
	{
		int side, pos, num1 = -1, num2 = -1, num3 = -1, num4 = -1;
		int x, c, on = 0, am = 2;
		xcolnr on_rgb = 0, on_rgb2 = 0, off_rgb = 0, pen_rgb = 0;
		int half = 0;

		pen_rgb = c1;
		if (led >= LED_DF0 && led <= LED_DF3)
		{
			int pled = led - LED_DF0;
			int track = gui_data.drive_track[pled];
			pos = 7 + pled;
			on_rgb = 0x00cc00;
			on_rgb2 = 0x006600;
			off_rgb = 0x003300;
			if (!gui_data.drive_disabled[pled])
			{
				num1 = -1;
				num2 = track / 10;
				num3 = track % 10;
				on = gui_data.drive_motor[pled];
				if (gui_data.drive_writing[pled])
				{
					on_rgb = 0xcc0000;
					on_rgb2 = 0x880000;
				}
				half = gui_data.drive_side ? 1 : -1;
				if (gui_data.df[pled][0] == 0)
					pen_rgb = ledcolor(0x00aaaaaa, rc, gc, bc, alpha);
			}
			side = gui_data.drive_side;
		}
		else if (led == LED_POWER)
		{
			pos = 3;
			//on_rgb = ((gui_data.powerled_brightness * 10 / 16) + 0x33) << 16;
			on_rgb = ((100 * 10 / 16) + 0x33) << 16;
			on = 1;
			off_rgb = 0x330000;
		}
		else if (led == LED_CD)
		{
			pos = 5;
			if (gui_data.cd >= 0)
			{
				on = gui_data.cd & (LED_CD_AUDIO | LED_CD_ACTIVE);
				on_rgb = (on & LED_CD_AUDIO) ? 0x00cc00 : 0x0000cc;
				if ((gui_data.cd & LED_CD_ACTIVE2) && !(gui_data.cd & LED_CD_AUDIO))
				{
					on_rgb &= 0xfefefe;
					on_rgb >>= 1;
				}
				off_rgb = 0x000033;
				num1 = -1;
				num2 = 10;
				num3 = 12;
			}
		}
		else if (led == LED_HD)
		{
			pos = 4;
			if (gui_data.hd >= 0)
			{
				on = gui_data.hd;
				on_rgb = on == 2 ? 0xcc0000 : 0x0000cc;
				off_rgb = 0x000033;
				num1 = -1;
				num2 = 11;
				num3 = 12;
			}
		}
		else if (led == LED_FPS)
		{
			pos = 2;
			if (pause_emulation)
			{
				num1 = -1;
				num2 = -1;
				num3 = 16;
				on_rgb = 0xcccccc;
				off_rgb = 0x000000;
				am = 2;
			}
			else
			{
				int fps = (gui_data.fps + 5) / 10;
				on_rgb = 0x000000;
				off_rgb = gui_data.fps_color ? 0xcccc00 : 0x000000;
				am = 3;
				if (fps > 999)
				{
					fps += 50;
					fps /= 10;
					if (fps > 999)
						fps = 999;
					num1 = fps / 100;
					num2 = 18;
					num3 = (fps - num1 * 100) / 10;
				}
				else
				{
					num1 = fps / 100;
					num2 = (fps - num1 * 100) / 10;
					num3 = fps % 10;
					if (num1 == 0)
						am = 2;
				}
			}
		}
		else if (led == LED_CPU)
		{
			int idle = (gui_data.idle + 5) / 10;
			pos = 1;
			on_rgb = 0xcc0000;
			off_rgb = 0x000000;
			if (gui_data.cpu_halted)
			{
				idle = 0;
				on = 1;
				if (gui_data.cpu_halted < 0)
				{
					on_rgb = 0x000000;
					num1 = 16; // PPC
					num2 = 16;
					num3 = 10;
					am = 3;
				}
				else
				{
					on_rgb = 0xcccc00;
					num1 = gui_data.cpu_halted >= 10 ? 11 : -1;
					num2 = gui_data.cpu_halted >= 10 ? gui_data.cpu_halted / 10 : 11;
					num3 = gui_data.cpu_halted % 10;
					am = 2;
				}
			}
			else
			{
				num1 = idle / 100;
				num2 = (idle - num1 * 100) / 10;
				num3 = idle % 10;
				num4 = num1 == 0 ? 13 : -1;
				am = 3;
			}
		}
		else if (led == LED_SND && gui_data.sndbuf_avail)
		{
			int snd = abs(gui_data.sndbuf + 5) / 10;
			if (snd > 99)
				snd = 99;
			pos = 0;
			on = gui_data.sndbuf_status;
			if (on < 3)
			{
				num1 = gui_data.sndbuf < 0 ? 15 : 14;
				num2 = snd / 10;
				num3 = snd % 10;
			}
			on_rgb = 0x000000;
			if (on < 0)
				on_rgb = 0xcccc00; // underflow
			else if (on == 2)
				on_rgb = 0xcc0000; // really big overflow
			else if (on == 1)
				on_rgb = 0x0000cc; // "normal" overflow
			off_rgb = 0x000000;
			am = 3;
		}
		else if (led == LED_MD && gui_data.drive_disabled[3])
		{
			// DF3 reused as internal non-volatile ram led (cd32/cdtv)
			pos = 7 + 3;
			if (gui_data.md >= 0)
			{
				on = gui_data.md;
				on_rgb = on == 2 ? 0xcc0000 : 0x00cc00;
				off_rgb = 0x003300;
			}
			num1 = -1;
			num2 = -1;
			num3 = -1;
		}
		else if (led == LED_NET)
		{
			pos = 6;
			if (gui_data.net >= 0)
			{
				on = gui_data.net;
				on_rgb = 0;
				if (on & 1)
					on_rgb |= 0x00cc00;
				if (on & 2)
					on_rgb |= 0xcc0000;
				off_rgb = 0x000000;
				num1 = -1;
				num2 = -1;
				num3 = 17;
				am = 1;
			}
		}
		else
		{
			continue;
		}
		on_rgb |= 0x33000000;
		off_rgb |= 0x33000000;
		if (half > 0)
		{
			c = ledcolor(on ? (y >= TD_TOTAL_HEIGHT / 2 ? on_rgb2 : on_rgb) : off_rgb, rc, gc, bc, alpha);
		}
		else if (half < 0)
		{
			c = ledcolor(on ? (y < TD_TOTAL_HEIGHT / 2 ? on_rgb2 : on_rgb) : off_rgb, rc, gc, bc, alpha);
		}
		else
		{
			c = ledcolor(on ? on_rgb : off_rgb, rc, gc, bc, alpha);
		}
		border = 0;
		if (y == 0 || y == TD_TOTAL_HEIGHT - 1)
		{
			c = ledcolor(TD_BORDER, rc, gc, bc, alpha);
			border = 1;
		}

		x = x_start + pos * TD_WIDTH;
		if (!border)
			putpixel(buf, nullptr, bpp, x - 1, cb, 0);
		for (j = 0; j < TD_LED_WIDTH; j++)
			putpixel(buf, nullptr, bpp, x + j, c, 0);
		if (!border)
			putpixel(buf, nullptr, bpp, x + j, cb, 0);

		if (y >= TD_PADY && y - TD_PADY < TD_NUM_HEIGHT)
		{
			if (num3 >= 0)
			{
				x += (TD_LED_WIDTH - am * TD_NUM_WIDTH) / 2;
				if (num1 > 0)
				{
					write_tdnumber(buf, bpp, x, y - TD_PADY, num1, pen_rgb, c2);
					x += TD_NUM_WIDTH;
				}
				if (num2 >= 0)
				{
					write_tdnumber(buf, bpp, x, y - TD_PADY, num2, pen_rgb, c2);
					x += TD_NUM_WIDTH;
				}
				write_tdnumber(buf, bpp, x, y - TD_PADY, num3, pen_rgb, c2);
				x += TD_NUM_WIDTH;
				if (num4 > 0)
					write_tdnumber(buf, bpp, x, y - TD_PADY, num4, pen_rgb, c2);
			}
		}
	}
}

#define MAX_STATUSLINE_QUEUE 8

struct statusline_struct
{
	TCHAR* text;
	int type;
};

struct statusline_struct statusline_data[MAX_STATUSLINE_QUEUE];
static TCHAR* statusline_text_active;
static int statusline_delay;
static bool statusline_had_changed;

bool has_statusline_updated(void)
{
	bool v = statusline_had_changed;
	statusline_had_changed = false;
	return v;
}

static void statusline_update_notification(void)
{
	statusline_had_changed = true;
}

void statusline_clear(void)
{
	statusline_text_active = nullptr;
	statusline_delay = 0;
	for (int i = 0; i < MAX_STATUSLINE_QUEUE; i++)
	{
		xfree(statusline_data[i].text);
		statusline_data[i].text = nullptr;
	}
	statusline_update_notification();
}

const TCHAR* statusline_fetch(void)
{
	return statusline_text_active;
}

void statusline_vsync(void)
{
	if (!statusline_data[0].text)
		return;
	if (statusline_delay == 0)
		statusline_delay = STATUSLINE_MS * vblank_hz / (1000 * 1);
	if (statusline_delay > STATUSLINE_MS * vblank_hz / (1000 * 1))
		statusline_delay = STATUSLINE_MS * vblank_hz / (1000 * 1);
	if (statusline_delay > STATUSLINE_MS * vblank_hz / (1000 * 3) && statusline_data[1].text)
		statusline_delay = STATUSLINE_MS * vblank_hz / (1000 * 3);
	statusline_delay--;
	if (statusline_delay)
		return;
	statusline_text_active = nullptr;
	xfree(statusline_data[0].text);
	for (int i = 1; i < MAX_STATUSLINE_QUEUE; i++)
	{
		statusline_data[i - 1].text = statusline_data[i].text;
	}
	statusline_data[MAX_STATUSLINE_QUEUE - 1].text = nullptr;
	statusline_text_active = statusline_data[0].text;
	statusline_update_notification();
}

void statusline_single_erase(int monid, uae_u8* buf, int bpp, int y, int totalwidth)
{
	memset(buf, 0, bpp * totalwidth);
}
