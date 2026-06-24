#include "stdafx.h"



// valid chunks
const CCapsLoader::ChunkType CCapsLoader::chunklist[] = {
	CAPS_IDFILE, ccidFile,
	CAPS_IDDUMP, ccidDump,
	CAPS_IDDATA, ccidData,
	CAPS_IDTRCK, ccidTrck,
	CAPS_IDINFO, ccidInfo,
	CAPS_IDIMGE, ccidImge,
	CAPS_IDCTEX, ccidCtex,
	CAPS_IDCTEI, ccidCtei,
	nullptr, ccidUnknown
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
int CCapsLoader::Lock(std::unique_ptr<CBaseFile> pf)
{
	// close previous file
	Unlock();

	// error if the input file does not exist or it's not already open
	if (!pf || !pf->IsOpen())
		return ccidErrFile;

	// own the file
	file = std::move(pf);

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
	file.reset();
	chunk.type = ccidUnknown;
}

// read caps chunk
int CCapsLoader::ReadChunk(int idbrk)
{
	// error if the input file does not exist or it's not already open
	if (!file || !file->IsOpen())
		return ccidErrFile;

	// skip data area if any
	SkipData();

	// check EOF
	auto rem_size = file->GetRemainingSize();
	if (!rem_size)
		return ccidEof;

	// check for ID chunk size
	if (rem_size < sizeof(CapsID))
		return ccidErrShort;

	// read ID chunk
	if (file->Read(&xchunk.file, sizeof(CapsID)) != sizeof(CapsID))
		return ccidErrShort;
	chunk.cg.file = xchunk.file;

	// identify chunk type if possible
	int type = GetChunkType(&chunk);

	// stop if break on unknown types
	if (idbrk && type == ccidUnknown)
		return ccidUnknown;

	// set ID to host format
	Swap((uint8_t *)&chunk.cg.file + sizeof(chunk.cg.file.name), sizeof(CapsID) - sizeof(chunk.cg.file.name));

	// error if the chunk size is smaller than the ID size
	if (chunk.cg.file.size < sizeof(CapsID))
		return ccidErrHeader;

	// load modifier for known chunk types, skip for unknown types
	auto modsize = chunk.cg.file.size - sizeof(CapsID);
	if (modsize > 0) {
		// check for valid modifier size
		if (file->GetRemainingSize() < modsize)
			return ccidErrShort;

		// read modifier chunk if buffer has enough space or skip it
		if (modsize <= sizeof(CapsMod)) {
			// read modifier chunk
			if (file->Read(&xchunk.mod, modsize) != modsize)
				return ccidErrShort;

			// set modifier to host format
			chunk.cg.mod = xchunk.mod;
			Swap(&chunk.cg.mod, modsize);
		} else {
			// seek to new chunk
			file->Seek(modsize, CBaseFile::Current);
		}
	}

	// check CRC value for completely read headers
	if (modsize <= sizeof(CapsMod)) {
		xchunk.file.hcrc = 0;
		if (chunk.cg.file.hcrc != CalcCRC((uint8_t *)&xchunk.file, chunk.cg.file.size))
			return ccidErrHeader;
	}

	return type;
}

// skip data area of data chunk
uint32_t CCapsLoader::SkipData()
{
	// nothing skipped if the input file does not exist or it's not already open
	if (!file || !file->IsOpen())
		return 0;

	// do not skip next time, clear type
	int type = chunk.type;
	chunk.type = ccidUnknown;

	// skip only data area
	if (type != ccidData)
		return 0;

	// do not skip empty area
	auto skip = chunk.cg.mod.data.size;
	if (!skip)
		return 0;

	// correct skip size if image is short
	auto rem_size = file->GetRemainingSize();
	if (rem_size < skip)
		skip = static_cast<decltype(skip)>(rem_size);

	// seek to new chunk
	file->Seek(skip, CBaseFile::Current);
	return skip;
}

// read data area of data chunk
uint32_t CCapsLoader::ReadData(uint8_t *buf)
{
	// nothing read if the input file does not exist or it's not already open
	if (!file || !file->IsOpen())
		return 0;

	// do not read next time, clear type
	int type = chunk.type;
	chunk.type = ccidUnknown;

	// read only data area
	if (type != ccidData)
		return 0;

	// do not read empty area
	auto size = chunk.cg.mod.data.size;
	if (!size)
		return 0;

	// skip to file end and read nothing if image is short
	auto rem_size = file->GetRemainingSize();
	if (rem_size < size) {
		file->Seek(0, CBaseFile::End);
		return 0;
	}

	// read data area, return 0 on error
	if (file->Read(buf, size) != size)
		return 0;

	// check data CRC if available and return 0 on error
	if (chunk.cg.mod.data.dcrc)
		if (chunk.cg.mod.data.dcrc != CalcCRC(buf, size))
			return 0;

	return size;
}

// get data area size of data chunk
uint32_t CCapsLoader::GetDataSize()
{
	// no data if the input file does not exist or it's not already open
	if (!file || !file->IsOpen())
		return 0;

	// only data area is allowed
	if (chunk.type != ccidData)
		return 0;

	// size in bytes
	return chunk.cg.mod.data.size;
}

// get file position
file_pos_t CCapsLoader::GetPosition()
{
	// return 0 if the input file does not exist or it's not already open
	if (!file || !file->IsOpen())
		return 0;

	return file->GetPosition();
}

// set file position
file_pos_t CCapsLoader::SetPosition(file_pos_t pos)
{
	// stop if the input file does not exist or it's not already open
	if (!file || !file->IsOpen())
		return 0;

	// chunk type unknown after seek
	chunk.type = ccidUnknown;

	// keep file boundaries
	if (pos < 0) {
		pos = 0;
	} else {
		std::make_unsigned_t<decltype(pos)> upos = pos;
		if (upos > file->GetSize())
			pos = static_cast<decltype(pos)>(file->GetSize());
	}

	file->Seek(pos, CBaseFile::Position);

	return pos;
}

// find chunk type
int CCapsLoader::GetChunkType(PCAPSCHUNK pc)
{
	int pos;

	for (pos = 0; chunklist[pos].name; pos++)
		if (std::equal(chunklist[pos].name, chunklist[pos].name + sizeof(pc->cg.file.name), pc->cg.file.name))
			break;

	return pc->type = chunklist[pos].type;
}

// convert chunk to its original network ordering
void CCapsLoader::ConvertChunk(PCAPSCHUNK pc)
{
	if (!pc)
		return;

	// make sure that the chunk size is valid and is a multiple of 4 (each element must be a 32 bit integer)
	auto ofs = sizeof(pc->cg.file.name);
	auto size = pc->cg.file.size;
	if (size < ofs || (size & 3))
		return;

	// set complete chunk to network byte order
	Swap((uint8_t *)&pc->cg.file + ofs, size - ofs);

	// calculate crc and set to network byte order
	pc->cg.file.hcrc = 0;
	pc->cg.file.hcrc = CalcCRC((uint8_t *)&pc->cg.file, size);
	CCapsLoader::Swap(&pc->cg.file.hcrc, sizeof(pc->cg.file.hcrc));
}

// set data to/from network byte order on little-endian host
void CCapsLoader::Swap(void *buf, size_t cnt)
{
#ifdef LITTLE_ENDIAN
	auto bbuf = static_cast<uint8_t *>(buf);

	for (cnt >>= 2; cnt > 0; cnt--, bbuf += sizeof(uint32_t)) {
		uint32_t val = CBitBuffer::ReadBit32(bbuf);
		CBitBuffer::WriteBitLE32(bbuf, val);
	}
#endif
}
