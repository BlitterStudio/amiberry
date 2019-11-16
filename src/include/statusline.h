#ifndef UAE_STATUSLINE_H
#define UAE_STATUSLINE_H

#include "uae/types.h"

#define TD_PADX 4
#define TD_PADY 2
#define TD_WIDTH 30
#define TD_LED_WIDTH 24
#define TD_LED_HEIGHT 4

#define TD_RIGHT 1
#define TD_BOTTOM 2

static int td_pos = (TD_RIGHT | TD_BOTTOM);

#define TD_NUM_WIDTH 7
#define TD_NUM_HEIGHT 7

#define TD_TOTAL_HEIGHT (TD_PADY * 2 + TD_NUM_HEIGHT)

#define NUMBERS_NUM 19

#define TD_BORDER 0x333333

#define STATUSLINE_CHIPSET 1
#define STATUSLINE_RTG 2
#define STATUSLINE_TARGET 0x80

extern void draw_status_line_single(uae_u8 *buf, int bpp, int y, int totalwidth, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha);
extern void statusline_single_erase(uae_u8 *buf, int bpp, int y, int totalwidth);
extern void statusline_getpos(int *x, int *y, int width, int height, int hx, int vx);
extern bool softstatusline(void);

#define STATUSTYPE_FLOPPY 1
#define STATUSTYPE_DISPLAY 2
#define STATUSTYPE_INPUT 3
#define STATUSTYPE_CD 4
#define STATUSTYPE_OTHER 5

extern bool createstatusline(int);
extern void deletestatusline(int);
extern void statusline_render(int, uae_u8 *buf, int bpp, int pitch, int width, int height, uae_u32 *rc, uae_u32 *gc, uae_u32 *bc, uae_u32 *alpha);
//extern void statusline_add_message(int statustype, const TCHAR *format, ...);
extern void statusline_clear(void);
extern void statusline_vsync(void);
extern void statusline_updated(int);
extern bool has_statusline_updated(void);
extern const TCHAR *statusline_fetch(void);

#endif /* UAE_STATUSLINE_H */
