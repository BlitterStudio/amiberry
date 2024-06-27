#include "stdafx.h"



using namespace std;

CCapsImageStd::CCapsImageStd()
{
	InitSystem();
	Clear();
}

CCapsImageStd::~CCapsImageStd()
{
	FreeDecoder();
}

// lock and scan file
int CCapsImageStd::Lock(PCAPSFILE pcf)
{
	// release image data
	Unlock();

	// try to lock
	switch (loader.Lock(pcf)) {
		// file open error
		case CCapsLoader::ccidErrFile:
			return imgeOpen;

		// not a caps file
		case CCapsLoader::ccidErrType:
			return imgeType;

		// yes, caps file
		case CCapsLoader::ccidCaps:
			break;

		// unknown problem
		default:
			return imgeGeneric;
	}

	// store open mode
	dii.readonly=(pcf->flag & CFF_WRITE) ? 0 : 1;

	// scan whole file for information
	int res=ScanImage();
	if (res != imgeOk)
		return res;

	return CheckEncoder(dii.ci.encoder, dii.ci.encrev);
}

// release file
int CCapsImageStd::Unlock()
{
	Destroy();
	loader.Unlock();
	return imgeOk;
}

// read and decode caps track format
int CCapsImageStd::LoadTrack(PDISKTRACKINFO pti, UDWORD flag)
{
	// set lock flags
	di.pdt=pti;
	di.flag=flag;

	int res = imgeOk;

	// update the track image if already available
	if (pti->trackcnt) {
		switch (pti->type) {
			case dtitCapsDump:
				res = UpdateDump();
				break;

			case dtitCapsImage:
				res = UpdateImage(0);
				break;

			default:
				res = imgeGeneric;
				break;
		}

		return res;
	}

	// process stored track data or generate data
	if (pti->datasize) {
		// load data, check data header
		loader.SetPosition(pti->datapos);
		if (loader.ReadChunk() == CCapsLoader::ccidData) {
			// allocate data buffer if needed
			AllocDiskData(pti->datasize);
			di.datacount=pti->datasize;

			// load and decode data
			if (loader.ReadData(di.data) == pti->datasize) {
				switch (pti->type) {
					case dtitCapsDump:
						res=DecompressDump();
						break;

					case dtitCapsImage:
						res=DecodeImage();
						break;

					default:
						res=imgeGeneric;
						break;
				}
			} else
				res=imgeGeneric;
		} else
			res=imgeGeneric;
	} else {
		// generate data
		switch (pti->type) {
			case dtitCapsImage:
				di.datacount=0;
				res=DecodeImage();
				break;

			default:
				res=imgeGeneric;
				break;
		}
	}

	return res;
}

// scan file; collect all information available
int CCapsImageStd::ScanImage()
{
	vector <ScanInfo> si;
	si.reserve(DEF_SCANINFO);
	ScanInfo empty_si = { 0, 0 };
	DiskTrackInfo dt;

	// process complete file
	for (int run = 1; run;) {
		int pos = loader.GetPosition();
		int type = loader.ReadChunk();
		PCAPSCHUNK pc = loader.GetChunk();
		int did = 0;

		switch (type) {
			// end of file
			case CCapsLoader::ccidEof:
				run = 0;
				continue;

			// remember data position
			case CCapsLoader::ccidData:
				did = pc->cg.mod.data.did;
				if ((unsigned)did >= si.size())
					si.resize(did + 1, empty_si);
				si[did].data = pos;
				loader.SkipData();
				break;

			// remember track header position
			case CCapsLoader::ccidTrck:
				did = pc->cg.mod.trck.did;
				if ((unsigned)did >= si.size())
					si.resize(did + 1, empty_si);
				si[did].track = pos;
				break;

			// handle file/format errors
			case CCapsLoader::ccidErrFile:
			case CCapsLoader::ccidErrType:
			case CCapsLoader::ccidErrShort:
			case CCapsLoader::ccidErrHeader:
			case CCapsLoader::ccidErrData:
				return imgeGeneric;

			// image info
			case CCapsLoader::ccidInfo:
				dii.ci = pc->cg.mod.info;
				dii.civalid = 1;
				continue;

			// remember track image descriptor
			case CCapsLoader::ccidImge:
				did = pc->cg.mod.imge.did;
				if ((unsigned)did >= si.size())
					si.resize(did + 1, empty_si);
				si[did].track = pos;
				break;

			// ignore other data
			default:
				continue;
		}

		// find highest data id
		if (did > dii.didmax)
			dii.didmax = did;

		// if did found, process if both header and data available
		if (did && (unsigned)did < si.size()) {
			if (si[did].track && si[did].data) {
				// reset track info
				memset(&dt, 0, sizeof(dt));

				// save image position
				int actpos = loader.GetPosition();

				// create track info
				loader.SetPosition(si[did].track);
				type = loader.ReadChunk();
				pc = loader.GetChunk();

				// set track by type
				switch (type) {
					case CCapsLoader::ccidTrck:
						dt.type = dtitCapsDump;
						dt.cylinder = pc->cg.mod.trck.cyl;
						dt.head = pc->cg.mod.trck.head;
						dii.dmpcount++;
						break;

					case CCapsLoader::ccidImge:
						dt.type = dtitCapsImage;
						dt.cylinder = pc->cg.mod.imge.cylinder;
						dt.head = pc->cg.mod.imge.head;
						dt.sectorcnt = pc->cg.mod.imge.blkcnt;
						dt.ci = pc->cg.mod.imge;
						dii.relcount++;
						break;

					default:
						return imgeGeneric;
				}

				// remember file positions
				dt.headerpos = si[did].track;
				dt.datapos = si[did].data;

				// create data info
				loader.SetPosition(si[did].data);
				loader.ReadChunk();
				dt.datasize = loader.GetDataSize();

				// write info
				int res = AddTrack(&dt);
				if (res != imgeOk)
					return res;

				// save to image is not allowed
				dii.modimage = 1;

				// restore image position
				loader.SetPosition(actpos);
			}
		}
	}

	return imgeOk;
}

// update any image data that changes for each revolution
int CCapsImageStd::UpdateImage(int group)
{
	int res=imgeOk;

	// set track buffer information
	di.track=di.pdt->trackbuf;
	di.trackbc=di.pdt->trackbc;
	di.singletrackbc=di.pdt->singletrackbc;
	trackbuf.InitBitSize(di.track, di.trackbc);
	
	// stop if track buffer is invalid
	if (!di.track || !di.trackbc || !di.singletrackbc)
		return imgeOk;

	// don't change track data if updates are disabled
	if (di.flag & DI_LOCK_NOUPDATE)
		return imgeOk;

	// update weak bit areas
	if (di.pdt->fdpsize && (di.flag & DI_LOCK_UPDATEFD))
		res=UpdateWeakBit(group);

	return res;
}

// update all weak bits in the image
int CCapsImageStd::UpdateWeakBit(int group)
{
	// don't change track data if updates are disabled
	if (di.flag & DI_LOCK_NOUPDATE)
		return imgeOk;

	// get random seed
	uint32_t seed=di.pdt->wseed;

	// update all areas marked
	for (int pos=0; pos < di.pdt->fdpsize; pos++) {
		PDISKDATAMARK pd=di.pdt->fdp+pos;

		// skip markers that belong to a different group
		if (pd->group != group)
			continue;

		uint32_t bitpos=pd->position;
		int bitcnt=pd->size;

		// write generated data
		while (bitcnt > 0) {
			// simple random generator
			seed<<=1;
			if ((seed>>22 ^ seed) & DF_1)
				seed++;

			// write largest possible bitfield
			int writebc=(bitcnt >= maxwritelen) ? maxwritelen : bitcnt;

			// write bitfield
			trackbuf.WriteBitWrap(bitpos, seed, writebc);

			// next write position
			bitcnt-=writebc;
			bitpos+=writebc;

			// handle track buffer wrap around (disk rotation)
			if (bitpos >= di.trackbc)
				bitpos-=di.trackbc;
		}
	}

	// save random seed, so any new update on the same track would give new results
	di.pdt->wseed=seed;

	return imgeOk;
}

// change a few bits at the overlap position
void CCapsImageStd::UpdateOverlap()
{
	// don't change track data if updates are disabled
	if (di.flag & DI_LOCK_NOUPDATE)
		return;

	// stop if no bits to change
	int ovlsize=CIMG_OVERLAPBIT;
	if (ovlsize <= 0)
		return;

	// stop if overlap position is not available
	if (di.pdt->overlapbit < 0)
		return;

	uint32_t bitpos=di.pdt->overlapbit;

	// flip all bits at the overlap position for all tracks
	for (int trk=0; trk < di.pdt->trackcnt; trk++) {
		uint32_t value=trackbuf.ReadBitWrap(bitpos, ovlsize);
		value=~value;
		trackbuf.WriteBitWrap(bitpos, value, ovlsize);

		// next track
		bitpos+=di.singletrackbc;

		// handle track buffer wrap around (disk rotation)
		if (bitpos >= di.trackbc)
			bitpos-=di.trackbc;
	}
}

// image decoder
int CCapsImageStd::DecodeImage()
{
	// initialize disk and track data used by decoder
	int res=InitDecoder();

	// run supported encoders only
	if (res == imgeOk) {
		switch (dii.ci.encoder) {
			case CAPS_ENCODER:
			case SPS_ENCODER:
				res=ProcessImage();
				break;

			default:
				res=imgeIncompatible;
				break;
		}
	}

	// if any error destroy track buffers and supress later calls
	if (res != imgeOk) {
		FreeTrack(di.pdt, 1);
		di.pdt->type=dtitError;
	}

	return res;
}

// decode and encode complete track image
int CCapsImageStd::ProcessImage()
{
	// check whether the image is in a supported format
	int res=CheckEncoder(dii.ci.encoder, dii.ci.encrev);
	if (res != imgeOk)
		return res;

	PDISKTRACKINFO pti=di.pdt;

	// check density type
	if (pti->ci.dentype<=cpdenNA || pti->ci.dentype>=cpdenLast)
		return imgeIncompatible;

	// check signal/cell type
	if (pti->ci.sigtype<=cpsigNA || pti->ci.sigtype>=cpsigLast)
		return imgeIncompatible;

	// check process
	if (pti->ci.process)
		return imgeIncompatible;

	// clear track data
	FreeTrack(pti, 1);

	// generate comparable image if supported
	if (di.flag & DI_LOCK_COMP)
		return CompareImage();

	// forced indexing not allowed for varied cell density
	switch (pti->ci.dentype) {
		case cpdenCLAmiga:
		case cpdenCLAmiga2:
		case cpdenCLST:
		case cpdenSLAmiga:
		case cpdenSLAmiga2:
		case cpdenABAmiga:
		case cpdenABAmiga2:
			di.flag&=~DI_LOCK_INDEX;
			break;
	}

	// original track size calculated
	uint32_t trackbits=di.dsctrackbc;

	// generate 1 revolution of data as default, 5 for analyser
	int trackcnt=(di.flag & DI_LOCK_ANA) ? 5 : 1;

	// set unformatted track parameters
	if (pti->ci.dentype == cpdenNoise) {
		// 2 revolutions if multiple revolutions defined
		if (di.flag & DI_LOCK_NOISEREV)
			trackcnt=2;

		// don't generate unformatted data unless requested, set default size unless already defined
		if (!(di.flag & DI_LOCK_NOISE))
			trackcnt=0;
		else
			if (!trackbits)
				trackbits=100000;
	}

	// 5 revolutions for weak data, if track update is not used
	if ((pti->ci.flag & CAPS_IF_FLAKEY) && !(di.flag & DI_LOCK_UPDATEFD))
		trackcnt=5;

	// 16 bit aligned track size if requested
	if (di.flag & DI_LOCK_ALIGN) {
		if (trackbits & 15)
			trackbits+=16-(trackbits & 15);
	}

	// re-align track to 8 bit
	if (!(di.flag & DI_LOCK_TRKBIT)) {
		if (trackbits & 7)
			trackbits+=8-(trackbits & 7);
	}

	// size for all tracks
	uint32_t alltrackbits=trackbits*trackcnt;

	// get number of bytes required for track buffer
	int bufsize=trackbuf.CalculateByteSize(alltrackbits);
	uint8_t *tbuf;

	// allocate and clear track buffer
	if (bufsize) {
		tbuf=new uint8_t[bufsize];
		memset(tbuf, 0, bufsize);
	} else
		tbuf=NULL;

	// encoding start position, always on first track
	uint32_t startbit=di.dscstartbit;
	if (trackbits)
		startbit=startbit % trackbits;

	// start position is 0/index if indexing is forced
	if (di.flag & DI_LOCK_INDEX)
		startbit=0;

	// write track info
	pti->trackcnt=trackcnt;
	pti->trackbuf=tbuf;
	pti->tracklen=bufsize;
	pti->sdpos=startbit >> 3;
	pti->overlap=-1;
	pti->overlapbit=-1;
	pti->trackbc=alltrackbits;
	pti->singletrackbc=trackbits;
	pti->startbit=startbit;

	// reset random seed
	pti->wseed=0x87654321;

	// initialize sector information
	AllocTrackSI(pti);

	// set track buffer information
	di.track=tbuf;
	di.trackbc=alltrackbits;
	di.singletrackbc=trackbits;
	trackbuf.InitBitSize(di.track, di.trackbc);

	// additional track size
	uint32_t trackdiff=trackbits-di.dsctrackbc;

	// track gap split position unknown
	int gsvalid=0;
	uint32_t gspos=0;

	// process all revolutions
	uint32_t trksize=0, actpos=startbit;
	for (int trk=0; trk < trackcnt; trk++) {
		// create track size and pointer for each revolution
		uint32_t ofsact=trksize >> 3;
		pti->trackdata[trk]=tbuf+ofsact;
		pti->trackstart[trk]=ofsact;
		trksize+=trackbits;
		uint32_t ofsnext=trksize >> 3;
		pti->tracksize[trk]=ofsnext-ofsact;

		// decode/encode all blocks
		for (int blk=0; blk < di.blockcount; blk++) {
			uint32_t datasize=di.block[blk].blockbits;
			uint32_t gapsize=di.block[blk].gapbits;

			// add additional size to last, track gap
			if (blk == di.blockcount-1) {
				// trying to grow an empty gap area is not supported
				if (!gapsize && trackdiff)
					return imgeGeneric;

				gapsize+=trackdiff;
			}

			// write block
			res=ProcessBlock(blk, actpos, datasize, gapsize);
			if (res != imgeOk)
				return res;

			// if track gap has known split position, save it
			if (!trk && (blk == di.blockcount-1) && di.encgsvalid) {
				gsvalid=1;
				gspos=di.encgapsplit;
			}

			// next write position, handle track buffer wrap around (disk rotation)
			actpos+=datasize+gapsize;
			if (actpos >= di.trackbc)
				actpos-=di.trackbc;
		}

		// fix MFM encoding
		MFMFixup();
	}

	// sanity check; writing must end where it started
	if (actpos != startbit)
		return imgeGeneric;

	// set track overlap position
	if (gsvalid) {
		uint32_t ofs=gspos % trackbits;
		pti->overlapbit=ofs;

		if (di.flag & DI_LOCK_OVLBIT)
			pti->overlap=ofs;
		else
			pti->overlap=ofs >> 3;
	}

	// create noise track
	if (pti->ci.dentype==cpdenNoise && (di.flag & DI_LOCK_NOISE))
		GenerateNoiseTrack(pti);

	// create density map
	if ((res=DecodeDensity(pti, di.data, di.flag)) != imgeOk)
		return res;

	// update bits at the overlap position
	UpdateOverlap();

	// first image update to ensure weak bits etc are in place
	res=UpdateImage(0);

	return res;
}

// compare image is unsupported in this class
int CCapsImageStd::CompareImage()
{
	return imgeUnsupported;
}

// decompress dump is unsupported in this class
int CCapsImageStd::DecompressDump()
{
	return imgeUnsupported;
}

// update for CT Raw format is unsupported in this class
int CCapsImageStd::UpdateDump()
{
	return imgeUnsupported;
}



// track density decoder
int CCapsImageStd::DecodeDensity(PDISKTRACKINFO pti, PUBYTE buf, UDWORD flag)
{
	// generate density
	switch (pti->ci.dentype) {
		case cpdenNoise:
			if (flag & DI_LOCK_DENNOISE)
				GenerateNoiseDensity(pti);
			break;

		case cpdenAuto:
			if (flag & DI_LOCK_DENAUTO)
				GenerateAutoDensity(pti);
			break;

		case cpdenCLAmiga:
			if (flag & DI_LOCK_DENVAR)
				GenerateCLA(pti, buf);
			break;

		case cpdenCLAmiga2:
			if (flag & DI_LOCK_DENVAR)
				GenerateCLA2(pti, buf);
			break;

		case cpdenCLST:
			if (flag & DI_LOCK_DENVAR)
				GenerateCLST(pti, buf);
			break;

		case cpdenSLAmiga:
			if (flag & DI_LOCK_DENVAR)
				GenerateSLA(pti, buf);
			break;

		case cpdenSLAmiga2:
			if (flag & DI_LOCK_DENVAR)
				GenerateSLA2(pti, buf);
			break;

		case cpdenABAmiga:
			if (flag & DI_LOCK_DENVAR)
				GenerateABA(pti, buf);
			break;

		case cpdenABAmiga2:
			if (flag & DI_LOCK_DENVAR)
				GenerateABA2(pti, buf);
			break;
	}

	// density map conversion
	if (flag & DI_LOCK_DENALT)
		ConvertDensity(pti);

	return imgeOk;
}

// density converter from default mapping
int CCapsImageStd::ConvertDensity(PDISKTRACKINFO pti)
{
	// cancel if buffer not available
	if (!pti->timebuf || !pti->timecnt)
		return imgeOk;

	// sum fractions
	UDWORD sum=0;
	for (int pos=0; pos < pti->timecnt; pos++) {
		sum+=pti->timebuf[pos];
		pti->timebuf[pos]=sum;
	}

	return imgeOk;
}

// generate unformatted track data
int CCapsImageStd::GenerateNoiseTrack(PDISKTRACKINFO pti)
{
	UDWORD val=CIMG_MFMNOISE;

	for (int pos=0; pos < pti->tracklen; pos++) {
		pti->trackbuf[pos]=(UBYTE)val;
		val=val<<8|val>>24;
	}

	return imgeOk;
}

// generate unformatted cell density
int CCapsImageStd::GenerateNoiseDensity(PDISKTRACKINFO pti)
{
	// same size with track data
	if (pti->tracklen && pti->trackcnt)
		pti->timecnt=pti->tracklen/pti->trackcnt;
	else
		if (!(pti->timecnt=pti->ci.trksize))
			pti->timecnt=12500;

	pti->timebuf=new UDWORD[pti->timecnt+1];

	int pos;
	for (pos=0; pos < pti->timecnt; pos++) {
		UDWORD val=1000;

		if (pos & 512)
			val+=(pos % 99)+(pos&31);
		else
			val-=(pos % 121)-(pos&31);

		pti->timebuf[pos]=val;
	}

	pti->timebuf[pos]=0;

	return imgeOk;
}

// generate auto cell density
int CCapsImageStd::GenerateAutoDensity(PDISKTRACKINFO pti)
{
	// same size with track data
	if (pti->tracklen && pti->trackcnt)
		pti->timecnt=pti->tracklen/pti->trackcnt;
	else
		if (!(pti->timecnt=pti->ci.trksize))
			pti->timecnt=12500;

	pti->timebuf=new UDWORD[pti->timecnt+1];

	int pos;
	for (pos=0; pos < pti->timecnt; pos++) {
		pti->timebuf[pos]=1000;
	}

	pti->timebuf[pos]=0;

	return imgeOk;
}

// generate Copylock Amiga cell density
int CCapsImageStd::GenerateCLA(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int lg = trackbuf.CalculateByteSize(di.block[3].gapbits);
	int pos=pti->sdpos, cp;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		switch (blk) {
			case 4:
				for (cp=0-lg; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=55;
				}
				break;

			case 5:
				for (cp=0-lg; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=5;
				}
				break;

			case 6:
				for (cp=0-lg; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=45;
				}
				break;
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
		lg=gs;
	}

	return imgeOk;
}

// generate Copylock Amiga, new cell density
int CCapsImageStd::GenerateCLA2(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int lg = trackbuf.CalculateByteSize(di.block[di.blockcount - 1].gapbits);
	int pos=pti->sdpos, cp;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		switch (blk) {
			case 0:
				for (cp=0-lg; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=55;
				}
				break;

			case 1:
				for (cp=0-lg; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=5;
				}
				break;

			case 2:
				for (cp=0-lg; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=45;
				}
				break;
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
		lg=gs;
	}

	return imgeOk;
}

// generate Copylock ST cell density
int CCapsImageStd::GenerateCLST(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int pos=pti->sdpos, cp;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		switch (blk) {
			case 5:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=50;
				}
				break;
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
	}

	return imgeOk;
}

// generate Speedlock Amiga cell density
int CCapsImageStd::GenerateSLA(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int pos=pti->sdpos, cp;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		switch (blk) {
			case 1:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=100;
				}
				break;

			case 2:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=100;
				}
				break;
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
	}

	return imgeOk;
}

// generate Speedlock Amiga, old cell density
int CCapsImageStd::GenerateSLA2(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int pos=pti->sdpos, cp;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		switch (blk) {
			case 1:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=50;
				}
				break;
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
	}

	return imgeOk;
}

// generate Adam Brierley Amiga cell density
int CCapsImageStd::GenerateABA(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int pos=pti->sdpos, cp;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		switch (blk) {
			case 1:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=100;
				}
				break;

			case 2:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]+=50;
				}
				break;

			case 4:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=50;
				}
				break;

			case 5:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=100;
				}
				break;

			case 6:
				for (cp=0; cp < bs; cp++) {
					pti->timebuf[pos+cp]-=150;
				}
				break;
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
	}

	return imgeOk;
}

// generate Adam Brierley, density key Amiga cell density
int CCapsImageStd::GenerateABA2(PDISKTRACKINFO pti, PUBYTE buf)
{
	// set auto density
	GenerateAutoDensity(pti);

	// iterate block positions
	int pos=pti->sdpos, cp;
	UDWORD key=0;
	UDWORD mask=1;

	for (unsigned blk=0; blk < pti->ci.blkcnt; blk++) {
		int bs = trackbuf.CalculateByteSize(di.block[blk].blockbits);
		int gs = trackbuf.CalculateByteSize(di.block[blk].gapbits);
		int xs=bs+gs;

		if (!blk) {
			key=ReadValue(buf+blk*sizeof(CapsBlock)+offsetof(CapsBlock, gapvalue), sizeof(UDWORD));
		} else {
			int dns=(key & mask) ? -50 : 50;
			mask<<=1;

			for (cp=0; cp < bs; cp++) {
				int val=pti->timebuf[pos+cp];
				val+=dns;
				pti->timebuf[pos+cp]=val;
			}
		}

		pos+=xs;
		if (pos >= pti->timecnt)
			pos-=pti->timecnt;
	}

	return imgeOk;
}



// initialize encoder/decoder system
void CCapsImageStd::InitSystem()
{
	// set bitfield write length supported
	maxwritelen = MAX_BITBUFFER_LEN;

	// keep number of bits to read at maximum supported by streaming for raw encoding
	rawreadlen=MAX_STREAMBIT;

	// initialize mfm encoding tables
	CDiskEncoding::InitMFM(0x10000);

	// keep number of bits to read at maximum supported by mfm encoding
	mfmreadlen=CDiskEncoding::mfmcodebit;

	// mfm encoding table index mask
	mfmindexmask=(1 << mfmreadlen)-1;

	// the mfm encoding table assumes the last bit to be 0, correct encoding if it's 1
	// 1: 1-01 -> 1-01 unchanged after AND, msb already 0
	// 0: 1-10 -> 1-00 AND clears msb
	mfmmsbclear=(1 << ((mfmreadlen*2)-1))-1;
}

// clear work area
void CCapsImageStd::Clear()
{
	di.pdt=NULL;
	di.data=NULL;
	di.block=NULL;
	di.flag=0;
	di.track=NULL;
	di.trackbc=0;
	di.singletrackbc=0;
	di.dsctrackbc=0;
	di.dscdatabc=0;
	di.dscgapbc=0;
	di.dscstartbit=0;
	di.encbitpos=0;
	di.encwritebc=0;
	di.encgsvalid=0;
	di.encgapsplit=0;

	trackbuf.InitBitSize(di.track, di.trackbc);
	FreeDecoder();
}

// allocate disk data buffer
void CCapsImageStd::AllocDiskData(int maxsize)
{
	if (di.datasize < maxsize) {
		FreeDiskData();

		di.data=new uint8_t[maxsize];
		di.datasize=maxsize;
	}
}

// allocate image block buffer
void CCapsImageStd::AllocImageBlock(int maxsize)
{
	if (di.blocksize < maxsize) {
		FreeImageBlock();

		di.block=new ImageBlockInfo[maxsize];
		di.blocksize=maxsize;
	}
}

// free disk data buffer
void CCapsImageStd::FreeDiskData()
{
	di.datacount=0;
	di.datasize=0;

	delete [] di.data;
	di.data=NULL;
}

// free image block buffer
void CCapsImageStd::FreeImageBlock()
{
	di.blockcount=0;
	di.blocksize=0;

	delete [] di.block;
	di.block=NULL;
}

// free decoder data
void CCapsImageStd::FreeDecoder()
{
	FreeDiskData();
	FreeImageBlock();
}



// check encoder compatibility
int CCapsImageStd::CheckEncoder(int encoder, int revision)
{
	if (!dii.civalid)
		return imgeOk;

	int err=0;

	switch (encoder) {
		case CAPS_ENCODER:
			if (revision < 1 || revision > CAPS_ENCODER_REV)
				err=1;
			break;

		case SPS_ENCODER:
			if (revision < 1 || revision > SPS_ENCODER_REV)
				err=1;
			break;

		default:
			err=1;
			break;
	}

	return err ? imgeIncompatible : imgeOk;
}

// get block descriptor and convert to image block descriptor
int CCapsImageStd::GetBlock(PIMAGEBLOCKINFO pi, int blk)
{
	// check errors
	if (!pi)
		return imgeGeneric;

	// get ipf block descriptor, stop on any error
	CapsBlock cb;
	int res=GetBlock(&cb, blk);
	if (res != imgeOk)
		return res;

	// set image descriptor
	pi->blockbits=cb.blockbits;
	pi->gapbits=cb.gapbits;
	pi->enctype=cb.enctype;
	pi->flag=cb.flag;
	pi->gapvalue=cb.gapvalue;
	pi->dataoffset=cb.dataoffset;

	// CAPS encoder support: no gap streams, no flags, 2us cells
	if (dii.ci.encoder == CAPS_ENCODER) {
		pi->gapoffset=0;
		pi->celltype=cpbct2us;
		pi->flag=0;
	} else {
		pi->gapoffset=cb.bt.sps.gapoffset;
		pi->celltype=cb.bt.sps.celltype;
	}

	// reset encoding information
	pi->fdenc=ibieNA;
	pi->fdbitpos=0;

	return imgeOk;
}

// get block descriptor
int CCapsImageStd::GetBlock(PCAPSBLOCK pb, int blk)
{
	// check errors
	if (!di.data || !pb || blk<0 || blk>=di.blockcount)
		return imgeGeneric;

	// check buffer overrun
	if (int((blk+1)*sizeof(CapsBlock)) > di.pdt->datasize)
		return imgeShort;

	// read block descriptor and convert it to host native endiannes
	memcpy(pb, di.data+blk*sizeof(CapsBlock), sizeof(CapsBlock));
	CCapsLoader::Swap((PUDWORD)pb, sizeof(CapsBlock));

	return imgeOk;
}

// initialize decoder data
int CCapsImageStd::InitDecoder()
{
	// stop if image descriptor is not available
	if (!dii.civalid)
		return imgeIncompatible;

	// allocate and create block descriptors
	AllocImageBlock(di.pdt->ci.blkcnt);
	di.blockcount=di.pdt->ci.blkcnt;

	int blk;

	// initialize block descriptors from image data
	for (blk=0; blk < di.blockcount; blk++) {
		int res=GetBlock(di.block+blk, blk);
		if (res != imgeOk)
			return res;
	}

	// reset track info
	di.dsctrackbc=0;
	di.dscdatabc=0;
	di.dscgapbc=0;
	di.dscstartbit=0;

	// calculate data and gap sizes
	for (blk=0; blk < di.blockcount; blk++) {
		PIMAGEBLOCKINFO pi=di.block+blk;

		// only allow gap size of 1 byte or more
		if (pi->gapbits < 8)
			pi->gapbits=0;

		di.dscdatabc+=pi->blockbits;
		di.dscgapbc+=pi->gapbits;
	}

	// track size
	di.dsctrackbc=di.dscdatabc+di.dscgapbc;

	// set encoding start position, limit to track size
	if (di.dsctrackbc)
		di.dscstartbit=di.pdt->ci.startbit % di.dsctrackbc;

	return imgeOk;
}

// initialize stream decoding
int CCapsImageStd::InitStream(PIMAGESTREAMINFO pstr, int strtype, int blk)
{
	// check errors
	if (!pstr || blk<0 || blk>=di.blockcount)
		return imgeGeneric;

	// set parameters so all data is available in the stream descriptor
	pstr->strtype=strtype;
	pstr->actblock=blk;
	pstr->enctype=di.block[blk].enctype;

	// active encoding is set by the block descriptor, but can be changed later
	pstr->actenctype=pstr->enctype;

	int res;

	switch (strtype) {
		case isitData:
			res=InitDataStream(pstr);
			break;

		case isitGap0:
		case isitGap1:
			res=InitGapStream(pstr);
			break;

		default:
			res=imgeGeneric;
			break;
	}

	// stop if stream could not be initialized
	if (res != imgeOk)
		return res;

	// stop if stream is unreadable
	res=ResetStream(pstr);

	return res;
}

// initialize data stream decoding
int CCapsImageStd::InitDataStream(PIMAGESTREAMINFO pstr)
{
	// sample looping on zero data count is not allowed
	pstr->allowloop=0;

	PIMAGEBLOCKINFO pi=di.block+pstr->actblock;

	// set data size mode to bits or bytes according to the encoder flag
	pstr->sizemodebit=(pi->flag & CAPS_BF_DMB) ? 1 : 0;

	// stream start from block descriptor
	pstr->strstart=pi->dataoffset;

	// check buffer overrun for file limit
	if (pstr->strstart >= (uint32_t)di.pdt->datasize)
		return imgeShort;

	// stream ends at the file limit for the current chunk for last block, or at the start of the next block stream
	if (pstr->actblock == di.blockcount-1)
		pstr->strend=di.pdt->datasize;
	else
		pstr->strend=pi[1].dataoffset;

	// check buffer overrun for stream limit
	if (pstr->strstart >= pstr->strend)
		return imgeShort;

	// set stream pointer and maximum size
	pstr->strbase=di.data+pstr->strstart;
	pstr->strsize=pstr->strend-pstr->strstart;

	return imgeOk;
}

// initialize gap stream decoding
int CCapsImageStd::InitGapStream(PIMAGESTREAMINFO pstr)
{
	// sample looping on zero data count is allowed
	pstr->allowloop=1;

	PIMAGEBLOCKINFO pi=di.block+pstr->actblock;

	// always set data size mode to bits for a gap stream
	pstr->sizemodebit=1;

	// generate gap stream from gap value; 1 byte size, 8 bit pattern, gap value
	int align=(pstr->strtype == isitGap0) ? 0 : 1;
	int gflag=align ? CAPS_BF_GP1 : CAPS_BF_GP0;
	int es=1;
	int gc=0;
	pstr->gapstr[gc++]=cpgapData | es<<CAPS_SIZE_S;
	pstr->gapstr[gc++]=8;
	pstr->gapstr[gc++]=(uint8_t)pi->gapvalue;
	pstr->gapstr[gc++]=cpgapEnd;

	// use the generated gap stream if no gap streams are available at all
	if (!(pi->flag & (CAPS_BF_GP0 | CAPS_BF_GP1))) {
		pstr->strstart=0;
		pstr->strend=0;
		pstr->strbase=pstr->gapstr;
		pstr->strsize=gc;
		return imgeOk;
	}

	// disable gap stream if not available
	if (!(pi->flag & gflag)) {
		pstr->strstart=0;
		pstr->strend=0;
		pstr->strbase=NULL;
		pstr->strsize=0;
		return imgeOk;
	}

	// stream start from block descriptor
	pstr->strstart=pi->gapoffset;

	// check buffer overrun for file limit
	if (pstr->strstart >= (uint32_t)di.pdt->datasize)
		return imgeShort;

	// find the next gap stream stored
	int bls;
	for (bls=pstr->actblock+1; bls < di.blockcount; bls++) {
		if (di.block[bls].flag & (CAPS_BF_GP0 | CAPS_BF_GP1))
			break;
	}

	// stream ends at the beginning of the data streams if this is the last gap stream stored
	// if another gap streams follows, this one ends where the next one begins
	if (bls == di.blockcount)
		pstr->strend=di.block[0].dataoffset;
	else
		pstr->strend=di.block[bls].gapoffset;

	// check buffer overrun for stream limit
	if (pstr->strstart >= pstr->strend)
		return imgeShort;

	// set stream pointer and maximum size
	pstr->strbase=di.data+pstr->strstart;
	pstr->strsize=pstr->strend-pstr->strstart;

	// resize stream; find the end of stream#0; if stream#0 is not present gap stream starts with stream#1
	int semode=align && (pi->flag & CAPS_BF_GP0);
	int res=FindGapStreamEnd(pstr, semode);

	return res;
}

// parse current gap stream until gap end reached and next stream starts
int CCapsImageStd::FindGapStreamEnd(PIMAGESTREAMINFO pstr, int next)
{
	uint8_t *buf=di.data;
	int end=0;

	uint32_t ofs;
	for (ofs=pstr->strstart; !end && ofs < pstr->strend; ) {
		int code=buf[ofs++];

		// read code, sizeof(count), count
		int vc=code >> CAPS_SIZE_S;
		code&=CAPS_DATAMASK;
		uint32_t count;

		if (vc) {
			if (ofs+vc > pstr->strend)
				return imgeTrackData;

			count=ReadValue(buf+ofs, vc);
			ofs+=vc;
		} else
			count=0;

		switch (code) {
			// end of stream
			case cpgapEnd:
				end=1;
				break;

			// skip total bitcount setting, already at next element
			case cpgapCount:
				break;

			// add sample data size and go to next element
			case cpgapData:
				ofs += trackbuf.CalculateByteSize(count);
				break;

			// no other data is allowed
			default:
				return imgeTrackStream;
		}
	}

	// no cpgapEnd encountered, stream error
	if (!end)
		return imgeTrackData;

	// update stream start/end position
	if (next)
		pstr->strstart=ofs;
	else
		pstr->strend=ofs;

	// check buffer overrun for stream limit
	if (pstr->strstart >= pstr->strend)
		return imgeShort;

	// update stream pointer and maximum size
	pstr->strbase=di.data+pstr->strstart;
	pstr->strsize=pstr->strend-pstr->strstart;

	return imgeOk;
}

// reset stream processing
int CCapsImageStd::ResetStream(PIMAGESTREAMINFO pstr)
{
	// set first stream data element
	pstr->strofs=0;

	// no stream errors
	pstr->readresult=imgeOk;
	pstr->readend=0;
	pstr->readvalue=0;

	// reset sample and data element streaming
	pstr->setencmode=isiemRaw;
	pstr->streambc=0;
	pstr->samplebc=0;
	pstr->remstreambc=0;
	pstr->remsamplebc=0;
	pstr->sampleofs=0;
	pstr->samplemask=0;
	pstr->samplebase=0;

	pstr->prcbitpos=0;
	pstr->prcrembc=0;
	pstr->prcskipbc=0;
	pstr->prcencstate=1;
	pstr->prcwritebc=0;

	pstr->loopofs=0;
	pstr->loopsize=0;
	pstr->looptype=isiltNone;

	pstr->esfixbc=0;
	pstr->esloopbc=0;

	pstr->scenable=0;
	pstr->scofs=0;
	pstr->scmul=0;

	// stop if stream is disabled
	if (!pstr->strsize) {
		pstr->readend=1;
		return imgeOk;
	}

	// decode the first stream sample data, set correct encoding mode
	int res=GetSample(pstr);

	return res;
}

// initialize sample reading for a new sample, return true for any processing change or stream end
int CCapsImageStd::ReadSampleInit(PIMAGESTREAMINFO pstr)
{
	// signal on stream end
	if (pstr->readend)
		return 1;

	// support sample looping if looping is allowed and last sample size is undefined
	if (pstr->allowloop && !pstr->streambc) {
		// set countdown values same as the original sample size
		pstr->remstreambc=pstr->samplebc;
		pstr->remsamplebc=pstr->samplebc;

		// reset reading the actual sample data part
		pstr->sampleofs=0;
		pstr->samplemask=0x80;
	} else {
		// get current encoding mode
		int setencmode=pstr->setencmode;
		int actenctype=pstr->actenctype;

		// get next sample, signal on error
		if (GetSample(pstr) != imgeOk)
			return 1;

		// signal on stream end
		if (pstr->readend)
			return 1;

		// signal on any encoding change
		if (pstr->setencmode != setencmode)
			return 1;

		if (pstr->actenctype != actenctype)
			return 1;
	}

	// no processing change required
	return 0;
}

// read sample data from an ipf stream
int CCapsImageStd::ReadSample(PIMAGESTREAMINFO pstr, int maxbc)
{
	// number of bits read
	int actbc=0;

	// value read
	uint32_t readvalue=0;

	// process remaining bits from stream
	while (maxbc > 0) {
		// get new sample when needed
		if (!pstr->remstreambc)
			if (ReadSampleInit(pstr))
				break;

		// restore bitstream state
		uint32_t remstreambc=pstr->remstreambc;
		uint32_t remsamplebc=pstr->remsamplebc;

		// stop on invalid sample state
		if (!remsamplebc)
			break;

		// read next sample if stream is empty, e.g. loop size has been changed
		if (!remstreambc)
			continue;

		uint32_t sampleofs=pstr->sampleofs;
		uint32_t samplemask=pstr->samplemask;
		uint32_t sampledata=pstr->samplebase[sampleofs];

		// read remaining bits from sample
		while (maxbc > 0) {
			// add new bit
			readvalue<<=1;
			if (sampledata & samplemask)
				readvalue|=1;

			// bit added
			actbc++;
			maxbc--;

			// one less sample bit to process 
			if (!--remsamplebc) {
				// reset the sample data if all sample bits have been read
				remsamplebc=pstr->samplebc;
				sampleofs=0;
				samplemask=0x80;
				sampledata=pstr->samplebase[sampleofs];
			} else {
				// set next bit position to read
				if (!(samplemask>>=1)) {
					sampleofs++;
					samplemask=0x80;
					sampledata=pstr->samplebase[sampleofs];
				}
			}

			// stop reading the current sample if complete data size has been read
			if (!--remstreambc)
				break;
		}

		// save bitstream state
		pstr->remstreambc=remstreambc;
		pstr->remsamplebc=remsamplebc;
		pstr->sampleofs=sampleofs;
		pstr->samplemask=samplemask;
	}

	// store value read
	pstr->readvalue=readvalue;

	return actbc;
}

// get next data element sample from the ipf stream
int CCapsImageStd::GetSample(PIMAGESTREAMINFO pstr)
{
	// reset reading the actual sample data part
	pstr->sampleofs=0;
	pstr->samplemask=0x80;

	// undefined bitcount
	pstr->streambc=0;
	pstr->remstreambc=0;

	int res;

	// stream encoding specific ipf stream processing
	switch (pstr->actenctype) {
		// physical encodings supported
		case cpencMFM:
			// default to encoded stream
			pstr->setencmode=isiemType;

			if (pstr->strtype == isitData)
				res=GetSampleData(pstr);
			else
				res=GetSampleGap(pstr);
			break;

		// raw should only be used for testing track data, not a supported release type
		case cpencRaw:
			// default to unencoded stream
			pstr->setencmode=isiemRaw;

			if (pstr->strtype == isitData)
				res=GetSampleRaw(pstr);
			else
				res=GetSampleGap(pstr);
			break;

		default:
			res=imgeIncompatible;
			break;
	}

	// set loop point, stop streaming on any error encountered
	if (res == imgeOk)
		GetLoop(pstr);
	else {
		pstr->readresult=res;
		pstr->readend=1;
	}

	return res;
}

// get next data element sample from an ipf raw stream
int CCapsImageStd::GetSampleRaw(PIMAGESTREAMINFO pstr)
{
	// check current offset in ipf encoding
	uint32_t ofs=pstr->strofs;

	if (ofs >= pstr->strsize)
		return imgeTrackData;

	// get stream code and sizeof(count)
	uint8_t *buf=pstr->strbase;
	int code=buf[ofs++];

	int vc=code >> CAPS_SIZE_S;
	code&=CAPS_DATAMASK;
	uint32_t count, bitcount;

	// read the value of the count parameter
	if (vc) {
		if (ofs+vc > pstr->strsize)
			return imgeTrackData;

		count=ReadValue(buf+ofs, vc);
		ofs+=vc;
	} else
		count=0;

	switch (code) {
		// stream finished
		case cpdatEnd:
			if (count)
				return imgeTrackData;

			pstr->readend=1;
			bitcount=0;
			break;

		// raw data sample, size in bytes
		case cpdatRaw:
			if (!count)
				return imgeTrackData;

			if (ofs+count > pstr->strsize)
				return imgeTrackData;

			bitcount=count << 3;
			break;

		// any other data type is an error
		default:
			return imgeTrackStream;
	}

	// next offset in ipf stream after the current sample
	pstr->strofs=ofs+count;

	// position of the sample data, left justified, starting at bit#7
	pstr->samplebase=buf+ofs;

	// set all sample counters to a simple countdown for the whole sample
	pstr->samplebc=bitcount;
	pstr->remstreambc=bitcount;
	pstr->remsamplebc=bitcount;

	return imgeOk;
}

// get next data element sample from an ipf gap stream
int CCapsImageStd::GetSampleGap(PIMAGESTREAMINFO pstr)
{
	// get current ipf encoding
	uint32_t ofs=pstr->strofs;
	uint8_t *buf=pstr->strbase;

	uint32_t bitcount, bytecount;

	while (1) {
		// check offset to be within stream boundaries
		if (ofs >= pstr->strsize)
			return imgeTrackData;

		// get stream code and sizeof(count)
		int code=buf[ofs++];

		int vc=code >> CAPS_SIZE_S;
		code&=CAPS_DATAMASK;
		uint32_t count;

		// read the value of the count parameter
		if (vc) {
			if (ofs+vc > pstr->strsize)
				return imgeTrackData;

			count=ReadValue(buf+ofs, vc);
			ofs+=vc;
		} else
			count=0;

		// support data size in bits or bytes
		if (pstr->sizemodebit) {
			bitcount=count;
			bytecount = trackbuf.CalculateByteSize(bitcount);
		} else {
			bytecount=count;
			bitcount=bytecount << 3;
		}

		switch (code) {
			// stream finished
			case cpgapEnd:
				if (bitcount)
					return imgeTrackData;

				pstr->readend=1;
				break;

			// set total sample size
			case cpgapCount:
				pstr->streambc=bitcount;
				continue;

			// data sample
			case cpgapData:
				if (!bitcount)
					return imgeTrackData;

				if (ofs+bytecount > pstr->strsize)
					return imgeTrackData;
				break;

			// any other data type is an error
			default:
				return imgeTrackStream;
		}

		// stop processing after complete data element or stream end
		break;
	}

	// next offset in ipf stream after the current sample
	pstr->strofs=ofs+bytecount;

	// position of the sample data, left justified, starting at bit#7
	pstr->samplebase=buf+ofs;

	// set all sample counters to a simple countdown for the whole sample
	pstr->samplebc=bitcount;
	pstr->remsamplebc=bitcount;

	// set total data size to sample size if undefined, or use the defined data size
	pstr->remstreambc=pstr->streambc ? pstr->streambc : bitcount;

	return imgeOk;
}

// get next data element sample from an ipf data stream
int CCapsImageStd::GetSampleData(PIMAGESTREAMINFO pstr)
{
	// get current ipf encoding
	uint32_t ofs=pstr->strofs;
	uint8_t *buf=pstr->strbase;

	uint32_t bitcount, bytecount;

	while (1) {
		// check offset to be within stream boundaries
		if (ofs >= pstr->strsize)
			return imgeTrackData;

		// get stream code and sizeof(count)
		int code=buf[ofs++];

		int vc=code >> CAPS_SIZE_S;
		code&=CAPS_DATAMASK;
		uint32_t count;

		// read the value of the count parameter
		if (vc) {
			if (ofs+vc > pstr->strsize)
				return imgeTrackData;

			count=ReadValue(buf+ofs, vc);
			ofs+=vc;
		} else
			count=0;

		// support data size in bits or bytes
		if (pstr->sizemodebit) {
			bitcount=count;
			bytecount = trackbuf.CalculateByteSize(bitcount);
		} else {
			bytecount=count;
			bitcount=bytecount << 3;
		}

		switch (code) {
			// stream finished
			case cpdatEnd:
				if (bitcount)
					return imgeTrackData;

				pstr->readend=1;
				break;

			// mark sample
			case cpdatMark:
				// switch to unencoded stream
				pstr->setencmode=isiemRaw;

			// data sample use default, encoded stream
			case cpdatData:
			case cpdatGap:
				if (!bitcount)
					return imgeTrackData;

				if (ofs+bytecount > pstr->strsize)
					return imgeTrackData;
				break;

			// weak bits
			case cpdatFData:
				pstr->setencmode=isiemWeak;
				break;

			// any other data type is an error
			default:
				return imgeTrackStream;
		}

		// stop processing after complete data element or stream end
		break;
	}

	// handle normal or weak data
	if (pstr->setencmode != isiemWeak) {
		// next offset in ipf stream after the current sample
		pstr->strofs=ofs+bytecount;

		// position of the sample data, left justified, starting at bit#7
		pstr->samplebase=buf+ofs;

		// set all sample counters to a simple countdown for the whole sample
		pstr->samplebc=bitcount;
		pstr->remsamplebc=bitcount;
	} else {
		// next offset in ipf stream, no sample present for weak data
		pstr->strofs=ofs;

		// position of the weak sample data, left justified, starting at bit#7
		pstr->samplebase=pstr->weakdata;

		// set all sample counters to a simple countdown for a 8 bit sample
		pstr->samplebc=8;
		pstr->remsamplebc=8;

		// set weak data
		pstr->weakdata[0]=0;
	}

	// set total data size to sample size if undefined, or use the defined data size
	pstr->remstreambc=pstr->streambc ? pstr->streambc : bitcount;

	return imgeOk;
}

// read ipf stream and encode to physical recording
int CCapsImageStd::ProcessStream(PIMAGESTREAMINFO pstr, uint32_t bitpos, int maxbc, int skipbc, int encnew)
{
	// set stream process state
	pstr->prcbitpos=bitpos;
	pstr->prcrembc=maxbc;
	pstr->prcskipbc=skipbc;
	pstr->prcencstate=encnew;
	pstr->prcwritebc=0;

	// process remaining bits
	while (pstr->prcrembc > 0) {
		// stop if ipf stream is over
		if (pstr->readend)
			break;

		// process by currently selected encoding mode
		switch (pstr->setencmode) {
			// raw, no encoding
			case isiemRaw:
				ProcessStreamRaw(pstr);
				break;

			// encoding specified by the data encoding type selected
			case isiemType:
				switch (pstr->actenctype) {
					// generic MFM
					case cpencMFM:
						ProcessStreamMFM(pstr);
						break;

					// raw data
					case cpencRaw:
						ProcessStreamRaw(pstr);
						break;

					default:
						return imgeIncompatible;
				}
				break;

			// weak bits
			case isiemWeak:
				ProcessStreamWeak(pstr);
				break;

			default:
				return imgeGeneric;
		}
	}

	return pstr->readresult;
}

// read ipf stream and write as raw track data
void CCapsImageStd::ProcessStreamRaw(PIMAGESTREAMINFO pstr)
{
	// get process state
	uint32_t bitpos=pstr->prcbitpos;
	int maxbc=pstr->prcrembc;
	int skipbc=pstr->prcskipbc;
	int actbc=0;

	// process remaining bits
	while (maxbc > 0) {
		// read the ipf stream
		int readbc=ReadSample(pstr, rawreadlen);

		// number of bits not read/missing
		int diffbc=rawreadlen-readbc;

		// write track data if anything was read
		if (readbc > 0) {
			// number of bits to be written
			int writebc=readbc;

			// skip bits if needed and write bitfield
			if (writebc > skipbc) {
				writebc-=skipbc;
				skipbc=0;

				// get value read
				uint32_t value=pstr->readvalue;

				// if less bits to write than available, right align field to bit#0
				if (writebc > maxbc) {
					value >>= (writebc-maxbc);
					writebc=maxbc;
				}

				trackbuf.WriteBitWrap(bitpos, value, writebc);

				// next write position
				actbc+=writebc;
				maxbc-=writebc;
				bitpos+=writebc;

				// handle track buffer wrap around (disk rotation)
				if (bitpos >= di.trackbc)
					bitpos-=di.trackbc;
			} else
				skipbc-=writebc;
		}

		// stop processing if encoding method changed or stream stopped 
		if (diffbc)
			break;
	}

	// set the type of the very first data
	if (!pstr->prcwritebc && pstr->prcencstate && actbc)
		di.block[pstr->actblock].fdenc=ibieRaw;

	// save process state
	pstr->prcbitpos=bitpos;
	pstr->prcrembc=maxbc;
	pstr->prcskipbc=skipbc;
	pstr->prcwritebc+=actbc;
}

// read ipf stream and write as mfm track data
void CCapsImageStd::ProcessStreamMFM(PIMAGESTREAMINFO pstr)
{
	// get process state
	uint32_t bitpos=pstr->prcbitpos;
	int maxbc=pstr->prcrembc;
	int skipbc=pstr->prcskipbc;
	int actbc=0;

	// ignore last bit written if encoding the first bit and new encoding, use last bit otherwise
	uint32_t lv;
	if (!pstr->prcwritebc && pstr->prcencstate)
		lv=0;
	else {
		uint32_t bp=(bitpos ? bitpos : di.trackbc)-1;
		lv = trackbuf.ReadBit(bp);
	}

	// process remaining bits
	while (maxbc > 0) {
		// read the ipf stream
		int readbc=ReadSample(pstr, mfmreadlen);

		// number of bits not read/missing
		int diffbc=mfmreadlen-readbc;

		// write track data if anything was read
		if (readbc > 0) {
			// number of mfm encoded bits to be written
			int writebc=readbc << 1;

			// skip bits if needed and write bitfield
			if (writebc > skipbc) {
				writebc-=skipbc;
				skipbc=0;

				// calculate the number of missing bits for indexing the mfm code table
				// index size-number of bits to be encoded to reach the target encoded length
				int encodebc=mfmreadlen-((writebc+1) >> 1);

				// get value read
				uint32_t value=pstr->readvalue;

				// add 0 bits for any missing data bits; get correct table index
				value <<= encodebc;

				// convert to mfm
				value=CDiskEncoding::mfmcode[value & mfmindexmask];

				// fix mfm encoding if last bit written is 1
				if (lv & 1)
					value &= mfmmsbclear;

				// right align all valid bits to bit#0; size doubled since value is in mfm now
				value >>= (encodebc << 1);

				// if less bits to write than available, right align field to bit#0
				if (writebc > maxbc) {
					value >>= (writebc-maxbc);
					writebc=maxbc;
				}

				// save last value writen
				lv=value;

				// write value as bitfield
				trackbuf.WriteBitWrap(bitpos, value, writebc);

				// next write position
				actbc+=writebc;
				maxbc-=writebc;
				bitpos+=writebc;

				// handle track buffer wrap around (disk rotation)
				if (bitpos >= di.trackbc)
					bitpos-=di.trackbc;
			} else
				skipbc-=writebc;
		}

		// stop processing if encoding method changed or stream stopped 
		if (diffbc)
			break;
	}

	// set the type of the very first data
	if (!pstr->prcwritebc && pstr->prcencstate && actbc)
		di.block[pstr->actblock].fdenc=ibieMFM;

	// save process state
	pstr->prcbitpos=bitpos;
	pstr->prcrembc=maxbc;
	pstr->prcskipbc=skipbc;
	pstr->prcwritebc+=actbc;
}

// read ipf stream and write as weak track data
void CCapsImageStd::ProcessStreamWeak(PIMAGESTREAMINFO pstr)
{
	// get process state
	uint32_t bitpos=pstr->prcbitpos;
	int maxbc=pstr->prcrembc;
	int skipbc=pstr->prcskipbc;
	int actbc=0;
		
	// process remaining bits
	if (maxbc > 0) {
		// get weak field size without encoding
		int writebc=pstr->remstreambc;

		// skip reading the sample associated with this field and get the next sample
		ReadSampleInit(pstr);

		// calculate field size after encoding
		switch (pstr->actenctype) {
			case cpencMFM:
				writebc <<= 1;
				break;
		}

		// skip bits if needed and write bitfield
		if (writebc > skipbc) {
			writebc-=skipbc;
			skipbc=0;

			// limit to available bit size
			if (writebc > maxbc)
				writebc=maxbc;

			// mark weak data, use group#0 for IPF image
			DiskDataMark ddm;
			ddm.group = 0;
			ddm.position=bitpos;
			ddm.size=writebc;
			AddFD(di.pdt, &ddm, 1, DEF_FDALLOC);

			// clear field
			trackbuf.ClearBitWrap(bitpos, writebc);

			// next write position
			actbc+=writebc;
			maxbc-=writebc;
			bitpos+=writebc;

			// handle track buffer wrap around (disk rotation)
			if (bitpos >= di.trackbc)
				bitpos-=di.trackbc;
		} else
			skipbc-=writebc;
	}

	// set the type of the very first data
	if (!pstr->prcwritebc && pstr->prcencstate && actbc)
		di.block[pstr->actblock].fdenc=ibieWeak;

	// save process state
	pstr->prcbitpos=bitpos;
	pstr->prcrembc=maxbc;
	pstr->prcskipbc=skipbc;
	pstr->prcwritebc+=actbc;
}

// calculate the complete stream size encoded
int CCapsImageStd::CalculateStreamSize(PIMAGESTREAMINFO pstr)
{
	// find loop point and loop type
	int res=FindLoopPoint(pstr);
	if (res != imgeOk)
		return res;

	// use a copy of the stream to parse data
	ImageStreamInfo isi=*pstr;

	// allbc: size without looping
	// fixbc: fixed bits
	// loopbc: loop bits
	int fixbc=0, loopbc=0;

	// sum all bit counts
	while (!isi.readend) {
		switch (isi.looptype) {
			// no loop
			case isiltNone:
				fixbc+=GetEncodedSize(&isi, isi.remstreambc);
				break;

			// auto-generated or stream loop
			case isiltAuto:
			case isiltStream:
				fixbc+=GetEncodedSize(&isi, isi.remstreambc);
				if (isi.strofs == isi.loopofs)
					loopbc+=GetEncodedSize(&isi, isi.samplebc);
				break;

			default:
				return imgeGeneric;
		}

		// get next sample
		if (GetSample(&isi) != imgeOk)
			return imgeGeneric;
	}

	// save results
	pstr->esfixbc=fixbc;
	pstr->esloopbc=loopbc;

	return imgeOk;
}

// calculate data size after physical encoding
int CCapsImageStd::GetEncodedSize(PIMAGESTREAMINFO pstr, int bitcnt)
{
	// process by currently selected and active encoding mode
	switch (pstr->setencmode) {
		case isiemType:
		case isiemWeak:
			switch (pstr->actenctype) {
				case cpencMFM:
					bitcnt <<= 1;
					break;
			}
			break;
	}

	return bitcnt;
}

// find the loop point in a stream and check for errors
int CCapsImageStd::FindLoopPoint(PIMAGESTREAMINFO pstr)
{
	// no loop point if sample looping is not allowed
	if (!pstr->allowloop) {
		pstr->loopofs=0;
		pstr->loopsize=0;
		pstr->looptype=isiltNone;
		return imgeOk;
	}

	// use a copy of the stream to parse data
	ImageStreamInfo isi=*pstr;

	// no loop point
	uint32_t loopofs=0;
	int loopsize=0, loopfound=0, cnt=0;

	while (!isi.readend) {
		uint32_t ofs=isi.strofs;
		int bc=isi.samplebc;

		switch (isi.strtype) {
			// loop point is the last data in forward gap stream; any data after a loop is invalid
			case isitGap0:
				if (loopfound)
					return imgeGeneric;

				loopofs=ofs;
				loopsize=bc;
				break;

			// loop point is the first data in backward gap stream; any loop after first data is invalid
			case isitGap1:
				if (!cnt) {
					loopofs=ofs;
					loopsize=bc;
				} else
					if (!isi.streambc)
						return imgeGeneric;
				break;
		}

		// only allow 1 loop
		if (!isi.streambc) {
			if (loopfound)
				return imgeGeneric;

			loopfound++;
		}

		cnt++;

		// get next sample
		if (GetSample(&isi) != imgeOk)
			return imgeGeneric;
	}

	// set loop point information
	pstr->loopofs=loopofs;
	pstr->loopsize=loopsize;

	if (loopsize)
		pstr->looptype=loopfound ? isiltStream : isiltAuto;
	else
		pstr->looptype=isiltNone;

	return imgeOk;
}

// encode a complete block
int CCapsImageStd::ProcessBlock(int blk, uint32_t bitpos, int datasize, int gapsize)
{
	int res;

	// set encoding parameters
	di.encbitpos=bitpos;
	di.encwritebc=0;
	di.encgsvalid=0;
	di.encgapsplit=0;

	// check parameters
	if (blk<0 || blk>=di.blockcount || datasize<0 || gapsize<0)
		return imgeGeneric;

	if (blk >= di.pdt->sipsize)
		return imgeGeneric;

	// set encoding information
	di.block[blk].fdenc=ibieNA;
	di.block[blk].fdbitpos=bitpos;

	PDISKSECTORINFO si=di.pdt->sip+blk;
	si->descdatasize=di.block[blk].blockbits;
	si->descgapsize=di.block[blk].gapbits;
	si->celltype=di.block[blk].celltype;
	si->enctype=di.block[blk].enctype;

	// create block data
	si->datastart=di.encbitpos;
	si->datasize=datasize;
	res=ProcessBlockData(blk, datasize);
	if (res != imgeOk)
		return res;

	// create gap data
	si->gapstart=di.encbitpos;
	si->gapsize=gapsize;
	res=ProcessBlockGap(blk, gapsize);
	if (res != imgeOk)
		return res;

	return imgeOk;
}

// encode block data
int CCapsImageStd::ProcessBlockData(int blk, int datasize)
{
	// stop if no data area
	if (!datasize)
		return imgeOk;

	int res;
	ImageStreamInfo dsi;

	// initialize the data stream
	res=InitStream(&dsi, isitData, blk);
	if (res != imgeOk)
		return res;

	// encode block data
	res=ProcessStream(&dsi, di.encbitpos, datasize, 0, !di.encwritebc);
	if (res != imgeOk)
		return res;

	// error if the requested data size was not filled
	if (dsi.prcwritebc != datasize)
		return imgeGeneric;

	// update bit position and bits written
	di.encbitpos=dsi.prcbitpos;
	di.encwritebc+=dsi.prcwritebc;

	return imgeOk;
}

// encode gap data
int CCapsImageStd::ProcessBlockGap(int blk, int gapsize)
{
	// stop if no gap area
	if (!gapsize)
		return imgeOk;

	int res;
	ImageStreamInfo gsi0, gsi1;

	// initialize the forward gap stream
	res=InitStream(&gsi0, isitGap0, blk);
	if (res != imgeOk)
		return res;

	res=CalculateStreamSize(&gsi0);
	if (res != imgeOk)
		return res;

	// initialize the backward gap stream
	res=InitStream(&gsi1, isitGap1, blk);
	if (res != imgeOk)
		return res;

	res=CalculateStreamSize(&gsi1);
	if (res != imgeOk)
		return res;

	// seN true if stream is enabled
	int se0=gsi0.esfixbc || gsi0.esloopbc;
	int se1=gsi1.esfixbc || gsi1.esloopbc;

	// number of enabled streams
	int secnt=0;
	if (se0)
		secnt++;
	if (se1)
		secnt++;

	// tlN true if stream has a real loop point defined (not generated)
	int tl0=gsi0.esloopbc && (gsi0.looptype == isiltStream);
	int tl1=gsi1.esloopbc && (gsi1.looptype == isiltStream);

	// number of streams with a real loop point
	int tlcnt=0;
	if (tl0)
		tlcnt++;
	if (tl1)
		tlcnt++;

	PDISKSECTORINFO si=di.pdt->sip+blk;

	// set size mode of the forward stream
	if (tl0)
		si->gapws0mode=csiegmResize;
	else
		si->gapws0mode=(gsi0.esloopbc) ? csiegmAuto : csiegmFixed;

	// set size mode of the backward stream
	if (tl1)
		si->gapws1mode=csiegmResize;
	else
		si->gapws1mode=(gsi1.esloopbc) ? csiegmAuto : csiegmFixed;

	// encode gap
	switch (secnt) {
		// error if gap area is present, but there is no gap stream at all
		case 0:
			return imgeGeneric;

		// 1 stream enabled
		case 1:
			res=ProcessBlockGap((se0 ? &gsi0 : &gsi1), gapsize);
			break;

		// 2 streams enabled, modify 1 stream if 1 true loop point is defined, otherwise modify both
		case 2:
			if (tlcnt == 1)
				res=ProcessBlockGap(&gsi0, &gsi1, gapsize, (tl0 ? 0 : 1));
			else
				res=ProcessBlockGap(&gsi0, &gsi1, gapsize);
			break;
	}

	return res;
}

// encode gap data with 1 gap stream present
int CCapsImageStd::ProcessBlockGap(PIMAGESTREAMINFO pg, int gapsize)
{
	// stop if no gap area
	if (!gapsize)
		return imgeOk;

	int skip=0;

	if (pg->esfixbc >= gapsize) {
		// fixed size is enough to fill the gap; disable loop
		SetLoop(pg, 0);

		// if backwards stream, skip unneeded bits
		if (pg->strtype == isitGap1)
			skip=pg->esfixbc-gapsize;
	} else {
		// loop required to fill the gap; error if loop point is not found
		if (!pg->esloopbc)
			return imgeGeneric;

		// number of bits to be generated using the loop
		int miss=gapsize-pg->esfixbc;

		// number of new samples required to generate the bits
		int scnt=miss/pg->esloopbc;

		// partial new sample size, 0 if all samples are complete
		int smod=miss%pg->esloopbc;

		// if partial sample is required
		if (smod) {
			// add another sample
			scnt++;

			// if backwards stream, skip bits not required from the added sample
			if (pg->strtype == isitGap1)
				skip=pg->esloopbc-smod;
		}

		// enable loop
		SetLoop(pg, scnt);
	}

	// encode gap data
	int res=ProcessStream(pg, di.encbitpos, gapsize, skip, !di.encwritebc);
	if (res != imgeOk)
		return res;

	// error if the requested data size was not filled
	if (pg->prcwritebc != gapsize)
		return imgeGeneric;

	// update bit position and bits written
	di.encbitpos=pg->prcbitpos;
	di.encwritebc+=pg->prcwritebc;

	if (pg->strtype == isitGap0)
		di.pdt->sip[pg->actblock].gapsizews0=gapsize;
	else
		di.pdt->sip[pg->actblock].gapsizews1=gapsize;

	return imgeOk;
}

// encode gap data with 2 gap streams present, 2 real loop points, or none
int CCapsImageStd::ProcessBlockGap(PIMAGESTREAMINFO pg0, PIMAGESTREAMINFO pg1, int gapsize)
{
	int gs0=pg0->esfixbc;
	int gs1=pg1->esfixbc;
	int fs=gs0+gs1;

	if (fs >= gapsize) {
		// fixed size would fill the gap; number of extra bits not needed
		int rem=fs-gapsize;
		int rem0=rem >> 1;
		int rem1=rem-rem0;

		// remove bits evenly as long as it is possible, then just remove from any stream that has enough bits
		while (rem0 || rem1) {
			if (gs0 >= rem0) {
				gs0-=rem0;
				rem0=0;
			} else {
				rem1+=rem0-gs0;
				rem0=0;
				gs0=0;
			}

			if (gs1 >= rem1) {
				gs1-=rem1;
				rem1=0;
			} else {
				rem0+=rem1-gs1;
				rem1=0;
				gs1=0;
			}
		}
	} else {
		// loop required to fill the gap; error if loop point is not found in either stream
		if (!pg0->esloopbc && !pg1->esloopbc)
			return imgeGeneric;

		// number of missing bits to fill the gap
		int miss=gapsize-fs;
		int miss0=miss >> 1;

		// if last block and the loop would pass the index (ie track gap at index) re-sync to index
		if (pg0->actblock == di.blockcount-1) {
			uint32_t sbp=di.encbitpos % di.singletrackbc;
			if (sbp+gs0 <= di.singletrackbc)
				if (sbp+gs0+miss >= di.singletrackbc)
					miss0=di.singletrackbc-(sbp+gs0);
		}

		int miss1=miss-miss0;

		// add bits evenly if both streams have loop points, otherwise just use the one with loop enabled
		while (miss0 || miss1) {
			if (pg0->esloopbc) {
				gs0+=miss0;
				miss0=0;
			} else {
				miss1+=miss0;
				miss0=0;
			}

			if (pg1->esloopbc) {
				gs1+=miss1;
				miss1=0;
			} else {
				miss0+=miss1;
				miss1=0;
			}
		}
	}

	// check if selected stream sizes would fill the entire gap
	if (gs0+gs1 != gapsize)
		return imgeGeneric;

	// encode forward stream
	int res=ProcessBlockGap(pg0, gs0);
	if (res != imgeOk)
		return res;

	// set gap split position
	di.encgsvalid=1;
	di.encgapsplit=di.encbitpos;

	// encode backwards stream
	ProcessBlockGap(pg1, gs1);
	if (res != imgeOk)
		return res;

	return imgeOk;
}

// encode gap data with 2 gap streams present, 1 real loop point
int CCapsImageStd::ProcessBlockGap(PIMAGESTREAMINFO pg0, PIMAGESTREAMINFO pg1, int gapsize, int loopsel)
{
	int gs0=pg0->esfixbc;
	int gs1=pg1->esfixbc;

	// fixed size stream uses its own size, the stream with the loop fills the remainder
	if (loopsel) {
		// 0: fix, 1: loop
		if (gs0 > gapsize)
			gs0=gapsize;

		gs1=gapsize-gs0;
	} else {
		// 0: loop, 1: fix
		if (gs1 > gapsize)
			gs1=gapsize;

		gs0=gapsize-gs1;
	}

	// encode forward stream
	int res=ProcessBlockGap(pg0, gs0);
	if (res != imgeOk)
		return res;

	// set gap split position
	di.encgsvalid=1;
	di.encgapsplit=di.encbitpos;

	// encode backwards stream
	ProcessBlockGap(pg1, gs1);
	if (res != imgeOk)
		return res;

	return imgeOk;
}

// set the selected sample loop point to a fixed size
void CCapsImageStd::SetLoop(PIMAGESTREAMINFO pg, int value)
{
	switch (pg->looptype) {
		// no loop point, disable sample size change
		case isiltNone:
			pg->scenable=0;
			break;

		// disable/enable size change at the loop point
		case isiltAuto:
			if (value) {
				pg->scenable=1;
				pg->scofs=pg->loopofs;
				pg->scmul=value;
			} else
				pg->scenable=0;
			break;

		// change loop point to fixed size sample
		case isiltStream:
			pg->scenable=1;
			pg->scofs=pg->loopofs;
			pg->scmul=value;
			break;
	}

	// update loop, just in case the current first sample is the loop point
	GetLoop(pg);
}

// update loop point if enabled
void CCapsImageStd::GetLoop(PIMAGESTREAMINFO pg)
{
	// stop, if not enabled
	if (!pg->scenable)
		return;

	// stop if not a loop point
	if (pg->strofs != pg->scofs)
		return;

	// add new samples
	pg->remstreambc+=pg->scmul*pg->samplebc;

	// make the sample size fixed
	pg->streambc=pg->remstreambc;
}

// fix mfm encoding where needed
void CCapsImageStd::MFMFixup()
{
	// process all blocks
	for (int blk=0; blk < di.blockcount; blk++) {
		PIMAGEBLOCKINFO pi=di.block+blk;

		// don't fix if not MFM
		if (pi->fdenc != ibieMFM)
			continue;

		// get bit before the first data bit written
		uint32_t lbp=(pi->fdbitpos ? pi->fdbitpos : di.trackbc)-1;
		int lv = trackbuf.ReadBit(lbp);

		// mfm encoding assumed the last bit to be 0; fix if it is 1
		if (lv)
			trackbuf.ClearBit(pi->fdbitpos, 1);
	}
}
