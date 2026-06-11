#include "stdafx.h"



CCTRawCodec::CCTRawCodec()
{
	Clear();
}

CCTRawCodec::~CCTRawCodec()
{
	Free();
}

// clear variables
void CCTRawCodec::Clear()
{
	wh = {};
}

// free all allocated buffers
void CCTRawCodec::Free()
{
	FreeCompressedDensity();
	FreeUncompressedDensity();
	FreeCompressedTrack();
	FreeUncompressedTrack();
	Clear();
}

// free compressed CT Raw density information
void CCTRawCodec::FreeCompressedDensity()
{
	delete[] wh.cdbuf;
	wh.cdbuf = nullptr;
	wh.cdlen = 0;
}

// free uncompressed CT Raw density information
void CCTRawCodec::FreeUncompressedDensity()
{
	delete[] wh.timbuf;
	wh.timbuf = nullptr;
	wh.timlen = 0;
}

// free compressed CT Raw track information
void CCTRawCodec::FreeCompressedTrack()
{
	delete[] wh.ctbuf;
	wh.ctbuf = nullptr;
	wh.ctlen = 0;
}

// free uncompressed CT Raw track information
void CCTRawCodec::FreeUncompressedTrack()
{
	FreeUncompressedTrack(&wh);
}

// free uncompressed CT Raw track information
void CCTRawCodec::FreeUncompressedTrack(PCAPSWH w)
{
	delete[] w->rawbuf;
	w->rawbuf = nullptr;
	w->rawlen = 0;

	for (int trk = 0; trk < CAPS_MTRS; trk++) {
		w->trkbuf[trk] = nullptr;
		w->trklen[trk] = 0;
	}

	w->trkcnt = 0;
}



// set data to/from network byte order on little-endian host
void CCTRawCodec::Swap(void *buf, size_t cnt)
{
#ifdef LITTLE_ENDIAN
	auto bbuf = static_cast<uint8_t *>(buf);

	for (cnt >>= 2; cnt > 0; cnt--, bbuf += sizeof(uint32_t)) {
		uint32_t val = CBitBuffer::ReadBit32(bbuf);
		CBitBuffer::WriteBitLE32(bbuf, val);
	}
#endif
}

