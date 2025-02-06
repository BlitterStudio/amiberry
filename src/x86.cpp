/*
* UAE - The Un*x Amiga Emulator
*
* X86 Bridge board emulation
* A1060, A2088, A2088T, A2286, A2386
*
* Copyright 2015 Toni Wilen
* 8088 x86 and PC support chip emulation from Fake86.
* 80286+ CPU and AT support chip emulation from DOSBox.
*
* 2018: Replaced Fake86 and DOSBox with PCem v14.
* 2020: PCem v16.
*
*/

#define X86_DEBUG_BRIDGE 1
#define FLOPPY_IO_DEBUG 0
#define X86_DEBUG_BRIDGE_IO 0
#define X86_DEBUG_BRIDGE_IRQ 0
#define X86_IO_PORT_DEBUG 0
#define X86_DEBUG_SPECIAL_IO 0
#define FLOPPY_DEBUG 1
#define EMS_DEBUG 0

#define DEBUG_DMA 0
#define DEBUG_PIT 0
#define DEBUG_INT 0

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef WITH_X86

#include "options.h"
#include "custom.h"
#include "memory.h"
#include "debug.h"
#include "x86.h"
#include "newcpu.h"
#include "uae.h"
#include "rommgr.h"
#include "zfile.h"
#include "disk.h"
#include "driveclick.h"
#include "scsi.h"
#include "idecontrollers.h"
#include "gfxboard.h"
#include "pci_hw.h"
#include "devices.h"
#include "audio.h"

#include "pcem/ibm.h"
#include "pcem/device.h"
#include "pcem/pic.h"
#include "pcem/pit.h"
#include "pcem/timer.h"
#include "pcem/pcemglue.h"
#include "pcem/model.h"
#include "pcem/mem.h"
#include "pcem/codegen.h"
#include "pcem/x86.h"
#include "pcem/nvr.h"
#include "pcem/keyboard_at.h"
#include "pcem/keyboard.h"
#include "pcem/dma.h"
#include "pcem/sound_speaker.h"
#include "pcem/sound.h"
#include "pcem/sound_cms.h"
#include "pcem/mouse.h"
#include "pcem/mouse_serial.h"
#include "pcem/serial.h"
extern int cpu;
extern int nvrmask;
void keyboard_at_write(uint16_t port, uint8_t val, void *priv);

#define MAX_IO_PORT 0x400

static uint8_t(*port_inb[MAX_IO_PORT])(uint16_t addr, void *priv);
static uint16_t(*port_inw[MAX_IO_PORT])(uint16_t addr, void *priv);
static uint32_t(*port_inl[MAX_IO_PORT])(uint16_t addr, void *priv);
static void(*port_outb[MAX_IO_PORT])(uint16_t addr, uint8_t  val, void *priv);
static void(*port_outw[MAX_IO_PORT])(uint16_t addr, uint16_t val, void *priv);
static void(*port_outl[MAX_IO_PORT])(uint16_t addr, uint32_t val, void *priv);
static void *port_priv[MAX_IO_PORT];

static float x86_base_event_clock;
static int x86_sndbuffer_playpos;
static int x86_sndbuffer_playindex;
static int sound_handlers_num;
static uint64_t sound_poll_latch;
static pc_timer_t sound_poll_timer;
void sound_poll(void *priv);
void sound_speed_changed(bool);

#define TYPE_SIDECAR 0
#define TYPE_2088 1
#define TYPE_2088T 2
#define TYPE_2286 3
#define TYPE_2386 4

void x86_doirq(uint8_t irqnum);

bool x86_cpu_active;
extern int cpu_multiplier;

static int x86_vga_mode;
static int x86_vga_board;
int x86_cmos_bank;

static uae_u8 *xtiderom;
static uae_u8 *rt1000rom;

int GAMEBLASTER = 1;
int sound_pos_global;
int SOUNDBUFLEN = 8192;
static int32_t outbuffer1[MAXSOUNDBUFLEN * 2];
static int32_t outbuffer2[MAXSOUNDBUFLEN * 2];
static int x86_sndbuffer_index;
int32_t *x86_sndbuffer[2];
bool x86_sndbuffer_filled[2];
bool ps2_mouse_supported;

struct membase
{
	uae_u8 *base;
	uae_u32 mask;
};

struct scamp_shadow
{
	mem_mapping_t r_map;
	mem_mapping_t w_map;
};

struct x86_bridge
{
	uae_u8 acmemory[128];
	addrbank *bank;
	int configured;
	int type;
	uaecptr baseaddress;
	uae_u8 *pc_ram;
	uae_u8 *pc_rom;
	uae_u8 *io_ports;
	uae_u8 *amiga_io;
	uae_u8 *amiga_parameter;
	uae_u8 *amiga_buffer;
	uae_u8 *amiga_mono;
	uae_u8 *amiga_color;
	int mode_register;
	bool x86_reset;
	bool amiga_irq;
	bool amiga_forced_interrupts;
	bool pc_irq3a, pc_irq3b, pc_irq7;
	int delayed_interrupt;
	uae_u8 pc_jumpers;
	int pc_maxbaseram;
	int bios_size;
	int settings;
	struct zfile *cmosfile;
	uae_u8 cmosregs[3 * 0x40];
	int a2386_default_video;
	bool pc_speaker;
	int cmossize;
	struct romconfig *rc;
	int vgaboard;
	int vgaboard_vram;
	const struct pci_board *ne2000_isa;
	struct pci_board_state *ne2000_isa_board_state;
	int ne2000_io;
	int ne2000_irq;
	mem_mapping_t sharedio_parameter;
	struct membase sharedio_parameter_mb;
	mem_mapping_t sharedio_mono;
	struct membase sharedio_mono_mb;
	mem_mapping_t sharedio_color;
	struct membase sharedio_color_mb;
	mem_mapping_t sharedio_buffer[3];
	uae_u32 shared_io_addr;
	struct membase sharedio_buffer_mb;
	mem_mapping_t vgamem;
	mem_mapping_t vgalfbmem;
	int audstream;
	int audeventtime;
	bool sound_emu;
	bool sound_initialized;
	void *sb_base;
	void *cms_base;
	void *mouse_base;
	int mouse_type;
	int mouse_port;
	bool video_initialized;

	struct scamp_shadow shadows[16];
	mem_mapping_t ems[0x24];
	int shadowvals[4];
	int emsvals[0x24];
	int sltptr;
	int scamp_idx1, scamp_idx2;
	uae_u8 vlsi_regs[0x100];
	uae_u16 vlsi_regs_ems[64];
	bool vlsi_config;
	int a2386flipper;
	bool a2386_amigapcdrive;

	X86_INTERRUPT_CALLBACK irq_callback;
};
static int x86_found;

#define X86_BRIDGE_A1060 0
#define X86_BRIDGE_MAX (X86_BRIDGE_A1060 + 1)

#define ACCESS_MODE_BYTE 0
#define ACCESS_MODE_WORD 1
#define ACCESS_MODE_GFX 2
#define ACCESS_MODE_IO 3

#define IO_AMIGA_INTERRUPT_STATUS 0x1ff1
#define IO_PC_INTERRUPT_STATUS 0x1ff3
#define IO_NEGATE_PC_RESET 0x1ff5
#define IO_MODE_REGISTER 0x1ff7
#define IO_INTERRUPT_MASK 0x1ff9
#define IO_PC_INTERRUPT_CONTROL 0x1ffb
#define IO_CONTROL_REGISTER 0x1ffd
#define IO_KEYBOARD_REGISTER_A1000 0x61f
#define IO_KEYBOARD_REGISTER_A2000 0x1fff
#define IO_A2386_CONFIG 0x1f9f

#define ISVGA() (bridges[0]->vgaboard >= 0)

static struct x86_bridge *bridges[X86_BRIDGE_MAX];

static struct x86_bridge *x86_bridge_alloc(void)
{
	struct x86_bridge *xb = xcalloc(struct x86_bridge, 1);
	return xb;
};

static void reset_cpu(void)
{
	struct x86_bridge *xb = bridges[0];

	resetx86();
}

static void reset_x86_hardware(struct x86_bridge *xb);
static void reset_x86_cpu(struct x86_bridge *xb)
{
	write_log(_T("x86 CPU reset\n"));
	reset_cpu();
}

uint8_t x86_get_jumpers(void)
{
	struct x86_bridge *xb = bridges[0];
	uint8_t v = 0;

	if (xb->type >= TYPE_2286) {
		if (xb->pc_maxbaseram > 512 * 1024)
			v |= 0x20;
	}

	if (xb->type == TYPE_2386) {
		// A2386 = software controlled
		if (xb->a2386_default_video)
			v |= 0x40;
	} else {
		// Others have default video jumper
		if (!(xb->settings & (1 << 14))) {
			// default video mde, 0 = cga, 1 = mda
			v |= 0x40;
		}
	}

	if (!(xb->settings & (1 << 15))) {
		// key lock state: 0 = on, 1 = off
		v |= 0x80;
	}
	return v;
}


static int get_shared_io(struct x86_bridge *xb)
{
	int opt = (xb->amiga_io[IO_MODE_REGISTER] >> 5) & 3;
	// disk buffer
	if (xb->type >= TYPE_2286) {
		switch (opt)
		{
		case 1:
			return 0xa0000;
		case 2:
		default:
			return 0xd0000;
		}
	}
	else {
		switch (opt)
		{
		case 1:
		default:
			return 0xa0000;
		case 2:
			return 0xd0000;
		case 3:
			return 0xe0000;
		}
	}
}

static void set_configurable_shared_io(struct x86_bridge *xb)
{
	if (get_shared_io(xb) == xb->shared_io_addr)
		return;
	xb->shared_io_addr = get_shared_io(xb);
	mem_mapping_disable(&xb->sharedio_buffer[0]);
	mem_mapping_disable(&xb->sharedio_buffer[1]);
	mem_mapping_disable(&xb->sharedio_buffer[2]);
	switch (xb->shared_io_addr)
	{
	case 0xa0000:
		mem_mapping_enable(&xb->sharedio_buffer[0]);
		break;
	case 0xd0000:
		mem_mapping_enable(&xb->sharedio_buffer[1]);
		break;
	case 0xe0000:
		mem_mapping_enable(&xb->sharedio_buffer[2]);
		break;
	}
	write_log(_T("Buffer IO = %08x\n"), xb->shared_io_addr);
}

static uae_u8 get_mode_register(struct x86_bridge *xb, uae_u8 v)
{
	if (xb->type == TYPE_SIDECAR) {
		// PC/XT + keyboard. Read-only.
		v = 0x84;
		if (!(xb->settings & (1 << 12)))
			v |= 0x02;
		if (!(xb->settings & (1 << 8)))
			v |= 0x08;
		if (!(xb->settings & (1 << 9)))
			v |= 0x10;
		if ((xb->settings & (1 << 10)))
			v |= 0x20;
		if ((xb->settings & (1 << 11)))
			v |= 0x40;
	} else if (xb->type < TYPE_2286) {
		// PC/XT (read-write)
		v |= 0x80;
	} else {
		// AT (read-write)
		v &= ~0x80;
	}
	set_configurable_shared_io(xb);
	return v;
}

static uae_u8 x86_bridge_put_io(struct x86_bridge *xb, uaecptr addr, uae_u8 v)
{
#if X86_DEBUG_BRIDGE_IO
	uae_u8 old = xb->amiga_io[addr];
	write_log(_T("IO write %08x %02x -> %02x\n"), addr, old, v);
#endif

	switch (addr)
	{
		// read-only
		case IO_AMIGA_INTERRUPT_STATUS:
		v = xb->amiga_io[addr];
		break;
		case IO_PC_INTERRUPT_STATUS:
		v = xb->amiga_io[addr];
#if X86_DEBUG_BRIDGE_IRQ
		write_log(_T("IO_PC_INTERRUPT_STATUS %02X -> %02x\n"), old, v);
#endif
		break;
		case IO_NEGATE_PC_RESET:
		v = xb->amiga_io[addr];
		break;
		case IO_PC_INTERRUPT_CONTROL:
		if ((v & 1) || (v & (2 | 4 | 8)) != (2 | 4 | 8)) {
#if X86_DEBUG_BRIDGE_IRQ
			write_log(_T("IO_PC_INTERRUPT_CONTROL %02X -> %02x\n"), old, v);
#endif
			if (xb->type < TYPE_2286 && (v & 1))
				x86_doirq(1);
			if (!(v & 2) || !(v & 4))
				x86_doirq(3);
			if (!(v & 8))
				x86_doirq(7);
		}
		break;
		case IO_CONTROL_REGISTER:
		if (!(v & 0x4)) {
			xb->x86_reset = true;
			reset_x86_hardware(xb);
		}
		if (!(v & 1))
			v |= 2;
		else if (!(v & 2))
			v |= 1;
		xb->a2386flipper = (v >> 5) & 3;
#if X86_DEBUG_BRIDGE_IO
		write_log(_T("IO_CONTROL_REGISTER %02X -> %02x\n"), old, v);
#endif
		break;
		case IO_KEYBOARD_REGISTER_A1000:
		if (xb->type == TYPE_SIDECAR) {
#if X86_DEBUG_BRIDGE_IO
			write_log(_T("IO_KEYBOARD_REGISTER %02X -> %02x\n"), old, v);
#endif
			xb->io_ports[0x60] = v;
		}
		break;
		case IO_KEYBOARD_REGISTER_A2000:
		if (xb->type >= TYPE_2088) {
#if X86_DEBUG_BRIDGE_IO
			write_log(_T("IO_KEYBOARD_REGISTER %02X -> %02x\n"), old, v);
#endif
			xb->io_ports[0x60] = v;
			if (xb->type >= TYPE_2286) {
				keyboard_send(v);
			}
		}
		break;
		case IO_MODE_REGISTER:
		v = get_mode_register(xb, v);
#if X86_DEBUG_BRIDGE_IO
		write_log(_T("IO_MODE_REGISTER %02X -> %02x\n"), old, v);
#endif
		break;
		case IO_INTERRUPT_MASK:
#if X86_DEBUG_BRIDGE_IO
		write_log(_T("IO_INTERRUPT_MASK %02X -> %02x\n"), old, v);
#endif
		break;
		case IO_A2386_CONFIG:
		write_log(_T("A2386 CONFIG BYTE %02x\n"), v);
		if (v == 8 || v == 9) {
			xb->a2386_default_video = v & 1;
			write_log(_T("A2386 Default mode = %s\n"), xb->a2386_default_video ? _T("MDA") : _T("CGA"));
		}
		if (v == 6 || v == 7) {
			xb->pc_speaker = (v & 1) != 0;
		}
		if (v == 10 || v == 11) {
			xb->a2386_amigapcdrive = (v & 1) != 0;
			write_log(_T("A2386 Flipper mode = %s\n"), xb->a2386_amigapcdrive ? _T("PC") : _T("Amiga"));
		}
		break;

		default:
		if (addr >= 0x400)
			write_log(_T("Unknown bridge IO write %08x = %02x\n"), addr, v);
		break;
	}

	xb->amiga_io[addr] = v;
	return v;
}
static uae_u8 x86_bridge_get_io(struct x86_bridge *xb, uaecptr addr)
{
	uae_u8 v = xb->amiga_io[addr];

	v = xb->amiga_io[addr];

	switch(addr)
	{
		case IO_AMIGA_INTERRUPT_STATUS:
		xb->amiga_io[addr] = 0;
#if X86_DEBUG_BRIDGE_IRQ
		if (v)
			write_log(_T("IO_AMIGA_INTERRUPT_STATUS %02x. CLEARED.\n"), v);
#endif
		break;
		case IO_PC_INTERRUPT_STATUS:
		v |= 0xf0;
#if X86_DEBUG_BRIDGE_IRQ
		write_log(_T("IO_PC_INTERRUPT_STATUS %02x\n"), v);
#endif
		break;
		case IO_NEGATE_PC_RESET:
		{
			if (xb->x86_reset) {
				reset_x86_cpu(xb);
				xb->x86_reset = false;
			}
			// because janus.library has stupid CPU loop wait
			int vp = vpos;
			while (vp == vpos) {
				x_do_cycles(maxhpos * CYCLE_UNIT);
			}
		}
		break;
		case IO_MODE_REGISTER:
		v = get_mode_register(xb, v);
#if X86_DEBUG_BRIDGE_IO
		write_log(_T("IO_MODE_REGISTER %02x\n"), v);
#endif
		break;

		default:
		if (addr >= 0x400)
			write_log(_T("Unknown bridge IO read %08x\n"), addr);
		break;
	}

#if X86_DEBUG_BRIDGE_IO > 1
	write_log(_T("IO read %08x %02x\n"), addr, v);
#endif
	return v;
}

static void x86_bridge_rethink(void);
static void set_interrupt(struct x86_bridge *xb, int bit)
{
	if (xb->amiga_io[IO_AMIGA_INTERRUPT_STATUS] & (1 << bit))
		return;
#if X86_DEBUG_BRIDGE_IRQ
	write_log(_T("IO_AMIGA_INTERRUPT_STATUS set bit %d\n"), bit);
#endif
	xb->amiga_io[IO_AMIGA_INTERRUPT_STATUS] |= 1 << bit;
	devices_rethink_all(x86_bridge_rethink);
}

void x86_ack_keyboard(void)
{
	set_interrupt(bridges[0], 4);
}

void x86_clearirq(uint8_t irqnum)
{
	struct x86_bridge *xb = bridges[0];
	if (xb->irq_callback) {
		xb->irq_callback(irqnum, false);
	} else {
		picintc(1 << irqnum);
	}
}

void x86_doirq(uint8_t irqnum)
{
	struct x86_bridge *xb = bridges[0];
	if (xb->irq_callback) {
		xb->irq_callback(irqnum, true);
	} else {
		picint(1 << irqnum);
	}
}

struct pc_floppy
{
	int seek_offset;
	int phys_cyl;
	int cyl;
	uae_u8 sector;
	uae_u8 head;
	bool disk_changed;
};

static struct pc_floppy floppy_pc[4];
static uae_u8 floppy_dpc;
static uae_s8 floppy_idx;
static uae_u8 floppy_dir;
static uae_s8 floppy_cmd_len;
static uae_u8 floppy_cmd[16];
static uae_u8 floppy_result[16];
static uae_u8 floppy_status[4];
static uae_u8 floppy_num;
static int floppy_seeking[4];
static uae_u8 floppy_seekcyl[4];
static int floppy_delay_hsync;
static bool floppy_did_reset;
static bool floppy_irq;
static bool floppy_specify_pio;
static uae_u8 *floppy_pio_buffer;
static int floppy_pio_len, floppy_pio_cnt, floppy_pio_active;
static uae_s8 floppy_rate;

#define PC_SEEK_DELAY 50

static void floppy_reset(void)
{
	struct x86_bridge *xb = bridges[0];

#if FLOPPY_DEBUG
	write_log(_T("Floppy reset\n"));
#endif
	floppy_idx = 0;
	floppy_dir = 0;
	floppy_did_reset = true;
	floppy_specify_pio = false;
	floppy_pio_active = 0;
	floppy_dpc = 0;
	floppy_cmd_len = 0;
	floppy_num = 0;
	floppy_delay_hsync = 0;
	floppy_irq = false;
	floppy_pio_len = 0;
	floppy_pio_cnt = 0;
	for (int i = 0; i < 4; i++) {
		floppy_seeking[i] = 0;
		floppy_seekcyl[i] = 0;
	}

	if (xb->type == TYPE_2286) {
		// apparently A2286 BIOS AT driver assumes
		// floppy reset also resets IDE.
		// Perhaps this is forgotten feature from
		// Commodore PC model that uses same BIOS variant?
		x86_ide_hd_put(-1, 0, 0);
	}
}

static void floppy_hardreset(void)
{
	struct x86_bridge *xb = bridges[0];
	if (xb->type >= TYPE_2286 || xb->type < 0) {
		floppy_rate = 0;
	} else {
		floppy_rate = -1;
	}
	floppy_reset();
}


static void do_floppy_irq2(void)
{
#if FLOPPY_DEBUG
	write_log(_T("floppy%d irq (enable=%d)\n"), floppy_num, (floppy_dpc & 8) != 0);
#endif
	if (floppy_dpc & 8) {
		floppy_irq = true;
		x86_doirq(6);
	}
}

static void do_floppy_irq(void)
{
	if (floppy_delay_hsync > 0) {
		do_floppy_irq2();
	}
	floppy_delay_hsync = 0;
}

static void do_floppy_seek(int num, int error)
{
	struct pc_floppy *pcf = &floppy_pc[num];

	disk_reserved_reset_disk_change(num);
	if (!error) {
		struct floppy_reserved fr = { 0 };
		bool valid_floppy = disk_reserved_getinfo(num, &fr);
		if (floppy_seekcyl[num] != pcf->phys_cyl) {

			pcf->disk_changed = false;

			if (floppy_seekcyl[num] > pcf->phys_cyl)
				pcf->phys_cyl++;
			else if (pcf->phys_cyl > 0)
				pcf->phys_cyl--;

#ifdef DRIVESOUND
			if (valid_floppy)
				driveclick_click(fr.num, pcf->phys_cyl);
#endif
#if FLOPPY_DEBUG
			write_log(_T("Floppy%d seeking.. %d\n"), num, pcf->phys_cyl);
#endif
			if (pcf->phys_cyl - pcf->seek_offset <= 0) {
				pcf->phys_cyl = 0;
				if (pcf->seek_offset) {
					floppy_seekcyl[num] = 0;
#if FLOPPY_DEBUG
					write_log(_T("Floppy%d early track zero\n"), num);
#endif
					pcf->seek_offset = 0;
				}
			}

			if (valid_floppy && fr.cyls < 80 && fr.drive_cyls >=80)
				pcf->cyl = pcf->phys_cyl / 2;
			else
				pcf->cyl = pcf->phys_cyl;

			floppy_seeking[num] = PC_SEEK_DELAY;
			disk_reserved_setinfo(num, pcf->cyl, pcf->head, 1);
			return;
		}

		if (valid_floppy && pcf->phys_cyl > fr.drive_cyls + 3) {
			pcf->seek_offset = pcf->phys_cyl - (fr.drive_cyls + 3);
			pcf->phys_cyl = fr.drive_cyls + 3;
			goto done;
		}

		if (valid_floppy && fr.cyls < 80 && fr.drive_cyls >= 80)
			pcf->cyl = pcf->phys_cyl / 2;
		else
			pcf->cyl = pcf->phys_cyl;

		if (floppy_seekcyl[num] != pcf->phys_cyl)
			return;
	}

done:
#if FLOPPY_DEBUG
	write_log(_T("Floppy%d seek done err=%d. pcyl=%d cyl=%d h=%d\n"), num, error, pcf->phys_cyl,pcf->cyl, pcf->head);
#endif
	floppy_status[0] = 0;
	floppy_status[0] |= 0x20; // seek end
	if (error) {
		floppy_status[0] |= 0x40; // abnormal termination
		floppy_status[0] |= 0x10; // equipment check
	}
	floppy_status[0] |= num;
	floppy_status[0] |= pcf->head ? 4 : 0;
	do_floppy_irq2();
}

static int floppy_exists(void)
{
	uae_u8 sel = floppy_dpc & 3;
	if (sel == 0)
		return 0;
	if (sel == 1)
		return 1;
	return -1;
}

static int floppy_selected(void)
{
	uae_u8 motormask = (floppy_dpc >> 4) & 15;
	uae_u8 sel = floppy_dpc & 3;
	if (floppy_exists() < 0)
		return -1;
	if (motormask & (1 << sel))
		return sel;
	return -1;
}

static void floppy_clear_irq(void)
{
	if (floppy_irq) {
		x86_clearirq(6);
		floppy_irq = false;
	}
}

static bool floppy_valid_format(struct floppy_reserved *fr)
{
	if (fr->secs == 11 || fr->secs == 22)
		return false;
	return true;
}

static bool floppy_valid_rate(struct floppy_reserved *fr)
{
	struct x86_bridge *xb = bridges[0];
	// A2386 BIOS sets 720k data rate for 720k, 1.2M and 1.4M drives
	// probably because it thinks Amiga half-speed drive is connected?
	if (xb->type == TYPE_2386 && fr->rate == 0 && (floppy_rate == 1 || floppy_rate == 2))
		return true;
	return fr->rate == floppy_rate || floppy_rate < 0;
}

static void floppy_format(struct x86_bridge *xb, bool real)
{
	uae_u8 cmd = floppy_cmd[0];
	struct pc_floppy *pcf = &floppy_pc[floppy_num];
	struct floppy_reserved fr = { 0 };
	bool valid_floppy = disk_reserved_getinfo(floppy_num, &fr);

#if FLOPPY_DEBUG
	write_log(_T("Floppy%d %s format MF=%d N=%d:SC=%d:GPL=%d:D=%d\n"),
		floppy_num, real ? _T("real") : _T("sim"), (floppy_cmd[0] & 0x40) ? 1 : 0,
		floppy_cmd[2], floppy_cmd[3], floppy_cmd[4], floppy_cmd[5]);
	write_log(_T("DMA addr %08x len %04x\n"), dma[2].page | dma[2].ac, dma[2].cb);
	write_log(_T("IMG: Secs=%d Heads=%d\n"), fr.secs, fr.heads);
#endif
	int secs = floppy_cmd[3];
	int sector = pcf->sector;
	int head = pcf->head;
	int cyl = pcf->cyl;
	if (valid_floppy && fr.img) {
		// TODO: CHRN values totally ignored
		pcf->head = (floppy_cmd[1] & 4) ? 1 : 0;
		uae_u8 buf[512];
		memset(buf, floppy_cmd[5], sizeof buf);
		uae_u8 *pioptr = floppy_pio_buffer;
		for (int i = 0; i < secs && i < fr.secs && !fr.wrprot; i++) {
			uae_u8 cx = 0, hx = 0, rx = 0, nx = 0;
			if (floppy_specify_pio) {
				if (real) {
					floppy_pio_cnt += 4;
					if (floppy_pio_cnt <= floppy_pio_len) {
						cx = *pioptr++;
						hx = *pioptr++;
						rx = *pioptr++;
						nx = *pioptr++;
					}
				} else {
					floppy_pio_len += 4;
				}
			} else {
				if (real) {
					cx = dma_channel_read(2);
					hx = dma_channel_read(2);
					rx = dma_channel_read(2);
					nx = dma_channel_read(2);
				}
			}
			pcf->sector = rx - 1;
#if FLOPPY_DEBUG
			write_log(_T("Floppy%d %d/%d: C=%d H=%d R=%d N=%d\n"), floppy_num, i, fr.secs, cx, hx, rx, nx);
#endif
			if (real) {
				zfile_fseek(fr.img, (pcf->cyl * fr.secs * fr.heads + pcf->head * fr.secs + pcf->sector) * 512, SEEK_SET);
				zfile_fwrite(buf, 1, 512, fr.img);
			}
			pcf->sector++;
		}
	} else {
		floppy_status[0] |= 0x40; // abnormal termination
		floppy_status[0] |= 0x10; // equipment check
	}
	floppy_cmd_len = 7;
	if (fr.wrprot) {
		floppy_status[0] |= 0x40; // abnormal termination
		floppy_status[1] |= 0x02; // not writable
	}
	floppy_result[0] = floppy_status[0];
	floppy_result[1] = floppy_status[1];
	floppy_result[2] = floppy_status[2];
	floppy_result[3] = pcf->cyl;
	floppy_result[4] = pcf->head;
	floppy_result[5] = pcf->sector + 1;
	floppy_result[6] = floppy_cmd[2];
	if (real) {
		floppy_delay_hsync = 10;
		disk_reserved_setinfo(floppy_num, pcf->cyl, pcf->head, 1);
	} else {
		pcf->cyl = cyl;
		pcf->sector = sector;
		pcf->head = head;
	}
}

static void floppy_write(struct x86_bridge *xb, bool real)
{
	uae_u8 cmd = floppy_cmd[0];
	struct pc_floppy *pcf = &floppy_pc[floppy_num];
	struct floppy_reserved fr = { 0 };
	bool valid_floppy = disk_reserved_getinfo(floppy_num, &fr);

#if FLOPPY_DEBUG
	write_log(_T("Floppy%d %s write MT=%d MF=%d C=%d:H=%d:R=%d:N=%d:EOT=%d:GPL=%d:DTL=%d\n"),
		floppy_num, real ? _T("real") : _T("sim"), (floppy_cmd[0] & 0x80) ? 1 : 0, (floppy_cmd[0] & 0x40) ? 1 : 0,
		floppy_cmd[2], floppy_cmd[3], floppy_cmd[4], floppy_cmd[5],
		floppy_cmd[6], floppy_cmd[7], floppy_cmd[8]);
	write_log(_T("DMA addr %08x len %04x\n"), dma[2].page | dma[2].ac, dma[2].ab);
#endif
	floppy_delay_hsync = 50;
	int eot = floppy_cmd[6];
	bool mt = (floppy_cmd[0] & 0x80) != 0;
	int sector = pcf->sector;
	int head = pcf->head;
	int cyl = pcf->cyl;
	if (valid_floppy) {
		if (fr.img && pcf->cyl != floppy_cmd[2]) {
			floppy_status[0] |= 0x40; // abnormal termination
			floppy_status[2] |= 0x20; // wrong cylinder
		} else if (fr.img) {
			int end = 0;
			pcf->sector = floppy_cmd[4] - 1;
			pcf->head = (floppy_cmd[1] & 4) ? 1 : 0;
			uae_u8 *pioptr = floppy_pio_buffer;
			while (!end && !fr.wrprot) {
				int len = 128 << floppy_cmd[5];
				uae_u8 buf[512] = { 0 };
				if (floppy_specify_pio) {
					for (int i = 0; i < 512 && i < len; i++) {
						if (real) {
							if (floppy_pio_cnt >= floppy_pio_len) {
								end = 1;
								break;
							}
							int v = *pioptr++;
							buf[i] = v;
							floppy_pio_cnt++;
						} else {
							floppy_pio_len++;
						}
					}
				} else {
					for (int i = 0; i < 512 && i < len; i++) {
						if (real) {
							int v = dma_channel_read(2);
							if (v < 0) {
								end = -1;
								break;
							}
							buf[i] = v;
							if (v >= 0x10000) {
								end = 1;
								break;
							}
						}
					}
				}
#if FLOPPY_DEBUG
				write_log(_T("LEN=%d END=%d C=%d H=%d S=%d. IMG S=%d H=%d\n"), len, end, pcf->cyl, pcf->head, pcf->sector, fr.secs, fr.heads);
#endif
				if (end < 0)
					break;
				if (real) {
					zfile_fseek(fr.img, (pcf->cyl * fr.secs * fr.heads + pcf->head * fr.secs + pcf->sector) * 512, SEEK_SET);
					zfile_fwrite(buf, 1, 512, fr.img);
				}
				pcf->sector++;
				if (pcf->sector == eot) {
					if (mt) {
						pcf->sector = 0;
						if (pcf->head)
							pcf->cyl++;
						pcf->head ^= 1;
					} else {
						break;
					}
				}
				if (pcf->sector >= fr.secs) {
					pcf->sector = 0;
					pcf->head ^= 1;
					break;
				}
			}
			floppy_result[3] = cyl;
			floppy_result[4] = pcf->head;
			floppy_result[5] = pcf->sector + 1;
			floppy_result[6] = floppy_cmd[5];
		} else {
			floppy_status[0] |= 0x40; // abnormal termination
			floppy_status[0] |= 0x10; // equipment check
		}
	}
	floppy_cmd_len = 7;
	if (fr.wrprot) {
		floppy_status[0] |= 0x40; // abnormal termination
		floppy_status[1] |= 0x02; // not writable
	}
	floppy_result[0] = floppy_status[0];
	floppy_result[1] = floppy_status[1];
	floppy_result[2] = floppy_status[2];
	if (real) {
		floppy_delay_hsync = 10;
		disk_reserved_setinfo(floppy_num, pcf->cyl, pcf->head, 1);
	} else {
		pcf->cyl = cyl;
		pcf->sector = sector;
		pcf->head = head;
	}
}

static void floppy_do_cmd(struct x86_bridge *xb)
{
	uae_u8 cmd = floppy_cmd[0];
	struct pc_floppy *pcf = &floppy_pc[floppy_num];
	struct floppy_reserved fr = { 0 };
	bool valid_floppy;

	valid_floppy = disk_reserved_getinfo(floppy_num, &fr);

#if FLOPPY_DEBUG
	if (floppy_cmd_len > 0) {
		write_log(_T("Command: "));
		for (int i = 0; i < floppy_cmd_len; i++) {
			write_log(_T("%02x "), floppy_cmd[i]);
		}
		write_log(_T("\n"));
	}
#endif

	if (cmd == 8) {
#if FLOPPY_DEBUG
		write_log(_T("Floppy%d Sense Interrupt Status\n"), floppy_num);
#endif
		floppy_cmd_len = 2;
#if FLOPPY_DEBUG
		if (floppy_irq) {
			write_log(_T("Floppy interrupt reset\n"));
		}
#endif
		if (!floppy_irq) {
			floppy_status[0] = 0x80;
			floppy_cmd_len = 1;
		} else if (floppy_did_reset) {
			floppy_did_reset = false;
			// 0xc0 status after reset.
			floppy_status[0] = 0xc0;
		}
		floppy_clear_irq();
		floppy_result[0] = floppy_status[0];
		floppy_result[1] = pcf->phys_cyl;
		floppy_status[0] = 0;
		goto end;
	}

	memset(floppy_status, 0, sizeof floppy_status);
	memset(floppy_result, 0, sizeof floppy_result);
	if (floppy_exists() >= 0) {
		if (pcf->head)
			floppy_status[0] |= 4;
		floppy_status[3] |= pcf->phys_cyl == 0 ? 0x10 : 0x00;
		floppy_status[3] |= pcf->head ? 4 : 0;
		floppy_status[3] |= 8; // two side
		if (fr.wrprot)
			floppy_status[3] |= 0x40; // write protected
		floppy_status[3] |= 0x20; // ready
	}
	floppy_status[3] |= floppy_dpc & 3;
	floppy_status[0] |= floppy_dpc & 3;
	floppy_cmd_len = 0;

	switch (cmd & 31)
	{
		case 3:
#if FLOPPY_DEBUG
		write_log(_T("Floppy%d Specify SRT=%d HUT=%d HLT=%d ND=%d\n"), floppy_num, floppy_cmd[1] >> 4, floppy_cmd[1] & 0x0f, floppy_cmd[2] >> 1, floppy_cmd[2] & 1);
#endif
		floppy_delay_hsync = -5;
		floppy_specify_pio = (floppy_cmd[2] & 1) != 0;
		if (floppy_specify_pio && !floppy_pio_buffer) {
			floppy_pio_buffer = xcalloc(uae_u8, 512 * 36);
		}
		break;

		case 4:
#if FLOPPY_DEBUG
		write_log(_T("Floppy%d Sense Drive Status\n"), floppy_num);
#endif
		floppy_delay_hsync = 5;
		floppy_result[0] = floppy_status[3];
		floppy_cmd_len = 1;
		break;

		case 5:
		{
			floppy_write(xb, true);
		}
		break;

		case 6:
		{
#if FLOPPY_DEBUG
			write_log(_T("Floppy%d read MT=%d MF=%d SK=%d C=%d:H=%d:R=%d:N=%d:EOT=%d:GPL=%d:DTL=%d\n"),
				floppy_num, (floppy_cmd[0] & 0x80) ? 1 : 0, (floppy_cmd[0] & 0x40) ? 1 : 0, (floppy_cmd[0] & 0x20) ? 1 : 0,
				floppy_cmd[2], floppy_cmd[3], floppy_cmd[4], floppy_cmd[5],
				floppy_cmd[6], floppy_cmd[7], floppy_cmd[8]);
			write_log(_T("DMA addr %08x len %04x\n"), dma[2].page | dma[2].ac, dma[2].cb);
			write_log(_T("IMG: Secs=%d Heads=%d\n"), fr.secs, fr.heads);
#endif
			floppy_delay_hsync = 50;
			int eot = floppy_cmd[6];
			bool mt = (floppy_cmd[0] & 0x80) != 0;
			int cyl = pcf->cyl;
			bool nodata = false;
			if (valid_floppy) {
				if (!floppy_valid_rate(&fr) || !floppy_valid_format(&fr)) {
					nodata = true;
				} else if (pcf->head && fr.heads == 1) {
					nodata = true;
				} else if (fr.img && pcf->cyl != floppy_cmd[2]) {
					floppy_status[0] |= 0x40; // abnormal termination
					floppy_status[2] |= 0x20; // wrong cylinder
				} else if (fr.img) {
					bool end = false;
					pcf->sector = floppy_cmd[4] - 1;
					pcf->head = (floppy_cmd[1] & 4) ? 1 : 0;
					floppy_pio_len = 0;
					floppy_pio_cnt = 0;
					uae_u8 *pioptr = floppy_pio_buffer;
					while (!end) {
						int len = 128 << floppy_cmd[5];
						uae_u8 buf[512];
						zfile_fseek(fr.img, (pcf->cyl * fr.secs * fr.heads + pcf->head * fr.secs + pcf->sector) * 512, SEEK_SET);
						zfile_fread(buf, 1, 512, fr.img);
						if (floppy_specify_pio) {
							memcpy(pioptr, buf, 512);
							pioptr += 512;
							floppy_pio_len += 512;
							floppy_pio_active = 1;
						} else {
							for (int i = 0; i < 512 && i < len; i++) {
								int v = dma_channel_write(2, buf[i]);
								if (v < 0 || v > 65535)
									end = true;
							}
						}
						pcf->sector++;
						if (pcf->sector == eot) {
							if (mt) {
								pcf->sector = 0;
								if (pcf->head)
									pcf->cyl++;
								pcf->head ^= 1;
							} else {
								end = true;
							}
						}
						if (pcf->sector >= fr.secs) {
							pcf->sector = 0;
							pcf->head ^= 1;
							end = true;
						}
					}
					floppy_result[3] = cyl;
					floppy_result[4] = pcf->head;
					floppy_result[5] = pcf->sector + 1;
					floppy_result[6] = floppy_cmd[5];
				} else {
					nodata = true;
				}
			}
			if (nodata) {
				floppy_status[0] |= 0x40; // abnormal termination
				floppy_status[1] |= 0x04; // no data
			}
			floppy_cmd_len = 7;
			floppy_result[0] = floppy_status[0];
			floppy_result[1] = floppy_status[1];
			floppy_result[2] = floppy_status[2];
			floppy_delay_hsync = 10;
			disk_reserved_setinfo(floppy_num, pcf->cyl, pcf->head, 1);
		}
		break;

		case 7:
#if FLOPPY_DEBUG
		write_log(_T("Floppy%d recalibrate\n"), floppy_num);
#endif
		if (valid_floppy) {
			floppy_seekcyl[floppy_num] = 0;
			floppy_seeking[floppy_num] = PC_SEEK_DELAY;
		} else {
			floppy_seeking[floppy_num] = -PC_SEEK_DELAY;
		}
		break;

		case 10:
#if FLOPPY_DEBUG
		write_log(_T("Floppy read ID\n"));
#endif
		if (!valid_floppy || !fr.img || !floppy_valid_rate(&fr) || (pcf->head && fr.heads == 1) || !floppy_valid_format(&fr)) {
			floppy_status[0] |= 0x40; // abnormal termination
			floppy_status[1] |= 0x04; // no data
		}
		floppy_cmd_len = 7;
		floppy_result[0] = floppy_status[0];
		floppy_result[1] = floppy_status[1];
		floppy_result[2] = floppy_status[2];
		floppy_result[3] = pcf->cyl;
		floppy_result[4] = pcf->head;
		floppy_result[5] = pcf->sector + 1;
		floppy_result[6] = 2;

		if (valid_floppy && fr.img) {
			pcf->sector++;
			pcf->sector %= fr.secs;
		}

		floppy_delay_hsync = maxvpos * 20;
		disk_reserved_setinfo(floppy_num, pcf->cyl, pcf->head, 1);
		break;

		case 13:
		{
			floppy_format(xb, true);
		}
		break;

		case 15:
		{
			int newcyl = floppy_cmd[2];
#if FLOPPY_DEBUG
			write_log(_T("Floppy%d seek %d->%d (max %d)\n"), floppy_num, pcf->phys_cyl, newcyl, fr.cyls);
#endif
			floppy_seekcyl[floppy_num] = newcyl;
			floppy_seeking[floppy_num] = valid_floppy ? PC_SEEK_DELAY : -PC_SEEK_DELAY;
			pcf->head = (floppy_cmd[1] & 4) ? 1 : 0;
		}
		break;

		case 16:
		floppy_status[0] = 0x90;
		floppy_cmd_len = 1;
		break;

		default:
		floppy_status[0] = 0x80;
		floppy_cmd_len = 1;
		break;

	}

end:
	if (floppy_cmd_len > 0) {
		floppy_idx = -1;
		floppy_dir = 1;
#if FLOPPY_DEBUG
		write_log(_T("Status return: "));
		for (int i = 0; i < floppy_cmd_len; i++) {
			write_log(_T("%02x "), floppy_result[i]);
		}
		write_log(_T("\n"));
#endif
	} else {
		floppy_idx = 0;
		floppy_dir = 0;
	}
}

static int draco_force_irq;

static void outfloppy(struct x86_bridge *xb, int portnum, uae_u8 v)
{
#if FLOPPY_IO_DEBUG
	write_log(_T("out floppy port %04x %02x\n"), portnum, v);
#endif

	switch (portnum)
	{
		case 0x3f2: // dpc
		if ((v & 4) && !(floppy_dpc & 4)) {
			floppy_reset();
			floppy_delay_hsync = 20;
		}
#if FLOPPY_IO_DEBUG
		write_log(_T("DPC=%02x: Motormask %02x sel=%d dmaen=%d reset=%d\n"), v, (v >> 4) & 15, v & 3, (v & 8) ? 1 : 0, (v & 4) ? 0 : 1);
#endif
#ifdef DRIVESOUND
		for (int i = 0; i < 2; i++) {
			int mask = 0x10 << i;
			if ((floppy_dpc & mask) != (v & mask)) {
				struct floppy_reserved fr = { 0 };
				bool valid_floppy = disk_reserved_getinfo(i, &fr);
				if (valid_floppy)
					driveclick_motor(fr.num, (v & mask) ? 1 : 0);
			}
		}
#endif
		floppy_dpc = v;
		if (xb->type < 0) {
			floppy_dpc |= 8;
		}
		floppy_num = v & 3;
		for (int i = 0; i < 2; i++) {
			disk_reserved_setinfo(i, floppy_pc[i].cyl, floppy_pc[i].head, floppy_selected() == i);
		}
		break;
		case 0x3f5: // data reg
		if (floppy_pio_len > 0 && floppy_pio_cnt < floppy_pio_len && floppy_pio_active) {
			floppy_pio_buffer[floppy_pio_cnt++] = v;
			if (floppy_pio_cnt >= floppy_pio_len) {
				floppy_pio_cnt = 0;
				floppy_pio_active = 0;
				floppy_clear_irq();
				floppy_do_cmd(xb);
				floppy_pio_cnt = 0;
				floppy_pio_len = 0;
			}
		} else {
			floppy_cmd[floppy_idx] = v;
			if (floppy_idx == 0) {
				floppy_cmd_len = -1;
				switch(v & 31)
				{
					case 3: // specify
					floppy_cmd_len = 3;
					break;
					case 4: // sense drive status
					floppy_cmd_len = 2;
					break;
					case 5: // write data
					floppy_cmd_len = 9;
					break;
					case 6: // read data
					floppy_cmd_len = 9;
					break;
					case 7: // recalibrate
					floppy_cmd_len = 2;
					break;
					case 8: // sense interrupt status
					floppy_cmd_len = 1;
					break;
					case 10: // read id
					floppy_cmd_len = 2;
					break;
					case 12: // perpendiculaor mode
					if (xb->type < 0) {
						floppy_cmd_len = 2;
					}
					break;
					case 13: // format track
					floppy_cmd_len = 6;
					break;
					case 15: // seek
					floppy_cmd_len = 3;
					break;
					case 16: // get versionm
					if (xb->type < 0) {
						floppy_cmd_len = 1;
					}
					break;
					case 19: // configure
					if (xb->type < 0) {
						floppy_cmd_len = 4;
					}
					break;
				}
				if (floppy_cmd_len < 0) {
					write_log(_T("Floppy unimplemented command %02x\n"), v);
					floppy_cmd_len = 1;
				}
			}
			floppy_idx++;
			if (floppy_idx >= floppy_cmd_len) {
				if (floppy_specify_pio && (floppy_cmd[0] & 31) == 5) {
					floppy_write(xb, false);
					floppy_pio_cnt = 0;
					floppy_pio_active = 0;
					if (floppy_pio_len > 0) {
						floppy_delay_hsync = 10;
						floppy_pio_active = 1;
					} else {
						floppy_write(xb, true);
					}
				} else if (floppy_specify_pio && (floppy_cmd[0] & 31) == 13) {
					floppy_format(xb, false);
					floppy_pio_cnt = 0;
					floppy_pio_active = 0;
					if (floppy_pio_len > 0) {
						floppy_delay_hsync = 10;
						floppy_pio_active = 1;
					} else {
						floppy_format(xb, true);
					}
				} else {
					floppy_do_cmd(xb);
				}
			}
		}
		break;
		case 0x3f7: // configuration control
		if (xb->type >= TYPE_2286) {
#if FLOPPY_DEBUG
			write_log(_T("FDC Control Register %02x\n"), v);
#endif
			floppy_rate = v & 3;
		} else {
			floppy_rate = -1;
		}
		break;
		default:
		write_log(_T("Unknown FDC %04x write %02x\n"), portnum, v);
		break;
	}
}

static uae_u8 infloppy(struct x86_bridge *xb, int portnum)
{
	uae_u8 v = 0;
	switch (portnum)
	{
		case 0x3f0: // PS/2 status A (draco)
		if (xb->type < 0) {
			struct floppy_reserved fr = { 0 };
			bool valid_floppy = disk_reserved_getinfo(floppy_num, &fr);
			v |= floppy_irq ? 0x80 : 0x00; // INT PENDING
			v |= 0x40; // nDRV2
			v |= fr.wrprot ? 0 : 2; // nWP
			v |= fr.cyl == 0 ? 0 : 16; // nTRK0
		}
		break;
		case 0x3f1: // PS/2 status B (draco)
		if (xb->type < 0) {
			v |= 0x80 | 0x40;
			if (floppy_dpc & 1)
				v |= 0x20;
			if ((floppy_dpc >> 4) & 1)
				v |= 0x01;
			if ((floppy_dpc >> 4) & 2)
				v |= 0x02;
		}
		break;
		case 0x3f2:
			v = floppy_dpc;
			break;
		case 0x3f4: // main status
		v = 0;
		if (!floppy_delay_hsync && (floppy_dpc & 4))
			v |= 0x80;
		if (floppy_idx || floppy_delay_hsync || floppy_pio_active) {
			v |= 0x10;
			if (floppy_pio_active)
				v |= 0x20;
		}
		if ((v & 0x80) && floppy_dir)
			v |= 0x40;
		for (int i = 0; i < 4; i++) {
			if (floppy_seeking[i])
				v |= 1 << i;
		}
		break;
		case 0x3f5: // data reg
		if (floppy_pio_len > 0 && floppy_pio_cnt < floppy_pio_len) {
			v = floppy_pio_buffer[floppy_pio_cnt++];
			if (floppy_pio_cnt >= floppy_pio_len) {
				floppy_pio_len = 0;
				floppy_pio_active = 0;
				floppy_clear_irq();
				floppy_delay_hsync = 50;
			}
		} else if (floppy_cmd_len && floppy_dir) {
			int idx = (-floppy_idx) - 1;
			if (idx < sizeof floppy_result) {
				v = floppy_result[idx];
				idx++;
				floppy_idx--;
				if (idx >= floppy_cmd_len) {
					floppy_cmd_len = 0;
					floppy_dir = 0;
					floppy_idx = 0;
					floppy_clear_irq();
				}
			}
		}
		break;
		case 0x3f7: // digital input register
		if (xb->type >= TYPE_2286 || xb->type < 0) {
			struct pc_floppy *pcf = &floppy_pc[floppy_num];
			struct floppy_reserved fr = { 0 };
			bool valid_floppy = disk_reserved_getinfo(floppy_num, &fr);
			v = 0x00;
			if (valid_floppy && fr.disk_changed && (floppy_dpc >> 4) & (1 << floppy_num)) {
				pcf->disk_changed = true;
			}
			if (pcf->disk_changed) {
				v = 0x80;
			}
		}
		break;
		default:
		write_log(_T("Unknown FDC %04x read\n"), portnum);
		break;
	}
#if FLOPPY_IO_DEBUG
	write_log(_T("in  floppy port %04x %02x\n"), portnum, v);
#endif
	return v;
}

uae_u16 floppy_get_raw_data(int *rate)
{
	struct floppy_reserved fr = { 0 };
	bool valid_floppy = disk_reserved_getinfo(floppy_num, &fr);
	*rate = fr.rate;
	if (valid_floppy) {
		return DSKBYTR_fake(fr.num);
	}
	return 0x8000;
}

static void set_cpu_turbo(struct x86_bridge *xb)
{
	cpu_multiplier = (int)currprefs.x86_speed_throttle;
	cpu_set();
	cpu_update_waitstates();
	cpu_set_turbo(0);
	cpu_set_turbo(1);
}

static void set_initial_cpu_turbo(struct x86_bridge *xb)
{
	xb->video_initialized = true;
	if (currprefs.x86_speed_throttle) {
		write_log(_T("Video initialized, activating x86 CPU turbo.\n"));
		set_cpu_turbo(xb);
	}
}

static void set_pc_address_access(struct x86_bridge *xb, uaecptr addr)
{
	if (addr >= 0xb0000 && addr < 0xb2000) {
		// mono
		if (xb->amiga_io[IO_MODE_REGISTER] & 8) {
			xb->delayed_interrupt |= 1 << 0;
		}
	}
	if (addr >= 0xb8000 && addr < 0xc0000) {
		// color
		if (xb->amiga_io[IO_MODE_REGISTER] & 16) {
			xb->delayed_interrupt |= 1 << 1;
		}
	}
}

static void set_pc_io_access(struct x86_bridge *xb, uaecptr portnum, bool write)
{
	uae_u8 mode_register = xb->amiga_io[IO_MODE_REGISTER];

	if (xb->mode_register < 0 || ((mode_register & (8 | 16)) != (xb->mode_register & (8 | 16)))) {
		xb->mode_register = mode_register;
		if (!(mode_register & 8)) {
			mem_mapping_disable(&xb->sharedio_mono);
		} else {
			mem_mapping_enable(&xb->sharedio_mono);
		}
		if (!(mode_register & 16)) {
			mem_mapping_disable(&xb->sharedio_color);
		} else {
			mem_mapping_enable(&xb->sharedio_color);
		}
	}

	if (write && (portnum == 0x3b1 || portnum == 0x3b3 || portnum == 0x3b5 || portnum == 0x3b7 || portnum == 0x3b8)) {
		if (!xb->video_initialized) {
			set_initial_cpu_turbo(xb);
		}
		// mono crt data register
		if (mode_register & 8) {
			xb->delayed_interrupt |= 1 << 2;
		}
	} else if (write && (portnum == 0x3d1 || portnum == 0x3d3 || portnum == 0x3d5 || portnum == 0x3d7 || portnum == 0x3d8 || portnum == 0x3d9 || portnum == 0x3dd)) {
		// color crt data register
		if (!xb->video_initialized) {
			set_initial_cpu_turbo(xb);
		}
		if (mode_register & 16) {
			xb->delayed_interrupt |= 1 << 3;
		}
	} else if (portnum >= 0x37a && portnum < 0x37b) {
		// LPT1
		set_interrupt(xb, 5);
	} else if (portnum >= 0x2f8 && portnum < 0x2f9) {
		// COM2
		set_interrupt(xb, 6);
	}
}

static bool is_port_enabled(struct x86_bridge *xb, uint16_t portnum)
{
	uae_u8 enables = xb->amiga_io[IO_MODE_REGISTER];
	// COM2
	if (portnum >= 0x2f8 && portnum < 0x300) {
		if (!(enables & 1))
			return false;
	}
	// LPT1
	if (portnum >= 0x378 && portnum < 0x37f) {
		if (!(enables & 2))
			return false;
	}
	// Keyboard
	// ???
	return true;
}

static uae_u8 get0x3da(struct x86_bridge *xb)
{
	static int toggle;
	uae_u8 v = 0;
	// not really correct but easy.
	if (vpos < 40) {
		v |= 8 | 1;
	}
	if (toggle) {
		// hblank or vblank active
		v |= 1;
	}
	// just toggle to keep programs happy and fast..
	toggle = !toggle;
	return v;
}


#define EMS_BANK 0x4000
#define EMS_MASK 0x3fff

static uint8_t mem_read_emsb(uint32_t addr, void *priv)
{
	uint8_t *p = (uint8_t*)priv;
	return p[addr & EMS_MASK];
}
static uint16_t mem_read_emsw(uint32_t addr, void *priv)
{
	uint8_t *p = (uint8_t*)priv;
	return ((uint16_t*)&p[addr & EMS_MASK])[0];
}
static uint32_t mem_read_emsl(uint32_t addr, void *priv)
{
	uint8_t *p = (uint8_t*)priv;
	return ((uint32_t*)&p[addr & EMS_MASK])[0];
}
static void mem_write_emsb(uint32_t addr, uint8_t val, void *priv)
{
	uint8_t *p = (uint8_t*)priv;
	p[addr & EMS_MASK] = val;
}
static void mem_write_emsw(uint32_t addr, uint16_t val, void *priv)
{
	uint8_t *p = (uint8_t*)priv;
	((uint16_t*)&p[addr & EMS_MASK])[0] = val;
}
static void mem_write_emsl(uint32_t addr, uint32_t val, void *priv)
{
	uint8_t *p = (uint8_t*)priv;
	((uint32_t*)&p[addr & EMS_MASK])[0] = val;
}

static uint8_t mem_read_shadowb(uint32_t addr, void *priv)
{
	struct scamp_shadow *m = (struct scamp_shadow*)priv;
	return ram[addr];
}
static uint16_t mem_read_shadoww(uint32_t addr, void *priv)
{
	struct scamp_shadow *m = (struct scamp_shadow*)priv;
	return ((uint16_t*)&ram[addr])[0];
}
static uint32_t mem_read_shadowl(uint32_t addr, void *priv)
{
	struct scamp_shadow *m = (struct scamp_shadow*)priv;
	return ((uint32_t*)&ram[addr])[0];
}
static void mem_write_shadowb(uint32_t addr, uint8_t val, void *priv)
{
	struct scamp_shadow *m = (struct scamp_shadow*)priv;
	ram[addr] = val;
}
static void mem_write_shadoww(uint32_t addr, uint16_t val, void *priv)
{
	struct scamp_shadow *m = (struct scamp_shadow*)priv;
	((uint16_t*)&ram[addr])[0] = val;
}
static void mem_write_shadowl(uint32_t addr, uint32_t val, void *priv)
{
	struct scamp_shadow *m = (struct scamp_shadow*)priv;
	((uint32_t*)&ram[addr])[0] = val;
}

static const uae_u32 shadow_bases[] = { 0xa0000, 0xc0000, 0xd0000, 0xe0000 };
static const uae_u32 shadow_sizes[] = { 0x08000, 0x04000, 0x04000, 0x08000 };

static void vlsi_set_mapping(struct x86_bridge *xb)
{
	int sltptr = xb->vlsi_regs[2];
	if (sltptr != xb->sltptr) {
		xb->sltptr = sltptr;
		if (sltptr < 0x0a)
			xb->vlsi_regs[0x0b] &= ~0x40; // disable ems backfill
		if (sltptr >= 0x0a && sltptr <= 0x0f)
			sltptr = 0x10;
		uae_u32 addr = sltptr << 16;
		mem_set_mem_state(addr, 640 * 1024, MEM_READ_INTERNAL | MEM_WRITE_INTERNAL);
		mem_set_mem_state(0x100000, (mem_size - 1024) * 1024, MEM_READ_INTERNAL | MEM_WRITE_INTERNAL);
		if (sltptr < 0x10) {
			mem_set_mem_state(addr, 640 * 1024 - addr, MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
		} else if (sltptr < 0xfe) {
			mem_set_mem_state(addr, (16384 - 128) * 1024 - addr, MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
		}
	}
}

static void vlsi_set_ems(struct x86_bridge *xb, bool all)
{
	int emsenablebits = (xb->vlsi_regs[0x0b] << 8) | (xb->vlsi_regs[0x0c]);
	bool emsenable = ((xb->vlsi_regs[0x0b] >> 7) & 1) != 0;
	bool emsbackfillenable = ((xb->vlsi_regs[0x0b] >> 6) & 1) != 0;
	for (int i = 0; i < 0x24; i++) {
		mem_mapping_t *m = &xb->ems[i];
		int v = xb->vlsi_regs_ems[i];
		if (!all && v == xb->emsvals[i])
			return;
		xb->emsvals[i] = v;
		uae_u32 baseaddr;
		uae_u32 emsaddr;
		if (i >= 0x0c) {
			baseaddr = 0x40000 + (i - 0x0c) * EMS_BANK;
		} else {
			bool emsmap = ((xb->vlsi_regs[0x0b] >> 4) & 1) != 0;
			if (emsmap) {
				if (i < 0x04)
					baseaddr = 0xa0000 + (i - 0) * EMS_BANK;
				else if (i < 0x08)
					baseaddr = 0xd0000 + (i - 4) * EMS_BANK;
				else
					baseaddr = 0xb0000 + (i - 8) * EMS_BANK;
			} else {
				baseaddr = 0xc0000 + i * EMS_BANK;
			}
		}
		emsaddr = xb->vlsi_regs_ems[i] << 14;
		if (((i < 0x0c && (emsenablebits & (1 << i))) || (i >= 0x0c && emsbackfillenable)) && emsenable) {
			uae_u8 *paddr = ram + emsaddr;
#if EMS_DEBUG
			if (!m->enable || m->base != baseaddr || m->exec != paddr)
				write_log(_T("EMS %06x = %08x (%p)\n"), baseaddr, emsaddr, paddr);
#endif
			mem_mapping_set_p(m, paddr);
			mem_mapping_set_addr(m, baseaddr, EMS_BANK);
			mem_mapping_set_exec(m, paddr);
		}	else {
#if EMS_DEBUG
			if (m->enable)
				write_log(_T("EMS %06x = -\n"), baseaddr);
#endif
			mem_mapping_disable(m);
		}
	}
}


static void vlsi_set_shadow(struct x86_bridge *xb, int base, uae_u8 v)
{
	uae_u32 shadow_start = shadow_bases[base];
	uae_u32 shadow_size = shadow_sizes[base];
	if (xb->shadowvals[base] == v)
		return;
	xb->shadowvals[base] = v;
	for (int i = 0; i < 4; i++) {
		int state = (v >> (i * 2)) & 3;
		struct scamp_shadow *m = &xb->shadows[base * 4 + i];
		switch (state)
		{
		case 0: // read = rom, write = rom
			mem_mapping_disable(&m->r_map);
			mem_mapping_disable(&m->w_map);
			break;
		case 1: // read = rom, write = ram
			mem_mapping_disable(&m->r_map);
			mem_mapping_enable(&m->w_map);
			break;
		case 2: // read = ram, write = rom
			mem_mapping_enable(&m->r_map);
			mem_mapping_disable(&m->w_map);
			break;
		case 3: // read = ram, write = ram
			mem_mapping_enable(&m->r_map);
			mem_mapping_enable(&m->w_map);
			break;
		}
		write_log(_T("%06x - %06x : shadow status=%d\n"), shadow_start, shadow_start + shadow_size - 1, state);
		shadow_start += shadow_size;
	}
}

static void vlsi_init_mapping(struct x86_bridge *xb)
{
	// Shadow
	for (int i = 0; i < 4; i++) {
		uae_u32 b = shadow_bases[i];
		uae_u32 s = shadow_sizes[i];
		for (int j = 0; j < 4; j++) {
			struct scamp_shadow *m = &xb->shadows[i * 4 + j];
			mem_mapping_add(&m->r_map, b, s,
				mem_read_shadowb, mem_read_shadoww, mem_read_shadowl,
				NULL, NULL, NULL,
				ram + b, MEM_READ_INTERNAL, m);
			mem_mapping_add(&m->w_map, b, s,
				NULL, NULL, NULL,
				mem_write_shadowb, mem_write_shadoww, mem_write_shadowl,
				ram + b, MEM_WRITE_INTERNAL, m);
			mem_mapping_disable(&m->r_map);
			mem_mapping_disable(&m->w_map);
			b += s;
		}
		xb->shadowvals[i] = -1;
	}
	// EMS
	for (int i = 0x0; i < 0xc; i++) {
		uae_u32 addr = 0xa0000 + i * EMS_BANK;
		mem_mapping_add(&xb->ems[i], addr, EMS_BANK,
			mem_read_emsb, mem_read_emsw, mem_read_emsl,
			mem_write_emsb, mem_write_emsw, mem_write_emsl,
			ram + addr, MEM_READ_INTERNAL | MEM_WRITE_INTERNAL, NULL);
		mem_mapping_disable(&xb->ems[i]);
	}
	// EMS backfill
	for (int i = 0x0c; i < 0x24; i++) {
		uae_u32 addr = 0x40000 + i * EMS_BANK;
		mem_mapping_add(&xb->ems[i], addr, EMS_BANK,
			mem_read_emsb, mem_read_emsw, mem_read_emsl,
			mem_write_emsb, mem_write_emsw, mem_write_emsl,
			ram + addr, MEM_READ_INTERNAL | MEM_WRITE_INTERNAL, NULL);
		mem_mapping_disable(&xb->ems[i]);
	}
	xb->vlsi_regs[2] = 0xff;
	xb->sltptr = 0xff;
}

static void vlsi_reset(struct x86_bridge *xb)
{
	memset(xb->vlsi_regs, 0, sizeof xb->vlsi_regs);
	vlsi_set_ems(xb, true);
	vlsi_set_mapping(xb);
}


static void vlsi_reg_out(struct x86_bridge *xb, int reg, uae_u8 v)
{
	int shadowbase = -1;

	switch(reg)
	{
		case 0x02: // SLTPTR
		write_log(_T("SLTPTR = %02x\n"), v);
		vlsi_set_ems(xb, true);
		vlsi_set_mapping(xb);
		break;

		case 0x03: // RAMMAP
		write_log(_T("RAMMAP = %02x\n"), v);
		if ((v & (0x40 | 0x20 | 0x10)) != 0x20)
			write_log(_T("RAMMAP unsupported configuration!\n"));
		if (mem_size > 4 * 1024 * 1024)
			v &= ~0x10; // REMAP384 not support if >4M RAM
		vlsi_set_ems(xb, true);
		vlsi_set_mapping(xb);
		break;

		case 0x0b: // ENSEN1
		case 0x0c: // ENSEN2
#if EMS_DEBUG
		write_log(_T("ENSEN%d = %02x\n"), reg == 0x0b ? 1 : 2, v);
#endif
		vlsi_set_ems(xb, true);
		vlsi_set_mapping(xb);
		break;

		case 0x0e: // ABAX
			shadowbase = 0;
		break;
		case 0x0f: // CAXS
			shadowbase = 1;
		break;
		case 0x10: // DAXS
			shadowbase = 2;
		break;
		case 0x11: // FEAXS
			shadowbase = 3;
		break;

		case 0x1d:
			x86_cmos_bank = (v & 0x20) ? 1 : 0;
		break;
	}
	if (shadowbase >= 0) {
		vlsi_set_shadow(xb, shadowbase, v);
		vlsi_set_mapping(xb);
	}
}

static void vlsi_autoinc(struct x86_bridge *xb)
{
	if (!(xb->scamp_idx1 & 0x40))
		return;
	int reg = xb->scamp_idx1 & 0x3f;
	reg++;
	xb->scamp_idx1 = (xb->scamp_idx1 & (0x80 | 0x40)) | (reg & 0x3f);
}

static void vlsi_out(struct x86_bridge *xb, int portnum, uae_u8 v)
{
#if 0
	switch (portnum)
	{
		case 0xf9: // config disable
		xb->vlsi_config = false;
		break;
		case 0xfb: // config enable
		xb->vlsi_config = true;
		break;
	}

	if (!xb->vlsi_config)
		return;
#endif

	switch (portnum)
	{
		case 0xe8:
		xb->scamp_idx1 = v;
		break;
		case 0xea:
		xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f] &= 0xff00;
		xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f] |= v << 0;
		break;
		case 0xeb:
		xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f] &= 0x00ff;
		xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f] |= v << 8;
#if EMS_DEBUG
		write_log(_T("EMS %02d = %04x\n"), xb->scamp_idx1, xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f]);
#endif
		vlsi_set_ems(xb, false);
		vlsi_set_mapping(xb);
		vlsi_autoinc(xb);
		break;
		case 0xec:
		xb->scamp_idx2 = v;
		break;
		case 0xed:
		xb->vlsi_regs[xb->scamp_idx2] = v;
		vlsi_reg_out(xb, xb->scamp_idx2, v);
		break;
		case 0xee: // fast a20 (write=disable)
		mem_a20_alt = 0;
		mem_a20_recalc();
		break;
		case 0xef: // fast reset
		break;
	}
	write_log(_T("VLSI_OUT %02x = %02x\n"), portnum, v);
}


static uae_u8 vlsi_in(struct x86_bridge *xb, int portnum)
{
	uae_u8 v = 0;
	switch (portnum)
	{
		case 0xe8:
		v = xb->scamp_idx1 | 0x80;
		break;
		case 0xea:
		v = (uae_u8)xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f];
		break;
		case 0xeb:
		v = xb->vlsi_regs_ems[xb->scamp_idx1 & 0x3f] >> 8;
		v |= ~0x03;
		vlsi_autoinc(xb);
		break;
		case 0xec:
		v = xb->scamp_idx2;
		break;
		case 0xed:
		v = xb->vlsi_regs[xb->scamp_idx2];
		if (xb->scamp_idx2 == 0)
			v = 0xd6; // version
		break;
		case 0xee: // fast a20 (read=enable)
		mem_a20_alt = 1;
		mem_a20_recalc();
		break;
		case 0xef: // fast reset
		softresetx86();
		cpu_set_edx();
		break;
	}
	write_log(_T("VLSI_IN %02x = %02x\n"), portnum, v);
	return v;
}

void portout(uint16_t portnum, uint8_t v)
{
	struct x86_bridge *xb = bridges[0];
	uae_u8 *io = xb->io_ports;
	int aio = -1;
	uae_u8 enables = xb->amiga_io[IO_MODE_REGISTER];
	bool mda_emu = (enables & 8) != 0;
	bool cga_emu = (enables & 16) != 0;

	if (portnum >= MAX_IO_PORT)
		return;

	if (!is_port_enabled(xb, portnum))
		return;

	set_pc_io_access(xb, portnum, true);

	if (xb->ne2000_isa && portnum >= xb->ne2000_io && portnum < xb->ne2000_io + 32) {
		xb->ne2000_isa->bars[0].bput(xb->ne2000_isa_board_state, portnum - xb->ne2000_io, v);
		goto end;
	}

	switch(portnum)
	{
		case 0x60:
		if (xb->type >= TYPE_2286) {
			keyboard_at_write(portnum, v, &pit);
		} else {
			aio = 0x41f;
		}
		break;
		case 0x61:
		//write_log(_T("OUT Port B %02x\n"), v);
		if (xb->type >= TYPE_2286) {
			keyboard_at_write(portnum, v, &pit);
		} else {
			timer_process();
			//timer_update_outstanding();
			speaker_update();
			speaker_gated = v & 1;
			speaker_enable = v & 2;
			if (speaker_enable)
				was_speaker_enable = 1;
			pit_set_gate(&pit, 2, v & 1);
			aio = 0x5f;
		}
		break;
		case 0x62:
#if X86_DEBUG_SPECIAL_IO
		write_log(_T("AMIGA SYSINT. %02x\n"), v);
#endif
		set_interrupt(xb, 7);
		aio = 0x3f;
		if (xb->type > TYPE_SIDECAR) {
			// default video mode bits 4-5 come from jumpers.
			if (!(xb->io_ports[0x63] & 8)) {
				xb->pc_jumpers &= ~0xcf;
				xb->pc_jumpers |= v & 0xcf;
			}
		}
		break;
		case 0x63:
#if X86_DEBUG_SPECIAL_IO
		write_log(_T("OUT CONFIG %02x\n"), v);
#endif
		if (xb->type > TYPE_SIDECAR) {
			if (xb->io_ports[0x63] & 8) {
				v |= 8;
			}
			if (xb->type == TYPE_2088T) {
				int speed = v >> 6;
				switch (speed)
				{
				case 0:
					write_log(_T("A2088T: 4.77MHz\n"));
					cpu_set_turbo(0);
					break;
				case 1:
					write_log(_T("A2088T: 7.15MHz\n"));
					cpu_set_turbo(1);
					break;
				case 2:
					write_log(_T("A2088T: 9.54MHz\n"));
					cpu_set_turbo(1);
					break;
				}
			}
		}
		break;
		case 0x64:
		if (xb->type >= TYPE_2286) {
			keyboard_at_write(portnum, v, &pit);
		}
		break;

		case 0x2f8:
		if (xb->amiga_io[0x13f] & 0x80)
			aio = 0x7f;
		else
			aio = 0x7d;
		break;
		case 0x2f9:
		if (xb->amiga_io[0x13f] & 0x80)
			aio = 0x9f;
		else
			aio = 0xbd;
		break;
		case 0x2fb:
		aio = 0x11f;
		break;
		case 0x2fc:
		aio = 0x13f;
		break;
		case 0x2fa:
		case 0x2fd:
		case 0x2fe:
		case 0x2ff:
		aio = 0x1f;
		break;

		// vga
		case 0x3c2:
		x86_vga_mode = v & 1;
		case 0x3c0:
		case 0x3c1:
		case 0x3c3:
		case 0x3c4:
		case 0x3c5:
		case 0x3c6:
		case 0x3c7:
		case 0x3c8:
		case 0x3c9:
		case 0x3ca:
		case 0x3cb:
		case 0x3cc:
		case 0x3cd:
		case 0x3ce:
		case 0x3cf:
		case 0x3b9:
		if (ISVGA()) {
			vga_io_put(xb->vgaboard, portnum, v);
		}
		break;

		// mono video
		case 0x3b0:
		case 0x3b2:
		case 0x3b4:
		case 0x3b6:
		if (mda_emu) {
			aio = 0x1ff;
		} else if (ISVGA()) {
			if (x86_vga_mode == 0)
				vga_io_put(xb->vgaboard, portnum, v);
		}
		break;
		case 0x3b1:
		case 0x3b3:
		case 0x3b5:
		case 0x3b7:
		if (mda_emu) {
			aio = 0x2a1 + (xb->amiga_io[0x1ff] & 15) * 2;
		} else if (ISVGA()) {
			if (x86_vga_mode == 0)
				vga_io_put(xb->vgaboard, portnum, v);
		}
		break;
		case 0x3b8:
		if (mda_emu) {
			aio = 0x2ff;
		}
		break;

		// color video
		case 0x3d0:
		case 0x3d2:
		case 0x3d4:
		case 0x3d6:
		if (cga_emu) {
			aio = 0x21f;
		} else if (ISVGA()) {
			if (x86_vga_mode == 1)
				vga_io_put(xb->vgaboard, portnum, v);
		}
		break;
		case 0x3d1:
		case 0x3d3:
		case 0x3d5:
		case 0x3d7:
		if (cga_emu) {
			aio = 0x2c1 + (xb->amiga_io[0x21f] & 15) * 2;
		} else if (ISVGA()) {
			if (x86_vga_mode == 1)
				vga_io_put(xb->vgaboard, portnum, v);
		}
		break;
		case 0x3d8:
		if (cga_emu) {
			aio = 0x23f;
		}
		break;
		case 0x3d9:
		if (cga_emu) {
			aio = 0x25f;
		}
		break;
		case 0x3dd:
		if (cga_emu) {
			aio = 0x29f;
		}
		break;

		case 0x3ba:
		if (cga_emu) {
			aio = 0x1f;
		} else if (ISVGA()) {
			if (x86_vga_mode == 0)
				vga_io_put(xb->vgaboard, portnum, v);
		}
		break;
		case 0x3da:
		if (cga_emu) {
			aio = 0x1f;
		} else if (ISVGA()) {
			if (x86_vga_mode == 1)
				vga_io_put(xb->vgaboard, portnum, v);
		}
		break;

		case 0x378:
		write_log(_T("BIOS DIAGNOSTICS CODE: %02x ~%02X\n"), v, v ^ 0xff);
		aio = 0x19f; // ??
		break;
		case 0x379:
#if X86_DEBUG_SPECIAL_IO
		write_log(_T("0x379: %02x\n"), v);
#endif
		if (xb->amiga_io[IO_MODE_REGISTER] & 2) {
			xb->amiga_forced_interrupts = (v & 40) ? false : true;
		}
		aio = 0x19f; // ??
		break;
		case 0x37a:
		aio = 0x1df;
		break;

		case 0x3bb:
		case 0x3bc:
		case 0x3bd:
		case 0x3be:
		case 0x3bf:
		if (mda_emu) {
			aio = 0x1f;
		}
		break;

		case 0x3de:
		case 0x3df:
		if (cga_emu) {
			aio = 0x1f;
		}
		break;

		// A2386SX only
		case 0xe8:
		case 0xea:
		case 0xeb:
		case 0xec:
		case 0xed:
		case 0xee:
		case 0xf0:
		case 0xf1:
		case 0xf4:
		case 0xf5:
		case 0xf9:
		case 0xfb:
			if (xb->type >= TYPE_2386)
				vlsi_out(xb, portnum, v);
		break;

		// floppy
		case 0x3f0:
		case 0x3f1:
		case 0x3f2:
		case 0x3f3:
		case 0x3f4:
		case 0x3f5:
		case 0x3f7:
		outfloppy(xb, portnum, v);
		break;

		// at ide 1
		case 0x170:
		case 0x171:
		case 0x172:
		case 0x173:
		case 0x174:
		case 0x175:
		case 0x176:
		case 0x177:
		case 0x376:
		x86_ide_hd_put(portnum, v, 0);
		break;
		// at ide 0
		case 0x1f0:
		case 0x1f1:
		case 0x1f2:
		case 0x1f3:
		case 0x1f4:
		case 0x1f5:
		case 0x1f6:
		case 0x1f7:
		case 0x3f6:
		x86_ide_hd_put(portnum, v, 0);
		break;

		// universal xt bios
		case 0x300:
		case 0x301:
		case 0x302:
		case 0x303:
		case 0x304:
		case 0x305:
		case 0x306:
		case 0x307:
		case 0x308:
		case 0x309:
		case 0x30a:
		case 0x30b:
		case 0x30c:
		case 0x30d:
		case 0x30e:
		case 0x30f:
		x86_ide_hd_put(portnum, v, 0);
		break;

		default:
		if (port_outb[portnum]) {
			port_outb[portnum](portnum, v, port_priv[portnum]);
		} else {
			write_log(_T("X86_OUT unknown %02x %02x\n"), portnum, v);
			return;
		}
		break;
	}

end:
#if X86_IO_PORT_DEBUG
	write_log(_T("X86_OUT %08x %02X\n"), portnum, v);
#endif
	if (aio >= 0)
		xb->amiga_io[aio] = v;
	xb->io_ports[portnum] = v;
}
void portout16(uint16_t portnum, uint16_t value)
{
	struct x86_bridge *xb = bridges[0];

	if (portnum >= MAX_IO_PORT)
		return;

	if (xb->ne2000_isa && portnum >= xb->ne2000_io && portnum < xb->ne2000_io + 32) {
		xb->ne2000_isa->bars[0].wput(xb->ne2000_isa_board_state, portnum - xb->ne2000_io, value);
		return;
	}

	switch (portnum)
	{
		case 0x170:
		case 0x1f0:
		case 0x300:
		x86_ide_hd_put(portnum, value, 1);
		break;
		default:
		if (port_outw[portnum]) {
			port_outw[portnum](portnum, value, port_priv[portnum]);
		} else {
			portout(portnum, (uae_u8)value);
			portout(portnum + 1, value >> 8);
		}
		break;
	}
}

void portout32(uint16_t portnum, uint32_t value)
{
	if (portnum >= MAX_IO_PORT)
		return;

	switch (portnum)
	{
		case 0x170:
		case 0x1f0:
		case 0x300:
		x86_ide_hd_put(portnum, value, 1);
		x86_ide_hd_put(portnum, value >> 16, 1);
		break;
		default:
		write_log(_T("portout32 %08x %08x\n"), portnum, value);
		break;
	}
}

uint8_t portin(uint16_t portnum)
{
	struct x86_bridge *xb = bridges[0];
	int aio = -1;
	uae_u8 enables = xb->amiga_io[IO_MODE_REGISTER];
	bool mda_emu = (enables & 8) != 0;
	bool cga_emu = (enables & 16) != 0;

	if (!is_port_enabled(xb, portnum))
		return 0;

	if (portnum >= MAX_IO_PORT)
		return 0;

	set_pc_io_access(xb, portnum, false);

	uae_u8 v = xb->io_ports[portnum];

	if (xb->ne2000_isa && portnum >= xb->ne2000_io && portnum < xb->ne2000_io + 32) {
		v = xb->ne2000_isa->bars[0].bget(xb->ne2000_isa_board_state, portnum - xb->ne2000_io);
		goto end;
	}

	switch (portnum)
	{
		case 0x2f8:
		if (xb->amiga_io[0x11f] & 0x80)
			aio = 0x7f;
		else
			aio = 0x9d;
		xb->pc_irq3b = false;
		break;
		case 0x2f9:
		if (xb->amiga_io[0x11f] & 0x80)
			aio = 0x9f;
		else
			aio = 0xdd;
		break;
		case 0x2fa:
		aio = 0xff;
		break;
		case 0x2fd:
		aio = 0x15f;
		break;
		case 0x2fe:
		aio = 0x17f;
		break;
		case 0x2fb:
		case 0x2fc:
		case 0x2ff:
		aio = 0x1f;
		break;

		case 0x378:
		aio = 0x19f; // ?
		break;
		case 0x379:
		xb->pc_irq7 = false;
		aio = 0x1bf;
		break;
		case 0x37a:
		aio = 0x19f; // ?
		break;

		// vga
		case 0x3c0:
		case 0x3c1:
		case 0x3c2:
		case 0x3c3:
		case 0x3c4:
		case 0x3c5:
		case 0x3c6:
		case 0x3c7:
		case 0x3c8:
		case 0x3c9:
		case 0x3ca:
		case 0x3cb:
		case 0x3cc:
		case 0x3cd:
		case 0x3ce:
		case 0x3cf:
		case 0x3b9:
		if (ISVGA()) {
			v = vga_io_get(xb->vgaboard, portnum);
		}
		break;

		// mono video
		case 0x3b0:
		xb->pc_irq3a = false;
		aio = 0x1ff;
		break;
		case 0x3b2:
		case 0x3b4:
		case 0x3b6:
		if (mda_emu) {
			aio = 0x1ff;
		} else if (ISVGA()) {
			if (x86_vga_mode == 0)
				v = vga_io_get(xb->vgaboard, portnum);
		}
		break;
		case 0x3b1:
		case 0x3b3:
		case 0x3b5:
		case 0x3b7:
		if (mda_emu) {
			aio = 0x2a1 + (xb->amiga_io[0x1ff] & 15) * 2;
		} else if (ISVGA()) {
			if (x86_vga_mode == 0)
				v = vga_io_get(xb->vgaboard, portnum);
		}
		break;
		case 0x3b8:
		if (mda_emu) {
			aio = 0x2ff;
		}
		break;

		// color video
		case 0x3d0:
		case 0x3d2:
		case 0x3d4:
		case 0x3d6:
		if (cga_emu) {
			aio = 0x21f;
		} else if (ISVGA()) {
			if (x86_vga_mode == 1)
				v = vga_io_get(xb->vgaboard, portnum);
		}
		break;
		case 0x3d1:
		case 0x3d3:
		case 0x3d5:
		case 0x3d7:
		if (cga_emu) {
			aio = 0x2c1 + (xb->amiga_io[0x21f] & 15) * 2;
		} else if (ISVGA()) {
			if (x86_vga_mode == 1)
				v = vga_io_get(xb->vgaboard, portnum);
		}
		break;
		case 0x3d8:
		if (cga_emu) {
			aio = 0x23f;
		}
		break;
		case 0x3d9:
		if (cga_emu) {
			aio = 0x25f;
		}
		break;

		case 0x3ba:
		if (mda_emu) {
			v = get0x3da(xb);
		} else if (ISVGA()) {
			if (x86_vga_mode == 0)
				v = vga_io_get(xb->vgaboard, portnum);
		}
		break;
		case 0x3da:
		if (cga_emu) {
			v = get0x3da(xb);
		} else if (ISVGA()) {
			if (x86_vga_mode == 1)
				v = vga_io_get(xb->vgaboard, portnum);
		}
		break;
		case 0x3dd:
		if (cga_emu) {
			aio = 0x29f;
		}
		break;

		case 0x3bb:
		case 0x3bc:
		case 0x3bd:
		case 0x3be:
		case 0x3bf:
		if (mda_emu) {
			aio = 0x1f;
		}
		break;
		case 0x3de:
		case 0x3df:
		if (cga_emu) {
			aio = 0x1f;
		}
		break;

		// A2386SX only
		case 0xe8:
		case 0xea:
		case 0xeb:
		case 0xec:
		case 0xed:
		case 0xee:
		case 0xef:
		if (xb->type >= TYPE_2386)
			v = vlsi_in(xb, portnum);
		break;

		// floppy
		case 0x3f0:
		case 0x3f1:
		case 0x3f2:
		case 0x3f3:
		case 0x3f4:
		case 0x3f5:
		case 0x3f7:
		v = infloppy(xb, portnum);
		break;

		case 0x60:
		if (xb->type >= TYPE_2286) {
			v = keyboard_at_read(portnum, NULL);
			//write_log(_T("PC read keycode %02x\n"), v);
		}
		break;
		case 0x61:
		if (xb->type >= TYPE_2286) {
			// bios test hack
			v = keyboard_at_read(portnum, NULL);
		} else {
			v = xb->amiga_io[0x5f];
		}
		//write_log(_T("IN Port B %02x\n"), v);
		break;
		case 0x62:
		{
			v = xb->amiga_io[0x3f];
			if (xb->type == TYPE_SIDECAR) {
				// Sidecar has jumpers.
				if (xb->amiga_io[0x5f] & 8) {
					// bit 0-1: display (11=mono 80x25,10=01=color 80x25,00=no video)
					// bit 2-3: number of drives (11=1..)
					v &= 0xf0;
					v |= (xb->pc_jumpers >> 4) & 0x0f;
				} else {
					v &= 0xf0;
					// bit 0: 0=loop on POST
					// bit 1: 0=8087 installed
					// bit 2-3: (11=640k,10=256k,01=512k,00=128k) RAM size
					v |= xb->pc_jumpers & 0xf;
				}
			} else {
				// A2088+ are software configurable (Except default video mode)
				if (!(xb->amiga_io[0x5f] & 4)) {
					v &= 0xf0;
					v |= (xb->pc_jumpers >> 4) & 0x0f;
				} else {
					v &= 0xf0;
					v |= xb->pc_jumpers & 0xf;
				}
			}
			v &= ~0x20;
			if (!(xb->amiga_io[0x5f] & 1)) {
				bool timer2gate = pit.gate[2] == 0;
				if (timer2gate)
					v |= 0x20;
			}
			//write_log(_T("IN Port C %02x\n"), v);
		}
		break;
		case 0x63:
		//write_log(_T("IN Control %02x\n"), v);
		break;
		case 0x64:
		if (xb->type >= TYPE_2286) {
			v = keyboard_at_read(portnum, NULL);
		}
		break;

		// at ide 1
		case 0x170:
		case 0x171:
		case 0x172:
		case 0x173:
		case 0x174:
		case 0x175:
		case 0x176:
		case 0x177:
		case 0x376:
		v = (uae_u8)x86_ide_hd_get(portnum, 0);
		break;
		// at ide 0
		case 0x1f0:
		case 0x1f1:
		case 0x1f2:
		case 0x1f3:
		case 0x1f4:
		case 0x1f5:
		case 0x1f6:
		case 0x1f7:
		case 0x3f6:
		v = (uae_u8)x86_ide_hd_get(portnum, 0);
		break;

		// universal xt bios
		case 0x300:
		case 0x301:
		case 0x302:
		case 0x303:
		case 0x304:
		case 0x305:
		case 0x306:
		case 0x307:
		case 0x308:
		case 0x309:
		case 0x30a:
		case 0x30b:
		case 0x30c:
		case 0x30d:
		case 0x30e:
		case 0x30f:
		v = (uae_u8)x86_ide_hd_get(portnum, 0);
		break;

		// led debug (a2286 bios uses this)
		case 0x108:
		case 0x109:
		case 0x10a:
		case 0x10b:
		case 0x10c:
		case 0x10d:
		case 0x10e:
		case 0x10f:
			break;

		default:
		if (port_inb[portnum]) {
			v = port_inb[portnum](portnum, port_priv[portnum]);
		} else {
			write_log(_T("X86_IN unknown %02x\n"), portnum);
			return 0;
		}
		break;
	}

end:
	if (aio >= 0)
		v = xb->amiga_io[aio];

#if X86_IO_PORT_DEBUG
	write_log(_T("X86_IN %08x %02X\n"), portnum, v);
#endif

	return v;
}
uint16_t portin16(uint16_t portnum)
{
	struct x86_bridge *xb = bridges[0];
	uae_u16 v = 0;

	if (portnum >= MAX_IO_PORT)
		return v;

	if (xb->ne2000_isa && portnum >= xb->ne2000_io && portnum < xb->ne2000_io + 32) {
		v = xb->ne2000_isa->bars[0].wget(xb->ne2000_isa_board_state, portnum - xb->ne2000_io);
		return v;
	}
	switch (portnum)
	{
		case 0x170:
		case 0x1f0:
		case 0x300:
		v = x86_ide_hd_get(portnum, 1);
		break;
		default:
		if (port_inw[portnum]) {
			v = port_inw[portnum](portnum, port_priv[portnum]);
		} else {
			v = (portin(portnum) << 0) | (portin(portnum + 1) << 8);
		}
		break;
	}
	return v;
}

uint32_t portin32(uint16_t portnum)
{
	uint32_t v = 0;

	if (portnum >= MAX_IO_PORT)
		return v;

	switch (portnum)
	{
		case 0x170:
		case 0x1f0:
		case 0x300:
		v = x86_ide_hd_get(portnum, 1) << 0;
		v |= x86_ide_hd_get(portnum, 1) << 16;
		break;
		default:
		write_log(_T("portin32 %08x\n"), portnum);
		break;
	}
	return v;
}

static uaecptr get_x86_address(struct x86_bridge *xb, uaecptr addr, int *mode, uae_u8 **base)
{
	addr -= xb->baseaddress;
	if (!xb->baseaddress || addr >= 0x80000) {
		*mode = -1;
		return 0;
	}
	*mode = addr >> 17;
	addr &= 0x1ffff;
	if (addr < 0x10000) {
		*base = xb->amiga_buffer + addr;
		return get_shared_io(xb) + addr;
	}
	if (addr < 0x18000) {
		// color video
		addr -= 0x10000;
		*base = xb->amiga_color + addr;
		return 0xb8000 + addr;
	}
	if (addr < 0x1c000) {
		// parameter
		addr -= 0x18000;
		if (xb->type >= TYPE_2286) {
			if (get_shared_io(xb) == 0xd0000) {
				*base = xb->amiga_buffer + addr;
				return 0xd0000 + addr;
			}
			*base = xb->amiga_parameter + addr;
			return 0xd0000 + addr;
		} else {
			*base = xb->amiga_parameter + addr;
			return 0xf0000 + addr;
		}
	}
	if (addr < 0x1e000) {
		// mono video
		addr -= 0x1c000;
		*base = xb->amiga_mono + addr;
		return 0xb0000 + addr;
	}
	// IO
	addr -= 0x1e000;
	return addr;
}

static struct x86_bridge *get_x86_bridge(uaecptr addr)
{
	return bridges[0];
}

static void cga_gfx_read(struct x86_bridge *xb, uaecptr a_addr, uae_u8 *addr, uae_u8 *plane0p, uae_u8 *plane1p)
{
	uae_u16 w;

	if (a_addr & 1)
		w = (addr[-1] << 8) | addr[0];
	else
		w = (addr[0] << 8) | addr[1];

	uae_u8 plane1 = (((w >> 14) & 1) << 7) | (((w >> 12) & 1) << 6) | (((w >> 10) & 1) << 5) | (((w >> 8) & 1) << 4);
	plane1 |= (((w >> 6) & 1) << 3) | (((w >> 4) & 1) << 2) | (((w >> 2) & 1) << 1) | (((w >> 0) & 1) << 0);

	uae_u8 plane0 = (((w >> 15) & 1) << 7) | (((w >> 13) & 1) << 6) | (((w >> 11) & 1) << 5) | (((w >> 9) & 1) << 4);
	plane0 |= (((w >> 7) & 1) << 3) | (((w >> 5) & 1) << 2) | (((w >> 3) & 1) << 1) | (((w >> 1) & 1) << 0);

	*plane0p = plane0;
	*plane1p = plane1;
}

static uae_u32 REGPARAM2 x86_bridge_wget(uaecptr addr)
{
	uae_u16 v = 0;
	uae_u8 *base;
	struct x86_bridge *xb = get_x86_bridge(addr);
	if (!xb)
		return v;
	int mode;
	uaecptr a = get_x86_address(xb, addr, &mode, &base);

	switch (mode)
	{
		case -1:
		break;
		case ACCESS_MODE_WORD:
		v = (base[1] << 8) | (base[0] << 0);
		break;
		case ACCESS_MODE_GFX:
		{
			uae_u8 plane0, plane1;
			cga_gfx_read(xb, a, base, &plane0, &plane1);
			if (a & 1)
				v = (plane1 << 8) | plane0;
			else
				v = (plane0 << 8) | plane1;
		}
		break;
		case ACCESS_MODE_IO:
		v = x86_bridge_get_io(xb, a);
		v |= x86_bridge_get_io(xb, a + 1) << 8;
		break;
		default:
		v = (base[1] << 0) | (base[0] << 8);
		break;
	}
#if X86_DEBUG_BRIDGE > 1
	write_log(_T("x86_bridge_wget %08x (%08x,%d)=%04x PC=%08x\n"), addr - xb->baseaddress, a, mode, v, M68K_GETPC);
#endif
	return v;
}
static uae_u32 REGPARAM2 x86_bridge_lget(uaecptr addr)
{
	uae_u32 v;
	v = x86_bridge_wget(addr) << 16;
	v |= x86_bridge_wget(addr + 2);
	return v;
}

static uae_u32 REGPARAM2 x86_bridge_bget(uaecptr addr)
{
	uae_u8 v = 0;
	uae_u8 *base;
	struct x86_bridge *xb = get_x86_bridge(addr);
	if (!xb)
		return v;
	if (!xb->configured) {
		uaecptr offset = addr & 65535;
		if (offset >= sizeof xb->acmemory)
			return 0;
		return xb->acmemory[offset];
	}
	int mode;
	uaecptr a = get_x86_address(xb, addr, &mode, &base);
	switch(mode)
	{
		case -1:
		break;
		case ACCESS_MODE_WORD:
		v = base[(a ^ 1) & 1];
		break;
		case ACCESS_MODE_IO:
		v = x86_bridge_get_io(xb, a);
		break;
		case ACCESS_MODE_GFX:
		{
			uae_u8 plane0, plane1;
			cga_gfx_read(xb, a, base, &plane0, &plane1);
			if (a & 1)
				v = plane1;
			else
				v = plane0;
		}
		break;
		default:
		v = base[0];
		break;
	}
#if X86_DEBUG_BRIDGE > 1
	write_log(_T("x86_bridge_bget %08x (%08x,%d)=%02x PC=%08x\n"), addr - xb->baseaddress, a, mode, v, M68K_GETPC);
#endif
	return v;
}

static void REGPARAM2 x86_bridge_wput(uaecptr addr, uae_u32 b)
{
	struct x86_bridge *xb = get_x86_bridge(addr);
	if (!xb)
		return;
	int mode;
	uae_u8 *base;
	uaecptr a = get_x86_address(xb, addr, &mode, &base);

#if X86_DEBUG_BRIDGE > 1
	write_log(_T("pci_bridge_wput %08x (%08x,%d) %04x PC=%08x\n"), addr - xb->baseaddress, a, mode, b & 0xffff, M68K_GETPC);
#endif

	if (a >= 0x100000 || mode < 0)
		return;

	switch (mode)
	{
		case -1:
		break;
		case ACCESS_MODE_IO:
		x86_bridge_put_io(xb, a + 0, b & 0xff);
		x86_bridge_put_io(xb, a + 1, (b >> 8) & 0xff);
		break;
		case ACCESS_MODE_WORD:
		base[0] = b;
		base[1] = b >> 8;
		break;
		case ACCESS_MODE_GFX:
		// does not seem to be used
		//write_log(_T("ACCESS_MODE_GFX %08x %04x\n"), addr, b & 0xffff);
		break;
		default:
		base[1] = b;
		base[0] = b >> 8;
		break;
	}
}
static void REGPARAM2 x86_bridge_lput(uaecptr addr, uae_u32 b)
{
	x86_bridge_wput(addr, b >> 16);
	x86_bridge_wput(addr + 2, b);
}
static void REGPARAM2 x86_bridge_bput(uaecptr addr, uae_u32 b)
{
	struct x86_bridge *xb = get_x86_bridge(addr);
	if (!xb)
		return;
	if (!xb->configured) {
		uaecptr offset = addr & 65535;
		switch (offset)
		{
			case 0x48:
			map_banks_z2(xb->bank, b, expamem_board_size >> 16);
			xb->baseaddress = b << 16;
			xb->configured = 1;
			expamem_next(xb->bank, NULL);
			break;
			case 0x4c:
			xb->configured = -1;
			expamem_shutup(xb->bank);
			break;
		}
	}

	int mode;
	uae_u8 *base;
	uaecptr a = get_x86_address(xb, addr, &mode, &base);

	if (a >= 0x100000 || mode < 0)
		return;

#if X86_DEBUG_BRIDGE > 1
	write_log(_T("x86_bridge_bput %08x (%08x,%d) %02x PC=%08x\n"), addr - xb->baseaddress, a, mode, b & 0xff, M68K_GETPC);
#endif

	switch(mode)
	{
		case -1:
		break;
		case ACCESS_MODE_IO:
		x86_bridge_put_io(xb, a, b);
		break;
		case ACCESS_MODE_WORD:
		base[(a ^ 1) & 1] = b;
		break;
		case ACCESS_MODE_GFX:
		// does not seem to be used
		//write_log(_T("ACCESS_MODE_GFX %08x %02x\n"), addr, b & 0xff);
		break;
		default:
		base[0] = b;
		break;
	}
}

addrbank x86_bridge_bank = {
	x86_bridge_lget, x86_bridge_wget, x86_bridge_bget,
	x86_bridge_lput, x86_bridge_wput, x86_bridge_bput,
	default_xlate, default_check, NULL, NULL, _T("X86 BRIDGE"),
	x86_bridge_lget, x86_bridge_wget,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static uint8_t mem_read_sharedb(uint32_t addr, void *priv)
{
	struct membase *mb = (struct membase*)priv;
	return mb->base[addr & mb->mask];
}
static uint16_t mem_read_sharedw(uint32_t addr, void *priv)
{
	struct membase *mb = (struct membase*)priv;
	return ((uae_u16*)(mb->base + (addr & mb->mask)))[0];
}
static uint32_t mem_read_sharedl(uint32_t addr, void *priv)
{
	struct membase *mb = (struct membase*)priv;
	uae_u8 *base = (uae_u8*)priv;
	return ((uae_u32*)(mb->base + (addr & mb->mask)))[0];
}
static void mem_write_sharedb(uint32_t addr, uint8_t val, void *priv)
{
	struct membase *mb = (struct membase*)priv;
	set_pc_address_access(bridges[0], addr);
	mb->base[addr & mb->mask] = val;
}
static void mem_write_sharedw(uint32_t addr, uint16_t val, void *priv)
{
	struct membase *mb = (struct membase*)priv;
	set_pc_address_access(bridges[0], addr);
	((uae_u16*)(mb->base + (addr & mb->mask)))[0] = val;
}
static void mem_write_sharedl(uint32_t addr, uint32_t val, void *priv)
{
	struct membase *mb = (struct membase*)priv;
	set_pc_address_access(bridges[0], addr);
	((uae_u32*)(mb->base + (addr & mb->mask)))[0] = val;
}

static uint8_t vga_readb(uint32_t addr, void *priv)
{
	return vga_ram_get(x86_vga_board, addr);
}
static uint16_t vga_readw(uint32_t addr, void *priv)
{
	return (vga_ram_get(x86_vga_board, addr) << 0) | (vga_ram_get(x86_vga_board, addr + 1) << 8);
}
static uint32_t vga_readl(uint32_t addr, void *priv)
{
	return (vga_readw(addr, priv) << 0) | (vga_readw(addr + 2, priv) << 16);
}
static void vga_writeb(uint32_t addr, uint8_t val, void *priv)
{
	vga_ram_put(x86_vga_board, addr, val);
}
static void vga_writew(uint32_t addr, uint16_t val, void *priv)
{
	vga_ram_put(x86_vga_board, addr, (uae_u8)val);
	vga_ram_put(x86_vga_board, addr + 1, val >> 8);
}
static void vga_writel(uint32_t addr, uint32_t val, void *priv)
{
	vga_ram_put(x86_vga_board, addr, val);
	vga_ram_put(x86_vga_board, addr + 1, val >> 8);
	vga_ram_put(x86_vga_board, addr + 2, val >> 16);
	vga_ram_put(x86_vga_board, addr + 3, val >> 24);
}

static uint8_t vgalfb_readb(uint32_t addr, void *priv)
{
	return vgalfb_ram_get(x86_vga_board, addr);
}
static uint16_t vgalfb_readw(uint32_t addr, void *priv)
{
	return (vgalfb_ram_get(x86_vga_board, addr) << 0) | (vgalfb_ram_get(x86_vga_board, addr + 1) << 8);
}
static uint32_t vgalfb_readl(uint32_t addr, void *priv)
{
	return (vgalfb_readw(addr, priv) << 0) | (vgalfb_readw(addr + 2, priv) << 16);
}
static void vgalfb_writeb(uint32_t addr, uint8_t val, void *priv)
{
	vgalfb_ram_put(x86_vga_board, addr, val);
}
static void vgalfb_writew(uint32_t addr, uint16_t val, void *priv)
{
	vgalfb_ram_put(x86_vga_board, addr, (uae_u8)val);
	vgalfb_ram_put(x86_vga_board, addr + 1, val >> 8);
}
static void vgalfb_writel(uint32_t addr, uint32_t val, void *priv)
{
	vgalfb_ram_put(x86_vga_board, addr, val);
	vgalfb_ram_put(x86_vga_board, addr + 1, val >> 8);
	vgalfb_ram_put(x86_vga_board, addr + 2, val >> 16);
	vgalfb_ram_put(x86_vga_board, addr + 3, val >> 24);
}

static int to53c400reg(uint32_t addr, bool rw)
{
	//write_log(_T("%c %08x\n"), rw ? 'W' : 'R', addr);
	// Scratchpad RAM
	if (addr >= 0x3800 && addr <= 0x387f)
		return (addr & 0x3f) | 0x200;
	// 53C80 registers
	if (addr >= 0x3880 && addr <= 0x38ff)
		return addr & 7;
	// Data port
	if (addr >= 0x3900 && addr <= 0x397f)
		return 0x80;
	// 53C400 registers
	if (addr >= 0x3980 && addr <= 0x39ff) {
		int reg = addr & 3;
		if (reg >= 2)
			return -1;
		return reg | 0x100;
	}
	return -1;
}

static uint8_t mem_read_romext3(uint32_t addr, void *priv)
{
	uae_u8 v = 0xff;
	addr &= 0x3fff;
	if (addr < 0x2000) {
		v = rt1000rom[addr];
	} else if (addr >= 0x3a00) {
		write_log(_T("mem_read_romext3 %08x\n"), addr);
	} else if (addr >= 0x3800) {
		int reg = to53c400reg(addr, false);
		if (reg >= 0)
			v = x86_rt1000_bget(reg);
	}
	return v;
}
static uint16_t mem_read_romextw3(uint32_t addr, void *priv)
{
	uae_u16 v = 0xffff;
	addr &= 0x3fff;
	if (addr < 0x2000) {
		v =  *(uint16_t *)&rt1000rom[addr & 0x1fff];
	} else if (addr >= 0x3a00) {
		write_log(_T("mem_read_romext3 %08x\n"), addr);
	} else if (addr >= 0x3800) {
		int reg = to53c400reg(addr + 0, false);
		v = 0;
		if (reg >= 0)
			v = x86_rt1000_bget(reg);
		reg = to53c400reg(addr + 1, false);
		if (reg >= 0)
			v |= x86_rt1000_bget(reg) << 8;
	}
	return v;
}
static uint32_t mem_read_romextl3(uint32_t addr, void *priv)
{
	return *(uint32_t *)&rt1000rom[addr & 0x3fff];
}
static void mem_write_romext3(uint32_t addr, uint8_t val, void *priv)
{
	addr &= 0x3fff;
	if (addr >= 0x3a00) {
		write_log(_T("mem_write_romext3 %08x %02x\n"), addr, val);
	} else if (addr >= 0x3800) {
		int reg = to53c400reg(addr, true);
		if (reg >= 0)
			x86_rt1000_bput(reg, val);
	}
}
static void mem_write_romextw3(uint32_t addr, uint16_t val, void *priv)
{
	addr &= 0x3fff;
	if (addr >= 0x3a00) {
		write_log(_T("mem_write_romextw3 %08x %04x\n"), addr, val);
	} else if (addr >= 0x3800) {
		int reg = to53c400reg(addr + 0, true);
		if (reg >= 0)
			x86_rt1000_bput(reg, (uae_u8)val);
		reg = to53c400reg(addr + 1, true);
		if (reg >= 0)
			x86_rt1000_bput(reg, val >> 8);
	} else {
		write_log(_T("mem_write_romextw3 %08x %04x\n"), addr, val);
	}
}
static void mem_write_romextl3(uint32_t addr, uint32_t val, void *priv)
{

}

static uint8_t mem_read_romext2(uint32_t addr, void *priv)
{
	return xtiderom[addr & 0x3fff];
}
static uint16_t mem_read_romextw2(uint32_t addr, void *priv)
{
	return *(uint16_t *)&xtiderom[addr & 0x3fff];
}
static uint32_t mem_read_romextl2(uint32_t addr, void *priv)
{
	return *(uint32_t *)&xtiderom[addr & 0x3fff];
}

static void x86_bridge_rethink(void)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb)
		return;
	if (!(xb->amiga_io[IO_CONTROL_REGISTER] & 1)) {
		xb->amiga_io[IO_AMIGA_INTERRUPT_STATUS] |= xb->delayed_interrupt;
		xb->delayed_interrupt = 0;
		uae_u8 intreq = xb->amiga_io[IO_AMIGA_INTERRUPT_STATUS];
		uae_u8 intena = xb->amiga_io[IO_INTERRUPT_MASK];
		uae_u8 status = intreq & ~intena;
		if (status)
			safe_interrupt_set(IRQ_SOURCE_X86, 0, false);
	}
}

static void x86_bridge_reset(int hardreset)
{
	for (int i = 0; i < X86_BRIDGE_MAX; i++) {
		struct x86_bridge *xb = bridges[i];
		if (!xb)
			continue;
		if (xb->ne2000_isa) {
			xb->ne2000_isa->free(xb->ne2000_isa_board_state);
			xb->ne2000_isa = NULL;
			xfree(xb->ne2000_isa_board_state);
			xb->ne2000_isa_board_state = NULL;
		}
		if (xb->cmosfile) {
			zfile_fseek(xb->cmosfile, 0, SEEK_SET);
			zfile_fwrite(nvrram, 1, xb->cmossize, xb->cmosfile);
		}
		if (xb->audstream) {
			audio_enable_stream(false, xb->audstream, 0, NULL, NULL);
		}
		zfile_fclose(xb->cmosfile);
		xfree(xb->amiga_io);
		xfree(xb->io_ports);
		xfree(xb->pc_rom);
		rom = NULL;
		xfree(xtiderom);
		xtiderom = NULL;
		xfree(rt1000rom);
		rt1000rom = NULL;
		xfree(xb->mouse_base);
		xfree(xb->cms_base);
		xfree(xb->sb_base);
		mem_free();
		bridges[i] = NULL;
		memset(port_inb, 0, sizeof(port_inb));
		memset(port_inw, 0, sizeof(port_inw));
		memset(port_inl, 0, sizeof(port_inl));
		memset(port_outb, 0, sizeof(port_outb));
		memset(port_outw, 0, sizeof(port_outw));
		memset(port_outl, 0, sizeof(port_outl));
	}
}

static void x86_bridge_free(void)
{
	x86_bridge_reset(1);
	x86_found = 0;
}

static void check_floppy_delay(void)
{
	for (int i = 0; i < 4; i++) {
		if (floppy_seeking[i]) {
			bool neg = floppy_seeking[i] < 0;
			if (floppy_seeking[i] > 0)
				floppy_seeking[i]--;
			else if (neg)
				floppy_seeking[i]++;
			if (floppy_seeking[i] == 0)
				do_floppy_seek(i, neg);
		}
	}
	if (floppy_delay_hsync > 1 || floppy_delay_hsync < -1) {
		if (floppy_delay_hsync > 0)
			floppy_delay_hsync--;
		else
			floppy_delay_hsync++;
		if (floppy_delay_hsync == 1 || floppy_delay_hsync == -1)
			do_floppy_irq();
	}
}

static void x86_cpu_execute(int cycs)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb->x86_reset) {

		// no free buffers to fill
		if (x86_sndbuffer_filled[x86_sndbuffer_index])
			return;

		if (AT)
			exec386(cycs);
		else
			execx86(cycs);
	}

	// BIOS has CPU loop delays in floppy driver...
	check_floppy_delay();
}

static bool audio_state_sndboard_x86(int streamid, void *param)
{
	static int smp[2] = { 0, 0 };
	struct x86_bridge *xb = bridges[0];

	if (streamid != xb->audstream)
		return false;

	if (x86_sndbuffer_filled[x86_sndbuffer_playindex]) {
		int32_t *p = x86_sndbuffer[x86_sndbuffer_playindex];
		int32_t l = p[x86_sndbuffer_playpos * 2 + 0];
		int32_t r = p[x86_sndbuffer_playpos * 2 + 1];
		if (l < -32768)
			l = -32768;
		if (l > 32767)
			l = 32767;
		if (r < -32768)
			r = -32768;
		if (r > 32767)
			r = 32767;
		x86_sndbuffer_playpos++;
		if (x86_sndbuffer_playpos == SOUNDBUFLEN) {
			x86_sndbuffer_filled[x86_sndbuffer_playindex] = false;
			x86_sndbuffer_playindex ^= 1;
			x86_sndbuffer_playpos = 0;
		}
		smp[0] = l;
		smp[1] = r;
	}
	audio_state_stream_state(xb->audstream, smp, 2, xb->audeventtime);
	return true;
}

void x86_bridge_sync_change(void)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb)
		return;
}

static void x86_bridge_vsync(void)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb)
		return;

	if (xb->delayed_interrupt) {
		devices_rethink_all(x86_bridge_rethink);
	}
	struct romconfig *rc = get_device_romconfig(&changed_prefs, ROMTYPE_X86MOUSE, 0);
	if (rc) {
		xb->mouse_port = (rc->device_settings & 3) + 1;
	}
	xb->audeventtime = (int)(x86_base_event_clock * CYCLE_UNIT / currprefs.sound_freq + 1);
}

static void x86_bridge_hsync(void)
{
	static float totalcycles;
	struct x86_bridge *xb = bridges[0];
	if (!xb)
		return;

	if (!xb->sound_initialized) {
		// x86_base_event_clock is not initialized until syncs start
		xb->sound_initialized = true;
		if (xb->sound_emu) {
			write_log(_T("x86 sound init\n"));
			xb->audeventtime = (int)(x86_base_event_clock * CYCLE_UNIT / currprefs.sound_freq + 1);
			timer_add(&sound_poll_timer, sound_poll, NULL, 0);
			xb->audstream = audio_enable_stream(true, -1, 2, audio_state_sndboard_x86, NULL);
			sound_speed_changed(true);
		}
	}

	check_floppy_delay();

	if (xb->ne2000_isa)
		xb->ne2000_isa->hsync(xb->ne2000_isa_board_state);

	if (!xb->x86_reset) {
		float cycles_to_run = (float)cpu_get_speed() / (vblank_hz * maxvpos);
		totalcycles += cycles_to_run;
		int cycs = (int)totalcycles;
		x86_cpu_execute(cycs);
		totalcycles -= (int)totalcycles;
	}

	if (currprefs.x86_speed_throttle != changed_prefs.x86_speed_throttle) {
		currprefs.x86_speed_throttle = changed_prefs.x86_speed_throttle;
		set_cpu_turbo(xb);
	}
}

static void ew(uae_u8 *acmemory, int addr, uae_u8 value)
{
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		acmemory[addr] = (value & 0xf0);
		acmemory[addr + 2] = (value & 0x0f) << 4;
	} else {
		acmemory[addr] = ~(value & 0xf0);
		acmemory[addr + 2] = ~((value & 0x0f) << 4);
	}
}

static void bridge_reset(struct x86_bridge *xb)
{
	xb->x86_reset = true;
	xb->configured = 0;
	xb->amiga_forced_interrupts = false;
	xb->amiga_irq = false;
	xb->pc_irq3a = xb->pc_irq3b = xb->pc_irq7 = false;
	xb->mode_register = -1;
	xb->video_initialized = false;
	xb->a2386flipper = 0;
	xb->a2386_amigapcdrive = false;
	x86_cpu_active = false;
	memset(xb->amiga_io, 0, 0x50000);
	memset(xb->io_ports, 0, 0x10000);
	memset(xb->pc_ram, 0, xb->pc_maxbaseram);

	if (xb->ne2000_isa)
		xb->ne2000_isa->reset(xb->ne2000_isa_board_state);

	xb->amiga_io[IO_CONTROL_REGISTER] =	0xfe;
	xb->amiga_io[IO_PC_INTERRUPT_CONTROL] = 0xff;
	xb->amiga_io[IO_INTERRUPT_MASK] = 0xff;
	xb->amiga_io[IO_MODE_REGISTER] = 0x00;
	xb->amiga_io[IO_PC_INTERRUPT_STATUS] = 0xfe;
	memset(xb->vlsi_regs, 0, sizeof xb->vlsi_regs);

	int sel1 = (xb->settings >> 10) & 1;
	int sel2 = (xb->settings >> 11) & 1;
	xb->amiga_io[IO_MODE_REGISTER] |= sel1 << 5;
	xb->amiga_io[IO_MODE_REGISTER] |= sel2 << 6;

	x86_bridge_sync_change();
	floppy_hardreset();
	pcem_close();
}

int is_x86_cpu(struct uae_prefs *p)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb) {
		if (x86_found > 0)
			return X86_STATE_STOP;
		else if (x86_found < 0)
			return X86_STATE_INACTIVE;
		if (is_device_rom(p, ROMTYPE_A1060, 0) < 0 &&
			is_device_rom(p, ROMTYPE_A2088, 0) < 0 &&
			is_device_rom(p, ROMTYPE_A2088T, 0) < 0 &&
			is_device_rom(p, ROMTYPE_A2286, 0) < 0 &&
			is_device_rom(p, ROMTYPE_A2386, 0) < 0) {
			if (p == &currprefs)
				x86_found = -1;
			return X86_STATE_INACTIVE;
		} else {
			if (p == &currprefs)
				x86_found = 1;
		}
	}
	if (!xb || xb->x86_reset)
		return X86_STATE_STOP;
	if (xb && xb->type < 0)
		return X86_STATE_INACTIVE;
	return X86_STATE_ACTIVE;
}

static void ne2000_isa_irq_callback(struct pci_board_state *pcibs, bool irq)
{
	struct x86_bridge *xb = bridges[0];
	if (irq)
		x86_doirq(xb->ne2000_irq);
	else
		x86_clearirq(xb->ne2000_irq);
}

void x86_rt1000_bios(struct zfile *z, struct romconfig *rc)
{
	struct x86_bridge *xb = bridges[0];
	uae_u32 addr = 0;
	if (!xb || !z)
		return;
	addr = (rc->device_settings & 7) * 0x4000 + 0xc8000;
	rt1000rom = xcalloc(uae_u8, 0x2000 + 1);
	zfile_fread(rt1000rom, 1, 0x2000, z);
	mem_mapping_add(&bios_mapping[6], addr, 0x4000, mem_read_romext3, mem_read_romextw3, mem_read_romextl3, mem_write_romext3, mem_write_romextw3, mem_write_romextl3, rt1000rom, MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
}

void x86_xt_ide_bios(struct zfile *z, struct romconfig *rc)
{
	struct x86_bridge *xb = bridges[0];
	uae_u32 addr = 0;
	if (!xb || !z)
		return;
	switch (rc->device_settings)
	{
		case 0:
		addr = 0xcc000;
		break;
		case 1:
		addr = 0xdc000;
		break;
		case 2:
		default:
		addr = 0xec000;
		break;
	}
	xtiderom = xcalloc(uae_u8, 0x4000);
	zfile_fread(xtiderom, 1, 0x4000, z);
	mem_mapping_add(&bios_mapping[5], addr, 0x4000, mem_read_romext2, mem_read_romextw2, mem_read_romextl2, mem_write_null, mem_write_nullw, mem_write_nulll, xtiderom, MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
}

void *sb_1_init();
void *sb_15_init();
void *sb_2_init();
void *sb_pro_v1_init();
void *sb_pro_v2_init();
void *sb_16_init();
void *cms_init(const device_t*);

static int x86_global_settings;

int device_get_config_int(const char *s)
{
	if (!strcmp(s, "bilinear")) {
		return 1;
	}
	if (!strcmp(s, "dithering")) {
		return 1;
	}
	if (!strcmp(s, "dacfilter")) {
		return 1;
	}
	if (!strcmp(s, "recompiler")) {
		return 1;
	}
	if (!strcmp(s, "memory")) {
		return pcem_getvramsize() >> 20;
	}
	if (!strcmp(s, "render_threads")) {
#ifdef _WIN32
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		if (si.dwNumberOfProcessors >= 8)
			return 4;
		if (si.dwNumberOfProcessors >= 4)
			return 2;
		return 1;
#else
		return 4;
#endif
	}


	if (x86_global_settings < 0)
		return 0;
	if (!strcmp(s, "addr")) {
		int io = 0x210 + ((x86_global_settings >> 3) & 7) * 0x10;
		return io;
	} else if (!strcmp(s, "irq")) {
		int irq = (x86_global_settings >> 6) & 7;
		if (irq == 0)
			return 2;
		if (irq == 1)
			return 3;
		if (irq == 2)
			return 5;
		if (irq == 3)
			return 7;
		if (irq == 4)
			return 10;
		return 5;
	} else if (!strcmp(s, "dma")) {
		int irq = (x86_global_settings >> 9) & 1;
		return irq ? 3 : 1;
	} else if (!strcmp(s, "opl_emu")) {
		int opl = (x86_global_settings >> 10) & 1;
		return opl ? 1 : 0;
	}
	return 0;
}

static void set_sb_emu(struct x86_bridge *xb)
{
	if (!is_board_enabled(&currprefs, ROMTYPE_SBISA, 0))
		return;
	void *p = NULL, *c = NULL;
	struct romconfig *rc = get_device_romconfig(&currprefs, ROMTYPE_SBISA, 0);
	x86_global_settings = rc->device_settings;
	int model = (x86_global_settings >> 0) & 7;
	switch (model)
	{
	case 0:
		c = cms_init(NULL);
		p = sb_1_init();
		break;
	case 1:
		c = cms_init(NULL);
		p = sb_15_init();
		break;
	case 2:
		c = cms_init(NULL);
		p = sb_2_init();
		break;
	case 3:
		p = sb_pro_v1_init();
		break;
	case 4:
		p = sb_pro_v2_init();
		break;
	case 5:
		p = sb_16_init();
		break;
	}
	x86_global_settings = -1;
	xb->sb_base = p;
	xb->cms_base = c;
	xb->sound_emu = true;
}

static void x86_ne2000(struct x86_bridge *xb)
{
	if (!is_board_enabled(&currprefs, ROMTYPE_NE2KISA, 0))
		return;
	struct romconfig *rc = get_device_romconfig(&currprefs, ROMTYPE_NE2KISA, 0);
	if (rc) {
		xb->ne2000_isa = &ne2000_pci_board_x86;
		xb->ne2000_isa_board_state = xcalloc(pci_board_state, 1);
		xb->ne2000_isa_board_state->irq_callback = ne2000_isa_irq_callback;
		switch (rc->device_settings & 7)
		{
		case 0:
			xb->ne2000_io = 0x240;
			break;
		case 1:
			xb->ne2000_io = 0x260;
			break;
		case 2:
			xb->ne2000_io = 0x280;
			break;
		case 3:
			xb->ne2000_io = 0x2a0;
			break;
		case 4:
		default:
			xb->ne2000_io = 0x300;
			break;
		case 5:
			xb->ne2000_io = 0x320;
			break;
		case 6:
			xb->ne2000_io = 0x340;
			break;
		case 7:
			xb->ne2000_io = 0x360;
			break;
		}
		switch ((rc->device_settings >> 3) & 15)
		{
		case 0:
			xb->ne2000_irq = 3;
			break;
		case 1:
			xb->ne2000_irq = 4;
			break;
		case 2:
			xb->ne2000_irq = 5;
			break;
		case 3:
		default:
			xb->ne2000_irq = 7;
			break;
		case 4:
			xb->ne2000_irq = 9;
			break;
		case 5:
			xb->ne2000_irq = 10;
			break;
		case 6:
			xb->ne2000_irq = 11;
			break;
		case 7:
			xb->ne2000_irq = 12;
			break;
		case 8:
			xb->ne2000_irq = 15;
			break;
		}
		struct romconfig *rc = get_device_romconfig(&currprefs, ROMTYPE_NE2KISA, 0);
		struct autoconfig_info aci = { 0 };
		aci.rc = rc;
		if (xb->ne2000_isa->init(xb->ne2000_isa_board_state, &aci)) {
			write_log(_T("NE2000 ISA configured, IO=%3X, IRQ=%d\n"), xb->ne2000_io, xb->ne2000_irq);
		} else {
			xb->ne2000_isa = NULL;
			xfree(xb->ne2000_isa_board_state);
			xb->ne2000_isa_board_state = NULL;
		}
	}
}

static void load_vga_bios(void)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb || !ISVGA())
		return;
	struct zfile *zf = read_device_rom(&currprefs, ROMTYPE_x86_VGA, 0, NULL);
	if (zf) {
		int size = (int)zfile_fread(romext, 1, 32768, zf);
		write_log(_T("X86 VGA BIOS '%s' loaded, %08x %d bytes\n"), zfile_getname(zf), 0xc0000, size);
		zfile_fclose(zf);
		mem_mapping_add(&bios_mapping[4], 0xc0000, 0x8000, mem_read_romext, mem_read_romextw, mem_read_romextl, mem_write_null, mem_write_nullw, mem_write_nulll, romext, MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
	}
}

static void set_vga(struct x86_bridge *xb)
{
	// load vga bios
	xb->vgaboard = -1;
	for (int i = 0; i < MAX_RTG_BOARDS; i++) {
		if (currprefs.rtgboards[i].rtgmem_type == GFXBOARD_ID_VGA) {
			xb->vgaboard = i;
			x86_vga_board = i;
			xb->vgaboard_vram = currprefs.rtgboards[i].rtgmem_size;
			load_vga_bios();
			mem_mapping_add(&xb->vgamem, 0xa0000, 0x20000, vga_readb, vga_readw, vga_readl, vga_writeb, vga_writew, vga_writel, NULL, MEM_MAPPING_EXTERNAL, NULL);
			mem_mapping_add(&xb->vgalfbmem, 0x100000, 0x00200000, vgalfb_readb, vgalfb_readw, vgalfb_readl, vgalfb_writeb, vgalfb_writew, vgalfb_writel, NULL, MEM_MAPPING_EXTERNAL, NULL);
			mem_mapping_disable(&xb->vgalfbmem);
			break;
		}
	}
}

void mouse_serial_poll(int x, int y, int z, int b, void *p);
void mouse_ps2_poll(int x, int y, int z, int b, void *p);

bool x86_mouse(int port, int x, int y, int z, int b)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb || !xb->mouse_port || !xb->mouse_base || xb->mouse_port != port + 1)
		return false;
	switch (xb->mouse_type)
	{
		case 0:
		default:
		if (b < 0)
			return true;
		mouse_serial_poll(x, y, z, b, xb->mouse_base);
		return true;
		case 1:
		case 2:
		if (b < 0)
			return true;
		mouse_ps2_poll(x, y, z, b, xb->mouse_base);
		return true;
	}
	return false;
}

void *mouse_serial_init();
void *mouse_ps2_init();
void *mouse_intellimouse_init();

static void set_mouse(struct x86_bridge *xb)
{
	ps2_mouse_supported = false;
	if (!is_board_enabled(&currprefs, ROMTYPE_X86MOUSE, 0))
		return;
	struct romconfig *rc = get_device_romconfig(&currprefs, ROMTYPE_X86MOUSE, 0);
	if (rc) {
		xb->mouse_port = (rc->device_settings & 3) + 1;
		xb->mouse_type = (rc->device_settings >> 2) & 3;
		switch (xb->mouse_type)
		{
			case 0:
			default:
			serial1_init(0x3f8, 4, 1);
			serial_reset();
			xb->mouse_base = mouse_serial_init();
			break;
			case 1:
			ps2_mouse_supported = true;
			xb->mouse_base = mouse_ps2_init();
			break;
			case 2:
			ps2_mouse_supported = true;
			xb->mouse_base = mouse_intellimouse_init();
			break;
		}
	}
}

static const uae_u8 a1060_autoconfig[16] = { 0xc4, 0x01, 0x80, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uae_u8 a2386_autoconfig[16] = { 0xc4, 0x67, 0x80, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

bool x86_bridge_init(struct autoconfig_info *aci, uae_u32 romtype, int type)
{
	const uae_u8 *ac;
	struct romconfig *rc = aci->rc;

	device_add_reset(x86_bridge_reset);
	if (type >= TYPE_2286) {
		ac = type >= TYPE_2386 ? a2386_autoconfig : a1060_autoconfig;
	}
	else {
		ac = a1060_autoconfig;
	}

	for (int i = 0; i < 16; i++) {
		ew(aci->autoconfig_raw, i * 4, ac[i]);
	}
	if (!aci->doinit)
		return true;

	struct x86_bridge *xb = x86_bridge_alloc();
	if (!xb) {
		aci->addrbank = &expamem_null;
		return true;
	}
	memcpy(xb->acmemory, aci->autoconfig_raw, sizeof aci->autoconfig_raw);
	bridges[0] = xb;
	xb->rc = rc;
	x86_global_settings = -1;

	xb->type = type;

	xb->io_ports = xcalloc(uae_u8, 0x10000);
	xb->amiga_io = xcalloc(uae_u8, 0x50000);
	xb->amiga_buffer = xb->amiga_io + 0x10000;
	xb->amiga_parameter = xb->amiga_io + 0x20000;
	xb->amiga_mono = xb->amiga_io + 0x30000;
	xb->amiga_color = xb->amiga_io + 0x40000;

	xb->settings = rc->device_settings;
	if (xb->type >= TYPE_2286) {
		xb->settings |= 0xff;
		xb->bios_size = 65536;
	} else {
		xb->bios_size = 32768;
	}

	// PC Spekaer?
	xb->pc_speaker = true;
	xb->sound_emu = (xb->settings >> 13) & 1;

	xb->pc_rom = xcalloc(uae_u8, 0x20000);
	memset(xb->pc_rom, 0xff, 0x20000);

	x86_ne2000(xb);

	xb->pc_jumpers = (xb->settings & 0xff) ^ ((0x80 | 0x40) | (0x20 | 0x10 | 0x01 | 0x02));

	int ramsize = (xb->settings >> 2) & 3;
	switch(ramsize) {
		case 0:
		xb->pc_maxbaseram = 128 * 1024;
		break;
		case 1:
		xb->pc_maxbaseram = 256 * 1024;
		break;
		case 2:
		xb->pc_maxbaseram = 512 * 1024;
		break;
		case 3:
		xb->pc_maxbaseram = 640 * 1024;
		break;
	}

	romset = ROM_GENXT;
	enable_sync = 1;
	fpu_type = 0;

	switch (xb->type)
	{
	case TYPE_SIDECAR:
		model = 0;
		cpu_manufacturer = 0;
		cpu = 0; // 4.77MHz
		fpu_type = (xb->settings  & (1 << 19)) ? FPU_8087 : FPU_NONE;
		break;
	case TYPE_2088:
		model = 0;
		cpu_manufacturer = 0;
		cpu = 0; // 4.77MHz
		fpu_type = (xb->settings & (1 << 19)) ? FPU_8087 : FPU_NONE;
		break;
	case TYPE_2088T:
		model = 0;
		cpu_manufacturer = 0;
		cpu = 0; // 4.77MHz
		fpu_type = (xb->settings & (1 << 19)) ? FPU_8087 : FPU_NONE;
		break;
	case TYPE_2286:
		model = 1;
		cpu_manufacturer = 0;
		cpu = 1; // 8MHz
		fpu_type = (xb->settings & (1 << 19)) ? FPU_287 : FPU_NONE;
		break;
	case TYPE_2386:
		model = 2;
		cpu_manufacturer = 0;
		cpu = 2; // 25MHz
		fpu_type = (xb->settings & (1 << 19)) ? FPU_387 : FPU_NONE;
		break;
	}
	if (rc->configtext[0]) {
		MODEL *m = &models[model];
		int mannum = 0;
		while (m->cpu[mannum].cpus) {
			CPU *cpup = m->cpu[mannum].cpus;
			int cpunum = 0;
			while (cpup[cpunum].cpu_type >= 0) {
				TCHAR *cpuname = au(cpup[cpunum].name);
				if (_tcsstr(rc->configtext, cpuname)) {
					int cputype = cpup[cpunum].cpu_type;
					cpu_manufacturer = mannum;
					cpu = cpunum;
					if (cputype == CPU_i486SX) {
						fpu_type = FPU_NONE;
					} else if (cputype == CPU_i486DX) {
						fpu_type = FPU_BUILTIN;
					} else if ((cputype == CPU_386DX || cputype == CPU_386SX) && fpu_type != FPU_NONE) {
						fpu_type = FPU_387;
					} else if (cputype == CPU_286 && fpu_type != FPU_NONE) {
						fpu_type = FPU_287;
					} else if (cputype == CPU_8088 && fpu_type != FPU_NONE) {
						fpu_type = FPU_8087;
					}
					write_log(_T("CPU override = %s\n"), cpuname);
				}
				xfree(cpuname);
				cpunum++;
			}
			mannum++;
		}
	}

	cpu_multiplier = (int)currprefs.x86_speed_throttle;
	cpu_set();
	if (xb->type >= TYPE_2286) {
		mem_size = (1024 * 1024) << ((xb->settings >> 16) & 7);
	} else {
		mem_size = xb->pc_maxbaseram;
	}
	mem_size /= 1024;
	mem_init();
	mem_alloc();
	xb->pc_ram = ram;
	if (xb->type < TYPE_2286) {
		mem_set_704kb();
	}

	bridge_reset(xb);

	timer_reset();

	// SB setup must come before DMA init
	sound_reset();
	speaker_init();
	set_sb_emu(xb);
	sound_pos_global = xb->sound_emu ? 0 : -1;

	dma_init();
	pit_init();
	pic_init();

	if (xb->type >= TYPE_2286) {
		AT = 1;
		nvr_device.init(&nvr_device);
		TCHAR path[MAX_DPATH];
		cfgfile_resolve_path_out_load(currprefs.flashfile, path, MAX_DPATH, PATH_ROM);
		xb->cmossize = xb->type == TYPE_2386 ? 192 : 64;
		xb->cmosfile = zfile_fopen(path, _T("rb+"), ZFD_NORMAL);
		if (!xb->cmosfile) {
			xb->cmosfile = zfile_fopen(path, _T("wb"));
		}
		memset(nvrram, 0, sizeof nvrram);
		if (xb->cmosfile) {
			zfile_fread(nvrram, 1, xb->cmossize, xb->cmosfile);
		}
		loadnvr();
		nvrmask = 127;
		pit_set_out_func(&pit, 1, pit_refresh_timer_at);
		keyboard_at_init();
		keyboard_at_init_ps2();
		dma16_init();
		pic2_init();
	} else {
		AT = 0;
		pit_set_out_func(&pit, 1, pit_refresh_timer_xt);
	}
	pic_reset();
	dma_reset();
	cpu_set_turbo(0);
	cpu_set_turbo(1);
	codegen_init();

	// load bios
	if (!load_rom_rc(rc, romtype, xb->bios_size, 0, xb->pc_rom, xb->bios_size, LOADROM_FILL)) {
		error_log(_T("Bridgeboard BIOS failed to load"));
		x86_bridge_free();
		aci->addrbank = &expamem_null;
		return false;
	}
	rom = xb->pc_rom;
	biosmask = xb->bios_size - 1;
	if (xb->type < TYPE_2286) {
		mem_mapping_add(&bios_mapping[0], 0xf8000, 0x04000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x8000 & biosmask), MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_mapping[1], 0xfc000, 0x04000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0xc000 & biosmask), MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
	} else {
		mem_mapping_add(&bios_mapping[0], 0xe0000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom, MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_mapping[1], 0xe8000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x8000 & biosmask), MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_mapping[2], 0xf0000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x10000 & biosmask), MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_mapping[3], 0xf8000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x18000 & biosmask), MEM_MAPPING_EXTERNAL | MEM_MAPPING_ROM, 0);
		
		mem_mapping_add(&bios_high_mapping[0], (AT && cpu_16bitbus) ? 0xfe0000 : 0xfffe0000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom, MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_high_mapping[1], (AT && cpu_16bitbus) ? 0xfe8000 : 0xfffe8000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x8000 & biosmask), MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_high_mapping[2], (AT && cpu_16bitbus) ? 0xff0000 : 0xffff0000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x10000 & biosmask), MEM_MAPPING_ROM, 0);
		mem_mapping_add(&bios_high_mapping[3], (AT && cpu_16bitbus) ? 0xff8000 : 0xffff8000, 0x08000, mem_read_bios, mem_read_biosw, mem_read_biosl, mem_write_null, mem_write_nullw, mem_write_nulll, rom + (0x18000 & biosmask), MEM_MAPPING_ROM, 0);
	}

	set_vga(xb);

	// buffer ram
	xb->sharedio_buffer_mb.base = xb->amiga_buffer;
	xb->sharedio_buffer_mb.mask = 0xffff;
	mem_mapping_add(&xb->sharedio_buffer[0], 0xa0000, 0x10000,
		mem_read_sharedb, mem_read_sharedw, mem_read_sharedl,
		mem_write_sharedb, mem_write_sharedw, mem_write_sharedl,
		xb->amiga_buffer, 0, &xb->sharedio_buffer_mb);
	mem_mapping_add(&xb->sharedio_buffer[1], 0xd0000, 0x10000,
		mem_read_sharedb, mem_read_sharedw, mem_read_sharedl,
		mem_write_sharedb, mem_write_sharedw, mem_write_sharedl,
		xb->amiga_buffer, 0, &xb->sharedio_buffer_mb);
	mem_mapping_add(&xb->sharedio_buffer[2], 0xe0000, 0x10000,
		mem_read_sharedb, mem_read_sharedw, mem_read_sharedl,
		mem_write_sharedb, mem_write_sharedw, mem_write_sharedl,
		xb->amiga_buffer, 0, &xb->sharedio_buffer_mb);

	// mono ram
	xb->sharedio_mono_mb.base = xb->amiga_mono;
	xb->sharedio_mono_mb.mask = 0x1fff;
	mem_mapping_add(&xb->sharedio_mono, 0xb0000, 0x2000,
		mem_read_sharedb, mem_read_sharedw, mem_read_sharedl,
		mem_write_sharedb, mem_write_sharedw, mem_write_sharedl,
		xb->amiga_mono, 0, &xb->sharedio_mono_mb);

	// color ram
	xb->sharedio_color_mb.base = xb->amiga_color;
	xb->sharedio_color_mb.mask = 0x7fff;
	mem_mapping_add(&xb->sharedio_color, 0xb8000, 0x8000,
		mem_read_sharedb, mem_read_sharedw, mem_read_sharedl,
		mem_write_sharedb, mem_write_sharedw, mem_write_sharedl,
		xb->amiga_color, 0, &xb->sharedio_color_mb);

	// parameter ram
	if (xb->type < TYPE_2286) {
		xb->sharedio_parameter_mb.base = xb->amiga_parameter;
		xb->sharedio_parameter_mb.mask = 0x3fff;
		mem_mapping_add(&xb->sharedio_parameter, 0xf0000, 0x4000,
			mem_read_sharedb, mem_read_sharedw, mem_read_sharedl,
			mem_write_sharedb, mem_write_sharedw, mem_write_sharedl,
			xb->amiga_parameter, 0, &xb->sharedio_parameter_mb);
	}

	set_configurable_shared_io(xb);

	if (xb->type == TYPE_2386) {
		vlsi_init_mapping(xb);
	}

	set_mouse(xb);

	xb->bank = &x86_bridge_bank;

	aci->addrbank = xb->bank;

	device_add_hsync(x86_bridge_hsync);
	device_add_vsync_pre(x86_bridge_vsync);
	device_add_exit(x86_bridge_free, NULL);
	device_add_rethink(x86_bridge_rethink);

	return true;
}

static void reset_x86_hardware(struct x86_bridge *xb)
{
	x86_ide_hd_put(-1, 0, 0);
	x86_rt1000_bput(-1, 0);

#if 0
	if (xb->type >= TYPE_2386) {
		vlsi_reset(xb);
	}
#endif
	if (xb->ne2000_isa) {
		xb->ne2000_isa->reset(xb->ne2000_isa_board_state);
	}

	serial_reset();
}


void x86_map_lfb(int v)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb)
		return;
	uae_u32 addr = v << 20;

	if (addr) {
		mem_mapping_set_addr(&xb->vgalfbmem, addr, xb->vgaboard_vram);
	} else {
		mem_mapping_disable(&xb->vgalfbmem);
	}
}

void io_sethandler(uint16_t base, int size,
	uint8_t(*inb)(uint16_t addr, void *priv),
	uint16_t(*inw)(uint16_t addr, void *priv),
	uint32_t(*inl)(uint16_t addr, void *priv),
	void(*outb)(uint16_t addr, uint8_t  val, void *priv),
	void(*outw)(uint16_t addr, uint16_t val, void *priv),
	void(*outl)(uint16_t addr, uint32_t val, void *priv),
	void *priv)
{
	if (base + size > MAX_IO_PORT)
		return;

	for (int i = 0; i < size; i++) {
		int io = base + i;
		port_inb[io] = inb;
		port_inw[io] = inw;
		port_inl[io] = inl;
		port_outb[io] = outb;
		port_outw[io] = outw;
		port_outl[io] = outl;
		port_priv[io] = priv;
	}
}

void io_removehandler(uint16_t base, int size,
	uint8_t(*inb)(uint16_t addr, void *priv),
	uint16_t(*inw)(uint16_t addr, void *priv),
	uint32_t(*inl)(uint16_t addr, void *priv),
	void(*outb)(uint16_t addr, uint8_t  val, void *priv),
	void(*outw)(uint16_t addr, uint16_t val, void *priv),
	void(*outl)(uint16_t addr, uint32_t val, void *priv),
	void *priv)
{
	if (base + size > MAX_IO_PORT)
		return;

	for (int i = 0; i < size; i++) {
		int io = base + i;
		port_inb[io] = NULL;
		port_inw[io] = NULL;
		port_inl[io] = NULL;
		port_outb[io] = NULL;
		port_outw[io] = NULL;
		port_outl[io] = NULL;
	}
}

static struct
{
	void(*get_buffer)(int32_t *buffer, int len, void *p);
	void *priv;
} sound_handlers[8];

void sound_add_handler(void(*get_buffer)(int32_t *buffer, int len, void *p), void *p)
{
	sound_handlers[sound_handlers_num].get_buffer = get_buffer;
	sound_handlers[sound_handlers_num].priv = p;
	sound_handlers_num++;
}

void sound_speed_changed(bool enable)
{
	if (sound_poll_timer.enabled || enable) {
		sound_poll_latch = (uae_u64)((float)TIMER_USEC * (1000000.0f / currprefs.sound_freq));
		timer_set_delay_u64(&sound_poll_timer, sound_poll_latch);
	}
}

void sound_poll(void *priv)
{
	struct x86_bridge *xb = bridges[0];

	timer_advance_u64(&sound_poll_timer, sound_poll_latch);

	if (x86_sndbuffer_filled[x86_sndbuffer_index]) {
		return;
	}

	sound_pos_global++;
	if (sound_pos_global == SOUNDBUFLEN) {
		int32_t *p = x86_sndbuffer[x86_sndbuffer_index];
		memset(p, 0, SOUNDBUFLEN * sizeof(int32_t) * 2);
		for (int c = 0; c < sound_handlers_num; c++) {
			if (c > 0 || (c == 0 && xb->pc_speaker)) {
				sound_handlers[c].get_buffer(p, SOUNDBUFLEN, sound_handlers[c].priv);
			}
		}
		x86_sndbuffer_filled[x86_sndbuffer_index] = true;
		x86_sndbuffer_index ^= 1;
		sound_pos_global = 0;
	}
}

void sound_reset()
{
	sound_handlers_num = 0;
	sound_pos_global = 0;
	x86_sndbuffer[0] = outbuffer1;
	x86_sndbuffer[1] = outbuffer2;
	x86_sndbuffer_filled[0] = x86_sndbuffer_filled[1] = false;
}

void sound_set_cd_volume(unsigned int l, unsigned int r)
{

}

void x86_update_sound(float clk)
{
	struct x86_bridge *xb = bridges[0];
	x86_base_event_clock = clk;
	if (xb) {
		xb->audeventtime = (int)(x86_base_event_clock * CYCLE_UNIT / currprefs.sound_freq + 1);
		sound_speed_changed(false);
	}
}

bool a1060_init(struct autoconfig_info *aci)
{
	return x86_bridge_init(aci, ROMTYPE_A1060, TYPE_SIDECAR);
}
bool a2088xt_init(struct autoconfig_info *aci)
{
	return x86_bridge_init(aci, ROMTYPE_A2088, TYPE_2088);
}
bool a2088t_init(struct autoconfig_info *aci)
{
	return x86_bridge_init(aci, ROMTYPE_A2088T, TYPE_2088T);
}
bool a2286_init(struct autoconfig_info *aci)
{
	return x86_bridge_init(aci, ROMTYPE_A2286, TYPE_2286);
}
bool a2386_init(struct autoconfig_info *aci)
{
	return x86_bridge_init(aci, ROMTYPE_A2386, TYPE_2386);
}

bool isa_expansion_init(struct autoconfig_info *aci)
{
	static const int parent[] = { ROMTYPE_A1060, ROMTYPE_A2088, ROMTYPE_A2088T, ROMTYPE_A2286, ROMTYPE_A2386, 0 };
	aci->parent_romtype = parent;
	aci->zorro = 0;
	return true;
}

void x86_outfloppy(int portnum, uae_u8 v)
{
	struct x86_bridge *b = bridges[0];
	outfloppy(b, portnum, v);
}
uae_u8 x86_infloppy(int portnum)
{
	struct x86_bridge *b = bridges[0];
	return infloppy(b, portnum);
}
void x86_floppy_run(void)
{
	check_floppy_delay();
}
void x86_initfloppy(X86_INTERRUPT_CALLBACK irq_callback)
{
	struct x86_bridge *xb = bridges[0];
	if (!xb) {
		xb = x86_bridge_alloc();
		bridges[0] = xb;
	}
	xb->type = -1;
	xb->irq_callback = irq_callback;
	floppy_hardreset();
}

#endif //WITH_X86
