#ifndef UAE_GFXBOARD_H
#define UAE_GFXBOARD_H

#include "picasso96.h"
#include "rtgmodes.h"

extern bool gfxboard_init_memory (struct autoconfig_info*);
extern bool gfxboard_init_memory_p4_z2(struct autoconfig_info*);
extern bool gfxboard_init_registers(struct autoconfig_info*);
extern bool gfxboard_init_registers2(struct autoconfig_info*);
extern void gfxboard_free (void);
extern void gfxboard_reset (void);
extern void gfxboard_vsync_handler (bool, bool);
extern int gfxboard_get_configtype (struct rtgboardconfig*);
extern int gfxboard_is_registers (struct rtgboardconfig*);
extern int gfxboard_get_vram_min (struct rtgboardconfig*);
extern int gfxboard_get_vram_max (struct rtgboardconfig*);
extern bool gfxboard_need_byteswap (struct rtgboardconfig*);
extern int gfxboard_get_autoconfig_size(struct rtgboardconfig*);
extern bool gfxboard_get_switcher(struct rtgboardconfig *rbc);
extern double gfxboard_get_vsync (void);
extern void gfxboard_refresh (int monid);
extern int gfxboard_toggle (int monid, int mode, int msg);
extern int gfxboard_num_boards (struct rtgboardconfig*);
extern uae_u32 gfxboard_get_romtype(struct rtgboardconfig*);
extern const TCHAR *gfxboard_get_name(int);
extern const TCHAR *gfxboard_get_manufacturername(int);
extern const TCHAR *gfxboard_get_configname(int);
extern struct gfxboard_func *gfxboard_get_func(struct rtgboardconfig *rbc);
extern int gfxboard_get_index_from_id(int);
extern int gfxboard_get_id_from_index(int);
extern bool gfxboard_switch_away(int monid);

extern bool gfxboard_allocate_slot(int, int);
extern void gfxboard_free_slot(int);
extern bool gfxboard_rtg_enable_initial(int monid, int);
extern void gfxboard_rtg_disable(int monid, int);
extern bool gfxboard_init_board(struct autoconfig_info*);
extern bool gfxboard_set(int monid, bool rtg);
extern void gfxboard_resize(int width, int height, int hmult, int vmult, void *p);

uae_u8 *gfxboard_getrtgbuffer(int monid, int *widthp, int *heightp, int *pitch, int *depth, uae_u8 *palette);
void gfxboard_freertgbuffer(int monid, uae_u8 *dst);
bool gfxboard_isgfxboardscreen(int monid);

extern struct gfxboard_func a2410_func;
extern struct gfxboard_func harlequin_func;
extern struct gfxboard_func rainbowii_func;

extern void vga_io_put(int board, int portnum, uae_u8 v);
extern uae_u8 vga_io_get(int board, int portnum);
extern void vga_ram_put(int board, int offset, uae_u8 v);
extern uae_u8 vga_ram_get(int board, int offset);
extern void vgalfb_ram_put(int board, int offset, uae_u8 v);
extern uae_u8 vgalfb_ram_get(int board, int offset);

void gfxboard_get_a8_vram(int index);
void gfxboard_free_vram(int index);

int gfxboard_get_devnum(struct uae_prefs *p, int index);

int pcem_getvramsize(void);

void gfxboard_intreq(void *p, int act, bool safe);

#define GFXBOARD_UAE_Z2 0
#define GFXBOARD_UAE_Z3 1
#define GFXBOARD_HARDWARE 2

#define GFXBOARD_ID_PICASSO2 2
#define GFXBOARD_ID_PICASSO2PLUS 3
#define GFXBOARD_ID_PICCOLO_Z2 4
#define GFXBOARD_ID_PICCOLO_Z3 5
#define GFXBOARD_ID_SD64_Z2 6
#define GFXBOARD_ID_SD64_Z3 7
#define GFXBOARD_ID_SPECTRUM_Z2 8
#define GFXBOARD_ID_SPECTRUM_Z3 9
#define GFXBOARD_ID_PICASSO4_Z2 10
#define GFXBOARD_ID_PICASSO4_Z3 11
#define GFXBOARD_ID_A2410 12
#define GFXBOARD_ID_VGA 13
#define GFXBOARD_ID_HARLEQUIN 14
#define GFXBOARD_ID_CV643D_Z2 15
#define GFXBOARD_ID_CV643D_Z3 16
#define GFXBOARD_ID_CV64_Z3 17
#define GFXBOARD_ID_VOODOO3_PCI 18
#define GFXBOARD_ID_S3VIRGE_PCI 19
#define GFXBOARD_ID_PIXEL64 20
#define GFXBOARD_ID_RETINA_Z2 21
#define GFXBOARD_ID_RETINA_Z3 22
#define GFXBOARD_ID_ALTAIS_Z3 23
#define GFXBOARD_ID_S3TRIO64_PCI 24
#define GFXBOARD_ID_PERMEDIA2_PCI 25
#define GFXBOARD_ID_RAINBOWIII 26
#define GFXBOARD_ID_VISIONA 27
#define GFXBOARD_ID_EGS_110_24 28
#define GFXBOARD_ID_DOMINO 29
#define GFXBOARD_ID_MERLIN_Z2 30
#define GFXBOARD_ID_MERLIN_Z3 31
#define GFXBOARD_ID_OMNIBUS_ET4000 32
#define GFXBOARD_ID_OMNIBUS_ET4000W32 33
#define GFXBOARD_ID_GRAFFITY_Z2 34
#define GFXBOARD_ID_GRAFFITY_Z3 35
#define GFXBOARD_ID_RAINBOWII 36
#define GFXBOARD_ID_MATROX_MILLENNIUM_PCI 37
#define GFXBOARD_ID_MATROX_MILLENNIUM_II_PCI 38
#define GFXBOARD_ID_MATROX_MYSTIQUE_PCI 39
#define GFXBOARD_ID_MATROX_MYSTIQUE220_PCI 40
#define GFXBOARD_ID_GD5446_PCI 41
#define GFXBOARD_ID_VOODOO5_PCI 42

#define GFXBOARD_BUSTYPE_Z 0
#define GFXBOARD_BUSTYPE_PCI 1
#define GFXBOARD_BUSTYPE_DRACO 2

struct gfxboard_mode
{
	int width;
	int height;
	RGBFTYPE mode;
	bool redraw_required;
	int hlinedbl, vlinedbl;
};

typedef bool(*GFXBOARD_INIT)(struct autoconfig_info*);
typedef void(*GFXBOARD_FREE)(void*);
typedef void(*GFXBOARD_RESET)(void*);
typedef void(*GFXBOARD_HSYNC)(void*);
typedef bool(*GFXBOARD_VSYNC)(void*, struct gfxboard_mode*);
typedef bool(*GFXBOARD_TOGGLE)(void*, int);
typedef void(*GFXBOARD_CONFIGURED)(void*, uae_u32);
typedef void(*GFXBOARD_REFRESH)(void*);

struct gfxboard_func
{
	GFXBOARD_INIT init;
	GFXBOARD_FREE free;
	GFXBOARD_RESET reset;
	GFXBOARD_HSYNC hsync;
	GFXBOARD_VSYNC vsync;
	GFXBOARD_REFRESH refresh;
	GFXBOARD_TOGGLE toggle;
	GFXBOARD_CONFIGURED configured;
};

#endif /* UAE_GFXBOARD_H */
