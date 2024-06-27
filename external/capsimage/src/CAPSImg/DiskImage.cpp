#include "stdafx.h"



// platform name helper
LPCSTR CDiskImage::pidname[cppidLast]= {
	"N/A",
	"Amiga",
	"Atari ST",
	"PC",
	"Amstrad CPC",
	"Spectrum",
	"Sam Coupe",
	"Archimedes",
	"C64",
	"Atari 8-bit",
};



CDiskImage::CDiskImage()
{
	MakeCRCTable();
	dti=NULL;
	Destroy();
}

CDiskImage::~CDiskImage()
{
	Destroy();
}

// lock and scan file
int CDiskImage::Lock(PCAPSFILE pcf)
{
	return imgeUnsupported;
}

// release file
int CDiskImage::Unlock()
{
	return imgeUnsupported;
}

// load image file and test for erros
int CDiskImage::LoadImage(UDWORD flag, int free)
{
	int res=imgeOk;

	// cancel, if no track data
	if (!dti)
		return res;

	// try to load all tracks
	for (int trk=0; trk < dticnt; trk++) {
		PDISKTRACKINFO pt=dti+trk;

		// test only untested tracks
		switch (pt->type) {
			case dtitUndefined:
			case dtitError:
				continue;
		}

		// load and decode
		int read=AllocTrack(pt, flag);

		// free track only if requested
		if (free)
			FreeTrack(pt);

		// test load result
		switch (read) {
			// stop if image can't be read
			case imgeUnsupported:
			case imgeIncompatible:
				trk=dticnt;
				res=read;
				continue;

			// no errors
			case imgeOk:
				break;

			// set error
			default:
				res=imgeGeneric;
				break;
		}
	}

	return res;
}

// destroy any cached information
void CDiskImage::Destroy()
{
	// unlock all track data
	UnlockTrack(true);

	// invalid image
	memset(&dii, 0, sizeof(dii));

	// read only
	dii.readonly=true;

	// track lock to raw format
	dii.rawlock=true;

	// no track data
	dticnt=0;
	delete [] dti;
	dti=NULL;
	dticyl=0;
	dtihed=0;
}

// add or replace track
int CDiskImage::AddTrack(PDISKTRACKINFO pdti)
{
	// clear track data
	UnlockTrack(pdti->cylinder, pdti->head, true);

	// map track info to memory
	PDISKTRACKINFO pt=MapTrack(pdti->cylinder, pdti->head);
	if (!pt)
		return imgeOutOfRange;

	// replace info
	*pt=*pdti;

	// update image info
	UpdateImageInfo(pdti);

	return imgeOk;
}

// get track information
PDISKTRACKINFO CDiskImage::GetTrack(int cylinder, int head)
{
	// cancel, if no track data
	if (!dti)
		return NULL;

	// ignore wrong cylinder
	if (cylinder<0 || cylinder>=dticyl)
		return NULL;

	// ignore wrong head
	if (head<0 || head>=dtihed)
		return NULL;

	return &dti[cylinder*dtihed+head];
}

// get track information and lock track into memory
PDISKTRACKINFO CDiskImage::LockTrack(int cylinder, int head, UDWORD flag)
{
	// save the current revolution counter
	dii.lastrev = dii.nextrev;

	// locate track information
	PDISKTRACKINFO pti=GetTrack(cylinder, head);

	// allocate and load buffers
	dii.error=AllocTrack(pti, flag);

	// update the revolution counter unless updates are disabled
	// limit the counter value to 8 bit
	if (!(flag & DI_LOCK_NOUPDATE))
		dii.nextrev = (dii.nextrev + 1) & 0xff;

	return dii.error == imgeOk ? pti : NULL;
}

// get track information and lock track into memory for comparison
PDISKTRACKINFO CDiskImage::LockTrackComp(int cylinder, int head, UDWORD flag, int sblk, int eblk)
{
	// locate track information
	PDISKTRACKINFO pti=GetTrack(cylinder, head);
	pti->compsblk=sblk;
	pti->compeblk=eblk;
	flag|=DI_LOCK_COMP;

	// allocate and load buffers
	dii.error=AllocTrack(pti, flag);
	return dii.error == imgeOk ? pti : NULL;
}

// get track information and unlock track from memory
PDISKTRACKINFO CDiskImage::UnlockTrack(int cylinder, int head, int forced)
{
	// locate track information
	PDISKTRACKINFO pti=GetTrack(cylinder, head);

	// free buffers
	FreeTrack(pti, forced);

	return pti;
}

// unlock track from memory
void CDiskImage::UnlockTrack(PDISKTRACKINFO pti, int forced)
{
	// free buffers
	FreeTrack(pti, forced);
}

// unlock all tracks from memory
void CDiskImage::UnlockTrack(int forced)
{
	// cancel, if no track data
	if (!dti)
		return;

	// free all buffers
	for (int trk=0; trk < dticnt; trk++)
		FreeTrack(dti+trk, forced);
}

// replace locked track data with a private buffer
int CDiskImage::LinkTrackData(PDISKTRACKINFO pti, int size)
{
	// cancel if bad parameters
	if (!pti || size<0)
		return imgeGeneric;

	// free existing buffers
	FreeTrackData(pti);

	// 1 revolution
	if (size) {
		pti->trackcnt=1;
		pti->tracklen=size;
		pti->trackbuf=new UBYTE[pti->tracklen];
		pti->trackdata[0]=pti->trackbuf;
		pti->tracksize[0]=pti->tracklen;
		pti->trackstart[0]=0;

		// clear track data
		memset(pti->trackbuf, 0, pti->tracklen);
	}

	// prevent operations that could modify or remove the buffer
	pti->linked=true;
	pti->linkinfo=0;
	pti->linkflag=0;

	return imgeOk;
}

// read and decode track format
int CDiskImage::LoadTrack(PDISKTRACKINFO pti, UDWORD flag)
{
	return imgeUnsupported;
}

// read and decode plain track format
int CDiskImage::LoadPlain(PDISKTRACKINFO pti)
{
	return imgeUnsupported;
}

// allocate and decode track
int CDiskImage::AllocTrack(PDISKTRACKINFO pti, UDWORD flag)
{
	// cancel if bad track
	if (!pti)
		return imgeGeneric;

	// track type support
	switch (pti->type) {
		// load known formats
		case dtitCapsDump:
		case dtitCapsImage:
			return LoadTrack(pti, flag);

		// plain format is generic
		case dtitPlain:
			return LoadPlain(pti);

		// any other state is undefined
		default:
			return imgeGeneric;
	}
}

// free track buffers
void CDiskImage::FreeTrack(PDISKTRACKINFO pti, int forced)
{
	// cancel if invalid track data
	if (!pti)
		return;

	// cancel if track data linked and free is not forced
	if (pti->linked && !forced)
		return;

	// linked track data does not exist anymore
	pti->linked=false;
	pti->linkinfo=0;
	pti->linkflag=0;

	// free buffers
	FreeTrackData(pti);
	FreeTrackTiming(pti);
}

// free track data buffers
void CDiskImage::FreeTrackData(PDISKTRACKINFO pti)
{
	// cancel if invalid track data
	if (!pti)
		return;

	if (pti->rawdump)
		delete[] pti->trackdata[0];

	// clear track data
	for (int trk=0; trk < CAPS_MTRS; trk++) {
		pti->trackdata[trk]=NULL;
		pti->tracksize[trk]=0;
	}

	// free track buffer
	pti->trackcnt=0;

	if (!pti->rawdump)
		delete[] pti->trackbuf;

	pti->trackbuf=NULL;
	pti->tracklen=0;

	// free data specific buffers
	FreeTrackFD(pti);
	FreeTrackSI(pti);
}

// free track timing buffer
void CDiskImage::FreeTrackTiming(PDISKTRACKINFO pti)
{
	// cancel if invalid track data
	if (!pti)
		return;

	// free timing buffer
	pti->timecnt=0;
	delete [] pti->timebuf;
	pti->timebuf=NULL;

	// free raw timing buffer
	pti->rawtimecnt = 0;
	delete[] pti->rawtimebuf;
	pti->rawtimebuf = NULL;
}

// free flakey position buffer
void CDiskImage::FreeTrackFD(PDISKTRACKINFO pti)
{
	// cancel if invalid track data
	if (!pti)
		return;

	// free flakey buffer
	pti->fdpsize=0;
	pti->fdpmax=0;
	delete [] pti->fdp;
	pti->fdp=NULL;
}

// map track info into memory
PDISKTRACKINFO CDiskImage::MapTrack(int cylinder, int head)
{
	// ignore wrong cylinder
	if (cylinder<0 || cylinder>=MAX_CYLINDER)
		return NULL;

	// ignore wrong head
	if (head<0 || head>=MAX_HEAD)
		return NULL;

	// return if available
	PDISKTRACKINFO pt=GetTrack(cylinder, head);
	if (pt)
		return pt;

	if (!dti) {
		// first allocation
		dtihed=MAX_HEAD;
		dticnt=((cylinder*dtihed)/DEF_TRACKINFO+1)*DEF_TRACKINFO;
		dticyl=dticnt/MAX_HEAD;
		dti=new DiskTrackInfo[dticnt];
		memset(dti, 0, sizeof(DiskTrackInfo)*dticnt);
	} else {
		// reallocation
		int scnt=dticnt;
		PDISKTRACKINFO sti=dti;
		dticnt=((cylinder*dtihed)/DEF_TRACKINFO+1)*DEF_TRACKINFO;
		dticyl=dticnt/MAX_HEAD;
		dti=new DiskTrackInfo[dticnt];
		memcpy(dti, sti, sizeof(DiskTrackInfo)*scnt);
		memset(dti+scnt, 0, sizeof(DiskTrackInfo)*(dticnt-scnt));
		delete [] sti;
	}

	return &dti[cylinder*dtihed+head];
}

// update diskimage information
void CDiskImage::UpdateImageInfo(PDISKTRACKINFO pti)
{
	if (!dii.valid) {
		// first time
		dii.umincylinder=pti->cylinder;
		dii.umaxcylinder=pti->cylinder;
		dii.uminhead=pti->head;
		dii.umaxhead=pti->head;
		dii.valid=true;
	} else {
		// compare and update
		if (pti->cylinder < dii.umincylinder)
			dii.umincylinder=pti->cylinder;
		if (pti->cylinder > dii.umaxcylinder)
			dii.umaxcylinder=pti->cylinder;
		if (pti->head < dii.uminhead)
			dii.uminhead=pti->head;
		if (pti->head > dii.umaxhead)
			dii.umaxhead=pti->head;
	}
}



// create packed date.time
void CDiskImage::CreateDateTime(PCAPSDATETIME pcd)
{
	// cancel on error
	if (!pcd)
		return;

	// read OS time, since time_t won't work in a few decades... :)
	SYSTEMTIME st;
	GetLocalTime(&st);

	// create date
	UDWORD t=0;
	t+=st.wYear*10000;
	t+=st.wMonth*100;
	t+=st.wDay;
	pcd->date=t;

	// create time
	t=0;
	t+=st.wHour*10000000;
	t+=st.wMinute*100000;
	t+=st.wSecond*1000;
	t+=st.wMilliseconds%1000;
	pcd->time=t;
}

// decode packed date.time
void CDiskImage::DecodeDateTime(PCAPSDATETIMEEXT dec, PCAPSDATETIME pcd)
{
	// cancel if no result buffer
	if (!dec)
		return;

	// clear result
	memset(dec, 0, sizeof(CapsDateTimeExt));

	// cancel if no source
	if (!pcd)
		return;

	// decode date
	UDWORD t=pcd->date;
	dec->year=t/10000;
	t%=10000;
	dec->month=t/100;
	t%=100;
	dec->day=t;

	// decode time;
	t=pcd->time;
	dec->hour=t/10000000;
	t%=10000000;
	dec->min=t/100000;
	t%=100000;
	dec->sec=t/1000;
	t%=1000;
	dec->tick=t;
}

// get name for cppid
LPCSTR CDiskImage::GetPlatformName(int pid)
{
	if (pid>=cppidNA && pid<cppidLast)
		return pidname[pid];
	else
		return NULL;
}

// calculate CRC32 on file
UDWORD CDiskImage::CrcFile(PCAPSFILE pcf)
{
	UDWORD crc=0;

	// shortcut for memory files
	if (pcf->flag & CFF_MEMMAP) {
		if (!pcf->memmap || pcf->size<0)
			return crc;

		return CalcCRC(pcf->memmap, pcf->size);
	}

	// open file
	CCapsFile file;
	if (file.Open(pcf))
		return crc;

	int len=file.GetSize();

	if (len) {
		int bufsize=DEF_CRCBUF;
		PUBYTE buf=new UBYTE[bufsize];

		// calculate CRC32 on file
		while (len) {
			int size=len > bufsize ? bufsize : len;
			if (file.Read(buf, size) != size) {
				crc=0;
				break;
			}

			crc=CalcCRC32(buf, size, crc);
			len-=size;
		}

		delete [] buf;
	}

	return crc;
}

// read network ordered value
UDWORD CDiskImage::ReadValue(PUBYTE buf, int cnt)
{
	UDWORD val;

	for (val=0; cnt > 0; cnt--) {
		val<<=8;
		val|=*buf++;
	}

	return val;
}

// add flakey data
PDISKDATAMARK CDiskImage::AddFD(PDISKTRACKINFO pti, PDISKDATAMARK src, int size, int units)
{
	// cancel if nothing to add
	if (!src || size<=0)
		return NULL;

	// allocate and copy to buffer
	PDISKDATAMARK alloc=AllocFD(pti, size, units);
	if (alloc)
		memcpy(alloc, src, size*sizeof(DiskDataMark));

	return alloc;
}

// allocate flakey data
PDISKDATAMARK CDiskImage::AllocFD(PDISKTRACKINFO pti, int size, int units)
{
	// cancel if invalid allocation
	if (!pti)
		return NULL;

	// get buffer mark
	if (size <= 0)
		return pti->fdp+pti->fdpsize;

	// grow buffer if necessary
	if (pti->fdpsize+size > pti->fdpmax) {
		units+=pti->fdpsize+size;
		PDISKDATAMARK fdp=pti->fdp;
		pti->fdp=new DiskDataMark[units];
		if (pti->fdpsize)
			memcpy(pti->fdp, fdp, pti->fdpsize*sizeof(DiskDataMark));
		delete [] fdp;
		pti->fdpmax=units;
	}

	// allocate new data
	PDISKDATAMARK alloc=pti->fdp+pti->fdpsize;
	memset(alloc, 0, size*sizeof(DiskDataMark));
	pti->fdpsize+=size;
	return alloc;
}

// allocate sector information buffer
void CDiskImage::AllocTrackSI(PDISKTRACKINFO pti)
{
	// cancel if invalid allocation
	if (!pti)
		return;

	FreeTrackSI(pti);

	// allocate buffer if needed
	if (pti->sectorcnt > 0) {
		int size=pti->sectorcnt;
		pti->sip=new DiskSectorInfo[size];
		pti->sipsize=size;
		memset(pti->sip, 0, size*sizeof(DiskSectorInfo));
	}
}

// free sector information buffer
void CDiskImage::FreeTrackSI(PDISKTRACKINFO pti)
{
	// cancel if invalid track data
	if (!pti)
		return;

	// free sector info buffer
	pti->sipsize=0;
	delete [] pti->sip;
	pti->sip=NULL;
}

