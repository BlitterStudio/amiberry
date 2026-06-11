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
	int Lock(std::unique_ptr<CBaseFile> pf);
	void Unlock();
	int ReadChunk(int idbrk = false);
	uint32_t SkipData();
	uint32_t ReadData(uint8_t *buf);
	uint32_t GetDataSize();
	file_pos_t GetPosition();
	file_pos_t SetPosition(file_pos_t pos);
	PCAPSCHUNK GetChunk();
	static void ConvertChunk(PCAPSCHUNK pc);
	static void Swap(void *buf, size_t cnt);

protected:
	struct ChunkType {
		const char *name;
		int type;
	};

	typedef ChunkType *PCHUNKTYPE;

	static int GetChunkType(PCAPSCHUNK pc);

protected:
	std::unique_ptr<CBaseFile> file;
	CapsGeneric xchunk;
	CapsChunk chunk;
	static const ChunkType chunklist[];
};

typedef CCapsLoader *PCCAPSLOADER;



// access chunk data
inline PCAPSCHUNK CCapsLoader::GetChunk()
{
	return &chunk;
}

#endif
