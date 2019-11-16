#ifndef UAE_GFXBOARD_H
#define UAE_GFXBOARD_H

#include "picasso96.h"

extern bool gfxboard_init_memory (struct autoconfig_info*);
extern bool gfxboard_init_memory_p4_z2(struct autoconfig_info*);
extern bool gfxboard_init_registers(struct autoconfig_info*);
extern void gfxboard_free (void);
extern void gfxboard_reset (void);
//extern void gfxboard_vsync_handler (bool, bool);
extern int gfxboard_get_configtype (struct rtgboardconfig*);
extern bool gfxboard_is_registers (struct rtgboardconfig*);
extern int gfxboard_get_vram_min (struct rtgboardconfig*);
extern int gfxboard_get_vram_max (struct rtgboardconfig*);
extern bool gfxboard_need_byteswap (struct rtgboardconfig*);
extern int gfxboard_get_autoconfig_size(struct rtgboardconfig*);
extern double gfxboard_get_vsync (void);
//extern void gfxboard_refresh ();
extern int gfxboard_toggle (int monid, int mode, int msg);
extern int gfxboard_num_boards (struct rtgboardconfig*);
extern uae_u32 gfxboard_get_romtype(struct rtgboardconfig*);
extern const TCHAR *gfxboard_get_name(int);
extern const TCHAR *gfxboard_get_manufacturername(int);
extern const TCHAR *gfxboard_get_configname(int);
extern struct gfxboard_func *gfxboard_get_func(struct rtgboardconfig *rbc);

extern bool gfxboard_allocate_slot(int, int);
extern void gfxboard_free_slot(int);
extern bool gfxboard_rtg_enable_initial(int monid, int);
extern void gfxboard_rtg_disable(int monid, int);
extern bool gfxboard_init_board(struct autoconfig_info*);
extern bool gfxboard_set(int monid, bool rtg);

extern struct gfxboard_func a2410_func;
extern struct gfxboard_func harlequin_func;

extern void vga_io_put(int board, int portnum, uae_u8 v);
extern uae_u8 vga_io_get(int board, int portnum);
extern void vga_ram_put(int board, int offset, uae_u8 v);
extern uae_u8 vga_ram_get(int board, int offset);
extern void vgalfb_ram_put(int board, int offset, uae_u8 v);
extern uae_u8 vgalfb_ram_get(int board, int offset);

void gfxboard_get_a8_vram(int index);
void gfxboard_free_vram(int index);

int gfxboard_get_devnum(struct uae_prefs *p, int index);
extern bool gfxboard_set(bool rtg);

#define GFXBOARD_UAE_Z2 0
#define GFXBOARD_UAE_Z3 1
#define GFXBOARD_HARDWARE 2

#define GFXBOARD_PICASSO2 2
#define GFXBOARD_PICASSO2PLUS 3
#define GFXBOARD_PICCOLO_Z2 4
#define GFXBOARD_PICCOLO_Z3 5
#define GFXBOARD_SD64_Z2 6
#define GFXBOARD_SD64_Z3 7
#define GFXBOARD_SPECTRUM_Z2 8
#define GFXBOARD_SPECTRUM_Z3 9
#define GFXBOARD_PICASSO4_Z2 10
#define GFXBOARD_PICASSO4_Z3 11
#define GFXBOARD_A2410 12
#define GFXBOARD_VGA 13

struct gfxboard_mode
{
	int width;
	int height;
	RGBFTYPE mode;
	bool redraw_required;
};

typedef bool(*GFXBOARD_INIT)(struct autoconfig_info*);
typedef void(*GFXBOARD_FREE)(void*);
typedef void(*GFXBOARD_RESET)(void*);
typedef void(*GFXBOARD_HSYNC)(void*);
typedef bool(*GFXBOARD_VSYNC)(void*, struct gfxboard_mode*);
typedef bool(*GFXBOARD_TOGGLE)(void*, int);
typedef void(*GFXBOARD_CONFIGURED)(void*, uae_u32);

struct gfxboard_func
{
	GFXBOARD_INIT init;
	GFXBOARD_FREE free;
	GFXBOARD_RESET reset;
	GFXBOARD_HSYNC hsync;
	GFXBOARD_VSYNC vsync;
	GFXBOARD_TOGGLE toggle;
	GFXBOARD_CONFIGURED configured;
};


#endif /* UAE_GFXBOARD_H */
