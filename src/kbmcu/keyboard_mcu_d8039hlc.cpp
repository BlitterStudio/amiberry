/*
* UAE - The Un*x Amiga Emulator
*
* Keyboard MCU emulation (D8039HLC, A2000 Cherry)
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
#include "8048/co8048.h"

extern int kb_mcu_log;

static uae_u8 mcu_rom[2048];
static uae_u8 mcu_io[2];
static uae_u8 matrix[128];
static ATCoProc8048 *mcu;
static bool state_loaded;
static bool mcu_caps_led;
static bool rom_loaded;
static bool kbhandshake;
static uae_u8 kbdata, kbdatacnt;
static evt_t lastcycle;
static evt_t kclk_reset_start;
static int fakemode;
static uae_u32 mcu_wd;
static bool mcu_t0;
static int mcu_flags;

// 13x8 matrix
static const uae_u8 matrixkeys[] = {
	0x55,0x59,0x0A,0x06,0x36,0x25,0x19,0x15,
	0x52,0x41,0xFF,0x05,0x35,0x24,0x44,0x14,
	0x51,0xFF,0xFF,0x02,0x32,0x21,0xFF,0x11,
	0x45,0xFF,0x30,0x01,0x31,0x20,0x62,0x10,
	0x54,0xFF,0x40,0x03,0x33,0x22,0x65,0x12,
	0x5D,0x5C,0x3F,0x4A,0x43,0x1F,0x5E,0x2F,
	0x5A,0x5B,0x3E,0x3D,0x1E,0x1D,0x2E,0x2D,
	0x50,0xFF,0x64,0x00,0x60,0x63,0x66,0x42,
	0x5F,0x46,0x4C,0x4E,0x3C,0x0F,0x4F,0x4D,
	0x57,0xFF,0x3A,0x07,0x37,0x26,0x29,0x16,
	0x58,0x0B,0x1A,0x09,0x39,0x28,0x2A,0x18,
	0x56,0x0C,0x1B,0x08,0x38,0x27,0x2B,0x17,
	0x53,0x0D,0x67,0x04,0x34,0x23,0x61,0x13
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
			mcu->AssertIrq();
			fakemode = 10000;
		} else {
			cia_keyreq(kbdata);
		}
	}
}

void mpFnWritePort(int p, uint8 v)
{
	if (kb_mcu_log & 2) {
		write_log("%04x write port %d = %02x\n", mcu->GetPC(), p, v);
	}

	uae_u8 o = mcu_io[p];
	if (p == 1) {

		if (mcu_caps_led != ((v & 0x20) != 0)) {
			mcu_caps_led = (v & 0x20) != 0;
			gui_data.capslock = mcu_caps_led == 0;
			if (!fakemode) {
				gui_led(LED_CAPS, gui_data.capslock, -1);
			}

			if (kb_mcu_log & 1) {
				write_log("KBMCU: CAPS = %d\n", mcu_caps_led);
			}
		}

		if ((o ^ v) & 0x80) {
			if (v & 0x80) {
				get_kbit(((o & 0x40) ? 1 : 0) != 0);
				kclk_reset_start = 0;
			} else {
				if (currprefs.cs_resetwarning) {
					kclk_reset_start = get_cycles() + CYCLE_UNIT * 1800000;
				}
			}

			if (kb_mcu_log & 1) {
				write_log("KCLK = %d\n", (v & 0x80) != 0);
			}
		}
		if ((o ^ v) & 0x40) {
			if (kb_mcu_log & 1) {
				write_log("KDAT = %d\n", (v & 0x40) != 0);
			}
		}
	}

	//write_log("%04x write port %02x %02x\n", mcu->GetPC(), mcu_io[0], mcu_io[1]);

	mcu_io[p] = v;
}
void mpFnWriteBus(uint8 v)
{
	write_log("%04x write bus = %02x\n", mcu->GetPC(), v);
}
uint8 mpFnReadBus(void)
{
	uint8 v = 0;

	uint8 io2 = mcu_io[1];
	int row = mcu_io[0];
	if (io2 & 1) {
		row |= 0x1000;
	}
	if (io2 & 2) {
		row |= 0x0800;
	}
	if (io2 & 4) {
		row |= 0x0400;
	}
	if (io2 & 8) {
		row |= 0x0200;
	}
	if (io2 & 0x10) {
		row |= 0x0100;
	}

	for (int i = 0; i < 13; i++) {
		if ((row & (1 << i))) {
			const uae_u8 *mr = matrixkeys + i * 8;
			for (int j = 0; j < 8; j++) {
				if (mr[j] < 0x80) {
					if (matrix[mr[j]]) {
						v |= 1 << j;
					}
				}
			}
		}
	}

	if (kb_mcu_log & 2) {
		write_log("%04x read bus = %02x\n", mcu->GetPC(), v);
	}

	return v;
}
uint8 mpFnReadPort(int p, uint8 vv)
{
	uint8 v = mcu_io[p];

	if (kb_mcu_log & 2) {
		write_log("%04x read port %d = %02x\n", mcu->GetPC(), p, v);
	}
	return v;
}
uint8 mpFnReadT0(void)
{
	uint8 v = 0;
	if (kb_mcu_log & 2) {
		write_log("%04x read T0 = %02x\n", mcu->GetPC(), v);
	}
	return v;
}
uint8 mpFnReadT1(void)
{
	uint8 v = 0;
	if (kb_mcu_log & 2) {
		write_log("%04x read T1 = %02x\n", mcu->GetPC(), v);
	}
	return v;
}

static void keymcu_res(void)
{
	mcu->ColdReset();
	mcu->NegateIrq();
	kclk_reset_start = 0;
}

void keymcu3_reset(void)
{
	if (currprefs.keyboard_mode == KB_A2000_8039) {
		if (!state_loaded) {
			memset(mcu_io, 0xff, sizeof mcu_io);
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
		if(!zf) {
			int ids[] = { 323, -1 };
			struct romlist *rl = getromlistbyids(ids, NULL);
			if (rl) {
				path = rl->path;
				zf = read_rom(rl->rd);
			}
		}
		if (zf) {
			zfile_fread(mcu_rom, sizeof(mcu_rom), 1, zf);
			zfile_fclose(zf);
			write_log(_T("Loaded 8039 KB MCU ROM '%s'\n"), path);
			rom_loaded = true;
		} else {
			write_log(_T("Failed to load 8039 KB MCU ROM\n"));
		}
		if (state_loaded) {
			gui_data.capslock = mcu_caps_led == 0;
		} else {
			keymcu_res();
			if (isrestore()) {
				// if loading statefile without keyboard state: execute MCU code until init phase is done
				fakemode = 1;
				for (int i = 0; i < 3000000; i++) {
					mcu->AddCycles(1);
					mcu->Run();
					if (fakemode > 1) {
						fakemode--;
						if (fakemode == 1) {
							mcu->NegateIrq();
						}
					}
				}
				fakemode = 0;
			}
		}
		lastcycle = get_cycles();
	}
	state_loaded = false;
}

void keymcu3_free(void)
{
	delete mcu;
	mcu = NULL;
	state_loaded = false;
}

void keymcu3_init(void)
{
	mcu = new ATCoProc8048();
	mcu->ColdReset();
	mcu->SetProgramBanks(mcu_rom, mcu_rom);
	keymcu_reset();
	state_loaded = false;
}

bool keymcu3_run(bool handshake)
{
	if (currprefs.keyboard_mode == KB_A2000_8039 && rom_loaded) {
		bool hs = kbhandshake;
		kbhandshake = handshake;
		if (!handshake && hs) {
			if (kb_mcu_log & 1) {
				write_log("handshake off\n");
			}
			kbdatacnt = 0;
			mcu->NegateIrq();
		} else if (handshake && !hs) {
			if (kb_mcu_log & 1) {
				write_log("handshake on\n");
			}
			kbdatacnt = 0;
			mcu->AssertIrq();
		}
		evt_t c = get_cycles();
		int m = CYCLE_UNIT * (currprefs.ntscmode ? 3579545 : 3546895) / (6000000 / (5 * 3));
		int cycles = (c - lastcycle) / m;
		lastcycle += cycles * m;
		mcu->AddCycles(cycles);
		mcu->Run();
		if (kclk_reset_start > 0 && kclk_reset_start < c) {
			write_log("Keyboard MCU system reset\n");
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

uae_u8 *save_kbmcu3(size_t *len, uae_u8 *dstptr)
{
	if (currprefs.keyboard_mode != KB_A2000_8039) {
		return NULL;
	}

	uae_u8 *dstbak, *dst;
	if (dstptr) {
		dstbak = dst = dstptr;
	} else {
		dstbak = dst = xmalloc(uae_u8, 1000 + 128 + sizeof(mcu_io) + sizeof(matrix));
	}

	save_u32(1);
	save_u32((kbhandshake ? 1 : 0) | (mcu->mbTimerActive ? 2 : 0) | (mcu->mbIrqAttention ? 4 : 0) |
		(mcu->mbIrqEnabled ? 8 : 0) | (mcu->mbF1 ? 16 : 0) | (mcu->mbTF ? 32 : 0) |
		(mcu->mbIF ? 64 : 0) | (mcu->mbPBK ? 128 : 0) | (mcu->mbDBF ? 256 : 0) |
		(mcu->mbIrqPending ? 512 : 0) | (mcu->mbTimerIrqPending ? 1024 : 0));
	save_u32(mcu_wd);
	save_u8(kbdata);
	save_u8(kbdatacnt);
	save_u64(lastcycle);
	save_u64(kclk_reset_start);
	for (int i = 0; i < 128; i++) {
		save_u8(mcu->mRAM[i]);
	}
	for (int i = 0; i < sizeof(mcu_io); i++) {
		save_u8(mcu_io[i]);
	}
	for (int i = 0; i < sizeof(matrix); i++) {
		save_u8(matrix[i]);
	}
	save_u8(mcu->mA);
	save_u8(mcu->mT);
	save_u8(mcu->mPSW);
	save_u16(mcu->mPC);
	save_u8(mcu->mP1);
	save_u8(mcu->mP2);
	save_u8(mcu->mTStatesLeft);

	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_kbmcu3(uae_u8* src)
{
	if (currprefs.keyboard_mode != KB_A2000_8039) {
		return NULL;
	}

	uae_u32 v = restore_u32();
	if (!(v & 1)) {
		return src;
	}
	v = restore_u32();
	kbhandshake = v & 1;
	mcu->mbTimerActive = (v & 2) != 0;
	mcu->mbIrqAttention = (v & 4) != 0;
	mcu->mbIrqEnabled = (v & 8) != 0;
	mcu->mbF1 = (v & 16) != 0;
	mcu->mbTF = (v & 32) != 0;
	mcu->mbIF = (v & 64) != 0;
	mcu->mbPBK = (v & 128) != 0;
	mcu->mbDBF = (v & 256) != 0;
	mcu->mbIrqPending = (v & 512) != 0;
	mcu->mbTimerIrqPending = (v & 1024) != 0;
	mcu_wd = restore_u32();
	kbdata = restore_u8();
	kbdatacnt = restore_u8();
	lastcycle = restore_u64();
	kclk_reset_start = restore_u64();
	for (int i = 0; i < 128; i++) {
		mcu->mRAM[i] = restore_u8();
	}
	for (int i = 0; i < sizeof(mcu_io); i++) {
		mcu_io[i] = restore_u8();
	}
	for (int i = 0; i < sizeof(matrix); i++) {
		matrix[i] = restore_u8();
	}
	mcu->mA = restore_u8();
	mcu->mT = restore_u8();
	mcu->mPSW = restore_u8();
	mcu->mPC = restore_u16();
	mcu->mP1 = restore_u8();
	mcu->mP2 = restore_u8();
	mcu->mTStatesLeft = restore_u8();

	state_loaded = true;
	return src;
}

#endif