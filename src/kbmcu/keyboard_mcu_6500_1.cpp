/*
* UAE - The Un*x Amiga Emulator
*
* Keyboard MCU emulation
* 6500-1 (early A1000 keyboards. ROM not yet dumped)
* CSG 6570-036 is same MCU as 6500-1 with ROM update that fixes key ghosting.
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
#include "mos6502.h"
#include "inputdevice.h"
#include "gui.h"
#include "savestate.h"

int kb_mcu_log = 0;

static uae_u8 mcu_rom[2048];
static uae_u8 mcu_ram[0x40];
static uae_u8 mcu_io[0x10];
static uae_u8 matrix[128];
static uae_u32 mcu_wd;
static bool mcu_caps_led;
static mos6502 *mcu;
static evt_t lastcycle, kclk_reset_start;
static uae_u8 kbdata, kbdatacnt;
static bool kbhandshake;
static bool rom_loaded;
static int fakemode;
static bool state_loaded;
static int mcu_flags;

// 15*6 matrix
static const uae_u8 matrixkeys[] = {
	0x5f,0x4c,0x4f,0x4e,0x4d,0x4a,
	0x59,0x0d,0x44,0x46,0x41,0x0f,
	0x58,0x0c,0x1b,0x2b,0x40,0x1d,
	0x57,0x0b,0x1a,0x2a,0x3b,0x2d,
	0x56,0x0a,0x19,0x29,0x3a,0x3d,
	0x5c,0x09,0x18,0x28,0x39,0x43,
	0x55,0x08,0x17,0x27,0x38,0x1e,
	0x5b,0x07,0x16,0x26,0x37,0x2e,
	0x54,0x06,0x15,0x25,0x36,0x3e,
	0x53,0x05,0x14,0x24,0x35,0x3c,
	0x52,0x04,0x13,0x23,0x34,0x1f,
	0x51,0x03,0x12,0x22,0x33,0x2f,
	0x50,0x02,0x11,0x21,0x32,0x3f,
	0x5a,0x01,0x10,0x20,0x31,0x5e,
	0x45,0x00,0x42,0x62,0x30,0x5d,
	// not connected in matrix but included in ROM matrix table
	0x49,0x48,0x47,0x2c,0x1c,0x0e
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

static void keymcu_irq(void)
{
	uae_u8 c = mcu_io[15];
	if  ((c & (0x80 | 0x10)) == (0x80 | 0x10) ||
		(c & (0x40 | 0x08)) == (0x40 | 0x08) ||
		(c & (0x20 | 0x04)) == (0x20 | 0x04)) {
		if (kb_mcu_log & 4) {
			write_log("%04x %04x IRQ %02x\n", mcu_wd, mcu->GetPC(), c);
		}
		mcu->IRQ();
	}
}

// This should be in CIA emulation but it is simpler to have it here.
static void get_kbit(bool v)
{
	if (kbhandshake && !fakemode) {
		return;
	}
	kbdata <<= 1;
	kbdata |= v ? 1 : 0;
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

static uae_u8 mcu_io_read(int addr)
{
	uint8_t v = mcu_io[addr];

	if (kb_mcu_log & 2) {
		write_log("%04x %04x IOR %d = %02x\n", mcu_wd, mcu->GetPC(), addr, v);
	}

	switch (addr)
	{
		case 0: // PORTA: KDAT=0,KCLK=1,2-7 = Matrix column read
		{
			if (kbhandshake || fakemode > 1) {
				v &= ~1;
			}

			int row = mcu_io[2] | ((mcu_io[3] & 0x7f) << 8);

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
		}
		break;
		case 1: // PORTB: RSHIFT=0,RALT=1,RAMIGA=2,CTRL=3,LSHIFT=4,LALT=5,LAMIGA=6,CAPSLED=7
		{
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
				v &= ~0x40;
			}
		}
		break;
		case 7:
		mcu_io[15] &= ~0x80;
		break;
	}
	return v;
}

static void mcu_io_write(int addr, uae_u8 v)
{
	if (kb_mcu_log & 2) {
		write_log("%04x %04X IOW %d = %02x\n", mcu_wd, mcu->GetPC(), addr, v);
	}

	switch (addr)
	{
		case 0xf: // CONTROL
		mcu_io[addr] = v;
		break;
		case 0x8: // UPPER LATCH AND TRANSFER LATCH TO COUNTER
		mcu_io[4] = v;
		mcu_io[7] = mcu_io[5];
		mcu_io[6] = mcu_io[4];
		mcu_io[15] &= ~0x80;
		break;
		case 9: // CLEAR PA0 POS EDGE DETECT
		mcu_io[15] &= ~0x40;
		break;
		case 10: // CLEAR PA1 NEG EDGE DETECT
		mcu_io[15] &= ~0x20;
		break;
		case 0x03: // PORTD
		// bit7 = watchdog reset
		if (!(mcu_io[3] & 0x80) && (v & 0x80)) {
			mcu_wd = 0;
			if (kb_mcu_log & 2) {
				write_log("KBMCU WD clear PC=%04x\n", mcu->GetPC());
			}
		}
		mcu_io[addr] = v;
		break;
		case 2:	// PORTC
		mcu_io[addr] = v;
		break;
		case 1: // PORTB
		// bit7 = CAPS LED
		if (mcu_caps_led != ((v & 0x80) != 0)) {
			mcu_caps_led = (v & 0x80) != 0;
			gui_data.capslock = mcu_caps_led;
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
		{
			uae_u8 vv = v;
			if (kbhandshake || fakemode > 1) {
				vv &= ~1;
			}
			// PA0 (KDAT)
			if ((mcu_io[0] & 0x01) != (vv & 0x01)) {
				if (kb_mcu_log & 1) {
					write_log("KDAT=%d KCLK=%d HS=%d CNT=%d PC=%04x\n", (vv & 0x01) != 0, (vv & 0x02) != 0, kbhandshake, kbdatacnt, mcu->GetPC());
				}
				if ((vv & 0x01)) {
					mcu_io[15] |= 0x40;
					keymcu_irq();
				}
			}
			// PA1 (KCLK)
			if ((mcu_io[0] & 0x02) != (vv & 0x02)) {
				if (kb_mcu_log & 1) {
					write_log("KCLK=%d KDAT=%d HS=%d CNT=%d PC=%04x\n", (vv & 0x02) != 0, (vv & 0x01) != 0, kbhandshake, kbdatacnt, mcu->GetPC());
				}
				if (!(vv & 0x02)) {
					mcu_io[15] |= 0x20;
					keymcu_irq();
					if (currprefs.cs_resetwarning) {
						kclk_reset_start = get_cycles() + CYCLE_UNIT * 1500000;
					}
				} else {
					kclk_reset_start = 0;
				}
				if (vv & 0x02) {
					get_kbit((vv & 0x01) != 0);
				}
			}
			mcu_io[addr] = v;
		}
		break;
		default:
		mcu_io[addr] = v;
		break;
	}
}

static void keymcu_res(void)
{
	mcu_io[3] = mcu_io[2] = mcu_io[1] = mcu_io[0] = 0xff;
	mcu_wd = 0;
	mcu_io[15] = 0;
	kclk_reset_start = 0;
	mcu->Reset();
}

static void dec_counter(void)
{
	mcu_io[7]--;
	if (mcu_io[7] == 0xff) {
		mcu_io[6]--;
		if (mcu_io[6] == 0xff) {
			mcu_io[15] |= 0x80;
			mcu_io[7] = mcu_io[5];
			mcu_io[6] = mcu_io[4];
			keymcu_irq();
		}
	}
	mcu_wd++;
	// WD TIMER fault
	if ((mcu_flags & 3) == 3) {
		mcu_wd = 0;
	}
	// Watchdog is about 40ms
	if (mcu_wd >= 60000) {
		if (kb_mcu_log & 1) {
			write_log("KBMCU WD reset\n");
		}
		keymcu_res();
	}
}

static uint8_t MemoryRead(uint16_t address)
{
	uint8_t v = 0;
	if (address >= 0x80 && address < 0x90) {
		v = mcu_io_read(address - 0x80);
	} else if (address >= 0x800) {
		address &= 0x7ff;
		v = mcu_rom[address];
		// ROM fault
		if ((mcu_flags & 3) == 1) {
			if (address == 0) {
				v ^= 0x55;
			}
		}
	} else {
		address &= 0x3f;
		v = mcu_ram[address];
	}
	return v;
}
static void MemoryWrite(uint16_t address, uint8_t value)
{
	if (address >= 0x80 && address < 0x90) {
		mcu_io_write(address - 0x80, value);
	} else if (address < 0x800) {
		address &= 0x3f;
		// RAM fault
		if ((mcu_flags & 3) == 2) {
			if (address == 0x3f) {
				value = 0xff;
			}
		}
		mcu_ram[address] = value;
	}
}

static bool ismcu(void)
{
	return currprefs.keyboard_mode == KB_A500_6570 ||
		currprefs.keyboard_mode == KB_A600_6570 ||
		currprefs.keyboard_mode == KB_A1000_6570 ||
		currprefs.keyboard_mode == KB_Ax000_6570;
}

void keymcu_reset(void)
{
	if (ismcu()) {
		if (!state_loaded) {
			memset(mcu_ram, 0, sizeof mcu_ram);
			memset(mcu_io, 0, sizeof mcu_io);
			memset(matrix, 0, sizeof matrix);
			kbhandshake = false;
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
			int ids[] = { 321, -1 };
			struct romlist *rl = getromlistbyids(ids, NULL);
			if (rl) {
				path = rl->path;
				zf = read_rom(rl->rd);
			}
		}
		if (zf) {
			zfile_fread(mcu_rom, sizeof(mcu_rom), 1, zf);
			zfile_fclose(zf);
			write_log(_T("Loaded 6500-1/6570-036 KB MCU ROM '%s'\n"), path);
			rom_loaded = true;
		} else {
			write_log(_T("Failed to load 6500-1/6570-036 KB MCU ROM\n"));
		}
		if (state_loaded) {
			uint16_t pc = mcu->GetPC();
			uint8_t status = mcu->GetP();
			mcu->Reset();
			mcu->SetPC(pc);
			mcu->SetP(status);
			gui_data.capslock = mcu_caps_led;
		} else {
			keymcu_res();
			if (isrestore()) {
				// if loading statefile without keyboard state: execute MCU code until init phase is done
				fakemode = 1;
				uint64_t cycleCount = 0;
				for (int i = 0; i < 1000000; i++) {
					mcu->Run(1, cycleCount, mos6502::CYCLE_COUNT);
					if (fakemode > 1) {
						fakemode--;
						if (fakemode == 1) {
							mcu_io[15] |= 0x40;
							keymcu_irq();
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

void keymcu_free(void)
{
	if (mcu) {
		delete mcu;
		mcu = NULL;
	}
	state_loaded = false;
}

static void ClockCycle(mos6502 *m)
{
	dec_counter();
	keymcu_irq();
}

void keymcu_init(void)
{
	mcu = new mos6502(MemoryRead, MemoryWrite, ClockCycle);
	mcu->Reset();
	state_loaded = false;
}

bool keymcu_run(bool handshake)
{
	if (ismcu() && rom_loaded) {
		bool hs = kbhandshake;
		kbhandshake = handshake;
		if (!handshake && hs) {
			if (kb_mcu_log & 1) {
				write_log("handshake off\n");
			}
			mcu_io[15] |= 0x40;
			keymcu_irq();
			kbdatacnt = 0;
		} else if (handshake && !hs) {
			if (kb_mcu_log & 1) {
				write_log("handshake on\n");
			}
			kbdatacnt = 0;
		}
		evt_t c = get_cycles();
		int m = CYCLE_UNIT * (currprefs.ntscmode ? 3579545 : 3546895) / 1500000;
		int cycles = (c - lastcycle) / m;
		if (cycles > 0) {
			uint64_t cycleCount = 0;
			if (mcu) {
				mcu->Run(cycles, cycleCount, mos6502::CYCLE_COUNT);
			}
			lastcycle += cycles * m;
		}
		if (kclk_reset_start > 0 && kclk_reset_start < c) {
			write_log("Keyboard MCU generated system reset\n");
			inputdevice_do_kb_reset();
		}
	}
	while (keys_available()) {
		uae_u8 kbcode = get_next_key();
		key_to_matrix(kbcode);
	}
	return kbdatacnt >= 3;
}

#ifdef SAVESTATE

uae_u8 *save_kbmcu(size_t *len, uae_u8 *dstptr)
{
	if (!mcu || !ismcu()) {
		return NULL;
	}

	uae_u8 *dstbak, *dst;
	if (dstptr) {
		dstbak = dst = dstptr;
	} else {
		dstbak = dst = xmalloc(uae_u8, 1000 + sizeof(mcu_ram) + sizeof(mcu_io) + sizeof(matrix));
	}

	save_u32(1);
	save_u32((kbhandshake ? 1 : 0) | (mcu_caps_led ? 2 : 0));
	save_u32(mcu_wd);
	save_u8(kbdata);
	save_u8(kbdatacnt);
	save_u64(lastcycle);
	save_u64(kclk_reset_start);
	for (int i = 0; i < sizeof(mcu_ram); i++) {
		save_u8(mcu_ram[i]);
	}
	for (int i = 0; i < sizeof(mcu_io); i++) {
		save_u8(mcu_io[i]);
	}
	for (int i = 0; i < sizeof(matrix); i++) {
		save_u8(matrix[i]);
	}
	save_u8(mcu->GetA());
	save_u8(mcu->GetX());
	save_u8(mcu->GetY());
	save_u8(mcu->GetP());
	save_u8(mcu->GetS());
	save_u16(mcu->GetPC());

	*len = dst - dstbak;
	return dstbak;
}

uae_u8* restore_kbmcu(uae_u8 *src)
{
	if (!mcu || !ismcu()) {
		return NULL;
	}

	int v = restore_u32();
	if (!(v & 1)) {
		return NULL;
	}
	uae_u32 flags = restore_u32();
	kbhandshake = flags != 0;
	mcu_caps_led = flags & 2;
	mcu_wd = restore_u32();
	kbdata = restore_u8();
	kbdatacnt = restore_u8();
	lastcycle = restore_u64();
	kclk_reset_start = restore_u64();
	for (int i = 0; i < sizeof(mcu_ram); i++) {
		mcu_ram[i] = restore_u8();
	}
	for (int i = 0; i < sizeof(mcu_io); i++) {
		mcu_io[i] = restore_u8();
	}
	for (int i = 0; i < sizeof(matrix); i++) {
		matrix[i] = restore_u8();
	}
	mcu->SetResetA(restore_u8());
	mcu->SetResetX(restore_u8());
	mcu->SetResetY(restore_u8());
	mcu->SetResetP(restore_u8());
	mcu->SetResetS(restore_u8());
	mcu->SetPC(restore_u16());

	state_loaded = true;

	return src;
}

#endif