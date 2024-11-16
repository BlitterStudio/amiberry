/*
* UAE - The Un*x Amiga Emulator
*
* CDROM/HD low level access code (IOCTL)
*
* Copyright 2024 Dimitris Panokostas
* Copyright 2002-2010 Toni Wilen
*
*/

#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "traps.h"
#include "uae.h"
#include "threaddep/thread.h"
#include "blkdev.h"
#include "scsidev.h"
#include "gui.h"
#include "audio.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef __MACH__
#include <errno.h>
#include <IOKit/storage/IODVDMediaBSDClient.h>
#include <IOKit/storage/IODVDMedia.h>
#include <IOKit/storage/IODVDTypes.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#else
#include <linux/cdrom.h>
#include <linux/hdreg.h>
#include <linux/errno.h>
#include <scsi/sg.h>
#endif

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "cda_play.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

#define IOCTL_DATA_BUFFER 8192

#ifdef WITH_SCSI_IOCTL

struct dev_info_ioctl {
	int fd;
	uae_u8* tempbuffer;
	TCHAR drvletter[30];
	TCHAR drvlettername[30];
	TCHAR devname[30];
	int type;
	struct cdrom_tocentry cdromtoc;
	uae_u8 trackmode[100];
	UINT errormode;
	int fullaccess;
	struct device_info di;
	bool open;
	bool usesptiread;
	bool changed;
	struct cda_play cda;
};

static struct dev_info_ioctl ciw32[MAX_TOTAL_SCSI_DEVICES];
static int unittable[MAX_TOTAL_SCSI_DEVICES];
static int bus_open;
static uae_sem_t play_sem;

static int sys_cddev_open(struct dev_info_ioctl* ciw, int unitnum);
static void sys_cddev_close(struct dev_info_ioctl* ciw, int unitnum);

static int getunitnum(struct dev_info_ioctl* ciw)
{
	if (!ciw)
		return -1;
	int idx = (int)(ciw - &ciw32[0]);
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		if (unittable[i] - 1 == idx)
			return i;
	}
	return -1;
}

static struct dev_info_ioctl* unitcheck(int unitnum)
{
	if (unitnum < 0 || unitnum >= MAX_TOTAL_SCSI_DEVICES)
		return NULL;
	if (unittable[unitnum] <= 0)
		return NULL;
	unitnum = unittable[unitnum] - 1;
	if (ciw32[unitnum].drvletter == 0)
		return NULL;
	return &ciw32[unitnum];
}

static struct dev_info_ioctl* unitisopen(int unitnum)
{
	struct dev_info_ioctl* di = unitcheck(unitnum);
	if (!di)
		return NULL;
	if (di->open == false)
		return NULL;
	return di;
}

static int win32_error(struct dev_info_ioctl* ciw, int unitnum, const TCHAR* format, ...)
{
	va_list arglist;
	TCHAR buf[1000];
	int err = errno;

	if (err == ENOMEDIUM) {
		write_log(_T("IOCTL: media change, re-opening device\n"));
		sys_cddev_close(ciw, unitnum);
		if (sys_cddev_open(ciw, unitnum))
			write_log(_T("IOCTL: re-opening failed!\n"));
		return -1;
	}
	va_start(arglist, format);
	_vsntprintf(buf, sizeof buf / sizeof(TCHAR), format, arglist);
	write_log(_T("IOCTL ERR: unit=%d,%s,%d: %s\n"), unitnum, buf, err, strerror(err));
	va_end(arglist);
	return err;
}

static int close_createfile(struct dev_info_ioctl* ciw)
{
	ciw->fullaccess = 0;
	if (ciw->fd != -1) {
		if (log_scsi)
			write_log(_T("IOCTL: IOCTL close\n"));
		close(ciw->fd);
		if (log_scsi)
			write_log(_T("IOCTL: IOCTL close completed\n"));
		ciw->fd = -1;
		return 1;
	}
	return 0;
}

static int open_createfile(struct dev_info_ioctl* ciw, int fullaccess)
{
	if (ciw->fd != -1) {
		//if (fullaccess && ciw->fullaccess == 0) {
			close_createfile(ciw);
		//}
		// else {
		// 	return 1;
		// }
	}
	if (log_scsi)
		write_log(_T("IOCTL: opening IOCTL %s\n"), ciw->devname);
	ciw->fd = open(ciw->devname, fullaccess ? O_RDWR : O_RDONLY);
	if (ciw->fd == -1) {
		int err = errno;
		write_log(_T("IOCTL: failed to open '%s', err=%d\n"), ciw->devname, err);
		return 0;
	}
	ciw->fullaccess = fullaccess;
	if (log_scsi)
		write_log(_T("IOCTL: IOCTL open completed\n"));
	return 1;
}


static int do_raw_scsi(struct dev_info_ioctl* ciw, int unitnum, unsigned char* cmd, int cmdlen, unsigned char* data, int datalen) {
	struct sg_io_hdr io_hdr;
	unsigned char sense_buffer[32];
	memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = cmdlen;
	io_hdr.mx_sb_len = sizeof(sense_buffer);
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.dxfer_len = datalen;
	io_hdr.dxferp = data;
	io_hdr.cmdp = cmd;
	io_hdr.sbp = sense_buffer;
	io_hdr.timeout = 20000; // 20 seconds

	if (ioctl(ciw->fd, SG_IO, &io_hdr) < 0) {
		perror("SG_IO");
		return 0;
	}
	return io_hdr.dxfer_len;
}

static void sub_deinterleave(const uae_u8* s, uae_u8* d)
{
	for (int i = 0; i < 8 * 12; i++) {
		int dmask = 0x80;
		int smask = 1 << (7 - (i / 12));
		(*d) = 0;
		for (int j = 0; j < 8; j++) {
			(*d) |= (s[(i % 12) * 8 + j] & smask) ? dmask : 0;
			dmask >>= 1;
		}
		d++;
	}
}

static int spti_read(struct dev_info_ioctl* ciw, int unitnum, uae_u8* data, int sector, int sectorsize)
{
	uae_u8 cmd[12] = { 0xbe, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
	int tlen = sectorsize;

	write_log(_T("spti_read %d %d %d\n"), unitnum, sector, sectorsize);

	if (sectorsize == 2048 || sectorsize == 2336 || sectorsize == 2328) {
		cmd[9] |= 1 << 4; // userdata
	}
	else if (sectorsize >= 2352) {
		cmd[9] |= 1 << 4; // userdata
		cmd[9] |= 1 << 3; // EDC&ECC
		cmd[9] |= 1 << 7; // sync
		cmd[9] |= 3 << 5; // header code
		if (sectorsize > 2352) {
			cmd[10] |= 1; // RAW P-W
		}
		if (sectorsize > 2352 + SUB_CHANNEL_SIZE) {
			cmd[9] |= 0x2 << 1; // C2
		}
	}
	cmd[3] = (uae_u8)(sector >> 16);
	cmd[4] = (uae_u8)(sector >> 8);
	cmd[5] = (uae_u8)(sector >> 0);
	if (unitnum >= 0)
		gui_flicker_led(LED_CD, unitnum, LED_CD_ACTIVE);
	int len = sizeof cmd;
	return do_raw_scsi(ciw, unitnum, cmd, len, data, tlen);
}

extern void encode_l2(uae_u8* p, int address);

static int read2048(struct dev_info_ioctl* ciw, int sector)
{
	off_t offset = (off_t)sector * 2048;

	if (lseek(ciw->fd, offset, SEEK_SET) == (off_t)-1) {
		return 0;
	}
	ssize_t dtotal = read(ciw->fd, ciw->tempbuffer, 2048);
	return dtotal == 2048 ? 2048 : 0;
}

static int read_block(struct dev_info_ioctl* ciw, int unitnum, uae_u8* data, int sector, int size, int sectorsize)
{
	uae_u8* p = ciw->tempbuffer;
	int ret;
	int origsize = size;
	int origsector = sector;
	uae_u8* origdata = data;
	bool got;

retry:
	if (!open_createfile(ciw, ciw->usesptiread ? 1 : 0))
		return 0;
	ret = 0;
	while (size > 0) {
		int track = cdtracknumber(&ciw->di.toc, sector);
		if (track < 0)
			return 0;
		got = false;
		if (!ciw->usesptiread && sectorsize == 2048 && ciw->trackmode[track] == 0) {
			if (read2048(ciw, sector) == 2048) {
				memcpy(data, p, 2048);
				sector++;
				data += sectorsize;
				ret += sectorsize;
				got = true;
			}
		}
		if (!got && !ciw->usesptiread) {
			int len = spti_read(ciw, unitnum, data, sector, sectorsize);
			if (len) {
				if (data) {
					memcpy(data, p, sectorsize);
					data += sectorsize;
					ret += sectorsize;
				}
				got = true;
			}
		}
		if (!got) {
			int dtotal = read2048(ciw, sector);
			if (dtotal != 2048)
				return ret;
			if (sectorsize >= 2352) {
				memset(data, 0, 16);
				memcpy(data + 16, p, 2048);
				encode_l2(data, sector + 150);
				if (sectorsize > 2352)
					memset(data + 2352, 0, sectorsize - 2352);
				sector++;
				data += sectorsize;
				ret += sectorsize;
			}
			else if (sectorsize == 2048) {
				memcpy(data, p, 2048);
				sector++;
				data += sectorsize;
				ret += sectorsize;
			}
			got = true;
		}
		sector++;
		size--;
	}
	return ret;
}

static int read_block_cda(struct cda_play* cda, int unitnum, uae_u8* data, int sector, int size, int sectorsize)
{
	return read_block(&ciw32[unitnum], unitnum, data, sector, size, sectorsize);
}

/* pause/unpause CD audio */
static int ioctl_command_pause(int unitnum, int paused)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return -1;
	int old = ciw->cda.cdda_paused;
	if ((paused && ciw->cda.cdda_play) || !paused)
		ciw->cda.cdda_paused = paused;
	return old;
}


/* stop CD audio */
static int ioctl_command_stop(int unitnum)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return 0;

	ciw_cdda_stop(&ciw->cda);

	return 1;
}

static uae_u32 ioctl_command_volume(int unitnum, uae_u16 volume_left, uae_u16 volume_right)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return -1;
	uae_u32 old = (ciw->cda.cdda_volume[1] << 16) | (ciw->cda.cdda_volume[0] << 0);
	ciw->cda.cdda_volume[0] = volume_left;
	ciw->cda.cdda_volume[1] = volume_right;
	return old;
}

/* play CD audio */
static int ioctl_command_play(int unitnum, int startlsn, int endlsn, int scan, play_status_callback statusfunc, play_subchannel_callback subfunc)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return 0;

	ciw->cda.di = &ciw->di;
	ciw->cda.cdda_play_finished = 0;
	ciw->cda.cdda_subfunc = subfunc;
	ciw->cda.cdda_statusfunc = statusfunc;
	ciw->cda.cdda_scan = scan > 0 ? 10 : (scan < 0 ? 10 : 0);
	ciw->cda.cdda_delay = ciw_cdda_setstate(&ciw->cda, -1, -1);
	ciw->cda.cdda_delay_frames = ciw_cdda_setstate(&ciw->cda, -2, -1);
	ciw_cdda_setstate(&ciw->cda, AUDIO_STATUS_NOT_SUPPORTED, -1);
	ciw->cda.read_block = read_block_cda;

	if (!open_createfile(ciw, 0)) {
		ciw_cdda_setstate(&ciw->cda, AUDIO_STATUS_PLAY_ERROR, -1);
		return 0;
	}
	if (!isaudiotrack(&ciw->di.toc, startlsn)) {
		ciw_cdda_setstate(&ciw->cda, AUDIO_STATUS_PLAY_ERROR, -1);
		return 0;
	}
	if (!ciw->cda.cdda_play) {
		uae_start_thread(_T("ioctl_cdda_play"), ciw_cdda_play, &ciw->cda, NULL);
	}
	ciw->cda.cdda_start = startlsn;
	ciw->cda.cdda_end = endlsn;
	ciw->cda.cd_last_pos = ciw->cda.cdda_start;
	ciw->cda.cdda_play++;

	return 1;
}

/* read qcode */
static int ioctl_command_qcode(int unitnum, uae_u8* buf, int sector, bool all)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return 0;

	uae_u8* p;
	int trk;
	cdrom_tocentry* toc = &ciw->cdromtoc;
	int pos;
	int msf;
	int start, end;
	int status;
	bool valid = false;
	bool regenerate = true;

	if (all)
		return 0;

	memset(buf, 0, SUBQ_SIZE);
	p = buf;

	status = AUDIO_STATUS_NO_STATUS;
	if (ciw->cda.cdda_play) {
		status = AUDIO_STATUS_IN_PROGRESS;
		if (ciw->cda.cdda_paused)
			status = AUDIO_STATUS_PAUSED;
	}
	else if (ciw->cda.cdda_play_finished) {
		status = AUDIO_STATUS_PLAY_COMPLETE;
	}

	p[1] = status;
	p[3] = 12;

	p = buf + 4;

	if (sector < 0)
		pos = ciw->cda.cd_last_pos;
	else
		pos = sector;

	if (!regenerate) {
		if (sector < 0 && ciw->cda.subcodevalid && ciw->cda.cdda_play) {
			uae_sem_wait(&ciw->cda.sub_sem2);
			uae_u8 subbuf[SUB_CHANNEL_SIZE];
			sub_deinterleave(ciw->cda.subcodebuf, subbuf);
			memcpy(p, subbuf + 12, 12);
			uae_sem_post(&ciw->cda.sub_sem2);
			valid = true;
		}
		if (!valid && sector >= 0) {
			unsigned int len;
			uae_sem_wait(&ciw->cda.sub_sem);
			struct cdrom_subchnl subchnl;
			subchnl.cdsc_format = CDROM_MSF;
			if (ioctl(ciw->fd, CDROMSUBCHNL, &subchnl) == -1) {
				perror("CDROMSUBCHNL ioctl failed");
			}
			uae_u8 subbuf[SUB_CHANNEL_SIZE];
			sub_deinterleave((uae_u8*)&subchnl, subbuf);
			uae_sem_post(&ciw->cda.sub_sem);
			memcpy(p, subbuf + 12, 12);
			valid = true;
		}
	}

	if (!valid) {
		start = end = 0;
		struct cdrom_tocentry tocentry;
		for (trk = 0; trk <= ciw->di.toc.last_track; trk++) {
			tocentry.cdte_track = trk;
			tocentry.cdte_format = CDROM_MSF;
			if (ioctl(ciw->fd, CDROMREADTOCENTRY, &tocentry) == -1) {
				perror("CDROMREADTOCENTRY ioctl failed");
				return 0;
			}
			start = msf2lsn((tocentry.cdte_addr.msf.minute << 16) | (tocentry.cdte_addr.msf.second << 8) | tocentry.cdte_addr.msf.frame);
			if (trk < ciw->di.toc.last_track) {
				tocentry.cdte_track = trk + 1;
				if (ioctl(ciw->fd, CDROMREADTOCENTRY, &tocentry) == -1) {
					perror("CDROMREADTOCENTRY ioctl failed");
					return 0;
				}
				end = msf2lsn((tocentry.cdte_addr.msf.minute << 16) | (tocentry.cdte_addr.msf.second << 8) | tocentry.cdte_addr.msf.frame);
			}
			else {
				end = INT_MAX;
			}
			if (pos < start)
				break;
			if (pos >= start && pos < end)
				break;
		}
		p[0] = (tocentry.cdte_ctrl << 4) | (tocentry.cdte_adr << 0);
		p[1] = tobcd(trk + 1);
		p[2] = tobcd(1);
		msf = lsn2msf(pos);
		tolongbcd(p + 7, msf);
		msf = lsn2msf(pos - start - 150);
		tolongbcd(p + 3, msf);
	}

	return 1;
}

static int ioctl_command_rawread(int unitnum, uae_u8* data, int sector, int size, int sectorsize, uae_u32 extra)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return 0;

	uae_u8* p = ciw->tempbuffer;
	int ret = 0;

	if (log_scsi)
		write_log(_T("IOCTL rawread unit=%d sector=%d blocksize=%d\n"), unitnum, sector, sectorsize);
	ciw_cdda_stop(&ciw->cda);
	gui_flicker_led(LED_CD, unitnum, LED_CD_ACTIVE);
	if (sectorsize > 0) {
		if (sectorsize != 2336 && sectorsize != 2352 && sectorsize != 2048 &&
			sectorsize != 2336 + 96 && sectorsize != 2352 + 96 && sectorsize != 2048 + 96)
			return 0;
		while (size-- > 0) {
			if (!read_block(ciw, unitnum, data, sector, 1, sectorsize))
				break;
			ciw->cda.cd_last_pos = sector;
			data += sectorsize;
			ret += sectorsize;
			sector++;
		}
	}
	else {
		uae_u8 sectortype = extra >> 16;
		uae_u8 cmd9 = extra >> 8;
		int sync = (cmd9 >> 7) & 1;
		int headercodes = (cmd9 >> 5) & 3;
		int userdata = (cmd9 >> 4) & 1;
		int edcecc = (cmd9 >> 3) & 1;
		int errorfield = (cmd9 >> 1) & 3;
		uae_u8 subs = extra & 7;
		if (subs != 0 && subs != 1 && subs != 2 && subs != 4)
			return -1;
		if (errorfield >= 3)
			return -1;
		uae_u8* d = data;

		if (isaudiotrack(&ciw->di.toc, sector)) {

			if (sectortype != 0 && sectortype != 1)
				return -2;

			for (int i = 0; i < size; i++) {
				uae_u8* odata = data;
				int blocksize = errorfield == 0 ? 2352 : (errorfield == 1 ? 2352 + 294 : 2352 + 296);
				int readblocksize = errorfield == 0 ? 2352 : 2352 + 296;

				if (!read_block(ciw, unitnum, NULL, sector, 1, readblocksize)) {
					return ret;
				}
				ciw->cda.cd_last_pos = sector;

				if (subs == 0) {
					memcpy(data, p, blocksize);
					data += blocksize;
				}
				else if (subs == 4) { // all, de-interleaved
					memcpy(data, p, blocksize);
					data += blocksize;
					sub_to_deinterleaved(p + readblocksize, data);
					data += SUB_CHANNEL_SIZE;
				}
				else if (subs == 2) { // q-only
					memcpy(data, p, blocksize);
					data += blocksize;
					uae_u8 subdata[SUB_CHANNEL_SIZE];
					sub_to_deinterleaved(p + readblocksize, subdata);
					memcpy(data, subdata + SUB_ENTRY_SIZE, SUB_ENTRY_SIZE);
					p += SUB_ENTRY_SIZE;
				}
				else if (subs == 1) { // all, interleaved
					memcpy(data, p, blocksize);
					memcpy(data + blocksize, p + readblocksize, SUB_CHANNEL_SIZE);
					data += blocksize + SUB_CHANNEL_SIZE;
				}
				ret += (int)(data - odata);
				sector++;
			}
		}


	}
	return ret;
}

static int ioctl_command_readwrite(int unitnum, int sector, int size, int do_write, uae_u8* data)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return 0;

	if (ciw->usesptiread)
		return ioctl_command_rawread(unitnum, data, sector, size, 2048, 0);

	ciw_cdda_stop(&ciw->cda);

	ssize_t dtotal;
	int cnt = 3;
	uae_u8* p = ciw->tempbuffer;
	int blocksize = ciw->di.bytespersector;

	if (!open_createfile(ciw, 0))
		return 0;
	ciw->cda.cd_last_pos = sector;
	while (cnt-- > 0) {
		off_t offset = (off_t)sector * ciw->di.bytespersector;
		gui_flicker_led(LED_CD, unitnum, LED_CD_ACTIVE);
		if (lseek(ciw->fd, offset, SEEK_SET) == (off_t)-1) {
			if (win32_error(ciw, unitnum, _T("SetFilePointer")) < 0)
				continue;
			return 0;
		}
		break;
	}
	while (size-- > 0) {
		gui_flicker_led(LED_CD, unitnum, LED_CD_ACTIVE);
		if (do_write) {
			if (data) {
				memcpy(p, data, blocksize);
				data += blocksize;
			}
			if (write(ciw->fd, p, blocksize) != blocksize) {
				int err;
				err = win32_error(ciw, unitnum, _T("write"));
				if (err < 0)
					continue;
				if (err == EROFS)
					return -1;
				return 0;
			}
		}
		else {
			dtotal = read(ciw->fd, p, blocksize);
			if (dtotal != blocksize) {
				if (win32_error(ciw, unitnum, _T("read")) < 0)
					continue;
				return 0;
			}
			if (dtotal == 0) {
				static int reported;
				/* ESS Mega (CDTV) "fake" data area returns zero bytes and no error.. */
				spti_read(ciw, unitnum, data, sector, 2048);
				if (reported++ < 100)
					write_log(_T("IOCTL unit %d, sector %d: read()==0. SPTI=%d\n"), unitnum, sector, errno);
				return 1;
			}
			if (data) {
				memcpy(data, p, blocksize);
				data += blocksize;
			}
		}
		gui_flicker_led(LED_CD, unitnum, LED_CD_ACTIVE);
	}
	return 1;

}

static int ioctl_command_write(int unitnum, uae_u8* data, int sector, int size)
{
	return ioctl_command_readwrite(unitnum, sector, size, 1, data);
}

static int ioctl_command_read(int unitnum, uae_u8* data, int sector, int size)
{
	return ioctl_command_readwrite(unitnum, sector, size, 0, data);
}

static int fetch_geometry(struct dev_info_ioctl* ciw, int unitnum, struct device_info* di)
{
    if (!open_createfile(ciw, 0))
        return 0;
    uae_sem_wait(&ciw->cda.sub_sem);

	int status = ioctl(ciw->fd, CDROM_DRIVE_STATUS, CDSL_NONE);
	if (status == -1)
	{
		perror("IOCTL: CDROM_DRIVE_STATUS");
	}
	if (status != CDS_DISC_OK)
	{
		ciw->changed = true;
		uae_sem_post(&ciw->cda.sub_sem);
		return 0;
	}

    uae_sem_post(&ciw->cda.sub_sem);
    // if (di) {
    //     di->cylinders = geom.cylinders;
    //     di->sectorspertrack = geom.sectors;
    //     di->trackspercylinder = geom.heads;
    //     di->bytespersector = 2048; // Typical CD-ROM sector size
    // }
    return 1;
}

static int ismedia(struct dev_info_ioctl* ciw, int unitnum)
{
	return fetch_geometry(ciw, unitnum, &ciw->di);
}

static int eject(int unitnum, bool eject)
{
    struct dev_info_ioctl* ciw = unitisopen(unitnum);

    if (!ciw)
        return 0;
    if (!unitisopen(unitnum))
        return 0;
    ciw_cdda_stop(&ciw->cda);
    if (!open_createfile(ciw, 0))
        return 0;
    int ret = 0;
    if (ioctl(ciw->fd, eject ? CDROMEJECT : CDROMCLOSETRAY, 0) < 0) {
        ret = 1;
    }
    return ret;
}

static int ioctl_command_toc2(int unitnum, struct cd_toc_head* tocout, bool hide_errors)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return 0;

	int len;
	int i;
	struct cd_toc_head* th = &ciw->di.toc;
	struct cd_toc* t = th->toc;
	int cnt = 3;
	memset(ciw->trackmode, 0xff, sizeof(ciw->trackmode));
	struct cdrom_tochdr tochdr;
	struct cdrom_tocentry tocentry;

	if (!unitisopen(unitnum))
		return 0;

	if (!open_createfile(ciw, 0))
		return 0;
	while (cnt-- > 0) {
		if (ioctl(ciw->fd, CDROMREADTOCHDR, &tochdr) == -1) {
			int err = errno;
			if (!hide_errors || (hide_errors && err == ENOMEDIUM)) {
				if (win32_error(ciw, unitnum, _T("CDROMREADTOCHDR")) < 0)
					continue;
			}
			return 0;
		}
		break;
	}

	memset(th, 0, sizeof(struct cd_toc_head));
	th->first_track = tochdr.cdth_trk0;
	th->last_track = tochdr.cdth_trk1;
	th->tracks = th->last_track - th->first_track + 1;
	th->points = th->tracks + 3;
	th->firstaddress = 0;
	th->lastaddress = msf2lsn((tocentry.cdte_addr.msf.minute << 16) | (tocentry.cdte_addr.msf.second << 8) |
		(tocentry.cdte_addr.msf.frame << 0));

	t->adr = 1;
	t->point = 0xa0;
	t->track = th->first_track;
	t++;

	th->first_track_offset = 1;
	for (i = 0; i < th->last_track; i++) {
		tocentry.cdte_track = i + 1;
		tocentry.cdte_format = CDROM_MSF;
		if (ioctl(ciw->fd, CDROMREADTOCENTRY, &tocentry) == -1) {
			return 0;
		}
		t->adr = tocentry.cdte_adr;
		t->control = tocentry.cdte_ctrl;
		t->paddress = msf2lsn((tocentry.cdte_addr.msf.minute << 16) | (tocentry.cdte_addr.msf.second << 8) |
			(tocentry.cdte_addr.msf.frame << 0));
		t->point = t->track = i + 1;
		t++;
	}

	th->last_track_offset = th->last_track;
	t->adr = 1;
	t->point = 0xa1;
	t->track = th->last_track;
	t->paddress = th->lastaddress;
	t++;

	t->adr = 1;
	t->point = 0xa2;
	t->paddress = th->lastaddress;
	t++;

	for (i = th->first_track_offset; i <= th->last_track_offset + 1; i++) {
		uae_u32 addr;
		uae_u32 msf;
		t = &th->toc[i];
		if (i <= th->last_track_offset) {
			write_log(_T("%2d: "), t->track);
			addr = t->paddress;
			msf = lsn2msf(addr);
		}
		else {
			write_log(_T("    "));
			addr = th->toc[th->last_track_offset + 2].paddress;
			msf = lsn2msf(addr);
		}
		write_log(_T("%7d %02d:%02d:%02d"),
			addr, (msf >> 16) & 0x7fff, (msf >> 8) & 0xff, (msf >> 0) & 0xff);
		if (i <= th->last_track_offset) {
			write_log(_T(" %s %x"),
				(t->control & 4) ? _T("DATA    ") : _T("CDA     "), t->control);
		}
		write_log(_T("\n"));
	}

	memcpy(tocout, th, sizeof(struct cd_toc_head));
	return 1;
}
static int ioctl_command_toc(int unitnum, struct cd_toc_head* tocout)
{
	return ioctl_command_toc2(unitnum, tocout, false);
}

#ifdef AMIBERRY
#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6
#endif

static void update_device_info(int unitnum)
{
	struct dev_info_ioctl* ciw = unitcheck(unitnum);
	if (!ciw)
		return;
	struct device_info* di = &ciw->di;
	di->bus = unitnum;
	di->target = 0;
	di->lun = 0;
	di->media_inserted = 0;
	di->bytespersector = 2048;
	strncpy(di->mediapath, ciw->drvletter, sizeof(ciw->drvletter) - 1);
	if (fetch_geometry(ciw, unitnum, di)) { // || ioctl_command_toc (unitnum))
		di->media_inserted = 1;
	}
	if (ciw->changed) {
		ioctl_command_toc2(unitnum, &di->toc, true);
		ciw->changed = false;
	}
	di->removable = ciw->type == DRIVE_CDROM ? 1 : 0;
	di->write_protected = ciw->type == DRIVE_CDROM ? 1 : 0;
	di->type = ciw->type == DRIVE_CDROM ? INQ_ROMD : INQ_DASD;
	di->unitnum = unitnum + 1;
	_tcscpy(di->label, ciw->drvlettername);
	di->backend = _T("IOCTL");
}

static void trim(TCHAR* s)
{
	while (s[0] != '\0' && s[_tcslen(s) - 1] == ' ')
		s[_tcslen(s) - 1] = 0;
}

/* open device level access to cd rom drive */
static int sys_cddev_open(struct dev_info_ioctl* ciw, int unitnum)
{
	ciw->cda.cdda_volume[0] = 0x7fff;
	ciw->cda.cdda_volume[1] = 0x7fff;
	ciw->tempbuffer = (unsigned char*)malloc(IOCTL_DATA_BUFFER);
	if (!ciw->tempbuffer) {
		write_log("IOCTL: Failed to allocate buffer\n");
		return 1;
	}

	memset(ciw->di.vendorid, 0, sizeof(ciw->di.vendorid));
	memset(ciw->di.productid, 0, sizeof(ciw->di.productid));
	memset(ciw->di.revision, 0, sizeof(ciw->di.revision));
	_tcscpy(ciw->di.vendorid, _T("UAE"));
	_stprintf(ciw->di.productid, _T("SCSI CD%d IMG"), unitnum);
	_tcscpy(ciw->di.revision, _T("0.2"));

	if (!open_createfile(ciw, 0)) {
		write_log("IOCTL: Failed to open '%s'\n", ciw->devname);
		goto error;
	}

	struct hd_driveid id;
	if (ioctl(ciw->fd, HDIO_GET_IDENTITY, &id) == 0) {
		strncpy(ciw->di.vendorid, (const char*)id.model, sizeof(ciw->di.vendorid) - 1);
		strncpy(ciw->di.productid, (const char*)id.serial_no, sizeof(ciw->di.productid) - 1);
		strncpy(ciw->di.revision, (const char*)id.fw_rev, sizeof(ciw->di.revision) - 1);
	}

	write_log(_T("IOCTL: device '%s' (%s/%s/%s) opened successfully (unit=%d,media=%d)\n"),
		ciw->devname, ciw->di.vendorid, ciw->di.productid, ciw->di.revision,
		unitnum, ciw->di.media_inserted);
	if (!_tcsicmp(ciw->di.vendorid, _T("iomega")) && !_tcsicmp(ciw->di.productid, _T("rrd"))) {
		write_log(_T("Device blacklisted\n"));
		goto error;
	}
	uae_sem_init(&ciw->cda.sub_sem, 0, 1);
	uae_sem_init(&ciw->cda.sub_sem2, 0, 1);
	ioctl_command_stop(unitnum);
	update_device_info(unitnum);
	ciw->open = true;
	return 0;

error:
	win32_error(ciw, unitnum, _T("CreateFile"));

	free(ciw->tempbuffer);
	ciw->tempbuffer = NULL;
	close(ciw->fd);
	ciw->fd = -1;
	return -1;
}

/* close device handle */
static void sys_cddev_close(struct dev_info_ioctl* ciw, int unitnum) {
	if (ciw->open == false)
		return;
	ciw_cdda_stop(&ciw->cda);
	close_createfile(ciw);
	free(ciw->tempbuffer);
	ciw->tempbuffer = NULL;
	uae_sem_destroy(&ciw->cda.sub_sem);
	uae_sem_destroy(&ciw->cda.sub_sem2);
	ciw->open = false;
	write_log(_T("IOCTL: device '%s' closed\n"), ciw->devname, unitnum);
}

static int open_device(int unitnum, const char* ident, int flags)
{
	struct dev_info_ioctl* ciw = NULL;
	if (ident && ident[0]) {
		for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
			ciw = &ciw32[i];
			if (unittable[i] == 0 && ciw->drvletter != 0) {
				if (!strcmp(ciw->drvlettername, ident)) {
					unittable[unitnum] = i + 1;
					if (sys_cddev_open(ciw, unitnum) == 0)
						return 1;
					unittable[unitnum] = 0;
					return 0;
				}
			}
		}
		return 0;
	}
	ciw = &ciw32[unitnum];
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		if (unittable[i] == unitnum + 1)
			return 0;
	}
	if (ciw->drvletter == 0)
		return 0;
	unittable[unitnum] = unitnum + 1;
	if (sys_cddev_open(ciw, unitnum) == 0)
		return 1;
	unittable[unitnum] = 0;
	blkdev_cd_change(unitnum, ciw->drvlettername);
	return 0;
}

static void close_device(int unitnum)
{
	struct dev_info_ioctl* ciw = unitcheck(unitnum);
	if (!ciw)
		return;
	sys_cddev_close(ciw, unitnum);
	blkdev_cd_change(unitnum, ciw->drvlettername);
	unittable[unitnum] = 0;
}

static int total_devices;

static void close_bus(void)
{
	if (!bus_open) {
		write_log(_T("IOCTL close_bus() when already closed!\n"));
		return;
	}
	total_devices = 0;
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		sys_cddev_close(&ciw32[i], i);
		memset(&ciw32[i], 0, sizeof(struct dev_info_ioctl));
		ciw32[i].fd = -1;
		unittable[i] = 0;
	}
	bus_open = 0;
	uae_sem_destroy(&play_sem);
	write_log(_T("IOCTL driver closed.\n"));
}

static int open_bus(int flags)
{
	if (bus_open) {
		write_log(_T("IOCTL open_bus() more than once!\n"));
		return 1;
	}
	total_devices = 0;
	auto cd_drives = get_cd_drives();
	if (!cd_drives.empty())
	{
		for (const auto& drive: cd_drives)
		{
			int fd = open(drive.c_str(), O_RDONLY | O_NONBLOCK);
			if (fd != -1) {
				struct stat st;
				if (fstat(fd, &st) == 0 && S_ISBLK(st.st_mode)) {
					strncpy(ciw32[total_devices].drvletter, drive.c_str(), sizeof(ciw32[total_devices].drvletter));
					strcpy(ciw32[total_devices].drvlettername, drive.c_str());
					ciw32[total_devices].type = st.st_rdev;
					ciw32[total_devices].di.bytespersector = 2048;
					strcpy(ciw32[total_devices].devname, drive.c_str());
					ciw32[total_devices].fd = fd;
					total_devices++;
				}
				close(fd);
			}
		}
	}
	bus_open = 1;
	uae_sem_init(&play_sem, 0, 1);
	write_log(_T("IOCTL driver open, %d devices.\n"), total_devices);
	return total_devices;
}

static int ioctl_ismedia(int unitnum, int quick)
{
	struct dev_info_ioctl* ciw = unitisopen(unitnum);
	if (!ciw)
		return -1;
	if (quick) {
		return ciw->di.media_inserted;
	}
	update_device_info(unitnum);
	return ismedia(ciw, unitnum);
}

static struct device_info* info_device(int unitnum, struct device_info* di, int quick, int session)
{
	struct dev_info_ioctl* ciw = unitcheck(unitnum);
	if (!ciw)
		return 0;
	if (!quick)
		update_device_info(unitnum);
	ciw->di.open = ciw->open;
	memcpy(di, &ciw->di, sizeof(struct device_info));
	return di;
}

// bool win32_ioctl_media_change(TCHAR driveletter, int insert)
// {
// 	for (int i = 0; i < total_devices; i++) {
// 		struct dev_info_ioctl* ciw = &ciw32[i];
// 		if (ciw->drvletter == driveletter && ciw->di.media_inserted != insert) {
// 			write_log(_T("IOCTL: media change %s %d\n"), ciw->drvlettername, insert);
// 			ciw->di.media_inserted = insert;
// 			ciw->changed = true;
// 			int unitnum = getunitnum(ciw);
// 			if (unitnum >= 0) {
// 				update_device_info(unitnum);
// 				scsi_do_disk_change(unitnum, insert, NULL);
// 				filesys_do_disk_change(unitnum, insert != 0);
// 				blkdev_cd_change(unitnum, ciw->drvlettername);
// 				return true;
// 			}
// 		}
// 	}
// 	return false;
// }

static int ioctl_scsiemu(int unitnum, uae_u8* cmd)
{
	uae_u8 c = cmd[0];
	if (c == 0x1b) {
		int mode = cmd[4] & 3;
		if (mode == 2)
			eject(unitnum, true);
		else if (mode == 3)
			eject(unitnum, false);
		return 1;
	}
	return -1;
}

struct device_functions devicefunc_scsi_ioctl = {
	_T("IOCTL"),
	open_bus, close_bus, open_device, close_device, info_device,
	0, 0, 0,
	ioctl_command_pause, ioctl_command_stop, ioctl_command_play, ioctl_command_volume, ioctl_command_qcode,
	ioctl_command_toc, ioctl_command_read, ioctl_command_rawread, ioctl_command_write,
	0, ioctl_ismedia, ioctl_scsiemu
};

#endif
