#ifndef CTRAWCODEC_H
#define CTRAWCODEC_H

// CAPS work helper
struct CapsWH {
	CapsRaw cr;
	PUDWORD timbuf;
	int timlen;
	PUBYTE cdbuf;
	int cdlen;
	PUBYTE cdmem;
	PUBYTE rawbuf;
	int rawlen;
	PUBYTE trkbuf[CAPS_MTRS];
	int trklen[CAPS_MTRS];
	int trkcnt;
	PUBYTE ctbuf;
	int ctlen;
	PUBYTE ctmem;
	PUBYTE txsrc;
	int txlen;
	int txact;
	PUBYTE ctstore;
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
	int DecompressDump(PUBYTE buf, int len);
	int DecompressDensity(int verify = 0);
	int DecompressTrack(int verify = 0);
	int CompressDensity();
	int CompressTrack();
	static void Swap(PUDWORD buf, int cnt);

protected:
	void Clear();
	void FreeCompressedDensity();
	void FreeUncompressedDensity();
	void FreeCompressedTrack();
	void FreeUncompressedTrack();
	static void FreeUncompressedTrack(PCAPSWH w);

	static PUDWORD DecompressDensity(PUBYTE src, int slen, PUDWORD dst = NULL);
	static PCAPSWH DecompressTrack(PCAPSWH w, PUBYTE src, int slen, PUBYTE dst = NULL);
	static void DecompressTrackData(PCAPSWH w);
	static PCAPSPACK GetPackHeader(PCAPSPACK cpk, PUBYTE src, int slen);
	static UDWORD CTR(PCAPSWH w, int size);

	int EncodeDensity(int equ, int epos, int dpos, int dcnt);
	void CompressTrackData();
	void EncodeData(int dpos, int dcnt, int epos, int ecnt, int shf);
	static void CTW(PCAPSWH w, UDWORD value, int size);
	
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
