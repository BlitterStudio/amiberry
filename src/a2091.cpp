/*
* UAE - The Un*x Amiga Emulator
*
* A590/A2091/A3000/CDTV SCSI expansion (DMAC/SuperDMAC + WD33C93) emulation
* Includes A590 + XT drive emulation.
* GVP Series I and II
* A2090 SCSI and ST-506
*
* Copyright 2007-2015 Toni Wilen
*
*/

#define GVP_S1_DEBUG_IO 0
#define GVP_S2_DEBUG_IO 0
#define A2091_DEBUG 0
#define A2091_DEBUG_IO 0
#define XT_DEBUG 0
#define A3000_DEBUG 0
#define A3000_DEBUG_IO 0
#define COMSPEC_DEBUG 0

#define WD33C93_DEBUG 0
#define WD33C93_DEBUG_PIO 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "debug.h"
#include "scsi.h"
#include "threaddep/thread.h"
#include "a2091.h"
#include "blkdev.h"
#include "gui.h"
#include "zfile.h"
#include "filesys.h"
#include "autoconf.h"
#include "cdtv.h"
#include "savestate.h"
#include "cpuboard.h"
#include "rtc.h"
#include "devices.h"
#ifdef WITH_DSP
#include "dsp3210/dsp_glue.h"
#endif

#define DMAC_8727_ROM_VECTOR 0x8000
#define CDMAC_ROM_VECTOR 0x2000
#define CDMAC_ROM_OFFSET 0x2000
#define GVP_ROM_OFFSET 0x8000
#define GVP_SERIES_I_RAM_OFFSET_1 0x04000
#define GVP_SERIES_I_RAM_OFFSET_2 0x04000
#define GVP_SERIES_I_RAM_OFFSET_3 0x10000
#define GVP_SERIES_I_RAM_MASK_1 (4096 - 1)
#define GVP_SERIES_I_RAM_MASK_2 (16384 - 1)
#define GVP_SERIES_I_RAM_MASK_3 (16384 - 1)

/* SuperDMAC CNTR bits. */
#define SCNTR_TCEN	(1<<5)
#define SCNTR_PREST	(1<<4)
#define SCNTR_PDMD	(1<<3)
#define SCNTR_INTEN	(1<<2)
#define SCNTR_DDIR	(1<<1)
#define SCNTR_IO_DX	(1<<0)
/* DMAC CNTR bits. */
#define CNTR_TCEN	(1<<7)
#define CNTR_PREST	(1<<6)
#define CNTR_PDMD	(1<<5)
#define CNTR_INTEN	(1<<4)
#define CNTR_DDIR	(1<<3)
/* ISTR bits. */
#define ISTR_INT_F	(1<<7)	/* Interrupt Follow */
#define ISTR_INTS	(1<<6)	/* SCSI or XT Peripheral Interrupt */
#define ISTR_E_INT	(1<<5)	/* End-Of-Process Interrupt */
#define ISTR_INT_P	(1<<4)	/* Interrupt Pending */
#define ISTR_UE_INT	(1<<3)	/* Under-Run FIFO Error Interrupt */
#define ISTR_OE_INT	(1<<2)	/* Over-Run FIFO Error Interrupt */
#define ISTR_FF_FLG	(1<<1)	/* FIFO-Full Flag */
#define ISTR_FE_FLG	(1<<0)	/* FIFO-Empty Flag */

/* GVP models */
#define GVP_GFORCE_040		0x20
#define GVP_GFORCE_040_SCSI	0x30
#define GVP_A1291			0x46
#define GVP_A1291_SCSI		0x47
#define GVP_COMBO_R4		0x60
#define GVP_COMBO_R4_SCSI	0x70
#define GVP_A1208			0x97
#define GVP_IO_EXTENDER		0x98
#define GVP_GFORCE_030		0xa0
#define GVP_GFORCE_030_SCSI	0xb0
#define GVP_A530			0xc0
#define GVP_A530_SCSI		0xd0
#define GVP_COMBO_R3		0xe0
#define GVP_COMBO_R3_SCSI	0xf0
#define GVP_SERIESII		0xf8

/* wd register names */
#define WD_OWN_ID		0x00
#define WD_CONTROL		0x01
#define WD_TIMEOUT_PERIOD	0x02
#define WD_CDB_1		0x03
#define WD_T_SECTORS	0x03
#define WD_CDB_2		0x04
#define WD_T_HEADS		0x04
#define WD_CDB_3		0x05
#define WD_T_CYLS_0		0x05
#define WD_CDB_4		0x06
#define WD_T_CYLS_1		0x06
#define WD_CDB_5		0x07
#define WD_L_ADDR_0		0x07
#define WD_CDB_6		0x08
#define WD_L_ADDR_1		0x08
#define WD_CDB_7		0x09
#define WD_L_ADDR_2		0x09
#define WD_CDB_8		0x0a
#define WD_L_ADDR_3		0x0a
#define WD_CDB_9		0x0b
#define WD_SECTOR		0x0b
#define WD_CDB_10		0x0c
#define WD_HEAD			0x0c
#define WD_CDB_11		0x0d
#define WD_CYL_0		0x0d
#define WD_CDB_12		0x0e
#define WD_CYL_1		0x0e
#define WD_TARGET_LUN		0x0f
#define WD_COMMAND_PHASE	0x10
#define WD_SYNCHRONOUS_TRANSFER 0x11
#define WD_TRANSFER_COUNT_MSB	0x12
#define WD_TRANSFER_COUNT	0x13
#define WD_TRANSFER_COUNT_LSB	0x14
#define WD_DESTINATION_ID	0x15
#define WD_SOURCE_ID		0x16
#define WD_SCSI_STATUS		0x17
#define WD_COMMAND		0x18
#define WD_DATA			0x19
#define WD_QUEUE_TAG		0x1a
#define WD_AUXILIARY_STATUS	0x1f
/* WD commands */
#define WD_CMD_RESET		0x00
#define WD_CMD_ABORT		0x01
#define WD_CMD_ASSERT_ATN	0x02
#define WD_CMD_NEGATE_ACK	0x03
#define WD_CMD_DISCONNECT	0x04
#define WD_CMD_RESELECT		0x05
#define WD_CMD_SEL_ATN		0x06
#define WD_CMD_SEL		0x07
#define WD_CMD_SEL_ATN_XFER	0x08
#define WD_CMD_SEL_XFER		0x09
#define WD_CMD_RESEL_RECEIVE	0x0a
#define WD_CMD_RESEL_SEND	0x0b
#define WD_CMD_WAIT_SEL_RECEIVE	0x0c
#define WD_CMD_TRANS_ADDR	0x18
#define WD_CMD_TRANS_INFO	0x20
#define WD_CMD_TRANSFER_PAD	0x21
#define WD_CMD_SBT_MODE		0x80

/* paused or aborted interrupts */
#define CSR_MSGIN			0x20
#define CSR_SDP				0x21
#define CSR_SEL_ABORT		0x22
#define CSR_RESEL_ABORT		0x25
#define CSR_RESEL_ABORT_AM	0x27
#define CSR_ABORT			0x28
/* successful completion interrupts */
#define CSR_RESELECT		0x10
#define CSR_SELECT			0x11
#define CSR_TRANS_ADDR		0x15
#define CSR_SEL_XFER_DONE	0x16
#define CSR_XFER_DONE		0x18
/* terminated interrupts */
#define CSR_INVALID			0x40
#define CSR_UNEXP_DISC		0x41
#define CSR_TIMEOUT			0x42
#define CSR_PARITY			0x43
#define CSR_PARITY_ATN		0x44
#define CSR_BAD_STATUS		0x45
#define CSR_UNEXP			0x48
/* service required interrupts */
#define CSR_RESEL			0x80
#define CSR_RESEL_AM		0x81
#define CSR_ATN_ASSERT		0x84
#define CSR_DISC			0x85
#define CSR_SRV_REQ			0x88
/* SCSI Bus Phases */
#define PHS_DATA_OUT	0x00
#define PHS_DATA_IN		0x01
#define PHS_COMMAND		0x02
#define PHS_STATUS		0x03
#define PHS_MESS_OUT	0x06
#define PHS_MESS_IN		0x07

/* Auxialiry status */
#define ASR_INT			0x80	/* Interrupt pending */
#define ASR_LCI			0x40	/* Last command ignored */
#define ASR_BSY			0x20	/* Busy, only cmd/data/asr readable */
#define ASR_CIP			0x10	/* Busy, cmd unavail also */
#define ASR_xxx			0x0c
#define ASR_PE			0x02	/* Parity error (even) */
#define ASR_DBR			0x01	/* Data Buffer Ready */
/* Status */
#define CSR_CAUSE		0xf0
#define CSR_RESET		0x00	/* chip was reset */
#define CSR_CMD_DONE	0x10	/* cmd completed */
#define CSR_CMD_STOPPED	0x20	/* interrupted or abrted*/
#define CSR_CMD_ERR		0x40	/* end with error */
#define CSR_BUS_SERVICE	0x80	/* REQ pending on the bus */
/* Control */
#define CTL_DMA			0x80	/* Single byte dma */
#define CTL_DBA_DMA		0x40	/* direct buffer access (bus master) */
#define CTL_BURST_DMA	0x20	/* continuous mode (8237) */
#define CTL_NO_DMA		0x00	/* Programmed I/O */
#define CTL_HHP			0x10	/* Halt on host parity error */
#define CTL_EDI			0x08	/* Ending disconnect interrupt */
#define CTL_IDI			0x04	/* Intermediate disconnect interrupt*/
#define CTL_HA			0x02	/* Halt on ATN */
#define CTL_HSP			0x01	/* Halt on SCSI parity error */

/* SCSI Messages */
#define MSG_COMMAND_COMPLETE 0x00
#define MSG_SAVE_DATA_POINTER 0x02
#define MSG_RESTORE_DATA_POINTERS 0x03
#define MSG_NOP 0x08
#define MSG_IDENTIFY 0x80

/* XT hard disk controller registers */
#define XD_DATA         0x00    /* data RW register */
#define XD_RESET        0x01    /* reset WO register */
#define XD_STATUS       0x01    /* status RO register */
#define XD_SELECT       0x02    /* select WO register */
#define XD_JUMPER       0x02    /* jumper RO register */
#define XD_CONTROL      0x03    /* DMAE/INTE WO register */
#define XD_RESERVED     0x03    /* reserved */

/* XT hard disk controller commands (incomplete list) */
#define XT_CMD_TESTREADY   0x00    /* test drive ready */
#define XT_CMD_RECALIBRATE 0x01    /* recalibrate drive */
#define XT_CMD_SENSE       0x03    /* request sense */
#define XT_CMD_FORMATDRV   0x04    /* format drive */
#define XT_CMD_VERIFY      0x05    /* read verify */
#define XT_CMD_FORMATTRK   0x06    /* format track */
#define XT_CMD_FORMATBAD   0x07    /* format bad track */
#define XT_CMD_READ        0x08    /* read */
#define XT_CMD_WRITE       0x0A    /* write */
#define XT_CMD_SEEK        0x0B    /* seek */
/* Controller specific commands */
#define XT_CMD_DTCSETPARAM 0x0C    /* set drive parameters (DTC 5150X & CX only?) */

/* Bits for command status byte */
#define XT_CSB_ERROR       0x02    /* error */
#define XT_CSB_LUN         0x20    /* logical Unit Number */

/* XT hard disk controller status bits */
#define XT_STAT_READY      0x01    /* controller is ready */
#define XT_STAT_INPUT      0x02    /* data flowing from controller to host */
#define XT_STAT_COMMAND    0x04    /* controller in command phase */
#define XT_STAT_SELECT     0x08    /* controller is selected */
#define XT_STAT_REQUEST    0x10    /* controller requesting data */
#define XT_STAT_INTERRUPT  0x20    /* controller requesting interrupt */

/* XT hard disk controller control bits */
#define XT_INT          0x02    /* Interrupt enable */
#define XT_DMA_MODE     0x01    /* DMA enable */

#define XT_UNIT 8
#define XT_SECTORS 17 /* hardwired */

#define XT506_UNIT0 8
#define XT506_UNIT1 9

#define MAX_SCSI_UNITS (8 + 2)

static struct wd_state *wd_a2091[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct wd_state *wd_a2090[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct wd_state *wd_a3000;
static struct wd_state *wd_gvps1[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct wd_state *wd_gvps2[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct wd_state *wd_gvpa1208[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct wd_state *wd_gvps2accel;
static struct wd_state *wd_comspec[MAX_DUPLICATE_EXPANSION_BOARDS];
struct wd_state *wd_cdtv;
static bool configured;
static uae_u8 gvp_accelerator_bank;

static struct wd_state *scsi_units[MAX_SCSI_UNITS + 1];

static void wd_init(void);
static void wd_addreset(void);

static void freencrunit(struct wd_state *wd)
{
	if (!wd)
		return;
	for (int i = 0; i < MAX_SCSI_UNITS; i++) {
		if (scsi_units[i] == wd) {
			scsi_units[i] = NULL;
		}
	}
	scsi_freenative(wd->scsis, MAX_SCSI_UNITS);
	if (wd->rom >= wd->bank.baseaddr && wd->rom < wd->bank.baseaddr + wd->bank.allocated_size)
		free_expansion_bank(&wd->bank);
	else
		xfree (wd->rom);
	if (wd->rom2 >= wd->bank2.baseaddr && wd->rom2 < wd->bank2.baseaddr + wd->bank2.allocated_size)
		mapped_free(&wd->bank2);
	else
		xfree (wd->rom2);
	xfree(wd->userdata);
	wd->rom = NULL;
	if (wd->self_ptr)
		*wd->self_ptr = NULL;
	xfree(wd);
}

static struct wd_state *allocscsi(struct wd_state **wd, struct romconfig *rc, int ch)
{
	struct wd_state *scsi;

	if (ch < 0) {
		freencrunit(*wd);
		*wd = NULL;
	}
	configured = true;
	if ((*wd) == NULL) {
		scsi = xcalloc(struct wd_state, 1);
		for (int i = 0; i < MAX_SCSI_UNITS; i++) {
			if (scsi_units[i] == NULL) {
				scsi_units[i] = scsi;
				if (rc)
					rc->unitdata = scsi;
				scsi->rc = rc;
				scsi->self_ptr = wd;
				scsi->id = i;
				*wd = scsi;
				return scsi;
			}
		}
	}
	return *wd;
}

static struct wd_state *getscsi(struct romconfig *rc)
{
	if (rc->unitdata)
		return (struct wd_state*)rc->unitdata;
	return NULL;
}

static struct wd_state *getscsiboard(uaecptr addr)
{
	for (int i = 0; scsi_units[i]; i++) {
		if (!scsi_units[i]->baseaddress)
			return scsi_units[i];
		if ((addr & ~scsi_units[i]->board_mask) == scsi_units[i]->baseaddress)
			return scsi_units[i];
		if (scsi_units[i]->baseaddress2 && (addr & ~scsi_units[i]->board_mask) == scsi_units[i]->baseaddress2)
			return scsi_units[i];
	}
	return NULL;
}

static void reset_dmac(struct wd_state *wd)
{
	switch (wd->dmac_type)
	{
		case GVP_DMAC_S1:
		case GVP_DMAC_S2:
		wd->gdmac.cntr = 0;
		wd->gdmac.dma_on = 0;
		break;
		case COMMODORE_SDMAC:
		case COMMODORE_DMAC:
		case COMMODORE_8727:
		wd->cdmac.dmac_dma = 0;
		wd->cdmac.dmac_istr = 0;
		wd->cdmac.dmac_cntr = 0;
		break;
	}
}

static int isirq(struct wd_state *wd)
{
	if (!wd->enabled)
		return false;
	switch (wd->dmac_type)
	{
		case GVP_DMAC_S1:
		if ((wd->gdmac.cntr & 0x80) && (wd->wc.auxstatus & ASR_INT))
			return 1;
		break;
		case GVP_DMAC_S2:
		if (wd->wc.auxstatus & ASR_INT)
			wd->gdmac.cntr |= 2;
		if ((wd->gdmac.cntr & (2 | 8)) == (2 | 8))
			return 1;
		break;
		case COMMODORE_SDMAC:
		if (wd->wc.auxstatus & ASR_INT)
			wd->cdmac.dmac_istr |= ISTR_INTS | ISTR_INT_F;
		else
			wd->cdmac.dmac_istr &= ~ISTR_INT_F;
		if ((wd->cdmac.dmac_cntr & SCNTR_INTEN) && (wd->cdmac.dmac_istr & (ISTR_INTS | ISTR_E_INT)))
			return 1;
		break;
		case COMMODORE_DMAC:
		if (wd->cdmac.xt_irq)
			wd->cdmac.dmac_istr |= ISTR_INTS | ISTR_INT_F;
		else if (wd->wc.auxstatus & ASR_INT)
			wd->cdmac.dmac_istr |= ISTR_INTS | ISTR_INT_F;
		else
			wd->cdmac.dmac_istr &= ~ISTR_INT_F;
		if ((wd->cdmac.dmac_cntr & CNTR_INTEN) && (wd->cdmac.dmac_istr & (ISTR_INTS | ISTR_E_INT)))
			return 1;
		break;
		case COMMODORE_8727:
		if (wd->cdmac.xt_irq)
			wd->cdmac.dmac_istr |= ISTR_INTS;
		if (wd->wc.auxstatus & ASR_INT)
			wd->cdmac.dmac_istr |= ISTR_INTS;
		if ((wd->cdmac.dmac_cntr & CNTR_INTEN) && (wd->cdmac.dmac_istr & ISTR_INTS))
			return 1;
		break;
		case COMSPEC_CHIP:
		{
			int irq = 0;
			if ((wd->comspec.status & 0x20) && (wd->wc.auxstatus & ASR_INT))
				irq |= 2;
			if ((wd->comspec.status & 0x08) && wd->wc.wd_data_avail)
				irq |= 1;
			return irq;
		}
		break;
	}
	return 0;
}

static void set_dma_done(struct wd_state *wds)
{
	switch (wds->dmac_type)
	{
		case GVP_DMAC_S1:
		case GVP_DMAC_S2:
		wds->gdmac.dma_on = -1;
		break;
		case COMMODORE_SDMAC:
		case COMMODORE_DMAC:
		case COMMODORE_8727:
		wds->cdmac.dmac_dma = -1;
		break;
	}
}

static bool is_dma_enabled(struct wd_state *wds)
{
	switch (wds->dmac_type)
	{
		case GVP_DMAC_S1:
		return true;
		case GVP_DMAC_S2:
		return wds->gdmac.dma_on > 0;
		case COMMODORE_SDMAC:
		case COMMODORE_DMAC:
		case COMMODORE_8727:
		return wds->cdmac.dmac_dma > 0;
	}
	return false;
}

static void rethink_a2091(void)
{
	if (!configured)
		return;
	for (int i = 0; i < MAX_SCSI_UNITS; i++) {
		if (scsi_units[i]) {
			int irq = isirq(scsi_units[i]);
			if (irq & 1)
				safe_interrupt_set(IRQ_SOURCE_WD, i, false);
			if (irq & 2)
				safe_interrupt_set(IRQ_SOURCE_WD, i, true);
#if DEBUG > 2 || A3000_DEBUG > 2
			write_log (_T("Interrupt_RETHINK:%d\n"), irq);
#endif
		}
	}
}

static void dmac_scsi_int(struct wd_state *wd)
{
	if (!wd->enabled)
		return;
	if (!(wd->wc.auxstatus & ASR_INT))
		return;
	devices_rethink_all(rethink_a2091);
}

static void dmac_a2091_xt_int(struct wd_state *wd)
{
	if (!wd->enabled)
		return;
	wd->cdmac.xt_irq = true;
	devices_rethink_all(rethink_a2091);
}

void scsi_dmac_a2091_start_dma (struct wd_state *wd)
{
#if A3000_DEBUG > 0 || A2091_DEBUG > 0
	write_log (_T("DMAC DMA started, ADDR=%08X, LEN=%08X words\n"), wd->cdmac.dmac_acr, wd->cdmac.dmac_wtc);
#endif
	wd->cdmac.dmac_dma = 1;
}
void scsi_dmac_a2091_stop_dma (struct wd_state *wd)
{
	wd->cdmac.dmac_dma = 0;
	wd->cdmac.dmac_istr &= ~ISTR_E_INT;
}

static void dmac_reset (struct wd_state *wd)
{
#if WD33C93_DEBUG > 0
	if (wd->dmac_type == COMMODORE_SDMAC)
		write_log (_T("A3000 %s SCSI reset\n"), WD33C93);
	else if (wd->dmac_type == COMMODORE_DMAC)
		write_log (_T("A2091 %s SCSI reset\n"), WD33C93);
#endif
}

static void incsasr (struct wd_chip_state *wd, int w)
{
	if (wd->sasr == WD_AUXILIARY_STATUS || wd->sasr == WD_DATA || wd->sasr == WD_COMMAND)
		return;
	if (w && wd->sasr == WD_SCSI_STATUS)
		return;
	wd->sasr++;
	wd->sasr &= 0x1f;
}

static void dmac_a2091_cint (struct wd_state *wd)
{
	wd->cdmac.dmac_istr = 0;
	devices_rethink_all(rethink_a2091);
}

static void doscsistatus(struct wd_state *wd, uae_u8 status)
{
	wd->wc.wdregs[WD_SCSI_STATUS] = status;
	wd->wc.auxstatus |= ASR_INT;
#if WD33C93_DEBUG > 1
	write_log (_T("%s STATUS=%02X\n"), WD33C93, status);
#endif
	if (!wd->enabled)
		return;
	if (wd->cdtv) {
		cdtv_scsi_int ();
		return;
	}
	dmac_scsi_int(wd);
#if WD33C93_DEBUG > 2
	write_log (_T("Interrupt\n"));
#endif
}

static void set_status_intmask(struct wd_chip_state *wd, uae_u16 intmask)
{
	wd->intmask |= intmask;
}

static void set_status (struct wd_chip_state *wd, uae_u8 status, int delay)
{
	if (wd->queue_index >= WD_STATUS_QUEUE) {
		write_log(_T("WD int queue overflow!\n"));
		return;
	}
	wd->status[wd->queue_index].status = status;
	wd->status[wd->queue_index].irq = delay == 0 ? 1 : (delay <= 2 ? 2 : delay);
	wd->queue_index++;
}

static void set_status (struct wd_chip_state *wd, uae_u8 status)
{
	set_status (wd, status, 0);
}

static uae_u32 gettc (struct wd_chip_state *wd)
{
	return wd->wdregs[WD_TRANSFER_COUNT_LSB] | (wd->wdregs[WD_TRANSFER_COUNT] << 8) | (wd->wdregs[WD_TRANSFER_COUNT_MSB] << 16);
}
static void settc (struct wd_chip_state *wd, uae_u32 tc)
{
	wd->wdregs[WD_TRANSFER_COUNT_LSB] = tc & 0xff;
	wd->wdregs[WD_TRANSFER_COUNT] = (tc >> 8) & 0xff;
	wd->wdregs[WD_TRANSFER_COUNT_MSB] = (tc >> 16) & 0xff;
}
static bool decreasetc(struct wd_chip_state *wd)
{
	uae_u32 tc = gettc (wd);
	if (!tc)
		return true;
	tc--;
	settc (wd, tc);
	return tc == 0;
}

static int canwddma(struct wd_state *wds)
{
	struct wd_chip_state *wd = &wds->wc;
	uae_u8 mode = wd->wdregs[WD_CONTROL] >> 5;
	switch(wds->dmac_type)
	{
		case COMMODORE_8727:
		if (mode != 0 && mode != 4) {
			write_log (_T("%s weird DMA mode %d!!\n"), WD33C93, mode);
		}
		return mode == 4;
		case COMMODORE_DMAC:
		case COMMODORE_SDMAC:
		case GVP_DMAC_S2:
		if (mode != 0 && mode != 4 && mode != 1) {
			write_log (_T("%s weird DMA mode %d!!\n"), WD33C93, mode);
		}
		return mode == 4 || mode == 1;
		case GVP_DMAC_S1:
		if (mode != 0 && mode != 2) {
			write_log (_T("%s weird DMA mode %d!!\n"), WD33C93, mode);
		}
		return mode == 2;
		case COMSPEC_CHIP:
		return -1;
		default:
		return 0;
	}
}

#if WD33C93_DEBUG > 0
static TCHAR *scsitostring (struct wd_chip_state *wd, struct scsi_data *scsi)
{
	static TCHAR buf[200];
	TCHAR *p;
	int i;

	p = buf;
	p[0] = 0;
	for (i = 0; i < scsi->offset && i < sizeof wd->wd_data; i++) {
		if (i > 0) {
			_tcscat (p, _T("."));
			p++;
		}
		_stprintf (p, _T("%02X"), wd->wd_data[i]);
		p += _tcslen (p);
	}
	return buf;
}
#endif

static void setphase(struct wd_chip_state *wd, uae_u8 phase)
{
	wd->wdregs[WD_COMMAND_PHASE] = phase;
}

static bool dmacheck_a2091 (struct wd_state *wd)
{
	wd->cdmac.dmac_acr++;
	if (wd->cdmac.old_dmac && (wd->cdmac.dmac_cntr & CNTR_TCEN)) {
		if (wd->cdmac.dmac_wtc == 0) {
			wd->cdmac.dmac_istr |= ISTR_E_INT;
			return true;
		} else {
			if ((wd->cdmac.dmac_acr & 1) == 1)
				wd->cdmac.dmac_wtc--;
		}
	}
	return false;
}

static bool dmacheck_a2090 (struct wd_state *wd)
{
	int dir = wd->cdmac.dmac_acr & 0x00800000;
	wd->cdmac.dmac_acr &= 0x7fffff;
	wd->cdmac.dmac_acr++;
	wd->cdmac.dmac_acr &= 0x7fffff;
	wd->cdmac.dmac_acr |= dir;
	wd->cdmac.dmac_wtc--;
	return wd->cdmac.dmac_wtc == 0;
}


static bool do_dma_commodore_8727(struct wd_state *wd, struct scsi_data *scsi)
{
	int dir = wd->cdmac.dmac_acr & 0x00800000;

	if (scsi->direction < 0) {
		if (dir) {
			write_log(_T("8727 mismatched direction!\n"));
			return false;
		}
#if WD33C93_DEBUG > 0
		uaecptr odmac_acr = wd->cdmac.dmac_acr;
#endif
		for (;;) {
			uae_u8 v1 = 0, v2 = 0;
			int status;
			status = scsi_receive_data (scsi, &v1, true);
			if (!status)
				status = scsi_receive_data(scsi, &v2, true);
			dma_put_word((wd->cdmac.dmac_acr << 1) & wd->dma_mask, (v1 << 8) | v2);
			if (wd->wc.wd_dataoffset < sizeof wd->wc.wd_data - 1) {
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v1;
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v2;
			}
			if (decreasetc (&wd->wc))
				break;
			if (decreasetc (&wd->wc))
				break;
			if (status)
				break;
			if (dmacheck_a2090 (wd))
				break;
		}
#if WD33C93_DEBUG > 0
		write_log (_T("%s Done DMA from WD, %d/%d %08X\n"), WD33C93, scsi->offset, scsi->data_len, (odmac_acr << 1) & wd->dma_mask);
#endif
		wd->cdmac.c8727_pcsd |= 1 << 7;
		return true;
	} else if (scsi->direction > 0) {
		if (!dir) {
			write_log(_T("8727 mismatched direction!\n"));
			return false;
		}
#if WD33C93_DEBUG > 0
		uaecptr odmac_acr = wd->cdmac.dmac_acr;
#endif
		for (;;) {
			int status;
			uae_u16 v = dma_get_word((wd->cdmac.dmac_acr << 1) & wd->dma_mask);
			if (wd->wc.wd_dataoffset < sizeof wd->wc.wd_data - 1) {
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v >> 8;
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = (uae_u8)v;
			}
			status = scsi_send_data (scsi, v >> 8);
			if (!status)
				status = scsi_send_data (scsi, (uae_u8)v);
			if (decreasetc (&wd->wc))
				break;
			if (decreasetc (&wd->wc))
				break;
			if (status)
				break;
			if (dmacheck_a2090 (wd))
				break;
		}
#if WD33C93_DEBUG > 0
		write_log (_T("%s Done DMA to WD, %d/%d %08x\n"), WD33C93, scsi->offset, scsi->data_len, (odmac_acr << 1) & wd->dma_mask);
#endif
		wd->cdmac.c8727_pcsd |= 1 << 7;
		return true;
	}
	return false;
}

static bool do_dma_commodore(struct wd_state *wd, struct scsi_data *scsi)
{
	if (wd->cdtv)
		cdtv_getdmadata(&wd->cdmac.dmac_acr);
	if (scsi->direction < 0) {
#if WD33C93_DEBUG || XT_DEBUG > 0
		uaecptr odmac_acr = wd->cdmac.dmac_acr;
#endif
		bool run = true;
		while (run) {
			uae_u8 v;
			int status = scsi_receive_data(scsi, &v, true);
			dma_put_byte(wd->cdmac.dmac_acr & wd->dma_mask, v);
			if (wd->wc.wd_dataoffset < sizeof wd->wc.wd_data)
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v;
			if (decreasetc (&wd->wc))
				run = false;
			if (dmacheck_a2091 (wd))
				run = false;
			if (status)
				run = false;
		}
#if WD33C93_DEBUG || XT_DEBUG > 0
		write_log (_T("%s Done DMA from WD, %d/%d %08X\n"), WD33C93, scsi->offset, scsi->data_len, odmac_acr);
#endif
		return true;
	} else if (scsi->direction > 0) {
#if WD33C93_DEBUG || XT_DEBUG > 0
		uaecptr odmac_acr = wd->cdmac.dmac_acr;
#endif
		bool run = true;
		while (run) {
			int status;
			uae_u8 v = dma_get_byte(wd->cdmac.dmac_acr & wd->dma_mask);
			if (wd->wc.wd_dataoffset < sizeof wd->wc.wd_data)
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v;
			status = scsi_send_data (scsi, v);
			if (decreasetc (&wd->wc))
				run = false;
			if (dmacheck_a2091 (wd))
				run = false;
			if (status)
				run = false;
		}
#if WD33C93_DEBUG || XT_DEBUG > 0
		write_log (_T("%s Done DMA to WD, %d/%d %08x\n"), WD33C93, scsi->offset, scsi->data_len, odmac_acr);
#endif
		return true;
	}
	return false;
}

static bool do_dma_gvp_s1(struct wd_state *wd, struct scsi_data *scsi)
{
	if (scsi->direction < 0) {
		for (;;) {
			uae_u8 v;
			int status = scsi_receive_data(scsi, &v, true);
			wd->gdmac.buffer[wd->wc.wd_dataoffset++] = v;
			wd->wc.wd_dataoffset &= wd->gdmac.s1_rammask;
			if (decreasetc (&wd->wc))
				break;
			if (status)
				break;
		}
#if WD33C93_DEBUG > 0
		write_log (_T("%s Done DMA from WD, %d/%d\n"), WD33C93, scsi->offset, scsi->data_len);
#endif
		return true;
	} else if (scsi->direction > 0) {
		for (;;) {
			int status;
			uae_u8 v = wd->gdmac.buffer[wd->wc.wd_dataoffset++];
			wd->wc.wd_dataoffset &= wd->gdmac.s1_rammask;
			status = scsi_send_data (scsi, v);
			wd->gdmac.addr++;
			if (decreasetc (&wd->wc))
				break;
			if (status)
				break;
		}
#if WD33C93_DEBUG > 0
		write_log (_T("%s Done DMA to WD, %d/%d\n"), WD33C93, scsi->offset, scsi->data_len);
#endif
		return true;
	}
	return false;
}

static uae_u32 get_gvp_s2_addr(struct gvp_dmac *g)
{
	uae_u32 v = g->addr & g->addr_mask;
	if (g->bank_ptr) {
		v |= g->bank_ptr[0] << 24;
	}
	return v;
}

static bool do_dma_gvp_s2(struct wd_state *wd, struct scsi_data *scsi)
{
#if WD33C93_DEBUG > 0
	uae_u32 dmaptr = get_gvp_s2_addr(&wd->gdmac);
#endif
	if (!is_dma_enabled(wd))
		return false;

	if (scsi->direction < 0) {
		if (wd->gdmac.cntr & 0x10) {
			write_log(_T("GVP DMA: mismatched direction when reading!\n"));
			return false;
		}
		for (;;) {
			uae_u8 v;
			int status = scsi_receive_data(scsi, &v, true);
			dma_put_byte(get_gvp_s2_addr(&wd->gdmac) & wd->dma_mask, v);
			if (wd->wc.wd_dataoffset < sizeof wd->wc.wd_data)
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v;
			wd->gdmac.addr++;
			wd->gdmac.addr &= wd->gdmac.addr_mask;
			if (decreasetc (&wd->wc))
				break;
			if (status)
				break;
		}
#if WD33C93_DEBUG > 0
		write_log (_T("%s Done DMA from WD, %d/%d %08x\n"), WD33C93, scsi->offset, scsi->data_len, dmaptr);
#endif
		return true;
	} else if (scsi->direction > 0) {
		if (!(wd->gdmac.cntr & 0x10)) {
			write_log(_T("GVP DMA: mismatched direction when writing!\n"));
			return false;
		}
		for (;;) {
			int status;
			uae_u8 v = dma_get_byte(get_gvp_s2_addr(&wd->gdmac) & wd->dma_mask);
			if (wd->wc.wd_dataoffset < sizeof wd->wc.wd_data)
				wd->wc.wd_data[wd->wc.wd_dataoffset++] = v;
			status = scsi_send_data (scsi, v);
			wd->gdmac.addr++;
			wd->gdmac.addr &= wd->gdmac.addr_mask;
			if (decreasetc (&wd->wc))
				break;
			if (status)
				break;
		}
#if WD33C93_DEBUG > 0
		write_log (_T("%s Done DMA to WD, %d/%d %08x\n"), WD33C93, scsi->offset, scsi->data_len, dmaptr);
#endif
		return true;
	}
	return false;
}

static bool do_dma(struct wd_state *wd)
{
	struct scsi_data *scsi = wd->wc.scsi;
	wd->wc.wd_data_avail = 0;
	m68k_cancel_idle();
	if (scsi->direction == 0)
		write_log (_T("%s DMA but no data!?\n"), WD33C93);
	switch (wd->dmac_type)
	{
		case COMMODORE_DMAC:
		case COMMODORE_SDMAC:
		return do_dma_commodore(wd, scsi);
		case COMMODORE_8727:
		return do_dma_commodore_8727(wd, scsi);
		case GVP_DMAC_S2:
		return do_dma_gvp_s2(wd, scsi);
		case GVP_DMAC_S1:
		return do_dma_gvp_s1(wd, scsi);
	}
	return false;
}

static void wd_cmd_sel_xfer (struct wd_chip_state *wd, struct wd_state *wds, bool atn);

static bool wd_do_transfer_out (struct wd_chip_state *wd, struct wd_state *wds, struct scsi_data *scsi)
{
#if WD33C93_DEBUG > 0
	write_log (_T("%s SCSI O [%02X] %d/%d TC=%d %s\n"), WD33C93, wd->wdregs[WD_COMMAND_PHASE], scsi->offset, scsi->data_len, gettc (wd), scsitostring (wd, scsi));
#endif
	if (wd->wdregs[WD_COMMAND_PHASE] < 0x20) {
		int msg = wd->wd_data[0];
		/* message was sent */
		setphase (wd, 0x20);
		wd->wd_phase = CSR_XFER_DONE | PHS_COMMAND;
		scsi->status = 0;
		scsi_start_transfer (scsi);
#if WD33C93_DEBUG > 0
		write_log (_T("%s SCSI got MESSAGE %02X\n"), WD33C93, msg);
#endif
		scsi->message[0] = msg;
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x30) {
#if WD33C93_DEBUG > 0
		write_log (_T("%s SCSI got COMMAND %02X\n"), WD33C93, wd->wd_data[0]);
#endif
		if (scsi->offset < scsi->data_len) {
			// data missing, ask for more
			wd->wd_phase = CSR_XFER_DONE | PHS_COMMAND;
			setphase (wd, 0x30 + scsi->offset);
			set_status (wd, wd->wd_phase, 1);
			return false;
		}
		settc (wd, 0);
		scsi_start_transfer (scsi);
		scsi_emulate_analyze (scsi);
		if (scsi->direction > 0) {
			/* if write command, need to wait for data */
			if (scsi->data_len <= 0 || scsi->direction == 0) {
				// Status phase if command didn't return anything and don't want anything
				wd->wd_phase = CSR_XFER_DONE | PHS_STATUS;
				setphase (wd, 0x46);
			} else {
				wd->wd_phase = CSR_XFER_DONE | PHS_DATA_OUT;
				setphase (wd, 0x45);
			}
		} else {
			scsi_emulate_cmd (scsi);
			if (wd->scsi->data_len <= 0 || scsi->direction == 0) {
				// Status phase if command didn't return anything and don't want anything
				wd->wd_phase = CSR_XFER_DONE | PHS_STATUS;
				setphase (wd, 0x46);
			} else {
				wd->wd_phase = CSR_XFER_DONE | PHS_DATA_IN;
				setphase (wd, 0x45); // just skip all reselection and message stuff for now..
			}
		}
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x46 || wd->wdregs[WD_COMMAND_PHASE] == 0x45) {
		if (wd->scsi->offset < scsi->data_len) {
			// data missing, ask for more
			wd->wd_phase = CSR_XFER_DONE | (scsi->direction < 0 ? PHS_DATA_IN : PHS_DATA_OUT);
			set_status (wd, wd->wd_phase, 10);
			return false;
		}
		settc (wd, 0);
		if (scsi->direction > 0) {
			/* data was sent */
			scsi_emulate_cmd (scsi);
			scsi->data_len = 0;
			wd->wd_phase = CSR_XFER_DONE | PHS_STATUS;
		}
		scsi_start_transfer (scsi);
		setphase (wd, 0x47);
	}
	wd->wd_dataoffset = 0;
	if (wd->wdregs[WD_COMMAND] == WD_CMD_SEL_ATN_XFER || wd->wdregs[WD_COMMAND] == WD_CMD_SEL_XFER) {
		wd_cmd_sel_xfer(wd, wds, false);
		return true;
	}
	set_status (wd, wd->wd_phase, scsi->direction <= 0 ? 0 : 1);
	wd->wd_busy = false;
	return true;
}

static bool wd_do_transfer_in (struct wd_chip_state *wd, struct wd_state *wds, struct scsi_data *scsi, bool message_in_transfer_info)
{
#if WD33C93_DEBUG > 0
	write_log (_T("%s SCSI I [%02X] %d/%d TC=%d %s\n"), WD33C93, wd->wdregs[WD_COMMAND_PHASE], scsi->offset, scsi->data_len, gettc (wd), scsitostring (wd, scsi));
#endif
	wd->wd_dataoffset = 0;
	if (wd->wdregs[WD_COMMAND_PHASE] >= 0x36 && wd->wdregs[WD_COMMAND_PHASE] < 0x46) {
		if (scsi->offset < scsi->data_len) {
			// data missing, ask for more
			wd->wd_phase = CSR_XFER_DONE | (scsi->direction < 0 ? PHS_DATA_IN : PHS_DATA_OUT);
			set_status(wd, wd->wd_phase, 1);
			return false;
		}
		if (gettc (wd) != 0) {
			wd->wd_phase = CSR_UNEXP | PHS_STATUS;
			setphase(wd, 0x46);
		} else {
			wd->wd_phase = CSR_XFER_DONE | PHS_STATUS;
			setphase(wd, 0x46);
			if (wd->wdregs[WD_COMMAND] == WD_CMD_SEL_ATN_XFER || wd->wdregs[WD_COMMAND] == WD_CMD_SEL_XFER) {
				wd_cmd_sel_xfer(wd, wds, false);
				return true;
			}
		}
		scsi_start_transfer(scsi);
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x46 || wd->wdregs[WD_COMMAND_PHASE] == 0x47) {
		if (wd->wdregs[WD_COMMAND] == WD_CMD_SEL_ATN_XFER || wd->wdregs[WD_COMMAND] == WD_CMD_SEL_XFER) {
			wd_cmd_sel_xfer(wd, wds, false);
			return true;
		}
		setphase(wd, 0x50);
		wd->wd_phase = CSR_XFER_DONE | PHS_MESS_IN;
		scsi_start_transfer(scsi);
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x50) {
		// was TRANSFER INFO with message phase, wait for Negate ACK
		if (!message_in_transfer_info) {
			wd->wd_phase = CSR_DISC;
			wd->wd_selected = false;
			scsi_start_transfer(scsi);
			setphase(wd, 0x60);
		} else {
			wd->wd_phase = CSR_MSGIN;
		}
	}
	set_status(wd, wd->wd_phase, 1);
	scsi->direction = 0;
	return true;
}

static void set_pio_data_irq(struct wd_chip_state *wd, struct wd_state *wds)
{
	if (!wd->wd_data_avail)
		return;
	switch(wds->dmac_type)
	{
		case COMSPEC_CHIP:
		if (wds->comspec.status & 0x10) {
			wds->comspec.status |= 0x08;
			set_status_intmask(wd, 0x2000);
		}
		break;
	}
}

static void wd_cmd_sel_xfer (struct wd_chip_state *wd, struct wd_state *wds, bool atn)
{
	int i, tmp_tc;
	struct scsi_data *scsi;

	wd->auxstatus = 0;
	wd->wd_data_avail = 0;
	tmp_tc = gettc (wd);
	scsi = wd->scsi = wds->scsis[wd->wdregs[WD_DESTINATION_ID] & 7];
	if (!scsi) {
		set_status (wd, CSR_TIMEOUT, 0);
		wd->wdregs[WD_COMMAND_PHASE] = 0x00;
#if WD33C93_DEBUG > 0
		write_log (_T("* %s select and transfer%s, ID=%d: No device\n"),
			WD33C93, atn ? _T(" with atn") : _T(""), wd->wdregs[WD_DESTINATION_ID] & 0x7);
#endif
		return;
	}
	if (!wd->wd_selected) {
		scsi->message[0] = 0x80;
		wd->wd_selected = true;
		wd->wdregs[WD_COMMAND_PHASE] = 0x10;
	}
#if WD33C93_DEBUG > 0
	write_log (_T("* %s select and transfer%s, ID=%d PHASE=%02X TC=%d wddma=%d dmac=%d\n"),
		WD33C93, atn ? _T(" with atn") : _T(""), wd->wdregs[WD_DESTINATION_ID] & 0x7, wd->wdregs[WD_COMMAND_PHASE], tmp_tc, wd->wdregs[WD_CONTROL] >> 5, wds->cdmac.dmac_dma);
#endif
	if (wd->wdregs[WD_COMMAND_PHASE] <= 0x30) {
		scsi->buffer[0] = 0;
		scsi->status = 0;
		memcpy (scsi->cmd, &wd->wdregs[3], 16);
		scsi->data_len = tmp_tc;
		scsi_emulate_analyze (scsi);
		settc (wd, scsi->cmd_len);
		wd->wd_dataoffset = 0;
		scsi_start_transfer (scsi);
		scsi->direction = 2;
		scsi->data_len = scsi->cmd_len;
		for (i = 0; i < gettc (wd); i++) {
			uae_u8 b = scsi->cmd[i];
			wd->wd_data[i] = b;
			scsi_send_data (scsi, b);
			wd->wd_dataoffset++;
		}
		// 0x30 = command phase has started
		scsi->data_len = tmp_tc;
#if WD33C93_DEBUG > 0
		write_log (_T("%s: Got Command %s, datalen=%d\n"), WD33C93, scsitostring (wd, scsi), scsi->data_len);
#endif
		if (!scsi_emulate_analyze (scsi)) {
			wd->wdregs[WD_COMMAND_PHASE] = 0x46;
			goto end;
		}
		wd->wdregs[WD_COMMAND_PHASE] = 0x30 + gettc (wd);
		settc (wd, 0);
		if (scsi->direction <= 0) {
			scsi_emulate_cmd (scsi);
		}
		scsi_start_transfer (scsi);
	}

	if (wd->wdregs[WD_COMMAND_PHASE] <= 0x41) {
		wd->wdregs[WD_COMMAND_PHASE] = 0x44;
#if 0
		if (wd->wdregs[WD_CONTROL] & CTL_IDI) {
			wd->wd_phase = CSR_DISC;
			set_status (wd, wd->wd_phase, delay);
			wd->wd_phase = CSR_RESEL;
			set_status (wd, wd->wd_phase, delay + 10);
			return;
		}
#endif
		wd->wdregs[WD_COMMAND_PHASE] = 0x44;
	}

	// target replied or start/continue data phase (if data available)
	if (wd->wdregs[WD_COMMAND_PHASE] == 0x44) {
		wd->wdregs[WD_COMMAND_PHASE] = 0x45;
	}
		
	if (wd->wdregs[WD_COMMAND_PHASE] == 0x45) {
		settc (wd, tmp_tc);
		wd->wd_dataoffset = 0;
		setphase (wd, 0x45);

		if (gettc (wd) == 0) {
			if (scsi->direction != 0) {
				// TC = 0 but we may have data
				if (scsi->direction < 0) {
					if (scsi->data_len == 0 || scsi->offset >= scsi->data_len) {
						// no data, continue normally to status phase
						setphase (wd, 0x46);
						goto end;
					}
				}
				wd->wd_phase = CSR_UNEXP;
				if (scsi->direction < 0)
					wd->wd_phase |= PHS_DATA_IN;
				else
					wd->wd_phase |= PHS_DATA_OUT;
				set_status (wd, wd->wd_phase, 1);
				return;
			}
		}

		if (wd->scsi->direction) {
			// wanted data but nothing available?
			if (scsi->direction < 0 && scsi->data_len == 0 && gettc(wd)) {
				setphase(wd, 0x46);
				wd->wd_phase = CSR_UNEXP | PHS_STATUS;
				set_status (wd, wd->wd_phase, 1);
				return;
			}
			if (canwddma (wds) > 0) {
				if (scsi->direction <= 0) {
					do_dma(wds);
					if (scsi->offset < scsi->data_len) {
						// buffer not completely retrieved?
						wd->wd_phase = CSR_UNEXP | PHS_DATA_IN;
						set_status (wd, wd->wd_phase, 1);
						return;
					}
					if (gettc (wd) > 0) {
						// requested more data than was available.
						setphase(wd, 0x46);
						wd->wd_phase = CSR_UNEXP | PHS_STATUS;
						set_status (wd, wd->wd_phase, 1);
						return;
					}
					setphase (wd, 0x46);
				} else {
					if (do_dma(wds)) {
						setphase (wd, 0x46);
						if (scsi->offset < scsi->data_len) {
							// not enough data?
							wd->wd_phase = CSR_UNEXP | PHS_DATA_OUT;
							set_status (wd, wd->wd_phase, 1);
							return;
						}
						// got all data -> execute it
						scsi_emulate_cmd (scsi);
					}
				}
			} else if (canwddma (wds) < 0) {
				// pio
				wd->wd_data_avail = 1;
				set_pio_data_irq(wd, wds);
				return;
			} else {
				// no dma = Service Request
				wd->wd_phase = CSR_SRV_REQ;
				if (scsi->direction < 0)
					wd->wd_phase |= PHS_DATA_IN;
				else
					wd->wd_phase |= PHS_DATA_OUT;
				set_status (wd, wd->wd_phase, 1);
				return;
			}
		} else {
			// TC > 0 but no data to transfer
			if (gettc (wd)) {
				wd->wd_phase = CSR_UNEXP | PHS_STATUS;
				set_status (wd, wd->wd_phase, 1);
				return;
			}
			setphase(wd, 0x46);
		}
	}

	end:
	if (wd->wdregs[WD_COMMAND_PHASE] == 0x46) {
		scsi->buffer[0] = 0;
		wd->wdregs[WD_COMMAND_PHASE] = 0x50;
		wd->wdregs[WD_TARGET_LUN] = scsi->status;
		scsi->buffer[0] = scsi->status;
	}

	// 0x60 = command complete
	wd->wdregs[WD_COMMAND_PHASE] = 0x60;
	if (!(wd->wdregs[WD_CONTROL] & CTL_EDI)) {
		wd->wd_phase = CSR_SEL_XFER_DONE;
		set_status (wd, wd->wd_phase, 2);
		wd->wd_phase = CSR_DISC;
		set_status (wd, wd->wd_phase, 0);
	} else {
		wd->wd_phase = CSR_SEL_XFER_DONE;
		set_status (wd, wd->wd_phase, 2);
	}
	wd->wd_selected = 0;
}

static void wd_cmd_trans_info (struct wd_state *wds, struct scsi_data *scsi, bool pad)
{
	struct wd_chip_state *wd = &wds->wc;
	if (wd->wdregs[WD_COMMAND_PHASE] == 0x20) {
		wd->wdregs[WD_COMMAND_PHASE] = 0x30;
		scsi->status = 0;
	}
	wd->wd_busy = true;
	if (wd->wdregs[WD_COMMAND] & 0x80)
		settc (wd, 1);
	if (gettc (wd) == 0)
		settc (wd, 1);
	wd->wd_dataoffset = 0;

	if (wd->wdregs[WD_COMMAND_PHASE] == 0x30) {
		scsi->direction = 2; // command
		scsi->cmd_len = scsi->data_len = gettc (wd);
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x10) {
		scsi->direction = 1; // message
		scsi->data_len = gettc (wd);
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x45) {
		scsi_emulate_analyze (scsi);
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x46 || wd->wdregs[WD_COMMAND_PHASE] == 0x47) {
		scsi->buffer[0] = scsi->status;
		wd->wdregs[WD_TARGET_LUN] = scsi->status;
		scsi->direction = -1; // status
		scsi->data_len = 1;
	} else if (wd->wdregs[WD_COMMAND_PHASE] == 0x50) {
		scsi->direction = -1;
		scsi->data_len = gettc (wd);
	}

	if (pad) {
		int v;
		settc(wd, 0);
		if (wd->scsi->direction < 0) {
			v = wd_do_transfer_in (wd, wds, wd->scsi, false);
		} else if (wd->scsi->direction > 0) {
			v = wd_do_transfer_out (wd, wds, wd->scsi);
		}
		wd->scsi->direction = 0;
		wd->wd_data_avail = 0;
	} else {
		if (canwddma (wds) > 0) {
			wd->wd_data_avail = -1;
		} else {
			wd->wd_data_avail = 1;
		}
	}

#if WD33C93_DEBUG > 0
	write_log (_T("* %s transfer info phase=%02x TC=%d dir=%d data=%d/%d wddma=%d\n"),
		WD33C93, wd->wdregs[WD_COMMAND_PHASE], gettc (wd), scsi->direction, scsi->offset, scsi->data_len, wd->wdregs[WD_CONTROL] >> 5);
#endif

}

/* Weird stuff, XT driver (which has nothing to do with SCSI or WD33C93) uses this WD33C93 command! */
static void wd_cmd_trans_addr(struct wd_chip_state *wd, struct wd_state *wds)
{
	uae_u32 tcyls = (wd->wdregs[WD_T_CYLS_0] << 8) | wd->wdregs[WD_T_CYLS_1];
	uae_u32 theads = wd->wdregs[WD_T_HEADS];
	uae_u32 tsectors = wd->wdregs[WD_T_SECTORS];
	uae_u32 lba = (wd->wdregs[WD_L_ADDR_0] << 24) | (wd->wdregs[WD_L_ADDR_1] << 16) |
		(wd->wdregs[WD_L_ADDR_2] << 8) | (wd->wdregs[WD_L_ADDR_3] << 0);
	uae_u32 cyls, heads, sectors;

	cyls = lba / (theads * tsectors);
	heads = (lba - ((cyls * theads * tsectors))) / tsectors;
	sectors = (lba - ((cyls * theads * tsectors))) % tsectors;

	write_log(_T("WD TRANS ADDR: LBA=%d TC=%d TH=%d TS=%d -> C=%d H=%d S=%d\n"), lba, tcyls, theads, tsectors, cyls, heads, sectors);

	wd->wdregs[WD_CYL_0] = cyls >> 8;
	wd->wdregs[WD_CYL_1] = cyls;
	wd->wdregs[WD_HEAD] = heads;
	wd->wdregs[WD_SECTOR] = sectors;

	if (cyls >= tcyls)
		set_status(wd, CSR_BAD_STATUS);
	else
		set_status(wd, CSR_TRANS_ADDR);
}

static void wd_cmd_sel (struct wd_chip_state *wd, struct wd_state *wds, bool atn)
{
	struct scsi_data *scsi;
#if WD33C93_DEBUG > 0
	write_log (_T("* %s select%s, ID=%d\n"), WD33C93, atn ? _T(" with atn") : _T(""), wd->wdregs[WD_DESTINATION_ID] & 0x7);
#endif
	wd->wd_phase = 0;
	wd->wdregs[WD_COMMAND_PHASE] = 0;

	scsi = wd->scsi = wds->scsis[wd->wdregs[WD_DESTINATION_ID] & 7];
	if (!scsi || (wd->wdregs[WD_DESTINATION_ID] & 7) == 7) {
#if WD33C93_DEBUG > 0
		write_log (_T("%s no drive\n"), WD33C93);
#endif
		set_status (wd, CSR_TIMEOUT, 1000);
		return;
	}
	scsi_start_transfer (wd->scsi);
	wd->wd_selected = true;
	scsi->message[0] = 0x80;
	set_status (wd, CSR_SELECT, 2);
	if (atn) {
		wd->wdregs[WD_COMMAND_PHASE] = 0x10;
		set_status (wd, CSR_SRV_REQ | PHS_MESS_OUT, 4);
	} else {
		wd->wdregs[WD_COMMAND_PHASE] = 0x20;
		set_status (wd, CSR_SRV_REQ | PHS_COMMAND, 4);
	} 
}

static void wd_cmd_reset (struct wd_chip_state *wd, bool irq, bool fast)
{
#if WD33C93_DEBUG > 0
	write_log (_T("%s reset %d\n"), WD33C93, irq);
#endif
	for (int i = 1; i <= 0x16; i++)
		wd->wdregs[i] = 0;
	wd->wdregs[0x18] = 0;
	wd->sasr = 0;
	wd->wd_selected = false;
	wd->scsi = NULL;
	for (int j = 0; j < WD_STATUS_QUEUE; j++) {
		memset(&wd->status[j], 0, sizeof(status_data));
	}
	wd->queue_index = 0;
	wd->auxstatus = 0;
	wd->wd_data_avail = 0;
	wd->resetnodelay_active = false;
	if (irq) {
		uae_u8 status = (wd->wdregs[0] & 0x08) ? 1 : 0;
		if (fast) {
			wd->wdregs[WD_SCSI_STATUS] = status;
			wd->auxstatus |= ASR_INT;
			set_status(wd, status);
			wd->wd_busy = false;
			wd->resetnodelay_active = true;
			rethink_a2091();
		} else {
			set_status(wd, status, 50);
		}
	} else {
		wd->wd_busy = false;
	}
}

static void wd_master_reset(struct wd_state *wd, bool irq)
{
	memset(wd->wc.wdregs, 0, sizeof wd->wc.wdregs);
	wd_cmd_reset(&wd->wc, false, false);
	if (irq) {
		// this needs to be fast but must not call INTREQ() directly.
		wd->wc.wdregs[WD_SCSI_STATUS] = 0;
		wd->wc.auxstatus |= ASR_INT;
		set_status(&wd->wc, 0);
		wd->wc.resetnodelay_active = true;
		rethink_a2091();
	}
}

static void wd_cmd_abort (struct wd_chip_state *wd)
{
#if WD33C93_DEBUG > 0
	write_log (_T("%s abort\n"), WD33C93);
#endif
}

static void xt_command_done(struct wd_state *wd, uae_u8);

static void wd_check_interrupt(struct wd_state *wds, bool checkonly)
{
	struct wd_chip_state *wd = &wds->wc;
	if (wd->intmask) {
		safe_interrupt_set(IRQ_SOURCE_WD, wds->id, (wd->intmask & 0x2000) != 0);
		wd->intmask = 0;
	}
	if (wd->auxstatus & ASR_INT)
		return;
	if (wd->queue_index == 0)
		return;
	if (wd->status[0].irq == 1) {
		wd->status[0].irq = 0;
		doscsistatus(wds, wd->status[0].status);
		wd->wd_busy = false;
		if (wd->queue_index == 2) {
			wd->status[0].irq = 1;
			memcpy(&wd->status[0], &wd->status[1], sizeof(status_data));
			wd->queue_index = 1;
		} else {
			wd->queue_index = 0;
		}
	} else if (!checkonly && wd->status[0].irq > 1) {
		wd->status[0].irq--;
	}
}

static uae_u32 makecmd (struct scsi_data *s, int msg, uae_u8 cmd)
{
	uae_u32 v = 0;
	if (s)
		v |= s->id << 24;
	v |= msg << 8;
	v |= cmd;
	return v;
}

static void wd_execute_cmd(struct wd_state *wds, int cmd, int msg, int unit);
static void wd_execute(struct wd_state *wds, struct scsi_data *scsi, int msg, uae_u8 cmd)
{
	if (wds->threaded) {
		write_comm_pipe_u32 (&wds->requests, makecmd (scsi, msg, cmd), 1);
	} else {
		wd_execute_cmd(wds, cmd, msg, scsi ? scsi->id : 0);
	}
}

static void scsi_hsync_check_dma(struct wd_state *wds)
{
	struct wd_chip_state *wd = &wds->wc;
	if (wd->wd_data_avail < 0 && is_dma_enabled(wds)) {
		bool v;
		do_dma(wds);
		if (wd->scsi->direction < 0) {
			v = wd_do_transfer_in (wd, wds, wd->scsi, false);
		} else if (wd->scsi->direction > 0) {
			v = wd_do_transfer_out (wd, wds, wd->scsi);
		} else {
			write_log (_T("%s data transfer attempt without data!\n"), WD33C93);
			v = true;
		}
		if (v) {
			wd->scsi->direction = 0;
			wd->wd_data_avail = 0;
		} else {
			set_dma_done(wds);
		}
	}
}

static void scsi_hsync2_a2091 (struct wd_state *wds)
{
	struct wd_chip_state *wd = &wds->wc;

	if (!wds || !wds->enabled)
		return;
	scsi_hsync_check_dma(wds);
	if (wds->cdmac.dmac_dma > 0 && (wds->cdmac.xt_control & XT_DMA_MODE) && (wds->cdmac.xt_status & (XT_STAT_INPUT | XT_STAT_REQUEST) && !(wds->cdmac.xt_status & XT_STAT_COMMAND))) {
		wd->scsi = wds->scsis[XT_UNIT];
		if (do_dma(wds)) {
			xt_command_done(wds, 0);
		}
	}
	wd_check_interrupt(wds, false);

}

static void scsi_hsync2_gvp (struct wd_state *wds)
{
	if (!wds || !wds->enabled)
		return;
	scsi_hsync_check_dma(wds);
	wd_check_interrupt(wds, false);
}

static void scsi_hsync2_comspec(struct wd_state *wds)
{
	if (!wds || !wds->enabled)
		return;
	wd_check_interrupt(wds, false);
}

static void scsi_hsync(void)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		scsi_hsync2_a2091(wd_a2091[i]);
		scsi_hsync2_a2091(wd_a2090[i]);
		scsi_hsync2_gvp(wd_gvps1[i]);
		scsi_hsync2_gvp(wd_gvps2[i]);
		scsi_hsync2_gvp(wd_gvpa1208[i]);
		scsi_hsync2_comspec(wd_comspec[i]);
	}
	scsi_hsync2_gvp(wd_gvps2accel);
	scsi_hsync2_a2091(wd_a3000);
	scsi_hsync2_a2091(wd_cdtv);
}


static int writeonlyreg (int reg)
{
	if (reg == WD_SCSI_STATUS)
		return 1;
	return 0;
}

static void writewdreg (struct wd_chip_state *wd, int sasr, uae_u8 val)
{
	switch (sasr)
	{
	case WD_OWN_ID:
		if (wd->wd33c93_ver == 0)
			val &= ~(0x20 | 0x08);
		else if (wd->wd33c93_ver == 1)
			val &= ~0x20;
		break;
	}
	if (sasr > WD_QUEUE_TAG && sasr < WD_AUXILIARY_STATUS)
		return;
	// queue tag is B revision only
	if (sasr == WD_QUEUE_TAG && wd->wd33c93_ver < 2)
		return;
	wd->wdregs[sasr] = val;
}

void wdscsi_put (struct wd_chip_state *wd, struct wd_state *wds, uae_u8 d)
{
#if WD33C93_DEBUG > 1
	if (WD33C93_DEBUG > 3 || wd->sasr != WD_DATA)
		write_log (_T("W %s REG %02X = %02X (%d) PC=%08X\n"), WD33C93, wd->sasr, d, d, M68K_GETPC);
#endif
	if (!writeonlyreg (wd->sasr)) {
		writewdreg (wd, wd->sasr, d);
	}
	if (!wd->wd_used) {
		wd->wd_used = 1;
		write_log (_T("%s %s in use\n"), wds->bank.name, WD33C93);
	}
	if (wd->sasr == WD_COMMAND_PHASE) {
#if WD33C93_DEBUG > 1
		write_log (_T("%s PHASE=%02X\n"), WD33C93, d);
#endif
		;
	} else if (wd->sasr == WD_DATA) {
#if WD33C93_DEBUG_PIO
		write_log (_T("%s WD_DATA WRITE %02x %d/%d\n"), WD33C93, d, wd->scsi->offset, wd->scsi->data_len);
#endif
		if (!wd->wd_data_avail) {
			write_log (_T("%s WD_DATA WRITE without data request!? %08x\n"), WD33C93, M68K_GETPC);
			return;
		}
		if (wd->wd_dataoffset < sizeof wd->wd_data)
			wd->wd_data[wd->wd_dataoffset] = wd->wdregs[wd->sasr];
		wd->wd_dataoffset++;
		decreasetc (wd);
		wd->wd_data_avail = 1;
		if (scsi_send_data (wd->scsi, wd->wdregs[wd->sasr]) || gettc (wd) == 0) {
			wd->wd_data_avail = 0;
			wd_execute(wds, wd->scsi, 2, 0);
		}
		set_pio_data_irq(wd, wds);
	} else if (wd->sasr == WD_COMMAND) {
		wd->wd_busy = true;
		if (wd->resetnodelay && d == WD_CMD_RESET) {
			// stupid cpu loops that fail if CPU is too fast..
			wd_cmd_reset(wd, true, true);
		} else {
			wd_execute(wds, wds->scsis[wd->wdregs[WD_DESTINATION_ID] & 7], 0, d);
		}
	}
	incsasr (wd, 1);
}

static void wdscsi_put_data(struct wd_chip_state *wd, struct wd_state *wds, uae_u8 v)
{
	uae_u8 sasr = wd->sasr;
	wd->sasr = WD_DATA;
	wdscsi_put(wd, wds, v);
	wd->sasr = sasr;
}

void wdscsi_sasr (struct wd_chip_state *wd, uae_u8 b)
{
	wd->sasr = b;
}
uae_u8 wdscsi_getauxstatus (struct wd_chip_state *wd)
{
	return (wd->auxstatus & ASR_INT) | (wd->wd_busy || wd->wd_data_avail < 0 ? ASR_BSY : 0) | (wd->wd_data_avail != 0 ? ASR_DBR : 0);
}

uae_u8 wdscsi_get (struct wd_chip_state *wd, struct wd_state *wds)
{
	uae_u8 v;
#if WD33C93_DEBUG > 1
	uae_u8 osasr = wd->sasr;
#endif

	v = wd->wdregs[wd->sasr];
	if (wd->sasr == WD_DATA) {
		if (!wd->wd_data_avail) {
			write_log (_T("%s WD_DATA READ without data request!? %08x\n"), WD33C93, M68K_GETPC);
			return 0;
		}
		int status = scsi_receive_data(wd->scsi, &v, true);
#if WD33C93_DEBUG_PIO
		write_log (_T("%s WD_DATA READ %02x %d/%d\n"), WD33C93, v, wd->scsi->offset, wd->scsi->data_len);
#endif
		if (wd->wd_dataoffset < sizeof wd->wd_data)
			wd->wd_data[wd->wd_dataoffset] = v;
		wd->wd_dataoffset++;
		decreasetc (wd);
		wd->wdregs[wd->sasr] = v;
		wd->wd_data_avail = 1;
		if (status || gettc (wd) == 0) {
			wd->wd_data_avail = 0;
			wd_execute(wds, wd->scsi, 3, 0);
		}
		set_pio_data_irq(wd, wds);
	} else if (wd->sasr == WD_SCSI_STATUS) {
		if (wd->auxstatus & ASR_INT) {
			wd->auxstatus &= ~ASR_INT;
			if (wd->resetnodelay_active) {
				wd->queue_index = 0;
			}
			wd->resetnodelay_active = false;
		}
		if (wds->cdtv)
			cdtv_scsi_clear_int ();
		wds->cdmac.dmac_istr &= ~ISTR_INTS;
		wd_check_interrupt(wds, true);
#if 0
		if (wd->wdregs[WD_COMMAND_PHASE] == 0x10) {
			wd->wdregs[WD_COMMAND_PHASE] = 0x11;
			wd->wd_phase = CSR_SRV_REQ | PHS_MESS_OUT;
			set_status (wd, wd->wd_phase, 1);
		}
#endif
	} else if (wd->sasr == WD_AUXILIARY_STATUS) {
		v = wdscsi_getauxstatus (wd);
	}
	incsasr (wd, 0);
#if WD33C93_DEBUG > 1
	if (WD33C93_DEBUG > 3 || osasr != WD_DATA)
		write_log (_T("R %s REG %02X = %02X (%d) PC=%08X\n"), WD33C93, osasr, v, v, M68K_GETPC);
#endif
	return v;
}

static uae_u8 wdscsi_get_data(struct wd_chip_state *wd, struct wd_state *wds)
{
	uae_u8 sasr = wd->sasr;
	wd->sasr = WD_DATA;
	uae_u8 v = wdscsi_get(wd, wds);
	wd->sasr = sasr;
	return v;
}

/* A590 XT */

static void xt_default_geometry(struct wd_state *wds)
{
	wds->cdmac.xt_cyls[0] = wds->wc.scsi->hdhfd->cyls > 1023 ? 1023 : wds->wc.scsi->hdhfd->cyls;
	wds->cdmac.xt_heads[0] = wds->wc.scsi->hdhfd->heads > 31 ? 31 : wds->wc.scsi->hdhfd->heads;
	wds->cdmac.xt_sectors[0] = wds->wc.scsi->hdhfd->secspertrack;
	write_log(_T("XT Default CHS %d %d %d\n"), wds->cdmac.xt_cyls[0], wds->cdmac.xt_heads[0], wds->cdmac.xt_sectors[0]);
}

static void xt_set_status(struct wd_state *wds, uae_u8 state)
{
	wds->cdmac.xt_status = state;
	wds->cdmac.xt_status |= XT_STAT_SELECT;
	wds->cdmac.xt_status |= XT_STAT_READY;
}

static void xt_reset(struct wd_state *wds)
{
	wds->wc.scsi = wds->scsis[XT_UNIT];
	if (!wds->wc.scsi)
		return;
	wds->cdmac.xt_control = 0;
	wds->cdmac.xt_datalen = 0;
	wds->cdmac.xt_status = 0;
	xt_default_geometry(wds);
	write_log(_T("XT reset\n"));
}

static void xt_command_done(struct wd_state *wds, uae_u8 status)
{
	struct scsi_data *scsi = wds->scsis[XT_UNIT];
	if (scsi->direction > 0) {
		if (scsi->cmd[0] == 0x0c) {
			xt_default_geometry(wds);
			int size = wds->cdmac.xt_cyls[0] * wds->cdmac.xt_heads[0] * wds->cdmac.xt_sectors[0];
			wds->cdmac.xt_heads[0] = scsi->buffer[2] & 0x1f;
			wds->cdmac.xt_cyls[0] = (scsi->buffer[0] << 8) | scsi->buffer[1];
			wds->cdmac.xt_sectors[0] = size / (wds->cdmac.xt_cyls[0] * wds->cdmac.xt_heads[0]);
			write_log(_T("XT_SETPARAM: Cyls=%d Heads=%d Sectors=%d\n"), wds->cdmac.xt_cyls[0], wds->cdmac.xt_heads[0], wds->cdmac.xt_sectors[0]);
			for (int i = 0; i < 8; i++) {
				write_log(_T("%02X "), scsi->buffer[i]);
			}
			write_log(_T("\n"));
		} else {
			scsi_emulate_cmd(scsi);
			if (scsi->status || scsi->data_len < 0)
				status = XT_CSB_ERROR;
		}
	}

	xt_set_status(wds, XT_STAT_INTERRUPT);
	if (wds->cdmac.xt_control & XT_INT)
		dmac_a2091_xt_int(wds);
	wds->cdmac.xt_datalen = 0;
	wds->cdmac.xt_statusbyte = status;
#if XT_DEBUG > 0
	write_log(_T("XT command %02x done\n"), wds->cdmac.xt_cmd[0]);
#endif
}

static void xt_wait_data(struct wd_state *wds, int len)
{
	xt_set_status(wds, XT_STAT_REQUEST);
	wds->cdmac.xt_offset = 0;
	wds->cdmac.xt_datalen = len;
}

static void xt_command(struct wd_state *wds)
{
	wds->wc.scsi = wds->scsis[XT_UNIT];
	struct scsi_data *scsi = wds->scsis[XT_UNIT];
#if XT_DEBUG > 0
	write_log(_T("XT command %02x. DMA=%d\n"), wds->cdmac.xt_cmd[0], (wds->cdmac.xt_control & XT_DMA_MODE) ? 1 : 0);
#endif

	memcpy(scsi->cmd, wds->cdmac.xt_cmd, 6);
	scsi->data_len = -1;
	wds->cdmac.xt_datalen = 0;
	wds->cdmac.xt_offset = 0;
	wds->cdmac.xt_statusbyte = 0;
	scsi_emulate_analyze(scsi);
	scsi_start_transfer(scsi);
	if (scsi->direction > 0) {
		xt_set_status(wds, XT_STAT_REQUEST);
	} else if (scsi->direction < 0) {
		scsi_emulate_cmd(scsi);
		if (scsi->status || scsi->data_len < 0) {
			xt_command_done(wds, XT_CSB_ERROR);
		} else {
			xt_set_status(wds, XT_STAT_INPUT);
		}
	} else {
		xt_command_done(wds, 0);
	}
	if (!scsi->status && scsi->data_len >= 0)
		wds->cdmac.xt_datalen = scsi->data_len;
	settc(&wds->wc, wds->cdmac.xt_datalen);
}

static uae_u8 read_xt_reg(struct wd_state *wds, int reg)
{
	uae_u8 v = 0xff;

	wds->wc.scsi = wds->scsis[XT_UNIT];
	if (!wds->wc.scsi)
		return v;

	switch(reg)
	{
	case XD_DATA:
		if (wds->cdmac.xt_status & XT_STAT_INPUT) {
			v = wds->wc.scsi->buffer[wds->cdmac.xt_offset];
#if XT_DEBUG > 1
			write_log(_T("XT data read %02X (%d/%d)\n"), v, wds->cdmac.xt_offset, wds->cdmac.xt_datalen);
#endif
			wds->cdmac.xt_offset++;
			if (wds->cdmac.xt_offset >= wds->cdmac.xt_datalen) {
				xt_command_done(wds, 0);
			}
		} else {
			v = wds->cdmac.xt_statusbyte;
#if XT_DEBUG > 1
			write_log(_T("XT status byte read %02X\n"), v);
#endif
		}
		break;
	case XD_STATUS:
		v = wds->cdmac.xt_status;
		break;
	case XD_JUMPER:
		// 20M: 2 40M: 0, xt.device checks it.
		v = wds->wc.scsi->hdhfd->size >= 41615 * 2 * 512 ? 0 : 2;
		break;
	case XD_RESERVED:
		break;
	}
#if XT_DEBUG > 2
	write_log(_T("XT read %d: %02X\n"), reg, v);
#endif
	return v;
}

static void write_xt_reg(struct wd_state *wds, int reg, uae_u8 v)
{
	wds->wc.scsi = wds->scsis[XT_UNIT];
	if (!wds->wc.scsi)
		return;

#if XT_DEBUG > 2
	write_log(_T("XT write %d: %02X\n"), reg, v);
#endif

	switch (reg)
	{
	case XD_DATA:
		if (!(wds->cdmac.xt_status & XT_STAT_REQUEST)) {
			wds->cdmac.xt_offset = 0;
			xt_set_status(wds, XT_STAT_COMMAND | XT_STAT_REQUEST);
		}
		if (wds->cdmac.xt_status & XT_STAT_REQUEST) {
			if (wds->cdmac.xt_status & XT_STAT_COMMAND) {
#if XT_DEBUG > 1
				write_log(_T("XT command write %02X (%d/6)\n"), v, wds->cdmac.xt_offset);
#endif
				wds->cdmac.xt_cmd[wds->cdmac.xt_offset++] = v;
				xt_set_status(wds, XT_STAT_COMMAND | XT_STAT_REQUEST);
				if (wds->cdmac.xt_offset == 6) {
					xt_command(wds);
				}
			} else {
#if XT_DEBUG > 1
				write_log(_T("XT data write %02X (%d/6)\n"), v, wds->cdmac.xt_offset, wds->cdmac.xt_datalen);
#endif
				wds->wc.scsi->buffer[wds->cdmac.xt_offset] = v;
				wds->cdmac.xt_offset++;
				if (wds->cdmac.xt_offset >= wds->cdmac.xt_datalen) {
					xt_command_done(wds, 0);
				}
			}
#if XT_DEBUG > 1
		} else {
			write_log(_T("XT data write without REQUEST %02X\n"), v);
#endif
		}

		break;
	case XD_RESET:
		xt_reset(wds);
		break;
	case XD_SELECT:
#if XT_DEBUG > 1
		write_log(_T("XT select %02X\n"), v);
#endif
		xt_set_status(wds, XT_STAT_SELECT);
		break;
	case XD_CONTROL:
#if XT_DEBUG > 1
	if ((v & XT_DMA_MODE) != (wds->cdmac.xt_control & XT_DMA_MODE))
			write_log(_T("XT DMA mode=%d\n"), (v & XT_DMA_MODE) ? 1 : 0);
		if ((v & XT_INT) != (wds->cdmac.xt_control & XT_INT))
			write_log(_T("XT IRQ mode=%d\n"), (v & XT_INT) ? 1 : 0);
#endif
		wds->cdmac.xt_control = v;
		wds->cdmac.xt_irq = 0;
		break;
	}
}

/* 8727 DMAC */

static uae_u8 dmac8727_read_pcss(struct wd_state *wd)
{
	uae_u8 v;
	v = wd->cdmac.c8727_pcss;
#if A2091_DEBUG_IO > 0
	write_log(_T("dmac8727_read_pcss %02x\n"), v);
#endif
	return v;
}

static void dmac8727_write_pcss(struct wd_state *wd, uae_u8 v)
{
	wd->cdmac.c8727_pcss = v;
	if (!(wd->cdmac.c8727_pcss & 8)) {
		// 0xF7 1111 0111
		wd->cdmac.dmac_dma = 1;
		wd->cdmac.c8727_pcsd = 0;
	}
#if A2091_DEBUG_IO > 0
	write_log(_T("dmac8727_write_pcss %02x\n"), v);
#endif
}

static uae_u8 dmac8727_read_pcsd(struct wd_state *wd)
{
	struct commodore_dmac *c = &wd->cdmac;
	uae_u8 v = 0;
	if (!(c->c8727_pcss & 8)) {
		// 0xF7 1111 0111
		// driver bug: checks DMA complete without correct mode.
		v = wd->cdmac.c8727_pcsd;
	}
	if (!(c->c8727_pcss & 0x10)) {
		// 0xEF 1110 1111
		// dma complete, no overflow
		v = wd->cdmac.c8727_pcsd;
	}
	v |= 1 << 5; // 1 = no error, 0 = DMA error
	return v;
}

static void dmac8727_write_pcsd(struct wd_state *wd, uae_u8 v)
{
	struct commodore_dmac *c = &wd->cdmac;

	if (!(c->c8727_pcss & 4)) {
		// 0xFB 1111 1011
		c->dmac_acr &= 0xff00ffff;
		c->dmac_acr |= v << 16;
	}
	if (!(c->c8727_pcss & 2)) {
		// 0xFD 1111 1101
		c->dmac_acr &= 0xffff00ff;
		c->dmac_acr |= v << 8;
	}
	if (!(c->c8727_pcss & 1)) {
		// 0xFE 1111 1110
		c->dmac_acr &= 0xffffff00;
		c->dmac_acr |= v << 0;
		c->dmac_wtc = 65535;
	}
	if (!(c->c8727_pcss & 0x80)) {
		c->dmac_dma = 0;
	}
#if 0
	switch (c->c8727_pcss)
	{
		case 0x7f:
		case 0xff:
		c->dmac_dma = 0;
		break;
	}
#endif
}

static void dmac8727_cbp(struct wd_state *wd)
{
	struct commodore_dmac *c = &wd->cdmac;

	c->c8727_st506_cb >>= 8;
	c->c8727_st506_cb |= c->c8727_wrcbp << (8 + 9);
}

static void a2090_st506(struct wd_state *wd, uae_u8 b)
{
	uae_u8 cb[16];

	if (wd->rc->device_settings & 1)
		return;
	if (!b) {
		wd->cdmac.dmac_istr &= ~(ISTR_INT_F | ISTR_INTS);
		return;
	}
	uaecptr cbp = wd->cdmac.c8727_st506_cb & ~1;
	if (!valid_address(cbp, 16)) {
		write_log(_T("Invalid ST-506 command block address %08x\n"), cbp);
		return;
	}
	for (int i = 0; i < sizeof cb; i++) {
		cb[i] = get_byte(cbp + i);
	}
	int lun = cb[1] >> 5;
	int unit = (lun & 1) ? 1 : 0;
	uaecptr dmaaddr = (cb[6] << 16) | (cb[7] << 8) | cb[8];
	memset(cb + 12, 0, 4);
	struct scsi_data *scsi = wd->scsis[XT506_UNIT0 + unit];
	if (scsi) {
		uae_u8 cmd = cb[0];
		memcpy(scsi->cmd, cb, 6);
		write_log(_T("ST-506 CMD %08X %02x.%02x.%02x.%02x.%02x.%02x %02x.%02x.%02x\n"),
			cbp,
			cb[0], cb[1], cb[2], cb[3], cb[4], cb[5],
			cb[6], cb[7], cb[8]);
		// Set Drive Parameters? 0x0c = both drives, 0xcc = drive 1 only.
		if (cmd == 0x0c || cmd == 0xcc) {
			uae_u8 sdp[6];
			for (int i = 0; i < sizeof sdp; i++) {
				sdp[i] = get_byte(dmaaddr + i);
			}
			uae_u8 user_options = sdp[0] >> 4;
			uae_u8 step_rate = sdp[0] & 15;
			uae_u8 heads = (sdp[1] >> 4) & 15;
			uae_u16 cyls = ((sdp[1] & 15) << 8) | sdp[2];
			uae_u8 precomp = sdp[3];
			uae_u8 reduce_write_current = sdp[4];
			uae_u8 secs_per_track = sdp[5];
			write_log(_T("ST-506 Set Drive Parameters %02X: CHS=%d,%d,%d %02x %02x %02x %02x\n"),
				cmd, cyls, secs_per_track, heads,
				user_options, step_rate, precomp, reduce_write_current);
			wd->cdmac.xt_cyls[1] = cyls;
			wd->cdmac.xt_heads[1] = heads;
			wd->cdmac.xt_sectors[1] = secs_per_track;
			if (cmd == 0x0c) {
				wd->cdmac.xt_cyls[0] = cyls;
				wd->cdmac.xt_heads[0] = heads;
				wd->cdmac.xt_sectors[0] = secs_per_track;
			}
		} else if (cmd == 0x0f) { // change command block address
			uae_u8 ccba[4];
			for (int i = 0; i < sizeof ccba; i++) {
				ccba[i] = get_byte(dmaaddr + i);
			}
			wd->cdmac.c8727_st506_cb = ((ccba[1] << 16) | (ccba[2] << 8) | (ccba[3] << 0)) & ~1;
			// return code must be written to new address
			cbp = wd->cdmac.c8727_st506_cb;
			write_log(_T("ST-506 Change Command Block %02x.%02x.%02x.%02x\n"),
				ccba[0], ccba[1], ccba[2], ccba[3]);
		} else if (cmd == 0x00 || cmd == 0x01 || cmd == 0x03 || cmd == 0x05 || cmd == 0x06 || cmd == 0x08 || cmd == 0x0a || cmd == 0x0b) {
			bool outofbounds = false;
			if (cmd == 0x05 || cmd == 0x06 || cmd == 0x08 || cmd == 0x0a) {
				// limit check
				uae_u32 lba = ((cb[1] & 31) << 16) | (cb[2] << 8) | cb[3];
				uae_u32 maxlba = wd->cdmac.xt_cyls[unit] * wd->cdmac.xt_heads[unit] * wd->cdmac.xt_sectors[unit];
				int blocks = cb[4] == 0 ? 256 : cb[4];
				if (lba + blocks > maxlba) {
					outofbounds = true;
				}
			}
			if (!outofbounds) {
				// We handle LUN here, SCSI emulation always sees zero LUN.
				scsi->cmd[1] &= ~0x20;
				scsi->cmd_len = 6;
				scsi_emulate_analyze(scsi);
				if (scsi->direction < 0) {
					scsi_emulate_cmd(scsi);
					for (int i = 0; i < scsi->data_len; i++) {
						put_byte(dmaaddr + i, scsi->buffer[i]);
					}
				} else {
					for (int i = 0; i < scsi->data_len; i++) {
						scsi->buffer[i] = get_byte(dmaaddr + i);
					}
					scsi_emulate_cmd(scsi);
				}
				if (scsi->status && scsi->sense_len) {
					memcpy(cb + 12, scsi->sense, 4);
				}
			} else {
				cb[12] = 0x21; // invalid sector address
			}
			if (cmd == 0x05 || cmd == 0x06 || cmd == 0x08 || cmd == 0x0a) {
				cb[12] |= 0x80; // ADV
				// Calculate LBA of last transferred block
				uae_u32 lba = ((cb[1] & 31) << 16) | (cb[2] << 8) | cb[3];
				uae_u32 add;
				if (cmd == 0x05 || cmd == 0x06)
					add = cb[4];
				else
					add = scsi->data_len / 512;
				if (add)
					lba += add - 1;
				cb[13] = ((lba >> 16) & 31) | (lun << 5);
				cb[14] = lba >> 8;
				cb[15] = lba >> 0;
			}
		} else {
			cb[12] = 0x20; // invalid command
		}
	} else {
		cb[12] = 0x04; // drive not ready
	}
	// return status bytes
	for (int i = 12; i < sizeof cb; i++) {
		put_byte(cbp + i, cb[i]);
	}
	set_dma_done(wd);
	wd->cdmac.xt_irq = true;
}

static uae_u32 dmac_a2091_read_word (struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0;

	if (wd->configured == 0xf1) {
		if (wd->rom && !(addr & 0x8000)) {
			int off = addr & wd->rom_mask;
			return (wd->rom[off] << 8) | wd->rom[off + 1];
		}
		return 0;
	}

	if (addr < 0x40)
		return (wd->dmacmemory[addr] << 8) | wd->dmacmemory[addr + 1];

	if (wd->dmac_type == COMMODORE_8727) {

		if (addr >= CDMAC_ROM_OFFSET) {
			if (wd->rom) {
				int off = addr & wd->rom_mask;
				return (wd->rom[off] << 8) | wd->rom[off + 1];
			}
			return 0;
		}
		switch (addr)
		{
		case 0x40:
			v = 0;
			if (wd->cdmac.dmac_istr & ISTR_INTS)
				v |= (1 << 7) | (1 << 4);
			break;
		case 0x42:
			v = 0;
			if (wd->cdmac.dmac_cntr & CNTR_INTEN)
				v |= 1 << 4;
			v |= wd->cdmac.c8727_ctl & 0x80;
			break;
		case 0x60:
			v = wdscsi_getauxstatus (&wd->wc);
			break;
		case 0x62:
			v = wdscsi_get (&wd->wc, wd);
			break;
		case 0x64:
			v = dmac8727_read_pcss(wd);
			break;
		case 0x68:
			v = dmac8727_read_pcsd(wd);
			break;
		}
		v <<= 8;
		return v;

	} else if (wd->dmac_type == COMMODORE_DMAC) {
		if (addr >= CDMAC_ROM_OFFSET) {
			if (wd->rom) {
				int off = addr & wd->rom_mask;
				if (wd->rombankswitcher && (addr & 0xffe0) == CDMAC_ROM_OFFSET)
					wd->rombank = (addr & 0x02) >> 1;
				off += wd->rombank * wd->rom_size;
				return (wd->rom[off] << 8) | wd->rom[off + 1];
			}
			return 0;
		}

		addr &= ~1;
		switch (addr)
		{
		case 0x40:
			v = wd->cdmac.dmac_istr;
			if ((v & (ISTR_E_INT | ISTR_INTS)) && (wd->cdmac.dmac_cntr & CNTR_INTEN))
				v |= ISTR_INT_P;
			wd->cdmac.dmac_istr &= ~0xf;
			break;
		case 0x42:
			v = wd->cdmac.dmac_cntr;
			break;
		case 0x80:
			if (wd->cdmac.old_dmac)
				v = (wd->cdmac.dmac_wtc >> 16) & 0xffff;
			break;
		case 0x82:
			if (wd->cdmac.old_dmac)
				v = wd->cdmac.dmac_wtc & 0xffff;
			break;
		case 0x90:
			v = wdscsi_getauxstatus(&wd->wc);
			break;
		case 0x92:
			v = wdscsi_get(&wd->wc, wd);
			break;
		case 0xc0:
			v = 0xf8 | (1 << 0) | (1 << 1) | (1 << 2); // bits 0-2 = dip-switches
			break;
		case 0xc2:
		case 0xc4:
		case 0xc6:
			v = 0xffff;
			break;
		case 0xe0:
			if (wd->cdmac.dmac_dma <= 0)
				scsi_dmac_a2091_start_dma (wd);
			break;
		case 0xe2:
			scsi_dmac_a2091_stop_dma (wd);
			break;
		case 0xe4:
			dmac_a2091_cint (wd);
			break;
		case 0xe8:
			/* FLUSH (new only) */
			if (!wd->cdmac.old_dmac && wd->cdmac.dmac_dma > 0)
				wd->cdmac.dmac_istr |= ISTR_FE_FLG;
			break;
		}
	}
#if A2091_DEBUG_IO > 0
	write_log (_T("dmac_wget %04X=%04X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u32 dmac_a2091_read_byte (struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0;

	if (wd->configured == 0xf1) {
		if (wd->rom && !(addr & 0x8000)) {
			int off = addr & wd->rom_mask;
			return wd->rom[off];
		}
		return 0;
	}

	if (addr < 0x40)
		return wd->dmacmemory[addr];

	if (wd->dmac_type == COMMODORE_8727) {

		if (addr >= DMAC_8727_ROM_VECTOR) {
			if (wd->rom) {
				int off = addr & wd->rom_mask;
				return wd->rom[off];
			}
			return 0;
		}
		switch (addr)
		{
		case 0x40:
			v = 0;
			if (wd->cdmac.dmac_istr & ISTR_INTS)
				v |= (1 << 7) | (1 << 4);
			break;
		case 0x42:
			v = 0;
			if (wd->cdmac.dmac_cntr & CNTR_INTEN)
				v |= 1 << 4;
			v |= wd->cdmac.c8727_ctl & 0x80;
			break;
		case 0x60:
			v = wdscsi_getauxstatus (&wd->wc);
			break;
		case 0x62:
			v = wdscsi_get (&wd->wc, wd);
			break;
		case 0x64:
			v = dmac8727_read_pcss(wd);
			break;
		case 0x68:
			v = dmac8727_read_pcsd(wd);
			break;
		}

	} else if (wd->dmac_type == COMMODORE_DMAC) {
		if (addr >= CDMAC_ROM_OFFSET) {
			if (wd->rom) {
				int off = addr & wd->rom_mask;
				if (wd->rombankswitcher && (addr & 0xffe0) == CDMAC_ROM_OFFSET)
					wd->rombank = (addr & 0x02) >> 1;
				off += wd->rombank * wd->rom_size;
				return wd->rom[off];
			}
			return 0;
		}

		switch (addr)
		{
		case 0x91:
			v = wdscsi_getauxstatus (&wd->wc);
			break;
		case 0x93:
			v = wdscsi_get (&wd->wc, wd);
			break;
		case 0xa1:
		case 0xa3:
		case 0xa5:
		case 0xa7:
			v = read_xt_reg(wd, (addr - 0xa0) / 2);
			break;
		default:
			v = dmac_a2091_read_word (wd, addr);
			if (!(addr & 1))
				v >>= 8;
			break;
		}
	}
#if A2091_DEBUG_IO > 0
	write_log (_T("dmac_bget %04X=%02X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
	return v;
}

static void dmac_a2091_write_word (struct wd_state *wd, uaecptr addr, uae_u32 b)
{
#if A2091_DEBUG_IO > 0
		write_log (_T("dmac_wput %04X=%04X PC=%08X\n"), addr, b & 65535, M68K_GETPC);
#endif

	if (wd->configured == 0xf1) {
		return;
	}

	if (addr < 0x40)
		return;

	if (wd->dmac_type == COMMODORE_8727) {

		if (addr >= DMAC_8727_ROM_VECTOR)
			return;

		b >>= 8;
		switch(addr)
		{
		case 0x40:
			break;
		case 0x42:
			wd->cdmac.dmac_cntr &= ~(CNTR_INTEN | CNTR_PDMD);
			if (b & (1 << 4))
				wd->cdmac.dmac_cntr |= CNTR_INTEN;
			wd->cdmac.c8727_ctl = b;
			if (b & 0x80)
				dmac8727_cbp(wd);
			break;
		case 0x50:
			// clear interrupt
			wd->cdmac.dmac_istr &= ~(ISTR_INT_F | ISTR_INTS);
			wd->cdmac.xt_irq = false;
			wd->cdmac.c8727_wrcbp = b;
			break;
		case 0x52:
			a2090_st506(wd, b);
			break;
		case 0x60:
			wdscsi_sasr (&wd->wc, b);
			break;
		case 0x62:
			wdscsi_put (&wd->wc, wd, b);
			break;
		case 0x64:
			dmac8727_write_pcss(wd, b);
			break;
		case 0x68:
			dmac8727_write_pcsd(wd, b);
			break;
		}


	} else if (wd->dmac_type == COMMODORE_DMAC) {

		if (addr >= CDMAC_ROM_OFFSET)
			return;

		addr &= ~1;
		switch (addr)
		{
		case 0x42:
			wd->cdmac.dmac_cntr = b;
			if (wd->cdmac.dmac_cntr & CNTR_PREST)
				dmac_reset (wd);
			break;
		case 0x80:
			wd->cdmac.dmac_wtc &= 0x0000ffff;
			wd->cdmac.dmac_wtc |= b << 16;
			break;
		case 0x82:
			wd->cdmac.dmac_wtc &= 0xffff0000;
			wd->cdmac.dmac_wtc |= b & 0xffff;
			break;
		case 0x84:
			wd->cdmac.dmac_acr &= 0x0000ffff;
			wd->cdmac.dmac_acr |= b << 16;
			break;
		case 0x86:
			wd->cdmac.dmac_acr &= 0xffff0000;
			wd->cdmac.dmac_acr |= b & 0xfffe;
			if (wd->cdmac.old_dmac)
				wd->cdmac.dmac_acr &= ~3;
			break;
		case 0x8e:
			wd->cdmac.dmac_dawr = b;
			break;
		case 0x90:
			wdscsi_sasr (&wd->wc, b);
			break;
		case 0x92:
			wdscsi_put (&wd->wc, wd, b);
			break;
		case 0xc2:
		case 0xc4:
		case 0xc6:
			break;
		case 0xe0:
			if (wd->cdmac.dmac_dma <= 0)
				scsi_dmac_a2091_start_dma (wd);
			break;
		case 0xe2:
			scsi_dmac_a2091_stop_dma (wd);
			break;
		case 0xe4:
			dmac_a2091_cint (wd);
			break;
		case 0xe8:
			/* FLUSH */
			wd->cdmac.dmac_istr |= ISTR_FE_FLG;
			break;
		}
	}
}

static void dmac_a2091_write_byte (struct wd_state *wd, uaecptr addr, uae_u32 b)
{
#if A2091_DEBUG_IO > 0
		write_log (_T("dmac_bput %04X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif
	
	if (wd->configured == 0xf1) {
		return;
	}

	if (addr < 0x40)
		return;

	if (wd->dmac_type == COMMODORE_8727) {

		if (addr >= DMAC_8727_ROM_VECTOR)
			return;

		switch(addr)
		{
		case 0x40:
			break;
		case 0x42:
			wd->cdmac.dmac_cntr &= ~CNTR_INTEN;
			if (b & (1 << 4))
				wd->cdmac.dmac_cntr |= CNTR_INTEN;
			wd->cdmac.c8727_ctl = b;
			if (b & 0x80)
				dmac8727_cbp(wd);
			break;
		case 0x50:
			// clear interrupt
			wd->cdmac.dmac_istr &= ~(ISTR_INT_F | ISTR_INTS);
			wd->cdmac.xt_irq = false;
			wd->cdmac.c8727_wrcbp = b;
			break;
		case 0x52:
			a2090_st506(wd, b);
			break;
		case 0x60:
			wdscsi_sasr (&wd->wc, b);
			break;
		case 0x62:
			wdscsi_put (&wd->wc, wd, b);
			break;
		case 0x64:
			dmac8727_write_pcss(wd, b);
			break;
		case 0x68:
			dmac8727_write_pcsd(wd, b);
			break;
		}

	} else if (wd->dmac_type == COMMODORE_DMAC) {

		if (addr >= CDMAC_ROM_OFFSET)
			return;

		switch (addr)
		{
		case 0x91:
			wdscsi_sasr (&wd->wc, b);
			break;
		case 0x93:
			wdscsi_put (&wd->wc, wd, b);
			break;
		case 0xa1:
		case 0xa3:
		case 0xa5:
		case 0xa7:
			write_xt_reg(wd, (addr - 0xa0) / 2, b);
			break;
		default:
			if (addr & 1)
				dmac_a2091_write_word (wd, addr, b);
			else
				dmac_a2091_write_word (wd, addr, b << 8);
		}
	}
}

static uae_u32 REGPARAM2 dmac_a2091_lget (struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = dmac_a2091_read_word(wd, addr) << 16;
	v |= dmac_a2091_read_word(wd, addr + 2) & 0xffff;
	return v;
}

static uae_u32 REGPARAM2 dmac_a2091_wget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = dmac_a2091_read_word(wd, addr);
	return v;
}

static uae_u32 REGPARAM2 dmac_a2091_bget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = dmac_a2091_read_byte(wd, addr);
	return v;
}

static void REGPARAM2 dmac_a2091_lput(struct wd_state *wd, uaecptr addr, uae_u32 l)
{
	addr &= 65535;
	dmac_a2091_write_word(wd, addr + 0, l >> 16);
	dmac_a2091_write_word(wd, addr + 2, l);
}

static void REGPARAM2 dmac_a2091_wput(struct wd_state *wd, uaecptr addr, uae_u32 w)
{
	addr &= 65535;
	dmac_a2091_write_word(wd, addr, w);
}

static void REGPARAM2 dmac_a2091_bput(struct wd_state *wd, uaecptr addr, uae_u32 b)
{
	b &= 0xff;
	addr &= 65535;
	if (wd->autoconfig) {
		if (addr == 0x48 && !wd->configured) {
			map_banks_z2 (&wd->bank, b, 0x10000 >> 16);
			wd->baseaddress = b << 16;
			wd->configured = 1;
			expamem_next (&wd->bank, NULL);
			return;
		}
		if (addr == 0x4c && !wd->configured) {
			wd->configured = 1;
			expamem_shutup(&wd->bank);
			return;
		}
		if (!wd->configured)
			return;
	}
	dmac_a2091_write_byte(wd, addr, b);
}

static uae_u32 REGPARAM2 dmac_a2091_wgeti(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0xffff;
	addr &= 65535;
	if (addr >= CDMAC_ROM_OFFSET || wd->configured == 0xf1)
		v = (wd->rom[addr & wd->rom_mask] << 8) | wd->rom[(addr + 1) & wd->rom_mask];
	else
		write_log(_T("Invalid DMAC instruction access %08x\n"), addr);
	return v;
}
static uae_u32 REGPARAM2 dmac_a2091_lgeti(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = dmac_a2091_wgeti(wd, addr) << 16;
	v |= dmac_a2091_wgeti(wd, addr + 2);
	return v;
}

static int REGPARAM2 dmac_a2091_check(struct wd_state *wd, uaecptr addr, uae_u32 size)
{
	return 1;
}

static uae_u8 *REGPARAM2 dmac_a2091_xlate(struct wd_state *wd, uaecptr addr)
{
	addr &= 0xffff;
	addr += wd->rombank * wd->rom_size;
	if (addr >= 0xc000)
		addr = 0xc000;
	return wd->rom + addr;
}

static uae_u8 *REGPARAM2 dmac_a2091_xlate (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_xlate(wd, addr);
	return default_xlate(0);
}
static int REGPARAM2 dmac_a2091_check (uaecptr addr, uae_u32 size)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_check(wd, addr, size);
	return 0;
}
static uae_u32 REGPARAM2 dmac_a2091_lgeti (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_lgeti(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_a2091_wgeti (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_wgeti(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_a2091_bget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_bget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_a2091_wget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_wget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_a2091_lget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_a2091_lget(wd, addr);
	return 0;
}
static void REGPARAM2 dmac_a2091_bput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		dmac_a2091_bput(wd, addr, b);
}
static void REGPARAM2 dmac_a2091_wput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		dmac_a2091_wput(wd, addr, b);
}
static void REGPARAM2 dmac_a2091_lput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		dmac_a2091_lput(wd, addr, b);
}

static const addrbank dmaca2091_bank = {
	dmac_a2091_lget, dmac_a2091_wget, dmac_a2091_bget,
	dmac_a2091_lput, dmac_a2091_wput, dmac_a2091_bput,
	dmac_a2091_xlate, dmac_a2091_check, NULL, _T("*"), _T("A2090/A2091/A590"),
	dmac_a2091_lgeti, dmac_a2091_wgeti,
	ABFLAG_IO | ABFLAG_SAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

static void REGPARAM2 combitec_put(uaecptr addr, uae_u32 b)
{
}
static uae_u32 REGPARAM2 combitec_bget(uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (!wd)
		return 0;
	addr &= 65535;
	uae_u32 v = wd->rom2[addr & wd->rom2_mask];
	return v;
}
static uae_u32 REGPARAM2 combitec_wget(uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (!wd)
		return 0;
	addr &= 65535;
	uae_u32 v = (wd->rom2[addr & wd->rom2_mask] << 8) | wd->rom2[(addr + 1) & wd->rom2_mask];
	return v;
}
static uae_u32 REGPARAM2 combitec_lget(uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = combitec_wget(addr) << 16;
	v |= combitec_wget(addr + 2);
	return v;
}
static int REGPARAM2 combitec_check(uaecptr addr, uae_u32 size)
{
	struct wd_state *wd = getscsiboard(addr);
	if (!wd)
		return 0;
	return 1;
}
static uae_u8 *REGPARAM2 combitec_xlate(uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (!wd)
		return NULL;
	addr &= 0xffff;
	return wd->rom + addr;
}
static const addrbank combitec_bank = {
	combitec_lget, combitec_wget, combitec_bget,
	combitec_put, combitec_put, combitec_put,
	combitec_xlate, combitec_check, NULL, _T("*"), _T("A2090 Combitec"),
	combitec_lget, combitec_wget,
	ABFLAG_IO | ABFLAG_SAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

static bool comspec_wd_aux(uaecptr addr)
{
	return addr == 0xc1;
}
static bool comspec_wd_data(uaecptr addr)
{
	return addr == 0xc3;
}
static uae_u8 comspec_status(struct wd_state *wd)
{
	uae_u8 v = wd->comspec.status;
	v &= ~0x80;
	if (wd->wc.wd_data_avail)
		v |= 0x80;
	return v;
}

static uae_u8 comspec_read_byte(struct wd_state *wd, uaecptr addr)
{
	uae_u8 v = 0;
	uaecptr oaddr = addr;
	addr &= 65535;
	if (!wd->configured && addr < 0x40 && (oaddr & 0xf00000) != 0xf00000) {
		v = wd->dmacmemory[addr];
	} else if (!(addr & 0x8000)) {
		v = wd->rom[addr];
	} else {
		addr &= 0x7fff;
		if ((addr & 0xf1) == 0x80) {
			v = comspec_status(wd);
		} else if ((addr & 0xf1) == 0x81) {
			v = wdscsi_get_data(&wd->wc, wd);
		} else if ((addr & 0xa0) == 0xa0) {
			if (wd->comspec.status & 1) {
				int rtc = (addr >> 1) & 15;
				v = get_clock_msm((struct rtc_msm_data*)wd->userdata, rtc, NULL);
			}
		} else {
			if (comspec_wd_aux(addr)) {
				v = wdscsi_getauxstatus(&wd->wc);
			} else if (comspec_wd_data(addr)) {
				v = wdscsi_get (&wd->wc, wd);
			}
		}
#if COMSPEC_DEBUG > 1
		write_log(_T("COMSPEC BGET %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
	}
	return v;
}

static void comspec_write_byte(struct wd_state *wd, uaecptr addr, uae_u8 v)
{
	addr &= 65535;
	if (addr & 0x8000) {
		addr &= 0x7fff;
		if ((addr & 0xf0) == 0x80) {
			wd->comspec.status = v & 0x7f;
			if (!(v & 0x40)) {
				wd_master_reset(wd, true);
			}
#if COMSPEC_DEBUG > 0
			write_log(_T("COMSPEC STATUS = %02X\n"), wd->comspec.status);
#endif
		} else if ((addr & 0xf1) == 0xe1) {
			wdscsi_put_data(&wd->wc, wd, v);
		} else if ((addr & 0xa0) == 0xa0) {
			if (wd->comspec.status & 1) {
				int rtc = (addr >> 1) & 15;
				put_clock_msm((struct rtc_msm_data*)wd->userdata, rtc, v);
			}
		} else {
			if (comspec_wd_aux(addr)) {
				wdscsi_sasr(&wd->wc, v);
			} else if (comspec_wd_data(addr)) {
				wdscsi_put(&wd->wc, wd, v);
			}
		}
	}
#if COMSPEC_DEBUG > 1
	write_log(_T("COMSPEC BPUT %08x %02x %08x\n"), addr, v, M68K_GETPC);
#endif
}

static uae_u16 comspec_read_word(struct wd_state *wd, uaecptr addr)
{
	uae_u16 v = 0;
	addr &= 65535;
	if (!(addr & 0x8000)) {
		v = (wd->rom[addr] << 8) | wd->rom[(addr + 1) & 0x7fff];
	} else {
		addr &= 0x7fff;
		if ((addr & 0xf0) == 0x80) {
			v = comspec_status(wd) << 8;
			v |= wdscsi_get_data(&wd->wc, wd);
		}
#if COMSPEC_DEBUG > 1
		write_log(_T("COMSPEC WGET %08x %04x %08x\n"), addr, v, M68K_GETPC);
#endif
	}
	return v;
}

static void comspec_write_word(struct wd_state *wd, uaecptr addr, uae_u16 v)
{
#if COMSPEC_DEBUG > 1
	addr &= 65535;
	if (addr & 0x8000) {
		write_log(_T("COMSPEC BWUT %08x %04x %08x\n"), addr, v, M68K_GETPC);
	}
#endif
}

static int REGPARAM2 comspec_check(struct wd_state *wd, uaecptr addr, uae_u32 size)
{
	return 1;
}

static uae_u8 *REGPARAM2 comspec_xlate(struct wd_state *wd, uaecptr addr)
{
	addr &= 0xffff;
	return wd->rom + addr;
}

static uae_u32 REGPARAM2 comspec_lget (struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	v = comspec_read_word(wd, addr) << 16;
	v |= comspec_read_word(wd, addr + 2) & 0xffff;
	return v;
}

static uae_u32 REGPARAM2 comspec_wget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	v = comspec_read_word(wd, addr);
	return v;
}

static uae_u32 REGPARAM2 comspec_bget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	v = comspec_read_byte(wd, addr);
	return v;
}

static void REGPARAM2 comspec_lput(struct wd_state *wd, uaecptr addr, uae_u32 l)
{
	comspec_write_word(wd, addr + 0, l >> 16);
	comspec_write_word(wd, addr + 2, l);
}

static void REGPARAM2 comspec_wput(struct wd_state *wd, uaecptr addr, uae_u32 w)
{
	comspec_write_word(wd, addr, w);
}

static void REGPARAM2 comspec_bput(struct wd_state *wd, uaecptr addr, uae_u32 b)
{
	b &= 0xff;
	if (wd->autoconfig && (addr & 0xf00000) != 0xf00000) {
		addr &= 65535;
		if (addr == 0x48 && !wd->configured) {
			map_banks_z2 (&wd->bank, b, 0x10000 >> 16);
			wd->baseaddress = b << 16;
			wd->configured = 1;
			if (!wd->rc->autoboot_disabled && wd->rc->subtype == 0) {
				map_banks(&dummy_bank, 0xf00000 >> 16, 1, 0);
			}
			expamem_next (&wd->bank, NULL);
			return;
		}
		if (addr == 0x4c && !wd->configured) {
			wd->configured = 1;
			if (!wd->rc->autoboot_disabled && wd->rc->subtype == 0) {
				map_banks(&dummy_bank, 0xf00000 >> 16, 1, 0);
			}
			expamem_shutup(&wd->bank);
			return;
		}
		if (!wd->configured)
			return;
	}
	comspec_write_byte(wd, addr, b);
}

static uae_u32 REGPARAM2 comspec_wgeti(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0xffff;
	addr &= 65535;
	if (!(addr & 0x8000))
		v = (wd->rom[addr] << 8) | wd->rom[(addr + 1) & 0x7fff];
	return v;
}
static uae_u32 REGPARAM2 comspec_lgeti(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= 65535;
	v = comspec_wgeti(wd, addr) << 16;
	v |= comspec_wgeti(wd, addr + 2);
	return v;
}

static uae_u8 *REGPARAM2 comspec_xlate (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_xlate(wd, addr);
	return default_xlate(0);
}
static int REGPARAM2 comspec_check (uaecptr addr, uae_u32 size)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_check(wd, addr, size);
	return 0;
}
static uae_u32 REGPARAM2 comspec_lgeti (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_lgeti(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 comspec_wgeti (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_wgeti(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 comspec_bget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_bget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 comspec_wget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_wget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 comspec_lget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return comspec_lget(wd, addr);
	return 0;
}
static void REGPARAM2 comspec_bput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		comspec_bput(wd, addr, b);
}
static void REGPARAM2 comspec_wput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		comspec_wput(wd, addr, b);
}
static void REGPARAM2 comspec_lput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		comspec_lput(wd, addr, b);
}

static const addrbank comspec_bank = {
	comspec_lget, comspec_wget, comspec_bget,
	comspec_lput, comspec_wput, comspec_bput,
	comspec_xlate, comspec_check, NULL, _T("*"), _T("COMSPEC"),
	comspec_lgeti, comspec_wgeti,
	ABFLAG_IO | ABFLAG_SAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};


/* GVP Series I and II */

static uae_u32 dmac_gvp_read_byte(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0;

	addr &= wd->board_mask;
	if (addr < 0x3e) {
		v = wd->dmacmemory[addr];
	} else if ((addr & 0x8000) == GVP_ROM_OFFSET) {
		addr &= 0xffff;
		if (wd->gdmac.series2) {
			if (addr & 1) {
				v = wd->gdmac.version;
			} else {
				if (wd->rom) {
					if (wd->rombankswitcher && (addr & 0xffe0) == GVP_ROM_OFFSET)
						wd->rombank = (addr & 0x02) >> 1;
					v = wd->rom[(addr - GVP_ROM_OFFSET) / 2 + wd->rombank * 16384];
				}
			}
		} else {
			if ((addr & 1) && wd->gdmac.use_version) {
				v = wd->gdmac.version;
			} else if (wd->rom) {
				v = wd->rom[addr - GVP_ROM_OFFSET];
			}
		}
	} else if (addr >= wd->gdmac.s1_ramoffset && !wd->gdmac.series2) {
#if GVP_S1_DEBUG_IO > 1
		int off = wd->gdmac.bufoffset;
#endif
		v =  wd->gdmac.buffer[wd->gdmac.bufoffset++];
		wd->gdmac.bufoffset &= wd->gdmac.s1_rammask;
#if GVP_S1_DEBUG_IO > 1
		write_log(_T("gvp_s1_bget sram %d %04x\n"), off, v);
#endif
	} else if (wd->configured) {
		if (wd->gdmac.series2) {
			switch (addr)
			{
				case 0x40:
				v = wd->gdmac.cntr >> 8;
				break;
				case 0x41:
				v = wd->gdmac.cntr;
				break;
				case 0x61: // SASR
				v = wdscsi_getauxstatus(&wd->wc);
				break;
				case 0x63: // SCMD
				v = wdscsi_get(&wd->wc, wd);
				break;
				default:
				write_log(_T("gvp_s2_bget_unk %04X PC=%08X\n"), addr, M68K_GETPC);
				break;
			}
		} else {
			switch (addr)
			{
				case 0x3e:
				v = (wd->wc.auxstatus & ASR_INT) ? 0x80 : 0x00;
				break;
				case 0x60: // SASR
				v = wdscsi_getauxstatus(&wd->wc);
				break;
				case 0x62: // SCMD
				v = wdscsi_get(&wd->wc, wd);
				break;
				case 0x68:
				v = 0;
				break;
				default:
				write_log(_T("gvp_s1_bget_unk %04X PC=%08X\n"), addr, M68K_GETPC);
				break;
			}
		}
	} else {
		v = 0xff;
	}

#if GVP_S2_DEBUG_IO > 0
	write_log(_T("gvp_bget %04X=%02X PC=%08X\n"), addr, v, M68K_GETPC);
#endif

	return v;
}

static uae_u32 dmac_gvp_read_word(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0;

	addr &= wd->board_mask;
	if (addr < 0x3e) {
		v = (wd->dmacmemory[addr] << 8) | wd->dmacmemory[addr + 1];
	} else if ((addr & 0x8000) == GVP_ROM_OFFSET) {
		addr &= 0xffff;
		if (wd->gdmac.series2) {
			if (wd->rom) {
				if (wd->rombankswitcher && (addr & 0xffe0) == GVP_ROM_OFFSET)
					wd->rombank = (addr & 0x02) >> 1;
				v = (wd->rom[(addr - GVP_ROM_OFFSET) / 2 + wd->rombank * 16384] << 8) | wd->gdmac.version;
			} else {
				v = wd->gdmac.version;
			}
		} else {
			if (wd->rom) {
				v = (wd->rom[addr - GVP_ROM_OFFSET] << 8) | (wd->rom[addr - GVP_ROM_OFFSET + 1]);
			}
			if (wd->gdmac.use_version) {
				v &= 0xff00;
				v |= wd->gdmac.version;
			}
		}
	} else if (addr >= wd->gdmac.s1_ramoffset && !wd->gdmac.series2) {
#if GVP_S1_DEBUG_IO > 1
		int off = wd->gdmac.bufoffset;
#endif
		v =  wd->gdmac.buffer[wd->gdmac.bufoffset++] << 8;
		wd->gdmac.bufoffset &= wd->gdmac.s1_rammask;
		v |= wd->gdmac.buffer[wd->gdmac.bufoffset++] << 0;
		wd->gdmac.bufoffset &= wd->gdmac.s1_rammask;
#if GVP_S1_DEBUG_IO > 1
		write_log(_T("gvp_s1_wget sram %d %04x\n"), off, v);
#endif
	} else if (wd->configured) {
		if (wd->gdmac.series2) {
			switch (addr)
			{
				case 0x40:
				v = wd->gdmac.cntr;
				break;
				case 0x68:
				v = (wd->gdmac.bank << 6) | (wd->gdmac.maprom & 0x3f);
				break;
				case 0x70:
				v = wd->gdmac.addr >> 16;
				break;
				case 0x72:
				v = wd->gdmac.addr;
				break;
				default:
				write_log(_T("gvp_s2_wget_unk %04X PC=%08X\n"), addr, M68K_GETPC);
				break;
			}
#if GVP_S2_DEBUG_IO > 0
			write_log(_T("gvp_s2_wget %04X=%04X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
		} else {

			v = dmac_gvp_read_byte(wd, addr) << 8;
			v |= dmac_gvp_read_byte(wd, addr + 1);

#if GVP_S1_DEBUG_IO > 0
			write_log(_T("gvp_s1_wget %04X=%04X PC=%08X\n"), addr, v, M68K_GETPC);
#endif
		}
	} else {
		v = 0xffff;
	}

	return v;
}

static void dmac_gvp_write_byte(struct wd_state *wd, uaecptr addr, uae_u32 b)
{
	addr &= wd->board_mask;

	if (addr >= GVP_ROM_OFFSET)
		return;

	if (addr >= wd->gdmac.s1_ramoffset && !wd->gdmac.series2) {
#if GVP_S1_DEBUG_IO > 1
		int off = wd->gdmac.bufoffset;
#endif
		if (!(addr & GVP_ROM_OFFSET)) {
			wd->gdmac.buffer[wd->gdmac.bufoffset++] = b;
			wd->gdmac.bufoffset &= wd->gdmac.s1_rammask;
		}
#if GVP_S1_DEBUG_IO > 1
		write_log(_T("gvp_s1_bput sram %d %04x\n"), off, b);
#endif
		return;
	}

	if (wd->gdmac.series2) {
#if GVP_S2_DEBUG_IO > 0
		write_log(_T("gvp_s2_bput %04X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif
		switch (addr)
		{
			case 0x40:
			wd->gdmac.cntr &= 0x00ff;
			wd->gdmac.cntr |= b << 8;
			break;
			case 0x41:
			b &= ~(1 | 2);
			wd->gdmac.cntr &= 0xff00;
			wd->gdmac.cntr |= b << 0;
			break;
			case 0x61: // SASR
			wdscsi_sasr(&wd->wc, b);
			break;
			case 0x63: // SCMD
			wdscsi_put(&wd->wc, wd, b);
			break;
		
			case 0x74: // "secret1"
			case 0x75:
			case 0x7a: // "secret2"
			case 0x7b:
			case 0x7c: // "secret3"
			case 0x7d:
			write_log(_T("gvp_s2_bput_config %04X=%04X PC=%08X\n"), addr, b & 255, M68K_GETPC);
			break;
			default:
			write_log(_T("gvp_s2_bput_unk %04X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
			break;
		}
	} else {
#if GVP_S1_DEBUG_IO > 0
		write_log(_T("gvp_s1_bput %04X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif
		switch (addr)
		{
			case 0x60: // SASR
			wdscsi_sasr(&wd->wc, b);
			break;
			case 0x62: // SCMD
			wdscsi_put(&wd->wc, wd, b);
			break;
		
			// 68:
			// 00 CPU SRAM access 
			// ff WD SRAM access

			// 6c:
			// 28 0010 startup reset?
			// b8 1011 before CPU reading from SRAM
			// a8 1100 before CPU writing to SRAM
			// e8 1110 before starting WD write DMA
			// f8 1111 access done/start WD read DMA
			// 08 = intena?

			case 0x68:
#if GVP_S1_DEBUG_IO > 0
			write_log(_T("gvp_s1_bput_s1 %04X=%04X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif
			wd->gdmac.bufoffset = 0;
			break;
			case 0x6c:
#if GVP_S1_DEBUG_IO > 0
			write_log(_T("gvp_s1_bput_s1 %04X=%04X PC=%08X\n"), addr, b & 255, M68K_GETPC);
#endif
			if (!(wd->gdmac.cntr & 8) && (b & 8))
				wd_master_reset(wd, true);
			wd->gdmac.cntr = b;
			break;
		
			default:
			write_log(_T("gvp_s1_bput_unk %04X=%02X PC=%08X\n"), addr, b & 255, M68K_GETPC);
			break;
		}
	}

}

void gvp_accelerator_set_dma_bank(uae_u8 v)
{
	gvp_accelerator_bank = v;
}

static void dmac_gvp_write_word(struct wd_state *wd, uaecptr addr, uae_u32 b)
{
	addr &= wd->board_mask;

	if (addr >= GVP_ROM_OFFSET && addr < 65536)
		return;

	if (addr >= wd->gdmac.s1_ramoffset && !wd->gdmac.series2) {
#if GVP_S1_DEBUG_IO > 1
		int off = wd->gdmac.bufoffset;
#endif
		if (!(addr & GVP_ROM_OFFSET)) {
			wd->gdmac.buffer[wd->gdmac.bufoffset++] = b >> 8;
			wd->gdmac.bufoffset &= wd->gdmac.s1_rammask;
			wd->gdmac.buffer[wd->gdmac.bufoffset++] = b;
			wd->gdmac.bufoffset &= wd->gdmac.s1_rammask;
		}
#if GVP_S1_DEBUG_IO > 1
		write_log(_T("gvp_s1_wput sram %d %04x\n"), off, b);
#endif
		return;
	}

	if (wd->gdmac.series2) {
#if GVP_S2_DEBUG_IO > 0
		write_log(_T("gvp_s2_wput %04X=%04X PC=%08X\n"), addr, b & 65535, M68K_GETPC);
#endif
		switch (addr)
		{
			case 0x40:
			b &= ~(1 | 2);
			wd->gdmac.cntr = b;
			break;
			case 0x68: // bank
			wd->gdmac.bank_ptr = &wd->gdmac.bank;
			wd->gdmac.bank = b >> 6;
			wd->gdmac.maprom = b & 0x3f;
			cpuboard_gvpmaprom((b >> 1) & 7);
			break;
			case 0x70: // ACR
			wd->gdmac.addr &= 0x0000ffff;
			wd->gdmac.addr |= (b & 0xff) << 16;
			wd->gdmac.addr &= wd->gdmac.addr_mask;
			break;
			case 0x72: // ACR
			wd->gdmac.addr &= 0xffff0000;
			wd->gdmac.addr |= b;
			wd->gdmac.addr &= wd->gdmac.addr_mask;
			break;
			case 0x76: // START DMA
			wd->gdmac.dma_on = 1;
			break;
			case 0x78: // STOP DMA
			wd->gdmac.dma_on = 0;
			break;
			case 0x74: // "secret1"
			case 0x7a: // "secret2"
			case 0x7c: // "secret3"
#if GVP_S2_DEBUG_IO > 0
			write_log(_T("gvp_s2_wput_config %04X=%04X PC=%08X\n"), addr, b & 65535, M68K_GETPC);
#endif
			break;
			default:
			write_log(_T("gvp_s2_wput_unk %04X=%04X PC=%08X\n"), addr, b & 65535, M68K_GETPC);
			break;
		}

	} else {

		dmac_gvp_write_byte(wd, addr, b >> 8);
		dmac_gvp_write_byte(wd, addr + 1, b);

#if GVP_S1_DEBUG_IO > 0
		write_log(_T("gvp_s1_wput %04X=%04X PC=%08X\n"), addr, b & 65535, M68K_GETPC);
#endif
	}
}

static uae_u32 REGPARAM2 dmac_gvp_lget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= wd->board_mask;
	v = dmac_gvp_read_word(wd, addr) << 16;
	v |= dmac_gvp_read_word(wd, addr + 2) & 0xffff;
	return v;
}

static uae_u32 REGPARAM2 dmac_gvp_wget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= wd->board_mask;
	v = dmac_gvp_read_word(wd, addr);
	return v;
}

static uae_u32 REGPARAM2 dmac_gvp_bget(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= wd->board_mask;
	v = dmac_gvp_read_byte(wd, addr);
	return v;
}

static void REGPARAM2 dmac_gvp_lput(struct wd_state *wd, uaecptr addr, uae_u32 l)
{
	addr &= wd->board_mask;
	dmac_gvp_write_word(wd, addr + 0, l >> 16);
	dmac_gvp_write_word(wd, addr + 2, l);
}

static void REGPARAM2 dmac_gvp_wput(struct wd_state *wd, uaecptr addr, uae_u32 w)
{
	addr &= wd->board_mask;
	dmac_gvp_write_word(wd, addr, w);
}
static void REGPARAM2 dmac_gvp_bput(struct wd_state *wd, uaecptr addr, uae_u32 b)
{
	b &= 0xff;
	addr &= wd->board_mask;
	if (wd->autoconfig) {
		if (addr == 0x48 && !wd->configured) {
			map_banks_z2(&wd->bank, b, (wd->board_mask + 1) >> 16);
			wd->baseaddress = b << 16;
			wd->configured = 1;
			expamem_next(&wd->bank, NULL);
			return;
		}
		if (addr == 0x4c && !wd->configured) {
			wd->configured = 1;
			expamem_shutup(&wd->bank);
			return;
		}
		if (!wd->configured)
			return;
	}
	dmac_gvp_write_byte(wd, addr, b);
}

static uae_u32 REGPARAM2 dmac_gvp_wgeti(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v = 0xffff;
	addr &= wd->board_mask;
	if (addr >= GVP_ROM_OFFSET) {
		addr -= GVP_ROM_OFFSET;
		v = (wd->rom[addr & wd->rom_mask] << 8) | wd->rom[(addr + 1) & wd->rom_mask];
	} else {
		write_log(_T("Invalid GVP instruction access %08x\n"), addr);
	}
	return v;
}
static uae_u32 REGPARAM2 dmac_gvp_lgeti(struct wd_state *wd, uaecptr addr)
{
	uae_u32 v;
	addr &= wd->board_mask;
	v = dmac_gvp_wgeti(wd, addr) << 16;
	v |= dmac_gvp_wgeti(wd, addr + 2);
	return v;
}

static void REGPARAM2 dmac_gvp_bput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		dmac_gvp_bput(wd, addr, b);
}
static void REGPARAM2 dmac_gvp_wput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		dmac_gvp_wput(wd, addr, b);
}
static void REGPARAM2 dmac_gvp_lput (uaecptr addr, uae_u32 b)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		dmac_gvp_lput(wd, addr, b);
}
static uae_u32 REGPARAM2 dmac_gvp_bget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_gvp_bget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_gvp_wget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_gvp_wget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_gvp_lget (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_gvp_lget(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_gvp_wgeti (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_gvp_wgeti(wd, addr);
	return 0;
}
static uae_u32 REGPARAM2 dmac_gvp_lgeti (uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (wd)
		return dmac_gvp_lgeti(wd, addr);
	return 0;
}
static int REGPARAM2 dmac_gvp_check(uaecptr addr, uae_u32 size)
{
	struct wd_state *wd = getscsiboard(addr);
	return wd ? 1 : 0;
}
static uae_u8 *REGPARAM2 dmac_gvp_xlate(uaecptr addr)
{
	struct wd_state *wd = getscsiboard(addr);
	if (!wd)
		return default_xlate(0);
	addr &= 0xffff;
	if (wd->rom >= wd->bank.baseaddr && wd->rom < wd->bank.baseaddr + wd->bank.allocated_size)
		return wd->bank.baseaddr + addr;
	return wd->rom + addr;
}

static const addrbank gvp_bank = {
	dmac_gvp_lget, dmac_gvp_wget, dmac_gvp_bget,
	dmac_gvp_lput, dmac_gvp_wput, dmac_gvp_bput,
	dmac_gvp_xlate, dmac_gvp_check, NULL, _T("*"), _T("GVP"),
	dmac_gvp_lgeti, dmac_gvp_wgeti,
	ABFLAG_IO | ABFLAG_SAFE | ABFLAG_PPCIOSPACE, S_READ, S_WRITE
};

/* SUPERDMAC (A3000 mainboard built-in) */

static void mbdmac_write_word (struct wd_state *wd, uae_u32 addr, uae_u32 val)
{
#if A3000_DEBUG_IO > 1
	write_log (_T("DMAC_WWRITE %08X=%04X PC=%08X\n"), addr, val & 0xffff, M68K_GETPC);
#endif
	addr &= 0xfffe;
	switch (addr)
	{
	case 0x02:
		wd->cdmac.dmac_dawr = val;
		break;
	case 0x04:
		wd->cdmac.dmac_wtc &= 0x0000ffff;
		wd->cdmac.dmac_wtc |= val << 16;
		break;
	case 0x06:
		wd->cdmac.dmac_wtc &= 0xffff0000;
		wd->cdmac.dmac_wtc |= val & 0xffff;
		break;
	case 0x0a:
		wd->cdmac.dmac_cntr = val;
		if (wd->cdmac.dmac_cntr & SCNTR_PREST)
			dmac_reset (wd);
		break;
	case 0x0c:
		wd->cdmac.dmac_acr &= 0x0000ffff;
		wd->cdmac.dmac_acr |= val << 16;
		break;
	case 0x0e:
		wd->cdmac.dmac_acr &= 0xffff0000;
		wd->cdmac.dmac_acr |= val & 0xfffe;
		break;
	case 0x12:
		if (wd->cdmac.dmac_dma <= 0)
			scsi_dmac_a2091_start_dma (wd);
		break;
	case 0x16:
		if (wd->cdmac.dmac_dma) {
			/* FLUSH */
			wd->cdmac.dmac_istr |= ISTR_FE_FLG;
			wd->cdmac.dmac_dma = 0;
		}
		break;
	case 0x1a:
		dmac_a2091_cint(wd);
		break;
	case 0x1e:
		/* ISTR */
		break;
	case 0x3e:
		scsi_dmac_a2091_stop_dma (wd);
		break;
	case 0x40:
	case 0x48:
		wdscsi_sasr(&wd->wc, val);
		break;
	case 0x42:
	case 0x46:
		wdscsi_put(&wd->wc, wd, val);
		break;
	case 0x5e:
	case 0x80:
#ifdef WITH_DSP
		if (is_dsp_installed) {
			dsp_write(val);
		}
#endif
		break;
	}
}

static void mbdmac_write_byte (struct wd_state *wd, uae_u32 addr, uae_u32 val)
{
#if A3000_DEBUG_IO > 1
	write_log (_T("DMAC_BWRITE %08X=%02X PC=%08X\n"), addr, val & 0xff, M68K_GETPC);
#endif
	addr &= 0xffff;
	switch (addr)
	{

	case 0x41:
	case 0x49:
		wdscsi_sasr(&wd->wc, val);
		break;
	case 0x43:
	case 0x47:
		wdscsi_put (&wd->wc, wd, val);
		break;
	case 0x5f:
	case 0x80:
#ifdef WITH_DSP
		if (is_dsp_installed) {
			dsp_write(val);
		}
#endif
		break;
	default:
		if (addr & 1)
			mbdmac_write_word (wd, addr, val);
		else
			mbdmac_write_word (wd, addr, val << 8);
	}
}

static uae_u32 mbdmac_read_word (struct wd_state *wd, uae_u32 addr)
{
#if A3000_DEBUG_IO > 1
	uae_u32 vaddr = addr;
#endif
	uae_u32 v = 0xffffffff;

	addr &= 0xfffe;
	switch (addr)
	{
	case 0x02:
		v = wd->cdmac.dmac_dawr;
		break;
	case 0x04:
	case 0x06:
		v = 0xffff;
		break;
	case 0x0a:
		v = wd->cdmac.dmac_cntr;
		break;
	case 0x0c:
		v = wd->cdmac.dmac_acr >> 16;
		break;
	case 0x0e:
		v = wd->cdmac.dmac_acr;
		break;
	case 0x12:
		if (wd->cdmac.dmac_dma <= 0)
			scsi_dmac_a2091_start_dma (wd);
		v = 0;
		break;
	case 0x1a:
		dmac_a2091_cint (wd);
		v = 0;
		break;;
	case 0x1e:
		v = wd->cdmac.dmac_istr;
		if (v & ISTR_INTS)
			v |= ISTR_INT_P;
		wd->cdmac.dmac_istr &= ~15;
		if (!wd->cdmac.dmac_dma)
			v |= ISTR_FE_FLG;
		break;
	case 0x3e:
		if (wd->cdmac.dmac_dma) {
			scsi_dmac_a2091_stop_dma (wd);
			wd->cdmac.dmac_istr |= ISTR_FE_FLG;
		}
		v = 0;
		break;
	case 0x40:
	case 0x48:
		v = wdscsi_getauxstatus(&wd->wc);
		break;
	case 0x42:
	case 0x46:
		v = wdscsi_get(&wd->wc, wd);
		break;
	case 0x5e:
	case 0x80:
#ifdef WITH_DSP
		if (is_dsp_installed) {
			v = dsp_read();
		}
#endif
		break;
	}
#if A3000_DEBUG_IO > 1
	write_log (_T("DMAC_WREAD %08X=%04X PC=%X\n"), vaddr, v & 0xffff, M68K_GETPC);
#endif
	return v;
}

static uae_u32 mbdmac_read_byte (struct wd_state *wd, uae_u32 addr)
{
#if A3000_DEBUG_IO > 1
	uae_u32 vaddr = addr;
#endif
	uae_u32 v = 0xffffffff;

	addr &= 0xffff;
	switch (addr)
	{
	case 0x41:
	case 0x49:
		v = wdscsi_getauxstatus (&wd->wc);
		break;
	case 0x43:
	case 0x47:
		v = wdscsi_get (&wd->wc, wd);
		break;
	default:
		v = mbdmac_read_word (wd, addr);
		if (!(addr & 1))
			v >>= 8;
		break;
	case 0x5f:
	case 0x80:
#ifdef WITH_DSP
		if (is_dsp_installed) {
			v = dsp_read();
		}
#endif
		break;
	}
#if A3000_DEBUG_IO > 1
	write_log (_T("DMAC_BREAD %08X=%02X PC=%X\n"), vaddr, v & 0xff, M68K_GETPC);
#endif
	return v;
}


static uae_u32 REGPARAM3 mbdmac_lget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 mbdmac_wget (uaecptr) REGPARAM;
static uae_u32 REGPARAM3 mbdmac_bget (uaecptr) REGPARAM;
static void REGPARAM3 mbdmac_lput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 mbdmac_wput (uaecptr, uae_u32) REGPARAM;
static void REGPARAM3 mbdmac_bput (uaecptr, uae_u32) REGPARAM;

static uae_u32 REGPARAM2 mbdmac_lget (uaecptr addr)
{
	uae_u32 v;
	v =  mbdmac_read_word (wd_a3000, addr + 0) << 16;
	v |= mbdmac_read_word (wd_a3000, addr + 2) << 0;
	return v;
}
static uae_u32 REGPARAM2 mbdmac_wget (uaecptr addr)
{
	uae_u32 v;
	v =  mbdmac_read_word (wd_a3000, addr);
	return v;
}
static uae_u32 REGPARAM2 mbdmac_bget (uaecptr addr)
{
	return mbdmac_read_byte (wd_a3000, addr);
}
static void REGPARAM2 mbdmac_lput (uaecptr addr, uae_u32 l)
{
	if ((addr & 0xffff) == 0x40) {
		// long write to 0x40 = write byte to SASR
		mbdmac_write_byte (wd_a3000, 0x41, l);
	} else {
		mbdmac_write_word (wd_a3000, addr + 0, l >> 16);
		mbdmac_write_word (wd_a3000, addr + 2, l >> 0);
	}
}
static void REGPARAM2 mbdmac_wput (uaecptr addr, uae_u32 w)
{
	mbdmac_write_word (wd_a3000, addr + 0, w);
}
static void REGPARAM2 mbdmac_bput (uaecptr addr, uae_u32 b)
{
	mbdmac_write_byte (wd_a3000, addr, b);
}

static addrbank mbdmac_a3000_bank = {
	mbdmac_lget, mbdmac_wget, mbdmac_bget,
	mbdmac_lput, mbdmac_wput, mbdmac_bput,
	default_xlate, default_check, NULL, NULL, _T("A3000 DMAC"),
	dummy_lgeti, dummy_wgeti,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

static void ew (uae_u8 *ac, int addr, uae_u32 value)
{
	addr &= 0xffff;
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		ac[addr] = (value & 0xf0);
		ac[addr + 2] = (value & 0x0f) << 4;
	} else {
		ac[addr] = ~(value & 0xf0);
		ac[addr + 2] = ~((value & 0x0f) << 4);
	}
}

static void wd_execute_cmd(struct wd_state *wds, int cmd, int msg, int unit)
{
	struct wd_chip_state *wd = &wds->wc;
	wd->scsi = wds->scsis[unit];
	//write_log (_T("scsi_thread got msg=%d cmd=%d\n"), msg, cmd);
	if (msg == 0) {
		if (WD33C93_DEBUG > 0)
			write_log (_T("%s command %02X\n"), WD33C93, cmd);
		switch (cmd & 0x7f)
		{
		case WD_CMD_RESET:
			wd_cmd_reset(wd, true, false);
			break;
		case WD_CMD_ABORT:
			wd_cmd_abort (wd);
			break;
		case WD_CMD_ASSERT_ATN:
			// gvpscsi v5
			wd->wdregs[WD_COMMAND_PHASE] = 0x10;
			break;
		case WD_CMD_SEL:
			wd_cmd_sel (wd, wds, false);
			break;
		case WD_CMD_SEL_ATN:
			wd_cmd_sel (wd, wds, true);
			break;
		case WD_CMD_SEL_ATN_XFER:
			wd_cmd_sel_xfer (wd, wds, true);
			break;
		case WD_CMD_SEL_XFER:
			wd_cmd_sel_xfer (wd, wds, false);
			break;
		case WD_CMD_TRANS_INFO:
			wd_cmd_trans_info (wds, wd->scsi, false);
			break;
		case WD_CMD_TRANS_ADDR:
			wd_cmd_trans_addr(wd, wds);
			break;
		case WD_CMD_NEGATE_ACK:
			if (wd->wd_phase == CSR_MSGIN && wd->wd_selected)
				wd_do_transfer_in(wd, wds, wd->scsi, false);
			break;
		case WD_CMD_TRANSFER_PAD:
			wd_cmd_trans_info (wds, wd->scsi, true);
			break;
		default:
			wd->wd_busy = false;
			write_log (_T("%s unimplemented/unknown command %02X\n"), WD33C93, cmd);
			set_status (wd, CSR_INVALID, 10);
			break;
		}
	} else if (msg == 1) {
		wd_do_transfer_in (wd, wds, wd->scsi, false);
	} else if (msg == 2) {
		wd_do_transfer_out (wd, wds, wd->scsi);
	} else if (msg == 3) {
		wd_do_transfer_in (wd, wds, wd->scsi, true);
	}
}

static int scsi_thread (void *wdv)
{
	struct wd_state *wds = (struct wd_state*)wdv;
	struct wd_chip_state *wd = &wds->wc;
	for (;;) {
		uae_u32 v = read_comm_pipe_u32_blocking (&wds->requests);
		if (wds->scsi_thread_running == 0 || v == 0xfffffff)
			break;
		int cmd = v & 0x7f;
		int msg = (v >> 8) & 0xff;
		int unit = (v >> 24) & 0xff;
		wd_execute_cmd(wds, cmd, msg, unit);
	}
	wds->scsi_thread_running = -1;
	return 0;
}

void init_wd_scsi (struct wd_state *wd, bool dma24bit)
{
	wd->configured = 0;
	wd->enabled = true;
	wd->wc.wd_used = 0;
	wd->wc.wd33c93_ver = 1;
	wd->baseaddress = 0;
	wd->dma_mask = dma24bit ? 0x00ffffff : 0xffffffff;
	if (wd == wd_cdtv) {
		wd->cdtv = true;
	}
	if (!wd->scsi_thread_running) {
		wd->scsi_thread_running = 1;
		init_comm_pipe (&wd->requests, 100, 1);
		uae_start_thread (_T("scsi"), scsi_thread, wd, NULL);
	}
	wd_init();
}

void a3000_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_a3000, rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

bool a3000scsi_init(struct autoconfig_info *aci)
{
	aci->zorro = 0;
	aci->start = 0xdd0000;
	aci->size = 0x10000;
	aci->hardwired = true;
	wd_addreset();
	if (!aci->doinit) {
		return true;
	}
	struct wd_state *wd = wd_a3000;
	if (!wd)
		return false;
	init_wd_scsi(wd, false);
	wd->enabled = true;
	wd->configured = -1;
	wd->baseaddress = 0xdd0000;
	wd->dmac_type = COMMODORE_SDMAC;
	map_banks(&mbdmac_a3000_bank, wd->baseaddress >> 16, 1, 0);
	wd_cmd_reset (&wd->wc, false, false);
	reset_dmac(wd);
	wd_init();
	return true;
}

static void a3000scsi_free (void)
{
	struct wd_state *wd = wd_a3000;
	if (!wd)
		return;
	freencrunit(wd);
	if (wd->scsi_thread_running > 0) {
		wd->scsi_thread_running = 0;
		write_comm_pipe_u32 (&wd->requests, 0xffffffff, 1);
		while(wd->scsi_thread_running == 0)
			sleep_millis (10);
		wd->scsi_thread_running = 0;
	}
}

void a2090_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_a2090[ci->controller_type_unit], rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
	if (ch >= XT506_UNIT0) {
		int unit = ch - XT506_UNIT0;
		wd->cdmac.xt_cyls[unit] = ci->pcyls > 2047 ? 2047 : ci->pcyls;
		wd->cdmac.xt_heads[unit] = ci->pheads > 15 ? 15 : ci->pheads;
		wd->cdmac.xt_sectors[unit] = ci->psecs > 255 ? 255 : ci->psecs;
	}
}

void a2091_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_a2091[ci->controller_type_unit], rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

void gvp_s1_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_gvps1[ci->controller_type_unit], rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

void gvp_s2_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_gvps2[ci->controller_type_unit], rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

void gvp_s2_add_accelerator_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_gvps2accel, rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

void gvp_a1208_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_gvpa1208[ci->controller_type_unit], rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

static void a2091_free_device (struct wd_state *wd)
{
	freencrunit(wd);
}

static void a2091_free(void)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		a2091_free_device(wd_a2091[i]);
		a2091_free_device(wd_a2090[i]);
	}
}

static void a2091_reset_device(struct wd_state *wd)
{
	if (!wd)
		return;
	wd->configured = 0;
	wd->wc.wd_used = 0;
	wd->wc.wd33c93_ver = 1;
	wd->dmac_type = COMMODORE_DMAC;
	wd->cdmac.old_dmac = 0;
	if (currprefs.scsi == 2)
		scsi_addnative(wd->scsis);
	wd_cmd_reset (&wd->wc, false, false);
	reset_dmac(wd);
	xt_reset(wd);
}

static void a2090_reset_device(struct wd_state *wd)
{
	if (!wd)
		return;
	wd->configured = 0;
	wd->wc.wd_used = 0;
	wd->wc.wd33c93_ver = 1;
	wd->dmac_type = COMMODORE_8727;
	wd->cdmac.old_dmac = 0;
	wd_cmd_reset (&wd->wc, false, false);
	reset_dmac(wd);
}

static void a2091_reset(int hardreset)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		a2091_reset_device(wd_a2091[i]);
		a2090_reset_device(wd_a2090[i]);
	}
}

bool a2091_init (struct autoconfig_info *aci)
{
	ew(aci->autoconfig_raw, 0x00, 0xc0 | 0x01 | 0x10 | (expansion_is_next_board_fastram() ? 0x08 : 0x00));
	/* A590/A2091 hardware id */
	ew(aci->autoconfig_raw, 0x04, aci->rc->subtype == 0 ? 0x02 : 0x03);
	/* commodore's manufacturer id */
	ew (aci->autoconfig_raw, 0x10, 0x02);
	ew (aci->autoconfig_raw, 0x14, 0x02);
	/* rom vector */
	ew (aci->autoconfig_raw, 0x28, CDMAC_ROM_VECTOR >> 8);
	ew (aci->autoconfig_raw, 0x2c, CDMAC_ROM_VECTOR);

	ew (aci->autoconfig_raw, 0x18, 0x00); /* ser.no. Byte 0 */
	ew (aci->autoconfig_raw, 0x1c, 0x00); /* ser.no. Byte 1 */
	ew (aci->autoconfig_raw, 0x20, 0x00); /* ser.no. Byte 2 */
	ew (aci->autoconfig_raw, 0x24, 0x00); /* ser.no. Byte 3 */

	wd_addreset();
	aci->label = _T("A2091");
	if (!aci->doinit)
		return true;

	struct wd_state *wd = getscsi(aci->rc);
	int slotsize;

	if (!wd)
		return false;

	init_wd_scsi(wd, aci->rc->dma24bit);

	wd->cdmac.old_dmac = aci->rc->subtype == 0;
	wd->threaded = true;
	wd->configured = 0;
	wd->autoconfig = true;
	wd->board_mask = 65535;
	memcpy(&wd->bank, &dmaca2091_bank, sizeof(addrbank));
	memcpy(wd->dmacmemory, aci->autoconfig_raw, sizeof wd->dmacmemory);

	alloc_expansion_bank(&wd->bank, aci);

	wd->rombankswitcher = 0;
	wd->rombank = 0;
	slotsize = 65536;
	wd->rom = wd->bank.baseaddr;
	memset(wd->rom, 0xff, slotsize);
	wd->rom_size = 16384;
	wd->rom_mask = wd->rom_size - 1;
	if (!aci->rc->autoboot_disabled) {
		struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_A2091);
		if (z) {
			wd->rom_size = zfile_size32(z);
			zfile_fread (wd->rom, wd->rom_size, 1, z);
			zfile_fclose (z);
			if (wd->rom_size == 32768) {
				wd->rombankswitcher = 1;
				for (int i = wd->rom_size - 1; i >= 0; i--) {
					wd->rom[i * 2 + 0] = wd->rom[i];
					wd->rom[i * 2 + 1] = 0xff;
				}
			} else {
				int i;
				if (wd->rom_size == 16384) {
					for (i = 1; i <= 256; i++) {
						if (wd->rom[wd->rom_size - i] != 0x00 && wd->rom[wd->rom_size - i] != 0xff)
							break;
					}
					if (i > 256) {
						// soft dumped rom without 0x2000 offset, fix it
						uae_u8 tmp[0x2000];
						memcpy(tmp, wd->rom + 0x2000, 0x2000);
						memcpy(wd->rom + 0x2000, wd->rom, 0x2000);
						memcpy(wd->rom, tmp, 0x2000);
					}
				}
				for (i = 1; i < slotsize / wd->rom_size; i++)
					memcpy (wd->rom + i * wd->rom_size, wd->rom, wd->rom_size);
			}
			wd->rom_mask = wd->rom_size - 1;
		}
	}
	aci->addrbank = &wd->bank;
	wd_init();
	return true;
}

static bool a2090x_init (struct autoconfig_info *aci, bool combitec)
{
	ew(aci->autoconfig_raw, 0x00, 0xc0 | 0x01 | 0x10);
	/* A590/A2091 hardware id */
	ew(aci->autoconfig_raw, 0x04, aci->rc->subtype ? 0x02 : 0x01);
	/* commodore's manufacturer id */
	ew(aci->autoconfig_raw, 0x10, 0x02);
	ew(aci->autoconfig_raw, 0x14, 0x02);
	/* rom vector */
	ew(aci->autoconfig_raw, 0x28, DMAC_8727_ROM_VECTOR >> 8);
	ew(aci->autoconfig_raw, 0x2c, DMAC_8727_ROM_VECTOR);

	ew(aci->autoconfig_raw, 0x18, 0x00); /* ser.no. Byte 0 */
	ew(aci->autoconfig_raw, 0x1c, 0x00); /* ser.no. Byte 1 */
	ew(aci->autoconfig_raw, 0x20, 0x00); /* ser.no. Byte 2 */
	ew(aci->autoconfig_raw, 0x24, 0x00); /* ser.no. Byte 3 */

	wd_addreset();
	aci->label = _T("A2090");
	if (!aci->doinit) {
		return true;
	}

	struct wd_state *wd = getscsi(aci->rc);
	int slotsize;

	if (!wd)
		return false;

	init_wd_scsi(wd, aci->rc->dma24bit);

	if (aci->rc->device_settings & 1)
		ew(aci->autoconfig_raw, 0x30, 0x80); // SCSI only flag

	wd->configured = 0;
	wd->autoconfig = true;
	wd->board_mask = 65535;
	memcpy(&wd->bank, &dmaca2091_bank, sizeof(addrbank));
	memcpy(wd->dmacmemory, aci->autoconfig_raw, sizeof wd->dmacmemory);

	alloc_expansion_bank(&wd->bank, aci);

	wd->rombankswitcher = 0;
	wd->rombank = 0;
	slotsize = 65536;
	wd->rom = wd->bank.baseaddr;
	memset(wd->rom, 0xff, slotsize);
	wd->rom_size = 16384;
	wd->rom_mask = wd->rom_size - 1;
	if (!aci->rc->autoboot_disabled && !combitec) {
		struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_A2090);
		if (z) {
			wd->rom_size = zfile_size32(z);
			zfile_fread (wd->rom, wd->rom_size, 1, z);
			zfile_fclose (z);
			for (int i = 1; i < slotsize / wd->rom_size; i++) {
				memcpy (wd->rom + i * wd->rom_size, wd->rom, wd->rom_size);
			}
			wd->rom_mask = wd->rom_size - 1;
		}
	}

	wd_init();

	return true;
}

bool a2090_init (struct autoconfig_info *aci)
{
	return a2090x_init(aci, false);
}
bool a2090b_init (struct autoconfig_info *aci)
{
	return a2090x_init(aci, true);
}
bool a2090b_preinit (struct autoconfig_info *aci)
{
	struct wd_state *wd = getscsi(aci->rc);
	if (!wd)
		return false;
	memcpy(&wd->bank2, &combitec_bank, sizeof(addrbank));
	wd->bank2.start = 0xf10000;
	wd->bank2.reserved_size = 0x10000;
	wd->bank2.mask = 0xffff;
	mapped_malloc(&wd->bank2);

	wd->rom2 = wd->bank2.baseaddr;
	wd->rom2_size = 65536;
	wd->rom2_mask = wd->rom2_size - 1;
	struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_A2090B);
	if (z) {
		zfile_fread(wd->rom2, 1, wd->rom2_size, z);
		zfile_fclose(z);
	}
	wd->baseaddress2 = 0xf10000;
	map_banks(&wd->bank2, wd->baseaddress2 >> 16, 1, 1);

	return true;
}

static void gvp_free_device (struct wd_state *wd)
{
	freencrunit(wd);
}

static void gvp_free(void)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		gvp_free_device(wd_gvps1[i]);
		gvp_free_device(wd_gvps2[i]);
		gvp_free_device(wd_gvpa1208[i]);
	}
	gvp_free_device(wd_gvps2accel);
}

static void gvp_reset_device(struct wd_state *wd)
{
	if (!wd)
		return;
	wd->configured = 0;
	wd->wc.wd_used = 0;
	wd->wc.wd33c93_ver = 1;
	wd->dmac_type = wd->gdmac.series2 ? GVP_DMAC_S2 : GVP_DMAC_S1;
	if (currprefs.scsi == 2)
		scsi_addnative(wd->scsis);
	wd_cmd_reset (&wd->wc, false, false);
	reset_dmac(wd);
}

static void gvp_reset(int hardreset)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		gvp_reset_device(wd_gvps1[i]);
		gvp_reset_device(wd_gvps2[i]);
		gvp_reset_device(wd_gvpa1208[i]);
	}
	gvp_reset_device(wd_gvps2accel);
}

static const uae_u8 gvp_scsi_i_autoconfig_1[16] = { 0xd1, 0x01, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
static const uae_u8 gvp_scsi_i_autoconfig_2[16] = { 0xd1, 0x02, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
static const uae_u8 gvp_scsi_i_autoconfig_3[16] = { 0xd2, 0x03, 0x00, 0x00, 0x07, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
static const uae_u8 gvp_scsi_ii_autoconfig[16] =  { 0xd1, 0x0b, 0x00, 0x00, 0x07, 0xe1, 0xee, 0xee, 0xee, 0xee, 0x80, 0x00 };

static bool is_gvp_accelerator(void)
{
	return ISCPUBOARD(BOARD_GVP, -1);
}

static bool gvp_init(struct autoconfig_info *aci, bool series2, bool accel, uae_u8 gvp_id)
{
	int romtype;
	bool autoboot_disabled = false;
	const uae_u8 *ac = gvp_scsi_ii_autoconfig;
	if (!series2) {
		ac = gvp_scsi_i_autoconfig_1;
		if (aci->rc->subtype == 1) {
			ac = gvp_scsi_i_autoconfig_2;
		} else if (aci->rc->subtype == 2) {
			ac = gvp_scsi_i_autoconfig_3;
		}
	}
	autoboot_disabled = aci->rc->autoboot_disabled;
	if (!accel) {
		romtype = series2 ? ROMTYPE_GVPS2 : ROMTYPE_GVPS1;
		aci->label = series2 ? _T("GVP SCSI S2") : _T("GVP SCSI S1");
	} else {
		romtype = ROMTYPE_CPUBOARD;
		aci->label = _T("GVP Accelerator SCSI");
		gvp_accelerator_bank = 0;
		autoboot_disabled = currprefs.cpuboard_settings & 1;
	}

	wd_addreset();
	if (!aci->doinit) {
		aci->autoconfigp = ac;
		return true;
	}

	struct wd_state *wd = getscsi(aci->rc);
	bool isscsi = true;

	if (!wd)
		return false;

	init_wd_scsi(wd, aci->rc->dma24bit);
	wd->configured = 0;
	wd->threaded = true;
	memcpy(&wd->bank, &gvp_bank, sizeof(addrbank));
	wd->autoconfig = true;
	wd->rombankswitcher = 0;
	memset(wd->dmacmemory, 0xff, sizeof wd->dmacmemory);
	wd->gdmac.series2 = series2;
	wd->gdmac.use_version = series2;
	wd->board_mask = 65535;

	wd->rom_size = 32768;
	wd->rom = xcalloc(uae_u8, 2 * wd->rom_size);
	memset(wd->rom, 0xff, 2 * wd->rom_size);
	wd->rom_mask = 32768 - 1;
	wd->gdmac.s1_subtype = 0;

	alloc_expansion_bank(&wd->bank, aci);

	if (!series2) {
		wd->gdmac.s1_rammask = GVP_SERIES_I_RAM_MASK_1;
		wd->gdmac.s1_ramoffset = GVP_SERIES_I_RAM_OFFSET_1;
		if (aci->rc->subtype == 1) {
			wd->gdmac.s1_rammask = GVP_SERIES_I_RAM_MASK_2;
			wd->gdmac.s1_ramoffset = GVP_SERIES_I_RAM_OFFSET_2;
		} else if (aci->rc->subtype == 2) {
			wd->gdmac.s1_rammask = GVP_SERIES_I_RAM_MASK_3;
			wd->gdmac.s1_ramoffset = GVP_SERIES_I_RAM_OFFSET_3;
			wd->board_mask = 131071;
		}
		wd->gdmac.s1_subtype = aci->rc->subtype;
	}
	xfree(wd->gdmac.buffer);
	wd->gdmac.buffer = xcalloc(uae_u8, 16384);
	if (!autoboot_disabled) {
		struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_GVPS2);
		if (z) {
			int size = zfile_size32(z);
			if (series2) {
				size_t total = 0;
				int seekpos = 0;
				size = zfile_size32(z);
				if (size > 16384 + 4096) {
					zfile_fread(wd->rom, 64, 1, z);
					zfile_fseek(z, 16384, SEEK_SET);
					zfile_fread(wd->rom + 64, 64, 1, z);
					if (!memcmp(wd->rom, wd->rom + 64, 64))
						wd->rombankswitcher = true;
					else
						seekpos = 16384;
				}
				while (total < 32768 - 4096) {
					size_t prevtotal = total;
					zfile_fseek(z, seekpos, SEEK_SET);
					total += zfile_fread(wd->rom + total, 1, wd->rom_size - total >= wd->rom_size ? wd->rom_size : wd->rom_size - total, z);
					if (prevtotal == total)
						break;
				}
			} else {
				int j = 0;
				bool oddonly = false;
				for (int i = 0; i < 16384; i++) {
					uae_u8 b;
					zfile_fread(&b, 1, 1, z);
					wd->rom[j + 16384] = b;
					wd->rom[j++] = b;
					if (i == 0 && (b & 0xc0) != 0x80) {
						// was not wordwide
						oddonly = true;
						wd->gdmac.use_version = true;
					}
					if (oddonly) {
						wd->rom[j + 16384] = 0xff;
						wd->rom[j++] = 0xff;
					}
				}
			}
			zfile_fclose(z);
		} else {
			isscsi = false;
		}
	}

	if (!wd->rombankswitcher) {
		memcpy(wd->bank.baseaddr + 32768, wd->rom, 32768);
		xfree(wd->rom);
		wd->rom = wd->bank.baseaddr + 32768;
	}

	if (series2) {
		wd->gdmac.version = gvp_id;
		wd->gdmac.addr_mask = 0x00ffffff;
		if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A530)) {
			wd->gdmac.version = isscsi ? GVP_A530_SCSI : GVP_A530;
		} else if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_GFORCE030)) {
			wd->gdmac.version = isscsi ? GVP_GFORCE_030_SCSI : GVP_GFORCE_030;
		} else if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_GFORCE040)) {
			wd->gdmac.version = isscsi ? GVP_GFORCE_040_SCSI : GVP_GFORCE_040;
		} else if (ISCPUBOARD(BOARD_GVP, BOARD_GVP_SUB_A1230SII)) {
			wd->gdmac.version = (currprefs.cpuboard_settings & 2) ? GVP_A1291 : GVP_A1291_SCSI;
			wd->wc.resetnodelay = true;
			wd->gdmac.bank_ptr = &gvp_accelerator_bank;
		}
	} else {
		wd->gdmac.version = 0x00;
	}

	for (int i = 0; i < 16; i++) {
		uae_u8 b = ac[i];
		ew(wd->dmacmemory, i * 4, b);
	}
	gvp_reset_device(wd);
	wd_init();
	return true;
}

bool gvp_init_s1(struct autoconfig_info *aci)
{
	return gvp_init(aci, false, false, 0);
}
bool gvp_init_s2(struct autoconfig_info *aci)
{
	return gvp_init(aci, true, false, GVP_SERIESII);
}
bool gvp_init_accelerator(struct autoconfig_info *aci)
{
	return gvp_init(aci, true, true, 0);
}
bool gvp_init_a1208(struct autoconfig_info *aci)
{
	return gvp_init(aci, true, true, GVP_A1208);
}

void cdtv_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_cdtv, rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd_cdtv->scsis[ch], ch, ci, rc);
}

static void comspec_ac(struct autoconfig_info *aci)
{
	ew(aci->autoconfig_raw, 0x00, 0xc0 | 0x01 | (aci->rc->subtype == 0 ? 0x00: 0x10));
	ew(aci->autoconfig_raw, 0x04, 0x11);
	ew(aci->autoconfig_raw, 0x10, 0x03);
	ew(aci->autoconfig_raw, 0x14, 0xee);
	/* rom vector */
	ew(aci->autoconfig_raw, 0x28, 0x00);
	ew(aci->autoconfig_raw, 0x2c, 0x20);
	/* serial number */
	ew(aci->autoconfig_raw, 0x18, 0x00);
	ew(aci->autoconfig_raw, 0x1c, 0x00);
	ew(aci->autoconfig_raw, 0x20, 0x00);
	ew(aci->autoconfig_raw, 0x24, (aci->rc->device_settings & 1) ? 0x00 : 0x01);
}

bool comspec_init (struct autoconfig_info *aci)
{
	comspec_ac(aci);
	aci->label = _T("COMSPEC");
	wd_addreset();
	if (!aci->doinit)
		return true;

	struct wd_state *wd = getscsi(aci->rc);
	if (!wd)
		return false;

	wd_init();
	return true;
}

bool comspec_preinit (struct autoconfig_info *aci)
{
	struct wd_state *wd = getscsi(aci->rc);
	if (!wd)
		return false;

	comspec_ac(aci);
	init_wd_scsi(wd, aci->rc->dma24bit);
	wd->configured = 0;
	wd->dmac_type = COMSPEC_CHIP;
	wd->autoconfig = true;
	wd->board_mask = 65535;
	wd->wc.resetnodelay = true;
	memcpy(&wd->bank, &comspec_bank, sizeof(addrbank));
	memcpy(wd->dmacmemory, aci->autoconfig_raw, sizeof wd->dmacmemory);

	alloc_expansion_bank(&wd->bank, aci);

	wd->rombank = 0;
	wd->rom_size = 16384;
	int slotsize = 65536;
	wd->rom = xcalloc(uae_u8, slotsize);
	memset(wd->rom, 0xff, slotsize);
	wd->rom_mask = wd->rom_size - 1;
	struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_COMSPEC);
	if (z) {
		wd->rom_size = zfile_size32(z);
		zfile_fread (wd->rom, wd->rom_size, 1, z);
		zfile_fclose (z);
		wd->rom_mask = wd->rom_size - 1;
	}

	wd->userdata = xcalloc(struct rtc_msm_data, 1);
	struct rtc_msm_data *rtc = (struct rtc_msm_data*)wd->userdata;
	rtc->clock_control_d = 1;
	rtc->clock_control_f = 0x4; /* 24/12 */
	rtc->yearmode = true;


	if (!aci->rc->autoboot_disabled && aci->rc->subtype == 0) {
		map_banks(&wd->bank, 0xf00000 >> 16, 1, 0);
		wd->baseaddress2 = 0xf00000;
	}

	return true;
}

void comspec_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct wd_state *wd = allocscsi(&wd_comspec[ci->controller_type_unit], rc, ch);
	if (!wd || ch < 0)
		return;
	add_scsi_device(&wd->scsis[ch], ch, ci, rc);
}

static void wd_addreset(void)
{
	device_add_reset(a2091_reset);
	device_add_reset(gvp_reset);
}

static void wd_init(void)
{
	device_add_hsync(scsi_hsync);
	device_add_rethink(rethink_a2091);
	device_add_exit(a2091_free, NULL);
	device_add_exit(gvp_free, NULL);
	device_add_exit(a3000scsi_free, NULL);
}

#if 0
uae_u8 *save_scsi_dmac (int wdtype, int *len, uae_u8 *dstptr)
{
	struct wd_state *wd = wdscsi[wdtype];
	uae_u8 *dstbak, *dst;

	if (!wd->enabled)
		return NULL;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);

	// model (0=original,1=rev2,2=superdmac)
	save_u32 (currprefs.cs_mbdmac == 1 ? 2 : 1);
	save_u32 (0); // reserved flags
	save_u8(wd->cdmac.dmac_istr);
	save_u8(wd->cdmac.dmac_cntr);
	save_u32(wd->cdmac.dmac_wtc);
	save_u32(wd->cdmac.dmac_acr);
	save_u16(wd->cdmac.dmac_dawr);
	save_u32(wd->cdmac.dmac_dma ? 1 : 0);
	save_u8 (wd->configured);
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_scsi_dmac (int wdtype, uae_u8 *src)
{
	struct wd_state *wd = wdscsi[wdtype];
	restore_u32 ();
	restore_u32 ();
	wd->cdmac.dmac_istr = restore_u8();
	wd->cdmac.dmac_cntr = restore_u8();
	wd->cdmac.dmac_wtc = restore_u32();
	wd->cdmac.dmac_acr = restore_u32();
	wd->cdmac.dmac_dawr = restore_u16();
	restore_u32 ();
	wd->configured = restore_u8 ();
	return src;
}

uae_u8 *save_scsi_device (int wdtype, int num, int *len, uae_u8 *dstptr)
{
	uae_u8 *dstbak, *dst;
	struct scsi_data *s;
	struct wd_state *wd = wdscsi[wdtype];

	if (!wd->enabled)
		return NULL;
	s = wd->scsis[num];
	if (!s)
		return NULL;
	if (dstptr)
		dstbak = dst = dstptr;
	else
		dstbak = dst = xmalloc (uae_u8, 1000);
	save_u32 (num);
	save_u32 (s->device_type); // flags
	switch (s->device_type)
	{
	case UAEDEV_HDF:
	case 0:
		save_u64 (s->hfd->size);
		save_string (s->hfd->hfd.ci.rootdir);
		save_u32 (s->hfd->hfd.ci.blocksize);
		save_u32 (s->hfd->hfd.ci.readonly);
		save_u32 (s->hfd->cyls);
		save_u32 (s->hfd->heads);
		save_u32 (s->hfd->secspertrack);
		save_u64 (s->hfd->hfd.virtual_size);
		save_u32 (s->hfd->hfd.ci.sectors);
		save_u32 (s->hfd->hfd.ci.surfaces);
		save_u32 (s->hfd->hfd.ci.reserved);
		save_u32 (s->hfd->hfd.ci.bootpri);
		save_u32 (s->hfd->ansi_version);
		if (num == 7) {
			save_u16(wd->cdmac.xt_cyls);
			save_u16(wd->cdmac.xt_heads);
			save_u16(wd->cdmac.xt_sectors);
			save_u8(wd->cdmac.xt_status);
			save_u8(wd->cdmac.xt_control);
		}
	break;
	case UAEDEV_CD:
		save_u32 (s->cd_emu_unit);
	break;
	case UAEDEV_TAPE:
		save_u32 (s->cd_emu_unit);
		save_u32 (s->tape->blocksize);
		save_u32 (s->tape->wp);
		save_string (s->tape->tape_dir);
	break;
	}
	*len = dst - dstbak;
	return dstbak;
}

uae_u8 *restore_scsi_device (int wdtype, uae_u8 *src)
{
	struct wd_state *wd = wdscsi[wdtype];
	int num, num2;
	struct hd_hardfiledata *hfd;
	struct scsi_data *s;
	uae_u64 size;
	uae_u32 flags;
	int blocksize, readonly;
	TCHAR *path;

	num = restore_u32 ();

	flags = restore_u32 ();
	switch (flags & 15)
	{
	case UAEDEV_HDF:
	case 0:
		hfd = xcalloc (struct hd_hardfiledata, 1);
		s = wd->scsis[num] = scsi_alloc_hd (num, hfd);
		size = restore_u64 ();
		path = restore_string ();
		_tcscpy (s->hfd->hfd.ci.rootdir, path);
		blocksize = restore_u32 ();
		readonly = restore_u32 ();
		s->hfd->cyls = restore_u32 ();
		s->hfd->heads = restore_u32 ();
		s->hfd->secspertrack = restore_u32 ();
		s->hfd->hfd.virtual_size = restore_u64 ();
		s->hfd->hfd.ci.sectors = restore_u32 ();
		s->hfd->hfd.ci.surfaces = restore_u32 ();
		s->hfd->hfd.ci.reserved = restore_u32 ();
		s->hfd->hfd.ci.bootpri = restore_u32 ();
		s->hfd->ansi_version = restore_u32 ();
		s->hfd->hfd.ci.blocksize = blocksize;
		if (num == 7) {
			wd->cdmac.xt_cyls = restore_u16();
			wd->cdmac.xt_heads = restore_u8();
			wd->cdmac.xt_sectors = restore_u8();
			wd->cdmac.xt_status = restore_u8();
			wd->cdmac.xt_control = restore_u8();
		}
		if (size)
			add_scsi_hd (&wd->scsis[num], num, hfd, NULL, s->hfd->ansi_version);
		xfree (path);
	break;
	case UAEDEV_CD:
		num2 = restore_u32 ();
		add_scsi_cd (wd->scsis, num, num2);
	break;
	case UAEDEV_TAPE:
		num2 = restore_u32 ();
		blocksize = restore_u32 ();
		readonly = restore_u32 ();
		path = restore_string ();
		add_scsi_tape (&wd->scsis[num], num, path, readonly != 0);
		xfree (path);
	break;
	}
	return src;
}
#endif
