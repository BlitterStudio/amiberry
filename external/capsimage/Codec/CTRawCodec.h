#ifndef CTRAWCODEC_H
#define CTRAWCODEC_H

// CAPS work helper
struct CapsWH {
	CapsRaw cr;
	uint32_t *timbuf;
	int timlen;
	uint8_t *cdbuf;
	int cdlen;
	uint8_t *cdmem;
	uint8_t *rawbuf;
	int rawlen;
	uint8_t *trkbuf[CAPS_MTRS];
	int trklen[CAPS_MTRS];
	int trkcnt;
	uint8_t *ctbuf;
	int ctlen;
	uint8_t *ctmem;
	uint8_t *txsrc;
	int txlen;
	int txact;
	uint8_t *ctstore;
	int ctstop;
};

typedef CapsWH *PCAPSWH;



// CT Raw codec wrapper - originally it's pure C code
class CCTRawCodec
{
public:
	CCTRawCodec();
	virtual ~CCTRawCodec();
	PCAPSWH GetInfo();
	void Free();
	int DecompressDump(uint8_t *buf, size_t len);
	int DecompressDensity(int verify = 0);
	int DecompressTrack(int verify = 0);
	int CompressDensity();
	int CompressTrack();
	static void Swap(void *buf, size_t cnt);

protected:
	void Clear();
	void FreeCompressedDensity();
	void FreeUncompressedDensity();
	void FreeCompressedTrack();
	void FreeUncompressedTrack();
	static void FreeUncompressedTrack(PCAPSWH w);

	static uint32_t *DecompressDensity(uint8_t *src, int slen, uint32_t *dst = nullptr);
	static PCAPSWH DecompressTrack(PCAPSWH w, uint8_t *src, int slen, uint8_t *dst = nullptr);
	static void DecompressTrackData(PCAPSWH w);
	static PCAPSPACK GetPackHeader(PCAPSPACK cpk, const uint8_t *src, size_t slen);
	static uint32_t ReadStream(PCAPSWH w, unsigned int size);

	int EncodeDensity(int equ, int epos, int dpos, int dcnt);
	void CompressTrackData();
	void EncodeData(int dpos, int dcnt, int epos, int ecnt, int shf);
	static void WriteStream(PCAPSWH w, uint32_t value, unsigned int size);

protected:
	CapsWH wh;
};

typedef CCTRawCodec *PCCTRAWCODEC;



// get codec work info
inline PCAPSWH CCTRawCodec::GetInfo()
{
	return &wh;
}

#endif
