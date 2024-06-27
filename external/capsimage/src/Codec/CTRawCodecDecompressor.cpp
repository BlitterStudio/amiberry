#include "stdafx.h"



// decompress CT Raw dump format
int CCTRawCodec::DecompressDump(PUBYTE buf, int len)
{
	CapsPack cpk;
	PCAPSPACK pk;

	// free all buffers
	Free();

	// check dump header
	int hlen = sizeof(CapsRaw);
	if (len < hlen)
		return imgeShort;
	wh.cr = *(PCAPSRAW)buf;
	Swap((PUDWORD)&wh.cr, sizeof(CapsRaw));

	// check dump size
	int tlen = wh.cr.time;
	int rlen = wh.cr.raw;
	if (len < hlen + tlen + rlen)
		return imgeShort;

	// check density information
	PUBYTE denbuf = buf + hlen;
	pk = GetPackHeader(&cpk, denbuf, tlen);
	if (!pk)
		return imgeDensityHeader;

	// check track information
	PUBYTE trkbuf = denbuf + tlen;
	pk = GetPackHeader(&cpk, trkbuf, rlen);
	if (!pk)
		return imgeTrackHeader;

	int err;

	// decompress density information
	wh.cdbuf = denbuf;
	wh.cdlen = tlen;
	if (!(err = DecompressDensity(1)))
		err = DecompressDensity();
	wh.cdbuf = NULL;
	if (err)
		return err;

	// decompress track data
	wh.ctbuf = trkbuf;
	wh.ctlen = rlen;
	if (!(err = DecompressTrack(1)))
		err = DecompressTrack();
	wh.ctbuf = NULL;

	return err;
}



// decompress or verify compressed CT Raw density information
int CCTRawCodec::DecompressDensity(int verify)
{
	CapsPack cpk;

	// in decompress mode free result buffer first
	if (!verify)
		FreeUncompressedDensity();

	// get density header
	PCAPSPACK pk = GetPackHeader(&cpk, wh.cdbuf, wh.cdlen);
	if (!pk)
		return imgeDensityHeader;

	// check crc on compressed data in verify mode
	if (verify)
		if (pk->ccrc != CalcCRC(wh.cdbuf + sizeof(CapsPack), pk->csize))
			return imgeDensityStream;

	// decompress density data
	PUDWORD dst = DecompressDensity(wh.cdbuf, wh.cdlen);
	int res = imgeOk;

	if (verify) {
		// in verify mode get density data in target format
		Swap(dst, pk->usize);

		// check crc on uncompressed data
		if (pk->ucrc != CalcCRC((PUBYTE)dst, pk->usize))
			res = imgeDensityData;

		// free density data
		delete[] dst;
	}	else {
		// in decompress mode save density data
		wh.timbuf = dst;
		wh.timlen = pk->usize >> 2;
	}

	return res;
}

// decompress CT Raw density information
PUDWORD CCTRawCodec::DecompressDensity(PUBYTE src, int slen, PUDWORD dst)
{
	CapsPack cpk;

	// get density header
	PCAPSPACK pk = GetPackHeader(&cpk, src, slen);
	if (!pk)
		return NULL;

	// use caller supplied buffer or allocate a new one
	PUDWORD buf = dst;
	if (!buf && pk->usize)
		buf = new UDWORD[pk->usize >> 2];

	// buffer pointer to end of destionation and source buffer
	PUDWORD mem = buf + (pk->usize >> 2);
	src += slen;

	// decode the data, see compressor for encoding
	while (mem > buf) {
		int size, ofs;
		UBYTE cb0 = *--src;

		switch (cb0 & 0x03) {
			// data block
			case 0x0:
				// 12 or 4 bit size
				if (cb0 & 0x08)
					size = ((cb0 << 4) & 0xf00) | (*--src);
				else
					size = (cb0 >> 4) + 1;
				if (cb0 & 0x04) {
					// 4 bytes per value
					while (size--) {
						UDWORD data;
						data = (*--src);
						data = (data << 8) | (*--src);
						data = (data << 8) | (*--src);
						data = (data << 8) | (*--src);
						*--mem = data;
					}
				} else {
					// 1 byte per value
					while (size--)
						*--mem = *--src;
				}
				continue;

			// copy block, 8 bit offset, 6 bit size
			case 0x1:
				size = (cb0 >> 2) + 1;
				ofs = (*--src) + 1;
				break;

			// copy block, 16 bit offset, 6 bit size
			case 0x2:
				size = (cb0 >> 2) + 1;
				ofs = (*--src);
				ofs = (ofs << 8) | (*--src);
				break;

				// copy block, 16 bit offset, 14 bit size
			case 0x3:
				size = ((cb0 << 6) & 0x3f00) | (*--src);
				ofs = (*--src);
				ofs = (ofs << 8) | (*--src);
				break;
		}

		// copy decoded data
		while (size--) {
			mem--;
			*mem = mem[ofs];
		}
	}

	return buf;
}



// decompress or verify compressed CT Raw track information
int CCTRawCodec::DecompressTrack(int verify)
{
	CapsPack cpk;
	CapsWH cwh;

	// in decompress mode free result buffer first
	if (!verify)
		FreeUncompressedTrack();

	// get track header
	PCAPSPACK pk = GetPackHeader(&cpk, wh.ctbuf, wh.ctlen);
	if (!pk)
		return imgeTrackHeader;

	// check crc on compressed data in verify mode
	if (verify)
		if (pk->ccrc != CalcCRC(wh.ctbuf + sizeof(CapsPack), pk->csize))
			return imgeTrackStream;

	// decompress track data
	PCAPSWH wb = DecompressTrack(&cwh, wh.ctbuf, wh.ctlen);
	int res = imgeOk;

	if (verify) {
		// in verify mode check crc on uncompressed data
		if (pk->ucrc != CalcCRC(wb->rawbuf, wb->rawlen))
			res = imgeTrackData;

		// free track data
		FreeUncompressedTrack(wb);
	} else {
		// in decompress mode save track data
		wh.rawbuf = wb->rawbuf;
		wh.rawlen = wb->rawlen;

		for (int trk = 0; trk < CAPS_MTRS; trk++) {
			wh.trkbuf[trk] = wb->trkbuf[trk];
			wh.trklen[trk] = wb->trklen[trk];
		}

		wh.trkcnt = wb->trkcnt;
	}

	return res;
}

// decompress CT Raw track information
PCAPSWH CCTRawCodec::DecompressTrack(PCAPSWH w, PUBYTE src, int slen, PUBYTE dst)
{
	CapsPack cpk;

	// get track header
	PCAPSPACK pk = GetPackHeader(&cpk, src, slen);
	if (!pk)
		return NULL;

	// use caller supplied buffer or allocate a new one
	w->rawbuf = NULL;
	FreeUncompressedTrack(w);
	w->rawlen = pk->usize;
	w->rawbuf = dst;
	if (!w->rawbuf && w->rawlen)
		w->rawbuf = new UBYTE[w->rawlen];

	// read track number
	w->ctmem = src + sizeof(CapsPack);
	w->trkcnt = CTR(w, 1);

	// read <track number> track sizes and calculate track buffer positions
	dst = w->rawbuf;
	for (int trk = 0; trk < w->trkcnt; trk++) {
		w->trklen[trk] = CTR(w, 2);
		w->trkbuf[trk] = dst;
		dst += w->trklen[trk];
	}

	// first track is stored
	if (w->trkcnt) {
		w->txsrc = w->trkbuf[0];
		w->txlen = w->trklen[0];
		memmove(w->txsrc, w->ctmem, w->txlen);
		w->ctmem += w->txlen;
	}

	// other tracks are compressed as deltas against the stored track
	for (w->txact = 1; w->txact < w->trkcnt; w->txact++)
		DecompressTrackData(w);

	return w;
}

// decompress CT Raw track data
void CCTRawCodec::DecompressTrackData(PCAPSWH w)
{
	// source stream
	PUBYTE src = w->ctmem;

	// destination buffer
	PUBYTE dst = w->trkbuf[w->txact];

	// end of destination buffer
	PUBYTE max = dst + w->trklen[w->txact];

	// first, stored track
	PUBYTE mem = w->txsrc;

	// decode the data, see compressor for encoding
	while (dst < max) {
		UDWORD size = *src++;

		if (size & 0x80) {
			// copy block
			// 3 bit shift
			int shift = size >> 4 & 7;

			// 12 bit size
			size = ((size & 0xf) << 8) | (*src++);

			// 16 bit offset
			UDWORD ofs = *src++;
			ofs = (ofs << 8) | (*src++);
			PUBYTE buf = mem + ofs;

			if (shift) {
				// bitshift copy
				ofs = *buf++;
				while (size--) {
					ofs = (ofs << 8) | (*buf++);
					*dst++ = (UBYTE)(ofs >> shift);
				}
			} else {
				// simple copy
				while (size--)
					*dst++ = *buf++;
			}
		} else {
			// data block, 15 bit size
			size = (size << 8) | (*src++);
			while (size--)
				*dst++ = *src++;
		}
	}

	// update the source stream position
	w->ctmem = src;
}



// get CT Raw pack format header in host format and test integrity
PCAPSPACK CCTRawCodec::GetPackHeader(PCAPSPACK cpk, PUBYTE src, int slen)
{
	// check header size
	if (!src || slen<sizeof(CapsPack))
		return NULL;

	// check signature
	if (memcmp(src, CAPS_IDPACK, sizeof(cpk->sign)))
		return NULL;

	// copy header to result
	memcpy(cpk, src, sizeof(CapsPack));

	// change header crc to host format and check crc
	Swap(&cpk->hcrc, sizeof(cpk->hcrc));
	UDWORD hcrc = cpk->hcrc;
	cpk->hcrc = 0;
	if (hcrc != CalcCRC((PUBYTE)cpk, sizeof(CapsPack)))
		return NULL;

	// change header to host format
	Swap(PUDWORD(PUBYTE(cpk) + sizeof(cpk->sign)), sizeof(CapsPack) - sizeof(cpk->sign));

	// buffer length must match the size of header and compressed data size
	if ((UDWORD)slen != sizeof(CapsPack) + cpk->csize)
		return NULL;

	return cpk;
}

// read up to a dword value from CT Raw stream, update stream position
UDWORD CCTRawCodec::CTR(PCAPSWH w, int size)
{
	UDWORD res = 0;

	for (; size; size--) {
		res <<= 8;
		res |= *w->ctmem++;
	}

	return res;
}
