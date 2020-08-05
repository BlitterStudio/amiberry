#include "sysconfig.h"
#include "sysdeps.h"

#include "crc32.h"
#include "diskutil.h"

#define MFMMASK 0x55555555
static uae_u32 getmfmlong (uae_u16 * mbuf)
{
	return (uae_u32)(((*mbuf << 16) | *(mbuf + 1)) & MFMMASK);
}

#define FLOPPY_WRITE_LEN 6250

static int drive_write_adf_amigados (uae_u16 *mbuf, uae_u16 *mend, uae_u8 *writebuffer, uae_u8 *writebuffer_ok, int track, int *outsize)
{
	int i;
	uae_u32 odd, even, chksum, id, dlong;
	uae_u8 *secdata;
	uae_u8 secbuf[544];

	mend -= (4 + 16 + 8 + 512);
	*outsize = 11 * 512;
	for (;;) {
		int trackoffs;

		/* all sectors complete? */
		for (i = 0; i < 11; i++) {
			if (!writebuffer_ok[i])
				break;
		}
		if (i == 11)
			return 0;

		do {
			while (*mbuf++ != 0x4489) {
				if (mbuf >= mend) {
					write_log (_T("* track %d, unexpected end of data\n"), track);
					return 1;
				}
			}
		} while (*mbuf++ != 0x4489);

		odd = getmfmlong (mbuf);
		even = getmfmlong (mbuf + 2);
		mbuf += 4;
		id = (odd << 1) | even;

		trackoffs = (id & 0xff00) >> 8;
		if (trackoffs > 10) {
			write_log (_T("* track %d, corrupt sector number %d\n"), track, trackoffs);
			goto next;
		}
		/* this sector is already ok? */
		if (writebuffer_ok[trackoffs])
			goto next;

		chksum = odd ^ even;
		for (i = 0; i < 4; i++) {
			odd = getmfmlong (mbuf);
			even = getmfmlong (mbuf + 8);
			mbuf += 2;

			dlong = (odd << 1) | even;
			if (dlong) {
				write_log (_T("* track %d, sector %d header crc error\n"), track, trackoffs);
				goto next;
			}
			chksum ^= odd ^ even;
		} /* could check here if the label is nonstandard */
		mbuf += 8;
		odd = getmfmlong (mbuf);
		even = getmfmlong (mbuf + 2);
		mbuf += 4;
		if (((odd << 1) | even) != chksum || ((id & 0x00ff0000) >> 16) != (uae_u32)track) return 3;
		odd = getmfmlong (mbuf);
		even = getmfmlong (mbuf + 2);
		mbuf += 4;
		chksum = (odd << 1) | even;
		secdata = secbuf + 32;
		for (i = 0; i < 128; i++) {
			odd = getmfmlong (mbuf);
			even = getmfmlong (mbuf + 256);
			mbuf += 2;
			dlong = (odd << 1) | even;
			*secdata++ = (uae_u8)(dlong >> 24);
			*secdata++ = (uae_u8)(dlong >> 16);
			*secdata++ = (uae_u8)(dlong >> 8);
			*secdata++ = (uae_u8)dlong;
			chksum ^= odd ^ even;
		}
		mbuf += 256;
		if (chksum) {
			write_log (_T("* track %d, sector %d data crc error\n"), track, trackoffs);
			goto next;
		}
		memcpy (writebuffer + trackoffs * 512, secbuf + 32, 512);
		writebuffer_ok[trackoffs] = 0xff;
		continue;
next:
		mbuf += 8;
	}
}

/* search and align to 0x4489 WORDSYNC markers */
int isamigatrack(uae_u16 *amigamfmbuffer, uae_u8 *mfmdata, int len, uae_u8 *writebuffer, uae_u8 *writebuffer_ok, int track, int *outsize)
{
	uae_u16 *dst = amigamfmbuffer;
	int shift, syncshift, sync;
	uae_u32 l;
	uae_u16 w;

	*outsize = 11 * 512;
	len *= 8;
	sync = syncshift = shift = 0;
	while (len--) {
		l = (mfmdata[0] << 16) | (mfmdata[1] << 8) | (mfmdata[2] << 0);
		w = (uae_u16)(l >> (8 - shift));
		if (w == 0x4489) {
			sync = 1;
			syncshift = 0;
		}
		if (sync) {
			if (syncshift == 0) *dst++ = w;
			syncshift++;
			if (syncshift == 16) syncshift = 0;
		}
		shift++;
		if (shift == 8) {
			mfmdata++;
			shift = 0;
		}
	}
	if (sync)
		return drive_write_adf_amigados (amigamfmbuffer, dst, writebuffer, writebuffer_ok, track, outsize);
	return -1;
}

static uae_u16 getmfmword (uae_u16 *mbuf, int shift)
{
	return (mbuf[0] << shift) | (mbuf[1] >> (16 - shift));
}
static uae_u8 mfmdecode (uae_u16 **mfmp, int shift)
{
	uae_u16 mfm = getmfmword (*mfmp, shift);
	uae_u8 out = 0;
	int i;

	(*mfmp)++;
	mfm &= 0x5555;
	for (i = 0; i < 8; i++) {
		out >>= 1;
		if (mfm & 1)
			out |= 0x80;
		mfm >>= 2;
	}
	return out;
}

static int drive_write_adf_pc (uae_u16 *mbuf, uae_u16 *mend, uae_u8 *writebuffer, uae_u8 *writebuffer_ok, int track, int *outsecs)
{
	int sectors, shift, sector, i;
	uae_u8 mark;
	uae_u8 secbuf[3 + 1 + 512];
	uae_u16 crc;
	int mfmcount;

	secbuf[0] = secbuf[1] = secbuf[2] = 0xa1;
	secbuf[3] = 0xfb;

	sectors = 0;
	sector = -1;
	shift = 0;
	mend -= (4 + 16 + 8 + 512);
	for (;;) {
		*outsecs = sectors;

		mfmcount = 0;
		while (getmfmword (mbuf, shift) != 0x4489) {
			mfmcount++;
			if (mbuf >= mend) {
				if (sectors >= 1)
					return 0;
				write_log (_T("* track %d, unexpected end of data\n"), track);
				return 1;
			}
			shift++;
			if (shift == 16) {
				shift = 0;
				mbuf++;
			}
			if (sector >= 0 && mfmcount / 16 >= 43)
				sector = -1;
		}
		mfmcount = 0;
		while (getmfmword (mbuf, shift) == 0x4489) {
			mfmcount++;
			if (mbuf >= mend) {
				if (sectors >= 1)
					return 0;
				return 1;
			}
			mbuf++;
		}
		if (mfmcount < 3) // ignore if less than 3 sync markers
			continue;
		mark = mfmdecode (&mbuf, shift);
		if (mark == 0xfe) {
			uae_u8 tmp[8];
			uae_u8 cyl, head, size;

			cyl = mfmdecode (&mbuf, shift);
			head = mfmdecode (&mbuf, shift);
			sector = mfmdecode (&mbuf, shift);
			size = mfmdecode (&mbuf, shift);
			crc = (mfmdecode (&mbuf, shift) << 8) | mfmdecode (&mbuf, shift);

			tmp[0] = 0xa1; tmp[1] = 0xa1; tmp[2] = 0xa1; tmp[3] = mark;
			tmp[4] = cyl; tmp[5] = head; tmp[6] = sector; tmp[7] = size;

			// skip 28 bytes
			for (i = 0; i < 28; i++)
				mfmdecode (&mbuf, shift);

			if (get_crc16 (tmp, 8) != crc || cyl != track / 2 || head != (track & 1) || size != 2 || sector < 1 || sector > 20) {
				write_log (_T("PCDOS: track %d, corrupted sector header\n"), track);
				continue;
			}
			sector--;
			continue;
		}
		if (mark != 0xfb && mark != 0xfa) {
			write_log (_T("PCDOS: track %d: unknown address mark %02X\n"), track, mark);
			continue;
		}
		if (sector < 0) {
			write_log (_T("PCDOS: track %d: data mark without header\n"), track);
			continue;
		}
		for (i = 0; i < 512; i++)
			secbuf[i + 4] = mfmdecode (&mbuf, shift);
		crc = (mfmdecode (&mbuf, shift) << 8) | mfmdecode (&mbuf, shift);
		if (get_crc16 (secbuf, 3 + 1 + 512) != crc) {
			write_log (_T("PCDOS: track %d, sector %d data checksum error\n"),
				track, sector + 1);
			continue;
		}
		memcpy (writebuffer + sector * 512, secbuf + 4, 512);
		sectors++;
		sector = -1;
	}

}

int ispctrack(uae_u16 *amigamfmbuffer, uae_u8 *mfmdata, int len, uae_u8 *writebuffer, uae_u8 *writebuffer_ok, int track, int *outsize)
{
	int i, outsecs;
	for (i = 0; i < len / 2; i++)
		amigamfmbuffer[i] = mfmdata[i * 2 + 1] | (mfmdata[i * 2 + 0] << 8);
	i = drive_write_adf_pc (amigamfmbuffer, amigamfmbuffer + len / 2, writebuffer, writebuffer_ok, track, &outsecs);
	*outsize = outsecs * 512;
	if (*outsize < 9 * 512)
		*outsize = 9 * 512;
	return i ? -1 : 0;
}
