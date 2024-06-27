#ifndef CAPSLOADER_H
#define CAPSLOADER_H



// CAPS image generic loader
class CCapsLoader  
{
public:
	enum {
		ccidCaps,
		ccidErrFile,
		ccidErrType,
		ccidErrShort,
		ccidErrHeader,
		ccidErrData,
		ccidEof,
		ccidUnknown,
		ccidFile,
		ccidDump,
		ccidData,
		ccidTrck,
		ccidInfo,
		ccidImge,
		ccidCtex,
		ccidCtei
	};

	CCapsLoader();
	virtual ~CCapsLoader();
	int Lock(PCAPSFILE pcf);
	void Unlock();
	int ReadChunk(int idbrk=false);
	int SkipData();
	int ReadData(PUBYTE buf);
	int GetDataSize();
	int GetPosition();
	int SetPosition(int pos);
	PCAPSCHUNK GetChunk();
	PCCAPSFILE GetFile();
	static void ConvertChunk(PCAPSCHUNK pc);
	static void Swap(PUDWORD buf, int cnt);

protected:
	struct ChunkType {
		LPCSTR name;
		int type;
	};

	typedef ChunkType *PCHUNKTYPE;

	static int GetChunkType(PCAPSCHUNK pc);

protected:
	int readmode;
	CCapsFile file;
	int flen;
	CapsGeneric xchunk;
	CapsChunk chunk;
	static ChunkType chunklist[];
};

typedef CCapsLoader *PCCAPSLOADER;



// access chunk data
inline PCAPSCHUNK CCapsLoader::GetChunk()
{
	return &chunk;
}

// access file directly
inline PCCAPSFILE CCapsLoader::GetFile()
{
	return &file;
}

#endif
