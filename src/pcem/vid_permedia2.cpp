
/*

Basic Permedia 2 (Amiga CyberVision/BlizzardVision PPC) emulation by Toni Wilen 2024

Emulated:

- Graphics processor mode standard screen modes supported (8/15/16/24/32 bits). Other weird modes are not supported.
- VRAM aperture byte swapping and RAMDAC red/blue swapping modes. (Used at least by Picasso96 driver)
- Hardware cursor.
- Most of 2D blitter operations. Amiga Picasso96 and CGX4 drivers work.

Not emulated and Someone Else's Problem:

- SVGA core. Amiga programs seem to always use Graphics processor mode.
- 3D. 3D is someone else's problem. (Maybe some PCem or x86Box developer is interested?)
- Other Permedia 2 special features (front/back buffer swapping, local buffer/DMA, stereo support etc)

*/

#include <stdlib.h>
#include "ibm.h"
#include "device.h"
#include "io.h"
#include "pci.h"
#include "mem.h"
#include "rom.h"
#include "thread.h"
#include "video.h"
#include "vid_permedia2.h"
#include "vid_svga.h"
#include "vid_svga_render.h"
#include "vid_sdac_ramdac.h"

#include "machdep/maccess.h"

#define BLIT_LOG 0

#define FRACT_ONE (1 << 16)

#define REG_STARTXDOM           0x0000
#define REG_DXDOM				0x0008
#define REG_STARTXSUB			0x0010
#define REG_DXSUB				0x0018
#define REG_STARTY				0x0020
#define REG_DY				    0x0028
#define REG_COUNT				0x0030
#define REG_RENDER				0x0038
#define REG_CONTINUENEWLINE	    0x0040
#define REG_CONTINUENEWSUB	    0x0050
#define REG_BITMASKPATTERN		0x0068
#define REG_RASTERIZERMODE		0x00a0
#define REG_RECTANGLEORIGIN	    0x00d0
#define REG_RECTANGLESIZE		0x00d8
#define REG_PACKEDDATALIMITS	0x0150
#define REG_SCISSORMODE		    0x0180
#define REG_SCISSORMINXY		0x0188
#define REG_SCISSORMAXXY		0x0190
#define REG_SCREENSIZE			0x0198
#define REG_AREASTIPPLEMODE	    0x01a0
#define REG_WINDOWORIGIN		0x01c8
#define REG_AREASTIPPLEPATTERN0 0x0200
#define REG_TEXEL0				0x0600
#define REG_COLORDDAMODE		0x07e0
#define REG_CONSTANTCOLOR		0x07e8
#define REG_FBSOFTWRITEMASK	    0x0820
#define REG_LOGICALOPMODE		0x0828
#define REG_FBWRITEDATA		    0x0830
#define REG_WINDOW				0x0980
#define REG_FBREADMODE			0x0a80
#define REG_FBSOURCEOFFSET		0x0a88
#define REG_FBPIXELOFFSET		0x0a90
#define REG_FBSOURCEDATA		0x0aa8
#define REG_FBWINDOWBASE		0x0ab0
#define REG_FBWRITEMODE		    0x0ab8
#define REG_FBHARDWRITEMASK	    0x0ac0
#define REG_FBBLOCKCOLOR		0x0ac8
#define REG_FBREADPIXEL		    0x0ad0
#define REG_SYNC                0x0c40
#define REG_FBSOURCEBASE		0x0d80
#define REG_FBSOURCEDELTA		0x0d88
#define REG_CONFIG				0x0d90

#ifdef DEBUGGER
extern void activate_debugger(void);
#endif

typedef struct permedia2_t
{
        mem_mapping_t linear_mapping[2];
        mem_mapping_t mmio_mapping;
        
        rom_t bios_rom;
        
        svga_t svga;
        sdac_ramdac_t ramdac;
        uint8_t pci_regs[256];
        int card;

        uint8_t ma_ext;

        int chip;
        uint8_t int_line;
        uint8_t id_ext_pci;

        uint32_t linear_base[2], linear_size;
        uint32_t mmio_base, mmio_size;

        uint32_t intena, intreq;

        uint32_t bank[4];
        uint32_t vram_mask;
        
        float (*getclock)(int clock, void *p);
        void *getclock_p;

        int vblank_irq;

        uint8_t ramdac_reg;
        uint8_t ramdac_pal, ramdac_palcomp;
        uint8_t ramdac_cpal, ramdac_cpalcomp;
        uint8_t ramdac_vals[256];
        uint32_t vc_regs[256];
        uint32_t ramdac_hwc_col[4];
        uint16_t ramdac_cramaddr;
        uint8_t ramdac_cram[1024];

        uint8_t linear_byte_control[2];

        uint32_t gp_regs[0x2000 / 4];
        int gp_pitch;
        int gp_bpp;
        int gp_bytesperline;
        int gp_type;
        bool gp_astipple, gp_texena, gp_fogena, gp_fastfill;
        bool gp_synconbitmask, gp_reusebitmask, gp_colordda;
        int gp_incx, gp_incy;
        int gp_dxsub, gp_dxdom;
        bool gp_readsrc, gp_readdst, gp_writedst;
        bool gp_doswmask, gp_dohwmask;
        bool gp_ropena, gp_constfbdata;
        bool gp_packeddata;
        uint32_t gp_color;
        uint8_t gp_rop;
        int gp_x, gp_y, gp_x1, gp_y1, gp_x2, gp_y2;
        int gp_dx, gp_dy, gp_sx, gp_sy;
        int gp_len;
        uint32_t gp_bitmaskpattern;
        uint32_t gp_src, gp_dst;
        bool gp_scissor_screen;
        bool gp_scissor_user;
        uint32_t gp_scissor_screen_w, gp_scissor_screen_h;
        int gp_scissor_min_x, gp_scissor_min_y;
        int gp_scissor_max_x, gp_scissor_max_y;

        bool gp_asena;
        bool gp_asmirrorx, gp_asmirrory;
        uint8_t gp_asinvert;
        bool gp_asforcebackgroundcolor;
        uint8_t gp_asxoffset, gp_asyoffset;

        bool gp_bitmaskena;
        bool gp_bitmaskrelative;
        int gp_bitmaskoffset;
        bool gp_bitmaskpacking;
        bool gp_bmforcebackgroundcolor;
        bool gp_mirrorbitmask;
        bool gp_waitbitmask;
        int gp_bitmaskpatterncnt;
        bool gp_bmcheckresult;

        int gp_packedlimitxstart, gp_packedlimitxend;

        uint32_t gp_outfifocnt;
        uint32_t gp_outfifodata;

} permedia2_t;

void permedia2_updatemapping(permedia2_t*);
void permedia2_updatebanking(permedia2_t*);

static int permedia2_vsync_enabled(permedia2_t *permedia2)
{
    if (permedia2->intena & 0x10) {
        return 1;
    }
    return 0;
}

static void permedia2_update_irqs(permedia2_t *permedia2)
{
    if (permedia2->vblank_irq > 0 && permedia2_vsync_enabled(permedia2)) {
        permedia2->intreq |= 0x10;
        pci_set_irq(0, PCI_INTA, nullptr);
    } else {
        pci_clear_irq(0, PCI_INTA, nullptr);
    }
}

static void permedia2_vblank_start(svga_t *svga)
{
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;
    if (permedia2->vblank_irq >= 0) {
        permedia2->vblank_irq = 1;
        permedia2_update_irqs(permedia2);
    }
}



void permedia2_out(uint16_t addr, uint8_t val, void *p)
{
        permedia2_t *permedia2 = (permedia2_t *)p;
        svga_t *svga = &permedia2->svga;
        uint8_t old;

        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;
        
        switch (addr)
        {
                case 0x3C6:
                    val &= ~0x10;
                    sdac_ramdac_out((addr & 3) | 4, val, &permedia2->ramdac, svga);
                    return;
                case 0x3C7: case 0x3C8: case 0x3C9:
                    sdac_ramdac_out(addr & 3, val, &permedia2->ramdac, svga);
                    return;

                case 0x3c4:
                svga->seqaddr = val & 0x3f;
                break;
                case 0x3c5:
                {
                    old = svga->seqregs[svga->seqaddr];
                    svga->seqregs[svga->seqaddr] = val;
                    if (old != val && svga->seqaddr >= 0x0a) {
                        svga->fullchange = changeframecount;
                    }
                    switch (svga->seqaddr)
                    {
                        default:
                        break;
                    }
                    if (old != val)
                    {
                        svga->fullchange = changeframecount;
                        svga_recalctimings(svga);
                    }
                }
                break;

                case 0x3d4:
                svga->crtcreg = val & svga->crtcreg_mask;
                return;
                case 0x3d5:
                {
                    old = svga->crtc[svga->crtcreg];
                    svga->crtc[svga->crtcreg] = val;
                    switch (svga->crtcreg)
                    {
                            case 0x11:
                            if (!(val & 0x10)) {
                                permedia2->vblank_irq = 0;
                            }
                            permedia2_update_irqs(permedia2);
                            if ((val & ~0x30) == (old & ~0x30))
                                old = val;
                            break;
                    }
                    if (old != val)
                    {
                            if (svga->crtcreg < 0xe || svga->crtcreg > 0x10)
                            {
                                    svga->fullchange = changeframecount;
                                    svga_recalctimings(svga);
                            }
                    }
                }
                break;
        }
        svga_out(addr, val, svga);
}

uint8_t permedia2_in(uint16_t addr, void *p)
{
        permedia2_t *permedia2 = (permedia2_t *)p;
        svga_t *svga = &permedia2->svga;
        uint8_t ret;

        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;

        switch (addr)
        {
                case 0x3c2:
                ret = svga_in(addr, svga);
                ret |= permedia2->vblank_irq > 0 ? 0x80 : 0x00;
                return ret;
                case 0x3c5:
                switch(svga->seqaddr)
                {
                    default:
                    break;
                }
                if (svga->seqaddr >= 0x08 && svga->seqaddr < 0x40)
                        return svga->seqregs[svga->seqaddr];
                break;

                case 0x3d4:
                return svga->crtcreg;
                case 0x3d5:
                return svga->crtc[svga->crtcreg];
        }
        return svga_in(addr, svga);
}

void permedia2_recalctimings(svga_t *svga)
{
        permedia2_t *permedia2 = (permedia2_t*)svga->priv;
        bool svga_mode = (svga->seqregs[0x05] & 0x08) != 0;
        int bpp = 8;

        if (svga_mode) {



        } else {
            // graphics processor mode
            svga->ma_latch = (permedia2->vc_regs[0 / 4] & 0xfffff) * 2;
            svga->rowoffset = permedia2->vc_regs[8 / 4] & 0xfffff;
            svga->htotal = (((permedia2->vc_regs[0x10 / 4] & 2047) - (permedia2->vc_regs[0x20 / 4] & 2047)) + 1) * 4;
            svga->vtotal = permedia2->vc_regs[0x38 / 4] & 2047;
            svga->vsyncstart = svga->vtotal;
            svga->dispend = svga->vtotal - (permedia2->vc_regs[0x40 / 4] & 2047) + 1;
            svga->hdisp = svga->htotal;
            svga->split = 99999;
            svga->vblankstart = svga->vtotal;

            svga->crtc[0x17] |= 0x80;
            svga->gdcreg[6] |= 1;

            svga->video_res_override = 1;

            switch(permedia2->ramdac_vals[0x18] & 15)
            {
                case 0:
                default:
                    bpp = 8;
                    break;
                case 4:
                    bpp = 15;
                    break;
                case 6:
                    bpp = 16;
                    break;
                case 8:
                    bpp = 32;
                    svga->hdisp /= 2;
                    break;
                case 9:
                    bpp = 24;
                    svga->hdisp *= 2;
                    svga->hdisp /= 3;
                    break;
            }
            svga->video_res_x = svga->hdisp;
            svga->video_res_y = svga->dispend;

            svga->bpp = bpp;
            svga->video_bpp = bpp;

            bool oswaprb = svga->swaprb;
            svga->swaprb = (permedia2->ramdac_vals[0x18] & 0x20) == 0;
            if (oswaprb != svga->swaprb) {
                void pcemvideorbswap(bool swapped);
                pcemvideorbswap(svga->swaprb);
            }
 
            svga->fullchange = changeframecount;

            // GP video enabled
            if (!(permedia2->vc_regs[0x58 / 4] & 1)) {
                bpp = 0;
            }
            svga->linedbl = 0;
            if (permedia2->vc_regs[0x58 / 4] & 4) {
                svga->linedbl = 1;
            }
        }


        switch (bpp)
        {
            default:
                svga->render = svga_render_blank;
                break;
            case 4:
                svga->render = svga_render_4bpp_highres;
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
            case 24:
                svga->render = svga_render_24bpp_highres;
                break;
            case 32:
                svga->render = svga_render_32bpp_highres;
                break;
        }

        svga->horizontal_linedbl = svga->dispend * 9 / 10 >= svga->hdisp;
}

static void permedia2_hwcursor_draw(svga_t *svga, int displine)
{
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;
    int addr = svga->hwcursor_latch.addr;
    int addradd = 0;
    uint8_t dat[2];
    int offset = svga->hwcursor_latch.x;
    uint8_t control = permedia2->ramdac_vals[6];
    int cmode = control & 3;

    offset <<= svga->horizontal_linedbl;

    if (svga->hwcursor.cur_xsize == 32) {
        addradd = ((control >> 4) & 3) * (32 * 32 / 8);
    }

    for (int x = 0; x < svga->hwcursor.cur_xsize; x += 8)
    {
        dat[0] = permedia2->ramdac_cram[addr + addradd];
        dat[1] = permedia2->ramdac_cram[addr + 0x200 + addradd];
        for (int xx = 0; xx < 8; xx++)
        {
            if (offset >= 0) {
                int cidx = 0;
                int comp = 0;
                int cval = ((dat[1] & 0x80) ? 2 : 0) | ((dat[0] & 0x80) ? 1 : 0);
                if (cmode == 1) {
                    cidx = cval;
                } else if (cmode == 2) {
                    if (cval == 0 || cval == 1) {
                        cidx = cval + 1;
                    } else if (cval == 3) {
                        comp = 1;
                    }
                } else {
                    cidx = cval - 1;
                }

                if (comp) {
                    ((uint32_t*)buffer32->line[displine])[offset + 32] ^= 0xffffff;
                } else if (cidx > 0) {
                    ((uint32_t*)buffer32->line[displine])[offset + 32] = permedia2->ramdac_hwc_col[cidx];
                }
            }

            offset++;
            dat[0] <<= 1;
            dat[1] <<= 1;
        }
        addr++;
    }
    svga->hwcursor_latch.addr += svga->hwcursor.cur_xsize / 8;

}


static void permedia2_write(uint32_t addr, uint8_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    addr = (addr & 0xffff) + permedia2->bank[(addr >> 15) & 3];

    addr &= svga->decode_mask;
    if (addr >= svga->vram_max)
        return;
    addr &= svga->vram_mask;
    svga->changedvram[addr >> 12] = changeframecount;
    *(uint8_t *)&svga->vram[addr] = val;
}
static void permedia2_writew(uint32_t addr, uint16_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    addr = (addr & 0xffff) + permedia2->bank[(addr >> 15) & 3];

    addr &= svga->decode_mask;
    if (addr >= svga->vram_max)
        return;
    addr &= svga->vram_mask;
    svga->changedvram[addr >> 12] = changeframecount;
    *(uint16_t *)&svga->vram[addr] = val;
}
static void permedia2_writel(uint32_t addr, uint32_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    addr = (addr & 0xffff) + permedia2->bank[(addr >> 15) & 3];

    addr &= svga->decode_mask;
    if (addr >= svga->vram_max)
        return;
    addr &= svga->vram_mask;
    svga->changedvram[addr >> 12] = changeframecount;
    *(uint32_t *)&svga->vram[addr] = val;
}

static uint8_t permedia2_read(uint32_t addr, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    addr = (addr & 0xffff) + permedia2->bank[(addr >> 15) & 3];

    addr &= svga->decode_mask;
    if (addr >= svga->vram_max)
        return 0xff;

    return *(uint8_t *)&svga->vram[addr & svga->vram_mask];
}
static uint16_t permedia2_readw(uint32_t addr, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    addr = (addr & 0xffff) + permedia2->bank[(addr >> 15) & 3];

    addr &= svga->decode_mask;
    if (addr >= svga->vram_max)
        return 0xffff;
    return *(uint16_t *)&svga->vram[addr & svga->vram_mask];
}
static uint32_t permedia2_readl(uint32_t addr, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    addr = (addr & 0xffff) + permedia2->bank[(addr >> 15) & 3];

    addr &= svga->decode_mask;
    if (addr >= svga->vram_max)
        return 0xffff;

    return *(uint32_t *)&svga->vram[addr & svga->vram_mask];
}

static void permedia2_ramdac_write(int reg, uint8_t v, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;

    if (reg == 0) { // palettewriteaddress
        permedia2->ramdac_reg = v;
        permedia2->ramdac_pal = v;
        permedia2->ramdac_palcomp = 0;
        permedia2->ramdac_cramaddr &= 0x0300;
        permedia2->ramdac_cramaddr |= v;
    } else if (reg == 8) { // palettedata
        int idx = permedia2->ramdac_pal;
        int comp = permedia2->ramdac_palcomp;
        if (!comp) {
            svga->vgapal[idx].r = v;
        } else if (comp == 1) {
            svga->vgapal[idx].g = v;
        } else {
            svga->vgapal[idx].b = v;
        }
        svga->pallook[idx] = makecol32(svga->vgapal[idx].r, svga->vgapal[idx].g, svga->vgapal[idx].b);
        permedia2->ramdac_palcomp++;
        if (permedia2->ramdac_palcomp >= 3) {
            permedia2->ramdac_palcomp = 0;
            permedia2->ramdac_pal++;
        }
    } else if (reg == 0x20) { // cursorcoloraddress
        permedia2->ramdac_cpal = v;
        permedia2->ramdac_cpalcomp = 0;
    } else if (reg == 0x28) { // cursorcolordata
        int idx = permedia2->ramdac_cpal;
        int comp = permedia2->ramdac_cpalcomp;
        if (!comp) {
            permedia2->ramdac_hwc_col[idx] &= 0x00ffff;
            permedia2->ramdac_hwc_col[idx] |= v << 16;
        } else if (comp == 1) {
            permedia2->ramdac_hwc_col[idx] &= 0xff00ff;
            permedia2->ramdac_hwc_col[idx] |= v << 8;
        } else {
            permedia2->ramdac_hwc_col[idx] &= 0xffff00;
            permedia2->ramdac_hwc_col[idx] |= v << 0;
        }
        permedia2->ramdac_cpalcomp++;
        if (permedia2->ramdac_cpalcomp >= 3) {
            permedia2->ramdac_cpalcomp = 0;
            permedia2->ramdac_cpal++;
            permedia2->ramdac_cpal &= 3;
        }
    } else if (reg == 0x50) { // indexeddata
        permedia2->ramdac_vals[permedia2->ramdac_reg] = v;
        switch (permedia2->ramdac_reg)
        {
            case 0x06: // cursorcontrol
            svga->hwcursor.ena = (v & 3) != 0;
            svga->hwcursor.cur_ysize = svga->hwcursor.cur_xsize = (v & 0x40) ? 64 : 32;
            permedia2->ramdac_cramaddr &= 0x00ff;
            permedia2->ramdac_cramaddr |= ((v >> 2) & 3) << 8;
            break;
            case 0x18: // colormode
            svga_recalctimings(&permedia2->svga);
            break;
            case 0x1e: // misccontrol
            svga_set_ramdac_type(&permedia2->svga, (v & 2) ? RAMDAC_8BIT : RAMDAC_6BIT);
            break;
        }
    } else if (reg == 0x58) { // cursorramdata
        permedia2->ramdac_cram[permedia2->ramdac_cramaddr] = v;
        permedia2->ramdac_cramaddr++;
        permedia2->ramdac_cramaddr &= 1023;
    } else if (reg == 0x60) { // cursorxlow
        svga->hwcursor.x += 64;
        svga->hwcursor.x &= 0xff00;
        svga->hwcursor.x |= v;
        svga->hwcursor.x -= 64;
    } else if (reg == 0x68) { // cursorxhigh
        svga->hwcursor.x += 64;
        svga->hwcursor.x &= 0x00ff;
        svga->hwcursor.x |= v << 8;
        svga->hwcursor.x -= 64;
    } else if (reg == 0x70) { // cursorylow
        svga->hwcursor.y += 64;
        svga->hwcursor.y &= 0xff00;
        svga->hwcursor.y |= v;
        svga->hwcursor.y -= 64;
    } else if (reg == 0x78) { // cursoryhigh
        svga->hwcursor.y += 64;
        svga->hwcursor.y &= 0x00ff;
        svga->hwcursor.y |= v << 8;
        svga->hwcursor.y -= 64;
    }
}

static uint32_t permedia2_ramdac_read(int reg, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;
    uint32_t v = 0;

    if (reg == 0) {
        // address register
        v = permedia2->ramdac_reg;
    } else if (reg == 0x18) { // palettereadaddress
        int idx = permedia2->ramdac_pal;
        int comp = permedia2->ramdac_palcomp;
        if (!comp) {
            v = svga->vgapal[idx].r;
        } else if (comp == 1) {
            v = svga->vgapal[idx].g;
        } else {
            v = svga->vgapal[idx].b;
        }
        permedia2->ramdac_palcomp++;
        if (permedia2->ramdac_palcomp >= 3) {
            permedia2->ramdac_palcomp = 0;
            permedia2->ramdac_pal++;
        }
    } else if (reg == 0x50) {
        // data register
        v = permedia2->ramdac_vals[permedia2->ramdac_reg];
        switch (permedia2->ramdac_reg)
        {
            case 0x29: // pixelclockstatus
                v = 0x10; // PLL locked
                break;
            case 0x33: // memoryclockstatus
                v = 0x10; // PLL locked
                break;
        }
    }

    return v;
}

static __inline int TOFRACT1215(uint32_t v)
{
    int vv = v & 0x7fffffe;
    if (v & 0x8000000) {
        vv = vv - 0x8000000;
    }
    return vv;
}
static __inline int TOFRACT1114(uint32_t v)
{
    int vv = v & 0x3fffffc;
    if (v & 0x4000000) {
        vv = vv - 0x4000000;
    }
    return vv;
}
static __inline int TOSIGN24(uint32_t v)
{
    int vv = v & 0x7fffff;
    if (v & 0x800000) {
        vv = vv - 0x800000;
    }
    return vv;
}
static __inline int TOSIGN12(uint32_t v)
{
    int vv = v & 0x7ff;
    if (v & 0x800) {
        vv = vv - 0x800;
    }
    return vv;
}
static __inline int TOSIGN3(uint32_t v)
{
    int vv = v & 0x3;
    if (v & 0x4) {
        vv = vv - 0x4;
    }
    return vv;
}
static __inline int FTOINT(int f)
{
    return f >> 16;
}

static void writefb(struct permedia2_t *permedia2, uint32_t dst, uint32_t color, int bpp)
{
    dst &= permedia2->svga.vram_mask;
    uint8_t *dstp = permedia2->svga.vram + dst;
    switch (bpp)
    {
        case 1:
            *((uint8_t*)dstp) = color;
            break;
        case 2:
            *((uint16_t*)dstp) = color;
            break;
        case 3:
            ((uint8_t*)dstp)[0] = color;
            ((uint8_t*)dstp)[1] = color >> 8;
            ((uint8_t*)dstp)[2] = color >> 16;
            break;
        case 4:
            *((uint32_t*)dstp) = color;
            break;
    }
    permedia2->svga.changedvram[dst >> 12] = changeframecount;
}
static uint32_t readfb(struct permedia2_t *permedia2, uint32_t dst, int bpp)
{
    dst &= permedia2->svga.vram_mask;
    uint8_t *dstp = permedia2->svga.vram + dst;
    uint32_t color;
    switch (bpp)
    {
        default:
        case 1:
            color = *((uint8_t*)dstp);
            break;
        case 2:
            color = *((uint16_t*)dstp);
            break;
        case 3:
            color = ((uint8_t*)dstp)[0];
            color |= ((uint8_t*)dstp)[1] << 8;
            color |= ((uint8_t*)dstp)[2] << 16;
            break;
        case 4:
            color = *((uint32_t*)dstp);
            break;
    }
    return color;
}

static uint32_t dorop(uint8_t rop, uint32_t s, uint32_t d)
{
    uint32_t out = 0;
    switch(rop)
    {
        case 0:
        default:
            out = 0x00000000;
            break;
        case 15:
            out = 0xffffffff;
            break;
        case 1:
            out = s & d;
            break;
        case 2:
            out = s & ~d;
            break;
        case 3:
            out = s;
            break;
        case 4:
            out = ~s & d;
            break;
        case 5:
            out = d;
            break;
        case 6:
            out = s ^ d;
            break;
        case 7:
            out = s | d;
            break;
        case 8:
            out = ~(s | d);
            break;
        case 9:
            out = ~(s ^ d);
            break;
        case 10:
            out = ~d;
            break;
        case 11:
            out = s | ~d;
            break;
        case 12:
            out = ~s;
            break;
        case 13:
            out = ~s | d;
            break;
        case 14:
            out = ~(s & d);
            break;           
    }
    return out;
}

static void do_stipple(permedia2_t *permedia2, int x, int y, uint32_t *cp, int *pxmode)
{
    int offx = (permedia2->gp_asxoffset + x) & 7;
    int offy = (permedia2->gp_asyoffset + y) & 7;
    if (permedia2->gp_asmirrorx) {
        offx = 7 - offx;
    }
    if (permedia2->gp_asmirrory) {
        offy = 7 - offy;
    }
    uint8_t mask = permedia2->gp_regs[(REG_AREASTIPPLEPATTERN0 >> 3) + offy];
    if (!((mask ^ permedia2->gp_asinvert) & (1 << offx))) {
        if (permedia2->gp_asforcebackgroundcolor) {
            *cp = permedia2->gp_regs[REG_TEXEL0 >> 3];
            *pxmode = 1;
        } else {
            *pxmode = -1;
        }
    }
}

static bool do_bitmask_check(permedia2_t *permedia2)
{
    if (permedia2->gp_bitmaskoffset > 31) {
        permedia2->gp_bitmaskoffset = 0;
        if (permedia2->gp_synconbitmask) {
            permedia2->gp_waitbitmask = true;
            return false;
        }
    }
    int offset = permedia2->gp_bitmaskoffset;
    if (permedia2->gp_mirrorbitmask) {
        offset = 31 - offset;
    }
    permedia2->gp_bmcheckresult = (permedia2->gp_bitmaskpattern & (1 << offset)) != 0;
    permedia2->gp_bitmaskoffset++;
    return true;
}
static void do_bitmask_do(permedia2_t *permedia2, uint32_t *cp, int *pxmode)
{
    if (!permedia2->gp_bmcheckresult) {
        if (permedia2->gp_bmforcebackgroundcolor) {
            *cp = permedia2->gp_regs[REG_TEXEL0 >> 3];
            *pxmode = 1;
        } else {
            *pxmode = -1;
        }
    }
}

static bool next_bitmask(permedia2_t *permedia2)
{
    if (permedia2->gp_bitmaskpacking && permedia2->gp_bitmaskoffset > 0) {
        permedia2->gp_bitmaskoffset = 0;
        if (permedia2->gp_synconbitmask) {
            permedia2->gp_waitbitmask = true;
            return false;
        }
    }
    return true;
}

static void do_blit_rect(permedia2_t *permedia2)
{
    if (permedia2->gp_waitbitmask) {
        return;
    }

    if (permedia2->gp_fastfill) {

        while (permedia2->gp_y >= permedia2->gp_y1 && permedia2->gp_y < permedia2->gp_y2) {
            int y = FTOINT(permedia2->gp_y);
            if (!permedia2->gp_scissor_user || (y >= permedia2->gp_scissor_min_y && y < permedia2->gp_scissor_max_y)) {
                uint32_t dp = permedia2->gp_dst + y * permedia2->gp_bytesperline;
                while (permedia2->gp_x >= permedia2->gp_x1 && permedia2->gp_x < permedia2->gp_x2) {
                    int x = FTOINT(permedia2->gp_x);
                    uint32_t ddp = dp + x * permedia2->gp_bpp;
                    // TODO: FBBlockColorL and FBBlockColorU
                    uint32_t c = permedia2->gp_regs[REG_FBBLOCKCOLOR >> 3];
                    int pxmode = 0;
                    if (permedia2->gp_asena) {
                        do_stipple(permedia2, x, y, &c, &pxmode);
                    }
                    if (permedia2->gp_bitmaskena) {
                        if (!do_bitmask_check(permedia2)) {
                            return;
                        }
                        if (pxmode >= 0) {
                            do_bitmask_do(permedia2, &c, &pxmode);
                        }
                    }
                    if (pxmode >= 0) {
                        if (!permedia2->gp_scissor_user || (x >= permedia2->gp_scissor_min_x && x < permedia2->gp_scissor_max_x)) {
                            if (permedia2->gp_dohwmask) {
                                uint32_t cd = readfb(permedia2, ddp, permedia2->gp_bpp);
                                c = (c & permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]) | (cd & ~permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]);
                            }
                            writefb(permedia2, ddp, c, permedia2->gp_bpp);
                        }
                    }
                    permedia2->gp_x += permedia2->gp_incx;
                }
            }
            permedia2->gp_x1 += permedia2->gp_dxdom;
            permedia2->gp_x2 += permedia2->gp_dxsub;
            permedia2->gp_x = permedia2->gp_incx > 0 ? permedia2->gp_x1 : permedia2->gp_x2 - FRACT_ONE;
            permedia2->gp_y += permedia2->gp_dy;
            if (permedia2->gp_bitmaskena) {
                if (!next_bitmask(permedia2)) {
                    return;
                }
            }
        }

    } else {

        while (permedia2->gp_y >= permedia2->gp_y1 && permedia2->gp_y < permedia2->gp_y2) {
            int y = FTOINT(permedia2->gp_y);
            if (!permedia2->gp_scissor_user || (y >= permedia2->gp_scissor_min_y && y < permedia2->gp_scissor_max_y)) {
                uint32_t dp = permedia2->gp_dst + y * permedia2->gp_bytesperline;
                uint32_t sp = permedia2->gp_src + y * permedia2->gp_bytesperline;
                while (permedia2->gp_x >= permedia2->gp_x1 && permedia2->gp_x < permedia2->gp_x2) {
                    int x = FTOINT(permedia2->gp_x);
                    uint32_t ddp = dp + x * permedia2->gp_bpp;
                    uint32_t ssp = sp + x * permedia2->gp_bpp;
                    uint32_t c = permedia2->gp_color, cd = permedia2->gp_color;
                    int pxmode = 0;
                    if (permedia2->gp_asena) {
                        do_stipple(permedia2, x, y, &c, &pxmode);
                    }
                    if (permedia2->gp_bitmaskena) {
                        if (!do_bitmask_check(permedia2)) {
                            return;
                        }
                        if (pxmode >= 0) {
                            do_bitmask_do(permedia2, &c, &pxmode);
                        }
                    }
                    if (x >= permedia2->gp_packedlimitxstart && x < permedia2->gp_packedlimitxend) {
                        if (permedia2->gp_readsrc && !pxmode) {
                            c = readfb(permedia2, ssp, permedia2->gp_bpp);
                        }
                        if (pxmode >=0) {
                            if (permedia2->gp_readdst) {
                                cd = readfb(permedia2, ddp, permedia2->gp_bpp);
                            }
                            if (permedia2->gp_ropena) {
                                c = dorop(permedia2->gp_rop, c, cd);
                            } else if (permedia2->gp_constfbdata) {
                                c = permedia2->gp_regs[REG_FBWRITEDATA >> 3];
                            }
                            if (permedia2->gp_writedst) {
                                if (!permedia2->gp_scissor_user || (x >= permedia2->gp_scissor_min_x && x < permedia2->gp_scissor_max_x)) {
                                    if (permedia2->gp_doswmask) {
                                        c = (c & permedia2->gp_regs[REG_FBSOFTWRITEMASK >> 3]) | (cd & ~permedia2->gp_regs[REG_FBSOFTWRITEMASK >> 3]);
                                    }
                                    if (permedia2->gp_dohwmask) {
                                        cd = readfb(permedia2, ddp, permedia2->gp_bpp);
                                        c = (c & permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]) | (cd & ~permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]);
                                    }
                                    writefb(permedia2, ddp, c, permedia2->gp_bpp);
                                }
                            }
                        }
                    }
                    permedia2->gp_x += permedia2->gp_incx;
                }
            }
            permedia2->gp_x1 += permedia2->gp_dxdom;
            permedia2->gp_x2 += permedia2->gp_dxsub;
            permedia2->gp_x = permedia2->gp_incx > 0 ? permedia2->gp_x1 : permedia2->gp_x2 - FRACT_ONE;
            permedia2->gp_y += permedia2->gp_dy;
            if (permedia2->gp_bitmaskena) {
                if (!next_bitmask(permedia2)) {
                    return;
                }
            }
        }
    }
}

static void do_blit_line(permedia2_t *permedia2)
{
    if (permedia2->gp_type != 0 && permedia2->gp_type != 1) {
        pclog("do_blit_line but type is not point or line!? was %d\n", permedia2->gp_type);
        return;
    }
    if (permedia2->gp_waitbitmask) {
        return;
    }
    while (permedia2->gp_len) {
        int x = FTOINT(permedia2->gp_x1);
        int y = FTOINT(permedia2->gp_y1);

        uint32_t d = permedia2->gp_dst + y * permedia2->gp_bytesperline + x * permedia2->gp_bpp;
        uint32_t s = permedia2->gp_src + y * permedia2->gp_bytesperline + x * permedia2->gp_bpp;

        if (!permedia2->gp_scissor_user || (y >= permedia2->gp_scissor_min_y && y < permedia2->gp_scissor_max_y)) {
            uint32_t c = permedia2->gp_color, cd = permedia2->gp_color;
            int pxmode = 0;
            if (permedia2->gp_asena) {
                do_stipple(permedia2, x, y, &c, &pxmode);
            }
            // is this allowed in line mode?
            if (permedia2->gp_bitmaskena) {
                if (!do_bitmask_check(permedia2)) {
                    return;
                }
                if (pxmode >= 0) {
                    do_bitmask_do(permedia2, &c, &pxmode);
                }
            }
            if (permedia2->gp_readsrc && !pxmode) {
                c = readfb(permedia2, s, permedia2->gp_bpp);
            }
            if (pxmode >= 0) {
                if (permedia2->gp_readdst) {
                    cd = readfb(permedia2, d, permedia2->gp_bpp);
                }
                if (permedia2->gp_ropena) {
                    c = dorop(permedia2->gp_rop, c, permedia2->gp_constfbdata ? permedia2->gp_regs[REG_FBWRITEDATA >> 3] : cd);
                }
                if (permedia2->gp_writedst) {
                    if (!permedia2->gp_scissor_user || (x >= permedia2->gp_scissor_min_x && x < permedia2->gp_scissor_max_x)) {
                        if (permedia2->gp_doswmask) {
                            c = (c & permedia2->gp_regs[REG_FBSOFTWRITEMASK >> 3]) | (cd & ~permedia2->gp_regs[REG_FBSOFTWRITEMASK >> 3]);
                        }
                        if (permedia2->gp_dohwmask) {
                            cd = readfb(permedia2, d, permedia2->gp_bpp);
                            c = (c & permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]) | (cd & ~permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]);
                        }
                        writefb(permedia2, d, c, permedia2->gp_bpp);
                    }
                }
            }
        }
        permedia2->gp_x1 += permedia2->gp_dx;
        permedia2->gp_y1 += permedia2->gp_dy;
        permedia2->gp_len--; 
    }
}

static void do_blit(permedia2_t *permedia2)
{
    switch(permedia2->gp_type)
    {
        case 0:
        do_blit_line(permedia2);
        break;
        case 3:
        do_blit_rect(permedia2);
        break;
    }
}

static void permedia2_blit(permedia2_t *permedia2)
{
    uint32_t cmd = permedia2->gp_regs[REG_RENDER >> 3];
    permedia2->gp_type = (cmd >> 6) & 3;
    permedia2->gp_astipple = (cmd >> 0) & 1;
    permedia2->gp_texena = (cmd >> 13) & 1;
    permedia2->gp_fogena = (cmd >> 14) & 1;
    permedia2->gp_fastfill = (cmd >> 3) & 1;
    permedia2->gp_synconbitmask = (cmd >> 11) & 1;
    permedia2->gp_reusebitmask = (cmd >> 17) & 1;

#if BLIT_LOG
    pclog("Permedia2 render %08x as=%d ff=%d type=%d te=%d fe=%d\n",
        cmd, permedia2->gp_astipple, permedia2->gp_fastfill,
        permedia2->gp_type, permedia2->gp_texena, permedia2->gp_fogena);
#endif
    int readpixel = (permedia2->gp_regs[REG_FBREADPIXEL >> 3] & 7);
    permedia2->gp_bpp = 1;
    switch(readpixel)
    {
        case 0:
        permedia2->gp_bpp = 1;
        break;
        case 1:
        permedia2->gp_bpp = 2;
        break;
        case 2:
        permedia2->gp_bpp = 4;
        break;
        case 4:
        permedia2->gp_bpp = 3;
        break;
    }

    uint32_t stipplemode = permedia2->gp_regs[REG_AREASTIPPLEMODE >> 3];
    permedia2->gp_asena = permedia2->gp_astipple && ((stipplemode >> 0) & 1);
    if (permedia2->gp_asena) {
        permedia2->gp_asxoffset = (stipplemode >> 7) & 7;
        permedia2->gp_asyoffset = (stipplemode >> 12) & 7;
        permedia2->gp_asinvert = ((stipplemode >> 17) & 1) ? 0xff : 0x00;
        permedia2->gp_asmirrorx = (stipplemode >> 18) & 1;
        permedia2->gp_asmirrory = (stipplemode >> 19) & 1;
        permedia2->gp_asforcebackgroundcolor = (stipplemode >> 20) & 1;
#if BLIT_LOG
        pclog("Areastipplemode: XOFF=%d YOFF=%d INV=%d MIRX=%d MIRY=%d FBG=%d\n",
            permedia2->gp_asxoffset, permedia2->gp_asyoffset, permedia2->gp_asinvert,
            permedia2->gp_asmirrorx, permedia2->gp_asmirrory, permedia2->gp_asforcebackgroundcolor);
#endif
    }

    uint32_t rasterizer = permedia2->gp_regs[REG_RASTERIZERMODE >> 3];
    permedia2->gp_bitmaskrelative = (rasterizer >> 19) & 1;
    permedia2->gp_bitmaskoffset = (rasterizer >> 10) & 31;
    permedia2->gp_bitmaskpacking = (rasterizer >> 9) & 1;
    permedia2->gp_bmforcebackgroundcolor = (rasterizer >> 6) & 1;
    permedia2->gp_mirrorbitmask = (rasterizer >> 0) & 1;
    permedia2->gp_bitmaskpattern = permedia2->gp_regs[REG_BITMASKPATTERN >> 3];
    if ((rasterizer >> 1) & 1) {
        permedia2->gp_bitmaskpattern ^= 0xffffffff;
    }
    permedia2->gp_waitbitmask = permedia2->gp_synconbitmask;
    permedia2->gp_bitmaskpatterncnt = 0;
    permedia2->gp_bitmaskena = permedia2->gp_synconbitmask || permedia2->gp_reusebitmask;
#if BLIT_LOG
    if (permedia2->gp_bitmaskena) {
        pclog("Bitmaskpattern: REL=%d OFF=%d PACK=%d FBG=%d MIRROR=%d\n",
            permedia2->gp_bitmaskrelative, permedia2->gp_bitmaskoffset, permedia2->gp_bitmaskpacking,
            permedia2->gp_bmforcebackgroundcolor, permedia2->gp_mirrorbitmask);
    }
#endif

    uint32_t readmode = permedia2->gp_regs[REG_FBREADMODE >> 3];
    permedia2->gp_readsrc = ((readmode >> 9) & 1) != 0;
    permedia2->gp_readdst = ((readmode >> 10) & 1) != 0;
    permedia2->gp_packeddata = ((readmode >> 19) & 1) != 0;
    bool gp_origin = ((readmode >> 16) & 1) != 0;
    permedia2->gp_writedst = (permedia2->gp_regs[REG_FBWRITEMODE >> 3] & 1) != 0;
    permedia2->gp_colordda = (permedia2->gp_regs[REG_COLORDDAMODE >> 3] & 1) != 0;
#if BLIT_LOG
    pclog("FBREADMODE: RSRC=%d RDST=%d WDST=%d PACKED=%d ORIGIN=%d CDDA=%d\n",
        permedia2->gp_readsrc, permedia2->gp_readdst, permedia2->gp_writedst,
        permedia2->gp_packeddata, gp_origin, permedia2->gp_colordda);
#endif

    uint32_t scissormode = permedia2->gp_regs[REG_SCISSORMODE >> 3];
    permedia2->gp_scissor_user = (scissormode & 1) != 0;
    permedia2->gp_scissor_screen = (scissormode & 2) != 0;
    permedia2->gp_scissor_min_x = (permedia2->gp_regs[REG_SCISSORMINXY >> 3] >> 0) & 0xfff;
    permedia2->gp_scissor_min_y = (permedia2->gp_regs[REG_SCISSORMINXY >> 3] >> 16) & 0xfff;
    permedia2->gp_scissor_max_x = (permedia2->gp_regs[REG_SCISSORMAXXY >> 3] >> 0) & 0xfff;
    permedia2->gp_scissor_max_y = (permedia2->gp_regs[REG_SCISSORMAXXY >> 3] >> 16) & 0xfff;

#if BLIT_LOG
    if (permedia2->gp_scissor_user) {
        pclog("SCISSOR: %d-%d %d-%d\n",
            permedia2->gp_scissor_min_x, permedia2->gp_scissor_min_y,
            permedia2->gp_scissor_max_x, permedia2->gp_scissor_max_y);
    }
#endif

    permedia2->gp_doswmask = (permedia2->gp_regs[REG_FBSOFTWRITEMASK >> 3] != 0xffffffff) && permedia2->gp_readdst;
    permedia2->gp_dohwmask = (permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3] != 0xffffffff);
    permedia2->gp_rop = permedia2->gp_regs[REG_LOGICALOPMODE >> 3] >> 1 & 15;
    permedia2->gp_ropena = (permedia2->gp_regs[REG_LOGICALOPMODE >> 3] & 1) != 0;
    permedia2->gp_constfbdata = ((permedia2->gp_regs[REG_LOGICALOPMODE >> 3] >> 5) & 1) != 0;
    int gp_srcoffset = TOSIGN24(permedia2->gp_regs[REG_FBSOURCEOFFSET >> 3]);
    int gp_pixeloffset = TOSIGN24(permedia2->gp_regs[REG_FBPIXELOFFSET >> 3]);
    int gp_packedlimitrel = 0;

#if BLIT_LOG
    pclog("SWMASK: %d %08x HWMASK %d %08x\n",
        permedia2->gp_doswmask, permedia2->gp_regs[REG_FBSOFTWRITEMASK >> 3],
        permedia2->gp_dohwmask, permedia2->gp_regs[REG_FBHARDWRITEMASK >> 3]);
#endif

    if (permedia2->gp_fastfill) {
        permedia2->gp_asforcebackgroundcolor = false;
        permedia2->gp_bmforcebackgroundcolor = false;
    }

    permedia2->gp_dst = (permedia2->gp_regs[REG_FBWINDOWBASE >> 3] + gp_pixeloffset) * permedia2->gp_bpp;
    permedia2->gp_src = (permedia2->gp_regs[REG_FBSOURCEBASE >> 3] + gp_pixeloffset + gp_srcoffset - gp_packedlimitrel) * permedia2->gp_bpp;

    permedia2->gp_bytesperline = permedia2->gp_pitch * permedia2->gp_bpp;
    permedia2->gp_color = permedia2->gp_colordda ? permedia2->gp_regs[REG_CONSTANTCOLOR >> 3] : permedia2->gp_regs[REG_FBSOURCEDATA >> 3];

    if (permedia2->gp_type == 0) {

        // LINE
        permedia2->gp_x1 = TOFRACT1215(permedia2->gp_regs[REG_STARTXDOM >> 3]);
        permedia2->gp_y1 = TOFRACT1215(permedia2->gp_regs[REG_STARTY >> 3]);
        permedia2->gp_dx = TOFRACT1215(permedia2->gp_regs[REG_DXDOM >> 3]);
        permedia2->gp_dy = TOFRACT1215(permedia2->gp_regs[REG_DY >> 3]);
        permedia2->gp_len = permedia2->gp_regs[REG_COUNT >> 3] & 0xfff;
        do_blit_line(permedia2);
        return;

    } else if (permedia2->gp_type == 2) {

        // POINT
        permedia2->gp_x1 = TOFRACT1215(permedia2->gp_regs[REG_STARTXDOM >> 3]);
        permedia2->gp_y1 = TOFRACT1215(permedia2->gp_regs[REG_STARTY >> 3]);
        permedia2->gp_len = 1;
        do_blit_line(permedia2);
        return;
    
    } else if (permedia2->gp_type == 1) {

        // TRAPEZOID
        permedia2->gp_x1 = TOFRACT1215(permedia2->gp_regs[REG_STARTXDOM >> 3]);
        permedia2->gp_y1 = TOFRACT1215(permedia2->gp_regs[REG_STARTY >> 3]);
        permedia2->gp_x2 = TOFRACT1215(permedia2->gp_regs[REG_STARTXSUB >> 3]);
        permedia2->gp_dxdom = TOFRACT1215(permedia2->gp_regs[REG_DXDOM >> 3]);
        permedia2->gp_dxsub = TOFRACT1215(permedia2->gp_regs[REG_DXSUB >> 3]);
        permedia2->gp_dy = TOFRACT1114(permedia2->gp_regs[REG_DY >> 3]);
        permedia2->gp_len = permedia2->gp_regs[REG_COUNT >> 3] & 0xfff;

        if (permedia2->gp_x2 >= permedia2->gp_x1) {
            permedia2->gp_incx = FRACT_ONE;
        } else {
            permedia2->gp_incx = -FRACT_ONE;
        }
        permedia2->gp_incy = permedia2->gp_dy < 0 ? -FRACT_ONE : FRACT_ONE;
        permedia2->gp_y2 = permedia2->gp_y1 + (permedia2->gp_len * FRACT_ONE);
        permedia2->gp_x = permedia2->gp_x1;
        permedia2->gp_y = permedia2->gp_y1;

        permedia2->gp_type = 3;

    } else {

        // RECTANGLE
        permedia2->gp_x1 = TOSIGN12(permedia2->gp_regs[REG_RECTANGLEORIGIN >> 3] >> 0);
        permedia2->gp_y1 = TOSIGN12(permedia2->gp_regs[REG_RECTANGLEORIGIN >> 3] >> 16);
        int w = TOSIGN12(permedia2->gp_regs[REG_RECTANGLESIZE >> 3] >> 0);
        int h = TOSIGN12(permedia2->gp_regs[REG_RECTANGLESIZE >> 3] >> 16);

        if (permedia2->gp_packeddata) {
            permedia2->gp_packedlimitxstart = TOSIGN12(permedia2->gp_regs[REG_PACKEDDATALIMITS >> 3] >> 16);
            permedia2->gp_packedlimitxend = TOSIGN12(permedia2->gp_regs[REG_PACKEDDATALIMITS >> 3] >> 0);
            gp_packedlimitrel = TOSIGN3(permedia2->gp_regs[REG_PACKEDDATALIMITS >> 3] >> 29);
            // "undo" packed data mode width modification. Not worth the trouble to really use packed mode.
            if (permedia2->gp_bpp == 1) {
                permedia2->gp_x1 <<= 2;
                w <<= 2;
            } else if (permedia2->gp_bpp == 2) {
                permedia2->gp_x1 <<= 1;
                w <<= 1;
            }
            permedia2->gp_src -= gp_packedlimitrel * permedia2->gp_bpp;
        } else {
            permedia2->gp_packedlimitxstart = -2048;
            permedia2->gp_packedlimitxend = 2047;
        }

        permedia2->gp_x2 = permedia2->gp_x1 + w;
        permedia2->gp_y2 = permedia2->gp_y1 + h;

        permedia2->gp_x1 <<= 16;
        permedia2->gp_x2 <<= 16;
        permedia2->gp_y1 <<= 16;
        permedia2->gp_y2 <<= 16;

        permedia2->gp_incx = ((cmd >> 21) & 1) ? FRACT_ONE : -FRACT_ONE;
        permedia2->gp_incy = ((cmd >> 22) & 1) ? FRACT_ONE : -FRACT_ONE;

        permedia2->gp_x = permedia2->gp_x1;
        if (permedia2->gp_incx < 0) {
            permedia2->gp_x = permedia2->gp_x2 - FRACT_ONE;
        }
        permedia2->gp_y = permedia2->gp_y1;
        permedia2->gp_dy = FRACT_ONE;
        if (permedia2->gp_incy < 0) {
            permedia2->gp_y = permedia2->gp_y2 - FRACT_ONE;
            permedia2->gp_dy = -FRACT_ONE;
        }

        permedia2->gp_dxdom = 0;
        permedia2->gp_dxsub = 0;
    }


    if (permedia2->gp_bitmaskrelative) {
        permedia2->gp_bitmaskoffset = (permedia2->gp_x >> 16) & 31;
    }

#if BLIT_LOG
    pclog("X1=%d Y1=%d X2=%d Y2=%d BPP=%d PITCH=%d SOFF=%08x ROP=%02X SRC=%08x DST=%08x\n",
        permedia2->gp_x1, permedia2->gp_y1, permedia2->gp_x2, permedia2->gp_y2, permedia2->gp_bpp,
        permedia2->gp_pitch, gp_srcoffset, permedia2->gp_rop,
        permedia2->gp_src, permedia2->gp_dst);
#endif

    do_blit(permedia2);
}

static void permedia2_mmio_write(uint32_t addr, uint8_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t*)p;
    pclog("PM2 MMIO WRITEB %08x -> %02x\n", addr, val);
}

static void permedia2_mmio_write_w(uint32_t addr, uint16_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t*)p;
    pclog("PM2 MMIO WRITEW %08x -> %04x\n", addr, val);
}

static void permedia2_mmio_write_l(uint32_t addr, uint32_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t*)p;
    int reg = addr & 0xffff;

    if (addr & 0x10000) {
        val = do_byteswap_32(val);
    }

    //pclog("PM2 MMIO Write %08x = %08x\n", addr, val);

    if (reg >= 0x8000 && reg < 0xa000) {
#if BLIT_LOG > 1
        pclog("PM2 GP MMIO Write %08x = %08x\n", addr, val);
#endif
        reg &= 0x7fff;
        int tag = reg >> 3;
        permedia2->gp_regs[tag] = val;
        if (reg == REG_RENDER) {
            permedia2_blit(permedia2);
        } else if (reg == REG_FBSOURCEDELTA) {
            int x = TOSIGN12(val >> 0);
            int y = TOSIGN12(val >> 16);
            int offset = y * permedia2->gp_pitch + x;
            offset += permedia2->gp_regs[REG_FBSOURCEBASE >> 3] & 0xffffff;
            offset -= permedia2->gp_regs[REG_FBWINDOWBASE >> 3] & 0xffffff;
            permedia2->gp_regs[REG_FBSOURCEOFFSET >> 3] = offset;
        } else if (reg == REG_FBWINDOWBASE) {
            permedia2->gp_regs[REG_FBSOURCEBASE >> 3] = val & 0xffffff;
        } else if (reg == REG_CONFIG) {
            int config = val;
            // TODO

        } else if (reg == REG_FBREADMODE) {
            int readmode = val;
            int ppv0 = (readmode >> 0) & 7;
            int ppv1 = (readmode >> 3) & 7;
            int ppv2 = (readmode >> 6) & 7;
            int pp0 = ppv0 ? 1 << (ppv0 - 1) : 0;
            int pp1 = ppv1 ? 1 << (ppv1 - 1) : 0;
            int pp2 = ppv2 ? 1 << (ppv2 - 1) : 0;
            permedia2->gp_pitch = (pp0 + pp1 + pp2) * 32;
        } else if (reg == REG_BITMASKPATTERN) {
            permedia2->gp_bitmaskpattern = val;
            uint32_t rasterizer = permedia2->gp_regs[REG_RASTERIZERMODE >> 3];
#if BLIT_LOG > 1
            pclog("REG_BITMASKPATTERN %08x %d\n", permedia2->gp_bitmaskpattern, permedia2->gp_bitmaskpatterncnt++);
#endif
            if ((rasterizer >> 1) & 1) {
                permedia2->gp_bitmaskpattern ^= 0xffffffff;
            }
            if (permedia2->gp_waitbitmask) {
                permedia2->gp_waitbitmask = false;
                do_blit(permedia2);
            } else {
                pclog("REG_BITMASKPATTERN when not waiting!?\n");
            }
        } else if (reg == REG_CONTINUENEWLINE) {
            uint32_t rasterizer = permedia2->gp_regs[REG_RASTERIZERMODE >> 3];
            permedia2->gp_len = val & 0xfff;
            // FractionAdjust
            switch ((rasterizer >> 2) & 3)
            {
                case 1:
                permedia2->gp_x &= 0xffff0000;
                permedia2->gp_y &= 0xffff0000;
                break;
                case 2:
                permedia2->gp_x &= 0xffff0000;
                permedia2->gp_y &= 0xffff0000;
                permedia2->gp_x |= 0x00008000;
                permedia2->gp_y |= 0x00008000;
                break;
                case 3:
                permedia2->gp_x &= 0xffff0000;
                permedia2->gp_y &= 0xffff0000;
                permedia2->gp_x |= 0x00007fff;
                permedia2->gp_y |= 0x00007fff;
                break;
            }
            do_blit(permedia2);
        } else if (reg == REG_CONTINUENEWSUB) {
            pclog("unimplemented\n");
        } else if (reg == REG_SYNC) {
            permedia2->gp_outfifodata = tag;
            permedia2->gp_outfifocnt = 1;
        }
    } else if (reg >= 0x4000 && reg < 0x5000) {
        permedia2_ramdac_write(reg & 0xff, val, p);
    } else if (reg >= 0x3000 && reg < 0x4000) {
        int vcreg = reg & 0xff;
        permedia2->vc_regs[vcreg / 4] = val;
        svga_recalctimings(&permedia2->svga);
    } else if (reg < 0x2000) {
        switch(reg)
        {
            case 0x08:
            permedia2->intena = val;
            break;
            case 0x10:
            if (val & 0x10) {
                permedia2->vblank_irq = 0;
            }
            permedia2->intreq &= ~val;
            break;
            case 0x50:
            //pclog("Aperture 1: %02x\n", val);
            permedia2->linear_byte_control[0] = val & 3;
            break;
            case 0x58:
            //pclog("Aperture 2: %02x\n", val);
            permedia2->linear_byte_control[1] = val & 3;
            break;
        }
        //pclog("control write %08x = %08x\n", addr, val);
    } else {
        pclog("Unsupported PM2 MMIO Write %08x = %08x\n", addr, val);
    }
}

static uint32_t permedia2_mmio_readl(uint32_t addr, void *p)
{
    permedia2_t *permedia2 = (permedia2_t*)p;
    uint32_t v = 0;
    int reg = (addr & 0xffff);

    if (reg >= 0x8000 && reg < 0xa000) {
        int tag = reg >> 3;
        v = permedia2->gp_regs[tag];
    } else if (reg >= 0x4000 && reg < 0x5000) {
        v = permedia2_ramdac_read(reg & 0xff, p);
    } else if (reg >= 0x2000 && reg < 0x3000) {
        v = permedia2->gp_outfifodata;
        permedia2->gp_outfifocnt = 0;
    } else if (reg >= 0x3000 && reg < 0x4000) {
        int vcreg = reg & 0xff;
        v = permedia2->vc_regs[vcreg / 4];
        switch (vcreg)
        {
            case 0x70: // LineCount
                v = permedia2->svga.vc;
            break;
        }
    } else if (reg < 0x2000) {
        switch (reg)
        {
            case 0x08:
            v = permedia2->intena;
            break;
            case 0x10:
            v = permedia2->intreq;
            break;
            case 0x18:
            v = 0x100;
            break;
            case 0x20:
            v = permedia2->gp_outfifocnt;
            break;
        }
        //pclog("control read %08x = %08x\n", addr, v);
    } else {
        pclog("Unsupported PM2 MMIO Read %08x\n", addr);
    }
    if (addr & 0x10000) {
        v = do_byteswap_32(v);
    }
    return v;
}

static uint16_t permedia2_mmio_readw(uint32_t addr, void *p)
{
    uint32_t v = permedia2_mmio_readl(addr, p);
    if (addr & 2) {
        v >>= 16;
    }
    return v;
}
static uint8_t permedia2_mmio_read(uint32_t addr, void *p)
{
    uint32_t v = permedia2_mmio_readl(addr, p);
    if (addr & 2) {
        v >>= 16;
    }
    if (addr & 1) {
        v >>= 8;
    }
    return v;
}

static uint8_t permedia2_read_linear1(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t*)p;
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;

    uint8_t *fbp = (uint8_t*)(&svga->vram[addr & svga->vram_mask]);
    uint8_t v = *fbp;
    return v;
}
static uint16_t permedia2_readw_linear1(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t*)p;
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;

    uint16_t *fbp = (uint16_t*)(&svga->vram[addr & svga->vram_mask]);
    uint16_t v = *fbp;

    if (permedia2->linear_byte_control[0] == 2) {
        v = (v >> 8) | (v << 8);
    }

    return v;
}
static uint32_t permedia2_readl_linear1(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t*)p;
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;

    uint32_t *fbp = (uint32_t*)(&svga->vram[addr & svga->vram_mask]);
    uint32_t v = *fbp;

    if (permedia2->linear_byte_control[0] == 2) {
        v = do_byteswap_32(v);
        v = (v >> 16) | (v << 16);
    } else if (permedia2->linear_byte_control[0] == 1) {
        v = do_byteswap_32(v);
    }

    return v;
}

static uint8_t permedia2_read_linear2(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t *)p;
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;

    uint8_t *fbp = (uint8_t *)(&svga->vram[addr & svga->vram_mask]);
    uint8_t v = *fbp;
    return v;
}
static uint16_t permedia2_readw_linear2(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t *)p;
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;

    uint16_t *fbp = (uint16_t *)(&svga->vram[addr & svga->vram_mask]);
    uint16_t v = *fbp;

    if (permedia2->linear_byte_control[1] == 2) {
        v = (v >> 8) | (v << 8);
    }

    return v;
}
static uint32_t permedia2_readl_linear2(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t *)p;
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;

    uint32_t *fbp = (uint32_t *)(&svga->vram[addr & svga->vram_mask]);
    uint32_t v = *fbp;

    if (permedia2->linear_byte_control[1] == 2) {
        v = do_byteswap_32(v);
        v = (v >> 16) | (v << 16);
    } else if (permedia2->linear_byte_control[1] == 1) {
        v = do_byteswap_32(v);
    }

    return v;
}

static void permedia2_write_linear1(uint32_t addr, uint8_t val, void *p)
{
    svga_t *svga = (svga_t*)p;
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;

    addr &= svga->vram_mask;
    uint8_t *fbp = (uint8_t*)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}
static void permedia2_writew_linear1(uint32_t addr, uint16_t val, void *p)
{
    svga_t *svga = (svga_t*)p;
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;

    if (permedia2->linear_byte_control[0] == 2) {
        val = (val >> 8) | (val << 8);
    }

    addr &= svga->vram_mask;
    uint16_t *fbp = (uint16_t *)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}
static void permedia2_writel_linear1(uint32_t addr, uint32_t val, void *p)
{
    svga_t *svga = (svga_t*)p;
    permedia2_t *permedia2 = (permedia2_t*)svga->priv;

    if (permedia2->linear_byte_control[0] == 2) {
        val = (val >> 16) | (val << 16);
        val = do_byteswap_32(val);
    } else if (permedia2->linear_byte_control[0] == 1) {
        val = do_byteswap_32(val);
    }

    addr &= svga->vram_mask;
    uint32_t *fbp = (uint32_t*)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}

static void permedia2_write_linear2(uint32_t addr, uint8_t val, void *p)
{
    svga_t *svga = (svga_t *)p;
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;

    addr &= svga->vram_mask;
    uint8_t *fbp = (uint8_t *)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}
static void permedia2_writew_linear2(uint32_t addr, uint16_t val, void *p)
{
    svga_t *svga = (svga_t *)p;
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;

    if (permedia2->linear_byte_control[1] == 2) {
        val = (val >> 8) | (val << 8);
    }

    addr &= svga->vram_mask;
    uint16_t *fbp = (uint16_t *)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}
static void permedia2_writel_linear2(uint32_t addr, uint32_t val, void *p)
{
    svga_t *svga = (svga_t *)p;
    permedia2_t *permedia2 = (permedia2_t *)svga->priv;

    if (permedia2->linear_byte_control[1] == 2) {
        val = (val >> 16) | (val << 16);
        val = do_byteswap_32(val);
    } else if (permedia2->linear_byte_control[1] == 1) {
        val = do_byteswap_32(val);
    }

    addr &= svga->vram_mask;
    uint32_t *fbp = (uint32_t *)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}

void permedia2_updatebanking(permedia2_t *permedia2)
{
    svga_t *svga = &permedia2->svga;

    svga->banked_mask = 0xffff;
}

void permedia2_updatemapping(permedia2_t *permedia2)
{
        svga_t *svga = &permedia2->svga;
        
        permedia2_updatebanking(permedia2);

        mem_mapping_disablex(&svga->mapping);
        mem_mapping_disablex(&permedia2->mmio_mapping);
        mem_mapping_disablex(&permedia2->linear_mapping[0]);
        mem_mapping_disablex(&permedia2->linear_mapping[1]);

        if (permedia2->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_MEM) {
            if (permedia2->linear_base[0]) {
                mem_mapping_set_addrx(&permedia2->linear_mapping[0], permedia2->linear_base[0], 0x00800000);
            }
            if (permedia2->linear_base[1]) {
                mem_mapping_set_addrx(&permedia2->linear_mapping[1], permedia2->linear_base[1], 0x00800000);
            }
            if (permedia2->mmio_base) {
                mem_mapping_set_addrx(&permedia2->mmio_mapping, permedia2->mmio_base, 0x20000);
            }
        }
}

static void permedia2_adjust_panning(svga_t *svga)
{
    int ar11 = svga->attrregs[0x13] & 7;
    int src = 0, dst = 8;

    switch (svga->bpp)
    {
        case 15:
        case 16:
            dst = (ar11 & 4) ? 7 : 8;
            break;
        case 24:
            src = ar11 >> 1;
            break;
        default:
            return;
    }

    dst += 24;
    svga->scrollcache_dst = dst;
    svga->scrollcache_src = src;

}

static inline uint32_t dword_remap(uint32_t in_addr)
{
    return in_addr;
}

static void permedia2_io_remove(permedia2_t *permedia2)
{
    io_removehandlerx(0x03c0, 0x0020, permedia2_in, NULL, NULL, permedia2_out, NULL, NULL, permedia2);
}
static void permedia2_io_set(permedia2_t *permedia2)
{
    permedia2_io_remove(permedia2);

    io_sethandlerx(0x03c0, 0x0020, permedia2_in, NULL, NULL, permedia2_out, NULL, NULL, permedia2);
}

uint8_t permedia2_pci_read(int func, int addr, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;
    switch (addr)
    {
        case 0x00: return 0x4c; /* 3DLabs */
        case 0x01: return 0x10;

        case 0x02: return permedia2->id_ext_pci;
        case 0x03: return 0x3d;

        case PCI_REG_COMMAND:
            return permedia2->pci_regs[PCI_REG_COMMAND]; /*Respond to IO and memory accesses*/

        case 0x07: return 1 << 1; /*Medium DEVSEL timing*/

        case 0x08: return 1; /*Revision ID*/
        case 0x09: return 0; /*Programming interface*/
        case 0x0a: return 0x80; /*Supports VGA interface*/
        case 0x0b: return 0x03;

        // Control space (MMIO)
        case 0x10: return 0x00;
        case 0x11: return 0x00;
        case 0x12: return (permedia2->mmio_base >> 16) & 0xfe;
        case 0x13: return permedia2->mmio_base >> 24;
        // Aperture 1
        case 0x14: return 0x00;
        case 0x15: return 0x00;
        case 0x16: return (permedia2->linear_base[0] >> 16) & 0x80;
        case 0x17: return permedia2->linear_base[0] >> 24;
        // Aperture 2
        case 0x18: return 0x00;
        case 0x19: return 0x00;
        case 0x1a: return (permedia2->linear_base[1] >> 16) & 0x80;
        case 0x1b: return permedia2->linear_base[1] >> 24;

        case 0x2c: return 0x4c;
        case 0x2d: return 0x10;
        case 0x2e: return permedia2->id_ext_pci;;
        case 0x2f: return 0x3d;

        case 0x30: return permedia2->pci_regs[0x30] & 0x01; /*BIOS ROM address*/
        case 0x31: return 0x00;
        case 0x32: return permedia2->pci_regs[0x32];
        case 0x33: return permedia2->pci_regs[0x33];

        case 0x3c: return permedia2->int_line;
        case 0x3d: return PCI_INTA;
    }
    return 0;
}

void permedia2_pci_write(int func, int addr, uint8_t val, void *p)
{
    permedia2_t *permedia2 = (permedia2_t *)p;
    svga_t *svga = &permedia2->svga;
    switch (addr)
    {
        case PCI_REG_COMMAND:
            permedia2->pci_regs[PCI_REG_COMMAND] = val & 0x23;
            if (val & PCI_COMMAND_IO)
                permedia2_io_set(permedia2);
            else
                permedia2_io_remove(permedia2);
            permedia2_updatemapping(permedia2);
            break;

        case 0x12:
            permedia2->mmio_base &= 0xff00ffff;
            permedia2->mmio_base |= (val & 0xfe) << 16;
            permedia2_updatemapping(permedia2);
            break;
        case 0x13:
            permedia2->mmio_base &= 0x00ffffff;
            permedia2->mmio_base |= val << 24;
            permedia2_updatemapping(permedia2);
            break;

        case 0x16:
            permedia2->linear_base[0] &= 0xff00ffff;
            permedia2->linear_base[0] |= (val & 0x80) << 16;
            permedia2_updatemapping(permedia2);
            break;
        case 0x17:
            permedia2->linear_base[0] &= 0x00ffffff;
            permedia2->linear_base[0] |= val << 24;
            permedia2_updatemapping(permedia2);
            break;

        case 0x1a:
            permedia2->linear_base[1] &= 0xff00ffff;
            permedia2->linear_base[1] |= (val & 0x80) << 16;
            permedia2_updatemapping(permedia2);
            break;
        case 0x1b:
            permedia2->linear_base[1] &= 0x00ffffff;
            permedia2->linear_base[1] |= val << 24;
            permedia2_updatemapping(permedia2);
            break;

        case 0x30: case 0x32: case 0x33:
            permedia2->pci_regs[addr] = val;
            if (permedia2->pci_regs[0x30] & 0x01)
            {
                uint32_t addr = (permedia2->pci_regs[0x32] << 16) | (permedia2->pci_regs[0x33] << 24);
                mem_mapping_set_addrx(&permedia2->bios_rom.mapping, addr, 0x8000);
            } else {
                mem_mapping_disablex(&permedia2->bios_rom.mapping);
            }
            return;

        case 0x3c:
            permedia2->int_line = val;
            return;
    }
}


static void *permedia2_init(char *bios_fn, int chip)
{
        permedia2_t *permedia2 = (permedia2_t*)calloc(sizeof(permedia2_t), 1);
        svga_t *svga = &permedia2->svga;
        int vram;
        uint32_t vram_size;
        
        memset(permedia2, 0, sizeof(permedia2_t));
        
        vram = device_get_config_int("memory");
        if (vram)
                vram_size = vram << 20;
        else
                vram_size = 512 << 10;
        permedia2->vram_mask = vram_size - 1;

        rom_init(&permedia2->bios_rom, bios_fn, 0xc0000, 0x8000, 0x7fff, 0, MEM_MAPPING_EXTERNAL);

        svga_init(NULL, &permedia2->svga, permedia2, vram_size,
            permedia2_recalctimings,
            permedia2_in, permedia2_out,
            permedia2_hwcursor_draw,
            NULL);

        mem_mapping_addx(&permedia2->linear_mapping[0], 0, 0, permedia2_read_linear1, permedia2_readw_linear1, permedia2_readl_linear1, permedia2_write_linear1, permedia2_writew_linear1, permedia2_writel_linear1, NULL, MEM_MAPPING_EXTERNAL, &permedia2->svga);
        mem_mapping_addx(&permedia2->linear_mapping[1], 0, 0, permedia2_read_linear2, permedia2_readw_linear2, permedia2_readl_linear2, permedia2_write_linear2, permedia2_writew_linear2, permedia2_writel_linear2, NULL, MEM_MAPPING_EXTERNAL, &permedia2->svga);
        mem_mapping_set_handlerx(&permedia2->svga.mapping, permedia2_read, permedia2_readw, permedia2_readl, permedia2_write, permedia2_writew, permedia2_writel);
        mem_mapping_addx(&permedia2->mmio_mapping, 0, 0, permedia2_mmio_read, permedia2_mmio_readw, permedia2_mmio_readl, permedia2_mmio_write, permedia2_mmio_write_w, permedia2_mmio_write_l, NULL, MEM_MAPPING_EXTERNAL, permedia2);
        mem_mapping_set_px(&permedia2->svga.mapping, permedia2);
        mem_mapping_disablex(&permedia2->mmio_mapping);

        svga->vram_max = 8 << 20;
        svga->decode_mask = svga->vram_max - 1;
        svga->vram_mask = svga->vram_max - 1;

        permedia2->pci_regs[4] = 3;
        permedia2->pci_regs[5] = 0;
        permedia2->pci_regs[6] = 0;
        permedia2->pci_regs[7] = 2;
        permedia2->pci_regs[0x32] = 0x0c;
        permedia2->pci_regs[0x3d] = 1;
        permedia2->pci_regs[0x3e] = 4;
        permedia2->pci_regs[0x3f] = 0xff;


        svga->vsync_callback = permedia2_vblank_start;
        svga->adjust_panning = permedia2_adjust_panning;

        permedia2->chip = chip;

        permedia2_updatemapping(permedia2);

        permedia2->card = pci_add(permedia2_pci_read, permedia2_pci_write, permedia2);

        return permedia2;
}

void *permedia2_init(const device_t *info)
{
    permedia2_t *permedia2 = (permedia2_t *)permedia2_init("permedia2.bin", 0);

    permedia2->svga.fb_only = -1;

    //permedia2->getclock = sdac_getclock;
    //permedia2->getclock_p = &permedia2->ramdac;
    permedia2->id_ext_pci = 0x07;
    sdac_init(&permedia2->ramdac);
    svga_set_ramdac_type(&permedia2->svga, RAMDAC_8BIT);
    svga_recalctimings(&permedia2->svga);

    return permedia2;
}

void permedia2_close(void *p)
{
        permedia2_t *permedia2 = (permedia2_t*)p;

        svga_close(&permedia2->svga);
        
        free(permedia2);
}

void permedia2_speed_changed(void *p)
{
        permedia2_t *permedia2 = (permedia2_t *)p;
        
        svga_recalctimings(&permedia2->svga);
}

void permedia2_force_redraw(void *p)
{
        permedia2_t *permedia2 = (permedia2_t *)p;

        permedia2->svga.fullchange = changeframecount;
}

void permedia2_add_status_info(char *s, int max_len, void *p)
{
        permedia2_t *permedia2 = (permedia2_t *)p;
        uint64_t new_time = timer_read();
    
        svga_add_status_info(s, max_len, &permedia2->svga);
}

device_t permedia2_device =
{
    "Permedia 2", NULL,
    0, 0,
    permedia2_init,
    permedia2_close,
    NULL,
    NULL,
    permedia2_speed_changed,
    permedia2_force_redraw,
    NULL
};
