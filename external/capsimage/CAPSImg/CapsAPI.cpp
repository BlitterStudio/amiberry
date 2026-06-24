#include "stdafx.h"



int api_debug_request;
CDiskEncoding dskenc;
CDiskImageFactory imgfactory;
std::vector<std::unique_ptr<CDiskImage>> img;

// version info size
int sizeversioninfo[] = {
	sizeof(CapsVersionInfo)
};

// track info size
int sizetrackinfo[] = {
	sizeof(CapsTrackInfo),
	sizeof(CapsTrackInfoT1),
	sizeof(CapsTrackInfoT2),
};



// start caps image support
int32_t __cdecl CAPSInit()
{
	return imgeOk;
}

// stop caps image support
int32_t __cdecl CAPSExit()
{
	img.clear();

	return imgeOk;
}

// add image container
int32_t __cdecl CAPSAddImage()
{
	// use the base image type as a placeholder, returning errors for the functions
	auto pi = std::make_unique<CDiskImage>();
	unsigned pos;

	for (pos = 0; pos < img.size(); pos++) {
		if (img[pos])
			continue;

		img[pos] = std::move(pi);
		return pos;
	}

	img.push_back(std::move(pi));

	return pos;
}

// delete image container
int32_t __cdecl CAPSRemImage(int32_t id)
{
	if (id < 0 || (unsigned)id >= img.size())
		return -1;

	img[id] = nullptr;

	return id;
}

// lock image using file name
int32_t __cdecl CAPSLockImage(int32_t id, const char *name)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	int res = img[id]->Unlock();
	if (res != imgeOk)
		return res;

	auto pf = std::make_unique<CDiskFile>();
	if (pf->Open(name, 0))
		return imgeOpen;

	return CAPSLockImage(id, std::move(pf));
}

// lock image using the specified memory buffer
int32_t __cdecl CAPSLockImageMemory(int32_t id, const uint8_t *buffer, uint32_t length, uint32_t flag)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	int res = img[id]->Unlock();
	if (res != imgeOk)
		return res;

	auto pf = std::make_unique<CMemoryFile>();
	int mode = (flag & DI_LOCK_MEMREF) ? 0 : BFFLAG_CREATE;
	if (pf->Open("", buffer, length, mode))
		return imgeOpen;

	return CAPSLockImage(id, std::move(pf));
}

// lock image using the specified file - internal function
int CAPSLockImage(int id, std::unique_ptr<CBaseFile> pf)
{
	int res = imgeOk;

	// get the possible image type used by the file
	int type = imgfactory.GetImageType(*pf);

	// check for file errors
	switch (type) {
		// problem with the file
		case citError:
			res = imgeOpen;
			break;

		// unknown file content
		case citUnknown:
			res = imgeType;
			break;
	}

	// file type is known and supported
	if (res == imgeOk) {
		auto newimg = imgfactory.CreateImage(type);

		if (newimg) {
			// replace the previous image class
			img[id] = std::move(newimg);

			// try to lock the file; this can still give an error as the checks will be more thorough
			res = img[id]->Lock(std::move(pf));
		} else
			res = imgeGeneric;
	}

	return res;
}

// unlock image
int32_t __cdecl CAPSUnlockImage(int32_t id)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	int res = img[id]->Unlock();

	return res;
}

// load and decode complete image
int32_t __cdecl CAPSLoadImage(int32_t id, uint32_t flag)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	int res = img[id]->LoadImage(flag);

	return res;
}

// get image information
int32_t __cdecl CAPSGetImageInfo(PCAPSIMAGEINFO pi, int32_t id)
{
	if (!pi)
		return imgeGeneric;

	memset(pi, 0, sizeof(CapsImageInfo));

	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKIMAGEINFO pd = img[id]->GetInfo();
	if (!pd)
		return imgeGeneric;

	// convert image info
	if (pd->civalid) {
		// use image info if available
		switch (pd->ci.type) {
			case cpimtFDD:
				pi->type = ciitFDD;
				break;

			default:
				pi->type = ciitNA;
				break;
		}

		pi->release = pd->ci.release;
		pi->revision = pd->ci.revision;
		pi->mincylinder = pd->ci.mincylinder;
		pi->maxcylinder = pd->ci.maxcylinder;
		pi->minhead = pd->ci.minhead;
		pi->maxhead = pd->ci.maxhead;
		CDiskImage::DecodeDateTime(&pi->crdt, &pd->ci.crdt);

		for (int plt = 0; plt < CAPS_MAXPLATFORM; plt++)
			pi->platform[plt] = pd->ci.platform[plt];
	} else {
		// image info not available
		if (pd->dmpcount) {
			// if there is at least one track dumped use dump information
			pi->type = ciitFDD;
			pi->release = 0;
			pi->revision = 0;
			pi->mincylinder = pd->umincylinder;
			pi->maxcylinder = pd->umaxcylinder;
			pi->minhead = pd->uminhead;
			pi->maxhead = pd->umaxhead;
		} else {
			// invalid image type if not a dumped image and there is no information available
			pi->type = ciitNA;
		}
	}

	return imgeOk;
}

// load and decode track
int32_t __cdecl CAPSLockTrack(void *ptrackinfo, int32_t id, uint32_t cylinder, uint32_t head, uint32_t flag)
{
	if (!ptrackinfo)
		return imgeGeneric;

	// structure revision is zero, unless newer one requested
	unsigned rev = 0;
	if (flag & DI_LOCK_TYPE)
		rev = *(uint32_t *)ptrackinfo;

	// if unsupported type set the highest supported version
	if (rev > 2) {
		*(uint32_t *)ptrackinfo = 2;
		return imgeUnsupportedType;
	}

	// check id
	if (id < 0 || (unsigned)id >= img.size() || !img[id]) {
		// clear structure
		memset(ptrackinfo, 0, sizetrackinfo[rev]);

		return imgeOutOfRange;
	}

	// set weak bit generator
	if (flag & DI_LOCK_SETWSEED) {
		PDISKTRACKINFO pt = img[id]->GetTrack(cylinder, head);
		if (pt) {
			switch (rev) {
				case 2:
					pt->wseed = ((PCAPSTRACKINFOT2)ptrackinfo)->wseed;
					break;
			}
		}
	}

	// clear structure
	memset(ptrackinfo, 0, sizetrackinfo[rev]);

	// lock track
	PDISKTRACKINFO pt = img[id]->LockTrack(cylinder, head, flag);
	if (!pt) {
		PDISKIMAGEINFO pd = img[id]->GetInfo();

		return pd ? pd->error : imgeGeneric;
	}

	// track type conversion
	uint32_t ttype;
	switch (pt->ci.dentype) {
		case cpdenNA:
			ttype = ctitNA;
			break;

		case cpdenNoise:
			ttype = ctitNoise;
			break;

		case cpdenAuto:
			ttype = ctitAuto;
			break;

		default:
			ttype = ctitVar;
			break;
	}

	// signal track update
	if (pt->fdpsize)
		ttype |= CTIT_FLAG_FLAKEY;

	// track type is variable density for raw dumps
	if (pt->rawdump)
		ttype = ctitVar;

	// if multiple revolutions used by a raw dump signal track update
	if (pt->rawupdate)
		ttype |= CTIT_FLAG_FLAKEY;

	// get the correct structure
	switch (rev) {
		case 0:
			CAPSLockTrackT0((PCAPSTRACKINFO)ptrackinfo, pt, ttype, flag);
			break;

		case 1:
			CAPSLockTrackT1((PCAPSTRACKINFOT1)ptrackinfo, pt, ttype, flag);
			break;

		case 2:
			CAPSLockTrackT2((PCAPSTRACKINFOT2)ptrackinfo, pt, ttype, flag);
			break;
	}

	return imgeOk;
}

// get track info type 0
void CAPSLockTrackT0(PCAPSTRACKINFO pi, PDISKTRACKINFO pt, uint32_t ttype, uint32_t flag)
{
	pi->type = ttype;
	pi->cylinder = pt->cylinder;
	pi->head = pt->head;
	pi->sectorcnt = pt->sectorcnt;
	pi->sectorsize = 0;
	pi->trackcnt = pt->trackcnt;
	pi->trackbuf = pt->trackbuf;
	pi->tracklen = (flag & DI_LOCK_TRKBIT) ? pt->trackbc : pt->tracklen;
	pi->timelen = pt->timecnt;
	pi->timebuf = pt->timebuf;

	for (int trk = 0; trk < pt->trackcnt; trk++) {
		pi->trackdata[trk] = pt->trackdata[trk];
		pi->tracksize[trk] = pt->tracksize[trk];
	}
}

// get track info type 1
void CAPSLockTrackT1(PCAPSTRACKINFOT1 pi, PDISKTRACKINFO pt, uint32_t ttype, uint32_t flag)
{
	pi->type = ttype;
	pi->cylinder = pt->cylinder;
	pi->head = pt->head;
	pi->sectorcnt = pt->sectorcnt;
	pi->sectorsize = 0;
	pi->trackbuf = pt->trackbuf;
	pi->tracklen = (flag & DI_LOCK_TRKBIT) ? pt->trackbc : pt->tracklen;
	pi->timelen = pt->timecnt;
	pi->timebuf = pt->timebuf;
	pi->overlap = pt->overlap;
}

// get track info type 1
void CAPSLockTrackT2(PCAPSTRACKINFOT2 pi, PDISKTRACKINFO pt, uint32_t ttype, uint32_t flag)
{
	pi->type = ttype;
	pi->cylinder = pt->cylinder;
	pi->head = pt->head;
	pi->sectorcnt = pt->sectorcnt;
	pi->sectorsize = 0;
	pi->trackbuf = pt->trackbuf;
	pi->tracklen = (flag & DI_LOCK_TRKBIT) ? pt->trackbc : pt->tracklen;
	pi->timelen = pt->timecnt;
	pi->timebuf = pt->timebuf;
	pi->overlap = pt->overlap;
	pi->startbit = pt->startbit;
	pi->wseed = pt->wseed;
	pi->weakcnt = pt->fdpsize;
}

// remove track from cache
int32_t __cdecl CAPSUnlockTrack(int32_t id, uint32_t cylinder, uint32_t head)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKTRACKINFO pt = img[id]->UnlockTrack(cylinder, head);

	return pt ? imgeOk : imgeOutOfRange;
}

// remove all tracks from cache
int32_t __cdecl CAPSUnlockAllTracks(int32_t id)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	img[id]->UnlockTrack();

	return imgeOk;
}

// get platform name
const char * __cdecl CAPSGetPlatformName(uint32_t pid)
{
	return CDiskImage::GetPlatformName(pid);
}

// get library version
int32_t __cdecl CAPSGetVersionInfo(void *pversioninfo, uint32_t flag)
{
	if (!pversioninfo)
		return imgeGeneric;

	// structure revision is zero, unless newer one requested
	unsigned rev = 0;
	if (flag & DI_LOCK_TYPE)
		rev = *(uint32_t *)pversioninfo;

	// if unsupported type set the highest supported version
	if (rev > 0) {
		*(uint32_t *)pversioninfo = 0;
		return imgeUnsupportedType;
	}

	// clear structure
	memset(pversioninfo, 0, sizeversioninfo[rev]);

	// get the correct structure
	switch (rev) {
		case 0:
			CAPSGetVersionInfoT0((PCAPSVERSIONINFO)pversioninfo);
			break;
	}

	return imgeOk;
}

// get library version type 0
void CAPSGetVersionInfoT0(PCAPSVERSIONINFO pi)
{
	pi->release = CAPS_LIB_RELEASE;
	pi->revision = CAPS_LIB_REVISION;
	pi->flag = DI_LOCK_INDEX |
		DI_LOCK_ALIGN |
		DI_LOCK_DENVAR |
		DI_LOCK_DENAUTO |
		DI_LOCK_DENNOISE |
		DI_LOCK_NOISE |
		DI_LOCK_NOISEREV |
		DI_LOCK_MEMREF |
		DI_LOCK_UPDATEFD |
		DI_LOCK_TYPE |
		DI_LOCK_DENALT |
		DI_LOCK_OVLBIT |
		DI_LOCK_TRKBIT |
		DI_LOCK_NOUPDATE |
		DI_LOCK_SETWSEED;
}

// get various information after decoding
int32_t __cdecl CAPSGetInfo(void *pinfo, int32_t id, uint32_t cylinder, uint32_t head, uint32_t inftype, uint32_t infid)
{
	if (!pinfo)
		return imgeGeneric;

	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKIMAGEINFO pd = img[id]->GetInfo();
	PDISKTRACKINFO pt = img[id]->GetTrack(cylinder, head);

	int res = imgeUnsupportedType;

	switch (inftype) {
		case cgiitSector:
			res = CAPSGetSectorInfo((PCAPSSECTORINFO)pinfo, pd, pt, infid);
			break;

		case cgiitWeak:
			res = CAPSGetWeakInfo((PCAPSDATAINFO)pinfo, pd, pt, infid);
			break;

		case cgiitRevolution:
			res = CAPSGetRevolutionInfo((PCAPSREVOLUTIONINFO)pinfo, pd, pt, infid);
			break;
	}

	return res;
}

// get sector information
int CAPSGetSectorInfo(PCAPSSECTORINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, uint32_t infid)
{
	memset(pi, 0, sizeof(CapsSectorInfo));

	if (!pt)
		return imgeOutOfRange;

	if (pt->sipsize <= 0 || !pt->sip)
		return imgeOutOfRange;

	if (infid >= (unsigned)pt->sipsize)
		return imgeOutOfRange;

	PDISKSECTORINFO si = pt->sip + infid;

	pi->descdatasize = si->descdatasize;
	pi->descgapsize = si->descgapsize;
	pi->datasize = si->datasize;
	pi->gapsize = si->gapsize;
	pi->datastart = si->datastart;
	pi->gapstart = si->gapstart;
	pi->gapsizews0 = si->gapsizews0;
	pi->gapsizews1 = si->gapsizews1;
	pi->gapws0mode = si->gapws0mode;
	pi->gapws1mode = si->gapws1mode;
	pi->celltype = si->celltype;
	pi->enctype = si->enctype;

	return imgeOk;
}

// get weak data information
int CAPSGetWeakInfo(PCAPSDATAINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, uint32_t infid)
{
	memset(pi, 0, sizeof(CapsDataInfo));

	if (!pt)
		return imgeOutOfRange;

	if (pt->fdpsize <= 0 || !pt->fdp)
		return imgeOutOfRange;

	if (infid >= (unsigned)pt->fdpsize)
		return imgeOutOfRange;

	PDISKDATAMARK dm = pt->fdp + infid;

	pi->type = cditWeak;
	pi->start = dm->position;
	pi->size = dm->size;

	return imgeOk;
}

// get revolution information
int CAPSGetRevolutionInfo(PCAPSREVOLUTIONINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, uint32_t infid)
{
	memset(pi, 0, sizeof(CapsRevolutionInfo));

	// set image data if image is valid
	if (pd) {
		// next, last and real revolution used by lock track
		pi->next = pd->nextrev;
		pi->last = pd->lastrev;
		pi->real = pd->realrev;
	}

	// set track data if track is valid
	if (pt) {
		// use the revolutions sampled for raw dumps, -1 (unlimited/randomized) otherwise
		pi->max = (pt->rawdump) ? pt->rawtrackcnt : -1;
	}

	return imgeOk;
}

// set the next revolution to be used by track lock
int32_t __cdecl CAPSSetRevolution(int32_t id, uint32_t value)
{
	if (id < 0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKIMAGEINFO pd = img[id]->GetInfo();
	if (!pd)
		return imgeGeneric;

	// set revolution
	pd->nextrev = value;

	return imgeOk;
}

// get image type using file name
int32_t __cdecl CAPSGetImageType(const char *name)
{
	auto pf = std::make_unique<CDiskFile>();
	if (pf->Open(name, 0))
		return citError;

	// get the possible image type used by the file
	return imgfactory.GetImageType(*pf);
}

// get image type using the specified memory buffer
int32_t __cdecl CAPSGetImageTypeMemory(const uint8_t *buffer, uint32_t length)
{
	auto pf = std::make_unique<CMemoryFile>();
	if (pf->Open("", buffer, length, 0))
		return citError;

	// get the possible image type used by the file
	return imgfactory.GetImageType(*pf);
}

// return the state of host debug request coming from the library and reset the state
int32_t __cdecl CAPSGetDebugRequest()
{
	int32_t res = api_debug_request;
	api_debug_request = 0;

	return res;
}
