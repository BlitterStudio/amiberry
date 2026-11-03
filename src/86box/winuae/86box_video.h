/*
* UAE - The Un*x Amiga Emulator
*
* Interface between gfxboard.cpp and the 86Box video cards.
*/

#ifndef WINUAE_86BOX_VIDEO_H
#define WINUAE_86BOX_VIDEO_H

#include <stdint.h>

struct _device_;
typedef struct _device_ device_t;

/* Boards gfxboard.cpp offers, in the 86Box card sources */
extern const device_t gd5426_device;
extern const device_t gd5426_swapped_device;
extern const device_t gd5428_device;
extern const device_t gd5428_swapped_device;
extern const device_t gd5434_vlb_device;
extern const device_t gd5434_vlb_swapped_device;
extern const device_t gd5446_device;
extern const device_t s3_cybervision_trio64_device;
extern const device_t s3_trio64_device;
extern const device_t s3_virge_device;
extern const device_t permedia2_device;
extern const device_t ncr_retina_z2_device;
extern const device_t ncr_retina_z3_device;
extern const device_t et4000w32_merlin_z2_device;
extern const device_t et4000w32_merlin_z3_device;
extern const device_t et4000w32_omnibus_device;
extern const device_t et4000_domino_device;
extern const device_t et4000_omnibus_device;
extern const device_t inmos_egs_110_24_device;
extern const device_t inmos_visiona_z2_device;
extern const device_t inmos_rainbow3_z3_device;
extern const device_t voodoo_3_3000_device;
extern const device_t millennium_device;
extern const device_t millennium_ii_device;
extern const device_t mystique_device;
extern const device_t mystique_220_device;

/* Each RTG board with an 86Box chip has its own glue state. Up to
   GFXCHIP_MAX_BOARDS boards can run at the same time. */
#define GFXCHIP_MAX_BOARDS 4
struct gfxchip;

/* Set up the video glue for board 'gfxboard' (rtggfxboard) and start the
   card. index: the board's number, 0 to GFXCHIP_MAX_BOARDS - 1. swapped:
   the board shows 15/16 bit pixels with red and blue swapped. *priv gets
   the card's private data, *svga its svga_t. */
struct gfxchip *gfxchip_device_init(const device_t *dev, void *gfxboard, int index, bool swapped, void **priv, void **svga);
/* Stop the card and the devices it added, free the glue state. */
void gfxchip_device_close(struct gfxchip *chip);
void gfxchip_device_force_redraw(struct gfxchip *chip);

/* Run one scanline, returns 1 at the start of vertical sync */
int gfxchip_poll(struct gfxchip *chip);
int svga_get_vtotal(void *svga);
bool svga_on(void *svga);
void svga_setvram(void *svga, uint8_t *vram);
uint8_t gfxchip_read_io(struct gfxchip *chip, int port, int index);
/* The Voodoo 3's VRAM moved */
void voodoo_update_vram(void *banshee);

/* Timers of the card, called once per Amiga horizontal sync */
void gfxchip_hsync(struct gfxchip *chip);

/* Board memory, I/O and PCI configuration space accesses, size 0 = byte,
   1 = word, 2 = long */
void put_io_gfxchip(struct gfxchip *chip, uint32_t addr, uint32_t v, int size);
uint32_t get_io_gfxchip(struct gfxchip *chip, uint32_t addr, int size);
void put_mem_gfxchip(struct gfxchip *chip, uint32_t addr, uint32_t v, int size);
uint32_t get_mem_gfxchip(struct gfxchip *chip, uint32_t addr, int size);
void put_pci_gfxchip(struct gfxchip *chip, uint32_t addr, uint8_t v);
uint8_t get_pci_gfxchip(struct gfxchip *chip, uint32_t addr);

/* Rendered line yy of the frame that starts at x, y */
uint8_t *gfxchip_getbuffer32(struct gfxchip *chip, int x, int y, int yy);
/* Mark VRAM the Amiga wrote directly as changed */
void gfxchip_linear_mark(struct gfxchip *chip, int offset);

/* The card's linear frame buffer handlers (its first mapping of 1MB or
   more), which gfxboard.cpp calls directly for VRAM accesses. The cards'
   linear handlers don't call back into the glue, so these calls don't
   switch boards. */
struct gfxchip_linear
{
	void *priv;
	uint32_t offset;
	uint8_t (*read_b)(uint32_t addr, void *priv);
	uint16_t (*read_w)(uint32_t addr, void *priv);
	uint32_t (*read_l)(uint32_t addr, void *priv);
	void (*write_b)(uint32_t addr, uint8_t val, void *priv);
	void (*write_w)(uint32_t addr, uint16_t val, void *priv);
	void (*write_l)(uint32_t addr, uint32_t val, void *priv);
};
struct gfxchip_linear *gfxchip_get_linear(struct gfxchip *chip);

/* In gfxboard.cpp, p is the board's rtggfxboard */
void gfxboard_resize(int width, int height, int hmult, int vmult, void *p);
void gfxboard_intreq(void *p, int act, bool safe);
/* Show lines y1 to y2 - 1 of the w x h frame at x, y */
void gfxboard_blit(int x, int y, int y1, int y2, int w, int h, void *p);
uint8_t *gfxboard_getvram(void *p);
int gfxboard_getvramsize(void *p);

#endif /* WINUAE_86BOX_VIDEO_H */
