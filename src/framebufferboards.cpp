
/*
* UAE - The Un*x Amiga Emulator
*
* Misc frame buffer boards
*
* - Harlequin
*
*/
#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "debug.h"
#include "memory.h"
#include "custom.h"
#include "picasso96.h"
#include "gfxboard.h"
#include "statusline.h"
#include "rommgr.h"
//#include "framebufferboards.h" // file was empty
#include "xwin.h"

typedef uae_u32(REGPARAM3 *fb_get_func)(struct fb_struct *, uaecptr) REGPARAM;
typedef void (REGPARAM3 *fb_put_func)(struct fb_struct *, uaecptr, uae_u32) REGPARAM;

DECLARE_MEMORY_FUNCTIONS(fb);

static addrbank generic_fb_bank
{
	fb_lget, fb_wget, fb_bget,
	fb_lput, fb_wput, fb_bput,
	default_xlate, default_check, NULL, NULL, _T("FRAMEBUFFER BOARD"),
	fb_lget, fb_wget,
	ABFLAG_IO, S_READ, S_WRITE
};

struct fb_struct
{
	int devnum;
	int monitor_id;
	uae_u32 configured;
	uae_u32 io_start, io_end;
	uae_u8 data[16];

	bool enabled;
	bool modechanged;
	bool visible;
	int width, height;
	bool lace;
	RGBFTYPE rgbtype;
	
	int fb_offset;
	int fb_offset_limit;
	int fb_vram_mask;
	int fb_vram_size;
	bool fb_modified;
	uae_u8 *fb;
	uae_u8 *surface;
	bool isalpha;
	bool ntsc;
	bool genlock;
	int model;
	int irq;

	fb_get_func lget, wget, bget;
	fb_put_func lput, wput, bput;
};

static struct fb_struct *fb_data[MAX_RTG_BOARDS];

static int fb_boards;
static struct fb_struct *fb_last;


static bool fb_get_surface(struct fb_struct *data)
{
	struct amigadisplay *ad = &adisplays[data->monitor_id];
	if (ad->picasso_on) {
		if (data->surface == NULL) {
			data->surface = gfx_lock_picasso(data->monitor_id, false);
		}
	}
	return data->surface != NULL;
}
static void fb_free_surface(struct fb_struct *data)
{
	if (!data->surface)
		return;
	gfx_unlock_picasso(data->monitor_id, true);
	data->surface = NULL;
}

static void fb_irq(struct fb_struct *data)
{
	if (!data->irq)
		return;
	if (data->irq == 2)
		INTREQ_0(0x8000 | 0x0008);
	else if (data->irq == 6)
		INTREQ_0(0x8000 | 0x2000);
}

static void harlequin_offset(struct fb_struct *data)
{
	int offset = data->data[0] & 0x3f;
	data->fb_offset = offset * 65536;
}

static void harlequin_setmode(struct fb_struct *data)
{
	int mode = data->data[1] & 3;
	switch (mode)
	{
		case 0:
		case 3:
		data->width = 910;
		break;
		case 1:
		data->width = 832;
		break;
		case 2:
		data->width = 740;
		break;
	}
	data->lace = (data->data[1] & 8) != 0;
	data->height = data->ntsc ? 486 : 576;
}

static void REGPARAM2 harlequin_wput(struct fb_struct *data, uaecptr addr, uae_u32 w)
{
	addr &= 0x1ffff;
	if (addr == 0) {
		uae_u8 old0 = data->data[0];
		uae_u8 old1 = data->data[1];
		data->data[0] = (uae_u8)(w >> 8);
		data->data[1] = (uae_u8)(w >> 0);
		data->data[0] &= ~0x80;
		data->data[0] |= data->genlock ? 0x00 : 0x80;
		if ((data->data[0] & 0x40) != (old0 & 0x40))
			data->fb_modified = true;
		if (data->data[1] != old1)
			data->fb_modified = true;
		harlequin_offset(data);
		harlequin_setmode(data);
	} else if (addr >= 0x10000) {
		if (data->fb_offset >= data->fb_offset_limit)
			return;
		addr -= 0x10000;
		if (!data->isalpha) {
			if ((addr & 3) == 2)
				w &= 0xff00;
			if ((addr & 3) == 3)
				w &= 0x00ff;
		}
		data->fb[data->fb_offset + addr + 0] = (uae_u8)(w >> 8);
		data->fb[data->fb_offset + addr + 1] = (uae_u8)(w >> 0);
		data->fb_modified = true;
	}
}
static void REGPARAM2 harlequin_bput(struct fb_struct *data, uaecptr addr, uae_u32 b)
{
	addr &= 0x1ffff;
	if (addr == 2) {
		data->data[2] = b;
		data->irq = 0;
	} else if (addr >= 0x20 && addr < 0x30) {
		write_log(_T("RAMDAC WRITE %0x = %02x\n"), addr, (uae_u8)b);
	} else if (addr >= 0x10000) {
		if (data->fb_offset >= data->fb_offset_limit)
			return;
		addr -= 0x10000;
		if (!data->isalpha && (addr & 3) == 3)
			b = 0;
		data->fb[data->fb_offset + addr] = (uae_u8)b;
		data->fb_modified = true;
	}
}
static uae_u32 REGPARAM2 harlequin_wget(struct fb_struct *data, uaecptr addr)
{
	uae_u16 v = 0;
	addr &= 0x1ffff;
	if (addr == 0) {
		v = (data->data[0] << 8) | (data->data[1] << 0);
	} else if (addr >= 0x10000) {
		if (data->fb_offset >= data->fb_offset_limit)
			return 0;
		addr -= 0x10000;
		v = data->fb[data->fb_offset + addr + 0] << 8;
		v |= data->fb[data->fb_offset + addr + 1] << 0;
	}
	return v;
}
static uae_u32 REGPARAM2 harlequin_bget(struct fb_struct *data, uaecptr addr)
{
	uae_u8 v = 0;
	addr &= 0x1ffff;
	if (addr == 0 || addr == 1 || addr == 2) {
		v = data->data[addr];
	} else if (addr >= 0x20 && addr < 0x30) {
		write_log(_T("RAMDAC READ %0x\n"), addr);
	} else if (addr >= 0x10000) {
		if (data->fb_offset >= data->fb_offset_limit)
			return 0;
		addr -= 0x10000;
		v = data->fb[data->fb_offset + addr];
	}
	return 0;
}

static bool harlequin_init(struct autoconfig_info *aci)
{
	uae_u8 model = 100;
	int vram = 0x200000;
	bool isac = true;
	bool ntsc = false;

	aci->label = _T("Harlequin");

	struct boardromconfig *brc = get_device_rom(aci->prefs, ROMTYPE_HARLEQUIN, gfxboard_get_devnum(aci->prefs, aci->devnum), NULL);
	if (brc) {
		model = (brc->roms[0].device_settings & 3) + 100;
		ntsc = (model & 1) != 0;
	}
	aci->autoconfig_bytes[1] = model;

	if (!aci->doinit) {
		return true;
	}
	struct fb_struct *data = xcalloc(struct fb_struct, 1);
	data->devnum = aci->devnum;
	fb_data[data->devnum] = data;
	
	data->bget = harlequin_bget;
	data->wget = harlequin_wget;
	data->bput = harlequin_bput;
	data->wput = harlequin_wput;

	if (brc) {
		switch((brc->roms[0].device_settings >> 2) & 3)
		{
			case 0: // 1.5M
			vram = 0x200000;
			isac = false;
			break;
			case 1: // 2M
			vram = 0x200000;
			isac = true;
			break;
			case 2: // 3M
			vram = 0x400000;
			isac = false;
			break;
			case 3: // 4M
			vram = 0x400000;
			isac = true;
			break;
		}
		data->genlock = ((brc->roms[0].device_settings >> 4) & 1) != 0;
	}

	data->model = model;
	data->ntsc = ntsc;
	data->isalpha = isac;
	data->fb_vram_size = vram;
	data->fb_vram_mask = data->fb_vram_size - 1;
	data->fb_offset_limit = vram;
	data->fb = xcalloc(uae_u8, data->fb_vram_size);

	data->rgbtype = RGBFB_R8G8B8A8;

	harlequin_setmode(data);

	aci->addrbank = &generic_fb_bank;
	aci->userdata = data;

	fb_boards++;
	return true;
}

static void harlequin_free(void *userdata)
{
	struct fb_struct *data = (struct fb_struct*)userdata;

	fb_free_surface(data);

	xfree(data->fb);
	data->fb = NULL;

	fb_data[data->devnum] = NULL;
	xfree(data);
}

static void harlequin_reset(void *userdata)
{
	struct fb_struct *data = (struct fb_struct*)userdata;
	fb_boards = 0;
	fb_last = NULL;
}

static void harlequin_hsync(void *userdata)
{
	struct fb_struct *data = (struct fb_struct*)userdata;
	fb_irq(data);
}

static void harlequin_convert(struct fb_struct *data)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[data->monitor_id];
	bool r = (data->data[1] & 0x80) != 0;
	bool g = (data->data[1] & 0x40) != 0;
	bool b = (data->data[1] & 0x20) != 0;
	int buf = (data->data[0] & 0x40) ? 1 : 0;
	int offset = 0, laceoffset = 0;
	int laceoffsetv = (data->width * data->height / 2) * 4;

	if (buf && data->fb_vram_size > 0x200000)
		offset += data->fb_vram_size / 2;

	int sy = 0;
	int w = vidinfo->width < data->width ? vidinfo->width : data->width;
	int h = vidinfo->height < data->height ? vidinfo->height : data->height;
	for (int y = 0; y < h; y++) {
		uae_u8 *s = data->fb + offset + laceoffset + sy * data->width * 4;
		if (r && g && b) {
			fb_copyrow(data->monitor_id, s, data->surface, 0, 0, data->width, 4, y);
		} else {
			uae_u8 tmp[1000 * 4];
			uae_u8 *d = tmp;
			for (int x = 0; x < w * 4; x += 4) {
				d[x + 0] = r ? s[x + 0] : 0;
				d[x + 1] = g ? s[x + 1] : 0;
				d[x + 2] = b ? s[x + 2] : 0;
				d[x + 3] = 0;
			}
			fb_copyrow(data->monitor_id, d, data->surface, 0, 0, data->width, 4, y);
		}
		if (data->lace) {
			laceoffset = laceoffset ? 0 : laceoffsetv;
			if (!laceoffset)
				sy++;
		} else {
			sy++;
		}
	}
}

static bool harlequin_vsync(void *userdata, struct gfxboard_mode *mode)
{
	struct fb_struct *data = (struct fb_struct*)userdata;
	bool rendered = false;

	if (data->model >= 102) {
		data->data[2] |= 0x80;
		if (data->data[2] & 0x40) {
			data->irq = 2;
			fb_irq(data);
		}
	}

	if (data->visible) {
		
		mode->width = data->width;
		mode->height = data->height;
		mode->mode = data->rgbtype;
		mode->hlinedbl = 1;
		mode->vlinedbl = 1;

		if (fb_get_surface(data)) {
			if (data->fb_modified || data->modechanged || mode->redraw_required) {

				data->fb_modified = false;
				data->modechanged = false;

				harlequin_convert(data);
			}
			fb_free_surface(data);
			rendered = true;
		}
	
	}
	
	return rendered;
}

static bool harlequin_toggle(void *userdata, int mode)
{
	struct fb_struct *data = (struct fb_struct*)userdata;

	if (!data->configured)
		return false;

	if (!mode) {
		if (!data->enabled)
			return false;
		data->enabled = false;
		data->modechanged = false;
		data->visible = false;
		return true;
	} else {
		if (data->enabled)
			return false;
		data->enabled = true;
		data->modechanged = true;
		data->visible = true;
		return true;
	}
	return false;
}

static void harlequin_configured(void *userdata, uae_u32 address)
{
	struct fb_struct *data = (struct fb_struct*)userdata;

	data->configured = address;
	data->io_start = address;
	data->io_end = data->io_start + 0x20000;
}

static void harlequin_refresh(void *userdata)
{
}



static void rainbow2_setmode(struct fb_struct *data)
{
	int mode = data->ntsc;
	if (data->data[0] & 4) {
		mode = mode ? 0 : 1;
	}
	switch (mode)
	{
		case 0:
			data->width = 768;
			data->height = 576;
			break;
		case 1:
			data->width = 768;
			data->height = 476;
			break;
	}
	data->lace = (data->data[0] & 1) != 0;
}

static void REGPARAM2 rainbow2_wput(struct fb_struct *data, uaecptr addr, uae_u32 w)
{
	addr &= 0x1fffff;
	if (addr < 0x1c0000) {
		data->fb[addr + 0] = (uae_u8)(w >> 8);
		data->fb[addr + 1] = (uae_u8)(w >> 0);
		data->fb_modified = true;
	}
}
static void REGPARAM2 rainbow2_bput(struct fb_struct *data, uaecptr addr, uae_u32 b)
{
	addr &= 0x1fffff;
	if (addr == 0x1ffff8) {
		// bit 0: interlace (only makes odd/even fields, does not increase resolution)
		// bit 1: display enable
		// bit 2: toggle PAL/NTSC (yes, toggle, not set!)
		// bit 3: 0=read ROM, 1= read VRAM
		data->data[0] = b;
		rainbow2_setmode(data);
	}
	if (addr < 0x18000 && (data->data[0] & 8)) {
		data->fb[addr] = (uae_u8)b;
		data->fb_modified = true;
	}
}
static uae_u32 REGPARAM2 rainbow2_wget(struct fb_struct *data, uaecptr addr)
{
	uae_u32 v = 0;
	addr &= 0x1fffff;
	if (addr < 0x1c0000 && (data->data[0] & 8)) {
		v = data->fb[addr + 0] << 8;
		v |= data->fb[addr + 1] << 0;
	}
	return v;
}
static uae_u32 REGPARAM2 rainbow2_bget(struct fb_struct *data, uaecptr addr)
{
	uae_u8 v = 0;
	addr &= 0x1fffff;
	if (addr == 0x1ffff8) {
		v = data->data[0];
	}
	if (addr < 0x1c0000 && (data->data[0] & 8)) {
		v = data->fb[addr];
	}
	return 0;
}

static bool rainbow2_init(struct autoconfig_info *aci)
{
	int vram = 0x200000;
	bool ntsc = true;

	aci->label = _T("Rainbow II");

	struct boardromconfig *brc = get_device_rom(aci->prefs, ROMTYPE_RAINBOWII, gfxboard_get_devnum(aci->prefs, aci->devnum), NULL);
	if (brc) {
		ntsc = (brc->roms[0].device_settings & 1) == 0;
	}

	if (!aci->doinit) {
		return true;
	}
	struct fb_struct *data = xcalloc(struct fb_struct, 1);
	data->devnum = aci->devnum;
	fb_data[data->devnum] = data;

	data->bget = rainbow2_bget;
	data->wget = rainbow2_wget;
	data->bput = rainbow2_bput;
	data->wput = rainbow2_wput;

	data->ntsc = ntsc;
	data->fb_vram_size = vram;
	data->fb_vram_mask = data->fb_vram_size - 1;
	data->fb = xcalloc(uae_u8, data->fb_vram_size);

	data->rgbtype = RGBFB_A8R8G8B8;

	rainbow2_setmode(data);

	aci->addrbank = &generic_fb_bank;
	aci->userdata = data;

	fb_boards++;
	return true;
}
static void rainbow2_free(void *userdata)
{
	struct fb_struct *data = (struct fb_struct *)userdata;

	fb_free_surface(data);

	xfree(data->fb);
	data->fb = NULL;

	fb_data[data->devnum] = NULL;
	xfree(data);
}

static void rainbow2_reset(void *userdata)
{
	struct fb_struct *data = (struct fb_struct *)userdata;
	fb_boards = 0;
	fb_last = NULL;
}

static void rainbow2_hsync(void *userdata)
{
	struct fb_struct *data = (struct fb_struct *)userdata;
	fb_irq(data);
}

static void rainbow2_convert(struct fb_struct *data)
{
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[data->monitor_id];

	int sy = 0;
	int w = vidinfo->width < data->width ? vidinfo->width : data->width;
	int h = vidinfo->height < data->height ? vidinfo->height : data->height;
	for (int y = 0; y < h; y++) {
		uae_u8 *s = data->fb + sy * data->width * 4;
		if (!(data->data[0] & 2)) {
			uae_u32 tmp[768];
			memset(tmp, 0, sizeof(tmp));
			fb_copyrow(data->monitor_id, (uae_u8*)tmp, data->surface, 0, 0, data->width, 4, y);
		} else {
			fb_copyrow(data->monitor_id, s, data->surface, 0, 0, data->width, 4, y);
		}
		sy++;
	}
}

static bool rainbow2_vsync(void *userdata, struct gfxboard_mode *mode)
{
	struct fb_struct *data = (struct fb_struct *)userdata;
	bool rendered = false;

	if (data->visible) {

		mode->width = data->width;
		mode->height = data->height;
		mode->mode = data->rgbtype;
		mode->hlinedbl = 1;
		mode->vlinedbl = 1;

		if (fb_get_surface(data)) {
			if (data->fb_modified || data->modechanged || mode->redraw_required) {

				data->fb_modified = false;
				data->modechanged = false;

				rainbow2_convert(data);
			}
			fb_free_surface(data);
			rendered = true;
		}

	}

	return rendered;
}

static bool rainbow2_toggle(void *userdata, int mode)
{
	struct fb_struct *data = (struct fb_struct *)userdata;

	if (!data->configured)
		return false;

	if (!mode) {
		if (!data->enabled)
			return false;
		data->enabled = false;
		data->modechanged = false;
		data->visible = false;
		return true;
	} else {
		if (data->enabled)
			return false;
		data->enabled = true;
		data->modechanged = true;
		data->visible = true;
		return true;
	}
	return false;
}

static void rainbow2_configured(void *userdata, uae_u32 address)
{
	struct fb_struct *data = (struct fb_struct *)userdata;

	data->configured = address;
	data->io_start = address;
	data->io_end = data->io_start + 0x200000;
}

static void rainbow2_refresh(void *userdata)
{
}



static struct fb_struct *getfbboard(uaecptr addr)
{
	if (fb_boards == 1)
		return fb_data[0];
	if (fb_last && addr >= fb_last->io_start && addr <fb_last->io_end)
		return fb_last;
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		struct fb_struct *fb = fb_data[i];
		if (!fb)
			continue;
		if (!fb->configured)
			continue;
		if (addr >= fb->io_start && addr < fb->io_end) {
			fb_last = fb;
			return fb;
		}
	}
	return NULL;
}

static void REGPARAM2 fb_lput(uaecptr addr, uae_u32 w)
{
	struct fb_struct *data = getfbboard(addr);
	if (data) {
		data->wput(data, addr + 0, w >> 16);
		data->wput(data, addr + 2, w & 65535);
	}
}
static void REGPARAM2 fb_wput(uaecptr addr, uae_u32 w)
{
	struct fb_struct *data = getfbboard(addr);
	if (data) {
		data->wput(data, addr, w);
	}
}
static void REGPARAM2 fb_bput(uaecptr addr, uae_u32 w)
{
	struct fb_struct *data = getfbboard(addr);
	if (data) {
		data->bput(data, addr, w);
	}
}

static uae_u32 REGPARAM2 fb_bget(uaecptr addr)
{
	uae_u32 v = 0;
	struct fb_struct *data = getfbboard(addr);
	if (data) {
		v = data->bget(data, addr);
	}
	return v;
}
static uae_u32 REGPARAM2 fb_wget(uaecptr addr)
{
	uae_u32 v = 0;
	struct fb_struct *data = getfbboard(addr);
	if (data) {
		v = data->wget(data, addr);
	}
	return v;
}
static uae_u32 REGPARAM2 fb_lget(uaecptr addr)
{
	uae_u32 v = 0;
	struct fb_struct *data = getfbboard(addr);
	if (data) {
		v = data->wget(data, addr) << 16;
		v |= data->wget(data, addr + 2);
	}
	return v;
}

struct gfxboard_func harlequin_func
{
	harlequin_init,
	harlequin_free,
	harlequin_reset,
	harlequin_hsync,
	harlequin_vsync,
	harlequin_refresh,
	harlequin_toggle,
	harlequin_configured
};

struct gfxboard_func rainbowii_func
{
	rainbow2_init,
	rainbow2_free,
	rainbow2_reset,
	rainbow2_hsync,
	rainbow2_vsync,
	rainbow2_refresh,
	rainbow2_toggle,
	rainbow2_configured
};
