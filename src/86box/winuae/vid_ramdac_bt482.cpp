/*
* UAE - The Un*x Amiga Emulator
*
* Brooktree BT482 RAMDAC of the Merlin RTG board, used by the 86Box
* ET4000/W32 emulation (vid_et4000w32.cpp). 86Box has no BT482, this is
* WinUAE's (Toni Wilen), based on 86Box's BT484 emulation.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <86box/86box.h>
#include <86box/device.h>
#include <86box/mem.h>
#include <86box/timer.h>
#include <86box/video.h>
#include <86box/vid_svga.h>

typedef struct bt482_ramdac_t
{
	uint32_t extpallook[4];
	uint8_t rgb[3];
	uint8_t cursor32_data[256];
	int hwc_y;
	int hwc_x;
	uint8_t cmd_r0;
	uint8_t cmd_r1;
	uint8_t cursor_r;
} bt482_ramdac_t;

static void bt482_set_bpp(bt482_ramdac_t *ramdac, svga_t *svga)
{
	svga->swaprb = 0;
	switch (ramdac->cmd_r0 >> 4) {
		case 0x0f:
			svga->bpp = 24;
			svga->swaprb = (ramdac->cmd_r0 & 2) == 0;
			break;
		case 0x09:
		case 0x0b:
			svga->bpp = 32;
			svga->swaprb = (ramdac->cmd_r0 & 2) == 0;
			break;
		case 0x0e:
		case 0x0c:
			svga->bpp = 16;
			break;
		case 0x08:
		case 0x0a:
			svga->bpp = 15;
			break;
		default:
			svga->bpp = 8;
			break;
	}
	svga_recalctimings(svga);
}

/* addr: 3C8-3CB for RS0-1, rs2: RS2 */
void bt482_ramdac_out(uint16_t addr, int rs2, uint8_t val, void *priv, svga_t *svga)
{
	bt482_ramdac_t *ramdac = (bt482_ramdac_t *)priv;
	uint8_t rs = addr & 0x03;
	uint16_t da_mask = 0x0ff;

	rs |= (!!rs2) << 2;
	switch (rs) {
		case 0x00: /* Palette write index */
			svga->dac_pos = 0;
			svga->dac_status = addr & 0x03;
			svga->dac_addr = val;
			if (svga->dac_status) {
				svga->dac_addr = (svga->dac_addr + 1) & da_mask;
			}
			break;
		case 0x01: /* Palette data */
			svga_out(addr, val, svga);
			break;
		case 0x02:
			if (ramdac->cmd_r0 & 1) {
				/* Extended registers, indexed by the palette write index */
				switch (svga->dac_addr) {
					case 2: /* Command register B */
						svga_set_ramdac_type(svga, (val & 0x02) ? RAMDAC_8BIT : RAMDAC_6BIT);
						ramdac->cmd_r1 = val;
						break;
					case 3: /* Cursor register */
						ramdac->cursor_r = val;
						svga->dac_hwcursor.cur_xsize = 32;
						svga->dac_hwcursor.cur_ysize = 32;
						svga->dac_hwcursor.ena = (val & 0x03) != 0;
						break;
					case 4: /* Cursor X low */
						ramdac->hwc_x = (ramdac->hwc_x & 0x0f00) | val;
						svga->dac_hwcursor.x = ramdac->hwc_x - svga->dac_hwcursor.cur_xsize;
						break;
					case 5: /* Cursor X high */
						ramdac->hwc_x = (ramdac->hwc_x & 0x00ff) | ((val & 0x0f) << 8);
						svga->dac_hwcursor.x = ramdac->hwc_x - svga->dac_hwcursor.cur_xsize;
						break;
					case 6: /* Cursor Y low */
						ramdac->hwc_y = (ramdac->hwc_y & 0x0f00) | val;
						svga->dac_hwcursor.y = ramdac->hwc_y - svga->dac_hwcursor.cur_ysize;
						break;
					case 7: /* Cursor Y high */
						ramdac->hwc_y = (ramdac->hwc_y & 0x00ff) | ((val & 0x0f) << 8);
						svga->dac_hwcursor.y = ramdac->hwc_y - svga->dac_hwcursor.cur_ysize;
						break;
				}
			} else {
				/* Pixel mask */
				svga->dac_mask = val;
			}
			break;
		case 0x04: /* Overlay and cursor color write index */
			svga->dac_addr = val;
			svga->dac_pos = 0;
			break;
		case 0x05: /* Overlay and cursor color, or cursor RAM */
			if (ramdac->cursor_r & (1 << 3)) {
				ramdac->cursor32_data[svga->dac_addr] = val;
				svga->dac_addr = (svga->dac_addr + 1) & da_mask;
			} else {
				ramdac->rgb[svga->dac_pos] = val;
				svga->dac_pos++;
				if (svga->dac_pos == 3) {
					ramdac->extpallook[svga->dac_addr & 3] = makecol32(ramdac->rgb[0], ramdac->rgb[1], ramdac->rgb[2]);
					svga->dac_addr++;
					svga->dac_pos = 0;
				}
			}
			break;
		case 0x06: /* Command register A */
			ramdac->cmd_r0 = val;
			bt482_set_bpp(ramdac, svga);
			break;
	}
}

uint8_t bt482_ramdac_in(uint16_t addr, int rs2, void *priv, svga_t *svga)
{
	bt482_ramdac_t *ramdac = (bt482_ramdac_t *)priv;
	uint8_t temp = 0xff;
	uint8_t rs = addr & 0x03;

	rs |= (!!rs2) << 2;
	switch (rs) {
		case 0x00: /* Palette write index */
			temp = svga_in(addr, svga);
			break;
		case 0x02:
			if (ramdac->cmd_r0 & 1) {
				switch (svga->dac_addr) {
					case 2: /* Command register B */
						temp = ramdac->cmd_r1;
						break;
					case 3: /* Cursor register */
						temp = ramdac->cursor_r;
						break;
					case 4: /* Cursor X low */
						temp = ramdac->hwc_x & 0xff;
						break;
					case 5: /* Cursor X high */
						temp = (ramdac->hwc_x >> 8) & 0xff;
						break;
					case 6: /* Cursor Y low */
						temp = ramdac->hwc_y & 0xff;
						break;
					case 7: /* Cursor Y high */
						temp = (ramdac->hwc_y >> 8) & 0xff;
						break;
				}
			} else {
				temp = svga->dac_mask;
			}
			break;
		case 0x03: /* Palette read index */
			temp = svga->dac_addr & 0xff;
			break;
		case 0x06: /* Command register A */
			temp = ramdac->cmd_r0;
			break;
	}
	return temp;
}

/* 32x32 two plane cursor from the cursor RAM */
void bt482_hwcursor_draw(svga_t *svga, int displine)
{
	bt482_ramdac_t *ramdac = (bt482_ramdac_t *)svga->ramdac;
	int offset = svga->dac_hwcursor_latch.x - svga->dac_hwcursor_latch.xoff;
	uint32_t clr1 = ramdac->extpallook[1];
	uint32_t clr2 = ramdac->extpallook[2];
	uint32_t clr3 = ramdac->extpallook[3];
	const uint8_t *cd = ramdac->cursor32_data;
	int mode = ramdac->cursor_r & 3;
	/* Bytes per line and per plane */
	int pitch = svga->dac_hwcursor_latch.cur_xsize >> 3;
	int bppl = pitch * svga->dac_hwcursor_latch.cur_ysize;
	uint32_t *p = buffer32->line[displine];

	/* The low resolution modes show each pixel twice */
	if (svga->lowres) {
		offset <<= 1;
	}

	if (svga->interlace && svga->dac_hwcursor_oddeven) {
		svga->dac_hwcursor_latch.addr += pitch;
	}

	for (int x = 0; x < svga->dac_hwcursor_latch.cur_xsize; x += 16) {
		uint16_t dat0 = (cd[svga->dac_hwcursor_latch.addr] << 8) | cd[svga->dac_hwcursor_latch.addr + 1];
		uint16_t dat1 = (cd[svga->dac_hwcursor_latch.addr + bppl] << 8) | cd[svga->dac_hwcursor_latch.addr + bppl + 1];

		for (int xx = 0; xx < 16; xx++) {
			int comb = ((dat0 >> (15 - xx)) & 1) | (((dat1 >> (15 - xx)) & 1) << 1);
			int x_pos = (offset + svga->x_add) & 2047;

			if (offset >= svga->dac_hwcursor_latch.x) {
				switch (mode) {
					case 1: /* Three color */
						if (comb == 1) {
							p[x_pos] = clr1;
						} else if (comb == 2) {
							p[x_pos] = clr2;
						} else if (comb == 3) {
							p[x_pos] = clr3;
						}
						break;
					case 2: /* PM/Windows */
						if (comb == 0) {
							p[x_pos] = clr1;
						} else if (comb == 1) {
							p[x_pos] = clr2;
						} else if (comb == 3) {
							p[x_pos] ^= 0xffffff;
						}
						break;
					case 3: /* X Windows */
						if (comb == 2) {
							p[x_pos] = clr1;
						} else if (comb == 3) {
							p[x_pos] = clr2;
						}
						break;
				}
			}
			offset++;
		}
		svga->dac_hwcursor_latch.addr += 2;
	}

	if (svga->interlace && !svga->dac_hwcursor_oddeven) {
		svga->dac_hwcursor_latch.addr += pitch;
	}
}

static void *bt482_ramdac_init(const device_t *info)
{
	return calloc(1, sizeof(bt482_ramdac_t));
}

static void bt482_ramdac_close(void *priv)
{
	free(priv);
}

extern const device_t bt482_ramdac_device;

const device_t bt482_ramdac_device = {
	.name          = "Brooktree Bt482 RAMDAC",
	.internal_name = "bt482_ramdac",
	.flags         = 0,
	.local         = 0,
	.init          = bt482_ramdac_init,
	.close         = bt482_ramdac_close,
	.reset         = NULL,
	.available     = NULL,
	.speed_changed = NULL,
	.force_redraw  = NULL,
	.config        = NULL
};
