#ifndef CAPSIMAGESTD_H
#define CAPSIMAGESTD_H

#define CIMG_MFMNOISE 0x10030f01
#define CIMG_OVERLAPBIT 3
#define MAX_STREAMBIT 32

// image block data descriptor
struct ImageBlockInfo {
	uint32_t blockbits; // decoded block size in bits
	uint32_t gapbits;   // decoded gap size in bits
	uint32_t gapoffset; // offset of gap stream in data area
	uint32_t celltype; // bitcell type
	uint32_t enctype;  // encoder type
	uint32_t flag; // block flags
	uint32_t gapvalue; // default gap value
	uint32_t dataoffset; // offset of data stream in data area
	int fdenc; // encoding type of the first data written
	uint32_t fdbitpos; // bit position of the first encoded data
};

typedef ImageBlockInfo *PIMAGEBLOCKINFO;

// encoding type
enum {
	ibieNA,  // no data written
	ibieRaw, // raw data
	ibieMFM, // mfm encoding
	ibieWeak // weak bits
};

// image stream decoder data
struct ImageStreamInfo {
	int strtype; // stream type
	int actblock; // current block#
	int enctype; // block encoder type
	int actenctype; // active encoder type
	int sizemodebit; // 1 if data sizes are encoded as bitcounts
	uint32_t strstart; // stream start offset in buffer
	uint32_t strend; // stream end offset in buffer
	uint32_t strofs; // offset of current stream data being processed from strbase
	uint32_t strsize; // maximum size of stream
	uint8_t *strbase; // stream start position in buffer
	uint8_t gapstr[4]; // generated gap stream when no gap stream is not present at all
	uint8_t weakdata[4]; // buffer for weak data sample
	int readresult; // error code after stream read
	int readend; // 1 if stream end has been reached
	uint32_t readvalue; // result of stream read operation
	int allowloop; // 1 if looping on zero total bitcount is allowed, ie gap
	int setencmode; // encoding set for the actaul data element
	uint32_t streambc; // total bitcount for the actual data element set in the stream, 0 undefined/loop for gap
	uint32_t samplebc; // number of valid sample data bits of the actual element
	uint32_t remstreambc; // remaining bits of the actual stream element
	uint32_t remsamplebc; // remaining number of valid sample data bits of the actual element
	uint32_t sampleofs; // byte offset in sample data buffer
	uint32_t samplemask; // current bit masked in sample data buffer; 0x80...01
	uint8_t *samplebase; // sample data buffer start for the actual element
	uint32_t prcbitpos; // current bit position for stream process
	int prcrembc; // remaining bit count for stream process
	int prcskipbc; // number of bits to skip during stream process
	uint32_t prcencstate; // encoding state, 1: new data, 0: continue existing data, e.g. MFM
	int prcwritebc; // total bit count written
	uint32_t loopofs; // loop offset in data stream, replace stream size to loop once reached
	int loopsize; // size of looped sample without encoding applied
	int looptype; // loop type found/generated
	int esfixbc; // stream encoding fixed size
	int esloopbc; // stream encoding loop size
	int scenable; // true if sample size change is enabled
	uint32_t scofs; // sample size change offset
	int scmul; // sample size change multiplier; original size+scmul*sample size
};

typedef ImageStreamInfo *PIMAGESTREAMINFO;

// stream types
enum {
	isitData, // data stream
	isitGap0, // gap stream forward, end of current block -> start of next block
	isitGap1  // gap stream backward, start of next block -> end of current block
};

// encoding mode set
enum {
	isiemRaw,  // use stream directly as physical raw data
	isiemType, // encode using the active encoder type
	isiemWeak  // weak bits, physical size by encoder type
};

// loop types
enum {
	isiltNone,   // sample loop not allowed
	isiltAuto,   // sample loop automatically generated; not in stream
	isiltStream  // sample loop found in stream
};

// disk decoder work area
struct DiskDecoderInfo {
	uint8_t *track; // track buffer
	uint32_t trackbc; // size of track buffer in bits, may hold multiple revolutions
	uint32_t singletrackbc; // size of track buffer in bits, one revolution
	uint32_t dsctrackbc; // size of track calculated from block descriptors
	uint32_t dscdatabc; // size of data blocks calculated from block descriptors
	uint32_t dscgapbc; // size of gap blocks calculated from block descriptors
	uint32_t dscstartbit; // bit position where track encoding starts
	uint32_t encbitpos; // bit position during and after encoding a block
	int encwritebc; // total number of bits written during encoding
	int encgsvalid; // true if gap split is valid
	uint32_t encgapsplit; // bit position where gap was split
	uint8_t *data; // source data buffer
	int datasize; // size of data buffer
	int datacount; // number of valid data size
	PIMAGEBLOCKINFO block; // block descriptor
	int blocksize; // block descriptor maximum size
	int blockcount; // number of valid block descriptors
	uint32_t flag; // flags selected
	PDISKTRACKINFO pdt; // current track decoded
};

typedef DiskDecoderInfo *PDISKDECODERINFO;



// CAPS standard image handler
class CCapsImageStd : public CDiskImage
{
public:
	CCapsImageStd();
	virtual ~CCapsImageStd();
	int Lock(PCAPSFILE pcf);
	int Unlock();

protected:
	struct ScanInfo {
		int track;
		int data;
	};

protected:
	int LoadTrack(PDISKTRACKINFO pti, UDWORD flag);
	int ScanImage();
	int UpdateImage(int group);
	int UpdateWeakBit(int group);
	void UpdateOverlap();
	int DecodeImage();
	int ProcessImage();
	virtual int CompareImage();
	virtual int DecompressDump();
	virtual int UpdateDump();

	int DecodeDensity(PDISKTRACKINFO pti, PUBYTE buf, UDWORD flag);
	int ConvertDensity(PDISKTRACKINFO pti);
	int GenerateNoiseTrack(PDISKTRACKINFO pti);
	int GenerateNoiseDensity(PDISKTRACKINFO pti);
	int GenerateAutoDensity(PDISKTRACKINFO pti);
	int GenerateCLA(PDISKTRACKINFO pti, PUBYTE buf);
	int GenerateCLA2(PDISKTRACKINFO pti, PUBYTE buf);
	int GenerateCLST(PDISKTRACKINFO pti, PUBYTE buf);
	int GenerateSLA(PDISKTRACKINFO pti, PUBYTE buf);
	int GenerateSLA2(PDISKTRACKINFO pti, PUBYTE buf);
	int GenerateABA(PDISKTRACKINFO pti, PUBYTE buf);
	int GenerateABA2(PDISKTRACKINFO pti, PUBYTE buf);

	void InitSystem();
	void Clear();
	void AllocDiskData(int maxsize);
	void AllocImageBlock(int maxsize);
	void FreeDiskData();
	void FreeImageBlock();
	void FreeDecoder();
	int CheckEncoder(int encoder, int revision);
	int GetBlock(PIMAGEBLOCKINFO pi, int blk);
	int GetBlock(PCAPSBLOCK pb, int blk);
	int InitDecoder();
	int InitStream(PIMAGESTREAMINFO pstr, int strtype, int blk);
	int InitDataStream(PIMAGESTREAMINFO pstr);
	int InitGapStream(PIMAGESTREAMINFO pstr);
	int FindGapStreamEnd(PIMAGESTREAMINFO pstr, int next);
	int ResetStream(PIMAGESTREAMINFO pstr);
	int ReadSampleInit(PIMAGESTREAMINFO pstr);
	int ReadSample(PIMAGESTREAMINFO pstr, int maxbc);
	int GetSample(PIMAGESTREAMINFO pstr);
	int GetSampleRaw(PIMAGESTREAMINFO pstr);
	int GetSampleGap(PIMAGESTREAMINFO pstr);
	int GetSampleData(PIMAGESTREAMINFO pstr);
	int ProcessStream(PIMAGESTREAMINFO pstr, uint32_t bitpos, int maxbc, int skipbc, int encnew);
	void ProcessStreamRaw(PIMAGESTREAMINFO pstr);
	void ProcessStreamMFM(PIMAGESTREAMINFO pstr);
	void ProcessStreamWeak(PIMAGESTREAMINFO pstr);
	int CalculateStreamSize(PIMAGESTREAMINFO pstr);
	int GetEncodedSize(PIMAGESTREAMINFO pstr, int bitcnt);
	int FindLoopPoint(PIMAGESTREAMINFO pstr);
	int ProcessBlock(int blk, uint32_t bitpos, int datasize, int gapsize);
	int ProcessBlockData(int blk, int datasize);
	int ProcessBlockGap(int blk, int gapsize);
	int ProcessBlockGap(PIMAGESTREAMINFO pg, int gapsize);
	int ProcessBlockGap(PIMAGESTREAMINFO pg0, PIMAGESTREAMINFO pg1, int gapsize);
	int ProcessBlockGap(PIMAGESTREAMINFO pg0, PIMAGESTREAMINFO pg1, int gapsize, int loopsel);
	void SetLoop(PIMAGESTREAMINFO pg, int value);
	void GetLoop(PIMAGESTREAMINFO pg);
	void MFMFixup();

protected:
	CBitBuffer trackbuf; // bit level access to track buffer
	CCapsLoader loader;
	DiskDecoderInfo di; // decoder work area
	int maxwritelen; // maximum write length allowed for bit operations
	int rawreadlen; // default stream read length for raw encoding
	int mfmreadlen; // default stream read length for mfm encoding
	uint32_t mfmindexmask; // valid bits mask for mfm encoding table index
	uint32_t mfmmsbclear; // bitclear mask for mfm encoding
};

typedef CCapsImageStd *PCCAPSIMAGESTD;

#endif
