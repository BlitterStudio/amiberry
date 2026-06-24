#include "stdafx.h"



// decompress CT Raw dump format
int CCTRawCodec::DecompressDump(uint8_t *buf, size_t len)
{
	CapsPack cpk;
	PCAPSPACK pk;

	// free all buffers
	Free();

	// check dump header
	auto hlen = sizeof(CapsRaw);
	if (len < hlen)
		return imgeShort;
	wh.cr = *(PCAPSRAW)buf;
	Swap(&wh.cr, sizeof(CapsRaw));

	// check dump size
	auto tlen = wh.cr.time;
	auto rlen = wh.cr.raw;
	if (len < hlen + tlen + rlen)
		return imgeShort;

	// check density information
	uint8_t *denbuf = buf + hlen;
	pk = GetPackHeader(&cpk, denbuf, tlen);
	if (!pk)
		return imgeDensityHeader;

	// check track information
	uint8_t *trkbuf = denbuf + tlen;
	pk = GetPackHeader(&cpk, trkbuf, rlen);
	if (!pk)
		return imgeTrackHeader;

	int err;

	// decompress density information
	wh.cdbuf = denbuf;
	wh.cdlen = tlen;
	if (!(err = DecompressDensity(1)))
		err = DecompressDensity();
	wh.cdbuf = nullptr;
	if (err)
		return err;

	// decompress track data
	wh.ctbuf = trkbuf;
	wh.ctlen = rlen;
	if (!(err = DecompressTrack(1)))
		err = DecompressTrack();
	wh.ctbuf = nullptr;

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
	uint32_t *dst = DecompressDensity(wh.cdbuf, wh.cdlen);
	int res = imgeOk;

	if (verify) {
		// in verify mode get density data in target format
		Swap(dst, pk->usize);

		// check crc on uncompressed data
		if (pk->ucrc != CalcCRC((uint8_t *)dst, pk->usize))
			res = imgeDensityData;

		// free density data
		delete[] dst;
	} else {
		// in decompress mode save density data
		wh.timbuf = dst;
		wh.timlen = pk->usize >> 2;
	}

	return res;
}

// decompress CT Raw density information
uint32_t *CCTRawCodec::DecompressDensity(uint8_t *src, int slen, uint32_t *dst)
{
	CapsPack cpk;

	// get density header
	PCAPSPACK pk = GetPackHeader(&cpk, src, slen);
	if (!pk)
		return nullptr;

	// use caller supplied buffer or allocate a new one
	uint32_t *buf = dst;
	if (!buf && pk->usize)
		buf = new uint32_t[pk->usize >> 2];

	// buffer pointer to end of destination and source buffer
	uint32_t *mem = buf + (pk->usize >> 2);
	src += slen;

	// decode the data, see compressor for encoding
	while (mem > buf) {
		int size, ofs;
		uint8_t cb0 = *--src;

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
						uint32_t data;
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

			default:
				NODEFAULT;
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
PCAPSWH CCTRawCodec::DecompressTrack(PCAPSWH w, uint8_t *src, int slen, uint8_t *dst)
{
	CapsPack cpk;

	// get track header
	PCAPSPACK pk = GetPackHeader(&cpk, src, slen);
	if (!pk)
		return nullptr;

	// use caller supplied buffer or allocate a new one
	w->rawbuf = nullptr;
	FreeUncompressedTrack(w);
	w->rawlen = pk->usize;
	w->rawbuf = dst;
	if (!w->rawbuf && w->rawlen)
		w->rawbuf = new uint8_t[w->rawlen];

	// read track number
	w->ctmem = src + sizeof(CapsPack);
	w->trkcnt = ReadStream(w, 1);

	// read <track number> track sizes and calculate track buffer positions
	dst = w->rawbuf;
	for (int trk = 0; trk < w->trkcnt; trk++) {
		// find the track size - we need to do this to update the source stream even if the value is discarded later
		auto tsize = ReadStream(w, 2);

		// ensure that only the supported number of tracks get updated
		if (trk < CAPS_MTRS) {
			w->trklen[trk] = tsize;
			w->trkbuf[trk] = dst;
			dst += tsize;
		}
	}

	// limit the number of tracks
	if (w->trkcnt > CAPS_MTRS)
		w->trkcnt = CAPS_MTRS;

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
	uint8_t *src = w->ctmem;

	// destination buffer
	uint8_t *dst = w->trkbuf[w->txact];

	// end of destination buffer
	uint8_t *max = dst + w->trklen[w->txact];

	// first, stored track
	uint8_t *mem = w->txsrc;

	// decode the data, see compressor for encoding
	while (dst < max) {
		uint32_t size = *src++;

		if (size & 0x80) {
			// copy block
			// 3 bit shift
			int shift = size >> 4 & 7;

			// 12 bit size
			size = ((size & 0xf) << 8) | (*src++);

			// 16 bit offset
			uint32_t ofs = *src++;
			ofs = (ofs << 8) | (*src++);
			uint8_t *buf = mem + ofs;

			if (shift) {
				// bitshift copy
				ofs = *buf++;
				while (size--) {
					ofs = (ofs << 8) | (*buf++);
					*dst++ = (uint8_t)(ofs >> shift);
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
PCAPSPACK CCTRawCodec::GetPackHeader(PCAPSPACK cpk, const uint8_t *src, size_t slen)
{
	// check header size
	if (!src || slen < sizeof(CapsPack))
		return nullptr;

	// check signature
	if (memcmp(src, CAPS_IDPACK, sizeof(cpk->sign)))
		return nullptr;

	// copy header to result
	memcpy(cpk, src, sizeof(CapsPack));

	// change header crc to host format and check crc
	Swap(&cpk->hcrc, sizeof(cpk->hcrc));
	uint32_t hcrc = cpk->hcrc;
	cpk->hcrc = 0;
	if (hcrc != CalcCRC((uint8_t *)cpk, sizeof(CapsPack)))
		return nullptr;

	// change header to host format
	Swap((uint8_t *)(cpk)+sizeof(cpk->sign), sizeof(CapsPack) - sizeof(cpk->sign));

	// buffer length must match the size of header and compressed data size
	if (slen != sizeof(CapsPack) + cpk->csize)
		return nullptr;

	return cpk;
}

// read up to a 32 bit value from CT Raw stream, update stream position
uint32_t CCTRawCodec::ReadStream(PCAPSWH w, unsigned int size)
{
	uint32_t res = 0;

	for (; size; size--) {
		res <<= 8;
		res |= *w->ctmem++;
	}

	return res;
}
