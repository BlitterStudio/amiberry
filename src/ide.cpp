/*
* UAE - The Un*x Amiga Emulator
*
* IDE HD/ATAPI CD emulation
*
* (c) 2006 - 2015 Toni Wilen
*/

#define IDE_LOG 0

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "blkdev.h"
#include "filesys.h"
#include "gui.h"
#include "uae.h"
#include "memory.h"
#include "newcpu.h"
#include "threaddep/thread.h"
#include "debug.h"
#include "savestate.h"
#include "scsi.h"
#include "ide.h"
#include "ini.h"

/* STATUS bits */
#define IDE_STATUS_ERR 0x01		// 0
#define IDE_STATUS_IDX 0x02		// 1
#define IDE_STATUS_DRQ 0x08		// 3
#define IDE_STATUS_DSC 0x10		// 4
#define IDE_STATUS_DRDY 0x40	// 6
#define IDE_STATUS_BSY 0x80		// 7
#define ATAPI_STATUS_CHK IDE_STATUS_ERR
/* ERROR bits */
#define IDE_ERR_UNC 0x40
#define IDE_ERR_MC 0x20
#define IDE_ERR_IDNF 0x10
#define IDE_ERR_MCR 0x08
#define IDE_ERR_ABRT 0x04
#define IDE_ERR_NM 0x02
#define ATAPI_ERR_EOM 0x02
#define ATAPI_ERR_ILI 0x01
/* ATAPI interrupt reason (Sector Count) */
#define ATAPI_IO 0x02
#define ATAPI_CD 0x01

#define ATAPI_MAX_TRANSFER 32768

// 1 = need soft reset before ATAPI signature appears
#define ATAPIWEIRD 0

void ata_parse_identity(uae_u8 *out, struct uaedev_config_info *uci, bool *lba, bool *lba48, int *max_multiple)
{
	struct uaedev_config_info uci2;
	uae_u16 v;

	memcpy(&uci2, uci, sizeof(struct uaedev_config_info));

	*lba = false;
	*lba48 = false;
	*max_multiple = 0;

	uci->blocksize = 512;

	uci->pcyls = (out[1 * 2 + 0] << 8) | (out[1 * 2 + 1] << 0);
	uci->pheads = (out[3 * 2 + 0] << 8) | (out[3 * 2 + 1] << 0);
	uci->psecs = (out[6 * 2 + 0] << 8) | (out[6 * 2 + 1] << 0);
	
	if (!uci->pcyls || !uci->pheads || !uci->psecs) {
		uci->pcyls = uci2.pcyls;
		uci->pheads = uci2.pheads;
		uci->psecs = uci2.psecs;
	}

	v = (out[47 * 2 + 0] << 8) | (out[47 * 2 + 1] << 0);
	if (v & 255) { // multiple mode?
		*max_multiple = v & 255;
	}

	v = (out[53 * 2 + 0] << 8) | (out[53 * 2 + 1] << 0);
	if (v & 1) { // 54-58 valid?
		uci->pcyls = (out[54 * 2 + 0] << 8) | (out[54 * 2 + 1] << 0);
		uci->pheads = (out[55 * 2 + 0] << 8) | (out[55 * 2 + 1] << 0);
		uci->psecs = (out[56 * 2 + 0] << 8) | (out[56 * 2 + 1] << 0);
		uci->max_lba = (out[57 * 2 + 0] << 24) | (out[57 * 2 + 1] << 16) | (out[58 * 2 + 0] << 8) | (out[58 * 2 + 1] << 0);
	}
	v = (out[49 * 2 + 0] << 8) | (out[49 * 2 + 1] << 0);
	if (v & (1 << 9)) { // LBA supported?
		uci->max_lba = (out[60 * 2 + 0] << 24) | (out[60 * 2 + 1] << 16) | (out[61 * 2 + 0] << 8) | (out[61 * 2 + 1] << 0);
		*lba = true;
	}
	v = (out[83 * 2 + 0] << 8) | (out[83 * 2 + 1] << 0);
	if ((v & 0xc000) == 0x4000 && (v & (1 << 10)) && (*lba)) { // LBA48 supported?
		*lba48 = true;
		uci->max_lba = (out[100 * 2 + 0] << 24) | (out[100 * 2 + 1] << 16) | (out[101 * 2 + 0] << 8) | (out[101 * 2 + 1] << 0);
		uci->max_lba <<= 32;
		uci->max_lba |= (out[102 * 2 + 0] << 24) | (out[102 * 2 + 1] << 16) | (out[103 * 2 + 0] << 8) | (out[103 * 2 + 1] << 0);	
	}
}

bool ata_get_identity(struct ini_data *ini, uae_u8 *out, bool overwrite)
{
	if (!out)
		return false;
	if (overwrite) {
		uae_u8 *out2;
		int len;
		if (!ini_getdata(ini, _T("IDENTITY"), _T("DATA"), &out2, &len))
			return false;
		if (len == 512) {
			memcpy(out, out2, 512);
		}
		xfree(out2);
	}
	int v;
	if (ini_getval(ini, _T("IDENTITY"), _T("multiple_mode"), &v)) {
		if (v) {
			out[59 * 2 + 1] |= 1;
			out[47 * 2 + 0] = 0;
			out[47 * 2 + 1] = v;
		} else {
			out[59 * 2 + 1] &= ~1;
		}
	}

	return true;
}

uae_u16 adide_decode_word(uae_u16 w)
{
	uae_u16 o = 0;

	if (w & 0x8000)
		o |= 0x0001;
	if (w & 0x0001)
		o |= 0x0002;

	if (w & 0x4000)
		o |= 0x0004;
	if (w & 0x0002)
		o |= 0x0008;

	if (w & 0x2000)
		o |= 0x0010;
	if (w & 0x0004)
		o |= 0x0020;

	if (w & 0x1000)
		o |= 0x0040;
	if (w & 0x0008)
		o |= 0x0080;

	if (w & 0x0800)
		o |= 0x0100;
	if (w & 0x0010)
		o |= 0x0200;

	if (w & 0x0400)
		o |= 0x0400;
	if (w & 0x0020)
		o |= 0x0800;

	if (w & 0x0200)
		o |= 0x1000;
	if (w & 0x0040)
		o |= 0x2000;

	if (w & 0x0100)
		o |= 0x4000;
	if (w & 0x0080)
		o |= 0x8000;

	return o;
}

uae_u16 adide_encode_word(uae_u16 w)
{
	uae_u16 o = 0;

	if (w & 0x0001)
		o |= 0x8000;
	if (w & 0x0002)
		o |= 0x0001;

	if (w & 0x0004)
		o |= 0x4000;
	if (w & 0x0008)
		o |= 0x0002;

	if (w & 0x0010)
		o |= 0x2000;
	if (w & 0x0020)
		o |= 0x0004;

	if (w & 0x0040)
		o |= 0x1000;
	if (w & 0x0080)
		o |= 0x0008;

	if (w & 0x0100)
		o |= 0x0800;
	if (w & 0x0200)
		o |= 0x0010;

	if (w & 0x0400)
		o |= 0x0400;
	if (w & 0x0800)
		o |= 0x0020;

	if (w & 0x1000)
		o |= 0x0200;
	if (w & 0x2000)
		o |= 0x0040;

	if (w & 0x4000)
		o |= 0x0100;
	if (w & 0x8000)
		o |= 0x0080;

	return o;
}

static void ide_grow_buffer(struct ide_hdf *ide, int newsize)
{
	if (ide->secbuf_size >= newsize)
		return;
	uae_u8 *oldbuf = ide->secbuf;
	int oldsize = ide->secbuf_size;
	ide->secbuf_size = newsize + 16384;
	if (oldsize)
		write_log(_T("IDE%d buffer %d -> %d\n"), ide->num, oldsize, ide->secbuf_size);
	ide->secbuf = xmalloc(uae_u8, ide->secbuf_size);
	memcpy(ide->secbuf, oldbuf, oldsize);
	xfree(oldbuf);
}

static void sl(uae_u8 *d, int o)
{
	o *= 2;
	uae_u16 t = (d[o + 0] << 8) | d[o + 1];
	d[o + 0] = d[o + 2];
	d[o + 1] = d[o + 3];
	d[o + 2] = t >> 8;
	d[o + 3] = (uae_u8)t;
}
static void ql(uae_u8 *d, int o)
{
	sl(d, o + 1);
	o *= 2;
	uae_u16 t = (d[o + 0] << 8) | d[o + 1];
	d[o + 0] = d[o + 6];
	d[o + 1] = d[o + 7];
	d[o + 6] = t >> 8;
	d[o + 7] = (uae_u8)t;
}

void ata_byteswapidentity(uae_u8 *d)
{
	for (int i = 0; i < 512; i += 2)
	{
		uae_u8 t = d[i + 0];
		d[i + 0] = d[i + 1];
		d[i + 1] = t;
	}
	sl(d, 7);
	sl(d, 57);
	sl(d, 60);
	sl(d, 98);
	sl(d, 117);
	sl(d, 210);
	sl(d, 212);
	sl(d, 215);
	ql(d, 100);
	ql(d, 230);
}

static void pl(struct ide_hdf *ide, int offset, uae_u32 l)
{
	if (ide->byteswap) {
		l = ((l >> 24) & 0x000000ff) | ((l >> 8) & 0x0000ff00) | ((l << 8) & 0x00ff0000) | ((l << 24) & 0xff000000);
	}
	ide->secbuf[offset * 2 + 0] = l;
	ide->secbuf[offset * 2 + 1] = l >> 8;
	ide->secbuf[offset * 2 + 2] = l >> 16;
	ide->secbuf[offset * 2 + 3] = l >> 24;
}

static void pq(struct ide_hdf *ide, int offset, uae_u64 q)
{
	if (ide->byteswap) {
		pl(ide, offset + 0, q >> 32);
		pl(ide, offset + 2, q >>  0);
	} else {
		pl(ide, offset + 0, q >>  0);
		pl(ide, offset + 2, q >> 32);
	}
}

static void pw (struct ide_hdf *ide, int offset, uae_u16 w)
{
	if (ide->byteswap) {
		w = (w >> 8) | (w << 8);
	}
	ide->secbuf[offset * 2 + 0] = (uae_u8)w;
	ide->secbuf[offset * 2 + 1] = w >> 8;
}

static void pwand (struct ide_hdf *ide, int offset, uae_u16 w)
{
	if (ide->byteswap) {
		w = (w >> 8) | (w << 8);
	}
	ide->secbuf[offset * 2 + 0] &= ~((uae_u8)w);
	ide->secbuf[offset * 2 + 1] &= ~(w >> 8);
}

static void pwor (struct ide_hdf *ide, int offset, uae_u16 w)
{
	if (ide->byteswap) {
		w = (w >> 8) | (w << 8);
	}
	ide->secbuf[offset * 2 + 0] |= (uae_u8)w;
	ide->secbuf[offset * 2 + 1] |= w >> 8;
}

static void ps (struct ide_hdf *ide, int offset, const TCHAR *src, int max)
{
	int i, len;
	char *s;

	s = ua(src);
	len = uaestrlen(s);
	for (i = 0; i < max; i += 2) {
		char c1 = ' ';
		if (i < len)
			c1 = s[i];
		char c2 = ' ';
		if (i + 1 < len)
			c2 = s[i + 1];
		uae_u16 w = (c1 << 0) | (c2 << 8);
		if (ide->byteswap) {
			w = (w >> 8) | (w << 8);
		}
		ide->secbuf[offset * 2 + 0] = w >> 8;
		ide->secbuf[offset * 2 + 1] = w >> 0;
		offset++;
	}
	xfree (s);
}

bool ide_isdrive (struct ide_hdf *ide)
{
	return ide && (ide->hdhfd.size != 0 || ide->atapi);
}

static void ide_interrupt (struct ide_hdf *ide)
{
	ide->regs.ide_status |= IDE_STATUS_BSY;
	ide->regs.ide_status &= ~IDE_STATUS_DRQ;
	ide->irq_delay = 2;
}

static void ide_fast_interrupt (struct ide_hdf *ide)
{
	ide->regs.ide_status |= IDE_STATUS_BSY;
	ide->regs.ide_status &= ~IDE_STATUS_DRQ;
	ide->irq_delay = 1;
}

static bool ide_interrupt_do (struct ide_hdf *ide)
{
	uae_u8 os = ide->regs.ide_status;
	ide->regs.ide_status &= ~IDE_STATUS_DRQ;
	if (ide->intdrq)
		ide->regs.ide_status |= IDE_STATUS_DRQ;
	ide->regs.ide_status &= ~IDE_STATUS_BSY;
	if (IDE_LOG > 1)
		write_log (_T("IDE INT %02X -> %02X\n"), os, ide->regs.ide_status);
	ide->intdrq = false;
	ide->irq_delay = 0;
	if ((ide->regs.ide_devcon & 2) || ide->irq_inhibit)
		return false;
	ide->irq_new = true;
	ide->irq = 1;
	return true;
}

bool ide_drq_check(struct ide_hdf *idep)
{
	for (int i = 0; idep && i < 2; i++) {
		struct ide_hdf *ide = i == 0 ? idep : idep->pair;
		if (ide) {
			if (ide->regs.ide_status & IDE_STATUS_DRQ)
				return true;
		}
	}
	return false;
}

bool ide_irq_check(struct ide_hdf *idep, bool edge_triggered)
{
	for (int i = 0; idep && i < 2; i++) {
		struct ide_hdf *ide = i == 0 ? idep : idep->pair;
		if (ide->irq) {
			if (edge_triggered) {
				if (ide->irq_new) {
					ide->irq_new = false;
					return true;
				}
				continue;
			}
			return true;
		}
	}
	return false;
}

bool ide_interrupt_hsync(struct ide_hdf *idep)
{
	bool irq = false;
	for (int i = 0; idep && i < 2; i++) {
		struct ide_hdf *ide = i == 0 ? idep : idep->pair;
		if (ide) {
			if (ide->irq_delay > 0) {
				ide->irq_delay--;
				if (ide->irq_delay == 0) {
					ide_interrupt_do (ide);
				}
			}
			if (ide->irq && !(ide->regs.ide_devcon & 2))
				irq = true;
		}
	}
	return irq;
}

static void ide_fail_err (struct ide_hdf *ide, uae_u8 err)
{
	ide->regs.ide_error |= err;
	if (ide->ide_drv == 1 && !ide_isdrive (ide->pair)) {
		ide->pair->regs.ide_status |= IDE_STATUS_ERR;
	}
	ide->regs.ide_status |= IDE_STATUS_ERR;
	ide_interrupt (ide);
}

static void ide_fail (struct ide_hdf *ide)
{
	ide_fail_err (ide, IDE_ERR_ABRT);
}

static void ide_data_ready (struct ide_hdf *ide)
{
	memset (ide->secbuf, 0, ide->blocksize);
	ide->data_offset = 0;
	ide->data_size = ide->blocksize;
	ide->buffer_offset = 0;
	ide->data_multi = 1;
	ide->intdrq = true;
	ide_interrupt (ide);
}

static void ide_recalibrate (struct ide_hdf *ide)
{
	write_log (_T("IDE%d recalibrate\n"), ide->num);
	ide->regs.ide_sector = 0;
	ide->regs.ide_lcyl = ide->regs.ide_hcyl = 0;
	ide_interrupt (ide);
}

static void ide_identity_buffer(struct ide_hdf *ide)
{
	TCHAR tmp[100];
	bool atapi = ide->atapi;
	int device_type = ide->atapi_device_type;
	bool cf = ide->media_type > 0;
	bool real = false;
	int v;

	memset(ide->secbuf, 0, 512);
	if (!device_type)
		device_type = 5; // CD

	if (ata_get_identity(ide->hdhfd.hfd.geometry, ide->secbuf, true)) {

		if (ini_getval(ide->hdhfd.hfd.geometry, _T("IDENTITY"), _T("atapi"), &v)) {
			ide->atapi = v != 0;
			if (ide->atapi) {
				ide->atapi_device_type = 0;
				ini_getval(ide->hdhfd.hfd.geometry, _T("IDENTITY"), _T("atapi_type"), &ide->atapi_device_type);
				if (!ide->atapi_device_type)
					ide->atapi_device_type = 5;
			}
		}
		if (!ide->byteswap) {
			ata_byteswapidentity(ide->secbuf);
		}

	} else if (ide->hdhfd.hfd.ci.loadidentity && (ide->hdhfd.hfd.identity[0] || ide->hdhfd.hfd.identity[1])) {

		memcpy(ide->secbuf, ide->hdhfd.hfd.identity, 512);
		if (!ide->byteswap) {
			ata_byteswapidentity(ide->secbuf);
		}
		real = true;

	} else {

		pw(ide, 0, atapi ? 0x80c0 | (device_type << 8) : (cf ? 0x848a : (1 << 6)));
		pw(ide, 1, ide->hdhfd.cyls_def);
		pw(ide, 2, 0xc837);
		pw(ide, 3, ide->hdhfd.heads_def);
		pw(ide, 4, ide->blocksize * ide->hdhfd.secspertrack_def);
		pw(ide, 5, ide->blocksize);
		pw(ide, 6, ide->hdhfd.secspertrack_def);
		ps(ide, 10, _T("68000"), 20); /* serial */
		pw(ide, 20, 3);
		pw(ide, 21, ide->blocksize);
		pw(ide, 22, 4);
		ps(ide, 23, _T("0.7"), 8); /* firmware revision */
		if (ide->atapi)
			_tcscpy (tmp, _T("UAE-ATAPI"));
		else
			_sntprintf (tmp, sizeof tmp, _T("UAE-IDE %s"), ide->hdhfd.hfd.product_id);
		ps(ide, 27, tmp, 40); /* model */
		pw(ide, 47, ide->max_multiple_mode ? (0x8000 | (ide->max_multiple_mode >> (ide->blocksize / 512 - 1))) : 0); /* max sectors in multiple mode */
		pw(ide, 48, 1);
		pw(ide, 49, (ide->lba ? (1 << 9) : 0) | (1 << 8)); /* LBA and DMA supported */
		pw(ide, 51, 0x200); /* PIO cycles */
		pw(ide, 52, 0x200); /* DMA cycles */
		pw(ide, 53, 1 | (ide->lba ? 2 | 4 : 0)); // b0 = 54-58 valid b1 = 64-70 valid b2 = 88 valid
		pw(ide, 54, ide->hdhfd.cyls);
		pw(ide, 55, ide->hdhfd.heads);
		pw(ide, 56, ide->hdhfd.secspertrack);
		uae_u64 totalsecs = (uae_u64)ide->hdhfd.cyls * ide->hdhfd.heads * ide->hdhfd.secspertrack;
		pl(ide, 57, (uae_u32)totalsecs);
		pw(ide, 59, ide->max_multiple_mode ? (0x100 | ide->max_multiple_mode >> (ide->blocksize / 512 - 1)) : 0); /* Multiple mode supported */
		pw(ide, 62, 0x0f);
		pw(ide, 63, 0x0f);
		if (ide->lba) {
			totalsecs = ide->blocksize ? ide->hdhfd.size / ide->blocksize : 0;
			if (totalsecs > 0x0fffffff)
				totalsecs = 0x0fffffff;
			pl(ide, 60, (uae_u32)totalsecs);
			if (ide->ata_level > 0) {
				pw(ide, 64, ide->ata_level ? 0x03 : 0x00); /* PIO3 and PIO4 */
				pw(ide, 65, 120); /* MDMA2 supported */
				pw(ide, 66, 120);
				pw(ide, 67, 120);
				pw(ide, 68, 120);
				pw(ide, 80, (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6)); /* ATA-1 to ATA-6 */
				pw(ide, 81, 0x1c); /* ATA revision */
				pw(ide, 82, (1 << 14) | (atapi ? 0x10 | 4 : 0)); /* NOP, ATAPI: PACKET and Removable media features supported */
				pw(ide, 83, (1 << 14) | (1 << 13) | (1 << 12) | (ide->lba48 ? (1 << 10) : 0)); /* cache flushes, LBA 48 supported */
				pw(ide, 84, 1 << 14);
				pw(ide, 85, 1 << 14);
				pw(ide, 86, (1 << 14) | (1 << 13) | (1 << 12) | (ide->lba48 ? (1 << 10) : 0)); /* cache flushes, LBA 48 enabled */
				pw(ide, 87, 1 << 14);
				pw(ide, 88, (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0)); /* UDMA modes */
				pw(ide, 93, (1 << 14) | (1 << 13) | (1 << 0));
				if (ide->lba48) {
					totalsecs = ide->hdhfd.size / ide->blocksize;
					pq(ide, 100, totalsecs);
				}
			}
		}
		ata_get_identity(ide->hdhfd.hfd.geometry, ide->secbuf, false);
	}

	if (!real) {
		v = ide->multiple_mode;
		pwor(ide, 59, v > 0 ? 0x100 : 0);
	}
	if (!atapi && cf) {
		pw(ide, 0, 0x848a);
	} else if (!atapi && !cf) {
		pwand(ide, 0, 0x8000);
	}
}

static void ide_identify_drive (struct ide_hdf *ide)
{
	if (!ide_isdrive (ide) || ide->ata_level < 0) {
		ide_fail (ide);
		return;
	}
	if (IDE_LOG > 0)
		write_log (_T("IDE%d identify drive\n"), ide->num);
	ide_data_ready (ide);
	ide->direction = 0;
	ide_identity_buffer(ide);
	if (ide->adide) {
		for (int i = 0; i < 256; i++) {
			uae_u16 w = (ide->secbuf[i * 2 + 0] << 8) | (ide->secbuf[i * 2 + 1] << 0);
			uae_u16 we = adide_decode_word(w);
			ide->secbuf[i * 2 + 0] = we >> 8;
			ide->secbuf[i * 2 + 1] = we >> 0;
		}
	}
}

static void set_signature (struct ide_hdf *ide, bool hard)
{
	if (ide->atapi) {
		ide->regs.ide_sector = 1;
		ide->regs.ide_nsector = 1;
		if (hard && ATAPIWEIRD) {
			ide->regs.ide_lcyl = 0;
			ide->regs.ide_hcyl = 0;
		} else {
			ide->regs.ide_lcyl = 0x14;
			ide->regs.ide_hcyl = 0xeb;
		}
		ide->regs.ide_status = 0;
		ide->atapi_drdy = false;
	} else {
		ide->regs.ide_nsector = 1;
		ide->regs.ide_sector = 1;
		ide->regs.ide_lcyl = 0;
		ide->regs.ide_hcyl = 0;
		ide->regs.ide_status = 0;
	}
	ide->regs.ide_error = 0x01; // device ok
	ide->packet_state = 0;
}

static void reset_device (struct ide_hdf *ide, bool both, bool hard)
{
	set_signature (ide, hard);
	if (both)
		set_signature (ide->pair, hard);
}

void ide_reset_device(struct ide_hdf *ide)
{
	reset_device(ide, true, false);
}

static void ide_execute_drive_diagnostics (struct ide_hdf *ide, bool irq)
{
	reset_device (ide, irq, false);
	if (irq)
		ide_interrupt (ide);
	else
		ide->regs.ide_status &= ~IDE_STATUS_BSY;
}

static void ide_initialize_drive_parameters (struct ide_hdf *ide)
{
	if (ide->hdhfd.size) {
		ide->hdhfd.secspertrack = ide->regs.ide_nsector == 0 ? 256 : ide->regs.ide_nsector;
		ide->hdhfd.heads = (ide->regs.ide_select & 15) + 1;
		if (ide->hdhfd.hfd.ci.pcyls)
			ide->hdhfd.cyls = ide->hdhfd.hfd.ci.pcyls;
		else
			ide->hdhfd.cyls = (int)((ide->hdhfd.size / ide->blocksize) / ((uae_u64)ide->hdhfd.secspertrack * ide->hdhfd.heads));
		if (ide->hdhfd.heads * ide->hdhfd.cyls * ide->hdhfd.secspertrack > 16515072 || ide->lba48) {
			if (ide->hdhfd.hfd.ci.pcyls)
				ide->hdhfd.cyls = ide->hdhfd.hfd.ci.pcyls;
			else
				ide->hdhfd.cyls = ide->hdhfd.cyls_def;
			ide->hdhfd.heads = ide->hdhfd.heads_def;
			ide->hdhfd.secspertrack = ide->hdhfd.secspertrack_def;
		}
	} else {
		ide->regs.ide_error |= IDE_ERR_ABRT;
		ide->regs.ide_status |= IDE_STATUS_ERR;
	}
	write_log (_T("IDE%d initialize drive parameters, CYL=%d,SPT=%d,HEAD=%d\n"),
		ide->num, ide->hdhfd.cyls, ide->hdhfd.secspertrack, ide->hdhfd.heads);
	ide_interrupt (ide);
}

static void ide_set_multiple_mode (struct ide_hdf *ide)
{
	if (ide->ata_level < 0) {
		ide_fail(ide);
		return;
	}

	write_log (_T("IDE%d drive multiple mode = %d\n"), ide->num, ide->regs.ide_nsector);
	if (ide->regs.ide_nsector > (ide->max_multiple_mode >> (ide->blocksize / 512 - 1))) {
		ide_fail(ide);
	} else {
		ide->multiple_mode = ide->regs.ide_nsector;
	}
	ide_interrupt(ide);
}

static void ide_set_features (struct ide_hdf *ide)
{
	if (ide->ata_level < 0) {
		ide_fail(ide);
		return;
	}

	int type = ide->regs.ide_nsector >> 3;
	int mode = ide->regs.ide_nsector & 7;

	write_log (_T("IDE%d set features %02X (%02X)\n"), ide->num, ide->regs.ide_feat, ide->regs.ide_nsector);
	switch (ide->regs.ide_feat)
	{
		// 8-bit mode
		case 1:
		ide->mode_8bit = true;
		ide_interrupt(ide);
		break;
		case 0x81:
		ide->mode_8bit = false;
		ide_interrupt(ide);
		break;
		// write cache
		case 2:
		case 0x82:
		ide_interrupt(ide);
		break;
		default:
		ide_fail (ide);
		break;
	}
}

static void get_lbachs (struct ide_hdf *ide, uae_u64 *lbap, unsigned int *cyl, unsigned int *head, unsigned int *sec)
{
	if (ide->lba48 && ide->lba48cmd && (ide->regs.ide_select & 0x40)) {
		uae_u64 lba;
		lba = (ide->regs.ide_hcyl << 16) | (ide->regs.ide_lcyl << 8) | ide->regs.ide_sector;
		lba |= ((uae_u64)(((ide->regs.ide_hcyl2 << 16) | (ide->regs.ide_lcyl2 << 8) | ide->regs.ide_sector2))) << 24;
		*lbap = lba;
	} else {
		if ((ide->regs.ide_select & 0x40) && ide->lba) {
			*lbap = ((ide->regs.ide_select & 15) << 24) | (ide->regs.ide_hcyl << 16) | (ide->regs.ide_lcyl << 8) | ide->regs.ide_sector;
		} else {
			*cyl = (ide->regs.ide_hcyl << 8) | ide->regs.ide_lcyl;
			*head = ide->regs.ide_select & 15;
			*sec = ide->regs.ide_sector;
			*lbap = (((uae_u64)(*cyl) * ide->hdhfd.heads + (*head)) * ide->hdhfd.secspertrack) + (*sec) - 1;
		}
	}
}

static int get_nsec (struct ide_hdf *ide)
{
	if (ide->lba48 && ide->lba48cmd)
		return (ide->regs.ide_nsector == 0 && ide->regs.ide_nsector2 == 0) ? 65536 : (ide->regs.ide_nsector2 * 256 + ide->regs.ide_nsector);
	else
		return ide->regs.ide_nsector == 0 ? 256 : ide->regs.ide_nsector;
}
static int dec_nsec (struct ide_hdf *ide, int v)
{
	if (ide->lba48 && ide->lba48cmd) {
		uae_u16 nsec;
		nsec = (ide->regs.ide_nsector2 << 8) | ide->regs.ide_nsector;
		nsec -= v;
		ide->regs.ide_nsector2 = nsec >> 8;
		ide->regs.ide_nsector = nsec & 0xff;
		return (ide->regs.ide_nsector2 << 8) | ide->regs.ide_nsector;
	} else {
		ide->regs.ide_nsector -= v;
		return ide->regs.ide_nsector;
	}
}

static void put_lbachs (struct ide_hdf *ide, uae_u64 lba, unsigned int cyl, unsigned int head, unsigned int sec, unsigned int inc)
{
	if (ide->lba48 && ide->lba48cmd) {
		lba += inc;
		ide->regs.ide_hcyl = (lba >> 16) & 0xff;
		ide->regs.ide_lcyl = (lba >> 8) & 0xff;
		ide->regs.ide_sector = lba & 0xff;
		lba >>= 24;
		ide->regs.ide_hcyl2 = (lba >> 16) & 0xff;
		ide->regs.ide_lcyl2 = (lba >> 8) & 0xff;
		ide->regs.ide_sector2 = lba & 0xff;
	} else {
		if ((ide->regs.ide_select & 0x40) && ide->lba) {
			lba += inc;
			ide->regs.ide_select &= ~15;
			ide->regs.ide_select |= (lba >> 24) & 15;
			ide->regs.ide_hcyl = (lba >> 16) & 0xff;
			ide->regs.ide_lcyl = (lba >> 8) & 0xff;
			ide->regs.ide_sector = lba & 0xff;
		} else {
			sec += inc;
			while (sec >= ide->hdhfd.secspertrack) {
				sec -= ide->hdhfd.secspertrack;
				head++;
				if (head >= ide->hdhfd.heads) {
					head -= ide->hdhfd.heads;
					cyl++;
				}
			}
			ide->regs.ide_select &= ~15;
			ide->regs.ide_select |= head;
			ide->regs.ide_sector = sec;
			ide->regs.ide_hcyl = cyl >> 8;
			ide->regs.ide_lcyl = (uae_u8)cyl;
		}
	}
}

static void check_maxtransfer (struct ide_hdf *ide, int state)
{
	if (state == 1) {
		// transfer was started
		if (ide->maxtransferstate < 2 && ide->regs.ide_nsector == 0) {
			ide->maxtransferstate = 1;
		} else if (ide->maxtransferstate == 2) {
			// second transfer was started (part of split)
			write_log (_T("IDE maxtransfer check detected split >256 block transfer\n"));
			ide->maxtransferstate = 0;
		} else {
			ide->maxtransferstate = 0;
		}
	} else if (state == 2) {
		// address was read
		if (ide->maxtransferstate == 1)
			ide->maxtransferstate++;
		else
			ide->maxtransferstate = 0;
	}
}

static void setdrq (struct ide_hdf *ide)
{
	ide->regs.ide_status |= IDE_STATUS_DRQ;
	ide->regs.ide_status &= ~IDE_STATUS_BSY;
}
static void setbsy (struct ide_hdf *ide)
{
	ide->regs.ide_status |= IDE_STATUS_BSY;
	ide->regs.ide_status &= ~IDE_STATUS_DRQ;
}

static void process_rw_command (struct ide_hdf *ide)
{
	setbsy (ide);
	write_comm_pipe_u32 (&ide->its->requests, ide->num, 1);
}
static void process_packet_command (struct ide_hdf *ide)
{
	setbsy (ide);
	write_comm_pipe_u32 (&ide->its->requests, ide->num | 0x8000, 1);
}

static void atapi_data_done (struct ide_hdf *ide)
{
	ide->regs.ide_nsector = ATAPI_IO | ATAPI_CD;
	ide->regs.ide_status = IDE_STATUS_DRDY;
	ide->data_size = 0;
	ide->packet_data_offset = 0;
	ide->data_offset = 0;
}

static bool atapi_set_size (struct ide_hdf *ide)
{
	int size;
	size = ide->data_size;
	ide->data_offset = 0;
	if (!size) {
		ide->packet_state = 0;
		ide->packet_transfer_size = 0;
		return false;
	}
	if (ide->packet_state == 2) {
		if (size > ide->packet_data_size)
			size = ide->packet_data_size;
		if (size > ATAPI_MAX_TRANSFER)
			size = ATAPI_MAX_TRANSFER;
		ide->packet_transfer_size = size & ~1;
		ide->regs.ide_lcyl = size & 0xff;
		ide->regs.ide_hcyl = size >> 8;
	} else {
		ide->packet_transfer_size = 12;
	}
	if (IDE_LOG > 1)
		write_log (_T("ATAPI data transfer %d/%d bytes\n"), ide->packet_transfer_size, ide->data_size);
	return true;
}

static void atapi_packet (struct ide_hdf *ide)
{
	ide->packet_data_offset = 0;
	ide->packet_data_size = (ide->regs.ide_hcyl << 8) | ide->regs.ide_lcyl;
	if (ide->packet_data_size == 65535)
		ide->packet_data_size = 65534;
	ide->data_size = 12;
	if (IDE_LOG > 0)
		write_log (_T("ATAPI packet command. Data size = %d\n"), ide->packet_data_size);
	ide->packet_state = 1;
	ide->data_multi = 1;
	ide->data_offset = 0;
	ide->regs.ide_nsector = ATAPI_CD;
	ide->regs.ide_error = 0;
	if (atapi_set_size (ide))
		setdrq (ide);
}

static void do_packet_command (struct ide_hdf *ide)
{
	memcpy (ide->scsi->cmd, ide->secbuf, 12);
	ide->scsi->cmd_len = 12;
	if (IDE_LOG > 0) {
		uae_u8 *c = ide->scsi->cmd;
		write_log (_T("ATASCSI %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n"),
			c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11]);
	}
	ide->direction = 0;
	scsi_emulate_analyze (ide->scsi);
	if (ide->scsi->direction <= 0) {
		// data in
		scsi_emulate_cmd (ide->scsi);
		ide->data_size = ide->scsi->data_len;
		ide->regs.ide_status = 0;
		if (ide->scsi->status) {
			// error
			ide->regs.ide_error = (ide->scsi->sense[2] << 4) | 4; // ABRT
			atapi_data_done (ide);
			ide->regs.ide_status |= ATAPI_STATUS_CHK;
			atapi_set_size (ide);
			return;
		} else if (ide->scsi->data_len) {
			// data in
			ide_grow_buffer(ide, ide->scsi->data_len);
			memcpy (ide->secbuf, ide->scsi->buffer, ide->scsi->data_len);
			ide->regs.ide_nsector = ATAPI_IO;
		} else {
			// no data
			atapi_data_done (ide);
		}
	} else {
		// data out
		ide->direction = 1;
		ide->regs.ide_nsector = 0;
		ide->data_size = ide->scsi->data_len;
	}
	ide->packet_state = 2; // data phase
	if (atapi_set_size (ide))
		ide->intdrq = true;
}

static void do_process_packet_command (struct ide_hdf *ide)
{
	if (ide->packet_state == 1) {
		do_packet_command (ide);
	} else {
		ide->packet_data_offset += ide->packet_transfer_size;
		if (!ide->direction) {
			// data still remaining, next transfer
			if (atapi_set_size (ide))
				ide->intdrq = true;
		} else {
			if (atapi_set_size (ide)) {
				ide->intdrq = true;
			} else {
				if (IDE_LOG > 1)
					write_log(_T("IDE%d ATAPI write finished, %d bytes\n"), ide->num, ide->packet_data_size);
				memcpy (ide->scsi->buffer, ide->secbuf, ide->packet_data_size);
				ide->scsi->data_len = ide->packet_data_size;
				scsi_emulate_cmd (ide->scsi);
			}
		}
	}
	ide_fast_interrupt (ide);
}

static const TCHAR *getidemode(struct ide_hdf *ide)
{
	return ide->lba48 ? (ide->lba48cmd ? _T("lba48") : _T("lba48*")) : ((ide->regs.ide_select & 0x40) ? _T("lba") : _T("chs"));
}

static void do_process_rw_command (struct ide_hdf *ide)
{
	unsigned int cyl, head, sec, nsec, nsec_total;
	uae_u64 lba;
	bool last = true;

	ide->data_offset = 0;

	if (ide->direction > 1) {
		// FORMAT TRACK?
		// Don't write anything
		goto end;
	}

	nsec = get_nsec (ide);
	get_lbachs (ide, &lba, &cyl, &head, &sec);
	if (IDE_LOG > 1)
		write_log(_T("IDE%d off=%d, nsec=%d (%d) %s\n"), ide->num, (uae_u32)lba, nsec, ide->multiple_mode, getidemode(ide));
	if (nsec > ide->max_lba - lba) {
		nsec = (unsigned int)(ide->max_lba - lba);
		if (IDE_LOG > 1)
			write_log (_T("IDE%d nsec changed to %d\n"), ide->num, nsec);
	}
	if (nsec <= 0) {
		ide_data_ready (ide);
		ide_fail_err (ide, IDE_ERR_IDNF);
		return;
	}
	nsec_total = nsec;
	ide_grow_buffer(ide, nsec_total * ide->blocksize);

	if (nsec > ide->data_multi)
		nsec = ide->data_multi;

	if (ide->buffer_offset == 0) {
		// store initial lba and number of sectors to transfer
		ide->start_lba = lba;
		ide->start_nsec = nsec_total;
	}

	if (ide->direction) {
		if (IDE_LOG > 1)
			write_log (_T("IDE%d write, %d/%d bytes, buffer offset %d\n"), ide->num, nsec * ide->blocksize, nsec_total * ide->blocksize, ide->buffer_offset);
	} else {
		if (ide->buffer_offset == 0) {
			hdf_read(&ide->hdhfd.hfd, ide->secbuf, ide->start_lba * ide->blocksize, ide->start_nsec * ide->blocksize);
			if (IDE_LOG > 1)
				write_log(_T("IDE%d initial read, %d bytes\n"), ide->num, nsec_total * ide->blocksize);
		}
		if (IDE_LOG > 1)
			write_log (_T("IDE%d read, read %d/%d bytes, buffer offset=%d\n"), ide->num, nsec * ide->blocksize, nsec_total * ide->blocksize, ide->buffer_offset);
	}
	ide->intdrq = true;
	last = dec_nsec (ide, nsec) == 0;
	// ATA-2 spec says CHS/LBA does only need to be updated if error condition
	if (ide->ata_level != 2 || !last) {
		put_lbachs (ide, lba, cyl, head, sec, last ? nsec - 1 : nsec);
	}
	if (last && ide->direction) {
		if (IDE_LOG > 1)
			write_log(_T("IDE%d write finished, %d bytes\n"), ide->num, ide->start_nsec * ide->blocksize);
		ide->intdrq = false;
		hdf_write(&ide->hdhfd.hfd, ide->secbuf, ide->start_lba * ide->blocksize, ide->start_nsec * ide->blocksize);
	}

end:
	if (ide->direction) {
		if (last) {
			ide_fast_interrupt(ide);
		} else {
			ide_interrupt(ide);
		}
	} else {
		if (ide->buffer_offset == 0) {
			ide_fast_interrupt(ide);
		} else {
			ide_interrupt(ide);
		}
	}
}

static void ide_read_sectors (struct ide_hdf *ide, int flags)
{
	unsigned int cyl, head, sec, nsec;
	uae_u64 lba;
	int multi = flags & 1;

	ide->lba48cmd = (flags & 2) != 0;
	if (multi && (ide->multiple_mode == 0 || ide->ata_level < 0)) {
		ide_fail (ide);
		return;
	}
	check_maxtransfer (ide, 1);
	gui_flicker_led (LED_HD, ide->uae_unitnum, 1);
	nsec = get_nsec (ide);
	get_lbachs (ide, &lba, &cyl, &head, &sec);
	if (lba >= ide->max_lba) {
		ide_data_ready (ide);
		ide_fail_err (ide, IDE_ERR_IDNF);
		return;
	}
	if (IDE_LOG > 0)
		write_log (_T("IDE%d %s off=%d, sec=%d (%d) %s\n"),
			ide->num, (flags & 4) ? _T("verify") : _T("read"), (uae_u32)lba, nsec, ide->multiple_mode, getidemode(ide));
	if (flags & 4) {
		// verify
		ide_interrupt(ide);
		return;
	}

	ide->data_multi = multi ? ide->multiple_mode : 1;
	ide->data_offset = 0;
	ide->data_size = nsec * ide->blocksize;
	ide->direction = 0;
	ide->buffer_offset = 0;
	// read start: preload sector(s), then trigger interrupt.
	process_rw_command (ide);
}

static void ide_write_sectors (struct ide_hdf *ide, int flags)
{
	unsigned int cyl, head, sec, nsec;
	uae_u64 lba;
	int multi = flags & 1;

	ide->lba48cmd = (flags & 2) != 0;
	if (multi && (ide->multiple_mode == 0 || ide->ata_level < 0)) {
		ide_fail (ide);
		return;
	}
	check_maxtransfer (ide, 1);
	gui_flicker_led (LED_HD, ide->uae_unitnum, 2);
	nsec = get_nsec (ide);
	get_lbachs (ide, &lba, &cyl, &head, &sec);
	if (lba >= ide->max_lba) {
		ide_data_ready (ide);
		ide_fail_err (ide, IDE_ERR_IDNF);
		return;
	}
	if (IDE_LOG > 0)
		write_log (_T("IDE%d write off=%d, sec=%d (%d) %s\n"), ide->num, (uae_u32)lba, nsec, ide->multiple_mode, getidemode(ide));
	if (nsec > ide->max_lba - lba)
		nsec = (unsigned int)(ide->max_lba - lba);
	if (nsec <= 0) {
		ide_data_ready (ide);
		ide_fail_err (ide, IDE_ERR_IDNF);
		return;
	}
	ide->data_multi = multi ? ide->multiple_mode : 1;
	ide->data_offset = 0;
	ide->data_size = nsec * ide->blocksize;
	ide->direction = 1;
	ide->buffer_offset = 0;
	// write start: set DRQ and clear BSY. No interrupt.
	ide->regs.ide_status |= IDE_STATUS_DRQ;
	ide->regs.ide_status &= ~IDE_STATUS_BSY;
}

static void ide_format_track(struct ide_hdf *ide)
{
	unsigned int cyl, head, sec;
	uae_u64 lba;

	gui_flicker_led(LED_HD, ide->uae_unitnum, 2);
	cyl = (ide->regs.ide_hcyl << 8) | ide->regs.ide_lcyl;
	head = ide->regs.ide_select & 15;
	sec = ide->regs.ide_nsector;
	lba = (((uae_u64)(cyl) * ide->hdhfd.heads + (head)) * ide->hdhfd.secspertrack);
	if (lba >= ide->max_lba) {
		ide_interrupt(ide);
		return;
	}
	if (IDE_LOG > 0)
		write_log(_T("IDE%d format cyl=%d, head=%d, sectors=%d\n"), ide->num, cyl, head, sec);

	ide->data_multi = 1;
	ide->data_offset = 0;
	ide->data_size = ide->blocksize;
	ide->direction = 2;
	ide->buffer_offset = 0;

	// write start: set DRQ and clear BSY. No interrupt.
	ide->regs.ide_status |= IDE_STATUS_DRQ;
	ide->regs.ide_status &= ~IDE_STATUS_BSY;
}

static void ide_do_command (struct ide_hdf *ide, uae_u8 cmd)
{
	int lba48 = ide->lba48;

	if (IDE_LOG > 1)
		write_log (_T("**** IDE%d command %02X\n"), ide->num, cmd);
	ide->regs.ide_status &= ~ (IDE_STATUS_DRDY | IDE_STATUS_DRQ | IDE_STATUS_ERR);
	ide->regs.ide_error = 0;
	ide->intdrq = false;
	ide->lba48cmd = false;
	ide->irq_delay = 0;

	if (ide->atapi) {

		ide->atapi_drdy = true;
		if (cmd == 0x00) { /* nop */
			ide_interrupt (ide);
		} else if (cmd == 0x08) { /* device reset */
			ide_execute_drive_diagnostics (ide, true);
		} else if (cmd == 0xa1) { /* identify packet device */
			ide_identify_drive (ide);
		} else if (cmd == 0xa0) { /* packet */
			atapi_packet (ide);
		} else if (cmd == 0x90) { /* execute drive diagnostics */
			ide_execute_drive_diagnostics (ide, true);
		} else {
			ide_execute_drive_diagnostics (ide, false);
			ide->atapi_drdy = false;
			ide_fail (ide);
			write_log (_T("IDE%d: unknown ATAPI command 0x%02x\n"), ide->num, cmd);
		}

	} else {

		if (cmd == 0x10) { /* recalibrate */
			ide_recalibrate (ide);
		} else if (cmd == 0xec) { /* identify drive */
			ide_identify_drive (ide);
		} else if (cmd == 0x90) { /* execute drive diagnostics */
			ide_execute_drive_diagnostics (ide, true);
		} else if (cmd == 0x91) { /* initialize drive parameters */
			ide_initialize_drive_parameters (ide);
		} else if (cmd == 0xc6) { /* set multiple mode */
			ide_set_multiple_mode (ide);
		} else if (cmd == 0x20 || cmd == 0x21) { /* read sectors */
			ide_read_sectors(ide, 0);
		} else if (cmd == 0x40 || cmd == 0x41) { /* verify sectors */
			ide_read_sectors(ide, 4);
		} else if (cmd == 0x24 && lba48) { /* read sectors ext */
			ide_read_sectors (ide, 2);
		} else if (cmd == 0xc4) { /* read multiple */
			ide_read_sectors (ide, 1);
		} else if (cmd == 0x29 && lba48) { /* read multiple ext */
			ide_read_sectors (ide, 1|2);
		} else if (cmd == 0x30 || cmd == 0x31) { /* write sectors */
			ide_write_sectors (ide, 0);
		} else if (cmd == 0x34 && lba48) { /* write sectors ext */
			ide_write_sectors (ide, 2);
		} else if (cmd == 0xc5) { /* write multiple */
			ide_write_sectors (ide, 1);
		} else if (cmd == 0x39 && lba48) { /* write multiple ext */
			ide_write_sectors (ide, 1|2);
		} else if (cmd == 0x50) { /* format track */
			ide_format_track (ide);
		} else if (cmd == 0xef) { /* set features  */
			ide_set_features (ide);
		} else if (cmd == 0x00) { /* nop */
			ide_fail (ide);
		} else if (cmd == 0x70) { /* seek */
			ide_interrupt (ide);
		} else if (cmd == 0xe0 || cmd == 0xe1 || cmd == 0xe7 || cmd == 0xea) { /* standby now/idle/flush cache/flush cache ext */
			if (ide->ata_level < 0) {
				ide_fail(ide);
			} else {
				ide_interrupt(ide);
			}
		} else if (cmd == 0xe5) { /* check power mode */
			ide->regs.ide_nsector = 0xff;
			ide_interrupt (ide);
		} else {
			ide_fail (ide);
			write_log (_T("IDE%d: unknown ATA command 0x%02x\n"), ide->num, cmd);
		}
	}
}

static uae_u16 ide_get_data_2(struct ide_hdf *ide, int bussize)
{
	bool irq = false;
	uae_u16 v;
	int inc = bussize ? 2 : 1;

	if (ide->data_size == 0) {
		if (IDE_LOG > 0)
			write_log (_T("IDE%d DATA but no data left!? %02X PC=%08X\n"), ide->num, ide->regs.ide_status, M68K_GETPC);
		if (!ide_isdrive (ide))
			return 0xffff;
		return 0;
	}
	if (ide->packet_state) {
		if (bussize) {
			v = ide->secbuf[ide->packet_data_offset + ide->data_offset + 1] | (ide->secbuf[ide->packet_data_offset + ide->data_offset + 0] << 8);
		} else {
			v = ide->secbuf[(ide->packet_data_offset + ide->data_offset)];
		}
		if (IDE_LOG > 4)
			write_log (_T("IDE%d DATA read %04x %08x\n"), ide->num, v, M68K_GETPC);
		ide->data_offset += inc;
		if (ide->data_size < 0)
			ide->data_size += inc;
		else
			ide->data_size -= inc;
		if (ide->data_offset == ide->packet_transfer_size) {
			if (IDE_LOG > 1)
				write_log (_T("IDE%d ATAPI partial read finished, %d bytes remaining\n"), ide->num, ide->data_size);
			if (ide->data_size == 0 || ide->data_size == 1) { // 1 byte remaining: ignore, ATAPI has word transfer size.
				ide->packet_state = 0;
				atapi_data_done (ide);
				if (IDE_LOG > 1)
					write_log (_T("IDE%d ATAPI read finished, %d bytes\n"), ide->num, ide->packet_data_offset + ide->data_offset);
				irq = true;
			} else {
				process_packet_command (ide);
			}
		}
	} else {
		if (bussize) {
			v = ide->secbuf[ide->buffer_offset + ide->data_offset + 1] | (ide->secbuf[ide->buffer_offset + ide->data_offset + 0] << 8);
		} else {
			v = ide->secbuf[(ide->buffer_offset + ide->data_offset)];
		}
		if (IDE_LOG > 4)
			write_log (_T("IDE%d DATA read %04x %d/%d %08x\n"), ide->num, v, ide->data_offset, ide->data_size, M68K_GETPC);
		ide->data_offset += inc;
		if (ide->data_size < 0) {
			ide->data_size += inc;
		} else {
			ide->data_size -= inc;
			if (((ide->data_offset % ide->blocksize) == 0) && ((ide->data_offset / ide->blocksize) % ide->data_multi) == 0) {
				if (ide->data_size) {
					ide->buffer_offset += ide->data_offset;
					do_process_rw_command(ide);
				}
			}
		}
		if (ide->data_size == 0) {
			if (!(ide->regs.ide_status & IDE_STATUS_DRQ)) {
				write_log (_T("IDE%d read finished but DRQ was not active?\n"), ide->num);
			}
			ide->regs.ide_status &= ~IDE_STATUS_DRQ;
			if (IDE_LOG > 1)
				write_log (_T("IDE%d read finished\n"), ide->num);
		}
	}
	if (irq)
		ide_fast_interrupt (ide);
	return v;
}

uae_u16 ide_get_data(struct ide_hdf *ide)
{
	return ide_get_data_2(ide, 1);
}
uae_u8 ide_get_data_8bit(struct ide_hdf *ide)
{
	return (uae_u8)ide_get_data_2(ide, 0);
}

static void ide_put_data_2(struct ide_hdf *ide, uae_u16 v, int bussize)
{
	int inc = bussize ? 2 : 1;
	if (IDE_LOG > 4)
		write_log (_T("IDE%d DATA write %04x %d/%d %08x\n"), ide->num, v, ide->data_offset, ide->data_size, M68K_GETPC);
	if (ide->data_size == 0) {
		if (IDE_LOG > 0)
			write_log (_T("IDE%d DATA write without request!? %02X PC=%08X\n"), ide->num, ide->regs.ide_status, M68K_GETPC);
		return;
	}
	ide_grow_buffer(ide, ide->packet_data_offset + ide->data_offset + 2);
	if (ide->packet_state) {
		if (bussize) {
			ide->secbuf[ide->packet_data_offset + ide->data_offset + 1] = v & 0xff;
			ide->secbuf[ide->packet_data_offset + ide->data_offset + 0] = v >> 8;
		} else {
			ide->secbuf[(ide->packet_data_offset + ide->data_offset) ^ 1] = (uae_u8)v;
		}
	} else {
		if (bussize) {
			ide->secbuf[ide->buffer_offset + ide->data_offset + 1] = v & 0xff;
			ide->secbuf[ide->buffer_offset + ide->data_offset + 0] = v >> 8;
		} else {
			ide->secbuf[(ide->buffer_offset + ide->data_offset)] = (uae_u8)v;
		}
	}
	ide->data_offset += inc;
	ide->data_size -= inc;
	if (ide->packet_state) {
		if (ide->data_offset == ide->packet_transfer_size) {
			if (IDE_LOG > 0) {
				uae_u16 v = (ide->regs.ide_hcyl << 8) | ide->regs.ide_lcyl;
				write_log (_T("Data size after command received = %d (%d)\n"), v, ide->packet_data_size);
			}
			process_packet_command (ide);
		}
	} else {
		if (ide->data_size == 0) {
			process_rw_command (ide);
		} else if (((ide->data_offset % ide->blocksize) == 0) && ((ide->data_offset / ide->blocksize) % ide->data_multi) == 0) {
			int off = ide->data_offset;
			do_process_rw_command(ide);
			ide->buffer_offset += off;
		}
	}
}

void ide_put_data(struct ide_hdf *ide, uae_u16 v)
{
	ide_put_data_2(ide, v, 1);
}
void ide_put_data_8bit(struct ide_hdf *ide, uae_u8 v)
{
	ide_put_data_2(ide, v, 0);
}

uae_u32 ide_read_reg (struct ide_hdf *ide, int ide_reg)
{
	uae_u8 v = 0;
	bool isdrv = ide_isdrive (ide);

	if (!ide)
		goto end;

	if (ide->regs.ide_status & IDE_STATUS_BSY)
		ide_reg = IDE_STATUS;
	if (!ide_isdrive (ide)) {
		if (ide_reg == IDE_STATUS || ide_reg == IDE_DEVCON) {
			if (ide->pair->irq)
				ide->pair->irq = 0;
			if (ide_isdrive (ide->pair))
				v = 0x01;
			else
				v = 0xff;
		} else {
			v = 0;
		}
		goto end;
	}

	switch (ide_reg)
	{
	case IDE_SECONDARY:
	case IDE_SECONDARY + 1:
	case IDE_SECONDARY + 2:
	case IDE_SECONDARY + 3:
	case IDE_SECONDARY + 4:
	case IDE_SECONDARY + 5:
		v = 0xff;
		break;
	case IDE_DRVADDR:
		v = ((ide->ide_drv ? 2 : 1) | ((ide->regs.ide_select & 15) << 2)) ^ 0xff;
		break;
	case IDE_DATA:
		break;
	case IDE_ERROR:
		v = ide->regs.ide_error;
		break;
	case IDE_NSECTOR:
		if (isdrv) {
			if (ide->regs.ide_devcon & 0x80)
				v = ide->regs.ide_nsector2;
			else
				v = ide->regs.ide_nsector;
		}
		break;
	case IDE_SECTOR:
		if (isdrv) {
			if (ide->regs.ide_devcon & 0x80)
				v = ide->regs.ide_sector2;
			else
				v = ide->regs.ide_sector;
			check_maxtransfer (ide, 2);
		}
		break;
	case IDE_LCYL:
		if (isdrv) {
			if (ide->regs.ide_devcon & 0x80)
				v = ide->regs.ide_lcyl2;
			else
				v = ide->regs.ide_lcyl;
		}
		break;
	case IDE_HCYL:
		if (isdrv) {
			if (ide->regs.ide_devcon & 0x80)
				v = ide->regs.ide_hcyl2;
			else
				v = ide->regs.ide_hcyl;
		}
		break;
	case IDE_SELECT:
		v = ide->regs.ide_select;
		break;
	case IDE_STATUS:
		if (ide->irq && IDE_LOG > 1)
			write_log(_T("IDE IRQ CLEAR\n"));
		ide->irq = 0;
		ide->irq_new = false;
		/* fall through */
	case IDE_DEVCON: /* ALTSTATUS when reading */
		if (!isdrv) {
			v = 0;
			if (ide->regs.ide_error)
				v |= IDE_STATUS_ERR;
		} else {
			v = ide->regs.ide_status;
			if (!ide->atapi || (ide->atapi && ide->atapi_drdy))
				v |= IDE_STATUS_DRDY | IDE_STATUS_DSC;
		}
		break;
	}
end:
	if (IDE_LOG > 2 && ide_reg > 0 && (1 || ide->num > 0))
		write_log (_T("IDE%d GET register %d->%02X (%08X)\n"), ide->num, ide_reg, (uae_u32)v & 0xff, m68k_getpc ());
	return v;
}

void ide_write_reg (struct ide_hdf *ide, int ide_reg, uae_u32 val)
{
	if (!ide)
		return;
	
	ide->regs1->ide_devcon &= ~0x80; /* clear HOB */
	ide->regs0->ide_devcon &= ~0x80; /* clear HOB */
	if (IDE_LOG > 2 && ide_reg > 0 && (1 || ide->num > 0))
		write_log (_T("IDE%d PUT register %d=%02X (%08X)\n"), ide->num, ide_reg, (uae_u32)val & 0xff, m68k_getpc ());

	switch (ide_reg)
	{
	case IDE_DRVADDR:
		break;
	case IDE_DEVCON:
		if ((ide->regs.ide_devcon & 4) == 0 && (val & 4) != 0) {
			reset_device (ide, true, false);
			if (IDE_LOG > 1)
				write_log (_T("IDE%d: SRST\n"), ide->num);
		}
		ide->regs0->ide_devcon = val;
		ide->regs1->ide_devcon = val;
		break;
	case IDE_DATA:
		break;
	case IDE_ERROR:
		ide->regs0->ide_feat2 = ide->regs0->ide_feat;
		ide->regs0->ide_feat = val;
		ide->regs1->ide_feat2 = ide->regs1->ide_feat;
		ide->regs1->ide_feat = val;
		break;
	case IDE_NSECTOR:
		ide->regs0->ide_nsector2 = ide->regs0->ide_nsector;
		ide->regs0->ide_nsector = val;
		ide->regs1->ide_nsector2 = ide->regs1->ide_nsector;
		ide->regs1->ide_nsector = val;
		break;
	case IDE_SECTOR:
		ide->regs0->ide_sector2 = ide->regs0->ide_sector;
		ide->regs0->ide_sector = val;
		ide->regs1->ide_sector2 = ide->regs1->ide_sector;
		ide->regs1->ide_sector = val;
		break;
	case IDE_LCYL:
		ide->regs0->ide_lcyl2 = ide->regs0->ide_lcyl;
		ide->regs0->ide_lcyl = val;
		ide->regs1->ide_lcyl2 = ide->regs1->ide_lcyl;
		ide->regs1->ide_lcyl = val;
		break;
	case IDE_HCYL:
		ide->regs0->ide_hcyl2 = ide->regs0->ide_hcyl;
		ide->regs0->ide_hcyl = val;
		ide->regs1->ide_hcyl2 = ide->regs1->ide_hcyl;
		ide->regs1->ide_hcyl = val;
		break;
	case IDE_SELECT:
		ide->regs0->ide_select = val;
		ide->regs1->ide_select = val;
#if IDE_LOG > 2
		if (ide->ide_drv != (val & 0x10) ? 1 : 0)
			write_log (_T("DRIVE=%d\n"), (val & 0x10) ? 1 : 0);
#endif
		ide->pair->ide_drv = ide->ide_drv = (val & 0x10) ? 1 : 0;
		break;
	case IDE_STATUS:
		ide->irq = 0;
		ide->irq_new = false;
		if (ide_isdrive (ide)) {
			ide->regs.ide_status |= IDE_STATUS_BSY;
			ide_do_command (ide, val);
		}
		break;
	}
}

static int ide_thread (void *idedata)
{
	struct ide_thread_state *its = (struct ide_thread_state*)idedata;
	for (;;) {
		uae_u32 unit = read_comm_pipe_u32_blocking (&its->requests);
		struct ide_hdf *ide;
		if (its->state == 0 || unit == 0xfffffff)
			break;
		ide = its->idetable[unit & 0x7fff];
		if (unit & 0x8000)
			do_process_packet_command (ide);
		else
			do_process_rw_command (ide);
	}
	its->state = -1;
	return 0;
}

void start_ide_thread(struct ide_thread_state *its)
{
	if (!its->state) {
		its->state = 1;
		init_comm_pipe (&its->requests, 100, 1);
		uae_start_thread (_T("ide"), ide_thread, its, NULL);
	}
}

void stop_ide_thread(struct ide_thread_state *its)
{
	if (its->state > 0) {
		its->state = 0;
		write_comm_pipe_u32 (&its->requests, 0xffffffff, 1);
		while(its->state == 0)
			sleep_millis (10);
		destroy_comm_pipe(&its->requests);
		its->state = 0;
	}
}

void ide_initialize(struct ide_hdf **idetable, int chpair)
{
	struct ide_hdf *ide0 = idetable[chpair * 2 + 0];
	struct ide_hdf *ide1 = idetable[chpair * 2 + 1];

	if (!ide0 || !ide1)
		return;

	ide0->regs0 = &ide0->regs;
	ide0->regs1 = &ide1->regs;
	ide0->pair = ide1;

	ide1->regs1 = &ide1->regs;
	ide1->regs0 = &ide0->regs;
	ide1->pair = ide0;

	ide0->num = chpair * 2 + 0;
	ide1->num = chpair * 2 + 1;

	reset_device (ide0, true, true);
}

void alloc_ide_mem (struct ide_hdf **idetable, int max, struct ide_thread_state *its)
{
	for (int i = 0; i < max; i++) {
		struct ide_hdf *ide;
		if (!idetable[i]) {
			ide = idetable[i] = xcalloc (struct ide_hdf, 1);
			ide->cd_unit_num = -1;
		}
		ide = idetable[i];
		ide_grow_buffer(ide, 1024);
		if (its)
			ide->its = its;
	}
}

void remove_ide_unit(struct ide_hdf **idetable, int ch)
{
	struct ide_hdf *ide;
	if (!idetable)
		return;
	ide = idetable[ch];
	if (ide) {
		struct ide_thread_state *its;
		hdf_hd_close(&ide->hdhfd);
		scsi_free(ide->scsi);
		xfree(ide->secbuf);
		its = ide->its;
		memset(ide, 0, sizeof(struct ide_hdf));
		ide->its = its;
	}
}

struct ide_hdf *add_ide_unit (struct ide_hdf **idetable, int max, int ch, struct uaedev_config_info *ci, struct romconfig *rc)
{
	struct ide_hdf *ide;
	bool vb;

	alloc_ide_mem(idetable, max, NULL);
	if (ch < 0)
		return NULL;
	ide = idetable[ch];
	if (ci)
		memcpy (&ide->hdhfd.hfd.ci, ci, sizeof (struct uaedev_config_info));
	if (ci->type == UAEDEV_CD && ci->device_emu_unit >= 0) {

		device_func_init(0);
		ide->scsi = scsi_alloc_cd(ch, ci->device_emu_unit, true, ci->uae_unitnum);
		if (!ide->scsi) {
			write_log(_T("IDE: CD EMU unit %d failed to open\n"), ide->cd_unit_num);
			return NULL;
		}

		ide_identity_buffer(ide);
		ide->cd_unit_num = ci->device_emu_unit;
		ide->atapi = true;
		ide->atapi_device_type = 5;
		ide->blocksize = 512;
		ide->uae_unitnum = ci->uae_unitnum;
		gui_flicker_led(LED_CD, ci->uae_unitnum, -1);

		write_log(_T("IDE%d CD %d\n"), ch, ide->cd_unit_num);

	} else if (ci->type == UAEDEV_TAPE) {

		ide->scsi = scsi_alloc_tape(ch, ci->rootdir, ci->readonly, ci->uae_unitnum);
		if (!ide->scsi) {
			write_log(_T("IDE: TAPE EMU unit %d failed to open\n"), ch);
			return NULL;
		}

		ide_identity_buffer(ide);
		ide->atapi = true;
		ide->atapi_device_type = 1;
		ide->blocksize = 512;
		ide->cd_unit_num = -1;
		ide->uae_unitnum = ci->uae_unitnum;

		write_log(_T("IDE%d TAPE %d\n"), ch, ide->cd_unit_num);


	} else if (ci->type == UAEDEV_HDF) {

		if (!hdf_hd_open (&ide->hdhfd))
			return NULL;

		ide->max_multiple_mode = 128;
		ide->blocksize = ide->hdhfd.hfd.virtual_rdb ? 512 : ide->hdhfd.hfd.ci.blocksize;
		ide->max_lba = ide->hdhfd.size / ide->blocksize;
		ide->lba48 = (ide->hdhfd.hfd.ci.unit_special_flags & 1) || ide->hdhfd.size >= 128 * (uae_u64)0x40000000 ? 1 : 0;
		ide->lba = true;
		ide->uae_unitnum = ci->uae_unitnum;
		gui_flicker_led (LED_HD, ide->uae_unitnum, -1);
		ide->cd_unit_num = -1;
		ide->media_type = ci->controller_media_type;
		ide->ata_level = ci->unit_feature_level;
		if (!ide->ata_level && (ide->hdhfd.size >= 4 * (uae_u64)0x40000000 || ide->media_type))
			ide->ata_level = 1;
		ide_identity_buffer(ide);

		if (!ide->byteswap)
			ata_byteswapidentity(ide->secbuf);
		struct uaedev_config_info ci = { 0 };
		ata_parse_identity(ide->secbuf, &ci, &ide->lba, &ide->lba48, &ide->max_multiple_mode);
		ide->hdhfd.cyls = ide->hdhfd.cyls_def = ci.pcyls;
		ide->hdhfd.heads = ide->hdhfd.heads_def = ci.pheads;
		ide->hdhfd.secspertrack = ide->hdhfd.secspertrack_def = ci.psecs;
		if (ci.max_lba)
			ide->max_lba = ci.max_lba;
		if (ide->lba48 && !ide->ata_level)
			ide->ata_level = 1;
		if (ini_getbool(ide->hdhfd.hfd.geometry, _T("IDENTITY"), _T("disabled"), &vb)) {
			if (vb) {
				ide->ata_level = -1;
				ide->lba48 = false;
				ide->lba = false;
				ide->max_multiple_mode = 0;
			}
		}

		write_log (_T("IDE%d HD '%s', LCHS=%d/%d/%d. PCHS=%d/%d/%d %uM. MM=%d LBA48=%d\n"),
			ch, ide->hdhfd.hfd.ci.rootdir,
			ide->hdhfd.cyls, ide->hdhfd.heads, ide->hdhfd.secspertrack,
			ide->hdhfd.hfd.ci.pcyls, ide->hdhfd.hfd.ci.pheads, ide->hdhfd.hfd.ci.psecs,
			(int)(ide->hdhfd.size / (1024 * 1024)), ide->max_multiple_mode, ide->lba48);

	}
	ide->regs.ide_status = 0;
	ide->data_offset = 0;
	ide->data_size = 0;
	return ide;
}

uae_u8 *ide_save_state(uae_u8 *dst, struct ide_hdf *ide)
{
	save_u64 (ide->hdhfd.size);
	save_string (ide->hdhfd.hfd.ci.rootdir);
	save_u32 (ide->hdhfd.hfd.ci.blocksize);
	save_u32 (ide->hdhfd.hfd.ci.readonly);
	save_u8 (ide->multiple_mode);
	save_u32 (ide->hdhfd.cyls);
	save_u32 (ide->hdhfd.heads);
	save_u32 (ide->hdhfd.secspertrack);
	save_u8 (ide->regs.ide_select);
	save_u8 (ide->regs.ide_nsector);
	save_u8 (ide->regs.ide_nsector2);
	save_u8 (ide->regs.ide_sector);
	save_u8 (ide->regs.ide_sector2);
	save_u8 (ide->regs.ide_lcyl);
	save_u8 (ide->regs.ide_lcyl2);
	save_u8 (ide->regs.ide_hcyl);
	save_u8 (ide->regs.ide_hcyl2);
	save_u8 (ide->regs.ide_feat);
	save_u8 (ide->regs.ide_feat2);
	save_u8 (ide->regs.ide_error);
	save_u8 (ide->regs.ide_devcon);
	save_u64 (ide->hdhfd.hfd.virtual_size);
	save_u32 (ide->hdhfd.hfd.ci.sectors);
	save_u32 (ide->hdhfd.hfd.ci.surfaces);
	save_u32 (ide->hdhfd.hfd.ci.reserved);
	save_u32 (ide->hdhfd.hfd.ci.bootpri);
	return dst;
}

uae_u8 *ide_restore_state(uae_u8 *src, struct ide_hdf *ide)
{
	ide->multiple_mode = restore_u8 ();
	ide->hdhfd.cyls = restore_u32 ();
	ide->hdhfd.heads = restore_u32 ();
	ide->hdhfd.secspertrack = restore_u32 ();
	ide->regs.ide_select = restore_u8 ();
	ide->regs.ide_nsector = restore_u8 ();
	ide->regs.ide_sector = restore_u8 ();
	ide->regs.ide_lcyl = restore_u8 ();
	ide->regs.ide_hcyl = restore_u8 ();
	ide->regs.ide_feat = restore_u8 ();
	ide->regs.ide_nsector2 = restore_u8 ();
	ide->regs.ide_sector2 = restore_u8 ();
	ide->regs.ide_lcyl2 = restore_u8 ();
	ide->regs.ide_hcyl2 = restore_u8 ();
	ide->regs.ide_feat2 = restore_u8 ();
	ide->regs.ide_error = restore_u8 ();
	ide->regs.ide_devcon = restore_u8 ();
	ide->hdhfd.hfd.virtual_size = restore_u64 ();
	ide->hdhfd.hfd.ci.sectors = restore_u32 ();
	ide->hdhfd.hfd.ci.surfaces = restore_u32 ();
	ide->hdhfd.hfd.ci.reserved = restore_u32 ();
	ide->hdhfd.hfd.ci.bootpri = restore_u32 ();
	return src;
}
