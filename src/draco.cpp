
/* MacroSystem DraCo
 *
 * Toni Wilen 2023-2024
 */

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef WITH_DRACO
#include "options.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "devices.h"
#include "custom.h"
#include "debug.h"
#include "x86.h"
#include "ncr_scsi.h"
#include "draco.h"
#include "zfile.h"
#include "keybuf.h"
#include "rommgr.h"
#include "disk.h"

static int maxcnt = 100;

#define ONEWIRE_DEBUG 1
#define KBD_DEBUG 1

/*
	.asciz	"kbd/soft"	| 1: native keyboard, soft ints
	.asciz	"cia/zbus"	| 2: cia, PORTS
	.asciz	"lclbus"	| 3: local bus, e.g. Altais vbl
	.asciz	"drscsi"	| 4: mainboard scsi
	.asciz	"superio"	| 5: superio chip
	.asciz	"lcl/zbus"	| 6: lcl/zorro lev6
	.asciz	"buserr"	| 7: nmi: bus timeout
*/

// CASABLANCA
// INTENA  : 0x02000043
// INTPEN  : 0x02000083
// INTFRC  : 0x020000c3
// ?       : 0x02000143
// SuperIO : 0x02400000 (PC-style serial, parallel, WD floppy)
// Interrupt

// Bit 0: Master enable / Soft INT1
// Bit 1: INT4
// Bit 2: INT2
// Bit 3: INT6
// Bit 4: INT1
// Bit 5:
// Bit 6: INT5 (SuperIO)

// DRACO
// INTENA  : 0x01000001
// INTPEN  : 0x01400001
// INTFRC  : 0x01800001
// ?       : 0x01c00001
// IO      : 0x02000000 (Misc control, 1-wire RTC)
// SuperIO : 0x02400000 (PC-style serial, parallel, WD floppy) PC IO = Amiga address >> 2
// CIA     : 0x02800000 (only in pre-4 revisions)
// KS ROM  : 0x02c00000 (A3000 KS 3.1)
// Z2      : 0x03e80000
// SCSI    : 0x04000000 (53C710)
// SVGA    : 0x20000000 (NCR 77C32BLT)

// Interrupt

// Bit 0: Master enable / Soft INT1
// Bit 1: INT4 (SCSI)
// Bit 2: INT2 (Timer)
// Bit 3: INT6
// SVGA vblank: INT3
// SuperIO: INT5

// IO:

// IO_control   : 01
#define DRCNTRL_FDCINTENA 1
#define DRCNTRL_KBDDATOUT 2
#define DRCNTRL_KBDCLKOUT 4
#define DRCNTRL_WDOGDIS 8
#define DRCNTRL_WDOGDAT 16
#define DRCNTRL_KBDINTENA 32
#define DRCNTRL_KBDKBDACK 64
#define DRCNTRL_SCSITERM 128

// IO_status    : 03
#define DRSTAT_CLKDAT 1 // (1-wire data)
#define DRSTAT_KBDDATIN 2
#define DRSTAT_KBDCLKIN 4
#define DRSTAT_KBDRECV 8
#define DRSTAT_CLKBUSY 16 // (1-wire, busy)
#define DRSTAT_BUSTIMO 32
#define DRSTAT_SCSILED 64

// IO_KBD_data  : 05

// IO_status2   : 07
#define DRSTAT2_KBDBUSY 1
#define DRSTAT2_PARIRQPEN 4
#define DRSTAT2_PARIRQENA 8
#define DRSTAT2_TMRINTENA 16
#define DRSTAT2_TMRIRQPEN 32

// Revision     : 09 (write=timer reset)
// IO_TimerHI   : 0B
// IO_TimerLO   : 0D
// unused       : 0F
// IO_clockw0   : 11 (1-wire, write 1)
// IO_clockw1   : 13 (1-wire, write 0)
// IO_clockrst  : 15 (1-wire, reset)
// IO_KBD_Reset : 17
// IO_bustimeout: 19 
// IO_scsiledres: 1b
// IO_fdcread   : 1d
// IO_parrst    : 1f


/*

	53C710 (SCSI)
	Base: 0x04000000
	IRQ: 4
	INTREQ: 7 (AUD0)

	DracoMotion
	Base: 0x20000000
	IRQ: 3
	INTREQ: 4 (COPER)
	Size: 128k
	Autoconfig: 83 17 30 00 47 54 00 00 00 00 00 00 00 00 00 00 (18260/23)

	Mouse
	Base: 0x02400BE3
	IRQ: 4
	INTREQ: 9 (AUD2)

	Serial
	Base: 0x02400FE3
	IRQ: 4
	INTREQ: 10 (AUD3)

	Floppy:
	Base: 0x02400003
	IRQ: 5
	INTREQ: 11 (RBF)
*/

void serial_reset();
void serial_write(uint16_t addr, uint8_t val, void *p);
uint8_t serial_read(uint16_t addr, void *p);
void draco_serial_init(void **s1, void **s2);
void mouse_serial_poll(int x, int y, int z, int b, void *p);
void *mouse_serial_init_draco();

void keyboard_at_write(uint8_t val, void *priv);
uint8_t keyboard_at_read(uint16_t port, void *priv);
void *draco_keyboard_init(void);
void draco_key_process(uint16_t scan, int down);
int draco_kbc_translate(uint8_t val);

static void *draco_serial[2];
static void *draco_mouse_base, *draco_keyboard;
static uae_u8 *dracorom;
static int dracoromsize;
static uae_u8 draco_revision, draco_cias;

static uae_u8 draco_intena, draco_intpen, draco_intfrc, draco_svga_irq_state;
static uae_u16 draco_timer, draco_timer_latch;
static bool draco_timer_latched;
static bool draco_fdc_intpen;
static uae_u8 draco_superio_cfg[16];
static uae_s8 draco_superio_idx;
static uae_u8 draco_reg[0x20];
static int draco_watchdog;
static int draco_scsi_intpen, draco_serial_intpen;

static bool draco_have_vmotion = false;

static void casa_irq(void)
{
	uae_u16 irq = 0;
	if (draco_scsi_intpen) {
		draco_intpen |= 2;
	} else {
		draco_intpen &= ~2;
	}
	if (draco_intena & 1) {
		uae_u16 mask = draco_intena & draco_intpen;
		if (mask) {
			if (mask & 1) { // INT1
				irq |= 1;
			}
			if (mask & 2) { // INT4
				irq |= 0x80;
			}
			if (mask & 4) { // INT2
				irq |= 8;
			}
			if (mask & 8) { // INT6
				irq |= 0x2000;
			}
		}
		if (draco_intfrc & 1) {
			irq |= 1; // software interrupt
		}
		if (draco_svga_irq_state) {
			irq |= 0x0010; // INT3
		}
	}
	INTREQ_f(0x7fff);
	if (irq) {
		INTREQ_f(0x8000 | irq);
		doint();
	}
}

static void draco_irq(void)
{
	uae_u16 irq = 0;
	if (draco_scsi_intpen) {
		draco_intpen |= 2;
	} else {
		draco_intpen &= ~2;
	}
	if (draco_intena & 1) {
		uae_u16 mask = draco_intena & draco_intpen;
		if (mask) {
			if (mask & 1) { // INT1
				irq |= 1;
			}
			if (mask & 2) { // INT4
				irq |= 0x80;
			}
			if (mask & 4) { // INT2
				irq |= 8;
			}
			if (mask & 8) { // INT6
				irq |= 0x2000;
			}
		}
		if (draco_intfrc & 1) {
			irq |= 1; // software interrupt
		}
		if (draco_svga_irq_state) {
			irq |= 0x0010; // INT3
		}
		if ((draco_fdc_intpen || draco_serial_intpen) && (draco_reg[1] & DRCNTRL_FDCINTENA)) {
			irq = 0x1000; // INT5
		}
		if ((draco_reg[7] & DRSTAT2_TMRINTENA) && (draco_reg[7] & DRSTAT2_TMRIRQPEN)) {
			irq |= 1;
		}
		if (draco_reg[3] & DRSTAT_KBDRECV) {
			irq |= 1;
		}
	}
	INTREQ_f(0x7fff);
	if (irq) {
		INTREQ_f(0x8000 | irq);
		doint();
	}
}

static void dc_irq(void)
{
	if (currprefs.cs_compatible == CP_DRACO) {
		draco_irq();
	}
	if (currprefs.cs_compatible == CP_CASABLANCA) {
		casa_irq();
	}
}

void draco_svga_irq(bool state)
{
	draco_svga_irq_state = state;
	dc_irq();
}

static uae_u8 draco_kbd_state;
static uae_s8 draco_kbd_state2, draco_kbd_poll;
static uae_u16 draco_kbd_code;
static uae_u8 draco_kbd_buffer[10];
static uae_u8 draco_kbd_buffer_len;
static uae_s8 draco_kdb_params;
static uae_u16 draco_kbd_in_buffer[16];
static int draco_kbd_in_buffer_len;

#define DRACO_KBD_POLL_VAL 48

static void draco_keyboard_reset(void)
{
	draco_kbd_buffer_len = 0;
	draco_kbd_in_buffer_len = 0;
	draco_kbd_state = 0;
	draco_kbd_poll = 0;
	draco_kbd_code = 0;
	draco_kdb_params = 0;
	draco_kbd_state2 = -1;
}

static void draco_keyboard_read(void)
{
	uae_u8 v = draco_reg[3];

	if (draco_kbd_poll > 0) {
		do_cycles(CYCLE_UNIT * 8);
		draco_kbd_poll--;
		if (draco_kbd_poll == 0) {
			draco_reg[3] &= ~DRSTAT_KBDCLKIN;
			v &= ~DRSTAT_KBDCLKIN;
			draco_kbd_poll = -DRACO_KBD_POLL_VAL;
		}
		draco_reg[3] = v;
		return;
	}

	if (draco_kbd_poll < 0) {
		do_cycles(CYCLE_UNIT * 8);
		draco_kbd_poll++;
		if (draco_kbd_poll == 0) {
			draco_kbd_poll = DRACO_KBD_POLL_VAL;
			v |= DRSTAT_KBDCLKIN;
			uae_u16 bit = (draco_reg[1] & DRCNTRL_KBDDATOUT) ? 0x8000 : 0;
#if KBD_DEBUG > 1
			write_log("draco keyboard got bit %d: %d\n", draco_kbd_state2, bit ? 1 : 0);
#endif
			draco_kbd_code >>= 1;
			draco_kbd_code |= bit;
			draco_kbd_state2++;

			if (draco_kbd_state2 == 11) {
				draco_kbd_code >>= 5;
				draco_kbd_code &= 0xff;

				if (currprefs.cpuboard_settings & 0x10) {
#if KBD_DEBUG
					write_log("draco->keyboard code %02x\n", draco_kbd_code);
#endif
					draco_reg[3] = v;
					keyboard_at_write((uae_u8)draco_kbd_code, draco_keyboard);
					v = draco_reg[3];
				}
			}
		}
	}

	draco_reg[3] = v;
}


static void draco_keyboard_write(uae_u8 v)
{
	v &= DRCNTRL_KBDDATOUT | DRCNTRL_KBDCLKOUT;
	if (v == draco_kbd_state) {
		return;
	}
	if ((v & DRCNTRL_KBDDATOUT) != (draco_kbd_state & DRCNTRL_KBDDATOUT)) {
#if KBD_DEBUG > 1
		write_log("DRCNTRL_KBDDATOUT %d -> %d\n", (draco_kbd_state & DRCNTRL_KBDDATOUT) ? 1 : 0, (v & DRCNTRL_KBDDATOUT) ? 1 : 0);
#endif
	}
	if ((v & DRCNTRL_KBDCLKOUT) != (draco_kbd_state & DRCNTRL_KBDCLKOUT)) {
#if KBD_DEBUG > 1
		write_log("DRCNTRL_KBDCLKOUT %d -> %d\n", (draco_kbd_state & DRCNTRL_KBDCLKOUT) ? 1 : 0, (v & DRCNTRL_KBDCLKOUT) ? 1 : 0);
#endif
	}

	// start receive
	if (draco_kbd_state2 < 0 && (v & DRCNTRL_KBDCLKOUT) && !(draco_kbd_state & DRCNTRL_KBDCLKOUT)) {
		draco_reg[3] |= DRSTAT_KBDCLKIN;
		draco_kbd_code = 0;
		draco_kbd_state2 = 0;
		draco_kbd_poll = DRACO_KBD_POLL_VAL;
	}

	draco_kbd_state = v;
}

static void draco_keyboard_send(void)
{
	if (draco_kbd_buffer_len > 0 && !(draco_reg[3] & DRSTAT_KBDRECV)) {
		uae_u8 code = draco_kbd_buffer[0];
		for (int i = 1; i < draco_kbd_buffer_len; i++) {
			draco_kbd_buffer[i - i] = draco_kbd_buffer[i];
		}
		draco_kbd_buffer_len--;
		draco_reg[5] = code;
		draco_reg[3] |= DRSTAT_KBDRECV;
#if KBD_DEBUG
		write_log("keyboard->draco code %02x\n", code);
#endif
		draco_irq();
	}
}

static void draco_keyboard_done(void)
{
#if KBD_DEBUG > 1
	write_log("draco keyboard interface reset\n");
#endif
	draco_reg[3] &= ~DRSTAT_KBDCLKIN;
	draco_reg[3] &= ~DRSTAT_KBDRECV;
	draco_reg[1] &= ~DRCNTRL_KBDDATOUT;
	draco_reg[1] &= ~DRCNTRL_KBDCLKOUT;
	draco_kbd_state2 = -1;
	draco_kbd_poll = 0;
	draco_keyboard_send();
}

void draco_kdb_queue_add(void *d, uint8_t val, int state)
{
	draco_kbd_buffer[draco_kbd_buffer_len++] = val;
}


static uae_u8 draco_1wire_data[40], draco_1wire_state, draco_1wire_dir;
static uae_u8 draco_1wire_sram[512 + 32], draco_1wire_scratchpad[32 + 3], draco_1wire_rom[8];
static uae_u8 draco_1wire_cmd, draco_1wire_bytes, draco_1wire_dat;
static uae_s16 draco_1wire_sram_offset, draco_1wire_sram_offset_copy;
static uae_s8 draco_1wire_rom_offset, draco_1wire_cnt, draco_1wire_busycnt;
static bool draco_1wire_bit;

#define DS_ROM_MATCH            0x55
#define DS_ROM_SEARCH           0xf0
#define DS_ROM_SKIP             0xcc
#define DS_ROM_READ             0x33

#define DS_READ_MEMORY          0xf0
#define DS_WRITE_SCRATCHPAD     0x0f
#define DS_READ_SCRATCHPAD      0xaa
#define DS_COPY_SCRATCHPAD      0x55

static void draco_1wire_rtc_count(void)
{
	uae_u8 *rtc = draco_1wire_sram + 512;
	rtc[2]++;
	if (rtc[2] == 0) {
		rtc[3]++;
		if (rtc[3] == 0) {
			rtc[4]++;
			if (rtc[4] == 0) {
				rtc[5]++;
			}
		}
	}
}

static void draco_1wire_rtc_validate(void)
{
	uae_u8 *rtc = draco_1wire_sram + 512;
	rtc[0] &= ~7;
	draco_1wire_rtc_count();
}

uint8_t draco_1wire_crc8(uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;
	for (uint8_t i = 0; i < len; i++)
	{
		uint8_t inbyte = addr[i];
		for (uint8_t j = 0; j < 8; j++)
		{
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix)
				crc ^= 0x8C;

			inbyte >>= 1;
		}
	}
	return crc;
}

static void draco_1wire_set_bit(void)
{
	uae_u8 *dptr = NULL;
	int maxlen = 0;
	if (draco_1wire_cmd == DS_READ_MEMORY) {
		dptr = draco_1wire_sram;
		maxlen = sizeof(draco_1wire_sram);
	} else if (draco_1wire_cmd == DS_READ_SCRATCHPAD) {
		dptr = draco_1wire_scratchpad;
		maxlen = sizeof(draco_1wire_scratchpad);
	} else if (draco_1wire_cmd == DS_ROM_READ) {
		dptr = draco_1wire_rom;
		maxlen = sizeof(draco_1wire_rom);
	}
	if (dptr && draco_1wire_rom_offset >= 0 && draco_1wire_dir) {
		uae_u8 bit = 1;
		draco_1wire_dat = 0xff;
		if (draco_1wire_rom_offset < sizeof(draco_1wire_rom)) {
			if (draco_1wire_rom_offset == 7) {
				draco_1wire_dat = draco_1wire_crc8(draco_1wire_rom, sizeof(draco_1wire_rom) - 1);
			} else {
				draco_1wire_dat = draco_1wire_rom[draco_1wire_rom_offset];
			}
			bit = (draco_1wire_dat >> draco_1wire_cnt) & 1;
		}
		draco_1wire_bit = bit != 0;
	} else if (dptr && draco_1wire_sram_offset >= 0 && draco_1wire_dir) {
		if (draco_1wire_sram_offset >= maxlen) {
			draco_1wire_dat = 0xff;
			draco_1wire_bit = true;
		} else {
			if (draco_1wire_sram_offset >= 0) {
				draco_1wire_dat = dptr[draco_1wire_sram_offset];
				uae_u8 bit = (dptr[draco_1wire_sram_offset] >> draco_1wire_cnt) & 1;
				draco_1wire_bit = bit != 0;
			}
		}
	}
}

static void draco_1wire_read(void)
{
	uae_u8 v = draco_reg[3];

	if (draco_1wire_bit) {
		v |= DRSTAT_CLKDAT;
	} else {
		v &= ~DRSTAT_CLKDAT;
	}

	if (draco_1wire_cnt == 8 && !(v & DRSTAT_CLKBUSY)) {
#if ONEWIRE_DEBUG > 1
		write_log("draco read 1-wire SRAM byte %02x, %02x\n", draco_1wire_dat, draco_1wire_bytes);
#endif
		draco_1wire_cnt = 0;
		draco_1wire_bytes++;
		if (draco_1wire_rom_offset >= 0) {
			draco_1wire_rom_offset++;
//			if (draco_1wire_rom_offset == 8)
//				activate_debugger();
		} else if (draco_1wire_cmd == DS_READ_SCRATCHPAD) {
			if (draco_1wire_sram_offset == 2) {
				draco_1wire_sram_offset = (draco_1wire_sram_offset_copy & 31) + 3;
			} else if (draco_1wire_sram_offset < 2) {
				draco_1wire_sram_offset++;
			} else {
				draco_1wire_sram_offset++;
				draco_1wire_sram_offset -= 3;
				draco_1wire_sram_offset &= 31;
				draco_1wire_sram_offset += 3;
			}
		} else {
			draco_1wire_sram_offset++;
		}
	}
	
	if (v & DRSTAT_CLKBUSY) {
		draco_1wire_busycnt--;
		if (draco_1wire_busycnt < 0) {
			v &= ~DRSTAT_CLKBUSY;
		}
	}

	draco_reg[3] = v;
}

static void draco_1wire_busy(void)
{
	draco_reg[3] |= DRSTAT_CLKBUSY;
	draco_1wire_busycnt = 3;
}


static void draco_1wire_send(int bit)
{
	if (draco_1wire_dir) {
		draco_1wire_set_bit();
		draco_1wire_cnt++;
		draco_1wire_busy();
	}

	if (!draco_1wire_dir) {
		draco_1wire_data[0] >>= 1;
		draco_1wire_data[0] |= bit ? 0x80 : 0;
		draco_1wire_cnt++;
		draco_1wire_busy();
	}
	if (draco_1wire_cnt == 8 && !draco_1wire_dir) {
		bool gotcmd = false;
		draco_1wire_cnt = 0;
		if (!draco_1wire_state) {
			draco_1wire_cmd = draco_1wire_data[0];
			if (draco_1wire_cmd == DS_ROM_READ) {
				draco_1wire_rom_offset = 0;
				draco_1wire_dir = 1;
				draco_1wire_bytes = 0;
				draco_1wire_set_bit();
#if ONEWIRE_DEBUG
				write_log("draco received 1-wire ROM read command\n");
#endif
				return;
			} else if (draco_1wire_cmd != DS_ROM_SKIP) {
				draco_1wire_state = 1;
				gotcmd = true;
			} else {
				draco_1wire_rom_offset = -1;
			}
		}
		for (int i = sizeof(draco_1wire_data) - 1; i >= 1; i--) {
			draco_1wire_data[i] = draco_1wire_data[i - 1];
		}
#if ONEWIRE_DEBUG > 1
		write_log("draco received 1-wire byte %02x, cnt %02x\n", draco_1wire_data[0], draco_1wire_bytes);
#endif
		draco_1wire_bytes++;
		// read data command + 2 address bytes
		if (draco_1wire_cmd == DS_READ_MEMORY && draco_1wire_data[3] == DS_READ_MEMORY) {
			draco_1wire_sram_offset = (draco_1wire_data[1] << 8) | draco_1wire_data[2];
			draco_1wire_dir = 1;
			draco_1wire_bytes = 0;
#if ONEWIRE_DEBUG
			write_log("draco received 1-wire SRAM read command, offset %04x\n", draco_1wire_sram_offset);
#endif
		}
		// write scratchpad + 2 address bytes
		if (draco_1wire_cmd == DS_WRITE_SCRATCHPAD && draco_1wire_data[3] == DS_WRITE_SCRATCHPAD) {
			draco_1wire_sram_offset = (draco_1wire_data[1] << 8) | draco_1wire_data[2];
			draco_1wire_sram_offset_copy = draco_1wire_sram_offset;
			memset(draco_1wire_scratchpad, 0, sizeof(draco_1wire_scratchpad));
#if ONEWIRE_DEBUG
			write_log("draco received 1-wire SRAM scratchpad write command, offset %04x\n", draco_1wire_sram_offset);
#endif
		}
		// read scratchpad
		if (draco_1wire_cmd == DS_READ_SCRATCHPAD) {
			draco_1wire_sram_offset = 0;
			draco_1wire_dir = 1;
			draco_1wire_bytes = 0;
			draco_1wire_set_bit();
#if ONEWIRE_DEBUG
			write_log("draco received 1-wire SRAM scratchpad read command\n");
#endif
		}
		// copy scratchpad
		if (draco_1wire_cmd == DS_COPY_SCRATCHPAD && draco_1wire_data[4] == DS_COPY_SCRATCHPAD) {
			draco_1wire_sram_offset = 0;
			uae_u8 status = draco_1wire_data[1];
			if (status == draco_1wire_scratchpad[2] &&
				draco_1wire_data[2] == draco_1wire_scratchpad[1] &&
				draco_1wire_data[3] == draco_1wire_scratchpad[0]) {
				int start = draco_1wire_sram_offset_copy & 31;
				int offset = draco_1wire_sram_offset_copy & ~31;
#if ONEWIRE_DEBUG
				write_log("draco received 1-wire SRAM scratchpad copy command, accepted\n");
#endif
				for (int i = 0; i < 32; i++) {
					draco_1wire_sram[offset + start] = draco_1wire_scratchpad[start + 3];
#if ONEWIRE_DEBUG > 1
					write_log("draco 1-wire SRAM scratchpad copy %02x -> %04x\n", draco_1wire_sram[offset + start], offset + start);
#endif
					if (start == (draco_1wire_scratchpad[2] & 31)) {
						break;
					}
					start++;
					start &= 31;
				}
				draco_1wire_scratchpad[0] |= 0x80;
				draco_1wire_rtc_validate();
				draco_1wire_busy();
				draco_1wire_bit = 0;
			} else {
#if ONEWIRE_DEBUG
				write_log("draco received 1-wire SRAM scratchpad copy command, rejected\n");
#endif
				draco_1wire_busy();
			}
		}
	}
}
static void draco_1wire_reset(void)
{
	if (draco_1wire_state) {
		// write scratchpad
		if (draco_1wire_cmd == DS_WRITE_SCRATCHPAD) {
			int len = draco_1wire_bytes - 4;
			int start = draco_1wire_sram_offset_copy;
			for (int i = 0; i < len; i++) {
				draco_1wire_scratchpad[(start & 31) + 3] = draco_1wire_data[len - i];
				start++;
				start &= 31;
			}
			draco_1wire_scratchpad[0] = draco_1wire_sram_offset_copy >> 0;
			draco_1wire_scratchpad[1] = draco_1wire_sram_offset_copy >> 8;
			draco_1wire_scratchpad[2] = (start - 1) & 31;
			if (len > 32) {
				draco_1wire_scratchpad[2] |= 0x40;
			} else if (len < 32) {
				draco_1wire_scratchpad[2] |= 0x20;
			}
#if ONEWIRE_DEBUG
			write_log("draco received 1-wire SRAM scratchpad write, %d bytes received\n", len);
#endif
		}
	}

	memset(draco_1wire_data, 0, sizeof(draco_1wire_data));
	draco_1wire_cnt = 0;
	draco_1wire_state = 0;
	draco_1wire_dir = 0;
	draco_1wire_bytes = 0;
	draco_1wire_sram_offset = -1;
	draco_1wire_rom_offset = 0;
#if ONEWIRE_DEBUG
	write_log("draco 1-wire reset\n");
#endif
}

static uae_u16 draco_floppy_data;
static int draco_floppy_bits, draco_floppy_rate;

// draco reads amiga disks by polling register
// that returns time since last flux change.
static uae_u8 draco_floppy_get_data(void)
{
	if (draco_floppy_bits < 8) {
		uae_u16 t = floppy_get_raw_data(&draco_floppy_rate);
		draco_floppy_data |= (t & 0xff) << (8 - draco_floppy_bits);
		draco_floppy_bits += 8;
	}
	int bit1 = (draco_floppy_data & 0x8000);
	int bit2 = (draco_floppy_data & 0x4000);
	int bit3 = (draco_floppy_data & 0x2000);
	int bit4 = (draco_floppy_data & 0x1000);
	int v = 0;

	if (bit1) {
		draco_floppy_data <<= 1;
		draco_floppy_bits--;
		v = 8;
	} else if (bit2) {
		draco_floppy_data <<= 2;
		draco_floppy_bits -= 2;
		v = 24;
	} else if (bit3) {
		draco_floppy_data <<= 3;
		draco_floppy_bits -= 3;
		v = 40;
	} else if (bit4) {
		draco_floppy_data <<= 4;
		draco_floppy_bits -= 4;
		v = 56;
	}
	switch (draco_floppy_rate) {
		case FLOPPY_RATE_250K:
			// above values are in 250Kbs
			break;
		case FLOPPY_RATE_500K:
			v /= 2;
			break;
		case FLOPPY_RATE_300K:
			v = v * 30 / 25;
			break;
		case FLOPPY_RATE_1M:
			v /= 4;
			break;
	}
	return v;
}

static void vmotion_write(uaecptr addr, uae_u8 v)
{
}
static uae_u8 vmotion_read(uaecptr addr)
{
	static const uae_u8 vmdata[] = { 0xff, 0xce, 0x17, 0x47, 0x54, 0x17, 0x28, 0x00, 0x03, 0x06, 0x00, 0xff, 0xff, 0xff, 0xff, 0x03 };
	uae_u8 v = 0xff;
	if (addr < 64) {
		if ((addr & 3) == 0) {
			v = vmdata[addr >> 2];
		}
	}
	return v;
}

static uaecptr draco_convert_cia_addr(uaecptr addr)
{
	uaecptr ciaaddr = 0;
	addr &= 0xffff;
	if ((addr & 0x1001) == 0x1001 && (draco_cias & 2)) {
		ciaaddr = 0xbfe001 + (addr & 0xf00);
	}
	if ((addr & 0x1001) == 0x0000 && (draco_cias & 1)) {
		ciaaddr = 0xbfd000 + (addr & 0xf00);
	}
	return ciaaddr;
}

void draco_bustimeout(uaecptr addr)
{
	write_log("draco bus timeout %08x PC=%08x\n", addr, M68K_GETPC);
	if (currprefs.cs_compatible == CP_DRACO) {
		draco_reg[3] |= DRSTAT_BUSTIMO;
	} else {
		draco_reg[2] |= 0x80;
	}
}

static const uae_u8 casa_video_config20[] = { 0x00, 0xce, 0x17, 0x47, 0x54, 0x1f, 0x28, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 };

static uae_u8 casa_video_bget(uaecptr addr)
{
	uae_u8 v = 0;
	if (addr < 0x28000000) {
		if (addr & 3) {
			draco_bustimeout(addr);
		} else {
			int reg = (addr >> 2) & 15;
			v = casa_video_config20[reg];
		}
	} else {
		if ((addr & 3) != 3) {
			draco_bustimeout(addr);
		}
	}
	return v;
}

static uae_u8 casa_read_io(uaecptr addr)
{
	int reg = ((addr & 0xfc0) >> 6) & 0x1f;
	uae_u8 v = draco_reg[reg];
	switch(reg)
	{
		case 0x02:
		if (draco_reg[0] & 1) {
			//v |= 0x80;
		}
		break;
		// casablanca revision
		case 0x1f: // 0x7c3
		v = draco_revision;
		break;
	}
	return v;
}

static uae_u8 draco_read_io(uaecptr addr)
{
	// io
	int reg = addr & 0x1f;
	uae_u8 v = draco_reg[reg];
	switch (reg)
	{
		case 3:
			draco_keyboard_read();
			draco_1wire_read();
			v = draco_reg[reg];
			break;
		case 5:
#if KBD_DEBUG > 1
			write_log("draco keyboard scan code read %02x\n", v);
#endif
			draco_reg[3] &= ~DRSTAT_KBDRECV;
			break;
		case 9:
			v = draco_revision;
			draco_timer_latched = true;
			break;
		case 0xb:
			v = (draco_timer_latched ? draco_timer_latch : draco_timer) >> 8;
			break;
		case 0xd:
			v = (draco_timer_latched ? draco_timer_latch : draco_timer) >> 0;
			draco_timer_latched = false;
			break;
		case 0x1d:
			v = draco_floppy_get_data();
			break;
	}
	return v;
}

static uae_u32 REGPARAM2 draco_bget(uaecptr addr)
{
	uae_u8 v = 0;

	if (maxcnt >= 0) {
		if (addr != 0x020007c3) {
			write_log(_T("draco_bget %08x %08x\n"), addr, M68K_GETPC);
		}
		maxcnt--;
	}

	if (addr >= 0x28000000 && addr < 0x30000000 && draco_have_vmotion) {
		if (addr < 0x28000004) {
			draco_bustimeout(addr);
		} else {
			v = vmotion_read(addr & 0x07ffffff);
		}
		return v;
	}

	if (addr >= 0x20000000) {
		if (currprefs.cs_compatible == CP_CASABLANCA) {
			v = casa_video_bget(addr);
		} else {
			draco_bustimeout(addr);
		}
		return v;
	}

	if ((addr & 0x07c00000) == 0x04000000) {

		int reg = addr & 0xffff;
		v = cpuboard_ncr710_io_bget(reg);
//		write_log("draco scsi read %08x\n", addr);

	} else if ((addr & 0x07c00000) == 0x02400000) {

		// super io

		int reg = (addr & 0x7fff) >> 2;
		switch(reg)
		{
			case 0x3f0:
				if (draco_superio_idx >= 0) {
					v = draco_superio_idx;
				} else {
					v = x86_infloppy(reg);
				}
				break;
			case 0x3f1:
				if (draco_superio_idx >= 0 && draco_superio_idx < 16) {
					v = draco_superio_cfg[draco_superio_idx];
				} else {
					v = x86_infloppy(reg);
				}
				break;

			case 0x3f2:
			case 0x3f3:
			case 0x3f4:
			case 0x3f5:
			case 0x3f7:
				v = x86_infloppy(reg);
				break;

			case 0x3f8:
			case 0x3f9:
			case 0x3fa:
			case 0x3fb:
			case 0x3fc:
			case 0x3fd:
			case 0x3fe:
			case 0x3ff:
				v = serial_read(reg, draco_serial[0]);
				break;

			case 0x2f8:
			case 0x2f9:
			case 0x2fa:
			case 0x2fb:
			case 0x2fc:
			case 0x2fd:
			case 0x2fe:
			case 0x2ff:
				v = serial_read(reg, draco_serial[1]);
				break;

			case 0x278:
				// parallel
				break;

			default:
				write_log("draco superio read %04x = %02x\n", (addr >> 2) & 0xfff, v);
				break;
		}


	} else if ((addr & 0x07c00000) == 0x02000000) {

		if (currprefs.cs_compatible == CP_DRACO) {
			v = draco_read_io(addr);
		} else {
			v = casa_read_io(addr);
		}

	} else if ((addr & 0x07c00000) == 0x02800000) {

		// CIA (no CIAs if rev4+)
		if (draco_cias) {
			uaecptr cia_addr = draco_convert_cia_addr(addr);
			if (cia_addr) {
				v = cia_bank.bget(cia_addr);
			}
		}

	} else if ((addr & 0x07000000) == 0x01000000) {

		// interrupt control
		int reg = (addr & 0x0c00001);
		switch(reg)
		{
			case 0x000001:
				v = draco_intena;
				break;
			case 0x400001:
				v = draco_intpen;
				break;
			case 0x800001:
				v = draco_intfrc;
				break;
			case 0xc00001:
				v = 0;
				break;
		}

	} else if (addr < 0x00100000) {

		// boot ROM
		if (addr < dracoromsize) {
			v = dracorom[addr];
		}

	} else {

		write_log("draco unknown bank read %08x\n", addr);
	}

	return v;
}

static uae_u32 REGPARAM2 draco_lget(uaecptr addr)
{
	uae_u32 l = 0;

	if ((addr & 0x07c00000) == 0x04000000) {

		//write_log("draco scsi lput %08x %08x\n", addr, l);
		int reg = addr & 0xffff;
		l |= cpuboard_ncr710_io_bget(reg + 3) << 0;
		l |= cpuboard_ncr710_io_bget(reg + 2) << 8;
		l |= cpuboard_ncr710_io_bget(reg + 1) << 16;
		l |= cpuboard_ncr710_io_bget(reg + 0) << 24;

	} else if (addr < 0x00100000) {

		l = draco_bget(addr + 0) << 24;
		l |= draco_bget(addr + 1) << 16;
		l |= draco_bget(addr + 2) << 8;
		l |= draco_bget(addr + 3) << 0;

	} else {

		write_log(_T("draco_lget %08x %08x\n"), addr, M68K_GETPC);
	}

	return l;
}
static uae_u32 REGPARAM2 draco_wget(uaecptr addr)
{
	uae_u16 v = 0;

	if (addr < 0x00100000) {

		v = draco_bget(addr + 0) << 8;
		v |= draco_bget(addr + 1);

	} else {

		write_log(_T("draco_wget %08x %08x\n"), addr, M68K_GETPC);

	}

	return v;
}

static void REGPARAM2 draco_lput(uaecptr addr, uae_u32 l)
{
	if ((addr & 0x07c00000) == 0x04000000) {

		//write_log("draco scsi lput %08x %08x\n", addr, l);
		int reg = addr & 0xffff;
		cpuboard_ncr710_io_bput(reg + 3, l >> 0);
		cpuboard_ncr710_io_bput(reg + 2, l >> 8);
		cpuboard_ncr710_io_bput(reg + 1, l >> 16);
		cpuboard_ncr710_io_bput(reg + 0, l >> 24);

	} else if (addr < 0x28000000) {

		write_log(_T("draco_lput %08x %08x %08x\n"), addr, l, M68K_GETPC);
	}
}
static void REGPARAM2 draco_wput(uaecptr addr, uae_u32 w)
{
	write_log(_T("draco_wput %08x %04x %08x\n"), addr, w & 0xffff, M68K_GETPC);
}

static void casa_write_io(uaecptr addr, uae_u8 b)
{
	int reg = ((addr & 0xfc0) >> 6) & 0x1f;
	draco_reg[reg] = b;
	switch (reg)
	{
		case 1:
			draco_intena = b & 63;
			casa_irq();
			break;
		case 2:
			draco_intpen = b & 63;
			casa_irq();
			break;
		case 3:
			draco_intfrc = b & 63;
			casa_irq();
			break;
		case 4:
			casa_irq();
			break;
	}
}

static void draco_write_io(uaecptr addr, uae_u8 b)
{
	int reg = addr & 0x1f;
	uae_u8 oldval = draco_reg[reg];
	draco_reg[reg] = b;

	//write_log(_T("draco_bput %08x %02x %08x\n"), addr, b & 0xff, M68K_GETPC);
	switch (reg)
	{
		case 1:
			if (b & DRCNTRL_WDOGDAT) {
				draco_watchdog = 0;
			}
			draco_irq();
			draco_keyboard_write(b);
			break;
		case 3: // RO
			draco_reg[reg] = oldval;
			break;
		case 7:
			draco_irq();
			break;
		case 9:
			draco_reg[7] &= ~DRSTAT2_TMRIRQPEN;
			break;
		case 0x0b:
			draco_timer &= 0x00ff;
			draco_timer |= b << 8;
			break;
		case 0x0d:
			draco_timer &= 0xff00;
			draco_timer |= b;
			break;
		case 0x11:
			draco_1wire_send(0);
			break;
		case 0x13:
			draco_1wire_send(1);
			break;
		case 0x15:
			draco_1wire_reset();
			break;
		case 0x17:
			draco_keyboard_done();
			break;
		case 0x19:
			draco_reg[3] &= ~DRSTAT_BUSTIMO;
			break;
	}
}

static void REGPARAM2 draco_bput(uaecptr addr, uae_u32 b)
{
	if (maxcnt >= 0) {
		maxcnt--;
		write_log(_T("draco_bput %08x %02x %08x\n"), addr, b & 0xff, M68K_GETPC);
	}

	if (addr >= 0x20000000) {
		if (currprefs.cs_compatible == CP_DRACO) {
			if (addr >= 0x28000000 && addr < 0x30000000 && draco_have_vmotion) {
				vmotion_write(addr & 0x07ffffff, b);
				return;
			}
			draco_bustimeout(addr);
		}
		return;
	}

	if ((addr & 0x07c00000) == 0x04000000) {

//		write_log("draco scsi put %08x\n", addr);
		int reg = addr & 0xffff;
		cpuboard_ncr710_io_bput(reg, b);

	} else if ((addr & 0x07c00000) == 0x02400000) {

		// super io
		int reg = (addr & 0x7fff) >> 2;
		switch (reg)
		{
			case 0x3f0:
				if (b == 0x55 && draco_superio_idx < 0) {
					draco_superio_idx++;
				} else if (b == 0xaa && draco_superio_idx >= 0) {
					draco_superio_idx = -2;
				} else if (draco_superio_idx >= 0) {
					draco_superio_idx = b;
				} else {
					x86_outfloppy(reg, b);
				}
				break;
			case 0x3f1:
				if (draco_superio_idx >= 0 && draco_superio_idx < 16) {
					draco_superio_cfg[draco_superio_idx] = b;
				} else {
					x86_outfloppy(reg, b);
				}
				break;

			case 0x3f2:
			case 0x3f3:
			case 0x3f4:
			case 0x3f5:
			case 0x3f7:
				x86_outfloppy(reg, b);
				draco_superio_idx = -2;
				break;

			case 0x3f8:
			case 0x3f9:
			case 0x3fa:
			case 0x3fb:
			case 0x3fc:
			case 0x3fd:
			case 0x3fe:
			case 0x3ff:
				serial_write(reg, b, draco_serial[0]);
				break;

			case 0x2f8:
			case 0x2f9:
			case 0x2fa:
			case 0x2fb:
			case 0x2fc:
			case 0x2fd:
			case 0x2fe:
			case 0x2ff:
				serial_write(reg, b, draco_serial[1]);
				break;

			case 0x278:
				// parallel
				break;

			default:
				write_log("draco superio write %04x = %02x\n", (addr >> 2) & 0xfff, b);
				break;
		}

	} else if ((addr & 0x07c00000) == 0x02800000) {

		// CIA (no CIAs if rev4+)
		if (draco_cias) {
			uaecptr cia_addr = draco_convert_cia_addr(addr);
			if (cia_addr) {
				cia_bank.bput(cia_addr, b);
			}
		}

	} else if ((addr & 0x07c00000) == 0x02000000) {

		if (currprefs.cs_compatible == CP_DRACO) {
			draco_write_io(addr, b);
		} else {
			casa_write_io(addr, b);
		}

	} else if ((addr & 0x07000000) == 0x01000000) {

		// interrupt control
		int reg = (addr & 0x0c00001);
		switch (reg)
		{
			case 0x000001:
				draco_intena = b & 15;
				draco_irq();
				break;
			case 0x400001:
				draco_intpen = b & 15;
//				if (b)
//					write_log("draco interrupt 0x400001 write %02x %08x\n", b, M68K_GETPC);
				draco_irq();
				break;
			case 0x800001:
//				if (b)
//					write_log("draco interrupt 0x800001 write %02x %08x\n", b, M68K_GETPC);
				draco_intfrc = b & 15;
				draco_irq();
				break;
			case 0xc00001:
//				if (b)
//					write_log("draco interrupt 0xc00001 write %02x %08x\n", b, M68K_GETPC);
				draco_irq();
				break;
		}

	} else if (addr < 0x00100000) {

		// boot ROM

	} else {
	
		write_log("draco unknown bank write %08x\n", addr);

	}
}

static int REGPARAM2 draco_check(uaecptr addr, uae_u32 size)
{
	if (addr >= 0x00100000) {
		return 0;
	}
	return 1;
}

static uae_u8 *REGPARAM2 draco_xlate(uaecptr addr)
{
	addr &= 0xfffff;
	return dracorom + addr;
}

static addrbank draco_bank = {
	draco_lget, draco_wget, draco_bget,
	draco_lput, draco_wput, draco_bput,
	draco_xlate, draco_check, NULL, NULL, _T("DraCo mainboard"),
	draco_lget, draco_wget, ABFLAG_IO, S_READ, S_WRITE
};

void draco_ext_interrupt(bool i6)
{
	if (i6) {
		draco_intpen |= 8;
	} else {
		draco_intpen |= 4;
	}
	dc_irq();
}

void draco_keycode(uae_u16 scancode, uae_u8 state)
{
	if (currprefs.cs_compatible == CP_DRACO && (currprefs.cpuboard_settings & 0x10)) {
		if (draco_kbd_buffer_len == 0 && !(draco_reg[3] & DRSTAT_KBDRECV)) {
			draco_key_process(scancode, state);
		} else {
			draco_kbd_in_buffer[draco_kbd_in_buffer_len++] = scancode | (state ? 0x8000 : 0x00);
		}
	}
}

static void draco_hsync(void)
{
	uae_u16 tm = 5, ot;
	static int hcnt;

#if 0
	if (currprefs.cs_compatible == CP_CASABLANCA) {
		static int casa_timer;
		casa_timer++;
		if (casa_timer >= maxvpos) {
			draco_svga_irq(true);
		} else {
			draco_svga_irq(false);
		}
	}
#endif

	if (currprefs.cs_compatible == CP_DRACO) {
		ot = draco_timer;
		draco_timer -= tm;
		if ((draco_timer > 0xf000 && ot < 0x1000) || (draco_timer < 0x1000 && ot > 0xf000)) {
			draco_reg[7] |= DRSTAT2_TMRIRQPEN;
			if (draco_reg[7] & DRSTAT2_TMRINTENA) {
				draco_irq();
			}
		}

		if (draco_kbd_buffer_len > 0) {
			draco_keyboard_send();
		}

		if (draco_kbd_buffer_len == 0 && draco_kbd_in_buffer_len  > 0 && !(draco_reg[3] & DRSTAT_KBDRECV)) {
			uae_u8 code = (uae_u8)draco_kbd_in_buffer[0];
			uae_u8 state = (draco_kbd_in_buffer[0] & 0x8000) ? 1 : 0;
			for (int i = 1; i < draco_kbd_in_buffer_len; i++) {
				draco_kbd_in_buffer[i - i] = draco_kbd_in_buffer[i];
			}
			draco_kbd_in_buffer_len--;
			draco_key_process(code, state);
		}

		hcnt++;
		if (hcnt >= 60) {
			draco_1wire_rtc_count();
			hcnt = 0;
		}

		draco_watchdog++;
		if (0 && draco_watchdog > 312 * 50) {
			IRQ_forced(7, -1);
			activate_debugger();
			draco_watchdog = 0;
		}
	}

	x86_floppy_run();
}

void draco_set_scsi_irq(int id, int level)
{
	draco_scsi_intpen = level;
	dc_irq();
}


static void x86_irq(int irq, bool state)
{
	if (irq == 4) {
		draco_serial_intpen = state;
	} else {
		draco_fdc_intpen = state;
	}
	dc_irq();
}

void draco_free(void)
{
	if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
		TCHAR path[MAX_DPATH];
		cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
		struct zfile *draco_flashfile = zfile_fopen(path, _T("wb"), ZFD_NORMAL);
		if (draco_flashfile) {
			uae_u8 zeros[8] = { 0 };
			zfile_fwrite(draco_1wire_rom, sizeof(draco_1wire_rom), 1, draco_flashfile);
			zfile_fwrite(zeros, sizeof(zeros), 1, draco_flashfile);
			zfile_fwrite(draco_1wire_sram, sizeof(draco_1wire_sram), 1, draco_flashfile);
			zfile_fclose(draco_flashfile);
		}
	}
	xfree(draco_mouse_base);
	draco_mouse_base = NULL;
	xfree(dracorom);
	dracorom = NULL;
}

void draco_reset(int hardreset)
{

	x86_initfloppy(x86_irq);
	draco_serial_init(&draco_serial[0], &draco_serial[1]);
	draco_mouse_base = mouse_serial_init_draco();
	draco_keyboard = draco_keyboard_init();

	draco_intena = 0;
	draco_intpen = 0;
	draco_timer = 0;
	draco_timer_latched = 0;
	draco_timer_latched = false;
	draco_svga_irq_state = 0;
	draco_fdc_intpen = false;
	draco_superio_idx = -2;
	draco_watchdog = 0;
	draco_scsi_intpen = 0;
	draco_serial_intpen = 0;
	memset(draco_superio_cfg, 0, sizeof(draco_superio_cfg));
	draco_superio_cfg[0] = 0x3b;
	draco_superio_cfg[1] = 0x9f;
	draco_superio_cfg[2] = 0xdc;
	draco_superio_cfg[3] = 0x78;
	draco_superio_cfg[6] = 0xff;
	draco_superio_cfg[13] = 0x65;
	draco_superio_cfg[14] = 1;
	draco_floppy_data = 0;
	draco_floppy_bits = 0;
	memset(draco_reg, 0, sizeof(draco_reg));
	draco_reg[1] = DRCNTRL_FDCINTENA | DRCNTRL_KBDINTENA;
	draco_reg[5] = 0xff;

	draco_keyboard_reset();
	draco_1wire_rtc_validate();
	draco_1wire_rom[0] = 0x04;
	draco_1wire_rom[1] = 1;
	draco_1wire_rom[2] = 0;
	draco_1wire_rom[3] = 0;
	draco_1wire_rom[4] = 0;
	draco_1wire_rom[5] = 0;
	draco_1wire_rom[6] = 0;
	draco_1wire_rom[7] = 0;
}

void draco_init(void)
{
	if (currprefs.cs_compatible == CP_DRACO) {
		TCHAR path[MAX_DPATH];
		cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
		struct zfile *draco_flashfile = zfile_fopen(path, _T("rb"), ZFD_NORMAL);
		if (draco_flashfile) {
			zfile_fread(draco_1wire_rom, sizeof(draco_1wire_rom), 1, draco_flashfile);
			zfile_fseek(draco_flashfile, 8, SEEK_CUR);
			zfile_fread(draco_1wire_sram, sizeof(draco_1wire_sram), 1, draco_flashfile);
			zfile_fclose(draco_flashfile);
		}

		draco_revision = (currprefs.cpuboard_settings & 1) ? 4 : 3;
		draco_cias = draco_revision < 4 ? 2 : 0;
		if (currprefs.cpuboard_settings & 4) {
			draco_cias |= 1;
		}
		if (currprefs.cpuboard_settings & 8) {
			draco_cias |= 2;
		}
		int idx;
		const TCHAR *romname = NULL;
		struct boardromconfig *brc;
		brc = get_device_rom(&currprefs, ROMTYPE_CPUBOARD, 0, &idx);
		if (brc) {
			romname = brc->roms[idx].romfile;
			dracorom = zfile_load_file(romname, &dracoromsize);
		}

		draco_reset(1);

		device_add_rethink(draco_irq);
		device_add_hsync(draco_hsync);
		device_add_reset(draco_reset);
	}
	if (currprefs.cs_compatible == CP_CASABLANCA) {

		draco_revision = 9;
		draco_cias = 3;
		draco_reset(1);

		device_add_rethink(draco_irq);
		device_add_hsync(draco_hsync);
		device_add_reset(draco_reset);
	}
}

bool draco_mouse(int port, int x, int y, int z, int b)
{
	if (currprefs.cs_compatible == CP_DRACO || currprefs.cs_compatible == CP_CASABLANCA) {
		if (b < 0)
			return true;
		mouse_serial_poll(x, y, z, b, draco_mouse_base);
		return true;
	}
	return false;
}

void casablanca_map_overlay(void)
{
	// Casablanca has ROM at address zero, no chip ram, no overlay.
	map_banks(&kickmem_bank, 524288 >> 16, 524288 >> 16, 0);
	map_banks(&extendedkickmem_bank, 0 >> 16, 524288 >> 16, 0);
	map_banks(&draco_bank, 0x02000000 >> 16, 0x01000000 >> 16, 0);
	// KS ROM is here
	map_banks(&kickmem_bank, 0x02c00000 >> 16, 524288 >> 16, 0);
	map_banks(&draco_bank, 0x03000000 >> 16, 0x01000000 >> 16, 0);
	map_banks(&draco_bank, 0x20000000 >> 16, 0x20000000 >> 16, 0);
}


void draco_map_overlay(void)
{
	// hide custom registers
	map_banks(&dummy_bank, 0xd00000 >> 16, 0x200000 >> 16, 0);
	// hide cias
	map_banks(&dummy_bank, 0xa00000 >> 16, 0x200000 >> 16, 0);

	map_banks(&draco_bank, 0 >> 16, 0x00200000 >> 16, 0);
	map_banks(&draco_bank, 0x01000000 >> 16, 0x01c00000 >> 16, 0);
	map_banks(&kickmem_bank, 0x02c00000 >> 16, 524288 >> 16, 0);
	map_banks(&draco_bank, 0x04000000 >> 16, 0x01000000 >> 16, 0);
	map_banks(&draco_bank, 0x21000000 >> 16, 0x1f000000 >> 16, 0);
}
#endif //WITH_DRACO
