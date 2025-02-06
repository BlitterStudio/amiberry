
/* BT482 RAMDAC (based on 86box BT484) */

#include <memory>
#include "ibm.h"
#include "mem.h"
#include "device.h"
#include "video.h"
#include "vid_svga.h"
#include "vid_sdac_ramdac.h"

enum {
    BT481 = 0,
    BT482,
};

static void
bt482_set_bpp(bt482_ramdac_t *ramdac, svga_t *svga)
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

void
bt482_ramdac_out(uint16_t addr, int rs2, uint8_t val, void *priv, svga_t *svga)
{
    bt482_ramdac_t *ramdac = (bt482_ramdac_t *) priv;
    uint8_t         rs      = (addr & 0x03);
    uint16_t        da_mask = 0x0ff;
    rs |= (!!rs2 << 2);

    switch (rs) {
        case 0x00: /* Palette Write Index Register (RS value = 0000) */
            svga->dac_pos    = 0;
            svga->dac_status = addr & 0x03;
            svga->dac_addr   = val;
            if (svga->dac_status)
                svga->dac_addr = (svga->dac_addr + 1) & da_mask;
            break;
        case 0x01: /* Palette Data Register (RS value = 0001) */
            svga_out(addr, val, svga);
            break;
        case 0x02:
            if (ramdac->cmd_r0 & 1) {
                switch(svga->dac_addr)
                {
                    case 2: // command B
                        svga_set_ramdac_type(svga, (val & 0x02) ? RAMDAC_8BIT : RAMDAC_6BIT);
                        ramdac->cmd_r1 = val;
                        break;
                    case 3: // cursor
                        ramdac->cursor_r = val;
                        svga->dac_hwcursor.cur_xsize = 32;
                        svga->dac_hwcursor.cur_ysize = 32;
                        svga->dac_hwcursor.ena = (val & 0x03) != 0;
                        break;
                    case 4: // cursor x low
                        ramdac->hwc_x = (ramdac->hwc_x & 0x0f00) | val;
                        svga->dac_hwcursor.x = ramdac->hwc_x - svga->dac_hwcursor.cur_xsize;
                        break;
                    case 5: // cursor x high
                        ramdac->hwc_x = (ramdac->hwc_x & 0x00ff) | ((val & 0x0f) << 8);
                        svga->dac_hwcursor.x = ramdac->hwc_x - svga->dac_hwcursor.cur_xsize;
                        break;
                    case 6: // cursor y low
                        ramdac->hwc_y = (ramdac->hwc_y & 0x0f00) | val;
                        svga->dac_hwcursor.y = ramdac->hwc_y - svga->dac_hwcursor.cur_ysize;
                        break;
                    case 7: // cursor y high
                        ramdac->hwc_y = (ramdac->hwc_y & 0x00ff) | ((val & 0x0f) << 8);
                        svga->dac_hwcursor.y = ramdac->hwc_y - svga->dac_hwcursor.cur_ysize;
                        break;
                }
            } else {
                // pixel mask
            }
            break;
        case 0x04: // overlay and cursor color write
            svga->dac_addr = val;
            svga->dac_pos = 0;
            break;
        case 0x05: // overlay/cursor color
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
        case 0x06: /* Command Register A (RS value = 0110) */
            ramdac->cmd_r0    = val;
            bt482_set_bpp(ramdac, svga);
            break;
    }

    return;
}

uint8_t
bt482_ramdac_in(uint16_t addr, int rs2, void *priv, svga_t *svga)
{
    bt482_ramdac_t *ramdac = (bt482_ramdac_t *) priv;
    uint8_t         temp   = 0xff;
    uint8_t         rs      = (addr & 0x03);
    uint16_t        da_mask = 0x00ff;
    rs |= (!!rs2 << 2);

    // TODO
    switch (rs) {
        case 0x00: /* Palette Write Index Register (RS value = 0000) */
            temp = svga_in(addr, svga);
            break;
        case 0x02:
            if (ramdac->cmd_r0 & 1) {
                switch (svga->dac_addr)
                {
                    case 2: // command B
                        temp = ramdac->cmd_r1;
                        break;
                    case 3: // cursor
                        temp = ramdac->cursor_r ;
                        break;
                    case 4: // cursor x low
                        temp = ramdac->hwc_x & 0xff;
                        break;
                    case 5: // cursor x high
                        temp = (ramdac->hwc_x >> 8) & 0xff;
                        break;
                    case 6: // cursor y low
                        temp = ramdac->hwc_y & 0xff;
                        break;
                    case 7: // cursor y high
                        temp = (ramdac->hwc_y >> 8) & 0xff;
                        break;
                }
            }
            break;
        case 0x03: /* Palette Read Index Register (RS value = 0011) */
            temp = svga->dac_addr & 0xff;
            break;
        case 0x06: /* Command Register 0 (RS value = 0110) */
            temp = ramdac->cmd_r0;
            break;
    }

    return temp;
}

void
bt482_recalctimings(void *priv, svga_t *svga)
{
    const bt482_ramdac_t *ramdac = (bt482_ramdac_t *) priv;
}

void
bt482_hwcursor_draw(svga_t *svga, int displine)
{
    int             comb;
    int             b0;
    int             b1;
    uint16_t        dat[2];
    int             offset = svga->dac_hwcursor_latch.x - svga->dac_hwcursor_latch.xoff;
    int             pitch;
    int             bppl;
    int             mode;
    int             x_pos;
    int             y_pos;
    uint32_t        clr1;
    uint32_t        clr2;
    uint32_t        clr3;
    uint32_t       *p;
    const uint8_t  *cd;
    bt482_ramdac_t *ramdac = (bt482_ramdac_t *) svga->ramdac;

    clr1 = ramdac->extpallook[1];
    clr2 = ramdac->extpallook[2];
    clr3 = ramdac->extpallook[3];

    offset <<= ((svga->horizontal_linedbl ? 1 : 0) + (svga->lowres ? 1 : 0));

    /* The planes come in two parts, and each plane is 1bpp,
       32x32 cursor has 4 bytes per line */
    pitch = (svga->dac_hwcursor_latch.cur_xsize >> 3); /* Bytes per line. */
    /* A 32x32 cursor has 128 bytes per line */
    bppl = (pitch * svga->dac_hwcursor_latch.cur_ysize); /* Bytes per plane. */

    if (svga->interlace && svga->dac_hwcursor_oddeven)
        svga->dac_hwcursor_latch.addr += pitch;

    cd = (uint8_t *) ramdac->cursor32_data;
    mode = ramdac->cursor_r & 3;

    for (int x = 0; x < svga->dac_hwcursor_latch.cur_xsize; x += 16) {
        dat[0] = (cd[svga->dac_hwcursor_latch.addr] << 8) | cd[svga->dac_hwcursor_latch.addr + 1];
        dat[1] = (cd[svga->dac_hwcursor_latch.addr + bppl] << 8) | cd[svga->dac_hwcursor_latch.addr + bppl + 1];

        for (uint8_t xx = 0; xx < 16; xx++) {
            b0   = (dat[0] >> (15 - xx)) & 1;
            b1   = (dat[1] >> (15 - xx)) & 1;
            comb = (b0 | (b1 << 1));

            y_pos = displine;
            x_pos = offset + svga->x_add + 32;
            p     = (uint32_t*)(buffer32->line[y_pos]);

            if (offset >= svga->dac_hwcursor_latch.x) {
                switch (mode) {
                    case 1: /* Three Color */
                        switch (comb) {
                            case 1:
                                p[x_pos] = clr1;
                                break;
                            case 2:
                                p[x_pos] = clr2;
                                break;
                            case 3:
                                p[x_pos] = clr3;
                                break;

                            default:
                                break;
                        }
                        break;
                    case 2: /* PM/Windows */
                        switch (comb) {
                            case 0:
                                p[x_pos] = clr1;
                                break;
                            case 1:
                                p[x_pos] = clr2;
                                break;
                            case 3:
                                p[x_pos] ^= 0xffffff;
                                break;

                            default:
                                break;
                        }
                        break;
                    case 3: /* X-Windows */
                        switch (comb) {
                            case 2:
                                p[x_pos] = clr1;
                                break;
                            case 3:
                                p[x_pos] = clr2;
                                break;

                            default:
                                break;
                        }
                        break;

                    default:
                        break;
                }
            }
            offset++;
        }
        svga->dac_hwcursor_latch.addr += 2;
    }

    if (svga->interlace && !svga->dac_hwcursor_oddeven)
        svga->dac_hwcursor_latch.addr += pitch;
}

void *
bt482_ramdac_init(const device_t *info)
{
    bt482_ramdac_t *ramdac = (bt482_ramdac_t *) malloc(sizeof(bt482_ramdac_t));
    memset(ramdac, 0, sizeof(bt482_ramdac_t));

    return ramdac;
}

void
bt482_ramdac_close(void *priv)
{
    bt482_ramdac_t *ramdac = (bt482_ramdac_t *) priv;

    if (ramdac)
        free(ramdac);
}
