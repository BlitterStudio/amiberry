#include <stdlib.h>
#include "ibm.h"
#include "device.h"
#include "io.h"
#include "mem.h"
#include "pci.h"
#include "rom.h"
#include "thread.h"
#include "video.h"
#include "vid_ddc.h"
#include "vid_svga.h"
#include "vid_svga_render.h"
#include "vid_voodoo_banshee.h"
#include "vid_voodoo_common.h"
#include "vid_voodoo_display.h"
#include "vid_voodoo_fifo.h"
#include "vid_voodoo_regs.h"
#include "vid_voodoo_render.h"
#include "x86.h"

#ifdef CLAMP
#undef CLAMP
#endif

static uint8_t vb_filter_v1_rb[256][256];
static uint8_t vb_filter_v1_g [256][256];

static uint8_t vb_filter_bx_rb[256][256];
static uint8_t vb_filter_bx_g [256][256];

enum
{
        TYPE_BANSHEE = 0,
        TYPE_V3_2000,
        TYPE_V3_3000
};

typedef struct banshee_t
{
        svga_t svga;
        
        rom_t bios_rom;
        
        uint8_t pci_regs[256];
        
        uint32_t memBaseAddr0;
        uint32_t memBaseAddr1;
        uint32_t ioBaseAddr;

        uint32_t agpInit0;
        uint32_t dramInit0, dramInit1;
        uint32_t lfbMemoryConfig;
        uint32_t miscInit0, miscInit1;
        uint32_t pciInit0;
        uint32_t vgaInit0, vgaInit1;
        
        uint32_t command_2d;
        uint32_t srcBaseAddr_2d;
        
        uint32_t pllCtrl0, pllCtrl1, pllCtrl2;
        
        uint32_t dacMode;
        int dacAddr;

        uint32_t vidDesktopOverlayStride;
        uint32_t vidDesktopStartAddr;
        uint32_t vidProcCfg;
        uint32_t vidScreenSize;
        uint32_t vidSerialParallelPort;
        
        int overlay_pix_fmt;
        
        uint32_t hwCurPatAddr, hwCurLoc, hwCurC0, hwCurC1;

        uint32_t intrCtrl;
        
        uint32_t overlay_buffer[2][4096];

        mem_mapping_t linear_mapping;

        mem_mapping_t reg_mapping_low;  /*0000000-07fffff*/
        mem_mapping_t reg_mapping_high; /*0c00000-1ffffff - Windows 2000 puts the BIOS ROM in between these two areas*/
        
        voodoo_t *voodoo;
        
        uint32_t desktop_addr;
        int desktop_y;
        uint32_t desktop_stride_tiled;

        int type;

        int vblank_irq;
} banshee_t;

enum
{
        Init_status     = 0x00,
        Init_pciInit0   = 0x04,
        Init_lfbMemoryConfig = 0x0c,
        Init_miscInit0  = 0x10,
        Init_miscInit1  = 0x14,
        Init_dramInit0  = 0x18,
        Init_dramInit1  = 0x1c,
        Init_agpInit0   = 0x20,
        Init_vgaInit0   = 0x28,
        Init_vgaInit1   = 0x2c,
        Init_2dCommand     = 0x30,
        Init_2dSrcBaseAddr = 0x34,
        Init_strapInfo  = 0x38,
        
        PLL_pllCtrl0    = 0x40,
        PLL_pllCtrl1    = 0x44,
        PLL_pllCtrl2    = 0x48,
        
        DAC_dacMode     = 0x4c,
        DAC_dacAddr     = 0x50,
        DAC_dacData     = 0x54,
        
        Video_vidProcCfg = 0x5c,
        Video_maxRgbDelta = 0x58,
        Video_hwCurPatAddr = 0x60,
        Video_hwCurLoc     = 0x64,
        Video_hwCurC0      = 0x68,
        Video_hwCurC1      = 0x6c,
        Video_vidSerialParallelPort = 0x78,
        Video_vidScreenSize = 0x98,
        Video_vidOverlayStartCoords = 0x9c,
        Video_vidOverlayEndScreenCoords = 0xa0,
        Video_vidOverlayDudx = 0xa4,
        Video_vidOverlayDudxOffsetSrcWidth = 0xa8,
        Video_vidOverlayDvdy = 0xac,
        Video_vidOverlayDvdyOffset = 0xe0,
        Video_vidDesktopStartAddr = 0xe4,
        Video_vidDesktopOverlayStride = 0xe8
};

enum
{
        cmdBaseAddr0  = 0x20,
        cmdBaseSize0  = 0x24,
        cmdBump0      = 0x28,
        cmdRdPtrL0    = 0x2c,
        cmdRdPtrH0    = 0x30,
        cmdAMin0      = 0x34,
        cmdAMax0      = 0x3c,
        cmdFifoDepth0 = 0x44,
        cmdHoleCnt0   = 0x48
};

#define VGAINIT0_RAMDAC_8BIT (1 << 2)
#define VGAINIT0_EXTENDED_SHIFT_OUT (1 << 12)

#define VIDPROCCFG_VIDPROC_ENABLE (1 << 0)
#define VIDPROCCFG_CURSOR_MODE (1 << 1)
#define VIDPROCCFG_INTERLACE (1 << 3)
#define VIDPROCCFG_HALF_MODE (1 << 4)
#define VIDPROCCFG_OVERLAY_ENABLE (1 << 8)
#define VIDPROCCFG_OVERLAY_CLUT_BYPASS (1 << 11)
#define VIDPROCCFG_OVERLAY_CLUT_SEL (1 << 13)
#define VIDPROCCFG_H_SCALE_ENABLE (1 << 14)
#define VIDPROCCFG_V_SCALE_ENABLE (1 << 15)
#define VIDPROCCFG_FILTER_MODE_MASK (3 << 16)
#define VIDPROCCFG_FILTER_MODE_POINT      (0 << 16)
#define VIDPROCCFG_FILTER_MODE_DITHER_2X2 (1 << 16)
#define VIDPROCCFG_FILTER_MODE_DITHER_4X4 (2 << 16)
#define VIDPROCCFG_FILTER_MODE_BILINEAR   (3 << 16)
#define VIDPROCCFG_DESKTOP_PIX_FORMAT ((banshee->vidProcCfg >> 18) & 7)
#define VIDPROCCFG_OVERLAY_PIX_FORMAT ((banshee->vidProcCfg >> 21) & 7)
#define VIDPROCCFG_OVERLAY_PIX_FORMAT_SHIFT (21)
#define VIDPROCCFG_OVERLAY_PIX_FORMAT_MASK (7 << VIDPROCCFG_OVERLAY_PIX_FORMAT_SHIFT)
#define VIDPROCCFG_DESKTOP_TILE (1 << 24)
#define VIDPROCCFG_OVERLAY_TILE (1 << 25)
#define VIDPROCCFG_2X_MODE      (1 << 26)
#define VIDPROCCFG_HWCURSOR_ENA (1 << 27)

#define OVERLAY_FMT_565        (1)
#define OVERLAY_FMT_YUYV422    (5)
#define OVERLAY_FMT_UYVY422    (6)
#define OVERLAY_FMT_565_DITHER (7)

#define OVERLAY_START_X_MASK (0xfff)
#define OVERLAY_START_Y_SHIFT (12)
#define OVERLAY_START_Y_MASK (0xfff << OVERLAY_START_Y_SHIFT)

#define OVERLAY_END_X_MASK (0xfff)
#define OVERLAY_END_Y_SHIFT (12)
#define OVERLAY_END_Y_MASK (0xfff << OVERLAY_END_Y_SHIFT)

#define OVERLAY_SRC_WIDTH_SHIFT (19)
#define OVERLAY_SRC_WIDTH_MASK  (0x1fff << OVERLAY_SRC_WIDTH_SHIFT)

#define VID_STRIDE_OVERLAY_SHIFT (16)
#define VID_STRIDE_OVERLAY_MASK (0x7fff << VID_STRIDE_OVERLAY_SHIFT)

#define VID_DUDX_MASK (0xffffff)
#define VID_DVDY_MASK (0xffffff)

#define PIX_FORMAT_8      0
#define PIX_FORMAT_RGB565 1
#define PIX_FORMAT_RGB24  2
#define PIX_FORMAT_RGB32  3

#define VIDSERIAL_DDC_DCK_W (1 << 19)
#define VIDSERIAL_DDC_DDA_W (1 << 20)
#define VIDSERIAL_DDC_DCK_R (1 << 21)
#define VIDSERIAL_DDC_DDA_R (1 << 22)
#define VIDSERIAL_I2C_SCK_W (1 << 24)
#define VIDSERIAL_I2C_SDA_W (1 << 25)
#define VIDSERIAL_I2C_SCK_R (1 << 26)
#define VIDSERIAL_I2C_SDA_R (1 << 27)

#define MISCINIT0_Y_ORIGIN_SWAP_SHIFT (18)
#define MISCINIT0_Y_ORIGIN_SWAP_MASK (0xfff << MISCINIT0_Y_ORIGIN_SWAP_SHIFT)

static int banshee_vga_vsync_enabled(banshee_t *banshee)
{
    if (!(banshee->svga.crtc[0x11] & 0x20) && (banshee->svga.crtc[0x11] & 0x10) && ((banshee->pciInit0 >> 18) & 1) != 0)
        return 1;
    return 0;
}

static void banshee_update_irqs(banshee_t *banshee)
{
    if (banshee->vblank_irq > 0 && banshee_vga_vsync_enabled(banshee)) {
        pci_set_irq(0, PCI_INTA, nullptr);
    } else {
        pci_clear_irq(0, PCI_INTA, nullptr);
    }
}

static void banshee_vblank_start(svga_t* svga)
{
    banshee_t *banshee = (banshee_t*)svga->priv;
    if (banshee->vblank_irq >= 0) {
        banshee->vblank_irq = 1;
        banshee_update_irqs(banshee);
    }
}

static uint32_t banshee_status(banshee_t *banshee);

static void banshee_out(uint16_t addr, uint8_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        svga_t *svga = &banshee->svga;
        uint8_t old;
        
//        /*if (addr != 0x3c9) */pclog("banshee_out : %04X %02X  %04X:%04X\n", addr, val, CS,cpu_state.pc);
                
        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;

        switch (addr)
        {
                case 0x3D4:
                svga->crtcreg = val & 0x3f;
                return;
                case 0x3D5:
                if ((svga->crtcreg < 7) && (svga->crtc[0x11] & 0x80))
                        return;
                if ((svga->crtcreg == 7) && (svga->crtc[0x11] & 0x80))
                        val = (svga->crtc[7] & ~0x10) | (val & 0x10);
                old = svga->crtc[svga->crtcreg];
                svga->crtc[svga->crtcreg] = val;
                if (old != val)
                {
                        if (svga->crtcreg == 0x11) {
                            if (!(val & 0x10)) {
                                if (banshee->vblank_irq > 0)
                                    banshee->vblank_irq = -1;
                            } else if (banshee->vblank_irq < 0) {
                                banshee->vblank_irq = 0;
                            }
                            banshee_update_irqs(banshee);
                            if ((val & ~0x30) == (old & ~0x30))
                                old = val;
                        }
                        if (svga->crtcreg < 0xe || svga->crtcreg > 0x11 || (svga->crtcreg == 0x11 && old != val))
                        {
                            svga->fullchange = changeframecount;
                            svga_recalctimings(svga);
                        }
                }
                break;
        }
        svga_out(addr, val, svga);
}

static uint8_t banshee_in(uint16_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        svga_t *svga = &banshee->svga;
        uint8_t temp;

//        if (addr != 0x3da) pclog("banshee_in : %04X ", addr);
                
        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;
             
        switch (addr)
        {
                case 0x3c2:
                if ((svga->vgapal[0].r + svga->vgapal[0].g + svga->vgapal[0].b) >= 0x40)
                        temp = 0;
                else
                        temp = 0x10;
                if (banshee->vblank_irq > 0)
                    temp |= 0x80;
                break;
                case 0x3D4:
                temp = svga->crtcreg;
                break;
                case 0x3D5:
                temp = svga->crtc[svga->crtcreg];
                break;
                default:
                temp = svga_in(addr, svga);
                break;
        }
//        if (addr != 0x3da) pclog("%02X  %04X:%04X %i\n", temp, CS,cpu_state.pc, ins);
        return temp;
}

static void banshee_updatemapping(banshee_t *banshee)
{
        svga_t *svga = &banshee->svga;

        if (!(banshee->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_MEM))
        {
//                pclog("Update mapping - PCI disabled\n");
                mem_mapping_disablex(&svga->mapping);
                mem_mapping_disablex(&banshee->linear_mapping);
                mem_mapping_disablex(&banshee->reg_mapping_low);
                mem_mapping_disablex(&banshee->reg_mapping_high);
                return;
        }

        pclog("Update mapping - bank %02X ", svga->gdcreg[6] & 0xc);        
        switch (svga->gdcreg[6] & 0xc) /*Banked framebuffer*/
        {
                case 0x0: /*128k at A0000*/
                mem_mapping_set_addrx(&svga->mapping, 0xa0000, 0x20000);
                svga->banked_mask = 0xffff;
                break;
                case 0x4: /*64k at A0000*/
                mem_mapping_set_addrx(&svga->mapping, 0xa0000, 0x10000);
                svga->banked_mask = 0xffff;
                break;
                case 0x8: /*32k at B0000*/
                mem_mapping_set_addrx(&svga->mapping, 0xb0000, 0x08000);
                svga->banked_mask = 0x7fff;
                break;
                case 0xC: /*32k at B8000*/
                mem_mapping_set_addrx(&svga->mapping, 0xb8000, 0x08000);
                svga->banked_mask = 0x7fff;
                break;
        }
        
        pclog("Linear framebuffer %08X  ", banshee->memBaseAddr1);
        mem_mapping_set_addrx(&banshee->linear_mapping, banshee->memBaseAddr1, 32 << 20);
        pclog("registers %08X\n", banshee->memBaseAddr0);
        mem_mapping_set_addrx(&banshee->reg_mapping_low, banshee->memBaseAddr0, 8 << 20);
        mem_mapping_set_addrx(&banshee->reg_mapping_high, banshee->memBaseAddr0 + 0xc00000, 20 << 20);
}

static void banshee_render_16bpp_tiled(svga_t *svga)
{
        banshee_t *banshee = (banshee_t *)svga->priv;
        int x;
        int offset = 32;
        uint32_t *p = &((uint32_t *)buffer32->line[svga->displine])[offset];
        uint32_t addr;
        int drawn = 0;

        if (banshee->vidProcCfg & VIDPROCCFG_HALF_MODE)
                addr = banshee->desktop_addr + ((banshee->desktop_y >> 1) & 31) * 128 + ((banshee->desktop_y >> 6) * banshee->desktop_stride_tiled);
        else
                addr = banshee->desktop_addr + (banshee->desktop_y & 31) * 128 + ((banshee->desktop_y >> 5) * banshee->desktop_stride_tiled);

        for (x = 0; x < svga->hdisp; x += 64)
        {
                if (svga->hwcursor_on || svga->overlay_on)
                        svga->changedvram[addr >> 12] = 2;
                if (svga->changedvram[addr >> 12] || svga->fullchange)
                {
                        uint16_t *vram_p = (uint16_t *)&svga->vram[addr & svga->vram_display_mask];
                        int xx;

                        for (xx = 0; xx < 64; xx++)
                                *p++ = video_16to32[*vram_p++];

                        drawn = 1;
                }
                else
                        p += 64;
                addr += 128*32;
        }

        if (drawn)
        {
                if (svga->firstline_draw == 2000)
                        svga->firstline_draw = svga->displine;
                svga->lastline_draw = svga->displine;
        }

        banshee->desktop_y++;
}

static void banshee_recalctimings(svga_t *svga)
{
        banshee_t *banshee = (banshee_t *)svga->priv;
        voodoo_t *voodoo = banshee->voodoo;
        
/*7 R/W Horizontal Retrace End bit 5. -
  6 R/W Horizontal Retrace Start bit 8 0x4
  5 R/W Horizontal Blank End bit 6. -
  4 R/W Horizontal Blank Start bit 8. 0x3
  3 R/W Reserved. -
  2 R/W Horizontal Display Enable End bit 8. 0x1
  1 R/W Reserved. -
  0 R/W Horizontal Total bit 8. 0x0*/
        if (svga->crtc[0x1a] & 0x01) svga->htotal      += 0x100;
        if (svga->crtc[0x1a] & 0x04) svga->hdisp       += 0x100;
/*6 R/W Vertical Retrace Start bit 10 0x10
  5 R/W Reserved. -
  4 R/W Vertical Blank Start bit 10. 0x15
  3 R/W Reserved. -
  2 R/W Vertical Display Enable End bit 10 0x12
  1 R/W Reserved. -
  0 R/W Vertical Total bit 10. 0x6*/
        if (svga->crtc[0x1b] & 0x01) svga->vtotal      += 0x400;
        if (svga->crtc[0x1b] & 0x04) svga->dispend     += 0x400;
        if (svga->crtc[0x1b] & 0x10) svga->vblankstart += 0x400;
        if (svga->crtc[0x1b] & 0x40) svga->vsyncstart  += 0x400;
//        pclog("svga->hdisp=%i\n", svga->hdisp);

        svga->interlace = 0;
//        if (banshee->vgaInit0 & VGAINIT0_EXTENDED_SHIFT_OUT)
        if (banshee->vidProcCfg & VIDPROCCFG_VIDPROC_ENABLE)
        {
                // this is some VGA-only feature? G-REX driver sets it and still expects normal 640x480 display.
                svga->lowres = 0;

                switch (VIDPROCCFG_DESKTOP_PIX_FORMAT)
                {
                        case PIX_FORMAT_8:
                        svga->render = svga_render_8bpp_highres;
                        svga->bpp = 8;
                        break;
                        case PIX_FORMAT_RGB565:
                        svga->render = (banshee->vidProcCfg & VIDPROCCFG_DESKTOP_TILE) ? banshee_render_16bpp_tiled : svga_render_16bpp_highres;
                        svga->bpp = 16;
                        break;
                        case PIX_FORMAT_RGB24:
                        svga->render = svga_render_24bpp_highres;
                        svga->bpp = 24;
                        break;
                        case PIX_FORMAT_RGB32:
                        svga->render = svga_render_32bpp_highres;
                        svga->bpp = 32;
                        break;

#ifndef RELEASE_BUILD
                        default:
                        fatal("Unknown pixel format %08x\n", banshee->vgaInit0);
#endif
                }
                svga->rowcount = 0;
                if (!(banshee->vidProcCfg & VIDPROCCFG_DESKTOP_TILE) && (banshee->vidProcCfg & VIDPROCCFG_HALF_MODE))
                        svga->linedbl = 1;
                else
                        svga->linedbl = 0;
                if (banshee->vidProcCfg & VIDPROCCFG_DESKTOP_TILE)
                        svga->rowoffset = ((banshee->vidDesktopOverlayStride & 0x3fff) * 128) >> 3;
                else
                        svga->rowoffset = (banshee->vidDesktopOverlayStride & 0x3fff) >> 3;
                svga->ma_latch = banshee->vidDesktopStartAddr >> 2;
                banshee->desktop_stride_tiled = (banshee->vidDesktopOverlayStride & 0x3fff) * 128 * 32;
//                pclog("Extended shift out %i rowoffset=%i %02x\n", VIDPROCCFG_DESKTOP_PIX_FORMAT, svga->rowoffset, svga->crtc[1]);

                svga->char_width = 8;
                svga->split = 99999;

                if (banshee->vidProcCfg & VIDPROCCFG_2X_MODE)
                {
                        svga->hdisp *= 2;
                        svga->htotal *= 2;
                }

                if (banshee->vidProcCfg & VIDPROCCFG_INTERLACE)
                {
                    svga->interlace = 1;
                }

                svga->overlay.ena = banshee->vidProcCfg & VIDPROCCFG_OVERLAY_ENABLE;

                svga->overlay.x = voodoo->overlay.start_x;
                svga->overlay.y = voodoo->overlay.start_y;
                svga->overlay.cur_xsize = voodoo->overlay.size_x;
                svga->overlay.cur_ysize = voodoo->overlay.size_y;
                svga->overlay.pitch = (banshee->vidDesktopOverlayStride & VID_STRIDE_OVERLAY_MASK) >> VID_STRIDE_OVERLAY_SHIFT;
                if (banshee->vidProcCfg & VIDPROCCFG_OVERLAY_TILE)
                        svga->overlay.pitch *= 128*32;
                if (svga->overlay.cur_xsize <= 0 || svga->overlay.cur_ysize <= 0)
                        svga->overlay.ena = 0;
                if (svga->overlay.ena)
                {
/*                        pclog("Overlay enabled : start=%i,%i end=%i,%i size=%i,%i pitch=%x\n",
                                voodoo->overlay.start_x, voodoo->overlay.start_y,
                                voodoo->overlay.end_x, voodoo->overlay.end_y,
                                voodoo->overlay.size_x, voodoo->overlay.size_y,
                                svga->overlay.pitch);*/
                        if (!voodoo->overlay.start_x && !voodoo->overlay.start_y &&
                            svga->hdisp == voodoo->overlay.size_x && svga->dispend == voodoo->overlay.size_y)
                        {
                                /*Overlay is full screen, so don't bother rendering the desktop
                                  behind it*/
                                svga->render = svga_render_null;
                                svga->bpp = 0;
                        }
                }

                svga->video_res_override = 1;
                svga->video_res_x = svga->hdisp;
                svga->video_res_y = svga->dispend;
                svga->video_bpp = svga->bpp;
        }
        else
        {
//                pclog("Normal shift out\n");
                svga->bpp = 8;
                svga->video_res_override = 0;
        }

        svga->fb_only = (banshee->vidProcCfg & VIDPROCCFG_VIDPROC_ENABLE);

        svga->horizontal_linedbl = svga->dispend * 9 / 10 >= svga->hdisp;

        if (((svga->miscout >> 2) & 3) == 3)
        {
                int k = banshee->pllCtrl0 & 3;
                int m = (banshee->pllCtrl0 >> 2) & 0x3f;
                int n = (banshee->pllCtrl0 >> 8) & 0xff;
                double freq = (((double)n + 2) / (((double)m + 2) * (double)(1 << k))) * 14318184.0;

                svga->clock = (cpuclock * (float)(1ull << 32)) / freq;
//                svga->clock = cpuclock / freq;
                
//                pclog("svga->clock = %g %g  m=%i k=%i n=%i\n", freq, freq / 1000000.0, m, k, n);
        }
}

static void banshee_ext_out(uint16_t addr, uint8_t val, void *p)
{
//        banshee_t *banshee = (banshee_t *)p;
//        svga_t *svga = &banshee->svga;

//        pclog("banshee_ext_out: addr=%04x val=%02x\n", addr, val);
        
        switch (addr & 0xff)
        {
                case 0xb0: case 0xb1: case 0xb2: case 0xb3:
                case 0xb4: case 0xb5: case 0xb6: case 0xb7:
                case 0xb8: case 0xb9: case 0xba: case 0xbb:
                case 0xbc: case 0xbd: case 0xbe: case 0xbf:
                case 0xc0: case 0xc1: case 0xc2: case 0xc3:
                case 0xc4: case 0xc5: case 0xc6: case 0xc7:
                case 0xc8: case 0xc9: case 0xca: case 0xcb:
                case 0xcc: case 0xcd: case 0xce: case 0xcf:
                case 0xd0: case 0xd1: case 0xd2: case 0xd3:
                case 0xd4: case 0xd5: case 0xd6: case 0xd7:
                case 0xd8: case 0xd9: case 0xda: case 0xdb:
                case 0xdc: case 0xdd: case 0xde: case 0xdf:
                banshee_out((addr & 0xff)+0x300, val, p);
                break;
                        
                default:
                pclog("bad banshee_ext_out: addr=%04x val=%02x\n", addr, val);
        }
}
static void banshee_ext_outl(uint16_t addr, uint32_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;

//        pclog("banshee_ext_outl: addr=%04x val=%08x %04x(%08x):%08x\n", addr, val, CS,cs,cpu_state.pc);
        
        switch (addr & 0xff)
        {
                case Init_pciInit0:
                banshee->pciInit0 = val;
                voodoo->read_time = pci_nonburst_time + pci_burst_time * ((val & 0x100) ? 2 : 1);
                voodoo->burst_time = pci_burst_time * ((val & 0x200) ? 1 : 0);
                voodoo->write_time = pci_nonburst_time + voodoo->burst_time;
                break;
                        
                case Init_lfbMemoryConfig:
                banshee->lfbMemoryConfig = val;
//                pclog("lfbMemoryConfig=%08x\n", val);
                voodoo->tile_base = (val & 0x1fff) << 12;
                voodoo->tile_stride = 1024 << ((val >> 13) & 7);
                voodoo->tile_stride_shift = 10 + ((val >> 13) & 7);
                voodoo->tile_x = ((val >> 16) & 0x7f) * 128;
                voodoo->tile_x_real = ((val >> 16) & 0x7f) * 128*32;
                break;

                case Init_miscInit0:
                banshee->miscInit0 = val;
                extern void gfxboard_voodoo_lfb_endianswap(int);
                gfxboard_voodoo_lfb_endianswap(val >> 30);
                break;
                case Init_miscInit1:
                banshee->miscInit1 = val;
                break;
                case Init_dramInit0:
                banshee->dramInit0 = val;
                break;
                case Init_dramInit1:
                banshee->dramInit1 = val;
                break;
                case Init_agpInit0:
                banshee->agpInit0 = val;
                break;

                case Init_2dCommand:
                banshee->command_2d = val;
                break;
                case Init_2dSrcBaseAddr:
                banshee->srcBaseAddr_2d = val;
                break;
                case Init_vgaInit0:
                banshee->vgaInit0 = val;
                svga_set_ramdac_type(svga, (val & VGAINIT0_RAMDAC_8BIT ? RAMDAC_8BIT : RAMDAC_6BIT));
                break;
                case Init_vgaInit1:
                banshee->vgaInit1 = val;
                svga->write_bank = (val & 0x3ff) << 15;
                svga->read_bank = ((val >> 10) & 0x3ff) << 15;
                break;

                case PLL_pllCtrl0:
                banshee->pllCtrl0 = val;
                break;
                case PLL_pllCtrl1:
                banshee->pllCtrl1 = val;
                break;
                case PLL_pllCtrl2:
                banshee->pllCtrl2 = val;
                break;

                case DAC_dacMode:
                banshee->dacMode = val;
                break;
                case DAC_dacAddr:
                banshee->dacAddr = val & 0x1ff;
                break;
                case DAC_dacData:
                svga->pallook[banshee->dacAddr] = val & 0xffffff;
                svga->fullchange = changeframecount;
                break;

                case Video_vidProcCfg:                                
                banshee->vidProcCfg = val;
//                pclog("vidProcCfg=%08x\n", val);
                banshee->overlay_pix_fmt = (val & VIDPROCCFG_OVERLAY_PIX_FORMAT_MASK) >> VIDPROCCFG_OVERLAY_PIX_FORMAT_SHIFT;
                svga->hwcursor.ena = val & VIDPROCCFG_HWCURSOR_ENA;
                svga->fullchange = changeframecount;
                svga_recalctimings(svga);
                break;

                case Video_maxRgbDelta:
                banshee->voodoo->scrfilterThreshold = val;
                if (val > 0x00)
                        banshee->voodoo->scrfilterEnabled = 1;
                else
                        banshee->voodoo->scrfilterEnabled = 0;
                voodoo_threshold_check(banshee->voodoo);
                //pclog("Banshee Filter: %06x\n", val);

                break;

                case Video_hwCurPatAddr:
                banshee->hwCurPatAddr = val;
                svga->hwcursor.addr = (val & 0xfffff0) + (svga->hwcursor.yoff * 16);
                break;
                case Video_hwCurLoc:
                banshee->hwCurLoc = val;
                svga->hwcursor.x = (val & 0x7ff) - 32;
                svga->hwcursor.y = ((val >> 16) & 0x7ff) - 64;
                if (svga->hwcursor.y < 0)
                {
                        svga->hwcursor.yoff = -svga->hwcursor.y;
                        svga->hwcursor.y = 0;
                }
                else
                        svga->hwcursor.yoff = 0;
                svga->hwcursor.addr = (banshee->hwCurPatAddr & 0xfffff0) + (svga->hwcursor.yoff * 16);
                svga->hwcursor.cur_xsize = 64;
                svga->hwcursor.cur_ysize = 64;
//                pclog("hwCurLoc %08x %i\n", val, svga->hwcursor.y);
                break;
                case Video_hwCurC0:
                banshee->hwCurC0 = val;
                break;
                case Video_hwCurC1:
                banshee->hwCurC1 = val;
                break;

                case Video_vidSerialParallelPort:
                banshee->vidSerialParallelPort = val;
//                pclog("vidSerialParallelPort: write %08x %08x %04x(%08x):%08x\n", val, val & (VIDSERIAL_DDC_DCK_W | VIDSERIAL_DDC_DDA_W), CS,cs,cpu_state.pc);
                //ddc_i2c_change((val & VIDSERIAL_DDC_DCK_W) ? 1 : 0, (val & VIDSERIAL_DDC_DDA_W) ? 1 : 0);
                break;

                case Video_vidScreenSize:
                banshee->vidScreenSize = val;
                voodoo->h_disp = (val & 0xfff) + 1;
                voodoo->v_disp = (val >> 12) & 0xfff;
                break;
                case Video_vidOverlayStartCoords:
                voodoo->overlay.vidOverlayStartCoords = val;
                voodoo->overlay.start_x = val & OVERLAY_START_X_MASK;
                voodoo->overlay.start_y = (val & OVERLAY_START_Y_MASK) >> OVERLAY_START_Y_SHIFT;
                voodoo->overlay.size_x = voodoo->overlay.end_x - voodoo->overlay.start_x;
                voodoo->overlay.size_y = voodoo->overlay.end_y - voodoo->overlay.start_y;
                svga_recalctimings(svga);
                break;
                case Video_vidOverlayEndScreenCoords:
                voodoo->overlay.vidOverlayEndScreenCoords = val;
                voodoo->overlay.end_x = val & OVERLAY_END_X_MASK;
                voodoo->overlay.end_y = (val & OVERLAY_END_Y_MASK) >> OVERLAY_END_Y_SHIFT;
                voodoo->overlay.size_x = (voodoo->overlay.end_x - voodoo->overlay.start_x) + 1;
                voodoo->overlay.size_y = (voodoo->overlay.end_y - voodoo->overlay.start_y) + 1;
                svga_recalctimings(svga);
                break;
                case Video_vidOverlayDudx:
                voodoo->overlay.vidOverlayDudx = val & VID_DUDX_MASK;
//                pclog("vidOverlayDudx=%08x\n", val);
                break;
                case Video_vidOverlayDudxOffsetSrcWidth:
                voodoo->overlay.vidOverlayDudxOffsetSrcWidth = val;
                voodoo->overlay.overlay_bytes = (val & OVERLAY_SRC_WIDTH_MASK) >> OVERLAY_SRC_WIDTH_SHIFT;
//                pclog("vidOverlayDudxOffsetSrcWidth=%08x\n", val);
                break;
                case Video_vidOverlayDvdy:
                voodoo->overlay.vidOverlayDvdy = val & VID_DVDY_MASK;
//                pclog("vidOverlayDvdy=%08x\n", val);
                break;
                case Video_vidOverlayDvdyOffset:
                voodoo->overlay.vidOverlayDvdyOffset = val;
                break;


                case Video_vidDesktopStartAddr:
                banshee->vidDesktopStartAddr = val & 0xffffff;
//                pclog("vidDesktopStartAddr=%08x\n", val);
                svga->fullchange = changeframecount;
                svga_recalctimings(svga);
                break;
                case Video_vidDesktopOverlayStride:
                banshee->vidDesktopOverlayStride = val;
//                pclog("vidDesktopOverlayStride=%08x\n", val);
                svga->fullchange = changeframecount;
                svga_recalctimings(svga);
                break;
//                default:
//                fatal("bad banshee_ext_outl: addr=%04x val=%08x\n", addr, val);
        }
}

static uint8_t banshee_ext_in(uint16_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
//        svga_t *svga = &banshee->svga;
        uint8_t ret = 0xff;
             
        switch (addr & 0xff)
        {
                case Init_status: case Init_status+1: case Init_status+2: case Init_status+3:
                ret = (banshee_status(banshee) >> ((addr & 3) * 8)) & 0xff;
//                pclog("Read status reg! %04x(%08x):%08x\n", CS, cs, cpu_state.pc);
                break;

                case 0xb0: case 0xb1: case 0xb2: case 0xb3:
                case 0xb4: case 0xb5: case 0xb6: case 0xb7:
                case 0xb8: case 0xb9: case 0xba: case 0xbb:
                case 0xbc: case 0xbd: case 0xbe: case 0xbf:
                case 0xc0: case 0xc1: case 0xc2: case 0xc3:
                case 0xc4: case 0xc5: case 0xc6: case 0xc7:
                case 0xc8: case 0xc9: case 0xca: case 0xcb:
                case 0xcc: case 0xcd: case 0xce: case 0xcf:
                case 0xd0: case 0xd1: case 0xd2: case 0xd3:
                case 0xd4: case 0xd5: case 0xd6: case 0xd7:
                case 0xd8: case 0xd9: case 0xda: case 0xdb:
                case 0xdc: case 0xdd: case 0xde: case 0xdf:
                ret = banshee_in((addr & 0xff)+0x300, p);
                break;

                default:
                pclog("bad banshee_ext_in: addr=%04x\n", addr);
                break;
        }

//        pclog("banshee_ext_in: addr=%04x val=%02x\n", addr, ret);
        
        return ret;
}

static uint32_t banshee_status(banshee_t *banshee)
{
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        int fifo_entries = FIFO_ENTRIES;
        int fifo_size = 0xffff - fifo_entries;
        int swap_count = voodoo->swap_count;
        int written = voodoo->cmd_written + voodoo->cmd_written_fifo;
        int busy = (written - voodoo->cmd_read) || (voodoo->cmdfifo_depth_rd != voodoo->cmdfifo_depth_wr) ||
                voodoo->render_voodoo_busy[0] || voodoo->render_voodoo_busy[1] ||
                voodoo->render_voodoo_busy[2] || voodoo->render_voodoo_busy[3] ||
                voodoo->voodoo_busy;
        uint32_t ret;

        ret = 0;
        if (fifo_entries < 0x20)
                ret |= 0x1f - fifo_entries;
        else
                ret |= 0x1f;
        if (fifo_entries)
                ret |= 0x20;
        if (swap_count < 7)
                ret |= (swap_count << 28);
        else
                ret |= (7 << 28);
        if (!(svga->cgastat & 8))
                ret |= 0x40;

        if (busy)
                ret |= 0x780; /*Busy*/

        if (voodoo->cmdfifo_depth_rd != voodoo->cmdfifo_depth_wr)
                ret |= (1 << 11);

        if (!voodoo->voodoo_busy)
                voodoo_wake_fifo_thread(voodoo);

//        pclog("banshee_status: busy %i  %i (%i %i)  %i   %i %i  %04x(%08x):%08x %08x\n", busy, written, voodoo->cmd_written, voodoo->cmd_written_fifo, voodoo->cmd_read, voodoo->cmdfifo_depth_rd, voodoo->cmdfifo_depth_wr, CS,cs,cpu_state.pc, ret);

        return ret;
}

static uint32_t banshee_ext_inl(uint16_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        uint32_t ret = 0xffffffff;

        cycles -= voodoo->read_time;
        
        switch (addr & 0xff)
        {
                case Init_status:
                ret = banshee_status(banshee);
//                pclog("Read status reg! %04x(%08x):%08x\n", CS, cs, cpu_state.pc);
                break;
                case Init_pciInit0:
                ret = banshee->pciInit0;
                break;
                case Init_lfbMemoryConfig:
                ret = banshee->lfbMemoryConfig;
                break;
                
                case Init_miscInit0:
                ret = banshee->miscInit0;
                break;
                case Init_miscInit1:
                ret = banshee->miscInit1;
                break;
                case Init_dramInit0:
                ret = banshee->dramInit0;
                break;
                case Init_dramInit1:
                ret = banshee->dramInit1;
                break;
                case Init_agpInit0:
                ret = banshee->agpInit0;
                break;

                case Init_vgaInit0:
                ret = banshee->vgaInit0;
                break;
                case Init_vgaInit1:
                ret = banshee->vgaInit1;
                break;

                case Init_2dCommand:
                ret = banshee->command_2d;
                break;
                case Init_2dSrcBaseAddr:
                ret = banshee->srcBaseAddr_2d;
                break;
                case Init_strapInfo:
                ret = 0x00000040; /*8 MB SGRAM, PCI, IRQ enabled, 32kB BIOS*/
                break;

                case PLL_pllCtrl0:
                ret = banshee->pllCtrl0;
                break;
                case PLL_pllCtrl1:
                ret = banshee->pllCtrl1;
                break;
                case PLL_pllCtrl2:
                ret = banshee->pllCtrl2;
                break;
                
                case DAC_dacMode:
                ret = banshee->dacMode;
                break;
                case DAC_dacAddr:
                ret = banshee->dacAddr;
                break;
                case DAC_dacData:
                ret = svga->pallook[banshee->dacAddr];
                break;

                case Video_vidProcCfg:                                
                ret = banshee->vidProcCfg;
                break;

                case Video_hwCurPatAddr:
                ret = banshee->hwCurPatAddr;
                break;
                case Video_hwCurLoc:
                ret = banshee->hwCurLoc;
                break;
                case Video_hwCurC0:
                ret = banshee->hwCurC0;
                break;
                case Video_hwCurC1:
                ret = banshee->hwCurC1;
                break;

                case Video_vidSerialParallelPort:
                ret = banshee->vidSerialParallelPort & ~(VIDSERIAL_DDC_DCK_R | VIDSERIAL_DDC_DDA_R);
                if ((banshee->vidSerialParallelPort & VIDSERIAL_DDC_DCK_W) && false) //ddc_read_clock())
                        ret |= VIDSERIAL_DDC_DCK_R;
                if ((banshee->vidSerialParallelPort & VIDSERIAL_DDC_DDA_W) && false) //ddc_read_data())
                        ret |= VIDSERIAL_DDC_DDA_R;
                ret = ret & ~(VIDSERIAL_I2C_SCK_R | VIDSERIAL_I2C_SDA_R);
                if (banshee->vidSerialParallelPort & VIDSERIAL_I2C_SCK_W)
                        ret |= VIDSERIAL_I2C_SCK_R;
                if (banshee->vidSerialParallelPort & VIDSERIAL_I2C_SDA_W)
                        ret |= VIDSERIAL_I2C_SDA_R;
//                pclog("vidSerialParallelPort: read %08x %08x  %04x(%08x):%08x\n", ret, ret & (VIDSERIAL_DDC_DCK_R | VIDSERIAL_DDC_DDA_R), CS,cs,cpu_state.pc);
                break;

                case Video_vidScreenSize:
                ret = banshee->vidScreenSize;
                break;
                case Video_vidOverlayStartCoords:
                ret = voodoo->overlay.vidOverlayStartCoords;
                break;
                case Video_vidOverlayEndScreenCoords:
                ret = voodoo->overlay.vidOverlayEndScreenCoords;
                break;
                case Video_vidOverlayDudx:
                ret = voodoo->overlay.vidOverlayDudx;
                break;
                case Video_vidOverlayDudxOffsetSrcWidth:
                ret = voodoo->overlay.vidOverlayDudxOffsetSrcWidth;
                break;
                case Video_vidOverlayDvdy:
                ret = voodoo->overlay.vidOverlayDvdy;
                break;
                case Video_vidOverlayDvdyOffset:
                ret = voodoo->overlay.vidOverlayDvdyOffset;
                break;

                case Video_vidDesktopStartAddr:
                ret = banshee->vidDesktopStartAddr;
                break;
                case Video_vidDesktopOverlayStride:
                ret = banshee->vidDesktopOverlayStride;
                break;

                default:
//                fatal("bad banshee_ext_inl: addr=%04x\n", addr);
                break;
        }

//        /*if (addr) */pclog("banshee_ext_inl: addr=%04x val=%08x\n", addr, ret);
        
        return ret;
}

// miscInit0 register swizzle
static uint32_t registerswizzle(uint32_t data, uint32_t addr, int addrtype, banshee_t *banshee)
{
    // swizzling enabled if miscInit0 swizzle bit(s) set and:
    // 2D register address space and address bit 19 set
    // 3D register address space and address bit 20 set
    if ((!addrtype && (addr & 0x00080000)) || (addrtype && (addr & 0x00100000))) {
        if (banshee->miscInit0 & 4)
            data = (data >> 24) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | (data << 24);
        if (banshee->miscInit0 & 8)
            data = (data >> 16) | (data << 16);
    }
    return data;
}


static uint32_t banshee_reg_readl(uint32_t addr, void *p);

static uint8_t banshee_reg_read(uint32_t addr, void *p)
{
//        pclog("banshee_reg_read: addr=%08x\n", addr);
        return banshee_reg_readl(addr & ~3, p) >> (8*(addr & 3));
}

static uint16_t banshee_reg_readw(uint32_t addr, void *p)
{
//        pclog("banshee_reg_readw: addr=%08x\n", addr);
        return banshee_reg_readl(addr & ~3, p) >> (8*(addr & 2));
}

static uint32_t banshee_cmd_read(banshee_t *banshee, uint32_t addr)
{
        voodoo_t *voodoo = banshee->voodoo;
        uint32_t ret = 0xffffffff;

        switch (addr & 0x1fc)
        {
                case cmdBaseAddr0:
                ret = voodoo->cmdfifo_base >> 12;
//                pclog("Read cmdfifo_base %08x\n", ret);
                break;
                
                case cmdRdPtrL0:
                ret = voodoo->cmdfifo_rp;
//                pclog("Read cmdfifo_rp %08x\n", ret);
                break;
                
                case cmdFifoDepth0:
                ret = voodoo->cmdfifo_depth_wr - voodoo->cmdfifo_depth_rd;
//                pclog("Read cmdfifo_depth %08x\n", ret);
                break;

                case cmdBaseSize0:
                ret = voodoo->cmdfifo_size;
                break;

                case 0x108:
                break;

#ifndef RELEASE_BUILD
                default:
                fatal("Unknown banshee_cmd_read %08x\n", addr);
#endif
        }
        
        return ret;
}

static uint32_t banshee_reg_readl(uint32_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        uint32_t ret = 0xffffffff;
        
        cycles -= voodoo->read_time;

        switch (addr & 0x1f00000)
        {
                case 0x0000000: /*IO remap*/
                if (!(addr & 0x80000))
                        ret = banshee_ext_inl(addr & 0xff, banshee);
                else
                        ret = banshee_cmd_read(banshee, addr);
                break;
                
                case 0x0100000: /*2D registers*/
                voodoo_flush(voodoo);
                switch (addr & 0x1fc)
                {
                        case SST_status:
                        ret = banshee_status(banshee);
                        break;

                        case SST_intrCtrl:
                        ret = banshee->intrCtrl & 0x0030003f;
                        break;

                        case 0x08:
                        ret = voodoo->banshee_blt.clip0Min;
                        break;
                        case 0x0c:
                        ret = voodoo->banshee_blt.clip0Max;
                        break;
                        case 0x10:
                        ret = voodoo->banshee_blt.dstBaseAddr;
                        break;
                        case 0x14:
                        ret = voodoo->banshee_blt.dstFormat;
                        break;
                        case 0x34:
                        ret = voodoo->banshee_blt.srcBaseAddr;
                        break;
                        case 0x38:
                        ret = voodoo->banshee_blt.commandExtra;
                        break;
                        case 0x5c:
                        ret = voodoo->banshee_blt.srcXY;
                        break;
                        case 0x60:
                        ret = voodoo->banshee_blt.colorBack;
                        break;
                        case 0x64:
                        ret = voodoo->banshee_blt.colorFore;
                        break;
                        case 0x68:
                        ret = voodoo->banshee_blt.dstSize;
                        break;
                        case 0x6c:
                        ret = voodoo->banshee_blt.dstXY;
                        break;
                        case 0x70:
                        ret = voodoo->banshee_blt.command;
                        break;
                        default:
                        pclog("banshee_reg_readl: addr=%08x\n", addr);
                }
                ret = registerswizzle(ret, addr, 0, banshee);
                break;

                case 0x0200000: case 0x0300000: case 0x0400000: case 0x0500000: /*3D registers*/
                switch (addr & 0x3fc)
                {
                        case SST_status:
                        ret = banshee_status(banshee);
                        break;

                        case SST_intrCtrl:
                        ret = banshee->intrCtrl & 0x0030003f;
                        break;
                        
                        case SST_fbzColorPath:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.fbzColorPath;
                        break;
                        case SST_fogMode:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.fogMode;
                        break;
                        case SST_alphaMode:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.alphaMode;
                        break;
                        case SST_fbzMode:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.fbzMode;
                        break;
                        case SST_lfbMode:
                        voodoo_flush(voodoo);
                        ret = voodoo->lfbMode;
                        break;
                        case SST_clipLeftRight:
                        ret = voodoo->params.clipRight | (voodoo->params.clipLeft << 16);
                        break;
                        case SST_clipLowYHighY:
                        ret = voodoo->params.clipHighY | (voodoo->params.clipLowY << 16);
                        break;

                        case SST_clipLeftRight1:
                        ret = voodoo->params.clipRight1 | (voodoo->params.clipLeft1 << 16);
                        break;
                        case SST_clipTopBottom1:
                        ret = voodoo->params.clipHighY1 | (voodoo->params.clipLowY1 << 16);
                        break;

                        case SST_stipple:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.stipple;
                        break;
                        case SST_color0:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.color0;
                        break;
                        case SST_color1:
                        voodoo_flush(voodoo);
                        ret = voodoo->params.color1;
                        break;

                        case SST_fbiPixelsIn:
                        ret = voodoo->fbiPixelsIn & 0xffffff;
                        break;
                        case SST_fbiChromaFail:
                        ret = voodoo->fbiChromaFail & 0xffffff;
                        break;
                        case SST_fbiZFuncFail:
                        ret = voodoo->fbiZFuncFail & 0xffffff;
                        break;
                        case SST_fbiAFuncFail:
                        ret = voodoo->fbiAFuncFail & 0xffffff;
                        break;
                        case SST_fbiPixelsOut:
                        ret = voodoo->fbiPixelsOut & 0xffffff;
                        break;

                        default:
                        pclog("banshee_reg_readl: 3D addr=%08x\n", addr);
                        break;
                }
                ret = registerswizzle(ret, addr, 1, banshee);
                break;
        }

//        /*if (addr != 0xe0000000) */pclog("banshee_reg_readl: addr=%08x ret=%08x %04x(%08x):%08x\n", addr, ret, CS,cs,cpu_state.pc);
//        if (cpu_state.pc == 0x1000e437)
//                output = 3;
        return ret;
}

static void banshee_reg_write(uint32_t addr, uint8_t val, void *p)
{
//        pclog("banshee_reg_writeb: addr=%08x val=%02x\n", addr, val);
}

static void banshee_reg_writew(uint32_t addr, uint16_t val, void *p)
{       
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;

        cycles -= voodoo->write_time;
        
//        pclog("banshee_reg_writew: addr=%08x val=%04x\n", addr, val);
        switch (addr & 0x1f00000)
        {
                case 0x1000000: case 0x1100000: case 0x1200000: case 0x1300000: /*3D LFB*/
                case 0x1400000: case 0x1500000: case 0x1600000: case 0x1700000:
                case 0x1800000: case 0x1900000: case 0x1a00000: case 0x1b00000:
                case 0x1c00000: case 0x1d00000: case 0x1e00000: case 0x1f00000:
                voodoo_queue_command(voodoo, (addr & 0xffffff) | FIFO_WRITEW_FB, val);
                break;
        }
}

static void banshee_cmd_write(banshee_t *banshee, uint32_t addr, uint32_t val)
{
        voodoo_t *voodoo = banshee->voodoo;
//        pclog("banshee_cmd_write: addr=%03x val=%08x\n", addr & 0x1fc, val);
        switch (addr & 0x1fc)
        {
                case cmdBaseAddr0:
                voodoo->cmdfifo_base = (val & 0xfff) << 12;
                voodoo->cmdfifo_end = voodoo->cmdfifo_base + (((voodoo->cmdfifo_size & 0xff) + 1) << 12);
//                pclog("cmdfifo_base=%08x  cmdfifo_end=%08x %08x\n", voodoo->cmdfifo_base, voodoo->cmdfifo_end, val);
                break;
                
                case cmdBaseSize0:
                voodoo->cmdfifo_size = val;
                voodoo->cmdfifo_end = voodoo->cmdfifo_base + (((voodoo->cmdfifo_size & 0xff) + 1) << 12);
                voodoo->cmdfifo_enabled = val & 0x100;
                if (!voodoo->cmdfifo_enabled)
                        voodoo->cmdfifo_in_sub = 0; /*Not sure exactly when this should be reset*/
//                pclog("cmdfifo_base=%08x  cmdfifo_end=%08x\n", voodoo->cmdfifo_base, voodoo->cmdfifo_end);
                break;
                
//                voodoo->cmdfifo_end = ((val >> 16) & 0x3ff) << 12;
//                pclog("CMDFIFO base=%08x end=%08x\n", voodoo->cmdfifo_base, voodoo->cmdfifo_end);
//                break;

                case cmdRdPtrL0:
                voodoo->cmdfifo_rp = val;
                break;
                case cmdAMin0:
                voodoo->cmdfifo_amin = val;
                break;
                case cmdAMax0:
                voodoo->cmdfifo_amax = val;
                break;
                case cmdFifoDepth0:
                voodoo->cmdfifo_depth_rd = 0;
                voodoo->cmdfifo_depth_wr = val & 0xffff;
                break;
                
                default:
                pclog("Unknown banshee_cmd_write: addr=%08x val=%08x\n", addr, val);
                break;
        }

/*        cmdBaseSize0  = 0x24,
        cmdBump0      = 0x28,
        cmdRdPtrL0    = 0x2c,
        cmdRdPtrH0    = 0x30,
        cmdAMin0      = 0x34,
        cmdAMax0      = 0x3c,
        cmdFifoDepth0 = 0x44,
        cmdHoleCnt0   = 0x48
        }*/
}

static void banshee_reg_writel(uint32_t addr, uint32_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        
        if (addr == voodoo->last_write_addr+4)
                cycles -= voodoo->burst_time;
        else
                cycles -= voodoo->write_time;
        voodoo->last_write_addr = addr;

//        pclog("banshee_reg_writel: addr=%08x val=%08x\n", addr, val);
        
        switch (addr & 0x1f00000)
        {
                case 0x0000000: /*IO remap*/
                if (!(addr & 0x80000))
                        banshee_ext_outl(addr & 0xff, val, banshee);
                else
                        banshee_cmd_write(banshee, addr, val);
//                        pclog("CMD!!! write %08x %08x\n", addr, val);
                break;

                case 0x0100000: /*2D registers*/
                    val = registerswizzle(val, addr, 0, banshee);
                    if ((addr & 0x3fc) == SST_intrCtrl) {
                        banshee->intrCtrl = val & 0x0030003f;
                    } else {
                        voodoo_queue_command(voodoo, (addr & 0x1fc) | FIFO_WRITEL_2DREG, val);
                    }
                break;
                
                case 0x0200000: case 0x0300000: case 0x0400000: case 0x0500000: /*3D registers*/
                    val = registerswizzle(val, addr, 1, banshee);
                    switch (addr & 0x3fc)
                {
                        case SST_intrCtrl:
                        banshee->intrCtrl = val & 0x0030003f;
//                        pclog("intrCtrl=%08x\n", val);
                        break;

                        case SST_userIntrCMD:
#ifndef RELEASE_BUILD
                        fatal("userIntrCMD write %08x\n", val);
#endif
                        break;

                        case SST_swapbufferCMD:
                        voodoo->cmd_written++;
                        voodoo_queue_command(voodoo, (addr & 0x3fc) | FIFO_WRITEL_REG, val);
                        if (!voodoo->voodoo_busy)
                                voodoo_wake_fifo_threads(voodoo->set, voodoo);
//                        pclog("SST_swapbufferCMD write: %i %i\n", voodoo->cmd_written, voodoo->cmd_written_fifo);
                        break;
                        case SST_triangleCMD:
                        voodoo->cmd_written++;
                        voodoo_queue_command(voodoo, (addr & 0x3fc) | FIFO_WRITEL_REG, val);
                        if (!voodoo->voodoo_busy)
                                voodoo_wake_fifo_threads(voodoo->set, voodoo);
                        break;
                        case SST_ftriangleCMD:
                        voodoo->cmd_written++;
                        voodoo_queue_command(voodoo, (addr & 0x3fc) | FIFO_WRITEL_REG, val);
                        if (!voodoo->voodoo_busy)
                                voodoo_wake_fifo_threads(voodoo->set, voodoo);
                        break;
                        case SST_fastfillCMD:
                        voodoo->cmd_written++;
                        voodoo_queue_command(voodoo, (addr & 0x3fc) | FIFO_WRITEL_REG, val);
                        if (!voodoo->voodoo_busy)
                                voodoo_wake_fifo_threads(voodoo->set, voodoo);
                        break;
                        case SST_nopCMD:
                        voodoo->cmd_written++;
                        voodoo_queue_command(voodoo, (addr & 0x3fc) | FIFO_WRITEL_REG, val);
                        if (!voodoo->voodoo_busy)
                                voodoo_wake_fifo_threads(voodoo->set, voodoo);
                        break;
                        
                        case SST_swapPending:
                        thread_wait_mutex(voodoo->swap_mutex);
                        voodoo->swap_count++;
                        thread_release_mutex(voodoo->swap_mutex);
//                        voodoo->cmd_written++;
                        break;
                        
                        default:
                        voodoo_queue_command(voodoo, (addr & 0x3ffffc) | FIFO_WRITEL_REG, val);
                        break;
                }
                break;
                
                case 0x0600000: case 0x0700000: /*Texture download*/
                voodoo->tex_count++;
                voodoo_queue_command(voodoo, (addr & 0x1ffffc) | FIFO_WRITEL_TEX, val);
                break;
                
                case 0x1000000: case 0x1100000: case 0x1200000: case 0x1300000: /*3D LFB*/
                case 0x1400000: case 0x1500000: case 0x1600000: case 0x1700000:
                case 0x1800000: case 0x1900000: case 0x1a00000: case 0x1b00000:
                case 0x1c00000: case 0x1d00000: case 0x1e00000: case 0x1f00000:
                voodoo_queue_command(voodoo, (addr & 0xfffffc) | FIFO_WRITEL_FB, val);
                break;
        }
}

static uint8_t banshee_read_linear(uint32_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        
#if 0
        cycles -= voodoo->read_time;
        cycles_lost += voodoo->read_time;
#endif
        addr &= svga->decode_mask;
        if (addr >= voodoo->tile_base)
        {
                int x, y;

                addr -= voodoo->tile_base;
                x = addr & (voodoo->tile_stride-1);
                y = addr >> voodoo->tile_stride_shift;

                addr = voodoo->tile_base + (x & 127) + ((x >> 7) * 128*32) + ((y & 31) * 128) + (y >> 5)*voodoo->tile_x_real;
//                pclog("  Tile rb %08x->%08x %i %i\n", old_addr, addr, x, y);
        }
        if (addr >= svga->vram_max)
                return 0xff;

#if 0
        egareads++;
        cycles -= video_timing_read_b;
        cycles_lost += video_timing_read_b;
#endif

//        pclog("read_linear: addr=%08x val=%02x\n", addr, svga->vram[addr & svga->vram_mask]);

        return svga->vram[addr & svga->vram_mask];
}

static uint16_t banshee_read_linear_w(uint32_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        
        if (addr & 1)
                return banshee_read_linear(addr, p) | (banshee_read_linear(addr+1, p) << 8);

#if 0
        cycles -= voodoo->read_time;
        cycles_lost += voodoo->read_time;
#endif

        addr &= svga->decode_mask;
        if (addr >= voodoo->tile_base)
        {
                int x, y;

                addr -= voodoo->tile_base;
                x = addr & (voodoo->tile_stride-1);
                y = addr >> voodoo->tile_stride_shift;

                addr = voodoo->tile_base + (x & 127) + ((x >> 7) * 128*32) + ((y & 31) * 128) + (y >> 5)*voodoo->tile_x_real;
//                pclog("  Tile rb %08x->%08x %i %i\n", old_addr, addr, x, y);
        }
        if (addr >= svga->vram_max)
                return 0xff;

#if 0
        egareads++;
        cycles -= video_timing_read_w;
        cycles_lost += video_timing_read_w;
#endif

//        pclog("read_linear: addr=%08x val=%02x\n", addr, svga->vram[addr & svga->vram_mask]);

        return *(uint16_t *)&svga->vram[addr & svga->vram_mask];
}

static uint32_t banshee_read_linear_l(uint32_t addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        
        if (addr & 3)
                return banshee_read_linear_w(addr, p) | (banshee_read_linear_w(addr+2, p) << 16);

#if 0
        cycles -= voodoo->read_time;
        cycles_lost += voodoo->read_time;
#endif

        addr &= svga->decode_mask;
        if (addr >= voodoo->tile_base)
        {
                int x, y;

                addr -= voodoo->tile_base;
                x = addr & (voodoo->tile_stride-1);
                y = addr >> voodoo->tile_stride_shift;

                addr = voodoo->tile_base + (x & 127) + ((x >> 7) * 128*32) + ((y & 31) * 128) + (y >> 5)*voodoo->tile_x_real;
//                pclog("  Tile rb %08x->%08x %i %i\n", old_addr, addr, x, y);
        }
        if (addr >= svga->vram_max)
                return 0xff;

#if 0
        egareads++;
        cycles -= video_timing_read_l;
        cycles_lost += video_timing_read_l;
#endif

//        pclog("read_linear: addr=%08x val=%02x\n", addr, svga->vram[addr & svga->vram_mask]);

        return *(uint32_t *)&svga->vram[addr & svga->vram_mask];
}

static void banshee_write_linear(uint32_t addr, uint8_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        
#if 0
        cycles -= voodoo->write_time;
        cycles_lost += voodoo->write_time;
#endif

        //        pclog("write_linear: addr=%08x val=%02x\n", addr, val);
        addr &= svga->decode_mask;
        if (addr >= voodoo->tile_base)
        {
                int x, y;

                addr -= voodoo->tile_base;
                x = addr & (voodoo->tile_stride-1);
                y = addr >> voodoo->tile_stride_shift;

                addr = voodoo->tile_base + (x & 127) + ((x >> 7) * 128*32) + ((y & 31) * 128) + (y >> 5)*voodoo->tile_x_real;
//                pclog("  Tile b %08x->%08x %i %i\n", old_addr, addr, x, y);
        }
        if (addr >= svga->vram_max)
                return;

#if 0
        egawrites++;

        cycles -= video_timing_write_b;
        cycles_lost += video_timing_write_b;
#endif
        svga->changedvram[addr >> 12] = changeframecount;
        svga->vram[addr & svga->vram_mask] = val;
}

static void banshee_write_linear_w(uint32_t addr, uint16_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        
        if (addr & 1)
        {
                banshee_write_linear(addr, val, p);
                banshee_write_linear(addr + 1, val >> 8, p);
                return;
        }

#if 0
        cycles -= voodoo->write_time;
        cycles_lost += voodoo->write_time;
#endif

//        pclog("write_linear: addr=%08x val=%02x\n", addr, val);
        addr &= svga->decode_mask;
        if (addr >= voodoo->tile_base)
        {
                int x, y;

                addr -= voodoo->tile_base;
                x = addr & (voodoo->tile_stride-1);
                y = addr >> voodoo->tile_stride_shift;

                addr = voodoo->tile_base + (x & 127) + ((x >> 7) * 128*32) + ((y & 31) * 128) + (y >> 5)*voodoo->tile_x_real;
//                pclog("  Tile b %08x->%08x %i %i\n", old_addr, addr, x, y);
        }
        if (addr >= svga->vram_max)
                return;

#if 0
        egawrites++;

        cycles -= video_timing_write_w;
        cycles_lost += video_timing_write_w;
#endif

        svga->changedvram[addr >> 12] = changeframecount;
        *(uint16_t *)&svga->vram[addr & svga->vram_mask] = val;
}

static void banshee_write_linear_l(uint32_t addr, uint32_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        svga_t *svga = &banshee->svga;
        int timing;

        if (addr & 3)
        {
                banshee_write_linear_w(addr, val, p);
                banshee_write_linear_w(addr + 2, val >> 16, p);
                return;
        }

#if 0
        if (addr == voodoo->last_write_addr+4)
                timing = voodoo->burst_time;
        else
                timing = voodoo->write_time;
        cycles -= timing;
        cycles_lost += timing;
#endif
        voodoo->last_write_addr = addr;

//        /*if (val) */pclog("write_linear_l: addr=%08x val=%08x  %08x\n", addr, val, voodoo->tile_base);
        addr &= svga->decode_mask;
        if (addr >= voodoo->tile_base)
        {
                int x, y;

                addr -= voodoo->tile_base;
                x = addr & (voodoo->tile_stride-1);
                y = addr >> voodoo->tile_stride_shift;
                
                addr = voodoo->tile_base + (x & 127) + ((x >> 7) * 128*32) + ((y & 31) * 128) + (y >> 5)*voodoo->tile_x_real;
//                pclog("  Tile %08x->%08x->%08x->%08x %i %i  tile_x=%i\n", old_addr, addr_off, addr2, addr, x, y, voodoo->tile_x_real);
        }

        if (addr >= svga->vram_max)
                return;

#if 0
        egawrites += 4;

        cycles -= video_timing_write_l;
        cycles_lost += video_timing_write_l;
#endif

        svga->changedvram[addr >> 12] = changeframecount;
        *(uint32_t *)&svga->vram[addr & svga->vram_mask] = val;
        if (voodoo->cmdfifo_enabled && addr >= voodoo->cmdfifo_base && addr < voodoo->cmdfifo_end)
        {
//                pclog("CMDFIFO write %08x %08x  old amin=%08x amax=%08x hlcnt=%i depth_wr=%i rp=%08x\n", addr, val, voodoo->cmdfifo_amin, voodoo->cmdfifo_amax, voodoo->cmdfifo_holecount, voodoo->cmdfifo_depth_wr, voodoo->cmdfifo_rp);
                if (addr == voodoo->cmdfifo_base && !voodoo->cmdfifo_holecount)
                {
//                        if (voodoo->cmdfifo_holecount)
//                                fatal("CMDFIFO reset pointers while outstanding holes\n");
                        /*Reset pointers*/
                        voodoo->cmdfifo_amin = voodoo->cmdfifo_base;
                        voodoo->cmdfifo_amax = voodoo->cmdfifo_base;
                        voodoo->cmdfifo_depth_wr++;
                        voodoo_wake_fifo_thread(voodoo);
                }
                else if (voodoo->cmdfifo_holecount)
                {
//                        if ((addr <= voodoo->cmdfifo_amin && voodoo->cmdfifo_amin != -4) || addr >= voodoo->cmdfifo_amax)
//                                fatal("CMDFIFO holecount write outside of amin/amax - amin=%08x amax=%08x holecount=%i\n", voodoo->cmdfifo_amin, voodoo->cmdfifo_amax, voodoo->cmdfifo_holecount);
//                        pclog("holecount %i\n", voodoo->cmdfifo_holecount);
                        voodoo->cmdfifo_holecount--;
                        if (!voodoo->cmdfifo_holecount)
                        {
                                /*Filled in holes, resume normal operation*/
                                voodoo->cmdfifo_depth_wr += ((voodoo->cmdfifo_amax - voodoo->cmdfifo_amin) >> 2);
                                voodoo->cmdfifo_amin = voodoo->cmdfifo_amax;
                                voodoo_wake_fifo_thread(voodoo);
//                                pclog("hole filled! amin=%08x amax=%08x added %i words\n", voodoo->cmdfifo_amin, voodoo->cmdfifo_amax, words_to_add);
                        }
                }
                else if (addr == voodoo->cmdfifo_amax+4)
                {
                        /*In-order write*/
                        voodoo->cmdfifo_amin = addr;
                        voodoo->cmdfifo_amax = addr;
                        voodoo->cmdfifo_depth_wr++;
                        voodoo_wake_fifo_thread(voodoo);
                }
                else
                {
                        /*Out-of-order write*/
                        if (addr < voodoo->cmdfifo_amin)
                        {
                                /*Reset back to start. Note that write is still out of order!*/
                                voodoo->cmdfifo_amin = voodoo->cmdfifo_base-4;

                        }
//                        else if (addr < voodoo->cmdfifo_amax)
//                                fatal("Out-of-order write really out of order\n");
                        voodoo->cmdfifo_amax = addr;
                        voodoo->cmdfifo_holecount = ((voodoo->cmdfifo_amax - voodoo->cmdfifo_amin) >> 2) - 1;
//                        pclog("CMDFIFO out of order: amin=%08x amax=%08x holecount=%i\n", voodoo->cmdfifo_amin, voodoo->cmdfifo_amax, voodoo->cmdfifo_holecount);
                }
        }
}

void banshee_hwcursor_draw(svga_t *svga, int displine)
{
        banshee_t *banshee = (banshee_t *)svga->priv;
        int x, c;
        int x_off;
        uint32_t col0 = banshee->hwCurC0;
        uint32_t col1 = banshee->hwCurC1;
        uint8_t plane0[8], plane1[8];

        for (c = 0; c < 8; c++)
                plane0[c] = svga->vram[svga->hwcursor_latch.addr + c];
        for (c = 0; c < 8; c++)
                plane1[c] = svga->vram[svga->hwcursor_latch.addr + c + 8];
        svga->hwcursor_latch.addr += 16;
        
        x_off = svga->hwcursor_latch.x;
        x_off <<= svga->horizontal_linedbl;

        if (banshee->vidProcCfg & VIDPROCCFG_CURSOR_MODE)
        {
                /*X11 mode*/
                for (x = 0; x < 64; x += 8)
                {
                        if (x_off > (32-8))
                        {
                                int xx;

                                for (xx = 0; xx < 8; xx++)
                                {
                                        if (plane0[x >> 3] & (1 << 7))
                                                ((uint32_t *)buffer32->line[displine])[x_off + xx] = (plane1[x >> 3] & (1 << 7)) ? col1 : col0;

                                        plane0[x >> 3] <<= 1;
                                        plane1[x >> 3] <<= 1;
                                }
                        }

                        x_off += 8;
                }
        }
        else
        {
                /*Windows mode*/
                for (x = 0; x < 64; x += 8)
                {
                        if (x_off > (32-8))
                        {
                                int xx;

                                for (xx = 0; xx < 8; xx++)
                                {
                                    if (((x_off + xx + svga->x_add) >= 0) && ((x_off + xx + svga->x_add) <= 2047)) {
                                        if (!(plane0[x >> 3] & (1 << 7)))
                                                ((uint32_t *)buffer32->line[displine])[x_off + xx] = (plane1[x >> 3] & (1 << 7)) ? col1 : col0;
                                        else if (plane1[x >> 3] & (1 << 7))
                                                ((uint32_t *)buffer32->line[displine])[x_off + xx] ^= 0xffffff;

                                        plane0[x >> 3] <<= 1;
                                        plane1[x >> 3] <<= 1;
                                    }
                                }
                        }

                        x_off += 8;
                }
        }
}

#define CLAMP(x) do                                     \
        {                                               \
                if ((x) & ~0xff)                        \
                        x = ((x) < 0) ? 0 : 0xff;       \
        }                               \
        while (0)

#define DECODE_RGB565(buf)                                              \
        do                                                              \
        {                                                               \
                int c;                                                  \
                int wp = 0;                                             \
                                                                        \
                for (c = 0; c < voodoo->overlay.overlay_bytes; c += 2)  \
                {                                                       \
                        uint16_t data = *(uint16_t *)src;               \
                        int r = data & 0x1f;                            \
                        int g = (data >> 5) & 0x3f;                     \
                        int b = data >> 11;                             \
                                                                        \
                        if (banshee->vidProcCfg & VIDPROCCFG_OVERLAY_CLUT_BYPASS) \
                                buf[wp++] = (r << 3) | (g << 10) | (b << 19); \
                        else                                            \
                                buf[wp++] = (clut[r << 3] & 0x0000ff) | \
                                            (clut[g << 2] & 0x00ff00) | \
                                            (clut[b << 3] & 0xff0000);  \
                        src += 2;                                       \
                }                                                       \
        } while (0)

#define DECODE_RGB565_TILED(buf)                                        \
        do                                                              \
        {                                                               \
                int c;                                                  \
                int wp = 0;                                             \
                uint32_t base_addr = (buf == banshee->overlay_buffer[1]) ? src_addr2 : src_addr;        \
                                                                        \
                for (c = 0; c < voodoo->overlay.overlay_bytes; c += 2) \
                {                                                       \
                        uint16_t data = *(uint16_t *)&svga->vram[(base_addr + (c & 127) + (c >> 7)*128*32) & svga->vram_mask];               \
                        int r = data & 0x1f;                            \
                        int g = (data >> 5) & 0x3f;                     \
                        int b = data >> 11;                             \
                                                                        \
                        if (banshee->vidProcCfg & VIDPROCCFG_OVERLAY_CLUT_BYPASS) \
                                buf[wp++] = (r << 3) | (g << 10) | (b << 19); \
                        else                                            \
                                buf[wp++] = (clut[r << 3] & 0x0000ff) | \
                                            (clut[g << 2] & 0x00ff00) | \
                                            (clut[b << 3] & 0xff0000);  \
                }                                                       \
        } while (0)

#define DECODE_YUYV422(buf)                                             \
        do                                                              \
        {                                                               \
                int c;                                                  \
                int wp = 0;                                             \
                                                                        \
                for (c = 0; c < voodoo->overlay.overlay_bytes; c += 4)  \
                {                                                       \
                        uint8_t y1, y2;                                 \
                        int8_t Cr, Cb;                                  \
                        int dR, dG, dB;                                 \
                        int r, g, b;                                    \
                                                                        \
                        y1 = src[0];                                    \
                        Cr = src[1] - 0x80;                             \
                        y2 = src[2];                                    \
                        Cb = src[3] - 0x80;                             \
                        src += 4;                                       \
                                                                        \
                        dR = (359*Cr) >> 8;                             \
                        dG = (88*Cb + 183*Cr) >> 8;                     \
                        dB = (453*Cb) >> 8;                             \
                                                                        \
                        r = y1 + dR;                                    \
                        CLAMP(r);                                       \
                        g = y1 - dG;                                    \
                        CLAMP(g);                                       \
                        b = y1 + dB;                                    \
                        CLAMP(b);                                       \
                        buf[wp++] = r | (g << 8) | (b << 16); \
                                                                        \
                        r = y2 + dR;                                    \
                        CLAMP(r);                                       \
                        g = y2 - dG;                                    \
                        CLAMP(g);                                       \
                        b = y2 + dB;                                    \
                        CLAMP(b);                                       \
                        buf[wp++] = r | (g << 8) | (b << 16); \
                }                                                       \
        } while (0)

#define DECODE_UYUV422(buf)                                             \
        do                                                              \
        {                                                               \
                int c;                                                  \
                int wp = 0;                                             \
                                                                        \
                for (c = 0; c < voodoo->overlay.overlay_bytes; c += 4)  \
                {                                                       \
                        uint8_t y1, y2;                                 \
                        int8_t Cr, Cb;                                  \
                        int dR, dG, dB;                                 \
                        int r, g, b;                                    \
                                                                        \
                        Cr = src[0] - 0x80;                             \
                        y1 = src[1];                                    \
                        Cb = src[2] - 0x80;                             \
                        y2 = src[3];                                    \
                        src += 4;                                       \
                                                                        \
                        dR = (359*Cr) >> 8;                             \
                        dG = (88*Cb + 183*Cr) >> 8;                     \
                        dB = (453*Cb) >> 8;                             \
                                                                        \
                        r = y1 + dR;                                    \
                        CLAMP(r);                                       \
                        g = y1 - dG;                                    \
                        CLAMP(g);                                       \
                        b = y1 + dB;                                    \
                        CLAMP(b);                                       \
                        buf[wp++] = r | (g << 8) | (b << 16); \
                                                                        \
                        r = y2 + dR;                                    \
                        CLAMP(r);                                       \
                        g = y2 - dG;                                    \
                        CLAMP(g);                                       \
                        b = y2 + dB;                                    \
                        CLAMP(b);                                       \
                        buf[wp++] = r | (g << 8) | (b << 16); \
                }                                                       \
        } while (0)


#define OVERLAY_SAMPLE(buf)                     \
        do                                      \
        {                                       \
                switch (banshee->overlay_pix_fmt)       \
                {                                       \
                        case 0:                         \
                        break;                          \
                                                        \
                        case OVERLAY_FMT_YUYV422:       \
                        DECODE_YUYV422(buf);            \
                        break;                          \
                                                        \
                        case OVERLAY_FMT_UYVY422:       \
                        DECODE_UYUV422(buf);            \
                        break;                          \
                                                        \
                        case OVERLAY_FMT_565:           \
                        case OVERLAY_FMT_565_DITHER:    \
                        if (banshee->vidProcCfg & VIDPROCCFG_OVERLAY_TILE)      \
                                DECODE_RGB565_TILED(buf);                       \
                        else                                                    \
                                DECODE_RGB565(buf);                             \
                        break;                          \
                }                                       \
        } while (0)

/* generate both filters for the static table here */
void voodoo_generate_vb_filters(voodoo_t *voodoo, int fcr, int fcg)
{
        int g, h;
        float difference, diffg;
        float thiscol, thiscolg;
	float clr, clg = 0;
	float hack = 1.0f;
	// pre-clamping

	fcr *= hack;
	fcg *= hack;


	/* box prefilter */
        for (g=0;g<256;g++)         	// pixel 1 - our target pixel we want to bleed into
        {
		for (h=0;h<256;h++)      // pixel 2 - our main pixel
		{
			float avg;
			float avgdiff;

			difference = (float)(g - h);
			avg = g;
			avgdiff = avg - h;

			avgdiff = avgdiff * 0.75f;
			if (avgdiff < 0) avgdiff *= -1;
			if (difference < 0) difference *= -1;

			thiscol = thiscolg = g;

			if (h > g)
			{
				clr = clg = avgdiff;

				if (clr>fcr) clr=fcr;
                                if (clg>fcg) clg=fcg;

				thiscol = g;
				thiscolg = g;

				if (thiscol>g+fcr)
					thiscol=g+fcr;
				if (thiscolg>g+fcg)
					thiscolg=g+fcg;

				if (thiscol>g+difference)
					thiscol=g+difference;
				if (thiscolg>g+difference)
					thiscolg=g+difference;

				// hmm this might not be working out..
				int ugh = g - h;
				if (ugh < fcr)
					thiscol = h;
				if (ugh < fcg)
					thiscolg = h;
			}

			if (difference > fcr)
				thiscol = g;
			if (difference > fcg)
				thiscolg = g;

			// clamp
			if (thiscol < 0) thiscol = 0;
			if (thiscolg < 0) thiscolg = 0;

			if (thiscol > 255) thiscol = 255;
			if (thiscolg > 255) thiscolg = 255;

			vb_filter_bx_rb[g][h] = (thiscol);
			vb_filter_bx_g [g][h] = (thiscolg);

                }
		float lined = g + 4;
                if (lined > 255)
                        lined = 255;
                voodoo->purpleline[g][0] = lined;
                voodoo->purpleline[g][2] = lined;

                lined = g + 0;
                if (lined > 255)
                        lined = 255;
                voodoo->purpleline[g][1] = lined;
        }

        /* 4x1 and 2x2 filter */
	//fcr *= 5;
	//fcg *= 6;

        for (g=0;g<256;g++)         // pixel 1
        {
                for (h=0;h<256;h++)      // pixel 2
                {
                        difference = (float)(h - g);
                        diffg = difference;

			thiscol = thiscolg =  g;

                        if (difference > fcr)
                                difference = fcr;
                        if (difference < -fcr)
                                difference = -fcr;

                        if (diffg > fcg)
                                diffg = fcg;
                        if (diffg < -fcg)
                                diffg = -fcg;

			if ((difference < fcr) || (-difference > -fcr))
                                thiscol =  g + (difference / 2);
			if ((diffg < fcg) || (-diffg > -fcg))
                                thiscolg =  g + (diffg / 2);

                        if (thiscol < 0)
                                thiscol = 0;
                        if (thiscol > 255)
                                thiscol = 255;

                        if (thiscolg < 0)
                                thiscolg = 0;
                        if (thiscolg > 255)
                                thiscolg = 255;

                        vb_filter_v1_rb[g][h] = thiscol;
                        vb_filter_v1_g [g][h] = thiscolg;

                }
        }

}


static void banshee_overlay_draw(svga_t *svga, int displine)
{
        banshee_t *banshee = (banshee_t *)svga->priv;
        voodoo_t *voodoo = banshee->voodoo;
        uint32_t *p;
        int x;
        int y = voodoo->overlay.src_y >> 20;
        uint32_t src_addr = svga->overlay_latch.addr + ((banshee->vidProcCfg & VIDPROCCFG_OVERLAY_TILE) ?
                ((y & 31) * 128 + (y >> 5) * svga->overlay_latch.pitch) :
                y * svga->overlay_latch.pitch);
        uint32_t src_addr2 = svga->overlay_latch.addr + ((banshee->vidProcCfg & VIDPROCCFG_OVERLAY_TILE) ?
                (((y + 1) & 31) * 128 + ((y + 1) >> 5) * svga->overlay_latch.pitch) :
                (y + 1) * svga->overlay_latch.pitch);
        uint8_t *src = &svga->vram[src_addr & svga->vram_mask];
        uint32_t src_x = 0;
        unsigned int y_coeff = (voodoo->overlay.src_y & 0xfffff) >> 4;
        int skip_filtering;
        uint32_t *clut = &svga->pallook[(banshee->vidProcCfg & VIDPROCCFG_OVERLAY_CLUT_SEL) ? 256 : 0];

        if (svga->render == svga_render_null &&
                        !svga->changedvram[src_addr >> 12] && !svga->changedvram[src_addr2 >> 12] &&
                        !svga->fullchange &&
                        ((voodoo->overlay.src_y >> 20) < 2048 && !voodoo->dirty_line[voodoo->overlay.src_y >> 20]) &&
                        !(banshee->vidProcCfg & VIDPROCCFG_V_SCALE_ENABLE))
        {
                voodoo->overlay.src_y += (1 << 20);
                return;
        }

        if ((voodoo->overlay.src_y >> 20) < 2048)
                voodoo->dirty_line[voodoo->overlay.src_y >> 20] = 0;
//        pclog("displine=%i addr=%08x %08x  %08x  %08x\n", displine, svga->overlay_latch.addr, src_addr, voodoo->overlay.vidOverlayDvdy, *(uint32_t *)src);
//        if (src_addr >= 0x800000)
//                fatal("overlay out of range!\n");
        p = &((uint32_t *)buffer32->line[displine])[svga->overlay_latch.x + 32];

        if (banshee->voodoo->scrfilter && banshee->voodoo->scrfilterEnabled)
                skip_filtering = ((banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK) != VIDPROCCFG_FILTER_MODE_BILINEAR &&
                            !(banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE) && !(banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_DITHER_4X4) &&
                            !(banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_DITHER_2X2));
        else
                skip_filtering = ((banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK) != VIDPROCCFG_FILTER_MODE_BILINEAR &&
                                !(banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE));

        if (skip_filtering)
        {
                /*No scaling or filtering required, just write straight to output buffer*/
                OVERLAY_SAMPLE(p);
        }
        else
        {
                OVERLAY_SAMPLE(banshee->overlay_buffer[0]);

                switch (banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK)
                {
                        case VIDPROCCFG_FILTER_MODE_BILINEAR:
                        src = &svga->vram[src_addr2 & svga->vram_mask];
                        OVERLAY_SAMPLE(banshee->overlay_buffer[1]);
                        if (banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE)
                        {
                                for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                {
                                        unsigned int x_coeff = (src_x & 0xfffff) >> 4;
                                        unsigned int coeffs[4] = {
                                                ((0x10000 - x_coeff) * (0x10000 - y_coeff)) >> 16,
                                                (           x_coeff  * (0x10000 - y_coeff)) >> 16,
                                                ((0x10000 - x_coeff) *            y_coeff) >> 16,
                                                (           x_coeff  *            y_coeff) >> 16
                                        };
                                        uint32_t samp0 = banshee->overlay_buffer[0][src_x >> 20];
                                        uint32_t samp1 = banshee->overlay_buffer[0][(src_x >> 20) + 1];
                                        uint32_t samp2 = banshee->overlay_buffer[1][src_x >> 20];
                                        uint32_t samp3 = banshee->overlay_buffer[1][(src_x >> 20) + 1];
                                        int r = (((samp0 >> 16) & 0xff) * coeffs[0] +
                                                ((samp1 >> 16) & 0xff) * coeffs[1] +
                                                ((samp2 >> 16) & 0xff) * coeffs[2] +
                                                ((samp3 >> 16) & 0xff) * coeffs[3]) >> 16;
                                        int g = (((samp0 >> 8) & 0xff) * coeffs[0] +
                                                ((samp1 >> 8) & 0xff) * coeffs[1] +
                                                ((samp2 >> 8) & 0xff) * coeffs[2] +
                                                ((samp3 >> 8) & 0xff) * coeffs[3]) >> 16;
                                        int b = ((samp0 & 0xff) * coeffs[0] +
                                                (samp1 & 0xff) * coeffs[1] +
                                                (samp2 & 0xff) * coeffs[2] +
                                                (samp3 & 0xff) * coeffs[3]) >> 16;
                                        p[x] = (r << 16) | (g << 8) | b;

                                        src_x += voodoo->overlay.vidOverlayDudx;
                                }
                        }
                        else
                        {
                                for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                {
                                        uint32_t samp0 = banshee->overlay_buffer[0][src_x >> 20];
                                        uint32_t samp1 = banshee->overlay_buffer[1][src_x >> 20];
                                        int r = (((samp0 >> 16) & 0xff) * (0x10000 - y_coeff) +
                                                ((samp1 >> 16) & 0xff) * y_coeff) >> 16;
                                        int g = (((samp0 >> 8) & 0xff) * (0x10000 - y_coeff) +
                                                ((samp1 >> 8) & 0xff) * y_coeff) >> 16;
                                        int b = ((samp0 & 0xff) * (0x10000 - y_coeff) +
                                                (samp1 & 0xff) * y_coeff) >> 16;
                                        p[x] = (r << 16) | (g << 8) | b;
                                }
                        }
                        break;
                
                        case VIDPROCCFG_FILTER_MODE_DITHER_4X4:
                        if (banshee->voodoo->scrfilter && banshee->voodoo->scrfilterEnabled)
                        {
                                //uint8_t fil[(svga->overlay_latch.xsize) * 3];
                                //uint8_t fil3[(svga->overlay_latch.xsize) * 3];
                                uint8_t fil[4096 * 3];
                                uint8_t fil3[4096 * 3];


                                if (banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE) /* leilei HACK - don't know of real 4x1 hscaled behavior yet, double for now */
                                {
                                        for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                        {
                                                fil[x*3] 	= ((banshee->overlay_buffer[0][src_x >> 20]));
                                                fil[x*3+1] 	= ((banshee->overlay_buffer[0][src_x >> 20] >> 8));
                                                fil[x*3+2] 	= ((banshee->overlay_buffer[0][src_x >> 20] >> 16));
                                                fil3[x*3+0] 	= fil[x*3+0];
                                                fil3[x*3+1] 	= fil[x*3+1];
                                                fil3[x*3+2] 	= fil[x*3+2];
                                                src_x += voodoo->overlay.vidOverlayDudx;
                                        }
                                }
                                else
                                {
                                        for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                        {
                                                fil[x*3] 	= ((banshee->overlay_buffer[0][x]));
                                                fil[x*3+1] 	= ((banshee->overlay_buffer[0][x] >> 8));
                                                fil[x*3+2] 	= ((banshee->overlay_buffer[0][x] >> 16));
                                                fil3[x*3+0] 	= fil[x*3+0];
                                                fil3[x*3+1] 	= fil[x*3+1];
                                                fil3[x*3+2] 	= fil[x*3+2];
                                        }
                                }
                                if (y % 2 == 0)
                                {
                                        for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                        {
                                                fil[x*3] = banshee->voodoo->purpleline[fil[x*3+0]][0];
                                                fil[x*3+1] = banshee->voodoo->purpleline[fil[x*3+1]][1];
                                                fil[x*3+2] = banshee->voodoo->purpleline[fil[x*3+2]][2];
                                        }
                                }

                                for (x=1; x<svga->overlay_latch.cur_xsize;x++)
                                {
                                        fil3[(x)*3]   = vb_filter_v1_rb [fil[x*3]]  [fil[(x-1) *3]];
                                        fil3[(x)*3+1] = vb_filter_v1_g  [fil[x*3+1]][fil[(x-1) *3+1]];
                                        fil3[(x)*3+2] = vb_filter_v1_rb [fil[x*3+2]] [fil[(x-1) *3+2]];
                                }
                                for (x=1; x<svga->overlay_latch.cur_xsize;x++)
                                {
                                        fil[(x)*3]   = vb_filter_v1_rb [fil[x*3]]  [fil3[(x-1) *3]];
                                        fil[(x)*3+1] = vb_filter_v1_g  [fil[x*3+1]][fil3[(x-1) *3+1]];
                                        fil[(x)*3+2] = vb_filter_v1_rb [fil[x*3+2]] [fil3[(x-1) *3+2]];
                                }
                                for (x=1; x<svga->overlay_latch.cur_xsize;x++)
                                {
                                        fil3[(x)*3]   = vb_filter_v1_rb [fil[x*3]]  [fil[(x-1) *3]];
                                        fil3[(x)*3+1] = vb_filter_v1_g  [fil[x*3+1]][fil[(x-1) *3+1]];
                                        fil3[(x)*3+2] = vb_filter_v1_rb [fil[x*3+2]] [fil[(x-1) *3+2]];
                                }
                                for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                {
                                        fil[(x)*3]   = vb_filter_v1_rb [fil[x*3]]  [fil3[(x+1) *3]];
                                        fil[(x)*3+1] = vb_filter_v1_g  [fil[x*3+1]][fil3[(x+1) *3+1]];
                                        fil[(x)*3+2] = vb_filter_v1_rb [fil[x*3+2]] [fil3[(x+1) *3+2]];
                                        p[x] = (fil[x*3+2] << 16) | (fil[x*3+1] << 8) | fil[x*3];
                                }
                        }
                        else  /* filter disabled by emulator option */
                        {
                                if (banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE)
                                {
                                        for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                        {
                                                p[x] = banshee->overlay_buffer[0][src_x >> 20];
                                                src_x += voodoo->overlay.vidOverlayDudx;
                                        }
                                }
                                else
                                {
                                        for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                                p[x] = banshee->overlay_buffer[0][x];
                                }
                        }
                        break;

                        case VIDPROCCFG_FILTER_MODE_DITHER_2X2:
                        if (banshee->voodoo->scrfilter && banshee->voodoo->scrfilterEnabled)
                        {
                                //uint8_t fil[(svga->overlay_latch.xsize) * 3];
                                //uint8_t soak[(svga->overlay_latch.xsize) * 3];
                                //uint8_t soak2[(svga->overlay_latch.xsize) * 3];
                                //uint8_t samp1[(svga->overlay_latch.xsize) * 3];
                                //uint8_t samp2[(svga->overlay_latch.xsize) * 3];
                                //uint8_t samp3[(svga->overlay_latch.xsize) * 3];
                                //uint8_t samp4[(svga->overlay_latch.xsize) * 3];

                                uint8_t fil[4096 * 3];
                                uint8_t soak[4096 * 3];
                                uint8_t soak2[4096 * 3];
                                uint8_t samp1[4096 * 3];
                                uint8_t samp2[4096 * 3];
                                uint8_t samp3[4096 * 3];
                                uint8_t samp4[4096 * 3];


                                src = &svga->vram[src_addr2 & svga->vram_mask];
                                OVERLAY_SAMPLE(banshee->overlay_buffer[1]);
                                for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                {
                                        samp1[x*3] 	= ((banshee->overlay_buffer[0][x]));
                                        samp1[x*3+1] 	= ((banshee->overlay_buffer[0][x] >> 8));
                                        samp1[x*3+2] 	= ((banshee->overlay_buffer[0][x] >> 16));

                                        samp2[x*3+0] 	= ((banshee->overlay_buffer[0][x+1]));
                                        samp2[x*3+1] 	= ((banshee->overlay_buffer[0][x+1] >> 8));
                                        samp2[x*3+2] 	= ((banshee->overlay_buffer[0][x+1] >> 16));

                                        samp3[x*3+0] 	= ((banshee->overlay_buffer[1][x]));
                                        samp3[x*3+1] 	= ((banshee->overlay_buffer[1][x] >> 8));
                                        samp3[x*3+2] 	= ((banshee->overlay_buffer[1][x] >> 16));

                                        samp4[x*3+0] 	= ((banshee->overlay_buffer[1][x+1]));
                                        samp4[x*3+1] 	= ((banshee->overlay_buffer[1][x+1] >> 8));
                                        samp4[x*3+2] 	= ((banshee->overlay_buffer[1][x+1] >> 16));

                                        /* sample two lines */

                                        soak[x*3+0]   = vb_filter_bx_rb [samp1[x*3+0]]   [samp2[x*3+0]];
                                        soak[x*3+1]   = vb_filter_bx_g  [samp1[x*3+1]]   [samp2[x*3+1]];
                                        soak[x*3+2]   = vb_filter_bx_rb [samp1[x*3+2]]   [samp2[x*3+2]];

                                        soak2[x*3+0]   = vb_filter_bx_rb[samp3[x*3+0]]   [samp4[x*3+0]];
                                        soak2[x*3+1]   = vb_filter_bx_g [samp3[x*3+1]]   [samp4[x*3+1]];
                                        soak2[x*3+2]   = vb_filter_bx_rb[samp3[x*3+2]]   [samp4[x*3+2]];

                                        /* then pour it on the rest */

                                        fil[x*3+0]   = vb_filter_v1_rb[soak[x*3+0]]   [soak2[x*3+0]];
                                        fil[x*3+1]   = vb_filter_v1_g [soak[x*3+1]]   [soak2[x*3+1]];
                                        fil[x*3+2]   = vb_filter_v1_rb[soak[x*3+2]]   [soak2[x*3+2]];
                                }

                                if (banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE)  /* 2x2 on a scaled low res */
                                {
                                        for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                        {
                                                p[x] = (fil[(src_x >> 20)*3+2] << 16) | (fil[(src_x >> 20)*3+1] << 8) | fil[(src_x >> 20)*3];
                                                src_x += voodoo->overlay.vidOverlayDudx;
                                        }
                                }
                                else
                                {
                                        for (x=0; x<svga->overlay_latch.cur_xsize;x++)
                                        {
                                                p[x] = (fil[x*3+2] << 16) | (fil[x*3+1] << 8) | fil[x*3];
                                        }
                                }
                        }
                        else  /* filter disabled by emulator option */
                        {
                                if (banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE)
                                {
                                        for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                        {
                                                p[x] = banshee->overlay_buffer[0][src_x >> 20];

                                                src_x += voodoo->overlay.vidOverlayDudx;
                                        }
                                }
                                else
                                {
                                        for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                                p[x] = banshee->overlay_buffer[0][x];
                                }
 		        }
                        break;

                        case VIDPROCCFG_FILTER_MODE_POINT:
                        default:
                        if (banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE)
                        {
                                for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                {
                                        p[x] = banshee->overlay_buffer[0][src_x >> 20];

                                        src_x += voodoo->overlay.vidOverlayDudx;
                                }
                        }
                        else
                        {
                                for (x = 0; x < svga->overlay_latch.cur_xsize; x++)
                                        p[x] = banshee->overlay_buffer[0][x];
                        }
                        break;
                }
        }
        
        if (banshee->vidProcCfg & VIDPROCCFG_V_SCALE_ENABLE)
                voodoo->overlay.src_y += voodoo->overlay.vidOverlayDvdy;
        else
                voodoo->overlay.src_y += (1 << 20);
}

void banshee_set_overlay_addr(void *p, uint32_t addr)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        
        banshee->svga.overlay.addr = banshee->voodoo->leftOverlayBuf & 0xfffffff;
        banshee->svga.overlay_latch.addr = banshee->voodoo->leftOverlayBuf & 0xfffffff;
        memset(voodoo->dirty_line, 1, sizeof(voodoo->dirty_line));
}

static void banshee_vsync_callback(svga_t *svga)
{
        banshee_t *banshee = (banshee_t *)svga->priv;
        voodoo_t *voodoo = banshee->voodoo;

        voodoo->retrace_count++;
        thread_wait_mutex(voodoo->swap_mutex);
        if (voodoo->swap_pending && (voodoo->retrace_count > voodoo->swap_interval))
        {
                if (voodoo->swap_count > 0)
                        voodoo->swap_count--;
                voodoo->swap_pending = 0;
                thread_release_mutex(voodoo->swap_mutex);

                memset(voodoo->dirty_line, 1, sizeof(voodoo->dirty_line));
                voodoo->retrace_count = 0;
                banshee_set_overlay_addr(banshee, voodoo->swap_offset);
                thread_set_event(voodoo->wake_fifo_thread);
                voodoo->frame_count++;
        }
        else
                thread_release_mutex(voodoo->swap_mutex);

        voodoo->overlay.src_y = 0;
        banshee->desktop_addr = banshee->vidDesktopStartAddr;
        banshee->desktop_y = 0;
}

static uint8_t banshee_pci_read(int func, int addr, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
//        svga_t *svga = &banshee->svga;
        uint8_t ret = 0;

        if (func)
                return 0xff;
//        pclog("Banshee PCI read %08X  ", addr);
        switch (addr)
        {
                case 0x00: ret = 0x1a; break; /*3DFX*/
                case 0x01: ret = 0x12; break;
                
                case 0x02: ret = (banshee->type == TYPE_BANSHEE) ? 0x03 : 0x05; break;
                case 0x03: ret = 0x00; break;

                case 0x04: ret = banshee->pci_regs[0x04] & 0x27; break;
                
                case 0x07: ret = banshee->pci_regs[0x07] & 0x36; break;
                                
                case 0x08: ret = (banshee->type == TYPE_BANSHEE) ? 3 : 1; break; /*Revision ID*/
                case 0x09: ret = 0; break; /*Programming interface*/
                
                case 0x0a: ret = 0x00; break; /*Supports VGA interface*/
                case 0x0b: ret = 0x03; /*output = 3; */break;

                case 0x0d: ret = banshee->pci_regs[0x0d] & 0xf8; break;
                                
                case 0x10: ret = 0x00; break; /*memBaseAddr0*/
                case 0x11: ret = 0x00; break;
                case 0x12: ret = 0x00; break;
                case 0x13: ret = banshee->memBaseAddr0 >> 24; break;

                case 0x14: ret = 0x00; break; /*memBaseAddr1*/
                case 0x15: ret = 0x00; break;
                case 0x16: ret = 0x00; break;
                case 0x17: ret = banshee->memBaseAddr1 >> 24; break;
                
                case 0x18: ret = 0x01; break; /*ioBaseAddr*/
                case 0x19: ret = banshee->ioBaseAddr >> 8; break;
                case 0x1a: ret = banshee->ioBaseAddr >> 16; break;
                case 0x1b: ret = banshee->ioBaseAddr >> 24; break;

                /*Subsystem vendor ID*/
                case 0x2c: ret = banshee->pci_regs[0x2c]; break;
                case 0x2d: ret = banshee->pci_regs[0x2d]; break;
                case 0x2e: ret = banshee->pci_regs[0x2e]; break;
                case 0x2f: ret = banshee->pci_regs[0x2f]; break;

                case 0x30: ret = banshee->pci_regs[0x30] & 0x01; break; /*BIOS ROM address*/
                case 0x31: ret = 0x00; break;
                case 0x32: ret = banshee->pci_regs[0x32]; break;
                case 0x33: ret = banshee->pci_regs[0x33]; break;

                case 0x3c: ret = banshee->pci_regs[0x3c]; break;
                                
                case 0x3d: ret = 0x01; break; /*INTA*/
                
                case 0x3e: ret = 0x04; break;
                case 0x3f: ret = 0xff; break;
                
        }
//        pclog("%02X\n", ret);
        return ret;
}

static void banshee_pci_write(int func, int addr, uint8_t val, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
//        svga_t *svga = &banshee->svga;
        uint32_t basemask = 0xfe;

        if (func)
                return;
//        pclog("Banshee write %08X %02X %04X:%08X\n", addr, val, CS, cpu_state.pc);
        switch (addr)
        {
                case 0x00: case 0x01: case 0x02: case 0x03:
                case 0x08: case 0x09: case 0x0a: case 0x0b:
                case 0x3d: case 0x3e: case 0x3f:
                return;
                
                case PCI_REG_COMMAND:
                if (val & PCI_COMMAND_IO)
                {
                        io_removehandlerx(0x03c0, 0x0020, banshee_in, NULL, NULL, banshee_out, NULL, NULL, banshee);
                        if (banshee->ioBaseAddr)
                                io_removehandlerx(banshee->ioBaseAddr, 0x0100, banshee_ext_in, NULL, banshee_ext_inl, banshee_ext_out, NULL, banshee_ext_outl, banshee);

                        io_sethandlerx(0x03c0, 0x0020, banshee_in, NULL, NULL, banshee_out, NULL, NULL, banshee);
                        if (banshee->ioBaseAddr)
                                io_sethandlerx(banshee->ioBaseAddr, 0x0100, banshee_ext_in, NULL, banshee_ext_inl, banshee_ext_out, NULL, banshee_ext_outl, banshee);
                }
                else
                {
                        io_removehandlerx(0x03c0, 0x0020, banshee_in, NULL, NULL, banshee_out, NULL, NULL, banshee);
                        io_removehandlerx(banshee->ioBaseAddr, 0x0100, banshee_ext_in, NULL, banshee_ext_inl, banshee_ext_out, NULL, banshee_ext_outl, banshee);
                }
                banshee->pci_regs[PCI_REG_COMMAND] = val & 0x27;
                banshee_updatemapping(banshee);
                return;
                case 0x07:
                banshee->pci_regs[0x07] = val & 0x3e;
                return;
                case 0x0d: 
                banshee->pci_regs[0x0d] = val & 0xf8;
                return;
                
                case 0x13:
                banshee->memBaseAddr0 = (val & basemask) << 24;
                banshee_updatemapping(banshee);
                return;

                case 0x17:
                banshee->memBaseAddr1 = (val & basemask) << 24;
                banshee_updatemapping(banshee);
                return;

                case 0x1a:
                    banshee->ioBaseAddr &= 0xff00ffff;
                    banshee->ioBaseAddr |= val << 16;
                    break;
                case 0x1b:
                    banshee->ioBaseAddr &= 0x00ffffff;
                    banshee->ioBaseAddr |= val << 24;
                    break;

                case 0x19:
                if (banshee->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_IO)
                        io_removehandlerx(banshee->ioBaseAddr, 0x0100, banshee_ext_in, NULL, banshee_ext_inl, banshee_ext_out, NULL, banshee_ext_outl, banshee);
                banshee->ioBaseAddr &= 0xffff00ff;
                banshee->ioBaseAddr |= val << 8;
                if ((banshee->pci_regs[PCI_REG_COMMAND] & PCI_COMMAND_IO) && banshee->ioBaseAddr)
                        io_sethandlerx(banshee->ioBaseAddr, 0x0100, banshee_ext_in, NULL, banshee_ext_inl, banshee_ext_out, NULL, banshee_ext_outl, banshee);
                pclog("Banshee ioBaseAddr=%08x\n", banshee->ioBaseAddr);
//                s3_virge_updatemapping(virge); 
                return;

                case 0x30: case 0x32: case 0x33:
                banshee->pci_regs[addr] = val;
                if (banshee->pci_regs[0x30] & 0x01)
                {
                        uint32_t addr = (banshee->pci_regs[0x32] << 16) | (banshee->pci_regs[0x33] << 24);
                        pclog("Banshee bios_rom enabled at %08x\n", addr);
                        mem_mapping_set_addrx(&banshee->bios_rom.mapping, addr, 0x10000);
                        mem_mapping_enablex(&banshee->bios_rom.mapping);
                }
                else
                {
                        pclog("Banshee bios_rom disabled\n");
                        mem_mapping_disablex(&banshee->bios_rom.mapping);
                }
                return;
                case 0x3c: 
                banshee->pci_regs[0x3c] = val;
                return;
        }
}

static device_config_t banshee_sgram_config[] =
{
        {
                "memory", // name
                "Memory size", // description
                CONFIG_SELECTION, // type
                "", // default_string
                16, // default_int
                {
                        {
                                "8 MB", // description
                                8 // value
                        },
                        {
                                "16 MB", // description
                                16 // value
                        },
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "bilinear", // name
                "Bilinear filtering", // description
                CONFIG_BINARY, // type
                "", // default_string
                1, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "dithersub", // name
                "Dither subtraction", // description
                CONFIG_BINARY, // type
                "", // default_string
                1, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "dacfilter", // name
                "Screen Filter", // description
                CONFIG_BINARY, // type
                "", // default_string
                0, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "render_threads", // name
                "Render threads", // description
                CONFIG_SELECTION, // type
                "", // default_string
                2, // default_int
                {
                        {
                                "1", // description
                                1 // value
                        },
                        {
                                "2", // description
                                2 // value
                        },
                        {
                                "4", // description
                                4 // value
                        },
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
#ifndef NO_CODEGEN
        {
                "recompiler", // name
                "Recompiler", // description
                CONFIG_BINARY, // type
                "", // default_string
                1, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
#endif
        {
                "", // name (end of config)
                "", // description
                -1, // type
                "", // default_string
                0, // default_int
                {
                        {
                                "", // description
                                0 // value
                        }
                }
        }
};

static device_config_t banshee_sdram_config[] =
{
        {
                "bilinear", // name
                "Bilinear filtering", // description
                CONFIG_BINARY, // type
                "", // default_string
                1, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "dithersub", // name
                "Dither subtraction", // description
                CONFIG_BINARY, // type
                "", // default_string
                1, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "dacfilter", // name
                "Screen Filter", // description
                CONFIG_BINARY, // type
                "", // default_string
                0, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
        {
                "render_threads", // name
                "Render threads", // description
                CONFIG_SELECTION, // type
                "", // default_string
                2, // default_int
                {
                        {
                                "1", // description
                                1 // value
                        },
                        {
                                "2", // description
                                2 // value
                        },
                        {
                                "4", // description
                                4 // value
                        },
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
#ifndef NO_CODEGEN
        {
                "recompiler", // name
                "Recompiler", // description
                CONFIG_BINARY, // type
                "", // default_string
                1, // default_int
                {
                        {
                                "", // description (end of selection)
                                0 // value
                        }
                }
        },
#endif
        {
                "", // name (end of config)
                "", // description
                -1, // type
                "", // default_string
                0, // default_int
                {
                        {
                                "", // description
                                0 // value
                        }
                }
        }
};

void voodoo_update_vram(void *p)
{
    banshee_t *banshee = (banshee_t*)p;
 
    banshee->voodoo->vram = banshee->svga.vram;
    banshee->voodoo->fb_mem = banshee->svga.vram;
    banshee->voodoo->tex_mem[0] = banshee->svga.vram;
    banshee->voodoo->tex_mem_w[0] = (uint16_t*)banshee->svga.vram;
    banshee->voodoo->tex_mem[1] = banshee->svga.vram;
    banshee->voodoo->tex_mem_w[1] = (uint16_t*)banshee->svga.vram;
}

static void *banshee_init_common(const char *fn, int has_sgram, int type, int voodoo_type)
{
        int mem_size;
        banshee_t *banshee = (banshee_t*)malloc(sizeof(banshee_t));
        memset(banshee, 0, sizeof(banshee_t));
        
        banshee->type = type;
        banshee->intrCtrl = 0x80000000;

        rom_init(&banshee->bios_rom, fn, 0xc0000, 0x10000, 0xffff, 0, MEM_MAPPING_EXTERNAL);
        mem_mapping_disablex(&banshee->bios_rom.mapping);

        if (has_sgram)
                mem_size = device_get_config_int("memory");
        else
                mem_size = 16; /*SDRAM Banshee only supports 16 MB*/

        svga_init(NULL, &banshee->svga, banshee, mem_size << 20,
                   banshee_recalctimings,
                   banshee_in, banshee_out,
                   banshee_hwcursor_draw,
                   banshee_overlay_draw);
        banshee->svga.vsync_callback = banshee_vsync_callback;

        mem_mapping_addx(&banshee->linear_mapping, 0, 0, banshee_read_linear,
                                                        banshee_read_linear_w,
                                                        banshee_read_linear_l,
                                                        banshee_write_linear,
                                                        banshee_write_linear_w,
                                                        banshee_write_linear_l,
                                                        NULL,
                                                        MEM_MAPPING_EXTERNAL,
                                                        &banshee->svga);
        mem_mapping_addx(&banshee->reg_mapping_low, 0, 0,banshee_reg_read,
                                                        banshee_reg_readw,
                                                        banshee_reg_readl,
                                                        banshee_reg_write,
                                                        banshee_reg_writew,
                                                        banshee_reg_writel,
                                                        NULL,
                                                        MEM_MAPPING_EXTERNAL,
                                                        banshee);
        mem_mapping_addx(&banshee->reg_mapping_high, 0,0,banshee_reg_read,
                                                        banshee_reg_readw,
                                                        banshee_reg_readl,
                                                        banshee_reg_write,
                                                        banshee_reg_writew,
                                                        banshee_reg_writel,
                                                        NULL,
                                                        MEM_MAPPING_EXTERNAL,
                                                        banshee);

//        io_sethandlerx(0x03c0, 0x0020, banshee_in, NULL, NULL, banshee_out, NULL, NULL, banshee);

        banshee->svga.bpp = 8;
        banshee->svga.miscout = 1;
        
        banshee->dramInit0 = 1 << 27;
        if (has_sgram && mem_size == 16)
                banshee->dramInit0 |= (1 << 26); /*2xSGRAM = 16 MB*/
        if (!has_sgram)
                banshee->dramInit1 = 1 << 30; /*SDRAM*/
        banshee->svga.decode_mask = 0x1ffffff;

        pci_add(banshee_pci_read, banshee_pci_write, banshee);
        
        banshee->voodoo = (voodoo_t*)voodoo_2d3d_card_init(voodoo_type);
        banshee->voodoo->p = banshee;
        banshee->voodoo->vram = banshee->svga.vram;
        banshee->voodoo->changedvram = banshee->svga.changedvram;
        banshee->voodoo->fb_mem = banshee->svga.vram;
        banshee->voodoo->fb_mask = banshee->svga.vram_mask;
        banshee->voodoo->tex_mem[0] = banshee->svga.vram;
        banshee->voodoo->tex_mem_w[0] = (uint16_t *)banshee->svga.vram;
        banshee->voodoo->tex_mem[1] = banshee->svga.vram;
        banshee->voodoo->tex_mem_w[1] = (uint16_t *)banshee->svga.vram;
        banshee->voodoo->texture_mask = banshee->svga.vram_mask;
        voodoo_generate_filter_v1(banshee->voodoo);

        banshee->vidSerialParallelPort = VIDSERIAL_DDC_DCK_W | VIDSERIAL_DDC_DDA_W;

        //ddc_init();

        switch (type)
        {
                case TYPE_BANSHEE:
                if (has_sgram)
                {
                        banshee->pci_regs[0x2c] = 0x1a;
                        banshee->pci_regs[0x2d] = 0x12;
                        banshee->pci_regs[0x2e] = 0x04;
                        banshee->pci_regs[0x2f] = 0x00;
                }
                else
                {
                        banshee->pci_regs[0x2c] = 0x02;
                        banshee->pci_regs[0x2d] = 0x11;
                        banshee->pci_regs[0x2e] = 0x17;
                        banshee->pci_regs[0x2f] = 0x10;
                }
                break;

                case TYPE_V3_2000:
                banshee->pci_regs[0x2c] = 0x1a;
                banshee->pci_regs[0x2d] = 0x12;
                banshee->pci_regs[0x2e] = 0x30;
                banshee->pci_regs[0x2f] = 0x00;
                break;

                case TYPE_V3_3000:
                banshee->pci_regs[0x2c] = 0x1a;
                banshee->pci_regs[0x2d] = 0x12;
                banshee->pci_regs[0x2e] = 0x3a;
                banshee->pci_regs[0x2f] = 0x00;
                break;
        }

        banshee->svga.vsync_callback = banshee_vblank_start;

        return banshee;
}

static void *banshee_init(const device_t *info)
{
        return banshee_init_common("pci_sg.rom", 1, TYPE_BANSHEE, VOODOO_BANSHEE);
}
static void *creative_banshee_init(const device_t *info)
{
        return banshee_init_common("blasterpci.rom", 0, TYPE_BANSHEE, VOODOO_BANSHEE);
}
static void *v3_2000_init(const device_t *info)
{
        return banshee_init_common("voodoo3_2000/2k11sd.rom", 0, TYPE_V3_2000, VOODOO_3);
}
static void *v3_3000_init(const device_t *info)
{
        return banshee_init_common("voodoo3_3000/3k12sd.rom", 0, TYPE_V3_3000, VOODOO_3);
}

static int banshee_available()
{
        return rom_present("pci_sg.rom");
}
static int creative_banshee_available()
{
        return rom_present("blasterpci.rom");
}
static int v3_2000_available()
{
        return rom_present("voodoo3_2000/2k11sd.rom");
}
static int v3_3000_available()
{
        return rom_present("voodoo3_3000/3k12sd.rom");
}

static void banshee_close(void *p)
{
        banshee_t *banshee = (banshee_t *)p;

        voodoo_card_close(banshee->voodoo);
        svga_close(&banshee->svga);
        
        free(banshee);
}

static void banshee_speed_changed(void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        
        svga_recalctimings(&banshee->svga);
}

static void banshee_force_redraw(void *p)
{
        banshee_t *banshee = (banshee_t *)p;

        banshee->svga.fullchange = changeframecount;
}

static uint64_t status_time = 0;

static void banshee_add_status_info(char *s, int max_len, void *p)
{
        banshee_t *banshee = (banshee_t *)p;
        voodoo_t *voodoo = banshee->voodoo;
        char temps[512];
        int pixel_count_current[4];
        int pixel_count_total;
        int texel_count_current[4];
        int texel_count_total;
        int render_time[4];
        uint64_t new_time = timer_read();
        uint64_t status_diff = new_time - status_time;
        int c;
        status_time = new_time;

        svga_add_status_info(s, max_len, &banshee->svga);


        for (c = 0; c < 4; c++)
        {
                pixel_count_current[c] = voodoo->pixel_count[c];
                texel_count_current[c] = voodoo->texel_count[c];
                render_time[c] = voodoo->render_time[c];
        }

        pixel_count_total = (pixel_count_current[0] + pixel_count_current[1] + pixel_count_current[2] + pixel_count_current[3]) -
                (voodoo->pixel_count_old[0] + voodoo->pixel_count_old[1] + voodoo->pixel_count_old[2] + voodoo->pixel_count_old[3]);
        texel_count_total = (texel_count_current[0] + texel_count_current[1] + texel_count_current[2] + texel_count_current[3]) -
                (voodoo->texel_count_old[0] + voodoo->texel_count_old[1] + voodoo->texel_count_old[2] + voodoo->texel_count_old[3]);
        _sntprintf(temps, sizeof temps, "%f Mpixels/sec (%f)\n%f Mtexels/sec (%f)\n%f ktris/sec\n%f%% CPU (%f%% real)\n%d frames/sec (%i)\n%f%% CPU (%f%% real)\n"/*%d reads/sec\n%d write/sec\n%d tex/sec\n*/,
                (double)pixel_count_total/1000000.0,
                ((double)pixel_count_total/1000000.0) / ((double)render_time[0] / status_diff),
                (double)texel_count_total/1000000.0,
                ((double)texel_count_total/1000000.0) / ((double)render_time[0] / status_diff),
                (double)voodoo->tri_count/1000.0, ((double)voodoo->time * 100.0) / timer_freq, ((double)voodoo->time * 100.0) / status_diff, voodoo->frame_count, voodoo_recomp,
                ((double)voodoo->render_time[0] * 100.0) / timer_freq, ((double)voodoo->render_time[0] * 100.0) / status_diff);
        if (voodoo->render_threads >= 2)
        {
                char temps2[512];
                _sntprintf(temps2, sizeof temps, "%f%% CPU (%f%% real)\n",
                        ((double)voodoo->render_time[1] * 100.0) / timer_freq, ((double)voodoo->render_time[1] * 100.0) / status_diff);
                strncat(temps, temps2, sizeof(temps)-1);
        }
        if (voodoo->render_threads == 4)
        {
                char temps2[512];
                _sntprintf(temps2, sizeof temps2, "%f%% CPU (%f%% real)\n%f%% CPU (%f%% real)\n",
                        ((double)voodoo->render_time[2] * 100.0) / timer_freq, ((double)voodoo->render_time[2] * 100.0) / status_diff,
                        ((double)voodoo->render_time[3] * 100.0) / timer_freq, ((double)voodoo->render_time[3] * 100.0) / status_diff);
                strncat(temps, temps2, sizeof(temps)-1);
        }

        strncat(s, temps, max_len);

        strncat(s, "Overlay mode: ", max_len); /* leilei debug additions */
        if ((banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK) == VIDPROCCFG_FILTER_MODE_DITHER_2X2)
        strncat(s, "2x2 box filter\n", max_len);
        if ((banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK) == VIDPROCCFG_FILTER_MODE_DITHER_4X4)
        strncat(s, "4x1 tap filter\n", max_len);
        if ((banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK) == VIDPROCCFG_FILTER_MODE_POINT)
        strncat(s, "Nearest neighbor\n", max_len);
        if ((banshee->vidProcCfg & VIDPROCCFG_FILTER_MODE_MASK) == VIDPROCCFG_FILTER_MODE_BILINEAR)
        strncat(s, "Bilinear filtered\n", max_len);
        if ((banshee->vidProcCfg & VIDPROCCFG_H_SCALE_ENABLE))
        strncat(s, "H scaled \n", max_len);
        if ((banshee->vidProcCfg & VIDPROCCFG_V_SCALE_ENABLE))
        strncat(s, "V scaled \n", max_len);
        if ((banshee->vidProcCfg & VIDPROCCFG_2X_MODE))
        strncat(s, "2X mode\n", max_len);

        strncat(s, "\n", max_len);

        for (c = 0; c < 4; c++)
        {
                voodoo->pixel_count_old[c] = pixel_count_current[c];
                voodoo->texel_count_old[c] = texel_count_current[c];
                voodoo->render_time[c] = 0;
        }

        voodoo->tri_count = voodoo->frame_count = 0;
        voodoo->rd_count = voodoo->wr_count = voodoo->tex_count = 0;
        voodoo->time = 0;

        voodoo->read_time = pci_nonburst_time + pci_burst_time;
        
        voodoo_recomp = 0;
}

device_t voodoo_banshee_device =
{
        "Voodoo Banshee PCI (reference)", NULL,
        DEVICE_PCI, 0,
        banshee_init,
        banshee_close,
        NULL,
        banshee_available,
        banshee_speed_changed,
        banshee_force_redraw
};

device_t creative_voodoo_banshee_device =
{
        "Creative Labs 3D Blaster Banshee PCI", NULL,
        DEVICE_PCI, 0,
        creative_banshee_init,
        banshee_close,
        NULL,
        creative_banshee_available,
        banshee_speed_changed,
        banshee_force_redraw
};

device_t voodoo_3_2000_device =
{
        "Voodoo 3 2000 PCI", NULL,
        DEVICE_PCI, 0,
        v3_2000_init,
        banshee_close,
        NULL,
        v3_2000_available,
        banshee_speed_changed,
        banshee_force_redraw
};

device_t voodoo_3_3000_device =
{
        "Voodoo 3 3000 PCI", NULL,
        DEVICE_PCI, 0,
        v3_3000_init,
        banshee_close,
        NULL,
        v3_3000_available,
        banshee_speed_changed,
        banshee_force_redraw
};
