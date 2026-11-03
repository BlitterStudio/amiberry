/*
* UAE - The Un*x Amiga Emulator
*
* NCR 77C22E+ and 77C32BLT (Retina Z2 / Retina Z3) emulation by Toni Wilen 2023
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
	NCR_TYPE_22EP = 2,
	NCR_TYPE_32BLT
};

typedef struct ncr_t
{
	mem_mapping_t linear_mapping;
	mem_mapping_t mmio_mapping;

	svga_t svga;
	void *ramdac;

	uint8_t ma_ext;

	int chip;

	uint32_t linear_base, linear_size;
	uint32_t mmio_base, mmio_size;

	uint32_t bankr[4], bankw[4];
	uint32_t vram_mask;

	int vblank_irq;
	uint8_t irq_state;

	uint8_t hwcursor_pal[2];

	uint8_t blt_status, blt_rop, blt_hprot, blt_vprot;
	uint16_t blt_width, blt_height, blt_control;
	uint32_t blt_src, blt_dst, blt_pat;
	uint64_t blt_fifo_data;
	uint32_t blt_srcbak, blt_dstbak, blt_patbak, blt_patbak2;
	uint32_t blt_c0, blt_c1;
	uint16_t blt_w, blt_h;
	uint8_t blt_stage;
	int8_t blt_fifo_read, blt_fifo_write;
	uint8_t blt_srcdata, blt_patdata, blt_dstdata;
	int8_t blt_expand_offset, blt_expand_bit;
	uint8_t blt_start;
	uint8_t blt_fifo_size;
	uint8_t blt_patx, blt_patx_cnt, blt_paty_cnt;
	uint8_t blt_src_size;
	int8_t blt_bpp, blt_bppdiv8, blt_bpp_cnt, blt_bppoffset, blt_xdir, blt_ydir;
	bool blt_color_expand, blt_color_fill, blt_transparent;
	uint8_t blt_pattern_size;

} ncr_t;

static void ncr_updatemapping(ncr_t *ncr);
static void ncr_updatebanking(ncr_t *ncr);

static int ncr_vga_vsync_enabled(ncr_t *ncr)
{
	if (!(ncr->svga.crtc[0x11] & 0x20) && (ncr->svga.crtc[0x17] & 0x80) && ncr->vblank_irq > 0) {
		return 1;
	}
	return 0;
}

static void ncr_update_irqs(ncr_t *ncr)
{
	if (ncr->vblank_irq > 0 && ncr_vga_vsync_enabled(ncr)) {
		pci_set_irq(0, PCI_INTA, &ncr->irq_state);
	} else {
		pci_clear_irq(0, PCI_INTA, &ncr->irq_state);
	}
}

static void ncr_vblank_start(svga_t *svga)
{
	ncr_t *ncr = (ncr_t *)svga->priv;
	if (ncr->vblank_irq >= 0) {
		ncr->vblank_irq = 1;
		ncr_update_irqs(ncr);
	}
}

static void ncr_hwcursor_draw(svga_t *svga, int displine)
{
	ncr_t *ncr = (ncr_t *)svga->priv;
	uint32_t *p = svga->monitor->target_buffer->line[displine];
	int width = svga->monitor->target_buffer->w;
	uint8_t dat[2];
	uint32_t c[2];
	int offset = svga->hwcursor_latch.x - svga->hwcursor_latch.xoff;
	int line_offset = 0;
	uint32_t addr = svga->hwcursor_latch.addr;

	if (svga->interlace && svga->hwcursor_oddeven) {
		svga->hwcursor_latch.addr += line_offset;
	}

	if (svga->bpp <= 8) {
		c[0] = svga->pallook[ncr->hwcursor_pal[0]];
		c[1] = svga->pallook[ncr->hwcursor_pal[1]];
	} else {
		for (int i = 0; i < 2; i++) {
			uint8_t r = (ncr->hwcursor_pal[i] & (0x80 | 0x40 | 0x20));
			uint8_t b = (ncr->hwcursor_pal[i] & (0x10 | 0x08)) << 3;
			uint8_t g = (ncr->hwcursor_pal[i] & (4 | 2 | 1) << 5);
			r |= (r >> 3) | (r >> 6);
			b |= (b >> 2) | (b >> 4) | (b >> 6);
			g |= (g >> 3) | (g >> 6);
			c[i] = (r << 16) | (g << 8) | b;
			c[i] <<= 8;
		}
	}

	addr += (32 / 8);
	int xdbl = 1 << svga->hwcursor.h_acc;
	if ((svga->bpp == 16 || svga->bpp == 15) && svga->hwcursor.h_acc > 0) {
		xdbl = 1 << (svga->hwcursor.h_acc - 1);
	}
	for (int x = 0; x < svga->hwcursor.cur_xsize; x += 8) {
		if (x == 32) {
			addr += (64 / 8);
		}
		addr--;
		uint32_t addroffset = addr & svga->vram_display_mask;
		dat[0] = svga->vram[addroffset];
		addroffset = (addr + (svga->hwcursor.cur_xsize / 8)) & svga->vram_display_mask;
		dat[1] = svga->vram[addroffset];
		for (int xx = 0; xx < 8; xx++) {
			if (offset >= svga->hwcursor_latch.x) {
				for (int i = 0; i < xdbl; i++) {
					int pos = svga->x_add + offset * xdbl + i;
					if (pos < 0 || pos >= width) {
						continue;
					}
					if (!(dat[0] & 0x80)) {
						p[pos] = c[(dat[1] & 0x80) ? 0 : 1];
					} else if (dat[1] & 0x80) {
						p[pos] ^= 0xffffff;
					}
				}
			}
			offset++;
			dat[0] <<= 1;
			dat[1] <<= 1;
		}
	}
	svga->hwcursor_latch.addr += (svga->hwcursor.cur_xsize / 8) * 2;

	if (svga->interlace && !svga->hwcursor_oddeven) {
		svga->hwcursor_latch.addr += line_offset;
	}
}

static void ncr_out(uint16_t addr, uint8_t val, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;
	uint8_t old;

	if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) {
		addr ^= 0x60;
	}

	switch (addr)
	{
		case 0x3c6:
			/* Always the SDAC's command register (RS2 set) */
			val &= ~0x10;
			sdac_ramdac_out(addr, 1, val, ncr->ramdac, svga);
			return;
		case 0x3c7:
		case 0x3c8:
		case 0x3c9:
			sdac_ramdac_out(addr, 0, val, ncr->ramdac, svga);
			return;

		case 0x3c4:
			svga->seqaddr = val & 0x3f;
			break;
		case 0x3c5:
		{
			old = svga->seqregs[svga->seqaddr];
			svga->seqregs[svga->seqaddr] = val;
			if (old != val && svga->seqaddr >= 0x0a) {
				svga->fullchange = svga->monitor->mon_changeframecount;
			}
			switch (svga->seqaddr)
			{
				case 0x0a:
					ncr->hwcursor_pal[0] = val;
					break;
				case 0x0b:
					ncr->hwcursor_pal[1] = val;
					break;
				case 0x0c:
					svga->hwcursor.ena = val & 1;
					svga->hwcursor.h_acc = (val >> 5) & 3;
					svga->hwcursor.cur_ysize = 16 << ((val >> 1) & 3);
					svga->hwcursor.cur_xsize = (val & 0x80) && ncr->chip == NCR_TYPE_32BLT ? 64 : 32;
					break;
				case 0x0d:
				case 0x0e:
				case 0x0f:
				case 0x10:
					svga->hwcursor.x = (svga->seqregs[0x0d] << 8) | svga->seqregs[0x0e];
					svga->hwcursor.y = (svga->seqregs[0x0f] << 8) | svga->seqregs[0x10];
					return;
				case 0x11:
					svga->hwcursor.xoff = val & (ncr->chip == NCR_TYPE_32BLT ? 0x3f : 0x1f);
					return;
				case 0x12:
					svga->hwcursor.yoff = val & 127;
					return;
				case 0x13:
				case 0x14:
				case 0x15:
				case 0x16:
					svga->hwcursor.addr = (svga->seqregs[0x15] << 16) | (svga->seqregs[0x16] << 8) | svga->seqregs[0x14];
					svga->hwcursor.addr >>= 2;
					break;
				case 0x18:
				case 0x19:
				case 0x1a:
				case 0x1b:
				case 0x1c:
				case 0x1d:
					ncr_updatebanking(ncr);
					break;
				case 0x1e:
					ncr_updatemapping(ncr);
					break;
				case 0x21:
					if (val & 1) {
						ncr->blt_bpp = 8 << ((val >> 4) & 3);
						if (ncr->blt_bpp > 24) {
							ncr->blt_bpp = 24;
						}
						svga->fb_only = 1;
					} else {
						ncr->blt_bpp = 8;
						svga->fb_only = 0;
					}
					break;
				case 0x30:
				case 0x31:
				case 0x32:
				case 0x33:
					ncr_updatemapping(ncr);
					break;
				case 0x20:
					svga->packed_chain4 = (val & 0x02) != 0;
					if (svga->packed_chain4) {
						svga->chain4 = 1;
					}
					ncr_updatebanking(ncr);
					break;
			}
			if (old != val) {
				svga->fullchange = svga->monitor->mon_changeframecount;
				svga_recalctimings(svga);
			}
		}
		break;

		case 0x3d4:
			svga->crtcreg = val & 0x3f;
			return;
		case 0x3d5:
		{
			old = svga->crtc[svga->crtcreg];
			svga->crtc[svga->crtcreg] = val;
			switch (svga->crtcreg)
			{
				case 0x11:
					if (!(val & 0x10)) {
						ncr->vblank_irq = 0;
					}
					ncr_update_irqs(ncr);
					if ((val & ~0x30) == (old & ~0x30)) {
						old = val;
					}
					break;
				case 0x31:
					ncr->ma_ext = val & 15;
					break;
			}
			if (old != val) {
				if (svga->crtcreg < 0xe || svga->crtcreg > 0x10) {
					svga->fullchange = svga->monitor->mon_changeframecount;
					svga_recalctimings(svga);
				}
			}
		}
		break;
	}
	svga_out(addr, val, svga);
}

static uint8_t ncr_in(uint16_t addr, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;
	uint8_t ret;

	if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) {
		addr ^= 0x60;
	}

	switch (addr)
	{
		case 0x3c2:
			ret = svga_in(addr, svga);
			ret |= ncr->vblank_irq > 0 ? 0x80 : 0x00;
			return ret;
		case 0x3c5:
			switch (svga->seqaddr)
			{
				case 0x08:
					ret = ncr->chip << 4;
					if (ncr->chip == NCR_TYPE_22EP) {
						ret |= 8;
					}
					return ret;
			}
			if (svga->seqaddr >= 0x08 && svga->seqaddr < 0x40) {
				return svga->seqregs[svga->seqaddr];
			}
			break;

		case 0x3c6:
			return sdac_ramdac_in(addr, 1, ncr->ramdac, svga);
		case 0x3c7:
		case 0x3c8:
		case 0x3c9:
			return sdac_ramdac_in(addr, 0, ncr->ramdac, svga);

		case 0x3d4:
			return svga->crtcreg;
		case 0x3d5:
			return svga->crtc[svga->crtcreg];
	}
	return svga_in(addr, svga);
}

static const int fontwidths[] =
{
	4, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 8, 8, 8, 8, 8
};

static void ncr_recalctimings(svga_t *svga)
{
	ncr_t *ncr = (ncr_t *)svga->priv;
	bool ext_end = (svga->crtc[0x30] & 0x20) != 0;

	svga->hdisp = svga->crtc[1] - ((svga->crtc[5] & 0x60) >> 5);
	if (ext_end) {
		if (svga->crtc[0x30] & 0x02) {
			svga->hdisp += 0x100;
		}
		if (svga->crtc[0x32] & 0x02) {
			svga->hdisp += 0x200;
		}
		if (svga->crtc[0x30] & 0x01) {
			svga->htotal += 0x100;
		}
		if (svga->crtc[0x32] & 0x01) {
			svga->htotal += 0x200;
		}
		if (svga->crtc[0x33] & 0x02) {
			svga->dispend += 0x400;
		}
		if (svga->crtc[0x33] & 0x01) {
			svga->vtotal += 0x400;
		}
	}
	svga->hdisp++;
	if (svga->seqregs[0x1f] & 0x10) {
		svga->hdisp *= fontwidths[svga->seqregs[0x1f] & 15];
	} else {
		svga->hdisp *= (svga->seqregs[1] & 8) ? 16 : 8;
	}

	if (svga->crtc[0x33] & 0x04) {
		svga->vblankstart += 0x400;
	}

	if (svga->crtc[0x33] & 0x10) {
		svga->split += 0x400;
	}

	if (svga->crtc[0x31] & 0x10) {
		svga->rowoffset += 0x100;
	}

	if (!svga->rowoffset) {
		svga->rowoffset = 256;
	}

	svga->interlace = svga->crtc[0x30] & 0x10;

	if (svga->crtc[0x30] & 0x40) {
		svga->clock /= 2;
	}

	svga->memaddr_latch |= (ncr->ma_ext << 16);

	if (svga->seqregs[0x21] & 0x02) {
		svga->render = svga_render_4bpp_highres;
		svga->hdisp /= 2;
	}

	switch (svga->bpp)
	{
		case 4:
			svga->render = svga_render_4bpp_highres;
			break;
		case 8:
			svga->render = svga_render_8bpp_highres;
			break;
		case 15:
			svga->render = svga_render_15bpp_highres;
			svga->hdisp /= 2;
			break;
		case 16:
			svga->render = svga_render_16bpp_highres;
			svga->hdisp /= 2;
			break;
		case 24:
			svga->render = svga_render_24bpp_highres;
			svga->hdisp /= 3;
			break;
	}
}

/* Horizontal panning (attribute controller register 0x13), done as a
   start address offset: hoverride (see ncr_init()) turns off the core's
   panning. 15/16 bit modes pan by one pixel, 24 bit modes by bytes. */
static void ncr_line_callback(svga_t *svga)
{
	int ar13 = svga->attrregs[0x13] & 7;
	int offset = 0;

	if (!svga->dispon && svga->vc != svga->vtotal) {
		return;
	}
	if ((svga->attrregs[0x10] & 0x20) && svga->vc >= svga->split) {
		return;
	}
	if (svga->render == svga_render_8bpp_highres) {
		offset = (ar13 & 6) >> 1;
	} else if (svga->render == svga_render_15bpp_highres || svga->render == svga_render_16bpp_highres) {
		offset = (ar13 & 4) ? 2 : 0;
	} else if (svga->render == svga_render_24bpp_highres) {
		offset = ar13 >> 1;
	} else if (svga->render == svga_render_32bpp_highres) {
		offset = ((ar13 & 6) >> 1) * 4;
	}
	svga->memaddr += offset;
}

static void ncr_write(uint32_t addr, uint8_t val, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;

	if (!svga->fb_only) {
		svga->write_bank = ncr->bankw[(addr >> 15) & 3];
		svga_write(addr, val, svga);
		return;
	}

	addr = (addr & svga->banked_mask) + ncr->bankw[(addr >> 15) & 3];
	addr &= svga->decode_mask;
	if (addr >= svga->vram_max) {
		return;
	}
	addr &= svga->vram_mask;
	svga->changedvram[addr >> 12] = svga->monitor->mon_changeframecount;
	*(uint8_t *)&svga->vram[addr] = val;
}

static void ncr_writew(uint32_t addr, uint16_t val, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;

	if (!svga->fb_only) {
		svga->write_bank = ncr->bankw[(addr >> 15) & 3];
		svga_writew(addr, val, svga);
		return;
	}

	addr = (addr & svga->banked_mask) + ncr->bankw[(addr >> 15) & 3];
	addr &= svga->decode_mask;
	if (addr >= svga->vram_max) {
		return;
	}
	addr &= svga->vram_mask;
	svga->changedvram[addr >> 12] = svga->monitor->mon_changeframecount;
	*(uint16_t *)&svga->vram[addr] = val;
}

static void ncr_writel(uint32_t addr, uint32_t val, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;

	if (!svga->fb_only) {
		svga->write_bank = ncr->bankw[(addr >> 15) & 3];
		svga_writel(addr, val, svga);
		return;
	}

	addr = (addr & svga->banked_mask) + ncr->bankw[(addr >> 15) & 3];
	addr &= svga->decode_mask;
	if (addr >= svga->vram_max) {
		return;
	}
	addr &= svga->vram_mask;
	svga->changedvram[addr >> 12] = svga->monitor->mon_changeframecount;
	*(uint32_t *)&svga->vram[addr] = val;
}

static uint8_t ncr_read(uint32_t addr, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;

	if (!svga->fb_only) {
		svga->read_bank = ncr->bankr[(addr >> 15) & 3];
		return svga_read(addr, svga);
	}

	addr = (addr & svga->banked_mask) + ncr->bankr[(addr >> 15) & 3];
	addr &= svga->decode_mask;
	if (addr >= svga->vram_max) {
		return 0xff;
	}

	return *(uint8_t *)&svga->vram[addr & svga->vram_mask];
}

static uint16_t ncr_readw(uint32_t addr, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;

	if (!svga->fb_only) {
		svga->read_bank = ncr->bankr[(addr >> 15) & 3];
		return svga_readw(addr, svga);
	}

	addr = (addr & svga->banked_mask) + ncr->bankr[(addr >> 15) & 3];
	addr &= svga->decode_mask;
	if (addr >= svga->vram_max) {
		return 0xffff;
	}
	return *(uint16_t *)&svga->vram[addr & svga->vram_mask];
}

static uint32_t ncr_readl(uint32_t addr, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	svga_t *svga = &ncr->svga;

	if (!svga->fb_only) {
		svga->read_bank = ncr->bankr[(addr >> 15) & 3];
		return svga_readl(addr, svga);
	}

	addr = (addr & svga->banked_mask) + ncr->bankr[(addr >> 15) & 3];
	addr &= svga->decode_mask;
	if (addr >= svga->vram_max) {
		return 0xffffffff;
	}

	return *(uint32_t *)&svga->vram[addr & svga->vram_mask];
}

static uint8_t blitter_rop(uint8_t rop, uint8_t src, uint8_t pat, uint8_t dst)
{
	uint8_t out = 0;
	for (int c = 0; c < 8; c++) {
		uint8_t d = (dst & (1 << c)) ? 1 : 0;
		if (src & (1 << c)) {
			d |= 2;
		}
		if (pat & (1 << c)) {
			d |= 4;
		}
		if (rop & (1 << d)) {
			out |= (1 << c);
		}
	}
	return out;
}

static uint8_t blitter_read(ncr_t *ncr, uint32_t addr, int off)
{
	uint32_t offset = ((addr >> 3) + off) & ncr->vram_mask;
	return ncr->svga.vram[offset];
}

static void blitter_write(ncr_t *ncr, uint32_t addr, uint8_t v)
{
	uint32_t offset = ((addr >> 3) + ncr->blt_bppoffset) & ncr->vram_mask;
	ncr->svga.vram[offset] = v;
	ncr->svga.changedvram[offset >> 12] = ncr->svga.monitor->mon_changeframecount;
}

static bool blitter_proc(ncr_t *ncr)
{
	bool next = false;

	if (ncr->blt_stage == 0) {
		uint8_t src = 0;
		if (!(ncr->blt_control & (1 << 15))) {
			// waiting for fifo write?
			if (ncr->blt_fifo_size < 8) {
				ncr->blt_fifo_write = 1;
				return false;
			}
			src = (ncr->blt_fifo_data) & 0xff;
			ncr->blt_fifo_data >>= 8;
			ncr->blt_fifo_size -= 8;
			ncr->blt_fifo_write = 0;
		} else if (ncr->blt_color_expand) {
			int shift = ncr->blt_expand_offset & 7;
			uint16_t data = blitter_read(ncr, ncr->blt_src, 0) << 0;
			if (shift) {
				data |= blitter_read(ncr, ncr->blt_src, 1) << 8;
				data >>= shift;
			}
			src = (uint8_t)data;
		} else {
			src = blitter_read(ncr, ncr->blt_src, ncr->blt_bppoffset);
		}
		ncr->blt_srcdata = src;
		ncr->blt_stage++;
	}

	if (ncr->blt_stage == 1) {

		uint8_t srcdata = ncr->blt_srcdata;
		bool bitset = false;
		ncr->blt_dstdata = blitter_read(ncr, ncr->blt_dst, ncr->blt_bppoffset);
		ncr->blt_patdata = blitter_read(ncr, ncr->blt_pat, ncr->blt_patx_cnt);
		if (ncr->blt_color_expand) {
			int8_t offset = ncr->blt_expand_bit;
			if (srcdata & (1 << offset)) {
				srcdata = ncr->blt_c0 >> (ncr->blt_bpp_cnt * 8);
				bitset = true;
			} else {
				srcdata = ncr->blt_c1 >> (ncr->blt_bpp_cnt * 8);
			}
		}
		uint8_t out = blitter_rop(ncr->blt_rop, srcdata, ncr->blt_patdata, ncr->blt_dstdata);
		if (ncr->blt_color_expand && ncr->blt_transparent && !bitset) {
			out = ncr->blt_dstdata;
		}
		blitter_write(ncr, ncr->blt_dst, out);

		ncr->blt_patx_cnt++;
		if (ncr->blt_patx_cnt == ncr->blt_pattern_size * ncr->blt_bppdiv8) {
			ncr->blt_patx_cnt = 0;
		}

		ncr->blt_bpp_cnt++;
		ncr->blt_bppoffset++;
		if (ncr->blt_bpp_cnt == ncr->blt_bppdiv8) {
			int16_t dx = (ncr->blt_bppdiv8) * ncr->blt_xdir;

			if (ncr->blt_color_expand) {
				ncr->blt_expand_bit--;
				if (ncr->blt_expand_bit < 0) {
					ncr->blt_expand_bit = 7;
					if (ncr->blt_control & (1 << 15)) {
						ncr->blt_src += (1 << 3);
					}
					ncr->blt_stage = 0;
				} else {
					ncr->blt_stage = 1;
				}
			} else {
				if (ncr->blt_control & (1 << 15)) {
					ncr->blt_src += dx << 3;
				}
				ncr->blt_stage = 0;
			}

			if (ncr->blt_control & (1 << 14)) {
				ncr->blt_dst += dx << 3;
			}

			ncr->blt_bpp_cnt = 0;
			if (ncr->blt_xdir < 0) {
				ncr->blt_bppoffset = -(ncr->blt_bppdiv8 - 1);
			} else {
				ncr->blt_bppoffset = 0;
			}
			next = true;

		} else {

			if (!ncr->blt_color_expand) {
				ncr->blt_stage = 0;
			}

		}

	}
	return next;
}

static void blitter_done(ncr_t *ncr)
{
	ncr->blt_status = 1;
	ncr->blt_start &= ~1;
}

static void blitter_reset_fifo(ncr_t *ncr)
{
	ncr->blt_fifo_size = -(ncr->blt_expand_offset >> 3);
	ncr->blt_fifo_read = 0;
	ncr->blt_fifo_write = 0;
	ncr->blt_expand_bit = 7 - (ncr->blt_expand_offset & 7);
}

static void blitter_run(ncr_t *ncr)
{
	for (;;) {
		if (ncr->blt_status & 1) {
			return;
		}
		if (ncr->blt_fifo_read > 0 || ncr->blt_fifo_write > 0) {
			break;
		}
		if (blitter_proc(ncr)) {
			ncr->blt_w++;
			if (ncr->blt_w >= ncr->blt_width) {
				int dy = ncr->blt_ydir * ncr->svga.rowoffset * 8;

				ncr->blt_w = 0;
				ncr->blt_stage = 0;

				blitter_reset_fifo(ncr);

				if (!(ncr->blt_control & (1 << 12)) && (ncr->blt_control & (1 << 14))) {
					ncr->blt_dstbak += dy << 3;
					ncr->blt_dst = ncr->blt_dstbak;
				}

				if (!(ncr->blt_control & (1 << 13)) && (ncr->blt_control & (1 << 15))) {
					ncr->blt_srcbak += dy << 3;
					ncr->blt_src = ncr->blt_srcbak;
				}

				if (!ncr->blt_color_fill) {
					int patdx = (ncr->blt_bpp / 4) << 3;
					ncr->blt_paty_cnt++;
					ncr->blt_patbak += patdx;
					if (ncr->blt_paty_cnt == ncr->blt_pattern_size) {
						ncr->blt_paty_cnt = 0;
						ncr->blt_patbak = ncr->blt_patbak2;
					}
				}
				ncr->blt_pat = ncr->blt_patbak;
				ncr->blt_patx_cnt = ncr->blt_patx;

				ncr->blt_h++;
				if (ncr->blt_h >= ncr->blt_height) {
					blitter_done(ncr);
					break;
				}
			}
		}
	}
}

static void blitter_write_fifo(ncr_t *ncr, uint32_t v)
{
	ncr->blt_fifo_write = -1;
	ncr->blt_fifo_data <<= 32;
	ncr->blt_fifo_data |= v;
	ncr->blt_fifo_size += 32;
	blitter_run(ncr);
}

static void blitter_start(ncr_t *ncr)
{
	ncr->blt_expand_offset = ncr->blt_src & 31;
	ncr->blt_color_fill = (ncr->blt_control & (1 << 9)) != 0;
	ncr->blt_color_expand = (ncr->blt_control & (1 << 11)) != 0;
	ncr->blt_transparent = (ncr->blt_control & (1 << 5)) != 0;
	ncr->blt_pattern_size = (ncr->blt_control & (1 << 4)) ? 16 : 8;
	ncr->blt_patx = 0;
	ncr->blt_patx_cnt = 0;
	ncr->blt_paty_cnt = 0;

	ncr->blt_w = 0;
	ncr->blt_h = 0;
	ncr->blt_xdir = (ncr->blt_control & (1 << 7)) ? 1 : -1;
	ncr->blt_ydir = (ncr->blt_control & (1 << 6)) ? 1 : -1;
	ncr->blt_bppdiv8 = ncr->blt_bpp / 8;
	ncr->blt_bpp_cnt = 0;
	if (ncr->blt_xdir < 0) {
		ncr->blt_bppoffset = -(ncr->blt_bppdiv8 - 1);
	} else {
		ncr->blt_bppoffset = 0;
	}

	ncr->blt_srcbak = ncr->blt_src;
	ncr->blt_dstbak = ncr->blt_dst;
	ncr->blt_patbak = ncr->blt_pat;
	ncr->blt_patbak2 = ncr->blt_pat;

	if (ncr->blt_bpp == 24) {
		// strange pattern offset in 24-bit mode
		uint32_t dst = ncr->blt_dst;
		dst >>= 3;
		dst /= 3;
		int off = dst & 3;
		int rgboff = 0;
		if (off == 1) {
			rgboff = 3;
		} else if (off == 2) {
			rgboff = 5;
		} else if (off == 3) {
			rgboff = 10;
		}
		ncr->blt_patx = rgboff;
	}

	ncr->blt_patx_cnt = ncr->blt_patx;

	blitter_reset_fifo(ncr);
	ncr->blt_stage = 0;
	ncr->blt_status = 0;

	if (ncr->blt_start & 2) {
		pclog("Blitter: text blits not implemented.\n");
		blitter_done(ncr);
		return;
	}
	if (!(ncr->blt_control & (1 << 14))) {
		pclog("Blitter: destination FIFO reads not implemented.\n");
		blitter_done(ncr);
		return;
	}

	blitter_run(ncr);
}

static void ncr_mmio_write(uint32_t addr, uint8_t val, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	int reg = addr & 0xff;
	switch (reg)
	{
		case 0x09: // mode control
			ncr->svga.seqregs[0x01e] = val;
			ncr_updatemapping(ncr);
			break;
		case 0x30: // blitter start
			ncr->blt_start = val;
			if (val & 1) {
				blitter_start(ncr);
			}
			break;
		case 0x38: // pattern x rotation
			ncr->blt_hprot = val;
			break;
		case 0x39: // pattern y rotation
			ncr->blt_vprot = val;
			break;
		case 0x03a: // rop
			ncr->blt_rop = val;
			break;
	}
}

static void ncr_mmio_write_w(uint32_t addr, uint16_t val, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	int reg = (addr & 0xff) & ~1;
	switch (reg)
	{
		case 0x00: // primary host offset
			break;
		case 0x04: // secondary host offset
			break;
		case 0x0c: // cursor x
			ncr->svga.hwcursor.x = val & ((1 << 10) - 1);
			break;
		case 0x0e: // cursor y
			ncr->svga.hwcursor.y = val & ((1 << 11) - 1);
			break;
		case 0x34: // blitter control
			ncr->blt_control = val;
			break;
		case 0x3c: // blitter bitmap width
			ncr->blt_width = val;
			break;
		case 0x3e: // blitter bitmap height
			ncr->blt_height = val;
			break;
		case 0x40: // blitter destination
			ncr->blt_dst &= 0xffff0000;
			ncr->blt_dst |= val << 0;
			break;
		case 0x42:
			ncr->blt_dst &= 0x0000ffff;
			ncr->blt_dst |= val << 16;
			break;
		case 0x44: // blitter source
			ncr->blt_src &= 0xffff0000;
			ncr->blt_src |= val << 0;
			break;
		case 0x46:
			ncr->blt_src &= 0x0000ffff;
			ncr->blt_src |= val << 16;
			break;
		case 0x48: // blitter pattern
			ncr->blt_pat &= 0xffff0000;
			ncr->blt_pat |= val << 0;
			break;
		case 0x4a:
			ncr->blt_pat &= 0x0000ffff;
			ncr->blt_pat |= val << 16;
			break;
		case 0x4c: // foreground
			ncr->blt_c0 &= 0xffff0000;
			ncr->blt_c0 |= val << 0;
			break;
		case 0x4e:
			ncr->blt_c0 &= 0x0000ffff;
			ncr->blt_c0 |= val << 16;
			break;
		case 0x50: // background
			ncr->blt_c1 &= 0xffff0000;
			ncr->blt_c1 |= val << 0;
			break;
		case 0x52:
			ncr->blt_c1 &= 0x0000ffff;
			ncr->blt_c1 |= val << 16;
			break;
		default:
			ncr_mmio_write(addr + 0, (uint8_t)val, p);
			ncr_mmio_write(addr + 1, val >> 8, p);
			break;
	}
}

static void ncr_mmio_write_l(uint32_t addr, uint32_t val, void *p)
{
	ncr_mmio_write_w(addr + 0, val, p);
	ncr_mmio_write_w(addr + 2, val >> 16, p);
}

static uint8_t ncr_mmio_read(uint32_t addr, void *p)
{
	ncr_t *ncr = (ncr_t *)p;
	int reg = addr & 0xff;
	uint8_t val = 0;
	switch (reg)
	{
		case 0x09: // mode control
			val = ncr->svga.seqregs[0x1e];
			break;
		case 0x30: // blt start
			val = ncr->blt_start;
			break;
		case 0x32: // blt status
			val = ncr->blt_status;
			break;
		default:
			pclog("blitter read reg %02x\n", reg);
			break;
	}

	return val;
}

static uint16_t ncr_mmio_readw(uint32_t addr, void *p)
{
	uint16_t v;
	v = ncr_mmio_read(addr + 0, p);
	v |= ncr_mmio_read(addr + 1, p) << 8;
	return v;
}

static uint32_t ncr_mmio_readl(uint32_t addr, void *p)
{
	uint32_t v;
	v = ncr_mmio_read(addr + 0, p);
	v |= ncr_mmio_read(addr + 2, p) << 16;
	return v;
}

static void ncr_writel_linear(uint32_t addr, uint32_t val, void *p)
{
	svga_t *svga = (svga_t *)p;
	ncr_t *ncr = (ncr_t *)svga->priv;
	if (ncr->blt_fifo_write > 0) {
		blitter_write_fifo(ncr, val);
		return;
	}
	svga_writel_linear(addr, val, p);
}

static void ncr_updatebanking(ncr_t *ncr)
{
	svga_t *svga = &ncr->svga;
	bool rwsep = false;
	uint32_t bankprimary, banksecondary;

	svga->banked_mask = (svga->seqregs[0x20] & 1) ? 0xffff : 0x7fff;
	if (svga->seqregs[0x1e] & 0x10) {
		bankprimary = (svga->seqregs[0x18] << 8) | svga->seqregs[0x19];
		bankprimary <<= 6;
		ncr->bankr[0] = bankprimary;
		ncr->bankr[1] = bankprimary + 0x8000;
		ncr->bankr[2] = bankprimary + 0x10000;
		ncr->bankr[3] = bankprimary + 0x18000;
	} else {
		ncr->bankr[0] = 0x0000;
		ncr->bankr[1] = 0x8000;
		ncr->bankr[2] = 0x10000;
		ncr->bankr[3] = 0x18000;
	}

	if ((svga->seqregs[0x1e] & 0x14) == 0x14) {
		bankprimary = (svga->seqregs[0x18] << 8) | svga->seqregs[0x19];
		banksecondary = (svga->seqregs[0x1c] << 8) | svga->seqregs[0x1d];
		bankprimary <<= 6;
		banksecondary <<= 6;
		int mode = svga->seqregs[0x1e] >> 5;
		if (mode != 0 && mode != 2 && mode != 3 && mode != 6) {
			pclog("unsupported banking mode %d\n", mode);
		}
		if (mode == 0) {
			// Write to Primary, Read From Secondary
			ncr->bankw[0] = bankprimary;
			ncr->bankw[1] = bankprimary + 0x8000;
			ncr->bankw[2] = bankprimary + 0x10000;
			ncr->bankw[3] = bankprimary + 0x18000;
			ncr->bankr[0] = banksecondary;
			ncr->bankr[1] = banksecondary + 0x8000;
			ncr->bankr[2] = banksecondary + 0x10000;
			ncr->bankr[3] = banksecondary + 0x18000;
			rwsep = true;
		}
		if (mode == 2) {
			// Primary at A0000h-AFFFFh, Secondary at B0000h-BFFFFh. Both Read / Write.
			ncr->bankr[0] = bankprimary;
			ncr->bankr[1] = bankprimary + 0x8000;
			ncr->bankr[2] = banksecondary;
			ncr->bankr[3] = banksecondary + 0x8000;
		} else if (mode == 3) {
			// Read and Write to Secondary only
			ncr->bankr[0] = banksecondary;
			ncr->bankr[1] = banksecondary + 0x8000;
			ncr->bankr[2] = banksecondary + 0x10000;
			ncr->bankr[3] = banksecondary + 0x18000;
		} else if (mode == 6) {
			// Reads/writes Primary on A0, Secondary on A8.
			// Both banks address the whole 64k window: the secondary bank
			// shows its second half at A8 (P96 Retina.card relies on it).
			ncr->bankr[0] = bankprimary;
			ncr->bankr[1] = banksecondary + 0x8000;
			ncr->bankr[2] = bankprimary + 0x8000;
			ncr->bankr[3] = banksecondary + 0x10000;
		}
	}
	if (!rwsep) {
		ncr->bankw[0] = ncr->bankr[0];
		ncr->bankw[1] = ncr->bankr[1];
		ncr->bankw[2] = ncr->bankr[2];
		ncr->bankw[3] = ncr->bankr[3];
	}
}

static void ncr_updatemapping(ncr_t *ncr)
{
	svga_t *svga = &ncr->svga;

	ncr_updatebanking(ncr);

	mem_mapping_disable(&svga->mapping);
	mem_mapping_disable(&ncr->mmio_mapping);
	mem_mapping_disable(&ncr->linear_mapping);

	if (ncr->chip == NCR_TYPE_32BLT) {
		ncr->linear_size = 0x100000 << (svga->seqregs[0x1a] & 3);
		if (ncr->linear_size > 0x400000) {
			ncr->linear_size = 0x400000;
		}
		ncr->linear_base = ((svga->seqregs[0x1a] >> 4) << 20) | (svga->seqregs[0x1b] << 24);
		ncr->linear_base &= ~(ncr->linear_size - 1);
		mem_mapping_set_addr(&ncr->linear_mapping, ncr->linear_base, ncr->linear_size);

		if (svga->seqregs[0x30] & 1) {
			ncr->mmio_base = (svga->seqregs[0x31] << 8) | (svga->seqregs[0x32] << 16) | (svga->seqregs[0x33] << 24);
			ncr->mmio_size = 0x100;
			mem_mapping_set_addr(&ncr->mmio_mapping, ncr->mmio_base, ncr->mmio_size);
			mem_mapping_enable(&ncr->mmio_mapping);
		} else {
			mem_mapping_disable(&ncr->mmio_mapping);
		}

	} else {

		mem_mapping_set_addr(&svga->mapping, 0xa0000, 0x10000);
		mem_mapping_enable(&svga->mapping);

	}
}

static void ncr_io_remove(ncr_t *ncr)
{
	io_removehandler(0x03c0, 0x0020, ncr_in, NULL, NULL, ncr_out, NULL, NULL, ncr);
}

static void ncr_io_set(ncr_t *ncr)
{
	ncr_io_remove(ncr);

	io_sethandler(0x03c0, 0x0020, ncr_in, NULL, NULL, ncr_out, NULL, NULL, ncr);
}

static ncr_t *ncr_init(const device_t *info, int chip)
{
	ncr_t *ncr = (ncr_t *)calloc(1, sizeof(ncr_t));
	svga_t *svga = &ncr->svga;
	int vram;
	uint32_t vram_size;

	vram = device_get_config_int("memory");
	if (vram) {
		vram_size = vram << 20;
	} else {
		vram_size = 512 << 10;
	}
	ncr->vram_mask = vram_size - 1;

	svga_init(info, &ncr->svga, ncr, vram_size,
		ncr_recalctimings,
		ncr_in, ncr_out,
		ncr_hwcursor_draw,
		NULL);

	mem_mapping_add(&ncr->linear_mapping, 0, 0, svga_read_linear, svga_readw_linear, svga_readl_linear, svga_write_linear, svga_writew_linear, ncr_writel_linear, NULL, MEM_MAPPING_EXTERNAL, &ncr->svga);
	mem_mapping_set_handler(&ncr->svga.mapping, ncr_read, ncr_readw, ncr_readl, ncr_write, ncr_writew, ncr_writel);
	mem_mapping_add(&ncr->mmio_mapping, 0xa0000, 0x10000, ncr_mmio_read, ncr_mmio_readw, ncr_mmio_readl, ncr_mmio_write, ncr_mmio_write_w, ncr_mmio_write_l, NULL, MEM_MAPPING_EXTERNAL, ncr);
	mem_mapping_set_p(&ncr->svga.mapping, ncr);
	mem_mapping_disable(&ncr->mmio_mapping);

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

	svga->vsync_callback = ncr_vblank_start;
	svga->line_callback = ncr_line_callback;
	/* hdisp comes from ncr_recalctimings() alone, the CRTC's 8 bit
	   horizontal blanking registers don't cover the extended modes. */
	svga->hoverride = 1;
	/* 0x3c6 is the SDAC's command register, the pixel mask stays open */
	svga->dac_mask = 0xff;

	ncr_io_set(ncr);

	ncr->chip = chip;
	ncr->blt_status = 1;
	ncr->blt_bpp = 8;

	ncr->ramdac = device_add(&sdac_ramdac_device);

	ncr_updatemapping(ncr);

	return ncr;
}

static void *ncr_retina_z2_init(const device_t *info)
{
	ncr_t *ncr = ncr_init(info, NCR_TYPE_22EP);

	ncr->svga.fb_only = 0;
	svga_set_ramdac_type(&ncr->svga, RAMDAC_8BIT);

	return ncr;
}

static void *ncr_retina_z3_init(const device_t *info)
{
	ncr_t *ncr = ncr_init(info, NCR_TYPE_32BLT);

	ncr->svga.fb_only = -1;

	return ncr;
}

static void ncr_close(void *p)
{
	ncr_t *ncr = (ncr_t *)p;

	svga_close(&ncr->svga);

	free(ncr);
}

static void ncr_speed_changed(void *p)
{
	ncr_t *ncr = (ncr_t *)p;

	svga_recalctimings(&ncr->svga);
}

static void ncr_force_redraw(void *p)
{
	ncr_t *ncr = (ncr_t *)p;

	ncr->svga.fullchange = ncr->svga.monitor->mon_changeframecount;
}

const device_t ncr_retina_z2_device = {
	.name          = "NCR 77C22E+",
	.internal_name = "ncr_retina_z2",
	.flags         = 0,
	.local         = 0,
	.init          = ncr_retina_z2_init,
	.close         = ncr_close,
	.reset         = NULL,
	.available     = NULL,
	.speed_changed = ncr_speed_changed,
	.force_redraw  = ncr_force_redraw,
	.config        = NULL
};

const device_t ncr_retina_z3_device = {
	.name          = "NCR 77C32BLT",
	.internal_name = "ncr_retina_z3",
	.flags         = 0,
	.local         = 0,
	.init          = ncr_retina_z3_init,
	.close         = ncr_close,
	.reset         = NULL,
	.available     = NULL,
	.speed_changed = ncr_speed_changed,
	.force_redraw  = ncr_force_redraw,
	.config        = NULL
};
