#ifndef CAPSFORM_H
#define CAPSFORM_H

#pragma pack(push, 1)

// block data descriptor for converter
struct CapsFormatBlock {
	UDWORD gapacnt;   // gap before first am count
	UDWORD gapavalue; // gap before first am value
	UDWORD gapbcnt;   // gap after first am count
	UDWORD gapbvalue; // gap after first am value
	UDWORD gapccnt;   // gap before second am count
	UDWORD gapcvalue; // gap before second am value
	UDWORD gapdcnt;   // gap after second am count
	UDWORD gapdvalue; // gap after second am value
	UDWORD blocktype; // type of block
	UDWORD track;     // track#
	UDWORD side;      // side#
	UDWORD sector;    // sector#
	SDWORD sectorlen; // sector length in bytes
	PUBYTE databuf;   // source data buffer
	UDWORD datavalue; // source data value if buffer is NULL
};

typedef struct CapsFormatBlock *PCAPSFORMATBLOCK;

// track data descriptor for converter
struct CapsFormatTrack {
	UDWORD type;      // structure type
	UDWORD gapacnt;   // gap after index count
	UDWORD gapavalue; // gap after index value
	UDWORD gapbvalue; // gap before index value
	PUBYTE trackbuf;  // track buffer memory
	UDWORD tracklen;  // track buffer memory length
	UDWORD buflen;    // track buffer allocation size
	UDWORD bufreq;    // track buffer requested size
	UDWORD startpos;  // start position in buffer
	SDWORD blockcnt;  // number of blocks
	PCAPSFORMATBLOCK block; // block data
	UDWORD size;      // internal counter
};

typedef struct CapsFormatTrack *PCAPSFORMATTRACK;

#pragma pack(pop)

// block types
enum {
	cfrmbtNA=0,  // invalid type
	cfrmbtIndex, // index 
	cfrmbtData   // data
};

#endif
