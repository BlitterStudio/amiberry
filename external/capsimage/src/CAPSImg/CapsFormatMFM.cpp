#include "stdafx.h"



// MFM format conversion
SDWORD __cdecl CAPSFormatDataToMFM(PVOID pformattrack, UDWORD flag)
{
	if (!pformattrack)
		return imgeGeneric;

	// structure revision is zero, unless newer one requested
	unsigned rev=0;
	if (flag & DI_LOCK_TYPE)
		rev=*(PUDWORD)pformattrack;

	// if unsupported type set the highest supported version
	if (rev > 0) {
		*(PUDWORD)pformattrack=0;
		return imgeUnsupportedType;
	}

	PCAPSFORMATTRACK pf=(PCAPSFORMATTRACK)pformattrack;

	// if buffer not set, return the required buffer size
	if (!pf->trackbuf || !pf->tracklen || !pf->buflen)
		return FmfmGetSize(pf);

	// track buffer should not exceed allocated memory
	if (pf->tracklen > pf->buflen)
		return imgeBufferShort;

	// calculate track size and check descriptors
	int res=FmfmGetSize(pf);

	// cancel if track definition error
	if (res != imgeOk)
		return res;

	// cancel if track size too short
	if (pf->tracklen < pf->bufreq)
		return imgeBufferShort;

	// error if data start position is invalid
	if (pf->startpos >= pf->tracklen)
		return imgeBadDataStart;

	// convert the data
	return FmfmConvert(pf);
}



// calculate minimum buffer size
int FmfmGetSize(PCAPSFORMATTRACK pf)
{
	// reset track size
	UDWORD len=0;
	pf->bufreq=0;

	// first gap size
	len+=pf->gapacnt;

	// cancel if invalid blocks
	if (pf->blockcnt && !pf->block)
		return imgeGeneric;

	// add length for each block
	for (int blk=0; blk < pf->blockcnt; blk++) {
		// get block
		PCAPSFORMATBLOCK pb=pf->block+blk;

		// add gap sizes
		len+=pb->gapacnt;
		len+=pb->gapbcnt;
		len+=pb->gapccnt;
		len+=pb->gapdcnt;

		// sum block data
		int sv=false;
		switch (pb->blocktype) {
			// index block; 3am, fc
			case cfrmbtIndex:
				len+=4;
				break;

			// data block; 3am, fe, 4hdr, 2crc, 3am, fb, data, 2crc
			case cfrmbtData:
				len+=16;
				len+=pb->sectorlen;
				sv=true;
				break;

			default:
				return imgeBadBlockType;
		}

		// if sector length used check its validity
		if (sv && FmfmSectorLength(pb->sectorlen)<0)
			return imgeBadBlockSize;
	}

	// set requested size in mfm
	pf->bufreq=len*2;

	return imgeOk;
}



// convert block data to mfm
int FmfmConvert(PCAPSFORMATTRACK pf)
{
	// reset size
	pf->size=0;

	// reset size counter and mfm state
	UDWORD state=(0)<<15 ^ 0xffff;

	// write index gap
	state=FmfmWriteDataByte(pf, state, pf->gapavalue, pf->gapacnt);

	// process all blocks
	for (int blk=0; blk < pf->blockcnt; blk++) {
		// get block
		PCAPSFORMATBLOCK pb=pf->block+blk;

		// write block data
		switch (pb->blocktype) {
			// index block; 3am, fc,gap
			case cfrmbtIndex:
				state=FmfmWriteBlockIndex(pf, state, pb);
				break;

			// data block; 3am, fe, 4hdr, 2crc, 3am, fb, data, 2crc
			case cfrmbtData:
				state=FmfmWriteBlockData(pf, state, pb);
				break;
		}
	}

	// write gap size before index
	int gapbcnt=(pf->tracklen-pf->size)/2;
	if (gapbcnt > 0)
		state=FmfmWriteDataByte(pf, state, pf->gapbvalue, gapbcnt);

	return imgeOk;
}

// write data bytes into mfm buffer
UDWORD FmfmWriteDataByte(PCAPSFORMATTRACK pf, UDWORD state, UDWORD value, int count)
{
	pf->size+=count*2;
	UDWORD pos=pf->startpos;

	// write mfm bytes
	while (count--) {
		UDWORD val=CDiskEncoding::mfmcode[value & 0xff] & state;
		state=(val & 1)<<15 ^ 0xffff;

		pf->trackbuf[pos++]=UBYTE(val>>8);
		if (pos >= pf->tracklen)
			pos=0;

		pf->trackbuf[pos++]=(UBYTE)val;
		if (pos >= pf->tracklen)
			pos=0;
	}

	pf->startpos=pos;

	return state;
}

// write mark bytes into mfm buffer
UDWORD FmfmWriteMarkByte(PCAPSFORMATTRACK pf, UDWORD state, UDWORD value, int count)
{
	pf->size+=count*2;
	UDWORD pos=pf->startpos;
	UDWORD val=value & 0xffff;
	state=(val & 1)<<15 ^ 0xffff;

	// write mfm bytes
	while (count--) {
		pf->trackbuf[pos++]=UBYTE(val>>8);
		if (pos >= pf->tracklen)
			pos=0;

		pf->trackbuf[pos++]=(UBYTE)val;
		if (pos >= pf->tracklen)
			pos=0;
	}

	pf->startpos=pos;

	return state;
}

// calculate crc
UWORD FmfmCrc(UWORD crc, UDWORD value, int count)
{
	value&=0xff;

	while (count--)
		crc=crctab_ccitt[value^(crc>>8)] ^ (crc << 8);

	return crc;
}

// calculate sector length value
int FmfmSectorLength(int value)
{
	switch (value) {
		case 128:
			return 0;

		case 256:
			return 1;

		case 512:
			return 2;

		case 1024:
			return 3;
	}

	return -1;
}

// write index block
UDWORD FmfmWriteBlockIndex(PCAPSFORMATTRACK pf, UDWORD state, PCAPSFORMATBLOCK pb)
{
	// gap before first am
	state=FmfmWriteDataByte(pf, state, pb->gapavalue, pb->gapacnt);

	// 3 am c2/5224
	state=FmfmWriteMarkByte(pf, state, 0x5224, 3);

	// fc
	state=FmfmWriteDataByte(pf, state, 0xfc, 1);

	// gap after first am
	state=FmfmWriteDataByte(pf, state, pb->gapbvalue, pb->gapbcnt);

	return state;
}

// write data block
UDWORD FmfmWriteBlockData(PCAPSFORMATTRACK pf, UDWORD state, PCAPSFORMATBLOCK pb)
{
	UWORD crc;

	// sector length
	int seclen=FmfmSectorLength(pb->sectorlen);

	// write sector ID
	// gap before first am
	state=FmfmWriteDataByte(pf, state, pb->gapavalue, pb->gapacnt);

	// 3 am a1/4489
	crc=~0;
	state=FmfmWriteMarkByte(pf, state, 0x4489, 3);
	crc=FmfmCrc(crc, 0xa1, 3);

	// fe
	state=FmfmWriteDataByte(pf, state, 0xfe, 1);
	crc=FmfmCrc(crc, 0xfe);

	// track#
	state=FmfmWriteDataByte(pf, state, pb->track, 1);
	crc=FmfmCrc(crc, pb->track);

	// side#
	state=FmfmWriteDataByte(pf, state, pb->side, 1);
	crc=FmfmCrc(crc, pb->side);

	// sector#
	state=FmfmWriteDataByte(pf, state, pb->sector, 1);
	crc=FmfmCrc(crc, pb->sector);

	// sector length
	state=FmfmWriteDataByte(pf, state, seclen, 1);
	crc=FmfmCrc(crc, seclen);

	// crc
	state=FmfmWriteDataByte(pf, state, crc>>8, 1);
	state=FmfmWriteDataByte(pf, state, crc, 1);

	// gap after first am
	state=FmfmWriteDataByte(pf, state, pb->gapbvalue, pb->gapbcnt);

	// write sector data
	// gap before second am
	state=FmfmWriteDataByte(pf, state, pb->gapcvalue, pb->gapccnt);

	// 3 am a1/4489
	crc=~0;
	state=FmfmWriteMarkByte(pf, state, 0x4489, 3);
	crc=FmfmCrc(crc, 0xa1, 3);

	// fb
	state=FmfmWriteDataByte(pf, state, 0xfb, 1);
	crc=FmfmCrc(crc, 0xfb);

	// data
	for (int i=0; i < pb->sectorlen; i++) {
		UDWORD data=pb->databuf ? pb->databuf[i] : pb->datavalue;
		state=FmfmWriteDataByte(pf, state, data, 1);
		crc=FmfmCrc(crc, data);
	}

	// crc
	state=FmfmWriteDataByte(pf, state, crc>>8, 1);
	state=FmfmWriteDataByte(pf, state, crc, 1);

	// gap after second am
	state=FmfmWriteDataByte(pf, state, pb->gapdvalue, pb->gapdcnt);

	return state;
}

