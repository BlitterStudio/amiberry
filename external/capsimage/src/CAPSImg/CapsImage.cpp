#include "stdafx.h"

// true if the fxb tables are initialized
int CCapsImage::fb_init = 0;

// position of the first 0 bit in a byte value starting from bit position 0 to 7 where position#0 is bit#7
int8_t CCapsImage::f0b_table[8][256];

// position of the first 1 bit in a byte value starting from bit position 0 to 7 where position#0 is bit#7
int8_t CCapsImage::f1b_table[8][256];



// compare image helper
int CCapsImage::CompareImage()
{
	PDISKTRACKINFO pti=di.pdt;

	int res=imgeOk;

	// default track size
	pti->tracklen=pti->ci.trksize;

	// generate 1 revolution
	pti->trackcnt=1;

	// unformatted track ignored
	if (pti->ci.dentype == cpdenNoise)
		pti->trackcnt=0;

	pti->tracklen*=pti->trackcnt;

	// allocate and set buffer
	if (pti->tracklen) {
		pti->trackbuf=new UBYTE[pti->tracklen];
		memset(pti->trackbuf, 0, pti->tracklen);
	}

	// set track size and pointer
	pti->trackdata[0]=pti->trackbuf;
	pti->tracksize[0]=0;
	pti->trackstart[0]=0;

	// decoder start position is always 0
	pti->comppos=0;
	pti->sdpos=0;

	// decode blocks
	if (pti->trackcnt) {
		int eblk=pti->compeblk;
		if (eblk < 0)
			eblk=pti->ci.blkcnt;
		else
			eblk++;

		for (int blk=pti->compsblk; blk < eblk; blk++)
			if ((res=CompareBlock(blk)) != imgeOk)
				return res;
	}

	pti->tracksize[0]=pti->comppos;
	return res;
}

// IPF image block comparator
int CCapsImage::CompareBlock(unsigned blk)
{
	PDISKTRACKINFO pti=di.pdt;
	uint8_t *buf=di.data;

	// check block number requested
	if (blk>=pti->ci.blkcnt || !buf || !di.datacount)
		return imgeGeneric;

	UDWORD maxofs=pti->datasize;

	// check buffer overrun
	if ((blk+1)*sizeof(CapsBlock) > maxofs)
		return imgeShort;

	// read block data in native format
	CapsBlock cb;
	memcpy(&cb, buf+blk*sizeof(CapsBlock), sizeof(CapsBlock));
	CCapsLoader::Swap((PUDWORD)&cb, sizeof(CapsBlock));

	// check buffer overrun
	if (cb.dataoffset >= maxofs)
		return imgeShort;

	// block stream limit is either the next block or buffer end
	if (blk != pti->ci.blkcnt-1)
		maxofs=ReadValue(buf+(blk+1)*sizeof(CapsBlock)+offsetof(CapsBlock, dataoffset), sizeof(UDWORD));

	// check buffer overrun
	if (cb.dataoffset >= maxofs)
		return imgeShort;

	// check encoder type
	switch (cb.enctype) {
		case cpencMFM:
			break;

		default:
			return imgeIncompatible;
	}

	// decode stream
	int end=false;
	int pos=pti->comppos;
	for (UDWORD ofs=cb.dataoffset; ofs < maxofs; ) {
		int code=buf[ofs++];

		// read count
		int vc=code>>CAPS_SIZE_S;
		code&=CAPS_DATAMASK;
		UDWORD count;

		if (vc) {
			if (ofs+vc > maxofs)
				return imgeTrackData;

			count=ReadValue(buf+ofs, vc);
			ofs+=vc;
		} else
			count=0;

		// process data
		switch (code) {
			// stream end
			case cpdatEnd:
				// safety checks
				if (count)
					return imgeTrackData;
				end=true;
				break;

			case cpdatData:
				// safety checks
				if (!count)
					return imgeTrackData;
				if (ofs+count > maxofs)
					return imgeTrackData;

				// copy data if requested
				if (di.flag & DI_COMP_DATA) {
					memcpy(pti->trackbuf+pos, buf+ofs, count);
					pos+=count;
				}
				ofs+=count;
				break;

			case cpdatGap:
				// safety checks
				if (!count)
					return imgeTrackData;
				if (ofs+count > maxofs)
					return imgeTrackData;

				// skip gap data
				ofs+=count;
				break;

			case cpdatMark:
				// safety checks
				if (!count)
					return imgeTrackData;
				if (ofs+count > maxofs)
					return imgeTrackData;

				// copy raw data if requested
				if (di.flag & DI_COMP_MARK) {
					memcpy(pti->trackbuf+pos, buf+ofs, count);
					pos+=count;
				}
				ofs+=count;
				break;

			case cpdatRaw:
				// safety checks
				if (!count)
					return imgeTrackData;
				if (ofs+count > maxofs)
					return imgeTrackData;

				// copy raw data if requested
				if (di.flag & DI_COMP_RAW) {
					memcpy(pti->trackbuf+pos, buf+ofs, count);
					pos+=count;
				}
				ofs+=count;
				break;

			case cpdatFData:
				// safety checks
				if (!count)
					return imgeTrackData;

				// set flakey data to 0 (memory already cleared) if requested
				if (di.flag & DI_COMP_FDATA)
					pos+=count;
				break;

			default:
				return imgeTrackStream;
		}
	}

	// no cpdatEnd encountered, definitely a problem
	if (!end)
		return imgeTrackData;

	pti->comppos=pos;
	return imgeOk;
}



// decompress CT Raw helper
int CCapsImage::DecompressDump()
{
	// raw images can't be: compared, 16 bit aligned
	if (di.flag & (DI_LOCK_COMP | DI_LOCK_ALIGN))
		return imgeUnsupported;

	PDISKTRACKINFO pti = di.pdt;
	uint8_t *buf=di.data;

	CCTRawCodec ctr;

	// decompress data
	int res=ctr.DecompressDump(buf, pti->datasize);
	if (res == imgeOk) {
		ConvertDumpInfo(ctr.GetInfo());
	} else
		pti->type=dtitError;

	return res;
}

// convert information about decompressed CT Raw file to generic track data
void CCapsImage::ConvertDumpInfo(PCAPSWH wh)
{
	PDISKTRACKINFO pti = di.pdt;

	// use the number of revolutions that could be stored by pti at most or the number of revolutions sampled if less
	int maxrev = min(CAPS_MTRS, wh->trkcnt);
	pti->rawtrackcnt = maxrev;

	// save the total size of all tracks decoded; don't have to recalculate later
	pti->rawlen = wh->rawlen;

	// prevent deallocating the buffer with the track data
	wh->rawbuf = NULL;

	// longest track found
	int maxtracksize = 0;

	// copy the track buffer start positions and sizes for the decompressed stream
	for (int rev = 0; rev < maxrev; rev++) {
		pti->trackdata[rev] = wh->trkbuf[rev];
		pti->tracksize[rev] = wh->trklen[rev];

		// find the longest track
		if (pti->tracksize[rev] > maxtracksize)
			maxtracksize = pti->tracksize[rev];
	}

	// save the original decoded timing data length
	pti->rawtimebuf = wh->timbuf;
	pti->rawtimecnt = wh->timlen;

	// prevent deallocating the buffer with the timing data
	wh->timbuf = NULL;

	// allocate timing buffer the same size as the longest data track; +1 for alternate density
	pti->timebuf = new UDWORD[maxtracksize + 1];

	// calculate the sum of the timing samples
	double timesum = 0;
	for (int i = 0; i < pti->rawtimecnt; i++) {
		timesum += pti->rawtimebuf[i];
	}

	// normalize all timing values; 1000 is the default value
	// compensate for precision loss to integer values by using a remainder
	int targetsum = pti->rawtimecnt * 1000;
	int actsum = 0;
	double tconv = (double)targetsum / timesum;
	double rem = 0;
	for (int i = 0; i < pti->rawtimecnt; i++) {
		double vr = pti->rawtimebuf[i] * tconv + rem;
		UDWORD vi = (UDWORD)vr;
		rem = vr - vi;
		pti->rawtimebuf[i] = vi;
		actsum += vi;
	}

	// If there is still a difference between the targeted normalized values and the actual values
	// add the remainder to the last timing.
	// tdiff can't be more than the target since it represents the remaining sum of precision losses
	int tdiff = targetsum - actsum;
	if (tdiff > 0)
		pti->rawtimebuf[pti->rawtimecnt - 1] += tdiff;

	// generate 1 revolution of data as default, 5 for analyser
	int trackcnt = (di.flag & DI_LOCK_ANA) ? 5 : 1;

	// 5 revolutions if track update is not used
	if (!(di.flag & DI_LOCK_UPDATEFD))
		trackcnt = 5;

	// the number of revolutions can't be more than the number of track revolutions decoded
	if (trackcnt > maxrev)
		trackcnt = maxrev;

	// write track info
	pti->trackcnt = trackcnt;
	pti->overlap = -1;
	pti->overlapbit = -1;

	// reset random seed
	pti->wseed = 0x87654321;

	// request raw track updates/streaming
	pti->rawdump = 1;
	pti->rawupdate = 1;

	// initialize lookup tables used by the weak bit detector
	InitFirstBitTables();

	// identify and record weak bit areas
	FindWeakBits();

	// first image update to initialize streaming
	UpdateDump();
}

// update decoded track and timing for CT Raw dump format
int CCapsImage::UpdateDump()
{
	PDISKTRACKINFO pti = di.pdt;

	// get actual revolution number, restart after the last revolution
	int rev = dii.nextrev % pti->rawtrackcnt;

	// true if all the available revolutions should be returned
	int allrev = (pti->trackcnt == pti->rawtrackcnt);

	// if all revolutions used the used data must be revolution#0
	if (allrev)
		rev = 0;

	// save the real revolution number
	dii.realrev = rev;

	// set data buffer
	pti->trackbuf = pti->trackdata[rev];
	pti->tracklen = allrev ? pti->rawlen : pti->tracksize[rev];
	pti->trackbc = pti->tracklen << 3;
	pti->singletrackbc = pti->trackbc;

	// timing information is the same size as the current data, rev#0 for all revolutions
	pti->timecnt = pti->tracksize[rev];

	// the real size of the sampled timings
	int rawsize = pti->rawtimecnt;

	// the amount of timing to be copied track size or real size - whichever is shorter
	int tsize = min(rawsize, pti->timecnt);

	// copy the timing
	memcpy(pti->timebuf, pti->rawtimebuf, tsize*sizeof(UDWORD));

	// If sampled timing is shorter, fill the remaining area with 1000, default value
	// This does not change the average, since the previous data adds up to an average of 1000
	int pos;
	for (pos = tsize; pos < pti->timecnt; pos++) {
			pti->timebuf[pos] = 1000;
	}

	// reset alternate density
	pti->timebuf[pos] = 0;

	// density map conversion
	if (di.flag & DI_LOCK_DENALT)
		ConvertDensity(pti);

	UpdateImage(rev);

	return imgeOk;
}

// find weak bit areas
void CCapsImage::FindWeakBits()
{
	PDISKTRACKINFO pti = di.pdt;

	// true if all the available revolutions should be returned
	int allrev = (pti->trackcnt == pti->rawtrackcnt);

	// process all revolutions
	for (int rev = 0; rev < pti->rawtrackcnt; rev++) {
		// get the track data buffer and the size of the buffer
		PUBYTE trackbuf = pti->trackdata[rev];
		int tracklen = allrev ? pti->rawlen : pti->tracksize[rev];

		// byte offset in track buffer for the current bitcell
		int bofs = 0;

		// number of consecutive 0 bitcells since the last 1 bit
		int zcnt = 0;

		// start position of the area containing 0s
		int lastbp = 0;

		// bitcell position in the current data byte bitcell; position#0 is bit#7
		int actbit = 0;

		// process the entire track buffer
		while (bofs < tracklen) {
			// get actual byte from track buffer
			uint8_t bval = trackbuf[bofs];
			
			while (1) {
				// first 0 bit position
				int f0b;

				// if area is empty search for area starting with 0 bit value
				if (!zcnt) {
					// find the first 0 bit from the current position in the current byte
					f0b = f0b_table[actbit][bval];
					if (f0b == 8) {
						// 0 bit not found, continue with the next byte
						break;
					} else {
						// 0 bit found, set the current bit position
						actbit = f0b;

						// set the area start position
						lastbp = bofs << 3 | actbit;
					}
				} else {
					// area not empty, start from position#0
					f0b = 0;
				}

				// find the first 1 bit from the current position in the current byte
				int f1b = f1b_table[actbit][bval];

				// add the number of bits from the last position (8 if no 1 bit found)
				zcnt += f1b - f0b;

				if (f1b == 8) {
					// 1 bit not found, continue with the next byte
					break;
				} else {
					// start the next search for 0 from the current position
					actbit = f1b;
				}

				// add the area
				if (zcnt >= MIN_CTRAW_WEAK_SIZE && zcnt <= MAX_CTRAW_WEAK_SIZE)
					AddWeakBitArea(rev, lastbp, zcnt);

				// area empty
				zcnt = 0;
			}

			// continue with the next byte
			actbit = 0;
			bofs++;
		}

		// add the last area if exists (loop ended before it was added)
		if (zcnt >= MIN_CTRAW_WEAK_SIZE && zcnt <= MAX_CTRAW_WEAK_SIZE)
			AddWeakBitArea(rev, lastbp, zcnt);
	}
}

// add a weak bit are definition
void CCapsImage::AddWeakBitArea(int group, int bitpos, int size)
{
	DiskDataMark ddm;
	ddm.group = group;
	ddm.position = bitpos;
	ddm.size = size;
	AddFD(di.pdt, &ddm, 1, (CAPS_MTRS * DEF_CTRAW_FDALLOC));
}

// initialize the tables containing the first 0 and 1 bits in a byte
void CCapsImage::InitFirstBitTables()
{
	// stop if already initialized
	if (fb_init)
		return;

	// initialize only once
	fb_init = 1;

	// process all positions
	for (int startbit = 0; startbit < 8; startbit++) {
		// position#0 is bit#7...position#7 is bit#0
		int bitmask = 1 << (7-startbit);

		// process all values
		for (int value = 0; value < 256; value++) {

			// find the first 0 bit starting with the current position
			int bitpos = startbit;
			for (int bm = bitmask; bm; bm >>= 1, bitpos++) {
				if (!(value & bm))
					break;
			}

			// store the position; 8 means not found
			f0b_table[startbit][value] = bitpos;

			// find the first 1 bit starting with the current position
			bitpos = startbit;
			for (int bm = bitmask; bm; bm >>= 1, bitpos++) {
				if (value & bm)
					break;
			}

			// store the position; 8 means not found
			f1b_table[startbit][value] = bitpos;
		}
	}
}

