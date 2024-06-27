#include "stdafx.h"



// valid chunks
CCapsLoader::ChunkType CCapsLoader::chunklist[]= {
	CAPS_IDFILE, ccidFile,
	CAPS_IDDUMP, ccidDump,
	CAPS_IDDATA, ccidData,
	CAPS_IDTRCK, ccidTrck,
	CAPS_IDINFO, ccidInfo,
	CAPS_IDIMGE, ccidImge,
	CAPS_IDCTEX, ccidCtex,
	CAPS_IDCTEI, ccidCtei,
	NULL, ccidUnknown
};



CCapsLoader::CCapsLoader()
{
	MakeCRCTable();
	Unlock();
}

CCapsLoader::~CCapsLoader()
{
	Unlock();
}

// lock file, test for CAPS header type
int CCapsLoader::Lock(PCAPSFILE pcf)
{
	// close previous file
	Unlock();

	// open file
	if (file.Open(pcf)) {
		Unlock();
		return ccidErrFile;
	}

	// set read mode
	readmode=(pcf->flag & CFF_WRITE) ? false : true;

	// get file size, no file skip
	flen=file.GetSize();

	// check for caps id
	if (ReadChunk(true) != ccidFile) {
		Unlock();
		return ccidErrType;
	}

	return ccidCaps;
}

// unlock file
void CCapsLoader::Unlock()
{
	file.Close();
	flen=0;
	chunk.type=ccidUnknown;
	readmode=true;
}

// read caps chunk
int CCapsLoader::ReadChunk(int idbrk)
{
	// handle file error
	if (!file.IsOpen())
		return ccidErrFile;

	// skip data area if any
	SkipData();

	// check EOF
	int pos=file.GetPosition();
	if (pos == flen)
		return ccidEof;

	// check for ID chunk size
	if (flen-pos < sizeof(CapsID))
		return ccidErrShort;

	// read ID chunk
	if (file.Read((PUBYTE)&xchunk.file, sizeof(CapsID)) != sizeof(CapsID))
		return ccidErrShort;
	chunk.cg.file=xchunk.file;

	// identify chunk type if possible
	int type=GetChunkType(&chunk);

	// stop if break on unknown types
	if (idbrk && type==ccidUnknown)
		return ccidUnknown;

	// set ID to host format
	Swap(PUDWORD((PUBYTE)&chunk.cg.file+sizeof(chunk.cg.file.name)), sizeof(CapsID)-sizeof(chunk.cg.file.name));

	// load modifier for known chunk types, skip for unknown types
	int modsize=chunk.cg.file.size-sizeof(CapsID);
	if (modsize > 0) {
		// check for valid modifier size
		pos=file.GetPosition();
		if (flen-pos < modsize)
			return ccidErrShort;

		// read mod chunk if buffer has enough space or skip it
		if (modsize <= sizeof(CapsMod)) {
			// read mod chunk
			if (file.Read((PUBYTE)&xchunk.mod, modsize) != modsize)
				return ccidErrShort;

			// set header to host format
			chunk.cg.mod=xchunk.mod;
			Swap(PUDWORD((PUBYTE)&chunk.cg.mod), modsize);
		} else {
			// seek to new chunk
			file.Seek(modsize, 0);
		}
	}

	// check CRC value for completely read headers
	if (modsize>=0 && modsize<=sizeof(CapsMod)) {
		xchunk.file.hcrc=0;
		if (chunk.cg.file.hcrc != CalcCRC((PUBYTE)&xchunk.file, chunk.cg.file.size))
			return ccidErrHeader;
	}

	return type;
}

// skip data area of data chunk
int CCapsLoader::SkipData()
{
	// handle file error
	if (!file.IsOpen())
		return 0;

	// do not skip next time, clear type
	int type=chunk.type;
	chunk.type=ccidUnknown;

	// skip only data area
	if (type != ccidData)
		return 0;

	// do not skip empty area
	int skip=chunk.cg.mod.data.size;
	if (!skip)
		return 0;

	// correct skip size if image is short
	int pos=file.GetPosition();
	if (flen-pos < skip)
		skip=flen-pos;

	// seek to new chunk
	file.Seek(skip, 0);
	return skip;
}

// read data area of data chunk
int CCapsLoader::ReadData(PUBYTE buf)
{
	// handle file error
	if (!file.IsOpen())
		return 0;

	// do not read next time, clear type
	int type=chunk.type;
	chunk.type=ccidUnknown;

	// skip only data area
	if (type != ccidData)
		return 0;

	// do not skip empty area
	int size=chunk.cg.mod.data.size;
	if (!size)
		return 0;

	// correct and skip size if image is short
	int pos=file.GetPosition();
	if (flen-pos < size) {
		size=flen-pos;

		// seek to new chunk
		file.Seek(size, 0);
		return 0;
	}

	// read data area
	if (file.Read(buf, size) != size)
		return 0;

	// check data CRC if available
	if (chunk.cg.mod.data.dcrc)
		if (chunk.cg.mod.data.dcrc != CalcCRC(buf, size))
			return 0;

	return size;
}

// get data area size of data chunk
int CCapsLoader::GetDataSize()
{
	// handle file error
	if (!file.IsOpen())
		return 0;

	// skip only data area
	if (chunk.type != ccidData)
		return 0;

	// size in bytes
	return chunk.cg.mod.data.size;
}

// get file position
int CCapsLoader::GetPosition()
{
	// handle file error
	if (!file.IsOpen())
		return 0;

	return file.GetPosition();
}

// set file position
int CCapsLoader::SetPosition(int pos)
{
	// handle file error
	if (!file.IsOpen())
		return 0;

	// chunk type unknown after seek
	chunk.type=ccidUnknown;

	// keep file boundaries
	if (pos < 0)
		pos=0;
	else
		if (flen < pos)
			pos=flen;

	// seek
	file.Seek(pos, -1);

	return pos;
}

// find chunk type
int CCapsLoader::GetChunkType(PCAPSCHUNK pc)
{
	int pos;

	for (pos=0; chunklist[pos].name; pos++)
		if (!memcmp(chunklist[pos].name, pc->cg.file.name, sizeof(pc->cg.file.name)))
			break;

	return pc->type=chunklist[pos].type;
}

// convert chunk to its original network ordering
void CCapsLoader::ConvertChunk(PCAPSCHUNK pc)
{
	if (!pc)
		return;

	// set complete chunk to network byte order
	int ofs=sizeof(pc->cg.file.name);
	int size=pc->cg.file.size;
	Swap(PUDWORD((PUBYTE)&pc->cg.file+ofs), size-ofs);

	// calculate crc and set to network byte order
	pc->cg.file.hcrc=0;
	pc->cg.file.hcrc=CalcCRC((PUBYTE)&pc->cg.file, size);
	CCapsLoader::Swap(&pc->cg.file.hcrc, sizeof(pc->cg.file.hcrc));
}

// set data to network byte order
void CCapsLoader::Swap(PUDWORD buf, int cnt)
{
#ifdef INTEL
	for (cnt>>=2; cnt > 0; cnt--, buf++) {
		UDWORD src=*buf;
		UDWORD dst=(_lrotl(src, 8)&0x00ff00ff) | (_lrotr(src, 8)&0xff00ff00);
		*buf=dst;
	}
#endif
}
