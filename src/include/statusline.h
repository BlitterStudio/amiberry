#ifndef UAE_STATUSLINE_H
#define UAE_STATUSLINE_H

#include "uae/types.h"

#define TD_DEFAULT_PADX 4
#define TD_DEFAULT_PADY 2
#define TD_DEFAULT_WIDTH 30
#define TD_DEFAULT_LED_WIDTH 24
#define TD_DEFAULT_LED_HEIGHT 4

#define TD_RIGHT 1
#define TD_BOTTOM 2

extern int td_numbers_pos;
extern int td_numbers_width, td_numbers_height;
extern int td_numbers_padx, td_numbers_pady;
extern const TCHAR *td_characters;
extern int td_led_width;
extern int td_width;

#define TD_DEFAULT_NUM_WIDTH 7
#define TD_DEFAULT_NUM_HEIGHT 7

#define TD_TOTAL_HEIGHT (td_numbers_pady * 2 + td_numbers_height)

#define NUMBERS_NUM 20

#define TD_BORDER 0x333333

#define STATUSLINE_CHIPSET 1
#define STATUSLINE_RTG 2
#define STATUSLINE_TARGET 0x80

void draw_status_line_single(int monid, uae_u8 *buf, int bpp, int y, int totalwidth, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha);
void statusline_single_erase(int monid, uae_u8 *buf, int bpp, int y, int totalwidth);
void statusline_getpos(int monid, int *x, int *y, int width, int height);

#define STATUSTYPE_FLOPPY 1
#define STATUSTYPE_DISPLAY 2
#define STATUSTYPE_INPUT 3
#define STATUSTYPE_CD 4
#define STATUSTYPE_OTHER 5

bool createstatusline(int);
void deletestatusline(int);
void statusline_render(int, uae_u8 *buf, int bpp, int pitch, int width, int height, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha);
void statusline_add_message(int statustype, const TCHAR *format, ...);
void statusline_clear(void);
void statusline_vsync(void);
void statusline_updated(int);
bool has_statusline_updated(void);
const TCHAR *statusline_fetch(void);
int statusline_set_multiplier(int, int, int);
int statusline_get_multiplier(int monid);
void statusline_set_font(const char *newnumbers, int width, int height);

#endif /* UAE_STATUSLINE_H */
