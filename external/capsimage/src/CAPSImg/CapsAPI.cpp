#include "stdafx.h"



using namespace std;

int api_debug_request;
CDiskEncoding dskenc;
CDiskImageFactory imgfactory;
vector <PCDISKIMAGE> img;

// version info size
int sizeversioninfo[]= {
	sizeof(CapsVersionInfo)
};

// track info size
int sizetrackinfo[]= {
	sizeof(CapsTrackInfo),
	sizeof(CapsTrackInfoT1),
	sizeof(CapsTrackInfoT2),
};



// start caps image support
SDWORD __cdecl CAPSInit()
{
	return imgeOk;
}

// stop caps image support
SDWORD __cdecl CAPSExit()
{
	for (PCDISKIMAGE &actimg : img) {
		delete actimg;
		actimg = NULL;
	}

	return imgeOk;
}

// add image container
SDWORD __cdecl CAPSAddImage()
{
	// use the base image type as a placeholder, returning errors for the functions
	PCDISKIMAGE pi = new CDiskImage;
	unsigned pos;

	for (pos = 0; pos < img.size(); pos++) {
		if (img[pos])
			continue;

		img[pos] = pi;
		return pos;
	}

	img.push_back(pi);

	return pos;
}

// delete image container
SDWORD __cdecl CAPSRemImage(SDWORD id)
{
	if (id<0 || (unsigned)id>=img.size())
		return -1;

	delete img[id];
	img[id]=NULL;
	return id;
}

// lock image using file name
SDWORD __cdecl CAPSLockImage(SDWORD id, PCHAR name)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	CapsFile cf;
	memset(&cf, 0, sizeof(CapsFile));
	cf.name=name;

	return CAPSLockImage(id, &cf);
}

// lock image using the specified memory buffer
SDWORD __cdecl CAPSLockImageMemory(SDWORD id, PUBYTE buffer, UDWORD length, UDWORD flag)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	CapsFile cf;
	memset(&cf, 0, sizeof(CapsFile));
	cf.memmap=buffer;
	cf.size=length;
	cf.flag|=CFF_MEMMAP;
	if (flag & DI_LOCK_MEMREF)
		cf.flag|=CFF_MEMREF;

	return CAPSLockImage(id, &cf);
}

// lock image using the specified file - internal function
int CAPSLockImage(int id, PCAPSFILE pcf)
{
	int res = imgeOk;

	// get the possible image type used by the file
	int type = imgfactory.GetImageType(pcf);

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
		PCDISKIMAGE newimg = imgfactory.CreateImage(type);

		if (newimg) {
			// delete the existing image class regardless of type since it can no longer be used
			CAPSRemImage(id);

			// add the new image class
			img[id] = newimg;

			// try to lock the file; this can still give an error as the checks will be more thorough
			res = img[id]->Lock(pcf);
		} else
			res = imgeGeneric;
	}

	return res;
}

// unlock image
SDWORD __cdecl CAPSUnlockImage(SDWORD id)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	int res=img[id]->Unlock();

	return res;
}

// load and decode complete image
SDWORD __cdecl CAPSLoadImage(SDWORD id, UDWORD flag)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	int res=img[id]->LoadImage(flag);

	return res;
}

// get image information
SDWORD __cdecl CAPSGetImageInfo(PCAPSIMAGEINFO pi, SDWORD id)
{
	if (!pi)
		return imgeGeneric;

	memset(pi, 0, sizeof(CapsImageInfo));

	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKIMAGEINFO pd=img[id]->GetInfo();
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
		}	else {
			// invalid image type if not a dumped image and there is no information available
			pi->type = ciitNA;
		}
	}

	return imgeOk;
}

// load and decode track
SDWORD __cdecl CAPSLockTrack(PVOID ptrackinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD flag)
{
	if (!ptrackinfo)
		return imgeGeneric;

	// structure revision is zero, unless newer one requested
	unsigned rev=0;
	if (flag & DI_LOCK_TYPE)
		rev=*(PUDWORD)ptrackinfo;

	// if unsupported type set the highest supported version
	if (rev > 2) {
		*(PUDWORD)ptrackinfo=2;
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
		PDISKTRACKINFO pt=img[id]->GetTrack(cylinder, head);
		if (pt) {
			switch (rev) {
				case 2:
					pt->wseed=((PCAPSTRACKINFOT2)ptrackinfo)->wseed;
					break;
			}
		}
	}

	// clear structure
	memset(ptrackinfo, 0, sizetrackinfo[rev]);

	// lock track
	PDISKTRACKINFO pt=img[id]->LockTrack(cylinder, head, flag);
	if (!pt) {
		PDISKIMAGEINFO pd=img[id]->GetInfo();

		return pd ? pd->error : imgeGeneric;
	}

	// track type conversion
	UDWORD ttype;
	switch (pt->ci.dentype) {
		case cpdenNA:
			ttype=ctitNA;
			break;

		case cpdenNoise:
			ttype=ctitNoise;
			break;

		case cpdenAuto:
			ttype=ctitAuto;
			break;

		default:
			ttype=ctitVar;
			break;
	}

	// signal track update
	if (pt->fdpsize)
		ttype|=CTIT_FLAG_FLAKEY;

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
void CAPSLockTrackT0(PCAPSTRACKINFO pi, PDISKTRACKINFO pt, UDWORD ttype, UDWORD flag)
{
	pi->type=ttype;
	pi->cylinder=pt->cylinder;
	pi->head=pt->head;
	pi->sectorcnt=pt->sectorcnt;
	pi->sectorsize=0;
	pi->trackcnt=pt->trackcnt;
	pi->trackbuf=pt->trackbuf;
	pi->tracklen=(flag & DI_LOCK_TRKBIT) ? pt->trackbc : pt->tracklen;
	pi->timelen=pt->timecnt;
	pi->timebuf=pt->timebuf;

	for (int trk=0; trk < pt->trackcnt; trk++) {
		pi->trackdata[trk]=pt->trackdata[trk];
		pi->tracksize[trk]=pt->tracksize[trk];
	}
}

// get track info type 1
void CAPSLockTrackT1(PCAPSTRACKINFOT1 pi, PDISKTRACKINFO pt, UDWORD ttype, UDWORD flag)
{
	pi->type=ttype;
	pi->cylinder=pt->cylinder;
	pi->head=pt->head;
	pi->sectorcnt=pt->sectorcnt;
	pi->sectorsize=0;
	pi->trackbuf=pt->trackbuf;
	pi->tracklen=(flag & DI_LOCK_TRKBIT) ? pt->trackbc : pt->tracklen;
	pi->timelen=pt->timecnt;
	pi->timebuf=pt->timebuf;
	pi->overlap=pt->overlap;
}

// get track info type 1
void CAPSLockTrackT2(PCAPSTRACKINFOT2 pi, PDISKTRACKINFO pt, UDWORD ttype, UDWORD flag)
{
	pi->type=ttype;
	pi->cylinder=pt->cylinder;
	pi->head=pt->head;
	pi->sectorcnt=pt->sectorcnt;
	pi->sectorsize=0;
	pi->trackbuf=pt->trackbuf;
	pi->tracklen=(flag & DI_LOCK_TRKBIT) ? pt->trackbc : pt->tracklen;
	pi->timelen=pt->timecnt;
	pi->timebuf=pt->timebuf;
	pi->overlap=pt->overlap;
	pi->startbit=pt->startbit;
	pi->wseed=pt->wseed;
	pi->weakcnt=pt->fdpsize;
}

// remove track from cache
SDWORD __cdecl CAPSUnlockTrack(SDWORD id, UDWORD cylinder, UDWORD head)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKTRACKINFO pt=img[id]->UnlockTrack(cylinder, head);

	return pt ? imgeOk : imgeOutOfRange;
}

// remove all tracks from cache
SDWORD __cdecl CAPSUnlockAllTracks(SDWORD id)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	img[id]->UnlockTrack();

	return imgeOk;
}

// get platform name
PCHAR  __cdecl CAPSGetPlatformName(UDWORD pid)
{
	return (PCHAR)CDiskImage::GetPlatformName(pid);
}

// get library version
SDWORD __cdecl CAPSGetVersionInfo(PVOID pversioninfo, UDWORD flag)
{
	if (!pversioninfo)
		return imgeGeneric;

	// structure revision is zero, unless newer one requested
	unsigned rev=0;
	if (flag & DI_LOCK_TYPE)
		rev=*(PUDWORD)pversioninfo;

	// if unsupported type set the highest supported version
	if (rev > 0) {
		*(PUDWORD)pversioninfo=0;
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
	pi->release=CAPS_LIB_RELEASE;
	pi->revision=CAPS_LIB_REVISION;
	pi->flag=DI_LOCK_INDEX|
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
SDWORD __cdecl CAPSGetInfo(PVOID pinfo, SDWORD id, UDWORD cylinder, UDWORD head, UDWORD inftype, UDWORD infid)
{
	if (!pinfo)
		return imgeGeneric;

	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKIMAGEINFO pd = img[id]->GetInfo();
	PDISKTRACKINFO pt = img[id]->GetTrack(cylinder, head);

	int res=imgeUnsupportedType;

	switch (inftype) {
		case cgiitSector:
			res=CAPSGetSectorInfo((PCAPSSECTORINFO)pinfo, pd, pt, infid);
			break;

		case cgiitWeak:
			res=CAPSGetWeakInfo((PCAPSDATAINFO)pinfo, pd, pt, infid);
			break;

		case cgiitRevolution:
			res = CAPSGetRevolutionInfo((PCAPSREVOLUTIONINFO)pinfo, pd, pt, infid);
			break;
	}

	return res;
}

// get sector information
int CAPSGetSectorInfo(PCAPSSECTORINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, UDWORD infid)
{
	memset(pi, 0, sizeof(CapsSectorInfo));

	if (!pt)
		return imgeOutOfRange;

	if (pt->sipsize <= 0 || !pt->sip)
		return imgeOutOfRange;

	if (infid >= (unsigned)pt->sipsize)
		return imgeOutOfRange;

	PDISKSECTORINFO si=pt->sip+infid;

	pi->descdatasize=si->descdatasize;
	pi->descgapsize=si->descgapsize;
	pi->datasize=si->datasize;
	pi->gapsize=si->gapsize;
	pi->datastart=si->datastart;
	pi->gapstart=si->gapstart;
	pi->gapsizews0=si->gapsizews0;
	pi->gapsizews1=si->gapsizews1;
	pi->gapws0mode=si->gapws0mode;
	pi->gapws1mode=si->gapws1mode;
	pi->celltype=si->celltype;
	pi->enctype=si->enctype;

	return imgeOk;
}

// get weak data information
int CAPSGetWeakInfo(PCAPSDATAINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, UDWORD infid)
{
	memset(pi, 0, sizeof(CapsDataInfo));

	if (!pt)
		return imgeOutOfRange;

	if (pt->fdpsize <= 0 || !pt->fdp)
		return imgeOutOfRange;

	if (infid >= (unsigned)pt->fdpsize)
		return imgeOutOfRange;

	PDISKDATAMARK dm=pt->fdp+infid;

	pi->type=cditWeak;
	pi->start=dm->position;
	pi->size=dm->size;

	return imgeOk;
}

// get revolution information
int CAPSGetRevolutionInfo(PCAPSREVOLUTIONINFO pi, PDISKIMAGEINFO pd, PDISKTRACKINFO pt, UDWORD infid)
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
SDWORD __cdecl CAPSSetRevolution(SDWORD id, UDWORD value)
{
	if (id<0 || (unsigned)id >= img.size() || !img[id])
		return imgeOutOfRange;

	PDISKIMAGEINFO pd = img[id]->GetInfo();
	if (!pd)
		return imgeGeneric;

	// set revolution
	pd->nextrev = value;

	return imgeOk;
}

// get image type using file name
SDWORD __cdecl CAPSGetImageType(PCHAR name)
{
	CapsFile cf;
	memset(&cf, 0, sizeof(CapsFile));
	cf.name = name;

	// get the possible image type used by the file
	return imgfactory.GetImageType(&cf);
}

// get image type using the specified memory buffer
SDWORD __cdecl CAPSGetImageTypeMemory(PUBYTE buffer, UDWORD length)
{
	CapsFile cf;
	memset(&cf, 0, sizeof(CapsFile));
	cf.memmap = buffer;
	cf.size = length;
	cf.flag |= CFF_MEMMAP | CFF_MEMREF;

	// get the possible image type used by the file
	return imgfactory.GetImageType(&cf);
}

// return the state of host debug request coming from the library and reset the state
SDWORD __cdecl CAPSGetDebugRequest()
{
	SDWORD res = api_debug_request;
	api_debug_request = 0;

	return res;
}
