/*
* UAE - The Un*x Amiga Emulator
*
* Keyboard MCU emulation (A1200 M68HC05)
*
* Copyright 2024 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "rommgr.h"
#include "zfile.h"
#include "events.h"
#include "keyboard_mcu.h"
#include "keybuf.h"
#include "inputdevice.h"
#include "gui.h"
#include "savestate.h"
#include "m6805/m68emu.h"

static struct M68_CTX ctx;

extern int kb_mcu_log;

static uae_u8 mcu_rom[8192];
static uae_u8 mcu_ram[320];
static uae_u8 mcu_io[32];
static uae_u8 mcu_option;
static uae_u8 matrix[128];
static uae_u8 mcu_status_read;
static uae_u8 mcu_dec_ps;
static bool state_loaded;
static bool mcu_caps_led;
static bool rom_loaded;
static uae_u8 mcu_timer_latch_low;
static bool mcu_timer_latched;
static bool mcu_input_capture_latch, mcu_output_compare_latch;
static evt_t lastcycle;
static bool kbhandshake;
static bool mcu_ram0, mcu_ram1;
static int fakemode;
static uae_u8 kbdata, kbdatacnt;
static uae_s32 mcu_wd, mcu_res;
static int mcu_flags;

// 15*6 matrix
static const uae_u8 matrixkeys[] = {
	0x45,0x00,0x42,0x62,0x30,0x5d,
	0x5a,0x01,0x10,0x20,0x31,0x5e,
	0x50,0x02,0x11,0x21,0x32,0x3f,
	0x51,0x03,0x12,0x22,0x33,0x2f,
	0x52,0x04,0x13,0x23,0x34,0x1f,
	0x53,0x05,0x14,0x24,0x35,0x3c,
	0x54,0x06,0x15,0x25,0x36,0x3e,
	0x5b,0x07,0x16,0x26,0x37,0x2e,
	0x55,0x08,0x17,0x27,0x38,0x1e,
	0x5c,0x09,0x18,0x28,0x39,0x43,
	0x56,0x0a,0x19,0x29,0x3a,0x3d,
	0x57,0x0b,0x1a,0x2a,0x3b,0x2d,
	0x58,0x0c,0x1b,0x2b,0x40,0x1d,
	0x59,0x0d,0x44,0x46,0x41,0x0f,
	0x5f,0x4c,0x4f,0x4e,0x4d,0x4a
};

static void key_to_matrix(uae_u8 code)
{
	uae_u8 c = code >> 1;
	if (code & 1) {
		matrix[c] = 0;
		if (kb_mcu_log & 1) {
			write_log("release %02x\n", c);
		}
	} else {
		matrix[c] = 1;
		if (kb_mcu_log & 1) {
			write_log("press %02x\n", c);
		}
	}
}

// This should be in CIA emulation but it is simpler to have it here.
static void get_kbit(bool v)
{
	if (kbhandshake && !fakemode) {
		return;
	}
	kbdata <<= 1;
	kbdata |= (v ? 1 : 0);
	kbdatacnt++;
	if (kbdatacnt >= 8) {
		kbdatacnt = 0;
		if (kb_mcu_log & 1) {
			uae_u8 code = (kbdata >> 1) | (kbdata << 7);
			write_log("got keycode %02x\n", code ^ 0xff);
		}
		if (fakemode) {
			fakemode = 10000;
		} else {
			cia_keyreq(kbdata);
		}
	}
}

static void keymcu_irq(void)
{
	if (mcu_io[0x12] & mcu_io[0x13] & (0x80 | 0x40 | 0x20)) {
		if (kb_mcu_log & 2) {
			write_log("%04x %04x IRQ %02x\n", mcu_wd, ctx.reg_pc, mcu_io[0x12] & mcu_io[0x13]);
		}
		m68_set_interrupt_line(&ctx, M68_INT_TIMER1);
	}
}

static void check_input_capture(uae_u8 v)
{
	if ((v & 0x01) == ((mcu_io[0x12] >> 1) & 1) && !mcu_input_capture_latch) {
		mcu_io[0x14] = mcu_io[0x18];
		mcu_io[0x15] = mcu_io[0x19];
		mcu_io[0x13] |= 1 << 7; // INPUT CAPTURE
		keymcu_irq();
	}
}

static uae_u8 mcu_io_read(int addr)
{
	uint8_t v = mcu_io[addr];

	if (kb_mcu_log & 2) {
		write_log("%04x IOR %04x = %02x\n", ctx.reg_pc, addr, v);
	}

	switch (addr)
	{
		case 0x13: // Read STATUS
		mcu_status_read |= v;
		break;
		case 0x14: // INPUT CAPTURE HIGH
		mcu_input_capture_latch = true;
		break;
		case 0x15: // Reset INPUT CAPTURE LOW
		mcu_input_capture_latch = false;
		if (mcu_status_read & (1 << 7)) {
			mcu_status_read &= ~(1 << 7);
			mcu_io[0x13] &= ~(1 << 7);
		}
		break;
		case 0x17: // Reset OUTPUT COMPARE LOW
		if (mcu_status_read & (1 << 6)) {
			mcu_status_read &= ~(1 << 6);
			mcu_io[0x13] &= ~(1 << 6);
		}
		break;
		case 0x18: // TIMER HIGH
		mcu_timer_latch_low = mcu_io[0x19];
		mcu_timer_latched = true;
		break;
		case 0x19: // Reset TIMER LOW
		if (mcu_timer_latched) {
			mcu_timer_latched = false;
			v = mcu_timer_latch_low;
		}
		if (mcu_status_read & (1 << 5)) {
			mcu_status_read &= ~(1 << 5);
			mcu_io[0x13] &= ~(1 << 5);
		}
		break;
		case 0x1a: // ALT TIMER HIGH
		mcu_timer_latch_low = mcu_io[0x19];
		mcu_timer_latched = true;
		v = mcu_io[0x18];
		break;
		case 0x1b: // ALT TIMER LOW
		if (mcu_timer_latched) {
			mcu_timer_latched = false;
			v = mcu_timer_latch_low;
		} else {
			v = mcu_io[0x19];
		}
		break;

		case 1: // PORTB: KDAT=0,KCLK=1,2-7 = Matrix column read
		{
			v &= ~3;
			if (kbhandshake || fakemode > 1) {
				v |= 2;
			} else {
				v |= 3;
			}
			// Pullups in key lines
			v |= 0x80 | 0x40 | 0x20 | 0x10 | 0x08 | 0x04;

			// row is PORTA + PORTC
			int row = mcu_io[0] | ((mcu_io[2] & 0x3f) << 8) | ((mcu_io[2] & 0x80) << 7);

			if (!currprefs.keyboard_nkro) {
				// matrix without diodes
				for (int i = 0; i < 15; i++) {
					if (!(row & (1 << i))) {
						const uae_u8 *mr = matrixkeys + i * 6;
						for (int j = 0; j < 6; j++) {
							if (matrix[mr[j]]) {
								for(int ii = 0; ii < 15; ii++) {
									if ((row & (1 << ii)) && ii != i) {
										const uae_u8 *mr2 = matrixkeys + ii * 6;
										if (matrix[mr2[j]]) {
											row &= ~(1 << ii);
											i = -1;
										}
									}
								}
							}
						}
					}
				}
			}

			for (int i = 0; i < 15; i++) {
				if (!(row & (1 << i))) {
					const uae_u8 *mr = matrixkeys + i * 6;
					for (int j = 0; j < 6; j++) {
						if (matrix[mr[j]]) {
							v &= ~(4 << j);
						}
					}
				}
			}

			v &= ~mcu_io[addr + 4];
			v |= mcu_io[addr] & mcu_io[addr + 4];
		}
		break;
		case 3: // PORTD: RSHIFT=0,RALT=1,RAMIGA=2,CTRL=3,LSHIFT=4,LALT=5,LAMIGA=7
		{
			v |= ~0x40; // Pullups in all key lines
			if (matrix[0x61]) {
				v &= ~0x01;
			}
			if (matrix[0x65]) {
				v &= ~0x02;
			}
			if (matrix[0x67]) {
				v &= ~0x04;
			}
			if (matrix[0x63]) {
				v &= ~0x08;
			}
			if (matrix[0x60]) {
				v &= ~0x10;
			}
			if (matrix[0x64]) {
				v &= ~0x20;
			}
			if (matrix[0x66]) {
				v &= ~0x80;
			}
		}
		break;
		case 0: // PORTA
		case 2: // PORTC
		v &= ~mcu_io[addr + 4];
		v |= mcu_io[addr] & mcu_io[addr + 4];
		break;
	}

	return v;
}

static void check_porta(uint16_t addr, uae_u8 old, uae_u8 vv)
{
	// PA0 (KDAT)
	if ((old & 0x01) != (vv & 0x01)) {
		if (kb_mcu_log & 1) {
			write_log("KDAT=%d KCLK=%d HS=%d CNT=%d PC=%04x\n", (vv & 0x01) != 0, (vv & 0x02) != 0, kbhandshake, kbdatacnt, ctx.reg_pc);
		}
		check_input_capture(vv);
	}
	// PA1 (KCLK)
	if ((old & 0x02) != (vv & 0x02)) {
		if (kb_mcu_log & 1) {
			write_log("KCLK=%d KDAT=%d HS=%d CNT=%d PC=%04x\n", (vv & 0x02) != 0, (vv & 0x01) != 0, kbhandshake, kbdatacnt, ctx.reg_pc);
		}
		if ((vv & 0x02) && (mcu_io[addr + 4] & 2)) {
			get_kbit((vv & 0x01) != 0);
		}
	}
}

static void mcu_io_write(uint16_t addr, uae_u8 v)
{

	if (kb_mcu_log & 2) {
		write_log("%04x IOW %04x = %02x\n", ctx.reg_pc, addr, v);
	}

	switch (addr)
	{
		case 0x16: // OUTPUT COMPARE HIGH
		mcu_output_compare_latch = true;
		mcu_io[addr] = v;
		break;
		case 0x17: // Reset OUTPUT COMPARE LOW
		mcu_output_compare_latch = false;
		if (mcu_status_read & (1 << 6)) {
			mcu_status_read &= ~(1 << 6);
			mcu_io[0x13] &= ~(1 << 6);
		}
		mcu_io[addr] = v;
		break;
		case 0x12: // TIMER CONTROL
		v &= (0x80 | 0x40 | 0x20 | 0x02 | 0x01);
		mcu_io[addr] = v;
		keymcu_irq();
		break;

		case 3: // PORTD
		mcu_io[addr] = v;
		break;
		case 2:	// PORTC
		// bit7 = CAPS LED
		if (mcu_caps_led != ((v & 0x80) != 0)) {
			mcu_caps_led = (v & 0x80) != 0;
			gui_data.capslock = mcu_caps_led == 0;
			if (!fakemode) {
				gui_led(LED_CAPS, gui_data.capslock, -1);
			}
			if (kb_mcu_log & 1) {
				write_log("KBMCU: CAPS = %d\n", mcu_caps_led);
			}
		}
		mcu_io[addr] = v;
		break;
		case 0: // PORTA
		mcu_io[addr] = v;
		break;
		case 1: // PORTB
		{
			uae_u8 vv = v;
			if (kbhandshake || fakemode > 1) {
				vv &= ~1;
			}
			// if input mode: external pullup -> 1
			if (!(mcu_io[addr + 4] & 1) && !(vv & 1)) {
				vv |= 1;
			}
			if (!(mcu_io[addr + 4] & 2) && !(vv & 2)) {
				vv |= 2;
			}
			check_porta(addr, mcu_io[addr], vv);
			mcu_io[addr] = v;
		}
		break;
		case 5: // PORTB data direction
		{
			uae_u8 dv = mcu_io[addr - 4];
			uae_u8 olddc = mcu_io[addr];
			// if input mode: external pullup -> 1
			if (!(v & 1) && !(dv & 1)) {
				dv |= 1;
			}
			if (!(v & 2) && !(dv & 2)) {
				dv |= 2;
			}
			check_porta(addr, mcu_io[addr - 4], dv);
			mcu_io[addr] = v;
		}
		break;
		// read only registers
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		break;
		default:
		mcu_io[addr] = v;
		break;
	}}

static uint8_t readfunc(struct M68_CTX* ctx, const uint16_t addr)
{
	uint8_t v = 0;
	uint16_t a = addr & 0x1fff;
	if (a < 0x20) {
		v = mcu_io_read(addr);
	} else if (a >= 0x20 && a < 0x30 && mcu_ram0) {
		v = 0xff;
	} else if (a >= 0x50 && a < 0x100) {
		v = mcu_ram[a - 0x30];
		// RAM fault
		if ((mcu_flags & 3) == 2) {
			if (a == 0x80) {
				v ^= 0x55;
			}
		}
	} else if (a >= 0x30 && a < 0x50 && mcu_ram0) {
		v = mcu_ram[a - 0x30];
	} else if (a >= 0x20 && a < 0x30 && !mcu_ram0) {
		v = mcu_rom[a];
	} else if (a >= 0x40 && a < 0x50) {
		v = mcu_rom[a];
	} else if (a >= 0x100 && a < 0x160 && mcu_ram1) {
		v = mcu_ram[a - 0x30];
	} else if (a == 0x1fdf) {
		v = mcu_option;
	} else if (a >= 0x100) {
		v = mcu_rom[a];
		// ROM fault
		if ((mcu_flags & 3) == 1) {
			if (a == 0x100) {
				v ^= 0x55;
			}
		}
	}
	return v;
}

void writefunc(struct M68_CTX *ctx, const uint16_t addr, const uint8_t data)
{
	uint16_t a = addr & 0x1fff;
	if (a < 0x20) {
		mcu_io_write(addr, data);
	} else if (a == 0x1fdf) {
		mcu_option = data;
		mcu_ram0 = (data & 0x80) != 0;
		mcu_ram1 = (data & 0x40) != 0;
	} else if (a == 0x1ff0) {
		if (!(data & 1)) {
			mcu_wd = 0;
		}
	} else if (a >= 0x30 && a < 0x50 && mcu_ram0) {
		mcu_ram[a - 0x30] = data;
	} else if (a >= 0x50 && a < 0x100) {
		mcu_ram[a - 0x30] = data;
	} else if (a >= 0x100 && a < 0x160 && mcu_ram1) {
		mcu_ram[a - 0x30] = data;
	}
}

static void keymcu_res(void)
{
	m68_reset(&ctx);
	ctx.irq = (mcu_flags & 4) == 0;
	mcu_wd = 0;
	mcu_res = 0;
	mcu_input_capture_latch = false;
	mcu_output_compare_latch = false;
	mcu_io[0] = 0;
	mcu_io[1] = 0;
	mcu_io[2] = 0;
	mcu_io[3] = 0;
	mcu_io[4] = 0;
	mcu_io[5] = 0;
	mcu_io[6] = 0;
	mcu_option = (0x02 | 0x08);
	mcu_io[0x0a] = 0;
	mcu_io[0x0b] = 0;
	mcu_io[0x12] = 0;
	mcu_io[0x13] = 0;
	mcu_io[0x18] = 0xff;
	mcu_io[0x19] = 0xfc;
}

static void dec_counter(void)
{
	mcu_dec_ps++;
	if ((mcu_dec_ps & 3) == 0) {
		mcu_wd++;
		// WD TIMER fault
		if ((mcu_flags & 3) == 3) {
			mcu_wd = 0;
		}
		if (mcu_wd >= 32768) {
			bool res = mcu_res < 0;
			keymcu_res();
			if (kb_mcu_log & 1) {
				write_log("KBMCU WD reset\n");
			}
			if (res) {
				mcu_res = 1;
			}
			return;
		}
		if (mcu_res > 0) {
			mcu_res++;
			if (mcu_res >= 128) {
				write_log("Keyboard MCU generated system reset\n");
				inputdevice_do_kb_reset();
				keymcu_res();
				return;
			}
		}
		mcu_io[0x19]++;
		if (mcu_io[0x19] == 0x00) {
			mcu_io[0x18]++;
			if (mcu_io[0x18] == 0x00) {
				mcu_io[0x13] |= 1 << 5; // TIMER OVERFLOW
				keymcu_irq();
			}
		}
		if (mcu_io[0x19] == mcu_io[0x17] && mcu_io[0x18] == mcu_io[0x16] && !mcu_output_compare_latch) {
			mcu_io[0x13] |= 1 << 6; // OUTPUT COMPARE
			keymcu_irq();
			if ((mcu_io[0x12] & 1) == 0) {
				if (mcu_res < 0) {
					// KB_RESET active
					mcu_res = 1;
					if (kb_mcu_log & 1) {
						write_log("KBMCU KB_RESET ACTIVE\n");
					}	
				}
			} else {
				if (mcu_res > 0) {
					if (kb_mcu_log & 1) {
						write_log("KBMCU KB_RESET INACTIVE\n");
					}	
				}
				mcu_res = -1;
			}
		}
	}
}

void keymcu2_reset(void)
{
	if (currprefs.keyboard_mode == KB_A1200_6805) {
		if (!state_loaded) {
			memset(mcu_ram, 0, sizeof mcu_ram);
			memset(mcu_io, 0, sizeof mcu_io);
			memset(matrix, 0, sizeof matrix);
			kbhandshake = false;
			mcu_option = 0;
			mcu_wd = 0;
			mcu_res = 0;
		}
		rom_loaded = false;

		memset(mcu_rom, 0, sizeof mcu_rom);
		struct zfile *zf = NULL;
		const TCHAR *path = NULL;
		struct boardromconfig *brc = get_device_rom_new(&currprefs, ROMTYPE_KBMCU, 0, NULL);
		if (brc) {
			mcu_flags = brc->roms[0].device_settings;
			if (brc->roms[0].romfile[0]) {
				path = brc->roms[0].romfile;
				zf = zfile_fopen(path, _T("rb"));
			}
		}
		if (!zf) {
			int ids[] = { 322, -1 };
			struct romlist *rl = getromlistbyids(ids, NULL);
			if (rl) {
				path = rl->path;
				zf = read_rom(rl->rd);
			}
		}
		if (zf) {
			zfile_fread(mcu_rom, sizeof(mcu_rom), 1, zf);
			zfile_fclose(zf);
			write_log(_T("Loaded 6805 KB MCU ROM '%s'\n"), path);
			rom_loaded = true;
		} else {
			write_log(_T("Failed to load 6805 KB MCU ROM\n"));
		}
		if (state_loaded) {
			gui_data.capslock = mcu_caps_led == 0;
		} else {
			keymcu_res();
			if (isrestore()) {
				// if loading statefile without keyboard state: execute MCU code until init phase is done
				fakemode = 1;
				uint64_t cycleCount = 0;
				for (int i = 0; i < 1000000; i++) {
					uint64_t cyc = m68_exec_cycle(&ctx);
					while (cyc > 0) {
						dec_counter();
						cyc--;
					}
					if (fakemode > 1) {
						fakemode--;
						if (fakemode == 1) {
							check_input_capture(0);
							check_input_capture(1);
						}
					}
				}
				fakemode = 0;
			}
			lastcycle = get_cycles();
		}
	}
	state_loaded = false;
}

void keymcu2_free(void)
{
	state_loaded = false;
}

void keymcu2_init(void)
{
	ctx.read_mem  = &readfunc;
	ctx.write_mem = &writefunc;
	ctx.opdecode  = NULL;
	m68_init(&ctx, M68_CPU_HC05C4);
	keymcu_reset();
	state_loaded = false;
}

bool keymcu2_run(bool handshake)
{
	if (currprefs.keyboard_mode == KB_A1200_6805 && rom_loaded) {
		bool hs = kbhandshake;
		kbhandshake = handshake;
		if (!handshake && hs) {
			if (kb_mcu_log & 1) {
				write_log("handshake off\n");
			}
			check_input_capture(1);
			kbdatacnt = 0;
		} else if (handshake && !hs) {
			if (kb_mcu_log & 1) {
				write_log("handshake on\n");
			}
			check_input_capture(0);
			kbdatacnt = 0;
		}
		evt_t c = get_cycles();
		int m = CYCLE_UNIT * (currprefs.ntscmode ? 3579545 : 3546895) / 1500000;
		int cycles = (c - lastcycle) / m;
		lastcycle += cycles * m;
		while (cycles > 0) {
			uint64_t cyc = m68_exec_cycle(&ctx);
			cycles -= cyc;
			while (cyc > 0) {
				dec_counter();
				cyc--;
			}
		}
	}
	while (keys_available()) {
		uae_u8 kbcode = get_next_key();
		key_to_matrix(kbcode);
	}
	return kbdatacnt >= 3;
}

#ifdef SAVESTATE

uae_u8 *save_kbmcu2(size_t *len, uae_u8 *dstptr)
{
	if (currprefs.keyboard_mode != KB_A1200_6805) {
		return NULL;
	}

	uae_u8 *dstbak, *dst;
	if (dstptr) {
		dstbak = dst = dstptr;
	} else {
		dstbak = dst = xmalloc(uae_u8, 1000 + sizeof(mcu_ram) + sizeof(mcu_io) + sizeof(matrix));
	}

	save_u32(1);
	save_u32((kbhandshake ? 1 : 0) | (ctx.is_stopped ? 2 : 0) | (ctx.is_waiting ? 4 : 0) |
		(mcu_timer_latched ? 8 : 0) | (mcu_input_capture_latch ? 0x10 : 0) | (mcu_output_compare_latch ? 0x20 : 0) |
		(mcu_caps_led ? 0x40 : 0));
	save_u32(mcu_wd);
	save_u8(kbdata);
	save_u8(kbdatacnt);
	save_u64(lastcycle);
	save_u32(mcu_res);
	save_u8(mcu_option);
	save_u8(mcu_status_read);
	save_u8(mcu_dec_ps);
	save_u8(mcu_timer_latch_low);
	for (int i = 0; i < sizeof(mcu_ram); i++) {
		save_u8(mcu_ram[i]);
	}
	for (int i = 0; i < sizeof(mcu_io); i++) {
		save_u8(mcu_io[i]);
	}
	for (int i = 0; i < sizeof(matrix); i++) {
		save_u8(matrix[i]);
	}
	save_u8(ctx.reg_acc);
	save_u8(ctx.reg_x);
	save_u16(ctx.reg_pc);
	save_u16(ctx.pc_next);
	save_u16(ctx.reg_sp);
	save_u8(ctx.reg_ccr);
	save_u8(ctx.pending_interrupts);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8* restore_kbmcu2(uae_u8 *src)
{
	if (currprefs.keyboard_mode != KB_A1200_6805) {
		return NULL;
	}

	int v = restore_u32();
	if (!(v & 1)) {
		return NULL;
	}

	uae_u32 flags = restore_u32();
	kbhandshake = flags != 0;
	ctx.is_stopped = (flags & 2) != 0;
	ctx.is_waiting = (flags & 4) != 0;
	mcu_timer_latched = (flags & 8) != 0;
	mcu_input_capture_latch = (flags & 0x10) != 0;
	mcu_output_compare_latch = (flags & 0x20) != 0;
	mcu_caps_led = (flags & 0x40) != 0;
	mcu_wd = restore_u32();
	kbdata = restore_u8();
	kbdatacnt = restore_u8();
	lastcycle = restore_u64();
	mcu_res = restore_u32();
	mcu_option = restore_u8();
	mcu_status_read = restore_u8();
	mcu_dec_ps = restore_u8();
	mcu_timer_latch_low = restore_u8();
	for (int i = 0; i < sizeof(mcu_ram); i++) {
		mcu_ram[i] = restore_u8();
	}
	for (int i = 0; i < sizeof(mcu_io); i++) {
		mcu_io[i] = restore_u8();
	}
	for (int i = 0; i < sizeof(matrix); i++) {
		matrix[i] = restore_u8();
	}
	ctx.reg_acc = restore_u8();
	ctx.reg_x = restore_u8();
	ctx.reg_pc = restore_u16();
	ctx.pc_next = restore_u16();
	ctx.reg_sp = restore_u16();
	ctx.reg_ccr = restore_u8();
	ctx.pending_interrupts = restore_u8();

	state_loaded = true;

	return src;
}

#endif