/*
* UAE - The Un*x Amiga Emulator
*
* NCR 53C9x
*
* (c) 2014 Toni Wilen
*/

#include "sysconfig.h"
#include "sysdeps.h"

#ifdef NCR9X

#define NCR_DEBUG 0

#include "options.h"
#include "uae.h"
#include "memory.h"
#include "rommgr.h"
#include "custom.h"
#include "newcpu.h"
#include "ncr9x_scsi.h"
#include "scsi.h"
#include "filesys.h"
#include "zfile.h"
#include "blkdev.h"
#include "cpuboard.h"
#include "flashrom.h"
#include "autoconf.h"
#ifdef WITH_QEMU_CPU
#include "qemuvga/qemuuaeglue.h"
#include "qemuvga/queue.h"
#include "qemuvga/scsi/scsi.h"
#include "qemuvga/scsi/esp.h"
#endif
#include "gui.h"
#include "devices.h"

#define FASTLANE_BOARD_SIZE (2 * 16777216)
#define FASTLANE_ROM_SIZE 32768
#define FASTLANE_HARDBITS 0x01000041
#define FASTLANE_RESETDMA 0x01000081

/* NetBSD Fastlane definitions */
#define FLSC_HB_DISABLED	0x01
#define FLSC_HB_BUSID6		0x02
#define FLSC_HB_SEAGATE		0x04
#define FLSC_HB_SLOW		0x08
#define FLSC_HB_SYNCHRON	0x10
#define FLSC_HB_CREQ		0x20
#define FLSC_HB_IACT		0x40
#define FLSC_HB_MINT		0x80

#define FLSC_PB_ESI		0x01
#define FLSC_PB_EDI		0x02
#define FLSC_PB_ENABLE_DMA	0x04
#define FLSC_PB_DMA_WRITE	0x08
#define FLSC_PB_LED		0x10

#define OKTAGON_BOARD_SIZE 65536
#define OKTAGON_ROM_SIZE 32768
#define OKTAGON_ROM_OFFSET 0x100

#define OKTAGON_ESP_ADDR 0x03000
#define OKTAGON_DMA_START 0x01000
#define OKTAGON_DMA_END 0x2000
#define OKTAGON_INTENA 0x8000
#define OKTAGON_EEPROM_SCL 0x8010
#define OKTAGON_EEPROM_SDA 0x8018
#define OKTAGON_EEPROM_SIZE 512

#define MASOBOSHI_ESP_ADDR 0xfa00
#define MASOBOSHI_DMA_START 0xf900
#define MASOBOSHI_DMA_END 0xfa00

#define DKB_BOARD_SIZE 131072
#define DKB_ROM_SIZE 32768
#define DKB_ROM_OFFSET 0x8000


struct ncr9x_state
{
	DeviceState devobject;
	SCSIDevice *scsid[8];
	SCSIBus scsibus;
	uae_u32 board_mask;
	uae_u32 board_mask2;
	uae_u8 *rom;
	void *flashrom;
	struct zfile *flashrom_file;
	uae_u8 acmemory[128];
	int configured;
	uaecptr baseaddress;
	uaecptr baseaddress2;
	uae_u32 expamem_hi;
	uae_u32 expamem_lo;
	bool enabled;
	int rom_start, rom_end, rom_offset;
	bool rom_bypass_if_write;
	int io_start, io_end;
	addrbank *bank;
	bool chipirq, boardirq, boardirqlatch;
	bool intena;
	bool irq6;
	bool pcmcia;
	void (*irq_func)(struct ncr9x_state*);
	int led;
	uaecptr dma_ptr;
	bool dma_on;
	int dma_cnt;
	uae_u32 dma_mask;
	uae_u8 states[16];
	struct romconfig *rc;
	struct ncr9x_state **self_ptr;

	uae_u8 data;
	bool data_valid_r, data_valid_w;
	void *eeprom;
	uae_u8 eeprom_data[512];
	bool romisoddonly;
	bool romisevenonly;

	uae_u8 *fakedma_data_buf;
	int fakedma_data_size_allocated;
	int fakedma_data_size;
	int fakedma_data_offset;
	uae_u8 *fakedma_data_write_buffer;
};



/*
	Blizzard SCSI Kit IV:

	scsi: 0x8000
	dma: 0x10000

	pa >>= 1;
	if (!bsc->sc_datain)
		pa |= 0x80000000;
	bsc->sc_dmabase[0x8000] = (u_int8_t)(pa >> 24);
	bsc->sc_dmabase[0] = (u_int8_t)(pa >> 24);
	bsc->sc_dmabase[0] = (u_int8_t)(pa >> 16);
	bsc->sc_dmabase[0] = (u_int8_t)(pa >> 8);
	bsc->sc_dmabase[0] = (u_int8_t)(pa);

	Blizzard 2060:

	scsi: 0x1ff00
	dma: 0x1fff0

	bsc->sc_reg[0xe0] = BZTZSC_PB_LED;	LED

	pa >>= 1;
	if (!bsc->sc_datain)
		pa |= 0x80000000;
	bsc->sc_dmabase[12] = (u_int8_t)(pa);
	bsc->sc_dmabase[8] = (u_int8_t)(pa >> 8);
	bsc->sc_dmabase[4] = (u_int8_t)(pa >> 16);
	bsc->sc_dmabase[0] = (u_int8_t)(pa >> 24);
	
*/

#define MAX_NCR9X_UNITS 10

static struct ncr9x_state *ncr_blizzard_scsi;
static struct ncr9x_state *ncr_fastlane_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_oktagon2008_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_alf3_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_masoboshi_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_dkb1200_scsi;
static struct ncr9x_state *ncr_ematrix530_scsi;
static struct ncr9x_state *ncr_multievolution_scsi;
static struct ncr9x_state *ncr_typhoon2_scsi;
static struct ncr9x_state *ncr_golemfast_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_scram5394_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_rapidfire_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_trifecta_scsi[MAX_DUPLICATE_EXPANSION_BOARDS];
static struct ncr9x_state *ncr_squirrel_scsi;
static struct ncr9x_state *ncr_mtec_mastercard;

static struct ncr9x_state *ncr_units[MAX_NCR9X_UNITS + 1];

// Only offsets 0x100 to 0x10f contains non-FF data
static uae_u8 oktagon_eeprom[16] =
{
	0x0b, 0xf4, 0x3f, 0x0a, 0xff, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xaf, 0xff
};

static void freescsi(SCSIDevice *scsi)
{
	if (scsi) {
		free_scsi((struct scsi_data*)scsi->handle);
		xfree(scsi);
	}
}

static void freencrunit(struct ncr9x_state *ncr)
{
	if (!ncr)
		return;
	for (int i = 0; i < MAX_NCR9X_UNITS; i++) {
		if (ncr_units[i] == ncr) {
			ncr_units[i] = NULL;
		}
	}
	for (int ch = 0; ch < 8; ch++) {
		freescsi(ncr->scsid[ch]);
		ncr->scsid[ch] = NULL;
	}
	flash_free(ncr->flashrom);
	zfile_fclose(ncr->flashrom_file);
	xfree(ncr->rom);
	if (ncr->self_ptr)
		*ncr->self_ptr = NULL;
	xfree(ncr);
}

static struct ncr9x_state *allocscsi(struct ncr9x_state **ncr, struct romconfig *rc, int ch)
{
	struct ncr9x_state *scsi;

	if (ch < 0) {
		freencrunit(*ncr);
		*ncr = NULL;
	}
	if ((*ncr) == NULL) {
		scsi = xcalloc(struct ncr9x_state, 1);
		for (int i = 0; i < MAX_NCR9X_UNITS; i++) {
			if (ncr_units[i] == NULL) {
				ncr_units[i] = scsi;
				if (rc)
					rc->unitdata = scsi;
				scsi->rc = rc;
				scsi->self_ptr = ncr;
				*ncr = scsi;
				return scsi;
			}
		}
	}
	return *ncr;
}

static struct ncr9x_state *getscsi(struct romconfig *rc)
{
	for (int i = 0; i < MAX_NCR9X_UNITS; i++) {
		if (ncr_units[i]) {
			struct ncr9x_state *ncr = ncr_units[i];
			if (ncr->rc == rc)
				return ncr;
		}
	}
	return NULL;
}

static void ncr9x_rethink(void)
{
	for (int i = 0; ncr_units[i]; i++) {
		if (ncr_units[i]->boardirq) {
			if (ncr_units[i]->pcmcia)
				pcmcia_interrupt_set(1);
			else
				safe_interrupt_set(IRQ_SOURCE_NCR9X, i, ncr_units[i]->irq6);
		}
	}
}

static void set_irq2(struct ncr9x_state *ncr)
{
	if (ncr->chipirq && !ncr->boardirq) {
		ncr->boardirq = true;
		devices_rethink_all(ncr9x_rethink);
	}
	if (!ncr->chipirq && ncr->boardirq) {
		ncr->boardirq = false;
		if (ncr->pcmcia)
			pcmcia_interrupt_set(0);
	}
}

static void set_irq2_dkb1200(struct ncr9x_state *ncr)
{
	if (!(ncr->states[0] & 0x40))
		ncr->boardirq = false;
	if (ncr->chipirq && !ncr->boardirq && (ncr->states[0] & 0x40)) {
		ncr->boardirq = true;
		devices_rethink_all(ncr9x_rethink);
	}
}

static void set_irq2_oktagon(struct ncr9x_state *ncr)
{
	if (!(ncr->states[0] & 0x80))
		ncr->boardirq = false;
	if (ncr->chipirq && !ncr->boardirq && (ncr->states[0] & 0x80)) {
		ncr->boardirq = true;
		devices_rethink_all(ncr9x_rethink);
	}
}

static void set_irq2_alf3(struct ncr9x_state *ncr)
{
	if (!(ncr->states[0] & 0x01))
		ncr->boardirq = false;
	if (ncr->chipirq && !ncr->boardirq && (ncr->states[0] & 0x01)) {
		ncr->boardirq = true;
		devices_rethink_all(ncr9x_rethink);
	}
}

static void set_irq2_fastlane(struct ncr9x_state *ncr)
{
	if (!ncr->chipirq || !(ncr->states[0] & FLSC_PB_ESI)) {
		ncr->states[0] |= FLSC_HB_MINT;
		ncr->boardirq = false;
		return;
	}
	ncr->states[0] |= FLSC_HB_CREQ;
	ncr->states[0] &= ~FLSC_HB_MINT;
	if (ncr->states[0] & FLSC_PB_ESI) {
		if (!ncr->boardirq) {
			ncr->boardirq = true;
			devices_rethink_all(ncr9x_rethink);
		}
	}
}

static void set_irq2_golemfast(struct ncr9x_state *ncr)
{
}

static void set_irq2_masoboshi(struct ncr9x_state *ncr)
{
	if (ncr->chipirq) {
		ncr->boardirqlatch = true;
		if (1 || ncr->intena) {
			ncr->boardirq = true;
			devices_rethink_all(ncr9x_rethink);
#if NCR_DEBUG > 1
			write_log(_T("MASOBOSHI IRQ\n"));
#endif
		} else {
			ncr->boardirq = false;
		}
	} else {
		ncr->boardirq = false;
	}
}

static void set_irq2_trifecta(struct ncr9x_state *ncr)
{
	if (ncr->chipirq) {
		ncr->boardirqlatch = true;
		if (ncr->intena) {
			ncr->boardirq = true;
			devices_rethink_all(ncr9x_rethink);
#if NCR_DEBUG > 1
			write_log(_T("TRIFECTA IRQ\n"));
#endif
		} else {
			ncr->boardirq = false;
		}
	} else {
		ncr->boardirq = false;
	}
}

void esp_irq_raise(qemu_irq irq)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)irq;
	ncr->chipirq = true;
#if NCR_DEBUG > 1
	write_log(_T("NCR9X +IRQ\n"));
#endif
	ncr->irq_func(ncr);
}
void esp_irq_lower(qemu_irq irq)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)irq;
	ncr->chipirq = false;
#if NCR_DEBUG > 1
	write_log(_T("NCR9X -IRQ\n"));
#endif
	ncr->irq_func(ncr);
}

static void fakedma_buffer_size(struct ncr9x_state *ncr, int size)
{
	size = (size + 1) & ~1;
	if (ncr->fakedma_data_size_allocated >= size)
		return;
	if (ncr->fakedma_data_buf)
		xfree(ncr->fakedma_data_buf);
	ncr->fakedma_data_buf = xmalloc(uae_u8, size);
	ncr->fakedma_data_size_allocated = size;
}

/* Fake DMA */

static int fake_dma_null(void *opaque, uint8_t *buf, int len)
{
	return 0;
}

static int fake_dma_read_ematrix(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	ncr->states[0] = 1;
	ncr->chipirq = true;
	set_irq2(ncr);
	ncr->fakedma_data_offset = 0;
	ncr->fakedma_data_write_buffer = buf;
	ncr->fakedma_data_size = len;
	fakedma_buffer_size(ncr, len);
	return 0;
}
static int fake_dma_write_ematrix(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	ncr->states[0] = 1;
	ncr->chipirq = true;
	set_irq2(ncr);
	ncr->fakedma_data_offset = 0;
	fakedma_buffer_size(ncr, len);
	memcpy(ncr->fakedma_data_buf, buf, len);
	if (len & 1)
		ncr->fakedma_data_buf[len] = 0;
	ncr->fakedma_data_size = len;
	return 0;
}

static int fake_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	ncr->fakedma_data_offset = 0;
	ncr->fakedma_data_write_buffer = buf;
	ncr->fakedma_data_size = len;
	fakedma_buffer_size(ncr, len);
	return 0;
}
static int fake_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	ncr->fakedma_data_offset = 0;
	fakedma_buffer_size(ncr, len);
	memcpy(ncr->fakedma_data_buf, buf, len);
	if (len & 1)
		ncr->fakedma_data_buf[len] = 0;
	ncr->fakedma_data_size = len;
	return 0;
}

static int fake2_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	esp_dma_enable(ncr->devobject.lsistate, 0);
	if (ncr->data_valid_r) {
		*buf = ncr->data;
		ncr->data_valid_r = false;
	}
	return 1;
}
static int fake2_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	esp_dma_enable(ncr->devobject.lsistate, 0);
	if (!ncr->data_valid_w) {
		ncr->data = *buf;
		ncr->data_valid_w = true;
		return 1;
	}
	return 0;
}

/* Following are true DMA */

static int masoboshi_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->dma_on) {
		m68k_cancel_idle();
		while (len > 0) {
			uae_u16 v = dma_get_word((ncr->dma_ptr & ~1) & ncr->dma_mask);
			*buf++ = v >> 8;
			len--;
			if (len > 0) {
				*buf++ = (uint8_t)v;
				len--;
			}
			ncr->dma_ptr += 2;
		}
		return -1;
	} else {
		esp_dma_enable(ncr->devobject.lsistate, 0);
		if (ncr->data_valid_r) {
			*buf = ncr->data;
			ncr->data_valid_r = false;
		}
		return 1;
	}
}
static int masoboshi_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->dma_on) {
		m68k_cancel_idle();
		while (len > 0) {
			uae_u16 v;
			v = *buf++;
			len--;
			v <<= 8;
			if (len > 0) {
				v |= *buf++;
				len--;
			}
			dma_put_word((ncr->dma_ptr & ~1) & ncr->dma_mask, v);
			ncr->dma_ptr += 2;
		}
		return -1;
	} else {
		esp_dma_enable(ncr->devobject.lsistate, 0);
		if (!ncr->data_valid_w) {
			ncr->data = *buf;
			ncr->data_valid_w = true;
			return 1;
		}
		return 0;
	}
}

static int trifecta_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->dma_on) {
		//write_log(_T("Trifecta DMA from %08x, %d bytes\n"), ncr->dma_ptr, len);
		m68k_cancel_idle();
		while (len > 0) {
			uae_u16 v = dma_get_word((ncr->dma_ptr & ~1) & ncr->dma_mask);
			*buf++ = v >> 8;
			len--;
			if (len > 0) {
				*buf++ = (uint8_t)v;
				len--;
			}
			ncr->dma_ptr += 2;
		}
		return -1;
	}
	return 0;
}
static int trifecta_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->dma_on) {
		//write_log(_T("Trifecta DMA to %08x, %d bytes\n"), ncr->dma_ptr, len);
		m68k_cancel_idle();
		while (len > 0) {
			uae_u16 v;
			v = *buf++;
			len--;
			v <<= 8;
			if (len > 0) {
				v |= *buf++;
				len--;
			}
			dma_put_word((ncr->dma_ptr & ~1) & ncr->dma_mask, v);
			ncr->dma_ptr += 2;
		}
		return -1;
	}
	return 0;
}


static int fastlane_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (!(ncr->states[0] & FLSC_PB_DMA_WRITE)) {
		write_log(_T("fastlane_dma_read mismatched direction!\n"));
		return -1;
	}
	m68k_cancel_idle();
	while (len > 0) {
		uae_u16 v = dma_get_word(ncr->dma_ptr & ~1);
		*buf++ = v >> 8;
		len--;
		if (len > 0) {
			*buf++ = (uint8_t)v;
			len--;
		}
		ncr->dma_ptr += 2;
	}
	return -1;
}
static int fastlane_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->states[0] & FLSC_PB_DMA_WRITE) {
		write_log(_T("fastlane_dma_write mismatched direction!\n"));
		return -1;
	}
	m68k_cancel_idle();
	while (len > 0) {
		uae_u16 v;
		v = *buf++;
		len--;
		v <<= 8;
		if (len > 0) {
			v |= *buf++;
			len--;
		}
		dma_put_word(ncr->dma_ptr & ~1, v);
		ncr->dma_ptr += 2;
	}
	return -1;
}

static int cyberstorm_mk1_mk2_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (!(ncr->dma_ptr & 0x00000001)) {
		write_log(_T("cyberstorm_dma_read mismatched direction!\n"));
		return -1;
	}
	m68k_cancel_idle();
	while (len > 0) {
		uae_u16 v = dma_get_word(ncr->dma_ptr & ~1);
		*buf++ = v >> 8;
		len--;
		if (len > 0) {
			*buf++ = (uint8_t)v;
			len--;
		}
		ncr->dma_ptr += 2;
	}
	return -1;
}
static int cyberstorm_mk1_mk2_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->dma_ptr & 0x00000001) {
		write_log(_T("cyberstorm_dma_write mismatched direction!\n"));
		return -1;
	}
	m68k_cancel_idle();
	while (len > 0) {
		uae_u16 v;
		v = *buf++;
		len--;
		v <<= 8;
		if (len > 0) {
			v |= *buf++;
			len--;
		}
		dma_put_word(ncr->dma_ptr & ~1, v);
		ncr->dma_ptr += 2;
	}
	return -1;
}

static int blizzard_dma_read(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (!(ncr->dma_ptr & 0x80000000)) {
		write_log(_T("blizzard_dma_read mismatched direction!\n"));
		return -1;
	}
	m68k_cancel_idle();
	while (len > 0) {
		uae_u16 v = dma_get_word((ncr->dma_ptr & 0x7fffffff) * 2);
		*buf++ = v >> 8;
		len--;
		if (len > 0) {
			*buf++ = (uint8_t)v;
			len--;
		}
		ncr->dma_ptr++;
	}
	return -1;
}
static int blizzard_dma_write(void *opaque, uint8_t *buf, int len)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)opaque;
	if (ncr->dma_ptr & 0x80000000) {
		write_log(_T("blizzard_dma_write mismatched direction!\n"));
		return -1;
	}
	m68k_cancel_idle();
	while (len > 0) {
		uae_u16 v;
		v = *buf++;
		len--;
		v <<= 8;
		if (len > 0) {
			v |= *buf++;
			len--;
		}
		dma_put_word((ncr->dma_ptr & 0x7fffffff) * 2, v);
		ncr->dma_ptr++;
	}
	return -1;
}

static int get_scb_len(uae_u8 cmd)
{
	if (cmd <= 0x1f)
		return 6;
	if (cmd >= 0x20 && cmd <= 0x5f)
		return 10;
	if (cmd >= 0x80 && cmd <= 0x9f)
		return 16;
	if (cmd >= 0xa0 && cmd <= 0xbf)
		return 12;
	return 0;
}

void scsiesp_req_continue(SCSIRequest *req)
{
	struct scsi_data *sd = (struct scsi_data*)req->dev->handle;
	if (sd->data_len < 0) {
		esp_command_complete(req, sd->status, 0);
	} else if (sd->data_len) {
		esp_transfer_data(req, sd->data_len);
	} else {
		if (sd->direction > 0)
			scsi_emulate_cmd(sd);
		esp_command_complete(req, sd->status, 0);
	}
}
SCSIRequest *scsiesp_req_new(SCSIDevice *d, uint32_t tag, uint32_t lun, uint8_t *buf, void *hba_private)
{
	SCSIRequest *req = xcalloc(SCSIRequest, 1);
	struct scsi_data *sd = (struct scsi_data*)d->handle;
	struct ncr9x_state *ncr = (struct ncr9x_state*)sd->privdata;
	int len = get_scb_len(buf[0]);

	req->dev = d;
	req->hba_private = hba_private;
	req->bus = &ncr->scsibus;
	req->bus->qbus.parent = &ncr->devobject;

	memcpy(sd->cmd, buf, len);
	sd->cmd_len = len;
	return req;
}
int32_t scsiesp_req_enqueue(SCSIRequest *req)
{
	struct scsi_data *sd = (struct scsi_data*)req->dev->handle;

	sd->data_len = 0;
	scsi_start_transfer(sd);
	scsi_emulate_analyze(sd);
#if NCR_DEBUG
	write_log (_T("%02x.%02x.%02x.%02x.%02x.%02x\n"), sd->cmd[0], sd->cmd[1], sd->cmd[2], sd->cmd[3], sd->cmd[4], sd->cmd[5]);
#endif
	if (sd->direction <= 0)
		scsi_emulate_cmd(sd);
	if (sd->direction == 0)
		return 1;
	if (sd->direction > 0)
		return -sd->data_len;
	return sd->data_len;
}
void scsiesp_req_unref(SCSIRequest *req)
{
	xfree(req);
}
uint8_t *scsiesp_req_get_buf(SCSIRequest *req)
{
	struct scsi_data *sd = (struct scsi_data*)req->dev->handle;
	sd->data_len = 0;
	return sd->buffer;
}
SCSIDevice *scsiesp_device_find(SCSIBus *bus, int channel, int target, int lun)
{
	struct ncr9x_state *ncr = (struct ncr9x_state*)bus->privdata;
	if (lun != 0 || target < 0 || target >= 8) {
#if NCR_DEBUG
		write_log(_T("Selected not supported: %d-%d-%d\n"), channel, target, lun);
#endif
		return NULL;
	}
#if NCR_DEBUG
	if (ncr->scsid[target])
		write_log(_T("Selected: %d-%d-%d\n"), channel, target, lun);
	else
		write_log(_T("Selected non-existing: %d-%d-%d\n"), channel, target, lun);
#endif
	return ncr->scsid[target];
}
void scsiesp_req_cancel(SCSIRequest *req)
{
	write_log(_T("scsi_req_cancel!?\n"));
	esp_request_cancelled(req);
}

#define IO_MASK 0x3f

static uaecptr beswap(uaecptr addr)
{
	return (addr & ~3) | (3 - (addr & 3));
}

static bool isncr(struct ncr9x_state *ncr, struct ncr9x_state **arr)
{
	for (int i = 0; i < MAX_DUPLICATE_EXPANSION_BOARDS; i++) {
		if (arr[i] == ncr)
			return true;
	}
	return false;
}

static void ncr9x_io_bput3(struct ncr9x_state *ncr, uaecptr addr, uae_u32 val, int *reg)
{
	int reg_shift = 2;
	uaecptr oldaddr = addr;

	if (!ncr)
		return;

	addr &= ncr->board_mask;

	if (isncr(ncr, ncr_scram5394_scsi)) {

		if (addr >= 0x8000 && addr < 0xa000) {
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size) {
				ncr->fakedma_data_buf[ncr->fakedma_data_offset++] = val;
				if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
					memcpy(ncr->fakedma_data_write_buffer, ncr->fakedma_data_buf, ncr->fakedma_data_size);
					esp_fake_dma_done(ncr->devobject.lsistate);
				}
			}
			return;
		}
		if (addr & 1) {
			return;
		}
		if (addr < 0x6000 || addr >= 0xc000) {
			return;
		}
		reg_shift = 1;

	} else if (ncr == ncr_multievolution_scsi) {

		reg_shift = 1;
		if (addr & 0x1000) {
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size) {
				ncr->fakedma_data_buf[ncr->fakedma_data_offset++] = val;
				if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
					memcpy(ncr->fakedma_data_write_buffer, ncr->fakedma_data_buf, ncr->fakedma_data_size);
					esp_fake_dma_done(ncr->devobject.lsistate);
				}
			}
			return;
		}
	} else if (isncr(ncr, ncr_golemfast_scsi)) {

		reg_shift = 1;
		if ((addr & 0x8400) == 0x8400) {
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size) {
				ncr->fakedma_data_buf[ncr->fakedma_data_offset++] = val;
				if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
					memcpy(ncr->fakedma_data_write_buffer, ncr->fakedma_data_buf, ncr->fakedma_data_size);
					esp_fake_dma_done(ncr->devobject.lsistate);
				}
			}
			return;
		}

	} else if (ncr == ncr_squirrel_scsi) {

		reg_shift = 0;
		if (addr >= 16) {
			ncr->data = val;
			ncr->data_valid_r = true;
			esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		}


	} else if (isncr(ncr, ncr_masoboshi_scsi)) {

		if (addr >= 0xf000 && addr < 0xf800)
			addr &= ~0x0700;

		// SCSI DMA LEN (words)
		if (addr >= 0xf00a && addr < 0xf00c) {
			if (addr == 0xf00a) {
				ncr->dma_cnt &= 0x00ff;
				ncr->dma_cnt |= val << 8;
			} else {
				ncr->dma_cnt &= 0xff00;
				ncr->dma_cnt |= val;
			}
			return;
		}

		// SCSI DMA PTR
		if (addr >= 0xf00c && addr < 0xf010) {
			int shift = (3 - (addr - 0xf00c)) * 8;
			uae_u32 mask = 0xff << shift;
			ncr->dma_ptr &= ~mask;
			ncr->dma_ptr |= val << shift;
			ncr->dma_ptr &= 0xffffff;
			return;
		}

		if (addr >= 0xf000 && addr < 0xf008) {
			ncr->states[addr - 0xf000] = val;
			if (addr == 0xf000) {
				ncr->boardirqlatch = false;
				set_irq2_masoboshi(ncr);
			}

			if (addr == 0xf007) {
				// scsi dma start
				ncr->dma_on = false;
				if (val & 0x80) {
					write_log(_T("MASOBOSHI SCSI DMA %s start %08x, %d\n"), (ncr->states[5] & 0x80) ? _T("READ") : _T("WRITE"), ncr->dma_ptr, ncr->dma_cnt);
					ncr->dma_on = true;
					esp_dma_enable(ncr->devobject.lsistate, 1);
				} else {
					esp_dma_enable(ncr->devobject.lsistate, 0);
				}
			}
#if 0
			write_log(_T("MASOBOSHI IO %08X PUT %02x %08x\n"), addr, val & 0xff, M68K_GETPC);
#endif
			return;
		}

		if (addr >= MASOBOSHI_DMA_START && addr < MASOBOSHI_DMA_END) {
#if NCR_DEBUG > 2
			write_log(_T("MASOBOSHI DMA %08X PUT %02x %08x\n"), addr, val & 0xff, M68K_GETPC);
#endif
			ncr->data = val;
			ncr->data_valid_r = true;
			esp_fake_dma_put(ncr->devobject.lsistate, val);
			return;
		}
		if (addr < MASOBOSHI_ESP_ADDR || addr >= MASOBOSHI_ESP_ADDR + 0x100) {
#if NCR_DEBUG
			write_log(_T("MASOBOSHI IO %08X PUT %02x %08x\n"), addr, val & 0xff, M68K_GETPC);
#endif
			return;
		}
		if (addr == MASOBOSHI_ESP_ADDR + 3 * 2 && val == 0x02)
			ncr->states[0] |= 0x80;
		reg_shift = 1;
		addr &= 0x3f;

	} else if (isncr(ncr, ncr_trifecta_scsi)) {

		if (addr == 0x400) {
			ncr->intena = (val & 8) != 0;
			ncr->dma_on = (val & 4) != 0;
			esp_dma_enable(ncr->devobject.lsistate, ncr->dma_on);
			ncr->states[0] = val;
			ncr->dma_cnt = 0;
#if NCR_DEBUG
			if (ncr->dma_on) {
				write_log(_T("Trifecta DMA %08x %c\n"), ncr->dma_ptr, (val & 1) ? 'R' : 'W');
			}
#endif
		} else if (addr == 0x402) {
			ncr->dma_ptr &= 0xffff00;
			ncr->dma_ptr |= val;
		} else if (addr == 0x404) {
			ncr->dma_ptr &= 0xff00ff;
			ncr->dma_ptr |= val << 8;
		} else if (addr == 0x406) {
			ncr->dma_ptr &= 0x00ffff;
			ncr->dma_ptr |= val << 16;
		}

		if (addr >= 0x200)
			return;

		if (!(addr & 1))
			return;

		reg_shift = 1;
		addr &= 0x3f;

	} else if (isncr(ncr, ncr_oktagon2008_scsi)) {
		if (addr == OKTAGON_EEPROM_SCL) {
			eeprom_i2c_set(ncr->eeprom, BITBANG_I2C_SCL, (val & 0x80) != 0);
			memcpy(oktagon_eeprom, ncr->eeprom_data + 0x100, 16);
		} else if (addr == OKTAGON_EEPROM_SDA) {
			eeprom_i2c_set(ncr->eeprom, BITBANG_I2C_SDA, (val & 0x80) != 0);
			memcpy(oktagon_eeprom, ncr->eeprom_data + 0x100, 16);
		} else if (addr >= OKTAGON_DMA_START && addr < OKTAGON_DMA_END) {
			ncr->data = val;
			ncr->data_valid_r = true;
			esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		} else if (addr == OKTAGON_INTENA) {
			ncr->states[0] = val;
			set_irq2_oktagon(ncr);
			return;
		}
		if (addr < OKTAGON_ESP_ADDR || addr >= OKTAGON_ESP_ADDR + 0x100) {
			return;
		}
		reg_shift = 1;

	} else if (isncr(ncr, ncr_alf3_scsi)) {

		if (addr >= 0xd000 && addr < 0xe000) {
			ncr->data = val;
			ncr->data_valid_r = true;
			esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		} else if (addr == 0xa001) {
			ncr->states[0] = val;
			set_irq2_alf3(ncr);
			return;
		}
		if (addr < 0x8000 || addr >= 0x8000 + 0x100)
			return;

		reg_shift = 1;

	} else if (isncr(ncr, ncr_fastlane_scsi)) {

		if (addr >= FASTLANE_HARDBITS) {
			if (addr == FASTLANE_HARDBITS) {
				int oldstate = ncr->states[0];
				ncr->states[0] = val;
				if (!(oldstate & FLSC_PB_ENABLE_DMA) && (ncr->states[0] & FLSC_PB_ENABLE_DMA))
					esp_dma_enable(ncr->devobject.lsistate, 1);
				else if ((oldstate & FLSC_PB_ENABLE_DMA) && !(ncr->states[0] & FLSC_PB_ENABLE_DMA))
					esp_dma_enable(ncr->devobject.lsistate, 0);
				set_irq2_fastlane(ncr);
			} else if (addr == FASTLANE_RESETDMA) {
				ncr->dma_cnt = 4;
				ncr->dma_ptr = 0;
			}
			return;
		} else if (addr < 0x01000000) {
			// DMA address bits  0 to 23 comes from address bus.
			// DMA address bits 24 to 31 comes from data bus.
			ncr->dma_cnt--;
			if (ncr->dma_cnt == 3) {
				ncr->dma_ptr = val << 24;
				ncr->dma_ptr |= addr;
			}
			if (ncr->dma_cnt == 0 && (ncr->states[0] & FLSC_PB_ENABLE_DMA))
				esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		}
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230II)) {
		addr &= 0xffff;
		if (addr >= 0x20) {
			if (addr & 0x10) {
				ncr->dma_cnt = 0;
				ncr->dma_ptr = 0;
			}
			ncr->dma_ptr |= (val & 0xff) << ((3 - ncr->dma_cnt) * 8);
			ncr->dma_cnt++;
			if (ncr->dma_cnt == 4)
				esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		}
		reg_shift = 1;
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230III)) {
		addr &= 0xffff;
		if (addr >= 0x20) {
			write_log(_T("BOARD_BLIZZARD_SUB_1230III WRITE %08x %08x\n"), addr, M68K_GETPC);
			return;
		}
		if (addr >= 0x10 && addr < 0x20) {
			fas408_write_fifo(ncr->devobject.lsistate, val);
			return;
		}
		reg_shift = 0;

	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_2060)) {
		if (addr >= BLIZZARD_2060_DMA_OFFSET) {
			//write_log (_T("Blizzard DMA PUT %08x %02X\n"), addr, (uae_u8)val);
			addr &= 0xf;
			addr >>= 2;
			addr = 3 - addr;

			ncr->dma_ptr &= ~(0xff << (addr * 8));
			ncr->dma_ptr |= (val & 0xff) << (addr * 8);
			if (addr == 3)
				esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		} else if (addr >= BLIZZARD_2060_LED_OFFSET) {
			ncr->led = val;
			return;
		}
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230IV) || ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1260)) {
		if (!is_board_enabled(&currprefs, ROMTYPE_BLIZKIT4, 0))
			return;
		if (addr >= BLIZZARD_SCSI_KIT4_DMA_OFFSET) {
			addr &= 0x18000;
			if (addr == 0x18000) {
				ncr->dma_ptr = 0;
				ncr->dma_cnt = 4;
			} else {
				ncr->dma_ptr <<= 8;
				ncr->dma_ptr |= (uae_u8)val;
				ncr->dma_cnt--;
				if (ncr->dma_cnt == 0)
					esp_dma_enable(ncr->devobject.lsistate, 1);
			}
			//write_log(_T("Blizzard DMA PUT %08x %02X\n"), oldaddr, (uae_u8)val);
			return;
		}
	} else if (ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK1)) {
		if (addr >= CYBERSTORM_MK1_JUMPER_OFFSET) {
			if (addr == CYBERSTORM_MK1_JUMPER_OFFSET)
				esp_dma_enable(ncr->devobject.lsistate, 1);
		} else if (addr >= CYBERSTORM_MK1_DMA_OFFSET) {
			addr &= 7;
			addr >>= 1;
			addr = 3 - addr;
			ncr->dma_ptr &= ~(0xff << (addr * 8));
			ncr->dma_ptr |= (val & 0xff) << (addr * 8);
			return;
		} else if (addr >= CYBERSTORM_MK2_LED_OFFSET) {
			ncr->led = val;
			return;
		}
	} else if (ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK2)) {
		if (addr >= CYBERSTORM_MK2_DMA_OFFSET) {
			addr &= 0xf;
			addr >>= 2;
			addr = 3 - addr;
			ncr->dma_ptr &= ~(0xff << (addr * 8));
			ncr->dma_ptr |= (val & 0xff) << (addr * 8);
			if (addr == 0)
				esp_dma_enable(ncr->devobject.lsistate, 1);
			return;
		 } else if (addr >= CYBERSTORM_MK2_LED_OFFSET) {
			 ncr->led = val;
			 return;
		 }
	} else if (isncr(ncr, ncr_rapidfire_scsi)) {
		reg_shift = 1;
		if (addr == 0x10040) {
			ncr->states[0] = val;
			esp_dma_enable(ncr->devobject.lsistate, 1);
			//write_log(_T("DKB IO PUT %02x %08x\n"), val & 0xff, M68K_GETPC);
			return;
		}
		if (addr >= 0x10020 && addr < 0x10028) {
			//write_log(_T("DKB PUT BYTE %02x\n"), val & 0xff);
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size) {
				ncr->fakedma_data_buf[ncr->fakedma_data_offset++] = val;
				if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
					memcpy(ncr->fakedma_data_write_buffer, ncr->fakedma_data_buf, ncr->fakedma_data_size);
					esp_fake_dma_done(ncr->devobject.lsistate);
				}
			}
			return;
		}
		if (addr < 0x10000 || addr >= 0x10020) {
			write_log(_T("DKB IO %08X PUT %02x %08x\n"), addr, val & 0xff, M68K_GETPC);
			return;
		}
	} else if (ISCPUBOARD(BOARD_DKB, BOARD_DKB_SUB_12x0)) {
		if (!(currprefs.cpuboard_settings & 1))
			return;
		if (addr == 0x10100) {
			ncr->states[0] = val;
			esp_dma_enable(ncr->devobject.lsistate, 1);
			//write_log(_T("DKB IO PUT %02x %08x\n"), val & 0xff, M68K_GETPC);
			return;
		}
		if (addr >= 0x10080 && addr < 0x10088) {
			//write_log(_T("DKB PUT BYTE %02x\n"), val & 0xff);
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size) {
				ncr->fakedma_data_buf[ncr->fakedma_data_offset++] = val;
				if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
					memcpy(ncr->fakedma_data_write_buffer, ncr->fakedma_data_buf, ncr->fakedma_data_size);
					esp_fake_dma_done(ncr->devobject.lsistate);
				}
			}
			return;
		}
		if (addr < 0x10000 || addr >= 0x10040) {
			write_log(_T("DKB IO %08X PUT %02x %08x\n"), addr, val & 0xff, M68K_GETPC);
			return;
		}
	} else if (ISCPUBOARD(BOARD_MTEC, BOARD_MTEC_SUB_EMATRIX530) || ISCPUBOARD(BOARD_DCE, BOARD_DCE_SUB_TYPHOON2) || ncr == ncr_mtec_mastercard) {
		if ((addr & 0xf000) >= 0xe000) {
			if ((addr & 0x3ff) <= 7) {
				if (ncr->fakedma_data_offset < ncr->fakedma_data_size) {
					ncr->fakedma_data_buf[ncr->fakedma_data_offset++] = val;
					if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
						memcpy(ncr->fakedma_data_write_buffer, ncr->fakedma_data_buf, ncr->fakedma_data_size);
						esp_fake_dma_done(ncr->devobject.lsistate);
						ncr->states[0] = 0;
					}
				}
			}
			return;
		}
		if (addr < 0xc000 || addr >= 0xe000)
			return;
 		if (addr & 1)
			return;
		if ((currprefs.cpuboard_settings & 1) && ncr != ncr_mtec_mastercard)
			return;
		reg_shift = 3;
	}
	if (!ncr->devobject.lsistate)
		return;
	addr >>= reg_shift;
	addr &= IO_MASK;
	esp_reg_write(ncr->devobject.lsistate, (addr), val);
	*reg = addr;
}

static void ncr9x_io_bput(struct ncr9x_state *ncr, uaecptr addr, uae_u32 val)
{
	int reg = -1;
	ncr9x_io_bput3(ncr, addr, val, &reg);
#if NCR_DEBUG > 1
	if (ncr)
		write_log(_T("ESP write %02X (%08X) %02X %08X\n"), reg & 0xff, addr, val & 0xff, M68K_GETPC);
#endif
}


static uae_u32 ncr9x_io_bget3(struct ncr9x_state *ncr, uaecptr addr, int *reg)
{
	uae_u8 v = 0xff;
	int reg_shift = 2;
	uaecptr oldaddr = addr;

	if (!ncr)
		return v;

	addr &= ncr->board_mask;

	if (isncr(ncr, ncr_scram5394_scsi)) {

		if (addr >= 0x8000 && addr < 0xa000) {
			if (ncr->fakedma_data_offset >= ncr->fakedma_data_size)
				return 0;
			v = ncr->fakedma_data_buf[ncr->fakedma_data_offset++];
			if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
				esp_fake_dma_done(ncr->devobject.lsistate);
			}
			return v;
		}
		if (addr & 1)
			return v;
		if (addr == 0) {
			v = ncr->rom[0];
			v |= !ncr->chipirq ? 0x80 : 0x00;
			return v;
		}
		if (addr < 0x4000 || addr >= 0xc000) {
			v = ncr->rom[(addr >> 1) & 8191];
			return v;
		}
		if (addr < 0x6000)
			return v;
		reg_shift = 1;

	} else if (ncr == ncr_multievolution_scsi) {

		reg_shift = 1;
		if (addr & 0x1000) {
			if (ncr->fakedma_data_offset >= ncr->fakedma_data_size)
				return 0;
			v = ncr->fakedma_data_buf[ncr->fakedma_data_offset++];
			if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
				esp_fake_dma_done(ncr->devobject.lsistate);
				//write_log(_T("MultiEvolution fake dma finished\n"));
			}
			//write_log(_T("MultiEvolution fake dma %02x %d/%d\n"), v, ncr->fakedma_data_offset, ncr->fakedma_data_size);
			return v;
		}

	} else if (isncr(ncr, ncr_golemfast_scsi)) {

		reg_shift = 1;
		if ((addr & 0x8400) == 0x8400) {
			if (ncr->fakedma_data_offset >= ncr->fakedma_data_size)
				return 0;
			v = ncr->fakedma_data_buf[ncr->fakedma_data_offset++];
			if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
				esp_fake_dma_done(ncr->devobject.lsistate);
			}
			//write_log(_T("FAKEDMA READ %08x %02X %08x (%d/%d)\n"), addr, v, M68K_GETPC, ncr->fakedma_data_offset, ncr->fakedma_data_size);
			return v;
		}

	} else if (ncr == ncr_squirrel_scsi) {

		reg_shift = 0;
		if (addr >= 16) {
			esp_dma_enable(ncr->devobject.lsistate, 1);
			v = ncr->data;
			ncr->data_valid_w = false;
			return v;
		}

	} else if (isncr(ncr, ncr_masoboshi_scsi)) {

		if (addr == MASOBOSHI_ESP_ADDR + 3 * 2 && (ncr->states[0] & 0x80))
			return 2;

		if (addr >= 0xf000 && addr < 0xf800)
			addr &= ~0x0700;

		if (ncr->states[0] & 0x80)
			ncr->states[0] &= ~0x80;

		if (addr >= 0xf00c && addr < 0xf010) {
			int shift = (3 - (addr - 0xf00c)) * 8;
			uae_u32 mask = 0xff << shift;
			if (addr == 0xf04f)
				write_log(_T("MASOBOSHI SCSI DMA PTR READ = %08x %08x\n"), ncr->dma_ptr, M68K_GETPC);
			return ncr->dma_ptr >> shift;
		}
		if (addr >= 0xf008 && addr < 0xf00c) {
			if (addr == 0xf00b)
				write_log(_T("MASOBOSHI SCSI DMA LEN READ = %04x %08x\n"), ncr->dma_cnt, v, M68K_GETPC);
			if (addr == 0xf00a)
				return ncr->dma_cnt >> 8;
			if (addr == 0xf00b)
				return ncr->dma_cnt >> 0;
			return v;
		}


		if (addr >= 0xf000 && addr <= 0xf007) {
			int idx = addr - 0xf000;
			if (addr == 0xf000) {
				ncr->states[0] |= 2;
				ncr->states[0] |= 1;
				if (esp_dreq(&ncr->devobject)) {
					ncr->states[0] &= ~1; // data request
				}
				if (ncr->boardirqlatch) {
					ncr->states[0] &= ~2; // scsi interrupt
				}
#if 0
				if (ncr->chipirq) {
					ncr->states[0] &= ~1;
				}
#endif
			}
			v = ncr->states[idx];
#if 0
			write_log(_T("MASOBOSHI IO %08X GET %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			return v;
		}
#if 0
		if (addr == 0xf007) {
			v = ncr->states[0];
		}
#endif
		if (addr >= MASOBOSHI_DMA_START && addr < MASOBOSHI_DMA_END) {
			esp_dma_enable(ncr->devobject.lsistate, 1);
			v = ncr->data;
			ncr->data_valid_w = false;
#if NCR_DEBUG > 2
			write_log(_T("MASOBOSHI DMA %08X GET %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			return v;
		}
		if (addr < MASOBOSHI_ESP_ADDR || addr >= MASOBOSHI_ESP_ADDR + 0x100) {
#if NCR_DEBUG
			write_log(_T("MASOBOSHI IO %08X GET %02x %08x\n"), addr, v, M68K_GETPC);
#endif
			return v;
		}
		reg_shift = 1;
		addr &= 0x3f;

	} else if (isncr(ncr, ncr_trifecta_scsi)) {

		if (addr >= 0x200)
			return v;

		if (!(addr & 1))
			return v;

		reg_shift = 1;
		addr &= 0x3f;

	} else if (isncr(ncr, ncr_oktagon2008_scsi)) {
		if (addr == OKTAGON_EEPROM_SCL) {
			return eeprom_i2c_set(ncr->eeprom, BITBANG_I2C_SCL, -1) ? 0x80 : 0x00;
		} else if (addr == OKTAGON_EEPROM_SDA) {
			return eeprom_i2c_set(ncr->eeprom, BITBANG_I2C_SDA, -1) ? 0x80 : 0x00;
		} else if (addr >= OKTAGON_DMA_START && addr < OKTAGON_DMA_END) {
			esp_dma_enable(ncr->devobject.lsistate, 1);
			v = ncr->data;
			ncr->data_valid_w = false;
			return v;
		} else if (addr == OKTAGON_INTENA) {
			return ncr->states[0];
		}
		if (addr < OKTAGON_ESP_ADDR || addr >= OKTAGON_ESP_ADDR + 0x100)
			return 0xff;
		reg_shift = 1;

	} else if (isncr(ncr, ncr_alf3_scsi)) {

		if (addr >= 0xd000 && addr < 0xe000) {
			esp_dma_enable(ncr->devobject.lsistate, 1);
			v = ncr->data;
			ncr->data_valid_w = false;
			return v;
		} else if (addr == 0xa000) {
			v = 0;
			if (ncr->rc->device_settings & 1)
				v |= 1 << 4;
			if (ncr->rc->device_settings & 2)
				v |= 1 << 5;
			if (ncr->rc->device_settings & 4)
				v |= 1 << 3;
			if (ncr->rc->device_settings & 8)
				v |= 1 << 7;
			if (ncr->rc->device_settings & 16)
				v |= 1 << 6;
			v |= ncr->rc->device_id & 7;
			return v;
		} else if (addr == 0xa001) {
			v = ncr->boardirq ? 1 : 0;
			if (ncr->rc->device_settings & (1 << 5))
				v |= 2;
			return v;
		}
		if (addr < 0x8000 || addr >= 0x8000 + 0x100)
			return 0xff;
		reg_shift = 1;

	} else if (isncr(ncr, ncr_fastlane_scsi)) {
		if (addr >= FASTLANE_HARDBITS) {
			if (addr == FASTLANE_HARDBITS) {
				uae_u8 v = ncr->states[0];
				v &= ~(FLSC_HB_DISABLED | FLSC_HB_BUSID6 | FLSC_HB_SEAGATE | FLSC_HB_SLOW | FLSC_HB_SYNCHRON);
				return v;
			}
			return 0;
		}
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230II)) {
		addr &= 0xffff;
		if (addr >= 0x20) {
			return 0;
		}
		reg_shift = 1;
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230III)) {
		addr &= 0xffff;
		if (addr >= 0x10 && addr < 0x20) {
			v = (uae_u8)fas408_read_fifo(ncr->devobject.lsistate);
			return v;
		}
		if (addr >= 0x20) {
			write_log(_T("BOARD_BLIZZARD_SUB_1230III %08x %08x\n"), addr, M68K_GETPC);
			return 0;
		}
		reg_shift = 0;

	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_2060)) {
		if (addr >= BLIZZARD_2060_DMA_OFFSET) {
			write_log(_T("Blizzard DMA GET %08x\n"), addr);
			return 0;
		} else if (addr >= BLIZZARD_2060_LED_OFFSET) {
			return ncr->led;
		}
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230IV) || ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1260)) {
		if (!is_board_enabled(&currprefs, ROMTYPE_BLIZKIT4, 0))
			return 0;
		if (addr >= BLIZZARD_SCSI_KIT4_DMA_OFFSET)
			return 0;
	} else if (ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK1)) {
		if (addr >= CYBERSTORM_MK1_JUMPER_OFFSET) {
			return 0xff;
		} else if (addr >= CYBERSTORM_MK1_DMA_OFFSET) {
			return 0;
		} else if (addr >= CYBERSTORM_MK1_LED_OFFSET) {
			return ncr->led;
		}
	} else if (ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK2)) {
		if (addr >= CYBERSTORM_MK2_DMA_OFFSET) {
			return 0;
		} else if (addr >= CYBERSTORM_MK2_LED_OFFSET) {
			return ncr->led;
		}
	} else if (isncr(ncr, ncr_rapidfire_scsi)) {
		reg_shift = 1;
		if (addr == 0x10040) {
			uae_u8 v = 0;
			if (ncr->chipirq || ncr->boardirq)
				v |= 0x40;
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size)
				v |= 0x80;
			ncr->boardirq = false;
			//write_log(_T("DKB IO GET %02x %08x\n"), v, M68K_GETPC);
			return v;
		}
		if (addr >= 0x10020 && addr < 0x10028) {
			//write_log(_T("DKB GET BYTE %02x %08x\n"), ncr->fakedma_data_buf[ncr->fakedma_data_offset], M68K_GETPC);
			if (ncr->fakedma_data_offset >= ncr->fakedma_data_size)
				return 0;
			v = ncr->fakedma_data_buf[ncr->fakedma_data_offset++];
			if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
				esp_fake_dma_done(ncr->devobject.lsistate);
				//write_log(_T("DKB fake dma finished\n"));
			}
			return v;
		}
		if (addr < 0x10000 || addr >= 0x10020) {
			write_log(_T("DKB IO GET %08x %08x\n"), addr, M68K_GETPC);
			return 0;
		}
	} else if (ISCPUBOARD(BOARD_DKB, BOARD_DKB_SUB_12x0)) {
		if (!(currprefs.cpuboard_settings & 1))
			return 0x00;
		if (addr == 0x10100) {
			uae_u8 v = 0;
			if (ncr->chipirq || ncr->boardirq)
				v |= 0x40;
			if (ncr->fakedma_data_offset < ncr->fakedma_data_size)
				v |= 0x80;
			ncr->boardirq = false;
			//write_log(_T("DKB IO GET %02x %08x\n"), v, M68K_GETPC);
			return v;
		}
		if (addr >= 0x10080 && addr < 0x10088) {
			//write_log(_T("DKB GET BYTE %02x\n"), ncr->fakedma_data_buf[ncr->fakedma_data_offset]);
			if (ncr->fakedma_data_offset >= ncr->fakedma_data_size)
				return 0;
			v = ncr->fakedma_data_buf[ncr->fakedma_data_offset++];
			if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
				esp_fake_dma_done(ncr->devobject.lsistate);
				//write_log(_T("DKB fake dma finished\n"));
			}
			return v;
		}
		if (addr < 0x10000 || addr >= 0x10040) {
			write_log(_T("DKB IO GET %08x %08x\n"), addr, M68K_GETPC);
			return 0;
		}
	} else if (ISCPUBOARD(BOARD_MTEC, BOARD_MTEC_SUB_EMATRIX530) || ISCPUBOARD(BOARD_DCE, BOARD_DCE_SUB_TYPHOON2) || ncr == ncr_mtec_mastercard) {
		if ((addr & 0xf000) >= 0xe000) {
			if ((addr & 0x3ff) <= 7) {
				if (ncr->fakedma_data_offset >= ncr->fakedma_data_size) {
					ncr->states[0] = 0;
					return 0;
				}
				v = ncr->fakedma_data_buf[ncr->fakedma_data_offset++];
				if (ncr->fakedma_data_offset == ncr->fakedma_data_size) {
					esp_fake_dma_done(ncr->devobject.lsistate);
					ncr->states[0] = 0;
				}
				return v;
			}
			return 0xff;
		}
		if ((addr & 1) && !(addr & 0x0800)) {
			// dma request
			return ncr->states[0] ? 0xff : 0x7f;
		}
		if ((addr & 1) && (addr & 0x0800)) {
			// hardware revision?
			return 0x7f;
		}
		if (addr & 1)
			return 0x7f;
		if ((currprefs.cpuboard_settings & 1) && ncr != ncr_mtec_mastercard)
			return 0x7f;
		if (addr < 0xc000 || addr >= 0xe000)
			return 0x7f;
		reg_shift = 3;
	}
	if (!ncr->devobject.lsistate)
		return v;
	addr >>= reg_shift;
	addr &= IO_MASK;
	v = (uae_u8)esp_reg_read(ncr->devobject.lsistate, (addr));
	*reg = addr;
	return v;
}

static uae_u32 ncr9x_io_bget(struct ncr9x_state *ncr, uaecptr addr)
{
	int reg = -1;
	uae_u8 v = ncr9x_io_bget3(ncr, addr, &reg);
#if NCR_DEBUG > 1
	if (ncr)
		write_log(_T("ESP read %02X (%08X) %02X %08X\n"), reg & 0xff, addr, v, M68K_GETPC);
#endif
	return v;
}

static uae_u8 read_rombyte(struct ncr9x_state *ncr, uaecptr addr)
{
	uae_u8 v = 0xff;
	if (ncr->flashrom) {
		if ((!ncr->romisoddonly && !ncr->romisevenonly) || (ncr->romisoddonly && (addr & 1)) || (ncr->romisevenonly && !(addr & 1)))
			v = flash_read(ncr->flashrom, addr >> 1);
	} else {
		v = ncr->rom[addr];
	}
	return v;
}
static void write_rombyte(struct ncr9x_state *ncr, uaecptr addr, uae_u8 v)
{
	if (!ncr->flashrom)
		return;
	if ((!ncr->romisoddonly && !ncr->romisevenonly) || (ncr->romisoddonly && (addr & 1)) || (ncr->romisevenonly && !(addr & 1)))
		flash_write(ncr->flashrom, addr >> 1, v);
}

static uae_u32 ncr9x_bget2(struct ncr9x_state *ncr, uaecptr addr)
{
	uae_u32 v = 0;

	addr &= ncr->board_mask;
	if (ncr->rom && addr >= ncr->rom_start && addr < ncr->rom_end) {
		if (addr < ncr->io_start || (!ncr->romisoddonly && !ncr->romisevenonly) || (ncr->romisoddonly && (addr & 1)) || (ncr->romisevenonly && !(addr & 1)))
			return read_rombyte (ncr, addr - ncr->rom_offset);
	}
	if (ncr->io_end && (addr < ncr->io_start || addr >= ncr->io_end))
		return v;

	v = ncr9x_io_bget(ncr, addr);

#if NCR_DEBUG > 5
	write_log(_T("ncr9x_bget2 %08x %02x %08x\n"), addr, (uae_u8)v, M68K_GETPC);
#endif

	return v;
}

static void ncr9x_bput2(struct ncr9x_state *ncr, uaecptr addr, uae_u32 val)
{
	uae_u32 v = val;

#if NCR_DEBUG > 5
	write_log(_T("ncr9x_bput %08x %02x %08x\n"), addr, (uae_u8)val, M68K_GETPC);
#endif

	addr &= ncr->board_mask;
	if (ncr->rom && addr >= ncr->rom_start && addr < ncr->rom_end && !ncr->rom_bypass_if_write) {
		if (addr < ncr->io_start || (!ncr->romisoddonly && !ncr->romisevenonly) || (ncr->romisoddonly && (addr & 1)) || (ncr->romisevenonly && !(addr & 1))) {
			write_rombyte(ncr, addr, val);
			return;
		}
	}
	if (ncr->io_end && (addr < ncr->io_start || addr >= ncr->io_end))
		return;
	ncr9x_io_bput(ncr, addr, val);
}

static uae_u32 REGPARAM2 ncr9x_lget(struct ncr9x_state *ncr, uaecptr addr)
{
	uae_u32 v;
	addr &= ncr->board_mask;
	if (isncr(ncr, ncr_oktagon2008_scsi)) {
		v =  ncr9x_io_bget(ncr, addr + 0) << 24;
		v |= ncr9x_io_bget(ncr, addr + 1) << 16;
		v |= ncr9x_io_bget(ncr, addr + 2) <<  8;
		v |= ncr9x_io_bget(ncr, addr + 3) <<  0;
	} else {
		v =  ncr9x_bget2(ncr, addr + 0) << 24;
		v |= ncr9x_bget2(ncr, addr + 1) << 16;
		v |= ncr9x_bget2(ncr, addr + 2) <<  8;
		v |= ncr9x_bget2(ncr, addr + 3) <<  0;
	}
	return v;
}

static uae_u32 REGPARAM2 ncr9x_wget(struct ncr9x_state *ncr, uaecptr addr)
{
	uae_u32 v;
	addr &= ncr->board_mask;
	if (isncr(ncr, ncr_oktagon2008_scsi)) {
		v = ncr9x_io_bget(ncr, addr) << 8;
		v |= ncr9x_io_bget(ncr, addr + 1);
	} else {
		v = ncr9x_bget2(ncr, addr) << 8;
		v |= ncr9x_bget2(ncr, addr + 1);
	}
	return v;
}

static uae_u32 REGPARAM2 ncr9x_bget(struct ncr9x_state *ncr, uaecptr addr)
{
	uae_u32 v;
	addr &= ncr->board_mask;
	if (!ncr->configured) {
		addr &= 65535;
		if (addr >= sizeof ncr->acmemory)
			return 0;
		return ncr->acmemory[addr];
	}
	v = ncr9x_bget2(ncr, addr);
	return v;
}

static void REGPARAM2 ncr9x_lput(struct ncr9x_state *ncr, uaecptr addr, uae_u32 l)
{
	addr &= ncr->board_mask;
	if (isncr(ncr, ncr_oktagon2008_scsi)) {
		ncr9x_io_bput(ncr, addr + 0, l >> 24);
		ncr9x_io_bput(ncr, addr + 1, l >> 16);
		ncr9x_io_bput(ncr, addr + 2, l >>  8);
		ncr9x_io_bput(ncr, addr + 3, l >>  0);
	} else {
		ncr9x_bput2(ncr, addr + 0, l >> 24);
		ncr9x_bput2(ncr, addr + 1, l >> 16);
		ncr9x_bput2(ncr, addr + 2, l >>  8);
		ncr9x_bput2(ncr, addr + 3, l >>  0);
	}
}

static void REGPARAM2 ncr9x_wput(struct ncr9x_state *ncr, uaecptr addr, uae_u32 w)
{
	w &= 0xffff;
	addr &= ncr->board_mask;
	if (!ncr->configured) {
		addr &= 65535;
		switch (addr)
		{
			case 0x44:
			map_banks_z3(ncr->bank, expamem_board_pointer >> 16, FASTLANE_BOARD_SIZE >> 16);
			ncr->baseaddress = expamem_board_pointer;
			ncr->configured = 1;
			expamem_next (ncr->bank, NULL);
			break;
		}
		return;
	}
	if (isncr(ncr, ncr_oktagon2008_scsi)) {
		ncr9x_io_bput(ncr, addr, w >> 8);
		ncr9x_io_bput(ncr, addr + 1, w);
	} else {
		ncr9x_bput2(ncr, addr, w >> 8);
		ncr9x_bput2(ncr, addr + 1, w);
	}
}

static void REGPARAM2 ncr9x_bput(struct ncr9x_state *ncr, uaecptr addr, uae_u32 b)
{
	b &= 0xff;
	addr &= ncr->board_mask;
	if (!ncr->configured) {
		addr &= 65535;
		switch (addr)
		{
			case 0x48:
			if (isncr(ncr, ncr_fastlane_scsi))
				return;
			map_banks_z2(ncr->bank, expamem_board_pointer >> 16, expamem_board_size >> 16);
			ncr->configured = 1;
			ncr->baseaddress = expamem_board_pointer;
			expamem_next (ncr->bank, NULL);
			break;
			case 0x4c:
			ncr->configured = 1;
			expamem_shutup(ncr->bank);
			break;
		}
		return;
	}
	ncr9x_bput2(ncr, addr, b);
}

static struct ncr9x_state *getscsiboard(uaecptr addr)
{
	for (int i = 0; ncr_units[i]; i++) {
		if (!ncr_units[i]->baseaddress)
			return ncr_units[i];
		if ((addr & ~ncr_units[i]->board_mask) == ncr_units[i]->baseaddress)
			return ncr_units[i];
		if (ncr_units[i]->baseaddress2 && (addr & ~ncr_units[i]->board_mask2) == ncr_units[i]->baseaddress2)
			return ncr_units[i];
	}
	return NULL;
}

static void REGPARAM2 ncr9x_generic_bput (uaecptr addr, uae_u32 b)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (ncr)
		ncr9x_bput(ncr, addr, b);
}
static void REGPARAM2 ncr9x_generic_wput (uaecptr addr, uae_u32 b)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (ncr)
		ncr9x_wput(ncr, addr, b);
}
static void REGPARAM2 ncr9x_generic_lput (uaecptr addr, uae_u32 b)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (ncr)
		ncr9x_lput(ncr, addr, b);
}
static uae_u32 REGPARAM2 ncr9x_generic_bget (uaecptr addr)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (ncr)
		return ncr9x_bget(ncr, addr);
	return 0;
}
static uae_u32 REGPARAM2 ncr9x_generic_wget (uaecptr addr)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (ncr)
		return ncr9x_wget(ncr, addr);
	return 0;
}
static uae_u32 REGPARAM2 ncr9x_generic_lget (uaecptr addr)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (ncr)
		return ncr9x_lget(ncr, addr);
	return 0;
}
static uae_u8 *REGPARAM2 ncr9x_generic_xlate(uaecptr addr)
{
	struct ncr9x_state *ncr = getscsiboard(addr);
	if (!ncr)
		return NULL;
	addr &= ncr->board_mask;
	return ncr->rom + addr;

}
static int REGPARAM2 ncr9x_generic_check(uaecptr a, uae_u32 b)
{
	struct ncr9x_state *ncr = getscsiboard(a);
	if (!ncr)
		return 0;
	a &= ncr->board_mask;
	if (a >= ncr->rom_start && a + b < ncr->rom_end)
		return 1;
	return 0;
}

static addrbank ncr9x_bank_generic = {
	ncr9x_generic_lget, ncr9x_generic_wget, ncr9x_generic_bget,
	ncr9x_generic_lput, ncr9x_generic_wput, ncr9x_generic_bput,
	ncr9x_generic_xlate, ncr9x_generic_check, NULL, NULL, _T("53C94/FAS216"),
	ncr9x_generic_lget, ncr9x_generic_wget,
	ABFLAG_IO | ABFLAG_SAFE, S_READ, S_WRITE
};

uae_u32 cpuboard_ncr9x_scsi_get(uaecptr addr)
{
	return ncr9x_io_bget(ncr_blizzard_scsi, addr);
}
void cpuboard_ncr9x_scsi_put(uaecptr addr, uae_u32 v)
{
	ncr9x_io_bput(ncr_blizzard_scsi, addr, v);
}

uae_u32 masoboshi_ncr9x_scsi_get(uaecptr addr, int devnum)
{
	return ncr9x_io_bget(ncr_masoboshi_scsi[devnum], addr);
}
void masoboshi_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum)
{
	ncr9x_io_bput(ncr_masoboshi_scsi[devnum], addr, v);
}

uae_u32 trifecta_ncr9x_scsi_get(uaecptr addr, int devnum)
{
	return ncr9x_io_bget(ncr_trifecta_scsi[devnum], addr);
}
void trifecta_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum)
{
	ncr9x_io_bput(ncr_trifecta_scsi[devnum], addr, v);
}

uae_u32 golemfast_ncr9x_scsi_get(uaecptr addr, int devnum)
{
	return ncr9x_io_bget(ncr_golemfast_scsi[devnum], addr);
}
void golemfast_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum)
{
	ncr9x_io_bput(ncr_golemfast_scsi[devnum], addr, v);
}

uae_u32 squirrel_ncr9x_scsi_get(uaecptr addr, int devnum)
{
	return ncr9x_io_bget(ncr_squirrel_scsi, addr);
}
void squirrel_ncr9x_scsi_put(uaecptr addr, uae_u32 v, int devnum)
{
	ncr9x_io_bput(ncr_squirrel_scsi, addr, v);
}


static void ew(struct ncr9x_state *ncr, int addr, uae_u8 value)
{
	if (addr == 00 || addr == 02 || addr == 0x40 || addr == 0x42) {
		ncr->acmemory[addr] = (value & 0xf0);
		ncr->acmemory[addr + 2] = (value & 0x0f) << 4;
	} else {
		ncr->acmemory[addr] = ~(value & 0xf0);
		ncr->acmemory[addr + 2] = ~((value & 0x0f) << 4);
	}
}

static void ncr9x_reset_board(struct ncr9x_state *ncr);
static void ncr9x_reset(int hardreset)
{
	for (int i = 0; ncr_units[i]; i++) {
		ncr9x_reset_board(ncr_units[i]);
		ncr_units[i]->enabled = false;
	}
}

static void ncr9x_free(void)
{
	for (int i = 0; ncr_units[i]; i++) {
		freencrunit(ncr_units[i]);
	}
}

static void ncr9x_reset_board(struct ncr9x_state *ncr)
{
	if (!ncr)
		return;
	ncr->configured = 0;
	ncr->boardirq = false;
	ncr->chipirq = false;

	device_add_rethink(ncr9x_rethink);
	device_add_reset(ncr9x_reset);
	device_add_exit(ncr9x_free, NULL);
}

void ncr_squirrel_init(struct romconfig *rc, uaecptr baseaddress)
{
	struct ncr9x_state *ncr = getscsi(rc);

	if (!ncr)
		return;

	ncr->enabled = true;
	ncr->pcmcia = true;
	ncr->baseaddress = baseaddress;

	ncr9x_reset_board(ncr);
}

void ncr_golemfast_autoconfig_init(struct romconfig *rc, uaecptr baseaddress)
{
	struct ncr9x_state *ncr = getscsi(rc);

	if (!ncr)
		return;

	ncr->enabled = true;
	ncr->baseaddress = baseaddress;

	ncr9x_reset_board(ncr);
}

bool ncr_multievolution_init(struct autoconfig_info *aci)
{
	device_add_reset(ncr9x_reset);
	if (!aci->doinit)
		return true;

	struct ncr9x_state *ncr = getscsi(aci->rc);

	xfree(ncr->rom);
	ncr->rom = NULL;

	if (!ncr)
		return false;

	ncr->bank = &ncr9x_bank_generic;
	ncr->enabled = true;
	ncr->rom = xcalloc(uae_u8, 65536);

	ncr->configured = -1;
	ncr->board_mask = 0x1ffff;
	ncr->board_mask2 = 0xffff;
	ncr->baseaddress = 0xf00000;
	ncr->baseaddress2 = 0xef0000;
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = 0x10000;
	ncr->io_start = 0x1e000;
	ncr->io_end = ncr->io_start + 0x2000;
	ncr->irq6 = true;

	load_rom_rc(aci->rc, ROMTYPE_MEVOLUTION, 65536, 0, ncr->rom, 65536, 0);

	map_banks(ncr->bank, ncr->baseaddress2 >> 16, (ncr->board_mask + 1) >> 16, 0);

	aci->addrbank = &expamem_null;
	return true;
}

bool ncr_fastlane_autoconfig_init(struct autoconfig_info *aci)
{
	struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_FASTLANE);
	uae_u8 *rom = xcalloc(uae_u8, FASTLANE_ROM_SIZE * 4);
	if (z) {
		// memory board at offset 0x100
		int autoconfig_offset = 0;
		memset(rom, 0xff, FASTLANE_ROM_SIZE * 4);
		for (int i = 0; i < FASTLANE_ROM_SIZE; i++) {
			int ia = i - autoconfig_offset;
			uae_u8 b;
			zfile_fread(&b, 1, 1, z);
			rom[i * 4 + 0] = b | 0x0f;
			rom[i * 4 + 2] = (b << 4) | 0x0f;
			if (ia >= 0 && ia < 0x20) {
				aci->autoconfig_raw[ia * 4 + 0] = b;
			} else if (ia >= 0x40 && ia < 0x60) {
				aci->autoconfig_raw[(ia - 0x40) * 4 + 2] = b;
			}
		}
		zfile_fclose(z);
	}

	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		xfree(rom);
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);

	xfree(ncr->rom);
	ncr->rom = rom;

	if (!ncr)
		return false;

	ncr->enabled = true;
	memcpy(ncr->acmemory, aci->autoconfig_raw, sizeof ncr->acmemory);
	ncr->rom_start = 0x800;
	ncr->rom_offset = 0;
	ncr->rom_bypass_if_write = true; // DMA pointer set can write to ROM space
	ncr->rom_end = FASTLANE_ROM_SIZE * 4;
	ncr->io_start = 0;
	ncr->io_end = 0;
	ncr->bank = &ncr9x_bank_generic;

	ncr9x_reset_board(ncr);

	aci->addrbank = ncr->bank;
	return true;
}

static const uae_u8 oktagon_autoconfig[16] = {
	0xd1, 0x05, 0x00, 0x00, 0x08, 0x2c, 0x00, 0x00, 0x00, 0x00, OKTAGON_ROM_OFFSET >> 8, OKTAGON_ROM_OFFSET & 0xff
};

bool ncr_oktagon_autoconfig_init(struct autoconfig_info *aci)
{
	device_add_reset(ncr9x_reset);
	aci->autoconfigp = oktagon_autoconfig;
	if (!aci->doinit)
		return true;

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr)
		return false;

	xfree(ncr->rom);
	ncr->rom = NULL;
	eeprom_free(ncr->eeprom);
	ncr->eeprom = NULL;

	ncr->enabled = true;
	memset (ncr->acmemory, 0xff, sizeof ncr->acmemory);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = OKTAGON_ROM_SIZE * 2;
	ncr->io_start = 0x2000;
	ncr->io_end = ncr->rom_end;
	ncr->romisoddonly = true;
	ncr->bank = &ncr9x_bank_generic;

	memset(ncr->eeprom_data, 0xff, OKTAGON_EEPROM_SIZE);
	memcpy(ncr->eeprom_data + 0x100, oktagon_eeprom, 16);
	ncr->eeprom = eeprom_new(ncr->eeprom_data, OKTAGON_EEPROM_SIZE, NULL);
	ncr->rom = xcalloc (uae_u8, OKTAGON_ROM_SIZE * 6);
	memset(ncr->rom, 0xff, OKTAGON_ROM_SIZE * 6);

	ncr9x_reset_board(ncr);

	if (!aci->rc->autoboot_disabled) {
		struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_OKTAGON);
		if (z) {
			// memory board at offset 0x100
			memset(ncr->rom, 0xff, OKTAGON_ROM_SIZE * 4);
			for (int i = 0; i < 0x1000 / 2; i++) {
				uae_u8 b;
				zfile_fread(&b, 1, 1, z);
				ncr->rom[OKTAGON_ROM_OFFSET + i * 4 + 0] = b;
				zfile_fread(&b, 1, 1, z);
				ncr->rom[OKTAGON_ROM_OFFSET + i * 4 + 2] = b;
			}
			for (int i = 0; i < OKTAGON_ROM_SIZE - 0x1000; i++) {
				uae_u8 b;
				zfile_fread(&b, 1, 1, z);
				ncr->rom[0x2000 + i * 4 + 1] = b;
				zfile_fread(&b, 1, 1, z);
				ncr->rom[0x2000 + i * 4 + 3] = b;
			}
			zfile_fclose(z);
		}
	}
	for (int i = 0; i < 16; i++) {
		uae_u8 b = oktagon_autoconfig[i];
		ew(ncr, i * 4, b);
	}

	aci->addrbank = ncr->bank;
	return true;
}

static const uae_u8 alf3_autoconfig[16] = {
	0xd1, 0x03, 0x40, 0x00, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0xC0, 0x16, 0x68
};

bool ncr_alf3_autoconfig_init(struct autoconfig_info *aci)
{
	struct zfile *z = read_device_from_romconfig(aci->rc, ROMTYPE_ALF3);
	if (z) {
		zfile_fread(aci->autoconfig_raw, 128, 1, z);
		zfile_fclose(z);
	} else {
		memcpy(aci->autoconfig_bytes, alf3_autoconfig, 16);
		aci->autoconfigp = aci->autoconfig_bytes;
	}
	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr)
		return false;

	xfree(ncr->rom);
	ncr->rom = NULL;
	eeprom_free(ncr->eeprom);
	ncr->eeprom = NULL;

	ncr->enabled = true;
	memset(ncr->acmemory, 0xff, sizeof ncr->acmemory);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = 0x8000;
	ncr->io_start = 0x8000;
	ncr->io_end = ncr->io_start + 0x8000;
	ncr->romisoddonly = true;
	ncr->bank = &ncr9x_bank_generic;

	ncr->irq6 = (ncr->rc->device_settings & (1 << 5)) == 0;

	ncr->rom = xcalloc(uae_u8, 65536);
	memset(ncr->rom, 0xff, 65536);

	ncr9x_reset_board(ncr);

	z = read_device_from_romconfig(aci->rc, ROMTYPE_ALF3);
	if (z) {
		zfile_fread(ncr->rom, 32768, 1, z);
		zfile_fclose(z);
	}

	if (!z) {
		for (int i = 0; i < 16; i++) {
			uae_u8 v = alf3_autoconfig[i];
			if (ncr->rc->autoboot_disabled && i == 0)
				v &= ~0x10;
			ew(ncr, i * 4, v);
		}
	} else {
		if (ncr->rc->autoboot_disabled)
			ncr->rom[0] &= ~0x10;
		for (int i = 0; i < 128; i++) {
			ncr->acmemory[i] = ncr->rom[i];
		}
	}

	aci->addrbank = ncr->bank;
	return true;
}

static const uae_u8 dkb_autoconfig[16] = {
	0xd2, 0x12, 0x40, 0x00, 0x07, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80
};

bool ncr_dkb_autoconfig_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_RAPIDFIRE);

	device_add_reset(ncr9x_reset);
	aci->autoconfigp = dkb_autoconfig;
	if (!aci->doinit)
		return true;

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr)
		return false;

	xfree(ncr->rom);
	ncr->rom = NULL;

	ncr->enabled = true;
	memset (ncr->acmemory, 0xff, sizeof ncr->acmemory);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = DKB_ROM_SIZE * 2;
	ncr->io_start = 0x10000;
	ncr->io_end = 0x20000;
	ncr->bank = &ncr9x_bank_generic;
	ncr->board_mask = 131071;
	ncr->romisevenonly = true;

	ncr9x_reset_board(ncr);

	ncr->rom = xcalloc (uae_u8, DKB_ROM_SIZE);
	load_rom_rc(aci->rc, ROMTYPE_CB_DKB, 32768, 0, ncr->rom, 32768, LOADROM_ONEFILL);
	for (int i = 0; i < 16; i++) {
		uae_u8 b = dkb_autoconfig[i];
		ew(ncr, i * 4, b);
	}
	ncr->flashrom_file = flashromfile_open(aci->rc->romfile);
	if (ncr->flashrom_file) {
		zfile_fread(ncr->rom, 32768, 1, ncr->flashrom_file);
	}
	ncr->flashrom = flash_new(ncr->rom, 32768, 32768, 0x1f, 0xdc, ncr->flashrom_file, FLASHROM_PARALLEL_EEPROM);

	aci->addrbank = ncr->bank;
	return true;
}

bool typhoon2scsi_init(struct autoconfig_info *aci)
{
	uae_u8 *rom = xcalloc(uae_u8, 65536);
	load_rom_rc(aci->rc, ROMTYPE_CB_TYPHOON2, 32768, 32768, rom, 65536, LOADROM_EVENONLY_ODDONE);
	memcpy(aci->autoconfig_raw, aci->rc->autoboot_disabled ? rom + 256 : rom, 128);
	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		xfree(rom);
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr) {
		xfree(rom);
		return false;
	}

	xfree(ncr->rom);
	ncr->rom = rom;

	ncr->enabled = true;
	memcpy(ncr->acmemory, aci->autoconfig_raw, sizeof aci->autoconfig_raw);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = 0x8000;
	ncr->io_start = 0x8000;
	ncr->io_end = 0x10000;
	ncr->bank = &ncr9x_bank_generic;
	ncr->board_mask = 65535;

	ncr9x_reset_board(ncr);

	aci->addrbank = ncr->bank;
	return true;
}

bool ncr_ematrix_autoconfig_init(struct autoconfig_info *aci)
{
	uae_u8 *rom = xcalloc(uae_u8, 65536);
	load_rom_rc(aci->rc, ROMTYPE_CB_EMATRIX, 32768, 32768, rom, 65536, LOADROM_EVENONLY_ODDONE);
	memcpy(aci->autoconfig_raw, aci->rc->autoboot_disabled ? rom + 256 : rom, 128);
	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		xfree(rom);
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr) {
		xfree(rom);
		return false;
	}

	xfree(ncr->rom);
	ncr->rom = rom;

	ncr->enabled = true;
	memcpy(ncr->acmemory, aci->autoconfig_raw, sizeof aci->autoconfig_raw);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = 0x8000;
	ncr->io_start = 0x8000;
	ncr->io_end = 0x10000;
	ncr->bank = &ncr9x_bank_generic;
	ncr->board_mask = 65535;

	ncr9x_reset_board(ncr);

	aci->addrbank = ncr->bank;
	return true;
}

void ncr_masoboshi_autoconfig_init(struct romconfig *rc, uaecptr baseaddress)
{
	struct ncr9x_state *ncr = getscsi(rc);

	if (!ncr)
		return;

	ncr->enabled = true;
	ncr->baseaddress = baseaddress;

	ncr9x_reset_board(ncr);
}

void ncr_trifecta_autoconfig_init(struct romconfig *rc, uaecptr baseaddress)
{
	struct ncr9x_state *ncr = getscsi(rc);

	if (!ncr)
		return;

	ncr->enabled = true;
	ncr->baseaddress = baseaddress;

	ncr9x_reset_board(ncr);
}


bool ncr_scram5394_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_SCRAM5394);

	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr)
		return false;

	ncr->enabled = true;
	ncr->bank = &ncr9x_bank_generic;
	ncr->board_mask = 65535;

	xfree(ncr->rom);
	ncr->rom = xcalloc(uae_u8, 8192);
	load_rom_rc(aci->rc, ROMTYPE_SCRAM5394, 8192, 0, ncr->rom, 8192, 0);
	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(ncr, i * 4, b);
	}

	ncr9x_reset_board(ncr);
	aci->addrbank = ncr->bank;
	return true;
}

bool ncr_mtecmastercard_init(struct autoconfig_info *aci)
{
	uae_u8 *rom = xcalloc(uae_u8, 65536);
	load_rom_rc(aci->rc, ROMTYPE_MASTERCARD, 32768, 0, rom, 65536, LOADROM_EVENONLY_ODDONE);
	memcpy(aci->autoconfig_raw, aci->rc->autoboot_disabled ? rom + 256 : rom, 128);
	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		xfree(rom);
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr) {
		xfree(rom);
		return false;
	}

	xfree(ncr->rom);
	ncr->rom = rom;

	ncr->enabled = true;
	memcpy(ncr->acmemory, aci->autoconfig_raw, sizeof aci->autoconfig_raw);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = 0x8000;
	ncr->io_start = 0x8000;
	ncr->io_end = 0x10000;
	ncr->bank = &ncr9x_bank_generic;
	ncr->board_mask = 65535;

	ncr9x_reset_board(ncr);

	aci->addrbank = ncr->bank;
	return true;
}

bool ncr_rapidfire_init(struct autoconfig_info *aci)
{
	const struct expansionromtype *ert = get_device_expansion_rom(ROMTYPE_RAPIDFIRE);

	device_add_reset(ncr9x_reset);
	if (!aci->doinit) {
		aci->autoconfigp = ert->autoconfig;
		return true;
	}

	struct ncr9x_state *ncr = getscsi(aci->rc);
	if (!ncr)
		return false;

	ncr->enabled = true;
	memset (ncr->acmemory, 0xff, sizeof ncr->acmemory);
	ncr->rom_start = 0;
	ncr->rom_offset = 0;
	ncr->rom_end = DKB_ROM_SIZE * 2;
	ncr->io_start = 0x10000;
	ncr->io_end = 0x20000;
	ncr->bank = &ncr9x_bank_generic;
	ncr->board_mask = 131071;
	ncr->romisevenonly = true;

	ncr9x_reset_board(ncr);

	ncr->rom = xcalloc (uae_u8, DKB_ROM_SIZE);
	load_rom_rc(aci->rc, ROMTYPE_RAPIDFIRE, 32768, 0, ncr->rom, 32768, LOADROM_ONEFILL);
	for (int i = 0; i < 16; i++) {
		uae_u8 b = ert->autoconfig[i];
		ew(ncr, i * 4, b);
	}
	ncr->flashrom_file = flashromfile_open(aci->rc->romfile);
	if (ncr->flashrom_file) {
		zfile_fread(ncr->rom, 32768, 1, ncr->flashrom_file);
	}
	ncr->flashrom = flash_new(ncr->rom, 32768, 32768, 0x1f, 0xdc, ncr->flashrom_file, FLASHROM_PARALLEL_EEPROM);

	aci->addrbank = ncr->bank;
	return true;
}


static void ncr9x_esp_scsi_init(struct ncr9x_state *ncr, ESPDMAMemoryReadWriteFunc read, ESPDMAMemoryReadWriteFunc write, void (*irq_func)(struct ncr9x_state*), int mode)
{
	ncr->board_mask = 0xffff;
	ncr->irq_func = irq_func ? irq_func : set_irq2;
	if (!ncr->devobject.lsistate)
		esp_scsi_init(&ncr->devobject, read, write, mode > 0 ? mode : 0);
	esp_scsi_reset(&ncr->devobject, ncr);
}

static void allocscsidevice(struct ncr9x_state *ncr, int ch, struct scsi_data *handle, int uae_unitnum)
{
	handle->privdata = ncr;
	ncr->scsid[ch] = xcalloc (SCSIDevice, 1);
	ncr->scsid[ch]->id = ch;
	ncr->scsid[ch]->handle = handle;
	handle->uae_unitnum = uae_unitnum;
}

static void add_ncr_scsi_hd(struct ncr9x_state *ncr, int ch, struct hd_hardfiledata *hfd, struct uaedev_config_info *ci, int uae_unitnum)
{
	struct scsi_data *handle = NULL;

	freescsi(ncr->scsid[ch]);
	ncr->scsid[ch] = NULL;
	if (!add_scsi_hd(&handle, ch, hfd, ci))
		return;
	allocscsidevice(ncr, ch, handle, uae_unitnum);
	ncr->enabled = true;
}

static void add_ncr_scsi_cd(struct ncr9x_state *ncr, int ch, int unitnum, int uae_unitnum)
{
	struct scsi_data *handle = NULL;

	freescsi(ncr->scsid[ch]);
	ncr->scsid[ch] = NULL;
	if (!add_scsi_cd(&handle, ch, unitnum))
		return;
	allocscsidevice(ncr, ch, handle, uae_unitnum);
	ncr->enabled = true;
}

static void add_ncr_scsi_tape(struct ncr9x_state *ncr, int ch, const TCHAR *tape_directory, bool readonly, int uae_unitnum)
{
	struct scsi_data *handle = NULL;

	freescsi(ncr->scsid[ch]);
	ncr->scsid[ch] = NULL;
	if (!add_scsi_tape(&handle, ch, tape_directory, readonly))
		return;
	allocscsidevice(ncr, ch, handle, uae_unitnum);
	ncr->enabled = true;
}

static void ncr9x_add_scsi_unit(struct ncr9x_state **ncrp, int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct ncr9x_state *ncr = allocscsi(ncrp, rc, ch);
	if (ch >= 0 && ncr) {
		ncr->dma_mask = rc->dma24bit ? 0x00ffffff : 0xffffffff;
		if (ci->type == UAEDEV_CD)
			add_ncr_scsi_cd (ncr, ch, ci->device_emu_unit, ci->uae_unitnum);
		else if (ci->type == UAEDEV_TAPE)
			add_ncr_scsi_tape (ncr, ch, ci->rootdir, ci->readonly, ci->uae_unitnum);
		else if (ci->type == UAEDEV_HDF)
			add_ncr_scsi_hd (ncr, ch, NULL, ci, ci->uae_unitnum);
	}
}

void cpuboard_ncr9x_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_blizzard_scsi, ch, ci, rc);
	ncr_blizzard_scsi->configured = -1;
	ncr_blizzard_scsi->enabled = true;
	if (ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK1) || ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK2)) {
		ncr9x_esp_scsi_init(ncr_blizzard_scsi, cyberstorm_mk1_mk2_dma_read, cyberstorm_mk1_mk2_dma_write, NULL, 0);
	} else if (ISCPUBOARD(BOARD_BLIZZARD, BOARD_BLIZZARD_SUB_1230III)) {
		ncr9x_esp_scsi_init(ncr_blizzard_scsi, fake_dma_null, fake_dma_null, NULL, 1);
	} else {
		ncr9x_esp_scsi_init(ncr_blizzard_scsi, blizzard_dma_read, blizzard_dma_write, NULL, 0);
	}
	if (ISCPUBOARD(BOARD_CYBERSTORM, BOARD_CYBERSTORM_SUB_MK1))
		ncr_blizzard_scsi->board_mask = 0xffff;
	else
		ncr_blizzard_scsi->board_mask = 0x1ffff;
}

void cpuboard_dkb_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_dkb1200_scsi, ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_dkb1200_scsi, fake_dma_read, fake_dma_write, set_irq2_dkb1200, 2);
}

void fastlane_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_fastlane_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_fastlane_scsi[ci->controller_type_unit], fastlane_dma_read, fastlane_dma_write, set_irq2_fastlane, 0);
	ncr_fastlane_scsi[ci->controller_type_unit]->board_mask = 32 * 1024 * 1024 - 1;
}

void oktagon_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_oktagon2008_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_oktagon2008_scsi[ci->controller_type_unit], fake2_dma_read, fake2_dma_write, set_irq2_oktagon, 0);
}

void alf3_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_alf3_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_alf3_scsi[ci->controller_type_unit], fake2_dma_read, fake2_dma_write, set_irq2_alf3, 0);
}

void masoboshi_add_scsi_unit (int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_masoboshi_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_masoboshi_scsi[ci->controller_type_unit], masoboshi_dma_read, masoboshi_dma_write, set_irq2_masoboshi, 0);
}

void trifecta_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_trifecta_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_trifecta_scsi[ci->controller_type_unit], trifecta_dma_read, trifecta_dma_write, set_irq2_trifecta, 0);
}

void ematrix_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_ematrix530_scsi, ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_ematrix530_scsi, fake_dma_read_ematrix, fake_dma_write_ematrix, set_irq2, 0);
	esp_dma_enable(ncr_ematrix530_scsi->devobject.lsistate, 1);
}

void multievolution_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_multievolution_scsi, ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_multievolution_scsi, fake_dma_read, fake_dma_write, set_irq2, 0);
	esp_dma_enable(ncr_multievolution_scsi->devobject.lsistate, 1);
}

void golemfast_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_golemfast_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_golemfast_scsi[ci->controller_type_unit], fake_dma_read, fake_dma_write, set_irq2_golemfast, 0);
	esp_dma_enable(ncr_golemfast_scsi[ci->controller_type_unit]->devobject.lsistate, 1);
}

void scram5394_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_scram5394_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_scram5394_scsi[ci->controller_type_unit], fake_dma_read, fake_dma_write, set_irq2, 0);
	esp_dma_enable(ncr_scram5394_scsi[ci->controller_type_unit]->devobject.lsistate, 1);
}

void rapidfire_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_rapidfire_scsi[ci->controller_type_unit], ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_rapidfire_scsi[ci->controller_type_unit], fake_dma_read, fake_dma_write, set_irq2, 2);
	esp_dma_enable(ncr_rapidfire_scsi[ci->controller_type_unit]->devobject.lsistate, 1);
}

void typhoon2scsi_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_typhoon2_scsi, ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_typhoon2_scsi, fake_dma_read_ematrix, fake_dma_write_ematrix, set_irq2, 0);
	esp_dma_enable(ncr_typhoon2_scsi->devobject.lsistate, 1);
}

void squirrel_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_squirrel_scsi, ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_squirrel_scsi, fake2_dma_read, fake2_dma_write, set_irq2, 0);
	esp_dma_enable(ncr_squirrel_scsi->devobject.lsistate, 1);
}

void mtecmastercard_add_scsi_unit(int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	ncr9x_add_scsi_unit(&ncr_mtec_mastercard, ch, ci, rc);
	ncr9x_esp_scsi_init(ncr_mtec_mastercard, fake_dma_read_ematrix, fake_dma_write_ematrix, set_irq2, 0);
	esp_dma_enable(ncr_mtec_mastercard->devobject.lsistate, 1);
}
#endif
