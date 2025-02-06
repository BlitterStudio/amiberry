
#define makecol(r, g, b)   ((b) | ((g) << 8) | ((r) << 16))
#define makecol32(r, g, b) ((b) | ((g) << 8) | ((r) << 16))
#define getcolr(color) (((color) >> 16) & 0xFF)
#define getcolg(color) (((color) >> 8) & 0xFF)
#define getcolb(color) ((color) & 0xFF)

typedef struct
{
        int w, h;
        uint8_t *dat;
        uint8_t *line[0];
} PCBITMAP;

extern PCBITMAP *screen;

PCBITMAP *create_bitmap(int w, int h);

typedef struct
{
        uint8_t r, g, b;
} PCRGB;
        
typedef PCRGB PALETTE[256];

#define makecol(r, g, b)    ((b) | ((g) << 8) | ((r) << 16))
#define makecol32(r, g, b)  ((b) | ((g) << 8) | ((r) << 16))

extern PCBITMAP *buffer32;

int video_card_available(int card);
char *video_card_getname(int card);
device_t *video_card_getdevice(int card, int romset);
int video_card_has_config(int card, int romset);
int video_card_getid(char *s);
int video_old_to_new(int card);
int video_new_to_old(int card);
char *video_get_internal_name(int card);
int video_get_video_from_internal_name(char *s);
int video_is_mda();
int video_is_cga();
int video_is_ega_vga();

extern int video_fullscreen, video_fullscreen_scale, video_fullscreen_first;
extern int video_force_aspect_ration;
extern int vid_disc_indicator;

enum
{
        FULLSCR_SCALE_FULL = 0,
        FULLSCR_SCALE_43,
        FULLSCR_SCALE_SQ,
        FULLSCR_SCALE_INT
};

extern int egareads,egawrites;

extern int fullchange;
extern int changeframecount;

extern uint8_t fontdat[2048][8];
extern uint8_t fontdatm[2048][16];
extern uint8_t fontdatksc5601[16384][32];
extern uint8_t fontdatksc5601_user[192][32];

extern uint32_t *video_15to32, *video_16to32;
extern uint32_t *video_6to8;

extern int xsize,ysize;

extern float cpuclock;

extern int emu_fps, frames, video_frames, video_refresh_rate;

extern int readflash;

extern void (*video_recalctimings)();

void video_blit_memtoscreen(int x, int y, int y1, int y2, int w, int h);

extern void (*video_blit_memtoscreen_func)(int x, int y, int y1, int y2, int w, int h);

extern int video_timing_read_b, video_timing_read_w, video_timing_read_l;
extern int video_timing_write_b, video_timing_write_w, video_timing_write_l;
extern int video_speed;

extern int video_res_x, video_res_y, video_bpp;

extern int vid_resize;

void video_wait_for_blit();
void video_wait_for_buffer();

typedef enum
{
	FONT_MDA,	/* MDA 8x14 */
	FONT_PC200,	/* MDA 8x14 and CGA 8x8, four fonts */
	FONT_CGA,	/* CGA 8x8, two fonts */
	FONT_WY700,	/* Wy700 16x16, two fonts */
	FONT_MDSI,	/* MDSI Genius 8x12 */
	FONT_T3100E,	/* Toshiba T3100e, four fonts */
	FONT_KSC5601,	/* Korean KSC-5601 */
	FONT_SIGMA400,	/* Sigma Color 400, 8x8 and 8x16 */
 	FONT_IM1024,	/* Image Manager 1024 */
} fontformat_t;

void loadfont(char *s, fontformat_t format);

void initvideo();
void video_init();
void closevideo();

void video_updatetiming();
void video_force_resize_set_monitor(uint8_t res, int monitor_index);

void hline(PCBITMAP *b, int x1, int y, int x2, int col);

void destroy_bitmap(PCBITMAP *b);

extern uint32_t cgapal[16];

#define DISPLAY_RGB 0
#define DISPLAY_COMPOSITE 1
#define DISPLAY_RGB_NO_BROWN 2
#define DISPLAY_GREEN 3
#define DISPLAY_AMBER 4
#define DISPLAY_WHITE 5

void cgapal_rebuild(int display_type, int contrast);

typedef struct video_timings_t {
    int type;
    int write_b;
    int write_w;
    int write_l;
    int read_b;
    int read_w;
    int read_l;
} video_timings_t;

typedef struct bitmap_t {
    int       w;
    int       h;
    uint32_t *dat;
    uint32_t *line[2112 * 2];
} bitmap_t;

typedef struct monitor_t {
    char                     name[512];
    int                      mon_xsize;
    int                      mon_ysize;
    int                      mon_scrnsz_x;
    int                      mon_scrnsz_y;
    int                      mon_efscrnsz_y;
    int                      mon_unscaled_size_x;
    int                      mon_unscaled_size_y;
    double                   mon_res_x;
    double                   mon_res_y;
    int                      mon_bpp;
    bitmap_t *target_buffer;
    int                      mon_video_timing_read_b;
    int                      mon_video_timing_read_w;
    int                      mon_video_timing_read_l;
    int                      mon_video_timing_write_b;
    int                      mon_video_timing_write_w;
    int                      mon_video_timing_write_l;
    int                      mon_overscan_x;
    int                      mon_overscan_y;
    int                      mon_force_resize;
    int                      mon_fullchange;
    int                      mon_changeframecount;
    //atomic_int               mon_screenshots;
    uint32_t *mon_pal_lookup;
    int *mon_cga_palette;
    int                      mon_pal_lookup_static;  /* Whether it should not be freed by the API. */
    int                      mon_cga_palette_static; /* Whether it should not be freed by the API. */
    const video_timings_t *mon_vid_timings;
    int                      mon_vid_type;
    //struct blit_data_struct *mon_blit_data_ptr;
} monitor_t;

typedef struct monitor_settings_t {
    int mon_window_x; /* (C) window size and position info. */
    int mon_window_y;
    int mon_window_w;
    int mon_window_h;
    int mon_window_maximized;
} monitor_settings_t;

extern int monitor_index_global;

#define MONITORS_NUM 2
extern monitor_t monitors[MONITORS_NUM];

enum {
    VIDEO_ISA = 0,
    VIDEO_MCA,
    VIDEO_BUS,
    VIDEO_PCI,
    VIDEO_AGP
};

#define VIDEO_FLAG_TYPE_CGA     0
#define VIDEO_FLAG_TYPE_MDA     1
#define VIDEO_FLAG_TYPE_SPECIAL 2
#define VIDEO_FLAG_TYPE_8514    3
#define VIDEO_FLAG_TYPE_XGA     4
#define VIDEO_FLAG_TYPE_NONE    5
#define VIDEO_FLAG_TYPE_MASK    7

void video_inform_monitor(int type, const video_timings_t *ptr, int monitor_index);
#define video_inform(type, video_timings_ptr) video_inform_monitor(type, video_timings_ptr, monitor_index_global)
