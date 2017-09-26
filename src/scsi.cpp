/*
* UAE - The Un*x Amiga Emulator
*
* SCSI and SASI emulation (not uaescsi.device)
*
* Copyright 2007-2015 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "filesys.h"
#include "blkdev.h"
#include "scsi.h"

#define NCR_LAST 1

static const int outcmd[] = { 0x04, 0x0a, 0x0c, 0x2a, 0xaa, 0x15, 0x55, 0x0f, -1 };
static const int incmd[] = { 0x01, 0x03, 0x08, 0x12, 0x1a, 0x5a, 0x25, 0x28, 0x34, 0x37, 0x42, 0x43, 0xa8, 0x51, 0x52, 0xb9, 0xbd, 0xbe, -1 };
static const int nonecmd[] = { 0x00, 0x05, 0x06, 0x09, 0x0b, 0x11, 0x16, 0x17, 0x19, 0x1b, 0x1d, 0x1e, 0x2b, 0x35, 0x45, 0x47, 0x48, 0x49, 0x4b, 0x4e, 0xa5, 0xa9, 0xba, 0xbc, 0xe0, 0xe3, 0xe4, -1 };
static const int scsicmdsizes[] = { 6, 10, 10, 12, 16, 12, 10, 6 };

static void scsi_illegal_command(struct scsi_data *sd)
{
	uae_u8 *s = sd->sense;

	memset (s, 0, sizeof (sd->sense));
	sd->status = SCSI_STATUS_CHECK_CONDITION;
	s[0] = 0x70;
	s[2] = 5; /* ILLEGAL REQUEST */
	s[12] = 0x24; /* ILLEGAL FIELD IN CDB */
	sd->sense_len = 0x12;
}

static void scsi_grow_buffer(struct scsi_data *sd, int newsize)
{
	if (sd->buffer_size >= newsize)
		return;
	uae_u8 *oldbuf = sd->buffer;
	int oldsize = sd->buffer_size;
	sd->buffer_size = newsize + SCSI_DEFAULT_DATA_BUFFER_SIZE;
	write_log(_T("SCSI buffer %d -> %d\n"), oldsize, sd->buffer_size);
	sd->buffer = xmalloc(uae_u8, sd->buffer_size);
	memcpy(sd->buffer, oldbuf, oldsize);
	xfree(oldbuf);
}

static int scsi_data_dir(struct scsi_data *sd)
{
	int i;
	uae_u8 cmd;

	cmd = sd->cmd[0];
	for (i = 0; outcmd[i] >= 0; i++) {
		if (cmd == outcmd[i]) {
			return 1;
		}
	}
	for (i = 0; incmd[i] >= 0; i++) {
		if (cmd == incmd[i]) {
			return -1;
		}
	}
	for (i = 0; nonecmd[i] >= 0; i++) {
		if (cmd == nonecmd[i]) {
			return 0;
		}
	}
	write_log (_T("SCSI command %02X, no direction specified!\n"), sd->cmd[0]);
	return 0;
}

bool scsi_emulate_analyze (struct scsi_data *sd)
{
	int cmd_len, data_len, data_len2, tmp_len;

	data_len = sd->data_len;
	data_len2 = 0;
	cmd_len = scsicmdsizes[sd->cmd[0] >> 5];
	if (sd->hdhfd && sd->hdhfd->ansi_version < 2 && cmd_len > 10)
		goto nocmd;
	sd->cmd_len = cmd_len;
	switch (sd->cmd[0])
	{
	case 0x04: // FORMAT UNIT
		if (sd->device_type == UAEDEV_CD)
			goto nocmd;
		// FmtData set?
		if (sd->cmd[1] & 0x10) {
			int cl = (sd->cmd[1] & 8) != 0;
			int dlf = sd->cmd[1] & 7;
			data_len2 = 4;
		} else {
			sd->direction = 0;
			sd->data_len = 0;
			return true;
		}
	break;
	case 0x06: // FORMAT TRACK
		if (sd->device_type == UAEDEV_CD)
			goto nocmd;
		sd->direction = 0;
		sd->data_len = 0;
		return true;
	case 0x0c: // INITIALIZE DRIVE CHARACTERICS (SASI)
		if (sd->hfd && sd->hfd->ci.unit_feature_level < HD_LEVEL_SASI)
			goto nocmd;
		data_len = 8;
	break;
	case 0x08: // READ(6)
		data_len2 = (sd->cmd[4] == 0 ? 256 : sd->cmd[4]) * sd->blocksize;
		scsi_grow_buffer(sd, data_len2);
	break;
	case 0x28: // READ(10)
		data_len2 = ((sd->cmd[7] << 8) | (sd->cmd[8] << 0)) * (uae_s64)sd->blocksize;
		scsi_grow_buffer(sd, data_len2);
	break;
	case 0xa8: // READ(12)
		data_len2 = ((sd->cmd[6] << 24) | (sd->cmd[7] << 16) | (sd->cmd[8] << 8) | (sd->cmd[9] << 0)) * (uae_s64)sd->blocksize;
		scsi_grow_buffer(sd, data_len2);
	break;
	case 0x0f: // WRITE SECTOR BUFFER
		data_len = sd->blocksize;
		scsi_grow_buffer(sd, data_len);
	break;
	case 0x0a: // WRITE(6)
		if (sd->device_type == UAEDEV_CD)
			goto nocmd;
		data_len = (sd->cmd[4] == 0 ? 256 : sd->cmd[4]) * sd->blocksize;
		scsi_grow_buffer(sd, data_len);
	break;
	case 0x2a: // WRITE(10)
		if (sd->device_type == UAEDEV_CD)
			goto nocmd;
		data_len = ((sd->cmd[7] << 8) | (sd->cmd[8] << 0)) * (uae_s64)sd->blocksize;
		scsi_grow_buffer(sd, data_len);
	break;
	case 0xaa: // WRITE(12)
		if (sd->device_type == UAEDEV_CD)
			goto nocmd;
		data_len = ((sd->cmd[6] << 24) | (sd->cmd[7] << 16) | (sd->cmd[8] << 8) | (sd->cmd[9] << 0)) * (uae_s64)sd->blocksize;
		scsi_grow_buffer(sd, data_len);
	break;
	case 0xbe: // READ CD
	case 0xb9: // READ CD MSF
		if (sd->device_type != UAEDEV_CD)
			goto nocmd;
		tmp_len = (sd->cmd[6] << 16) | (sd->cmd[7] << 8) | sd->cmd[8];
		// max block transfer size, it is usually smaller.
		tmp_len *= 2352 + 96;
		scsi_grow_buffer(sd, tmp_len);
	break;
	case 0x2f: // VERIFY
		if (sd->cmd[1] & 2) {
			sd->data_len = ((sd->cmd[7] << 8) | (sd->cmd[8] << 0)) * (uae_s64)sd->blocksize;
			scsi_grow_buffer(sd, sd->data_len);
			sd->direction = 1;
		} else {
			sd->data_len = 0;
			sd->direction = 0;
		}
		return true;
	case 0x15: // MODE SELECT
	case 0x55: 
	if (sd->device_type != UAEDEV_CD)
		goto nocmd;
	break;
	}
	if (data_len < 0) {
		if (cmd_len == 6)
			sd->data_len = sd->cmd[4];
		else
			sd->data_len = (sd->cmd[7] << 8) | sd->cmd[8];
	} else {
		sd->data_len = data_len;
	}
	sd->direction = scsi_data_dir (sd);
	return true;
nocmd:
	sd->status = SCSI_STATUS_CHECK_CONDITION;
	sd->direction = 0;
	scsi_illegal_command(sd);
	return false;
}

void scsi_clear_sense(struct scsi_data *sd)
{
	memset (sd->sense, 0, sizeof (sd->sense));
	memset (sd->reply, 0, sizeof (sd->reply));
	sd->sense[0] = 0x70;
}
static void copysense(struct scsi_data *sd)
{
	bool sasi = sd->hfd && (sd->hfd->ci.unit_feature_level >= HD_LEVEL_SASI && sd->hfd->ci.unit_feature_level <= HD_LEVEL_SASI_ENHANCED);
	int len = sd->cmd[4];
	if (len == 0 || sasi)
		len = 4;
	memset(sd->buffer, 0, len);
	int tlen = sd->sense_len > len ? len : sd->sense_len;
	memcpy(sd->buffer, sd->sense, tlen);
	if (!sasi && sd->sense_len == 0) {
		// at least 0x12 bytes if SCSI and no sense
		tlen = len > 0x12 ? 0x12 : len;
		sd->buffer[0] = 0x70;
		sd->sense_len = tlen;
	}
	sd->data_len = tlen;
	scsi_clear_sense(sd);
}
static void copyreply(struct scsi_data *sd)
{
	if (sd->status == 0 && sd->reply_len > 0) {
		memset(sd->buffer, 0, 256);
		memcpy(sd->buffer, sd->reply, sd->reply_len);
		sd->data_len = sd->reply_len;
	}
}

static bool handle_ca(struct scsi_data *sd)
{
	bool cc = sd->sense_len > 2 && sd->sense[2] >= 2;
	bool ua = sd->unit_attention != 0;
	uae_u8 cmd = sd->cmd[0];

	// INQUIRY
	if (cmd == 0x12) {
		if (ua && cc && sd->sense[2] == 6) {
			// INQUIRY clears UA only if previous
			// command was aborted due to UA
			sd->unit_attention = 0;
		}
		memset(sd->reply, 0, sizeof(sd->reply));
		return true;
	}

	// REQUEST SENSE
	if (cmd == 0x03) {
		if (ua) {
			uae_u8 *s = sd->sense;
			scsi_clear_sense(sd);
			s[0] = 0x70;
			s[2] = 6; /* UNIT ATTENTION */
			s[12] = (sd->unit_attention >> 8) & 0xff;
			s[13] = (sd->unit_attention >> 0) & 0xff;
			sd->sense_len = 0x12;
		}
		sd->unit_attention = 0;
		return true;
	}

	scsi_clear_sense(sd);

	if (ua) {
		uae_u8 *s = sd->sense;
		s[0] = 0x70;
		s[2] = 6; /* UNIT ATTENTION */
		s[12] = (sd->unit_attention >> 8) & 0xff;
		s[13] = (sd->unit_attention >> 0) & 0xff;
		sd->sense_len = 0x12;
		sd->unit_attention = 0;
		sd->status = 2;
		return false;
	}

	return true;
}

void scsi_emulate_cmd(struct scsi_data *sd)
{
	sd->status = 0;
	if ((sd->message[0] & 0xc0) == 0x80 && (sd->message[0] & 0x1f)) {
		uae_u8 lun = sd->message[0] & 0x1f;
		if (lun > 7)
			lun = 7;
		sd->cmd[1] &= ~(7 << 5);
		sd->cmd[1] |= lun << 5;
	}
	if (sd->device_type == UAEDEV_CD && sd->cd_emu_unit >= 0) {
		uae_u32 ua = 0;
		ua = scsi_cd_emulate(sd->cd_emu_unit, NULL, 0, 0, 0, 0, 0, 0, 0, sd->atapi);
		if (ua)
			sd->unit_attention = ua;
		if (handle_ca(sd)) {
			if (sd->cmd[0] == 0x03) { /* REQUEST SENSE */
				scsi_cd_emulate(sd->cd_emu_unit, sd->cmd, 0, 0, 0, 0, 0, 0, 0, sd->atapi); /* ack request sense */
				copysense(sd);
			} else {
				sd->status = scsi_cd_emulate(sd->cd_emu_unit, sd->cmd, sd->cmd_len, sd->buffer, &sd->data_len, sd->reply, &sd->reply_len, sd->sense, &sd->sense_len, sd->atapi);
				copyreply(sd);
			}
		}
	} else if (sd->device_type == UAEDEV_HDF) {
		uae_u32 ua = 0;
		ua = scsi_hd_emulate(sd->hfd, sd->hdhfd, NULL, 0, 0, 0, 0, 0, 0, 0);
		if (ua)
			sd->unit_attention = ua;
		if (handle_ca(sd)) {
			if (sd->cmd[0] == 0x03) { /* REQUEST SENSE */
				scsi_hd_emulate(sd->hfd, sd->hdhfd, sd->cmd, 0, 0, 0, 0, 0, sd->sense, &sd->sense_len);
				copysense(sd);
			} else {
				sd->status = scsi_hd_emulate(sd->hfd, sd->hdhfd,
					sd->cmd, sd->cmd_len, sd->buffer, &sd->data_len, sd->reply, &sd->reply_len, sd->sense, &sd->sense_len);
				copyreply(sd);
			}
		}
	}
	sd->offset = 0;
}

static void allocscsibuf(struct scsi_data *sd)
{
	sd->buffer_size = SCSI_DEFAULT_DATA_BUFFER_SIZE;
	sd->buffer = xcalloc(uae_u8, sd->buffer_size);
}

struct scsi_data *scsi_alloc_generic(struct hardfiledata *hfd, int type)
{
	struct scsi_data *sd = xcalloc(struct scsi_data, 1);
	sd->hfd = hfd;
	sd->id = -1;
	sd->cd_emu_unit = -1;
	sd->blocksize = hfd->ci.blocksize;
	sd->device_type = type;
	allocscsibuf(sd);
	return sd;
}

struct scsi_data *scsi_alloc_cd(int id, int unitnum, bool atapi)
{
	struct scsi_data *sd;
	if (!sys_command_open (unitnum)) {
		write_log (_T("SCSI: CD EMU scsi unit %d failed to open\n"), unitnum);
		return NULL;
	}
	sd = xcalloc (struct scsi_data, 1);
	sd->id = id;
	sd->cd_emu_unit = unitnum;
	sd->atapi = atapi;
	sd->blocksize = 2048;
	sd->device_type = UAEDEV_CD;
	allocscsibuf(sd);
	return sd;
}

void scsi_free(struct scsi_data *sd)
{
	if (!sd)
		return;
	if (sd->cd_emu_unit >= 0) {
		sys_command_close (sd->cd_emu_unit);
		sd->cd_emu_unit = -1;
	}
	xfree(sd->buffer);
	xfree(sd);
}

void scsi_start_transfer(struct scsi_data *sd)
{
	sd->offset = 0;
}
