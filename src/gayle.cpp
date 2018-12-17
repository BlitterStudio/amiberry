/*
* UAE - The Un*x Amiga Emulator
*
* Gayle (and motherboard resources) memory bank
*
* (c) 2006 - 2015 Toni Wilen
*/

#define GAYLE_LOG 0
#define MBRES_LOG 0
#define PCMCIA_LOG 0

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "sysdeps.h"
#include "options.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "filesys.h"
#include "gayle.h"
#include "savestate.h"
#include "uae.h"
#include "threaddep/thread.h"
#include "ide.h"
#include "autoconf.h"

#define PCMCIA_SRAM 1
#define PCMCIA_IDE 2

/*
600000 to 9FFFFF	4 MB	Credit Card memory if CC present
A00000 to A1FFFF	128 KB	Credit Card Attributes
A20000 to A3FFFF	128 KB	Credit Card I/O
A40000 to A5FFFF	128 KB	Credit Card Bits
A60000 to A7FFFF	128 KB	PC I/O

D80000 to D8FFFF	64 KB SPARE chip select
D90000 to D9FFFF	64 KB ARCNET chip select
DA0000 to DA3FFF	16 KB IDE drive
DA4000 to DA4FFF	16 KB IDE reserved
DA8000 to DAFFFF	32 KB Credit Card and IDE configregisters
DB0000 to DBFFFF	64 KB Not used (reserved for external IDE)
* DC0000 to DCFFFF	64 KB Real Time Clock (RTC)
DD0000 to DDFFFF	64 KB A3000 DMA controller
DD0000 to DD1FFF        A4000 DMAC
DD2000 to DDFFFF        A4000 IDE
DE0000 to DEFFFF	64 KB Motherboard resources
*/

/* Gayle definitions from Linux drivers and preliminary Gayle datasheet */

/* PCMCIA stuff */

#define GAYLE_RAM               0x600000
#define GAYLE_RAMSIZE           0x400000
#define GAYLE_ATTRIBUTE         0xa00000
#define GAYLE_ATTRIBUTESIZE     0x020000
#define GAYLE_IO                0xa20000     /* 16bit and even 8bit registers */
#define GAYLE_IOSIZE            0x010000
#define GAYLE_IO_8BITODD        0xa30000     /* odd 8bit registers */

#define GAYLE_ADDRESS   0xda8000      /* gayle main registers base address */
#define GAYLE_RESET     0xa40000      /* write 0x00 to start reset, read 1 byte to stop reset */

/*  Bases of the IDE interfaces */
#define GAYLE_BASE_4000 0xdd2020    /* A4000/A4000T */
#define GAYLE_BASE_1200 0xda0000    /* A1200/A600 and E-Matrix 530 */

/*
*  These are at different offsets from the base
*/
#define GAYLE_IRQ_4000  0x3020    /* WORD register MSB = 1, Harddisk is source of interrupt */
#define GAYLE_CS_1200	0x8000
#define GAYLE_IRQ_1200  0x9000
#define GAYLE_INT_1200  0xA000
#define GAYLE_CFG_1200	0xB000

/* DA8000 */
#define GAYLE_CS_IDE	0x80	/* IDE int status */
#define GAYLE_CS_CCDET	0x40    /* credit card detect */
#define GAYLE_CS_BVD1	0x20    /* battery voltage detect 1 */
#define GAYLE_CS_SC	0x20    /* credit card status change */
#define GAYLE_CS_BVD2	0x10    /* battery voltage detect 2 */
#define GAYLE_CS_DA	0x10    /* digital audio */
#define GAYLE_CS_WR	0x08    /* write enable (1 == enabled) */
#define GAYLE_CS_BSY	0x04    /* credit card busy */
#define GAYLE_CS_IRQ	0x04    /* interrupt request */
#define GAYLE_CS_DAEN   0x02    /* enable digital audio */ 
#define GAYLE_CS_DIS    0x01    /* disable PCMCIA slot */ 

/* DA9000 */
#define GAYLE_IRQ_IDE	    0x80
#define GAYLE_IRQ_CCDET	    0x40    /* credit card detect */
#define GAYLE_IRQ_BVD1	    0x20    /* battery voltage detect 1 */
#define GAYLE_IRQ_SC	    0x20    /* credit card status change */
#define GAYLE_IRQ_BVD2	    0x10    /* battery voltage detect 2 */
#define GAYLE_IRQ_DA	    0x10    /* digital audio */
#define GAYLE_IRQ_WR	    0x08    /* write enable (1 == enabled) */
#define GAYLE_IRQ_BSY	    0x04    /* credit card busy */
#define GAYLE_IRQ_IRQ	    0x04    /* interrupt request */
#define GAYLE_IRQ_RESET	    0x02    /* reset machine after CCDET change */ 
#define GAYLE_IRQ_BERR      0x01    /* generate bus error after CCDET change */ 

/* DAA000 */
#define GAYLE_INT_IDE	    0x80    /* IDE interrupt enable */
#define GAYLE_INT_CCDET	    0x40    /* credit card detect change enable */
#define GAYLE_INT_BVD1	    0x20    /* battery voltage detect 1 change enable */
#define GAYLE_INT_SC	    0x20    /* credit card status change enable */
#define GAYLE_INT_BVD2	    0x10    /* battery voltage detect 2 change enable */
#define GAYLE_INT_DA	    0x10    /* digital audio change enable */
#define GAYLE_INT_WR	    0x08    /* write enable change enabled */
#define GAYLE_INT_BSY	    0x04    /* credit card busy */
#define GAYLE_INT_IRQ	    0x04    /* credit card interrupt request */
#define GAYLE_INT_BVD_LEV   0x02    /* BVD int level, 0=lev2,1=lev6 */ 
#define GAYLE_INT_BSY_LEV   0x01    /* BSY int level, 0=lev2,1=lev6 */ 

/* 0xDAB000 GAYLE_CONFIG */
#define GAYLE_CFG_0V            0x00
#define GAYLE_CFG_5V            0x01
#define GAYLE_CFG_12V           0x02
#define GAYLE_CFG_100NS         0x08
#define GAYLE_CFG_150NS         0x04
#define GAYLE_CFG_250NS         0x00
#define GAYLE_CFG_720NS         0x0c

#define TOTAL_IDE 3
#define GAYLE_IDE_ID 0
#define PCMCIA_IDE_ID 2

static struct ide_hdf *idedrive[TOTAL_IDE * 2];
struct hd_hardfiledata *pcmcia_sram;

static int pcmcia_card;
static int pcmcia_readonly;
static int pcmcia_type;
static uae_u8 pcmcia_configuration[20];
static int pcmcia_configured;

static int gayle_id_cnt;
static uae_u8 gayle_irq, gayle_int, gayle_cs, gayle_cs_mask, gayle_cfg;
static int ide_splitter;

static struct ide_thread_state gayle_its;

static void pcmcia_reset (void)
{
	memset (pcmcia_configuration, 0, sizeof pcmcia_configuration);
	pcmcia_configured = -1;
	if (PCMCIA_LOG > 0)
		write_log (_T("PCMCIA reset\n"));
}

static uae_u8 checkpcmciaideirq (void)
{
	if (!idedrive[PCMCIA_IDE_ID * 2] || pcmcia_type != PCMCIA_IDE || pcmcia_configured < 0)
		return 0;
	if (!idedrive[PCMCIA_IDE_ID * 2]->regs0 || (idedrive[PCMCIA_IDE_ID * 2]->regs0->ide_devcon & 2))
		return 0;
	if (idedrive[PCMCIA_IDE_ID * 2]->irq)
		return GAYLE_IRQ_BSY;
	return 0;
}

static uae_u8 checkgayleideirq (void)
{
	int i;
	bool irq = false;

	for (i = 0; i < 2; i++) {
		if (idedrive[i]) {
			if (!(idedrive[i]->regs.ide_devcon & 2) && (idedrive[i]->irq || (idedrive[i + 2] && idedrive[i + 2]->irq)))
				irq = true;
			/* IDE killer feature. Do not eat interrupt to make booting faster. */
			if (idedrive[i]->irq && !ide_isdrive (idedrive[i]))
				idedrive[i]->irq = 0;
			if (idedrive[i + 2] && idedrive[i + 2]->irq && !ide_isdrive (idedrive[i + 2]))
				idedrive[i + 2]->irq = 0;
		}
	}
	return irq ? GAYLE_IRQ_IDE : 0;
}

void rethink_gayle (void)
{
	int lev2 = 0;
	int lev6 = 0;
	uae_u8 mask;

	if (currprefs.cs_ide == IDE_A4000) {
		gayle_irq |= checkgayleideirq ();
		if ((gayle_irq & GAYLE_IRQ_IDE) && !(intreq & 0x0008))
			INTREQ_0 (0x8000 | 0x0008);
		return;
	}

	if (currprefs.cs_ide != IDE_A600A1200 && !currprefs.cs_pcmcia)
		return;
	gayle_irq |= checkgayleideirq();
	gayle_irq |= checkpcmciaideirq();
	mask = gayle_int & gayle_irq;
	if (mask & (GAYLE_IRQ_IDE | GAYLE_IRQ_WR))
		lev2 = 1;
	if (mask & GAYLE_IRQ_CCDET)
		lev6 = 1;
	if (mask & (GAYLE_IRQ_BVD1 | GAYLE_IRQ_BVD2)) {
		if (gayle_int & GAYLE_INT_BVD_LEV)
			lev6 = 1;
		else
			lev2 = 1;
	}
	if (mask & GAYLE_IRQ_BSY) {
		if (gayle_int & GAYLE_INT_BSY_LEV)
			lev6 = 1;
		else
			lev2 = 1;
	}
	if (lev2 && !(intreq & 0x0008))
		INTREQ_0 (0x8000 | 0x0008);
	if (lev6 && !(intreq & 0x2000))
		INTREQ_0 (0x8000 | 0x2000);
}

static void gayle_cs_change (uae_u8 mask, int onoff)
{
	int changed = 0;
	if ((gayle_cs & mask) && !onoff) {
		gayle_cs &= ~mask;
		changed = 1;
	} else if (!(gayle_cs & mask) && onoff) {
		gayle_cs |= mask;
		changed = 1;
	}
	if (changed) {
		gayle_irq |= mask;
		rethink_gayle ();
		if ((mask & GAYLE_CS_CCDET) && (gayle_irq & (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR)) != (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR)) {
			if (gayle_irq & GAYLE_IRQ_RESET)
				uae_reset (0, 0);
			if (gayle_irq & GAYLE_IRQ_BERR)
				Exception (2);
		}
	}
}

static void card_trigger (int insert)
{
	if (insert) {
		if (pcmcia_card) {
			gayle_cs_change (GAYLE_CS_CCDET, 1);
			gayle_cfg = GAYLE_CFG_100NS;
			if (!pcmcia_readonly)
				gayle_cs_change (GAYLE_CS_WR, 1);
		}
	} else {
		gayle_cfg = 0;
		gayle_cs_change (GAYLE_CS_CCDET, 0);
		gayle_cs_change (GAYLE_CS_BVD2, 0);
		gayle_cs_change (GAYLE_CS_BVD1, 0);
		gayle_cs_change (GAYLE_CS_WR, 0);
		gayle_cs_change (GAYLE_CS_BSY, 0);
	}
	rethink_gayle ();
}

static void write_gayle_cfg (uae_u8 val)
{
	gayle_cfg = val;
}
static uae_u8 read_gayle_cfg (void)
{
	return gayle_cfg & 0x0f;
}
static void write_gayle_irq (uae_u8 val)
{
	gayle_irq = (gayle_irq & val) | (val & (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR));
	if ((gayle_irq & (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR)) == (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR))
		pcmcia_reset ();
}
static uae_u8 read_gayle_irq (void)
{
	return gayle_irq;
}
static void write_gayle_int (uae_u8 val)
{
	gayle_int = val;
}
static uae_u8 read_gayle_int (void)
{
	return gayle_int;
}
static void write_gayle_cs (uae_u8 val)
{
	int ov = gayle_cs;

	gayle_cs_mask = val & ~3;
	gayle_cs &= ~3;
	gayle_cs |= val & 3;
	if ((ov & 1) != (gayle_cs & 1)) {
		gayle_map_pcmcia ();
		/* PCMCIA disable -> enable */
		card_trigger (!(gayle_cs & GAYLE_CS_DIS) ? 1 : 0);
		if (PCMCIA_LOG)
			write_log (_T("PCMCIA slot: %s PC=%08X\n"), !(gayle_cs & 1) ? _T("enabled") : _T("disabled"), M68K_GETPC);
	}
}
static uae_u8 read_gayle_cs (void)
{
	uae_u8 v;

	v = gayle_cs_mask | gayle_cs;
	v |= checkgayleideirq ();
	v |= checkpcmciaideirq ();
	return v;
}

static int get_gayle_ide_reg (uaecptr addr, struct ide_hdf **ide)
{
	int ide2;
	addr &= 0xffff;
	*ide = NULL;
	if (addr >= GAYLE_IRQ_4000 && addr <= GAYLE_IRQ_4000 + 1 && currprefs.cs_ide == IDE_A4000)
		return -1;
	addr &= ~0x2020;
	addr >>= 2;
	ide2 = 0;
	if (addr & IDE_SECONDARY) {
		if (ide_splitter) {
			ide2 = 2;
			addr &= ~IDE_SECONDARY;
		}
	}
	*ide = idedrive[ide2 + idedrive[ide2]->ide_drv];
	return addr;
}


static uae_u32 gayle_read2 (uaecptr addr)
{
	struct ide_hdf *ide = NULL;
	int ide_reg;

	addr &= 0xffff;
	if ((GAYLE_LOG > 3 && (addr != 0x2000 && addr != 0x2001 && addr != 0x3020 && addr != 0x3021 && addr != GAYLE_IRQ_1200)) || GAYLE_LOG > 5)
		write_log (_T("IDE_READ %08X PC=%X\n"), addr, M68K_GETPC);
	if (currprefs.cs_ide <= 0) {
		if (addr == 0x201c) // AR1200 IDE detection hack
			return 0x7f;
		return 0xff;
	}
	if (addr >= GAYLE_IRQ_4000 && addr <= GAYLE_IRQ_4000 + 1 && currprefs.cs_ide == IDE_A4000) {
		uae_u8 v = gayle_irq;
		gayle_irq = 0;
		return v;
	}
	if (addr >= 0x4000) {
		if (addr == GAYLE_IRQ_1200) {
			if (currprefs.cs_ide == IDE_A600A1200)
				return read_gayle_irq ();
			return 0;
		} else if (addr == GAYLE_INT_1200) {
			if (currprefs.cs_ide == IDE_A600A1200)
				return read_gayle_int ();
			return 0;
		}
		return 0;
	}
	ide_reg = get_gayle_ide_reg (addr, &ide);
	/* Emulated "ide killer". Prevents long KS boot delay if no drives installed */
	if (!ide_isdrive (idedrive[0]) && !ide_isdrive (idedrive[1]) && !ide_isdrive (idedrive[2]) && !ide_isdrive (idedrive[3])) {
		if (ide_reg == IDE_STATUS)
			return 0x7f;
		return 0xff;
	}
	return ide_read_reg (ide, ide_reg);
}

static void gayle_write2 (uaecptr addr, uae_u32 val)
{
	struct ide_hdf *ide = NULL;
	int ide_reg;

	if ((GAYLE_LOG > 3 && (addr != 0x2000 && addr != 0x2001 && addr != 0x3020 && addr != 0x3021 && addr != GAYLE_IRQ_1200)) || GAYLE_LOG > 5)
		write_log (_T("IDE_WRITE %08X=%02X PC=%X\n"), addr, (uae_u32)val & 0xff, M68K_GETPC);
	if (currprefs.cs_ide <= 0)
		return;
	if (currprefs.cs_ide == IDE_A600A1200) {
		if (addr == GAYLE_IRQ_1200) {
			write_gayle_irq (val);
			return;
		}
		if (addr == GAYLE_INT_1200) {
			write_gayle_int (val);
			return;
		}
	}
	if (addr >= 0x4000)
		return;
	ide_reg = get_gayle_ide_reg (addr, &ide);
	ide_write_reg (ide, ide_reg, val);
}

static int gayle_read (uaecptr addr)
{
	uaecptr oaddr = addr;
	uae_u32 v = 0;
	int got = 0;
	if (currprefs.cs_ide == IDE_A600A1200) {
		if ((addr & 0xA0000) != 0xA0000)
			return 0;
	}
	addr &= 0xffff;
	if (currprefs.cs_pcmcia) {
		if (currprefs.cs_ide != IDE_A600A1200) {
			if (addr == GAYLE_IRQ_1200) {
				v = read_gayle_irq ();
				got = 1;
			} else if (addr == GAYLE_INT_1200) {
				v = read_gayle_int ();
				got = 1;
			}
		}
		if (addr == GAYLE_CS_1200) {
			v = read_gayle_cs ();
			got = 1;
			if (PCMCIA_LOG)
				write_log (_T("PCMCIA STATUS READ %08X=%02X PC=%08X\n"), oaddr, (uae_u32)v & 0xff, M68K_GETPC);
		} else if (addr == GAYLE_CFG_1200) {
			v = read_gayle_cfg ();
			got = 1;
			if (PCMCIA_LOG)
				write_log (_T("PCMCIA CONFIG READ %08X=%02X PC=%08X\n"), oaddr, (uae_u32)v & 0xff, M68K_GETPC);
		}
	}
	if (!got)
		v = gayle_read2 (addr);
	if (GAYLE_LOG)
		write_log (_T("GAYLE_READ %08X=%02X PC=%08X\n"), oaddr, (uae_u32)v & 0xff, M68K_GETPC);
	return v;
}

static void gayle_write (uaecptr addr, int val)
{
	uaecptr oaddr = addr;
	int got = 0;
	if (currprefs.cs_ide == IDE_A600A1200) {
		if ((addr & 0xA0000) != 0xA0000)
			return;
	}
	addr &= 0xffff;
	if (currprefs.cs_pcmcia) {
		if (currprefs.cs_ide != IDE_A600A1200) {
			if (addr == GAYLE_IRQ_1200) {
				write_gayle_irq (val);
				got = 1;
			} else if (addr == GAYLE_INT_1200) {
				write_gayle_int (val);
				got = 1;
			}
		}
		if (addr == GAYLE_CS_1200) {
			write_gayle_cs (val);
			got = 1;
			if (PCMCIA_LOG > 1)
				write_log (_T("PCMCIA STATUS WRITE %08X=%02X PC=%08X\n"), oaddr, (uae_u32)val & 0xff, M68K_GETPC);
		} else if (addr == GAYLE_CFG_1200) {
			write_gayle_cfg (val);
			got = 1;
			if (PCMCIA_LOG > 1)
				write_log (_T("PCMCIA CONFIG WRITE %08X=%02X PC=%08X\n"), oaddr, (uae_u32)val & 0xff, M68K_GETPC);
		}
	}

	if (GAYLE_LOG)
		write_log (_T("GAYLE_WRITE %08X=%02X PC=%08X\n"), oaddr, (uae_u32)val & 0xff, M68K_GETPC);
	if (!got)
		gayle_write2 (addr, val);
}

DECLARE_MEMORY_FUNCTIONS(gayle);
addrbank gayle_bank = {
	gayle_lget, gayle_wget, gayle_bget,
	gayle_lput, gayle_wput, gayle_bput,
	default_xlate, default_check, NULL, NULL, _T("Gayle (low)"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

static uae_u32 REGPARAM2 gayle_lget (uaecptr addr)
{
	struct ide_hdf *ide = NULL;
	int ide_reg;
	uae_u32 v;
	ide_reg = get_gayle_ide_reg (addr, &ide);
	if (ide_reg == IDE_DATA) {
		v = ide_get_data (ide) << 16;
		v |= ide_get_data (ide);
		if (GAYLE_LOG > 4)
			write_log(_T("IDE_DATA_LONG %08X=%08X PC=%X\n"), addr, v, M68K_GETPC);
		return v;
	}
	v = gayle_wget (addr) << 16;
	v |= gayle_wget (addr + 2);
	return v;
}
static uae_u32 REGPARAM2 gayle_wget (uaecptr addr)
{
	struct ide_hdf *ide = NULL;
	int ide_reg;
	uae_u32 v;
	ide_reg = get_gayle_ide_reg (addr, &ide);
	if (ide_reg == IDE_DATA) {
		v = ide_get_data (ide);
		if (GAYLE_LOG > 4)
			write_log(_T("IDE_DATA_WORD %08X=%04X PC=%X\n"), addr, v & 0xffff, M68K_GETPC);
		return v;
	}
	v = gayle_bget (addr) << 8;
	v |= gayle_bget (addr + 1);
	return v;
}
static uae_u32 REGPARAM2 gayle_bget (uaecptr addr)
{
	uae_u32 v;
	v = gayle_read (addr);
	return v;
}

static void REGPARAM2 gayle_lput (uaecptr addr, uae_u32 value)
{
	struct ide_hdf *ide = NULL;
	int ide_reg;
	ide_reg = get_gayle_ide_reg (addr, &ide);
	if (ide_reg == IDE_DATA) {
		ide_put_data (ide, value >> 16);
		ide_put_data (ide, value & 0xffff);
		return;
	}
	gayle_wput (addr, value >> 16);
	gayle_wput (addr + 2, value & 0xffff);
}
static void REGPARAM2 gayle_wput (uaecptr addr, uae_u32 value)
{
	struct ide_hdf *ide = NULL;
	int ide_reg;
	ide_reg = get_gayle_ide_reg (addr, &ide);
	if (ide_reg == IDE_DATA) {
		ide_put_data (ide, value);
		return;
	}
	gayle_bput (addr, value >> 8);
	gayle_bput (addr + 1, value & 0xff);
}

static void REGPARAM2 gayle_bput (uaecptr addr, uae_u32 value)
{
	gayle_write (addr, value);
}

static void gayle2_write (uaecptr addr, uae_u32 v)
{
	gayle_id_cnt = 0;
}

static uae_u32 gayle2_read (uaecptr addr)
{
	uae_u8 v = 0;
	addr &= 0xffff;
	if (addr == 0x1000) {
		/* Gayle ID. Gayle = 0xd0. AA Gayle = 0xd1 */
		if (gayle_id_cnt == 0 || gayle_id_cnt == 1 || gayle_id_cnt == 3 || ((currprefs.chipset_mask & CSMASK_AGA) && gayle_id_cnt == 7) ||
			(currprefs.cs_cd32cd && !currprefs.cs_ide && !currprefs.cs_pcmcia && gayle_id_cnt == 2))
			v = 0x80;
		else
			v = 0x00;
		gayle_id_cnt++;
	}
	return v;
}

DECLARE_MEMORY_FUNCTIONS(gayle2);
addrbank gayle2_bank = {
	gayle2_lget, gayle2_wget, gayle2_bget,
	gayle2_lput, gayle2_wput, gayle2_bput,
	default_xlate, default_check, NULL, NULL, _T("Gayle (high)"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

static uae_u32 REGPARAM2 gayle2_lget (uaecptr addr)
{
	uae_u32 v;
	v = gayle2_wget (addr) << 16;
	v |= gayle2_wget (addr + 2);
	return v;
}
static uae_u32 REGPARAM2 gayle2_wget (uaecptr addr)
{
	uae_u16 v;
	v = gayle2_bget (addr) << 8;
	v |= gayle2_bget (addr + 1);
	return v;
}
static uae_u32 REGPARAM2 gayle2_bget (uaecptr addr)
{
	return gayle2_read (addr);
}

static void REGPARAM2 gayle2_lput (uaecptr addr, uae_u32 value)
{
	gayle2_wput (addr, value >> 16);
	gayle2_wput (addr + 2, value & 0xffff);
}

static void REGPARAM2 gayle2_wput (uaecptr addr, uae_u32 value)
{
	gayle2_bput (addr, value >> 8);
	gayle2_bput (addr + 1, value & 0xff);
}

static void REGPARAM2 gayle2_bput (uaecptr addr, uae_u32 value)
{
	gayle2_write (addr, value);
}

static uae_u8 ramsey_config;
static int garyidoffset;
static int gary_coldboot;
int gary_timeout;
int gary_toenb;

static void mbres_write (uaecptr addr, uae_u32 val, int size)
{
	addr &= 0xffff;
	if (MBRES_LOG > 0)
		write_log (_T("MBRES_WRITE %08X=%08X (%d) PC=%08X S=%d\n"), addr, val, size, M68K_GETPC, regs.s);
	if (addr < 0x8000 && (1 || regs.s)) { /* CPU FC = supervisor only */
		uae_u32 addr2 = addr & 3;
		uae_u32 addr64 = (addr >> 6) & 3;
		if (addr == 0x1002)
			garyidoffset = -1;
		if (addr64 == 0 && addr2 == 0x03)
			ramsey_config = val;
		if (addr2 == 0x02)
			gary_coldboot = (val & 0x80) ? 1 : 0;
		if (addr2 == 0x01)
			gary_toenb = (val & 0x80) ? 1 : 0;
		if (addr2 == 0x00)
			gary_timeout = (val & 0x80) ? 1 : 0;
	}
}

static uae_u32 mbres_read (uaecptr addr, int size)
{
	uae_u32 v = 0;

	addr &= 0xffff;

	if (1 || regs.s) { /* CPU FC = supervisor only (only newest ramsey/gary? never implemented?) */
		uae_u32 addr2 = addr & 3;
		uae_u32 addr64 = (addr >> 6) & 3;
		/* Gary ID (I don't think this exists in real chips..) */
		if (addr == 0x1002 && currprefs.cs_fatgaryrev >= 0) {
			garyidoffset++;
			garyidoffset &= 7;
			v = (currprefs.cs_fatgaryrev << garyidoffset) & 0x80;
		}
		for (;;) {
			if (addr64 == 1 && addr2 == 0x03) { /* RAMSEY revision */
				if (currprefs.cs_ramseyrev >= 0)
					v = currprefs.cs_ramseyrev;
				break;
			}
			if (addr64 == 0 && addr2 == 0x03) { /* RAMSEY config */
				if (currprefs.cs_ramseyrev >= 0)
					v = ramsey_config;
				break;
			}
			if (addr2 == 0x03) {
				v = 0xff;
				break;
			}
			if (addr2 == 0x02) { /* coldreboot flag */
				if (currprefs.cs_fatgaryrev >= 0)
					v = gary_coldboot ? 0x80 : 0x00;
			}
			if (addr2 == 0x01) { /* toenb flag */
				if (currprefs.cs_fatgaryrev >= 0)
					v = gary_toenb ? 0x80 : 0x00;
			}
			if (addr2 == 0x00) { /* timeout flag */
				if (currprefs.cs_fatgaryrev >= 0)
					v = gary_timeout ? 0x80 : 0x00;
			}
			v |= 0x7f;
			break;
		}
	} else {
		v = 0xff;
	}
	if (MBRES_LOG > 0)
		write_log (_T("MBRES_READ %08X=%08X (%d) PC=%08X S=%d\n"), addr, v, size, M68K_GETPC, regs.s);
	return v;
}

static uae_u32 REGPARAM3 mbres_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 mbres_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 mbres_bget (uaecptr) REGPARAM;
static void REGPARAM3 mbres_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 mbres_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 mbres_bput (uaecptr, uae_u32) REGPARAM;

static uae_u32 REGPARAM2 mbres_lget (uaecptr addr)
{
	uae_u32 v;
	v = mbres_wget (addr) << 16;
	v |= mbres_wget (addr + 2);
	return v;
}
static uae_u32 REGPARAM2 mbres_wget (uaecptr addr)
{
	return mbres_read (addr, 2);
}
static uae_u32 REGPARAM2 mbres_bget (uaecptr addr)
{
	return mbres_read (addr, 1);
}

static void REGPARAM2 mbres_lput (uaecptr addr, uae_u32 value)
{
	mbres_wput (addr, value >> 16);
	mbres_wput (addr + 2, value & 0xffff);
}

static void REGPARAM2 mbres_wput (uaecptr addr, uae_u32 value)
{
	mbres_write (addr, value, 2);
}

static void REGPARAM2 mbres_bput (uaecptr addr, uae_u32 value)
{
	mbres_write (addr, value, 1);
}

static addrbank mbres_sub_bank = {
	mbres_lget, mbres_wget, mbres_bget,
	mbres_lput, mbres_wput, mbres_bput,
	default_xlate, default_check, NULL, NULL, _T("Motherboard Resources"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO, S_READ, S_WRITE
};

static struct addrbank_sub mbres_sub_banks[] = {
	{ &mbres_sub_bank, 0x0000 },
	{ &dummy_bank,     0x8000 },
	{ NULL }
};

addrbank mbres_bank = {
	sub_bank_lget, sub_bank_wget, sub_bank_bget,
	sub_bank_lput, sub_bank_wput, sub_bank_bput,
	sub_bank_xlate, sub_bank_check, NULL, NULL, _T("Motherboard Resources"),
	sub_bank_lgeti, sub_bank_wgeti,
	ABFLAG_IO, S_READ, S_WRITE, mbres_sub_banks
};

static int pcmcia_common_size, pcmcia_attrs_size;
static uae_u8 *pcmcia_common;
static uae_u8 *pcmcia_attrs;
static int pcmcia_write_min, pcmcia_write_max;
static uae_u16 pcmcia_idedata;

void gayle_hsync(void)
{
	if (ide_interrupt_hsync(idedrive[0]) || ide_interrupt_hsync(idedrive[2]) || ide_interrupt_hsync(idedrive[4]))
		rethink_gayle();
}

static uaecptr from_gayle_pcmcmia(uaecptr addr)
{
	addr &= 0x80000 - 1;
	if (addr < 0x20000)
		return 0xffffffff; /* attribute */
	if (addr >= 0x40000)
		return 0xffffffff;
	addr -= 0x20000;
	// 8BITODD
	if (addr >= 0x10000) {
		addr &= ~0x10000;
		addr |= 1;
	}
	return addr;
}

static int get_pcmcmia_ide_reg (uaecptr addr, int width, struct ide_hdf **ide)
{
	int reg = -1;

	*ide = NULL;
	addr = from_gayle_pcmcmia(addr);
	if (addr == 0xffffffff)
		return -1;
	*ide = idedrive[PCMCIA_IDE_ID * 2];
	if ((*ide)->ide_drv)
		*ide = idedrive[PCMCIA_IDE_ID * 2 + 1];
	if (pcmcia_configured == 1) {
		// IO mapped linear
		reg = addr & 15;
		if (reg < 8)
			return reg;
		if (reg == 8)
			reg = IDE_DATA;
		else if (reg == 9)
			reg = IDE_DATA;
		else if (reg == 13)
			reg = IDE_ERROR;
		else if (reg == 14)
			reg = IDE_DEVCON;
		else if (reg == 15)
			reg = IDE_DRVADDR;
		else
			reg = -1;
	} else if (pcmcia_configured == 2) {
		// primary io mapped (PC)
		if (addr >= 0x1f0 && addr <= 0x1f7) {
			reg = addr - 0x1f0;
		} else if (addr == 0x3f6) {
			reg = IDE_DEVCON;
		} else if (addr == 0x3f7) {
			reg = IDE_DRVADDR;
		} else {
			reg = -1;
		}
	}
	return reg;
}

static uae_u32 gayle_attr_read (uaecptr addr)
{
	struct ide_hdf *ide = NULL;
	uae_u8 v = 0;

	if (PCMCIA_LOG > 1)
		write_log (_T("PCMCIA ATTR R: %x %x\n"), addr, M68K_GETPC);
	addr &= 0x80000 - 1;
	if (addr >= 0x40000) {
		if (PCMCIA_LOG > 0)
			write_log (_T("GAYLE: Reset disabled\n"));
		return v;
	}
	if (addr >= pcmcia_attrs_size)
		return v;
	if (pcmcia_type == PCMCIA_IDE) {
		if (addr >= 0x200 && addr < 0x200 + sizeof (pcmcia_configuration) * 2) {
			int offset = (addr - 0x200) / 2;
			return pcmcia_configuration[offset];
		}
		if (pcmcia_configured >= 0) {
			int reg = get_pcmcmia_ide_reg (addr, 1, &ide);
			if (reg >= 0) {
				if (reg == 0) {
					if (addr >= 0x30000) {
						return pcmcia_idedata & 0xff;
					} else {
						pcmcia_idedata = ide_get_data (ide);
						return (pcmcia_idedata >> 8) & 0xff;
					}
				} else {
					return ide_read_reg (ide, reg);
				}
			}
		}
	}
	v = pcmcia_attrs[addr / 2];
	return v;
}

static void gayle_attr_write (uaecptr addr, uae_u32 v)
{
	struct ide_hdf *ide = NULL;
	if (PCMCIA_LOG > 1)
		write_log (_T("PCMCIA ATTR W: %x=%x %x\n"), addr, v, M68K_GETPC);
	addr &= 0x80000 - 1;
	if (addr >= 0x40000) {
		if (PCMCIA_LOG > 0)
			write_log (_T("GAYLE: Reset enabled\n"));
		pcmcia_reset ();
	} else if (addr < pcmcia_attrs_size) {
		 if (pcmcia_type == PCMCIA_IDE) {
			 if (addr >= 0x200 && addr < 0x200 + sizeof (pcmcia_configuration) * 2) {
				int offset = (addr - 0x200) / 2;
				pcmcia_configuration[offset] = v;
				if (offset == 0) {
					if (v & 0x80) {
						pcmcia_reset ();
					} else {
						int index = v & 0x3f;
						if (index != 1 && index != 2) {
							write_log (_T("WARNING: Only config index 1 and 2 emulated, attempted to select %d!\n"), index);
						} else {
							pcmcia_configured = index;
							write_log (_T("PCMCIA IDE IO configured = %02x\n"), v);
						}
					}
				}
			}
			if (pcmcia_configured >= 0) {
				int reg = get_pcmcmia_ide_reg (addr, 1, &ide);
				if (reg >= 0) {
					if (reg == 0) {
						if (addr >= 0x30000) {
							pcmcia_idedata = (v & 0xff) << 8;
						} else {
							pcmcia_idedata &= 0xff00;
							pcmcia_idedata |= v & 0xff;
							ide_put_data (ide, pcmcia_idedata);
						}
						return;
					}
					ide_write_reg (ide, reg, v);
				}
		  }
    }
	}
}

static void initscideattr (int readonly)
{
	uae_u8 *rp;
	uae_u8 *p = pcmcia_attrs;
	struct hardfiledata *hfd = &pcmcia_sram->hfd;

	/* Mostly just copied from real CF cards.. */

	/* CISTPL_DEVICE */
	*p++ = 0x01;
	*p++ = 0x04;
	*p++ = 0xdf;
	*p++ = 0x4a;
	*p++ = 0x01;
	*p++ = 0xff;

	/* CISTPL_DEVICEOC */
	*p++ = 0x1c;
	*p++ = 0x04;
	*p++ = 0x02;
	*p++ = 0xd9;
	*p++ = 0x01;
	*p++ = 0xff;

	/* CISTPL_JEDEC */
	*p++ = 0x18;
	*p++ = 0x02;
	*p++ = 0xdf;
	*p++ = 0x01;

	/* CISTPL_VERS_1 */
	*p++= 0x15;
	rp = p++;
	*p++= 4; /* PCMCIA 2.1 */
	*p++= 1;
	strcpy ((char*)p, "UAE");
	p += strlen ((char*)p) + 1;
	strcpy ((char*)p, "68000");
	p += strlen ((char*)p) + 1;
	strcpy ((char*)p, "Generic Emulated PCMCIA IDE");
	p += strlen ((char*)p) + 1;
	*p++= 0xff;
	*rp = p - rp - 1;

	/* CISTPL_FUNCID */
	*p++ = 0x21;
	*p++ = 0x02;
	*p++ = 0x04;
	*p++ = 0x01;

	/* CISTPL_FUNCE */
	*p++ = 0x22;
	*p++ = 0x02;
	*p++ = 0x01;
	*p++ = 0x01;

	/* CISTPL_FUNCE */
	*p++ = 0x22;
	*p++ = 0x03;
	*p++ = 0x02;
	*p++ = 0x0c;
	*p++ = 0x0f;

	/* CISTPL_CONFIG */
	*p++ = 0x1a;
	*p++ = 0x05;
	*p++ = 0x01;
	*p++ = 0x01;
	*p++ = 0x00;
	*p++ = 0x02;
	*p++ = 0x0f;

	/* CISTPL_CFTABLEENTRY */
	*p++ = 0x1b;
	*p++ = 0x06;
	*p++ = 0xc0;
	*p++ = 0x01;
	*p++ = 0x21;
	*p++ = 0xb5;
	*p++ = 0x1e;
	*p++ = 0x4d;

	/* CISTPL_NO_LINK */
	*p++ = 0x14;
	*p++ = 0x00;

	/* CISTPL_END */
	*p++ = 0xff;
}

static void initsramattr (int size, int readonly)
{
	uae_u8 *rp;
	uae_u8 *p = pcmcia_attrs;
	int sm, su, code, units;
	struct hardfiledata *hfd = &pcmcia_sram->hfd;

	code = 0;
	su = 512;
	sm = 16384;
	while (size > sm) {
		sm *= 4;
		su *= 4;
		code++;
	}
	units = 31 - ((sm - size) / su);

	/* CISTPL_DEVICE */
	*p++ = 0x01;
	*p++ = 3;
	*p++ = (6 /* DTYPE_SRAM */ << 4) | (readonly ? 8 : 0) | (4 /* SPEED_100NS */);
	*p++ = (units << 3) | code; /* memory card size in weird units */
	*p++ = 0xff;

	/* CISTPL_DEVICEGEO */
	*p++ = 0x1e;
	*p++ = 7;
	*p++ = 2; /* 16-bit PCMCIA */
	*p++ = 0;
	*p++ = 1;
	*p++ = 1;
	*p++ = 1;
	*p++ = 1;
	*p++ = 0xff;

	/* CISTPL_VERS_1 */
	*p++= 0x15;
	rp = p++;
	*p++= 4; /* PCMCIA 2.1 */
	*p++= 1;
	strcpy ((char*)p, "UAE");
	p += strlen ((char*)p) + 1;
	strcpy ((char*)p, "68000");
	p += strlen ((char*)p) + 1;
	sprintf ((char*)p, "Generic Emulated %dKB PCMCIA SRAM Card", size >> 10);
	p += strlen ((char*)p) + 1;
	*p++= 0xff;
	*rp = p - rp - 1;

	/* CISTPL_FUNCID */
	*p++ = 0x21;
	*p++ = 2;
	*p++ = 1; /* Memory Card */
	*p++ = 0;

	/* CISTPL_MANFID */
	*p++ = 0x20;
	*p++ = 4;
	*p++ = 0xff;
	*p++ = 0xff;
	*p++ = 1;
	*p++ = 1;

	/* CISTPL_END */
	*p++ = 0xff;
}

static void checkflush (int addr)
{
	if (pcmcia_card == 0 || pcmcia_sram == 0)
		return;
	if (addr >= 0 && pcmcia_common[0] == 0 && pcmcia_common[1] == 0 && pcmcia_common[2] == 0)
		return; // do not flush periodically if used as a ram expension
	if (addr < 0) {
		pcmcia_write_min = 0;
		pcmcia_write_max = pcmcia_common_size;
	}
	if (pcmcia_write_min >= 0) {
		if (abs (pcmcia_write_min - addr) >= 512 || abs (pcmcia_write_max - addr) >= 512) {
			int blocksize = pcmcia_sram->hfd.ci.blocksize;
			int mask = ~(blocksize - 1);
			int start = pcmcia_write_min & mask;
			int end = (pcmcia_write_max + blocksize - 1) & mask;
			int len = end - start;
			if (len > 0) {
				hdf_write (&pcmcia_sram->hfd, pcmcia_common + start, start, len);
				pcmcia_write_min = -1;
				pcmcia_write_max = -1;
			}
		}
	}
	if (pcmcia_write_min < 0 || pcmcia_write_min > addr)
		pcmcia_write_min = addr;
	if (pcmcia_write_max < 0 || pcmcia_write_max < addr)
		pcmcia_write_max = addr;
}

static int freepcmcia (int reset)
{
	if (pcmcia_sram) {
		checkflush (-1);
		if (reset) {
			hdf_hd_close (pcmcia_sram);
			xfree (pcmcia_sram);
			pcmcia_sram = NULL;
		} else {
			pcmcia_sram->hfd.drive_empty = 1;
		}
	}
	remove_ide_unit(idedrive, PCMCIA_IDE_ID * 2);

	if (pcmcia_card)
		gayle_cs_change (GAYLE_CS_CCDET, 0);
	
	pcmcia_reset ();
	pcmcia_card = 0;

	xfree (pcmcia_common);
	xfree (pcmcia_attrs);
	pcmcia_common = NULL;
	pcmcia_attrs = NULL;
	pcmcia_common_size = 0;
	pcmcia_attrs_size = 0;

	gayle_cfg = 0;
	gayle_cs = 0;
	return 1;
}

static int initpcmcia (const TCHAR *path, int readonly, int type, int reset, struct uaedev_config_info *uci)
{
	if (currprefs.cs_pcmcia == 0)
		return 0;
	freepcmcia (reset);
	if (!pcmcia_sram)
		pcmcia_sram = xcalloc (struct hd_hardfiledata, 1);
	if (!pcmcia_sram->hfd.handle_valid)
		reset = 1;
	if (path != NULL)
		_tcscpy (pcmcia_sram->hfd.ci.rootdir, path);
	pcmcia_sram->hfd.ci.readonly = readonly != 0;
	pcmcia_sram->hfd.ci.blocksize = 512;

	if (type == PCMCIA_SRAM) {
		if (reset) {
			if (path)
				hdf_hd_open (pcmcia_sram);
		} else {
			pcmcia_sram->hfd.drive_empty = 0;
		}

		if (pcmcia_sram->hfd.ci.readonly)
			readonly = 1;
		pcmcia_common_size = 0;
		pcmcia_readonly = readonly;
		pcmcia_attrs_size = 256;
		pcmcia_attrs = xcalloc (uae_u8, pcmcia_attrs_size);
		pcmcia_type = type;

		if (!pcmcia_sram->hfd.drive_empty) {
			pcmcia_common_size = pcmcia_sram->hfd.virtsize;
			if (pcmcia_sram->hfd.virtsize > 4 * 1024 * 1024) {
				write_log (_T("PCMCIA SRAM: too large device, %llu bytes\n"), pcmcia_sram->hfd.virtsize);
				pcmcia_common_size = 4 * 1024 * 1024;
			}
			pcmcia_common = xcalloc (uae_u8, pcmcia_common_size);
			write_log (_T("PCMCIA SRAM: '%s' open, size=%d\n"), path, pcmcia_common_size);
			hdf_read (&pcmcia_sram->hfd, pcmcia_common, 0, pcmcia_common_size);
			pcmcia_card = 1;
			initsramattr (pcmcia_common_size, readonly);
		}

	} else if (type == PCMCIA_IDE) {

		if (reset && path) {	
			add_ide_unit (idedrive, TOTAL_IDE * 2, PCMCIA_IDE_ID * 2, uci, NULL);
		}
		ide_initialize(idedrive, PCMCIA_IDE_ID);

		pcmcia_common_size = 0;
		pcmcia_readonly = uci->readonly;
		pcmcia_attrs_size = 0x40000;
		pcmcia_attrs = xcalloc (uae_u8, pcmcia_attrs_size);
		pcmcia_type = type;

		write_log (_T("PCMCIA IDE: '%s' open\n"), path);
		pcmcia_card = 1;
		initscideattr (pcmcia_readonly);
	}

	if (pcmcia_card && !(gayle_cs & GAYLE_CS_DIS)) {
		gayle_map_pcmcia();
		card_trigger(1);
	}

	pcmcia_write_min = -1;
	pcmcia_write_max = -1;
	return 1;
}

static uae_u32 gayle_common_read (uaecptr addr)
{
	uae_u8 v = 0;
	if (PCMCIA_LOG > 2)
		write_log (_T("PCMCIA COMMON R: %x %x\n"), addr, M68K_GETPC);
	if (!pcmcia_common_size)
		return 0;
	addr -= PCMCIA_COMMON_START & (PCMCIA_COMMON_SIZE - 1);
	addr &= PCMCIA_COMMON_SIZE - 1;
	if (addr < pcmcia_common_size)
		v = pcmcia_common[addr];
	return v;
}

static void gayle_common_write (uaecptr addr, uae_u32 v)
{
	if (PCMCIA_LOG > 2)
		write_log (_T("PCMCIA COMMON W: %x=%x %x\n"), addr, v, M68K_GETPC);
	if (!pcmcia_common_size)
		return;
	if (pcmcia_readonly)
		return;
	addr -= PCMCIA_COMMON_START & (PCMCIA_COMMON_SIZE - 1);
	addr &= PCMCIA_COMMON_SIZE - 1;
	if (addr < pcmcia_common_size) {
		if (pcmcia_common[addr] != v) {
			checkflush (addr);
			pcmcia_common[addr] = v;
		}
	}
}

static uae_u32 REGPARAM3 gayle_common_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 gayle_common_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 gayle_common_bget (uaecptr) REGPARAM;
static void REGPARAM3 gayle_common_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 gayle_common_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 gayle_common_bput (uaecptr, uae_u32) REGPARAM;

static int REGPARAM2 gayle_common_check (uaecptr addr, uae_u32 size)
{
	if (!pcmcia_common_size)
		return 0;
	addr -= PCMCIA_COMMON_START & (PCMCIA_COMMON_SIZE - 1);
	addr &= PCMCIA_COMMON_SIZE - 1;
	return (addr + size) <= PCMCIA_COMMON_SIZE;
}

static uae_u8 *REGPARAM2 gayle_common_xlate (uaecptr addr)
{
	addr -= PCMCIA_COMMON_START & (PCMCIA_COMMON_SIZE - 1);
	addr &= PCMCIA_COMMON_SIZE - 1;
	return pcmcia_common + addr;
}

static addrbank gayle_common_bank = {
	gayle_common_lget, gayle_common_wget, gayle_common_bget,
	gayle_common_lput, gayle_common_wput, gayle_common_bput,
	gayle_common_xlate, gayle_common_check, NULL, NULL, _T("Gayle PCMCIA Common"),
	gayle_common_lget, gayle_common_wget,
	ABFLAG_RAM | ABFLAG_SAFE, S_READ, S_WRITE
};


static uae_u32 REGPARAM3 gayle_attr_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 gayle_attr_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 gayle_attr_bget (uaecptr) REGPARAM;
static void REGPARAM3 gayle_attr_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 gayle_attr_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 gayle_attr_bput (uaecptr, uae_u32) REGPARAM;

static addrbank gayle_attr_bank = {
	gayle_attr_lget, gayle_attr_wget, gayle_attr_bget,
	gayle_attr_lput, gayle_attr_wput, gayle_attr_bput,
	default_xlate, default_check, NULL, NULL, _T("Gayle PCMCIA Attribute/Misc"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static uae_u32 REGPARAM2 gayle_attr_lget (uaecptr addr)
{
	uae_u32 v;
	v = gayle_attr_wget (addr) << 16;
	v |= gayle_attr_wget (addr + 2);
	return v;
}
static uae_u32 REGPARAM2 gayle_attr_wget (uaecptr addr)
{
	uae_u16 v = 0;

	if (pcmcia_configured >= 0) {
		if (pcmcia_type == PCMCIA_IDE) {
		  struct ide_hdf *ide = NULL;
		  int reg = get_pcmcmia_ide_reg (addr, 2, &ide);
		  if (reg == IDE_DATA) {
			  // 16-bit register
			  pcmcia_idedata = ide_get_data (ide);
			  return pcmcia_idedata;
			}
		}
	}

	v = gayle_attr_bget (addr) << 8;
	v |= gayle_attr_bget (addr + 1);
	return v;
}
static uae_u32 REGPARAM2 gayle_attr_bget (uaecptr addr)
{
	return gayle_attr_read (addr);
}
static void REGPARAM2 gayle_attr_lput (uaecptr addr, uae_u32 value)
{
	gayle_attr_wput (addr, value >> 16);
	gayle_attr_wput (addr + 2, value & 0xffff);
}
static void REGPARAM2 gayle_attr_wput (uaecptr addr, uae_u32 value)
{
	if (pcmcia_configured >= 0) {
		if (pcmcia_type == PCMCIA_IDE) {
		  struct ide_hdf *ide = NULL;
		  int reg = get_pcmcmia_ide_reg (addr, 2, &ide);
		  if (reg == IDE_DATA) {
			  // 16-bit register
			  pcmcia_idedata = value;
			  ide_put_data (ide, pcmcia_idedata);
			  return;
			}
		}
	}

	gayle_attr_bput (addr, value >> 8);
	gayle_attr_bput (addr + 1, value & 0xff);
}
static void REGPARAM2 gayle_attr_bput (uaecptr addr, uae_u32 value)
{
	gayle_attr_write (addr, value);
}


static uae_u32 REGPARAM2 gayle_common_lget (uaecptr addr)
{
	uae_u32 v;
	v = gayle_common_wget (addr) << 16;
	v |= gayle_common_wget (addr + 2);
	return v;
}
static uae_u32 REGPARAM2 gayle_common_wget (uaecptr addr)
{
	uae_u16 v;
	v = gayle_common_bget (addr) << 8;
	v |= gayle_common_bget (addr + 1);
	return v;
}
static uae_u32 REGPARAM2 gayle_common_bget (uaecptr addr)
{
	return gayle_common_read (addr);
}
static void REGPARAM2 gayle_common_lput (uaecptr addr, uae_u32 value)
{
	gayle_common_wput (addr, value >> 16);
	gayle_common_wput (addr + 2, value & 0xffff);
}
static void REGPARAM2 gayle_common_wput (uaecptr addr, uae_u32 value)
{
	gayle_common_bput (addr, value >> 8);
	gayle_common_bput (addr + 1, value & 0xff);
}
static void REGPARAM2 gayle_common_bput (uaecptr addr, uae_u32 value)
{
	gayle_common_write (addr, value);
}

void gayle_map_pcmcia (void)
{
	if (currprefs.cs_pcmcia == 0)
		return;
	if (pcmcia_card == 0 || (gayle_cs & GAYLE_CS_DIS)) {
		map_banks_cond (&dummy_bank, 0xa0, 8, 0);
		if (currprefs.chipmem_size <= 4 * 1024 * 1024 && !expansion_get_autoconfig_by_address(&currprefs, 4 * 1024 * 1024))
			map_banks_cond (&dummy_bank, PCMCIA_COMMON_START >> 16, PCMCIA_COMMON_SIZE >> 16, 0);
	} else {
		map_banks_cond (&gayle_attr_bank, 0xa0, 8, 0);
		if (currprefs.chipmem_size <= 4 * 1024 * 1024 && !expansion_get_autoconfig_by_address(&currprefs, 4 * 1024 * 1024))
			map_banks_cond (&gayle_common_bank, PCMCIA_COMMON_START >> 16, PCMCIA_COMMON_SIZE >> 16, 0);
	}
}

void gayle_free_units (void)
{
	for (int i = 0; i < TOTAL_IDE * 2; i++) {
		remove_ide_unit(idedrive, i);
	}
	freepcmcia (1);
}

void gayle_add_ide_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct ide_hdf *ide;

	if (ch >= 2 * 2)
		return;
	ide = add_ide_unit (idedrive, TOTAL_IDE * 2, ch, ci, NULL);
}

bool gayle_ide_init(struct autoconfig_info *aci)
{
	aci->zorro = 0;
	if (aci->prefs->cs_ide == 1) {
		aci->start = GAYLE_BASE_1200;
		aci->size = 0x10000;
	} else {
		aci->start = GAYLE_BASE_4000;
		aci->size = 0x1000;
	}
	return true;
}

int gayle_add_pcmcia_sram_unit (struct uaedev_config_info *uci)
{
	return initpcmcia (uci->rootdir, uci->readonly, PCMCIA_SRAM, 1, NULL);
}

int gayle_add_pcmcia_ide_unit (struct uaedev_config_info *uci)
{
	return initpcmcia (uci->rootdir, 0, PCMCIA_IDE, 1, uci);
}

int gayle_modify_pcmcia_sram_unit (struct uaedev_config_info *uci, int insert)
{
	if (insert)
		return initpcmcia (uci->rootdir, uci->readonly, PCMCIA_SRAM, pcmcia_sram ? 0 : 1, NULL);
	else
		return freepcmcia (0);
}

int gayle_modify_pcmcia_ide_unit (struct uaedev_config_info *uci, int insert)
{
	if (insert)
		return initpcmcia (uci->rootdir, 0, PCMCIA_IDE, pcmcia_sram ? 0 : 1, uci);
	else
		return freepcmcia (0);
}

void gayle_add_pcmcia_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
}
bool gayle_pcmcia_init(struct autoconfig_info *aci)
{
	aci->start = 0x600000;
	aci->size = 0xa80000 - aci->start;
	aci->zorro = 0;
	return true;
}


static void initide (void)
{
	gayle_its.idetable = idedrive;
	gayle_its.idetotal = TOTAL_IDE * 2;
	start_ide_thread(&gayle_its);
	alloc_ide_mem (idedrive, TOTAL_IDE * 2, &gayle_its);
	ide_initialize(idedrive, GAYLE_IDE_ID);
	ide_initialize(idedrive, GAYLE_IDE_ID + 1);

	if (isrestore ())
		return;
	ide_splitter = 0;
	if (ide_isdrive (idedrive[2]) || ide_isdrive(idedrive[3])) {
		ide_splitter = 1;
		write_log (_T("IDE splitter enabled\n"));
	}
	gayle_irq = gayle_int = 0;
}

void gayle_free (void)
{
	stop_ide_thread(&gayle_its);
}

void gayle_reset (int hardreset)
{
	static TCHAR bankname[100];

	initide ();
	if (hardreset) {
		ramsey_config = 0;
		gary_coldboot = 1;
		gary_timeout = 0;
		gary_toenb = 0;
	}
	_tcscpy (bankname, _T("Gayle (low)"));
	if (currprefs.cs_ide == IDE_A4000)
		_tcscpy (bankname, _T("A4000 IDE"));
	gayle_bank.name = bankname;
}

uae_u8 *restore_gayle (uae_u8 *src)
{
	changed_prefs.cs_ide = restore_u8 ();
	gayle_int = restore_u8 ();
	gayle_irq = restore_u8 ();
	gayle_cs = restore_u8 ();
	gayle_cs_mask = restore_u8 ();
	gayle_cfg = restore_u8 ();
	return src;
}

uae_u8 *save_gayle (int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;

	if (currprefs.cs_ide <= 0)
		return NULL;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u8 (currprefs.cs_ide);
	save_u8 (gayle_int);
	save_u8 (gayle_irq);
	save_u8 (gayle_cs);
	save_u8 (gayle_cs_mask);
	save_u8 (gayle_cfg);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *save_gayle_ide (int num, int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	struct ide_hdf *ide;

	if (num >= TOTAL_IDE * 2 || idedrive[num] == NULL)
		return NULL;
	if (currprefs.cs_ide <= 0)
		return NULL;
	ide = idedrive[num];
	if (ide->hdhfd.size == 0)
		return NULL;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (num);
	dst = ide_save_state(dst, ide);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_gayle_ide (uae_u8 *src)
{
	int num, readonly, blocksize;
	uae_u64 size;
	TCHAR *path;
	struct ide_hdf *ide;

	alloc_ide_mem (idedrive, TOTAL_IDE * 2, NULL);
	num = restore_u32 ();
	ide = idedrive[num];
	size = restore_u64 ();
	path = restore_string ();
	_tcscpy (ide->hdhfd.hfd.ci.rootdir, path);
	blocksize = restore_u32 ();
	readonly = restore_u32 ();
	src = ide_restore_state(src, ide);
	if (ide->hdhfd.hfd.virtual_size)
		gayle_add_ide_unit (num, NULL, NULL);
	else
		gayle_add_ide_unit (num, NULL, NULL);
	xfree (path);
	return src;
}
