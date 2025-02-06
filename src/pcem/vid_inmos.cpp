
/* INMOS G300/G364 emulation by Toni Wilen 2024 */

#include <stdlib.h>
#include "ibm.h"
#include "device.h"
#include "io.h"
#include "pci.h"
#include "mem.h"
#include "rom.h"
#include "thread.h"
#include "video.h"
#include "vid_inmos.h"
#include "vid_svga.h"
#include "vid_svga_render.h"
#include "vid_sdac_ramdac.h"

#ifdef DEBUGGER
extern void activate_debugger(void);
#endif

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

    rom_t bios_rom;

    svga_t svga;
    sdac_ramdac_t ramdac;

    int chip;
    uint64_t status_time;

    uint32_t linear_base, linear_size;
    uint32_t mmio_base, mmio_size;
    uint32_t mmio_mask;

    uint32_t vram_mask;

    uint32_t regs[0x400];
    uint32_t hwcursor_col[4];
    int addressalign;
    int syncdelay;

    float (*getclock)(int clock, void *p);
    void *getclock_p;

    int vblank_irq;

} inmos_t;

void inmos_updatemapping(inmos_t*);
void inmos_updatebanking(inmos_t*);

static void inmos_update_irqs(inmos_t *inmos)
{
    if (inmos->vblank_irq) {
        inmos->vblank_irq = 0;
        pci_set_irq(NULL, PCI_INTA, NULL);
        pci_clear_irq(NULL, PCI_INTA, NULL);
    }
}

static void inmos_vblank_start(svga_t *svga)
{
    inmos_t *inmos = (inmos_t*)svga->priv;
    uint32_t control = inmos->regs[0x160];
    if (control & 1) {
        inmos->vblank_irq = 1;
        inmos_update_irqs(inmos);
    }
}

void inmos_hwcursor_draw(svga_t *svga, int displine)
{
    inmos_t *inmos = (inmos_t*)svga->priv;
    int addr = svga->hwcursor_latch.addr;
    int offset = svga->hwcursor_latch.x;
    int line_offset = 0;

    offset <<= svga->horizontal_linedbl;
    if (svga->interlace && svga->hwcursor_oddeven)
        svga->hwcursor_latch.addr += line_offset;

    for (int x = 0; x < 8; x++)
    {
        uint16_t dat = inmos->regs[0x200 + addr + x];
        for (int xx = 0; xx < 8; xx++)
        {
            if (offset >= 0) {
                int cidx = ((dat & 0x0002) ? 2 : 0) | ((dat & 0x0001) ? 1 : 0);
                if (cidx > 0) {
                    ((uint32_t*)buffer32->line[displine])[offset + 32] = inmos->hwcursor_col[cidx];
                }
            }
            offset++;
            dat >>= 2;
        }
    }
    svga->hwcursor_latch.addr += 8;
    if (svga->interlace && !svga->hwcursor_oddeven)
        svga->hwcursor_latch.addr += line_offset;
}

void inmos_recalctimings(svga_t *svga)
{
    inmos_t *inmos = (inmos_t*)svga->priv;

    if (inmos->chip == INMOS_TYPE_G300) {

        uint32_t control = inmos->regs[0x160];
        int vo = (control >> 19) & 3;
        int vramoffset = 1;
        if (vo == 1) {
            vramoffset = 2;
        } else if (vo == 2) {
            vramoffset = 3;
        } else {
            vramoffset = 4;
        }

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

        int row = inmos->regs[0x180] / 0x2000;
        int col = inmos->regs[0x180] % 0x2000;
        int width = svga->rowoffset / (4 * 4);

        svga->ma_latch = col * width + row;

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
        int vo = (control >> 12) & 3;
        bool interleave = ((control >> 18) & 1) != 0;
        int meminit = inmos->regs[0x02d];
        int tdelay = inmos->regs[0x02e];
        int len = meminit + tdelay;

        int vramoffset = 1;
        if (vo == 1) {
            vramoffset = 256;
        } else if (vo == 2) {
            vramoffset = 512;
        } else if (vo == 3) {
            vramoffset = 1024;
        }

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

        uint32_t start = inmos->regs[0x080];
        int row = start / 0x2000;
        int col = start % 0x2000;
        int width = svga->rowoffset / (4 * 4);

        svga->ma_latch = start * 2;
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
    svga->video_res_override = 1;
    svga->hdisp = svga->htotal;

    svga->video_res_x = svga->hdisp;
    svga->video_res_y = svga->dispend;
    svga->video_bpp = svga->bpp;
    svga->fullchange = changeframecount;

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
            break;
    }

    svga->horizontal_linedbl = svga->dispend * 9 / 10 >= svga->hdisp;
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
    inmos_t *inmos = (inmos_t*)p;
}

static void inmos_mmio_write_w(uint32_t addr, uint16_t val, void *p)
{
    inmos_t *inmos = (inmos_t*)p;
}
static void inmos_mmio_write_l(uint32_t addr, uint32_t val, void *p)
{
    inmos_t *inmos = (inmos_t*)p;
    svga_t *svga = &inmos->svga;

    int reg = (addr >> inmos->addressalign) & inmos->mmio_mask;
    inmos->regs[reg] = val;

    //pclog("ADDR %08x REG %08x = %08x\n", addr, reg, val);

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
    inmos_t *inmos = (inmos_t*)p;
    uint8_t val = 0;
    return val;
}
static uint16_t inmos_mmio_readw(uint32_t addr, void *p)
{
    uint16_t v = 0;
    return v;
}

static uint32_t inmos_mmio_readl(uint32_t addr, void *p)
{
    inmos_t *inmos = (inmos_t*)p;

    int reg = (addr >> inmos->addressalign) & inmos->mmio_mask;
    uint32_t v = inmos->regs[reg];
    return v;
}

static uint8_t inmos_read_linear(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t*)p;
    inmos_t *inmos = (inmos_t*)svga->priv;
    uint8_t *fbp = (uint8_t*)(&svga->vram[addr & svga->vram_mask]);
    uint8_t v = *fbp;
    return v;
}
static uint16_t inmos_readw_linear(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t*)p;
    inmos_t *inmos = (inmos_t*)svga->priv;
    uint16_t *fbp = (uint16_t*)(&svga->vram[addr & svga->vram_mask]);
    uint16_t v = *fbp;
    return v;
}
static uint32_t inmos_readl_linear(uint32_t addr, void *p)
{
    svga_t *svga = (svga_t*)p;
    inmos_t *inmos = (inmos_t*)svga->priv;
    uint32_t *fbp = (uint32_t*)(&svga->vram[addr & svga->vram_mask]);
    uint32_t v = *fbp;
    return v;
}

static void inmos_write_linear(uint32_t addr, uint8_t val, void *p)
{
    svga_t *svga = (svga_t*)p;
    inmos_t *inmos = (inmos_t*)svga->priv;
    addr &= svga->vram_mask;
    uint8_t *fbp = (uint8_t*)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}
static void inmos_writew_linear(uint32_t addr, uint16_t val, void *p)
{
    svga_t *svga = (svga_t*)p;
    inmos_t *inmos = (inmos_t*)svga->priv;
    addr &= svga->vram_mask;
    uint16_t *fbp = (uint16_t*)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}
static void inmos_writel_linear(uint32_t addr, uint32_t val, void *p)
{
    svga_t *svga = (svga_t*)p;
    inmos_t *inmos = (inmos_t*)svga->priv;
    addr &= svga->vram_mask;
    uint32_t *fbp = (uint32_t*)(&svga->vram[addr]);
    *fbp = val;
    svga->changedvram[addr >> 12] = changeframecount;
}


void inmos_updatebanking(inmos_t *inmos)
{
    svga_t *svga = &inmos->svga;

}

void inmos_updatemapping(inmos_t *inmos)
{
    svga_t *svga = &inmos->svga;

    inmos_updatebanking(inmos);

    mem_mapping_disablex(&inmos->mmio_mapping);
    mem_mapping_disablex(&inmos->linear_mapping);

    if (inmos->chip == INMOS_TYPE_G300) {
        inmos->linear_base = 0;
        mem_mapping_set_addrx(&inmos->linear_mapping, inmos->linear_base, inmos->linear_size);
        inmos->mmio_base = 0x800000;
        inmos->mmio_size = 0x4000;
        mem_mapping_set_addrx(&inmos->mmio_mapping, inmos->mmio_base, inmos->mmio_size);
    } else if (inmos->chip >= INMOS_TYPE_G360) {
        inmos->linear_base = 0;
        mem_mapping_set_addrx(&inmos->linear_mapping, inmos->linear_base, inmos->linear_size);
        inmos->mmio_base = 0x1000000;
        inmos->mmio_size = 0x4000;
        mem_mapping_set_addrx(&inmos->mmio_mapping, inmos->mmio_base, inmos->mmio_size);
    }

    mem_mapping_enablex(&inmos->mmio_mapping);
    mem_mapping_enablex(&inmos->linear_mapping);
}

static void inmos_adjust_panning(svga_t *svga)
{
    inmos_t *inmos = (inmos_t *)svga->priv;
    int src = 0, dst = 8;

    dst += 24;
    svga->scrollcache_dst = dst;
    svga->scrollcache_src = src + inmos->syncdelay * 4;
}

static inline uint32_t dword_remap(uint32_t in_addr)
{
    return in_addr;
}

static void *inmos_init(int chip)
{
    inmos_t *inmos = (inmos_t*)calloc(sizeof(inmos_t), 1);
    svga_t *svga = &inmos->svga;
    int vram;
    uint32_t vram_size;

    memset(inmos, 0, sizeof(inmos_t));

    vram = device_get_config_int("memory");
    if (vram)
        vram_size = vram << 20;
    else
        vram_size = 512 << 10;
    inmos->vram_mask = vram_size - 1;

    svga_init(NULL, &inmos->svga, inmos, vram_size,
        inmos_recalctimings,
        NULL, NULL,
        inmos_hwcursor_draw,
        NULL);

    inmos->linear_size = 0x800000;
    mem_mapping_addx(&inmos->linear_mapping, 0, 0, inmos_read_linear, inmos_readw_linear, inmos_readl_linear, inmos_write_linear, inmos_writew_linear, inmos_writel_linear, NULL, MEM_MAPPING_EXTERNAL, &inmos->svga);
    mem_mapping_addx(&inmos->mmio_mapping, 0x800000, 0x4000, inmos_mmio_read, inmos_mmio_readw, inmos_mmio_readl, inmos_mmio_write, inmos_mmio_write_w, inmos_mmio_write_l, NULL, MEM_MAPPING_EXTERNAL, inmos);

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
        case 2: default: /*2MB*/
            svga->vram_mask = (2 << 20) - 1;
            svga->vram_max = 4 << 20;
            break;
        case 4: /*4MB*/
            svga->vram_mask = (4 << 20) - 1;
            svga->vram_max = 4 << 20;
            break;
    }

    svga->vsync_callback = inmos_vblank_start;
    svga->adjust_panning = inmos_adjust_panning;

    inmos->chip = chip;
    inmos->addressalign = 2;
    inmos->svga.hwcursor.cur_xsize = 64;
    inmos->svga.hwcursor.cur_ysize = 64;

    inmos_updatemapping(inmos);

    return inmos;
}

void *inmos_rainbow3_z3_init(const device_t *info)
{
    inmos_t *inmos = (inmos_t*)inmos_init(INMOS_TYPE_G360);

    inmos->svga.fb_only = -1;
    inmos->mmio_mask = 0x3ff;
    inmos->getclock = sdac_getclock;
    inmos->getclock_p = &inmos->ramdac;
    sdac_init(&inmos->ramdac);
    svga_set_ramdac_type(&inmos->svga, RAMDAC_8BIT);

    return inmos;
}

void *inmos_visiona_z2_init(const device_t *info)
{
    inmos_t *inmos = (inmos_t*)inmos_init(INMOS_TYPE_G300);

    inmos->svga.fb_only = -1;
    inmos->mmio_mask = 0x1ff;
    inmos->getclock = sdac_getclock;
    inmos->getclock_p = &inmos->ramdac;
    sdac_init(&inmos->ramdac);
    svga_set_ramdac_type(&inmos->svga, RAMDAC_8BIT);

    return inmos;
}

void *inmos_egs_110_24_init(const device_t *info)
{
    inmos_t *inmos = (inmos_t *)inmos_init(INMOS_TYPE_G364);

    inmos->svga.fb_only = -1;
    inmos->mmio_mask = 0x3ff;
    inmos->getclock = sdac_getclock;
    inmos->getclock_p = &inmos->ramdac;
    sdac_init(&inmos->ramdac);
    svga_set_ramdac_type(&inmos->svga, RAMDAC_8BIT);

    return inmos;
}

void inmos_close(void *p)
{
    inmos_t *inmos = (inmos_t*)p;

    svga_close(&inmos->svga);

    free(inmos);
}

void inmos_speed_changed(void *p)
{
    inmos_t *inmos = (inmos_t*)p;

    svga_recalctimings(&inmos->svga);
}

void inmos_force_redraw(void *p)
{
    inmos_t *inmos = (inmos_t*)p;

    inmos->svga.fullchange = changeframecount;
}

void inmos_add_status_info(char *s, int max_len, void *p)
{
    inmos_t *inmos = (inmos_t*)p;
    uint64_t new_time = timer_read();
    uint64_t status_diff = new_time - inmos->status_time;
    inmos->status_time = new_time;

    if (!status_diff)
        status_diff = 1;

    svga_add_status_info(s, max_len, &inmos->svga);
}

device_t inmos_visiona_z2_device =
{
    "Visiona", NULL,
    0, 0,
    inmos_visiona_z2_init,
    inmos_close,
    NULL,
    NULL,
    inmos_speed_changed,
    inmos_force_redraw
};

device_t inmos_rainbow3_z3_device =
{
    "Rainbow III", NULL,
    0, 0,
    inmos_rainbow3_z3_init,
    inmos_close,
    NULL,
    NULL,
    inmos_speed_changed,
    inmos_force_redraw
};

device_t inmos_egs_110_24_device =
{
    "EGS 110/24", NULL,
    0, 0,
    inmos_egs_110_24_init,
    inmos_close,
    NULL,
    NULL,
    inmos_speed_changed,
    inmos_force_redraw
};
