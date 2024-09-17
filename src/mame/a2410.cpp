/*
* UAE - The Un*x Amiga Emulator
*
* A2410
*
*/
#include "sysconfig.h"
#include "sysdeps.h"

#include "debug.h"
#include "mameglue.h"
#include "tm34010/tms34010.h"

#include "options.h"
#include "memory.h"
#include "custom.h"
#include "statusline.h"
#include "newcpu.h"
#include "gfxboard.h"
#include "xwin.h"

rectangle tms_rectangle;
static mscreen tms_screen;
static tms340x0_device tms_device;
static address_space tms_space;
mscreen *m_screen;

#define MAX_HEIGHT 2048

struct a2410_struct
{
	int tms_vp, tms_hp;
	int fullrefresh;
	int request_fullrefresh;
	uae_u8 *program_ram;
	int a2410_palette_index;
	uae_u8 a2410_palette[4 * (256 + 4)];
	uae_u32 a2410_palette_32[256 + 4];
	uae_u8 a2410_palette_temp[4];
	uae_u8 a2410_palette_control[4];
	uae_u16 a2410_control;
	int a2410_displaywidth;
	int a2410_displayend;
	int a2410_vertical_start;
	bool a2410_enabled, a2410_active;
	uae_u8 a2410_overlay_mask[2];
	int a2410_overlay_blink_rate_on;
	int a2410_overlay_blink_rate_off;
	int a2410_overlay_blink_cnt;
	uae_u32 tms_configured;
	int a2410_gfxboard;
	int coladdr;

	bool a2410_modechanged;
	int a2410_width, a2410_height;
	bool overlaylinetab[MAX_HEIGHT + 1];
	uae_u32 vramlinetab[MAX_HEIGHT + 1];
	int a2410_vram_start_offset;
	uae_u8 *a2410_surface;
	int a2410_interlace;
	int a2410_interrupt;
	int a2410_hsync_max;
	bool a2410_visible;

	int a2410_activecnt;
	bool a2410_newactive, a2410_preactive;

	addrbank *gfxbank;
};

static struct a2410_struct a2410_data;

extern addrbank tms_bank;

int mscreen::hpos()
{
	if (a2410_data.a2410_displayend) {
		a2410_data.tms_hp++;
		a2410_data.tms_hp %= a2410_data.a2410_displayend;
	} else {
		a2410_data.tms_hp = 0;
	}
	return a2410_data.tms_hp;
}
int mscreen::vpos()
{
	return a2410_data.tms_vp;
}

static void tms_execute_single(void)
{
	tms_device.m_icount = 2;
	tms_device.execute_run();
}

#define A2410_BANK_FRAMEBUFFER 1
#define A2410_BANK_PROGRAM 2
#define A2410_BANK_RAMDAC 3
#define A2410_BANK_CONTROL 4
#define A2410_BANK_TMSIO 5
#define A2410_BANK_DMA 6

static uaecptr makeaddr(UINT32 a, int *bank)
{
	uaecptr addr = 0;
	UINT32 aa = a << 3;
	if ((aa & 0xf0000000) == 0xc0000000) {
		*bank = A2410_BANK_TMSIO;
		addr = (a & 0xff) >> 1;
	} else if ((aa & 0x01900000) == 0x00800000) {
		*bank = A2410_BANK_RAMDAC;
		addr = (a >> 1) & 3;
	} else if ((aa & 0x01900000) == 0x00900000) {
		*bank = A2410_BANK_CONTROL;
	} else if ((aa & 0x30000000) == 0x10000000) {
		addr = a & 0xffffff;
		*bank = A2410_BANK_DMA;
	} else if ((aa & 0x01800000) == 0x01800000) {
		addr = a & 0xfffff;
		*bank = A2410_BANK_PROGRAM;
	} else if ((aa & 0x01800000) == 0x00000000) {
		addr = a & 0xfffff;
		*bank = A2410_BANK_FRAMEBUFFER;
	} else {
		*bank = 0;
		write_log(_T("Unknown BANK %08x PC=%08x\n"), aa, M68K_GETPC);
	}
	return addr;
}

/* CONTROL
 *
 * bit 0 - OSC_SEL - oscillator select
 * bit 1 - _SOG_EN - sync on green
 * bit 2 - SWAP - byte swap enable for DMA
 * bit 3 - SBR - bus request from TMS34010 (Sets bit 7 when bus granted)
 * bit 4 - D_BLANK (read only) - seems to be related to BLANK output of TMS and VSYNC
 * bit 5 - AGBG (read only) - comes straight from Zorro II BCLR signal ?
 * bit 6 - AID = Z3Sense pin when read, HS_INV when write (Horizontal Sync Invert)
 * bit 7 - LGBACK when read, VS_INV when write (Vertical Sync Invert) - LGBACK: A2410 owns the Zorro bus now.
 */

static uae_u8 get_a2410_control(struct a2410_struct *data)
{
	uae_u8 v = (uae_u8)data->a2410_control;
	v &= ~(0x10 | 0x40 | 0x80);
	v |= 0x20 | 0x80;
	if (v & 0x08) // SBR
		v &= ~0x80; // LGBACK (Active low)
	if (currprefs.cs_compatible == CP_A3000 || currprefs.cs_compatible == CP_A3000T ||
		currprefs.cs_compatible == CP_A4000 || currprefs.cs_compatible == CP_A4000T ||
		currprefs.cs_z3autoconfig)
		v |= 0x40; // AID
	return v;
}

UINT32 total_cycles(void)
{
	return (UINT32)(get_cycles() / CYCLE_UNIT);
}

void m_to_shiftreg_cb(address_space space, offs_t offset, UINT16 *shiftreg)
{
	memcpy(shiftreg, &gfxmem_banks[a2410_data.a2410_gfxboard]->baseaddr[TOWORD(offset)], 256 * sizeof(UINT16));
}
void m_from_shiftreg_cb(address_space space, offs_t offset, UINT16 *shiftreg)
{
	memcpy(&gfxmem_banks[a2410_data.a2410_gfxboard]->baseaddr[TOWORD(offset)], shiftreg, 256 * sizeof(UINT16));
}

UINT16 direct_read_data::read_decrypted_word(UINT32 pc)
{
	uae_u16 v = 0;
	int bank;
	uaecptr addr = makeaddr(pc, &bank);
	v = a2410_data.program_ram[addr] << 8;
	v |= a2410_data.program_ram[addr + 1];
	//write_log(_T("TMS instruction word read RAM %08x (%08x) =%04x\n"), pc, addr, v);
	return v;
}
UINT16 direct_read_data::read_raw_word(UINT32 pc)
{
	uae_u16 v = 0;
	int bank;
	uaecptr addr = makeaddr(pc, &bank);
	v = a2410_data.program_ram[addr] << 8;
	v |= a2410_data.program_ram[addr + 1];
	//write_log(_T("TMS instruction word read RAM %08x (%08x) =%04x\n"), pc, addr, v);
	return v;
}

static void mark_overlay(struct a2410_struct *data, int addr)
{
	if (!data->a2410_enabled)
		return;
	addr &= 0x1ffff;
	data->overlaylinetab[addr >> 7] = true;
}

static void a2410_create_palette32(struct a2410_struct *data, int offset)
{
	int idx = data->a2410_palette_index / 4 + offset;
	if (data->a2410_palette[idx * 4 + 0] != data->a2410_palette_temp[0] ||
		data->a2410_palette[idx * 4 + 1] != data->a2410_palette_temp[1] ||
		data->a2410_palette[idx * 4 + 2] != data->a2410_palette_temp[2]) {
		data->request_fullrefresh = 1;
	}
	data->a2410_palette[idx * 4 + 0] = data->a2410_palette_temp[0];
	data->a2410_palette[idx * 4 + 1] = data->a2410_palette_temp[1];
	data->a2410_palette[idx * 4 + 2] = data->a2410_palette_temp[2];
	data->a2410_palette_32[idx] =
		(data->a2410_palette_temp[0] << 16) |
		(data->a2410_palette_temp[1] <<  8) |
		(data->a2410_palette_temp[2] <<  0);
#if 0
	write_log(_T("PAL %d: %02x %02x %02x = %08x\n"),
		idx,
		a2410_palette_temp[0],
		a2410_palette_temp[1],
		a2410_palette_temp[2],
		a2410_palette_32[idx]);
#endif
}


static void write_ramdac(struct a2410_struct *data, int addr, uae_u8 v)
{
	int coloridx = data->a2410_palette_index & 3;
	switch (addr)
	{
		case 0:
		data->a2410_palette_index = v * 4;
		break;
		case 1:
		data->a2410_palette_temp[coloridx] = v;
		data->a2410_palette_index++;
		if ((data->a2410_palette_index & 3) == 3) {
			a2410_create_palette32(data, 0);
			data->a2410_palette_index++;
		}
		if (data->a2410_palette_index >= 256 * 4)
			data->a2410_palette_index = 0;
		break;
		case 2:
		data->a2410_palette_index &= ~3;
		if (data->a2410_palette_index >= 4 * 4 && data->a2410_palette_index < 8 * 4) {
			data->a2410_palette_control[data->a2410_palette_index / 4 - 4] = v;
		}
		data->a2410_overlay_mask[0] = 0xff;
		data->a2410_overlay_mask[1] = 0xff;
		if (!(data->a2410_palette_control[6 - 4] & 1))
			data->a2410_overlay_mask[0] = 0;
		if (!(data->a2410_palette_control[6 - 4] & 2))
			data->a2410_overlay_mask[1] = 0;
		switch((data->a2410_palette_control[6 - 4] >> 4) & 3)
		{
			case 0:
			data->a2410_overlay_blink_rate_on = 16;
			data->a2410_overlay_blink_rate_off = 48;
			break;
			case 1:
			data->a2410_overlay_blink_rate_on = 16;
			data->a2410_overlay_blink_rate_off = 16;
			break;
			case 2:
			data->a2410_overlay_blink_rate_on = 32;
			data->a2410_overlay_blink_rate_off = 32;
			break;
			case 3:
			data->a2410_overlay_blink_rate_on = 64;
			data->a2410_overlay_blink_rate_off = 64;
			break;
		}
		break;
		case 3:
		if (data->a2410_palette_index < 4 * 4) {
			data->a2410_palette_temp[coloridx] = v;
			data->a2410_palette_index++;
			if ((data->a2410_palette_index & 3) == 3) {
				a2410_create_palette32(data, 256);
				data->a2410_palette_index++;
			}
		}
		break;
	}
}

static uae_u8 read_ramdac(struct a2410_struct *data, int addr)
{
	uae_u8 v = 0;
	switch (addr)
	{
		case 0:
		v = data->a2410_palette_index / 4;
		break;
		case 1:
		v = data->a2410_palette[data->a2410_palette_index];
		data->a2410_palette_index++;
		if ((data->a2410_palette_index & 3) == 3)
			data->a2410_palette_index++;
		if (data->a2410_palette_index >= 256 * 4)
			data->a2410_palette_index = 0;
		break;
		case 2:
		if (data->a2410_palette_index >= 4 * 4 && data->a2410_palette_index < 8 * 4) {
			v = data->a2410_palette_control[data->a2410_palette_index / 4 - 4];
		}
		break;
		case 3:
		if (data->a2410_palette_index < 4 * 4) {
			v = data->a2410_palette[data->a2410_palette_index + 256 * 4];
			data->a2410_palette_index++;
			if ((data->a2410_palette_index & 3) == 3)
				data->a2410_palette_index = 0;
			if (data->a2410_palette_index >= 4 * 4)
				data->a2410_palette_index = 0;
		}
		break;
	}
	return v;
}


static bool valid_dma(struct a2410_struct *data, uaecptr addr)
{
	// prevent recursive DMA
	return addr < data->tms_configured || addr >= (data->tms_configured + 65536);
}

UINT8 address_space::read_byte(UINT32 a)
{
	struct a2410_struct *data = &a2410_data;
	int bank;
	uae_u8 v = 0;
	UINT32 aa = a << 3;
	uaecptr addr = makeaddr(a, &bank);

	addr ^= 1;
	switch (bank)
	{
		case A2410_BANK_PROGRAM:
		v = data->program_ram[addr];
		//write_log(_T("TMS byte read RAM %08x (%08x) =%02x PC=%08x\n"), aa, addr, v, M68K_GETPC);
		break;
		case A2410_BANK_FRAMEBUFFER:
		v = data->gfxbank->baseaddr[addr];
		//write_log(_T("TMS byte read framebuffer %08x (%08x) = %02x PC=%08x\n"), aa, addr, v, M68K_GETPC);
		break;
		case A2410_BANK_RAMDAC:
		if (addr & 4) {
			v = read_ramdac(data, addr & 3);
		} else {
			write_ramdac(data, addr & 3, 0xff);
		}
		//write_log(_T("RAMDAC READ %08x = %02x PC=%08x\n"), aa, v, M68K_GETPC);
		break;
		case A2410_BANK_CONTROL:
		v = get_a2410_control(data);
		//write_log(_T("CONTROL READ %08x = %02x PC=%08x\n"), aa, v, M68K_GETPC);
		break;
		case A2410_BANK_DMA:
		if (valid_dma(data, addr)) {
			if (!(data->a2410_control & 4))
				addr ^= 1;
			v = get_byte(addr);
		}
		break;
		default:
		write_log(_T("UNKNOWN READ %08x = %02x PC=%08x\n"), aa, v, M68K_GETPC);
		break;

	}
	return v;
}
UINT16 address_space::read_word(UINT32 a)
{
	struct a2410_struct *data = &a2410_data;
	int bank;
	uae_u16 v = 0;
	UINT32 aa = a << 3;
	uaecptr addr = makeaddr(a, &bank);

	switch (bank)
	{
		case A2410_BANK_TMSIO:
		v = tms_device.io_register_r(*this, addr);
		//write_log(_T("TMS IO word read %08x (%08x) = %04x PC=%08x PC=%08x\n"), aa, addr, v, M68K_GETPC);
		break;
		case A2410_BANK_PROGRAM:
		v = data->program_ram[addr] << 8;
		v |= data->program_ram[addr + 1];
		//write_log(_T("TMS program word read RAM %08x (%08x) = %04x PC=%08x\n"), aa, addr, v, M68K_GETPC);
		break;
		case A2410_BANK_FRAMEBUFFER:
		v = data->gfxbank->baseaddr[addr] << 8;
		v |= data->gfxbank->baseaddr[addr + 1];
		//write_log(_T("TMS gfx word read %08x (%08x) = %04x PC=%08x\n"), aa, addr, v, M68K_GETPC);
		break;
		case A2410_BANK_RAMDAC:
		if (addr & 4) {
			v = read_ramdac(data, addr & 3);
		} else {
			write_ramdac(data, addr & 3, 0xff);
		}
		//write_log(_T("RAMDAC READ %08x = %02x PC=%08x\n"), aa, v, M68K_GETPC);
		break;
		case A2410_BANK_CONTROL:
		v = get_a2410_control(data);
		//write_log(_T("CONTROL READ %08x = %02x PC=%08x\n"), aa, v, M68K_GETPC);
		break;
		case A2410_BANK_DMA:
		if (valid_dma(data, addr)) {
			v = get_word(addr);
			if (!(data->a2410_control & 4))
				v = (v >> 8) | (v << 8);
		}
		break;
		default:
		write_log(_T("UNKNOWN READ %08x = %04x PC=%08x\n"), aa, v, M68K_GETPC);
		break;
	}
	return v;
}

void address_space::write_byte(UINT32 a, UINT8 b)
{
	struct a2410_struct *data = &a2410_data;
	int bank;
	UINT32 aa = a << 3;
	uaecptr addr = makeaddr(a, &bank);
	addr ^= 1;
	switch (bank)
	{
		case A2410_BANK_PROGRAM:
		data->program_ram[addr] = b;
		if (addr < 0x40000)
			mark_overlay(data, addr);
		//write_log(_T("TMS program byte write %08x (%08x) = %02x PC=%08x\n"), aa, addr, b, M68K_GETPC);
		break;
		case A2410_BANK_FRAMEBUFFER:
		data->gfxbank->baseaddr[addr] = b;
		//write_log(_T("TMS gfx byte write %08x (%08x) = %02x PC=%08x\n"), aa, addr, b, M68K_GETPC);
		break;
		case A2410_BANK_RAMDAC:
		//write_log(_T("RAMDAC WRITE %08x = %08x = %02x PC=%08x\n"), addr, aa, b, M68K_GETPC);
		if (!(addr & 4)) {
			write_ramdac(data, addr & 3, b);
		}
		break;
		case A2410_BANK_CONTROL:
		//write_log(_T("CONTROL WRITE %08x = %02x PC=%08x\n"), aa, b, M68K_GETPC);
		data->a2410_control = b;
		break;
		case A2410_BANK_DMA:
		if (valid_dma(data, addr)) {
			if (!(data->a2410_control & 4))
				addr ^= 1;
			put_byte(addr, b);
		}
		break;
		default:
		write_log(_T("UNKNOWN WRITE %08x = %02x PC=%08x\n"), aa, b, M68K_GETPC);
		break;
	}
}

void address_space::write_word(UINT32 a, UINT16 b)
{
	struct a2410_struct *data = &a2410_data;
	int bank;
	UINT32 aa = a << 3;
	uaecptr addr = makeaddr(a, &bank);
	switch (bank)
	{
		case A2410_BANK_TMSIO:
		tms_device.io_register_w(*this, addr, b);
		//write_log(_T("TMS IO word write %08x (%08x) = %04x PC=%08x\n"), aa, addr, b, M68K_GETPC);
		break;
		case A2410_BANK_PROGRAM:
		data->program_ram[addr] = b >> 8;
		data->program_ram[addr + 1] = b & 0xff;
		if (addr < 0x40000) {
			mark_overlay(data, addr);
		}
		//write_log(_T("TMS program word write RAM %08x (%08x) = %04x PC=%08x\n"), aa, addr, b, M68K_GETPC);
		break;
		case A2410_BANK_FRAMEBUFFER:
		data->gfxbank->baseaddr[addr] = b >> 8;
		data->gfxbank->baseaddr[addr + 1] = b & 0xff;
		//write_log(_T("TMS gfx word write %08x (%08x) = %04x PC=%08x\n"), aa, addr, b, M68K_GETPC);
		break;
		case A2410_BANK_RAMDAC:
		//write_log(_T("RAMDAC WRITE %08x = %08x = %04x (%d,%d) PC=%08x\n"), addr, aa, b, data->a2410_palette_index / 4, data->a2410_palette_index & 3, M68K_GETPC);
		if (!(addr & 4)) {
			write_ramdac(data, addr & 3, (uae_u8)b);
		}
		break;
		case A2410_BANK_CONTROL:
		//write_log(_T("CONTROL WRITE %08x = %04x PC=%08x\n"), aa, b, M68K_GETPC);
		data->a2410_control = b;
		break;
		case A2410_BANK_DMA:
		if (valid_dma(data, addr)) {
			if (!(data->a2410_control & 4))
				b = (b >> 8) | (b << 8);
			put_word(addr, b);
		}
		break;
		default:
		write_log(_T("UNKNOWN WRITE %08x = %04x PC=%08x\n"), aa, b, M68K_GETPC);
		break;
	}
}

static uae_u32 REGPARAM2 tms_bget(uaecptr addr)
{
	struct a2410_struct *data = &a2410_data;
	uae_u32 v = 0xff;
	addr &= 65535;
	uae_u16 vv = tms_device.host_r(tms_space, addr >> 1);
	if (!(addr & 1))
		vv >>= 8;
	v = (uae_u8)vv;
	//write_log(_T("tms_bget %08x = %02x PC=%08x\n"), addr, v & 0xff, M68K_GETPC);
	tms_execute_single();
	return v;
}

static uae_u32 REGPARAM2 tms_wget(uaecptr addr)
{
	struct a2410_struct *data = &a2410_data;
	uae_u16 v;
	addr &= 65535;
	v = tms_device.host_r(tms_space, addr >> 1);
	//write_log(_T("tms_wget %08x = %04x PC=%08x\n"), addr, v & 0xffff, M68K_GETPC);
	tms_execute_single();
	return v;
}

static void REGPARAM2 tms_wput(uaecptr addr, uae_u32 w)
{
	struct a2410_struct *data = &a2410_data;
	addr &= 65535;
	tms_device.host_w(tms_space, addr >> 1, w);
	tms_execute_single();
}

static void REGPARAM2 tms_bput(uaecptr addr, uae_u32 b)
{
	struct a2410_struct *data = &a2410_data;
	b &= 0xff;
	addr &= 65535;
	//write_log(_T("tms_bput %08x=%02x PC=%08x\n"), addr, b, M68K_GETPC);
	tms_device.host_w(tms_space, addr >> 1, (b << 8) | b);
	tms_execute_single();
}

static uae_u32 REGPARAM2 tms_lget(uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = tms_wget(addr) << 16;
	v |= tms_wget(addr + 2);
	return v;
}
static void REGPARAM2 tms_lput(uaecptr addr, uae_u32 l)
{
	addr &= 65535;
	tms_wput(addr, l >> 16);
	tms_wput(addr + 2, l);
}

addrbank tms_bank = {
	tms_lget, tms_wget, tms_bget,
	tms_lput, tms_wput, tms_bput,
	default_xlate, default_check, NULL, NULL, _T("A2410"),
	tms_lget, tms_wget,
	ABFLAG_IO, S_READ, S_WRITE
};

static void tms_reset(void *userdata)
{
	struct a2410_struct *data = (struct a2410_struct*)userdata;
	int monid = currprefs.rtgboards[data->a2410_gfxboard].monitor_id;

	if (data->a2410_surface)
		gfx_unlock_picasso(monid, true);
	data->a2410_surface = NULL;

	data->a2410_modechanged = false;
	data->a2410_interlace = 0;
	data->a2410_interrupt = 0;
	data->a2410_hsync_max = 2;
	data->a2410_visible = false;
	data->a2410_enabled = false;
	data->a2410_active = false;
	data->a2410_newactive = false;
	data->a2410_preactive = false;
	data->a2410_control = 0;
	data->tms_vp = data->tms_hp = 0;
	data->a2410_width = 0;
	data->a2410_height = 0;

	tms_screen.width_v = 0;
	tms_screen.height_v = 0;

	if (data->program_ram)
		tms_device.device_reset();

	data->tms_configured = 0;
}

static void tms_configured(void *userdata, uae_u32 address)
{
	struct a2410_struct *data = (struct a2410_struct*)userdata;
	data->tms_configured = address;
}

static void tms_free(void *userdata)
{
	struct a2410_struct *data = (struct a2410_struct*)userdata;
	int monid = currprefs.rtgboards[data->a2410_gfxboard].monitor_id;

	if (data->a2410_surface)
		gfx_unlock_picasso(monid, true);
	data->a2410_surface = NULL;
	if (data->a2410_gfxboard >= 0) {
		gfxboard_free_vram(data->a2410_gfxboard);
	}
	data->a2410_gfxboard = -1;
	xfree(data->program_ram);
	data->program_ram = NULL;
	data->gfxbank = NULL;
}

static bool tms_init(struct autoconfig_info *aci)
{
	struct a2410_struct *data = &a2410_data;

	aci->addrbank = &tms_bank;
	aci->label = _T("A2410");
	if (!aci->doinit) {
		return true;
	}

	data->a2410_gfxboard = aci->prefs->rtgboards[aci->devnum].rtg_index;

	if (data->a2410_gfxboard < 0)
		return false;

	data->gfxbank = gfxmem_banks[data->a2410_gfxboard];

	mapped_free(data->gfxbank);
	xfree(data->program_ram);

	data->gfxbank->label = _T("ram_a8");
	data->gfxbank->reserved_size = 1 * 1024 * 1024;
	gfxboard_get_a8_vram(data->a2410_gfxboard);

	picasso_allocatewritewatch(data->a2410_gfxboard, data->gfxbank->allocated_size);
	data->gfxbank->start = 0xa80000;

	data->program_ram = xcalloc(uae_u8, 1 * 1024 * 1024);

	m_screen = &tms_screen;
	tms_device.device_start();
	tms_reset(data);

	aci->userdata = data; 
	return true;
}

void mscreen::configure(int width, int height, rectangle vis)
{
	struct a2410_struct *data = &a2410_data;

	int ow = data->a2410_width, oh = data->a2410_height;
	data->a2410_width = vis.max_x - vis.min_x + 1;
	data->a2410_height = vis.max_y - vis.min_y + 1;
	data->a2410_interlace = vis.interlace ? 1 : 0;
	if (data->a2410_interlace)
		data->a2410_height *= 2;
	data->a2410_modechanged = true;
	m_screen->height_v = height;
	m_screen->width_v = width;
	tms_rectangle = vis;
	write_log(_T("A2410 %d*%d -> %d*%d\n"), ow, oh, data->a2410_width, data->a2410_height);
	data->a2410_hsync_max = data->a2410_height / 300;
	if (data->a2410_hsync_max <= 0) {
		data->a2410_hsync_max = 1;
	}
}

static void get_a2410_surface(struct a2410_struct *data)
{
	int monid = currprefs.rtgboards[data->a2410_gfxboard].monitor_id;
	struct amigadisplay *ad = &adisplays[monid];

	if (ad->picasso_on) {
		if (data->a2410_surface == NULL) {
			data->a2410_surface = gfx_lock_picasso(monid, false);
		}
	}
}

static bool tms_toggle(void *userdata, int mode)
{
	struct a2410_struct *data = (struct a2410_struct*)userdata;

	if (!data->tms_configured)
		return false;

	if (!mode) {
		data->a2410_enabled = false;
		data->a2410_modechanged = true;
		return true;
	} else {
		data->a2410_enabled = true;
		data->a2410_modechanged = true;
		return true;
	}
	return false;
}

static void tms_vsync_handler2(struct a2410_struct *data, bool internalsync)
{
	int monid = currprefs.rtgboards[data->a2410_gfxboard].monitor_id;
	struct amigadisplay *ad = &adisplays[monid];

	if (!data->tms_configured)
		return;

	tms34010_display_params parms;
	tms_device.get_display_params(&parms);
	bool active = parms.enabled != 0;

	// don't disable display if parms.enabled is inactive less than 1 field
	if (active != data->a2410_newactive) {
		if (data->a2410_activecnt > 0) {
			data->a2410_activecnt = 0;
			data->a2410_newactive = active;
			data->a2410_preactive = active;
		} else if (active) {
			data->a2410_preactive = active;
			data->a2410_newactive = active;
			data->request_fullrefresh = 1;
		} else {
			data->a2410_activecnt = 2;
			data->a2410_preactive = data->a2410_newactive;
			data->a2410_newactive = active;
			active = data->a2410_preactive;
		}
	} else {
		if (data->a2410_activecnt > 0) {
			active = data->a2410_preactive;
			data->a2410_activecnt--;
			if (!data->a2410_activecnt) {
				data->a2410_preactive = data->a2410_newactive;
				active = data->a2410_newactive;
				if (active) {
					data->request_fullrefresh = 1;
				}
			}
		}
	}

	if (active && !data->a2410_visible && !data->a2410_active && !data->a2410_enabled) {
		gfxboard_rtg_enable_initial(monid, data->a2410_gfxboard);
		data->a2410_visible = true;
		tms_toggle(data, 1);
	} else if ((!active || !data->a2410_enabled) && data->a2410_visible) {
		if (gfxboard_switch_away(monid)) {
			tms_toggle(data, 0);
			data->a2410_visible = false;
		}
		if (data->a2410_surface)
			gfx_unlock_picasso(monid, false);
		data->a2410_surface = NULL;
		return;
	} else if (active && data->a2410_enabled && !data->a2410_visible) {
		if (!gfxboard_rtg_enable_initial(monid, data->a2410_gfxboard)) {
			data->a2410_visible = true;
			tms_toggle(data, 1);
		}
	}

	if (data->a2410_visible) {
		if (active != data->a2410_active || data->a2410_modechanged) {
			if (data->a2410_surface)
				gfx_unlock_picasso(monid, false);
			data->a2410_surface = NULL;

			if (active) {
				data->fullrefresh = 2;
			}
			data->a2410_modechanged = false;
			write_log(_T("A2410 MONITOR=%d ACTIVE=%d\n"), monid, data->a2410_enabled);
		}

		if (ad->picasso_on) {
			if (currprefs.leds_on_screen & STATUSLINE_RTG) {
				get_a2410_surface(data);
			}
			if (internalsync) {
				if (data->request_fullrefresh) {
					data->fullrefresh = 2;
					data->request_fullrefresh = 0;
				}
			}
		}
		if (data->a2410_surface) {
			data->a2410_surface = NULL;
			gfx_unlock_picasso(monid, false);
		}
	}
	data->a2410_active = active;

	data->a2410_interlace = -data->a2410_interlace;

	data->a2410_overlay_blink_cnt++;
	if (data->a2410_overlay_blink_cnt == 0 || data->a2410_overlay_blink_cnt == data->a2410_overlay_blink_rate_on) {
		// any blink mode enabled?
		if (data->a2410_palette_control[5 - 4] != 0 || (data->a2410_palette_control[6 - 4] & (4 | 8))) {
			data->fullrefresh = 2;
		}
	}
	if (data->a2410_overlay_blink_cnt > data->a2410_overlay_blink_rate_off + data->a2410_overlay_blink_rate_on) {
		// any blink mode enabled?
		if (data->a2410_palette_control[5 - 4] != 0 || (data->a2410_palette_control[6 - 4] & (4 | 8))) {
			data->fullrefresh = 2;
		}
		data->a2410_overlay_blink_cnt = 0;
	}
}


static void a2410_rethink(struct a2410_struct *data)
{
	if (data->a2410_interrupt)
		INTREQ_0(0x8000 | 0x0008);
}

static bool tms_vsync(void *userdata, struct gfxboard_mode *mode)
{
	struct a2410_struct *data = (struct a2410_struct*)userdata;
	int monid = currprefs.rtgboards[data->a2410_gfxboard].monitor_id;

	bool flushed = false;
	if (!data->a2410_enabled)
		tms_vsync_handler2(data, false);

	data->a2410_surface = NULL;
	gfx_unlock_picasso(monid, true);

	if (data->a2410_visible) {
		mode->width = data->a2410_width;
		mode->height = data->a2410_height;
		mode->hlinedbl = 1;
		mode->vlinedbl = 1;
		mode->mode = RGBFB_CLUT;
	}

	return flushed;
}

static void tms_hsync_handler2(struct a2410_struct *data)
{
	int monid = currprefs.rtgboards[data->a2410_gfxboard].monitor_id;
	struct picasso_vidbuf_description *vidinfo = &picasso_vidinfo[monid];
	struct amigadisplay *ad = &adisplays[monid];

	if (!data->tms_configured)
		return;

	tms_device.m_icount = 1000;
	tms_device.execute_run();
	int a2410_vpos = data->tms_vp;

	tms34010_display_params parms;
	tms_device.get_display_params(&parms);

	data->tms_vp = tms_device.scanline_callback(NULL, data->tms_vp, data->a2410_interlace < 0);

	a2410_rethink(data);

	if (!data->a2410_preactive)
		return;

	if (a2410_vpos == 0) {
		tms_vsync_handler2(data, true);
		if (data->a2410_interlace <= 0) {
			picasso_getwritewatch(data->a2410_gfxboard, data->a2410_vram_start_offset, NULL, NULL);
		}
	}

	if (data->a2410_modechanged || !ad->picasso_on)
		return;

	if (a2410_vpos == 0 && data->fullrefresh > 0) {
		data->fullrefresh--;
	}

	data->a2410_displaywidth = parms.hsblnk - parms.heblnk;
	data->a2410_displayend = parms.heblnk;
	data->a2410_vertical_start = parms.veblnk;

	int yoffset = a2410_vpos - data->a2410_vertical_start;

	int coladdr = parms.coladdr & 0x1ff;
	int vramoffset = ((parms.rowaddr << 8) & 0x7ffff);
	int overlayoffset = (parms.rowaddr << 6) & 0x1ffff;
	int overlayline = parms.yoffset;

	if (yoffset < 0 || yoffset >= MAX_HEIGHT || a2410_vpos < 0 || a2410_vpos >= MAX_HEIGHT)
		return;

	if (data->a2410_interlace) {
		yoffset *= 2;
		if (data->a2410_interlace < 0) {
			yoffset++;
		}
	}

	uae_u16 *vram = (uae_u16 *)data->gfxbank->baseaddr + vramoffset;

	get_a2410_surface(data);
	uae_u8 *dst = data->a2410_surface;
	if (!dst)
		return;

	if (yoffset >= data->a2410_height || yoffset >= vidinfo->maxheight)
		return;
	if (overlayline < 0 || overlayline >= MAX_HEIGHT)
		return;

	bool linerefresh = false;

	if (data->vramlinetab[a2410_vpos] != vramoffset) {
		data->vramlinetab[a2410_vpos] = vramoffset;
		linerefresh = true;
	}

	if (coladdr != data->coladdr) {
		data->fullrefresh = 1;
		data->coladdr = coladdr;
	}

	overlayoffset += (coladdr >> 3) << 1;

	if (!data->fullrefresh && !data->overlaylinetab[overlayoffset >> 7] && !linerefresh) {
		if (!picasso_is_vram_dirty(data->a2410_gfxboard, data->gfxbank->start + (vramoffset << 1), data->a2410_displaywidth)) {
			return;
		}
	}

	if (data->a2410_interlace <= 0) {
		data->overlaylinetab[overlayoffset >> 7] = false;
	}

	dst += yoffset * vidinfo->rowbytes;
	uae_u32 *dst32 = (uae_u32*)dst;

	uae_u8 *overlay0 = data->program_ram + overlayoffset;
	uae_u8 *overlay1 = overlay0 + 0x20000;

	bool overlay0color = !(data->a2410_palette_control[6 - 4] & 0x40);
	uae_u16 bitmap_mask = data->a2410_palette_control[4 - 4];

	if (data->a2410_overlay_blink_cnt >= data->a2410_overlay_blink_rate_on) {
		uae_u8 overlay_mask[2] = { data->a2410_overlay_mask[0], data->a2410_overlay_mask[1] };
		if (data->a2410_palette_control[6 - 4] & 4)
			overlay_mask[0] = 0;
		if (data->a2410_palette_control[6 - 4] & 8)
			overlay_mask[1] = 0;
		bitmap_mask &= ~data->a2410_palette_control[5 - 4];
	}

	int xx = 0;
	int overlay_offset = 0;
	int overlay_bitcount = 0;
	uae_u8 opix0 = 0, opix1 = 0;

	for (int x = parms.heblnk; x < parms.hsblnk && xx < vidinfo->maxwidth; x += 2, xx += 2) {

		if (a2410_vpos >= parms.veblnk && a2410_vpos < parms.vsblnk) {

			if (!overlay_bitcount && overlayoffset >= 0) {
				opix0 = overlay0[overlay_offset ^ 1] & data->a2410_overlay_mask[0];
				opix1 = overlay1[overlay_offset ^ 1] & data->a2410_overlay_mask[1];
				overlay_offset++;
			}

			uae_u16 pix = 0;
			if (coladdr < 0x400) {
				pix = vram[coladdr];
				coladdr++;
			}

			if (overlay0color || opix0 || opix1) {

				int pal;
				uae_u8 ov;

				pal = (pix >> 8) & bitmap_mask;
				ov = opix0 & 1;
				ov |= (opix1 & 1) << 1;
				if (ov || overlay0color)
					pal = 256 + ov;
				*dst32++ = data->a2410_palette_32[pal];
				opix0 >>= 1;
				opix1 >>= 1;

				pal = pix & bitmap_mask;
				ov = opix0 & 1;
				ov |= (opix1 & 1) << 1;
				if (ov || overlay0color)
					pal = 256 + ov;
				*dst32++ = data->a2410_palette_32[pal];
				opix0 >>= 1;
				opix1 >>= 1;


			} else {

				*dst32++ = data->a2410_palette_32[pix >> 8];
				*dst32++ = data->a2410_palette_32[pix & 0xff];

				opix0 >>= 2;
				opix1 >>= 2;

			}

			overlay_bitcount += 2;
			overlay_bitcount &= 7;
		}
	}

	while (xx < vidinfo->maxwidth) {
		*dst32++ = 0;
		xx++;
	}

}

static void tms_hsync(void *userdata)
{
	struct a2410_struct *data = (struct a2410_struct*)userdata;

	for (int i = 0; i < data->a2410_hsync_max; i++)
		tms_hsync_handler2(data);
}

void standard_irq_callback(int level)
{
	struct a2410_struct *data = &a2410_data;

	data->a2410_interrupt = level;
	a2410_rethink(data);
}

static void tms_refresh(void *userdata)
{
	struct a2410_struct *data = (struct a2410_struct *)userdata;
	data->request_fullrefresh = 1;
}

struct gfxboard_func a2410_func
{
	tms_init,
	tms_free,
	tms_reset,
	tms_hsync,
	tms_vsync,
	tms_refresh,
	tms_toggle,
	tms_configured
};
