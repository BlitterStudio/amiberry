/*
* UAE - The Un*x Amiga Emulator
*
* INMOS G300/G364 (Visiona, Rainbow III, EGS 110/24) emulation by Toni Wilen 2024
*
* Ported from WinUAE's PCem based version to the 86Box SVGA core.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
/* The device declarations, first: 86Box's video.h has a video_blit_memtoscreen() macro */
#include "86box_video.h"
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/io.h>
#include <86box/mem.h>
#include <86box/pci.h>
#include <86box/timer.h>
#include <86box/video.h>
#include <86box/vid_svga.h>
#include <86box/vid_svga_render.h>

enum
{
	INMOS_TYPE_G300 = 3,
	INMOS_TYPE_G360,
	INMOS_TYPE_G364
};

typedef struct inmos_t
{
	mem_mapping_t linear_mapping;
	mem_mapping_t mmio_mapping;

	svga_t svga;

	int chip;

	uint32_t linear_base, linear_size;
	uint32_t mmio_base, mmio_size;
	uint32_t mmio_mask;

	uint32_t vram_mask;

	uint32_t regs[0x400];
	uint32_t hwcursor_col[4];
	int addressalign;
	int syncdelay;

	int vblank_irq;
	uint8_t irq_state;

} inmos_t;

static void inmos_update_irqs(inmos_t *inmos)
{
	if (inmos->vblank_irq) {
		inmos->vblank_irq = 0;
		pci_set_irq(0, PCI_INTA, &inmos->irq_state);
		pci_clear_irq(0, PCI_INTA, &inmos->irq_state);
	}
}

static void inmos_vblank_start(svga_t *svga)
{
	inmos_t *inmos = (inmos_t *)svga->priv;
	uint32_t control = inmos->regs[0x160];
	if (control & 1) {
		inmos->vblank_irq = 1;
		inmos_update_irqs(inmos);
	}
}

static void inmos_hwcursor_draw(svga_t *svga, int displine)
{
	inmos_t *inmos = (inmos_t *)svga->priv;
	uint32_t *p = svga->monitor->target_buffer->line[displine];
	int width = svga->monitor->target_buffer->w;
	int addr = svga->hwcursor_latch.addr;
	int offset = svga->hwcursor_latch.x;
	int line_offset = 0;

	if (svga->interlace && svga->hwcursor_oddeven) {
		svga->hwcursor_latch.addr += line_offset;
	}

	for (int x = 0; x < 8; x++) {
		uint16_t dat = inmos->regs[0x200 + ((addr + x) & 0x1ff)];
		for (int xx = 0; xx < 8; xx++) {
			if (offset >= 0 && svga->x_add + offset < width) {
				int cidx = ((dat & 0x0002) ? 2 : 0) | ((dat & 0x0001) ? 1 : 0);
				if (cidx > 0) {
					p[svga->x_add + offset] = inmos->hwcursor_col[cidx];
				}
			}
			offset++;
			dat >>= 2;
		}
	}
	svga->hwcursor_latch.addr += 8;
	if (svga->interlace && !svga->hwcursor_oddeven) {
		svga->hwcursor_latch.addr += line_offset;
	}
}

static void inmos_recalctimings(svga_t *svga)
{
	inmos_t *inmos = (inmos_t *)svga->priv;

	if (inmos->chip == INMOS_TYPE_G300) {

		uint32_t control = inmos->regs[0x160];

		svga->bpp = 1 << ((control >> 17) & 3);
		int bs = 0;
		inmos->syncdelay = 0;
		if ((control >> 8) & 1) {
			svga->bpp = 32;
			bs = 2;
			svga->rowoffset <<= 2;
			inmos->syncdelay = 6;
		}

		int meminit = inmos->regs[0x12b];
		int tdelay = inmos->regs[0x12c];
		int len = (meminit + tdelay) * 4;

		svga->htotal = inmos->regs[0x123] * 4;
		svga->rowoffset = 2048 + (svga->htotal << bs) - (len << bs);

		// Top of screen counts 512 byte VRAM rows
		svga->memaddr_latch = inmos->regs[0x180] * (512 / 4);

		svga->rowoffset >>= 3;

		svga->vtotal = (inmos->regs[0x128] + inmos->regs[0x0127] * 3 + inmos->regs[0x12a]) / 2;
		svga->dispend = inmos->regs[0x128] / 2;
		svga->split = 99999;
		svga->vblankstart = svga->vtotal;
		svga->vsyncstart = svga->vtotal;
		svga->interlace = (control >> 1) & 1;

		if (!(control & 1)) {
			svga->bpp = 0;
		}

	} else if (inmos->chip >= INMOS_TYPE_G360) {

		uint32_t control = inmos->regs[0x060];
		int meminit = inmos->regs[0x02d];
		int tdelay = inmos->regs[0x02e];
		int len = meminit + tdelay;

		int bv = (control >> 20) & 7;
		int bpp = 0;
		int bs = 0;
		switch (bv)
		{
			default:
				bpp = 0;
				break;
			case 0:
				bpp = 1;
				break;
			case 1:
				bpp = 2;
				break;
			case 2:
				bpp = 4;
				break;
			case 3:
				bpp = 8;
				break;
			case 4:
				bpp = 15;
				bs = 1;
				break;
			case 5:
				bpp = 16;
				bs = 1;
				break;
			case 6:
				bpp = 32;
				bs = 2;
				break;
		}

		svga->htotal = inmos->regs[0x023] * 4;
		svga->rowoffset = 2048 + (svga->htotal << bs) - (len << bs);

		// Top of screen and line start count 2048 byte VRAM rows
		uint32_t start = inmos->regs[0x080] + inmos->regs[0x02c];

		svga->memaddr_latch = start * (2048 / 4);
		svga->rowoffset >>= 3;
		inmos->syncdelay = ((control >> 15) & 7) * ((bpp + 7) >> 3);
		svga->vtotal = (inmos->regs[0x02a] + inmos->regs[0x029] * 3 + inmos->regs[0x26]) / 2;
		svga->dispend = inmos->regs[0x02a] / 2;
		svga->split = 99999;
		svga->vblankstart = svga->vtotal;
		svga->vsyncstart = svga->vtotal;
		svga->interlace = (control >> 1) & 1;

		svga->bpp = bpp;

		svga->hwcursor.ena = (control >> 23) == 0;

		if (!(control & 1)) {
			svga->bpp = 0;
		}

	}

	svga->crtc[0x17] |= 0x80;
	svga->gdcreg[6] |= 1;
	svga->hdisp = svga->htotal;

	svga->fullchange = svga->monitor->mon_changeframecount;

	svga->render = svga_render_blank;
	switch (svga->bpp)
	{
		// Also supports 1 and 2 bpp modes
		case 4:
			svga->render = svga_render_8bpp_highres;
			break;
		case 8:
			svga->render = svga_render_8bpp_highres;
			break;
		case 15:
			svga->render = svga_render_15bpp_highres;
			break;
		case 16:
			svga->render = svga_render_16bpp_highres;
			break;
		case 32:
			svga->render = svga_render_ABGR8888_highres;
			/* The pipeline delay skips pixels at the start of the line.
			   memaddr_latch counts 4 byte units, syncdelay * 4 bytes. */
			svga->memaddr_latch += inmos->syncdelay;
			break;
	}
}

static uint8_t mirrorbits(uint8_t v)
{
	uint8_t vout = 0;
	for (int i = 0; i < 8; i++) {
		vout |= (v & (1 << i)) ? (1 << (7 - i)) : 0;
	}
	return vout;
}

static void load_palette(int idx, uint32_t val, svga_t *svga)
{
	uint8_t r = val & 0xff;
	uint8_t g = (val >> 8) & 0xff;
	uint8_t b = (val >> 16) & 0xff;
	uint8_t vidx = mirrorbits(idx);
	// Chip reads VRAM "backwards": mirror palette index bits.
	svga->pallook[vidx] = makecol32(r, g, b);
}

static void inmos_mmio_write(uint32_t addr, uint8_t val, void *p)
{
}

static void inmos_mmio_write_w(uint32_t addr, uint16_t val, void *p)
{
}

static void inmos_mmio_write_l(uint32_t addr, uint32_t val, void *p)
{
	inmos_t *inmos = (inmos_t *)p;
	svga_t *svga = &inmos->svga;

	int reg = (addr >> inmos->addressalign) & inmos->mmio_mask;
	inmos->regs[reg] = val;

	if (inmos->chip == INMOS_TYPE_G300) {

		if (reg < 0x100) {
			load_palette(reg, val, svga);
		} else {
			if (reg == 0x12a) {
				inmos->regs[0x180] = inmos->regs[0x12a];
			}
			svga_recalctimings(&inmos->svga);
		}

	} else if (inmos->chip >= INMOS_TYPE_G360) {

		if (reg >= 0x100 && reg < 0x200) {
			load_palette(reg - 0x100, val, svga);
		} else if (reg >= 0xa0 && reg < 0xa4) {
			uint8_t r = val & 0xff;
			uint8_t g = (val >> 8) & 0xff;
			uint8_t b = (val >> 16) & 0xff;
			inmos->hwcursor_col[reg - 0xa0] = makecol32(r, g, b);
		} else if (reg == 0x0c7) {
			int v = (val >> 12) & 0xfff;
			inmos->svga.hwcursor.x = v;
			if (v & 0x800) {
				inmos->svga.hwcursor.x = (v & 0x7ff) - 0x800;
			}
			v = (val >> 0) & 0xfff;
			inmos->svga.hwcursor.y = v;
			if (v & 0x800) {
				inmos->svga.hwcursor.y = (v & 0x7ff) - 0x800;
			}
		} else if (reg < 0x100) {
			svga_recalctimings(&inmos->svga);
		}
		if (reg == 0x000) {
			inmos->addressalign = (val & (1 << 6)) ? 3 : 2;
		}
	}
}

static uint8_t inmos_mmio_read(uint32_t addr, void *p)
{
	return 0;
}

static uint16_t inmos_mmio_readw(uint32_t addr, void *p)
{
	return 0;
}

static uint32_t inmos_mmio_readl(uint32_t addr, void *p)
{
	inmos_t *inmos = (inmos_t *)p;

	int reg = (addr >> inmos->addressalign) & inmos->mmio_mask;
	return inmos->regs[reg];
}

static uint8_t inmos_read_linear(uint32_t addr, void *p)
{
	svga_t *svga = (svga_t *)p;
	return *(uint8_t *)&svga->vram[addr & svga->vram_mask];
}

static uint16_t inmos_readw_linear(uint32_t addr, void *p)
{
	svga_t *svga = (svga_t *)p;
	return *(uint16_t *)&svga->vram[addr & svga->vram_mask];
}

static uint32_t inmos_readl_linear(uint32_t addr, void *p)
{
	svga_t *svga = (svga_t *)p;
	return *(uint32_t *)&svga->vram[addr & svga->vram_mask];
}

static void inmos_write_linear(uint32_t addr, uint8_t val, void *p)
{
	svga_t *svga = (svga_t *)p;
	addr &= svga->vram_mask;
	*(uint8_t *)&svga->vram[addr] = val;
	svga->changedvram[addr >> 12] = svga->monitor->mon_changeframecount;
}

static void inmos_writew_linear(uint32_t addr, uint16_t val, void *p)
{
	svga_t *svga = (svga_t *)p;
	addr &= svga->vram_mask;
	*(uint16_t *)&svga->vram[addr] = val;
	svga->changedvram[addr >> 12] = svga->monitor->mon_changeframecount;
}

static void inmos_writel_linear(uint32_t addr, uint32_t val, void *p)
{
	svga_t *svga = (svga_t *)p;
	addr &= svga->vram_mask;
	*(uint32_t *)&svga->vram[addr] = val;
	svga->changedvram[addr >> 12] = svga->monitor->mon_changeframecount;
}

static void inmos_updatemapping(inmos_t *inmos)
{
	mem_mapping_disable(&inmos->mmio_mapping);
	mem_mapping_disable(&inmos->linear_mapping);

	if (inmos->chip == INMOS_TYPE_G300) {
		inmos->linear_base = 0;
		mem_mapping_set_addr(&inmos->linear_mapping, inmos->linear_base, inmos->linear_size);
		inmos->mmio_base = 0x800000;
		inmos->mmio_size = 0x4000;
		mem_mapping_set_addr(&inmos->mmio_mapping, inmos->mmio_base, inmos->mmio_size);
	} else if (inmos->chip >= INMOS_TYPE_G360) {
		inmos->linear_base = 0;
		mem_mapping_set_addr(&inmos->linear_mapping, inmos->linear_base, inmos->linear_size);
		inmos->mmio_base = 0x1000000;
		inmos->mmio_size = 0x4000;
		mem_mapping_set_addr(&inmos->mmio_mapping, inmos->mmio_base, inmos->mmio_size);
	}

	mem_mapping_enable(&inmos->mmio_mapping);
	mem_mapping_enable(&inmos->linear_mapping);
}

static inmos_t *inmos_init(const device_t *info, int chip)
{
	inmos_t *inmos = (inmos_t *)calloc(1, sizeof(inmos_t));
	svga_t *svga = &inmos->svga;
	int vram;
	uint32_t vram_size;

	vram = device_get_config_int("memory");
	if (vram) {
		vram_size = vram << 20;
	} else {
		vram_size = 512 << 10;
	}
	inmos->vram_mask = vram_size - 1;

	svga_init(info, &inmos->svga, inmos, vram_size,
		inmos_recalctimings,
		NULL, NULL,
		inmos_hwcursor_draw,
		NULL);

	inmos->linear_size = 0x800000;
	mem_mapping_add(&inmos->linear_mapping, 0, 0, inmos_read_linear, inmos_readw_linear, inmos_readl_linear, inmos_write_linear, inmos_writew_linear, inmos_writel_linear, NULL, MEM_MAPPING_EXTERNAL, &inmos->svga);
	mem_mapping_add(&inmos->mmio_mapping, 0x800000, 0x4000, inmos_mmio_read, inmos_mmio_readw, inmos_mmio_readl, inmos_mmio_write, inmos_mmio_write_w, inmos_mmio_write_l, NULL, MEM_MAPPING_EXTERNAL, inmos);

	svga->decode_mask = (4 << 20) - 1;
	switch (vram)
	{
		case 0: /*512kb*/
			svga->vram_mask = (1 << 19) - 1;
			svga->vram_max = 4 << 20;
			break;
		case 1: /*1MB*/
			svga->vram_mask = (1 << 20) - 1;
			svga->vram_max = 4 << 20;
			break;
		case 2:
		default: /*2MB*/
			svga->vram_mask = (2 << 20) - 1;
			svga->vram_max = 4 << 20;
			break;
		case 4: /*4MB*/
			svga->vram_mask = (4 << 20) - 1;
			svga->vram_max = 4 << 20;
			break;
	}

	svga->vsync_callback = inmos_vblank_start;
	/* Not a VGA: the display timing comes from inmos_recalctimings()
	   alone, not from the (unused) VGA CRTC blanking registers. */
	svga->hoverride = 1;
	svga->dac_mask = 0xff;

	inmos->chip = chip;
	inmos->addressalign = 2;
	inmos->svga.hwcursor.cur_xsize = 64;
	inmos->svga.hwcursor.cur_ysize = 64;

	inmos_updatemapping(inmos);

	return inmos;
}

static void *inmos_rainbow3_z3_init(const device_t *info)
{
	inmos_t *inmos = inmos_init(info, INMOS_TYPE_G360);

	inmos->svga.fb_only = -1;
	inmos->mmio_mask = 0x3ff;
	svga_set_ramdac_type(&inmos->svga, RAMDAC_8BIT);

	return inmos;
}

static void *inmos_visiona_z2_init(const device_t *info)
{
	inmos_t *inmos = inmos_init(info, INMOS_TYPE_G300);

	inmos->svga.fb_only = -1;
	inmos->mmio_mask = 0x1ff;
	svga_set_ramdac_type(&inmos->svga, RAMDAC_8BIT);

	return inmos;
}

static void *inmos_egs_110_24_init(const device_t *info)
{
	inmos_t *inmos = inmos_init(info, INMOS_TYPE_G364);

	inmos->svga.fb_only = -1;
	inmos->mmio_mask = 0x3ff;
	svga_set_ramdac_type(&inmos->svga, RAMDAC_8BIT);

	return inmos;
}

static void inmos_close(void *p)
{
	inmos_t *inmos = (inmos_t *)p;

	svga_close(&inmos->svga);

	free(inmos);
}

static void inmos_speed_changed(void *p)
{
	inmos_t *inmos = (inmos_t *)p;

	svga_recalctimings(&inmos->svga);
}

static void inmos_force_redraw(void *p)
{
	inmos_t *inmos = (inmos_t *)p;

	inmos->svga.fullchange = inmos->svga.monitor->mon_changeframecount;
}

const device_t inmos_visiona_z2_device = {
	.name          = "Visiona",
	.internal_name = "inmos_visiona_z2",
	.flags         = 0,
	.local         = 0,
	.init          = inmos_visiona_z2_init,
	.close         = inmos_close,
	.reset         = NULL,
	.available     = NULL,
	.speed_changed = inmos_speed_changed,
	.force_redraw  = inmos_force_redraw,
	.config        = NULL
};

const device_t inmos_rainbow3_z3_device = {
	.name          = "Rainbow III",
	.internal_name = "inmos_rainbow3_z3",
	.flags         = 0,
	.local         = 0,
	.init          = inmos_rainbow3_z3_init,
	.close         = inmos_close,
	.reset         = NULL,
	.available     = NULL,
	.speed_changed = inmos_speed_changed,
	.force_redraw  = inmos_force_redraw,
	.config        = NULL
};

const device_t inmos_egs_110_24_device = {
	.name          = "EGS 110/24",
	.internal_name = "inmos_egs_110_24",
	.flags         = 0,
	.local         = 0,
	.init          = inmos_egs_110_24_init,
	.close         = inmos_close,
	.reset         = NULL,
	.available     = NULL,
	.speed_changed = inmos_speed_changed,
	.force_redraw  = inmos_force_redraw,
	.config        = NULL
};
