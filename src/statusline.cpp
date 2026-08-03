#include "sysconfig.h"
#include "sysdeps.h"

#include <cctype>
#include <cassert>

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

static bool td_custom;

void statusline_getpos(int monid, int *x, int *y, int width, int height)
{
	int mx = td_custom ? 1 : statusline_get_multiplier(monid) / 100;
	int total_height = TD_TOTAL_HEIGHT * mx;
	if (currprefs.osd_pos.x >= 20000) {
		if (currprefs.osd_pos.x >= 30000)
			*y = width * (currprefs.osd_pos.x - 30000) / 1000;
		else
			*y = width - (width * (30000 - currprefs.osd_pos.y) / 1000);
	} else {
		if (currprefs.osd_pos.x >= 0)
			*x = currprefs.osd_pos.x;
		else
			*x = -currprefs.osd_pos.x + 1;
	}
	if (currprefs.osd_pos.y >= 20000) {
		if (currprefs.osd_pos.y >= 30000)
			*y = (height - total_height) * (currprefs.osd_pos.y - 30000) / 1000;
		else
			*y = (height - total_height) - ((height - total_height) * (30000 - currprefs.osd_pos.y) / 1000);
	} else {
		if (currprefs.osd_pos.y >= 0)
			*y = height - total_height - currprefs.osd_pos.y;
		else
			*y = -currprefs.osd_pos.y + 1;
	}
}

int td_numbers_pos = TD_RIGHT | TD_BOTTOM;
int td_numbers_width = TD_DEFAULT_NUM_WIDTH;
int td_numbers_height = TD_DEFAULT_NUM_HEIGHT;
int td_numbers_padx = TD_DEFAULT_PADX;
int td_numbers_pady = TD_DEFAULT_PADY;
const TCHAR *td_characters = _T("0123456789CHD%+-PNKV");
int td_led_width = TD_DEFAULT_LED_WIDTH;
static int td_led_height = TD_DEFAULT_LED_HEIGHT;
int td_width = TD_DEFAULT_WIDTH;

static const char *numbers_default = { /* ugly  0123456789CHD%+-PNKV */
	"+++++++--++++-+++++++++++++++++-++++++++++++++++++++++++++++++++++++++++++++-++++++-++++----++---+--------------++++++++++-++++++++++++  +++"
	"+xxxxx+--+xx+-+xxxxx++xxxxx++x+-+x++xxxxx++xxxxx++xxxxx++xxxxx++xxxxx++xxxx+-+x++x+-+xxx++-+xx+-+x---+----------+xxxxx++x+-+x++x++x++x+  +x+"
	"+x+++x+--++x+-+++++x++++++x++x+++x++x++++++x++++++++++x++x+++x++x+++x++x++++-+x++x+-+x++x+--+x++x+--+x+----+++--+x---x++xx++x++x+x+++x+  +x+"
	"+x+-+x+---+x+-+xxxxx++xxxxx++xxxxx++xxxxx++xxxxx+--++x+-+xxxxx++xxxxx++x+----+xxxx+-+x++x+----+x+--+xxx+--+xxx+-+xxxxx++x+x+x++xx+   +x++x+ "
	"+x+++x+---+x+-+x++++++++++x++++++x++++++x++x+++x+--+x+--+x+++x++++++x++x++++-+x++x+-+x++x+---+x+x+--+x+----+++--+x++++++x+x+x++x+x++  +xx+  "
	"+xxxxx+---+x+-+xxxxx++xxxxx+----+x++xxxxx++xxxxx+--+x+--+xxxxx++xxxxx++xxxx+-+x++x+-+xxx+---+x++xx--------------+x+----+x++xx++x++x+  +xx+  "
	"+++++++---+++-++++++++++++++----+++++++++++++++++--+++--++++++++++++++++++++-++++++-++++------------------------+++----+++++++++++++  ++++  "
//   x      x      x      x      x      x      x      x      x      x      x      x      x      x      x      x      x      x      x      x      x  
};
static const char *numbers_mapping = "0123456789CHD%+-PNKV";

static const char *statusline_numbers = numbers_default;


static uae_u32 ledcolor(uae_u32 c, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *a)
{
	uae_u32 v = rc[(c >> 16) & 0xff] | gc[(c >> 8) & 0xff] | bc[(c >> 0) & 0xff];
	if (a)
		v |= a[255 - ((c >> 24) & 0xff)];
	return v;
}

static void write_tdnumber(uae_u8 *buf, int x, int y, int num, uae_u32 c1, uae_u32 c2, int mult)
{
	int j;
	const char *numptr;

	numptr = statusline_numbers + num * td_numbers_width + NUMBERS_NUM * td_numbers_width * y;
	for (j = 0; j < td_numbers_width; j++) {
		for (int k = 0; k < mult; k++) {
			if (*numptr == 'x')
				putpixel(buf, NULL, x + j * mult + k, c1);
			else if (*numptr == '+')
				putpixel(buf, NULL, x + j * mult + k, c2);
		}
		numptr++;
	}
}

static uae_u32 rgbmuldiv(uae_u32 rgb, int mul, int div)
{
	uae_u32 out = 0;
	for (int i = 0; i < 3; i++) {
		int v = (rgb >> (i * 8)) & 0xff;
		v *= mul;
		v /= div;
		out |= v << (i * 8);
	}
	out |= rgb & 0xff000000;
	return out;
}

static int statusline_mult[2];

void statusline_set_font(const char *newnumbers, int width, int height)
{
	td_numbers_width = TD_DEFAULT_NUM_WIDTH;
	td_numbers_height = TD_DEFAULT_NUM_HEIGHT;
	td_numbers_padx = TD_DEFAULT_PADX;
	td_numbers_pady = TD_DEFAULT_PADY;
	td_led_width = TD_DEFAULT_LED_WIDTH;
	td_led_height = TD_DEFAULT_LED_HEIGHT;
	td_width = TD_DEFAULT_WIDTH;
	td_custom = false;
	statusline_numbers = numbers_default;
	if (!newnumbers)
		return;
	statusline_numbers = newnumbers;
	td_numbers_width = width;
	td_numbers_height = height;
	td_led_width = td_numbers_width * 3 + td_numbers_width / 2;
	td_width = td_led_width + 6;
	td_custom = true;
}

int statusline_set_multiplier(int monid, int width, int height)
{
	struct amigadisplay *ad = &adisplays[monid];
	int idx = ad->picasso_on ? 1 : 0;
	int mult = currprefs.leds_on_screen_multiplier[idx];
	mult = std::max(mult, 1 * 100);
	mult = std::min(mult, 4 * 100);
	statusline_mult[idx] = mult;
	return mult;
}

int statusline_get_multiplier(int monid)
{
	struct amigadisplay *ad = &adisplays[monid];
	int idx = ad->picasso_on ? 1 : 0;
	if (statusline_mult[idx] < 1 * 100)
		return 1 * 100;
	return statusline_mult[idx];
}

const uae_s8 defaultosdledpos[LED_MAX] = {
	LED_SND,
	LED_CPU,
	LED_FPS,
	LED_LINES,
	LED_CAPS,
	LED_POWER,
	LED_HD,
	LED_CD,
	LED_DF0,
	LED_DF1,
	LED_DF2,
	LED_DF3,
#ifdef AMIBERRY
	LED_TEMP,
#endif
	LED_MD,
	LED_NET
};

bool statusline_led_visible(int led)
{
	if (led == LED_SND && !gui_data.sndbuf_avail) {
		return false;
	}
	if (led == LED_MD && gui_data.md < 0) {
		return false;
	}
	if (led == LED_HD && gui_data.hd < 0) {
		return false;
	}
	if (led == LED_CD && gui_data.cd < 0) {
		return false;
	}
	if (led == LED_NET && gui_data.net < 0) {
		return false;
	}
	return true;
}

void draw_status_line_single(int monid, uae_u8 *buf, int y, int totalwidth, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha)
{
	struct amigadisplay *ad = &adisplays[monid];
	int x_start, j, led, border, pos;
	uae_u32 c1, c2, cb;
	int mult = td_custom ? 1 : statusline_mult[ad->picasso_on ? 1 : 0] / 100;
	int num_leds = LED_MAX;
	const uae_s8 *ledposptr;
	uae_s8 ledposconf[LED_MAX];

	if (!mult) {
		mult = 1;
	}

	y /= mult;

	num_leds = 0;
	ledposptr = ledposconf;
	pos = 0;
	memset(ledposconf, -1, sizeof(ledposconf));
	for (int i = 0; i < sizeof(defaultosdledpos); i++) {
		led = defaultosdledpos[i];
		if (!statusline_led_visible(led)) {
			continue;
		}
		if (currprefs.leds_on_screen_mask[ad->picasso_on ? 1 : 0] & (1 << led)) {
			ledposconf[led] = num_leds;
			num_leds++;
		}
	}

	c1 = ledcolor(0x00ffffff, rc, gc, bc, alpha);
	c2 = ledcolor(0x00111111, rc, gc, bc, alpha);

	if (td_numbers_pos & TD_RIGHT) {
		x_start = totalwidth - (td_numbers_padx + num_leds * td_width) * mult;
	} else {
		x_start = td_numbers_padx * mult;
	}

	for (led = 0; led < LED_MAX; led++) {
		char text[TD_MAX_CHARS + 1] = { 0 };
		int x, c, on = 0;
		xcolnr on_rgb = 0, on_rgb2 = 0, off_rgb = 0, pen_rgb = 0;
		int half = 0, extraborder = 0;

		cb = ledcolor(TD_BORDER, rc, gc, bc, alpha);
		pen_rgb = c1;
		pos = ledposconf[led];
		if (pos < 0) {
			continue;
		}

		if (led >= LED_DF0 && led <= LED_DF3) {
			int pled = led - LED_DF0;
			struct floppyslot *fs = &currprefs.floppyslots[pled];
			struct gui_info_drive *gid = &gui_data.drives[pled];
			int track = gid->drive_track;
			on_rgb = 0x00cc00;
			if (!gid->drive_disabled) {
				sprintf(text, "%02d", track);
				on = gid->drive_motor;
				if (gid->drive_writing) {
					on_rgb = 0xcc0000;
				}
				half = gui_data.drive_side ? 1 : -1;
				if (!gid->floppy_inserted) {
					pen_rgb = ledcolor(0x00aaaaaa, rc, gc, bc, alpha);
				} else if (gid->floppy_protected) {
					cb = ledcolor(0x00cc00, rc, gc, bc, alpha);
					extraborder = 1;
				}
			}
			on_rgb &= 0xffffff;
			off_rgb = rgbmuldiv(on_rgb, 2, 4);
			on_rgb2 = rgbmuldiv(on_rgb, 2, 3);
		} else if (led == LED_CAPS) {
			on_rgb = 0xcc9900;
			on = gui_data.capslock;
			off_rgb = (on_rgb & 0xfefefe) >> 1;
		} else if (led == LED_POWER) {
			on_rgb = ((gui_data.powerled_brightness * 10 / 16) + 0x33) << 16;
			on = 1;
			off_rgb = 0x330000;
		} else if (led == LED_CD) {
			on = gui_data.cd & (LED_CD_AUDIO | LED_CD_ACTIVE);
			if (on & LED_CD_AUDIO) {
				on_rgb = 0x009900;
			} else if (on == LED_CD_ACTIVE) {
				on_rgb = 0x000099;
			} else {
				on = 0;
			}
			off_rgb = 0x000033;
			strcpy(text, "CD");
		} else if (led == LED_HD) {
			on = gui_data.hd;
			on_rgb = on == 2 ? 0xcc0000 : 0x0000cc;
			off_rgb = 0x000033;
			strcpy(text, "HD");
		} else if (led == LED_FPS) {
			if (pause_emulation) {
				strcpy(text, "P");
				on_rgb = 0xcccccc;
				off_rgb = 0x111111;
			} else {
				int fps = (gui_data.fps + 5) / 10;
				on_rgb = 0x111111;
				off_rgb = gui_data.fps_color == 1 ? 0xcccc00 : (gui_data.fps_color == 2 ? 0x0000cc : 0x111111);
				if (gui_data.fps_color >= 2) {
					if (ad->picasso_on) {
						fps = (int)(p96vblank + 0.5f);
					} else {
						fps = -1;
					}
				}
				if (fps < 0) {
					strcpy(text, "--");
				} else {
					if (fps > 999) {
						if (fps > 9999) {
							fps = 9999;
						}
						sprintf(text, "%dK", fps / 1000);
					} else if (fps >= 100) {
						sprintf(text, "%3d", fps);
					} else {
						sprintf(text, "%2d", fps);
					}
				}
			}
		} else if (led == LED_CPU) {
			int idle = (gui_data.idle + 5) / 10;
			on_rgb = 0xcc0000;
			off_rgb = 0x111111;
			if (gui_data.cpu_halted || gui_data.cpu_stopped) {
				idle = 0;
				if (gui_data.cpu_halted < 0) {
					on = 1;
					on_rgb = 0x111111;
					strcpy(text, "PPC");
				} else if (gui_data.cpu_halted > 0) {
					on = 1;
					on_rgb = 0xcccc00;
					sprintf(text, "H%d", gui_data.cpu_halted);
				}
			} else if (idle >= 100) {
				if (idle >= 1000) {
					idle = 999;
				}
				sprintf(text, "%3d", idle);
			} else if (idle >= 0) {
				sprintf(text, "%2d%%", idle);
			} else {
				strcpy(text, "--");
			}
		} else if (led == LED_SND) {
			int snd = abs(gui_data.sndbuf + 5) / 10;
			if (snd > 99)
				snd = 99;
			on = gui_data.sndbuf_status;
			if (on < 3) {
				sprintf(text, "%2d", snd);
			}
			on_rgb = 0x111111;
			if (on < 0)
				on_rgb = 0xcccc00; // underflow
			else if (on == 2)
				on_rgb = 0xcc0000; // really big overflow
			else if (on == 1)
				on_rgb = 0x0000cc; // "normal" overflow
			off_rgb = 0x111111;
		} else if (led == LED_MD) {
			on = gui_data.md;
			on_rgb = on == 2 ? 0xcc0000 : 0x00cc00;
			off_rgb = 0x003300;
			strcpy(text, "NV");
		} else if (led == LED_NET) {
			on = gui_data.net;
			on_rgb = 0;
			if (on & 1)
				on_rgb |= 0x00cc00;
			if (on & 2)
				on_rgb |= 0xcc0000;
			off_rgb = 0x111111;
			strcpy(text, "N");
		} else if (led == LED_LINES) {
			if (gui_data.fps_color < 2) {
				int lines = gui_data.lines;
				if (lines > 999) {
					if (lines > 9999) {
						lines = 9999;
					}
					sprintf(text, "%dK", lines / 1000);
				} else if (lines > 0) {
					sprintf(text, "%3d", lines);
				} else {
					strcpy(text, "--");
				}
			} else {
				strcpy(text, "--");
			}
			on_rgb = 0x111111;
			off_rgb = 0x111111;
#ifdef AMIBERRY // Board Temperature, if available
		} else if (led == LED_TEMP) {
			int temp = gui_data.temperature;
			if (temp > 999)
				temp = 999;
			else if (temp < -99)
				temp = -99;
			on = 1;
			off_rgb = 0x000000;
			int range = 0xf0 / (100 - 20);
			int v = range * abs(temp - 20);
			on_rgb = v << 16;
			sprintf(text, "%02d", temp);
#endif
		} else {
			continue;
		}
		on_rgb |= 0x33111111;
		off_rgb |= 0x33111111;
		if (half > 0) {
			int halfon = y >= TD_TOTAL_HEIGHT / 2;
			c = ledcolor(on ? (halfon ? on_rgb2 : on_rgb) : off_rgb, rc, gc, bc, alpha);
			if (!halfon && on && extraborder)
				cb = rgbmuldiv(cb, 2, 3);
		} else if (half < 0) {
			int halfon = y < TD_TOTAL_HEIGHT / 2;
			c = ledcolor(on ? (halfon ? on_rgb2 : on_rgb) : off_rgb, rc, gc, bc, alpha);
			if (!halfon && on && extraborder)
				cb = rgbmuldiv(cb, 2, 3);
		} else {
			c = ledcolor(on ? on_rgb : off_rgb, rc, gc, bc, alpha);
		}

		border = 0;
		if (y == 0 || y == TD_TOTAL_HEIGHT - 1) {
			c = cb;
			border = 1;
		}

		x = x_start + pos * td_width * mult;
		for (int xx = 0; xx < mult; xx++) {
			if (!border) {
				putpixel(buf, NULL, x - mult + xx, cb);
			}
			for (j = 0; j < td_led_width * mult; j += mult) {
				putpixel(buf, NULL, x + j + xx, c);
			}
			if (!border) {
				putpixel(buf, NULL, x + j + xx, cb);
			}
		}

		if (y >= td_numbers_pady && y - td_numbers_pady < td_numbers_height) {
			int xd = (TD_MAX_CHARS - strlen(text)) * td_numbers_width * mult / 2 + TD_DEFAULT_PADX;
			for (int i = 0; i < strlen(text); i++) {
				char ch = text[i];
				for (int j = 0; j < strlen(numbers_mapping); j++) {
					if (ch == numbers_mapping[j]) {
						write_tdnumber(buf, x + xd, y - td_numbers_pady, j, pen_rgb, c2, mult);
						break;
					}
				}
				x += td_numbers_width * mult;
			}
		}
	}
}

#define MAX_STATUSLINE_QUEUE 8
struct statusline_struct
{
	TCHAR *text;
	int type;
};
struct statusline_struct statusline_data[MAX_STATUSLINE_QUEUE];
static TCHAR *statusline_text_active;
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
	statusline_updated(0);
}

void statusline_clear(void)
{
	statusline_text_active = NULL;
	statusline_delay = 0;
	for (auto& i : statusline_data)
	{
		xfree(i.text);
		i.text = NULL;
	}
	statusline_update_notification();
}

const TCHAR *statusline_fetch(void)
{
	return statusline_text_active;
}

void statusline_add_message(int statustype, const TCHAR *format, ...)
{
	va_list parms;
	TCHAR buffer[256];

	if (isguiactive()) {
		switch (statustype)
		{
			case STATUSTYPE_DISPLAY:
			break;
			default:
			return;
		}
	}

	va_start(parms, format);
	buffer[0] = ' ';
	_vsntprintf(buffer + 1, 256 - 2, format, parms);
	_tcscat(buffer, _T(" "));

	for (int i = 0; i < MAX_STATUSLINE_QUEUE; i++) {
		if (statusline_data[i].text != NULL && statusline_data[i].type == statustype) {
			xfree(statusline_data[i].text);
			statusline_data[i].text = NULL;
			for (int j = i + 1; j < MAX_STATUSLINE_QUEUE; j++) {
				memcpy(&statusline_data[j - 1], &statusline_data[j], sizeof(struct statusline_struct));
			}
			statusline_data[MAX_STATUSLINE_QUEUE - 1].text = NULL;
		}
	}

	if (statusline_data[1].text) {
		for (int i = 0; i < MAX_STATUSLINE_QUEUE; i++) {
			if (statusline_data[i].text && !_tcscmp(statusline_data[i].text, buffer)) {
				xfree(statusline_data[i].text);
				for (int j = i + 1; j < MAX_STATUSLINE_QUEUE; j++) {
					memcpy(&statusline_data[j - 1], &statusline_data[j], sizeof(struct statusline_struct));
				}
				statusline_data[MAX_STATUSLINE_QUEUE - 1].text = NULL;
				i = 0;
			}
		}
	} else if (statusline_data[0].text) {
		if (!_tcscmp(statusline_data[0].text, buffer))
			return;
	}

	for (int i = 0; i < MAX_STATUSLINE_QUEUE; i++) {
		if (statusline_data[i].text == NULL) {
			statusline_data[i].text = my_strdup(buffer);
			statusline_data[i].type = statustype;
			if (i == 0)
				statusline_delay = (int)(STATUSLINE_MS * vblank_hz / 1000.0f);
			statusline_text_active = statusline_data[0].text;
			statusline_update_notification();
			return;
		}
	}
	statusline_text_active = NULL;
	xfree(statusline_data[0].text);
	for (int i = 1; i < MAX_STATUSLINE_QUEUE; i++) {
		memcpy(&statusline_data[i - 1], &statusline_data[i], sizeof(struct statusline_struct));
	}
	statusline_data[MAX_STATUSLINE_QUEUE - 1].text = my_strdup(buffer);
	statusline_data[MAX_STATUSLINE_QUEUE - 1].type = statustype;
	statusline_text_active = statusline_data[0].text;
	statusline_update_notification();

	va_end(parms);
}

void statusline_vsync(void)
{
	if (!statusline_data[0].text)
		return;
	if (statusline_delay == 0)
		statusline_delay = (int)(STATUSLINE_MS * vblank_hz / (1000.0f * 1.0f));
	if (statusline_delay > STATUSLINE_MS * vblank_hz / (1000.0f * 1.0f))
		statusline_delay = (int)(STATUSLINE_MS * vblank_hz / (1000.0f * 1.0f));
	if (statusline_delay > STATUSLINE_MS * vblank_hz / (1000.0f * 3.0f) && statusline_data[1].text)
		statusline_delay = (int)(STATUSLINE_MS * vblank_hz / (1000.0f * 3.0f));
	statusline_delay--;
	if (statusline_delay)
		return;
	statusline_text_active = NULL;
	xfree(statusline_data[0].text);
	for (int i = 1; i < MAX_STATUSLINE_QUEUE; i++) {
		statusline_data[i - 1].text = statusline_data[i].text;
	}
	statusline_data[MAX_STATUSLINE_QUEUE - 1].text = NULL;
	statusline_text_active = statusline_data[0].text;
	statusline_update_notification();
}

void statusline_single_erase(int monid, uae_u8 *buf, int y, int totalwidth)
{
	memset(buf, 0, 4 * totalwidth);
}

#ifdef AMIBERRY
void statusline_updated(int monid)
{

}

// Simple 5x7 font for LDP text rendering (ASCII 32-95)
// Each character is 5 pixels wide, 7 pixels tall
static const uae_u8 ldp_font[64][7] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // ' ' (32)
	{ 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00 }, // '!'
	{ 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '"'
	{ 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x00, 0x00 }, // '#'
	{ 0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04 }, // '$'
	{ 0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03 }, // '%'
	{ 0x08, 0x14, 0x08, 0x15, 0x12, 0x0D, 0x00 }, // '&'
	{ 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 }, // '''
	{ 0x02, 0x04, 0x04, 0x04, 0x04, 0x02, 0x00 }, // '('
	{ 0x08, 0x04, 0x04, 0x04, 0x04, 0x08, 0x00 }, // ')'
	{ 0x00, 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00 }, // '*'
	{ 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 }, // '+'
	{ 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08 }, // ','
	{ 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }, // '-'
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 }, // '.'
	{ 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 }, // '/'
	{ 0x0E, 0x11, 0x13, 0x15, 0x19, 0x0E, 0x00 }, // '0' (48)
	{ 0x04, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00 }, // '1'
	{ 0x0E, 0x11, 0x01, 0x06, 0x08, 0x1F, 0x00 }, // '2'
	{ 0x0E, 0x11, 0x02, 0x01, 0x11, 0x0E, 0x00 }, // '3'
	{ 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x00 }, // '4'
	{ 0x1F, 0x10, 0x1E, 0x01, 0x11, 0x0E, 0x00 }, // '5'
	{ 0x06, 0x08, 0x1E, 0x11, 0x11, 0x0E, 0x00 }, // '6'
	{ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x00 }, // '7'
	{ 0x0E, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00 }, // '8'
	{ 0x0E, 0x11, 0x0F, 0x01, 0x02, 0x0C, 0x00 }, // '9'
	{ 0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00 }, // ':'
	{ 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08 }, // ';'
	{ 0x02, 0x04, 0x08, 0x08, 0x04, 0x02, 0x00 }, // '<'
	{ 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00 }, // '='
	{ 0x08, 0x04, 0x02, 0x02, 0x04, 0x08, 0x00 }, // '>'
	{ 0x0E, 0x11, 0x02, 0x04, 0x00, 0x04, 0x00 }, // '?'
	{ 0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E }, // '@' (64)
	{ 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00 }, // 'A'
	{ 0x1E, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00 }, // 'B'
	{ 0x0E, 0x11, 0x10, 0x10, 0x11, 0x0E, 0x00 }, // 'C'
	{ 0x1E, 0x11, 0x11, 0x11, 0x11, 0x1E, 0x00 }, // 'D'
	{ 0x1F, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00 }, // 'E'
	{ 0x1F, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x00 }, // 'F'
	{ 0x0E, 0x11, 0x10, 0x13, 0x11, 0x0F, 0x00 }, // 'G'
	{ 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00 }, // 'H'
	{ 0x0E, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00 }, // 'I'
	{ 0x07, 0x02, 0x02, 0x02, 0x12, 0x0C, 0x00 }, // 'J'
	{ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, // 'K'
	{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00 }, // 'L'
	{ 0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x00 }, // 'M'
	{ 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00 }, // 'N'
	{ 0x0E, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00 }, // 'O'
	{ 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x00 }, // 'P' (80)
	{ 0x0E, 0x11, 0x11, 0x15, 0x12, 0x0D, 0x00 }, // 'Q'
	{ 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }, // 'R'
	{ 0x0E, 0x11, 0x10, 0x0E, 0x01, 0x1E, 0x00 }, // 'S'
	{ 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00 }, // 'T'
	{ 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00 }, // 'U'
	{ 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00 }, // 'V'
	{ 0x11, 0x11, 0x11, 0x15, 0x1B, 0x11, 0x00 }, // 'W'
	{ 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }, // 'X'
	{ 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x00 }, // 'Y'
	{ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x1F, 0x00 }, // 'Z'
	{ 0x0E, 0x08, 0x08, 0x08, 0x08, 0x0E, 0x00 }, // '['
	{ 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 }, // '\'
	{ 0x0E, 0x02, 0x02, 0x02, 0x02, 0x0E, 0x00 }, // ']'
	{ 0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00 }, // '^'
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00 }, // '_' (95)
};

#define LDP_FONT_COLOR 0xFFFFFFFF

void ldp_render(const char* txt, int len, uae_u8* buf, struct vidbuffer* dst, int x, int y, int mx, int my)
{
	if (!buf || !dst || !txt || len <= 0)
		return;

	for (int i = 0; i < len; i++) {
		unsigned char c = static_cast<unsigned char>(txt[i]);
		// Map character to font index (ASCII 32-95)
		if (c < 32 || c > 95) {
			// For characters outside our range, use space
			c = 32;
		}
		int font_idx = c - 32;

		// Render each row of the character
		for (int row = 0; row < LDP_CHAR_HEIGHT; row++) {
			uae_u8 rowdata = ldp_font[font_idx][row];

			// Render each pixel column of the character (with multiplier)
			for (int yy = 0; yy < my; yy++) {
				int dest_y = y + row * my + yy;
				if (dest_y < 0 || dest_y >= dst->inheight)
					continue;

				uae_u8* rowbuf = buf + dest_y * dst->rowbytes;

				for (int col = 0; col < 5; col++) {
					if (rowdata & (0x10 >> col)) {
						// Pixel is set, draw it
						for (int xx = 0; xx < mx; xx++) {
							int dest_x = x + (i * LDP_CHAR_WIDTH + col) * mx + xx;
							if (dest_x >= 0 && dest_x < dst->inwidth) {
								uae_u32* p = reinterpret_cast<uae_u32*>(rowbuf) + dest_x;
								*p = LDP_FONT_COLOR;
							}
						}
					}
				}
			}
		}
	}
}

// NOTE: This implementation renders directly in RGBA32 pixel format.
// The rc/gc/bc/alpha color table arguments are not used; this function
// is only called from update_leds() which uses an RGBA32 surface.
void statusline_render(int monid, uae_u8 *buf, int pitch, int width, int height, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha)
{
	const TCHAR *text = statusline_fetch();
	if (!text || !buf || width <= 0 || height <= 0)
		return;

	int len = _tcslen(text);
	if (len <= 0)
		return;

	int m = statusline_get_multiplier(monid) / 100;
	if (m < 2) m = 2; // minimum 2x for readability

	const int char_w = LDP_CHAR_WIDTH * m;
	const int char_h = LDP_CHAR_HEIGHT * m;
	const int padding = 2 * m;

	// Center text horizontally
	int text_width = len * char_w;
	int x = (width - text_width) / 2;
	if (x < padding) x = padding;
	int y = padding;

	// Draw background box behind text
	int box_x0 = x - padding;
	int box_y0 = y - padding;
	int box_x1 = x + text_width + padding;
	int box_y1 = y + char_h + padding;
	if (box_x0 < 0) box_x0 = 0;
	if (box_y0 < 0) box_y0 = 0;
	if (box_x1 > width) box_x1 = width;
	if (box_y1 > height) box_y1 = height;

	for (int row = box_y0; row < box_y1; row++) {
		uae_u32 *row_ptr = reinterpret_cast<uae_u32*>(buf + row * pitch);
		for (int col = box_x0; col < box_x1; col++) {
			row_ptr[col] = 0xC0000000; // semi-transparent black
		}
	}

	// Draw text using ldp_font (ASCII 32-95 only, convert lowercase to upper)
	for (int i = 0; i < len; i++) {
		unsigned char c = static_cast<unsigned char>(text[i]);
		if (c >= 'a' && c <= 'z')
			c = c - 'a' + 'A';
		if (c < 32 || c > 95)
			c = 32;
		int font_idx = c - 32;

		for (int row = 0; row < LDP_CHAR_HEIGHT; row++) {
			uae_u8 rowdata = ldp_font[font_idx][row];
			for (int yy = 0; yy < m; yy++) {
				int dest_y = y + row * m + yy;
				if (dest_y < 0 || dest_y >= height)
					continue;
				uae_u32 *row_ptr = reinterpret_cast<uae_u32*>(buf + dest_y * pitch);
				for (int col = 0; col < 5; col++) {
					if (rowdata & (0x10 >> col)) {
						for (int xx = 0; xx < m; xx++) {
							int dest_x = x + (i * LDP_CHAR_WIDTH + col) * m + xx;
							if (dest_x >= 0 && dest_x < width) {
								row_ptr[dest_x] = 0xFFFFFFFF; // white
							}
						}
					}
				}
			}
		}
	}
}
#endif
