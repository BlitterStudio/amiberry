/*
* UAE - The Un*x Amiga Emulator
*
* Other IDE controllers
*
* (c) 2015 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"

#include "memory.h"
#include "newcpu.h"
#include "filesys.h"
#include "debug.h"
#include "ide.h"
#include "idecontrollers.h"
#include "zfile.h"
#include "rommgr.h"
#include "cpuboard.h"
#include "scsi.h"
#include "ncr9x_scsi.h"
#include "autoconf.h"
#include "devices.h"
#include "flashrom.h"


#define DEBUG_IDE 0
#define DEBUG_IDE_MASK 0xf800
#define DEBUG_IDE_MASK_VAL 0x0800

#define DEBUG_IDE_GVP 0
#define DEBUG_IDE_ALF 0
#define DEBUG_IDE_APOLLO 0
#define DEBUG_IDE_MASOBOSHI 0

#define GVP_IDE 0 // GVP A3001
#define ALF_IDE 1
#define ARRIBA_IDE 2
#define APOLLO_IDE (ARRIBA_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define MASOBOSHI_IDE (APOLLO_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define ADIDE_IDE (MASOBOSHI_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define MTEC_IDE (ADIDE_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define PROTAR_IDE (MTEC_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define ROCHARD_IDE (PROTAR_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define x86_AT_IDE (ROCHARD_IDE + 2 * MAX_DUPLICATE_EXPANSION_BOARDS)
#define GOLEMFAST_IDE (x86_AT_IDE + 2 * MAX_DUPLICATE_EXPANSION_BOARDS)
#define BUDDHA_IDE (GOLEMFAST_IDE + 2 * MAX_DUPLICATE_EXPANSION_BOARDS)
#define DATAFLYERPLUS_IDE (BUDDHA_IDE + 3 * MAX_DUPLICATE_EXPANSION_BOARDS)
#define ATEAM_IDE (DATAFLYERPLUS_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define FASTATA4K_IDE (ATEAM_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define ELSATHD_IDE (FASTATA4K_IDE + 2 * MAX_DUPLICATE_EXPANSION_BOARDS)
#define ACCESSX_IDE (ELSATHD_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define IVST500AT_IDE (ACCESSX_IDE + 2 * MAX_DUPLICATE_EXPANSION_BOARDS)
#define TRIFECTA_IDE (IVST500AT_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define TANDEM_IDE (TRIFECTA_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define DOTTO_IDE (TANDEM_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define DEV_IDE (DOTTO_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define RIPPLE_IDE (DEV_IDE + MAX_DUPLICATE_EXPANSION_BOARDS)
#define TOTAL_IDE (RIPPLE_IDE + 2 * MAX_DUPLICATE_EXPANSION_BOARDS)

#define ALF_ROM_OFFSET 0x0100
#define GVP_IDE_ROM_OFFSET 0x8000
#define APOLLO_ROM_OFFSET 0x8000
#define ADIDE_ROM_OFFSET 0x8000
#define MASOBOSHI_ROM_OFFSET 0x0080
#define MASOBOSHI_ROM_OFFSET_END 0xf000
#define MASOBOSHI_SCSI_OFFSET 0xf800
#define MASOBOSHI_SCSI_OFFSET_END 0xfc00

#define TRIFECTA_SCSI_OFFSET 0x0000
#define TRIFECTA_SCSI_OFFSET_END 0x0100

/* masoboshi:

IDE 

- FFCC = base address, data (0)
- FF81 = -004B
- FF41 = -008B
- FF01 = -01CB
- FEC1 = -010B
- FE81 = -014B
- FE41 = -018B select (6)
- FE01 = -01CB status (7)
- FE03 = command (7)

- FA00 = ESP, 2 byte register spacing
- F9CC = data

- F047 = -0F85 (-0FB9) interrupt request? (bit 3)
- F040 = -0F8C interrupt request? (bit 1) Write anything = clear irq?
- F000 = some status register

- F04C = DMA address (long)
- F04A = number of words
- F044 = ???
- F047 = bit 7 = start dma

*/

#define MAX_IDE_UNITS 10

static struct ide_board *gvp_ide_rom_board, *gvp_ide_controller_board;
static struct ide_board *alf_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *apollo_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *masoboshi_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *adide_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *mtec_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *protar_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *rochard_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *x86_at_ide_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *golemfast_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *buddha_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *dataflyerplus_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *ateam_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *arriba_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *fastata4k_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *elsathd_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *accessx_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *ivst500at_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *trifecta_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *tandem_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *dotto_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *dev_board[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ide_board *ripple_board[MAX_DUPLICATE_EXPANSION_BOARDS];

static struct ide_hdf *idecontroller_drive[TOTAL_IDE * 2];
static struct ide_thread_state idecontroller_its;

static struct ide_board *ide_boards[MAX_IDE_UNITS + 1];

static int dev_hd_mode;
static int dev_hd_io_base, dev_hd_io_total;
static int dev_hd_io_size;
static int dev_hd_data_base;
static int dev_hd_io_secondary;

static void freencrunit(struct ide_board *ide)
{
	if (!ide)
		return;
	for (int i = 0; i < MAX_IDE_UNITS; i++) {
		if (ide_boards[i] == ide) {
			ide_boards[i] = NULL;
		}
	}
	for(int i = 0; i < MAX_IDE_PORTS_BOARD; i++) {
		remove_ide_unit(&ide->ide[i], 0);
	}
	if (ide->self_ptr)
		*ide->self_ptr = NULL;
	flash_free(ide->flashrom);
	zfile_fclose(ide->romfile);
	xfree(ide->rom);
	xfree(ide);
}

static struct ide_board *allocide(struct ide_board **idep, struct romconfig *rc, int ch)
{
	struct ide_board *ide;

	if (ch < 0) {
		if (*idep) {
			freencrunit(*idep);
			*idep = NULL;
		}
		ide = xcalloc(struct ide_board, 1);
		for (int i = 0; i < MAX_IDE_UNITS; i++) {
			if (ide_boards[i] == NULL) {
				ide_boards[i] = ide;
				rc->unitdata = ide;
				ide->rc = rc;
				ide->self_ptr = idep;
				if (idep)
					*idep = ide;
				return ide;
			}
		}
	}
	return *idep;
}

static struct ide_board *getideboard(uaecptr addr)
{
	for (int i = 0; ide_boards[i]; i++) {
		if (!ide_boards[i]->baseaddress && !ide_boards[i]->configured)
			return ide_boards[i];
		if ((addr & ~ide_boards[i]->mask) == ide_boards[i]->baseaddress)
			return ide_boards[i];
	}
	return NULL;
}

static void init_ide(struct ide_board *board, int ide_num, int maxunit, bool byteswap, bool adide)
{
	for (int i = 0; i < maxunit / 2; i++) {
		struct ide_hdf **idetable = &idecontroller_drive[(ide_num + i) * 2];
		alloc_ide_mem (idetable, 2, &idecontroller_its);
		board->ide[i] = idetable[0];
		idetable[0]->board = board;
		idetable[1]->board = board;
		idetable[0]->byteswap = byteswap;
		idetable[1]->byteswap = byteswap;
		idetable[0]->adide = adide;
		idetable[1]->adide = adide;
		ide_initialize(idecontroller_drive, ide_num + i);
	}
	idecontroller_its.idetable = idecontroller_drive;
	idecontroller_its.idetotal = TOTAL_IDE * 2;
	start_ide_thread(&idecontroller_its);
}

static void add_ide_standard_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc, struct ide_board **ideboard, int idetype, bool byteswap, bool adide, int maxunit)
{
	struct ide_hdf *ide;
	struct ide_board *ideb;
	int maxch = maxunit / 2;
	int idetypenum = idetype + maxch * ci->controller_type_unit;
	if (ch >= maxunit)
		return;
	ideb = allocide(&ideboard[ci->controller_type_unit], rc, ch);
	if (!ideb)
		return;
	ideb->keepautoconfig = true;
	ideb->type = idetype;
	ide = add_ide_unit (&idecontroller_drive[idetypenum * 2], maxunit, ch, ci, rc);
	init_ide(ideb, idetypenum, maxunit, byteswap, adide);
}

static bool ide_interrupt_check(struct ide_board *board, bool edge_triggered)
{
	if (!board->configured)
		return false;
	bool irq = false;
	for (int i = 0; i < MAX_IDE_PORTS_BOARD; i++) {
		if (board->ide[i] && !irq) {
			irq = ide_irq_check(board->ide[i], edge_triggered);
		}
	}
#if 0
	if (board->irq != irq)
		write_log(_T("IDE irq %d -> %d\n"), board->irq, irq);
#endif
	board->irq = irq;
	return irq;
}

static bool ide_rethink(struct ide_board *board, bool edge_triggered)
{
	bool irq = false;
	if (board->configured) {
		if (board->intena && ide_interrupt_check(board, edge_triggered)) {
			irq = true;
		}
	}
	return irq;
}

void x86_doirq(uint8_t irqnum);

static void idecontroller_rethink(void)
{
	bool irq = false;
	for (int i = 0; ide_boards[i]; i++) {
#ifdef WITH_X86
		if (ide_boards[i] == x86_at_ide_board[0] || ide_boards[i] == x86_at_ide_board[1]) {
			bool x86irq = ide_rethink(ide_boards[i], true);
			if (x86irq && ide_boards[i] == x86_at_ide_board[0]) {
				x86_doirq(14);
			}
		} else
#endif
		{
			if (ide_rethink(ide_boards[i], false))
				safe_interrupt_set(IRQ_SOURCE_IDE, i, ide_boards[i]->intlev6);
		}
	}
}

static void idecontroller_hsync(void)
{
	for (int i = 0; ide_boards[i]; i++) {
		struct ide_board *board = ide_boards[i];
		if (board->hsync_cnt > 0) {
			board->hsync_cnt--;
			if (!board->hsync_cnt && board->hsync_code)
				board->hsync_code(board);
		}
		if (board->configured) {
			for (int j = 0; j < MAX_IDE_PORTS_BOARD; j++) {
				if (board->ide[j]) {
					ide_interrupt_hsync(board->ide[j]);
				}
			}
			if (ide_interrupt_check(board, false)) {
				devices_rethink_all(idecontroller_rethink);
			}
		}
	}
}

static void reset_ide(struct ide_board *board, int hardreset)
{
	board->configured = 0;
	board->intena = false;
	board->enabled = false;
	board->hardreset = hardreset != 0;
}

static void idecontroller_reset(int hardreset)
{
	for (int i = 0; ide_boards[i]; i++) {
		reset_ide(ide_boards[i], hardreset);
	}
}

static void idecontroller_free(void)
{
	stop_ide_thread(&idecontroller_its);
	for (int i = 0; i < MAX_IDE_UNITS; i++) {
		freencrunit(ide_boards[i]);
	}
}

static bool is_gvp2_intreq(uaecptr addr)
{
	if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SII) && (addr & 0x440) == 0x440)
		return true;
	return false;
}
static bool is_gvp1_intreq(uaecptr addr)
{
	if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SI) && (addr & 0x440) == 0x40)
		return true;
	return false;
}

static bool get_ide_is_8bit(struct ide_board *board)
{
	struct ide_hdf *ide = board->ide[0];
	if (!ide)
		return false;
	if (ide->ide_drv)
		ide = ide->pair;
	return ide->mode_8bit;
}

static uae_u32 get_ide_reg_multi(struct ide_board *board, int reg, int portnum, int dataportsize)
{
	struct ide_hdf *ide = board->ide[portnum];
	if (!ide)
		return 0;
	if (ide->ide_drv)
		ide = ide->pair;
	if (reg == 0) {
		if (dataportsize)
			return ide_get_data(ide);
		else
			return ide_get_data_8bit(ide);
	} else {
		return ide_read_reg(ide, reg);
	}
}
static void put_ide_reg_multi(struct ide_board *board, int reg, uae_u32 v, int portnum, int dataportsize)
{
	struct ide_hdf *ide = board->ide[portnum];
	if (!ide)
		return;
	if (ide->ide_drv)
		ide = ide->pair;
	if (reg == 0) {
		if (dataportsize)
			ide_put_data(ide, v);
		else
			ide_put_data_8bit(ide, v);
	} else {
		ide_write_reg(ide, reg, v);
	}
}
static uae_u32 get_ide_reg(struct ide_board *board, int reg)
{
	return get_ide_reg_multi(board, reg, 0, reg == 0 ? 1 : 0);
}
static void put_ide_reg(struct ide_board *board, int reg, uae_u32 v)
{
	put_ide_reg_multi(board, reg, v, 0, reg == 0 ? 1 : 0);
}
static uae_u32 get_ide_reg_8bitdata(struct ide_board *board, int reg)
{
	return get_ide_reg_multi(board, reg, 0, 0);
}
static void put_ide_reg_8bitdata(struct ide_board *board, int reg, uae_u32 v)
{
	put_ide_reg_multi(board, reg, v, 0, 0);
}


static int get_gvp_reg(uaecptr addr, struct ide_board *board)
{
	int reg = -1;

	if (addr & 0x1000) {
		reg = IDE_SECONDARY + ((addr >> 8) & 7);
	} else if (addr & 0x0800) {
		reg = (addr >> 8) & 7;
	}
	if (!(addr & 0x400) && (addr & 0x20)) {
		if (reg < 0)
			reg = 0;
		int extra = (addr >> 1) & 15;
		if (extra >= 8)
			reg |= IDE_SECONDARY;
		reg |= extra;
	}
	if (reg >= 0)
		reg &= IDE_SECONDARY | 7;

	return reg;
}

static int get_apollo_reg(uaecptr addr, struct ide_board *board)
{
	if (addr & 0x4000)
		return -1;
	int reg = addr & 0x1fff;
	reg >>= 10;
	if (addr & 0x2000)
		reg |= IDE_SECONDARY;
	if (reg != 0 && !(addr & 1))
		reg = -1;
	return reg;
}

static int get_alf_reg(uaecptr addr, struct ide_board *board)
{
	if (addr & 0x8000)
		return -1;
	if (addr & 0x4000) {
		;
	} else if (addr & 0x1000) {
		addr &= 0xfff;
		addr >>= 9;
	} else if (addr & 0x2000) {
		addr &= 0xfff;
		addr >>= 9;
		addr |= IDE_SECONDARY;
	}
	return addr;
}

static int get_masoboshi_reg(uaecptr addr, struct ide_board *board)
{
	int reg;
	if (addr < 0xfc00)
		return -1;
	reg = 7 - ((addr >> 6) & 7);
	if (addr < 0xfe00)
		reg |= IDE_SECONDARY;
	return reg;
}
static void masoboshi_ide_dma(struct ide_board *board)
{
	board->state2[0] |= 0x80;
}

static int get_trifecta_reg(uaecptr addr, struct ide_board *board)
{
	int reg;
	if (!(addr & 0x800))
		return -1;
	reg = (addr >> 6) & 7;
	if (addr & 0x200)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_adide_reg(uaecptr addr, struct ide_board *board)
{
	int reg;
	if (addr & 0x8000)
		return -1;
	reg = (addr >> 1) & 7;
	if (addr & 0x10)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_buddha_reg(uaecptr addr, struct ide_board *board, int *portnum, int *flashoffset)
{
	int reg = -1;
	if (flashoffset) {
		*flashoffset = -1;
	}
	if (board->flashenabled) {
		if (flashoffset && !(addr & 1)) {
			*flashoffset = ((board->userdata & 0x100) ? 32768 : 0) + (addr >> 1);
		}
		return -1;
	}
	if (addr < 0x800 || addr >= 0xe00)
		return reg;
	*portnum = (addr - 0x800) / 0x200;
	if ((board->aci->rc->device_settings & 3) == 1) {
		if ((addr & (0x80 | 0x40)) == 0x80) {
			return IDE_DATA;
		}
		if ((addr & (0x80 | 0x40)) == 0xc0) {
			return -1;
		}
	}
	reg = (addr >> 2) & 15;
	if (addr & 0x100)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_rochard_reg(uaecptr addr, struct ide_board *board, int *portnum)
{
	int reg = -1;
	if ((addr & 0x8001) != 0x8001)
		return -1;
	*portnum = (addr & 0x4000) ? 1 : 0;
	reg = (addr >> 5) & 7;
	if (addr & 0x2000)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_dataflyerplus_reg(uaecptr addr, struct ide_board *board)
{
	int reg = -1;
	if (!(addr & 0x8000))
		return -1;
	reg = (addr / 0x40) & 7;
	return reg;
}

static int get_golemfast_reg(uaecptr addr, struct ide_board *board)
{
	int reg = -1;
	if ((addr & 0x8001) != 0x8000)
		return -1;
	reg = (addr >> 2) & 7;
	if (addr & 0x2000)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_ateam_reg(uaecptr addr, struct ide_board *board)
{
	if (!(addr & 1))
		return -1;
	if ((addr & 0xf000) != 0xf000)
		return -1;
	addr >>= 7;
	addr &= 15;
	if (addr >= 8) {
		addr &= 7;
		addr |= IDE_SECONDARY;
	}
	return addr;
}

static int get_arriba_reg(uaecptr addr, struct ide_board *board)
{
	if (!(addr & 0x8000))
		return -1;
	if (addr & 1)
		return -1;
	int reg = ((addr & 0x0fff) >> 9) & 7;
	return reg;
}

static int get_elsathd_reg(uaecptr addr, struct ide_board *board)
{
	if (!(addr & 0x8000))
		return -1;
	if (!(addr & 1))
		return -1;
	int reg = ((addr & 0x0fff) >> 9) & 7;
	return reg;
}

static int get_fastata4k_reg(uaecptr addr, struct ide_board *board, int *portnum)
{
	*portnum = 0;
	if (addr & 1)
		return -1;
	if (!(addr & 0x8000))
		return -1;
	if ((addr & 0xf500) == 0x8500)
		return -2;
	if (addr & 0x0100)
		return -1;
	*portnum = (addr & 0x1000) ? 1 : 0;
	addr &= 0xe00;
	addr >>= 9;
	return addr;
}

static int get_accessx_reg(uaecptr addr, struct ide_board *board, int *portnum)
{
	*portnum = 0;
	if (!(addr & 0x8000))
		return -1;
	if ((addr & 1))
		return -1;
	if (addr & 0x400)
		*portnum = 1;
	int reg = (addr >> 6) & 7;
	if (addr & 0x200)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_ivst500at_reg(uaecptr addr, struct ide_board *board, int *portnum)
{
	*portnum = 0;
	if (addr & (0x4000 | 0x8000))
		return -1;
	if (!(addr & 1))
		return -1;
	int reg = (addr >> 2) & 7;
	if (addr & 0x2000)
		reg |= IDE_SECONDARY;
	return reg;
}

static int get_dev_hd_reg(uaecptr addr, struct ide_board* board)
{
	int reg = -1;
	if (addr & 0x8000) {
		return -1;
	}
	if (addr >= dev_hd_io_base && addr < dev_hd_io_base + dev_hd_io_total) {
		reg = (addr - dev_hd_io_base) / dev_hd_io_size;
		reg &= 7;
		if (reg == 0) {
			write_log("invalid %08x\n", addr);
		}
	} else if (addr >= dev_hd_data_base && addr < dev_hd_data_base + 0x100) {
		reg = 0;
	} else {
		write_log("out of range %08x\n", addr);
	}
	return reg;
}

static int get_ripple_reg(uaecptr addr, struct ide_board *board, int *portnum)
{
	int reg = -1;
	*portnum = (addr & 0x2000) ? 1 : 0;

	if (addr & 0x3000) {
		reg = (addr >> 9) & 7;

		if (addr & 0x4000) {
			reg |= IDE_SECONDARY;
		}
	}

	return reg;
}

static int getidenum(struct ide_board *board, struct ide_board **arr)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		if (board == arr[i])
			return i;
	}
	return 0;
}

static uae_u32 ide_read_byte2(struct ide_board *board, uaecptr addr)
{
	uaecptr oaddr = addr;
	uae_u8 v = 0xff;

	addr &= board->mask;

	if (addr < 0x40 && !board->flashenabled && (!board->configured || board->keepautoconfig))
		return board->acmemory[addr];

	if (board->type == BUDDHA_IDE) {

		int portnum;
		int flashoffset = -1;
		bool p1 = (board->aci->rc->device_settings & 3) == 1;
		int regnum = get_buddha_reg(addr, board, &portnum, &flashoffset);
		if (flashoffset >= 0) {
			v = flash_read(board->flashrom, flashoffset);
		} else if (regnum >= 0) {
			if (board->ide[portnum])
				v = get_ide_reg_multi(board, regnum, portnum, 1);
		} else if (addr >= 0xf00 && addr < 0x1000) {
			if ((addr & ~3) == 0xf00) {
				v = ide_irq_check(board->ide[0], false) ? 0x80 : 0x00;
			} else if ((addr & ~3) == 0xf40) {
				v = ide_irq_check(board->ide[1], false) ? 0x80 : 0x00;
			} else if ((addr & ~3) == 0xf80 && !p1) {
				v = ide_irq_check(board->ide[2], false) ? 0x80 : 0x00;
			} else {
				v = 0;
			}
			if (p1) {
				if (addr == 0xf42 && board->hardreset) {
					v |= 0x80;
				}
				if (addr & 2) {
					v |= 1 << 5;
				}
			}
		} else if (addr >= 0x7fc && addr <= 0x7ff) {
			v = board->userdata;
		} else if (!(addr & 1)) {
			int offset = (addr >> 1) & board->rom_mask;
			if (p1 && (board->userdata & 0x100)) {
				offset += 0x8000;
			}
			v = board->rom[offset];
		}

	} else if (board->type == ALF_IDE || board->type == TANDEM_IDE) {

		if (addr < 0x1100 || (addr & 1)) {
			if (board->rom)
				v = board->rom[addr & board->rom_mask];
			return v;
		}
		int regnum = get_alf_reg(addr, board);
		if (regnum >= 0) {
			v = get_ide_reg(board, regnum);
		}
#if DEBUG_IDE_ALF
		write_log(_T("ALF GET %08x %02x %d %08x\n"), addr, v, regnum, M68K_GETPC);
#endif

	} else if (board->type == GOLEMFAST_IDE) {

		if (!(addr & 0x8000)) {
			if (board->rom) {
				v = board->rom[addr & board->rom_mask];
			}
		} else if ((addr & 0x8700) == 0x8400 || (addr & 0x8700) == 0x8000) {
#ifdef NCR9X
			v = golemfast_ncr9x_scsi_get(oaddr, getidenum(board, golemfast_board));
#endif
		} else if ((addr & 0x8700) == 0x8100) {
			int regnum = get_golemfast_reg(addr, board);
			if (regnum >= 0) {
				v = get_ide_reg(board, regnum);
			}
		} else if ((addr & 0x8700) == 0x8300) {
			v = board->original_rc->device_id ^ 7;
			if (!board->original_rc->autoboot_disabled)
				v |= 0x20;
			if (!(board->original_rc->device_settings & 1))
				v |= 0x08;
			if (ide_irq_check(board->ide[0], false) || ide_drq_check(board->ide[0]))
				v |= 0x80; // IDE IRQ | DRQ
			//write_log(_T("READ JUMPER %08x %02x %08x\n"), addr, v, M68K_GETPC);
		}
		
	} else if (board->type == MASOBOSHI_IDE) {
		int regnum = -1;
		bool rom = false;
		if (addr >= MASOBOSHI_ROM_OFFSET && addr < MASOBOSHI_ROM_OFFSET_END) {
			if (board->rom) {
				v = board->rom[addr & board->rom_mask];
				rom = true;
			}
		} else if ((addr >= 0xf000 && addr <= 0xf00f) || (addr >= 0xf100 && addr <= 0xf10f)) {
			// scsi dma controller
#ifdef NCR9X
			if (board->subtype)
				v = masoboshi_ncr9x_scsi_get(oaddr, getidenum(board, masoboshi_board));
#endif
		} else if (addr == 0xf040) {
			v = 1;
			if (ide_irq_check(board->ide[0], false)) {
				v |= 2;
				board->irq = true;
			}
			if (board->irq) {
				v &= ~1;
			}
			v |= board->state2[0] & 0x80;
		} else if (addr == 0xf047) {
			v = board->state;
		} else {
			regnum = get_masoboshi_reg(addr, board);
			if (regnum >= 0) {
				v = get_ide_reg(board, regnum);
			} else if (addr >= MASOBOSHI_SCSI_OFFSET && addr < MASOBOSHI_SCSI_OFFSET_END) {
#ifdef NCR9X
				if (board->subtype)
					v = masoboshi_ncr9x_scsi_get(oaddr, getidenum(board, masoboshi_board));
				else
#endif
					v = 0xff;
			}
		}
#if DEBUG_IDE_MASOBOSHI
		if (!rom)
			write_log(_T("MASOBOSHI BYTE GET %08x %02x %d %08x\n"), addr, v, regnum, M68K_GETPC);
#endif

	} else if (board->type == TRIFECTA_IDE) {

		if (addr & 1) {
			int regnum = get_trifecta_reg(addr, board);
			if (regnum >= 0) {
				v = get_ide_reg(board, regnum);
			}
		}
		if (addr >= TRIFECTA_SCSI_OFFSET && addr < TRIFECTA_SCSI_OFFSET_END) {
#ifdef NCR9X
			if (board->subtype)
				v = trifecta_ncr9x_scsi_get(oaddr, getidenum(board, trifecta_board));
			else
#endif
				v = 0xff;
		}
		if (addr == 0x401) {
			if (board->subtype) {
				// LX (SCSI+IDE)
				v = (board->aci->rc->device_id ^ 7) & 7; // inverted SCSI ID
			} else {
				// EC (IDE)
				v = 0;
			}
			v |= (board->aci->rc->device_settings ^ 0xfc) << 3;
		} else if (addr >= 0x1000) {
			if (board->rom)
				v = board->rom[addr & board->rom_mask];
		}
		//write_log(_T("trifecta get %08x %08x\n"), addr, M68K_GETPC);


	} else if (board->type == APOLLO_IDE) {

		if (addr >= APOLLO_ROM_OFFSET) {
			if (board->rom)
				v = board->rom[(addr - APOLLO_ROM_OFFSET) & board->rom_mask];
		} else if (board->configured) {
			if ((addr & 0xc000) == 0x4000) {
				v = apollo_scsi_bget(oaddr, board->userdata);
			} else if (addr < 0x4000) {
				int regnum = get_apollo_reg(addr, board);
				if (regnum >= 0) {
					v = get_ide_reg(board, regnum);
				} else {
					v = 0;
				}
			}
		}

	} else if (board->type == GVP_IDE) {

		if (addr >= GVP_IDE_ROM_OFFSET) {
			if (board->rom) {
				if (addr & 1)
					v = 0xe8; // board id
				else 
					v = board->rom[((addr - GVP_IDE_ROM_OFFSET) / 2) & board->rom_mask];
				return v;
			}
			v = 0xe8;
#if DEBUG_IDE_GVP
			write_log(_T("GVP BOOT GET %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			return v;
		}
		if (board->configured) {
			if (board == gvp_ide_rom_board && ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SII)) {
				if (addr == 0x42) {
					v = 0xff;
				}
#if DEBUG_IDE_GVP
				write_log(_T("GVP BOOT GET %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			} else {
				int regnum = get_gvp_reg(addr, board);
#if DEBUG_IDE_GVP
				write_log(_T("GVP IDE GET %08x %02x %d %08x\n"), addr, v, regnum, M68K_GETPC);
#endif
				if (regnum >= 0) {
					v = get_ide_reg(board, regnum);
				} else if (is_gvp2_intreq(addr)) {
					v = board->irq ? 0x40 : 0x00;
#if DEBUG_IDE_GVP
					write_log(_T("GVP IRQ %02x\n"), v);
#endif
					ide_interrupt_check(board, false);
				} else if (is_gvp1_intreq(addr)) {
					v = board->irq ? 0x80 : 0x00;
#if DEBUG_IDE_GVP
					write_log(_T("GVP IRQ %02x\n"), v);
#endif
					ide_interrupt_check(board, false);
				}
			}
		} else {
			v = 0xff;
		}

	} else if (board->type == ADIDE_IDE) {

		if (addr & ADIDE_ROM_OFFSET) {
			v = board->rom[addr & board->rom_mask];
		} else if (board->configured) {
			int regnum = get_adide_reg(addr, board);
			v = get_ide_reg(board, regnum);
			v = (uae_u8)adide_decode_word(v);
		}

	} else if (board->type == MTEC_IDE) {

		if (!(addr & 0x8000)) {
			v = board->rom[addr & board->rom_mask];
		} else if (board->configured) {
			v = get_ide_reg(board, (addr >> 8) & 7);
		}

	} else if (board->type == PROTAR_IDE) {

		v = board->rom[addr & board->rom_mask];

	} else if (board->type == DATAFLYERPLUS_IDE) {

		v = board->rom[addr & board->rom_mask];
		if (board->configured) {
			if (addr == 0x10) {
				if (board->subtype & 2) {
					v = ide_irq_check(board->ide[0], false) ? 0x08 : 0x00;
				}
			} else if (addr < 0x80) {
				if (board->subtype & 1) {
					v = idescsi_scsi_get(oaddr);
				} else {
					v = 0xff;
				}
			}
		}
		if ((addr & 0x8000) && (board->subtype & 2)) {
			int regnum = get_dataflyerplus_reg(addr, board);
			if (regnum >= 0)
				v = get_ide_reg(board, regnum);
		}

	} else if (board->type == ROCHARD_IDE) {

		if (addr & 0x8000) {
			int portnum;
			int regnum = get_rochard_reg(addr, board, &portnum);
			if (regnum >= 0 && board->ide[portnum])
				v = get_ide_reg_multi(board, regnum, portnum, 1);
		} else if ((addr & 0x7c00) == 0x7000) {
			if (board->subtype)
				v = idescsi_scsi_get(oaddr);
			else
				v = 0;
		} else {
			v = board->rom[addr & board->rom_mask];
		}

	} else if (board->type == ATEAM_IDE) {

		if (addr == 1) {
			v = ide_irq_check(board->ide[0], false) ? 0x00 : 0x80;
		} else {
			int reg = get_ateam_reg(addr, board);
			if (reg >= 0) {
				v = get_ide_reg(board, reg);
			} else {
				v = board->rom[addr & board->rom_mask];
			}
		}

	} else if (board->type == ARRIBA_IDE) {

		int reg = get_arriba_reg(addr, board);
		if (reg >= 0) {
			v = get_ide_reg(board, reg);
		} else if (board->rom && !(addr & 0x8000)) {
			int offset = addr & 0x7fff;
			v = board->rom[offset];
		}

	} else if (board->type == ELSATHD_IDE) {

		int reg = get_elsathd_reg(addr, board);
		if (reg >= 0) {
			v = get_ide_reg(board, reg);
		} else if (board->rom && !(addr & 0x8000)) {
			int offset = addr & 0x7fff;
			v = board->rom[offset];
		}

	} else if (board->type == FASTATA4K_IDE) {

		int portnum;
		int reg = get_fastata4k_reg(addr, board, &portnum);
		if (reg >= 0) {
			if (board->ide[portnum])
				v = get_ide_reg_multi(board, reg, portnum, 1);
		} else if (reg == -2) {
			v = ide_irq_check(board->ide[0], false) ? 1 << 6 : 0;
			if (board->ide[1])
				v |= ide_irq_check(board->ide[1], false) ? 1 << 5 : 0;
			if (v)
				v |= 1 << 7;
		} else {
			if (board->rom && addr >= 0x0200) {
				int offset = addr - 0x200;
				offset &= board->rom_mask;
				v = board->rom[(offset + 0) & board->rom_mask];
			}
		}

	} else if (board->type == ACCESSX_IDE) {

		int portnum;
		int reg = get_accessx_reg(addr, board, &portnum);
		if (reg >= 0) {
			if (board->ide[portnum])
				v = get_ide_reg_multi(board, reg, portnum, 1);
		} else if (board->rom && !(addr & 0x8000)) {
			int offset = addr & 0x7fff;
			v = board->rom[offset];
		}

	} else if (board->type == IVST500AT_IDE) {

		int portnum;
		int reg = get_ivst500at_reg(addr, board, &portnum);
		if (reg >= 0) {
			if (board->ide[portnum])
				v = get_ide_reg_multi(board, reg, portnum, 1);
		} else if (board->rom && (addr & 0x8000)) {
			int offset = addr & 0x7fff;
			v = board->rom[offset];
		} else if ((addr & 0x4000) && (addr & 1)) {
			v = ide_irq_check(board->ide[0], false) ? 1 : 0;
		}

	} else if (board->type == DOTTO_IDE) {

		int offset = addr;
		v = board->rom[offset];
		if (board->configured) {
			int regnum = get_adide_reg(addr, board);
			if (regnum >= 0) {
				v = get_ide_reg(board, regnum);
				v = (uae_u8)adide_decode_word(v);
			}
		}

	} else if (board->type == DEV_IDE) {

		if (dev_hd_mode == 1) {

			struct ide_hdf *ide = board->ide[0];
			int reg = (addr & 7);
			if (reg == 2) {
				v = ide->secbuf[board->dma_cnt & (ide->blocksize - 1)];
				board->dma_cnt++;
			}

		} else {

			if (addr == 0x88) {
				v = 0xff;
			} else if (addr == 0x86 || addr == 0x90 || addr == 0x92 || addr == 0x94 || addr == 0x96) {
				if (addr == 0x86) {
					board->dma_ptr = 0x80;
					board->dma_cnt = 1;
				}
				v = board->rom[board->dma_ptr * 2 + 0x8000];
				board->dma_ptr++;
				if (board->dma_cnt == 1) {
					uae_u8 v2 = board->rom[board->dma_ptr * 2 + 0x8000];
					board->dma_ptr++;
					if (v != (v2 ^ 0xff)) {
						write_log("error!\n");
					}
					board->dma_cnt = 0;
				}
			} else {
				int reg = get_dev_hd_reg(addr, board);
				if (reg >= 0) {
					v = get_ide_reg(board, reg);
				} else {
					v = board->rom[addr];
				}
			}

		}
	} else if (board->type == RIPPLE_IDE) {
			if (board->rom && board->userdata == 0) {
				v = board->rom[(addr & board->rom_mask)];
				return v;
			}
			int portnum = 0;
			int regnum = get_ripple_reg(addr,board,&portnum);
			if (!ide_isdrive(board->ide[portnum]) && !ide_isdrive(board->ide[portnum]->pair)) {
				v = 0xff;
				return v;
			}
			if (regnum >= 0 && board->ide[portnum]) {
				v = get_ide_reg_multi(board,regnum,portnum,1);
			}

	}

	return v;
}

static uae_u32 ide_read_byte(struct ide_board *board, uaecptr addr)
{
	uae_u32 v = ide_read_byte2(board, addr);
#if DEBUG_IDE
	if ((addr & DEBUG_IDE_MASK) == DEBUG_IDE_MASK_VAL) {
		write_log(_T("IDE IO BYTE READ %08x=%02x %08x\n"), addr & board->mask, v & 0xff, M68K_GETPC);
	}
#endif
	return v;
}

static uae_u32 ide_read_word(struct ide_board *board, uaecptr addr)
{
	uae_u32 v = 0xffff;

	addr &= board->mask;

	if (addr < 0x40 && (!board->configured || board->keepautoconfig)) {
		v = board->acmemory[addr] << 8;
		v |= board->acmemory[addr + 1];
		return v;
	}

	if (board->type == APOLLO_IDE) {

		if (addr >= APOLLO_ROM_OFFSET) {
			if (board->rom) {
				v = board->rom[(addr + 0 - APOLLO_ROM_OFFSET) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(addr + 1 - APOLLO_ROM_OFFSET) & board->rom_mask];
			}
		}

	} else if (board->type == DATAFLYERPLUS_IDE) {

		if (!(addr & 0x8000) && (board->subtype & 2)) {
			if (board->rom) {
				v = board->rom[(addr + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(addr + 1) & board->rom_mask];
			}
		}

	} else if (board->type == ROCHARD_IDE) {

		if (addr < 8192) {
			if (board->rom) {
				v = board->rom[(addr + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(addr + 1) & board->rom_mask];
			}
		}

	}

	if (board->configured) {

		if (board->type == BUDDHA_IDE) {

			int portnum;
			int regnum = get_buddha_reg(addr, board, &portnum, NULL);
			if (regnum == IDE_DATA) {
				if (board->ide[portnum]) {
					v = get_ide_reg_multi(board, IDE_DATA, portnum, 1);
				}
			} else {
				v = ide_read_byte(board, addr) << 8;
				v |= ide_read_byte(board, addr + 1);
			}
			
		} else if (board->type == ALF_IDE || board->type == TANDEM_IDE) {

			int regnum = get_alf_reg(addr, board);
			if (regnum == IDE_DATA) {
				v = get_ide_reg(board, IDE_DATA);
			} else {
				v = 0;
				if (addr == 0x4000 && board->intena)
					v = board->irq ? 0x8000 : 0x0000;
#if DEBUG_IDE_ALF
				write_log(_T("ALF IO WORD READ %08x %08x\n"), addr, M68K_GETPC);
#endif
			}

		} else if (board->type == GOLEMFAST_IDE) {

			if ((addr & 0x8700) == 0x8100) {
				int regnum = get_golemfast_reg(addr, board);
				if (regnum == IDE_DATA) {
					v = get_ide_reg(board, IDE_DATA);
				} else {
					v = ide_read_byte(board, addr) << 8;
					v |= ide_read_byte(board, addr + 1);
				}
			} else {
				v = ide_read_byte(board, addr) << 8;
				v |= ide_read_byte(board, addr + 1);
			}

		} else if (board->type == MASOBOSHI_IDE) {

			if (addr >= MASOBOSHI_ROM_OFFSET && addr < MASOBOSHI_ROM_OFFSET_END) {
				if (board->rom) {
					v = board->rom[addr & board->rom_mask] << 8;
					v |= board->rom[(addr + 1) & board->rom_mask];
				}
			} else if (addr == 0xf04c || addr == 0xf14c) {
				v = board->dma_ptr >> 16;
			} else if (addr == 0xf04e || addr == 0xf14e) {
				v = board->dma_ptr;
				write_log(_T("MASOBOSHI IDE DMA PTR READ = %08x %08x\n"), board->dma_ptr, M68K_GETPC);
			} else if (addr == 0xf04a || addr == 0xf14a) {
				v = board->dma_cnt;
				write_log(_T("MASOBOSHI IDE DMA LEN READ = %04x %08x\n"), board->dma_cnt, v, M68K_GETPC);
			} else {
				int regnum = get_masoboshi_reg(addr, board);
				if (regnum == IDE_DATA) {
					v = get_ide_reg(board, IDE_DATA);
				} else {
					v = ide_read_byte(board, addr) << 8;
					v |= ide_read_byte(board, addr + 1);
				}
			}

		} else if (board->type == TRIFECTA_IDE) {

			if (addr >= 0x1000) {
				if (board->rom) {
					v = board->rom[addr & board->rom_mask] << 8;
					v |= board->rom[(addr + 1) & board->rom_mask];
				}
			} else {
				int regnum = get_trifecta_reg(addr, board);
				if (regnum == IDE_DATA) {
					v = get_ide_reg(board, IDE_DATA);
				} else {
					v = ide_read_byte(board, addr) << 8;
					v |= ide_read_byte(board, addr + 1);
				}
			}

		} else if (board->type == APOLLO_IDE) {

			if ((addr & 0xc000) == 0x4000) {
				v = apollo_scsi_bget(addr, board->userdata);
				v <<= 8;
				v |= apollo_scsi_bget(addr + 1, board->userdata);
			} else if (addr < 0x4000) {
				int regnum = get_apollo_reg(addr, board);
				if (regnum == IDE_DATA) {
					v = get_ide_reg(board, IDE_DATA);
				} else {
					v = 0;
				}
			}

		} else if (board->type == GVP_IDE) {

			if (board == gvp_ide_controller_board || ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SI)) {
				if (addr < 0x60) {
					if (is_gvp1_intreq(addr))
						v = gvp_ide_controller_board->irq ? 0x8000 : 0x0000;
					else if (addr == 0x40) {
						if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SII))
							v = board->intena ? 8 : 0;
					}
#if DEBUG_IDE_GVP
					write_log(_T("GVP IO WORD READ %08x %08x\n"), addr, M68K_GETPC);
#endif
				} else {
					int regnum = get_gvp_reg(addr, board);
					if (regnum == IDE_DATA) {
						v = get_ide_reg(board, IDE_DATA);
#if DEBUG_IDE_GVP > 2
						write_log(_T("IDE WORD READ %04x\n"), v);
#endif
					} else {
						v = ide_read_byte(board, addr) << 8;
						v |= ide_read_byte(board, addr + 1);
					}
				}	
			}

		} else if (board->type == ADIDE_IDE) {

			int regnum = get_adide_reg(addr, board);
			if (regnum == IDE_DATA) {
				v = get_ide_reg(board, IDE_DATA);
			} else {
				v = get_ide_reg(board, regnum) << 8;
				v = adide_decode_word(v);
			}

		} else if (board->type == MTEC_IDE) {

			if (addr & 0x8000) {
				int regnum = (addr >> 8) & 7;
				if (regnum == IDE_DATA)
					v = get_ide_reg(board, regnum);
				else
					v = ide_read_byte(board, addr) << 8;
			}

		} else if (board->type == DATAFLYERPLUS_IDE) {

			if (board->subtype & 2) {
				int reg = get_dataflyerplus_reg(addr, board);
				if (reg >= 0)
					v = get_ide_reg_multi(board, reg, 0, 1);
			} else {
				v = 0xff;
			}

		} else if (board->type == ROCHARD_IDE) {

			if ((addr & 0x8020) == 0x8000) {
				v = get_ide_reg_multi(board, IDE_DATA, (addr & 0x4000) ? 1 : 0, 1);
			}

		} else if (board->type == ATEAM_IDE) {

			if ((addr & 0xf800) == 0xf800) {
				v = get_ide_reg_multi(board, IDE_DATA, 0, 1);
			}

		} else if (board->type == ARRIBA_IDE) {

			int reg = get_arriba_reg(addr, board);
			if (reg == 0) {
				v = get_ide_reg_multi(board, IDE_DATA, 0, 1);
			} else if (board->rom && !(addr & 0x8000)) {
				int offset = addr & 0x7fff;
				v = board->rom[(offset + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(offset + 1) & board->rom_mask];
			}

		} else if (board->type == ELSATHD_IDE) {

			int reg = get_elsathd_reg(addr | 1, board);
			if (reg == 0) {
				v = get_ide_reg_multi(board, IDE_DATA, 0, 1);
			} else if (reg > 0) {
				v = get_ide_reg(board, reg);
			} else if (board->rom && !(addr & 0x8000)) {
				int offset = addr & 0x7fff;
				v = board->rom[(offset + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(offset + 1) & board->rom_mask];
			}

		} else if (board->type == FASTATA4K_IDE) {

			int portnum;
			int reg = get_fastata4k_reg(addr, board, &portnum);
			if (reg == 0) {
				v = get_ide_reg_multi(board, IDE_DATA, portnum, 1);
			} else if (board->rom && addr >= 0x0200) {
				int offset = addr - 0x200;
				offset &= board->rom_mask;
				v = board->rom[(offset + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(offset + 1) & board->rom_mask];
			}

		} else if (board->type == ACCESSX_IDE) {

			int portnum;
			int reg = get_accessx_reg(addr, board, &portnum);
			if (reg == 0) {
				v = get_ide_reg_multi(board, IDE_DATA, portnum, 1);
			} else if (board->rom && !(addr & 0x8000)) {
				int offset = addr & 0x7fff;
				v = board->rom[(offset + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(offset + 1) & board->rom_mask];
			}

		} else if (board->type == IVST500AT_IDE) {

			int portnum;
			int reg = get_ivst500at_reg(addr, board, &portnum);
			if (reg == 0 || addr == 0 || addr == 2) {
				v = get_ide_reg_multi(board, IDE_DATA, portnum, 1);
			} else if (board->rom && (addr & 0x8000)) {
				int offset = addr & 0x7fff;
				v = board->rom[(offset + 0) & board->rom_mask];
				v <<= 8;
				v |= board->rom[(offset + 1) & board->rom_mask];
			}

		} else if (board->type == DOTTO_IDE) {

			int offset = addr;
			v = board->rom[(offset + 0) & board->rom_mask];
			v <<= 8;
			v |= board->rom[(offset + 1) & board->rom_mask];
			if (board->configured) {
				int regnum = get_adide_reg(addr, board);
				if (regnum >= 0) {
					if (regnum == IDE_DATA) {
						v = get_ide_reg(board, IDE_DATA);
					} else {
						v = get_ide_reg(board, regnum) << 8;
						v = adide_decode_word(v);
					}
				}
			}

		} else if (board->type == DEV_IDE) {

			if (dev_hd_mode == 1) {

			} else {

				int reg = get_dev_hd_reg(addr, board);
				if (reg == IDE_DATA) {
					v = get_ide_reg(board, reg);
				}
			}

		} else if (board->type == RIPPLE_IDE) {
			if (board->rom && board->userdata == 0) {
				v = board->rom[addr & board->rom_mask];
				v <<= 8;
				v |= board->rom[(addr + 1) & board->rom_mask];
				return v;
			}
			int portnum = 0;
			int regnum = get_ripple_reg(addr,board,&portnum);
			if (regnum == IDE_DATA && board->ide[portnum]) {
				v = get_ide_reg_multi(board, regnum, portnum, 1);
			}

		}

	}

#if DEBUG_IDE
	if ((addr & DEBUG_IDE_MASK) == DEBUG_IDE_MASK_VAL) {
		write_log(_T("IDE IO WORD READ %08x %04x %08x\n"), addr, v, M68K_GETPC);
	}
#endif

	return v;
}

static void ide_write_byte(struct ide_board *board, uaecptr addr, uae_u8 v)
{
	uaecptr oaddr = addr;
	addr &= board->mask;

#if DEBUG_IDE
	if ((addr & DEBUG_IDE_MASK) == DEBUG_IDE_MASK_VAL) {
		write_log(_T("IDE IO BYTE WRITE %08x=%02x %08x\n"), addr, v, M68K_GETPC);
	}
#endif

	if (!board->configured) {
		addrbank *ab = board->bank;
		if (addr == 0x48) {
			if (board->aci->zorro == 2) {
				map_banks_z2(ab, v, (board->mask + 1) >> 16);
				board->baseaddress = v << 16;
				board->configured = 1;
				if (board->type == ROCHARD_IDE) {
					rochard_scsi_init(board->original_rc, board->baseaddress);
#ifdef NCR9X
				} else if (board->type == MASOBOSHI_IDE) {
					ncr_masoboshi_autoconfig_init(board->original_rc, board->baseaddress);
				} else if (board->type == GOLEMFAST_IDE) {
					ncr_golemfast_autoconfig_init(board->original_rc, board->baseaddress);
#endif
				} else if (board->type == DATAFLYERPLUS_IDE) {
					dataflyerplus_scsi_init(board->original_rc, board->baseaddress);
#ifdef NCR9X
				} else if (board->type == TRIFECTA_IDE) {
					ncr_trifecta_autoconfig_init(board->original_rc, board->baseaddress);
#endif
				}
				expamem_next(ab, NULL);
			}
			return;
		}
		if (addr == 0x4c) {
			board->configured = 1;
			expamem_shutup(ab);
			return;
		}
	}
	if (board->configured) {

		if (board->type == BUDDHA_IDE) {

			int portnum;
			int flashoffset = -1;
			bool p1 = (board->aci->rc->device_settings & 3) == 1;
			int regnum = get_buddha_reg(addr, board, &portnum, &flashoffset);
			if (flashoffset >= 0) {
				flash_write(board->flashrom, flashoffset, v);
			} else if (regnum >= 0) {
				if (board->ide[portnum]) {
					put_ide_reg_multi(board, regnum, v, portnum, 1);
				}
			} else if (addr >= 0xfc0 && addr < 0xfc4) {
				board->intena = true;
				if (addr == 0xfc2 && p1) {
					uae_u8 v2 = v & 0xf0;
					int cnt = (board->userdata >> 16) & 15;
					if (v2 == 0xa0 && cnt == 0) {
						cnt++;
					} else if (v2 == 0x50 && cnt == 1) {
						cnt++;
					} else if (v2 == 0xa0 && cnt == 2) {
						cnt++;
					} else if (v2 == 0x70 && cnt == 3) {
						board->userdata |= 0x100000;
						write_log("RAM expansion OFF\n");
					} else if (v2 == 0xc0 && cnt == 3) {
						board->userdata |= 0x200000;
					} else if (v2 == 0xe0 && cnt == 3) {
						write_log("Lockdown EEPROM\n");
					} else if (v2 == 0xb0 && cnt == 3) {
						write_log("Fast-Z2 ON\n");
					} else if (v2 == 0x30 && cnt == 3) {
						write_log("Fast-Z2 OFF\n");
					} else if (v2 == 0x90 && cnt == 3) {
						write_log("Early write ON\n");
					} else if (v2 == 0x80 && cnt == 3) {
						write_log("Early write OFF\n");
					} else if (v2 == 0x60 && (board->userdata & 0x100000)) {
						write_log("$a00000 ON\n");
					} else if (v2 == 0x60 && (board->userdata & 0x200000)) {
						write_log("$c00000 ON\n");
					} else if (v2 == 0xf0 && cnt == 3) {
						board->flashenabled = true;
					} else {
						cnt = 0;
					}
					board->userdata &= 0xfff0ffff;
					board->userdata |= cnt << 16;
				}
				if (addr == 0xf42 && p1 && (v & 0xf0) == 0x60) {
					board->hardreset = false;
				}
			} else if (addr >= 0x7fc && addr <= 0x7ff) {
				board->userdata &= ~0xff;
				board->userdata |= v;
			} else if (addr == 0xc3) {
				if (p1) {
					board->userdata |= 0x100;
				}
			} else if (addr == 0xc1) {
				board->userdata &= ~0x100;
			} else if (addr == 1) {
				board->userdata = 0;
				board->flashenabled = false;
			}

		} else  if (board->type == ALF_IDE || board->type == TANDEM_IDE) {

			int regnum = get_alf_reg(addr, board);
			if (regnum >= 0)
				put_ide_reg(board, regnum, v);
#if DEBUG_IDE_ALF
			write_log(_T("ALF PUT %08x %02x %d %08x\n"), addr, v, regnum, M68K_GETPC);
#endif
		} else if (board->type == GOLEMFAST_IDE) {
#ifdef NCR9X
			if ((addr & 0x8700) == 0x8400 || (addr & 0x8700) == 0x8000) {
				golemfast_ncr9x_scsi_put(oaddr, v, getidenum(board, golemfast_board));
			} else
#endif
			if ((addr & 0x8700) == 0x8100) {
				int regnum = get_golemfast_reg(addr, board);
				if (regnum >= 0)
					put_ide_reg(board, regnum, v);
			}

		} else if (board->type == MASOBOSHI_IDE) {

#if DEBUG_IDE_MASOBOSHI
			write_log(_T("MASOBOSHI IO BYTE PUT %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			int regnum = get_masoboshi_reg(addr, board);
			if (regnum >= 0) {
				put_ide_reg(board, regnum, v);
			} else if (addr >= MASOBOSHI_SCSI_OFFSET && addr < MASOBOSHI_SCSI_OFFSET_END) {
#ifdef NCR9X
				if (board->subtype)
					masoboshi_ncr9x_scsi_put(oaddr, v, getidenum(board, masoboshi_board));
#endif
			} else if ((addr >= 0xf000 && addr <= 0xf007)) {
#ifdef NCR9X
				if (board->subtype)
					masoboshi_ncr9x_scsi_put(oaddr, v, getidenum(board, masoboshi_board));
#endif
			} else if (addr >= 0xf00a && addr <= 0xf00f) {
#ifdef NCR9X
				// scsi dma controller
				masoboshi_ncr9x_scsi_put(oaddr, v, getidenum(board, masoboshi_board));
#endif
			} else if (addr >= 0xf040 && addr <= 0xf04f) {
				// ide dma controller
				if (addr >= 0xf04c && addr < 0xf050) {
					int shift = (3 - (addr - 0xf04c)) * 8;
					uae_u32 mask = 0xff << shift;
					board->dma_ptr &= ~mask;
					board->dma_ptr |= v << shift;
					board->dma_ptr &= 0xffffff;
				} else if (addr >= 0xf04a && addr < 0xf04c) {
					if (addr == 0xf04a) {
						board->dma_cnt &= 0x00ff;
						board->dma_cnt |= v << 8;
					} else {
						board->dma_cnt &= 0xff00;
						board->dma_cnt |= v;
					}
				} else if (addr >= 0xf040 && addr < 0xf048) {
					board->state2[addr - 0xf040] = v;
					board->state2[0] &= ~0x80;
					if (addr == 0xf047) {
						board->state = v;
						board->intena = (v & 8) != 0;
						// masoboshi ide dma
						if (v & 0x80) {
							board->hsync_code = masoboshi_ide_dma;
							board->hsync_cnt = (board->dma_cnt / maxhpos) * 2 + 1;
							write_log(_T("MASOBOSHI IDE DMA %s start %08x, %d\n"), (board->state2[5] & 0x80) ? _T("READ") : _T("WRITE"), board->dma_ptr, board->dma_cnt);
							if (ide_drq_check(board->ide[0])) {
								if (!(board->state2[5] & 0x80)) {
									for (int i = 0; i < board->dma_cnt; i++) {
										put_ide_reg(board, IDE_DATA, get_word(board->dma_ptr & ~1));
										board->dma_ptr += 2;
									}
								} else {
									for (int i = 0; i < board->dma_cnt; i++) {
										put_word(board->dma_ptr & ~1, get_ide_reg(board, IDE_DATA));
										board->dma_ptr += 2;
									}
								}
								board->dma_cnt = 0;
							}
						}
					}
					if (addr == 0xf040) {
						board->state2[0] &= ~0x80;
						board->irq = false;
					}
				}
			}

		} else if (board->type == TRIFECTA_IDE) {

			if (addr & 1) {
				int regnum = get_trifecta_reg(addr, board);
				if (regnum >= 0) {
					put_ide_reg(board, regnum, v);
				}
			}
			if (addr >= TRIFECTA_SCSI_OFFSET && addr < TRIFECTA_SCSI_OFFSET_END) {
#ifdef NCR9X
				if (board->subtype)
					trifecta_ncr9x_scsi_put(oaddr, v, getidenum(board, trifecta_board));
#endif
			}
			if (addr >= 0x400 && addr <= 0x407) {
#ifdef NCR9X
				trifecta_ncr9x_scsi_put(oaddr, v, getidenum(board, trifecta_board));
#endif
			}
			
		} else if (board->type == APOLLO_IDE) {

			if ((addr & 0xc000) == 0x4000) {
				apollo_scsi_bput(oaddr, v, board->userdata);
			} else if (addr < 0x4000) {
				int regnum = get_apollo_reg(addr, board);
				if (regnum >= 0) {
					put_ide_reg(board, regnum, v);
				}
			}

		} else if (board->type == GVP_IDE) {

			if (board == gvp_ide_rom_board && ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SII)) {
#if DEBUG_IDE_GVP
				write_log(_T("GVP BOOT PUT %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			} else {
				int regnum = get_gvp_reg(addr, board);
#if DEBUG_IDE_GVP
				write_log(_T("GVP IDE PUT %08x %02x %d %08x\n"), addr, v, regnum, M68K_GETPC);
#endif
				if (regnum >= 0)
					put_ide_reg(board, regnum, v);
			}

		} else if (board->type == ADIDE_IDE) {

			if (board->configured) {
				int regnum = get_adide_reg(addr, board);
				v = (uae_u8)adide_encode_word(v);
				put_ide_reg(board, regnum, v);
			}

		} else if (board->type == MTEC_IDE) {

			if (board->configured && (addr & 0x8000)) {
				put_ide_reg(board, (addr >> 8) & 7, v);
			}

		} else if (board->type == DATAFLYERPLUS_IDE) {

			if (board->configured) {
				if (addr & 0x8000) {
					if  (board->subtype & 2) {
						int regnum = get_dataflyerplus_reg(addr, board);
						if (regnum >= 0)
							put_ide_reg(board, regnum, v);
					}
				} else if (addr < 0x80) {
					if (board->subtype & 1) {
						idescsi_scsi_put(oaddr, v);
					}
				}
			}

		} else if (board->type == ROCHARD_IDE) {

			if (board->configured) {
				if (addr & 0x8000) {
					int portnum;
					int regnum = get_rochard_reg(addr, board, &portnum);
					if (regnum >= 0 && board->ide[portnum])
						put_ide_reg_multi(board, regnum, v, portnum, 1);
				} else if ((addr & 0x7c00) == 0x7000) {
					if (board->subtype)
						idescsi_scsi_put(oaddr, v);
				}
			}

		} else if (board->type == ATEAM_IDE) {

			if ((addr & 0xff01) == 0x0101) {
				// disable interrupt strobe address
				board->intena = false;
			} else if ((addr & 0xff01) == 0x0201) {
				// enable interrupt strobe address
				board->intena = true;
			} else {
				int reg = get_ateam_reg(addr, board);
				if (reg >= 0) {
					put_ide_reg(board, reg, v);
				}
			}

		} else if (board->type == ARRIBA_IDE) {

			int reg = get_arriba_reg(addr, board);
			if (reg >= 0) {
				put_ide_reg(board, reg, v);
			}

		} else if (board->type == ELSATHD_IDE) {

			int reg = get_elsathd_reg(addr, board);
			if (reg >= 0) {
				put_ide_reg(board, reg, v);
			}

		} else if (board->type == FASTATA4K_IDE) {

			int portnum;
			int reg = get_fastata4k_reg(addr, board, &portnum);
			if (board->ide[portnum]) {
				put_ide_reg_multi(board, reg, v, portnum, 1);
			}

		} else if (board->type == ACCESSX_IDE) {

			int portnum;
			int reg = get_accessx_reg(addr, board, &portnum);
			if (board->ide[portnum]) {
				put_ide_reg_multi(board, reg, v, portnum, 1);
			}

		} else if (board->type == IVST500AT_IDE) {

			int portnum;
			int reg = get_ivst500at_reg(addr, board, &portnum);
			if (board->ide[portnum]) {
				put_ide_reg_multi(board, reg, v, portnum, 1);
			}

		} else if (board->type == DOTTO_IDE) {

			if (board->configured) {
				int regnum = get_adide_reg(addr, board);
				v = (uae_u8)adide_encode_word(v);
				put_ide_reg(board, regnum, v);
			}

		} else if (board->type == DEV_IDE) {

			if (dev_hd_mode == 1) {

				struct ide_hdf *ide = board->ide[0];
				int reg = (addr & 7);
				if (reg == 0 || reg == 4) {
					board->dma_cnt = 0;
				} else if (reg == 1) {
					board->dma_ptr >>= 8;
					board->dma_ptr |= v << 24;
					board->dma_cnt++;
					if (board->dma_cnt == 4) {
						uae_u32 error = 0;
						hdf_read(&ide->hdhfd.hfd, ide->secbuf, board->dma_ptr * ide->blocksize, ide->blocksize);
						board->dma_cnt = 0;
					}
				} else if (reg == 4 + 1) {
					board->dma_ptr >>= 8;
					board->dma_ptr |= v << 24;
					board->dma_cnt++;
					if (board->dma_cnt == 4) {
						board->dma_cnt = 0;
					}
				} else if (reg == 4 + 2) {
					ide->secbuf[board->dma_cnt & (ide->blocksize - 1)] = v;
					board->dma_cnt++;
					if (board->dma_cnt == ide->blocksize) {
						uae_u32 error = 0;
						hdf_write(&ide->hdhfd.hfd, ide->secbuf, board->dma_ptr *ide->blocksize, ide->blocksize);
					}
				}

			} else {

				if (addr == 0x86) {
					board->dma_ptr = 0;
					board->dma_cnt = 0;
				} else if (addr == 0x90 || addr == 0x92 || addr == 0x94 || addr == 0x96) {
					board->dma_ptr <<= 8;
					board->dma_ptr |= v;
					board->dma_ptr &= 0xffffff;
				} else {
					int reg = get_dev_hd_reg(addr, board);
					if (reg >= 0) {
						put_ide_reg(board, reg, v);
					}
				}

			}

		} else if (board->type == RIPPLE_IDE) {
			if ((addr & 0x3000) && board->userdata == 0) board->userdata = 1;
			int portnum = 0;
			int reg = get_ripple_reg(addr,board,&portnum);
			if (board->ide[portnum] && reg >= 0) {
				put_ide_reg_multi(board,reg,v,portnum,1);
			}
		}
	}
}

static void ide_write_word(struct ide_board *board, uaecptr addr, uae_u16 v)
{
	addr &= board->mask;

#if DEBUG_IDE
	if ((addr & DEBUG_IDE_MASK) == DEBUG_IDE_MASK_VAL) {
		write_log(_T("IDE IO WORD WRITE %08x=%04x %08x\n"), addr, v, M68K_GETPC);
	}
#endif

	if (board->configured) {

		if (board->type == BUDDHA_IDE) {

			int portnum;
			int regnum = get_buddha_reg(addr, board, &portnum, NULL);
			if (regnum == IDE_DATA) {
				if (board->ide[portnum])
					put_ide_reg_multi(board, IDE_DATA, v, portnum, 1);
			} else {
				ide_write_byte(board, addr, v >> 8);
				ide_write_byte(board, addr + 1, (uae_u8)v);
			}

		} else if (board->type == ALF_IDE || board->type == TANDEM_IDE) {

			int regnum = get_alf_reg(addr, board);
			if (regnum == IDE_DATA) {
				put_ide_reg(board, IDE_DATA, v);
			} else {
#if DEBUG_IDE_ALF
				write_log(_T("ALF IO WORD WRITE %08x %04x %08x\n"), addr, v, M68K_GETPC);
#endif
			}


		} else if (board->type == GOLEMFAST_IDE) {

			if ((addr & 0x8700) == 0x8100) {
				int regnum = get_golemfast_reg(addr, board);
				if (regnum == IDE_DATA) {
					put_ide_reg(board, IDE_DATA, v);
				} else {
					ide_write_byte(board, addr, v >> 8);
					ide_write_byte(board, addr + 1, (uae_u8)v);
				}
			} else {
				ide_write_byte(board, addr, v >> 8);
				ide_write_byte(board, addr + 1, (uae_u8)v);
			}
		
		} else if (board->type == MASOBOSHI_IDE) {

			int regnum = get_masoboshi_reg(addr, board);
			if (regnum == IDE_DATA) {
				put_ide_reg(board, IDE_DATA, v);
			} else {
				ide_write_byte(board, addr, v >> 8);
				ide_write_byte(board, addr + 1, (uae_u8)v);
			}
#if DEBUG_IDE_MASOBOSHI
			write_log(_T("MASOBOSHI IO WORD WRITE %08x %04x %08x\n"), addr, v, M68K_GETPC);
#endif
		} else if (board->type == TRIFECTA_IDE) {

			int regnum = get_trifecta_reg(addr, board);
			if (regnum == IDE_DATA) {
				put_ide_reg(board, IDE_DATA, v);
			} else {
				ide_write_byte(board, addr, v >> 8);
				ide_write_byte(board, addr + 1, (uae_u8)v);
			}

		} else if (board->type == APOLLO_IDE) {

			if ((addr & 0xc000) == 0x4000) {
				apollo_scsi_bput(addr, v >> 8, board->userdata);
				apollo_scsi_bput(addr + 1, (uae_u8)v, board->userdata);
			} else if (addr < 0x4000) {
				int regnum = get_apollo_reg(addr, board);
				if (regnum == IDE_DATA) {
					put_ide_reg(board, IDE_DATA, v);
				}
			}

		} else if (board->type == GVP_IDE) {

			if (board == gvp_ide_controller_board || ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SI)) {
				if (addr < 0x60) {
#if DEBUG_IDE_GVP
					write_log(_T("GVP IO WORD WRITE %08x %04x %08x\n"), addr, v, M68K_GETPC);
#endif
					if (addr == 0x40 && ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SII))
						board->intena = (v & 8) != 0;
				} else {
					int regnum = get_gvp_reg(addr, board);
					if (regnum == IDE_DATA) {
						put_ide_reg(board, IDE_DATA, v);
#if DEBUG_IDE_GVP > 2
						write_log(_T("IDE WORD WRITE %04x\n"), v);
#endif
					} else {
						ide_write_byte(board, addr, v >> 8);
						ide_write_byte(board, addr + 1, v & 0xff);
					}
				}
			}

		} else if (board->type == ADIDE_IDE) {

			int regnum = get_adide_reg(addr, board);
			if (regnum == IDE_DATA) {
				put_ide_reg(board, IDE_DATA, v);
			} else {
				v = adide_encode_word(v);
				put_ide_reg(board, regnum, v >> 8);
			}

		} else if (board->type == MTEC_IDE) {

			if (board->configured && (addr & 0x8000)) {
				int regnum = (addr >> 8) & 7;
				if (regnum == IDE_DATA)
					put_ide_reg(board, regnum, v);
				else
					ide_write_byte(board, addr, v >> 8);
			}

		} else if (board->type == DATAFLYERPLUS_IDE) {

			if (board->configured) {
				if (board->subtype & 2) {
					int reg = get_dataflyerplus_reg(addr, board);
					if (reg >= 0)
						put_ide_reg_multi(board, reg, v, 0, 1);
				}
			}

		} else if (board->type == ROCHARD_IDE) {

			if (board->configured && (addr & 0x8020) == 0x8000) {
				put_ide_reg_multi(board, IDE_DATA, v, (addr & 0x4000) ? 1 : 0, 1);
			}

		} else if (board->type == ATEAM_IDE) {

			if (board->configured && (addr & 0xf800) == 0xf800) {
				put_ide_reg_multi(board, IDE_DATA, v, 0, 1);
			}

		} else if (board->type == ARRIBA_IDE) {

			int reg = get_arriba_reg(addr, board);
			if (!reg) {
				put_ide_reg_multi(board, IDE_DATA, v, 0, 1);
			}

		} else if (board->type == ELSATHD_IDE) {

			int reg = get_elsathd_reg(addr | 1, board);
			if (!reg) {
				put_ide_reg_multi(board, IDE_DATA, v, 0, 1);
			} else if (reg > 0) {
				put_ide_reg(board, reg, v & 0xff);
			}

		} else if (board->type == FASTATA4K_IDE) {

			int portnum;
			int reg = get_fastata4k_reg(addr, board, &portnum);
			if (reg == 0) {
				put_ide_reg_multi(board, IDE_DATA, v, portnum, 1);
			}

		} else if (board->type == ACCESSX_IDE) {

			int portnum;
			int reg = get_accessx_reg(addr, board, &portnum);
			if (!reg) {
				put_ide_reg_multi(board, IDE_DATA, v, portnum, 1);
			}

		} else if (board->type == IVST500AT_IDE) {

			int portnum;
			int reg = get_ivst500at_reg(addr, board, &portnum);
			if (!reg || addr == 0 || addr == 2) {
				put_ide_reg_multi(board, IDE_DATA, v, portnum, 1);
			}

		} else if (board->type == DOTTO_IDE) {

			int regnum = get_adide_reg(addr, board);
			if (regnum == IDE_DATA) {
				put_ide_reg(board, IDE_DATA, v);
			} else {
				v = adide_encode_word(v);
				put_ide_reg(board, regnum, v >> 8);
			}
		} else if (board->type == DEV_IDE) {

			int reg = get_dev_hd_reg(addr, board);
			if (reg == IDE_DATA) {
				put_ide_reg(board, reg, v);
			}

		} else if (board->type == RIPPLE_IDE) {
			if ((addr & 0x3000) && board->userdata == 0)
				board->userdata = 1;
			if (board->configured) {
				int portnum = 0;
				int reg = get_ripple_reg(addr, board, &portnum);
				if (reg >= 0 && board->ide[portnum])
					put_ide_reg_multi(board,reg,v,portnum,1);
			}
		}
	}
}

static uae_u32 ide_read_wordi(struct ide_board *board, uaecptr addr)
{
	return ide_read_word(board, addr);
}

IDE_MEMORY_FUNCTIONS(ide_controller_gvp, ide, gvp_ide_controller_board);

addrbank gvp_ide_controller_bank = {
	ide_controller_gvp_lget, ide_controller_gvp_wget, ide_controller_gvp_bget,
	ide_controller_gvp_lput, ide_controller_gvp_wput, ide_controller_gvp_bput,
	default_xlate, default_check, NULL, NULL, _T("GVP IDE"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

IDE_MEMORY_FUNCTIONS(ide_rom_gvp, ide, gvp_ide_rom_board);

addrbank gvp_ide_rom_bank = {
	ide_rom_gvp_lget, ide_rom_gvp_wget, ide_rom_gvp_bget,
	ide_rom_gvp_lput, ide_rom_gvp_wput, ide_rom_gvp_bput,
	default_xlate, default_check, NULL, NULL, _T("GVP BOOT"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static void REGPARAM2 ide_generic_bput (uaecptr addr, uae_u32 b)
{
	struct ide_board *ide = getideboard(addr);
	if (ide)
		ide_write_byte(ide, addr, b);
}
static void REGPARAM2 ide_generic_wput (uaecptr addr, uae_u32 b)
{
	struct ide_board *ide = getideboard(addr);
	if (ide)
		ide_write_word(ide, addr, b);
}
static void REGPARAM2 ide_generic_lput (uaecptr addr, uae_u32 b)
{
	struct ide_board *ide = getideboard(addr);
	if (ide) {
		ide_write_word(ide, addr, b >> 16);
		ide_write_word(ide, addr + 2, b);
	}
}
static uae_u32 REGPARAM2 ide_generic_bget (uaecptr addr)
{
	struct ide_board *ide = getideboard(addr);
	if (ide)
		return ide_read_byte(ide, addr);
	return 0;
}
static uae_u32 REGPARAM2 ide_generic_wget (uaecptr addr)
{
	struct ide_board *ide = getideboard(addr);
	if (ide)
		return ide_read_word(ide, addr);
	return 0;
}
static uae_u32 REGPARAM2 ide_generic_lget (uaecptr addr)
{
	struct ide_board *ide = getideboard(addr);
	if (ide) {
		uae_u32 v = ide_read_word(ide, addr) << 16;
		v |= ide_read_word(ide, addr + 2);
		return v;
	}
	return 0;
}
static uae_u32 REGPARAM2 ide_generic_wgeti(uaecptr addr)
{
	struct ide_board *ide = getideboard(addr);
	if (ide)
		return ide_read_wordi(ide, addr);
	return 0;
}
static uae_u32 REGPARAM2 ide_generic_lgeti(uaecptr addr)
{
	struct ide_board *ide = getideboard(addr);
	if (ide) {
		uae_u32 v = ide_read_wordi(ide, addr) << 16;
		v |= ide_read_wordi(ide, addr + 2);
		return v;
	}
	return 0;
}
static uae_u8 *REGPARAM2 ide_generic_xlate(uaecptr addr)
{
	struct ide_board *ide = getideboard(addr);
	if (!ide)
		return NULL;
	addr &= ide->rom_mask;
	return ide->rom + addr;
}
static int REGPARAM2 ide_generic_check(uaecptr a, uae_u32 b)
{
	struct ide_board *ide = getideboard(a);
	if (!ide)
		return 0;
	a &= ide->rom_mask;
	if (a >= ide->rom_start && a + b < ide->rom_size)
		return 1;
	return 0;
}

static addrbank ide_bank_generic = {
	ide_generic_lget, ide_generic_wget, ide_generic_bget,
	ide_generic_lput, ide_generic_wput, ide_generic_bput,
	ide_generic_xlate, ide_generic_check, NULL, NULL, _T("IDE"),
	ide_generic_lgeti, ide_generic_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static struct ide_board *getide(struct autoconfig_info *aci)
{
	device_add_rethink(idecontroller_rethink);
	device_add_hsync(idecontroller_hsync);
	device_add_exit(idecontroller_free, NULL);

	for (int i = 0; i < MAX_IDE_UNITS; i++) {
		if (ide_boards[i]) {
			struct ide_board *ide = ide_boards[i];
			if (ide->rc == aci->rc) {
				ide->original_rc = aci->rc;
				ide->rc = NULL;
				ide->aci = aci;
				return ide;
			}
		}
	}
	return NULL;
}

static void ide_add_reset(void)
{
	device_add_reset(idecontroller_reset);
}

static void ew(struct ide_board *ide, int addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		ide->acmemory[addr] = (value & 0xf0);
		ide->acmemory[addr + 2] = (value & 0x0f) << 4;
	} else {
		ide->acmemory[addr] = ~(value & 0xf0);
		ide->acmemory[addr + 2] = ~((value & 0x0f) << 4);
	}
}

static const uae_u8 gvp_ide2_rom_autoconfig[16] = { 0xd1, 0x0d, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
static const uae_u8 gvp_ide2_controller_autoconfig[16] = { 0xc1, 0x0b, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uae_u8 gvp_ide1_controller_autoconfig[16] = { 0xd1, 0x08, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };

bool gvp_ide_rom_autoconfig_init(struct autoconfig_info *aci)
{
	const uae_u8 *autoconfig;
	if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SI)) {
		autoconfig = gvp_ide1_controller_autoconfig;
	} else {
		autoconfig = gvp_ide2_rom_autoconfig;
	}
	ide_add_reset();
	aci->autoconfigp = autoconfig;
	if (!aci->doinit)
		return true;

	struct ide_board *ide = getide(aci);

	if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A3001SI)) {
		ide->bank = &gvp_ide_rom_bank;
		init_ide(ide, GVP_IDE, 2, true, false);
		ide->rom_size = 8192;
		gvp_ide_controller_board->intena = true;
		ide->intena = true;
		gvp_ide_controller_board->configured = -1;
	} else {
		ide->bank = &gvp_ide_rom_bank;
		ide->rom_size = 16384;
	}
	ide->mask = 65536 - 1;
	ide->type = GVP_IDE;
	ide->configured = 0;
	memset(ide->acmemory, 0xff, sizeof ide->acmemory);

	ide->rom = xcalloc(uae_u8, ide->rom_size);
	memset(ide->rom, 0xff, ide->rom_size);
	ide->rom_mask = ide->rom_size - 1;

	load_rom_rc(aci->rc, ROMTYPE_CB_A3001S1, ide->rom_size, 0, ide->rom, ide->rom_size, LOADROM_FILL);
	for (int i = 0; i < 16; i++) {
		uae_u8 b = autoconfig[i];
		ew(ide, i * 4, b);
	}
	aci->addrbank = ide->bank;
	return true;
}

bool gvp_ide_controller_autoconfig_init(struct autoconfig_info *aci)
{
	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = gvp_ide2_controller_autoconfig;
		return true;
	}
	struct ide_board *ide = getide(aci);

	init_ide(ide, GVP_IDE, 2, true, false);
	ide->configured = 0;
	ide->bank = &gvp_ide_controller_bank;
	memset(ide->acmemory, 0xff, sizeof ide->acmemory);
	for (int i = 0; i < 16; i++) {
		uae_u8 b = gvp_ide2_controller_autoconfig[i];
		ew(ide, i * 4, b);
	}
	aci->addrbank = ide->bank;
	return true;
}

void gvp_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct ide_hdf *ide;

	if (!allocide(&gvp_ide_rom_board, rc, ch))
		return;
	if (!allocide(&gvp_ide_controller_board, rc, ch))
		return;
	ide = add_ide_unit (&idecontroller_drive[(GVP_IDE + ci->controller_type_unit) * 2], 2, ch, ci, rc);
}

static const uae_u8 alf_autoconfig[16] = { 0xd1, 6, 0x00, 0x00, 0x08, 0x2c, 0x00, 0x00, 0x00, 0x00, ALF_ROM_OFFSET >> 8, ALF_ROM_OFFSET & 0xff };
static const uae_u8 alfplus_autoconfig[16] = { 0xd1, 38, 0x00, 0x00, 0x08, 0x2c, 0x00, 0x00, 0x00, 0x00, ALF_ROM_OFFSET >> 8, ALF_ROM_OFFSET & 0xff };

bool alf_init(struct autoconfig_info *aci)
{
	bool alfplus = is_board_enabled(&currprefs, ROMTYPE_ALFAPLUS, 0);
	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = alfplus ? alfplus_autoconfig : alf_autoconfig;
		return true;
	}

	struct ide_board *ide = getide(aci);
	if (!ide)
		return false;

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->type = ALF_IDE;
	ide->rom_size = 32768 * 6;
	ide->userdata = alfplus;
	ide->intena = alfplus;
	ide->mask = 65536 - 1;

	memset(ide->acmemory, 0xff, sizeof ide->acmemory);

	ide->rom = xcalloc(uae_u8, ide->rom_size);
	memset(ide->rom, 0xff, ide->rom_size);
	ide->rom_mask = ide->rom_size - 1;

	for (int i = 0; i < 16; i++) {
		uae_u8 b = alfplus ? alfplus_autoconfig[i] : alf_autoconfig[i];
		ew(ide, i * 4, b);
	}

	if (!aci->rc->autoboot_disabled) {
		struct zfile *z = read_device_from_romconfig(aci->rc, alfplus ? ROMTYPE_ALFAPLUS : ROMTYPE_ALFA);
		if (z) {
			for (int i = 0; i < 0x1000 / 2; i++) {
				uae_u8 b;
				zfile_fread(&b, 1, 1, z);
				ide->rom[ALF_ROM_OFFSET + i * 4 + 0] = b;
				zfile_fread(&b, 1, 1, z);
				ide->rom[ALF_ROM_OFFSET + i * 4 + 2] = b;
			}
			for (int i = 0; i < 32768 - 0x1000; i++) {
				uae_u8 b;
				zfile_fread(&b, 1, 1, z);
				ide->rom[0x2000 + i * 4 + 1] = b;
				zfile_fread(&b, 1, 1, z);
				ide->rom[0x2000 + i * 4 + 3] = b;
			}
			zfile_fclose(z);
		}
	}
	aci->addrbank = ide->bank;
	return true;
}

void alf_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, alf_board, ALF_IDE, false, false, 2);
}

// prod 0x22 = IDE + SCSI
// prod 0x23 = SCSI only
// prod 0x33 = IDE only

const uae_u8 apollo_autoconfig[16] = { 0xd1, 0x22, 0x00, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, APOLLO_ROM_OFFSET >> 8, APOLLO_ROM_OFFSET & 0xff };
const uae_u8 apollo_autoconfig_cpuboard_12xx[16] = { 0xd2, 0x23, 0x40, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, APOLLO_ROM_OFFSET >> 8, APOLLO_ROM_OFFSET & 0xff };
const uae_u8 apollo_autoconfig_cpuboard_12xx_060[16] = { 0xd2, 0x23, 0x40, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x02, APOLLO_ROM_OFFSET >> 8, APOLLO_ROM_OFFSET & 0xff };

static bool apollo_init(struct autoconfig_info *aci, int cpuboard_model)
{
	const uae_u8 *autoconfig = apollo_autoconfig;
	if (cpuboard_model == 1200) {
		if (currprefs.cpu_model == 68060)
			autoconfig = apollo_autoconfig_cpuboard_12xx_060;
		else
			autoconfig = apollo_autoconfig_cpuboard_12xx;
	}

	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = autoconfig;
		return true;
	}

	struct ide_board *ide = getide(aci);

	if (!ide)
		return false;

	if (cpuboard_model) {
		// bit 0: scsi enable
		// bit 1: memory disable
		ide->userdata = currprefs.cpuboard_settings & 1;
	} else {
		ide->userdata = aci->rc->autoboot_disabled ? 2 : 0;
		ide->userdata |= 1;
		if (aci->rc->device_settings & 1) {
			ide->userdata &= ~1;
		}
	}

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->rom_size = 32768;
	ide->type = APOLLO_IDE;

	memset(ide->acmemory, 0xff, sizeof ide->acmemory);

	ide->rom = xcalloc(uae_u8, ide->rom_size);
	memset(ide->rom, 0xff, ide->rom_size);
	ide->rom_mask = ide->rom_size - 1;
	ide->keepautoconfig = false;
	for (int i = 0; i < 16; i++) {
		uae_u8 b = autoconfig[i];
		if (cpuboard_model && i == 9 && (currprefs.cpuboard_settings & 2))
			b |= 1; // memory disable (serial bit 0)
		ew(ide, i * 4, b);
	}
	if (cpuboard_model == 1200) {
		ide->mask = 131072 - 1;
		struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_CB_APOLLO_12xx);
		if (z) {
			int len = zfile_size32(z);
			// skip 68060 $f0 ROM block
			if (len >= 65536)
				zfile_fseek(z, 32768, SEEK_SET);
			for (int i = 0; i < 32768; i++) {
				uae_u8 b;
				zfile_fread(&b, 1, 1, z);
				ide->rom[i] = b;
			}
			zfile_fclose(z);
		}
	} else {
		ide->mask = 65536 - 1;
		load_rom_rc(aci->rc, ROMTYPE_APOLLO, 16384, 0, ide->rom, 32768, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);
	}
	aci->addrbank = ide->bank;
	return true;
}

bool apollo_init_hd(struct autoconfig_info *aci)
{
	return apollo_init(aci, 0);
}
bool apollo_init_cpu_12xx(struct autoconfig_info *aci)
{
	return apollo_init(aci, 1200);
}

void apollo_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, apollo_board, APOLLO_IDE, true, false, 2);
}

bool masoboshi_init(struct autoconfig_info *aci)
{
	int rom_size = 65536;
	uae_u8 *rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);

	load_rom_rc(aci->rc, ROMTYPE_MASOBOSHI, 32768, 0, rom, 65536, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);
	ide_add_reset();
	if (!aci->doinit) {
		if (aci->rc && aci->rc->autoboot_disabled)
			memcpy(aci->autoconfig_raw, rom + 0x100, sizeof aci->autoconfig_raw);
		else
			memcpy(aci->autoconfig_raw, rom + 0x000, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	if (!ide)
		return false;

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->type = MASOBOSHI_IDE;
	ide->rom_size = rom_size;
	ide->rom_mask = ide->mask = rom_size - 1;
	ide->rom = rom;
	ide->subtype = aci->rc->subtype;

	if (aci->rc && aci->rc->autoboot_disabled)
		memcpy(ide->acmemory, ide->rom + 0x100, sizeof ide->acmemory);
	else
		memcpy(ide->acmemory, ide->rom + 0x000, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	return true;
}

static void masoboshi_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, masoboshi_board, MASOBOSHI_IDE, true, false, 2);
}

void masoboshi_add_idescsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	if (ch < 0) {
		masoboshi_add_ide_unit(ch, ci, rc);
#ifdef NCR9X
		masoboshi_add_scsi_unit(ch, ci, rc);
#endif
	} else {
		if (ci->controller_type < HD_CONTROLLER_TYPE_SCSI_FIRST)
			masoboshi_add_ide_unit(ch, ci, rc);
#ifdef NCR9X
		else
			masoboshi_add_scsi_unit(ch, ci, rc);
#endif
	}
}

bool trifecta_init(struct autoconfig_info *aci)
{
	int rom_size = 65536;
	uae_u8 *rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);

	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = aci->ert->autoconfig;
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	if (!ide)
		return false;

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->type = TRIFECTA_IDE;
	ide->rom_size = rom_size;
	ide->rom_mask = ide->mask = rom_size - 1;
	ide->rom = rom;
	ide->subtype = aci->rc->subtype;
	ide->keepautoconfig = false;
	ide->intena = true;

	if (!aci->rc->autoboot_disabled)
		load_rom_rc(aci->rc, ROMTYPE_TRIFECTA, 32768, 0, rom, 65536, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);

	for (int i = 0; i < 16; i++) {
		uae_u8 b = aci->ert->autoconfig[i];
		ew(ide, i * 4, b);
	}

	aci->addrbank = ide->bank;
	return true;
}

static void trifecta_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, trifecta_board, TRIFECTA_IDE, true, false, 2);
}

void trifecta_add_idescsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	if (ch < 0) {
		trifecta_add_ide_unit(ch, ci, rc);
#ifdef NCR9X
		trifecta_add_scsi_unit(ch, ci, rc);
#endif
	} else {
		if (ci->controller_type < HD_CONTROLLER_TYPE_SCSI_FIRST)
			trifecta_add_ide_unit(ch, ci, rc);
#ifdef NCR9X
		else
			trifecta_add_scsi_unit(ch, ci, rc);
#endif
	}
}


static const uae_u8 adide_autoconfig[16] = { 0xd1, 0x02, 0x00, 0x00, 0x08, 0x17, 0x00, 0x00, 0x00, 0x00, ADIDE_ROM_OFFSET >> 8, ADIDE_ROM_OFFSET & 0xff };

bool adide_init(struct autoconfig_info *aci)
{
	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = adide_autoconfig;
		return true;
	}
	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->keepautoconfig = false;
	ide->bank = &ide_bank_generic;
	ide->rom_size = 32768;
	ide->mask = 65536 - 1;

	memset(ide->acmemory, 0xff, sizeof ide->acmemory);

	ide->rom = xcalloc(uae_u8, ide->rom_size);
	memset(ide->rom, 0xff, ide->rom_size);
	ide->rom_mask = ide->rom_size - 1;
	if (!aci->rc->autoboot_disabled) {
		load_rom_rc(aci->rc, ROMTYPE_ADIDE, 16384, 0, ide->rom, 32768, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);
	}
	for (int i = 0; i < 16; i++) {
		uae_u8 b = adide_autoconfig[i];
		ew(ide, i * 4, b);
	}
	aci->addrbank = ide->bank;
	return true;
}

void adide_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, adide_board, ADIDE_IDE, false, true, 2);
}

bool mtec_init(struct autoconfig_info *aci)
{
	uae_u8 *rom;
	int rom_size = 32768;

	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ROMTYPE_MTEC, 16384, !aci->rc->autoboot_disabled ? 16384 : 0, rom, 32768, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);

	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;

	ide->rom = rom;
	ide->rom_size = rom_size;
	ide->rom_mask = rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	return true;
}

void mtec_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, mtec_board, MTEC_IDE, false, false, 2);
}

bool rochard_init(struct autoconfig_info *aci)
{
	ide_add_reset();
	if (!aci->doinit) {
		load_rom_rc(aci->rc, ROMTYPE_ROCHARD, 8192, !aci->rc->autoboot_disabled ? 8192 : 0, aci->autoconfig_raw, sizeof aci->autoconfig_raw, 0);
		return true;
	}
	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->rom_size = 32768;
	ide->mask = 65536 - 1;
	ide->subtype = aci->rc->subtype;

	ide->rom = xcalloc(uae_u8, ide->rom_size);
	memset(ide->rom, 0xff, ide->rom_size);
	ide->rom_mask = ide->rom_size - 1;
	load_rom_rc(aci->rc, ROMTYPE_ROCHARD, 8192, !aci->rc->autoboot_disabled ? 8192 : 0, ide->rom, 16384, 0);
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);
	aci->addrbank = ide->bank;
	return true;
}

static void rochard_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, rochard_board, ROCHARD_IDE, true, false, 4);
}

bool buddha_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_BUDDHA);
	bool p1 = (aci->rc->device_settings & 3) == 1;

	ide_add_reset();
	if (!aci->doinit) {
		if (p1) {
			load_rom_rc(aci->rc, ROMTYPE_BUDDHA, 65536, 0, aci->autoconfig_raw, sizeof aci->autoconfig_raw, LOADROM_EVENONLY_ODDONE);
		} else {
			aci->autoconfigp = ert->autoconfig;
		}
		return true;
	}
	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->rom_size = 65536;
	ide->mask = 65536 - 1;

	ide->rom = xcalloc(uae_u8, ide->rom_size * 2);
	memset(ide->rom, 0xff, ide->rom_size * 2);
	ide->rom_mask = ide->rom_size - 1;
	ide->romfile = load_rom_rc_zfile(aci->rc, ROMTYPE_BUDDHA, 65536, 0, ide->rom, 65536, LOADROM_FILL);
	if (p1) {
		ide->flashrom = flash_new(ide->rom, 65536, 65536, 0xbf, 0x5d, ide->romfile, FLASHROM_PARALLEL_EEPROM | FLASHROM_DATA_PROTECT);
	}

	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		if (i == 1 && (aci->rc->device_settings & 3) == 2)
			b = 42;
		ew(ide, i * 4, b);
	}
	aci->addrbank = ide->bank;
	return true;
}

void buddha_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, buddha_board, BUDDHA_IDE, false, false, 6);
	if ((rc->device_settings & 3) == 1) {
		// 3rd port has no interrupt
		buddha_board[ci->controller_type_unit]->ide[2]->irq_inhibit = true;
	}
}

void rochard_add_idescsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	if (ch < 0) {
		rochard_add_ide_unit(ch, ci, rc);
		rochard_add_scsi_unit(ch, ci, rc);
	} else {
		if (ci->controller_type < HD_CONTROLLER_TYPE_SCSI_FIRST)
			rochard_add_ide_unit(ch, ci, rc);
		else
			rochard_add_scsi_unit(ch, ci, rc);
	}
}

bool golemfast_init(struct autoconfig_info *aci)
{
	int rom_size = 32768;
	uae_u8 *rom;
	
	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ROMTYPE_GOLEMFAST, 16384, 0, rom, 32768, 0);

	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->rom = rom;
	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->rom_size = rom_size;
	ide->mask = 65536 - 1;

	ide->rom_mask = ide->rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);
	aci->addrbank = ide->bank;
	return true;
}

static void golemfast_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, golemfast_board, GOLEMFAST_IDE, false, false, 2);
}

void golemfast_add_idescsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	if (ch < 0) {
		golemfast_add_ide_unit(ch, ci, rc);
#ifdef NCR9X
		golemfast_add_scsi_unit(ch, ci, rc);
#endif
	} else {
		if (ci->controller_type < HD_CONTROLLER_TYPE_SCSI_FIRST)
			golemfast_add_ide_unit(ch, ci, rc);
#ifdef NCR9X
		else
			golemfast_add_scsi_unit(ch, ci, rc);
#endif
	}
}

bool dataflyerplus_init(struct autoconfig_info *aci)
{
	int rom_size = 16384;
	uae_u8 *rom;

	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ROMTYPE_DATAFLYER, 32768, aci->rc->autoboot_disabled ? 8192 : 0, rom, 16384, LOADROM_EVENONLY_ODDONE);

	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->rom = rom;
	ide->configured = 0;
	ide->bank = &ide_bank_generic;
	ide->rom_size = rom_size;
	ide->mask = 65536 - 1;
	ide->keepautoconfig = false;
	ide->subtype = ((aci->rc->device_settings & 3) <= 1) ? 1 : 0; // scsi
	ide->subtype |= ((aci->rc->device_settings & 3) != 1) ? 2 : 0; // ide

	ide->rom_mask = ide->rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);
	aci->addrbank = ide->bank;

	return true;
}

static void dataflyerplus_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, dataflyerplus_board, DATAFLYERPLUS_IDE, true, false, 2);
}

void dataflyerplus_add_idescsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	if (ch < 0) {
		dataflyerplus_add_ide_unit(ch, ci, rc);
		dataflyerplus_add_scsi_unit(ch, ci, rc);
	} else {
		if (ci->controller_type < HD_CONTROLLER_TYPE_SCSI_FIRST)
			dataflyerplus_add_ide_unit(ch, ci, rc);
		else
			dataflyerplus_add_scsi_unit(ch, ci, rc);
	}
}

bool ateam_init(struct autoconfig_info *aci)
{
	uae_u8 *rom;
	int rom_size = 32768;

	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ROMTYPE_ATEAM, 16384, !aci->rc->autoboot_disabled ? 0xc000 : 0x8000, rom, 32768, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);

	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->keepautoconfig = false;
	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;

	ide->rom = rom;
	ide->rom_size = rom_size;
	ide->rom_mask = rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	return true;
}

void ateam_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, ateam_board, ATEAM_IDE, true, false, 2);
}

bool fastata4k_init(struct autoconfig_info *aci)
{
	uae_u8 *rom;
	int rom_size = 65536;

	if (aci->postinit) {
		struct ide_board *ide = getideboard(expamem_board_pointer);
		ide->baseaddress = expamem_board_pointer;
		ide->configured = 1;
		return true;
	}

	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_bytes, aci->ert->autoconfig, sizeof aci->ert->autoconfig);
		int type = aci->rc->device_settings & 3;
		if (type == 0)
			aci->autoconfig_bytes[1] = 25;
		else if (type == 1)
			aci->autoconfig_bytes[1] = 29;
		aci->autoconfigp = aci->autoconfig_bytes;
		return true;
	}

	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ROMTYPE_FASTATA4K, 65536, 0, rom, 65536, LOADROM_EVENONLY_ODDONE | LOADROM_FILL);

	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->keepautoconfig = false;
	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;

	ide->rom = rom;
	ide->rom_size = rom_size;
	ide->rom_mask = rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	aci->autoconfig_automatic = true;
	return true;
}

void fastata4k_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, fastata4k_board, FASTATA4K_IDE, false, false, 2);
}

bool arriba_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_ARRIBA);
	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;
	ide->rom_size = 32768;
	ide->rom_mask = 32768 - 1;

	ide->rom = xcalloc(uae_u8, 32768);
	load_rom_rc(aci->rc, ROMTYPE_ARRIBA, 32768, 0, ide->rom, 32768, LOADROM_FILL);

	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		if (i == 0 && aci->rc->autoboot_disabled)
			b &= ~0x10;
		ew(ide, i * 4, b);
	}

	aci->addrbank = ide->bank;
	return true;
}

void arriba_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, arriba_board, ARRIBA_IDE, false, false, 2);
}

bool elsathd_init(struct autoconfig_info *aci)
{
	uae_u8 *rom;
	int rom_size = 16384;

	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ELSATHD_IDE, 16384, 0, rom, 16384, LOADROM_FILL);
	if (aci->rc->autoboot_disabled)
		rom[0] &= ~0x10;

	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->keepautoconfig = false;
	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;

	ide->rom = rom;
	ide->rom_size = rom_size;
	ide->rom_mask = rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	return true;
}

void elsathd_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, elsathd_board, ELSATHD_IDE, true, false, 2);
}

bool accessx_init(struct autoconfig_info *aci)
{
	uae_u8 *rom;
	int rom_size = 32768;

	rom = xcalloc(uae_u8, rom_size);
	memset(rom, 0xff, rom_size);
	load_rom_rc(aci->rc, ROMTYPE_ACCESSX, 32768, 0, rom, 32768, LOADROM_FILL);
	uae_u8 rom2[32768];
	memcpy(rom2, rom, 32768);

	if ((rom[0] != 0xd0 && rom[1] != 0x10) && (rom[0] != 0xc0 && rom[1] != 0x10)) {
		// descramble
		if (aci->rc->subtype == 1) {
			// 2000 variant
			for (int i = 0; i < 32768; i += 2) {
				int addr = 0;
				addr |= ((i >> 7) & 1) << 0;
				addr |= ((i >> 10) & 1) << 1;
				addr |= ((i >> 9) & 1) << 2;
				addr |= ((i >> 8) & 1) << 3;
				addr |= ((i >> 1) & 1) << 4;
				addr |= ((i >> 2) & 1) << 5;
				addr |= ((i >> 3) & 1) << 6;
				addr |= ((i >> 4) & 1) << 7;
				addr |= ((i >> 13) & 1) << 8;
				addr |= ((i >> 14) & 1) << 9;
				if (!aci->rc->autoboot_disabled)
					addr |= 1 << 10;
				addr |= ((i >> 11) & 1) << 11;
				addr |= ((i >> 6) & 1) << 12;
				addr |= ((i >> 12) & 1) << 13;
				addr |= ((i >> 5) & 1) << 14;
				uae_u8 b = rom2[addr];
				uae_u8 v = 0;
				v |= ((b >> 4) & 1) << 1;
				v |= ((b >> 5) & 1) << 2;
				v |= ((b >> 6) & 1) << 0;
				v |= ((b >> 7) & 1) << 3;
				rom[i + 0] = v << 4;
				rom[i + 1] = 0xff;
			}

		} else {
			// 500 variant
			for (int i = 0; i < 16384; i++) {
				int addr = 0;
				addr |= ((i >> 5) & 1) << 0;
				addr |= ((i >> 4) & 1) << 1;
				addr |= ((i >> 3) & 1) << 2;
				addr |= ((i >> 6) & 1) << 3;
				addr |= ((i >> 1) & 1) << 4;
				addr |= ((i >> 0) & 1) << 5;
				addr |= ((i >> 9) & 1) << 6;
				addr |= ((i >> 11) & 1) << 7;
				addr |= ((i >> 10) & 1) << 8;
				addr |= ((i >> 8) & 1) << 9;
				addr |= ((i >> 2) & 1) << 10;
				addr |= ((i >> 7) & 1) << 11;
				addr |= ((i >> 13) & 1) << 12;
				addr |= ((i >> 12) & 1) << 13;
				if (aci->rc->autoboot_disabled)
					addr |= 1 << 14;
				uae_u8 b = rom2[addr];
				uae_u8 v = 0;
				v |= ((b >> 0) & 1) << 1;
				v |= ((b >> 1) & 1) << 2;
				v |= ((b >> 2) & 1) << 3;
				v |= ((b >> 3) & 1) << 0;
				rom[i * 2 + 0] = v << 4;
				rom[i * 2 + 1] = 0xff;
			}
		}
	} else {
		int offset = aci->rc->autoboot_disabled ? 16384 : 0;
		for (int i = 0; i < 16384; i++) {
			rom[i * 2 + 0] = rom2[i + offset];
			rom[i * 2 + 1] = 0xff;
		}
	}


	ide_add_reset();
	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->configured = 0;
	ide->intena = true;
	ide->intlev6 = true;
	ide->keepautoconfig = false;
	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;

	ide->rom = rom;
	ide->rom_size = rom_size;
	ide->rom_mask = rom_size - 1;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	return true;
}

void accessx_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, accessx_board, ACCESSX_IDE, false, false, 4);
}

bool trumpcard500at_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_IVST500AT);
	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;
	ide->rom_size = 32768;
	ide->rom_mask = 32768 - 1;
	ide->keepautoconfig = false;
	ide->intena = true;

	ide->rom = xcalloc(uae_u8, 32768);
	load_rom_rc(aci->rc, ROMTYPE_IVST500AT, 16384, 0, ide->rom, 32768, LOADROM_EVENONLY_ODDONE);

	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		if (i == 0 && aci->rc->autoboot_disabled)
			b &= ~0x10;
		ew(ide, i * 4, b);
	}

	aci->addrbank = ide->bank;
	return true;
}

void trumpcard500at_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, ivst500at_board, IVST500AT_IDE, true, false, 2);
}


bool tandem_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_TANDEM);
	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	struct ide_board *ide = getide(aci);

	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;

	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(ide, i * 4, b);
	}

	aci->addrbank = ide->bank;
	return true;
}

void tandem_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, tandem_board, TANDEM_IDE, false, false, 2);
}

bool dotto_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_DOTTO);
	ide_add_reset();

	uae_u8 *rom = xcalloc(uae_u8, 65536);
	load_rom_rc(aci->rc, ROMTYPE_DOTTO, 32768, 0, rom, 65536, LOADROM_EVENONLY_ODDONE);

	if (!aci->doinit) {
		memcpy(aci->autoconfig_raw, rom, sizeof aci->autoconfig_raw);
		xfree(rom);
		return true;
	}
	struct ide_board *ide = getide(aci);

	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;
	ide->rom_size = 65536;
	ide->rom_mask = 65536 - 1;
	ide->keepautoconfig = false;

	ide->rom = rom;
	memcpy(ide->acmemory, ide->rom, sizeof ide->acmemory);

	aci->addrbank = ide->bank;
	return true;
}

void dotto_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, dotto_board, DOTTO_IDE, false, true, 2);
}

static const uae_u8 dev_autoconfig[16] = { 0xd1, 1, 0x00, 0x00, 0x77, 0x77, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };

bool dev_hd_init(struct autoconfig_info *aci)
{
	bool ac = true;
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_DEVHD);
	ide_add_reset();

	uae_u8 *rom = xcalloc(uae_u8, 262144);
	load_rom_rc(aci->rc, ROMTYPE_DEVHD, 131072, 0, rom, 262144, LOADROM_EVENONLY_ODDONE);
	memmove(rom + 0x8000, rom, 262144 - 0x8000);

	if (!ac) {
		// fake
		aci->start = 0xe90000;
		aci->size = 0x10000;
	}
	dev_hd_mode = 1;
	dev_hd_io_base = 0x4000;
	dev_hd_io_size = 4;
	dev_hd_data_base = 0x4800;
	dev_hd_io_secondary = 0x1000;
	dev_hd_io_total = 8 * 4;

	if (!aci->doinit) {
		if (ac) {
			aci->autoconfigp = dev_autoconfig;
		}
		xfree(rom);
		return true;
	}

	struct ide_board *ide = getide(aci);

	if (ac) {
		for (int i = 0; i < 16; i++) {
			uae_u8 b = dev_autoconfig[i];
			ew(ide, i * 4, b);
		}
	}

	ide->bank = &ide_bank_generic;
	ide->mask = 65536 - 1;
	ide->keepautoconfig = true;
	ide->rom = rom;
	memcpy(ide->rom, ide->acmemory, 128);

	if (!ac) {
		ide->baseaddress = aci->start;
		ide->configured = 1;
		map_banks(ide->bank, aci->start >> 16, aci->size >> 16, 0);
	}

	aci->addrbank = ide->bank;
	return true;
}

void dev_hd_add_ide_unit(int ch, struct uaedev_config_info* ci, struct romconfig* rc)
{
	add_ide_standard_unit(ch, ci, rc, dev_board, DEV_IDE, false, true, 2);
}

bool ripple_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_RIPPLE);

	ide_add_reset();
	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	struct ide_board *ide = getide(aci);
	if (!ide)
		return false;

	ide->configured     = 0;
	ide->bank           = &ide_bank_generic;
	ide->type           = RIPPLE_IDE;
	ide->rom_size       = 131072;
	ide->userdata       = 0;
	ide->intena         = false;
	ide->mask           = 131072 - 1;
	ide->keepautoconfig = false;

	ide->rom = xcalloc(uae_u8, ide->rom_size);
	if (ide->rom) {
		memset(ide->rom, 0xff, ide->rom_size);
	}

	ide->rom_mask = ide->rom_size - 1;

	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		if (i == 0 && aci->rc->autoboot_disabled)
			b &= ~0x10;
		ew(ide, i * 4, b);
	}

	load_rom_rc(aci->rc, ROMTYPE_RIPPLE, 65536, 0, ide->rom, 131072, LOADROM_EVENONLY_ODDONE);

	aci->addrbank = ide->bank;
	return true;
}

void ripple_add_ide_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, ripple_board, RIPPLE_IDE, false, false, 4);
}

#ifdef WITH_X86
extern void x86_xt_ide_bios(struct zfile*, struct romconfig*);
static bool x86_at_hd_init(struct autoconfig_info *aci, int type)
{
	static const int parent[] = { ROMTYPE_A1060, ROMTYPE_A2088, ROMTYPE_A2088T, ROMTYPE_A2286, ROMTYPE_A2386, 0 };
	aci->parent_romtype = parent;
	ide_add_reset();
	if (!aci->doinit)
		return true;

	struct ide_board *ide = getide(aci);
	if (!ide)
		return false;

	ide->intena = type == 0;
	ide->configured = 1;
	ide->bank = &ide_bank_generic;

	struct zfile *f = read_device_from_romconfig(aci->rc, 0);
	if (f) {
		x86_xt_ide_bios(f, aci->rc);
		zfile_fclose(f);
	}
	return true;
}
bool x86_at_hd_init_1(struct autoconfig_info *aci)
{
	return x86_at_hd_init(aci, 0);
}
bool x86_at_hd_init_xt(struct autoconfig_info *aci)
{
	return x86_at_hd_init(aci, 1);
}

void x86_add_at_hd_unit_1(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, &x86_at_ide_board[0], x86_AT_IDE + 0, false, false, 2);
}
void x86_add_at_hd_unit_xt(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	add_ide_standard_unit(ch, ci, rc, &x86_at_ide_board[1], x86_AT_IDE + 1, false, false, 2);
}

static int x86_ide_reg(int portnum, int *unit)
{
	if (portnum >= 0x1f0 && portnum < 0x1f8) {
		*unit = 0;
		return portnum & 7;
	}
	if (portnum == 0x3f6) {
		*unit = 0;
		return 6 | IDE_SECONDARY;
	}
	if (portnum >= 0x300 && portnum < 0x310) {
		*unit = 1;
		if (portnum < 0x308)
			return portnum & 7;
		if (portnum == 0x308)
			return 8;
		if (portnum >= 0x308+6)
			return (portnum & 7) | IDE_SECONDARY;
	}
	return -1;
}

void x86_ide_hd_put(int portnum, uae_u16 v, int size)
{

	if (portnum < 0) {
		for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
			struct ide_board *board = x86_at_ide_board[i];
			if (board)
				ide_reset_device(board->ide[0]);
		}
		return;
	}
	int unit;
	int regnum = x86_ide_reg(portnum, &unit);
	if (regnum >= 0) {
		struct ide_board *board = x86_at_ide_board[unit];
		if (board) {
			if (size == 0) {
				if (get_ide_is_8bit(board)) {
					v = get_ide_reg_8bitdata(board, regnum);
				} else {
					if (regnum == 8) {
						board->data_latch = v;
					} else if (regnum == 0) {
						v <<= 8;
						v |= board->data_latch;
						put_ide_reg(board, regnum, v);
					} else {
						put_ide_reg_8bitdata(board, regnum, v);
					}
				}
			} else {
				put_ide_reg(board, regnum, (v >> 8) | (v << 8));
			}
		}
	}
}
uae_u16 x86_ide_hd_get(int portnum, int size)
{
	uae_u16 v = 0;
	int unit;
	int regnum = x86_ide_reg(portnum, &unit);
	if (regnum >= 0) {
		struct ide_board *board = x86_at_ide_board[unit];
		if (board) {

			if (size == 0) {
				if (get_ide_is_8bit(board)) {
					v = get_ide_reg_8bitdata(board, regnum);
				} else {
					if (regnum == 0) {
						board->data_latch = get_ide_reg(board, regnum);
						v = board->data_latch >> 8;
					} else if (regnum == 8) {
						v = board->data_latch;
					} else {
						v = get_ide_reg_8bitdata(board, regnum & 7);
					}
				}
			} else {
				v = get_ide_reg(board, regnum);
				v = (v >> 8) | (v << 8);
			}
		}
	}
	return v;
}
#endif
